#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "ght.h"



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
    //pthread_setaffinity_np
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

    unsigned int cpunumber = hash(atoi(key)) % 4; // Put num cpus here
    printf("This data should be put on cpu nr %d \n", cpunumber);
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

    //Testing
    int i;
    //Initialize DB
    db_t *db = db_new();

    int zerocount = 0;
    int onecount = 0;
    int twocount = 0;
    int threecount = 0;
    
    srand(time(NULL));
    for(i=0; i < 10; i++){

        
        int rkey = rand();
        unsigned int cpunumber = hash(rkey) % 4; // Put num cpus here

        switch(cpunumber) {
            case 0:
                zerocount +=1;
                break;
            case 1:
                onecount +=1;
                break;
            case 2:
                twocount +=1;
                break;
            case 3:
                threecount +=1;
                break;
            
        }
        printf("Key of number %d should be put on cpu: %d \n", rkey, cpunumber);

        char value[] = "asdf";
        char key[20];
        sprintf(key,"%d",rkey);
        db_put(db, key, value);
    }

    printf("Number of 0: %d 1: %d 2: %d 3: %d \n", zerocount, onecount, twocount, threecount);
    sleep(1000);

}