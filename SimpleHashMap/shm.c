#include "shm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../config.h"

//Hash Function used, proven to provide good uniform distrbution, see report for source.
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

//Creates new hashmap instance of given initial size. Returns pointer to hashmap.
map_t *shm_new( int size ) {
    map_t *map = (map_t*)malloc(sizeof(map_t));
    map->size = size;

    //Init variables used for simulating memory read/write.
    strcpy(map->EXAMPLEDATA_R, "abcdefghijklmnopqrstuvwxyzABCDE");
    strcpy(map->EXAMPLEDATA_W, "abcdefghijklmnopqrstuvwxyzABCDE");

    //Malloc entire hashmap of initial size.
    map->table = malloc(sizeof(dataval_t)*size);
    if(!map->table)
        printf("Allocation error in NEW \n");

    //Zero out address space to prevent false 
    memset(map->table, 0, sizeof(dataval_t)*size);
    pthread_rwlock_init(&map->lock, NULL);

    return map;
}

//Insert key/value pair into hashmap.
void shm_put(map_t *map, int key, int value){

    //Find which bucket the key/value pair should reside at.
    int bucket = simplehash(key) % map->size;

    pthread_rwlock_wrlock(&map->lock);

    //If bucket has not been used, insert key/value pair.
    if(map->table[bucket].used == false){
        dataval_t* record = &map->table[bucket];
        record->key = key;
        record->val = value;
        record->next = NULL;
        record->used = true;

        //Simulate memory write.
        strncpy(record->simdata, map->EXAMPLEDATA_R, 32);


    //If bucket has been used.    
    }else{
        dataval_t* record = &map->table[bucket];

        //Is the key at the bucket the same key? Then replace value.
        if(record->key==key){
            record->val = value;
            strncpy(record->simdata, map->EXAMPLEDATA_R, 32);
            pthread_rwlock_unlock(&map->lock);
            return;
        }
            
        //While not the same key, iterate to next key.
        while(record->next != NULL){
            record = record->next;

            //Overwrite value if the same key.
            if(record->key==key){
                record->val = value;
                strncpy(record->simdata, map->EXAMPLEDATA_R, 32);
                pthread_rwlock_unlock(&map->lock);
                return;
            }
        }

        //At last link in chain, create new record to put at chains tail.
        dataval_t* new_record = malloc(sizeof(dataval_t));
        if(!new_record)
            printf("Allocation error in PUT \n");
        new_record->key = key;
        new_record->val = value;
        new_record->next = NULL;
        new_record->used = true;
        record->next = new_record;

        //Simulate memory write.
        strncpy(record->simdata, map->EXAMPLEDATA_R, 32);
        }

    pthread_rwlock_unlock(&map->lock);
}

//Get value from hashmap for given key. Returns pointer to value of key, NULL if key not exist.
int* shm_get(map_t *map, int key){

    //Find which bucket the key/value pair should reside at.
    int bucket = simplehash(key) % map->size;
    
    pthread_rwlock_rdlock(&map->lock);
    dataval_t* record = &map->table[bucket];

    //If the bucket is empty, return NULL. (key not in hashmap)
    if(record->used == false){
        pthread_rwlock_unlock(&map->lock);
        return NULL;
    }else{

        //Is the key at the first record in the bucket?
        if(record->key == key){
            
            //Simulate overwriting the records value.
            strcpy(map->EXAMPLEDATA_W, record->simdata);
            pthread_rwlock_unlock(&map->lock);
            return &record->val;

        //Key not at first record, iterate through the keys in the chain.
        }else{
            do{
                //Is the key at the record?
                if(record->key == key){

                    //Simulate overwriting the records value.
                    strcpy(map->EXAMPLEDATA_W, record->simdata);
                    pthread_rwlock_unlock(&map->lock);
                    return &record->val;
                }
                record = record->next;
            }while(record != NULL);
            
        }
    }
pthread_rwlock_unlock(&map->lock);

//Key has not been found in the hasmap.
return NULL;
}