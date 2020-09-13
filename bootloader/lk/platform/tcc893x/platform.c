/*
 * Copyright (C) 2010 Telechips, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <debug.h>
#include <kernel/thread.h>
#include <platform/debug.h>
#include <dev/fbcon.h>
#include <dev/uart.h>
#include <dev/adc.h>
#include <platform/gpio.h>
#include <reg.h>
#include <platform/iomap.h>
#include <i2c.h>

#ifdef TCC_CHIP_REV
#include "tcc_chip_rev.h"
#endif//TCC_CHIP_REV

#ifdef TCC_CM3_USE
#include "tcc_cm3_control.h"
#endif

#if defined(TARGET_BOARD_DAUDIO)
#include <daudio_ver.h>

#define ADC_READ_COUNT	10
#endif

#ifdef DAUDIO_USE_PMIC_LTC3676
#include <dev/ltc3676.h>
#endif

#include "../../../app/aboot/recovery.h"

#define IOBUSCFG_HCLKEN0	(TCC_IOBUSCFG_BASE + 0x10)
#define IOBUSCFG_HCLKEN1	(TCC_IOBUSCFG_BASE + 0x14)
#define PMU_WATCHDOG		(TCC_PMU_BASE + 0x04)
#define PMU_CONFIG1			(TCC_PMU_BASE + 0x10)
#define PMU_USSTATUS		(TCC_PMU_BASE + 0x18)

static struct fbcon_config *fb_config;

extern void clock_init_early(void);
extern void clock_init(void);

void platform_init_interrupts(void);
void platform_init_timer();
void pca953x_init(void);
void uart_init(void);

unsigned int system_rev;

#if defined(TARGET_BOARD_DAUDIO)
void init_daudio_rev(void)
{
	int i;
	int ret = -1;
	unsigned long daudio_borad_revs[4] = {0, };

	gpio_adc_input_init_early();

	adc_init_early();
	for (i = 0; i < 4; i++)		//GPIO_ADC[2] ~ GPIO_ADC[5]
	{
		int count = ADC_READ_COUNT;
		unsigned long adc = 0;
		while (count-- > 0)
		{
			adc += adc_read(i + 2);
		}
		//daudio_borad_revs[i] = adc_read(i + 2);
		daudio_borad_revs[i] = adc / ADC_READ_COUNT;
	}
	adc_power_down();

	init_daudio_ver(daudio_borad_revs);
}
#endif

void platform_early_init(void)
{
	system_rev = HW_REV;

	/* initialize clocks */
	clock_init_early();

#if defined(TARGET_BOARD_DAUDIO)
	init_daudio_rev();
#endif

	/* initialize GPIOs */
	gpio_init_early();

#ifdef TCC_CM3_USE
	/* turn off the cache */
	arch_disable_cache(UCACHE);
#endif
	/* initialize the interrupt controller */
	platform_init_interrupts();
#ifdef TCC_CM3_USE
	/* turn the cache back on */
	arch_enable_cache(UCACHE);
#endif

	/* initialize the timer block */
	platform_init_timer();

	/* initialize the uart */
	uart_init_early();

#if defined(TCC_I2C_USE)
	i2c_init_early();
#endif

#if !defined(TARGET_BOARD_DAUDIO)
	adc_init_early();
#endif
}

#if defined(CONFIG_SNAPSHOT_BOOT)
extern time_t time_logo;
#endif

void platform_init(void)
{
#if defined(TCC_I2C_USE)
	i2c_init();
#endif

	clock_init();

#if defined(TCC_PCA953X_USE)
	pca953x_init();
#endif

#if defined(DAUDIO_USE_PMIC_LTC3676)
	ltc3676_init();
#endif

#ifdef TCC_CHIP_REV
	tcc_extract_chip_revision();
#endif//TCC_CHIP_REV

	uart_init();

#if defined(CONFIG_DRAM_AUTO_TRAINING)
	sdram_test();
#endif

/**
 * @author valky@cleinsoft
 * @date 2013/11/14
 * TCC splash boot logo patch
 **/
#if defined(BOOTSD_INCLUDE)
	if (target_is_emmc_boot())
	{
		emmc_boot_main();
	}
#endif

#if !defined(TARGET_BOARD_DAUDIO)
	adc_init();
#endif

#if defined(CONFIG_QB_STEP_LOG)
	dprintf(CRITICAL, "\x1b[1;33m QB: Start DISPLAY LOGO!!! \x1b[0m \n");
#endif
#if defined(DISPLAY_SPLASH_SCREEN) || defined(DISPLAY_SPLASH_SCREEN_DIRECT)
	#ifndef DEFAULT_DISPLAY_OUTPUT_DUAL
		#if defined(CONFIG_SNAPSHOT_BOOT)
		time_logo = current_time();
		#endif
		display_init();
		#if defined(CONFIG_SNAPSHOT_BOOT)
		time_logo = current_time() - time_logo;
		#endif
		dprintf(INFO, "Display initialized\n");
		//disp_init = 1;
	#endif

#endif

    display_booting_logo();

	dprintf(INFO, "%s finished\n", __func__);
}

void display_init(void)
{
#if defined(TCC_LCD_USE)
	fb_config = lcdc_init();
	ASSERT(fb_config);
	fbcon_setup(fb_config);	
#endif
}

void reboot(unsigned reboot_reason)
{
	unsigned int usts;

	if (reboot_reason == 0x77665500) {
		usts = 1;
	} else {
		usts = 0;
	}

	writel(usts, PMU_USSTATUS);

	writel((readl(PMU_CONFIG1) & 0xCFFFFFFF), PMU_CONFIG1);

	writel(0xFFFFFFFF, IOBUSCFG_HCLKEN0);
	writel(0xFFFFFFFF, IOBUSCFG_HCLKEN1);

	while (1) {
		writel((1 << 31) | 1, PMU_WATCHDOG);
	}
}

unsigned check_reboot_mode(void)
{
	static bool sbSet=false;
	unsigned int usts;
	unsigned int mode;

	if( false==sbSet)
	{
		struct recovery_message Msg;

		if( emmc_get_recovery_msg(&Msg) == 0 )
		{
			Msg.command[sizeof(Msg.command)-1]=0;

			if( 0==strcmp("boot-bootloader", Msg.command) )
			{
				writel(1, PMU_USSTATUS);
				memset(&Msg, 0, sizeof(Msg));
				if( 0!=emmc_set_recovery_msg(&Msg) ) // clear message.
					dprintf(INFO, "[fail]emmc_set_recovery_msg\n");
			}
			else if( 0==strcmp("boot-recovery", Msg.command) )
			{
				writel(2, PMU_USSTATUS);
			}
			else if( 0==strcmp("boot-force_normal", Msg.command) )
			{
				writel(3, PMU_USSTATUS);
			}
			else
			{
				writel(0, PMU_USSTATUS);
			}

		}
		else
		{
			dprintf(INFO, "[fail]emmc_get_recovery_msg\n");
		}

		sbSet=true;
	}
	/* XXX: convert reboot mode value because USTS register
	 * hold only 8-bit value
	 */
	usts = readl(PMU_USSTATUS);
	printf("usts(0x%08X)\n", usts);
	switch (usts) {
	case 1:
		mode = 0x77665500;	/* fastboot mode */
		break;

	case 2:
		mode = 0x77665502;	/* recovery mode */
		break;

	case 3:
		mode = 0x77665503;	/* force normal mode */
		break;

	default:
		mode = 0x77665501;
		break;
	}

	dprintf(SPEW, "reboot mode = 0x%x\n", mode);
	return mode;
}

unsigned is_reboot_mode(void)
{
	return check_reboot_mode();
}

#if 0
unsigned char daudio_board_rev(void)
{
	return daudio_borad_rev;
}
#endif
