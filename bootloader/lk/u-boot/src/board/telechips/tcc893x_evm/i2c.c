#include <common.h>
#include <i2c.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/globals.h>
#include <asm/arch/reg_physical.h>
#include <asm/arch/i2c.h>

static void i2c_set_gpio(int i2c_name)
{
	switch (i2c_name)
	{
		case I2C_CH_MASTER0:
				if(CONFIG_TCC_BOARD_REV == 0x1000){
					//I2C[8] - GPIOB[9][10]
					BITCSET(((PI2CPORTCFG)(HwI2C_PORTCFG_BASE))->PCFG0.nREG, 0x000000FF, 8);
					gpio_config(TCC_GPB(9), GPIO_FN11|GPIO_OUTPUT|GPIO_LOW);
					gpio_config(TCC_GPB(10), GPIO_FN11|GPIO_OUTPUT|GPIO_LOW);
				}else if(CONFIG_TCC_BOARD_REV == 0x2000 || CONFIG_TCC_BOARD_REV == 0x3000){
					//I2C[18] - GPIOF[13][14]
					BITCSET(((PI2CPORTCFG)(HwI2C_PORTCFG_BASE))->PCFG0.nREG, 0x000000FF, 18);
					gpio_config(TCC_GPF(13), GPIO_FN10|GPIO_OUTPUT|GPIO_LOW);
					gpio_config(TCC_GPF(14), GPIO_FN10|GPIO_OUTPUT|GPIO_LOW);
				}else if(CONFIG_TCC_BOARD_REV == 0x5000 || CONFIG_TCC_BOARD_REV == 0x5001 || CONFIG_TCC_BOARD_REV == 0x5002 || CONFIG_TCC_BOARD_REV == 0x5003){
#if defined(CONFIG_CHIP_TCC8935S) || defined(CONFIG_CHIP_TCC8933S) || defined(CONFIG_CHIP_TCC8937S)
					BITCSET(((PI2CPORTCFG)(HwI2C_PORTCFG_BASE))->PCFG0.nREG, 0x000000FF, 12);
					gpio_config(TCC_GPC(2), GPIO_FN7|GPIO_OUTPUT|GPIO_LOW);
					gpio_config(TCC_GPC(3), GPIO_FN7|GPIO_OUTPUT|GPIO_LOW);
#else
					BITCSET(((PI2CPORTCFG)(HwI2C_PORTCFG_BASE))->PCFG0.nREG, 0x000000FF, 13);
					gpio_config(TCC_GPC(20), GPIO_FN7|GPIO_OUTPUT|GPIO_LOW);
					gpio_config(TCC_GPC(21), GPIO_FN7|GPIO_OUTPUT|GPIO_LOW);
#endif
				}
			break;
		case I2C_CH_MASTER1:
				if(CONFIG_TCC_BOARD_REV == 0x1000){
					//I2C[21] - GPIOF[27][28]
					BITCSET(((PI2CPORTCFG)(HwI2C_PORTCFG_BASE))->PCFG0.nREG, 0x0000FF00, 21<<8);
					gpio_config(TCC_GPF(27), GPIO_FN10|GPIO_OUTPUT|GPIO_LOW);
					gpio_config(TCC_GPF(28), GPIO_FN10|GPIO_OUTPUT|GPIO_LOW);
				}else if(CONFIG_TCC_BOARD_REV == 0x2000 || CONFIG_TCC_BOARD_REV == 0x3000){
					//I2C[28] - GPIO_ADC[2][3]
					BITCSET(((PI2CPORTCFG)(HwI2C_PORTCFG_BASE))->PCFG0.nREG, 0x0000FF00, 28<<8);
					gpio_config(TCC_GPADC(2), GPIO_FN3|GPIO_OUTPUT|GPIO_LOW);
					gpio_config(TCC_GPADC(3), GPIO_FN3|GPIO_OUTPUT|GPIO_LOW);
				}else if(CONFIG_TCC_BOARD_REV == 0x5000 || CONFIG_TCC_BOARD_REV == 0x5001 || CONFIG_TCC_BOARD_REV == 0x5002 || CONFIG_TCC_BOARD_REV == 0x5003){
#if defined(CONFIG_CHIP_TCC8935S) || defined(CONFIG_CHIP_TCC8933S) || defined(CONFIG_CHIP_TCC8937S)
					BITCSET(((PI2CPORTCFG)(HwI2C_PORTCFG_BASE))->PCFG0.nREG, 0x0000FF00, 25<<8);
					gpio_config(TCC_GPG(12), GPIO_FN4|GPIO_OUTPUT|GPIO_LOW);
					gpio_config(TCC_GPG(13), GPIO_FN4|GPIO_OUTPUT|GPIO_LOW);
#else
					BITCSET(((PI2CPORTCFG)(HwI2C_PORTCFG_BASE))->PCFG0.nREG, 0x0000FF00, 22<<8);
					gpio_config(TCC_GPG(12), GPIO_FN14|GPIO_OUTPUT|GPIO_LOW);
					gpio_config(TCC_GPG(13), GPIO_FN14|GPIO_OUTPUT|GPIO_LOW);
#endif
				}
			break;
		case I2C_CH_MASTER2:
			if(CONFIG_TCC_BOARD_REV == 0x1000){
				//I2C[15] - GPIOC[30][31]
				BITCSET(((PI2CPORTCFG)(HwI2C_PORTCFG_BASE))->PCFG0.nREG, 0x00FF0000, 15<<16);
				gpio_config(TCC_GPC(30), GPIO_FN7|GPIO_OUTPUT|GPIO_LOW);
				gpio_config(TCC_GPC(31), GPIO_FN7|GPIO_OUTPUT|GPIO_LOW);
			}else if(CONFIG_TCC_BOARD_REV == 0x5000 || CONFIG_TCC_BOARD_REV == 0x5001 || CONFIG_TCC_BOARD_REV == 0x5002 || CONFIG_TCC_BOARD_REV == 0x5003){
				BITCSET(((PI2CPORTCFG)(HwI2C_PORTCFG_BASE))->PCFG0.nREG, 0x00FF0000, 18<<8);
				gpio_config(TCC_GPF(13), GPIO_FN10|GPIO_OUTPUT|GPIO_LOW);
				gpio_config(TCC_GPF(14), GPIO_FN10|GPIO_OUTPUT|GPIO_LOW);
			}
			break;
		case I2C_CH_MASTER3:
			break;
		default:
			break;
	}
}


void i2c_init_board(void)
{
	int ch;

	/* I2C Controller */
	for (ch = I2C_CH_MASTER0; ch <= I2C_CH_MASTER3; ch++) {
		i2c_set_gpio(ch);
	}
}


