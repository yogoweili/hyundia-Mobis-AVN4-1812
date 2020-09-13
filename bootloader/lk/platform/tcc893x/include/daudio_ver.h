#ifndef __DAUDIO_BOARD_VER__
#define __DAUDIO_BOARD_VER__

typedef enum daudio_board_ver_data
{
	DAUDIO_BOARD_5TH = 2,	// D-Audio US, Platform V.01, Default
	DAUDIO_BOARD_6TH,	// D-Audio+ , Platform V.02, 2015.4.9
	DAUDIO_PLATFORM_3,
	DAUDIO_PLATFORM_4,
	DAUDIO_PLATFORM_5,
	DAUDIO_PLATFORM_6,
	DAUDIO_PLATFORM_7,
	DAUDIO_PLATFORM_8,
	DAUDIO_PLATFORM_9,
} daudio_board_ver_data_t;

typedef enum daudio_ver_bt
{
	DAUDIO_BT_2MODULE,
	DAUDIO_BT_1MODULE,
	DAUDIO_BT_RESERVED,
	DAUDIO_BT_ONLY_CK,
	DAUDIO_BT_ONLY_SMD,
} daudio_ver_bt_t;

typedef enum daudio_ver_data
{
	DAUDIO_VER_GPS,
	DAUDIO_VER_HW,
	DAUDIO_VER_MAIN,
	DAUDIO_VER_BT,
} daudio_ver_data_t;

typedef enum daudio_ver_hw
{
        DAUDIO_HW_1ST,		// HW V.01, Default
        DAUDIO_HW_2ND,
        DAUDIO_HW_3RD,
        DAUDIO_HW_4TH,
        DAUDIO_HW_5TH,
        DAUDIO_HW_6TH,
        DAUDIO_HW_7TH,
        DAUDIO_HW_8TH,
        DAUDIO_HW_9TH,
} daudio_ver_hw_t;

typedef enum daudio_ver_gps
{
	DAUDIO_KET_GPS_7,
	DAUDIO_TRIMBLE_8_LG,
	DAUDIO_KET_GPS_8_LG,
	DAUDIO_TRIMBLE_8_AUO,
	DAUDIO_TRIMBLE_7,
	DAUDIO_KET_GPS_8_AUO,
	DAUDIO_GPS_RESERVED,
} daudio_ver_gps_t;

void init_daudio_ver(unsigned long *adc);
unsigned char get_daudio_hw_ver(void);
unsigned char get_daudio_main_ver(void);
unsigned char get_daudio_bt_ver(void);
unsigned char get_daudio_gps_ver(void);
unsigned long get_daudio_hw_adc(void);
unsigned long get_daudio_main_adc(void);
unsigned long get_daudio_bt_adc(void);
unsigned long get_daudio_gps_adc(void);
#endif
