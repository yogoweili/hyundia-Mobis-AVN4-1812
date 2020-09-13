/*
 * linux/sound/soc/tcc/tcc9200-pcm.c  
 *
 * Based on:    linux/sound/arm/pxa2xx-pcm.c
 * Author:  <linux@telechips.com>
 * Created: Nov 30, 2004
 * Modified:    Nov 25, 2008
 * Description: ALSA PCM interface for the Intel PXA2xx chip
 *
 * Copyright:	MontaVista Software, Inc.
 * Copyright (C) 2008-2009 Telechips 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>

#include <linux/delay.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/bsp.h>
#include <mach/tca_ckc.h>
#include <mach/tca_i2s.h>

#include "tcc-i2s.h"
#include "tcc-pcm.h"

#include "tcc/tca_tcchwcontrol.h"

#define AUDIO0_BLOCK_RESET

#undef alsa_dbg
#if 0
#define alsa_dbg(f, a...)  printk("== alsa-debug PCM CH0 == " f, ##a)
#else
#define alsa_dbg(f, a...)
#endif

#if defined(CONFIG_PM)
static ADMA gADMA;
#endif

/** 
* @author sjpark@cleinsoft
* @date 2014/04/21
* Support spdif 24bit sample bit. 
**/
#if defined(CONFIG_DAUDIO)
	#define CONFIG_SUPPORT_24BIT_SPDIF_RX
	#define CONFIG_SUPPORT_24BIT_SPDIF_TX
#endif

#if defined(CONFIG_SND_SOC_DAUDIO_CLOCK_CONTROL)
#define TAG_ACLK_CTRL "[ACLK CTRL]"
DEFINE_MUTEX(aclk0_ctrl_lock);
#endif

// Planet 20130801 CDIF
int CDIF_En;

tcc_interrupt_info_x tcc_alsa_info;
EXPORT_SYMBOL(tcc_alsa_info);

/**
*   @author sjpark@cleinsoft
*   @date 2014/08/25
*   To remove pop-noise : Telechips guide
*   @date 2014/11/i06
*   update - multi channel support
**/
static int first_init_delay = 0;
static int first_init_delay_ms = 0;

static const struct snd_pcm_hardware tcc_pcm_hardware_play = {
    .info = (SNDRV_PCM_INFO_MMAP
             | SNDRV_PCM_INFO_MMAP_VALID
             | SNDRV_PCM_INFO_INTERLEAVED
             | SNDRV_PCM_INFO_BLOCK_TRANSFER
             | SNDRV_PCM_INFO_PAUSE
             | SNDRV_PCM_INFO_RESUME),

    .formats = (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U16_LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_IEC958_SUBFRAME_LE),	// Planet 20120306
#if defined(CONFIG_SND_SOC_TCC_MULTICHANNEL)
    .rates        = SNDRV_PCM_RATE_8000_192000,
    .rate_min     = 8000,
    .rate_max     = 192000,
    .channels_min = 2,
    .channels_max = 8,
#else
    .rates        = SNDRV_PCM_RATE_8000_96000,
    .rate_min     = 8000,
    .rate_max     = 96000,
    .channels_min = 2,
    .channels_max = 2,
#endif
    .period_bytes_min = 128,
    .period_bytes_max = __play_buf_size,
    .periods_min      = 2,
    .periods_max      = __play_buf_cnt,
    .buffer_bytes_max = __play_buf_cnt * __play_buf_size,
    .fifo_size = 16,  //ignored
};

static const struct snd_pcm_hardware tcc_pcm_hardware_capture = {
    .info = (SNDRV_PCM_INFO_MMAP
             | SNDRV_PCM_INFO_MMAP_VALID
             | SNDRV_PCM_INFO_INTERLEAVED
             | SNDRV_PCM_INFO_BLOCK_TRANSFER
             | SNDRV_PCM_INFO_PAUSE
             | SNDRV_PCM_INFO_RESUME),

    .formats      = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
#if defined(CONFIG_SND_SOC_TCC_MULTICHANNEL)    // MultiChannel_In_20140113
	.rates        = SNDRV_PCM_RATE_8000_192000,
	.rate_min     = 8000,
	.rate_max     = 192000,
	.channels_min = 2, 
	.channels_max = 8, 
#else
    .rates        = SNDRV_PCM_RATE_8000_96000,
    .rate_min     = 8000,
    .rate_max     = 96000,
    .channels_min = 2,
    .channels_max = 2,                                
#endif

    .period_bytes_min = 128,
    .period_bytes_max = __cap_buf_size,
    .periods_min      = 2,
    .periods_max      = __cap_buf_cnt,
    .buffer_bytes_max = __cap_buf_cnt * __cap_buf_size,
    .fifo_size = 16, //ignored
};

static int alsa_get_intr_num(struct snd_pcm_substream *substream)
{
    if (substream) {
#if defined(CONFIG_ARCH_TCC893X)
        return INT_ADMA0;
#endif
    }
    return -1;
}

static void set_dma_outbuffer(unsigned int addr, unsigned int length, unsigned int period)
{
#if defined(CONFIG_ARCH_TCC893X)
    volatile PADMA pADMA = (volatile PADMA)tcc_p2v(BASE_ADDR_ADMA0);
#endif
    unsigned long dma_buffer = 0;
	int bit_count;

    BITCLR(pADMA->ChCtrl, Hw28);

//    dma_buffer = 0xFFFFFF00 / ((length >> 4) << 8);
//    dma_buffer = dma_buffer * ((length >> 4) << 8);

	bit_count = 31;
	while(bit_count > 3)
	{
		if((0x00000001 << bit_count) & length) 
			break;

		bit_count--;
	};

	if(bit_count <= 3)
	{
		printk("Error : not valid length\n");
		return;
	}
	else
	{
		length = 0x00000001<<bit_count;
		alsa_dbg("[%s], original len[%u]\n", __func__, length);
		printk("[%s], original len[%u]\n", __func__, length);
	}
	
    dma_buffer = 0xFFFFFF00 & (~((length<<4)-1));

    pADMA->TxDaParam = dma_buffer | 4;
    // generate interrupt when half of buffer was played
    pADMA->TxDaTCnt = (period >> 0x05) - 1;

    alsa_dbg("[%s], addr[0x%08X], len[%u], period[%u]\n", __func__, addr, length, period);
    alsa_dbg("[%s] HwTxDaParam [0x%X]\n", __func__, (int)(dma_buffer | 4));
    alsa_dbg("[%s] HwTxDaTCnt [%d]\n", __func__, ((period) >> 0x05) - 1);
    pADMA->TxDaSar = addr;

    pADMA->TransCtrl |= (Hw28 | Hw16 | (Hw9|Hw8) | (Hw0 | Hw1));
    pADMA->ChCtrl |= Hw12;
}

static void set_dma_spdif_outbuffer(unsigned int addr, unsigned int length, unsigned int period)
{
#if defined(CONFIG_ARCH_TCC893X)
    volatile PADMA pADMA = (volatile PADMA)tcc_p2v(BASE_ADDR_ADMA0);
    volatile PADMASPDIFTX pADMASPDIFTX = (volatile PADMASPDIFTX)tcc_p2v(BASE_ADDR_SPDIFTX0);
#endif
    unsigned long dma_buffer = 0;
	int bit_count;

    alsa_dbg("%s, addr[0x%08X], len[%u], period[%u]\n", __func__, addr, length, period);
    BITCLR(pADMA->ChCtrl, Hw29);

//    dma_buffer = 0xFFFFFF00 / (((length) >> 4) << 8);
//    dma_buffer = dma_buffer * (((length) >> 4) << 8);

	bit_count = 31;
	while(bit_count > 3)
	{
		if((0x00000001 << bit_count) & length) 
			break;

		bit_count--;
	};

	if(bit_count <= 3)
	{
		printk("Error : not valid length\n");
		return;
	}
	else
	{
		length = 0x00000001<<bit_count;
		alsa_dbg("[%s], original len[%u]\n", __func__, length);
		printk("[%s], original len[%u]\n", __func__, length);
	}
	
    dma_buffer = 0xFFFFFF00 & (~((length<<4)-1));

    pADMA->TxSpParam = dma_buffer | 4;
    pADMA->TxSpTCnt = (period >> 0x05) - 1;

    alsa_dbg("[%s] HwTxDaParam [0x%X]\n", __func__, (int)(dma_buffer | 4));
    alsa_dbg("[%s] HwTxDaTCnt [%d]\n", __func__, ((period) >> 0x05) - 1);

    pADMA->TxSpSar = addr;
    pADMA->TransCtrl |= (Hw28 | Hw17 | (Hw11|Hw10) | (Hw3 | Hw2));

    memset((void *)pADMASPDIFTX->UserData, 0, 24);
    memset((void *)pADMASPDIFTX->ChStatus, 0, 24);
    memset((void *)pADMASPDIFTX->TxBuffer, 0, 128);

/** 
* @author sjpark@cleinsoft
* @date 2014/04/01
* SPDIF TxConfig - Do not set data valid register at initial time. 
**/
#if defined(CONFIG_DAUDIO)
    pADMASPDIFTX->TxConfig |= HwZERO | Hw8;
#else
    pADMASPDIFTX->TxConfig |= HwZERO | Hw8 | Hw1;
#endif
    pADMASPDIFTX->TxBuffer[0] = 0;

    /*
     * Hw2: interrupt output enable
     * Hw1: data being valid
     * Hw0: enable the transmission
     */
/** 
* @author sjpark@cleinsoft
* @date 2014/04/01
* SPDIF TxConfig - Do not set data valid register at initial time. 
**/
#if defined(CONFIG_DAUDIO)
    pADMASPDIFTX->TxConfig |= HwZERO;
#else
    pADMASPDIFTX->TxConfig |= HwZERO | Hw1 | Hw0;
#endif
    pADMASPDIFTX->TxIntStat = 0x1E; /* Clear Interrupt Status */


    alsa_dbg("%s, spdif current addr[0x%08X] \n", __func__, pADMA->TxSpCsar);
    pADMA->ChCtrl |= (Hw13);
}

static void set_dma_inbuffer(unsigned int addr, unsigned int length, unsigned int period)
{
#if defined(CONFIG_ARCH_TCC893X)
    volatile PADMA pADMA = (volatile PADMA)tcc_p2v(BASE_ADDR_ADMA0);
#endif
    unsigned long dma_buffer = 0;
	int bit_count;


    BITCLR(pADMA->ChCtrl, Hw30);

//    dma_buffer = 0xFFFFFF00 / ((length >> 4) << 8);
//    dma_buffer = dma_buffer * ((length >> 4) << 8);

	bit_count = 31;
	while(bit_count > 3)
	{
		if((0x00000001 << bit_count) & length) 
			break;

		bit_count--;
	};

	if(bit_count <= 3)
	{
		printk("Error : not valid length\n");
		return;
	}
	else
	{
		length = 0x00000001<<bit_count;
		alsa_dbg("[%s], original len[%u]\n", __func__, length);
		printk("[%s], original len[%u]\n", __func__, length);
	}
	
    dma_buffer = 0xFFFFFF00 & (~((length<<4)-1));

    pADMA->RxDaParam = dma_buffer | 4;
    pADMA->RxDaTCnt = (period >> 0x05) - 1;

    alsa_dbg("[%s], addr[0x%08X], len[%d]\n", __func__, addr, length);
    alsa_dbg("[%s] HwRxDaParam [0x%X]\n", __func__, (int)dma_buffer | 4);
    alsa_dbg("[%s] HwRxDaTCnt [%d]\n", __func__, ((period) >> 0x04) - 1);

    pADMA->RxDaDar = addr;
    pADMA->TransCtrl |= (Hw29 | Hw18 | (Hw13|Hw12) | (Hw4 | Hw5) );
    pADMA->ChCtrl |= Hw14;
}

// Planet 20130709 S/PDIF_Rx Start
#if defined(CONFIG_SND_SOC_TCC_CDIF)
static void set_dma_spdif_inbuffer(unsigned int addr, unsigned int length, unsigned int period, int mode)	// CDIF_20140113
#else
static void set_dma_spdif_inbuffer(unsigned int addr, unsigned int length, unsigned int period)
#endif
{
#if defined(CONFIG_ARCH_TCC893X)
    volatile PADMA pADMA = (volatile PADMA)tcc_p2v(BASE_ADDR_ADMA0);
    volatile PADMASPDIFRX pADMASPDIFRX = (volatile PADMASPDIFRX)tcc_p2v(BASE_ADDR_SPDIFRX0);
#endif
    unsigned long dma_buffer = 0;
	int bit_count;

	alsa_dbg(" %s \n", __func__);

    alsa_dbg("%s, addr[0x%08X], len[%u], period[%u]\n", __func__, addr, length, period);
    BITCLR(pADMA->ChCtrl, Hw31);	// DMA Channel disable of SPDIF Rx

//    dma_buffer = 0xFFFFFF00 / (((length) >> 4) << 8);
//    dma_buffer = dma_buffer * (((length) >> 4) << 8);

	bit_count = 31;
	while(bit_count > 3)
	{
		if((0x00000001 << bit_count) & length) 
			break;

		bit_count--;
	};

	if(bit_count <= 3)
	{
		printk("Error : not valid length\n");
		return;
	}
	else
	{
		length = 0x00000001<<bit_count;
		alsa_dbg("[%s], original len[%u]\n", __func__, length);
		printk("[%s], original len[%u]\n", __func__, length);
	}

    dma_buffer = 0xFFFFFF00 & (~((length<<4)-1));

    pADMA->RxCdParam = dma_buffer | 4;
    pADMA->RxCdTCnt = (period >> 0x05) - 1;

    alsa_dbg("[%s] HwRxCdParam [0x%X]\n", __func__, (int)(dma_buffer | 4));
    alsa_dbg("[%s] HwRxCdTCnt [%d]\n", __func__, ((period) >> 0x05) - 1);

    pADMA->RxCdDar = addr;

#if defined(CONFIG_SND_SOC_TCC_CDIF)
	if(mode)
		pADMA->TransCtrl |= (Hw29 | Hw19 | (Hw15) | (Hw7|Hw6));		// CDIF_20140113
	else
#endif
		pADMA->TransCtrl |= (Hw29 | Hw19 | (Hw15|Hw14) | (Hw7|Hw6));
	//pADMA->RptCtrl = 0;		//Planet 20130529

    //memset((void *)pADMASPDIFRX->RxBuffer, 0, 8);
 
    /*
     * Hw2: interrupt output enable
     * Hw1: data being valid
     * Hw0: enable the transmission
     */
    pADMASPDIFRX->RxConfig |= HwZERO | Hw1 | Hw2 | Hw3;
    /** 
    * @author sjpark@cleinsoft
    * @date 2015/01/24
    * disable S/PDIF valid check register
    * Sample data stored only when sub frame validity bit is 0.
    **/
    /* | Hw4;*/
    BITCLR(pADMASPDIFRX->RxConfig, Hw4);

    //pADMASPDIFRX->RxIntStat = 0x1E; 				/* Clear Interrupt Status */
	pADMASPDIFRX->RxIntStat = (Hw4|Hw3|Hw2|Hw1);	/* Clear Interrupt Status */
	//pADMASPDIFRX->RxIntMask = (Hw4|Hw3|Hw2|Hw1);
	pADMASPDIFRX->RxPhaseDet = 0x8c0c;

    alsa_dbg("%s, spdif current Destination addr[0x%08X] \n", __func__, pADMA->RxCdCdar);
    alsa_dbg("%s, spdif current Rx Config [0x%08X]\n", pADMASPDIFRX->RxConfig);
    pADMA->ChCtrl |= (Hw15);
	
}
// Planet 20130709 S/PDIF_Rx End

static irqreturn_t tcc_dma_done_handler(int irq, void *dev_id)
{
#if defined(CONFIG_ARCH_TCC893X)
    volatile PADMA pADMA = (volatile PADMA)tcc_p2v(BASE_ADDR_ADMA0);
#endif
    struct snd_pcm_runtime *runtime;
    struct tcc_runtime_data *prtd;
    int dmaInterruptSource = 0;

    unsigned long reg_temp = 0;                 

    if (pADMA->IntStatus & (Hw4 | Hw0)) {
        dmaInterruptSource |= DMA_CH_OUT;
        reg_temp |= Hw4 | Hw0;
    }
    if (pADMA->IntStatus & (Hw6 | Hw2)) {
        dmaInterruptSource |= DMA_CH_IN;
        reg_temp |= Hw6 | Hw2;
    }
    if (pADMA->IntStatus & (Hw5 | Hw1)) {
        dmaInterruptSource |= DMA_CH_SPDIF;
        reg_temp |= Hw5 | Hw1;
    }
	// Planet 20130709 S/PDIF_Rx
	if (pADMA->IntStatus & (Hw7 | Hw3)) {
        dmaInterruptSource |= DMA_CH_SPDIF_IN;
#if defined(CONFIG_SND_SOC_TCC_CDIF)
		dmaInterruptSource |= DMA_CH_CDIF;		// CDIF_20140113
#endif
        reg_temp |= Hw7 | Hw3;
    }

    if (reg_temp) { 
        pADMA->IntStatus |= reg_temp;
    }

    if ((dmaInterruptSource & DMA_CH_SPDIF)
        && (tcc_alsa_info.flag & TCC_RUNNING_SPDIF)) {

        snd_pcm_period_elapsed(tcc_alsa_info.spdif_ptr);

        runtime = tcc_alsa_info.spdif_ptr->runtime;
        prtd = (struct tcc_runtime_data *)runtime->private_data;
    }
	// Planet 20130709 S/PDIF_Rx
	if ((dmaInterruptSource & DMA_CH_SPDIF_IN)
        && (tcc_alsa_info.flag & TCC_RUNNING_SPDIF_CAPTURE)) {
        
        snd_pcm_period_elapsed(tcc_alsa_info.spdif_cap_ptr);

        runtime = tcc_alsa_info.spdif_cap_ptr->runtime;
        prtd = (struct tcc_runtime_data *)runtime->private_data;
    }

#if defined(CONFIG_SND_SOC_TCC_CDIF)
	// CDIF_20140113
	if ((dmaInterruptSource & DMA_CH_CDIF)
			&& (tcc_alsa_info.flag & TCC_RUNNING_CDIF_CAPTURE)) {
		snd_pcm_period_elapsed(tcc_alsa_info.cdif_cap_ptr);

		runtime = tcc_alsa_info.cdif_cap_ptr->runtime;
		prtd = (struct tcc_runtime_data *)runtime->private_data;
	}
#endif

    if ((dmaInterruptSource & DMA_CH_OUT)
        && (tcc_alsa_info.flag & TCC_RUNNING_PLAY)) {
        snd_pcm_period_elapsed(tcc_alsa_info.play_ptr);

        runtime = tcc_alsa_info.play_ptr->runtime;
        prtd = (struct tcc_runtime_data *) runtime->private_data;
    }
    if ((dmaInterruptSource & DMA_CH_IN)
        && (tcc_alsa_info.flag & TCC_RUNNING_CAPTURE)) {
        snd_pcm_period_elapsed(tcc_alsa_info.cap_ptr);

        runtime = tcc_alsa_info.cap_ptr->runtime;
        prtd = (struct tcc_runtime_data *)runtime->private_data;
    }

    return IRQ_HANDLED;
}

static dma_addr_t get_dma_addr_offset(dma_addr_t start_addr, dma_addr_t cur_addr, unsigned int total_size, unsigned int dma_len)
{
    if ((start_addr <= cur_addr) && ((start_addr + total_size) > cur_addr)) {
        unsigned long remainder = cur_addr - start_addr;
        return (remainder - (remainder % dma_len));
    }
    return 0;
}

static int tcc_pcm_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{
    struct snd_pcm_runtime     *runtime = substream->runtime;
    struct tcc_runtime_data    *prtd    = runtime->private_data;
    struct snd_soc_pcm_runtime *rtd     = substream->private_data;
    struct tcc_pcm_dma_params  *dma;

    size_t totsize = params_buffer_bytes(params);
    size_t period = params_period_bytes(params); 
    size_t period_bytes = params_period_bytes(params);
    int chs = params_channels(params);
    int rate = params_rate(params);
    int ret = 0;

#if defined(CONFIG_ARCH_TCC893X)
    volatile PADMA    pADMA     = (volatile PADMA)tcc_p2v(BASE_ADDR_ADMA0);
    volatile PADMADAI pADMA_DAI = (volatile PADMADAI)tcc_p2v(BASE_ADDR_DAI0);
	volatile PADMASPDIFTX pADMA_SPDIFTX = (volatile PADMASPDIFTX)tcc_p2v(BASE_ADDR_SPDIFTX0);
	volatile PADMASPDIFRX pADMA_SPDIFRX = (volatile PADMASPDIFRX)tcc_p2v(BASE_ADDR_SPDIFRX0);	// Planet 20130709 S/PDIF_Rx
#if defined(CONFIG_SND_SOC_TCC_CDIF)
	volatile PADMACDIF    pADMA_CDIF    = (volatile PADMACDIF)tcc_p2v(BASE_ADDR_CDIF0);			// CDIF_20140113
#endif
#endif

    dma = snd_soc_dai_get_dma_data(rtd->cpu_dai, substream);

    /* return if this is a bufferless transfer e.g.
     * codec <--> BT codec or GSM modem -- lg FIXME */
    if (!dma) {
        return -EACCES;
    }

    /* this may get called several times by oss emulation
     * with different params */
    if (prtd->params == NULL) {
        prtd->params = dma;
        alsa_dbg(" prtd->params = dma\n");

        mutex_lock(&(tcc_alsa_info.mutex));
        if (!(tcc_alsa_info.flag & TCC_INTERRUPT_REQUESTED)) {
            ret = request_irq(alsa_get_intr_num(substream),
                              tcc_dma_done_handler,
                              IRQF_SHARED,
                              "alsa-audio",
                              &tcc_alsa_info);
            if (ret < 0) {
                mutex_unlock(&(tcc_alsa_info.mutex));
                return -EBUSY;
            }
            tcc_alsa_info.flag |= TCC_INTERRUPT_REQUESTED;
        }
        mutex_unlock(&(tcc_alsa_info.mutex));

        if (substream->pcm->device == __SPDIF_DEV_NUM__) {
            prtd->dma_ch = (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? DMA_CH_SPDIF : DMA_CH_SPDIF_IN;	// Planet 20130709 S/PDIF_Rx
#if defined(CONFIG_SND_SOC_TCC_CDIF)
		} else if(substream->pcm->device == __CDIF_DEV_NUM__){
			prtd->dma_ch = DMA_CH_CDIF;	// CDIF_20140113
#endif
        } else {
            prtd->dma_ch = (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? DMA_CH_OUT : DMA_CH_IN;
        }
    } else {
        /* should works more? */
    }

	// Planet 20130801 CDIF Start
	if(params_format(params) == SNDRV_PCM_FORMAT_U16) {
		alsa_dbg("~~~!!! tcc_pcm_open() CDIF Enable !!!~~~\n");
		CDIF_En = 1;
	} else {
		alsa_dbg("~~~!!! tcc_pcm_open() CDIF Disable !!!~~~\n");
		CDIF_En = 0;
	}
	// Planet 20130801 CDIF End

    snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);        
    runtime->dma_bytes = totsize;

    prtd->dma_buffer_end = runtime->dma_addr + totsize;
    prtd->dma_buffer = runtime->dma_addr;
    prtd->period_size = period_bytes;
    prtd->nperiod = period;

    alsa_dbg(" totsize=0x%X period=0x%X  period num=%d, rate=%d, chs=%d\n",
             totsize, period_bytes, period, params_rate(params), params_rate(params));

    if (substream->pcm->device == __SPDIF_DEV_NUM__) {
        if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
            set_dma_spdif_outbuffer(prtd->dma_buffer, totsize, period_bytes);
        } else {
#if defined(CONFIG_SND_SOC_TCC_CDIF)
			set_dma_spdif_inbuffer(prtd->dma_buffer, totsize, period_bytes, 0);	
#else
			set_dma_spdif_inbuffer(prtd->dma_buffer, totsize, period_bytes);	// Planet 20130709 S/PDIF_Rx
#endif
		}
#if defined(CONFIG_SND_SOC_TCC_CDIF)
	} else if (substream->pcm->device == __CDIF_DEV_NUM__) {   
		set_dma_spdif_inbuffer(prtd->dma_buffer, totsize, period_bytes, 1);	// CDIF_20140113
#endif
	} else {
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			set_dma_outbuffer(prtd->dma_buffer, totsize, period_bytes);
		} else {
			set_dma_inbuffer(prtd->dma_buffer, totsize, period_bytes);
		}
	}

#if defined(CONFIG_ARCH_TCC893X)
	if (substream->pcm->device == __I2S_DEV_NUM__) {
		BITCLR(pADMA_DAI->MCCR0, Hw31 | Hw30 | Hw29 | Hw28);
		if (chs > 2) {                                  // 5.1ch or 7.1ch
			pADMA_DAI->DAMR |= (Hw29 | Hw28);
			pADMA->RptCtrl  |= (Hw26 | Hw25 | Hw24);
			pADMA->ChCtrl   |= (Hw24 | Hw21 | Hw20);
            first_init_delay_ms = ((TCC_DMA_HALF_BUFF_MULTI * 2 * 1000)+(rate-1))/rate + 1;
		}
		else {                                          // 2 ch
			BITSET(pADMA_DAI->DAMR, (Hw29));
			BITSET(pADMA->RptCtrl, (Hw26 | Hw25 | Hw24));
			BITCLR(pADMA_DAI->DAMR, (Hw28));
			BITCLR(pADMA->ChCtrl,(Hw24 | Hw21 | Hw20));
            first_init_delay_ms = ((TCC_DMA_HALF_BUFF_STEREO * 2 * 1000)+(rate-1))/rate + 1;
		}

/** 
* @author sjpark@cleinsoft
* @date 2014/05/20
* Support PCM & IIS IF switching
**/
#if defined(CONFIG_SND_SOC_DAUDIO_SUPPORT_PCMIF)
		if(params->reserved[SNDRV_PCM_HW_PARAM_EFLAG0] & SNDRV_PCM_MODE_PCMIF){
			alsa_dbg( "================ Audio PCM mode set =============\n");
			pADMA_DAI->DAMR |= Hw25 // DSP Mode : 0 - IIS / 1 - DSP or TDM
					| Hw26 // DSP Word length : 0 - 24bit / 1 - 16 bit
					| Hw3 // Bit clock polarity : 0 - positive / 1 - negative
					| Hw4 | Hw5; // xfs

			BITCLR(pADMA_DAI->DAMR, Hw12); // Sync mode : 0 - IIS DSP TDM 1 - L/R Justified

			pADMA_DAI->MCCR0 |= Hw0 | Hw1 | Hw2 | Hw3 | Hw4 // Frame size
					| Hw10 | Hw11 	// Frame clock divider select 
					| Hw14; 		// Frame interface

			BITCLR(pADMA_DAI->MCCR0, Hw5 | Hw6 | Hw7 | Hw8 // Frame size
						| Hw15// Frame interface
						| Hw25 ); // pcm mode 

			pADMA_DAI->MCCR1 |= Hw0 | Hw1 | Hw2 | Hw3; // VDE
			BITCLR(pADMA_DAI->MCCR1, Hw4| Hw5 | Hw6 | Hw7 | Hw8); // VDE
		}else{
			alsa_dbg( "================ Audio IIS mode set =============\n");
			BITCLR(pADMA_DAI->DAMR, Hw25 // DSP Mode : 0 - IIS / 1 - DSP or TDM
					| Hw12  // Sync mode : 0 - IIS DSP TDM 1 - L/R Justified
					| Hw3 	// Bit clock polarity : 0 - positive / 1 - negative
					| Hw4 | Hw5 // DAI Frame clock divider select (32fs)
					);
			pADMA_DAI->DAMR |= Hw26; // DSP Word length : 0 - 24bit / 1 - 16 bit

			pADMA_DAI->MCCR0 = 0x00;
			pADMA_DAI->MCCR1 = 0x00; 
		}    
#endif
    }
    else
    {
        //BITCLR(pADMA->RptCtrl, (Hw26 | Hw25 | Hw24));	// TCC_MULTIOUTPUT_SUPPORT 20130711
        BITCLR(pADMA->ChCtrl,(Hw24 | Hw21 | Hw20));
    }
#else
    BITCLR(pADMA_DAI->MCCR0, Hw31 | Hw30 | Hw29 | Hw28);
    if (chs > 2) {                                  // 5.1ch or 7.1ch
        pADMA_DAI->DAMR |= (Hw29 | Hw28);
        pADMA->RptCtrl  |= (Hw25 | Hw24);
        pADMA->ChCtrl   |= (Hw24 | Hw21 | Hw20);
        first_init_delay_ms = ((TCC_DMA_HALF_BUFF_MULTI * 2 * 1000)+(rate-1))/rate + 1;
    }
    else {                                          // 2 ch
        BITSET(pADMA_DAI->DAMR, (Hw29));
        BITSET(pADMA->RptCtrl, (Hw25 | Hw24));
        BITCLR(pADMA_DAI->DAMR, (Hw28));
        BITCLR(pADMA->ChCtrl,(Hw24 | Hw21 | Hw20));
        first_init_delay_ms = ((TCC_DMA_HALF_BUFF_STEREO* 2 * 1000)+(rate-1))/rate + 1;
    }
#endif

	if (substream->pcm->device == __SPDIF_DEV_NUM__) {
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {	// Planet 20130709 S/PDIF_Rx
			//Set Audio or Data Format
			printk("%s : runtime->format11 = %d\n", __func__, params_format(params));

			if (params_format(params) == SNDRV_PCM_FORMAT_U16) {
				BITSET(pADMA_SPDIFTX->TxChStat, Hw0);	//Data format
				printk("%s : set SPDIF TX Channel STATUS to Data format \n",__func__);
			}
			else {
				BITCLR(pADMA_SPDIFTX->TxChStat, Hw0);	//Audio format
				printk("%s : set SPDIF TX Channel STATUS to PCM format \n",__func__);
			}
		}
	}


    return ret;
}

static int tcc_pcm_hw_free(struct snd_pcm_substream *substream)
{
    struct tcc_runtime_data *prtd = substream->runtime->private_data;

    alsa_dbg("[%s] \n", __func__);

    if (prtd->dma_ch) {
        snd_pcm_set_runtime_buffer(substream, NULL);
        //추가 필요  ??
    }

    return 0;
}

static int tcc_pcm_prepare(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    alsa_dbg("[%s] \n", __func__);

    //DMA initialize

    /* Disable Noise */
    //memset((void *)(&HwDADO_L0), 0, 32);
    memset(runtime->dma_area, 0x00, runtime->dma_bytes);

    return 0;
}


static void tcc_pcm_delay(int delay)
{
    volatile int loop;

    for(loop = 0;loop < delay;loop++)
        ;
}

static int tcc_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
    //struct tcc_runtime_data *prtd = substream->runtime->private_data;
    //    audio_stream_t *s = &output_stream;        
    int ret = 0;
#if defined(CONFIG_ARCH_TCC893X)
	volatile PADMADAI pDAI  = (volatile PADMADAI)tcc_p2v(BASE_ADDR_DAI0);
	volatile PADMASPDIFTX pADMASPDIFTX = (volatile PADMASPDIFTX)tcc_p2v(BASE_ADDR_SPDIFTX0);
	volatile PADMASPDIFRX pADMASPDIFRX = (volatile PADMASPDIFRX)tcc_p2v(BASE_ADDR_SPDIFRX0);	// Planet 20130709 S/PDIF_Rx
	volatile PADMA    pADMA = (volatile PADMA)tcc_p2v(BASE_ADDR_ADMA0);
	volatile PADMACDIF pADMACDIF = (volatile PADMACDIF)tcc_p2v(BASE_ADDR_CDIF0);	// Planet 20130801 CDIF
#endif

    alsa_dbg("%s() cmd[%d] frame_bits[%d]\n", __func__, cmd, substream->runtime->frame_bits);
    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
    case SNDRV_PCM_TRIGGER_RESUME:        
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        if (substream->pcm->device == __SPDIF_DEV_NUM__) {
			// Planet 20130709 S/PDIF_Rx Start
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
				alsa_dbg("%s() S/PDIF playback start\n", __func__);
	            pADMASPDIFTX->TxConfig |= HwZERO | Hw0 | Hw2 | Hw1;
	            BITSET(pADMA->ChCtrl, Hw1 | Hw29);
/** 
* @author sjpark@cleinsoft
* @date 2014/04/21
* Support spdif 24bit sample bit. 
**/
#if defined(CONFIG_SUPPORT_24BIT_SPDIF_TX)
				if(substream->runtime->format == SNDRV_PCM_FORMAT_S24_LE || 
				   substream->runtime->format == SNDRV_PCM_FORMAT_U24_LE){
					alsa_dbg("%s() S/PDIF 24bit modet\n", __func__);
					BITCLR(pADMA->ChCtrl, Hw13);
					BITSET(pADMASPDIFTX->TxConfig, Hw23);
					BITCLR(pADMASPDIFTX->DMACFG, Hw12 | Hw13);
				}else{
					alsa_dbg("%s() S/PDIF 16bit modet\n", __func__);
					BITSET(pADMA->ChCtrl, Hw13);
					BITCLR(pADMASPDIFTX->TxConfig, Hw23);
					BITSET(pADMASPDIFTX->DMACFG, Hw13);
				}
#endif
			} else {
				alsa_dbg("%s() S/PDIF recording start\n", __func__);
				pADMASPDIFRX->RxConfig |= Hw0;
				BITSET(pADMA->ChCtrl, Hw3 | Hw26 | Hw31);
/** 
* @author sjpark@cleinsoft
* @date 2014/04/21
* Support spdif 24bit sample bit. 
**/
#if defined(CONFIG_SUPPORT_24BIT_SPDIF_RX)
				if(substream->runtime->format == SNDRV_PCM_FORMAT_S24_LE || 
				   substream->runtime->format == SNDRV_PCM_FORMAT_U24_LE){
					alsa_dbg("%s() S/PDIF 24bit modet\n", __func__);
					BITCLR(pADMA->ChCtrl, Hw15);
					BITSET(pADMASPDIFRX->RxConfig, Hw23);
				}else{    
					alsa_dbg("%s() S/PDIF 16bit modet\n", __func__);
					BITSET(pADMA->ChCtrl, Hw15);
					BITCLR(pADMASPDIFRX->RxConfig, Hw23);
				}    
#endif
			}	// Planet 20130709 S/PDIF_Rx End
        }
#if defined(CONFIG_SND_SOC_TCC_CDIF)
		else if (substream->pcm->device == __CDIF_DEV_NUM__) {	// CDIF_20140113
			alsa_dbg("%s() CDIF recording start\n", __func__);
			pADMASPDIFRX->RxConfig |= Hw0;
			pADMACDIF->CICR |= Hw7;
			BITCLR(pADMA->ChCtrl, Hw26);
			BITSET(pADMA->ChCtrl, Hw3 | Hw31);
		}
#endif
		else {
            unsigned long tmp = 0;
#if defined(CONFIG_SND_SOC_DAUDIO_CLOCK_CONTROL)
				/**
				 *   @author sjpark@cleinsoft
				 *   @date 2014/07/10
				 *   Audio Clock Control - Enable DAI master clock  
				 *   @date 2014/08/21
				 *   Audio Clock Control - adjust audio system clock enable timing 
				 **/
				if(mutex_is_locked(&aclk0_ctrl_lock)){
					alsa_dbg(TAG_ACLK_CTRL "Mutex locked !!! wait lock\n");
				}
				mutex_lock(&aclk0_ctrl_lock);
				if(pDAI->DAMR & Hw15){
					alsa_dbg(TAG_ACLK_CTRL "Audio Clock Already Enabled\n");
				}else{
					BITSET(pDAI->DAMR, Hw15);
					alsa_dbg(TAG_ACLK_CTRL "Generating Audio Clock...\n");
				}
				mutex_unlock(&aclk0_ctrl_lock);
#endif
            if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
                alsa_dbg("%s() playback start\n", __func__);
				// Planet 20130801 CDIF Start
				if(CDIF_En == 1) {
					pADMACDIF->CICR |= (HwZERO | Hw7);	// CDIF Enable
					BITSET(pDAI->DAMR, (Hw2 | Hw8));	// CDIF Monitor Mode & CDIF Clock Select
				}
				// Planet 20130801 CDIF End
                //tca_i2s_start(pDAI, pADMASPDIFTX, 0);
                BITSET(pADMA->ChCtrl, Hw0);
                BITSET(pADMA->ChCtrl, Hw28);
                tmp = pDAI->DAVC;
				pDAI->DAVC = 0x10;
				pDAI->DAMR |= Hw14;
				/**
				 *   @author sjpark@cleinsoft
				 *   @date 2014/08/25
				 *   To remove pop-noise : Telechips guide
                 *   @date 2014/11/i06
                 *   update - multi channel support
				 **/
				if(first_init_delay){
					mdelay(1);
				}
				else{
					first_init_delay = 1;
                    mdelay(first_init_delay_ms);
				}
				pDAI->DAVC = tmp;

            } else {
            	alsa_dbg("%s() recording start\n", __func__);
                //tca_i2s_start(pDAI, pADMASPDIFTX, 1);
                BITSET(pADMA->ChCtrl, Hw2);
                BITSET(pADMA->ChCtrl, Hw30);
                pDAI->DAMR |= Hw13;
            }
        }
        break;

    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        if (substream->pcm->device == __SPDIF_DEV_NUM__) {
			// Planet 20130709 S/PDIF_Rx Start
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
				alsa_dbg("%s() S/PDIF playback end\n", __func__);
	            pADMASPDIFTX->TxConfig &= ~(Hw0 | Hw2 | Hw1);
    	        BITCLR(pADMA->ChCtrl, Hw1 | Hw29);
			} else {
				alsa_dbg("%s() S/PDIF recording end\n", __func__);
				pADMASPDIFRX->RxConfig &= ~Hw0;
				BITCLR(pADMA->ChCtrl, Hw3 | Hw26 | Hw31);
			}	// Planet 20130709 S/PDIF_Rx End
        }
#if defined(CONFIG_SND_SOC_TCC_CDIF)
		else if (substream->pcm->device == __CDIF_DEV_NUM__) {
			alsa_dbg("%s() CDIF recording start\n", __func__);
			BITCLR(pADMACDIF->CICR, Hw7);
			BITCLR(pADMA->ChCtrl, Hw3 | Hw26 | Hw31);
		} 
#endif
		else {
            if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
				alsa_dbg("%s() playback end\n", __func__);
				// Planet 20130801 CDIF Start
				if(CDIF_En == 1) {
					pADMACDIF->CICR &= ~Hw7;			// CDIF Disable
					BITCLR(pDAI->DAMR, (Hw2 | Hw8));	// CDIF Monitor Mode & CDIF Clock Select Disable
				}
				// Planet 20130801 CDIF End
                //tca_i2s_stop(pDAI, pADMASPDIFTX, 0);
                pDAI->DAMR &= ~Hw14;
                mdelay(1);
                BITCLR(pADMA->ChCtrl, Hw28);
                BITCLR(pADMA->ChCtrl, Hw0);
#if defined(CONFIG_SND_SOC_DAUDIO_CLOCK_CONTROL)
				/**
				 *   @author sjpark@cleinsoft
				 *   @date 2014/07/10
				 *   Audio Clock Control - Control audio clock by checing the transmit or receive register 
				 *   @date 2014/08/21
				 *   Audio Clock Control - adjust audio system clock enable timing 
				 **/
				if(mutex_is_locked(&aclk0_ctrl_lock)){
					alsa_dbg(TAG_ACLK_CTRL "Mutex locked !!! wait lock\n");
				}
				mutex_lock(&aclk0_ctrl_lock);
				if(pDAI->DAMR & Hw13){
					alsa_dbg(TAG_ACLK_CTRL "Now Capturing...\n");
				}else{
					BITCLR(pDAI->DAMR, Hw15);
					alsa_dbg(TAG_ACLK_CTRL "Stop Audio Clock !\n");
				}
				mutex_unlock(&aclk0_ctrl_lock);
#endif
            } else {
				alsa_dbg("%s() recording end\n", __func__);
                //tca_i2s_stop(pDAI, pADMASPDIFTX, 1);
                pDAI->DAMR &= ~Hw13;
				BITCLR(pADMA->ChCtrl, Hw30);
				BITCLR(pADMA->ChCtrl, Hw20);
#if defined(CONFIG_SND_SOC_DAUDIO_CLOCK_CONTROL)
				/**
				 *   @author sjpark@cleinsoft
				 *   @date 2014/07/10
				 *   Audio Clock Control - Control audio clock by checing the transmit or receive register 
				 *   @date 2014/08/21
				 *   Audio Clock Control - adjust audio system clock enable timing 
				 **/
				if(mutex_is_locked(&aclk0_ctrl_lock)){
					alsa_dbg(TAG_ACLK_CTRL "Mutex locked !!! wait lock\n");
				}
				mutex_lock(&aclk0_ctrl_lock);
				if(pDAI->DAMR & Hw14){
					alsa_dbg(TAG_ACLK_CTRL "Now Playing...\n");
				}else{
					BITCLR(pDAI->DAMR, Hw15);
					alsa_dbg(TAG_ACLK_CTRL "Stop Audio Clock !\n");
				}
				mutex_unlock(&aclk0_ctrl_lock);
#endif
            }
        }
        break;

    default:
        ret = -EINVAL;
    }

    return ret;
}


//buffer point update
//current position   range 0-(buffer_size-1)
static snd_pcm_uframes_t tcc_pcm_pointer(struct snd_pcm_substream *substream)
{
#if defined(CONFIG_ARCH_TCC893X)
    volatile PADMA pADMA = (volatile PADMA)tcc_p2v(BASE_ADDR_ADMA0);
#endif
    struct snd_pcm_runtime *runtime = substream->runtime;
    snd_pcm_uframes_t x;
    dma_addr_t ptr = 0;

    //alsa_dbg(" %s \n", __func__);
#if defined(CONFIG_SND_SOC_TCC_CDIF)
	if (substream->pcm->device == __SPDIF_DEV_NUM__ || substream->pcm->device == __CDIF_DEV_NUM__) {	// CDIF_20140113
#else
    if (substream->pcm->device == __SPDIF_DEV_NUM__) {
#endif
        ptr = (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? pADMA->TxSpCsar : pADMA->RxCdCdar;	// Planet 20130709 S/PDIF_Rx
    } else {
        ptr = (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? pADMA->TxDaCsar : pADMA->RxDaCdar;
    }

    x = bytes_to_frames(runtime, ptr - runtime->dma_addr);

    if (x < runtime->buffer_size) {
        return x;
    }
    return 0;
}

static int tcc_pcm_open(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct tcc_runtime_data *prtd;
    int ret;

#ifdef AUDIO0_BLOCK_RESET
	volatile PIOBUSCFG    pIOBUSCFG    = (volatile PIOBUSCFG)tcc_p2v(BASE_ADDR_IOBUSCFG);		// udioBlock Reset

	if(tcc_alsa_info.flag == NULL) {
		pIOBUSCFG->HRSTEN0.bREG.ADMA0 = 0;
		mdelay(1);
		alsa_dbg("~~~!!! [%s] Audio0_Block_Off!!!\n", __func__);

		pIOBUSCFG->HRSTEN0.bREG.ADMA0 = 1;
		mdelay(1);
		alsa_dbg("~~~!!! [%s] Audio0_Block_On!!!\n", __func__);
	}
#endif


#if defined(CONFIG_SND_SOC_TCC_CDIF)
	alsa_dbg("[%s] open %s device, %s\n", __func__, 
			(substream->pcm->device == __SPDIF_DEV_NUM__) ? "spdif":
			(substream->pcm->device == __CDIF_DEV_NUM__?"cdif":"pcm"), 	// CDIF_20140113
			substream->stream == SNDRV_PCM_STREAM_PLAYBACK ? "output" : "input");
#else
    alsa_dbg("[%s] open %s device, %s\n", __func__, 
										substream->pcm->device == __SPDIF_DEV_NUM__ ? "spdif":"pcm", 
										substream->stream == SNDRV_PCM_STREAM_PLAYBACK ? "output" : "input");
#endif

    mutex_lock(&(tcc_alsa_info.mutex));
    if (substream->pcm->device == __SPDIF_DEV_NUM__) {
        if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
            tcc_alsa_info.flag |= TCC_RUNNING_SPDIF;
            tcc_alsa_info.spdif_ptr = substream;
            snd_soc_set_runtime_hwparams(substream, &tcc_pcm_hardware_play);
            alsa_dbg("[%s] open spdif device\n", __func__);
        } else {
        	// Planet 20130709 S/PDIF_Rx Start
            tcc_alsa_info.flag |= TCC_RUNNING_SPDIF_CAPTURE;
            tcc_alsa_info.spdif_cap_ptr = substream;
            snd_soc_set_runtime_hwparams(substream, &tcc_pcm_hardware_capture);
            alsa_dbg("[%s] open spdif Capture device\n", __func__);
			// Planet 20130709 S/PDIF_Rx End
        }
    }
#if defined(CONFIG_SND_SOC_TCC_CDIF)
	else if (substream->pcm->device == __CDIF_DEV_NUM__) {	// CDIF_20140113
		tcc_alsa_info.flag |= TCC_RUNNING_CDIF_CAPTURE;
		tcc_alsa_info.cdif_cap_ptr = substream;
		snd_soc_set_runtime_hwparams(substream, &tcc_pcm_hardware_capture);
		alsa_dbg("[%s] open cdif Capture device\n", __func__);
	}
#endif
	else {
        if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {  
            tcc_alsa_info.flag |= TCC_RUNNING_PLAY;
            tcc_alsa_info.play_ptr = substream;
            snd_soc_set_runtime_hwparams(substream, &tcc_pcm_hardware_play);
        } else {
            tcc_alsa_info.flag |= TCC_RUNNING_CAPTURE;
            tcc_alsa_info.cap_ptr = substream;
            snd_soc_set_runtime_hwparams(substream, &tcc_pcm_hardware_capture);
        }
    }
    mutex_unlock(&(tcc_alsa_info.mutex));

    prtd = kzalloc(sizeof(struct tcc_runtime_data), GFP_KERNEL);

    if (prtd == NULL) {
        ret = -ENOMEM;
        goto out;
    }
    runtime->private_data = prtd;

    return 0;
    kfree(prtd);
out:
    return ret;
}

static int tcc_pcm_close(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct tcc_runtime_data *prtd = runtime->private_data;
#if defined(CONFIG_ARCH_TCC893X)
    volatile PADMADAI pDAI = (volatile PADMADAI)tcc_p2v(BASE_ADDR_DAI0);
    volatile PADMASPDIFTX pADMASPDIFTX = (volatile PADMASPDIFTX)tcc_p2v(BASE_ADDR_SPDIFTX0);
	volatile PADMASPDIFRX pADMASPDIFRX = (volatile PADMASPDIFRX)tcc_p2v(BASE_ADDR_SPDIFRX0);	// Planet 20130709 S/PDIF_Rx
#if defined(CONFIG_SND_SOC_TCC_CDIF)
	volatile PADMACDIF    pADMA_CDIF    = (volatile PADMACDIF)tcc_p2v(BASE_ADDR_CDIF0);
#endif
#endif

#if defined(CONFIG_SND_SOC_TCC_CDIF)
	alsa_dbg("[%s] close %s device, %s\n", __func__, substream->pcm->device == __SPDIF_DEV_NUM__ ? "spdif":"pcm", 
			(substream->pcm->device == __CDIF_DEV_NUM__?"cdif":"pcm"), 		// CDIF_20140113
			substream->stream == SNDRV_PCM_STREAM_PLAYBACK ? "output" : "input");
#else
    alsa_dbg("[%s] close %s device, %s\n", __func__, substream->pcm->device == __SPDIF_DEV_NUM__ ? "spdif":"pcm", 
			substream->stream == SNDRV_PCM_STREAM_PLAYBACK ? "output" : "input");
#endif

    mutex_lock(&(tcc_alsa_info.mutex));
    if (substream->pcm->device == __SPDIF_DEV_NUM__) {
		// Planet 20130709 S/PDIF_Rx Start
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
	        tcc_alsa_info.flag &= ~TCC_RUNNING_SPDIF;
	        //tca_i2s_stop(pDAI, pADMASPDIFTX, 0);
	        pADMASPDIFTX->TxConfig &= ~Hw0;
       		alsa_dbg("[%s] close spdif device\n", __func__);
		} else {
			tcc_alsa_info.flag &= ~TCC_RUNNING_SPDIF_CAPTURE;
	        //tca_i2s_stop(pDAI, pADMASPDIFRX, 0);
	        pADMASPDIFRX->RxConfig &= ~Hw0;
			 alsa_dbg("[%s] close spdif Capture device\n", __func__);
		}	// Planet 20130709 S/PDIF_Rx End
    } 
#if defined(CONFIG_SND_SOC_TCC_CDIF)
	else if (substream->pcm->device == __CDIF_DEV_NUM__) {	// CDIF_20140113
		tcc_alsa_info.flag &= ~TCC_RUNNING_CDIF_CAPTURE;
		//tca_i2s_stop(pDAI, pADMASPDIFRX, 0);
		pADMA_CDIF->CICR &= ~Hw7;
		alsa_dbg("[%s] close cdif Capture device\n", __func__);
	}
#endif
	else {
        if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
            tcc_alsa_info.flag &= ~TCC_RUNNING_PLAY;
            //tca_i2s_stop(pDAI, pADMASPDIFTX, 0);
            pDAI->DAMR &= ~Hw14;
        } else {
            tcc_alsa_info.flag &= ~TCC_RUNNING_CAPTURE;
            //tca_i2s_stop(pDAI, pADMASPDIFTX, 1);
            pDAI->DAMR &= ~Hw13;
        }
    }
    // dma_free_writecombine(substream->pcm->card->dev, PAGE_SIZE,); 

    if (prtd) {
        kfree(prtd);
    }

    if (tcc_alsa_info.flag & TCC_INTERRUPT_REQUESTED) {
#if defined(CONFIG_SND_SOC_TCC_CDIF)
		if (!(tcc_alsa_info.flag & (TCC_RUNNING_SPDIF | TCC_RUNNING_SPDIF_CAPTURE | TCC_RUNNING_CDIF_CAPTURE | TCC_RUNNING_PLAY | TCC_RUNNING_CAPTURE))) {
#else
        if (!(tcc_alsa_info.flag & (TCC_RUNNING_SPDIF | TCC_RUNNING_SPDIF_CAPTURE | TCC_RUNNING_PLAY | TCC_RUNNING_CAPTURE))) {		// Planet 20130709 S/PDIF_Rx
#endif
            free_irq(alsa_get_intr_num(substream), &tcc_alsa_info);
            tcc_alsa_info.flag &= ~TCC_INTERRUPT_REQUESTED;
        }
    }
    mutex_unlock(&(tcc_alsa_info.mutex));

    return 0;
}

static int tcc_pcm_mmap(struct snd_pcm_substream *substream, struct vm_area_struct *vma)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    return dma_mmap_writecombine(substream->pcm->card->dev, vma,
        runtime->dma_area,
        runtime->dma_addr,
        runtime->dma_bytes);
}

struct snd_pcm_ops tcc_pcm_ops = {
    .open  = tcc_pcm_open,
    .close  = tcc_pcm_close,
    .ioctl  = snd_pcm_lib_ioctl,
    .hw_params = tcc_pcm_hw_params,
    .hw_free = tcc_pcm_hw_free,
    .prepare = tcc_pcm_prepare,
    .trigger = tcc_pcm_trigger,
    .pointer = tcc_pcm_pointer,
    .mmap = tcc_pcm_mmap,
};

static int tcc_pcm_preallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
    struct snd_pcm_substream *substream = pcm->streams[stream].substream;
    struct snd_dma_buffer *buf = &substream->dma_buffer;
    size_t size = 0;

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        size = tcc_pcm_hardware_play.buffer_bytes_max;
    } else {
        size = tcc_pcm_hardware_capture.buffer_bytes_max;
    }

    buf->dev.type = SNDRV_DMA_TYPE_DEV;
    buf->dev.dev = pcm->card->dev;
    buf->private_data = NULL;

    alsa_dbg("tcc_pcm_preallocate_dma_buffer size [%d]\n", size);
    buf->area = dma_alloc_writecombine(pcm->card->dev, size, &buf->addr, GFP_KERNEL);
    if (!buf->area) {
        return -ENOMEM;
    }

    buf->bytes = size;

    return 0;
}


static void tcc_pcm_free_dma_buffers(struct snd_pcm *pcm)
{
    struct snd_pcm_substream *substream;
    struct snd_dma_buffer *buf;
    int stream;

    alsa_dbg(" %s \n", __func__);

    for (stream = 0; stream < 2; stream++) {
        substream = pcm->streams[stream].substream;
        if (!substream) { continue; }

        buf = &substream->dma_buffer;
        if (!buf->area) { continue; }

        dma_free_writecombine(pcm->card->dev, buf->bytes, buf->area, buf->addr);
        buf->area = NULL;
    }
    mutex_init(&(tcc_alsa_info.mutex));
}

static u64 tcc_pcm_dmamask = DMA_BIT_MASK(32);

int tcc_pcm_new(struct snd_soc_pcm_runtime *rtd) 
{
    struct snd_card *card   = rtd->card->snd_card;
    struct snd_soc_dai *dai = rtd->cpu_dai;
    struct snd_pcm *pcm     = rtd->pcm;
    int ret = 0;

    alsa_dbg("[%s] \n", __func__);

    memset(&tcc_alsa_info, 0, sizeof(tcc_interrupt_info_x));
    mutex_init(&(tcc_alsa_info.mutex));

    if (!card->dev->dma_mask) {
        card->dev->dma_mask = &tcc_pcm_dmamask;
    }
    if (!card->dev->coherent_dma_mask) {
        card->dev->coherent_dma_mask = DMA_BIT_MASK(32);
    }
    if (dai->driver->playback.channels_min) {
        ret = tcc_pcm_preallocate_dma_buffer(pcm, SNDRV_PCM_STREAM_PLAYBACK);
        if (ret) { goto out; }
    }

    if (dai->driver->capture.channels_min) {
        ret = tcc_pcm_preallocate_dma_buffer(pcm, SNDRV_PCM_STREAM_CAPTURE);
        if (ret) { goto out; }
    } 
	{
#if defined(CONFIG_ARCH_TCC893X)
        volatile ADMASPDIFTX *pADMASPDIFTX = (volatile ADMASPDIFTX *)tcc_p2v(BASE_ADDR_SPDIFTX0);
#endif

        memset((void *)pADMASPDIFTX->UserData, 0, 24);
        memset((void *)pADMASPDIFTX->ChStatus, 0, 24);
        memset((void *)pADMASPDIFTX->TxBuffer, 0, 128);

        //unsigned int bitSet = 0x0;

        pADMASPDIFTX->TxConfig |= HwZERO
            //16bits 
            |Hw8 //Clock Divider Ratio is 2(384*fs to default)
            //| Hw7 | Hw6 //User Data Enalbe Bits is set reserved
            //| Hw5 | Hw4 //Channel Status Enalbe Bits is set reserved
            //| Hw2 //interrupt output enable
#if defined(CONFIG_DAUDIO)
/** 
* @author sjpark@cleinsoft
* @date 2014/04/01
* SPDIF TxConfig - Do not set data valid register at initial time. 
**/
#else
            | Hw1 //data being valid
#endif
            //| Hw0 //enable the transmission
            ;
        //Set SPDIF Transmit Channel Status
        pADMASPDIFTX->TxChStat |= HwZERO
            //| 0x00000400 // Store Channel Status bit
            //| 0x00000200 // Store User Data bit
            //| 0x00000100 // Store Validity bit
            //44.1kHz
            //| Hw3 //Original/Commercially Pre-recored data
            //| Hw2 //Pre-emphasis is 50/15us
            //| Hw1 //Copy permitted
            //Audio format
            ;		

        //pADMASPDIFTX->TxIntMask = HwZERO
        //	| Hw2//Higher Data buffer is empty
        //	| Hw1//Lower Data buffer is empty
        //	;

        pADMASPDIFTX->DMACFG = Hw7;
        msleep(100);

        pADMASPDIFTX->DMACFG = HwZERO
            //| Hw14	//Swap Sample Enable
            | Hw13 	//Read Address 16bit Mode
            | Hw11	//DMA Request enable for user data buffer
            | Hw10	//DMA Request enable for sample data buffer
            //| Hw8	//Enable address
            //| Hw3	//FIFO Threshold for DMA request
            | Hw1| Hw0	// [3:0] FIFO Threshold for DMA Request
            ;

        /* Initialize Sample Data Buffer */
        //while(pADMASPDIFTX->DMACFG & 0x00300000) pADMASPDIFTX->TxBuffer[0] = 0;
        pADMASPDIFTX->TxBuffer[0] = 0;

        pADMASPDIFTX->TxConfig |= HwZERO
            //| Hw2 //interrupt output enable
#if defined(CONFIG_DAUDIO)
/** 
* @author sjpark@cleinsoft
* @date 2014/04/01
* SPDIF TxConfig - Do not set data valid register at initial time. 
**/
#else
            | Hw1 //data being valid
            | Hw0 //enable the transmission
#endif
            ;

        pADMASPDIFTX->TxIntStat = 0x1E; /* Clear Interrupt Status */
    }

	// Planet 20130709 S/PDIF_Rx Start
	{
#if defined(CONFIG_ARCH_TCC893X)
        volatile ADMASPDIFRX *pADMASPDIFRX = (volatile ADMASPDIFRX *)tcc_p2v(BASE_ADDR_SPDIFRX0);
#endif
    
		//Set SPDIF Receiver Configuration
		pADMASPDIFRX->RxConfig |= HwZERO
/** 
* @author sjpark@cleinsoft
* @date 2014/02/11
* Set spdif rx sample bit as 24.  
* @date 2014/04/21
* spdif 24 bit support register should be set when audio playback triggered - removed Hw23.
* @date 2015/01/24
* disable S/PDIF valid check register
* Sample data stored only when sub frame validity bit is 0.
**/
#if defined(CONFIG_DAUDIO)
#else
			| Hw23 //Store Samples as Bit
			| Hw4 // Sample Data Store		
#endif
			| Hw3 // RxStatus Register Hold Channel
			| Hw2 // Enable Interrupt output
			| Hw1 // Stored Data
			//| Hw0 //enable the transmission
			;

		pADMASPDIFRX->RxIntStat = (Hw4|Hw3|Hw2|Hw1); /* Clear Interrupt Status */
	}
	// Planet 20130709 S/PDIF_Rx End

out:
    return ret;
}


#if defined(CONFIG_PM)
int tcc_pcm_suspend(struct snd_soc_dai *dai)
{
#if defined(CONFIG_ARCH_TCC893X)
    volatile PADMA pADMA = (volatile PADMA)tcc_p2v(BASE_ADDR_ADMA0);
#endif

    alsa_dbg(" %s \n", __func__);
    gADMA.TransCtrl = pADMA->TransCtrl;
    gADMA.RptCtrl   = pADMA->RptCtrl;
    gADMA.ChCtrl    = pADMA->ChCtrl;
    gADMA.GIntReq   = pADMA->GIntReq;
    return 0;
}

int tcc_pcm_resume(struct snd_soc_dai *dai)
{
#if defined(CONFIG_ARCH_TCC893X)
    volatile PADMA pADMA = (volatile PADMA)tcc_p2v(BASE_ADDR_ADMA0);
#endif

    alsa_dbg(" %s \n", __func__);
    pADMA->TransCtrl = gADMA.TransCtrl;
    pADMA->RptCtrl   = gADMA.RptCtrl;
    pADMA->ChCtrl    = gADMA.ChCtrl;
    pADMA->GIntReq   = gADMA.GIntReq;

    return 0;
}
#endif

static struct snd_soc_platform_driver tcc_soc_platform = {
    .ops      = &tcc_pcm_ops,
    .pcm_new  = tcc_pcm_new,
    .pcm_free = tcc_pcm_free_dma_buffers,
#if defined(CONFIG_PM)
    .suspend  = tcc_pcm_suspend,
    .resume   = tcc_pcm_resume,
#endif
};

static __devinit int tcc_pcm_probe(struct platform_device *pdev)
{
    alsa_dbg(" %s \n", __func__);
	return snd_soc_register_platform(&pdev->dev, &tcc_soc_platform);
}

static int __devexit tcc_pcm_remove(struct platform_device *pdev)
{
    alsa_dbg(" %s \n", __func__);
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

static struct platform_driver tcc_pcm_driver = {
	.driver = {
			.name = "tcc-pcm-audio",
			.owner = THIS_MODULE,
	},

	.probe = tcc_pcm_probe,
	.remove = __devexit_p(tcc_pcm_remove),
};

static int __init snd_tcc_platform_init(void)
{
    alsa_dbg(" %s \n", __func__);
	return platform_driver_register(&tcc_pcm_driver);
}
module_init(snd_tcc_platform_init);

static void __exit snd_tcc_platform_exit(void)
{
    alsa_dbg(" %s \n", __func__);
	platform_driver_unregister(&tcc_pcm_driver);
}
module_exit(snd_tcc_platform_exit);





/************************************************************************
 * Dummy function for S/PDIF. This is a codec part.
************************************************************************/
static int tcc_dummy_probe(struct snd_soc_codec *codec) { return 0; }
static int tcc_dummy_remove(struct snd_soc_codec *codec) { return 0; }
static int tcc_dummy_set_bias_level(struct snd_soc_codec *codec, enum snd_soc_bias_level level) { return 0; }

static struct snd_soc_codec_driver soc_codec_dev_tcc_dummy = {
	.probe =	tcc_dummy_probe,
	.remove =	tcc_dummy_remove,
	.set_bias_level = tcc_dummy_set_bias_level,
	.reg_cache_size = 0,
	.reg_word_size = sizeof(u16),
	.reg_cache_default = NULL,
	.dapm_widgets = NULL,
	.num_dapm_widgets = 0,
	.dapm_routes = NULL,
	.num_dapm_routes = 0,
};

static int  iec958_pcm_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *cpu_dai) { return 0; }
static int  iec958_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params, struct snd_soc_dai *cpu_dai) { return 0; }
static void iec958_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *cpu_dai) { }
static int  iec958_mute(struct snd_soc_dai *dai, int mute) { return 0; }
static int  iec958_set_dai_sysclk(struct snd_soc_dai *codec_dai, int clk_id, unsigned int freq, int dir) { return 0; }
static int  iec958_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt) { return 0; }

static struct snd_soc_dai_ops iec958_dai_ops = {
    .set_sysclk = iec958_set_dai_sysclk,
    .set_fmt    = iec958_set_dai_fmt,
    .digital_mute = iec958_mute,

    .prepare    = iec958_pcm_prepare,
    .hw_params  = iec958_hw_params,
    .shutdown   = iec958_shutdown,
};

#if defined(CONFIG_SND_SOC_TCC_CDIF)
static struct snd_soc_dai_driver iec958_dai[] = {
		[0]={
#else
static struct snd_soc_dai_driver iec958_dai = {
#endif
	.name = "IEC958",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
#if defined(CONFIG_SND_SOC_TCC_MULTICHANNEL)
		.channels_max = 8,
        .rates    = SNDRV_PCM_RATE_8000_192000,
        .rate_max = 192000,
#else
        .channels_max = 2,
        .rates    = SNDRV_PCM_RATE_8000_96000,
        .rate_max = 96000,
#endif
/** 
* @author sjpark@cleinsoft
* @date 2014/04/21
* Support spdif 24bit sample bit. 
**/
		.formats  = (SNDRV_PCM_FMTBIT_U24_LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U16_LE),
	},	// Planet 20120306
	.capture = {	// Planet 20130709 S/PDIF_Rx
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates    = SNDRV_PCM_RATE_8000_96000,
		.rate_max = 96000,
/** 
* @author sjpark@cleinsoft
* @date 2014/04/21
* Support spdif 24bit sample bit. 
**/
		.formats  = (SNDRV_PCM_FMTBIT_U24_LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U16_LE),
	},
    .ops = &iec958_dai_ops,
#if defined(CONFIG_SND_SOC_TCC_CDIF)
	},
	[1]={	// CDIF_20140113
		.name = "cdif-dai-dummy",
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates    = SNDRV_PCM_RATE_8000_96000,
			.rate_max = 96000,
			.formats = (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U16_LE),},
		.ops = &iec958_dai_ops,
	}
#endif
};


static int tcc_iec958_probe(struct platform_device *pdev)
{
    int ret = 0;
    alsa_dbg(" %s \n", __func__);
#if defined(CONFIG_SND_SOC_TCC_CDIF)
	ret = snd_soc_register_codec(&pdev->dev, &soc_codec_dev_tcc_dummy, iec958_dai, ARRAY_SIZE(iec958_dai));	// CDIF_20140113
#else
    ret = snd_soc_register_codec(&pdev->dev, &soc_codec_dev_tcc_dummy, &iec958_dai, 1);
#endif
    printk("%s()   ret: %d\n", __func__, ret);
    return ret;

	//return snd_soc_register_dai(&pdev->dev, &iec958_dai);
}

static int tcc_iec958_remove(struct platform_device *pdev)
{
    alsa_dbg(" %s \n", __func__);
    snd_soc_unregister_codec(&pdev->dev);
    //snd_soc_unregister_dai(&pdev->dev);
	return 0;
}

static struct platform_driver tcc_iec958_driver = {
	.driver = {
			.name = "tcc-iec958",
			.owner = THIS_MODULE,
	},

	.probe = tcc_iec958_probe,
	.remove = __devexit_p(tcc_iec958_remove),
};

static int __init snd_tcc_iec958_platform_init(void)
{
    alsa_dbg(" %s \n", __func__);
	return platform_driver_register(&tcc_iec958_driver);
}
module_init(snd_tcc_iec958_platform_init);

static void __exit snd_tcc_iec958_platform_exit(void)
{
    alsa_dbg(" %s \n", __func__);
	platform_driver_unregister(&tcc_iec958_driver);
}
module_exit(snd_tcc_iec958_platform_exit);

/***********************************************************************/



MODULE_AUTHOR("Telechips");
MODULE_DESCRIPTION("Telechips TCC PCM DMA module");
MODULE_LICENSE("GPL");

