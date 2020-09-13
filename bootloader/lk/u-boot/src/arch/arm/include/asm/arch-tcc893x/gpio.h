/*
 * Copyright (c) 2011 Telechips, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __TCC_GPIO_H
#define __TCC_GPIO_H

#define GPIO_INPUT  0x0001
#define GPIO_OUTPUT 0x0002

#define GPIO_LEVEL  0x0000
#define GPIO_EDGE   0x0010

#define GPIO_RISING 0x0020
#define GPIO_FALLING    0x0040

#define GPIO_HIGH   0x0020
#define GPIO_LOW    0x0040

#define GPIO_PULLUP 0x0100
#define GPIO_PULLDOWN   0x0200
#define GPIO_PULLDISABLE 0x0400

#define GPIO_REGMASK    0x000001E0
#define GPIO_REG_SHIFT  5
#define GPIO_BITMASK    0x0000001F

#define GPIO_PORTA      (0 << GPIO_REG_SHIFT)
#define GPIO_PORTB      (1 << GPIO_REG_SHIFT)
#define GPIO_PORTC      (2 << GPIO_REG_SHIFT)
#define GPIO_PORTD      (3 << GPIO_REG_SHIFT)
#define GPIO_PORTE      (4 << GPIO_REG_SHIFT)
#define GPIO_PORTF      (5 << GPIO_REG_SHIFT)
#define GPIO_PORTG      (6 << GPIO_REG_SHIFT)
#define GPIO_PORTHDMI   (7 << GPIO_REG_SHIFT)
#define GPIO_PORTADC    (8 << GPIO_REG_SHIFT)
#define GPIO_PORTEXT1 	( 9 << GPIO_REG_SHIFT)
#define GPIO_PORTEXT2 	(10 << GPIO_REG_SHIFT)
#define GPIO_PORTEXT3 	(11 << GPIO_REG_SHIFT)
#define GPIO_PORTEXT4 	(12 << GPIO_REG_SHIFT)
#define GPIO_PORTEXT5 	(13 << GPIO_REG_SHIFT)
#define GPIO_PORTSD 	(14 << GPIO_REG_SHIFT)

#define TCC_GPA(x)    (GPIO_PORTA | (x))
#define TCC_GPB(x)    (GPIO_PORTB | (x))
#define TCC_GPC(x)    (GPIO_PORTC | (x))
#define TCC_GPD(x)    (GPIO_PORTD | (x))
#define TCC_GPE(x)    (GPIO_PORTE | (x))
#define TCC_GPF(x)    (GPIO_PORTF | (x))
#define TCC_GPG(x)    (GPIO_PORTG | (x))
#define TCC_GPHDMI(x) (GPIO_PORTHDMI | (x))
#define TCC_GPADC(x)  (GPIO_PORTADC | (x))
#define TCC_GPEXT1(x) (GPIO_PORTEXT1 | (x))		//PCA9539_U3	0x74
#define TCC_GPEXT2(x) (GPIO_PORTEXT2 | (x))		//PCA9539_U2	0x77
#define TCC_GPEXT3(x) (GPIO_PORTEXT3 | (x))		//PCA9539_U5	0x75
#define TCC_GPEXT4(x) (GPIO_PORTEXT4 | (x))		//PCA9539_U4	0x70
#define TCC_GPEXT5(x) (GPIO_PORTEXT5 | (x))		//power board PCA9539_U18	0x76
#define TCC_GPSD(x) 	(GPIO_PORTSD | (x))

#define GPIO_FN_BITMASK	0xFF000000
#define GPIO_FN_SHIFT	24
#define GPIO_FN(x)    (((x) + 1) << GPIO_FN_SHIFT)
#define GPIO_FN0      (1  << GPIO_FN_SHIFT)
#define GPIO_FN1      (2  << GPIO_FN_SHIFT)
#define GPIO_FN2      (3  << GPIO_FN_SHIFT)
#define GPIO_FN3      (4  << GPIO_FN_SHIFT)
#define GPIO_FN4      (5  << GPIO_FN_SHIFT)
#define GPIO_FN5      (6  << GPIO_FN_SHIFT)
#define GPIO_FN6      (7  << GPIO_FN_SHIFT)
#define GPIO_FN7      (8  << GPIO_FN_SHIFT)
#define GPIO_FN8      (9  << GPIO_FN_SHIFT)
#define GPIO_FN9      (10 << GPIO_FN_SHIFT)
#define GPIO_FN10     (11 << GPIO_FN_SHIFT)
#define GPIO_FN11     (12 << GPIO_FN_SHIFT)
#define GPIO_FN12     (13 << GPIO_FN_SHIFT)
#define GPIO_FN13     (14 << GPIO_FN_SHIFT)
#define GPIO_FN14     (15 << GPIO_FN_SHIFT)
#define GPIO_FN15     (16 << GPIO_FN_SHIFT)

#define GPIO_CD_BITMASK	0x00F00000
#define GPIO_CD_SHIFT	20
#define GPIO_CD(x)    (((x) + 1) << GPIO_CD_SHIFT)
#define GPIO_CD0      (1 << GPIO_CD_SHIFT)
#define GPIO_CD1      (2 << GPIO_CD_SHIFT)
#define GPIO_CD2      (3 << GPIO_CD_SHIFT)
#define GPIO_CD3      (4 << GPIO_CD_SHIFT)

#ifdef CONFIG_PCA953X
//PCA9539_U2
#define GXP_MUTE_CTL      		(1 << 0)
#define GXP_LVDSIVT_ON    	(1 << 1)
#define GXP_LVDS_LP_CTRL  	(1 << 2)
#define GXP_AUTH_RST_     		(1 << 3)
#define GXP_BT_RST_       		(1 << 4)
#define GXP_SDWF_RST_     		(1 << 5)
#define GXP_CAS_RST_      		(1 << 6)
#define GXP_CAS_GP        		(1 << 7)

#define GXP_DXB1_PD       		(1 << 8)
#define GXP_DXB2_PD       		(1 << 9)
#define GXP_PWR_CONTROL0  	(1 << 10)
#define GXP_PWR_CONTROL1  	(1 << 11)
#define GXP_HDD_RST_      		(1 << 12)
#define GXP_OTG0_VBUS_EN  	(1 << 13)
#define GXP_OTG1_VBUS_EN  	(1 << 14)
#define GXP_HOST_VBUS_EN  	(1 << 15)

//PCA9539_U3      
#define GXP_LCD_ON        		(1 << 0)
#define GXP_CODEC_ON      		(1 << 1)
#define GXP_SD0_ON        		(1 << 2)
#define GXP_SD1_ON        		(1 << 3)
#define GXP_SD2_ON        		(1 << 4)
#define GXP_EXT_CODEC_ON  	(1 << 5)
#define GXP_GPS_PWREN     		(1 << 6)
#define GXP_BT_ON         		(1 << 7)

#define GXP_DXB_ON        		(1 << 8)
#define GXP_IPOD_ON       		(1 << 9)
#define GXP_CAS_ON        		(1 << 10)
#define GXP_FMTC_ON       		(1 << 11)
#define GXP_P_CAM_PWR_ON  	(1 << 12)
#define GXP_CAM1_ON       		(1 << 13)
#define GXP_CAM2_ON       		(1 << 14)
#define GXP_ATAPI_ON      		(1 << 15)

//PCA9539_U5      
#define GXP_FMT_RST_     		(1 << 0)
#define GXP_FMT_IRQ       		(1 << 1)
#define GXP_BT_WAKE       		(1 << 2)
#define GXP_BT_HWAKE      		(1 << 3)
#define GXP_PHY2_ON       		(1 << 4)
#define GXP_COMPASS_RST   	(1 << 5)
#define GXP_CAM_FL_EN     		(1 << 6)
#define GXP_CAM2_FL_EN    	(1 << 7)

#define GXP_CAM2_RST_     		(1 << 8)
#define GXP_CAM2_PWDN     		(1 << 9)
#define GXP_LCD_RST_      		(1 << 10)
#define GXP_TV_SLEEP_     		(1 << 11)
#define GXP_ETH_ON        		(1 << 12)
#define GXP_ETH_RST_      		(1 << 13)
#define GXP_SMART_AUX1    	(1 << 14)
#define GXP_SMART_AUX2    	(1 << 15)

//PCA9538_U11
#define GXP_PLL1_EN          	(1 << 0)
#define GXP_OTG_EN            	(1 << 1)
#define GXP_HS_HOST_EN       	(1 << 2)
#define GXP_FS_HOST_EN       	(1 << 3)
#define GXP_HDMI_EN          	(1 << 4)
#define GXP_MIPI_EN          	(1 << 5)
#define GXP_SATA_EN          	(1 << 6)
#define GXP_LVDS_EN          	(1 << 7)

#define GXP_ATV_V5P0_EN            	(1 << 8)
#define GXP_ATAPI_IPOD_V5P0_EN     	(1 << 9)
#define GXP_USB_CHARGE_CURRENT_SEL 	(1 << 10)
#define GXP_USB_SUSPEND            	(1 << 11)
#define GXP_CHARGE_EN              	(1 << 12)
#define GXP_SD30PWR_EN             	(1 << 13)
#define GXP_SD30PWR_CTL            	(1 << 14)
#define GXP_CHARGE_FINISH          	(1 << 15)
#endif

extern int gpio_config(unsigned n, unsigned flags);
extern int gpio_set_value(unsigned gpio, int value);

#endif
