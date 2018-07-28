#include <stdio.h>
#include <stdlib.h>
#include "transx_def.hpp"
#include "transx_manager.hpp"


extern transx_manager *t_manager;

// finds the object in the hash table; returns NULL if it is not there

transx_hlink *transx_hash_table::find (long sgno, long obno){

	transx_hlink *linkp;

  	//  Hash on the object number in the obno and search for the obno
  	//  in the bucket

  	linkp = t_manager->head[hashing(sgno,obno)];
  
  	while (linkp != NULL){
      	if ((linkp->obno == obno)&& (linkp->sgno == sgno)) 
        	return (linkp);
      	else  
			linkp = linkp->next;
    }
  	//  Return unsuccessfully
    return (NULL); 
}

transx_hlink *transx_hash_table::findt (long tid, long sgno, long obno)
{
  	transx_hlink *linkp; 
  	int h;

  	//  Hash on the object number in the obno and search for the obno
    	//  in the bucket

  	linkp = t_manager->head[hashing(sgno, obno)];

  	while (linkp != NULL)  //check this{
      	if ((linkp->obno == obno)&& 
     	    (linkp->sgno == sgno)&&
	    (tid == linkp->tid)) 
        	return (linkp);
      	
	else 
	    linkp = linkp->next;

    

  	//  Return unsuccessfully

    return (NULL); 
}

// adds and object to the hash table. Need to pass Tx object to make sure
// links are set properly

int transx_hash_table::add ( transx *tp,long sgno, long obno,  char lockmode )
{
	cout << "\nin transx_hash_table::add " << tp->tid << " adding "<< obno << " " << sgno <<" to hash table \n";
	transx_hlink *tmp, *linkp;
     
  	tmp = t_manager->head[hashing(sgno, obno)]; //save the lock node ptr
	cout << "\ntmp"<<tmp<<"\n";
  
  	linkp = t_manager->head[hashing(sgno, obno)]=(transx_hlink*)malloc(sizeof(struct transx_hlink));
	cout << "\nlinkp : " << linkp << " hashing : " << hashing(sgno,obno)<<"\n";
  	
	if (linkp == NULL) 
			return(-1); //memory not there

  	linkp->next = tmp;
  	linkp->obno = obno;
  	linkp->sgno = sgno;
  	linkp->lockmode =lockmode ;
  	linkp->tid = tp->tid;    
  	linkp->pid = tp->pid;

  	// Add the ep to the front of the transaction it belongs to
  	linkp->nextp=tp->head;
  	tp->head = linkp;
	cout << "\ncalling print_ht() from add()\n";
	print_ht();

  	return (0);   //  Return successfully 
}

int transx_hash_table::remove ( transx *tr,long sgno, long obno )
{
	cout << "\nin transx_hash_table::remove " << tr->tid << " removing "<< obno<< " " << sgno <<" to hash table \n";
  	transx_hlink *prevp, *linkp;
  	transx_hlink *tprev, *tlink;
	cout << hashing(sgno,obno) ;

    prevp = linkp = t_manager->head[hashing(sgno,obno)];

  	while (linkp){
    	if ((linkp->tid==tr->tid)&&(linkp->obno==obno)&&(linkp->sgno==sgno)) {
			cout << "\nin transx_hash_table::remove found matching entry\n";
        	break;
		}
      	prevp = linkp;
      	linkp = linkp->next;
    }

  	if (linkp == NULL) {
		cout <<"\nin transx_hash_table:: linkp = NULL\n";	
		return (1);  // linkp not found

	}
    // first remove it off the transaction link
  	if (tr->head == linkp) 
		tr->head = linkp->nextp;
  	else {
		tprev = tlink = tr->head;
	
	while ((tlink)&&(tlink!=linkp)){
	    if (tlink== linkp) {
	      	tprev->nextp = tlink->nextp;
	      	break;
	    }
	    tprev = tlink;
	    tlink = tlink->nextp;
	  }
  	}

  	if (prevp != linkp) 
		prevp->next = linkp->next;
  	else 
		t_manager->head[hashing(sgno,obno)] = linkp->next;
  	//  Return successfully
	cout << "\ncalling print_ht() from remove()\n";
	print_ht();
  	return (0);
};

// prints the hash table if the HT_DEBUG flag is set. Shows all the elements
// along with the lockmode etc. Useful for debugging.

void transx_hash_table::print_ht(){

  	transx_hlink *hlink;
  	int i;
#ifdef HT_DEBUG
  	printf("printing the Hash table\n");
  	printf("Bucket \t Tid \t \t objno \t lockmode \n");
  	fflush(stdout);
#endif
  	
	for (i=0;i< this->size;i++){
    	hlink=t_manager->head[i];
    	if (hlink !=NULL){
#ifdef HT_DEBUG
      		printf("%d: ", i);
      		fflush(stdout);
#endif
			while (hlink != NULL) {
#ifdef HT_DEBUG
				printf("%d %d %c ->", hlink->tid, hlink->obno, hlink->lockmode);
				fflush(stdout);
#endif
				hlink = hlink->next;
   			}
   			printf("\n");
   		}
  	}
  	fflush(stdout);
}

//initializes the  hash table


transx_hash_table::transx_hash_table (int ht_size) 
{
	int i;
	transx_hlink *linkp;

	for (i=0;i<ht_size;i++)
		t_manager->head[i]=NULL; 

  	this->mask = ht_size - 1;
  	this->size = DEFAULT_HASH_TABLE_SIZE;

}

transx_hash_table::~transx_hash_table () {};
