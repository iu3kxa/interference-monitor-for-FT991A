#include "main.h"

#define RTX_MODE_LSB	'1'
#define RTX_MODE_USB '2'
#define RTX_MODE_CW	'3'
#define RTX_MODE_FM	'4'
#define RTX_MODE_AM	'5'
#define RTX_MODE_RTTY_LSB	'6'
#define RTX_MODE_CW_R	'7'
#define RTX_MODE_DATA_LSB	'8'
#define RTX_MODE_RTTY_USB	'9'
#define RTX_MODE_DATA_FM	'A'
#define RTX_MODE_FM_N		'B'
#define RTX_MODE_DATA_USB	'C'
#define RTX_MODE_AM_N		'D'
#define RTX_MODE_C4FM		'E'

#define RTX_TX_TXOFF			'0'
#define RTX_TX_TXON			'2'
#define RTX_TX_TXCAT			'1'

#define RTX_PS_ON				'1'
#define RTX_PS_OFF			'0'

#define RTX_INFO_VFO_A_TX	'0'

struct _rtx_status
{
	u32 vfo_A;
	u32 vfo_A_old;
	u32 vfo_B;
	u32 vfo_B_old;
	u16 spettro_rtx_left;
	u16 spettro_rtx_right;
	u16 tx_time;
	u8 mode;
	u8 power;
	u8 txmode;
	u8 info;
	u8 pc_debug_len;
	u8 rtx_debug_len;
	u8 pc_debugdata[20];
	u8 rtx_debugdata[20];
	u8 poll;
	bool changed;
	bool lcd_update;
	bool poweron;
};

//prototipi
void rtx_init(void);
void rtx_poll(void);
void rtx_ft991a_decode (u8 * message,u8 len);
