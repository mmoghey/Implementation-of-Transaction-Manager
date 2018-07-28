#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>
#include "transx_def.hpp"
#include "transx_manager.hpp"
#include "transx_extern.hpp"
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <pthread.h>
#include <ctime>
#include <sys/types.h>
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)

extern void *start_operation(long, long); //starts operation by doing conditional wait
extern void *finish_operation(long); // finishes abn operation by removing conditional wait
extern void *open_logfile_for_append(); //opens log file for writing
extern void *do_commit_abort(long, char); //commit/abort based on char value (the code is same for us)

extern transx_manager *t_manager; // Transaction manager object

FILE *logfile; //declare globally to be used by all

/* Transaction class constructor */
/* Initializes transaction id and status and thread id */
/* Input: Transaction id, status, thread id */

transx::transx(long tid, char Txstatus, char type, pthread_t thrid) {
	this->lockmode = (char) ' '; //default
	this->Txtype = type; //R = read only, W=Read/Write
	this->sgno = 1;
	this->tid = tid;
	this->obno = -1; //set it to a invalid value
	this->status = Txstatus;
	this->pid = thrid;
	this->head = NULL;
	this->nextr = NULL;
	this->semno = -1; //init to  an invalid sem value
}

/* Method used to obtain reference to a transaction node      */
/* Inputs the transaction id. Makes a linear scan over the    */
/* linked list of transaction nodes and returns the reference */
/* of the required node if found. Otherwise returns NULL      */

transx* get_tx(long tid1) {
	transx *txptr, *lastr1;
	if (t_manager->lastr != NULL) { 									// If the list is not empty
		lastr1 = t_manager->lastr; 									// Initialize lastr1 to first node's ptr
		for (txptr = lastr1; (txptr != NULL); txptr = txptr->nextr)
			if (txptr->tid == tid1) 								// if required id is found									
				return txptr;
		return (NULL); 												// if not found in list return NULL
	}
	return (NULL); 													// if list is empty return NULL
}


// routine to start an operation by checking the previous operation of the same
// tx has completed; otherwise, it will do a conditional wait until the
// current thread signals a broadcast

void *start_operation(long tid, long count) {

	cout << "thread: "<< gettid() << "calling start_operation " << tid<< " "<< count<< "\n";
	pthread_mutex_lock(&t_manager->mutexpool[tid]); // Lock mutex[t] to make other
	// threads of same transaction to wait

	while (t_manager->condset[tid] != count) // wait if condset[t] is != count
		pthread_cond_wait(&t_manager->condpool[tid], &t_manager->mutexpool[tid]);

}

// Otherside of the start operation;
// signals the conditional broadcast

void *finish_operation(long tid) {
	cout << "thread: "<< gettid() << "calling finish_operation " << tid<< " "<<t_manager->condset[tid]<< "\n";
	t_manager->condset[tid]--; // decr condset[tid] for allowing the next op
	pthread_cond_broadcast(&t_manager->condpool[tid]); // other waiting threads of same tx
	pthread_mutex_unlock(&t_manager->mutexpool[tid]);
}



/* Method that handles "BeginTx tid" in test file     */
/* Inputs a pointer to transaction id, obj pair as a struct. Creates a new  */
/* transaction node, initializes its data members and */
/* adds it to transaction list */

void *begintx(void *arg) 
{
#ifdef TM_DEBUG
	cout << "thread: "<< gettid() << " entering begintx\n";
#endif
	
	struct t_info *td = (struct t_info*) arg;
//YOUR CODE HERE 

	start_operation(td->tid, td->count);
	fflush(stdout);
	// create a new transx object
	transx *newTr = new transx(td->tid, TR_ACTIVE, td->type, pthread_self());
	transx_P(0);
	// add to the list
	if (t_manager->lastr == NULL) 
		t_manager->lastr = newTr;
	else {
		newTr->nextr = t_manager->lastr;
		t_manager->lastr = newTr;
	}

	transx_V(0);
	finish_operation(td->tid);
#ifdef TM_DEBUG
	 cout << "thread: "<< gettid() << " leaving begintx\n";
#endif
	pthread_exit(NULL);

}

/* Method to handle Readtx action in test file    */
/* Inputs a pointer to structure that contans     */
/* tx id and object no to read. Reads the object  */
/* if the object is not yet present in hash table */
/* or same tx holds a lock on it. Otherwise waits */
/* until the lock is released */


void *readtx(void *arg)
{
#ifdef TM_DEBUG
	cout << "thread: "<< gettid() << "  entering readtx\n";
#endif

	//YOUR CODE HERE
	struct t_info *td = (struct t_info*) arg;

	start_operation (td->tid, td->count);

	transx_P(0);
	transx *curr_tx = get_tx(td->tid);
	cout << curr_tx;

	// if this transaction does not exist then return
	if (curr_tx == NULL) {
		cout << "thread: "<< gettid() << "  Trying to read a nonexistent transaction: "<<td->tid<<"\n";
		transx_V(0);
		finish_operation (td->tid);
		pthread_exit(NULL);
		return (0);
	}
	// if this transaction is aborted then return else perform read
	if (curr_tx->status != TR_ABORT) {
		curr_tx->set_lock(curr_tx->tid, 1, td->obno, td->count, 'S');
	} else {
		//cout << "thread: "<< gettid() <<" abort the transaction marked abort\n";
		//curr_tx->status = TR_ABORT;
		//do_commit_abort(curr_tx->tid, curr_tx->status);
		transx_V(0);
	}
#ifdef TM_DEBUG
	cout << "thread: "<< gettid() <<"   leaving readtx\n";
	fflush(stdout);
#endif
	finish_operation (td->tid);
	pthread_exit(NULL);

}

/*Similar with readtx but to handle Writetx action*/
void *writetx(void *arg) 
{ 
#ifdef TM_DEBUG
	cout << "thread: "<< gettid() <<"  entering writetx\n";
#endif

	//YOUR CODE HERE
	struct t_info *td = (struct t_info*) arg;

	start_operation (td->tid, td->count);
	transx_P(0);
	
	transx *curr_tx = get_tx(td->tid);
	// if this transaction does not exist then return
    if (curr_tx == NULL) {
        cout <<"thread: "<< gettid() <<"  Trying to read a nonexistent transaction: "<<td->tid<<"\n";
		transx_V(0);
		finish_operation (td->tid);
		pthread_exit(NULL);
        return (0);
    }
	// if this transaction is already aborted then return else perform write
    if (curr_tx->status != TR_ABORT) {
        curr_tx->set_lock(curr_tx->tid, 1, td->obno, td->count, 'X');
    } else {
		//curr_tx->status = TR_ABORT;
        //do_commit_abort(curr_tx->tid, curr_tx->status);
        transx_V(0);
    } 
#ifdef TM_DEBUG
	cout << "thread: "<< gettid() <<"  leaving writetx\n";
#endif

	finish_operation (td->tid);
	pthread_exit(NULL);

}


/*Handle of abort action*/
void *aborttx(void *arg)
{
	//YOUR CODE HERE 
#ifdef TM_DEBUG
	cout << "thread: "<< gettid() <<"  entering aborttx\n";
#endif

	struct t_info *td = (struct t_info*) arg;
	start_operation (td->tid, td->count);
	transx_P(0);		

	// call abort transaction after setting locks.
	do_commit_abort(td->tid, TR_ABORT) ;

	transx_V(0);
	finish_operation (td->tid);
	pthread_exit(NULL);
	
#ifdef TM_DEBUG
	cout << "thread: "<< gettid() <<"  leaving aborttx\n";
#endif

}

/*Handle of commit action*/
void *committx(void *arg)
{
	//YOUR CODE HERE 
#ifdef TM_DEBUG
	cout<<"thread: "<< gettid() <<"  entering committx\n";
#endif

	struct t_info *td = (struct t_info*) arg;
	start_operation (td->tid, td->count);
	transx_P(0);
        
	transx *curr_tx = get_tx(td->tid);

	// if this transaction does not exist then return
    if (curr_tx == NULL) {
        cout <<"thread: "<< gettid() <<" ERROR: Trying to commit/abort a non existent transaction\n";
	    transx_V(0);
      	finish_operation (td->tid);
       	pthread_exit(NULL);
        return(0);
    }

	// if this transaction is already aborted then return
	/*if (curr_tx->status == TR_ABORT) {
        cout << "thread: "<< gettid() << "calling transx_V(0) from committx \n";
        transx_V(0);
        finish_operation (td->tid);
        cout << "thread: "<< gettid() <<"  leaving committx\n";
        pthread_exit(NULL);
        return(0);
	}*/

	if (curr_tx->status != TR_ABORT) {

		// if this transaction is not aborted then continue
		do_commit_abort(td->tid, TR_END) ;
	}
	transx_V(0);
	finish_operation (td->tid);
#ifdef TM_DEBUG
	cout << "thread: "<< gettid() <<"  leaving committx\n";
#endif
	pthread_exit(NULL);
}


/* this method sets lock on objno1 with lockmode1 for a tx in this*/

int transx::set_lock(long tid1, long sgno1, long obno1, int count, char lockmode1) {
	//if the thread has to wait, block the thread on a semaphore from the
	//sempool in the transaction manager. Set the appropriate parameters in the
	//transaction list if waiting.
	//if successful  return(0);
	// If the owner is different, current txn may go to deadlock
	// so we will put the transaction in waiting mode and see if deadlock happens 
	// If the requested object is no longer acquired by any other transaction,
	// we will lock the object for this transaction

	//YOUR CODE HERE
#ifdef TM_DEBUG
	cout << "thread: "<< gettid() <<"  entering TxWnsx::set_lock\n";
#endif
	int t_status;

	//open_logfile_for_append();											
	transx_hlink *amIowner = hash_t1->findt(this->tid, sgno1, obno1);				

	// if this object is not occupied by current transaction else then 
	// check who is the owner
	if (amIowner == NULL) {
		transx_hlink * whoIsOwner = hash_t1->find(sgno1, obno1);
		if (whoIsOwner == NULL) {
			hash_t1->add(this, sgno1, obno1, lockmode1);
			transx_V(0);
			perform_readWrite(tid1, obno1, lockmode1);
		} else {
			//if requested object is acquired by other transaction 
			//then wait till the object becomes free
			//also check if we can enter deadlock while waiting
#ifdef TM_DEBUG
			cout << "thread: "<< gettid() << " " << whoIsOwner->tid << " is owner of "<< obno1 <<"\n";
#endif
			transx_V(0);
			//Set the appropriate parameters in the transaction list if waiting
			wait_for *dd = new wait_for();					
			this->setTx_semno(whoIsOwner->tid, whoIsOwner->tid);
			this->status = TR_WAIT;					
			this->lockmode = lockmode1;
			this->obno = obno1;

			// make this transaction wait so that we can check if we have a deadlock
			if (dd->deadlock() != 1)
			{
#ifdef TM_DEBUG
				cout << "thread: "<< gettid() <<"  owner tid: " << whoIsOwner->tid<<"\n";
#endif
				// if deadlock wont happen then we can continue
				transx_P(whoIsOwner->tid);

				if (this->status != TR_ABORT) 
					this->status = TR_ACTIVE;
				transx_P(0);
				set_lock(this->tid, sgno1, obno1, count, lockmode1);
			}
			//since currently in this peroject perform_readwrite() is modifying the value of 
			//object even for read operation ('S' lock mode) hence we did not distinguish this 
			//function for 'S' and 'X' lock mode. 
			//If, read was not modifying the objects value then set_lock would allow two 'S' transactions
			//together while blcoking the writes.
			//Still, we are implementing this effect here by not aborting if the transaction is demanding
			//a shared lock when we do wound wait( instead we ask it to wait). 
			//We only abort when we are requesting 'X' lock.
		}
	} else {
		// if this object is occupied by current transaction then
		// we can continue performing read write	
#ifdef TM_DEBUG
		cout << "thread: "<< gettid() << " " << this->tid <<" is owner of "<< obno1 <<"\n";
#endif

		this->status = TR_ACTIVE;
		this->lockmode = lockmode1;
		transx_V(0);
		this->perform_readWrite(tid, obno1, lockmode1);
	}
#ifdef TM_DEBUG
	cout<<"thread: "<< gettid() <<" leaving TxWnsx::set_lock\n";
#endif
	return (0);

}


// this part frees all locks owned by the transaction
// Need to free the thread in the waiting queue
// try to obtain the lock for the freed threads
// if the process itself is blocked, clear the wait and semaphores

int transx::free_locks() {


	//YOUR CODE HERE 
#ifdef TM_DEBUG
	cout<<"thread: "<< gettid() <<" entering transx::free_locks \n";
#endif

	//open_logfile_for_append();
	transx_hlink *link = head;

	while (link != NULL) {

		hash_t1->remove(this, 1, (long)link->obno) ;
		link = link->nextp;
	}

	fprintf(logfile, "\n");
	fflush(logfile);
#ifdef TM_DEBUG
	cout<< "thread: "<< gettid() <<" leaving transx::free_locks\n";
#endif

	return(0);	

}

// called from commit/abort with appropriate parameter to do the actual
// operation. Make sure you give error messages if you are trying to
// commit/abort a non-existant tx

void *do_commit_abort(long t, char status)
{
	//YOUR CODE HERE
#ifdef TM_DEBUG
	cout<<"thread: "<< gettid() << " entering do_commit_abort\n";
#endif

	int waiting;
	transx *curr_tx = get_tx(t);
	open_logfile_for_append();

	if (curr_tx == NULL) {
		return(0);
	}

	//set the status to commit or abort
	curr_tx->status = status;								

	if (status == TR_END) {
		fprintf(logfile, "T%d\t      \tCommitTx\t\t\t\t         \t\t        %c\n",
				curr_tx->tid, curr_tx->status);
		fflush(logfile);
	} else {
		fprintf(logfile, "T%d\t      \tAbortTx\t\t\t\t         \t\t        %c\n",
				curr_tx->tid,  curr_tx->status);
		fflush(logfile);
	}

	// check if there are transactions waiting for this semaphore	
	// if yes then release the semaphore
	if(curr_tx->semno >= 0){
		waiting = transx_wait(curr_tx->semno); 
		if (waiting > 0) {
			for (int i = waiting; i > 0; i--) {
				transx_V(curr_tx->semno);
			}
		}
	}
	//free all the locks which are held by this transaction
	//before removing it from the transaction manager list
	curr_tx->free_locks();
	curr_tx->remove_tx();
#ifdef TM_DEBUG
	cout<<"thread: "<< gettid() <<" leaving do_commit_abort\n";
#endif
}


int transx::remove_tx() 
{
	//remove the transaction from the TM
	transx *txptr, *lastr1;
	lastr1 = t_manager->lastr;
	for (txptr = t_manager->lastr; txptr != NULL; txptr = txptr->nextr) { // scan through list
		if (txptr->tid == this->tid) { // if req node is found          
			lastr1->nextr = txptr->nextr; // update nextr value; done
			//delete this;
			return (0);
		} else lastr1 = txptr->nextr; // else update prev value
	}
	fprintf(logfile, "Trying to Remove a Tx:%d that does not exist\n", this->tid);
	fflush(logfile);
	printf("Trying to Remove a Tx:%d that does not exist\n", this->tid);
	fflush(stdout);
	return (-1);
}






// CURRENTLY Not USED
// USED to COMMIT
// remove the transaction and free all associate dobjects. For the time being
// this can be used for commit of the transaction.

int transx::end_tx() 
{
	transx *linktx, *prevp;

	linktx = prevp = t_manager->lastr;
	while (linktx) {
		if (linktx->tid == this->tid) 
			break;
		prevp = linktx;
		linktx = linktx->nextr;
	}
	if (linktx == NULL) {
		printf("\ncannot remove a Tx node; error\n");
		fflush(stdout);
		return (1);
	}
	if (linktx == t_manager->lastr) 
		t_manager->lastr = linktx->nextr;
	else {
		prevp = t_manager->lastr;

		while (prevp->nextr != linktx) 
			prevp = prevp->nextr;

		prevp->nextr = linktx->nextr;
	}
}


// check which other transaction has the lock on the same obno
// returns the hash node

transx_hlink * transx::others_lock(transx_hlink *hnodep, long sgno1, long obno1) {
	transx_hlink *ep;
	ep = hash_t1->find(sgno1, obno1);
	while (ep) // while ep is not null
	{
		if ((ep->obno == obno1)&&(ep->sgno == sgno1)&&(ep->tid != this->tid))
			return (ep); // return the hashnode that holds the lock
		else ep = ep->next;
	}
	return (NULL); //  Return null otherwise 

}

// routine to print the tx list
// TX_DEBUG should be defined in the Makefile to print

void transx::print_tm() {

	transx *txptr;

#ifdef TX_DEBUG
	printf("printing the tx list \n");
	printf("Tid\tTxType\tThrid\t\tobjno\tlock\tstatus\tsemno\n");
	fflush(stdout);
#endif
	txptr = t_manager->lastr;
	while (txptr != NULL) {
#ifdef TX_DEBUG
		printf("%d\t%c\t%d\t%d\t%c\t%c\t%d\n", txptr->tid, txptr->Txtype, txptr->pid, txptr->obno, txptr->lockmode, txptr->status, txptr->semno);
		fflush(stdout);
#endif
		txptr = txptr->nextr;
	}
	fflush(stdout);
}



// routine to perform the acutual read/write operation
// based  on the lockmode

void transx::perform_readWrite(long tid, long obno, char lockmode) {

	int j;
	double result = 0.0;
	cout <<"thread: "<< gettid() <<" In read write\n";
	open_logfile_for_append();

	int obValue = t_manager->objarray[obno]->value;
	if (this->status != TR_ABORT) {
		if (lockmode == 'X') // write op
		{
			t_manager->objarray[obno]->value = obValue + 1; // update value of obj  
			fprintf(logfile, "T%d\t      \tWriteTx\t\t%d:%d:%d\tWriteLock\tGranted\t\t %c\n",
					this->tid, obno, t_manager->objarray[obno]->value, t_manager->optime[tid], this->status);
			fflush(logfile);
			for (j = 0; j < t_manager->optime[tid]*20; j++)
				result = result + (double) random();
		} else { //read op
			t_manager->objarray[obno]->value = obValue - 1; // update value of obj  
			fprintf(logfile, "T%d\t      \tReadTx\t\t%d:%d:%d\tReadLock\tGranted\t\t %c\n",
					this->tid, obno, t_manager->objarray[obno]->value, t_manager->optime[tid], this->status);
			fflush(logfile);
			for (j = 0; j < t_manager->optime[tid]*10; j++)
				result = result + (double) random();
		}
	}
	cout <<"thread: "<< gettid() <<" exit read write\n";

}

// routine that sets the semno in the Tx when another tx waits on it.
// the same number is the same as the tx number on which a Tx is waiting

int transx::setTx_semno(long tid, int semno) {
	transx *txptr;

	txptr = get_tx(tid);
	if (txptr == NULL) {
		printf("\n:::ERROR:Txid %d wants to wait on sem:%d of tid:%d which does not exist\n", this->tid, semno, tid);
		fflush(stdout);
		return (-1);
	}
	if (txptr->semno == -1) {
		txptr->semno = semno;
		return (0);
	} else if (txptr->semno != semno) {
#ifdef TX_DEBUG
		printf(":::ERROR Trying to wait on sem:%d, but on Tx:%d\n", semno, txptr->tid);
		fflush(stdout);
#endif
		exit(1);
	}
	return (0);
}



void *open_logfile_for_append() {

	if ((logfile = fopen(t_manager->logfile, "a")) == NULL) {
		printf("\nCannot open log file for append:%s\n", t_manager->logfile);
		fflush(stdout);
		exit(1);
	}
}
