
#include <common.h>
//#include <asm/arch/globals.h>
//#include <asm/arch/clock.h>
#include <asm/arch/reg_physical.h>
//#include <asm/arch/tcc_used_mem_tcc893x.h>
//#include <asm/gpio.h>
#include <lcd.h>
#include <asm/arch/tcc_chip_rev.h>
#include "tcc_used_mem.h"

DECLARE_GLOBAL_DATA_PTR;

//#define ARRAY_SIZE(x)	(sizeof(x)/sizeof((x)[0]))

#define ATAG_MEM		0x54410002
#define ATAG_REVISION	0x54410007
#define ATAG_CAMERA		0x5441000c
#define ATAG_TCC_PMAP	0x5443436d	/* TCCm */
#ifdef TCC_CHIP_REV
#define ATAG_CHIPREV	0x54410010
#endif//
#define ATAG_DISPLAY	0x54434380

#define KERNEL_MEM_SIZE	TOTAL_SDRAM_SIZE

#define TCC_PMAP_NAME_LEN	16

typedef struct {
	char name[TCC_PMAP_NAME_LEN];
	unsigned base;
	unsigned size;
} pmap_t;

struct atag_tcc_entry
{
	char output;
	char resolution;
	char hdmi_resolution;
	char composite_resolution;
	char component_resolution;
	char hdmi_mode;
};

struct atag_tcc_entry tcc_param = {
	.output	= 0,
	.resolution	= 0,
	.hdmi_resolution = 0,
	.composite_resolution = 0,
	.component_resolution = 0,
};

static pmap_t pmap_table[] = {
	{ "pmem", PMEM_SURF_BASE, PMEM_SURF_SIZE },
	{ "ump_reserved", UMP_RESERVED_BASE, UMP_RESERVED_SIZE },
	{ "v4l2_video0", V4L2_VIDEO0_BASE, V4L2_VIDEO0_SIZE },
	{ "v4l2_video1", V4L2_VIDEO1_BASE, V4L2_VIDEO1_SIZE },
	{ "v4l2_video2", V4L2_VIDEO2_BASE, V4L2_VIDEO2_SIZE },
	{ "v4l2_video3", V4L2_VIDEO3_BASE, V4L2_VIDEO3_SIZE },
	{ "v4l2_video4", V4L2_VIDEO4_BASE, V4L2_VIDEO4_SIZE },
	{ "fb_wmixer", FB_WMIXER_MEM_BASE, FB_WMIXER_MEM_SIZE},
	{ "ram_console", RAM_CONSOLE_BASE, RAM_CONSOLE_SIZE },
	{ "secured_inbuff", SECURED_IN_MEM_BASE, SECURED_IN_MEM_SIZE},
	{ "video_sbackup", VIDEO_SBACKUP_MEM_BASE, VIDEO_SBACKUP_MEM_SIZE},
	{ "overlay", OVERLAY_0_PHY_ADDR, OVERLAY_0_MEM_SIZE },
	{ "overlay1", OVERLAY_1_PHY_ADDR, OVERLAY_1_MEM_SIZE },
	{ "overlay_rot", OVERLAY_ROTATION_PHY_ADDR, OVERLAY_ROTATION_MEM_SIZE },
	{ "video", VIDEO_MEM_BASE, VIDEO_MEM_SIZE },
	{ "video_thumb", VIDEO_THUMB_MEM_BASE, VIDEO_THUMB_MEM_SIZE},
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
	{ "directfb_mem", DIRECTFB_HW_MEM_BASE, DIRECTFB_HW_MEM_SIZE },
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

	*ptr++ = CONFIG_SYS_TEXT_BASE;

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
	*ptr++ = CONFIG_TCC_BOARD_REV;
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

#ifdef CONFIG_LCD
unsigned *target_atag_panel(unsigned *ptr)
{
	vidinfo_t vdi;

	get_panel_info(&vdi);

	*ptr++ = 3;
	*ptr++ = ATAG_TCC_PANEL;
	*ptr++ = vdi.id;
	return ptr;
}
#endif

unsigned* target_atag_display(unsigned* ptr)
{
	if(tcc_param.resolution == 0xFF)
	{
		tcc_param.output = 0x80;
		tcc_param.resolution = 0;
		//tcc_param.hdmi_resolution = 0;
		//tcc_param.composite_resolution = 0;
		//tcc_param.component_resolution = 0;
	}

	*ptr++ = 8;
	*ptr++ = ATAG_DISPLAY;
	*ptr++ = (unsigned)tcc_param.output;
	*ptr++ = (unsigned)tcc_param.resolution;
	*ptr++ = (unsigned)tcc_param.hdmi_resolution;
	*ptr++ = (unsigned)tcc_param.composite_resolution;
	*ptr++ = (unsigned)tcc_param.component_resolution;
	*ptr++ = (unsigned)tcc_param.hdmi_mode;

	return ptr;
}

void setup_pmap_tag(struct tag **in_params)
{
	unsigned *ptr;
	unsigned i;

	(*in_params)->hdr.tag = ATAG_TCC_PMAP;
	(*in_params)->hdr.size = 2 + (sizeof(pmap_table) / sizeof(unsigned));

	ptr = (unsigned *)&(*in_params)->u;
	for (i = 0; i < ARRAY_SIZE(pmap_table); i++) {
		memcpy(ptr, &pmap_table[i], sizeof(pmap_t));
		ptr += sizeof(pmap_t) / sizeof(unsigned);
	}
	*in_params = tag_next (*in_params);
}

void setup_board_tags(struct tag **in_params) 
{
	setup_pmap_tag(in_params);
}

