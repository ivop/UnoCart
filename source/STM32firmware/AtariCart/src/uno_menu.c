#include "cart_bus.h"
#include "sdcard.h"
#include "uno_menu.h"
#include "atr_emu.h"
#include "cart_emu.h"
#include "xex_ldr.h"
#include "osrom.h"

#include <stdio.h>
#include <string.h>

char path[256];
static char curPath[256] = "";
static char searchStr[32];

uint8_t uno_menu_cmd(void) {
	uint8_t cmd = cart_d5xx[0xDF];
	uint8_t cmd_arg = cart_d5xx[0x00];
	DIR_ENTRY *entry = (DIR_ENTRY *) &cart_ram1[0];
	int ret, len;
	switch (cmd) {
	// OPEN ITEM n
	case CART_CMD_OPEN_ITEM:
		if (entry[cmd_arg].isDir) {	// directory
			strcat(curPath, "/");
			strcat(curPath, entry[cmd_arg].filename);
			cart_d5xx[0x01] = CMD_SUCCESS; // path changed
		} else {	// file/search result
			if (entry[cmd_arg].full_path[0])
				strcpy(path, entry[cmd_arg].full_path);	// search result
			else
				strcpy(path, curPath); // file in current directory
			strcat(path, "/");
			strcat(path, entry[cmd_arg].filename);
			if (stricmp(get_filename_ext(entry[cmd_arg].filename), "ATR")
					== 0) {	// ATR
				cart_d5xx[0x01] = 3;	// ATR
				cartType = CART_TYPE_ATR;
			} else {	// ROM,CAR or XEX
				cartType = load_file(path);
				if (cartType)
					cart_d5xx[0x01] = (cartType != CART_TYPE_XEX ? 1 : 2);// file was loaded
				else {
					cart_d5xx[0x01] = 4;	// error
					strcpy((char*) &cart_d5xx[0x02], errorBuf);
				}
			}
		}
		break;
		// READ DIR
	case CART_CMD_READ_CUR_DIR:
		ret = read_directory(curPath);
		if (ret) {
			cart_d5xx[0x01] = CMD_SUCCESS;	// ok
			cart_d5xx[0x02] = num_dir_entries;
		} else {
			cart_d5xx[0x01] = CMD_ERROR;	// error
			strcpy((char*) &cart_d5xx[0x02], errorBuf);
		}
		break;
		// GET DIR ENTRY n
	case CART_CMD_GET_DIR_ENTRY:
		cart_d5xx[0x01] = entry[cmd_arg].isDir;
		strcpy((char*) &cart_d5xx[0x02], entry[cmd_arg].long_filename);
		break;
		// UP A DIRECTORY LEVEL
	case CART_CMD_UP_DIR:
		len = strlen(curPath);
		while (len && curPath[--len] != '/')
			;
		curPath[len] = 0;
		break;
		// ROOT DIR (when atari reset pressed)
	case CART_CMD_ROOT_DIR:
		curPath[0] = 0;
		break;
		// SEARCH str
	case CART_CMD_SEARCH:
		strcpy(searchStr, (char*) &cart_d5xx[0x00]);
		ret = search_directory(curPath, searchStr);
		if (ret) {
			cart_d5xx[0x01] = CMD_SUCCESS;	// ok
			cart_d5xx[0x02] = num_dir_entries;
		} else {
			cart_d5xx[0x01] = CMD_ERROR;	// error
			strcpy((char*) &cart_d5xx[0x02], errorBuf);
		}
		break;
	case CART_CMD_LOAD_SOFT_OS:
		ret = load_file("UNO_OS.ROM");
		if (!ret) {
			for (int i = 0; i < 16384; i++)
				cart_ram1[i] = os_rom[i];
		}
		cart_d5xx[0x01] = CMD_SUCCESS;	// ok
		break;
	case CART_CMD_SOFT_OS_CHUNK:
		for (int i = 0; i < 128; i++)
			cart_d5xx[0x01 + i] = cart_ram1[cmd_arg * 128 + i];
		break;
		// NO CART
	case CART_CMD_NO_CART:
		cartType = 0;
		break;
		// REBOOT TO CART
	case CART_CMD_ACTIVATE_CART:
		return 1;
	}
	return 0;
}
