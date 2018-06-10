/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MODE7_H
#define __MODE7_H

#define CART_TYPE_M7				252

#define CART_CMD_REFRESH_M7_SCREEN		0x30
#define CART_CMD_LOAD_M7_VRAM			0x31
#define CART_CMD_PROCESS_MOVEMENT		0x32

extern void emulate_Mode7_vram(void);

#endif // __MODE7_H
