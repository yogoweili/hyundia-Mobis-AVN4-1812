/*
 * linux/sound/soc/tcc/tcc-pcm.h   
 *
 * Based on:    linux/sound/arm/pxa2xx-pcm.h
 * Author:  <linux@telechips.com>
 * Created:	Nov 30, 2004
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

#ifndef _tcc_PCM_H
#define _tcc_PCM_H

#include <mach/hardware.h>
#include <mach/bsp.h>

#define __play_buf_size 65536
#define __play_buf_cnt  8

#if defined(CONFIG_SND_SOC_TCC_MULTICHANNEL)
#define __cap_buf_size 65536	// MultiChannel_In_20140113
#else
#define __cap_buf_size 8192
#endif
#define __cap_buf_cnt  8

#define DMA_CH_OUT 0x0001
#define DMA_CH_IN  0x0002
#define DMA_CH_SPDIF 0x0004
#define DMA_CH_SPDIF_IN 0x0008	// Planet 20130709 S/PDIF_Rx
#if defined(CONFIG_SND_SOC_TCC_CDIF)
#define DMA_CH_CDIF 0x0010		// CDIF_20140113
#endif

#define TCC_INTERRUPT_REQUESTED 	0x0001
#define TCC_RUNNING_PLAY        	0x0002
#define TCC_RUNNING_CAPTURE     	0x0004
#define TCC_RUNNING_SPDIF       	0x0008
#define TCC_RUNNING_SPDIF_CAPTURE 	0x0100	// Planet 20130709 S/PDIF_Rx
#if defined(CONFIG_SND_SOC_TCC_CDIF)
#define TCC_RUNNING_CDIF_CAPTURE 	0x0200	// CDIF_20140113
#endif

#define TCC_TX_INTERRUPT_REQUESTED 0x0010
#define TCC_RX_INTERRUPT_REQUESTED 0x0020
#define NEXT_BUF(_s_,_b_) { \
    (_s_)->_b_##_idx++; \
    (_s_)->_b_##_idx %= (_s_)->nbfrags; \
    (_s_)->_b_ = (_s_)->buffers + (_s_)->_b_##_idx; }

/**
 **   @author sjpark@cleinsoft
 **   @date 2014/11/i06
 **   To remove pop-noise - multi channel support
 **   Define word size to calculate delay time
 **/
#define TCC_DMA_HALF_BUFF_STEREO 64 
#define TCC_DMA_HALF_BUFF_MULTI  16 

typedef struct _tcc_interrupt_info_x {
    struct snd_pcm_substream *play_ptr;
    struct snd_pcm_substream *cap_ptr;
    struct snd_pcm_substream *spdif_ptr;
	struct snd_pcm_substream *spdif_cap_ptr;	// Planet 20130709 S/PDIF_Rx
#if defined(CONFIG_SND_SOC_TCC_CDIF)
	struct snd_pcm_substream *cdif_cap_ptr;		// CDIF_20140113
#endif
    struct mutex mutex;
    unsigned int flag;

    volatile ADMASPDIFTX adma_spdif_tx_base;
    volatile ADMA adma;
    volatile ADMADAI dai;
} tcc_interrupt_info_x;

extern tcc_interrupt_info_x tcc_alsa_info;

struct tcc_pcm_dma_params {
    char *name; /* stream identifier */
    int channel;
    dma_addr_t dma_addr;
    int dma_size; /* Size of the DMA transfer */
};

struct tcc_runtime_data {
    int dma_ch;
    struct tcc_pcm_dma_params *params;
    dma_addr_t dma_buffer;          /* physical address of dma buffer */
    dma_addr_t dma_buffer_end;      /* first address beyond DMA buffer */
    size_t period_size;
    size_t nperiod;     
    dma_addr_t period_ptr;          /* physical address of next period */
};

typedef struct {
    int size;       /* buffer size */
    char *start;        /* point to actual buffer */
    dma_addr_t dma_addr;    /* physical buffer address */
    struct semaphore sem;    /* down before touching the buffer */
    int master;     /* owner for buffer allocation, contain size when true */
    int dma_size;
} audio_buf_t;

typedef struct {
    audio_buf_t *buffers;   /* pointer to audio buffer structures */
    audio_buf_t *buf;   /* current buffer used by read/write */
    u_int buf_idx;      /* index for the pointer above */
    u_int play_idx;     /* the current buffer that playing */
    u_int fragsize;     /* fragment i.e. buffer size */
    u_int nbfrags;      /* nbr of fragments */
} audio_stream_t;

typedef struct {
    char *addr;
    dma_addr_t dma_addr;
    int  buf_size;  // total size of DMA
    int  page_size; // size of each page
    int  dma_half;  // 0 or 1, mark the next available buffer
}dma_buf_t;

/* platform data */
//extern struct snd_soc_platform tcc_soc_platform;

#endif
