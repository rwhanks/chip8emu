#include <stdio.h>
#include <stdlib.h>
#include "chip8_hw.h"
#include "SDL/SDL.h"

int main(int argc, char **argv)
{
  if(argc < 1)
  {
    printf("Usage: ./chip8_emu ROM_file\n");
    exit(1);
  }
  FILE *f = fopen(argv[1], "rb");
  if(f == NULL)
  {
    printf("Error opening %s\n", argv[1]);
    exit(1);
  }

  struct chip8_hw *chip = malloc(sizeof(struct chip8_hw));
  chip8_initialize(chip);

  //Read the ROM into memory starting at 0x200
  fseek(f, 0L, SEEK_END);
  chip->rom_size = ftell(f);
  fseek(f, 0L, SEEK_SET);

  if(chip->rom_size < (CHIP8_MEM_SIZE - 0x200))
  {
    fread(chip->memory + 0x200, chip->rom_size, 1, f);
    fclose(f);
  }
  else
  {
    printf("Error: ROM too large to fit in memory space, must be smaller than: %u\n", (CHIP8_MEM_SIZE - 0x200));
    fclose(f);
    free(chip);
    exit(1);
  }

  //Setup display
  SDL_Delay(16);
  SDL_Init(SDL_INIT_EVERYTHING);
  SDL_WM_SetCaption(argv[1], 0);
  chip->screen = SDL_SetVideoMode(CHIP8_DISPLAY_X * CHIP8_DISPLAY_SCALE, CHIP8_DISPLAY_Y * CHIP8_DISPLAY_SCALE, 0, 0);

  //Start emulation
  printf("Running %s, hit ESC to exit\n", argv[1]);
  chip->running = 1;
  while(chip->running )
  {
    chip8_emulate_cycle(chip);
  }

  free(chip);
  SDL_Quit(); //This frees chip->screen

  return 0;
}
