#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "msgq.h"

/// An Energy efficient message queue of operations  ///

//Initialize message queue
queue_t* queue_init(){
    //Allocate queue struct
    queue_t *queue = malloc(sizeof(queue_t));

    //Initializers not used?
    //queue.condvar = PTHREAD_COND_INITIALIZER;
    //queue.mutex = PTHREAD_MUTEX_INITIALIZER;

    //Message queue should be of set length
    queue->lenght = 1000;

    //Allocate array of length operations
    queue->array = malloc(sizeof(operation_t)*queue->lenght);

    //Current and lastadded start at 0
    queue->current = 0;
    queue->lastadded = 0;

    return queue;
}

//Add an operation to queue
void queue_add(queue_t *queue, operation_t operation){

    //Lock before editing queue
    pthread_mutex_lock(&queue->mutex);

    //Add operation to queue on lastadded position
    queue->array[queue->lastadded] = operation;
    //Increment lastadded for next item
    queue->lastadded += 1;


    //Unblock condition variable when new data has arrived
    pthread_cond_signal(&queue->condvar);
    //Unlock mutex after?
    pthread_mutex_unlock(&queue->mutex);

}

//Read data from queue
operation_t queue_read(queue_t *queue){

    //Lock queue before reading from it 
    pthread_mutex_lock(&queue->mutex);

    //If current is at lastadded, wait for new data to be added
    if(queue->current == queue->lastadded){
        pthread_cond_wait(&queue->condvar, &queue->mutex);
    }

    //Get next data from queue
    operation_t data = queue->array[queue->current];

    //increment current variable
    queue->current += 1;

    //Unlock queue and return data
    pthread_mutex_unlock(&queue->mutex);
    return data;
}


int main2 (void) {
    // Used only to test queue(can be deleted if causing errors)
    int i;
    queue_t *q = queue_init();

    
    for(i = 0; i < 5; i++){
        printf("Adding value %d\n", i);
        char key[]  = "keyasdf";
        char value[] = "Hello World";
        operation_t o;
        o.key = key;
        o.value = value;
        queue_add(q, o);
    }

    for(i = 0; i < 5; i++){
        printf("Reading value %d\n", i);
        operation_t ro = queue_read(q);
        printf("Read operation Key: %s Value: %s \n", ro.key, ro.value);
    }

    return 0;
}