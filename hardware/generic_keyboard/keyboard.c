#include <stdlib.h>
#include <string.h>
#include "keyboard.h"

static int keyboard_interrupt(dcpu16_hardware_t *hardware)
{
	keyboard_t *keyboard = (keyboard_t *) hardware->custom_struct;

	// Read the A and B registers
	DCPU16_WORD reg_a = keyboard->computer->registers[DCPU16_INDEX_REG_A];
	DCPU16_WORD reg_b = keyboard->computer->registers[DCPU16_INDEX_REG_B];

	// Handle the interrupt
	switch(reg_a) {
	case KEYBOARD_INC_INT_CLEAR_BUFFER:
		pthread_mutex_lock(&keyboard->buffer_mutex);

		memset((void *)keyboard->buffer, 0, sizeof(keyboard->buffer));
		keyboard->keys_in_buffer = 0;

		pthread_mutex_unlock(&keyboard->buffer_mutex);
		break;
	case KEYBOARD_INC_INT_NEXT_KEY:
		printf("NEXT KEY\n");

		pthread_mutex_lock(&keyboard->buffer_mutex);

		if(keyboard->keys_in_buffer > 0) {

			keyboard->computer->registers[DCPU16_INDEX_REG_C] = keyboard->buffer[0];
			keyboard->keys_in_buffer--;
			
			// Move rest of buffer
			DCPU16_WORD tmp_buffer[KEYBOARD_BUFFER_SIZE];
			memset((void *)tmp_buffer, 0, sizeof(tmp_buffer));
			memcpy((void *)tmp_buffer, (void *)&keyboard->buffer[1], sizeof(keyboard->buffer) - sizeof(DCPU16_WORD));

			memset((void *)keyboard->buffer, 0, sizeof(keyboard->buffer));
			memcpy((void *)keyboard->buffer, (void *)tmp_buffer, sizeof(keyboard->buffer));

		printf("C Set to %i \n", keyboard->computer->registers[DCPU16_INDEX_REG_C]);
		} else {
			printf("no key in buffer\n");
			keyboard->computer->registers[DCPU16_INDEX_REG_C] = 0;
		}

		pthread_mutex_unlock(&keyboard->buffer_mutex);
		break;
	case KEYBOARD_INC_INT_KEY_STATE:
		pthread_mutex_lock(&keyboard->key_state_mutex);

		keyboard->computer->registers[DCPU16_INDEX_REG_C] = keyboard->key_state[reg_b];

		pthread_mutex_unlock(&keyboard->key_state_mutex);
		break;
	case KEYBOARD_INC_INT_ENABLE_INT:
		keyboard->interrupt_message = reg_b;
		break;
	};

	return 0;
}


void keyboard_create(dcpu16_hardware_t *hardware, dcpu16_t *computer)
{
	// Create a keyboard structure
	keyboard_t *keyboard = malloc(sizeof(keyboard_t));
	memset(keyboard, 0, sizeof(keyboard_t));

	keyboard->computer = computer;

	// Set the hardware structure
	hardware->present = 1;
	hardware->hardware_id = KEYBOARD_ID;
	hardware->hardware_version = KEYBOARD_VERSION;
	hardware->hardware_manufacturer = KEYBOARD_MANUFACTURER;
	hardware->interrupt = keyboard_interrupt;
	hardware->custom_struct = (void *) keyboard;
}

void keyboard_destroy(dcpu16_hardware_t *hardware)
{
	// Free the keyboard structure
	free(hardware->custom_struct);

	// Set the present indicator to 0
	hardware->present = 0;
}

void keyboard_key_typed(dcpu16_hardware_t * hardware, DCPU16_WORD key)
{
	keyboard_t *keyboard = (keyboard_t *) hardware->custom_struct;
	
	pthread_mutex_lock(&keyboard->buffer_mutex);

	if(keyboard->keys_in_buffer < KEYBOARD_BUFFER_SIZE) {
		keyboard->buffer[keyboard->keys_in_buffer] = key;
		keyboard->keys_in_buffer++;
	}

	pthread_mutex_unlock(&keyboard->buffer_mutex);
}


void keyboard_update_key_state(dcpu16_hardware_t * hardware, DCPU16_WORD key, char state)
{
	keyboard_t *keyboard = (keyboard_t *) hardware->custom_struct;

	pthread_mutex_lock(&keyboard->key_state_mutex);

	keyboard->key_state[key] = state;

	pthread_mutex_unlock(&keyboard->key_state_mutex);
}

void keyboard_throw_interrupt(dcpu16_hardware_t * hardware)
{
	keyboard_t *keyboard = (keyboard_t *) hardware->custom_struct;
	DCPU16_WORD imsg = keyboard->interrupt_message;

	if(imsg != 0) {
		dcpu16_interrupt(keyboard->computer, imsg, 0);
	}
}
