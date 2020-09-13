#ifndef __ISL248XX__
#define __ISL248XX__

#include <i2c.h>
#include <platform/gpio.h>
#include <daudio_settings.h>

#define SLAVE_ADDRESS_ISL248XX 	0xE8
#define I2C_CH_ISL248XX 		I2C_CH_MASTER3

#define ISL248XX_SUCCESS		1
#define ISL248XX_FAIL			-1
#define I2C_RETRY_COUNT			3

#define ID_ADDRESS_ISL248XX		0x00

typedef struct
{
	unsigned char cmd;
	unsigned char value[2];		//value[0] = MSB, value[1] = LSB
} ISL248XX_REG;

int isl248xx_init(void);
int isl248xx_read_id(unsigned char *recv_data);

#endif
