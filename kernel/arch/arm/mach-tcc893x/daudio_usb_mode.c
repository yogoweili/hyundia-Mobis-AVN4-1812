/*
 * Copyright (C) 2013, Telechips, Inc.
 * Copyright 2013 Cleinsoft.
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * C1913, Intellige, 177, Jungjailro, Bundang, Sungnam, Kyounggi, Korea.
 */

#include "daudio_usb_mode.h"

#define OTG_HOST    1
#define OTG_DEVICE  0

#define DEBUG       1

#if (DEBUG)
#define VPRINTK(fmt, args...) printk(__FILE__ " " fmt, ##args)
#else
#define VPRINTK(args...) do {} while(0)
#endif

#define DRIVER_AUTHOR "Valky"
#define DRIVER_DESC "D-Audio USB3.0 ID irq manage."

//USB MODE
#if defined(CONFIG_DAUDIO_20140220)
	#define GPIO_USB_MODE		TCC_GPB(4)
#endif

//USB30_5V_CTL
#if defined(CONFIG_DAUDIO_20140220)
	#define GPIO_USB30_5V_CTL	TCC_GPD(11)
#endif

struct platform_device *pdev;
spinlock_t lock;

int usb_mode;
static int set_usb_irq_flag = 0;

struct usb3_data {
	struct work_struct	wq;
	unsigned long		flag;
};
struct usb3_data *usb_mode_wq;

extern void tcc_usb30_phy_on(void);
extern int tcc_usb30_clkset(int on);
extern void msleep(unsigned int msecs);

int vbus_ctrl(int on)
{
	gpio_request(GPIO_USB30_5V_CTL, "USB30_5V_CTL");
	gpio_direction_output(GPIO_USB30_5V_CTL, (on) ? 1 : 0);

	return 0;
}

static void usb_mode_change(struct work_struct *work)
{
	int retval = 0;
	struct usb3_data *p =  container_of(work, struct usb3_data, wq);
	unsigned long flag = p->flag;

	struct subprocess_info *set_property;
	static char *envp[] = {NULL};
	char *argv_property[] = {"/system/bin/setprop", "sys.usb.mode", flag ? "usb_device" : "usb_host", NULL};
	#if defined(GPIO_USB_MODE)
		//to Micom - gpio high = host , gpio low = device
		int usb_mode = flag ? OTG_DEVICE : OTG_HOST;
	#endif

	if (set_usb_irq_flag)
	{
		free_irq(INT_OTGID, pdev);
	}

	set_usb_irq_flag = 0;

	VPRINTK("%s\n", __func__);

	set_property = call_usermodehelper_setup(argv_property[0], argv_property, envp, GFP_ATOMIC);
	if (set_property == NULL)
	{
		VPRINTK("Error call_usermodehelper_freeinfo\n");
		retval = -ENOMEM;
	}
	else
	{
		retval = call_usermodehelper_exec(set_property, UMH_WAIT_PROC);
		#if defined(GPIO_USB_MODE) && defined(GPIO_USB30_5V_CTL)
			gpio_set_value(GPIO_USB_MODE, usb_mode);
		#endif
		#if defined(GPIO_USB30_5V_CTL)
			vbus_ctrl(usb_mode);
		#endif
	}

	if (flag)
	{
		VPRINTK("USB DEVICE mode\n");

		retval = request_irq(INT_OTGID, &usb_mode_host_irq,
				IRQF_SHARED|IRQ_TYPE_EDGE_FALLING, "USB30_IRQ5", pdev);
		if (retval)
		{
			VPRINTK("failed to request falling edge\n");
			retval = -EBUSY;
		}
		else
		{
			usb_mode = OTG_DEVICE;
			set_usb_irq_flag = 1;
		}
	}
	else
	{
		VPRINTK("USB HOST mode\n");

		retval = request_irq(INT_OTGID, &usb_mode_device_irq,
				IRQF_SHARED|IRQ_TYPE_EDGE_RISING, "USB30_IRQ11", pdev);
		if (retval)
		{
			VPRINTK("failed to request rising edge\n");
			retval = -EBUSY;
		}
		else
		{
			usb_mode = OTG_HOST;
			set_usb_irq_flag = 1;
		}
	}

	return;
}

static irqreturn_t usb_mode_host_irq(int irq, struct platform_device *pdev)
{
	VPRINTK("%s\n", __func__);

	spin_lock(&lock);
	usb_mode_wq->flag = 0;
	schedule_work(&usb_mode_wq->wq);
	spin_unlock(&lock);

	return IRQ_HANDLED;
}

static irqreturn_t usb_mode_device_irq(int irq, struct platform_device *pdev)
{
	VPRINTK("%s\n", __func__);

	spin_lock(&lock);
	usb_mode_wq->flag = 1;
	schedule_work(&usb_mode_wq->wq);
	spin_unlock(&lock);

	return IRQ_HANDLED;
}

static int usb_mode_probe(struct platform_device *dev)
{
	int retval = 0;
	unsigned int uTmp;

	VPRINTK("%s\n", __func__);

	spin_lock_init(&lock);
	pdev = dev;

	usb_mode_wq = (struct usb3_data *)kzalloc(sizeof(struct usb3_data), GFP_KERNEL);
	if (usb_mode_wq == NULL)
	{
		retval = -ENOMEM;
	}

	INIT_WORK(&usb_mode_wq->wq, usb_mode_change);
	retval = request_irq(INT_OTGID, &usb_mode_device_irq,
			IRQF_SHARED|IRQ_TYPE_EDGE_RISING, "USB30_IRQ", pdev);

	if (retval)
	{
		dev_err(&pdev->dev, "request rising edge of irq%d failed!\n", INT_OTGID);
		retval = -EBUSY;
		goto fail_usb_mode_change;
	}
	else
	{
		PUSBPHYCFG USBPHYCFG = (PUSBPHYCFG)tcc_p2v(HwUSBPHYCFG_BASE);
		usb_mode = OTG_HOST;
		set_usb_irq_flag = 1;

		msleep(100);
		tcc_usb30_clkset(1);
		tcc_usb30_phy_on();

		// Initialize All PHY & LINK CFG Registers
		USBPHYCFG->UPCR0 = 0xB5484068;
		USBPHYCFG->UPCR1 = 0x0000041C;
		USBPHYCFG->UPCR2 = 0x919E14C8;
		USBPHYCFG->UPCR3 = 0x4AB05D00;
		USBPHYCFG->UPCR4 = 0x00000000;
		USBPHYCFG->UPCR5 = 0x00108001;
		USBPHYCFG->LCFG  = 0x80420013;

		BITSET(USBPHYCFG->UPCR4, Hw20|Hw21); //ID pin interrupt enable

		USBPHYCFG->UPCR1 = 0x00000412;
		USBPHYCFG->LCFG  = 0x86420013;

		BITCLR(USBPHYCFG->LCFG, Hw30);
		BITCLR(USBPHYCFG->UPCR1,Hw9);

		while(1) {
			uTmp = readl(tcc_p2v(HwUSBDEVICE_BASE+0x164));
			uTmp = (uTmp>>18)&0xF;

			if (uTmp != 0){
				break;
			}
		}
	}

	return retval;

fail_usb_mode_change:
	VPRINTK("fail_usb_mode_change.\n");

	return retval;
}

static int usb_mode_remove(struct platform_device *dev)
{
	VPRINTK("%s\n", __func__);

	return 0;
}

static int usb_mode_suspend(struct platform_device *dev, pm_message_t pm)
{
	PUSBPHYCFG USBPHYCFG = (PUSBPHYCFG)tcc_p2v(HwUSBPHYCFG_BASE);

	VPRINTK("%s\n", __func__);
	if (set_usb_irq_flag)
		free_irq(INT_OTGID, pdev);

	set_usb_irq_flag = 0;
	BITCLR(USBPHYCFG->UPCR4, Hw20|Hw21);

	return 0;
}

static int usb_mode_resume(struct platform_device *dev)
{
	VPRINTK("%s\n", __func__);

	spin_lock(&lock);
	usb_mode_wq->flag = 0;
	schedule_work(&usb_mode_wq->wq);
	spin_unlock(&lock);

	return 0;
}

struct platform_device usb_mode_device =
{
	.name		= "daudio_usb3_mode",
	.id			= -1,
};

static struct platform_driver usb_mode_driver =
{
	.probe		= usb_mode_probe,
	.remove		= usb_mode_remove,
	.suspend	= usb_mode_suspend,
	.resume		= usb_mode_resume,
	.driver		=
	{
		.name	= (char *) "daudio_usb3_mode",
		.owner	= THIS_MODULE,
	}
};

static int __init usb_mode_init(void)
{
	int ret = platform_device_register(&usb_mode_device);

	VPRINTK("%s ret: %d\n", __func__, ret);
	if (ret < 0)
	{
		VPRINTK("%s platform_device_register failed!\n", __func__);
	}

	ret = platform_driver_register(&usb_mode_driver);
	if (ret < 0)
	{
		VPRINTK("%s platform_driver_register failed!\n", __func__);
	}
	return ret;
}

module_init(usb_mode_init);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR (DRIVER_AUTHOR);
MODULE_LICENSE ("GPL");
