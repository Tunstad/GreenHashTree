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

void subTreeFunc(subtree_t *subtree){
    printf("Subtree number: %d \n", subtree->threadnum);
    //pthread_setaffinity_np
    while(1){

        operation_t o = queue_read(&subtree->msgq);
        printf("Got operation Key: %s and Value %s \n", o.key, o.value);
        printf("iam groot\n");
        sleep(10);
    }
    return;
}

db_t *db_new()
{
    db_t* db = malloc(sizeof(db_t));
    int i;
    int numofcpus = sysconf(_SC_NPROCESSORS_ONLN);
    db->numthreads = numofcpus;
    db->threadlist = malloc(sizeof(subtree_t)*numofcpus);

    printf("Number of logical CPU's: %d\n", numofcpus);
    for(i = 0; i < numofcpus; i++){
        subtree_t *subtreeStruct = malloc(sizeof(subtree_t));
        
        db->threadlist[i] = subtreeStruct; 
        subtreeStruct->msgq = queue_init();
        subtreeStruct->threadnum = i;
        pthread_t threadTree;
        pthread_create(&threadTree, NULL, subTreeFunc, subtreeStruct);
    }

    printf("Running GreenHashTree.. please wait...\n");
    return db;
}


int db_put(db_t *db_data, char *key, char *val) {

    unsigned int cpunumber = hash(atoi(key)) % 4; // Put num cpus here
    printf("This data should be put on cpu nr %d \n", cpunumber);
    operation_t o;
    o.key = key;
    o.value = val;
    queue_add(&db_data->threadlist[cpunumber]->msgq, o);

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
        //printf("Key of number %d should be put on cpu: %d \n", rkey, cpunumber);

        char value[] = "asdf";
        char key[20];
        sprintf(key,"%d",rkey);
        printf("Key before send: %s\n", key);
        db_put(db, key, value);
    }

    printf("Number of 0: %d, 1: %d, 2: %d, 3: %d \n", zerocount, onecount, twocount, threecount);
    sleep(1000);

}