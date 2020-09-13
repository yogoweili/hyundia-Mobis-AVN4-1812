/*
 * SAMSUNG tcc USB HOST XHCI Controller
 *
 * Copyright (C) 2012 Samsung Electronics Co.Ltd
 *	Vivek Gautam <gautam.vivek@samsung.com>
 *	Vikas Sajjan <vikas.sajjan@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*
 * This file is a conglomeration for DWC3-init sequence and further
 * tcc specific PHY-init sequence.
 */

#include <common.h>
#include <fdtdec.h>
#include <libfdt.h>
#include <malloc.h>
#include <usb.h>
#include <watchdog.h>
//#include <asm/arch/cpu.h>
//#include <asm/arch/power.h>
//#include <asm/arch/xhci-tcc.h>
#include <asm/gpio.h>
#include <asm-generic/errno.h>
#include <linux/compat.h>
#include <linux/usb/dwc3.h>
#include "xhci.h"
#include <asm/arch/sys_proto.h>

/* Declare global data pointer */
DECLARE_GLOBAL_DATA_PTR;

/**
 * Contains pointers to register base addresses
 * for the usb controller.
 */
struct tcc_xhci {
	struct tcc_usb3_phy *usb3_phy;
	struct xhci_hccr *hcd;
	struct dwc3 *dwc3_reg;
	struct fdt_gpio_state vbus_gpio;
};

static struct tcc_xhci tcc;

#ifdef CONFIG_OF_CONTROL
static int tcc_usb3_parse_dt(const void *blob, struct tcc_xhci *tcc)
{
	fdt_addr_t addr;
	unsigned int node;
	int depth;

	node = fdtdec_next_compatible(blob, 0, COMPAT_SAMSUNG_tcc_XHCI);
	if (node <= 0) {
		debug("XHCI: Can't get device node for xhci\n");
		return -ENODEV;
	}

	/*
	 * Get the base address for XHCI controller from the device node
	 */
	addr = fdtdec_get_addr(blob, node, "reg");
	if (addr == FDT_ADDR_T_NONE) {
		debug("Can't get the XHCI register base address\n");
		return -ENXIO;
	}
	tcc->hcd = (struct xhci_hccr *)addr;

	/* Vbus gpio */
	fdtdec_decode_gpio(blob, node, "samsung,vbus-gpio", &tcc->vbus_gpio);

	depth = 0;
	node = fdtdec_next_compatible_subnode(blob, node,
				COMPAT_SAMSUNG_tcc_USB3_PHY, &depth);
	if (node <= 0) {
		debug("XHCI: Can't get device node for usb3-phy controller\n");
		return -ENODEV;
	}

	/*
	 * Get the base address for usbphy from the device node
	 */
	tcc->usb3_phy = (struct tcc_usb3_phy *)fdtdec_get_addr(blob, node,
								"reg");
	if (tcc->usb3_phy == NULL) {
		debug("Can't get the usbphy register address\n");
		return -ENXIO;
	}

	return 0;
}
#endif

void wait_usb_phy_clock_valid(void)
{
    unsigned int uTmp;

    // Wait USB30 PHY initialization finish. PHY FREECLK should be stable.
    while(1) {
        // GDBGLTSSM : Check LTDB Sub State is non-zero
        // When PHY FREECLK is valid, LINK mac_resetn is released and LTDB Sub State change to non-zero
        //uTmp = HwUSB30->sGlobal.uGDBGLTSSM.bReg.LTDBSubState;
        uTmp = readl((HwUSBGLOBAL_BASE+0x64));
        uTmp = (uTmp>>18)&0xF;

        // Break when LTDB Sub state is non-zero and CNR is zero
        if (uTmp != 0){
            break;
        }
    }
}

#define TXVRT_SHIFT 6
#define TXVRT_MASK (0xF << TXVRT_SHIFT)

#define TXRISE_SHIFT 10
#define TXRISE_MASK (0x3 << TXRISE_SHIFT)

static void tcc_usb3_phy_init(void)
{
	PUSBPHYCFG USBPHYCFG = (PUSBPHYCFG)HwUSBPHYCFG_BASE;

	//	printk(KERN_CRIT "\x1b[1;33m%s: dwc_usb3_phy_init()\x1b[0m\n", dwc_driver_name);
	// Initialize All PHY & LINK CFG Registers
	USBPHYCFG->UPCR0 = 0xB5484068;
	USBPHYCFG->UPCR1 = 0x0000041C;

#if defined(TARGET_BOARD_DAUDIO)
	USBPHYCFG->UPCR2 = 0xF19E14C8;
#else
	USBPHYCFG->UPCR2 = 0x919E14C8;
#endif

	USBPHYCFG->UPCR3 = 0x4AB05D00;
#if !defined(CONFIG_TCC_USB_DRD)
	USBPHYCFG->UPCR4 = 0x00000000;
#endif
	USBPHYCFG->UPCR5 = 0x00108001;
	USBPHYCFG->LCFG  = 0x80420013;

#if defined(CONFIG_TCC_USB_DRD)
	BITSET(USBPHYCFG->UPCR4, Hw20|Hw21);
#endif
	// De-assert Resets
	// USB30PHY_CFG1[01:01] : PIPE_RESETN = 1'b1;
	// USB30PHY_CFG1[02:02] : UTMI_RESET(=PORTRESET) = 1'b0;
	// USB30PHY_CFG1[03:03] : PHY_RESET = 1'b0;
	// USB30PHY_CFG1[10:10] : COMMONONN = 1'b1;
	// USB30LINK_CFG[25:25] : VCC_RESETN = 1'b1;
	// USB30LINK_CFG[26:26] : VAUX_RESETN = 1'b1;

	USBPHYCFG->UPCR1 = 0x00000412;
	USBPHYCFG->LCFG  = 0x86420013;

    printf("USBPHYCFG->UPCR2 = : %X\n", USBPHYCFG->UPCR2);

	// PHY Clock Setting (all are reset values)
	// USB3PHY_CFG0[26:21] : FSEL = 6'b101010 
	// USB3PHY_CFG0[28:27] : REFCLKSEL = 2'b10
	// USB3PHY_CFG0[29:29] : REF_SSP_EN = 1'b1
	// USB3PHY_CFG0[30:30] : REF_USE_PAD = 1'b0
	// USB3PHY_CFG0[31:31] : SSC_EN = 1'b1

	// Setting LINK Suspend clk dividor (all are reset values)
	//HwHSB_CFG->uUSB30LINK_CFG.bReg.SUSCLK_DIV_EN = 0x1;
	//HwHSB_CFG->uUSB30LINK_CFG.bReg.SUSCLK_DIV = 0x3; // busclk / (3+1) = 250 / 4 = 15.625MHz

	// USB20 Only Mode
#if defined(CONFIG_USB_XHCI_SS_MODE)
	BITCLR(USBPHYCFG->LCFG, Hw30);
	BITCLR(USBPHYCFG->UPCR1,Hw9);
#else
	BITSET(USBPHYCFG->LCFG, Hw30);
	BITSET(USBPHYCFG->UPCR1,Hw9);
#endif
	//HC DC controll
	BITCSET(USBPHYCFG->UPCR2, TXVRT_MASK, 0xC << TXVRT_SHIFT);
	BITCSET(USBPHYCFG->UPCR2, TXRISE_MASK, 0x3 << TXRISE_SHIFT);
	//USBPHYCFG->LCFG	 |= Hw30;	//USB2.0 HS only mode
	//USBPHYCFG->UPCR1 |= Hw9;	//test_powerdown_ssp
	//BITCLR(USBPHYCFG->UPCR1,Hw9);
	//USBPHYCFG->UPCR1 |= Hw8;	//test_powerdown_hsp

	// Wait USB30 PHY initialization finish. PHY FREECLK should be stable.
	wait_usb_phy_clock_valid();
}

PUSB3_GLB	gUSB3_GLB = (PUSB3_GLB)(HwUSBGLOBAL_BASE);

void wait_phy_reset_timing (unsigned int count)
{
	unsigned int i;
	volatile unsigned int tmp;

	// Wait 10us
	tmp = 0;
	for (i=0;i<count * 100;i++)
		tmp++;
}

static void tcc_usb3_phy_exit(void)
{
	PUSBPHYCFG USBPHYCFG = (PUSBPHYCFG)(HwUSBPHYCFG_BASE);

    //HwUSB30->sGlobal.uGUSB2PHYCFG.bReg.PHYSoftRst = 1; // Assert Global PHY reset
    BITSET(gUSB3_GLB->GUSB2PHYCFG.nREG, Hw31);
	//HwHSB_CFG->uUSB30PHY_CFG1.bReg.test_powerdown_ssp = 1; // turn off SS circuit
	BITSET(USBPHYCFG->UPCR1,Hw9);

    // Wait T1 (>10us)
    //msleep(1);
    wait_phy_reset_timing(300*2);

    //HwUSB30->sGlobal.uGUSB2PHYCFG.bReg.PHYSoftRst = 0; // De-Assert Global PHY reset
    BITCLR(gUSB3_GLB->GUSB2PHYCFG.nREG, Hw31);

    // Wait T3-T1 (>165us)
    //msleep(1);
    wait_phy_reset_timing(4800*2);
}

void dwc3_set_mode(struct dwc3 *dwc3_reg, u32 mode)
{
	clrsetbits_le32(&dwc3_reg->g_ctl,
			DWC3_GCTL_PRTCAPDIR(DWC3_GCTL_PRTCAP_OTG),
			DWC3_GCTL_PRTCAPDIR(mode));
}

static void dwc3_core_soft_reset(struct dwc3 *dwc3_reg)
{
	/* Before Resetting PHY, put Core in Reset */
	setbits_le32(&dwc3_reg->g_ctl,
			DWC3_GCTL_CORESOFTRESET);

	/* Assert USB3 PHY reset */
	setbits_le32(&dwc3_reg->g_usb3pipectl[0],
			DWC3_GUSB3PIPECTL_PHYSOFTRST);

	/* Assert USB2 PHY reset */
	setbits_le32(&dwc3_reg->g_usb2phycfg,
			DWC3_GUSB2PHYCFG_PHYSOFTRST);

	mdelay(100);

	/* Clear USB3 PHY reset */
	clrbits_le32(&dwc3_reg->g_usb3pipectl[0],
			DWC3_GUSB3PIPECTL_PHYSOFTRST);

	/* Clear USB2 PHY reset */
	clrbits_le32(&dwc3_reg->g_usb2phycfg,
			DWC3_GUSB2PHYCFG_PHYSOFTRST);

	/* After PHYs are stable we can take Core out of reset state */
	clrbits_le32(&dwc3_reg->g_ctl,
			DWC3_GCTL_CORESOFTRESET);
}

static int dwc3_core_init(struct dwc3 *dwc3_reg)
{
	u32 reg;
	u32 revision;
	unsigned int dwc3_hwparams1;

	revision = readl(&dwc3_reg->g_snpsid);
	/* This should read as U3 followed by revision number */
	if ((revision & DWC3_GSNPSID_MASK) != 0x55330000) {
		puts("this is not a DesignWare USB3 DRD Core\n");
		return -EINVAL;
	}

	dwc3_core_soft_reset(dwc3_reg);

	dwc3_hwparams1 = readl(&dwc3_reg->g_hwparams1);

	reg = readl(&dwc3_reg->g_ctl);
	reg &= ~DWC3_GCTL_SCALEDOWN_MASK;
	reg &= ~DWC3_GCTL_DISSCRAMBLE;
	switch (DWC3_GHWPARAMS1_EN_PWROPT(dwc3_hwparams1)) {
	case DWC3_GHWPARAMS1_EN_PWROPT_CLK:
		reg &= ~DWC3_GCTL_DSBLCLKGTNG;
		break;
	default:
		debug("No power optimization available\n");
	}

	/*
	 * WORKAROUND: DWC3 revisions <1.90a have a bug
	 * where the device can fail to connect at SuperSpeed
	 * and falls back to high-speed mode which causes
	 * the device to enter a Connect/Disconnect loop
	 */
	if ((revision & DWC3_REVISION_MASK) < 0x190a)
		reg |= DWC3_GCTL_U2RSTECN;

	writel(reg, &dwc3_reg->g_ctl);

	return 0;
}

static void tcc_start_xhci(void)
{
	tca_ckc_setperi(PERI_USBOTG    , ENABLE,	480000);

}
/*
static void tcc_stop_xhci(void)
{
	tca_ckc_setperi(PERI_USBOTG    , DISABLE,	10000);
}
*/
static int tcc_xhci_core_init(struct tcc_xhci *tcc)
{
	int ret;

	tcc_start_xhci();

	tcc_usb3_phy_init();

	ret = dwc3_core_init(tcc->dwc3_reg);
	if (ret) {
		debug("failed to initialize core\n");
		return -EINVAL;
	}

	/* We are hard-coding DWC3 core to Host Mode */
	dwc3_set_mode(tcc->dwc3_reg, DWC3_GCTL_PRTCAP_HOST);

	return 0;
}

static void tcc_xhci_core_exit(struct tcc_xhci *tcc)
{
	tcc_usb3_phy_exit();
}

#define GPIO_OTG0_VBUS_EN   TCC_GPEXT2(13)

#define GPIO_OTG_EN         TCC_GPEXT5(1)


int xhci_hcd_init(int index, struct xhci_hccr **hccr, struct xhci_hcor **hcor)
{
	struct tcc_xhci *ctx = &tcc;
	int ret;

	ctx->hcd = (struct xhci_hccr *)HwUSBLINK_BASE;
	ctx->dwc3_reg = (struct dwc3 *)((char *)(ctx->hcd) + DWC3_REG_OFFSET);

	/* setup the Vbus gpio here */
	gpio_config(GPIO_OTG0_VBUS_EN, GPIO_OUTPUT);
	gpio_set_value(GPIO_OTG0_VBUS_EN, 1);

	gpio_config(GPIO_OTG_EN, GPIO_OUTPUT);
	gpio_set_value(GPIO_OTG_EN, 1);

	ret = tcc_xhci_core_init(ctx);
	if (ret) {
		puts("XHCI: failed to initialize controller\n");
		return -EINVAL;
	}

	*hccr = (ctx->hcd);
	*hcor = (struct xhci_hcor *)((uint32_t) *hccr
				+ HC_LENGTH(xhci_readl(&(*hccr)->cr_capbase)));

	printf("tcc-xhci: init hccr %x and hcor %x hc_length %d\n",
		(uint32_t)*hccr, (uint32_t)*hcor,
		(uint32_t)HC_LENGTH(xhci_readl(&(*hccr)->cr_capbase)));

	return 0;
}

void xhci_hcd_stop(int index)
{
	struct tcc_xhci *ctx = &tcc;

	tcc_xhci_core_exit(ctx);
}
