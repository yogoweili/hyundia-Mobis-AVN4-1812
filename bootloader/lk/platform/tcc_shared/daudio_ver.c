#include <daudio_ver.h>
#include <stdlib.h>

#if defined(CONFIG_DAUDIO_PRINT_DEBUG_MSG)
#define DEBUG_VER   1
#else
#define DEBUG_VER	0
#endif

#if (DEBUG_VER)
#define LKPRINT(fmt, args...) dprintf(INFO, "[cleinsoft board version] " fmt, ##args)
#else
#define LKPRINT(args...) do {} while(0)
#endif

#define	LSB			0.8056

typedef struct ADC_Range
{
	unsigned char ver;
	unsigned long min;
	unsigned long max;
} ADC_Range_t;

static ADC_Range_t hw_ranges[] =
{
	{DAUDIO_HW_1ST, 120, 439},	//HW Ver.01 - 0.3V
	{DAUDIO_HW_2ND, 440, 799},	//HW Ver.02 - 0.6V
	{DAUDIO_HW_3RD, 800, 1159},	//HW Ver.03 - 0.99V
	{DAUDIO_HW_4TH, 1160, 1489},	//HW Ver.04 - 1.33V
	{DAUDIO_HW_5TH, 1490, 1819},	//HW Ver.05 - 1.65V
	{DAUDIO_HW_6TH, 1820, 2149},	//HW Ver.06 - 1.98V
	{DAUDIO_HW_7TH, 2150, 2499},	//HW Ver.07 - 2.32V
	{DAUDIO_HW_8TH, 2500, 2879},	//HW Ver.08 - 2.72V
	{DAUDIO_HW_9TH, 2880, 3189},	//HW Ver.09 - 3.04V
};

static ADC_Range_t main_ranges[] =
{
	{DAUDIO_BOARD_5TH, 440, 799},	//D-AUDIO US platform - 0.6V
	{DAUDIO_BOARD_6TH, 1490, 1819},	//D-AUDIO+ platform - 1.65V
	{DAUDIO_PLATFORM_3, 1820, 2149},//Platform V.03 - 1.98V
	{DAUDIO_PLATFORM_4, 2150, 2499},//Platform V.04 - 2.32V
	{DAUDIO_PLATFORM_5, 2500, 2879},//Platform V.05 - 2.72V
	{DAUDIO_PLATFORM_6, 2880, 3189},//Platform V.06 - 3.04V
	{DAUDIO_PLATFORM_7, 120, 439},	//Platform V.07 - 0.27V
	{DAUDIO_PLATFORM_8, 800, 1159},	//Platform V.08 - 0.99V
	{DAUDIO_PLATFORM_9, 1160, 1489},//Platform V.09 - 1.33V
};

static ADC_Range_t bt_ranges[] =
{
	{DAUDIO_BT_2MODULE, 0, 449},     // BT only + BT/WIFI Combo(BCM89355) - 0V ~ 0.45V
	{DAUDIO_BT_1MODULE, 450, 1649},	 // BT/WIFI Combo(BCM89335), mornitor disassembled - 0.45V ~ 1.649V
	{DAUDIO_BT_2MODULE, 1650, 2249}, // BT only + BT/WIFI Combo(BCM89335) - 1.65V ~ 2.249V
	{DAUDIO_BT_RESERVED, 2250, 2849},// BT reserved - 2.25V ~ 2.849V
	{DAUDIO_BT_ONLY_CK, 2850, 3149}, // BT only, carkit - 2.85V ~ 3.149V
	{DAUDIO_BT_ONLY_SMD, 3150, 3300},// BT only, SMD - 3.15V ~ 3.3V
};

static ADC_Range_t gps_ranges[] =
{
	{DAUDIO_KET_GPS_7, 0, 599},		//KET GPS, 7inch, 0.3V
	{DAUDIO_TRIMBLE_8_LG, 600, 1299},	//TRIMBLE, 8inch(LG), 1.06V
	{DAUDIO_KET_GPS_7, 1300, 1819},		//KET GPS, 7inch, 1.65V
	{DAUDIO_KET_GPS_8_LG, 1820, 2149},	//KET_GPS, 8inch(LG), 1.98V
	{DAUDIO_TRIMBLE_8_AUO, 2150, 2499},	//TRIMBLE, 8inch(AUO), 2.32V
	{DAUDIO_TRIMBLE_7, 2500, 2879},		//TRIMBLE, 7inch, 2.72V
	{DAUDIO_KET_GPS_8_AUO, 2880, 3189},	//KET_GPS, 8inch(AUO), 3.04V
	{DAUDIO_GPS_RESERVED, 3190, 3300},	//RESERVED
};

static unsigned long adcs[4];
static unsigned char vers[4];
static int count = 0;

static int sizeof_adc_range_t(int what)
{
	int ret = 0;

	switch (what)
	{
		case DAUDIO_VER_HW:
			ret = sizeof(hw_ranges) / sizeof(hw_ranges[0]);
			break;

		case DAUDIO_VER_MAIN:
			ret = sizeof(main_ranges) / sizeof(hw_ranges[0]);
			break;

		case DAUDIO_VER_BT:
			ret = sizeof(bt_ranges) / sizeof(hw_ranges[0]);
			break;
		case DAUDIO_VER_GPS:
			ret = sizeof(gps_ranges) / sizeof(hw_ranges[0]);
			break;
		default:
			ret = 0;
			break;
	}

	return ret;
}

static unsigned char parse_ver(unsigned long adc, int what)
{
	int i;
	int size = sizeof_adc_range_t(what);
	unsigned char ret = DAUDIO_BOARD_5TH;		//default board
	unsigned long min, max;
	ADC_Range_t *temp_rage;

	switch (what)
	{
		case DAUDIO_VER_HW:
			temp_rage = hw_ranges;
			break;

		case DAUDIO_VER_MAIN:
			temp_rage = main_ranges;
			break;

		case DAUDIO_VER_BT:
			temp_rage = bt_ranges;
			break;
		case DAUDIO_VER_GPS:
			temp_rage = gps_ranges;
			break;

		default:
			temp_rage = 0;
			break;
	}

	if (temp_rage != 0)
	{
		for (i = 0; i < size; i++)
		{
			min = temp_rage[i].min;
			max = temp_rage[i].max;

			if (adc >= min && adc <= max)
			{
				ret = temp_rage[i].ver;
				count = i;
				break;
			}
		}
	}

	return ret;
}

void init_daudio_ver(unsigned long *adc)
{
	int i = 0;

	for (i = 0; i < 4; i++)
	{
		adcs[i] = *(adc + i) * LSB + LSB;
		vers[i] = parse_ver(adcs[i], i);
	}
}

unsigned char get_daudio_hw_ver(void)
{
	return vers[DAUDIO_VER_HW];
}

unsigned char get_daudio_main_ver(void)
{
	return vers[DAUDIO_VER_MAIN];
}

unsigned char get_daudio_bt_ver(void)
{
	return vers[DAUDIO_VER_BT];
}

unsigned char get_daudio_gps_ver(void)
{
	return vers[DAUDIO_VER_GPS];
}

unsigned long get_daudio_hw_adc(void)
{
	return adcs[DAUDIO_VER_HW];
}

unsigned long get_daudio_main_adc(void)
{
	return adcs[DAUDIO_VER_MAIN];
}

unsigned long get_daudio_bt_adc(void)
{
	return adcs[DAUDIO_VER_BT];
}

unsigned long get_daudio_gps_adc(void)
{
	return adcs[DAUDIO_VER_GPS];
}
