#ifndef DCPU16_H
#define DCPU16_H

#include <pthread.h>
#include "config.h"

typedef unsigned short DCPU16_WORD;
typedef signed short DCPU16_WORD_SIGNED;

/* BASIC OPCODES */
#define DCPU16_OPCODE_NON_BASIC			0x00
#define DCPU16_OPCODE_SET			0x01
#define DCPU16_OPCODE_ADD			0x02
#define DCPU16_OPCODE_SUB			0x03
#define DCPU16_OPCODE_MUL			0x04
#define DCPU16_OPCODE_MLI			0x05
#define DCPU16_OPCODE_DIV			0x06
#define DCPU16_OPCODE_DVI			0x07
#define DCPU16_OPCODE_MOD			0x08
#define DCPU16_OPCODE_MDI			0x09
#define DCPU16_OPCODE_AND			0x0A
#define DCPU16_OPCODE_BOR			0x0B
#define DCPU16_OPCODE_XOR			0x0C
#define DCPU16_OPCODE_SHR			0x0D
#define DCPU16_OPCODE_ASR			0x0E
#define DCPU16_OPCODE_SHL			0x0F
#define DCPU16_OPCODE_IFB			0x10
#define DCPU16_OPCODE_IFC			0x11
#define DCPU16_OPCODE_IFE			0x12
#define DCPU16_OPCODE_IFN			0x13
#define DCPU16_OPCODE_IFG			0x14
#define DCPU16_OPCODE_IFA			0x15
#define DCPU16_OPCODE_IFL			0x16
#define DCPU16_OPCODE_IFU			0x17
// 0x18 unused
// 0x19 unused
#define DCPU16_OPCODE_ADX			0x1A
#define DCPU16_OPCODE_SBX			0x1B
// 0x1C unused
// 0x1D unused
#define DCPU16_OPCODE_STI			0x1E
#define DCPU16_OPCODE_STD			0x1F

/* NON BASIC OPCODES */
#define DCPU16_NON_BASIC_OPCODE_RESERVED_0	0x00
#define DCPU16_NON_BASIC_OPCODE_JSR_A		0x01
// 0x02 unused
// 0x03 unused
// 0x04 unused
// 0x05 unused
// 0x06 unused
// 0x07 unused
#define DCPU16_NON_BASIC_OPCODE_INT		0x08
#define DCPU16_NON_BASIC_OPCODE_IAG		0x09
#define DCPU16_NON_BASIC_OPCODE_IAS		0x0A
#define DCPU16_NON_BASIC_OPCODE_RFI		0x0B
#define DCPU16_NON_BASIC_OPCODE_IAQ		0x0C
// 0x0D unused
// 0x0E unused
// 0x0F unused
#define DCPU16_NON_BASIC_OPCODE_HWN		0x10
#define DCPU16_NON_BASIC_OPCODE_HWQ		0x11
#define DCPU16_NON_BASIC_OPCODE_HWI		0x12
// 0x13 unused
// 0x14 unused
// 0x15 unused
// 0x16 unused
// 0x17 unused
// 0x18 unused
// 0x19 unused
// 0x1A unused
// 0x1B unused
// 0x1C unused
// 0x1D unused
// 0x1E unused
// 0x1F unused

/* AB VALUES */
#define DCPU16_AB_VALUE_REG_A			0x00
#define DCPU16_AB_VALUE_REG_B			0x01
#define DCPU16_AB_VALUE_REG_C			0x02
#define DCPU16_AB_VALUE_REG_X			0x03
#define DCPU16_AB_VALUE_REG_Y			0x04
#define DCPU16_AB_VALUE_REG_Z			0x05
#define DCPU16_AB_VALUE_REG_I			0x06
#define DCPU16_AB_VALUE_REG_J			0x07
#define DCPU16_AB_VALUE_PTR_REG_A		0x08
#define DCPU16_AB_VALUE_PTR_REG_B		0x09
#define DCPU16_AB_VALUE_PTR_REG_C		0x0A
#define DCPU16_AB_VALUE_PTR_REG_X		0x0B
#define DCPU16_AB_VALUE_PTR_REG_Y		0x0C
#define DCPU16_AB_VALUE_PTR_REG_Z		0x0D
#define DCPU16_AB_VALUE_PTR_REG_I		0x0E
#define DCPU16_AB_VALUE_PTR_REG_J		0x0F
#define DCPU16_AB_VALUE_PTR_REG_A_PLUS_WORD	0x10
#define DCPU16_AB_VALUE_PTR_REG_B_PLUS_WORD	0x11
#define DCPU16_AB_VALUE_PTR_REG_C_PLUS_WORD	0x12
#define DCPU16_AB_VALUE_PTR_REG_X_PLUS_WORD	0x13
#define DCPU16_AB_VALUE_PTR_REG_Y_PLUS_WORD	0x14
#define DCPU16_AB_VALUE_PTR_REG_Z_PLUS_WORD	0x15
#define DCPU16_AB_VALUE_PTR_REG_I_PLUS_WORD	0x16
#define DCPU16_AB_VALUE_PTR_REG_J_PLUS_WORD	0x17
#define DCPU16_AB_VALUE_PUSH_OR_POP		0x18
#define DCPU16_AB_VALUE_PEEK			0x19
#define DCPU16_AB_VALUE_PICK			0x1A
#define DCPU16_AB_VALUE_REG_SP			0x1B
#define DCPU16_AB_VALUE_REG_PC			0x1C
#define DCPU16_AB_VALUE_REG_EX			0x1D
#define DCPU16_AB_VALUE_PTR_WORD		0x1E
#define DCPU16_AB_VALUE_WORD			0x1F

#define DCPU16_REGISTER_COUNT			12
#define DCPU16_INDEX_REG_A				0
#define DCPU16_INDEX_REG_B				1
#define DCPU16_INDEX_REG_C				2
#define DCPU16_INDEX_REG_X				3
#define DCPU16_INDEX_REG_Y				4
#define DCPU16_INDEX_REG_Z				5
#define DCPU16_INDEX_REG_I				6
#define DCPU16_INDEX_REG_J				7
#define DCPU16_INDEX_REG_PC				8
#define DCPU16_INDEX_REG_SP				9
#define DCPU16_INDEX_REG_EX				10
#define DCPU16_INDEX_REG_IA				11

#define DCPU16_RAM_SIZE 			0x10000
#define DCPU16_HARDWARE_SLOTS			65535
#define DCPU16_MAX_INTERRUPT_QUEUE_LENGTH	256

typedef struct _dcpu16_hardware_t
{
	char present;
	unsigned int hardware_id;
	unsigned short hardware_version;
	unsigned int hardware_manufacturer;
	int (* interrupt)(struct _dcpu16_hardware_t *);
	void *custom_struct;
} dcpu16_hardware_t;

typedef struct _dcpu16_queued_interrupt_t
{
	DCPU16_WORD message;
	struct _dcpu16_queued_interrupt_t *next;
} dcpu16_queued_interrupt_t;

typedef struct _dcpu16_t
{
	// Used for performance profiling
	struct profiling {
		unsigned char enabled;
		double sample_time;
		double sample_frequency;
		double sample_start_time;
		unsigned instruction_count;
	} profiling;

	// Pointers to callback functions
	struct callback {
		void (* register_changed)(unsigned char reg, DCPU16_WORD val);
		void (* ram_changed)(DCPU16_WORD address, DCPU16_WORD val);
	} callback;

	// Registers, RAM and hardware
	DCPU16_WORD registers[DCPU16_REGISTER_COUNT];
	DCPU16_WORD ram[DCPU16_RAM_SIZE];
	dcpu16_hardware_t hardware[DCPU16_HARDWARE_SLOTS];

	// Interrupt queue
	dcpu16_queued_interrupt_t * interrupt_queue;
	dcpu16_queued_interrupt_t * interrupt_queue_end;
	unsigned short interrupt_queue_length;
	unsigned char trigger_interrupts;

	// Mutex for accessing the interrupt queue
	pthread_mutex_t interrupt_queue_mutex;
} dcpu16_t;

/* Declaration of "public" functions */
void dcpu16_interrupt(dcpu16_t *computer, DCPU16_WORD message, char software_interrupt);
void dcpu16_set(dcpu16_t *computer, DCPU16_WORD *where, DCPU16_WORD value);
void dcpu16_init(dcpu16_t *computer);
int dcpu16_load_ram(dcpu16_t *computer, const char *file, char binary);
void dcpu16_run_debug(dcpu16_t *computer);
void dcpu16_run(dcpu16_t *computer);
unsigned char dcpu16_step(dcpu16_t *computer);
void dcpu16_dump_ram(dcpu16_t *computer, DCPU16_WORD start, DCPU16_WORD end);
void dcpu16_print_registers(dcpu16_t *computer);

/* If you want to redirect all console writes, follow the instructions in config.h */
#ifndef PRINTF
	#define PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif // PRINTF

#endif // DCPU16_H
