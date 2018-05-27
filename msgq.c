#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include "msgq.h"

/// Queue of operations for communicating with substructure threads  ///

//Initialize message queue
queue_t* queue_init(){
    //Allocate queue struct
    queue_t *queue = malloc(sizeof(queue_t));

    //Init synchronization primitives
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->condvar, NULL);

    //Message queue should be of set length and bound when full
    queue->lenght = 10000;

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
    queue->array[queue->lastadded % queue->lenght] = operation;

    //Increment lastadded for next item
    queue->lastadded += 1;

    //Unblock condition variable when new data has arrived
    pthread_cond_broadcast(&queue->condvar);
    pthread_mutex_unlock(&queue->mutex);

}

//Read data from queue
operation_t queue_read(queue_t *queue){

    //Lock queue before reading from it 
    pthread_mutex_lock(&queue->mutex);

    //Block on condition variable if queue is empty
    while(queue->current == queue->lastadded){
        pthread_cond_wait(&queue->condvar, &queue->mutex); 
    }

    //Read operation from queue and move one slot up
    operation_t data = queue->array[queue->current % queue->lenght];
    queue->current += 1;

    //Unlock queue and return data
    pthread_mutex_unlock(&queue->mutex);
    return data;
}
