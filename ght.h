#ifndef __GHT_H__
#define __GHT_H__
#include <pthread.h>

//dummy keystore use int as dbstructure
typedef struct{
    queue_t msgq;
    int threadnum;
}subtree_t;

typedef struct{
    int numthreads;
    subtree_t** threadlist;
}db_t;

db_t *db_new();
int db_put(db_t *db_data, char *key, char *val);
char* db_get(db_t *db_data, char *key);
int db_free(db_t *db_data);




#endif