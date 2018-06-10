#include <string.h>
#include <string.h>
#include <math.h>
#include "cart_bus.h"
#include "cart_emu.h"
#include "vram_m7.h"

extern uint8_t *nes2a8;

#define SWIDTH 160
#define SHEIGHT 208

#define _2PI 0.024543693

#define TILESIZE 8
#define MAPSIZE 32

#define KEY_ESCAPE 32
#define KEY_SPACE 16
#define KEY_UP 8
#define KEY_DOWN 4
#define KEY_LEFT 2
#define KEY_RIGHT 1
#define KEY_Z 128
#define KEY_X 64
#define KEY_H 32
#define KEY_J 16
#define KEY_Q 8
#define KEY_W 4
#define KEY_E 2
#define KEY_R 1

/* MODE_7_PARAMS is a struct containing all the different parameters
that are relevant for Mode 7, so you can pass them to the functions
as a single unit */

float pos_x = 128.0;
float pos_y = 0.0;
float dx = 0.0;
float dy = 0.0;
float space_z = 40.0;
float scale_x = 100.0;
float scale_y = 300.0;
int horizon = -22;
int angle = 192;
float speed = 0.8;

void init_m7_vram(void) {
	pos_x = 128.0;
	pos_y = 0.0;
	dx = 0.0;
	dy = 0.0;
	space_z = 40.0;
	scale_x = 100.0;
	scale_y = 300.0;
	horizon = -22;
	angle = 192;
	speed = 0.8;
}

unsigned char get_tile(int x, int y)
{
	unsigned char *NameTable = VRAM + 0x1800;
	unsigned int char_idx, nt_offset = x + y * 32;
	char_idx = NameTable[nt_offset];
	char_idx += 0x80;
	char_idx &= 0xFF;
	return char_idx;
}

unsigned char get_pixel(int tile, int x, int y)
{
	unsigned char *CharRam = VRAM;
	unsigned int cr_offset = 0x800 + (16 * tile);
	unsigned int char_val, char_lo, char_hi;
	cr_offset += (y % 8) * 2;
	char_hi = CharRam[cr_offset++];
	char_lo = CharRam[cr_offset++];
	if (x > 3) {
		char_lo &= 0xF;
		char_hi <<= 4;
		char_hi &= 0xF0;
	}
	else {
		char_lo >>= 4;
		char_hi &= 0xF0;
	}
	char_val = nes2a8[char_lo | char_hi];

	switch (x & 3)
	{
	case 0:
		return (char_val >> 6) & 3;
	case 1:
		return (char_val >> 4) & 3;
	case 2:
		return (char_val >> 2) & 3;
	case 3:
		return char_val & 3;
	}
}

void put_pixel(int x, int y, unsigned int c)
{
	uint8_t *ScreenBuf = ScreenRAM + 0x10; // helps align to 4K boundary
	ScreenBuf += (40 * y) + (x / 4);
	unsigned char s = *ScreenBuf;
	unsigned char z = (0xC0 >> ((x % 4) << 1));
	unsigned char m = 0xFF ^ z;
	c = (c << 6) | (c << 4) | (c << 2) | c;
	*ScreenBuf = (s & m) | (c & z);
}

void refresh_m7_vram(void) {
	unsigned char tile, c;

	dx = speed * cosf((float)angle*_2PI);
	dy = speed * sinf((float)angle*_2PI);

	pos_x += dx;
	pos_y += dy;

	// the distance and horizontal scale of the line we are drawing
	float distance, horizontal_scale;
	// step for points in space between two pixels on a horizontal line
	float line_dx, line_dy;
	// current space position
	float space_x, space_y;
	// masks to make sure we don't read pixels outside the tile
	int tile_x, tile_y;
	int pixel_x, pixel_y;
	// current screen position
	int screen_x, screen_y;

	for (screen_y = 0; screen_y < SHEIGHT; screen_y++)
	{
		// first calculate the distance of the line we are drawing
		distance = (space_z * scale_y) / (float)(screen_y + horizon);
		// then calculate the horizontal scale, or the distance between
		// space points on this horizontal line
		horizontal_scale = distance / scale_x;

		// calculate the dx and dy of points in space when we step
		// through all points on this line
		line_dx = -sinf((float)angle*_2PI) * horizontal_scale;
		line_dy = cosf((float)angle*_2PI) * horizontal_scale;

		// calculate the starting position
		space_x = pos_x + (distance * cosf((float)angle*_2PI)) - ((SWIDTH / 2) * line_dx);
		space_y = pos_y + (distance * sinf((float)angle*_2PI)) - ((SWIDTH / 2) * line_dy);

		// go through all points in this screen line
		for (screen_x = 0; screen_x < SWIDTH; screen_x++)
		{
			if (horizon < 0 && screen_y < (-2 * horizon))
			{
				c = 3;
			}
			else
			{
				tile_x = ((int)(space_x) >> 3) & (MAPSIZE - 1);
				tile_y = ((int)(space_y) >> 3) & (MAPSIZE - 1);

				tile = get_tile(tile_x, tile_y);

				pixel_x = (int)(space_x) & (TILESIZE - 1);
				pixel_y = (int)(space_y) & (TILESIZE - 1);

				// get a pixel from the tile and put it on the screen
				c = get_pixel(tile, pixel_x, pixel_y);
			}
			put_pixel(screen_x, screen_y, 3-c);

			// advance to the next position in space
			space_x += line_dx;
			space_y += line_dy;
		}
	}
}

void refresh_m7_process(unsigned char cmd1, unsigned char cmd2)
{
	// act on keyboard input
//	if (cmd1&KEY_ESCAPE)
//		quit = true;
	if (cmd1&KEY_SPACE)
		init_m7_vram();
	if (cmd1&KEY_UP && speed < 5.0)
		speed += 0.1;
	if (cmd1&KEY_DOWN && speed > -5.0)
		speed -= 0.1;
	if (cmd1&KEY_LEFT)
		angle = (angle - 4) & 0xFF;
	if (cmd1&KEY_RIGHT)
		angle = (angle + 4) & 0xFF;
	if (cmd2&KEY_Z)
		space_z += 5.0;
	if (cmd2&KEY_X)
		space_z -= 5.0;
	if (cmd2&KEY_Q && scale_x < 2000.0)
		scale_x += 20.0;
	if (cmd2&KEY_W && scale_x > 20.0)
		scale_x -= 20.0;
	if (cmd2&KEY_E && scale_y < 2000.0)
		scale_y += 20.0;
	if (cmd2&KEY_R && scale_x > 20.0)
		scale_y -= 20.0;
	if (cmd2&KEY_H)
		horizon++;
	if (cmd2&KEY_J)
		horizon--;
}
