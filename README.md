## Ryan's CHIP-8 emulator project

### Environment
Developed and tested in Ubuntu 16.04.1 x86_64

### Prerequisites
GCC
SDL 1.2 development libraries
CMake
```
sudo apt-get install gcc libsdl1.2-dev cmake
```

### Building
```
gcc -g -o chip8_emu src/chip8_hw.c src/main.c -Iinclude
mkdir _build && cd _build
cmake ../
make
```

### Running
```
./chip8_emu ROMfile
```

#### Keyboard Mapping
```
  printf("Real keys      Emulated keys\n");
  printf("|1|2|3|4|  ==  |1|2|3|C|\n");
  printf("|q|w|e|r|  ==  |4|5|6|D|\n");
  printf("|a|s|d|f|  ==  |7|8|9|E|\n");
  printf("|z|x|c|v|  ==  |A|0|B|F|\n\n");
```
