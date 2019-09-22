//Air Traffic Route Control Center interference monitor

#define EEPROM_SIGNATURE_1 0x55
#define EEPROM_SIGNATURE_2 0xAE

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
#include "stm32f10x.h"
#include "main.h"
#include "artcc.h"
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

struct _artcc_frequencies artcc[CHANNELS];
extern struct _rtx_status rtx;
static u32 frequenze_enav[CHANNELS] =
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
static char udp_msg_line_stop[] = "</B>";

//verifica la eeprom e carica i valori iniziali se necessario.
void eeprom_check(void)
{
	u8 i;
	u16 addr;
	if( eeprom_read(0) != EEPROM_SIGNATURE_1 || eeprom_read(1) != EEPROM_SIGNATURE_2)
	{
		LCD_setCursor(0,riga+=20);
		LCD_writeString((u8 *) "EEprom vuota, scrivo pagine:");
		
		//inserisce signature
		eeprom_write(0,EEPROM_SIGNATURE_1);
		eeprom_write(1,EEPROM_SIGNATURE_2);
		
		//inserire qui altre variabili
		
		//inserisce canali preset
		for (i=0;i<CHANNELS;i++)
		{
			LCD_writeString((u8 *) " ");
			itoa(i, buf, 10);
			LCD_writeString(buf);
			
			addr=(EEPROM_PAGE_SIZE * i) + EE_CH_PAGE;
			eeprom_sequential_write(addr, (u8 *) & frequenze_enav[i],4);
			
			//se trattasi di frequenza valida
			if(frequenze_enav[i] != 0)
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
	else
	{
		LCD_setCursor(0,riga+=20);
		LCD_writeString((u8 *) "Checksum EEprom valido.");
	}
}


//carica i variabili da eeprom
u8 artcc_init(void)
{
	u8 i,count=0;
	struct _artcc_frequencies eeprom;
	
	rtx.pc_debug_len=0;
	rtx.rtx_debug_len=0;

	//popola lista
	for (i=0;i<CHANNELS;i++)
	{
		eeprom_multiread((EEPROM_PAGE_SIZE * i) + EE_CH_PAGE,i2cbuf,sizeof(eeprom));
		if(eeprom.deviazione == 0)
			artcc[i].deviazione=DEVIAZIONE_AM_4000;	//se valore mancante imposta +- 4khz
		else
			artcc[i].deviazione=eeprom.deviazione;
		artcc[i].frequenza=eeprom.frequenza;
		
		if(artcc[i].frequenza)
			count++;
	}
	return count;
}

//invia al webserver il log della trasmissione effetuata
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

void artcc_main_open (void)
{		
	LCD_fillScreen(BLACK);
	LCD_print(410,300,(u8 *) "MENU",WHITE,BLACK,2);
}

void artcc_main_run (void)
{
	if(rtx.lcd_update)
	{
		u8 i=0;
		rtx.lcd_update=0;
		
		//informazioni prima riga
		LCD_print(5,1,(u8 *) "F",WHITE,BLACK,2);
		itoa(rtx.vfo_A,buf,10);
		LCD_writeString(buf);
		LCD_writeString((u8 *) " USB/FT8 PWR: ");

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
		
		if(rtx.vfo_A!=rtx.vfo_A_old)
		{
			//aggiorna spettro delle varie frequenze
			while(artcc[i].frequenza)
			{

				itoa(artcc[i].frequenza/1000,buf,10);
				LCD_print(0,30+(9*i),buf,WHITE,BLACK,1);
				LCD_fillRect(42, 32+(9*i),430, 5, GREEN);		//disegna barra
				i++;
			}
			rtx.vfo_A_old=rtx.vfo_A;
		}
	
	if (rtx.poweron)
		LCD_setTextBgColor(BLACK);
	else
	{
		rtx.vfo_A=0;
		LCD_setTextBgColor(RED);
		
	}
	LCD_setTextColor(BLACK);
	LCD_setTextSize(2);
	LCD_setCursor(10,290);
	LCD_writeString((u8 *) "ERR: L'FT991A non risponde!");
	}
	
	if(pos.spot_X4_Y3 == 12)
				menu_back();
}

void calculate_emi(void)
{
	u32 fr_left;
	u32 fr_right;
	fr_left=rtx.vfo_A-rtx.spettro_rtx_left;
	fr_right=rtx.vfo_A+rtx.spettro_rtx_right;
	
	artcc[0].conflitto=0;
	
	while(fr_left <artcc[0].frequenza && fr_right <artcc[0].frequenza)
	{
		if(fr_left > (artcc[0].frequenza-artcc[0].deviazione) && fr_right < (artcc[0].frequenza+artcc[0].deviazione))
			artcc[0].conflitto=1;
	}

	
	
	
	
}

