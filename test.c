/* NOTE: People using this project as a base for their own emulator GUI etc should implement their own main function (in a seperate file) and also change the MAIN variable in the Makefile. */

#include <stdio.h>
#include <string.h>
#include "dcpu16.h"

int main(int argc, char *argv[]) 
{
	dcpu16_t computerOnTheStack;
	dcpu16_t *computer = &computerOnTheStack;
	dcpu16_init(computer);

	// Command line arguments
	char *ram_file 	= 0;
	char binary_ram_file 	= 0;
	char debug_mode 	= 0;
	char enable_profiling 	= 0;
	
	// Parse the arguments
	for(int c = 1; c < argc; c++) {
		if(strcmp(argv[c], "-d") == 0) {
			debug_mode = 1;
		} else if(strcmp(argv[c], "-b") == 0) {
			binary_ram_file = 1;
		} else if(strcmp(argv[c], "-p") == 0) {
			enable_profiling = 1;
		} else {
			ram_file = argv[c];
		}
	}

	// Load RAM file
	if(ram_file) {
		if(!dcpu16_load_ram(computer, ram_file, binary_ram_file)) {
			PRINTF("Couldn't load RAM file (too large or bad file).\n");
			return 0;
		}
	} else {
		PRINTF("No RAM file specified.\n");
		return 0;
	}

	// Profiling
	if(enable_profiling) {
		// Enable profiling
		computer->profiling.enabled = 1;
		
		// Show sample data every 1.0 seconds
		computer->profiling.sample_frequency = 1.0;
	}

	// Start the emulator
	if(debug_mode)
		dcpu16_run_debug(computer);
	else
		dcpu16_run(computer, 0);
	
	return 0;
}
