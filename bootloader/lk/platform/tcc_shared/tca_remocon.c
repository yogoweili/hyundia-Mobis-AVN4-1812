/****************************************************************************

 *   FileName    : tca_remocon.c

 *   Description : IR driver for remote controller

 ****************************************************************************

 *

 *   TCC Version 1.0

 *   Copyright (c) Telechips Inc.

 *   All rights reserved 

 

This source code contains confidential information of Telechips.

Any unauthorized use without a written permission of Telechips including not limited to re-distribution in source or binary form is strictly prohibited.

This source code is provided ��AS IS�� and nothing contained in this source code shall constitute any express or implied warranty of any kind, including without limitation, any warranty of merchantability, fitness for a particular purpose or non-infringement of any patent, copyright or other third party intellectual property right. No warranty is made, express or implied, regarding the information��s accuracy, completeness, or performance. 

In no event shall Telechips be liable for any claim, damages or other liability arising from, out of or in connection with this source code or the use in the source code. 

This source code is provided subject to the terms of a Mutual Non-Disclosure Agreement between Telechips and Company.

*

****************************************************************************/

//#include <linux/delay.h>
//#include <asm/io.h>
//#include <mach/bsp.h>
//#include <asm/mach-types.h>
#include <platform/globals.h>
#include "tcc_remocon.h"

//======================================================
// REMOCON initialize
//======================================================
void    RemoconCommandOpen (void)
{
	PREMOTECON      pRcu = (volatile PREMOTECON)tcc_p2v(HwREMOTE_BASE);

	#if defined(CONFIG_ARCH_TCC892X)
		BITCLR(pRcu->CMD.nREG, Hw0);
		BITSET(pRcu->CMD.nREG, Hw1|Hw2|(20*Hw3)|Hw12|Hw13|Hw14);
	#else
		BITCLR(pRcu->CMD, Hw0);
		BITSET(pRcu->CMD, Hw1|Hw2|(20*Hw3)|Hw12|Hw13|Hw14);
	#endif
}

void    RemoconCommandReset (void)
{
	PREMOTECON      pRcu = (volatile PREMOTECON)tcc_p2v(HwREMOTE_BASE);
	
	#if defined(CONFIG_ARCH_TCC892X)
		BITCLR(pRcu->CMD.nREG, Hw0);
		BITCLR(pRcu->CMD.nREG, Hw12);
		BITSET(pRcu->CMD.nREG, Hw1|Hw2|(20*Hw3)|Hw13|Hw14);
	#else
		if(0)//(machine_is_tcc8900() || machine_is_tcc9200() || machine_is_tcc9201() || machine_is_tcc9200s()) {
			pRcu->CMD = Hw1|Hw2|(14*Hw3)|Hw13|Hw14;
		}
		else {
			BITCLR(pRcu->CMD, Hw0);
			BITCLR(pRcu->CMD, Hw12);
			BITSET(pRcu->CMD, Hw1|Hw2|(20*Hw3)|Hw13|Hw14);
		}
	#endif
}

//======================================================
// configure address
//======================================================
void    RemoconConfigure (void)
{
	PREMOTECON      pRcu = (volatile PREMOTECON)tcc_p2v(HwREMOTE_BASE);

	#if defined(CONFIG_ARCH_TCC892X)
		#ifdef CONFIG_PBUS_DIVIDE_CLOCK
			BITCLR(pRcu->INPOL.nREG, Hw8);
		#else
			BITSET(pRcu->INPOL.nREG, Hw8);
		#endif
			BITSET(pRcu->INPOL.nREG, Hw0|Hw9|Hw10|Hw11);

			tcc_remocon_set_io();
	#else
		if(0)//(machine_is_tcc8900() || machine_is_tcc9200() || machine_is_tcc9201() || machine_is_tcc9200s()) {
			BITSET(pRcu->INPOL, Hw1);             // polarity enable
		}
		else {
			#ifdef CONFIG_PBUS_DIVIDE_CLOCK
				BITCLR(pRcu->INPOL, Hw8);
			#else
				BITSET(pRcu->INPOL, Hw8);
			#endif
			BITSET(pRcu->INPOL, Hw0|Hw9|Hw10|Hw11);
			//BITCLR(pRcu->INPOL, (Hw9|Hw10|Hw11));

			tcc_remocon_set_io();
		}
	#endif
}

//======================================================
// REMOCON STATUS : irq enacle and fifo overflow (active low)
//======================================================
void    RemoconStatus (void)
{
	PREMOTECON      pRcu = (volatile PREMOTECON)tcc_p2v(HwREMOTE_BASE);

#if defined(CONFIG_ARCH_TCC892X)
	BITCSET(pRcu->STA.nREG, 0x0, Hw12);
#else
	BITCSET(pRcu->STA, 0x0, Hw12);
#endif
}

//======================================================
// REMOCON DIVIDE enable & ir clk & end count setting
// (end count use remocon clk)
//======================================================
void    RemoconDivide (void)
{
	PREMOTECON      pRcu = (volatile PREMOTECON)tcc_p2v(HwREMOTE_BASE);
	unsigned int    uiclock = tca_ckc_getfbusctrl(FBUS_IO);

	unsigned int    uidiv   = (unsigned int)(uiclock/32768)*100;

	//printk("+++++++++++++++++++++++++++++++++++++[%d][%d]", uiclock, uidiv);

	#if defined(CONFIG_ARCH_TCC892X)
		#ifdef CONFIG_PBUS_DIVIDE_CLOCK
			//pRcu->CLKDIV.nREG = ((500*10)*Hw14)|(1*Hw0);
			pRcu->CLKDIV.nREG = (uidiv*Hw14)|(1*Hw0);
		#else
			pRcu->CLKDIV.nREG = (1*Hw0);
		#endif

		pRcu->CLKDIV.nREG = BITCSET(pRcu->CLKDIV.nREG, 0x000000FF, 0xFE);	// Set END_CNT as 0xFE
	#else
		if(machine_is_tcc8900() || machine_is_tcc9200() || machine_is_tcc9201() || machine_is_tcc9200s()) {
			pRcu->CLKDIV = ((500*24)*Hw14)|(1*Hw0);
		}
		else{
			#ifdef CONFIG_PBUS_DIVIDE_CLOCK
				pRcu->CLKDIV = ((500*30)*Hw14)|(1*Hw0);
			#else
				pRcu->CLKDIV = (1*Hw0);
			#endif
		}

		pRcu->CLKDIV = BITCSET(pRcu->CLKDIV, 0x000000FF, 100);	// Set END_CNT as 100
	#endif
}

//======================================================
// REMOCON diable 
//======================================================
void    DisableRemocon (void)
{
	PREMOTECON      pRcu = (volatile PREMOTECON)tcc_p2v(HwREMOTE_BASE);

#if defined(CONFIG_ARCH_TCC892X)
	BITSET(pRcu->CMD.nREG, Hw0);
	BITCLR(pRcu->CMD.nREG, Hw1|Hw2|Hw13|Hw14);
#else
	BITSET(pRcu->CMD, Hw0);
	BITCLR(pRcu->CMD, Hw1|Hw2|Hw13|Hw14);
#endif
}

//======================================================
// REMOCON functions
//======================================================
void    RemoconInit (void)
{
	PREMOTECON      pRcu = (volatile PREMOTECON)tcc_p2v(HwREMOTE_BASE);

	RemoconDivide();
	RemoconIrqClear();
	RemoconIntClear();

#if defined(CONFIG_ARCH_TCC892X)
	pRcu->CLKDIV.nREG = BITCSET(pRcu->CLKDIV.nREG, 0x000000FF, 0xFE);	// Set END_CNT as 0xFE
#else
	pRcu->CLKDIV = BITCSET(pRcu->CLKDIV, 0x000000FF, 100);	// Set END_CNT as 100
#endif
}

void    RemoconIntClear ()
{
	PREMOTECON      pRcu = (volatile PREMOTECON)tcc_p2v(HwREMOTE_BASE);

#if defined(CONFIG_ARCH_TCC892X)
	BITCLR(pRcu->CMD.nREG, Hw2);
	BITSET(pRcu->CMD.nREG, Hw0);

	//udelay(120);

	BITSET(pRcu->CMD.nREG, Hw2);
	BITCLR(pRcu->CMD.nREG, Hw0);
#else
	BITCLR(pRcu->CMD, Hw2);
	BITSET(pRcu->CMD, Hw0);

	//udelay(120);

	BITSET(pRcu->CMD, Hw2);
	BITCLR(pRcu->CMD, Hw0);
#endif
}

void    RemoconIntWait ()
{
	PPIC pPic = (volatile PPIC)tcc_p2v(HwPIC_BASE);
	uint rx_data;

#if defined(CONFIG_ARCH_TCC892X)
	while(1)
	{
		rx_data = pPic->STS1.nREG;
		if((rx_data&0x400) == 0x400)
			break;
	}
	pPic->CLR1.nREG = 0x400;     //RMT bit set to 1 which means rmt interrupt clear      
#else
	while(1)
	{
		rx_data = pPic->STS1;
		if((rx_data&0x400) == 0x400)
			break;
	}
	pPic->CLR1 = 0x400;     //RMT bit set to 1 which means rmt interrupt clear      
#endif
}

void    RemoconIrqClear ()
{
	uint idx;
	PPIC pPic = (volatile PPIC)tcc_p2v(HwPIC_BASE);

#if defined(CONFIG_ARCH_TCC892X)
	for(idx=1; idx<3000; idx ++)
		pPic->CLR1.nREG = 0x400;                     // Clear VIC Interrupt Clear Register 
#else
	for(idx=1; idx<3000; idx ++)
		pPic->CLR1 = 0x400;                     // Clear VIC Interrupt Clear Register 
#endif
}

void	tcc_remocon_set_io(void)
{
#if defined(CONFIG_ARCH_TCC93XX) || defined(CONFIG_ARCH_TCC88XX)
    	PGPIO pGpioA = (volatile PGPIO)tcc_p2v(HwGPIOA_BASE);
    	BITSET(pGpioA->GPAIS, Hw5);
#endif
}
