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
 */

#define VIDEO_SCALE 5

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;

void init_video();
void update_video();
void toggle_pixel(int x, int y);
void quit();
void usage(char *);
void handle_key_down(int keycode);
void handle_key_up(int keycode);
uint8_t get_chip8_key(int keycode);

extern uint32_t video[WIDTH*HEIGHT];
extern int draw_flag;
extern uint8_t key[KEY_SIZE];

void
usage(char *program)
{
	printf("usage: %s [romfile]\n", program);
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
		video[WIDTH*y+x] = (255<<16)|(255<<8)|255;
	else
		video[WIDTH*y+x] = 0;
}

void
handle_key_down(int keycode)
{
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

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	if (!load_rom(argv[1]))
	{
		fprintf(stderr, "cannot start interpreter\n");
		exit(EXIT_FAILURE);
	}

	chip8_init();

	init_video();

	const int fps = 60;
	const int frame_delay = 1000/fps;
	uint32_t  frame_start;
	uint32_t frame_time;

	int do_quit = 0;

	chip8_draw_sprite(0, 0, 0xD*5, 0x5);
	chip8_draw_sprite(5, 0, 0xE*5, 0x5);
	chip8_draw_sprite(10, 0, 0xA*5, 0x5);
	chip8_draw_sprite(15, 0, 0xD*5, 0x5);
	chip8_draw_sprite(20, 0, 0xB*5, 0x5);
	chip8_draw_sprite(25, 0, 0xE*5, 0x5);
	chip8_draw_sprite(30, 0, 0xE*5, 0x5);
	chip8_draw_sprite(35, 0, 0xf*5, 0x5);

	chip8_draw_sprite(0,  9, 0xD*5, 0x5);
	chip8_draw_sprite(5,  9, 0xE*5, 0x5);
	chip8_draw_sprite(10, 9, 0xA*5, 0x5);
	chip8_draw_sprite(15, 9, 0xD*5, 0x5);
	chip8_draw_sprite(20, 9, 0xB*5, 0x5);
	chip8_draw_sprite(25, 9, 0xE*5, 0x5);
	chip8_draw_sprite(30, 9, 0xE*5, 0x5);
	chip8_draw_sprite(35, 9, 0xf*5, 0x5);

	chip8_draw_sprite(0, 20, 0x200, 0x6);
	chip8_draw_sprite(8, 20, 0x200, 0x6);
	chip8_draw_sprite(16, 20, 0x200, 0x6);
	chip8_draw_sprite(18, 20, 0x200, 0x6);

	chip8_draw_sprite(61, 25, 0x200, 0x6);
	chip8_draw_sprite(50, 30, 0x200, 0x6);

	SDL_Event e;
	while(!do_quit)
	{
		frame_start = SDL_GetTicks();

		// logic
		while (SDL_PollEvent(&e) != 0)
		{
			switch (e.type)
			{
				case SDL_QUIT:
					do_quit = 1;
					break;
				case SDL_KEYDOWN:
				{
					switch(e.key.keysym.sym)
					{
					case SDLK_ESCAPE:
						do_quit = 1;
						break;
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

		chip8_cycle();

		if (draw_flag)
			update_video();

		frame_time = SDL_GetTicks() - frame_start;
		if (frame_delay > frame_time)
		{
			SDL_Delay(frame_delay - frame_time);
		}
	}

	quit();
	return EXIT_SUCCESS;
}
