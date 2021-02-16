#include "performance.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <pthread.h>

int STEP = 2048; // size of step = (2^10)*4 Bytes = 2^12 Bytes = 4 KB  
int NUM_ITEMS = (1 << 24); // size of buffer  = (2^24)*4 Bytes = (16777216)*4 Bytes = 2^26 Bytes = 2^16  Kilo Bytes = 2^6 MegaBytes = 64 MB of space.   

int* BUF = NULL;



void do_work() {
	for (int i = 0 ; i < NUM_ITEMS ; i += STEP ){
		BUF[i] = 1;
	}
	return;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "Syntax: %s <N>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	// parse N from the command line
	int n = atoi(argv[1]);
		
	int* BUF = (int* ) calloc(NUM_ITEMS, sizeof(int));

	if (BUF == NULL)
	{
		perror("error on calloc \n");
		exit(EXIT_FAILURE);
	}

	// process reactivity
	printf("Process reactivity, %d tests...\n", n);
	unsigned long sum = 0;
	timer t;
	int i;

	begin(&t);
	for (i = 0; i < n; i++) {
		pid_t pid = fork();
		if (pid == -1) {
			fprintf(stderr, "Can't fork, error %d\n", errno);
			exit(EXIT_FAILURE);
		} else if (pid == 0) {
			do_work();
			_exit(EXIT_SUCCESS);
		} else {
			wait(0);
		}
	}
	end(&t);
	sum += get_microseconds(&t);

	// compute statistics
	unsigned long process_avg = sum / n;
	printf("Average: %lu microseconds\n", process_avg);
	
	return EXIT_SUCCESS;
}
