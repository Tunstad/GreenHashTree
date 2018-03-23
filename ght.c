#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <msgq.h>
#include <ght.h>


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
void subTreeFunc(subtree_t *subtree){
    printf("Starting subtree number: %d \n", subtree->threadnum);
    //pthread_setaffinity_np used here to set cpu core to run on
    while(1){

        // Read operation from queue here, this should later be invoked by db_get
        operation_t o = queue_read(subtree->msgq); 

        //Print operation read from queue 
        printf("Got operation Key: %s and Value %s \n", o.key, o.value);

        //Sleep for a set interval before trying to read another operation
        int sleeptime = rand() % 10; 
        sleep(sleeptime);
    }
    return;
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
        subtreeStruct->threadnum = i;
        
        //Start a new thread for this subtree
        pthread_t threadTree;
        pthread_create(&threadTree, NULL, subTreeFunc, subtreeStruct);
    }

    printf("Finished set up of GreenHashTree!\n");
    return db;
}

/* Function for putting data into GreenHashTree */
int db_put(db_t *db_data, char *key, char *val) {

    //Hash key to determine which cpu should hold this data
    unsigned int cpunumber = hash(atoi(key)) % db_data->numthreads; // Put num cpus here
    //printf("This data should be put on cpu nr %d \n", cpunumber);

    //Create an operation struct to hold the key-value pair
    operation_t o;
    o.key = key;
    o.value = val;

    //Send operation to message queue on the desired subtree
    queue_add(db_data->subtreelist[(int)cpunumber]->msgq, o);

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

    //Main used for testing only
    int i;
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
        int rkey = rand();

        //Determine what cpu it should be used on, only for counting
        //This is recomputed in db_put
        unsigned int cpunumber = hash(rkey) % 4; 

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
        char value[] = "asdf";

        //Use sprintf to transform random key into string
        char key[20];
        sprintf(key,"%d",rkey);

        //Call on db_put to place value into store
        printf("Put Value into store: %s on CPU number %d\n", key, cpunumber);
        db_put(db, key, value);

        //Sleep for random time between 0-10 seconds before adding new value
        //int sleeptime = rand() % 10; 
        //sleep(sleeptime);
    }

    printf("Number of operations on cpu 0: %d, 1: %d, 2: %d, 3: %d \n", zerocount, onecount, twocount, threecount);

    //Main should not finish before all values is retrieved by worker threads
    sleep(1000);

}