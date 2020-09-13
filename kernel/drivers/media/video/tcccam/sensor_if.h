/****************************************************************************
One line to give the program's name and a brief idea of what it does.
Copyright (C) 2013 Telechips Inc.

This program is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place,
Suite 330, Boston, MA 02111-1307 USA
****************************************************************************/

#include <linux/videodev2.h>

#ifndef TCC_SENSOR_IF_H
#define TCC_SENSOR_IF_H

#define CAM_CORE 	0
#define CAM_IO 		1
#define CAM_ETC		2 //AF or RAM

//#define TCC_POWER_CONTROL

typedef struct {
	#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
		int (*Open)(bool bChangeCamera);
	#else
		int (*Open)(void);
	#endif

	int (*Close)(void);
	int (*PowerDown)(bool bChangeCamera);

	int (*Set_Preview)(void);
	int (*Set_Capture)(void);
	int (*Set_CaptureCfg)(int width, int height);

	int (*Set_Zoom)(int val);
	int (*Set_AF)(int val);
	int (*Set_Effect)(int val);
	int (*Set_Flip)(int val);
	int (*Set_ISO)(int val);
	int (*Set_ME)(int val);
	int (*Set_WB)(int val);
	int (*Set_Bright)(int val);
	int (*Set_Scene)(int val);
	int (*Set_Exposure)(int val);
	int (*Set_FocusMode)(int val);
	int (*Set_Contrast)(int val);
	int (*Set_Saturation)(int val);
	int (*Set_Hue)(int val);
	int (*Get_Video)(void);
	int (*Check_Noise)(void);
	int (*Reset_Video_Decoder)(void);
	int (*Set_Path)(int val);
	int (*Get_videoFormat)(void);
	int (*Set_writeRegister)(int reg, int val);
	int (*Get_readRegister)(int reg);

	int (*Check_ESD)(int val);
	int (*Check_Luma)(int val);
}
SENSOR_FUNC_TYPE;

#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
enum sensor_reg_width {
	WORD_LEN,
	BYTE_LEN,
	MS_DELAY
};

struct sensor_reg {
	unsigned short reg;
	unsigned short val;
	enum sensor_reg_width width;
};

struct capture_size {
	unsigned long width;
	unsigned long height;
};

typedef struct tcc_sensor_info
{
	int i2c_addr;
	int reg_term;
	int val_term;
	int m_clock;
	int m_clock_source;
	int s_clock;
	int s_clock_source;
	int p_clock_pol;
	int v_sync_pol;
	int h_sync_pol;
#if  defined(CONFIG_ARCH_TCC893X)
	int de_pol;
#endif
	int format;
	int preview_w;
	int preview_h;
	int preview_zoom_offset_x;
	int preview_zoom_offset_y;
	int capture_w;
	int capture_h;
	int capture_zoom_offset_x;
	int capture_zoom_offset_y;
	int max_zoom_step;
	int cam_capchg_width;
	int capture_skip_frame;
	int framerate;
	struct capture_size *sensor_sizes;
}TCC_SENSOR_INFO_TYPE;

#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9P111)
#define SENSOR_5M
#define USING_HW_I2C
#include "mt9p111_5mp.h"
#endif
#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9T111)
#define SENSOR_3M
#define USING_HW_I2C
#include "mt9t111_3mp.h"
#endif
#if defined(CONFIG_VIDEO_CAMERA_SENSOR_S5K5CAGA)
#define SENSOR_3M
#define USING_HW_I2C
#include "s5k5caga_3mp.h"
#endif
#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MV9317)
#define SENSOR_3M
#define USING_HW_I2C
#include "mv9317_3mp.h"
#endif
#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9D112)
#define SENSOR_2M
#define USING_HW_I2C
#include "mt9d112_2mp.h"
#endif
#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9M113)
#define SENSOR_1_3M 
#define USING_HW_I2C 
#include "mt9m113_1.3mp.h"
#endif
#if defined(CONFIG_VIDEO_CAMERA_SENSOR_SR130PC10)
#define SENSOR_1_3M 
#define USING_HW_I2C 
#include "sr130pc10_1.3mp.h"
#endif
#if defined(CONFIG_VIDEO_CAMERA_SENSOR_OV7690)
#define SENSOR_VGA
#define USING_HW_I2C
#include "ov7690_vga.h"
#endif
#if defined(CONFIG_VIDEO_CAMERA_SENSOR_SIV100B)
#define SENSOR_VGA
#define USING_HW_I2C
#include "siv100b_vga.h"
#endif
#else // CONFIG_VIDEO_DUAL_CAMERA_SUPPORT
#if defined(CONFIG_VIDEO_CAMERA_SENSOR_AIT848_ISP)
#define SENSOR_5M
#define USE_SENSOR_ZOOM_IF
#define USE_SENSOR_EFFECT_IF
#include "venus_ait848_5mp.h"
#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9P111)
#define SENSOR_5M
#define USING_HW_I2C
#include "mt9p111_5mp.h"
#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9T111)
#define SENSOR_3M
#define USING_HW_I2C
#include "mt9t111_3mp.h"
#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9T113)
#define SENSOR_3M
#define USING_HW_I2C
#include "mt9t113_3mp.h"
#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_S5K5CAGA)
#define SENSOR_3M
#define USING_HW_I2C
#include "s5k5caga_3mp.h"
#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_MV9317)
#define SENSOR_3M
#define USING_HW_I2C
#include "mv9317_3mp.h"
#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9D112)
#define SENSOR_2M
#define USING_HW_I2C
#include "mt9d112_2mp.h"
#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_OV3640)
#define SENSOR_3M
#define USING_HW_I2C
#include "ov3640_3mp.h"
#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_S5K4BAFB)
#define SENSOR_2M
#define USING_HW_I2C
//#define TCC_VCORE_30FPS_CAMERASENSOR
#include "s5k4bafb_2mp.h"
#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_ISX006)
#define SENSOR_5M
#define USE_SENSOR_ZOOM_IF
#define USE_SENSOR_EFFECT_IF
#define USING_HW_I2C
#include "isx006_5mp.h"
#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_OV7690)
#define SENSOR_VGA
#define USING_HW_I2C
#include "ov7690_vga.h"
#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_SIV100B)
#define SENSOR_VGA
#define USING_HW_I2C
#include "siv100b_vga.h"
#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_GT2005)
#define SENSOR_2M
#define USING_HW_I2C
#include "gt2005_2mp.h"
#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9M113)
#define SENSOR_1_3M 
#define USING_HW_I2C 
#include "mt9m113_1.3mp.h" 
#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_SR130PC10)
#define SENSOR_1_3M 
#define USING_HW_I2C 
#include "sr130pc10_1.3mp.h" 
#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_HI704) //CONFIG_COBY_MID9125
#define USING_HW_I2C
#define SENSOR_HI704
#define SENSOR_VGA
#include "HI704_300K.h"
#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_GC0329) //CONFIG_COBY_MID7048
#define USING_HW_I2C
#define SENSOR_VGA
#include "gc0329_vga.h"
#elif defined(CONFIG_VIDEO_ATV_SENSOR_TVP5150)
#define USING_HW_I2C
#define SENSOR_TVP5150
#define SENSOR_BT656
#include "atv/tvp5150.h"
#elif defined(CONFIG_VIDEO_ATV_SENSOR_RDA5888)
#define USING_HW_I2C
#define SENSOR_RDA5888
#include "atv/rda5888_atv.h"
#elif defined(CONFIG_VIDEO_CAMERA_NEXTCHIP_TEST)
#define SENSOR_3M
#define USING_HW_I2C
#include "nextchip_test.h"
#elif defined(CONFIG_VIDEO_ATV_SENSOR_DAUDIO)
#define USING_HW_I2C
#define SENSOR_DAUDIO
#include "atv/daudio_atv.h"
#endif
#endif // CONFIG_VIDEO_DUAL_CAMERA_SUPPORT

extern int sensor_if_change_mode(unsigned char capture_mode);
extern int sensor_if_adjust_autofocus(void);
extern int sensor_if_query_control(struct v4l2_queryctrl *qc);
extern int sensor_if_get_control(struct v4l2_control *vc);
extern int sensor_if_set_control(struct v4l2_control *vc, unsigned char init);
extern int sensor_if_check_control(void);
extern int sensor_if_set_current_control(void);
extern int sensor_if_enum_pixformat(struct v4l2_fmtdesc *fmt);
extern int sensor_if_configure(struct v4l2_pix_format *pix, unsigned long xclk);	
extern int sensor_if_init(struct v4l2_pix_format *pix);
extern int sensor_if_deinit(void);
extern void sensor_pwr_control(int name, int enable);
#if defined(CONFIG_VIDEO_CAMERA_SENSOR_AIT848_ISP)
extern void sensor_if_set_enable(void);
extern int sensor_if_get_enable(void);
#endif
extern int sensor_if_get_max_resolution(int index);
extern int sensor_if_get_sensor_framerate(int *nFrameRate);
#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
extern void sensor_if_set_facing_front(void);
extern void sensor_if_set_facing_back(void);
extern void sensor_init_func_set_facing_front(SENSOR_FUNC_TYPE *sensor_func);
extern void sensor_init_func_set_facing_back(SENSOR_FUNC_TYPE *sensor_func);
#endif

extern int sensor_isit_change_camera(void);
extern int sensor_if_restart(void);
extern int sensor_if_capture_config(int width, int height);
extern int sensor_if_isESD(void);
extern int sensor_if_cleanup(void);
extern void sensor_delay(int ms);

extern void sensor_turn_on_camera_flash(void);
extern void sensor_turn_off_camera_flash(void);

extern void sensor_power_enable(void);
extern void sensor_power_disable(void);
extern int sensor_get_powerdown(void);
extern void sensor_powerdown_enable(void);
extern void sensor_powerdown_disable(void);
extern void sensor_reset_high(void);
extern void sensor_reset_low(void);

typedef struct {
	unsigned char reg;
	unsigned char val;
} I2C_DATA_Type;

extern int sensor_if_get_video(int tmp);
extern int sensor_if_check_video_noise(int tmp);
extern int sensor_if_reset_video_decoder(int tmp);
extern int sensor_if_set_path(int *path);
extern int sensor_if_get_video_format(int tmp);
extern int sensor_if_set_write_i2c(I2C_DATA_Type *i2c);
extern int sensor_if_get_read_i2c(int *reg);
extern int sensor_if_get_rgear_status(int tmp);
extern int sensor_if_init_avn(struct v4l2_pix_format *pix);

//#define FEATURE_HW_COLOR_EFFECT
#if defined(FEATURE_HW_COLOR_EFFECT)
typedef struct {
	unsigned int rgb_table[256];
	unsigned int lut_number;
} TCC_CAM_HW_RGB_LUT_SET_Type;

typedef struct {
	unsigned int yuv_table[256];
	unsigned int lut_number;
} TCC_CAM_HW_YUV_LUT_SET_Type;

extern TCC_CAM_HW_RGB_LUT_SET_Type g_rgb_cmd;
extern TCC_CAM_HW_YUV_LUT_SET_Type g_yuv_cmd;
extern int sensor_if_set_hw_rgb_effect(TCC_CAM_HW_RGB_LUT_SET_Type* rgb_cmd);
extern int sensor_if_set_hw_yuv_effect(TCC_CAM_HW_YUV_LUT_SET_Type* yuv_cmd);
#endif

//#define FEATURE_SW_COLOR_EFFECT
#if defined(FEATURE_SW_COLOR_EFFECT)
/*
	LUT TABLE SET.
	cos_value, sin_value : HUE angle value and It is mulitpled by 1000.
	if hue angle 15 degree : cos_value = cos(15) * 1000, sin_value = sin(15) * 1000

ex>	 sin   	cos
	{87, 	996}, 	HUE_DGREE_5,	
	{173, 	984}, 	HUE_DGREE_10
	{258, 	965}, 	HUE_DGREE_15
	{342, 	939}, 	HUE_DGREE_20
	{422, 	906},	HUE_DGREE_25
	{-87, 	996}, 	HUE_DGREE_355
	{-173, 	984}, 	HUE_DGREE_350
	{-258, 	965}, 	HUE_DGREE_345
	{-342, 	939}, 	HUE_DGREE_340
	{-422, 	906}	HUE_DGREE_335

	saturation : default value is 10.
*/

typedef struct {
	int sin_value;
	int cos_value;
	int saturation;
} TCC_CAM_SW_LUT_SET_Type;

typedef struct {
	unsigned int ImgSizeWidth;
	unsigned int ImgSizeHeight;
	unsigned int ImgFormat;
	unsigned int BaseAddr;
	unsigned int BaseAddr1;
	unsigned int BaseAddr2;
	unsigned int TarAddr;
	unsigned int TarAddr1;
	unsigned int TarAddr2;
} TCC_CAM_LUT_INFO_Type;

extern void sensor_if_sw_lut_table_alloc(void);
extern void sensor_if_sw_lut_table_free(void);
extern int sensor_if_set_sw_lut_effect(TCC_CAM_SW_LUT_SET_Type* lut_cmd);
extern int sensor_if_run_sw_lut_effect(TCC_CAM_LUT_INFO_Type* lut_info);
#endif
#endif
