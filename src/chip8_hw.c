#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "chip8_hw.h"

void chip8_initialize(struct chip8_hw *chip)
{
  memset(chip->memory, 0, CHIP8_MEM_SIZE);
  chip->rom_size = 0;

  memset(chip->V, 0, 0xF); // data registers

  chip->I = 0; // address register
  chip->pc = 0x200; // program counter

  // Wikipedia mentions originally this was 48 bytes, but most use 16 now
  memset(chip->stack, 0, CHIP8_STACK_SIZE);
  chip->sp = 0; // stack pointer

  chip->delay_timer = 0; //60Hz
  chip->sound_timer = 0; //60Hz

  chip8_build_sprites(chip);

  //initialize so we can use it for CXNN opcode
  srand(time(NULL));
}


void chip8_emulate_cycle(struct chip8_hw *chip)
{
  //DEBUG: loop through everything and make sure we are decoding correctly
  int pc = 0x200;
  printf("Chip ROM size: %u\n", chip->rom_size);
  while(pc < chip->rom_size + 0x200)
  {
    chip8_decode_opcode(chip, pc);
    chip8_dump_registers(chip);
    pc += 2;
  }
}


// Private functions
void chip8_decode_opcode(struct chip8_hw *chip, uint16_t pc)
{
  // TODO make this less terrible than a giant switch
  // Do this 1 byte at a time so we don't have to worry about endianness
  uint8_t reg = 0;
  uint16_t value = 0;
  uint8_t i;
  printf("%04X ", pc);
  switch(chip->memory[pc] & 0xF0)
  {
      case 0x00: // couple of operations
      switch(chip->memory[pc + 1])
      {
        case 0x00: //ignored
          printf("SYS addr (Ignored)\n");
          break;
        case 0xE0: //CLS - clear display
          printf("CLS\n");
          //TODO do this
          break;
        case 0xEE: //return from subroutine
          printf("RET\n");
          chip->pc = chip->stack[chip->sp];
          chip->sp--;
          break;
        default:
          printf("Unknown opcode: %04x\n", chip->memory[pc] << 8 | chip->memory[pc + 1]);
          break;
      }
      break;
      case 0x10: //1NNN - jump to address NNN
        value = (chip->memory[pc] & 0x0F) << 8 | chip->memory[pc + 1];
        printf("JMP    $0x%03X\n", value);
        chip->pc = value;
        break;
      case 0x20: //2NNN - call subroutine at NNN
        value = (chip->memory[pc] & 0x0F) << 8 | chip->memory[pc + 1];
        printf("CALL    $0x%03X\n", value);
        chip->sp++;
        chip->stack[chip->sp] = chip->pc;
        chip->pc = value;
        break;
      case 0x30: //3XNN - if VX == NN then skip next instruction
        reg = chip->memory[pc] & 0x0F;
        value = chip->memory[pc + 1];
        printf("SE    V%01X, #%u\n", reg, value);
        if(chip->V[reg] == value)
        {
          chip->pc += 2;
        }
        break;
      case 0x40: //4XNN - if VX != NN then skip next instruction
        reg = chip->memory[pc] & 0x0F;
        value = chip->memory[pc + 1];
        printf("SNE    V%01X, #%u\n", reg, value);
        if(chip->V[reg] != value)
        {
          chip->pc += 2;
        }
        break;
      case 0x50: //5XY0 - set VX to value of VY
        reg = chip->memory[pc] & 0x0F;
        printf("SE    V%01X, V%01X\n", reg, (chip->memory[pc + 1] & 0xF0) >> 4);
        chip->V[reg] = chip->V[(chip->memory[pc + 1] & 0xF0) >> 4];
        break;
      case 0x60: //6XNN - sets VX to NN
        reg = chip->memory[pc] & 0x0F;
        value = chip->memory[pc + 1];
        printf("LD    V%01X, #%u\n", reg, value);
        chip->V[reg] = value;
        break;
      case 0x70: //7XNN - Adds NN to VX (Carry flag is not changed)
        reg = chip->memory[pc] & 0x0F;
        value = chip->memory[pc + 1];
        printf("ADD    V%01X, #%u\n", value, reg);
        chip->V[reg] += value;
        break;
      case 0x80: //bit operations & math, only the last nibble matters
        switch(chip->memory[pc + 1] & 0x0F)
        {
          case 0x0: //VX = VY
            printf("LD    V%01X, V%01X\n", chip->memory[pc] & 0x0F, (chip->memory[pc + 1] & 0xF0) >> 4);
            chip->V[chip->memory[pc] & 0x0F] = chip->V[(chip->memory[pc + 1] & 0xF0) >> 4];
            break;
          case 0x1: //OR - VX = VX | VY
            printf("OR   V%01X, V%01X\n", chip->memory[pc] & 0x0F, (chip->memory[pc + 1] & 0xF0) >> 4);
            chip->V[chip->memory[pc] & 0x0F] = chip->V[chip->memory[pc] & 0x0F] | chip->V[(chip->memory[pc + 1] & 0xF0) >> 4];
            break;
          case 0x2: //AND VX = VX & VY
            printf("AND    V%01X, V%01X\n", chip->memory[pc] & 0x0F, (chip->memory[pc + 1] & 0xF0) >> 4);
            chip->V[chip->memory[pc] & 0x0F] = chip->V[chip->memory[pc] & 0x0F] & chip->V[(chip->memory[pc + 1] & 0xF0) >> 4];
            break;
          case 0x3: //XOR VX = VX ^ VY
            printf("XOR    V%01X, V%01X\n", chip->memory[pc] & 0x0F, (chip->memory[pc + 1] & 0xF0) >> 4);
            chip->V[chip->memory[pc] & 0x0F] = chip->V[chip->memory[pc] ^ 0x0F] & chip->V[(chip->memory[pc + 1] & 0xF0) >> 4];
            break;
          case 0x4: // ADD - VX = VX + VY w/carry
            printf("ADD    V%01X, V%01X\n", chip->memory[pc] & 0x0F, (chip->memory[pc + 1] & 0xF0) >> 4);
            if(chip->V[chip->memory[pc] & 0x0F] + chip->V[(chip->memory[pc + 1] & 0xF0) >> 4] > 0xFF)
            {
              //overflow
              chip->V[0xF] = 1;
            }
            else
            {
              chip->V[0xF] = 0;
            }
            chip->V[chip->memory[pc] & 0x0F] =
              (chip->V[chip->memory[pc] & 0x0F] + chip->V[(chip->memory[pc + 1] & 0xF0) >> 4]) & 0xFF;
            break;
          case 0x5: // borrow subtraction VX = VX - VY
            printf("SUB    V%01X, V%01X\n", chip->memory[pc] & 0x0F, (chip->memory[pc + 1] & 0xF0) >> 4);
            if(chip->V[chip->memory[pc] & 0x0F] > chip->V[(chip->memory[pc + 1] & 0xF0) >> 4])
            {
              // NO borrow
              chip->V[0xF] = 1;
            }
            else
            {
              chip->V[0xF] = 0;
            }
            chip->V[chip->memory[pc] & 0x0F] =
              (chip->V[chip->memory[pc] & 0x0F] - chip->V[(chip->memory[pc + 1] & 0xF0) >> 4]) & 0xFF;
            break;
          case 0x6: //Wikipedia says one thing, but then a footnote that says this is VX = VX >> 1
            // Same with http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#8xy6
            printf("SHR    V%01X\n", chip->memory[pc] & 0x0F);
            chip->V[chip->memory[pc] & 0x0F] = chip->V[chip->memory[pc] & 0x0F] >> 1;
            break;
          case 0x7: // borrow subtraction VX = VY - VX
            printf("SUBN    V%01X, V%01X\n", chip->memory[pc] & 0x0F, (chip->memory[pc + 1] & 0xF0) >> 4);
            if(chip->V[(chip->memory[pc + 1] & 0xF0) >> 4] > chip->V[chip->memory[pc] & 0x0F])
            {
              // NO borrow
              chip->V[0xF] = 1;
            }
            else
            {
              chip->V[0xF] = 0;
            }
            chip->V[chip->memory[pc] & 0x0F] =
              (chip->V[(chip->memory[pc + 1] & 0xF0) >> 4] - chip->V[chip->memory[pc] & 0x0F]) & 0xFF;
            break;
          case 0xE: //Wikipedia says one thing, but then a footnote that says this is VX = VX << 1
            // Same with http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#8xye
            printf("SHL    V%01X\n", chip->memory[pc] & 0x0F);
            chip->V[chip->memory[pc] & 0x0F] = chip->V[chip->memory[pc] & 0x0F] << 1;
            break;
          default:
            printf("Unknown opcode: %04x\n", chip->memory[pc] << 8 | chip->memory[pc + 1]);
            break;
        }
        break;
      case 0x90: //9XY0 - if(VX != VY) skip next instruction
        printf("SNE    V%01X, V%01X\n", chip->memory[pc] & 0x0F, (chip->memory[pc + 1] & 0xF0) >> 4);
        if((chip->memory[pc] & 0x0F) != (chip->memory[pc + 1] & 0xF0) >> 4)
        {
          chip->pc += 2;
        }
        break;
      case 0xA0: //ANNN - sets I to the address NNN
        value = (chip->memory[pc] & 0x0F) << 8 | chip->memory[pc + 1];
        printf("LD    I, $0x%03X\n", value);
        chip->I = value;
        break;
      case 0xB0: //BNNN - jumps to address NNN + V0
        value = (chip->memory[pc] & 0x0F) << 8 | chip->memory[pc + 1];
        printf("JMP    V0, $0x%03X\n", value);
        chip->pc = value + chip->V[0x0];
        break;
      case 0xC0: //CXNN - VX = rand() & NN
        reg = chip->memory[pc] & 0x0F;
        printf("RND    V%01X, #%u\n", reg, chip->memory[pc + 1]);
        chip->V[reg] = (rand() % 255) & chip->memory[pc + 1];
        break;
      case 0xD0: //DXYN - draw sprite at VX,VY width 8, height N
        reg = chip->memory[pc] & 0x0F;
        printf("DRW    V%01X, V%01X, %u\n", reg, (chip->memory[pc + 1] & 0xF0) >> 4, (chip->memory[pc + 1] & 0x0F));
        //TODO do this
        break;
      case 0xE0: //keyboard presses
        switch(chip->memory[pc + 1])
        {
          case 0x9E: //EX9E - skip next instruction if key with value in VX is pressed
            printf("SKP    V%01X\n", chip->memory[pc] & 0x0F);
            //TODO do this
            break;
          case 0xA1: //EXA1 - skip next instruction if key with value in VX is NOT pressed
            printf("SKNP    V%01X\n", chip->memory[pc] & 0x0F);
            //TODO do this
            break;
          default:
            printf("Unknown opcode: %04x\n", chip->memory[pc] << 8 | chip->memory[pc + 1]);
            break;
        }
        break;
      case 0xF0: //myriad, switch from here
        switch(chip->memory[pc + 1])
        {
          case 0x07: //FX07 - set VX to delay delay_timer
            printf("LD    V%01X, DT\n", chip->memory[pc] & 0x0F);
            chip->V[chip->memory[pc] & 0x0F] = chip->delay_timer;
            break;
          case 0x0A: //FX0A - blocks until key press and stores in VX
            printf("LD    V%01X, K\n", chip->memory[pc] & 0x0F);
            //TODO do this
            break;
          case 0x15: //FX15 - set delay timer to VX
            printf("LD    DT, V%01X\n", chip->memory[pc] & 0x0F);
            chip->delay_timer = chip->V[chip->memory[pc] & 0x0F];
            break;
          case 0x18: //FX18 - set sound timer to VX
            printf("LD    ST, V%01X\n", chip->memory[pc] & 0x0F);
            chip->sound_timer = chip->V[chip->memory[pc] & 0x0F];
            break;
          case 0x1E: //FX1E - I = I + VX
            printf("ADD    I, V%01X\n", chip->memory[pc] & 0x0F);
            chip->I += chip->V[chip->memory[pc] & 0x0F];
            break;
          case 0x29: //FX29 - I = location of sprite for digit VX
            printf("LD    F, V%01X\n", chip->memory[pc] & 0x0F);
            //TODO do this
            break;
          case 0x33: //FX33 - Store BCD of VX in I, I+1, I+2
            printf("LD    B, V%01X\n", chip->memory[pc] & 0x0F);
            chip->memory[chip->I] = chip->V[chip->memory[pc] & 0x0F] / 100;
            chip->memory[chip->I] = (chip->V[chip->memory[pc] & 0x0F] / 10) % 10;
            chip->memory[chip->I] = (chip->V[chip->memory[pc] & 0x0F] % 100) % 10;
            break;
          case 0x55: //FX55 - store V0 through VX in memory starting at location I
            printf("LD    [I], V%01X\n", chip->memory[pc] & 0x0F);
            for(i = 0; i < chip->memory[pc] & 0x0F; i++)
            {
              chip->memory[chip->I + i] = chip->V[i];
            }
            break;
          case 0x65: //FX65 - read from memory starting at I into V0 through VX
            printf("LD    V%01X, [I]\n", chip->memory[pc] & 0x0F);
            for(i = 0; i < chip->memory[pc] & 0x0F; i++)
            {
              chip->V[i] = chip->memory[chip->I + i];
            }
            break;
          default:
            printf("Unknown opcode: %04x\n", chip->memory[pc] << 8 | chip->memory[pc + 1]);
            break;
        }
        break;
      default:
        printf("Unknown opcode: %04x\n", chip->memory[pc] << 8 | chip->memory[pc + 1]);
        break;
  }
}

void chip8_build_sprites(struct chip8_hw *chip)
{
  //TODO fill 0x000 to 0x1FF with sprite data
}

void chip8_dump_registers(struct chip8_hw *chip)
{
  //Dump the current state of the registers
  printf("Current register values:\n");
  uint8_t i;
  for(i = 0; i <= 0xF; i++)
  {
    printf("V%01X:0x%02X, ", i, chip->V[i]);
  }
  printf("I:0x%02X, PC:0x%04X, SP:0x%04X\n", chip->I, chip->pc, chip->sp);
}
