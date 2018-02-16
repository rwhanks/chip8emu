#pragma once

#include <stdint.h>
#include "SDL/SDL.h"

#define CHIP8_STACK_SIZE 16
#define CHIP8_MEM_SIZE 4096
#define CHIP8_DISPLAY_SCALE 10
#define CHIP8_DISPLAY_X 64
#define CHIP8_DISPLAY_Y 32

struct chip8_hw
{
	// Per http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#memmap
	/*
	+---------------+= 0xFFF (4095) End of Chip-8 RAM
  |               |
  | 0x200 to 0xFFF|
  |     Chip-8    |
  | Program / Data|
  |     Space     |
  |               |
  +---------------+= 0x200 (512) Start of most Chip-8 programs
  | 0x000 to 0x1FF|
  | Reserved for  |
  |  interpreter  |
  +---------------+= 0x000 (0) Start of Chip-8 RAM
  */
	uint8_t memory[CHIP8_MEM_SIZE];
	uint16_t rom_size; // size of the ROM currently stored in memory

	uint8_t V[CHIP8_STACK_SIZE]; // data registers

	uint16_t I; // address register
	uint16_t pc; // program counter

	// Wikipedia mentions originally this was 48 bytes, but most use 16 now
	uint16_t stack[16];
	uint16_t sp; // stack pointer

	uint8_t delay_timer; //60Hz
	uint8_t sound_timer; //60Hz

	uint8_t running; // ROM is currently executing

	// TODO gfx, sound, keyboard
	SDL_Surface *screen;

  // the size of the display in scaled pixels
	uint16_t display_x;
	uint16_t display_y;

  // array to hold the display pixels
	uint8_t display_pixels[CHIP8_DISPLAY_X*CHIP8_DISPLAY_SCALE][CHIP8_DISPLAY_Y*CHIP8_DISPLAY_SCALE];

	uint8_t keyboard[16]; //keyboard struct
};


// Public functions
/*
* Initialize the chip8 emulator and build the sprite table
* @param *chip - chip8_hw pointer
*/
void chip8_initialize(struct chip8_hw *chip);

/*
* Emulate 1 cycle of the chip8 hardware including decoding the next opcode
* @param *chip - chip8_hw pointer
*/
void chip8_emulate_cycle(struct chip8_hw *chip);


// Private functions
/*
* Decode an opcode at the current PC -- PC is NOT updated after returning
* @param *chip - chip8_hw pointer
* @param pc - current PC location TODO remove
*/
void chip8_decode_opcode(struct chip8_hw *chip, uint16_t pc);

/*
* Update the chip8 keyboard based on a pressed/released key
* @param *chip - chip8_hw pointer
* @param event - the SDL_Event that was triggered; supported events:SDL_KEYDOWN, SDL_KEYUP
* @param pressed - was the lhe key pressed(1) or released(0)?
*/
void chip8_update_keyboard(struct chip8_hw *chip, SDL_Event event, uint8_t pressed);

/*
* Build the sprite table for the user display
* @param *chip - chip8_hw pointer
*/
void chip8_build_sprites(struct chip8_hw *chip);

/*
* Print the current register state for debugging
* @param *chip - chip8_hw pointer
*/
void chip8_dump_registers(struct chip8_hw *chip);
