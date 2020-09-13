/*
 * (C) Copyright 2013 Telechips
 * Telechips <linux@telechips.com>
 *
 * Configuation settings for the TCC893X EVM board.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CONFIG_H
#define __CONFIG_H

//#define DEBUG

/*
 * System recovery for usb update Configuration
 */
#define CONFIG_USB_UPDATE
#define UPDATE_FILE_NAME		"mobis.fai"		/* Just support the only "SD Data.fai" type file*/
/**
** @author sjpark@cleinsoft
** @date 2015/10/15
** f/w buffer address set
** 0x84000000 - conflict with fb buffer
***/
//#define UPDATE_BUFFER_ADDR			"84000000"
#define UPDATE_BUFFER_ADDR				"8DE00000"
#define UPDATE_BUFFER_SIZE				(100*1024*1024)	//100MB
#define UPDATE_BOOTLOADER_NAME          "lk.rom"
#define UPDATE_BOOTLOADER_DEFAULT_SIZE  (10*1024*1024)

/***********************************************************
 * Telechips Chip Specific Configurations
 ***********************************************************/
#define CONFIG_TELECHIPS		1	/* in a Telechips Core */
#define CONFIG_TCC				1	/* which is in a TCC Family */
#define CONFIG_TCC893X			1	/* which is in a TCC893X */
#define CONFIG_CHIP_TCC8930		1
#define CONFIG_TCC_BOARD_REV	0x1000 //HW_REV
#define CONFIG_ODT_TCC8931        1
#define CONFIG_ARCH_TCC893X
/*
 * Telechips DDR Configurations
 */
#define DRAM_DDR3
#define	CONFIG_DRAM_32BIT_USED
#define CONFIG_TCC_MEM_1024MB
#define CONFIG_DDR3_H5TQ4G83AFR_PBC

/*
 * Telechips Debug UART Configurations
 */
#define CONFIG_TCC_SERIAL
#define CONFIG_TCC_SERIAL_CLK	48000000
#define CONFIG_TCC_SERIAL0_PORT	16
#define CONFIG_DEBUG_UART		0
#define CONFIG_BAUDRATE			115200

/*
 * Telechips Drivers for U-Boot
 */
#define CONFIG_TCC_GPIO // GPIO Driver
//#define CONFIG_FWDN_V7
//#define TCC_USB_30_USE
//#define CONFIG_USB_DWC3
#define TCC_CHIP_REV			1
#define FWDN_V7					1

/* SD/MMC */
#define CONFIG_TCC_SDMMC
#define BOOTSD_INCLUDE
#define TODO_EMMC
#define _EMMC_BOOT

/***********************************************************
 * Physical Memory configuration for U-Boot
 ***********************************************************/
#define PHYS_DDR_BASE			0x80000000		// DDR Base Address
#define PHYS_DDR_SIZE			(512)			// 1024 MB

#define CONFIG_NR_DRAM_BANKS	1 // numbers of ATAG_MEM
#define FEATURE_SDMMC_MMC43_BOOT 1
/*
 * Memory Map for U-boot
 */
#define CONFIG_SYS_SDRAM_BASE		PHYS_DDR_BASE // DDR base
#define CONFIG_SYS_TEXT_BASE		0x80000000 //text base
#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_SDRAM_BASE + (PHYS_DDR_SIZE << 20)) // stack pointer
#define CONFIG_SYS_LOAD_ADDR		(CONFIG_SYS_SDRAM_BASE + 0x8000) // default img load address
#define CONFIG_STANDALONE_LOAD_ADDR	0x90000000 // Standalone program's base addr (ex. hello world)

#define BASE_ADDR					CONFIG_SYS_TEXT_BASE
#define SCRATCH_ADDR     			BASE_ADDR + 0x06000000
#define KERNEL_ADDR					BASE_ADDR + 0x00008000
#define MEMBASE						CONFIG_SYS_TEXT_BASE

/***********************************************************
 * Configurations for U-Boot
 ***********************************************************/
#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO

/*
 * Miscellaneous configurable options
 */
#define CONFIG_ZERO_BOOTDELAY_CHECK
#define CONFIG_BOOTDELAY			0
#define CONFIG_SYS_LONGHELP			/* undef to save memory */
#define CONFIG_SYS_HUSH_PARSER		/* use "hush" command parser	*/
#define CONFIG_SYS_PROMPT			"TCC # "
#define CONFIG_SYS_CBSIZE			256	/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE			384	/* Print Buffer Size */
#define CONFIG_SYS_MAXARGS			16	/* max number of command args */
#define CONFIG_SYS_BARGSIZE			CONFIG_SYS_CBSIZE /* Boot Argument Buffer Size */


#define CONFIG_SYS_L2CACHE_OFF
#define CONFIG_SYS_PL310
#define CONFIG_SYS_PL310_BASE		0x6C000000
#define CONFIG_SYS_CACHELINE_SIZE	32

#ifndef SUCCESS
#define	SUCCESS  0
#endif

#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR 	0x300 /* address 0x60000 */
#define CONFIG_SYS_U_BOOT_MAX_SIZE_SECTORS      	0x200 /* 256 KB */
#define CONFIG_SYS_MMC_SD_FAT_BOOT_PARTITION    	1

/*
 * FLASH and environment organization
 */
#define CONFIG_SYS_NO_FLASH		1

/*
 * U-Boot Environment configuration
 */
#define CONFIG_ENV_IS_NOWHERE

//#define CONFIG_SYS_MMC_ENV_DEV 		0
#define CONFIG_ENV_SIZE				0x100000	// 1MB
#define CONFIG_ENV_OFFSET			0x4000
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + (4 << 20)) /* Size of malloc() pool */
#define CONFIG_ENV_OVERWRITE	/* allow to overwrite serial and ethaddr */




/*-----------------------------------------------------------------------
 * USB configuration
 */
#define CONFIG_USB_XHCI
#define CONFIG_USB_XHCI_TCC
#define CONFIG_SYS_USB_XHCI_MAX_ROOT_PORTS	2

#define CONFIG_USB_STORAGE
#define CONFIG_DOS_PARTITION
#define CONFIG_FS_FAT

/*
 * Enable ATAGs for Linux Kernel 
 */

#define CONFIG_BOOTARGS "root=/dev/ram rw  iuser_debug=255, console=ttyTCC,115200n8"

#define CONFIG_PANEL_TAG
#define CONFIG_REVISION_TAG
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_CMDLINE_TAG

/*
 * U-Boot Default Boot Command after CONFIG_BOOTDELAY
 */
#define CONFIG_BOOTCOMMAND		"reset"


/***********************************************************
 * Command definition
 ***********************************************************/
#include <config_cmd_default.h>

#undef CONFIG_CMD_FLASH
#undef CONFIG_CMD_IMLS
#undef CONFIG_CMD_NAND
#undef CONFIG_CMD_NFS

//#define CONFIG_CMD_PING
//define CONFIG_CMD_MII

//#define CONFIG_CMD_CACHE
//#define CONFIG_CMD_ELF
#define CONFIG_CMD_USB
#define CONFIG_CMD_FAT


/***********************************************************
 * D-Audio definition
 ***********************************************************/
#define TCC893X_EMMC_BOOT
#define EMMC_SPEED_DDR
#define TARGET_BOARD_DAUDIO
#define DBG_FWUPDATE_DISPLAY

#define FB_BPP (32)
#define FB_BASE_ADDR "8D100000"
#define FB_BASE_ADDRH 0x8D100000

#define DISP_WIDTH  (800)
#define DISP_HEIGHT (480)
#define CCODE_RED   0x00FF0000
#define CCODE_GREEN 0x0000FF00
#define CCODE_BLUE  0x000000FF


#endif	/* __CONFIG_H */
