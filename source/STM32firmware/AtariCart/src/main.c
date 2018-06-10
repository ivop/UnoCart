/*
 * --------------------------------------
 * UNOCart Firmware (c)2016 Robin Edwards
 * --------------------------------------
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE

#include "defines.h"
#include "cart_bus.h"
#include "sdcard.h"
#include "uno_menu.h"
#include "atr_emu.h"
#include "xex_ldr.h"
#include "cart_emu.h"
#include "BootStrap.h"
#include "GameBoy.h"
#include "Mode7.h"

#include <stdio.h>
#include <string.h>

/*
 Theory of Operation
 -------------------
 Atari sends command to mcu on cart by writing to $D5DF ($D5E0-$D5FF = SDX)
 (extra paramters for the command in $D500-$D5DE)
 Atari must be running from RAM when it sends a command, since the mcu on the cart will
 go away at that point.
 Atari polls $D500 until it reads $11. At this point it knows the mcu is back
 and it is safe to rts back to code in cartridge ROM again.
 Results of the command are in $D501-$D5DF
 */

int main(void) {
	/* Output: LEDS - PB{0..1}, RD5 - PB2, RD4 - PB4 */
	config_gpio_leds_RD45();
	/* InOut: Data - PE{8..15} */
	config_gpio_data();
	/* In: Address - PD{0..15} */
	config_gpio_addr();
	/* In: Other Cart Input Sigs - PC{0..2, 4..5} */
	config_gpio_sig();

	RED_LED_ON
	init_sdcard();

	while (1) {
		GREEN_LED_OFF

		emulate_boot_rom();

		GREEN_LED_ON

		if (uno_menu_cmd()) {
			break;
		}
	}

	if (cartType == CART_TYPE_ATR) {
		emulate_disk_access();
	} else if (cartType == CART_TYPE_XEX) {
		feed_XEX_loader();
	} else if (cartType == CART_TYPE_GB) {
		emulate_GameBoy_vram();
	} else if (cartType == CART_TYPE_M7) {
		emulate_Mode7_vram();
	} else {
		emulate_cartridge();
	}
}
