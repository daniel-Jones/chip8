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

extern uint32_t video[WIDTH*HEIGHT];
extern int draw_flag;

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
	static int count = 0;
	//printf("frame %d\n", count++);
	/*
	for (uint32_t i = 0 ;i < WIDTH*HEIGHT; i++)
	{

		video[i] = (255<<16)|(255<<8)|255;
	}
	video[0] = (255);
	video[1] = (255<<8);
	video[2] = (255<<16);
	*/

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

	while(!do_quit)
	{
		frame_start = SDL_GetTicks();

		// logic
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
