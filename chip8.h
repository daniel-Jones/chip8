#ifndef CHIP8_H
#define CHIP8_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define WIDTH 64
#define HEIGHT 32
#define KEY_SIZE 15
#define MEMORY_SIZE 4096
#define STACK_SIZE 16
#define MAX_ROM_SIZE 0x1000 - 0x200 // memory size - reseved memory

int load_rom();
void chip8_init();

#endif
