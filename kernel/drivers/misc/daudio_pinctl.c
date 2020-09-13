#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <mach/daudio.h>
#include <mach/daudio_debug.h>
#include <mach/daudio_info.h>
#include <mach/daudio_pinctl.h>
#include <mach/gpio.h>
#include <mach/hardware.h>

static int debug_pinctl = DEBUG_DAUDIO_PINCTL;

#define VPRINTK(fmt, args...) if (debug_pinctl) printk(KERN_INFO TAG_DAUDIO_PINCTL fmt, ##args)

#define PINCTL_FAIL				-1

static DEFINE_MUTEX(daudio_pinctl_lock);
static struct class *daudio_pinctl_class;

static int gpio_list_size = 0;

static D_AUDIO_GPIO_LIST *gpio_list;

static D_AUDIO_GPIO_LIST gpio_list_5th[] =
{
	{CTL_SD1_RESET,			TCC_GPA(19) ,	"CTL_SD1_RESET"},
	{CTL_CPU_BOOT_OK,		TCC_GPB(4)  ,	"CTL_CPU_BOOT_OK"},
	{DET_CDMA_BOOT_OK_OUT,	TCC_GPB(5)  ,	"DET_CDMA_BOOT_OK_OUT"},
	{DET_GPS_ANT_SHORT,		TCC_GPB(13) ,	"DET_GPS_ANT_SHORT"},
	{DET_GPS_PPS_INT,		TCC_GPB(14) ,	"DET_GPS_PPS_INT"},
	{CTL_GPS_RESET_N,		TCC_GPB(15) ,	"CTL_GPS_RESET_N"},
	{CTL_FM1288_RESET,		TCC_GPB(22) ,	"CTL_FM1288_RESET"},
	{DET_REVERSE_CPU,		TCC_GPC(29) ,	"DET_REVERSE_CPU"},
	{CTL_I2C_TOUCH_SDA,		TCC_GPC(30) ,	"DET_I2C_TOUCH_SDA"},
	{CTL_I2C_TOUCH_SCL,		TCC_GPC(31) ,	"DET_I2C_TOUCH_SCL"},
	{DET_SIRIUS_INT,		TCC_GPE(16) ,	"DET_SIRIUS_INT"},
	{CTL_USB30_5V_VBUS,		TCC_GPA(0) ,	"CTL_USB30_5V_VBUS - RESERVED"},
	{DET_XM_F_ACT,			TCC_GPD(12) ,	"DET_XM_F_ACT"},
	{CTL_TW8836_RESET,		TCC_GPF(18) ,	"CTL_TW8836_RESET"},
	{DET_GPS_ANT_OPEN,		TCC_GPD(21) ,	"DET_GPS_ANT_OPEN"},
	{DET_I2C_TOUCH_INT,		TCC_GPF(0)  ,	"DET_I2C_TOUCH_INT"},
	{CTL_TOUCH_RESET,		TCC_GPF(2)  ,	"CTL_TOUCH_RESET"},
	{CTL_XM_SHDN,			TCC_GPF(3)  ,	"CTL_XM_SHDN"},
	{CTL_XM_SIRIUS_RESET,	TCC_GPF(5)  ,	"CTL_XM_SIRIUS_RESET"},
	{CTL_WL_REG_ON,			TCC_GPF(9)  ,	"CTL_WL_REG_ON"},
	{CTL_GPS_BOOT_MODE,		TCC_GPF(11) ,	"CTL_GPS_BOOT_MODE"},
	{CTL_BT_REG_ON,			TCC_GPF(12) ,	"CTL_BT_REG_ON"},
	{CTL_USB20_PWR_EN,		TCC_GPF(17) ,	"CTL_USB20_PWR_EN"},
	{CTL_USB_PWR_EN,		TCC_GPF(19) ,	"CTL_USB_PWR_EN"},
	{CTL_UCS_M1,			TCC_GPA(0) ,	"CTL_UCS_M1 - RESERVED"},
	{CTL_UCS_M2,			TCC_GPA(0) ,	"CTL_UCS_M2 - RESERVED"},
	{DET_UCS_ALERT,			TCC_GPA(0) ,	"DET_UCS_ALERT - RESERVED"},
	{DET_UCS_DEV_DET,		TCC_GPF(24) ,	"DET_UCS_DEV_DET"},
	{CTL_UCS_EM_EN,			TCC_GPA(0) ,	"CTL_UCS_EM_EN - RESERVED"},
	{CTL_GPS_ANT_PWR_ON,	TCC_GPG(15) ,	"CTL_GPS_ANT_PWR_ON"},
	{CTL_DRM_RESET,			TCC_GPG(16) ,	"CTL_DRM_RESET"},
	{DET_BOARD_REV_2,		TCC_GPADC(2),	"DET_BOARD_REV_2"},
	{DET_BOARD_REV_3,		TCC_GPADC(3),	"DET_BOARD_REV_3"},
	{DET_BOARD_REV_4,		TCC_GPADC(4),	"DET_BOARD_REV_4"},
	{DET_BOARD_REV_5,		TCC_GPADC(5),	"DET_BOARD_REV_5"},
	{DET_UBS_CABLE_CHK,		TCC_GPA(0) ,	"DET_UBS_CABLE_CHK - RESERVED"},
	{CTL_USB_CABLE_DIG,		TCC_GPA(0) ,	"CTL_USB_CABLE_DIG - RESERVED"},
	{DET_M105_CPU_4_5V,		TCC_GPB(18) ,	"DET_M105_CPU_4.5V"},
	{DET_TOUCH_JIG,			TCC_GPB(30) ,	"DET_TOUCH_JIG"},

	//5th
	{DET_PMIC_I2C_IRQ,		TCC_GPC(20) ,	"DET_PMIC_I2C_IRQ"},
	{CTL_TW9912_RESET,		TCC_GPD(17) ,	"CTL_TW9912_RESET"},
	{DET_SDCARD_WP,			TCC_GPD(18) ,	"DET_SDCARD_WP"},
	{CTL_SDCARD_PWR_EN,		TCC_GPD(19) ,	"CTL_SDCARD_PWR_EN"},
	{DET_SDCARD_CD,			TCC_GPD(20) ,	"DET_SDCARD_CD"},
	{DET_SIRIUS_INT_CMMB,	TCC_GPE(16) ,	"DET_SIRIUS_INT_CMMB"},
	{DET_DMB_ANT_SHORT,		TCC_GPF(6) ,	"DET_DMB_ANT_SHORT"},
	{DET_DMB_ANT_OPEN,		TCC_GPF(7) ,	"DET_DMB_ANT_OPEN"},
	{CTL_DMB_ANT_SHDN,		TCC_GPF(8) ,	"CTL_DMB_ANT_SHDN"},
	{CTL_BT_RESET,			TCC_GPF(10) ,	"CTL_BT_RESET"},
	{DET_WL_HOST_WAKE_UP,	TCC_GPF(21) ,	"DET_WL_HOST_WAKE_UP"},
	{DET_M101_MICOM_PB12,	TCC_GPF(26) ,	"DET_M101_MICOM_PB12"},
	{DET_M101_MICOM_PD12,	TCC_GPF(27) ,	"DET_M101_MICOM_PD12"},
	{DET_M96_MICOM_PD11,	TCC_GPF(28) ,	"DET_M96_MICOM_PD11"},
	{CTL_M95_MICOM_PD10,	TCC_GPF(29) ,	"CTL_M95_MICOM_PD10"},
};

#define GPIO_BASE_ADDRESS		0x74200000
#define GPIO_REGS_STEP			0x40
#define GPIO_OUTPUT_ENABLE_REG	0x04
#define GPIO_OUTPUT_ENABLE		0x01
#define GPIO_REG(reg)			IO_ADDRESS(reg)

static int sizeof_list(int what)
{
	int ret = 0;

	switch (what)
	{
		case DAUDIO_BOARD_6TH:
		case DAUDIO_BOARD_5TH:
		default:
			ret = sizeof(gpio_list_5th) / sizeof(D_AUDIO_GPIO_LIST);
			break;
	}

	return ret;
}

static int init_list(void)
{
	int ret = 0;
	int main_ver = daudio_main_version();
	gpio_list_size = sizeof_list(main_ver);

	if (gpio_list_size > 0)
	{
		switch (main_ver)
		{
			case DAUDIO_BOARD_6TH:
			case DAUDIO_BOARD_5TH:
			default:
				gpio_list = gpio_list_5th;
				break;
		}
		if (gpio_list != 0)
		{
			ret = 1;
		}
	}

	return ret;
}

static int atoi(const char *name)
{
	int val = 0;
	for (;; name++)
	{
		switch (*name)
		{
			case '0'...'9':
				val = 10 * val + (*name - '0');
				break;
			default:
				return val;
		}
	}
	return val;
}

static void get_gpio_group_name(unsigned gpio, char *name)
{
	char group_name[11] = {'T', 'C', 'C', '_', 'G', 'P', ' ', ' ', ' ', ' ', };
	int group = gpio / 32;
	unsigned gpio_group_g = TCC_GPG(19) / 32;
	unsigned gpio_group_hdmi = TCC_GPHDMI(3) / 32;
	unsigned gpio_group_adc = TCC_GPADC(5) / 32;

	if (group >= 0 && group <= gpio_group_g)
	{
		group_name[6] = 'A' + group;
		group_name[10] = '\0';
	}
	else if (group == gpio_group_hdmi)
	{
		group_name[6] = 'H';
		group_name[7] = 'D';
		group_name[9] = 'M';
		group_name[9] = 'I';
		group_name[10] = '\0';
	}
	else if (group == gpio_group_adc)
	{
		group_name[6] = 'A';
		group_name[7] = 'D';
		group_name[8] = 'C';
		group_name[10] = '\0';
	}

	if (name != NULL)
		memcpy(name, group_name, 11);
	else
	{
		printk(KERN_ERR "%s name is NULL\n", __func__);
	}
}

/**
 * @return 0 is input, 1 is output
 **/
static int get_gpio_output_enable(unsigned gpio)
{
	int group = gpio / 32;
	int number = gpio % 32;
	int *output_enable = (int *)IO_ADDRESS(GPIO_BASE_ADDRESS +
				(group * GPIO_REGS_STEP) + GPIO_OUTPUT_ENABLE_REG);
	int ret_val = (*output_enable >> number) & GPIO_OUTPUT_ENABLE;
	return ret_val;
}

int get_gpio_number(unsigned gpio)
{
	int i;
	if (gpio >= 0)
	{
		for (i = 0; i < gpio_list_size; i++)
		{
			if (gpio_list[i].gpio_name == gpio)
				return gpio_list[i].gpio_number;
		}
	}
	return PINCTL_FAIL;
}

#if defined(INCLUDE_XM)
#define GPIO_UART4_TX TCC_GPD(13)
#define GPIO_UART4_RX TCC_GPD(14)

void daudio_uart_pinctl(bool value)
{
	if (value)
	{
		printk(KERN_INFO "Set UART4 as UART\n");
		tcc_gpio_config(GPIO_UART4_TX, GPIO_FN(4));
		tcc_gpio_config(GPIO_UART4_RX, GPIO_FN(4));
	}
	else
	{
	    printk(KERN_INFO "Set UART4 as GPIO\n");
	    tcc_gpio_config(GPIO_UART4_TX, GPIO_FN(0)|GPIO_OUTPUT|GPIO_LOW|GPIO_PULL_DISABLE);
		tcc_gpio_config(GPIO_UART4_RX, GPIO_FN(0)|GPIO_OUTPUT|GPIO_LOW|GPIO_PULL_DISABLE);
	}
}
#endif

static int daudio_pinctl_set_gpio(int gpio, int value)
{
	int ret = PINCTL_FAIL;

	if (gpio >= 0)
	{
   		if (gpio == TCC_GPF(29))
        {
        	printk(KERN_ERR "%s() Ignoring CTL_M95_MICOM_PD10 control\n", __FUNCTION__);
         	return ret;
		}
													        
		if (get_gpio_output_enable(gpio))
		{
#if defined(INCLUDE_XM)
            if ((gpio == TCC_GPF(3)) && (value == 0))
                daudio_uart_pinctl(0);
#endif
			gpio_set_value(gpio, value);
#if defined(INCLUDE_XM)
			if ((gpio == TCC_GPF(3)) && (value == 1))
				daudio_uart_pinctl(1);
#endif
			ret = 0;
		}
		else
		{
			//input setting
			#if 0	//not support
			int pull_status = 0;
			char gpio_group[11] = {' ',};
			get_gpio_group_name(gpio, gpio_group);

			pull_status = value ? GPIO_PULLUP : GPIO_PULLDOWN;
			gpio_request(gpio, gpio_group);
			tcc_gpio_config(gpio, GPIO_FN0 | GPIO_INPUT | pull_status);
			#endif
			printk(KERN_INFO "%s Not support input control.\n", __func__);
		}
	}

	return ret;
}

static int daudio_pinctl_get_gpio(int gpio)
{
	if (gpio >= 0)
		return gpio_get_value(gpio);
	else
		return PINCTL_FAIL;
}

static void print_usage(void)
{
	printk(KERN_INFO "echo <debug> > /dev/daudio_pinctl\n");
	printk(KERN_INFO "<debug> > 1:Enable, 0:Disable\n\n");
	printk(KERN_INFO "echo <gpio_num> <value> > /dev/daudio_pinctl\n");
	printk(KERN_INFO "<gpio_num> gpio index: [0 ~ %d]\n", gpio_list_size - 1);
	printk(KERN_INFO "<value> gpio value 1:High, 0:Low\n\n");
}

static ssize_t daudio_pinctl_read(struct file *file, char *buf, size_t count, loff_t *pos)
{
	int i;

	mutex_lock(&daudio_pinctl_lock);
	print_usage();
	printk(KERN_INFO "========== D-AUDIO GPIO FUNCTION DEBUG =========");
	printk(KERN_INFO "D-Audio PINCTL debug: %s\n",
			debug_pinctl ? "ENABLE" : "DISABLE");
	printk(KERN_INFO "========== D-AUDIO GPIO FUNCTION LIST ==========");
	printk(KERN_INFO " NUM    GPIO        DIRECTION VALUE DESCRIPTION");
	for (i = 0; i < gpio_list_size; i++)
	{
		int output_enable = get_gpio_output_enable(gpio_list[i].gpio_number);
		int number = gpio_list[i].gpio_number % 32;
		char *group_name = 0;
		char *direction = output_enable ? "OUTPUT" : "INPUT ";
		char *value = daudio_pinctl_get_gpio(gpio_list[i].gpio_number) ?
				"HIGH" : "LOW ";
		group_name = kmalloc(11, GFP_KERNEL);
		get_gpio_group_name(gpio_list[i].gpio_number, group_name);

		printk(KERN_INFO "[%2d] %s(%2d) %s    %s  %s\n",
				i, group_name, number, direction, value, gpio_list[i].gpio_description);
		kfree(group_name);
	}
	printk(KERN_INFO "\n");
	mutex_unlock(&daudio_pinctl_lock);

	return 0;
}

static ssize_t daudio_pinctl_write(struct file *file, const char *buf, size_t count, loff_t *pos)
{
	unsigned char *gpio;
	unsigned char *cmd;
	int gpio_name = -1;
	int gpio_cmd = -1;
	int gpio_number = -1;
	int i = 0;
	int seperator = -1;
	int ret = -1;

	mutex_lock(&daudio_pinctl_lock);

	for (; i < count; i++)
	{
		if (buf[i] == ' ' || buf[i] == '\0')
		{
			seperator = i;
			break;
		}
	}

	if (seperator == count || seperator < 0)
	{
		if (count <= 2)	//debug setting
		{
			int temp = atoi(buf);
			if (temp == 0) debug_pinctl = 0;
			else if (temp == 1) debug_pinctl = 1;
			printk(KERN_INFO "D-Audio GPIO Function setting: %s\n",
				debug_pinctl ? "Enable" : "Disable");
		}
		else
		{
			printk(KERN_ERR "%s args error.\n", __func__);
		}
		mutex_unlock(&daudio_pinctl_lock);
		return count;
	}

	gpio = kmalloc(seperator, GFP_KERNEL);
	cmd = kmalloc(count - (seperator + 1) - 1, GFP_KERNEL);

	memcpy(gpio, buf, seperator);
	memcpy(cmd, buf + seperator + 1, count - (seperator + 1) - 1);

	gpio_name = atoi(gpio);
	gpio_cmd = atoi(cmd);

#if 0	//debug
	printk(KERN_INFO "%s cmd: %s, size: %d\n", __func__, cmd,
			(count - (seperator + 1) - 1));

	printk(KERN_INFO "%s string: [%s] count: %d gpio: %d cmd: %d\n",
			__func__, buf, count, gpio_name, gpio_cmd);
#endif

	kfree(gpio);
	kfree(cmd);

	if (gpio_name >= 0 && gpio_name < gpio_list_size)
		gpio_number = gpio_list[gpio_name].gpio_number;
	else
		gpio_number = -1;

	if (gpio_number >= 0)
		ret = daudio_pinctl_set_gpio(gpio_number, gpio_cmd);
	else
	{
		ret = -1;
		printk(KERN_ERR "%s failed daudio_pinctl_set_gpio\n", __func__);
	}

	if (ret == 0)
	{
		char *value = gpio_cmd > 0 ? "HIGH" : "LOW";
		char *group = kmalloc(11, GFP_KERNEL);
		get_gpio_group_name(gpio_number, group);

		printk(KERN_INFO "%s  %s(%d), value: %s\n", __func__,
				group, gpio_number % 32, value);
		kfree(group);
	}

	mutex_unlock(&daudio_pinctl_lock);

	return count;
}

static long daudio_pinctl_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct D_Audio_pinioc_cmddata *data;
	char *value = 0;
	int gpio_group;
	int gpio_num;
	int gpio_number;

	mutex_lock(&daudio_pinctl_lock);

	if (cmd < 0)
	{
		ret = PINCTL_FAIL;
		printk(KERN_ERR "%s cmd < 0\n", __func__);
	}
	else
	{
		data = (struct D_Audio_pinioc_cmddata *)arg;
		gpio_number = get_gpio_number(data->pin_name);

		if (gpio_number < 0)
		{
			printk(KERN_ERR "%s gpio_number < 0\n", __func__);
			mutex_unlock(&daudio_pinctl_lock);
			return PINCTL_FAIL;
		}

		switch (cmd)
		{
			case DAUDIO_PINCTL_CTL:
#if 0
				printk(KERN_INFO "%s DAUDIO_PINCTL_CTL pin:%d value:%d\n",
						 __func__, gpio_number, data->pin_value);
#endif
				ret = daudio_pinctl_set_gpio(gpio_number, data->pin_value);
				break;

			case DAUDIO_PINCTL_DET:
#if 0
				printk(KERN_INFO "%s DAUDIO_PINCTL_DET\n", __func__);
#endif
				data->pin_value = daudio_pinctl_get_gpio(gpio_number);
				if (data->pin_value != 0 && data->pin_value != 1)
					ret = PINCTL_FAIL;
				break;

			default:	//TODO Delete below code later - valky
				ret = daudio_pinctl_set_gpio(data->pin_name, data->pin_value);
				break;
		}

		if (ret == 0 && debug_pinctl)
		{
			char *group = 0;
			value = data->pin_value > 0 ? "HIGH" : "LOW";
			group = kmalloc(11, GFP_KERNEL);

			if (cmd == DAUDIO_PINCTL_CTL || cmd == DAUDIO_PINCTL_DET)
			{
				gpio_group = gpio_number / 32;
				gpio_num = gpio_number % 32;
				get_gpio_group_name(gpio_number, group);
			}
			else
			{
				gpio_group = data->pin_name / 32;
				gpio_num = data->pin_name % 32;
				get_gpio_group_name(data->pin_name, group);
			}

			if (data->pin_name != DET_REVERSE_CPU)	//reverse signal check
				VPRINTK("%s  %s(%d), value: %s\n", __func__, group, gpio_num, value);

			kfree(group);
		}
	}

	mutex_unlock(&daudio_pinctl_lock);

	return ret;
}

static int daudio_pinctl_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int daudio_pinctl_release(struct inode *inode, struct file *file)
{
	return 0;
}

int daudio_pinctl_probe(struct platform_device *pdev)
{
	VPRINTK("%s\n", __func__);

	return 0;
}

int daudio_pinctl_remove(struct platform_device *pdev)
{
	VPRINTK("%s\n", __func__);

	return 0;
}

int daudio_pinctl_suspend(struct platform_device *pdev, pm_message_t state)
{
	VPRINTK("%s\n", __func__);

	return 0;
}

int daudio_pinctl_resume(struct platform_device *pdev)
{
	VPRINTK("%s\n", __func__);

	return 0;
}

static struct file_operations daudio_pinctl_fops =
{
	.owner			= THIS_MODULE,
	.unlocked_ioctl	= daudio_pinctl_ioctl,
	.read			= daudio_pinctl_read,
	.write			= daudio_pinctl_write,
	.open			= daudio_pinctl_open,
	.release		= daudio_pinctl_release,
};

static int __init daudio_pinctl_init(void)
{
	int ret = register_chrdev(DAUDIO_PINCTL_MAJOR, PINCTL_DRIVER_NAME, &daudio_pinctl_fops);
	if (ret < 0)
	{
		VPRINTK("%s failed to register_chrdev\n", __func__);
		ret = -EIO;
		goto out;
	}

	daudio_pinctl_class = class_create(THIS_MODULE, PINCTL_DRIVER_NAME);
	if (IS_ERR(daudio_pinctl_class))
	{
		ret = PTR_ERR(daudio_pinctl_class);
		goto out_chrdev;
	}

	device_create(daudio_pinctl_class, NULL, MKDEV(DAUDIO_PINCTL_MAJOR, 0),
			NULL, PINCTL_DRIVER_NAME);
	VPRINTK("%s %d\n", __func__, ret);

	if (init_list() > 0)
		goto out;

out_chrdev:
	unregister_chrdev(DAUDIO_PINCTL_MAJOR, PINCTL_DRIVER_NAME);
out:
	return ret;
}

static void __exit daudio_pinctl_exit(void)
{
	device_destroy(daudio_pinctl_class, MKDEV(DAUDIO_PINCTL_MAJOR, 0));
	class_destroy(daudio_pinctl_class);
	unregister_chrdev(DAUDIO_PINCTL_MAJOR, PINCTL_DRIVER_NAME);
}

module_init(daudio_pinctl_init);
module_exit(daudio_pinctl_exit);

MODULE_AUTHOR("Cleinsoft.");
MODULE_DESCRIPTION("DAUDIO Pin control driver");
