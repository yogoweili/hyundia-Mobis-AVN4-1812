/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 *
 * Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
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

#include <arch/tcc_used_mem.h>      // add by Telechips
#include <platform/reg_physical.h>
#include <platform/globals.h>

#include <i2c.h>
#include <dev/pmic.h>

#include <fwdn/Disk.h>
#include <sdmmc/sd_memory.h>
#include <sdmmc/emmc.h>

#include "swap_watchdog.h"
#include "../libsnapshot.h"
/*========================================================
  = definition
  ========================================================*/
#define ROUND_TO_PAGE(x,y) (((x) + (y)) & (~(y)))
#define BYTE_TO_SECOTR(x) (x)/512
#if defined(QUICKBOOT_SIG)
#define SWSUSP_SIG				QUICKBOOT_SIG
//#define FORCE_NOT_ALLOW_SNAPSHOT_BOOT
#else
#define SWSUSP_SIG               "S1SUSPEND"
#endif
#define PAGE_SIZE                4096
#define MAP_PAGE_ENTRIES	    (PAGE_SIZE / sizeof(sector_t) - 1)
#define addr(x) (0xF0000000+x)



/*========================================================
  = Variable 
  ========================================================*/
extern unsigned int page_size;
extern unsigned int page_mask;
extern unsigned int scratch_addr;

typedef uint64_t sector_t;
typedef uint64_t blkcnt_t;

struct swap_map_page {
     sector_t entries[MAP_PAGE_ENTRIES];
     sector_t next_swap;
 };

 struct swap_map_handle {
     struct swap_map_page *cur;
     sector_t cur_swap;
     sector_t first_sector;
     unsigned int k;
 };

typedef struct {
   unsigned long val;
} swp_entry_t;

#define BSIZE (sizeof(unsigned int) * 100)
#define TSIZE (sizeof(unsigned int) * 7)

struct swsusp_header {
	char reserved[PAGE_SIZE - sizeof(sector_t) - sizeof(int) - BSIZE - QB_SIG_SIZE*2];
	sector_t image;		
	unsigned int flags;	/* Flags to pass to the "boot" kernel */	
	unsigned int reg[100];	
	//unsigned int ptable[7];	
	char	orig_sig[QB_SIG_SIZE];
	char	sig[QB_SIG_SIZE];
}__attribute__((packed));


struct new_utsname {
   char sysname[65];
   char nodename[65];
   char release[65];
   char version[65];
   char machine[65];
   char domainname[65];
};

/* page backup entry */
typedef struct pbe {
   unsigned long address;      /* address of the copy */
   unsigned long orig_address; /* original address of page */
   swp_entry_t swap_address;
   swp_entry_t dummy;      /* we need scratch space at
                    * end of page (see link, diskpage)
                    */
} suspend_pagedir_t;

struct swsusp_info {
    struct new_utsname  uts;
    uint32_t            version_code;
    unsigned long       num_physpages;
    int                 cpus;
    unsigned long       image_pages;
    unsigned long       pages;
    unsigned long       size;   
} __attribute__((aligned(PAGE_SIZE)));

static struct swsusp_info	context;
static unsigned int         buffers[100];
static unsigned long long   gemmc_read_addr;
static unsigned long long   gemmc_read_addr_min;
static unsigned long long   gemmc_read_addr_max;
static unsigned long long   gemmc_page_max;

char 						swsusp_header_sig[10];
unsigned int				swsusp_header_addr = 0;

/*==========================================================================
  = function
  ===========================================================================*/

#if defined(PLATFORM_TCC88XX)
extern void tca_bus_poweron(void);
#endif
extern void tcc_mmu_switch(void);
extern void restore_coprocessor(void);

static struct swsusp_header    *gpSwsusp_header;
static struct swsusp_info      *gpSwsusp_info;
static struct swap_map_page    *gpSwap_map_page;
static uint32_t                *gpMeta_page_info;

static sector_t         gdata_read_offset_page_num;
static uint32_t         gdata_read_offset;
static uint32_t         gdata_write_offset;

static uint32_t         gswap_map_page_offset;
static unsigned int     gnr_meta_pages;
static unsigned int     gnr_copy_pages;

static unsigned int     gdebug_emmc_read_size;
static unsigned int     sizeQBImagePage;	// QuickBoot Image size ( Page unit )
static time_t			time_qb_load, time_start, time_total, time_chipboot = 600;
static time_t           time_prev, time_end;
static time_t           time_lz4, time_memcpy, time_flash;
time_t					time_logo = 0;

struct tcc_snapshot {
    unsigned char swsusp_header[2*PAGE_SIZE];
    unsigned char swsusp_info[2*PAGE_SIZE];
    unsigned char swap_map_page[2*PAGE_SIZE];

    unsigned int    buffers[100];
    unsigned int    ptable[4];
};

#define IO_OFFSET	0x00000000
#define io_p2v(pa)	((pa) - IO_OFFSET)
#define tcc_p2v(pa)         io_p2v(pa)

//=============================================================================
// memory map for snapshot image.
//=============================================================================
#define SNAPSHOT_FLASH_DATA_SIZE            0x00080000                                          // 512 kB(0x00080000), 256kB(0x00040000)
#define SNAPSHOT_UNCOMPRESS_DATA_SIZE       (0x00040000 + SNAPSHOT_FLASH_DATA_SIZE)             // 256 kB
#define SNAPSHOT_COMPRESS_DATA_SIZE         (0x00040000 + SNAPSHOT_UNCOMPRESS_DATA_SIZE)        // 256 kB
#define SNAPSHOT_META_DATA_SIZE             (0x00080000 + SNAPSHOT_COMPRESS_DATA_SIZE)          // 512 kB
#define SNAPSHOT_INFO_SIZE                  (0x00040000 + SNAPSHOT_META_DATA_SIZE)              // 256 kB


static unsigned char *gpData;
static unsigned char *gpDataUnc;
static unsigned char *gpDataCmp;
static unsigned char *gpDataMeta;
static struct tcc_snapshot  *gpSnapshot;

/*		To Show QuickBoot SIG in Kernel.		*/
extern char qb_sig[];
extern char qb_log[];
//static int qb_cmd_num = 0;
static void set_qb_cmdline(char *str_log)
{
	memset(qb_sig, 0, QB_SIG_SIZE+1);
	memcpy(qb_sig, gpSwsusp_header->sig, sizeof(SWSUSP_SIG));

	int qb_cmd_num = strlen(qb_log);
	memset(qb_log + qb_cmd_num, 0, 128 - qb_cmd_num);

	if ( 127 - qb_cmd_num < strlen(str_log)) {
		memcpy(qb_log +qb_cmd_num, str_log, 127 - qb_cmd_num);	
		qb_cmd_num = 127;
	} else {
		memcpy(qb_log +qb_cmd_num, str_log, strlen(str_log));	
		qb_cmd_num += strlen(str_log);
	}
	qb_log[qb_cmd_num] = '\0';
}
static void tail_qb_cmdline(void)
{
//	qb_log[120] = '\0';
//	strncat(qb_log, "(No_QB)", 7);
}
//
/* We need to remember how much compressed data we need to read. */
#define LZ4_HEADER	sizeof(uint32_t) * 2

/* Number of pages/bytes we'll compress at one time. */
#define LZ4_UNC_PAGES	32
#define LZ4_UNC_SIZE	(LZ4_UNC_PAGES * PAGE_SIZE)

static void emmc_read_offset_page_num_inc() 
{
    gdata_read_offset_page_num++;
    if (gdata_read_offset_page_num > gemmc_page_max) {
        gdata_read_offset_page_num = 1;
    }
}

static void get_next_data_from_emmc()
{
    uint32_t        index;
    uint32_t        data_offset;
    uint32_t        req_size;
    unsigned char  *dst, *src;

    dst = gpData;
    src = gpData + gdata_read_offset;
    req_size    = gdata_read_offset + SNAPSHOT_FLASH_DATA_SIZE - gdata_write_offset;
    data_offset = gdata_write_offset - gdata_read_offset;

    // copy remain data from tail to head.
    for(index = 0;index < data_offset/PAGE_SIZE;index++) {
        tcc_memcpy(dst, src);
        dst += PAGE_SIZE;
        src += PAGE_SIZE;
    }

    // check address ragne of snapshot partition.
    if (gemmc_read_addr >= gemmc_read_addr_max) {
        gemmc_read_addr = gemmc_read_addr_min + (PAGE_SIZE / page_size);
    }

    if (gemmc_read_addr + req_size/page_size >= gemmc_read_addr_max) {
        req_size = (gemmc_read_addr_max - gemmc_read_addr) * page_size;
    }

    // read next data from emmc.
    if (emmc_read(gemmc_read_addr, req_size, dst) != 0) {
        dprintf(CRITICAL, "Error: can not read data from emmc...   gemmc_read_addr:%llu,  req_size:%u\n", gemmc_read_addr, req_size);
		set_qb_cmdline("| ERR emmc_read |");  // cmdline
    }
    gdebug_emmc_read_size += req_size;		// for debug

    // update flash offset
    gemmc_read_addr += (req_size/page_size);

    gdata_read_offset = 0;
    gdata_write_offset = data_offset + req_size;
}

static void get_next_swap_page_info() 
{
    while(gpSwap_map_page->next_swap != gdata_read_offset_page_num) {
        gdata_read_offset += PAGE_SIZE;
        emmc_read_offset_page_num_inc();

        if (gdata_write_offset <= gdata_read_offset) {
            get_next_data_from_emmc();
        }
    }

    // find next pfn of swap map page info.
    tcc_memcpy(gpSnapshot->swap_map_page, gpData + gdata_read_offset);

    gdata_read_offset += PAGE_SIZE;
    emmc_read_offset_page_num_inc();
    gswap_map_page_offset = 0;

}

static void get_next_swap_data(unsigned char *out, unsigned req_page) 
{
    uint32_t index;
    unsigned char *dst, *src;

    if (req_page >= (gdata_write_offset - gdata_read_offset)/PAGE_SIZE) {
        get_next_data_from_emmc();
    }

    dst = out;
    src = gpData + gdata_read_offset;

    for(index = 0;index < req_page;index++) {
        // update swap map page
        if (gswap_map_page_offset >= MAP_PAGE_ENTRIES)
            get_next_swap_page_info();

        // copy data
        if(gpSwap_map_page->entries[gswap_map_page_offset] != gdata_read_offset_page_num) {
//			dprintf(CRITICAL, "QB: gpSwap_map_page->entries[gswap_map_page_offset] != gdata_read_offset_page_num\n");

			if( gpSwap_map_page->entries[gswap_map_page_offset] > (gdata_read_offset_page_num + (gdata_read_offset + SNAPSHOT_FLASH_DATA_SIZE - gdata_write_offset)/PAGE_SIZE)) {
				/*		Jump to Next Data Sector position if gpData doesn't have that data.  	*/
//				dprintf(CRITICAL, "QB: Jump to Next Data Sector Position to load data.\n");
				gdata_read_offset = 0;	// Clear gpData Buffer.
				gdata_write_offset = 0;	// Clear gpData Buffer.
				gdata_read_offset_page_num = gpSwap_map_page->entries[gswap_map_page_offset]; // storage offset ( PAGE ).
				gemmc_read_addr = gemmc_read_addr_min + BYTE_TO_SECTOR(gdata_read_offset_page_num * PAGE_SIZE);	// Storage read addr ( sector )
				get_next_data_from_emmc();	// Read Data from storage to gpData.

			} else if(gpSwap_map_page->entries[gswap_map_page_offset] < gdata_read_offset_page_num) {
				/*		Jump to Front of data sector (gdata_read_offset_page_num)		*/
//				dprintf(CRITICAL, "QB: Jump to front of Data Sector Position to load data.\n");
				gdata_read_offset = 0;	// Clear gpData Buffer.
				gdata_write_offset = 0;	// Clear gpData Buffer.
				gdata_read_offset_page_num = gpSwap_map_page->entries[gswap_map_page_offset]; // storage offset ( PAGE ).
				gemmc_read_addr = gemmc_read_addr_min + BYTE_TO_SECTOR(gdata_read_offset_page_num * PAGE_SIZE);	// Storage read addr ( sector )
				get_next_data_from_emmc();	// Read Data from storage to gpData.

			} else {
//				dprintf(CRITICAL, "QB: Seek Data to read by one page unit.\n");
				/*		Load Data from Storage by one page.		*/
	            while(gpSwap_map_page->entries[gswap_map_page_offset] != gdata_read_offset_page_num) {
    	            gdata_read_offset += PAGE_SIZE;
        	        emmc_read_offset_page_num_inc();
            	    if (gdata_write_offset <= gdata_read_offset) {
                	    get_next_data_from_emmc();
                	}
				}
            }
        }

        // copy data
        //tcc_memcpy(dst, gpData + gdata_read_offset, PAGE_SIZE);
        tcc_memcpy(dst, gpData + gdata_read_offset);

        dst += PAGE_SIZE;
        gdata_read_offset += PAGE_SIZE;
        gswap_map_page_offset++;
        emmc_read_offset_page_num_inc();
    }
	sizeQBImagePage += req_page;
}


static inline unsigned long checksum(unsigned long *addr, int len)
{
	unsigned long csum = 0;

	len /= sizeof(*addr);
	while (len-- > 0)
		csum ^= *addr++;
	csum = ((csum>>1) & 0x55555555)  ^  (csum & 0x55555555);

	return csum;
}

static int emmc_load_snapshot_image()
{
    int                     error;
    unsigned                index;
    unsigned                loop, max_copy_pages;
    static uint32_t         cmp_data_size;
    static uint32_t         unc_data_size;
    uint32_t                cmp_page_num;
    uint32_t                copy_page_num;
    static uint32_t			saved_checksum_data;
    static uint32_t			cal_checksum_data;

    unsigned char          *dst, *src;
    static uint32_t         meta_page_offset;

	watchdog_init();	// Initiatlize & Start WatchDog.


    time_lz4 = time_memcpy = time_flash = 0;

    // copy swap map page info.
    tcc_memcpy(gpSnapshot->swap_map_page, gpData);

    emmc_read_offset_page_num_inc();
    gdata_read_offset = PAGE_SIZE;

    // copy swsusp_info
    get_next_swap_data(gpSnapshot->swsusp_info, 1);

    gnr_meta_pages = gpSwsusp_info->pages - gpSwsusp_info->image_pages - 1;
    gnr_copy_pages = gpSwsusp_info->image_pages;
    copy_page_num = gnr_copy_pages;

	// load meta data 
    max_copy_pages = 0;

	/*		CRC Index Number 		*/
	static int CRC_cont = 0;

    while (max_copy_pages < gnr_meta_pages) {
        // get meta data
        get_next_swap_data(gpDataCmp, 1);

        // uncompress data
        cmp_data_size = *(uint32_t *)gpDataCmp;	// 4byte
        saved_checksum_data = *(uint32_t *)(gpDataCmp + 4) ; // 4byte

		cmp_page_num  = (LZ4_HEADER + cmp_data_size + PAGE_SIZE - 1) / PAGE_SIZE;
	
		get_next_swap_data(gpDataCmp + PAGE_SIZE, (cmp_page_num - 1));

		/*			01. Moved CRC Check Point.	( For MetaData )		*/
#ifdef CONFIG_SNAPSHOT_CHECKSUM
 //		dprintf(CRITICAL, " 01. Checksum : cmp_data_size : %d\n", cmp_data_size);
    	cal_checksum_data = checksum(gpDataCmp + LZ4_HEADER, cmp_data_size );

		/*			Show CRC DATA		*/
//		dprintf(CRITICAL, " 01. QuickBoot CRC : [%4d]\t\tSwap : [%x]\t\tLoad : [%x]\n", CRC_cont++, saved_checksum_data, cal_checksum_data);

       if( cal_checksum_data != saved_checksum_data ) {
        	dprintf(CRITICAL, " 01. Lz4 decompression crc check failed [%x] [%x]\n", cal_checksum_data, saved_checksum_data);
        	hexdump((gpDataMeta + max_copy_pages * PAGE_SIZE), 512 );
			set_qb_cmdline("01.CHECKSUM failed.");   // cmdline
        	return -1;
        }
#endif

	   unc_data_size = LZ4_uncompress_unknownOutputSize(gpDataCmp + LZ4_HEADER,
			   (gpDataMeta + max_copy_pages * PAGE_SIZE),
			   		cmp_data_size, LZ4_UNC_SIZE);
	   // gpDataCmp + LZ4_HEADER = Compressed Image Loading addrdss.
	   // gpDataMeta + max_copy_pages * PAGE_SIZE = UnCompressed Image saving address.
	   // cmp_data_size = Compressed Data Size
	   // LZ4_UNC_SIZE = UnCompressed Data Size
	   

    	if (unc_data_size <= 0 || unc_data_size > LZ4_UNC_SIZE) {
    		dprintf(CRITICAL, " 01. Lz4 decompression failed  [%d] [0x%08x]  cmp[0x%08x]\n", unc_data_size, unc_data_size, cmp_data_size);
			set_qb_cmdline("01.Lz4 decomp failed.");   // cmdline
            return (-1);
    	}

		// copy swap page data.
        meta_page_offset = 0;
        max_copy_pages += (unc_data_size / PAGE_SIZE);
    }

    copy_page_num = copy_page_num - (max_copy_pages - gnr_meta_pages);
    
    for (index = gnr_meta_pages;index < max_copy_pages;index++) {
        tcc_memcpy(gpMeta_page_info[meta_page_offset++] * PAGE_SIZE, gpDataMeta + index * PAGE_SIZE);
    }


// Load snapshot image to memory
    dprintf(CRITICAL, "load_image_from_flash.... meta[%6u] copy[%6u]\n", gnr_meta_pages, gnr_copy_pages);
    for (index = 0;index < copy_page_num;) {
        // get meta data
        get_next_swap_data(gpDataCmp, 1);

        // uncompress data
    	unc_data_size = LZ4_UNC_SIZE;
        cmp_data_size = *(uint32_t *)gpDataCmp;
        saved_checksum_data = *(uint32_t *)(gpDataCmp + 4) ;


        cmp_page_num  = (LZ4_HEADER + cmp_data_size + PAGE_SIZE - 1) / PAGE_SIZE;
     
		get_next_swap_data(gpDataCmp + PAGE_SIZE, (cmp_page_num - 1));

        time_prev = current_time();

		/*			02. Moved CRC Check Point. ( For QB Data )			*/
#ifdef CONFIG_SNAPSHOT_CHECKSUM
//		dprintf(CRITICAL, " 02. Checksum : cmp_data_size : %d\n", cmp_data_size);
        cal_checksum_data = checksum(gpDataCmp + LZ4_HEADER, cmp_data_size );

		/*			Show CRC DATA		*/
//		dprintf(CRITICAL, " 02. QuickBoot CRC : [%4d]\t\tSwap : [%x]\t\tLoad : [%x]\n", CRC_cont++, saved_checksum_data, cal_checksum_data);

       if( cal_checksum_data != saved_checksum_data ) {
        	dprintf(CRITICAL, " 02. Lz4 decompression crc check failed [%x] [%x]\n", cal_checksum_data, saved_checksum_data);
        	hexdump((gpDataMeta + max_copy_pages * PAGE_SIZE), 512 );
			set_qb_cmdline("02.CHECKSUM failed.");   // cmdline
        	return -1;
        }
#endif

    	unc_data_size = LZ4_uncompress_unknownOutputSize(gpDataCmp + LZ4_HEADER, gpDataUnc, cmp_data_size, LZ4_UNC_SIZE);
	    if (unc_data_size <= 0 || unc_data_size > LZ4_UNC_SIZE) {
    		dprintf(CRITICAL, " 02. Lz4 decompression failed  [%d]\n", unc_data_size);
			set_qb_cmdline("02.Lz4 decomp failed.");   // cmdline
            return (-1);
    	}

        time_lz4 += (current_time() - time_prev);
        time_prev = current_time();

        max_copy_pages = unc_data_size / PAGE_SIZE;

        src = gpDataUnc;
        for (loop = 0;loop < max_copy_pages;loop++) {
            dst = gpMeta_page_info[meta_page_offset++] * PAGE_SIZE;
            //memcpy(dst, src, PAGE_SIZE);
            tcc_memcpy(dst, src);
            src += PAGE_SIZE;
        }

        time_memcpy += (current_time() - time_prev);

        index += max_copy_pages;
        if (unc_data_size != 128*1024) dprintf(CRITICAL, "index [%d] copy_page_num[%u][%uMByte] unc_data_size %lu \n", index, copy_page_num, copy_page_num*4/1024, unc_data_size);
        if ((index & 0xfff) == 0)
			dprintf(CRITICAL, "load %3d % \n", index * 100 / copy_page_num);
		watchdog_clear();   // Clear WatchDog.
    
    }

    time_end = current_time();
    time_flash = time_end - time_start - time_memcpy - time_lz4;

    dprintf(CRITICAL, "index [%d] copy_page_num[%u][%uMByte] unc_data_size %lu \n", index, copy_page_num, copy_page_num*4/1024, unc_data_size);
    dprintf(CRITICAL, "load_image_from_emmc.... meta[%6u] copy[%6u] index[%u]\n", gnr_meta_pages, gnr_copy_pages, index);
	time_qb_load = time_memcpy + time_lz4 + time_flash;

    return 0;
}

static int snapshot_boot()
{
    int ret = 0;

    dprintf(CRITICAL, "snapshot_boot...\n");

    time_start = current_time();
	dprintf(CRITICAL, "\x1b[1;31mLK_init[%4lums] Display_Logo[%lums]\x1b[0m\n", time_start, time_logo);

// Initialize variable
    // flash read buffer
    gdata_read_offset  = 0;
    gdata_write_offset = 0;
    gdata_read_offset_page_num = gpSwsusp_header->image;

    // swap map page
    gswap_map_page_offset = 0;

    // swsusp info
    gnr_copy_pages = 0;
    gnr_meta_pages = 0;

    gpSwsusp_info    = (struct swsusp_info   *)gpSnapshot->swsusp_info;
    gpSwap_map_page  = (struct swap_map_page *)gpSnapshot->swap_map_page;
    gpMeta_page_info = gpDataMeta;

    // read data from flash
    get_next_data_from_emmc();


    // Load Image
	sizeQBImagePage = 1;	// QuickBoot Image Size ( Page unit ). HEADER = 1 page.
    ret = emmc_load_snapshot_image();
	time_total = current_time() - time_start;

	if (ret == 0) {
		dprintf(CRITICAL, "\x1b[1;31mQB Image load[%4lums] = load QB Image info[%4lums] + memcpy[%4lums] + lz4[%4lums] + load_storage[%4lums][%uMBps] \x1b[0m\n", time_total, time_total - time_qb_load, time_memcpy, time_lz4, time_flash, gdebug_emmc_read_size/1024/time_flash);
		dprintf(CRITICAL, "QuickBoot Image Size[%dMB]\n", (sizeQBImagePage*4)/1024);
    	dprintf(CRITICAL, "read data size from emmc [%6uMByte][%uMBps]\n", gdebug_emmc_read_size/1024/1024, gdebug_emmc_read_size/1024/time_flash);		// for debug
	} else {
		watchdog_disable();	// Disable WatchDog
	}

#if defined(CONFIG_QB_STEP_LOG)
	dprintf(CRITICAL, "\x1b[1;33m QB: Complite to load QuickBoot Image!!! \x1b[0m \n");
#endif

    return ret;
}


static int emmc_check_sig(void)
{
#if 0
	char tmp_sig[11];
	memset(tmp_sig, 0, 11);
	memcpy(tmp_sig, gpSwsusp_header->sig, 10);
	//dprintf(CRITICAL, "PM: SIG: [%s], snapshot header sig [%s]\n", SWSUSP_SIG, tmp_sig);

	int ret = 0;
	ret = !memcmp(SWSUSP_SIG, gpSwsusp_header->sig, 10);
	if (ret == 0) {
		//dprintf(CRITICAL, "ERROR: snapshot header sig compare fail.\n");
	}

	return ret;
#else
	dprintf(CRITICAL, "QB: QB SIG[%s] QB SIG SIZE[%d]\n",gpSwsusp_header->sig, sizeof(SWSUSP_SIG));
	return !memcmp(SWSUSP_SIG, gpSwsusp_header->sig, sizeof(SWSUSP_SIG));
#endif
}

#include <platform/iomap.h>
#include <reg.h>
#define PMU_USSTATUS        (TCC_PMU_BASE + 0x18)
extern void jump_to_kernel(unsigned int start, unsigned int arch);

static void emmc_snapshot_jump_to_resume(unsigned int *start)
{
   if (HW_REV <= 0x0624)
      tca_bus_poweron();
	  
   watchdog_clear();	// Clear WatchDog.

#if !defined(PLATFORM_TCC88XX)
   enter_critical_section();
   platform_uninit_timer();
   arch_disable_cache(UCACHE);
   arch_disable_mmu();
#endif

   time_t lk_total = time_start + time_total + time_chipboot;
   dprintf(CRITICAL, "\x1b[1;31mLK Boot Time : LK_Total[%4lums] = LK_init[%4lums] + QB Image load [%4lums] + chip boot [600ms] \x1b[0m\n", lk_total, time_start, time_total);

   /*		Send LK Total Time to Kernel	*/
   unsigned int usts;
   memset(usts, 0, 8);
   usts = (unsigned int)lk_total;
   writel(usts, PMU_USSTATUS);

   watchdog_clear();	// Clear WatchDog.

#if defined(PLATFORM_TCC893X)
   jump_to_kernel(buffers, 0x04);
#endif
}

#if !defined(CONFIG_QB_LOGO)
/*==============================================================
  = Snapshot Boot
  =============================================================*/
int snapshot_boot_from_emmc()
{
	set_qb_cmdline("] [QBLoad] ");	// cmdline

    gdebug_emmc_read_size = 0;		// for debug

    gpData     = (unsigned char *)(scratch_addr);
    gpDataUnc  = (unsigned char *)(scratch_addr + SNAPSHOT_FLASH_DATA_SIZE);
    gpDataCmp  = (unsigned char *)(scratch_addr + SNAPSHOT_UNCOMPRESS_DATA_SIZE);
    gpDataMeta = (unsigned char *)(scratch_addr + SNAPSHOT_COMPRESS_DATA_SIZE);
    gpSnapshot = (struct tcc_snapshot *)(scratch_addr + SNAPSHOT_META_DATA_SIZE);

	gpSwsusp_header  = (struct swsusp_header *)gpSnapshot->swsusp_header;
	memset(gpSwsusp_header, 0, PAGE_SIZE);
	memcpy(gpSwsusp_header->sig, "NO_QBSIG", 8);

    gemmc_read_addr = emmc_ptn_offset("snapshot");
    gemmc_read_addr_max = emmc_ptn_size("snapshot");
    if (gemmc_read_addr == 0 || gemmc_read_addr_max == 0) {
        dprintf(CRITICAL, "ERROR: swap partition table not found\n");
		set_qb_cmdline("NO swap ptn table.");	// cmdline
        return -1;
    }

    gemmc_page_max = gemmc_read_addr_max * page_size / PAGE_SIZE - 1;

    gemmc_read_addr_min = gemmc_read_addr;
    gemmc_read_addr_max += gemmc_read_addr;

    /* read snapshot header */
//    gpSwsusp_header  = (struct swsusp_header *)gpSnapshot->swsusp_header;
    if (emmc_read(gemmc_read_addr, PAGE_SIZE, gpSwsusp_header) != 0) {
        dprintf(INFO, "ERROR: snapshot header read fail...\n");
		set_qb_cmdline("READ_QB_HEADER");	// cmdline
        return -1;
    }

    if (!emmc_check_sig()) {
        dprintf(CRITICAL, "ERROR: snapshot header sig compare fail. [snapshot]\n");
		set_qb_cmdline("INVALID QB_SIG");	// cmdline
        return -1;
    } else {
		gemmc_read_addr += (gpSwsusp_header->image * PAGE_SIZE / page_size);

		/* get coprocessor data */
		memcpy(buffers, gpSwsusp_header->reg, sizeof(buffers));

#if defined(CONFIG_QB_STEP_LOG)
		dprintf(CRITICAL, "\x1b[1;33m QB: Start Loading QuickBoot Image!!! \x1b[0m \n");
#endif

		/* kernel page restore */
		if (snapshot_boot() == 0) {
			/* recover kernel context */
			emmc_snapshot_jump_to_resume(buffers);
		}
		else {
			dprintf(CRITICAL, "ERROR: snapshot boot fail.\n");
		}
	}

	tail_qb_cmdline();	// cmdline

	return -1;
}




#else	// CONFIG_QB_LOGO

/*==============================================================
  = Snapshot Boot
  =============================================================*/
int snapshot_boot_from_emmc()
{
	set_qb_cmdline("] [QBLoad] ");	// cmdline

	int check_sig = 0;
	char print_sig[QB_SIG_SIZE+1];
	memset(swsusp_header_sig, 0, QB_SIG_SIZE);
	memset(print_sig, 0, QB_SIG_SIZE+1);

    gdebug_emmc_read_size = 0;		// for debug
#if 1
    gpData     = (unsigned char *)(scratch_addr);
    gpDataUnc  = (unsigned char *)(scratch_addr + SNAPSHOT_FLASH_DATA_SIZE);
    gpDataCmp  = (unsigned char *)(scratch_addr + SNAPSHOT_UNCOMPRESS_DATA_SIZE);
    gpDataMeta = (unsigned char *)(scratch_addr + SNAPSHOT_COMPRESS_DATA_SIZE);
    gpSnapshot = (struct tcc_snapshot *)(scratch_addr + SNAPSHOT_META_DATA_SIZE);

	gpSwsusp_header  = (struct swsusp_header *)gpSnapshot->swsusp_header;
	memset(gpSwsusp_header, 0, PAGE_SIZE);
	memcpy(gpSwsusp_header->sig, "NO_QBSIG", 8);

    gemmc_read_addr = emmc_ptn_offset("snapshot");
    gemmc_read_addr_max = emmc_ptn_size("snapshot");
    if (gemmc_read_addr == 0 || gemmc_read_addr_max == 0) {
        dprintf(CRITICAL, "ERROR: swap partition table not found\n");
		set_qb_cmdline("NO swap ptn table.");	// cmdline
        return -1;
    }

    gemmc_page_max = gemmc_read_addr_max * page_size / PAGE_SIZE - 1;

    gemmc_read_addr_min = gemmc_read_addr;
    gemmc_read_addr_max += gemmc_read_addr;

    /* read snapshot header */
//    gpSwsusp_header  = (struct swsusp_header *)gpSnapshot->swsusp_header;
    if (emmc_read(gemmc_read_addr, PAGE_SIZE, gpSwsusp_header) != 0) {
        dprintf(CRITICAL, "ERROR: snapshot header read fail...\n");
		set_qb_cmdline("READ_QB_HEADER");	// cmdline
        return -1;
    }
#endif

	check_sig = emmc_check_sig();
	memcpy(swsusp_header_sig, gpSwsusp_header->sig, QB_SIG_SIZE);
	memcpy(print_sig, swsusp_header_sig, QB_SIG_SIZE);
	dprintf(INFO, "SIG: [%s], snapshot header sig [%s]\n",
			SWSUSP_SIG, print_sig);

    if (!check_sig) {
        dprintf(CRITICAL, "ERROR: snapshot header sig compare fail. [snapshot]\n");
		set_qb_cmdline("INVALID QB_SIG");	// cmdline
        return -1;
    } else {
		gemmc_read_addr += (gpSwsusp_header->image * PAGE_SIZE / page_size);

		/* get coprocessor data */
		memcpy(buffers, gpSwsusp_header->reg, sizeof(buffers));

		swsusp_header_addr = gpSwsusp_header->reg[1];
		dprintf(INFO, "snapshot addr: 0x%x\n", swsusp_header_addr);

#if defined(CONFIG_QB_STEP_LOG)
		dprintf(CRITICAL, "\x1b[1;33m QB: Start Loading QuickBoot Image!!! \x1b[0m \n");
#endif

		/* kernel page restore */
		if (snapshot_boot() == 0) {
#if !defined(FORCE_NOT_ALLOW_SNAPSHOT_BOOT)
			/* recover kernel context */
			emmc_snapshot_jump_to_resume(buffers);
#endif
		}
		else {
			dprintf(CRITICAL, "ERROR: snapshot boot fail.\n");
		}
	}
	tail_qb_cmdline();	// cmdline

	return -1;
}

#if defined(TCC893X_EMMC_BOOT)
/*==============================================================
  = For QuickBoot Logo Selection
  =============================================================*/
int snapshot_boot_header()
{
    gdebug_emmc_read_size = 0;		// for debug

    gpData     = (unsigned char *)(scratch_addr);
    gpDataUnc  = (unsigned char *)(scratch_addr + SNAPSHOT_FLASH_DATA_SIZE);
    gpDataCmp  = (unsigned char *)(scratch_addr + SNAPSHOT_UNCOMPRESS_DATA_SIZE);
    gpDataMeta = (unsigned char *)(scratch_addr + SNAPSHOT_COMPRESS_DATA_SIZE);
    gpSnapshot = (struct tcc_snapshot *)(scratch_addr + SNAPSHOT_META_DATA_SIZE);

    gemmc_read_addr = emmc_ptn_offset("snapshot");
    gemmc_read_addr_max = emmc_ptn_size("snapshot");
    if (gemmc_read_addr == 0 || gemmc_read_addr_max == 0) {
        dprintf(CRITICAL, "ERROR: swap partition table not found\n");
		set_qb_cmdline("[LOGO]NO swap ptn table.");	// cmdline
        return -1;
    }

    gemmc_page_max = gemmc_read_addr_max * page_size / PAGE_SIZE - 1;

    gemmc_read_addr_min = gemmc_read_addr;
    gemmc_read_addr_max += gemmc_read_addr;

    /* read snapshot header */
    gpSwsusp_header  = (struct swsusp_header *)gpSnapshot->swsusp_header;
    if (emmc_read(gemmc_read_addr, PAGE_SIZE, gpSwsusp_header) != 0) {
        dprintf(INFO, "ERROR: snapshot header read fail...\n");
		set_qb_cmdline("[LOGO]READ_QB_HEADER ");	// cmdline
        return -1;
    }

    if (!emmc_check_sig()) {
//        dprintf(CRITICAL, "ERROR: snapshot header sig compare fail. [snapshot]\n");
		set_qb_cmdline("[LOGO]INVALID_QB_SIG ");	// cmdline
        return -1;
    } else {
//		gemmc_read_addr += (gpSwsusp_header->image * PAGE_SIZE / page_size);
	}
	set_qb_cmdline("[LOGO]VALID_QB_SIG ");	// cmdline

	return 0;
}
#endif
#endif	// CONFIG_QB_LOGO
