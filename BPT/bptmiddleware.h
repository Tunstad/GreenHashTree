#ifndef __btpmiddleware_H__
#define __btpmiddleware_H__

void *initialize_tree();
void *insert_into_tree(void *root, int key, int value);
int* search_tree(void *root, int key);

#endif