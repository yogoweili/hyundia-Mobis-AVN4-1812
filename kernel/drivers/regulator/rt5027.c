/*
 * rt5027.c  --  Voltage and current regulation for the RT5027
 *
 * Copyright (C) 2013 by Telechips
 *
 * This program is free software;
 *
 */
#include <linux/module.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <asm/mach-types.h>

#include <linux/regulator/rt5027.h>

#if defined(DEBUG)
#define dbg(msg...) printk("RT5027: " msg)
#else
#define dbg(msg...)
#endif

#define RT5027_MIN_UV   700000
#define RT5027_MAX_UV  3500000

/********************************************************************
	Define Types
********************************************************************/
struct rt5027_voltage_t {
	int uV;
	u8  val;
};

struct rt5027_data {
	struct i2c_client *client;
	unsigned int min_uV;
	unsigned int max_uV;
	struct work_struct work;
#if defined(CONFIG_REGULATOR_RT5027_PEK)
	struct input_dev *input_dev;
	struct timer_list timer;
#endif
	struct regulator_dev *rdev[0];
};


/********************************************************************
	I2C Command & Values
********************************************************************/
//config & setting
#define RT5027_DEVICE_ID_REG 					0x00
#define RT5027_RESET_CONTROL_REG				0x15
#define RT5027_POK_TIME_SETTING_REG			0x19
#define RT5027_SHDN_CONTROL_REG				0x1A
#define RT5027_POWEROFF_CONTROL_REG			0x1B
#define RT5027_OFF_MODE_CONFIG_REG			0x5D

// DC/DC
#define RT5027_BUCK1_CONTROL_REG				0x08
#define RT5027_BUCK2_CONTROL_REG				0x09
#define RT5027_BUCK3_CONTROL_REG				0x0A
#define RT5027_BOOST_REG						0x0B
#define RT5027_BUCK_MODE_REG					0x0C
#define RT5027_VSYS_BUCK_ENABLE_REG			0x17	

// LDO
#define RT5027_LDO2_CONTROL_REG				0x0D
#define RT5027_LDO3_CONTROL_REG				0x0E
#define RT5027_LDO4_CONTROL_REG				0x0F
#define RT5027_LDO5_CONTROL_REG				0x10
#define RT5027_LDO6_CONTROL_REG				0x11
#define RT5027_LDO7_CONTROL_REG				0x12
#define RT5027_LDO1_8_CONTROL_REG				0x13
#define RT5027_LDO_MODE_REG					0x14
#define RT5027_LDO_ENABLE_REG					0x18

// GPIO
#define RT5027_GPIO0_FUNCTION_REG				0x1C
#define RT5027_GPIO1_FUNCTION_REG				0x1D
#define RT5027_GPIO2_FUNCTION_REG				0x1E	
#define RT5027_DCDC4_OCP_REG					0xA9

// EVENT & IRQ
#define RT5027_EVENT_STANBY_MODE_REG			0x16
#define RT5027_OFF_EVENT_STATUS_REG			0x20
#define RT5027_IRQ1_ENABLE_REG					0x30
#define RT5027_IRQ1_STATUS_REG					0x31
#define RT5027_IRQ2_ENABLE_REG					0x32
#define RT5027_IRQ2_STATUS_REG					0x33
#define RT5027_IRQ3_ENABLE_REG					0x34
#define RT5027_IRQ3_STATUS_REG					0x35
#define RT5027_IRQ4_ENABLE_REG					0x36
#define RT5027_IRQ4_STATUS_REG					0x37
#define RT5027_IRQ5_ENABLE_REG					0x38
#define RT5027_IRQ5_STATUS_REG					0x39
#define RT5027_IRQ_CONTROL_REG				0x50
#define RT5027_IRQ_FLG_REG						0x51

// CHARGE CONFIG
#define RT5027_CHGCONTROL1_REG				0x01
#define RT5027_CHGCONTROL2_REG				0x02
#define RT5027_CHGCONTROL3_REG				0x03
#define RT5027_CHGCONTROL4_REG				0x04
#define RT5027_CHGCONTROL5_REG				0x05
#define RT5027_CHGCONTROL6_REG				0x06
#define RT5027_CHGCONTROL7_REG				0x07

// VOLTAGE & CURRENT
#define RT5027_VBATS_HIGH_REG					0x58
#define RT5027_VBATS_LOW_REG					0x59
#define RT5027_INTEMP_HIGH_REG					0x5A
#define RT5027_INTEMP_LOW_REG					0x5B
#define RT5027_RSVD2_REG						0x5C
#define RT5027_AIN_VOLTAGE_HIGH_REG			0x5E
#define RT5027_AIN_VOLTAGE_LOW_REG			0x5F
#define RT5027_ACIN_VOLTAGE_HIGH_REG			0x64
#define RT5027_ACIN_VOLTAGE_LOW_REG			0x65
#define RT5027_VBUS_VOLTAGE_HIGH_REG			0x66
#define RT5027_VBUS_VOLTAGE_LOW_REG			0x67
#define RT5027_VSYS_VOLTAGE_HIGH_REG			0x68
#define RT5027_VSYS_VOLTAGE_LOW_REG			0x69
#define RT5027_GPIO0_VOLTAGE_HIGH_REG		0x6A
#define RT5027_GPIO0_VOLTAGE_LOW_REG			0x6B
#define RT5027_GPIO1_VOLTAGE_HIGH_REG		0x6C
#define RT5027_GPIO1_VOLTAGE_LOW_REG			0x6D
#define RT5027_GPIO2_VOLTAGE_HIGH_REG		0x6E
#define RT5027_GPIO2_VOLTAGE_LOW_REG			0x6F
#define RT5027_BUCK1_VOLTAGE_HIGH_REG		0x70
#define RT5027_BUCK1_VOLTAGE_LOW_REG			0x71
#define RT5027_BUCK2_VOLTAGE_HIGH_REG		0x72
#define RT5027_BUCK2_VOLTAGE_LOW_REG			0x73
#define RT5027_BUCK3_VOLTAGE_HIGH_REG		0x74
#define RT5027_BUCK3_VOLTAGE_LOW_REG			0x75
#define RT5027_VBAT_CURRENT_HIGH_REG			0x76
#define RT5027_VBAT_CURRENT_LOW_REG			0x77

// COULOMB COUNTER
#define RT5027_COULOMB_TIMER_HIGH_REG 		0x60
#define RT5027_COULOMB_TIMER_LOW_REG		0x61
#define RT5027_COULOMB_CHANNEL_HIGH_REG 	0x62
#define RT5027_COULOMB_CHANNEL_LOW_REG		0x63
#define RT5027_COULOMB_COUNTER_CHG_H_H		0x78
#define RT5027_COULOMB_COUNTER_CHG_H_L		0x79
#define RT5027_COULOMB_COUNTER_CHG_L_H		0x7A
#define RT5027_COULOMB_COUNTER_CHG_L_L		0x7B
#define RT5027_COULOMB_COUNTER_DISCHG_H_H		0x7C
#define RT5027_COULOMB_COUNTER_DISCHG_H_L		0x7D
#define RT5027_COULOMB_COUNTER_DISCHG_L_H		0x7E
#define RT5027_COULOMB_COUNTER_DISCHG_L_L		0x7F

// UNKNOWN
#define RT5027_RSVD1_REG						0x52
#define RT5027_VARLTMAX_REG					0x53
#define RT5027_VARLTMIN1_REG					0x54
#define RT5027_VARLTMIN2_REG					0x55
#define RT5027_TARLTMAX_REG					0x56
#define RT5027_TARLTMIN_REG					0x57

// IRQ values
#define RT5027_IRQ1_ACIN_OVP					0x80
#define RT5027_IRQ1_ACIN_CONNECTED			0x40
#define RT5027_IRQ1_ACIN_REMOVED				0x20
#define RT5027_IRQ1_VBUS_OVP					0x10
#define RT5027_IRQ1_VBUS_CONNECTED			0x08
#define RT5027_IRQ1_VBUS_REMOVED				0x04
#define RT5027_IRQ1_BAT_ABSENCE				0x01

#define RT5027_IRQ2_ACIN_SLEEP_MODE			0x80
#define RT5027_IRQ2_ACIN_BAD					0x40
#define RT5027_IRQ2_ACIN_GOOD					0x20
#define RT5027_IRQ2_VBUS_SLEEP_MODE			0x10
#define RT5027_IRQ2_VBUS_BAD					0x08
#define RT5027_IRQ2_VBUS_GOOD					0x04
#define RT5027_IRQ2_BAT_OVP					0x02
#define RT5027_IRQ2_CHARGE_TERMINATED			0x01

#define RT5027_IRQ3_RECHARGE_REQUEST			0x80
#define RT5027_IRQ3_CHARGER_WARNING1			0x40
#define RT5027_IRQ3_CHARGER_WARNING2			0x20
#define RT5027_IRQ3_READY_TO_CHARGE			0x10
#define RT5027_IRQ3_FAST_CHARGE				0x08
#define RT5027_IRQ3_CHG_INSUFFICIENT			0x04
#define RT5027_IRQ3_TIMEOUT_PRECHARGE		0x02
#define RT5027_IRQ3_TIMEOUT_FASTCHARGE		0x01

#define RT5027_IRQ4_OVER_TEMPERATURE			0x80
#define RT5027_IRQ4_BUCK1_LOWER				0x40
#define RT5027_IRQ4_BUCK2_LOWER				0x20
#define RT5027_IRQ4_BUCK3_LOWER				0x10
#define RT5027_IRQ4_POK_SHORT_PRESS			0x08
#define RT5027_IRQ4_POK_LONG_PRESS			0x04
#define RT5027_IRQ4_BOOST_LOWER				0x02
#define RT5027_IRQ4_VSYS_LOWER				0x01

#define RT5027_IRQ5_POK_FORCE_SHDN			0x80
#define RT5027_IRQ5_POK_RISING_EDGE			0x40
#define RT5027_IRQ5_POK_FALLING_EDGE			0x20
#define RT5027_IRQ5_RSTINB_DEGLITCH			0x10
#define RT5027_IRQ5_GPIO2_INPUT_EDGE			0x08
#define RT5027_IRQ5_GPIO1_INPUT_EDGE			0x04
#define RT5027_IRQ5_GPIO0_INPUT_EDGE			0x02



/* initial setting values */

#define RT5027_IRQ1 ( 0 \
		)
		
#define RT5027_IRQ2 ( 0 \
		)
		
#define RT5027_IRQ3 ( 0 \
		)
		
#define RT5027_IRQ4 ( 0 \
		)
		
#define RT5027_IRQ5 ( 0 \
		)

/* LDO1 voltage level */
static struct rt5027_voltage_t ldo1_voltages[] = {
	{ 1500, 0x00 }, { 1800, 0x01 }, { 2500, 0x02 }, { 3000, 0x03 },
	{ 3300, 0x04 }, { 3300, 0x05 }, { 3300, 0x06 }, { 3300, 0x07 },
};

#define NUM_LDO1	ARRAY_SIZE(ldo1_voltages)


/********************************************************************
	Variables
********************************************************************/
static struct workqueue_struct *rt5027_wq;
static struct i2c_client *rt5027_i2c_client = NULL;
static int rt5027_acin_det = 0;
static int rt5027_charge_sts = 0;
static unsigned int rt5027_suspend_status = 0;

/********************************************************************
	Functions (Global)
********************************************************************/
int rt5027_battery_voltage(void)
{
	signed long data[2];
	int ret = 4200;

	data[0] = i2c_smbus_read_byte_data(rt5027_i2c_client, RT5027_VBATS_HIGH_REG);
	data[1] = i2c_smbus_read_byte_data(rt5027_i2c_client, RT5027_VBATS_LOW_REG);
	ret = (data[0] << 8) | data[1];

	//dbg("%s: %dmV\n", __func__, ret);
	return ret;
}

int rt5027_acin_detect(void)
{
	unsigned char value,clear;

	value = i2c_smbus_read_byte_data(rt5027_i2c_client, RT5027_IRQ1_STATUS_REG);
	printk("RT5027 : %x\n", value);
	clear = value | 0x40;
	i2c_smbus_write_byte_data(rt5027_i2c_client, RT5027_IRQ1_STATUS_REG, clear);

	if(value & 0x40)
		return 1;
	else
		return 0;

}

void rt5027_power_off(void)
{
	int value;

	dbg("%s\n", __func__);	
	
	value = i2c_smbus_read_byte_data(rt5027_i2c_client, RT5027_SHDN_CONTROL_REG);
	value = value | 0x80;
	i2c_smbus_write_byte_data(rt5027_i2c_client, RT5027_SHDN_CONTROL_REG, value);
}


#if 0
void rt5027_charge_current(unsigned char curr)
{
	todo:
}

int rt5027_charge_status(void)
{
	todo:
}

int rt5027_get_batt_discharge_current(void)
{
	signed long dischg[2];
	int discharge_cur = 0;
	if (rt5027_i2c_client) {
		dischg[0] = i2c_smbus_read_byte_data(rt5027_i2c_client, RT5027_BATT_DISCHARGE_CURRENT_H_REG);
		dischg[1] = i2c_smbus_read_byte_data(rt5027_i2c_client, RT5027_BATT_DISCHARGE_CURRENT_L_REG);
		discharge_cur = ((dischg[0] << 5) | (dischg[1] & 0x1f))*5/10;
//		dbg("BATT Discharge Current %d\n", discharge_cur);
	}
	return discharge_cur;	

}

int rt5027_get_batt_charge_current(void)
{
	signed long chg[2];
	int charge_cur = 0;
	if (rt5027_i2c_client) {
		chg[0] = i2c_smbus_read_byte_data(rt5027_i2c_client, RT5027_BATT_CHARGE_CURRENT_H_REG);
		chg[1] = i2c_smbus_read_byte_data(rt5027_i2c_client, RT5027_BATT_CHARGE_CURRENT_L_REG);
		charge_cur = ((chg[0] << 4) | (chg[1] & 0x0f))*5/10;
//		dbg("BATT Charge Current %d\n", charge_cur);
	}
	return charge_cur;	
}
#endif

EXPORT_SYMBOL(rt5027_battery_voltage);
EXPORT_SYMBOL(rt5027_acin_detect);
EXPORT_SYMBOL(rt5027_power_off);
//EXPORT_SYMBOL(rt5027_charge_current);
//EXPORT_SYMBOL(rt5027_charge_status);
//EXPORT_SYMBOL(rt5027_get_batt_discharge_current);
//EXPORT_SYMBOL(rt5027_get_batt_charge_current);

/*******************************************
	Functions (Internal)
*******************************************/
static void rt5027_work_func(struct work_struct *work)
{
	struct rt5027_data* rt5027 = container_of(work, struct rt5027_data, work);
	unsigned char data[5];
	
	dbg("%s\n", __func__);

	data[0] =0;
	data[1] =0;
	data[2] =0;
	data[3] =0;
	data[4] =0;
	
	data[0] = i2c_smbus_read_byte_data(rt5027->client, RT5027_IRQ1_STATUS_REG);
	data[1] = i2c_smbus_read_byte_data(rt5027->client, RT5027_IRQ2_STATUS_REG);
	data[2] = i2c_smbus_read_byte_data(rt5027->client, RT5027_IRQ3_STATUS_REG);
	data[3] = i2c_smbus_read_byte_data(rt5027->client, RT5027_IRQ4_STATUS_REG);
	data[4] = i2c_smbus_read_byte_data(rt5027->client, RT5027_IRQ5_STATUS_REG);

	i2c_smbus_write_byte_data(rt5027->client, RT5027_IRQ1_STATUS_REG, 0xFF);
	i2c_smbus_write_byte_data(rt5027->client, RT5027_IRQ2_STATUS_REG, 0xFF);
	i2c_smbus_write_byte_data(rt5027->client, RT5027_IRQ3_STATUS_REG, 0xFF);
	i2c_smbus_write_byte_data(rt5027->client, RT5027_IRQ4_STATUS_REG, 0xFF);
	i2c_smbus_write_byte_data(rt5027->client, RT5027_IRQ5_STATUS_REG, 0xFF);

	if(data[0] & RT5027_IRQ1_ACIN_OVP){
		dbg("ACIN over-voltage, IRQ status\n");
	}
	if (data[0]&RT5027_IRQ1_ACIN_CONNECTED) {
		dbg("ACIN connected, IRQ status\n");
		rt5027_acin_det = 1;
	}
	if (data[0]&RT5027_IRQ1_ACIN_REMOVED) {
		dbg("ACIN removed, IRQ status\n");
		rt5027_acin_det = 0;
		rt5027_charge_sts = RT5027_CHG_NONE;
	}
	if (data[0]&RT5027_IRQ1_VBUS_OVP) {
		dbg("VBUS over-voltage, IRQ status\n");
	}
	if (data[0]&RT5027_IRQ1_VBUS_CONNECTED) {
		dbg("VBUS connected, IRQ status\n");
	}
	if (data[0]&RT5027_IRQ1_VBUS_REMOVED) {
		dbg("VBUS removed, IRQ status\n");
	}
	if (data[0]&RT5027_IRQ1_BAT_ABSENCE) {
		dbg("BAT absence, IRQ status\n");
	}
	if (data[1]&RT5027_IRQ2_ACIN_SLEEP_MODE) {
		dbg("Charger fault. ACIN Sleep mode, IRQ status\n");
	}
	if (data[1]&RT5027_IRQ2_ACIN_BAD) {
		dbg("Charger fault. Bad ACIN, IRQ status\n");
	}
	if (data[1]&RT5027_IRQ2_ACIN_GOOD) {
		dbg("Good ACIN, IRQ status\n");
	}
	if (data[1]&RT5027_IRQ2_VBUS_SLEEP_MODE) {
		dbg("Charger fault. VBUS Sleep mode, IRQ status\n");
	}
	if (data[1]&RT5027_IRQ2_VBUS_BAD) {
		dbg("Charger fault. Bad VBUS, IRQ status\n");
	}
	if (data[1]&RT5027_IRQ2_VBUS_GOOD) {
		dbg("Good VBUS, IRQ status\n");
	}
	if (data[1]&RT5027_IRQ2_BAT_OVP) {
		dbg("Battery over-voltage, IRQ status\n");
	}
	if (data[1]&RT5027_IRQ2_CHARGE_TERMINATED) {
		dbg("Charge terminated , IRQ status\n");
	}
	if (data[2]&RT5027_IRQ3_RECHARGE_REQUEST) {
		dbg("Charge recharge request, IRQ status\n");
	}
	if (data[2]&RT5027_IRQ3_CHARGER_WARNING1) {
		dbg("Charge warning. Thermal regulation loop active, IRQ status\n");
	}
	if (data[2]&RT5027_IRQ3_CHARGER_WARNING2) {
		dbg("Charge warning. Input voltage regulation loop active , IRQ status\n");
	}
	if (data[2]&RT5027_IRQ3_READY_TO_CHARGE) {
		dbg("ACIN or VBUS power good and CHG_BUCK is ready for charging path, IRQ status\n");
	}
	if (data[2]&RT5027_IRQ3_FAST_CHARGE) {
		dbg("Operates in fast-Charge state, IRQ status\n");
	}
	if (data[2]&RT5027_IRQ3_TIMEOUT_PRECHARGE) {
		dbg("Pre-charge timeout. AC plug or USB plug or CHG_EN rising edge can trigger, IRQ status\n");
	}
	if (data[2]&RT5027_IRQ3_TIMEOUT_FASTCHARGE) {
		dbg("Fast charge timeout. AC plug or USB plug or CHG_EN or recharge signal rising edge can clear, IRQ status\n");
	}
	if (data[3]&RT5027_IRQ4_OVER_TEMPERATURE) {
		dbg("Internal over temperature, IRQ status\n");
	}
	if (data[3]&RT5027_IRQ4_BUCK1_LOWER) {
		dbg("Buck1 ouput voltage is lower than 10%, IRQ status\n");
	}
	if (data[3]&RT5027_IRQ4_BUCK2_LOWER) {
		dbg("Buck2 ouput voltage is lower than 10%, IRQ status\n");
	}
	if (data[3]&RT5027_IRQ4_BUCK3_LOWER) {
		dbg("Buck3 ouput voltage is lower than 10%, IRQ status\n");
	}
	if (data[3]&RT5027_IRQ4_POK_SHORT_PRESS) {
#if defined(CONFIG_REGULATOR_RT5027_PEK)
    		del_timer(&rt5027->timer);
		input_event(rt5027->input_dev, EV_KEY, KEY_POWER, 1);
		input_event(rt5027->input_dev, EV_SYN, 0, 0);
		rt5027->timer.expires = jiffies + msecs_to_jiffies(100);
		add_timer(&rt5027->timer);
#endif		
		dbg("POK short press, IRQ status\n");
	}
	if (data[3]&RT5027_IRQ4_POK_LONG_PRESS){
#if defined(CONFIG_REGULATOR_RT5027_PEK)
        del_timer(&rt5027->timer);
		input_event(rt5027->input_dev, EV_KEY, KEY_POWER, 1);
		input_event(rt5027->input_dev, EV_SYN, 0, 0);
		rt5027->timer.expires = jiffies + msecs_to_jiffies(1000);
		add_timer(&rt5027->timer);
#endif		
		dbg("POK long press, IRQ status\n");
	}
	if (data[3]&RT5027_IRQ4_BOOST_LOWER) {
		dbg("BOOST output is lower than 10%, IRQ status\n");
	}
	if (data[3]&RT5027_IRQ4_VSYS_LOWER) {
		dbg("SYS voltage is lower than VOFF, IRQ status\n");
	}
	if (data[4]&RT5027_IRQ5_POK_FORCE_SHDN) {
		dbg("Key-press forced shutdown, IRQ status\n");
	}
	if (data[4]&RT5027_IRQ5_POK_RISING_EDGE) {
		dbg("POK press rising edge, IRQ status\n");
	}
	if (data[4]&RT5027_IRQ5_POK_FALLING_EDGE) {
		dbg("POK press falling edge, IRQ status\n");
	}
	if (data[4]&RT5027_IRQ5_RSTINB_DEGLITCH) {
		dbg("From high to low input into RSTINB pin with over 100ms deglitch time, IRQ status\n");
	}
	if (data[4]&RT5027_IRQ5_GPIO2_INPUT_EDGE) {
		dbg("GPIO2 input edge trigger, IRQ status\n");
	}
	if (data[4]&RT5027_IRQ5_GPIO1_INPUT_EDGE) {
		dbg("GPIO1 input edge trigger, IRQ status\n");
	}
	if (data[4]&RT5027_IRQ5_GPIO0_INPUT_EDGE) {
		dbg("GPIO0 input edge trigger, IRQ status\n");
	}

	enable_irq(rt5027->client->irq);
}

static irqreturn_t rt5027_interrupt(int irqno, void *param)
{
	struct rt5027_data *rt5027 = (struct rt5027_data *)param;
	dbg("%s\n", __func__);

	disable_irq_nosync(rt5027->client->irq);
	queue_work(rt5027_wq, &rt5027->work);
	return IRQ_HANDLED;
}

#if defined(CONFIG_REGULATOR_RT5027_PEK)
static void rt5027_timer_func(unsigned long data)
{
	struct rt5027_data *rt5027 = (struct rt5027_data *)data;
	dbg("%s\n", __func__);

	input_event(rt5027->input_dev, EV_KEY, KEY_POWER, 0);
	input_event(rt5027->input_dev, EV_SYN, 0, 0);
}
#endif

static int rt5027_dcdc_set_voltage(struct regulator_dev *rdev, int min_uV, int max_uV, unsigned *selector)
{
	struct rt5027_data* rt5027 = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	u8 reg, value = 0, old_value = 0;
	int max, step, vrc, ret;

	if(rt5027_suspend_status)
		return -EBUSY;

	switch (id) {
		case RT5027_ID_DCDC1:
			reg = RT5027_BUCK1_CONTROL_REG;
			max = 2275; step = 25; vrc = 2; //(max v : 2.275v, step : 25mv , vrc : 2bit)
			break;
		case RT5027_ID_DCDC2:
			reg = RT5027_BUCK2_CONTROL_REG;
			max = 3500; step = 25; vrc = 1; //(max v : 3.5v, step : 25mv , vrc : 1bit)
			break;
		case RT5027_ID_DCDC3:
			reg = RT5027_BUCK3_CONTROL_REG;
			max = 3500; step = 50; vrc = 2; //(max v : 3.5v, step : 50mv , vrc : 2bit)
			break;
		default:
			return -1;
	}

	min_uV /= 1000;

	if (min_uV > max || min_uV < 700){
		dbg("%s: Wrong BUCK%d value!\n",__func__, id);
		return -EINVAL;
	}

	
	old_value = i2c_smbus_read_byte_data(rt5027->client, reg);
	value = ((min_uV - 700) / step) << vrc;
	value = value | (old_value & (vrc + vrc/2));


	dbg("%s: reg:0x%x value:%dmV\n", __func__, reg, min_uV);

	ret = i2c_smbus_write_byte_data(rt5027->client, reg, value);
	udelay(50);

	return ret;
}

static int rt5027_dcdc_get_voltage(struct regulator_dev *rdev)
{
	struct rt5027_data* rt5027 = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	u8 reg;
	int ret, voltage = 0, max, step, vrc;

	switch (id) {
		case RT5027_ID_DCDC1:
			reg = RT5027_BUCK1_CONTROL_REG;
			max = 2275; step = 25; vrc = 2; //(max v : 2.275v, step : 25mv , vrc : 2bit)
			break;
		case RT5027_ID_DCDC2:
			reg = RT5027_BUCK2_CONTROL_REG;
			max = 3500; step = 25; vrc = 1; //(max v : 3.5v, step : 25mv , vrc : 1bit)
			break;
		case RT5027_ID_DCDC3:
			reg = RT5027_BUCK3_CONTROL_REG;
			max = 3500; step = 50; vrc = 2; //(max v : 3.5v, step : 50mv , vrc : 2bit)
			break;
		default:
			return -1;
	}

	ret = i2c_smbus_read_byte_data(rt5027->client, reg);
	if (ret < 0)
		return -EINVAL;

	voltage = (700 + (ret >> vrc)*step) * 1000;
	
	if ( voltage/1000 > max)
		return -EINVAL;

	dbg("%s: reg:0x%x value:%dmV\n", __func__, reg, voltage/1000);
	return voltage;
}

static struct regulator_ops rt5027_dcdc_ops = {
	.set_voltage = rt5027_dcdc_set_voltage,
	.get_voltage = rt5027_dcdc_get_voltage,
};

static int rt5027_ldo_set_voltage(struct regulator_dev *rdev, int min_uV, int max_uV, unsigned *selector)
{
	struct rt5027_data* rt5027 = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	u8 reg, value = 0;
	u8 step = 0, rsv = 0;
	unsigned int i, min, max;

	switch (id) {
		case RT5027_ID_LDO1:
			break;
		case RT5027_ID_LDO2:
			reg = RT5027_LDO2_CONTROL_REG;	break;
		case RT5027_ID_LDO3:
			reg = RT5027_LDO3_CONTROL_REG;	break;
		case RT5027_ID_LDO4:
			reg = RT5027_LDO4_CONTROL_REG;   break;			
		case RT5027_ID_LDO5:
			reg = RT5027_LDO5_CONTROL_REG;   break;						
		case RT5027_ID_LDO6:			
			reg = RT5027_LDO6_CONTROL_REG;	break;		
		case RT5027_ID_LDO7:			
			reg = RT5027_LDO7_CONTROL_REG;	break;	
		case RT5027_ID_LDO8:			
			reg = RT5027_LDO1_8_CONTROL_REG;	 		
			break;
		default:
			return -1;
	}

	switch(id){
		case RT5027_ID_LDO1:
			break;
		case RT5027_ID_LDO2:
		case RT5027_ID_LDO3:
			min = 700; max = 3500; step = 25; rsv = 0;
			break;
		case RT5027_ID_LDO4:
		case RT5027_ID_LDO5:
		case RT5027_ID_LDO6:			
		case RT5027_ID_LDO7:			
		case RT5027_ID_LDO8:			
			min = 1000; max = 3300; step = 100; rsv = 3;
			break;
		default:
			return -1;
	}

	min_uV /= 1000;

	if(id == RT5027_ID_LDO1){
		for (i = 0; i < NUM_LDO1; i++) {
			if (ldo1_voltages[i].uV >= min_uV) {
				value = ldo1_voltages[i].val;
				break;
			}
		}
	}
	else{
		if(min_uV > max || min_uV < min){
			dbg("%s: Wrong ldo value!\n", __func__);
			return -1;
		}
		value = ((min_uV - min) / step) << rsv;
	}
	
	dbg("%s: reg:0x%x value:%dmV\n", __func__, reg, min_uV);
	return i2c_smbus_write_byte_data(rt5027->client, reg, value);
}

static int rt5027_ldo_get_voltage(struct regulator_dev *rdev)
{
	struct rt5027_data* rt5027 = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	u8 reg, ret;
	u8 step = 0, rsv = 0;
	int i, min, max, voltage;

	switch (id) {
		case RT5027_ID_LDO1:
			break;
		case RT5027_ID_LDO2:
			reg = RT5027_LDO2_CONTROL_REG;	break;
		case RT5027_ID_LDO3:
			reg = RT5027_LDO3_CONTROL_REG;	break;
		case RT5027_ID_LDO4:
			reg = RT5027_LDO4_CONTROL_REG;   break;			
		case RT5027_ID_LDO5:
			reg = RT5027_LDO5_CONTROL_REG;   break;						
		case RT5027_ID_LDO6:			
			reg = RT5027_LDO6_CONTROL_REG;	break;		
		case RT5027_ID_LDO7:			
			reg = RT5027_LDO7_CONTROL_REG;	break;	
		case RT5027_ID_LDO8:			
			reg = RT5027_LDO1_8_CONTROL_REG;	 		
			break;
		default:
			return -1;
	}

	switch(id){
		case RT5027_ID_LDO1:
			break;
		case RT5027_ID_LDO2:
		case RT5027_ID_LDO3:
			min = 700; max = 3500; step = 25; rsv = 0;
			break;
		case RT5027_ID_LDO4:
		case RT5027_ID_LDO5:
		case RT5027_ID_LDO6:			
		case RT5027_ID_LDO7:			
		case RT5027_ID_LDO8:			
			min = 1000; max = 3300; step = 100; rsv = 3;
			break;
		default:
			return -1;
	}

	ret = i2c_smbus_read_byte_data(rt5027->client, reg);
	if (ret < 0)
		return -EINVAL;

	if(id == RT5027_ID_LDO1){
		for (i = 0; i < NUM_LDO1; i++) {
			if (ldo1_voltages[i].val == ret) {
				voltage = ldo1_voltages[i].uV*1000;
				break;
			}
		}
	}
	else{
		voltage = (min + ((ret >> rsv) * step )) * 1000;
		if ( voltage/1000 > max)
			return -EINVAL;
	}

	dbg("%s: reg:0x%x value: %dmV\n", __func__, reg, voltage/1000);
	return voltage;
}

static int rt5027_ldo_enable(struct regulator_dev *rdev)
{
	struct rt5027_data* rt5027 = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	u8 reg, value;

	if(id < RT5027_ID_LDO2 || id > RT5027_ID_LDO8)
		return -EINVAL;
	
	reg = RT5027_LDO_ENABLE_REG;
	value  = (u8)i2c_smbus_read_byte_data(rt5027->client, reg);
	value |= (1<<(id-5));

	dbg("%s: id:%d\n", __func__, id);
	return i2c_smbus_write_byte_data(rt5027->client, reg, value);
}

static int rt5027_ldo_disable(struct regulator_dev *rdev)
{
	struct rt5027_data* rt5027 = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	u8 reg, value;

	if(id < RT5027_ID_LDO2 || id > RT5027_ID_LDO8)
		return -EINVAL;
	
	reg = RT5027_LDO_ENABLE_REG;
	value  = (u8)i2c_smbus_read_byte_data(rt5027->client, reg);
	value &= ~(1<<(id-5));

	dbg("%s: id:%d\n", __func__, id);
	return i2c_smbus_write_byte_data(rt5027->client, reg, value);
}

static int rt5027_ldo_is_enabled(struct regulator_dev *rdev)
{
	return 0;
}

static struct regulator_ops rt5027_ldo_ops = {
	.set_voltage = rt5027_ldo_set_voltage,
	.get_voltage = rt5027_ldo_get_voltage,
	.enable	  = rt5027_ldo_enable,
	.disable	 = rt5027_ldo_disable,
	.is_enabled  = rt5027_ldo_is_enabled,
};

static struct regulator_desc rt5027_reg[] = {
	{
		.name = "dcdc1",
		.id = RT5027_ID_DCDC1,
		.ops = &rt5027_dcdc_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},	
	{
		.name = "dcdc2",
		.id = RT5027_ID_DCDC2,
		.ops = &rt5027_dcdc_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "dcdc3",
		.id = RT5027_ID_DCDC3,
		.ops = &rt5027_dcdc_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "ldo1",
		.id = RT5027_ID_LDO1,
		.ops = &rt5027_ldo_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = 5,
		.owner = THIS_MODULE,
	},
	{
		.name = "ldo2",
		.id = RT5027_ID_LDO2,
		.ops = &rt5027_ldo_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "ldo3",
		.id = RT5027_ID_LDO3,
		.ops = &rt5027_ldo_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "ldo4",
		.id = RT5027_ID_LDO4,
		.ops = &rt5027_ldo_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "ldo5",
		.id = RT5027_ID_LDO5,
		.ops = &rt5027_ldo_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "ldo6",
		.id = RT5027_ID_LDO6,
		.ops = &rt5027_ldo_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "ldo7",
		.id = RT5027_ID_LDO7,
		.ops = &rt5027_ldo_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "ldo8",
		.id = RT5027_ID_LDO8,
		.ops = &rt5027_ldo_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
};
#define NUM_OUPUT	ARRAY_SIZE(rt5027_reg)

static int rt5027_pmic_probe(struct i2c_client *client, const struct i2c_device_id *i2c_id)
{
	struct regulator_dev **rdev;
	struct rt5027_platform_data *pdata = client->dev.platform_data;
	struct rt5027_data *rt5027;
	int i, id, ret = -ENOMEM;
	unsigned int rt5027_id;
 
	rt5027 = kzalloc(sizeof(struct rt5027_data) +
			sizeof(struct regulator_dev *) * (NUM_OUPUT + 1),
			GFP_KERNEL);
	if (!rt5027)
		goto out;

	rt5027->client = client;
	rt5027_i2c_client = client;

	rt5027->min_uV = RT5027_MIN_UV;
	rt5027->max_uV = RT5027_MAX_UV;

	rdev = rt5027->rdev;

	i2c_set_clientdata(client, rdev);

	rt5027_id = i2c_smbus_read_byte_data(client, 0x00);
	if ((rt5027_id&0xFF) == 0xFF) {
		ret = -ENODEV;
		goto err_nodev;
	} else {
		printk("######## %s: %x ########\n", __func__, rt5027_id);
	}

	INIT_WORK(&rt5027->work, rt5027_work_func);

	for (i = 0; i < pdata->num_subdevs && i <= NUM_OUPUT; i++) {
		id = pdata->subdevs[i].id;
		if (!pdata->subdevs[i].platform_data) {
			rdev[i] = NULL;
			continue;
		}
		if (id >= RT5027_ID_MAX) {
			dev_err(&client->dev, "invalid regulator id %d\n", id);
			goto err;
		}
		rdev[i] = regulator_register(&rt5027_reg[id], &client->dev,
						 pdata->subdevs[i].platform_data,
						 rt5027);
		if (IS_ERR(rdev[i])) {
			ret = PTR_ERR(rdev[i]);
			dev_err(&client->dev, "failed to register %s\n",
				rt5027_reg[id].name);
			goto err;
		}
	}


	/* Current and voltage channels need open */
	i2c_smbus_write_byte_data(client, RT5027_COULOMB_CHANNEL_LOW_REG, 0xff);
	i2c_smbus_write_byte_data(client, RT5027_COULOMB_CHANNEL_HIGH_REG, 0xff);

	/* Start up time set to 2S, long time key press time set to 1S, long press power off time set to 6S */
 	i2c_smbus_write_byte_data(client, RT5027_POK_TIME_SETTING_REG, 0x86); // start up:2s, long pok: 1s, shdn: 6s


	if (pdata->init_port) {
		pdata->init_port(client->irq);
	}

	if (client->irq) {
		/* irq enable */
		#if 0
		i2c_smbus_write_byte_data(client, RT5027_IRQ_EN1_REG, RT5027_IRQ1);
		i2c_smbus_write_byte_data(client, RT5027_IRQ_EN2_REG, RT5027_IRQ2);
		i2c_smbus_write_byte_data(client, RT5027_IRQ_EN3_REG, RT5027_IRQ3);
		i2c_smbus_write_byte_data(client, RT5027_IRQ_EN4_REG, RT5027_IRQ4);
		i2c_smbus_write_byte_data(client, RT5027_IRQ_EN5_REG, RT5027_IRQ5);
		#else // temporary all enable
		i2c_smbus_write_byte_data(client, RT5027_IRQ1_ENABLE_REG, 0xff);
		i2c_smbus_write_byte_data(client, RT5027_IRQ2_ENABLE_REG, 0xff);
		i2c_smbus_write_byte_data(client, RT5027_IRQ3_ENABLE_REG, 0xff);
		i2c_smbus_write_byte_data(client, RT5027_IRQ4_ENABLE_REG, 0xff);
		i2c_smbus_write_byte_data(client, RT5027_IRQ5_ENABLE_REG, 0xff);
		#endif

		if (request_irq(client->irq, rt5027_interrupt, IRQ_TYPE_EDGE_FALLING|IRQF_DISABLED, "rt5027_irq", rt5027)) {
			dev_err(&client->dev, "could not allocate IRQ_NO(%d) !\n", client->irq);
			ret = -EBUSY;
			goto err;
		}
	}

#if defined(CONFIG_REGULATOR_RT5027_PEK)
	// register input device for power key.
	rt5027->input_dev = input_allocate_device();
	if (rt5027->input_dev == NULL) {
		ret = -ENOMEM;
		dev_err(&client->dev, "%s: Failed to allocate input device\n", __func__);
		goto err_input_dev_alloc_failed;
	}

	rt5027->input_dev->evbit[0] = BIT(EV_KEY);
	rt5027->input_dev->name = "rt5027 power-key";
	set_bit(KEY_POWER & KEY_MAX, rt5027->input_dev->keybit);
	ret = input_register_device(rt5027->input_dev);
	if (ret) {
		dev_err(&client->dev, "%s: Unable to register %s input device\n", __func__, rt5027->input_dev->name);
		goto err_input_register_device_failed;
	}

	init_timer(&rt5027->timer);
	rt5027->timer.data = (unsigned long)rt5027;
	rt5027->timer.function = rt5027_timer_func;
#endif

	dev_info(&client->dev, "RT5027 regulator driver loaded\n");
	return 0;

#if defined(CONFIG_REGULATOR_RT5027_PEK)
err_input_register_device_failed:
	input_free_device(rt5027->input_dev);
err_input_dev_alloc_failed:
#endif
err:
	while (--i >= 0)
		regulator_unregister(rdev[i]);
err_nodev:
	kfree(rt5027);
out:
	rt5027_i2c_client = NULL;
	return ret;
}

static int rt5027_pmic_remove(struct i2c_client *client)
{
	struct regulator_dev **rdev = i2c_get_clientdata(client);
	struct rt5027_data* rt5027 = NULL;
	int i;

	for (i=0 ; (rdev[i] != NULL) && (i<RT5027_ID_MAX) ; i++)
		rt5027 = rdev_get_drvdata(rdev[i]);

#if defined(CONFIG_REGULATOR_RT5027_PEK)
	if (rt5027)
		del_timer(&rt5027->timer);
#endif

	for (i = 0; i <= NUM_OUPUT; i++)
		if (rdev[i])
			regulator_unregister(rdev[i]);
	kfree(rdev);
	i2c_set_clientdata(client, NULL);
	rt5027_i2c_client = NULL;

	return 0;
}

static int rt5027_pmic_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int i, ret = 0;
	struct regulator_dev **rdev = i2c_get_clientdata(client);
	struct rt5027_data* rt5027 = NULL;

	for (i=0 ; (rdev[i] != NULL) && (i<RT5027_ID_MAX) ; i++)
		rt5027 = rdev_get_drvdata(rdev[i]);

	if (client->irq)
		disable_irq(client->irq);

	/* clear irq status */
	i2c_smbus_write_byte_data(client, RT5027_IRQ1_STATUS_REG, 0xFF);
	i2c_smbus_write_byte_data(client, RT5027_IRQ2_STATUS_REG, 0xFF);
	i2c_smbus_write_byte_data(client, RT5027_IRQ3_STATUS_REG, 0xFF);
	i2c_smbus_write_byte_data(client, RT5027_IRQ4_STATUS_REG, 0xFF);
	i2c_smbus_write_byte_data(client, RT5027_IRQ5_STATUS_REG, 0xFF);

	if (rt5027) {
		ret = cancel_work_sync(&rt5027->work);
		flush_workqueue(rt5027_wq);
	}

	if (ret && client->irq)
		enable_irq(client->irq);

	rt5027_suspend_status = 1;

	return 0;
}

static int rt5027_pmic_resume(struct i2c_client *client)
{
	int i;
	struct regulator_dev **rdev = i2c_get_clientdata(client);
	struct rt5027_data* rt5027 = NULL;

	for (i=0 ; (rdev[i] != NULL) && (i<RT5027_ID_MAX) ; i++)
		rt5027 = rdev_get_drvdata(rdev[i]);

	#if defined(CONFIG_REGULATOR_RT5027_PEK)
	if (rt5027)
		queue_work(rt5027_wq, &rt5027->work);
	#else
	if (client->irq)
		enable_irq(client->irq);

	/* clear irq status */
	i2c_smbus_write_byte_data(client, RT5027_IRQ1_STATUS_REG, 0xFF);
	i2c_smbus_write_byte_data(client, RT5027_IRQ2_STATUS_REG, 0xFF);
	i2c_smbus_write_byte_data(client, RT5027_IRQ3_STATUS_REG, 0xFF);
	i2c_smbus_write_byte_data(client, RT5027_IRQ4_STATUS_REG, 0xFF);
	i2c_smbus_write_byte_data(client, RT5027_IRQ5_STATUS_REG, 0xFF);
	#endif

	rt5027_suspend_status = 0;
	return 0;
}

static const struct i2c_device_id rt5027_id[] = {
	{ "rt5027", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, rt5027_id);

static struct i2c_driver rt5027_pmic_driver = {
	.probe	  = rt5027_pmic_probe,
	.remove	 = rt5027_pmic_remove,
	.suspend	= rt5027_pmic_suspend,
	.resume	 = rt5027_pmic_resume,
	.driver	 = {
		.name   = "rt5027",
	},
	.id_table   = rt5027_id,
};

static int __init rt5027_pmic_init(void)
{
	rt5027_wq = create_singlethread_workqueue("rt5027_wq");
	if (!rt5027_wq)
		return -ENOMEM;

	return i2c_add_driver(&rt5027_pmic_driver);
}
subsys_initcall(rt5027_pmic_init);

static void __exit rt5027_pmic_exit(void)
{
	i2c_del_driver(&rt5027_pmic_driver);
	if (rt5027_wq)
		destroy_workqueue(rt5027_wq);}
module_exit(rt5027_pmic_exit);

/* Module information */
MODULE_DESCRIPTION("RT5027 regulator driver");
MODULE_AUTHOR("Telechips");
MODULE_LICENSE("GPL");

