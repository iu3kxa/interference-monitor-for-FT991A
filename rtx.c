#include "rtx.h"
#include "usart.h"

#define RTX_TIMEOUT 10					//non responding tries

struct _rtx_status rtx;
extern struct _usart_data ser[];

void rtx_init (void)
{
	memset (&rtx, 0, sizeof(rtx));	//azzera tutto
}


void rtx_poll (void)
{
	switch (rtx.poll)
	{
		case 0:
			usartx_dmaSend8(SERIAL2,(u8 *) "IF;", 3);		//lettura informazioni
			rtx.poll++;
			if(rtx.vfo_A_old != rtx.vfo_A)
				rtx.poll=0;
		break;
		case 1:
			usartx_dmaSend8(SERIAL2,(u8 *) "TX;", 3);		//ltrasmissione in corso
			rtx.poll++;
		break;
		case 2:
			usartx_dmaSend8(SERIAL2,(u8 *) "PC;", 3);		//lettura potenza impostata
			rtx.poll++;
		break;
		case 3:
			usartx_dmaSend8(SERIAL2,(u8 *) "TX;", 3);		//ltrasmissione in corso
			rtx.poll++;
		break;
		case 4:
			rtx.poll++;
			if (rtx.timeout)
			{
				rtx.timeout--;
				if (!rtx.timeout)									//no repsponse from rtx
				{
					rtx.vfo_A=0;									//update display
					rtx.lcd_update=1;
				}
			}
			usartx_dmaSend8(SERIAL2,(u8 *) "IF;", 3);		//lettura informazioni
		break;	
		case 5:
			usartx_dmaSend8(SERIAL2,(u8 *) "TX;", 3);		//ltrasmissione in corso
			rtx.poll++;
		break;	
		default:
			rtx.poll=0;
		break;
	}
}

//interpreta comandi ricevuti dall-rtx
void rtx_ft991a_decode (u8 * message,u8 len)
{
	u8 i;
	rtx.poweron=1;
	rtx.timeout=RTX_TIMEOUT;
	
	//information
	if(message[0]=='I' && message[1]=='F')
	{
		rtx.vfo_A=0;
		
		//frequenza corrente(P2)
		for(i=5;i<14;i++)
		{
			rtx.vfo_A*=10;
			rtx.vfo_A+=message[i]-0x30;
		}
		
		//modulazione
		rtx.mode=message[21];
		
		if(rtx.vfo_A!=rtx.vfo_A_old)
		{
			rtx.changed=10;
			rtx.lcd_update=1;
		}
		
		if(rtx.mode != rtx.mode_old)
		{
			rtx.changed=10;
			rtx.lcd_update=1;
		}

	}
	
	//vfo_A
	if(message[0]=='F' && message[1]=='A')
	{	
		rtx.vfo_A=0;
		for(i=2;i<11;i++)
		{
			rtx.vfo_A*=10;
			rtx.vfo_A+=message[i]-0x30;
		}
	
		if(rtx.vfo_A!=rtx.vfo_A_old)
		{
			rtx.changed=10;
			rtx.lcd_update=1;
			//rtx.vfo_A_old=rtx.vfo_A;
		}
	}
	
	//vfo B
	if(message[0]=='F' && message[1]=='B')
	{	
		rtx.vfo_B=0;
		for(i=2;i<11;i++)
		{
			rtx.vfo_B*=10;
			rtx.vfo_B+=message[i]-0x30;
		}
		
		if(rtx.vfo_B!=rtx.vfo_B_old)
		{
			rtx.changed=10;
			rtx.vfo_B_old=rtx.vfo_B;
		}
	}
	
	//potenza di trasmissione
	if(message[0]=='P' && message[1]=='C')
	{	
		rtx.power=0;
		for(i=2;i<5;i++)
		{
			rtx.power*=10;
			rtx.power+=message[i]-0x30;
		}
		rtx.lcd_update=1;
		rtx.power=rtx.power>>1;
	}
	
	//trasmissione in corso
	if(message[0]=='T' && message[1]=='X')
	{	
		rtx.txmode=message[2]-0x30;
		rtx.changed=10;
		rtx.lcd_update=1;
	}
	
	
	//Apparato acceso o spento
	if(message[0]=='P' && message[1]=='S')
	{	
		rtx.poweron=message[2]-0x30;
		rtx.changed=10;
	}
}
