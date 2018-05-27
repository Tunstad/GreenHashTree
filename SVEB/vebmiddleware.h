#ifndef __vebmiddleware_H__
#define __vebmiddleware_H__

void *init_struct();
void *insert_into_struct(void *tree, int key, int val);
int *search_struct(void *tree, int key);

#endif