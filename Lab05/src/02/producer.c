#include <string.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>       // nanosleep()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include "common.h"

// definizione struttura memoria
struct shared_memory {
    int buf [BUFFER_SIZE];
    int read_index;
    int write_index;
};

//definizione shared memory
struct shared_memory *myshm_ptr;
int fd_shm;

//definizione semafori named
sem_t *sem_empty, *sem_filled, *sem_cs;

void initMemory() {
    /** COMPLETE THE FOLLOWING CODE BLOCK
     *
     * Request the kernel to creare a shared memory, set its size to the size of
     * struct shared_memory, and map the shared memory in the shared_mem_ptr variable.
     * Initialize the shared memory to 0.
    **/
    shm_unlink(SH_MEM_NAME); //Prevent previous executions failures.
    
	fd_shm = shm_open(SH_MEM_NAME , O_CREAT | O_EXCL | O_RDWR, 0666 );
	if(fd_shm < 0 ) {handle_error("Error in shm_open in initMemory in producer \n");}
	
	int ret = ftruncate(fd_shm, sizeof(struct shared_memory));
	if(ret) {handle_error("Error in ftruncate in initMemory in producer \n");}
	
	myshm_ptr = (struct shared_memory*) mmap(0 , sizeof(struct shared_memory) , PROT_READ | PROT_WRITE , MAP_SHARED , fd_shm ,0  );
	if(myshm_ptr == MAP_FAILED ){handle_error("Error in mmap in initMemory in producer \n");}
	
	for(int i = 0 ; i < BUFFER_SIZE ; ++i)
		myshm_ptr-> buf[i] = 0 ;
			
	myshm_ptr-> read_index = BUFFER_SIZE-1;
	myshm_ptr-> write_index = 0;
	
	
}

void closeMemory() {
    /** COMPLETE THE FOLLOWING CODE BLOCK
     *
     * unmap the shared memory, unlink the shared memory and close its descriptor
     **/
     
	int ret;
	ret = munmap(myshm_ptr , sizeof(struct shared_memory));
	if(ret){handle_error("Error in munmap in closeMemory in producer \n");}

	ret = close(fd_shm);
	if(ret){handle_error("Error in close in closeMemory in producer \n");}

	ret = shm_unlink(SH_MEM_NAME); 
	if(ret){handle_error("Error in shm_unlink in closeMemory in producer \n");}
}



void initSemaphores() {
    // delete state semaphores from a previous crash (if any)
    sem_unlink(SEMNAME_FILLED);
    sem_unlink(SEMNAME_EMPTY);
    sem_unlink(SEMNAME_CS_PROD);
    sem_unlink(SEMNAME_CS_CONS);

    sem_filled = sem_open(SEMNAME_FILLED, O_CREAT | O_EXCL, 0600, 0);
    if (sem_filled == SEM_FAILED) handle_error("sem_open filled");

    sem_empty = sem_open(SEMNAME_EMPTY, O_CREAT | O_EXCL, 0600, BUFFER_SIZE);
    if (sem_empty == SEM_FAILED) handle_error("sem_open empty");

    sem_cs = sem_open(SEMNAME_CS_PROD, O_CREAT | O_EXCL, 0600, 1);
    if (sem_cs == SEM_FAILED) handle_error("sem_open cs prod");
}

void closeSemaphores() {
    int ret = sem_close(sem_filled);
    if (ret) handle_error("sem_close filled");

    ret = sem_close(sem_empty);
    if (ret) handle_error("sem_close empty");

    ret = sem_close(sem_cs);
    if (ret) handle_error("sem_close cs");
}

// generates a number between -MAX_TRANSACTION and +MAX_TRANSACTION
static inline int performRandomTransaction() {
    struct timespec pause = {0};
    pause.tv_nsec = 10000000; // 10 ms (100*10^6 ns)
    nanosleep(&pause, NULL);

    int amount = rand() % (2 * MAX_TRANSACTION); // {0, ..., 2*MAX_TRANSACTION - 1}
    if (amount >= MAX_TRANSACTION) {
        return MAX_TRANSACTION - (amount+1); // {-MAX_TRANSACTION, ..., -1}
    } else {
        return amount + 1; // {1, ..., MAX_TRANSACTION}
    }
}

void produce(int id, int numOps) {
    int localSum = 0;
    while (numOps > 0) {
        // producer, just do your thing!
        int value = performRandomTransaction();
		printf("value in random transaction : %d \n ", value);
        int ret = sem_wait(sem_empty);
        if (ret) handle_error("sem_wait empty\n");

        ret = sem_wait(sem_cs);
        if (ret) handle_error("sem_wait cs");

        /**
         * Complete the following code:
         * write value in the buffer inside the shared memory and update the producer position
         */
         myshm_ptr->buf[myshm_ptr->write_index] = value;
         myshm_ptr->write_index = (myshm_ptr->write_index +1) % BUFFER_SIZE;


        ret = sem_post(sem_cs);
        if (ret) handle_error("sem_post cs");

        ret = sem_post(sem_filled);
        if (ret) handle_error("sem_post filled");

        localSum += value;
        numOps--;
    }
    printf("Producer %d ended. Local sum is %d\n", id, localSum);
}

int main(int argc, char** argv) {
    srand(PRNG_SEED);
    initSemaphores();
    initMemory();

    int i, ret;
    for (i=0; i<NUM_PRODUCERS; ++i) {
        pid_t pid = fork();
        if (pid == -1) {
            handle_error("fork");
        } else if (pid == 0) {
            produce(i, OPS_PER_PRODUCER);
            _exit(EXIT_SUCCESS);
        }
    }

    for (i=0; i<NUM_PRODUCERS; ++i) {
        int status;
        ret = wait(&status);
        if (ret == -1) handle_error("wait");
        if (WEXITSTATUS(status)) handle_error_en(WEXITSTATUS(status), "child crashed");
    }

    printf("Producers have terminated. Exiting...\n");
    closeSemaphores();
    closeMemory();

    exit(EXIT_SUCCESS);
}

