
#include <stdint.h>

#define CHIP8_STACK_SIZE 16
#define CHIP8_MEM_SIZE 4096

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

	uint8_t V[CHIP8_STACK_SIZE]; // data registers

	uint16_t I; // address register
	uint16_t pc; // program counter

	// Wikipedia mentions originally this was 48 bytes, but most use 16 now
	uint16_t stack[16];
	uint16_t sp; // stack pointer

	uint8_t delay_timer; //60Hz
	uint8_t sound_timer; //60Hz

	// TODO gfx, sound, keyboard
};


// Public functions
void chip8_initialize(struct chip8_hw *chip);

void chip8_decode_opcode(struct chip8_hw *chip, uint16_t pc);


// Private functions
void chip8_build_sprites(struct chip8_hw *chip);
