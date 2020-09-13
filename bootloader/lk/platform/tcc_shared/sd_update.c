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

#include <fwdn/Disk.h>
#include <sdmmc/sd_memory.h>
#include <sdmmc/sd_update.h>
#endif

#if defined(_LINUX_)
#include <printf.h>
#define	LPRINTF	printf
#elif defined(_WINCE_)
#define	LPRINTF	EdbgOutputDebugString
#else
#define	LPRINTF
#endif

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

//int FwdnUpdateGetBootSDSerial(char* SerialNum);

#define			CRC32_SIZE	4
#define			FWDN_WRITE_BUFFER_SIZE		(1024*1024)
unsigned int	global_buf[ FWDN_WRITE_BUFFER_SIZE/4 ];

#if 1
void mmc_init_page(
		page_t *p,
		unsigned long start_addr,
		unsigned long size,
		unsigned long hidden,
		unsigned char *buf,
		unsigned int boot_partition)
{
	p->start_page = start_addr;
	p->rw_size = size;
	p->hidden_index = hidden;
	p->buff = buf;
	p->boot_partition = boot_partition;
}
#else
#define MMC_INIT_PAGE(p, x, y, z, q, r) ({  \
		p.start_page = x; \
		p.rw_size = y;  \
		p.hidden_index = z; \
		p.buff = q; \
		p.boot_partition = r; })
#endif

#if defined(TSBM_ENABLE)
int init_secure_head(BOOTSD_sHeader_Info_T *h, unsigned char* buf)
{
	memset( h, 0x00, sizeof(BOOTSD_sHeader_Info_T) );
	memcpy( (unsigned char*)h, buf, sizeof(BOOTSD_sHeader_Info_T) );

	return 0;
}
#endif

int mmc_update_valid_test(unsigned int rom_size)
{
	ioctl_diskinfo_t disk_info;

	if( DISK_Ioctl( DISK_DEVICE_TRIFLASH, DEV_GET_DISKINFO, (void *)&disk_info ) ||
			disk_info.sector_size != 512 ||
			!rom_size )
		return -1;

	return 0;
}

int mmc_get_serial_num(unsigned char* serial_num, unsigned int boot_partition)
{
	page_t p;
	unsigned char buf[512];

	if( mmc_update_valid_test(1) )
		return -1;

	mmc_init_page( &p, BOOTSD_GetHeaderAddr( boot_partition ), 1, 0, buf, boot_partition );

	if( DISK_Ioctl(DISK_DEVICE_TRIFLASH, DEV_BOOTCODE_READ_PAGE, (void *)&p) )
		return -1;

	if( serial_num != NULL )
		memcpy(serial_num, ((BOOTSD_Header_Info_T*)buf)->ucSerialNum, 64);

	FWDN_FNT_VerifySN(buf, 32);

	switch( FWDN_DeviceInformation.DevSerialNumberType )
	{
		case SN_VALID_16: return 16;
		case SN_VALID_32: return 32;
		case SN_INVALID_16: return -16;
		case SN_INVALID_32: return -32;
	}
	return -1;
}


static int __do_mmc_update( page_t *p )
{
	page_t q;
	unsigned char *buf;

	if( DISK_Ioctl( DISK_DEVICE_TRIFLASH, DEV_BOOTCODE_WRITE_PAGE, (void*)p ) )
		return -1;

	buf = p->buff + ((p->rw_size) << 9);
	mmc_init_page( &q, p->start_page, p->rw_size, p->hidden_index, buf, p->boot_partition );

	if( DISK_Ioctl( DISK_DEVICE_TRIFLASH, DEV_BOOTCODE_READ_PAGE, (void*)&q ) ||
			memcmp( p->buff, q.buff, (p->rw_size) << 9 ) )
		return -1;
	return 0;
}

int mmc_update_rom_code(
		unsigned long rom_size,
		unsigned long config_size,
		unsigned char *buf,
		unsigned int boot_partition)
{
	page_t p;
#if 0
	printf("%s, %08x\n", __func__,
			BOOTSD_GetBootStart(boot_partition) +
			BOOTSD_GetROMAddr(rom_size, config_size+CRC32_SIZE, boot_partition));
#endif


	mmc_init_page(
			&p,
#if defined(TSBM_ENABLE)
			BOOTSD_GetROMAddr(rom_size, config_size, boot_partition),
#else
			BOOTSD_GetROMAddr(rom_size, config_size+CRC32_SIZE, boot_partition),
#endif
			BYTE_TO_SECTOR(rom_size),
			0,
			buf,
			boot_partition
			);

	if( __do_mmc_update( &p ) ) {
		LPRINTF("[FWDN\t] Write Code Failed\n");
		return ERR_FWUG_FAIL_CODEWRITETFLASH;
	}
	LPRINTF("[FWDN\t] Write Code Completed\n");
	return SUCCESS;
}


int mmc_update_config_code(
		unsigned long rom_size, // ks_8930
		unsigned long config_size,
		unsigned char *buf,
		unsigned int boot_partition)
{
	page_t  p;
	unsigned long crc;
	unsigned char *p_buf = (unsigned char*)global_buf;

	memset( p_buf, 0, FWDN_WRITE_BUFFER_SIZE );
	memcpy( p_buf, buf, config_size );
#if !defined(TSBM_ENABLE)
	crc = FWUG_CalcCrc8( (unsigned char*)p_buf, config_size );
	word_of(p_buf + config_size) = crc;
	config_size += CRC32_SIZE;
#endif

	mmc_init_page(
			&p,
			BOOTSD_GetConfigCodeAddr(rom_size, config_size, boot_partition), // ks_8930
			BYTE_TO_SECTOR(config_size),
			0,
			p_buf,
			boot_partition
			);

	if( __do_mmc_update( &p ) )
	{
		LPRINTF("[FWDN\t] Write Config Code Failed\n");
		return ERR_FWUG_FAIL_HEADERWRITETFLASH;
	}
	LPRINTF("[FWDN\t] Write Config Code Completed\n");
	return SUCCESS;
}

int mmc_update_header (
		unsigned long firmware_base,
		unsigned long config_size,
		unsigned long rom_size,
		unsigned int boot_partition,
		int fwdn_mode)
{
	page_t p, q;
	BOOTSD_Header_Info_T header;
	unsigned char *pTempData   = (unsigned char*)global_buf;

	memset(&header, 0, sizeof(BOOTSD_Header_Info_T));
	memset(pTempData, 0x00, FWDN_WRITE_BUFFER_SIZE);

	header.ulEntryPoint = firmware_base;
	header.ulBaseAddr = firmware_base;
#if defined(TSBM_ENABLE)
	header.ulConfSize = config_size;     //CRC plus
#else
	header.ulConfSize = config_size + CRC32_SIZE;     //CRC plus
#endif
	header.ulROMFileSize = rom_size;
	header.ulBootConfig = (BOOTCONFIG_NORMAL_SPEED | BOOTCONFIG_4BIT_BUS);


	header.ulConfigCodeBase = BOOTSD_GetBootStart(boot_partition) +
#if defined(TSBM_ENABLE)
		BOOTSD_GetConfigCodeAddr(rom_size, config_size, boot_partition);
#else
	BOOTSD_GetConfigCodeAddr(rom_size, config_size+CRC32_SIZE, boot_partition);
#endif

	header.ulBootloaderBase = BOOTSD_GetBootStart(boot_partition) +
#if defined(TSBM_ENABLE)
		BOOTSD_GetROMAddr(rom_size, config_size, boot_partition);
#else
	BOOTSD_GetROMAddr(rom_size, config_size+CRC32_SIZE, boot_partition);
#endif


#if !defined(FEATURE_SDMMC_MMC43_BOOT)
	if( header.ulBootConfig & BOOTCONFIG_8BIT_BUS)
	{
		LPRINTF("[FWDN\t] SD Boot can't support 8 bit data rate\n");
		return -1;
	}
#endif
	{
		sHidden_Size_T stHidden;
		BOOTSD_Hidden_Info(&stHidden);
		header.ulHiddenStartAddr = stHidden.ulHiddenStart[0] + stHidden.ulSectorSize[0];
		memcpy( (void *)header.ulHiddenSize, (const void*)stHidden.ulSectorSize, (BOOTSD_HIDDEN_MAX_COUNT << 2));
	}

	memcpy(header.ucHeaderFlag, "HEAD", 4);

	if( fwdn_mode && g_uiFWDN_WriteSNFlag == 1 )
	{
		unsigned int crc;
		mmc_init_page( &p, BOOTSD_GetHeaderAddr( boot_partition ), 1, 0, pTempData, boot_partition);//ks_8930
		DISK_Ioctl(DISK_DEVICE_TRIFLASH, DEV_BOOTCODE_READ_PAGE, (void*)&p);
		crc = FWUG_CalcCrc8((unsigned char*) &header, 512-4);
		if( word_of(pTempData + 512 -4) != crc)
			return -1;
		memcpy( header.ucSerialNum, pTempData+16, 64 );
	} else if( !fwdn_mode )
		mmc_get_serial_num( header.ucSerialNum, boot_partition );

	FWDN_FNT_InsertSN( header.ucSerialNum );                //Write Serial number

	header.ulCRC = FWUG_CalcCrc8((unsigned char*) &header, 512-4);

	memcpy(pTempData, &header, 512);
	mmc_init_page( &p, BOOTSD_GetHeaderAddr( boot_partition ), 1, 0, pTempData, boot_partition);

	if( __do_mmc_update( &p ) )
		return ERR_FWUG_FAIL_HEADERWRITETFLASH;
	LPRINTF("[FWDN\t] Write Header Completed!!\n");
	return SUCCESS;
}

extern unsigned long InitRoutine_Start;
extern unsigned long InitRoutine_End;

int mmc_update_bootloader(unsigned int rom_size, unsigned int boot_partition)
{
	unsigned int config_size = (unsigned int)&InitRoutine_End - (unsigned int)&InitRoutine_Start;

	if( mmc_update_valid_test(rom_size) )
		return -1;

#if defined(FEATURE_SDMMC_MMC43_BOOT)
	LPRINTF("[FWDN\t] Begin eMMC Bootloader write to T-Flash\n");
#else
	LPRINTF("[FWDN\t] Begin SD Bootloader write to T-Flash\n");
#endif

	if( mmc_update_header(MEMBASE, config_size, rom_size, boot_partition, 0) )
		return -1;
	if( mmc_update_config_code(rom_size, config_size, (unsigned char*)&InitRoutine_Start, boot_partition) ) //ks_8930
		return -1;
	if( mmc_update_rom_code(rom_size, config_size, (unsigned char*)ROMFILE_TEMP_BUFFER,
				boot_partition) )
		return -1;

	LPRINTF("[FWDN\t] Updating Bootloader Complete!\n");
	return SUCCESS;
}

#if defined(TSBM_ENABLE)
int mmc_update_secure_bootloader(unsigned int rom_size, int boot_partition)
{
#ifdef BOOTSD_INCLUDE
	unsigned char *buf = (unsigned char*)ROMFILE_TEMP_BUFFER;
	BOOTSD_sHeader_Info_T secure_h;

	if( mmc_update_valid_test(rom_size) )
		return -1;

	if( init_secure_head(&secure_h, buf) )
		return -1;

	if( mmc_update_header(
				MEMBASE,
				secure_h.dram_init_size,
				secure_h.bootloader_size,
				boot_partition,
				0 ) )
		return -1;

	if( mmc_update_config_code(
				/*rom_size, // ks_8930*/
				secure_h.bootloader_size,
				secure_h.dram_init_size,
				(unsigned char*)(buf+(unsigned int)secure_h.dram_init_start),
				boot_partition ) )
		return -1;

	if( mmc_update_rom_code(
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


unsigned int FwdnUpdateReadBootSDFirmware(unsigned int master)
{
	return 0;
}

int FwdnUpdateSetBootSDSerial(unsigned char *ucData, unsigned int overwrite)
{
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
	return 0;
}
#if 0
static int _loc_BootSD_Write_N_Verify(ioctl_diskrwpage_t *rwpage, unsigned long Size)
{
	if(DISK_Ioctl(DISK_DEVICE_TRIFLASH, DEV_BOOTCODE_WRITE_PAGE, (void *) rwpage))
		return -1;
	{//Verify
		ioctl_diskrwpage_t	veri_rwpage;
		unsigned char 		*pVerifyData	= rwpage->buff + ((rwpage->rw_size) << 9);
		
		veri_rwpage.rw_size		= rwpage->rw_size;
		veri_rwpage.buff		= pVerifyData;
		veri_rwpage.start_page	= rwpage->start_page;

		if (DISK_Ioctl(DISK_DEVICE_TRIFLASH, DEV_BOOTCODE_READ_PAGE, (void *) & veri_rwpage))
			return -1;
		if (memcmp(rwpage->buff, pVerifyData, Size) != 0)
			return -1;		// Verifying Error
	}
	return SUCCESS;
}

static int FwdnUpdateCodeBootSD(unsigned long ulROMFileSize, unsigned long ulConfCodeSize, unsigned char *pWriteBufAddr)
{
	ioctl_diskrwpage_t	sBootCodeWrite;

#if defined(TSBM_ENABLE)
	sBootCodeWrite.start_page	=   (unsigned long)BOOTSD_GetROMAddr(ulROMFileSize, ulConfCodeSize);
#else
	sBootCodeWrite.start_page	=   (unsigned long)BOOTSD_GetROMAddr(ulROMFileSize, ulConfCodeSize + CRC32_SIZE) ;
#endif
	sBootCodeWrite.buff			= 	pWriteBufAddr;
	sBootCodeWrite.rw_size 		= 	BYTE_TO_SECTOR(ulROMFileSize);

	if(_loc_BootSD_Write_N_Verify(&sBootCodeWrite, ulROMFileSize))
	{		
		LPRINTF("[FWDN        ] Updata BOOTLOADER Verify FAiled !! Check([Func : %s] [Line : %d])\n", __func__ , __LINE__);
		return ERR_FWUG_FAIL_CODEWRITETFLASH;
	}
	LPRINTF("[FWDN		] Write Code Completed\n");
	return SUCCESS;
	
	return SUCCESS;
}

int FwdnUpdateConfigBootSD(unsigned long ulConfigSize, unsigned char *pConfigAddr)
{
	unsigned long 		ulCRC;
	ioctl_diskrwpage_t	rwpage;
	unsigned char *pTempData = (unsigned char*)tempBuffer;
	
	memset(pTempData, 0, FWDN_WRITE_BUFFER_SIZE);
	{
		//Copy from Config code to Buffer
		memcpy(pTempData, pConfigAddr, ulConfigSize);
#if !defined(TSBM_ENABLE)		
		ulCRC = FWUG_CalcCrc8((unsigned char*) pTempData, ulConfigSize);
		word_of(pTempData + ulConfigSize) = ulCRC;
		ulConfigSize += CRC32_SIZE;
#endif
	}


	rwpage.buff			= pTempData;
	rwpage.start_page	= (unsigned long)BOOTSD_GetConfigCodeAddr(ulConfigSize);
	rwpage.rw_size		= BYTE_TO_SECTOR(ulConfigSize);

	if(_loc_BootSD_Write_N_Verify(&rwpage, ulConfigSize))
	{
		LPRINTF("[FWDN        ] Updata DRAM Config Code Verify Failed !! check([Func : %s] [Line : %s])\n", __func__ , __LINE__);
		return ERR_FWUG_FAIL_HEADERWRITETFLASH;
	}	
	LPRINTF("[FWDN		] Write Config Code Completed\n");
	return SUCCESS;
}

static int FwdnUpdateHeaderBootSD(unsigned long ulFirmwareBase, unsigned long ulROMFileSize, unsigned long ulConfCodeSize)
{
	//=====================================================================
	// Set SD Booting Header's Value ======================================
	//=====================================================================	
	ioctl_diskrwpage_t	rwpage;
	BOOTSD_Header_Info_T sHeaderInfo;
	unsigned char		*pTempData   = (unsigned char*)tempBuffer;
	
	memset(&sHeaderInfo, 0, sizeof(BOOTSD_Header_Info_T));
	
	sHeaderInfo.ulEntryPoint 		= ulFirmwareBase;		
	sHeaderInfo.ulBaseAddr 			= ulFirmwareBase;
#if defined(TSBM_ENABLE)
	sHeaderInfo.ulConfSize			= ulConfCodeSize;				//Do not need CRC on Secure boot			
#else
	sHeaderInfo.ulConfSize			= ulConfCodeSize + CRC32_SIZE;			//CRC plus			
#endif
	sHeaderInfo.ulROMFileSize		= ulROMFileSize;
	sHeaderInfo.ulBootConfig		= (BOOTCONFIG_NORMAL_SPEED | BOOTCONFIG_4BIT_BUS);
	#if !defined(FEATURE_SDMMC_MMC43_BOOT)
	if(sHeaderInfo.ulBootConfig & BOOTCONFIG_8BIT_BUS)
	{
		LPRINTF("[FWDN        ] SD Boot can't support 8 bit data rate\n");
		return -1;
	}
	#endif
	{
		sHidden_Size_T stHidden;
		BOOTSD_Hidden_Info(&stHidden);
		sHeaderInfo.ulHiddenStartAddr = stHidden.ulHiddenStart[0] + stHidden.ulSectorSize[0];
		memcpy((void *)sHeaderInfo.ulHiddenSize, (const void*)stHidden.ulSectorSize, (BOOTSD_HIDDEN_MAX_COUNT << 2));
	}
	
	FwdnUpdateGetBootSDSerial(sHeaderInfo.ucSerialNum);

	FWDN_FNT_InsertSN(sHeaderInfo.ucSerialNum);								//Write Serial number
	memcpy(sHeaderInfo.ucHeaderFlag, "HEAD", 4);
	
	sHeaderInfo.ulCRC				= FWUG_CalcCrc8((unsigned char*) &sHeaderInfo, 512-4);
	
	//=====================================================================
	// Begin Writing Header to T-Flash ====================================
	//=====================================================================
	
	memset(pTempData, 0x00, FWDN_WRITE_BUFFER_SIZE);
	memcpy(pTempData, &sHeaderInfo, 512);
	
	rwpage.start_page	= BOOTSD_GetHeaderAddr();
	rwpage.buff			= pTempData;
	rwpage.rw_size		= 1;

	if(_loc_BootSD_Write_N_Verify(&rwpage, 512)){
		LPRINTF("[FWDN		] BOOTSD Header Verify Failed !! Check([Func : %s] [Line : %s])\n", __func__, __LINE__);
		return ERR_FWUG_FAIL_HEADERWRITETFLASH;
	}

	LPRINTF("[FWDN		] Write Header Completed!!\n");
	return SUCCESS;
}

int FwdnUpdateBootSDFirmware(unsigned int uiROMFileSize)
{
	int					res = 1;
	ioctl_diskinfo_t 	disk_info;
		
	unsigned long	*InitRoutine_Start 	=	CONFIG_CODE_START_ADDRESS;
	unsigned long	*InitRoutine_End	=	CONFIG_CODE_END_ADDRESS;
	unsigned long 	*FirmwareBase 		=	FIRMWARE_BASE_ADDRESS;
	
	LPRINTF("%x , %x , %x\n" , CONFIG_CODE_START_ADDRESS , CONFIG_CODE_END_ADDRESS , FIRMWARE_BASE_ADDRESS);

	unsigned long InitCode_Offset_ROM   = (*InitRoutine_Start) - (*FirmwareBase);
	unsigned long Init_Start = InitCode_Offset_ROM + ROMFILE_TEMP_BUFFER;
	unsigned long ulConfigSize = *InitRoutine_End - *InitRoutine_Start;

	//=====================================================================	
	// Check                   ============================================
	//=====================================================================
	if (DISK_Ioctl(DISK_DEVICE_TRIFLASH, DEV_GET_DISKINFO, (void *)&disk_info))
		return -1;
	if (disk_info.sector_size != 512 || !uiROMFileSize)
		return -1;

#if defined(TSBM_ENABLE)
	LPRINTF("[FWDN		] TSBM UPDATE START\n");

	BOOTSD_sHeader_Info_T sboot_header; 
	unsigned char pTempBuffer[512];
	
	memset(&sboot_header, 0x00, sizeof(BOOTSD_sHeader_Info_T));
	memcpy((unsigned char*)&sboot_header, ROMFILE_TEMP_BUFFER, sizeof(BOOTSD_sHeader_Info_T)); 

	if(memcmp(sboot_header.magic, SBOOT_MAGIC, SBOOT_MAGIC_SIZE)){
		LPRINTF("[FWDN		] Secure Boot Magic is not Matched !!!\n");
		return -1;
	}

	if(memcmp(sboot_header.sc_tag, SECURE_TAG, SECURE_TAG_SIZE)){
		LPRINTF("[FWDN		] Secure Boot TAG is not Matched !!!\n");
		return -1;
	}

	unsigned int seConfigSize		= (unsigned int)sboot_header.dram_init_size;
	unsigned int seROMFileSize		= (unsigned int)sboot_header.bootloader_size;
	unsigned int seConfigStart		= (unsigned int)sboot_header.dram_init_start;

	LPRINTF("[FWDN		] Secure Boot ConfigSize : %x\n", seConfigSize);
	LPRINTF("[FWDN		] Secure Boot RomFile Size : %x\n", seROMFileSize);
	LPRINTF("[FWDN		] Secure Boot Config Code Start  : %x\n", seConfigStart);

	res = FwdnUpdateHeaderBootSD(BASE_ADDR, seROMFileSize, seConfigSize);
	if(res != SUCCESS)
		return res;
	
	res = FwdnUpdateConfigBootSD(seConfigSize, (unsigned char *)(ROMFILE_TEMP_BUFFER + (unsigned int)seConfigStart));
	if(res != SUCCESS)
		return res;
	
	res = FwdnUpdateCodeBootSD(seROMFileSize, seConfigSize, (unsigned char *)(ROMFILE_TEMP_BUFFER + (unsigned int)sboot_header.bootloader_start));
	if(res != SUCCESS)
		return res;

#else

	//=====================================================================	
	// Firware Write Start     ============================================
	//=====================================================================
#if defined(FEATURE_SDMMC_MMC43_BOOT)
	LPRINTF("[FWDN        ] Begin eMMC Bootloader write to T-Flash\n");
#else
	LPRINTF("[FWDN        ] Begin SD Bootloader write to T-Flash\n");
#endif
	
	res = FwdnUpdateHeaderBootSD(*FirmwareBase, uiROMFileSize, ulConfigSize);
	if(res != SUCCESS)
		return res;
	
	res = FwdnUpdateConfigBootSD(ulConfigSize, (unsigned char *)Init_Start);
	if(res != SUCCESS)
		return res;
	
	res = FwdnUpdateCodeBootSD(uiROMFileSize, ulConfigSize, (unsigned char *)ROMFILE_TEMP_BUFFER);
	if(res != SUCCESS)
		return res;
#endif
	
	LPRINTF("[FWDN        ] Bootloader write Complete!\n");
	return SUCCESS;
}

unsigned int FwdnUpdateReadBootSDFirmware(unsigned int master)
{
	return 0;
}

int FwdnUpdateGetBootSDSerial(char* SerialNum)
{
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

	memcpy(SerialNum ,((BOOTSD_Header_Info_T*)readData)->ucSerialNum, 64);

	FWDN_FNT_VerifySN(readData, 32);
	switch (FWDN_DeviceInformation.DevSerialNumberType)
	{
		case SN_VALID_16:	return 16;
		case SN_VALID_32:	return 32;
		case SN_INVALID_16:	return -16;
		case SN_INVALID_32:	return -32;
		default:	return -1;
	}
}

int FwdnUpdateSetBootSDSerial(unsigned char *ucData, unsigned int overwrite)
{
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
	return 0;
}

#endif


#endif
