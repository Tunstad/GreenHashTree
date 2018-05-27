CC = gcc
CFLAGS = -O2 #-Wall #-Wextra
INC =
LIB = -pthread -lm
BENCHLIB = -lpthread -lpoet -lhb-acc-pow-shared -lhb-energy-msr -lm #-pg

gpsbpt = gps.o msgq.o bpt.o bptmiddleware.o
gpssveb = gps.o msgq.o SVEB/SVEB.so
gpsshm = gps.o msgq.o shm.o shmmiddleware.o

blbpt = baseline.o msgq.o bpt.o bptmiddleware.o
blsveb = baseline.o msgq.o SVEB/SVEB.so
blshm = baseline.o msgq.o shm.o shmmiddleware.o

all: bench_gps_shm

%.o: %.c
	$(CC) $(CFLAGS) -c $^ $(INC) $(LIB)

bpt.o: BPT/bpt.c
	$(CC) $(CFLAGS) -c $^ -o $@ $(INC) $(LIB)

bptmiddleware.o: BPT/bptmiddleware.c bpt.o
	$(CC) $(CFLAGS) -c $^ -o $@ $(INC) bpt.o $(LIB)

shm.o: SimpleHashMap/shm.c
	$(CC) $(CFLAGS) -c $^ -o $@ $(INC) $(LIB)

shmmiddleware.o: SimpleHashMap/shmmiddleware.c shm.o
	$(CC) $(CFLAGS) -c $^ -o $@ $(INC) shm.o $(LIB)

SVEB/SVEB.so:
	make -C SVEB/

bench_gps_bpt: bench_client.o $(gpsbpt)
	gcc -g -O3 -o bench_client bench_client.o $(gpsbpt) $(BENCHLIB)

bench_gps_sveb: bench_client.o $(gpssveb)
	gcc -g -O3 -o bench_client bench_client.o $(gpssveb) $(BENCHLIB)

bench_gps_shm: bench_client.o $(gpsshm)
	gcc -g -O3 -o bench_client bench_client.o $(gpsshm) $(BENCHLIB)

bench_baseline_bpt: bench_client.o $(blbpt)
	gcc -g -O3 -o bench_client bench_client.o $(blbpt) $(BENCHLIB)

bench_baseline_sveb: bench_client.o $(blsveb)
	gcc -g -O3 -o bench_client bench_client.o $(blsveb) $(BENCHLIB)

bench_baseline_shm: bench_client.o $(blshm)
	gcc -g -O3 -o bench_client bench_client.o $(blshm) $(BENCHLIB)

bench_client.o: benchmark/bench_client.c
	gcc -g -O3 -c benchmark/bench_client.c

clean:
	@rm -fv *~ *.o core*
	@rm -fv $(gpsbpt) $(gpssveb) $(gpsshm) $(blbpt) $(blsveb) $(blshm)
	@rm -fv bench_client gmon.out

