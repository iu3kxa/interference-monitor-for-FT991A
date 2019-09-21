#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "stm32f10x_gpio.h"
#include "touch.h"
#include "i2c.h"
#include "artcc.h"
#include "hw_config.h"
#include "lcd_commands.h"


static const u8 TOUCH_CMD_X      = 0xD0;
static const u8 TOUCH_CMD_Y      = 0x90;
static const u8 XPT2046_SMPSIZE  = 20;
#define THRESHOLD 2										//per calibrazione
#define XPT2046_SMP_MAX 4095

//vairaibili
//float scale_x, scale_y,offset_x,offset_y,width,height;
u32	pos_x,pos_y;
struct coord pos;
//per calibrazione 
//Matrix        matrix;
//Coordinate    display;
//Coordinate    ScreenSample[3];
//Coordinate    DisplaySample[3] = { {30, 45}, {450, 45}, {210, 290} };	//per 480x320

//static const char* TAG = "TOUCH_SCREEN";
//CXpt2046 *xpt;
//calibration parameters

int _offset_x, _offset_y;
//int m_width, m_height;
//int m_rotation;


bool touch_init(void)
{
	bool error=0;
	pos.touch_pressed = false;
	pos_x = 0;
	pos_y = 0;
	pos.touch_rotation = 0;
	
	//se contenuto eeprom invalida, inserisce valori di partenza
	if(eeprom_read(EE_TOUCH) != 0x55 || eeprom_read(EE_TOUCH+1) != 0xAA)
	{
		eeprom_write(EE_TOUCH,0x55);
		eeprom_write(EE_TOUCH+1,0xAA);
		
		//min x
		eeprom_write(EE_TOUCH+2,0x64>>8);
		eeprom_write(EE_TOUCH+3,0x64&0xff);
		
		//max x
		eeprom_write(EE_TOUCH+4,0x770>>8);
		eeprom_write(EE_TOUCH+5,0x770&0xff);
		
		//min y
		eeprom_write(EE_TOUCH+6,0xA6>>8);
		eeprom_write(EE_TOUCH+7,0xA6&0xff);
		
		//max y
		eeprom_write(EE_TOUCH+8,0x0740>>8);
		eeprom_write(EE_TOUCH+9,0x0740&0xff);
		error=1;
	}
	
	//min x
	pos.x_min=(eeprom_read(EE_TOUCH+2)<<8)+eeprom_read(EE_TOUCH+3); //min x
	pos.x_max=(eeprom_read(EE_TOUCH+4)<<8)+eeprom_read(EE_TOUCH+5); //max x
	pos.y_min=(eeprom_read(EE_TOUCH+6)<<8)+eeprom_read(EE_TOUCH+7); //min y
	pos.y_max=(eeprom_read(EE_TOUCH+8)<<8)+eeprom_read(EE_TOUCH+9); //max y
	return error;
}


bool touch_isPressed()
{
    return pos.touch_pressed;
}

int touch_getSample(uint8_t pin)
{
	u16 in;
	SPI2_SetSpeed(SPI_LOSPEED);
	
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_BSY));
	SPI_I2S_SendData(SPI2, 0xaa);										//empty tx buffer after dma
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_BSY));
	
	//select touch
	TOUCH_CS_RESET;
	SPI_I2S_SendData(SPI2, pin);
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_BSY));
	in=SPI_I2S_ReceiveData(SPI2)<<8;								//empty rx buffer
	
	SPI_I2S_SendData(SPI2,0);											//dummy for receive
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_BSY));		//wait for completion
	in=SPI_I2S_ReceiveData(SPI2)<<8;									//receive upper value
	
	SPI_I2S_SendData(SPI2,0);
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_BSY));
	in |=SPI_I2S_ReceiveData(SPI2);

	TOUCH_CS_SET;
	SPI2_SetSpeed(SPI_HISPEED);
	return in >> 4;
}

void touch_sample(void)
{
	u32 aveX,aveY,i,j,xx,yy,tx,ty;

	struct coord samples[XPT2046_SMPSIZE];
	struct coord distances[XPT2046_SMPSIZE];
	pos.touch_pressed = true;

	aveX = 0;
	aveY = 0;

	//legge XPT2046_SMPSIZE volte il valore
	for (i = 0; i < XPT2046_SMPSIZE; i++)
	{
	  samples[i].x = touch_getSample(TOUCH_CMD_X);
	  samples[i].y = touch_getSample(TOUCH_CMD_Y);
	  if (samples[i].x == 0 || samples[i].x == 4095 || samples[i].y == 0 || samples[i].y == 4095)
		  pos.touch_pressed = false;	//non premuto
	  aveX += samples[i].x;				//somme letture
	  aveY += samples[i].y;
	}

	aveX /= XPT2046_SMPSIZE;			//e ottiene valore medio
	aveY /= XPT2046_SMPSIZE;

	for (i = 0; i < XPT2046_SMPSIZE; i++)
	{
	  distances[i].x = i;
	  distances[i].y = ((aveX - samples[i].x) * (aveX - samples[i].x)) + ((aveY - samples[i].y) * (aveY - samples[i].y));
	}

	// sort by distance
	for (i = 0; i < XPT2046_SMPSIZE-1; i++)
	{
		for (j = 0; j < XPT2046_SMPSIZE-1; j++)
		{
			if (samples[j].y > samples[j+1].y)
			{
				yy = samples[j+1].y;
				samples[j+1].y = samples[j].y;
				samples[j].y = yy;
				xx = samples[j+1].x;
				samples[j+1].x = samples[j].x;
				samples[j].x = xx;
			}
		}
	}

	tx = 0;
	ty = 0;
	for (i = 0; i < (XPT2046_SMPSIZE >> 1); i++)
	{
	  tx += samples[distances[i].x].x;
	  ty += samples[distances[i].x].y;
	}

	tx = tx / (XPT2046_SMPSIZE >> 1);
	ty = ty / (XPT2046_SMPSIZE >> 1);
	
	switch (pos.touch_rotation)
	{
		case 0:
			pos.x = tx;
			pos.y = ty;
			break;
		case 3:
			pos.x = XPT2046_SMP_MAX - ty;
			pos.y = tx;
			break;
		case 2:
			pos.x = XPT2046_SMP_MAX - tx;
			pos.y = XPT2046_SMP_MAX - ty;
			break;
		case 1:
			pos.x = ty;
			pos.y = XPT2046_SMP_MAX - tx;
		case 4:
			pos.x = ty;
			pos.y = tx;
			break;
		default:
			break;
  }

	if (pos.touch_pressed)
	{
		//memorizza lo spot dello schermo selezionato in base alla suddivisione
		i=(9 * (pos.y>pos.y_min ? pos.y-pos.y_min : pos.y_min)) /( pos_y<pos.y_max ? pos.y_max-pos.y_min : pos.y_max);
		pos.spot_X12_Y9=pos.touch_pressed + (i*12) + (12*(pos.x>pos.x_min ? pos.x-pos.x_min : pos.x_min))/(pos.x<pos.x_max ? pos.x_max-pos.x_min : pos.x_max);

		i=(6 * (pos.y>pos.y_min ? pos.y-pos.y_min : pos.y_min)) /( pos_y<pos.y_max ? pos.y_max-pos.y_min : pos.y_max);
		pos.spot_X8_Y6=pos.touch_pressed + (i*8) + (8*(pos.x>pos.x_min ? pos.x-pos.x_min : pos.x_min))/(pos.x<pos.x_max ? pos.x_max-pos.x_min : pos.x_max);

		i=(3 * (pos.y>pos.y_min ? pos.y-pos.y_min : pos.y_min)) /( pos_y<pos.y_max ? pos.y_max-pos.y_min : pos.y_max);
		pos.spot_X4_Y3=pos.touch_pressed + (i*4) + (4*(pos.x>pos.x_min ? pos.x-pos.x_min : pos.x_min))/(pos.x<pos.x_max ? pos.x_max-pos.x_min : pos.x_max);

		i=(2 * (pos.y>pos.y_min ? pos.y-pos.y_min : pos.y_min)) /( pos_y<pos.y_max ? pos.y_max-pos.y_min : pos.y_max);
		pos.spot_X3_Y2=pos.touch_pressed + (i*3) + (3*(pos.x>pos.x_min ? pos.x-pos.x_min : pos.x_min))/(pos.x<pos.x_max ? pos.x_max-pos.x_min : pos.x_max);
	}
	else
	{
		pos.spot_X12_Y9=0;
		pos.spot_X8_Y6=0;
		pos.spot_X4_Y3=0;
		pos.spot_X3_Y2=0; 
	}
}

//Imposta rotazione touch
void touch_setRotation(u32 r)
{
    pos.touch_rotation = r;
}

//calibrazione touch
void touch_calibration(int width, int height, int rotation)
{
	LCD_fillScreen(BLACK);
	LCD_setTextSize(2);
	LCD_setTextColor(WHITE);
	LCD_setTextBgColor(BLACK);
	LCD_setCursor(50,0);

	LCD_writeString((u8 *) "Premi i cerchi per calibrare");
	delay_ms(1000);
	LCD_drawCircle(20,20,20,WHITE);

	//punto in alto a sinistra
	LCD_setCursor(45,145);
	LCD_writeString((u8 *) "Superiore sinistro...   ");
	pos.touch_pressed=0;
	while(!pos.touch_pressed)
		touch_sample();
	pos.x_min=pos.x;
	pos.y_min=pos.y;
	LCD_drawCircle(20,20,20,BLACK);	//cancella cerchio
	while(pos.touch_pressed)
		touch_sample();
	
	//punto in alto a destra
	LCD_drawCircle(460,20,20,WHITE);
	LCD_setCursor(45,145);
	LCD_writeString((u8 *) "Superiore destro...   ");

   while(!pos.touch_pressed)
			touch_sample();
	
	pos.x_max=pos.x;
	pos.y_min=(pos.y_min+pos.y)>>1;	//media delle due letture
	LCD_drawCircle(460,20,20,BLACK);
	while(pos.touch_pressed)
		touch_sample();
	
	LCD_drawCircle(460,300,20,WHITE);
	LCD_setCursor(45,145);
	LCD_writeString((u8 *) "Inferiore destro...   ");

	while(!pos.touch_pressed)
		touch_sample();
	
	pos.x_max=(pos.x_max+pos.x)>>1;
	pos.y_max=pos.y;
	LCD_drawCircle(460,300,20,BLACK);
	while(pos.touch_pressed)
		touch_sample();
	
	LCD_drawCircle(20,300,20,WHITE);
	LCD_setCursor(45,145);
	LCD_writeString((u8 *) "Inferiore sinistro...   ");
	while(!pos.touch_pressed)
		touch_sample();
	
	pos.x_min=(pos.x_min+pos.x)>>1;
	pos.y_max=(pos.y_max+pos.y)>>1;
		
	//salva min x su eeprom
	eeprom_write(EE_TOUCH+2,pos.x_min>>8);
	eeprom_write(EE_TOUCH+3,pos.x_min&0xff);
	
	//max x
	eeprom_write(EE_TOUCH+4,pos.x_max>>8);
	eeprom_write(EE_TOUCH+5,pos.x_max&0xff);
	
	//min y
	eeprom_write(EE_TOUCH+6,pos.y_min>>8);
	eeprom_write(EE_TOUCH+7,pos.y_min&0xff);
	
	//max y
	eeprom_write(EE_TOUCH+8,pos.y_max>>8);
	eeprom_write(EE_TOUCH+9,pos.y_max&0xff);

	LCD_fillScreen(BLACK);
}
