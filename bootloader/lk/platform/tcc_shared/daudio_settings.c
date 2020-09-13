#include <daudio_settings.h>

#include <stdio.h>
#include <string.h>

#include <dev/flash.h>
#include <sdmmc/emmc.h>
#include <sys/types.h>

#define TAG_SETTINGS		"[cleinsoft settings]"
#define BYTE_TO_SECTOR(X)	((X + 511) >> 9)

#define DEBUG_SETTINGS		1

#if (DEBUG_SETTINGS)
#define LKPRINT(fmt, args...) dprintf(INFO, TAG_SETTINGS " " fmt, ##args)
#else
#define LKPRINT(fmt, args...) do {} while(0)
#endif

#define FORCE_RESTORE_CMD
#undef	FORCE_RESTORE_CMD

char *ie_setting_field_name[] = {
	"id",
	"version",
	"tw8836_brightness",
	"tw8836_contrast",
	"tw8836_hue",
	"tw8836_saturation",
	"twxxxx_cam_brightness",
	"twxxxx_cam_contrast",
	"twxxxx_cam_hue",
	"twxxxx_cam_saturation",
	"tcc_brightness",
	"tcc_contrast",
	"tcc_hue",
	"tcc_saturation",
	"tcc_video_brightness",
	"tcc_video_contrast",
	"tcc_video_gamma",
	"tcc_video_saturation",
	"tcc_video_brightness2",
	"tcc_video_contrast2",
	"tcc_video_gamma2",
	"tcc_video_saturation2",
	"temp_1",
	"temp_2",
	"temp_3",
	"temp_4",
};

/**
 * Get the cmd.
 * @param	type The cmd type
 * @param	cmd Pointer to receive the result.
 * @return	1 - success, 0 - error.
 */
static int get_cmd(enum CMD_TYPE type, unsigned long long *cmd)
{
	int ret = 0;
	unsigned long long ptn = 0;
	const unsigned int page_size = flash_page_size();
	unsigned char data[page_size];

	ptn = emmc_ptn_offset((unsigned char*)SETTINGS_PARTITION);
	if (ptn == 0)
		return ret;

	dprintf(SPEW, "%s %s ptn: 0x%llx\n", TAG_SETTINGS, __func__, ptn);

	switch (type)
	{
		case NONE:
			ptn += BYTE_TO_SECTOR(OFFSET_IE_SETTINGS_CMD);
			break;

		case BACKUP:
			ptn += BYTE_TO_SECTOR(OFFSET_IE_SETTINGS_BACKUP_CMD);
			break;

		default:
			return ret;
	}

	ret = emmc_read(ptn, page_size, data);
	if (ret == 0)
	{
		ret = 1;
		memcpy(cmd, data, sizeof(long long));
		dprintf(SPEW, "%s %s data_addr:0x%llx cmd: 0x%llx\n", TAG_SETTINGS, __func__, ptn, *cmd);
	}

	return ret;
}

/**
 * Set the cmd. Not used in LK.
 * @param	type The cmd type
 * @param	cmd The new cmd value to set.
 * @return	1 - success, 0 - error.
 */
static int set_cmd(enum CMD_TYPE type, unsigned long long *cmd)
{
	int ret = 0;
	unsigned long long ptn = 0;
	const unsigned int page_size = flash_page_size();
	unsigned int data[page_size];

	ptn = emmc_ptn_offset((unsigned char*)SETTINGS_PARTITION);
	if (ptn == 0)
		return ret;

	switch (type)
	{
		case NONE:
			ptn += BYTE_TO_SECTOR(OFFSET_IE_SETTINGS_CMD);
			break;

		case BACKUP:
			ptn += BYTE_TO_SECTOR(OFFSET_IE_SETTINGS_BACKUP_CMD);
			break;

		default:
			return ret;
	}

	ret = emmc_read(ptn, page_size, data);
	if (ret != 0)
		return 0;

	memcpy(data, cmd, sizeof(long long));

	dprintf(SPEW, "%s %s data 0x%llx\n", TAG_SETTINGS, __func__, *cmd);

	ret = emmc_write(ptn, page_size, data);
	if (ret == 0)
		ret = 1;

	return ret;
}

/**
 * Wether cmd is vaild
 * @param	cmd The cmd value.
 * @return	1 - valid cmd, 0 - invalid cmd.
 */
static int is_valid_cmd(unsigned long long cmd)
{
	int ret = 0;
	// If cmd is 0, maybe it will first boot.
	if (cmd == WRITING_STARTED || cmd == WRITING_FINISHED)
		ret = 1;

	dprintf(SPEW, "%s %s cmd: 0x%llx\n", TAG_SETTINGS, __func__, cmd);

	return ret;
}

/**
 * Read IE settings from settings partition.
 * @param	info Pointer to receive the result.
 * @return	1 - success, 0 - read fail.
 */
int read_ie_setting(struct ie_setting_info *info)
{
	int ret = 0;
	unsigned long long ptn = 0;
	unsigned long long data_addr = OFFSET_IE_SETTINGS;
	const unsigned int page_size = flash_page_size();
	const unsigned int size = sizeof(struct ie_setting_info);
	unsigned char data[page_size];
	unsigned long long cmd = 0;
#if defined(FORCE_RESTORE_CMD)
	unsigned long long temp = WRITING_FINISHED;
#endif

	dprintf(SPEW, "%s %s\n", TAG_SETTINGS, __func__);

	if (info == NULL)
	{
		dprintf(CRITICAL, "%s %s invalid arg\n", TAG_SETTINGS, __func__);
		return ret;
	}

	ptn = emmc_ptn_offset((unsigned char*)SETTINGS_PARTITION);
	if (ptn == 0)
	{
		dprintf(CRITICAL, "%s %s Not found settings partition\n", TAG_SETTINGS, __func__);
		return ret;
	}

#if defined(FORCE_RESTORE_CMD)
	ret = set_cmd(NONE, &temp);
	dprintf(INFO, "%s %s set_cmd ret:%d\n", TAG_SETTINGS, __func__, ret);
#endif

	ret = get_cmd(NONE, &cmd);
	dprintf(SPEW, "%s %s get_cmd ret:%d cmd:0x%llx\n", TAG_SETTINGS, __func__, ret, cmd);
	if (ret != 1)
	{
		dprintf(CRITICAL, "%s %s failed to read cmd\n", TAG_SETTINGS, __func__);
		return ret;	//error
	}

	if (!is_valid_cmd(cmd))	//exception case
	{
		// check backup cmd
		cmd = 0;
		ret = get_cmd(BACKUP, &cmd);
		if (ret != 1 || !is_valid_cmd(cmd))
		{
			dprintf(INFO, "%s %s backup cmd(0x%llx) is invalid!\n", TAG_SETTINGS, __func__, cmd);
			return ret; //error
		}

		data_addr = OFFSET_IE_SETTINGS_BACKUP_DATA;	//read backup data
	}

	ret = emmc_read(ptn + BYTE_TO_SECTOR(data_addr), page_size, (unsigned int *)data);
	if (ret == 0)
	{
		memcpy(info, data, size);
		ret = 1;
	}

	return ret;
}

/**
 * Write new IE settings to settings partition. Not used in LK.
 **/
int write_ie_setting(struct ie_setting_info *info)
{
	int ret = 0;
	unsigned long long ptn = 0;
	const unsigned int page_size = flash_page_size();
	const unsigned int size = sizeof(struct ie_setting_info);
	unsigned char data[page_size];

	if (info == NULL)
		return ret;

	ptn = emmc_ptn_offset((unsigned char*)SETTINGS_PARTITION);
	if (ptn == 0)
		return ret;

	memset(data, 0, page_size);
	memcpy(data, info, size);
	ret = emmc_write(ptn, page_size, data);
	if (ret == 0)
		ret = 1;	//success

	return ret;
}

/**
 * Read Engineer mode settings from settings partition.
 * @param	info Pointer to receive the result.
 * @return	1 - success, 0 - read fail.
 */
int read_em_setting(struct em_setting_info *info)
{
	int ret = 0;
	unsigned long long ptn = 0;
	const unsigned int page_size = flash_page_size();
	const unsigned int size = sizeof(struct em_setting_info);
	unsigned char data[page_size];

	ptn = emmc_ptn_offset((unsigned char*)SETTINGS_PARTITION);
	if (ptn == 0)
	{
		dprintf(CRITICAL, "%s %s Not found settings partition\n", TAG_SETTINGS, __func__);
		return ret;
	}

	ret = emmc_read(ptn + BYTE_TO_SECTOR(OFFSET_EM_SETTINGS), page_size, (unsigned int *)data);
	if (ret == 0)
	{
		memcpy(info, data, size);
		ret = 1;
	}

	return ret;
}

/**
 * Print Image Enhancement settings.
 * @param	info Pointer to print the result.
 */
void print_ie_settings(struct ie_setting_info *info)
{
	int i;
	unsigned int temp;
	const int size = sizeof(struct ie_setting_info) / sizeof(int);

	if (info == NULL)
	{
		LKPRINT("%s IE Settings is NULL\n", __func__);
		return;
	}

	for (i = 0; i < size; i++)
	{
		temp = ((unsigned int *)info)[i];
		if (temp)
			LKPRINT("%s %s: %d\n", __func__, ie_setting_field_name[i], temp);
	}
}

/**
 * Print ENGINEER MODE settings.
 * @param	info Pointer to print the result.
 */
void print_em_settings(struct em_setting_info *info)
{
	if (info == NULL)
	{
		LKPRINT("%s EM Settings is NULL\n", __func__);
		return;
	}

	LKPRINT("%s id: %lld\n", __func__, info->id);
	LKPRINT("%s mode: %d\n", __func__, info->mode);
}
