
#define SYSTICK_TIMER_FRACTION	10000
#define TMR_INTERVAL_SERIAL_POLL	20					//2ms
#define TMR_INTERVAL_RTX1	10000				//1s
#define TMR_INTERVAL_ETH	500
#define TMR_INTERVAL_TOUCH	500				//50millisecondi

enum _menu_evt {
	MAIN_OPEN,MAIN_RUN,
	MENU_OPEN,MENU_RUN,
	CLOCK_OPEN,CLOCK_RUN,
	SERIAL_OPEN,SERIAL_RUN,
	BT_SERIAL_OPEN,BT_SERIAL_RUN,
	NET_OPEN,NET_RUN,
	NET_SEND_OPEN,NET_SEND_RUN,
	REFGEN_OPEN,REFGEN_RUN,
	GPIO_OPEN,GPIO_RUN,
	TOUCH_CALIBRATION_OPEN,TOUCH_CALIBRATION_RUN
};
 
#include <math.h>
#include <string.h>
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h" // timer
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"
#include "misc.h" // interrupts
#include "main.h"
#include "i2c.h"
#include "rtc.h"
#include "enav.h"
#include "usart.h"
#include "eth_w5500.h"
#include "eth_socket.h"
#include "enav.h"
#include "lcd_core.h"
#include "stm32f10x_i2c.h"
#include "touch.h"
#include "usart.h"

volatile u32 systick_counter;
u16 riga;
extern struct Time_s s_TimeStructVar;
extern struct coord pos;
extern unsigned char i2cbuf[I2C_BUFSIZE];
extern unsigned char buf[16];
extern struct Enav_Frequenze enav[];
extern struct Enav_Status enav_status;
extern struct _usart_data ser[3];	//variabili usart
enum _menu_evt menu_evt;

#define M_PI 3.14159

void SysTick_Handler (void);
void InitializeLED(void);
void InitializeTimer(void);
void EnableTimerInterrupt(void);
void timeToDigits(void);
void menu_open(void);
void menu_run(void);


int hours = 11, minutes = 59, seconds = 56, mseconds = 0;

int main(void)
{
	u32 curTicks=0;
	u32 last_tmr_interval_eth=0;
	u32 last_tmr_interval_touch=0;
	u32 last_tmr_interval_serial_poll=0;
	u32 last_tmr_interval_rtx1=0;
	u8 last_tmr_interval_rtx1_accel=0;	//accelerazione della lettura quando ci sono variazioni
	u32 error;
	u16 riga = 0;
	menu_evt=MAIN_OPEN;
	//unsigned char lb_hello[16] = "hello\n";

	//u8 destip[4]={1,1,1,254};
	//InitializeLED();
	//InitializeTimer();
	//EnableTimerInterrupt();
	
	systickInit(10000);	//0.1ms
	
	
	//lcd
	LCD_init();
	LCD_setOrientation(ORIENTATION_LANDSCAPE); //ORIENTATION_PORTRAIT_MIRROR ORIENTATION_PORTRAIT
	LCD_fillScreen(BLACK);
	
	riga=0;
	LCD_setTextSize(1);
	LCD_setTextColor(WHITE);
	LCD_setTextBgColor(BLACK);
	LCD_setCursor(0,riga+=20);
	LCD_writeString((u8 *) "Inizializzazione:");
	LCD_setCursor(0,riga+=20);
	LCD_writeString((u8 *) "Verifica EEprom!");
	
	//eeprom
	I2C1_init();	//collega eeprom
	
	eeprom_check();
	
	//frequenze
	error=enav_init();
	LCD_setCursor(0,riga+=10);
	itoa(error, buf, 10);
	LCD_writeString(buf);
	LCD_writeString((u8 *) "Leggo frequenze Enav dalla eeprom!");

	LCD_setCursor(0,riga+=10);

	//touch
	if(touch_init())
		LCD_writeString((u8 *) "Touch screen non calibrato, uso defaults!");
	else
		LCD_writeString((u8 *) "Leggo calibrazione del touch screen dalla eeprom! Ok.");
		;
	pos.touch_rotation=4;
	//touch_setRotation(4);
	
	LCD_setCursor(0,riga+=10);
	LCD_writeString((u8 *) "Ethernet");
	
	//interrupt ethernet
	W5500_Init();
	Net_Conf();
		
	Display_Net_Conf();
	socket(0,Sn_MR_UDP,7777,SF_IO_NONBLOCK);
	
	//orologio
	LCD_setCursor(0,riga+=10);
	error=RTC_init();
	if (error)
		LCD_writeString((u8 *) "Orologio invalido, setta il calendario");
	else
		LCD_writeString((u8 *) "Leggo calendario");
		
	//usart
	LCD_setCursor(0,riga+=10);
	LCD_writeString((u8 *) "Inizializzo porte seriali");
	usartInit();
	
	
	LCD_setTextSize(2);
	LCD_setCursor(10,150);
	LCD_setTextColor(BLACK);
	LCD_setTextBgColor(RED);
	LCD_writeString((u8 *) "Premere per calibrare il touch screen.");

	LCD_setTextSize(2);
	LCD_setTextColor(WHITE);
	LCD_setTextBgColor(BLACK);
	
	pos.touch_pressed=0;
	
	
	
	for (error=0;error < 30 && pos.touch_pressed==0;error++)
	{
		//aggiorna orario sullo schermo
		if(s_TimeStructVar.SecLow != s_TimeStructVar.SecLowOld)
			LCD_DrawDate(0,310,1);		
				
		touch_sample();			//legge stato touchscreen
		delay_ms(30);
	}
	
	if(pos.touch_pressed==1)
		touch_calibration(480,320,pos.touch_rotation);
	
	LCD_setTextBgColor(BLACK);
	LCD_fillScreen(BLACK);
	
	
	while(1)
	{
		curTicks = systick_counter;
		
		//legge touchscren ogni ## millisecondi
		if (curTicks - last_tmr_interval_touch > TMR_INTERVAL_TOUCH)
		{
			touch_sample();			//legge stato touchscreen
			last_tmr_interval_eth=curTicks;
		}
		
		
		//mostra la schermata selezionata	
		switch(menu_evt)
		{
			//menu' principale con stato globale
			case MAIN_OPEN:
				enav_main_open();
				menu_evt=MAIN_RUN;
			case MAIN_RUN:
				enav_main_run();
				break;
			
			//scelta altri menu'
			case MENU_OPEN:
				menu_open();
				menu_evt=MENU_RUN;
			case MENU_RUN:
				menu_run();
				break;
			
			//regolazione orologio
			case CLOCK_OPEN:
				menu_evt=CLOCK_RUN;
			case CLOCK_RUN:
				rtc_adjust();
				menu_back();
				break;
			
			//impostazioni seriali
			case SERIAL_OPEN:
				menu_evt=SERIAL_RUN;
			case SERIAL_RUN:
				menu_back();
				break;
			
			//impostazioni bluetooth
			case BT_SERIAL_OPEN:
				menu_evt=BT_SERIAL_RUN;
			case BT_SERIAL_RUN:
				menu_back();
				break;
			
			//impostazioni rete
			case NET_OPEN:
				menu_evt=NET_RUN;
			case NET_RUN:
				menu_back();
				break;

			//client udp
			case NET_SEND_OPEN:
				menu_evt=NET_SEND_RUN;
			case NET_SEND_RUN:
				menu_back();
				break;
			
			//generatore di segnale
			case REFGEN_OPEN:
				menu_evt=REFGEN_RUN;
			case REFGEN_RUN:
				menu_back();
				break;
			
			// i/o
			case GPIO_OPEN:
				menu_evt=GPIO_RUN;
			case GPIO_RUN:
				menu_back();
				break;
			
			case TOUCH_CALIBRATION_OPEN:
				touch_calibration(480,320,pos.touch_rotation);
				menu_evt=TOUCH_CALIBRATION_RUN;
			case TOUCH_CALIBRATION_RUN:
				menu_back();
				break;

			default:
				break;
		}
		
		//informazioni di debug vicino all'ora
		if(enav_status.debug_len)
		{
			LCD_setTextSize(1);
			LCD_setCursor(180,310);
			LCD_setTextColor(WHITE);
			LCD_setTextBgColor(BLACK);
			//scrive n chars
			LCD_writeString((u8 *) "dbg:");
			for(error=0;error<enav_status.debug_len;error++)
				LCD_write(enav_status.dabugdata[error]);
			LCD_writeString((u8 *) "        ");
			
			enav_status.debug_len=0;
		}
		
		if(pos.touch_pressed && pos.spot_X8_Y6 == 48 )	//se premuto spot 48 di 48
			menu_evt=MENU_OPEN;

		
		//aggiorna orario sullo schermo
		if(s_TimeStructVar.SecLow != s_TimeStructVar.SecLowOld)
		{
			LCD_DrawDate(0,310,1);		
			log_udp();
		}
		
			
		if (curTicks - last_tmr_interval_eth > TMR_INTERVAL_ETH)
		{
			//enav_status.changed=1;
			last_tmr_interval_eth=curTicks;
		}
		
		
		if (curTicks - last_tmr_interval_rtx1 > (TMR_INTERVAL_RTX1 >> (last_tmr_interval_rtx1_accel>>2)) )
		{
			if (last_tmr_interval_rtx1_accel)
				last_tmr_interval_rtx1_accel=last_tmr_interval_rtx1_accel>>1;
			
			enav_status.poweron=0;
			usartx_dmaSend8(1,(u8 *) "FA;", 3);
			//enav_status.changed=1;
			last_tmr_interval_rtx1=curTicks;
		}

		//seriali 
		if (curTicks - last_tmr_interval_serial_poll > TMR_INTERVAL_SERIAL_POLL)
		{
			
			//evento dal pc
			if(ser[SERIAL1].evt)
			{
				enav_status.changed=1;
				ser[SERIAL2].evt=0;
			}
			
			//evento dall'rtx			
			if(ser[SERIAL2].evt)
			{
				enav_status.changed=1;
				last_tmr_interval_rtx1_accel=0xff;	//accelera lettura dallrtx finche' lo stato cambia
				ser[SERIAL2].evt=0;
			}
			
			//evento dal nextion	
			if(ser[SERIAL3].evt)
			{
				enav_status.changed=1;
				ser[SERIAL3].evt=0;
			}		
			last_tmr_interval_serial_poll=curTicks;
		}
			//LCD_fillRect(0, 30, pos.y>>2, 24, PURPLE);					//draw y bar
			//LCD_fillRect(pos.y>>2, 30, 480-(pos.y>>2), 24, BLACK);	//delete excess
			//for(i = 5; i<480; i+=30)
			//	LCD_drawLine( i, 5, i, 315, LGRAY);
			//for(i = 5; i<315; i+=30)
			//	LCD_drawLine( 5, i, 480, i, LGRAY);
			//for(i = 0; i < 480; i++) // count point
			//{
				//LCD_fillCircle(i, sin(i), 2, GREEN);
				//LCD_fillCircle(i, (100*i/180)+120, 2, RED);
				//LCD_fillCircle(i, 80*sin(30*0.2*M_PI*i/180)+120, 2, BLUE);
			//}
  }
}



//torna a menu' iniziale
void menu_back(void)
{
	menu_evt=MAIN_OPEN;
}

//disegna pannello menu'
void menu_open (void)
{
	LCD_fillScreen(BLACK);
	LCD_setTextSize(2);
	LCD_setTextColor(WHITE);
	LCD_setTextBgColor(NAVY);
	
	//riga1
	LCD_fillRect(5, 25, 110, 60, NAVY);
	LCD_setCursor(17,48);
	LCD_writeString((u8*) "NETWORK");
	
	LCD_fillRect(125, 25, 110, 60, NAVY);
	LCD_setCursor(162,38);
	LCD_writeString((u8*) "UDP");
	LCD_setCursor(145,58);
	LCD_writeString((u8*) "CLIENT");
	
	LCD_fillRect(245, 25, 110, 60, NAVY);
	LCD_setCursor(260,48);
	LCD_writeString((u8*) "SERIALI");
	
	LCD_fillRect(365, 25, 110, 60, NAVY);
	LCD_setCursor(377,48);
	LCD_writeString((u8*) "BT SER.");
	
	//riga2
	LCD_fillRect(5, 130, 110, 60, NAVY);
	LCD_setCursor(42,142);
	LCD_writeString((u8*) "REF");
	LCD_setCursor(5,162);
	LCD_writeString((u8*) "GENERATOR");
	
	LCD_fillRect(125, 130, 110, 60, NAVY);
	LCD_setCursor(140,152);
	LCD_writeString((u8*) "IN/OUT");
	
	//LCD_fillRect(245, 110, 110, 60, NAVY);
	
	LCD_fillRect(365, 130, 110, 60, NAVY);
	LCD_setCursor(372,152);
	LCD_writeString((u8*) "OROLOGIO");
	
	//riga3
	LCD_fillRect(5, 235, 110, 60, NAVY);
	LCD_setCursor(28,247);
	LCD_writeString((u8*) "TOUCH");
	LCD_setCursor(5,267);
	LCD_writeString((u8*) "CALIBRATE");
	
	//LCD_fillRect(125, 235, 110, 60, NAVY);
	//LCD_fillRect(245, 235, 110, 60, NAVY);
	LCD_fillRect(365, 235, 110, 60, RED);
	LCD_setCursor(395,257);
	LCD_setTextBgColor(RED);
	LCD_writeString((u8*) "EXIT");
	
}

//selezione pulsanti menu'
void menu_run (void)
{
	touch_sample();
	if(pos.touch_pressed)
	{
		switch(pos.spot_X4_Y3)
		{
			case 1:
				menu_evt=NET_OPEN;
				break;
			case 2:
				menu_evt=NET_SEND_OPEN;
				break;
			case 3:
				menu_evt=SERIAL_OPEN;
				break;
			case 4:
				menu_evt=BT_SERIAL_OPEN;
				break;
		
			case 5:
				menu_evt=REFGEN_OPEN;
				break;
			case 6:
				menu_evt=GPIO_OPEN;
				break;
			case 8:
				menu_evt=CLOCK_OPEN;
				break;
			case 9:
				menu_evt=TOUCH_CALIBRATION_OPEN;
				break;
			case 12:
				menu_back();
				break;
			default:
				break;
		}
		while(pos.touch_pressed)
			touch_sample();
	}
}


/*------------------------------------------------------------------------------
  Systick interrupt 1ms
 *------------------------------------------------------------------------------*/
void SysTick_Handler (void)
{
	systick_counter++;
}

void systickInit (u16 frequency)
{
   RCC_ClocksTypeDef RCC_Clocks;
   RCC_GetClocksFreq (&RCC_Clocks);
   (void) SysTick_Config (RCC_Clocks.HCLK_Frequency / frequency);
}


/*------------------------------------------------------------------------------
  Delay in milliseconds
 *------------------------------------------------------------------------------*/
void delay_ms (u32 ms)
{
	u32 curTicks;
	ms=ms * 10;	 //so 1 = 1 milliseconds
	curTicks = systick_counter;
	while ((systick_counter - curTicks) < ms);
}



/**  * @brief This function handles Hard fault interrupt.  */
void HardFault_Handler(void)
{

  while (1)
  {
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

void MemManage_Handler(void)
{
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
  }
}

//debug sullo schermo
void lcd_Debug(u8 *data,u8 len)
{
	enav_status.debug_len=len;
	memcpy(enav_status.dabugdata,data,len);
}

//converte i in stringa verso buf con radice di radix
unsigned long itoa(int value, unsigned char *sp, int radix)
{
    unsigned long len;
    char          tmp[16], *tp = tmp;
    int           i, v, sign   = radix == 10 && value < 0;

    v = sign ? -value : value;

    while (v || tp == tmp) {
        i = v % radix;
        v /= radix;
        *tp++ = i < 10 ? (char) (i + '0') : (char) (i + 'a' - 10);
    }


    len = tp - tmp;

    if (sign) {
        *sp++ = '-';
        len++;
    }

    while (tp > tmp)
        *sp++ = *--tp;

    *sp++ = '\0';
	 *sp++ =0;

    return len;
}
