#ifndef __GHT_H__
#define __GHT_H__
#include <pthread.h>
#include "msgq.h"
#include "BPT/bpt.h"


typedef struct{
    node* root;
    int* intval;
}db_t;

db_t *db_new();
int* db_put(db_t *db_data, int key, int val);
int* db_get(db_t *db_data, int key);
int db_free(db_t *db_data);




#endif