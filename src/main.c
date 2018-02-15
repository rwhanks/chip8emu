#include <stdio.h>
#include <stdlib.h>
#include "chip8_hw.h"

int main(int argc, char **argv)
{
  FILE *f = fopen(argv[1], "rb");
  if(f == NULL)
  {
    printf("Error opening %s\n", argv[1]);
    exit(1);
  }

  int pc = 0;
  struct chip8_hw *chip = malloc(sizeof(struct chip8_hw));
  chip8_initialize(chip);

  fseek(f, 0L, SEEK_END);
  int file_size = ftell(f);
  fseek(f, 0L, SEEK_SET);

  fread(chip->memory, file_size, 1, f);
  fclose(f);

  while(pc < file_size)
  {
    chip8_decode_opcode(chip, pc);
    pc+=2;
  }
  return 0;
}
