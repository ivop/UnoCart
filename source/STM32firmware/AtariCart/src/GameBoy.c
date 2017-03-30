#include "cart_bus.h"
#include "cart_emu.h"
#include "vram.h"
#include "GameBoy.h"

#define CHARS_PER_ROW 48

void emulate_vram(void) {
	uint8_t charCount = CHARS_PER_ROW;
	uint16_t addr, data, c;
	__disable_irq();	// Disable interrupts
	while (1) {
		// wait for phi2 high
		while (!((c = CONTROL_IN) & PHI2)) ;

		addr = ADDR_IN;
		if (!(c & CCTL)) {
			// CCTL low
			addr &= 0xFF;
			if (c & RW) {
				if (addr < CHARS_PER_ROW) {
					// read (screen)
					SET_DATA_MODE_OUT
					DATA_OUT = ((uint16_t)(*ScreenLine++)) << 8;
					// wait for phi2 low
					while (CONTROL_IN & PHI2) ;
					SET_DATA_MODE_IN
					charCount++;
					if (addr == 0)
					{
						if (charCount < CHARS_PER_ROW)
						{
							for (charCount = CHARS_PER_ROW - charCount; charCount > 0; charCount--, ScreenLine++);
						}
						charCount = 0;
					}
				} else {
					// read (control)
					SET_DATA_MODE_OUT
					DATA_OUT = ((uint16_t)(cart_d5xx[addr])) << 8;
					// wait for phi2 low
					while (CONTROL_IN & PHI2) ;
					SET_DATA_MODE_IN
				}
			} else {
				data = DATA_IN;
				// read data bus on falling edge of phi2
				while (CONTROL_IN & PHI2)
					data = DATA_IN;
				cart_d5xx[addr] = data >> 8;
				if (addr == 0xDF)	// write to $D5DF
					break;
			}
		}
		else if (!(c & S5)) {
//			if (c & RW) {
				SET_DATA_MODE_OUT
				//addr = ADDR_IN;
				DATA_OUT = ((uint16_t)(ScreenRAM[addr])) << 8;
				// wait for phi2 low
				while (CONTROL_IN & PHI2) ;
				SET_DATA_MODE_IN
/*			} else {
				//addr = ADDR_IN;
				data = DATA_IN;
				// read data bus on falling edge of phi2
				while (CONTROL_IN & PHI2)
					data = DATA_IN;
				ScreenRAM[addr] = data >> 8;
			}
*/		}
		else if (!(c & S4)) {
			if (c & RW) {
				SET_DATA_MODE_OUT
				//addr = ADDR_IN;
				DATA_OUT = ((uint16_t)(VRAM[addr])) << 8;
				// wait for phi2 low
				while (CONTROL_IN & PHI2) ;
				SET_DATA_MODE_IN
			} else {
				//addr = ADDR_IN;
				data = DATA_IN;
				// read data bus on falling edge of phi2
				while (CONTROL_IN & PHI2)
					data = DATA_IN;
				VRAM[addr] = data >> 8;
			}
		}
	}
	__enable_irq();
}

#define CmdCommence() cart_d5xx[0xDE] = 0xFF
#define CmdComplete() cart_d5xx[0xDE] = 'X'

static void uno_GameBoy_cmd(void) {
	uint8_t cmd = cart_d5xx[0xDF];
	switch (cmd) {
	// rebuild the screen from VRAM
	case CART_CMD_REFRESH_SCREEN:
		refresh_vram();
		break;
	case CART_CMD_LOAD_VRAM:
		init_vram();
		ScreenRAM[0x1FFD] = 0xFF;
		break;
	}
}

void emulate_GameBoy_vram(void) {
	GREEN_LED_ON
	RD5_HIGH
	RD4_HIGH
	CmdComplete();
	while (1) {
		emulate_vram();
		RED_LED_OFF
		CmdCommence();
		uno_GameBoy_cmd();
		CmdComplete();
		RED_LED_ON
	}
}
