#include "chip8.h"

uint16_t opcode;		// 2 byte opcode
uint8_t key[KEY_SIZE]; 		// hex key input
uint32_t video[WIDTH*HEIGHT]; 	// video memory TODO: video should be 8 bit not 32
uint8_t V[REGISTER_COUNT];	// registers - 0x0 to 0xF
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
	fread(&memory[PROGRAM_START], 1, MAX_ROM_SIZE, romfile);

	fclose(romfile);
	return ret;
}

void
chip8_init()
{
	/* clear everything and set sane defaults */
	memset(key, 0, sizeof(uint8_t) * KEY_SIZE);
	memset(video, 0, (WIDTH*HEIGHT) * sizeof(uint32_t));
	memset(V, 0, sizeof(uint8_t) * REGISTER_COUNT);
	memset(stack, 0, (STACK_SIZE) * sizeof(uint16_t));
	memset(memory, 0, sizeof(uint8_t) * MEMORY_SIZE);

	PC = PROGRAM_START; 	// set program counter to where roms are stored
	opcode = 0;
	I = 0;
	SP = 0;
	draw_flag = 1;		// draw by default i guess
	delay_timer = 0;
	sound_timer = 0;
	srand(time(NULL));

	/* load font into memory at 0x00: we have 0x00 to 0x1FF free for anything we want */
	//TODO: add whole alphabet, then can write a rom to write strings or something
	for (uint8_t i = 0; i < FONT_BYTE_SIZE; i++)
	{
		memory[0x0 + i] = chip8_fontset[i];
	}

	/*
	uint8_t sinv[] = {0xBA, 0x7C, 0xD6, 0xFE, 0x54, 0xAA};
	// temp
	for (uint8_t i = 0; i <= FONT_WIDTH; i++)
	{
		memory[PROGRAM_START+i] = sinv[i];
	}
	*/
}

void
chip8_draw_sprite(int startx, int starty, uint16_t mem, uint8_t size)
{
	/*
	 * draw sprite located at loc of height size at startx,starty
	 */

	uint8_t byte = 0;
	uint8_t mask = 0x1;
	V[0xF] = 0; /* set collision register to 0 */
	for (uint8_t byteoffset = 0; byteoffset < size; byteoffset++)
	{
		/* loop through each byte from mem to mem+size */
		byte = memory[mem+byteoffset];
		int bit = 0;
		for (mask = BYTE_MASK; mask != 0; mask >>= 1)
		{
			if (byte & mask)
			{
				uint32_t pixel = WIDTH*(starty%HEIGHT)+((startx%WIDTH)+bit);
				if (video[pixel] != 0)
				{
					/* if the video bit is already set, we need to set the collision register */
					V[0xF] = 1;
				}
				video[pixel] ^= PIXEL_COLOR;
			}
			bit++;
		}
		starty++;
	}
}

void
chip8_beep()
{
	// TODO: holy shit this is terrible, do something better
	puts("\a");
}

void
unknown_opcode(uint16_t bad_opcode)
{
	fprintf(stderr, "unknown opcode: 0x%02X at 0x%02X\n", bad_opcode, PC);
}
void
chip8_cycle()
{
	/*
	 * fetch, decode, execute
	*/

	/*
	 * the opcode is a 2 byte value, while our memory is 1 byte
	 * so to get our opcode we need to combine 2 bytes located at PC and PC+1
	 * we right shift the first byte by 8 (1 byte) to place it in the high byte
	 * and we store the second byte in the lower byte
	 */
	opcode = (memory[PC] << 8) | memory[PC+1];

	uint16_t nnn = opcode & 0x0FFF; 	// lowest 12 bits
	uint8_t n = opcode & 0x000F; 		// lowest 4 bits
	uint8_t x = (opcode >> 8) & 0x000F;	// lower 4 bits of the high byte, we discard the low byte by right shifting it out
	uint8_t y = (opcode >> 4) & 0x000F;	// upper 4 bits of the low byte, so we need to discard the lower 4 bits
	uint8_t kk = opcode & 0x00FF;		// lowest 8 bits

	switch (opcode & 0xF000)
	{
	/*
	 * decode highest 4 bits
	 * the highest 4 bits contains the instruction
	 */
	case 0x0000:
	{
		switch (kk)
		{
		case 0x00E0: /* cls (clear screen) */
			puts("clear");
			memset(video, 0, (WIDTH*HEIGHT) * sizeof(uint32_t));
			draw_flag = 1;
			break;
		case 0x00EE: /* ret (return from subroutine) */
			break;
		default: unknown_opcode(opcode);
		}
		break;
	}
	case 0x6000: /* LD Vx, byte (load byte in register x) */
	{
		V[x] = kk;
		break;
	}

	default: unknown_opcode(opcode);
	}

	/* timers */
	if (delay_timer > 0)
		delay_timer--;
	if (sound_timer > 0)
	{
		chip8_beep();
		sound_timer--;
	}
	PC += 2;
}
