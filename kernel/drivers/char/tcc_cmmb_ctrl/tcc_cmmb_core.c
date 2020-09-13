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

#include <linux/module.h>
#include <linux/delay.h>
#include <mach/io.h>
#include <linux/gpio.h>
#include <mach/bsp.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <asm/mach-types.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/platform_device.h>

#include <mach/tcc_cmmb_ioctl.h>
#include "tcc_cmmb_video_interface.h"
#include "tcc_cmmb_core.h"

#if defined(CONFIG_DAUDIO)
#include <mach/daudio.h>
#include "cmmb_control.h"
#endif

static int debug = 1;
#define dprintk(msg...) if (debug) { printk(KERN_INFO "TCC_CMMB:   " msg); }

//DECLARE_WAIT_QUEUE_HEAD(cmmb_wq);

static struct device *pDevCmmb = NULL;
static struct clk *pViocClk = NULL;
static char device_opened = 0;

static int tcc_cmmb_core_open(struct inode *inode, struct file *filp)
{
	int ret =0;

	dprintk("%s()   \n", __func__);

	clk_enable(pViocClk);
	if (!device_opened) {
		tcc_cmmb_video_init_vioc_path();
		#if !defined(FEATURE_CMMB_DISPLAY_TEST)
			if ((ret = request_irq(INT_VIOC_WD7, tcc_cmmb_video_irq_handler, IRQF_SHARED, "cmmb", NULL)) < 0) {
				printk(KERN_ERR "FAILED to aquire CMMB IRQ(%d). \n", ret);
				return ret;
			}
		#endif
	}
	device_opened++;

#if defined(FEATURE_CMMB_DISPLAY_TEST)
	device_opened++;
#endif

	return ret;
}

static int tcc_cmmb_core_release(struct inode *inode, struct file *filp)
{
	dprintk("%s()   \n", __func__);

	if (device_opened > 0) {
		device_opened--;
	}

	if (device_opened == 0) {
		#if !defined(FEATURE_CMMB_DISPLAY_TEST)
			free_irq(INT_VIOC_WD7, NULL);
		#endif
		clk_disable(pViocClk);
	}

	return 0;
}

static long tcc_cmmb_core_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	CMMB_PREVIEW_IMAGE_INFO cmmb_info;
	CMMB_IMAGE_BUFFER_INFO buf_info;
	CMMB_RESET_INFO reset_info;

	dprintk("%s()   cmd = 0x%x. \n", __func__, cmd);
	switch (cmd) {
		case IOCTL_TCC_CMMB_VIDEO_START:
		#if defined(FEATURE_CMMB_DISPLAY_TEST)
			if (0) {
		#else
			if (copy_from_user(&cmmb_info, (CMMB_PREVIEW_IMAGE_INFO *)arg, sizeof(CMMB_PREVIEW_IMAGE_INFO))) {
		#endif
				printk(KERN_ALERT "%s():  Not Supported copy_from_user(%d). \n", __func__, cmd);
				ret = -EFAULT;
			}
			ret = tcc_cmmb_video_interface_start_stream(&cmmb_info);
			break;

		case IOCTL_TCC_CMMB_VIDEO_STOP:
			ret = tcc_cmmb_video_interface_stop_stream();
			break;

		case IOCTL_TCC_CMMB_VIDEO_SET_ADDR:
			if (copy_from_user(&buf_info, (CMMB_IMAGE_BUFFER_INFO *)arg, sizeof(CMMB_IMAGE_BUFFER_INFO))) {
				printk(KERN_ALERT "%s():  Not Supported copy_from_user(%d). \n", __func__, cmd);
				ret = -EFAULT;
			}
			ret = tcc_cmmb_video_set_buffer_addr(&buf_info);
			break;

		case IOCTL_TCC_CMMB_VIDEO_QBUF:
			ret = tcc_cmmb_video_set_qbuf();
			break;

		case IOCTL_TCC_CMMB_VIDEO_DQBUF:
			ret = tcc_cmmb_video_get_dqbuf();
			break;

#if defined(CONFIG_DAUDIO)
		case DAUDIO_CMMBIOC_RESET:
			ret = cmmb_reset();
			break;

		case DAUDIO_CMMBIOC_RESET_CONTROL:
			if (copy_from_user(&reset_info, (CMMB_RESET_INFO *)arg, sizeof(CMMB_RESET_INFO))) {
				printk(KERN_ALERT "%s(): Not Supported copy_from_user(%d). \n", __func__, cmd);
				ret = -EINVAL;
			} else if (CMMB_RESET_LOW == reset_info.value || CMMB_RESET_HIGH == reset_info.value) {
				ret = cmmb_reset_value(reset_info.value);
			} else
				ret = -EINVAL;
			break;
#endif

		default:
			dprintk("%s()   tcc_cmmb ::   unrecognized ioctl (0x%x). \n", __func__, cmd);
			ret = -EINVAL;
			break;
	}

	return ret;
}

struct file_operations tcc_cmmb_core_fops = {
	.owner = THIS_MODULE,
	.open = tcc_cmmb_core_open,
	.release = tcc_cmmb_core_release,
	.unlocked_ioctl = tcc_cmmb_core_ioctl,
};

static struct class *tcc_cmmb_class;

static int tcc_cmmb_core_probe(struct platform_device *pdev)
{
	int ret = 0;
	printk(KERN_INFO "%s\n", __func__);

	ret = register_chrdev(TCC_CMMB_DEVICE_MAJOR, TCC_CMMB_DEVICE_NAME, &tcc_cmmb_core_fops);
	if (ret < 0) {
		dprintk("%s()   char_device rigister fail(%d). \n", __func__, ret);
		return ret;
	}

	tcc_cmmb_class = class_create(THIS_MODULE, TCC_CMMB_DEVICE_NAME);
	if (NULL == device_create(tcc_cmmb_class, NULL, MKDEV(TCC_CMMB_DEVICE_MAJOR,
			TCC_CMMB_DEVICE_MINOR), NULL, TCC_CMMB_DEVICE_NAME)) {
		dprintk("%s()   char_device create failed. \n", __func__);
		return -1;
	}

	pDevCmmb = &pdev->dev;
	pViocClk = clk_get(NULL, "lcdc");
	BUG_ON(pViocClk == NULL);

	dprintk("%s()   char_device probe OK....... \n", __func__);

	return 0;
}

static int tcc_cmmb_core_remove(struct platform_device *pdev)
{
	device_destroy(tcc_cmmb_class, MKDEV(TCC_CMMB_DEVICE_MAJOR, TCC_CMMB_DEVICE_MINOR));
	class_destroy(tcc_cmmb_class);
	unregister_chrdev(TCC_CMMB_DEVICE_MAJOR, TCC_CMMB_DEVICE_NAME);

	return 0;
}

static struct platform_driver tcc_cmmb_driver = {
	.probe = tcc_cmmb_core_probe,
	.remove = tcc_cmmb_core_remove,
	.driver = {
		.name = TCC_CMMB_DEVICE_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init tcc_cmmb_core_init(void)
{
	int ret = platform_driver_register(&tcc_cmmb_driver);
	dprintk("%s ret:%d\n", __func__, ret);
	return ret;
}

static void __exit tcc_cmmb_core_exit(void)
{
	platform_driver_unregister(&tcc_cmmb_driver);
}

module_init(tcc_cmmb_core_init);
module_exit(tcc_cmmb_core_exit);

MODULE_AUTHOR("Telechips Inc.");
MODULE_DESCRIPTION("Support to TCC893x CMMB Video Interface");
MODULE_LICENSE("GPL");
