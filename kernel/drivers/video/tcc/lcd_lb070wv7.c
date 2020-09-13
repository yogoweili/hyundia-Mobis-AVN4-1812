/*
 * lb070wv7_lcd.c -- support for TPO LB070WV7 LCD
 *
 * Copyright (C) 2009, 2010 Telechips, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/mutex.h>

#include <mach/hardware.h>
#include <mach/bsp.h>
#include <asm/io.h>
#include <asm/mach-types.h>

#include <mach/tcc_fb.h>
#include <mach/gpio.h>
#include <mach/tca_lcdc.h>
#include <mach/tca_tco.h>
#include <mach/TCC_LCD_Interface.h>

#if defined(CONFIG_ARCH_TCC893X)
#include <mach/reg_physical.h>
#endif


static struct mutex panel_lock;
static char lcd_pwr_state;

extern void lcdc_initialize(struct lcd_panel *lcd_spec, unsigned int lcdc_num);



static int lb070wv7_panel_init(struct lcd_panel *panel)
{
	printk("%s : %d\n", __func__, 0);
	return 0;
}

static int lb070wv7_set_power(struct lcd_panel *panel, int on, unsigned int lcd_num)
{
	return 0;

	struct lcd_platform_data *pdata = panel->dev->platform_data;

	if(!on && panel->bl_level)
		panel->set_backlight_level(panel , 0);
	
	mutex_lock(&panel_lock);
	panel->state = on;

	printk("%s : %d %d  lcd number = (%d) \n", __func__, on, panel->bl_level, lcd_num);

	if (on) {
		gpio_set_value(pdata->power_on, 1);
		udelay(100);

		gpio_set_value(pdata->reset, 1);
		msleep(20);

		lcdc_initialize(panel, lcd_num);
		LCDC_IO_Set(lcd_num, panel->bus_width);
		msleep(100);
	}
	else 
	{
		msleep(10);
		gpio_set_value(pdata->reset, 0);
		gpio_set_value(pdata->power_on, 0);

		LCDC_IO_Disable(lcd_num, panel->bus_width);

	}
	mutex_unlock(&panel_lock);

	if(on && panel->bl_level)
		panel->set_backlight_level(panel , panel->bl_level);	

	return 0;
}

static int lb070wv7_set_backlight_level(struct lcd_panel *panel, int level)
{
	return 0;

	#define MAX_BL_LEVEL	255	

	struct lcd_platform_data *pdata = panel->dev->platform_data;

//	printk("%s : level:%d  power:%d \n", __func__, level, panel->state);
	
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
			#if  defined(CONFIG_ARCH_TCC893X)
				tca_tco_pwm_ctrl(0, pdata->bl_on, MAX_BACKLIGTH, level);
			#else
				printk("%s :  Do not support chipset !!!\n",__func__);
			#endif//
		}
	}

 	mutex_unlock(&panel_lock);

	return 0;
}

static struct lcd_panel lb070wv7_panel = {
	.name		= "LB070WV7",
	.manufacturer	= "INNOLUX",
	.id		= PANEL_ID_LB070WV7,
	.xres		= 800,
	.yres		= 480,
	.width		= 154,
	.height		= 85,
	.bpp		= 24,
	
	.clk_freq	= 321800, //310000,
	.clk_div	= 2,
	.bus_width	= 24,
	.lpw		= 0, //10,
	.lpc		= 800,
	.lswc		= 100, //34,
	.lewc		= 100, //130,
	.vdb		= 0,
	.vdf		= 0,

	.fpw1		= 0,
	.flc1		= 480,
	.fswc1		= 20,
	.fewc1		= 20, //21,

	.fpw2		= 0,
	.flc2		= 480,
	.fswc2		= 20,
	.fewc2		= 20, //21,
	
	//.sync_invert	= IV_INVERT | IH_INVERT,

	.sync_invert	= 0,
	.init		= lb070wv7_panel_init,
	.set_power	= lb070wv7_set_power,
	.set_backlight_level = lb070wv7_set_backlight_level,
};

static int lb070wv7_probe(struct platform_device *pdev)
{
	printk("%s : %d\n", __func__, 0);

	mutex_init(&panel_lock);

	lb070wv7_panel.state = 1;
	lb070wv7_panel.dev = &pdev->dev;
	
	tccfb_register_panel(&lb070wv7_panel);
	return 0;
}

static int lb070wv7_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver lb070wv7_lcd = {
	.probe	= lb070wv7_probe,
	.remove	= lb070wv7_remove,
	.driver	= {
		.name	= "lb070wv7_lcd",
		.owner	= THIS_MODULE,
	},
};

static __init int lb070wv7_init(void)
{
	printk("~ %s ~ \n", __func__);
	return platform_driver_register(&lb070wv7_lcd);
}

static __exit void lb070wv7_exit(void)
{
	platform_driver_unregister(&lb070wv7_lcd);
}

subsys_initcall(lb070wv7_init);
module_exit(lb070wv7_exit);

MODULE_DESCRIPTION("LB070WV7 LCD driver");
MODULE_LICENSE("GPL");
