#ifndef __GHT_H__
#define __GHT_H__
#include <pthread.h>

typedef struct{
    queue_t* msgq;  //Message queue held by every subtree
    int threadnum;  //Thread number of this subtree
}subtree_t;

typedef struct{
    int numthreads;             //Number of threads/subtrees in database
    subtree_t** subtreelist;    //List of subtrees in database
}db_t;

db_t *db_new();
int db_put(db_t *db_data, char *key, char *val);
char* db_get(db_t *db_data, char *key);
int db_free(db_t *db_data);




#endif