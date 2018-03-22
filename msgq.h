#ifndef __MSGQ_H__
#define __MSGQ_H__
#include <pthread.h>

/// An Energy efficient message queue of operations  ///
typedef struct{
    char* key;
    char* value;
} operation_t;

typedef struct{
operation_t* array;
int lenght;
int current;
int lastadded;
pthread_cond_t condvar;
pthread_mutex_t mutex;
}queue_t;

queue_t queue_init();
void queue_add(queue_t *queue, operation_t operation);
operation_t queue_read(queue_t *queue);

#endif