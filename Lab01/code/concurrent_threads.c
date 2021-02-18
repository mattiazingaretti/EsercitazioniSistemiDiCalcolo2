#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#define N 1000 // number of threads
#define M 10000 // number of iterations per thread
#define V 1 // value added to the balance by each thread at each iteration

unsigned long int shared_variable;
int n = N, m = M, v = V;

//Global structure
int* BUF;

void* thread_work(void *arg) {
	
	int* index = (int*) arg;
	//DEBUG printf("Thread %d doing stuff \n", *index);
	int i, local = 0; 
	for (i = 0; i < m; i++)
		local += v;
	BUF[*index] = local; 
	return NULL;
}

int main(int argc, char **argv)
{
	if (argc > 1) n = atoi(argv[1]);
	if (argc > 2) m = atoi(argv[2]);
	if (argc > 3) v = atoi(argv[3]);
	shared_variable = 0;

	//Init Global BUF
	BUF = (int*) calloc(N, sizeof(int));

	printf("Going to start %d threads, each adding %d times %d to a shared variable initialized to zero...", n, m, v); fflush(stdout);
	pthread_t* threads = (pthread_t*)malloc(n * sizeof(pthread_t)); // also calloc(n,sizeof(pthread_t))
	int i;
	for (i = 0; i < n; i++)
		
		if (pthread_create(&threads[i], NULL, thread_work, &i) != 0) {
			fprintf(stderr, "Can't create a new thread, error %d\n", errno);
			exit(EXIT_FAILURE);
		}
	printf("ok\n");

	printf("Waiting for the termination of all the %d threads...", n); fflush(stdout);
	for (i = 0; i < n; i++)
		pthread_join(threads[i], NULL);
	printf("ok\n");


	for(int j = 0 ; j < n ; j++){
		printf("Buff elem %d = %d \n",j, BUF[j]);
		shared_variable +=  BUF[j];
	}

	unsigned long int expected_value = (unsigned long int)n*m*v;
	printf("The value of the shared variable is %lu. It should have been %lu\n", shared_variable, expected_value);
	if (expected_value > shared_variable) {
		unsigned long int lost_adds = (expected_value - shared_variable) / v;
		printf("Number of lost adds: %lu\n", lost_adds);
	}

    free(threads);

	return EXIT_SUCCESS;
}

