#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "msgq.h"
#include "ght.h"
#include "BPT/bpt.h"

//Setaffinify is linux specific, so to run on other os comment out this line
#define LINUX

#define DATASTRUCT 1

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

/* Function for running subtree, takes in a subtree struct to hold queue and possible other info*/
void* subTreeFunc(void* arg){
    int *i;
    subtree_t * subtree = (subtree_t*) arg;
    printf("Starting subtree number: %d \n", subtree->threadnum);
    #ifdef LINUX
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(subtree->threadnum, &cpuset); 
        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    #endif

    node* root = NULL; 
    // used here to set cpu core to run on
    while(1){

        // Read operation from queue here, this should later be invoked by db_get
        operation_t o = queue_read(subtree->msgq);

         //Print operation read from queue 
        //printf("Operation on Key: %d and Value %d should run on cpu %d \n", o.key, o.value,  subtree->threadnum);

        if(o.type == OP_ADD){
            //Insert data into Bplustree
            switch (DATASTRUCT){
                case 0:
                    root = insert(root, o.key, o.value);
                case 1:

            }
            
        }else if(o.type == OP_READ){
            //Get data from Bplustree
            i = find(root, o.key, 0);
            operation_t res;
            res.key = o.key;
            res.value = *i;
            queue_add(subtree->resq, res);
            //printf("Found value %d at Key %d \n", *i, o.key);
        }else{
            printf("!!! TypeERROR!!!\n\n");
        }

       

        //Sleep for a set interval before trying to read another operation
        //int sleeptime = rand() % 10; 
        //sleep(sleeptime);
    }
}

/* Function to set up a new key-value store of GreenHashTree */
db_t *db_new()
{
    //Seed random at beginning of store
    srand(time(NULL));

    //Allocate memory for db-struct
    db_t* db = malloc(sizeof(db_t));
    int i;

    //Get number of LOGICAL cpu cores from sysconf
    int numofcpus = sysconf(_SC_NPROCESSORS_ONLN);
    //int numofcpus = 2;
    db->numthreads = numofcpus;
    printf("Number of logical CPU's: %d\n", numofcpus);

    //Allocate space for subtrees 
    db->subtreelist = malloc(sizeof(subtree_t)*numofcpus);

    //Start a subtree for each cpu
    for(i = 0; i < numofcpus; i++){

        //Create subtree data struct
        subtree_t *subtreeStruct = malloc(sizeof(subtree_t));
        db->subtreelist[i] = subtreeStruct;

        //Initialize message queue for this subtree
        subtreeStruct->msgq = queue_init(); 
        subtreeStruct->resq = queue_init();
        subtreeStruct->threadnum = i;
        
        //Start a new thread for this subtree
        pthread_t threadTree;
        pthread_create(&threadTree, NULL, subTreeFunc, subtreeStruct);
    }

    printf("Finished set up of GreenHashTree!\n");
    return db;
}

/* Function for putting data into GreenHashTree */
int db_put(db_t *db_data, int key, int val) {

    //Hash key to determine which cpu should hold this data
    unsigned int cpunumber = hash(key) % db_data->numthreads; // Put num cpus here
    //printf("This data should be put on cpu nr %d \n", cpunumber);

    //Create an operation struct to hold the key-value pair
    operation_t o;
    o.key = key;
    o.value = val;
    o.type = OP_ADD;

    //Send operation to message queue on the desired subtree
    queue_add(db_data->subtreelist[(int)cpunumber]->msgq, o);

    return 0;
}

int db_get(db_t *db_data, int key) {
    
    //Hash key to determine which cpu should hold this data
    unsigned int cpunumber = hash(key) % db_data->numthreads; // Put num cpus here
    //printf("This data should be put on cpu nr %d \n", cpunumber);

    //Create an operation struct to hold the key-value pair
    operation_t o;
    o.key = key;
    o.value = NULL;
    o.type = OP_READ;

    //Send operation to message queue on the desired subtree
    queue_add(db_data->subtreelist[(int)cpunumber]->msgq, o);

    operation_t res;

    res = queue_read(db_data->subtreelist[(int)cpunumber]->resq);

    return res.value;
}

int db_free(db_t *db_data) {
    return 0 ;
}

int main (int argc, char **argv) 
{
    //Test BPT
    /*
    node* root = NULL;
    root = insert(root, 5, 10);

    int *r = find(root, 5,0);
    printf("Got value %d \n", *r);
    sleep(10000);
    */


    //Main used for testing only
    int i, result;
    //Initialize DB
    db_t *db = db_new();

    //Used to count uniformity of hash on different cpus
    int zerocount = 0;
    int onecount = 0;
    int twocount = 0;
    int threecount = 0;
    
    srand(time(NULL));

    //Send 10 operations to key-value store
    for(i=0; i < 10; i++){

        //Get random int to use as key
        int rkey = rand()%100000;

        //Determine what cpu it should be used on, only for counting
        //This is recomputed in db_put
        int numofcpus = sysconf(_SC_NPROCESSORS_ONLN);
        unsigned int cpunumber = hash(rkey) % numofcpus; 

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

        // Put asdf as value of all key-value pairs
        //char value[] = "asdf";
        int value = rand() % 1000;

        //Use sprintf to transform random key into string
        //char key[20];
        //sprintf(key,"%d",rkey);

        //Call on db_put to place value into store
        printf("Put Value %d into storeon key: %d using CPU number %d\n", value, rkey, cpunumber);
        db_put(db, rkey, value);
                //dleep for random time between 0-10 seconds before adding new value
        //int sleeptime = rand() % 10; 
        //sleep(sleeptime);

        //printf("Get same value from store..\n");
        result = db_get(db, rkey);
        printf("Value was %d \n", result);


    }

    printf("Number of operations on cpu 0: %d, 1: %d, 2: %d, 3: %d \n", zerocount, onecount, twocount, threecount);

    //Main should not finish before all values is retrieved by worker threads
    sleep(1000);

}