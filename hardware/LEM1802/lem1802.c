#include <stdlib.h>
#include <string.h>
#include "lem1802.h"

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
		memcpy(destination, lem1802_default_font, LEM1802_FONT_DATA_SIZE * 2);
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
		memcpy(destination, lem1802_default_palette, LEM1802_PALETTE_DATA_SIZE * 2);
	}
}

void lem1802_create(dcpu16_hardware_t *hardware, dcpu16_t *computer)
{
	// Create a LEM structure
	LEM1802_t *lem = malloc(sizeof(LEM1802_t));
	memset(lem, 0, sizeof(LEM1802_t));

	lem->computer = computer;

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
