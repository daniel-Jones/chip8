#include <stdio.h>
#include <SDL2/SDL.h>

#define WIDTH 64
#define HEIGHT 32

/* references
 * https://austinmorlan.com/posts/chip8_emulator/
 * http://www.multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter/
 * http://mattmik.com/files/chip8/mastering/chip8.html (lots of info)
 * http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#0.1
 */

#define VIDEO_SCALE 10

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;

void init_video();
void update_video();
void toggle_pixel(int x, int y);
void quit();

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

uint32_t video[WIDTH*HEIGHT];

void
update_video()
{
	static int count = 0;
	printf("frame %d\n", count++);
	/*
	for (uint32_t i = 0 ;i < WIDTH*HEIGHT; i++)
	{

		video[i] = (255<<16)|(255<<8)|255;
	}
	video[0] = (255);
	video[1] = (255<<8);
	video[2] = (255<<16);
	*/

	toggle_pixel(0, 0);
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
	init_video();

	const int fps = 60;
	const int frame_delay = 1000/fps;
	uint32_t  frame_start;
	uint32_t frame_time;
	int do_quit = 0;
	while(!do_quit)
	{
		frame_start = SDL_GetTicks();

		// logic
		update_video();

		frame_time = SDL_GetTicks() - frame_start;
		if (frame_delay > frame_time)
		{
			SDL_Delay(frame_delay - frame_time);
		}
	}

	quit();
	return 0;
}
