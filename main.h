#ifndef _MAIN_H_
#define _MAIN_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "hw_config.h"
#include "stm32f10x.h"
#include "misc.h"

//#define assert_param(expr) ((expr) ? (void)0 : assert_failed((uint8_t *)__FILE__, __LINE__))
//  void assert_failed(uint8_t* file, uint32_t line);
#define assert_param(expr) ((void)0)
unsigned long itoa(int value, unsigned char *sp, int radix);
void delay_ms (u32 ms);
void systickInit (u16 frequency);
void menu_back(void);
void blink(u8);
#endif
