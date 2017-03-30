/* CARTRIDGE HANDLING */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CART_BUS_H
#define __CART_BUS_H

#include "stm32f4xx.h"

#define RD5_LOW GPIOB->BSRRH = GPIO_Pin_2;
#define RD4_LOW GPIOB->BSRRH = GPIO_Pin_4;
#define RD5_HIGH GPIOB->BSRRL = GPIO_Pin_2;
#define RD4_HIGH GPIOB->BSRRL = GPIO_Pin_4;

#define CONTROL_IN GPIOC->IDR
#define ADDR_IN GPIOD->IDR
#define DATA_IN GPIOE->IDR
#define DATA_OUT GPIOE->ODR

#define PHI2_RD (GPIOC->IDR & 0x0001)
#define S5_RD (GPIOC->IDR & 0x0002)
#define S4_RD (GPIOC->IDR & 0x0004)
#define S4_AND_S5_HIGH (GPIOC->IDR & 0x0006) == 0x6

#define PHI2	0x0001
#define S5		0x0002
#define S4		0x0004
#define CCTL	0x0010
#define RW		0x0020

#define SET_DATA_MODE_IN GPIOE->MODER = 0x00000000;
#define SET_DATA_MODE_OUT GPIOE->MODER = 0x55550000;

#define GREEN_LED_OFF GPIOB->BSRRH = GPIO_Pin_0;
#define RED_LED_OFF GPIOB->BSRRH = GPIO_Pin_1;
#define GREEN_LED_ON GPIOB->BSRRL = GPIO_Pin_0;
#define RED_LED_ON GPIOB->BSRRL = GPIO_Pin_1;

extern void config_gpio_leds_RD45(void);
extern void config_gpio_sig(void);
extern void config_gpio_data(void);
extern void config_gpio_addr(void);
extern void config_gpio_interrupt(void);

#endif // __CART_BUS_H
