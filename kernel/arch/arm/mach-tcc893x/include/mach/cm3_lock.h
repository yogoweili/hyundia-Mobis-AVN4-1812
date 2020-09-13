#ifndef _CM3_LOCK_H_
#define _CM3_LOCK_H_

/*
**----------------------------------------------------------------------------
**  Definitions
**----------------------------------------------------------------------------
*/
#define CM_SYNC_LOCK_INIT_CODE	0x12345678
/*
**----------------------------------------------------------------------------
**  Type Definitions
**----------------------------------------------------------------------------
*/

typedef struct {
	volatile int m_lock;		//  only main core can write this variable, sub core just can read it.
	volatile int s_lock;
} t_cm3_sync_lock;

//This type should be same to Main Core's one.
//Add new item if any cotrolled IO Block is added at sub core.
typedef struct {
	volatile unsigned int init_code;
	volatile unsigned int pg_addr; // parking guide image address
	volatile unsigned int preview_addr; // preview address
	volatile unsigned int decoder_chip; // tw8836 -> 8836, tw9912 -> 9912, tvp5150 -> 5150
	volatile unsigned int camera_type; // common -> 0X100, pgs -> 0x200, avm_svideo -> 0X300, avm_component -> 0X400
	volatile t_cm3_sync_lock gpio_c_lock;
	volatile t_cm3_sync_lock lcd_lock;
} t_cm3_lock_list;


/*
**---------------------------------------------------------------------------
**  Global variables
**---------------------------------------------------------------------------
*/
extern t_cm3_lock_list* gpCM3SyncList;

/*
**---------------------------------------------------------------------------
**  Function Define
**---------------------------------------------------------------------------
*/
int cm3_sync_lock_init(void);
int cm3_sync_lock(t_cm3_sync_lock* lock);
int cm3_sync_unlock(t_cm3_sync_lock* lock);
int cm3_set_pg_addr(unsigned int addr);
int cm3_get_pg_addr(void);
int cm3_set_preview_addr(unsigned int addr);
int cm3_get_preview_addr(void);
//GT system

int cm3_set_decoder_chip(unsigned int chip);
int cm3_get_decoder_chip(void);
int cm3_set_camera_type(unsigned int type);
int cm3_get_camera_type(void);

#endif	// _CM3_LOCK_H_

