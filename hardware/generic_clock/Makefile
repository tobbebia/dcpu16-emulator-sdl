CC=gcc
CFLAGS=-std=c99 -O3 -g -Wno-unused-result -I ../../include -c

all: clock

clock: clock.c
	$(CC) $(CFLAGS) -o clock.o clock.c 

clean:
	rm clock.o
