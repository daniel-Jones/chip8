#include "chip8.h"

uint16_t opcode;		// 2 byte opcode
uint8_t key[KEY_SIZE]; 		// hex key input
uint32_t video[WIDTH*HEIGHT]; 	// video memory TODO: video should be 8 bit not 32
uint8_t V[16];			// registers - 0x0 to 0xF
uint16_t I;			// special I register, stores addresses
uint16_t PC;			// program counter
uint8_t SP;			// stack pointer
uint16_t stack[STACK_SIZE];	// stack, for address storage when using subroutines, maximum of 16 levels of nested subroutines
uint8_t memory[MEMORY_SIZE];	// 4KB of program memory
int draw_flag;			// draw flag, when 1 the screen will be drawn
uint8_t delay_timer;		// delay timer, decremented by one at 60hz, at 0 do nothing
uint8_t sound_timer;		// sound timer, decremented by one at 60hz, while it is > 0 a beep is played

/* 0 - F hexidecimal font as required by the chip8 spec */
uint8_t chip8_fontset[80] =
{
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

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
	/* clear everything and set sane defaults */
	memset(key, 0, sizeof(uint8_t) * 15);
	memset(video, 0, (WIDTH*HEIGHT) * sizeof(uint32_t));
	memset(V, 0, sizeof(uint8_t) * 16);
	memset(stack, 0, (STACK_SIZE) * sizeof(uint16_t));
	memset(memory, 0, sizeof(uint8_t) * MEMORY_SIZE);

	PC = 0x200; 		// set program counter to where roms are stored
	opcode = 0;
	I = 0;
	SP = 0;
	draw_flag = 1;		// draw by default i guess
	delay_timer = 0;
	sound_timer = 0;
	srand(time(NULL));	// seed rng

	/* load font into memory at 0x00: we have 0x00 to 0x1FF free for anything we want */
	//TODO: add whole alphabet, then can write a rom to write strings or something
	for (uint8_t i = 0; i < 80; i++)
	{
		memory[0x0 + i] = chip8_fontset[i];
	}
}
