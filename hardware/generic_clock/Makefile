CC=gcc
CFLAGS=-std=gnu99 -O3 -g -Wno-unused-result -I ../../ -c -pthread

all: clock

lem1802: clock.c
	$(CC) $(CFLAGS) -o clock.o clock.c

clean:
	rm clock.o
