#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "dcpu16.h"

#define KEYBOARD_ID		0x30cf7406
#define KEYBOARD_VERSION	1
#define KEYBOARD_MANUFACTURER	0 // UNKNOWN

#define KEYBOARD_INC_INT_CLEAR_BUFFER		0
#define KEYBOARD_INC_INT_NEXT_KEY		1
#define KEYBOARD_INC_INT_KEY_STATE		2
#define KEYBOARD_INC_INT_ENABLE_INT		3

#define KEYBOARD_BUFFER_SIZE			16

#define KEYBOARD_KEY_BACKSPACE		0x10
#define KEYBOARD_KEY_RETURN		0x11
#define KEYBOARD_KEY_INSERT		0x12
#define KEYBOARD_KEY_DELETE		0x13
#define KEYBOARD_KEY_ASCII_START	0x20
#define KEYBOARD_KEY_ASCII_END		0x7f
#define KEYBOARD_KEY_ARROW_UP		0x80
#define KEYBOARD_KEY_ARROW_DOWN 	0x81
#define KEYBOARD_KEY_ARROW_LEFT		0x82
#define KEYBOARD_KEY_ARROW_RIGHT	0x83
#define KEYBOARD_KEY_SHIFT		0x90
#define KEYBOARD_KEY_CONTROL		0x91

#define KEYBOARD_KEY_STATE_BUFFER_SIZE		0xFFFF

typedef struct _keyboard_t
{
	// Key buffer, may only be accessed by one thread at a time
	volatile DCPU16_WORD buffer[KEYBOARD_BUFFER_SIZE];
	volatile unsigned int keys_in_buffer;
	pthread_mutex_t buffer_mutex;

	// Key state array
	unsigned char key_state[KEYBOARD_KEY_STATE_BUFFER_SIZE];
	pthread_mutex_t key_state_mutex;

	volatile DCPU16_WORD interrupt_message;

	dcpu16_t *computer;
} keyboard_t;

void keyboard_create(dcpu16_hardware_t *hardware, dcpu16_t *computer);
void keyboard_destroy(dcpu16_hardware_t *hardware);

void keyboard_key_typed(dcpu16_hardware_t * hardware, DCPU16_WORD key);
void keyboard_update_key_state(dcpu16_hardware_t * hardware, DCPU16_WORD key, char state);
void keyboard_throw_interrupt(dcpu16_hardware_t * hardware);

#endif
