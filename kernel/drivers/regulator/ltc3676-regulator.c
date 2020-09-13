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
 * @file ltc3676-regulator.c
 * @brief This is the main file for the LTC3676 PMIC regulator driver.
 * i2c should be providing the interface between the PMIC and the MCU.
 *
 * @ingroup PMIC_CORE
 */

#include <linux/slab.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/ltc3676.h>
#include <linux/mfd/ltc3676/core.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <mach/hardware.h>

//enable D-DAUDIO VOUT
#define ENABLE_DAUDIO_VOUT	0

/*
 * STOP -- LOOK at the hardware!  UPDATE these voltage tables!
 *
 * The voltage setting of LTC3676 depend on the R1/R2 values
 * for each switcher.  Look at the hardware design
 * and calculate the correct values for these tables:
 *
 *
 * ltc_sw_[] look-up tables define switching regulator output voltages
 * per DAC code DVBx(0-31) and external resistor divider per formula:
 *
 * Vout = (1+R1/R2)(DVBx*12.5+412.5)(mV) scaled by 1000 to microvolts
 *
 * Following are the feedback resistor values on the NovPek i.MX6 board -- SW1 DDR_VTT
 * DCDC_1_TOP_FB_RESISTOR_R1     100000 (in Ohms)
 * DCDC_1_BOTTOM_FB_RESISTOR_R2  100000 (Do Not Populate, SW1 in Unity Gain for DDR_VTT)
 *
 */

/*
// D-Audio NOT used.
static const int ltc_sw1[] = {
	825000,850000,875000,900000,925000,950000,975000,1000000,1025000,1050000,
	1075000,1100000,1125000,1150000,1175000,1200000,1225000,1250000,1275000,
	1300000,1325000,1350000,1375000,1400000,1425000,1450000,1475000,1500000,
	1525000,1550000,1575000,1600000,
};
*/

/**
 * @author back_yak@cleinsoft
 * @date   2013/11/18
 *
 * D-Audio FB_R1, FB_R2 Voltage is changed
 * DCDC_1_TOP_FB_RESISTOR_R1	360000
 * DCDC_2_BOTTOM_FB_RESISTOR R2	422000
 **/
static const int ltc_sw1[] = {
	764395,787559,810722,833886,857049,880213,903376,926540,949703,972867,
	996030,1019194,1042357,1065521,1088684,1111848,1135011,1158175,1181338,
	1204502,1227665,1250829,1273992,1297156,1320319,1343483,1366649,1389810,
	1412973,1436137,1459300,1482464,
};

/* 
 * Following are the feedback resistor values on the NovPek i.MX6 board -- SW2 VDDSOC_IN
 * DCDC_2_TOP_FB_RESISTOR_R1     100000
 * DCDC_2_BOTTOM_FB_RESISTOR_R2  107000
 */
/*
// D-Audio NOT used.
static const int ltc_sw2[] = {
	798014,822196,846379,870561,894743,918925,943107,967290,991472,1015654,
	1039836,1064019,1088201,1112383,1136565,1160748,1184930,1209112,1233294,
	1257477,1281659,1305841,1330023,1354206,1378388,1402570,1426752,1450935,
	1475117,1499299,1523481,1547664,
};
*/

/**
 * @author back_yak@cleinsoft
 * @date   2013/11/18
 *
 * D-Audio FB_R1, FB_R2 Voltage is changed
 * DCDC_1_TOP_FB_RESISTOR_R1	360000
 * DCDC_2_BOTTOM_FB_RESISTOR R2	100000
 **/
static const int ltc_sw2[] = {
	1897500,1955000,2112500,2070000,2127500,2185000,2242500,2300000,2357500,2415000,
	2472500,2530000,2587500,2645000,2702500,2760000,2817500,2875000,2932500,
	2990000,3047500,3105000,3162500,3220000,3277500,3335000,3392500,3450000,
	3507500,3565000,3622500,3680000,
};

/* 
 * Following are the feedback resistor values on the NovPek i.MX6 board -- SW3 VDDARM_IN
 * DCDC_3_TOP_FB_RESISTOR_R1     100000
 * DCDC_3_BOTTOM_FB_RESISTOR_R2  107000
 */
/*
// D-Audio NOT used
static const int ltc_sw3[] = {
	798014,822196,846379,870561,894743,918925,943107,967290,991472,1015654,
	1039836,1064019,1088201,1112383,1136565,1160748,1184930,1209112,1233294,
	1257477,1281659,1305841,1330023,1354206,1378388,1402570,1426752,1450935,
	1475117,1499299,1523481,1547664,
};
*/

/**
 * @author back_yak@cleinsoft
 * @date   2013/11/18
 *
 * D-Audio FB_R1, FB_R2 Voltage is changed
 * DCDC_1_TOP_FB_RESISTOR_R1	280000
 * DCDC_2_BOTTOM_FB_RESISTOR R2	422000
 **/
static const int ltc_sw3[] = {
	686196,706990,727784,748578,769372,790165,810959,831753,852547,873341,
	894135,914928,935722,956516,977310,998104,1018891,1039691,1060485,
	1081279,1102073,1122867,1143661,1164454,1185248,1206042,1226836,1247630,
	1268424,1289218,1310011,1330805,
};

/* 
 * Following are the feedback resistor values on the NovPek i.MX6 board -- SW4 DDR_+1.5V
 * DCDC_4_TOP_FB_RESISTOR_R1     100000
 * DCDC_4_BOTTOM_FB_RESISTOR_R2   93100
 */
/*
// D-Adudio NOT used
static const int ltc_sw4[] = {
	855572,881498,907425,933351,959278,985204,1011131,1037057,1062983,1088910,
	1114836,1140763,1166689,1192615,1218542,1244468,1270395,1296321,1322248,
	1348174,1374100,1400027,1425953,1451879,1477806,1503733,1529658,1555585,
	1581511,1607438,1633365,1659291,
};
*/

/**
 * @author back_yak@cleinsoft
 * @date   2013/11/18
 *
 * D-Audio FB_R1, FB_R2 Voltage is changed
 * DCDC_1_TOP_FB_RESISTOR_R1	107000
 * DCDC_2_BOTTOM_FB_RESISTOR R2	100000
 **/
static const int ltc_sw4[] = {
	853875,879750,905625,931500,957375,983250,1009125,1035000,1060875,1086750,
	1112625,1138500,1164375,1190250,1216125,1242000,1267875,1293750,1319625,
	1345500,1371375,1397250,1423125,1449000,1474875,1500750,1526625,1552500,
	1578375,1604250,1630125,1656000,
};

/* Supported voltage values for LD04 regulator in mV */
static const u16 LDO4_VSEL_table[] = {
	1200, 2500, 2800, 3000,
};

struct i2c_client *ltc3676_client;

/*
 * LTC3676 Device IO
 */
static DEFINE_MUTEX(io_mutex);

static int ltc_3676_set_bits(struct ltc3676_regulator *ltc, u8 reg, u8 mask)
{
	int err, data;

	mutex_lock(&io_mutex);

	data = ltc->read_dev(ltc, reg);
	if (data < 0) {
		dev_err(&ltc->i2c_client->dev,
			"Read from reg 0x%x failed\n", reg);
		err = data;
		goto out;
	}

	data |= mask;
	err = ltc->write_dev(ltc, reg, data);
	if (err)
		dev_err(&ltc->i2c_client->dev,
			"Write for reg 0x%x failed\n", reg);
out:
	mutex_unlock(&io_mutex);
	return err;
}

static int ltc_3676_clear_and_set_bits(struct ltc3676_regulator *ltc, u8 reg, u8 mask)
{
	int err, data;

	mutex_lock(&io_mutex);

	data = ltc->read_dev(ltc, reg);
	if (data < 0) {
		dev_err(&ltc->i2c_client->dev,
			"Read from reg 0x%x failed\n", reg);
		err = data;
		goto out;
	}

	data &= ~0x1F;
	data |= mask;
	err = ltc->write_dev(ltc, reg, data);
	if (err)
		dev_err(&ltc->i2c_client->dev,
			"Write for reg 0x%x failed\n", reg);
out:
	mutex_unlock(&io_mutex);
	return err;
}

static int ltc_3676_clear_bits(struct ltc3676_regulator *ltc, u8 reg, u8 mask)
{
	int err, data;

	mutex_lock(&io_mutex);

	data = ltc->read_dev(ltc, reg);
	if (data < 0) {
		dev_err(&ltc->i2c_client->dev,
			"Read from reg 0x%x failed\n", reg);
		err = data;
		goto out;
	}

	data &= ~mask;

	err = ltc->write_dev(ltc, reg, data);
	if (err)
		dev_err(&ltc->i2c_client->dev,
			"Write for reg 0x%x failed\n", reg);

out:
	mutex_unlock(&io_mutex);
	return err;

}

static int ltc3676_reg_read(struct ltc3676_regulator *ltc, u8 reg)
{
	int data;

	mutex_lock(&io_mutex);

	data = ltc->read_dev(ltc, reg);
	if (data < 0)
		dev_err(&ltc->i2c_client->dev,
			"Read from reg 0x%x failed\n", reg);

	mutex_unlock(&io_mutex);
	return data;
}

static int ltc_3676_reg_write(struct ltc3676_regulator *ltc, u8 reg, u8 val)
{
	int err;

	mutex_lock(&io_mutex);

	err = ltc->write_dev(ltc, reg, val);
	if (err < 0)
		dev_err(&ltc->i2c_client->dev,
			"Write for reg 0x%x failed\n", reg);

	mutex_unlock(&io_mutex);
	return err;
}
/* 
 * This set_slew_rate funciton set the switch edge rate on each DC-DC
 * The default edge rate is slow for low EMI.  The fast setting improves
 * efficiency. LTC3676 has slew limiting a regulator startup (soft start)
 * and also a controlled slew rate during voltage changes (Dynamic Voltage
 * Scaling) but neither slew rate is adjustable so cannot be set here.
 */

int ltc3676_set_slew_rate(struct regulator_dev *dev, int rate)
{
	struct ltc3676_regulator *ltc = rdev_get_drvdata(dev);
	int dcdc = rdev_get_id(dev);

	// Since there is no slew rate control for LDOs, we return here
	// if the ID is of an LDO
	if (dcdc == LTC3676_LDO_1 || dcdc == LTC3676_LDO_2 ||
		dcdc == LTC3676_LDO_3 || dcdc == LTC3676_LDO_4)
		return 0;
        // For the LTC3676 the slow rate is slow '0' or fast '1'
	if (rate > 0x1)
		return -EINVAL;

	switch (dcdc)
	{ 
		case LTC3676_DCDC_1:
		{
		#ifdef LTC3676_1  //DCDC_1 has a fix function in LTC3676-1, see datasheet.
			return 0;
			break;
		#endif				   
			if (rate)  // Rate is set to FAST
				return ltc_3676_set_bits(ltc, LTC3676_REG_BUCK1, BIT_0_MASK);
			else       // Rate is set to Slow
				return ltc_3676_clear_bits(ltc, LTC3676_REG_BUCK1, BIT_0_MASK);

			break;
		}
		case LTC3676_DCDC_2:
		{
			if (rate)  // Rate is set to FAST
				return ltc_3676_set_bits(ltc, LTC3676_REG_BUCK2, BIT_0_MASK);
			else       // Rate is set to Slow
				return ltc_3676_clear_bits(ltc, LTC3676_REG_BUCK2, BIT_0_MASK);
			break;
		}
		case LTC3676_DCDC_3:
		{
			if (rate)  // Rate is set to FAST
				return ltc_3676_set_bits(ltc, LTC3676_REG_BUCK3, BIT_0_MASK);
			else       // Rate is set to Slow
				return ltc_3676_clear_bits(ltc, LTC3676_REG_BUCK3, BIT_0_MASK);
			break;
		}	

		case LTC3676_DCDC_4:
		{
			if (rate)  // Rate is set to FAST
				return ltc_3676_set_bits(ltc, LTC3676_REG_BUCK4, BIT_0_MASK);
			else       // Rate is set to Slow
				return ltc_3676_clear_bits(ltc, LTC3676_REG_BUCK4, BIT_0_MASK);
			break;
		}	
		default:
		return -EINVAL;			 
	}
	return 0;
}

/* 
 * If hard-wire startup control is used initialization code should 
 * set the Software Control Mode bit in CNTRL Reg 0x09 and set the
 * appropriate regulator enable bits so that this function works
 * correctly.
 *
 * If only hardware enable is used then this function will return
 * an incorrect disabled status because the software enable bits will be low.
 */
static int ltc3676_dcdc_is_enabled(struct regulator_dev *dev)
{
	struct ltc3676_regulator *ltc = rdev_get_drvdata(dev);
	int data, dcdc = rdev_get_id(dev);
	
	if (dcdc < LTC3676_DCDC_1 || dcdc > LTC3676_DCDC_4)
		return -EINVAL;

	switch (dcdc)
	{
		case LTC3676_DCDC_1:
	     	{
		#ifdef LTC3676_1  //DCDC_1 has a fixed function in LTC3676-1, see datasheet.
	   		return 1;
	   		break;
		#endif				    
	    		data = ltc3676_reg_read(ltc, LTC3676_REG_BUCK1);
	    		if (data < 0)
				return data;
		
			return (data & 0x80) ? 1 : 0;  //Checking B[7] status
			break;
		}		
		case LTC3676_DCDC_2:
		{
			data = ltc3676_reg_read(ltc, LTC3676_REG_BUCK2);
		   	if (data < 0)
				return data;
			return (data & 0x80) ? 1 : 0;  //Checking B[7] status
			break;
		}			
		case LTC3676_DCDC_3:
		{
			data = ltc3676_reg_read(ltc, LTC3676_REG_BUCK3);
		    	if (data < 0)
				return data;
			return (data & 0x80) ? 1 : 0;  //Checking B[7] status
			break;
		}
		case LTC3676_DCDC_4:
		{
			data = ltc3676_reg_read(ltc, LTC3676_REG_BUCK4);
		    	if (data < 0)
				return data;
			return (data & 0x80) ? 1 : 0;  //Checking B[7] status
			break;
		}	
		default:
		     return -EINVAL;			 
	}
	return 0;
}

/* 
 * If hard-wire startup control is used initialization code should 
 * set the Software Control Mode bit in CNTRL Reg 0x09 and set the
 * appropriate regulator enable bits so that this function works
 * correctly.
 *
 * If only hardware enable is used then this function will return
 * an incorrect disabled status because the software enable bits will be low.
 */
static int ltc3676_ldo_is_enabled(struct regulator_dev *dev)
{
	struct ltc3676_regulator *ltc = rdev_get_drvdata(dev);
	int data, ldo = rdev_get_id(dev);

	if (ldo < LTC3676_LDO_1 || ldo > LTC3676_LDO_4)
		return -EINVAL;

	if (ldo == LTC3676_LDO_1)
		return 1;
		
    	switch (ldo)
	{
		case LTC3676_LDO_2:
	     	{
			data = ltc3676_reg_read(ltc, LTC3676_REG_LDOA);
	    		if (data < 0)
				return data;
		
			return (data & 0x04) ? 1 : 0;  //Checking B[2] status
			break;
		}	
		case LTC3676_LDO_3:
	     	{
		    	data = ltc3676_reg_read(ltc, LTC3676_REG_LDOA);
	    		if (data < 0)
				return data;
		
			return (data & 0x20) ? 1 : 0;  //Checking B[5] status
			break;
		}
		case LTC3676_LDO_4:
		{
			data = ltc3676_reg_read(ltc, LTC3676_REG_LDOB);
		    	if (data < 0)
				return data;
			return (data & 0x04) ? 1 : 0;  //Checking B[2] status
			break;
		}	
	 	default:
	     		return -EINVAL;	  
        }			 
	return 0;
}

/* 
 * If hard-wire startup control is used, initialization code should 
 * set the Software Control Mode bit in CNTRL Reg 0x09 and set the
 * appropriate regulator enable bits so that this function works
 * correctly.
 *
 * If only hardware enable is used then this function can still enable
 * any regulator that is not already enabled by the hardware enable pin.
 */
static int ltc3676_dcdc_enable(struct regulator_dev *dev)
{
	struct ltc3676_regulator *ltc = rdev_get_drvdata(dev);
	int dcdc = rdev_get_id(dev);

	if (dcdc < LTC3676_DCDC_1 || dcdc > LTC3676_DCDC_4)
		return -EINVAL;

    	switch (dcdc)
	{
		case LTC3676_DCDC_1:
		{
		#ifdef LTC3676_1  //DCDC_1 has a fix function in LTC3676-1, see datasheet.
			break;
		#endif					
			return ltc_3676_set_bits(ltc, LTC3676_REG_BUCK1, BIT_7_MASK);
			break;
		}		
		case LTC3676_DCDC_2:
		{
			return ltc_3676_set_bits(ltc, LTC3676_REG_BUCK2, BIT_7_MASK);
			break;
		}
		case LTC3676_DCDC_3:
		{
			return ltc_3676_set_bits(ltc, LTC3676_REG_BUCK3, BIT_7_MASK);
			break;
		}
		case LTC3676_DCDC_4:
		{
			return ltc_3676_set_bits(ltc, LTC3676_REG_BUCK4, BIT_7_MASK);
			break;
		}	
		default:
		return -EINVAL;			 
	}
    	return 1;
}

/* 
 * If hard-wire startup control is used initialization code should 
 * set the Software Control Mode bit in CNTRL Reg 0x09 and set the
 * appropriate regulator enable bits so that this function works
 * correctly.
 *
 * If only hardware enable is used then this function cannot disable
 * any regulator that is enabled by its hardware enable pin.
 */

static int ltc3676_dcdc_disable(struct regulator_dev *dev)
{
	struct ltc3676_regulator *ltc = rdev_get_drvdata(dev);
	int dcdc = rdev_get_id(dev);

	if (dcdc < LTC3676_DCDC_1 || dcdc > LTC3676_DCDC_4)
		return -EINVAL;

    	switch (dcdc)
	{
		case LTC3676_DCDC_1:
		{
		#ifdef LTC3676_1  //DCDC_1 has a fix function in LTC3676-1, see datasheet.
			return 0;
			break;
		#endif					
			return ltc_3676_clear_bits(ltc, LTC3676_REG_BUCK1, BIT_7_MASK);
			break;
		}		
		case LTC3676_DCDC_2:
		{
			return ltc_3676_clear_bits(ltc, LTC3676_REG_BUCK2, BIT_7_MASK);
			break;
		}
		case LTC3676_DCDC_3:
		{
			return ltc_3676_clear_bits(ltc, LTC3676_REG_BUCK3, BIT_7_MASK);
			break;
		}
		case LTC3676_DCDC_4:
		{
			return ltc_3676_clear_bits(ltc, LTC3676_REG_BUCK4, BIT_7_MASK);
			break;
		}	
		default:
		return -EINVAL;			 
	 
	}
	return 0;	
}

/* 
 * If hard-wire startup control is used, initialization code should 
 * set the Software Control Mode bit in CNTRL Reg 0x09 and set the
 * appropriate regulator enable bits so that this function works
 * correctly.
 *
 * If only hardware enable is used then this function can still enable
 * any regulator that is not already enabled by the hardware enable pin.
 */
static int ltc3676_ldo_enable(struct regulator_dev *dev)
{
	struct ltc3676_regulator *ltc = rdev_get_drvdata(dev);
	int ldo = rdev_get_id(dev);

	if (ldo < LTC3676_LDO_1 || ldo > LTC3676_LDO_4)
		return -EINVAL;

	if (ldo == LTC3676_LDO_1) {
		printk(KERN_ERR "LDO1 is always enabled\n");
		return 0;
	}
	switch (ldo)
	{
		case LTC3676_LDO_2:
		{
			return ltc_3676_set_bits(ltc, LTC3676_REG_LDOA, BIT_2_MASK);
			break;
             	}		
	
	      	case LTC3676_LDO_3:
		{
			return ltc_3676_set_bits(ltc, LTC3676_REG_LDOA, BIT_5_MASK);
			break;
             	}

	      	case LTC3676_LDO_4:
		{
			return ltc_3676_set_bits(ltc, LTC3676_REG_LDOB, BIT_2_MASK);
			break;
             	}	
     
          	default:
		     return -EINVAL;	 
	}	
   	return 0;
}

/* 
 * If hard-wire startup control is used initialization code should 
 * set the Software Control Mode bit in CNTRL Reg 0x09 and set the
 * appropriate regulator enable bits so that this function works
 * correctly.
 *
 * If only hardware enable is used then this function cannot disable
 * any regulator that is enabled by its hardware enable pin.
 */
static int ltc3676_ldo_disable(struct regulator_dev *dev)
{
	struct ltc3676_regulator *ltc = rdev_get_drvdata(dev);
	int ldo = rdev_get_id(dev);

	if (ldo < LTC3676_LDO_1 || ldo > LTC3676_LDO_4)
		return -EINVAL;

	if (ldo == LTC3676_LDO_1) {
		printk(KERN_ERR "LDO1 is always enabled\n");
		return 0;
	}

	switch (ldo)
	{
		case LTC3676_LDO_2:
		{
			return ltc_3676_clear_bits(ltc, LTC3676_REG_LDOA, BIT_2_MASK);
			break;
		}		
		case LTC3676_LDO_3:
		{
			return ltc_3676_clear_bits(ltc, LTC3676_REG_LDOA, BIT_5_MASK);
			break;
		}
		case LTC3676_LDO_4:
		{
			return ltc_3676_clear_bits(ltc, LTC3676_REG_LDOB, BIT_2_MASK);
			break;
		}	
		default:
			return -EINVAL;			 
	}	
	return 0;	
}
/* 
 * This dcdc_set_mode is a basic implementation that puts the regulator in the best mode
 * for the 4 Linux pre-defined operating states.  This function could (should?) be more
 * sophisticated.  The Linux operating mode could also define specific DVFS operating points.
 * FAST mode should raise the core supply voltage to accomodate a fast processor frequency.
 * NORMAL mode should set the regualtor output voltage to a nominal value.
 * IDLE and STANDBY modes should ideally lower the output voltage to reduce processor leakage
 * current and power disipation to extend battery life in mobile applications.
 * Some of the above consideration might be handled in a specific DVFS driver.
 */
static int ltc3676_dcdc_set_mode(struct regulator_dev *dev,
				       unsigned int mode)
{
	struct ltc3676_regulator *ltc = rdev_get_drvdata(dev);
	int dcdc = rdev_get_id(dev);
	u16 reg_offset;
	int return_value;

	if (dcdc < LTC3676_DCDC_1 || dcdc > LTC3676_DCDC_4)
		return -EINVAL;
		
	#ifdef LTC3676_1  //DCDC_1 has a fix function in LTC3676-1, see datasheet.
	if (dcdc == LTC3676_DCDC_1)
		return -1;
	#endif			

	reg_offset = dcdc - LTC3676_DCDC_1;

	switch (mode) 
	{
		case REGULATOR_MODE_FAST: 
			/* Force Continuous mode, value of '10' for bits 6/5, lowest ripple */
			return_value = ltc_3676_clear_bits(ltc, (LTC3676_REG_BUCK1+reg_offset), BIT_5_MASK);
			if (return_value < 0)
				return return_value;
			return ltc_3676_set_bits(ltc, (LTC3676_REG_BUCK1+reg_offset), BIT_6_MASK);
			break;
		case REGULATOR_MODE_NORMAL:
			/* Pulse Skip mode, value of 00 for bits 6/5, normal fixed frequency operation */
			return_value = ltc_3676_clear_bits(ltc, (LTC3676_REG_BUCK1+reg_offset), BIT_6_MASK);
			if (return_value < 0)
				return return_value;
			return ltc_3676_clear_bits(ltc, (LTC3676_REG_BUCK1+reg_offset), BIT_5_MASK);
			break;
		case REGULATOR_MODE_IDLE:
		case REGULATOR_MODE_STANDBY:
			/* Burst mode, value of 01 for bits 6/5, highest light load efficiency */
			return_value = ltc_3676_clear_bits(ltc, (LTC3676_REG_BUCK1+reg_offset), BIT_6_MASK);
			if (return_value < 0)
				return return_value;
			return ltc_3676_set_bits(ltc, (LTC3676_REG_BUCK1+reg_offset), BIT_5_MASK);
			break;
		default:
			return -EINVAL;	 
	}
	return 0;		
}

static unsigned int ltc3676_dcdc_get_mode(struct regulator_dev *dev)
{
	struct ltc3676_regulator *ltc = rdev_get_drvdata(dev);
	int dcdc = rdev_get_id(dev);
	u16 reg_offset;
	int data;
	
	if (dcdc < LTC3676_DCDC_1 || dcdc > LTC3676_DCDC_4)
		return -EINVAL;
	
	#ifdef LTC3676_1  //DCDC_1 has a fix function in LTC3676-1, see datasheet.
	if (dcdc == LTC3676_DCDC_1)
		return -1;
	#endif	

	reg_offset = dcdc - LTC3676_DCDC_1;
	data = ltc3676_reg_read(ltc, LTC3676_REG_BUCK1+reg_offset);

	if (data < 0)
		return data;

	data = (data & 0x60) >> 5;  // Retrieving bits 6/5 

	switch (data) 
	{
		case 0:
		     	return REGULATOR_MODE_NORMAL;
		case 1:
		     	return REGULATOR_MODE_IDLE;
		case 2:
		     	return REGULATOR_MODE_FAST;
		default:
			return -EINVAL;	 
	}

return 0;
}
 
int lookUpTable(int table, int min_uV, int max_uV)
{
	int i = 0;
	for (i=0; i<32; i++)
	{
		switch(table)
		{
			case 1:
				if( (ltc_sw1[i] >= min_uV) && (ltc_sw1[i] <= max_uV) )
					return i;
			case 2:
				if( (ltc_sw2[i] >= min_uV) && (ltc_sw2[i] <= max_uV) )
					return i;
			case 3:
				if( (ltc_sw3[i] >= min_uV) && (ltc_sw3[i] <= max_uV) )
					return i;
			case 4:
				if( (ltc_sw4[i] >= min_uV) && (ltc_sw4[i] <= max_uV) )
					return i;
		}
	}
	return -1;
}

static int ltc3676_dcdc_get_voltage(struct regulator_dev *dev)
{
	struct ltc3676_regulator *ltc = rdev_get_drvdata(dev);
	int  volt_reg, data, dcdc = rdev_get_id(dev);
	int  table_uV = 0;

	if (dcdc < LTC3676_DCDC_1 || dcdc > LTC3676_DCDC_4)
		return -EINVAL;

	switch (dcdc) 
	{
		case LTC3676_DCDC_1:
		#ifdef LTC3676_1  //DCDC_1 has a fix function in LTC3676-1, see datasheet.
		       return -1;
		#else			
		
	        	data = ltc3676_reg_read(ltc, LTC3676_REG_DVB1A);
	           	if (data < 0)
		          return data;
		   
		       	volt_reg = data & 0x1F;    //Storing the reference setting for VA reference
		       	data = data & BIT_5_MASK;  //BIT 5 indicates to use VA or VB reference for DCDC_1 
        
               		if (data) // VB is being use, volt_reg will receive the VB value.
		        {
                     		volt_reg = ltc3676_reg_read(ltc, LTC3676_REG_DVB1B);
           	            	if (volt_reg < 0)
		                   return volt_reg;
                     		volt_reg &= 0x1F; //Storing the reference setting for VB reference					
                  	}
			
			table_uV = ltc_sw1[volt_reg];
	        	break;		 
	    	#endif
	    	case LTC3676_DCDC_2:
	       		data = ltc3676_reg_read(ltc, LTC3676_REG_DVB2A);
			if (data < 0)
		        	return data;
		   
		       	volt_reg = data & 0x1F;   	//Storing the reference setting for VA reference
			data = data & BIT_5_MASK;  	//BIT 5 indicates if we use VA or VB reference for DCDC_2 
        
               		if (data) // VB is being use, volt_reg will receive the VB value.
		        {
                     		volt_reg = ltc3676_reg_read(ltc, LTC3676_REG_DVB2B);
           	         	if (volt_reg < 0)
		                   return volt_reg;
                     		volt_reg &= 0x1F;    //Storing the reference setting for VB reference		
                  	}
				  
			table_uV = ltc_sw2[volt_reg];	  
	        	break;	
	    	case LTC3676_DCDC_3:
	        	data = ltc3676_reg_read(ltc, LTC3676_REG_DVB3A);
	           	if (data < 0)
		          	return data;
		   
		       	volt_reg = data & 0x1F;     //Storing the reference setting for VA reference
		      	 data = data & BIT_5_MASK;  //BIT 5 indicates if we use VA or VB reference for DCDC_3 
        
               		if (data) //VB is being use, volt_reg will receive the VB value.
		        {
                     		volt_reg = ltc3676_reg_read(ltc, LTC3676_REG_DVB3B);
           	            	if (volt_reg < 0)
		                   return volt_reg;
                    		 volt_reg &= 0x1F;    //Storing the reference setting for VB reference	
                  	}
				  
			table_uV = ltc_sw3[volt_reg];	  
	        	break;
	   	case LTC3676_DCDC_4:
	    		data = ltc3676_reg_read(ltc, LTC3676_REG_DVB4A);
	           	if (data < 0)
		          return data;
		   
		       	volt_reg = data & 0x1F;     //Storing the reference setting for VA reference
		       	data = data & BIT_5_MASK;   //BIT 5 indicates if we use VA or VB reference for DCDC_4 
        
               		if (data) //VB is being use, volt_reg will receive the VB value.
		        {
                     		volt_reg = ltc3676_reg_read(ltc, LTC3676_REG_DVB4B);
           	            	if (volt_reg < 0)
		                   return volt_reg;
                     		volt_reg &= 0x1F;    //Storing the reference setting for VB reference		
                  	}
			table_uV = ltc_sw4[volt_reg];	  
		        break;			
	    	default:
		   return -EINVAL;
	}
	return table_uV;
}

static int ltc3676_dcdc_set_voltage(struct regulator_dev *dev, int min_uV, int max_uV)
{
	struct ltc3676_regulator *ltc = rdev_get_drvdata(dev);
	int dcdc = rdev_get_id(dev);
	int dac_vol, data;

	if (dcdc < LTC3676_DCDC_1 || dcdc > LTC3676_DCDC_4)
		return -EINVAL;

	switch (dcdc) 
	{
      		case LTC3676_DCDC_1: 
            	#ifdef LTC3676_1  //DCDC_1 has a fix function in LTC3676-1, see datasheet.
               		return -1;
			break;
            	#endif	
			dac_vol = lookUpTable(1, min_uV, max_uV); //ltc_sw1[data];	
			if ((dac_vol > 0x1F) || (dac_vol < 0))
		        	return -1;
	  
             		data = ltc3676_reg_read(ltc, LTC3676_REG_DVB1A);
		     	if (data < 0)
		        	return data;

		    	data &= BIT_5_MASK; //Keep the Buck1 Reference Select bit 
		     	data |= dac_vol;    //Now adding the Feedback Reference value
		     	return ltc_3676_reg_write(ltc, LTC3676_REG_DVB1A, data);
		     	break;
	      case LTC3676_DCDC_2:
			dac_vol = lookUpTable(2, min_uV, max_uV); //ltc_sw2[data];
			if ((dac_vol > 0x1F) || (dac_vol < 0))
		        	return -1;

             	    	data = ltc3676_reg_read(ltc, LTC3676_REG_DVB2A);
		     	if (data < 0)
		        	return data;

		     	data &= BIT_5_MASK; //Keep the Buck2 Reference Select bit 	 
		     	data |= dac_vol;    //Now adding the Feedback Reference value
		     	return ltc_3676_reg_write(ltc, LTC3676_REG_DVB2A, data);	
			break;
	      case LTC3676_DCDC_3:
			dac_vol = lookUpTable(3, min_uV, max_uV); //ltc_sw3[data];	
			if ((dac_vol > 0x1F) || (dac_vol < 0))
		        	return -1;

             		data = ltc3676_reg_read(ltc, LTC3676_REG_DVB3A);
		     	if (data < 0)
		        	return data;

		     	data &= BIT_5_MASK; //Keep the Buck3 Reference Select bit 	 
		     	data |= dac_vol;    //Now adding the Feedback Reference value
		     	return ltc_3676_reg_write(ltc, LTC3676_REG_DVB3A, data);	
			break;
	      case LTC3676_DCDC_4:
			dac_vol = lookUpTable(4, min_uV, max_uV); //ltc_sw4[data];	
			if ((dac_vol > 0x1F) || (dac_vol < 0))
		        	return -1;

             		data = ltc3676_reg_read(ltc, LTC3676_REG_DVB4A);
		     	if (data < 0)
		        	return data;

		     	data &= BIT_5_MASK; //Keep the Buck4 Reference Select bit 	 
		     	data |= dac_vol;    //Now adding the Feedback Reference value
		     	return ltc_3676_reg_write(ltc, LTC3676_REG_DVB4A, data);	
			break; 			 
		default:
			return -EINVAL;
	}		
	return 0;
}

/*
 * _set_suspend_voltage loads the Dynamic Voltage Buck[1-4] Register B (DVBxB)
 * with the requested voltage set point in uV.  However, it does not activate
 * the suspend votlage.  The API must call _set_suspend_enable to activate
 * the suspend voltage level.  This allows the suspend voltage to be configured
 * once and then activated (enabled) easily when the suspend state is entered.
 */
static int ltc3676_set_suspend_voltage(struct regulator_dev *dev, int uV)
{
	struct ltc3676_regulator *ltc = rdev_get_drvdata(dev);
	int dcdc = rdev_get_id(dev);
	int dac_vol, data;

	if (dcdc < LTC3676_DCDC_1 || dcdc > LTC3676_DCDC_4)
		return -EINVAL;

	switch (dcdc) 
	{
      		case LTC3676_DCDC_1: 
            	#ifdef LTC3676_1  //DCDC_1 has a fix function in LTC3676-1, see datasheet.
               		return -1;
			break;
            	#endif	
			dac_vol = lookUpTable(1, uV, uV); //ltc_sw1[data];	
			if ((dac_vol > 0x1F) || (dac_vol < 0))
		        	return -1;
	  
             		data = ltc3676_reg_read(ltc, LTC3676_REG_DVB1B);
		     	if (data < 0)
		        	return data;

		    	data &= BIT_5_MASK; //Keep the Buck1 PGood Mask bit 
		     	data |= dac_vol;    //Now adding the Feedback Reference value
		     	return ltc_3676_reg_write(ltc, LTC3676_REG_DVB1B, data);
		     	break;
	      case LTC3676_DCDC_2:
			dac_vol = lookUpTable(2, uV, uV); //ltc_sw2[data];
			if ((dac_vol > 0x1F) || (dac_vol < 0))
		        	return -1;

             	    	data = ltc3676_reg_read(ltc, LTC3676_REG_DVB2B);
		     	if (data < 0)
		        	return data;

		     	data &= BIT_5_MASK; //Keep the Buck2 PGood Mask bit 	 
		     	data |= dac_vol;    //Now adding the Feedback Reference value
		     	return ltc_3676_reg_write(ltc, LTC3676_REG_DVB2B, data);	
			break;
	      case LTC3676_DCDC_3:
			dac_vol = lookUpTable(3, uV, uV); //ltc_sw3[data];	
			if ((dac_vol > 0x1F) || (dac_vol < 0))
		        	return -1;

             		data = ltc3676_reg_read(ltc, LTC3676_REG_DVB3B);
		     	if (data < 0)
		        	return data;

		     	data &= BIT_5_MASK; //Keep the Buck3 PGood Mask bit 	 
		     	data |= dac_vol;    //Now adding the Feedback Reference value
		     	return ltc_3676_reg_write(ltc, LTC3676_REG_DVB3B, data);	
			break;
	      case LTC3676_DCDC_4:
			dac_vol = lookUpTable(4, uV, uV); //ltc_sw4[data];	
			if ((dac_vol > 0x1F) || (dac_vol < 0))
		        	return -1;

             		data = ltc3676_reg_read(ltc, LTC3676_REG_DVB4B);
		     	if (data < 0)
		        	return data;

		     	data &= BIT_5_MASK; //Keep the Buck4 PGood Mask bit 	 
		     	data |= dac_vol;    //Now adding the Feedback Reference value
		     	return ltc_3676_reg_write(ltc, LTC3676_REG_DVB4B, data);	
			break; 			 
		default:
			return -EINVAL;
	}		
	return 0;
}

/*
 * _set_suspend_enable sets the DC-DC Reference Select Bit in Reg DVBxA to activate
 * the suspend voltage reference stored in DVBxB to move the output to that pre-loaded
 * suspend voltage (usually a lower voltage for the processor suspend state).
 */
static int ltc3676_set_suspend_enable(struct regulator_dev *dev)
{
	struct ltc3676_regulator *ltc = rdev_get_drvdata(dev);
	int dcdc = rdev_get_id(dev);

	if (dcdc < LTC3676_DCDC_1 || dcdc > LTC3676_DCDC_4)
		return -EINVAL;

	switch (dcdc) 
	{
      		case LTC3676_DCDC_1: 
            	#ifdef LTC3676_1  //DCDC_1 has a fix function in LTC3676-1, see datasheet.
               		return -1;
			break;
            	#endif	 
             		//Set the Buck1 Reference Select bit to activate DVB1B suspend voltage
		     	return ltc_3676_set_bits(ltc, LTC3676_REG_DVB1A, BIT_5_MASK);
		     	break;
	      case LTC3676_DCDC_2:
			//Set the Buck2 Reference Select bit to activate DVB2B suspend voltage
		     	return ltc_3676_set_bits(ltc, LTC3676_REG_DVB2A, BIT_5_MASK);	
			break;
	      case LTC3676_DCDC_3:
             		//Set the Buck3 Reference Select bit to activate DVB3B suspend voltage	 
		     	return ltc_3676_set_bits(ltc, LTC3676_REG_DVB3A, BIT_5_MASK);	
			break;
	      case LTC3676_DCDC_4:
			//Set the Buck4 Reference Select bit to activate DVB4B suspend voltage	 
		     	return ltc_3676_set_bits(ltc, LTC3676_REG_DVB4A, BIT_5_MASK);
			break; 			 
		default:
			return -EINVAL;
	}		
	return 0;
}

/*
 * _set_suspend_disable clears the DC-DC Reference Select Bit in Reg DVBxA to disable
 * the suspend voltage reference stored in DVBxB and select the normal voltage refence
 * stored in DVBxA.
 */
static int ltc3676_set_suspend_disable(struct regulator_dev *dev)
{
	struct ltc3676_regulator *ltc = rdev_get_drvdata(dev);
	int dcdc = rdev_get_id(dev);
	int reg_offset;

	if (dcdc < LTC3676_DCDC_1 || dcdc > LTC3676_DCDC_4)
		return -EINVAL;
	  
	switch (dcdc) 
	{
      		case LTC3676_DCDC_1: 
            	#ifdef LTC3676_1  //DCDC_1 has a fix function in LTC3676-1, see datasheet.
               		return -1;
			break;
            	#endif	 
             		//Clear the Buck1 Reference Select bit to activate DVB1A normal voltage
		     	return ltc_3676_clear_bits(ltc, LTC3676_REG_DVB1A, BIT_5_MASK);
		     	break;
	      case LTC3676_DCDC_2:
			//Clear the Buck2 Reference Select bit to activate DVB2A normal voltage
		     	return ltc_3676_clear_bits(ltc, LTC3676_REG_DVB2A, BIT_5_MASK);	
			break;
	      case LTC3676_DCDC_3:
             		//Clear the Buck3 Reference Select bit to activate DVB3A normal voltage	 
		     	return ltc_3676_clear_bits(ltc, LTC3676_REG_DVB3A, BIT_5_MASK);	
			break;
	      case LTC3676_DCDC_4:
			//Clear the Buck4 Reference Select bit to activate DVB4A normal voltage	 
		     	return ltc_3676_clear_bits(ltc, LTC3676_REG_DVB4A, BIT_5_MASK);
			break; 			 
		default:
			return -EINVAL;
	}		
	return 0;
}

/*
 * suspend mode doesn't exist.  Suspend voltage must be set and then enabled or disabled.
 */
static int ltc3676_set_suspend_mode(struct regulator_dev *dev,unsigned int mode)
{
	return 0;
}

static int ltc3676_ldo_get_voltage(struct regulator_dev *dev)
{
	struct ltc3676_regulator *ltc = rdev_get_drvdata(dev);
	int data, ldo = rdev_get_id(dev);
	//int uV;
		
	// Only LDO4 can be set by software in LTC3676-1
	
	#ifdef LTC3676_1
	if (ldo != LTC3676_LDO_4)  //Only address request for LDO4
		return -EINVAL;

	data = ltc3676_reg_read(ltc, LTC3676_REG_LDOB);
	if (data < 0)
		return data;
			 
	data >>= 3;     // Shift bits 3/4 
       	data &= 0x03;   // Keep the LDO4 voltage bits value	
		
	switch (data) 
	{
		case 0:
			return 1800000;
		case 1:
		    	return 2500000;
		case 2:
		    	return 2800000;
		case 3:
		    	return 3000000;
		default:
		    	return -EINVAL;		
	}				
	#else
	    return -EINVAL;   
	#endif	
}

static int ltc3676_ldo_set_voltage(struct regulator_dev *dev, int min_uV, int max_uV)
{
	struct ltc3676_regulator *ltc = rdev_get_drvdata(dev);
	int data, vsel, ldo = rdev_get_id(dev);
	
	// Only LDO4 can be set by software in LTC3676-1
	
	#ifdef LTC3676_1
	if (ldo != LTC3676_LDO_4)  //Only address request for LDO4
		return -EINVAL;

	for (vsel = 0; vsel < 4; vsel++) 
	{
	 	int mV = LDO4_VSEL_table[vsel];
	     	int uV = mV * 1000;

	     	/* Break at the first in-range value */
	     	if (min_uV <= uV && uV <= max_uV)
			break;
	}
		  
	if (vsel == 4)
		return -EINVAL;	

     	data = ltc3676_reg_read(ltc, LTC3676_REG_LDOB);
	if (data < 0)
		return data;
			 
	data &= 0xE7; // Clear the LDO4 voltage select bits
	vsel <<= 3;   // Shift the LDO4 voltage select bit to the correct location within the register
        data |= vsel; 	   
			 		  
	return ltc_3676_reg_write(ltc, LTC3676_REG_LDOB, data);	 		
	
	#else
	    return -EINVAL;   
	#endif		
}

static int ltc3676_ldo4_list_voltage(struct regulator_dev *dev,
					unsigned selector)
{
	int ldo = rdev_get_id(dev);

	#ifdef LTC3676_1	
	if (ldo != LTC3676_LDO_4)
		return -EINVAL;

	if (selector >= 4)
		return -EINVAL;
	else
		return LDO4_VSEL_table[selector] * 1000;
	#endif
	
	return -EINVAL;
}

/* Operations permitted on SWx */
static struct regulator_ops ltc3676_sw_ops = 
{
	.is_enabled = ltc3676_dcdc_is_enabled,
	.enable = ltc3676_dcdc_enable,
	.disable = ltc3676_dcdc_disable,
	.get_mode = ltc3676_dcdc_get_mode,
	.set_mode = ltc3676_dcdc_set_mode,
	.get_voltage = ltc3676_dcdc_get_voltage,
	.set_voltage = ltc3676_dcdc_set_voltage,
	.set_suspend_voltage = ltc3676_set_suspend_voltage,
	.set_suspend_enable = ltc3676_set_suspend_enable,
	.set_suspend_disable = ltc3676_set_suspend_disable,
	.set_suspend_mode = ltc3676_set_suspend_mode,
};

// Operations permitted on SW4 
// static struct regulator_ops ltc3676_sw4_ops = {
//	.is_enabled = ltc3676_dcdc_is_enabled,
//	.enable = ltc3676_dcdc_enable,
//	.disable = ltc3676_dcdc_disable,
//	.get_mode = ltc3676_dcdc_get_mode,
//	.set_mode = ltc3676_dcdc_set_mode,
// };

// Operations permitted on LDO1_STBY, LDO3  // LDO1 cannot be controlled
// static struct regulator_ops ltc3676_ldo13_ops = {
//	.is_enabled = ltc3676_ldo_is_enabled,
//	.enable = ltc3676_ldo_enable,
//	.disable = ltc3676_ldo_disable,
// };

/* Operations permitted on LDO2 */
static struct regulator_ops ltc3676_ldo2_ops = 
{
	.is_enabled = ltc3676_ldo_is_enabled,
	.enable = ltc3676_ldo_enable,
	.disable = ltc3676_ldo_disable,
};

/* Operations permitted on LDO3 */
static struct regulator_ops ltc3676_ldo3_ops = 
{
	.is_enabled = ltc3676_ldo_is_enabled,
	.enable = ltc3676_ldo_enable,
	.disable = ltc3676_ldo_disable,
};

/* Operations permitted on LDO4 */
static struct regulator_ops ltc3676_ldo4_ops = 
{
	.is_enabled = ltc3676_ldo_is_enabled,
	.enable = ltc3676_ldo_enable,
	.disable = ltc3676_ldo_disable,
	#ifdef LTC3676_1
	   .get_voltage = ltc3676_ldo_get_voltage,      
	   .set_voltage = ltc3676_ldo_set_voltage,      
	   .list_voltage = ltc3676_ldo4_list_voltage,
	#endif   
};

/**
 *	@author back_yak@cleinsoft
 *	@date 2013/11/18
 *	LTC3676 Register info
 **/
static struct regulator_desc ltc3676_reg_desc[] = {
	{
		//.name = "SW1",		<= renaming to CPU_BLOCK
		.name = "CPU_BLOCK",
		.id = LTC3676_BUCK_CH1,
		.ops = &ltc3676_sw_ops,
		.irq = 0,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE
	},
	{
		//.name = "vddarm_ext",	<= renaming to GPIO_BLOCK
		.name = "GPIO_BLOCK", 
		.id = LTC3676_BUCK_CH2,
		.ops = &ltc3676_sw_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = 0x1F + 1,
		.owner = THIS_MODULE,
	},
	{
		//.name = "vddsoc_ext",	<= renaming to CORE_BLOCK
		.name = "CORE_BOCK",
		.id = LTC3676_BUCK_CH3,
		.ops = &ltc3676_sw_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = 0x1F + 1,
		.owner = THIS_MODULE,
	},
	{
		//.name = "SW4",		<= renaming to DDR_BLOCK
		.name = "DDR_BLOCK",
		.id = LTC3676_BUCK_CH4,
		.ops = &ltc3676_sw_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = 0x1F + 1,
		.owner = THIS_MODULE,
	},
};

int ltc3676_regulator_probe(struct platform_device *pdev)
{
	struct regulator_desc *rdesc;
	struct regulator_dev *rdev;
	struct ltc3676_regulator *sreg;
	struct regulator_init_data *initdata;

	//sreg = platform_get_drvdata(pdev);
	sreg = dev_get_drvdata(&pdev->dev);
	initdata = pdev->dev.platform_data;

	init_waitqueue_head(&sreg->wait_q);
	spin_lock_init(&sreg->lock);

	if (pdev->id > LTC3676_SUPPLY_NUM) {		//declare define supply_num 
		rdesc = kzalloc(sizeof(struct regulator_desc), GFP_KERNEL);
		memcpy(rdesc, &ltc3676_reg_desc[LTC3676_SUPPLY_NUM],
			sizeof(struct regulator_desc));
		rdesc->name = kstrdup(sreg->rdata->name, GFP_KERNEL);
	} else
		rdesc = &ltc3676_reg_desc[pdev->id];

	pr_debug("probing regulator %s %s %d\n",
			sreg->rdata->name,
			rdesc->name,
			pdev->id);

	/**
	 * @author back_yak@cleinsoft
	 * @date 2013/11/18
	 * Buck Channel1~4 Vout Setting && Switching Frequence control
	 **/
#if (ENABLE_DAUDIO_VOUT)
	switch(pdev->id) {
		//CPU Block
		case LTC3676_BUCK_CH1 :
			ltc_3676_clear_and_set_bits(sreg, LTC3676_REG_DVB1A, 0x17);
			ltc_3676_set_bits(sreg, LTC3676_REG_BUCK1, BIT_2_MASK);
			break;

		//GPIO Block
		case LTC3676_BUCK_CH2 :
			ltc_3676_clear_and_set_bits(sreg, LTC3676_REG_DVB2A, 0x018);
			ltc_3676_set_bits(sreg, LTC3676_REG_BUCK2, BIT_2_MASK);
			break;

		//Core Block
		case LTC3676_BUCK_CH3 :
			ltc_3676_clear_and_set_bits(sreg, LTC3676_REG_DVB3A, 0x01A);
			ltc_3676_set_bits(sreg, LTC3676_REG_BUCK3, BIT_2_MASK);
			break;

		//DDR Block
		case LTC3676_BUCK_CH4 :
			ltc_3676_clear_and_set_bits(sreg, LTC3676_REG_DVB4A, 0x01B);
			ltc_3676_set_bits(sreg, LTC3676_REG_BUCK4, BIT_2_MASK);
			break;
	}
#endif

	/* register regulator */
	rdev = regulator_register(rdesc, &pdev->dev,
				  initdata, sreg);

	if (IS_ERR(rdev)) {
		dev_err(&pdev->dev, "failed to register %s\n",
			rdesc->name);
		return PTR_ERR(rdev);
	}

	return 0;
}


int ltc3676_regulator_remove(struct platform_device *pdev)
{
	struct regulator_dev *rdev = platform_get_drvdata(pdev);

	regulator_unregister(rdev);

	return 0;

}

int ltc3676_register_regulator(struct ltc3676_regulator *reg_data, int reg,
			       struct regulator_init_data *initdata)
{
	struct platform_device *pdev;
	int ret;

	if (reg_data->pmic.pdev[reg])
		return -EBUSY;

	pdev = platform_device_alloc("ltc3676_reg", reg);
	if (!pdev)
		return -ENOMEM;

	printk(KERN_INFO "%s: Trying to register regulator %s, %d: %d\n", __func__,
	initdata->constraints.name, reg, ret);
	
	reg_data->pmic.pdev[reg] = pdev;
	initdata->driver_data = reg_data;

	pdev->dev.platform_data = initdata;
	pdev->dev.parent = reg_data->dev;
	
	platform_set_drvdata(pdev, reg_data);
	ret = platform_device_add(pdev);

	if (ret != 0) {
		pr_debug("Failed to register regulator %d: %d\n",
			reg, ret);
		platform_device_del(pdev);
	}
	printk(KERN_INFO "register regulator %s, %d: %d\n",
			reg_data->rdata->name, reg, ret);

	return ret;
}
EXPORT_SYMBOL_GPL(ltc3676_register_regulator);

struct platform_driver ltc3676_reg = {
	.driver = {
		.name	= "ltc3676_reg",
		.owner	= THIS_MODULE,
	},
	.probe	= ltc3676_regulator_probe,
	.remove	= ltc3676_regulator_remove,
};

int ltc3676_regulator_init(void)
{
	printk(KERN_INFO "%s Initializing LTC3676 regulator_init\n", __func__);
	return platform_driver_register(&ltc3676_reg);
}

subsys_initcall_sync(ltc3676_regulator_init);
//postcore_initcall(ltc3676_regulator_init);

void ltc3676_regulator_exit(void)
{
	platform_driver_unregister(&ltc3676_reg);
}
module_exit(ltc3676_regulator_exit);

MODULE_AUTHOR("Linear Technology & NovTech, Inc.");
MODULE_DESCRIPTION("LTC3676 Regulator driver");
MODULE_LICENSE("GPL");
