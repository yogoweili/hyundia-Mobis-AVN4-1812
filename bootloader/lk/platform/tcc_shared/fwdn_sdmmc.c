/****************************************************************************
 *   FileName    : fwdn_SDMMC.c
 *   Description : SDMMC F/W downloader function
 ****************************************************************************
 *
 *   TCC Version 1.0
 *   Copyright (c) Telechips, Inc.
 *   ALL RIGHTS RESERVED
 *
 ****************************************************************************/
#if defined(_LINUX_) || defined(_WINCE_)
#include <platform/globals.h>
#include "IO_TCCXXX.h"
#include <string.h>
//#include "fwdn_protocol.h"
#include <fwdn/fwdn_drv_v7.h>
#include <fwdn/fwupgrade.h>
#endif

#include <fwdn/Disk.h>
#include <sdmmc/sd_memory.h>
#include <sdmmc/sd_update.h>

#include <debug.h>

#if defined(_LINUX_)
#define	LPRINTF	printf
#elif defined(_WINCE_)
#define	LPRINTF	EdbgOutputDebugString
#else
#define	LPRINTF
#endif

//#define SDRAM_TEST
extern unsigned long	InitRoutine_Start2;
extern unsigned long	InitRoutine_End2;
//=============================================================================
//*
//*
//*                     [ EXTERN VARIABLE & FUNCTIONS DEFINE ]
//*
//*
//=============================================================================
#ifdef BOOTSD_INCLUDE
extern const unsigned int		CRC32_TABLE[256];
extern unsigned int				g_uiFWDN_OverWriteSNFlag;
extern unsigned int				g_uiFWDN_WriteSNFlag;

extern unsigned int				FWDN_FNT_SetSN(unsigned char* ucTempData, unsigned int uiSNOffset);
extern void						FWDN_FNT_InsertSN(unsigned char * pSerialNumber);
extern void						FWDN_FNT_VerifySN(unsigned char* ucTempData, unsigned int uiSNOffset);
extern void						IO_DBG_SerialPrintf_(char *format, ...);
#endif

extern void *memcpy(void *, const void *, unsigned int);
extern void *memset(void *, int, unsigned int);
extern void *malloc(size_t size);


#define			CRC32_SIZE	4
#if defined(TSBM_ENABLE)
	#define			FWDN_WRITE_BUFFER_SIZE		(2*1024*1024)
#else
	#define			FWDN_WRITE_BUFFER_SIZE		(32*1024)
#endif
unsigned int	tempBuffer[FWDN_WRITE_BUFFER_SIZE];

#define FWDN_PROGRESS_RATE(A, B)  ((A *100) / B)

extern unsigned long  InitRoutine_Start;
extern unsigned long  InitRoutine_End;



static int fwdn_mmc_header_write(
		unsigned long base_addr,
		unsigned long config_size,
		unsigned long rom_size,
		unsigned int boot_partition,
		int fwdn_mode)
{
	return mmc_update_header( MEMBASE, config_size, rom_size, boot_partition, fwdn_mode );
}

static int fwdn_mmc_config_write( unsigned int rom_size,  // ks_8930
		unsigned int config_size, unsigned char *addr, int boot_partition)
{
	return mmc_update_config_code(rom_size, config_size, addr, boot_partition); //ks_8930
}

static int fwdn_mmc_rom_write (
		unsigned int rom_size ,
		unsigned int config_code_size ,
		unsigned char *buf ,
		/*unsigned int buf_size ,*/
		int boot_partition )
{
	/*FWDN_DRV_FirmwareWrite_Read(0, buf, rom_size ,FWDN_PROGRESS_RATE(0,rom_size));*/
	return mmc_update_rom_code( rom_size, config_code_size, buf, boot_partition );
}

int fwdn_mmc_update_bootloader(unsigned int rom_size, int boot_partition)
{
#ifdef BOOTSD_INCLUDE
	unsigned char *buf = (unsigned char*)tempBuffer;
#ifdef SDRAM_TEST
	unsigned int config_size = (unsigned int)&InitRoutine_End2 - (unsigned int)&InitRoutine_Start2;
#else
	unsigned int config_size = (unsigned int)&InitRoutine_End - (unsigned int)&InitRoutine_Start;
#endif
	/*BOOTSD_sHeader_Info_T secure_boot_head;*/

#if 0
	FWDN_DRV_FirmwareWrite_Read(0, buf, rom_size ,100); /*FWDN_PROGRESS_RATE(100));*/
#endif

	if( mmc_update_valid_test(rom_size) )
		return -1;

#if 0
	if( init_secure_head(&secure_boot_head, buf) )
		return -1;
#endif

	if( fwdn_mmc_header_write( MEMBASE, config_size, rom_size, boot_partition, 1 ) )
		return -1;

#ifdef SDRAM_TEST
	if( fwdn_mmc_config_write( rom_size, config_size, (unsigned char *)&InitRoutine_Start2, boot_partition ) )
#else
	if( fwdn_mmc_config_write( rom_size, config_size, (unsigned char *)&InitRoutine_Start, boot_partition ) )
#endif
		return -1;

#if 0
	printf("[ksjung_%s] rom_size : %d\n", __func__, rom_size);
	if( fwdn_mmc_rom_write( rom_size, config_size, buf,
				/*FWDN_WRITE_BUFFER_SIZE, */
				boot_partition ) )
		return -1;
#else

	printf("[FWDN\t] Start to write rom code to boot_partition no.%d\n", boot_partition);
	page_t p, q;
	unsigned int current_buf_index = 0;
	unsigned int rom_size_bak = rom_size;
	unsigned int full_size_of_buf = FWDN_WRITE_BUFFER_SIZE;
	unsigned int request_buf_size = FWDN_WRITE_BUFFER_SIZE;
	unsigned int partial_buf_size = 0;

	request_buf_size = full_size_of_buf >> 1;
	request_buf_size &= 0xfffffe00;

	memset(buf, 0x0, request_buf_size);

	p.start_page = BOOTSD_GetROMAddr(rom_size, config_size+CRC32_SIZE, boot_partition);
	p.buff = buf;
	p.boot_partition = boot_partition;

	while (rom_size)
	{
		partial_buf_size = (rom_size > request_buf_size) ? request_buf_size : rom_size;
		p.rw_size = BYTE_TO_SECTOR(partial_buf_size);
		FWDN_DRV_FirmwareWrite_Read(current_buf_index, p.buff, partial_buf_size,
				FWDN_PROGRESS_RATE(current_buf_index, rom_size_bak));
#if 0	
		printf("[%s] p.start_page : %d\n", __func__, p.start_page);
		printf("[%s] p.rw_size : %d\n", __func__, p.rw_size);
		printf("[%s] partial_buf_size : %d\n", __func__, partial_buf_size);
		printf("[%s] rom_size : %d\n", __func__, rom_size);
#endif
		if (DISK_Ioctl(DISK_DEVICE_TRIFLASH, DEV_BOOTCODE_WRITE_PAGE, (void*)&p) < 0)
		{
			printf("[%s] Fail to write rom code\n", __func__);
			return -1;
		}
	
		q.start_page = p.start_page;
		q.rw_size = p.rw_size;
		q.hidden_index = p.hidden_index;
		//q.buff = &buf[0] + request_buf_size;
		q.buff = p.buff + request_buf_size;
		q.boot_partition = p.boot_partition;
	
		if (DISK_Ioctl(DISK_DEVICE_TRIFLASH, DEV_BOOTCODE_READ_PAGE, (void*)&q) ||
			memcmp(p.buff, q.buff, (p.rw_size)<<9))
		{
			//printf("[%s] Fail to verify rom code\n", __func__);
			//return -1;
			printf("!");
			continue;
		}
		rom_size -= partial_buf_size;
		current_buf_index += request_buf_size;
		p.start_page += p.rw_size;
		printf(".");
	}
	printf("Done.\n");

#endif

#if 0
	if( fwdn_mmc_rom_write( rom_size, config_size, buf,
				/*FWDN_WRITE_BUFFER_SIZE, */
				boot_partition ) )
		return -1;

	mmc_update_rom_code( rom_size, config_code_size, buf, boot_partition );

	p->start_page = BOOTSD_GetROMAddr(rom_size, config_size+CRC32_SIZE, boot_partition);
	p->rw_size = BYTE_TO_SECTOR(rom_size);
	p->hidden_index = 0;
	p->buff = buf;
	p->boot_partition = boot_partition;

	if (DISK_Ioctl(DISK_DEVICE_TRIFLASH, DEV_BOOTCODE_WRITE_PAGE, (void*)p))
		return -1;

	buf = p->buff + ((p->rw_size) << 9);
	q->start_page = p->start_page;
	q->rw_size = p->rw_size;
	q->hidden_index = p->hidden_index;
	q->buff = p->buff;
	q->boot_partition = p->boot_partition;

	if (DISK_Ioctl(DISK_DEVICE_TRIFLASH, DEV_BOOTCODE_READ_PAGE, (void*)&q) ||
			memcmp(p->buff, q->buff, (p->rw_size)<<9))
		return -1;

#endif
#endif
	return 0;
}

#if defined(TSBM_ENABLE)
int fwdn_mmc_update_secure_bootloader(unsigned int rom_size, int boot_partition)
{
#ifdef BOOTSD_INCLUDE
	unsigned char *buf = (unsigned char*)tempBuffer;
	BOOTSD_sHeader_Info_T secure_h;

	FWDN_DRV_FirmwareWrite_Read(0, buf, rom_size ,100 ); /*FWDN_PROGRESS_RATE(100));*/

	if( mmc_update_valid_test(rom_size) )
		return -1;

	if( init_secure_head(&secure_h, buf) )
		return -1;

	if( fwdn_mmc_header_write(
				MEMBASE,
				secure_h.dram_init_size,
				secure_h.bootloader_size,
				boot_partition,
				1 ) )
		return -1;

	if( fwdn_mmc_config_write(
				/*rom_size, // ks_8930*/
				secure_h.bootloader_size,
				secure_h.dram_init_size,
				(unsigned char*)(buf+(unsigned int)secure_h.dram_init_start),
				boot_partition ) )
		return -1;

	if( fwdn_mmc_rom_write(
				secure_h.bootloader_size,
				secure_h.dram_init_size,
				buf + (unsigned int)secure_h.bootloader_start,
				/*FWDN_WRITE_BUFFER_SIZE, */
				boot_partition ) )
		return -1;
#endif
	return 0;
}
#endif

unsigned int FwdnReadBootSDFirmware(unsigned int master)
{
	return 0;
}

int fwdn_mmc_get_serial_num(void)
{
#ifdef BOOTSD_INCLUDE
	return mmc_get_serial_num(NULL, 0);
#endif
}

int FwdnSetBootSDSerial(unsigned char *ucData, unsigned int overwrite)
{
#ifdef BOOTSD_INCLUDE
	ioctl_diskinfo_t  disk_info;
	unsigned char   ucTempData[512];
	int       iRev;
	ioctl_diskrwpage_t  header;

	if (DISK_Ioctl(DISK_DEVICE_TRIFLASH, DEV_GET_DISKINFO, (void *)&disk_info) < 0)
	{
		return -1;
	}
	if (disk_info.sector_size != 512)
		return -1;

	memcpy( FWDN_DeviceInformation.DevSerialNumber, ucData , 32);
	g_uiFWDN_OverWriteSNFlag  =  overwrite;
	g_uiFWDN_WriteSNFlag    = 0;

	if ( overwrite == 0 )
	{
		/*----------------------------------------------------------------
		 *       G E T S E R I A L N U M B E R
		 *             ----------------------------------------------------------------*/
		header.start_page = BOOTSD_GetHeaderAddr( 0 );
		header.rw_size    = 1;
		header.buff     = ucTempData;
		iRev  = DISK_Ioctl(DISK_DEVICE_TRIFLASH, DEV_BOOTCODE_READ_PAGE, (void *)&header);
		if(iRev != SUCCESS)
			return (-1);

		iRev = FWDN_FNT_SetSN(ucTempData, 32);
	}
#endif
	return 0;
}



















#if 0

static int _loc_FWDN_Verify_Firmware(ioctl_diskrwpage_t *verify, unsigned long size)
{

	LPRINTF("[FWDN		] VERIFYING ................. \n");
	ioctl_diskrwpage_t veri_rwpage;
	unsigned char *pVerifyData = verify->buff + ((verify->rw_size) << 9);

	veri_rwpage.rw_size	= verify->rw_size;
	veri_rwpage.buff	= pVerifyData;
	veri_rwpage.start_page	= verify->start_page;

	if(DISK_Ioctl(DISK_DEVICE_TRIFLASH, DEV_BOOTCODE_READ_PAGE, (void*)&veri_rwpage))
		return ERR_FWUG_FAIL_CODEREADTFLASH;

	if(memcmp(verify->buff, pVerifyData, size) !=0 )
		return ERR_FWUG_FAIL + 100;

	return SUCCESS;

}

static int FwdnWriteCodeBootSD(unsigned int uiROMFileSize, unsigned int uiConfCodeSize, unsigned char* RomBuffer)
{
#ifdef BOOTSD_INCLUDE
	ioctl_diskrwpage_t	sBootCodeWrite;

#if defined(TSBM_ENABLE)
	sBootCodeWrite.start_page	=   BOOTSD_GetROMAddr(uiROMFileSize, uiConfCodeSize) ;
#else
	sBootCodeWrite.start_page	=   BOOTSD_GetROMAddr(uiROMFileSize, uiConfCodeSize + CRC32_SIZE) ;
#endif
	sBootCodeWrite.buff		= RomBuffer;
	LPRINTF("[FWDN		] Write Code Begins\n");

	sBootCodeWrite.rw_size 	= BYTE_TO_SECTOR(uiROMFileSize);

	if(DISK_Ioctl(DISK_DEVICE_TRIFLASH, DEV_BOOTCODE_WRITE_PAGE, (void *) &sBootCodeWrite) < 0)
		return ERR_FWUG_FAIL_CODEWRITETFLASH;

	if(_loc_FWDN_Verify_Firmware(&sBootCodeWrite, uiROMFileSize)){
		LPRINTF("[FWDN		] BOOTLOADER Verify Failed !!! Check([Func : %s] [Line : %d])\n" , __func__ , __LINE__);
		return ERR_FWUG_FAIL_VERIFY;
	}

	LPRINTF("[FWDN		] Write Code Completed\n");
#endif
	return SUCCESS;
}

static int FwdnWriteConfigBootSD(unsigned char* RomBuffer , unsigned int uiConfigStart, unsigned int uiConfigSize)
{
	unsigned int 		uiCRC;
	ioctl_diskrwpage_t	rwpage;
	unsigned char *pTempData = (unsigned char*)tempBuffer;
	memset(pTempData, 0x00, FWDN_WRITE_BUFFER_SIZE);

	//Copy from Config code to Buffer
	memcpy(pTempData, (RomBuffer+uiConfigStart) , uiConfigSize);
#if !defined(TSBM_ENABLE)
	uiCRC = FWUG_CalcCrc8(pTempData, uiConfigSize);
	word_of(pTempData + uiConfigSize) = uiCRC;
	uiConfigSize += CRC32_SIZE;
#endif

	rwpage.buff			= pTempData;
	rwpage.rw_size		= BYTE_TO_SECTOR(uiConfigSize);
	rwpage.start_page	= (unsigned long)BOOTSD_GetConfigCodeAddr(uiConfigSize);

	if(DISK_Ioctl(DISK_DEVICE_TRIFLASH, DEV_BOOTCODE_WRITE_PAGE, (void *) & rwpage) < 0)
		return ERR_FWUG_FAIL_HEADERWRITETFLASH;

	if(_loc_FWDN_Verify_Firmware(&rwpage, uiConfigSize)){
		LPRINTF("[FWDN		] DRAM Config Code Verify Failed !!! Check([Func : %s] [Line : %d])\n" , __func__ , __LINE__);
		return ERR_FWUG_FAIL_VERIFY;
	}

	return SUCCESS;
}

static int FwdnUpdateHeaderBootSD(unsigned long uiROMFileSize, unsigned long uiConfigSize)
{
#ifdef BOOTSD_INCLUDE
	ioctl_diskrwpage_t	rwpage;
	BOOTSD_Header_Info_T sHeaderInfo;
	unsigned char		*pTempData   = (unsigned char*)tempBuffer;

	memset(&sHeaderInfo, 0, sizeof(BOOTSD_Header_Info_T));

	sHeaderInfo.ulEntryPoint 		= BASE_ADDR;		
	sHeaderInfo.ulBaseAddr 			= BASE_ADDR;
#if defined(TSBM_ENABLE)
	sHeaderInfo.ulConfSize			= uiConfigSize;
#else
	sHeaderInfo.ulConfSize			= uiConfigSize + CRC32_SIZE;			//CRC plus			
#endif
	sHeaderInfo.ulROMFileSize		= uiROMFileSize;
	sHeaderInfo.ulBootConfig		= (BOOTCONFIG_NORMAL_SPEED | BOOTCONFIG_4BIT_BUS);

	{
		sHidden_Size_T stHidden;
		BOOTSD_Hidden_Info(&stHidden);
		sHeaderInfo.ulHiddenStartAddr = stHidden.ulHiddenStart[0] + stHidden.ulSectorSize[0];
		memcpy((void *)sHeaderInfo.ulHiddenSize, (const void*)stHidden.ulSectorSize, (BOOTSD_HIDDEN_MAX_COUNT << 2));
	}

	FWDN_FNT_InsertSN(sHeaderInfo.ucSerialNum);

	memcpy(sHeaderInfo.ucHeaderFlag, "HEAD", 4);

	sHeaderInfo.ulCRC				= FWUG_CalcCrc8((unsigned char*) &sHeaderInfo, 512-4);

	memset(pTempData, 0x00, FWDN_WRITE_BUFFER_SIZE);
	memcpy(pTempData, &sHeaderInfo, 512);

	rwpage.start_page	= BOOTSD_GetHeaderAddr();
	rwpage.buff			= pTempData;
	rwpage.rw_size		= 1;

	if(DISK_Ioctl(DISK_DEVICE_TRIFLASH, DEV_BOOTCODE_WRITE_PAGE, (void *) & rwpage))
		return ERR_FWUG_FAIL_HEADERWRITETFLASH;

	if(_loc_FWDN_Verify_Firmware(&rwpage, 512)){
		LPRINTF("[FWDN		] BOOTSD Header Verify Failed !!! Check([Func : %s] [Line : %d])\n" , __func__ , __LINE__);
		return ERR_FWUG_FAIL_VERIFY;
	}

	LPRINTF("[FWDN		] Write Header Completed!!\n");

#endif

	return SUCCESS;

}

extern unsigned long	InitRoutine_Start;
extern unsigned long	InitRoutine_End;
int FwdnWriteBootSDFirmware(unsigned char* pucRomBuffer , unsigned int uiROMFileSize)
{

#ifdef BOOTSD_INCLUDE
	int 			res = 1;
	unsigned int 		uiConfigSize;
	unsigned int 		uiConfigStart;

	//=====================================================================
	// Check                   ============================================
	//=====================================================================
	ioctl_diskinfo_t 		disk_info;

	if ( (res=DISK_Ioctl(DISK_DEVICE_TRIFLASH, DEV_GET_DISKINFO, (void *)&disk_info)) < 0)
		return res;

	if (disk_info.sector_size != 512 || !uiROMFileSize)
		return -1;
#if defined (FEATURE_SDMMC_MMC43_BOOT)
	LPRINTF("[FWDN		] Begin eMMC Bootloader Write To TRI-FLASH\n");
#else
	LPRINTF("[FWDN		] Begin SD Bootloader Write To TRI-FLASH\n");
#endif

#if defined(TSBM_ENABLE)
	LPRINTF("[FWDN		] TSBM\n");


	BOOTSD_sHeader_Info_T sboot_header;
	unsigned char pTempBuffer[512];

	memset(&sboot_header, 0x00, sizeof(BOOTSD_sHeader_Info_T));
	memcpy((unsigned char*)&sboot_header, pucRomBuffer, sizeof(BOOTSD_sHeader_Info_T));

	if(memcmp(sboot_header.magic ,SBOOT_MAGIC , SBOOT_MAGIC_SIZE)){
		LPRINTF("[FWDN		] Secure Boot Magic is not Matched!!!");
		return -1;
	}

	if(memcmp(sboot_header.sc_tag, SECURE_TAG, SECURE_TAG_SIZE)){
		LPRINTF("[FWDN		] Secure Boot TAG is not Matched!!!");
		return -1;
	}

	uiConfigSize		= (unsigned int)sboot_header.dram_init_size;
	uiROMFileSize		= (unsigned int)sboot_header.bootloader_size;
	uiConfigStart		= (unsigned int)sboot_header.dram_init_start;

	LPRINTF("[FWDN		] uiConfigSize : %x\n" , uiConfigSize);
	LPRINTF("[FWDN		] ioROMFileSize : %x\n", uiROMFileSize);

	res = FwdnUpdateHeaderBootSD(uiROMFileSize , uiConfigSize);
	if(res != SUCCESS)
		return res;

	res = FwdnWriteConfigBootSD(pucRomBuffer, uiConfigStart, uiConfigSize);
	if(res != SUCCESS)
		return res;

	res = FwdnWriteCodeBootSD(uiROMFileSize, uiConfigSize, pucRomBuffer+(unsigned int)sboot_header.bootloader_start);
	if(res != SUCCESS)
		return res;

#else
	uiConfigSize = (unsigned int)&InitRoutine_End - (unsigned int)&InitRoutine_Start;
	uiConfigStart = (unsigned int)&InitRoutine_Start - MEMBASE;


	res = FwdnUpdateHeaderBootSD(uiROMFileSize , uiConfigSize);
	if(res != SUCCESS)
		return res;

	res = FwdnWriteConfigBootSD(pucRomBuffer, uiConfigStart, uiConfigSize);
	if(res != SUCCESS)
		return res;

	res = FwdnWriteCodeBootSD(uiROMFileSize, uiConfigSize, pucRomBuffer);
	if(res != SUCCESS)
		return res;

#endif
#endif
	return 0;
}

unsigned int FwdnReadBootSDFirmware(unsigned int master)
{
	return 0;
}

int FwdnGetBootSDSerial(void)
{
#ifdef BOOTSD_INCLUDE
	ioctl_diskinfo_t 	disk_info;
	ioctl_diskrwpage_t	bootCodeRead;
	unsigned char		readData[512];
	int				res;

	/*----------------------------------------------------------------
		G E T	S E R I A L	N U M B E R
		----------------------------------------------------------------*/
	if (DISK_Ioctl(DISK_DEVICE_TRIFLASH, DEV_GET_DISKINFO, (void *)&disk_info) < 0)
		return -1;

	if (disk_info.sector_size != 512)
		return -1;

	bootCodeRead.start_page	= BOOTSD_GetHeaderAddr();
	bootCodeRead.rw_size	= 1;
	bootCodeRead.buff		= readData;
	res = DISK_Ioctl(DISK_DEVICE_TRIFLASH, DEV_BOOTCODE_READ_PAGE, (void *) &bootCodeRead);
	if ( res != 0) return res;

	/*----------------------------------------------------------------
		V E R I F Y	S E R I A L	N U M B E R
		----------------------------------------------------------------*/
	FWDN_FNT_VerifySN(readData, 32);
#endif
	switch (FWDN_DeviceInformation.DevSerialNumberType)
	{
		case SN_VALID_16:	return 16;
		case SN_VALID_32:	return 32;
		case SN_INVALID_16:	return -16;
		case SN_INVALID_32:	return -32;
		default:	return -1;
	}
}

int FwdnSetBootSDSerial(unsigned char *ucData, unsigned int overwrite)
{
#ifdef BOOTSD_INCLUDE
	ioctl_diskinfo_t 	disk_info;
	unsigned char		ucTempData[512];
	int				iRev;
	ioctl_diskrwpage_t	header;

	if (DISK_Ioctl(DISK_DEVICE_TRIFLASH, DEV_GET_DISKINFO, (void *)&disk_info) < 0)
	{
		return -1;
	}
	if (disk_info.sector_size != 512)
		return -1;

	memcpy( FWDN_DeviceInformation.DevSerialNumber, ucData , 32);
	g_uiFWDN_OverWriteSNFlag	=  overwrite;
	g_uiFWDN_WriteSNFlag		= 0;

	if ( overwrite == 0 )
	{
		/*----------------------------------------------------------------
			G E T	S E R I A L	N U M B E R
			----------------------------------------------------------------*/
		header.start_page	= BOOTSD_GetHeaderAddr();
		header.rw_size		= 1;
		header.buff			= ucTempData;
		iRev	=	DISK_Ioctl(DISK_DEVICE_TRIFLASH, DEV_BOOTCODE_READ_PAGE, (void *)&header);
		if(iRev != SUCCESS)
			return (-1);

		iRev = FWDN_FNT_SetSN(ucTempData, 32);
	}
#endif
	return 0;
}

#endif
