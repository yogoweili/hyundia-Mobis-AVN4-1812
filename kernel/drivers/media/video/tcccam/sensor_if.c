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

#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/videodev2.h>
//#include <media/video-buf.h>
#include <linux/delay.h>
#include <asm/mach-types.h>
#include <linux/regulator/consumer.h>
#include <linux/spinlock.h>
#include <asm/system.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#if defined(CONFIG_ARCH_TCC893X)
#include <linux/gpio.h>
#include <mach/bsp.h>
#include <mach/gpio.h>
#include <mach/daudio_info.h>
#endif
#include "tcc_cam_i2c.h"
#include <mach/tcc_cam_ioctrl.h>
#include "sensor_if.h"
#include "tdd_cif.h"
#include "cam.h"

#if defined(CONFIG_REGULATOR) && (defined(CONFIG_MACH_TCC9200S_SPACEY) || defined(CONFIG_MACH_TCC9200S_M340EVB))
#if defined(CONFIG_MACH_TCC9200S_M340EVB)
#define VDD_AF_NAME "vdd_cam_af28"
#define VDD_IO_NAME "vdd_cam_io27"
#define VDD_CORE_NAME "vdd_cam_core12"
#else
#define VDD_AF_NAME "vdd_isp_ram18"
#define VDD_IO_NAME "vdd_isp_io28"
#define VDD_CORE_NAME "vdd_isp_core12"
#endif

struct regulator *vdd_cam_core;
struct regulator *vdd_cam_af;
struct regulator *vdd_cam_io;
#endif


static int debug	   = 0;
#define dprintk(msg...)	if (debug) { printk(KERN_INFO "Sensor_if: " msg); }

static int enabled = 0;

/* list of image formats supported by sensor sensor */
const static struct v4l2_fmtdesc sensor_formats[] = {
	{
		/* Note:  V4L2 defines RGB565 as:
		 *
		 *	Byte 0			  Byte 1
		 *	g2 g1 g0 r4 r3 r2 r1 r0	  b4 b3 b2 b1 b0 g5 g4 g3
		 *
		 * We interpret RGB565 as:
		 *
		 *	Byte 0			  Byte 1
		 *	g2 g1 g0 b4 b3 b2 b1 b0	  r4 r3 r2 r1 r0 g5 g4 g3
		 */
		.description	= "RGB565, le",
		.pixelformat	= V4L2_PIX_FMT_RGB565,
	},{
		/* Note:  V4L2 defines RGB565X as:
		 *
		 *	Byte 0			  Byte 1
		 *	b4 b3 b2 b1 b0 g5 g4 g3	  g2 g1 g0 r4 r3 r2 r1 r0
		 *
		 * We interpret RGB565X as:
		 *
		 *	Byte 0			  Byte 1
		 *	r4 r3 r2 r1 r0 g5 g4 g3	  g2 g1 g0 b4 b3 b2 b1 b0
		 */
		.description	= "RGB565, be",
		.pixelformat	= V4L2_PIX_FMT_RGB565X,
	},
	{
		.description	= "YUYV (YUV 4:2:2), packed",
		.pixelformat	= V4L2_PIX_FMT_YUYV,
	},{
		.description	= "UYVY, packed",
		.pixelformat	= V4L2_PIX_FMT_UYVY,
	},
	{
		/* Note:  V4L2 defines RGB555 as:
		 *
		 *	Byte 0			  Byte 1
		 *	g2 g1 g0 r4 r3 r2 r1 r0	  x  b4 b3 b2 b1 b0 g4 g3
		 *
		 * We interpret RGB555 as:
		 *
		 *	Byte 0			  Byte 1
		 *	g2 g1 g0 b4 b3 b2 b1 b0	  x  r4 r3 r2 r1 r0 g4 g3
		 */
		.description	= "RGB555, le",
		.pixelformat	= V4L2_PIX_FMT_RGB555,
	},{
		/* Note:  V4L2 defines RGB555X as:
		 *
		 *	Byte 0			  Byte 1
		 *	x  b4 b3 b2 b1 b0 g4 g3	  g2 g1 g0 r4 r3 r2 r1 r0
		 *
		 * We interpret RGB555X as:
		 *
		 *	Byte 0			  Byte 1
		 *	x  r4 r3 r2 r1 r0 g4 g3	  g2 g1 g0 b4 b3 b2 b1 b0
		 */
		.description	= "RGB555, be",
		.pixelformat	= V4L2_PIX_FMT_RGB555X,
	}
};

#define NUM_CAPTURE_FORMATS ARRAY_SIZE(sensor_formats)

#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
enum image_size { QQXGA, QXGA, UXGA, SXGA, XGA, SVGA, VGA, QVGA, QCIF };
#define NUM_IMAGE_SIZES 9
#else // CONFIG_VIDEO_DUAL_CAMERA_SUPPORT
#if defined(SENSOR_2M)
enum image_size { UXGA, SXGA, XGA, SVGA, VGA, QVGA, QCIF };
#define NUM_IMAGE_SIZES 7
#elif defined(SENSOR_1_3M)
enum image_size { SXGA, XGA, SVGA, VGA, QVGA, QCIF };
#define NUM_IMAGE_SIZES 6
#elif defined(SENSOR_3M)
enum image_size { QXGA, UXGA, SXGA, XGA, SVGA, VGA, QVGA, QCIF };
#define NUM_IMAGE_SIZES 8
#elif defined(SENSOR_5M)
enum image_size { QQXGA, QXGA, UXGA, SXGA, XGA, SVGA, VGA, QVGA, QCIF };
#define NUM_IMAGE_SIZES 9
#elif defined(SENSOR_VGA)
enum image_size { VGA, sCIF, QVGA, QCIF };
#define NUM_IMAGE_SIZES 4
#elif defined(SENSOR_TVP5150) || defined(SENSOR_RDA5888) || defined(SENSOR_DAUDIO)
enum image_size { XGA, VGA };
#define NUM_IMAGE_SIZES 2
#endif
#endif // CONFIG_VIDEO_DUAL_CAMERA_SUPPORT

enum pixel_format { YUV, RGB565, RGB555 };
#define NUM_PIXEL_FORMATS 3

SENSOR_FUNC_TYPE sensor_func;

//need to revised  MCC
#ifdef CONFIG_ARCH_OMAP24XX
#define NUM_OVERLAY_FORMATS 4
#else
#define NUM_OVERLAY_FORMATS 2
#endif


#define DEF_BRIGHTNESS 				10
#define DEF_CONTRAST 				10
#define DEF_SATURATION 			10
#define DEF_HUE 						10
#define DEF_ISO 						0
#define DEF_AWB 					0
#define DEF_EFFECT 					0
#define DEF_ZOOM 					0
#define DEF_FLIP 					0
#define DEF_SCENE 					0
#define DEF_METERING_EXPOSURE 	0
#define DEF_EXPOSURE 				0
#define DEF_FOCUSMODE 				0
#define DEF_FLASH 					0

/*  Video controls  */
static unsigned int need_new_set = 0;
static struct vcontrol {
        struct v4l2_queryctrl qc;
        int current_value;
		int	need_set;
} control[] = {
	{ { V4L2_CID_BRIGHTNESS, V4L2_CTRL_TYPE_INTEGER, "Brightness", 0, 21, 1, DEF_BRIGHTNESS }, 0, 0},
	{ { V4L2_CID_CONTRAST, V4L2_CTRL_TYPE_INTEGER, "Contrast", 0, 21, 1, DEF_CONTRAST }, 0, 0},
	{ { V4L2_CID_SATURATION, V4L2_CTRL_TYPE_INTEGER, "Saturation", 0, 21, 1, DEF_SATURATION }, 0, 0},
	{ { V4L2_CID_HUE, V4L2_CTRL_TYPE_INTEGER, "Hue", 0, 21, 1, DEF_HUE }, 0, 0},
	{ { V4L2_CID_AUTO_WHITE_BALANCE, V4L2_CTRL_TYPE_INTEGER, "Auto White Balance", 0, 5, 1, DEF_AWB }, 0, 0},
	{ { V4L2_CID_ISO, V4L2_CTRL_TYPE_INTEGER, "Iso", 0, 3, 1, DEF_ISO }, 0, 0},
	{ { V4L2_CID_EFFECT, V4L2_CTRL_TYPE_INTEGER, "Special Effect", 0, 6, 1, DEF_EFFECT }, 0, 0},
	{ { V4L2_CID_ZOOM, V4L2_CTRL_TYPE_INTEGER, "Zoom", 0, 24, 1, DEF_ZOOM }, 0, 0},
	{ { V4L2_CID_FLIP, V4L2_CTRL_TYPE_INTEGER, "Flip", 0, 3, 1, DEF_FLIP }, 0, 0},
	{ { V4L2_CID_SCENE, V4L2_CTRL_TYPE_INTEGER, "Scene", 0, 5, 1, DEF_SCENE}, 0, 0},
	{ { V4L2_CID_METERING_EXPOSURE, V4L2_CTRL_TYPE_INTEGER, "Metering Exposure", 0, 3, 1, DEF_METERING_EXPOSURE}, 0, 0},
	{ { V4L2_CID_EXPOSURE, V4L2_CTRL_TYPE_INTEGER, "Exposure", -20, 20, 5, DEF_EXPOSURE}, 0, 0},
	{ { V4L2_CID_FOCUS_MODE, V4L2_CTRL_TYPE_INTEGER, "AutoFocus", 0, 1, 2, DEF_FOCUSMODE}, 0, 0},
	{ { V4L2_CID_FLASH, V4L2_CTRL_TYPE_INTEGER, "Flash", 0, 4, 1, DEF_FLASH}, 0, 0},
};


unsigned int sensor_control[] =
{
	V4L2_CID_BRIGHTNESS,
	V4L2_CID_CONTRAST,
	V4L2_CID_SATURATION,
	V4L2_CID_HUE,
	V4L2_CID_AUTO_WHITE_BALANCE,
	V4L2_CID_ISO,
	V4L2_CID_EFFECT,
	V4L2_CID_ZOOM,
	V4L2_CID_FLIP,
	V4L2_CID_SCENE,
	V4L2_CID_METERING_EXPOSURE,
	V4L2_CID_EXPOSURE,
	V4L2_CID_FOCUS_MODE
	//V4L2_CID_FLASH
};

#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
extern TCC_SENSOR_INFO_TYPE tcc_sensor_info;
extern int CameraID;

spinlock_t sensor_lock = __SPIN_LOCK_UNLOCKED(sensor_lock);
#endif

#ifndef USE_SENSOR_EFFECT_IF
extern int tccxxx_cif_set_effect (u8 nCameraEffect);
#endif
#ifndef USE_SENSOR_ZOOM_IF
extern int tccxxx_cif_set_zoom(unsigned char arg);
#endif

int sensor_if_change_mode(unsigned char capture_mode)
{
    int ret=0;

	if(capture_mode)
		ret = sensor_func.Set_Capture();
	else
		ret = sensor_func.Set_Preview();

    return ret;
}

int sensor_if_adjust_autofocus(void)
{
	return sensor_func.Set_AF(0);
}

/* Returns the index of the requested ID from the control structure array */
static int find_vctrl(int id)
{
	int i;
	int ctrl_cnt = ARRAY_SIZE(control);

	if (id < V4L2_CID_BASE)
		return -EDOM;

	for (i = ctrl_cnt - 1; i >= 0; i--)
	{
		if (control[i].qc.id == id)
			break;
	}

	if (i < 0)
		i = -EINVAL;

	return i;
}

/* initialize Sensor after module was reset. */
int sensor_init_module(enum image_size isize, enum pixel_format pfmt,unsigned long xclk)
{
	return 0;
}


/* Find the best match for a requested image capture size.  The best match
 * is chosen as the nearest match that has the same number or fewer pixels
 * as the requested size, or the smallest image size if the requested size
 * has fewer pixels than the smallest image.
 */
static enum image_size sensor_find_size(unsigned int width, unsigned int height)
{
	enum image_size isize;
	unsigned long pixels = width*height;

	for(isize=0; isize < (NUM_IMAGE_SIZES-1); isize++)
	{
		#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
		if((tcc_sensor_info.sensor_sizes[isize + 1].height * tcc_sensor_info.sensor_sizes[isize + 1].width) > pixels)
		#else
		if((sensor_sizes[isize + 1].height * sensor_sizes[isize + 1].width) > pixels)
		#endif
		{
			return isize;
		}
	}
	//CONFIG_COBY_MID9125
	#if defined(CONFIG_VIDEO_ATV_SENSOR_DAUDIO)
		return VGA;
	#else
		return SXGA;
	#endif
}

/* following are sensor interface functions implemented by
 * sensor driver.
 */
int sensor_if_query_control(struct v4l2_queryctrl *qc)
{
	int i;

	i = find_vctrl (qc->id);
	if(i == -EINVAL) {
		qc->flags = V4L2_CTRL_FLAG_DISABLED;
		return 0;
	}

	if(i < 0) 		return -EINVAL;
	*qc = control[i].qc;

	return 0;
}

int sensor_if_get_control(struct v4l2_control *vc)
{
	int i;
	struct vcontrol * lvc;

	if((i = find_vctrl(vc->id)) < 0)
		return -EINVAL;

	lvc = &control[i];
	vc->value = lvc->current_value;

	return 0;
}

int sensor_if_set_control(struct v4l2_control *vc, unsigned char init)
{
	struct vcontrol *lvc;
	int val = vc->value;
	int i;

	if ((i = find_vctrl(vc->id)) < 0) 	return -EINVAL;
	lvc = &control[i];
	if (lvc->qc.maximum < val) 			return val;
	if (lvc->current_value != val || init) {
		#if defined(CONFIG_ARCH_TCC892X) || defined(CONFIG_ARCH_TCC893X)
			if (lvc->qc.id == V4L2_CID_ZOOM) {
				tccxxx_cif_set_zoom(val);
			} else {
				lvc->current_value = val;
				lvc->need_set = 1;
				need_new_set = 1;
			}
		#else
			lvc->current_value = val;
			lvc->need_set = 1;
			need_new_set = 1;
		#endif
	}

	return 0;
}

int sensor_if_check_control(void)
{
	int i, err;
	struct vcontrol *lvc;
	int ctrl_cnt = ARRAY_SIZE(control);

	if (!need_new_set) 	return 0;
	for (i=(ctrl_cnt - 1); i >= 0; i--) {
		lvc = &control[i];
		if (lvc->need_set) {
			switch(lvc->qc.id) {
				case V4L2_CID_BRIGHTNESS:
					err = sensor_func.Set_Bright(lvc->current_value);
					break;

				case V4L2_CID_CONTRAST:
					err = sensor_func.Set_Contrast(lvc->current_value);
					break;

				case V4L2_CID_SATURATION:
					err = sensor_func.Set_Saturation(lvc->current_value);
					break;

				case V4L2_CID_HUE:
					err = sensor_func.Set_Hue(lvc->current_value);
					break;

				case V4L2_CID_AUTO_WHITE_BALANCE:
					err = sensor_func.Set_WB(lvc->current_value);
					break;

				case V4L2_CID_EXPOSURE:
					err = sensor_func.Set_Exposure(lvc->current_value);
					break;

				case V4L2_CID_ISO:
					err = sensor_func.Set_ISO(lvc->current_value);
					break;

				case V4L2_CID_EFFECT:
					#ifdef USE_SENSOR_EFFECT_IF
						err = sensor_func.Set_Effect(lvc->current_value);
					#else
						err = tccxxx_cif_set_effect(lvc->current_value);
					#endif
					break;

				case V4L2_CID_ZOOM:
					#ifdef USE_SENSOR_ZOOM_IF
						err = sensor_func.Set_Zoom(lvc->current_value);
					#else
						err = tccxxx_cif_set_zoom(lvc->current_value);
					#endif
					break;

				case V4L2_CID_FLIP:
					err = sensor_func.Set_Flip(lvc->current_value);
					break;

				case V4L2_CID_SCENE:
					err = sensor_func.Set_Scene(lvc->current_value);
					break;

				case V4L2_CID_METERING_EXPOSURE:
					err = sensor_func.Set_ME(lvc->current_value);
					break;

				case V4L2_CID_FLASH:
					// todo:
					break;

				case V4L2_CID_FOCUS_MODE:
					err = sensor_func.Set_FocusMode(lvc->current_value);
					break;

				default:
					break;
			}
			lvc->need_set = 0;
		}
	}

	return 0;
}

/* In case of ESD-detection, to set current value in sensor after module was resetted. */
int sensor_if_set_current_control(void)
{
	struct v4l2_control vc;
	int i;
	int ctrl_cnt = ARRAY_SIZE(sensor_control);

	printk("Setting Sensor-ctrl!! %d /n", ctrl_cnt);

	for (i = ctrl_cnt - 1; i >= 0; i--)
	{
		vc.id = sensor_control[i];
		sensor_if_get_control(&vc);
		sensor_if_set_control(&vc, 1);
	}

	return 0;
}

/* Implement the VIDIOC_ENUM_FMT ioctl for the CAPTURE buffer type.
 */
int sensor_if_enum_pixformat(struct v4l2_fmtdesc *fmt)
{
	int index = fmt->index;
	enum v4l2_buf_type type = fmt->type;

	memset(fmt, 0, sizeof(*fmt));
	fmt->index = index;
	fmt->type = type;

	switch (fmt->type) {
		case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		if (index >= NUM_CAPTURE_FORMATS)
			return -EINVAL;
		break;

		case V4L2_BUF_TYPE_VIDEO_OVERLAY:
		if (index >= NUM_OVERLAY_FORMATS)
			return -EINVAL;
		break;

		default:
			return -EINVAL;
	}

	fmt->flags = sensor_formats[index].flags;
	strlcpy(fmt->description, sensor_formats[index].description, sizeof(fmt->description));
	fmt->pixelformat = sensor_formats[index].pixelformat;

	return 0;
}

/* Implement the VIDIOC_TRY_FMT ioctl for the CAPTURE buffer type.  This
 * ioctl is used to negotiate the image capture size and pixel format
 * without actually making it take effect.
 */
static int sensor_try_format(struct v4l2_pix_format *pix)
{
	enum image_size isize;
	int ifmt;

	isize = sensor_find_size(pix->width, pix->height);
	#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
	pix->width  = tcc_sensor_info.sensor_sizes[isize].width;
	pix->height = tcc_sensor_info.sensor_sizes[isize].height;
	#else
	pix->width  = sensor_sizes[isize].width;
	pix->height = sensor_sizes[isize].height;
	#endif

	for(ifmt=0; ifmt < NUM_CAPTURE_FORMATS; ifmt++)
	{
		if(pix->pixelformat == sensor_formats[ifmt].pixelformat)
			break;
	}

	if(ifmt == NUM_CAPTURE_FORMATS)
		ifmt = 0;

	pix->pixelformat = sensor_formats[ifmt].pixelformat;
	pix->field = V4L2_FIELD_NONE;
	pix->bytesperline = pix->width*2;
	pix->sizeimage = pix->bytesperline*pix->height;
	pix->priv = 0;

	switch(pix->pixelformat) {
		case V4L2_PIX_FMT_YUYV:
		case V4L2_PIX_FMT_UYVY:
		default:
			pix->colorspace = V4L2_COLORSPACE_JPEG;
			break;

		case V4L2_PIX_FMT_RGB565:
		case V4L2_PIX_FMT_RGB565X:
		case V4L2_PIX_FMT_RGB555:
		case V4L2_PIX_FMT_RGB555X:
			pix->colorspace = V4L2_COLORSPACE_SRGB;
			break;
	}

	return 0;
}



/* Given a capture format in pix, the frame period in timeperframe, and
 * the xclk frequency, set the capture format of the sensor.
 * The actual frame period will be returned in timeperframe.
 */
int sensor_if_configure(struct v4l2_pix_format *pix, unsigned long xclk)
{
  	enum pixel_format pfmt = YUV;

	switch (pix->pixelformat) {
		case V4L2_PIX_FMT_RGB565:
		case V4L2_PIX_FMT_RGB565X:
			pfmt = RGB565;
			break;
		case V4L2_PIX_FMT_RGB555:
		case V4L2_PIX_FMT_RGB555X:
			pfmt = RGB555;
			break;
		case V4L2_PIX_FMT_YUYV:
		case V4L2_PIX_FMT_UYVY:
		default:
			pfmt = YUV;
 	}

	return sensor_init_module(sensor_find_size(pix->width, pix->height), pfmt, xclk);
}


/* Initialize the sensor.
 * This routine allocates and initializes the data structure for the sensor,
 * powers up the sensor, registers the I2C driver, and sets a default image
 * capture format in pix.  The capture format is not actually programmed
 * into the sensor by this routine.
 * This function must return a non-NULL value to indicate that
 * initialization is successful.
 */
int sensor_if_init(struct v4l2_pix_format *pix)
{
 	int ret, gpio_ret;

	// set to initialize normal gpio that power, power_down(or stand-by) and reset of sensor.
	#if defined(CONFIG_ARCH_TCC893X)
	{
		dprintk("[TCC892x] gpio init start!\n");
		#if defined(CONFIG_MACH_M805_892X)
		#else
			#if defined(CONFIG_ARCH_TCC893X)
				if(system_rev == 0x1000) { 				// TCC8930
					// camera power
/**
 * @author sjpark@cleinsoft
 * @date 2013/08/02
 * Not use power enable pin.
 **/
#if defined(SENSOR_DAUDIO)

#else
					gpio_request(TCC_GPF(24), NULL);
					gpio_direction_output(TCC_GPF(24), 1);
#endif

					// camera stand-by( or pwdn )
					#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
						if(CameraID) { 	// front camera
							gpio_request(TCC_GPF(15), NULL);
							gpio_direction_output(TCC_GPF(15), 1);
						} else { 			// back camera
							gpio_request(TCC_GPF(29), NULL);
							gpio_direction_output(TCC_GPF(29), 1);
						}
					#else
						#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9P111) || defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9T111)
							gpio_request(TCC_GPF(29), NULL);
							gpio_direction_output(TCC_GPF(29), 1);
						#else
/**
 * @author sjpark@cleinsoft
 * @date 2013/08/02
 * Not use power enable pin.
 **/
#if defined(SENSOR_DAUDIO)

#else
							gpio_request(TCC_GPF(15), NULL);
							gpio_direction_output(TCC_GPF(15), 1);
#endif
						#endif
					#endif

					// camera reset
					#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
						if(CameraID) { 	// front camera
							gpio_request(TCC_GPF(14), NULL);
							gpio_direction_output(TCC_GPF(14), 0);
						} else { 			// back camera
							gpio_request(TCC_GPF(13), NULL);
							gpio_direction_output(TCC_GPF(13), 0);
						}
					#else
						#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9P111) || defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9T111)
							gpio_request(TCC_GPF(13), NULL);
							gpio_direction_output(TCC_GPF(13), 0);
						#else

							#if defined(SENSOR_DAUDIO)
							/**
							 * @author sjpark@cleinsoft
							 * @date 2013/10/15
							 * Do not use hw reset
							 **/
							#else
							gpio_request(TCC_GPF(14), NULL);
							gpio_direction_output(TCC_GPF(14), 0);
							#endif
						#endif
					#endif
				}
				else if(system_rev == 0x2000 || system_rev == 0x3000) { 				// TCC8935, TCC8933
					// camera power
					gpio_request(TCC_GPD(3), NULL);
					gpio_direction_output(TCC_GPD(3), 1);

					// camera stand-by( or pwdn )
					#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
						if(CameraID) { 	// front camera
							gpio_request(TCC_GPD(2), NULL);
							gpio_direction_output(TCC_GPD(2), 1);
						} else { 			// back camera
							gpio_request(TCC_GPG(1), NULL);
							gpio_direction_output(TCC_GPG(1), 1);
						}
					#else
						#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9P111) || defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9T111)
							gpio_request(TCC_GPG(1), NULL);
							gpio_direction_output(TCC_GPG(1), 1);
						#else
							gpio_request(TCC_GPD(2), NULL);
							gpio_direction_output(TCC_GPD(2), 1);
						#endif
					#endif

					// camera reset
					#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
						if(CameraID) { 	// front camera
							gpio_request(TCC_GPF(15), NULL);
							gpio_direction_output(TCC_GPF(15), 0);
						} else { 			// back camera
							gpio_request(TCC_GPD(1), NULL);
							gpio_direction_output(TCC_GPD(1), 0);
						}
					#else
						#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9P111) || defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9T111)
							gpio_request(TCC_GPD(1), NULL);
							gpio_direction_output(TCC_GPD(1), 0);
						#else
							gpio_request(TCC_GPF(15), NULL);
							gpio_direction_output(TCC_GPF(15), 0);
						#endif
					#endif
				}
				else if(system_rev == 0x5000 || system_rev == 0x5001 || system_rev == 0x5002) { 				// M805_TCC8935
					// do not control camera power.

					#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
						// camera power down
						if(CameraID) { 	// front camera
							gpio_request(TCC_GPD(2), NULL);
							gpio_direction_output(TCC_GPD(2), 1);
						} else { 			// back camera
							gpio_request(TCC_GPD(1), NULL);
							gpio_direction_output(TCC_GPD(1), 1);
						}
					#else
						// camera power down
						gpio_request(TCC_GPD(1), NULL);
						gpio_direction_output(TCC_GPD(1), 1);
						dprintk("Sensor init powerdown!!\n");
					#endif

					// camera reset
					gpio_request(TCC_GPF(15), NULL);
					gpio_direction_output(TCC_GPF(15), 0);
					dprintk("Sensor init reset !!\n");
				}
			#endif
		#endif
		dprintk("[TCC892x] gpio init end!\n");
	}
	#endif

	#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
	if(CameraID){
			spin_lock(&sensor_lock);
			CameraID = 0;

			if(!sensor_get_powerdown()){
				dprintk("First mt9p111 powerdown! \n");
				sensor_init_func_set_facing_back(&sensor_func);


				/* Go To PowerDown Mode */
				if(sensor_func.PowerDown(false) < 0)
				{
					printk("mt9p111 close error! \n");
					return -1;
				}


			}
			CameraID = 1;
			spin_unlock(&sensor_lock);

			sensor_init_func_set_facing_front(&sensor_func);

	}
	else{
			spin_lock(&sensor_lock);
			CameraID = 1;

			if(!sensor_get_powerdown()){
				dprintk("First mt9m113 powerdown! \n");
				sensor_init_func_set_facing_front(&sensor_func);


				/* Go To PowerDown Mode */
				if(sensor_func.PowerDown(false) < 0)
				{
					printk("mt9m113 close error! \n");
					return -1;
				}

			}

			CameraID = 0;
			spin_unlock(&sensor_lock);


			sensor_init_func_set_facing_back(&sensor_func);

	}
	#else // CONFIG_VIDEO_DUAL_CAMERA_SUPPORT
	sensor_init_fnc(&sensor_func);
	#endif // CONFIG_VIDEO_DUAL_CAMERA_SUPPORT

	TDD_CIF_Initialize();

	if(!enabled)
	{
		#if !defined(CONFIG_VIDEO_CAMERA_SENSOR_AIT848_ISP)
		if((ret = DDI_I2C_Initialize()) < 0)
		{
			return -1;
		}
		enabled = 1;
		#endif
	}

	/* common register initialization */
#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
	if (sensor_func.Open(false) < 0) {
#else
	if (sensor_func.Open() < 0) {
#endif
		return -1;
	}

	/* Make the default capture format QVGA YUV422 */
	#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
		pix->width = 640; // tcc_sensor_info.sensor_sizes[0].width;
		pix->height = 480; // tcc_sensor_info.sensor_sizes[0].height;
	#else
		pix->width = sensor_sizes[VGA].width;
		pix->height = sensor_sizes[VGA].height;
	#endif

	pix->pixelformat = V4L2_PIX_FMT_YUYV;
	sensor_try_format(pix);

	need_new_set = 0;

	return 0;
}

int sensor_if_deinit(void)
{
	#if defined(CONFIG_ARCH_TCC893X)
		// In case of 892X, we have to add
		#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)

		#endif
	#else
	// todo :
	#endif

	return 0;
}

void sensor_pwr_control(int name, int enable)
{
#if defined(CONFIG_REGULATOR) && (defined(CONFIG_MACH_TCC9200S_SPACEY) || defined(CONFIG_MACH_TCC9200S_M340EVB))
	if(enable)
	{
		switch(name)
		{
			case CAM_CORE:
				if (vdd_cam_core)
					regulator_enable(vdd_cam_core);
				break;

			case CAM_ETC:
				if (vdd_cam_af)
					regulator_enable(vdd_cam_af);
				break;

			case CAM_IO:
				if (vdd_cam_io)
					regulator_enable(vdd_cam_io);
				break;

			default:
				break;
		}
	}
	else
	{
		switch(name)
		{
			case CAM_CORE:
				if (vdd_cam_core)
					regulator_disable(vdd_cam_core);
				break;

			case CAM_ETC:
				if (vdd_cam_af)
					regulator_disable(vdd_cam_af);
				break;

			case CAM_IO:
				if (vdd_cam_io)
					regulator_disable(vdd_cam_io);
				break;

			default:
				break;
		}

	}
#endif
}

#if defined(CONFIG_VIDEO_CAMERA_SENSOR_AIT848_ISP)
void sensor_if_set_enable(void)
{
	DDI_I2C_Initialize();
	enabled = 1;
}

int sensor_if_get_enable(void)
{
	return enabled;
}
#endif

int sensor_if_get_max_resolution(int index)
{
	// parameter : index?? ???\u07ff? ?????? ???? ??\C0\BD.
	#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
		switch(tcc_sensor_info.sensor_sizes[0].width) {
			case 2560: return QQXGA; 	// 5M size
			case 2048: return QXGA; 		// 3M size
			case 1600: return UXGA; 		// 2M size
			case 1280: return SXGA;		//1.3M size
			case 720: return XGA; 		// analog tv
			case 640: return VGA; 		// VGA
		}
	#else // CONFIG_VIDEO_DUAL_CAMERA_SUPPORT
		#if defined(SENSOR_5M)
			return 0; // QQXGA
		#elif defined(SENSOR_3M)
			return 1; // QXGA
		#elif defined(SENSOR_2M)
			return 2; // UXGA
		#elif defined(SENSOR_1_3M)
			return 3; // SXGA
		#elif defined(SENSOR_TVP5150) || defined(SENSOR_RDA5888) || defined(SENSOR_DAUDIO)
			return 4; // XGA
		#elif defined(SENSOR_VGA)
			return 6; // VGA
		#endif
	#endif // CONFIG_VIDEO_DUAL_CAMERA_SUPPORT
}

int sensor_if_get_sensor_framerate(int *nFrameRate)
{
	#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
		*nFrameRate = tcc_sensor_info.framerate;
	#else
		*nFrameRate = SENSOR_FRAMERATE;
	#endif

	if(*nFrameRate)
		return 0;
	else{
		printk("Sensor Driver dosen't have frame rate information!!\n");
		return -1;
	}
}


#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
void sensor_if_set_facing_front(void)
{
	#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9M113)
		sensor_info_init_mt9m113(&tcc_sensor_info);
	#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_SR130PC10)
		sensor_info_init_sr130pc10(&tcc_sensor_info);
	#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_OV7690)
		sensor_info_init_ov7690(&tcc_sensor_info);
	#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_SIV100B)
		sensor_info_init_siv100b(&tcc_sensor_info);
	#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_HI704)
		sensor_info_init_hi704(&tcc_sensor_info);
	#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_SP0A18)
		sensor_info_init_sp0a18(&tcc_sensor_info);
	#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_SP0A19)
		sensor_info_init_sp0a19(&tcc_sensor_info);
	#endif
}

void sensor_if_set_facing_back(void)
{
	#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9P111)
		sensor_info_init_mt9p111(&tcc_sensor_info);
	#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_S5K5CAGA)
		sensor_info_init_s5k5caga(&tcc_sensor_info);
	#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9T111)
		sensor_info_init_mt9t111(&tcc_sensor_info);
	#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9D112)
		sensor_info_init_mt9d112(&tcc_sensor_info);
	#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_HI253)
		sensor_info_init_hi253(&tcc_sensor_info);
	#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_SP2518)
		sensor_info_init_sp2518(&tcc_sensor_info);
	#endif
}

void sensor_init_func_set_facing_front(SENSOR_FUNC_TYPE *sensor_func)
{
	#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9M113)
		sensor_init_fnc_mt9m113(sensor_func);
	#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_SR130PC10)
		sensor_init_fnc_sr130pc10(sensor_func);
	#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_OV7690)
		sensor_init_fnc_ov7690(sensor_func);
	#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_SIV100B)
		sensor_init_fnc_siv100b(sensor_func);
	#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_HI704)
		sensor_init_fnc_hi704(sensor_func);
	#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_SP0A18)
		sensor_init_fnc_sp0a18(sensor_func);
	#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_SP0A19)
		sensor_init_fnc_sp0a19(sensor_func);
	#endif
}

void sensor_init_func_set_facing_back(SENSOR_FUNC_TYPE *sensor_func)
{
	#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9P111)
		sensor_init_fnc_mt9p111(sensor_func);
	#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_S5K5CAGA)
		sensor_init_fnc_s5k5caga(sensor_func);
	#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9T111)
		sensor_init_fnc_mt9t111(sensor_func);
	#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9D112)
		sensor_init_fnc_mt9d112(sensor_func);
	#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_HI253)
		sensor_init_fnc_hi253(sensor_func);
	#elif defined(CONFIG_VIDEO_CAMERA_SENSOR_SP2518)
		sensor_init_fnc_sp2518(sensor_func);
	#endif
}
#endif

int sensor_if_restart(void)
{
#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
	sensor_func.Open(false);
#else
	sensor_func.Open();
#endif
	return 0;
}

int sensor_if_capture_config(int width, int height)
{
	return sensor_func.Set_CaptureCfg(width, height);
}

int sensor_if_isESD(void)
{
	return sensor_func.Check_ESD(0);
}
/* Prepare for the driver to exit.
 * Balances sensor_if_init().
 * This function must de-initialize the sensor and its associated data
 * structures.
 */
int sensor_if_cleanup(void)
{
	dprintk("enabled = [%d]\n", enabled);

	if(enabled)
	{
		sensor_func.Close();
		TDD_CIF_Termination();
		DDI_I2C_Terminate();
		enabled = 0;
	}

    return 0;
}

void sensor_delay(int ms)
{
#if 1 //ZzaU
	unsigned int msec;

	msec = ms / 10; //10msec unit

	if(!msec)
		msleep(1);
	else
		msleep(msec);
#else
	int totCnt, iCnt, temp = 0;

	totCnt = ms*20000;
	for(iCnt = 0; iCnt < totCnt; iCnt++)
	{
		temp++;
	}
#endif
}

void sensor_turn_on_camera_flash(void)
{
	#if	defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.
	#else
		if(machine_is_m803()){
			gpio_request(TCC_GPA(6), NULL);
			gpio_direction_output(TCC_GPA(6), 1);

			gpio_set_value(TCC_GPA(6), 1);
		}
	#endif
}

void sensor_turn_off_camera_flash(void)
{
	#if	defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.
	#else
		if(machine_is_m803())
			gpio_set_value(TCC_GPA(6), 0);
	#endif
}

void sensor_power_enable(void)
{
	#if defined(CONFIG_ARCH_TCC893X)
		#if defined(CONFIG_MACH_M805_892X)
			#ifdef CONFIG_M805S_8923_0XA
				gpio_set_value(TCC_GPE(3), 1);
			#endif
		#else
			#if defined(CONFIG_ARCH_TCC893X)
				if(system_rev == 0x1000) { 			// TCC8930
/**
 * @author sjpark@cleinsoft
 * @date 2013/08/02
 * Not use power enable pin.
 **/
#if defined(SENSOR_DAUDIO)

#else
					gpio_set_value(TCC_GPF(24), 1); 
#endif
				} else if(system_rev == 0x2000) { 		// TCC8935
					gpio_set_value(TCC_GPD(3), 1);
				} else if(system_rev == 0x3000) { 		// TCC8933
					gpio_set_value(TCC_GPD(3), 1);
				} else if(system_rev == 0x5000 || system_rev == 0x5001 || system_rev == 0x5002) { 		// M805_TCC8935
					// do not control camera power.
				}
			#elif defined(CONFIG_ARCH_TCC892X)
				if(system_rev == 0x1005 || system_rev == 0x1007) {
					gpio_set_value(TCC_GPF(24), 1);
				} else if(system_rev == 0x1006) {
					gpio_set_value(TCC_GPD(3), 1);
				} else if(system_rev == 0x1008){
					gpio_set_value(TCC_GPC(6), 1);
				} else{
					gpio_set_value(TCC_GPD(21), 1);
				}
			#endif
		#endif
	#endif
}


void sensor_power_disable(void)
{
	#if defined(CONFIG_ARCH_TCC893X)
		#if defined(CONFIG_MACH_M805_892X)
			#ifdef CONFIG_M805S_8923_0XA
				gpio_set_value(TCC_GPE(3), 0);
			#endif
		#else
			#if defined(CONFIG_ARCH_TCC893X)
				if(system_rev == 0x1000) { 			// TCC8930
/**
 * @author sjpark@cleinsoft
 * @date 2013/08/02
 * Not use power enable pin.
 **/
#if defined(SENSOR_DAUDIO)

#else
					gpio_set_value(TCC_GPF(24), 0);
#endif
				} else if(system_rev == 0x2000) { 		// TCC8935
					gpio_set_value(TCC_GPD(3), 0);
				} else if(system_rev == 0x3000) { 		// TCC8933
					gpio_set_value(TCC_GPD(3), 0);
				} else if(system_rev == 0x5000 || system_rev == 0x5001 || system_rev == 0x5002) { 		// M805_TCC8935
					// do not control camera power.
				}
			#elif defined(CONFIG_ARCH_TCC892X)
				if(system_rev == 0x1005 || system_rev == 0x1007) {
					gpio_set_value(TCC_GPF(24), 0);
				} else if(system_rev == 0x1006) {
					gpio_set_value(TCC_GPD(3), 0);
				} else if(system_rev == 0x1008) {
					gpio_set_value(TCC_GPC(6), 0);
				} else {
					gpio_set_value(TCC_GPD(21), 0);
				}
			#endif
		#endif
	#endif
}

int sensor_get_powerdown(void)
{
	#if defined(CONFIG_ARCH_TCC893X)
		#if defined(CONFIG_MACH_M805_892X)
			// todo :
		#else
			#if defined(CONFIG_ARCH_TCC893X)
				if(system_rev == 0x1000) { 			// TCC8930
					#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
						if(CameraID) { 	// front camera
							return gpio_get_value(TCC_GPF(15));
						} else { 			// back camera
							return gpio_get_value(TCC_GPF(29));
						}
					#else
						#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9P111) || defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9T111)
							return gpio_get_value(TCC_GPF(29));
						#else
/**
 * @author sjpark@cleinsoft
 * @date 2013/08/02
 * Not use power enable pin.
 **/
#if defined(SENSOR_DAUDIO)
							return 0;
#else
							return gpio_get_value(TCC_GPF(15));
#endif
						#endif
					#endif
				} else if(system_rev == 0x2000 || system_rev == 0x3000) { 		// TCC8935, TCC8933
					#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
						if(CameraID) { 	// front camera
							return gpio_get_value(TCC_GPD(2));
						} else { 			// back camera
							return gpio_get_value(TCC_GPG(1));
						}
					#else
						#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9P111) || defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9T111) || defined(CONFIG_VIDEO_CAMERA_SENSOR_S5K5CAGA)
							return gpio_get_value(TCC_GPG(1));
						#else
							return gpio_get_value(TCC_GPD(2));
						#endif
					#endif
				} else if(system_rev == 0x5000 || system_rev == 0x5001 || system_rev == 0x5002) { 		// M805_TCC8935
					#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
						if(CameraID) { 	// front camera
							return gpio_get_value(TCC_GPD(2));
						} else { 			// back camera
							return gpio_get_value(TCC_GPD(1));
						}
					#else
						return gpio_get_value(TCC_GPD(1));
					#endif
				}
			#endif
		#endif
	#endif
}



 
 void sensor_powerdown_enable(void)
 {
	#if defined(CONFIG_ARCH_TCC893X)
		#if defined(CONFIG_MACH_M805_892X)
			#if defined(CONFIG_M805S_8925_0XX)
				 gpio_set_value(TCC_GPD(1), 1);
			#else
				 if(system_rev == 0x2002 || system_rev == 0x2003 || system_rev == 0x2004 || system_rev == 0x2005 || system_rev == 0x2006 || system_rev == 0x2007)
					 gpio_set_value(TCC_GPD(1), 1);
				 else
					 dprintk("M805S PWDN Enable!!\n");
					 gpio_set_value(TCC_GPE(22), 1);
			#endif
		#else
			#if defined(CONFIG_ARCH_TCC893X)
				 dprintk("camera power down enable. \n");
				 if(system_rev == 0x1000) { 		 // TCC8930
					#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
						 if(CameraID) {  // front camera
							 gpio_set_value(TCC_GPF(15), 1);
						 } else {			 // back camera
							 gpio_set_value(TCC_GPF(29), 1);
						 }
					#else
						#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9P111) || defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9T111)
							 gpio_set_value(TCC_GPF(29), 1);
						#else					
 /**
  * @author sjpark@cleinsoft
  * @date 2013/08/02
  * Not use power enable pin.
  **/
#if defined(SENSOR_DAUDIO)
 
#else
							 gpio_set_value(TCC_GPF(15), 1);
#endif
						#endif
					#endif
				 } else if(system_rev == 0x2000 || system_rev == 0x3000) {		 // TCC8935, TCC8933
					#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
						 if(CameraID) {  // front camera
							 gpio_set_value(TCC_GPD(2), 1);
						 } else {			 // back camera
							 gpio_set_value(TCC_GPG(1), 1);
						 }
					#else
						#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9P111) || defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9T111) || defined(CONFIG_VIDEO_CAMERA_SENSOR_S5K5CAGA)
							 gpio_set_value(TCC_GPG(1), 1);
						#else
							 gpio_set_value(TCC_GPD(2), 1);
						#endif
					#endif
				 } else if(system_rev == 0x5000 || system_rev == 0x5001 || system_rev == 0x5002) {		 // M805_TCC8935
					#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
						 if(CameraID) {  // front camera
							 gpio_set_value(TCC_GPD(2), 1);
						 } else {			 // back camera
							 gpio_set_value(TCC_GPD(1), 1);
						 }
					#else
						 gpio_set_value(TCC_GPD(1), 1);
					#endif
				 }
			#endif
		#endif
	#endif
 }
 void sensor_powerdown_disable(void)
 {
	#if defined(CONFIG_ARCH_TCC893X)
		#if defined(CONFIG_MACH_M805_892X)
			#if defined(CONFIG_M805S_8925_0XX)
				 gpio_set_value(TCC_GPD(1), 0);
			#else
				 if(system_rev == 0x2002 || system_rev == 0x2003 || system_rev == 0x2004 || system_rev == 0x2005 || system_rev == 0x2006 || system_rev == 0x2007)
					 gpio_set_value(TCC_GPD(1), 0);
				 else
					 dprintk("M805S PWDN Disable!!\n");
					 gpio_set_value(TCC_GPE(22), 0);
			#endif
		#else
			#if defined(CONFIG_ARCH_TCC893X)
				 dprintk("camera power down disable. \n");
				 if(system_rev == 0x1000) { 		 // TCC8930
					#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
						 if(CameraID) {  // front camera
							 gpio_set_value(TCC_GPF(15), 0);
						 } else {			 // back camera
							 gpio_set_value(TCC_GPF(29), 0);
						 }
					#else
						#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9P111) || defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9T111)
							 gpio_set_value(TCC_GPF(29), 0);
						#else					
 /**
  * @author sjpark@cleinsoft
  * @date 2013/08/02
  * Not use power enable pin.
  **/
#if defined(SENSOR_DAUDIO)
 
#else
							 gpio_set_value(TCC_GPF(15), 0);
#endif
						#endif
					#endif
				 } else if(system_rev == 0x2000 || system_rev == 0x3000) {		 // TCC8935, TCC8933
					#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
						 if(CameraID) {  // front camera
							 gpio_set_value(TCC_GPD(2), 0);
						 } else {			 // back camera
							 gpio_set_value(TCC_GPG(1), 0);
						 }
					#else
						#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9P111) || defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9T111) || defined(CONFIG_VIDEO_CAMERA_SENSOR_S5K5CAGA)
							 gpio_set_value(TCC_GPG(1), 0);
						#else
							 gpio_set_value(TCC_GPD(2), 0);
						#endif
					#endif
				 } else if (system_rev == 0x5000 || system_rev == 0x5001 || system_rev == 0x5002) { 	 // M805_TCC8935
					#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
						 if(CameraID) {  // front camera
							 gpio_set_value(TCC_GPD(2), 0);
						 } else {			 // back camera
							 gpio_set_value(TCC_GPD(1), 0);
						 }
					#else
						 gpio_set_value(TCC_GPD(1), 0);
					#endif
				 }
			#endif
		#endif
	#endif
 }


void sensor_reset_high(void)
{
	#if defined(SENSOR_DAUDIO)
	unsigned reset_gpio = datv_get_reset_pin();
	if (reset_gpio < 0) //error
		return;

	gpio_set_value(reset_gpio, 1);

	
	
	#elif defined(CONFIG_ARCH_TCC893X)
		
			#if defined(CONFIG_ARCH_TCC893X)
				dprintk("camera reset high. \n");
				if(system_rev == 0x1000) { 			// TCC8930
					#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
						if(CameraID) { 	// front camera
							gpio_set_value(TCC_GPF(14), 1);
						} else { 			// back camera
							gpio_set_value(TCC_GPF(13), 1);
						}
					#else
						#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9P111) || defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9T111)
							gpio_set_value(TCC_GPF(13), 1);
						#else					
							gpio_set_value(TCC_GPF(14), 1);
						#endif
					#endif
				} else if(system_rev == 0x2000 || system_rev == 0x3000) { 		// TCC8935, TCC8933
					#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
						if(CameraID) { 	// front camera
							gpio_set_value(TCC_GPF(15), 1);
						} else { 			// back camera
							gpio_set_value(TCC_GPD(1), 1);
						}
					#else
						#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9P111) || defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9T111) || defined(CONFIG_VIDEO_CAMERA_SENSOR_S5K5CAGA)
							gpio_set_value(TCC_GPD(1), 1);
						#else
							gpio_set_value(TCC_GPF(15), 1);
						#endif
					#endif
				} else if(system_rev == 0x5000 || system_rev == 0x5001 || system_rev == 0x5002) { 		// M805_TCC8935
					gpio_set_value(TCC_GPF(15), 1);
				}
			#endif
	#endif
}


void sensor_reset_low(void)
{
	#if defined(SENSOR_DAUDIO)
	unsigned reset_gpio = datv_get_reset_pin();
	if (reset_gpio < 0)	//error
		return;

	gpio_set_value(reset_gpio, 0);

	#elif defined(CONFIG_ARCH_TCC893X)
			#if defined(CONFIG_ARCH_TCC893X)
				dprintk("camera reset low. \n");
				if(system_rev == 0x1000) { 			// TCC8930
					#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
						if(CameraID) { 	// front camera
							gpio_set_value(TCC_GPF(14), 0);
						} else { 			// back camera
							gpio_set_value(TCC_GPF(13), 0);
						}
					#else
						#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9P111) || defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9T111)
							gpio_set_value(TCC_GPF(13), 0);
						#else					
							gpio_set_value(TCC_GPF(14), 0);
						#endif
					#endif
				} else if(system_rev == 0x2000 || system_rev == 0x3000) { 		// TCC8935, TCC8933
					#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
						if(CameraID) { 	// front camera
							gpio_set_value(TCC_GPF(15), 0);
						} else { 			// back camera
							gpio_set_value(TCC_GPD(1), 0);
						}
					#else
						#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9P111) || defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9T111) || defined(CONFIG_VIDEO_CAMERA_SENSOR_S5K5CAGA)
							gpio_set_value(TCC_GPD(1), 0);
						#else
							gpio_set_value(TCC_GPF(15), 0);
						#endif
					#endif
				} else if(system_rev == 0x5000 || system_rev == 0x5001 || system_rev == 0x5002) { 		// M805_TCC8935
					gpio_set_value(TCC_GPF(15), 0);
				}
			#endif
	#endif
}


int sensor_if_get_video(int tmp)
{
	return sensor_func.Get_Video();
}

int sensor_if_check_video_noise(int tmp)
{
	return sensor_func.Check_Noise();
}

int sensor_if_reset_video_decoder(int tmp)
{
	return sensor_func.Reset_Video_Decoder();
}

int sensor_if_set_path(int *path)
{
	return sensor_func.Set_Path(*path);
}

int sensor_if_get_video_format(int tmp)
{
	return sensor_func.Get_videoFormat();
}

int sensor_if_set_write_i2c(I2C_DATA_Type *i2c)
{
	return sensor_func.Set_writeRegister(i2c->reg, i2c->val);
}

int sensor_if_get_read_i2c(int *reg)
{
	return sensor_func.Get_readRegister(*reg);
}

int sensor_if_get_rgear_status(int tmp)
{
	int ret = -1;

	#if defined(FEATURE_MOBIS_AVN)
		ret = gpio_get_value(TCC_GPG(13));
	#elif defined(FEATURE_LG_AVN)
		ret = gpio_get_value(TCC_GPF(16));
	#else
		ret = gpio_get_value(TCC_GPF(29));
	#endif

	return ret;
}

int sensor_if_init_avn(struct v4l2_pix_format *pix)
{
	int ret, gpio_ret;

	#if !defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
		sensor_init_fnc(&sensor_func);
	#endif

	if (!enabled) {
		enabled = 1;
	}

	#if !defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
		#if !defined(CONFIG_VIDEO_ATV_SENSOR_DAUDIO)
			if (sensor_func.Open() < 0) {
				return -1;
			}
		#endif

		pix->width = sensor_sizes[VGA].width;
		pix->height = sensor_sizes[VGA].height;
	#endif
	pix->pixelformat = V4L2_PIX_FMT_YUYV;
	sensor_try_format(pix);

	need_new_set = 0;

	return 0;
}

#if defined(FEATURE_HW_COLOR_EFFECT)
TCC_CAM_HW_RGB_LUT_SET_Type g_rgb_cmd;
int sensor_if_set_hw_rgb_effect(TCC_CAM_HW_RGB_LUT_SET_Type* rgb_cmd)
{
	int i;
/*
	printk("%s():  lut_number = %d. \n", __func__, rgb_cmd->lut_number);
	printk("%s():  rgb table... \n", __func__);
	for (i = 0; i < 256; i++) {
		printk("0x%x, ", rgb_cmd->rgb_table[i]);
	}
	printk("\n");

	memset(&g_rgb_cmd, 0, sizeof(TCC_CAM_HW_RGB_LUT_SET_Type));
	if (copy_from_user((void *)&g_rgb_cmd, (const void *)rgb_cmd, sizeof(TCC_CAM_HW_RGB_LUT_SET_Type))) {
		printk("%s()   copy_from_user() fail....... \n", __func__);
		return -EFAULT;
	}
*/
	g_rgb_cmd.lut_number = rgb_cmd->lut_number;
	for (i = 0; i < 256; i++) {
		g_rgb_cmd.rgb_table[i] = rgb_cmd->rgb_table[i];
	}
	return 0;
}

TCC_CAM_HW_YUV_LUT_SET_Type g_yuv_cmd;
int sensor_if_set_hw_yuv_effect(TCC_CAM_HW_YUV_LUT_SET_Type* yuv_cmd)
{
	int i;
/*
	printk("%s():  lut_number = %d. \n", __func__, yuv_cmd->lut_number);
	printk("%s():  rgb table... \n", __func__);
	for (i = 0; i < 256; i++) {
		printk("0x%x, ", yuv_cmd->yuv_table[i]);
	}
	printk("\n");

	memset(&g_yuv_cmd, 0, sizeof(TCC_CAM_HW_YUV_LUT_SET_Type));
	if (copy_from_user((void *)&g_yuv_cmd, (const void *)yuv_cmd, sizeof(TCC_CAM_HW_YUV_LUT_SET_Type))) {
		printk("%s()   copy_from_user() fail....... \n", __func__);
		return -EFAULT;
	}
*/
	g_yuv_cmd.lut_number = yuv_cmd->lut_number;
	for (i = 0; i < 256; i++) {
		g_yuv_cmd.yuv_table[i] = yuv_cmd->yuv_table[i];
	}
	return 0;
}
#endif

#if defined(FEATURE_SW_COLOR_EFFECT)
extern void update_cbcr(unsigned short *src_vcbcr_addr, unsigned int width, unsigned int height, short *p_cbcr_table); 
unsigned int *CAM_SW_LUT_TABLE;

void sensor_if_sw_lut_table_alloc(void)
{
	unsigned int size = 256 * 256 * 2;
	CAM_SW_LUT_TABLE = kzalloc(size , GFP_KERNEL);
	printk("%s() -   0x%p size : %d. \n", __func__, CAM_SW_LUT_TABLE, size);
}

void sensor_if_sw_lut_table_free(void)
{
	printk("%s() -  0x%p. \n", __func__, CAM_SW_LUT_TABLE);
	if(CAM_SW_LUT_TABLE) kzfree(CAM_SW_LUT_TABLE);
}

int sensor_if_set_sw_lut_effect(TCC_CAM_SW_LUT_SET_Type* lut_cmd)
{
	int change_cr = 0, change_cb = 0, cb  = 0, cr = 0;
	unsigned short *hue_set_v;
	printk("%s() -   saturation : %d, sin : %d, cos : %d. \n", __func__, lut_cmd->saturation, lut_cmd->sin_value, lut_cmd->cos_value);

	if (CAM_SW_LUT_TABLE == NULL) return ;
	hue_set_v = (unsigned short *)CAM_SW_LUT_TABLE;
	
	for (cr=0; cr <= 0xff; cr++) {
		for (cb=0 ; cb <= 0xff; cb++) {
			change_cb = (int)(cb * lut_cmd->cos_value + cr * lut_cmd->sin_value);
			change_cr = (int)(cr * lut_cmd->cos_value - cb * lut_cmd->sin_value);          

			#if 0
				change_cb  = change_cb/1000;
				change_cr  = change_cr/1000;
			#else
				change_cb  = (((change_cb) - 128000) * lut_cmd->saturation) / 10000 + 128;
				change_cr  = (((change_cr) - 128000) * lut_cmd->saturation) / 10000 + 128;
			#endif//

			if (change_cb > 255) change_cb = 255;
			if (change_cb < 0) change_cb = 0;

			if (change_cr > 255) change_cr = 255;
			if (change_cr < 0) change_cr = 0;
						
                   	*hue_set_v++  = (((change_cr & 0xff ) << 8) | (change_cb & 0xff)) & 0xFFFF;
		}
	}
	
	return 0;
}

int sensor_if_run_sw_lut_effect(TCC_CAM_LUT_INFO_Type* lut_info)
{
	unsigned int *base_y, *target_y;
	unsigned short *src_vcbcr_addr, *src_vcbcr_base_addr, *tar_vcbcr_addr, *tar_vcbcr_base_addr;
	unsigned short int_cbcr;
	unsigned int loop = 0;
	unsigned short *p_cbcr_table ;
	printk("%s() -   BaseAddr : 0x%x, BaseAddr1 : 0x%x, BaseAddr2 : 0x%x. \n", __func__, lut_info->BaseAddr, lut_info->BaseAddr1, lut_info->BaseAddr2);
	printk("%s() -   TarAddr : 0x%x, TarAddr1 : 0x%x, TarAddr2 : 0x%x. \n", __func__, lut_info->TarAddr, lut_info->TarAddr1, lut_info->TarAddr2);
	printk("%s() -   ImgSizeWidth : %d, ImgSizeHeight : %d, ImgFormat : %d. \n", __func__, lut_info->ImgSizeWidth, lut_info->ImgSizeHeight, lut_info->ImgFormat);

	base_y = ioremap_nocache(lut_info->BaseAddr, (lut_info->ImgSizeWidth * lut_info->ImgSizeHeight));
	target_y = ioremap_nocache(lut_info->TarAddr, (lut_info->ImgSizeWidth * lut_info->ImgSizeHeight));

	if ((base_y != NULL) && (target_y != NULL)) {
		memcpy(target_y, base_y, (lut_info->ImgSizeWidth * lut_info->ImgSizeHeight));
	}

	if (base_y != NULL) iounmap(base_y);
	if (target_y != NULL) iounmap(target_y);

	if (lut_info->BaseAddr1 == lut_info->TarAddr1) {
		src_vcbcr_base_addr = src_vcbcr_addr = ioremap_nocache(lut_info->BaseAddr1, (lut_info->ImgSizeWidth * lut_info->ImgSizeHeight));
		if (src_vcbcr_addr == NULL) return -ENOMEM;
		update_cbcr(src_vcbcr_addr, lut_info->ImgSizeWidth, lut_info->ImgSizeHeight,  (short *)CAM_SW_LUT_TABLE);
		iounmap(src_vcbcr_base_addr);
	} else {
		src_vcbcr_base_addr = src_vcbcr_addr = ioremap_nocache(lut_info->BaseAddr1, (lut_info->ImgSizeWidth * lut_info->ImgSizeHeight));
		tar_vcbcr_base_addr = tar_vcbcr_addr = ioremap_nocache(lut_info->TarAddr1, (lut_info->ImgSizeWidth * lut_info->ImgSizeHeight));
		if ((src_vcbcr_addr == NULL) || (tar_vcbcr_addr == NULL)) return -ENOMEM;
		loop = lut_info->ImgSizeHeight * lut_info->ImgSizeWidth / 8;
		p_cbcr_table = (short *)CAM_SW_LUT_TABLE;

		while (loop--) {
			int_cbcr = *src_vcbcr_addr++;
			*tar_vcbcr_addr++ = (p_cbcr_table[int_cbcr&0xFFFF] &0xFFFF) | ((p_cbcr_table[(int_cbcr>>16)&0xFFFF]<<16) &0xFFFF0000);
		}

		iounmap(src_vcbcr_base_addr);
		iounmap(tar_vcbcr_base_addr);
	}	

	return 0;
}
#endif


/* ----------------------------------------------------------------------- */
MODULE_DESCRIPTION("Camera SENSOR  driver");
MODULE_AUTHOR("Telechips");
MODULE_LICENSE("GPL");

