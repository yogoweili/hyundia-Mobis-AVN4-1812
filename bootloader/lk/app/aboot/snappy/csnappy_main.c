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

#ifndef PLATFORM_TCC892X
#include <arch/tcc_used_mem_tcc8800.h>
#include <arch/tcc_used_mem.h>      // add by Telechips
#include <platform/structures.h>
#include <platform/physical.h>
#else
#include <arch/tcc_used_mem_tcc892x.h>
#include <arch/tcc_used_mem.h>      // add by Telechips
#include <platform/reg_physical.h>
#include <platform/structures_smu_pmu.h>
#endif
#include <platform/globals.h>

#if defined(TNFTL_V8_INCLUDE)
#define BYTE_TO_SECTOR(X)			((X + 511) >> 9)
#endif

#define SWSUSP_SIG               "S1SUSPEND"
#define PAGE_SIZE                4096
#define PAGE_OFFSET             (0xc0000000UL)
#define PHYS_OFFSET             (0x10000000UL)
#define MAP_PAGE_ENTRIES	    (PAGE_SIZE / sizeof(sector_t) - 1)
#define addr(x) (0xF0000000+x)

/*========================================================
  = Variable 
  ========================================================*/
extern unsigned int page_size;
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

static unsigned int buffers[100];


static PTIMER pTMR;
static PPMU pPMU;
#define TCCWDT_RESET_TIME       4 // sec unit
#define TCCWDT_KICK_TIME_DIV    4

static int tccwdt_reset_time = TCCWDT_RESET_TIME;
#define HwCLK_BASE                           (0xF0400000)

#define IO_OFFSET	0x00000000
#define io_p2v(pa)	((pa) - IO_OFFSET)
#define tcc_p2v(pa)         io_p2v(pa)

/*==========================================================================
  = function
  ===========================================================================*/
extern void tca_bus_poweron(void);
extern void jump_to_kernel(unsigned int start, unsigned int arch);

static struct swsusp_header    *gpSwsusp_header;
static struct swsusp_info      *gpSwsusp_info;
static struct swap_map_page    *gpSwap_map_page;
static uint32_t                *gpMeta_page_info;

static uint32_t         gflash_read_offset;
static uint32_t         gflash_read_offset_max;
static sector_t         gflash_read_page_max;

static sector_t         gdata_read_offset_page_num;
static uint32_t         gdata_read_offset;
static uint32_t         gdata_write_offset;

static uint32_t         gswap_map_page_offset;
static unsigned int     gnr_meta_pages;
static unsigned int     gnr_copy_pages;
static uint32_t         gdata_size;

static unsigned int     gdebug_flash_read_size;

struct tcc_snapshot {
    unsigned char swsusp_header[2*PAGE_SIZE];
    unsigned char swsusp_info[2*PAGE_SIZE];
    unsigned char swap_map_page[2*PAGE_SIZE];

    unsigned int    buffers[100];
    unsigned int    ptable[4];
};

//=============================================================================
// memory map for snapshot image.
//=============================================================================
#define SNAPSHOT_FLASH_DATA_SIZE            0x00080000                                          // 512 kB
#define SNAPSHOT_UNCOMPRESS_DATA_SIZE       (0x00040000 + SNAPSHOT_FLASH_DATA_SIZE)             // 256 kB
#define SNAPSHOT_COMPRESS_DATA_SIZE         (0x00040000 + SNAPSHOT_UNCOMPRESS_DATA_SIZE)        // 256 kB
#define SNAPSHOT_META_DATA_SIZE             (0x00040000 + SNAPSHOT_COMPRESS_DATA_SIZE)          // 256 kB
#define SNAPSHOT_INFO_SIZE                  (0x00040000 + SNAPSHOT_META_DATA_SIZE)              // 256 kB


static unsigned char *gpData;
static unsigned char *gpDataUnc;
static unsigned char *gpDataCmp;
static unsigned char *gpDataMeta;
static struct tcc_snapshot  *gpSnapshot;

extern void jump_to_sram(unsigned int);
extern int NAND_Ioctl(int function, void *param);
extern unsigned tcc_memcpy(void *dest, void *src);

/* We need to remember how much compressed data we need to read. */
#define SNAPPY_HEADER	sizeof(size_t)

/* Number of pages/bytes we'll compress at one time. */
#define SNAPPY_UNC_PAGES	32
#define SNAPPY_UNC_SIZE	(SNAPPY_UNC_PAGES * PAGE_SIZE)

static void flash_read_offset_page_num_inc() 
{
    gdata_read_offset_page_num++;
    if (gdata_read_offset_page_num > gflash_read_page_max) {
        dprintf(CRITICAL, "flash_read_offset_page_num_inc:  page_num[%llu] max[%llu]\n", gdata_read_offset_page_num, gflash_read_page_max);
        gdata_read_offset_page_num = 1;
    }
}


#if defined(TNFTL_V8_INCLUDE)
static void get_next_data_from_flash(unsigned long long ptn)
#else
static void get_next_data_from_flash(struct ptentry *ptn)
#endif
{
    uint32_t        index;
    uint32_t        data_offset;
    uint32_t        req_size;
    uint32_t        copy_size;
    unsigned char  *dst, *src;

    copy_size = 0;

    dst = gpData;
    src = gpData + gdata_read_offset;
    req_size    = (gdata_read_offset + SNAPSHOT_FLASH_DATA_SIZE - gdata_write_offset) / page_size;
    data_offset = gdata_write_offset - gdata_read_offset;

    // copy remain data from tail to head.
    for(index = 0;index < data_offset/PAGE_SIZE;index++) {
        tcc_memcpy(dst, src);
        dst += PAGE_SIZE;
        src += PAGE_SIZE;
    }

    // check address ragne of snapshot partition.
    if (gflash_read_offset >= gflash_read_offset_max) {
        dprintf(CRITICAL, "get_next_data_from_flash: \n");
        dprintf(CRITICAL, "    gflash_read_offset     [0x%08x]\n", gflash_read_offset);
        dprintf(CRITICAL, "    gflash_read_offset_max [0x%08x]\n", gflash_read_offset_max);

        /*  read first page of NAND */
        gflash_read_offset = 0;

        if (page_size > PAGE_SIZE) {
#if defined(TNFTL_V8_INCLUDE)
            flash_read_tnftl_v8(ptn + BYTE_TO_SECTOR(gflash_read_offset),  page_size, dst);
#else
            flash_read(ptn, gflash_read_offset, dst, page_size);
#endif
            gdebug_flash_read_size += page_size;	// for debug
            gflash_read_offset += page_size;

            src   = dst + PAGE_SIZE;
            index = page_size - PAGE_SIZE;
            copy_size = index;
            req_size -= 1;
            
            while(index) {
                tcc_memcpy(dst, src);
                dst   += PAGE_SIZE;
                src   += PAGE_SIZE;
                index -= PAGE_SIZE; 
            }
        }
        else {
            gflash_read_offset += PAGE_SIZE;
#if defined(TNFTL_V8_INCLUDE)
            flash_read_tnftl_v8(ptn + BYTE_TO_SECTOR(gflash_read_offset),  PAGE_SIZE, dst);
#else
            flash_read(ptn, gflash_read_offset, dst, PAGE_SIZE);
#endif
            gdebug_flash_read_size += PAGE_SIZE;	// for debug
            gflash_read_offset += PAGE_SIZE;

            dst      += PAGE_SIZE;
            req_size -= (PAGE_SIZE/page_size);
            copy_size = PAGE_SIZE;
        }
    }
    else if (gflash_read_offset + req_size * page_size >= gflash_read_offset_max) {
        dprintf(CRITICAL, "get_next_data_from_flash: \n");
        dprintf(CRITICAL, "    req_size               [0x%08x]\n", req_size);
        dprintf(CRITICAL, "    gflash_read_offset     [0x%08x]\n", gflash_read_offset);
        dprintf(CRITICAL, "    gflash_read_offset_max [0x%08x]\n", gflash_read_offset_max);
        req_size = (gflash_read_offset_max - gflash_read_offset) / page_size;

        dprintf(CRITICAL, "    new req_size           [0x%08x]\n", req_size);
    }

    // read next data from flash.
#if defined(TNFTL_V8_INCLUDE)
	if (flash_read_tnftl_v8(ptn + BYTE_TO_SECTOR(gflash_read_offset),  req_size * page_size, dst)) {
        dprintf(CRITICAL, "error .... Can not read data from flash...\n");
    }
#else
    if (flash_read(ptn, gflash_read_offset, dst, req_size * page_size) ) {
        dprintf(CRITICAL, "error .... Can not read data from flash...\n");
    }
#endif
    gdebug_flash_read_size += (req_size * page_size);	// for debug

    // update flash offset
    gflash_read_offset += (req_size*page_size);

    gdata_read_offset = 0;
    gdata_write_offset = data_offset + req_size * page_size + copy_size;
}

#if defined(TNFTL_V8_INCLUDE)
static void get_next_swap_page_info(unsigned long long ptn) 
#else
static void get_next_swap_page_info(struct ptentry *ptn) 
#endif
{
    while(gpSwap_map_page->next_swap != gdata_read_offset_page_num) {
        gdata_read_offset += PAGE_SIZE;
        flash_read_offset_page_num_inc();
        if (gdata_write_offset <= gdata_read_offset) {
            get_next_data_from_flash(ptn);
        }
    }

    // find next pfn of swap map page info.
    tcc_memcpy(gpSnapshot->swap_map_page, gpData + gdata_read_offset);

    gdata_read_offset += PAGE_SIZE;
    flash_read_offset_page_num_inc();
    gswap_map_page_offset = 0;

}

#if defined(TNFTL_V8_INCLUDE)
static void get_next_swap_data(unsigned long long ptn, unsigned char *out, unsigned req_page) 
#else
static void get_next_swap_data(struct ptentry *ptn, unsigned char *out, unsigned req_page) 
#endif
{
    uint32_t index;
    unsigned char *dst, *src;

    if (req_page >= (gdata_write_offset - gdata_read_offset)/PAGE_SIZE) {
        get_next_data_from_flash(ptn);
    }

    dst = out;
    src = gpData + gdata_read_offset;

    for(index = 0;index < req_page;index++) {
        // update swap map page
        if (gswap_map_page_offset >= MAP_PAGE_ENTRIES)
            get_next_swap_page_info(ptn);

        // copy data
        if(gpSwap_map_page->entries[gswap_map_page_offset] != gdata_read_offset_page_num) {

            while(gpSwap_map_page->entries[gswap_map_page_offset] != gdata_read_offset_page_num) {
                gdata_read_offset += PAGE_SIZE;
                flash_read_offset_page_num_inc();
                if (gdata_write_offset <= gdata_read_offset) {
                    get_next_data_from_flash(ptn);
                }
            }
        }

        //memcpy(dst, gpData + gdata_read_offset, PAGE_SIZE);
        tcc_memcpy(dst, gpData + gdata_read_offset);

        dst += PAGE_SIZE;
        gdata_read_offset += PAGE_SIZE;
        gswap_map_page_offset++;
        flash_read_offset_page_num_inc();
    }

}


static volatile unsigned int cnt = 0;
#define nop_delay(x) for(cnt=0 ; cnt<x ; cnt++){ \
					__asm__ __volatile__ ("nop\n"); }
#if defined(TNFTL_V8_INCLUDE)
static int load_snapshot_image(unsigned long long ptn)
#else
static int load_snapshot_image(struct ptentry *ptn)
#endif
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



    dprintf(CRITICAL, "load_snapshot_image...\n");

    time_snappy = time_memcpy = time_flash = 0;
    time_start = current_time();

    // copy swsusp_info
    get_next_swap_data(ptn, gpSnapshot->swsusp_info, 1);

    gnr_meta_pages = gpSwsusp_info->pages - gpSwsusp_info->image_pages - 1;
    gnr_copy_pages = gpSwsusp_info->image_pages;
    copy_page_num = gnr_copy_pages;


    // copy meta data
    max_copy_pages = 0;

    while (max_copy_pages < gnr_meta_pages) {
        // get meta data
        get_next_swap_data(ptn, gpDataCmp, 1);
        
        // uncompress data
        unc_data_size = SNAPPY_UNC_SIZE;
        cmp_data_size = *(size_t *)gpDataCmp;
        cmp_page_num  = (SNAPPY_HEADER + cmp_data_size + PAGE_SIZE - 1) / PAGE_SIZE;
        
        get_next_swap_data(ptn, gpDataCmp + PAGE_SIZE, (cmp_page_num - 1));
        
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
        get_next_swap_data(ptn, gpDataCmp, 1);

        // uncompress data
    	unc_data_size = SNAPPY_UNC_SIZE;
        cmp_data_size = *(size_t *)gpDataCmp;
        cmp_page_num  = (SNAPPY_HEADER + cmp_data_size + PAGE_SIZE - 1) / PAGE_SIZE;

        get_next_swap_data(ptn, gpDataCmp + PAGE_SIZE, (cmp_page_num - 1));

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
            tcc_memcpy(dst, src);
            src += PAGE_SIZE;
        }

        time_memcpy += (current_time() - time_prev);

        index += max_copy_pages;
        if (unc_data_size != 128*1024) dprintf(CRITICAL, "index [%d] copy_page_num[%u] unc_data_size %lu \n", index, copy_page_num, unc_data_size);
        if ((index & 0xFFF) == 0) {
           dprintf(CRITICAL, "load %3d % \n", index * 100 / copy_page_num);      
        }
    }

    dprintf(CRITICAL, "index [%d] copy_page_num[%u][%uMbyte] unc_data_size %lu \n", index, copy_page_num, copy_page_num*4/1024, unc_data_size);
    dprintf(CRITICAL, "load_image_from_flash.... meta[%6u] copy[%6u] index[%u]\n", gnr_meta_pages, gnr_copy_pages, index);
    dprintf(CRITICAL, "time(ms)... totoal[%8lu]  memcpy[%8lu] snappy[%8lu] \n", current_time() - time_start, time_memcpy, time_snappy);

    return 0;
}

#if defined(TNFTL_V8_INCLUDE)
static int snapshot_boot(unsigned long long ptn)
#else
static int snapshot_boot(struct ptentry *ptn)
#endif
{
    int ret = 0;
    time_t start_time;
    uint32_t flash_read_offset_remain;
    sector_t start_pos = gpSwsusp_header->image;

    dprintf(CRITICAL, "snapshot_boot... image[%llu]\n", start_pos);

    start_time = current_time();

    if (!start_pos)
        start_pos = 1;

// Initialize variable
    flash_read_offset_remain = (start_pos * PAGE_SIZE) % page_size;

    // flash read buffer
    gdata_read_offset  = 0;
    gdata_write_offset = 0;
    gflash_read_offset         = (start_pos * PAGE_SIZE) - flash_read_offset_remain;
    gdata_read_offset_page_num = start_pos;

    // swap map page
    gswap_map_page_offset = 0;

    // swsusp info
    gnr_copy_pages = 0;
    gnr_meta_pages = 0;

    gpSwsusp_info    = (struct swsusp_info   *)gpSnapshot->swsusp_info;
    gpSwap_map_page  = (struct swap_map_page *)gpSnapshot->swap_map_page;
    gpMeta_page_info = gpDataMeta;

    // read data from flash
    get_next_data_from_flash(ptn);

    // copy swap map page info.
    tcc_memcpy(gpSnapshot->swap_map_page, gpData + flash_read_offset_remain);

    gdata_read_offset = flash_read_offset_remain + PAGE_SIZE;
    flash_read_offset_page_num_inc();

    // Load Image
    ret = load_snapshot_image(ptn);
    if (ret < 0) {                          // Watchdog disable
    }
    else 
        dprintf(CRITICAL, "load_image_from_flash done... [%lu]   0x%p, 0x%p, 0x%p\n", current_time() - start_time, gpData, gpDataUnc, gpDataCmp);

    dprintf(CRITICAL, "read data size from flash: [%6uMByte] \n", gdebug_flash_read_size/1024/1024);
    return ret;
}


static int check_sig(void)
{
	return !memcmp(SWSUSP_SIG, gpSwsusp_header->sig, 10);
}

static void snapshot_jump_to_resume(unsigned int *start)
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
int snapshot_boot_from_flash()
{
   int ret;
#if defined(TNFTL_V8_INCLUDE)
    unsigned long long ptn;
#else
    struct ptentry *ptn;
    struct ptable *ptable;
#endif

	unsigned int read_page_size = page_size;

    gpData     = (unsigned char *)(scratch_addr);
    gpDataUnc  = (unsigned char *)(scratch_addr + SNAPSHOT_FLASH_DATA_SIZE);
    gpDataCmp  = (unsigned char *)(scratch_addr + SNAPSHOT_UNCOMPRESS_DATA_SIZE);
    gpDataMeta = (unsigned char *)(scratch_addr + SNAPSHOT_COMPRESS_DATA_SIZE);
    gpSnapshot = (struct tcc_snapshot *)(scratch_addr + SNAPSHOT_META_DATA_SIZE);

    gdebug_flash_read_size = 0;		// for debug

    dprintf(CRITICAL, "snapshot_boot_from_flash: read_page_size[%d]\n", read_page_size);

	if(read_page_size < PAGE_SIZE)
		read_page_size = PAGE_SIZE;

#if defined(TNFTL_V8_INCLUDE)
    ptn = flash_ptn_offset("snapshot");
	if (ptn == NULL) {
		dprintf(CRITICAL, "ERROR: Not find snapshot\n");
		return -1;
	}

    gflash_read_offset_max = flash_ptn_size("snapshot") * 512;
    gflash_read_page_max = gflash_read_offset_max / PAGE_SIZE - 1;

    dprintf(CRITICAL, "snapshot_boot_from_flash: gflash_read_offset_max[%u] gflash_read_page_max[%u]\n", gflash_read_offset_max, gflash_read_page_max);
#else
	ptable = flash_get_ptable();
	if (ptable == NULL) {
		dprintf(CRITICAL, "ERROR: Partition table not found\n");
		return -1;
	}

	ptn = ptable_find(ptable, "snapshot");
	if (ptn == NULL) {
		dprintf(CRITICAL, "ERROR: Not find snapshot\n");
		return -1;
	}

    gflash_read_offset_max = ptn->length * 1024 * 1024 * 2;
    gflash_read_page_max = gflash_read_offset_max / PAGE_SIZE - 1;
#endif

#if 0
    dprintf(CRITICAL, "ptn:\n");
    dprintf(CRITICAL, "    name  : %s \n", ptn->name);
    dprintf(CRITICAL, "    flags : %x \n", ptn->flags);
    dprintf(CRITICAL, "    start : %lu, %x \n", ptn->start, ptn->start);
    dprintf(CRITICAL, "    length: %llu \n", ptn->length);
    dprintf(CRITICAL, "    gflash_read_offset_max: 0x%08x \n", gflash_read_offset_max);
    dprintf(CRITICAL, "    gflash_read_page_max  : 0x%08x \n", gflash_read_page_max);
#endif

	/* read snapshot header */
    gpSwsusp_header  = (struct swsusp_header *)gpSnapshot->swsusp_header;

#if defined(TNFTL_V8_INCLUDE)
	if (flash_read_tnftl_v8(ptn + BYTE_TO_SECTOR(0), read_page_size, gpSwsusp_header)) {
		dprintf(INFO, "ERROR: snapshot header read fail.\n");
		return -1;
    }
#else
	if (flash_read(ptn, 0, gpSwsusp_header, read_page_size)) {
		dprintf(INFO, "ERROR: snapshot header read fail.\n");
		return -1;
    }
#endif

	if (!check_sig()) {
		dprintf(CRITICAL, "ERROR: snapshot header sig compare fail.\n");
    } else {
      /* get coprocessor data */
      memcpy(buffers, gpSwsusp_header->reg, sizeof(buffers));

		/* kernel page restore */
        if (snapshot_boot(ptn) == 0) {
    		/* recover kernel context */
    		snapshot_jump_to_resume(buffers);
        }
        else {
            dprintf(CRITICAL, "ERROR: snapshot boot fail.\n");
        }
	}
   return ret;
}

