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

  fread(chip->memory + 0x200, chip->rom_size, 1, f);
  fclose(f);

  // Display keyboard mapping
  printf("Emulator keyboard mapping:\n");
  printf("Real keys      Emulated keys\n");
  printf("|1|2|3|4|  ==  |1|2|3|C|\n");
  printf("|q|w|e|r|  ==  |4|5|6|D|\n");
  printf("|a|s|d|f|  ==  |7|8|9|E|\n");
  printf("|z|x|c|v|  ==  |A|0|B|F|\n\n");

  //Setup display
  SDL_Delay(16);
  SDL_Init(SDL_INIT_EVERYTHING);
  SDL_WM_SetCaption(argv[1], 0);
  chip->screen = SDL_SetVideoMode(chip->display_x, chip->display_y, 0, 0);

  printf("Running %s, hit ESC to exit\n", argv[1]);
  chip->running = 1;
  //Start emulation
  while(chip->running)
  {
    chip8_emulate_cycle(chip);
  }

  SDL_Quit(); //This frees chip->screen

  return 0;
}
