#ifndef TEST1_CONFIG_H
#define TEST1_CONFIG_H

//pins
#define RESET_PIN   		GPIO_Pin_5  //PA5
#define LCD_DC_PIN      GPIO_Pin_8	//pa8
#define LCD_LED_PIN     GPIO_Pin_7  //PA7
#define TOUCH_CS_PIN    GPIO_Pin_6	//PA6
#define LAN_CS_PIN		GPIO_Pin_12	//PB12

#define LCD_CS_PIN      GPIO_Pin_5	//PB5
#define SPI2_CLK_PIN		GPIO_Pin_13	//PB13
#define SPI2_MOSI_PIN	GPIO_Pin_15	//PB15
#define SPI2_MISO_PIN	GPIO_Pin_14	//PB14


//macro
#define LCD_DC_SET		GPIO_SetBits(GPIOA, LCD_DC_PIN);
#define LCD_DC_RESET		GPIO_ResetBits(GPIOA, LCD_DC_PIN);
#define LCD_RST_SET		GPIO_SetBits(GPIOA, RESET_PIN);
#define LCD_RST_RESET	GPIO_ResetBits(GPIOA, RESET_PIN);
#define LCD_CS_SET		GPIO_SetBits(GPIOB, LCD_CS_PIN);
//#define LCD_CS_RESET  GPIO_ResetBits(GPIOB, LCD_CS_PIN)
#define LCD_LED_SET		GPIO_SetBits(GPIOA, LCD_LED_PIN);
#define LCD_LED_RESET	GPIO_ResetBits(GPIOA, LCD_LED_PIN);
#define TOUCH_CS_SET		GPIO_SetBits(GPIOA, TOUCH_CS_PIN);
#define TOUCH_CS_RESET	GPIO_ResetBits(GPIOA, TOUCH_CS_PIN);
#define LAN_CS_SET		GPIO_SetBits(GPIOB, LAN_CS_PIN);
#define LAN_CS_RESET		GPIO_ResetBits(GPIOB, LAN_CS_PIN);

#define SPI_HISPEED		SPI_BaudRatePrescaler_2		//18mhz (36 with ahpb1 at 72)
#define SPI_LOSPEED	 	SPI_BaudRatePrescaler_64	//0.3mhz

#define BLACK24				0x000000
#define NAVY24            0x000080	/*   0,   0, 128 */
#define DGREEN24        	0x008000  	/*   0, 128,   0 */
#define DCYAN24          0x008080      /*   0, 128, 128 */
#define MAROON24       	0x800000      /* 128,   0,   0 */
#define PURPLE24         0x800080      /* 128,   0, 128 */
#define OLIVE24          	0x808000      /* 128, 128,   0 */
#define LGRAY24				0xc0c0c0      /* 192, 192, 192 */
#define DGRAY24          0x808080      /* 128, 128, 128 */
#define BLUE24           	0x0000ff      /*   0,   0, 255 */
#define GREEN24         	0x00ff00      /*   0, 255,   0 */
#define CYAN24           	0x00ffff      /*   0, 255, 255 */
#define RED24             	0xff0000      /* 255,   0,   0 */
#define MAGENTA24     	0xff00ff      /* 255,   0, 255 */
#define YELLOW24      	0xffff00      /* 255, 255,   0 */
#define WHITE24           0xffffff      /* 255, 255, 255 */
#define ORANGE24         0xffa500   /* 255, 165,   0 */
#define GREENYELLOW24 0xadff30    0xAFE5      /* 173, 255,  47 */
//#define BROWN24                 0XBC40 //
//#define BRRED24                 0XFC07 //


#define BLACK           0x0000      /*   0,   0,   0 */
#define NAVY            0x000F      /*   0,   0, 128 */
#define DGREEN          0x03E0      /*   0, 128,   0 */
#define DCYAN           0x03EF      /*   0, 128, 128 */
#define MAROON          0x7800      /* 128,   0,   0 */
#define PURPLE          0x780F      /* 128,   0, 128 */
#define OLIVE           0x7BE0      /* 128, 128,   0 */
#define LGRAY           0xC618      /* 192, 192, 192 */
#define DGRAY           0x7BEF      /* 128, 128, 128 */
#define BLUE            0x001F      /*   0,   0, 255 */
#define GREEN           0x07E0      /*   0, 255,   0 */
#define CYAN            0x07FF      /*   0, 255, 255 */
#define RED             0xF800      /* 255,   0,   0 */
#define MAGENTA         0xF81F      /* 255,   0, 255 */
#define YELLOW          0xFFE0      /* 255, 255,   0 */
#define WHITE           0xFFFF      /* 255, 255, 255 */
#define ORANGE          0xFD20      /* 255, 165,   0 */
#define GREENYELLOW     0xAFE5      /* 173, 255,  47 */
#define BROWN                 0XBC40 //
#define BRRED                 0XFC07 //

#include "stm32f10x_gpio.h"

// </editor-fold>

#endif //TEST1_CONFIG_H
