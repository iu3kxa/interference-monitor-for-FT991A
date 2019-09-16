#define USART1_USE_DMA 0

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "usart.h"
#include "lcd_core.h"
#include "enav.h"
#include "main.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_spi.h"
#include "stm32f10x.h"

struct _usart_data ser[3];	//variabili usart
extern struct Enav_Status enav_status;


void usart1_process_data(void);
void usart2_process_data(void);
void usart3_process_data(void);

//inizializza seriali
void usartInit(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	//azzera struttura dati
	memset (&ser, 0, sizeof(ser));
	
	ser[SERIAL1].usartN=USART1;
	ser[SERIAL1].dma_rx=DMA1_Channel4;		//non usato
	ser[SERIAL1].dma_tx=DMA1_Channel5;
	ser[SERIAL2].usartN=USART2;
	ser[SERIAL2].dma_rx=DMA1_Channel6;		//non usato
	ser[SERIAL2].dma_tx=DMA1_Channel7;
	ser[SERIAL3].usartN=USART3;
	ser[SERIAL3].dma_rx=DMA1_Channel3;		//non usato
	ser[SERIAL3].dma_tx=DMA1_Channel2;
	
	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	
	//abilita pins
	//GPIO_PinRemapConfig(AFIO_MAPR_USART1_REMAP,ENABLE);	//sposta usart1 su PA9 e PA10
	
	//usart1 e 2
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_2|GPIO_Pin_9;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_3|GPIO_Pin_10;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	//usart3
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_10;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_11;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStruct);


	//inizializza seriali
	USART_DeInit(USART1);
	USART_DeInit(USART2);
	USART_DeInit(USART3);
	USART_InitStructure.USART_BaudRate            = 38400;
	USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits            = USART_StopBits_1;
	USART_InitStructure.USART_Parity              = USART_Parity_No;
	USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(USART1, &USART_InitStructure);
	USART_Init(USART2, &USART_InitStructure);
	USART_Init(USART3, &USART_InitStructure);

	//abilita seriali
	USART_Cmd(USART1, ENABLE);
	USART_Cmd(USART2, ENABLE);
	USART_Cmd(USART3, ENABLE);
	
	//irq per usart1
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	NVIC_EnableIRQ(USART1_IRQn);									//usart1
	USART_ITConfig(USART1, USART_IT_RXNE , ENABLE);	
	
	//irq per usart2
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	NVIC_EnableIRQ(USART2_IRQn);									//usart2
	USART_ITConfig(USART2, USART_IT_RXNE , ENABLE);

	//irq per usart3
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	NVIC_EnableIRQ(USART3_IRQn);									//usart3
	USART_ITConfig(USART2, USART_IT_RXNE , ENABLE);

	//dma per trasmissione
	//USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);		//in conflitto con spi
	USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
	USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
}



//invia dati alla porta prescelta
void usartx_dmaSend8(u8 sn,u8 *data, u32 n)
{
	DMA_InitTypeDef dmaStructure;
		
	DMA_Cmd(ser[sn].dma_tx, DISABLE);
	DMA_StructInit(&dmaStructure);
	dmaStructure.DMA_M2M						= DMA_M2M_Disable;
	dmaStructure.DMA_MemoryBaseAddr		= (u32) data;
	/*switch(sn)
	{
		case 0:
			dmaStructure.DMA_PeripheralBaseAddr = (u32) &USART1->DR;
		break;
		case 1:
			dmaStructure.DMA_PeripheralBaseAddr = (u32) &USART2->DR;
		break;
		case 2:
			dmaStructure.DMA_PeripheralBaseAddr = (u32) &USART3->DR;
		break;
	}*/
	dmaStructure.DMA_PeripheralBaseAddr = (u32) &ser[sn].usartN->DR;
	dmaStructure.DMA_Priority           = DMA_Priority_Medium;
	dmaStructure.DMA_BufferSize			= n;
	dmaStructure.DMA_Mode               = DMA_Mode_Normal;
	dmaStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
	dmaStructure.DMA_PeripheralInc		= DMA_PeripheralInc_Disable;
	dmaStructure.DMA_DIR                = DMA_DIR_PeripheralDST;
	dmaStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
	dmaStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;

	DMA_Init(ser[sn].dma_tx, &dmaStructure);
	DMA_Cmd(ser[sn].dma_tx, ENABLE);
}

//attende che dma1 canale 5 sia libero
void usartx_dmaWait(u8 sn)
{
	while(USART_GetFlagStatus(ser[sn].usartN,USART_FLAG_TXE) == RESET);
	if(sn==SERIAL1)
		while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_BSY) == SET);				//seriale condivisa con spi
}

//////////////////////////////////////////////////////////
//////////////////////// IRQ /////////////////////////////
//////////////////////////////////////////////////////////
void USART1_IRQHandler(void) 
{
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		ser[SERIAL1].rx_buf[ser[SERIAL1].pos]=USART1->DR;
		if(ser[SERIAL1].pos<RX_BUFSIZE)
		{
			ser[SERIAL1].pos++;
			usart1_process_data();
		}
		else
			ser[SERIAL1].pos=0;		//buffer overflow
	}
}

void USART2_IRQHandler(void) 
{
	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		ser[SERIAL2].rx_buf[ser[SERIAL2].pos]=USART2->DR;
		if(ser[SERIAL2].pos<RX_BUFSIZE)
		{
			ser[SERIAL2].pos++;
			usart2_process_data();
		}
		else
			ser[SERIAL2].pos=0;		//buffer overflow
	}
}

void USART3_IRQHandler(void) 
{
	if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
	{
		ser[SERIAL3].rx_buf[ser[SERIAL3].pos]=USART3->DR;
		if(ser[SERIAL3].pos<RX_BUFSIZE)
		{
			ser[SERIAL3].pos++;
			usart3_process_data();
		}
		else
			ser[SERIAL3].pos=0;		//buffer overflow
		
	}
}

///////////////////////////////////
//ricezione dal pc con protocolo ft950 //
///////////////////////////////////
void usart1_process_data(void)
{
	u8 i,pos=0;
	u32 f;
	
	
	//cerca terminazione nel buffer
	for(pos=0;pos<RX_BUFSIZE && pos<ser[SERIAL1].pos && ser[SERIAL1].rx_buf[pos]!=';';pos++);
	
	//nessun dato utile, esco e attendo altri caratteri
	if(ser[SERIAL1].rx_buf[pos] != ';' )
		return;
	
	//buffer pieno, nessun dato utile, azzero buffer
	if(pos==RX_BUFSIZE)
	{
		ser[SERIAL1].pos=0;
		return;
	}
	
	lcd_Debug(ser[SERIAL1].rx_buf,ser[SERIAL1].pos);
	
	//processo contenuto del buffer
	f=enav_status.frequenza_rtx;
	if (f>100000000)							// l'FT950 ha una cifra in meno
				f=0;
	
	//AT; sconosciuto
	if(ser[SERIAL1].rx_buf[0]=='A' && ser[SERIAL1].rx_buf[1]=='T')
	{
		memcpy(ser[SERIAL1].tx_buf, & "AT0;",4);
		for(i=0;i<4;i++)
		{
			USART1->DR=ser[SERIAL1].tx_buf[i];
			while((USART1->SR & USART_SR_TXE) == 0);
		}
	}
	
	//PS; stato alimentazone
	if(ser[SERIAL1].rx_buf[0]=='P' && ser[SERIAL1].rx_buf[1]=='S')
	{
		memcpy(ser[SERIAL1].tx_buf, & "PS1;",4);
		for(i=0;i<4;i++)
		{
			USART1->DR=ser[SERIAL1].tx_buf[i];
			while((USART1->SR & USART_SR_TXE) == 0);
		}
	}
	
	if(ser[SERIAL1].rx_buf[0]=='F' && ser[SERIAL1].rx_buf[1]=='S')
	{
		memcpy(ser[SERIAL1].tx_buf, & "FS0;",4);
		for(i=0;i<4;i++)
		{
			USART1->DR=ser[SERIAL1].tx_buf[i];
			while((USART1->SR & USART_SR_TXE) == 0);
		}
	}
		
	if(ser[SERIAL1].rx_buf[0]=='B' && ser[SERIAL1].rx_buf[1]=='Y')
	{
		memcpy(ser[SERIAL1].tx_buf, & "BY00;",5);
		for(i=0;i<5;i++)
		{
			USART1->DR=ser[SERIAL1].tx_buf[i];
			while((USART1->SR & USART_SR_TXE) == 0);
		}
	}
		
	if(ser[SERIAL1].rx_buf[0]=='U' && ser[SERIAL1].rx_buf[1]=='L')
	{
		memcpy(ser[SERIAL1].tx_buf, & "UL0;",4);
		for(i=0;i<4;i++)
		{
			USART1->DR=ser[SERIAL1].tx_buf[i];
			while((USART1->SR & USART_SR_TXE) == 0);
		}
	}
	
	if(ser[SERIAL1].rx_buf[0]=='R' && ser[SERIAL1].rx_buf[1]=='I')
	{
		if(ser[SERIAL1].rx_buf[2]=='0')
			memcpy(ser[SERIAL1].tx_buf, & "UL00;",5);
		
		if(ser[SERIAL1].rx_buf[2]=='1')
			memcpy(ser[SERIAL1].tx_buf, & "UL10;",5);

		if(ser[SERIAL1].rx_buf[2]=='3')
			memcpy(ser[SERIAL1].tx_buf, & "UL30;",5);
		
		if(ser[SERIAL1].rx_buf[2]=='4')
			memcpy(ser[SERIAL1].tx_buf, & "UL40;",5);
		
		for(i=0;i<5;i++)
		{
			USART1->DR=ser[SERIAL1].tx_buf[i];
			while((USART1->SR & USART_SR_TXE) == 0);
		}
	}

	
	if(ser[SERIAL1].rx_buf[0]=='R' && ser[SERIAL1].rx_buf[1]=='S')
	{
		memcpy(ser[SERIAL1].tx_buf, & "RS0;",4);
		for(i=0;i<4;i++)
		{
			USART1->DR=ser[SERIAL1].tx_buf[i];
			while((USART1->SR & USART_SR_TXE) == 0);
		}
}
		
	if(ser[SERIAL1].rx_buf[0]=='A' && ser[SERIAL1].rx_buf[1]=='N')
	{
		memcpy(ser[SERIAL1].tx_buf, & "AN0110;",7);
		for(i=0;i<7;i++)
		{
			USART1->DR=ser[SERIAL1].tx_buf[i];
			while((USART1->SR & USART_SR_TXE) == 0);
		}
	}

	if(ser[SERIAL1].rx_buf[0]=='R' && ser[SERIAL1].rx_buf[1]=='A')
	{
		memcpy(ser[SERIAL1].tx_buf, & "RA00;",5);
		for(i=0;i<5;i++)
		{
			USART1->DR=ser[SERIAL1].tx_buf[i];
			while((USART1->SR & USART_SR_TXE) == 0);
		}
	}
	
	//IF: informazioni
	if(ser[SERIAL1].rx_buf[0]=='I' && ser[SERIAL1].rx_buf[1]=='F')
	{
		pos=0;
		ser[SERIAL1].tx_buf[pos++]='I';
		ser[SERIAL1].tx_buf[pos++]='F';
		ser[SERIAL1].tx_buf[pos++]='0';					//P1:3 memoria
		ser[SERIAL1].tx_buf[pos++]='0';
		ser[SERIAL1].tx_buf[pos++]='1';
		ser[SERIAL1].tx_buf[pos++]=0X30+((f%100000000)/10000000);	//P2:8 frequenza formato ft950
		ser[SERIAL1].tx_buf[pos++]=0X30+((f%10000000) /1000000);
		ser[SERIAL1].tx_buf[pos++]=0X30+((f%1000000)  /100000);
		ser[SERIAL1].tx_buf[pos++]=0X30+((f%100000)   /10000);
		ser[SERIAL1].tx_buf[pos++]=0X30+((f%10000)   /1000);
		ser[SERIAL1].tx_buf[pos++]=0X30+((f%1000)  /100);
		ser[SERIAL1].tx_buf[pos++]=0X30+((f%100)  /10);
		ser[SERIAL1].tx_buf[pos++]=0X30+(f%10);
		ser[SERIAL1].tx_buf[pos++]='+';					//P3:5 clari offset
		ser[SERIAL1].tx_buf[pos++]='0';
		ser[SERIAL1].tx_buf[pos++]='0';
		ser[SERIAL1].tx_buf[pos++]='0';
		ser[SERIAL1].tx_buf[pos++]='0';
		ser[SERIAL1].tx_buf[pos++]='0';					//P4:1 rx clari on/off
		ser[SERIAL1].tx_buf[pos++]='0';					//P5:1 tx clari on/off
		ser[SERIAL1].tx_buf[pos++]='9';					//P6:1 modo
		ser[SERIAL1].tx_buf[pos++]='0';					//P7:1 vfo/mem
		ser[SERIAL1].tx_buf[pos++]='0';					//P8:1 ctcff
		ser[SERIAL1].tx_buf[pos++]='4';					//P9:2 tone
		ser[SERIAL1].tx_buf[pos++]='9';
		ser[SERIAL1].tx_buf[pos++]='0';					//P10:1 che cacchio e'? simplex/plus/minu shift
		ser[SERIAL1].tx_buf[pos++]=';';	
		
		for(i=0;i<pos;i++)
		{
			USART1->DR=ser[SERIAL1].tx_buf[i];
			while((USART1->SR & USART_SR_TXE) == 0);
		}
	}
	



	//frequenza VFO A
	if(ser[SERIAL1].rx_buf[0]=='F' && ser[SERIAL1].rx_buf[1]=='A')
	{	
		//richiesta frequenza dall'rtx
		if(ser[SERIAL1].rx_buf[2]==';')
		{
			pos=0;
			ser[SERIAL1].tx_buf[pos++]='F';
			ser[SERIAL1].tx_buf[pos++]='A';
			ser[SERIAL1].tx_buf[pos++]=0X30+((f%100000000)/10000000);	//P2:8 frequenza formato ft950
			ser[SERIAL1].tx_buf[pos++]=0X30+((f%10000000) /1000000);
			ser[SERIAL1].tx_buf[pos++]=0X30+((f%1000000)  /100000);
			ser[SERIAL1].tx_buf[pos++]=0X30+((f%100000)   /10000);
			ser[SERIAL1].tx_buf[pos++]=0X30+((f%10000)   /1000);
			ser[SERIAL1].tx_buf[pos++]=0X30+((f%1000)  /100);
			ser[SERIAL1].tx_buf[pos++]=0X30+((f%100)  /10);
			ser[SERIAL1].tx_buf[pos++]=0X30+(f%10);
			ser[SERIAL1].tx_buf[pos++]=';';
			
			//impostafrequenza sull'FT991
			for(i=0;i<pos;i++)
			{
				USART1->DR=ser[SERIAL1].tx_buf[i];
				while((USART1->SR & USART_SR_TXE) == 0);
			}
		}
		else
		//il pc imposta la frequenza
		{
			enav_status.frequenza_rtx=0;
			ser[SERIAL1].evt=1;								//cambio di stato
			
			//prepara buffer per l'rtx
			ser[SERIAL2].tx_buf[0]='F';
			ser[SERIAL2].tx_buf[1]='A';
			ser[SERIAL2].tx_buf[2]='0';						//il FT991 ha una cifra in piu'

			//riceve la frequenza dal programma e aggiorna locale
			for(i=2;i<10;i++)
			{
				enav_status.frequenza_rtx*=10;
				enav_status.frequenza_rtx+=ser[SERIAL1].rx_buf[i]-0x30;
				ser[SERIAL2].tx_buf[i+1]=ser[SERIAL1].rx_buf[i];
			}
			//impostafrequenza sull'FT991
			usartx_dmaSend8(SERIAL2, ser[SERIAL2].tx_buf,11);		
			
		}
	}//end VFO A
	
	//frequenza VFO B
	if(ser[SERIAL1].rx_buf[0]=='F' && ser[SERIAL1].rx_buf[1]=='B')
	{	
		//richiesta frequenza dall'rtx
		if(ser[SERIAL1].rx_buf[2]==';')
		{
			pos=0;
			ser[SERIAL1].tx_buf[pos++]='F';
			ser[SERIAL1].tx_buf[pos++]='B';
			ser[SERIAL1].tx_buf[pos++]=0X30+((f%100000000)/10000000);	//P2:8 frequenza formato ft950
			ser[SERIAL1].tx_buf[pos++]=0X30+((f%10000000) /1000000);
			ser[SERIAL1].tx_buf[pos++]=0X30+((f%1000000)  /100000);
			ser[SERIAL1].tx_buf[pos++]=0X30+((f%100000)   /10000);
			ser[SERIAL1].tx_buf[pos++]=0X30+((f%10000)   /1000);
			ser[SERIAL1].tx_buf[pos++]=0X30+((f%1000)  /100);
			ser[SERIAL1].tx_buf[pos++]=0X30+((f%100)  /10);
			ser[SERIAL1].tx_buf[pos++]=0X30+(f%10);
			ser[SERIAL1].tx_buf[pos++]=';';
			
			//impostafrequenza sull'FT991
			for(i=0;i<pos;i++)
			{
				USART1->DR=ser[SERIAL1].tx_buf[i];
				while((USART1->SR & USART_SR_TXE) == 0);
			}
		}
		else
		//il pc imposta la frequenza
		{
			enav_status.frequenza_rtx=0;
			ser[SERIAL1].evt=1;								//cambio di stato
			
			//prepara buffer per l'rtx
			ser[SERIAL2].tx_buf[0]='F';
			ser[SERIAL2].tx_buf[1]='B';
			ser[SERIAL2].tx_buf[2]='0';						//il FT991 ha una cifra in piu'

			//riceve la frequenza dal programma e aggiorna locale
			for(i=2;i<10;i++)
			{
				enav_status.frequenza_rtx*=10;
				enav_status.frequenza_rtx+=ser[SERIAL1].rx_buf[i]-0x30;
				ser[SERIAL2].tx_buf[i+1]=ser[SERIAL1].rx_buf[i];
			}
			//impostafrequenza sull'FT991
			usartx_dmaSend8(SERIAL2, ser[SERIAL2].tx_buf,11);		
			
		}
	}//end VFO B
	
	memset (ser[SERIAL1].rx_buf, 0,RX_BUFSIZE);
	ser[SERIAL1].pos=0;	
	
}

/////////////////////////////////////
//ricezione dall'rtx con protocolo FT991 //
/////////////////////////////////////
void usart2_process_data()
{
	u8 i,pos=0;
	
	//cerca terminazione nel buffer
	while(pos<RX_BUFSIZE && pos<ser[SERIAL2].pos)
	{
		if(ser[SERIAL2].rx_buf[pos]==';') break;
		pos++;
	}
	
	//nessun dato utile, esco e attendo altri caratteri
	if(pos==ser[SERIAL2].pos)
		return;
	
	//buffer pieno, nessun dato utile, azzero buffer
	if(pos==RX_BUFSIZE)
	{
		ser[SERIAL2].pos=0;
		return;
	}

	enav_status.poweron=1;
	if(ser[SERIAL2].rx_buf[0]=='F' && ser[SERIAL2].rx_buf[1]=='A')
	{	
		enav_status.frequenza_rtx=0;
		for(i=2;i<11;i++)
		{
			enav_status.frequenza_rtx*=10;
			enav_status.frequenza_rtx+=ser[SERIAL2].rx_buf[i]-0x30;
		}
	
		ser[SERIAL2].evt=1;
	}
	
		ser[SERIAL2].oldpos=0;
		ser[SERIAL2].pos=0;	
	
	//usartx_dmaSend8(sn,data,len);
	//usartx_dmaWait(sn);
}


//nextion
void usart3_process_data(void)
{

/*
	if(data[0]=='F' && data[1]=='A')
	{	
		enav_status.frequenza_rtx=0;
		for(i=2;i<len;i++)
		{
			enav_status.frequenza_rtx*=10;
			enav_status.frequenza_rtx+=data[i]-0x30;
		}
	
		ser[sn].evt=1;
	}
	
		ser[sn].oldpos=0;
		ser[sn].pos=0;	
	*/
	//usartx_dmaSend8(sn,data,len);
	//usartx_dmaWait(sn);
}

