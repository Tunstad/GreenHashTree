# GreenPartitionStore #

## How to run benchmark ##
To run the partitoned key value store with the provided benchmark you first need to make one of the provided makefile-targets. The targets that exist are:
make bench_baseline_shm  - Baseline measurement for a single instance of the Hash Map
make bench_baseline_bpt  - Baseline measurement for a single instance of the B+ Tree
make bench_baseline_sveb - Baseline measurement for a single instance of the VEB-Tree
make bench_ght_shm   - Measurement for our solution of a partitioned Hash Map
make bench_ght_bpt   - Measurement for our solution of a partitioned B+ Tree
make bench_ght_sveb   - Measurement for our solution of a partitioned VEB-Tree

To run the benchmark you need to supply it with a dataset from the YSCB benchmark eg.
./bench_client -l /data/datasets/traces/kv10M_op1M_zipf.dat.a.run.trace
You can also specify additional variables, -t for number of client threads and -d for duration.
So to run with 12 client threads for 30 seconds you would do:
./bench_client -t 12 -d 30 -l /data/datasets/traces/kv10M_op1M_zipf.dat.a.run.trace

Our solution is based on partitioning across specific cores, for this we use pthreads_setaffinity which is based on LINUX-specific calls. So if you need to run on some other platform where this is not available you may need to uncomment the #define LINUX statement at line 17 in gps.c

## Running with Heartbeat and Poet ##
To run using Heartbeat for measuring only energy usage or to run with both heartbeat and poet enabled for actually using poet to enable DVFS, there are two statement in gps.c and baseline.c that define on compile time weather the code should run with heartbeat/poet or not. These can be found in the start of both files and are #define USE_HB and #define USE_POET

Poet requires access to the userspace cpu governor and may need some configuration to be supported.
1. Add "intel_pstate = disable" to your grub config
2. Reload and reboot grup, "sudo update-grub && reboot"
3. Check that userspace governor is available "cat /sys/devices/system/cpu/cpu0/cpufreq/
scaling_available_governors", if it's not you may have to do "modprobe
cpufreq_userspace"
4. Poet needs to access RAPL as root so run "sudo bash"
5. Load MSR with "modprobe msr"
6. Set environment variable for libheartbeat "export HEARTBEAT_ENABLED_DIR=/tmp"
7. Toggle to 'userspace' governor with "/tools/set-governor.sh"
8. Now run benchmark with poet

To run only heartbeat for energy measurment you only need
1. You need to be root to access MSR "sudo bash"
2. You may need to load MSR "modprobe MSR"
3. Set environment variable for libheartbeat "export HEARTBEAT_ENABLED_DIR=/tmp"
4. Now run benchmark with hearbeat and results can be found in heartbeat.log 

## Substructures ##
There are several substructures implemented for testing with corresponding middleware.
For all theese substructures we have added a strcpy of 32 bytes where data would be transferred to/from the KVS to give correct measurements for the 32 bytes of data in the YSCB benchmarks.
### B+ Tree ###
The B+ Tree is based on previous code by Amittai Aviram and Ibrahim Umar. The solution implemented will lock the parent(s) of any leaf that requires a change before beeing able to modify. This may cause a substantial part of the three to be locked when modifing, but may also benefit from partitioning then. There exists better solutions for concurrent B-tree but this was chosen because of familiarity. 
### Hash Map ###
The Hash Map was written from scratch for this assignment. It uses a simple hash to uniformly distribute keys across buckets. The buckets may chain additional records that hash to the same bucket. When the hash map starts to fill up we would generally repartition the hashmap to avoid long chained records, but we dont want to take this repartitioning occuring into measurements so we just allocate the map so large that this should never occur. 
### VEB-Tree ### 
The VEB-tree is based on previous code, and is designed from G. S. Brodal, R. Fagerberg, and R. Jacob, “Cache oblivious search trees via binary trees of small height,”. It was supplied as precode for the course but unfortunately is based on sharing global variables and is therefore not fit for partitioning. We include it however in some comparisons and making it partitionable would be interesting for future work.

## Overall Program Flow ##
The overall flow of the partitioned program is operations are read by the bench_client.c then db_put and db_get in gps.c is called to send operations into a message queue defined by msgq.c. The operations are retrieved from the queue by the SubStructFunc which in turn calls insert_into_struct or search_struct in %structname%middleware.c files. The middleware in turn calls on the actual insert/search operation of its designated struct. Then this whole path is reversed untill an result is retrieved in bench_client.

