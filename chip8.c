/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 *(at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

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
	printf("loading rom %s..\n", rom);
	FILE *romfile;
	romfile = fopen(rom, "rb");
	if (romfile == NULL)
	{
		fprintf(stderr, "cannot read rom %s: %s\n", rom, strerror(errno));
		return 0;
	}

	/* read a maximum of MAX_ROM_SIZE bytes into memory starting after the reserved memory */
	fread(&memory[PROGRAM_START], 1, MAX_ROM_SIZE, romfile);

	fclose(romfile);
	return 1;
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
}

void
chip8_draw_sprite(int startx, int starty, uint16_t mem, uint8_t size)
{
	/*
	 * draw sprite located at loc of height size at startx,starty
	 */
	uint8_t byte = 0;
	uint8_t mask = 0x1;
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
				/* if the video bit is already set, we need to set the collision register */
				V[0xF] = (video[pixel] != 0) ? 1 : 0;
				video[pixel] ^= PIXEL_COLOR;
				draw_flag = 1;
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
	printf("\a");
	fflush(stdout);
}

void
unknown_opcode(uint16_t bad_opcode)
{
	fprintf(stderr, "unknown opcode: 0x%04X at 0x%02X\n", bad_opcode, PC-2); // minus 2 because we increment PC by 2 at the start of decoding
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
	 * we left shift the first byte by 8 (1 byte) to place it in the high byte
	 * and we store the second byte in the lower byte
	 */
	opcode = (memory[PC] << 8) | memory[PC+1];

	uint16_t nnn = opcode & 0x0FFF; 	// lowest 12 bits
	uint8_t n = opcode & 0x000F; 		// lowest 4 bits
	uint8_t x = (opcode >> 8) & 0x000F;	// lower 4 bits of the high byte, we discard the low byte by right shifting it out
	uint8_t y = (opcode >> 4) & 0x000F;	// upper 4 bits of the low byte, so we need to discard the lower 4 bits
	uint8_t kk = opcode & 0x00FF;		// lowest 8 bits
	//printf("parsing at 0x%03X opcode = 0x%04X\n", PC, opcode);
	PC += 2; // TODO: remove
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
			memset(video, 0, (WIDTH*HEIGHT) * sizeof(uint32_t));
			draw_flag = 1;
			break;
		case 0x00EE: /* ret (return from subroutine) */
			stack[SP] = 0x0;
			PC = stack[--SP];
			break;
		default: unknown_opcode(opcode);
		}
		break;
	}
	case 0x1000: /*JP addr (jump to memory address nnn) */
		PC = nnn;
		break;
	case 0x2000: /* CALL addr (call subroutine at addr, increment SP and put current PC on top of stack, set PC to nnn) */
		stack[SP++] = PC;
		PC = nnn;
		break;
	case 0x3000: /* SE Vx, byte (skip next instruction if Vx = kk) */
		// TODO: not tested
		PC += (V[x] == kk) ? 2 : 0;
		break;
	case 0x4000: /* SN Vx, byte (skip next instruction if Vx != kk) */
		// TODO: not tested
		PC += (V[x] != kk) ? 2 : 0;
		break;
	case 0x5000: /* SE Vx, Vy (skip next instruction if Vx == Vy) */
		// TODO: not tested
		PC += (V[x] == V[y]) ? 2 : 0;
		break;
	case 0x6000: /* LD Vx, byte (load byte in register x) */
	{
		V[x] = kk;
		break;
	}
	case 0x7000: /* ADD Vx, byte (add byte to Vx and store result in Vx) */
		V[x] = V[x] + kk;
		break;
	case 0x8000: /* math */
	{
		switch (n)
		{
		case 0x00: /* LD Vx, Vy (load Vy into Vx register) */
			// TODO: not tested
			V[x] = V[y];
			break;
		case 0x01: /* OR Vx, Vy (XOR Vx with Vy and store the result in Vx) */
			// TODO: not tested
			V[x] |= V[y];
			break;
		case 0x02: /* AND Vx, Vy (AND Vx with Vy and store the result in Vx) */
			// TODO: not tested
			V[x] &= V[y];
			break;
		case 0x03: /* XOR Vx, Vy (XOR Vx with Vy and store the result in Vx) */
			// TODO: not tested
			V[x] ^= V[y];
			break;
		case 0x04: /* ADD Vx, Vy (add Vx to Vy, store in Vx, if > 255 set V[F] 1, otherwise 0, Vx = lower 4 bits) */
			// TODO: not tested
		{
			V[0xF] = 0;
			uint16_t res = V[x] + V[y];
			if (res > 0xFF)
			{
				res &= 0xFF;
				V[0xF] = 1;
			}
			V[x] = res;
			break;
		}
		case 0x05: /* SUB Vx, Vy (subtract Vx from Vy, store in Vx, if Vx > Vy set V[F] 1, otherwise 0 */
			// TODO: not tested
			V[0xF] = (V[x] > V[y]) ? 1 : 0;
			V[x] -= V[y];
			break;
		case 0x06: /* SHR Vx (if the least significant bit of Vx is 1, V[F] set to 1, otherwise 0, then Vx is divided by 2 */
			// TODO: not tested
			V[0xF] = V[x] & 0x01;
			V[x] >>= 1;
			break;
		case 0x07: /* SUBN Vx, Vy (if Vy > Vx then set V[F] 1 otherwise 0, then Vx is subtracted from Vy, result stored in Vx */
			V[0xF] = (V[y] > V[x]) ? 1 : 0;
			V[x] = V[y] - V[x];
			break;
		case 0x0E: /* SHL Vx (if the most significant bit of Vx is 1, V[F] set to 1, otherwise 0, then Vx is multiplied by 2 */
			// TODO: not tested
			V[0xF] = (V[x] >> 7) & 0x01;
			V[x] <<= 1;
			break;
		default: unknown_opcode(opcode);
		}
		break;
	}
	case 0x9000: /* SNE Vx, Vy (skip next instruction if Vx != Vy) */
		// TODO: not tested
		PC += (V[x] != V[y]) ? 2 : 0;
		break;
	case 0xA000: /* LD I, addr (load register I with addr) */
		// TODO: not tested
		I = nnn;
		break;
	case 0xB000: /* JP V0, addr (set PC to location nnn + V0) */
		// TODO: not tested
		PC = V[0] + nnn;
		break;
	case 0xC000: /* RND Vx, byte (set Vx to a random byte and AND it by kk) */
		// TODO: not tested
		V[x] = (rand() % 256) & kk; // random number between 0 and 255
		break;
	case 0xD000: /* DRW Vx, Vy, n (read n bytes from starting starting at address in register I, and draws then at coordinates x,y */
		// TODO: not tested
		chip8_draw_sprite(V[x], V[y], I, n);
		break;
	case 0xE000: /* key input */
	{
		switch(kk)
		{
		case 0x9E: /* SKP Vx (skip next instruction if key Vx is pressed) */
			// TODO: not tested
			PC += (key[V[x]] == 1) ? 2 : 0;
			break;
		case 0xA1: /* SKNP Vx (skip next instruction if key Vx is not pressed) */
			// TODO: not tested
			PC += (key[V[x]] == 0) ? 2 : 0;
			break;
		default: unknown_opcode(opcode);
		}
		break;
	}
	case 0xF000:
	{
		switch (kk)
		{
		case 0x07: /* LD Vx, DT (load Vx with the value of delay timer register) */
			// TODO: not tested
			V[x] = delay_timer;
			break;
		case 0x0A: /* LD Vx, K (all execution stops until a key is pressed, then that key is put into Vx) */
		{
			// TODO: not tested
			int found = 0;
			for (int i = 0; i <= KEY_SIZE; i++)
			{
				if (key[i])
				{
					V[x] = i;
					found = 1;
				}
			}
			PC -= (found == 0) ? 2 : 0;
			break;
		}
		case 0x15: /* LD DT, Vx (delay timer is set to the value of Vx */
			// TODO: not tested
			delay_timer = V[x];
			break;
		case 0x18: /* LD ST, Vx (sound timer is set to the value of Vx */
			// TODO: not tested
			sound_timer = V[x];
			break;
		case 0x1E: /* ADD I, Vx (set I register to I + Vx) */
			// TODO: not tested
			I += V[x];
			break;
		case 0x29: /* LD F, Vx (set register I to location of the hex sprite Vx) */
			// TODO: not tested
			I = V[x] * 5;
			break;
		case 0x33: /* LD B, Vx (store BCD representation of Vx in memory locations I, I+1 and I+2) */
			// TODO: not tested
			memory[I]  = (V[x] % 1000) / 100;
			memory[I+1] = (V[x] % 100) / 10;
			memory[I+2] = (V[x] % 10);
			break;
		case 0x55: /* LD [I], Vx (store registers V0 through Vx starting at memory location I) */
		{
			// TODO: not tested
			for(int i = 0; i <= x; ++i)
			{
				memory[I + i] = V[i];
			}
			break;
		}
		case 0x65: /* LD Vx, [I] (read registers V0 through Vx from memory starting at I) */
		{
			// TODO: not tested
			for(int i = 0; i <= x; ++i)
			{
				V[i] = memory[I + i];
			}
			break;
		}
			break;

		default: unknown_opcode(opcode);
		}
		break;
	}
	default: unknown_opcode(opcode);
	}
}

void chip8_timer_cycle()
{
	/* timers */
	if (delay_timer > 0)
		delay_timer--;
	if (sound_timer > 0)
	{
		chip8_beep();
		sound_timer--;
	}
}
