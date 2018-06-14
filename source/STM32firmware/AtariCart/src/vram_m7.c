#include <string.h>
#include <math.h>
#include "cart_bus.h"
#include "cart_emu.h"
#include "vram_m7.h"

#define SWIDTH 160
#define SHEIGHT 200

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

static float pos_x = 128.0;
static float pos_y = 0.0;
static float space_z = 40.0;
static float scale_x = 100.0;
static float scale_y = 300.0;
static int horizon = -22;
static int angle = 192;
static float speed = 0.8;

static int offset;
static uint8_t *ScreenBuf;
static uint8_t *CharRam;
static uint8_t *NameTable;
static uint8_t *BackBuf;
static uint8_t colmsk[4] = { 0x00, 0x55, 0xAA, 0xFF };

void init_m7_vram(void) {
	ScreenBuf = ScreenRAM + 0x10; // helps align to 4K boundary
	CharRam = VRAM;
	NameTable = VRAM + 0x1800;
	BackBuf = &cart_ram1[0x4000];
	pos_x = 128.0;
	pos_y = 0.0;
	space_z = 40.0;
	scale_x = 100.0;
	scale_y = 300.0;
	horizon = -22;
	angle = 192;
	speed = 0.8;
}

void prepare_m7_screen(int line_start, int line_end) {
	uint8_t tile, c;
	uint8_t char_hi, char_lo;
	uint8_t s, z, m;
	uint16_t cr_offset;

	float angle_2PI = (float) angle * _2PI;
	float cos_angle_2PI = cosf(angle_2PI);
	float sin_angle_2PI = sinf(angle_2PI);

	if (line_start == 0)
	{
		offset = 0;
		pos_x += (speed * cos_angle_2PI);
		pos_y += (speed * sin_angle_2PI);
	}

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

	for (screen_y = line_start; screen_y < line_end; screen_y++) {
		// first calculate the distance of the line we are drawing
		distance = (space_z * scale_y) / (float) (screen_y + horizon);
		// then calculate the horizontal scale, or the distance between
		// space points on this horizontal line
		horizontal_scale = distance / scale_x;

		// calculate the dx and dy of points in space when we step
		// through all points on this line
		line_dx = -sin_angle_2PI * horizontal_scale;
		line_dy = cos_angle_2PI * horizontal_scale;

		// calculate the starting position
		space_x = pos_x + (distance * cos_angle_2PI) - ((SWIDTH / 2) * line_dx);
		space_y = pos_y + (distance * sin_angle_2PI) - ((SWIDTH / 2) * line_dy);

		// go through all points in this screen line
		for (screen_x = 0; screen_x < SWIDTH; screen_x++) {
			if (horizon < 0 && screen_y < (-2 * horizon)) {
				c = 3;
			} else {
				tile_x = ((int) (space_x) >> 3) & (MAPSIZE - 1);
				tile_y = ((int) (space_y) >> 3) & (MAPSIZE - 1);

				//tile = get_tile(tile_x, tile_y);
				tile = NameTable[tile_x + (tile_y * 32)];
				tile += 0x80;
				tile &= 0xFF;

				pixel_x = (int) (space_x) & (TILESIZE - 1);
				pixel_y = (int) (space_y) & (TILESIZE - 1);

				// get a pixel from the tile and put it on the screen
				//c = get_pixel(tile, pixel_x, pixel_y);
				cr_offset = 0x800 + (16 * tile) + ((pixel_y & 7) * 2);
				char_hi = CharRam[cr_offset++];
				char_lo = CharRam[cr_offset++];
				if (pixel_x > 3) {
					char_lo &= 0xF;
					char_hi <<= 4;
					char_hi &= 0xF0;
				} else {
					char_lo >>= 4;
					char_hi &= 0xF0;
				}

				c = nes2a8[char_lo | char_hi];
				switch (pixel_x & 3) {
				case 0:
					c >>= 6;
					break;
				case 1:
					c >>= 4;
					break;
				case 2:
					c >>= 2;
					break;
				}
				c &= 3;
			}
			//put_pixel(screen_x, screen_y, c);
			s = BackBuf[offset];
			z = (0xC0 >> ((screen_x & 3) << 1));
			m = 0xFF ^ z;
			BackBuf[offset] = (s & m) | (colmsk[c] & z);
			if (z == 3) offset++;

			// advance to the next position in space
			space_x += line_dx;
			space_y += line_dy;
		}
	}
}

void prepare_m7_vram(void) {
	angle += 1;
	prepare_m7_screen(0, (SHEIGHT / 2) + 8);
}

void refresh_m7_vram(void) {
	prepare_m7_screen((SHEIGHT / 2) + 8, SHEIGHT);
	memcpy(ScreenBuf, BackBuf, (SWIDTH / 4) * SHEIGHT);
}

void refresh_m7_process(uint8_t cmd1, uint8_t cmd2) {
	// act on keyboard input
//	if (cmd1&KEY_ESCAPE)
//		quit = true;
	if (cmd1 & KEY_SPACE)
		init_m7_vram();
	if (cmd1 & KEY_UP && speed < 5.0)
		speed += 0.1;
	if (cmd1 & KEY_DOWN && speed > -5.0)
		speed -= 0.1;
	if (cmd1 & KEY_LEFT)
		angle = (angle - 4) & 0xFF;
	if (cmd1 & KEY_RIGHT)
		angle = (angle + 4) & 0xFF;
	if (cmd2 & KEY_Z)
		space_z += 5.0;
	if (cmd2 & KEY_X)
		space_z -= 5.0;
	if (cmd2 & KEY_Q && scale_x < 2000.0)
		scale_x += 20.0;
	if (cmd2 & KEY_W && scale_x > 20.0)
		scale_x -= 20.0;
	if (cmd2 & KEY_E && scale_y < 2000.0)
		scale_y += 20.0;
	if (cmd2 & KEY_R && scale_x > 20.0)
		scale_y -= 20.0;
	if (cmd2 & KEY_H && horizon < 0)
		horizon++;
	if (cmd2 & KEY_J)
		horizon--;
}
