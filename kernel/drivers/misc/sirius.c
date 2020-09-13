#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/platform_device.h>

#include <mach/gpio.h>
#include <mach/daudio.h>

#define DEBUG   1

#if (DEBUG)
#define VPRINTK(fmt, args...) printk(KERN_INFO "[SIRIUS] " fmt, ##args)
#else
#define VPRINTK(args...) do {} while(0)
#endif

#define SIRIUS_POWER_DISABLED	0
#define SIRIUS_POWER_ENABLED	1
#define SIRIUS_FAIL				300

#define SIRIUS_CMD_RESET		31
#define SIRIUS_CMD_POWER_ENABLE	32
#define SIRIUS_CMD_GET_STATUS	41

static DEFINE_MUTEX(sirius_lock);

static int power_enabled = 0;

/* Module Power, Enable and Reset Timing
 *
 *                Startup timing                           Shutdown Timing
 *
 * 5V +/- 5% ____------------------------- ... | ... ------------------------_____
 *               <- 50ms ->                                        <- 50ms ->
 * ENABLE    ______________--------------- ... | ... --------------_______________
 *                         <- 250ms ->     ... | ...     <- 50ms ->
 * RESET     _________________________---- ... | ... ----_________________________
 */

static void sirius_reset(void)
{
	VPRINTK("%s power_enabled: %d\n", __func__, power_enabled);

	if (power_enabled)
	{
		gpio_set_value(SIRIUS_GPIO_RESET, 0);
		msleep(SIRIUS_RESET_TIME);
		gpio_set_value(SIRIUS_GPIO_RESET, 1);
	}
}

static void set_power_enable(int enable)
{
	VPRINTK("%s enable: %d\n", __func__, enable);

	if (enable == SIRIUS_POWER_ENABLED)
	{
		//power enable
		msleep(SIRIUS_RESET_TIME);					//sleep 50ms
		gpio_set_value(SIRIUS_GPIO_SHDN, 1);		//enable high
		msleep(SIRIUS_ENABLE_TIME);					//sleep 250ms
		gpio_set_value(SIRIUS_GPIO_RESET, 1);		//reset high
		power_enabled = 1;
	}
	else if (enable == SIRIUS_POWER_DISABLED)
	{
		//power disable
		gpio_set_value(SIRIUS_GPIO_RESET, 0);		//reset low
		msleep(SIRIUS_RESET_TIME);					//sleep 50ms
		gpio_set_value(SIRIUS_GPIO_SHDN, 0);		//enable low
		power_enabled = 0;
	}
}

static int get_power_enabled(void)
{
	return power_enabled;
}

static long sirius_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	const int enable = (int)arg;
	VPRINTK("%s cmd: %d, arg: %ld\n", __func__, cmd, arg);

	mutex_lock(&sirius_lock);

	switch (cmd)
	{
		case SIRIUS_CMD_RESET:
			sirius_reset();
			break;

		case SIRIUS_CMD_POWER_ENABLE:
			if ((enable == SIRIUS_POWER_ENABLED) || (enable == SIRIUS_POWER_DISABLED))
				set_power_enable(enable);
			else
				ret = SIRIUS_FAIL;
			break;

		case SIRIUS_CMD_GET_STATUS:
			ret = get_power_enabled();
			break;

		default:
			ret = SIRIUS_FAIL;
			break;
	}

	mutex_unlock(&sirius_lock);

	VPRINTK("%s cmd: %d, ret: %d\n", __func__, cmd, ret);

	return ret;
}

static int sirius_open(struct inode *inode, struct file *file)
{
	VPRINTK("%s\n", __func__);

	return 0;
}

static int sirius_release(struct inode *inode, struct file *file)
{
	VPRINTK("%s\n", __func__);

	return 0;
}

int sirius_probe(struct platform_device *pdev)
{
	VPRINTK("%s\n", __func__);

	return 0;
}

int sirius_remove(struct platform_device *pdev)
{
	VPRINTK("%s\n", __func__);

	return 0;
}

int sirius_suspend(struct platform_device *pdev, pm_message_t state)
{
	VPRINTK("%s\n", __func__);

	return 0;
}

int sirius_resume(struct platform_device *pdev)
{
	VPRINTK("%s\n", __func__);

	return 0;
}

static struct file_operations sirius_fops =
{
	.owner			= THIS_MODULE,
	.unlocked_ioctl	= sirius_ioctl,
	.open			= sirius_open,
	.release		= sirius_release,
};

static struct class *sirius_class;
static dev_t dev;

static struct platform_driver sirius_driver = {
	.probe      = sirius_probe,
	.remove     = sirius_remove,
	.suspend	= sirius_suspend,
	.resume		= sirius_resume,
	.driver = {
		.name   = SIRIUS_PLATFORM_DRIVER_NAME,
		.owner  = THIS_MODULE,
	},
};

static int __init sirius_init(void)
{
	int ret = platform_driver_register(&sirius_driver);
	if (ret < 0)
	{
		VPRINTK("%s failed to platform_driver_register\n", __func__);
		return ret;
	}

	ret = register_chrdev(0, SIRIUS_CLASS_DRIVER_NAME, &sirius_fops);
	if (ret < 0)
	{
		VPRINTK("%s failed to register_chrdev\n", __func__);
		return ret;
	}

	sirius_class = class_create(THIS_MODULE, SIRIUS_CLASS_DRIVER_NAME);
	if (IS_ERR(sirius_class))
	{
		ret = PTR_ERR(sirius_class);
	}

	device_create(sirius_class, NULL, MKDEV(ret, 0), NULL, SIRIUS_CLASS_DRIVER_NAME);

	VPRINTK("%s ret: %d\n", __func__, ret);

	//GPIO init
	gpio_request(SIRIUS_GPIO_SHDN, "sirius_shutdown");
	gpio_request(SIRIUS_GPIO_RESET, "sirius_reset");

	return ret;
}

static void __exit sirius_exit(void)
{
	platform_driver_unregister(&sirius_driver);
	unregister_chrdev(dev, (char *)(MINOR(dev)));
}

module_init(sirius_init);
module_exit(sirius_exit);

MODULE_AUTHOR("Cleinsoft.");
MODULE_DESCRIPTION("DAUDIO SIRIUS control driver");
