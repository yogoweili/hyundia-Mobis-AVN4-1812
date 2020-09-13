#ifndef __DAUDIO_SETTINGS__
#define __DAUDIO_SETTINGS__

#include <mach/daudio.h>
#include <mach/daudio_eng.h>

#define SETTINGS_PARTITION      "settings"

//Writing FLAG
#define WRITING_STARTED			0x44455452415453
#define WRITING_FINISHED		0x44454853494E4946

/**
 * SETTING PARTITION STRUCTURE.
 *
 * IE ORIGINAL DATA			0 ~ 511
 * IE ORIGINAL CMD			512 ~ 1023
 * IE BACKUP DATA			1024 ~ 1535
 * IE BACKUP CMD			1536 ~ 2047
 * ENGINEER MODE			2048 ~ 2559
 **/
#define OFFSET_IE_SETTINGS						0
#define OFFSET_IE_SETTINGS_CMD					OFFSET_IE_SETTINGS + 512
#define OFFSET_IE_SETTINGS_BACKUP_DATA			OFFSET_IE_SETTINGS_CMD + 512
#define OFFSET_IE_SETTINGS_BACKUP_CMD			OFFSET_IE_SETTINGS_BACKUP_DATA + 512
#define OFFSET_EM_SETTINGS						OFFSET_IE_SETTINGS_BACKUP_CMD + 512

enum CMD_TYPE {
	NONE,
	BACKUP,
};

int read_ie_settings(ie_setting_info *info);
int write_ie_settings(ie_setting_info *info);
void print_ie_info(ie_setting_info *info);

int read_em_setting(em_setting_info *info);
int write_em_setting(em_setting_info *info);
void print_em_info(em_setting_info *info);

#endif
