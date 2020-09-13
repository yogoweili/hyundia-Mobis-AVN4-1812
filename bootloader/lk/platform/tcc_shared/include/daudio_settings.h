#ifndef __DAUDIO_SETTINGS__
#define __DAUDIO_SETTINGS__

#define SETTINGS_PARTITION		"settings"

//CMD
#define WRITING_STARTED			0x44455452415453
#define WRITING_FINISHED		0x44454853494E4946

/**
 * SETTING PARTITION STRUCTURE.
 *
 * IE ORIGINAL	DATA				0 ~ 511
 * IE ORIGINAL	CMD					512 ~ 1023
 * IE BACKUP	DATA				1024 ~ 1535
 * IE BACKUP	CMD					1536 ~ 2047
 * ENGINEER MODE					2048 ~ 2559
 **/
#define OFFSET_IE_SETTINGS						0
#define OFFSET_IE_SETTINGS_CMD					OFFSET_IE_SETTINGS + 512
#define OFFSET_IE_SETTINGS_BACKUP_DATA			OFFSET_IE_SETTINGS_CMD + 512
#define OFFSET_IE_SETTINGS_BACKUP_CMD			OFFSET_IE_SETTINGS_BACKUP_DATA + 512
#define OFFSET_EM_SETTINGS						OFFSET_IE_SETTINGS_BACKUP_CMD + 512

#define QB_SIG_SIZE		20

enum CMD_TYPE {
	NONE,
	BACKUP,
};

struct ie_setting_info {
	unsigned int id;
	unsigned int version;

	unsigned int tw8836_brightness;
	unsigned int tw8836_contrast;
	unsigned int tw8836_hue;
	unsigned int tw8836_saturation;

	unsigned int twxxxx_cam_brightness;
	unsigned int twxxxx_cam_contrast;
	unsigned int twxxxx_cam_hue;
	unsigned int twxxxx_cam_saturation;

	unsigned int tcc_brightness;
	unsigned int tcc_contrast;
	unsigned int tcc_hue;
	unsigned int tcc_saturation;

	unsigned int tcc_video_brightness;
	unsigned int tcc_video_contrast;
	unsigned int tcc_video_gamma;
	unsigned int tcc_video_saturation;

	unsigned int tcc_video_brightness2;
	unsigned int tcc_video_contrast2;
	unsigned int tcc_video_gamma2;
	unsigned int tcc_video_saturation2;

	unsigned int temp_1;
	unsigned int temp_2;
	unsigned int temp_3;
	unsigned int temp_4;
};

#define DAUDIO_IMAGE_ENHANCEMENT_ID		0x64

#define EM_SETTING_ID		0x45444F4D5F4D45
#define EM_MODE_ENABLE		0x1
#define EM_MODE_DISABLE		0x0

struct em_setting_info {
	unsigned long long id;
	unsigned int mode;
	int temp;
};

struct quickboot_info {
	char sig[QB_SIG_SIZE];
	unsigned int addr;
};

struct board_adc_info {
	int hw_adc;
	int main_adc;
	int bt_adc;
	int gps_adc;
};

int read_ie_setting(struct ie_setting_info *info);
int write_ie_setting(struct ie_setting_info *info);
void print_ie_settings(struct ie_setting_info *info);

int read_em_setting(struct em_setting_info *info);
void print_em_settings(struct em_setting_info *info);
#endif
