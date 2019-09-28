#include "dma.h"
#include "lcd_commands.h"
#include "lcd_core.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_gpio.h"

DMA_InitTypeDef dmaStructure;
u16 dma16BufIndex = 0;
u16 dma16Buffer[DMA16_BUF_SIZE];
u8 dma8BufIndex = 0;
u8 dma8Buffer[DMA8_BUF_SIZE];

u16 SPIReceivedValue[2];
u16 SPITransmittedValue[2] = {0xFF00,0x00FF};

void Spi2DmaInit(void)
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	// TX
	NVIC_EnableIRQ(DMA1_Channel5_IRQn);
	DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE);

	// RX
	NVIC_EnableIRQ(DMA1_Channel4_IRQn);
	DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);

	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Rx, ENABLE);
}


void dmaReceive8(u8 *data, u32 n)
{
	dmaWait();
	DMA_Cmd(DMA1_Channel4, DISABLE);
	
	USART_DMACmd(USART1, USART_DMAReq_Rx, DISABLE);			//dma sharet with usart1
	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Rx, ENABLE);

	dmaStructure.DMA_MemoryBaseAddr = (u32) data;
	dmaStructure.DMA_BufferSize     = n;

	dmaStructure.DMA_Mode               = DMA_Mode_Normal;
	dmaStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
	dmaStructure.DMA_DIR                = DMA_DIR_PeripheralSRC;
	dmaStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
	dmaStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;

	dmaStartRx();
	DMA_Cmd(DMA1_Channel4, ENABLE);
}

void dmaSend8(u8 *data, u32 n)
{
	dmaWait();
	DMA_Cmd(DMA1_Channel5, DISABLE);
	
	USART_DMACmd(USART1, USART_DMAReq_Tx, DISABLE);				//dma shared with usart1
	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
	
	
	DMA_StructInit(&dmaStructure);
	dmaStructure.DMA_PeripheralBaseAddr = (u32) &(SPI2->DR);
	dmaStructure.DMA_Priority           = DMA_Priority_Medium;

	dmaStructure.DMA_MemoryBaseAddr = (u32) data;
	dmaStructure.DMA_BufferSize     = n;

	dmaStructure.DMA_Mode               = DMA_Mode_Normal;
	dmaStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
	dmaStructure.DMA_DIR                = DMA_DIR_PeripheralDST;
	dmaStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
	dmaStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;

	dmaStartTx();
	DMA_Cmd(DMA1_Channel5, ENABLE);
}

void dmaSendCircular8(u16 *data, u32 n)
{
	dmaWait();
	DMA_Cmd(DMA1_Channel5, DISABLE);
	
	USART_DMACmd(USART1, USART_DMAReq_Tx, DISABLE);				//dma shared with usart1
	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
	
	DMA_StructInit(&dmaStructure);
	dmaStructure.DMA_PeripheralBaseAddr = (u32) &(SPI2->DR);
	dmaStructure.DMA_Priority           = DMA_Priority_Medium;

	dmaStructure.DMA_MemoryBaseAddr = (u32) data;
	dmaStructure.DMA_BufferSize     = n;

	dmaStructure.DMA_Mode               = DMA_Mode_Circular;
	dmaStructure.DMA_MemoryInc          = DMA_MemoryInc_Disable;
	dmaStructure.DMA_DIR                = DMA_DIR_PeripheralDST;
	dmaStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
	dmaStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;

	dmaStartTx();
	DMA_Cmd(DMA1_Channel5, ENABLE);
}

void dmaSendCircular16(u16 *data, u32 n)
{
	dmaWait();
	DMA_Cmd(DMA1_Channel5, DISABLE);
	
	USART_DMACmd(USART1, USART_DMAReq_Tx, DISABLE);					//dma shared with usart1
	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
	
	DMA_StructInit(&dmaStructure);
	dmaStructure.DMA_PeripheralBaseAddr = (u32) &(SPI2->DR);
	dmaStructure.DMA_Priority           = DMA_Priority_Medium;

	dmaStructure.DMA_MemoryBaseAddr = (u32) data;
	dmaStructure.DMA_BufferSize     = n;

	dmaStructure.DMA_Mode               = DMA_Mode_Circular;
	dmaStructure.DMA_MemoryInc          = DMA_MemoryInc_Disable;
	dmaStructure.DMA_DIR                = DMA_DIR_PeripheralDST;
	dmaStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord;
	dmaStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;

	dmaStartTx();
	DMA_Cmd(DMA1_Channel5, ENABLE);
}

void dmaSend16(u16 *data, u32 n)
{
	dmaWait();
	DMA_Cmd(DMA1_Channel5, DISABLE);

	USART_DMACmd(USART1, USART_DMAReq_Tx, DISABLE);						//dma shared with usart1
	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);

	DMA_StructInit(&dmaStructure);
	dmaStructure.DMA_PeripheralBaseAddr = (u32) &(SPI2->DR);
	dmaStructure.DMA_Priority           = DMA_Priority_Medium;

	dmaStructure.DMA_MemoryBaseAddr = (u32) data;
	dmaStructure.DMA_BufferSize     = n;

	dmaStructure.DMA_Mode               = DMA_Mode_Normal;
	dmaStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
	dmaStructure.DMA_DIR                = DMA_DIR_PeripheralDST;
	dmaStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord;
	dmaStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;

	dmaStartTx();
	DMA_Cmd(DMA1_Channel5, ENABLE);
}

void dmaSendCmd(u8 cmd)
{
	dmaWait();
	Lcd_CS_Reset();
	LCD_DC_RESET;
	dmaSend8(&cmd, 1);
	dmaWait();
	LCD_CS_SET;
}

void dmaSendCmdCont(u8 cmd)
{
	LCD_DC_RESET;
	dmaSend8(&cmd, 1);
}

void dmaReceiveDataCont8(u8 *data)
{
	u8 dummy = 0xFF;
	dmaSend8(&dummy, 1);
	dmaReceive8(data, 1);
}

void dmaSendData8(u8 *data, u32 n)
{
	dmaWait();
	Lcd_CS_Reset();
	LCD_DC_SET;
	dmaSend8(data, n);
	dmaWait();
	LCD_CS_SET;
}

void dmaSendDataCont8(u8 *data, u32 n)
{
	LCD_DC_SET;
	dmaSend8(data, n);
}

void dmaSendData16(u16 *data, u32 n)
{
	dmaWait();
	Lcd_CS_Reset();
	LCD_DC_SET;
	dmaSend16(data, n);
	dmaWait();
	LCD_CS_SET;
}

void dmaSendDataCont16(u16 *data, u32 n)
{
	LCD_DC_SET;
	dmaSend16(data, n);
}

void dmaSendDataBuf16(void)
{
   if(dma16BufIndex == 0) return;
	LCD_DC_SET;
   dmaSend16(dma16Buffer, dma16BufIndex);
   dma16BufIndex = 0;
}

void dmaSendDataContBuf16(u16 *data, u32 n)
{
	while (n--) {
		dma16Buffer[dma16BufIndex] = *data++;
		if (dma16BufIndex == DMA16_BUF_SIZE - 1)
		{
			dmaSendDataBuf16();
		}
		dma16BufIndex++;
	}
}

void dmaSendDataCircular8(u16 *data, u32 n)
{
	LCD_DC_SET;
	dmaSendCircular16(data, n);
}

void dmaSendDataCircular16(u16 *data, u32 n)
{
	LCD_DC_SET;
	dmaSendCircular16(data, n);
}

void dmaFill16(u16 color, u32 n)
{
	u16 i,ts;
	u8 r,g,b;
	
	r=(u8) ((color&0xf800)>>8);	//red
	g=(u8) ((color&0x7e0)>>3);
	b=(u8) ((color&0x1f)<<3);

	n=n*3;
	//for(i=0;i<(DMA16_BUF_SIZE-1);)
	for( i =0 ; i<( n < DMA16_BUF_SIZE ? n : DMA16_BUF_SIZE) ; )
	{
		dma8Buffer[i++]=b;
		dma8Buffer[i++]=g;
		dma8Buffer[i++]=r;
	}
		
	dmaWait();
	Lcd_CS_Reset();
	dmaSendCmdCont(LCD_GRAM);
	while (n != 0)
	{
		ts = (u16) (n > DMA8_BUF_SIZE ? DMA8_BUF_SIZE : n);
		dmaSendData8(dma8Buffer, ts);
		n -= ts;
	}
   dmaWait();
	LCD_CS_SET;
}

//"IRQ handlers"
//spi
void DMA1_Channel4_IRQHandler(void)
{
	if (DMA_GetITStatus(DMA1_IT_TC4) == SET)
	{
		DMA_Cmd(DMA1_Channel4, DISABLE);
		DMA_ClearITPendingBit(DMA1_IT_TC4);
	}
}

void DMA1_Channel5_IRQHandler(void)
{
	if (DMA_GetITStatus(DMA1_IT_TC5) == SET)
	{
		DMA_Cmd(DMA1_Channel5, DISABLE);
		DMA_ClearITPendingBit(DMA1_IT_TC5);
	}
}



//spi
void dmaWait(void)
{
	while(USART_GetFlagStatus(USART1,USART_FLAG_TXE) == RESET && USART1->CR1&USART_CR1_TE);	//trasmissione seriale completata
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_BSY) == SET);
}

