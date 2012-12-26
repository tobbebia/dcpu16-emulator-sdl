#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "dcpu16.h"

#define __need_struct_timeval 1

/* Must be used when setting the value of any register or RAM address of the emulated computer. */
inline void dcpu16_set(dcpu16_t *computer, DCPU16_WORD *where, DCPU16_WORD value)
{
	if(where >= computer->ram && where < computer->ram + DCPU16_RAM_SIZE) {	// RAM
		// Calculate the RAM address
		DCPU16_WORD ram_address = where - computer->ram;

		// Call the ram_changed callback function
		if(computer->callback.ram_changed)
			computer->callback.ram_changed(ram_address, value);

	} else if(where >= computer->registers && where < computer->registers + DCPU16_REGISTER_COUNT) {	// Register
		// Call the register_changed callback function
		if(computer->callback.register_changed)
			computer->callback.register_changed(where - computer->registers, value);
	}	
	
	// Set
	*where = value;
}

/* Must be used when getting the value of any register or RAM of the emulated computer.
   Right now this function doesn't do anything special, but it can be used if we want to intercept reads in the future. */
inline DCPU16_WORD dcpu16_get(dcpu16_t *computer, DCPU16_WORD *where)
{
	if(where >= computer->ram && where < computer->ram + DCPU16_RAM_SIZE) {	// RAM

	} else if(where >= computer->registers && where < computer->registers + DCPU16_REGISTER_COUNT) {	// Register
		
	}

	// Get
	return *where;
}

/* Adds an interrupt to the queue. It's okay to call this function from different threads, it's supposed to be thread safe. */
void dcpu16_interrupt(dcpu16_t *computer, DCPU16_WORD message, char software_interrupt) 
{
	if(!computer->on_breakpoint) {
		// Lock the interrupt queue mutex
		pthread_mutex_lock(&computer->interrupt_queue_mutex);

		// Incoming hardware interrupts are ignored if IA is 0, software interrupts are still queued
		if(dcpu16_get(computer, &computer->registers[DCPU16_INDEX_REG_IA]) || software_interrupt) {
			if(computer->interrupt_queue) {
				if(computer->interrupt_queue_length == DCPU16_MAX_INTERRUPT_QUEUE_LENGTH) {
					// Set computer on fire
					PRINTF("COMPUTER IS ON FIRE!");
				} else {
					dcpu16_queued_interrupt_t *interrupt = malloc(sizeof(dcpu16_queued_interrupt_t));
					interrupt->message = message;
					interrupt->next = 0;

					computer->interrupt_queue_end->next = interrupt;
					computer->interrupt_queue_end = interrupt;
					computer->interrupt_queue_length++;
				}
		
			} else {
				dcpu16_queued_interrupt_t *interrupt = malloc(sizeof(dcpu16_queued_interrupt_t));
				interrupt->message = message;
				interrupt->next = 0;

				computer->interrupt_queue = interrupt;
				computer->interrupt_queue_end = interrupt;
				computer->interrupt_queue_length++;
			}
		}

		// Unock the interrupt queue mutex
		pthread_mutex_unlock(&computer->interrupt_queue_mutex);
	}
}

/* Checks the queue and handles the interrupt if there is one. */
void dcpu16_trigger_interrupt(dcpu16_t *computer) 
{
	if(computer->trigger_interrupts) {
		// Lock the interrupt queue mutex
		pthread_mutex_lock(&computer->interrupt_queue_mutex);

		if(computer->interrupt_queue) {
			if(dcpu16_get(computer, &computer->registers[DCPU16_INDEX_REG_IA]) == 0) {
				// Just remove the interrupt from the queue
				dcpu16_queued_interrupt_t * to_remove = computer->interrupt_queue;
				computer->interrupt_queue = to_remove->next;

				// Free the removed interrupt
				free(to_remove);
				computer->interrupt_queue_length--;
			} else {
				// Remove the interrupt from the queue
				dcpu16_queued_interrupt_t * to_remove = computer->interrupt_queue;
				computer->interrupt_queue = to_remove->next;

				// Push PC
				dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_SP], dcpu16_get(computer, &computer->registers[DCPU16_INDEX_REG_SP]) - 1);
				dcpu16_set(computer, &computer->ram[computer->registers[DCPU16_INDEX_REG_SP]], computer->registers[DCPU16_INDEX_REG_PC]);
	
				// Push A
				dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_SP], dcpu16_get(computer, &computer->registers[DCPU16_INDEX_REG_SP]) - 1);
				dcpu16_set(computer, &computer->ram[computer->registers[DCPU16_INDEX_REG_SP]], computer->registers[DCPU16_INDEX_REG_A]);

				// Set PC to IA
				dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_PC], dcpu16_get(computer, &computer->registers[DCPU16_INDEX_REG_IA]));

				// Set A to the interrupt message
				dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_A], to_remove->message);

				// Disable interrupt triggering until this interrupt returns
				computer->trigger_interrupts = 0;

				// PRINT
				PRINTF("triggering interrupt, ia: %x, message: %x\n", computer->registers[DCPU16_INDEX_REG_IA], to_remove->message);

				// Free the removed interrupt
				free(to_remove);
				computer->interrupt_queue_length--;
			}

		}

		// Unock the interrupt queue mutex
		pthread_mutex_unlock(&computer->interrupt_queue_mutex);
	}
}

/* This function calls the register_changed callback for the PC register. */
static inline void dcpu16_pc_callback(dcpu16_t *computer)
{	
	if(computer->callback.register_changed)
		computer->callback.register_changed(DCPU16_INDEX_REG_PC, computer->registers[DCPU16_INDEX_REG_PC]);
}

/* Returns a pointer to the register with the index specified.
   NOTE: this can work with any register, but it is intended to
   be used for registers a, b, c, x, y, z, i and j.  Addressing
   other registers using this function is not recommended. */
static inline DCPU16_WORD *dcpu16_register_pointer(dcpu16_t *computer, char index)
{
	return &computer->registers[index];
}

/* Sets *retval to point to a register or a DCPU16_WORD in RAM. Returns the number of cycles it took to look it up. */
static unsigned char dcpu16_get_pointer(dcpu16_t *computer, unsigned char where, DCPU16_WORD *tmp_storage, DCPU16_WORD **retval, char is_a, char to_be_skipped)
{
	if(where <= DCPU16_AB_VALUE_REG_J) {
		// 0x00-0x07 (value of register)
		*retval = dcpu16_register_pointer(computer, where);
		return 0;
	} else if(where <= DCPU16_AB_VALUE_PTR_REG_J) {
		// 0x08-0x0f (value at address pointed to by register)
		*retval = &computer->ram[*dcpu16_register_pointer(computer, where - DCPU16_AB_VALUE_PTR_REG_A)];
		return 0;
	} else if(where <= DCPU16_AB_VALUE_PTR_REG_J_PLUS_WORD) {
		// 0x10-0x17 (value at address pointed to by the sum of the register and the next word)
		*retval = &computer->ram[(DCPU16_WORD)(*dcpu16_register_pointer(computer, where - DCPU16_AB_VALUE_PTR_REG_A_PLUS_WORD) +
			computer->ram[computer->registers[DCPU16_INDEX_REG_PC]])];
		computer->registers[DCPU16_INDEX_REG_PC]++;
		return 1;
	} else if(where >= 0x20 && where <= 0x3F) {
		// 0x20-0x3F (literal value from 0xFFFF to 0x1E (-1 to 30))
		if(tmp_storage)
			*tmp_storage = where - 0x20 + 0xFFFF;
		*retval = tmp_storage;
		return 0;
	}

	// The rest are handled individually
	switch(where) {
	case DCPU16_AB_VALUE_PUSH_OR_POP:
		if(is_a) {
			*retval = &computer->ram[computer->registers[DCPU16_INDEX_REG_SP]];

			if(!to_be_skipped)
				dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_SP], dcpu16_get(computer, &computer->registers[DCPU16_INDEX_REG_SP]) + 1);
			
		} else {
			if(!to_be_skipped)
				dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_SP], dcpu16_get(computer, &computer->registers[DCPU16_INDEX_REG_SP]) - 1);
			*retval = &computer->ram[computer->registers[DCPU16_INDEX_REG_SP]];
		}

		return 0;
	case DCPU16_AB_VALUE_PEEK:
		*retval = &computer->ram[computer->registers[DCPU16_INDEX_REG_SP]];
		return 0;
	case DCPU16_AB_VALUE_PICK:
		*retval = &computer->ram[computer->registers[DCPU16_INDEX_REG_SP] + computer->ram[computer->registers[DCPU16_INDEX_REG_PC]]];
		computer->registers[DCPU16_INDEX_REG_PC]++;
		return 1;
	case DCPU16_AB_VALUE_REG_SP:
		*retval = &computer->registers[DCPU16_INDEX_REG_SP];
		return 0;
	case DCPU16_AB_VALUE_REG_PC:
		*retval = &computer->registers[DCPU16_INDEX_REG_PC];
		return 0;
	case DCPU16_AB_VALUE_REG_EX:
		*retval = &computer->registers[DCPU16_INDEX_REG_EX];
		return 0;
	case DCPU16_AB_VALUE_PTR_WORD:
		*retval = &computer->ram[computer->ram[computer->registers[DCPU16_INDEX_REG_PC]]];
		computer->registers[DCPU16_INDEX_REG_PC]++;
		return 1;
	case DCPU16_AB_VALUE_WORD:
		*retval = &computer->ram[computer->registers[DCPU16_INDEX_REG_PC]];
		computer->registers[DCPU16_INDEX_REG_PC]++;
		return 1;
	};

	*retval = 0;
	return 0;
}

/* Returns true if v is a literal (v is expected to be a 6-bit AB value). */
static inline char dcpu16_is_literal(char v)
{
	if(v == DCPU16_AB_VALUE_WORD || (v >= 0x20 && v <= 0x3F))
		return 1;

	return 0;
}

/* Skips the next instruction (advances PC). */
static int dcpu16_skip_next_instruction(dcpu16_t *computer)
{
	int extra_cycles = 0;

	// Parse the instruction but don't execute it
	DCPU16_WORD w = computer->ram[computer->registers[DCPU16_INDEX_REG_PC]];
	char opcode = w & 0x1F;
	computer->registers[DCPU16_INDEX_REG_PC]++;

	// Call the PC callback
	dcpu16_pc_callback(computer);

	if(opcode != DCPU16_OPCODE_NON_BASIC) {
		char a = (w >> 5) & 0x1F;
		char b = (w >> 10) & 0x3F;

		DCPU16_WORD *b_word;
		DCPU16_WORD *a_word;
		dcpu16_get_pointer(computer, a, 0, &a_word, 1, 1);
		dcpu16_get_pointer(computer, b, 0, &b_word, 0, 1);
	} else {
		char o = (w >> 5) & 0x1F;

		if(o != DCPU16_OPCODE_UNOFFICIAL_BREAKPOINT) {	// Only run get_pointer if this is is not a breakpoint opcode
			char a = (w >> 10) & 0x3F;

			DCPU16_WORD *a_word;
			dcpu16_get_pointer(computer, a, 0, &a_word, 1, 1);
		}
	}

	if(opcode >= DCPU16_OPCODE_IFB && opcode <= DCPU16_OPCODE_IFU) {

		extra_cycles += 1;
		extra_cycles += dcpu16_skip_next_instruction(computer);
	}

	return extra_cycles;
}

/* Executes the next instruction, returns the number of cycles used. */
unsigned char dcpu16_step(dcpu16_t *computer) 
{
	unsigned char cycles = 0;

	// Get the next instruction
	DCPU16_WORD w = computer->ram[computer->registers[DCPU16_INDEX_REG_PC]];
	char opcode = w & 0x1F;

	computer->registers[DCPU16_INDEX_REG_PC]++;

	// Find out if it is a non basic or basic instruction
	if(opcode == DCPU16_OPCODE_NON_BASIC) {
		// Non-basic instruction
		char o = (w >> 5) & 0x1F;
		char a = (w >> 10) & 0x3F;

		// Temporary storage for embedded literal values
		DCPU16_WORD a_literal_tmp;

		// Get pointer to A
		DCPU16_WORD *a_word;
		cycles += dcpu16_get_pointer(computer, a, &a_literal_tmp, &a_word, 1, 0);

		switch(o) {
		case DCPU16_NON_BASIC_OPCODE_RESERVED_0:
				
			break;
		case DCPU16_NON_BASIC_OPCODE_JSR_A:
			cycles += 3;
			
			DCPU16_WORD new_pc = dcpu16_get(computer, a_word);

			dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_SP], dcpu16_get(computer, &computer->registers[DCPU16_INDEX_REG_SP]) - 1);
			computer->ram[computer->registers[DCPU16_INDEX_REG_SP]] = computer->registers[DCPU16_INDEX_REG_PC];
			computer->registers[DCPU16_INDEX_REG_PC] = new_pc;	

			break;
		case DCPU16_NON_BASIC_OPCODE_INT:
			cycles += 4;
			dcpu16_interrupt(computer, dcpu16_get(computer, a_word), 1);
			break;
		case DCPU16_NON_BASIC_OPCODE_IAG:
			cycles += 1;
			dcpu16_set(computer, a_word, dcpu16_get(computer, &computer->registers[DCPU16_INDEX_REG_IA]));
			break;
		case DCPU16_NON_BASIC_OPCODE_IAS:
			cycles += 1;
			dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_IA], dcpu16_get(computer, a_word));
			break;
		case DCPU16_NON_BASIC_OPCODE_RFI:
			cycles += 3;

			// Pop A and PC
			dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_A], dcpu16_get(computer, &computer->ram[computer->registers[DCPU16_INDEX_REG_SP]]));
			dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_SP], dcpu16_get(computer, &computer->registers[DCPU16_INDEX_REG_SP]) + 1);
			dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_PC], dcpu16_get(computer, &computer->ram[computer->registers[DCPU16_INDEX_REG_SP]]));
			dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_SP], dcpu16_get(computer, &computer->registers[DCPU16_INDEX_REG_SP]) + 1);

			// Turn on interrupt triggering
			computer->trigger_interrupts = 1;
			
			break;
		case DCPU16_NON_BASIC_OPCODE_IAQ:
			cycles += 2;

			if(dcpu16_get(computer, a_word))
				computer->trigger_interrupts = 0;
			else
				computer->trigger_interrupts = 1;

			break;
		case DCPU16_NON_BASIC_OPCODE_HWN:
			cycles += 2;
			
			{
				// Count the number of connected hardware devices
				DCPU16_WORD connected_hardware = 0;
				for(int h = 0; h <  DCPU16_HARDWARE_SLOTS; h++) {
					if(computer->hardware[h].present)
						connected_hardware++;
				}

				dcpu16_set(computer, a_word, connected_hardware);
			}

			break;
		case DCPU16_NON_BASIC_OPCODE_HWQ: 
			cycles += 4;

			{
				// Get the hardware index
				DCPU16_WORD hardware_index = dcpu16_get(computer, a_word);
				dcpu16_hardware_t * hardware = &computer->hardware[hardware_index];

				// Set A, B, C, X, Y to information about the hardware
				if(hardware->present) {
					dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_A], (DCPU16_WORD) hardware->hardware_id);
					dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_B], (DCPU16_WORD) (hardware->hardware_id >> 16));
					dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_C], (DCPU16_WORD) hardware->hardware_version);
					dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_X], (DCPU16_WORD) hardware->hardware_manufacturer);
					dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_Y], (DCPU16_WORD) (hardware->hardware_manufacturer >> 16));
				}
			}

			break;
		case DCPU16_NON_BASIC_OPCODE_HWI:
			cycles += 4;
			
			{
				// Get the hardware index
				DCPU16_WORD hardware_index = dcpu16_get(computer, a_word);
				dcpu16_hardware_t * hardware = &computer->hardware[hardware_index];

				// Interrupt
				if(hardware->present && hardware->interrupt) {
					hardware->received_initial_hwi = 1;
					cycles += hardware->interrupt(hardware);
				}
			}


			break;
		};

	} else {
		// Basic instruction
		char b = (w >> 5) & 0x1F;
		char a = (w >> 10) & 0x3F;

		// Temporary storage for embedded literal values
		DCPU16_WORD a_literal_tmp, b_literal_tmp;

		// Get pointer to A and B
		DCPU16_WORD *a_word;
		DCPU16_WORD *b_word;
		cycles += dcpu16_get_pointer(computer, a, &a_literal_tmp, &a_word, 1, 0);
		cycles += dcpu16_get_pointer(computer, b, &b_literal_tmp, &b_word, 0, 0);

		// Give up if illegal instruction detected (trying set a literal value)
		char b_literal = dcpu16_is_literal(b);

		if(b_literal && ((opcode >= DCPU16_OPCODE_SET && opcode <= DCPU16_OPCODE_SHL) || (opcode >= DCPU16_OPCODE_ADX && opcode <= DCPU16_OPCODE_SBX) || (opcode >= DCPU16_OPCODE_STI || opcode <= DCPU16_OPCODE_STD))) 
			return 0; // TODO: find out if it is legal to return 0 cycles in this case.

		// Variables for use in the switch statement
		DCPU16_WORD a_val = 0;
		DCPU16_WORD b_val = 0;

		DCPU16_WORD_SIGNED a_val_s = 0;
		DCPU16_WORD_SIGNED b_val_s = 0;

		switch(opcode) {
		case DCPU16_OPCODE_SET:
			cycles += 1;
			dcpu16_set(computer, b_word, dcpu16_get(computer, a_word));

			break;
		case DCPU16_OPCODE_ADD:
			cycles += 2;

			a_val = dcpu16_get(computer, a_word);
			b_val = dcpu16_get(computer, b_word);

			if((int) b_val + (int) a_val > 0xFFFF) {
				dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_EX], 1);
			} else {
				dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_EX], 0);
			}

			dcpu16_set(computer, b_word, b_val + a_val);

			break;
		case DCPU16_OPCODE_SUB:
			cycles += 2;

			a_val = dcpu16_get(computer, a_word);
			b_val = dcpu16_get(computer, b_word);

			if((int) b_val - (int) a_val < 0) {
				dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_EX], 0xFFFF);
			} else {
				dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_EX], 0);
			}

			dcpu16_set(computer, b_word, b_val - a_val);
	
			break;
		case DCPU16_OPCODE_MUL:
			cycles += 2;

			a_val = dcpu16_get(computer, a_word);
			b_val = dcpu16_get(computer, b_word);

			dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_EX], ((b_val * a_val) >> 16) & 0xFFFF);
			dcpu16_set(computer, b_word, b_val * a_val);

			break;
		case DCPU16_OPCODE_MLI:
			cycles += 2;

			a_val_s = dcpu16_get(computer, a_word);
			b_val_s = dcpu16_get(computer, b_word);

			dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_EX], ((b_val_s * a_val_s) >> 16) & 0xFFFF);
			dcpu16_set(computer, b_word, b_val_s * a_val_s);

			break;
		case DCPU16_OPCODE_DIV:
			cycles += 3;

			a_val = dcpu16_get(computer, a_word);
			b_val = dcpu16_get(computer, b_word);

			if(a_val == 0) {
				dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_B], 0);
				dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_EX], 0);
			} else {
				dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_EX], ((b_val << 16) / a_val) & 0xFFFF);
				dcpu16_set(computer, b_word, b_val / a_val);
			}

			break;
		case DCPU16_OPCODE_DVI:
			cycles += 3;

			a_val_s = dcpu16_get(computer, a_word);
			b_val_s = dcpu16_get(computer, b_word);

			if(a_val_s == 0) {
				dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_B], 0);
				dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_EX], 0);
			} else {
				dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_EX], ((b_val_s << 16) / a_val_s) & 0xFFFF);
				dcpu16_set(computer, b_word, b_val_s / a_val_s);
			}

			break;
		case DCPU16_OPCODE_MOD:
			cycles += 3;

			a_val = dcpu16_get(computer, a_word);
			b_val = dcpu16_get(computer, b_word);

			if(a_val == 0) {
				dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_B], 0);
			} else {
				dcpu16_set(computer, b_word, b_val % a_val);
			}

			break;
		case DCPU16_OPCODE_MDI:
			cycles += 3;

			a_val_s = dcpu16_get(computer, a_word);
			b_val_s = dcpu16_get(computer, b_word);

			if(a_val_s == 0) {
				dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_B], 0);
			} else {
				dcpu16_set(computer, b_word, b_val_s % a_val_s);
			}

			break;
		case DCPU16_OPCODE_AND:
			cycles += 1;

			dcpu16_set(computer, b_word, dcpu16_get(computer, a_word) & dcpu16_get(computer, b_word));

			break;
		case DCPU16_OPCODE_BOR:
			cycles += 1;

			dcpu16_set(computer, b_word, dcpu16_get(computer, a_word) | dcpu16_get(computer, b_word));

			break;
		case DCPU16_OPCODE_XOR:
			cycles += 1;

			dcpu16_set(computer, b_word, dcpu16_get(computer, a_word) ^ dcpu16_get(computer, b_word));

			break;	
		case DCPU16_OPCODE_SHR:
			cycles += 1;

			a_val = dcpu16_get(computer, a_word);
			b_val = dcpu16_get(computer, b_word);

			dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_EX], ((b_val << 16) >> a_val) & 0xFFFF);
			dcpu16_set(computer, b_word, b_val >> a_val);

			break;
		case DCPU16_OPCODE_ASR:
			cycles += 1;

			a_val = dcpu16_get(computer, a_word);
			b_val_s = dcpu16_get(computer, b_word);

			dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_EX], ((b_val_s << 16) >> a_val) & 0xFFFF);
			dcpu16_set(computer, b_word, b_val_s >> a_val);

			break;
		case DCPU16_OPCODE_SHL:
			cycles += 1;

			a_val = dcpu16_get(computer, a_word);
			b_val = dcpu16_get(computer, b_word);

			dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_EX], ((b_val << a_val) >> 16) & 0xFFFF);
			dcpu16_set(computer, b_word, b_val << a_val);

			break;
		case DCPU16_OPCODE_IFB:
			cycles += 2;	

			if(dcpu16_get(computer, a_word) & dcpu16_get(computer, b_word) == 0)
			{
				cycles += dcpu16_skip_next_instruction(computer);
				cycles++;
			}

			break;
		case DCPU16_OPCODE_IFC:
			cycles += 2;	

			if(dcpu16_get(computer, a_word) & dcpu16_get(computer, b_word) != 0)
			{
				cycles += dcpu16_skip_next_instruction(computer);
				cycles++;
			}

			break;
		case DCPU16_OPCODE_IFE:
			cycles += 2;

			if(dcpu16_get(computer, a_word) != dcpu16_get(computer, b_word))
			{
				cycles += dcpu16_skip_next_instruction(computer);
				cycles++;
			}
			
			printf("IFE\n");


			break;
		case DCPU16_OPCODE_IFN:
			cycles += 2;

			if(dcpu16_get(computer, a_word) == dcpu16_get(computer, b_word))
			{
				cycles += dcpu16_skip_next_instruction(computer);
				cycles++;
			}

			break;
		case DCPU16_OPCODE_IFG:
			cycles += 2;

			a_val = dcpu16_get(computer, a_word);
			b_val = dcpu16_get(computer, b_word);

			if(b_val <= a_val)
			{
				cycles += dcpu16_skip_next_instruction(computer);
				cycles++;
			}

			break;
		case DCPU16_OPCODE_IFA:
			cycles += 2;

			a_val = dcpu16_get(computer, a_word);
			b_val = dcpu16_get(computer, b_word);

			if((DCPU16_WORD_SIGNED) b_val <= (DCPU16_WORD_SIGNED) a_val)
			{
				cycles += dcpu16_skip_next_instruction(computer);
				cycles++;
			}

			break;
		case DCPU16_OPCODE_IFL:
			cycles += 2;

			a_val = dcpu16_get(computer, a_word);
			b_val = dcpu16_get(computer, b_word);

			if(b_val >= a_val)
			{
				cycles += dcpu16_skip_next_instruction(computer);
				cycles++;
			}

			break;
		case DCPU16_OPCODE_IFU:
			cycles += 2;
			
			a_val = dcpu16_get(computer, a_word);
			b_val = dcpu16_get(computer, b_word);


			if((DCPU16_WORD_SIGNED) b_val >= (DCPU16_WORD_SIGNED) a_val)
			{
				cycles += dcpu16_skip_next_instruction(computer);
				cycles++;
			}

			break;
		case DCPU16_OPCODE_ADX:
			cycles += 3;

			a_val = dcpu16_get(computer, a_word);
			b_val = dcpu16_get(computer, b_word);

			if((int) b_val + (int) a_val + (int) computer->registers[DCPU16_INDEX_REG_EX] > 0xFFFF) {
				dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_EX], 1);
			} else {
				dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_EX], 0);
			}

			dcpu16_set(computer, b_word, b_val + a_val + computer->registers[DCPU16_INDEX_REG_EX]);	

			break;
		case DCPU16_OPCODE_SBX:
			cycles += 3;

			a_val = dcpu16_get(computer, a_word);
			b_val = dcpu16_get(computer, b_word);

			if((int) b_val - (int) a_val + (int) computer->registers[DCPU16_INDEX_REG_EX] < 0) {
				dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_EX], 1);
			} else {
				dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_EX], 0);
			}

			dcpu16_set(computer, b_word, b_val - a_val + computer->registers[DCPU16_INDEX_REG_EX]);	

			break;
		case DCPU16_OPCODE_STI:
			cycles += 2;

			dcpu16_set(computer, b_word, dcpu16_get(computer, a_word));
			dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_I], dcpu16_get(computer, &computer->registers[DCPU16_INDEX_REG_I]) + 1);
			dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_J], dcpu16_get(computer, &computer->registers[DCPU16_INDEX_REG_J]) + 1);

			break;
		case DCPU16_OPCODE_STD:
			cycles += 2;

			dcpu16_set(computer, b_word, dcpu16_get(computer, a_word));
			dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_I], dcpu16_get(computer, &computer->registers[DCPU16_INDEX_REG_I]) - 1);
			dcpu16_set(computer, &computer->registers[DCPU16_INDEX_REG_J], dcpu16_get(computer, &computer->registers[DCPU16_INDEX_REG_J]) - 1);

			break;
		case DCPU16_OPCODE_UNOFFICIAL_BREAKPOINT:
			if(computer->allow_breakpoints)
				computer->on_breakpoint = 1;
			break;
		};
	}
	// Call the PC callback
	dcpu16_pc_callback(computer);

	return cycles;
}

/* Displays the contents of the registers. */
void dcpu16_print_registers(dcpu16_t *computer)
{
	PRINTF("--------------------------------------------------------------\n");
	PRINTF("a:%x b:%x c:%x x:%x y:%x z:%x i:%x j:%x pc:%x sp:%x ex:%x ia:%x\n", 
		computer->registers[DCPU16_INDEX_REG_A], computer->registers[DCPU16_INDEX_REG_B], computer->registers[DCPU16_INDEX_REG_C],
		computer->registers[DCPU16_INDEX_REG_X], computer->registers[DCPU16_INDEX_REG_Y], computer->registers[DCPU16_INDEX_REG_Z],
		computer->registers[DCPU16_INDEX_REG_I], computer->registers[DCPU16_INDEX_REG_J], computer->registers[DCPU16_INDEX_REG_PC],
		computer->registers[DCPU16_INDEX_REG_SP], computer->registers[DCPU16_INDEX_REG_EX], computer->registers[DCPU16_INDEX_REG_IA]);
	PRINTF("--------------------------------------------------------------\n");
}

/* Prints the contents of the RAM. */
void dcpu16_dump_ram(dcpu16_t *computer, DCPU16_WORD start, DCPU16_WORD end)
{
	// Align start and end addresses to multiples of 8
	if(start % 8 != 0) {
		int tmp = start / 8;
		start = tmp *8;
	}

	if(end % 8 != 0) {
		int tmp = end / 8;
		end = (tmp + 1) *8;
	}

	// Let the printing begin
	PRINTF("\nRAM DUMP\n");

	for(; start <= end; start+=8) {
		PRINTF("%.4x:", start);

		for(int i = 0; i < 8; i++) {
			DCPU16_WORD w = computer->ram[start + i];
			PRINTF("%.4x ", w);
		}

		putchar('\n');
	}

	putchar('\n');
}

/* Initializes the emulator (clears the registers and RAM) */
void dcpu16_init(dcpu16_t *computer)
{
	memset(computer, 0 , sizeof(*computer));
	computer->trigger_interrupts = 1;
	computer->allow_breakpoints = 1;
}

/* Loads a program into the RAM, returns true on success.
   If binary is false the file is opened as a binary file and it expects the 16-bit integers to be stored in little endian order. */
int dcpu16_load_ram(dcpu16_t *computer, const char *file, char binary, char little_endian)
{
	FILE *f;
	DCPU16_WORD * ram_p = computer->ram;

	// Open the file for reading
	if(binary)
		f = fopen(file, "rb");
	else
		f = fopen(file, "r");

	// Make sure it's open
	if(!f)
		return 0;

	// Read the contents into RAM
	while(!feof(f)) {
		// Make sure we can fit more in RAM
		if(ram_p > computer->ram + DCPU16_RAM_SIZE) {
			fclose(f);
			return 0;
		}

		// Read and put in RAM
		DCPU16_WORD w;

		if(binary) {
			if(little_endian) {
				w = fgetc(f);
				w = w | (fgetc(f) << 8);
			} else {
				w = fgetc(f) << 8;
				w += fgetc(f);
			}
		} else {
			fscanf(f, "%hx", &w);
		}

		*ram_p = w;
		ram_p++;
	}

	fclose(f);

	// Calculate the number of words loaded
	DCPU16_WORD words_loaded = ram_p - computer->ram;
	PRINTF("Loaded %d words into RAM\n", (int)words_loaded);
	
	return 1;
}


/* Reads one character using getchar() and does the following depending on the character read:
   r - prints the contents of the registers
   d - ram dump
   Return value is 0 if the read character has been handled by this function, otherwise the character is returned. */
static char dcpu16_explore_state(dcpu16_t *computer)
{
	char c = getchar();

	if(c == 'r') {
		dcpu16_print_registers(computer);
	} else if(c == 'd') {
		DCPU16_WORD d_start;
		DCPU16_WORD d_end;

		PRINTF("\nRAM dump start address (hex): 0x");
		scanf("%hx", &d_start);
		PRINTF("RAM dump end address (hex): 0x");
		scanf("%hx", &d_end);

		dcpu16_dump_ram(computer, d_start, d_end);
	} else {
		return c;
	}

	return 0;
}


void dcpu16_run_debug(dcpu16_t *computer)
{
	computer->allow_breakpoints = 0;

	PRINTF("DCPU16 emulator now running in debug mode\n"
		"\tType 's' to execute the next instruction\n"
		"\tType 'r' to print the contents of the registers\n"
		"\tType 'd' to display what's in the RAM\n"
		"\tType 'q' to quit\n\n");

	char c = 0;
	while(c != 'q') {
		c = dcpu16_explore_state(computer);

		if(c == 's') {
			// Step
			int pc_before = computer->registers[DCPU16_INDEX_REG_PC];
			int cycles = dcpu16_step(computer);
			PRINTF("pc: %.4x | instruction: %.4x | cycles: %d | pc afterwards: %.4x\t\n\n",
				pc_before, computer->ram[pc_before], cycles, computer->registers[DCPU16_INDEX_REG_PC]);

			// Trigger interrupts
			dcpu16_trigger_interrupt(computer);
		}
	}
}

static void dcpu16_profiler_step(dcpu16_t *computer)
{
	computer->profiling.instruction_count++;
	
	if ((computer->profiling.instruction_count % 1000) == 0) {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		double now = (double)tv.tv_sec + ((double)tv.tv_usec *0.000001);
		
		// If sampling was just enabled, then show first sample sample_frequency seconds from now
		if (computer->profiling.sample_time == 0.0)
			computer->profiling.sample_time = now;
		
		double sample_elapsed = now - computer->profiling.sample_time;
		if (sample_elapsed >= computer->profiling.sample_frequency) {
			// Time since last sample was taken
			double instructions_per_second = (double)computer->profiling.instruction_count / sample_elapsed;
			
			PRINTF("[ PROFILE ]\nSample Duration: %.3lf\nInstructions: %u\nMHz: %.2lf\n-----------\n",
				   sample_elapsed, computer->profiling.instruction_count, (instructions_per_second / 1000000.0));
			
			// Reset instruction count
			computer->profiling.instruction_count = 0;
			
			// Remember when this sample was taken (for next time)
			computer->profiling.sample_time = now;
		}
	}
}

void dcpu16_run(dcpu16_t *computer, unsigned int hertz)
{
	computer->running = 1;
	PRINTF("DCPU16 emulator now running\n");

	// Clock frequency capped?
	char capped = 0;
	unsigned int us_per_instruction = 0;
	
	if(hertz > 0) {
		capped = 1;
		us_per_instruction = (unsigned int) (1000000.0f / hertz);
	} 

	while(computer->running) {
		// Get start time
		struct timeval tv;
		unsigned int start_time;
		unsigned int end_time;

		if(capped) {
			gettimeofday(&tv, NULL);
			start_time = (unsigned int) tv.tv_sec * 1000000 + tv.tv_usec;
		}

		// Step and trigger interrupts
		dcpu16_step(computer);
		dcpu16_trigger_interrupt(computer);

		// Get end time
		if(capped) {
			gettimeofday(&tv, NULL);
			end_time = (unsigned int) tv.tv_sec * 1000000 + tv.tv_usec;

			// Sleep?
			int to_sleep = us_per_instruction - (end_time - start_time);
			
			if(to_sleep > 0)
				usleep(to_sleep);
			
		}

		// Are we on a breakpoint?
		if(computer->on_breakpoint == 1) {
			// Let the user explore the state
			PRINTF("\nBREAKPOINT DETECTED\nYou can now explore the state of the machine\n"
			"\tType 'r' to print the contents of the registers\n"
			"\tType 'd' to display what's in the RAM\n"
			"\tType 'c' to continue execution\n"
		       	"\tType 'q' to quit\n\n");

			char c = 0;
			while(c != 'q' && c != 'c') {
				c = dcpu16_explore_state(computer);
			}

			if(c == 'q') {
				computer->running = 0;
				return;
			} else if(c == 'c') {
				computer->on_breakpoint = 0;
				printf("CONTINUING EXECUTION\n\n");
			}

		}

		// Profiling
		if (computer->profiling.enabled != 0)
			dcpu16_profiler_step(computer);
	}

	PRINTF("Emulator halted\n\n");

	// Let the user explore the state
	PRINTF("\nYou can now explore the state of the machine\n"
		"\tType 'r' to print the contents of the registers\n"
		"\tType 'd' to display what's in the RAM\n"
	       	"\tType 'q' to quit\n\n");

	char c = 0;
	while(c != 'q') {
		c = dcpu16_explore_state(computer);
	}
}
