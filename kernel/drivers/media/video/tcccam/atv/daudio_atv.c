#include <asm/io.h>
#include <asm/system.h>

#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <mach/daudio.h>
#include <mach/daudio_debug.h>
#include <mach/daudio_info.h>
#include <mach/daudio_pinctl.h>
#include <mach/hardware.h>

#include "../sensor_if.h"
#include "../cam.h"
#include "../tcc_cam_i2c.h"
#include "../tcc_cam.h"

#include "tw8836.h"
#include "tw9912.h"
#include "daudio_cmmb.h"

static int debug_daudio_atv = DEBUG_DAUDIO_ATV;
#define VPRINTK(fmt, args...) if (debug_daudio_atv) printk(KERN_INFO TAG_DAUDIO_ATV fmt, ##args)

static SENSOR_FUNC_TYPE_DAUDIO sf_daudio[SENSOR_MAX];

extern struct TCCxxxCIF hardware_data;

struct capture_size sensor_sizes[] = {
	{720, 480}, // NTSC
	{720, 576}  // PAL
};

static int sensor_readRegister(int reg)
{
	char ret;
	DDI_I2C_Read((unsigned short)reg, 1, &ret, 1);

	return ret;
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

static int sensor_open(void)
{
	int what = 0;
	eTW_YSEL yin=TW_YIN1;

	VPRINTK("%s\n", __func__);
	switch (daudio_main_version())
	{
		case DAUDIO_BOARD_6TH:
		case DAUDIO_BOARD_5TH:
		default:
#if defined(FEATURE_SUPPORT_ATV_PROGRESSIVE)
			what = SENSOR_TW8836;
#else
			if (hardware_data.cam_info == DAUDIO_CAMERA_REAR)
			{
				unsigned int gpioF26=0,gpioF27=0;
				unsigned int value=0;

				what = SENSOR_TW9912;
				//GT system start 
				gpioF26 =  gpio_get_value(TCC_GPF(26));
				gpioF27 =  gpio_get_value(TCC_GPF(27));	
				value = ((gpioF26<<1)|gpioF27);
				if( value == 0 )
				{
					yin = TW_YIN1;
				}
				else
				{
					yin = TW_YIN2;
				}
				//GT system end
			}
			else if( DAUDIO_CAMERA_CMMB==hardware_data.cam_info )
			{
				what = SENSOR_CMMB;
			}
			else if( DAUDIO_CAMERA_AUX==hardware_data.cam_info )
			{
				what=SENSOR_TW9912;
				yin=TW_YIN0;
			}
#endif
			break;
	}

	if (sf_daudio[what].sensor_open != NULL)
		return sf_daudio[what].sensor_open(yin);
	else
		return FAIL;
}

static int sensor_close(void)
{
	int what = 0;
	VPRINTK("%s\n", __func__);
	switch (daudio_main_version())
	{
		case DAUDIO_BOARD_6TH:
		case DAUDIO_BOARD_5TH:
		default:
#if defined(FEATURE_SUPPORT_ATV_PROGRESSIVE)
			what = SENSOR_TW8836;
#else
			if (hardware_data.cam_info == DAUDIO_CAMERA_CMMB)
				what = SENSOR_CMMB;
			else
				what = SENSOR_TW9912;
#endif
			break;
	}

	if (sf_daudio[what].sensor_close != NULL)
		return sf_daudio[what].sensor_close();
	else
		return FAIL;
}

static int sensor_capture(void)
{
	int what = 0;
	VPRINTK("%s\n", __func__);
	switch (daudio_main_version())
	{
		case DAUDIO_BOARD_6TH:
		case DAUDIO_BOARD_5TH:
		default:
#if defined(FEATURE_SUPPORT_ATV_PROGRESSIVE)
			what = SENSOR_TW8836;
#else
			if (hardware_data.cam_info == DAUDIO_CAMERA_CMMB)
				what = SENSOR_CMMB;
			else
				what = SENSOR_TW9912;
#endif
			break;
	}

	if (sf_daudio[what].sensor_capture != NULL)
		return sf_daudio[what].sensor_capture();
	else
		return FAIL;
}

static int sensor_capturecfg(int width, int height)
{
	int what = 0;
	VPRINTK("%s\n", __func__);
	switch (daudio_main_version())
	{
		case DAUDIO_BOARD_6TH:
		case DAUDIO_BOARD_5TH:
		default:
#if defined(FEATURE_SUPPORT_ATV_PROGRESSIVE)
			what = SENSOR_TW8836;
#else
			if (hardware_data.cam_info == DAUDIO_CAMERA_CMMB)
				what = SENSOR_CMMB;
			else
				what = SENSOR_TW9912;
#endif
			break;
	}

	if (sf_daudio[what].sensor_capturecfg != NULL)
		return sf_daudio[what].sensor_capturecfg(width, height);
	else
		return FAIL;
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
	return 0;
}

static int sensor_scene(int val)
{
	return 0;
}

static int sensor_contrast(int val)
{
	return 0;
}

static int sensor_saturation(int val)
{
	return 0;
}

static int sensor_hue(int val)
{
	return 0;
}

static int sensor_getVideo(void)
{
	return 0;
}

static int sensor_checkNoise(void)
{
	return 0;
}

static int sensor_resetVideoDecoder(void)
{
	int what = 0;
	VPRINTK("%s\n", __func__);
	switch (daudio_main_version())
	{
		case DAUDIO_BOARD_6TH:
		case DAUDIO_BOARD_5TH:
		default:
#if defined(FEATURE_SUPPORT_ATV_PROGRESSIVE)
			what = SENSOR_TW8836;
#else
			if (hardware_data.cam_info == DAUDIO_CAMERA_CMMB)
				what = SENSOR_CMMB;
			else
				what = SENSOR_TW9912;
#endif
			break;
	}

	if (sf_daudio[what].sensor_reset_video_decoder != NULL)
		return sf_daudio[what].sensor_reset_video_decoder();
	else
		return FAIL;
}

static int sensor_setPath(int val)
{
	int (*pfn)(int);

	switch( daudio_main_version() )
	{
		case DAUDIO_BOARD_6TH:
		case DAUDIO_BOARD_5TH: 
		default:
#if defined(FEATURE_SUPPORT_ATV_PROGRESSIVE)
			pfn=sf_daudio[SENSOR_TW8836].Set_Path;
#else
			if( DAUDIO_CAMERA_CMMB==hardware_data.cam_info )
			{
				pfn=sf_daudio[SENSOR_CMMB].Set_Path;
			}
			else
			{
				pfn=sf_daudio[SENSOR_TW9912].Set_Path;
			}
#endif
			break;
	}

	switch( val )
	{
        case VIDIOC_USER_SET_PATH_AUX_NTSC:
			val=1;
			break;

		case VIDIOC_USER_SET_PATH_AUX_PAL:
			val=2;
			break;

		case VIDIOC_USER_SET_PATH_REAR_CAMERA:
		default:
			val=0;
			break;
	}

	if( pfn )
	{
		return pfn(val);
	}

	return FAIL;
}

static int sensor_getVideoFormat(void)
{
	return 0;
}

static int sensor_check_esd(int val)
{
	return 0;
}

static int sensor_check_luma(int val)
{
	return 0;
}

static int sensor_preview(void)
{
	int what = 0;
	VPRINTK("%s\n", __func__);
	switch (daudio_main_version())
	{
		case DAUDIO_BOARD_6TH:
		case DAUDIO_BOARD_5TH:
		default:
#if defined(FEATURE_SUPPORT_ATV_PROGRESSIVE)
			what = SENSOR_TW8836;
#else
			if (hardware_data.cam_info == DAUDIO_CAMERA_CMMB)
				what = SENSOR_CMMB;
			else
				what = SENSOR_TW9912;
#endif
			break;
	}

	if (sf_daudio[what].sensor_preview != NULL)
		return sf_daudio[what].sensor_preview();
	else
		return FAIL;
}

int datv_write_ie(int cmd, unsigned char value)
{
	int what = 0;
	VPRINTK("%s\n", __func__);
	switch (daudio_main_version())
	{
		case DAUDIO_BOARD_6TH:
		case DAUDIO_BOARD_5TH:
		default:
			if (hardware_data.cam_info == DAUDIO_CAMERA_CMMB)
				what = SENSOR_CMMB;
			else
				what = SENSOR_TW9912;
			break;
	}

	if (sf_daudio[what].sensor_write_ie != NULL)
		return sf_daudio[what].sensor_write_ie(cmd, value);
	else
		return FAIL;
}

int datv_read_ie(int cmd, char *level)
{
	int what = 0;
	VPRINTK("%s\n", __func__);
	switch (daudio_main_version())
	{
		case DAUDIO_BOARD_6TH:
		case DAUDIO_BOARD_5TH:
		default:
			if (hardware_data.cam_info == DAUDIO_CAMERA_CMMB)
				what = SENSOR_CMMB;
			else
				what = SENSOR_TW9912;
			break;	
	}

	if (sf_daudio[what].sensor_read_ie != NULL)
		return sf_daudio[what].sensor_read_ie(cmd, level);
	else
		return FAIL;
}

int datv_init(void)
{
	int what = 0;
	int ver = daudio_main_version();
	VPRINTK("%s mainboard ver: %d\n", __func__, ver);

	tw9912_sensor_init_fnc(&sf_daudio[SENSOR_TW9912]);
	tw8836_sensor_init_fnc(&sf_daudio[SENSOR_TW8836]);
	daudio_cmmb_sensor_init_fnc(&sf_daudio[SENSOR_CMMB]);

	switch (ver)
	{
		case DAUDIO_BOARD_6TH:
		case DAUDIO_BOARD_5TH:
		default:
			if (hardware_data.cam_info == DAUDIO_CAMERA_CMMB)
				what = SENSOR_CMMB;
			else
				what = SENSOR_TW9912;
			break;
	}

	if (sf_daudio[what].sensor_init != NULL)
		return sf_daudio[what].sensor_init();
	else
		return FAIL;
}

int datv_check_video_signal(unsigned long arg)
{
	int what = 0;
	VPRINTK("%s ver: %d ,  arg = %d \n", __func__, daudio_main_version(), arg);
	switch (daudio_main_version())
	{
		case DAUDIO_BOARD_6TH:
		case DAUDIO_BOARD_5TH:
		default:
#if defined(FEATURE_SUPPORT_ATV_PROGRESSIVE)
			what = SENSOR_TW8836;
#else
			if (hardware_data.cam_info == DAUDIO_CAMERA_CMMB)
				what = SENSOR_CMMB;
			else
				what = SENSOR_TW9912;
#endif
			break;
	}

	if (sf_daudio[what].sensor_check_video_signal != NULL)
		return sf_daudio[what].sensor_check_video_signal(arg);
	else
		return FAIL;
}

int datv_display_mute(int mute)
{
	int what = 0;

	VPRINTK("%s ver: %d\n", __func__, daudio_main_version());
	switch (daudio_main_version())
	{
		case DAUDIO_BOARD_6TH:
		case DAUDIO_BOARD_5TH:
		default:
#if defined(FEATURE_SUPPORT_ATV_PROGRESSIVE)
			what = SENSOR_TW8836;
#else
			if (hardware_data.cam_info == DAUDIO_CAMERA_CMMB)
				what = SENSOR_CMMB;
			else
				what = SENSOR_TW9912;
#endif
			break;
	}

	if (sf_daudio[what].sensor_display_mute != NULL)
		return sf_daudio[what].sensor_display_mute(mute);
	else
		return FAIL;
}

void datv_close_cam(void)
{
	int ver = daudio_main_version();
	int what = 0;

	VPRINTK("%s ver: %d\n", __func__, ver);
	switch (daudio_main_version())
	{
		case DAUDIO_BOARD_6TH:
		case DAUDIO_BOARD_5TH:
		default:
#if defined(FEATURE_SUPPORT_ATV_PROGRESSIVE)
			what = SENSOR_TW8836;
#else
			if (hardware_data.cam_info == DAUDIO_CAMERA_CMMB)
				what = SENSOR_CMMB;
			else
				what = SENSOR_TW9912;
#endif
			break;
	}

	if (sf_daudio[what].sensor_close_cam != NULL)
		sf_daudio[what].sensor_close_cam();
}

int datv_get_reset_pin(void)
{
	int ver = daudio_main_version();

	VPRINTK("%s ver: %d\n", __func__, ver);
	switch (daudio_main_version())
	{
		case DAUDIO_BOARD_6TH:
		case DAUDIO_BOARD_5TH:
		default:
			if (hardware_data.cam_info == DAUDIO_CAMERA_CMMB)
				return get_gpio_number(CTL_XM_SIRIUS_RESET);	// CMMB reset
			else
				return get_gpio_number(CTL_TW9912_RESET);
	}

	return FAIL;
}

void datv_get_preview_size(int *width, int *height)
{
	int ver = daudio_main_version();
	int what = 0;

	VPRINTK("%s ver: %d\n", __func__, ver);
	switch (daudio_main_version())
	{
		case DAUDIO_BOARD_6TH:
		case DAUDIO_BOARD_5TH:
		default:
			if (hardware_data.cam_info == DAUDIO_CAMERA_CMMB)
				what = SENSOR_CMMB;
			else
				what = SENSOR_TW9912;
			break;
	}

	if (sf_daudio[what].sensor_get_preview_size != NULL)
		sf_daudio[what].sensor_get_preview_size(width, height);
}

void datv_get_capture_size(int *width, int *height)
{
	int ver = daudio_main_version();
	int what = 0;

	VPRINTK("%s ver: %d\n", __func__, ver);
	switch (daudio_main_version())
	{
		case DAUDIO_BOARD_6TH:
		case DAUDIO_BOARD_5TH:
		default:
			if (hardware_data.cam_info == DAUDIO_CAMERA_CMMB)
				what = SENSOR_CMMB;
			else
				what = SENSOR_TW9912;
			break;
	}

	if (sf_daudio[what].sensor_get_capture_size != NULL)
		sf_daudio[what].sensor_get_capture_size(width, height);
}

void sensor_init_fnc(SENSOR_FUNC_TYPE *sensor_func)
{
	sensor_func->Open					= sensor_open;
	sensor_func->Close					= sensor_close;

	sensor_func->Set_Preview			= sensor_preview;
	sensor_func->Set_Capture			= sensor_capture;
	sensor_func->Set_CaptureCfg			= sensor_capturecfg;

	sensor_func->Set_Zoom				= sensor_zoom;
	sensor_func->Set_AF					= sensor_autofocus;
	sensor_func->Set_Effect				= sensor_effect;
	sensor_func->Set_Flip				= sensor_flip;
	sensor_func->Set_ISO				= sensor_iso;
	sensor_func->Set_ME					= sensor_me;
	sensor_func->Set_WB					= sensor_wb;
	sensor_func->Set_Bright				= sensor_bright;
	sensor_func->Set_Scene				= sensor_scene;
	sensor_func->Set_Contrast			= sensor_contrast;
	sensor_func->Set_Saturation			= sensor_saturation;
	sensor_func->Set_Hue				= sensor_hue;
	sensor_func->Get_Video				= sensor_getVideo;
	sensor_func->Check_Noise			= sensor_checkNoise;
	sensor_func->Reset_Video_Decoder	= sensor_resetVideoDecoder;
	sensor_func->Set_Path				= sensor_setPath;
	sensor_func->Get_videoFormat		= sensor_getVideoFormat;
	sensor_func->Set_writeRegister		= sensor_writeRegister;
	sensor_func->Get_readRegister		= sensor_readRegister;

	sensor_func->Check_ESD				= NULL;
	sensor_func->Check_Luma				= NULL;
}
