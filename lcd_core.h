#ifndef TEST1_ILI93xx_CORE_H
#define TEST1_ILI93xx_CORE_H

#define TRANSPARENT_COLOR CYAN

#define pgm_read_word(addr) (*(const unsigned short *)(addr))
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#define pgm_read_pointer(addr) ((void *)pgm_read_word(addr))
	

#ifndef _int16_swap
#define _int16_swap(a, b) { int16_t t = a; a = b; b = t; }
#endif

#ifndef abs
#define abs(a) ((a)<0?-(a):a)
#endif

#define ILI9341_COLOR(r, g, b)\
     ((((uint16_t)b) >> 3) |\
            ((((uint16_t)g) << 3) & 0x07E0) |\
            ((((uint16_t)r) << 8) & 0xf800))

#include "dma.h"
#include "stm32f10x.h"
#include "stm32f10x_spi.h"
#include "main.h"
#include "lcd_commands.h"
#include "dma.h"

#define LCD_setAddressWindowToWrite(x1,y1,x2,y2) \
	LCD_setAddressWindow(x1, y1, x2, y2); \
	dmaSendCmd(LCD_GRAM)

#define LCD_setAddressWindowToRead(x1,y1,x2,y2) \
	LCD_setAddressWindow(x1, y1, x2, y2); \
	dmaSendCmd(LCD_RAMRD)

u16 LCD_getWidth(void);
u16 LCD_getHeight(void);

void LCD_init(void);
void LAN_init(void);

static void LCD_pinsInit(void);

void LCD_setSpi16(void);
void LCD_setSpi8(void);
void SPI2_SetSpeed(u16);

void LCD_setOrientation(u8 o);
void LCD_setAddressWindow(u16 x1, u16 y1, u16 x2, u16 y2);

void TEST_fill(void);

void LCD_write(unsigned char c);
void LCD_writeString(unsigned char *s);
void LCD_writeString2(unsigned char *string);

void LCD_setCursor(u16 x, u16 y);
void LCD_setTextSize(u8 size);
void LCD_setTextColor(u16 color);
void LCD_setTextBgColor(u16 color);

u16 LCD_getCursorX(void);
u16 LCD_getCursorY(void);

void TEST_fillPrimitives(u16 step);

void LCD_readPixels(u16 x1, u16 y1, u16 x2, u16 y2, u16 *buf);

void LCD_fillRect(u16 x1, u16 y1, u16 w, u16 h, u32 color);
void LCD_fillScreen(u16 color);
void LCD_drawRect(u16 x, u16 y, u16 w, u16 h, u32 color);
void LCD_drawCircle(u16 x0, u16 y0, u16 r, u16 color);
void LCD_fillCircle(u16 x0, u16 y0, u16 r, u16 color);
void LCD_fillRect(u16 x1, u16 y1, u16 w, u16 h, u32 color);
void LCD_drawFastHLine(u16 x0, u16 y0, u16 w, u32 color);
void LCD_drawFastVLine(u16 x0, u16 y0, u16 h, u32 color);
void LCD_drawLine(u16 x0, u16 y0, u16 x1, u16 y1, u32 color);
void LCD_putPixel(u16 x, u16 y, u32 color);
static void LCD_putPixelCont(u16 x, u16 y, u32 color);
void LCD_putPixelCont(u16 x, u16 y, u32 color);

void LCD_drawChar(u16 x0, u16 y0, unsigned char c, u32 color, u16 bg, uint8_t size);
void LCD_write(unsigned char c);
void LCD_print(u16 x, u16 y, unsigned char *s,u32 tcolor,u32 bcolor,u8 size);
void LCD_DrawDate(u32 x,u32 y,u8 size);
void lcd_pcDebug(u8 *data,u8 len);
void lcd_rtxDebug(u8 *data,u8 len);
void Lcd_CS_Reset(void);

void Lcd_CS_Reset(void);

#endif //TEST1_ILI93xx_CORE_H
