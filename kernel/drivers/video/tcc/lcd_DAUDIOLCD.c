/*
 * lcd_DAUDIOLCD.c -- support for DAUDIOLCD LVDS Panel
 *
 * Copyright (C) 2009, 2010 Telechips, Inc.
 * Copyright (C) 2013 Cleinsoft, Inc.
 *
 * @author daudio@cleinsoft
 * @date 2013/07/19
 * DAUDIOLCD Kernel driver, origin is from FLD0800 driver
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <asm/io.h>

#include <linux/clk.h>
#include <linux/fs.h>
#include <linux/platform_device.h>

#include <mach/globals.h>
#include <mach/gpio.h>
#include <mach/tcc_fb.h>
#include <mach/tca_lcdc.h>
#include <mach/reg_physical.h>

#include <mach/daudio.h>
#include <mach/daudio_debug.h>
#include <mach/daudio_info.h>

#define	LVDS_VESA

#define LVDS_CLK_33		//D-Audio 5th
//#define LVDS_CLK_45		//D-Audio 3rd, 4th
//#define LVDS_CLK_50	//for temperature test

//fake gpio setting. TCC_GPA(4) not use.
#define GPIO_POWER_ON	TCC_GPA(4)
#define GPIO_DISPLAY_ON	TCC_GPA(4)
#define GPIO_BL_ON		TCC_GPA(4)
#define GPIO_RESET		TCC_GPA(4)

static int debug_daudio_lvds = DEBUG_DAUDIO_LVDS;

#define VPRINTK(fmt, args...) if (debug_daudio_lvds) printk(KERN_INFO TAG_DAUDIO_LVDS fmt, ##args)

#define PLATFORM_DRIVER_NAME	"daudio_lvds"

static struct mutex panel_lock;
static char lcd_pwr_state;
static unsigned int lcd_bl_level;
extern void lcdc_initialize(struct lcd_panel *lcd_spec, unsigned int lcdc_num);
static struct clk *lvds_clk;

unsigned int lvds_stby;
unsigned int lvds_power;

static ie_setting_info ie_info;

extern int read_ie_settings(ie_setting_info *info);
extern int write_ie_settings(ie_setting_info *info);
extern void print_ie_info(ie_setting_info *info);

extern int tw8836_write_ie(int cmd, unsigned char value);
static int daudio_set_settings(int mode, char level);
static int daudio_set_ie_level(int mode, char level);

static int daudio_lvds_panel_init(struct lcd_panel *panel)
{
	VPRINTK("%s\n", __func__);

	return 0;
}

static int daudio_lvds_set_power(struct lcd_panel *panel, int on, unsigned int lcd_num)
{
	PDDICONFIG	pDDICfg;
	unsigned int P, M, S, VSEL, TC;
	int main_ver = 0;

	VPRINTK("%s\n", __func__);

	mutex_lock(&panel_lock);
	panel->state = on;

	pDDICfg = (volatile PDDICONFIG)tcc_p2v(HwDDI_CONFIG_BASE);
	if (on) {
		// LVDS power on
		clk_enable(lvds_clk);

		lcdc_initialize(panel, lcd_num);

		//LVDS 6bit setting for internal dithering option !!!
		//tcc_lcdc_dithering_setting(lcd_num);

		// LVDS reset
		pDDICfg->LVDS_CTRL.bREG.RST = 1;
		pDDICfg->LVDS_CTRL.bREG.RST = 0;

#if defined(LVDS_VESA)
		/**
		 * @author valky@cleinsoft
		 * @date 2013/08/20
		 * LVDS VESA Mode
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
#else
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

		main_ver = daudio_main_version();

		M = 7; P = 7; S = 1; VSEL = 0;		//LVDS clock 30MHz ~ 45MHz (TCC8931 revision - TCC guide)
		TC = 5;

		BITCSET(pDDICfg->LVDS_CTRL.nREG, 0x00FFFFF0, (VSEL<<4)|(S<<5)|(M<<8)|(P<<15)|(TC<<21)); //LCDC1

		// LVDS Select LCDC 1
		if(lcd_num)
			BITCSET(pDDICfg->LVDS_CTRL.nREG, 0x3 << 30 , 0x1 << 30);
		else
			BITCSET(pDDICfg->LVDS_CTRL.nREG, 0x3 << 30 , 0x0 << 30);

		pDDICfg->LVDS_CTRL.bREG.RST = 1;	// reset
		pDDICfg->LVDS_CTRL.bREG.EN = 1;		// enable
		//msleep(100);
	}
	else
	{
		pDDICfg->LVDS_CTRL.bREG.RST = 1;	// reset
		pDDICfg->LVDS_CTRL.bREG.EN = 0;		// reset

		clk_disable(lvds_clk);
	}
	mutex_unlock(&panel_lock);

	if (lcd_pwr_state)
		panel->set_backlight_level(panel , lcd_bl_level);

	return 0;
}

static int daudio_lvds_set_backlight_level(struct lcd_panel *panel, int level)
{
	VPRINTK("%s level: %d\n", __func__, level);

#if defined(CONFIG_DAUDIO)
	if (level != 0 && panel->state)
	{
		int ret = daudio_set_ie_level(SET_BRIGHTNESS, (char)level);
		if (ret == SUCCESS_I2C)
		{
			ret = daudio_set_settings(SET_BRIGHTNESS, level);
		}
	}

#else	//valky@cleinsoft - D-Audio LVDS driver not support LCD backlight control.
	#define MAX_BL_LEVEL	255
	struct lcd_platform_data *pdata = panel->dev->platform_data;
	//printk("%s : level:%d  power:%d \n", __func__, level, panel->state);
	mutex_lock(&panel_lock);
	panel->bl_level = level;

	#define MAX_BACKLIGTH 255
	if (level == 0) {
		tca_tco_pwm_ctrl(0, pdata->bl_on, MAX_BACKLIGTH, level);
	}
	else
	{
		if(panel->state)
		{
			tca_tco_pwm_ctrl(0, pdata->bl_on, MAX_BACKLIGTH, level);
		}
	}

	mutex_unlock(&panel_lock);
#endif

	return 0;
}

#if 1  /* For the Android standard backlight control.
		* D-Audio not use this functions. */
static int init_ie_settings(void)
{
	int ret = FAIL;

	if (ie_info.id == (unsigned int)DAUDIO_IE_SETTINGS_ID)
		return SUCCESS;

	//read IE settings
	ret = read_ie_settings(&ie_info);
	if (ret == SUCCESS)
	{
		print_ie_info(&ie_info);

		if (ie_info.version == 0)
		{
			//ret = FAIL_NO_SETTING;
		}

		//TODO: compare version
		//overwrite id and version
		ie_info.id		= DAUDIO_IE_SETTINGS_ID;
		ie_info.version	= DAUDIO_IE_SETTINGS_VERSION;
	}
	else
	{
		VPRINTK("%s setting read failed!\n", __func__);
	}

	return ret;
}

static int daudio_set_settings(int mode, char level)
{
	int ret = init_ie_settings();
	if (ret == SUCCESS)
	{
		int count = mode - SET_BRIGHTNESS;
		int size_id = sizeof(ie_info.id) / 4;
		int size_version = sizeof(ie_info.version) / 4;
		int *plevel = (int *)(&ie_info);
		plevel[size_id + size_version + count] = level;

		ret = write_ie_settings(&ie_info);
	}

	return ret;
}

static int daudio_set_ie_level(int mode, char level)
{
	int ret = FAIL;

	mutex_lock(&panel_lock);
	ret = tw8836_write_ie(mode, level);
	mutex_unlock(&panel_lock);

	return ret;
}
#endif

static void print_debug(void)
{
	printk(KERN_INFO "debug usage\n");
	printk(KERN_INFO "echo [debug] > debug\n");
	printk(KERN_INFO "[debug] 0: Disable, 1: Enable\n");
	printk(KERN_INFO "D-Audio LVDS driver debug state: %s\n",
			debug_daudio_lvds == 0 ? "Disable" : "Enable");
}

static ssize_t show_debug(struct device *dev, struct device_attribute *attr, char *buf)
{
	print_debug();
	return sprintf(buf, "%d\n", debug_daudio_lvds);
}

static ssize_t store_debug(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int debug = 0;

	sscanf(buf, "%d", &debug);
	debug_daudio_lvds = debug;

	printk(KERN_INFO "D-Audio LVDS driver debug state: %s\n",
			debug_daudio_lvds == 0 ? "Disable" : "Enable");

	return count;
}

static DEVICE_ATTR(debug, 0664, show_debug, store_debug);

struct lcd_panel daudio_lvds_panel_clock_33 = {
	.name           = "DAUDIOLCD",
	.manufacturer   = "AUO",
	.id             = PANEL_ID_DAUDIOLCD,
	.xres           = 800,
	.yres           = 480,
	.width          = 154,
	.height         = 85,
	.bpp            = 24,

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
	.init		= daudio_lvds_panel_init,
	.set_power	= daudio_lvds_set_power,
	.set_backlight_level = daudio_lvds_set_backlight_level,
};

struct lcd_panel daudio_lvds_panel_3_4_th = {
	.name			= "DAUDIOLCD",
	.manufacturer	= "AUO",
	.id				= PANEL_ID_DAUDIOLCD,
	.xres       	= 800,
    .yres       	= 480,
    .width      	= 154,
    .height     	= 85,
    .bpp        	= 24,

#if defined(LVDS_CLK_33)
	//mobis H/W recommend setting for AUO LCD - 140830
	//33.25MHz - 60.58Hz
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
	//140113    45MHz - 60.02Hz
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
#elif defined(LVDS_CLK_50)
	//140113    50MHz - 59.99Hz
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
	.init			= daudio_lvds_panel_init,
	.set_power		= daudio_lvds_set_power,
	.set_backlight_level = daudio_lvds_set_backlight_level,
};

static int daudio_lvds_probe(struct platform_device *pdev)
{
	int main_ver = daudio_main_version();
	struct lcd_panel *daudio_lvds_panel;

	VPRINTK("%s %s\n", __func__,  pdev->name);
	mutex_init(&panel_lock);
	lcd_pwr_state = 1;

	switch (main_ver)
	{
		case DAUDIO_BOARD_5TH:
		case DAUDIO_BOARD_6TH:
		default:
			daudio_lvds_panel = &daudio_lvds_panel_clock_33;
			break;
	}

	daudio_lvds_panel->state = 1;
	daudio_lvds_panel->dev = &pdev->dev;
	lvds_clk = clk_get(0, "lvds_phy"); //8920
	if (IS_ERR(lvds_clk))
		lvds_clk = NULL;
	clk_enable(lvds_clk);	
	tccfb_register_panel(daudio_lvds_panel);

	return 0;
}

static int daudio_lvds_remove(struct platform_device *pdev)
{
	VPRINTK("%s\n", __func__);
	return 0;
}

static struct platform_driver daudio_lvds_driver = {
	.probe		= daudio_lvds_probe,
	.remove		= daudio_lvds_remove,
	.driver	= {
		.name	= PLATFORM_DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
};

static struct lcd_platform_data lvds_pdata = {
	.power_on	= GPIO_POWER_ON,
	.display_on	= GPIO_DISPLAY_ON,
	.bl_on		= GPIO_BL_ON,
	.reset		= GPIO_RESET,
};

static struct platform_device daudio_lvds_device = {
	.name	= PLATFORM_DRIVER_NAME,
	.dev    = {
		.platform_data	= &lvds_pdata,
	},
};

struct platform_device* get_daudio_lvds_platform_device(void)
{
	return &daudio_lvds_device;
}

static __init int daudio_lvds_init(void)
{
	int ret = -EIO;
	struct platform_device *pdev = get_daudio_lvds_platform_device();

	VPRINTK("%s\n", __func__);

	if (pdev == NULL) goto fail_platform_driver_register;

	ret = platform_device_register(pdev);
	if (ret)
	{
		printk(KERN_ERR "%s failed to platform_device_register\n", __func__);
		goto fail_platform_device_register;
	}

	ret = platform_driver_register(&daudio_lvds_driver);
	if (ret < 0)
	{
		printk(KERN_ERR "%s failed to platform_driver_register\n", __func__);
		goto fail_platform_driver_register;
	}

	ret = device_create_file(&pdev->dev, &dev_attr_debug);
	if (ret)
	{
		printk(KERN_ERR "failed to create daudio device: %d\n", ret);
		goto fail_device_create_file;
	}

	VPRINTK("%s success to init\n", __func__);
	return ret;

fail_device_create_file:
	platform_driver_unregister(&daudio_lvds_driver);
fail_platform_driver_register:
	platform_device_unregister(pdev);
fail_platform_device_register:
	return ret;
}

static __exit void daudio_lvds_exit(void)
{
	struct platform_device *pdev = get_daudio_lvds_platform_device();

	device_remove_file(&pdev->dev, &dev_attr_debug);
	platform_driver_unregister(&daudio_lvds_driver);
	platform_device_unregister(pdev);
}

subsys_initcall(daudio_lvds_init);
module_exit(daudio_lvds_exit);

MODULE_AUTHOR("Cleinsoft");
MODULE_DESCRIPTION("DAUDIO LVDS driver");
MODULE_LICENSE("GPL");
