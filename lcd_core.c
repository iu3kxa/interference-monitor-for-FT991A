#include "lcd_core.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_bkp.h"
#include "lcd_glcdfont.h"
#include "rtc.h"
#include "main.h"

u16 screen_width,screen_height;
u8 _cp437 = 0;
u16 cursorX = 0,cursorY = 0;
u8 textSize = 1;
u8 wrap = 1;
u16 textColor   = GREEN,    textBgColor = TRANSPARENT_COLOR;
extern u32 lcd_type;
u16 spi2_cr2_val;

extern u16 dma16BufIndex;
extern u16 dma16Buffer[DMA16_BUF_SIZE];
extern u8 dma8BufIndex;
extern u8 dma8Buffer[DMA8_BUF_SIZE];
extern struct Time_s s_TimeStructVar;
extern struct Date_s s_DateStructVar;
extern unsigned char buf[16];

//<editor-fold desc="Init commands">

static const uint8_t init_commands_9488[] =
{
	5, 0xf7, 0xa9, 0x51, 0x2c, 0x82,
	3, 0xc0, 0x11, 0x09,
	2, 0xc1, 0x41,
	4, 0xc5, 0x00, 0x0a, 0x80,
	3, 0xb1, 0xb0, 0x11,
	2, 0xb4, 0x02,
	3, 0xb6, 0x02,  0x42,
	//4, 0xb6, 0x02, 0x22, 0x3b,
	2, 0xb7, 0xc6,
	3, 0xbe, 0x00, 0x04,
	2, 0xe9, 0x00,
	//2, 0x36, 0x08,				/* Memory Access Control register*/
	2, 0x36, 0x68,
	2, 0x3a, 0x66,
	16,0xe0, 0x00, 0x07, 0x10, 0x09, 0x17, 0x0b, 0x41, 0x89, 0x4b, 0x0a, 0x0c, 0x0e, 0x18, 0x1b, 0x0f,
	16,0xe1, 0x00, 0x17, 0x1a, 0x04, 0x0e, 0x06, 0x2f, 0x45, 0x43, 0x02, 0x0a, 0x09, 0x32, 0x36, 0x0f,
	1,	0x11,
	0
};


//</editor-fold>

//<editor-fold desc="LCD initialization functions">

static void LCD_pinsInit()
{
	SPI_InitTypeDef  spiStructure;
	GPIO_InitTypeDef gpioStructure;

	//RCC_PCLK1Config(RCC_HCLK_Div1);			//36 mhz
	RCC_PCLK1Config(RCC_HCLK_Div1);				//2 mhz lento per debug
	RCC_PCLK2Config(RCC_HCLK_Div1);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//enable clock gpioa
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	//enable clock gpiob
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);	//enable clock spi2
	RCC_APB2PeriphClockCmd(RCC_APB2ENR_AFIOEN, ENABLE);	//enable clock alternate funcion


	// GPIO speed by default
	gpioStructure.GPIO_Speed = GPIO_Speed_50MHz;

		
	// GPIO for CS/DC/LED/RESET
	gpioStructure.GPIO_Pin  = RESET_PIN | LCD_DC_PIN | LCD_LED_PIN | TOUCH_CS_PIN;
	gpioStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &gpioStructure);
	
	// GPIO for CS
	gpioStructure.GPIO_Pin  = LAN_CS_PIN|LCD_CS_PIN;
	gpioStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &gpioStructure);
	
	// GPIO for clock mosi
	gpioStructure.GPIO_Pin  = SPI2_CLK_PIN | SPI2_MOSI_PIN;
	gpioStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &gpioStructure);
	
	//gpio for miso
	gpioStructure.GPIO_Pin  = SPI2_MISO_PIN;
	gpioStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &gpioStructure);
	delay_ms(1);
	
	
	//GPIO_PinRemapConfig(GPIO_Remap_SPI1,ENABLE);

	SPI_I2S_DeInit(SPI2);
	SPI_StructInit(&spiStructure);
	spiStructure.SPI_Mode              = SPI_Mode_Master;
	spiStructure.SPI_Direction 		  = SPI_Direction_2Lines_FullDuplex;
	spiStructure.SPI_NSS               = SPI_NSS_Soft;
	spiStructure.SPI_CPOL              = SPI_CPOL_High;
	spiStructure.SPI_CPHA              = SPI_CPHA_2Edge;
	spiStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	//spiStructure.SPI_CRCPolynomial = 7;
	spiStructure.SPI_CRCPolynomial = 0;
	SPI_Init(SPI2, &spiStructure);
	delay_ms(1);
	SPI_Cmd(SPI2, ENABLE);
	delay_ms(1);
	
	//Valori di partenza
	
	LCD_CS_SET;			//lcd deselezionato
	LCD_RST_SET;		//reset alto
	LCD_DC_SET;			//lcd modalita' comandi
	TOUCH_CS_SET;	//deseleziona lcd
	LAN_CS_SET;	//deseleziona lcd
	GPIO_SetBits(GPIOA, TOUCH_CS_PIN);
	GPIO_SetBits(GPIOB, TOUCH_CS_PIN);
}

void LCD_reset()
{
    LCD_RST_RESET;
    delay_ms(100);
    LCD_RST_SET;
    delay_ms(200);
}

void LCD_exitStandby() {
    //dmaSendCmd(LCD_SLEEP_OUT);
    delay_ms(150);
    dmaSendCmd(LCD_DISPLAY_ON);
}

static void LCD_configure(void)
{
	u8 count;
	u8 *address;

	address = (u8 *) init_commands_9488;

	Lcd_CS_Reset();
	while (1)
	{
		count = *(address++);
		if (count-- == 0) break;
		dmaSendCmdCont(*(address++));
		dmaSendDataCont8(address, count);
		address += count;
	}
	LCD_CS_SET;
	delay_ms(1);
	//LCD_setOrientation(0);
	delay_ms(1);
}

void LCD_init()
{
	LCD_pinsInit();
	delay_ms(1);
	Spi2DmaInit();
	delay_ms(1);
	LCD_reset();
	LCD_configure();
	LCD_exitStandby();
	
	delay_ms(50);
	LCD_LED_SET;
	delay_ms(1);

}

void LAN_init()
{
    //LanDmaInit();

    //LAN_reset();
    //LAN_exitStandby();
    //LAN_configure();
}

void SPI2_SetSpeed(u16 speed)
{
	SPI2->CR1 &= ~(1<<6);	//disable spi
	SPI2->CR1 &= ~(7<<3);
	
	if (speed==SPI_LOSPEED)
	{
		spi2_cr2_val=SPI2->CR1;
		SPI2->CR2=0;				//disable interrupt,dma
		SPI2->CR1 &= ~(SPI_DataSize_16b);
	}
	else
		SPI2->CR2=spi2_cr2_val;	//restore interrupt,dma
	
	SPI2->CR1 |=  speed;			//set speed
	SPI2->CR1 |=  1<<6;			//enable spi
}

//</editor-fold>

//<editor-fold desc="LCD common operations">

void LCD_setOrientation(u8 orientation)
{
	if (orientation == ORIENTATION_LANDSCAPE || orientation == ORIENTATION_LANDSCAPE_MIRROR)
	{
		screen_height = LCD_PIXEL_WIDTH_9488;
		screen_width  = LCD_PIXEL_HEIGHT_9488;
	}
	else
	{
		screen_height = LCD_PIXEL_HEIGHT_9488;
		screen_width  = LCD_PIXEL_WIDTH_9488;
    }

    Lcd_CS_Reset();
    dmaSendCmdCont(LCD_MAC);
    dmaSendDataCont8(&orientation, 1);
    LCD_CS_SET;
}

void LCD_setAddressWindow(u16 x1, u16 y1, u16 x2, u16 y2) {
    u16 pointData[2];

    Lcd_CS_Reset();
    dmaSendCmdCont(LCD_COLUMN_ADDR);
    pointData[0] = x1;
    pointData[1] = x2;
    LCD_setSpi16();
    dmaSendDataCont16(pointData, 2);
    LCD_setSpi8();

    dmaSendCmdCont(LCD_PAGE_ADDR);
    pointData[0] = y1;
    pointData[1] = y2;
    LCD_setSpi16();
    dmaSendDataCont16(pointData, 2);
    LCD_setSpi8();
    LCD_CS_SET;
}

u16 LCD_getWidth() {
    return screen_width;
}

u16 LCD_getHeight() {
    return screen_height;
}

//</editor-fold>

//<editor-fold desc="SPI functions">

void LCD_setSpi8(void)
{
    SPI2->CR1 &= ~SPI_CR1_SPE; // DISABLE SPI
    SPI2->CR1 &= ~SPI_CR1_DFF; // SPI 8
    SPI2->CR1 |= SPI_CR1_SPE;  // ENABLE SPI
}

void LCD_setSpi16(void)
{
    SPI2->CR1 &= ~SPI_CR1_SPE; // DISABLE SPI
    SPI2->CR1 |= SPI_CR1_DFF;  // SPI 16
    SPI2->CR1 |= SPI_CR1_SPE;  // ENABLE SPI
}

void LCD_drawChar(u16 x0, u16 y0, unsigned char c, u16 color, u16 bg, uint8_t size)
{
	u16 scaledWidth,doubleScaledWidth,doubleSize,count,mx,my,pixelColor,characterNumber;
	s16 x1,y1,kdebug ;
	s8  i, j, sx, sy;
   u8  line;
	
	
	#define MAX_PIXELS	0x1ff
	
	////u16 charPixels[count];
	u16 charPixels[MAX_PIXELS];
	
	scaledWidth       = (u16) (size * 6);
	doubleScaledWidth = scaledWidth * size;

	x1 = (u16) (x0 + scaledWidth - 1);
	y1 = (u16) (y0 + 8 * size - 1);

	doubleSize = size * size;
	count      = (u16) (48 * doubleSize);

	if (x0 >= LCD_getWidth() || y0 >= LCD_getHeight() || x1 < 0 || y1 < 0) return;

    if (!_cp437 && (c >= 176)) c++; // Handle 'classic' charset behavior

    characterNumber = (u16) (c * 5);

    if (bg == TRANSPARENT_COLOR) {
        LCD_readPixels(x0, y0, x1, y1, charPixels);
    }

    LCD_setAddressWindowToWrite(x0, y0, x1, y1);

	for (i = 0; i < 6; i++)
	{
		line = (u8) (i < 5 ? pgm_read_byte(font + characterNumber + i) : 0x0);
		my   = (u16) (i * size);

		for (j = 0; j < 8; j++, line >>= 1)
		{
			mx = (u16) (j * doubleScaledWidth);
			pixelColor = line & 0x1 ? color : bg;
			if (pixelColor == TRANSPARENT_COLOR) continue;
			for (sx = 0; sx < size; ++sx)
			{
				for (sy = 0; sy < size; ++sy)
				{
					kdebug=mx + my + sy * scaledWidth + sx;
					if (kdebug<MAX_PIXELS)
						charPixels[kdebug] = pixelColor;
				}
			}
		}
	}

	y1=0;
	
	for( x1 =0 ; x1<count;x1++)
	{
		dma8Buffer[y1++]=(u8) ((charPixels[x1]&0x1f)<<3);	//blue
		dma8Buffer[y1++]=(u8) ((charPixels[x1]&0x7e0)>>3);	//green
		dma8Buffer[y1++]=(u8) ((charPixels[x1]&0xf800)>>8);	//red
	}
	
	dmaSendData8(dma8Buffer, y1-1);
	
	//LCD_setSpi16();
	//dmaSendData16(charPixels, count);
	LCD_setSpi8();
}

void LCD_write(unsigned char c)
{
    if (c == '\n') {
        cursorY += textSize * 8;
        cursorX = 0;
    } else if (c == '\r') {
        cursorX = 0;
    } else {
        if (wrap && ((cursorX + textSize * 6) >= LCD_getWidth())) { // Heading off edge?
            cursorX = 0;            // Reset x to zero
            cursorY += textSize * 8; // Advance y one line
        }
        LCD_drawChar(cursorX, cursorY, c, textColor, textBgColor, textSize);
        cursorX += textSize * 6;
    }
}

void LCD_writeString(unsigned char *s) {
    while (*(s)) LCD_write(*s++);
}

void LCD_setCursor(u16 x, u16 y) {
    cursorX = x;
    cursorY = y;
}

void LCD_setTextSize(u8 size) {
    textSize = size;
}

void LCD_setTextColor(u16 color) {
    textColor = color;
}

void LCD_setTextBgColor(u16 color) {
    textBgColor = color;
}

u16 LCD_getCursorX() {
    return cursorX;
}

u16 LCD_getCursorY() {
    return cursorY;
}

void TEST_fillPrimitives(u16 step)
{
	u16 halfStep,quartStep,halfQuartStep,w,h,x,y;

	LCD_fillScreen(BLACK);

    halfStep      = (u16) (step / 2);
    quartStep     = (u16) (halfStep / 2);
    halfQuartStep = (u16) (quartStep / 2);

    w = LCD_getWidth();
    h = LCD_getHeight();

    for (x = 0; x < w; x += step) {
        LCD_drawFastVLine(x, 0, h, DGRAY);
    }

    for (y = 0; y < h; y += step) {
        LCD_drawFastHLine(0, y, w, DGRAY);
    }

    for (x = 0; x < w; x += step) {
        for (y = 0; y < h; y += step) {
            LCD_drawRect(x, y, halfStep, halfStep, DGREEN);
            LCD_fillRect(x + halfStep, y + halfStep, halfStep, halfStep, GREENYELLOW);
            LCD_drawCircle(x + quartStep, y + quartStep, halfQuartStep, GREENYELLOW);
            LCD_fillCircle(x + halfStep + quartStep, y + halfStep + quartStep, halfQuartStep, DGREEN);
            LCD_putPixel(x + quartStep, y + quartStep, YELLOW);

            LCD_drawLine(x + halfStep, y + halfStep, x + step, y + step, WHITE);
        }
    }
}

void LCD_readPixels(u16 x1, u16 y1, u16 x2, u16 y2, u16 *buf)
{
	u8  red, green, blue;
	u32 i,count;
	count	= (u32) ((x2 - x1 + 1) * (y2 - y1 + 1));

    LCD_setAddressWindowToRead(x1, y1, x2, y2);

    Lcd_CS_Reset();
    LCD_DC_SET;

    dmaReceiveDataCont8(&red);

    for (i = 0; i < count; ++i) {
        dmaReceiveDataCont8(&red);
        dmaReceiveDataCont8(&green);
        dmaReceiveDataCont8(&blue);

        buf[i] = (u16) ILI9341_COLOR(red, green, blue);
    }
    LCD_CS_SET;
}


//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

void LCD_fillRect(u16 x1, u16 y1, u16 w, u16 h, u16 color)
{
	u8 r,g,b;
	u32 count,i,ts;
	
	if (x1>screen_width)
		x1=screen_width;
		//w=screen_width;
	if (y1>screen_height)
		y1=screen_height;
		//h=screen_width;
	
	if (x1+w>screen_width)
		w=screen_width-x1;
	if (y1+h>screen_height)
		h=screen_width-y1;
	
	count = w * h;
	LCD_setAddressWindowToWrite(x1, y1, (u16) (x1 + w - 1), (u16) (y1 + h - 1));
	
	r=(u8) ((color&0xf800)>>8);	//red
	g=(u8) ((color&0x7e0)>>3);
	b=(u8) ((color&0x1f)<<3);

	count=count*3;

	for( i =0 ; i<( count < DMA8_BUF_SIZE ? count : DMA8_BUF_SIZE) ; )
	{
		dma8Buffer[i++]=b;
		dma8Buffer[i++]=g;
		dma8Buffer[i++]=r;
	}

    dmaSendCmdCont(LCD_GRAM);
	
	LCD_setSpi8();
	while (count != 0)
	{
		ts = (u16) (count > DMA8_BUF_SIZE ? DMA8_BUF_SIZE : count);
		dmaSendData8(dma8Buffer, ts);
		count -= ts;
	}

	//LCD_setSpi16();
	//dmaFill16(color, count);
	//LCD_setSpi8();
}

void LCD_fillScreen(u16 color) {
    LCD_fillRect(0, 0, LCD_getWidth(), LCD_getHeight(), color);
}

void LCD_drawFastHLine(u16 x0, u16 y0, u16 w, u16 color) {
    if (w == 1) {
        LCD_putPixel(x0, y0, color);
        return;
    }
    LCD_fillRect(x0, y0, w, 1, color);
}

void LCD_putPixel(u16 x, u16 y, u16 color) {
    LCD_setAddressWindowToWrite(x, y, x, y);
    LCD_setSpi16();
    dmaFill16(color, 1);
    LCD_setSpi8();
}

void LCD_putPixelCont(u16 x, u16 y, u16 color) {
    LCD_setAddressWindowToWrite(x, y, x, y);
    dmaFill16(color, 1);
}

void LCD_drawFastVLine(u16 x0, u16 y0, u16 h, u16 color)	
{
    if (h == 1) {
        LCD_putPixel(x0, y0, color);
        return;
    }
    LCD_fillRect(x0, y0, 1, h, color);
}

void LCD_drawCircle(u16 x0, u16 y0, u16 r, u16 color)
{
	s16 f,dx,dy,x,y;
	if (r == 0)
	{
		LCD_putPixel(x0, y0, color);
		return;
	}

	f  = (s16) (1 - r);
	dx = 1;
	dy = (s16) (-2 * r);
	x  = 0;

	y = r;

	LCD_setSpi16();

	LCD_putPixelCont(x0, y0 + r, color);
	LCD_putPixelCont(x0, y0 - r, color);
	LCD_putPixelCont(x0 + r, y0, color);
	LCD_putPixelCont(x0 - r, y0, color);

	while (x < y)
	{
      if (f >= 0)
		{
			y--;
			dy += 2;
			f += dy;
		}
		x++;
		dx += 2;
		f += dx;

		LCD_putPixelCont(x0 + x, y0 + y, color);
		LCD_putPixelCont(x0 - x, y0 + y, color);
		LCD_putPixelCont(x0 + x, y0 - y, color);
		LCD_putPixelCont(x0 - x, y0 - y, color);
		LCD_putPixelCont(x0 + y, y0 + x, color);
		LCD_putPixelCont(x0 - y, y0 + x, color);
		LCD_putPixelCont(x0 + y, y0 - x, color);
		LCD_putPixelCont(x0 - y, y0 - x, color);
	}

	LCD_setSpi8();
}

// Used to do circles and roundrects
void LCD_fillCircleHelper(u16 x0, u16 y0, u16 r, u8 cornername, s16 delta, u16 color)
{
	s16 f,dx,dy,x,y;
	
    if (r == 0)
        return;
    if (r == 1)
	 {
        LCD_putPixel(x0, y0, color);
        return;
    }

    f  = (s16) (1 - r);
    dx = 1;
    dy = (s16) (-2 * r);
    x  = 0;
	 y = r;

    while (x < y)
	 {
        if (f >= 0)
		  {
            y--;
            dy=dy+2;
            f += dy;
        }
        x++;
        dx=dx+2;
        f += dx;

        if (cornername & 0x1)
		  {
            LCD_drawFastVLine(x0 + x, y0 - y, (u16) (2 * y + 1 + delta), color);
            LCD_drawFastVLine(x0 + y, y0 - x, (u16) (2 * x + 1 + delta), color);
        }
        if (cornername & 0x2)
		  {
            LCD_drawFastVLine(x0 - x, y0 - y, (u16) (2 * y + 1 + delta), color);
            LCD_drawFastVLine(x0 - y, y0 - x, (u16) (2 * x + 1 + delta), color);
        }
    }
}

void LCD_fillCircle(u16 x0, u16 y0, u16 r, u16 color) {
    LCD_drawFastVLine(x0, y0 - r, (u16) (2 * r + 1), color);
    LCD_fillCircleHelper(x0, y0, r, 3, 0, color);
}

void LCD_drawLine(u16 x0, u16 y0, u16 x1, u16 y1, u16 color)
{
	s16 dx,dy,Dx,Dy,steep,err,yStep;	
	Dx = (s16) abs(x1 - x0);
	Dy = (s16) abs(y1 - y0);

	if (Dx == 0 && Dy == 0)
	{
		LCD_putPixel(x0, y0, color);
		return;
	}

	steep = Dy > Dx;

	if (steep)
	{
		_int16_swap(x0, y0);
		_int16_swap(x1, y1);
	}

	if (x0 > x1)
	{
		_int16_swap(x0, x1);
        _int16_swap(y0, y1);
	}

	dx = x1 - x0;
	dy = (s16) abs(y1 - y0);

	err = (s16) (dx / 2);

	if (y0 < y1)
	{
		yStep = 1;
	}
	else
	{
		yStep = -1;
	}

    for (; x0 <= x1; x0++)
	{
		if (steep)
		{
			LCD_putPixel(y0, x0, color);
		}
		else
		{
			LCD_putPixel(x0, y0, color);
		}
		err -= dy;
		if (err < 0)
		{
			y0 += yStep;
			err += dx;
		}
	}
}

void LCD_drawRect(u16 x, u16 y, u16 w, u16 h, u16 color) {
    if (w == 0 || h == 0) return;
    if (w == 1) {
        LCD_drawFastVLine(x, y, h, color);
        return;
    }
    if (h == 1) {
        LCD_drawFastHLine(x, y, w, color);
        return;
    }
    LCD_drawFastHLine(x, y, w, color);
    LCD_drawFastHLine(x, (u16) (y + h - 1), w, color);
    LCD_drawFastVLine(x, y, h, color);
    LCD_drawFastVLine((u16) (x + w - 1), y, h, color);
}

void LCD_setVerticalScrolling(u16 startY, u16 endY)
{
	u16 d[3];
	
	d[0]=startY;

	d[1]=(u16) (LCD_PIXEL_HEIGHT_9488 - startY - endY);
	d[2]=endY;
	 
	 dmaSendCmd(LCD_VSCRDEF);
    LCD_setSpi16();
    dmaSendData16(d, 3);
    LCD_setSpi8();
}

void LCD_scroll(u16 v) {
    dmaSendCmd(LCD_VSCRSADD);
    LCD_setSpi16();
    dmaSendData16(&v, 1);
    LCD_setSpi8();
}

void Lcd_CS_Reset(void)
{
	u8 i;
	GPIO_ResetBits(GPIOB, LCD_CS_PIN);
	for (i=0;i<10;i++)
	{
		__NOP;
	}
}

//disegna l'orario
void LCD_DrawDate(u32 x,u32 y,u8 size)
{
	u8 buf[16];
	LCD_setTextSize(size);
	LCD_setCursor(x,y);
	LCD_setTextColor(WHITE);
	LCD_setTextBgColor(BLACK);

	s_DateStructVar.Year=(BKP_ReadBackupRegister(BKP_DR4));
	s_DateStructVar.Month=(BKP_ReadBackupRegister(BKP_DR2));
	s_DateStructVar.Day=(BKP_ReadBackupRegister(BKP_DR3));

	if (size <2)
	{
		buf[0]=(s_DateStructVar.Year/1000) + 0x30;
		buf[1]=((s_DateStructVar.Year/100)%10) + 0x30;
		buf[2]=((s_DateStructVar.Year/10)%10) + 0x30;
		buf[3]=(s_DateStructVar.Year%10) + 0x30;
		buf[4]='-';
		buf[5]=0;
	}
	else
	{
		buf[0]=((s_DateStructVar.Year/10)%10) + 0x30;
		buf[1]=(s_DateStructVar.Year%10) + 0x30;
		buf[2]='-';
		buf[3]=0;
	}
	
	LCD_writeString(buf);																		//anno

	LCD_writeString(MonthsNames[s_DateStructVar.Month-1]);		//mese

	buf[0]='-';
	buf[1]=(s_DateStructVar.Day/10) + 0x30;
	buf[2]=(s_DateStructVar.Day%10) + 0x30;
	buf[3]=' ';
	buf[4]=0;
	LCD_writeString(buf);																		//giorno

	buf[0]=s_TimeStructVar.HourHigh + 0x30;
	buf[1]=s_TimeStructVar.HourLow + 0x30;
	buf[2]=':';
	buf[3]=s_TimeStructVar.MinHigh + 0x30;
	buf[4]=s_TimeStructVar.MinLow + 0x30;
	buf[5]=':';
	buf[6]=s_TimeStructVar.SecHigh + 0x30;
	buf[7]=s_TimeStructVar.SecLow + 0x30;
	buf[8]=' ';
	if (size <2)
	{
		buf[9]='U';
		buf[10]='T';
		buf[11]='C';
		buf[13]=' ';
		buf[14]=' ';
		buf[15]=0;
	}
	else
		buf[9]=0;
		
	LCD_writeString(buf);																		//orario
	s_TimeStructVar.SecLowOld=s_TimeStructVar.SecLow;			//schermo aggiornato
}
