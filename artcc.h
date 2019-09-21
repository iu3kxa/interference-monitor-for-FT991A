#ifndef	_AIR_ROUTE_TRAFFIC_CONTROL_CENTER
#define	_AIR_ROUTE_TRAFFIC_CONTROL_CENTER

#define EE_TOUCH 		0x0003		//10 bytes riservati	
#define EE_RTC_SKEW 	0x0010		//3 bytes riservati

#define EE_CH_PAGE 11				//prima pagina nella eeprom dedicata alle frequenze


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "stm32f10x.h"
#include "hw_config.h"



struct _artcc_frequencies
{
	u32 frequenza;
	u16 deviazione;	//rispetto alla portante
	u8	 attivo;
	u8  orari;
	u8  conflitto;
};


u8 artcc_init(void);
void eeprom_check(void);
void log_udp (u16);
void artcc_main_open (void);
void artcc_main_run (void);
#endif
