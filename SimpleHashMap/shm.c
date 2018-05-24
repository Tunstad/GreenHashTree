#include "shm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../config.h"

// Hash Function used, proven to provide good uniform distrbution, see report for source.
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
    strcpy(map->EXAMPLEDATA_R, "abcdefghijklmnopqrstuvwxyzABCDE");
    strcpy(map->EXAMPLEDATA_W, "abcdefghijklmnopqrstuvwxyzABCDE");
    map->table = malloc(sizeof(dataval_t)*size);
    if(!map->table)
        printf("Allocation error in NEW \n");
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
        strncpy(record->simdata, map->EXAMPLEDATA_R, 32);


        
    }else{
        dataval_t* record = &map->table[bucket];
        if(record->key==key){
            record->val = value;
            strncpy(record->simdata, map->EXAMPLEDATA_R, 32);
            pthread_rwlock_unlock(&map->lock);
            return;
        }
            
        //Iterate to next key
        while(record->next != NULL){
            record = record->next;
            //Overwrite key if existing
            if(record->key==key){
                record->val = value;
                strncpy(record->simdata, map->EXAMPLEDATA_R, 32);
                pthread_rwlock_unlock(&map->lock);
                return;
            }
        }
        dataval_t* new_record = malloc(sizeof(dataval_t));
        if(!new_record)
            printf("Allocation error in PUT \n");
        new_record->key = key;
        new_record->val = value;
        new_record->next = NULL;
        new_record->used = true;
        record->next = new_record;

        strncpy(record->simdata, map->EXAMPLEDATA_R, 32);
        }

    pthread_rwlock_unlock(&map->lock);
}


int* shm_get(map_t *map, int key){
    int bucket = simplehash(key) % map->size;
    
    pthread_rwlock_rdlock(&map->lock);
    dataval_t* record = &map->table[bucket];

    if(record==NULL)
        return NULL;

    if(record->used == false){
        pthread_rwlock_unlock(&map->lock);
        return NULL;
    }else{
        if(record->key == key){
            
            strcpy(map->EXAMPLEDATA_W, record->simdata);
            pthread_rwlock_unlock(&map->lock);
            return &record->val;
        }else{
            do{
                if(record->key == key){
                    strcpy(map->EXAMPLEDATA_W, record->simdata);
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