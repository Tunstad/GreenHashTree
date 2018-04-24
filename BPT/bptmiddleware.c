#include <bpt.h>

void *initialize_tree(){
    void *root = insert(node, 0, 0);

    return root;
}

void *insert_into_tree(void *root, int key, int value){
    void *root = insert(root, key, value);

    return root;
}

int search_tree(void *root, int key){
    void *node_record = find(root, key, 0);

    return node_record -> value;
}
