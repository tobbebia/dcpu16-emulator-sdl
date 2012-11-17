#include <stdlib.h>
#include <string.h>
#include "clock.h"

static void * clock_thread(void * arg)
{
	generic_clock_t * clock = (generic_clock_t *) arg;

	while(clock->running) {
		// Sleep
		usleep(clock->interval * 1000);
		
		// Increase tick count
		clock->ticks++;
		
		// Interrupt computer?
		if(clock->message) {
			dcpu16_interrupt(clock->computer, clock->message, 0);
		}
	}

	pthread_exit(0);
}

static int clock_interrupt(dcpu16_hardware_t *hardware)
{
	generic_clock_t * clock = (generic_clock_t *) hardware->custom_struct;

	// Read the A and B registers
	DCPU16_WORD reg_a = clock->computer->registers[DCPU16_INDEX_REG_A];
	DCPU16_WORD reg_b = clock->computer->registers[DCPU16_INDEX_REG_B];
	
	// Handle the interrupt
	switch(reg_a) {
	case CLOCK_INC_INT_0:
		if(reg_b == 0) {
			clock->running = 0;
		} else {
			clock->interval = (unsigned int) (1000.0 / (60.0 / reg_b));

			if(!clock->running) {
				// Start the clock thread
				clock->running = 1;
				pthread_create(&clock->thread, 0, (void *) &clock_thread, (void *) clock);
			}
		}

		break;
	case CLOCK_INC_INT_1:
		dcpu16_set(clock->computer, &clock->computer->registers[DCPU16_INDEX_REG_C], clock->ticks);	

		break;
	case CLOCK_INC_INT_2:
		clock->message = reg_b;

		break;
	};

	return 0;
}

void clock_create(dcpu16_hardware_t *hardware, dcpu16_t *computer)
{
	// Create a clock structure
	generic_clock_t *clock = malloc(sizeof(generic_clock_t));
	memset(clock, 0, sizeof(generic_clock_t));

	clock->computer = computer;

	// Set the hardware structure
	hardware->present = 1;
	hardware->hardware_id = CLOCK_ID;
	hardware->hardware_version = CLOCK_VERSION;
	hardware->hardware_manufacturer = CLOCK_MANUFACTURER;
	hardware->interrupt = clock_interrupt;
	hardware->custom_struct = (void *) clock;
}

void clock_destroy(dcpu16_hardware_t *hardware)
{
	generic_clock_t * clock = (generic_clock_t *) hardware->custom_struct;
	clock->running = 0;
	
	// Join the thread
	pthread_join(clock->thread, 0);

	// Free the clock structure
	free(hardware->custom_struct);

	// Set the present indicator to 0
	hardware->present = 0;
}
