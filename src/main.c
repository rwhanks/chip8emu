#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "chip8_hw.h"
#include "SDL/SDL.h"

int main(int argc, char **argv)
{
  uint8_t dump_flag = 0;
  char *rom_name = NULL;
  int c;

  while((c = getopt(argc, argv, "dr:")) != -1)
  {
    switch(c)
    {
      case 'd':
        dump_flag = 1;
        break;
      case 'r':
        rom_name = optarg;
        break;
      case '?':
        if(optopt == 'r')
          fprintf(stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr, "Unknown option character `\\x%x'.\n",optopt);
        return 1;
      default:
        abort ();
    }
  }

  if(rom_name == NULL)
  {
    printf("-r ROM_file option required\n");
    exit(1);
  }

  FILE *f = fopen(rom_name, "rb");
  if(f == NULL)
  {
    printf("Error opening %s\n", rom_name);
    exit(1);
  }

  struct chip8_hw *chip = malloc(sizeof(struct chip8_hw));
  chip8_initialize(chip, rom_name);

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

  // If we aren't dumping the ROM, we should execute it
  if(!dump_flag)
  {
    printf("Running %s, hit ESC to exit\n", rom_name);
    chip->running = 1;
    while(chip->running)
    {
      chip8_emulate_cycle(chip, dump_flag);
    }
  }
  else
  {
    //TODO make this better because if we use any block on keyboard input instructions, those keys will
    // need to be pressed to continue
    //Dump the contents of the ROM
    while(chip->pc < (0x200 + chip->rom_size))
    {
      chip8_emulate_cycle(chip, dump_flag);
    }
  }

  SDL_Quit(); //This frees chip->screen
  free(chip);

  return 0;
}
