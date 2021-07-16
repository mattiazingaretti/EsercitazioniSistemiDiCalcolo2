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

void initMemory() {
    /** COMPLETE THE FOLLOWING CODE BLOCK
     *
     * Request the kernel to creare a shared memory, set its size to the size of
     * struct shared_memory, and map the shared memory in the shared_mem_ptr variable.
     * Initialize the shared memory to 0.
     **/
     shm_unlink(SH_MEM_NAME);
     
     fd_shm = shm_open(SH_MEM_NAME, O_CREAT |O_EXCL |O_RDWR, 0666);
     if(fd_shm <0){handle_error("Error in shm_open in initMemory in prod \n");}
     
     int ret = ftruncate(fd_shm, sizeof(struct shared_memory));
     if(ret <0){handle_error("Error in ftruncate in initMemory in prod \n");}
     
     myshm_ptr = (struct shared_memory*) mmap(0 , sizeof(struct shared_memory) , PROT_READ | PROT_WRITE , MAP_SHARED , fd_shm ,0);
	 if(myshm_ptr == MAP_FAILED){handle_error("Error in mmap in initMemory in prod \n");}
	 
	 for(int i = 0; i< BUFFER_SIZE; ++i)
		myshm_ptr-> buf[i] = 0;
		
	myshm_ptr->read_index = 0 ;
	myshm_ptr->write_index = 0 ;
	
}

void closeMemory() {
    /** COMPLETE THE FOLLOWING CODE BLOCK
     *
     * unmap the shared memory, unlink the shared memory and close its descriptor
     **/
	int ret = munmap(myshm_ptr , sizeof(struct shared_memory));
    if(ret <0){handle_error("Error in munmap in closeMemory in prod \n");}

	ret = close(fd_shm);
	if(ret <0){handle_error("Error in close in closeMemory in prod \n");}

	ret = shm_unlink(SH_MEM_NAME);
	if(ret <0){handle_error("Error in shm_unlink in closeMemory in prod \n");}

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
    int localSum,next_pos = 0;
    while (numOps > 0) {
        // producer, just do your thing!
        int value = performRandomTransaction();
        /**
         * Complete the following code
         * check that we can write
         * write value in the buffer inside the shared memory and update the producer position
         */
		
		//Controllo buffer not full
		if((myshm_ptr-> write_index+1)%BUFFER_SIZE != myshm_ptr->read_index){
			myshm_ptr->buf[myshm_ptr->write_index] = value;
			myshm_ptr-> write_index = (myshm_ptr-> write_index+1)%BUFFER_SIZE;
		}
		
			localSum += value;
			numOps--;
        
    }
    printf("Producer %d ended. Local sum is %d\n", id, localSum);
}

int main(int argc, char** argv) {
    srand(PRNG_SEED);
    initMemory();

    produce(0, OPS_PER_PRODUCER);

    printf("Producer has terminated. Exiting...\n");
    closeMemory();

    exit(EXIT_SUCCESS);
}

