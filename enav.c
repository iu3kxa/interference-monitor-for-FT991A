#define EE_CH_PAGE 11				//prima pagina nella eeprom dedicata alle frequenze
#define CHANNELS 24					//max frequenze da monitorabili

//spettro occupato
#define DEVIAZIONE_AM_2500	2500	//deviazione rispetto alla sottoportante quindi 5khz
#define DEVIAZIONE_AM_3000	3000
#define DEVIAZIONE_AM_3500	3500
#define DEVIAZIONE_AM_4000	4000
#define DEVIAZIONE_AM_5000	5000
//#define FREQUENZA		1
//#define DEVIAZIONE 	2

//periodo utilizzabilita'
#define ORARIO0024 	0x01
#define ORARIO0820 	0x02
#define STAGIONEMAX 	0x04
#define STAGIONEMED 	0x08
#define STAGIONEMIN 	0x10
#define FERIALE		0x20
#define WEEKEND		0x40
#define SEMPRE			0x80

#define ETH_BUF_LEN	200		//lunghezza buffer dati da inviare via ethernet

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "i2c.h"
#include "main.h"
#include "eth_w5500.h"
#include "eth_socket.h"
#include "stm32f10x.h"
#include "hw_config.h"
#include "enav.h"
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

struct Enav_Frequenze enav[CHANNELS];
struct Enav_Status enav_status;
static u32 frequenze[CHANNELS] =
{
	120725000,	123100000,
	124150000,	124700000,
	124750000,	124740000,
	125900000,	129075000,
	131525000,	131725000,	//acars??? chiedere se rx presente
	133300000,	134625000,
	134750000,	135000000,
	120725000,	0,
	0,				0,
	0,				0,
	0,				0,
	0,				0
};

static char udp_msg_line_start[] = "<BR>";
static char udp_msg_line_stop[] = "</BR>";


//carica i valori da eeprom
u8 enav_init(void)
{
	u8 i,count=0;
	struct Enav_Frequenze ee;
	
	enav_status.debug_len=0;

	//popola lista
	for (i=0;i<CHANNELS;i++)
	{
		eeprom_multiread((EEPROM_PAGE_SIZE * i) + EE_CH_PAGE,i2cbuf,I2C_BUFSIZE);
		if(ee.deviazione == 0)
		{
			enav[i].deviazione=DEVIAZIONE_AM_4000;
			enav[i].frequenza=frequenze[i];
		}else{
			enav[i].deviazione=ee.deviazione;
			enav[i].frequenza=ee.frequenza;
		}
		
		if(enav[i].frequenza)
			count++;
	}
	return count;
}

void eeprom_check(void)
{
	u8 i;
	u16 addr;
	if( eeprom_read(0) != 0x55 || eeprom_read(1) != 0xAA)
	{
		LCD_setCursor(0,riga+=20);
		LCD_writeString((u8 *) "EEprom vuota, scrivo pagine:");
		
		//inserisce signature
		eeprom_write(0,0x55);
		eeprom_write(1,0xaa);
		
		//inserire qui altre variabili
		
		//inserisce canai preset
		for (i=0;i<CHANNELS;i++)
		{
			LCD_writeString((u8 *) " ");
			itoa(i, buf, 10);
			LCD_writeString(buf);
			
			addr=(EEPROM_PAGE_SIZE * i) + EE_CH_PAGE;
			eeprom_sequential_write(addr, (u8 *) & frequenze[i],4);
			
			//se trattasi di frequenza valid
			if(frequenze[i] != 0)
			{
				eeprom_write(addr+4, DEVIAZIONE_AM_4000>>8);
				eeprom_write(addr+5, DEVIAZIONE_AM_4000&0xff);
			}else{
				eeprom_write(addr+4,0);
				eeprom_write(addr+5,0);
			}
			eeprom_write(addr+6,SEMPRE);
		}
	}
}

void log_udp (void)
{
	u8 destip[4]={10,0,0,69};
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
	
	//br
	memcpy(&eth_buf[buf_pos++] , udp_msg_line_stop , sizeof(udp_msg_line_stop)-1);
	buf_pos += sizeof(udp_msg_line_start)-1;
	eth_buf[buf_pos++]=0x0d;
	eth_buf[buf_pos++]=0x0a;
	
	sendto(0,eth_buf,buf_pos,destip,7777);
}

void enav_main_open (void)
{		
	LCD_fillScreen(BLACK);
	//LCD_fillRect(395, 285, 80, 30, NAVY);
	LCD_setTextBgColor(BLACK);
	LCD_setTextColor(WHITE);
	LCD_setTextSize(2);
	LCD_setCursor(410, 300);
	LCD_writeString((u8*) "MENU");
	enav_status.changed=1;
}

void enav_main_run (void)
{
	if(enav_status.changed)
	{
		u8 i=0;
		
		///////////////////
		enav_status.power_rtx=90;
		//enav_status.frequenza_rtx=14074000;
		
		LCD_setTextBgColor(BLACK);
		LCD_setTextColor(WHITE);
		LCD_setTextSize(2);
		LCD_setCursor(5,1);
		LCD_writeString((u8 *) "F: ");
		itoa(enav_status.frequenza_rtx,buf,10);
		LCD_writeString(buf);
		LCD_writeString((u8 *) " USB/FT8 PWR: ");

		itoa(enav_status.power_rtx,buf,10);
		LCD_writeString(buf);
		LCD_writeString((u8 *) " Watt ");
		
		if(enav_status.tx)
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
		}
			
		LCD_setTextBgColor(BLACK);
		LCD_setTextColor(WHITE);
		LCD_setTextSize(1);
		
		while(enav[i].frequenza)
		{
			//if (enav[i].attivo)
			{
				LCD_setCursor(5, 30+(9*i));
				itoa(enav[i].frequenza/1000,buf,10);
				LCD_writeString(buf);
				LCD_fillRect(42, 32+(9*i),430, 5, GREEN);		//draw safe bar
			}
			i++;
		}
	
	if (enav_status.poweron)
		LCD_setTextBgColor(BLACK);
	else
		LCD_setTextBgColor(RED);
	LCD_setTextColor(BLACK);
	LCD_setTextSize(2);
	LCD_setCursor(10,42+(9*i));
	LCD_writeString((u8 *) "ERR: L'FT991A e' spento o scollegato!");
	enav_status.changed=0;
	}
	if(pos.spot_X4_Y3 == 12)
				menu_back();
}
