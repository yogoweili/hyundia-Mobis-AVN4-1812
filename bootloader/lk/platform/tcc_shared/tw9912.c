#include <tw9912.h>
#include <stdlib.h>
#include <stdio.h>
#include <daudio_ie.h>

#include <dev/gpio.h>
#include <tcc_lcd.h>

#if defined(CONFIG_DAUDIO_PRINT_DEBUG_MSG)
#define DEBUG_TW9912	1
#else
#define DEBUG_TW9912	0
#endif

#if (DEBUG_TW9912)
#define LKPRINT(fmt, args...) dprintf(INFO, "[cleinsoft tw9912] " fmt, ##args)
#else
#define LKPRINT(args...) do {} while(0)
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

static struct TW9912_REG TW9912_INIT_REGS[] =
{
	//james.kang@zentech.co.kr 140923
	//cmd, value
	{ 0x01, 0x68 },
	{ 0x02, 0x44 },
	{ 0x03, 0x27 },
	{ 0x04, 0x00 },
	{ 0x05, 0x0D },
	{ 0x06, 0x03 },
	{ 0x07, 0x02 },
//	{ 0x08, 0x16 },
	{ 0x08, 0x13 }, // vertical line 
	{ 0x09, 0xF0 },
	{ 0x0A, 0x10 },
	{ 0x0B, 0xD0 },
	{ 0x0C, 0xCC },
	{ 0x0D, 0x15 },
	{ 0x10, 0x00 },
	{ 0x11, 0x64 },
	{ 0x12, 0x11 },
	{ 0x13, 0x80 },
	{ 0x14, 0x80 },
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
//	{ 0x2F, 0x26 },	// blue color
	{ 0x2F, 0X24 },	// black color
	{ 0x30, 0x00 },
	{ 0x31, 0x10 },
	{ 0x32, 0x00 },
//	{ 0x33, 0x05 },
	{ 0x33, 0x85 }, // FreeRun 60Hz
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
//	{ 0xE9, 0x14 }, pclk falling
	{ 0xE9, 0x34 },	// pclk rising
};

static struct TW9912_REG TW9912_ANALOG_INPUT_OPEN_REGS[] =
{
	{ 0x03, 0x21 },
};

static struct TW9912_REG TW9912_ANALOG_INPUT_CLOSE_REGS[] =
{
	{ 0x03, 0x27 },
};

static int tw9912_i2c_read(unsigned char cmd, unsigned char *recv_data)
{
	int ret = -1, count = TW9912_I2C_RETRY_COUNT;

	do {
		ret = i2c_xfer(SLAVE_ADDRESS_TW9912, 1, &cmd, 1, recv_data, I2C_CH_TW9912);
		if (ret == 0)
			return ret;
		else
			count--;

	} while(count > 0);

	return ret;
}

static int tw9912_i2c_write(unsigned char cmd, unsigned char value)
{
	unsigned char send_data[2];
	int ret = -1, count = TW9912_I2C_RETRY_COUNT;
	send_data[0] = cmd;
	send_data[1] = value;

	do {
		ret = i2c_xfer(SLAVE_ADDRESS_TW9912, 2, send_data, 0, 0, I2C_CH_TW9912);
		if (ret == 0)
			return ret;
		else
			count--;

	} while(count > 0);

	return ret;
}

int tw9912_set_ie(int mode, unsigned char value)
{
	int ret = FAIL;

	if (value == 0) return ret;

	switch (mode)
	{
		case SET_CAM_BRIGHTNESS:
			value -= 127;
			ret = tw9912_i2c_write(TW9912_I2C_BRIGHTNESS, value);
			break;

		case SET_CAM_CONTRAST:
			ret = tw9912_i2c_write(TW9912_I2C_CONTRAST, value);
			break;

		case SET_CAM_HUE:
			value -= 127;
			ret = tw9912_i2c_write(TW9912_I2C_HUE, value);
			break;

		case SET_CAM_SATURATION:
			ret = tw9912_i2c_write(TW9912_I2C_SATURATION_U, value);
			if (ret == 0)
				ret = tw9912_i2c_write(TW9912_I2C_SATURATION_V, value);
			break;
	}

	return ret;
}

void tw9912_init()
{
	int i, ret = 0;
	unsigned char *read_data = 0;
	const int size = ARRAY_SIZE(TW9912_INIT_REGS);

	//TW9912 reset
	gpio_set(TW9912_RESET_GPIO, 0);
	gpio_set(TW9912_RESET_GPIO, 1);
	lcd_delay_us(10);

	//read id
	ret = tw9912_i2c_read(0x00, read_data);
	if (ret == 0)
	{
		LKPRINT("%s TW9912 id: 0x%x \n", __func__, *read_data);
	}
	else
	{
		LKPRINT("%s failed to read id.\n", __func__);
	}

	for (i = 0; i < size; i++)
	{
		const unsigned char cmd = TW9912_INIT_REGS[i].cmd;
		const unsigned char value = TW9912_INIT_REGS[i].value;

		ret = tw9912_i2c_write(cmd, value);
	}

	LKPRINT("%s finished.\n", __func__);
}
