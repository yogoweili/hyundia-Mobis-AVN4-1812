#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>

#include <asm/io.h>
#include <asm/mach-types.h>

#include <mach/bsp.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/kdev_t.h>

#include <mach/reg_physical.h>
#include <mach/structures_hsio.h>
#include <mach/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>

static irqreturn_t usb_mode_host_irq(int irq, struct platform_device *pdev);
static irqreturn_t usb_mode_device_irq(int irq, struct platform_device *pdev);
static void usb_mode_change(struct work_struct *work);
