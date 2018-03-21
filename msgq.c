#include <stdio.h>
#include <stdlib.h>

/// An Energy efficient message queue of operations  ///
typedef struct{
    void *key;
    void *value;
} operation_t;

typedef struct{
operation_t* array;
int lenght;
int current;
int lastadded;
//Condition variable
}queue_t;


queue_t queue_init(){
    queue_t queue;
    queue.lenght = 1000;
    queue.array = malloc(sizeof(operation_t)*queue.lenght);
    queue.current = 0;
    queue.lastadded = 0;

    return queue;
}
void queue_add(queue_t *queue, operation_t operation){
    queue->array[queue->lastadded] = operation;
    queue->lastadded += 1;

    if(queue->lastadded >= queue->lenght){

    }
    //Unblock condition variable when new data has arrived


}
operation_t* queue_read(queue_t *queue){
    operation_t* data = &queue->array[queue->current];


    //If no more data, block on condition variable


}