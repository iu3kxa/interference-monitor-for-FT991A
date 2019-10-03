//Air Traffic Route Control Center, interferences previsions

#define EEPROM_SIGNATURE_1 0x55	//need to check if the data in eeprom is initialized
#define EEPROM_SIGNATURE_2 0xAE

#define EE_CH_PAGE 11							//first page in eeprom containing the air traffic informations
#define MAX_CHANNELS 24					//max frequenze da monitorabili

//bandwidth
#define DEVIAZIONE_AM_2500	2500	//deviation left and right -2500 to +2500
#define DEVIAZIONE_AM_3000	3000
#define DEVIAZIONE_AM_3500	3500
#define DEVIAZIONE_AM_4000	4000
#define DEVIAZIONE_AM_5000	5000

//possible bandwidth, need correction
#define SSB_BANDWIDTH 2400			//deviation for various modes
#define FT8_BANDWIDTH 1400			//potential deviation, depends on the software, code need same tweaks since it start at 1000
#define FT4_BANDWIDTH 1450
#define C4FM_BANDWIDTH 5000
#define FM_BANDWIDTH 4000
#define AM_BANDWIDTH 3500
#define CW_BANDWIDTH 300
#define RTTY_BANDWIDTH 1000

//blacklisted periods
#define ORARIO0024 	0xff	
#define ORARIO0820 	0x02				//more flights, more sectors are open
#define STAGIONEMAX 	0x04			//more flights in summer
#define STAGIONEMED 	0x08
#define STAGIONEMIN 	0x10
#define FERIALE		0x20					//work days
#define WEEKEND		0x40					//Saturday and sunday more flights, in july theu hit ~4900 commrcial flights in a single day
#define ALWAYS			0xff					//all day, for example first sector and info traffic are always open

#define ETH_BUF_LEN	200					//maximun lengh of data to send on a single udp packet, maximum 1500

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "stm32f10x.h"
#include "main.h"
#include "atrcc.h"
#include "usart.h"
#include "rtx.h"
#include "eth_w5500.h"
#include "eth_socket.h"
#include "i2c.h"
#include "hw_config.h"
#include "lcd_core.h"
#include "rtc.h"
#include "touch.h"

extern unsigned char i2cbuf[I2C_BUFSIZE];
extern unsigned char buf[16];
u8 eth_buf[ETH_BUF_LEN];

u8 total=0;												//numero frequenze in database
extern u16 riga;
extern struct Time_s s_TimeStructVar;
extern struct Date_s s_DateStructVar;
extern struct coord pos;
u16 total_deviation;

struct _atrcc_frequencies atrcc[MAX_CHANNELS];
extern struct _rtx_status rtx;
static u32 frequenze_enav[MAX_CHANNELS] =
{
	120725000,	123100000,
	124150000,	124700000,
	124750000,	124740000,
	125900000,	129075000,
	131525000,	131725000,	//acars receivers??? i'll ask to Techno Sky about
	133300000,	134625000,
	134750000,	135000000,
	120725000,	0,
	0,				0,								//same frequency may stay reserved
	0,				0,
	0,				0,
	0,				0
};

static char udp_msg_line_start[] = "<BR>";
static char udp_msg_line_stop[] = "</B>";

void calculate_emi(void);

//verifica la eeprom e carica i valori iniziali se necessario.
void eeprom_check(void)
{
	u8 i;
	u16 addr;
	if( eeprom_read(0) != EEPROM_SIGNATURE_1 || eeprom_read(1) != EEPROM_SIGNATURE_2)
	{
		struct _atrcc_frequencies eeprom;	
		
		LCD_writeString((u8 *) "EEprom vuota, scrivo pagine:");
		
		//inserisce signature
		eeprom_write(0,EEPROM_SIGNATURE_1);
		eeprom_write(1,EEPROM_SIGNATURE_2);
		
		//inserire qui altre variabili
		
		//inserisce canali preset
		for (i=0;i<MAX_CHANNELS;i++)
		{
			LCD_writeString((u8 *) " ");
			itoa(i, buf, 10);
			LCD_writeString(buf);
			
			eeprom.frequenza=frequenze_enav[i];
			if(frequenze_enav[i] != 0)
				eeprom.deviazione=DEVIAZIONE_AM_4000;
			else
				eeprom.deviazione=0;
			
			eeprom.attivo=ALWAYS;
			eeprom.orari=ORARIO0024;
			
			addr=(EEPROM_PAGE_SIZE) * (i + EE_CH_PAGE);
			eeprom_sequential_write(addr, (u8 *) & eeprom,sizeof(eeprom));
		}
	}
	else
	{
		LCD_setCursor(0,riga+=20);
		LCD_writeString((u8 *) "Checksum EEprom valido.");
	}
}


//carica i variabili da eeprom
u8 atrcc_init(void)
{
	u8 i,count=0;
	u16 addr;
	struct _atrcc_frequencies eeprom;
	
	rtx.pc_debug_len=0;
	rtx.rtx_debug_len=0;

	//popola lista
	for (i=0;i<MAX_CHANNELS;i++)
	{
		addr=(EEPROM_PAGE_SIZE) * (i + EE_CH_PAGE);
		eeprom_multiread(addr,(u8 *) &eeprom,sizeof(eeprom));
		
		if(eeprom.deviazione == 0)
			atrcc[i].deviazione=DEVIAZIONE_AM_4000;	//se valore mancante imposta +- 4khz
		else
			atrcc[i].deviazione=eeprom.deviazione;
		
		atrcc[i].frequenza=eeprom.frequenza;
		
		if(atrcc[i].frequenza)
			count++;
	}
	return count;
}

//initialize main page on display
void atrcc_main_open (void)
{		
	LCD_fillScreen(BLACK);
	LCD_print(410,300,(u8 *) "MENU",WHITE,BLACK,2);
	rtx.lcd_update=1;
	rtx.vfo_A_old=1;			//invalidate to force display
}

//update main page
void atrcc_main_run (void)
{
	if(rtx.lcd_update)
	{
		u32 ch=0,u=0;
		rtx.lcd_update=0;
		GPIO_ResetBits(GPIOC, GPIO_Pin_13);
		//draw spectrum bars
		if(rtx.vfo_A!=rtx.vfo_A_old || rtx.mode != rtx.mode_old)
		{
			//aggiorna spettro delle varie frequenze
			while(atrcc[ch].frequenza)
			{
				itoa(atrcc[ch].frequenza/1000,buf,10);
				LCD_print(0,30+(9*ch),buf,WHITE,BLACK,1);
				//LCD_fillRect(42, 32+(9*ch),430, 5, GREEN);		//disegna barra
				ch++;
			}
			calculate_emi();
			rtx.vfo_A_old=rtx.vfo_A;
			rtx.mode_old = rtx.mode;
		}
		//informazioni prima riga
		LCD_print(5,1,(u8 *) "F",WHITE,rtx.conflict==0?BLACK:RED,2);
		itoa(rtx.vfo_A,buf,10);
		LCD_writeString(buf);
		
		//transmission mode
		LCD_writeString((u8 *) " ");
		switch(rtx.mode)
		{
			case RTX_MODE_LSB:
				LCD_writeString((u8 *) "SSB/LSB");
			break;
			case RTX_MODE_USB:
				LCD_writeString((u8 *) "SSB/USB");
			break;
			case RTX_MODE_CW:
				LCD_writeString((u8 *) "CW");
			break;
			case RTX_MODE_FM:
				LCD_writeString((u8 *) "FM");
			break;
			case RTX_MODE_AM:
				LCD_writeString((u8 *) "AM");
			break;
			case RTX_MODE_RTTY_LSB:
				LCD_writeString((u8 *) "RTTY");
			break;
			case RTX_MODE_CW_R:
				LCD_writeString((u8 *) "CW_R");
			break;
			case RTX_MODE_DATA_LSB:
				LCD_writeString((u8 *) "MFSK");
			break;
			case  RTX_MODE_RTTY_USB:
				LCD_writeString((u8 *) "RTTY");
			break;
			case RTX_MODE_DATA_FM:
				LCD_writeString((u8 *) "FMPKT");
			break;
			case RTX_MODE_FM_N:
				LCD_writeString((u8 *) "FM N");
			break;
			case RTX_MODE_DATA_USB:
				LCD_writeString((u8 *) "FT4/FT8");
			break;
			case RTX_MODE_AM_N:
				LCD_writeString((u8 *) "AM N");
			break;
			case RTX_MODE_C4FM:
				LCD_writeString((u8 *) "C4FM");
			break;
			default:
				LCD_writeString((u8 *) "NONE");
			break;
		}
		//write power
		LCD_writeString((u8 *) " PWR: ");
		itoa(rtx.power,buf,10);
		LCD_writeString(buf);
		LCD_writeString((u8 *) " Watt ");
				
		//stato tramissione rosso/ricezione verde
		if(rtx.txmode)
		{
			LCD_setTextBgColor(RED);
			LCD_setTextColor(BLACK);
			LCD_writeString((u8 *) " TX ");
		}
		else
		{
			LCD_setTextBgColor(GREEN);
			LCD_setTextColor(BLACK);
			LCD_writeString((u8 *) " RX ");
			if(rtx.tx_time)
				log_udp(rtx.tx_time);			
			rtx.tx_time=0;
		}
		
		//clean dirt
		LCD_setTextBgColor(BLACK);
		u=100000000;
		while(u>rtx.vfo_A+1)
		{
			u/=10;
			LCD_write(' ');
		}
		
	
		if (rtx.timeout)
			LCD_setTextBgColor(BLACK);
		else
		{
			rtx.vfo_A=0;
			LCD_setTextBgColor(RED);		
		}
		LCD_setTextColor(BLACK);
		LCD_setTextSize(2);
		LCD_setCursor(0,290);
		LCD_writeString((u8 *) "ERR: L'FT991A non risponde!");
	}
	GPIO_SetBits(GPIOC, GPIO_Pin_13);
	
	if(pos.spot_X4_Y3 == 12)
				menu_back();
}

//search for possible harmonics overlap
void calculate_emi(void)
{
	u32 fr_left, fr_right,start,stop,f_start,f_stop,vfo_start,vfo_stop,dispmin,dispmax;
	u8 harmonics;
	u8 ch;
	rtx.conflict=0;
	
	#define HERTZ_SPAN_BAR 5000
	#define HERTZ_PER_PIXEL 20
	
	switch(rtx.mode)
	{
		case RTX_MODE_LSB:
			fr_left=SSB_BANDWIDTH;
			fr_right=0;
		break;
		case RTX_MODE_USB:
			fr_left=0;
			fr_right=SSB_BANDWIDTH;
		break;
		case RTX_MODE_CW:
			fr_left=CW_BANDWIDTH;
			fr_right=CW_BANDWIDTH;
		break;
		case RTX_MODE_FM:
			fr_left=FM_BANDWIDTH;
			fr_right=FM_BANDWIDTH;
		break;
		case RTX_MODE_AM:
			fr_left=AM_BANDWIDTH;
			fr_right=AM_BANDWIDTH;
		break;
		case RTX_MODE_RTTY_LSB:
			fr_left=RTTY_BANDWIDTH;
			fr_right=0;
		break;
		case RTX_MODE_CW_R:
			fr_left=CW_BANDWIDTH;
			fr_right=CW_BANDWIDTH;
		break;
		case RTX_MODE_DATA_LSB:
			fr_left=RTTY_BANDWIDTH;
			fr_right=RTTY_BANDWIDTH;
		break;
		case  RTX_MODE_RTTY_USB:
			fr_left=0;
			fr_right=RTTY_BANDWIDTH;
		break;
		case RTX_MODE_DATA_FM:
			fr_left=FM_BANDWIDTH;
			fr_right=FM_BANDWIDTH;
		break;
		case RTX_MODE_FM_N:
			fr_left=FM_BANDWIDTH;
			fr_right=FM_BANDWIDTH;
		break;
		case RTX_MODE_DATA_USB:
			fr_left=0;
			fr_right=FT4_BANDWIDTH;
		break;
		case RTX_MODE_AM_N:
			fr_left=AM_BANDWIDTH;
			fr_right=AM_BANDWIDTH;
		break;
		default:
			fr_left=AM_BANDWIDTH;
			fr_right=AM_BANDWIDTH;
		break;
	}

	total_deviation=fr_left+fr_right;
	
	for(ch=0;ch<MAX_CHANNELS;ch++)
	{
		if(atrcc[ch].frequenza)
		{
			
			dispmin=260-(fr_left/HERTZ_PER_PIXEL);
			dispmax=260+(fr_right/HERTZ_PER_PIXEL);

			LCD_fillRect(42, 32+(9*ch),dispmin-42, 4, GREEN);					//draw initial green bar, free spectrum
			LCD_fillRect(dispmin, 32+(9*ch),dispmax-dispmin, 4,YELLOW);	//draw yellow bar, overlap spectrum
			LCD_fillRect(dispmax, 32+(9*ch),478-dispmax, 4, GREEN);		//draw remaining green bar
						
			//indica i valori del range indicato
			if(ch<(MAX_CHANNELS-1) && atrcc[ch+1].frequenza ==0)
			{
				LCD_print(42,30+9+(9*ch),(u8 *) "-",WHITE,BLACK,1);
				itoa(HERTZ_SPAN_BAR,buf,10);
				LCD_print(49,30+9+(9*ch),buf,WHITE,BLACK,1);
				LCD_print(450,30+9+(9*ch),(u8 *) "+",WHITE,BLACK,1);
				itoa(HERTZ_SPAN_BAR,buf,10);
				LCD_print(455,30+9+(9*ch),buf,WHITE,BLACK,1);
			}
		
			f_start   = atrcc[ch].frequenza-atrcc[ch].deviazione;
			f_stop    = atrcc[ch].frequenza+atrcc[ch].deviazione;
			
			//search for any possible conflict on all harmonics
			for(harmonics=0;(rtx.vfo_A*harmonics)<137000000 && rtx.vfo_A > 1840000 && rtx.vfo_A <54000000 ; harmonics++)
			{
				vfo_start = (rtx.vfo_A-fr_left)*harmonics;
				vfo_stop = (rtx.vfo_A+fr_right)*harmonics;
				
				//check for spectrum overlap
				if((vfo_start >= f_start && vfo_start <= f_stop) || (vfo_stop >= f_start && vfo_stop <= f_stop))
					rtx.conflict=1;	//mark possible interference
				
				start=stop=0;
				
				//show spectrum overlap on display
				//start spectrum
				if(f_start < (rtx.vfo_A+HERTZ_SPAN_BAR)*harmonics && f_start > (rtx.vfo_A-HERTZ_SPAN_BAR)*harmonics)	//se punto di inizio nel range visivo
					{
						if(f_start > (rtx.vfo_A-HERTZ_SPAN_BAR)*harmonics)
						{
							start=f_start-(rtx.vfo_A-HERTZ_SPAN_BAR)*harmonics;	//distance from harmonic of vfo
							start=start/harmonics;															//distance from vfo fundamental
							start=start/HERTZ_PER_PIXEL;												//rescale to display
							if (start < 42)																			//keep inside bars
								start=42;
						}
						else
							start=42;																					//inizio x barra
					}

				//stop spectrum				
				if(f_stop < (rtx.vfo_A+HERTZ_SPAN_BAR)*harmonics && f_stop > (rtx.vfo_A-HERTZ_SPAN_BAR)*harmonics)	
				{
					if(f_stop < (rtx.vfo_A+HERTZ_SPAN_BAR)*harmonics)
					{
						stop=f_stop-(rtx.vfo_A-HERTZ_SPAN_BAR)*harmonics;			//distance from harmonic of vfo
						stop=stop/harmonics;																	//distance from vfo fundamental
						stop=stop/HERTZ_PER_PIXEL;													//rescale to display
						if(stop>478)																					//keep inside bars
							stop=478;
					}
				}
				
				//CASINI A 3.851300
				if(start && stop)
					LCD_fillRect(start, 32+(9*ch),stop-start, 4, RED);						//draw overlap area
			}
		}
	}
	
	if(rtx.conflict==1)
		usartx_dmaSend8(SERIAL2, (u8*) "TX0;",4);		//stop transmission immefdiately: DOESN'T WORK!!!
}

//send transmission log to webserver
void log_udp (u16 time)
{
	u8 destip[4]={1,1,1,254};
	u8 buf_pos=0;
	//<BR>
	memcpy(eth_buf , udp_msg_line_start , sizeof(udp_msg_line_start)-1);
	buf_pos += sizeof(udp_msg_line_start)-1;

	//data
	eth_buf[buf_pos++]=(s_DateStructVar.Year/1000) + 0x30;
	eth_buf[buf_pos++]=((s_DateStructVar.Year/100)%10) + 0x30;
	eth_buf[buf_pos++]=((s_DateStructVar.Year/10)%10) + 0x30;
	eth_buf[buf_pos++]=(s_DateStructVar.Year%10) + 0x30;
	eth_buf[buf_pos++]='-';
	eth_buf[buf_pos++]=(s_DateStructVar.Month/10) + 0x30;
	eth_buf[buf_pos++]=(s_DateStructVar.Month%10) + 0x30;
	eth_buf[buf_pos++]='-';
	eth_buf[buf_pos++]=(s_DateStructVar.Day/10) + 0x30;
	eth_buf[buf_pos++]=(s_DateStructVar.Day%10) + 0x30;
	eth_buf[buf_pos++]=' ';

	//mese 
	memcpy(&eth_buf[buf_pos], MonthsNames[s_DateStructVar.Month-1] , sizeof(MonthsNames[s_DateStructVar.Month-1])-1);
	buf_pos+=sizeof(MonthsNames[s_DateStructVar.Month-1])-1;
	
	eth_buf[buf_pos++]=' ';
	
	//orario
	eth_buf[buf_pos++]=s_TimeStructVar.HourHigh + 0x30;
	eth_buf[buf_pos++]=s_TimeStructVar.HourLow + 0x30;
	eth_buf[buf_pos++]=':';
	eth_buf[buf_pos++]=s_TimeStructVar.MinHigh + 0x30;
	eth_buf[buf_pos++]=s_TimeStructVar.MinLow + 0x30;
	eth_buf[buf_pos++]=':';
	eth_buf[buf_pos++]=s_TimeStructVar.SecHigh + 0x30;
	eth_buf[buf_pos++]=s_TimeStructVar.SecLow + 0x30;
	eth_buf[buf_pos++]=' ';
	eth_buf[buf_pos++]='U';
	eth_buf[buf_pos++]='T';
	eth_buf[buf_pos++]='C';
	
	eth_buf[buf_pos++]=' ';
	eth_buf[buf_pos++]='-';
	eth_buf[buf_pos++]=' ';
	
	eth_buf[buf_pos++]='D';
	eth_buf[buf_pos++]='u';
	eth_buf[buf_pos++]='r';
	eth_buf[buf_pos++]='a';
	eth_buf[buf_pos++]='t';
	eth_buf[buf_pos++]='a';
	eth_buf[buf_pos++]=':';
	eth_buf[buf_pos++]=' ';
	
	if(time>99)
		eth_buf[buf_pos++]=((time/100)%10) + 0x30;
	if(time>9)
		eth_buf[buf_pos++]=((time/10)%10) + 0x30;
	else
		eth_buf[buf_pos++]=' ';
	eth_buf[buf_pos++]=(time%10) + 0x30;
		eth_buf[buf_pos++]=' ';
	eth_buf[buf_pos++]='s';
	eth_buf[buf_pos++]='e';
	eth_buf[buf_pos++]='c';
	eth_buf[buf_pos++]='o';
	eth_buf[buf_pos++]='n';
	eth_buf[buf_pos++]='d';
	eth_buf[buf_pos++]='i';
	
	eth_buf[buf_pos++]=' ';
	eth_buf[buf_pos++]='-';
	eth_buf[buf_pos++]=' ';
	
	eth_buf[buf_pos++]='M';
	eth_buf[buf_pos++]='o';
	eth_buf[buf_pos++]='d';
	eth_buf[buf_pos++]='o';
	eth_buf[buf_pos++]=':';
	eth_buf[buf_pos++]=' ';
	
	if(rtx.txmode == RTX_TX_TXCAT)	//transmission originated for the autotuner.
	{
			eth_buf[buf_pos++]='T';
			eth_buf[buf_pos++]='U';
			eth_buf[buf_pos++]='N';
			eth_buf[buf_pos++]='E';
	}
	else
		switch(rtx.mode)
		{
			case RTX_MODE_LSB:
				eth_buf[buf_pos++]='L';
				eth_buf[buf_pos++]='S';
				eth_buf[buf_pos++]='B';
			break;
			case RTX_MODE_USB:
				eth_buf[buf_pos++]='U';
				eth_buf[buf_pos++]='S';
				eth_buf[buf_pos++]='B';
			break;
			case RTX_MODE_CW:
				eth_buf[buf_pos++]='C';
				eth_buf[buf_pos++]='W';
			break;
			case RTX_MODE_FM:
				eth_buf[buf_pos++]='F';
				eth_buf[buf_pos++]='M';
			break;
			case RTX_MODE_AM:
				eth_buf[buf_pos++]='A';
				eth_buf[buf_pos++]='M';
			break;
			case RTX_MODE_RTTY_LSB:
				eth_buf[buf_pos++]='R';
				eth_buf[buf_pos++]='T';
				eth_buf[buf_pos++]='T';
				eth_buf[buf_pos++]='Y';
				eth_buf[buf_pos++]='-';
				eth_buf[buf_pos++]='L';
				eth_buf[buf_pos++]='S';
				eth_buf[buf_pos++]='B';
			break;
			case RTX_MODE_CW_R:
				eth_buf[buf_pos++]='C';
				eth_buf[buf_pos++]='W';
				eth_buf[buf_pos++]='-';
				eth_buf[buf_pos++]='R';
			break;
			case RTX_MODE_DATA_LSB:
				eth_buf[buf_pos++]='D';
				eth_buf[buf_pos++]='A';
				eth_buf[buf_pos++]='T';
				eth_buf[buf_pos++]='A';
				eth_buf[buf_pos++]='-';
				eth_buf[buf_pos++]='L';
				eth_buf[buf_pos++]='S';
				eth_buf[buf_pos++]='B';
			break;
			case RTX_MODE_RTTY_USB:
				eth_buf[buf_pos++]='L';
				eth_buf[buf_pos++]='S';
				eth_buf[buf_pos++]='B';
			break;
			case RTX_MODE_DATA_FM:
				eth_buf[buf_pos++]='D';
				eth_buf[buf_pos++]='A';
				eth_buf[buf_pos++]='T';
				eth_buf[buf_pos++]='A';
				eth_buf[buf_pos++]='-';
				eth_buf[buf_pos++]='A';
				eth_buf[buf_pos++]='F';
				eth_buf[buf_pos++]='M';
			break;
			case RTX_MODE_FM_N:
				eth_buf[buf_pos++]='F';
				eth_buf[buf_pos++]='M';
				eth_buf[buf_pos++]='-';
				eth_buf[buf_pos++]='N';
			break;
			case RTX_MODE_DATA_USB:
				eth_buf[buf_pos++]='F';
				eth_buf[buf_pos++]='T';
				eth_buf[buf_pos++]='8';
				eth_buf[buf_pos++]='/';
				eth_buf[buf_pos++]='U';
				eth_buf[buf_pos++]='S';
				eth_buf[buf_pos++]='B';
			break;
			case RTX_MODE_AM_N:
				eth_buf[buf_pos++]='A';
				eth_buf[buf_pos++]='M';
				eth_buf[buf_pos++]='-';
				eth_buf[buf_pos++]='N';
			break;
			case RTX_MODE_C4FM:
				eth_buf[buf_pos++]='C';
				eth_buf[buf_pos++]='4';
				eth_buf[buf_pos++]='F';
				eth_buf[buf_pos++]='M';
			break;
			default:
				eth_buf[buf_pos++]='U';
				eth_buf[buf_pos++]='N';
				eth_buf[buf_pos++]='D';
				eth_buf[buf_pos++]='E';
				eth_buf[buf_pos++]='F';
			break;
		}
	
	eth_buf[buf_pos++]=' ';
	eth_buf[buf_pos++]='-';
	eth_buf[buf_pos++]=' ';
	
	
	eth_buf[buf_pos++]='F';
	eth_buf[buf_pos++]='r';
	eth_buf[buf_pos++]='e';
	eth_buf[buf_pos++]='q';
	eth_buf[buf_pos++]='u';
	eth_buf[buf_pos++]='e';
	eth_buf[buf_pos++]='n';
	eth_buf[buf_pos++]='z';
	eth_buf[buf_pos++]='a';
	eth_buf[buf_pos++]=':';
	eth_buf[buf_pos++]=' ';
	
	
	if(rtx.vfo_A>99999999)
		eth_buf[buf_pos++]=0X30+(rtx.vfo_A/100000000);								//centinaia mhz
	if(rtx.vfo_A>9999999)
		eth_buf[buf_pos++]=0X30+((rtx.vfo_A%100000000)/10000000);		//decine di mhz	
	eth_buf[buf_pos++]=0X30+((rtx.vfo_A%10000000) /1000000);			//unita
	eth_buf[buf_pos++]='.';
	eth_buf[buf_pos++]=0X30+((rtx.vfo_A%1000000)  /100000);
	eth_buf[buf_pos++]=0X30+((rtx.vfo_A%100000)   /10000);
	eth_buf[buf_pos++]=0X30+((rtx.vfo_A%10000)   /1000);
	eth_buf[buf_pos++]=0X30+((rtx.vfo_A%1000)  /100);
	//eth_buf[buf_pos++]=0X30+((rtx.vfo_A%100)  /10);
	//eth_buf[buf_pos++]=0X30+(rtx.vfo_A%10);
	
	eth_buf[buf_pos++]=' ';
	eth_buf[buf_pos++]='M';
	eth_buf[buf_pos++]='H';
	eth_buf[buf_pos++]='z';
	
	eth_buf[buf_pos++]=' ';
	eth_buf[buf_pos++]='-';
	eth_buf[buf_pos++]=' ';
	
	eth_buf[buf_pos++]='P';
	eth_buf[buf_pos++]='o';
	eth_buf[buf_pos++]='w';
	eth_buf[buf_pos++]='e';
	eth_buf[buf_pos++]='r';
	eth_buf[buf_pos++]=':';
	eth_buf[buf_pos++]=' ';
	
	if(rtx.power>99)
		eth_buf[buf_pos++]=((rtx.power/100)%10) + 0x30;
	if(rtx.power>9)
		eth_buf[buf_pos++]=((rtx.power/10)%10) + 0x30;
	eth_buf[buf_pos++]=(rtx.power%10) + 0x30;
	eth_buf[buf_pos++]='W';
		
	//br
	memcpy(&eth_buf[buf_pos++] , udp_msg_line_stop , sizeof(udp_msg_line_stop)-1);
	buf_pos += sizeof(udp_msg_line_start)-1;
	eth_buf[buf_pos++]=0x0d;
	eth_buf[buf_pos++]=0x0a;
	
	eth_buf[buf_pos]=0;
	sendto(0,eth_buf,buf_pos,destip,37991);
}

