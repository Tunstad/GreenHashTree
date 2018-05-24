#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "BPT/bpt.h"
#include "BPT/bptmiddleware.h"
#include "SVEB/vebmiddleware.h"
#include "baseline.h"


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
#define USE_HB

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
    db_t* db = malloc(sizeof(db_t));
    db->root = initialize_tree();
    db->intval = malloc(sizeof(int));
    *db->intval = 3579;
    return db;

}

/* Function for putting data into GreenHashTree */
int* db_put(db_t *db_data, int key, int val) {

    db_data->root = insert_into_tree(db_data->root, key, val);

    return db_data->intval;
}

int* db_get(db_t *db_data, int key) {
    
    int * i;
    i = search_tree(db_data->root, key);

    if(i == NULL){
        // MISS
        return NULL;
    }else{
        // HIT, value not important
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
