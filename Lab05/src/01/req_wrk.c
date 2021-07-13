#include "common.h"

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <pthread.h>


// data array
int * data;
/** COMPLETE THE FOLLOWING CODE BLOCK
*
* Add any needed resource 
**/
int shm_fd;

//Synch res
sem_t * sem_req;
sem_t * sem_wrk;


int request() {
  /** COMPLETE THE FOLLOWING CODE BLOCK
  *
  * map the shared memory in the data array
  **/
	
	int ret;
  //INIT shm
  shm_fd = shm_open(SHM_NAME ,O_RDWR , 0666 );
  if(shm_fd < 0 ) {handle_error("Error in open shm in request process \n ");}
  
  if((data = (int*) mmap(0, SIZE , PROT_READ | PROT_WRITE , MAP_SHARED , shm_fd , 0)) == MAP_FAILED ){handle_error("Error in mmap shm in request process \n ");} ;
  
  //Init sem_req for sync
  sem_req = sem_open(SEM_NAME_REQ , 0);
  if(sem_req == SEM_FAILED ){handle_error("Error in open sem_req in request process \n ");}

  sem_wrk = sem_open(SEM_NAME_WRK , 0);
  if(sem_wrk == SEM_FAILED ){handle_error("Error in open sem_wrk in request process \n ");}


  printf("request: mapped address: %p\n", data);

  int i;
  for (i = 0; i < NUM; ++i) {
    data[i] = i;
  }
  printf("request: data generated\n");

   /** COMPLETE THE FOLLOWING CODE BLOCK
    *
    * Signal the worker that it can start the elaboration
    * and wait it has terminated
    **/
    
  ret = sem_post(sem_req);
  if(ret) {handle_error("Error in sem_post sem_req in request process \n ");}
  
   
  printf("request: acquire updated data\n");
  
  ret = sem_wait(sem_wrk);
  if(ret) {handle_error("Error in sem_wait sem_wrk in request process \n ");}

  printf("request: updated data:\n");
  for (i = 0; i < NUM; ++i) {
    printf("%d\n", data[i]);
  }

   /** COMPLETE THE FOLLOWING CODE BLOCK
    *
    * Release resources
    **/
    
  ret = sem_close(sem_wrk);
  if(ret) {handle_error("Error in sem_close sem_wrk in request process \n ");}

  ret = sem_close(sem_req);
  if(ret) {handle_error("Error in sem_close sem_req in request process \n ");}
  
  ret = close(shm_fd);
  if(ret) {handle_error("Error in close shm in request process \n ");}


  return EXIT_SUCCESS;
}

int work() {

  /** COMPLETE THE FOLLOWING CODE BLOCK
  *
  * map the shared memory in the data array
  **/
  int ret;
  
  
  printf("worker: mapped address: %p\n", data);
  
  //Init sem sync
  sem_req = sem_open(SEM_NAME_REQ , 0);
  if(sem_req == SEM_FAILED ){handle_error("Error in open sem_req in work process \n ");}

  sem_wrk = sem_open(SEM_NAME_WRK , 0);
  if(sem_wrk == SEM_FAILED ){handle_error("Error in open sem_wrk in work process \n ");}



   /** COMPLETE THE FOLLOWING CODE BLOCK
    *
    * Wait that the request() process generated data
    **/
  
  
  printf("worker: waiting initial data\n");
  
  ret = sem_wait(sem_req);
  if (ret){handle_error("Error in sem_wait per sem_req in work \n ");}
  
  //INIT shm
  shm_fd = shm_open(SHM_NAME , O_RDWR , 0600);
  if(shm_fd < 0 ) {handle_error("Error in work in shm_open \n");}
  
  if((data = (int*) mmap(0, SIZE , PROT_WRITE , MAP_SHARED , shm_fd , 0)) == MAP_FAILED) {handle_error("Error in work in mmap \n");};
  
  printf("worker: initial data acquired\n");

  printf("worker: update data\n");
  
  //INITIALIZATION NOT NEEDED 
  int i;
  for (i = 0; i < NUM; ++i) {
    data[i] = data[i] * data[i];
  }

  printf("worker: release updated data\n");
  
  ret = sem_post(sem_wrk);
  if (ret){handle_error("Error in sem_wait per sem_wrk in work \n ");}
  

   /** COMPLETE THE FOLLOWING CODE BLOCK
    *
    * Signal the requester that elaboration terminated
    **/


  /** COMPLETE THE FOLLOWING CODE BLOCK
   *
   * Release resources
   **/
   
  ret = close(shm_fd);
  if(ret) {handle_error("Error in work in close \n");} 
      
  ret = sem_close(sem_wrk);
  if(ret) {handle_error("Error in sem_close sem_wrk in work process \n ");}

  ret = sem_close(sem_req);
  if(ret) {handle_error("Error in sem_close sem_req in work process \n ");}
  
  
  return EXIT_SUCCESS;
}



int main(int argc, char **argv){

   /** COMPLETE THE FOLLOWING CODE BLOCK
    *
    * Create and open the needed resources 
    **/
	int ret;
    
	
	//First unlink the shm from previous executions.
	shm_unlink(SHM_NAME);
	
	shm_fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR , 0666);
	if(shm_fd <0){handle_error_en(shm_fd, "Error in open shm in main process \n ");}
	
	ret = ftruncate(shm_fd , SIZE);
	if (ret) {handle_error("Error in ftruncate in main process \n ");}
	
	if((data = (int*) mmap(0, SIZE , PROT_WRITE | PROT_READ , MAP_SHARED , shm_fd , 0)) == MAP_FAILED ){handle_error("Error in mmap in main process \n ");} ;
	
	
	//Semaphores initializations
	sem_unlink(SEM_NAME_REQ);
	sem_unlink(SEM_NAME_WRK); //Do not handle ret value here !
	
	sem_req = sem_open(SEM_NAME_REQ , O_CREAT | O_EXCL , 0600, 0);
	if(sem_req == SEM_FAILED){handle_error("Error in sem open sem_req \n");}
	
	sem_wrk = sem_open(SEM_NAME_WRK , O_CREAT | O_EXCL , 0600, 0);
	if(sem_wrk == SEM_FAILED){handle_error("Error in sem open sem_wrk \n");}
	
	
	
	

    pid_t pid = fork();
    if (pid == -1) {
        handle_error("main: fork");
    } else if (pid == 0) {
        work();
        _exit(EXIT_SUCCESS);
    }

    request();
    int status;
    ret = wait(&status);
    if (ret == -1)
      handle_error("main: wait");
    if (WEXITSTATUS(status)) handle_error_en(WEXITSTATUS(status), "request() crashed");


   /** COMPLETE THE FOLLOWING CODE BLOCK
    *
    * Close and release resources
    **/
    
    
    
	ret = sem_unlink(SEM_NAME_WRK);
	if(ret){handle_error("Error in sem unlink sem_wrk in main process \n ");}
	
	ret = sem_unlink(SEM_NAME_REQ);
	if(ret){handle_error("Error in sem unlink sem_req in main process \n ");}
	
	ret = shm_unlink(SHM_NAME);
	if(ret){handle_error("Error in shm unlink  in main process \n ");}
	if(ret){handle_error("Error in shm unlink  in main process \n ");}
    
    
    _exit(EXIT_SUCCESS);

}
