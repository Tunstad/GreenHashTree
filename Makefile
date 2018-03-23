CC = gcc

SOURCE = ght.c msgq.c bpt.c
HEADER = ght.h msgq.h bpt.h
# Cflags
CFLAGS = -O2 #-Wall #-Wextra -g

INC = 
LIB = -pthread -lm

all: BPT MsgQ GreenHashTree main


main:  $(SOURCE) $(HEADER)
	$(CC) $(CFLAGS) -o GreenHashTree ght.o msgq.o bpt.o $(INC) $(LIB)

GreenHashTree: $(SOURCE) $(HEADER)
	$(CC) $(CFLAGS) -c ght.c $(INC) $(LIB)

MsgQ:  $(SOURCE) $(HEADER)
	$(CC) $(CFLAGS) -c msgq.c $(INC) $(LIB)

BPT:  $(SOURCE) $(HEADER)
	$(CC) $(CFLAGS) -c bpt.c $(INC) $(LIB)

clean:
	rm -f *~ *.o core* GreenHashTree
	rm -f *~ *.o core* msgq.o ght.o
