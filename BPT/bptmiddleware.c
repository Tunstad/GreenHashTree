#include "bpt.h"
#include "bptmiddleware.h"

void *initialize_tree(){
    node *root = insert(NULL, 0, 0);

    //node* root = malloc(sizeof(node*));

    return root;
}

void *insert_into_tree(void *root, int key, int value){
    void* root2 = insert(root, key, value);

    return root2;
}

int* search_tree(void *root, int key){
    record *node_record = find(root, key, 0);

    if(node_record == NULL)
        return NULL;
    //printf("Found value: %d\n", node_record->value);

    return &node_record -> value;
}
