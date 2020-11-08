#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h> 

#define N 10//1000 // number of threads
#define M 10 //10000 // number of iterations per thread
#define V 1 // value added to the balance by each thread at each iteration

unsigned long int shared_variable;
int n = N, m = M, v = V;

//Definisco un semaforo globale per potervi accedere da thread_work
sem_t * my_sem;

void* thread_work(void *arg) {
	int i;
	for (i = 0; i < m; i++)
		
		if (sem_wait(my_sem) != 0){
			printf("Errore sulla sem_wait !");
			return NULL;
		}
		//Inizio area critica
		shared_variable += v;
		//Fine area critica
		if (sem_post(my_sem) != 0){
			printf("Errore sulla sempost!");
			return NULL;	
		}
	return NULL;
}

int main(int argc, char **argv)
{
	if (argc > 1) n = atoi(argv[1]);
	if (argc > 2) m = atoi(argv[2]);
	if (argc > 3) v = atoi(argv[3]);
	shared_variable = 0;
	timer myTimer;
	
	printf("Going to start %d threads, each adding %d times %d to a shared variable initialized to zero...", n, m, v); fflush(stdout);
	pthread_t* threads = (pthread_t*)malloc(n * sizeof(pthread_t));
	
	//Alloco e inizializzo il semaforo globale
	my_sem = (sem_t*) malloc(sizeof(sem_t));
	sem_init(my_sem, 0 , 1);
	
	begin(&t);
	int i;
	for (i = 0; i < n; i++)
		if (pthread_create(&threads[i], NULL, thread_work, NULL) != 0) {
			fprintf(stderr, "Can't create a new thread, error %d\n", errno);
			exit(EXIT_FAILURE);
		}
	printf("ok\n");

	printf("Waiting for the termination of all the %d threads...", n); fflush(stdout);
	for (i = 0; i < n; i++)
		pthread_join(threads[i], NULL);
	end(&t);
	printf("ok\n");
	
	
	unsigned long int expected_value = (unsigned long int)n*m*v;
	printf("The value of the shared variable is %lu. It should have been %lu\n", shared_variable, expected_value);
	if (expected_value > shared_variable) {
		unsigned long int lost_adds = (expected_value - shared_variable) / v;
		printf("Number of lost adds: %lu\n", lost_adds);
	}

    free(threads);
	//Dopo che tutti i threads hanno terminato posso deallocare il mio semaforo globale.
	sem_destroy(my_sem);
	free(my_sem);
	
	return EXIT_SUCCESS;
}
