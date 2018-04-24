CC = gcc

SOURCE = ght.c msgq.c
HEADER = ght.h msgq.h 
# Cflags
CFLAGS = -O2 #-Wall #-Wextra -g

INC = 
LIB = -pthread -lm

CBTobjs = ght.o msgq.o cbtree.o #CBTree/CBTree.so
BPTobjs = ght.o msgq.o bpt.o bptmiddleware.o
SVEBobjs = ght.o msgq.o SVEB/SVEB.so

all: bptmain

svebmain: $(SVEBobjs)
	$(CC) $(CFLAGS) -o $@ $^ $(INC) $(LIB) -LSVEB/SVEB.so

cbtmain:  $(CBTobjs)
	$(CC) $(CFLAGS) -o $@ $^ $(INC) $(LIB)

bptmain:  $(BPTobjs)
	$(CC) $(CFLAGS) -o $@ $^ $(INC) $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ $(INC) $(LIB)

bpt.o: BPT/bpt.c
	$(CC) $(CFLAGS) -c $^ -o $@ $(INC) $(LIB)

bptmiddleware.o: BPT/bptmiddleware.c bpt.o
	$(CC) $(CFLAGS) -c $^ -o $@ $(INC) bpt.o $(LIB)

cbtree.o: CBTree/cbtree.c
	$(CC) $(CFLAGS) -c $^ -o $@ $(INC) -Icommon $(LIB)

CBTree/CBTree.so:
	make -C CBTree/

SVEB/SVEB.so:
	make -C SVEB/

clean:
	@rm -fv *~ *.o core*
	@rm -fv $(CBTobjs) $(BPTobjs) $(SVEBobjs)
