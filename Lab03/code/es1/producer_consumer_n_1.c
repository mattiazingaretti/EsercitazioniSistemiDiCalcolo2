#include <string.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>       // nanosleep()
#include "common.h"

#define BUFFER_SIZE         128
#define INITIAL_DEPOSIT     0
#define MAX_TRANSACTION     1000
#define NUM_CONSUMERS       1
#define NUM_PRODUCERS       100
#define PRNG_SEED           0

#define NUM_OPERATIONS      400
#define OPS_PER_CONSUMER    (NUM_OPERATIONS/NUM_CONSUMERS)
#define OPS_PER_PRODUCER    (NUM_OPERATIONS/NUM_PRODUCERS)

// we use the preprocessor to check if our parameters are okay
#if OPS_PER_CONSUMER*NUM_CONSUMERS != OPS_PER_PRODUCER*NUM_PRODUCERS
#error "Choose NUM_CONSUMERS and NUM_PRODUCERS so that we get exactly NUM_OPERATIONS operations"
#endif

//Semaphore handling
sem_t* s;
sem_t* n;
sem_t* e;

// struct used to specify arguments for a thread
typedef struct {
    int threadId;
    int numOps;
} thread_args_t;

// shared data
int transactions[BUFFER_SIZE];
int deposit = INITIAL_DEPOSIT;
int read_index, write_index;

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

// producer thread start routine
void* performTransactions(void* x) {
    thread_args_t* args = (thread_args_t*)x;
    printf("Starting producer thread %d\n", args->threadId);

    while (args->numOps > 0) {
        // produce the item
        int currentTransaction = performRandomTransaction();
		
		int res = sem_wait(e);
		if(res) handle_error_en(res, "Error in empty sem_wait prod \n");
        
		
		res = sem_wait(s);
		if(res) handle_error_en(res, "Error in sem_wait prod \n");
        // write the item and update write_index accordingly
        transactions[write_index] = currentTransaction;
        write_index = (write_index + 1) % BUFFER_SIZE;
		
		res = sem_post(s);
		if(res) handle_error_en(res, "Error in sem_post prod \n");
        
        res = sem_post(n);
        if(res) handle_error_en(res, "Error in sem_post for sem n prod \n");
        
        
        args->numOps--;
        //printf("P %d\n", args->numOps);
    }

    free(args);
    pthread_exit(NULL);
}
//consumer thread start routine
void* processTransactions(void* x) {
    thread_args_t* args = (thread_args_t*)x;
    printf("Starting consumer thread %d\n", args->threadId);

    while (args->numOps > 0) {
        // consume the item and update (shared) variable deposit
        int res = sem_wait(n);
		if(res) handle_error_en(res, "Error in sem_wait for sem n cons \n");
        
        res = sem_wait(s);
		if(res) handle_error_en(res, "Error in sem_wait cons \n");
        
        deposit += transactions[read_index];
        read_index = (read_index + 1) % BUFFER_SIZE;
        if (read_index % 100 == 0)
			printf("After the last 100 transactions balance is now %d.\n", deposit);
        
        res = sem_post(s);
		if(res) handle_error_en(res, "Error in sem_post cons \n");
        
        res = sem_post(e);
		if(res) handle_error_en(res, "Error in empty sem_post consumer \n");
        
        
        args->numOps--;
        //printf("C %d\n", args->numOps);
    }

    free(args);
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    printf("Welcome! This program simulates financial transactions on a deposit.\n");
    printf("\nThe maximum amount of a single transaction is %d (negative or positive).\n", MAX_TRANSACTION);
    printf("\nInitial balance is %d. Press CTRL+C to quit.\n\n", INITIAL_DEPOSIT);

    // initialize read and write indexes
    read_index  = 0;
    write_index = 0;

    // set seed for pseudo-random number generator: we use this to make
    // this code yield the same result across different runs, as long
    // as they are race-free and you make no mistakes :-)
    srand(PRNG_SEED); 

    int ret;
    pthread_t producer[NUM_PRODUCERS], consumer[NUM_CONSUMERS];

	s = (sem_t*) malloc(sizeof(sem_t));
	n = (sem_t*) malloc(sizeof(sem_t));
	e = (sem_t*) malloc(sizeof(sem_t));
	
	int r = sem_init(s , 0, 1);
	if (r) handle_error_en(r, "Error in sem_init ! \n ");
	
	r = sem_init(n , 0, 0);
	if (r) handle_error_en(r, "Error in sem_init ! \n ");
	
	r = sem_init(e , 0, BUFFER_SIZE);
	if (r) handle_error_en(r, "Error in sem_init ! \n ");
	
	
    int i;
    for (i=0; i<NUM_PRODUCERS; ++i) {
        thread_args_t* arg = malloc(sizeof(thread_args_t));
        arg->threadId = i;
        arg->numOps = OPS_PER_PRODUCER;

        ret = pthread_create(&producer[i], NULL, performTransactions, arg);
        if (ret != 0) { fprintf(stderr, "Error %d in pthread_create\n", ret); exit(EXIT_FAILURE); }
    }

    int j;
    for (j=0; j<NUM_CONSUMERS; ++j) {
        thread_args_t* arg = malloc(sizeof(thread_args_t));
        arg->threadId = j;
        arg->numOps = OPS_PER_CONSUMER;

        ret = pthread_create(&consumer[j], NULL, processTransactions, arg);
        if (ret != 0) { fprintf(stderr, "Error %d in pthread_create\n", ret); exit(EXIT_FAILURE); }
    }

    // join on threads
    for (i=0; i<NUM_PRODUCERS; ++i) {
        ret = pthread_join(producer[i], NULL);
        if (ret != 0) { fprintf(stderr, "Error %d in pthread_join\n", ret); exit(EXIT_FAILURE); }
    }

    for (j=0; j<NUM_CONSUMERS; ++j) {
        ret = pthread_join(consumer[j], NULL);
        if (ret != 0) { fprintf(stderr, "Error %d in pthread_join\n", ret); exit(EXIT_FAILURE); }
    }

	
    printf("Final value for deposit: %d\n", deposit);

	int res = sem_destroy(s);
	if(res) handle_error_en(res, "Error in sem_destoy s in main \n");
	
    res = sem_destroy(n);
	if(res) handle_error_en(res, "Error in sem_destoy  n in main \n");
	
	res = sem_destroy(e);
	if(res) handle_error_en(res, "Error in sem_destoy  n in main \n");
	
	free(e);
	free(n);
	free(s);
	
    exit(EXIT_SUCCESS);
}
