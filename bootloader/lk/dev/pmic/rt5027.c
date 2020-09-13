/***************************************************************************************
*	FileName    : rt5027.c
*	Description : Enhanced single Cell Li-Battery and Power System Management IC driver
****************************************************************************************
*
*	TCC Board Support Package
*	Copyright (c) Telechips, Inc.
*	ALL RIGHTS RESERVED
*
****************************************************************************************/

#include <platform/reg_physical.h>
#include <i2c.h>
#include <debug.h>

#if defined(RT5027_PMIC)
#include <dev/rt5027.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

struct rt5027_voltage_t {
	unsigned int uV;
	unsigned char val;
};

/* device address */
#define SLAVE_ADDR_RT5027	0x68


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



/* LDO1 voltage level */
static struct rt5027_voltage_t ldo1_voltages[] = {
	{ 1500, 0x00 }, { 1800, 0x01 }, { 2500, 0x02 }, { 3000, 0x03 },
	{ 3300, 0x04 }, { 3300, 0x05 }, { 3300, 0x06 }, { 3300, 0x07 },
};

#define NUM_LDO1	ARRAY_SIZE(ldo1_voltages)

static int rt5027_initialized = 0;
static int rt5027_i2c_ch = 0;
static int rt5027_acin_det = 0;
static unsigned int rt5027_acin_voltage_read_count = 20;


static unsigned char rt5027_read(unsigned char cmd)
{
	unsigned char recv_data;
	i2c_xfer(SLAVE_ADDR_RT5027, 1, &cmd, 1, &recv_data, rt5027_i2c_ch);
	return recv_data;
}

static void rt5027_write(unsigned char cmd, unsigned char value)
{
	unsigned char send_data[2];
	send_data[0] = cmd;
	send_data[1] = value;
	i2c_xfer(SLAVE_ADDR_RT5027, 2, send_data, 0, 0, rt5027_i2c_ch);
}

int rt5027_battery_voltage(void)
{
	signed long data[2];
	int ret = 4200;

	data[0] = rt5027_read(RT5027_VBATS_HIGH_REG);
	data[1] = rt5027_read(RT5027_VBATS_LOW_REG);
	ret = (data[0] << 8) | data[1];

//	dbg("%s: %dmV\n", __func__, ret);
	return ret;
}

int rt5027_acin_detect(void)
{
	unsigned char value;

	value = rt5027_read(RT5027_IRQ1_STATUS_REG);

	if(value & 0xBF)
		return 1;
	else
		return 0;

}

void rt5027_init(int i2c_ch)
{
	signed long data[2];
	rt5027_i2c_ch = i2c_ch;
	rt5027_initialized = 1;

	dprintf("%s\n", __func__);
}

void rt5027_power_off(void)
{
	int value;

	dprintf(INFO,"%s\n", __func__);	
	
	value = rt5027_read(RT5027_SHDN_CONTROL_REG);
	value = value | 0x80;
	rt5027_write(RT5027_SHDN_CONTROL_REG, value);
}

static int rt5027_output_control(rt5027_src_id id, unsigned int onoff)
{
	unsigned char reg, old_value, value, bit, offset;

	switch (id){
		case RT5027_ID_DCDC1:
		case RT5027_ID_DCDC2:
		case RT5027_ID_DCDC3:
		case RT5027_ID_BOOST:
			reg = RT5027_VSYS_BUCK_ENABLE_REG;
			offset = 0;
			break;
		case RT5027_ID_LDO1:
		case RT5027_ID_LDO2:
		case RT5027_ID_LDO3:
		case RT5027_ID_LDO4:
		case RT5027_ID_LDO5:
		case RT5027_ID_LDO6:
		case RT5027_ID_LDO7:
		case RT5027_ID_LDO8:
			reg = RT5027_LDO_ENABLE_REG;
			offset = 5;
			break;
		default:
			return -1;
	}

	old_value = rt5027_read(reg);
	if (onoff)
		value = (old_value | (1<<(id- offset)));
	else
		value = (old_value & ~(1<<(id-offset)));

	dprintf(INFO, "%s: id:%d, onoff:%d, reg:0x%x, value:0x%x\n", __func__, id, onoff, reg, value);
	rt5027_write(reg, value);

	return 0;
}

static int rt5027_dcdc_set_voltage(rt5027_src_id id, unsigned int uV)
{
	unsigned char reg, value = 0, old_value = 0;
	unsigned char step = 0, vrc = 0;
	unsigned int i, max = 0;

	switch (id) {
		case RT5027_ID_DCDC1:
			reg = RT5027_BUCK1_CONTROL_REG;
			max = 2275; step = 25; vrc = 2;
			break;
		case RT5027_ID_DCDC2:
			reg = RT5027_BUCK2_CONTROL_REG;
			max = 3500; step = 25; vrc = 1;
			break;
		case RT5027_ID_DCDC3:
			reg = RT5027_BUCK3_CONTROL_REG;
			max = 3500; step = 50; vrc = 2;
			break;
		default:
			return -1;
	}

	if (uV > max || uV < 700){
		dprintf(INFO,"Wrong BUCK%d value!\n", id);
		return -1;
	}

	old_value = rt5027_read(reg);
	value = ((uV - 700) / step) << vrc;
	value = value | (old_value & (vrc + vrc/2));

	dprintf(INFO, "%s: id:%d, uV:%d, reg:0x%x, value:0x%x\n", __func__, id, uV, reg, value);
	rt5027_write(reg, value);
	rt5027_output_control(id, 1);
	return 0;
}

static int rt5027_boost_set_voltage(rt5027_src_id id, unsigned int uV)
{
	unsigned char value = 0, old_value = 0;
	const int min = 4500, max = 5500;

	if(uV < min || uV > max){
		dprintf(INFO,"Wrong boost value!\n");
		return -1;
	}

	old_value = rt5027_read(RT5027_BOOST_REG);
	value = (uV - min)/100 | (old_value & 0xF0);

	return 0;
}

static int rt5027_ldo_set_voltage(rt5027_src_id id, unsigned int uV)
{
	unsigned char reg, value = 0;
	unsigned char step = 0, rsv = 0;
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

	if(id == RT5027_ID_LDO1){
		for (i = 0; i < NUM_LDO1; i++) {
			if (ldo1_voltages[i].uV >= uV) {
				value = ldo1_voltages[i].val;
				break;
			}
		}
	}
	else{
		if(uV > max || uV < min){
			dprintf(INFO,"Wrong ldo value!\n");
			return -1;
		}
		value = ((uV - min) / step) << rsv;
	}

	dprintf(INFO, "%s: id:%d, uV:%d, reg:0x%x, value:0x%x\n", __func__, id, uV, reg, value);
	rt5027_write(reg, value);
	rt5027_output_control(id, 1);

	return 0;
}

int rt5027_set_voltage(rt5027_src_id id, unsigned int mV)
{
	if (rt5027_initialized == 0)
		return -1;

	/* power off */
	if (mV == 0)
		rt5027_output_control(id, 0);
	else {
		switch (id){
			case RT5027_ID_DCDC1:
			case RT5027_ID_DCDC2:
			case RT5027_ID_DCDC3:
				rt5027_dcdc_set_voltage(id, mV);
				break;
			case RT5027_ID_BOOST:
				rt5027_boost_set_voltage(id, mV);
				break;
			case RT5027_ID_LDO1:
			case RT5027_ID_LDO2:
			case RT5027_ID_LDO3:
			case RT5027_ID_LDO4:
			case RT5027_ID_LDO5:
			case RT5027_ID_LDO6:
			case RT5027_ID_LDO7:
			case RT5027_ID_LDO8:				
				rt5027_ldo_set_voltage(id, mV);
				break;
			default:
				return -1;
		}
	}

	return 0;
}

int rt5027_set_power(rt5027_src_id id, unsigned int onoff)
{
	if (rt5027_initialized == 0)
		return -1;

	if (onoff)
		rt5027_output_control(id, 1);
	else
		rt5027_output_control(id, 0);

	return 0;
}
#endif


