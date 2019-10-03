#define RTC_ACCESS_CODE	0xA55A
#define RTC_DEFAULT_CALIBRATION		26		//24 (max 127) per decrenentare 60 secondi al mese

#include <stdbool.h>
#include "stm32f10x.h"
#include "stm32f10x_bkp.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_rtc.h"
#include "stm32f10x_pwr.h"
#include "rtc.h"
#include "touch.h"
#include "lcd_core.h"
#include "atrcc.h"
#include "i2c.h"

//variabili
struct Time_s s_TimeStructVar;
struct Date_s s_DateStructVar;
extern unsigned char buf[16];
extern struct coord pos;

//prototipi
void get_RTC(void);
void set_RTC(u32 year, u8 month, u8 date, u8 hour, u8 minute, u8 second);
u8 check_for_leap_year(u16 year);

//Funzioni

//inizializza orologio
u8 RTC_init(void)
{
	u32 i;
	NVIC_InitTypeDef NVIC_InitStructure;
	//EXTI_InitTypeDef EXTI_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel =  RTC_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	//Abilita interrupt ogni secondo
	RTC->CRL |= RTC_CRL_CNF;
	while(!RTC->CRL & RTC_CRL_RTOFF);
	RTC->CRH|=RTC_CRH_SECIE;
	RTC->CRL &= ~RTC_CRL_CNF;
	while(!RTC->CRL & RTC_CRL_RTOFF);
		
		
	
	//Abilita bus di accesso all'orologio e ai registri backuppati
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
	
	//Se valore eeprom invalido
	if(eeprom_read(EE_RTC_SKEW) != 0x55 || eeprom_read(EE_RTC_SKEW+1) != 0xAA)
	{
		LCD_writeString((u8 *) " Skew invalido su eeprom, setto default.");
		eeprom_write(EE_RTC_SKEW,0x55);
		eeprom_write(EE_RTC_SKEW+1,0xAA);
		eeprom_write(EE_RTC_SKEW+2,RTC_DEFAULT_CALIBRATION);
	}

	//Permette l'accesso
	PWR_BackupAccessCmd(ENABLE);
	
	//la sincronizzazione fra orologio e bus richiede del tempo
	RTC_WaitForSynchro();
	
	//Corregge skew orologio
	i=eeprom_read(EE_RTC_SKEW+2);
	BKP_SetRTCCalibrationValue(i);
	s_TimeStructVar.skew=i;
	
	//attende che l'ultima scrittura sui registri sia terminata
	RTC_WaitForLastTask();

	//Se il valore del registro DR1 e' preservato, l'rtc sta funzionando., altrimenti effettua la prima programmazione.
	if(BKP->DR1 != RTC_ACCESS_CODE)
	{	
		//valori di partenza
		u8 def_hour = 14;
		u8 def_minute = 12;
		u8 def_second = 20;
		u8 def_day = 3;
		u8 def_month = 9;

		u32 def_year= 2019;
		s_TimeStructVar.skew=RTC_DEFAULT_CALIBRATION;
		
		//resetta backup domain
		BKP_DeInit();

		//Abilita clock esterno da 32768hz
		RCC_LSEConfig(RCC_LSE_ON);

		//Attende avvio 
		while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);
		
		//Seleziona oscillatore esterno rtc
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

		//abilita orologio
		RCC_RTCCLKCmd(ENABLE);

		//la sincronizzazione fra orologio e bus richiede del tempo
		RTC_WaitForSynchro();

		//attende che l'ultima scrittura sui registri sia terminata
		RTC_WaitForLastTask();

		//Abilita interrupt ogni secondo VEDERE SE SERVE QUI???
		RTC_ITConfig(RTC_IT_SEC, ENABLE);

		//attende che l'ultima scrittura sui registri sia terminata
		RTC_WaitForLastTask();

		//imposta divisore a 3276, periodo contato re 1 secondo
		// RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) 
		RTC_SetPrescaler(32767); 	

		//attende che l'ultima scrittura sui registri sia terminata
		RTC_WaitForLastTask();
		
		//abilita scrittura sui registri
		RTC_EnterConfigMode();

		//imposta valori fittizi sul calendario
		SetDate(def_day, def_month, def_year);
		SetTime(def_hour,def_minute,def_second);
				
		//attende che l'ultima scrittura sui registri sia terminata
		RTC_WaitForLastTask();

		//imposta valore conosciuto sul registro backuppato DR1
		BKP->DR1=RTC_ACCESS_CODE;
		
		//abilita scrittura sui registri
		RTC_EnterConfigMode();
		
		//attende che l'ultima scrittura sui registri sia terminata
		RTC_WaitForLastTask();

		return 1;
	}
	return 0;
}

//verifica se si tratta di un anno bisestile
u8 check_for_leap_year(u16 year)
{
	if((year%400)==0)
	{
		return 1;
	}
	else if((year%100)==0)
	{
		return 0;
	}
	else if((year%4)==0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

//interrupt chiamato da orologio ogni secondo
void RTC_IRQHandler(void)
{
	//If counter is equal to 86399: one day was elapsed, This takes care of date change and resetting of counter in case of power on - Run mode/ Main supply switched on condition
	if(RTC_GetCounter() == 86399)
	{
		// Wait until last write operation on RTC registers has finished
		RTC_WaitForLastTask();
		
		// Reset counter value
		RTC_SetCounter(0x0);
		
		// Wait until last write operation on RTC registers has finished
		RTC_WaitForLastTask();

		//Increment the date */
		DateUpdate();
	}
	
	/////////////////////////////
	CalculateTime();
	
	//resetta stato interrupt
	NVIC_ClearPendingIRQ(RTC_IRQn);
	RTC_ClearITPendingBit(RTC_IT_SEC);
}

/**
  * @brief Determines the weekday
  * @param Year,Month and Day
  * @retval :Returns the CurrentWeekDay Number 0- Sunday 6- Saturday
  */
uint16_t WeekDay(uint16_t CurrentYear,uint8_t CurrentMonth,uint8_t CurrentDay)
{
  uint16_t Temp1,Temp2,Temp3,Temp4,CurrentWeekDay;
  
  if(CurrentMonth < 3)
  {
    CurrentMonth=CurrentMonth + 12;
    CurrentYear=CurrentYear-1;
  }
  
  Temp1=(6*(CurrentMonth + 1))/10;
  Temp2=CurrentYear/4;
  Temp3=CurrentYear/100;
  Temp4=CurrentYear/400;
  CurrentWeekDay=CurrentDay + (2 * CurrentMonth) + Temp1 \
     + CurrentYear + Temp2 - Temp3 + Temp4 +1;
  CurrentWeekDay = CurrentWeekDay % 7;
  
  return(CurrentWeekDay);
}



/**
  * @brief  Sets the RTC Current Counter Value
  * @param Hour, Minute and Seconds data
  * @retval : None
  */
void SetTime(uint8_t Hour,uint8_t Minute,uint8_t Seconds)
{
  uint32_t CounterValue;
	
  CounterValue=((Hour * 3600)+ (Minute * 60)+Seconds);

  RTC_WaitForLastTask();
  RTC_SetCounter(CounterValue);
  RTC_WaitForLastTask();
}


/**
  * @brief  Sets the RTC Date(DD/MM/YYYY)
  * @param DD,MM,YYYY
  * @retval : None
  */
void SetDate(uint8_t Day, uint8_t Month, uint16_t Year)
{
	uint32_t DateTimer;
  
	//Check if the date entered by the user is correct or not, Displays an error message if date is incorrect
	if((( Month==4 || Month==6 || Month==9 || Month==11) && Day ==31) \
		|| (Month==2 && Day==31)|| (Month==2 && Day==30)|| \
      (Month==2 && Day==29 && (check_for_leap_year(Year)==0)))
	{
		// LCD_DisplayString(Line3,Column2,"INCORRECT DATE");
		DateTimer=RTC_GetCounter();
		while((RTC_GetCounter()-DateTimer)<2);
	}
  	
	// if date entered is correct then set the date
	else
	{
		PWR_BackupAccessCmd(ENABLE);
		BKP_WriteBackupRegister(BKP_DR2,Month);
		BKP_WriteBackupRegister(BKP_DR3,Day);
		BKP_WriteBackupRegister(BKP_DR4,Year);
		BKP_WriteBackupRegister(BKP_DR10,Year);
	}
}


/**
  * @brief Updates the Date (This function is called when 1 Day has elapsed
  * @param None
  * @retval :None
  */
void DateUpdate(void)
{
	s_DateStructVar.Month=BKP_ReadBackupRegister(BKP_DR2);
	if(s_DateStructVar.Month>12) s_DateStructVar.Month=12;
	s_DateStructVar.Year=BKP_ReadBackupRegister(BKP_DR4);
	s_DateStructVar.Day=BKP_ReadBackupRegister(BKP_DR3);
  
  if(s_DateStructVar.Month == 1 || s_DateStructVar.Month == 3 || \
    s_DateStructVar.Month == 5 || s_DateStructVar.Month == 7 ||\
     s_DateStructVar.Month == 8 || s_DateStructVar.Month == 10 \
       || s_DateStructVar.Month == 12)
  {
    if(s_DateStructVar.Day < 31)
    {
      s_DateStructVar.Day++;
    }
    /* Date structure member: s_DateStructVar.Day = 31 */
    else
    {
      if(s_DateStructVar.Month != 12)
      {
        s_DateStructVar.Month++;
        s_DateStructVar.Day = 1;
      }
     /* Date structure member: s_DateStructVar.Day = 31 & s_DateStructVar.Month =12 */
      else
      {
        s_DateStructVar.Month = 1;
        s_DateStructVar.Day = 1;
        s_DateStructVar.Year++;
      }
    }
  }
  else if(s_DateStructVar.Month == 4 || s_DateStructVar.Month == 6 \
            || s_DateStructVar.Month == 9 ||s_DateStructVar.Month == 11)
  {
    if(s_DateStructVar.Day < 30)
    {
      s_DateStructVar.Day++;
    }
    /* Date structure member: s_DateStructVar.Day = 30 */
    else
    {
      s_DateStructVar.Month++;
      s_DateStructVar.Day = 1;
    }
  }
  else if(s_DateStructVar.Month == 2)
  {
    if(s_DateStructVar.Day < 28)
    {
      s_DateStructVar.Day++;
    }
    else if(s_DateStructVar.Day == 28)
    {
      /* Leap Year Correction */
      if(check_for_leap_year(s_DateStructVar.Year))
      {
        s_DateStructVar.Day++;
      }
      else
      {
        s_DateStructVar.Month++;
        s_DateStructVar.Day = 1;
      }
    }
    else if(s_DateStructVar.Day == 29)
    {
      s_DateStructVar.Month++;
      s_DateStructVar.Day = 1;
    }
  }
  
  BKP_WriteBackupRegister(BKP_DR2,s_DateStructVar.Month);
  BKP_WriteBackupRegister(BKP_DR3,s_DateStructVar.Day);
  BKP_WriteBackupRegister(BKP_DR4,s_DateStructVar.Year);
}


/**
  * @brief Calcuate the Time (in hours, minutes and seconds  derived from
  *   COunter value
  * @param None
  * @retval :None
  */
void CalculateTime(void)
{
  uint32_t TimeVar;
  
  TimeVar=RTC_GetCounter();
  TimeVar=TimeVar % 86400;
  s_TimeStructVar.HourHigh=(uint8_t)(TimeVar/3600)/10;
  s_TimeStructVar.HourLow=(uint8_t)(TimeVar/3600)%10;
  s_TimeStructVar.MinHigh=(uint8_t)((TimeVar%3600)/60)/10;
  s_TimeStructVar.MinLow=(uint8_t)((TimeVar%3600)/60)%10;
  s_TimeStructVar.SecHigh=(uint8_t)((TimeVar%3600)%60)/10;
  s_TimeStructVar.SecLow=(uint8_t)((TimeVar %3600)%60)%10;
}

/**
  * @brief Chaeks is counter value is more than 86399 and the number of
  *   elapsed and updates date that many times
  * @param None
  * @retval :None
  */
void CheckForDaysElapsed(void)
{
  uint8_t DaysElapsed;
 
  if((RTC_GetCounter() / SECONDS_IN_DAY) != 0)
  {
    for(DaysElapsed = 0; DaysElapsed < (RTC_GetCounter() / SECONDS_IN_DAY)\
         ;DaysElapsed++)
    {
      DateUpdate();
    }

    RTC_SetCounter(RTC_GetCounter() % SECONDS_IN_DAY);
  }
}

//menu touch per settare il calendario
void rtc_adjust(void)
{
	u32 i;
	LCD_fillScreen(BLACK);
	
	//disagna pulsanti incremento
	LCD_fillRect(5, 5, 50, 40, NAVY);
	LCD_fillRect(65, 5, 50, 40, NAVY);
	LCD_fillRect(125, 5, 50, 40, NAVY);
	LCD_fillRect(185, 5, 50, 40, NAVY);
	LCD_fillRect(245, 5, 50, 40, NAVY);
	LCD_fillRect(305, 5, 50, 40, NAVY);
	//LCD_fillRect(365, 5, 50, 40, NAVY);
	LCD_fillRect(425, 5, 50, 40, NAVY);
	
	LCD_setTextSize(3);
	LCD_setTextColor(WHITE);
	LCD_setTextBgColor(NAVY);

	LCD_setCursor(22,15);
	LCD_writeString((u8*) "+");
	LCD_setCursor(82,15);
	LCD_writeString((u8*) "+");
	LCD_setCursor(142,15);
	LCD_writeString((u8*) "+");
	LCD_setCursor(202,15);
	LCD_writeString((u8*) "+");
	LCD_setCursor(262,15);
	LCD_writeString((u8*) "+");
	LCD_setCursor(322,15);
	LCD_writeString((u8*) "+");
	LCD_setCursor(442,15);
	LCD_writeString((u8*) "+");

	//disegna pulsanti decremento
	LCD_fillRect(5, 120, 50, 40, MAROON);
	LCD_fillRect(65, 120, 50, 40, MAROON);
	LCD_fillRect(125, 120, 50, 40, MAROON);
	LCD_fillRect(185, 120, 50, 40, MAROON);
	LCD_fillRect(245, 120, 50, 40, MAROON);
	LCD_fillRect(305, 120, 50, 40, MAROON);
	//LCD_fillRect(365, 120, 50, 40, MAROON);
	LCD_fillRect(425, 120, 50, 40, MAROON);
	
	LCD_setTextBgColor(MAROON);
	LCD_setCursor(22,128);
	LCD_writeString((u8*) "-");
	LCD_setCursor(82,128);
	LCD_writeString((u8*) "-");
	LCD_setCursor(142,128);
	LCD_writeString((u8*) "-");
	LCD_setCursor(202,128);
	LCD_writeString((u8*) "-");
	LCD_setCursor(262,128);
	LCD_writeString((u8*) "-");
	LCD_setCursor(322,128);
	LCD_writeString((u8*) "-");
	LCD_setCursor(442,128);
	LCD_writeString((u8*) "-");
	
	LCD_setCursor(442,128);
		

	//tasto exit
	LCD_setTextSize(3);
	LCD_setTextBgColor(BLACK);
	LCD_setTextColor(WHITE);
	LCD_setCursor(345,258);
	LCD_writeString((u8*) "EXIT");
	
	//istruzioni calibrazione
	LCD_setTextSize(2);
	LCD_setCursor(5,180);
	LCD_writeString((u8*) "Skew rallenta l'orologio in ppm/s.");
	LCD_setCursor(5,198);
	LCD_writeString((u8*) "Inserire valore 0 a 127, default 25.");
	LCD_setCursor(5,216);
	LCD_writeString((u8*) "Ogni step decrementa 3 secondi al mese.");

	//se schermo premuto attende rilasciao
	while(pos.touch_pressed==1)
		touch_sample();
	
	//verifica che non sia premuto exit
	while(pos.spot_X3_Y2 != 6)
	{
		delay_ms(100);
		
		//aggiorna calendario su schermo
		LCD_DrawDate(15,72,3);
		LCD_DrawDate(0,310,1);
		LCD_setCursor(345,258);
		LCD_setTextSize(2);
		LCD_setCursor(362,77);
		LCD_writeString((u8*) "SKEW");
		LCD_setCursor(420 ,72);
		LCD_setTextSize(3);
		buf[0]=(s_TimeStructVar.skew / 100) + 0x30;
		buf[1]=(s_TimeStructVar.skew / 10) + 0x30;
		buf[2]=(s_TimeStructVar.skew % 10) + 0x30;
		buf[3]=0;
		LCD_writeString(buf);
		
		//legge spot premuto ed aggiusta calendario
		touch_sample();
		if (pos.touch_pressed)
		{
			LCD_DrawDate(15,72,3);
			switch(pos.spot_X8_Y6)
			{
				//+
				case 1:
					SetDate(s_DateStructVar.Day,s_DateStructVar.Month,s_DateStructVar.Year+1);
				break;
				case 2:
					if(s_DateStructVar.Month==12)
						SetDate(s_DateStructVar.Day,1,s_DateStructVar.Year);
					else
						SetDate(s_DateStructVar.Day,s_DateStructVar.Month+1,s_DateStructVar.Year);
				break;
				case 3:
					if(s_DateStructVar.Day==31)
						SetDate(1,s_DateStructVar.Month,s_DateStructVar.Year);
					else
						SetDate(s_DateStructVar.Day+1,s_DateStructVar.Month,s_DateStructVar.Year);
				break;
				case 4:
					RTC_SetCounter(RTC_GetCounter()+3600);
				break;
				case 5:
					RTC_SetCounter(RTC_GetCounter()+60);
				break;
				case 6:
					RTC_SetCounter(RTC_GetCounter()+1);
				break;
				case 8:	//skew+
					i=eeprom_read(EE_RTC_SKEW+2);
					if(i<127)
					{	
						s_TimeStructVar.skew=i+1;
						eeprom_write(EE_RTC_SKEW+2,i+1);
					}
				break;
				
				//-
				case 17:
					SetDate(s_DateStructVar.Day,s_DateStructVar.Month,s_DateStructVar.Year-1);
				break;
				case 18:
					if(s_DateStructVar.Month==1)
						SetDate(s_DateStructVar.Day,12,s_DateStructVar.Year);
					else
						SetDate(s_DateStructVar.Day,s_DateStructVar.Month-1,s_DateStructVar.Year);
				break;
				case 19:
					if(s_DateStructVar.Day==1)
						SetDate(31,s_DateStructVar.Month,s_DateStructVar.Year);
					else
						SetDate(s_DateStructVar.Day-1,s_DateStructVar.Month+1,s_DateStructVar.Year);
				break;
				case 20:
					RTC_SetCounter(RTC_GetCounter()-3600);
				break;
				case 21:
					RTC_SetCounter(RTC_GetCounter()-60);
				break;
				case 22:
					RTC_SetCounter(RTC_GetCounter()-1);
				break;
				case 24:	//salva calibrazione in eeprom
					i=eeprom_read(EE_RTC_SKEW+2);
					if(i>0)
					{
						s_TimeStructVar.skew=i-1;
						eeprom_write(EE_RTC_SKEW+2,i-1);
					}
				break;
				default:
				;
			}						//fine switch
		}							//fine pos.touch
	}								//se EXIT premuto esce dal loop e termina
	LCD_fillScreen(BLACK);
}
