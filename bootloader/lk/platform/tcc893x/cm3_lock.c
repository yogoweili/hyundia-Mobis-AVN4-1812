/****************************************************************************/
/* MODULE:                                                                  */
/*   cm3_lock.c - Synchronize Main core and Sub core			            */
/****************************************************************************/
/*
 *   TCC Version 0.0
 *   Copyright (c) telechips, Inc.
 *   ALL RIGHTS RESERVED
*/
/****************************************************************************/

#include "cm3_lock.h"

/*
**----------------------------------------------------------------------------
**  Definitions
**----------------------------------------------------------------------------
*/
#define SYNC_LOCK_BASE_ADDR		0x1000FC00
// if you need to increase this value, check stack and heap memory in map file.
#define SYNC_LOCK_MAX_SIZE		0x00000200	
/*
**---------------------------------------------------------------------------
**  General defines
**---------------------------------------------------------------------------
*/

/*
**---------------------------------------------------------------------------
**  Global variables
**---------------------------------------------------------------------------
*/
t_cm3_lock_list* gpCM3SyncList;

/*
**---------------------------------------------------------------------------
**  Internal variables
**---------------------------------------------------------------------------
*/


/*
**---------------------------------------------------------------------------
**  Internal Function Define
**---------------------------------------------------------------------------
*/



/*
**---------------------------------------------------------------------------
**  External Function Define
**---------------------------------------------------------------------------
*/
int cm3_sync_lock_init(void)
{
	if (sizeof(t_cm3_lock_list) > SYNC_LOCK_MAX_SIZE)
		return -1;
	
	gpCM3SyncList = (t_cm3_lock_list*)SYNC_LOCK_BASE_ADDR;

	return 0;
}


int cm3_sync_lock(t_cm3_sync_lock* lock)
{
	int ret;

	if (lock == 0)
		return -1;
	
	if (gpCM3SyncList->init_code != CM_SYNC_LOCK_INIT_CODE)
		return -2;

	do {
		while(lock->s_lock);

		lock->m_lock = 1;
		mdelay(1);
		if(lock->s_lock)
			lock->m_lock = 0;
			
	} while(lock->m_lock == 0);

	return 0;
}

int cm3_sync_unlock(t_cm3_sync_lock* lock)
{
	if (lock == 0)
		return -1;

	if (gpCM3SyncList->init_code != CM_SYNC_LOCK_INIT_CODE)
		return -2;
	
	lock->m_lock = 0;
	
	return 0; 
}

int cm3_set_pg_addr(unsigned int addr)
{
	gpCM3SyncList->pg_addr = addr;
	//printf("%s() -   parking guide image address is 0x%x. \n", __func__, gpCM3SyncList->pg_addr);
	return 0;
}

int cm3_get_pg_addr(void)
{
	return gpCM3SyncList->pg_addr;
}

int cm3_set_preview_addr(unsigned int addr)
{
	gpCM3SyncList->preview_addr = addr;
	//printf("%s() -   preview address is 0x%x. \n", __func__, gpCM3SyncList->preview_addr);
	return 0;
}

int cm3_get_preview_addr(void)
{
	return gpCM3SyncList->preview_addr;
}

int cm3_set_decoder_chip (unsigned int chip)
{
	gpCM3SyncList->decoder_chip = chip;
	//printf("%s() -   decoder chip number is 0x%x. \n", __func__, gpCM3SyncList->decoder_chip);
	return 0;
}

int cm3_get_decoder_chip(void)
{
	return gpCM3SyncList->decoder_chip;
}

// GT system IM002A-368
int cm3_set_camera_type(unsigned int type)
{
	gpCM3SyncList->camera_type = type;
	//printf("%s() -   camera type is 0x%x. \n", __func__, gpCM3SyncList->camera_type);

	return 0;
}

int cm3_get_camera_type(void)
{
	return gpCM3SyncList->camera_type;
}


