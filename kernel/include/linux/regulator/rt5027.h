/*
 * rt5027.h  --  Voltage regulation for the KrossPower RT5027
 *
 * Copyright (C) 2013 by Telechips
 *
 * This program is free software
 *
 */

#ifndef REGULATOR_RT5027
#define REGULATOR_RT5027

#include <linux/regulator/machine.h>

typedef enum {
	RT5027_ID_DCDC1 = 0,
	RT5027_ID_DCDC2,
	RT5027_ID_DCDC3,
	RT5027_ID_LDO1,
	RT5027_ID_LDO2,
	RT5027_ID_LDO3,
	RT5027_ID_LDO4,
	RT5027_ID_LDO5,
	RT5027_ID_LDO6,
	RT5027_ID_LDO7,
	RT5027_ID_LDO8,	
	RT5027_ID_BOOST,	
	RT5027_ID_MAX
};

/**
 * RT5027_subdev_data - regulator data
 * @id: regulator Id (either RT5027_ID_DCDC2 or RT5027_ID_DCDC3 or ,,,)
 * @name: regulator cute name (example for RT5027_ID_DCDC2: "vcc_core")
 * @platform_data: regulator init data (constraints, supplies, ...)
 */
struct rt5027_subdev_data {
	int                         id;
	char                        *name;
	struct regulator_init_data  *platform_data;
};

/**
 * rt5027_platform_data - platform data for rt5027
 * @num_subdevs: number of regulators used (may be 1 or 2 or ,,)
 * @subdevs: regulator used
 *           At most, there will be a regulator for RT5027_ID_DCDC2 and one for RT5027_ID_LDO5 voltages.
 * @init_irq: main chip irq initialize setting.
 */
struct rt5027_platform_data {
	int num_subdevs;
	struct rt5027_subdev_data *subdevs;
	int (*init_port)(int irq_num);
};

enum {
	RT5027_CHG_CURR_300mA = 0,
	RT5027_CHG_CURR_400mA,
	RT5027_CHG_CURR_500mA,
	RT5027_CHG_CURR_600mA,
	RT5027_CHG_CURR_700mA,
	RT5027_CHG_CURR_800mA,
	RT5027_CHG_CURR_900mA,
	RT5027_CHG_CURR_1000mA,
	RT5027_CHG_CURR_1100mA,
	RT5027_CHG_CURR_1200mA,
	RT5027_CHG_CURR_1300mA,
	RT5027_CHG_CURR_1400mA,
	RT5027_CHG_CURR_1500mA,
	RT5027_CHG_CURR_1600mA,
	RT5027_CHG_CURR_1700mA,
	RT5027_CHG_CURR_1800mA,
	RT5027_CHG_CURR_MAX
};

#define RT5027_CHG_OK			0x02
#define RT5027_CHG_ING			0x01
#define RT5027_CHG_NONE			0x00

extern int rt5027_battery_voltage(void);
extern int rt5027_acin_detect(void);
extern int rt5027_vbus_voltage(void);
extern void rt5027_power_off(void);
extern void rt5027_charge_current(unsigned char val);
extern int rt5027_charge_status(void);
#endif

