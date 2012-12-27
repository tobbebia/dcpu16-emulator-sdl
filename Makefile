CC=gcc
LD=ld
CFLAGS=-std=c99 -O3 -g -Wno-unused-result -Iinclude -D_POSIX_C_SOURCE=199309

# Hardware modules path
generic_clock		= hardware/generic_clock/clock.o
generic_keyboard	= hardware/generic_keyboard/keyboard.o
LEM1802			= hardware/LEM1802/lem1802.o
SPC200			=

# Variables to edit if you are making your own project based on this emulator core
MAIN=main.c
HARDWARE=$(LEM1802) $(generic_keyboard) $(generic_clock)
ADDITIONAL_CFILES=
ADDITIONAL_CFLAGS=
ADDITIONAL_LIBS=-lSDL
OUTPUT_BINARY_NAME=dcpu16sdl


all: dcpu16

dcpu16: dcpu16.c
	mkdir -p bin
	$(CC) $(CFLAGS) $(ADDITIONAL_CFLAGS) $(HARDWARE) dcpu16.c $(MAIN) $(ADDITIONAL_CFILES) $(ADDITIONAL_LIBS) -o bin/$(OUTPUT_BINARY_NAME) 

clean:
	rm bin/dcpu16
