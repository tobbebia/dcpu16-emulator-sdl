#ifndef LEM1802_H
#define LEM1802_H

#include "dcpu16.h"

#define LEM1802_ID		0x7349f615
#define LEM1802_VERSION		0x1802
#define LEM1802_MANUFACTURER	0x1c6c8b36

#define LEM1802_INC_INT_MEM_MAP_SCREEN		1
#define LEM1802_INC_INT_MEM_MAP_FONT		2
#define LEM1802_INC_INT_MEM_MAP_PALETTE		3
#define LEM1802_INC_INT_SET_BORDER_COLOR	4
#define LEM1802_INC_INT_MEM_DUMP_FONT		5
#define LEM1802_INC_INT_MEM_DUMP_PALETTE	6

// Size measured in DCPU16 words
#define LEM1802_VIDEO_RAM_SIZE		386
#define LEM1802_FONT_DATA_SIZE		256
#define LEM1802_PALETTE_DATA_SIZE	16

DCPU16_WORD lem1802_default_font[LEM1802_FONT_DATA_SIZE];
DCPU16_WORD lem1802_default_palette[LEM1802_PALETTE_DATA_SIZE];

typedef struct _LEM1802_t
{
	char connected;
	DCPU16_WORD video_ram_address;

	char font_mapped;
	DCPU16_WORD font_ram_address;

	char palette_mapped;
	DCPU16_WORD palette_ram_address;

	DCPU16_WORD border_color;

	dcpu16_t *computer;
} LEM1802_t;

void lem1802_create(dcpu16_hardware_t *hardware, dcpu16_t *computer);
void lem1802_destroy(dcpu16_hardware_t *hardware);

void lem1802_copy_video_ram(dcpu16_hardware_t *hardware, DCPU16_WORD * destination);
void lem1802_copy_font_data(dcpu16_hardware_t *hardware, DCPU16_WORD * destination);
void lem1802_copy_palette_data(dcpu16_hardware_t *hardware, DCPU16_WORD * destination);

#endif
