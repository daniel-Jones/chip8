cmake_minimum_required(VERSION 3.5)
cmake_minimum_required(VERSION 3.14)

project(chip8 LANGUAGES C)
 set(CMAKE_C_STANDARD 99)

find_package(SDL2 REQUIRED)

add_executable(
	chip8
	main.c chip8.c chip8.h)

target_compile_options(chip8 PRIVATE -Wall)

target_link_libraries(chip8 PRIVATE SDL2::SDL2)

