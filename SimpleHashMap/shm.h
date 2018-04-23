#ifndef __SHM_H__
#define __SHM_H__

typedef struct{
    char *key;
    char *val;
    struct dataval_t* next;
}dataval_t;

typedef struct{
    struct dataval_t **table;
    int size;
}map_t;

extern map_t shm_new(int size);
extern int shm_put(map_t map, int key, int value);
extern int shm_get(map_t map, int key);

#endif