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

/*!
 * @file ltc3676-i2c.c
 * @brief This is the generic I2C driver for Linear 3676 PMIC.
 *
 * @ingroup PMIC_CORE
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/mfd/ltc3676/core.h>
#include <linux/regulator/ltc3676.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

static inline int ltc3676_i2c_read(struct ltc3676_regulator *ltc, u8 reg)
{
	return i2c_smbus_read_byte_data(ltc->i2c_client, reg);
}

static inline int ltc3676_i2c_write(struct ltc3676_regulator *ltc, u8 reg, u8 val)
{
	return i2c_smbus_write_byte_data(ltc->i2c_client, reg, val);
}

static int ltc3676_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct ltc3676_regulator *ltc3676;
	struct ltc3676_platform_data *plat_data = i2c->dev.platform_data;
	int ret = 0;

	ltc3676 = kzalloc(sizeof(struct ltc3676_regulator), GFP_KERNEL);
	if (ltc3676 == NULL)
		return -ENOMEM;

	i2c_set_clientdata(i2c, ltc3676);
	ltc3676->dev = &i2c->dev;
	ltc3676->i2c_client = i2c;
	ltc3676->read_dev = ltc3676_i2c_read;
	ltc3676->write_dev = ltc3676_i2c_write;

	// YHN, checking if we can access the LTC3676 vi
	ltc3676_i2c_write(ltc3676, 0x14, 0x5A);

	if (ltc3676_i2c_read(ltc3676, 0x14) != 0x5A) {
		kfree(ltc3676);
		return -ENODEV;
	}

	/* so far, we got matched chip on board */
	ltc3676_client = i2c;
	if (plat_data && plat_data->init) {
		ret = plat_data->init(ltc3676);
		if (ret != 0)
			return -EINVAL;
	}

	dev_info(&i2c->dev, "Loaded\n");

	return 0;
}

static int ltc3676_i2c_remove(struct i2c_client *i2c)
{
	struct ltc3676_regulator *ltc3676 = i2c_get_clientdata(i2c);

	kfree(ltc3676);
	return 0;
}

static const struct i2c_device_id ltc3676_id[] = {
       { "ltc3676", 0 },
       { }
};
MODULE_DEVICE_TABLE(i2c, ltc3676_id);


static struct i2c_driver ltc3676_i2c_driver = {
	.driver = {
		   .name = "ltc3676",
		   .owner = THIS_MODULE,
	},
	.probe = ltc3676_i2c_probe,
	.remove = ltc3676_i2c_remove,
	.id_table = ltc3676_id,
};

static int __init ltc3676_i2c_init(void)
{
	return i2c_add_driver(&ltc3676_i2c_driver);
}
/* init early so consumer devices can complete system boot */
subsys_initcall(ltc3676_i2c_init);

static void __exit ltc3676_i2c_exit(void)
{
	i2c_del_driver(&ltc3676_i2c_driver);
}
module_exit(ltc3676_i2c_exit);

MODULE_AUTHOR("Linear Technology and NovTech, Inc.");
MODULE_DESCRIPTION("I2C support for the Linear LTC3676 PMIC");
MODULE_LICENSE("GPL");
