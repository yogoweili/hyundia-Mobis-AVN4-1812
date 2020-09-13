/****************************************************************************
 *   FileName    : tca_i2c.c
 *   Description : 
 ****************************************************************************
 *
 *   TCC Version 1.0
 *   Copyright (c) Telechips, Inc.
 *   ALL RIGHTS RESERVED
 *
 ****************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <asm/mach-types.h>

#include <mach/bsp.h>
#include <mach/tca_ckc.h>
#include <mach/tca_i2c.h>
#include <mach/gpio.h>

/*****************************************************************************
* Function Name : tca_i2c_setgpio(int ch)
* Description: I2C port configuration
* input parameter:
* 		int core; 	// I2C Core
*       int ch;   	// I2C master channel
******************************************************************************/

/**
 * @author valky@cleinsoft
 * @date 2013/11/19
 * D-Audio I2C
 *
 * 2nd board
 * channel 0
 *		iPOD
 *
 * channel 1
 * 		Touch mXT336S-AT
 *
 * channel 2
 *		PMIC LTC3676
 *
 * channel 3
 * 		TW8836
 **/

void tca_i2c_setgpio(int core, int ch)
{
	//PI2CPORTCFG i2c_portcfg = (PI2CPORTCFG)tcc_p2v(HwI2C_PORTCFG_BASE);
	switch (core)
	{
		case 0:
		#if defined(CONFIG_DAUDIO)
			/**
			 * @author valky@cleinsoft
			 * @date 2013/11/1
			 * for iPOD
			 **/
			#if defined(CONFIG_DAUDIO_20140220)
			BITCSET(((PI2CPORTCFG)io_p2v(HwI2C_PORTCFG_BASE))->PCFG0.nREG, 0x000000FF, 8);
			tcc_gpio_config(TCC_GPB(9), GPIO_FN11|GPIO_OUTPUT|GPIO_LOW);
			tcc_gpio_config(TCC_GPB(10), GPIO_FN11|GPIO_OUTPUT|GPIO_LOW);
			#endif
		#endif
			break;
		case 1:
		#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT_336S_AT)
			#if defined(CONFIG_DAUDIO_20140220)
			/**
			 * @author valky@cleinsoft
			 * @date 2013/10/19
			 * Touch mXT336S-AT I2C
			 **/
			BITCSET(((PI2CPORTCFG)io_p2v(HwI2C_PORTCFG_BASE))->PCFG0.nREG, 0x0000FF00, 15<<8);
			tcc_gpio_config(TCC_GPC(30), GPIO_FN7|GPIO_OUTPUT|GPIO_LOW);
			tcc_gpio_config(TCC_GPC(31), GPIO_FN7|GPIO_OUTPUT|GPIO_LOW);
			#endif	
		#endif
			break;
		case 2:
		#if defined(CONFIG_DAUDIO)
			#if defined(CONFIG_DAUDIO_20140220)
			/**
			 * @author sjpark@cleinsoft
			 * @date 2013/07/25
			 * Set GPIO Port for PMIC I2C.
			 * valky@cleinsoft 2013/11/12 change channel PMIC LTC3676
			 * @date 2014/12/15
             * Set GPIO port for TDMB - 'i2c-ch2' is used for TDMB control on kernel. 
			 **/
#if defined(INCLUDE_TDMB)
			// TDMB
			BITCSET(((PI2CPORTCFG)io_p2v(HwI2C_PORTCFG_BASE))->PCFG0.nREG, 0x00FF0000, 5<<16);
			tcc_gpio_config(TCC_GPD(13), GPIO_FN8|GPIO_OUTPUT|GPIO_LOW);
			tcc_gpio_config(TCC_GPD(14), GPIO_FN8|GPIO_OUTPUT|GPIO_LOW);
			BITCSET(((PI2CPORTCFG)io_p2v(HwI2C_PORTCFG_BASE))->PCFG1.nREG, 0xFFFFFFFF, 0x00000000);

#else
            // PMIC
			//I2C[12] - GPIOC[18][19]
			BITCSET(((PI2CPORTCFG)io_p2v(HwI2C_PORTCFG_BASE))->PCFG0.nREG, 0x00FF0000, 12<<16);
			tcc_gpio_config(TCC_GPC(18), GPIO_FN7|GPIO_OUTPUT|GPIO_LOW);
			tcc_gpio_config(TCC_GPC(19), GPIO_FN7|GPIO_OUTPUT|GPIO_LOW);
#endif
			#endif
		#endif
			break;
		case 3:
		#if defined(CONFIG_DAUDIO)
			#if defined(CONFIG_DAUDIO_20140220)
			/**
			 * @author valky@cleinsoft
			 * @date 2013/10/19
			 * TW8836 I2C
			 **/
			BITCSET(((PI2CPORTCFG)io_p2v(HwI2C_PORTCFG_BASE))->PCFG0.nREG, 0xFF000000, 6<<24);
			tcc_gpio_config(TCC_GPD(15), GPIO_FN8|GPIO_OUTPUT|GPIO_LOW);
			tcc_gpio_config(TCC_GPD(16), GPIO_FN8|GPIO_OUTPUT|GPIO_LOW);
			#endif			
		#endif
			break;
		default:
			break;
	}
}
