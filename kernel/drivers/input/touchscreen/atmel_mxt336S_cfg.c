/*
*  atmel_maxtouch.c - Atmel maXTouch Touchscreen Controller
*
*  Version 0.2a
*
*  An early alpha version of the maXTouch Linux driver.
*
*
*  Copyright (C) 2010 Iiro Valkonen <iiro.valkonen@atmel.com>
*  Copyright (C) 2009 Ulf Samuelsson <ulf.samuelsson@atmel.com>
*  Copyright (C) 2009 Raphael Derosso Pereira <raphaelpereira@gmail.com>
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/earlysuspend.h>
#include <linux/firmware.h>
#include <linux/wakelock.h>

#include <linux/delay.h>
#include <linux/i2c.h>

#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/syscalls.h>

#include <linux/atmel_mxt336S.h>
#include "atmel_mxt336S_cfg.h"
#include "atmel_mxt336S_parse.h"
#include "device_config_mxt336S.h"

u8 firmware_latest[] = {
	/* only for test */
	/* #include "mxt_firmware.h" */
};

/* mxt ICs has two slave address,
 * one is the application mode, the other is the boot mode
 * */
static struct mxt_i2c_addr {
	u8 app;		/* application */
	u8 boot;	/* Bootloader */
} mxt_i2c_addr_list[4] = {
	{0x4A, 0x24},
	{0x4B, 0x25},
	{0x5A, 0x34},
	{0x5B, 0x35},
};

/******************************************************************************
*
*
*       Object table init
*
*
* *****************************************************************************/
/** General Object **/
/* Power config settings. */
static gen_powerconfig_t7_config_t power_config = {0, };
/* Acquisition config. */
gen_acquisitionconfig_t8_config_t acquisition_config = {0, };
EXPORT_SYMBOL(acquisition_config);

/** Touch Object **/
/* Multitouch screen config. */
static touch_multitouchscreen_t9_config_t touchscreen_config = {0, };
/* static touch_multitouchscreen_t9_config_t touchscreen_config1 = {0, }; */
/* Key array config. */
/* static touch_keyarray_t15_config_t keyarray_config = {0, }; */

/** Signal Processing Objects **/
/* Proximity Config */
/* static touch_proximity_t52_config_t proximity_config = {0, }; */
/* One-touch gesture config. */
/*static
proci_onetouchgestureprocessor_t24_config_t onetouch_gesture_config = {0, }; */
/*
static
proci_twotouchgestureprocessor_t27_config_t twotouch_gesture_config = {0, };*/

/** Support Objects **/
/* GPIO/PWM config */
/* static spt_gpiopwm_t19_config_t	gpiopwm_config = {0, }; */

/* Selftest config. */
static spt_selftest_t25_config_t selftest_config = {0, };
/* firmware 2.1
 * self test config. structure */
static spt_selftest_t25_ver2_config_t selftest_ver2_config = {0, };
/* Communication config settings. */
static spt_comcconfig_t18_config_t comc_config = {0, };

/* for registers in runtime mode */
struct mxt_runmode_registers_t runmode_regs = {0, };

/* MXT540E Objects */
/* static proci_gripsuppression_t40_config_t gripsuppression_t40_config = {0, }; */
static proci_touchsuppression_t42_config_t \
touchsuppression_t42_config = {0, };
/* firmware2.1 */
static proci_tsuppression_t42_ver2_config_t \
tsuppression_t42_ver2_config = {0, };
static spt_cteconfig_t46_config_t cte_t46_config = {0, };
/* firmware2.1 */
static spt_cteconfig_t46_ver2_config_t cte_t46_ver2_config = {0, };

/* static proci_stylus_t47_config_t stylus_t47_config = {0, }; */
static procg_noisesuppression_t48_config_t \
noisesuppression_t48_config = {0, };

/* firmware2.1 T48 */
static procg_nsuppression_t48_ver2_config_t \
nsuppression_t48_ver2_config = {0, };

static spt_userdata_t38_t	userdata_t38 = {{0}, };

/* add firmware 2.1 */
static proci_adaptivethreshold_t55_config_t \
adaptivethreshold_t55_config = {0, };
/* add firmware 2.1 */
static proci_shieldless_t56_config_t shieldless_t56_config = {0, };
/* add firmware 2.1 */
static proci_extratouchscreendata_t57_config_t \
exttouchdata_t57_config = {0, };

// sjpark@cleinsoft
/* static proci_stylus_t47_config_t stylus_t47_config = {0, }; */
static procg_noisesuppression_t62_config_t \
noisesuppression_t62_config = {0, };

int mxt_get_objects(struct mxt_data *mxt, u8 *buf, int buf_size, int obj_type)
{
	u16 obj_size = 0;
	u16 obj_addr = 0;
	int error;

	if (!mxt || !buf) {
		pr_err("[TSP] invalid parameter (mxt:%p, buf:%p)\n", mxt, buf);
		return -1;
	}

	obj_addr = MXT_BASE_ADDR(obj_type);
	obj_size = MXT_GET_SIZE(obj_type);

	if ((obj_addr == 0) || (obj_size == 0) || (obj_size > buf_size)) {
		dev_err(&mxt->client->dev, "[TSP] Invalid object(addr:0x%04X, "
				"size:%d, object_type:%d)\n",
				obj_addr, obj_size, obj_type);
		return -1;
	}

	error = mxt_read_block(mxt->client, obj_addr, obj_size, buf);
	if (error < 0) {
		dev_err(&mxt->client->dev, "[TSP] mxt_read_block failed! "
				"(%s, %d)\n", __func__, __LINE__);
		return -EIO;
	}

	return 0;
}

int mxt_get_object_values(struct mxt_data *mxt, int obj_type)
{
	struct i2c_client *client = mxt->client;


	u8 *obj = NULL;
	u16 obj_size = 0;
	u16 obj_addr = 0;
	int error;
	int version;
	version = (mxt->device_info.major * 10) + (mxt->device_info.minor);

	switch (obj_type) {
	case MXT_GEN_POWERCONFIG_T7:
		obj = (u8 *)&power_config;
		break;

	case MXT_GEN_ACQUIRECONFIG_T8:
		obj = (u8 *)&acquisition_config;
		break;

	case MXT_TOUCH_MULTITOUCHSCREEN_T9:
		obj = (u8 *)&touchscreen_config;
		break;

	/*case MXT_PROCI_GRIPSUPPRESSION_T40:
		obj = (u8 *)&gripsuppression_t40_config;
		break;*/

	case MXT_PROCI_TOUCHSUPPRESSION_T42:
		obj = (u8 *)&touchsuppression_t42_config;
		break;

	case MXT_SPT_CTECONFIG_T46:
		obj = (u8 *)&cte_t46_config;
		break;

	/*case MXT_PROCI_STYLUS_T47:
		obj = (u8 *)&stylus_t47_config;
		break;*/

	case MXT_PROCG_NOISESUPPRESSION_T48:
		if (version <= 10) {
			obj = (u8 *)&noisesuppression_t48_config;
			break;
		}
		else if (version >= 21) {
			obj = (u8 *)&nsuppression_t48_ver2_config;
			break;
		}
	case MXT_USER_INFO_T38:
		obj = (u8 *)&userdata_t38;
		break;

	default:
		dev_err(&mxt->client->dev, "[TSP] Not supporting object type "
				"(object type: %d)", obj_type);
		return -1;
	}

	obj_addr = MXT_BASE_ADDR(obj_type);
	obj_size = MXT_GET_SIZE(obj_type);

	if ((obj_addr == 0) || (obj_size == 0)) {
		dev_err(&mxt->client->dev, "[TSP] Not supporting object type "
				"(object type: %d)\n", obj_type);
		return -1;
	}

	error = mxt_read_block(client, obj_addr, obj_size, obj);
	if (error < 0) {
		dev_err(&client->dev, "[TSP] mxt_read_block failed! "
				"(%s, %d)\n", __func__, __LINE__);
		return -EIO;
	}

	return 0;
}

int mxt_copy_object(struct mxt_data *mxt, u8 *buf, int obj_type)
{
	u8 *obj = NULL;
	u16 obj_size = 0;
	int version;
	version = (mxt->device_info.major * 10) + (mxt->device_info.minor);

	switch (obj_type) {
	case MXT_GEN_POWERCONFIG_T7:
		obj = (u8 *)&power_config;
		obj_size = MXT_GET_SIZE(obj_type);
		break;

	case MXT_GEN_ACQUIRECONFIG_T8:
		obj = (u8 *)&acquisition_config;
		obj_size = MXT_GET_SIZE(obj_type);
		break;

	case MXT_TOUCH_MULTITOUCHSCREEN_T9:
		obj = (u8 *)&touchscreen_config;
		obj_size = MXT_GET_SIZE(obj_type);
		break;

	/*case MXT_PROCI_GRIPSUPPRESSION_T40:
		obj = (u8 *)&gripsuppression_t40_config;
		obj_size = MXT_GET_SIZE(obj_type);
		break;*/

	case MXT_PROCI_TOUCHSUPPRESSION_T42:
		obj = (u8 *)&touchsuppression_t42_config;
		obj_size = MXT_GET_SIZE(obj_type);
		break;

	case MXT_SPT_CTECONFIG_T46:
		obj = (u8 *)&cte_t46_config;
		obj_size = MXT_GET_SIZE(obj_type);
		break;

	/*case MXT_PROCI_STYLUS_T47:
		obj = (u8 *)&stylus_t47_config;
		obj_size = MXT_GET_SIZE(obj_type);
		break;*/

	case MXT_PROCG_NOISESUPPRESSION_T48:
		if (version <= 10) {
			obj = (u8 *)&noisesuppression_t48_config;
			obj_size = MXT_GET_SIZE(obj_type);
			break;
		}
		else if (version >= 21) {
			obj = (u8 *)&nsuppression_t48_ver2_config;
			obj_size = MXT_GET_SIZE(obj_type);
			break;
		}

	case MXT_USER_INFO_T38:
		obj = (u8 *)&userdata_t38;
		obj_size = MXT_GET_SIZE(obj_type);
		break;

	default:
		dev_err(&mxt->client->dev, "[TSP] Not supporting object type "
				"(object type: %d)\n", obj_type);
		return -1;
	}

	dev_info(&mxt->client->dev, "[TSP] obj type: %d, obj size: %d\n",
			obj_type, obj_size);

	if (memcpy(buf, obj, obj_size) == NULL) {
		dev_err(&mxt->client->dev, "[TSP] memcpy failed! (%s, %d)\n",
				__func__, __LINE__);
		return -1;
	}

	return 0;
}

int mxt_userdata_config(struct mxt_data *mxt)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error;

	obj_addr = MXT_BASE_ADDR(MXT_USER_INFO_T38);
	obj_size = MXT_GET_SIZE(MXT_USER_INFO_T38);

	userdata_t38.data[0] = T38_USERDATA0;
	userdata_t38.data[1] = T38_USERDATA1;
	userdata_t38.data[2] = T38_USERDATA2;
	userdata_t38.data[3] = T38_USERDATA3;
	userdata_t38.data[4] = T38_USERDATA4;
	userdata_t38.data[5] = T38_USERDATA5;
	userdata_t38.data[6] = T38_USERDATA6;
	userdata_t38.data[7] = T38_USERDATA7;

	error = mxt_write_block(client,
			obj_addr, obj_size, (u8 *)&userdata_t38);
	if (error < 0) {
		dev_err(&client->dev, "[TSP] mxt_write_block failed! "
				"(%s, %d)\n", __func__, __LINE__);
		return -EIO;
	}

	return 0;
}

int mxt_power_config(struct mxt_data *mxt)
{
	struct i2c_client *client = mxt->client;

	u16 obj_addr, obj_size;
	int error;

	obj_addr = MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7);
	obj_size = MXT_GET_SIZE(MXT_GEN_POWERCONFIG_T7);

	/* Set Idle Acquisition Interval to 32 ms. */
	power_config.idleacqint     = T7_IDLEACQINT;
	/* Set Active Acquisition Interval to 16 ms. */
	power_config.actvacqint     = T7_ACTVACQINT;
	/* Set Active to Idle Timeout to 4 s (one unit = 200ms). */
	power_config.actv2idleto    = T7_ACTV2IDLETO;

	// sjpark@cleinsoft : modified
	power_config.cfg    = T7_CFG;

	error = mxt_write_block(client,
			obj_addr, obj_size, (u8 *)&power_config);
	if (error < 0) {
		dev_err(&client->dev, "[TSP] mxt_write_block failed! "
				"(%s, %d)\n", __func__, __LINE__);
		return -EIO;
	}
	return 0;
}

/* extern bool mxt_reconfig_flag; */
int mxt_acquisition_config(struct mxt_data *mxt)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error;

	obj_addr = MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8);
	obj_size = MXT_GET_SIZE(MXT_GEN_ACQUIRECONFIG_T8);

	acquisition_config.chrgtime     = T8_CHRGTIME;
	acquisition_config.reserved     = T8_RESERVED;
	acquisition_config.tchdrift     = T8_TCHDRIFT;
	acquisition_config.driftst      = T8_DRIFTST;
	acquisition_config.tchautocal   = T8_TCHAUTOCAL;
	acquisition_config.sync         = T8_SYNC;
	acquisition_config.atchcalst    = T8_ATCHCALST;
	acquisition_config.atchcalsthr  = T8_ATCHCALSTHR;
	/*!< Anti-touch force calibration threshold */
	acquisition_config.atchcalfrcthr = T8_ATCHFRCCALTHR;
	/*!< Anti-touch force calibration ratio */
	acquisition_config.atchcalfrcratio = T8_ATCHFRCCALRATIO;

	error = mxt_write_block(client,
			obj_addr, obj_size, (u8 *)&acquisition_config);
	if (error < 0) {
		dev_err(&client->dev, "[TSP] mxt_write_block failed! "
				"(%s, %d)\n", __func__, __LINE__);
		return -EIO;
	}

	return 0;
}
EXPORT_SYMBOL(mxt_acquisition_config);

int mxt_multitouch_config(struct mxt_data *mxt)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error;

	obj_addr = MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9);
	obj_size = MXT_GET_SIZE(MXT_TOUCH_MULTITOUCHSCREEN_T9);

	touchscreen_config.ctrl         = T9_CTRL;
	touchscreen_config.xorigin      = T9_XORIGIN;
	touchscreen_config.yorigin      = T9_YORIGIN;
	touchscreen_config.xsize        = T9_XSIZE;
	touchscreen_config.ysize        = T9_YSIZE;
	touchscreen_config.akscfg       = T9_AKSCFG;
	touchscreen_config.blen         = T9_BLEN;

	touchscreen_config.tchthr       = T9_TCHTHR;

	touchscreen_config.tchdi        = T9_TCHDI;
	touchscreen_config.orient       = T9_ORIENT;
	touchscreen_config.mrgtimeout   = T9_MRGTIMEOUT;
	touchscreen_config.movhysti     = T9_MOVHYSTI;
	touchscreen_config.movhystn     = T9_MOVHYSTN;
	touchscreen_config.movfilter    = T9_MOVFILTER;
	touchscreen_config.numtouch     = T9_NUMTOUCH;
	touchscreen_config.mrghyst      = T9_MRGHYST;
	touchscreen_config.mrgthr       = T9_MRGTHR;
	touchscreen_config.amphyst      = T9_AMPHYST;
	touchscreen_config.xrange       = T9_XRANGE;
	touchscreen_config.yrange       = T9_YRANGE;
	touchscreen_config.xloclip      = T9_XLOCLIP;
	touchscreen_config.xhiclip      = T9_XHICLIP;
	touchscreen_config.yloclip      = T9_YLOCLIP;
	touchscreen_config.yhiclip      = T9_YHICLIP;
	touchscreen_config.xedgectrl    = T9_XEDGECTRL;
	touchscreen_config.xedgedist    = T9_XEDGEDIST;
	touchscreen_config.yedgectrl    = T9_YEDGECTRL;
	touchscreen_config.yedgedist    = T9_YEDGEDIST;
	touchscreen_config.jumplimit    = T9_JUMPLIMIT;

	touchscreen_config.tchhyst     = T9_TCHHYST;
	touchscreen_config.xpitch      = T9_XPITCH;
	touchscreen_config.ypitch      = T9_YPITCH;

	touchscreen_config.nexttchdi   = T9_NEXTTCHDI;
	touchscreen_config.cfg         = T9_CFG;

	error = mxt_write_block(client, obj_addr,
			obj_size, (u8 *)&touchscreen_config);
	if (error < 0) {
		dev_err(&client->dev, "[TSP] mxt_write_block failed! "
				"(%s, %d)\n", __func__, __LINE__);
		return -EIO;
	}
	return 0;
}

int mxt_touch_suppression_t42_config(struct mxt_data *mxt)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error;
	int version;
	version = (mxt->device_info.major * 10) + (mxt->device_info.minor);

	obj_addr = MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42);
	obj_size = MXT_GET_SIZE(MXT_PROCI_TOUCHSUPPRESSION_T42);

	//valky@cleinsoft if (version <= 10) {
	if (1) {
		touchsuppression_t42_config.ctrl = T42_CTRL;
		touchsuppression_t42_config.apprthr = T42_APPRTHR;
		touchsuppression_t42_config.maxapprarea = T42_MAXAPPRAREA;
		touchsuppression_t42_config.maxtcharea  = T42_MAXTCHAREA;
		touchsuppression_t42_config.supstrength = T42_SUPSTRENGTH;
		touchsuppression_t42_config.supextto    = T42_SUPEXTTO;
		touchsuppression_t42_config.maxnumtchs  = T42_MAXNUMTCHS;
		touchsuppression_t42_config.shapestrength = T42_SHAPESTRENGTH;

		error = mxt_write_block(client, obj_addr,
				obj_size, (u8 *)&touchsuppression_t42_config);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}
	else if (version >= 21) {
		tsuppression_t42_ver2_config.ctrl = T42_CTRL;
		tsuppression_t42_ver2_config.apprthr = T42_APPRTHR;
		tsuppression_t42_ver2_config.maxapprarea = T42_MAXAPPRAREA;
		tsuppression_t42_ver2_config.maxtcharea = T42_MAXTCHAREA;
		tsuppression_t42_ver2_config.supstrength = T42_SUPSTRENGTH;
		tsuppression_t42_ver2_config.supextto = T42_SUPEXTTO;
		tsuppression_t42_ver2_config.maxnumtchs = T42_MAXNUMTCHS;
		tsuppression_t42_ver2_config.shapestrength = T42_SHAPESTRENGTH;
		tsuppression_t42_ver2_config.supdist = T42_SUPDIST;
		tsuppression_t42_ver2_config.disthyst = T42_DISTHYST;

		error = mxt_write_block(client, obj_addr,
				obj_size, (u8 *)&tsuppression_t42_ver2_config);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}
	return 0;
}

int mxt_cte_t46_config(struct mxt_data *mxt)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error = 0;
	int version;
	version = (mxt->device_info.major * 10) + (mxt->device_info.minor);

	obj_addr = MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46);
	obj_size = MXT_GET_SIZE(MXT_SPT_CTECONFIG_T46);

	/* Set CTE config */
	//valky@cleinsoft if (version <= 10) {
	if (1) {
		cte_t46_config.ctrl = T46_CTRL;
		cte_t46_config.mode = T46_RESERVED;
		cte_t46_config.idlesyncsperx = T46_IDLESYNCSPERX;
		cte_t46_config.actvsyncsperx = T46_ACTVSYNCSPERX;
		cte_t46_config.adcspersync	= T46_ADCSPERSYNC;
		cte_t46_config.pulsesperadc = T46_PULSESPERADC;
		cte_t46_config.xslew = T46_XSLEW;
		cte_t46_config.syncdelay = T46_SYNCDELAY;
		cte_t46_config.xvoltage = T46_XVOLTAGE;

		/* Write CTE config to chip. */
		error = mxt_write_block(client, obj_addr, obj_size,
				(u8 *)&cte_t46_config);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}
	else if (version >= 21) {
		cte_t46_ver2_config.ctrl = T46_CTRL;
		cte_t46_ver2_config.reserved = T46_RESERVED;
		cte_t46_ver2_config.idlesyncsperx = T46_IDLESYNCSPERX;
		cte_t46_ver2_config.actvsyncsperx = T46_ACTVSYNCSPERX;
		cte_t46_ver2_config.adcspersync	= T46_ADCSPERSYNC;
		cte_t46_ver2_config.pulsesperadc = T46_PULSESPERADC;
		cte_t46_ver2_config.xslew = T46_XSLEW;
		cte_t46_ver2_config.syncdelay = T46_SYNCDELAY;
		cte_t46_ver2_config.xvoltage = T46_XVOLTAGE;

		/* Write CTE config to chip. */
		error = mxt_write_block(client, obj_addr, obj_size,
				(u8 *)&cte_t46_ver2_config);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	return error;
}

int mxt_noisesuppression_t48_config(struct mxt_data *mxt)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size, size;
	int error;

	obj_addr = MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T48);
	obj_size = MXT_GET_SIZE(MXT_PROCG_NOISESUPPRESSION_T48);

	size = sizeof(noisesuppression_t48_config);
	if (obj_size > size) {
		char *buff = NULL;
		buff = kzalloc(obj_size, GFP_KERNEL);
		if (!buff) {
			dev_err(&client->dev, "[TSP] failed to alloc mem\n");
			return -ENOMEM;
		}
		memcpy(buff, &noisesuppression_t48_config, size);
		error = mxt_write_block(client,
				obj_addr, obj_size, buff);
		kfree(buff);
	} else {
		error = mxt_write_block(client,
				obj_addr, obj_size,
				(u8 *)&noisesuppression_t48_config);
	}
	if (error < 0) {
		dev_err(&client->dev, "[TSP] mxt_write_block failed! "
				"(%s, %d)\n", __func__, __LINE__);
		return -EIO;
	}

	return 0;
}
EXPORT_SYMBOL(mxt_noisesuppression_t48_config);

int mxt_noisesuppression_t48_ver2_config(struct mxt_data *mxt)
{
	return 0;
}

EXPORT_SYMBOL(mxt_noisesuppression_t48_ver2_config);

int mxt_proximity_config(struct mxt_data *mxt)
{
	return 1;
}

int mxt_proximity_config1(struct mxt_data *mxt)
{
	return 1;
}

int mxt_shieldless_t56_config(struct mxt_data *mxt)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error = 0;

	obj_addr = MXT_BASE_ADDR(MXT_SHIELDLESS_T56);
	obj_size = MXT_GET_SIZE(MXT_SHIELDLESS_T56);

	shieldless_t56_config.ctrl = T56_CTRL;
	shieldless_t56_config.command = T56_COMMAND;
	shieldless_t56_config.optint = T56_OPTINT;
	shieldless_t56_config.inttime = T56_INTTIME;
	shieldless_t56_config.intdelay[0] = T56_INTDELAY_0;
	shieldless_t56_config.intdelay[1] = T56_INTDELAY_1;
	shieldless_t56_config.intdelay[2] = T56_INTDELAY_2;
	shieldless_t56_config.intdelay[3] = T56_INTDELAY_3;
	shieldless_t56_config.intdelay[4] = T56_INTDELAY_4;
	shieldless_t56_config.intdelay[5] = T56_INTDELAY_5;
	shieldless_t56_config.intdelay[6] = T56_INTDELAY_6;
	shieldless_t56_config.intdelay[7] = T56_INTDELAY_7;
	shieldless_t56_config.intdelay[8] = T56_INTDELAY_8;
	shieldless_t56_config.intdelay[9] = T56_INTDELAY_9;
	shieldless_t56_config.intdelay[10] = T56_INTDELAY_10;
	shieldless_t56_config.intdelay[11] = T56_INTDELAY_11;
	shieldless_t56_config.intdelay[12] = T56_INTDELAY_12;
	shieldless_t56_config.intdelay[13] = T56_INTDELAY_13;
	shieldless_t56_config.intdelay[14] = T56_INTDELAY_14;
	shieldless_t56_config.intdelay[15] = T56_INTDELAY_15;
	shieldless_t56_config.intdelay[16] = T56_INTDELAY_16;
	shieldless_t56_config.intdelay[17] = T56_INTDELAY_17;
	shieldless_t56_config.intdelay[18] = T56_INTDELAY_18;
	shieldless_t56_config.intdelay[19] = T56_INTDELAY_19;
	shieldless_t56_config.intdelay[20] = T56_INTDELAY_20;
	shieldless_t56_config.intdelay[21] = T56_INTDELAY_21;
	shieldless_t56_config.intdelay[22] = T56_INTDELAY_22;
	shieldless_t56_config.intdelay[23] = T56_INTDELAY_23;
	shieldless_t56_config.multicutgc = T56_MULTICUTGC;
	shieldless_t56_config.reserved = T56_RESERVED;
	shieldless_t56_config.ncncl = T56_NCNCL;
	shieldless_t56_config.touchbias = T56_TOUCHBIAS;
	shieldless_t56_config.basescale = T56_BASESCALE;
	shieldless_t56_config.shiftlimit = T56_SHIFTLIMIT;
	shieldless_t56_config.ylonoisemul = T56_YLONOISEMUL;
	shieldless_t56_config.ylonoisediv = T56_YLONOISEDIV;
	shieldless_t56_config.yhinoisemul = T56_YHINOISEMUL;
	shieldless_t56_config.yhinoisediv = T56_YHINOISEDIV;
	shieldless_t56_config.reserved = T56_RESERVED;

	error = mxt_write_block(client, obj_addr,
			obj_size, (u8 *)&shieldless_t56_config);
	if (error < 0) {
		dev_err(&client->dev, "[TSP] mxt_write_block failed! "
				"(%s, %d)\n", __func__, __LINE__);
		return -EIO;
	}
	return 0;
}

int mxt_extra_touchscreen_data_t57_config(struct mxt_data *mxt)
{
	return 0;
}

int mxt_extra_touchscreen_data_t57_config1(struct mxt_data *mxt)
{
	return 0;
}

/**
 * @author sjpark@cleinsoft
 * @date 2013/11/27
 * mXT336S_AT new fuction for T62
 **/
int mxt_noisesuppression_t62_config(struct mxt_data *mxt)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error;
	obj_addr = MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62);
	obj_size = MXT_GET_SIZE(MXT_PROCG_NOISESUPPRESSION_T62);

	noisesuppression_t62_config.ctrl			= T62_CTRL;
	noisesuppression_t62_config.calcfg1			= T62_CALCFG1;
	noisesuppression_t62_config.calcfg2			= T62_CALCFG2;
	noisesuppression_t62_config.calcfg3			= T62_CALCFG3;
	noisesuppression_t62_config.cfg				= T62_CFG1;
	noisesuppression_t62_config.reserved0		= T62_RESERVED0;
	noisesuppression_t62_config.minthradj		= T62_MINTHRADJ;
	noisesuppression_t62_config.basefreq		= T62_BASEFREQ;
	noisesuppression_t62_config.maxselfreq		= T62_MAXSELFREQ;
	noisesuppression_t62_config.freq0			= T62_FREQ0;
	noisesuppression_t62_config.freq1			= T62_FREQ1;
	noisesuppression_t62_config.freq2			= T62_FREQ2;
	noisesuppression_t62_config.freq3			= T62_FREQ3;
	noisesuppression_t62_config.freq4			= T62_FREQ4;
	noisesuppression_t62_config.hopcnt			= T62_HOPCNT;
	noisesuppression_t62_config.reserved1		= T62_RESERVED1;
	noisesuppression_t62_config.hopcntper		= T62_HOPCNTPER;
	noisesuppression_t62_config.hopevalto		= T62_HOPEVALTO;
	noisesuppression_t62_config.hopst			= T62_HOPST;
	noisesuppression_t62_config.nlgain			= T62_NLGAIN;
	noisesuppression_t62_config.minnlthr		= T62_MINNLTHR;
	noisesuppression_t62_config.incnlthr		= T62_INCNLTHR;
	noisesuppression_t62_config.adcperxthr		= T62_ADCPERXTHR;
	noisesuppression_t62_config.nlthrmargin		= T62_NLTHRMARGIN;
	noisesuppression_t62_config.maxadcperx		= T62_MAXADCPERX;
	noisesuppression_t62_config.actvadcsvldnod	= T62_ACTVADCSVLDNOD;
	noisesuppression_t62_config.idleadcsvldnod 	= T62_IDLEADCSVLDNOD;
	noisesuppression_t62_config.mingclimit		= T62_MINGCLIMIT;
	noisesuppression_t62_config.maxgclimit		= T62_MAXGCLIMIT;
	noisesuppression_t62_config.reserved2		= T62_RESERVED2;
	noisesuppression_t62_config.reserved3		= T62_RESERVED3;
	noisesuppression_t62_config.reserved4		= T62_RESERVED4;
	noisesuppression_t62_config.reserved5		= T62_RESERVED5;
	noisesuppression_t62_config.reserved6		= T62_RESERVED6;
	noisesuppression_t62_config.blen0			= T62_BLEN;
	noisesuppression_t62_config.tchthr0			= T62_TCHTHR;
	noisesuppression_t62_config.tchdi0			= T62_TCHDI;
	noisesuppression_t62_config.movhysti0		= T62_MOVHYSTI;
	noisesuppression_t62_config.movhystn0		= T62_MOVHYSTN;
	noisesuppression_t62_config.movfilter0		= T62_MOVFILTER;
	noisesuppression_t62_config.numtouch0		= T62_NUMTOUCH;
	noisesuppression_t62_config.mrghyst0		= T62_MRGHYST;
	noisesuppression_t62_config.mrgthr0			= T62_MRGTHR;
	noisesuppression_t62_config.xloclip0		= T62_XLOCLIP;
	noisesuppression_t62_config.xhiclip0		= T62_XHICLIP;
	noisesuppression_t62_config.yloclip0		= T62_YLOCLIP;
	noisesuppression_t62_config.yhiclip0		= T62_YHICLIP;
	noisesuppression_t62_config.xedgectrl0		= T62_XEDGECTRL;
	noisesuppression_t62_config.xedgedist0		= T62_XEDGEDIST;
	noisesuppression_t62_config.yedgectrl0		= T62_YEDGECTRL;
	noisesuppression_t62_config.yedgedist0		= T62_YEDGEDIST;
	noisesuppression_t62_config.jumplimit0		= T62_JUMPLIMIT;
	noisesuppression_t62_config.tchhyst0		= T62_TCHHYST;
	noisesuppression_t62_config.nexttchdi0		= T62_NEXTTCHDI;

	error = mxt_write_block(client, obj_addr, obj_size,
                (u8 *)&noisesuppression_t62_config);
	if (error < 0) {
		dev_err(&client->dev, "[TSP] noisesuppression_t62_config error "
				"(%s, %d)\n", __func__, __LINE__);
		return -EIO;
	}

	return 0;
}
EXPORT_SYMBOL(mxt_noisesuppression_t62_config);

void mxt_config_get_runmode_registers(struct mxt_runmode_registers_t *regs)
{
	memcpy(regs, &runmode_regs,
			sizeof(struct mxt_runmode_registers_t));
}

int mxt_config_save_runmode(struct mxt_data *mxt)
{
	int ret = 0;
	int retry = 3;
	u8 t38_buff[64] = {0, };
	u16 t38_addr;
	struct i2c_client *client = mxt->client;

	t38_addr = MXT_BASE_ADDR(MXT_USER_INFO_T38);
retry_runmode:
	ret = mxt_get_object_values(mxt, MXT_GEN_ACQUIRECONFIG_T8);
	/* ret |= mxt_get_object_values(mxt, MXT_TOUCH_MULTITOUCHSCREEN_T9); */
	/* ret |= mxt_get_object_values(mxt, MXT_PROCI_TOUCHSUPPRESSION_T42); */
	ret |= mxt_read_block(client, t38_addr + MXT_ADR_T38_USER1,
			4, &t38_buff[MXT_ADR_T38_USER1]);
	ret |= mxt_read_block(client, t38_addr + MXT_ADR_T38_USER25,
			10, &t38_buff[MXT_ADR_T38_USER25]);

	if (ret == 0) {
		/* T8 */
		/* The below registers will be reloaded after calibration */
		runmode_regs.t8_atchcalst = acquisition_config.atchcalst;
		runmode_regs.t8_atchcalsthr = acquisition_config.atchcalsthr;
		runmode_regs.t8_atchfrccalthr =
			acquisition_config.atchcalfrcthr;
		runmode_regs.t8_atchfrccalratio =
			acquisition_config.atchcalfrcratio;

		runmode_regs.t38_atchcalst = t38_buff[MXT_ADR_T38_USER1];
		runmode_regs.t38_atchcalsthr = t38_buff[MXT_ADR_T38_USER2];
		runmode_regs.t38_atchfrccalthr = t38_buff[MXT_ADR_T38_USER3];
		runmode_regs.t38_atchfrccalratio = t38_buff[MXT_ADR_T38_USER4];
		runmode_regs.t38_palm_check_flag = t38_buff[MXT_ADR_T38_USER26];
		memcpy(&runmode_regs.t38_palm_param,
				&t38_buff[MXT_ADR_T38_USER27], 4);
		runmode_regs.t38_cal_thr = t38_buff[MXT_ADR_T38_USER32];
		runmode_regs.t38_num_of_antitouch = t38_buff[MXT_ADR_T38_USER33];
		runmode_regs.t38_supp_ops = t38_buff[MXT_ADR_T38_USER34];
	} else {
		if (retry--) {
			dev_warn(&client->dev,
					"%s() retry:%d\n", __func__, retry);
			goto retry_runmode;
		} else {
			dev_warn(&client->dev, "%s() failed to"
					"get runmode configs.\n"
					"set the default value\n", __func__);
			runmode_regs.t8_atchcalst = T8_ATCHCALST;
			runmode_regs.t8_atchcalsthr = T8_ATCHCALSTHR;
			runmode_regs.t8_atchfrccalthr = T8_ATCHFRCCALTHR;
			runmode_regs.t8_atchfrccalratio = T8_ATCHFRCCALRATIO;

			runmode_regs.t38_palm_check_flag = 1;
			/* 2012_0510 : ATMEL */
			runmode_regs.t38_palm_param[0] = 100;
			runmode_regs.t38_palm_param[1] = 90;
			runmode_regs.t38_palm_param[2] = 100;
			runmode_regs.t38_palm_param[3] = 10;
			/* 2012_0510 : ATEML */
			runmode_regs.t38_cal_thr = 10;
			runmode_regs.t38_num_of_antitouch = 2;

			runmode_regs.t38_atchcalst = T8_ATCHCALST;
			runmode_regs.t38_atchcalsthr = T8_ATCHCALSTHR;
			runmode_regs.t38_atchfrccalthr = T8_ATCHFRCCALTHR;
			runmode_regs.t38_atchfrccalratio = T8_ATCHFRCCALRATIO;

			/* 2012_0521 :
			 *  - kykim : 5sec = 5000ms */
			runmode_regs.t38_supp_ops = 81;
		}
	}

	return ret;
}

int mxt_config_settings(struct mxt_data *mxt)
{
	int version;
	version = (mxt->device_info.major * 10) + (mxt->device_info.minor);

	printk("%s\n", __func__);
	dev_dbg(&mxt->client->dev, "[TSP] mxt_config_settings");

	if (mxt_userdata_config(mxt) < 0)	/* T38 */
		return -1;

	if (mxt_power_config(mxt) < 0)		/* T7 */
		return -1;

	if (mxt_acquisition_config(mxt) < 0)	/* T8 */
		return -1;

	if (mxt_multitouch_config(mxt) < 0)	/* T9 */
		return -1;

	if (mxt_touch_suppression_t42_config(mxt) < 0) /* T42 */
		return -1;

	if (mxt_cte_t46_config(mxt) < 0)               /* T46 */
		return -1;

	if (mxt_noisesuppression_t62_config(mxt) < 0)               /* T62 */
		return -1;

	if (mxt_shieldless_t56_config(mxt) < 0)
		return -1;

	return 0;
}

/*
* Bootloader functions
*/

static void bootloader_status(struct i2c_client *client, u8 value)
{
	u8 *str = NULL;

	switch (value) {
	case 0xC0:
		str = "WAITING_BOOTLOAD_CMD"; break;
	case 0x80:
		str = "WAITING_FRAME_DATA"; break;
	case 0x40:
		str = "APP_CRC_FAIL"; break;
	case 0x02:
		str = "FRAME_CRC_CHECK"; break;
	case 0x03:
		str = "FRAME_CRC_FAIL"; break;
	case 0x04:
		str = "FRAME_CRC_PASS"; break;
	default:
		str = "Unknown Status"; break;
	}

	dev_dbg(&client->dev, "[TSP] bootloader status: %s (0x%02X)\n",
			str, value);
}

static int mxt_wait_change_signal(struct mxt_data *mxt)
{
	int ret = 0;

	if (mxt->pdata->check_chg_platform_hw) {
		int retry = 0;

		do {
			/* wait 100ms */
			if (retry > 100)
				return -EIO;
			ret = mxt->pdata->check_chg_platform_hw(NULL);
			if (ret == 0)
				break;
			mdelay(1);
			dev_info(&mxt->client->dev, "wait CHG, ret = %d[%d]\n",
					ret, retry);
		} while (retry++);
	}

	return ret;
}

static int check_bootloader(struct mxt_data *mxt, unsigned int status)
{
	u8 val = 0;
	u16 retry = 0;
	struct i2c_client *client = mxt->client;

	msleep(10);  /* recommendation from ATMEL */

recheck:
	if (retry++ >= 10) {
		dev_err(&client->dev, "[TSP] failed to check the request\n");
		return -EINVAL;
	}

	if (status == WAITING_BOOTLOAD_COMMAND ||
			status == FRAME_CRC_PASS) {
		int ret;
		/* in this case, we should check CHG line */
		/* I think that msleep(10) is enough, but keep the datasheet */
		ret = mxt_wait_change_signal(mxt);
		if (ret < 0) {
			dev_err(&client->dev, "[TSP] CHG didn't go down\n");
			return -EIO;
		}
	}

	if (i2c_master_recv(client, &val, 1) != 1) {
		dev_err(&client->dev, "[TSP] i2c recv failed\n");
		return -EIO;
	}

	switch (status) {
	case WAITING_FRAME_DATA:
	case WAITING_BOOTLOAD_COMMAND:
		val &= ~BOOTLOAD_STATUS_MASK;
		bootloader_status(client, val);
		if (val == APP_CRC_FAIL) {
			dev_info(&client->dev, "[TSP] We've got a APP_CRC_FAIL,"
					" so try again (count=%d)\n", retry);
			goto recheck;
		}
		break;

	case FRAME_CRC_PASS:
		bootloader_status(client, val);
		/* this checks FRAME_CRC_CHECK and FRAME_CRC_PASS */
		if (val == FRAME_CRC_CHECK)
			goto recheck;
		break;

	default:
		return -EINVAL;
	}

	if (val != status) {
		dev_err(&client->dev, "[TSP] Invalid status: 0x%02X\n", val);
		return -EINVAL;
	}

	return 0;
}

static int unlock_bootloader(struct i2c_client *client)
{
	u8 cmd[2] = { 0};

	cmd[0] = 0xdc;  /* MXT_CMD_UNLOCK_BL_LSB */
	cmd[1] = 0xaa;  /*MXT_CMD_UNLOCK_BL_MSB */

	return i2c_master_send(client, cmd, 2);
}

void mxt_change_i2c_addr(struct mxt_data *mxt, bool to_boot)
{
	int i, size;
	u8 to, from;

	size = sizeof(mxt_i2c_addr_list) / sizeof(mxt_i2c_addr_list[0]);
	to = from = mxt->client->addr;

	for (i = 0; i < size; i++) {
		if (to_boot) {
			/* application to bootloader */
			if (from == mxt_i2c_addr_list[i].app) {
				to = mxt_i2c_addr_list[i].boot;
				break;
			}
		} else {
			/* bootloader to application */
			if (from == mxt_i2c_addr_list[i].boot) {
				to = mxt_i2c_addr_list[i].app;
				break;
			}
		}
	}

	if (likely(i < size)) {
		mxt->client->addr = to;
	} else {
		dev_info(&mxt->client->dev,
				"%s() invalid i2c slave addr : %02X\n",
				__func__, mxt->client->addr);
		/* set the default mx336s address */
		if (to_boot)
			mxt->client->addr = to = 0x24;
		else
			mxt->client->addr = to = 0x4A;
	}
	dev_info(&mxt->client->dev,
			"[TSP] I2C address: 0x%02X --> 0x%02X\n", from, to);
}

/**
 * @author sjpark@cleinsoft
 * @date 2014/11/18
 * Release firmware structure 
 **/
int release_mxt_firmware(struct firmware *fw)
{
    if(fw==NULL)
        return -EINVAL;
    if(fw->size > 0 && fw->data)
        kfree(fw->data);
    kfree(fw);
}

/**
 * @author sjpark@cleinsoft
 * @date 2014/11/18
 * Read firmware packet frame data.
 **/
int mxt_read_firmware_fs(struct firmware *fw, const char *path, unsigned int pos)
{
    int fd = 0, ret = 0, frame_size = 0;
    char frame_bytes[2]={0};
    mm_segment_t old_fs;       

    old_fs = get_fs();         
    set_fs(KERNEL_DS);         
    fd = sys_open(path, O_RDONLY, 0);

    if (fd >= 0){
        if(sys_lseek(fd, pos, SEEK_SET) != pos){
            dev_err("[TSP] FW file seeking err (%s)\n", __func__); 
            goto out;
        }
        if((ret = sys_read(fd, frame_bytes, FWFRAME_INFO_LEN)) != FWFRAME_INFO_LEN){
            dev_err("[TSP] Reading touch firmware frame size err  : %d\n", ret);
            goto out;
        }
		frame_size = (frame_bytes[0] << 8) | frame_bytes[1];
        if(frame_size >= fw->size || frame_size <= 0){
            dev_err("[TSP] Touch firmware frame size err : %d\n", frame_size);
            goto out;
        }
        fw->data = kmalloc(frame_size+FWFRAME_INFO_LEN, GFP_KERNEL);
        if(fw->data == NULL){
            dev_err("[TSP] Frame memory alloc err (%s)\n", __func__); 
            ret = -ENOMEM;
            goto out;
        }
        if(sys_lseek(fd, pos, SEEK_SET) != pos){
            dev_err("[TSP] FW file seeking err (%s)\n", __func__); 
            goto out;
        }
        ret = sys_read(fd, fw->data, frame_size+FWFRAME_INFO_LEN); 
        if(ret<0){
            dev_err("[TSP] Reading touch firmware file err: %d\n", ret);
        }
    }
    else{
        dev_err("[TSP] Cannot open firmware file: %d\n", fd);
    }
out:
    if(fd>=0) 
        sys_close(fd);
    return ret;
}

/**
 * @author sjpark@cleinsoft
 * @date 2014/11/18
 * Creating firmware object & get file size for updating
 **/
int mxt_init_firmware_fs(const struct firmware **firmware_p, const char *name, 
        char* path, struct device *device)          
{ 
    int ret = 0, fd = 0, sz = 0;
    unsigned char *data;
    struct firmware *firmware;
    mm_segment_t old_fs;       

    if(path==NULL)
        return -EINVAL;

    *firmware_p = firmware = kzalloc(sizeof(*firmware), GFP_KERNEL);
    if (firmware==NULL) {           
        dev_err(device, "[TSP] %s: kmalloc(struct firmware) failed\n", __func__);         
        ret = -ENOMEM;      
        goto out;              
    }

    old_fs = get_fs();         
    set_fs(KERNEL_DS);         
    fd = sys_open(path, O_RDONLY, 0);
    if (fd >= 0){
        sz = sys_lseek(fd, 0L, SEEK_END);
        if(sz <= 0){
            ret = -EINVAL;
            goto out;
        }
        firmware->size = sz;
    }else{
        dev_err(device, "[TSP] Firmware file open failed: %s\n", path);
        ret = fd;
    }
out:
    if(fd>=0) 
        sys_close(fd);
    set_fs(old_fs);            

    return ret;
} 

int mxt_load_firmware(struct device *dev, const char *fn, const char* path, int flag)
{
	int ret;
	int frame_size;
	unsigned int pos = 0;
	unsigned int retry;
	struct firmware *fw = NULL;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	if (MXT336S_FIRM_IN_KERNEL == flag) {
		fw = kzalloc(sizeof(struct firmware), GFP_KERNEL);
		if (NULL == fw) {
			dev_err(dev, "[TSP] failed to kzalloc for fw\n");
			return -ENOMEM;
		}
		fw->data = firmware_latest;
		fw->size = sizeof(firmware_latest);
	} else if (MXT336S_FIRM_EXTERNAL == flag) {
		ret = request_firmware((const struct firmware **)&fw, fn, dev);
		if (ret < 0) {
			dev_err(dev, "[TSP] Unable to open firmware %s\n", fn);
			return -ENOMEM;
		}
	} 
    else if (MXT336S_FIRM_EXTERNAL_NEW == flag) {
        ret = mxt_init_firmware_fs((const struct firmware **)&fw, fn, path, dev);
		if (ret < 0) {
			dev_err(dev, "[TSP] Unable to load firmware %s\n", path);
            release_mxt_firmware(fw);
			return -ENOMEM;
		}
    }
    else{
		dev_err(dev, "unknown firmware flag\n");
        return -EINVAL;
    }

	/* set resets into bootloader mode */
	ret = sw_reset_chip(mxt, RESET_TO_BOOTLOADER);
	if (ret < 0) {
		dev_warn(dev, "[TSP] couldn't reset by soft(%d), "
				"try to reset by h/w\n", ret);
		hw_reset_chip(mxt);
	}
	msleep(250);

	/* change to slave address of bootloader */
	mxt_change_i2c_addr(mxt, true);
	ret = check_bootloader(mxt, WAITING_BOOTLOAD_COMMAND);
	if (ret < 0) {
		dev_err(dev, "[TSP] ... Waiting bootloader command: Failed");
		goto err_fw;
	}

	/* unlock bootloader */
	unlock_bootloader(mxt->client);
	msleep(200);

	dev_info(dev, "Updating progress ( %d bytes)\n", fw->size);

	while (pos < fw->size) {
		retry = 0;

		ret = check_bootloader(mxt, WAITING_FRAME_DATA);
		if (ret < 0) {
			dev_err(dev, "... Waiting frame data: Failed\n");
			goto err_fw;
		}

        if (MXT336S_FIRM_EXTERNAL_NEW == flag) {
            frame_size = mxt_read_firmware_fs(fw, path, pos);
        }else{
            frame_size = ((*(fw->data + pos) << 8) | *(fw->data + pos + 1));
        }
        if(frame_size <= 0){
            dev_err("[TSP] Read firmware frame data failed : %d\n", frame_size);
            goto err_fw;
        }

		/* We should add 2 at frame size as the the firmware data is not
		* included the CRC bytes.
		*/
        if (MXT336S_FIRM_EXTERNAL_NEW != flag)
            frame_size += 2;

		if ((pos + frame_size) > fw->size) {
			dev_err(dev, "[TSP] invalid file, file size:%d, "
					"pos : %d, frame_size : %d\n",
					fw->size, pos, frame_size);
			ret = -EINVAL;
			goto err_fw;
		}

		/* write one frame to device */
try_to_resend_the_last_frame:
        if (MXT336S_FIRM_EXTERNAL_NEW == flag)
            i2c_master_send(mxt->client,(u8 *)(fw->data), frame_size);
        else
            i2c_master_send(mxt->client,(u8 *)(fw->data+pos), frame_size);

	#if defined(CONFIG_DISASSEMBLED_MONITOR)
		udelay(100);
	#endif

		ret = check_bootloader(mxt, FRAME_CRC_PASS);
		if (ret < 0) {
			if (++retry < 10) {
				/* recommendation from ATMEL */
				check_bootloader(mxt, WAITING_FRAME_DATA);
				dev_info(dev, "[TSP] We've got a "
						"FRAME_CRC_FAIL, so try again "
						"up to 10 times (count=%d)\n",
						retry);
				goto try_to_resend_the_last_frame;
			}
			dev_err(dev, "... CRC on the frame failed after "
					"10 trials!\n");
			goto err_fw;
		}

		pos += frame_size;

		dev_info(dev, "%zd / %zd (bytes) updated...", pos, fw->size);
        if (MXT336S_FIRM_EXTERNAL_NEW == flag){
            if(fw->data){
                kfree(fw->data);
                fw->data = NULL;
            }
        }
	}
	dev_info(dev, "\n[TSP] Updating firmware completed!\n");
	dev_info(dev, "[TSP] note: You may need to reset this target.\n");

err_fw:
	/* change to slave address of application */
	mxt_change_i2c_addr(mxt, false);
	if (MXT336S_FIRM_IN_KERNEL == flag) 
		kfree(fw);
	else if (MXT336S_FIRM_EXTERNAL == flag)
		release_firmware(fw);
    else if (MXT336S_FIRM_EXTERNAL_NEW == flag)
        release_mxt_firmware(fw);

	return ret;
}

int mxt_load_registers(struct mxt_data *mxt, const char *buf, int size)
{
	int ret = -1;
	char *dup = NULL;
	struct mxt_config_t mxt_cfg;
	int version;

	dup = kstrdup(buf, GFP_KERNEL);
	if (!dup) {
		dev_err(&mxt->client->dev, "failed to duplicate string\n");
		return ret;
	}

	version = mxt->device_info.major * 10 + mxt->device_info.minor;
	mxt_cfg.version = version;

	ret = request_parse(&mxt->client->dev,
			dup, &mxt_cfg, mxt->reg_update.verbose);
	if (ret == 0) {
		int error;
		u16 obj_addr = 0, obj_size = 0;
		u8 *data = NULL;

		data = (u8 *)&mxt_cfg.config;

		if (data && mxt_cfg.obj_info.object_num != 0) {
			obj_addr = get_object_address(
					mxt_cfg.obj_info.object_num,
					mxt_cfg.obj_info.instance_num,
					mxt->object_table,
					mxt->device_info.num_objs,
					mxt->object_link);
			obj_size = MXT_GET_SIZE(mxt_cfg.obj_info.object_num);
			error = mxt_write_block(mxt->client,
					obj_addr, obj_size, data);
			if (error < 0) {
				dev_err(&mxt->client->dev, "[TSP] "
						"mxt_write_block failed! "
						"(%s, %d)\n",
						__func__, __LINE__);
				ret = -1;
			}
		}
	}

	kfree(dup);
	return ret;
}
