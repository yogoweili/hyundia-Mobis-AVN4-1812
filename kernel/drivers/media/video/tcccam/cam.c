/****************************************************************************
One line to give the program's name and a brief idea of what it does.
Copyright (C) 2013 Telechips Inc.

This program is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place,
Suite 330, Boston, MA 02111-1307 USA
****************************************************************************/

#include <linux/delay.h>
#include <mach/hardware.h>
#include <linux/clk.h>


#if  defined(CONFIG_ARCH_TCC893X)
#include <mach/bsp.h>
#endif

#include "cam.h"
#include "cam_reg.h"
#include "sensor_if.h"


static struct clk *cifmc_clk;
#if defined(CONFIG_USE_ISP)
static struct clk *isps_clk;
static struct clk *ispj_clk;
#elif defined(CONFIG_ARCH_TCC893X)
// Already, VIOC Block enable.
#else
static struct clk *cifsc_clk;
#endif

#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
extern TCC_SENSOR_INFO_TYPE tcc_sensor_info;
#endif
/**********************************************************
*
*    Function of
*
*
*
*    Input    :
*    Output    :
*    Return    :
*
*    Description    :
**********************************************************/
void CIF_Clock_Get(void)
{
	#if defined(CONFIG_ARCH_TCC893X)
		if((system_rev == 0x1000) || (system_rev == 0x2000) || (system_rev == 0x5000) || (system_rev == 0x5001) || (system_rev == 0x5002) || system_rev == 0x3000) {
			cifmc_clk = clk_get(NULL, "out0");
		}
		BUG_ON(cifmc_clk == NULL);
	#else
		cifmc_clk = clk_get(NULL, "cifmc");
		BUG_ON(cifmc_clk == NULL);

		#if defined(CONFIG_USE_ISP)
		isps_clk = clk_get(NULL, "isps");
		BUG_ON(isps_clk == NULL);

		ispj_clk = clk_get(NULL, "ispj");
		BUG_ON(ispj_clk == NULL);
		#else
		cifsc_clk = clk_get(NULL, "cifsc");
		BUG_ON(cifsc_clk == NULL);
		#endif
	#endif
}

void CIF_Clock_Put(void)
{
	#if defined(CONFIG_ARCH_TCC893X)
		clk_put(cifmc_clk);
	#else
		clk_put(cifmc_clk);
		#if defined(CONFIG_USE_ISP)
		clk_put(isps_clk);
		clk_put(ispj_clk);
		#else
		clk_put(cifsc_clk);
		#endif
	#endif
}

/**********************************************************
*
*    Function of
*
*
*
*    Input    :
*    Output    :
*    Return    :
*
*    Description    :
**********************************************************/
void CIF_Open(void)
{
#if defined(CONFIG_ARCH_TCC893X)
	volatile PVIOC_IREQ_CONFIG pVIOCCfg = (PVIOC_IREQ_CONFIG)tcc_p2v(HwVIOC_CONFIG);
#endif

#if defined(CONFIG_ARCH_TCC893X)
	#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
		clk_set_rate(cifmc_clk, tcc_sensor_info.m_clock*100);
		printk("VIDEO IN Dual Camera CLK :: MCLK = %d \n", tcc_sensor_info.m_clock/10000);
	#else
		#if !defined(CONFIG_VIDEO_ATV_SENSOR_DAUDIO)
			clk_set_rate(cifmc_clk, CKC_CAMERA_MCLK*100);
			printk("VIDEO IN Single Camera CLK :: MCLK = %d \n", CKC_CAMERA_MCLK/10000);
		#endif
	#endif //CONFIG_VIDEO_DUAL_CAMERA_SUPPORT

	#if !defined(CONFIG_VIDEO_ATV_SENSOR_DAUDIO)
		clk_enable(cifmc_clk);
	#endif
#else
	#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
	clk_set_rate(cifmc_clk, tcc_sensor_info.m_clock*100);
	clk_set_rate(cifsc_clk, tcc_sensor_info.s_clock*100);
	printk("CLK :: MCLK = %d, SCLK = %d. \n", tcc_sensor_info.m_clock/10000, tcc_sensor_info.s_clock/10000);
	#else // CONFIG_VIDEO_DUAL_CAMERA_SUPPORT
	#if !(defined(CONFIG_VIDEO_CAMERA_SENSOR_TVP5150) || defined(CONFIG_VIDEO_CAMERA_SENSOR_GT2005))
	//PLL2 : 400Mhz
	clk_set_rate(cifmc_clk, CKC_CAMERA_MCLK*100);
	#endif
	clk_set_rate(cifsc_clk, CKC_CAMERA_SCLK*100);
	printk("CLK :: MCLK = %d, SCLK = %d. \n", CKC_CAMERA_MCLK/10000, CKC_CAMERA_SCLK/10000);
	#endif // CONFIG_VIDEO_DUAL_CAMERA_SUPPORT
	clk_enable(cifmc_clk);
	clk_enable(cifsc_clk);
#endif

}

/**********************************************************
*
*    Function of
*
*
*
*    Input    :
*    Output    :
*    Return    :
*
*    Description    :
**********************************************************/
void CIF_Close(void)
{
#if  defined(CONFIG_ARCH_TCC893X)
	#if !defined(CONFIG_VIDEO_ATV_SENSOR_DAUDIO)
		clk_disable(cifmc_clk);
	#endif
#else
	clk_disable(cifmc_clk);
	clk_disable(cifsc_clk);
#endif
}

/**********************************************************
*
*    Function of
*
*
*
*    Input    :
*    Output    :
*    Return    :
*
*    Description    :
**********************************************************/
void CIF_ONOFF(unsigned int uiOnOff)
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.
#else  //CONFIG_ARCH_TCC92XX
	volatile PCIF pCIF = (PCIF)tcc_p2v(HwCIF_BASE);

	if(uiOnOff == ON)
	{
		BITCSET(pCIF->ICPCR1, HwICPCR1_ON, (uiOnOff << 31));
	}
	else if(uiOnOff == OFF)
	{
		BITCSET(pCIF->ICPCR1, HwICPCR1_ON, (uiOnOff << 31));
	}
#endif
}


/**********************************************************
*
*    Function of
*
*
*    Input    :
*    Output    :
*    Return    :
*
*    Description    :
**********************************************************/
void CIF_WaitFrameSync(unsigned int exp_timer)
{
	unsigned int cnt=0;

#ifdef CONFIG_ARCH_TCC79X
	while((HwCIRQ & HwCIRQ_SOF) != HwCIRQ_SOF)
	{
		mdelay(1);
		cnt++;

		if(cnt>exp_timer)
			return;

	}
#elif defined(CONFIG_ARCH_TCC893X)
	// In case of 892X, we have to add
#else  //CONFIG_ARCH_TCC92XX
	PCIF pCIF = (PCIF)tcc_p2v(HwCIF_BASE);

	while((pCIF->CIRQ & HwCIRQ_SOF) != HwCIRQ_SOF)
	{
		mdelay(1);
		cnt++;

		if(cnt>exp_timer)
			return;

	}
#endif
}

void CIF_OpStop(char wait_SOF, char sw_reset )
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.
#endif

	CIF_ONOFF(OFF);

	if(sw_reset)
	{
#if  defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.
#endif
	}
}
/* end of file */
