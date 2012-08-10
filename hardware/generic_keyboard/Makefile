CC=gcc
CFLAGS=-std=c99 -O3 -g -Wno-unused-result -I ../../include -c

all: keyboard

keyboard: keyboard.c
	$(CC) $(CFLAGS) -o keyboard.o keyboard.c

clean:
	rm keyboard.o
