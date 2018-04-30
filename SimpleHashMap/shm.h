#ifndef __SHM_H__
#define __SHM_H__
#include <stdbool.h>

typedef struct dataval dataval_t;
struct dataval{
    int key;
    int val;
    dataval_t* next;
    bool used;
};

typedef struct{
    dataval_t *table;
    int size;
}map_t;

extern map_t* shm_new(int size);
extern void shm_put(map_t *map, int key, int value);
extern int* shm_get(map_t *map, int key);

#endif