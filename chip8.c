#include "chip8.h"

uint8_t key[KEY_SIZE]; 		// hex key input
uint32_t video[WIDTH*HEIGHT]; 	// video memory TODO: video should be 8 bit not 32
uint8_t V[16];			// registers - 0x0 to 0xF
uint16_t I;			// special I register, stores addresses
uint16_t PC;			// program counter
uint8_t SP;			// stack pointer
uint16_t stack[STACK_SIZE];	// stack, for address storage when using subroutines, maximum of 16 levels of nested subroutines
uint8_t memory[MEMORY_SIZE];	// 4KB of program memory

int
load_rom(char *rom)
{
	int ret = 1;
	printf("loading rom %s..\n", rom);
	FILE *romfile;
	romfile = fopen(rom, "rb");
	if (romfile == NULL)
	{
		fprintf(stderr, "cannot read rom %s: %s\n", rom, strerror(errno));
		ret = 0;
	}

	/* read a maximum of MAX_ROM_SIZE bytes into memory starting after the reserved memory */
	fread(&memory[0x200], 1, MAX_ROM_SIZE, romfile);

	fclose(romfile);
	return ret;
}

void
chip8_init()
{
	/* clear everything */
	memset(key, 0, 15);
	memset(video, 0, (WIDTH*HEIGHT) * sizeof(uint32_t));
	memset(V, 0, 16);
	memset(stack, 0, (STACK_SIZE) * sizeof(uint16_t));
	memset(memory, 0, MEMORY_SIZE);
}
