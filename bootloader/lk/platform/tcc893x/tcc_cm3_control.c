/****************************************************************************

 *   FileName    : tcc_cm3_control.c

 *   Description : Telechips Cortex M3 Driver

 ****************************************************************************

 *

 *   TCC Version 1.0

 *   Copyright (c) Telechips Inc.

 *   All rights reserved 

 

This source code contains confidential information of Telechips.

Any unauthorized use without a written permission of Telechips including not limited to re-distribution in source or binary form is strictly prohibited.

This source code is provided ??AS IS?? and nothing contained in this source code shall constitute any express or implied warranty of any kind, including without limitation, any warranty of merchantability, fitness for a particular purpose or non-infringement of any patent, copyright or other third party intellectual property right. No warranty is made, express or implied, regarding the information??s accuracy, completeness, or performance. 

In no event shall Telechips be liable for any claim, damages or other liability arising from, out of or in connection with this source code or the use in the source code. 

This source code is provided subject to the terms of a Mutual Non-Disclosure Agreement between Telechips and Company.

*

****************************************************************************/

#include <platform/globals.h>
#include <platform/iomap.h>
#include <platform/reg_physical.h>
#include <arch/tcc_used_mem.h>

#include "tcc_cm3_control.h"
#if 1
#include "TCC893xCM3AVN.h"
#else
#include "TCC893xCM3AVN_TCC.h"
#endif
#include "cm3_lock.h"

#ifndef tcc_p2v
#define tcc_p2v(x)      (x)
#endif

#ifndef NULL
#define NULL 0
#endif

#ifdef TARGET_BOARD_DAUDIO
#include <daudio_ver.h>
#include <stdlib.h>
#endif

#include "include/cm3_lock.h" //GT system
#include "include/platform/gpio.h"

static int cm3_clk = 0;
static int cm3_cmd_status = 0;

static int retData[8];

static int (*cm3_proc)(unsigned long);

#undef cm3_dbg
#if 0
#define cm3_dbg(f, a...)  printf("== cm3_dbg == " f, ##a)
#else
#define cm3_dbg(f, a...)
#endif

/*****************************************************************************
* Function Name : static void CM3_UnloadBinary(void);
* Description : CM3 Code Un-Loading
* Arguments :
******************************************************************************/
static void CM3_UnloadBinary(void)
{
    PCM3_TSD_CFG pTSDCfg = (volatile PCM3_TSD_CFG)tcc_p2v(HwCORTEXM3_TSD_CFG_BASE);
    BITSET(pTSDCfg->CM3_RESET.nREG, Hw1|Hw2); //m3 no reset
    cm3_dbg("%s:%d\n",__func__, __LINE__);
}


/*****************************************************************************
* Function Name : static void CM3_LoadBinary(void);
* Description : CM3 Code Loading
* Arguments :
******************************************************************************/
static void CM3_LoadBinary(void* binary, unsigned size)
{
//    volatile unsigned int * pCodeMem = (volatile unsigned int *)tcc_p2v(HwCORTEXM3_CODE_MEM_BASE);
    volatile unsigned int * pCodeMem = (volatile unsigned int *)0x10000000;
    PCM3_TSD_CFG pTSDCfg = (volatile PCM3_TSD_CFG)tcc_p2v(HwCORTEXM3_TSD_CFG_BASE);
    CM3_UnloadBinary();
    cm3_dbg("%s:%d\n",__func__, __LINE__);
    pTSDCfg->REMAP0.bREG.ICODE = 0x1;
    pTSDCfg->REMAP0.bREG.DCODE = 0x1;    
    pTSDCfg->REMAP0.bREG.REMAP_2 = 0x2;    
    pTSDCfg->REMAP0.bREG.REMAP_3 = 0x3;    
    pTSDCfg->REMAP0.bREG.REMAP_4 = 0x4;    
    pTSDCfg->REMAP0.bREG.REMAP_5 = 0x5;    
    pTSDCfg->REMAP0.bREG.REMAP_6 = 0x6;    
    pTSDCfg->REMAP0.bREG.REMAP_7 = 0x7;    
    memcpy((void *)pCodeMem, binary, size);

	BITCLR(pTSDCfg->CM3_RESET.nREG, Hw1|Hw2); //m3 reset
}

/*****************************************************************************
* Function Name : static void MailBox_Configure(void);
* Description : MailBox register init
* Arguments :
******************************************************************************/
static void CM3_MailBox_Configure(void)
{
    volatile PMAILBOX pMailBoxMain = (volatile PMAILBOX)tcc_p2v(HwCORTEXM3_MAILBOX0_BASE);
    volatile PMAILBOX pMailBoxSub = (volatile PMAILBOX)tcc_p2v(HwCORTEXM3_MAILBOX1_BASE);
    volatile PCM3_TSD_CFG pTSDCfg = (volatile PCM3_TSD_CFG)tcc_p2v(HwCORTEXM3_TSD_CFG_BASE);
    BITSET(pMailBoxMain->uMBOX_CTL_016.nREG, Hw0|Hw1|Hw4|Hw5|Hw6);
    BITSET(pMailBoxSub->uMBOX_CTL_016.nREG, Hw0|Hw1|Hw4|Hw5|Hw6);
    BITSET(pTSDCfg->IRQ_MASK_POL.nREG, Hw16|Hw22); //IRQ_MASK_POL, IRQ, FIQ(CHANGE POLARITY)
}

#if 0
static irqreturn_t CM3_MailBox_Handler(int irq, void *dev)
{	
    int i;
    char cmd, dmxid;
    PMAILBOX pMailBox = (volatile PMAILBOX)tcc_p2v(HwCORTEXM3_MAILBOX0_BASE);
	memcpy(retData, &(pMailBox->uMBOX_RX0), sizeof(int)*8);

#if 0
#if 0
	printf("MB : [0x%X][0x%X][0x%X][0x%X][0x%X][0x%X][0x%X][0x%X]\n", nReg[0], nReg[1], nReg[2], nReg[3], nReg[4], nReg[5], nReg[6], nReg[7]);
#else
	printf("MB : ");
	for(i=0; i<8; i++)
	{
		if(i == 1)
			printf("[0x%X]", retData[i]);
		else
			printf("[%d]", retData[i]);			
	}
	printf("\n");
#endif	
#endif
	wake_up_interruptible( &wq_cm3_cmd ) ;
	cm3_cmd_status = 0;
    return IRQ_HANDLED;
}
#endif

long tcc_cm3_ctrl_ioctl(unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	//printf("%s cmd: %d, arg: %x\n", __FUNCTION__, cmd, arg);
	switch (cmd) {
		case IOCTL_CM3_CTRL_OFF:
		{
			if (cm3_clk) {
				CM3_UnloadBinary();
				//free_irq(MBOX_IRQ, NULL);
				tca_ckc_setfbusctrl( FBUS_CM3, DISABLE, 1000000); /*FBUS_CM3      100 MHz */
			}
		}
		break;

		case IOCTL_CM3_CTRL_ON:
		{	
			unsigned int loop=0;
			unsigned int value =0,gpio26=0,gpio27= 0;
			unsigned int mode_value[3];

			unsigned int parking_guide = (unsigned int)PA_JPEG_RAW_BASE_ADDR;
			unsigned int preview = (unsigned int)PA_JPEG_HEADER_BASE_ADDR;
			unsigned int decoder_chip = 0;
			int main_ver = get_daudio_main_ver();

			switch (main_ver)
			{
				case DAUDIO_BOARD_6TH:
				case DAUDIO_BOARD_5TH:
				default:
					decoder_chip = 9912;
					break;
			}
			dprintf(INFO, "[GT system]%s cm3 decoder_chip: tw%d \n", __func__, decoder_chip);

			ret = cm3_sync_lock_init();
             // GT system start 
			for(loop = 0 ; loop < 3 ; loop++)
			{
	             gpio26 = gpio_get( GPIO_PORTF|26 );
    	         gpio27 = gpio_get( GPIO_PORTF|27 );
        	     value = ((gpio26<<1)|gpio27);
				 mode_value[loop]=value;				
				 //msleep(10); //??
				 mdelay(10);
			}
                 if( mode_value[0] == mode_value[1] && mode_value[1] == mode_value[2] )
				 {
					 dprintf(INFO, "[GT system]%s : gpio read value pass.................. \n", __func__);
            		 if(value == 1)
                		 cm3_set_camera_type(0x300);//avm_svideo
	            	 else if(value == 2)
    	            	 cm3_set_camera_type(0x400);//avm-component
	        	     else if(value == 3)
    	        	     cm3_set_camera_type(0x200);//update
	    	         else 
    	    	         cm3_set_camera_type(0x100);//CVBS
				 }
			dprintf(INFO, "[GT system]%s : gpio read value = %d \n", __func__, value);
             //GT system end


			#if defined(EARLY_CAM_SHOW_PARKING_GUIDE)	//default no parking guide line.
			ret = cm3_set_pg_addr(parking_guide);
			#else
			ret = cm3_set_pg_addr(NULL);
			#endif

			ret = cm3_set_preview_addr(preview);
			ret = cm3_set_decoder_chip(decoder_chip);

			tca_ckc_setfbusctrl( FBUS_CM3, ENABLE, 1000000); /*FBUS_CM3      100 MHz */
			CM3_MailBox_Configure();
			CM3_LoadBinary((void*)TCC893xCM3AVN, sizeof(TCC893xCM3AVN));
			#if 0
				cm3_proc = CM3_AVN_Proc;
				ret = request_irq(MBOX_IRQ, CM3_MailBox_Handler, IRQ_TYPE_LEVEL_LOW | IRQF_DISABLED, DEVICE_NAME, NULL);
				msleep(100); //Wait for CM3 Booting
				if(ret < 0)
				printf("error : request_irq %d\n", ret);
			#endif

			cm3_clk = 1;
		}
		break;

		case IOCTL_CM3_CTRL_CMD:
		{
			#if 0
				ret = (*cm3_proc)(arg);
			#endif
		}
		break;

		case IOCTL_CM3_CTRL_RESET:
		default:
			printf("cm3 error: unrecognized ioctl (0x%x)\n", cmd); 
			return -1;
		break;
	}

	return ret;
}


