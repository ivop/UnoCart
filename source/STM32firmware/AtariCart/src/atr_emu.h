/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ATR_EMU_H
#define __ATR_EMU_H

#include "tm_stm32f4_fatfs.h"

#define CART_TYPE_ATR				254

#define CART_CMD_MOUNT_ATR			0x20	// unused, done automatically by firmware
#define CART_CMD_READ_ATR_SECTOR	0x21
#define CART_CMD_WRITE_ATR_SECTOR	0x22
#define CART_CMD_ATR_HEADER			0x23

extern void emulate_disk_access(void);

#endif // __ATR_EMU_H
