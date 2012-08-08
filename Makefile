CC=gcc
LD=ld
CFLAGS=-std=c99 -O3 -g -Wno-unused-result -Iinclude

# Hardware modules path
generic_clock		= hardware/generic_clock/clock.o
generic_keyboard	= 
LEM1802			= hardware/LEM1802/lem1802.o
SPC200			=

# Variables to edit if you are making your own project based on this emulator core
MAIN=main.c
HARDWARE=$(LEM1802)
ADDITIONAL_CFILES=
ADDITIONAL_CFLAGS=
ADDITIONAL_LIBS=-lSDL
OUTPUT_BINARY_NAME=dcpu16sdl


all: dcpu16

dcpu16: dcpu16.c
	mkdir -p bin
	$(CC) $(CFLAGS) $(ADDITIONAL_CFLAGS) $(HARDWARE) dcpu16.c $(MAIN) $(ADDITIONAL_CFILES) -o bin/$(OUTPUT_BINARY_NAME) $(ADDITIONAL_LIBS)

clean:
	rm bin/dcpu16
