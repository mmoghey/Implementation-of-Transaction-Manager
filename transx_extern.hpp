#define READWRITE 0
extern  void *begintx(void *);
extern  void *readtx(void *);
extern  void *writetx(void *);
extern  void *aborttx(void *);
extern  void *committx(void *);
extern  transx* get_tx(long);
extern  int transx_init_sem(int);
extern void  transx_init_sem_0(int);
extern void  transx_init_sem_rest(int);
extern int   transx_sem_release(int);

extern int  transx_P(int);
extern int  transx_V(int);
extern int  transx_wait(int);


extern transx_hash_table *hash_t1;
extern transx *transx_obj;

extern int errno;

extern key_t t_keysem;
extern int t_semid;
extern int t_semnum;

extern int system(char *);
extern int transx_Initp;
extern int transx_errno;

struct param
{
  long tid, obno, count;
  char Txtype;
};
