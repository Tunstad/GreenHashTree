CC = gcc

# Cflags
CFLAGS = -O2 -Wall #-Wextra -g

INC = 
LIB = 

all: GreenHashTree

GreenHashTree: ght.c
	$(CC) $(CFLAGS) ght.c -o GreenHashTree $(INC) $(LIB)

GreenHashTreeTwo: main.c
	$(CC) $(CFLAGS) ght.c -o GreenHashTreeTwo $(INC) $(LIB)
clean:
	rm -f *~ *.o core* GreenHashTree
