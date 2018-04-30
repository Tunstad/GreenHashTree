CC = gcc

SOURCE = ght.c msgq.c
HEADER = ght.h msgq.h 
# Cflags
CFLAGS = -O2 #-Wall #-Wextra -g

INC = 
LIB = -pthread -lm


BPTobjs = ght.o msgq.o bpt.o bptmiddleware.o
SVEBobjs = ght.o msgq.o SVEB/SVEB.so
SHMobjs = ght.o msgq.o shm.o shmmiddleware.o

all: bptmain

svebmain: $(SVEBobjs)
	$(CC) $(CFLAGS) -o $@ $^ $(INC) $(LIB) -LSVEB/SVEB.so

ghtsveb.o: $(SVEBobjs)
	$(CC) $(CFLAGS) -c $^ -o $@ $(INC) $(LIB) -LSVEB/SVEB.so

shmmain:  $(SHMobjs)
	$(CC) $(CFLAGS) -o $@ $^ $(INC) $(LIB)

ghtshm.o:  $(SHMobjs)
	$(CC) $(CFLAGS) -c $^ -o $@ $(INC) $(LIB)

bptmain:  $(BPTobjs)
	$(CC) $(CFLAGS) -o $@ $^ $(INC) $(LIB)

ghtbpt.o:  $(SHMobjs)
	$(CC) $(CFLAGS) -c $^ -o $@  $(INC) $(LIB)

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

bench_client: bench_client.o $(SVEBobjs)
	gcc -g -O3 -o bench_client bench_client.o $(SVEBobjs) -lpthread -lpoet -lhb-acc-pow-shared -lhb-energy-msr -lm

bench_client.o: benchmark/bench_client.c
	gcc -g -O3 -c benchmark/bench_client.c

clean:
	@rm -fv *~ *.o core*
	@rm -fv $(CBTobjs) $(BPTobjs) $(SVEBobjs)
	@rm -fv svebmain shmmain bptmain
