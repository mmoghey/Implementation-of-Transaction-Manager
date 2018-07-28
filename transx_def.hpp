#include <stddef.h>

#define  DEFAULT_HASH_TABLE_SIZE  13

#define NTRANSACTION_TYPES 2
#define ODD 1

#define TR_ACTIVE 'P'
#define TR_WAIT   'W'
#define TR_ABORT  'A'
#define TR_END    'E'