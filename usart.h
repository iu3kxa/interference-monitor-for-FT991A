#ifndef TEST1_USART_H
#define TEST1_USART_H

#define RX_BUFSIZE 64
#define TX_BUFSIZE 64

#define SERIAL1	0
#define SERIAL2	1
#define SERIAL3	2

#include "stm32f10x_usart.h"

struct _usart_data
{
	u8 rx_buf[RX_BUFSIZE];
	u8 tx_buf[TX_BUFSIZE];
	u32 pos;
	u32 oldpos;
	USART_TypeDef * usartN;
	DMA_Channel_TypeDef * dma_rx;
	DMA_Channel_TypeDef * dma_tx;
	bool evt;
};

void usartInit(void);
void usartx_rx_check(u8 serialnum);
void usartx_dmaSend8(u8 sn,u8 *data, u32 n);
void usartx_dmaWait(u8 sn);

#endif //TEST1_USART_H
