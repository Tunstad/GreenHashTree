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
#include "BPT/bptmiddleware.h"
#include "SVEB/vebmiddleware.h"

//Setaffinify is linux specific, so to run on other os comment out this line
#define LINUX

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
    int i;
    subtree_t * subtree = (subtree_t*) arg;
    printf("Starting subtree number: %d \n", subtree->threadnum);
    #ifdef LINUX
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(subtree->threadnum, &cpuset); 
        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    #endif

    node* root = initialize_tree(); 

    // used here to set cpu core to run on
    while(1){

        //printf("Popping operation, thread: %d\n", subtree->threadnum);
        // Read operation from queue here, this should later be invoked by db_get
        operation_t o = queue_read(subtree->msgq);

         //Print operation read from queue 
        printf("Operation Type: %d on Key: %d and Value %d should run on cpu %d \n",o.type, o.key, o.value,  subtree->threadnum);

        if(o.type == OP_ADD){
            //printf("Inserting into tree, thread: %d\n", subtree->threadnum);
            //Insert data into Bplustree
            root = insert_into_tree(root, o.key, o.value);
        }else if(o.type == OP_READ){
            //Get data from Bplustree
            i = search_tree(root, o.key);
            //int k=3;
            //i=&k;
            //operation_t res;
            //res.key = o.key;
            //res.value =  i;

            //queue_add(subtree->resq, res);
            *o.retval = i;
            //printf("Found value %d at Key %d \n", *i, o.key);
        }else{
            printf("!!! TypeERROR!!!\n\n");
        }

       
        //printf("OPERATION DONE, thread: %d\n", subtree->threadnum);
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
    db->subtreelist = malloc(sizeof(subtree_t*)*numofcpus);

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
int db_put(db_t *db_data, int key, char* val) {

    /*
    unsigned int keyint = 0;
    //printf("Lenght of srting is %d \n", strlen(key));
    for(int i = 0; i < strlen(key); i++){
        keyint += (unsigned int)key[i];
    }
    //printf("New keyint is : %d \n", keyint);
    */
    int value = 32;
    
    //Hash key to determine which cpu should hold this data
    unsigned int cpunumber = hash(key) % db_data->numthreads; // Put num cpus here
    //printf("This data should be put on cpu nr %d \n", cpunumber);

    //Create an operation struct to hold the key-value pair
    operation_t o;
    o.key = key;
    o.value = value;
    o.type = OP_ADD;

    //Send operation to message queue on the desired subtree
    queue_add(db_data->subtreelist[(int)cpunumber]->msgq, o);

    return 0;
}

int *db_get(db_t *db_data, int key) {

    /*
    unsigned int keyint = 0;
    //printf("Lenght of srting is %d \n", strlen(key));
    for(int i = 0; i < strlen(key); i++){
        keyint += (unsigned int)key[i];
    }
    //printf("New keyint is : %d \n", keyint);
    */
    //Hash key to determine which cpu should hold this data
    unsigned int cpunumber = hash(key) % db_data->numthreads; // Put num cpus here
    //printf("This data should be put on cpu nr %d \n", cpunumber);

    //Create an operation struct to hold the key-value pair
    operation_t o;
    o.key = key;
    o.value = 0;
    o.type = OP_READ;
    int* returnlocationint = malloc(sizeof(int));
    
    o.retval = returnlocationint;
    *o.retval = -9573946;

    //Send operation to message queue on the desired subtree
    queue_add(db_data->subtreelist[(int)cpunumber]->msgq, o);

    operation_t res;

    //res = queue_read(db_data->subtreelist[(int)cpunumber]->resq);
    while(*returnlocationint == -9573946){
        sleep(0.001);
    }

    if(*returnlocationint == -6666666){
        return NULL;
    }

    printf("REturned value %d \n", *returnlocationint);
    return returnlocationint;
}

int db_free(db_t *db_data) {
    return 0 ;
}
/*
int main (int argc, char **argv) 
{
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

        //Get random string to use as key

        char *string = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-#'?!";
        size_t stringLen = strlen(string);
        int length = 16;
        char *randomString = NULL;
        randomString = malloc(sizeof(char) * (length +1));

        if (randomString) {
        short key = 0;

        for (int n = 0;n < length;n++) {            
            key = rand() % stringLen;          
            randomString[n] = string[key];
        }

        randomString[length] = '\0';
        printf("Generated random string %s \n", randomString);      
    }
    else {
        printf("No memory");
        exit(1);
    }


        int rkey = rand()%100000;

        //Determine what cpu it should be used on, only for counting
        //This is recomputed in db_put
        int numofcpus = sysconf(_SC_NPROCESSORS_ONLN);
        unsigned int cpunumber = hash(randomString) % numofcpus; 

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
        char value[] = "asdfvalue";
        //int value = rand() % 1000;

        //Use sprintf to transform random key into string
        //char key[20];
        //sprintf(key,"%d",rkey);

        //Call on db_put to place value into store
        printf("Put Value %s into storeon key: %d using CPU number %d\n", value, rkey, cpunumber);
        db_put(db, randomString, value);
                //dleep for random time between 0-10 seconds before adding new value
        //int sleeptime = rand() % 10; 
        //sleep(sleeptime);

        //printf("Get same value from store..\n");
        result = db_get(db, randomString);
        printf("Value was %s \n", result);


    }

    printf("Number of operations on cpu 0: %d, 1: %d, 2: %d, 3: %d \n", zerocount, onecount, twocount, threecount);

    //Main should not finish before all values is retrieved by worker threads
    sleep(1000);

}*/