/*
 * Copyright 2013 Linear Technology Corp. All rights reserved.
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/**
 * @file pmic_ltc3676.c
 * @author woody@cleinsoft
 * @brief This is the board file with LTC3676-1 PMIC regulator driver.
 * origin is mx6x_novtech_pmic_ltc3676.c  --  NOVPEKi.MX6x Driver for Linear LTC3676-1
 * PMIC
 **/

#include <linux/device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/ltc3676.h>
#include <linux/mfd/ltc3676/core.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/machine.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <mach/irqs.h>
#include <mach/gpio.h>

//STOP LOOK HERE
//
//LTC3676 and LTC3676-1 have different I2C addresses
#ifdef LTC3676_1
#define LTC_I2C_ADDR 	0x3D
#else
#define LTC_I2C_ADDR 	0x3C
#endif

#if defined(CONFIG_DAUDIO_20140220)
	#define LTC_REG_GPIO	TCC_GPC(20)
#endif

static int get_voltage(struct ltc3676_regulator *sreg)
{
	return 0;
}

static int set_voltage(struct ltc3676_regulator *sreg, int uv)
{
	return 0;
}

static int enable(struct ltc3676_regulator *sreg)
{
	printk(KERN_INFO "%s: LTC3676 enable \n", __func__);
	return 1;
}

static int disable(struct ltc3676_regulator *sreg)
{
	return 0;
}

static int is_enabled(struct ltc3676_regulator *sreg)
{
	return 1;
}

// min/max voltage are board and processor specific for boundary checking
// sw1 (buck1) is DDR VTT tracking supply on LTC3676-1
// Tracks 1/2 DDR VDDQ which is nominally 1.5V so VTT is 0.75V
static struct ltc3676_regulator_data sw1_data = {
	//.name		= "SW1",		<= renaming to CPU_BLOCK
	.name		= "CPU_BLOCK",
	.set_voltage	= set_voltage,
	.get_voltage	= get_voltage,
	.enable		= enable,
	.disable	= disable,
	.is_enabled	= is_enabled,
	.min_voltage	= 700000,
	.max_voltage	= 800000,
};

// sw2 (buck2) is VDDSOC on NovPek board. Nominally 1.375V and adjustable in software
static struct ltc3676_regulator_data sw2_data = {
	//.name		= "vddsoc_ext",	<= renaming to GPIO_BLOCK
	.name		= "GPIO_BLOCK",
	.set_voltage	= set_voltage,
	.get_voltage	= get_voltage,
	.enable		= enable,
	.disable	= disable,
	.is_enabled	= is_enabled,
	.min_voltage	= 925000,
	.max_voltage	= 1500000,
};

// sw3 (buck3) is VDDARM on NovPek board. Nominally 1.375V and adjustable in software
static struct ltc3676_regulator_data sw3_data = {
	//.name		= "vddarm_ext",	<= renaming to CORE_BLOCK
	.name		= "CORE_BLOCK",
	.set_voltage	= set_voltage,
	.get_voltage	= get_voltage,
	.enable		= enable,
	.disable	= disable,
	.is_enabled	= is_enabled,
	.min_voltage	= 925000,
	.max_voltage	= 1500000,
};

// sw4 (buck4) is DDR (VDDQ) +1.5V on NovPek board. Generally not adjusted.
static struct ltc3676_regulator_data sw4_data = {
	//.name		= "SW4",		<= renaming to DDR_BLOCK
	.name		= "DDR_BLOCK",
	.set_voltage	= set_voltage,
	.get_voltage	= get_voltage,
	.enable		= enable,
	.disable	= disable,
	.is_enabled	= is_enabled,
	.min_voltage	= 1425000,
	.max_voltage	= 1575000,
};

// LDO1 set to 3.3V on NovPek Board. Not adjustable by software.
static struct ltc3676_regulator_data ldo1_stby_data = {
	.name		= "LDO1_STBY",
	.set_voltage	= set_voltage,
	.get_voltage	= get_voltage,
	.enable		= enable,
	.disable	= disable,
	.is_enabled	= is_enabled,
	.min_voltage	= 3000000,
	.max_voltage	= 3600000,
};

// LDO2 set to 1.1V on NovPek Board. Not adjustable by software
static struct ltc3676_regulator_data ldo2_data = {
	.name		= "LDO2",
	.set_voltage	= set_voltage,
	.get_voltage	= get_voltage,
	.enable		= enable,
	.disable	= disable,
	.is_enabled	= is_enabled,
	.min_voltage	= 1000000,
	.max_voltage	= 1200000,
};

// LDO3 always set at 1.8V.  Never adjustable.
static struct ltc3676_regulator_data ldo3_data = {
	.name		= "LDO3",
	.set_voltage	= set_voltage,
	.get_voltage	= get_voltage,
	.enable		= enable,
	.disable	= disable,
	.is_enabled	= is_enabled,
	.min_voltage	= 1700000,
	.max_voltage	= 1900000,
};

// LDO4 set to 2.5V on NovPek Board. Four software select values on LTC3676-1.
// With LTC3676 (no -1) LDO4 is set with resistor divider and not software adjustable.
static struct ltc3676_regulator_data ldo4_data = {
	.name		= "LDO4",
	.set_voltage	= set_voltage,
	.get_voltage	= get_voltage,
	.enable		= enable,
	.disable	= disable,
	.is_enabled	= is_enabled,
	.min_voltage	= 1200000, // selectable min for LTC3676-1
	.max_voltage	= 3000000, // selectable max for LTC3676-1
};

static struct regulator_consumer_supply sw1_consumers[] = {
	{
		//.supply = "SW1",			<= renaming to CPU_BLOCK
		.supply = "CPU_BLOCK",
	},
};

static struct regulator_consumer_supply sw2_consumers[] = {
	{
		//.supply = "vddsoc_ext",	<= renaming to GPIO_BLOCK
		.supply = "GPIO_BLOCK",
	},
};

static struct regulator_consumer_supply sw3_consumers[] = {
	{
		//.supply = "vddarm_ext",	<= renaming to CORE_BLOCK
		.supply = "CORE_BLOCK",
	},
};

static struct regulator_consumer_supply sw4_consumers[] = {
	{
		//.supply = "SW4",			<= renaming to DDR_BLOCK
		.supply = "DDR_BLOCK",
	},
};

static struct regulator_consumer_supply ldo1_stby_consumers[] = {
	{
		.supply = "LDO1_STBY",
	},
};

static struct regulator_consumer_supply ldo2_consumers[] = {
	{
		.supply = "LDO2",
	},
};

static struct regulator_consumer_supply ldo3_consumers[] = {
	{
		.supply = "LDO3",
	},
};

static struct regulator_consumer_supply ldo4_consumers[] = {
	{
		.supply = "LDO4",
	},
};

static struct regulator_init_data sw1_init = {
	.constraints = {
		//.name			= "SW1",		<= renaming to CPU_BLOCK
		.name			= "CPU_BLOCK",
		.min_uV			= 700000,
		.max_uV			= 800000,
		.valid_modes_mask	= REGULATOR_MODE_FAST |
					  REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE |
					  REGULATOR_CHANGE_MODE,
		.always_on		= 1,
	},
	.num_consumer_supplies = ARRAY_SIZE(sw1_consumers),
	.consumer_supplies = &sw1_consumers[0],
};

static struct regulator_init_data sw2_init = {
	.constraints = {
		//.name			= "vddsoc_ext",	<= renaming to GPIO_BLOCK
		.name			= "GPIO_BLOCK",
		.min_uV			= 925000,
		.max_uV			= 1500000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_MODE,
		.always_on		= 1,
	},
	.num_consumer_supplies = ARRAY_SIZE(sw2_consumers),
	.consumer_supplies = &sw2_consumers[0],
};

static struct regulator_init_data sw3_init = {
	.constraints = {
		//.name			= "vddarm_ext",	<= renaming to CORE_BLOCK
		.name			= "CORE_BLOCK",
		.min_uV			= 925000,
		.max_uV			= 1500000,
		.valid_modes_mask	= REGULATOR_MODE_FAST |
					  REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE |
					  REGULATOR_CHANGE_MODE,
		.always_on		= 1,
	},
	.num_consumer_supplies = ARRAY_SIZE(sw3_consumers),
	.consumer_supplies = &sw3_consumers[0],
};

static struct regulator_init_data sw4_init = {
	.constraints = {
		//.name			= "SW4",		<= renaming to DDR_BLOCK
		.name			= "DDR_BLOCK",
		.min_uV			= 1425000,
		.max_uV			= 1575000,
		.valid_modes_mask	= REGULATOR_MODE_FAST |
					  REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE |
					  REGULATOR_CHANGE_MODE,
		.always_on		= 1,
	},
	.num_consumer_supplies = ARRAY_SIZE(sw4_consumers),
	.consumer_supplies = &sw4_consumers[0],
};

static struct regulator_init_data ldo1_stby_init = {
	.constraints = {
		.name			= "LDO1_STBY",
		.min_uV			= 3000000,
		.max_uV			= 3600000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_MODE,
		.always_on		= 1,
	},
	.num_consumer_supplies = ARRAY_SIZE(ldo1_stby_consumers),
	.consumer_supplies = &ldo1_stby_consumers[0],
};

static struct regulator_init_data ldo2_init = {
	.constraints = {
		.name			= "LDO2",
		.min_uV			= 1000000,
		.max_uV			= 1200000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_MODE,
		.always_on		= 1,
	},
	.num_consumer_supplies = ARRAY_SIZE(ldo2_consumers),
	.consumer_supplies = &ldo2_consumers[0],
};

static struct regulator_init_data ldo3_init = {
	.constraints = {
		.name			= "LDO3",
		.min_uV			= 1700000,
		.max_uV			= 1900000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_MODE,
		.always_on		= 1,
	},
	.num_consumer_supplies = ARRAY_SIZE(ldo3_consumers),
	.consumer_supplies = &ldo3_consumers[0],
};

static struct regulator_init_data ldo4_init = {
	.constraints = {
		.name			= "LDO4",
		.min_uV			= 1200000,
		.max_uV			= 3000000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_MODE,
		.always_on		= 1,
	},
	.num_consumer_supplies = ARRAY_SIZE(ldo4_consumers),
	.consumer_supplies = &ldo4_consumers[0],
};

static struct ltc3676_regulator sw1_reg = {
		.rdata = &sw1_data,
};

static struct ltc3676_regulator sw2_reg = {
		.rdata = &sw2_data,
};

static struct ltc3676_regulator sw3_reg = {
		.rdata = &sw3_data,
};


static struct ltc3676_regulator sw4_reg = {
		.rdata = &sw4_data,
};

static struct ltc3676_regulator ldo1_stby_reg = {
		.rdata = &ldo1_stby_data,
};

static struct ltc3676_regulator ldo2_reg = {
		.rdata = &ldo2_data,
};

static struct ltc3676_regulator ldo3_reg = {
		.rdata = &ldo3_data,
};

static struct ltc3676_regulator ldo4_reg = {
		.rdata = &ldo4_data,
};

static int __init regulators_init(struct ltc3676_regulator *ltc3676)
{
	/**
	 * @author back_yak@cleinsoft
	 * @date 2013/11/18
	 * LTC3676 Buck Channel init
	 **/
	sw1_reg.i2c_client = ltc3676->i2c_client;
	sw1_reg.read_dev = ltc3676->read_dev;
	sw1_reg.write_dev = ltc3676->write_dev;

	sw2_reg.i2c_client = ltc3676->i2c_client;
	sw2_reg.read_dev = ltc3676->read_dev;
	sw2_reg.write_dev = ltc3676->write_dev;

	sw3_reg.i2c_client = ltc3676->i2c_client;
	sw3_reg.read_dev = ltc3676->read_dev;
	sw3_reg.write_dev = ltc3676->write_dev;

	sw4_reg.i2c_client = ltc3676->i2c_client;
	sw4_reg.read_dev = ltc3676->read_dev;
	sw4_reg.write_dev = ltc3676->write_dev;

	//ONLY the ARM and SOC rails are init registered, since others are not changeable by Linux

	ltc3676_register_regulator(&sw1_reg, LTC3676_BUCK_CH1, &sw1_init);
	ltc3676_register_regulator(&sw2_reg, LTC3676_BUCK_CH2, &sw2_init);
	ltc3676_register_regulator(&sw3_reg, LTC3676_BUCK_CH3, &sw3_init);
	ltc3676_register_regulator(&sw4_reg, LTC3676_BUCK_CH4, &sw4_init);

	// D-Audio not need.
	/*
	ltc3676_register_regulator(&ldo1_stby_reg, LTC3676_LDO1_STBY, &ldo1_stby_init);
	ltc3676_register_regulator(&ldo2_reg, LTC3676_LDO2, &ldo2_init);
	ltc3676_register_regulator(&ldo3_reg, LTC3676_LDO3, &ldo3_init);
	ltc3676_register_regulator(&ldo4_reg, LTC3676_LDO4, &ldo4_init);
	*/
	return 0;
}

static struct ltc3676_platform_data __initdata ltc3676_plat = {
	.init = regulators_init,
};

static struct i2c_board_info __initdata ltc3676_i2c_device = {
	I2C_BOARD_INFO("ltc3676", LTC_I2C_ADDR),
	.irq = gpio_to_irq(LTC_REG_GPIO),   //Please review IRQ declaration at the beginning of file
	.platform_data = &ltc3676_plat,
};

int __init novpek6_init_i2c(void)
{
	/**
	 * @author sjpark@cleinsoft
	 * @date 2013/07/25
	 *
	 * i2c master channel 0
	 *
	 * @author back_yak@cleinsoft
	 * @date 2014/11/15
	 * i2c master channel 0 -> 2
     **/
	return i2c_register_board_info(2, &ltc3676_i2c_device, 1);
}
subsys_initcall(novpek6_init_i2c);

static __init int ltc3676_pmic_init(void)
{
	int i = 0;
	int ret = 0;
	struct regulator *reg;

	/* D-Audio Renaming
	char *ltc3676_global_regulator[] = {
		"SW1",
		"vddsoc_ext",
		"vddarm_ext",
		"SW4",
//		"LDO1_STBY",
//		"LDO2",
//		"LDO3",
//		"LDO4",
	};
	*/
	char *ltc3676_global_regulator[] = {
		"CPU_BLOCK",
		"GPIO_BLOCK",
		"CORE_BLOCK",
		"DDR_BLOCK",
	};

	printk(KERN_INFO "%s: LTC3676 pmic init\n", __func__);

	{
		if (IS_ERR(reg)) {
			printk(KERN_ERR "%s: LTC can't get regulator %s.\n", __func__, ltc3676_global_regulator[i]);
			//goto err0;
		} else {
			printk(KERN_INFO "%s: Got %s regulator\n", __func__,ltc3676_global_regulator[i] );
			ret = regulator_enable(reg);
			if (ret) {
				printk(KERN_INFO "%s: Regulator Enable error %d.\n", __func__, ret);
				//goto err0;
			}
			else
				printk(KERN_INFO "%s: Regulator %s Enable.\n",__func__, ltc3676_global_regulator[i]);

			ret = regulator_get_mode(reg);
			if (ret < 0) {
				printk(KERN_INFO "%s: Regulator Get Mode error %d.\n", __func__, ret);
				//goto err0;
			}
			else
				printk(KERN_INFO "%s: Regulator %s Mode = %d.\n",__func__, ltc3676_global_regulator[i], ret);

			ret = regulator_get_voltage(reg);
			if (!ret) {
				printk(KERN_INFO "%s: Regulator Get Voltage error %d.\n", __func__, ret);
				//goto err0;
			}
			else
				printk(KERN_INFO "%s: Regulator %s Voltage = %d.\n",__func__, ltc3676_global_regulator[i], ret);
			msleep(900);
		}
		i++;
	}

err0:
	reg = NULL;

	return ret;
}

late_initcall(ltc3676_pmic_init);
