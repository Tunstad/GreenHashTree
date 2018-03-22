#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <msgq.h>

queue_t queue_init(){
    queue_t queue;
    //queue.condvar = PTHREAD_COND_INITIALIZER;
    //queue.mutex = PTHREAD_MUTEX_INITIALIZER;
    queue.lenght = 1000;
    queue.array = malloc(sizeof(operation_t)*queue.lenght);
    queue.current = 0;
    queue.lastadded = 0;

    return queue;
}
void queue_add(queue_t *queue, operation_t operation){

    pthread_mutex_lock(&queue->mutex);
    queue->array[queue->lastadded] = operation;
    queue->lastadded += 1;


    //Unblock condition variable when new data has arrived
    pthread_cond_signal(&queue->condvar);
    pthread_mutex_unlock(&queue->mutex);

}
operation_t queue_read(queue_t *queue){
    pthread_mutex_lock(&queue->mutex);
    if(queue->current >= queue->lastadded){
        pthread_cond_wait(&queue->condvar, &queue->mutex);
    }
    operation_t data = queue->array[queue->current];
    queue->current += 1;
    pthread_mutex_unlock(&queue->mutex);

    return data;
}


int main2 (void) {
    // Test message queue
    int i;
    queue_t q = queue_init();

    
    for(i = 0; i < 5; i++){
        printf("Adding value %d\n", i);
        int key  = i;
        char value[] = "Hello World";
        operation_t o;
        o.key = key;
        o.value = value;
        queue_add(&q, o);
    }

    for(i = 0; i < 5; i++){
        printf("Reading value %d\n", i);
        operation_t ro = queue_read(&q);
        printf("Read operation Key: %d Value: %s \n", ro.key, ro.value);
    }

    return 0;
}