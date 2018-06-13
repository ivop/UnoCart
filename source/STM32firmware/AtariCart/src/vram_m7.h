/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MODE7_VRAM_H
#define __MODE7_VRAM_H

extern uint8_t nes2a8[];
extern uint8_t *VRAM;
extern uint8_t *ScreenRAM;

extern void init_m7_vram(void);
extern void prepare_m7_vram(void);
extern void refresh_m7_vram(void);
extern void refresh_m7_process(unsigned char cmd1, unsigned char cmd2);

#endif // __MODE7_VRAM_H
