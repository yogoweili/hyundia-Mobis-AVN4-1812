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

#ifndef __LINUX_MFD_LTC3676_CORE_H_
#define __LINUX_MFD_LTC3676_CORE_H_

#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/regulator/driver.h>

/* regulator supplies for Anatop */
/** 
 * @author back_yak@cleinsoft
 * @data 2013/12/02
 * ltc3676_regulator_renaming
 **/
 /* D-Audio Renaming
enum ltc3676_regulator_supplies {
	LTC3676_VSW1,
	LTC3676_VSW2,
	LTC3676_VSW3,
	LTC3676_VSW4,
	LTC3676_SUPPLY_NUM
};
*/
enum ltc3676_regulator_supplies {
	LTC3676_BUCK_CH1,
	LTC3676_BUCK_CH2,
	LTC3676_BUCK_CH3,
	LTC3676_BUCK_CH4,
	LTC3676_SUPPLY_NUM
};
extern struct i2c_client *ltc3676_client;

struct ltc3676_regulator;
struct regulator_init_data;

struct ltc3676_platform_data {
	int (*init)(struct ltc3676_regulator *);
};

struct ltc3676_pmic {
	/* regulator devices */
	struct platform_device *pdev[LTC3676_SUPPLY_NUM];
};

struct ltc3676_regulator_data {
	char name[80];
	char *parent_name;
	int (*reg_register)(struct ltc3676_regulator *sreg);
	int (*set_voltage)(struct ltc3676_regulator *sreg, int uv);
	int (*get_voltage)(struct ltc3676_regulator *sreg);
	int (*set_current)(struct ltc3676_regulator *sreg, int uA);
	int (*get_current)(struct ltc3676_regulator *sreg);
	int (*enable)(struct ltc3676_regulator *sreg);
	int (*disable)(struct ltc3676_regulator *sreg);
	int (*is_enabled)(struct ltc3676_regulator *sreg);
	int (*set_mode)(struct ltc3676_regulator *sreg, int mode);
	int (*get_mode)(struct ltc3676_regulator *sreg);
	int (*get_optimum_mode)(struct ltc3676_regulator *sreg,
			int input_uV, int output_uV, int load_uA);
	u32 control_reg;
	int vol_bit_shift;
	int vol_bit_mask;
	int min_bit_val;
	int min_voltage;
	int max_voltage;
	int max_current;
	struct regulation_constraints *cnstraints;
};


struct ltc3676_regulator {
	struct regulator_desc regulator;
	struct ltc3676_regulator *parent;
	struct ltc3676_regulator_data *rdata;
	struct completion done;

	spinlock_t         lock;
	wait_queue_head_t  wait_q;
	struct notifier_block nb;

	int rev;		/* chip revision */

	struct device *dev;
	/* device IO */
	struct i2c_client *i2c_client;
	int (*read_dev)(struct ltc3676_regulator *ltc, u8 reg);
	int (*write_dev)(struct ltc3676_regulator *ltc, u8 reg, u8 val);
	u16 *reg_cache;

	/* Client devices */
	struct ltc3676_pmic pmic;
};

int ltc3676_register_regulator(
		struct ltc3676_regulator *reg_data, int reg,
		      struct regulator_init_data *initdata);


#endif
