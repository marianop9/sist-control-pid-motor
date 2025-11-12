cmake_minimum_required(VERSION 3.13)

# set(PICO_BOARD pico_w)
set(PICO_BOARD pico2_w)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include(pico_sdk_import.cmake)

project(motor-pid C CXX ASM)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_subdirectory(src)