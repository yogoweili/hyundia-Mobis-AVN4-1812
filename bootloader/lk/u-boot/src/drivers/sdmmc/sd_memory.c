/****************************************************************************
 *   FileName    : sd_memory.c
 *   Description :
 ****************************************************************************
 *
 *   TCC Version 1.0
 *   Copyright (c) Telechips, Inc.
 *   ALL RIGHTS RESERVED
 *
 ****************************************************************************/
#include <asm/arch/fwdn/Disk.h>
#include <asm/arch/sdmmc/sd_bus.h>
#include <asm/arch/sdmmc/sd_memory.h>
#include <common.h>
//#include <debug.h>

static int SD_MEM_Ioctl(int iSlotIndex, int function, void *param);

unsigned long BOOTSD_GetHiddenBase(void)
{
	unsigned long ulCardSize;
	unsigned long ulBootStart;

	if(!(ulCardSize = (unsigned long)SD_BUS_GetCardCapacity(SD_BUS_GetBootSlotIndex())))
		return -1;

#if defined(FEATURE_SDMMC_MMC43_BOOT)
	ulBootStart = ulCardSize - 1;
#else
	ulBootStart = ulCardSize - BOOTSD_BOOT_PAGESIZE;
#endif

	return ulBootStart;
}

unsigned long BOOTSD_GetBootStart(unsigned int boot_partition)	// B120040-ksjung
{
#if defined(FEATURE_SDMMC_MMC43_BOOT)
	return 0;
#else
#if defined(CONFIG_TCC893X)
	unsigned long ulCardSize;
	unsigned long ulBootStart;

	if(!(ulCardSize = (unsigned long)SD_BUS_GetCardCapacity(SD_BUS_GetBootSlotIndex())))
		return -1;

	ulBootStart = ulCardSize - BOOTSD_BOOT_PAGESIZE;

	return ulBootStart;
#else
	unsigned long ulCardSize;
	unsigned long ulBootStart;

	if(!(ulCardSize = (unsigned long)SD_BUS_GetCardCapacity(SD_BUS_GetBootSlotIndex())))
		return -1;

	ulBootStart = ulCardSize - BOOTSD_BOOT_PAGESIZE;
	if( boot_partition != 0 ) {
		ulBootStart -= BOOTSD_BOOT_PAGESIZE/2;	//ks_8930
	}

	return ulBootStart;
#endif
#endif
}

//unsigned BOOTSD_GetHeaderAddr(void)	/* [1448] */
unsigned BOOTSD_GetHeaderAddr(unsigned int boot_partition)	// ks_8930
{
#if defined(FEATURE_SDMMC_MMC43_BOOT)
	return 0;
#else
#if defined(CONFIG_TCC893X)
	if( boot_partition )
		return BOOTSD_BOOT_PAGESIZE - 1 - (boot_partition*2);
	return BOOTSD_BOOT_PAGESIZE - 1; 	// Return address of 1st header
#else
	return BOOTSD_BOOT_PAGESIZE - 1; 	// Return address of 1st header
#endif
#endif
}

//unsigned BOOTSD_GetConfigCodeAddr(unsigned int uiConfigCodeBytes)	/* [1448] */
unsigned BOOTSD_GetConfigCodeAddr(unsigned int uiROM_Bytes, unsigned int uiConfigCodeBytes,
		unsigned int boot_partition)
{
#if defined(FEATURE_SDMMC_MMC43_BOOT)
	return 1;
#else
#if defined(CONFIG_TCC893X)
	if( boot_partition )
		return (BOOTSD_GetHeaderAddr(1) - 1) - BYTE_TO_SECTOR(uiConfigCodeBytes) -
			BYTE_TO_SECTOR(uiConfigCodeBytes) - BYTE_TO_SECTOR(uiROM_Bytes);
	return  (BOOTSD_GetHeaderAddr(1) - 1) - (BYTE_TO_SECTOR(uiConfigCodeBytes));

#else
	return  BOOTSD_GetHeaderAddr( boot_partition ) - BYTE_TO_SECTOR(uiConfigCodeBytes);
#endif
#endif
}

//unsigned BOOTSD_GetROMAddr(unsigned int uiROM_Bytes, unsigned int uiConfigCodeBytes)	/* [1448] */
unsigned BOOTSD_GetROMAddr(unsigned int uiROM_Bytes, unsigned int uiConfigCodeBytes,
		unsigned int boot_partition) 
{
#if defined(FEATURE_SDMMC_MMC43_BOOT)
	return  BOOTSD_GetConfigCodeAddr( uiROM_Bytes, uiConfigCodeBytes, boot_partition) + BYTE_TO_SECTOR(uiConfigCodeBytes);
#else
#if defined(CONFIG_TCC893X)
	return  BOOTSD_GetConfigCodeAddr( uiROM_Bytes, uiConfigCodeBytes, boot_partition) -
		( BYTE_TO_SECTOR(uiROM_Bytes) );

#else
	return  BOOTSD_GetConfigCodeAddr( uiROM_Bytes, uiConfigCodeBytes, boot_partition) - BYTE_TO_SECTOR(uiROM_Bytes);
#endif
#endif
}

int BOOTSD_Hidden_Info(pHidden_Size_T psHiddenSector)
{

	psHiddenSector->ulSectorSize[0] = BOOTSD_HIDDEN0_PAGESIZE;
	psHiddenSector->ulSectorSize[1] = BOOTSD_HIDDEN1_PAGESIZE;
	psHiddenSector->ulSectorSize[2] = BOOTSD_HIDDEN2_PAGESIZE;
	psHiddenSector->ulSectorSize[3] = BOOTSD_HIDDEN3_PAGESIZE;
	psHiddenSector->ulSectorSize[4] = BOOTSD_HIDDEN4_PAGESIZE;
	psHiddenSector->ulSectorSize[5] = BOOTSD_HIDDEN5_PAGESIZE;
	psHiddenSector->ulSectorSize[6] = BOOTSD_HIDDEN6_PAGESIZE;
	psHiddenSector->ulSectorSize[7] = BOOTSD_HIDDEN7_PAGESIZE;
	psHiddenSector->ulSectorSize[8] = BOOTSD_HIDDEN8_PAGESIZE;
	psHiddenSector->ulSectorSize[9] = BOOTSD_HIDDEN9_PAGESIZE;

	{
		int i;
		unsigned long ulHiddenBase;
		ulHiddenBase = BOOTSD_GetHiddenBase();
		for(i=0;i<BOOTSD_HIDDEN_MAX_COUNT;i++)
		{
			psHiddenSector->ulHiddenStart[i] =
				((i)? psHiddenSector->ulHiddenStart[i-1] : ulHiddenBase) - psHiddenSector->ulSectorSize[i];
		}
	}

	return 0;
}

int BOOTSD_Read(int reserved, unsigned long ulLBA_addr, unsigned long ulSector, void *buff)
{
	return SD_BUS_MEM_Request(SD_BUS_GetBootSlotIndex(), SD_BUS_MEM_READ, ulLBA_addr, ulSector, buff);
}
int BOOTSD_ReadMultiStart(unsigned long ulAddr, unsigned long size)
{
	return 0;
}
int BOOTSD_ReadMulti(int reserved, unsigned long ulLBA_addr, unsigned long ulSector, void *buff)
{
	return SD_BUS_MEM_Request(SD_BUS_GetBootSlotIndex(), SD_BUS_MEM_READ, ulLBA_addr, ulSector, buff);
}
int BOOTSD_ReadMultiStop(void)
{
	return 0;
}
int BOOTSD_Write(int reserved, unsigned long ulLBA_addr, unsigned long ulSector, void *buff)
{
	return SD_BUS_MEM_Request(SD_BUS_GetBootSlotIndex(), SD_BUS_MEM_WRITE, ulLBA_addr, ulSector, buff);
}
int BOOTSD_WriteMultiStart(unsigned long ulAddr, unsigned long size)
{
	return 0;
}
int BOOTSD_WriteMulti(int reserved, unsigned long ulLBA_addr, unsigned long ulSector, void *buff)
{
	return SD_BUS_MEM_Request(SD_BUS_GetBootSlotIndex(), SD_BUS_MEM_WRITE, ulLBA_addr, ulSector, buff);
}
int BOOTSD_WriteMultiStop(void)
{
	return 0;
}
int BOOTSD_Ioctl(int function, void *param)
{
	int iSlotIndex = SD_BUS_GetBootSlotIndex();
	int res = -1;


	if(iSlotIndex < 0)
		return res;

	switch(function)
	{

		case DEV_GET_BOOTCODE_SIZE:
			{
				unsigned long *pulSectorSize = (unsigned long*)param;
#if defined(FEATURE_SDMMC_MMC43_BOOT)
				sEXTCSDREG *pEXTCSD = SD_BUS_GetEXTCSD(iSlotIndex);
				*pulSectorSize = 0;
				if (pEXTCSD->EXTCSDRev >= 3)
				{
					*pulSectorSize = ((unsigned long)pEXTCSD->BootSizeMult/*unit=128KB*/) << 8;
					res = 0;
				}
#else
				*pulSectorSize = BOOTSD_BOOT_PAGESIZE;
				res = 0;
#endif
				break;
			}

		case DEV_BOOTCODE_READ_PAGE:
			{
				int retVal = 0;
				ioctl_diskrwpage_t	*rw	 =	(ioctl_diskrwpage_t *)param;
				unsigned int uiBootStart = (unsigned int)BOOTSD_GetBootStart(rw->boot_partition);
#if defined(FEATURE_SDMMC_MMC43_BOOT)
				sEXTCSDREG *pEXTCSD = SD_BUS_GetEXTCSD(iSlotIndex);
#endif

#if defined(FEATURE_SDMMC_MMC43_BOOT)
				//EnableBootPartition
				SD_BUS_switchEXTCSD( iSlotIndex, 179, 
						BITCSET( pEXTCSD->PartitionConfig, 0x07, rw->boot_partition+1), 0);
#endif

				retVal = BOOTSD_Read(iSlotIndex, uiBootStart + rw->start_page, rw->rw_size, rw->buff);

#if defined(FEATURE_SDMMC_MMC43_BOOT)
				//DisableBootPartition
				SD_BUS_switchEXTCSD( iSlotIndex, 179,
						BITCSET(pEXTCSD->PartitionConfig, 0x07, 0x00), 0);
#endif
				if(retVal < 0)
					return (-1);

				res = 0;
				break;
			}

		case DEV_BOOTCODE_WRITE_PAGE:
			{
				int retVal=0;
				ioctl_diskrwpage_t	*rw	 =	(ioctl_diskrwpage_t *)param;
				unsigned int uiBootStart = (unsigned int)BOOTSD_GetBootStart( rw->boot_partition );
#if defined(FEATURE_SDMMC_MMC43_BOOT)
				sEXTCSDREG *pEXTCSD = SD_BUS_GetEXTCSD(iSlotIndex);
#endif

#if defined(FEATURE_SDMMC_MMC43_BOOT)
				//EnableBootPartition
				SD_BUS_switchEXTCSD( iSlotIndex, 179,
						BITCSET(pEXTCSD->PartitionConfig, 0x07, rw->boot_partition+1), 0);
#endif

				if(BOOTSD_Write(iSlotIndex, uiBootStart + rw->start_page, rw->rw_size, rw->buff) < 0)
				{
					retVal = BOOTSD_Write(iSlotIndex, uiBootStart + rw->start_page, rw->rw_size, rw->buff);
				}

#if defined(FEATURE_SDMMC_MMC43_BOOT)
				//DisableBootPartition
				SD_BUS_switchEXTCSD( iSlotIndex, 179,
						BITCSET(pEXTCSD->PartitionConfig, 0x07, 0x00), 0);

				// if header writes...
				if(rw->start_page==BOOTSD_GetHeaderAddr(rw->boot_partition) && rw->rw_size==1)
				{
					retVal = BOOTSD_Write(iSlotIndex, BOOTSD_GetHiddenBase(),1,rw->buff);
				}
#endif
				if(retVal < 0)
				{
					printf("\x1b[1;36mfunc: %s, line: %d  \x1b[0m\n",__func__,__LINE__);
					return (-1);
				}

				res = 0;
				break;
			}

		case DEV_HIDDEN_READ_PAGE:
			{
				unsigned long		hiddenStart, limitedSize;
				sHidden_Size_T		stHidden;
				ioctl_diskrwpage_t	*rw	= (ioctl_diskrwpage_t *)param;

				BOOTSD_Hidden_Info(&stHidden);

				hiddenStart = stHidden.ulHiddenStart[rw->hidden_index];
				limitedSize = stHidden.ulSectorSize[rw->hidden_index];
				if(rw->hidden_index > BOOTSD_HIDDEN_MAX_COUNT || (rw->start_page + rw->rw_size) >= limitedSize)
					return (-1);
				else
				{
					if(BOOTSD_Read(iSlotIndex, (hiddenStart + rw->start_page), rw->rw_size, rw->buff) < 0)
						return BOOTSD_Read(iSlotIndex, (hiddenStart + rw->start_page), rw->rw_size, rw->buff);
					else
						return 0;
				}

				break;
			}

		case DEV_HIDDEN_WRITE_PAGE:
			{
				unsigned long		hiddenStart, limitedSize;
				sHidden_Size_T		stHidden;
				ioctl_diskrwpage_t	*rw	= (ioctl_diskrwpage_t *)param;

				BOOTSD_Hidden_Info(&stHidden);

				hiddenStart = stHidden.ulHiddenStart[rw->hidden_index];
				limitedSize = stHidden.ulSectorSize[rw->hidden_index];
				if(rw->hidden_index > BOOTSD_HIDDEN_MAX_COUNT || (rw->start_page + rw->rw_size) >= limitedSize)
					return (-1);
				else
				{
					if(BOOTSD_Write(iSlotIndex, (hiddenStart + rw->start_page), rw->rw_size, rw->buff) < 0)
						return BOOTSD_Write(iSlotIndex, (hiddenStart + rw->start_page), rw->rw_size, rw->buff);
					else
						return 0;
				}

				break;
			}

		case DEV_SERIAL_PROCESS:	/* [1448] */
			{
				//unsigned char	ucSID[32];
				//int	iSIDSize;
				//
				//iSIDSize = FWUG_GetSerialNumberTriFlash(ucSID);
				//if (iSIDSize > 0)
				//{
				//	SDMMC_memcpy(param, ucSID, iSIDSize);
				//	return iSIDSize;
				//}
				//else
				//	return (-1);
				//
				//break;
				res = ENOTSUPPORT;
				break;
			}

		case DEV_GET_DISK_SIZE:
			{
				unsigned long *pulSectorSize = (unsigned long*)param;

				if (SD_BUS_IsMemoryCard(iSlotIndex))
				{
					sHidden_Size_T stHidden;
					BOOTSD_Hidden_Info(&stHidden);
					*pulSectorSize = stHidden.ulHiddenStart[BOOTSD_HIDDEN_MAX_COUNT-1];

					res = 0;
				}
				break;
			}

		case DEV_GET_HIDDEN_COUNT:
			{
				unsigned long *pulCount = (unsigned long*)param;
				*pulCount = BOOTSD_HIDDEN_MAX_COUNT;
				res = 0;
				break;
			}

		case DEV_GET_HIDDEN_SIZE:
			{
				ioctl_diskhidden_t *pDiskHidden = (ioctl_diskhidden_t*)param;
				sHidden_Size_T stHidden;
				BOOTSD_Hidden_Info(&stHidden);
				if(pDiskHidden->ulIndex<BOOTSD_HIDDEN_MAX_COUNT)
				{
					pDiskHidden->ulTotalSector = stHidden.ulSectorSize[pDiskHidden->ulIndex];
					res = 0;
				}
				break;
			}

		default:
			res = SD_MEM_Ioctl(iSlotIndex,function, param);
			break;
	}

	return res;
}

#if defined(SD_UPDATE_INCLUDE)
int SD_Read(int reserved, unsigned long ulLBA_addr, unsigned long ulSector, void *buff)
{
	return SD_BUS_MEM_Request(SD_BUS_GetUpdateSlotIndex(), SD_BUS_MEM_READ, ulLBA_addr, ulSector, buff);
}

int SD_ReadMultiStart(unsigned long ulLBA_addr, unsigned long size)
{
	return 0;
}

int SD_ReadMulti(int reserved, unsigned long ulLBA_addr, unsigned long ulSector, void *buff)
{
	return SD_BUS_MEM_Request(SD_BUS_GetUpdateSlotIndex(), SD_BUS_MEM_READ, ulLBA_addr, ulSector, buff);
}

int SD_ReadMultiStop(void)
{
	return 0;
}

int SD_Write(int reserved, unsigned long ulLBA_addr, unsigned long ulSector, void *buff)
{
	return SD_BUS_MEM_Request(SD_BUS_GetUpdateSlotIndex(), SD_BUS_MEM_WRITE, ulLBA_addr, ulSector, buff);
}

int SD_WriteMultiStart(unsigned long ulLBA_addr, unsigned long size)
{
	return 0;
}

int SD_WriteMulti(int reserved, unsigned long ulLBA_addr, unsigned long ulSector, void *buff)
{
	return SD_BUS_MEM_Request(SD_BUS_GetUpdateSlotIndex(), SD_BUS_MEM_WRITE, ulLBA_addr, ulSector, buff);
}

int SD_WriteMultiStop(void)
{
	return 0;
}

int SD_Ioctl(int function, void *param)
{
	return SD_MEM_Ioctl(SD_BUS_GetUpdateSlotIndex(), function, param);
}
#endif //SD_UPDATE_INCLUDE

static int SD_MEM_Ioctl(int iSlotIndex, int function, void *param)
{
	int res = -1;


	switch(function)
	{
		case DEV_INITIALIZE:
			{
				if (SD_BUS_IsMemoryCard(iSlotIndex))
					res = 0;
				else if (SD_BUS_Probe(iSlotIndex) == 1)
					if (SD_BUS_IsMemoryCard(iSlotIndex))
						res = 0;

				break;
			}

		case DEV_GET_DISKINFO:
			{
				ioctl_diskinfo_t	*info = (ioctl_diskinfo_t *) param;

				info->sector_size =	512;
				if (SD_BUS_IsMemoryCard(iSlotIndex))
				{
					unsigned long ulSectorCount = SD_BUS_GetCardCapacity(iSlotIndex);
					info->head		= 31;
					info->sector	= 32;
					info->cylinder	= (unsigned short)(ulSectorCount/(31*32));
					res = 0;
				}
				else
				{
					info->cylinder	= 0;
					info->head		= 0;
					info->sector	= 0;
				}

				break;
			}

		case DEV_GET_MAX_SECTOR_PER_BLOCK:
			{
				unsigned short *pusMaxSectPerBlock = (unsigned short*)param;
				*pusMaxSectPerBlock = 128/*pSlot->sect_per_block*/;
				res = 0;
				break;
			}

		case DEV_GET_INSERTED:
			{
				res = SD_BUS_IsCardDetected(iSlotIndex);
				break;
			}

		case DEV_GET_INITED:
			{
				res = SD_BUS_IsMemoryCard(iSlotIndex);
				break;
			}

		case DEV_GET_WRITE_PROTECT:
			{
				res = SD_BUS_IsWriteProtected(iSlotIndex);
				break;
			}

		case DEV_STOP_TRANSFER:
			{
				SD_BUS_StopTransfer(iSlotIndex);
				res = 0;
				break;
			}

		case DEV_GET_DISK_SIZE:
			{
				unsigned long *pulSectorSize = (unsigned long*)param;
				if (SD_BUS_IsMemoryCard(iSlotIndex))
				{
					*pulSectorSize = SD_BUS_GetCardCapacity(iSlotIndex);
					res = 0;
				}
				break;
			}

		default:
			{
				res = ENOTSUPPORT;
				break;
			}
	}

	return res;
}

/* end of file */

