 /****************************************************************************

 *   FileName    : tca_tco.c

 *   Description : PWM Timer Count

 ****************************************************************************

 *

 *   TCC Version 1.0

 *   Copyright (c) Telechips Inc.

 *   All rights reserved 

 

This source code contains confidential information of Telechips.

Any unauthorized use without a written permission of Telechips including not limited to re-distribution in source or binary form is strictly prohibited.

This source code is provided ¡°AS IS¡± and nothing contained in this source code shall constitute any express or implied warranty of any kind, including without limitation, any warranty of merchantability, fitness for a particular purpose or non-infringement of any patent, copyright or other third party intellectual property right. No warranty is made, express or implied, regarding the information¡¯s accuracy, completeness, or performance. 

In no event shall Telechips be liable for any claim, damages or other liability arising from, out of or in connection with this source code or the use in the source code. 

This source code is provided subject to the terms of a Mutual Non-Disclosure Agreement between Telechips and Company.

*

****************************************************************************/

#include <debug.h>
#include <stdlib.h>
#include <string.h>
#include <platform/reg_physical.h>
#include <platform/gpio.h>
#include <dev/gpio.h>

#define	HwTCO_BASE_ADDR(X)		(HwTMR_BASE+(X)*0x10)

PTIMERN IO_TCO_GetBaseAddr(unsigned uCH)
{
	return (PTIMERN)HwTCO_BASE_ADDR(uCH);
}

unsigned IO_TCO_GetGpioFunc(unsigned uCH, unsigned uGPIO)
{
#if defined(TCC893X)
	switch(uGPIO)
	{
		case TCC_GPB(13):	// TCO4
		case TCC_GPB(14):	// TCO5
			return GPIO_FN12;

		#if defined(TCC893X)
		case TCC_GPB(28): //TCO0 / PWM0
		case TCC_GPB(29): //TCO1 / PWM1
		case TCC_GPB(30): //TCO2 / PWM2
		case TCC_GPB(31): //TCO3 / PWM3
			return GPIO_FN5;
		#endif

		case TCC_GPC(0): 	// TCO0
		case TCC_GPC(1):	// TCO1
		case TCC_GPC(2):	// TCO2
		case TCC_GPC(3):	// TCO3
		case TCC_GPC(4):	// TCO4
		case TCC_GPC(5):	// TCO5
			return GPIO_FN9;

		case TCC_GPD(9): 	// TCO0
		case TCC_GPD(10):	// TCO1
		case TCC_GPD(11):	// TCO2
		case TCC_GPD(12):	// TCO3
		case TCC_GPD(13):	// TCO4
		case TCC_GPD(14):	// TCO5
			return GPIO_FN9;

		case TCC_GPE(12): 	// TCO0
		case TCC_GPE(13):	// TCO1
		case TCC_GPE(14):	// TCO2
		case TCC_GPE(15):	// TCO3
		case TCC_GPE(16):	// TCO4
		case TCC_GPE(17):	// TCO5
			return GPIO_FN7;
			
		case TCC_GPF(16): 	// TCO0
		case TCC_GPF(17):	// TCO1
		case TCC_GPF(18):	// TCO2
		case TCC_GPF(19):	// TCO3
		case TCC_GPF(20):	// TCO4
		case TCC_GPF(21):	// TCO5
			return GPIO_FN11;	
			
		case TCC_GPG(5): 	// TCO0
		case TCC_GPG(6):	// TCO1
		case TCC_GPG(7):	// TCO2
		case TCC_GPG(8):	// TCO3
		case TCC_GPG(9):	// TCO4
		case TCC_GPG(10):	// TCO5
			return GPIO_FN7;				

		default:
			break;
	}
#else
#error code : not define TCO gpio functional setting at chipset
#endif//
	return 0;
}


int tca_tco_pwm_ctrl(unsigned tco_ch, unsigned uGPIO, unsigned int max_cnt, unsigned int level_cnt)
{
	unsigned uFGPIO;
	volatile PTIMERN pTIMER = IO_TCO_GetBaseAddr(tco_ch);

	printf("tco:%d, GPIO(G:%d, Num:%d) max:%d level:%d TCOaddr:0x%p \n", tco_ch, (uGPIO>>5), (uGPIO &0x1F), max_cnt, level_cnt, pTIMER);
	if(pTIMER == NULL)
		return -1;
	
	gpio_config(uGPIO, GPIO_OUTPUT);

	if(max_cnt <= level_cnt)	{
		gpio_config(uGPIO, GPIO_OUTPUT | GPIO_FN0);
		gpio_set(uGPIO, 1);
	}
	else if(level_cnt == 0) {
		gpio_config(uGPIO, GPIO_OUTPUT | GPIO_FN0);
		gpio_set(uGPIO, 0);
	}
	else 	{
		pTIMER->TREF = max_cnt;
		pTIMER->TCFG	= 0x105;	
		pTIMER->TMREF = level_cnt;
		pTIMER->TCFG	= 0x105;
		uFGPIO = IO_TCO_GetGpioFunc(tco_ch, uGPIO);
		gpio_config(uGPIO, GPIO_OUTPUT | uFGPIO);
	}
}

