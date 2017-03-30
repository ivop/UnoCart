#include <string.h>
#include "cart_bus.h"
#include "cart_emu.h"
#include "uno_menu.h"
#include "atr_emu.h"

/* ATR Handling */

// ATR format
typedef struct {
	uint16_t signature;
	uint16_t pars;
	uint16_t secSize;
	uint16_t parsHigh;
	uint8_t flags;
	uint16_t protInfo;
	uint8_t unused[5];
} ATRHeader;

typedef struct {
	char path[256];
	ATRHeader atrHeader;
	int filesize;
	FIL fil;
} MountedATR;

#define ATR_HEADER_SIZE 16
#define ATR_SIGNATURE 0x0296

static MountedATR mountedATRs[1] = { 0 };

static FATFS FatFs;
static int doneFatFsInit = 0;

static int mount_atr(char *filename) {
	// returns 0 for success or error code
	// 1 = no media, 2 = no file, 3 = bad atr
	if (!doneFatFsInit) {
		if (f_mount(&FatFs, "", 1) != FR_OK)
			return 1;
		doneFatFsInit = 1;
	}
	MountedATR *mountedATR = &mountedATRs[0];
	if (f_open(&mountedATR->fil, filename, FA_READ | FA_WRITE) != FR_OK)
		return 2;
	UINT br;
	if (f_read(&mountedATR->fil, &mountedATR->atrHeader, ATR_HEADER_SIZE, &br)
			!= FR_OK|| br != ATR_HEADER_SIZE ||
			mountedATR->atrHeader.signature != ATR_SIGNATURE) {
		f_close(&mountedATR->fil);
		return 3;
	}
	// success
	strcpy(mountedATR->path, filename);
	mountedATR->filesize = f_size(&mountedATR->fil);
	return 0;
}

static int read_atr_sector(uint16_t sector, uint8_t page, uint8_t *buf) {
	// returns 0 for success or error code
	// 1 = no ATR mounted, 2 = invalid sector
	MountedATR *mountedATR = &mountedATRs[0];
	if (!mountedATR->path[0])
		return 1;
	if (sector == 0)
		return 2;

	int offset = ATR_HEADER_SIZE;
	// first 3 sectors are always 128 bytes
	if (sector <= 3)
		offset += (sector - 1) * 128;
	else
		offset += (3 * 128) + ((sector - 4) * mountedATR->atrHeader.secSize)
				+ (page * 128);
	// check we're not reading beyond the end of the file..
	if (offset > (mountedATR->filesize - 128)) {
		memset(buf, 0, 128);	// return blank sector?
		return 0;
	}
	UINT br;
	if (f_lseek(&mountedATR->fil, offset) != FR_OK
			|| f_read(&mountedATR->fil, buf, 128, &br) != FR_OK || br != 128)
		return 2;
	return 0;
}

static int write_atr_sector(uint16_t sector, uint8_t page, uint8_t *buf) {
	// returns 0 for success or error code
	// 1 = no ATR mounted, 2 = write error
	MountedATR *mountedATR = &mountedATRs[0];
	if (!mountedATR->path[0])
		return 1;
	if (sector == 0)
		return 2;

	int offset = ATR_HEADER_SIZE;
	// first 3 sectors are always 128 bytes
	if (sector <= 3)
		offset += (sector - 1) * 128;
	else
		offset += (3 * 128) + ((sector - 4) * mountedATR->atrHeader.secSize)
				+ (page * 128);
	// check we're not writing beyond the end of the file..
	if (offset > (mountedATR->filesize - 128))
		return 2;
	UINT bw;
	if (f_lseek(&mountedATR->fil, offset) != FR_OK
			|| f_write(&mountedATR->fil, buf, 128, &bw) != FR_OK
			|| f_sync(&mountedATR->fil) != FR_OK || bw != 128)
		return 2;
	return 0;
}

static void atr_emu_cmd(void) {
	uint16_t sector;
	uint8_t offset;
	int ret;
	switch (cart_d5xx[0xDF]) {
	case CART_CMD_READ_ATR_SECTOR:
		//uint8_t device = cart_d5xx[0x00];
		sector = (cart_d5xx[0x02] << 8) | cart_d5xx[0x01];
		offset = cart_d5xx[0x03];// 0 = first 128 byte "page", 1 = second, etc
		ret = read_atr_sector(sector, offset, &cart_d5xx[0x02]);
		cart_d5xx[0x01] = ret;
		break;
	case CART_CMD_WRITE_ATR_SECTOR:
		//uint8_t device = cart_d5xx[0x00];
		GREEN_LED_OFF
		RED_LED_ON
		sector = (cart_d5xx[0x02] << 8) | cart_d5xx[0x01];
		offset = cart_d5xx[0x03];// 0 = first 128 byte "page", 1 = second, etc
		ret = write_atr_sector(sector, offset, &cart_d5xx[0x04]);
		cart_d5xx[0x01] = ret;
		RED_LED_OFF
		break;
	case CART_CMD_ATR_HEADER:
		//uint8_t device = cart_d5xx[0x00];
		if (!&mountedATRs[0].path[0])
			cart_d5xx[0x01] = 1;
		else {
			memcpy(&cart_d5xx[0x02], &mountedATRs[0].atrHeader, 16);
			cart_d5xx[0x01] = 0;
		}
		break;
	}
}

static void emulate_disk_atr(void) {
	__disable_irq();	// Disable interrupts
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
				bus_data = (uint16_t)(cart_d5xx[bus_addr]);
				DATA_OUT = bus_data << 8;
				// wait for phi2 low
				while (CONTROL_IN & PHI2)
					;
				SET_DATA_MODE_IN
			} else {
				// write
				bus_addr = ADDR_IN & 0xFF;
				bus_data = DATA_IN;
				// read data bus on falling edge of phi2
				while (CONTROL_IN & PHI2)
					bus_data = DATA_IN;
				bus_data >>= 8;
				cart_d5xx[bus_addr] = bus_data;
				if (bus_addr == 0xDF)	// write to $D5DF
					break;
			}
		}
	}
	__enable_irq();
}

void emulate_disk_access(void) {
	RED_LED_OFF
	RD5_LOW
	RD4_LOW
	cart_d5xx[0x00] = 0x11;	// signal that we are here
	int ret = mount_atr(path);
	if (ret == 0)
		memcpy(&cart_d5xx[0x02], &mountedATRs[0].atrHeader, 16);
	cart_d5xx[0x01] = ret;

	while (1) {
		GREEN_LED_OFF
		emulate_disk_atr();
		GREEN_LED_ON

		atr_emu_cmd();
	}
}
