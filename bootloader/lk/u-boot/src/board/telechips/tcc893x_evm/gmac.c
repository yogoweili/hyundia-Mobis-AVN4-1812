/*
 *  Copyright (C) 2013 Telechips
 *  Telechips <linux@telechips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/arch/globals.h>
#include <asm/arch/clock.h>
#include <asm/arch/reg_physical.h>
#include <asm/gpio.h>
#include <phy.h>
#include <netdev.h>

// GMAC
#define GPIO_GMAC_TXD0      TCC_GPC(5)	//FUNC 1
#define GPIO_GMAC_TXD1      TCC_GPC(6)	//FUNC 1
#define GPIO_GMAC_TXD2      TCC_GPC(12)	//FUNC 1
#if defined(CONFIG_CHIP_TCC8930)
#define GPIO_GMAC_TXD3      TCC_GPC(13)	//FUNC 1
#elif defined(CONFIG_CHIP_TCC8935) || defined(CONFIG_CHIP_TCC8933) || defined(CONFIG_CHIP_TCC8935S) || defined(CONFIG_CHIP_TCC8933S) || defined(CONFIG_CHIP_TCC8937S)
#define GPIO_GMAC_TXD3      TCC_GPC(18)	//FUNC 15
#endif
#define GPIO_GMAC_TXD4      
#define GPIO_GMAC_TXD5      
#define GPIO_GMAC_TXD6      
#define GPIO_GMAC_TXD7      
#define GPIO_GMAC_TXEN      TCC_GPC(7)	//FUNC 1
#define GPIO_GMAC_TXER      
#define GPIO_GMAC_TXCLK     TCC_GPC(0)	//FUNC 1
#if defined(CONFIG_CHIP_TCC8930)
#define GPIO_GMAC_RXD0      TCC_GPC(3)	//FUNC 1
#elif defined(CONFIG_CHIP_TCC8935) || defined(CONFIG_CHIP_TCC8933) || defined(CONFIG_CHIP_TCC8935S) || defined(CONFIG_CHIP_TCC8933S) || defined(CONFIG_CHIP_TCC8937S)
#define GPIO_GMAC_RXD0      TCC_GPC(21)	//FUNC 15
#endif
#define GPIO_GMAC_RXD1      TCC_GPC(4)	//FUNC 1
#define GPIO_GMAC_RXD2      TCC_GPC(10)	//FUNC 1
#define GPIO_GMAC_RXD3      TCC_GPC(11)	//FUNC 1
#define GPIO_GMAC_RXD4      
#define GPIO_GMAC_RXD5      
#define GPIO_GMAC_RXD6      
#define GPIO_GMAC_RXD7      
#define GPIO_GMAC_RXDV      TCC_GPC(8)	// FUNC 1
#if defined(CONFIG_CHIP_TCC8930)
#define GPIO_GMAC_RXER		TCC_GPC(14)	// FUNC 1
#elif defined(CONFIG_CHIP_TCC8935) || defined(CONFIG_CHIP_TCC8933) || defined(CONFIG_CHIP_TCC8935S) || defined(CONFIG_CHIP_TCC8933S) || defined(CONFIG_CHIP_TCC8937S)
#define GPIO_GMAC_RXER		TCC_GPC(19)	//FUNC 15
#endif

#define GPIO_GMAC_RXCLK     TCC_GPC(26)	// FUNC 1

#if defined(CONFIG_CHIP_TCC8930)
#define GPIO_GMAC_COL       TCC_GPC(16)	// FUNC 1
#define GPIO_GMAC_CRS       TCC_GPC(17)	// FUNC 1
#elif defined(CONFIG_CHIP_TCC8935) || defined(CONFIG_CHIP_TCC8933) || defined(CONFIG_CHIP_TCC8935S) || defined(CONFIG_CHIP_TCC8933S) || defined(CONFIG_CHIP_TCC8937S)
#define GPIO_GMAC_COL       TCC_GPC(16)	// FUNC 1
#define GPIO_GMAC_CRS		TCC_GPC(17)	// FUNC 1
#endif

#define GPIO_GMAC_MDC       TCC_GPC(1)	//FUNC 1
#if defined(CONFIG_CHIP_TCC8930)
#define GPIO_GMAC_MDIO      TCC_GPC(2)
#elif defined(CONFIG_CHIP_TCC8935) || defined(CONFIG_CHIP_TCC8933) || defined(CONFIG_CHIP_TCC8935S) || defined(CONFIG_CHIP_TCC8933S) || defined(CONFIG_CHIP_TCC8937S)
#define GPIO_GMAC_MDIO      TCC_GPC(20)	//FUNC 15
#endif

#if defined(CONFIG_CHIP_TCC8930)
#if defined(CONFIG_MACH_TCC8930ST)
#define GPIO_PHY_RST        TCC_GPC(9)	//FUNC 0
#else
#define GPIO_PHY_RST        TCC_GPC(28)	//FUNC 0
#endif
#else
#define GPIO_PHY_RST        TCC_GPC(9)	//FUNC 0
#endif
#if defined(CONFIG_CHIP_TCC8930)
#define GPIO_PHY_ON         TCC_GPC(27)	//FUNC 0
#elif defined(CONFIG_CHIP_TCC8935) || defined(CONFIG_CHIP_TCC8933) || defined(CONFIG_CHIP_TCC8935S) || defined(CONFIG_CHIP_TCC8933S) || defined(CONFIG_CHIP_TCC8937S)
#if defined(CONFIG_MACH_TCC8930ST)
#define GPIO_PHY_ON			TCC_GPC(27)	//FUNC 0
#else
#define GPIO_PHY_ON      	TCC_GPE(25)	//FUNC 0
#endif
#endif

void tca_gmac_phy_pwr_on(void)
{
	gpio_direction_output(GPIO_PHY_ON, 1);
	mdelay(10);
}

void tca_gmac_phy_pwr_off(void)
{
	gpio_direction_output(GPIO_PHY_RST, 0);
	mdelay(10);

	gpio_direction_output(GPIO_PHY_ON, 0);
	mdelay(10);
}

void tca_gmac_phy_reset(void)
{
	gpio_direction_output(GPIO_PHY_RST, 0);
	mdelay(10);
	gpio_direction_output(GPIO_PHY_RST, 1);
	mdelay(150);
}

void tca_gmac_port_init(void)
{
	volatile PHSIOBUSCFG pHSIOCFG= (volatile PHSIOBUSCFG)HwHSIOBUSCFG_BASE;
	volatile PGPIO pGPIO= (volatile PGPIO)HwGPIO_BASE;

	pGPIO->GPCFN0.bREG.GPFN01 = 1; //MDC
#if defined(CONFIG_CHIP_TCC8930)
	pGPIO->GPCFN0.bREG.GPFN02 = 1; //MDIO
#elif defined(CONFIG_CHIP_TCC8935) || defined(CONFIG_CHIP_TCC8933) || defined(CONFIG_CHIP_TCC8935S) || defined(CONFIG_CHIP_TCC8933S) || defined(CONFIG_CHIP_TCC8937S)
	pGPIO->GPCFN2.bREG.GPFN20 = 15; //MDIO
#endif

	pGPIO->GPCFN0.bREG.GPFN00 = 1; //TXCLK
	pGPIO->GPCFN0.bREG.GPFN05 = 1; //TXD0
	pGPIO->GPCFN0.bREG.GPFN06 = 1; //TXD1
	pGPIO->GPCFN1.bREG.GPFN12 = 1; //TXD2
#if defined(CONFIG_CHIP_TCC8930)
	pGPIO->GPCFN1.bREG.GPFN13= 1; //TXD3
#elif defined(CONFIG_CHIP_TCC8935) || defined(CONFIG_CHIP_TCC8933) || defined(CONFIG_CHIP_TCC8935S) || defined(CONFIG_CHIP_TCC8933S) || defined(CONFIG_CHIP_TCC8937S)
	pGPIO->GPCFN2.bREG.GPFN18 = 15; //TXD3
#endif
	pGPIO->GPCFN0.bREG.GPFN07 = 1; //TXEN

	pGPIO->GPCFN3.bREG.GPFN26 = 1; //RXCLK
#if defined(CONFIG_CHIP_TCC8930)
	pGPIO->GPCFN0.bREG.GPFN03= 1; //RXD0
#elif defined(CONFIG_CHIP_TCC8935) || defined(CONFIG_CHIP_TCC8933) || defined(CONFIG_CHIP_TCC8935S) || defined(CONFIG_CHIP_TCC8933S) || defined(CONFIG_CHIP_TCC8937S)
	pGPIO->GPCFN2.bREG.GPFN21 = 15; //RXD0
#endif
	pGPIO->GPCFN0.bREG.GPFN04 = 1; //RXD1
	pGPIO->GPCFN1.bREG.GPFN10 = 1; //RXD2
	pGPIO->GPCFN1.bREG.GPFN11 = 1; //RXD3
	pGPIO->GPCFN1.bREG.GPFN08 = 1; //RXDV

#if defined(CONFIG_TCC_GMAC_MII_MODE)
	pGPIO->GPCFN1.bREG.GPFN15 = 1; //TXER
#if defined(CONFIG_CHIP_TCC8930)
	pGPIO->GPCFN1.bREG.GPFN14 = 1; //RXER
#else
	pGPIO->GPCFN2.bREG.GPFN19 = 15; //RXER
#endif
	pGPIO->GPCFN2.bREG.GPFN16 = 1; //COL
	pGPIO->GPCFN2.bREG.GPFN17 = 1; //CRS
#elif defined(CONFIG_TCC_GMAC_GMII_MODE)
	pGPIO->GPCFN1.bREG.GPFN15 = 1; //TXER
	pGPIO->GPCFN1.bREG.GPFN14 = 1; //RXER
	pGPIO->GPCFN2.bREG.GPFN16 = 1; //COL
	pGPIO->GPCFN2.bREG.GPFN17 = 1; //sCRS

	pGPIO->GPCFN2.bREG.GPFN22 = 1; //GPIO_GMAC_TXD4
	pGPIO->GPCFN2.bREG.GPFN23 = 1; //GPIO_GMAC_TXD5
	pGPIO->GPCFN3.bREG.GPFN24= 1; //GPIO_GMAC_TXD6
	pGPIO->GPCFN3.bREG.GPFN25 = 1; //GPIO_GMAC_TXD7

	pGPIO->GPCFN2.bREG.GPFN18 = 1; //GPIO_GMAC_RXD4
	pGPIO->GPCFN2.bREG.GPFN19 = 1; //GPIO_GMAC_RXD5
	pGPIO->GPCFN2.bREG.GPFN20= 1; //GPIO_GMAC_RXD6
	pGPIO->GPCFN2.bREG.GPFN21 = 1; //GPIO_GMAC_RXD7
#elif defined(CONFIG_TCC_GMAC_RGMII_MODE)
#endif

#define GMAC_RX_DRV_STRENGTH   3
#define GMAC_TX_DRV_STRENGTH   1

	pGPIO->GPCCD0.bREG.GPCD01 = GMAC_RX_DRV_STRENGTH; //MDC
#if defined(CONFIG_CHIP_TCC8930)
	pGPIO->GPCCD0.bREG.GPCD02 = GMAC_RX_DRV_STRENGTH; //MDIO
#elif defined(CONFIG_CHIP_TCC8935) || defined(CONFIG_CHIP_TCC8933) || defined(CONFIG_CHIP_TCC8935S) || defined(CONFIG_CHIP_TCC8933S) || defined(CONFIG_CHIP_TCC8937S)
	pGPIO->GPCCD1.bREG.GPCD20= GMAC_RX_DRV_STRENGTH; //MDIO
#endif

	pGPIO->GPCCD0.bREG.GPCD00 = 2; //TXCLK
	pGPIO->GPCCD0.bREG.GPCD05 = GMAC_TX_DRV_STRENGTH; //TXD0
	pGPIO->GPCCD0.bREG.GPCD06 = GMAC_TX_DRV_STRENGTH; //TXD1
	pGPIO->GPCCD0.bREG.GPCD12 = GMAC_TX_DRV_STRENGTH; //TXD2
#if defined(CONFIG_CHIP_TCC8930)
	pGPIO->GPCCD0.bREG.GPCD13 = GMAC_TX_DRV_STRENGTH; //TXD3
#elif defined(CONFIG_CHIP_TCC8935) || defined(CONFIG_CHIP_TCC8933) || defined(CONFIG_CHIP_TCC8935S) || defined(CONFIG_CHIP_TCC8933S) || defined(CONFIG_CHIP_TCC8937S)
	pGPIO->GPCCD1.bREG.GPCD18 = GMAC_TX_DRV_STRENGTH; //TXD3
#endif
	pGPIO->GPCCD0.bREG.GPCD07 = GMAC_TX_DRV_STRENGTH; //TXEN

	pGPIO->GPCCD1.bREG.GPCD26 = GMAC_RX_DRV_STRENGTH; //RXCLK
#if defined(CONFIG_CHIP_TCC8930)
	pGPIO->GPCCD0.bREG.GPCD03 = GMAC_RX_DRV_STRENGTH; //RXD0
#elif defined(CONFIG_CHIP_TCC8935) || defined(CONFIG_CHIP_TCC8933) || defined(CONFIG_CHIP_TCC8935S) || defined(CONFIG_CHIP_TCC8933S) || defined(CONFIG_CHIP_TCC8937S)
	pGPIO->GPCCD1.bREG.GPCD21 = GMAC_RX_DRV_STRENGTH; //RXD0
#endif
	pGPIO->GPCCD0.bREG.GPCD04 = GMAC_RX_DRV_STRENGTH; //RXD1
	pGPIO->GPCCD0.bREG.GPCD10 = GMAC_RX_DRV_STRENGTH; //RXD2
	pGPIO->GPCCD0.bREG.GPCD11 = GMAC_RX_DRV_STRENGTH; //RXD3
	pGPIO->GPCCD0.bREG.GPCD08 = GMAC_RX_DRV_STRENGTH; //RXEN

	pGPIO->GPCSR.bREG.GP01 = 0; //fast slew rate MDC
#if defined(CONFIG_CHIP_TCC8930)
	pGPIO->GPCSR.bREG.GP02 = 0; //fast slew rate MDIO
#elif defined(CONFIG_CHIP_TCC8935) || defined(CONFIG_CHIP_TCC8933) || defined(CONFIG_CHIP_TCC8935S) || defined(CONFIG_CHIP_TCC8933S) || defined(CONFIG_CHIP_TCC8937S)
	pGPIO->GPCSR.bREG.GP20 = 0; //fast slew rate MDIO
#endif

	pGPIO->GPCSR.bREG.GP00 = 0; //fast slew rate TXCLK
	pGPIO->GPCSR.bREG.GP05 = 0; //fast slew rate TXD0
	pGPIO->GPCSR.bREG.GP06 = 0; //fast slew rate TXD1
	pGPIO->GPCSR.bREG.GP12 = 0; //fast slew rate TXD2
#if defined(CONFIG_CHIP_TCC8930)
	pGPIO->GPCSR.bREG.GP13 = 0; //fast slew rate TXD3
#elif defined(CONFIG_CHIP_TCC8935) || defined(CONFIG_CHIP_TCC8933) || defined(CONFIG_CHIP_TCC8935S) || defined(CONFIG_CHIP_TCC8933S) || defined(CONFIG_CHIP_TCC8937S)
	pGPIO->GPCSR.bREG.GP18 = 0; //fast slew rate TXD3
#endif
	pGPIO->GPCSR.bREG.GP07 = 0; //fast slew rate TXEN

	pGPIO->GPCSR.bREG.GP26 = 0; //fast slew rate RXCLK
#if defined(CONFIG_CHIP_TCC8930)
	pGPIO->GPCSR.bREG.GP03 = 0; //fast slew rate RXD0
#elif defined(CONFIG_CHIP_TCC8935) || defined(CONFIG_CHIP_TCC8933) || defined(CONFIG_CHIP_TCC8935S) || defined(CONFIG_CHIP_TCC8933S) || defined(CONFIG_CHIP_TCC8937S)
	pGPIO->GPCSR.bREG.GP21 = 0; //fast slew rate RXD0
#endif
	pGPIO->GPCSR.bREG.GP04 = 0; //fast slew rate RXD1
	pGPIO->GPCSR.bREG.GP10 = 0; //fast slew rate RXD2
	pGPIO->GPCSR.bREG.GP11 = 0; //fast slew rate RXD3
	pGPIO->GPCSR.bREG.GP08 = 0; //fast slew rate RXEN

#if defined(CONFIG_TCC_GMAC_MII_MODE)
	pGPIO->GPCCD0.bREG.GPCD15 = GMAC_TX_DRV_STRENGTH; //TXER

#if defined(CONFIG_CHIP_TCC8930)
	pGPIO->GPCCD0.bREG.GPCD14 = GMAC_TX_DRV_STRENGTH; //RXER
#else
	pGPIO->GPCCD1.bREG.GPCD19 = GMAC_TX_DRV_STRENGTH; //RXER
#endif
	pGPIO->GPCCD1.bREG.GPCD16 = GMAC_TX_DRV_STRENGTH; //COL
	pGPIO->GPCCD1.bREG.GPCD17 = GMAC_TX_DRV_STRENGTH; //CRS

	pGPIO->GPCSR.bREG.GP15 = 0; //fast slew rate TXER

	pGPIO->GPCSR.bREG.GP14 = 0; //fast slew rate RXER
	pGPIO->GPCSR.bREG.GP16 = 0; //fast slew rate COL
	pGPIO->GPCSR.bREG.GP17 = 0; //fast slew rate CRS

#elif defined(CONFIG_TCC_GMAC_GMII_MODE)
	pGPIO->GPCCD0.bREG.GPCD15 = GMAC_TX_DRV_STRENGTH; //TXER
	pGPIO->GPCCD0.bREG.GPCD14 = GMAC_TX_DRV_STRENGTH; //RXER
	pGPIO->GPCCD1.bREG.GPCD16 = GMAC_TX_DRV_STRENGTH; //COL
	pGPIO->GPCCD1.bREG.GPCD17 = GMAC_TX_DRV_STRENGTH; //CRS

	pGPIO->GPCCD1.bREG.GPCD22 = GMAC_TX_DRV_STRENGTH; //TXD4
	pGPIO->GPCCD1.bREG.GPCD23 = GMAC_TX_DRV_STRENGTH; //TXD5
	pGPIO->GPCCD1.bREG.GPCD24 = GMAC_TX_DRV_STRENGTH; //TXD6
	pGPIO->GPCCD1.bREG.GPCD25 = GMAC_TX_DRV_STRENGTH; //TXD7

	pGPIO->GPCCD0.bREG.GPCD18 = GMAC_RX_DRV_STRENGTH; //RXD4
	pGPIO->GPCCD0.bREG.GPCD19 = GMAC_RX_DRV_STRENGTH; //RXD5
	pGPIO->GPCCD1.bREG.GPCD20 = GMAC_RX_DRV_STRENGTH; //RXD6
	pGPIO->GPCCD1.bREG.GPCD21 = GMAC_RX_DRV_STRENGTH; //RXD7

	pGPIO->GPCSR.bREG.GP15 = 0; //fast slew rate TXER
	pGPIO->GPCSR.bREG.GP14 = 0; //fast slew rate RXER
	pGPIO->GPCSR.bREG.GP16 = 0; //fast slew rate COL
	pGPIO->GPCSR.bREG.GP17 = 0; //fast slew rate CRS

	pGPIO->GPCSR.bREG.GP22 = 0; //fast slew rate TXD4
	pGPIO->GPCSR.bREG.GP23 = 0; //fast slew rate TXD5
	pGPIO->GPCSR.bREG.GP24 = 0; //fast slew rate TXD6
	pGPIO->GPCSR.bREG.GP25 = 0; //fast slew rate TXD7

	pGPIO->GPCSR.bREG.GP18 = 0; //fast slew rate RXD4
	pGPIO->GPCSR.bREG.GP19 = 0; //fast slew rate RXD5
	pGPIO->GPCSR.bREG.GP20 = 0; //fast slew rate RXD6
	pGPIO->GPCSR.bREG.GP21 = 0; //fast slew rate RXD7
#elif defined(CONFIG_TCC_GMAC_RGMII_MODE)
	pGPIO->GPCPE.bREG.GP00 = 1; //TXCLK pull-enable
	pGPIO->GPCPS.bREG.GP00 = 0; //TXCLK pull-down
#endif
	gpio_config(GPIO_PHY_RST, GPIO_FN(0));
	gpio_config(GPIO_PHY_ON, GPIO_FN(0));

	gpio_request(GPIO_PHY_RST, "PHY_RST");
	gpio_request(GPIO_PHY_ON, "PHY_ON");

	gpio_direction_output(GPIO_PHY_RST, 0);
	gpio_direction_output(GPIO_PHY_ON, 0);

	pHSIOCFG->ETHER_CFG1.bREG.CE = 0; //Clock Disable
  #if defined(CONFIG_TCC_GMAC_MII_MODE)
	pHSIOCFG->ETHER_CFG1.bREG.PHY_INFSEL = 6;
  #elif defined(CONFIG_TCC_GMAC_GMII_MODE)
	pHSIOCFG->ETHER_CFG1.bREG.PHY_INFSEL = 0;
  #elif defined(CONFIG_TCC_GMAC_RGMII_MODE)
	pHSIOCFG->ETHER_CFG1.bREG.PHY_INFSEL = 1;
  #endif
	pHSIOCFG->ETHER_CFG1.bREG.CE = 1; //Clock Enable
}


#ifdef CONFIG_CMD_NET

int board_eth_init(bd_t *bis)
{
	int ret = 0;
	const uchar enetaddr[7] = CONFIG_MAC_ADDR;
#if defined(CONFIG_TCC_GMAC_MII_MODE)
	u32 interface = PHY_INTERFACE_MODE_MII;
#elif defined(CONFIG_TCC_GMAC_GMII_MODE)
	u32 interface = PHY_INTERFACE_MODE_GMII;
#elif defined(CONFIG_TCC_GMAC_RGMII_MODE)
	u32 interface = PHY_INTERFACE_MODE_RGMII;
#else
	#error "PHY_INTERFACE_MODE Not defined"
#endif

	eth_setenv_enetaddr("ethaddr", enetaddr);

#if defined(CONFIG_DESIGNWARE_ETH)
	tca_gmac_port_init();
	tca_gmac_phy_pwr_on();
	tca_gmac_phy_reset();

	if (designware_initialize(0, CONFIG_GMAC_BASE, CONFIG_DW0_PHY, interface) >= 0)
		ret++;
#endif

	return ret;
}
#endif

