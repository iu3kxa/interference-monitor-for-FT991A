#ifndef	_EEPROM_24C64
#define	_EEPROM_24C64

//per 24lc64
//per scrittura singola
#define	EEPROM_SIZE_KBIT		64
#define	EEPROM_TOTAL_ADDR		(EEPROM_SIZE_KBIT<<7)
//per scrittura sequenziale
#define	EEPROM_TOTAL_PAGES	(EEPROM_TOTAL_ADDR>>5)
#define	EEPROM_PAGE_SIZE		32							
#define	EEPROM24XX_I2C			hi2c1	

#define I2C_BUFSIZE				30

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "stm32f10x.h"
#include "hw_config.h"

u8 eeprom_write(u16 address,u8 data);
u8 eeprom_read(u16 address);
void nvram_reset(void);
void eeprom_sequential_write(u16 addres,u8 *data, u8 count);
void eeprom_sequential_read(u16 addres,u8 count);
u8 eeprom_multiread(u16 Addr,u8 *data, u16 Count);
void I2C1_init(void);

#endif
