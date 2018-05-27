#include "bpt.h"
#include "bptmiddleware.h"

void *init_struct(){
    //Insert 
    node *root = insert(NULL, 0, 0);

    return root;
}

void *insert_into_struct(void *root, int key, int value){
    void* newroot = insert(root, key, value);

    return newroot;
}

int* search_struct(void *root, int key){
    record *node_record = find(root, key, 0);

    if(node_record == NULL)
        return NULL;
    //printf("Found value: %d\n", node_record->value);

    return &node_record -> value;
}
