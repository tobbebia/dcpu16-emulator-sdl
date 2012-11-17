#ifndef CLOCK_H
#define CLOCK_H

#include <pthread.h>
#include "dcpu16.h"

#define CLOCK_ID		0x12d0b402
#define CLOCK_VERSION		1
#define CLOCK_MANUFACTURER	0		// Unknown manufacturer (not present in specs)

#define CLOCK_INC_INT_0		0
#define CLOCK_INC_INT_1		1
#define CLOCK_INC_INT_2		2

typedef struct _generic_clock_t
{
	DCPU16_WORD message;
	DCPU16_WORD ticks;
	unsigned int interval;	// Milliseconds

	volatile char running;
	pthread_t thread;

	dcpu16_t *computer;
} generic_clock_t;

void clock_create(dcpu16_hardware_t *hardware, dcpu16_t *computer);
void clock_destroy(dcpu16_hardware_t *hardware);

#endif
