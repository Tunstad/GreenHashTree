#include "shm.h"
#include "shmmiddleware.h"

void *init_struct(){
    map_t *map = shm_new(10000000);

    return map;
}

void *insert_into_struct(void *root, int key, int value){
    shm_put(root, key, value);

    return root;
}

int* search_struct(void *root, int key){
    int* value = shm_get(root, key);
    return value;
}
