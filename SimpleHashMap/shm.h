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

// Simple Hash Function https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
unsigned int simplehash(unsigned int x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}
unsigned int simpleunhash(unsigned int x) {
    x = ((x >> 16) ^ x) * 0x119de1f3;
    x = ((x >> 16) ^ x) * 0x119de1f3;
    x = (x >> 16) ^ x;
    return x;
}

extern map_t* shm_new(int size);
extern void shm_put(map_t *map, int key, int value);
extern int shm_get(map_t *map, int key);

#endif