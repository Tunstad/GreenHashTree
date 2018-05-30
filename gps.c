#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "msgq.h"
#include "gps.h"
#include "BPT/bpt.h"
#include "BPT/bptmiddleware.h"
#include "SVEB/vebmiddleware.h"

//Setaffinify is linux specific, so to run on other os comment out this line
//but then you will not have advantage of running on specific cores.
#define LINUX

// Power and performance control
//#define USE_POET 
// Used without poet to measure energy usage
//#define USE_HB   

/* START POET & HEARTBEAT */
// HB Interval (in useconds)
#define HB_INTERVAL 100000 //10000 by default, set to 100000 to measure every 0.1 seconds
int stop_heartbeat = 0;
pthread_t hb_thread_handler;

#define HB_ENERGY_IMPL
#include <heartbeats/hb-energy.h>
#include <heartbeats/heartbeat-accuracy-power.h>
#include <poet/poet.h>
#include <poet/poet_config.h>

#define PREFIX "gps"

heartbeat_t* heart;
poet_state* state;
static poet_control_state_t* control_states;
static poet_cpu_state_t* cpu_states;
int heartbeatcount = 0;

//Timer thread will tick a heartbeat every and apply poet control if enabled
//This is only used for energy measurements, while for poet heatbeat should tick every job
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
//Initialize hb/poet with dafault values for max/min heartrate, window size and desired power target
void hb_poet_init() {
    float min_heartrate;
    float max_heartrate;
    int window_size;
    double power_target;
    unsigned int nstates;

    if(getenv(PREFIX"_MIN_HEART_RATE") == NULL) {
        min_heartrate = 120000;
    }
    else {
        min_heartrate = atof(getenv(PREFIX"_MIN_HEART_RATE"));
    }
    if(getenv(PREFIX"_MAX_HEART_RATE") == NULL) {
        max_heartrate = min_heartrate;
    }
    else {
        max_heartrate = atof(getenv(PREFIX"_MAX_HEART_RATE"));
    }
    if(getenv(PREFIX"_WINDOW_SIZE") == NULL) {
        window_size = 1000;
    }
    else {
        window_size = atoi(getenv(PREFIX"_WINDOW_SIZE"));
    }
    if(getenv(PREFIX"_POWER_TARGET") == NULL) {
        power_target = 150;
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
//Called by db_free to finish HB/poet
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

// Hash Function used, proven to provide good uniform distrbution, see report for source.
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

/* Function for running subtree, takes in a subtree struct to hold queue and other specific info */
void* subStructFunc(void* arg){
    substruct_t * subtree = (substruct_t*) arg;
    int* i;

    printf("Starting subtree %d.. \n", subtree->cpunum);

    //Set subtree to run on a specific logical cpu as provided by cpunum in subtreestruct
    #ifdef LINUX
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(subtree->cpunum, &cpuset); 
        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    #endif

    //Initialze struct, root pointer only required for B+ tree, empty for others
    node* root = init_struct(); 

    //Run contionus loop reading operations from queue and performing them
    while(1){

        // Read operation from queue, otherwise block on conditionvar waiting
        operation_t o = queue_read(subtree->msgq);


        if(o.type == OP_ADD){
            //Insert data into substrucutre
            root = insert_into_struct(root, o.key, o.value);

            //Insert has been completed, set value to returnpointer to indicate it was completed
            *o.retval = 2;

        }else if(o.type == OP_READ){
            //Search the substruture for operation key
            i = search_struct(root, o.key);

            //We do not care about the actual value, only whether it was found or not
            //The data should be read within the structs with a strcpy, if NULL returned the data was not found
            if(i == NULL){
                //Set value to 1 to indicate search was completed but result was not found
                *o.retval = 1;
            }else{
                //Set value to 2 to indicate search was completed and result was found
                *o.retval = 2;
            }
            
        }else{
            printf("TypeError in Subtree: %d \n", o.type);
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
    //Seed random at beginning of store, TODO: deprecated?
    srand(time(NULL));

    //Allocate memory for db-struct
    db_t* db = malloc(sizeof(db_t));
    int i;

    //Static int to return instead of actual values
    db->intval = malloc(sizeof(int));
    *db->intval = INT32_MAX;

    //Get number of LOGICAL cpu cores from sysconf
    //Only one NUMA node(or uneven numbered logical cores) will be used for structures
    //The other NUMA node(even numbered logical cores) will be used for client threads
    int numofcpus = sysconf(_SC_NPROCESSORS_ONLN)/2;
    db->numthreads = numofcpus;
    printf("Number of logical CPU's for substructures's: %d\n", numofcpus);

    //Allocate space for substructures 
    db->substructlist = malloc(sizeof(substruct_t*)*numofcpus);

    //Start with cpu1 and increment by 2 so cores 1,3,5,7.. will be used for strucutres
    int cpunum = 1;
    //Start a subtree for each logical core dedicated to substructures
    for(i = 0; i < numofcpus; i++){

        //Create substructures metadata
        substruct_t *subtreeStruct = malloc(sizeof(substruct_t));
        db->substructlist[i] = subtreeStruct;
        subtreeStruct->cpunum = cpunum;

        //Initialize message queue for this substructure, resq was previously used to return values
        subtreeStruct->msgq = queue_init(); 
        //subtreeStruct->resq = queue_init();
        
        //Start a new thread for this subtree, will be pinned to the set cpunum
        pthread_t threadTree;
        pthread_create(&threadTree, NULL, subStructFunc, subtreeStruct);
        cpunum += 2;
    }

    printf("Finished setting up GreenHashTree!\n");
    return db;
}

/* Function for putting data into structure */
int* db_put(db_t *db_data, int key, int val) {

    //Hash key to determine which substructure should be responsible for this data
    unsigned int cpunumber = hash(key) % db_data->numthreads;

    //Operation struct hold the KV-pair and returnvalue
    operation_t o;
    o.key = key;
    o.value = val;
    o.type = OP_ADD;
    //Allocate memory for return value, more efficient than return queue
    o.retval = malloc(sizeof(int));
    *o.retval = 0;

    //Send operation to message queue on the desired subtree
    queue_add(db_data->substructlist[(int)cpunumber]->msgq, o);

    //While return value has not been set, sleep to yield control
    while(*o.retval == 0){
        sleep(0);
    }

#ifdef USE_POET
    //Count job for POET
    heartbeatcount += 1;
    heartbeat_acc(heart, heartbeatcount, 1);
    poet_apply_control(state);
#endif
    //Return value has been set to indicate the put was completed
    return db_data->intval;
}

/* Function for getting data from structure */
int* db_get(db_t *db_data, int key) {
    
    //Hash key to determine which substructure should be responsible for this data
    unsigned int cpunumber = hash(key) % db_data->numthreads;

    //Operation struct hold the KV-pair and returnvalue
    operation_t o;
    o.key = key;
    o.value = 0;
    o.type = OP_READ;
    //Allocate memory for return value, more efficient than return queue
    o.retval = malloc(sizeof(int));
    *o.retval = 0;

    //Send operation to message queue on the desired subtree
    queue_add(db_data->substructlist[(int)cpunumber]->msgq, o);

    //While return value has not been set, sleep to yield control
    while(*o.retval == 0){
        sleep(0);
    }

       
#ifdef USE_POET
    //Count job for POET 
    heartbeatcount += 1;
    heartbeat_acc(heart, heartbeatcount, 1);
    poet_apply_control(state);
#endif
    //If return value is 1 the value was not found so we return NULL,
    //If return value is 2 return static intval to indicate it was found
    if(*o.retval == 1){
        return NULL;
    }else if(*o.retval == 2){
        return db_data->intval;
    }else{
        printf("Something went wrong when getting key: %d \n", key);
    }
}

/* Function to shut down db */
int db_free(db_t *db_data) {
    //Stop heartbeat (measurements will be written to heartbeat.log)
    #ifdef USE_HB
    stop_heartbeat = 1;
    int rc = pthread_join(hb_thread_handler, NULL);
    if (rc) {
        perror("error, pthread_join\n");
        exit(-1);
    }
    hb_poet_finish();
    #endif
    return 0 ;
}
