#define NULL	0
#define TRUE	1
#define FALSE	0
#include "transx_def.hpp"
#include "transx_manager.hpp"
#include "transx_extern.hpp"
#include <sys/types.h>
#include <sys/syscall.h>

extern transx_manager *t_manager;
extern FILE *logfile;
/*void *open_logfile_for_append() {

        if ((logfile = fopen(t_manager->logfile, "a")) == NULL) {
                printf("\nCannot open log file for append:%s\n", t_manager->logfile);
                fflush(stdout);
                exit(1);
        }
}*/
extern void *open_logfile_for_append();

/*wait_for constructor
 * Initializes the waiting transaction in wtable
 * and setting the appropriate pointers.
 */
wait_for::wait_for() {
	transx *tp;
	node* np;
	wtable = NULL;
	fprintf(stdout, "::entering wait_for method:::\n");
	fflush(stdout);
	for (tp = (transx *) t_manager->lastr; tp != NULL; tp = tp->nextr) {
		if (tp->status != TR_WAIT) {
			cout <<"\nTansaction : "<<tp->tid<< "not waiting\n";
			continue;
		}
		np = new node;
		np->tid = tp->tid;
		np->sgno = tp->sgno;
		np->obno = tp->obno;
		np->lockmode = tp->lockmode;
		np->semno = tp->semno;
		np->next = (wtable != NULL) ? wtable : NULL;
		np->parent = NULL;
		np->next_s = NULL;
		np->level = 0;
		wtable = np;
		printf("\nwtable****%d %d %d %c %d %d\n", wtable->tid, wtable->sgno, wtable->obno, wtable->lockmode, wtable->semno, wtable->level);
	}
	fprintf(stdout, "::leaving wait_for method:::\n\n");
	fflush(stdout);
}



/*
 * deadlock(), this method is used to check the waiting transaction for deadlock
 * go through the wtable and create a graph
 * use the traverse method to find the victim
 * if the transaction tid matches to that of the victim then
 * set the status of transaction to TR_ABORT
 * use the tp->free_locks() to release the lock owned by that transaction
 */
int wait_for::deadlock() {
	fprintf(stdout, "::entering deadlock method:::\n");
	fflush(stdout);

	//YOUR CODE HERE
	transx *curr_tx;
	for (node *ptr = wtable; ptr != NULL; ptr = ptr->next) {
		cout <<"wtapble ptr->tid: "<<ptr->tid<<" level: "<<ptr->level; 
		if (ptr->level == 0) {
			head = new node();
			head->lockmode = ptr->lockmode;
			head->semno = ptr->semno;
			head->obno = ptr->obno;
			head->tid = ptr->tid;
			head->sgno = ptr->sgno;
			head->next = head->parent = head->next_s = NULL;
			head->level = 1;
			// check if we have a deadlock by checking for the cycles in the graph
			found = FALSE;
			// traverse function will set found to TRUE if found
			traverse(head);

			// if we have found a cycle and a victim then start process to abort it.
			if ((found==TRUE)&&(victim != NULL)) {
				cout <<"\nThere is a cycle. Victim "<<victim->tid<<" will abort now\n";
                transx_P(0);
				for (curr_tx = (transx*) t_manager->lastr; curr_tx != NULL; curr_tx = curr_tx->nextr){
					if (curr_tx->tid == victim->tid) 
						break;
				}
				// set the status to abort
				curr_tx->status = TR_ABORT;
				open_logfile_for_append();

                //update the log file
				fprintf(logfile, "T%d\t      \tAbortTx\t\t\t\t         \t\t        %c\n",
                                curr_tx->tid,  curr_tx->status);
                fflush(logfile);
	
				// check if there are transactions waiting for this semaphore
        		// if yes then release the semaphore
				if (curr_tx->semno >= 0) 
				{
					int waiting = transx_wait(curr_tx->semno);		
					if (waiting > 0)
						for (int i = waiting; i > 0; i--)
							transx_V(curr_tx->semno);
				}

	    	    //free all the locks which are held by this transaction
    	    	//before removing it from the transaction manager list
				curr_tx->free_locks();
				curr_tx->remove_tx();			

                transx_V(0);
				fprintf(stdout, "\n::leaving deadlock method:::\n");
				fflush(stdout);
                return 1;
            
			}
		}
	}
	fprintf(stdout, "\n::leaving deadlock method:::\n");
	fflush(stdout);
	return (0);
}

/*
 * this method is used by deadlock() find the owners
 * of the lock which are being waiting if it is visited
 * location is not null then it means a cycle is found
 * so choose a victim set the found to TRUE and return.
 * 
 * Create a node q set its appropriate pointers
 * assign the tid, sgno,lockmode, obno, head and last
 * to q.
 * 
 * Mark the wtable by going through wtable
 * and find the tid in hashash_t1(hlink) equal to
 * q's tid and set the level of q to 1 set the hlink
 * pointer by traveling it to proper obno and sgno
 * as in p.
 */ 

int wait_for::traverse(node* p) {

 	node *q, *last = NULL;
	transx *itr;
    transx_hlink *sp;

    sp = hash_t1->find(p->sgno, p->obno);
		

	cout << "\nIn wait_for::traverse() \n";
	// check all the nodes which are in the hash_table to see if
	// we have already visited those nodes. if yes, then we have a 
	// deadlock and we should choose a victim
    while (sp != NULL) {
		if (visited(sp->tid) == TRUE) {
			cout <<"sp->tid: "<<sp->tid<<"\n";
            q = head;
            while (q != NULL) {
				if (sp->tid == q->tid) {
					// we have this node waiting for other nodes
					// we have a cycle so resolve it by chooseing a victim
					if (q->level > 0) {
                        found = TRUE;
                        victim = choose_victim(p, q);
                        return (0);
                    } else if (q->level == 0)
                        q->level -= 1;
				}
				q = q->next;
            }
			return (0);
        }

		// we havent seen this node in table 
		// so add this now
		q = new node();							
	
		q->obno = sp->obno;						
        q->lockmode = sp->lockmode;
        q->tid = sp->tid;
		q->sgno = sp->sgno;

        q->next = head;
		q->next_s = NULL;
        head = q;
        last = q;

		// assign a wtable to this node
        q = wtable;
        q->level = 1;
        q = q->next;
        sp = sp->next;

		while (sp != NULL) {
            if ((sp->obno == p->obno)&&(sp->sgno == p->sgno)) {
                break;
            }
            sp = sp->next;
        }
    }

    q = last; 

	// traverse the active transactions to find out 
	// dependencies and add those in the graph
	// call this function in recursion to form the graph
	while (q != NULL) {
        itr = (transx *) t_manager->lastr;	
        while (itr != NULL) {
            if (itr->tid == q->tid) {
                if (q->level >= 0) {
					if (itr->status == TR_WAIT) {
						q->semno = itr->semno;
						q->sgno = itr->sgno;
						q->obno = itr->obno;
						q->lockmode = itr->lockmode;
						q->level = p->level++;
						
						traverse(q);					
						if (found)
							return (0);
					}
				}
            }
            itr = itr->nextr;
        }
    
        delete last;		
        last = q->next_s;
        q = last;
    }
   	return 0;
}


/*
 * this method is used by traverse method to choose a
 * victim after a deadlock is encountered while traversing
 * the wait_for graph
 */
node* wait_for::choose_victim(node* p, node* q) {
	node* n;
	int total;
	transx * ptr;
	
	//traverse through the dependency graph
	//the trnsaction which is older will kill other transaction
	for (n=p; n != q->parent; n=n->parent) {
		cout << "\nIn choose_victim " << n->tid << " " << p->tid << " " << q->tid <<"\n";
		if (n->lockmode == 'X') {
			if (q->lockmode == 'X') {
				// check the operation time to figure out if we have to wound or wait
				if (t_manager->optime [n->tid] > t_manager->optime[q->tid])
					return (n);
				else 
					return (q);
			} else {
				return (n);
			}
		}  

	}
	return (NULL);
}


/*
 * this method is used by traverse() to keep track of the
 * visited transaction while traversing through the wait_for
 * graph using the tid of transaction in hash table(hlink pointer)
 * similar to a DFS
 */
int wait_for::visited(long t) {
		node* n;
		for (n = head; n != NULL; n = n->next) if (t == n->tid) return (TRUE);

		return (FALSE);
};

/*
 * this method is used by the traverse() method in
 * to get the node in wtable using the tid of transaction
 * in hash table(hlink pointer).
 */
node * wait_for::location(long t) {
		node* n;
		for (n = head; n != NULL; n = n->next)
				if ((t == n->tid)&&(n->level >= 0)) return (n);
		return (NULL);
}


