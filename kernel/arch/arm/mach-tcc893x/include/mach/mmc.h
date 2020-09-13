/* linux/arch/arm/mach-tcc88xx/include/mach/mmc.h
 *
 * Copyright (C) 2010 Telechips, Inc.
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

#ifndef __TCC_MMC_H
#define __TCC_MMC_H

#include <linux/types.h>
#include <linux/device.h>
#include <linux/mmc/host.h>

#ifdef CONFIG_MMC_TCC_NORMAL_MODE
	#define MMC_MAX_CLOCK_SPEED 24000000 
#elif defined(CONFIG_MMC_TCC_HIGHSPEED_MODE) && defined(CONFIG_CHIP_TCC8930)
#if defined(CONFIG_DAUDIO)
	#define MMC_MAX_CLOCK_SPEED 48000000 
#else
	#define MMC_MAX_CLOCK_SPEED 40000000 
#endif
#elif CONFIG_MMC_TCC_DDR_MODE
	#define MMC_MAX_CLOCK_SPEED 40000000 
#else
	#define MMC_MAX_CLOCK_SPEED 48000000 
#endif

struct tcc_mmc_platform_data {
	int slot;
	unsigned int caps;
	unsigned int f_min;
	unsigned int f_max;
	u32 ocr_mask;

	int (*init)(struct device *dev, int id);

	int (*card_detect)(struct device *dev, int id);

	int (*cd_int_config)(struct device *dev, int id, unsigned int cd_irq);	

	int (*suspend)(struct device *dev, int id);
	int (*resume)(struct device *dev, int id);

	int (*set_power)(struct device *dev, int id, int on);

	int (*set_bus_width)(struct device *dev, int id, int width);
	
	int (*switch_voltage)(struct device *dev, int id, int on);
	int (*get_ro)(struct device *dev, int id);
/**  
* @author sjpark@cleinsoft
* @date 2014/03/10
* emmc clock control 
**/
#if defined(CONFIG_MMC_TCC_EMMC_CLOCK_CONTROL)
	int (*clock_onoff)(struct device *dev, int id, int on);
#endif
	
	unsigned int cd_int_num;
	unsigned int cd_irq_num;
	unsigned int cd_ext_irq;
	unsigned int peri_name;			/* sd core name */
	unsigned int io_name;			/* sd core name */	
	unsigned int pic;			/* Priority Interrupt Controller register name */
#if defined(CONFIG_ENABLE_TCC_MMC_KPANIC)
	bool is_kpanic_card;
#endif
#ifdef CONFIG_HIBERNATION
	bool is_external_sd;
#endif
};

struct mmc_port_config {
	int data0;
	int data1;
	int data2;
	int data3;
	int data4;
	int data5;
	int data6;
	int data7;
	int cmd;
	int clk;
	int wp;
	int func;
	int width;
	int strength;

	int cd;
	int pwr;
	int vctl;
};


#endif
