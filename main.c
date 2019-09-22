
#define SYSTICK_TIMER_FRACTION	10000
#define TMR_INTERVAL_SERIAL_POLL	20			//2ms
#define TMR_INTERVAL_RTX1	1000						//100ms
#define TMR_INTERVAL_ETH	500
#define TMR_INTERVAL_TOUCH	500						//50millisecondi
#define TMR_INTERVAL_LED	10							//1millisecondi

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
	TOUCH_CALIBRATION_OPEN,TOUCH_CALIBRATION_RUN,
	ERASE_NVRAM_OPEN,ERASE_NVRAM_RUN
};
 
#include <math.h>
#include <string.h>
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h" // timer
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_i2c.h"
#include "misc.h" // interrupts
#include "main.h"
#include "i2c.h"
#include "rtc.h"
#include "rtx.h"
#include "artcc.h"
#include "usart.h"
#include "eth_w5500.h"
#include "eth_socket.h"
#include "lcd_core.h"

#include "touch.h"


volatile u32 systick_counter;
u16 riga;
bool led;
extern struct Time_s s_TimeStructVar;
extern struct coord pos;
extern unsigned char i2cbuf[I2C_BUFSIZE];
extern unsigned char buf[16];
extern struct _artcc_frequencies artcc[];
extern struct _rtx_status rtx;
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

int main(void)
{
	u32 curTicks=0;
	u32 last_tmr_interval_eth=0;			//variabili scheduler
	u32 last_tmr_interval_touch=0;
	u32 last_tmr_interval_serial_poll=0;
	u32 tmr_interval_rtx1=TMR_INTERVAL_RTX1;
	u32 last_tmr_interval_rtx1=0;
	u32 last_tmr_interval_led=0;
	u32 error;
	u16 riga = 0;								//riga di testo corrente
	GPIO_InitTypeDef gpioStructure;
	menu_evt=MAIN_OPEN;
	
	systickInit(10000);			//0.1ms, 10khz
	delay_ms(100);					//attende l'accensione delle periferiche
	
	//led
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);	//enable clock gpiob
	gpioStructure.GPIO_Pin  = GPIO_Pin_13;
	gpioStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	gpioStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOC, &gpioStructure);
	
	
	//lcd
	LCD_init();
	LCD_setOrientation(ORIENTATION_LANDSCAPE);	//orientamento orizzontale conenttori a sinistra.
	LCD_fillScreen(BLACK);
	
	riga=0;							//messaggi inizializzazione su schermo
	LCD_print(0,0,(u8 *) "Inizializzazione:",WHITE,BLACK,1);
	LCD_setCursor(0,riga+=20);
	LCD_writeString((u8 *) "Inizializzazione:");
	LCD_setCursor(0,riga+=20);
	LCD_writeString((u8 *) "Verifica EEprom!");

	I2C1_init();					//inizializza bus i2c
	eeprom_check();				//controlla e inizializza eeprom 24lc64
	
	//frequenze
	error=artcc_init();
	LCD_setCursor(0,riga+=10);
	itoa(error, buf, 10);
	LCD_writeString(buf);
	LCD_writeString((u8 *) "Leggo frequenze Enav dalla eeprom!");
	LCD_setCursor(0,riga+=10);

	//touch
	if(touch_init())
		LCD_writeString((u8 *) "Touch screen non calibrato, uso default!");
	else
		LCD_writeString((u8 *) "Leggo calibrazione del touch screen dalla eeprom! Ok.");
	pos.touch_rotation=4;		//Orientamento lettura touch
	
	LCD_setCursor(0,riga+=10);
	LCD_writeString((u8 *) "Ethernet");
	W5500_Init();
	Net_Conf();						//imposta rete default
		
	Display_Net_Conf();			//mostra impostazioni attuali
	socket(0,Sn_MR_UDP,7777,SF_IO_NONBLOCK);
	
	LCD_setCursor(0,riga+=10);
	error=RTC_init();				//Inizializza accesso orologio/calendario
	if (error)
		LCD_writeString((u8 *) "Orologio invalido, entra nel menu' e setta il calendario");
	else
		LCD_writeString((u8 *) "Leggo calendario");
		
	//usart
	LCD_setCursor(0,riga+=10);
	LCD_writeString((u8 *) "Inizializzo porte seriali");
	usartInit();
	
	LCD_print(10,150,(u8 *) "Premere per calibrare il touch screen.",BLACK,RED,2);
	pos.touch_pressed=0;
	for (error=0;error < 60 && pos.touch_pressed==0;error++)
	{
		//aggiorna orario sullo schermo
		if(s_TimeStructVar.SecLow != s_TimeStructVar.SecLowOld)
			LCD_DrawDate(0,310,1);		
				
		touch_sample();			//legge stato touchscreen
		delay_ms(30);
	}
	
	if(pos.touch_pressed==1)	//se viene premuto il touch entra in modalita' calibrazione del touch
		touch_calibration(480,320,pos.touch_rotation);
	
	LCD_print(10,150,(u8 *) " ",WHITE,BLACK,2);
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
			case MAIN_OPEN:				//menu' principale con stato globale
				artcc_main_open();
				menu_evt=MAIN_RUN;
			case MAIN_RUN:
				artcc_main_run();
				break;
			
			case MENU_OPEN:				//scelta altri menu'
				menu_open();
				menu_evt=MENU_RUN;
			case MENU_RUN:
				menu_run();
				break;
			
			case CLOCK_OPEN:				//regolazione orologio
				menu_evt=CLOCK_RUN;
			case CLOCK_RUN:
				rtc_adjust();
				menu_back();
				break;
			
			case SERIAL_OPEN:				//impostazioni seriali
				menu_evt=SERIAL_RUN;
			case SERIAL_RUN:
				menu_back();
				break;
			
			case BT_SERIAL_OPEN:			//impostazioni bluetooth
				menu_evt=BT_SERIAL_RUN;
			case BT_SERIAL_RUN:
				menu_back();
				break;
			
			case NET_OPEN:					//impostazioni rete
				menu_evt=NET_RUN;
			case NET_RUN:
				menu_back();
				break;

			case NET_SEND_OPEN:			//client udp
				menu_evt=NET_SEND_RUN;
			case NET_SEND_RUN:
				menu_back();
				break;
				
			case REFGEN_OPEN:				//generatore di segnale si5351
				menu_evt=REFGEN_RUN;
			case REFGEN_RUN:
				menu_back();
				break;
			
			case GPIO_OPEN:				// impostazioni gpio
				menu_evt=GPIO_RUN;
			case GPIO_RUN:
				menu_back();
				break;
			
			case TOUCH_CALIBRATION_OPEN:	//pannello calibrazione touch
				touch_calibration(480,320,pos.touch_rotation);
				menu_evt=TOUCH_CALIBRATION_RUN;
			case TOUCH_CALIBRATION_RUN:
				menu_back();
			
			case ERASE_NVRAM_OPEN:				// impostazioni gpio
				nvram_reset();
				menu_evt=ERASE_NVRAM_RUN;
			case ERASE_NVRAM_RUN:
				menu_back();
			break;
			
			default:
			break;
		}

		if(rtx.pc_debug_len)	//informazioni di debug vicino all'ora
		{
			LCD_print(180,310,(u8 *) "dbg:",WHITE,BLACK,1);	//scrive n chars
			for(error=0;error<rtx.pc_debug_len;error++)
				LCD_write(rtx.pc_debugdata[error]);
			LCD_writeString((u8 *) "    ");
			rtx.pc_debug_len=0;
		}
		
		/*
		if(rtx.rtx_debug_len)	//informazioni di debug vicino all'ora
		{
			LCD_print(240,310,(u8 *) "r:",WHITE,BLACK,1);	//scrive n chars
			for(error=0;error<rtx.rtx_debug_len;error++)
				LCD_write(rtx.rtx_debugdata[error]);
			LCD_writeString((u8 *) "        ");
			rtx.rtx_debug_len=0;
		}
		*/
		
		if(pos.touch_pressed && pos.spot_X8_Y6 == 48 )							//se premuto spot 48 di 48
			menu_evt=MENU_OPEN;

		//aggiorna orario sullo schermo
		if(s_TimeStructVar.SecLow != s_TimeStructVar.SecLowOld)
		{
			if(rtx.txmode)
				rtx.tx_time++;
			
			LCD_DrawDate(0,310,1);		
		}
		
		//poll ethernet
		if (curTicks - last_tmr_interval_eth > TMR_INTERVAL_ETH)
		{
			last_tmr_interval_eth=curTicks;
		}
		
		//led
		if (curTicks - last_tmr_interval_led > TMR_INTERVAL_LED)
		{
			if(led)
				led--;
			else
				GPIO_SetBits(GPIOC, GPIO_Pin_13);
				
			last_tmr_interval_led=curTicks;
		}
		
		
		//poll stato dell'rtx
		if (curTicks - last_tmr_interval_rtx1 > tmr_interval_rtx1)
		{
			
			if(!ser[SERIAL1].evt)		//se non c'e' attivita' sul pc
			{
				rtx_poll();
			}
			else
				ser[SERIAL1].evt--;
				
			if(rtx.changed)					//se c'e' attivita sulle impostazione dell'apparato accelera il polling
			{
				tmr_interval_rtx1=TMR_INTERVAL_RTX1/10;
				rtx.changed--;
			}
			else
				tmr_interval_rtx1=TMR_INTERVAL_RTX1;
	
			last_tmr_interval_rtx1=curTicks;
		}

		//seriali 
		if (curTicks - last_tmr_interval_serial_poll > TMR_INTERVAL_SERIAL_POLL)
		{
			
			//evento dal pc
			if(ser[SERIAL1].evt)
			{
				ser[SERIAL2].evt=0;
			}
			
			//evento dall'rtx			
			if(ser[SERIAL2].evt)
			{
				ser[SERIAL2].evt=0;
			}
			
			//evento dal nextion	
			if(ser[SERIAL3].evt)
			{
				ser[SERIAL3].evt=0;
			}		
			last_tmr_interval_serial_poll=curTicks;
		}
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
	
	//riga1
	LCD_fillRect(5, 25, 110, 60, NAVY);
	LCD_print(17,48,(u8 *) "NETWORK",WHITE,NAVY,2);
	
	LCD_fillRect(125, 25, 110, 60, NAVY);
	LCD_print(162,38,(u8 *) "UDP",WHITE,NAVY,2);
	LCD_print(145,58,(u8 *) "CLIENT",WHITE,NAVY,2);
	
	LCD_fillRect(245, 25, 110, 60, NAVY);
	LCD_print(260,48,(u8 *) "SERIALI",WHITE,NAVY,2);
	
	LCD_fillRect(365, 25, 110, 60, NAVY);
	LCD_print(377,48,(u8 *) "BT SER.",WHITE,NAVY,2);
	
	//riga2
	LCD_fillRect(5, 130, 110, 60, NAVY);
	LCD_print(42,142,(u8 *) "REF",WHITE,NAVY,2);
	LCD_print(5,162,(u8 *) "GENERATOR",WHITE,NAVY,2);
	
	LCD_fillRect(125, 130, 110, 60, NAVY);
	LCD_print(140,152,(u8 *) "IN/OUT",WHITE,NAVY,2);
	//LCD_fillRect(245, 110, 110, 60, NAVY);
	
	LCD_fillRect(365, 130, 110, 60, NAVY);
	LCD_print(372,152,(u8 *) "OROLOGIO",WHITE,NAVY,2);
	
	//riga3
	LCD_fillRect(5, 235, 110, 60, NAVY);
	LCD_print(28,247,(u8 *) "TOUCH",WHITE,NAVY,2);
	LCD_print(5,267,(u8 *) "CALIBRATE",WHITE,NAVY,2);
	
	LCD_fillRect(125, 235, 110, 60, NAVY);
	LCD_print(148,247,(u8 *) "ERASE",WHITE,NAVY,2);
	LCD_print(148,267,(u8 *) "NVRAM",WHITE,NAVY,2);
	
	//LCD_fillRect(245, 235, 110, 60, NAVY);

	LCD_fillRect(365, 235, 110, 60, RED);
	LCD_print(395,257,(u8 *) "EXIT",WHITE,RED,2);
	LCD_setCursor(395,257);
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
			case 10:
				menu_evt=ERASE_NVRAM_OPEN;
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

// This function handles Hard fault interrupt.
void HardFault_Handler(void)
{

  while (1)
  {
    //USER CODE END W1_HardFault_IRQn 0
  }
}

void MemManage_Handler(void)
{
  while (1)
  {
    // USER CODE BEGIN W1_MemoryManagement_IRQn 0
  }
}

//debug sullo schermo
void lcd_pcDebug(u8 *data,u8 len)
{
	rtx.pc_debug_len=len;
	memcpy(rtx.pc_debugdata,data,len);
}

//debug sullo schermo
void lcd_rtxDebug(u8 *data,u8 len)
{
	rtx.rtx_debug_len=len;
	memcpy(rtx.rtx_debugdata,data,len);
}

void blink(u8 ms)
{
	GPIO_ResetBits(GPIOC, GPIO_Pin_13);
	led=ms;
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
