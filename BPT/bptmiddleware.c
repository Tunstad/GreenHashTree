#include "bpt.h"
#include "bptmiddleware.h"

void *init_struct(){
    //Create an initial root node for the tree.
    node *root = insert(NULL, 0, 0);

    //Return handle to the tree.
    return root;
}

void *insert_into_struct(void *root, int key, int value){

    //Insert value into tree. Insert returns trees root (after potential rebalance).
    void* newroot = insert(root, key, value);

    return newroot;
}

int* search_struct(void *root, int key){

    //Search the tree for key. Returned record holds the value (if key exist in tree).
    record *node_record = find(root, key, 0);

    //If key does not exist in the tree, NULL is returned to signal key miss.
    if(node_record == NULL)
        return NULL;

    //Return the value of the key.
    return &node_record -> value;
}
