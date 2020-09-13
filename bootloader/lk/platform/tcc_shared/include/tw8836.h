#ifndef _TW8836_
#define _TW8836_

#include <i2c.h>
#include <platform/gpio.h>

#include <daudio_settings.h>

#if (DAUDIO_HW_REV >= 0x20131007)
#if defined(LVDS_CAM_ENABLE)
#define TW8836_RESET_GPIO		TCC_GPF(18)
#else
#define TW8836_RESET_GPIO		TCC_GPD(17)
#endif
#else
#define TW8836_RESET_GPIO		0xFFFFFFFF	//NC_PORT
#endif

#define SLAVE_ADDRESS_TW8836 	0x8A
#define I2C_CH_TW8836 			I2C_CH_MASTER3

#define I2C_CH_MXT336S_AT 		I2C_CH_MASTER1

/***********************************************************************
 *  14/08/29 , 
 *  referenced :  /kernel/arch/arm/mach-tcc893x/include/mach
 *  Issue : TW8836 initial error 
 ***********************************************************************/
#define TW8836_SETTINGS_ID			100
#define TW8836_SETTINGS_VERSION		140212

#define I2C_RETRY_COUNT                     3
#define TW8836_IMAGE_ENHANCEMENT_PAGE       2
#define TW8836_CAM_IMAGE_ENHANCEMENT_PAGE   1

#define MOVE_PAGE_ADDRESS       0xFF

//page 2
//BRIGHTNESS 0 ~ 255
#define I2C_BRIGHTNESS_R        0x87
#define I2C_BRIGHTNESS_G        0x88
#define I2C_BRIGHTNESS_B        0x89

//CONTRAST 0 ~ 255
#define I2C_CONTRAST_R          0x81
#define I2C_CONTRAST_G          0x82
#define I2C_CONTRAST_B          0x83

//HUE -45 ~ +45
#define I2C_HUE                 0x80

//SATURATION 0 ~ 255
#define I2C_SATURTION_CB        0x85
#define I2C_SATURTION_CR        0x86

//page 1
//BRIGHTNESS -128 ~ +127
#define I2C_CAM_BRIGHTNESS      0x10

//CONTRAST  0 ~ 255
#define I2C_CAM_CONTRAST        0x11

//SATURATION 0 ~ 200
#define I2C_CAM_SATURATION_U    0x13
#define I2C_CAM_SATURATION_V    0x14

#define I2C_CAM_HUE             0x15

struct TW8836_REG
{
	unsigned char cmd;
	unsigned char value;
};

int tw8836_set_ie(int cmd, unsigned char level);
int tw8836_write_value(unsigned char page,
	int address_size, const unsigned char* addresses, char value);

void tw8836_init();
void tw8836_analog_input_init();

int has_MXT336S(void);
int tw8836_get_bt656(void);
int tw8836_read_id(unsigned char *read_data);

#endif
