/****************************************************************************
 *   FileName    : lcd_tw8816.c
 *   Description :  support for Techwell TW8816 LCD
 ****************************************************************************
 *
 *   TCC Version 1.0
 *   Copyright (c) Telechips Inc.
 *   All rights reserved 
 
This source code contains confidential information of Telechips.
Any unauthorized use without a written permission of Telechips including not limited to re-distribution in source or binary form is strictly prohibited.
This source code is provided "AS IS" and nothing contained in this source code shall constitute any express or implied warranty of any kind, including without limitation, any warranty of merchantability, fitness for a particular purpose or non-infringement of any patent, copyright or other third party intellectual property right. No warranty is made, express or implied, regarding the information's accuracy, completeness, or performance. 
In no event shall Telechips be liable for any claim, damages or other liability arising from, out of or in connection with this source code or the use in the source code. 
This source code is provided subject to the terms of a Mutual Non-Disclosure Agreement between Telechips and Company.
*
****************************************************************************/
#include <debug.h>
#include <stdlib.h>
#include <string.h>
#include <tcc_lcd.h>

#ifdef TW8816
#include <i2c.h>
#include <dev/gpio.h>
#include "tw8816_reg_Init.h"

#define KITLOutputDebugString(msg...)		printf(msg)

void TW88_Delay(int uDelay)
{
	unsigned int uCnt;

	uCnt = uDelay * 1000;
	while(uCnt--);

}

void WriteTW88(unsigned char uSlaveAddr,unsigned char uIndex,unsigned char uData)
{

	char i2c_write_buf[2];

	i2c_write_buf[0] = uIndex;
	i2c_write_buf[1] = uData;
	
	i2c_xfer(uSlaveAddr, 2, (unsigned char *)i2c_write_buf, 0, 0, I2C_CORE0_CH0);

}

unsigned char ReadTW88(unsigned char uIndex)
{
	unsigned char uData;

	
	i2c_xfer(TW8816_I2C_ADDR, 0, 0, 1, &uData, I2C_CORE0_CH0);


	return uData;

}

void XscaleU(unsigned int scale)
{
	unsigned char val;

	WriteTW88(TW8816_I2C_ADDR, 0x69, (unsigned char)scale);
	KITLOutputDebugString(" TW8816 SCAL WRITE = [0x69]=[0x%X] \n ", (unsigned char)scale);	
	scale >>= 8;
	WriteTW88(TW8816_I2C_ADDR,0x60, (unsigned char)scale);
	KITLOutputDebugString(" TW8816 SCAL WRITE = [0x60]=[0x%X] \n ", (unsigned char)scale);	
	scale >>= 8;
	val = ReadTW88(0x63);
	val &= 0xfe;
	val |= (unsigned char)scale;
	WriteTW88(TW8816_I2C_ADDR,0x63, val );
	KITLOutputDebugString(" TW8816 SCAL WRITE = [0x63]=[0x%X] \n ", val);	
}

void XscaleD(unsigned int  scale)
{
	unsigned char val;

	WriteTW88(TW8816_I2C_ADDR,0x61, (unsigned char)scale);
	KITLOutputDebugString(" TW8816 SCAL WRITE = [0x61]=[0x%X] \n ", (unsigned char)scale);	
	scale >>= 8;
	scale <<= 1;
	val = ReadTW88(0x63);
	val &= 0xfd;
	val |= (unsigned char)scale;
	WriteTW88(TW8816_I2C_ADDR,0x63, val );
	
	KITLOutputDebugString(" TW8816 SCAL WRITE = [0x63]=[0x%X] \n ",val);	
}

void XScale2(unsigned int  scale)
{
	if( scale==0x10000 ) {		// No Scale
		XscaleU(0x10000);
		XscaleD(0x80);
	}
	else if( scale<0x10000 ) {	// Up Scale
		XscaleU(scale);
		XscaleD(0x80);
	}
	else {						// Down Scale
		XscaleU(0x10000);
		XscaleD(scale/2/256);
	}
}

void YScale2(unsigned int  scale)
{
	unsigned char val;

	WriteTW88(TW8816_I2C_ADDR,0x6a, (unsigned char)(scale));
	KITLOutputDebugString(" TW8816 SCAL WRITE = [0x6a]=[0x%X] \n ", (unsigned char)scale);	
	scale >>= 8;
	WriteTW88(TW8816_I2C_ADDR,0x62, (unsigned char)(scale));
	KITLOutputDebugString(" TW8816 SCAL WRITE = [0x62]=[0x%X] \n ", (unsigned char)scale);	
	scale >>= 8;
	scale <<= 2;
	val = ReadTW88(0x63) & 0xf3;
	val |= (unsigned char)scale;
	WriteTW88(TW8816_I2C_ADDR,0x63, val );
	KITLOutputDebugString(" TW8816 SCAL WRITE = [0x63]=[0x%X] \n ", val);	
}
void tca_tw8816_PanelProcessor_Init(void)
{

	int uCnt = 0;
	
	KITLOutputDebugString(" TW8816 RGB Init \n");
	for (uCnt=0 ; uCnt<250 ; uCnt++)
	{
		if (TW8816_Init_Data[uCnt][0] == 0xFF && TW8816_Init_Data[uCnt][1] == 0xFF) 
			break;
		WriteTW88(TW8816_I2C_ADDR,TW8816_Init_Data[uCnt][0], TW8816_Init_Data[uCnt][1]);
		TW88_Delay(100);
		
	}
	KITLOutputDebugString(" TW8816 RGB finish %d  \n", uCnt);

#if 0
	
	{
		unsigned int scale=0;
		unsigned int width=800;//624;
		unsigned int height=480;

		
		scale = width;
		scale *= 0x10000;
		scale /= 800;			// LCD width;
		XScale2(scale);
		KITLOutputDebugString(" H scale: %xh - %xh\n", (unsigned short)(scale>>8), (unsigned short)(scale & 0xff));


		scale = height;
		scale *= 0x10000;
		scale /= 480;			// LCD hight;
		YScale2(scale);
		
		KITLOutputDebugString(" V scale: %xh - %xh\n", (unsigned short)(scale>>8), (unsigned short)(scale & 0xff));
	
	
	}
#endif//
}


static int tw8816_panel_init(struct lcd_panel *panel)
{
	struct lcd_platform_data *pdata = &(panel->dev);

	printf("%s : %d  \n", __func__, 0);

	tcclcd_gpio_config(pdata->display_on, 1);
	tcclcd_gpio_config(pdata->bl_on, 1);
	tcclcd_gpio_config(pdata->reset, 1);
	tcclcd_gpio_config(pdata->power_on, 1);

	return 0;
}

static int tw8816_set_power(struct lcd_panel *panel, int on)
{
	struct lcd_platform_data *pdata = &(panel->dev);

	printf("%s : %d \n", __func__, on);
	if (on) {
		tcclcd_gpio_set_value(pdata->power_on, 1);
		tcclcd_gpio_set_value(pdata->reset, 1);
		mdelay(10);
		tca_tw8816_PanelProcessor_Init();
		mdelay(10);

		lcdc_initialize(pdata->lcdc_num, panel);
		
		LCDC_IO_Set(pdata->lcdc_num, panel->bus_width);

		mdelay(16);
		
		tcclcd_gpio_set_value(pdata->display_on, 1);

	}
	else 
	{
		tcclcd_gpio_set_value(pdata->display_on, 0);
		mdelay(10);
		tcclcd_gpio_set_value(pdata->reset, 0);
		tcclcd_gpio_set_value(pdata->power_on, 0);
		LCDC_IO_Disable(pdata->lcdc_num, panel->bus_width);
	}

	return 0;
}




static int tw8816_set_backlight_level(struct lcd_panel *panel, int level)
{

	return 0;
}

/*		VSYNC = LOW, HSYNC = LOW, DE = HIGH, PCLK=Faling 			*/	
/*		HSYNC Period = 928,			
		HSYNC Low width = 48
		Horizontal Display Period = 800
		Horizontal Back porch = 88
		Horizontal Front porch = 40

		VSYNC(Frame) Period = 525
		VSYNC Low width = 3
		Vertical Display Period = 480
		Vertical Back porch = 32
		Vertical Front porch = 13
*/

static struct lcd_panel tw8816_panel = {
	.name		= "TW8816",
	.manufacturer	= "Techwell",
	.id			= 	PANEL_ID_TW8816,
	
	.xres		= 800,
	.yres		= 480,
	.width		= 76,
	.height		= 48,
	.bpp			= 32,
	.clk_freq		= 292320,
	.clk_div		= 2,
	.bus_width	= 24,
	.lpw			= 47,
	.lpc			= 800,
	.lswc		= 87,
	.lewc		= 39,
	.vdb			= 0,
	.vdf			= 0,
	.fpw1		= 3,
	.flc1			= 480,
	.fswc1		= 32,
	.fewc1		= 13,
	.fpw2		= 3,
	.flc2			= 480,
	.fswc2		= 32,
	.fewc2		= 13,
	.sync_invert	=  IV_INVERT | IH_INVERT | IP_INVERT,

	.init		= tw8816_panel_init,
	.set_power	= tw8816_set_power,
	.set_backlight_level = tw8816_set_backlight_level,
};

struct lcd_panel *tccfb_get_panel(void)
{
	return &tw8816_panel;
}
#endif//DX08D11VM0AAA
