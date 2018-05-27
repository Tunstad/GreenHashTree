#include "staticvebtree.h"
#include "stddef.h"

void *init_struct(){
    //Initialze VEB-tree with size so large that rebalancing does not affect measurements
    int ret_val = init_tree(2000000);

    //Returned to allow layer to be unaware of underlying data structure, B+ Tree would return root here.
    return NULL;
}

void *insert_into_struct(void *tree, int key, int val){

    //Insert value into VEB-Tree
    int ret_val = insert(key);

    //Returned to allow layer to be unaware of underlying data structure, B+ Tree would return root here.
    return NULL;
}

int* search_struct(void *tree, int key){

    int* retval = search_test(key);

    //Return value is zero, then value was not found
    if(*retval == 0){
        
        return NULL;
    }
    return retval;
}