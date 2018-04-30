all: bench_client

bench_client: bench_client.o dummy-keystore.o
	gcc -g -O3 -o bench_client bench_client.o dummy-keystore.o -lpthread -lpoet -lhb-acc-pow-shared -lhb-energy-msr -lm

dummy-keystore.o: example/dummy-keystore.c
	gcc -g -O3 -c example/dummy-keystore.c

bench_client.o: bench_client.c
	gcc -g -O3 -c bench_client.c

clean:
	-rm *.o bench_client