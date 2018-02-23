cmake_minimum_required(VERSION 2.8.11)
project(chip8_emu)
set(CMAKE_C_FLAGS "-g")

find_package(SDL REQUIRED)

include_directories(include/)
add_executable(chip8_emu src/main.c src/chip8_hw.c)
target_link_libraries(chip8_emu SDL)
