#ifndef __MSGQ_H__
#define __MSGQ_H__
#include <pthread.h>

/* Operation to be held in queue of key and value*/
typedef struct{
    int key;
    int value;
    int type;
    int * retval;
} operation_t;

#define OP_ADD 0
#define OP_READ 1

/* Struct for a queue itself */
typedef struct{
operation_t* array;         //Array of operations in queue
int lenght;                 //Number of operations in queue
int current;                //Current location to get data from in queue
int lastadded;              //Position to add in queue, after last added
pthread_cond_t condvar;     //Condition variable of queue
pthread_mutex_t mutex;      //Queue mutex
}queue_t;

queue_t* queue_init();
void queue_add(queue_t *queue, operation_t operation);
operation_t queue_read(queue_t *queue, int key);

#endif