/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <compiler.h>
#include <debug.h>
#include <string.h>
#include <app.h>
#include <arch.h>
#include <platform.h>
#include <target.h>
#include <lib/heap.h>
#include <kernel/thread.h>
#include <kernel/timer.h>
#include <kernel/dpc.h>

extern void *__ctor_list;
extern void *__ctor_end;
extern int __bss_start;
extern int _end;

#if defined(TCC93XX)
#define RECOVERY_MODE   0x02
#define FASTBOOT_MODE   0x01
#else
#define RECOVERY_MODE   0x77665502
#define FASTBOOT_MODE   0x77665500
#endif

#if defined(TARGET_TCC8930ST_EVM)
	#define LCDC_NUM	0
#else
	#define LCDC_NUM	1
#endif

#if defined(TARGET_BOARD_DAUDIO)
#include <daudio_ver.h>

static char *daudio_hw_versions[] =
{
	"DAUDIO_HW_1ST",
	"DAUDIO_HW_2ND",
	"DAUDIO_HW_3RD",
	"DAUDIO_HW_4TH",
	"DAUDIO_HW_5TH",
	"DAUDIO_HW_6TH",
	"DAUDIO_HW_7TH",
	"DAUDIO_HW_8TH",
	"DAUDIO_HW_9TH",
};

//see also platform/tcc893x/include/daudio_ver.h - typedef enum daudio_ver_data
static char *daudio_board_versions[] =
{
	"Reserved",
	"Reserved",
	"D-Audio US(5TH)",
	"D-Audio+(6TH)",
	"Platform V.03",
	"Platform V.04",
	"Platform V.05",
	"Platform V.06",
	"Platform V.07",
	"Platform V.08",
	"Platform V.09",
};

//see also platform/tcc893x/include/daudio_ver.h - typedef enum daudio_ver_bt
static char *daudio_bt_versions[] =
{
	"BT 2MODULE",
	"BT 1MODULE",
	"Reserved",
	"BT ONLY CK",
	"BT ONLY SMD",
};

static char *daudio_gps_versions[] =
{
	"KET GPS 7Inch",
	"TRIMBLE 8Inch LG",
	"KET GPS 8Inch LG",
	"TRIMBLE 8Inch AUO",
	"TRIMBLE 7Inch",
	"KET GPS 8Inch AUO",
	"Reserved"
};

extern unsigned long daudio_borad_revs[4];
#endif

unsigned char board_rev = 0;

static int bootstrap2(void *arg);

#if (ENABLE_NANDWRITE)
void bootstrap_nandwrite(void);
#endif

static void call_constructors(void)
{
	void **ctor;
   
	ctor = &__ctor_list;
	while(ctor != &__ctor_end) {
		void (*func)(void);

		func = (void (*)())*ctor;

		func();
		ctor++;
	}
}

/* called from crt0.S */
void kmain(void) __NO_RETURN __EXTERNALLY_VISIBLE;
void kmain(void)
{
	time_t t1;
#if defined(TARGET_BOARD_DAUDIO)
	long lcd_rev_voltage[4] = {0, };
	unsigned char daudio_vers[4] = {0, };
	int i;
#endif

	// get us into some sort of thread context
	thread_init_early();

	// early arch stuff
	arch_early_init();

	// do any super early platform initialization
	platform_early_init();

	// do any super early target initialization
	target_early_init();

	// initialize kernel timers
	//dprintf(SPEW, "initializing timers\n");
	timer_init();

	dprintf(INFO, "\n");

#if defined(TARGET_BOARD_DAUDIO)

#if defined(BSP_CLEINSOFT_VERSION)
	dprintf(INFO, "***** BOOTLOADER BSP VERSION: %d *****\n", BSP_CLEINSOFT_VERSION);
#endif

	daudio_vers[DAUDIO_VER_HW] = get_daudio_hw_ver();
	daudio_vers[DAUDIO_VER_MAIN] = get_daudio_main_ver();
	daudio_vers[DAUDIO_VER_BT] = get_daudio_bt_ver();
	daudio_vers[DAUDIO_VER_GPS] = get_daudio_gps_ver();

	dprintf(INFO, "***** D-Audio HW ver: %s, adc: %d mV  *****\n",
			daudio_hw_versions[daudio_vers[DAUDIO_VER_HW]], get_daudio_hw_adc());
	dprintf(INFO, "***** D-Audio Platform ver: %s, adc: %d mV *****\n",
			daudio_board_versions[daudio_vers[DAUDIO_VER_MAIN]], get_daudio_main_adc());
	dprintf(INFO, "***** D-Audio BT ver: %s, adc: %d mV *****\n",
			daudio_bt_versions[daudio_vers[DAUDIO_VER_BT]], get_daudio_bt_adc());
	dprintf(INFO, "***** D-Audio GPS ver: %s, adc: %d mV *****\n",
			daudio_gps_versions[daudio_vers[DAUDIO_VER_GPS]], get_daudio_gps_adc());
#endif

	dprintf(INFO, "welcome to lk (%s)\n", LK_VERSION);

	// deal with any static constructors
	dprintf(SPEW, "calling constructors\n");
	t1 = current_time();
	call_constructors();
	print_process_time("call_constructors", t1);
	t1 = current_time();

	// bring up the kernel heap
	dprintf(SPEW, "initializing heap\n");
	heap_init();

	print_process_time("heap_init", t1);
	t1 = current_time();

	// initialize the threading system
	dprintf(SPEW, "initializing threads\n");
	thread_init();

	print_process_time("thread_init", t1);
	t1 = current_time();

	// initialize the dpc system
	dprintf(SPEW, "initializing dpc\n");
	dpc_init();

	print_process_time("dpc_init", t1);
	t1 = current_time();

#if (!ENABLE_NANDWRITE)
	// create a thread to complete system initialization
	dprintf(SPEW, "creating bootstrap completion thread\n");
	thread_resume(thread_create("bootstrap2", &bootstrap2, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));

	print_process_time("creating bootstrap completion threa", t1);
	t1 = current_time();

	// enable interrupts
	exit_critical_section();

#ifdef BOOT_REMOCON
	if(!check_fwdn_mode() && is_reboot_mode()!=RECOVERY_MODE){
		printf("Press Power Button to booting\n");
#if (1)
		remocon_probe();
		while(getRemoconState()==0){
			tcc_pm_enter(0);
		}
#else
		tcc_pm_enter(0);
#endif
		printf("Start!!!!!!!!!!!!!!!!!!!!!!!\n");
	}
#endif

	// become the idle thread
	thread_become_idle();
#else
        bootstrap_nandwrite();
#endif
}

int main(void);

#ifdef TCC_CM3_USE
#include "tcc_cm3_control.h"
#endif
static int bootstrap2(void *arg)
{
	time_t t1;
	unsigned char c = 0;

#ifdef BOOT_REMOCON    
    if(getRemoconState() == 2)
    {
        thread_sleep(5*1000);
        if(!isMagicKey())    
        {    		
            dprintf(INFO, "rebooting the device\n");
            reboot_device(0);
    		while(1);
        }        
    }
#endif
	dprintf(INFO, "top of bootstrap2() start [%lu] \n", current_time());

	arch_init();

	print_process_time("arch_init", t1);
	t1 = current_time();

	// initialize the rest of the platform
	dprintf(INFO, "initializing platform\n");
	platform_init();

	/**
	 * @author legolamp@cleinsoft
	 * @date 2014/11/21
	 * move Dram Overclock test because CM3 use SRAM
	 */
	if(getc(&c) >= 0)
	{
		printf("==========================================\n");
		printf("getc: %c\n", c);
		if (c == 't')
		{
#if defined(TARGET_BOARD_DAUDIO)
			lcdc_display_off(LCDC_NUM);
			daudio_test_start();
			lcdc_display_on(LCDC_NUM);
#endif
		}
		if (c == 'm')
		{
#if defined(TARGET_BOARD_DAUDIO)
			lcdc_display_off(LCDC_NUM);
			dram_overclock_test(1);
			lcdc_display_on(LCDC_NUM);
#endif
		}
	}
	print_process_time("platform_init", t1);
	t1 = current_time();

#ifdef TCC_CM3_USE
	if( (0==check_fwdn_mode()) && (RECOVERY_MODE!=check_reboot_mode()) )
	{
		tcc_cm3_ctrl_ioctl(IOCTL_CM3_CTRL_ON, 0);
		mdelay(800);

		print_process_time("tcc_cm3_ctrl_ioctl ", t1);
		t1=current_time();
	}
#endif

	// initialize the target
	dprintf(INFO, "initializing target\n");
	target_init();

	print_process_time("target_init ", t1);
	t1 = current_time();

	dprintf(SPEW, "calling apps_init()\n");
	apps_init();

	print_process_time("apps_init ", t1);
	t1 = current_time();

	return 0;
}

#if (ENABLE_NANDWRITE)
void bootstrap_nandwrite(void)
{
	dprintf(SPEW, "top of bootstrap2()\n");

	arch_init();

	// initialize the rest of the platform
	dprintf(SPEW, "initializing platform\n");
	platform_init();

	// initialize the target
	dprintf(SPEW, "initializing target\n");
	target_init();

	dprintf(SPEW, "calling nandwrite_init()\n");
	nandwrite_init();

	return 0;
}
#endif

#ifdef ENABLE_EMMC_TZOS
#include "tzos.h"
void copy_tzos(void)
{
	memcpy((void *)TZOS_ADDR, (void *)(MEMBASE+LK_SIZE), TZOS_SIZE);
}
#endif
