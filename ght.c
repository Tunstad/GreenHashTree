#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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

/* START POET & HEARTBEAT */

// HB Interval (in useconds)
#define HB_INTERVAL 100000 //10000 by default
int stop_heartbeat = 0;
pthread_t hb_thread_handler;


#define HB_ENERGY_IMPL
#include <heartbeats/hb-energy.h>
#include <heartbeats/heartbeat-accuracy-power.h>
#include <poet/poet.h>
#include <poet/poet_config.h>

#define PREFIX "GHT"

//#define USE_POET // Power and performance control
//#define USE_HB

heartbeat_t* heart;
poet_state* state;
static poet_control_state_t* control_states;
static poet_cpu_state_t* cpu_states;
unsigned int num_runs = 1000;
int hbcount = 0;

void *heartbeat_timer_thread(){

    int i = 0;
    while(!stop_heartbeat){

        heartbeat_acc(heart, i, 1);
#ifdef USE_POET
        poet_apply_control(state);
#endif
        i++;
        usleep(HB_INTERVAL);

    }
}

void hb_poet_init() {
    float min_heartrate;
    float max_heartrate;
    int window_size;
    double power_target;
    unsigned int nstates;

    if(getenv(PREFIX"_MIN_HEART_RATE") == NULL) {
        min_heartrate = 0.0;
    }
    else {
        min_heartrate = atof(getenv(PREFIX"_MIN_HEART_RATE"));
    }
    if(getenv(PREFIX"_MAX_HEART_RATE") == NULL) {
        max_heartrate = 100.0;
    }
    else {
        max_heartrate = atof(getenv(PREFIX"_MAX_HEART_RATE"));
    }
    if(getenv(PREFIX"_WINDOW_SIZE") == NULL) {
        window_size = 30;
    }
    else {
        window_size = atoi(getenv(PREFIX"_WINDOW_SIZE"));
    }
    if(getenv(PREFIX"_POWER_TARGET") == NULL) {
        power_target = 70;
    }
    else {
        power_target = atof(getenv(PREFIX"_POWER_TARGET"));
    }

    if(getenv("HEARTBEAT_ENABLED_DIR") == NULL) {
        fprintf(stderr, "ERROR: need to define environment variable HEARTBEAT_ENABLED_DIR (see README)\n");
        exit(1);
    }

    printf("init heartbeat with %f %f %d\n", min_heartrate, max_heartrate, window_size);

    heart = heartbeat_acc_pow_init(window_size, 100, "heartbeat.log",
                                   min_heartrate, max_heartrate,
                                   0, 100,
                                   1, hb_energy_impl_alloc(), power_target, power_target);
    if (heart == NULL) {
        fprintf(stderr, "Failed to init heartbeat.\n");
        exit(1);
    }
#ifdef USE_POET
    if (get_control_states("benchmark/config/control_config", &control_states, &nstates)) {
        fprintf(stderr, "Failed to load control states.\n");
        exit(1);
    }
    if (get_cpu_states("benchmark/config/cpu_config", &cpu_states, &nstates)) {
        fprintf(stderr, "Failed to load cpu states.\n");
        exit(1);
    }
    state = poet_init(heart, nstates, control_states, cpu_states, &apply_cpu_config, &get_current_cpu_state, 1, "poet.log");
    if (state == NULL) {
        fprintf(stderr, "Failed to init poet.\n");
        exit(1);
    }
#endif
    printf("heartbeat init'd\n");

}

void hb_poet_finish() {
#ifdef USE_POET
    poet_destroy(state);
    free(control_states);
    free(cpu_states);
#endif
    heartbeat_finish(heart);
    printf("heartbeat finished\n");
}
/* END POET AND HEARTBEAT */

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
    int* i;
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

        // Read operation from queue here, this should later be invoked by db_get
        operation_t o = queue_read(subtree->msgq, INT32_MAX);

         //Print operation read from queue 
        //printf("Operation on Key: %d and Value %d should run on cpu %d \n", o.key, o.value,  subtree->threadnum);

        if(o.type == OP_ADD){
            //Insert data into Bplustree
            root = insert_into_tree(root, o.key, o.value);
            *o.retval = 2;

        }else if(o.type == OP_READ){
            //Get data from Bplustree
            i = search_tree(root, o.key);

            if(i == NULL){
                *o.retval = 1;
            }else{
                *o.retval = 2;
            }
            
        }else{
            printf("!!! TypeERROR!!! type: %d \n\n", o.type);
        }

    }
}

/* Function to set up a new key-value store of GreenHashTree */
db_t *db_new()
{
#ifdef USE_HB
    /* init runtime control (e.g., POET) */
    hb_poet_init();


    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    int rc = pthread_create(&hb_thread_handler, &attr, heartbeat_timer_thread, NULL);
    if (rc) {
        perror("failed: HB thread create\n");
        exit(-1);
    }
#endif
    //Seed random at beginning of store
    srand(time(NULL));

    //Allocate memory for db-struct
    db_t* db = malloc(sizeof(db_t));
    int i;

    db->intval = malloc(sizeof(int));
    *db->intval = 3579;

    //Get number of LOGICAL cpu cores from sysconf, use only the first half for subtrees
    //other half will be used for benchmarking.
    int numofcpus = sysconf(_SC_NPROCESSORS_ONLN)/2;
    db->numthreads = numofcpus;
    printf("Number of logical CPU's: %d\n", numofcpus);

    //Allocate space for subtrees 
    db->subtreelist = malloc(sizeof(subtree_t*)*numofcpus);

    int counter = 1;
    //Start a subtree for each cpu
    for(i = 0; i < numofcpus; i++){

        
        //Create subtree data struct
        subtree_t *subtreeStruct = malloc(sizeof(subtree_t));
        db->subtreelist[i] = subtreeStruct;

        //Initialize message queue for this subtree
        subtreeStruct->msgq = queue_init(); 
        subtreeStruct->resq = queue_init();
        subtreeStruct->threadnum = counter;
        
        //Start a new thread for this subtree
        pthread_t threadTree;
        pthread_create(&threadTree, NULL, subTreeFunc, subtreeStruct);
        counter += 2;
    }

    printf("Finished set up of GreenHashTree!\n");
    return db;
}

/* Function for putting data into GreenHashTree */
int* db_put(db_t *db_data, int key, int val) {

    //Hash key to determine which cpu should hold this data
    unsigned int cpunumber = hash(key) % db_data->numthreads; // Put num cpus here

    //Create an operation struct to hold the key-value pair
    operation_t o;
    o.key = key;
    o.value = val;
    o.type = OP_ADD;
    o.retval = malloc(sizeof(int));
    *o.retval = 0;

    //Send operation to message queue on the desired subtree
    queue_add(db_data->subtreelist[(int)cpunumber]->msgq, o);

    while(*o.retval == 0){
        sleep(0);
    }

    return db_data->intval;
}

int* db_get(db_t *db_data, int key) {
    
    //Hash key to determine which cpu should hold this data
    unsigned int cpunumber = hash(key) % db_data->numthreads; // Put num cpus here

    //Create an operation struct to hold the key-value pair
    operation_t o;
    o.key = key;
    o.value = 0;
    o.type = OP_READ;
    o.retval = malloc(sizeof(int));
    *o.retval = 0;
    struct timespec timestruct;
    timestruct.tv_sec = 0;
    timestruct.tv_nsec = 1;


    //Send operation to message queue on the desired subtree
    queue_add(db_data->subtreelist[(int)cpunumber]->msgq, o);

    while(*o.retval == 0){
        sleep(0);
    }

    if(*o.retval == 1){
        return NULL;
    }else{
        return db_data->intval;
    }

}

int db_free(db_t *db_data) {
    #ifdef USE_HB

    stop_heartbeat = 1;
 
    int rc = pthread_join(hb_thread_handler, NULL);
    if (rc) {
        printf("IN GHT\n");
        perror("error, pthread_join\n");
        exit(-1);
    }
    hb_poet_finish();
    #endif

    return 0 ;
}
