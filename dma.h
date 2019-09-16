#ifndef TEST1_DMA_H
#define TEST1_DMA_H

#define DMA16_BUF_SIZE_UNTRIMMED 	2048
#define DMA8_BUF_SIZE_UNTRIMMED 	2048
#define DMA16_BUF_SIZE						((DMA16_BUF_SIZE_UNTRIMMED/3)*3)
#define DMA8_BUF_SIZE 							((DMA8_BUF_SIZE_UNTRIMMED/3)*3)


#include "hw_config.h"
#include "stm32f10x_dma.h"

#define dmaStartRx() DMA_Init(DMA1_Channel4, &dmaStructure); \
    DMA_Cmd(DMA1_Channel4, ENABLE);

#define dmaStartTx() DMA_Init(DMA1_Channel5, &dmaStructure); \
    DMA_Cmd(DMA1_Channel5, ENABLE);
	 


void dmaWait(void);
void dmaInit(void);
void Spi2DmaInit(void);

void dmaSend16(u16 *data, u32 n);

void dmaSendCmdCont(u8 cmd);

void dmaSendCmd(u8 cmd);
void dmaSendCmdCont(u8 cmd);

void dmaReceiveDataCont8(u8 *);

void dmaReceive8(u8 *data, u32);
void dmaSend16(u16 *data, u32 n);
void dmaSend8(u8 *data, u32 n);

void dmaSendData8(u8 *data, u32 n);

void dmaSendData16(u16 *data, u32 n);

void dmaSendDataCont8(u8 *data, u32 n);
void dmaSendDataCont16(u16 *data, u32 n);

void dmaSendDataBuf16(void);
void dmaSendDataContBuf16(u16 *data, u32 n);

void dmaSendCircular8(u16 *data, u32 n);
void dmaSendCircular16(u16 *data, u32 n);

void dmaFill16(u16 color, u32 n);

#endif //TEST1_DMA_H
