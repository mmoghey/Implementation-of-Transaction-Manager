#include <stddef.h>

#define READWRITE 0

int errno;

key_t t_keysem;
int t_semid;
int t_semnum;

transx_hash_table * hash_t1;
transx_manager * t_manager;
transx *transx_obj;

int transx_Initp;
int transx_errno =0;