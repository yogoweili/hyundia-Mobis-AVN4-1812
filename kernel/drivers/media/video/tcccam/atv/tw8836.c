#include <linux/slab.h>

#include <mach/daudio.h>
#include <mach/daudio_debug.h>

#include "../tcc_cam_i2c.h"

#include "daudio_atv.h"
#include "tw8836.h"

#define PRV_W 720
#define PRV_H 240
#define CAP_W 720
#define CAP_H 240

static int debug_tw8836 = DEBUG_DAUDIO_ATV;
#define VPRINTK(fmt, args...) if (debug_tw8836) printk(KERN_INFO "[cleinsoft TW8836] " fmt, ##args)

static struct sensor_reg TW8836_CAM_INIT[] =
{
	{0xFF, 0x00},   //select page 0
	{0x02, 0x00},
	{0x03, 0xFE},
	{0x04, 0x01},
	{0x06, 0x06},
	{0x07, 0x02},
	{0x08, 0x86},
	{0x0F, 0x00},
	{0x1F, 0x00},
	{0x40, 0x0B},
	{0x41, 0x01},
	{0x42, 0x13},
	{0x43, 0x05},
	{0x44, 0xE0},
	{0x45, 0x30},
	{0x46, 0x30},
	{0x47, 0x00},
	{0x48, 0x00},
	{0x4B, 0x10},
	{0x50, 0x80},
	{0x51, 0x09},
	{0x52, 0x01},
	{0x53, 0x08},
	{0x54, 0x11},
	{0x56, 0x04},
	{0x57, 0x00},
	{0x5F, 0x00},
	{0x60, 0x00},
	{0x66, 0x00},
	{0x67, 0xC0},
	{0x68, 0x00},
	{0x69, 0x12},
	{0x6A, 0x1C},
	{0x6B, 0xE0},
	{0x6C, 0x22},
	{0x6D, 0xD0},
	{0x6E, 0x10},
	{0x6F, 0x10},
	{0x9C, 0x3D},
	{0x9E, 0x80},
	{0xD4, 0x00},
	{0xE9, 0x00},
	{0xEC, 0x30},
	{0xED, 0x34},
	{0xF6, 0x00},
	{0xF7, 0x16},
	{0xF8, 0x01},
	{0xF9, 0x00},
	{0xFA, 0x00},
	{0xFB, 0x40},
	{0xFC, 0x23},
	{0xFD, 0x24},   //SSPLL ANALOG CONTROL VCO Range control.(54 ~ 108MHz)

	{0xFF, 0x01},   //select page 1
	{0x01, 0xA3},
	{0x02, 0x48},
	{0x04, 0x00},
	{0x05, 0x00},
	{0x06, 0x03},
	{0x07, 0x02},
	{0x08, 0x12},
	{0x09, 0xF0},
	{0x0A, 0x0B},
	{0x0B, 0xD0},
	{0x0C, 0xCC},
	{0x0D, 0x00},
	{0x10, 0x00},
	{0x11, 0x5C},
	{0x12, 0x11},
	{0x13, 0x80},
	{0x14, 0x80},
	{0x15, 0x00},
	{0x17, 0x30},
	{0x18, 0x44},
	{0x1A, 0x10},
	{0x1B, 0x00},
	{0x1C, 0x27},
	{0x1D, 0x7F},
	{0x1E, 0x00},
	{0x1F, 0x00},
	{0x20, 0x50},
	{0x21, 0x22},
	{0x22, 0xF0},
	{0x23, 0xD8},
	{0x24, 0xBC},
	{0x25, 0xB8},
	{0x26, 0x44},
	{0x27, 0x38},
	{0x28, 0x00},
	{0x29, 0x00},
	{0x2A, 0x78},
	{0x2B, 0x44},
	{0x2C, 0x30},
	{0x2D, 0x14},
	{0x2E, 0xA5},
	//{0x2F, 0xE6}, // When FCS is set high or video loss condition is detected when LCS is set high, one of two colors display can be selected. - Blue color
	{0x2F, 0xE4},   //Black color
	{0x30, 0xC0},
	{0x31, 0x00},
	{0x32, 0x00},
	{0x33, 0x05},
	{0x34, 0x1A},
	{0x35, 0x00},
	{0x36, 0x03},
	{0x37, 0x28},
	{0x38, 0xAF},
	{0x40, 0x00},
	{0x41, 0x80},
	{0x42, 0x00},
	{0xC0, 0x01},
	{0xC1, 0xC7},
	{0xC2, 0xD2},
	{0xC3, 0x03},
	{0xC4, 0x5A},
	{0xC5, 0x00},
	{0xC6, 0x20},
	{0xC7, 0x04},
	{0xC8, 0x00},
	{0xC9, 0x06},
	{0xCA, 0x06},
	{0xCB, 0x16},
	{0xCC, 0x00},
	{0xCD, 0x54},
	{0xD0, 0x00},
	{0xD1, 0xF0},
	{0xD2, 0xF0},
	{0xD3, 0xF0},
	{0xD4, 0x20},
	{0xD5, 0x00},
	{0xD6, 0x10},
	{0xD7, 0x00},
	{0xD8, 0x00},
	{0xD9, 0x02},
	{0xDA, 0x80},
	{0xDB, 0x80},
	{0xDC, 0x10},
	{0xE0, 0x00},
	{0xE1, 0x05},
	{0xE2, 0x59},
	{0xE3, 0x37},
	{0xE4, 0x55},
	{0xE5, 0x55},
	{0xE6, 0x20},
	{0xE7, 0x2A},
	{0xE8, 0x0F},   // RGB input mode. Enable YOUT buffer. Enable V, C, Y channel anti-aliasing filter (decoder mode).
	{0xE9, 0x00},
	{0xEA, 0x03},
	{0xF6, 0xB0},   //CM_SLEEP unconditionally power down the entire common mode restore circuit
	{0xF7, 0x00},
	{0xF8, 0x00},
	{0xF9, 0x00},
	{0xFA, 0x38},

	{0xFF, 0x02},   //select page 2
	{0x01, 0x00},
	{0x02, 0x20},
	{0x03, 0x00},
	{0x04, 0x20},
	{0x05, 0x00},
	{0x06, 0x20},
	{0x07, 0x40},
	{0x08, 0x20},
	{0x09, 0x00},
	{0x0A, 0x04},
	{0x0B, 0x10},
	{0x0C, 0x20},
	{0x0D, 0x8d},
	{0x0E, 0x30},
	{0x0F, 0x10},
	{0x10, 0x30},
	{0x11, 0x20},
	{0x12, 0x03},
	{0x13, 0x09},
	{0x14, 0x0A},
	{0x15, 0x06},
	{0x16, 0xE0},
	{0x17, 0x01},
	{0x18, 0x40},
	{0x19, 0x0D},
	{0x1A, 0x00},
	{0x1B, 0x00},
	{0x1C, 0x12},
	{0x1D, 0xB4},
	{0x1E, 0x02},
	{0x20, 0x00},
	{0x21, 0x00},
	{0x40, 0x10},
	{0x41, 0x00},
	{0x42, 0x05},
	{0x43, 0x00},
	{0x44, 0x64},
	{0x45, 0x00},
	{0x46, 0x00},
	{0x47, 0x0A},
	{0x48, 0x36},
	{0x49, 0x10},
	{0x4A, 0x00},
	{0x4B, 0x00},
	{0x4C, 0x00},
	{0x4D, 0x00},
	{0x4E, 0x00},
	{0xBF, 0x0D},
	{0xE4, 0x00},
	{0xF8, 0x00},
	{0xF9, 0x80},

	{0xFF, 0x04},   //select page 4
	{0x79, 0x06},
	{0xC7, 0x90},
	{0xDC, 0x06},

	{0xFF, 0x05},   //select page 5
	{0x01, 0x10},
	{0x02, 0x03},
	{0x03, 0xE8},
	{0x05, 0x10},
	{0x06, 0x03},
	{0x07, 0x00},
	{0x08, 0x09},
	{0x2E, 0x01},
	{0x2F, 0x75},

	{0xFF, 0x06},   //select page 6
	{0x40, 0x0C},   //8bit
	{0x41, 0x00},
	{0x42, 0xC1},
	{0x43, 0xC0},
	{0x44, 0x0F},
	{0x45, 0xBF},
	{0x46, 0xBF},
	{0x47, 0x00},   //LVDS TX ICTL Output current control
	{0x48, 0x07},
	{0x49, 0x01},
	{0x4A, 0x00},
	{0x4B, 0x30},
	{0x4C, 0x40},
	{0x4D, 0x00},   //LVDS RX CONFIGURATION Termination resistor adjustment off
	{0x4E, 0x00},
	{0x4F, 0x00},
	{0xFF, 0x00},   //select page 0
	{REG_TERM, VAL_TERM}
};

static struct sensor_reg TW8836_CAM_OPEN[] = {
	{ 0xFF, 0x00 },
#if defined(FEATURE_SUPPORT_ATV_PROGRESSIVE)
	{ 0x07, 0x08 },
#else
	{ 0x07, 0x0A },
#endif
	{ REG_TERM, VAL_TERM }
};

static struct sensor_reg TW8836_CAM_CLOSE[] = {
	{ 0xFF, 0x00 },
#if defined(FEATURE_SUPPORT_ATV_PROGRESSIVE)
	{ 0x07, 0x02 },
#else
	{ 0x07, 0x02 },
#endif
	{ REG_TERM, VAL_TERM },
};

static struct sensor_reg TW8836_NTSC_DataSet[]   = {
	{ REG_TERM, VAL_TERM },
};

static struct sensor_reg TW8836_CCIR_NTSC_DataSet[] = {
	{ REG_TERM, VAL_TERM }
};

static struct sensor_reg TW8836_480P_YPbPr[] = {
	{ REG_TERM, VAL_TERM }
};

struct sensor_reg TW8836_576P_YPbPr[] = {
	{ REG_TERM, VAL_TERM }
};

static struct sensor_reg TW8836_NTSC_Int[] = {
	{ REG_TERM, VAL_TERM }
};

static struct sensor_reg TW8836_NTSC_Pro[] = {
	{ REG_TERM, VAL_TERM }
};

static struct sensor_reg TW8836_PAL_Int[] = {
	{ REG_TERM, VAL_TERM }
};

static struct sensor_reg TW8836_PAL_Pro[] = {
	{ REG_TERM, VAL_TERM }
};

struct sensor_reg *tw8836_sensor_reg_common[] = {
	TW8836_CAM_INIT,
	TW8836_CAM_OPEN,
	TW8836_CAM_CLOSE,
};

static struct sensor_reg sensor_brightness_p0[] = {
	{ REG_TERM, VAL_TERM }
};

struct sensor_reg *tw8836_sensor_reg_brightness[] = {
	sensor_brightness_p0,
};

static struct sensor_reg sensor_contrast_p0[] = {
	{ REG_TERM, VAL_TERM }
};

struct sensor_reg *tw8836_sensor_reg_contrast[] = {
	sensor_contrast_p0,
};

static struct sensor_reg sensor_saturation_p0[] = {
	{ REG_TERM, VAL_TERM }
};

struct sensor_reg *tw8836_sensor_reg_saturation[] = {
	sensor_saturation_p0,
};

static struct sensor_reg sensor_hue_p0[] = {
	{ REG_TERM, VAL_TERM }
};

struct sensor_reg *tw8836_sensor_reg_hue[21] = {
	sensor_hue_p0,
};

static struct sensor_reg tw8836_sensor_reg_backup[] = {
	{0xFF, 0x00},   //select page 0
	{0x02, 0x00},
	{0x03, 0xFE},
	{0x04, 0x01},
	{0x06, 0x06},
	{0x07, 0x02},
	{0x08, 0x86},
	{0x0F, 0x00},
	{0x1F, 0x00},
	{0x40, 0x0B},
	{0x41, 0x01},
	{0x42, 0x13},
	{0x43, 0x05},
	{0x44, 0xE0},
	{0x45, 0x30},
	{0x46, 0x30},
	{0x47, 0x00},
	{0x48, 0x00},
	{0x4B, 0x10},
	{0x50, 0x80},
	{0x51, 0x09},
	{0x52, 0x01},
	{0x53, 0x08},
	{0x54, 0x11},
	{0x56, 0x04},
	{0x57, 0x00},
	{0x5F, 0x00},
	{0x60, 0x00},
	{0x66, 0x00},
	{0x67, 0xC0},
	{0x68, 0x00},
	{0x69, 0x12},
	{0x6A, 0x1C},
	{0x6B, 0xE0},
	{0x6C, 0x22},
	{0x6D, 0xD0},
	{0x6E, 0x10},
	{0x6F, 0x10},
	{0x9C, 0x3D},
	{0x9E, 0x80},
	{0xD4, 0x00},
	{0xE9, 0x00},
	{0xEC, 0x30},
	{0xED, 0x34},
	{0xF6, 0x00},
	{0xF7, 0x16},
	{0xF8, 0x01},
	{0xF9, 0x00},
	{0xFA, 0x00},
	{0xFB, 0x40},
	{0xFC, 0x23},
	{0xFD, 0x24},   //SSPLL ANALOG CONTROL VCO Range control.(54 ~ 108MHz)

	{0xFF, 0x01},   //select page 1
	{0x01, 0xA3},
	{0x02, 0x48},
	{0x04, 0x00},
	{0x05, 0x00},
	{0x06, 0x03},
	{0x07, 0x02},
	{0x08, 0x12},
	{0x09, 0xF0},
	{0x0A, 0x0B},
	{0x0B, 0xD0},
	{0x0C, 0xCC},
	{0x0D, 0x00},
	{0x10, 0x00},
	{0x11, 0x5C},
	{0x12, 0x11},
	{0x13, 0x80},
	{0x14, 0x80},
	{0x15, 0x00},
	{0x17, 0x30},
	{0x18, 0x44},
	{0x1A, 0x10},
	{0x1B, 0x00},
	{0x1C, 0x27},
	{0x1D, 0x7F},
	{0x1E, 0x00},
	{0x1F, 0x00},
	{0x20, 0x50},
	{0x21, 0x22},
	{0x22, 0xF0},
	{0x23, 0xD8},
	{0x24, 0xBC},
	{0x25, 0xB8},
	{0x26, 0x44},
	{0x27, 0x38},
	{0x28, 0x00},
	{0x29, 0x00},
	{0x2A, 0x78},
	{0x2B, 0x44},
	{0x2C, 0x30},
	{0x2D, 0x14},
	{0x2E, 0xA5},
	//{0x2F, 0xE6}, // When FCS is set high or video loss condition is detected when LCS is set high, one of two colors display can be selected. - Blue color
	{0x2F, 0xE4},   //Black color
	{0x30, 0xC0},
	{0x31, 0x00},
	{0x32, 0x00},
	{0x33, 0x05},
	{0x34, 0x1A},
	{0x35, 0x00},
	{0x36, 0x03},
	{0x37, 0x28},
	{0x38, 0xAF},
	{0x40, 0x00},
	{0x41, 0x80},
	{0x42, 0x00},
	{0xC0, 0x01},
	{0xC1, 0xC7},
	{0xC2, 0xD2},
	{0xC3, 0x03},
	{0xC4, 0x5A},
	{0xC5, 0x00},
	{0xC6, 0x20},
	{0xC7, 0x04},
	{0xC8, 0x00},
	{0xC9, 0x06},
	{0xCA, 0x06},
	{0xCB, 0x16},
	{0xCC, 0x00},
	{0xCD, 0x54},
	{0xD0, 0x00},
	{0xD1, 0xF0},
	{0xD2, 0xF0},
	{0xD3, 0xF0},
	{0xD4, 0x20},
	{0xD5, 0x00},
	{0xD6, 0x10},
	{0xD7, 0x00},
	{0xD8, 0x00},
	{0xD9, 0x02},
	{0xDA, 0x80},
	{0xDB, 0x80},
	{0xDC, 0x10},
	{0xE0, 0x00},
	{0xE1, 0x05},
	{0xE2, 0x59},
	{0xE3, 0x37},
	{0xE4, 0x55},
	{0xE5, 0x55},
	{0xE6, 0x20},
	{0xE7, 0x2A},
	{0xE8, 0x0F},   // RGB input mode. Enable YOUT buffer. Enable V, C, Y channel anti-aliasing filter (decoder mode).
	{0xE9, 0x00},
	{0xEA, 0x03},
	{0xF6, 0xB0},   //CM_SLEEP unconditionally power down the entire common mode restore circuit
	{0xF7, 0x00},
	{0xF8, 0x00},
	{0xF9, 0x00},
	{0xFA, 0x38},

	{0xFF, 0x02},   //select page 2
	{0x01, 0x00},
	{0x02, 0x20},
	{0x03, 0x00},
	{0x04, 0x20},
	{0x05, 0x00},
	{0x06, 0x20},
	{0x07, 0x40},
	{0x08, 0x20},
	{0x09, 0x00},
	{0x0A, 0x04},
	{0x0B, 0x10},
	{0x0C, 0x20},
	{0x0D, 0x8d},
	{0x0E, 0x30},
	{0x0F, 0x10},
	{0x10, 0x30},
	{0x11, 0x20},
	{0x12, 0x03},
	{0x13, 0x09},
	{0x14, 0x0A},
	{0x15, 0x06},
	{0x16, 0xE0},
	{0x17, 0x01},
	{0x18, 0x40},
	{0x19, 0x0D},
	{0x1A, 0x00},
	{0x1B, 0x00},
	{0x1C, 0x12},
	{0x1D, 0xB4},
	{0x1E, 0x02},
	{0x20, 0x00},
	{0x21, 0x00},
	{0x40, 0x10},
	{0x41, 0x00},
	{0x42, 0x05},
	{0x43, 0x00},
	{0x44, 0x64},
	{0x45, 0x00},
	{0x46, 0x00},
	{0x47, 0x0A},
	{0x48, 0x36},
	{0x49, 0x10},
	{0x4A, 0x00},
	{0x4B, 0x00},
	{0x4C, 0x00},
	{0x4D, 0x00},
	{0x4E, 0x00},
	{0xBF, 0x0D},
	{0xE4, 0x00},
	{0xF8, 0x00},
	{0xF9, 0x80},

	{0xFF, 0x04},   //select page 4
	{0x79, 0x06},
	{0xC7, 0x90},
	{0xDC, 0x06},

	{0xFF, 0x05},   //select page 5
	{0x01, 0x10},
	{0x02, 0x03},
	{0x03, 0xE8},
	{0x05, 0x10},
	{0x06, 0x03},
	{0x07, 0x00},
	{0x08, 0x09},
	{0x2E, 0x01},
	{0x2F, 0x75},

	{0xFF, 0x06},   //select page 6
	{0x40, 0x0C},   //8bit
	{0x41, 0x00},
	{0x42, 0xC1},
	{0x43, 0xC0},
	{0x44, 0x0F},
	{0x45, 0xBF},
	{0x46, 0xBF},
	{0x47, 0x00},   //LVDS TX ICTL Output current control
	{0x48, 0x07},
	{0x49, 0x01},
	{0x4A, 0x00},
	{0x4B, 0x30},
	{0x4C, 0x40},
	{0x4D, 0x00},   //LVDS RX CONFIGURATION Termination resistor adjustment off
	{0x4E, 0x00},
	{0x4F, 0x00},
	{0xFF, 0x00},   //select page 0
	{ REG_TERM, VAL_TERM }
};

static struct sensor_reg tw8836_sensor_path_rear_camera[] = {
	{ REG_TERM, VAL_TERM }
};

static struct sensor_reg tw8836_sensor_path_aux_ntsc[] = {
	{ REG_TERM, VAL_TERM }
};

static struct sensor_reg tw8836_sensor_path_aux_pal[] = {
	{ REG_TERM, VAL_TERM }
};

struct sensor_reg *sensor_reg_path_tw8836[] = {
	tw8836_sensor_path_rear_camera,
	tw8836_sensor_path_aux_ntsc,
	tw8836_sensor_path_aux_pal
};

const int TW8836_BRIGHTNESS_LIMITS[]			= {0, 255};
const int TW8836_CONTRAST_LIMITS[]				= {0, 255};
const int TW8836_HUE_LIMITS[]					= {0, 90 };
const int TW8836_SATURATION_LIMITS[]			= {0, 255};
const int TW8836_CAM_BRIGHTNESS_LIMITS[]		= {-128, 127};
const int TW8836_CAM_CONTRAST_LIMITS[]			= {0, 255};
const int TW8836_CAM_HUE_LIMITS[]				= {0, 255};
const int TW8836_CAM_SATURATION_LIMITS[]		= {0, 255};

static const unsigned char I2C_ADDRESSES_HUE[] =
		{TW8836_I2C_HUE, };

static const unsigned char I2C_ADDRESSES_CONTRAST[] =
		{TW8836_I2C_CONTRAST_R, TW8836_I2C_CONTRAST_G, TW8836_I2C_CONTRAST_B, };

static const unsigned char I2C_ADDRESSES_SATURATION[] =
		{TW8836_I2C_SATURTION_CB, TW8836_I2C_SATURTION_CR, };

static const unsigned char I2C_ADDRESSES_BRIGHTNESS[] =
		{TW8836_I2C_BRIGHTNESS_R, TW8836_I2C_BRIGHTNESS_G, TW8836_I2C_BRIGHTNESS_B, };

static const unsigned char I2C_ADDRESSES_CAM_HUE[] =
		{TW8836_I2C_CAM_HUE, };

static const unsigned char I2C_ADDRESSES_CAM_CONTRAST[] =
		{TW8836_I2C_CAM_CONTRAST, };

static const unsigned char I2C_ADDRESSES_CAM_SATURATION[] =
		{TW8836_I2C_CAM_SATURATION_U, TW8836_I2C_CAM_SATURATION_V, };

static const unsigned char I2C_ADDRESSES_CAM_BRIGHTNESS[] =
		{TW8836_I2C_CAM_BRIGHTNESS, };

static int get_size(int cmd);
static int tw8836_read(int reg);
static int tw8836_read_value(unsigned char page,
		unsigned address_size, const unsigned char* addresses, char *ret_data);
static int tw8836_write(unsigned char* data, unsigned char reg_bytes, unsigned char data_bytes);
static int tw8836_write_value(unsigned char page,
		unsigned address_size, const unsigned char* addresses, char value);
static int is_matched_data(int size, char *data);

static int write_regs(const struct sensor_reg reglist[])
{
	int err = 0, err_cnt = 0, backup_cnt = 0, i2c_err_count = 0;
	unsigned char data[132];
	unsigned char bytes;
	const struct sensor_reg *next = reglist;

	while (!((next->reg == REG_TERM) && (next->val == VAL_TERM))) {
		bytes = 0;
		data[bytes]= (u8)next->reg & 0xff; 	bytes++;
		data[bytes]= (u8)next->val & 0xff; 	bytes++;

		while (i2c_err_count < I2C_RETRY_COUNT)
		{
			err = DDI_I2C_Write(data, 1, 1);
			if (err)
			{
				i2c_err_count++;
				VPRINTK("%s I2C error count: %d\n", __func__, i2c_err_count);
			}
			else
			{
				i2c_err_count = 0;
				break;
			}
		}

		if (err) {
			err_cnt++;
			if (err_cnt >= 3) {
				VPRINTK("%s ERROR: Sensor I2C !!!!\n", __func__);
				return err;
			}
		} else {
			for (backup_cnt = 0; backup_cnt < ARRAY_SIZE(tw8836_sensor_reg_backup); backup_cnt++) {
				if (tw8836_sensor_reg_backup[backup_cnt].reg == next->reg) {
					tw8836_sensor_reg_backup[backup_cnt].val = next->val;
				}
			}

			err_cnt = 0;
			next++;
		}
	}

	return 0;
}

static void tw8836_sensor_get_preview_size(int *width, int *height)
{
	*width = PRV_W;
	*height = PRV_H;
}

static void tw8836_sensor_get_capture_size(int *width, int *height)
{
	*width = CAP_W;
	*height = CAP_H;
}

static int sensor_open(eTW_YSEL yin)
{
	VPRINTK("%s\n", __func__);

	return write_regs(tw8836_sensor_reg_common[MODE_TW8836_OPEN]);
}

static int sensor_close(void)
{
	VPRINTK("%s\n", __func__);

	return write_regs(tw8836_sensor_reg_common[MODE_TW8836_CLOSE]);
}

static int sensor_capture(void)
{
	return 0;
}

static int sensor_capturecfg(int width, int height)
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
	char ret = 0;

	/*
	DDI_I2C_Read(0x01, 1, &ret, 1);
	if ((ret & 0x80) == 0x00)  {
		ret = 0; // detected video signal
	} else {
		ret = 1; // not detected video signal
	}
	*/
	return ret;
}

static int sensor_checkNoise(void)
{
	char ret = 0;

	return ret;
}

static int sensor_resetVideoDecoder(void)
{
/**
 * @author sjpark@cleinsoft
 * @date 2013/10/15
 * Do not use hw reset
 **/
#if 0
	sensor_reset_low();
	sensor_delay(10);

	sensor_reset_high();
	sensor_delay(10);
	return write_regs(tw8836_sensor_reg_backup);
#endif

	return 0;
}

static int sensor_setPath(int val)
{
	//return write_regs(sensor_reg_path[val]);
	return 0;
}

static int sensor_getVideoFormat(void)
{
	char ret = 0;

#if 0
	DDI_I2C_Read(0x01, 1, &ret, 1);
	if ((ret & 0x01) == 0x00)  {
		ret = 0; // NTSC
	} else {
		ret = 1; // PAL
	}
#endif

	return ret;
}

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

static int sensor_check_esd(int val)
{
	return 0;
}

static int sensor_check_luma(int val)
{
	return 0;
}

static int get_size(int cmd)
{
	switch(cmd)
	{
		case SET_BRIGHTNESS:
		case GET_BRIGHTNESS:
			return ARRAY_SIZE(I2C_ADDRESSES_BRIGHTNESS);

		case SET_CONTRAST:
		case GET_CONTRAST:
			return ARRAY_SIZE(I2C_ADDRESSES_CONTRAST);

		case GET_HUE:
		case SET_HUE:
			return ARRAY_SIZE(I2C_ADDRESSES_HUE);

		case GET_SATURATION:
		case SET_SATURATION:
			return ARRAY_SIZE(I2C_ADDRESSES_SATURATION);

		case GET_CAM_BRIGHTNESS:
		case SET_CAM_BRIGHTNESS:
			return ARRAY_SIZE(I2C_ADDRESSES_CAM_BRIGHTNESS);

		case GET_CAM_CONTRAST:
		case SET_CAM_CONTRAST:
			return ARRAY_SIZE(I2C_ADDRESSES_CAM_CONTRAST);

		case GET_CAM_HUE:
		case SET_CAM_HUE:
			return ARRAY_SIZE(I2C_ADDRESSES_CAM_HUE);

		case GET_CAM_SATURATION:
		case SET_CAM_SATURATION:
			return ARRAY_SIZE(I2C_ADDRESSES_CAM_SATURATION);
	}
	return 0;
}

static int is_matched_data(int size, char *data)
{
	int i;

	if (size < 1)
		return FAIL;
	else if (size == 1)
		return data[0];

	for (i = 0; i < size -1; i++)
	{
		if (data[i] != data[i + 1])
		{
			return FAIL_UNMATCH_SETTING;
		}
	}

	return 1;
}

static int tw8836_read(int reg)
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

static int tw8836_read_value(unsigned char page,
		unsigned address_size, const unsigned char* addresses, char *ret_data)
{
	unsigned char data[] = {TW8836_MOVE_PAGE_ADDRESS, page};
	int i;
	int ret = 0;

	if (addresses == NULL || ret_data == NULL)
	{
		VPRINTK("%s args NULL\n", __func__);
		return FAIL;
	}

	ret = tw8836_write(data, 1, 1);
	if (ret != SUCCESS_I2C)
		return FAIL_I2C;

	for (i = 0; i < address_size; i++)
	{
		unsigned char retval = tw8836_read(addresses[i]);
		if (retval != FAIL_I2C)
		{
			ret_data[i] = (unsigned char)retval;
		}
		else
		{
			return FAIL_I2C;
		}
	}

	//Ignore whether the page has been moved 0.
	data[0] = TW8836_MOVE_PAGE_ADDRESS;
	data[1] = 0x00;
	tw8836_write(data, 1, 1);

	return SUCCESS_I2C;
}

static int tw8836_write(unsigned char* data, unsigned char reg_bytes, unsigned char data_bytes)
{
	int i = I2C_RETRY_COUNT;
	do {
		int ret = DDI_I2C_Write(data, reg_bytes, data_bytes);
		if (ret == 0)
			//success to I2C.
			return SUCCESS_I2C;

		i--;
	} while(i > 0);

	return FAIL_I2C;
}

static int tw8836_write_value(unsigned char page,
		unsigned address_size, const unsigned char* addresses, char value)
{
	unsigned char data[] = {TW8836_MOVE_PAGE_ADDRESS, page, };
	int ret = tw8836_write(data, 1, 1);
	int i;
	if (ret != SUCCESS_I2C)
		//failed to move page.
		return FAIL_I2C;

	data[1] = value;
	for (i = 0; i < address_size; i++)
	{
		data[0] = addresses[i];
		ret = tw8836_write(data, 1, 1);
		if (ret != SUCCESS_I2C)
			return FAIL_I2C;
	}

	//Ignore whether the page has been moved 0.
	data[0] = TW8836_MOVE_PAGE_ADDRESS;
	data[1] = 0x00;
	tw8836_write(data, 1, 1);

	return SUCCESS_I2C;
}

int tw8836_write_ie(int cmd, unsigned char value)
{
	int ret = FAIL;
	int size = get_size(cmd);
	int page;

	if (size < 1)
		return ret;

	page = cmd < SET_CAM_BRIGHTNESS ? TW8836_IMAGE_ENHANCEMENT_PAGE :
		TW8836_CAM_IMAGE_ENHANCEMENT_PAGE;
	switch (cmd)
	{
		case SET_BRIGHTNESS:
			ret = tw8836_write_value(page, size, I2C_ADDRESSES_BRIGHTNESS, value);
			break;
		case SET_CONTRAST:
			ret = tw8836_write_value(page, size, I2C_ADDRESSES_CONTRAST, value);
			break;
		case SET_HUE:
			ret = tw8836_write_value(page, size, I2C_ADDRESSES_HUE, value);
			break;
		case SET_SATURATION:
			ret = tw8836_write_value(page, size, I2C_ADDRESSES_SATURATION, value);
			break;
		case SET_CAM_BRIGHTNESS:
			ret = tw8836_write_value(page, size, I2C_ADDRESSES_CAM_BRIGHTNESS, value);
			break;
		case SET_CAM_CONTRAST:
			ret = tw8836_write_value(page, size, I2C_ADDRESSES_CAM_CONTRAST, value);
			break;
		case SET_CAM_HUE:
			ret = tw8836_write_value(page, size, I2C_ADDRESSES_CAM_HUE, value);
			break;
		case SET_CAM_SATURATION:
			ret = tw8836_write_value(page, size, I2C_ADDRESSES_CAM_SATURATION, value);
			break;
	}

	return ret;
}

int tw8836_read_ie(int cmd, char *level)
{
	int ret = FAIL;
	int page;
	int size = get_size(cmd);
	//char data[size];
	char *data = kmalloc(size, GFP_KERNEL);

	//invalid cmd
	if (size < 1)
		return ret;

	page = cmd < GET_CAM_BRIGHTNESS ? TW8836_IMAGE_ENHANCEMENT_PAGE :
					TW8836_CAM_IMAGE_ENHANCEMENT_PAGE;

	switch (cmd)
	{
		case GET_BRIGHTNESS:
			ret = tw8836_read_value(page, size, I2C_ADDRESSES_BRIGHTNESS, data);
			break;
		case GET_CONTRAST:
			ret = tw8836_read_value(page, size, I2C_ADDRESSES_CONTRAST, data);
			break;
		case GET_HUE:
			ret = tw8836_read_value(page, size, I2C_ADDRESSES_HUE, data);
			break;
		case GET_SATURATION:
			ret = tw8836_read_value(page, size, I2C_ADDRESSES_SATURATION, data);
			break;
		case GET_CAM_BRIGHTNESS:
			ret = tw8836_read_value(page, size, I2C_ADDRESSES_CAM_BRIGHTNESS, data);
			break;
		case GET_CAM_CONTRAST:
			ret = tw8836_read_value(page, size, I2C_ADDRESSES_CAM_CONTRAST, data);
			break;
		case GET_CAM_HUE:
			ret = tw8836_read_value(page, size, I2C_ADDRESSES_CAM_HUE, data);
			break;
		case GET_CAM_SATURATION:
			ret = tw8836_read_value(page, size, I2C_ADDRESSES_CAM_SATURATION, data);
			break;
	}

	if(is_matched_data(size, data))
	{
		*level = data[0];
		ret = 1;
	}

	kfree(data);

	return ret;
}

int tw8836_init(void)
{
	VPRINTK("%s\n", __func__);

	return 0;
}

int tw8836_read_id(void)
{
	int id = 0;
	unsigned char data[2] = {TW8836_MOVE_PAGE_ADDRESS, 0x00};
	int ret = tw8836_write(data, 1, 1);

	VPRINTK("%s\n", __func__);

	if (ret != 0)
		return FAIL_I2C;

	id = tw8836_read(0x00);
	VPRINTK("%s tw8836 id: 0x%x\n", __func__, id);

	tw8836_write(data, 1, 1);

	if (id != FAIL_I2C)
		return id;
	else
		return FAIL_I2C;
}

static int sensor_preview(void)
{
	if (debug_tw8836)
		return tw8836_read_id();
	else
		return 0;
}

int tw8836_check_video_signal(unsigned long arg)
{
	int ret = 0;
	tw_cstatus *tw8836_cstatus;
	int cstatus = 0;

	unsigned char data[2] = {TW8836_MOVE_PAGE_ADDRESS, TW8836_ADDRESS_CSTATUS_PAGE};
	ret = tw8836_write(data, 1, 1);
	if (ret != SUCCESS_I2C)
		return ret;

	tw8836_cstatus = (tw_cstatus *)arg;
	cstatus = tw8836_read(TW8836_ADDRESS_CSTATUS_REG);

#if defined(FEATURE_SUPPORT_ATV_PROGRESSIVE)
	cstatus = 0x68;
#endif

	if (cstatus >= FAIL)
		//error
		return cstatus;

	tw8836_cstatus->vdloss = (char)cstatus & VDLOSS ? 1 : 0;
	tw8836_cstatus->hlock = (char)cstatus & HLOCK ? 1 : 0;
	tw8836_cstatus->slock = (char)cstatus & VLOCK ? 1 : 0;
	tw8836_cstatus->vlock = (char)cstatus & SLOCK ? 1 : 0;
	tw8836_cstatus->mono = (char)cstatus & MONO ? 1 : 0;

	data[1] = 0x00;
	//move page 0
	tw8836_write(data, 1, 1);

	ret = 1;
	VPRINTK("%s cstatus:0x%x vdloss:0x%x, hlock:0x%x, vlock:0x%x, slock:0x%x, mono:0x%x\n",
			__func__, cstatus, tw8836_cstatus->vdloss, tw8836_cstatus->hlock, 
			tw8836_cstatus->vlock, tw8836_cstatus->slock, tw8836_cstatus->mono);

	return ret;
}

static struct sensor_reg TW8836_display_mute_on[] = {
	{ 0xFF, 0x02 },
	{ 0x1E, 0x03 },
	{ 0xFF, 0x00 },
	{ REG_TERM, VAL_TERM },
};

static struct sensor_reg TW8836_display_mute_off[] = {
	{ 0xFF, 0x02 },
	{ 0x1E, 0x02 },
	{ 0xFF, 0x00 },
	{ REG_TERM, VAL_TERM },
};

int tw8836_display_mute(int mute)
{
	int ret = FAIL;
	struct sensor_reg *mute_reg;

	VPRINTK("%s mute: %d\n", __func__, mute);

	if (mute)
	{
		mute_reg = (struct sensor_reg *)&TW8836_display_mute_on;
	}
	else
	{
		mute_reg = (struct sensor_reg *)&TW8836_display_mute_off;
	}

	ret = write_regs(mute_reg);

	return ret;
}

void tw8836_close_cam(void)
{
	VPRINTK("%s\n", __func__);
	sensor_close();
}

void tw8836_sensor_init_fnc(SENSOR_FUNC_TYPE_DAUDIO *sensor_func)
{
	sensor_func->sensor_open				= sensor_open;
	sensor_func->sensor_close				= sensor_close;

	sensor_func->sensor_preview				= sensor_preview;
	sensor_func->sensor_capture				= sensor_capture;
	sensor_func->sensor_capturecfg			= sensor_capturecfg;
	sensor_func->sensor_reset_video_decoder	= sensor_resetVideoDecoder;
	sensor_func->sensor_init				= tw8836_init;
	sensor_func->sensor_read_id				= tw8836_read_id;
	sensor_func->sensor_check_video_signal	= tw8836_check_video_signal;
	sensor_func->sensor_display_mute		= tw8836_display_mute;
	sensor_func->sensor_close_cam			= tw8836_close_cam;
	sensor_func->sensor_write_ie			= tw8836_write_ie;
	sensor_func->sensor_read_ie				= tw8836_read_ie;
	sensor_func->sensor_get_preview_size	= tw8836_sensor_get_preview_size;
	sensor_func->sensor_get_capture_size	= tw8836_sensor_get_capture_size;

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

	sensor_func->Set_Path				= sensor_setPath;
	sensor_func->Get_videoFormat		= sensor_getVideoFormat;
	sensor_func->Set_writeRegister		= sensor_writeRegister;
	sensor_func->Get_readRegister		= sensor_readRegister;

	sensor_func->Check_ESD				= NULL;
	sensor_func->Check_Luma				= NULL;
}
