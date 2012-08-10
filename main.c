#include <stdio.h>
#include <string.h>
#include "SDL/SDL.h"
#include "dcpu16.h"
#include "lem1802.h"
#include "keyboard.h"
#include "clock.h"

const int scale = 4;
const int borderThickness = 50;

int width;
int height;

volatile char running = 1;

// SDL
SDL_Surface *screen;
pthread_t sdl_loop_thread;

// Emulator
dcpu16_t computerInRAM;
dcpu16_t *computer = &computerInRAM;

// Hardware
dcpu16_hardware_t *screen_device;
LEM1802_t *lem1802;

dcpu16_hardware_t *keyboard_device;
dcpu16_hardware_t *clock_device;

int SDL_setup()
{
	width = 128 * scale;
	height = 96 * scale;

	// Initialize SDL
	if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		printf("SDL_Init failed");
		return -1;
	}

	// Create the SDL surface
	screen = SDL_SetVideoMode(width + borderThickness * 2, height + borderThickness * 2, 32, SDL_DOUBLEBUF | SDL_SWSURFACE);
	if(screen == 0) {
		printf("SDL_SetVideoMode failed");
		return -1;
	}

	// Needed to get ascii code from input
	SDL_EnableUNICODE(1);

	return 0;
}

int emulator_setup(char * ram_file, char binary, char enable_profiling)
{
	// Create the DCPU16 virtual computer
	dcpu16_init(computer);

	// Load the RAM file
	if(!dcpu16_load_ram(computer, ram_file, binary)) {
		PRINTF("Couldn't load RAM file (too large or bad file).\n");
		return -1;
	}

	// Profiling
	if(enable_profiling) {
		// Enable profiling
		computer->profiling.enabled = 1;
		
		// Show sample data every 1.0 seconds
		computer->profiling.sample_frequency = 1.0;
	}

	return 0;	
}

int hardware_setup()
{
	// Point the hardware to device slots
	screen_device = &computer->hardware[0];
	keyboard_device = &computer->hardware[1];
	clock_device = &computer->hardware[2];

	// Create the LEM1802 device
	lem1802_create(screen_device, computer);
	lem1802 = (LEM1802_t *) screen_device->custom_struct;

	// Create the keyboard device
	keyboard_create(keyboard_device, computer);

	// Create the clock device
	clock_create(clock_device, computer);

	return 0;
}

void hardware_release()
{
	// Release screen device
	lem1802_destroy(screen_device);

	// Release keyboard device
	keyboard_destroy(keyboard_device);

	// Release clock device
	clock_destroy(clock_device);
}

void loop_sdl(void *arg)
{
	// SDL event for watching if the window is trying to be closed
	SDL_Event event;

	// Create the border rectangles
	SDL_Rect topBorder, bottomBorder, rightBorder, leftBorder;
	
	topBorder.w = width + borderThickness * 2;
	topBorder.h = borderThickness;
	topBorder.x = 0;
	topBorder.y = 0;

	bottomBorder.w = width + borderThickness * 2;
	bottomBorder.h = borderThickness;
	bottomBorder.x = 0;
	bottomBorder.y = height + borderThickness;

	rightBorder.w = borderThickness;
	rightBorder.h = height;
	rightBorder.x = width + borderThickness;
	rightBorder.y = borderThickness;

	leftBorder.w = borderThickness;
	leftBorder.h = height;
	leftBorder.x = 0;
	leftBorder.y = borderThickness;

	// Local copies of the video ram, font data and palette data
	DCPU16_WORD video_ram[LEM1802_VIDEO_RAM_SIZE];
	DCPU16_WORD font_data[LEM1802_FONT_DATA_SIZE];
	DCPU16_WORD palette_data[LEM1802_PALETTE_DATA_SIZE];

	while(running) {
		char sendInterrupt = 0;

		// Check for SDL events		
		while(SDL_PollEvent(&event)) {
			if(event.type == SDL_QUIT) {
				// Tell the emulator to stop running, but do not exit
				computer->running = 0;
				continue;
			}

			char eventHappened = 1;

			switch(event.type) {
				case SDL_KEYDOWN:
					if(event.key.keysym.sym == SDLK_BACKSPACE) {
						keyboard_update_key_state(keyboard_device, KEYBOARD_KEY_BACKSPACE, 1);
						keyboard_key_typed(keyboard_device, KEYBOARD_KEY_BACKSPACE);
					} else if(event.key.keysym.sym == SDLK_RETURN) {
						keyboard_update_key_state(keyboard_device, KEYBOARD_KEY_RETURN, 1);
						keyboard_key_typed(keyboard_device, KEYBOARD_KEY_RETURN);
					} else if(event.key.keysym.sym == SDLK_INSERT) {
						keyboard_update_key_state(keyboard_device, KEYBOARD_KEY_INSERT, 1);
						keyboard_key_typed(keyboard_device, KEYBOARD_KEY_INSERT);
					} else if(event.key.keysym.sym == SDLK_DELETE) {
						keyboard_update_key_state(keyboard_device, KEYBOARD_KEY_DELETE, 1);
						keyboard_key_typed(keyboard_device, KEYBOARD_KEY_DELETE);
					} else if(event.key.keysym.unicode > 0x20 && event.key.keysym.unicode <= 0x7F) {
						keyboard_update_key_state(keyboard_device, event.key.keysym.unicode, 1);
						keyboard_key_typed(keyboard_device, event.key.keysym.unicode);
					} else if(event.key.keysym.sym == SDLK_UP) {
						keyboard_update_key_state(keyboard_device, KEYBOARD_KEY_ARROW_UP, 1);
						keyboard_key_typed(keyboard_device, KEYBOARD_KEY_ARROW_UP);
					} else if(event.key.keysym.sym == SDLK_DOWN) {
						keyboard_update_key_state(keyboard_device, KEYBOARD_KEY_ARROW_DOWN, 1);
						keyboard_key_typed(keyboard_device, KEYBOARD_KEY_ARROW_DOWN);
					} else if(event.key.keysym.sym == SDLK_LEFT) {
						keyboard_update_key_state(keyboard_device, KEYBOARD_KEY_ARROW_LEFT, 1);
						keyboard_key_typed(keyboard_device, KEYBOARD_KEY_ARROW_LEFT);
					} else if(event.key.keysym.sym == SDLK_RIGHT) {
						keyboard_update_key_state(keyboard_device, KEYBOARD_KEY_ARROW_RIGHT, 1);
						keyboard_key_typed(keyboard_device, KEYBOARD_KEY_ARROW_RIGHT);
					} else if(event.key.keysym.sym == SDLK_LSHIFT) {
						keyboard_update_key_state(keyboard_device, KEYBOARD_KEY_SHIFT, 1);
						keyboard_key_typed(keyboard_device, KEYBOARD_KEY_SHIFT);
					} else if(event.key.keysym.sym == SDLK_LCTRL) {
						keyboard_update_key_state(keyboard_device, KEYBOARD_KEY_CONTROL, 1);
						keyboard_key_typed(keyboard_device, KEYBOARD_KEY_CONTROL);
					} else {
						eventHappened = 0;
					}

					break;
				case SDL_KEYUP:
					if(event.key.keysym.sym == SDLK_BACKSPACE) {
						keyboard_update_key_state(keyboard_device, KEYBOARD_KEY_BACKSPACE, 0);
					} else if(event.key.keysym.sym == SDLK_RETURN) {
						keyboard_update_key_state(keyboard_device, KEYBOARD_KEY_RETURN, 0);
					} else if(event.key.keysym.sym == SDLK_INSERT) {
						keyboard_update_key_state(keyboard_device, KEYBOARD_KEY_INSERT, 0);
					} else if(event.key.keysym.sym == SDLK_DELETE) {
						keyboard_update_key_state(keyboard_device, KEYBOARD_KEY_DELETE, 0);
					} else if(event.key.keysym.unicode > 0x20 && event.key.keysym.unicode <= 0x7F) {
						keyboard_update_key_state(keyboard_device, event.key.keysym.unicode, 0);
					} else if(event.key.keysym.sym == SDLK_UP) {
						keyboard_update_key_state(keyboard_device, KEYBOARD_KEY_ARROW_UP, 0);
					} else if(event.key.keysym.sym == SDLK_DOWN) {
						keyboard_update_key_state(keyboard_device, KEYBOARD_KEY_ARROW_DOWN, 0);
					} else if(event.key.keysym.sym == SDLK_LEFT) {
						keyboard_update_key_state(keyboard_device, KEYBOARD_KEY_ARROW_LEFT, 0);
					} else if(event.key.keysym.sym == SDLK_RIGHT) {
						keyboard_update_key_state(keyboard_device, KEYBOARD_KEY_ARROW_RIGHT, 0);
					} else if(event.key.keysym.sym == SDLK_LSHIFT) {
						keyboard_update_key_state(keyboard_device, KEYBOARD_KEY_SHIFT, 0);
					} else if(event.key.keysym.sym == SDLK_LCTRL) {
						keyboard_update_key_state(keyboard_device, KEYBOARD_KEY_CONTROL, 0);
					} else {
						eventHappened = 0;
					}

					break;
				default:
					eventHappened = 0;
			};

			if(eventHappened)
				sendInterrupt = 1;
		}

		if(sendInterrupt)
			keyboard_throw_interrupt(keyboard_device);

		// Get the border color
		unsigned int borderColor = SDL_MapRGB(screen->format, ((palette_data[lem1802->border_color & 0xF] >> 8) & 0xF) * 0xF,
											((palette_data[lem1802->border_color & 0xF] >> 4) & 0xF) * 0xF,
											(palette_data[lem1802->border_color & 0xF] & 0xF) * 0xF);

		// Draw the border
		SDL_FillRect(screen, &topBorder, borderColor);
		SDL_FillRect(screen, &bottomBorder, borderColor);
		SDL_FillRect(screen, &rightBorder, borderColor);
		SDL_FillRect(screen, &leftBorder, borderColor);

		// Refresh local data
		lem1802_copy_video_ram(screen_device, video_ram);
		lem1802_copy_font_data(screen_device, font_data);
		lem1802_copy_palette_data(screen_device, palette_data);

		// Draw the content
		for(int y = 0; y < 12; y++) {
			for(int x = 0; x < 32; x++) {
				DCPU16_WORD *w = &video_ram[y * 32 + x];

				// Extract the data
				char character = *w & 0x7F;
				char blinking = (*w >> 7) & 1;
				char background = (*w >> 8) & 0xF;
				char foreground = (*w >> 12) & 0xF;

				// Create the colors
				DCPU16_WORD backgroundColorWord = palette_data[background];
				DCPU16_WORD foregroundColorWord = palette_data[foreground];

				unsigned int backgroundColor = SDL_MapRGB(screen->format, ((backgroundColorWord >> 8) & 0xF) * 0xF,
											((backgroundColorWord >> 4) & 0xF) * 0xF,
											(backgroundColorWord & 0xF) * 0xF);

				unsigned int foregroundColor = SDL_MapRGB(screen->format, ((foregroundColorWord >> 8) & 0xF) * 0xF,
											((foregroundColorWord >> 4) & 0xF) * 0xF,
											(foregroundColorWord & 0xF) * 0xF);

				// Draw the background color
				SDL_Rect backgroundRect;

				backgroundRect.w = 4 * scale;
				backgroundRect.h = 8 * scale;
				backgroundRect.x = borderThickness + x * 4 * scale;
				backgroundRect.y = borderThickness + y * 8 * scale;
				
				SDL_FillRect(screen, &backgroundRect, backgroundColor);

				// Draw the character pixels
				unsigned int pixelData = font_data[character * 2] << 16 | font_data[character * 2 + 1];
				int bit = 31;
				for(int pixelX = 0; pixelX < 4; pixelX++) {
					for(int pixelY = 7; pixelY >= 0; pixelY--) {
						if((pixelData >> bit) & 1) {
							// Draw the pixel rect			
							SDL_Rect pixelRect;
							pixelRect.w = scale;
							pixelRect.h = scale;
							pixelRect.x = backgroundRect.x + pixelX * scale;
							pixelRect.y = backgroundRect.y + pixelY * scale;

							SDL_FillRect(screen, &pixelRect, foregroundColor); 
						}

						bit--;
					}
				}

			}
		}

		// Update the screen
		SDL_Flip(screen);
	}

	pthread_exit(0);
}

int main(int argc, char *argv[]) 
{
	// Setup SDL
	if(SDL_setup() != 0)
		return -1;

	// Command line arguments
	char *ram_file 		= 0;
	char binary 		= 0;
	char debug_mode 	= 0;
	char enable_profiling 	= 0;
	
	// Parse the arguments
	for(int c = 1; c < argc; c++) {
		if(strcmp(argv[c], "-d") == 0) {
			debug_mode = 1;
		} else if(strcmp(argv[c], "-b") == 0) {
			binary = 1;
		} else if(strcmp(argv[c], "-p") == 0) {
			enable_profiling = 1;
		} else {
			ram_file = argv[c];
		}
	}

	// Make sure the user specified a RAM file
	if(ram_file) {
		// Prepare the emulator
		if(emulator_setup(ram_file, binary, enable_profiling) != 0)
			return -1;
	} else {
		PRINTF("No RAM file specified.\n");
		return 0;
	}
	
	// Setup hardware
	hardware_setup();

	// Start the screen thread
	pthread_create(&sdl_loop_thread, 0, (void *) &loop_sdl, 0);

	// Start the emulator
	if(debug_mode)
		dcpu16_run_debug(computer);
	else
		dcpu16_run(computer, 1000000);

	// Wait for the screen thread to finish
	running = 0;
	pthread_join(sdl_loop_thread, 0);

	// Cleanup
	hardware_release();

	// Quit SDL
	SDL_Quit();

	return 0;
}
