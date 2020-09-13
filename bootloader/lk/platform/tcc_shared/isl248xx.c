#include <isl248xx.h>
#include <stdlib.h>
#include <daudio_ver.h>

#if defined(CONFIG_DAUDIO_PRINT_DEBUG_MSG)
#define DEBUG_ISL			1
#define DEBUG_ISL_DETAIL	0
#else
#define DEBUG_ISL			0
#define DEBUG_ISL_DETAIL	0
#endif

#if (DEBUG_ISL)
#define LKPRINT(fmt, args...) dprintf(INFO, "[cleinsoft ISL248xx] " fmt, ##args)
#else
#define LKPRINT(args...) do {} while(0)
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define ISL24813_STANDARD_MODE
//#define ISL24813_REGISTER_MODE	//D-Audio H/W not support.

#if defined(ISL24813_STANDARD_MODE)
//I2C standard mode
static unsigned char ISL24813_INIT_REGS[] =
{
	//2014. 06. 02
	0x00, 0x00,
	0x03, 0xE3,
	0x03, 0xB6,
	0x03, 0x40,
	0x03, 0x11,
	0x02, 0xDC,
	0x02, 0xA9,
	0x02, 0x7E,
	0x02, 0x20,
	0x02, 0x1E,
	0x01, 0xEC,
	0x01, 0xE1,
	0x01, 0x7F,
	0x01, 0x57,
	0x01, 0x23,
	0x00, 0xEE,
	0x00, 0xC0,
	0x00, 0x4C,
	0x00, 0x30,
};
#else
//I2C register mode
static ISL248XX_REG ISL24813_INIT_REGS[] =
{
	{0x00, {0x00, 0x06}},
	{0x02, {0x03, 0xCD}},
	{0x04, {0x03, 0xA2}},
	{0x06, {0x03, 0x2E}},
	{0x08, {0x02, 0xFF}},
	{0x0A, {0x02, 0xCD}},
	{0x0C, {0x02, 0x9B}},
	{0x0E, {0x02, 0x71}},
	{0x10, {0x02, 0x16}},
	{0x12, {0x02, 0x14}},
	{0x14, {0x01, 0xE3}},
	{0x16, {0x01, 0xD7}},
	{0x18, {0x01, 0x77}},
	{0x1A, {0x01, 0x51}},
	{0x1C, {0x01, 0x1E}},
	{0x1E, {0x00, 0xEB}},
	{0x20, {0x00, 0xBC}},
	{0x22, {0x00, 0x4A}},
	{0x24, {0x00, 0x30}},
};
#endif

static ISL248XX_REG ISL24833_INIT_REGS[] =
{
	{0x00, {0x00, 0x00}},
	{0x01, {0x03, 0xCD}},
	{0x02, {0x03, 0xA2}},
	{0x03, {0x03, 0x2E}},
	{0x04, {0x02, 0xFF}},
	{0x05, {0x02, 0xCD}},
	{0x06, {0x02, 0x9B}},
	{0x07, {0x02, 0x71}},
	{0x08, {0x02, 0x16}},
	{0x09, {0x02, 0x14}},
	{0x0A, {0x01, 0xE3}},
	{0x0B, {0x01, 0xD7}},
	{0x0C, {0x01, 0x77}},
	{0x0D, {0x01, 0x51}},
	{0x0E, {0x01, 0x1E}},
	{0x0F, {0x00, 0xEB}},
	{0x10, {0x00, 0xBC}},
	{0x11, {0x00, 0x4A}},
	{0x12, {0x00, 0x30}},
};

static int isl_i2c_read(unsigned char cmd, unsigned char *recv_data)
{
	int ret = ISL248XX_FAIL;
	int count = I2C_RETRY_COUNT;
	unsigned char send_data[2];

	do {
		send_data[0] = cmd;

		ret = i2c_xfer(SLAVE_ADDRESS_ISL248XX, 1, send_data, 2, recv_data, I2C_CH_ISL248XX);
		if (ret == 0)
			return ret;
		else
			count--;

	} while(count > 0);

	return ret;
}

static int isl_i2c_write(unsigned char cmd, unsigned char *value)
{
	int ret = ISL248XX_FAIL;
	int count = I2C_RETRY_COUNT;

	unsigned char send_data[3];
	send_data[0] = cmd;
	send_data[1] = value[0];
	send_data[2] = value[1];

	do {
		ret = i2c_xfer(SLAVE_ADDRESS_ISL248XX, 3, send_data, 0, 0, I2C_CH_ISL248XX);
		if (ret == 0)
			return ret;
		else
			count--;

	} while(count > 0);

	return ret;
}

int isl248xx_read_id(unsigned char *recv_data)
{
	return isl_i2c_read(ID_ADDRESS_ISL248XX, recv_data);
}

/**
 * Null Byte(1) + DVR Byte(1) + (DAC 1 MSB(1) + DAC 1 LSB(1)) ... + (DAC 18 MSB(1) + DAC 18 LSB(1))
 */
#define STANDARD_MODE_READ_SIZE  1 + 1 + (2 * 18)

static int isl_i2c_standard_read(unsigned char *recv_data)
{
	int ret = ISL248XX_FAIL;
	int count = I2C_RETRY_COUNT;
	unsigned char send_data[1];

	do {
		send_data[0] = 0x00;

		ret = i2c_xfer(SLAVE_ADDRESS_ISL248XX, 1, send_data,
				STANDARD_MODE_READ_SIZE, recv_data, I2C_CH_ISL248XX);
		if (ret == 0)
			return ret;
		else
			count--;

	} while(count > 0);

	return ret;
}

/**
 * Control Byte(1) + DVR Byte(1) + (DAC 1 MSB(1) + DAC 1 LSB(1)) ... + (DAC 18 MSB(1) + DAC 18 LSB(1))
 */
#define STANDARD_MODE_WRITE_SIZE	1 + 1 + (2 * 18)

static int isl_i2c_standard_write(unsigned char *value)
{
	int ret = ISL248XX_FAIL;
	int i;
	int count = I2C_RETRY_COUNT;

	unsigned char send_data[STANDARD_MODE_WRITE_SIZE] = {0, };

	for (i = 0; i < STANDARD_MODE_WRITE_SIZE; i++)
	{
		send_data[i] = *(value + i);
	}

	do {
		ret = i2c_xfer(SLAVE_ADDRESS_ISL248XX,
				STANDARD_MODE_WRITE_SIZE, send_data, 0, 0, I2C_CH_ISL248XX);
		if (ret == 0)
			return ret;
		else
			count--;

	} while(count > 0);

	return ret;
}

int isl248xx_init(void)
{
	unsigned int i;
	unsigned char recv_data[2] = {0, };
	unsigned char lcd_ver = get_daudio_lcd_ver();
	unsigned char main_ver = get_daudio_main_ver();
	int ret = 0;

	if (main_ver == DAUDIO_BOARD_3RD)
		ret = isl248xx_read_id(recv_data);

	LKPRINT("%s main ver:%d read 0x00 0x%2x%2x, ret: %d\n",
			__func__, (int)main_ver, recv_data[0], recv_data[1], ret);

#if (DEBUG_ISL_DETAIL)	//read reset value
	switch (lcd_ver)
	{
		case DAUDIO_BOARD_4TH:
		#if defined(ISL24813_STANDARD_MODE)
			//ISL24813 read fully
			ret = isl_i2c_standard_read(data);
			for (i = 0; i < STANDARD_MODE_READ_SIZE; i++)
			{
				LKPRINT("%s %2d read: 0x%2x\n", __func__, i, data[i]);
			}
		#elif defined(ISL24813_REGISTER_MODE)
			//24813 write 0x80 register
			ret = isl_i2c_write(0x80, 0xC5);
			ret = isl_i2c_read(0x80, recv_data);
			LKPRINT("%s ISL24813 read 0x80: 0x%x%x\n",
					__func__, recv_data[0], recv_data[1]);
		#endif
			break;

		case DAUDIO_BOARD_3RD:
			for (i = 1; i < 19; i++)
			{
				ret = isl_i2c_read(i, recv_data);
				LKPRINT("%s read 0x%2x 0x%2x%2x, ret: %d\n",
						__func__, i, recv_data[0], recv_data[1], ret);
			}
			break;
	}
#endif

	if (ret == 0 && (recv_data[0] != 0xFF && recv_data[1] != 0xFF))		//success i2c
	{
		switch (lcd_ver)
		{
			case DAUDIO_BOARD_4TH:
				LKPRINT("%s ISL24813 init\n", __func__);
			#if defined(ISL24813_STANDARD_MODE)
				//ISL24813 standard mode
				ret = isl_i2c_standard_write(ISL24813_INIT_REGS);
			#elif defined(ISL24813_REGISTER_MODE)
				//ISL24813 register mode
				for (i = 0; i < ARRAY_SIZE(ISL24813_INIT_REGS); i++)
				{
					ret = isl_i2c_write(ISL24813_INIT_REGS[i].cmd, ISL24813_INIT_REGS[i].value);
				}
			#endif
				break;

			case DAUDIO_BOARD_3RD:
				//ISL24833
				LKPRINT("%s ISL24833 init\n", __func__);
				for (i = 0; i < ARRAY_SIZE(ISL24833_INIT_REGS); i++)
				{
					ret = isl_i2c_write(ISL24833_INIT_REGS[i].cmd, ISL24833_INIT_REGS[i].value);
				}
				break;
		}

		LKPRINT("%s write finished ret: %d\n", __func__, ret);

#if (DEBUG_ISL_DETAIL)	//detail debug
		switch (lcd_ver)
		{
			case DAUDIO_BOARD_4TH:
			#if defined(ISL24813_STANDARD_MODE)
				//24813 standard mode
				//ISL24813 read fully
				ret = isl_i2c_standard_read(data);
				for (i = 0; i < STANDARD_MODE_READ_SIZE; i++)
				{
					LKPRINT("%s %2d read: 0x%x ret: %d\n", __func__, i, data[i], ret);
				}
			#elif defined(ISL24813_REGISTER_MODE)
				//24813 register mode
				for (i = 0; i < ARRAY_SIZE(ISL24813_INIT_REGS); i++)
				{
					ret = isl_i2c_read(ISL24813_INIT_REGS[i].cmd, recv_data);
					LKPRINT("%s ISL24813 read 0x%2x 0x%2x%2x, ret: %d\n",
							__func__, ISL24813_INIT_REGS[i].cmd, recv_data[0], recv_data[1], ret);
				}
			#endif
				break;

			case DAUDIO_BOARD_3RD:
				for (i = 0; i < ARRAY_SIZE(ISL24833_INIT_REGS); i++)
				{
					ret = isl_i2c_read(ISL24833_INIT_REGS[i].cmd, recv_data);
					LKPRINT("%s ISL24833 read 0x%2x 0x%2x%2x, ret: %d\n",
							__func__, ISL24833_INIT_REGS[i].cmd, recv_data[0], recv_data[1], ret);
				}
				break;
		}
#endif
	}
	else
	{
		LKPRINT("%s Not found ISL248xx\n", __func__);
	}

	return ret;
}
