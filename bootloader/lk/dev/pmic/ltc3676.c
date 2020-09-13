#include <i2c.h>
#include <debug.h>

#include <dev/ltc3676.h>
#include <daudio_ver.h>

#if defined(CONFIG_DAUDIO_PRINT_DEBUG_MSG)
#define DEBUG	1
#else
#define DEBUG	0
#endif

#if (DEBUG)
#define LKPRINT(fmt, args...) dprintf(INFO, "[ltc3676] " fmt, ##args)
#else
#define LKPRINT(args...) do {} while(0)
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

/* device address */
#define SLAVE_ADDR_LTC3676		0x78
#define I2C_CH_LTC3676			2

/* init settings */
static struct LTC3676_REG DAUDIO_REV_4_INIT_SETTINGS[] =
{
	{LTC3676_REG_DVB1A, 0x17},	//ttlim@mobis.co.kr request.
};

static struct LTC3676_REG DAUDIO_REV_5_INIT_SETTINGS[] =
{
	{LTC3676_REG_DVB1A, 0x17},	//ttlim@mobis.co.kr request.
	{LTC3676_REG_DVB4A, 0x1A},
};

static int ltc3676_read(unsigned char cmd)
{
	/**
	 * @author legolamp@cleinsoft
	 * @date 2014/11/28
	 * Add Read data exception hadnling - retry three times
	 **/

	int count = 0;
	int ret = LTC3376_FAIL;
	unsigned char recv_data = 0;

	do {
		ret = i2c_xfer(SLAVE_ADDR_LTC3676, 1, &cmd, 1, &recv_data, I2C_CH_LTC3676);
		if (ret == 0)
		{
			return recv_data;
		}
		else {
			LKPRINT(" %s i2c Read Fail - Retry %d\n", __func__, count);
			count++;
		}

	} while ( count < LTC3376_I2C_RETRY_COUNT );

	return ret;
}

static int ltc3676_write(unsigned char cmd, unsigned char value)
{
	/**
	 * @author legolamp@cleinsoft
	 * @date 2014/11/25
	 * Add Write data exception hadnling - retry three times
	 * @date 2014/11/28
	 * Compare to read the write value
	 **/

	int count = 0;
	int ret = LTC3376_FAIL;
	unsigned char send_data[2];
	send_data[0] = cmd;
	send_data[1] = value;

	do {
		ret = i2c_xfer(SLAVE_ADDR_LTC3676, 2, send_data, 0, 0, I2C_CH_LTC3676);
		if (ret == 0)
		{
			int read_ret = ltc3676_read(cmd);

			if(read_ret == value)
			{
				return ret;
			}
			else
			{
				LKPRINT("Fail : recv_data 0x%x\n", read_ret);
			}
		}
		else {
			LKPRINT(" %s i2c Write Fail - Retry %d\n", __func__, count);
			count++;
		}

	} while( count < LTC3376_I2C_RETRY_COUNT);

	return ret;
}

int ltc3676_init(void)
{
	struct LTC3676_REG *pLTC3676_REG;
	int i, ArraySize, err_count = 0;
	unsigned char main_ver = get_daudio_main_ver();

	/**
	 * @author legolamp@cleinsoft
	 * @date 2014/11/10
	 * Divide board generation in power
	 * @date 2014/11/25
	 * Add Write data exception hadnling - compare read data
	 **/

	switch(main_ver)
	{
		case DAUDIO_BOARD_6TH:
		case DAUDIO_BOARD_5TH:
		default:
			ArraySize = ARRAY_SIZE(DAUDIO_REV_5_INIT_SETTINGS);
			pLTC3676_REG = DAUDIO_REV_5_INIT_SETTINGS;
			break;
	}

	for (i = 0; i < ArraySize; i++)
	{
		int ret =  ltc3676_write(pLTC3676_REG[i].cmd, pLTC3676_REG[i].value);
		if (ret != 0) {
			LKPRINT("%s Fail Write Data \n", __func__);
		}
	}

	return err_count > 0 ? -err_count : 0;
}

static int dvb_test_start(void)
{
	unsigned char cpu = ltc3676_read(LTC3676_REG_DVB1A);
	unsigned char core = ltc3676_read(LTC3676_REG_DVB3A);
	unsigned char ddr = ltc3676_read(LTC3676_REG_DVB4A);
	int count = 0;
	int type_count = 0;
	unsigned char input_val = 0;
	char new_val[2] = {0, };
	char *input_type[3] = {"CPU", "CORE", "DDR" };
	unsigned char REGS[3] = {LTC3676_REG_DVB1A, LTC3676_REG_DVB3A, LTC3676_REG_DVB4A};

	LKPRINT("%s read default CPU=0x%x, CORE=0x%x DDR=0x%x\n",
			__func__, cpu, core, ddr);

	for (type_count = 0; type_count < 3; type_count++)
	{
		LKPRINT("%s input (0 ~ 31)\n", input_type[type_count]);
		while (count != 2)
		{
			if (getc(&new_val[count]) >= 0 && new_val[count] >= '0' && new_val[count] <= '9')
			{
				LKPRINT("input: %c\n", new_val[count]);
				count++;
			}
			else if (new_val[count] == 'x')
				return -1;
		}
		input_val = atoi(new_val);
		if (input_val < 0 || input_val > 0x1F)
			input_val = 0x19;

		LKPRINT("%s %d -> 0x%x\n", input_type[type_count], (int)input_val, input_val);
		ltc3676_write(REGS[type_count], input_val);
		mdelay(5);
		input_val = ltc3676_read(REGS[type_count]);
		LKPRINT("%s new value: 0x%x\n", input_type[type_count], input_val);

		count = 0;
	}

	cpu = ltc3676_read(LTC3676_REG_DVB1A);
	core = ltc3676_read(LTC3676_REG_DVB3A);
	ddr = ltc3676_read(LTC3676_REG_DVB4A);

	LKPRINT("%s CPU=0x%x, CORE=0x%x, DDR=0x%x\n",
			__func__, cpu, core, ddr);

	return 0;
}

static int dvb_test_stop(void)
{
	ltc3676_write(LTC3676_REG_DVB3A, 0x19);
	ltc3676_write(LTC3676_REG_DVB4A, 0x19);

	//for default PMIC LTC3676 settings
	ltc3676_init();

	unsigned char cpu = ltc3676_read(LTC3676_REG_DVB1A);
	unsigned char core = ltc3676_read(LTC3676_REG_DVB3A);
	unsigned char ddr = ltc3676_read(LTC3676_REG_DVB4A);

	LKPRINT("%s read         CPU=0x%x, CORE=0x%x, DDR=0x%x\n",
			__func__, cpu, core, ddr);

	return 0;
}

int ltc3676_buck_control(void)
{
	unsigned char buck1 = ltc3676_read(LTC3676_REG_BUCK1);
	unsigned char buck2 = ltc3676_read(LTC3676_REG_BUCK2);
	unsigned char buck3 = ltc3676_read(LTC3676_REG_BUCK3);
	unsigned char buck4 = ltc3676_read(LTC3676_REG_BUCK4);
	int count = 0;
	int type_count = 0;
	int input_val = 0;
	char new_val[2] = {0, };

	char *input_type[4] = {"BUCK1", "BUCK2", "BUCK3", "BUCK4"};
	unsigned char REGS[4] = {LTC3676_REG_BUCK1, LTC3676_REG_BUCK2, LTC3676_REG_BUCK3, LTC3676_REG_BUCK4};
	int is_skip = 0;

	LKPRINT("%s read default BUCK1=0x0%x, BUCK2=0x0%x, BUCK3=0x0%x, BUCK4=0x0%x\n",
			__func__, buck1, buck2, buck3, buck4);

	for (type_count = 0; type_count < 4; type_count++)
	{
		count = 0;
		is_skip = 0;
		new_val[0] = 0; new_val[1] = 0;

		LKPRINT("%s BUCK %d setting\n", __func__, (type_count + 1));
		LKPRINT("%s Phase select: Clock Phase 1(0), Clock Phase 2(1), skip(x)\n", input_type[type_count]);

		while (count == 0 && is_skip == 0)
		{
			if (getc(&new_val[count]) >= 0 && (new_val[count] == '0' || new_val[count] == '1'))
			{
				LKPRINT("intput : %c\n", new_val[count]);
				count++;
			}
			else if (new_val[count] == 'x')
			{
				is_skip = 1;
			}
		}

		if (is_skip == 0)
			LKPRINT("%s Clock Rate: 2.25Mhz(0), 1.125MHz(1), skip(x)\n", input_type[type_count]);

		while (count == 1 && is_skip == 0)
		{
			if (getc(&new_val[count]) >= 0 && (new_val[count] == '0' || new_val[count] == '1'))
			{
				LKPRINT("intput : %c\n", new_val[count]);
				count++;
			}
			else if (new_val[count] == 'x')
			{
				is_skip = 1;
			}
		}

		if (is_skip == 0)
		{
			input_val = atoi(new_val);

			switch (input_val)
			{
				case 0:
					ltc3676_write(REGS[type_count], 0x0);
					break;
				case 1:
					ltc3676_write(REGS[type_count], 0x4);
					break;
				case 10:
					ltc3676_write(REGS[type_count], 0x8);
					break;
				case 11:
					ltc3676_write(REGS[type_count], 0xC);
					break;
				default :
					ltc3676_write(REGS[type_count], 0x0);
					break;
			}

			mdelay(5);
			input_val = ltc3676_read(REGS[type_count]);
			LKPRINT("%s new value : 0x0%x\n", input_type[type_count], input_val);
		}
	}

	buck1 = ltc3676_read(LTC3676_REG_BUCK1);
	buck2 = ltc3676_read(LTC3676_REG_BUCK2);
	buck3 = ltc3676_read(LTC3676_REG_BUCK3);
	buck4 = ltc3676_read(LTC3676_REG_BUCK4);

	LKPRINT("%s read default buck1=0x0%x, buck2=0x0%x, buck3=0x0%x, buck4=0x0%x\n",
			__func__, buck1, buck2, buck3, buck4);
}

int ltc3676_test_start(void)
{
	return dvb_test_start();
}

int ltc3676_test_stop(void)
{
	int ret = dvb_test_stop();
	mdelay(20);

	return ret;
}
