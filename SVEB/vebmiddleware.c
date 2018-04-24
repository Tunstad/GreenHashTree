#include "staticvebtree.h"
#include "stddef.h"

void *initialize_tree(){
    int ret_val = init_tree(10);

    //Returned to allow greenhashtree layer to be unaware of underlying data structure.
    return NULL;
}

void *insert_into_tree(void *tree, int key, int val){
    int ret_val = insert(key);

    //Returned to allow greenhashtree layer to be unaware of underlying data structure.
    return NULL;
}

int search_tree(void *tree, int key){
    int retval = search_test(key);

    //If 1, OK
    return retval;
}