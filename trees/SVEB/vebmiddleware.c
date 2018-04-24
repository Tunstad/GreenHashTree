#include <staticvebtree.h>

void *initialize_tree(){
    int ret_val = init_tree(0);

    //Returned to allow greenhashtree layer to be unaware of underlying data structure.
    return void* middlewarepointer;
}

void *insert_into_tree(void *tree, int key, int val){
    int ret_val = insert(key);

    //Returned to allow greenhashtree layer to be unaware of underlying data structure.
    return void* middlewarepointer;
}

int search_tree(void *tree, int key){
    int retval = search_test(key);

    //If 1, OK
    return retval;
}