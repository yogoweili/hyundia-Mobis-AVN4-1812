/****************************************************************************
One line to give the program's name and a brief idea of what it does.
Copyright (C) 2013 Telechips Inc.

This program is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place,
Suite 330, Boston, MA 02111-1307 USA
****************************************************************************/
 
#include <asm/io.h>
#include <asm/scatterlist.h>
#include <asm/mach-types.h>

#include <generated/autoconf.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/sched.h>
#include <linux/irq.h>
#include <linux/gpio.h>

#include <mach/irqs.h>
#include <mach/hardware.h>
#include <mach/bsp.h>
#include <mach/reg_physical.h>
#include <mach/memory.h>

#include <mach/vioc_rdma.h>
#include <mach/vioc_ireq.h>
#include <mach/vioc_config.h>
#include <mach/vioc_wdma.h>
#include <mach/vioc_wmix.h>
#include <mach/vioc_scaler.h>
#include <mach/vioc_vin.h>
#include <mach/vioc_viqe.h>
#include <mach/vioc_global.h>

#include <plat/pmap.h>

#include <mach/tcc_cmmb_ioctl.h>
#include "tcc_cmmb_video_interface.h"

static int debug = 1;
#define dprintk(msg...) if (debug) { printk("TCC_CMMB_INTERFACE:   " msg); }

static pmap_t pmap_base_addr;
#define PMAP_DEINTERLACE_BASE_ADDR 		(pmap_base_addr.base+pmap_base_addr.size/2)

#define CMMB_MAX_BUFFER 				14
#define VIOC_DEINT_VIN_PATH_00 		10
#define VIOC_DEINT_VIN_PATH_01 		12

//#define tcc_p2v(x) 			(x)

static DDICONFIG* pDDIConfig;
static VIOC_VIN* pVINBase;
static VIQE* pVIQEBase;
static VIOC_SC* pSCBase;
static VIOC_WMIX* pWMIXBase;
static VIOC_WDMA* pWDMABase;
static VIOC_IREQ_CONFIG* pIREQConfig;

static int total_buf_idx;
static int curr_buf_idx;
static CMMB_VIDEO_BUFFER cmmb_video_buf[CMMB_MAX_BUFFER];

struct list_head cmmb_video_list;
struct list_head cmmb_video_done_list;

static int use_to_simple_deint = 0;
static unsigned int frame_count;
static unsigned int previous_priority = 0;


void tcc_cmmb_video_init_vioc_path(void)
{
	pmap_get_info("jpeg_raw", &pmap_base_addr);
	pDDIConfig 	= (DDICONFIG *)tcc_p2v(HwDDI_CONFIG_BASE);
	pVINBase	= (VIOC_VIN *)tcc_p2v(HwVIOC_VIN10);
	pVIQEBase 	= (VIQE *)tcc_p2v(HwVIOC_VIQE0);
	pSCBase 	= (VIOC_SC *)tcc_p2v(HwVIOC_SC2);
	pWMIXBase 	= (VIOC_WMIX *)tcc_p2v(HwVIOC_WMIX6);
	pWDMABase 	= (VIOC_WDMA *)tcc_p2v(HwVIOC_WDMA07);
	pIREQConfig 	= (VIOC_IREQ_CONFIG *)tcc_p2v(HwVIOC_IREQ);

	dprintk("%s() pVINBase = 0x%x. \n", __func__, (unsigned int)pVINBase);
}

int tcc_cmmb_video_set_gpio(void)
{
	int ret = 0;

	tcc_gpio_config(TCC_GPE(0), GPIO_FN4); 	// data 7
	tcc_gpio_config(TCC_GPE(1), GPIO_FN4); 	// data 6
	tcc_gpio_config(TCC_GPE(2), GPIO_FN4); 	// data 5
	tcc_gpio_config(TCC_GPE(3), GPIO_FN4); 	// data 4
	tcc_gpio_config(TCC_GPE(4), GPIO_FN4); 	// data 3
	tcc_gpio_config(TCC_GPE(5), GPIO_FN4); 	// data 2
	tcc_gpio_config(TCC_GPE(6), GPIO_FN4); 	// data 1
	tcc_gpio_config(TCC_GPE(7), GPIO_FN4); 	// data 0
	tcc_gpio_config(TCC_GPE(10), GPIO_FN4); // PCLK

	return ret;
}

int tcc_cmmb_video_sensor_open(void)
{
	int ret = 0;

	// todo : 

	return ret;
}

int tcc_cmmb_video_sensor_close(void)
{
	int ret = 0;

	// todo : 

	return ret;
}

int tcc_cmmb_video_sensor_mode(int mode)
{
	int ret = 0;

	switch (mode) {
		case CMMB_PRE_INIT:
			// todo : 
			break;

		case CMMB_PRE_START:
			// todo : 
			break;

		case CMMB_PRE_STOP:
			// todo : 
			break;
	}

	return ret;
}

int tcc_cmmb_video_set_vin(VIOC_VIN* pVIN, unsigned int width, unsigned int height)
{
	int ret = 0;
	unsigned int* pLUT = (unsigned int *)tcc_p2v(HwVIOC_VIN11);

	VIOC_VIN_SetSyncPolarity(pVIN, OFF, OFF, OFF, OFF, OFF, OFF);
	VIOC_VIN_SetCtrl(pVIN, ON, OFF, OFF, FMT_YUV422_8BIT, ORDER_RGB);
	VIOC_VIN_SetInterlaceMode(pVIN, ON, OFF);
	VIOC_VIN_SetImageSize(pVIN, width, (height/2));
	VIOC_VIN_SetImageOffset(pVIN, 0, 0, 0);
	VIOC_VIN_SetImageCropSize(pVIN, width, (height/2));
	VIOC_VIN_SetImageCropOffset(pVIN, 0, 0);
	VIOC_VIN_SetY2RMode(pVIN, 2);
	#if defined(FEATURE_CMMB_DISPLAY_TEST)
		VIOC_VIN_SetY2REnable(pVIN, ON);
	#else
		VIOC_VIN_SetY2REnable(pVIN, OFF);
	#endif
	VIOC_VIN_SetLUT(pVIN, pLUT);
	VIOC_VIN_SetLUTEnable(pVIN, OFF, OFF, OFF);
	VIOC_VIN_SetEnable(pVIN, ON);

	return ret;
}

int tcc_cmmb_video_set_viqe(VIQE* pVIQE, unsigned int use_top_size, unsigned int width, unsigned int height)
{
	int ret = 0;
	int interlace_width = width;
	int interlace_height = height/2;
	unsigned int cdf_lut_en 	= OFF; // color LUT (pixel_mapper)
	unsigned int his_en 		= OFF; // histogram
	unsigned int gamut_en 	= OFF; // gamut
	unsigned int d3d_en 		= OFF; // denoise 3d
	unsigned int deintl_en 		= ON;  // deinterlacer
	unsigned int img_fmt 		= FMT_FC_YUV420;
	unsigned int bypass_deintl = ON;
	unsigned int deintl_base0 	= PMAP_DEINTERLACE_BASE_ADDR;
	unsigned int deintl_base1 	= deintl_base0 + (interlace_width * interlace_height * 2 * 2);
	unsigned int deintl_base2 	= deintl_base1 + (interlace_width * interlace_height * 2 * 2);
	unsigned int deintl_base3 	= deintl_base2 + (interlace_width * interlace_height * 2 * 2);

	if (use_top_size == ON) { // size information of VIQE is gotten by VIOC modules.
		interlace_width = interlace_height = 0;
	}

	VIOC_VIQE_SetControlRegister(pVIQE, interlace_width, interlace_height, img_fmt);
	VIOC_VIQE_SetDeintlRegister(pVIQE, img_fmt, OFF, interlace_width, interlace_height, bypass_deintl, \
		deintl_base0, deintl_base1, deintl_base2, deintl_base3);
	VIOC_VIQE_SetControlEnable(pVIQE, cdf_lut_en, his_en, gamut_en, d3d_en, deintl_en);
	VIOC_VIQE_SetDeintlMode(pVIQE, VIOC_VIQE_DEINTL_MODE_2D);

	return ret;
}

int tcc_cmmb_video_set_deint_component(unsigned int deint_simple, unsigned int plug_path)
{
	int ret = 0;
	unsigned int component;

	if (deint_simple) {
		component = VIOC_DEINTLS;
	} else {
		component = VIOC_VIQE;
	}
	VIOC_CONFIG_PlugIn(component, plug_path);

	return ret;
}

int tcc_cmmb_video_set_scaler(VIOC_SC* pSC, unsigned int width, unsigned int height)
{
	int ret = 0;

	VIOC_CONFIG_PlugIn(VIOC_SC0, VIOC_SC_VIN_00);
	VIOC_SC_SetBypass(pSC, OFF);
	VIOC_SC_SetDstSize(pSC, width, height);
	VIOC_SC_SetOutPosition(pSC, 0, 0);
	VIOC_SC_SetOutSize(pSC, width, height);
	VIOC_SC_SetUpdate(pSC); 

	return ret;
}

int tcc_cmmb_video_set_wmix(VIOC_WMIX* pWMIX, unsigned int mix, unsigned int width, unsigned int height)
{
	int ret = 0;
	unsigned int layer  = 0x0;
	unsigned int key_R = 0xFF, key_G = 0x0, key_B = 0xC6;
	unsigned int key_mask_R = 0xF8, key_mask_G = 0xFC, key_mask_B = 0xF8;

#if defined(FEATURE_CMMB_DISPLAY_TEST)
	if (0) {
#else
	if (mix) {
#endif
		VIOC_CONFIG_WMIXPath(WMIX60, ON);
		VIOC_WMIX_SetSize(pWMIX, width, height);
		VIOC_WMIX_SetChromaKey(pWMIX, layer, ON, key_R, key_G, key_B, key_mask_R, key_mask_G, key_mask_B);
	} else {
		VIOC_CONFIG_WMIXPath(WMIX60, OFF);
	}
	VIOC_WMIX_SetUpdate(pWMIX);

	return ret;
}

int tcc_cmmb_video_set_wdma(VIOC_WDMA* pWDMA, unsigned int width, unsigned int height, unsigned int fmt)
{
	int ret = 0;	

	#if defined(FEATURE_CMMB_DISPLAY_TEST)
		VIOC_WDMA_SetImageFormat(pWDMA, VIOC_IMG_FMT_RGB888);
		VIOC_WDMA_SetImageSize(pWDMA, width, height);
		VIOC_WDMA_SetImageOffset(pWDMA, VIOC_IMG_FMT_RGB888, width);
		VIOC_WDMA_SetImageBase(pWDMA, (unsigned int)pmap_base_addr.base, (unsigned int)NULL, (unsigned int)NULL);
		VIOC_WDMA_SetImageEnable(pWDMA, ON);
	#else
		VIOC_WDMA_SetImageFormat(pWDMA, fmt);
		VIOC_WDMA_SetImageSize(pWDMA, width, height);
		VIOC_WDMA_SetImageOffset(pWDMA, fmt, width);
		curr_buf_idx = 0;
		VIOC_WDMA_SetImageBase(pWDMA, cmmb_video_buf[curr_buf_idx].p_Y, cmmb_video_buf[curr_buf_idx].p_Cb, cmmb_video_buf[curr_buf_idx].p_Cr);
		VIOC_WDMA_SetImageEnable(pWDMA, ON);
	#endif

	return ret;
}

int tcc_cmmb_video_set_rdma(unsigned int width, unsigned int height, unsigned int fmt)
{
	int ret = 0;
	VIOC_RDMA* pRDMA = (VIOC_RDMA *)tcc_p2v(HwVIOC_RDMA07);

	VIOC_RDMA_SetImageFormat(pRDMA, fmt);
	VIOC_RDMA_SetImageSize(pRDMA, width, height);
	VIOC_RDMA_SetImageOffset(pRDMA, fmt, width);
	VIOC_RDMA_SetImageBase(pRDMA, (unsigned int)pmap_base_addr.base, (unsigned int)NULL, (unsigned int)NULL);
	VIOC_RDMA_SetImageEnable(pRDMA);

	return ret;
}

int tcc_cmmb_video_set_display(unsigned int OnOff)
{
	int ret = 0;
	VIOC_WMIX* pWMIXBase = (VIOC_WMIX *)tcc_p2v(HwVIOC_WMIX1);

	VIOC_WMIX_SetPosition(pWMIXBase, 3, 0, 0);
	if (OnOff == 1) { // display on.
		VIOC_WMIX_GetOverlayPriority(pWMIXBase, &previous_priority);
		VIOC_WMIX_SetOverlayPriority(pWMIXBase, 20);
	} else {
		VIOC_WMIX_SetOverlayPriority(pWMIXBase, previous_priority);
	}
	VIOC_WMIX_SetUpdate(pWMIXBase);

	return ret;
}

int tcc_cmmb_video_init(void)
{
	int ret = 0;

	tcc_cmmb_video_set_gpio();
	tcc_cmmb_video_sensor_open();

	return ret;
}

int tcc_cmmb_video_start(CMMB_PREVIEW_IMAGE_INFO* cmmb_info)
{
	int ret = 0;
#if defined(FEATURE_CMMB_DISPLAY_TEST)
	cmmb_info->cmmb_img_width = 800;
	cmmb_info->cmmb_img_height = 480;
#endif

	dprintk("%s() \n", __func__);
	dprintk("image format = %d, image width = %d, image height = %d. \n", cmmb_info->cmmb_img_fmt, cmmb_info->cmmb_img_width, \
		cmmb_info->cmmb_img_height);
	dprintk("image left = %d, image top = %d, image right = %d, image bottom = %d. \n", cmmb_info->cmmb_img_left, cmmb_info->cmmb_img_top, \
		cmmb_info->cmmb_img_right, cmmb_info->cmmb_img_bottom);

	dprintk("pIREQConfig = 0x%x. \n", (unsigned int)pIREQConfig);

	BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (1<<7), (1<<7)); 		// WDMA7 reset
	BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (1<<15), (1<<15)); 	// WMIX6 reset
	BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (1<<30), (1<<30)); 	// SCALER2 reset
	BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (1<<25), (1<<25)); 	// VIN1 reset
	BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (1<<16), (1<<16)); 	// VIQE0 reset

	BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (1<<16), (0<<16)); 	// VIQE0 reset clear
	BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (1<<25), (0<<25)); 	// VIN1 reset clear
	BITCSET(pIREQConfig->uSOFTRESET.nREG[0], (1<<30), (0<<30)); 	// SCALER2 reset clear
	BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (1<<15), (0<<15)); 	// WMIX6 reset clear
	BITCSET(pIREQConfig->uSOFTRESET.nREG[1], (1<<7), (0<<7)); 		// WDMA7 reset clear

	frame_count = 0;

	// set CIF port configuration
	BITCSET(pDDIConfig->CIFPORT.nREG, 0x00077777, ((4<<16)|(3<<12)|(1<<8)|(2<<4)|(0)));

	// set preview mode of sensor
	tcc_cmmb_video_sensor_mode(CMMB_PRE_START);

	tcc_cmmb_video_set_vin(pVINBase, cmmb_info->cmmb_img_width, cmmb_info->cmmb_img_height);
	#if defined(FEATURE_CMMB_DISPLAY_TEST)
		use_to_simple_deint = 1;
	#endif
	tcc_cmmb_video_set_deint_component(use_to_simple_deint, VIOC_DEINT_VIN_PATH_01);
	if (!use_to_simple_deint) {
		tcc_cmmb_video_set_viqe(pVIQEBase, ON, cmmb_info->cmmb_img_width, cmmb_info->cmmb_img_height);
	}
	tcc_cmmb_video_set_scaler(pSCBase, cmmb_info->cmmb_img_width, cmmb_info->cmmb_img_height);
	tcc_cmmb_video_set_wmix(pWMIXBase, cmmb_info->cmmb_mix_mode, cmmb_info->cmmb_img_width, cmmb_info->cmmb_img_height);
	tcc_cmmb_video_set_wdma(pWDMABase, cmmb_info->cmmb_img_width, cmmb_info->cmmb_img_height, cmmb_info->cmmb_img_fmt);
	#if defined(FEATURE_CMMB_DISPLAY_TEST)
		tcc_cmmb_video_set_rdma(cmmb_info->cmmb_img_width, cmmb_info->cmmb_img_height, VIOC_IMG_FMT_RGB888);
	#endif

	return ret;
}

int tcc_cmmb_video_stop(void)
{
	int ret = 0;
	VIOC_RDMA* 	pRDMABase = (VIOC_RDMA *)tcc_p2v(HwVIOC_RDMA07);

	BITCSET(pWDMABase->uIRQMSK.nREG, 0x1FF, 0x1FF); // disable WDMA interrupt
	BITCSET(pWDMABase->uCTRL.nREG, (1<<28), (0<<28)); // disable WDMA
	BITCSET(pWDMABase->uCTRL.nREG, (1<<16), (1<<16)); // update WDMA
	msleep(1);

	BITCSET(pWMIXBase->uCTRL.nREG, (1<<16), (0<<16)); // disable WMIX
	msleep(1);

	BITCSET(pVINBase->uVIN_CTRL.nREG, 1, 0); // disable VIN
	msleep(1);

	BITCSET(pRDMABase->uIRQMSK.nREG, 0x7F, 0x7F); // disable RDMA interrupt
	BITCSET(pRDMABase->uCTRL.nREG, (1<<28), (0<<28)); // disable RDMA
	BITCSET(pRDMABase->uCTRL.nREG, (1<<16), (1<<16)); // update WDMA
	msleep(1);

	VIOC_CONFIG_PlugOut(VIOC_SC0);
	if (use_to_simple_deint) {
		VIOC_CONFIG_PlugOut(VIOC_DEINTLS);
	} else {
		VIOC_CONFIG_PlugOut(VIOC_VIQE);
	}

	tcc_cmmb_video_sensor_close();

	return ret;
}

irqreturn_t tcc_cmmb_video_irq_handler(int irq, void *client_data)
{
	int index;
	
	if (pWDMABase->uIRQSTS.nREG & VIOC_WDMA_IREQ_EOFF_MASK) {
		if (curr_buf_idx >= total_buf_idx) {
			index = 0;
		} else {
			index++;
		}
		curr_buf_idx = index;
		VIOC_WDMA_SetImageBase(pWDMABase, cmmb_video_buf[index].p_Y, cmmb_video_buf[index].p_Cb, cmmb_video_buf[index].p_Cr);

		if (frame_count == 2) {
			/**********  VERY IMPORTANT                  *************************************/
			// if we want to operate temporal deinterlacer mode, initial 3 field are operated by spatial,
			// then change the temporal mode in next fields.
			VIOC_VIQE_SetDeintlMode((VIQE *)pVIQEBase, VIOC_VIQE_DEINTL_MODE_3D);
		}

		if (frame_count < 3) {
			frame_count++;
		}

		BITCSET(pWDMABase->uCTRL.nREG, (1<<16), (1<<16)); // update WDMA
		BITCSET(pWDMABase->uIRQSTS.nREG, (1<<6), (1<<6)); // clear WDMA EOFF interrupt
	}

	return IRQ_HANDLED;
}

int tcc_cmmb_video_interface_start_stream(CMMB_PREVIEW_IMAGE_INFO* cmmb_info)
{
	int ret = 0;

	ret = tcc_cmmb_video_init();
	if (ret < 0 ) {
		printk("FAILED to cmmb video init. \n");
		return -1;
	}

	ret = tcc_cmmb_video_start(cmmb_info);
	if (ret < 0 ) {
		printk("FAILED to cmmb video start. \n");
		return -1;
	}

	#if defined(FEATURE_CMMB_DISPLAY_TEST)
	ret = tcc_cmmb_video_set_display(1);
	if (ret < 0 ) {
		printk("FAILED to cmmb video preview display. \n");
		return -1;
	}
	#endif

	return ret;
}

int tcc_cmmb_video_interface_stop_stream(void)
{
	int ret = 0;

	#if defined(FEATURE_CMMB_DISPLAY_TEST)
	ret = tcc_cmmb_video_set_display(0);
	if (ret < 0 ) {
		printk("FAILED to cmmb video preview display. \n");
		return -1;
	}
	#endif

	ret = tcc_cmmb_video_stop();
	if (ret < 0 ) {
		printk("FAILED to cmmb video stop. \n");
		return -1;
	}

	return ret;
}

int tcc_cmmb_video_set_buffer_addr(CMMB_IMAGE_BUFFER_INFO* buf_info)
{
	int ret = 0;
	unsigned int stride = 0, y_offset = 0, uv_offset = 0;

	stride = ALIGNED_BUFF(buf_info->width, 16);
	y_offset = stride * buf_info->height;
	uv_offset = ALIGNED_BUFF((stride/2), 16) * (buf_info->height/2);

	if (buf_info->buffer_index == 0) {
		INIT_LIST_HEAD(&cmmb_video_list);
		INIT_LIST_HEAD(&cmmb_video_list);
	}

	cmmb_video_buf[buf_info->buffer_index].p_Y = buf_info->buffer_addr;
	cmmb_video_buf[buf_info->buffer_index].p_Cb = cmmb_video_buf[buf_info->buffer_index].p_Y + y_offset;
	cmmb_video_buf[buf_info->buffer_index].p_Cr = cmmb_video_buf[buf_info->buffer_index].p_Cb + uv_offset;

	return ret;
}

int tcc_cmmb_video_set_qbuf(void)
{
	int ret = 0;

	// todo : 

	return ret;
}

int tcc_cmmb_video_get_dqbuf(void)
{
	int ret = 0;

	// todo : 

	return ret;
}
