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

#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <mach/bsp.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <mach/gpio.h>

#include "tcc_avn_proc.h"
#include "adec.h"

#if defined(CONFIG_ARCH_TCC893X)
#include <mach/io.h>
#include <mach/vioc_api.h>
#include <mach/vioc_rdma.h>
#include <mach/vioc_config.h>
#include <mach/vioc_wdma.h>
#include <mach/vioc_wmix.h>
#include <mach/vioc_global.h>
#if defined(CONFIG_ARCH_TCC893X)
#include <mach/vioc_plugin_tcc893x.h>
#endif
#endif


#undef adec_dbg
#if 0
#define adec_dbg(f, a...)  printk("== adec_proc == " f, ##a)
#else
#define adec_dbg(f, a...)
#endif


static int CM3_AVN_TEST(int time_interval)
{
	adec_dbg("\n%s:%d\n",__func__, __LINE__);
	ARG_CM3_CMD InArg;
	ARG_CM3_CMD OutArg;	

	InArg.uiCmdId = HW_TIMER_TEST;
	InArg.uiParam0 = time_interval;

	memset(&OutArg, 0x00, sizeof(OutArg));
	CM3_SEND_COMMAND(&InArg, &OutArg);

	return OutArg.uiParam0;
}

int CM3_AVN_Proc(unsigned long arg)
{
	t_cm3_avn_cmd avn_cmd;
	int ret = -1;

	copy_from_user(&avn_cmd, (t_cm3_avn_cmd*)arg, sizeof(t_cm3_avn_cmd));
	printk("cm3_cmd:   iOpCode = 0x%x, pParam1 = 0x%x, pParam2 = 0x%x. \n", avn_cmd.iOpCode, avn_cmd.pParam1, avn_cmd.pParam2);

	switch(avn_cmd.iOpCode)
	{
		case HW_TIMER_TEST:
		{
			ret = CM3_AVN_TEST((int)(avn_cmd.pParam1));
		}
		break;

		case GET_EARLY_CAMERA_STATUS:
		{
			#if (1)
				ARG_CM3_CMD InCmd, OutCmd;

				memset(&InCmd, 0x00, sizeof(InCmd));
				InCmd.uiCmdId = GET_EARLY_CAMERA_STATUS;
				memset(&OutCmd, 0xFF, sizeof(OutCmd));

				CM3_SEND_COMMAND(&InCmd, &OutCmd);
				//printk("CM3_AVN_Proc :   early-camera status (0x%x), gpio_g[19] status (0x%x). \n", OutCmd.uiParam0, gpio_get_value(TCC_GPG(19)));

				ret = avn_cmd.pParam1 = OutCmd.uiParam0;
				//memcpy((t_cm3_avn_cmd *)arg, &avn_cmd, sizeof(t_cm3_avn_cmd)); // return to pParam1
				copy_to_user((t_cm3_avn_cmd *)arg, &avn_cmd, sizeof(t_cm3_avn_cmd)); // return to pParam1
			#else
				ret = avn_cmd.pParam1 = gpio_get_value(TCC_GPG(19));
				//printk("CM3_AVN_Proc :   gpio_g[19] status (0x%x). \n", ret);
				memcpy((t_cm3_avn_cmd *)arg, &avn_cmd, sizeof(t_cm3_avn_cmd)); // return to pParam1
			#endif
		}
		break;

		case SET_EARLY_CAMERA_STOP:
		{
			ARG_CM3_CMD InCmd, OutCmd;

			memset(&InCmd, 0x00, sizeof(InCmd));
			memset(&OutCmd, 0xFF, sizeof(OutCmd)); 

			#if 1
				InCmd.uiCmdId = SET_EARLY_CAMERA_DISPLAY_OFF;
				CM3_SEND_COMMAND(&InCmd, &OutCmd);
				//printk("CM3_AVN_Proc ret :   OutCmd.uiParam0 = 0x%x. \n", OutCmd.uiParam0);

				ret = tcc_early_camera_display_off();
				ret = tcc_early_camera_stop();
			#else
				InCmd.uiCmdId = SET_EARLY_CAMERA_STOP;
				CM3_SEND_COMMAND(&InCmd, &OutCmd);
				//printk("CM3_AVN_Proc ret :   OutCmd.uiParam0 = 0x%x. \n", OutCmd.uiParam0);

				ret = OutCmd.uiParam0;
			#endif
		}
		break;

		case SET_EARLY_CAMERA_DISPLAY_OFF:
		{
			ARG_CM3_CMD InCmd, OutCmd;
			memset(&InCmd, 0x00, sizeof(InCmd));
			memset(&OutCmd, 0xFF, sizeof(OutCmd));

			InCmd.uiCmdId = SET_EARLY_CAMERA_DISPLAY_OFF;
			CM3_SEND_COMMAND(&InCmd, &OutCmd);
			//printk("CM3_AVN_Proc ret :   OutCmd.uiParam0 = 0x%x. \n", OutCmd.uiParam0);

			ret = tcc_early_camera_display_off();
		}
		break;

		case SET_REAR_CAMERA_RGEAR_DETECT:
		{
			ARG_CM3_CMD InCmd, OutCmd;

			memset(&InCmd, 0x00, sizeof(InCmd));
			InCmd.uiCmdId = SET_REAR_CAMERA_RGEAR_DETECT;
			memset(&OutCmd, 0xFF, sizeof(OutCmd));

			CM3_SEND_COMMAND(&InCmd, &OutCmd);
			//printk("CM3_AVN_Proc ret :   OutCmd.uiParam0 = 0x%x. \n", OutCmd.uiParam0);

			ret = OutCmd.uiParam0;
		}
		break;

		case SET_EARLY_CAMERA_HANDOVER:
		{
			ARG_CM3_CMD InCmd, OutCmd;
			memset(&InCmd, 0x00, sizeof(InCmd));
			memset(&OutCmd, 0xFF, sizeof(OutCmd));

			InCmd.uiCmdId = SET_EARLY_CAMERA_HANDOVER;
			CM3_SEND_COMMAND(&InCmd, &OutCmd);
			//printk("CM3_AVN_Proc ret :   OutCmd.uiParam0 = 0x%x. \n", OutCmd.uiParam0);

			ret = OutCmd.uiParam0;
		}
		break;

		case SET_EARLY_CAMERA_BLACK_SCREEN:
		{
			ARG_CM3_CMD InCmd, OutCmd;
			memset(&InCmd, 0x00, sizeof(InCmd));
			memset(&OutCmd, 0xFF, sizeof(OutCmd));

			InCmd.uiCmdId = SET_EARLY_CAMERA_BLACK_SCREEN;
			CM3_SEND_COMMAND(&InCmd, &OutCmd);
			//printk("CM3_AVN_Proc ret :   OutCmd.uiParam0 = 0x%x. \n", OutCmd.uiParam0);

			ret = OutCmd.uiParam0;
		}
		break;

		default:
		break;
	}

	//printk("CM3_AVN_Proc ret : 0x%x\n", ret);
	return ret;
}

int tcc_early_camera_stop(void)
{
	volatile PVIOC_WDMA 	pWDMABase  = (volatile PVIOC_WDMA)tcc_p2v(HwVIOC_WDMA05);
	volatile PVIOC_WMIX 	pWMIXBase 	= (volatile PVIOC_WMIX)tcc_p2v(HwVIOC_WMIX5);
	volatile PVIOC_VIN 	pVINBase 	= (volatile PVIOC_VIN)tcc_p2v(HwVIOC_VIN00);
	volatile PVIOC_RDMA 	pRDMABase 	= (volatile PVIOC_RDMA)tcc_p2v(HwVIOC_RDMA16);

	BITCSET(pWDMABase->uIRQMSK.nREG, 0x1FF, 0x1FF); // disable WDMA interrupt
	BITCSET(pWDMABase->uCTRL.nREG, (1<<28), (0<<28)); // disable WDMA
	BITCSET(pWDMABase->uCTRL.nREG, (1<<16), (1<<16)); // update WDMA

	BITCSET(pWMIXBase->uCTRL.nREG, (1<<16), (0<<16)); // disable WMIX

	BITCSET(pVINBase->uVIN_CTRL.nREG, 1, 0); // disable VIN

	BITCSET(pRDMABase->uIRQMSK.nREG, 0x7F, 0x7F); // disable RDMA interrupt
	BITCSET(pRDMABase->uCTRL.nREG, (1<<28), (0<<28)); // disable RDMA
	BITCSET(pRDMABase->uCTRL.nREG, (1<<16), (1<<16)); // update RDMA

	VIOC_CONFIG_PlugOut(VIOC_SC0);
	VIOC_CONFIG_PlugOut(VIOC_DEINTLS);

	return 1;
}

int tcc_early_camera_display_off(void)
{
	volatile PVIOC_WMIX pWMIXBase = (volatile PVIOC_WMIX)tcc_p2v((unsigned int)HwVIOC_WMIX1);
	volatile PVIOC_RDMA pRDMABase = (volatile PVIOC_RDMA)tcc_p2v((unsigned int)HwVIOC_RDMA05);

	VIOC_RDMA_SetImageDisable(pRDMABase);

	// set to WMIX-1 channel priority.
	BITCSET(pWMIXBase->uROPC0.nREG, 0xFFFFFFFF, 0x0000C018);
	BITCSET(pWMIXBase->uCTRL.nREG, 0x0000001F, 24); // 24 :  layer3(image0) layer2(image1) layer1(image2) layer1(image3)
	BITCSET(pWMIXBase->uCTRL.nREG, (1<<16), (1<<16));

	printk("%s() -  overlay priority is %d. \n", __func__, (pWMIXBase->uCTRL.nREG & 0x0000001F));
	return 1;
}


