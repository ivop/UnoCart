#include "cart_bus.h"
#include "cart_emu.h"
#include "uno_menu.h"
#include "BootStrap.h"

#include "rom.h"

void emulate_boot_rom(void) {
	__disable_irq();	// Disable interrupts
	RD5_HIGH
	RD4_LOW
	cart_d5xx[0x00] = 0x11;	// signal that we are here
	uint16_t addr, data, c;
	while (1) {
		// wait for phi2 high
		while (!((c = CONTROL_IN) & PHI2))
			;

		if (!(c & CCTL)) {
			// CCTL low
			if (c & RW) {
				// read
				SET_DATA_MODE_OUT
				addr = ADDR_IN;
				DATA_OUT = ((uint16_t)(cart_d5xx[addr & 0xFF])) << 8;
				// wait for phi2 low
				while (CONTROL_IN & PHI2)
					;
				SET_DATA_MODE_IN
			} else {
				// write
				addr = ADDR_IN;
				data = DATA_IN;
				// read data bus on falling edge of phi2
				while (CONTROL_IN & PHI2)
					data = DATA_IN;
				cart_d5xx[addr & 0xFF] = data >> 8;
				if ((addr & 0xFF) == 0xDF)	// write to $D5DF
					break;
			}
		}
		if (!(c & S5)) {
			// normal cartridge read
			SET_DATA_MODE_OUT
			addr = ADDR_IN;
			DATA_OUT = ((uint16_t)(UnoCart_rom[addr])) << 8;
			// wait for phi2 low
			while (CONTROL_IN & PHI2)
				;
			SET_DATA_MODE_IN
		}
	}
	__enable_irq();
}
