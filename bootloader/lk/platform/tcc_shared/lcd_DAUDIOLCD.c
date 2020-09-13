#include <stdlib.h>
#include <string.h>
#include <tcc_lcd.h>

#ifdef DAUDIOLCD
#include <tnftl/IO_TCCXXX.h>

#include <tw8836.h>
#include <tw9912.h>
#include <isl248xx.h>
#include <daudio_settings.h>
#include <daudio_ie.h>
#include <daudio_ver.h>

#define LVDS_VESA
#define DEBUG_LCD	1

#if !defined(CONFIG_DAUDIO_PRINT_DEBUG_MSG)
#undef 	DEBUG_LCD
#define DEBUG_LCD	0
#endif

#if (DEBUG_LCD)
#define INFOP(fmt, args...) dprintf(INFO, "[cleinsoft lcd] " fmt, ##args)
#define SPEWP(fmt, args...) dprintf(SPEW, "[cleinsoft lcd] " fmt, ##args)
#else
#define INFOP(args...) do {} while(0)
#define SPEWP(args...) do {} while(0)
#endif

static int daudiolcd_panel_init(struct lcd_panel *panel)
{
	SPEWP("%s\n", __func__);

	return 0;
}

static int daudiolcd_set_power(struct lcd_panel *panel, int on)
{
	unsigned int P, M, S, VSEL, TC;
	PDDICONFIG pDDICfg = (DDICONFIG *)HwDDI_CONFIG_BASE;
	struct lcd_platform_data *pdata = &(panel->dev);
	struct ie_setting_info info;
	int ret = 0, main_ver = get_daudio_main_ver();

	INFOP("%s on: %d\n", __func__, on);

	if (on) {
		lcdc_initialize(pdata->lcdc_num, panel);

		// LVDS reset	
		pDDICfg->LVDS_CTRL.bREG.RST =1;
		pDDICfg->LVDS_CTRL.bREG.RST =0;

#if defined(LVDS_VESA)
		/**
		 * @author valky@cleinsoft
		 * @date 2013/08/20
		 * LVDS VESA mode
		 **/
		BITCSET(pDDICfg->LVDS_TXO_SEL0.nREG, 0xFFFFFFFF, 0x13121110);	//R3, R2, R1, R0
		BITCSET(pDDICfg->LVDS_TXO_SEL1.nREG, 0xFFFFFFFF, 0x09081514);	//G1, G0, R5, R4
		BITCSET(pDDICfg->LVDS_TXO_SEL2.nREG, 0xFFFFFFFF, 0x0D0C0B0A);	//G5, G4, G3, G2
		BITCSET(pDDICfg->LVDS_TXO_SEL3.nREG, 0xFFFFFFFF, 0x03020100);	//B3, B2, B1, B0
		BITCSET(pDDICfg->LVDS_TXO_SEL4.nREG, 0xFFFFFFFF, 0x1A190504);	//VS, HS, B5, B4
		BITCSET(pDDICfg->LVDS_TXO_SEL5.nREG, 0xFFFFFFFF, 0x0E171618);	//G6, R7, R6, DE
		BITCSET(pDDICfg->LVDS_TXO_SEL6.nREG, 0xFFFFFFFF, 0x1F07060F);	//XX, B7, B6, G7
		BITCSET(pDDICfg->LVDS_TXO_SEL7.nREG, 0xFFFFFFFF, 0x1F1E1F1E);
		BITCSET(pDDICfg->LVDS_TXO_SEL8.nREG, 0xFFFFFFFF, 0x1F1E1F1E);
#elif defined(LVDS_JEIDA)
		//LVDS JEIDA Mode
		BITCSET(pDDICfg->LVDS_TXO_SEL0.nREG, 0xFFFFFFFF, 0x15141312);	//R5, R4, R3, R2
		BITCSET(pDDICfg->LVDS_TXO_SEL1.nREG, 0xFFFFFFFF, 0x0B0A1716);	//G3, G2, R7, R6
		BITCSET(pDDICfg->LVDS_TXO_SEL2.nREG, 0xFFFFFFFF, 0x0F0E0D0C);	//G7, G6, G5, G4
		BITCSET(pDDICfg->LVDS_TXO_SEL3.nREG, 0xFFFFFFFF, 0x05040302);	//B5, B4, B3, B2
		BITCSET(pDDICfg->LVDS_TXO_SEL4.nREG, 0xFFFFFFFF, 0x1A190706);	//VS, HS, B7, B6
		BITCSET(pDDICfg->LVDS_TXO_SEL5.nREG, 0xFFFFFFFF, 0x08111018);	//G0, R1, R0, DE
		BITCSET(pDDICfg->LVDS_TXO_SEL6.nREG, 0xFFFFFFFF, 0x1F010009);	//XX, B1, B0, G1
		BITCSET(pDDICfg->LVDS_TXO_SEL7.nREG, 0xFFFFFFFF, 0x1F1E1F1E);
		BITCSET(pDDICfg->LVDS_TXO_SEL8.nREG, 0xFFFFFFFF, 0x1F1E1F1E);
#endif

		M = 7; P = 7; S = 1; VSEL = 0;		//LVDS clock 30MHz ~ 45MHz (TCC8931 revision - TCC guide)

		TC = 5;

		BITCSET(pDDICfg->LVDS_CTRL.nREG, 0x00FFFFF0,
			(VSEL << 4) | (S << 5) | (M << 8) | (P << 15) | (TC << 21)); //LCDC1

		// LVDS Select LCDC 1
		if (pdata->lcdc_num == 1)
			BITCSET(pDDICfg->LVDS_CTRL.nREG, 0x3 << 30 , 0x1 << 30);
		else
			BITCSET(pDDICfg->LVDS_CTRL.nREG, 0x3 << 30 , 0x0 << 30);

		// LVDS reset
		pDDICfg->LVDS_CTRL.bREG.RST = 1;

		// LVDS enable
		pDDICfg->LVDS_CTRL.bREG.EN = 1;
	}
	else
	{
		//Not support power offf
	}

	//read image enhancement settings
	memset(&info, 0, sizeof(struct ie_setting_info) /sizeof(int));
	//ret = read_ie_setting(&info);

	//for debug
	print_ie_settings(&info);

	/**
	 * @author valky@cleinsoft
	 * @date 2014/09/16
	 * 3RD/4TH board TW8836 & ISL248XX init
	 * 5TH board TW9912 init
	 **/
	tw9912_init();

	#if defined(LVDS_CAM_ENABLE)
	tw8836_init();
	INFOP("%s LVDS CAM TESET MODE\n", __func__);
	#endif

	if (ret == SUCCESS_ && info.id == DAUDIO_IMAGE_ENHANCEMENT_ID)
	{
		//TCC8931 Display Device Color Enhancement
		init_tcc_ie(pdata->lcdc_num, &info);

		//TW9912 image enhancement
		init_tw9912_ie(&info);
	}

	return 0;
}

static int daudiolcd_set_backlight_level(struct lcd_panel *panel, int level)
{
	SPEWP("%s : %d\n", __func__, level);

	return 0;
}

static struct lcd_panel daudiolcd_panel_clock_33 = {
	.name			= "DAUDIOLCD",
	.manufacturer	= "AUO",
	.id				= PANEL_ID_DAUDIOLCD,
	.xres			= 800,
	.yres			= 480,
	.width			= 154,
	.height			= 85,
	.bpp			= 24,

	//mobis H/W recommend setting for AUO LCD - 140916
	//33.26MHz - 60.35Hz
	.clk_freq		= 332600,
	.clk_div		= 2,
	.bus_width		= 24,

	.lpw			= 12,		//Horizontal pulse width
	.lpc			= 800,		//Horizontal display area
	.lswc			= 48,		//Horizontal back porch
	.lewc			= 100,		//Horizontal front porch

	.vdb			= 0,
	.vdf			= 0,

	.fpw1			= 1,		//Vertical pulse width
	.flc1			= 480,		//Vertical display area
	.fswc1			= 3,		//Vertical back porch
	.fewc1			= 90,		//Vertical front porch

	.fpw2			= 1,
	.flc2			= 480,
	.fswc2			= 3,
	.fewc2			= 90,

	.sync_invert	= IV_INVERT | IH_INVERT,
	.init		= daudiolcd_panel_init,
	.set_power	= daudiolcd_set_power,
	.set_backlight_level = daudiolcd_set_backlight_level,
};

static struct lcd_panel daudiolcd_panel_3_4_th = {
	.name			= "DAUDIOLCD",
	.manufacturer	= "AUO",
	.id				= PANEL_ID_DAUDIOLCD,
	.xres			= 800,
	.yres			= 480,
	.width			= 154,
	.height			= 85,
	.bpp			= 24,

#if defined(LVDS_CLK_33)
	//mobis H/W recommend setting for TW8836 (AUO LCD) - 140830
	//33.26MHz - 60.58Hz
	.clk_freq		= 332600,
	.clk_div		= 2,
	.bus_width		= 24,

	.lpw			= 12,
	.lpc			= 800,
	.lswc			= 48,
	.lewc			= 200,

	.vdb			= 0,
	.vdf			= 0,

	.fpw1			= 1,
	.flc1			= 480,
	.fswc1			= 3,
	.fewc1			= 28,

	.fpw2			= 1,
	.flc2			= 480,
	.fswc2			= 3,
	.fewc2			= 28,
#elif defined(LVDS_CLK_45)
	//140113	45MHz - 60.02Hz
	.clk_freq		= 450000,
	.clk_div		= 2,
	.bus_width		= 24,

	.lpw			= 10,
	.lpc			= 800,
	.lswc			= 239,
	.lewc			= 400,

	.vdb			= 0,
	.vdf			= 0,

	.fpw1			= 1,
	.flc1			= 480,
	.fswc1			= 10,
	.fewc1			= 21,

	.fpw2			= 1,
	.flc2			= 480,
	.fswc2			= 10,
	.fewc2			= 21,
#elif defined(LVDS_CLK_50)	//for test
	//140113	50MHz - 59.99Hz
	.clk_freq		= 500000,
	.clk_div		= 2,
	.bus_width		= 24,

	.lpw			= 10,
	.lpc			= 800,
	.lswc			= 401,
	.lewc			= 400,

	.vdb			= 0,
	.vdf			= 0,

	.fpw1			= 1,
	.flc1			= 480,
	.fswc1			= 10,
	.fewc1			= 21,

	.fpw2			= 1,
	.flc2			= 480,
	.fswc2			= 10,
	.fewc2			= 21,
#endif

	.sync_invert	= IV_INVERT | IH_INVERT,
	.init		= daudiolcd_panel_init,
	.set_power	= daudiolcd_set_power,
	.set_backlight_level = daudiolcd_set_backlight_level,
};

struct lcd_panel *tccfb_get_panel(void)
{
	int main_ver = get_daudio_main_ver();
	switch (main_ver)
	{
		case DAUDIO_BOARD_6TH:
		case DAUDIO_BOARD_5TH:
		default:
			return &daudiolcd_panel_clock_33;
			break;
	}

	return NULL;
}
#endif
