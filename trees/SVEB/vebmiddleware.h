#ifndef __vebmiddleware_H__
#define __vebmiddleware_H__

void *init();
void insert(void *tree, int key, int val);
int search(void *tree, int key);

#endif