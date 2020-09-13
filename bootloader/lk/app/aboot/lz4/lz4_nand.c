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

#ifdef PLATFORM_TCC893X
#include <arch/tcc_used_mem_tcc893x.h>
#include <arch/tcc_used_mem.h>      // add by Telechips
#include <platform/tcc893x/reg_physical.h>
#include <platform/tcc893x/structures_smu_pmu.h>
#endif
#include <platform/globals.h>
#include <dev/pmic.h>

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
static sector_t 		gflash_read_offset_min;
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
static unsigned int     sizeQBImagePage;       // QuickBoot Image size ( Page unit )
static time_t			time_qb_load;

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
#define SNAPSHOT_META_DATA_SIZE             (0x00080000 + SNAPSHOT_COMPRESS_DATA_SIZE)          // 512 kB
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
#define LZ4_HEADER	sizeof(uint32_t) * 2

/* Number of pages/bytes we'll compress at one time. */
#define LZ4_UNC_PAGES	32
#define LZ4_UNC_SIZE	(LZ4_UNC_PAGES * PAGE_SIZE)

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
		//dprintf(CRITICAL, "QuickBoot - Return to the head of snapshot partition to read data\n");

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
#if defined(TNFTL_V8_INCLUDE)
			dprintf(CRITICAL, "QB: gpSwap_map_page->entries[gswap_map_page_offset] != gdata_read_offset_page_num\n");

			if( gpSwap_map_page->entries[gswap_map_page_offset] > (gdata_read_offset_page_num + (gdata_read_offset + SNAPSHOT_FLASH_DATA_SIZE - gdata_write_offset)/PAGE_SIZE)) {
				/*		Jump to Next Data Sector position if gpData doesn't have that data.  	*/
				dprintf(CRITICAL, "QB: Jump to Next Data Sector Position to load data.\n");
				gdata_read_offset = 0;	// Clear gpData Buffer.
				gdata_write_offset = 0;	// Clear gpData Buffer.
				gdata_read_offset_page_num = gpSwap_map_page->entries[gswap_map_page_offset]; // storage offset ( PAGE ).
				gflash_read_offset = gflash_read_offset_min + BYTE_TO_SECTOR(gdata_read_offset_page_num * PAGE_SIZE);	// Storage read addr ( sector )
				get_next_data_from_flash(gflash_read_offset_min);	// Read Data from storage to gpData.

			} else if(gpSwap_map_page->entries[gswap_map_page_offset] < gdata_read_offset_page_num) {
				/*		Jump to Front of data sector (gdata_read_offset_page_num)		*/
				dprintf(CRITICAL, "QB: Jump to front of Data Sector Position to load data.\n");
				gdata_read_offset = 0;	// Clear gpData Buffer.
				gdata_write_offset = 0;	// Clear gpData Buffer.
				gdata_read_offset_page_num = gpSwap_map_page->entries[gswap_map_page_offset]; // storage offset ( PAGE ).
				gflash_read_offset = gflash_read_offset_min + BYTE_TO_SECTOR(gdata_read_offset_page_num * PAGE_SIZE);	// Storage read addr ( sector )
				get_next_data_from_flash(gflash_read_offset_min);	// Read Data from storage to gpData.

			} else {
				dprintf(CRITICAL, "QB: Seek Data to read by one page unit.\n");
				/*		Load Data from Storage by one page.		*/
	            while(gpSwap_map_page->entries[gswap_map_page_offset] != gdata_read_offset_page_num) {
   	             	gdata_read_offset += PAGE_SIZE;
	   	            flash_read_offset_page_num_inc();
   		            if (gdata_write_offset <= gdata_read_offset) {
   		                get_next_data_from_flash(ptn);
        	        }
				}
            }
#else
           while(gpSwap_map_page->entries[gswap_map_page_offset] != gdata_read_offset_page_num) {
   	             	gdata_read_offset += PAGE_SIZE;
	   	            flash_read_offset_page_num_inc();
   		            if (gdata_write_offset <= gdata_read_offset) {
   		                get_next_data_from_flash(ptn);
        	        }
				}
#endif
        }

        //memcpy(dst, gpData + gdata_read_offset, PAGE_SIZE);
        tcc_memcpy(dst, gpData + gdata_read_offset);

        dst += PAGE_SIZE;
        gdata_read_offset += PAGE_SIZE;
        gswap_map_page_offset++;
        flash_read_offset_page_num_inc();
    }
	sizeQBImagePage += req_page;
}


static volatile unsigned int cnt = 0;
#define nop_delay(x) for(cnt=0 ; cnt<x ; cnt++){ \
					__asm__ __volatile__ ("nop\n"); }

static inline unsigned long checksum(unsigned long *addr, int len)
{
	unsigned long csum = 0;

	len /= sizeof(*addr);
	while (len-- > 0)
		csum ^= *addr++;
	csum = ((csum>>1) & 0x55555555)  ^  (csum & 0x55555555);

	return csum;
}

#if defined(TNFTL_V8_INCLUDE)
static int load_snapshot_image(unsigned long long ptn)
#else
static int load_snapshot_image(struct ptentry *ptn)
#endif
{
    int                     error;
    unsigned                index;
    unsigned                loop, max_copy_pages;
    static uint32_t         cmp_data_size;
    static uint32_t         unc_data_size;
    static uint32_t			saved_checksum_data;
    static uint32_t			cal_checksum_data;
    uint32_t                cmp_page_num;

    uint32_t                copy_page_num;

    unsigned char          *dst, *src;
    static uint32_t         meta_page_offset;

    time_t                  time_start, time_prev, time_end;
    time_t                  time_lz4, time_memcpy, time_flash;

#if defined(PLATFORM_TCC893X)
	pPMU = (volatile PPMU)(tcc_p2v(HwPMU_BASE));
	pPMU->PMU_WDTCTRL.bREG.EN = 0;
	pPMU->PMU_WDTCTRL.bREG.CNT = tccwdt_reset_time*200;	
	pPMU->PMU_WDTCTRL.bREG.EN = 1;
	
	dprintf(CRITICAL, "watchdog_time=%dsec, kick_time=%dsec\n", tccwdt_reset_time, tccwdt_reset_time/TCCWDT_KICK_TIME_DIV);		
#endif


    dprintf(CRITICAL, "load_snapshot_image...\n");

    time_lz4 = time_memcpy = time_flash = 0;
    time_start = current_time();

    // copy swsusp_info
    get_next_swap_data(ptn, gpSnapshot->swsusp_info, 1);

    gnr_meta_pages = gpSwsusp_info->pages - gpSwsusp_info->image_pages - 1;
    gnr_copy_pages = gpSwsusp_info->image_pages;
    copy_page_num = gnr_copy_pages;


    // copy meta data
    max_copy_pages = 0;

	/*		CRC Index Number 		*/
	static int CRC_cont = 0;

    while (max_copy_pages < gnr_meta_pages) {
        // get meta data
        get_next_swap_data(ptn, gpDataCmp, 1);
        
        // uncompress data
        unc_data_size = LZ4_UNC_SIZE;
        cmp_data_size = *(uint32_t *)gpDataCmp;
        saved_checksum_data = *(uint32_t *)(gpDataCmp + 4) ;

		cmp_page_num  = (LZ4_HEADER + cmp_data_size + PAGE_SIZE - 1) / PAGE_SIZE;

        get_next_swap_data(ptn, gpDataCmp + PAGE_SIZE, (cmp_page_num - 1));
        
		/*			01. Moved CRC Check Point.		*/
#ifdef CONFIG_SNAPSHOT_CHECKSUM
        cal_checksum_data = checksum(gpDataCmp + LZ4_HEADER, cmp_data_size );

		/*			Show CRC DATA		*/
//		dprintf(CRITICAL, " 01. QuickBoot CRC : [%4d]\t\tSwap : [%x]\t\tLoad : [%x]\n", CRC_cont++, saved_checksum_data, cal_checksum_data);

       if( cal_checksum_data != saved_checksum_data ) {
        	dprintf(CRITICAL, " 01. Lz4 decompression crc check failed [%x] [%x]\n", cal_checksum_data, saved_checksum_data);
        	hexdump((gpDataMeta + max_copy_pages * PAGE_SIZE), 512 );
        	return -1;
        }
#endif

        unc_data_size = LZ4_uncompress_unknownOutputSize(gpDataCmp + LZ4_HEADER, (gpDataMeta + max_copy_pages * PAGE_SIZE), 
                                   cmp_data_size, LZ4_UNC_SIZE);
	   // gpDataCmp + LZ4_HEADER = Compressed Image Loading addrdss.
	   // gpDataMeta + max_copy_pages * PAGE_SIZE = UnCompressed Image saving address.
	   // cmp_data_size = Compressed Data Size
	   // LZ4_UNC_SIZE = UnCompressed Data Size
	
        if (unc_data_size <= 0 || unc_data_size > LZ4_UNC_SIZE) {
            dprintf(CRITICAL, " 01. Lz4 decompression failed  [%d]\n", unc_data_size);
            return -1;
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
    	unc_data_size = LZ4_UNC_SIZE;
        cmp_data_size = *(uint32_t *)gpDataCmp;
        saved_checksum_data = *(uint32_t *)(gpDataCmp + 4) ;
        cmp_page_num  = (LZ4_HEADER + cmp_data_size + PAGE_SIZE - 1) / PAGE_SIZE;

        get_next_swap_data(ptn, gpDataCmp + PAGE_SIZE, (cmp_page_num - 1));

        time_prev = current_time();

		/*			02. Moved CRC Check Point.		*/
#ifdef CONFIG_SNAPSHOT_CHECKSUM
        cal_checksum_data = checksum(gpDataCmp + LZ4_HEADER, cmp_data_size );

		/*			Show CRC DATA		*/
//		dprintf(CRITICAL, " 02. QuickBoot CRC : [%4d]\t\tSwap : [%x]\t\tLoad : [%x]\n", CRC_cont++, saved_checksum_data, cal_checksum_data);

		if( cal_checksum_data != saved_checksum_data ) {
			dprintf(CRITICAL, " 02. Lz4 decompression checksum failed [%x] [%x]\n", cal_checksum_data, saved_checksum_data);
			hexdump((gpDataMeta + max_copy_pages * PAGE_SIZE), 512 );
			return -1;
		}
#endif


    	unc_data_size = (uint32_t)LZ4_uncompress_unknownOutputSize(gpDataCmp + LZ4_HEADER, gpDataUnc, (int32_t)cmp_data_size, LZ4_UNC_SIZE);

    	if (unc_data_size < 0 || unc_data_size > LZ4_UNC_SIZE) {
    		dprintf(CRITICAL, " 02. Lz4 decompression failed  [%d]\n", unc_data_size);
            return -1;
    	}

        time_lz4 += (current_time() - time_prev);
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
#if  defined(PLATFORM_TCC893X)
           pPMU->PMU_WDTCTRL.bREG.CLR = 1; // Clear watchdog
#endif           
        }
    }

    time_end = current_time();
    time_flash = time_end - time_start - time_memcpy - time_lz4;

    dprintf(CRITICAL, "index [%d] copy_page_num[%u][%uMByte] unc_data_size %lu \n", index, copy_page_num, copy_page_num*4/1024, unc_data_size);
    dprintf(CRITICAL, "load_image_from_flash.... meta[%6u] copy[%6u] index[%u]\n", gnr_meta_pages, gnr_copy_pages, index);
	dprintf(CRITICAL, "\x1b[1;31mTime(ms) memcpy[%4lu] lz4[%4lu] load_storage[%4lu][%uMBps] QuickBoot Image[%dMB]\x1b[0m\n", time_memcpy, time_lz4, time_flash, gdebug_flash_read_size/1024/time_flash, (sizeQBImagePage*4)/1024);
	time_qb_load = time_memcpy + time_lz4 + time_flash;

#if  defined(PLATFORM_TCC893X)
           pPMU->PMU_WDTCTRL.bREG.CLR = 1; // Clear watchdog
#endif           


#if defined(PLATFORM_TCC893X)
    pPMU->PMU_WDTCTRL.bREG.EN = 0; // Disable watchdog
#endif           
    return 0;
}

#if defined(TNFTL_V8_INCLUDE)
static int snapshot_boot(unsigned long long ptn)
#else
static int snapshot_boot(struct ptentry *ptn)
#endif
{
    int ret = 0;
	time_t time_start;
    time_t time_total;
    uint32_t flash_read_offset_remain;
    sector_t start_pos = gpSwsusp_header->image;

    dprintf(CRITICAL, "snapshot_boot... image[%llu]\n", start_pos);

    time_start = current_time();

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
	sizeQBImagePage = 1;    // QuickBoot Image Size ( Page unit ). HEADER = 1 page.
    ret = load_snapshot_image(ptn);
	time_total = current_time() - time_start;

    if (ret < 0) {                          // Watchdog disable
#if defined(PLATFORM_TCC893X)
        pPMU->PMU_WDTCTRL.bREG.EN = 0;
        pPMU->PMU_WDTCTRL.bREG.CNT = tccwdt_reset_time*200;	
#endif
    }
    else 
		dprintf(CRITICAL, "\x1b[1;31mLK_init[%4lu] Total[%lums] etc[%lums] 0x%p, 0x%p, 0x%p\x1b[0m\n", time_start, time_total, time_total - time_qb_load, gpData, gpDataUnc, gpDataCmp);
    dprintf(CRITICAL, "read data size from flash: [%6uMByte] \n", gdebug_flash_read_size/1024/1024);
    return ret;
}


static int check_sig(void)
{
    //dprintf(CRITICAL, "QuickBoot:%s\n", gpSwsusp_header->sig);
	return !memcmp(SWSUSP_SIG, gpSwsusp_header->sig, 10);
}

static void snapshot_jump_to_resume(unsigned int *start)
{
   if (HW_REV <= 0x0624)
      tca_bus_poweron();

#if defined(PLATFORM_TCC893X)
	pPMU = (volatile PPMU)(tcc_p2v(HwPMU_BASE));

	/* remap to internal ROM */
	pPMU->PMU_CONFIG.bREG.REMAP = 0;
	
	pPMU->PMU_ISOL.nREG = 0x00000000;
	nop_delay(100000);
	pPMU->PWRUP_MBUS.bREG.DATA = 0;
	nop_delay(100000);
	pPMU->PWRUP_VBUS.bREG.DATA = 0;
	nop_delay(100000);
	pPMU->PWRUP_GBUS.bREG.DATA = 0;
	nop_delay(100000);
	pPMU->PWRUP_DBUS.bREG.DATA = 0;
	nop_delay(100000);
	pPMU->PWRUP_HSBUS.bREG.DATA = 0;
	nop_delay(100000);
	
	pPMU->PMU_SYSRST.bREG.VB = 1;
	pPMU->PMU_SYSRST.bREG.GB = 1;
	pPMU->PMU_SYSRST.bREG.DB = 1;
	pPMU->PMU_SYSRST.bREG.HSB = 1;
	nop_delay(100000);

	pPMU->PMU_WDTCTRL.bREG.EN = 0;
	pPMU->PMU_WDTCTRL.bREG.CNT = tccwdt_reset_time*200;	
	pPMU->PMU_WDTCTRL.bREG.EN = 1;
	
	dprintf(CRITICAL, "watchdog_time=%dsec, kick_time=%dsec\n", tccwdt_reset_time, tccwdt_reset_time/TCCWDT_KICK_TIME_DIV);		
#endif

#if !defined(PLATFORM_TCC88XX)
   enter_critical_section();
   platform_uninit_timer();
   arch_disable_cache(UCACHE);
   arch_disable_mmu();
#endif

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

	gflash_read_offset_min = ptn;

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

#if (defined(CONFIG_QB_LOGO) && !defined(TCC893X_EMMC_BOOT))
int snapshot_boot_header()
{
	return -1;
}
#endif
