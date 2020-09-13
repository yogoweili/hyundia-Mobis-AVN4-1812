/*
 * linux/arch/arm/mach-tcc893x/vioc_lut.c
 * Author:  <linux@telechips.com>
 * Created: June 10, 2008
 * Description: TCC VIOC h/w block 
 *
 * Copyright (C) 2008-2009 Telechips
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <linux/kernel.h>
#include <linux/module.h>

#include <asm/io.h>
#include <mach/vioc_lut.h>
#include <mach/globals.h>
#include <linux/slab.h>

#if 1
#define dprintk(msg...)	 { printk(KERN_INFO "tca_hdmi: " msg); }
#else
#define dprintk(msg...)	 
#endif
#define LUT_TABLE_SIZE	(256)

#define VIOC_DISPLAY_LUT_MAX 	VIOC_LUT_DEV1
unsigned int DisplayLutBackUp[VIOC_DISPLAY_LUT_MAX + 1][LUT_TABLE_SIZE];

#define VIOC_DMA_LUT_START		VIOC_LUT_COMP0
#define VIOC_DMA_LUT_MAX		VIOC_LUT_COMP3
unsigned int DmaLutBackUp[VIOC_LUT_COMP3 - VIOC_LUT_COMP0 + 1][LUT_TABLE_SIZE];

void VIOC_LUT_Set_LutBackUp_Init(void)
{
	unsigned int i, j;
	for (i = 0; i < LUT_TABLE_SIZE; i++)
	{
		for (j = 0; j <= VIOC_DISPLAY_LUT_MAX; j++)
		{
			DisplayLutBackUp[j][i]= i<<16 | i << 8| i;
		}
	}
}

void VIOC_DMA_LutBackUp_Init(void)
{
	unsigned int i, j;
	for (i = 0; i < LUT_TABLE_SIZE; i++)
	{
		for (j = VIOC_DMA_LUT_START; j <= VIOC_DMA_LUT_MAX; j++)
		{
			DmaLutBackUp[j - VIOC_DMA_LUT_START][i]= i<<16 | i << 8 | i;
		}
	}
}

void VIOC_LUT_Set_value(VIOC_LUT *pLUT, unsigned int select,  unsigned int *LUT_Value)
{
	unsigned int *pLUT_TABLE;
	unsigned int i;
	dprintk("[%s select : %d ]\n", __func__ , select);

	pLUT_TABLE = (unsigned int *)tcc_p2v(HwVIOC_LUT_TAB);
	BITCSET(pLUT->uSEL.nREG, 0x0000000F, select);

	memcpy(pLUT_TABLE, LUT_Value, LUT_TABLE_SIZE * 4);

	//backup display lut up table
	if (select <= VIOC_DISPLAY_LUT_MAX)
		memcpy(DisplayLutBackUp[select], LUT_Value, LUT_TABLE_SIZE * 4);
	else if ((select >= VIOC_DMA_LUT_START) &&(select <= VIOC_DMA_LUT_MAX))
	{
		memcpy(DmaLutBackUp[select - VIOC_DMA_LUT_START], LUT_Value, LUT_TABLE_SIZE * 4);
	}
}

void VIOC_LUT_Set (VIOC_LUT *pLUT, unsigned int select, unsigned int inv, unsigned int bright_value)
{
	unsigned int cnt, c;
	unsigned int pLUT_TABLE[LUT_TABLE_SIZE];

	dprintk("[%s ] select=[%d] inv=[%d] \n",__func__,select, inv );

	if (inv == LUT_NORMAL)
	{
#if 1
		for (cnt = 0 ; cnt < 256 ; cnt++)
		{
			pLUT_TABLE[cnt] = (cnt << 16) | (cnt << 8) | (cnt << 0);
		}
#else /* If you need gamma correction, you must make your own LUT for Gamma Curve */
		for (cnt = 0 ; cnt <64; cnt++)
		{
			c=cnt/2;
			pLUT_TABLE[cnt] = (c << 16) | (c << 8) | (c << 0);
		}
		for (; cnt <192;cnt++)
		{
			c=cnt*2-64;
			pLUT_TABLE[cnt] = (c << 16) | (c << 8) | (c << 0);
		}
		for (; cnt <256 ; cnt++)
		{
			c=(cnt/2+128)%256;
			pLUT_TABLE[cnt] = (c << 16) | (c << 8) | (c << 0);
		}
#endif
	}
	else if (inv == LUT_BRIGHTNESS)
	{
		for(cnt = 0; cnt < 256; cnt++)
		{
			c = (unsigned int)cnt + bright_value;

			if (c >= 255)
				c = 255;

			pLUT_TABLE[cnt] = (c << 16) | (c << 8) | (c << 0);
		}
	}
	else	//invert
	{
		for (cnt = 0; cnt < 256; cnt++)
		{
			pLUT_TABLE[cnt] = ~((cnt << 16) | (cnt << 8) | (cnt << 0));
		}
	}

	VIOC_LUT_Set_value(pLUT,  select, pLUT_TABLE);
}

void VIOC_LUT_Plugin (VIOC_LUT *pLUT, unsigned int lut_sel, unsigned int plug_in)
{
	dprintk("[%s ] lut_sel=[%d] plug_in=[%d] \n",__func__,lut_sel, plug_in );

	switch (lut_sel)
	{
		case (VIOC_LUT_DEV0):
			pLUT->uDEV0CFG.bREG.SEL	= plug_in;
			break;

		case (VIOC_LUT_DEV1):
			pLUT->uDEV1CFG.bREG.SEL	= plug_in;
			break;

		case (VIOC_LUT_DEV2):
			pLUT->uDEV2CFG.bREG.SEL	= plug_in;
			break;

		case (VIOC_LUT_COMP0):
			//pLUT->uCOMP0CFG.bREG.SEL= plug_in;
			BITCSET(pLUT->uCOMP0CFG.nREG, 0x000000FF, plug_in);
			break;

		case (VIOC_LUT_COMP1):
			//pLUT->uCOMP1CFG.bREG.SEL= plug_in;
			BITCSET(pLUT->uCOMP1CFG.nREG, 0x000000FF, plug_in);
			break;

		case (VIOC_LUT_COMP2):
			BITCSET(pLUT->uCOMP2CFG.nREG, 0x000000FF, plug_in);
			break;

		case (VIOC_LUT_COMP3):
			BITCSET(pLUT->uCOMP3CFG.nREG, 0x000000FF, plug_in);
			break;

		default:
			break;
	}
}

void VIOC_LUT_Enable (VIOC_LUT *pLUT, unsigned int lut_sel, unsigned int enable)
{
	dprintk("[%s ] lut_sel=[%d] enable=[%d] \n", __func__, lut_sel, enable);

	switch (lut_sel)
	{
		case (VIOC_LUT_DEV0):
			pLUT->uDEV0CFG.bREG.EN	= enable;
			break;

		case (VIOC_LUT_DEV1):
			pLUT->uDEV1CFG.bREG.EN	= enable;
			break;

		case (VIOC_LUT_DEV2):
			pLUT->uDEV2CFG.bREG.EN = enable;
			break;

		case (VIOC_LUT_COMP0):
			BITCSET(pLUT->uCOMP0CFG.nREG, 0x80000000, enable << 31);
			break;

		case (VIOC_LUT_COMP1):
			BITCSET(pLUT->uCOMP1CFG.nREG, 0x80000000, enable << 31);
			break;

		case (VIOC_LUT_COMP2):
			BITCSET(pLUT->uCOMP2CFG.nREG, 0x80000000, enable << 31);
			break;

		case (VIOC_LUT_COMP3):
			BITCSET(pLUT->uCOMP3CFG.nREG, 0x80000000, enable << 31);
			break;

		default:
			break;
	}
}

void VIOC_LUT_SetMain(VIOC_LUT *pLUT, unsigned int sel, unsigned int plugin, unsigned int inv, unsigned int bright_value, unsigned int onoff)
{
	dprintk("[%s ] sel=[%d] plugin=[%d] inv=[%d] bright_value=[%d] onoff=[%d] \n",
			__func__,sel, plugin, inv, bright_value, onoff);

	if(onoff){
		VIOC_LUT_Set(pLUT, sel, inv, bright_value);
		VIOC_LUT_Plugin(pLUT, sel, plugin);
		VIOC_LUT_Enable(pLUT, sel, TRUE);
	}
	else
	{
		VIOC_LUT_Set(pLUT, sel, inv, 0);
		VIOC_LUT_Plugin(pLUT, sel, plugin);
		VIOC_LUT_Enable(pLUT, sel, FALSE);
	}
}

void VIOC_LUT_ReStore(VIOC_LUT *pLUT, unsigned int select)
{
	if (select <= VIOC_DISPLAY_LUT_MAX)
	{
		dprintk("[%s select : %d ]\n", __func__ , select);
		VIOC_LUT_Set_value(pLUT, select,  DisplayLutBackUp[select]);
		VIOC_LUT_Enable (pLUT, select, 1);
	}
}

void VIOC_DMA_LUT_restore(void)
{
	int j;
	VIOC_LUT *pLUT =(VIOC_LUT*)tcc_p2v(HwVIOC_LUT);
	
	for (j = VIOC_DMA_LUT_START; j <= VIOC_DMA_LUT_MAX; j++)
	{
		VIOC_LUT_Set_value(pLUT, j,  DmaLutBackUp[j - VIOC_DMA_LUT_START]);
	}
}
//#define HUE_TIME_MEASURE

#ifdef HUE_TIME_MEASURE
#define AVERAGE_MAX_CNT 100

unsigned int average_count;
static long total_dt[AVERAGE_MAX_CNT];

inline long myclock(void)
{
	struct timeval tv;
	do_gettimeofday (&tv);
	return (tv.tv_sec * 1000000 + tv.tv_usec);
}
#endif//

unsigned int *hue_table;

unsigned int * tcc_hue_table_alloc(void)
{
	unsigned int size = 256 * 256 * 2;
	hue_table = kzalloc(size , GFP_KERNEL);
	printk("%s 0x%p size :%d \n", __func__, hue_table, size);

	return hue_table;
}

unsigned int tcc_hue_table_free(void)
{
	printk("%s 0x%p \n", __func__, hue_table);
	if(hue_table)
		kzfree(hue_table);

	return 0;
}

void tcc_sw_hue_table_set(int v_sin, int v_cos, int saturation)
{
	int change_cr = 0, change_cb = 0;				
	int  cb  = 0, cr = 0;
	unsigned short *hue_set_v;

#ifdef HUE_TIME_MEASURE
	long t, dt;
	t = myclock();
#endif//

	if (hue_table == NULL)
		return ;

	hue_set_v = (unsigned short *)hue_table;

	printk("%s   saturation:%d v_sin:%d v_cos:%d \n", __func__, saturation , v_sin, v_cos);

	for (cr = 0; cr <=0xff; cr++)
	{
		for (cb =0 ; cb <= 0xff; cb++)
		{
			change_cb = (int)(cb * v_cos + cr * v_sin);
			change_cr = (int)(cr * v_cos - cb * v_sin);

#if 0
			change_cb  = change_cb/1000;
			change_cr  = change_cr/1000;
#else
			change_cb  = (((change_cb) - 128000)*saturation)/10000 + 128;
			change_cr  = (((change_cr) - 128000)*saturation)/10000 + 128;
#endif//

			if (change_cb > 255) change_cb = 255;
			if (change_cb < 0) change_cb = 0;

			if (change_cr > 255) change_cr = 255;
			if (change_cr < 0) change_cr = 0;

			*hue_set_v++  = (((change_cr & 0xff ) << 8)|(change_cb & 0xff)) & 0xFFFF;
		}
	}

#ifdef HUE_TIME_MEASURE
	dt = myclock() - t;
	printk("--- %d.%d msec  \n",(int)( dt / 1000),(int)( dt % 1000));
#endif
}

extern void update_cbcr(unsigned short *src_vcbcr_addr, unsigned int width, unsigned int height, short *p_cbcr_table); 

int tcc_sw_hue_table_opt(unsigned int src_phy_cbcr, unsigned int tar_phy_cbcr, unsigned int width, unsigned int height)
{
	unsigned short *src_vcbcr_addr, *src_vcbcr_base_addr, *tar_vcbcr_addr, *tar_vcbcr_base_addr;
	unsigned short int_cbcr;
	unsigned int loop = 0;
	unsigned short *p_cbcr_table;

#ifdef HUE_TIME_MEASURE
	long t, dt;
	t = myclock();
#endif

	if (src_phy_cbcr == tar_phy_cbcr)
	{
		src_vcbcr_base_addr = src_vcbcr_addr = ioremap_nocache(src_phy_cbcr, width * height);
		if ((src_vcbcr_addr == NULL))
			return -ENOMEM;

		update_cbcr(src_vcbcr_addr, width, height, (short *)hue_table);

		iounmap(src_vcbcr_base_addr);
	}
	else
	{
		src_vcbcr_base_addr = src_vcbcr_addr = ioremap_nocache(src_phy_cbcr, width * height);
		tar_vcbcr_base_addr = tar_vcbcr_addr = ioremap_nocache(tar_phy_cbcr, width * height);

		if ((src_vcbcr_addr == NULL) || (tar_vcbcr_addr == NULL))
			return -ENOMEM;

		loop = height * width/8;
		p_cbcr_table = (short *)hue_table;

		while (loop--)
		{
			int_cbcr = *src_vcbcr_addr++;
			*tar_vcbcr_addr++ = (p_cbcr_table[int_cbcr & 0xFFFF] & 0xFFFF) |
				((p_cbcr_table[(int_cbcr >> 16) & 0xFFFF] << 16) & 0xFFFF0000);
		}

		iounmap(src_vcbcr_base_addr);
		iounmap(tar_vcbcr_base_addr);
	}

#ifdef HUE_TIME_MEASURE
	dt = myclock() - t;

	if (average_count == AVERAGE_MAX_CNT) {
		int i;
		long sum_dt = 0;
		average_count = 0;
		for (i = 0; i < AVERAGE_MAX_CNT; i++)
		{
			sum_dt += total_dt[i];
		}
		sum_dt = sum_dt/AVERAGE_MAX_CNT;

		printk("--- %d.%d msec  w:%d -h:%d \n",(int)( sum_dt / 1000),(int)( sum_dt % 1000), width, height);
	}
	else
	{
		total_dt[average_count] = dt;
		average_count++;
	}
#endif//

	return 0;
}

int __init tcc_hue_init(void)
{
	tcc_hue_table_alloc();
	return 0;
}

static void __exit tcc_hue_initt(void)
{
	tcc_hue_table_free();
}

late_initcall(tcc_hue_init);
module_exit(tcc_hue_initt);

MODULE_AUTHOR("linux <linux@telechips.com>");
MODULE_DESCRIPTION("Telechips TCC HUE driver");
MODULE_LICENSE("GPL");
