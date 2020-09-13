#ifndef _DEV_PMIC_LTC3676_H_
#define _DEV_PMIC_LTC3676_H_

/*
 * @author valky@cleinsoft
 * @date  2014/03/06
 * Set as LTC3676 not LTC3676_1 by cleinsoft
 *
 * This driver support both LTC3676 and LTC3676-1
 * When LTC3676-1 is set this driver supports LTC3676-1 operation
 * else the driver is for LTC3676
 **/
#define LTC3676  1              

#define LTC3676_DCDC_1   0x00
#define LTC3676_DCDC_2   0x01
#define LTC3676_DCDC_3   0x02
#define LTC3676_DCDC_4   0x03
#define LTC3676_LDO_1    0x04
#define LTC3676_LDO_2    0x05
#define LTC3676_LDO_3    0x06
#define LTC3676_LDO_4    0x07
#define LTC3676_REG_NUM  7
#define LTC3676_NUM_REGULATOR   7
#define LTC3376_I2C_RETRY_COUNT	3
#define LTC3376_FAIL			-1

#define BIT_0_MASK       0x01
#define BIT_1_MASK       0x02
#define BIT_2_MASK       0x04
#define BIT_3_MASK       0x08
#define BIT_4_MASK       0x10
#define BIT_5_MASK       0x20
#define BIT_6_MASK       0x40
#define BIT_7_MASK       0x80

/* LTC3676 Registers */
#define LTC3676_REG_BUCK1     0x01
#define LTC3676_REG_BUCK2     0x02
#define LTC3676_REG_BUCK3     0x03
#define LTC3676_REG_BUCK4     0x04
#define LTC3676_REG_LDOA      0x05
#define LTC3676_REG_LDOB      0x06
#define LTC3676_REG_SQD1      0x07
#define LTC3676_REG_SQD2      0x08
#define LTC3676_REG_CNTRL     0x09
#define LTC3676_REG_DVB1A     0x0A
#define LTC3676_REG_DVB1B     0x0B
#define LTC3676_REG_DVB2A     0x0C
#define LTC3676_REG_DVB2B     0x0D
#define LTC3676_REG_DVB3A     0x0E
#define LTC3676_REG_DVB3B     0x0F
#define LTC3676_REG_DVB4A     0x10
#define LTC3676_REG_DVB4B     0x11
#define LTC3676_REG_MSKIRQ    0x12
#define LTC3676_REG_MSKPG     0x13
#define LTC3676_REG_USER      0x14
#define LTC3676_REG_IRQSTAT   0x15
#define LTC3676_REG_PGSTATL   0x16
#define LTC3676_REG_PGSTATRT  0x17
#define LTC3676_REG_HRST      0x1E
#define LTC3676_REG_CLIRQ     0x1F

struct LTC3676_REG
{
	unsigned char cmd;
	unsigned char value;
};

int ltc3676_init(void);
int ltc3676_test_start(void);
int ltc3676_test_stop(void);

#endif /* _DEV_PMIC_LTC3676_H_ */
