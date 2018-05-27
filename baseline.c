#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "BPT/bpt.h"
#include "BPT/bptmiddleware.h"
#include "SVEB/vebmiddleware.h"
#include "baseline.h"

// Power and performance control
//#define USE_POET 
// Used without poet to measure energy usage
#define USE_HB  
 
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



/* Function to set up a new key-value store with no partitioning */
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
    //Allocate memory for db-struct
    db_t* db = malloc(sizeof(db_t));
    //Initialze a single data strucutre for db
    db->root = init_struct();
     //Static int to return instead of actual values
    db->intval = malloc(sizeof(int));
    *db->intval = 3579;
    return db;
}

/* Function for putting data into structure */
int* db_put(db_t *db_data, int key, int val) {

    //Put data into structure
    db_data->root = insert_into_struct(db_data->root, key, val);
    

#ifdef USE_POET
    //Count job for POET
    heartbeatcount += 1;
    heartbeat_acc(heart, heartbeatcount, 1);
    poet_apply_control(state);
#endif

    return db_data->intval;
}

/* Function for getting data from structure */
int* db_get(db_t *db_data, int key) {
    int * i;

    //Search strucutre for data with key
    i = search_struct(db_data->root, key);

#ifdef USE_POET
    //Count job for POET
    heartbeatcount += 1;
    heartbeat_acc(heart, heartbeatcount, 1);
    poet_apply_control(state);
#endif

    //If struct returned NULL the data was not found, benchmark should count MISS with NULL
    if(i == NULL){
        return NULL;
    }else{
        //If data was found return static value to indicate it was found
        return db_data->intval;
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
