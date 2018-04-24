#ifndef __vebmiddleware_H__
#define __vebmiddleware_H__

void *initialize_tree();
void *insert_into_tree(void *tree, int key, int val);
int search_tree(void *tree, int key);

#endif