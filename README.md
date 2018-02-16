# Ryan's CHIP-8 emulator project

## Environment
Developed and tested in Ubuntu 16.04.1 x86_64

### Prerequisites
GCC
SDL 1.2 development libraries
...
sudo apt-get install gcc
sudo apt-get install libsdl-dev
...

### Building
gcc -g -o chip8_emu src/chip8_hw.c src/main.c -Iinclude

### Running
...
./chip8_emu ROMfile
...
