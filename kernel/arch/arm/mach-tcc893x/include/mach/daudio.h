#ifndef __DAUDIO___
#define __DAUDIO___

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define DAUDIO_PATH_LEN (512)

#define TW8836_SENSOR_I2C_SLAVE_ID	0x8A
#define TW9912_SENSOR_I2C_SLAVE_ID	0x88

#if defined(CONFIG_HIBERNATION) && defined(CONFIG_SNAPSHOT_BOOT)	//if quickboot mode
#define SETTINGS_BLOCK				"/dev/block/mmcblk0p9"
#endif
//#define FEATURE_SUPPORT_ATV_PROGRESSIVE
//#define FEATURE_SUPPORT_CMMB

#define DAUDIO_IE_SETTINGS_ID		100
#define DAUDIO_IE_SETTINGS_VERSION	140929

#define SUCCESS						1
#define SUCCESS_I2C					2
#define SUCCESS_WRITE_SETTING		3

#define FAIL						300
#define FAIL_I2C					301
#define FAIL_READ_SETTING			302
#define FAIL_WRITE_SETTING			303
#define FAIL_NO_SETTING				304
#define FAIL_UNMATCH_SETTING		305
#define FAIL_OVERFLOW_LIMIT_ARG		306
#define FAIL_NO_COMMNAD				307


typedef enum _daudio_ie_cmds
{
	SET_BRIGHTNESS				= 10,
	SET_CONTRAST				,
	SET_HUE						,
	SET_SATURATION				,
	SET_CAM_BRIGHTNESS			,  /*cam */ /*14*/
	SET_CAM_CONTRAST			,
	SET_CAM_HUE					,
	
/*	SET_CAM_SHARPNESS			,
	SET_CAM_U_GAIN				,
	SET_CAM_V_GAIN				,
*/	
	SET_CAM_SATURATION			,
	
	SET_AUX_BRIGHTNESS			,  /*AUX */ /*18*/
	SET_AUX_CONTRAST			,
	SET_AUX_GAMMA				,
	SET_AUX_SATURATION			,

	SET_TCC_BRIGHTNESS			,  /* UI */  /*22*/
	SET_TCC_CONTRAST			,
	SET_TCC_HUE					,
	SET_TCC_SATURATION			,
	SET_TCC_VIDEO_BRIGHTNESS	,  /* car play*/ /*26*/
	SET_TCC_VIDEO_CONTRAST		,
	SET_TCC_VIDEO_GAMMA			,
	SET_TCC_VIDEO_SATURATION	,
	SET_TCC_VIDEO_BRIGHTNESS2	,  /* android audo */ /*30*/
	SET_TCC_VIDEO_CONTRAST2		,
	SET_TCC_VIDEO_GAMMA2		,
	SET_TCC_VIDEO_SATURATION2	,

	SET_TCC_DMB_BRIGHTNESS		,  /* DMB */ /*34*/
	SET_TCC_DMB_CONTRAST		,
	SET_TCC_DMB_GAMMA			,
	SET_TCC_DMB_SATURATION		,

	SET_TCC_CMMB_BRIGHTNESS		,  /* CMMB */ /*38*/
	SET_TCC_CMMB_CONTRAST		,
	SET_TCC_CMMB_GAMMA			,
	SET_TCC_CMMB_SATURATION		,

	SET_TCC_USB_BRIGHTNESS		,  /* USB */ /*42*/
	SET_TCC_USB_CONTRAST		,
	SET_TCC_USB_GAMMA			,
	SET_TCC_USB_SATURATION		,

	GET_BRIGHTNESS				= SET_TCC_USB_SATURATION + 10,  /* 55 */
	GET_CONTRAST				,
	GET_HUE						,
	GET_SATURATION				,
	GET_CAM_BRIGHTNESS			,
	GET_CAM_CONTRAST			,  /* 60 */
	
	GET_CAM_HUE					,
/*	GET_CAM_SHARPNESS			,
	GET_CAM_U_GAIN				,
	GET_CAM_V_GAIN				,
*/	
	GET_CAM_SATURATION			,

	GET_AUX_BRIGHTNESS			, 
	GET_AUX_CONTRAST			,
	GET_AUX_GAMMA				,  /* 65 */
	GET_AUX_SATURATION			,  
	
	GET_TCC_BRIGHTNESS			,
	GET_TCC_CONTRAST			,
	GET_TCC_HUE					,  
	GET_TCC_SATURATION			,  /* 70 */
	GET_TCC_VIDEO_BRIGHTNESS	,
	GET_TCC_VIDEO_CONTRAST		,
	GET_TCC_VIDEO_GAMMA			,
	GET_TCC_VIDEO_SATURATION	,  
	GET_TCC_VIDEO_BRIGHTNESS2	,  /* 75 */
	GET_TCC_VIDEO_CONTRAST2		,
	GET_TCC_VIDEO_GAMMA2		,
	GET_TCC_VIDEO_SATURATION2	,

	GET_TCC_DMB_BRIGHTNESS		, 
	GET_TCC_DMB_CONTRAST		, /* 80 */
	GET_TCC_DMB_GAMMA			,
	GET_TCC_DMB_SATURATION		,

	GET_TCC_CMMB_BRIGHTNESS		,  
	GET_TCC_CMMB_CONTRAST		,  
	GET_TCC_CMMB_GAMMA			, /* 85 */
	GET_TCC_CMMB_SATURATION		,

	GET_TCC_USB_BRIGHTNESS		, 
	GET_TCC_USB_CONTRAST		,
	GET_TCC_USB_GAMMA			,  
	GET_TCC_USB_SATURATION		, /* 90 */

} daudio_ie_cmds;

/**
 * For convenience, the 4-byte aligned.
 **/
typedef struct _ie_setting_info_t
{
	unsigned int id;
	unsigned int version;

	unsigned int tw8836_brightness;
	unsigned int tw8836_contrast;
	unsigned int tw8836_hue;
	unsigned int tw8836_saturation;

	unsigned int tw8836_cam_brightness;
	unsigned int tw8836_cam_contrast;
	unsigned int tw8836_cam_hue;
	/*
	unsigned int tw8836_cam_sharpness; //GT start
	unsigned int tw8836_cam_u_gain;
	unsigned int tw8836_cam_v_gain;   */ //GT end
	unsigned int tw8836_cam_saturation;

	unsigned int tcc_aux_brightness;
	unsigned int tcc_aux_contrast;
	unsigned int tcc_aux_hue;
	unsigned int tcc_aux_saturation;

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

	/*GT system*/
	unsigned int tcc_dmb_brightness;
	unsigned int tcc_dmb_contrast;
	unsigned int tcc_dmb_gamma;
	unsigned int tcc_dmb_saturation;

	unsigned int tcc_cmmb_brightness;
	unsigned int tcc_cmmb_contrast;
	unsigned int tcc_cmmb_gamma;
	unsigned int tcc_cmmb_saturation;

	unsigned int tcc_usb_brightness;
	unsigned int tcc_usb_contrast;
	unsigned int tcc_usb_gamma;
	unsigned int tcc_usb_saturation;

	unsigned int temp_1;
	unsigned int temp_2;
	unsigned int temp_3;
	unsigned int temp_4;
} ie_setting_info;

#define DAUDIO_CAMERA_IO_FLAG		0x8000000
#define DAUDIO_CHECK_VIDEO_SIGNAL	DAUDIO_CAMERA_IO_FLAG + 0
#define DAUDIO_DISPLAY_MUTE			DAUDIO_CAMERA_IO_FLAG + 1
#define DAUDIO_DISABLE_BT656		DAUDIO_CAMERA_IO_FLAG + 2
#define DAUDIO_RESET_REARCAMERA		DAUDIO_CAMERA_IO_FLAG + 3

//SIRIUS XM RADIO
#define SIRIUS_PLATFORM_DRIVER_NAME		"sirius_dev"
#define SIRIUS_CLASS_DRIVER_NAME		"sirius_ctl"

#define SIRIUS_RESET_TIME				50	//50ms
#define SIRIUS_ENABLE_TIME				250	//250ms

#if defined(CONFIG_DAUDIO_20140220)
#define SIRIUS_GPIO_SHDN				TCC_GPF(3)
#define SIRIUS_GPIO_RESET				TCC_GPF(5)
#else
#define SIRIUS_GPIO_SHDN				TCC_GPA(4)
#define SIRIUS_GPIO_RESET				TCC_GPA(4)
#endif

//D-Audio pin control
#define PINCTL_DRIVER_NAME				"daudio_pinctl"

typedef struct D_Audio_pinioc_cmddata
{
	int pin_name;	//GPIO PIN NAME
	int pin_value;	//GPIO VALUE - LOW or HIGH
} D_AUDIO_PINIOC_CMDDATA;

typedef struct D_Audio_gpio_list
{
	int gpio_name;
	int gpio_number;
	char *gpio_description;
} D_AUDIO_GPIO_LIST;

//D-Audio pin control PIN_ID DEFINITIONS
#define CTL_SD1_RESET					100
#define CTL_CPU_BOOT_OK					101
#define DET_CDMA_BOOT_OK_OUT			102
#define DET_GPS_ANT_SHORT				103
#define DET_GPS_PPS_INT					104
#define CTL_GPS_RESET_N					105
#define CTL_FM1288_RESET				106
#define CTL_LCD_RESET					107
#define DET_REVERSE_CPU					108
#define DET_SIRIUS_INT					109
#define CTL_USB30_5V_VBUS				110
#define DET_XM_F_ACT					111
#define CTL_TW8836_RESET				112
#define DET_GPS_ANT_OPEN				113
#define DET_I2C_TOUCH_INT				114
#define CTL_TOUCH_RESET					115
#define CTL_XM_SHDN						116
#define CTL_XM_SIRIUS_RESET				117
#define CTL_WL_REG_ON					118
#define CTL_GPS_BOOT_MODE				119
#define CTL_BT_REG_ON					120
#define CTL_USB20_PWR_EN				121
#define CTL_USB_PWR_EN					122
#define CTL_UCS_M1						123
#define CTL_UCS_M2						124
#define DET_UCS_ALERT					125
#define DET_UCS_DEV_DET					126
#define CTL_UCS_EM_EN					127
#define CTL_M170_NAVI_VOICE_ON			128
#define CTL_GPS_ANT_PWR_ON				129
#define CTL_DRM_RESET					130

//4th board
#define DET_UBS_CABLE_CHK				131
#define CTL_USB_CABLE_DIG				132
#define DET_M105_CPU_4_5V				133
#define DET_TOUCH_JIG					134

#define DET_BOARD_REV_2					200
#define DET_BOARD_REV_3					201
#define DET_BOARD_REV_4					202
#define DET_BOARD_REV_5					203

//5th board
#define	DET_PMIC_I2C_IRQ				135
#define	CTL_TW9912_RESET				136
#define	DET_SDCARD_WP					137
#define	CTL_SDCARD_PWR_EN				138
#define	DET_SDCARD_CD					139
#define	DET_SIRIUS_INT_CMMB				140
#define	DET_DMB_ANT_SHORT				141
#define	DET_DMB_ANT_OPEN				142
#define	CTL_DMB_ANT_SHDN				143
#define	CTL_BT_RESET					144
#define	DET_WL_HOST_WAKE_UP				145
#define	DET_M101_MICOM_PB12				146
#define	DET_M101_MICOM_PD12				147
#define	DET_M96_MICOM_PD11				148
#define	CTL_M95_MICOM_PD10				149

#define CTL_I2C_TOUCH_SCL               150
#define CTL_I2C_TOUCH_SDA               151


#define DAUDIO_PINCTL_CTL				_IOW('p', 100, struct D_Audio_pinioc_cmddata)
#define DAUDIO_PINCTL_DET				_IOR('p', 101, struct D_Audio_pinioc_cmddata)

//D-Audio Info
#define ATAG_DAUDIO_LK_VERSION			0x54410011
#define ATAG_DAUDIO_HW_VERSION			0x54410012
#define ATAG_DAUDIO_EM_SETTING			0x54410013
#define ATAG_DAUDIO_QUICKBOOT_INFO		0x54410014
#define ATAG_DAUDIO_BOARD_ADC			0x54410015

#define DAUDIO_BSP_VERSION_DET			_IO('i', 100)
#define DAUDIO_BSP_ADC_DET				_IO('i', 101)

#define QB_SIG_SIZE						20

typedef struct D_AUDIO_BSP_VERSION
{
	int lk_version;
	int kernel_version;
	int hw_version;
	int main_version;
	int bt_version;
	int gps_version;
} D_Audio_bsp_version;

typedef enum daudio_ver_hw
{
	DAUDIO_HW_1ST,          // HW V.01, Default
	DAUDIO_HW_2ND,
	DAUDIO_HW_3RD,
	DAUDIO_HW_4TH,
	DAUDIO_HW_5TH,
	DAUDIO_HW_6TH,
	DAUDIO_HW_7TH,
	DAUDIO_HW_8TH,
	DAUDIO_HW_9TH,
} daudio_ver_hw_t;

typedef enum daudio_board_ver_data
{
	DAUDIO_BOARD_5TH = 2,	// D-Audio US, Platform V.01, Default
	DAUDIO_BOARD_6TH,	// D-Audio+ , Platform V.02, 2015.4.9
	DAUDIO_PLATFORM_3,
	DAUDIO_PLATFORM_4,
	DAUDIO_PLATFORM_5,
	DAUDIO_PLATFORM_6,
	DAUDIO_PLATFORM_7,
	DAUDIO_PLATFORM_8,
	DAUDIO_PLATFORM_9,
} daudio_board_ver_data_t;

typedef enum daudio_ver_bt
{
	DAUDIO_BT_2MODULE,
	DAUDIO_BT_1MODULE,
	DAUDIO_BT_RESERVED,
	DAUDIO_BT_ONLY_CK,
	DAUDIO_BT_ONLY_SMD,
} daudio_ver_bt_t;

typedef enum daudio_ver_gps
{
	DAUDIO_KET_GPS_7,
	DAUDIO_TRIMBLE_8_LG,
	DAUDIO_KET_GPS_8_LG,
	DAUDIO_TRIMBLE_8_AUO,
	DAUDIO_TRIMBLE_7,
	DAUDIO_KET_GPS_8_AUO,
	DAUDIO_GPS_RESERVED,
} daudio_ver_gps_t;

struct quickboot_info {
	char sig[QB_SIG_SIZE];
	unsigned int addr;
};

typedef struct D_AUDIO_BSP_ADC
{
	int hw_adc;
	int main_adc;
	int bt_adc;
	int gps_adc;
} D_Audio_bsp_adc;

//D-Audio RTC
#define DAUDIO_RTC_DAY					1
#define DAUDIO_RTC_MONTH				1
#define DAUDIO_RTC_WEEK					1
#define DAUDIO_RTC_HOUR					9
#define DAUDIO_RTC_MINUTE				30
#define DAUDIO_RTC_SECOND				0
#define DAUDIO_RTC_YEAR					2015

#define PORT_HIGH (1)
#define PORT_LOW (0)

//DAUDIO SOUND SYSTEM
#define CODEC_NONE	 (0)
#define CODEC_FM1288 (1)
#define CODEC_NXP    (2)
#define CODEC_AUDIO0_SPDIF    (3)
#define CODEC_AUDIO1_SPDIF    (4)

#define CODEC_FM1288_NAME "tcc-fm1288"
#define CODEC_FM1288_DAI_NAME "fm1288-hifi"
#define CODEC_NXP_NAME "tcc-nxp"
#define CODEC_NXP_DAI_NAME "nxp-hifi"

#if defined(CONFIG_DAUDIO_20140220)
////////// AUDIO MULTI CHANNEL //////////
	#if defined(CONFIG_SND_SOC_TCC_MULTICHANNEL)
		#define DAUDIO_MULTICHANNEL_USE (CODEC_FM1288)
	#else
		#define DAUDIO_MULTICHANNEL_USE (CODEC_NONE)
	#endif
////////// AUDIO CHANNEL DSP //////////
	#define DAUDIO_AUDIO0_CODEC_NAME 		(CODEC_FM1288_NAME)
	#define DAUDIO_AUDIO0_CODEC_DAI_NAME 	(CODEC_FM1288_DAI_NAME)
	#define DAUDIO_AUDIO1_CODEC_NAME 		(CODEC_NXP_NAME)
	#define DAUDIO_AUDIO1_CODEC_DAI_NAME 	(CODEC_NXP_DAI_NAME)
////////// NAVI VOICE SIGNAL CONTROL //////////
	#define GPIO_NAVI_VOC_ON 				TCC_GPG(4)
	#define DAUDIO_NAVI_VOC_CTRL 			(CODEC_AUDIO0_SPDIF)
#else
	#if defined(CONFIG_SND_SOC_TCC_MULTICHANNEL)
		#define DAUDIO_MULTICHANNEL_USE (CODEC_FM1288)
	#else
		#define DAUDIO_MULTICHANNEL_USE (CODEC_NONE)
	#endif
	#define DAUDIO_AUDIO0_CODEC_NAME 		(CODEC_FM1288_NAME)
	#define DAUDIO_AUDIO0_CODEC_DAI_NAME 	(CODEC_FM1288_DAI_NAME)
	#define DAUDIO_AUDIO1_CODEC_NAME 		(CODEC_NXP_NAME)
	#define DAUDIO_AUDIO1_CODEC_DAI_NAME 	(CODEC_NXP_DAI_NAME)
	#define GPIO_NAVI_VOC_ON 				TCC_GPG(4)
	#define DAUDIO_NAVI_VOC_CTRL 			(CODEC_AUDIO0_SPDIF)
#endif

//DAUDIO TOUCH DRIVER
#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT_336S_AT)

	#define TOUCH_VERSION_BUF_SIZE 128
	#define TOUCH_TEST_BUF_SIZE	   128

	struct atmel_touch_version{
		char ver[TOUCH_VERSION_BUF_SIZE];
		char conf[TOUCH_VERSION_BUF_SIZE];
		char firm[TOUCH_VERSION_BUF_SIZE];
	};

	struct atmel_touch_self_test{
		char references[TOUCH_TEST_BUF_SIZE];
		char pinfault[TOUCH_TEST_BUF_SIZE];
	};

	#define DAUDIO_TOUCH_IOC_MAGIC_V1               'T'
	#define DAUDIO_TOUCHIOC_S_MODE_VERSION		_IOW(DAUDIO_TOUCH_IOC_MAGIC_V1, 0, char[sizeof(struct atmel_touch_version)])
	#define DAUDIO_TOUCHIOC_S_MODE_LOCK			_IO(DAUDIO_TOUCH_IOC_MAGIC_V1, 1)
	#define DAUDIO_TOUCHIOC_S_MODE_UNLOCK		_IO(DAUDIO_TOUCH_IOC_MAGIC_V1, 2)
	#define DAUDIO_TOUCHIOC_DISABLE_AUTO_CAL	_IO(DAUDIO_TOUCH_IOC_MAGIC_V1, 3)
	#define DAUDIO_TOUCHIOC_CHECK_RE_CAL		_IO(DAUDIO_TOUCH_IOC_MAGIC_V1, 4)
	#define DAUDIO_TOUCHIOC_SENSITIVITY			_IOW(DAUDIO_TOUCH_IOC_MAGIC_V1, 5, unsigned int)
	#define DAUDIO_TOUCHIOC_REFERENCES_TEST		_IOW(DAUDIO_TOUCH_IOC_MAGIC_V1, 6, char[sizeof(struct atmel_touch_self_test)])
	#define DAUDIO_TOUCHIOC_PINFAULT_TEST		_IOW(DAUDIO_TOUCH_IOC_MAGIC_V1, 7, char[sizeof(struct atmel_touch_self_test)])
#endif

//DAUDIO CMMB
#define DAUDIO_CMMBIOC_RESET					225
#define DAUDIO_CMMBIOC_RESET_CONTROL			226

#define CMMB_RESET_LOW							0
#define CMMB_RESET_HIGH							1

typedef struct {
	unsigned int value;
} CMMB_RESET_INFO;

#endif // __DAUDIO___
