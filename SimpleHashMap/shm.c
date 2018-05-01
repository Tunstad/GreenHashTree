#include "shm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

map_t *shm_new( int size ) {
    map_t *map = (map_t*)malloc(sizeof(map_t));
    map->size = size;
    map->table = malloc(sizeof(dataval_t)*size);
    memset(map->table, 0, sizeof(dataval_t)*size);
    pthread_rwlock_init(&map->lock, NULL);


    return map;
}

void shm_put(map_t *map, int key, int value){
    int bucket = simplehash(key) % map->size;

    pthread_rwlock_wrlock(&map->lock);

    if(map->table[bucket].used == false){
        dataval_t* record = &map->table[bucket];
        record->key = key;
        record->val = value;
        record->next = NULL;
        record->used = true;
        
    }else{
        dataval_t* record = &map->table[bucket];
        while(record->next != NULL){
            record = record->next;
        }
        dataval_t* new_record = malloc(sizeof(dataval_t));
        new_record->key = key;
        new_record->val = value;
        new_record->next = NULL;
        new_record->used = true;
        record->next = new_record;
    }
    pthread_rwlock_unlock(&map->lock);
}


int* shm_get(map_t *map, int key){
    int bucket = simplehash(key) % map->size;

    pthread_rwlock_rdlock(&map->lock);

    dataval_t* record = &map->table[bucket];
    if(record->used == false){
        pthread_rwlock_unlock(&map->lock);
        return NULL;
    }else{
        if(record->key == key){
            pthread_rwlock_unlock(&map->lock);
            return &record->val;
        }else{
            do{
                if(record->key == key){
                    pthread_rwlock_unlock(&map->lock);
                    return &record->val;
                }
                record = record->next;
            }while(record != NULL);
            
        }
    }
pthread_rwlock_unlock(&map->lock);
return NULL;
}
/*
int main (void) {
int result;
map_t* testmap = shm_new(50);

for(int i = 0; i < 100; i++){
    printf("Putting key: %d and result %d \n", i, i+1);
    shm_put(testmap, i, i+1);

    result = shm_get(testmap, i);
    printf("Got result %d \n", result);
}
return 1;
} */