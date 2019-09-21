#define I2C_TIMEOUT_MAX 		100000		// 1000 per 50khz, 100 per 400khz
#define MEM_DEVICE_WRITE_ADDR 0xA0
#define MEM_DEVICE_READ_ADDR	0xA1
//#define I2C_SPEED					100000 	// 100 KHz	saltuariamente non va
//#define I2C_SPEED					200000 	// 200 KHz
#define I2C_SPEED					50000 	// 50 KHz


#include "i2c.h"
#include "main.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

unsigned char i2cbuf[I2C_BUFSIZE];
unsigned char buf[16];

u32 wait_for_i2c(u32);

/*******************************************************************/
// I2C
/*******************************************************************/
void I2C1_init(void)
{
	I2C_InitTypeDef  I2C_InitStructure;
	GPIO_InitTypeDef  GPIO_InitStructure;

	//I2C_Cmd(I2C1,ENABLE);
	I2C_DeInit(I2C1);
	RCC_APB2PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2ENR_AFIOEN, ENABLE);			//enable clock alternate funcion
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
	
	//Configure I2C_EE pins: SCL and SDA
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//I2C configuration
	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
	I2C_InitStructure.I2C_OwnAddress1 = 0x00;
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitStructure.I2C_ClockSpeed = I2C_SPEED;

	/* I2C Peripheral Enable */
	I2C_Cmd(I2C1, ENABLE);
	/* Apply I2C configuration after enabling it */
	I2C_Init(I2C1, &I2C_InitStructure);
	//I2C_ITConfig(I2C1, I2C_IT_ERR, ENABLE); 	//interrupt
} 

//----------------------------------------

u8 eeprom_write (u16 Addr,u8 Data)
{
	u8 upper_addr,lower_addr;
	
	lower_addr = (u8)((0x00FF)&Addr);
	upper_addr = Addr>>8;

	//Avvio i2c
	I2C_GenerateSTART(I2C1, ENABLE);
	if(wait_for_i2c(I2C_EVENT_MASTER_MODE_SELECT)) return 0xff;
	
	//comando scrittura
	I2C_Send7bitAddress(I2C1, MEM_DEVICE_WRITE_ADDR, I2C_Direction_Transmitter);
	if(wait_for_i2c(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) return 0xff;

	//indirizzo alto
	I2C_SendData(I2C1, upper_addr);
	if(wait_for_i2c(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) return 0xff;

	//indirizzo basso
	I2C_SendData(I2C1, lower_addr);
	if(wait_for_i2c(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) return 0xff;

	//invia dato
	I2C_SendData(I2C1, Data);
	if(wait_for_i2c(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) return 0xff;

	//commando arresto i2c
	I2C_GenerateSTOP(I2C1, ENABLE);
	delay_ms(10);
	return 0;
}

u8 eeprom_read(u16 Addr)
{
	u8 Data = 0;
	u8 upper_addr,lower_addr;

	lower_addr = (u8)((0x00FF)&Addr);
	upper_addr = Addr>>8;

	//avvia transazione i2c
	I2C_GenerateSTART(I2C1, ENABLE);
	if(wait_for_i2c(I2C_EVENT_MASTER_MODE_SELECT)) return 0x01;
	
	//invia byte controllo
	I2C_Send7bitAddress(I2C1, MEM_DEVICE_WRITE_ADDR, I2C_Direction_Transmitter);
	if(wait_for_i2c(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) return 0x02;
	
	//invia indirizzo byte alto
	I2C_SendData(I2C1,upper_addr);
	if(wait_for_i2c(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) return 0x03;
	
	//invia indirizzo byte basso
	I2C_SendData(I2C1, lower_addr);
	if(wait_for_i2c(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) return 0x04;
	
	//avvia transazione i2c
	I2C_GenerateSTART(I2C1, ENABLE);
	if(wait_for_i2c(I2C_EVENT_MASTER_MODE_SELECT)) return 0xff;
	
	I2C_Send7bitAddress(I2C1, MEM_DEVICE_READ_ADDR, I2C_Direction_Receiver);
	if(wait_for_i2c(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) return 0x05;
	
	I2C_AcknowledgeConfig(I2C1, DISABLE);  
	if(wait_for_i2c(I2C_EVENT_MASTER_BYTE_RECEIVED)) return 0x06;

	I2C_GenerateSTOP(I2C1, ENABLE);
	
	Data = I2C_ReceiveData(I2C1);
	return Data;
}


u8 eeprom_multiread(u16 Addr,u8 *data, u16 Count)
{
	u16 i;
	
	u8 *Data=data;
	
	for(i=0;i<Count;i++)
		Data[i] = eeprom_read(Addr+i);
	return 0;
}



/*
u8 eeprom_multiread(u16 Addr,u8 *data, u16 Count)
{
	u8 upper_addr,lower_addr,d;
	u16 i;
	
	u8 *Data=data;

	lower_addr = (u8)((0x00FF)&Addr);
	upper_addr = Addr>>8;

	//avvia transazione i2c
	I2C_GenerateSTART(I2C1, ENABLE);
	if(wait_for_i2c(I2C_EVENT_MASTER_MODE_SELECT)) return 11;
	
	//invia byte controllo
	I2C_Send7bitAddress(I2C1, MEM_DEVICE_WRITE_ADDR, I2C_Direction_Transmitter);
	if(wait_for_i2c(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) return 12;
	
	//invia indirizzo byte alto
	I2C_SendData(I2C1,upper_addr);
	if(wait_for_i2c(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) return 13;
	
	//invia indirizzo byte basso
	I2C_SendData(I2C1, lower_addr);
	if(wait_for_i2c(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) return 14;
	
	//avvia transazione i2c
	I2C_GenerateSTART(I2C1, ENABLE);
	if(wait_for_i2c(I2C_EVENT_MASTER_MODE_SELECT)) return 0xff;
	
	I2C_Send7bitAddress(I2C1, MEM_DEVICE_READ_ADDR, I2C_Direction_Receiver);
	if(wait_for_i2c(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) return 15;
	
	//first byte invalid
	i=I2C_ReceiveData(I2C1);
	I2C_AcknowledgeConfig(I2C1, ENABLE);  
		if(wait_for_i2c(I2C_EVENT_MASTER_BYTE_RECEIVED)) return 16;
	
	for(i=0;i<Count;i++)
	{
		d=I2C_ReceiveData(I2C1);	
		Data[i] = i;
		I2C_AcknowledgeConfig(I2C1, ENABLE);  
		if(wait_for_i2c(I2C_EVENT_MASTER_BYTE_RECEIVED)) return 17+i;
	}
	
	I2C_AcknowledgeConfig(I2C1, DISABLE);  
	if(wait_for_i2c(I2C_EVENT_MASTER_BYTE_RECEIVED)) return 0x18;

	I2C_GenerateSTOP(I2C1, ENABLE);

	return 0;
}
*/


void eeprom_cleanup(void)
{
  u16 adres=0;
  for(adres=0;adres<EEPROM_TOTAL_ADDR;adres++)
  {
    eeprom_write(adres,0xFF);
  }
}

void eeprom_sequential_write(u16 address,u8 *s,u8 size)
{
	u8 count;
	for (count=0; count < size ; count++)
		eeprom_write(address++,*s++);
}
 
void eeprom_sequential_read(u16 address,u8 count)
{
	u8 i;

	for(i=0;i<count;i++)
		i2cbuf[i]=eeprom_read(address+i);
}


u32 wait_for_i2c(u32 event)
{
	u32 timeout = I2C_TIMEOUT_MAX;
	while(!I2C_CheckEvent(I2C1, event))
		//if ((timeout--) == 0) return 0;			//per qualche motivo non viene letto l'ack
		if ((timeout--) == 0) return 0xFF;
	return 0;
}

//---------------------------------------- 

/*
int main(void)
{
	char buffer[80] = {'\0'};

	long temperature = 0;
	long pressure = 0;

	I2C1_init();
	BMP280_Init();
	usart_dma_init();

	TIM4_init();

    while(1)
    {
		if (FLAG_USART == 1) {
			bmp280Convert(&temperature, &pressure);
			sprintf(buffer, "Temperature: %d, Pressure: %d\r\n", (int)temperature/10, (int)pressure);
    		USARTSendDMA(buffer);
			FLAG_USART = 0;
    	}
    }
} 



#include "i2c.h"

//##########################################################################
bool		EEPROM24XX_IsConnected(void)
{

	if(HAL_I2C_IsDeviceReady(&_EEPROM24XX_I2C,0xa0,1,100)==HAL_OK)
		return true;
	else
		return false;	
}
//##########################################################################
bool		EEPROM24XX_Save(u16 Address,void *data,size_t size_of_data)
{

	if(size_of_data > 16)
		return false;
	
	if(HAL_I2C_Mem_Write(&_EEPROM24XX_I2C,0xa0|(Address&0x0001),Address>>1,I2C_MEMADD_SIZE_8BIT,(u8*)data,size_of_data,100) == HAL_OK)
	{
		HAL_Delay(7);
		return true;
	}
	return false;
}
//##########################################################################
bool		EEPROM24XX_Load(u16 Address,void *data,size_t size_of_data)
{
	if(HAL_I2C_Mem_Read(&_EEPROM24XX_I2C,0xa0|(Address&0x0001),Address>>1,I2C_MEMADD_SIZE_8BIT,(u8*)data,size_of_data,100) == HAL_OK)
		return true;

	return false;		
}
*/
