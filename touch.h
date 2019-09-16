#ifndef _XPT2046_H
#define _XPT2046_H

#include <stdbool.h>
#include "hw_config.h"
#include "stm32f10x_dma.h"
#include "lcd_core.h"

//touch
struct coord
{
	u32 x;		//posizione raw attuale filtrata sulla media 20 letture
	u32 y;
	u32 x_min;	//posizione leggibile minima
	u32 x_max;	//posizione leggibile massima
	u32 y_min;
	u32 y_max;
	u32 x_old;	//lettura raw precedente
	u32 y_old;
	u32 touch_rotation;	//orientamento touch 0,1,2,3,4
	u8 spot_X12_Y9;		//spot schermo selezionato x=0-11 y=0-8 totale 108 spot
	u8 spot_X8_Y6;			//spot schermo selezionato x=0-7 y=0-5 totale 48 spot
	u8 spot_X4_Y3;			//spot schermo selezionato x=0-3 y=0-2 totale 12 spot
	u8 spot_X3_Y2;			//spot schermo selezionato x=0-2 y=0-1 totale 6 spot
	//u32 _offset_x;
	//u32 _offset_y;
	u8 touch_cs;				//DSPI *_spi;
	bool touch_pressed;
	bool touch_pressed_old;
	//float xFactor;
	//float yFactor;
};

//calibrazione
/* Private typedef -----------------------------------------------------------*/
typedef	struct POINT 
{
   uint16_t x;
   uint16_t y;
}Coordinate;


typedef struct Matrix 
{						
long double An,  
            Bn,     
            Cn,   
            Dn,    
            En,    
            Fn,     
            Divider ;
} Matrix ;

extern Coordinate ScreenSample[3];
extern Coordinate DisplaySample[3];
extern Matrix matrix ;
extern Coordinate  display ;


//prototipi
void touch_sample(void);
u32 touch_x(void);
u32 touch_y(void);
bool touch_isPressed(void);

bool touch_init(void);
void touch_setRotation(u32 r);
int touch_getSample(u8 pin);

//per calibrazione
void touch_calibration(int width, int height, int rotation);
//void TouchPanel_Calibrate(void);
//FunctionalState setCalibrationMatrix(Coordinate * displayPtr,Coordinate * screenPtr,Matrix * matrixPtr);
//FunctionalState getDisplayPoint(Coordinate * displayPtr,Coordinate * screenPtr,Matrix * matrixPtr );

#endif
