#define _GNU_SOURCE
#include <getopt.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sched.h>
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <inttypes.h>
#include <signal.h>

#include "../gps.h"

#include "bench_common.h"

pthread_mutex_t printmutex;

typedef struct {
  size_t tid;
  query *queries;
  size_t num_ops;
  size_t num_puts;
  size_t num_gets;
  size_t num_miss;
  size_t num_hits;
  double tput;
  double time;
  int cpunumber;
} thread_param;

/* default parameter settings */
static size_t key_len;
static size_t val_len;
static size_t num_queries;
static size_t num_threads = 1;
static size_t num_mget = 1;
static float duration = 10.0;
static char* inputfile = NULL;

/* db structure is global */
db_t *db_data;

/* using sigalarm for timer */
volatile int stop = 0;

void trigger(int sig)
{
    stop = 1;
}


/* Calculate the second difference*/
static double timeval_diff(struct timeval *start, 
                           struct timeval *end)
{
  double r = end->tv_sec - start->tv_sec;

  /* Calculate the microsecond difference */
  if (end->tv_usec > start->tv_usec)
    r += (end->tv_usec - start->tv_usec)/1000000.0;
  else if (end->tv_usec < start->tv_usec)
    r -= (start->tv_usec - end->tv_usec)/1000000.0;
  return r;
}

/* init all queries from the ycsb trace file before issuing them */
static query *queries_init(char* filename)
{
  FILE *input;

  input = fopen(filename, "rb");
  if (input == NULL) {
    perror("can not open file");
    perror(filename);
    exit(1);
  }

  int n;
  n = fread(&key_len, sizeof(key_len), 1, input);
  if (n != 1)
    perror("fread error");
  n = fread(&val_len, sizeof(val_len), 1, input);
  if (n != 1)
    perror("fread error");
  n = fread(&num_queries, sizeof(num_queries), 1, input);
  if (n != 1)
    perror("fread error");

  printf("trace(%s):\n", filename);
  printf("\tkey_len = %zu\n", key_len);
  printf("\tval_len = %zu\n", val_len);
  printf("\tnum_queries = %zu\n", num_queries);
  printf("\n");

  query *queries = malloc(sizeof(query) * num_queries);
  if (queries == NULL) {
    perror("not enough memory to init queries\n");
    exit(-1);
  }

  size_t num_read;
  num_read = fread(queries, sizeof(query), num_queries, input);
  if (num_read < num_queries) {
    fprintf(stderr, "num_read: %zu\n", num_read);
    perror("can not read all queries\n");
    fclose(input);
    exit(-1);
  }

  fclose(input);
  printf("queries_init...done\n");
  return queries;
}

//Hashes the trace file querys key from char* to an int.
//This allows the use of existing pre-code datastructures supporting int operations.
//Will lead to some hash collisions and faulty key matching, but should be an insignificant amount for testing.
int bintointhash(char* data){
  unsigned long int hashval;
  int i = 0;

  while( hashval < UINT64_MAX && i < NKEY ) {
    hashval = hashval << 8;
    hashval += data[ i ];
    i++;
  }

  return hashval;
}

/* executing queries at each thread */
static void* queries_exec(void *param)
{
  thread_param* p = (thread_param*) param;

  //Client threads are divided and pinned to halv of the total logical CPUs of the system.
  //For a system with two sockets, all clients reside at a single socket and the data structures at the other.
  //This allows the data structures to be unaffected by clients work, prevents uneccesary cache invalidation and so on.
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(p->cpunumber, &cpuset); 
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

  /* get the key-value store structure */
  struct timeval tv_s, tv_e;

  pthread_mutex_lock (&printmutex);
  printf("start benching using thread %zu\n", p->tid);
  pthread_mutex_unlock (&printmutex);

  //Gets the queries assigned to this client.
  query* queries = p->queries;
  p->time = 0;

  //Structure to hold keys after hashing.
  int intqueries[num_queries/num_threads];

  //For each query assigned to this client.
  for(int j = 0; j < num_queries/num_threads; j++){

    //Convert char* key for each query to an int key to support underlying data structure.
    intqueries[j] = bintointhash(queries[j].hashed_key);
  }

  /* Strictly obey the timer */
  while (!stop) {
    gettimeofday(&tv_s, NULL);  // start timing

    //Iterate over operations.
    for (size_t i = 0 ; i < p->num_ops; i++) {
      enum query_types type = queries[i].type;
      int key = intqueries[i];

      //Static int is sent for val to put, data structure simulates write of int size.
      int buf = NVAL;

      //If operation PUT.
      if (type == query_put) {
        db_put(db_data, key, buf);
        p->num_puts++;

      //If operation GET.
      } else if (type == query_get) {
        int *val = db_get(db_data, key);
        p->num_gets++;

        //If key is not in data structure.
        if (val == NULL) {
          // cache miss, put something (garbage) in cache
          p->num_miss++;
          db_put(db_data, key, buf);

        //Key hit.
        } else {
          p->num_hits++;
        }
      } else {
        fprintf(stderr, "unknown query type\n");
      }

      /* Strictly obey the timer */
      if(stop) break;
    }
    gettimeofday(&tv_e, NULL);  // stop timing
    p->time += timeval_diff(&tv_s, &tv_e);
  }

  size_t nops = p->num_gets + p->num_puts;
  p->tput = nops / p->time;

  pthread_mutex_lock (&printmutex);
  printf("thread%zu gets %zu items in %.2f sec \n",
         p->tid, nops, p->time);
  printf("#put = %zu, #get = %zu\n", p->num_puts, p->num_gets);
  printf("#miss = %zu, #hits = %zu\n", p->num_miss, p->num_hits);
  printf("hitratio = %.4f\n",   (float) p->num_hits / p->num_gets);
  printf("tput = %.2f\n",  p->tput);
  printf("\n");
  pthread_mutex_unlock (&printmutex);

  printf("queries_exec...done\n");
  pthread_exit(NULL);
}

static void usage(char* binname)
{
  printf("%s [-t #] [-b #] [-l trace] [-d #] [-h]\n", binname);
  printf("\t-t #: number of working threads, by default %zu\n", num_threads);
  printf("\t-d #: duration of the test in seconds, by default %f\n", duration);
  printf("\t-l trace: e.g., /path/to/ycsbtrace, required\n");
  printf("\t-h  : show usage\n");
}


int
main(int argc, char **argv)
{
  if (argc <= 1) {
    usage(argv[0]);
    exit(-1);
  }

  int ch;
  while ((ch = getopt(argc, argv, "t:d:h:l:")) != -1) {
    switch (ch) {
    case 't': num_threads = atoi(optarg); break;
    case 'd': duration    = atof(optarg); break;
    case 'l': inputfile   = optarg; break;
    case 'h': usage(argv[0]); exit(0); break;
    default:
      usage(argv[0]);
      exit(-1);
    }
  }

  if (inputfile == NULL) {
    usage(argv[0]);
    exit(-1);
  }

  query *queries = queries_init(inputfile);

  pthread_t threads[num_threads];
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);;

  pthread_mutex_init(&printmutex, NULL);

  size_t t;
  thread_param tp[num_threads];

  //Initialize data structure.
  db_data = db_new();

  printf("\n\nFirst round of benchmark, ALL (%d) threads\n\n", num_threads);

  //Find number of logical CPUs available in the system. We want clients operating on halv the CPUs.
  //Other halv is used for the data structure.
  int numofcpus = sysconf(_SC_NPROCESSORS_ONLN);
  int benchthreads = numofcpus/2;
  int counter = 0;

  for (t = 0; t < num_threads; t++) {
    tp[t].queries = queries + t * (num_queries / num_threads);
    tp[t].tid     = t;
    tp[t].num_ops = num_queries / num_threads;
    tp[t].num_puts = tp[t].num_gets = tp[t].num_miss = tp[t].num_hits = 0;
    tp[t].time = tp[t].tput = 0.0;

    //Figure out which CPU the client should be pinned to and spawn client.
    tp[t].cpunumber = counter%numofcpus;
    counter +=2;
    int rc = pthread_create(&threads[t], &attr, queries_exec, (void *) &tp[t]);
    if (rc) {
      perror("failed: pthread_create\n");
      exit(-1);
    }
  }

  /* Start the timer */
  signal(SIGALRM, trigger);
  alarm(duration);

    //Only once
  result_t result;
  result.grand_total_time = 0.0;

  result.total_time = 0.0;;
  result.total_tput = 0.0;
  result.total_hits = 0;
  result.total_miss = 0;
  result.total_gets = 0;
  result.total_puts = 0;
  result.num_threads = num_threads;

  for (t = 0; t < num_threads; t++) {
    void *status;
    int rc = pthread_join(threads[t], &status);
    if (rc) {
      perror("error, pthread_join");
      exit(-1);
    }
    result.total_time = (result.total_time > tp[t].time) ? result.total_time : tp[t].time;
    result.total_tput += tp[t].tput;
    result.total_hits += tp[t].num_hits;
    result.total_miss += tp[t].num_miss;
    result.total_gets += tp[t].num_gets;
    result.total_puts += tp[t].num_puts;
  }

  result.grand_total_time += result.total_time;
  stop = 0;

  //Free the data structure.
  db_free(db_data);

  printf("total_time = %.2f\n", result.grand_total_time);
  printf("total_tput = %.2f\n", (float) (result.total_gets+result.total_puts)/result.grand_total_time);
  printf("total_hitratio = %.4f\n", (float) result.total_hits / result.total_gets);

  pthread_attr_destroy(&attr);
  printf("bye\n");
  return 0;
}
