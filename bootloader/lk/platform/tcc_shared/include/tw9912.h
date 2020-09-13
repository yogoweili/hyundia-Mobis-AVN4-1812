#ifndef __TW9912__
#define __TW9912__

#include <i2c.h>
#include <platform/gpio.h>

#include <daudio_settings.h>

#define TW9912_RESET_GPIO		TCC_GPD(17)

#define SLAVE_ADDRESS_TW9912 	0x88
#define I2C_CH_TW9912 			I2C_CH_MASTER3

#define TW9912_SUCCESS_					1
#define TW9912_SUCCESS_I2C				2
#define TW9912_SUCCESS_WRITE_SETTING	3

#define TW9912_FAIL						300
#define TW9912_FAIL_I2C					301
#define TW9912_FAIL_READ_SETTING		302
#define TW9912_FAIL_WRITE_SETTING		303

#define TW9912_I2C_RETRY_COUNT			3

#define TW9912_I2C_BRIGHTNESS		0x10
#define TW9912_I2C_CONTRAST			0x11
#define TW9912_I2C_HUE				0x15
#define TW9912_I2C_SATURATION_U		0x13
#define TW9912_I2C_SATURATION_V		0x14

struct TW9912_REG
{
	unsigned char cmd;
	unsigned char value;
};

void tw9912_init();
int tw9912_set_ie(int mode, unsigned char value);

#endif
