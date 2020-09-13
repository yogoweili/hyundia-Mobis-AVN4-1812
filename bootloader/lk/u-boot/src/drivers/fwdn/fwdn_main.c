
#include <common.h>
#include <asm/arch/usb/usb_manager.h>
#include <asm/arch/reg_physical.h>
int fwdn_connect = 0;

#if 0
#include "debug.h"
#include <debug.h>
#include <string.h>
#include <stdlib.h>
#include <kernel/thread.h>
#include <kernel/event.h>
#include <dev/udc.h>
#endif


extern char FirmwareVersion[];

int check_fwdn_mode(void)
{
	if ( FirmwareVersion[3] != '?')
		return 0;
	else
		return 1;
}

void fwdn_start(void)
{
	printf("\x1b[1;33mFWDN start\x1b[1;0m\n");

	fwdn_connect = 1;

#if defined(CONFIG_TCC893X) || defined(CONFIG_TCC892X)
	USB_Init(USBDEV_VTC);
#else
#error
#endif

	USB_DEVICE_On();

	#if 1
	for(;;)
	{
#if defined(CONFIG_TCC893X) && defined(TCC_USB_30_USE)
		USB_Polling_Isr();
#else
		#if !defined(FEATURE_USB_ISR)
		USB_Isr();
		#endif
#endif
		USB_DEVICE_Run();
	}
	#endif
}
