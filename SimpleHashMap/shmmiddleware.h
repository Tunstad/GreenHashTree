#ifndef __shmmiddleware_H__
#define __shmmiddleware_H__

void *init_struct();
void *insert_into_struct(void *root, int key, int value);
int* search_struct(void *root, int key);

#endif