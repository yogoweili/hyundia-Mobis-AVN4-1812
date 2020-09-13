/*
 * linux/arch/arm/mach-tcc893x/board-tcc8930-gpio.c
 *
 * Copyright (C) 2011 Telechips, Inc.
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
#include <linux/module.h>
#include <mach/gpio.h>
#include <linux/i2c.h>
#include <linux/i2c/pca953x.h>
#include "board-tcc8930.h"
#include <asm/mach-types.h>

#include <linux/delay.h>
#include <linux/vt_kern.h>
#include <asm/system.h>
#include <mach/system.h>


static struct board_gpio_irq_config tcc8930_gpio_irqs[] = {
	{ -1, -1 },
};

static struct pca953x_platform_data pca9539_data1 = {
	.gpio_base	= GPIO_PORTEXT1,
};

static struct pca953x_platform_data pca9539_data2 = {
	.gpio_base	= GPIO_PORTEXT2,
};

static struct pca953x_platform_data pca9539_data3 = {
	.gpio_base	= GPIO_PORTEXT3,
};

static struct pca953x_platform_data pca9539_data4 = {
	.gpio_base	= GPIO_PORTEXT4,
};

static struct pca953x_platform_data pca9539_data5 = {
	.gpio_base	= GPIO_PORTEXT5,
};


/* I2C core0 channel0 devices */
static struct i2c_board_info __initdata i2c_devices_expander[] = {
	{
		I2C_BOARD_INFO("pca9539", 0x74),
		.platform_data = &pca9539_data1,
	},
	{
		I2C_BOARD_INFO("pca9539", 0x77),
		.platform_data = &pca9539_data2,
	},
	{
		I2C_BOARD_INFO("pca9539", 0x75),
		.platform_data = &pca9539_data3,
	},
	{
		I2C_BOARD_INFO("pca9538", 0x70),
		.platform_data = &pca9539_data4,
	},
	{	// SV60 Power Board
		I2C_BOARD_INFO("pca9539", 0x76),
		.platform_data = &pca9539_data5,
	},
};

struct timer_list dbg_timer;

#define addr_mem(x) (tcc_p2v(0x73000000+x))
#define ddr_phy(x) (*(volatile unsigned long *)addr_mem(0x420400+(x*4)))

static void dbg_timer_handler(unsigned long data)
{
//	printk("%s\n", __func__);
      printk( "1. Write Leveling Delay - ");
      printk( "WLD      B0:0x%8x B1:0x%8x B2:0x%8x B3:0x%8x\n", ddr_phy(0x78), ddr_phy(0x88), ddr_phy(0x98), ddr_phy(0xA8) );
      printk( "2. DQS Gate Delay       - ");
      printk( "DGD      B0:0x%8x B1:0x%8x B2:0x%8x B3:0x%8x\n", ddr_phy(0x7a), ddr_phy(0x8a), ddr_phy(0x9a), ddr_phy(0xAa) );
      printk( "                          DGSD     B0:0x%8x B1:0x%8x B2:0x%8x B3:0x%8x\n", ddr_phy(0x7c), ddr_phy(0x8c), ddr_phy(0x9c), ddr_phy(0xAc) );
      printk( "3. Read Delay           - ");
      printk( "RDQSD    B0:0x%8x B1:0x%8x B2:0x%8x B3:0x%8x\n"  , ( ddr_phy(0x79) & 0x0000ff00 ) >> 8
                                                                        , ( ddr_phy(0x89) & 0x0000ff00 ) >> 8
                                                                        , ( ddr_phy(0x99) & 0x0000ff00 ) >> 8
                                                                        , ( ddr_phy(0xA9) & 0x0000ff00 ) >> 8
                                                                        );
        printk( "                          B0RBDL D0:0x%2x D1:0x%2x D2:0x%2x D3:0x%2x D4:0x%2x D5:0x%2x D6:0x%2x D7:0x%2x DM:0x%2x\n"
                                                                        , ( ddr_phy(0x76) & 0x0000003f ) >> 0
                                                                        , ( ddr_phy(0x76) & 0x00000fc0 ) >> 6
                                                                        , ( ddr_phy(0x76) & 0x0003f000 ) >> 12
                                                                        , ( ddr_phy(0x76) & 0x00fc0000 ) >> 18
                                                                        , ( ddr_phy(0x76) & 0x3f000000 ) >> 24

                                                                        , ( ddr_phy(0x77) & 0x0000003f ) >> 0
                                                                        , ( ddr_phy(0x77) & 0x00000fc0 ) >> 6
                                                                        , ( ddr_phy(0x77) & 0x0003f000 ) >> 12
                                                                        , ( ddr_phy(0x77) & 0x00fc0000 ) >> 18
                                                                        );
        printk( "                          B1RBDL D0:0x%2x D1:0x%2x D2:0x%2x D3:0x%2x D4:0x%2x D5:0x%2x D6:0x%2x D7:0x%2x DM:0x%2x\n"
                                                                        , ( ddr_phy(0x86) & 0x0000003f ) >> 0
                                                                        , ( ddr_phy(0x86) & 0x00000fc0 ) >> 6
                                                                        , ( ddr_phy(0x86) & 0x0003f000 ) >> 12
                                                                        , ( ddr_phy(0x86) & 0x00fc0000 ) >> 18
                                                                        , ( ddr_phy(0x86) & 0x3f000000 ) >> 24

                                                                        , ( ddr_phy(0x87) & 0x0000003f ) >> 0
                                                                        , ( ddr_phy(0x87) & 0x00000fc0 ) >> 6
                                                                        , ( ddr_phy(0x87) & 0x0003f000 ) >> 12
                                                                        , ( ddr_phy(0x87) & 0x00fc0000 ) >> 18
                                                                        );

        printk( "                          B2RBDL D0:0x%2x D1:0x%2x D2:0x%2x D3:0x%2x D4:0x%2x D5:0x%2x D6:0x%2x D7:0x%2x DM:0x%2x\n"
                                                                        , ( ddr_phy(0x96) & 0x0000003f ) >> 0
                                                                        , ( ddr_phy(0x96) & 0x00000fc0 ) >> 6
                                                                        , ( ddr_phy(0x96) & 0x0003f000 ) >> 12
                                                                        , ( ddr_phy(0x96) & 0x00fc0000 ) >> 18
                                                                        , ( ddr_phy(0x96) & 0x3f000000 ) >> 24

                                                                        , ( ddr_phy(0x97) & 0x0000003f ) >> 0
                                                                        , ( ddr_phy(0x97) & 0x00000fc0 ) >> 6
                                                                        , ( ddr_phy(0x97) & 0x0003f000 ) >> 12
                                                                        , ( ddr_phy(0x97) & 0x00fc0000 ) >> 18
                                                                        );
        printk( "                          B3RBDL D0:0x%2x D1:0x%2x D2:0x%2x D3:0x%2x D4:0x%2x D5:0x%2x D6:0x%2x D7:0x%2x DM:0x%2x\n"
                                                                        , ( ddr_phy(0xA6) & 0x0000003f ) >> 0
                                                                        , ( ddr_phy(0xA6) & 0x00000fc0 ) >> 6
                                                                        , ( ddr_phy(0xA6) & 0x0003f000 ) >> 12
                                                                        , ( ddr_phy(0xA6) & 0x00fc0000 ) >> 18
                                                                        , ( ddr_phy(0xA6) & 0x3f000000 ) >> 24

                                                                        , ( ddr_phy(0xA7) & 0x0000003f ) >> 0
                                                                        , ( ddr_phy(0xA7) & 0x00000fc0 ) >> 6
                                                                        , ( ddr_phy(0xA7) & 0x0003f000 ) >> 12
                                                                        , ( ddr_phy(0xA7) & 0x00fc0000 ) >> 18
                                                                        );

        printk( "4. Write Delay          - ");


        printk( "WDD      B0:0x%8x B1:0x%8x B2:0x%8x B3:0x%8x\n"  , ( ddr_phy(0x79) & 0x000000ff ) >> 0
                                                                        , ( ddr_phy(0x89) & 0x000000ff ) >> 0
                                                                        , ( ddr_phy(0x99) & 0x000000ff ) >> 0
                                                                        , ( ddr_phy(0xA9) & 0x000000ff ) >> 0
                                                                        );


        printk( "                          B0WBDL D0:0x%2x D1:0x%2x D2:0x%2x D3:0x%2x D4:0x%2x D5:0x%2x D6:0x%2x D7:0x%2x DM:0x%2x\n" // DS:0x%2x\n"
                                                                        , ( ddr_phy(0x73) & 0x0000003f ) >> 0
                                                                        , ( ddr_phy(0x73) & 0x00000fc0 ) >> 6
                                                                        , ( ddr_phy(0x73) & 0x0003f000 ) >> 12
                                                                        , ( ddr_phy(0x73) & 0x00fc0000 ) >> 18
                                                                        , ( ddr_phy(0x73) & 0x3f000000 ) >> 24

                                                                        , ( ddr_phy(0x74) & 0x0000003f ) >> 0
                                                                        , ( ddr_phy(0x74) & 0x00000fc0 ) >> 6
                                                                        , ( ddr_phy(0x74) & 0x0003f000 ) >> 12
                                                                        , ( ddr_phy(0x74) & 0x00fc0000 ) >> 18
                                //                                        , ( ddr_phy(0x74) & 0x3f000000 ) >> 24
                                                                        );

        printk( "                          B1WBDL D0:0x%2x D1:0x%2x D2:0x%2x D3:0x%2x D4:0x%2x D5:0x%2x D6:0x%2x D7:0x%2x DM:0x%2x\n" // DS:0x%2x\n"
                                                                        , ( ddr_phy(0x83) & 0x0000003f ) >> 0
                                                                        , ( ddr_phy(0x83) & 0x00000fc0 ) >> 6
                                                                        , ( ddr_phy(0x83) & 0x0003f000 ) >> 12
                                                                        , ( ddr_phy(0x83) & 0x00fc0000 ) >> 18
                                                                        , ( ddr_phy(0x83) & 0x3f000000 ) >> 24

                                                                        , ( ddr_phy(0x84) & 0x0000003f ) >> 0
                                                                        , ( ddr_phy(0x84) & 0x00000fc0 ) >> 6
                                                                        , ( ddr_phy(0x84) & 0x0003f000 ) >> 12
                                                                        , ( ddr_phy(0x84) & 0x00fc0000 ) >> 18
                                //                                        , ( ddr_phy(0x84) & 0x3f000000 ) >> 24
                                                                        );

        printk( "                          B2WBDL D0:0x%2x D1:0x%2x D2:0x%2x D3:0x%2x D4:0x%2x D5:0x%2x D6:0x%2x D7:0x%2x DM:0x%2x\n" // DS:0x%2x\n"
                                                                        , ( ddr_phy(0x93) & 0x0000003f ) >> 0
                                                                        , ( ddr_phy(0x93) & 0x00000fc0 ) >> 6
                                                                        , ( ddr_phy(0x93) & 0x0003f000 ) >> 12
                                                                        , ( ddr_phy(0x93) & 0x00fc0000 ) >> 18
                                                                        , ( ddr_phy(0x93) & 0x3f000000 ) >> 24

                                                                        , ( ddr_phy(0x94) & 0x0000003f ) >> 0
                                                                        , ( ddr_phy(0x94) & 0x00000fc0 ) >> 6
                                                                        , ( ddr_phy(0x94) & 0x0003f000 ) >> 12
                                                                        , ( ddr_phy(0x94) & 0x00fc0000 ) >> 18
                                //                                        , ( ddr_phy(0x94) & 0x3f000000 ) >> 24
                                                                        );
        printk( "                          B3WBDL D0:0x%2x D1:0x%2x D2:0x%2x D3:0x%2x D4:0x%2x D5:0x%2x D6:0x%2x D7:0x%2x DM:0x%2x\n" // DS:0x%2x\n"
                                                                        , ( ddr_phy(0xA3) & 0x0000003f ) >> 0
                                                                        , ( ddr_phy(0xA3) & 0x00000fc0 ) >> 6
                                                                        , ( ddr_phy(0xA3) & 0x0003f000 ) >> 12
                                                                        , ( ddr_phy(0xA3) & 0x00fc0000 ) >> 18
                                                                        , ( ddr_phy(0xA3) & 0x3f000000 ) >> 24

                                                                        , ( ddr_phy(0xA4) & 0x0000003f ) >> 0
                                                                        , ( ddr_phy(0xA4) & 0x00000fc0 ) >> 6
                                                                        , ( ddr_phy(0xA4) & 0x0003f000 ) >> 12
                                                                        , ( ddr_phy(0xA4) & 0x00fc0000 ) >> 18
                                //                                        , ( ddr_phy(0xA4) & 0x3f000000 ) >> 24
                                                                        );

	printk( "\n");

	dbg_timer.expires = jiffies + msecs_to_jiffies(1000);
	add_timer(&dbg_timer);
}

void __init tcc8930_init_gpio(void)
{
	if (!machine_is_tcc893x())
		return;

	#if defined(CONFIG_DAUDIO_PRINT_DRAM_DELAY)
	ddr_phy(0x02) |= 0x3f;

	init_timer(&dbg_timer);
	dbg_timer.function = dbg_timer_handler;

	dbg_timer.expires = jiffies + msecs_to_jiffies(1000);
	add_timer(&dbg_timer);
	#endif

	#if defined(CONFIG_DAUDIO)
		return;
	#else
		i2c_register_board_info(0, i2c_devices_expander, ARRAY_SIZE(i2c_devices_expander));
	#endif

	return;
}
