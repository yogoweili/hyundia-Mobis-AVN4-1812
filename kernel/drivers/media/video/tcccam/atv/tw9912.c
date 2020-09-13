/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

               CAMERA    API    M O D U L E

                        EDIT HISTORY FOR MODULE

when        who       what, where, why
--------    ---       -------------------------------------------------------
10/xx/08  Telechips   Created file.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                           INCLUDE FILES FOR MODULE

===========================================================================*/

#include <linux/delay.h>
#include <mach/daudio.h>
#include <mach/daudio_debug.h>

#include "../tcc_cam_i2c.h"

#include "daudio_atv.h"
#include "tw9912.h"

#define PRV_W 720
#define PRV_H 240
#define CAP_W 720
#define CAP_H 240

static int debug_tw9912	= DEBUG_DAUDIO_ATV;

static int sensor_writeRegister(int reg, int val);
static int sensor_readRegister(int reg);

#define VPRINTK(fmt, args...) if (debug_tw9912) printk(KERN_CRIT TAG_DAUDIO_TW9912 fmt, ##args)

static struct sensor_reg TW9912_INIT[] = {
	//james.kang@zentech.co.kr 140923
	//cmd, value
	{ 0x01, 0x68 },
	{ 0x02, 0x44 },
	{ 0x03, 0x27 },
	{ 0x04, 0x00 },
	{ 0x05, 0x0D },
	{ 0x06, 0x03 },
	{ 0x07, 0x02 },
	{ 0x08, 0x16 },
	{ 0x09, 0xF0 },
	{ 0x0A, 0x10 },
	{ 0x0B, 0xD0 },
	{ 0x0C, 0xCC },
	{ 0x0D, 0x15 },
	{ 0x10, 0x00 },
	{ 0x11, 0x64 },
	{ 0x12, 0x11 },//GT 0X12 . SHARPNESS CONTROL REGISTER I (SHARPNESS)
	{ 0x13, 0x80 },//GT 0X13 . CHROMA (U) GAIN REGISTER (SAT_U)
	{ 0x14, 0x80 },//GT 0X14 . CHROMA (V) GAIN REGISTER (SAT_V)
	{ 0x15, 0x00 },
	{ 0x17, 0x30 },
	{ 0x18, 0x44 },
	{ 0x1A, 0x10 },
	{ 0x1B, 0x00 },
	{ 0x1C, 0x0F },
	{ 0x1D, 0x7F },
	{ 0x1E, 0x08 },
	{ 0x1F, 0x00 },
	{ 0x20, 0x50 },
	{ 0x21, 0x42 },
	{ 0x22, 0xF0 },
	{ 0x23, 0xD8 },
	{ 0x24, 0xBC },
	{ 0x25, 0xB8 },
	{ 0x26, 0x44 },
	{ 0x27, 0x38 },
	{ 0x28, 0x00 },
	{ 0x29, 0x00 },
	{ 0x2A, 0x78 },
	{ 0x2B, 0x44 },
	{ 0x2C, 0x30 },
	{ 0x2D, 0x14 },
	{ 0x2E, 0xA5 },
	{ 0x2F, 0x26 },
	{ 0x30, 0x00 },
	{ 0x31, 0x10 },
	{ 0x32, 0x00 },
	{ 0x33, 0x05 },
	{ 0x34, 0x1A },
	{ 0x35, 0x00 },
	{ 0x36, 0xE2 },
	{ 0x37, 0x12 },
	{ 0x38, 0x01 },
	{ 0x40, 0x00 },
	{ 0x41, 0x80 },
	{ 0x42, 0x00 },
	{ 0xC0, 0x01 },
	{ 0xC1, 0x07 },
	{ 0xC2, 0x01 },
	{ 0xC3, 0x03 },
	{ 0xC4, 0x5A },
	{ 0xC5, 0x00 },
	{ 0xC6, 0x20 },
	{ 0xC7, 0x04 },
	{ 0xC8, 0x00 },
	{ 0xC9, 0x06 },
	{ 0xCA, 0x06 },
	{ 0xCB, 0x30 },
	{ 0xCC, 0x00 },
	{ 0xCD, 0x54 },
	{ 0xD0, 0x00 },
	{ 0xD1, 0xF0 },
	{ 0xD2, 0xF0 },
	{ 0xD3, 0xF0 },
	{ 0xD4, 0x00 },
	{ 0xD5, 0x00 },
	{ 0xD6, 0x10 },
	{ 0xD7, 0x70 },
	{ 0xD8, 0x00 },
	{ 0xD9, 0x04 },
	{ 0xDA, 0x80 },
	{ 0xDB, 0x80 },
	{ 0xDC, 0x20 },
	{ 0xE0, 0x00 },
	{ 0xE1, 0x05 },
	{ 0xE2, 0xD9 },
	{ 0xE3, 0x00 },
	{ 0xE4, 0x00 },
	{ 0xE5, 0x00 },
	{ 0xE6, 0x00 },
	{ 0xE7, 0x2A },
	{ 0xE8, 0x0F },
	//{ 0xE9, 0x14 }, pclk falling
	{ 0xE9, 0x34 },	//pclk rising
	{ REG_TERM, VAL_TERM }
};

static struct sensor_reg TW9912_OPEN[] = {
	{ 0x03, 0x21 },
	{ REG_TERM, VAL_TERM }
};

static struct sensor_reg TW9912_OPEN_YIN0[] = {
	{ 0x02, 0x40 },
	{ 0x03, 0x21 },
	{ REG_TERM, VAL_TERM }
};

static struct sensor_reg TW9912_OPEN_YIN1[] = {
	{ 0x02, 0x44 },
	{ 0x03, 0x21 },
	{ REG_TERM, VAL_TERM }
};
//GT system start
static struct sensor_reg TW9912_OPEN_YIN2[] = {
	{ 0x02, 0x54 },
	{ 0x03, 0x21 },
	{ 0x06, 0x00 },
	{ REG_TERM, VAL_TERM }
};
//GT system end

struct sensor_reg *TW9912_OPEN_YIN[] = {
	TW9912_OPEN_YIN0,
	TW9912_OPEN_YIN1,
	TW9912_OPEN_YIN2

};

static struct sensor_reg TW9912_CLOSE[] = {
	{ 0x03, 0x27 },
	{ REG_TERM, VAL_TERM }
};

struct sensor_reg *tw9912_sensor_reg_common[] = {
	TW9912_INIT,
	TW9912_OPEN,
	TW9912_CLOSE,
};

static struct sensor_reg sensor_brightness_p0[] = {
	{ REG_TERM, VAL_TERM }
};

struct sensor_reg *tw9912_sensor_reg_brightness[] = {
	sensor_brightness_p0,
};

static struct sensor_reg sensor_contrast_p0[] = {
	{ REG_TERM, VAL_TERM }
};

struct sensor_reg *tw9912_sensor_reg_contrast[] = {
	sensor_contrast_p0,
};

static struct sensor_reg sensor_saturation_p0[] = {
	{ REG_TERM, VAL_TERM }
};

//GT system start
static struct sensor_reg sensor_u_gain_p0[] = {
	{ 0x12, 0x11 },
	{ REG_TERM, VAL_TERM }
};
static struct sensor_reg sensor_v_gain_p0[] = {
	{ 0x13, 0x80 },
	{ REG_TERM, VAL_TERM }
};
static struct sensor_reg sensor_sharpness_p0[] = {
	{ 0x14, 0x80 },
	{ REG_TERM, VAL_TERM }
};
//GT system end

struct sensor_reg *tw9912_sensor_reg_saturation[] = {
	sensor_saturation_p0,
};

//GT system start
struct sensor_reg *tw9912_sensor_reg_u_gain[] = {
	sensor_u_gain_p0,
};
struct sensor_reg *tw9912_sensor_reg_v_gain[] = {
	sensor_v_gain_p0,
};
struct sensor_reg *tw9912_sensor_reg_sharpness[] = {
	sensor_sharpness_p0,
};
//GT system end

static struct sensor_reg sensor_hue_p0[] = {
	{ REG_TERM, VAL_TERM }
};

struct sensor_reg *tw9912_sensor_reg_hue[] = {
	sensor_hue_p0,
};

static struct sensor_reg tw9912_sensor_reg_backup[] = {
	//james.kang@zentech.co.kr 140923
	//cmd, value
	{ 0x01, 0x68 },
	{ 0x02, 0x44 },
	{ 0x03, 0x27 },
	{ 0x04, 0x00 },
	{ 0x05, 0x0D },
	{ 0x06, 0x03 },
	{ 0x07, 0x02 },
	{ 0x08, 0x16 },
	{ 0x09, 0xF0 },
	{ 0x0A, 0x10 },
	{ 0x0B, 0xD0 },
	{ 0x0C, 0xCC },
	{ 0x0D, 0x15 },
	{ 0x10, 0x00 },
	{ 0x11, 0x64 },
	{ 0x12, 0x11 },//GT 0X12 . SHARPNESS CONTROL REGISTER I (SHARPNESS)
	{ 0x13, 0x80 },//GT 0X13 . CHROMA (U) GAIN REGISTER (SAT_U)
	{ 0x14, 0x80 },//GT 0X14 . CHROMA (V) GAIN REGISTER (SAT_V)
	{ 0x15, 0x00 },
	{ 0x17, 0x30 },
	{ 0x18, 0x44 },
	{ 0x1A, 0x10 },
	{ 0x1B, 0x00 },
	{ 0x1C, 0x0F },
	{ 0x1D, 0x7F },
	{ 0x1E, 0x08 },
	{ 0x1F, 0x00 },
	{ 0x20, 0x50 },
	{ 0x21, 0x42 },
	{ 0x22, 0xF0 },
	{ 0x23, 0xD8 },
	{ 0x24, 0xBC },
	{ 0x25, 0xB8 },
	{ 0x26, 0x44 },
	{ 0x27, 0x38 },
	{ 0x28, 0x00 },
	{ 0x29, 0x00 },
	{ 0x2A, 0x78 },
	{ 0x2B, 0x44 },
	{ 0x2C, 0x30 },
	{ 0x2D, 0x14 },
	{ 0x2E, 0xA5 },
	{ 0x2F, 0x26 },
	{ 0x30, 0x00 },
	{ 0x31, 0x10 },
	{ 0x32, 0x00 },
	{ 0x33, 0x05 },
	{ 0x34, 0x1A },
	{ 0x35, 0x00 },
	{ 0x36, 0xE2 },
	{ 0x37, 0x12 },
	{ 0x38, 0x01 },
	{ 0x40, 0x00 },
	{ 0x41, 0x80 },
	{ 0x42, 0x00 },
	{ 0xC0, 0x01 },
	{ 0xC1, 0x07 },
	{ 0xC2, 0x01 },
	{ 0xC3, 0x03 },
	{ 0xC4, 0x5A },
	{ 0xC5, 0x00 },
	{ 0xC6, 0x20 },
	{ 0xC7, 0x04 },
	{ 0xC8, 0x00 },
	{ 0xC9, 0x06 },
	{ 0xCA, 0x06 },
	{ 0xCB, 0x30 },
	{ 0xCC, 0x00 },
	{ 0xCD, 0x54 },
	{ 0xD0, 0x00 },
	{ 0xD1, 0xF0 },
	{ 0xD2, 0xF0 },
	{ 0xD3, 0xF0 },
	{ 0xD4, 0x00 },
	{ 0xD5, 0x00 },
	{ 0xD6, 0x10 },
	{ 0xD7, 0x70 },
	{ 0xD8, 0x00 },
	{ 0xD9, 0x04 },
	{ 0xDA, 0x80 },
	{ 0xDB, 0x80 },
	{ 0xDC, 0x20 },
	{ 0xE0, 0x00 },
	{ 0xE1, 0x05 },
	{ 0xE2, 0xD9 },
	{ 0xE3, 0x00 },
	{ 0xE4, 0x00 },
	{ 0xE5, 0x00 },
	{ 0xE6, 0x00 },
	{ 0xE7, 0x2A },
	{ 0xE8, 0x0F },
	//{ 0xE9, 0x14 }, pclk falling
	{ 0xE9, 0x14 },	//pclk rising
	{ REG_TERM, VAL_TERM }
};

static struct sensor_reg tw9912_sensor_path_rear_camera[] = {
	{ 0x07, 0x02 },
	{ 0x09, 0xf0 },
	{ 0x1c, 0x08 },
	{ 0x1d, 0x80 },
	{ REG_TERM, VAL_TERM }
};

static struct sensor_reg tw9912_sensor_path_aux_ntsc[] = {
	{ 0x07, 0x02 },
	{ 0x09, 0xf0 },
	{ 0x1c, 0x08 },
	{ 0x1d, 0x80 },
	{ REG_TERM, VAL_TERM }
};

static struct sensor_reg tw9912_sensor_path_aux_pal[] = {
	{ 0x07, 0x12 },
	{ 0x09, 0x20 },
	{ 0x1c, 0x09 },
	{ 0x1d, 0x80 },
	{ REG_TERM, VAL_TERM }
};

struct sensor_reg *tw9912_sensor_reg_path[] = {
	tw9912_sensor_path_rear_camera,
	tw9912_sensor_path_aux_ntsc,
	tw9912_sensor_path_aux_pal
};

int tw9912_read_ie(int cmd, char *level);

static void tw9912_sensor_get_preview_size(int *width, int *height)
{
	*width = PRV_W;
	*height = PRV_H;
}

static void tw9912_sensor_get_capture_size(int *width, int *height)
{
	*width = CAP_W;
	*height = CAP_H;
}

static int write_regs(const struct sensor_reg reglist[])
{
	int err, err_cnt = 0, backup_cnt;
	unsigned char data[132];
	unsigned char bytes;
	const struct sensor_reg *next = reglist;

	while (!((next->reg == REG_TERM) && (next->val == VAL_TERM))) {
		bytes = 0;
		data[bytes]= (u8)next->reg & 0xff; 	bytes++;
		data[bytes]= (u8)next->val & 0xff; 	bytes++;

		err = DDI_I2C_Write(data, 1, 1);
		if (err) {
			err_cnt++;
			if(err_cnt >= 3) {
				printk(KERN_ERR "ERROR: Sensor I2C !!!! \n"); 
				return err;
			}
		} else {
			for (backup_cnt = 0; backup_cnt < ARRAY_SIZE(tw9912_sensor_reg_backup); backup_cnt++) {
				if (tw9912_sensor_reg_backup[backup_cnt].reg == next->reg) {
					tw9912_sensor_reg_backup[backup_cnt].val = next->val;
				}
			}

			err_cnt = 0;
			next++;
		}
	}

	return 0;
}

static int sensor_writeRegister(int reg, int val)
{
	int ret;
	unsigned char data[2];

	data[0]= (u8)reg & 0xff;
	data[1]= (u8)val & 0xff;
	ret = DDI_I2C_Write(data, 1, 1);

	return ret;
}

static int sensor_readRegister(int reg)
{
	char ret;
	DDI_I2C_Read((unsigned short)reg, 1, &ret, 1);
	return ret;
}

static int tw9912_read(int reg)
{
	int i = I2C_RETRY_COUNT;
	unsigned char ret;
	do {
		int retval = DDI_I2C_Read((unsigned short)reg, 1, &ret, 1);
		if (retval == 0)
		{
			//success to I2C.
			return ret;
		}
		i--;
	} while(i > 0);

	return FAIL_I2C;
}

static int tw9912_write(unsigned char* data, unsigned char reg_bytes, unsigned char data_bytes)
{
	int i = I2C_RETRY_COUNT;
	do {
		int ret = DDI_I2C_Write(data, reg_bytes, data_bytes);
		if (ret == 0)
		{
			//success to I2C.
			return SUCCESS_I2C;
		}
		i--;
	} while(i > 0);

	return FAIL_I2C;
}

static int tw9912_sensor_open(eTW_YSEL yin)
{
	VPRINTK("%s\n", __func__);

#if 0
    {
        struct sensor_reg *p=TW9912_OPEN_YIN[yin];

        printk(KERN_INFO"%s[%d]\n", __func__, yin);
        for(; REG_TERM!=p->reg && VAL_TERM!=p->val; ++p)
        {
            printk(KERN_INFO"sensor_reg=%02x %02x\n", p->reg, p->val);
        }
    }
#endif

    return write_regs(TW9912_OPEN_YIN[yin]);
}

static int tw9912_sensor_close(void)
{
	VPRINTK("%s\n", __func__);
	return write_regs(tw9912_sensor_reg_common[MODE_TW9912_CLOSE]);
}

int tw9912_read_id(void)
{
	return tw9912_read(TW9912_REG_ID);
}

static int tw9912_sensor_preview(void)
{
	if (debug_tw9912)	//for debug
	{
		int id = tw9912_read_id();
		VPRINTK("%s read id: 0x%x\n", __func__, id);
	}

	return 0;
}

static int tw9912_sensor_resetVideoDecoder(void)
{
	VPRINTK("%s\n", __func__);

	sensor_reset_low();
	sensor_delay(10);

	sensor_reset_high();
	sensor_delay(10);

	return write_regs(tw9912_sensor_reg_backup);
}

void tw9912_close_cam(void)
{
	VPRINTK("%s\n", __func__);

	tw9912_sensor_close();
}

int tw9912_init(void)
{
	VPRINTK("%s\n", __func__);

	return 0;
}

int tw9912_check_video_signal(unsigned long arg)
{
	int ret = 0, cstatus = 0;
	tw_cstatus *tw9912_cstatus;

	tw9912_cstatus = (tw_cstatus *)arg;
	cstatus = tw9912_read(TW9912_REG_CSTATUS);
	if (cstatus >= FAIL)
		//error
		return cstatus;

	tw9912_cstatus->vdloss = (char)cstatus & VDLOSS ? 1 : 0;
	tw9912_cstatus->hlock = (char)cstatus & HLOCK ? 1 : 0;
	tw9912_cstatus->slock = (char)cstatus & VLOCK ? 1 : 0;
	tw9912_cstatus->vlock = (char)cstatus & SLOCK ? 1 : 0;
	tw9912_cstatus->mono = (char)cstatus & MONO ? 1 : 0;
	ret = 1;

	printk("[%s] vdloss : 0x%x, hlock : 0x%x, slock : 0x%x, vlock : 0x%x, mono : 0x%x\n", __func__, tw9912_cstatus->vdloss, tw9912_cstatus->hlock, tw9912_cstatus->slock, tw9912_cstatus->vlock, tw9912_cstatus->mono);

	return ret;
}

int tw9912_display_mute(int mute)
{
	printk(KERN_INFO "%s TW9912 not support display mute.\n", __func__);

	return FAIL;
}

int tw9912_write_ie(int cmd, unsigned char value)
{
	int ret = FAIL;
	unsigned char data[2] = {0x0, };
	printk(KERN_CRIT "%s cmd: %d value: 0x%x\n", __func__, cmd, value);

	if (cmd < SET_CAM_BRIGHTNESS || cmd > SET_AUX_SATURATION)
	{
		printk(KERN_CRIT "%s cmd: cmd < SET_CAM_BRIGHTNESS || cmd > SET_AUX_SATURATION return ~~\n", __func__);
		return ret;
	}	
	switch (cmd)
	{
		case SET_AUX_BRIGHTNESS:
		case SET_CAM_BRIGHTNESS:
			data[0] = TW9912_I2C_BRIGHTNESS;
			break;
		case SET_AUX_CONTRAST:
		case SET_CAM_CONTRAST:
			data[0] = TW9912_I2C_CONTRAST;
			break;
		case SET_AUX_GAMMA:
		case SET_CAM_HUE:
			data[0] = TW9912_I2C_HUE;
			break;

	/*	case SET_CAM_SHARPNESS:
			data[0] = TW9912_I2C_SHARPNESS;
			
			break;

		case SET_CAM_U_GAIN:
			data[0] = TW9912_I2C_SATURATION_U;
			break;
		case SET_CAM_V_GAIN:
			data[0] = TW9912_I2C_SATURATION_V;
			break;
	*/
		case SET_AUX_SATURATION:
		case SET_CAM_SATURATION:
			data[0] = TW9912_I2C_SATURATION_U;
			break;
	}
	
	/*if(cmd == SET_CAM_SHARPNESS)
	{	
		int val=0;
		char lev;
		val = tw9912_read_ie( GET_CAM_SHARPNESS , &lev);
		printk(KERN_CRIT "[GT system] %s, set cam sharpness resd =%d \n", __func__, lev);
		if(val != SUCCESS)
			return ret ;
		data[1] = lev ;       // 4~7 bit setting
		data[1] = 0x0f&value; // SHARPNESS 0~3 bit
	}
	else*/
	{
		data[1] = value;
		printk(KERN_CRIT "[GT system] %s value = %d\n", __func__, value);
	}

	if ((cmd == SET_CAM_BRIGHTNESS || cmd == SET_CAM_HUE) ||
		(cmd == SET_AUX_BRIGHTNESS || cmd == SET_AUX_GAMMA)   )
		data[1] -= 128;

	ret = tw9912_write(data, 1, 1);

	printk(KERN_CRIT "[GT system] %s , data[1]= %d,  ret = %d\n", __func__,data[1], ret );
	if( (ret == SUCCESS_I2C && cmd == SET_CAM_SATURATION) ||
		(ret == SUCCESS_I2C && cmd == SET_AUX_SATURATION) )
	{
		data[0] = TW9912_I2C_SATURATION_V;
		ret = tw9912_write(data, 1, 1);
	}

	if (ret >= FAIL)
		ret = FAIL;
	else
		ret = SUCCESS;

	return ret;
}

int tw9912_read_ie(int cmd, char *level)
{
	int ret = FAIL;
	int value_u = 0, value_v = 0;
	//VPRINTK("%s cmd: %d\n", __func__, cmd);

	switch (cmd)
	{
		case GET_CAM_BRIGHTNESS:
		case GET_AUX_BRIGHTNESS:
			ret = tw9912_read(TW9912_I2C_BRIGHTNESS);
			break;

		case GET_CAM_CONTRAST:
		case GET_AUX_CONTRAST:
			ret = tw9912_read(TW9912_I2C_CONTRAST);
			break;

		case GET_CAM_HUE:
		case GET_AUX_GAMMA:
			ret = tw9912_read(TW9912_I2C_HUE);
			break;

		case GET_CAM_SATURATION:
		case GET_AUX_SATURATION:
			value_u = tw9912_read(TW9912_I2C_SATURATION_U);
			//printk(KERN_CRIT "[GT system] %s , value_u= %d \n", __func__ , value_u);
			value_v = tw9912_read(TW9912_I2C_SATURATION_V);
			//printk(KERN_CRIT "[GT system] %s , value_v= %d \n", __func__ , value_v);

			if (value_u == FAIL_I2C || value_v == FAIL_I2C)
				ret = FAIL_I2C;
			else
				ret = value_u;
			break;
		/*
		case GET_CAM_U_GAIN:
			ret = tw9912_read(TW9912_I2C_SATURATION_U);
			printk(KERN_CRIT "[GT system] %s , value_u= %d \n", __func__ , ret);
			break;			
		case GET_CAM_V_GAIN:
			ret = tw9912_read(TW9912_I2C_SATURATION_V);
			printk(KERN_CRIT "[GT system] %s , value_v= %d \n", __func__ , ret);
			break;
		case GET_CAM_SHARPNESS:
			ret = tw9912_read(TW9912_I2C_SHARPNESS);
			break;*/
		default:
			break;
			
	}

	if (ret >= FAIL)
	{
		ret = FAIL;
		*level = 0;
	}
	else
	{
		*level = ret;
		if ((cmd == GET_CAM_BRIGHTNESS || cmd == GET_CAM_HUE) ||
			(cmd == GET_AUX_BRIGHTNESS || cmd == GET_AUX_GAMMA))
			*level += 128;

		ret = SUCCESS;
	}
	//printk(KERN_CRIT "[GT system] %s , ret=%d : level= %d \n", __func__ , ret ,*level);

	return ret;
}

static int tw9912_sensor_capture(void)
{
	return 0;
}

static int tw9912_sensor_capturecfg(int width, int height)
{
	return 0;
}

static int sensor_zoom(int val)
{
	return 0;
}

static int sensor_autofocus(int val)
{
	return 0;
}

static int sensor_effect(int val)
{
	return 0;
}

static int sensor_flip(int val)
{
	return 0;
}

static int sensor_iso(int val)
{
	return 0;
}

static int sensor_me(int val)
{
	return 0;
}

static int sensor_wb(int val)
{
	return 0;
}

static int sensor_bright(int val)
{
	return write_regs(tw9912_sensor_reg_brightness[val]);
}

static int sensor_scene(int val)
{
	return 0;
}

static int sensor_contrast(int val)
{
	return write_regs(tw9912_sensor_reg_contrast[val]);
}

static int sensor_saturation(int val)
{
	return write_regs(tw9912_sensor_reg_saturation[val]);
}

//GT system start
static int sensor_u_gain(int val)
{
	return write_regs(tw9912_sensor_reg_u_gain[val]);
}
static int sensor_v_gain(int val)
{
	return write_regs(tw9912_sensor_reg_v_gain[val]);
}
static int sensor_sharpness(int val)
{
	return write_regs(tw9912_sensor_reg_sharpness[val]);
}
//GT system end

static int sensor_hue(int val)
{
	return write_regs(tw9912_sensor_reg_hue[val]);
}

static int sensor_getVideo(void)
{
	char ret;

	DDI_I2C_Read(0x01, 1, &ret, 1);
	if ((ret & 0x80) == 0x00)  {
		ret = 0; // detected video signal
	} else {
		ret = 1; // not detected video signal
	}
	
	return ret;
}

static int sensor_checkNoise(void)
{
	char ret;

	DDI_I2C_Read(0x01, 1, &ret, 1);
	if ((ret & 0xE8) == 0x68)  {
		ret = 0; // not detected video noise
	} else {
		ret = 1; // detected video noise
	}
	
	return ret;
}

static int tw9912_sensor_setPath(int val)
{
#if 0
    {
        struct sensor_reg *p=tw9912_sensor_reg_path[val];

        printk(KERN_INFO"%s [%d]\n", __func__, val);
        for(; REG_TERM!=p->reg && VAL_TERM!=p->val; ++p)
        {
            printk(KERN_INFO"sensor_reg=%02x %02x\n", p->reg, p->val);
        }
    }
#endif

	return write_regs(tw9912_sensor_reg_path[val]);
}

static int tw9912_sensor_getVideoFormat(void)
{
	char ret = 0;
	//TODO

	return ret;
}

static int tw9912_sensor_check_esd(int val)
{
	return 0;
}

static int tw9912_sensor_check_luma(int val)
{
	return 0;
}

void tw9912_sensor_init_fnc(SENSOR_FUNC_TYPE_DAUDIO *sensor_func)
{
	sensor_func->sensor_open				= tw9912_sensor_open;
	sensor_func->sensor_close				= tw9912_sensor_close;

	sensor_func->sensor_preview				= tw9912_sensor_preview;
	sensor_func->sensor_capture				= tw9912_sensor_capture;
	sensor_func->sensor_capturecfg			= tw9912_sensor_capturecfg;
	sensor_func->sensor_reset_video_decoder	= tw9912_sensor_resetVideoDecoder;
	sensor_func->sensor_init				= tw9912_init;
	sensor_func->sensor_read_id				= tw9912_read_id;
	sensor_func->sensor_check_video_signal	= tw9912_check_video_signal;
	sensor_func->sensor_display_mute		= tw9912_display_mute;
	sensor_func->sensor_close_cam			= tw9912_close_cam;
	sensor_func->sensor_write_ie			= tw9912_write_ie;
	sensor_func->sensor_read_ie				= tw9912_read_ie;
	sensor_func->sensor_get_preview_size	= tw9912_sensor_get_preview_size;
	sensor_func->sensor_get_capture_size	= tw9912_sensor_get_capture_size;

	sensor_func->Set_Zoom					= sensor_zoom;
	sensor_func->Set_AF						= sensor_autofocus;
	sensor_func->Set_Effect					= sensor_effect;
	sensor_func->Set_Flip					= sensor_flip;
	sensor_func->Set_ISO					= sensor_iso;
	sensor_func->Set_ME						= sensor_me;
	sensor_func->Set_WB						= sensor_wb;
	sensor_func->Set_Bright					= sensor_bright;
	sensor_func->Set_Scene					= sensor_scene;
	sensor_func->Set_Contrast				= sensor_contrast;
	sensor_func->Set_Saturation				= sensor_saturation;

	sensor_func->Set_UGain					= sensor_u_gain; //GT system start
	sensor_func->Set_VGain					= sensor_v_gain;
	sensor_func->Set_Sharpness				= sensor_sharpness; //GT system end

	sensor_func->Set_Hue					= sensor_hue;
	sensor_func->Get_Video					= sensor_getVideo;
	sensor_func->Check_Noise				= sensor_checkNoise;

	sensor_func->Set_Path					= tw9912_sensor_setPath;
	sensor_func->Get_videoFormat			= tw9912_sensor_getVideoFormat;
	sensor_func->Set_writeRegister			= sensor_writeRegister;
	sensor_func->Get_readRegister			= sensor_readRegister;

	sensor_func->Check_ESD					= NULL;
	sensor_func->Check_Luma					= NULL;
}
