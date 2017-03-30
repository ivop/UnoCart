/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GAMEBOY_VRAM_H
#define __GAMEBOY_VRAM_H

extern uint8_t *VRAM;
extern uint8_t *ScreenRAM;
extern uint8_t *ScreenLine;
extern uint8_t cmd_received;

extern void init_vram(void);
extern void refresh_vram(void);

#endif // __GAMEBOY_VRAM_H
