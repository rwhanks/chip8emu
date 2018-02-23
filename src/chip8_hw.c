#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "chip8_hw.h"

void chip8_initialize(struct chip8_hw *chip, const char *rom_name)
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

  chip->running = 0;

  chip8_build_sprites(chip);
  chip8_build_keyboard(chip);

  memset(chip->display_pixels, 0, sizeof(chip->display_pixels[0][0]) * CHIP8_DISPLAY_X * CHIP8_DISPLAY_Y);

  //Setup display
  SDL_Delay(16);
  SDL_Init(SDL_INIT_EVERYTHING);
  SDL_WM_SetCaption(rom_name, 0);
  chip->screen = SDL_SetVideoMode(CHIP8_DISPLAY_X * CHIP8_DISPLAY_SCALE, CHIP8_DISPLAY_Y * CHIP8_DISPLAY_SCALE, 0, 0);
  chip8_draw_display(chip);

  //initialize so we can use it for CXNN opcode
  srand(time(NULL));
}

void chip8_emulate_cycle(struct chip8_hw *chip, uint8_t dump_flag)
{
  //TODO implement some sort of scaling/slowdown better than just the delay at the bottom
  // Check if a keyboard key is pressed
  SDL_Event event;
  uint16_t old_pc = chip->pc;
  while(SDL_PollEvent(&event))
  {
    if(event.type == SDL_KEYDOWN)
    {
      chip8_update_keyboard(chip, event, 1);
    }
    else if(event.type == SDL_KEYUP)
    {
      chip8_update_keyboard(chip, event, 0);
    }
    else if(event.type == SDL_QUIT)
    {
      chip->running = 0;
    }
  }

  // Run 1 cycle of the CPU
  chip8_decode_opcode(chip);

  //TODO implement real timing so these run at 60Hz in actual clock time
  //Modify the timers
  if(chip->delay_timer > 0)
  {
    chip->delay_timer--;
  }

  if(chip->sound_timer > 0)
  {
    //TODO play sound?
    chip->sound_timer--;
  }

  if(dump_flag)
  {
    // If we are just dumping the ROM, set the pc to +2 from its previous value
    // we won't care about register state, etc
    chip->pc = old_pc + 2;
  }

  // Lame hack
  SDL_Delay(1);
}


// Private functions
void chip8_decode_opcode(struct chip8_hw *chip)
{
  // TODO make this less terrible than a giant switch
  // Do this 1 byte at a time
  uint8_t reg = 0;
  uint8_t reg2 = 0;
  uint16_t value = 0;
  uint8_t i;
  printf("%04X %04X ", chip->pc, (chip->memory[chip->pc] << 8 | chip->memory[chip->pc + 1]));
  switch(chip->memory[chip->pc] & 0xF0)
  {
      case 0x00: // couple of operations
      switch(chip->memory[chip->pc + 1])
      {
        case 0x00: //ignored
          printf("SYS addr (Ignored)\n");
          chip->pc += 2;
          break;
        case 0xE0: //CLS - clear display
          printf("CLS\n");
          memset(chip->display_pixels, 0, sizeof(chip->display_pixels[0][0]) * CHIP8_DISPLAY_X * CHIP8_DISPLAY_Y);
          chip8_draw_display(chip);
          chip->pc += 2;
          break;
        case 0xEE: //return from subroutine
          printf("RET\n");
          chip->pc = chip->stack[chip->sp];
          chip->sp--;
          break;
        default:
          printf("Unknown opcode: %04x\n", chip->memory[chip->pc] << 8 | chip->memory[chip->pc + 1]);
          chip->pc += 2;
          break;
      }
      break;
      case 0x10: //1NNN - jump to address NNN
        value = (chip->memory[chip->pc] & 0x0F) << 8 | chip->memory[chip->pc + 1];
        printf("JMP   $0x%03X\n", value);
        chip->pc = value;
        break;
      case 0x20: //2NNN - call subroutine at NNN
        value = (chip->memory[chip->pc] & 0x0F) << 8 | chip->memory[chip->pc + 1];
        printf("CALL  $0x%03X\n", value);
        chip->pc += 2; //set the RET point
        chip->sp++;
        chip->stack[chip->sp] = chip->pc;
        chip->pc = value;
        break;
      case 0x30: //3XNN - if VX == NN then skip next instruction
        reg = chip->memory[chip->pc] & 0x0F;
        value = chip->memory[chip->pc + 1];
        printf("SE    V%01X, #%u\n", reg, value);
        if(chip->V[reg] == value)
        {
          chip->pc += 2;
        }
        chip->pc += 2;
        break;
      case 0x40: //4XNN - if VX != NN then skip next instruction
        reg = chip->memory[chip->pc] & 0x0F;
        value = chip->memory[chip->pc + 1];
        printf("SNE   V%01X, #%u\n", reg, value);
        if(chip->V[reg] != value)
        {
          chip->pc += 2;
        }
        chip->pc += 2;
        break;
      case 0x50: //5XY0 - set VX to value of VY
        reg = chip->memory[chip->pc] & 0x0F;
        printf("SE    V%01X, V%01X\n", reg, (chip->memory[chip->pc + 1] & 0xF0) >> 4);
        chip->V[reg] = chip->V[(chip->memory[chip->pc + 1] & 0xF0) >> 4];
        chip->pc += 2;
        break;
      case 0x60: //6XNN - sets VX to NN
        reg = chip->memory[chip->pc] & 0x0F;
        value = chip->memory[chip->pc + 1];
        printf("LD    V%01X, #%u\n", reg, value);
        chip->V[reg] = value;
        chip->pc += 2;
        break;
      case 0x70: //7XNN - Adds NN to VX (Carry flag is not changed)
        reg = chip->memory[chip->pc] & 0x0F;
        value = chip->memory[chip->pc + 1];
        printf("ADD   V%01X, #%u\n", reg, value);
        chip->V[reg] += value;
        chip->pc += 2;
        break;
      case 0x80: //bit operations & math, only the last nibble changes
        reg = chip->memory[chip->pc] & 0x0F;
        reg2 = (chip->memory[chip->pc + 1] & 0xF0) >> 4;
        switch(chip->memory[chip->pc + 1] & 0x0F)
        {
          case 0x0: //VX = VY
            printf("LD    V%01X, V%01X\n", reg, reg2);
            chip->V[reg] = chip->V[reg2];
            break;
          case 0x1: //OR - VX = VX | VY
            printf("OR   V%01X, V%01X\n", reg, reg2);
            chip->V[reg] |= chip->V[reg2];
            break;
          case 0x2: //AND VX = VX & VY
            printf("AND   V%01X, V%01X\n", reg, reg2);
            chip->V[reg] &= chip->V[reg2];
            break;
          case 0x3: //XOR VX = VX ^ VY
            printf("XOR   V%01X, V%01X\n", reg, reg2);
            chip->V[reg] ^= chip->V[reg2];
            break;
          case 0x4: // ADD - VX = VX + VY w/carry
            printf("ADD   V%01X, V%01X\n", reg, reg2);
            if(chip->V[reg] + chip->V[reg2] > 0xFF)
            {
              //overflow
              chip->V[0xF] = 1;
            }
            else
            {
              chip->V[0xF] = 0;
            }
            chip->V[reg] = (chip->V[reg] + chip->V[reg2]) & 0xFF;
            break;
          case 0x5: // borrow subtraction VX = VX - VY
            printf("SUB   V%01X, V%01X\n", reg, reg2);
            if(chip->V[reg] > chip->V[reg2])
            {
              // NO borrow
              chip->V[0xF] = 1;
            }
            else
            {
              chip->V[0xF] = 0;
            }
            chip->V[reg] = (chip->V[reg] - chip->V[reg2]) & 0xFF;
            break;
          case 0x6: // VX = VX >> 1
            // Also set VF to LSB prior to shift
            printf("SHR   V%01X\n", reg);
            chip->V[0xF] = chip->V[reg] & 1;
            chip->V[reg] = chip->V[reg] >> 1;
            break;
          case 0x7: // borrow subtraction VX = VY - VX
            printf("SUBN  V%01X, V%01X\n", reg, reg2);
            if(chip->V[reg2] > chip->V[reg])
            {
              // NO borrow
              chip->V[0xF] = 1;
            }
            else
            {
              chip->V[0xF] = 0;
            }
            chip->V[reg] = (chip->V[reg2] - chip->V[reg]) & 0xFF;
            break;
          case 0xE: //VX = VX << 1
            // Also set VF to MSB prior to shift
            printf("SHL   V%01X\n", reg);
            chip->V[0xF] = chip->V[reg] & 0x80;
            chip->V[reg] = chip->V[reg] << 1;
            break;
          default:
            printf("Unknown opcode: %04x\n", chip->memory[chip->pc] << 8 | chip->memory[chip->pc + 1]);
            break;
        }
        chip->pc += 2;
        break;
      case 0x90: //9XY0 - if(VX != VY) skip next instruction
        printf("SNE   V%01X, V%01X\n", chip->memory[chip->pc] & 0x0F, (chip->memory[chip->pc + 1] & 0xF0) >> 4);
        if((chip->memory[chip->pc] & 0x0F) != (chip->memory[chip->pc + 1] & 0xF0) >> 4)
        {
          chip->pc += 2;
        }
        chip->pc += 2;
        break;
      case 0xA0: //ANNN - sets I to the address NNN
        value = (chip->memory[chip->pc] & 0x0F) << 8 | chip->memory[chip->pc + 1];
        printf("LD    I, $0x%03X\n", value);
        chip->I = value;
        chip->pc += 2;
        break;
      case 0xB0: //BNNN - jumps to address NNN + V0
        value = (chip->memory[chip->pc] & 0x0F) << 8 | chip->memory[chip->pc + 1];
        printf("JMP   V0, $0x%03X\n", value);
        chip->pc = value + chip->V[0x0];
        break;
      case 0xC0: //CXNN - VX = rand() & NN
        reg = chip->memory[chip->pc] & 0x0F;
        value = chip->memory[chip->pc + 1] & (rand() % 0xFF);
        printf("RND   V%01X, #%u\n", reg, value);
        chip->V[reg] = value;
        chip->pc += 2;
        break;
      case 0xD0: //DXYN - draw sprite at VX,VY width 8, height N
        {
          reg = chip->memory[chip->pc] & 0x0F;
          reg2 = (chip->memory[chip->pc + 1] & 0xF0) >> 4;
          value = chip->memory[chip->pc + 1] & 0x0F;
          printf("DRW   V%01X, V%01X, #%u\n", reg, reg2, value);
          // read in N bytes of memory starting at I
          // Display these bytes as sprites at (VX,VY) by XOR to existing screen
          //   If erased VF = 1
          //   Wraparound?
          uint8_t pixel;
          chip->V[0xF] = 0;
          for(uint8_t y = 0; y < value; y++)
          {
            pixel = chip->memory[chip->I + y];
            // Compare the set bits in I+y to those in the display_pixels
            for(uint8_t x = 0; x < 8; x++)
            {
              // test bits in the pixel
              if(0 != (pixel & (0x80 >> x)))
              {
                // against those already set in display_pixels
                if(chip->display_pixels[(x + chip->V[reg])][(y + chip->V[reg2])] == 1)
                {
                  // collision
                  chip->V[0xF] = 1;
                }
                // set the new display pixel
                chip->display_pixels[(x + chip->V[reg])][(y + chip->V[reg2])] ^= 1;
                // update the display based on new pixels
                chip8_update_display(chip, (x + chip->V[reg]), (y + chip->V[reg2]));
              }
            }
          }
        }
        chip->pc += 2;
        break;
      case 0xE0: //keyboard presses
        switch(chip->memory[chip->pc + 1])
        {
          case 0x9E: //EX9E - skip next instruction if key with value in VX is pressed
            reg = chip->memory[chip->pc] & 0x0F;
            printf("SKP   V%01X\n", reg);
            if(1 == chip->keyboard.pressed[chip->V[reg]])
            {
              chip->pc += 2;
            }
            break;
          case 0xA1: //EXA1 - skip next instruction if key with value in VX is NOT pressed
            reg = chip->memory[chip->pc] & 0x0F;
            printf("SKNP  V%01X\n", reg);
            if(0 == chip->keyboard.pressed[chip->V[reg]])
            {
              chip->pc += 2;
            }
            break;
          default:
            printf("Unknown opcode: %04x\n", chip->memory[chip->pc] << 8 | chip->memory[chip->pc + 1]);
            break;
        }
        chip->pc += 2;
        break;
      case 0xF0: //myriad, switch from here
        switch(chip->memory[chip->pc + 1])
        {
          case 0x07: //FX07 - set VX to delay delay_timer
            reg = chip->memory[chip->pc] & 0x0F;
            printf("LD    V%01X, DT\n", reg);
            chip->V[reg] = chip->delay_timer;
            break;
          case 0x0A: //FX0A - blocks until key press and stores in VX
            reg = chip->memory[chip->pc] & 0x0F;
            printf("LD    V%01X, K\n", reg);
            chip->V[reg] = chip8_poll_for_keypress(chip); //blocks
            break;
          case 0x15: //FX15 - set delay timer to VX
            printf("LD    DT, V%01X\n", chip->memory[chip->pc] & 0x0F);
            chip->delay_timer = chip->V[chip->memory[chip->pc] & 0x0F];
            break;
          case 0x18: //FX18 - set sound timer to VX
            printf("LD    ST, V%01X\n", chip->memory[chip->pc] & 0x0F);
            chip->sound_timer = chip->V[chip->memory[chip->pc] & 0x0F];
            break;
          case 0x1E: //FX1E - I = I + VX
            printf("ADD   I, V%01X\n", chip->memory[chip->pc] & 0x0F);
            chip->I += chip->V[chip->memory[chip->pc] & 0x0F];
            break;
          case 0x29: //FX29 - I = location of sprite for digit VX
            reg = chip->memory[chip->pc] & 0x0F;
            printf("LD    F, V%01X\n", reg);
            // fonts start at 0x0 and is 5*16
            chip->I = chip->V[reg] * 5;
            break;
          case 0x33: //FX33 - Store BCD of VX in I, I+1, I+2
            printf("LD    B, V%01X\n", chip->memory[chip->pc] & 0x0F);
            chip->memory[chip->I] = chip->V[chip->memory[chip->pc] & 0x0F] / 100;
            chip->memory[chip->I+1] = (chip->V[chip->memory[chip->pc] & 0x0F] / 10) % 10;
            chip->memory[chip->I+2] = (chip->V[chip->memory[chip->pc] & 0x0F] % 100) % 10;
            break;
          case 0x55: //FX55 - store V0 through VX in memory starting at location I
            reg = chip->memory[chip->pc] & 0x0F;
            printf("LD    [I], V%01X\n",reg);
            for(i = 0; i <= reg; i++)
            {
              chip->memory[chip->I + i] = chip->V[i];
            }
            break;
          case 0x65: //FX65 - read from memory starting at I into V0 through VX
            reg = chip->memory[chip->pc] & 0x0F;
            printf("LD    V%01X, [I]\n", reg);
            for(i = 0; i <= reg; i++)
            {
              chip->V[i] = chip->memory[chip->I + i];
            }
            break;
          default:
            printf("Unknown opcode: %04x\n", chip->memory[chip->pc] << 8 | chip->memory[chip->pc + 1]);
            break;
        }
        chip->pc += 2;
        break;
      default:
        printf("Unknown opcode: %04x\n", chip->memory[chip->pc] << 8 | chip->memory[chip->pc + 1]);
        chip->pc += 2;
        break;
  }
}

void chip8_draw_display(struct chip8_hw *chip)
{
  // https://wiki.libsdl.org/SDL_Rect
  SDL_Rect rect;

  rect.x = 0;
  rect.y = 0;
  rect.w = CHIP8_DISPLAY_X * CHIP8_DISPLAY_SCALE;
  rect.h = CHIP8_DISPLAY_X * CHIP8_DISPLAY_SCALE;

  uint8_t value = 0;

  for(uint8_t x = 0; x < CHIP8_DISPLAY_X; x++)
  {
    for(uint8_t y = 0; y < CHIP8_DISPLAY_Y; y++)
    {
      rect.x = x*CHIP8_DISPLAY_SCALE;
      rect.y = y*CHIP8_DISPLAY_SCALE;
      rect.w = CHIP8_DISPLAY_SCALE;
      rect.h = CHIP8_DISPLAY_SCALE;
      value = chip->display_pixels[x][y]*255;
      SDL_FillRect(chip->screen, &rect, SDL_MapRGB(chip->screen->format, value, value, value));
    }
  }

  // This is effectively SDL_UpdateRect(screen, 0, 0, 0, 0)
  // https://www.libsdl.org/release/SDL-1.2.15/docs/html/sdlflip.html
  SDL_Flip(chip->screen);
}

void chip8_update_display(struct chip8_hw *chip, uint16_t xpos, uint16_t ypos)
{
  // Draw the pixel that changed
  SDL_Rect rect;
  rect.x = xpos * CHIP8_DISPLAY_SCALE;
  rect.y = ypos * CHIP8_DISPLAY_SCALE;
  rect.w = CHIP8_DISPLAY_SCALE;
  rect.h = CHIP8_DISPLAY_SCALE;
  uint8_t value = chip->display_pixels[xpos][ypos]*255;

  SDL_FillRect(chip->screen, &rect, SDL_MapRGB(chip->screen->format, value, value, value));
  SDL_Flip(chip->screen);
}

void chip8_build_keyboard(struct chip8_hw *chip)
{

  // Display keyboard mapping
  printf("Emulator keyboard mapping:\n");
  printf("Real keys      Emulated keys\n");
  printf("|1|2|3|4|  ==  |1|2|3|C|\n");
  printf("|q|w|e|r|  ==  |4|5|6|D|\n");
  printf("|a|s|d|f|  ==  |7|8|9|E|\n");
  printf("|z|x|c|v|  ==  |A|0|B|F|\n\n");

    // Shamelessly borrowed/modified from somewhere on the Internet
  chip->keyboard.key[0x1] = SDLK_1;
  chip->keyboard.key[0x2] = SDLK_2;
  chip->keyboard.key[0x3] = SDLK_3;
  chip->keyboard.key[0xC] = SDLK_4;
  chip->keyboard.key[0x4] = SDLK_q;
  chip->keyboard.key[0x5] = SDLK_w;
  chip->keyboard.key[0x6] = SDLK_e;
  chip->keyboard.key[0xD] = SDLK_r;
  chip->keyboard.key[0x7] = SDLK_a;
  chip->keyboard.key[0x8] = SDLK_s;
  chip->keyboard.key[0x9] = SDLK_d;
  chip->keyboard.key[0xE] = SDLK_f;
  chip->keyboard.key[0xA] = SDLK_z;
  chip->keyboard.key[0x0] = SDLK_x;
  chip->keyboard.key[0xB] = SDLK_c;
  chip->keyboard.key[0xF] = SDLK_v;
}

void chip8_update_keyboard(struct chip8_hw *chip, SDL_Event event, uint8_t pressed)
{
  for(uint8_t i=0; i < 16; i++)
  {
    if(event.key.keysym.sym == chip->keyboard.key[i])
    {
      chip->keyboard.pressed[i] = pressed;
    }
    else if(event.key.keysym.sym == SDLK_ESCAPE)
    {
      chip->running = 0;
    }
  }
}

uint8_t chip8_poll_for_keypress(struct chip8_hw *chip)
{
  SDL_Event keyevent;

  uint8_t done = 0;
  while(!done && SDL_WaitEvent(&keyevent))
  {
    switch(keyevent.type)
    {
      case SDL_KEYDOWN:
        for(uint8_t i = 0; i < 16; i++)
        {
          if(keyevent.key.keysym.sym == chip->keyboard.key[i])
          {
            // Update that its pressed?
            chip->keyboard.pressed[i] = 1;
            return i;
          }
        }
        if(keyevent.key.keysym.sym == SDLK_ESCAPE)
        {
          chip->running = 0;
          return 0; // return a dummy value here because we are exiting
        }
        break;
      default:
        break;
    }
  }
}

void chip8_build_sprites(struct chip8_hw *chip)
{
  //Load the sprite table into somewhere in 0x000 to 0x1FFF
  // 5 bytes * 16 characters
  uint8_t chip8_font[] = {0xF0, 0x90, 0x90, 0x90, 0xF0,  //0
                          0x20, 0x60, 0x20, 0x20, 0x70,  //1
                          0xF0, 0x10, 0xF0, 0x80, 0xF0,  //2
                          0xF0, 0x10, 0xF0, 0x10, 0xF0,  //3
                          0x90, 0x90, 0xF0, 0x10, 0x10,  //4
                          0xF0, 0x80, 0xF0, 0x10, 0xF0,  //5
                          0xF0, 0x80, 0xF0, 0x90, 0xF0,  //6
                          0xF0, 0x10, 0x20, 0x40, 0x40,  //7
                          0xF0, 0x90, 0xF0, 0x90, 0xF0,  //8
                          0xF0, 0x90, 0xF0, 0x10, 0xF0,  //9
                          0xF0, 0x90, 0xF0, 0x90, 0x90,  //A
                          0xE0, 0x90, 0xE0, 0x90, 0xE0,  //B
                          0xF0, 0x80, 0x80, 0x80, 0xF0,  //C
                          0xE0, 0x90, 0x90, 0x90, 0xE0,  //D
                          0xF0, 0x80, 0xF0, 0x80, 0xF0,  //E
                          0xF0, 0x80, 0xF0, 0x80, 0x80   //F
                        };
    memcpy(&chip->memory[0], chip8_font, 80);
}

// Debugging functions
void chip8_dump_registers(struct chip8_hw *chip)
{
  //Dump the current state of the registers
  printf("Current register values:\n");
  uint8_t i;
  for(i = 0; i <= 0xF; i++)
  {
    printf("V%01X:0x%02X, ", i, chip->V[i]);
  }
  printf("I:0x%02X, chip->pc:0x%04X, SP:0x%04X\n", chip->I, chip->pc, chip->sp);
}

void chip8_print_display_pixels(struct chip8_hw *chip)
{
  for(uint8_t x = 0; x < CHIP8_DISPLAY_X; x++)
  {
    for(uint8_t y = 0; y < CHIP8_DISPLAY_Y; y++)
    {
      printf("%02X ", chip->display_pixels[x][y]);
    }
    printf("\n");
  }
}
