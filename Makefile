CC = gcc
CFLAGS = -O2 #-Wall #-Wextra
INC =
LIB = -pthread -lm
BENCHLIB = -lpthread -lpoet -lhb-acc-pow-shared -lhb-energy-msr -lm

ghtbpt = ght.o msgq.o bpt.o bptmiddleware.o
ghtsveb = ght.o msgq.o SVEB/SVEB.so
ghtshm = ght.o msgq.o shm.o shmmiddleware.o

blbpt = baseline.o msgq.o bpt.o bptmiddleware.o
blsveb = baseline.o msgq.o SVEB/SVEB.so
blshm = baseline.o msgq.o shm.o shmmiddleware.o

all: bench_ght_btp

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

bench_ght_btp: bench_client.o $(ghtbpt)
	gcc -g -O3 -o bench_client bench_client.o $(ghtbpt) $(BENCHLIB)

bench_ght_sveb: bench_client.o $(ghtsveb)
	gcc -g -O3 -o bench_client bench_client.o $(ghtsveb) $(BENCHLIB)

bench_ght_shm: bench_client.o $(ghtshm)
	gcc -g -O3 -o bench_client bench_client.o $(ghtshm) $(BENCHLIB)

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
	@rm -fv $(ghtbpt) $(ghtsveb) $(ghtshm) $(blbpt) $(blsveb) $(blshm)
	@rm -fv bench_client
