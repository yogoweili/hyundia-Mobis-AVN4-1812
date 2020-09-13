#include "cmmb_control.h"

#include <linux/delay.h>

#include <mach/daudio.h>
#include <mach/daudio_debug.h>
#include <mach/daudio_pinctl.h>
#include <mach/gpio.h>

static int debug_cmmb = DEBUG_DAUDIO_CMMB;

#define INFO(fmt, args...) if (debug_cmmb) printk(KERN_INFO TAG_DAUDIO_CMMB fmt, ##args)

int cmmb_reset(void)
{
	INFO("%s\n", __func__);

	cmmb_reset_value(CMMB_RESET_LOW);
	mdelay(CMMB_RESET_DURATION);
	cmmb_reset_value(CMMB_RESET_HIGH);

	return 0;
}

int cmmb_reset_value(int value)
{
	unsigned cmmb_reset_gpio = get_gpio_number(CTL_XM_SIRIUS_RESET);
	INFO("%s gpio: 0x%x\n", __func__, cmmb_reset_gpio);
	
	if ( cmmb_reset_gpio < 0) {
		return 0;
	}

	gpio_set_value(cmmb_reset_gpio, value);

	return 0;
}
