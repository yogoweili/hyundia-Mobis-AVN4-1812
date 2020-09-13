/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 *
 * Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
 * Copyright (c) 2009-2012, Telechips, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <app.h>
#include <debug.h>
#include <arch/arm.h>
#include <dev/udc.h>
#include <string.h>
#include <kernel/thread.h>
#include <arch/ops.h>
#include <platform.h>

#include <dev/flash.h>
#include <lib/ptable.h>
#include <dev/keys.h>
#include <dev/fbcon.h>
#include <dev/gpio.h>
#include <dev/fbcon.h>

#include "recovery.h"
#include "bootimg.h"
#include "fastboot.h"

#ifdef PLATFORM_TCC
#include <arch/tcc_used_mem.h>      // add by Telechips
#include <platform/reg_physical.h>
#include <platform/globals.h>
#include <platform/gpio.h>
#include "sparse_format.h"
#endif

#if defined(TSBM_ENABLE)
#include <tcsb/tcsb_api.h>
#endif

#ifdef BOOTSD_INCLUDE
#include <fwdn/Disk.h>
#include <sdmmc/sd_memory.h>
#include <sdmmc/emmc.h>
#if defined(_TCC9200_) || defined(_TCC9201_)
extern void sd_deinit(void);
#endif
#endif

#ifdef TCC_CM3_USE
#include <tcc_cm3_control.h>
#endif

#include <daudio_ver.h>
#include <splash/splash.h>

#ifdef CONFIG_SNAPSHOT_BOOT
#include "libsnapshot.h"
#endif

#define DEFAULT_CMDLINE "mem=100M console=null"

#define EMMC_BOOT_IMG_HEADER_ADDR 0xFF000
#if defined(TCC93XX)
#define RECOVERY_MODE   0x02
#define FASTBOOT_MODE   0x01
#else
#define RECOVERY_MODE   	0x77665502
#define FASTBOOT_MODE   	0x77665500
#define FORCE_NORMAL_MODE   0x77665503
#endif

#define ATAG_NONE       0x00000000
#define ATAG_CORE       0x54410001
#define ATAG_MEM        0x54410002
#define ATAG_VIDEOTEXT  0x54410003
#define ATAG_RAMDISK    0x54410004
#define ATAG_INITRD2    0x54420005
#define ATAG_SERIAL     0x54410006
#define ATAG_REVISION   0x54410007
#define ATAG_VIDEOLFB   0x54410008
#define ATAG_CMDLINE    0x54410009

#if defined(PLATFORM_TCC)
#define ATAG_PARTITION	0x54434370 /* TCCp*/
#else
#define ATAG_PARTITION	0x4d534d70 /* MSMp */
#endif

#define ATAG_DISPLAY	0x54434380
#define SNAPSHOT_BOOT
#if !defined(TCC_NAND_USE)
unsigned boot_into_recovery = 0;
#endif
unsigned boot_into_uboot = 0;
char qb_log[128];
char qb_sig[QB_SIG_SIZE+1];
char boot_mode_log[128];

unsigned boot_into_factory = 0;

unsigned skip_loading_quickboot = 0;
unsigned force_disable_quickboot = 0;

#define BYTE_TO_SECTOR(X)			((X + 511) >> 9)

#if defined(TARGET_BOARD_DAUDIO)
#include <dev/ltc3676.h>
#include <daudio_test.h>

#if defined(CONFIG_DAUDIO_PRINT_DEBUG_MSG)
#define DEBUG_ABOOT		1
#else
#define DEBUG_ABOOT		0
#endif

#if (DEBUG_ABOOT)
#define LKPRINT(fmt, args...) dprintf(INFO, "[cleinsoft aboot] " fmt, ##args)
#else
#define LKPRINT(args...) do {} while(0)
#endif

	#define FASTBOOT_MAX_SIZE	400 * 1024 * 1024 // 131206 LeeWonHui : change max size 300M -> 400M

	/**
	 * @author valky@cleinsoft
	 * @date 2014/04/02
	 * BOOT_OK & USB30_5V_CTL
	 **/
	#if (DAUDIO_HW_REV == 0x20131007 || DAUDIO_HW_REV == 0x20140220)
		#define GPIO_BOOT_OK		TCC_GPB(4)
	#endif

	#if (DAUDIO_HW_REV == 0x20140220)
		#define GPIO_USB30_5V_CTL	TCC_GPD(11)
	#endif
#endif

#ifdef DISPLAY_DEBUG
#define fbout(fmt, args...) fbprintf(fmt, ##args)
#else
#define fbout(fmt, args...) while(0){}
#endif

#define display_dbg(fmt, args...)\
	printf(fmt, ##args);\
	fbout(fmt, ##args);\

//static const char *emmc_cmdline = " androidboot.emmc=true";

static struct udc_device fastboot_udc_device = {
	.vendor_id	= 0x18d1,
	.product_id	= 0xD00D,
	.version_id	= 0x0100,
	.manufacturer	= "Google",
	.product	= "Android",
};

struct atag_ptbl_entry
{
	char name[16];
	unsigned long long offset;
	unsigned long long size;
	unsigned flags;
};

struct atag_tcc_entry
{
	char output;
	char resolution;
	char hdmi_resolution;
	char composite_resolution;
	char component_resolution;
	char hdmi_mode;
};

struct atag_tcc_entry tcc_param = {
	.output	= 0,
	.resolution	= 0,
	.hdmi_resolution = 0,
	.composite_resolution = 0,
	.component_resolution = 0,
};

#ifdef CONFIG_SNAPSHOT_BOOT
unsigned int scratch_addr = SCRATCH_ADDR;
#endif
void platform_uninit_timer(void);
unsigned* target_atag_mem(unsigned* ptr);
unsigned* target_atag_panel(unsigned* ptr);
unsigned* target_atag_revision(unsigned* ptr);
#if defined(TCC_CHIP_REV)
unsigned* target_atag_chip_revision(unsigned* ptr);
#endif//TCC_CHIP_REV
unsigned board_machtype(void);
unsigned check_reboot_mode(void);
void *target_get_scratch_address(void);
int target_is_emmc_boot(void);
void reboot_device(unsigned);
void target_battery_charging_enable(unsigned enable, unsigned disconnect);
int board_get_serialno(char *serialno);
int board_get_wifimac(char *wifimac);
int board_get_btaddr(char *btaddr);
#if defined(BOOTSD_INCLUDE)
void cmd_flash_emmc_sparse_img(const char* arg , void* data , unsigned sz);
#endif

#if defined(TNFTL_V8_INCLUDE)
void cmd_flash_sparse_img(const char* arg , void* data , unsigned sz);
#endif

#if !defined(TNFTL_V8_INCLUDE)
static void ptentry_to_tag(unsigned **ptr, struct ptentry *ptn)
{
	struct atag_ptbl_entry atag_ptn;
#if defined(PLATFORM_TCC)
	struct flash_info *flash_info = flash_get_info();
	unsigned block_size = flash_info->page_size * flash_info->block_size;
#endif

	memcpy(atag_ptn.name, ptn->name, 16);
	atag_ptn.name[15] = '\0';
#if defined(PLATFORM_TCC)
	atag_ptn.offset = (unsigned long long)( ( ptn->start ) - flash_info->offset) * block_size;
	atag_ptn.size = (unsigned long long) (ptn->length * block_size);
#else
	atag_ptn.offset =(unsigned long long) ptn->start;
	atag_ptn.size = (unsigned long long) ptn->length;
#endif
	atag_ptn.flags = ptn->flags;
	memcpy(*ptr, &atag_ptn, sizeof(struct atag_ptbl_entry));
	*ptr += sizeof(struct atag_ptbl_entry) / sizeof(unsigned);
}
#endif


#if defined(CONFIG_UBOOT_FAIL_SAFE)
extern unsigned int __uboot_start;
extern unsigned int __uboot_end;
void boot_uboot(void)
{
    int i = 0, j = 0;
    unsigned int uboot_start = &__uboot_start;
    unsigned int uboot_end = &__uboot_end;
    unsigned int uboot_size = uboot_end-uboot_start;

    mdelay(10000);

    printf("## %s: __uboot_start:0x%08x, __uboot_end:0x%08x, size:0x%08x\n",
            __func__, uboot_start, uboot_end, uboot_size);


    enter_critical_section();
    arch_disable_ints();
    arm_invalidate_tlb();
    platform_uninit_timer();
    arch_flush_invalidate_dcache();
    //arch_clean_invalidate_dcache();
    arch_disable_cache(UCACHE);
    arch_disable_mmu();

    void (*entry)(void) = 0x80000000;
    memcpy((void*)0x80000000, &__uboot_start, uboot_size);

    entry();
}
#else
void boot_uboot(void) { }
#endif

void boot_linux(void *kernel, unsigned *tags,
		const char *cmdline, unsigned machtype,
		void *ramdisk, unsigned ramdisk_size)
{
	unsigned *ptr = tags;
	void (*entry)(unsigned,unsigned,unsigned*) = kernel;
	struct ptable *ptable;
	size_t cmdline_len;

	/* CORE */
	*ptr++ = 2;
	*ptr++ = ATAG_CORE;

	if (ramdisk_size) {
		*ptr++ = 4;
		*ptr++ = ATAG_INITRD2;
		*ptr++ = (unsigned)ramdisk;
		*ptr++ = ramdisk_size;
	}

	ptr = target_atag_mem(ptr);

	if (cmdline) {
		cmdline_len = strlen(cmdline);
		*ptr++ = 2 + ((cmdline_len + 3) / 4);
		*ptr++ = ATAG_CMDLINE;
		strcpy(ptr, cmdline);
		ptr += (cmdline_len + 3) / 4;
	}

	/* tag for the board revision */
	ptr = target_atag_revision(ptr);

	ptr = target_atag_is_camera_enable(ptr);

#if defined(TCC_CHIP_REV)
	ptr = target_atag_chip_revision(ptr);
#endif//defined(TCC_CHIP_REV)

#if defined(ATAG_DAUDIO)
	ptr = target_atag_daudio_lk_version(ptr);
	ptr = target_atag_daudio_board_version(ptr);
	ptr = target_atag_daudio_em_setting_info(ptr);
	ptr = target_atag_daudio_quickboot_info(ptr);
	ptr = target_atag_daudio_board_adc(ptr);
#endif

#if defined(NAND_BOOT_INCLUDE) && !defined(TNFTL_V8_INCLUDE)
	//if (!target_is_emmc_boot())
	{
		/* Skip NAND partition ATAGS for eMMC boot */
		if ((ptable = flash_get_ptable()) && (ptable->count != 0)) {
			int i, pt_count = 0;
			unsigned *p = ptr;

			*ptr++ = 2;
			*ptr++ = ATAG_PARTITION;
			for (i = 0; i < ptable->count; ++i) {
				struct ptentry *ptn = ptable_get(ptable, i);
				if (!(ptn->flags & PART_HIDDEN)) {
					ptentry_to_tag(&ptr, ptn);
					++pt_count;
				}
			}
			*p += pt_count * (sizeof(struct atag_ptbl_entry) /
					sizeof(unsigned));
		}
	}
#endif

#if defined(PLATFORM_TCC) && defined(TCC_LCD_USE)
	ptr = target_atag_panel(ptr);
#endif

	if(tcc_param.resolution == 0xFF)
	{
		tcc_param.output = 0x80;
		tcc_param.resolution = 0;
		//tcc_param.hdmi_resolution = 0;
		//tcc_param.composite_resolution = 0;
		//tcc_param.component_resolution = 0;
	}

	*ptr++ = 8;
	*ptr++ = ATAG_DISPLAY;
	*ptr++ = (unsigned)tcc_param.output;
	*ptr++ = (unsigned)tcc_param.resolution;
	*ptr++ = (unsigned)tcc_param.hdmi_resolution;
	*ptr++ = (unsigned)tcc_param.composite_resolution;
	*ptr++ = (unsigned)tcc_param.component_resolution;
	*ptr++ = (unsigned)tcc_param.hdmi_mode;

	/* END */
	*ptr++ = 0;
	*ptr++ = 0;

	dprintf(INFO, "booting linux @ %p, ramdisk @ %p (%d)\n",
			kernel, ramdisk, ramdisk_size);
	if (cmdline)
		dprintf(INFO, "cmdline: %s\n", cmdline);

	enter_critical_section();
	platform_uninit_timer();

	//arch_disable_cache(UCACHE);
	//arch_disable_mmu();

#if defined(PLATFORM_TCC893X)
#if defined(DEFAULT_DISPLAY_LCD)
	#if defined(_M805_8923_) || defined(_M805_8925_) || (HW_REV == 0x1006) || (HW_REV == 0x1008) || defined(CONFIG_SOC_TCC8925S)
		#define LCDC_NUM	0
	#else
		#define LCDC_NUM 	1
	#endif
#else
	#define LCDC_NUM	1
#endif
	
	DEV_LCDC_Wait_signal(LCDC_NUM);
#endif

	arch_disable_cache(UCACHE);
	arch_disable_mmu();

	entry(0, machtype, tags);
}

unsigned page_size = 0;
unsigned page_mask = 0;

#define ROUND_TO_PAGE(x,y) (((x) + (y)) & (~(y)))
#define BYTE_TO_SECOTR(x) (x)/512

static unsigned char buf[16384]; //Equal to max-supported pagesize

static char serialno[21];	// max length of serial number is 20(CTS sepc)
static char wifimac[18];	// xx:xx:xx:xx:xx:xx
static char btaddr[13];		// Bluetooth device address (12 chars)
//static char memtype[2];		// memory type
static char cmdline[2048];

typedef enum {
	BOOTMODE_NAND = 0,
	BOOTMODE_EMMC = 1,
	BOOTMODE_CHARGER = 2,
#if 0
	BOOTMODE_FACTORY = 3
#endif
} bootmode_t;
static bootmode_t bootmode;
int dignosticmode = 0;

static void cmdline_bootmode(char *cmdline)
{
	char s[32] = "";
	char s2[32] = "";

	switch (bootmode) {
	case BOOTMODE_NAND:
		strcpy(s, "nand");
		break;
	case BOOTMODE_EMMC:
		strcpy(s, "emmc");
		break;
	case BOOTMODE_CHARGER:
		strcpy(s, "charger");
		break;
#if 0
	case BOOTMODE_FACTORY:
		strcpy(s, "tcc_factory");
		break;
#endif
	}
	strcat(cmdline, " androidboot.mode=");
	strcat(cmdline, s);

	if (dignosticmode) {
		strcat(s2, "tcc_factory");
	} 
	strcat(cmdline, " androidboot.dignostic=");
	strcat(cmdline, s2);
}

static void cmdline_serialno(char *cmdline)
{
	char s[64];

	if( force_disable_quickboot ) {
		#if defined(_EMMC_BOOT)
			sprintf(s, " androidboot.serialno=%s resume=/dev/block/mmcblk0p10 sys.quickboot.force.disable=1", serialno);
		#else
			sprintf(s, " androidboot.serialno=%s resume=/dev/block/ndda11 sys.quickboot.force.disable=1", serialno);
		#endif
	} else {
		#if defined(_EMMC_BOOT)
			sprintf(s, " androidboot.serialno=%s resume=/dev/block/mmcblk0p10", serialno);
		#else
			sprintf(s, " androidboot.serialno=%s resume=/dev/block/ndda11", serialno);
		#endif
	}
	strcat(cmdline, s);
}

static void cmdline_wifimac(char *cmdline)
{
	char s[64];

	sprintf(s, " androidboot.wifimac=%s", wifimac);
	strcat(cmdline, s);
}

static void cmdline_btaddr(char *cmdline)
{
	char s[64];

	sprintf(s, " androidboot.btaddr=%s", btaddr);
	strcat(cmdline, s);
}

static void cmdline_memtype(char *cmdline)
{
	char s[64];
	char memtype[2] = {""};
#if defined(DRAM_DDR2)
	char temp = '1';
#elif defined(DRAM_DDR3)
	char temp = '2';
#elif ! defined(TCC88XX)
	char temp = '1';
#else
	char temp = '0';
#endif
	memtype[0] = temp;
	memtype[1] = '\0';

	sprintf(s, " androidboot.memtype=%s", memtype);
	strcat(cmdline, s);
}

static void cmdline_kpanic(char *cmdline)
{
	char s[32];
	unsigned long kpanic_base = 0;
	unsigned long kpanic_size = 0;

	if (get_partition_info("settings", &kpanic_base, &kpanic_size) )
	{
		printf("%s : get kpanic info failed...\n", __func__);
		return -1;
	}

	printf("%s : kpanic_base(%d), kpanic_size(%d)\n", __func__, kpanic_base, kpanic_size);

	sprintf(s, " tcc_kpanic_base=%d tcc_kpanic_size=%d", kpanic_base, kpanic_size);
	strcat(cmdline, s);
}

void cmdline_qb_sig(char *cmdline)
{
	char tmp_str[128];
	qb_sig[QB_SIG_SIZE+1] = '\0';
	sprintf(tmp_str, " QB_SIG[%s]", qb_sig);

	strcat(cmdline, tmp_str);
}

void cmdline_qb_log(char *cmdline)
{
	char tmp_str[140];
	qb_log[128] = '\0';
	sprintf(tmp_str, " qb_log[%s]", qb_log);

	strcat(cmdline, tmp_str);
}

void cmdline_boot_log(char *cmdline)
{
	char tmp_str[128];
	sprintf(tmp_str, " boot_mode[%s]", boot_mode_log);

	strcat(cmdline, tmp_str);
}
#if defined(BOOTSD_INCLUDE)
#include "sha/sha.h"
int boot_linux_from_emmc(void)
{
	struct boot_img_hdr *hdr = (void*) buf;
#if defined(TSBM_ENABLE)
	struct sboot_img_hdr *shdr = SCRATCH_ADDR;
#endif
	unsigned n;
	unsigned long long ptn;
	unsigned sheader_offset =0, image_offset = 0;

/**
 * @author valky@cleinsoft
 * @date 2013/11/14
 * TCC splash boot logo patch
 **/
#if 0//defined(DISPLAY_SPLASH_SCREEN) && !defined(DISPLAY_SPLASH_SCREEN_DIRECT)
	/**
	 * @author valky@cleinsoft
	 * @date 2013/10/1
	 * for tcc splash boot image
	 **/
	splash_image("bootlogo");
#endif

    /**
     * @author sjpark@cleinsoft 
     * @date 2014/10/30        
     * jump uboot if MBR broken flag is set 
     **/ 
    if(boot_into_uboot == 1){
        boot_uboot();
    }
	if(!boot_into_recovery){
		ptn = emmc_ptn_offset("boot");

		if(ptn == 0){
			fbcon_clear();
			display_dbg("ERROR : No BOOT Partition Found!!\n");
			if (keys_event_waiting() < 0)
				return -1;
		}
	}
	else{
		ptn = emmc_ptn_offset("recovery");

		if(ptn == 0){
			fbcon_clear();
			display_dbg("ERROR : No Recvoery Partition Found!!\n");
			if (keys_event_waiting() < 0)
				return -1;
		}
	}

#if !defined(TSBM_ENABLE)
	if(emmc_read((ptn+BYTE_TO_SECTOR(image_offset)), sizeof(boot_img_hdr), (unsigned int*)buf))
	{
		fbcon_clear();
		display_dbg( "ERROR : Cannot read boot imgage header\n");
		if (keys_event_waiting() < 0)
			return -1;
	}
#else
	if(emmc_read((ptn + BYTE_TO_SECTOR(sheader_offset)), page_size, (unsigned int*)shdr))
	{
		fbcon_clear();
		display_dbg("ERROR : Cannot read boot imgage header\n");
		if (keys_event_waiting() < 0)
			return -1;
	}
	if(memcmp(shdr->magic, SBOOT_MAGIC, SBOOT_MAGIC_SIZE)){
		fbcon_clear();
		display_dbg("ERROR: Invalid boot image header\n");
		if (keys_event_waiting() < 0)
			return -1;
	}
	sheader_offset = sizeof(struct sboot_img_hdr);
	memset(SKERNEL_ADDR, 0x0, shdr->image_size);

	if(emmc_read((ptn + BYTE_TO_SECTOR(sheader_offset)), shdr->image_size, SKERNEL_ADDR))
	{
		fbcon_clear();
		display_dbg("ERROR : Cannot read boot imgage header\n");
		if (keys_event_waiting() < 0)
			return -1;
	}

	if(tcsb_api_check_secureboot(600, NULL, NULL) == 0)
	{
		fbcon_clear();
		display_dbg("ERROR : it does not support secure boot\n");
		if (keys_event_waiting() < 0)
			return -1;
	}
	else
	{
		if(tcsb_api_decrypt(SKERNEL_ADDR, SCRATCH_ADDR, shdr->image_size) == NULL)
		{
			fbcon_clear();
			display_dbg("ERROR : secure boot kernel image verify failed !!\n");
			if (keys_event_waiting() < 0)
				return -1;
		}
		memcpy(buf, SCRATCH_ADDR , page_size);
	}

#endif

	if(memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)){
		fbcon_clear();
		display_dbg("ERROR: Invalid boot image header\n");
		if (keys_event_waiting() < 0)
			return -1;
	}

	if (hdr->page_size && (hdr->page_size != page_size)) {
		page_size = hdr->page_size;
		page_mask = page_size - 1;
	}

	image_offset += page_size;
	n = ROUND_TO_PAGE(hdr->kernel_size, page_mask);

#if defined(TSBM_ENABLE)
	if (!memcpy((void *)hdr->kernel_addr, SCRATCH_ADDR + image_offset, n))
#else
	if (emmc_read(ptn+BYTE_TO_SECTOR(image_offset), n, (void*)hdr->kernel_addr))
#endif
	{
		fbcon_clear();
		display_dbg("ERROR: Cannot read kernel image\n");
		if (keys_event_waiting() < 0)
			return -1;
	}

	n = ROUND_TO_PAGE(hdr->kernel_size, hdr->page_size-1); //base size is different from nand.
	image_offset += n;
	n = ROUND_TO_PAGE(hdr->ramdisk_size, page_mask);

#if defined(TSBM_ENABLE)
	if (!memcpy((void*)hdr->ramdisk_addr, SCRATCH_ADDR + image_offset, n))
#else
	if(emmc_read(ptn+BYTE_TO_SECTOR(image_offset), n , (void*)hdr->ramdisk_addr))
#endif
	{
		fbcon_clear();
		display_dbg("ERROR: Cannot read ramdisk image\n");
		if (keys_event_waiting() < 0)
			return -1;
	}
	image_offset += n;

#if 1
	SHA_CTX ctx;
	uint8_t* sha;
	uint8_t* sha_preserved;
	int i = 0;
	int j = 0;
	unsigned char c = -1;
	memset(&ctx, 0x0, sizeof(SHA_CTX));

	SHA_init(&ctx);
	SHA_update(&ctx, hdr->kernel_addr, hdr->kernel_size);
	SHA_update(&ctx, &hdr->kernel_size, sizeof(hdr->kernel_size));
	SHA_update(&ctx, hdr->ramdisk_addr, hdr->ramdisk_size);
	SHA_update(&ctx, &hdr->ramdisk_size, sizeof(hdr->ramdisk_size));
	SHA_update(&ctx, hdr->second_addr, hdr->second_size);
	SHA_update(&ctx, &hdr->second_size, sizeof(hdr->second_size));
	sha = SHA_final(&ctx);

	j = SHA_DIGEST_SIZE > sizeof(hdr->id) ? sizeof(hdr->id) : SHA_DIGEST_SIZE;

	i = memcmp(hdr->id, sha, j);

	printf("[SHA] Result of comparison : %d\n", i);
	if (i != 0)
	{
		fbcon_clear();
		sha_preserved = (uint8_t*)&hdr->id[0];
		display_dbg("[SHA] Comparision failed ...\n");
		display_dbg("|| --- Calculated Value --- || --- Preserved Value --- ||\n");
		for (i=0; i<j; i++) {
			display_dbg("||       sha(%02d):0x%02X       ||       sha(%02d):0x%02X      ||\n",
					i, (unsigned int)*(sha+i), i, (unsigned int)*(sha_preserved+i));
		}
		display_dbg("|| ------------------------ || ----------------------- ||\n\n");
		if (keys_event_waiting() < 0)
			return -1;
	}
#endif

unified_boot:
	dprintf(INFO, "\nkernel  @ %x (%d bytes)\n", hdr->kernel_addr,
			hdr->kernel_size);
	dprintf(INFO, "ramdisk @ %x (%d bytes)\n", hdr->ramdisk_addr,
			hdr->ramdisk_size);

	if(hdr->cmdline[0]) {
		strcpy(cmdline, (char*) hdr->cmdline);
	} else {
		strcpy(cmdline, DEFAULT_CMDLINE);
	}
	cmdline_serialno(cmdline);
	cmdline_wifimac(cmdline);
	cmdline_btaddr(cmdline);
	cmdline_bootmode(cmdline);
	cmdline_memtype(cmdline);
	cmdline_kpanic(cmdline);
#ifdef CONFIG_SNAPSHOT_BOOT
	cmdline_boot_log(cmdline);
	cmdline_qb_sig(cmdline);
	cmdline_qb_log(cmdline);
#endif
	dprintf(SPEW, "cmdline = '%s'\n", cmdline);

	/* TODO: create/pass atags to kernel */

	dprintf(INFO, "\nBooting Linux\n");
	boot_linux((void *)hdr->kernel_addr, (void *)TAGS_ADDR,
			(const char *)cmdline, board_machtype(),
			(void *)hdr->ramdisk_addr, hdr->ramdisk_size);


	return 0;
}
#endif

#if defined(TNFTL_V8_INCLUDE)
int boot_linux_from_flash_v8(void)
{
	struct boot_img_hdr *hdr = (void*) buf;
	#if defined(TSBM_ENABLE)
	struct sboot_img_hdr *shdr = SCRATCH_ADDR;
	#endif
	unsigned n;
	unsigned long long ptn;
	unsigned offset = 0, sheader_offset=0;

	unsigned int i;
#if defined(DISPLAY_SPLASH_SCREEN) && !defined(DISPLAY_SPLASH_SCREEN_DIRECT)
	if(snapshot_boot_header())
		splash_image("quickboot");
	else
		splash_image("bootlogo");
#endif

	if(!boot_into_recovery){
		ptn = flash_ptn_offset("boot");

		if(ptn == 0){
			dprintf(CRITICAL , "ERROR : No BOOT Partition Found!!\n");
			return -1;
		}
	}
	else{
		ptn = flash_ptn_offset("recovery");

		if(ptn == 0){
			dprintf(CRITICAL , "ERROR : No Recvoery Partition Found!!\n");
			return -1;
		}
	}

	#if !defined(TSBM_ENABLE)
	if(flash_read_tnftl_v8((ptn + BYTE_TO_SECTOR(offset)), page_size, (unsigned int*)buf))
	{
		dprintf(CRITICAL , "ERROR : Cannot read boto imgage header\n");
		return -1;
	}
	#else
	if(flash_read_tnftl_v8((ptn + BYTE_TO_SECTOR(sheader_offset)), page_size, (unsigned int*)shdr))
	{
		dprintf(CRITICAL , "ERROR : Cannot read boot imgage header\n");
		return -1;
	}
	if(memcmp(shdr->magic, SBOOT_MAGIC, SBOOT_MAGIC_SIZE)){
		dprintf(CRITICAL ,"ERROR: Invalid boot image header\n");
		return -1;
	}
	sheader_offset = sizeof(struct sboot_img_hdr);
	memset(SKERNEL_ADDR, 0x0, shdr->image_size);

	if(flash_read_tnftl_v8((ptn + BYTE_TO_SECTOR(sheader_offset)), shdr->image_size, SKERNEL_ADDR))
	{
		dprintf(CRITICAL , "ERROR : Cannot read boot imgage header\n");
		return -1;
	}

	if(tcsb_api_check_secureboot(600, NULL, NULL) == 0)
	{
		dprintf(CRITICAL , "ERROR : it does not support secure boot\n");
		return -1;
	}
	else
	{
		if(tcsb_api_decrypt(SKERNEL_ADDR, SCRATCH_ADDR, shdr->image_size) == NULL)
		{
			dprintf(CRITICAL , "ERROR : secure boot kernel image verify failed !!\n");
			return -1;
		}
		memcpy(buf, SCRATCH_ADDR , page_size);
	}
	#endif

	if(memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)){
		dprintf(CRITICAL ,"ERROR: Invalid boot image header\n");
		return -1;
	}

	if (hdr->page_size && (hdr->page_size != page_size)) {
		page_size = hdr->page_size;
		page_mask = page_size - 1;
	}

	offset += page_size;
	n = ROUND_TO_PAGE(hdr->kernel_size, page_mask);

#if defined(TSBM_ENABLE)
	if (!memcpy((void *)hdr->kernel_addr, SCRATCH_ADDR + offset, n))
#else
	if (flash_read_tnftl_v8(ptn + BYTE_TO_SECTOR(offset),n, (void *)hdr->kernel_addr))
#endif
	{
		dprintf(CRITICAL, "ERROR: Cannot read kernel image\n");
		return -1;
	}

	n = ROUND_TO_PAGE(hdr->kernel_size, hdr->page_size-1); //base size is different from nand.
	offset += n;
	n = ROUND_TO_PAGE(hdr->ramdisk_size, page_mask);

#if defined(TSBM_ENABLE)
	if (!memcpy((void*)hdr->ramdisk_addr, SCRATCH_ADDR + offset, n))
#else
	if (flash_read_tnftl_v8(ptn + BYTE_TO_SECTOR(offset), n, (void *)hdr->ramdisk_addr))
#endif
	{
		dprintf(CRITICAL, "ERROR: Cannot read ramdisk image\n");
		return -1;
	}
	offset += n;

unified_boot:
	dprintf(INFO, "\nkernel  @ %x (%d bytes)\n", hdr->kernel_addr,
			hdr->kernel_size);
	dprintf(INFO, "ramdisk @ %x (%d bytes)\n", hdr->ramdisk_addr,
			hdr->ramdisk_size);

	if(hdr->cmdline[0]) {
		strcpy(cmdline, (char*) hdr->cmdline);
	} else {
		strcpy(cmdline, DEFAULT_CMDLINE);
	}
	cmdline_serialno(cmdline);
	cmdline_wifimac(cmdline);
	cmdline_btaddr(cmdline);
	cmdline_bootmode(cmdline);
	cmdline_memtype(cmdline);
	dprintf(SPEW, "cmdline = '%s'\n", cmdline);

	/* TODO: create/pass atags to kernel */

	dprintf(INFO, "\nBooting Linux\n");
	boot_linux((void *)hdr->kernel_addr, (void *)TAGS_ADDR,
			(const char *)cmdline, board_machtype(),
			(void *)hdr->ramdisk_addr, hdr->ramdisk_size);


	return 0;
}

#endif

void cmd_boot(const char *arg, void *data, unsigned sz)
{
	unsigned kernel_actual;
	unsigned ramdisk_actual;
	static struct boot_img_hdr hdr;
	char *ptr = ((char*) data);

	if (sz < sizeof(hdr)) {
		fastboot_fail("invalid bootimage header");
		return;
	}

	memcpy(&hdr, data, sizeof(hdr));

	/* ensure commandline is terminated */
	hdr.cmdline[BOOT_ARGS_SIZE-1] = 0;

	kernel_actual = ROUND_TO_PAGE(hdr.kernel_size, page_mask);
	ramdisk_actual = ROUND_TO_PAGE(hdr.ramdisk_size, page_mask);

	if (page_size + kernel_actual + ramdisk_actual < sz) {
		fastboot_fail("incomplete bootimage");
		return;
	}

	memmove((void*) KERNEL_ADDR, ptr + page_size, hdr.kernel_size);
	memmove((void*) RAMDISK_ADDR, ptr + page_size + kernel_actual,
			hdr.ramdisk_size);

	fastboot_okay("");
	target_battery_charging_enable(0, 1);

#if defined(TCC_USB_USE)
	udc_stop();
#endif

	boot_linux((void*) KERNEL_ADDR, (void*) TAGS_ADDR,
			(const char*) hdr.cmdline, board_machtype(),
			(void*) RAMDISK_ADDR, hdr.ramdisk_size);
}
void cmd_erase_emmc(const char* arg , void *data, unsigned sz)
{
#if 0
	printf("cmd_erase_emmc, arg : %s\n", arg);
	unsigned long long ptn;
	unsigned char *erase_buffer = malloc(sizeof(char)*4*1024);
	memset(erase_buffer, 0x0 , sizeof(char)*4*1024);

	ptn = emmc_ptn_offset(arg);

	printf("[ cmd_erase_emmc ] ptn : %llu\n", ptn);

	if(!emmc_write(ptn , sizeof(char)*4*1024, erase_buffer)){
		fastboot_okay("");
		printf("complete\n");
	}
	else 
		fastboot_fail("emmcpartition erase fail");

#else
	int res = 0;
	unsigned long start_addr = 0;
	unsigned long erase_size = 0;

	printf("cmd_erase_emmc, arg : %s\n", arg);
	res = get_partition_info(arg, &start_addr, &erase_size);
	if( res ) {
		printf("[cmd_erase_emmc] get_partition_info() error!\n");
		return -1;
	}

	if( !erase_emmc(start_addr, erase_size, 0) ) {
		printf("Finished!\n");
		fastboot_okay("");
	} else {
		printf("Failed...\n");
		fastboot_fail("emmcpartition erase fail");
	}
#endif
}
void cmd_erase(const char *arg, void *data, unsigned sz)
{
	#if defined(NAND_BOOT_INCLUDE)
	#if defined(TNFTL_V8_INCLUDE)
	// temporal partition erase code.
	// set 0xFF Data at first 16Kbyte of buf.
	unsigned long long ptn;
	unsigned char* erase_buffer;

	erase_buffer = malloc(sizeof(char)*16*1024);
	memset(erase_buffer , 0xFF , 16*1024);

	ptn = flash_ptn_offset(arg);

	if(!flash_write_tnftl_v8(arg, ptn , 16*1024, erase_buffer)){
		fastboot_okay("");
	}
	else
		fastboot_fail("nand partition erase fail");

	dprintf(CRITICAL, " Input 0xFF Data to NAND Partition \n");

	return ;
	#endif
	#endif
}
#if defined(BOOTSD_INCLUDE)
void cmd_flash_emmc(const char *arg, void *data , unsigned sz)
{
	unsigned long long ptn = 0;
	sparse_header_t *sparse_header;

	ptn = emmc_ptn_offset(arg);
	dprintf(INFO ,"ptn start addr : %llu \n" , ptn);

	if(ptn == -1){
		dprintf(INFO, "Partition [%s] does not found\n" , arg);
		fastboot_fail("Partition not found!!\n");
		return ;
	}
	sparse_header = (sparse_header_t*)data;
	if(sparse_header->magic != SPARSE_HEADER_MAGIC){

		if(emmc_write(ptn, sz , data)){
			fastboot_fail("falsh write failure");
			return;
		}
	}
	else{
		cmd_flash_emmc_sparse_img(arg, data, sz);
	}


	fastboot_okay("");
	return;

}
void cmd_flash_emmc_sparse_img(const char *arg, void *data , unsigned sz)
{

	unsigned int chunk;
	unsigned int chunk_data_sz;
	sparse_header_t *sparse_header;
	chunk_header_t *chunk_header;
	uint32_t total_blocks = 0;
	unsigned long long ptn = 0;

	ptn = emmc_ptn_offset(arg);
	dprintf(INFO ,"ptn start addr : %llu \n" , ptn);

	sparse_header = (sparse_header_t *)data;
	data+=sparse_header->file_hdr_sz;
	if(sparse_header->file_hdr_sz > sizeof(sparse_header_t))
	{
		/* Skip the remaining bytes in a header that is long than we expected.*/
		data += (sparse_header->file_hdr_sz - sizeof(sparse_header_t));
	}

	dprintf (INFO, "=== Sparse Image Header ===\n");
	dprintf (INFO, "magic: 0x%x\n", sparse_header->magic);
	dprintf (INFO, "major_version: 0x%x\n", sparse_header->major_version);
	dprintf (INFO, "minor_version: 0x%x\n", sparse_header->minor_version);
	dprintf (INFO, "file_hdr_sz: %d\n", sparse_header->file_hdr_sz);
	dprintf (INFO, "chunk_hdr_sz: %d\n", sparse_header->chunk_hdr_sz);
	dprintf (INFO, "blk_sz: %d\n", sparse_header->blk_sz);
	dprintf (INFO, "total_blks: %d\n", sparse_header->total_blks);
	dprintf (INFO, "total_chunks: %d\n", sparse_header->total_chunks);

	/* start processing chunks*/
	for(chunk =0; chunk<sparse_header->total_chunks; chunk++)
	{
		/* Read and skip over chunk header*/
		chunk_header = (chunk_header_t*)data;
		data +=sizeof(chunk_header_t);
#if 0
		dprintf (INFO, "=== Chunk Header ===\n");
		dprintf (INFO, "chunk_type: 0x%x\n", chunk_header->chunk_type);
		dprintf (INFO, "chunk_data_sz: 0x%x\n", chunk_header->chunk_sz);
		dprintf (INFO, "total_size: 0x%x\n", chunk_header->total_sz);
#endif

		if(sparse_header->chunk_hdr_sz > sizeof(chunk_header_t))
		{
			/* Skip the remaining bytes in a header that is long than we expected.*/
			data+= (sparse_header->chunk_hdr_sz - sizeof(chunk_header_t));
		}

		chunk_data_sz = sparse_header->blk_sz * chunk_header->chunk_sz;
		switch(chunk_header->chunk_type)
		{
			case CHUNK_TYPE_RAW:
				if(chunk_header->total_sz != (sparse_header->chunk_hdr_sz + chunk_data_sz))
				{
					fastboot_fail("bogus chunk size for chunk type raw");
					return;
				}
				if(emmc_write(ptn+(total_blocks*sparse_header->blk_sz)/512,chunk_data_sz,data))
				{
					fastboot_fail("flash write failure");
					return;
				}
				total_blocks += chunk_header-> chunk_sz;
				data+=chunk_data_sz;
				break;
			case CHUNK_TYPE_DONT_CARE:
				total_blocks += chunk_header->chunk_sz;
				break;
			case CHUNK_TYPE_CRC:
				if(chunk_header->total_sz != sparse_header->chunk_hdr_sz)
				{
					fastboot_fail("bogus chunk size ofr chunk type dont care");
					return;
				}
				total_blocks += chunk_header->chunk_sz;
				data += chunk_data_sz;
				break;
			default:
				fastboot_fail("unknown chunk type");
				return;

		}

	}

	dprintf(INFO, "Wrote %d blocks, expected to write %d blocks\n",
			total_blocks, sparse_header->total_blks);

	fastboot_okay("");
	return;
}
#endif


void cmd_flash(const char *arg, void *data, unsigned sz)
{
#if defined(TCC_NAND_USE)
#if defined(TNFTL_V8_INCLUDE)

	unsigned long long ptn = 0;
	sparse_header_t *sparse_header;

	ptn = flash_ptn_offset(arg);
	dprintf(INFO ,"ptn start addr : %llu \n" , ptn);

	if(ptn == -1){
		dprintf(INFO, "Partition [%s] does not found\n" , arg);
		fastboot_fail("Partition not found!!\n");
		return ;
	}
	sparse_header = (sparse_header_t*)data;
	if(sparse_header->magic != SPARSE_HEADER_MAGIC)
	{
		if(flash_write_tnftl_v8(arg, ptn, sz , data))
		{
			fastboot_fail("falsh write failure");
			return;
		}
	}
	else
	{
		cmd_flash_sparse_img(arg, data, sz);
	}

	fastboot_okay("");
	return;
#endif
#endif
}

void cmd_flash_sparse_img(const char *arg, void *data , unsigned sz)
{

	unsigned int chunk;
	unsigned int chunk_data_sz;
	sparse_header_t *sparse_header;
	chunk_header_t *chunk_header;
	uint32_t total_blocks = 0;
	unsigned long long ptn = 0;

	ptn = flash_ptn_offset(arg);
	dprintf(INFO ,"ptn start addr : %llu \n" , ptn);

	sparse_header = (sparse_header_t *)data;
	data+=sparse_header->file_hdr_sz;
	if(sparse_header->file_hdr_sz > sizeof(sparse_header_t))
	{
		/* Skip the remaining bytes in a header that is long than we expected.*/
		data += (sparse_header->file_hdr_sz - sizeof(sparse_header_t));
	}

	dprintf (INFO, "=== [TNFTL V8]Sparse Image Header ===\n");
	dprintf (INFO, "magic: 0x%x\n", sparse_header->magic);
	dprintf (INFO, "major_version: 0x%x\n", sparse_header->major_version);
	dprintf (INFO, "minor_version: 0x%x\n", sparse_header->minor_version);
	dprintf (INFO, "file_hdr_sz: %d\n", sparse_header->file_hdr_sz);
	dprintf (INFO, "chunk_hdr_sz: %d\n", sparse_header->chunk_hdr_sz);
	dprintf (INFO, "blk_sz: %d\n", sparse_header->blk_sz);
	dprintf (INFO, "total_blks: %d\n", sparse_header->total_blks);
	dprintf (INFO, "total_chunks: %d\n", sparse_header->total_chunks);

	/* start processing chunks*/
	for(chunk =0; chunk<sparse_header->total_chunks; chunk++)
	{
		/* Read and skip over chunk header*/
		chunk_header = (chunk_header_t*)data;
		data +=sizeof(chunk_header_t);
#if 0
		dprintf (INFO, "=== Chunk Header ===\n");
		dprintf (INFO, "chunk_type: 0x%x\n", chunk_header->chunk_type);
		dprintf (INFO, "chunk_data_sz: 0x%x\n", chunk_header->chunk_sz);
		dprintf (INFO, "total_size: 0x%x\n", chunk_header->total_sz);
#endif

		if(sparse_header->chunk_hdr_sz > sizeof(chunk_header_t))
		{
			/* Skip the remaining bytes in a header that is long than we expected.*/
			data+= (sparse_header->chunk_hdr_sz - sizeof(chunk_header_t));
		}

		chunk_data_sz = sparse_header->blk_sz * chunk_header->chunk_sz;
		switch(chunk_header->chunk_type)
		{
			case CHUNK_TYPE_RAW:
				if(chunk_header->total_sz != (sparse_header->chunk_hdr_sz + chunk_data_sz))
				{
					fastboot_fail("bogus chunk size for chunk type raw");
					return;
				}
				//if(emmc_write(ptn+(total_blocks*sparse_header->blk_sz)/512,chunk_data_sz,data))
				if( flash_write_tnftl_v8(arg, ptn+(total_blocks*sparse_header->blk_sz)/512, chunk_data_sz, data ))
				{
					fastboot_fail("flash write failure");
					return;
				}
				total_blocks += chunk_header-> chunk_sz;
				data+=chunk_data_sz;
				break;
			case CHUNK_TYPE_DONT_CARE:
				total_blocks += chunk_header->chunk_sz;
				break;
			case CHUNK_TYPE_CRC:
				if(chunk_header->total_sz != sparse_header->chunk_hdr_sz)
				{
					fastboot_fail("bogus chunk size ofr chunk type dont care");
					return;
				}
				total_blocks += chunk_header->chunk_sz;
				data += chunk_data_sz;
				break;
			default:
				fastboot_fail("unknown chunk type");
				return;

		}

	}

	dprintf(INFO, "Wrote %d blocks, expected to write %d blocks\n",
			total_blocks, sparse_header->total_blks);

	fastboot_okay("");
	return;
}



void cmd_continue(const char *arg, void *data, unsigned sz)
{
#if defined(NAND_BOOT_INCLUDE)
	fastboot_okay("");
	target_battery_charging_enable(0, 1);

#if defined(TCC_USB_USE)
	udc_stop();
#endif

#if defined(TNFTL_V8_INCLUDE)
	boot_linux_from_flash_v8();
#endif

#endif
}

void cmd_reboot(const char *arg, void *data, unsigned sz)
{
	dprintf(INFO, "rebooting the device\n");
	fastboot_okay("");
	reboot_device(0);
}

void cmd_reboot_bootloader(const char *arg, void *data, unsigned sz)
{
	dprintf(INFO, "rebooting the device\n");
	fastboot_okay("");
	reboot_device(FASTBOOT_MODE);
}

#if defined(DEFAULT_DISPLAY_OUTPUT_DUAL)
static int get_tcc_param(void)
{
	#if defined(BOOTSD_INCLUDE) || defined(TNFTL_V8_INCLUDE)
	unsigned long long ptn;
	#endif
	char* data;
	int output_mode = 0xff, output_resolution = 0;
	int hdmi_resolution = 0;
	int hdmi_mode = 0;
	int composite_resolution = 0;
	int component_resolution = 0;
	int page = 0, page_offset = 0, page_num = 0;

	#if defined(BOOTSD_INCLUDE)
		ptn = emmc_ptn_offset("tcc");
	#elif defined(TNFTL_V8_INCLUDE)
		ptn = flash_ptn_offset("tcc");
	#endif
	
	if (ptn == NULL)
	{
		printf(CRITICAL, "ERROR: No tcc partition found\n");
	}
	else
	{
		data = (char*)buf;

		#if defined(BOOTSD_INCLUDE) || defined(TNFTL_V8_INCLUDE)
			page_num = (1*1024*1024)/512;
		#endif

		for(page=0; page<page_num; page++)
		{
		#if defined(BOOTSD_INCLUDE)
			if(emmc_read(ptn + BYTE_TO_SECTOR(page_offset), 128, &data[0]))
		#elif defined(TNFTL_V8_INCLUDE)
			if(flash_read_tnftl_v8((ptn + BYTE_TO_SECTOR(page_offset)), 128, &data[0]))
		#endif
			{
				printf(CRITICAL, "ERROR: Cannot read data from tcc partition\n");
				break;
			}
			else
			{
				//printf("page:%d, data[0]=0x%02x, data[1]=0x%02x, data[2]=0x%02x, data[3]=0x%02x, data[4]=0x%02x, data[5]=0x%02x, data[6]=0x%02x, data[7]=0x%02x, data[8]=0x%02x, data[9]=0x%02x, data[10]=0x%02x\n",
				//		page, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10]);

			#if defined(BOOTSD_INCLUDE) || defined(TNFTL_V8_INCLUDE)
				if((data[0] == 0xff && data[1] == 0xff && data[2] == 0xff) || (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00))
			#endif
				{
					printf("[tcc_param] page=%d, output=%d, resolution=%d, hdmi_resolution=%d\n", page, output_mode, output_resolution, hdmi_resolution);
					printf("[tcc_param] composite_resolution=%d, component_resolution=%d, hdmi_mode=%d\n", composite_resolution, component_resolution, hdmi_mode);
					tcc_param.output = output_mode;
					tcc_param.resolution = output_resolution;
					tcc_param.hdmi_resolution = hdmi_resolution;
					tcc_param.composite_resolution = composite_resolution;
					tcc_param.component_resolution = component_resolution;
					tcc_param.hdmi_mode = hdmi_mode;
					break;
				}
				else
				{
					output_mode = data[3];
					output_resolution = data[4];
					hdmi_resolution	= data[5];
					composite_resolution = data[6];
					component_resolution = data[7];
					hdmi_mode = data[10];
				}
			}

			page_offset += page_size;
		}
	}

	return 0;
}
#endif

void aboot_init(const struct app_descriptor *app)
{
	unsigned reboot_mode = 0;
	unsigned disp_init = 0;
	unsigned char c = 0;
	int main_ver = 0;

#if defined(_EMMC_BOOT_TCC)
	char *board_name;
#endif

	board_get_serialno(serialno);
	board_get_wifimac(wifimac);
	board_get_btaddr(btaddr);

#if defined(TCC_NAND_USE)
	page_size = flash_page_size();
	page_mask = page_size - 1;
#endif

#if (defined(DISPLAY_SPLASH_SCREEN) || defined(DISPLAY_SPLASH_SCREEN_DIRECT)) && defined(TCC_LCD_USE)
#if defined(DEFAULT_DISPLAY_OUTPUT_DUAL)
	get_tcc_param();
	display_init();
	dprintf(INFO, "Diplay initialized\n");
#endif

	disp_init = 1;
#endif

#if defined(PLATFORM_TCC) && defined(FWDN_V7) && defined(TCC_FWDN_USE)
	if (check_fwdn_mode() || keys_get_state(KEY_MENU) != 0) {
		fwdn_start();
		return;
	}
#endif

#if defined(GPIO_BOOT_OK)
	//to Micom low -> high (backlight on & chage usb mode(device -> host))
	gpio_set(GPIO_BOOT_OK, 1);
	LKPRINT("%s GPIO_BOOT_OK high!\n", __func__);
#endif

#if defined(GPIO_USB30_5V_CTL)
	//to Micom low -> high (usb3.0 5V_VBUS high)
	gpio_set(GPIO_USB30_5V_CTL, 1);
#endif

#ifdef MMC_EXTEND_BOOT_PARTITION
	mmc_extend_boot_part_main();
#endif

#if defined (CONFIG_UBOOT_UPDATE_AUTOTEST)
    dprintf(INFO, "###### UBOOT AUTO TEST #######\n");
    boot_uboot();
#endif

#ifdef BOOT_REMOCON
	if(getRemoconState() == 2)
	{
		if(isMagicKey())
			boot_into_recovery = 1;
	}
	else
#endif

	main_ver = get_daudio_main_ver();
	if (main_ver >= DAUDIO_BOARD_5TH)
	{
		unsigned int loop=0;
		unsigned int value_1=0,value_2=0,value=0;
		unsigned int mode_value[3];
		// GT system start 

		for(loop = 0 ; loop < 3 ; loop++)
		{
			value_1 = gpio_get( GPIO_PORTF|26 );
			value_2 = gpio_get( GPIO_PORTF|27 );
			value = ((value_1<<1)|value_2);
			mode_value[loop]=value;
			//msleep(10); //??
			 mdelay(10);
		};
		if( mode_value[0] == mode_value[1] && mode_value[1] == mode_value[2] )
		{
			if( value == 3 )
			{
				dprintf(INFO, "[DEBUG] U-Boot Recovery Button pushed .... \n");
				boot_uboot();
			}
		}
		// GT system end
		dprintf(INFO, "###### TCC GPIOF-26-27 (%d)-(%d)  #######\n",gpio_get(TCC_GPF(26)) , gpio_get(TCC_GPF(27)) );
	}
	

	if(getc(&c) >= 0)
	{
		dprintf(INFO,"==========================================\n");
		dprintf(INFO,"getc: %c\n", c);

		if (c == 'r')
			boot_into_recovery = 1;
		if (c == 'f')
			goto fastboot;
		if (c == 'd')
			boot_into_factory = 1;
		if (c == 's')
			skip_loading_quickboot = 1;
		if (c == 'x')
			force_disable_quickboot = 1;
        if (c == 'u')
            boot_uboot();
	}
	else
	{
		boot_into_recovery = 0;
		boot_into_factory = 0;
	}

	if(boot_into_recovery == 0)
	{
		if (keys_get_state(KEY_HOME) != 0)
			boot_into_recovery = 1;
		if( keys_get_state(KEY_F1) != 0 )
			boot_into_factory = 1;
		if (keys_get_state(KEY_BACK) != 0 || keys_get_state(KEY_VOLUMEDOWN) != 0 || keys_get_state(KEY_CLEAR) != 0)
			goto fastboot;
	}

#ifdef TCC_FACTORY_RESET_SUPPORT //use key1
        #if defined(TARGET_TCC8925_STB_DONGLE)
                gpio_config(GPIO_PORTE|20, GPIO_FN0 | GPIO_INPUT); /* CVBS_DET */

                if(boot_into_recovery == 0)
                {
                        if(gpio_get(GPIO_PORTE|20) == false)
                        {
                                boot_into_recovery = 1;
                        }
                }
        #elif defined(TARGET_TCC8935_DONGLE)
		#if defined(CONFIG_CHIP_TCC8935S)
                gpio_config(GPIO_PORTE|20, GPIO_FN0 | GPIO_INPUT); /* CVBS_DET */

                if(boot_into_recovery == 0)
                {
                        if(gpio_get(GPIO_PORTE|20) == false)
                        {
                                boot_into_recovery = 1;
                        }
                }
		#else
                gpio_config(GPIO_PORTE|16, GPIO_FN0 | GPIO_INPUT); /* CVBS_DET */

                if(boot_into_recovery == 0)
                {
                        if(gpio_get(GPIO_PORTE|16) == false)
                        {
                                boot_into_recovery = 1;
                        }
                }
		#endif
        #endif
#endif

	/* determine boot mode */
	if (boot_into_factory == 1) {
		dignosticmode = 1;
	} 
	
	if (target_is_battery_charging()) {
		bootmode = BOOTMODE_CHARGER;
	} else if (target_is_emmc_boot()) {
		bootmode = BOOTMODE_EMMC;
	} else {
		bootmode = BOOTMODE_NAND;
	}

	reboot_mode = check_reboot_mode();
   if (reboot_mode == RECOVERY_MODE){
     printf("reboot_mode = Recovery\n");
	 boot_into_recovery = 1;
	 strcpy(boot_mode_log, "RECOVERY_MODE");
   }else if(reboot_mode == FASTBOOT_MODE){
     printf("reboot_mode = Fastboot\n");
	 strcpy(boot_mode_log, "FASTBOOT_MODE");
     goto fastboot;
   } else if(reboot_mode == FORCE_NORMAL_MODE) {
     printf("reboot_mode = Force_Normal\n");
     skip_loading_quickboot = 1;
	 strcpy(boot_mode_log, "FORCE_NORMAL_MODE");
   } else {
     printf("reboot_mode = (0x%08X)\n", reboot_mode);
	 strcpy(boot_mode_log, "QUICK_BOOT_MODE");
   }

#ifdef JTAG_BOOT
   memcpy(cmdline, 
         "console=ttyTCC0,115200n8 androidboot.console=ttyTCC0 androidboot.serialno=00001210262308580590 androidboot.wifimac= androidboot.btaddr=9C8E993DF697 androidboot.mode=emmc androidboot.memtype=2",
         strlen("console=ttyTCC0,115200n8 androidboot.console=ttyTCC0 androidboot.serialno=00001210262308580590 androidboot.wifimac= androidboot.btaddr=9C8E993DF697 androidboot.mode=emmc androidboot.memtype=2"));

   boot_linux((void *)0x80008000, (void *)TAGS_ADDR,
         (const char *)cmdline, 4010,
         (void *)0x81000000, 0xAC7BF);
#endif

#if defined(BOOTSD_INCLUDE)
   emmc_recovery_init();
#ifdef CONFIG_SNAPSHOT_BOOT
   if( boot_into_recovery == 0 && skip_loading_quickboot == 0 && force_disable_quickboot == 0 ) {
	   int qb_cnt;
	   int ret = -1;
	   for(qb_cnt = 0; (qb_cnt < 10) && (ret != 0); qb_cnt++) {
		   memset(qb_log, 0, sizeof(qb_log));
		   ret  = snapshot_boot_from_emmc();
		   dprintf(CRITICAL, "QB: Error to load QB Image. Try Again %d...\n", qb_cnt);
		   thread_sleep(100);
	   }
	   dprintf(CRITICAL, "QB: Error to load QB Image... Start Normal Boot.\n");
   }
#endif  /* #ifdef CONFIG_SNAPSHOT_BOOT */
   boot_linux_from_emmc();

#elif defined(TCC_NAND_USE)
#if defined(TNFTL_V8_INCLUDE)
   recovery_init_v8();
#ifdef CONFIG_SNAPSHOT_BOOT
   if( boot_into_recovery == 0 && skip_loading_quickboot == 0 && force_disable_quickboot == 0 )
      snapshot_boot_from_flash();
#endif  /* #ifdef CONFIG_SNAPSHOT_BOOT */
   boot_linux_from_flash_v8();
#else
#ifdef CONFIG_SNAPSHOT_BOOT
   if( boot_into_recovery == 0 && skip_loading_quickboot == 0 && force_disable_quickboot == 0 )
      snapshot_boot_from_flash();
#endif  /* #ifdef CONFIG_SNAPSHOT_BOOT */
#endif
#endif
   dprintf(CRITICAL, "ERROR: Could not do normal boot. Reverting "
         "to fastboot mode.\n");

fastboot:

#if defined(GPIO_BOOT_OK)
	//to Micom high -> low (chage usb mode(host->device)
	mdelay(100);
	gpio_set(GPIO_BOOT_OK, 0);
	LKPRINT("%s GPIO_BOOT_OK low for fastboot mode!\n", __func__);
#endif

#if !defined(DEFAULT_DISPLAY_OUTPUT_DUAL)
   if(!disp_init) {
      display_init();
   } else {
      fbcon_clear();
   }
#endif

   dprintf(INFO, "Diplay initialized\n");

#ifdef TCC_CM3_USE
	tcc_cm3_ctrl_ioctl(IOCTL_CM3_CTRL_OFF, 0);
#endif	


#if defined(TCC_USB_USE)
   udc_init(&fastboot_udc_device);

   fastboot_register("boot", cmd_boot);
   fastboot_register("erase:", cmd_erase);

#if defined(BOOTSD_INCLUDE)
	fastboot_register("flash:", cmd_flash_emmc);
	fastboot_register("erase:", cmd_erase_emmc);
#elif defined(TNFTL_V8_INCLUDE)
	fastboot_register("flash:", cmd_flash);
#endif

	fastboot_register("continue", cmd_continue);
	fastboot_register("reboot", cmd_reboot);
	fastboot_register("reboot-bootloader", cmd_reboot_bootloader);

#ifdef _EMMC_BOOT_TCC
	sprintf(board_name, "%s_emmc", BOARD);
	fastboot_publish("product", board_name);
#else
	fastboot_publish("product", BOARD);
#endif

	fastboot_publish("kernel", "lk");
	fastboot_publish("serialno", serialno);

#if defined(FASTBOOT_MAX_SIZE)
	fastboot_init(target_get_scratch_address(), FASTBOOT_MAX_SIZE);
#else
	fastboot_init(target_get_scratch_address(), 300 * 1024 * 1024);
#endif

	udc_start();
#endif
	target_battery_charging_enable(1, 0);
}

APP_START(aboot)
	.init = aboot_init,
	APP_END

