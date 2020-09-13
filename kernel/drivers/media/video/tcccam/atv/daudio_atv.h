#ifndef __DAUDIO_ATV_H__
#define __DAUDIO_ATV_H__

#define REG_TERM 0xFF
#define VAL_TERM 0xFF

#define FRAMESKIP_COUNT_FOR_CAPTURE 1
#define SENSOR_FRAMERATE			15

#define FEATURE_TV_STANDARD_NTSC

#define PRV_ZOFFX 0
#define PRV_ZOFFY 0
#define CAP_ZOFFX 0
#define CAP_ZOFFY 0

#define CAM_2XMAX_ZOOM_STEP 25
#define CAM_CAPCHG_WIDTH	720

#define I2C_RETRY_COUNT		3

enum daudio_atvs {
	SENSOR_TW8836 = 0,
	SENSOR_TW9912,
	SENSOR_CMMB,
	SENSOR_MAX,
};


/**
 * vdloss
 * 1 = Video not present. (sync is not detected in number of consecutive line periods specified by MISSCNT register)
 * 0 = Video detected.
 *
 * hlock
 * 1 = Horizontal sync PLL is locked to the incoming video source. 0 = Horizontal sync PLL is not locked.
 *
 * vlock
 * 1 = Vertical logic is locked to the incoming video source. 0 = Vertical logic is not locked.
 *
 * slock
 * 1 = Sub-carrier PLL is locked to the incoming video source. 0 = Sub-carrier PLL is not locked.
 **/
typedef struct {
	int vdloss;
	int hlock;
	int vlock;
	int slock;
	int mono;
} tw_cstatus;

#define VDLOSS	0x80
#define HLOCK	0x40
#define SLOCK	0x20
#define VLOCK	0x08
#define MONO	0x02


/**
-* @author arc@cleinsoft
-* @date 2014/12/19
-* TW9912 input video selection parameter added
-**/

typedef enum {
	TW_YIN0=0,
	TW_YIN1,
	TW_YIN2,
} eTW_YSEL;

typedef enum {
	TW_NTSC=0,
	TW_PAL
} eTW_STANDARD;


typedef struct SENSOR_FUNC_TYPE_DAUDIO_ {
	int (*sensor_open)(eTW_YSEL yin);
	int (*sensor_close)(void);
	int (*PowerDown)(bool bChangeCamera);

	int (*sensor_preview)(void);
	int (*sensor_capture)(void);
	int (*sensor_capturecfg)(int width, int height);
	int (*sensor_reset_video_decoder)(void);
	int (*sensor_init)(void);
	int (*sensor_read_id)(void);
	int (*sensor_check_video_signal)(unsigned long arg);
	int (*sensor_display_mute)(int mute);
	void (*sensor_close_cam)(void);
	int (*sensor_write_ie)(int cmd, unsigned char value);
	int (*sensor_read_ie)(int cmd, char *level);
	void (*sensor_get_preview_size)(int *width, int *heith);
	void (*sensor_get_capture_size)(int *width, int *height);

	int (*Set_Zoom)(int val);
	int (*Set_AF)(int val);
	int (*Set_Effect)(int val);
	int (*Set_Flip)(int val);
	int (*Set_ISO)(int val);
	int (*Set_ME)(int val);
	int (*Set_WB)(int val);
	int (*Set_Bright)(int val);
	int (*Set_Scene)(int val);
	int (*Set_Exposure)(int val);
	int (*Set_FocusMode)(int val);
	int (*Set_Contrast)(int val);
	int (*Set_Saturation)(int val);
	
	int (*Set_UGain)(int val);     //GT system start
	int (*Set_VGain)(int val);
	int (*Set_Sharpness)(int val); //GT system end

	int (*Set_Hue)(int val);
	int (*Get_Video)(void);
	int (*Check_Noise)(void);

	int (*Set_Path)(int val);
	int (*Get_videoFormat)(void);
	int (*Set_writeRegister)(int reg, int val);
	int (*Get_readRegister)(int reg);

	int (*Check_ESD)(int val);
	int (*Check_Luma)(int val);
} SENSOR_FUNC_TYPE_DAUDIO;

struct sensor_reg {
	unsigned char reg;
	unsigned char val;
};

struct capture_size {
	unsigned long width;
	unsigned long height;
};

extern struct capture_size sensor_sizes[];
extern struct sensor_reg *sensor_reg_common[];
extern void sensor_init_fnc(SENSOR_FUNC_TYPE *sensor_func);
extern void tw8836_sensor_init_fnc(SENSOR_FUNC_TYPE_DAUDIO *sensor_func);
extern void tw9912_sensor_init_fnc(SENSOR_FUNC_TYPE_DAUDIO *sensor_func);
extern void daudio_cmmb_sensor_init_fnc(SENSOR_FUNC_TYPE_DAUDIO *sensor_func);

int datv_init(void);
int datv_read_id(void);
int datv_check_video_signal(unsigned long arg);
int datv_display_mute(int mute);
void datv_close_cam(void);
int datv_get_reset_pin(void);

void datv_get_preview_size(int *width, int *height);
void datv_get_capture_size(int *width, int *height);

//D-Audio Image Enhancement
int datv_write_ie(int cmd, unsigned char value);
int datv_read_ie(int cmd, char *level);

#endif
