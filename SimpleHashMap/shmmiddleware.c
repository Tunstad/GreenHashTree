#include "shm.h"
#include "shmmiddleware.h"

void *init_struct(){

    //Initialze hashmap with size so large that significant chaining does not affect measurements.
    //Shm_new mallocs room for n number of elements at once, which improves operations per second.
    map_t *map = shm_new(10000000);

    return map;
}

void *insert_into_struct(void *map, int key, int value){

    //Insert key/value pair into hashmap.
    shm_put(map, key, value);

    //Returned to allow layer to be unaware of underlying data structure, B+ Tree would return root here.
    return map;
}

int* search_struct(void *map, int key){

    //Get value from hashmap for given key.
    int* value = shm_get(map, key);

    return value;
}
