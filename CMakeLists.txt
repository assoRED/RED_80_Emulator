cmake_minimum_required(VERSION 3.12)
project(RED_80_Emulator)

set(CMAKE_CXX_STANDARD 17)

file(GLOB src_files CONFIGURE_DEPENDS src/*.cpp src/*.hpp)

add_executable(RED80Emu ${src_files} libz80/z80.c libz80/z80.h)
