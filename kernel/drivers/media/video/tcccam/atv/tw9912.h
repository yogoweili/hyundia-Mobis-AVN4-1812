/*
 * drivers/media/video/tcccam/atv/tw9912.h
 *
 * Author: khcho (khcho@telechips.com)
 *
 * Copyright (C) 2008 Telechips, Inc.
 * Copyright (C) 2013 Cleinsoft, Inc.
 *
 * This file is licensed under the terms of the GNU General Public License 
 * version 2. This program is licensed "as is" without any warranty of any 
 * kind, whether express or implied.
 */

#ifndef __TW9912_H__
#define __TW9912_H__

enum TW9912_MODE {
	MODE_TW9912_INIT = 0,
	MODE_TW9912_OPEN,
	MODE_TW9912_CLOSE,
};

#define TW9912_REG_ID		0x00
#define TW9912_REG_CSTATUS	0x01

#define TW9912_I2C_BRIGHTNESS		0x10
#define TW9912_I2C_CONTRAST			0x11
#define TW9912_I2C_HUE				0x15
#define TW9912_I2C_SATURATION_U		0x13
#define TW9912_I2C_SATURATION_V		0x14
#define TW9912_I2C_SHARPNESS		0x12 //GT system SHARPNESS CONTROL REGISTER I 


#endif
