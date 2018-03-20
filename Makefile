CC = gcc

# Cflags
CFLAGS = -O2 -Wall #-Wextra -g

INC = 
LIB = 

all: GreenHashTree

GreenHashTree: main.c
	$(CC) $(CFLAGS) main.c -o GreenHashTree $(INC) $(LIB)

GreenHashTreeTwo: main.c
	$(CC) $(CFLAGS) main.c -o GreenHashTreeTwo $(INC) $(LIB)
clean:
	rm -f *~ *.o core* main
