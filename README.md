## Ryan's CHIP-8 emulator project

### Environment
Developed and tested in Ubuntu 16.04.1 x86_64

### Prerequisites
GCC, SDL 1.2 development libraries, CMake
```
sudo apt-get install gcc libsdl1.2-dev cmake
```

### Building
```
mkdir _build && cd _build
cmake ../
make
```

### Running
Arguments:
```
-r ROM_file : ROM file to emulate/decode (REQUIRED)
-d : Dump the ROM file assembly to stdout
```
Running:
```
./chip8_emu -r ROMfile
```

#### Keyboard Mapping
```
ESC to Exit 

Real keys      Emulated keys
|1|2|3|4|  ==  |1|2|3|C|
|q|w|e|r|  ==  |4|5|6|D|
|a|s|d|f|  ==  |7|8|9|E|
|z|x|c|v|  ==  |A|0|B|F|
```
