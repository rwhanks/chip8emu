#include <stdio.h>
#include <stdlib.h>
#include "chip8_hw.h"

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

  //Start emulation -- for now run until ctrl+c?
  while(1)
  {
    chip8_emulate_cycle(chip);
  }

  return 0;
}
