/*!
 * fm1288.c  --  FM1288 ALSA SoC Audio driver
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>

#include "fm1288.h"

#if defined(CONFIG_DAUDIO)
#include "mach/daudio.h"
#endif

#define FM1288_VERSION "0.01"

#define FM1288_NUM_SUPPLIES 0

#define GPIO_RESET_FM1288 TCC_GPB(22)

#if defined(CONFIG_DAUDIO_20140220)
	#if defined(CONFIG_SND_SOC_TCC_MULTICHANNEL)
		#define FM1288_RATES (SNDRV_PCM_RATE_8000_192000)
	#else
		#define FM1288_RATES (SNDRV_PCM_RATE_8000_96000)
	#endif
#else
	#define FM1288_RATES (SNDRV_PCM_RATE_8000_96000)
#endif

#define FM1288_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
	SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_U16_LE)	// Planet 20130801 CDIF

#if 0 
#define alsa_dbg(f, a...)  printk("== alsa-debug == " f, ##a)
#else
#define alsa_dbg(f, a...)
#endif

#define print_func alsa_dbg("FM1288 : [%s]\n", __func__);

static int fm1288_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai)
{
	print_func;
	return 0;
}

static int fm1288_set_dai_sysclk(struct snd_soc_dai *codec_dai,
		int clk_id, unsigned int freq, int dir)
{
	print_func;
	return 0;
}

static int fm1288_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	print_func;
	return 0;
}

static int fm1288_startup(struct snd_pcm_substream *substream,
		                struct snd_soc_dai *dai)
{
	print_func;
	return 0;
}

static void fm1288_shutdown(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	print_func;
}

static int fm1288_hw_free(struct snd_pcm_substream *substream)
{
	print_func;
	return 0;
}

static struct snd_soc_dai_ops fm1288_dai_ops = {
	.startup    = fm1288_startup,
	.shutdown   = fm1288_shutdown,
	.set_sysclk	= fm1288_set_dai_sysclk,
	.set_fmt	= fm1288_set_dai_fmt,
	.hw_params	= fm1288_hw_params,
	.hw_free    = fm1288_hw_free,
};

static struct snd_soc_dai_driver fm1288_dai = {
	.name = CODEC_FM1288_DAI_NAME,
	.playback = {
		.stream_name = "Playback",
#if (DAUDIO_MULTICHANNEL_USE == CODEC_FM1288)
		.channels_max = 8,
#else
		.channels_max = 2,
#endif
		.channels_min = 1,
		.rates = FM1288_RATES,
		.formats = FM1288_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
#if (DAUDIO_MULTICHANNEL_USE == CODEC_FM1288)
		.channels_max = 8,
#else
		.channels_max = 2,
#endif
		.channels_min = 1,
		.rates = FM1288_RATES,
		.formats = FM1288_FORMATS,
	},
	.ops = &fm1288_dai_ops,
	.symmetric_rates = 1,
};

static int fm1288_suspend(struct snd_soc_codec *codec, pm_message_t state)
{
	print_func;
	return 0;
}

static int fm1288_resume(struct snd_soc_codec *codec)
{
	print_func;
	return 0;
}

static int fm1288_probe(struct snd_soc_codec *codec)
{
	print_func;
	return 0;
}

static int fm1288_remove(struct snd_soc_codec *codec)
{
	print_func;
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_fm1288 = {
	.probe =	fm1288_probe,
	.remove =	fm1288_remove,
	.suspend =	fm1288_suspend,
	.resume =	fm1288_resume,
};

static __devinit int fm1288_codec_probe(struct platform_device *pdev)
{
	int ret;
	print_func;
	ret =  snd_soc_register_codec(&pdev->dev, &soc_codec_dev_fm1288, &fm1288_dai, 1);
	print_func;

	if (ret) {
		alsa_dbg("Failed to register codec\n");
		return -1;
	}
	return ret;
}

static int __devexit fm1288_codec_remove(struct platform_device *pdev)
{
	print_func;
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

static struct platform_driver fm1288_codec_driver = {
	.driver = {
		.name = CODEC_FM1288_NAME, 
		.owner = THIS_MODULE,
	},  
	.probe = fm1288_codec_probe,
	.remove = __devexit_p(fm1288_codec_remove),
};

static int __init fm1288_modinit(void)
{
	print_func;
	return platform_driver_register(&fm1288_codec_driver);
}
module_init(fm1288_modinit);

static void __exit fm1288_exit(void)
{
	print_func;
	platform_driver_unregister(&fm1288_codec_driver);
}
module_exit(fm1288_exit);

MODULE_DESCRIPTION("FM1288 ALSA driver");
MODULE_AUTHOR("Cleinsoft, Inc.");
MODULE_LICENSE("GPL");
