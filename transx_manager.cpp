#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <sys/sem.h>
#include <fstream>
#include "transx_def.hpp"
#include "transx_manager.hpp"
#include "transx_extern.hpp"

#define TNO 1    

//Sets the value of member function logfile and opens the file for writing
//in the append mode
void transx_manager::openlog(string lfile) {
    FILE *outfile;
#ifdef TM_DEBUG
    printf("entering open log \n");
    fflush(stdout);
    fflush(stdout);
#endif
    logfile = (char *) malloc(sizeof (char) * MAX_FILENAME);

    int i = 0;
    while (lfile[i] != '\0') {
        logfile[i] = lfile[i];
        i++;
    }//CHECK LOGFILE RECORD, MOVE TO THE NEW LINE
    logfile[i] = '\0';
    if ((outfile = fopen(logfile, "a")) == NULL) {
        printf("\nCannot open log file for append:%s\n", logfile);
        fflush(stdout);
        exit(1);
    }
    fprintf(outfile, "---------------------------------------------------------------\n");
    fprintf(outfile, "TxId\tTxtype\tOperation\tObId:Obvalue:optime\tLockType\tStatus\t\tTxStatus\n");
    fflush(outfile);
#ifdef TM_DEBUG
    printf("leaving open log \n");
    fflush(stdout);
    fflush(stdout);
#endif
}





//create a thread and call the constructor of transaction to
//create the object and intialize the other members of transx in
//begintx(void *thdarg). Pass the thread arguments in a structure

int transx_manager::BeginTx(long tid, int thrNum, char type) {

#ifdef TM_DEBUG
    cout<<"\n entering BeginTx\n";
#endif
     

    //YOUR CODE HERE
    t_info *td = new t_info();
    int rc;

	//fill the parameter in te structure
    SEQNUM[tid]=0;
    td->tid = tid;
    td->type = type;
    td->thrid = threadid[thrNum];
    td->count = 0;

	//create a new thread and call the begintx
    rc = pthread_create(&threadid[thrNum], NULL, begintx, (void *)td);

    if (rc) {
       cout << "Error:unable to create thread," << rc << endl;
       exit(-1);
    }



#ifdef TM_DEBUG
    cout<<"\nleaving BeginTx\n";
#endif
    return (0);

}




// Call the read function in transaction class. Read operation is just printing
// the value of the item; But to perform the operation, one needs to make sure
// that the strict 2-phase locking is followed.
// now create the thread and call the method readtx(void *)

int transx_manager::TxRead(long tid, long obno, int thrNum) {

#ifdef TM_DEBUG
    cout<<"\nentering TxRead\n";
#endif
   
    //YOUR CODE HERE
    t_info *td = new t_info();
    int rc;

	// fill the parameters in the structure
    SEQNUM[tid]=SEQNUM[tid] - 1;
    td->tid = tid;
    td->obno = obno;
    td->count = SEQNUM[tid];

	//create a new thread and call readtx
    rc = pthread_create(&threadid[thrNum], NULL, readtx, (void *)td);

    if (rc) {
       cout << "Error:unable to create thread," << rc << endl;
       exit(-1);
    }
    	

#ifdef TM_DEBUG
    cout<<"\nleaving TxRead\n";
#endif
    return (0); //successful operation
}




// write operation is to increement the value by 1. Again the protocol
// need to be adheared to

int transx_manager::TxWrite(long tid, long obno, int thrNum)
{

#ifdef TM_DEBUG
    cout<<"\n entering TxWrite\n";
#endif

    t_info *td = new t_info();
    int rc;
	
	// fill the parameters in the structure
    SEQNUM[tid]=SEQNUM[tid] - 1;
    td->tid = tid;
    td->obno = obno;
    td->thrid = threadid[thrNum];
    td->count = SEQNUM[tid];

	// create a new thread and call wrtitetx
    rc = pthread_create(&threadid[thrNum], NULL, writetx, (void *)td);

    if (rc) {
       cout << "Error:unable to create thread," << rc << endl;
       exit(-1);
    }

#ifdef TM_DEBUG
    cout<<"\n leaving TxWrite\n";
#endif
    return (0); //successful operation
}



int transx_manager::CommitTx(long tid, int thrNum)
{

#ifdef TM_DEBUG
    cout<<"\n entering TxCommit\n";
#endif


        //YOUR CODE HERE
    t_info *td = new t_info();
    int rc;

	// fill the structure
    SEQNUM[tid]=SEQNUM[tid] - 1;
    td->tid = tid;
    td->obno = -1;
    td->thrid = threadid[thrNum];
    td->count = SEQNUM[tid];

	//create a new thread and call committx
    rc = pthread_create(&threadid[thrNum], NULL, committx, (void *)td);

    if (rc) {
       cout << "Error:unable to create thread," << rc << endl;
       exit(-1);
    }

#ifdef TM_DEBUG
    cout<<"\n leaving TxCommit\n";
#endif
    return (0); //successful operation
}



int transx_manager::AbortTx(long tid, int thrNum) {

//YOUR CODE HERE
#ifdef TM_DEBUG
    cout<<"\n entering AbortTx\n";
#endif
    t_info *td = new t_info();
    int rc;

	//fill the structure
    SEQNUM[tid]=SEQNUM[tid] - 1;
    td->tid = tid;
    td->obno = -1;
    td->thrid = threadid[thrNum];
    td->count = SEQNUM[tid];

	// create a new thread and call aborttx
    rc = pthread_create(&threadid[thrNum], NULL, aborttx, (void *)td);

    if (rc) {
       cout << "Error:unable to create thread," << rc << endl;
       exit(-1);
    }


#ifdef TM_DEBUG
    cout<<"\nleaving AbortTx\n";
    fflush(stdout);
#endif
    return (0); //successful operation
}





int transx_manager::endTm(int thrNum) {
    int rc = 0;
    int i;
#ifdef TM_DEBUG

    printf("\nentering End of schedule with thrNum: %d\n", thrNum);
    fflush(stdout);
#endif
    printf("Wait for threads and cleanup\n");
    for (i = 0; i < thrNum; i++) {
        rc = pthread_join(threadid[i], NULL);
        printf("Thread %d completed with ret value: %d\n", i, rc);
        fflush(stdout);
    }
    printf("ALL threads finished their work\n");
    printf("Releasing mutexes and condpool\n");
    fflush(stdout);
    //release condpool and mutexpool
    for (i = 1; i < MAX_TRANSACTIONS + 1; ++i) {
        pthread_mutex_destroy(&mutexpool[i]);
        pthread_cond_destroy(&condpool[i]);
    }
    printf("Releasing all semaphores\n");
    fflush(stdout);
    transx_sem_release(t_semid);
    printf("endTm completed\n");
    fflush(stdout);
#ifdef TM_DEBUG
    printf("\nleaving endTm\n");
    fflush(stdout);
#endif
    return (0); //successful operation

}



transx_manager::transx_manager() {
    int i, init;

    lastr = NULL;
    //initialize objarray; each element points to a different item
    for (i = 0; i < MAX_ITEMS; ++i)
        objarray[i] = new item(0); //all init'd to zero

    //initialize optime for the thread to sleep;
    //get a int from random function to sleep 

    int seed = 10000;
    srand(seed); /*initialize random number generator*/
    int M = 1000; /* Multiplier */
    for (i = 1; i < MAX_TRANSACTIONS + 1; ++i) {
        double r = ((double) rand() / (double) (RAND_MAX + 1.0)); //RAND_MAX is defined in stdlib
        double x = (r * M);
        int y = (int) x;
        optime[i] = abs(y);
    }


    //initialize condpool and mutexpool
    for (i = 1; i < MAX_TRANSACTIONS + 1; ++i) {
        pthread_mutex_init(&mutexpool[i], NULL);
        pthread_cond_init(&condpool[i], NULL);
        condset[i] = 0;
        SEQNUM[i] = 0;
    }
    t_semnum = MAX_TRANSACTIONS + 1; //setting the no of semaphore 

    t_keysem = TNO; //setting the key_t data to const 

    //semget() gets a array of semaphore for a particular key.Here
    //	creating a semaphore with  key 1


    if ((sem = transx_init_sem(IPC_CREAT)) < 0) {
        cout << "Error creating semaphores \n";
        exit(1);
    }

        t_semid = sem;

    //intialising the semaphore value with 0 to 1 and the rest to 0
    transx_init_sem_0(t_semid);
    transx_init_sem_rest(t_semid);
};













