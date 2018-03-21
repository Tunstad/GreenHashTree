#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "ght.h"

typedef struct{
    void *key;
    void *value;
} operation_t;

operation_t OperationsArray[100];

// Example Hash Function https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
unsigned int hash(unsigned int x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}
unsigned int unhash(unsigned int x) {
    x = ((x >> 16) ^ x) * 0x119de1f3;
    x = ((x >> 16) ^ x) * 0x119de1f3;
    x = (x >> 16) ^ x;
    return x;
}

void *subTree(){
    while(1){
        printf("iam groot\n");
        sleep(10);
    }
}

db_t *db_new()
{
    db_t* db = malloc(sizeof(db_t));
    int i;
    int numofcpus = sysconf(_SC_NPROCESSORS_ONLN);
    db->numthreads = numofcpus;
    printf("Number of logical CPU's: %d\n", numofcpus);
    for(i = 0; i < numofcpus; i++){
        pthread_t threadTree;
        pthread_create(&threadTree, NULL, subTree, NULL);
    }

    printf("Running GreenHashTree.. please wait...\n");
    return db;
}


int db_put(db_t *db_data, char *key, char *val) {
    return 0;
}

char* db_get(db_t *db_data, char *key) {
    return 0;
}

int db_free(db_t *db_data) {
    return 0 ;
}

int main (int argc, char **argv) 
{
    int i;
    //Initialize DB
    db_t *db = db_new();

    for(i=0; i < 5; i++){
        char value[] = "asdf";
        char key[10];
        sprintf(key,"%d",i);
        db_put(db, key, value);
    }
    sleep(1000);

}