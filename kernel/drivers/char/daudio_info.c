#include <asm/io.h>
#include <asm/setup.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/swap.h>
#include <linux/uaccess.h>

#include <mach/daudio.h>
#include <mach/daudio_info.h>
#include <mach/daudio_settings.h>

#define DRIVER_NAME				"daudio_info"

#define DEBUG_DAUDIO_VERSION	1

#if (DEBUG_DAUDIO_VERSION)
#define VPRINTK(fmt, args...) printk(KERN_INFO "[cleinsoft Info] " fmt, ##args)
#else
#define VPRINTK(args...) do {} while(0)
#endif

static struct class *daudio_info_class;
static struct mutex info_lock;

static D_Audio_bsp_version	bsp_version;
static D_Audio_bsp_adc		bsp_adc;

int tag_lk_version = -1;

static char swsusp_header_sig[QB_SIG_SIZE];
static unsigned int swsusp_header_addr = 0;

int daudio_hw_version(void)
{
	return bsp_version.hw_version;
}

int daudio_main_version(void)
{
	return bsp_version.main_version;
}

int daudio_bt_version(void)
{
	return bsp_version.bt_version;
}

int daudio_gps_version(void)
{
	return bsp_version.gps_version;
}

int daudio_hw_adc(void)
{
	return bsp_adc.hw_adc;
}

int daudio_main_adc(void)
{
	return bsp_adc.main_adc;
}

int daudio_bt_adc(void)
{
	return bsp_adc.bt_adc;
}

int daudio_gsp_adc(void)
{
	return bsp_adc.gps_adc;
}

static int __init parse_tag_lk_version(const struct tag *tag)
{
	int *lk_version = (int *)(&tag->u);

	bsp_version.lk_version = *lk_version;
#if defined(CONFIG_DAUDIO_BSP_VERSION)
	bsp_version.kernel_version = CONFIG_DAUDIO_BSP_VERSION;
#endif

	VPRINTK("BSP LK ver: %d, Kernel ver: %d\n", bsp_version.lk_version, bsp_version.kernel_version);

	return 0;
}
__tagtable(ATAG_DAUDIO_LK_VERSION, parse_tag_lk_version);

/*
 * Structure of H/W version
 * temp (1byte) + HW version (1byte) + Platform version (1byte) + BT version (1byte)
 */
static int __init parse_tag_hw_version(const struct tag *tag)
{
	int *hw_version = (int *)(&tag->u);

	bsp_version.gps_version = (*hw_version & 0xFF000000) >> 24;
	bsp_version.hw_version = (*hw_version & 0x00FF0000) >> 16;
	bsp_version.main_version = (*hw_version & 0x0000FF00) >> 8;
	bsp_version.bt_version = *hw_version & 0x000000FF;

	VPRINTK("H/W HW ver: %d, Platform ver: %d, BT ver: %d, GPS ver: %d\n", bsp_version.hw_version, bsp_version.main_version, bsp_version.bt_version, bsp_version.gps_version);

	return 0;
}
__tagtable(ATAG_DAUDIO_HW_VERSION, parse_tag_hw_version);

static int __init parse_tag_quickboot_info(const struct tag *tag)
{
	struct quickboot_info *info = (struct quickboot_info *)(&tag->u);
	char temp_sig[QB_SIG_SIZE+1];

	swsusp_header_addr = info->addr;
	memcpy(swsusp_header_sig, info->sig, QB_SIG_SIZE);

	memset(temp_sig, 0, QB_SIG_SIZE+1);
	memcpy(temp_sig, swsusp_header_sig, QB_SIG_SIZE);

	VPRINTK("Quickboot info sig: [%s], addr: 0x%x\n", temp_sig, swsusp_header_addr);

	return 0;
}
__tagtable(ATAG_DAUDIO_QUICKBOOT_INFO, parse_tag_quickboot_info);

static int __init parse_tag_board_adc(const struct tag *tag)
{
	D_Audio_bsp_adc *info = (D_Audio_bsp_adc *)(&tag->u);

	bsp_adc.hw_adc = info->hw_adc;
	bsp_adc.main_adc = info->main_adc;
	bsp_adc.bt_adc = info->bt_adc;
	bsp_adc.gps_adc = info->gps_adc;

	VPRINTK("H/W HW adc: %d mv, Platform adc: %d mv, BT adc: %d mv, GPS adc: %d mv\n",
			bsp_adc.hw_adc, bsp_adc.main_adc, bsp_adc.bt_adc, bsp_adc.gps_adc);

	return 0;
}
__tagtable(ATAG_DAUDIO_BOARD_ADC, parse_tag_board_adc);
#if defined(QUICKBOOT_SIG)
#define HIBERNATE_SIG  QUICKBOOT_SIG
#else
#define HIBERNATE_SIG  "S1SUSPEND"
#endif

static ssize_t daudio_info_read(struct file *file, char *buf, size_t count, loff_t *offset)
{
	char snapshot_sig[QB_SIG_SIZE+1];
	char kernel_sig[QB_SIG_SIZE+1];

	memset(snapshot_sig, 0, QB_SIG_SIZE+1);
	memset(kernel_sig, 0, QB_SIG_SIZE+1);
	memcpy(snapshot_sig, swsusp_header_sig, QB_SIG_SIZE);
	get_hibernate_sig(kernel_sig);

	printk(KERN_INFO "BSP LK Version : %d\n", bsp_version.lk_version);
	printk(KERN_INFO "BSP Kerenl Version: %d\n", bsp_version.kernel_version);
	printk(KERN_INFO "H/W HW Version: %d, adc %d mv\n", bsp_version.hw_version, bsp_adc.hw_adc);
	printk(KERN_INFO "H/W Mainboard Version: %d, adc %d mv\n", bsp_version.main_version, bsp_adc.main_adc);
	printk(KERN_INFO "H/W BT Version: %d, adc: %d mv\n", bsp_version.bt_version, bsp_adc.bt_adc);
	printk(KERN_INFO "H/W GPS Version: %d, adc: %d mv\n", bsp_version.gps_version, bsp_adc.gps_adc);
	printk(KERN_INFO "Quickboot Kernel define sig: [%s], snapshot header sig: [%s]\n",
			kernel_sig, snapshot_sig);
	printk(KERN_INFO "Quickboot start addr: 0x%x\n", swsusp_header_addr);

	return 0;
}

static long daudio_info_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd)
	{
		case DAUDIO_BSP_VERSION_DET:
			{
				D_Audio_bsp_version bsp_ver;
				copy_from_user(&bsp_ver, (D_Audio_bsp_version*)arg, sizeof(D_Audio_bsp_version));
				bsp_ver.lk_version = bsp_version.lk_version;
				bsp_ver.kernel_version = bsp_version.kernel_version;
				bsp_ver.hw_version = bsp_version.hw_version;
				bsp_ver.main_version = bsp_version.main_version;
				bsp_ver.bt_version = bsp_version.bt_version;
				bsp_ver.gps_version = bsp_version.gps_version;
				copy_to_user((D_Audio_bsp_version*)arg, &bsp_ver, sizeof(D_Audio_bsp_version));
				ret = 1;
			}
			break;

		case DAUDIO_BSP_ADC_DET:
			{
				D_Audio_bsp_adc     hw_adc;
				copy_from_user(&hw_adc, (D_Audio_bsp_adc*)arg, sizeof(D_Audio_bsp_adc));
				hw_adc.hw_adc = bsp_adc.hw_adc;
				hw_adc.main_adc = bsp_adc.main_adc;
				hw_adc.bt_adc = bsp_adc.bt_adc;
				hw_adc.gps_adc = bsp_adc.gps_adc;
				copy_to_user((D_Audio_bsp_adc*)arg, &hw_adc, sizeof(D_Audio_bsp_adc));
				ret = 1;
			}
			break;
	}

	return ret;
}

static int daudio_info_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int daudio_info_release(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations daudio_info_fops =
{
	.owner          = THIS_MODULE,
	.unlocked_ioctl = daudio_info_ioctl,
	.open           = daudio_info_open,
	.release        = daudio_info_release,
	.read           = daudio_info_read,
};

static __init int daudio_info_init(void)
{
	int ret = register_chrdev(DAUDIO_INFO_MAJOR, DRIVER_NAME, &daudio_info_fops);
	VPRINTK("%s %d\n", __func__, ret);
	if (ret < 0)
	{
		printk(KERN_ERR "%s failed to register_chrdev\n", __func__);
		ret = -EIO;
		goto out;
	}

	daudio_info_class = class_create(THIS_MODULE, DRIVER_NAME);
	if (IS_ERR(daudio_info_class))
	{
		ret = PTR_ERR(daudio_info_class);
		goto out_chrdev;
	}

	device_create(daudio_info_class, NULL, MKDEV(DAUDIO_INFO_MAJOR, 0), NULL, DRIVER_NAME);
	mutex_init(&info_lock);
	goto out;

out_chrdev:
	unregister_chrdev(DAUDIO_INFO_MAJOR, DRIVER_NAME);
out:
	return ret;
}

static __exit void daudio_info_exit(void)
{
	device_destroy(daudio_info_class, MKDEV(DAUDIO_INFO_MAJOR, 0));
	class_destroy(daudio_info_class);
	unregister_chrdev(DAUDIO_INFO_MAJOR, DRIVER_NAME);
}

subsys_initcall(daudio_info_init);
module_exit(daudio_info_exit);

MODULE_AUTHOR("Cleinsoft");
MODULE_DESCRIPTION("D-Audio Info Driver");
MODULE_LICENSE("GPL");
