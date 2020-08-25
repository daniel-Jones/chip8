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

#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#include "chip8.h"

/* references
 * https://austinmorlan.com/posts/chip8_emulator/
 * http://www.multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter/
 * http://mattmik.com/files/chip8/mastering/chip8.html (lots of info)
 * http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#0.1
 * http://www.emulator101.com/chip-8-sprites.html
 * https://slack-files.com/T3CH37TNX-F3RKEUKL4-b05ab4930d?nojsmode=1
 */

//#define VIDEO_SCALE 5
#define STEPPING 0 // set to 1 to step manually through program

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;

void init_video();
void update_video();
void toggle_pixel(int x, int y);
void quit();
void usage(char *);
int handle_sdl_events();
void handle_key_down(int keycode);
void handle_key_up(int keycode);
uint8_t get_chip8_key(int keycode);
void print_registers();

int VIDEO_SCALE = 5;
int step_cycle = 0;

extern uint32_t video[WIDTH*HEIGHT];
extern int draw_flag;
extern uint8_t key[KEY_SIZE];
extern uint8_t V[REGISTER_COUNT];
extern uint16_t I;
extern uint16_t PC;
extern int8_t SP;
extern uint8_t delay_timer;
extern uint8_t sound_timer;
extern uint16_t stack[STACK_SIZE];

void
usage(char *program)
{
	printf("usage: %s [scale] [speed] [romfile]\nscale - pixel scaling (~5 recommended)\nspeed - how many cycles per second should be run (60-1000 or so, depends on the game)\n", program);
}

void
quit()
{
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void
init_video()
{
	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("chip8 interpreter", 0, 0, WIDTH*VIDEO_SCALE, HEIGHT*VIDEO_SCALE, SDL_WINDOW_SHOWN);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
}

void
update_video()
{
	SDL_UpdateTexture(texture, NULL, video, sizeof(video[0])*WIDTH);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void
toggle_pixel(int x, int y)
{
	// TODO: clean
	uint32_t dos = video[WIDTH*y+x];
	if (dos <= 0)
		video[WIDTH*y+x] = PIXEL_COLOR;
	else
		video[WIDTH*y+x] = 0;
}

void
print_registers()
{
	puts("DUMP");
	puts("--------------------------");
	for (int reg = 0; reg < REGISTER_COUNT; reg++)
	{
		printf("V[0x%01X] = 0x%02X\n", reg, V[reg]);
	}
	// minus 2 on PC because we increment PC by 2 at the start of decoding
	printf("PC = 0x%03X\nI = 0x%03X\nSP = 0x%02X\ndelay_timer = 0x%02X\nsound_timer = 0x%02X\n", PC-2, I, SP, delay_timer, sound_timer);
	puts("STACK:");
	for (int i = 0; i < 16; i++)
	{
		printf("[%d] = 0x%03X", i, stack[i]);
		if (i == 7) puts("");
	}
	puts("");
	puts("--------------------------");
}

void
handle_key_down(int keycode)
{
	switch (keycode)
	{
	/* detect interpreter debug keys */
	case SDLK_p: print_registers(); break;
	case SDLK_PERIOD: step_cycle = 1; break;
	default: break;
	}

	uint8_t k = get_chip8_key(keycode);
	if (k > 0xF) return; /* unknown key */
	key[k] = 1;
}

void
handle_key_up(int keycode)
{
	uint8_t k = get_chip8_key(keycode);
	if (k > 0xF) return; /* unknown key */
	key[k] = 0;
}

uint8_t
get_chip8_key(int keycode)
{
	switch(keycode)
	{
	case SDLK_1: return 0x1;
	case SDLK_2: return 0x2;
	case SDLK_3: return 0x3;
	case SDLK_4: return 0xC;
	case SDLK_q: return 0x4;
	case SDLK_w: return 0x5;
	case SDLK_e: return 0x6;
	case SDLK_r: return 0xD;
	case SDLK_a: return 0x7;
	case SDLK_s: return 0x8;
	case SDLK_d: return 0x9;
	case SDLK_f: return 0xE;
	case SDLK_z: return 0xA;
	case SDLK_x: return 0x0;
	case SDLK_c: return 0xB;
	case SDLK_v: return 0xF;
	default: return 0xFF;
	}
}

int
handle_sdl_events()
{
	SDL_Event e;
	while (SDL_PollEvent(&e) != 0)
	{
		switch (e.type)
		{
		case SDL_QUIT: return 1;
		case SDL_KEYDOWN:
		{
			switch(e.key.keysym.sym)
			{
			case SDLK_ESCAPE: return 1;
			default:
				handle_key_down(e.key.keysym.sym);
				break;
			}
			break;
		}
		case SDL_KEYUP:
			handle_key_up(e.key.keysym.sym);
			break;
		default: break;
		}
	}
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 4)
	{
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}


	VIDEO_SCALE = atoi(argv[1]);
	chip8_init();

	if (!load_rom(argv[3]))
	{
		fprintf(stderr, "cannot start interpreter\n");
		exit(EXIT_FAILURE);
	}

	init_video();

	const int fps = atoi(argv[2]);
	const uint32_t frame_delay = 1000/fps;
	uint32_t  frame_start;
	uint32_t frame_time;

	int do_quit = 0;

	while(!do_quit)
	{
		frame_start = SDL_GetTicks();

		// logic
		do_quit = handle_sdl_events();
		if (STEPPING)
		{
			if (step_cycle == 1)
			{
				chip8_cycle();

				if (draw_flag)
				{
					update_video();
					draw_flag = 0;
				}

				step_cycle = 0;
			}
		}
		else
		{
			chip8_cycle();

			if (draw_flag)
			{
				update_video();
				draw_flag = 0;
			}
		}

		frame_time = SDL_GetTicks() - frame_start;
		if (frame_delay > frame_time)
		{
			SDL_Delay(frame_delay - frame_time);
		}
	}

	quit();
	return EXIT_SUCCESS;
}
