#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

// macros for error handling
#include "common.h"

#define N 10   // child process count
#define M 10    // thread per child process count
#define T 3     // time to sleep for main process

#define FILENAME	"accesses.log"

//Semaphore/Named Semaphore constants
#define READY_SEM_NAME "/ready_sem"
#define START_SEM_NAME "/start_sem"
#define HALT_SEM_NAME "/halt_sem"

/*
 * data structure required by threads
 */
typedef struct thread_args_s {
    unsigned int child_id;
    unsigned int thread_id;
} thread_args_t;

/*
 * parameters can be set also via command-line arguments
 */
int n = N, m = M, t = T;

/* TODO: declare as many semaphores as needed to implement
 * the intended semantics, and choose unique identifiers for
 * them (e.g., "/mysem_critical_section") */

sem_t* ready_sem; //Named semaphore
sem_t* start_sem; //Named semaphore
sem_t* halt_sem;  //Named semaphore
sem_t* file_sem;  //Binary Semaphore

/*
 * Ensures that an empty file with given name exists.
 */
void init_file(const char *filename) {
    printf("[Main] Initializing file %s...", FILENAME);
    fflush(stdout);
    int fd = open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd<0) handle_error("error while initializing file");
    close(fd);
    printf("closed...file correctly initialized!!!\n");
}

/*
 * Create a named semaphore with a given name, mode and initial value.
 * Also, tries to remove any pre-existing semaphore with the same name.
 */
sem_t *create_named_semaphore(const char *name, mode_t mode, unsigned int value) {
    printf("[Main] Creating named semaphore %s...", name);
    fflush(stdout);
    
    // TODO

    printf("done!!!\n");
    return NULL;
}

void parseOutput() {
    // identify the child that accessed the file most times
    int* access_stats = calloc(n, sizeof(int)); // initialized with zeros
    printf("[Main] Opening file %s in read-only mode...", FILENAME);
	fflush(stdout);
    int fd = open(FILENAME, O_RDONLY);
    if (fd < 0) handle_error("error while opening output file");
    printf("ok, reading it and updating access stats...");
	fflush(stdout);

    size_t read_bytes;
    int index;
    do {
        read_bytes = read(fd, &index, sizeof(int));
        if (read_bytes > 0)
            access_stats[index]++;
    } while(read_bytes > 0);
    printf("ok, closing it...");
	fflush(stdout);

    close(fd);
    printf("closed!!!\n");

    int max_child_id = -1, max_accesses = -1;
    for (int i = 0; i < n; i++) {
        printf("[Main] Child %d accessed file %s %d times\n", i, FILENAME, access_stats[i]);
        if (access_stats[i] > max_accesses) {
            max_accesses = access_stats[i];
            max_child_id = i;
        }
    }
    printf("[Main] ===> The process that accessed the file most often is %d (%d accesses)\n", max_child_id, max_accesses);
    free(access_stats);
}

void* thread_function(void* x) {
    thread_args_t *args = (thread_args_t*)x;

    /* TODO: protect the critical section using semaphore(s) as needed */
	
	int ret = sem_wait(file_sem);
	if (ret) {handle_error("Error in semwait in thread_function \n");exit(EXIT_FAILURE);}
    // open file, write child identity and close file
    int fd = open(FILENAME, O_WRONLY | O_APPEND);
    if (fd < 0) handle_error("error while opening file");
    printf("[Child#%d-Thread#%d] File %s opened in append mode!!!\n", args->child_id, args->thread_id, FILENAME);	

    write(fd, &(args->child_id), sizeof(int));
    printf("[Child#%d-Thread#%d] %d appended to file %s opened in append mode!!!\n", args->child_id, args->thread_id, args->child_id, FILENAME);	

    close(fd);
    printf("[Child#%d-Thread#%d] File %s closed!!!\n", args->child_id, args->thread_id, FILENAME);
	
	ret = sem_post(file_sem);
	if (ret) {handle_error("Error in sempost in thread_function \n");exit(EXIT_FAILURE);}
    
    free(x);
    pthread_exit(NULL);
}

void mainProcess() {
    /* TODO: the main process waits for all the children to start,
     * it notifies them to start their activities, and sleeps
     * for some time t. Once it wakes up, it notifies the children
     * to end their activities, and waits for their termination.
     * Finally, it calls the parseOutput() method and releases
     * any shared resources. */
    sem_t* ready_parent_named_sem = sem_open(READY_SEM_NAME, 0);
	if (ready_parent_named_sem == SEM_FAILED) { handle_error("Error in opening named-semaphore for READY notification in parent process \n"); exit(EXIT_FAILURE);}
    
    //Wait for all the children to start correctly
    int ret,check;
    while(check != N) { 
		ret = sem_getvalue(ready_parent_named_sem, &check);
		if (ret) {handle_error("Error in  sem_getvalue in parent process \n"); exit(EXIT_FAILURE);}
	}
	
	ret = sem_close(ready_parent_named_sem);
	if (ret) {handle_error("Error In closing semaphore in parent process function \n"); exit(EXIT_FAILURE);}
	
	
	//Notifica START
	sem_t* start_parent_named_sem = sem_open(START_SEM_NAME, 0);
	if (start_parent_named_sem == SEM_FAILED) { handle_error("Error in opening named-semaphore for START notification in parent process \n"); exit(EXIT_FAILURE);}
    
		for(int i = 0; i < N ; ++i ){
			ret = sem_post(start_parent_named_sem);
			if (ret) { handle_error("Error in sem_post in parent START process \n"); exit(EXIT_FAILURE);}
		}
    
	ret = sem_close(start_parent_named_sem);
	if (ret) {handle_error("Error In closing named semaphore for START notification in main function \n"); exit(EXIT_FAILURE);}
	
	
	//Sleep
	sleep(T);
	
	
	//Notifica HALT
	sem_t* halt_parent_named_sem = sem_open(HALT_SEM_NAME, 0);
	if (halt_parent_named_sem == SEM_FAILED) { handle_error("Error in opening named-semaphore for START notification in parent process \n"); exit(EXIT_FAILURE);}
    
		for(int i = 0; i < N ; ++i ){
			ret = sem_post(halt_parent_named_sem);
			if (ret) { handle_error("Error in sem_post in parent START process \n"); exit(EXIT_FAILURE);}
		}
    
	ret = sem_close(halt_parent_named_sem);
	if (ret) {handle_error("Error In closing named semaphore for START notification in main function \n"); exit(EXIT_FAILURE);}
	
     
}

void childProcess(int child_id) {
    /* TODO: each child process notifies the main process that it
     * is ready, then waits to be notified from the main in order
     * to start. As long as the main process does not notify a
     * termination event [hint: use sem_getvalue() here], the child
     * process repeatedly creates m threads that execute function
     * thread_function() and waits for their completion. When a
     * notification has arrived, the child process notifies the main
     * process that it is about to terminate, and releases any
     * shared resources before exiting. */
     
    //Named semaphore per la notifica ready.
	sem_t* read_child_named_sem = sem_open(READY_SEM_NAME, 0);
	if (read_child_named_sem == SEM_FAILED) { handle_error("Error in opening named-semaphore for ready notification in child process \n"); exit(EXIT_FAILURE);}
    
    //Here each child notifies the father that is ready
    int ret = sem_post(read_child_named_sem);
    if (ret) { handle_error("Error in sem_post in child process \n"); exit(EXIT_FAILURE);}
    
    ret = sem_close(read_child_named_sem);
	if (ret) {handle_error("Error In closing semaphore in child process function \n"); exit(EXIT_FAILURE);}
	 
    //Wait for the START notification from parent
    sem_t* start_child_named_sem = sem_open(START_SEM_NAME, 0);
    if (start_child_named_sem == SEM_FAILED) { handle_error("Error in opening named-semaphore for Start notification in child process \n"); exit(EXIT_FAILURE);}
    
    ret = sem_wait(start_child_named_sem);
    if (ret) { handle_error("Error in sem_wait for START notification in child process \n"); exit(EXIT_FAILURE);}
	
	//Until HALT notification
	sem_t* halt_child_named_sem = sem_open(HALT_SEM_NAME, 0);
    if (halt_child_named_sem == SEM_FAILED) { handle_error("Error in opening named-semaphore for HALT notification in child process \n"); exit(EXIT_FAILURE);}
	
	int check;
	while(check != N){
		ret = sem_getvalue(halt_child_named_sem, &check);
		if (ret) {handle_error("Error in  sem_getvalue in child process \n"); exit(EXIT_FAILURE);}
	
		//Start M threads
		pthread_t * threads = (pthread_t*) malloc(sizeof(pthread_t)*M);
			
		
		for(int i = 0; i < M ; ++i){
			thread_args_t * args = (thread_args_t*) malloc(sizeof(thread_args_t));
			args->child_id = child_id;
			args->thread_id = i;
			
			ret = pthread_create(&threads[i] , 0 , thread_function , args );
			if (ret) {handle_error("Error in child process in M thread creation \n"); exit(EXIT_FAILURE);}
		}
		
		//Attende terminazione M threads.
		for(int j = 0; j < M ; ++j){
			ret = pthread_join(threads[j], NULL);
			if (ret) {handle_error("Error in child process in M thread join \n"); exit(EXIT_FAILURE);}
		}
		
	}
		
    
    
    ret = sem_close(start_child_named_sem);
	if (ret) {handle_error("Error In closing named semaphore for START notification in child process function \n"); exit(EXIT_FAILURE);}
	 
     
}

int main(int argc, char **argv) {
    // arguments
    if (argc > 1) n = atoi(argv[1]);
    if (argc > 2) m = atoi(argv[2]);
    if (argc > 3) t = atoi(argv[3]);

    // initialize the file
    init_file(FILENAME);

    /* TODO: initialize any semaphore needed in the implementation, and
     * create N children where the i-th child calls childProcess(i); then
     * the main process executes function mainProcess() once all the
     * children have been created */
     
    //In order to clear previous executions. No need to handle returning value.
    sem_unlink(READY_SEM_NAME);
    sem_unlink(START_SEM_NAME);
    sem_unlink(HALT_SEM_NAME);
    
    
    ready_sem = sem_open(READY_SEM_NAME, O_CREAT | O_EXCL , 0600 , 0);
    if (ready_sem == SEM_FAILED) { handle_error("Error in creating named-semaphore for READY notification \n"); exit(EXIT_FAILURE);}
    start_sem = sem_open(START_SEM_NAME, O_CREAT | O_EXCL , 0600 , 0);
    if (start_sem == SEM_FAILED) { handle_error("Error in creating named-semaphore for START notification \n"); exit(EXIT_FAILURE);}
    halt_sem = sem_open(HALT_SEM_NAME, O_CREAT | O_EXCL , 0600 , 0);
    if (halt_sem == SEM_FAILED) { handle_error("Error in creating named-semaphore for START notification \n"); exit(EXIT_FAILURE);}
    
    
    //Thread Semaphore init
    file_sem = (sem_t*) malloc(sizeof(sem_t));
    int ret2 = sem_init(file_sem , 0 , 1);
	if (ret2) { handle_error("Error in sem_init per thread semaphore \n"); exit(EXIT_FAILURE);}
     
    int i, j; 
    pid_t pid;
	
    for(i = 0 ; i< N ; ++i ) {
		j = i;
		pid = fork();
		if(pid == 0 ){ 
			childProcess(j);
			exit(EXIT_SUCCESS);
		}else if(pid == -1){
			handle_error_en(pid, "Error in child process cretaion in \n");
			exit(EXIT_FAILURE);
		}
	}
    
    mainProcess();
    
	 
	int ret = sem_close(ready_sem);
	if (ret) {handle_error("Error In closing named semaphore for READY notification in main function \n"); exit(EXIT_FAILURE);}
	ret = sem_close(start_sem);
	if (ret) {handle_error("Error In closing named semaphore for START notification in main function \n"); exit(EXIT_FAILURE);}
	ret = sem_close(halt_sem);
	if (ret) {handle_error("Error In closing named semaphore for HALT notification in main function \n"); exit(EXIT_FAILURE);}
	
	ret = sem_unlink(READY_SEM_NAME);
	if (ret) {handle_error("Error In unlinking named semaphore for READY notification in main function \n"); exit(EXIT_FAILURE);}
	ret = sem_unlink(START_SEM_NAME);
	if (ret) {handle_error("Error In unlinking named semaphore for START notification in main function \n"); exit(EXIT_FAILURE);}
	ret = sem_unlink(HALT_SEM_NAME);
	if (ret) {handle_error("Error In unlinking named semaphore for HALT notification in main function \n"); exit(EXIT_FAILURE);}
	
	ret2 = sem_destroy(file_sem);
	if (ret2) { handle_error("Error in sem_init per thread semaphore \n"); exit(EXIT_FAILURE);}
    
	
	parseOutput();
	
	
    exit(EXIT_SUCCESS);
}
