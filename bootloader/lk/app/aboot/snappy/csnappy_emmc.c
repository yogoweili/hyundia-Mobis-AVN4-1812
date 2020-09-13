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

#include <fwdn/Disk.h>
#include <sdmmc/sd_memory.h>
#include <sdmmc/emmc.h>
#include "sparse_format.h"


/*========================================================
  = definition
  ========================================================*/
#define ROUND_TO_PAGE(x,y) (((x) + (y)) & (~(y)))
#define BYTE_TO_SECOTR(x) (x)/512
#define SWSUSP_SIG               "S1SUSPEND"
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
	char reserved[PAGE_SIZE - 20 - BSIZE - sizeof(sector_t) - sizeof(int)];	
	sector_t image;		
	unsigned int flags;	/* Flags to pass to the "boot" kernel */	
	unsigned int reg[100];	
	//unsigned int ptable[7];	
	char	orig_sig[10];
	char	sig[10];		
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

static PPMU pPMU;
#define TCCWDT_RESET_TIME       4 // sec unit
#define TCCWDT_KICK_TIME_DIV    4
static int tccwdt_reset_time = TCCWDT_RESET_TIME;

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
#define SNAPSHOT_META_DATA_SIZE             (0x00040000 + SNAPSHOT_COMPRESS_DATA_SIZE)          // 256 kB
#define SNAPSHOT_INFO_SIZE                  (0x00040000 + SNAPSHOT_META_DATA_SIZE)              // 256 kB


static unsigned char *gpData;
static unsigned char *gpDataUnc;
static unsigned char *gpDataCmp;
static unsigned char *gpDataMeta;
static struct tcc_snapshot  *gpSnapshot;


/* We need to remember how much compressed data we need to read. */
#define SNAPPY_HEADER	sizeof(size_t)

/* Number of pages/bytes we'll compress at one time. */
#define SNAPPY_UNC_PAGES	32
#define SNAPPY_UNC_SIZE	(SNAPPY_UNC_PAGES * PAGE_SIZE)

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
            while(gpSwap_map_page->entries[gswap_map_page_offset] != gdata_read_offset_page_num) {
                gdata_read_offset += PAGE_SIZE;
                emmc_read_offset_page_num_inc();
                if (gdata_write_offset <= gdata_read_offset) {
                    get_next_data_from_emmc();
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

}


static volatile unsigned int cnt = 0;
#define nop_delay(x) for(cnt=0 ; cnt<x ; cnt++){ \
					__asm__ __volatile__ ("nop\n"); }


static int emmc_load_snapshot_image()
{
    int                     error;
    unsigned                index;
    unsigned                loop, max_copy_pages;
    static size_t           cmp_data_size;
    static size_t           unc_data_size;
    uint32_t                cmp_page_num;

    uint32_t                copy_page_num;

    unsigned char          *dst, *src;
    static uint32_t         meta_page_offset;

    time_t                  time_start, time_prev;
    time_t                  time_snappy, time_memcpy, time_flash;




    time_snappy = time_memcpy = time_flash = 0;

    time_start = current_time();

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

    while (max_copy_pages < gnr_meta_pages) {
    // get meta data
    get_next_swap_data(gpDataCmp, 1);

    // uncompress data
	unc_data_size = SNAPPY_UNC_SIZE;
    cmp_data_size = *(size_t *)gpDataCmp;
    cmp_page_num  = cmp_data_size / PAGE_SIZE;
    if (cmp_data_size & (PAGE_SIZE -1 ))
        cmp_page_num++;
    get_next_swap_data(gpDataCmp + PAGE_SIZE, (cmp_page_num - 1));

	error = csnappy_decompress(gpDataCmp + SNAPPY_HEADER, cmp_data_size,
 	                              (gpDataMeta + max_copy_pages * PAGE_SIZE), &unc_data_size);
	if (error < 0) {
		dprintf(CRITICAL, "Snappy decompression failed  [%d]\n", error);
        return error;
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
    	unc_data_size = SNAPPY_UNC_SIZE;
        cmp_data_size = *(size_t *)gpDataCmp;
        cmp_page_num  = (SNAPPY_HEADER + cmp_data_size + PAGE_SIZE - 1) / PAGE_SIZE;

        get_next_swap_data(gpDataCmp + PAGE_SIZE, (cmp_page_num - 1));

        time_prev = current_time();

    	error = csnappy_decompress(gpDataCmp + SNAPPY_HEADER, cmp_data_size,
    	                           gpDataUnc, &unc_data_size);
    	if (error < 0) {
    		dprintf(CRITICAL, "Snappy decompression failed  [%d]\n", error);
            return error;
    	}

        time_snappy += (current_time() - time_prev);
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
        if (unc_data_size != 128*1024) dprintf(CRITICAL, "index [%d] copy_page_num[%u] unc_data_size %lu \n", index, copy_page_num, unc_data_size);
        if ((index & 0x7ff) == 0)    dprintf(CRITICAL, "load %3d % \n", index * 100 / copy_page_num);
        
    }

    dprintf(CRITICAL, "index [%d] copy_page_num[%u][%uMbyte] unc_data_size %lu \n", index, copy_page_num,copy_page_num*4/1024, unc_data_size);
    dprintf(CRITICAL, "load_image_from_emmc.... meta[%6u] copy[%6u] index[%u]\n", gnr_meta_pages, gnr_copy_pages, index);
    dprintf(CRITICAL, "time(ms)... totoal[%8lu]  memcpy[%8lu] snappy[%8lu] \n", current_time() - time_start, time_memcpy, time_snappy);

    
    return 0;
}

static int snapshot_boot()
{
    int ret = 0;
    time_t start_time;

    dprintf(CRITICAL, "snapshot_boot...\n");

    start_time = current_time();

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
    ret = emmc_load_snapshot_image();

    dprintf(CRITICAL, "load_image_from_emmc done... [%lu]   0x%p, 0x%p, 0x%p\n", current_time() - start_time, gpData, gpDataUnc, gpDataCmp);

    dprintf(CRITICAL, "read data size from emmc [%6uMByte] \n", gdebug_emmc_read_size/1024/1024);		// for debug
    return ret;
}


static int emmc_check_sig(void)
{
	return !memcmp(SWSUSP_SIG, gpSwsusp_header->sig, 10);
}


extern void jump_to_kernel(unsigned int start, unsigned int arch);
static void emmc_snapshot_jump_to_resume(unsigned int *start)
{
   if (HW_REV <= 0x0624)
      tca_bus_poweron();

#if defined(PLATFORM_TCC893X)
   jump_to_kernel(buffers, 0x04);
#endif
}


/*==============================================================
  = Snapshot Boot
  =============================================================*/
int snapshot_boot_from_emmc()
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
        return -1;
    }

    gemmc_page_max = gemmc_read_addr_max * page_size / PAGE_SIZE - 1;

    gemmc_read_addr_min = gemmc_read_addr;
    gemmc_read_addr_max += gemmc_read_addr;

    /* read snapshot header */
    gpSwsusp_header  = (struct swsusp_header *)gpSnapshot->swsusp_header;
    if (emmc_read(gemmc_read_addr, PAGE_SIZE, gpSwsusp_header) != 0) {
        dprintf(INFO, "ERROR: snapshot header read fail...\n");
        return -1;
    }

    if (!emmc_check_sig()) {
        dprintf(CRITICAL, "ERROR: snapshot header sig compare fail. [snapshot]\n");
        return -1;
    } else {
		gemmc_read_addr += (gpSwsusp_header->image * PAGE_SIZE / page_size);

		/* get coprocessor data */
		memcpy(buffers, gpSwsusp_header->reg, sizeof(buffers));

		/* kernel page restore */
		if (snapshot_boot() == 0) {
			/* recover kernel context */
			emmc_snapshot_jump_to_resume(buffers);
		}
		else {
			dprintf(CRITICAL, "ERROR: snapshot boot fail.\n");
		}
	}

	return 0;
}

