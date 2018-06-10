#include <string.h>
#include "cart_bus.h"
#include "cart_emu.h"
#include "vram.h"

#define LCDCONT 0x0
#define LCDSTAT 0x1
#define SCROLLY 0x2
#define SCROLLX 0x3
#define CURLINE 0x4
#define CMPLINE 0x5
#define DMACONT 0x6
#define BGRDPAL 0x7
#define OBJ0PAL 0x8
#define OBJ1PAL 0x9
#define WNDPOSY 0xA
#define WNDPOSX 0xB

typedef struct {
	uint8_t Y;
	uint8_t X;
	uint8_t img;
	uint8_t flg;
} sprite;

uint8_t *VRAM = &cart_ram1[0x2000]; // 0x8000 -> 0x9FFF (used up to 0x9800+0x400 (32*32) = $9C00)
uint8_t *ScreenRAM = &cart_ram1[0]; // 0xA000 -> 0xBDFF (40*192 = 0x1E00)
sprite *OAM = (sprite *)&cart_ram1[0x3E00]; // 0x9E00 -> 0x9E9F (40*4 = 160 = 0xA0)
uint8_t *SPARE = &cart_ram1[0x3EA0]; // 0x9EA0 -> 0x9F3F (160 = 0xA0)
uint8_t *REGS = &cart_ram1[0x3F40]; // 0x9F40 -> 0x9F4B (12 bytes)
uint8_t *ScreenLine; // dynamic view into ScreenRAM

#define SPRPRIOR 0x80
#define SPRFLIPY 0x40
#define SPRFLIPX 0x20
#define SPRPALLT 0x10

#define MAX_SPRITE 18

uint8_t nes2a8[0x100] = {
		0x00, 0x02, 0x08, 0x0A, 0x20, 0x22, 0x28, 0x2A, 0x80, 0x82, 0x88, 0x8A, 0xA0, 0xA2, 0xA8, 0xAA,
		0x01, 0x03, 0x09, 0x0B, 0x21, 0x23, 0x29, 0x2B, 0x81, 0x83, 0x89, 0x8B, 0xA1, 0xA3, 0xA9, 0xAB,
		0x04, 0x06, 0x0C, 0x0E, 0x24, 0x26, 0x2C, 0x2E, 0x84, 0x86, 0x8C, 0x8E,	0xA4, 0xA6, 0xAC, 0xAE,
		0x05, 0x07, 0x0D, 0x0F, 0x25, 0x27, 0x2D, 0x2F, 0x85, 0x87, 0x8D, 0x8F, 0xA5, 0xA7, 0xAD, 0xAF,
		0x10, 0x12, 0x18, 0x1A, 0x30, 0x32, 0x38, 0x3A, 0x90, 0x92, 0x98, 0x9A, 0xB0, 0xB2, 0xB8, 0xBA,
		0x11, 0x13, 0x19, 0x1B, 0x31, 0x33, 0x39, 0x3B, 0x91, 0x93, 0x99, 0x9B, 0xB1, 0xB3, 0xB9, 0xBB,
		0x14, 0x16, 0x1C, 0x1E, 0x34, 0x36, 0x3C, 0x3E, 0x94, 0x96, 0x9C, 0x9E, 0xB4, 0xB6, 0xBC, 0xBE,
		0x15, 0x17, 0x1D, 0x1F, 0x35, 0x37, 0x3D, 0x3F, 0x95, 0x97, 0x9D, 0x9F, 0xB5, 0xB7, 0xBD, 0xBF,
		0x40, 0x42, 0x48, 0x4A, 0x60, 0x62, 0x68, 0x6A, 0xC0, 0xC2, 0xC8, 0xCA, 0xE0, 0xE2, 0xE8, 0xEA,
		0x41, 0x43, 0x49, 0x4B, 0x61, 0x63, 0x69, 0x6B, 0xC1, 0xC3, 0xC9, 0xCB, 0xE1, 0xE3, 0xE9, 0xEB,
		0x44, 0x46, 0x4C, 0x4E, 0x64, 0x66, 0x6C, 0x6E, 0xC4, 0xC6, 0xCC, 0xCE, 0xE4, 0xE6, 0xEC, 0xEE,
		0x45, 0x47, 0x4D, 0x4F, 0x65, 0x67, 0x6D, 0x6F, 0xC5, 0xC7, 0xCD, 0xCF, 0xE5, 0xE7, 0xED, 0xEF,
		0x50, 0x52, 0x58, 0x5A, 0x70, 0x72, 0x78, 0x7A, 0xD0, 0xD2, 0xD8, 0xDA, 0xF0, 0xF2, 0xF8, 0xFA,
		0x51, 0x53, 0x59, 0x5B, 0x71, 0x73, 0x79, 0x7B, 0xD1, 0xD3, 0xD9, 0xDB, 0xF1, 0xF3, 0xF9, 0xFB,
		0x54, 0x56, 0x5C, 0x5E, 0x74, 0x76, 0x7C, 0x7E, 0xD4, 0xD6, 0xDC, 0xDE, 0xF4, 0xF6, 0xFC, 0xFE,
		0x55, 0x57, 0x5D, 0x5F, 0x75, 0x77, 0x7D, 0x7F, 0xD5, 0xD7, 0xDD, 0xDF, 0xF5, 0xF7, 0xFD, 0xFF,
};

void init_vram(void) {
	// clear VRAM
//	memset(VRAM, 0, BANK_SIZE);
	// clear Sprites
	memset(OAM, 0, MAX_SPRITE * 4);
	for (int n = 0; n < MAX_SPRITE; n++)
	{
		sprite *spr = &OAM[n];
		spr->Y = 0xF0;
	}
}

void refresh_vram(void) {
	uint8_t *CharRam = VRAM;
	uint8_t *NameTable = VRAM + 0x1800;
	uint8_t *ScreenBuf = ScreenRAM + 0x10; // helps align to 4K boundary
	uint8_t i, j, NoScroll;
	uint16_t nt_offset, cr_offset;
	uint16_t char_idx, char_lo, char_hi;
	uint16_t scrollx = REGS[SCROLLX], scrolly = REGS[SCROLLY];
	cart_d5xx[0x40] = 12 - (scrollx & 7);
	NoScroll = cart_d5xx[0x42];
	// processing background
	for (i = 0; i < 160; i++) {
		if (NoScroll > 0 && i == NoScroll) scrollx = 0;
		for (j = 0; j < 42; j++) {
			nt_offset = (((i + scrolly) / 8) * 32) + ((j / 2 + scrollx / 8) & 0x1F);
			char_idx = NameTable[nt_offset];
			char_idx += 0x80;
			char_idx &= 0xFF;
			cr_offset = 0x800 + (16 * char_idx);
			cr_offset += ((i + scrolly) % 8) * 2;
			char_hi = CharRam[cr_offset++];
			char_lo = CharRam[cr_offset++];
			if (j & 1) {
				char_lo &= 0xF;
				char_hi <<= 4;
				char_hi &= 0xF0;
			} else {
				char_lo >>= 4;
				char_hi &= 0xF0;
			}
			char_idx = char_lo | char_hi;
			ScreenBuf[(i*48) + j + 1] = nes2a8[char_idx];
			//ScreenBuf[((i/8)*8) + (j*8) + (i%8)] = nes2a8[char_idx];
		}
	}
	// sprites
	uint8_t col_map[4], m, n, MaxSprite;
	uint8_t SpriteData0, SpriteData1, SpriteData2;
	uint8_t SpriteMask0, SpriteMask1, SpriteMask2;
	int16_t x, y;

	MaxSprite = cart_d5xx[0x41];
	for (n = 0; n < MaxSprite; n++)
	{
		sprite *spr = &OAM[n];
		cr_offset = 16 * spr->img;
		char_idx = REGS[(spr->flg & SPRPALLT) ? OBJ1PAL : OBJ0PAL];
		col_map[0] = char_idx & 3;
		char_idx >>= 2;
		col_map[1] = char_idx & 3;
		char_idx >>= 2;
		col_map[2] = char_idx & 3;
		char_idx >>= 2;
		col_map[3] = char_idx & 3;
		m = (REGS[LCDCONT] & 4) ? 16 : 8;
		for (i = 0; i < m; i++)
		{
			char_lo = CharRam[cr_offset++];
			char_hi = CharRam[cr_offset++];
			y = spr->Y - 16;
			y += (spr->flg & SPRFLIPY) ? m - 1 - i : i;
			if (y >= 0 && y < 144)
			{
				SpriteData0 = SpriteData1 = 0;
				SpriteMask0 = SpriteMask1 = 0;
				SpriteData2 = 0;
				SpriteMask2 = 0xFF;
				// prepare image
				for (j = 0; j < 8; j++)
				{
					x = ((spr->flg & SPRFLIPX) ? 7-j : j);
					char_idx = 1 << x;
					char_idx = ((char_hi & char_idx) << 1) | (char_lo & char_idx);
					char_idx >>= x;
					if (j < 4)
					{
						SpriteMask1 |= ((char_idx ? 0 : 3) << (2 * (j & 3)));
						char_idx = col_map[char_idx];
						SpriteData1 |= (char_idx << (2 * (j & 3)));
					}
					else
					{
						SpriteMask0 |= ((char_idx ? 0 : 3) << (2 * (j & 3)));
						char_idx = col_map[char_idx];
						SpriteData0 |= (char_idx << (2 * (j & 3)));
					}
				}
				x = spr->X + (scrollx & 7);
				switch (x & 3)
				{
				case 1:
					SpriteData2 = (SpriteData1 & 3) << 6;
					SpriteData1 >>= 2;
					SpriteData1 |= (SpriteData0 & 3) << 6;
					SpriteData0 >>= 2;
					SpriteMask2 = ((SpriteMask1 & 3) << 6) | 0x3F;
					SpriteMask1 >>= 2;
					SpriteMask1 |= (SpriteMask0 & 3) << 6;
					SpriteMask0 >>= 2;
					SpriteMask0 |= 0xC0;
					break;
				case 2:
					SpriteData2 = (SpriteData1 & 15) << 4;
					SpriteData1 >>= 4;
					SpriteData1 |= (SpriteData0 & 15) << 4;
					SpriteData0 >>= 4;
					SpriteMask2 = ((SpriteMask1 & 15) << 4) | 0x0F;
					SpriteMask1 >>= 4;
					SpriteMask1 |= (SpriteMask0 & 15) << 4;
					SpriteMask0 >>= 4;
					SpriteMask0 |= 0xF0;
					break;
				case 3:
					SpriteData2 = (SpriteData1 << 2) & 0xFC;
					SpriteData1 >>= 6;
					SpriteData1 |= (SpriteData0 << 2) & 0xFC;
					SpriteData0 >>= 6;
					SpriteMask2 = ((SpriteMask1 << 2) & 0xFC) | 0x03;
					SpriteMask1 >>= 6;
					SpriteMask1 |= (SpriteMask0 << 2) & 0xFC;
					SpriteMask0 >>= 6;
					SpriteMask0 |= 0xFC;
					break;
				}
				x >>= 2;
				//x -= 2;
				y *= 48;
				if (x >= 0 && x < 40)
				{
					ScreenBuf[y + x] &= SpriteMask0;
					ScreenBuf[y + x] |= SpriteData0;
				}
				x++;
				if (x >= 0 && x < 40)
				{
					ScreenBuf[y + x] &= SpriteMask1;
					ScreenBuf[y + x] |= SpriteData1;
				}
				x++;
				if (x >= 0 && x < 40)
				{
					ScreenBuf[y + x] &= SpriteMask2;
					ScreenBuf[y + x] |= SpriteData2;
				}
			}
		}
	}

	ScreenLine = ScreenBuf;
}

#ifdef USE_CCTL_INTERRUPT
uint8_t cmd_received = 0;

/* Set interrupt handlers */
/* Handle CCTL interrupt */
void EXTI4_IRQHandler(void) {
	/* Make sure that interrupt flag is set */
	if (EXTI_GetITStatus(EXTI_Line4) != RESET) {
		uint16_t bus_addr, c;
		// wait for phi2 high
		while (!((c = CONTROL_IN) & PHI2)) ;
		bus_addr = ADDR_IN & 0xFF;
		if (c & RW) {
			// read
			SET_DATA_MODE_OUT
			DATA_OUT = ((bus_addr < 40) ? *ScreenLine++ : cart_d5xx[bus_addr]) << 8;
			// wait for phi2 low
			while (CONTROL_IN & PHI2) ;
			SET_DATA_MODE_IN
		} else {
			// write
			while (CONTROL_IN & PHI2) ;
			// read data bus on falling edge of phi2
			cart_d5xx[bus_addr] = DATA_IN >> 8;
			if (bus_addr == 0xDF)	// write to $D5DF
				cmd_received = 1;
		}
		/* Clear interrupt flag */
		EXTI_ClearITPendingBit(EXTI_Line4);
	}
}
#endif
