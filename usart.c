#define USART1_USE_DMA 0

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "usart.h"
#include "lcd_core.h"
#include "atrcc.h"
#include "rtx.h"
#include "main.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_spi.h"
#include "stm32f10x.h"

struct _usart_data ser[3];	//variabili usart
extern struct _rtx_status rtx;

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

	//about dma rx issue!!
	//when fulltransfer and halftrasnfer arent'reached, the dma doesn't trigger any interrupt 
	//the only mode to know if there are data on rx buffer is by polling the register
	//but dma work based on a halfword access on bus; this mean that CNDTR advance only when
	//two bytes are received, but the rtx command lenght vary on a byte size and an odd bytes
	//doesn't increase the CNDTR register so polling doesn't work as espected.
	//This is why i used irq for receiving from the usarts.
	
	//dma per trasmissione
	//USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);		//in conflitto con spi
	USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
	USART_DMACmd(USART3, USART_DMAReq_Tx, ENABLE);
}

//about dma tx issue on usart1!!
//DMA1 channel 5 is shared between spi2 and usart1, spi2 connect the lcd, ethernet and touch sensor.
//Even if i deinit dma, disable dma on spi2 and enable on usart1, while sending to usart1, only the first character is sent;
//But if in the same way i de/init dma, disable dma on usart1 and enable on spi2 there are no problems at all with spi2!!!
//Please help!!!

//invia dati alla porta prescelta
void usartx_dmaSend8(u8 sn,u8 *data, u32 n)
{
	DMA_InitTypeDef dmaStructure;
	
	DMA_Cmd(ser[sn].dma_tx, DISABLE);
	/*																		//disabled because usart send only the first byte
	if(sn==SERIAL1)
		SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, DISABLE);
		USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
	*/
	DMA_StructInit(&dmaStructure);
	dmaStructure.DMA_M2M						= DMA_M2M_Disable;
	dmaStructure.DMA_MemoryBaseAddr		= (u32) data;

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

//attende che dma1 canale sn sia libero
void usartx_dmaWait(u8 sn)
{
	while(USART_GetFlagStatus(ser[sn].usartN,USART_FLAG_TXE) == RESET);		//wait for transmission completion on usart1
	if(sn==SERIAL1)																			//if usart1, dma shared with spi2
		while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_BSY) == SET);				//wait for transmission completion on spi2
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
	
	ser[SERIAL1].evt=10;								//attivita' lato pc
	//lcd_pcDebug(ser[SERIAL1].rx_buf,ser[SERIAL1].pos);
	
	//AT; sconosciuto
	if(ser[SERIAL1].rx_buf[0]=='A' && ser[SERIAL1].rx_buf[1]=='T')
	{
		/*
		//risponde AT0;
		memcpy(ser[SERIAL1].tx_buf, & "AT0;",4);
		for(i=0;i<4;i++)
		{
			USART1->DR=ser[SERIAL1].tx_buf[i];
			while((USART1->SR & USART_SR_TXE) == 0);
		}
		*/
		
		usartx_dmaSend8(SERIAL2, ser[SERIAL1].rx_buf,3);
	}
	//PR; diventa PR0
	else if(ser[SERIAL1].rx_buf[0]=='P' && ser[SERIAL1].rx_buf[1]=='R')
	{
		//prepara risposta per il pc
		ser[SERIAL1].tx_buf[0]='P';
		ser[SERIAL1].tx_buf[1]='R';
		ser[SERIAL1].tx_buf[2]='0';	
		ser[SERIAL1].tx_buf[3]=';';	
		
		for(i=0;i<4;i++)
		{
			USART1->DR=ser[SERIAL1].tx_buf[i];
			while((USART1->SR & USART_SR_TXE) == 0);
		}
	}
	//FR; risponde FR0
	else if(ser[SERIAL1].rx_buf[0]=='F' && ser[SERIAL1].rx_buf[1]=='R')
	{
		//prepara risposta per il pc
		ser[SERIAL1].tx_buf[0]='F';
		ser[SERIAL1].tx_buf[1]='R';
		ser[SERIAL1].tx_buf[2]='0';	
		ser[SERIAL1].tx_buf[3]=';';	
		
		for(i=0;i<4;i++)
		{
			USART1->DR=ser[SERIAL1].tx_buf[i];
			while((USART1->SR & USART_SR_TXE) == 0);
		}
	}
	//MK; risponde MK2
	else if(ser[SERIAL1].rx_buf[0]=='M' && ser[SERIAL1].rx_buf[1]=='K')
	{
		//prepara risposta per il pc
		ser[SERIAL1].tx_buf[0]='M';
		ser[SERIAL1].tx_buf[1]='K';
		ser[SERIAL1].tx_buf[2]='2';	
		ser[SERIAL1].tx_buf[3]=';';	
		
		for(i=0;i<4;i++)
		{
			USART1->DR=ser[SERIAL1].tx_buf[i];
			while((USART1->SR & USART_SR_TXE) == 0);
		}
	}
	//PPR; diventa PPR0
	else if(ser[SERIAL1].rx_buf[0]=='P' && ser[SERIAL1].rx_buf[1]=='P' && ser[SERIAL1].rx_buf[2]=='R')
	{
		//prepara risposta per il pc
		ser[SERIAL1].tx_buf[0]='P';
		ser[SERIAL1].tx_buf[1]='P';
		ser[SERIAL1].tx_buf[2]='R';
		ser[SERIAL1].tx_buf[3]='0';	
		ser[SERIAL1].tx_buf[4]=';';	
		
		for(i=0;i<5;i++)
		{
			USART1->DR=ser[SERIAL1].tx_buf[i];
			while((USART1->SR & USART_SR_TXE) == 0);
		}
	}
	//RI1; diventa RI11
	else if(ser[SERIAL1].rx_buf[0]=='R' && ser[SERIAL1].rx_buf[1]=='I' && ser[SERIAL1].rx_buf[2]=='1')
	{
		//prepara risposta per il pc
		ser[SERIAL1].tx_buf[0]='R';
		ser[SERIAL1].tx_buf[1]='I';
		ser[SERIAL1].tx_buf[2]='1';
		ser[SERIAL1].tx_buf[3]='1';	
		ser[SERIAL1].tx_buf[4]=';';	
		
		for(i=0;i<5;i++)
		{
			USART1->DR=ser[SERIAL1].tx_buf[i];
			while((USART1->SR & USART_SR_TXE) == 0);
		}
	}
	//CN; diventa CN00
	else if(ser[SERIAL1].rx_buf[0]=='C' && ser[SERIAL1].rx_buf[1]=='N')
	{
		//prepara risposta per il pc
		ser[SERIAL1].tx_buf[0]='C';
		ser[SERIAL1].tx_buf[1]='N';
		ser[SERIAL1].tx_buf[2]='0';
		ser[SERIAL1].tx_buf[3]='0';	
		ser[SERIAL1].tx_buf[4]='0';	
		ser[SERIAL1].tx_buf[5]=';';	
		
		for(i=0;i<6;i++)
		{
			USART1->DR=ser[SERIAL1].tx_buf[i];
			while((USART1->SR & USART_SR_TXE) == 0);
		}
	}

	//CO;
	else if(ser[SERIAL1].rx_buf[0]=='C' && ser[SERIAL1].rx_buf[1]=='O')
	{
		//prepara risposta per il pc
		ser[SERIAL1].tx_buf[0]='C';
		ser[SERIAL1].tx_buf[1]='O';
		ser[SERIAL1].tx_buf[2]='0';
		ser[SERIAL1].tx_buf[3]=ser[SERIAL1].rx_buf[3];
		if(ser[SERIAL1].tx_buf[3]-0x30 == 0)
		{
			ser[SERIAL1].tx_buf[4]='0';
			ser[SERIAL1].tx_buf[5]='0';
		}
		else
		{
			ser[SERIAL1].tx_buf[4]='3';	
			ser[SERIAL1].tx_buf[5]='0';	
		}
		ser[SERIAL1].tx_buf[6]=';';	
		
		for(i=0;i<7;i++)
		{
			USART1->DR=ser[SERIAL1].tx_buf[i];
			while((USART1->SR & USART_SR_TXE) == 0);
		}
	}
	//AAC; risponde AAC0
	else if(ser[SERIAL1].rx_buf[0]=='A' && ser[SERIAL1].rx_buf[1]=='A' && ser[SERIAL1].rx_buf[2]=='C')
	{
		//prepara risposta per il pc
		ser[SERIAL1].tx_buf[0]='A';
		ser[SERIAL1].tx_buf[1]='A';
		ser[SERIAL1].tx_buf[2]='C';
		ser[SERIAL1].tx_buf[3]='0';	
		ser[SERIAL1].tx_buf[4]=';';	
		
		for(i=0;i<5;i++)
		{
			USART1->DR=ser[SERIAL1].tx_buf[i];
			while((USART1->SR & USART_SR_TXE) == 0);
		}
	}
	//AN; risponde AN000
	else if(ser[SERIAL1].rx_buf[0]=='A' && ser[SERIAL1].rx_buf[1]=='N' && ser[SERIAL1].rx_buf[2]=='0')
	{
		//prepara risposta per il pc
		ser[SERIAL1].tx_buf[0]='A';
		ser[SERIAL1].tx_buf[1]='N';
		ser[SERIAL1].tx_buf[2]='0';	
		ser[SERIAL1].tx_buf[3]='0';
		ser[SERIAL1].tx_buf[4]='0';
		ser[SERIAL1].tx_buf[5]=';';
		
		for(i=0;i<6;i++)
		{
			USART1->DR=ser[SERIAL1].tx_buf[i];
			while((USART1->SR & USART_SR_TXE) == 0);
		}
	}
	else if(ser[SERIAL1].rx_buf[0]=='F' && ser[SERIAL1].rx_buf[1]=='A')
	//frequenza VFO A
	{	
		//richiesta frequenza dall'rtx
		if(ser[SERIAL1].rx_buf[2]==';')
		{
			memcpy(ser[SERIAL2].tx_buf,ser[SERIAL1].rx_buf,3);
			usartx_dmaSend8(SERIAL2, ser[SERIAL2].tx_buf,3);
		}
		else
		//il pc imposta la frequenza
		{
			rtx.vfo_A=0;
			
			//prepara buffer per l'rtx
			ser[SERIAL2].tx_buf[0]='F';
			ser[SERIAL2].tx_buf[1]='A';
			ser[SERIAL2].tx_buf[2]='0';						//il FT991 ha una cifra in piu'

			//riceve la frequenza dal programma e aggiorna locale
			for(i=2;i<12;i++)
			{
				rtx.vfo_A+=ser[SERIAL1].rx_buf[i]-0x30;
				ser[SERIAL2].tx_buf[i+1]=ser[SERIAL1].rx_buf[i];
			}
			//impostafrequenza sull'FT991
			usartx_dmaSend8(SERIAL2, ser[SERIAL2].tx_buf,12);		
		}		
	}//end VFO A
	else if(ser[SERIAL1].rx_buf[0]=='F' && ser[SERIAL1].rx_buf[1]=='B')
	//frequenza VFO B
	{	
		//richiesta frequenza dall'rtx
		if(ser[SERIAL1].rx_buf[2]==';')
		{
			memcpy(ser[SERIAL2].tx_buf,ser[SERIAL1].rx_buf,3);
			usartx_dmaSend8(SERIAL2, ser[SERIAL2].tx_buf,3);
		}
		else
		//il pc imposta la frequenza
		{
			rtx.vfo_B=0;
			
			//prepara buffer per l'rtx
			ser[SERIAL2].tx_buf[0]='F';
			ser[SERIAL2].tx_buf[1]='B';
			ser[SERIAL2].tx_buf[2]='0';						//il FT991 ha una cifra in piu'

			//riceve la frequenza dal programma e aggiorna locale
			for(i=2;i<12;i++)
			{
				rtx.vfo_B+=ser[SERIAL1].rx_buf[i]-0x30;
				ser[SERIAL2].tx_buf[i+1]=ser[SERIAL1].rx_buf[i];
			}
			//impostafrequenza sull'FT991
			usartx_dmaSend8(SERIAL2, ser[SERIAL2].tx_buf,12);		
		}		
	}//end VFO A
	else	if(ser[SERIAL1].rx_buf[0]=='F' && ser[SERIAL1].rx_buf[1]=='B')	//frequenza VFO B
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
			rtx.vfo_A=0;
			
			//prepara buffer per l'rtx
			ser[SERIAL2].tx_buf[0]='F';
			ser[SERIAL2].tx_buf[1]='B';
			ser[SERIAL2].tx_buf[2]='0';						//il FT991 ha una cifra in piu'

			//riceve la frequenza dal programma e aggiorna locale
			for(i=2;i<10;i++)
			{
				rtx.vfo_A*=10;
				rtx.vfo_A+=ser[SERIAL1].rx_buf[i]-0x30;
				ser[SERIAL2].tx_buf[i+1]=ser[SERIAL1].rx_buf[i];
			}
			//impostafrequenza sull'FT991
			usartx_dmaSend8(SERIAL2, ser[SERIAL2].tx_buf,11);
			
		}
	}//end VFO B
	else
	{
		for(i=0;ser[SERIAL1].rx_buf[i]!=';' && i < TX_BUFSIZE;i++);	//cerca teminazione del comando
		if (i<TX_BUFSIZE)	//se non trovata non e' un comando valido
		{
			memcpy(ser[SERIAL2].tx_buf,ser[SERIAL1].rx_buf,i+1);
			usartx_dmaSend8(SERIAL2, ser[SERIAL2].tx_buf,i+1);		//reinvia il comando all'ft991
		}
	}
	
	//pulisce buffer rx
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

	//interpreta comandi ricevuti dall'rtx
	rtx_ft991a_decode(ser[SERIAL2].rx_buf,ser[SERIAL2].pos);
	
	//debug a schermo
	//lcd_rtxDebug(ser[SERIAL2].rx_buf, ser[SERIAL2].pos<20 ? ser[SERIAL2].pos : 20);

	
	
	//se era presente una trasmissione recente su usart1 li reinvia sulla seriale 1 verso pc col protocollo FT950
	if(ser[SERIAL1].evt)
	{
		if(ser[SERIAL2].rx_buf[0]=='R' && ser[SERIAL2].rx_buf[1]=='A')
		{
			memcpy(ser[SERIAL1].tx_buf, & "RA00;",5);
			for(i=0;i<5;i++)
			{
				USART1->DR=ser[SERIAL1].tx_buf[i];
				while((USART1->SR & USART_SR_TXE) == 0);
			}
		}
		
		//IF: informazioni
		if(ser[SERIAL2].rx_buf[0]=='I' && ser[SERIAL2].rx_buf[1]=='F')
		{
			pos=0;
			ser[SERIAL1].tx_buf[0]=ser[SERIAL2].rx_buf[0];
			ser[SERIAL1].tx_buf[1]=ser[SERIAL2].rx_buf[1];
			ser[SERIAL1].tx_buf[2]=ser[SERIAL2].rx_buf[2];	//P1:3 memoria
			ser[SERIAL1].tx_buf[3]=ser[SERIAL2].rx_buf[3];
			ser[SERIAL1].tx_buf[4]=ser[SERIAL2].rx_buf[4];
			//salta la cifra delle centinaia di mhz
			ser[SERIAL1].tx_buf[5]=ser[SERIAL2].rx_buf[6];	//P2:8 frequenza formato ft950
			ser[SERIAL1].tx_buf[6]=ser[SERIAL2].rx_buf[7];
			ser[SERIAL1].tx_buf[7]=ser[SERIAL2].rx_buf[8];
			ser[SERIAL1].tx_buf[8]=ser[SERIAL2].rx_buf[9];
			ser[SERIAL1].tx_buf[9]=ser[SERIAL2].rx_buf[10];
			ser[SERIAL1].tx_buf[10]=ser[SERIAL2].rx_buf[11];
			ser[SERIAL1].tx_buf[11]=ser[SERIAL2].rx_buf[12];
			ser[SERIAL1].tx_buf[12]=ser[SERIAL2].rx_buf[13];
			ser[SERIAL1].tx_buf[13]=ser[SERIAL2].rx_buf[14];//P3:5 clari offset
			ser[SERIAL1].tx_buf[14]=ser[SERIAL2].rx_buf[15];
			ser[SERIAL1].tx_buf[15]=ser[SERIAL2].rx_buf[16];
			ser[SERIAL1].tx_buf[16]=ser[SERIAL2].rx_buf[17];
			ser[SERIAL1].tx_buf[17]=ser[SERIAL2].rx_buf[18];
			ser[SERIAL1].tx_buf[18]=ser[SERIAL2].rx_buf[19];//P4:1 rx clari on/off
			ser[SERIAL1].tx_buf[19]=ser[SERIAL2].rx_buf[20];//P5:1 tx clari on/off
			ser[SERIAL1].tx_buf[20]=ser[SERIAL2].rx_buf[21];//P6:1 modo
			ser[SERIAL1].tx_buf[21]=ser[SERIAL2].rx_buf[22];//P7:1 vfo/mem
			ser[SERIAL1].tx_buf[22]=ser[SERIAL2].rx_buf[23];//P8:1 ctcff
			ser[SERIAL1].tx_buf[23]=ser[SERIAL2].rx_buf[24];//P9:2 tone
			ser[SERIAL1].tx_buf[24]=ser[SERIAL2].rx_buf[25];
			ser[SERIAL1].tx_buf[25]=ser[SERIAL2].rx_buf[26];//P10:1 che cacchio e'? simplex/plus/minu shift
			ser[SERIAL1].tx_buf[26]=';';	
			
			for(i=0;i<27;i++)
			{
				USART1->DR=ser[SERIAL1].tx_buf[i];
				while((USART1->SR & USART_SR_TXE) == 0);
			}
		}
		else if(ser[SERIAL2].rx_buf[0]=='F' && ser[SERIAL2].rx_buf[1]=='A')
		{
			ser[SERIAL1].tx_buf[0]='F';
			ser[SERIAL1].tx_buf[1]='A';
			ser[SERIAL1].tx_buf[2]=0X30+((rtx.vfo_A%100000000)/10000000);	//P2:8 frequenza formato ft950
			ser[SERIAL1].tx_buf[3]=0X30+((rtx.vfo_A%10000000) /1000000);
			ser[SERIAL1].tx_buf[4]=0X30+((rtx.vfo_A%1000000)  /100000);
			ser[SERIAL1].tx_buf[5]=0X30+((rtx.vfo_A%100000)   /10000);
			ser[SERIAL1].tx_buf[6]=0X30+((rtx.vfo_A%10000)   /1000);
			ser[SERIAL1].tx_buf[7]=0X30+((rtx.vfo_A%1000)  /100);
			ser[SERIAL1].tx_buf[8]=0X30+((rtx.vfo_A%100)  /10);
			ser[SERIAL1].tx_buf[9]=0X30+(rtx.vfo_A%10);
			ser[SERIAL1].tx_buf[10]=';';
				
			//impostafrequenza sull'FT991
			for(i=0;i<11;i++)
			{
				USART1->DR=ser[SERIAL1].tx_buf[i];
				while((USART1->SR & USART_SR_TXE) == 0);
			}
		}
		else if(ser[SERIAL2].rx_buf[0]=='F' && ser[SERIAL2].rx_buf[1]=='B')
		{
			ser[SERIAL1].tx_buf[0]='F';
			ser[SERIAL1].tx_buf[1]='B';
			ser[SERIAL1].tx_buf[3]=0X30+((rtx.vfo_B%10000000) /1000000);	//P2:8 frequenza formato ft950
			ser[SERIAL1].tx_buf[4]=0X30+((rtx.vfo_B%1000000)  /100000);
			ser[SERIAL1].tx_buf[5]=0X30+((rtx.vfo_B%100000)   /10000);
			ser[SERIAL1].tx_buf[6]=0X30+((rtx.vfo_B%10000)   /1000);
			ser[SERIAL1].tx_buf[7]=0X30+((rtx.vfo_B%1000)  /100);
			ser[SERIAL1].tx_buf[8]=0X30+((rtx.vfo_B%100)  /10);
			ser[SERIAL1].tx_buf[9]=0X30+(rtx.vfo_B%10);
			ser[SERIAL1].tx_buf[10]=';';
				
			//invia frequenza al pc
			for(i=0;i<pos;i++)
			{
				USART1->DR=ser[SERIAL1].tx_buf[i];
				while((USART1->SR & USART_SR_TXE) == 0);
			}
		}
		else	//se nessuno dei filtri copia il pacchetto dell'rtx.
		{
			for(i=0;ser[SERIAL2].rx_buf[i] != ';' && i<TX_BUFSIZE;i++)
			{
				USART1->DR=ser[SERIAL2].rx_buf[i];
				while((USART1->SR & USART_SR_TXE) == 0);		
			}
			USART1->DR=ser[SERIAL2].rx_buf[i];				//invia il ';' mancante
			while((USART1->SR & USART_SR_TXE) == 0);		
		}
	}
	
	ser[SERIAL2].oldpos=0;
	ser[SERIAL2].pos=0;	
	
	//usartx_dmaSend8(sn,data,len);
	//usartx_dmaWait(sn);
}


//nextion
void usart3_process_data(void)
{
	
	ser[SERIAL2].oldpos=0;
	ser[SERIAL2].pos=0;	

}

