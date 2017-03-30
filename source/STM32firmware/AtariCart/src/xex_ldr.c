#include "cart_bus.h"
#include "cart_emu.h" // for cart_ram1&2
#include "xex_ldr.h"

static unsigned char *ramPtr = &cart_ram1[0];
static uint32_t bank = 0;

void feed_XEX_loader(void) {
	__disable_irq();	// Disable interrupts
	RD5_LOW
	RD4_LOW
	GREEN_LED_OFF
	uint16_t bus_addr, bus_data, c;
	while (1) {
		// wait for phi2 high
		while (!((c = CONTROL_IN) & PHI2))
			;
		if (!(c & CCTL)) {
			// CCTL low
			if (c & RW) {
				// read
				SET_DATA_MODE_OUT
				bus_addr = ADDR_IN & 0xFF;
				bus_data = (uint16_t)(ramPtr[bus_addr]);
				DATA_OUT = bus_data << 8;
				GREEN_LED_ON
			} else {
				// write
				bus_addr = ADDR_IN & 0xFF;
				bus_data = DATA_IN;
				while (CONTROL_IN & PHI2)
					bus_data = DATA_IN;
				if (bus_addr == 0)
					bank = (bank & 0xFF00) | (bus_data >> 8);
				else if (bus_addr == 1)
					bank = (bank & 0x00FF) | (bus_data & 0xFF00);

				ramPtr = (bank & 0xFF00) ? &cart_ram2[0] : &cart_ram1[0];
				ramPtr += 256 * (bank & 0x00FF);
			}
		} else
			GREEN_LED_OFF
		// wait for phi2 low
		while (CONTROL_IN & PHI2)
			;
		SET_DATA_MODE_IN
	}
}
