#include <stdlib.h>
#include <string.h>
#include "lem1802.h"

DCPU16_WORD lem1802_default_font[LEM1802_FONT_DATA_SIZE] = { 47006, 14478, 29228, 30196, 6587, 32655, 34297, 45400, 9262, 9216, 2090, 2048, 8, 0, 2056, 2056, 255, 0, 248, 2056, 2296, 0, 2063, 0, 15, 2056, 255, 2056, 2296, 2056, 2303, 0, 2063, 2056, 2303, 2056, 26163, 39372, 39219, 26316, 65272, 57472, 32543, 1793, 263, 8063, 32992, 63742, 21760, 43520, 21930, 21930, 65450, 65365, 3855, 3855, 61680, 61680, 0, 65535, 65535, 0, 65535, 65535, 0, 0, 95, 0, 768, 768, 15892, 15872, 9835, 12800, 24860, 17152, 13865, 30288, 2, 256, 7202, 16640, 16674, 7168, 5128, 5120, 2076, 2048, 16416, 0, 2056, 2048, 64, 0, 24604, 768, 15945, 15872, 17023, 16384, 25177, 17920, 8777, 13824, 3848, 32512, 10053, 14592, 15945, 12800, 24857, 1792, 13897, 13824, 9801, 15872, 36, 0, 16420, 0, 2068, 8769, 5140, 5120, 16674, 5128, 601, 1536, 15961, 24064, 32265, 32256, 32585, 13824, 15937, 8704, 32577, 15872, 32585, 16640, 32521, 256, 15937, 31232, 32520, 32512, 16767, 16640, 8256, 16128, 32520, 30464, 32576, 16384, 32518, 32512, 32513, 32256, 15937, 15872, 32521, 1536, 15937, 48640, 32521, 30208, 9801, 12800, 383, 256, 16192, 16128, 8032, 7936, 32560, 32512, 30472, 30464, 1912, 1792, 29001, 18176, 127, 16640, 796, 24576, 65, 32512, 513, 512, 32896, 32768, 1, 512, 9300, 30720, 32580, 14336, 14404, 10240, 14404, 32512, 14420, 22528, 2174, 2304, 18516, 15360, 32516, 30720, 17533, 16384, 8256, 15616, 32528, 27648, 16767, 16384, 31768, 31744, 31748, 30720, 14404, 14336, 31764, 2048, 2068, 31744, 31748, 2048, 18516, 9216, 1086, 17408, 15424, 31744, 7264, 7168, 31792, 31744, 27664, 27648, 19536, 15360, 25684, 19456, 2102, 16640, 119, 0, 16694, 2048, 513, 513, 517, 512 };

DCPU16_WORD lem1802_default_palette[LEM1802_PALETTE_DATA_SIZE] = { 0x0000, 0x000a, 0x00a0, 0x00aa, 0x0a00, 0x0a0a, 0x0a50, 0x0aaa, 0x0555, 0x055f, 0x05f5, 0x05ff, 0x0f55, 0x0f5f, 0x0ff5, 0x0fff };

static int lem1802_interrupt(dcpu16_hardware_t *hardware)
{
	LEM1802_t *lem = (LEM1802_t *) hardware->custom_struct;

	// Read the A and B registers
	DCPU16_WORD reg_a = lem->computer->registers[DCPU16_INDEX_REG_A];
	DCPU16_WORD reg_b = lem->computer->registers[DCPU16_INDEX_REG_B];

	// Handle the interrupt
	switch(reg_a) {
	case LEM1802_INC_INT_MEM_MAP_SCREEN:
		if(reg_b == 0) {
			lem->connected = 0;
		} else {
			lem->connected = 1;
			lem->video_ram_address = reg_b;
		}

		break;
	case LEM1802_INC_INT_MEM_MAP_FONT:
		if(reg_b == 0) {
			lem->font_mapped = 0;
		} else {
			lem->font_mapped = 1;
			lem->font_ram_address = reg_b;
		}

		break;
	case LEM1802_INC_INT_MEM_MAP_PALETTE:
		if(reg_b == 0) {
			lem->palette_mapped = 0;
		} else {
			lem->palette_mapped = 1;
			lem->palette_ram_address = reg_b;
		}

		break;
	case LEM1802_INC_INT_SET_BORDER_COLOR:
		lem->border_color = reg_b & 0xF;

		break;
	case LEM1802_INC_INT_MEM_DUMP_FONT:
		{
			DCPU16_WORD address = reg_b;
			for(int w = 0; w < sizeof(lem1802_default_font); w++, address++) {
				lem->computer->ram[address] = lem1802_default_font[w];
			}
		}

		return 256;
	case LEM1802_INC_INT_MEM_DUMP_PALETTE:
		{
			DCPU16_WORD address = reg_b;
			for(int w = 0; w < sizeof(lem1802_default_palette); w++, address++) {
				lem->computer->ram[address] = lem1802_default_palette[w];
			}
		}

		return 16;
	};

	return 0;
}

void lem1802_copy_video_ram(dcpu16_hardware_t *hardware, DCPU16_WORD * destination)
{
	LEM1802_t *lem = (LEM1802_t *) hardware->custom_struct;

	if(lem->connected) {
		if(lem->video_ram_address + LEM1802_VIDEO_RAM_SIZE < DCPU16_RAM_SIZE) {
			// One memcpy necessary
			memcpy(destination, &lem->computer->ram[lem->video_ram_address], LEM1802_VIDEO_RAM_SIZE * 2);
		} else {
			// Video RAM uses the wrap around trick, two memcpy's necessary
			memcpy(destination, &lem->computer->ram[lem->video_ram_address], (DCPU16_RAM_SIZE - LEM1802_VIDEO_RAM_SIZE) * 2);
			memcpy(destination, &lem->computer->ram[0], (LEM1802_VIDEO_RAM_SIZE - (DCPU16_RAM_SIZE - LEM1802_VIDEO_RAM_SIZE)) * 2);
		}
	}
}

void lem1802_copy_font_data(dcpu16_hardware_t *hardware, DCPU16_WORD * destination)
{
	LEM1802_t *lem = (LEM1802_t *) hardware->custom_struct;

	if(lem->font_mapped) {
		if(lem->font_ram_address + LEM1802_FONT_DATA_SIZE < DCPU16_RAM_SIZE) {
			// One memcpy necessary
			memcpy(destination, &lem->computer->ram[lem->font_ram_address], LEM1802_FONT_DATA_SIZE * 2);
		} else {
			// Video RAM uses the wrap around trick, two memcpy's necessary
			memcpy(destination, &lem->computer->ram[lem->font_ram_address], (DCPU16_RAM_SIZE - LEM1802_FONT_DATA_SIZE) * 2);
			memcpy(destination, &lem->computer->ram[0], (LEM1802_FONT_DATA_SIZE - (DCPU16_RAM_SIZE - LEM1802_FONT_DATA_SIZE)) * 2);
		}
	} else {
		memcpy(destination, lem1802_default_font, sizeof(lem1802_default_font));
	}
}

void lem1802_copy_palette_data(dcpu16_hardware_t *hardware, DCPU16_WORD * destination)
{
	LEM1802_t *lem = (LEM1802_t *) hardware->custom_struct;

	if(lem->palette_mapped) {
		if(lem->palette_ram_address + LEM1802_PALETTE_DATA_SIZE < DCPU16_RAM_SIZE) {
			// One memcpy necessary
			memcpy(destination, &lem->computer->ram[lem->palette_ram_address], LEM1802_PALETTE_DATA_SIZE * 2);
		} else {
			// Video RAM uses the wrap around trick, two memcpy's necessary
			memcpy(destination, &lem->computer->ram[lem->palette_ram_address], (DCPU16_RAM_SIZE - LEM1802_PALETTE_DATA_SIZE) * 2);
			memcpy(destination, &lem->computer->ram[0], (LEM1802_PALETTE_DATA_SIZE - (DCPU16_RAM_SIZE - LEM1802_PALETTE_DATA_SIZE)) * 2);
		}
	} else {
		memcpy(destination, lem1802_default_palette, sizeof(lem1802_default_palette));
	}
}

void lem1802_create(dcpu16_hardware_t *hardware, dcpu16_t *computer)
{
	// Create a LEM structure
	LEM1802_t *lem = malloc(sizeof(LEM1802_t));
	memset(lem, 0, sizeof(LEM1802_t));

	lem->computer = computer;
	lem->border_color = LEM1802_DEFAULT_BORDER_COLOR;

	// Set default vid ram address
	lem->video_ram_address = 0x8000;
	lem->connected = 1;

	// Set the hardware structure
	hardware->present = 1;
	hardware->hardware_id = LEM1802_ID;
	hardware->hardware_version = LEM1802_VERSION;
	hardware->hardware_manufacturer = LEM1802_MANUFACTURER;
	hardware->interrupt = lem1802_interrupt;
	hardware->custom_struct = (void *) lem;
}

void lem1802_destroy(dcpu16_hardware_t *hardware)
{
	// Free the LEM structure
	free(hardware->custom_struct);

	// Set the present indicator to 0
	hardware->present = 0;
}
