#include <stdio.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "transx_def.hpp"
#include "transx_manager.hpp"
#include "transx_extern.hpp"

extern FILE *logfile;


namespace local{
 union semun {
             int val;
             struct semid_ds *buf;
             ushort *array;
             } transx_arg;
}

struct sembuf  transx_sopbuf, *transx_sop= &transx_sopbuf;



int transx_init_sem(int create)
{
	int semid;  
	if (create != IPC_CREAT) 
		create = 0;
    
  	if ((semid= semget(t_keysem,t_semnum, create| 0666))< 0) {
    /* error handling */
    	printf("could not acquire semaphores: %s\n", semid);
    	exit(1);
  	}

  	return(semid);

}

// 0 (zero) semaphore is used for the Tx manager data structure; hence it is 
// initialized to 1 to permit the first operation to proceed

void transx_init_sem_0(int semid)
{
      local::transx_arg.val = 1;
      semctl(semid, 0, SETVAL, local::transx_arg);

}

// The rest of the semaphores are used by the transactions to wait on other Txs
// if a lock is NOT obtained. Hence they are initialized to 0. t_semnum is 
// initialized to MAX_TRANSATIONS+1. A transaction can be waiting only for 
// one other tx. Mutiple Txs can be waiting for a Tx

void transx_init_sem_rest(int semid) {
  	int k;
  	for (k = 1; k < t_semnum ; k++)
    {
      	local::transx_arg.val = 0;
      	semctl(semid, k, SETVAL, local::transx_arg);
    }  

}

// executes the p operation on the semaphore indicated.

int transx_P(int sem)
{
  	int errno;
  	transx_sop->sem_num= sem; //sembuf 
  	transx_sop->sem_op= -1;
  	transx_sop->sem_flg = 0;
  	if (semop(t_semid, transx_sop,1)<0){
    	printf("could not do a P semaphore operation on sem:%d\n", sem);
    	fflush(stdout);
    	fprintf(logfile, "could not do a P semaphore operation on sem:%d\n", sem);
    	fflush(logfile);
    	exit(1);
  	}     

  	return(0);
}

// executes the v operation on the semaphore indicated

int transx_V(int sem)
{
  	transx_sop->sem_num= sem;
  	transx_sop->sem_op= 1;
  	transx_sop->sem_flg = 0;
  	
	if (semop(t_semid,transx_sop,1)< 0){
    	printf("could not do a V semaphore operation on sem:%d\n", sem);
    	fflush(stdout);
    	fprintf(logfile, "could not do a V semaphore operation on sem:%d\n", sem);
    	fflush(logfile);
    	exit(1);
  	}

  	return(0);
}

// Returns the # of Txs waiting on a given semaphore; This is needed to release
// all Txs that are waiting on a tx when it commits or aborts.

int transx_wait(int sem)
{
  	return( semctl(t_semid,sem,GETNCNT,local::transx_arg));
}

int transx_sem_release(int semid){
    int k;
    for (k = 0; k < t_semnum ; k++){
      	local::transx_arg.val = 0;
      	semctl(semid, k, IPC_RMID, local::transx_arg);
    }  
  	return(0);
}


