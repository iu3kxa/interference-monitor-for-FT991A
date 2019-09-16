#ifndef	_ENAV_DEFS
#define	_ENAV_DEFS

#define EE_TOUCH 		0x0003		//10 bytes riservati	
#define EE_RTC_SKEW 	0x0010		//3 bytes riservati

#define EE_CH_PAGE 11				//prima pagina nella eeprom dedicata alle frequenze


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "stm32f10x.h"
#include "hw_config.h"



struct Enav_Frequenze
{
	u32 frequenza;
	u16 deviazione;	//rispetto alla portante
	u8	 attivo;
	u8  orari;
};

struct Enav_Status
{
	u32 frequenza_rtx;
	u16 spettro_rtx;
	u8 mode_rtx;
	u8 power_rtx;
	u8 tx;
	u8 debug_len;
	u8 dabugdata[20];
	bool changed;
	bool poweron;
};

u8 enav_init(void);
void eeprom_check(void);
void log_udp (void);
void enav_main_open (void);
void enav_main_run (void);
#endif
