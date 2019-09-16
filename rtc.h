#ifndef _RTC_H
#define _RTC_H

#include "misc.h"

u8 RTC_init(void);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////

#define BATTERY_REMOVED 98
#define BATTERY_RESTORED 99
#define SECONDS_IN_DAY 86399
#define CONFIGURATION_DONE 0xAAAA
#define CONFIGURATION_RESET 0x0000
#define OCTOBER_FLAG_SET 0x4000
#define MARCH_FLAG_SET 0x8000

static u8  *MonthsNames[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec","invalid"};

/* Date Structure definition */
struct Date_s
{
  uint8_t Month;
  uint8_t Day;
  uint16_t Year;
};

struct Time_s
{
	uint8_t SecLowOld;
	uint8_t SecLow;
	uint8_t SecHigh;
	uint8_t MinLow;
	uint8_t MinHigh;
	uint8_t HourLow;
	uint8_t HourHigh;
	uint8_t skew;
};

void CalculateTime(void);
void DateUpdate(void);
void SetDate(uint8_t Day, uint8_t Month, uint16_t Year);
void SetTime(uint8_t Hour,uint8_t Minute,uint8_t Seconds);
uint16_t WeekDay(uint16_t CurrentYear,uint8_t CurrentMonth,uint8_t CurrentDay);
void rtc_adjust(void);

#endif
