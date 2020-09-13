/*!
 * Author: sjpark@cleinsoft
 * Created: 2013.08.13
 * Description: FM1288 ALSA Driver
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
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/gpio.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/bsp.h>
#include <mach/tca_ckc.h>
#include <asm/mach-types.h>

#include "tcc/tca_tcchwcontrol.h"
#include "../codecs/fm1288.h"
#include "tcc-pcm-ch1.h"
#include "tcc-i2s-ch1.h"

#if defined(CONFIG_DAUDIO)
#include "mach/daudio.h"
#endif

#undef alsa_dbg
#if 0 
#define alsa_dbg(fmt,arg...)    printk("== alsa-debug == "fmt,##arg)
#else
#define alsa_dbg(fmt,arg...)
#endif

#define print_func alsa_dbg("[%s]\n", __func__);

static struct platform_device *daudio_snd_device;

static int tcc_startup(struct snd_pcm_substream *substream)
{
	print_func;
	return 0;
}

/* we need to unmute the HP at shutdown as the mute burns power on tcc83x */
static void tcc_shutdown(struct snd_pcm_substream *substream)
{
	print_func;
}

static int tcc_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	unsigned int clk = 0;
	int ret = 0, rate = 0;

	print_func;

	switch ((rate = params_rate(params))) {
	case 8000:
	case 16000:
	case 48000:
	case 96000:
		clk = 8192000;
		break;
	case 11025:
	case 22050:
	case 32000:
	case 44100:
	default:
		clk = 11289600;
		break;
	}

	alsa_dbg("hw params - rate : %d\n", rate );

	if(params->reserved[SNDRV_PCM_HW_PARAM_EFLAG0] & SNDRV_PCM_MODE_PCMIF){
		alsa_dbg("DSP mode format set\n");
		/* set codec DAI configuration */
		ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_DSP_B |
				SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
		if (ret < 0) {
			printk("tcc_hw_params()  codec_dai: set_fmt error[%d]\n", ret);
			return ret;
		}

		/* set cpu DAI configuration */
		ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_DSP_B |
				SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
		if (ret < 0) {
			printk("tcc_hw_params()  cpu_dai: set_fmt error[%d]\n", ret);
			return ret;
		}
	}else{
		/* set codec DAI configuration */
		alsa_dbg("IIS mode format set\n");
		ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
				SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
		if (ret < 0) {
			printk("tcc_hw_params()  codec_dai: set_fmt error[%d]\n", ret);
			return ret;
		}

		/* set cpu DAI configuration */
		ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |
				SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
		if (ret < 0) {
			printk("tcc_hw_params()  cpu_dai: set_fmt error[%d]\n", ret);
			return ret;
		}
	}

	/* set the codec system clock for DAC and ADC */
	ret = snd_soc_dai_set_sysclk(codec_dai, FM1288_SYSCLK_XTAL, clk,
		SND_SOC_CLOCK_IN);
	if (ret < 0) {
        printk("tcc_hw_params()  codec_dai: sysclk error[%d]\n", ret);
		return ret;
    }

	/* set the I2S system clock as input (unused) */
   	ret = snd_soc_dai_set_sysclk(cpu_dai, 0, 0, SND_SOC_CLOCK_IN);
	if (ret < 0) {
        printk("tcc_hw_params()  cpu_dai: sysclk error[%d]\n", ret);
		return ret;
    }
	return 0;
}

static struct snd_soc_ops tcc_ops = {
	.startup = tcc_startup,
	.hw_params = tcc_hw_params,
	.shutdown = tcc_shutdown,
};

static int tcc_iec958_dummy_init(struct snd_soc_pcm_runtime *rtd)
{
	print_func;
	return 0;
}

static int i2s_dummy_init(struct snd_soc_pcm_runtime *rtd)
{
	print_func;
	return 0;
}

/* tcc digital audio interface glue - connects codec <--> CPU */
static struct snd_soc_dai_link daudio_dai[] = {
	{   // Audio0 I2S Capture/Player
		.name = "Audio0",
		.stream_name = "Audio0",
		.platform_name  = "tcc-pcm-audio",
		.cpu_dai_name   = "tcc-dai-i2s",

		.codec_name = DAUDIO_AUDIO0_CODEC_NAME,
		.codec_dai_name = DAUDIO_AUDIO0_CODEC_DAI_NAME,
		.init = &i2s_dummy_init,
		.ops = &tcc_ops,
	},
	{   // Audio0 SPDIF Capture/player
		.name = "Audio0-spdif",
		.stream_name = "Audio0-spdif",
		.platform_name  = "tcc-pcm-audio",
		.cpu_dai_name   = "tcc-dai-spdif",

		.codec_name     = "tcc-iec958",
		.codec_dai_name = "IEC958",
		.init = tcc_iec958_dummy_init,
		.ops = &tcc_ops,
	},
    {	// Audio1 I2S Capture/Player
        .name = "Audio1",
        .stream_name = "Audio1",
        .platform_name  = "tcc-pcm-audio-ch1",
        .cpu_dai_name   = "tcc-dai-i2s-ch1",

		.codec_name = DAUDIO_AUDIO1_CODEC_NAME,
		.codec_dai_name = DAUDIO_AUDIO1_CODEC_DAI_NAME,
        .init = &i2s_dummy_init,
        .ops = &tcc_ops,
    },
	{   //Audio1 SPDIF Capture/Player
		.name = "Audio1-spdif",
		.stream_name = "Audio1-spdif",
		.platform_name  = "tcc-pcm-audio-ch1",
		.cpu_dai_name   = "tcc-dai-spdif-ch1",

		.codec_name     = "tcc-iec958-ch1",
		.codec_dai_name = "IEC958",
		.init = tcc_iec958_dummy_init,
		.ops = &tcc_ops,
	}
#if defined(CONFIG_SND_SOC_TCC_CDIF)
	,{	// Audio0 CDIF Capture
		.name = "TCC2",
		.stream_name	= "CDIF-CH0",
		.platform_name	= "tcc-pcm-audio",
		.cpu_dai_name	= "tcc-dai-cdif",

		.codec_name		= "tcc-iec958",
		.codec_dai_name	= "cdif-dai-dummy",
		.init = tcc_iec958_dummy_init,
		.ops = &tcc_ops,
	}	// CDIF_20140113 End
#endif
};

/* tcc audio machine driver */
static struct snd_soc_card daudio_soc_card = {
	.driver_name      = "D-Audio Snd Board",
    .long_name = "D-Audio Snd Board",
	.dai_link  = daudio_dai,
	.num_links = ARRAY_SIZE(daudio_dai),
};

static int __init daudio_snd_board_init(void)
{
	int ret;

    alsa_dbg("ALSA FM1288 Probe [%s]\n", __FUNCTION__);

    daudio_soc_card.name = "D-Audio Board";

    tca_tcc_initport();

	daudio_snd_device = platform_device_alloc("soc-audio", -1);
	if (!daudio_snd_device)
		return -ENOMEM;

	platform_set_drvdata(daudio_snd_device, &daudio_soc_card);

	ret = platform_device_add(daudio_snd_device);
	if (ret) {
        printk(KERN_ERR "Unable to add platform device\n");\
        platform_device_put(daudio_snd_device);
    }
	return ret;
}

static void __exit daudio_snd_board_exit(void)
{
	print_func;
	if(daudio_snd_device)
		platform_device_unregister(daudio_snd_device);
}

module_init(daudio_snd_board_init);
module_exit(daudio_snd_board_exit);

/* Module information */
MODULE_AUTHOR("Cleinsoft, Inc.<android@cleinsoft.com>");
MODULE_DESCRIPTION("Daudio ALSA Sound Driver");
MODULE_LICENSE("GPL");
