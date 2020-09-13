/*
 * linux/sound/soc/tcc/tcc-i2s.c
 *
 * Based on:    linux/sound/soc/pxa/pxa2xx-i2s.h
 * Author: Liam Girdwood<liam.girdwood@wolfsonmicro.com or linux@wolfsonmicro.com>
 * Rewritten by:    <linux@telechips.com>
 * Created:     12th Aug 2005   Initial version.
 * Modified:    Nov 25, 2008
 * Description: ALSA PCM interface for the Intel PXA2xx chip
 *
 * Copyright 2005 Wolfson Microelectronics PLC.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/soc.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/bsp.h>
#include <mach/tca_ckc.h>
#include <mach/tca_i2s.h>

#include "tcc-pcm.h"
#include "tcc-i2s.h"
#include "tcc/tca_tcchwcontrol.h"
#include <mach/daudio.h>
#include <mach/gpio.h>

/************************************************************************
 * definition
 ************************************************************************/
#define AMDA_RX         0
#define AMDA_TX         1

// Planet 20130709 S/PDIF_Rx Start
#define STREAM_CAPTURE  0
#define STREAM_PLAYBACK 1
// Planet 20130709 S/PDIF_Rx End

#if defined(CONFIG_SND_SOC_TCC_MULTICHANNEL)
#define TCC_I2S_RATES   SNDRV_PCM_RATE_8000_192000
#else
#define TCC_I2S_RATES   SNDRV_PCM_RATE_8000_96000
#endif

#undef alsa_dbg
#if 0 
#define alsa_dbg(f, a...)  printk("== alsa-debug I2S CH0 == " f, ##a)
#else
#define alsa_dbg(f, a...)  
#endif

/************************************************************************
 * global variable
 ************************************************************************/
#ifdef CONFIG_HAS_EARLYSUSPEND
    static struct early_suspend early_suspend;
#endif	


static ADMADAI     gADMA_DAI;
static ADMASPDIFTX gADMA_SPDIFTX;
static ADMASPDIFRX gADMA_SPDIFRX;	// Planet 20130709 S/PDIF_Rx
#if defined(CONFIG_SND_SOC_TCC_CDIF)
static ADMACDIF    gADMA_CDIF;		// CDIF_20140113
#endif

#if defined(CONFIG_ARCH_TCC893X)
struct clk *tcc_adma0_clk;
struct clk *tcc_dai_clk;
struct clk *tcc_dam_clk;
struct clk *tcc_adma1_clk;
struct clk *tcc_spdif_clk;
struct clk *pll3_clk;
#if defined(CONFIG_SND_SOC_TCC_CDIF)
struct clk *tcc_adma2_clk;	// for cdif	// CDIF_20140113
struct clk *tcc_cdif_clk;				// CDIF_20140113
#endif
#endif

static struct tcc_pcm_dma_params tcc_i2s_pcm_stereo_out = {
	.name       = "I2S PCM Stereo out",
    .dma_addr   = 0,
	.channel    = 0,
};

static struct tcc_pcm_dma_params tcc_i2s_pcm_stereo_in = {
	.name       = "I2S PCM Stereo in",
    .dma_addr   = 0,
	.channel    = 1,
};

static struct tcc_pcm_dma_params tcc_i2s_pcm_spdif_out = {
    .name       = "I2S PCM S/PDIF out",
    .dma_addr   = 0,
    .channel    = 0,
};

// Planet 20130709 S/PDIF_Rx Start
static struct tcc_pcm_dma_params tcc_i2s_pcm_spdif_in = {
    .name       = "I2S PCM S/PDIF in",
    .dma_addr   = 0,
    .channel    = 1,
};
// Planet 20130709 S/PDIF_Rx End

#if defined(CONFIG_SND_SOC_TCC_CDIF)
// CDIF_20140113 Start
static struct tcc_pcm_dma_params tcc_pcm_cdif = {
	.name       = "CDIF in",
	.dma_addr   = 0,
	.channel    = 0,
};
// CDIF_20140113 End
#endif


static unsigned int io_ckc_get_dai_clock(unsigned int freq)
{
    switch (freq) {
        case 44100: 
            return (44100 * 256);
        case 22000: 
            return (22050 * 256);
        case 11000: 
            return (11025 * 256);
        default:
            break;
    }
    return (freq * 256);
}




/************************************************************************
 * export Function
 ************************************************************************/
void tcc_i2s_set_clock(unsigned int ClockRate)
{
    unsigned int clk_rate;
#if defined(CONFIG_MEM_CLK_SYNC_MODE)
    unsigned int  pll3_rate;
	volatile PCKC pCKC = (volatile PCKC)tcc_p2v(HwCKC_BASE);
#endif

    if(tcc_dai_clk == NULL)
        return ;

    clk_rate = io_ckc_get_dai_clock(ClockRate);

#if defined(CONFIG_MEM_CLK_SYNC_MODE)
	if ((pCKC->MBUSCTRL & 0x1) == 0) {
		if (ClockRate == 32000)
			pll3_rate = 370*1000*1000;
		else if (ClockRate == 44100)
			pll3_rate = 336*1000*1000;
		else if (ClockRate == 48000)
			pll3_rate = 486*1000*1000;
		else if (ClockRate == 88000)
			pll3_rate = 400*1000*1000;
		else if (ClockRate == 96000)
			pll3_rate = 370*1000*1000;
		else if (ClockRate == 192000)
			pll3_rate = 508*1000*1000;
		else
			pll3_rate = 336*1000*1000;

		if (tcc_spdif_clk)
			clk_set_rate(tcc_spdif_clk, 1000000);

		clk_set_rate(pll3_clk, pll3_rate);
	}
#endif
    
#if defined(CONFIG_MEM_CLK_SYNC_MODE)
    clk_set_rate(tcc_dai_clk, 0);
#endif
    clk_set_rate(tcc_dai_clk, clk_rate);

    alsa_dbg("tcc_i2s_set_clock()   rate[%d] clk_rate[%d]\n", ClockRate, clk_rate);

#if defined(CONFIG_ARCH_TCC93XX)
	if(tcc_spdif_clk)
	{
	    clk_rate = io_ckc_get_dai_clock(ClockRate) * 2;   /* set 512fs for HDMI */
	    clk_set_rate(tcc_spdif_clk, clk_rate);
	}
#endif

}
EXPORT_SYMBOL(tcc_i2s_set_clock);

void tcc_spdif_set_clock(unsigned int clock_rate, bool stream)	// Planet 20130709 S/PDIF_Rx
{
    unsigned int clk_rate;
    unsigned tmpCfg, tmpStatus;	
    unsigned int tx_ratio = 2;
#if defined(CONFIG_ARCH_TCC893X)
    volatile ADMASPDIFTX *p_adma_spdif_tx_base = (volatile ADMASPDIFTX *)tcc_p2v(BASE_ADDR_SPDIFTX0);
#endif
#if defined(CONFIG_MEM_CLK_SYNC_MODE)
    unsigned int  pll3_rate;
	volatile PCKC pCKC = (volatile PCKC)tcc_p2v(HwCKC_BASE);
#endif

    alsa_dbg("[%s], clock_rate[%u]\n", __func__, clock_rate);

/** 
  * @author sjpark@cleinsoft
  * @date 2014/04/21
  * Set spdif clock as double value to decrease sample noise at 24bit mode.
**/
#if defined(CONFIG_DAUDIO)
    clk_rate = io_ckc_get_dai_clock(clock_rate) * 4;  
#else
    //clk_rate = ((clock_rate * 256 * 2) / 100);
    clk_rate = io_ckc_get_dai_clock(clock_rate) * 2;   /* set 512fs for HDMI */
#endif

#if defined(CONFIG_MEM_CLK_SYNC_MODE)
	if ((pCKC->MBUSCTRL & 0x1) == 0) {
		if (clock_rate == 44100)
			pll3_rate = 429*1000*1000;
		else if (clock_rate == 48000)
			pll3_rate = 639*1000*1000;
		else if (clock_rate == 32000)
			pll3_rate = 213*1000*1000;
		else if (clock_rate == 22000)
			pll3_rate = 429*1000*1000;
		else if (clock_rate == 11000)
			pll3_rate = 604*1000*1000;
		else
			pll3_rate = 429*1000*1000;

		if (tcc_spdif_clk)
			clk_set_rate(tcc_spdif_clk, 1000000);
		clk_set_rate(pll3_clk, pll3_rate);
	}
#endif

	if (tcc_spdif_clk)
	    clk_set_rate(tcc_spdif_clk, clk_rate);

	// Planet 20130709 S/PDIF_Rx Start
	if(stream == STREAM_PLAYBACK) {
	    tmpCfg = p_adma_spdif_tx_base->TxConfig;
	    tmpStatus = p_adma_spdif_tx_base->TxChStat;

/** 
  * @author sjpark@cleinsoft
  * @date 2014/04/21
  * set spdif tx ratio value 
**/
#if defined(CONFIG_DAUDIO)
	    tx_ratio = 7;
#endif
	    if (clock_rate == 44100) {          /* 44.1KHz */
	        p_adma_spdif_tx_base->TxConfig = ((tmpCfg & 0xFFFF00FF) | (tx_ratio << 8));
	        p_adma_spdif_tx_base->TxChStat = ((tmpStatus & 0xFFFFFF3F) | (0 << 6));
	    } else if (clock_rate == 48000) {   /* 48KHz */
	        p_adma_spdif_tx_base->TxConfig = ((tmpCfg & 0xFFFF00FF) | (tx_ratio << 8));
	        p_adma_spdif_tx_base->TxChStat = ((tmpStatus & 0xFFFFFF3F) | (1 << 6));
	    } else if (clock_rate == 32000) {   /* 32KHz */
	        p_adma_spdif_tx_base->TxConfig = ((tmpCfg & 0xFFFF00FF) | (tx_ratio << 8));
	        p_adma_spdif_tx_base->TxChStat = ((tmpStatus & 0xFFFFFF3F) | (2 << 6));
	    } else {                            /* Sampling Rate Converter */
	        p_adma_spdif_tx_base->TxConfig = ((tmpCfg & 0xFFFF00FF) | (tx_ratio << 8));
	        p_adma_spdif_tx_base->TxChStat = ((tmpStatus & 0xFFFFFF3F) | (3 << 6));
	    }
	}
	// Planet 20130709 S/PDIF_Rx End
}
EXPORT_SYMBOL(tcc_spdif_set_clock);




/************************************************************************
 * Local Function
 ************************************************************************/
static int tcc_i2s_init(void)
{
#if defined(CONFIG_ARCH_TCC893X)
	volatile PADMADAI pADMA_DAI = (volatile PADMADAI)tcc_p2v(BASE_ADDR_DAI0);

    alsa_dbg(" %s \n", __func__);

    /* clock enable */
    tcc_dai_clk = clk_get(NULL, CLK_NAME_DAI0);
    if(IS_ERR(tcc_dai_clk))     return (-EINVAL);
    clk_enable(tcc_dai_clk);

    tcc_dam_clk = clk_get(NULL, CLK_NAME_DAM0);
    if(IS_ERR(tcc_dam_clk))     return (-EINVAL);
    clk_enable(tcc_dam_clk);

    BITCLR(pADMA_DAI->DAMR, Hw27);
    clk_set_rate(tcc_dam_clk, 48000*256*4);
    BITSET(pADMA_DAI->DAMR, Hw27);
    
    tcc_adma0_clk = clk_get(NULL, CLK_NAME_ADMA0);
    if(IS_ERR(tcc_adma0_clk))    return (-EINVAL);
    clk_enable(tcc_adma0_clk);
#endif

#if defined(CONFIG_MEM_CLK_SYNC_MODE)
	if (pll3_clk == NULL) {
		pll3_clk = clk_get(NULL, "pll3");
		if (IS_ERR(pll3_clk))       return (-EINVAL);
	}
#endif

/** 
  * @author sjpark@cleinsoft
  * @date 2014/1/20
  * New Sound System - Audio 0 (Slave mode)
**/
    /* set DAI register */
#if defined(CONFIG_DAUDIO)
	#if defined(CONFIG_SND_SOC_DAUDIO_NEW_SOUND_SYSTEM)
		tca_i2s_dai_init_slave_mode(pADMA_DAI);
	#else
		tca_i2s_dai_init(pADMA_DAI);
	#endif
#else
	tca_i2s_dai_init(pADMA_DAI);
#endif

    BITSET(pADMA_DAI->MCCR0, Hw31 | Hw30 | Hw29 | Hw28);
    BITSET(pADMA_DAI->DAMR, Hw27);

	tca_i2s_stop(pADMA_DAI, AMDA_RX);         // ADMA Rx disable
	tca_i2s_stop(pADMA_DAI, AMDA_TX);         // ADMA Tx disable
	return 0;
}

static int tcc_spdif_init(void)
{
    alsa_dbg(" %s \n", __func__);

#if defined(CONFIG_ARCH_TCC893X)
    /* clock enable */
#if !defined(CONFIG_CHIP_TCC8935S) && !defined(CONFIG_CHIP_TCC8933S) && !defined(CONFIG_CHIP_TCC8937S)
    tcc_spdif_clk = clk_get(NULL, CLK_NAME_SPDIF0);
    if(IS_ERR(tcc_spdif_clk))   return (-EINVAL);
    clk_enable(tcc_spdif_clk);
#endif

    tcc_adma1_clk = clk_get(NULL, CLK_NAME_ADMA0);
    if(IS_ERR(tcc_adma1_clk))    return (-EINVAL);
    clk_enable(tcc_adma1_clk);
#else
    /* clock enable */
    tcc_spdif_clk = clk_get(NULL, CLK_NAME_SPDIF);
    if(IS_ERR(tcc_spdif_clk))   return (-EINVAL);
    clk_enable(tcc_spdif_clk);
#endif

#if defined(CONFIG_MEM_CLK_SYNC_MODE)
	if (pll3_clk == NULL) {
		pll3_clk = clk_get(NULL, "pll3");
		if (IS_ERR(pll3_clk))       return (-EINVAL);
	}
#endif

    return 0;
}


#if defined(CONFIG_SND_SOC_TCC_CDIF)
// CDIF_20140113 Start
static int tcc_cdif_init(void)
{
	alsa_dbg(" %s \n", __func__);

#if defined(CONFIG_ARCH_TCC893X)
	tcc_adma2_clk = clk_get(NULL, CLK_NAME_ADMA0);
	if(IS_ERR(tcc_adma2_clk))    return (-EINVAL);
	clk_enable(tcc_adma2_clk);
#endif

	return 0;
}
// CDIF_20140113 End
#endif

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Function:		StartI2SClock()

Description:	Enables the I2S clock that drives the audio codec chip.

Returns:		N/A
-------------------------------------------------------------------*/
static int tcc_i2s_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct snd_soc_dai *cpu_dai = rtd->cpu_dai;

    alsa_dbg("tcc_i2s_startup() \n");
    if (!cpu_dai->active) {
    }

    return 0;
}

static int tcc_i2s_set_dai_fmt(struct snd_soc_dai *cpu_dai, unsigned int fmt)
{
    alsa_dbg(" %s \n", __func__);

    switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
    case SND_SOC_DAIFMT_CBS_CFS:
        break;
    case SND_SOC_DAIFMT_CBM_CFS:
        break;
    default:
        break;
    }


    /* interface format */
    switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
    case SND_SOC_DAIFMT_I2S:
        break;
    case SND_SOC_DAIFMT_LEFT_J:
        break;
    }
    return 0;
}

static int tcc_i2s_set_dai_sysclk(struct snd_soc_dai *cpu_dai,
                                      int clk_id, unsigned int freq, int dir)
{
    alsa_dbg(" %s \n", __func__);

    if (clk_id != TCC_I2S_SYSCLK)
        return -ENODEV;  

    return 0;
}

static int tcc_i2s_hw_params(struct snd_pcm_substream *substream,
                                 struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
    struct tcc_pcm_dma_params *dma_data;
    volatile PADMADAI pADMA_DAI = (volatile PADMADAI)tcc_p2v(BASE_ADDR_DAI0);

    if (substream->pcm->device == __I2S_DEV_NUM__) {
/** 
* @author sjpark@cleinsoft
* @date 2014/04/21
* Support master / slave mode switching 
**/
#if defined(CONFIG_SND_SOC_DAUDIO_CLOCK_ROLE_SWITCHING)
		if(params->reserved[SNDRV_PCM_HW_PARAM_EFLAG0] & SNDRV_PCM_MODE_SLAVE){
			alsa_dbg("Slave mode \n");
			BITCLR( pADMA_DAI->DAMR , Hw11 | Hw9 | Hw10 | Hw31 );
		}else{
			alsa_dbg("Master mode \n");
			BITSET( pADMA_DAI->DAMR , Hw11 | Hw9 | Hw10 | Hw31 );
		}
#endif
        if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
            dma_data = &tcc_i2s_pcm_stereo_out;
        }
        else if(substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
            dma_data = &tcc_i2s_pcm_stereo_in;
        }
        snd_soc_dai_set_dma_data(cpu_dai, substream, dma_data);

        // Set DAI clock
		tcc_i2s_set_clock(params_rate(params));
	}

	alsa_dbg("=====================\n");
	alsa_dbg("= rate        : %d\n", params_rate(params));
	alsa_dbg("= channels    : %d\n", params_channels(params));
	alsa_dbg("= period_size : %d\n", params_period_size(params));

	return 0;
}

static int tcc_spdif_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
    alsa_dbg(" %s \n", __func__);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {        
/**
*   @author sjpark@cleinsoft
*   @date 2014/07/21
*   NAVI voice on - gpio control 
**/
#if defined(CONFIG_DAUDIO) && (DAUDIO_NAVI_VOC_CTRL == CODEC_AUDIO0_SPDIF)
		gpio_direction_output(GPIO_NAVI_VOC_ON, PORT_HIGH);
#endif
	} 
    return 0;
}

static int tcc_spdif_hw_params(struct snd_pcm_substream *substream,
                                 struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct snd_soc_dai *cpu_dai = rtd->cpu_dai;

    alsa_dbg(" %s \n", __func__);

	// Planet 20130709 S/PDIF_Rx Start
    if (substream->pcm->device == __SPDIF_DEV_NUM__) {
		if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			alsa_dbg(" %s Playback \n", __func__);
	        cpu_dai->playback_dma_data = &tcc_i2s_pcm_spdif_out;
			// Set SPDIF clock
			tcc_spdif_set_clock(params_rate(params), STREAM_PLAYBACK);
		} 
		else if(substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			alsa_dbg(" %s Capture \n", __func__);
			cpu_dai->capture_dma_data = &tcc_i2s_pcm_spdif_in;
			// Set SPDIF clock
			tcc_spdif_set_clock(params_rate(params), STREAM_CAPTURE);
		}
    }
	// Planet 20130709 S/PDIF_Rx End
    return 0;
}

static void tcc_spdif_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
    alsa_dbg(" %s \n", __func__);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {        
/**
*   @author sjpark@cleinsoft
*   @date 2014/07/21
*   NAVI voice on - gpio control 
**/
#if defined(CONFIG_DAUDIO) && (DAUDIO_NAVI_VOC_CTRL == CODEC_AUDIO0_SPDIF)
		gpio_direction_output(GPIO_NAVI_VOC_ON, PORT_LOW);
#endif
    } 
}

#if defined(CONFIG_SND_SOC_TCC_CDIF)
// CDIF_20140113 Start
static int tcc_cdif_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;

	alsa_dbg(" %s \n", __func__);

	cpu_dai->capture_dma_data = &tcc_pcm_cdif;

	return 0;
}
// CDIF_20140113 End
#endif

static int tcc_i2s_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
    int ret = 0;
	volatile PADMADAI1 pDAI  = (volatile PADMADAI1)tcc_p2v(BASE_ADDR_DAI0);

    alsa_dbg(" %s \n", __func__);

    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
    case SNDRV_PCM_TRIGGER_RESUME:
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        break;

    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        break;

    default:
        ret = -EINVAL;
    }
    return ret;
}

static void tcc_i2s_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
    alsa_dbg(" %s \n", __func__);
    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {        
    } 
    else {
    }
}

static int tcc_i2s_suspend(struct snd_soc_dai *dai)
{
#if defined(CONFIG_ARCH_TCC893X)
	volatile PADMADAI     pADMA_DAI     = (volatile PADMADAI)tcc_p2v(BASE_ADDR_DAI0);
	volatile PADMASPDIFTX pADMA_SPDIFTX = (volatile PADMASPDIFTX)tcc_p2v(BASE_ADDR_SPDIFTX0);
	volatile PADMASPDIFRX pADMA_SPDIFRX = (volatile PADMASPDIFRX)tcc_p2v(BASE_ADDR_SPDIFRX0);	// Planet 20130709 S/PDIF_Rx
#if defined(CONFIG_SND_SOC_TCC_CDIF)
	volatile PADMACDIF    pADMA_CDIF    = (volatile PADMACDIF)tcc_p2v(BASE_ADDR_CDIF0);			// CDIF_20140113
#endif
#endif

    alsa_dbg(" %s \n", __func__);
    if(dai->id == 0) {  // DAI
        gADMA_DAI.DAMR   = pADMA_DAI->DAMR;
        gADMA_DAI.DAVC   = pADMA_DAI->DAVC;
        gADMA_DAI.MCCR0  = pADMA_DAI->MCCR0;
        gADMA_DAI.MCCR1  = pADMA_DAI->MCCR1;

        if(tcc_dai_clk)
            clk_disable(tcc_dai_clk);

#if defined(CONFIG_ARCH_TCC893X)
        if(tcc_adma0_clk)
            clk_disable(tcc_adma0_clk);
#else
        if(tcc_adma_clk)
            clk_disable(tcc_adma_clk);
#endif
	} else 
#if defined(CONFIG_SND_SOC_TCC_CDIF)
		if(dai->id == 1)
#endif
	{ // SPDIFTX
        gADMA_SPDIFTX.TxConfig  = pADMA_SPDIFTX->TxConfig;
        gADMA_SPDIFTX.TxChStat  = pADMA_SPDIFTX->TxChStat;
        gADMA_SPDIFTX.TxIntMask = pADMA_SPDIFTX->TxIntMask;
//        gADMA_SPDIFTX.TxIntStat = pADMA_SPDIFTX->TxIntStat;
        gADMA_SPDIFTX.DMACFG    = pADMA_SPDIFTX->DMACFG;

						// Planet 20130709 S/PDIF_Rx
		gADMA_SPDIFRX.RxConfig  = pADMA_SPDIFRX->RxConfig;
		gADMA_SPDIFRX.RxStatus  = pADMA_SPDIFRX->RxStatus;
		gADMA_SPDIFRX.RxIntMask = pADMA_SPDIFRX->RxIntMask;

        if(tcc_spdif_clk)
            clk_disable(tcc_spdif_clk);
#if defined(CONFIG_ARCH_TCC893X)
        if(tcc_adma1_clk)
            clk_disable(tcc_adma1_clk);
#endif
    }
#if defined(CONFIG_SND_SOC_TCC_CDIF)
	else { // CDIF_20140113
#if defined(CONFIG_ARCH_TCC893X)
	gADMA_CDIF.CICR = pADMA_CDIF->CICR;

	if(tcc_adma2_clk)
		clk_disable(tcc_adma2_clk);
#endif
	}
#endif

    return 0;
}

static int tcc_i2s_resume(struct snd_soc_dai *dai)
{
#if defined(CONFIG_ARCH_TCC893X)
	volatile PADMADAI     pADMA_DAI     = (volatile PADMADAI)tcc_p2v(BASE_ADDR_DAI0);
	volatile PADMASPDIFTX pADMA_SPDIFTX = (volatile PADMASPDIFTX)tcc_p2v(BASE_ADDR_SPDIFTX0);
	volatile PADMASPDIFRX pADMA_SPDIFRX = (volatile PADMASPDIFRX)tcc_p2v(BASE_ADDR_SPDIFRX0);	// Planet 20130709 S/PDIF_Rx
#if defined(CONFIG_SND_SOC_TCC_CDIF)
	volatile PADMACDIF    pADMA_CDIF    = (volatile PADMACDIF)tcc_p2v(BASE_ADDR_CDIF0);			// CDIF_20140113
#endif
#endif

    alsa_dbg(" %s \n", __func__);
    if(dai->id == 0) {  // DAI
        if(tcc_dai_clk)
            clk_enable(tcc_dai_clk);

#if defined(CONFIG_ARCH_TCC893X)
        if(tcc_adma0_clk)
            clk_enable(tcc_adma0_clk);
#endif

        pADMA_DAI->DAMR   = gADMA_DAI.DAMR;
        pADMA_DAI->DAVC   = gADMA_DAI.DAVC;
        pADMA_DAI->MCCR0  = gADMA_DAI.MCCR0;
        pADMA_DAI->MCCR1  = gADMA_DAI.MCCR1;
    }
    else              // SPDIFTX
#if defined(CONFIG_SND_SOC_TCC_CDIF)
		if(dai->id == 1)
#endif
	{
        if(tcc_spdif_clk)
            clk_enable(tcc_spdif_clk);
#if defined(CONFIG_ARCH_TCC893X)
        if(tcc_adma1_clk)
            clk_enable(tcc_adma1_clk);
#endif

		// Planet 20130709 S/PDIF_Tx
        pADMA_SPDIFTX->TxConfig  = gADMA_SPDIFTX.TxConfig;
        pADMA_SPDIFTX->TxChStat  = gADMA_SPDIFTX.TxChStat;
        pADMA_SPDIFTX->TxIntMask = gADMA_SPDIFTX.TxIntMask;
//        pADMA_SPDIFTX->TxIntStat = gADMA_SPDIFTX.TxIntStat;
        pADMA_SPDIFTX->DMACFG    = gADMA_SPDIFTX.DMACFG;

		// Planet 20130709 S/PDIF_Rx
		pADMA_SPDIFRX->RxConfig  = gADMA_SPDIFRX.RxConfig;
		pADMA_SPDIFRX->RxStatus  = gADMA_SPDIFRX.RxStatus;
		pADMA_SPDIFRX->RxIntMask = gADMA_SPDIFRX.RxIntMask;
    }
#if defined(CONFIG_SND_SOC_TCC_CDIF)
	else {
#if defined(CONFIG_ARCH_TCC893X)
		if(tcc_adma2_clk)
			clk_enable(tcc_adma2_clk);

		pADMA_CDIF->CICR = gADMA_CDIF.CICR ;
#endif	
	}
#endif

    return 0;
}


#ifdef CONFIG_HAS_EARLYSUSPEND
static void tcc_i2s_early_suspend(struct early_suspend *h);
static void tcc_i2s_late_resume(struct early_suspend *h);
#endif


                           
int tcc_i2s_probe(struct snd_soc_dai *dai)
{
    alsa_dbg("== alsa-debug == %s() \n", __func__);
    if(tcc_i2s_init())
        return -EINVAL;

    // default clock of DAI : 44100Hz
    tcc_i2s_set_clock(44100);

#ifdef CONFIG_HAS_EARLYSUSPEND
	early_suspend.suspend = tcc_i2s_early_suspend;
	early_suspend.resume  = tcc_i2s_late_resume;
	early_suspend.level   = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	//register_early_suspend(&early_suspend);
#endif    
    
    return 0;
}

int tcc_spdif_probe(struct snd_soc_dai *dai)
{
    alsa_dbg("%s() \n", __func__);
    if(tcc_spdif_init())
        return -EINVAL;

    // default clock of SPDIF : 44100Hz
    tcc_spdif_set_clock(44100, STREAM_PLAYBACK);	// Planet 20130709 S/PDIF_Rx

/**
*   @author sjpark@cleinsoft
*   @date 2014/07/21
*   NAVI voice on - gpio control 
**/
#if defined(CONFIG_DAUDIO) && (DAUDIO_NAVI_VOC_CTRL == CODEC_AUDIO0_SPDIF)
	tcc_gpio_config(GPIO_NAVI_VOC_ON, GPIO_FN0| GPIO_OUTPUT| GPIO_LOW);
	gpio_request(GPIO_NAVI_VOC_ON, "NAVI_VIOCE_ON");
#endif
	
    return 0;
}

#if defined(CONFIG_SND_SOC_TCC_CDIF)
// CDIF_20140113 Start
int tcc_cdif_probe(struct snd_soc_dai *dai)
{
	alsa_dbg("%s() \n", __func__);
	if(tcc_cdif_init())
		return -EINVAL;

	return 0;
}
// CDIF_20140113 End
#endif

static struct snd_soc_dai_ops tcc_i2s_ops = {
    .set_sysclk = tcc_i2s_set_dai_sysclk,
    .set_fmt    = tcc_i2s_set_dai_fmt,

    .startup    = tcc_i2s_startup,
    .shutdown   = tcc_i2s_shutdown,
    .hw_params  = tcc_i2s_hw_params,
    .trigger    = tcc_i2s_trigger,
};

static struct snd_soc_dai_ops tcc_spdif_ops = {
    .set_sysclk = tcc_i2s_set_dai_sysclk,
    .set_fmt    = tcc_i2s_set_dai_fmt,

    .startup    = tcc_spdif_startup,
    .shutdown   = tcc_spdif_shutdown,
    .hw_params  = tcc_spdif_hw_params,
    .trigger    = tcc_i2s_trigger,
};

#if defined(CONFIG_SND_SOC_TCC_CDIF)
// CDIF_20140113 Start
static struct snd_soc_dai_ops tcc_cdif_ops = {
	.set_sysclk = tcc_i2s_set_dai_sysclk,
	.set_fmt    = tcc_i2s_set_dai_fmt,

	.startup    = tcc_i2s_startup,
	.shutdown   = tcc_i2s_shutdown,
	.hw_params  = tcc_cdif_hw_params,
	.trigger    = tcc_i2s_trigger,
};
// CDIF_20140113 End
#endif

static struct snd_soc_dai_driver tcc_i2s_dai[] = {
#if defined(CONFIG_SND_SOC_TCC_CDIF)
    [0] = {
#else
    [__I2S_DEV_NUM__] = {
#endif
        .name = "tcc-dai-i2s",
        .probe = tcc_i2s_probe,
        .suspend = tcc_i2s_suspend,
        .resume = tcc_i2s_resume,

        .playback = {
            .channels_min = 2,
#if defined(CONFIG_SND_SOC_TCC_MULTICHANNEL)
            .channels_max = 8,
#else
            .channels_max = 2,
#endif
            .rates = TCC_I2S_RATES,
            .formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U16_LE | SNDRV_PCM_FMTBIT_IEC958_SUBFRAME_LE,},  //should be change? phys:32 width:16 //Planet 20130801 CDIF
        .capture = {
            .channels_min = 2,
#if defined(CONFIG_SND_SOC_TCC_MULTICHANNEL)
            .channels_max = 8,
#else
            .channels_max = 2,
#endif
            .rates = TCC_I2S_RATES,
            .formats = SNDRV_PCM_FMTBIT_S16_LE,}, //should be change? phys:32 width:16
        .ops   = &tcc_i2s_ops,
    },
#if defined(CONFIG_SND_SOC_TCC_CDIF)
    [1] = 
#else
    [__SPDIF_DEV_NUM__] =
#endif
    {
        .name = "tcc-dai-spdif",
        .probe = tcc_spdif_probe,
        .suspend = tcc_i2s_suspend,
        .resume  = tcc_i2s_resume,

        .playback = {
            .channels_min = 2,
#if defined(CONFIG_SND_SOC_TCC_MULTICHANNEL)
            .channels_max = 8,
#else
            .channels_max = 2,
#endif
            .rates = TCC_I2S_RATES,
/** 
* @author sjpark@cleinsoft
* @date 2014/04/21
* Support spdif 24bit sample bit. 
**/
            .formats = SNDRV_PCM_FMTBIT_U24_LE | SNDRV_PCM_FMTBIT_S24_LE |SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U16_LE | SNDRV_PCM_FMTBIT_IEC958_SUBFRAME_LE,},  //should be change? phys:32 width:16	 // Planet 20120306
		.capture = {	// Planet 20130709 S/PDIF_Rx
			.channels_min = 2,
			.channels_max = 2,
			.rates = TCC_I2S_RATES,
/** 
* @author sjpark@cleinsoft
* @date 2014/04/21
* Support spdif 24bit sample bit. 
**/
			.formats = SNDRV_PCM_FMTBIT_U24_LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U16_LE | SNDRV_PCM_FMTBIT_IEC958_SUBFRAME_LE,},
        .ops   = &tcc_spdif_ops,
    },
#if defined(CONFIG_SND_SOC_TCC_CDIF)
	[2]={	// CDIF_20140113
		.name = "tcc-dai-cdif",
		.probe = tcc_cdif_probe,
		.suspend = tcc_i2s_suspend,
		.resume  = tcc_i2s_resume,

		.capture = {
			.channels_min = 2,
			.channels_max = 2,
			.rates = TCC_I2S_RATES,
			.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U16_LE,},
		.ops   = &tcc_cdif_ops,
	},
#endif
};

static __devinit int soc_tcc_i2s_probe(struct platform_device *pdev)
{
    alsa_dbg(" %s \n", __func__);
	return snd_soc_register_dais(&pdev->dev, tcc_i2s_dai, ARRAY_SIZE(tcc_i2s_dai));
}


static int __devexit soc_tcc_i2s_remove(struct platform_device *pdev)
{
    alsa_dbg(" %s \n", __func__);
	snd_soc_unregister_dais(&pdev->dev, ARRAY_SIZE(tcc_i2s_dai));
	return 0;
}


static struct platform_driver tcc_i2s_driver = {
	.driver = {
			.name = "tcc-dai",
			.owner = THIS_MODULE,
	},

	.probe = soc_tcc_i2s_probe,
	.remove = __devexit_p(soc_tcc_i2s_remove),
};


static int __init snd_tcc_i2s_init(void)
{
    alsa_dbg(" %s \n", __func__);
    return platform_driver_register(&tcc_i2s_driver);
}
module_init(snd_tcc_i2s_init);

static void __exit snd_tcc_i2s_exit(void)
{
    alsa_dbg(" %s \n", __func__);
    return platform_driver_unregister(&tcc_i2s_driver);
}
module_exit(snd_tcc_i2s_exit);


#ifdef CONFIG_HAS_EARLYSUSPEND
static void tcc_i2s_early_suspend(struct early_suspend *h)
{
	alsa_dbg(" %s\n", __func__);
//	tcc_i2s_dai[0].suspend;
//	tcc_i2s_dai[1].suspend;
}
static void tcc_i2s_late_resume(struct early_suspend *h)
{
	alsa_dbg(" %s\n", __func__);
//	tcc_i2s_dai[0].resume;
//	tcc_i2s_dai[1].resume;
}

#endif

/* Module information */
MODULE_AUTHOR("linux <linux@telechips.com>");
MODULE_DESCRIPTION("Telechips TCC I2S SoC Interface");
MODULE_LICENSE("GPL");
