/*
 * linux/drivers/char/tcc_lut.c
 * Author:  <linux@telechips.com>
 * Created: June 10, 2008
 * Description: TCC VIOC h/w block 
 *
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
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/div64.h>
#include <asm/mach/map.h>
#include <mach/bsp.h>
#include <plat/pmap.h>
#include <linux/fs.h> 
#include <linux/miscdevice.h>

#include <mach/vioc_rdma.h>
#include <mach/vioc_wdma.h>
#include <mach/vioc_wmix.h>
#include <mach/vioc_config.h>

#include <mach/vioc_api.h>
#include <mach/vioc_disp.h>
#include <mach/tcc_lut_ioctl.h>
#include <mach/vioc_lut.h>



#include <mach/vioc_plugin_tcc893x.h>


#define TCC_LUT_DEBUG 		1
#define dprintk(msg...) 		if(TCC_LUT_DEBUG) { printk("TCC LUT: " msg); }

typedef struct _INTERRUPT_DATA_T {
	struct mutex 			hue_table_lock;
	unsigned int  			dev_opened;
} LUT_DRV_DATA_T;

static LUT_DRV_DATA_T lut_drv_data;

#if defined(CONFIG_DAUDIO)
#include <mach/tcc_lut.h>

int tcc_lut_set(void *argp)
{
	struct VIOC_LUT_VALUE_SET lut_cmd;
	VIOC_LUT *pLUT =(VIOC_LUT*)tcc_p2v(HwVIOC_LUT);

	printk(KERN_INFO "%s \n", __func__);

	if(copy_from_user((void *)&lut_cmd, (const void *)argp, sizeof(lut_cmd)))
	{
		printk(KERN_ERR "%s failed copy_from_user\n", __func__);
		return -EFAULT;
	}

	VIOC_LUT_Set_value(pLUT, lut_cmd.lut_number, lut_cmd.Gamma);

	return 0;
}

#endif

long tcc_lut_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;

	LUT_DRV_DATA_T *lut_drv_data = (LUT_DRV_DATA_T *)file->private_data;

	switch(cmd) {
		case TCC_SW_LUT_HUE:
			{
				struct VIOC_LUT_INFO_Type lut_cmd;

				if(copy_from_user( &lut_cmd, argp, sizeof(lut_cmd))){
					printk("%s cmd:%d error \n", __func__, cmd);
					return -EFAULT;
				}
				
				if(lut_cmd.BaseAddr != lut_cmd.TarAddr)
				{
					int *base_y, *target_y;

					base_y = ioremap_nocache(lut_cmd.BaseAddr, lut_cmd.ImgSizeWidth * lut_cmd.ImgSizeHeight);
					target_y = ioremap_nocache(lut_cmd.TarAddr, lut_cmd.ImgSizeWidth * lut_cmd.ImgSizeHeight);

					if((base_y != NULL) && (target_y != NULL)) {
						memcpy(target_y, base_y, lut_cmd.ImgSizeWidth * lut_cmd.ImgSizeHeight);
					}

					if(base_y != NULL)
						iounmap(base_y);

					if (target_y != NULL)
						iounmap(target_y);
				}
				
				mutex_lock(&lut_drv_data->hue_table_lock);
				tcc_sw_hue_table_opt(lut_cmd.BaseAddr1, lut_cmd.TarAddr1, lut_cmd.ImgSizeWidth, lut_cmd.ImgSizeHeight);
				mutex_unlock(&lut_drv_data->hue_table_lock);			

			}
			return 0;

		case TCC_SW_LUT_SET:
			{
				struct VIOC_SW_LUT_SET_Type lut_cmd;

				if(copy_from_user(&lut_cmd, argp, sizeof(lut_cmd))){
					printk("%s cmd:%d error \n", __func__, cmd);
					return -EFAULT;
				}

				dprintk("sin : %d, cos : %d, saturation : %d \n", lut_cmd.sin_value, lut_cmd.cos_value, lut_cmd.saturation);
				
				mutex_lock(&lut_drv_data->hue_table_lock);
				tcc_sw_hue_table_set(lut_cmd.sin_value, lut_cmd.cos_value, lut_cmd.saturation);
				mutex_unlock(&lut_drv_data->hue_table_lock);	
			}
		
			return 0;

		case TCC_LUT_SET:
#if defined(CONFIG_DAUDIO)
			return tcc_lut_set(argp);
#else
			{
				struct VIOC_LUT_VALUE_SET lut_cmd;
				VIOC_LUT *pLUT =(VIOC_LUT*)tcc_p2v(HwVIOC_LUT);

				if(copy_from_user((void *)&lut_cmd, (const void *)argp, sizeof(lut_cmd)))
					return -EFAULT;

				VIOC_LUT_Set_value(pLUT, lut_cmd.lut_number, lut_cmd.Gamma);
			}

			return 0;
#endif
		case TCC_LUT_PLUG_IN:
			{
				struct VIOC_LUT_PLUG_IN_SET lut_cmd;
				VIOC_LUT *pLUT =(VIOC_LUT*)tcc_p2v(HwVIOC_LUT);
				
				if(copy_from_user((void *)&lut_cmd, (const void *)arg, sizeof(lut_cmd)))
					return -EFAULT;
				
				if(!lut_cmd.enable) {
					VIOC_LUT_Enable(pLUT, lut_cmd.lut_number, FALSE);
				}
				else 	{
					VIOC_LUT_Plugin(pLUT,  lut_cmd.lut_number, lut_cmd.lut_plug_in_ch);
					VIOC_LUT_Enable(pLUT, lut_cmd.lut_number, lut_cmd.enable);
				}
			}
			return 0;

		default:
			printk(KERN_ALERT "not supported LUT IOCTL(0x%x). \n", cmd);
			break;			
	}

	return 0;
}
EXPORT_SYMBOL(tcc_lut_ioctl);

int tcc_lut_release(struct inode *inode, struct file *filp)
{
	LUT_DRV_DATA_T *lut_drv_data = (LUT_DRV_DATA_T *)filp->private_data;
	
	if(lut_drv_data->dev_opened > 0) 	
	        lut_drv_data->dev_opened--;
	
	dprintk("lut_release:  %d'th. \n", lut_drv_data->dev_opened);
	return 0;
}
EXPORT_SYMBOL(tcc_lut_release);

int tcc_lut_open(struct inode *inode, struct file *filp)
{	
	int ret = 0;

	lut_drv_data.dev_opened++;
	filp->private_data = &lut_drv_data;
	
	printk("lut_open :  %d'th. \n", lut_drv_data.dev_opened);
	return ret;	
}

#define	DEVICE_NAME		"tcc_lut"
#define LUT_MINOR		233		/* Major 10, Minor 243, /dev/lut */
static char banner[] =	KERN_INFO "2013/09/24 ver TCC LUT Driver Initializing. \n";

static const struct file_operations lut_fops =
{
    .owner   = THIS_MODULE,
    .open    = tcc_lut_open,
    .release = tcc_lut_release,
    .unlocked_ioctl = tcc_lut_ioctl,
};

static struct miscdevice tcc_lut_misc_device =
{
    LUT_MINOR,
    DEVICE_NAME,
    &lut_fops,
};

static int lut_drv_probe(struct platform_device *pdev)
{
	unsigned int reg;

	printk(banner);

	if (misc_register(&tcc_lut_misc_device)) {
	    printk(KERN_WARNING "HPD: Couldn't register device 10, %d.\n", LUT_MINOR);
	    return -EBUSY;
	}

	memset(&lut_drv_data, 0, sizeof(LUT_DRV_DATA_T));
	mutex_init(&(lut_drv_data.hue_table_lock));

	return 0;
}

static int lut_drv_remove(struct platform_device *pdev)
{
	misc_deregister(&tcc_lut_misc_device);
	return 0;
}


/* suspend and resume support for the lcd controller */
static int lut_drv_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}

static int lut_drv_resume(struct platform_device *dev)
{
	VIOC_DMA_LUT_restore();
	return 0;
}

static struct platform_driver tcc_lut_driver = {
	.probe		= lut_drv_probe,
	.remove		= lut_drv_remove,
	.suspend	= lut_drv_suspend,
	.resume		= lut_drv_resume,
	.driver		= {
		.name	= "tcc_lut",
		.owner	= THIS_MODULE,
	},
};

struct platform_device tcc_lut_device = {
	.name	= "tcc_lut",
};

static int lut_drv_init(void)
{
	platform_device_register(&tcc_lut_device);
	return platform_driver_register(&tcc_lut_driver);
}

static __exit void lut_drv_exit(void)
{
	platform_driver_unregister(&tcc_lut_driver);
}

module_init(lut_drv_init);
module_exit(lut_drv_exit);

MODULE_AUTHOR("Telechips.");
MODULE_DESCRIPTION("TCC LUT Driver");
MODULE_LICENSE("GPL");
