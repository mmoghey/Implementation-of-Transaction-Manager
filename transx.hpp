#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <pthread.h>

struct transx_hlink
{
  char lockmode;
  long sgno;
  long obno;
  long tid;
  pthread_t pid;

  transx_hlink *next;        //links nodes hashed to the same bucket
  transx_hlink *nextp;       //links nodes of the same transaction
};
 

struct t_info {
    long tid;
    long thrNum;
    char type;
    long obno;
    int count;
    pthread_t thrid;
};


//def of transaction class
class transx {

 public:
  
  friend class transx_hash_table;
  friend class wait_for;
   

  long tid;         
  pthread_t pid;
  long sgno;
  long obno;
  char status;
  char lockmode;
  char Txtype; //transaction type R = Read-only or W = Read/Write
  int semno;
  transx_hlink *head;           // head of lock table
  transx_hlink *others_lock(transx_hlink *, long, long); 
  transx *nextr;
  
  public :
    
  int free_locks();
  int remove_tx();
  long get_tid(){return tid;}
  long set_tid(long t){tid = t; return tid;}
  char get_status() {return status;}
  int set_lock(long, long, long, int, char);
  int end_tx();
  transx(long,char,char,pthread_t);
  void perform_readWrite(long, long, char);
  int  setTx_semno(long, int);
  void print_tm();
  transx(){};
};




class transx_hash_table
{

  friend class transx;
  friend class wait_for;
    
 public:

  /*  methods  */
  
  transx_hlink *find (long, long); //find the obj in hash table 
  transx_hlink *findt (long, long, long); //find the tx obj belongs to
  int add ( transx *, long, long, char); //add an obj for a tx to hash table
  int remove ( transx *, long, long);  //remove a lock entry
  void print_ht();
  
  /*  constructors & destructors  */
  
  transx_hash_table (int ht_size = DEFAULT_HASH_TABLE_SIZE);
  ~transx_hash_table ();
  
 private:
  
  int  mask;
  int size;	
  int hashing(long sgno, long obno)
    {return((++sgno)*obno)&mask;}

};
