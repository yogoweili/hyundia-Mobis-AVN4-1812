/*
 * SAMSUNG tcc USB HOST EHCI Controller
 *
 * Copyright (C) 2012 Samsung Electronics Co.Ltd
 *	Vivek Gautam <gautam.vivek@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <fdtdec.h>
#include <libfdt.h>
#include <malloc.h>
#include <usb.h>
#include <asm/arch/gpio.h>
#include <asm-generic/errno.h>
#include <linux/compat.h>
#include "ehci.h"

#define GPIO_OTG0_VBUS_EN   TCC_GPEXT2(13)
#define GPIO_OTG1_VBUS_EN   TCC_GPEXT2(14)
#define GPIO_HOST_VBUS_EN	TCC_GPEXT2(15)

#define GPIO_V_5P0_EN       TCC_GPEXT5(0)
#define GPIO_OTG_EN         TCC_GPEXT5(1)
#define GPIO_HS_HOST_EN     TCC_GPEXT5(2)
#define GPIO_FS_HOST_EN     TCC_GPEXT5(3)

#define msleep(a) udelay(a * 1000)

/* Declare global data pointer */
DECLARE_GLOBAL_DATA_PTR;

/**
 * Contains pointers to register base addresses
 * for the usb controller.
 */
struct tcc_ehci {
	struct ehci_hccr *hcd;
	struct fdt_gpio_state vbus_gpio;
};

static struct tcc_ehci tcc;

/* Setup the EHCI host controller. */
static void tcc_USB20HPHY_cfg(void)
{
	unsigned int temp;
	PHSIOBUSCFG pEHCIPHYCFG;
	
	pEHCIPHYCFG = (PHSIOBUSCFG)(HwHSIOBUSCFG_BASE);

	#if defined(CONFIG_ARCH_TCC893X)
	/* A2X_USB20H: not use prefetch buffer all R/W
	 *             set A2X_USB20H in the bootloader
	 */
	//pEHCIPHYCFG->HSIO_CFG.nREG = 0xF000CFCF;
	pEHCIPHYCFG->HSIO_CFG.bREG.A2X_USB20H = 0x3;
	#endif
	
	temp = 0x00000000;
	temp = temp | Hw29 | Hw28;		// [31:28] UTM_TXFSLSTUNE[3:0]
	temp = temp | Hw25 | Hw24;		// [26:24] UTM_SQRXTUNE[2:0]
	//temp = temp | Hw22; 			// [22:20] UTM_OTGTUNE[2:0] // OTG Reference Value [00]
	temp = temp | Hw21 | Hw20;		// tmp
	//temp = temp | Hw17 | Hw16;	// [18:16] UTM_COMPDISTUNE[2:0]
	temp = temp | Hw18;				// [18:16] UTM_COMPDISTUNE[2:0] // OTG Reference Value[111]
    //temp = temp | Hw13;                   // [13] UTM_COMMONONN - Set: Suspend Mode -> Phy Clk disable >>> Can not access EHCI reg.(suspend fail)
	temp = temp | Hw11;				// [12:11] REFCLKSEL

#if defined(CONFIG_ARCH_TCC893X)
		//temp = temp | Hw10;			// [10:9] REFCLKDIV
		temp = temp | Hw9; 				// [10:9] REFCLKDIV	// Ref clock : 24MHz 
#endif
	
	//temp = temp | Hw8;			// [8] : SIDDQ
	temp = temp | Hw6;				// [6]
	temp = temp | Hw5;				// [5]
	temp = temp | Hw4;				// [4]
	temp = temp | Hw3;				// [3]
	temp = temp | Hw2;				// [2]
#if defined(CONFIG_ARCH_TCC893X)
	pEHCIPHYCFG->USB20H_PCFG0.nREG = temp;
#endif

	temp = 0x00000000;
	temp = temp | Hw29;
	temp = temp | Hw28;				// [28] OTG Disable
	//temp = temp | Hw27;			// [27] IDPULLUP
	temp = temp | Hw19 | Hw18;		// [19:18]
	//temp = temp | Hw17;
	temp = temp | Hw16;
	temp = temp | Hw6 | Hw5;
	temp = temp | Hw0;				// [0] TP HS transmitter Pre-Emphasis Enable
#if  defined(CONFIG_ARCH_TCC893X)
	pEHCIPHYCFG->USB20H_PCFG1.nREG = temp;
#endif

	temp = 0x00000000;
	//temp = temp | Hw9;
	//temp = temp | Hw6;
#if defined(CONFIG_ARCH_TCC893X)
	temp = temp | Hw15 | Hw5;
#else
	temp = temp | Hw5;
#endif
#if defined(CONFIG_ARCH_TCC893X)
	pEHCIPHYCFG->USB20H_PCFG2.nREG = temp;
#endif

#if defined(CONFIG_ARCH_TCC893X)
	//pEHCIPHYCFG->USB20H_PCFG0.nREG |= Hw8;					/* analog power-down: 1 */
#endif
	msleep(10);
#if defined(CONFIG_ARCH_TCC893X)
	pEHCIPHYCFG->USB20H_PCFG1.nREG |= Hw31;
#endif
	msleep(20);
}

static void tcc_ehci_phy_ctrl(int on)
{
#if  defined(CONFIG_ARCH_TCC893X)
	tca_ckc_setippwdn(PMU_ISOL_USBHP, !on);
#endif
}

static void tcc_start_ehci(void)
{
	tca_ckc_setperi(PERI_USB20H    , ENABLE,	480000);
}

static void tcc_stop_ehci(void)
{
	tca_ckc_setperi(PERI_USB20H    , DISABLE,	10000);
}

/*
 * EHCI-initialization
 * Create the appropriate control structures to manage
 * a new EHCI host controller.
 */
int ehci_hcd_init(int index, enum usb_init_type init,
		struct ehci_hccr **hccr, struct ehci_hcor **hcor)
{
	struct tcc_ehci *ctx = &tcc;

	//printf("\x1b[1;33m[%s:%d]\x1b[0m\n", __func__, __LINE__);

	ctx->hcd = (struct ehci_hccr *)HwUSB20HOST_EHCI_BASE;

	/* USB HS Nano-Phy Enable */
	tcc_ehci_phy_ctrl(1);

	/* setup the Vbus gpio here */
	//printf("\x1b[1;34m[%s:%d] VBUS Enable\x1b[0m\n", __func__, __LINE__);
	gpio_config(GPIO_OTG1_VBUS_EN, GPIO_OUTPUT);
	gpio_set_value(GPIO_OTG1_VBUS_EN, 1);

	gpio_config(GPIO_HS_HOST_EN, GPIO_OUTPUT);
	gpio_set_value(GPIO_HS_HOST_EN, 1);


	tcc_start_ehci();

	tcc_USB20HPHY_cfg();

	*hccr = ctx->hcd;
	*hcor = (struct ehci_hcor *)((uint32_t) *hccr
				+ HC_LENGTH(ehci_readl(&(*hccr)->cr_capbase)));

	printf("tcc-ehci: init hccr %x and hcor %x hc_length %d\n",
		(uint32_t)*hccr, (uint32_t)*hcor,
		(uint32_t)HC_LENGTH(ehci_readl(&(*hccr)->cr_capbase)));

	return 0;
}

/*
 * Destroy the appropriate control structures corresponding
 * the EHCI host controller.
 */
int ehci_hcd_stop(int index)
{
	//printf("\x1b[1;33m[%s:%d]\x1b[0m\n", __func__, __LINE__);
	tcc_ehci_phy_ctrl(0);
	tcc_stop_ehci();
	
	return 0;
}
