/*
 *  Copyright (C) 2013 Telechips
 *  Telechips <linux@telechips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <i2c.h>
#include <power/pmic.h>
#include <asm/arch/globals.h>
#include <asm/arch/clock.h>
#include <asm/arch/reg_physical.h>
#include <asm/arch/iomap.h>
#include <asm/gpio.h>
#include <axp192.h>
#include "tcc_used_mem.h"
#include <lcd.h>
#include <asm/arch/sys_proto.h>


DECLARE_GLOBAL_DATA_PTR;

extern void clock_init(void);



int power_init_board(void)
{
	struct pmic *p;


#ifdef CONFIG_PCA953X
	pca953x_set_dir(CONFIG_SYS_I2C_PCA953X_U2, 0xFFFF, 0x0000);		// All Pins are Output
	pca953x_set_dir(CONFIG_SYS_I2C_PCA953X_U3, 0xFFFF, 0x1000);		// P-CAM_PWRON is Input, Others are Output
	pca953x_set_dir(CONFIG_SYS_I2C_PCA953X_U5, 0xFFFF, 0xC00A);		// FMT_IRQ, HT_HWAKE are Input, SMART_AUX1,2 are Input, Others are Output
	pca953x_set_dir(CONFIG_SYS_I2C_PCA953X_U11, 0xFFFF, 0x8000);		// ChargeFin is Input, Others are Output

    pca953x_set_val(CONFIG_SYS_I2C_PCA953X_U2, 0xFFFF, 0
						//	|GXP_MUTE_CTL 
						#if defined(CONFIG_USE_LVDSLCD)
							|GXP_LVDSIVT_ON
							|GXP_LVDS_LP_CTRL
						#endif
							|GXP_AUTH_RST_
							|GXP_BT_RST_
							|GXP_SDWF_RST_
							|GXP_CAS_RST_
						//	|GXP_CAS_GP
						//	|GXP_DXB1_PD
						//	|GXP_DXB2_PD
						//	|GXP_PWR_CONTROL0
						//	|GXP_PWR_CONTROL1
							|GXP_HDD_RST_
						//	|GXP_OTG0_VBUS_EN
						//	|GXP_OTG1_VBUS_EN
						//	|GXP_HOST_VBUS_EN
							);

    pca953x_set_val(CONFIG_SYS_I2C_PCA953X_U3, 0xFFFF, 0
							|GXP_LCD_ON
							|GXP_CODEC_ON
						#if defined(SD_UPDATE_INCLUDE) || defined(BOOTSD_INCLUDE)
							|GXP_SD0_ON
							|GXP_SD1_ON
							|GXP_SD2_ON
						#endif
						//	|GXP_EXT_CODEC_ON
						//	|GXP_GPS_PWREN
						//	|GXP_BT_ON
						//	|GXP_DXB_ON
						//	|GXP_IPOD_ON
						//	|GXP_CAS_ON
							|GXP_FMTC_ON
						//	|GXP_P_CAM_PWR_ON
						//	|GXP_CAM1_ON
						//	|GXP_CAM2_ON
						//	|GXP_ATAPI_ON
							);

	pca953x_set_val(CONFIG_SYS_I2C_PCA953X_U5, 0xFFFF, 0
							|GXP_FMT_RST_
						//	|GXP_FMT_IRQ
						//	|GXP_BT_WAKE
						//	|GXP_BT_HWAKE
						//	|GXP_PHY2_ON
						//	|GXP_COMPASS_RST
						//	|GXP_CAM_FL_EN
						//	|GXP_CAM2_FL_EN
							|GXP_CAM2_RST_
						//	|GXP_CAM2_PWDN
							|GXP_LCD_RST_
							|GXP_TV_SLEEP_
						//	|GXP_ETH_ON
							|GXP_ETH_RST_
						//	|GXP_SMART_AUX1
						//	|GXP_SMART_AUX2
							);
	
	pca953x_set_val(CONFIG_SYS_I2C_PCA953X_U11, 0xFFFF, 0
							|GXP_PLL1_EN
							|GXP_OTG_EN
						//	|GXP_HS_HOST_EN
						//	|GXP_FS_HOST_EN
						#if 1//defined(USE_HDMI)
							|GXP_HDMI_EN
						#endif
						//	|GXP_MIPI_EN
						//	|GXP_SATA_EN
						#if defined(USE_LVDSLCD)
							|GXP_LVDS_EN
						#endif
						//	|GXP_ATV_V5P0_EN
						//	|GXP_ATAPI_IPOD_V5P0_EN
						//	|GXP_USB_CHARGE_CURRENT_SEL
						//	|GXP_USB_SUSPEND
						//	|GXP_CHARGE_EN
							|GXP_SD30PWR_EN
							|GXP_SD30PWR_CTL
						//	|GXP_CHARGE_FINISH
							);
#endif						


#ifdef CONFIG_POWER_AXP192
	axp192_pmic_init(I2C_0);

	p = pmic_get("axp192");
	pmic_set_voltage(p, AXP192_ID_DCDC1, 1200);  // CPU 1200mV
	pmic_set_voltage(p, AXP192_ID_DCDC2, 1175);  // Core 1175mV
	pmic_set_voltage(p, AXP192_ID_LDO2, 3000);   // SD 3.0
	pmic_set_power(p, AXP192_ID_LDO2, 1);
	pmic_set_voltage(p, AXP192_ID_LDO3, 3300);   // IOD0
	pmic_set_power(p, AXP192_ID_LDO3, 1);
	pmic_set_voltage(p, AXP192_ID_LDO4, 3300);   // IOD1
	pmic_set_power(p, AXP192_ID_LDO4, 1);
#endif

	clock_init();
	return 0;
}

int board_init(void)
{
	gd->bd->bi_arch_number = MACH_TYPE_TCC893X;
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;

	return 0;
}

#ifdef CONFIG_CMD_MMC
int board_mmc_init(bd_t *bd)
{
	int ret = 0;

#if defined(CONFIG_TCC_SDHCI)
	#if defined(CONFIG_TCC_SDHCI3)
		ret = tcc_sdhci_init(TCC_SDMMC3_BASE, 3, 4);
	#endif
	#if defined(CONFIG_TCC_SDHCI2)
		ret = tcc_sdhci_init(TCC_SDMMC2_BASE, 2, 4);
	#endif
#endif
	return ret;
}
#endif

// setup for max physical memory
int dram_init(void)
{
	gd->ram_size = get_ram_size((long *)CONFIG_SYS_SDRAM_BASE, TOTAL_SDRAM_SIZE);

	return 0;
}
 
// for ATAG_MEM
void dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = TOTAL_SDRAM_SIZE;
}

// for ATAG_REVISION
u32 get_board_rev(void)
{
	return CONFIG_TCC_BOARD_REV;
}


#ifdef CONFIG_DISPLAY_BOARDINFO
int checkboard(void)
{
	printf("Board:\tD-Audio\n");
	return 0;
}
#endif


#ifdef CONFIG_LCD
#define GPIO_LCD_ON			( GPIO_PORTEXT1|1)
#define GPIO_LCD_BL			TCC_GPF(16)
#define GPIO_LCD_DISPLAY	TCC_GPB(7)
#define GPIO_LCD_RESET		( GPIO_PORTEXT3|10 )
#define LCDC_NUM			1


const unsigned char logo_data[] =
{
#include "logo_480x272.c"
};

unsigned int get_board_fb_base(void)
{
	vidinfo_t vid;
	get_panel_info(&vid);
	
	return FB_SCALE_MEM_BASE - (vid.vl_col * vid.vl_row * NBITS(vid.vl_bpix) / 8);;
}

void init_panel_info(vidinfo_t *vid)
{
	vid->dev.power_on = GPIO_LCD_ON;
	vid->dev.display_on = GPIO_LCD_DISPLAY;
	vid->dev.bl_on = GPIO_LCD_BL;
	vid->dev.reset = GPIO_LCD_RESET;
	vid->dev.lcdc_num = LCDC_NUM;
	vid->dev.logo_data = (unsigned)logo_data;
}
#endif
