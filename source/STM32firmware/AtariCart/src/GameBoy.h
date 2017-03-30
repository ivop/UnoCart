/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GAMEBOY_H
#define __GAMEBOY_H

#define CART_TYPE_GB				253

#define CART_CMD_REFRESH_SCREEN		0x30
#define CART_CMD_LOAD_VRAM			0x31

extern void emulate_GameBoy_vram(void);

#endif // __GAMEBOY_H
