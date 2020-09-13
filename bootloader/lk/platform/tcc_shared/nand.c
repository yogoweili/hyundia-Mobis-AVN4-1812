/*
 * Copyright (c) 2010 Telechips, Inc.
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

#if defined(NAND_BOOT_INCLUDE)
#include <debug.h>
#include <partition.h>

/* Test Code */
#define NAND_BOOT_MAGIC "ANDROID!"
#define NAND_BOOT_MAGIC_SIZE 8
#define NAND_BOOT_NAME_SIZE 16
#define NAND_BOOT_ARGS_SIZE 512
#define NAND_BOOT_IMG_HEADER_ADDR 0xFF000

typedef struct nand_boot_img_hdr nand_boot_img_hdr;

struct nand_boot_img_hdr
{
    unsigned char magic[NAND_BOOT_MAGIC_SIZE];

    unsigned kernel_size;  /* size in bytes */
    unsigned kernel_addr;  /* physical load addr */

    unsigned ramdisk_size; /* size in bytes */
    unsigned ramdisk_addr; /* physical load addr */

    unsigned second_size;  /* size in bytes */
    unsigned second_addr;  /* physical load addr */

    unsigned tags_addr;    /* physical addr for kernel tags */
    unsigned page_size;    /* flash page size we assume */
    unsigned unused[2];    /* future expansion: should be 0 */

    unsigned char name[NAND_BOOT_NAME_SIZE]; /* asciiz product name */
    
    unsigned char cmdline[NAND_BOOT_ARGS_SIZE];

    unsigned id[8]; /* timestamp / checksum / sha1 / etc */
};
/* Test Code */

#define BLOCK_SIZE                0x200
#define TABLE_ENTRY_0             0x1BE
#define TABLE_ENTRY_1             0x1CE
#define TABLE_ENTRY_2             0x1DE
#define TABLE_ENTRY_3             0x1EE
#define TABLE_SIGNATURE           0x1FE
#define TABLE_ENTRY_SIZE          0x010

#define OFFSET_STATUS             0x00
#define OFFSET_TYPE               0x04
#define OFFSET_FIRST_SEC 	  0x08
#define OFFSET_SIZE 		  0x0c

#define MAX_PARTITIONS 64

#define NATIVE_LINUX_PARTITION 0x83
#define VFAT_PARTITION 0xb
#define WIN_EXTENDED_PARTITION 0xf
#define LINUX_EXTENDED_PARTITION 0x5
#define RAW_EMPTY_PARTITION 0x0

#define GET_LWORD_FROM_BYTE(x) ((unsigned)*(x) | ((unsigned)*(x+1) << 8) | ((unsigned)*(x+2) <<16) | ((unsigned)*(x+3) << 24))

struct mbr_entry
{
	unsigned dstatus;
	unsigned dtype;
	unsigned dfirstsec;
	unsigned dsize;
	unsigned char name[64];
};
struct mbr_entry mbr[MAX_PARTITIONS];
static char *ext4_partitions[] = {"system" , "userdata", "cache", "qb_data"};
static char *raw_partitions[] = {"boot","recovery", "kpanic" , "splash" , "misc", "tcc" , "snapshot"};

unsigned nand_partition_count = 0;
unsigned int ext4_count = 0;
unsigned int raw_count = 0;

static void mbr_set_partition_name(struct mbr_entry* mbr_ent , unsigned int type)
{
	switch(type) {
		case VFAT_PARTITION:
			memcpy(mbr_ent->name , "data" , 4);
			break;
		case NATIVE_LINUX_PARTITION:
			if(ext4_count == sizeof(ext4_partitions) / sizeof(char*))
				return;
			strcpy((char*)mbr_ent->name , (const char*) ext4_partitions[ext4_count]);
			ext4_count++;
			break;
		case RAW_EMPTY_PARTITION:
			if(raw_count == sizeof(raw_partitions)/sizeof(char*))
				return;
			strcpy((char*)mbr_ent->name, (const char*)raw_partitions[raw_count]);
			raw_count++;
			break;
			
		default : 
			break;
	}
}

static unsigned int flash_boot_read_MBR()
{
	unsigned char buffer[512];
	unsigned int dtype;
	unsigned int dfirstsec;
	unsigned int EBR_first_sec;
	unsigned int EBR_current_sec;
	int ret = 0;
	int idx, i;

	/* Read Master Boot Record from eMMC */
	ret = NAND_ReadSector(0 , 0 , 1, buffer);

	if(ret)
	{
		dprintf(INFO , "Master Boot Record Read Fail on NAND\n");
		return ret;
	}

	/* check to see if signature exists */
	if((buffer[TABLE_SIGNATURE] != 0x55) || (buffer[TABLE_SIGNATURE+1] !=0xAA))
	{
		dprintf(CRITICAL , "MBR Signature does not match \n");
		return ret;
	}

	/* Search 4 Primary Partitions */
	idx = TABLE_ENTRY_0;
	for(i=0; i<4; i++)
	{
		mbr[nand_partition_count].dstatus = buffer[idx+i*TABLE_ENTRY_SIZE + OFFSET_STATUS];
		mbr[nand_partition_count].dtype = buffer[idx+i*TABLE_ENTRY_SIZE + OFFSET_TYPE];
		mbr[nand_partition_count].dfirstsec = GET_LWORD_FROM_BYTE(&buffer[idx+i*TABLE_ENTRY_SIZE + OFFSET_FIRST_SEC]);
		mbr[nand_partition_count].dsize = GET_LWORD_FROM_BYTE(&buffer[idx+i*TABLE_ENTRY_SIZE + OFFSET_SIZE]);
		dtype = mbr[nand_partition_count].dtype;
		dfirstsec = mbr[nand_partition_count].dfirstsec;
		mbr_set_partition_name(&mbr[nand_partition_count], mbr[nand_partition_count].dtype); 
		nand_partition_count++;
		if (nand_partition_count == MAX_PARTITIONS)
			return ret;
	}

	EBR_first_sec = dfirstsec;
	EBR_current_sec = dfirstsec;

	ret = NAND_ReadSector(0, EBR_first_sec , 1, buffer); 

	if(ret) return ret;

	/* Search for Extended partion Extended Boot Record*/
	for (i = 0;; i++)
	{

		/* check to see if signature exists */
		if((buffer[TABLE_SIGNATURE] != 0x55) || (buffer[TABLE_SIGNATURE+1] !=0xAA))
		{
			dprintf(CRITICAL , "Extended Partition MBR Signature does not match \n");
			break;
		}

		mbr[nand_partition_count].dstatus =  buffer[TABLE_ENTRY_0 + OFFSET_STATUS];
		mbr[nand_partition_count].dtype   = buffer[TABLE_ENTRY_0 + OFFSET_TYPE];
		mbr[nand_partition_count].dfirstsec = GET_LWORD_FROM_BYTE(&buffer[TABLE_ENTRY_0 + OFFSET_FIRST_SEC])+EBR_current_sec;
		mbr[nand_partition_count].dsize = GET_LWORD_FROM_BYTE(&buffer[TABLE_ENTRY_0 + OFFSET_SIZE]);
		mbr_set_partition_name(&mbr[nand_partition_count], mbr[nand_partition_count].dtype); 
		nand_partition_count++;
		if (nand_partition_count == MAX_PARTITIONS)
			return ret;

		dfirstsec = GET_LWORD_FROM_BYTE(&buffer[TABLE_ENTRY_1 + OFFSET_FIRST_SEC]);
		if(dfirstsec == 0)
		{
			/* Getting to the end of the EBR tables */
			break;
		}
		/* More EBR to follow - read in the next EBR sector */
		ret = NAND_ReadSector(0, EBR_first_sec+dfirstsec , 1 , buffer); 

		if (ret)
		{
			return ret;
		}
		EBR_current_sec = EBR_first_sec + dfirstsec;
	}

	return ret;
}

static void printpartition()
{
	unsigned idx;

	for(idx = 0; idx < nand_partition_count; idx++)
	{
		if(mbr[idx].dtype == LINUX_EXTENDED_PARTITION)
			continue;

		dprintf(INFO, "[PARTITION : %10s] [START : %10d] [SIZE : %10d] [TYPE : %2x]\n" , mbr[idx].name, mbr[idx].dfirstsec, mbr[idx].dsize ,mbr[idx].dtype);
	}
}

void flash_boot_main()
{
	unsigned int nPartitionCnt =0;
	PARTITION PartitionArr[50];

	dprintf(INFO , "%s:init start from NAND\n" , __func__ );
	if(NAND_Ioctl(0, 0) != 0)
		dprintf(INFO , "%s:init failure!!!\n" , __func__ );

	memset(PartitionArr , 0 , sizeof(PartitionArr));
	nPartitionCnt = GetLocalPartition(0 , PartitionArr);

	flash_boot_read_MBR();
	printpartition();
}

unsigned long long flash_ptn_offset(unsigned char* name)
{
	unsigned n;

	for(n=0; n<nand_partition_count; n++){
		if(!strcmp((const char*)mbr[n].name, (const char*)name)){
			return (mbr[n].dfirstsec);
		}
	}

	return 0;
}
#ifdef CONFIG_SNAPSHOT_BOOT
unsigned long long flash_ptn_size(unsigned char* name)
{
	unsigned n;

	for(n=0; n<nand_partition_count; n++){
		if(!strcmp((const char*)mbr[n].name, (const char*)name)){
			return (mbr[n].dsize);
		}
	}

	return 0;
}
#endif
unsigned int flash_read_tnftl_v8(unsigned long long data_addr , unsigned data_len , void* in)
{
	int val = 0;
	unsigned int pageCount = 0;

	pageCount = (data_len + 511) / 512;

	if(pageCount)
		val = NAND_ReadSector(0, (unsigned long)data_addr, pageCount, in);

	return val;
}

unsigned int flash_write_tnftl_v8(char *part_name, unsigned long long data_addr , unsigned data_len , void* in)
{
	int val = 0;
	if( !strcmp( part_name, "bootloader"))
	{
		unsigned int romsize;
		unsigned int *data;

		data = in;				
		romsize = data[7];
		val = NAND_WriteFirmware(0, (unsigned char*)in, romsize);
	} 
	else
	{	
		unsigned int pageCount = 0;

		pageCount = (data_len + 511) / 512;

		if(pageCount)
			val = NAND_WriteSector(0, (unsigned long)data_addr, pageCount, in);	
	}
	
	return val;	
}

#define BYTE_TO_SECTOR(X)			((X + 511) >> 9)
#define ROUND_TO_PAGE(x,y) (((x) + (y)) & (~(y)))

unsigned int flash_nand_dump( unsigned char *buf)
{
	struct nand_boot_img_hdr *hdr = (void*) buf;
	struct nand_boot_img_hdr *uhdr;
	unsigned char *databuf = buf;
	unsigned n;
	unsigned long long ptn;
	unsigned offset = 0;
	unsigned int page_mask = 0;
	unsigned int flash_pagesize = 8192;

	flash_boot_read_MBR();
	printpartition();
		
	uhdr = (struct nand_boot_img_hdr *)NAND_BOOT_IMG_HEADER_ADDR;
	if(!memcmp(uhdr->magic , NAND_BOOT_MAGIC, NAND_BOOT_MAGIC_SIZE)){
		dprintf(INFO , "unified boot method!!\n");
		hdr = uhdr;
		return 0;
	}

	ptn = flash_ptn_offset("boot");

	if(ptn == 0){
		dprintf(CRITICAL , "ERROR : No BOOT Partition Found!!\n");
		return -1;
	}
	
	dprintf(CRITICAL , "[ Start Read Boot Image Read]\n");
	if(flash_read_tnftl_v8((ptn + BYTE_TO_SECTOR(offset)), flash_pagesize, (unsigned int*)buf))
	{
		dprintf(CRITICAL , "ERROR : Cannot read boto imgage header\n");
		return -1;
	}

	if(memcmp(hdr->magic, NAND_BOOT_MAGIC, NAND_BOOT_MAGIC_SIZE)){
		dprintf(CRITICAL ,"ERROR: Invalid boot image header\n");
		return -1;
	}

	if (hdr->page_size && (hdr->page_size != flash_pagesize)) {
		flash_pagesize = hdr->page_size;
		page_mask = flash_pagesize - 1;
	}

	offset += flash_pagesize;
	n = ROUND_TO_PAGE(hdr->kernel_size, page_mask);
	databuf += offset;

	dprintf(CRITICAL , "[ Start Read Kernel Image Read ]\n");
	
	if (flash_read_tnftl_v8(ptn + BYTE_TO_SECTOR(offset),n, (void *)databuf))
	{
		dprintf(CRITICAL, "ERROR: Cannot read kernel image\n");
                return -1;
	}
	databuf += n;
	n = ROUND_TO_PAGE(hdr->kernel_size, hdr->page_size-1); //base size is different from nand.		
	offset += n;
	n = ROUND_TO_PAGE(hdr->ramdisk_size, page_mask);

	dprintf(CRITICAL , "[ Start Read Ramdisk Image Read ]\n");
	if (flash_read_tnftl_v8(ptn + BYTE_TO_SECTOR(offset), n, (void *)databuf))
	{
		dprintf(CRITICAL, "ERROR: Cannot read ramdisk image\n");
                return -1;
	}
	offset += n;
	dprintf(CRITICAL, "[ End Read Boot Image ] [size:%d]\n", offset );
	return offset;
}

#endif

unsigned flash_page_size(void)
{
#if defined(_EMMC_BOOT_TCC) || defined(TNFTL_V8_INCLUDE)
	return 512;
#else
	return flash_pagesize;
#endif
}

