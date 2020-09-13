#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/syscalls.h>

#include <asm/uaccess.h>
#include <mach/daudio_settings.h>
#include <mach/daudio_eng.h>

static int debug_settings_info = 0;
static int debug_settings_detail =0 ;

#define VPRINTK(fmt, args...) if (debug_settings_info) printk(KERN_CRIT "[cleinsoft settings] " fmt, ##args)
#define DPRINTK(fmt, args...) if (debug_settings_detail) printk(KERN_CRIT "[cleinsoft settings] " fmt, ##args)

const char *ie_setting_field_name[] = {
	"id",
	"version",
	"tw8836_brightness",
	"tw8836_contrast",
	"w8836_hue",
	"tw8836_saturation",
	"tw8836_cam_brightness",
	"tw8836_cam_contrast",
	"tw8836_cam_hue",
	/*"tw8836_cam_sharpness", //GT start
	"tw8836_cam_u_gain",
	"tw8836_cam_v_gain",   //GT end*/
	"tw8836_cam_saturation",

	"tcc_aux_brightness",
	"tcc_aux_contrast",
	"tcc_aux_hue",
	"tcc_aux_saturation",

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
	/*GT system*/
	"tcc_dmb_brightness",
	"tcc_dmb_contrast",
	"tcc_dmb_gamma",
	"tcc_dmb_saturation",

	"tcc_cmmb_brightness",
	"tcc_cmmb_contrast",
	"tcc_cmmb_gamma",
	"tcc_cmmb_saturation",

	"tcc_usb_brightness",
	"tcc_usb_contrast",
	"tcc_usb_gamma",
	"tcc_usb_saturation",

	"temp_1",
	"temp_2",
	"temp_3",
	"temp_4",
};

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

	return ret;
}

/**
 * Get the cmd.
 * @param	type The cmd type
 * @param	cmd Pointer to receive the result.
 * @return	1 - success, 0 - error.
 */
static int get_cmd(enum CMD_TYPE type, unsigned long long *cmd)
{
	int ret = 0, fd = 0, offset = 0;
	unsigned char data[sizeof(long long)];
	mm_segment_t old_fs;

	switch (type)
	{
		case NONE:
			offset = OFFSET_IE_SETTINGS_CMD;
			break;

		case BACKUP:
			offset = OFFSET_IE_SETTINGS_BACKUP_CMD;
			break;

		default:
			return ret;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fd = sys_open(SETTINGS_BLOCK, O_RDONLY, 0);
	if (fd >= 0)
	{
		sys_lseek(fd, offset, 0);
		ret = sys_read(fd, data, sizeof(long long));
		sys_close(fd);
		if (ret >= 0)
		{
			ret = 1;
			memcpy(cmd, data, sizeof(long long));
		}
	}

	set_fs(old_fs);

	return ret;
}

/**
 * Set the cmd.
 * @param	type The cmd type
 * @param	cmd The new cmd value to set.
 * @return	1 - success, 0 - error.
 */
static int set_cmd(enum CMD_TYPE type, unsigned long long *cmd)
{
	int ret = 0, fd = 0, offset = 0;
	mm_segment_t old_fs;

	if (cmd == NULL)
		return ret;

	switch (type)
	{
		case NONE:
			offset = OFFSET_IE_SETTINGS_CMD;
			break;

		case BACKUP:
			offset = OFFSET_IE_SETTINGS_BACKUP_CMD;
			break;

		default:
			return ret;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fd = sys_open(SETTINGS_BLOCK, O_RDWR, 0);
	if (fd >= 0)
	{
		sys_lseek(fd, offset, 0);
		ret = sys_write(fd, (char *)cmd, sizeof(long long));
		sys_close(fd);
		if (ret >= 0)
		{
			ret = 1;
		}
	}

	set_fs(old_fs);

	return ret;
}

static int read_data(char *filename, char *data, int read_len, unsigned int offset)
{
	int ret = FAIL_READ_SETTING, fd = 0;

	mm_segment_t old_fs = get_fs();
	set_fs(KERNEL_DS);

	fd = sys_open(filename, O_RDONLY, 0);
	if (fd >= 0)
	{
		sys_lseek(fd, offset, SEEK_SET);
		ret = sys_read(fd, data, read_len);
		sys_close(fd);
		if (ret >= 0)
			ret = SUCCESS;
	}

	set_fs(old_fs);

	return ret;
}

static int write_data(char *filename, char *data, int write_len, unsigned int offset)
{
	int ret = FAIL_WRITE_SETTING, fd = 0;

	mm_segment_t old_fs = get_fs();
	set_fs(KERNEL_DS);

	fd = sys_open(filename, O_RDWR, 0);
	if (fd >= 0)
	{
		sys_lseek(fd, offset, SEEK_SET);
		ret = sys_write(fd, data, write_len);
		VPRINTK("%s sys_write ret: %d\n", __func__, ret);
		sys_close(fd);
		if (ret >= 0)
			ret = SUCCESS;
	}

	set_fs(old_fs);

	return ret;
}

int read_ie_settings(ie_setting_info *info)
{
	int ret = FAIL_READ_SETTING;
	unsigned int offset = 0;
	const int size = sizeof(ie_setting_info);
	char *buf;
	unsigned long long cmd;

	if (info == NULL)
	{
		VPRINTK("%s info is NULL!\n", __func__);
		return ret;
	}
	VPRINTK("%s\n", __func__);

	buf = (char *)kmalloc(size, GFP_KERNEL);
	if (buf == NULL)
	{
		VPRINTK("%s failed to kmalloc\n", __func__);
		return ret;
	}

	// 1. Check & set cmd
	ret = get_cmd(NONE, &cmd);
	if (ret == 0)
		goto read_out;

	if (!is_valid_cmd(cmd))
	{
		cmd = 0;
		ret = get_cmd(BACKUP, &cmd);
		if (ret == 0)
			goto read_out;

		if (is_valid_cmd(cmd))
			offset = OFFSET_IE_SETTINGS_BACKUP_DATA;
		else
			goto read_out;
	}
	else
	{
		offset = OFFSET_IE_SETTINGS;
	}

	DPRINTK("%s offset: %d\n", __func__, offset);

	// 2. Read info data
	ret = read_data(SETTINGS_BLOCK, buf, size, offset);
	if (ret == SUCCESS)
		memcpy(info, buf, size);

read_out:
	kfree(buf);

	return ret;
}

int write_ie_settings(ie_setting_info *info)
{
	int ret = FAIL_WRITE_SETTING;
	unsigned long long cmd;
	unsigned int offset = 0;
	const int size = sizeof(ie_setting_info);
	ie_setting_info tinfo;
	char *buf;
	if (info == NULL)
	{
		VPRINTK("%s info is NULL!\n", __func__);
		return ret;
	}

	VPRINTK("%s\n", __func__);

	buf = (char *)kmalloc(size, GFP_KERNEL);
	if (buf == NULL)
	{
		VPRINTK("%s failed to kmalloc\n", __func__);
		return ret;
	}

	// 1. Read current data
	offset = OFFSET_IE_SETTINGS;
	ret = read_data(SETTINGS_BLOCK, buf, size, offset);
	if (ret != SUCCESS) goto write_out;

	memcpy(&tinfo, buf, size);

	// 2. Set Backup cmd
	cmd = WRITING_STARTED;
	ret = set_cmd(BACKUP, &cmd);
	if (ret == 0) goto write_out;

	// 3. Write info data to backup
	offset = OFFSET_IE_SETTINGS_BACKUP_DATA;
	ret = write_data(SETTINGS_BLOCK, (char *)&tinfo, size, offset);
	if (ret == 0) goto write_out;

	// 4. Set finish backup cmd
	cmd = WRITING_FINISHED;
	ret = set_cmd(BACKUP, &cmd);

	// 5. Set start cmd
	cmd = WRITING_STARTED;
	ret = set_cmd(NONE, &cmd);
	if (ret == 0) goto write_out;

	// 6. Write info data
	offset = OFFSET_IE_SETTINGS;
	ret = write_data(SETTINGS_BLOCK, (char *)info, size, offset);
	if (ret == 0) goto write_out;

	// 7. Set finish cmd
	cmd = WRITING_FINISHED;
	ret = set_cmd(NONE, &cmd);

write_out:
	kfree(buf);

	return ret;
}

/**
 * Print Image Enhancement settings.
 * @param	info Pointer to print the result.
 */
void print_ie_info(ie_setting_info *info)
{
	int i;
	unsigned int temp = 0;
	const int size = sizeof(ie_setting_info) / sizeof(int);

	if (info == NULL)
	{
		VPRINTK("%s IE info is NULL\n", __func__);
		return;
	}

	for (i = 0; i < size; i++)
	{
		temp = ((unsigned int *)info)[i];
		if (temp)
			VPRINTK("%s %s: %d\n", __func__, ie_setting_field_name[i], temp);
	}
}

/**
 * Read Engineer mode settings from settings partition.
 * @param	info Pointer to receive the result.
 * @return	1 - success, 0 - read fail.
 */
int read_em_setting(em_setting_info *info)
{
	int ret = FAIL_READ_SETTING;
	const unsigned int size = sizeof(em_setting_info);
	char *buf;

	if (info == NULL)
	{
		VPRINTK("%s info is NULL!\n", __func__);
		return ret;
	}
	DPRINTK("%s\n", __func__);

	buf = (char *)kmalloc(size, GFP_KERNEL);
	if (buf == NULL)
	{
		VPRINTK("%s failed to kmalloc\n", __func__);
		return ret;
	}

	ret = read_data(SETTINGS_BLOCK, buf, size, OFFSET_EM_SETTINGS);

	if (ret == SUCCESS)
	{
		memcpy((char *)info, buf, size);
		if (info->id != EM_SETTING_ID)
			info->id = EM_SETTING_ID;
	}

	DPRINTK("%s id: 0x%llx mode: %d size: %d\n", __func__, info->id, info->mode, size);

	kfree(buf);

	return ret;
}

/**
 * Write Engineer mode settings to settings partition.
 * @param	info Pointer to save the mode.
 * @return	1 - success, 0 - read fail.
 */
int write_em_setting(em_setting_info *info)
{
	int ret = FAIL_READ_SETTING;
	const int size = sizeof(em_setting_info);

	if (info == NULL)
	{
		VPRINTK("%s info is NULL!\n", __func__);
		return ret;
	}

	if (info->id != EM_SETTING_ID)
		info->id = EM_SETTING_ID;

	DPRINTK("%s id: 0x%llx mode: %d size: %d\n", __func__, info->id, info->mode, size);
	ret = write_data(SETTINGS_BLOCK, (char *)info, size, OFFSET_EM_SETTINGS);

	return ret;
}

/**
 * Print ENGINEER MODE settings.
 * @param	info Pointer to print the result.
 */
void print_em_settings(em_setting_info *info)
{
	if (info == NULL)
	{
		VPRINTK("%s EM Settings is NULL\n", __func__);
		return;
	}

	VPRINTK("%s id: 0x%llx\n", __func__, info->id);
	VPRINTK("%s mode: %d\n", __func__, info->mode);
}
