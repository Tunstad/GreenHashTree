#include "shm.h"

map_t *shm_new( int size ) {
    int i;
    map_t *map = (map_t*)malloc(sizeof(map_t));
    map->size = size;
    map->table = (dataval_t*)malloc(sizeof(dataval_t*)*size);
    for(i=0 i<size; i++){
        map->table[i] = NULL;
    }
    return map;

	