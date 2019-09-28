//*****************************************************************************
//
//! \file w5500.c
//! \brief W5500 HAL Interface.
//! \version 1.0.1
//! \date 2013/10/21
//! \par  Revision history
//!       <2014/05/01> V1.0.2
//!         1. Implicit type casting -> Explicit type casting. Refer to M20140501
//!            Fixed the problem on porting into under 32bit MCU
//!            Issued by Mathias ClauBen, wizwiki forum ID Think01 and bobh
//!            Thank for your interesting and serious advices.
//!       <2013/10/21> 1st Release
//!       <2013/12/20> V1.0.1
//!         1. Remove warning
//!         2. WIZCHIP_READ_BUF WIZCHIP_WRITE_BUF in case _WIZCHIP_IO_MODE_SPI_FDM_
//!            for loop optimized(removed). refer to M20131220
//! \author MidnightCow
//! \copyright
//!
//! Copyright (c)  2013, WIZnet Co., LTD.
//! All rights reserved.
//! 
//! Redistribution and use in source and binary forms, with or without 
//! modification, are permitted provided that the following conditions 
//! are met: 
//! 
//!     * Redistributions of source code must retain the above copyright 
//! notice, this list of conditions and the following disclaimer. 
//!     * Redistributions in binary form must reproduce the above copyright
//! notice, this list of conditions and the following disclaimer in the
//! documentation and/or other materials provided with the distribution. 
//!     * Neither the name of the <ORGANIZATION> nor the names of its 
//! contributors may be used to endorse or promote products derived 
//! from this software without specific prior written permission. 
//! 
//! THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//! AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
//! IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//! ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
//! LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
//! CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
//! SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//! INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
//! CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
//! ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
//! THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************




#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stm32f10x.h>
#include <stdbool.h>
#include "stm32f10x_rcc.h"
#include "lcd_glcdfont.h"
#include "eth_w5500.h"
#include "eth_wizchip_conf.h"
#include "hw_config.h"
#include "dma.h"

#define _W5500_SPI_VDM_OP_          0x00
#define _W5500_SPI_FDM_OP_LEN1_     0x01
#define _W5500_SPI_FDM_OP_LEN2_     0x02
#define _W5500_SPI_FDM_OP_LEN4_     0x03

static u16 dummybyte[1]={0x0};

///////////////////////////
// Network Configuration //
///////////////////////////
wiz_NetInfo gWIZNETINFO = { .mac = {0x02, 0x00, 0x01,0x02, 0x03, 0x04},
                            .ip = {1, 1, 1, 104},
                            .sn = {255,255,255,252},
                            .gw = {1, 1, 1, 254},
                            .dns = {1,1,1,254},
                            .dhcp = NETINFO_STATIC };

/////////////////////
// PHYStatus check //
/////////////////////
#define SEC_PHYSTATUS_CHECK 		1		// sec
bool PHYStatus_check_enable = false;
bool PHYStatus_check_flag = true;


////////////////////////////////////////////////////

void W5500_Init(void)
{
	uint8_t memsize[2][8] = { { 2, 2, 2, 2, 2, 2, 2, 2 }, { 2, 2, 2, 2, 2, 2, 2, 2 } };
	uint8_t txsize[8] = {2,2,2,2,2,2,2,2};
   uint8_t rxsize[8] = {2,2,2,2,2,2,2,2};
	
	//Chip_GPIO_SetPinState(LPC_GPIO, 0, 2, true);	// SSEL(CS)


	//reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
	//reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);

	/* wizchip initialize*/
	if (ctlwizchip(CW_INIT_WIZCHIP, (void*) memsize) == -1)
	{
		//printf("WIZCHIP Initialized fail.\r\n");
		while (1);
	}
/*
	do {
		if (ctlwizchip(CW_GET_PHYLINK, (void*) &tmp) == -1)
			printf("Unknown PHY Link stauts.\r\n");
	} while (tmp == PHY_LINK_OFF);
*/
	network_init();
	wizchip_init(txsize,rxsize);
} 

uint8_t  WIZCHIP_READ(uint32_t AddrSel)
{
   uint8_t spi_data[3];

   //WIZCHIP_CRITICAL_ENTER();
	dmaWait();
	LAN_CS_RESET;

   AddrSel |= (_W5500_SPI_READ_ | _W5500_SPI_VDM_OP_);

   //WIZCHIP.IF.SPI._write_byte((AddrSel & 0x00FF0000) >> 16);
   //WIZCHIP.IF.SPI._write_byte((AddrSel & 0x0000FF00) >>  8);
   //WIZCHIP.IF.SPI._write_byte((AddrSel & 0x000000FF) >>  0);
   //ret = WIZCHIP.IF.SPI._read_byte();
   spi_data[0] = (AddrSel & 0x00FF0000) >> 16;
   spi_data[1] = (AddrSel & 0x0000FF00) >> 8;
   spi_data[2] = (AddrSel & 0x000000FF) >> 0;
	
	dmaSend8(spi_data,3);
	
	spi_data[0]=SPI2->DR;			//empty buffer
	dmaReceive8(spi_data,1);		//rearm dma for receiving a single byte
	dmaSendCircular8(dummybyte,1);	//dummy send to create clock for receive

	dmaWait();
	LAN_CS_SET;
	
   //WIZCHIP_CRITICAL_EXIT();
   return spi_data[0];
}

void     WIZCHIP_WRITE(uint32_t AddrSel, uint8_t wb )
{
   uint8_t spi_data[4];

   //WIZCHIP_CRITICAL_ENTER();
	dmaWait();
   LAN_CS_RESET;

   AddrSel |= (_W5500_SPI_WRITE_ | _W5500_SPI_VDM_OP_);

   //WIZCHIP.IF.SPI._write_byte((AddrSel & 0x00FF0000) >> 16);
   //WIZCHIP.IF.SPI._write_byte((AddrSel & 0x0000FF00) >>  8);
   //WIZCHIP.IF.SPI._write_byte((AddrSel & 0x000000FF) >>  0);
   //WIZCHIP.IF.SPI._write_byte(wb);
   spi_data[0] = (AddrSel & 0x00FF0000) >> 16;
   spi_data[1] = (AddrSel & 0x0000FF00) >> 8;
   spi_data[2] = (AddrSel & 0x000000FF) >> 0;
   spi_data[3] = wb;
	
	dmaSend8(spi_data,4);
	dmaWait();
	
   LAN_CS_SET;
   //WIZCHIP_CRITICAL_EXIT();
}
         
void WIZCHIP_READ_BUF (uint32_t AddrSel, uint8_t* pBuf, uint16_t len)
{
   uint8_t spi_data[3];
   //uint16_t i;

   //WIZCHIP_CRITICAL_ENTER();
	dmaWait();
   LAN_CS_RESET;

   AddrSel |= (_W5500_SPI_READ_ | _W5500_SPI_VDM_OP_);

   //WIZCHIP.IF.SPI._write_byte((AddrSel & 0x00FF0000) >> 16);
   //WIZCHIP.IF.SPI._write_byte((AddrSel & 0x0000FF00) >>  8);
   //WIZCHIP.IF.SPI._write_byte((AddrSel & 0x000000FF) >>  0);
   //for(i = 0; i < len; i++)
   //  pBuf[i] = WIZCHIP.IF.SPI._read_byte();
   spi_data[0] = (AddrSel & 0x00FF0000) >> 16;
   spi_data[1] = (AddrSel & 0x0000FF00) >> 8;
   spi_data[2] = (AddrSel & 0x000000FF) >> 0;
	dmaSend8(spi_data,3);
	
	dmaWait();
	spi_data[0]=SPI2->DR;				//empty buffer
	dmaReceive8(pBuf,len);				//rearm dma for receiving a single byte
	dmaSendCircular8(dummybyte,len);	//dummy send to create clock for receive
	
	dmaWait();
   LAN_CS_SET;
   //WIZCHIP_CRITICAL_EXIT();
}

void WIZCHIP_WRITE_BUF(uint32_t AddrSel, uint8_t* pBuf, uint16_t len)
{
   uint8_t spi_data[3];
   //uint16_t i;

   //WIZCHIP_CRITICAL_ENTER();
	dmaWait();
   LAN_CS_RESET;

   AddrSel |= (_W5500_SPI_WRITE_ | _W5500_SPI_VDM_OP_);

   //WIZCHIP.IF.SPI._write_byte((AddrSel & 0x00FF0000) >> 16);
   //WIZCHIP.IF.SPI._write_byte((AddrSel & 0x0000FF00) >>  8);
   //WIZCHIP.IF.SPI._write_byte((AddrSel & 0x000000FF) >>  0);
   //for(i = 0; i < len; i++)
   //   WIZCHIP.IF.SPI._write_byte(pBuf[i]);
   spi_data[0] = (AddrSel & 0x00FF0000) >> 16;
   spi_data[1] = (AddrSel & 0x0000FF00) >> 8;
   spi_data[2] = (AddrSel & 0x000000FF) >> 0;
	dmaSend8(spi_data,3);
	dmaSend8(pBuf,len);

	dmaWait();
   LAN_CS_SET;
   //WIZCHIP_CRITICAL_EXIT();
}


uint16_t getSn_TX_FSR(uint8_t sn)
{
   uint16_t val=0,val1=0;

   do
   {
      val1 = WIZCHIP_READ(Sn_TX_FSR(sn));
      val1 = (val1 << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_TX_FSR(sn),1));
      if (val1 != 0)
      {
        val = WIZCHIP_READ(Sn_TX_FSR(sn));
        val = (val << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_TX_FSR(sn),1));
      }
   }while (val != val1);
   return val;
}


uint16_t getSn_RX_RSR(uint8_t sn)
{
   uint16_t val=0,val1=0;

   do
   {
      val1 = WIZCHIP_READ(Sn_RX_RSR(sn));
      val1 = (val1 << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_RX_RSR(sn),1));
      if (val1 != 0)
      {
        val = WIZCHIP_READ(Sn_RX_RSR(sn));
        val = (val << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_RX_RSR(sn),1));
      }
   }while (val != val1);
   return val;
}

void wiz_send_data(uint8_t sn, uint8_t *wizdata, uint16_t len)
{
   uint16_t ptr = 0;
   uint32_t addrsel = 0;

   if(len == 0)  return;
   ptr = getSn_TX_WR(sn);
   //M20140501 : implict type casting -> explict type casting
   //addrsel = (ptr << 8) + (WIZCHIP_TXBUF_BLOCK(sn) << 3);
   addrsel = ((uint32_t)ptr << 8) + (WIZCHIP_TXBUF_BLOCK(sn) << 3);
   //
   WIZCHIP_WRITE_BUF(addrsel,wizdata, len);
   
   ptr += len;
   setSn_TX_WR(sn,ptr);
}

void wiz_recv_data(uint8_t sn, uint8_t *wizdata, uint16_t len)
{
   uint16_t ptr = 0;
   uint32_t addrsel = 0;
   
   if(len == 0) return;
   ptr = getSn_RX_RD(sn);
   //M20140501 : implict type casting -> explict type casting
   //addrsel = ((ptr << 8) + (WIZCHIP_RXBUF_BLOCK(sn) << 3);
   addrsel = ((uint32_t)ptr << 8) + (WIZCHIP_RXBUF_BLOCK(sn) << 3);
   //
   WIZCHIP_READ_BUF(addrsel, wizdata, len);
   ptr += len;
   
   setSn_RX_RD(sn,ptr);
}


void wiz_recv_ignore(uint8_t sn, uint16_t len)
{
   uint16_t ptr = 0;

   ptr = getSn_RX_RD(sn);
   ptr += len;
   setSn_RX_RD(sn,ptr);
}


/////////////////////////////////////////////////////////////
// Intialize the network information to be used in WIZCHIP //
/////////////////////////////////////////////////////////////
void network_init(void)
{
   uint8_t tmpstr[6];
	ctlnetwork(CN_SET_NETINFO, (void*)&gWIZNETINFO);
	ctlnetwork(CN_GET_NETINFO, (void*)&gWIZNETINFO);

	ctlwizchip(CW_GET_ID,(void*)tmpstr);
}

/////////////////////////////////////////////////////////////									 

void Net_Conf(void)
{
	/* wizchip netconf */
	ctlnetwork(CN_SET_NETINFO, (void*) &gWIZNETINFO);
}

void Display_Net_Conf(void)
{
	uint8_t tmpstr[6] = {0,};
	ctlnetwork(CN_GET_NETINFO, (void*) &gWIZNETINFO);

	// Display Network Information
	ctlwizchip(CW_GET_ID,(void*)tmpstr);
}


void PHYStatus_Check(void)
{
	uint8_t tmp;

	do
	{
		ctlwizchip(CW_GET_PHYLINK, (void*) &tmp);

		// Error indicator: LED Green ON when no PHY link detected
		if(tmp == PHY_LINK_OFF)
		{

		}
	}while(tmp == PHY_LINK_OFF);
}

