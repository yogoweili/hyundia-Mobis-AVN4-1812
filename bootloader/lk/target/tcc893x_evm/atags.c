/*
 * Copyright (C) 2010 Telechips, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <string.h>
#include <arch/tcc_used_mem.h>
#ifdef TCC_CHIP_REV
#include "tcc_chip_rev.h"
#endif//TCC_CHIP_REV
#define ARRAY_SIZE(x)	(sizeof(x)/sizeof((x)[0]))

#define ATAG_MEM		0x54410002
#define ATAG_REVISION	0x54410007
#define ATAG_CAMERA		0x5441000c
#define ATAG_TCC_PMAP	0x5443436d	/* TCCm */
#ifdef TCC_CHIP_REV
#define ATAG_CHIPREV	0x54410010
#endif//

#ifdef ATAG_DAUDIO
#include <stdlib.h>
#include <daudio_settings.h>

#define ATAG_DAUDIO_LK_VERSION		0x54410011
#define ATAG_DAUDIO_BOARD_VERSION	0x54410012
#define ATAG_DAUDIO_EM_SETTING		0x54410013
#define ATAG_DAUDIO_QUICKBOOT_INFO	0x54410014
#define ATAG_DAUDIO_BOARD_ADC		0x54410015

extern char swsusp_header_sig[QB_SIG_SIZE];
extern unsigned int swsusp_header_addr;

extern unsigned char get_daudio_hw_ver(void);
extern unsigned char get_daudio_main_ver(void);
extern unsigned char get_daudio_bt_ver(void);
extern unsigned char get_daudio_gps_ver(void);
extern unsigned char get_daudio_hw_adc(void);
extern unsigned char get_daudio_main_adc(void);
extern unsigned char get_daudio_bt_adc(void);
extern unsigned char get_duadio_gps_adc(void);

extern int read_em_setting(struct em_setting_info *info);
#endif

#define KERNEL_MEM_SIZE	TOTAL_SDRAM_SIZE

#define TCC_PMAP_NAME_LEN	16

typedef struct {
	char name[TCC_PMAP_NAME_LEN];
	unsigned base;
	unsigned size;
} pmap_t;

static pmap_t pmap_table[] = {
	{ "pmem", PMEM_SURF_BASE, PMEM_SURF_SIZE },
	{ "ump_reserved", UMP_RESERVED_BASE, UMP_RESERVED_SIZE },
	{ "fb_wmixer", FB_WMIXER_MEM_BASE, FB_WMIXER_MEM_SIZE},
	{ "ram_console", RAM_CONSOLE_BASE, RAM_CONSOLE_SIZE },
	{ "secured_inbuff", SECURED_IN_MEM_BASE, SECURED_IN_MEM_SIZE},
	{ "overlay", OVERLAY_0_PHY_ADDR, OVERLAY_0_MEM_SIZE },
	{ "overlay1", OVERLAY_1_PHY_ADDR, OVERLAY_1_MEM_SIZE },
	{ "overlay_rot", OVERLAY_ROTATION_PHY_ADDR, OVERLAY_ROTATION_MEM_SIZE },
	{ "video", VIDEO_MEM_BASE, VIDEO_MEM_SIZE },
	{ "viqe", PMEM_VIQE_BASE, PMEM_VIQE_SIZE },
	{ "ext_camera", EXT_CAM_BASE, EXT_CAM_SIZE },
	{ "fb_video", FB_MEM_BASE, FB_MEM_SIZE },
	{ "fb_scale", FB_SCALE_MEM_BASE, FB_SCALE_MEM_TOTAL_SIZE },
	{ "fb_scale0", FB_SCALE_ADDR0, FB_SCALE_SIZE },
	{ "fb_scale1", FB_SCALE_ADDR1, FB_SCALE_SIZE },
	{ "fb_g2d0", FB_G2D_ADDR0, FB_G2D_SIZE },
	{ "fb_g2d1", FB_G2D_ADDR1, FB_G2D_SIZE },
	{ "video_dual", VIDEO_DUAL_DISPLAY_BASE, VIDEO_DUAL_DISPLAY_SIZE },
	{ "jpeg_header", PA_JPEG_HEADER_BASE_ADDR, TCC_JPEG_HEADER_SIZE },
	{ "jpeg_raw", PA_JPEG_RAW_BASE_ADDR, JPEG_RAW_MEM_SIZE },
	{ "jpeg_stream", PA_JPEG_STREAM_BASE_ADDR, JPEG_STREAM_MEM_SIZE },
	{ "nand_mem", NAND_MEM_BASE, NAND_MEM_SIZE },
	{ "jpg_enc_dxb", JPEG_ENC_CAPTURE_BASE, JPEG_ENC_CAPTURE_SIZE},
	{ "jpg_raw_dxb", JPEG_RAW_CAPTURE_BASE, JPEG_RAW_CAPTURE_SIZE},
	{ "video_ext", VIDEO_MEM_EXT_BASE, VIDEO_MEM_EXT_SIZE },
	{ "total", PMEM_SURF_BASE, TOTAL_FIXED_MEM_SIZE },
};

unsigned* target_atag_mem(unsigned* ptr)
{
	unsigned i;

	/* ATAG_MEM */
	*ptr++ = 4;
	*ptr++ = ATAG_MEM;

	//*ptr++ = KERNEL_MEM_SIZE;
	if((unsigned int)(KERNEL_MEM_SIZE) < (unsigned int)(2048*SZ_1M))
		*ptr++ = KERNEL_MEM_SIZE;
	else
		*ptr++ = ((2048*SZ_1M)-4);

	*ptr++ = BASE_ADDR;

	/* ATAG_TCC_PMAP */
	*ptr++ = 2 + (sizeof(pmap_table) / sizeof(unsigned));
	*ptr++ = ATAG_TCC_PMAP;
	for (i = 0; i < ARRAY_SIZE(pmap_table); i++) {
		memcpy(ptr, &pmap_table[i], sizeof(pmap_t));
		ptr += sizeof(pmap_t) / sizeof(unsigned);
	}

	return ptr;
}

unsigned* target_atag_revision(unsigned* ptr)
{
	*ptr++ = 3;
	*ptr++ = ATAG_REVISION;
	*ptr++ = HW_REV;
	return ptr;
}

unsigned* target_atag_is_camera_enable(unsigned* ptr)
{
	*ptr++ = 3;
	*ptr++ = ATAG_CAMERA;	
	*ptr++ = 1;

	return ptr;
}

#ifdef TCC_CHIP_REV
unsigned* target_atag_chip_revision(unsigned* ptr)
{
	*ptr++ = 3;
	*ptr++ = ATAG_CHIPREV;	
	*ptr++ = (unsigned int)tcc_get_chip_revision();

	return ptr;
}
#endif//

#ifdef ATAG_DAUDIO
unsigned* target_atag_daudio_lk_version(unsigned* ptr)
{
	dprintf(SPEW, "%s LK version: %d\n",
			__func__, (unsigned int)BSP_CLEINSOFT_VERSION);

	*ptr++ = 3;
	*ptr++ = ATAG_DAUDIO_LK_VERSION;
	*ptr++ = (unsigned int)BSP_CLEINSOFT_VERSION;

	return ptr;
}

unsigned* target_atag_daudio_board_version(unsigned* ptr)
{
	unsigned char hw_ver = get_daudio_hw_ver();
	unsigned char main_ver = get_daudio_main_ver();
	unsigned char bt_ver = get_daudio_bt_ver();
	unsigned char gps_ver = get_daudio_gps_ver();
	unsigned int version = gps_ver << 24 | hw_ver << 16 | main_ver << 8 | bt_ver;
	dprintf(SPEW, "%s H/W version: %d\n", __func__, version);
	
	*ptr++ = 3;
	*ptr++ = ATAG_DAUDIO_BOARD_VERSION;
	*ptr++ = version;

	return ptr;
}

unsigned* target_atag_daudio_em_setting_info(unsigned* ptr)
{
	struct em_setting_info info;
	const int size = sizeof(struct em_setting_info) / sizeof(unsigned);
	int ret = read_em_setting(&info);

	dprintf(SPEW, "%s Engineer mode id: 0x%llx, mode: 0x%x, size: %d, ret: %d\n",
			__func__, info.id, info.mode, size, ret);

	*ptr++ = 2 + size;
	*ptr++ = ATAG_DAUDIO_EM_SETTING;
	memcpy(ptr, &info, sizeof(struct em_setting_info));
	ptr += size;

	return ptr;
}

unsigned* target_atag_daudio_quickboot_info(unsigned* ptr)
{
	struct quickboot_info info;
	const int size = sizeof(struct quickboot_info) / sizeof(unsigned);

	memset(info.sig, 0, QB_SIG_SIZE);
	memcpy(info.sig, swsusp_header_sig, QB_SIG_SIZE);
	info.addr = swsusp_header_addr;

	*ptr++ = 2 + size;
	*ptr++ = ATAG_DAUDIO_QUICKBOOT_INFO;
	memcpy(ptr, &info, sizeof(struct quickboot_info));
	ptr += size;

	return ptr;
}

unsigned* target_atag_daudio_board_adc(unsigned* ptr)
{
	struct board_adc_info info;
	const int size = sizeof(struct board_adc_info) / sizeof(unsigned);

	info.hw_adc	= get_daudio_hw_adc();
	info.main_adc	= get_daudio_main_adc();
	info.bt_adc	= get_daudio_bt_adc();
	info.gps_adc	= get_daudio_gps_adc();

	dprintf(SPEW, "%s hw adc: %d, platform adc: %d, bt adc: %d, gps adc: %d\n",
			__func__, info.hw_adc, info.main_adc, info.bt_adc, info.gps_adc);

	*ptr++ = 2 + size;
	*ptr++ = ATAG_DAUDIO_BOARD_ADC;
	memcpy(ptr, &info, sizeof(struct board_adc_info));
	ptr += size;

	return ptr;
}
#endif
