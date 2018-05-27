#ifndef __GHT_H__
#define __GHT_H__
#include <pthread.h>
#include "msgq.h"

typedef struct{
    queue_t* msgq;  //Message queue held by every subtree
    queue_t* resq;  //Result queue(Deprecated);
    int cpunum;     //Thread number of this subtree
}substruct_t;

typedef struct{
    int numthreads;                 //Number of threads/subtrees in database
    substruct_t** substructlist;    //List of subtrees in database
    int* intval;                    //Static int value returned to indicate value was found
}db_t;

db_t *db_new();
int* db_put(db_t *db_data, int key, int val);
int* db_get(db_t *db_data, int key);
int db_free(db_t *db_data);




#endif