#include <stdlib.h>

#include <daudio_ie.h>
#include <tw8836.h>
#include <tw9912.h>

#include <vioc/vioc_disp.h>

extern int parse_value_to_tw8836_hue(int value);

extern void VIOC_LUT_Set_value(VIOC_LUT *pLUT, unsigned int select,  unsigned int *LUT_Value);
extern void VIOC_LUT_Enable (VIOC_LUT *pLUT, unsigned int lut_sel, unsigned int enable);

static void init_disp_color_enhancement(int lcdc_num,
		signed char contrast, signed char brightness, signed char hue)
{
	VIOC_DISP *pDISP = lcdc_num ? (VIOC_DISP *)HwVIOC_DISP1 : (VIOC_DISP *)HwVIOC_DISP0;
	VIOC_DISP_SetColorEnhancement(pDISP, contrast, brightness, hue);
}

/**
 * DCENH (Display Device Color Enhancement Register) 0x72000070 + (0x100 * n)
 *
 * Field   Name       Reset  Description
 * 23-16   HUE        0x0    Hue Calibration Register - 2's complement signed value
 *                           -30 ~ 30 degree
 *                           0x80 for -30 degree
 *                           0x00 for 0 degree for default value
 *                           0x7F for about 30 degree
 *
 * 15-8    BRIGHTNESS 0x0    Brightness Calibration Register - 2's complement signed value
 *                           -128 ~ 128 value
 *                           0x80 for -128 offset
 *                           0x00 for 0 offset
 *                           0x7F for 127 offset
 *
 * 7-0     CONTRAST   0x20   Contrast Calibration Register - 2's complement signed value
 *                           -4 ~ 4
 *                           0x80 for -4.0 value
 *                           0xFF for -1.0 value
 *                           0x20 for 1.0 value
 *                           0x7F for about 4.0 value
 */
void init_tcc_ie(int lcdc_num, struct ie_setting_info *info)
{
	if (info == NULL) return;

	signed char contrast = info->tcc_contrast == 0 ? 0x20 : //0x20 reset value
		(signed char)(info->tcc_contrast - 127);
	signed char brightness = info->tcc_brightness == 0 ? 0 :
		(signed char)(info->tcc_brightness - 127);
	signed char hue = 0x0;  //D-Audio not support YUV

	init_disp_color_enhancement(lcdc_num, contrast, brightness, hue);
}

void init_tw8836_ie(struct ie_setting_info *info)
{
	if (info == NULL) return;

	tw8836_set_ie(SET_BRIGHTNESS,
			(unsigned char)info->tw8836_brightness);
	tw8836_set_ie(SET_CONTRAST,
			(unsigned char)info->tw8836_contrast);
	tw8836_set_ie(SET_HUE,
			(unsigned char)parse_value_to_tw8836_hue(info->tw8836_hue));
	tw8836_set_ie(SET_SATURATION,
			(unsigned char)info->tw8836_saturation);
	tw8836_set_ie(SET_CAM_BRIGHTNESS,
			(unsigned char)info->twxxxx_cam_brightness);
	tw8836_set_ie(SET_CAM_CONTRAST,
			(unsigned char)info->twxxxx_cam_contrast);
	tw8836_set_ie(SET_CAM_HUE,
			(unsigned char)info->twxxxx_cam_hue);
	tw8836_set_ie(SET_CAM_SATURATION,
			(unsigned char)info->twxxxx_cam_saturation);
}

void init_tw9912_ie(struct ie_setting_info *info)
{
	if (info == NULL) return;

	tw9912_set_ie(SET_CAM_BRIGHTNESS,
			(unsigned char)info->twxxxx_cam_brightness);
	tw9912_set_ie(SET_CAM_CONTRAST,
			(unsigned char)info->twxxxx_cam_contrast);
	tw9912_set_ie(SET_CAM_HUE,
			(unsigned char)info->twxxxx_cam_hue);
	tw9912_set_ie(SET_CAM_SATURATION,
			(unsigned char)info->twxxxx_cam_saturation);
}

#if 0   //LUT test
static int init_tcc_lut(int lcdc_num)
{
	int ret = 0;
	unsigned int i = 0;
	lcdc_gamma_params lcdc_gamma;
	VIOC_LUT *pLUT =(VIOC_LUT*)HwVIOC_LUT;
	INFOP("%s\n", __func__);

	for (i = 0; i < LUT_TABLE_SIZE; i++)
	{
		//for test - Reversal effect
		int r = 255 - i, g = 255 - i, b = 255 - i;
		lcdc_gamma.GammaBGR[i] = b << 16 | g << 8 | r;
	}

	lcdc_gamma.lcdc_num  = lcdc_num ? VIOC_LUT_DEV1 : VIOC_LUT_DEV0;
	lcdc_gamma.onoff = 1;   //on

	VIOC_LUT_Set_value(pLUT, lcdc_gamma.lcdc_num,  lcdc_gamma.GammaBGR);
	VIOC_LUT_Enable(pLUT, lcdc_gamma.lcdc_num, lcdc_gamma.onoff);

	return ret;
}
#endif
