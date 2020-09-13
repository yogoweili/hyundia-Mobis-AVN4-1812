/* linux/arch/arm/mach-tcc893x/board-tcc8900.c
 *
 * Copyright (C) 2010 Telechips, Inc.
 * Copyright (C) 2013 Cleinsoft, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/i2c/pca953x.h>
#include <linux/akm8975.h>
#include <linux/spi/spi.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/flash.h>

#include <mach/gpio.h>
#include <mach/bsp.h>
#include <plat/serial.h>
#include <mach/tca_serial.h>
#include <mach/tca_i2c.h>
#include <mach/ohci.h>
#include <mach/tcc_fb.h>
#include <plat/tcc_ts.h>

#include <plat/nand.h>

#include "devices.h"
#include "board-tcc8930.h"

#if defined(CONFIG_TCC_OUTPUT_STARTER)
#include <linux/i2c/hdmi_phy.h>
#include <linux/i2c/hdmi_edid.h>
#endif
#if defined(CONFIG_PN544_NFC)
#include <linux/nfc/pn544.h>
#endif

#if defined(CONFIG_TCC_OUTPUT_STARTER)
#define HDMI_PHY_I2C_SLAVE_ID 		(0x70>>1)
#define HDMI_EDID_I2C_SLAVE_ID 		(0xA0>>1)
#define HDMI_EDID_SEG_I2C_SLAVE_ID	(0x60>>1)
#endif // defined(CONFIG_TCC_OUTPUT_STARTER)

#if defined(CONFIG_RADIO_RDA5870)
#define RADIO_I2C_SLAVE_ID			(0x20>>1)
#endif

#if defined(CONFIG_FB_TCC_COMPONENT)
#include <linux/i2c/cs4954.h>
#endif

#if defined(CONFIG_FB_TCC_COMPONENT)
#define CS4954_I2C_SLAVE_ID 		(0x08>>1)
#define THS8200_I2C_SLAVE_ID 		(0x40>>1)

#endif // defined(CONFIG_FB_TCC_COMPONENT)

#if defined(CONFIG_DAUDIO)
#include <mach/daudio.h>
#include <mach/daudio_info.h>
#endif

#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT_336S_AT)
#include <linux/atmel_mxt336S.h>
#endif

extern void __init tcc_clk_init(void);
extern void __init tcc_init_irq(void);
extern void __init tcc_map_common_io(void);
extern void __init tcc_reserve_sdram(void);

/**  
 * @author sjpark@cleinsoft
 * @date 2013/12/03
 * spi0 - master : XM
 **/
static struct spi_board_info tcc8930_spi0_board_info[] = {
	{
		.modalias = "spidev",
		.bus_num = 0,					// spi channel 0
		.chip_select = 0,
		/* you can modify the following member variables */
		.max_speed_hz = 2*1000*1000,	// default 2Mhz
		.mode = 0						// default 0, you can choose [SPI_CPOL|SPI_CPHA|SPI_CS_HIGH|SPI_LSB_FIRST]
	},
};

/**  
 * @author sjpark@cleinsoft
 * @date 2013/11/19
 * spi setting for daudio - change mod name to "spidev"
 **/
static struct spi_board_info tcc8930_spi1_board_info[] = {
	{
		.modalias = "spidev",
		.bus_num = 1,
		.chip_select = 0,
	#if defined(CONFIG_DAUDIO)
		#if defined(INCLUDE_XM)
		.max_speed_hz = 10*1000*1000,
		#elif defined(INCLUDE_CMMB)
		.max_speed_hz = 5*1000*1000,	// default 2Mhz
		#else
		.max_speed_hz = 10*1000*1000,
		#endif
	#else
		.max_speed_hz = 2*1000*1000,	// default 2Mhz
	#endif
		.mode = 0						// default 0, you can choose [SPI_CPOL|SPI_CPHA|SPI_CS_HIGH|SPI_LSB_FIRST]
	},
};

#if defined(CONFIG_TCC_OUTPUT_STARTER)
static struct tcc_hdmi_phy_i2c_platform_data  hdmi_phy_i2c_data = {
};
static struct tcc_hdmi_phy_i2c_platform_data  hdmi_edid_i2c_data = {
};
static struct tcc_hdmi_phy_i2c_platform_data  hdmi_edid_seg_i2c_data = {
};
#endif

#if defined(CONFIG_TCC_OUTPUT_STARTER)
/* I2C core0 channel 0 devices */
static struct i2c_board_info __initdata i2c_devices0[] = {
    {
		I2C_BOARD_INFO("tcc-hdmi-edid", HDMI_EDID_I2C_SLAVE_ID),
		.platform_data = &hdmi_edid_i2c_data,
	},	
    {
		I2C_BOARD_INFO("tcc-hdmi-edid-seg", HDMI_EDID_SEG_I2C_SLAVE_ID),
		.platform_data = &hdmi_edid_seg_i2c_data,
	},	
};
#endif

#if defined(CONFIG_PN544_NFC)
static struct pn544_i2c_platform_data nfc_pdata = {
	#if defined(CONFIG_CHIP_TCC8930)
	.irq_gpio = TCC_GPB(15),//INT_DXB0_IRQ, //
	#elif defined(CONFIG_CHIP_TCC8935) || defined(CONFIG_CHIP_TCC8933) || defined(CONFIG_CHIP_TCC8935S) || defined(CONFIG_CHIP_TCC8933S) || defined(CONFIG_CHIP_TCC8937S)
	.irq_gpio = TCC_GPD(6),//INT_DXB0_IRQ, //
	#endif
	.ven_gpio = GPIO_DXB_ON,
	//.firm_gpio = GPIO_DXB_ON,
	};
#endif

#if defined(CONFIG_SENSORS_AK8975)
static struct akm8975_platform_data akm8975_data = {
    .gpio_DRDY = 0,
};
#endif

#if defined(CONFIG_FB_TCC_COMPONENT)
static struct cs4954_i2c_platform_data  cs4954_i2c_data = {
};
static struct cs4954_i2c_platform_data  ths8200_i2c_data = {
};
#endif

#if defined(CONFIG_I2C_TCC_CORE0)
static struct i2c_board_info __initdata i2c_devices0[] = {
	#if defined(CONFIG_RADIO_RDA5870)
	{
		I2C_BOARD_INFO("RDA5870E-FM", RADIO_I2C_SLAVE_ID),
		.platform_data = NULL,
	},
	#endif
    #if defined(CONFIG_SENSORS_AK8975)
    {
        I2C_BOARD_INFO("akm8975", 0x0F),
        .irq           = INT_EINT1,
        .platform_data = &akm8975_data,
    },
    #endif
	#if defined(CONFIG_FB_TCC_COMPONENT)
	{
		I2C_BOARD_INFO("component-cs4954", CS4954_I2C_SLAVE_ID),
		.platform_data = &cs4954_i2c_data,
	},
	{
		I2C_BOARD_INFO("component-ths8200", THS8200_I2C_SLAVE_ID),
		.platform_data = &ths8200_i2c_data,
	}, //CONFIG_FB_TCC_COMPONENT
	#endif

};
#endif

// Touch Touch mXT336S-AT
#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT_336S_AT)
static struct mxt_platform_data mxt336s_data = {
	#if  defined(CONFIG_DAUDIO_20140220)
	.irq_gpio		= TCC_GPF(0),
	.irq_jig_gpio	= TCC_GPB(30),
	.irq_jig_num	= INT_EINT7,
	#endif
	.max_x = 799,
	.max_y = 479,
};
#endif

#if defined(CONFIG_I2C_TCC_CORE1)
static struct i2c_board_info __initdata i2c_devices1[] = {
#if defined(CONFIG_TOUCHSCREEN_SOLOMON_SSD2532)
	{
		I2C_BOARD_INFO("ssd253x-ts", 0x48),
		.irq = INT_EINT4,
	},
#elif defined(CONFIG_TOUCHSCREEN_ATMEL_MXT_336S_AT)
	/**
	 * @author valky@cleinsoft
	 * @date 2013/11/19
	 * Touch mXT336S-AT
	 * device addresses are 0x4A (ADDSEL low) and 0x4B (ADDSEL high)
	 **/
	{
		I2C_BOARD_INFO("mxt336s", 0x4A),
		.platform_data = &mxt336s_data,
		.irq = INT_EINT4,
	},
#endif
#if defined(CONFIG_TOUCHSCREEN_TCC_AK4183)
	{
		I2C_BOARD_INFO("tcc-ts-ak4183", 0x48),
		.platform_data = NULL,
	},
#endif
#if defined(CONFIG_TOUCHSCREEN_TCC_SITRONIX)
	{
		I2C_BOARD_INFO("tcc-ts-sitronix", 0x55),
		.platform_data = NULL,
	},
#endif
#if defined(CONFIG_TOUCHSCREEN_TCC_GT813) || defined(CONFIG_TOUCHSCREEN_TCC_GT827)
	{
		I2C_BOARD_INFO("tcc-ts", 0x5D),
		.irq = INT_EINT2,
		.platform_data = NULL,
	},
#endif
#if defined(CONFIG_PN544_NFC)
	{
		I2C_BOARD_INFO("pn544", 0x2B), // IF0 = high, IF1 = high
		.platform_data = &nfc_pdata,
		.irq = INT_EINT8,
	},
#endif
};
#endif

#if defined(CONFIG_I2C_TCC_CORE2)
static struct i2c_board_info __initdata i2c_devices2[] = {
};
#endif

#if defined(CONFIG_I2C_TCC_CORE3)
static struct i2c_board_info __initdata i2c_devices3[] = {
};
#endif
#if defined(CONFIG_I2C_TCC_SMU)
static struct i2c_board_info __initdata i2c_devices_smu[] = {
};
#endif

#if defined(CONFIG_I2C_TCC_CORE0)
static struct tcc_i2c_platform_data tcc8930_core0_platform_data = {
    .core_clk_rate      = 4*1000*1000,    /* core clock rate: 4MHz */
    .core_clk_name      = "i2c0",
    .smu_i2c_flag       = 0,
    #if defined(CONFIG_TCC_CP_I2C0) 
        #if defined(CONFIG_TCC_CP_20C)
        .i2c_ch_clk_rate[0] = 100,
        #else
        .i2c_ch_clk_rate[0] = 25,
        #endif
    #else
    .i2c_ch_clk_rate[0] = 400,      /* SCL clock rate : 100kHz */
    #endif
};
#endif
#if defined(CONFIG_I2C_TCC_CORE1)
static struct tcc_i2c_platform_data tcc8930_core1_platform_data = {
    .core_clk_rate      = 4*1000*1000,    /* core clock rate: 4MHz */
    .core_clk_name      = "i2c1",
    .smu_i2c_flag       = 0,
    .i2c_ch_clk_rate[0] = 400,      /* SCL clock rate : 400kHz */
};
#endif
#if defined(CONFIG_I2C_TCC_CORE2)
static struct tcc_i2c_platform_data tcc8930_core2_platform_data = {
    .core_clk_rate      = 4*1000*1000,    /* core clock rate: 4MHz */
    .core_clk_name      = "i2c2",
    .smu_i2c_flag       = 0,
    .i2c_ch_clk_rate[0] = 100,      /* SCL clock rate : 100kHz */
};
#endif
#if defined(CONFIG_I2C_TCC_CORE3)
static struct tcc_i2c_platform_data tcc8930_core3_platform_data = {
    .core_clk_rate      = 4*1000*1000,	/* core clock rate: 4MHz */
    .core_clk_name      = "i2c3",
    .smu_i2c_flag       = 0,
    #if defined(CONFIG_TCC_CP_I2C3)
        #if defined(CONFIG_TCC_CP_20C) 
        .i2c_ch_clk_rate[0] = 100,		/* SCL clock rate : 100kHz */
        #else
        .i2c_ch_clk_rate[0] = 25,		/* SCL clock rate : 25kHz */
        #endif
    #else
    .i2c_ch_clk_rate[0] = 400,			/* SCL clock rate : 400kHz */
    #endif
};
#endif
#if defined(CONFIG_I2C_TCC_SMU)
static struct tcc_i2c_platform_data tcc8930_smu_platform_data = {
    .core_clk_name  = "smu",
    .smu_i2c_flag   = 1,
};
#endif

extern void __init tcc8930_init_pmic(void);
extern void __init tcc8930_init_gpio(void);
extern int __init tcc8930_init_camera(void);

static void __init tcc8930_init_irq(void)
{
    tcc_init_irq();
}

static int gMultiCS = 0;

static void tcc8930_nand_init(void)
{
	unsigned int gpio_wp = GPIO_NAND_WP;
	unsigned int gpio_rdy0 = GPIO_NAND_RDY0;	
		
	tcc_gpio_config(gpio_wp, GPIO_FN(0));
	tcc_gpio_config(gpio_rdy0, GPIO_FN(0));	
	
	gpio_request(gpio_wp, "nand_wp");
	gpio_direction_output(gpio_wp, 1);

	gpio_request(gpio_rdy0, "nand_rdy0");
	gpio_direction_input(gpio_rdy0);
}

static int tcc8930_nand_ready(void)
{
#if defined(CONFIG_CHIP_TCC8930)
	if ( gMultiCS == TRUE )
		return ! ( ( gpio_get_value(GPIO_NAND_RDY0) ) && ( gpio_get_value(GPIO_NAND_RDY1) ) );
	else
#endif
		return !gpio_get_value(GPIO_NAND_RDY0);	
}

static void tcc8930_SetFlags( int bMultiCS )
{
#if defined(CONFIG_CHIP_TCC8930)
	gMultiCS = bMultiCS;

	if( gMultiCS == TRUE )
	{
		unsigned int gpio_rdy1 = GPIO_NAND_RDY1;

		tcc_gpio_config(gpio_rdy1, GPIO_FN(0));
		gpio_request(gpio_rdy1, "nand_rdy1");
		gpio_direction_input(gpio_rdy1);
	}
#endif
}

static struct tcc_nand_platform_data tcc_nand_platdata = {
	.parts		= NULL,
	.nr_parts	= 0,
	.gpio_wp	= GPIO_NAND_WP,
	.init		= tcc8930_nand_init,
	.ready		= tcc8930_nand_ready,
	.SetFlags	= tcc8930_SetFlags,
};

static struct platform_device tcc_nand_device = {
	.name		= "tcc_mtd",
	.id			= -1,
	.dev		= {
		.platform_data = &tcc_nand_platdata,
	},
};

static struct platform_device tcc_nand_v8 = {
	.name		= "tcc_nand",
	.id			= -1,	
};

static void tcc_add_nand_device(void)
{
	tcc_get_partition_info(&tcc_nand_platdata.parts, &tcc_nand_platdata.nr_parts);
	platform_device_register(&tcc_nand_device);
}

static struct platform_device tcc_cdrom_device = {
	.name		= "tcc_cdrom",
	.id		= -1,
};

#if defined(CONFIG_WATCHDOG)
static struct platform_device tccwdt_device = {
	.name	= "tcc-wdt",
	.id		= -1,
};
#endif

#if defined(CONFIG_TCC_UART2_DMA)
static struct tcc_uart_platform_data uart2_data = {
    .modem_control_use   =  1,

    .tx_dma_use     = 0,
    .tx_dma_buf_size= SERIAL_TX_DMA_BUF_SIZE,
    .tx_dma_base    = io_p2v(HwGDMA2_BASE),
    .tx_dma_ch      = SERIAL_TX_DMA_CH_NUM,
    .tx_dma_intr    = INT_DMA2_CH0,
    .tx_dma_mode    = SERIAL_TX_DMA_MODE,

    .rx_dma_use     = 1,
    .rx_dma_buf_size= SERIAL_RX_DMA_BUF_SIZE,
    .rx_dma_base    = io_p2v(HwGDMA2_BASE),
    .rx_dma_ch      = SERIAL_RX_DMA_CH_NUM-2,
    .rx_dma_intr    = 0,
    .rx_dma_mode    = SERIAL_RX_DMA_MODE,
};
#endif

#if defined(CONFIG_TCC_UART3_DMA)
static struct tcc_uart_platform_data uart3_data = {
    .modem_control_use   =  1,

    .tx_dma_use     = 0,
    .tx_dma_buf_size= SERIAL_TX_DMA_BUF_SIZE,
    .tx_dma_base    = io_p2v(HwGDMA2_BASE),
    .tx_dma_ch      = SERIAL_TX_DMA_CH_NUM+1,
    .tx_dma_intr    = INT_DMA2_CH1,
    .tx_dma_mode    = SERIAL_TX_DMA_MODE,

    .rx_dma_use     = 1,
    .rx_dma_buf_size= SERIAL_RX_DMA_BUF_SIZE,
    .rx_dma_base    = io_p2v(HwGDMA2_BASE),
    .rx_dma_ch      = SERIAL_RX_DMA_CH_NUM-1,
    .rx_dma_intr    = 0,
    .rx_dma_mode    = SERIAL_RX_DMA_MODE,
};
#endif

#if defined(CONFIG_TCC_UART4_DMA)
static struct tcc_uart_platform_data uart4_data = {
    .modem_control_use   =  1,

    .tx_dma_use     = 0,
    .tx_dma_buf_size= SERIAL_TX_DMA_BUF_SIZE,
    .tx_dma_base    = io_p2v(HwGDMA0_BASE),
    .tx_dma_ch      = SERIAL_TX_DMA_CH_NUM+2,
    .tx_dma_intr    = INT_DMA0_CH2,
    .tx_dma_mode    = SERIAL_TX_DMA_MODE,

    .rx_dma_use     = 1,
    .rx_dma_buf_size= SERIAL_RX_DMA_BUF_SIZE,
    .rx_dma_base    = io_p2v(HwGDMA0_BASE),
    .rx_dma_ch      = SERIAL_RX_DMA_CH_NUM,
    .rx_dma_intr    = 0,
    .rx_dma_mode    = SERIAL_RX_DMA_MODE,
};
#endif

#if defined(CONFIG_TCC_BT_DEV)
static struct tcc_uart_platform_data uart1_data_bt = {
    .bt_use         = 1,
    .modem_control_use   =  1,

    .tx_dma_use     = 0,
    .tx_dma_buf_size= SERIAL_TX_DMA_BUF_SIZE,
    .tx_dma_base    = io_p2v(HwGDMA1_BASE),
    .tx_dma_ch      = SERIAL_TX_DMA_CH_NUM,
    .tx_dma_intr    = INT_DMA1_CH0,
    .tx_dma_mode    = SERIAL_TX_DMA_MODE,

    .rx_dma_use     = 1,
    .rx_dma_buf_size= SERIAL_RX_DMA_BUF_SIZE,
    .rx_dma_base    = io_p2v(HwGDMA1_BASE),
    .rx_dma_ch      = SERIAL_RX_DMA_CH_NUM-2,
    .rx_dma_intr    = 0,
    .rx_dma_mode    = SERIAL_RX_DMA_MODE,
};
#endif

static struct platform_device tcc_bluetooth_device = {	
	.name = "tcc_bluetooth_rfkill",	
	.id = -1,
};

static void tcc_add_bluetooth_device(void)
{
	platform_device_register(&tcc_bluetooth_device);
}


/*----------------------------------------------------------------------
 * Device	  : ADC touchscreen resource
 * Description: tcc8930_touchscreen_resource
 *----------------------------------------------------------------------*/
#if defined(CONFIG_TOUCHSCREEN_TCCTS) || defined(CONFIG_TOUCHSCREEN_TCCTS_MODULE)
static struct resource tcc8930_touch_resources[] = {
	[0] = {
		.start	= io_p2v(HwTSADC_BASE),
		.end	= io_p2v(HwTSADC_BASE + 0x20),
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start = INT_TSADC,
		.end   = INT_TSADC,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = INT_EINT2,
		.end   = INT_EINT2,
		.flags = IORESOURCE_IRQ,
	},
};

static struct tcc_adc_ts_platform_data tcc_touchscreen_pdata = {
#ifdef CONFIG_TOUCHSCREEN_CALIBRATION
	.min_x = 1,
	.max_x = 4095,
	.min_y = 1,
	.max_y = 4095,
#else
	.min_x = 110,
	.max_x = 3990,
	.min_y = 250,
	.max_y = 3800,
#endif
};

static struct platform_device tcc8930_touchscreen_device = {
	.name		= "tcc-ts",
	.id		= -1,
	.resource	= tcc8930_touch_resources,
	.num_resources	= ARRAY_SIZE(tcc8930_touch_resources),
	.dev = {
		.platform_data = &tcc_touchscreen_pdata
	},
};

static void tcc_add_touchscreen_device(void)
{

	if(tcc_panel_id == PANEL_ID_AT070TN93 || tcc_panel_id == PANEL_ID_TD070RDH11)
#if defined(CONFIG_PORTRAIT_LCD)
		tcc_touchscreen_pdata.swap_xy_pos = 1;
#else
		tcc_touchscreen_pdata.reverse_y_pos = 1;		
#endif
	else if(tcc_panel_id == PANEL_ID_LMS350DF01)
	{
		tcc_touchscreen_pdata.swap_xy_pos = 1;
		tcc_touchscreen_pdata.reverse_x_pos = 1;
		tcc_touchscreen_pdata.reverse_y_pos = 1;
	}
	else if(tcc_panel_id == PANEL_ID_EJ070NA)
	{
		tcc_touchscreen_pdata.reverse_y_pos = 1;
	}	
	else
	{
		#if defined(CONFIG_DAUDIO)
			#if defined(CONFIG_DAUDIO_20131007)
				/**
				 * @author valky@cleinsoft
				 * @date 2013/10/14
				 * tier1 2nd board adc touch option
				 **/
				tcc_touchscreen_pdata.reverse_y_pos = 1;
			#endif
		#else
			tcc_touchscreen_pdata.reverse_x_pos = 1;
		#endif
	}

	platform_device_register(&tcc8930_touchscreen_device);
}
#endif

static struct platform_device *tcc8930_devices[] __initdata = {
	&tcc8930_uart0_device,
	&tcc8930_uart1_device,
/**
 * @author sjpark@cleinsoft
 * @date 2013/08/23
 * @date 2014/03/06
 * add new uart devices
 * add uart 5 (fm1288)
 **/
#if defined(CONFIG_DAUDIO)
	&tcc8930_uart2_device,
	&tcc8930_uart3_device,
	&tcc8930_uart4_device,
#if defined(CONFIG_DAUDIO_20140220)
	&tcc8930_uart5_device,
#endif
#else
#if defined(CONFIG_GPS_JGR_SC3_S)
	&tcc8930_uart3_device,
#endif
	&tcc8930_uart4_device,
#endif

#if defined(CONFIG_I2C_TCC_SMU)
    &tcc8930_i2c_smu_device,
#endif
#if defined(CONFIG_I2C_TCC_CORE0)
    &tcc8930_i2c_core0_device,
#endif
#if defined(CONFIG_I2C_TCC_CORE1)
    &tcc8930_i2c_core1_device,
#endif
#if defined(CONFIG_I2C_TCC_CORE2)
    &tcc8930_i2c_core2_device,
#endif
#if defined(CONFIG_I2C_TCC_CORE3)
    &tcc8930_i2c_core3_device,
#endif

#if defined(CONFIG_TCC_ECID_SUPPORT)
	&tcc_cpu_id_device,
#endif

	&tcc8930_adc_device,
#if defined(CONFIG_RTC_DRV_TCC8930)
	&tcc8930_rtc_device,
#endif
#if defined(CONFIG_INPUT_TCC_REMOTE)
	&tcc8930_remote_device,
#endif
#if defined(CONFIG_USB_EHCI_TCC)  || defined(CONFIG_USB_EHCI_HCD_MODULE)
	&ehci_hs_device,
	&ehci_fs_device,
#endif
#if defined(CONFIG_USB_XHCI_TCC) || defined(CONFIG_USB_XHCI_TCC_MODULE)
	&xhci_hs_device,
#endif
#if defined(CONFIG_USB_DWC3) || defined(CONFIG_USB_DWC3_MODULE)
	&tcc8930_dwc3_device,
#endif
#if defined(CONFIG_TCC_DWC_OTG)  || defined(CONFIG_TCC_DWC_OTG_MODULE)
	&tcc8930_dwc_otg_device,
#endif
#if defined(CONFIG_SATA_AHCI)
	&tcc_ahci_device,
#endif
#if defined(CONFIG_BATTERY_TCC)
	&tcc_battery_device,
#endif
#if defined(CONFIG_TCC_GMAC) || defined(CONFIG_TCC_GMAC_MODULE)
	&tcc_gmac_device,
#endif

#if defined(CONFIG_WATCHDOG)
	&tccwdt_device,
#endif
#if defined(CONFIG_TCC_BT_DEV)
	&tcc_bluetooth_device,
#endif

	&tcc_nand_v8,	

	&tcc_cdrom_device,
};

/**  
* @author sjpark@cleinsoft
* @date 2014/12/15
* spi - spi master / tsif - spi slave
* (s) - slave / (m) - master
*              us       china       korea
*  gpsb0     micom(s)   micom(s)    micom(s)
*  gpsb1     XM (m)     CMMB(m)     TDMB(s)
*  gpsb2                            TPEG(s)
**/
static struct platform_device *tcc8930_gpsb_devices[] __initdata = {
    // MICOM - GPSB0
	&tcc_tsif0_device, 
#if defined(INCLUDE_TDMB)
    // TDMB - GPSB1
	&tcc_tsif1_device,
    // TPEG - GPSB 2
	&tcc_tsif2_device,
#else
    // XM or CMMB - GPSB1
    &tcc8930_spi1_device, 
#endif
};

static struct platform_device *daudio_4th_devices[] =
{
	&tcc8930_uart6_device,
};

static int __init board_serialno_setup(char *serialno)
{
	return 1;
}
__setup("androidboot.serialno=", board_serialno_setup);

char* atheros_wifi_mac;

static int __init board_wifimac_setup(char *wifimac)
{
	atheros_wifi_mac = wifimac;

	return 1;
}
__setup("androidboot.wifimac=", board_wifimac_setup);

EXPORT_SYMBOL(atheros_wifi_mac);

static int __init board_btaddr_setup(char *btaddr)
{
	return 1;
}
__setup("androidboot.btaddr=", board_btaddr_setup);

static void __init tcc8930_init_machine(void)
{
#if defined(CONFIG_DAUDIO)
	int main_ver = daudio_main_version();
	printk(KERN_INFO "%s\n", __func__);
#endif

	tcc8930_init_pmic();
	tcc8930_init_gpio();
	tcc8930_init_camera();

#if defined(CONFIG_SPI_TCCXXXX_MASTER)
	#if defined(CONFIG_DAUDIO)
	/**  
	 * @author sjpark@cleinsoft
	 * @date 2014/12/15
	 * spi0 - master : XM
	 **/
#if defined(INCLUDE_TDMB)
#else
    spi_register_board_info(tcc8930_spi1_board_info, ARRAY_SIZE(tcc8930_spi1_board_info));
#endif
	#else
	spi_register_board_info(tcc8930_spi0_board_info, ARRAY_SIZE(tcc8930_spi0_board_info));
	spi_register_board_info(tcc8930_spi1_board_info, ARRAY_SIZE(tcc8930_spi1_board_info));
	#endif
#endif

#if defined(CONFIG_SENSORS_AK8975) //set compass irq
    /* Input mode */
	if(system_rev == 0x1000)
	{
		#if !defined(CONFIG_CHIP_TCC8935S)
		tcc_gpio_config(TCC_GPA(27), GPIO_FN(0)|GPIO_PULL_DISABLE);  // GPIOE[29]: input mode, disable pull-up/down
		gpio_direction_input(TCC_GPA(27));
		tcc_gpio_config_ext_intr(INT_EINT1, EXTINT_GPIOA_27);
		#endif
	}
	else if(system_rev == 0x2000 || system_rev == 0x3000)
	{
		tcc_gpio_config(TCC_GPG(16), GPIO_FN(0)|GPIO_PULL_DISABLE);  // GPIOE[29]: input mode, disable pull-up/down
		gpio_direction_input(TCC_GPG(16));
		tcc_gpio_config_ext_intr(INT_EINT1, EXTINT_GPIOG_16);
	}
	else
	{
		tcc_gpio_config(TCC_GPE(29), GPIO_FN(0)|GPIO_PULL_DISABLE);  // GPIOE[29]: input mode, disable pull-up/down
		gpio_direction_input(TCC_GPE(29));
		tcc_gpio_config_ext_intr(INT_EINT1, EXTINT_GPIOE_29);
	}
#endif
#if defined(CONFIG_PN544_NFC) 
       // INT_DXB0_IRQ
	if(system_rev == 0x1000)
	{
    	    tcc_gpio_config_ext_intr(INT_EINT8, EXTINT_GPIOB_15);
	}
	else if(system_rev == 0x2000 || system_rev == 0x3000)
	{
    	    tcc_gpio_config_ext_intr(INT_EINT8, EXTINT_GPIOD_06);
	}
	else
	{
    	    tcc_gpio_config_ext_intr(INT_EINT8, EXTINT_GPIOB_15);
	}
#endif

#if defined(CONFIG_I2C_TCC_CORE0)
	i2c_register_board_info(0, i2c_devices0, ARRAY_SIZE(i2c_devices0));
#endif
#if defined(CONFIG_I2C_TCC_CORE1)
	i2c_register_board_info(1, i2c_devices1, ARRAY_SIZE(i2c_devices1));
#endif
#if defined(CONFIG_I2C_TCC_CORE2)
	i2c_register_board_info(2, i2c_devices2, ARRAY_SIZE(i2c_devices2));
#endif
#if defined(CONFIG_I2C_TCC_CORE3)
	i2c_register_board_info(3, i2c_devices3, ARRAY_SIZE(i2c_devices3));
#endif
#if defined(CONFIG_I2C_TCC_SMU)
	i2c_register_board_info(4, i2c_devices_smu, ARRAY_SIZE(i2c_devices_smu));
#endif

// Touch intr mXT336S-AT
#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT_336S_AT)
	tcc_gpio_config_ext_intr(INT_EINT4, EXTINT_GPIOF_00);
	tcc_gpio_config_ext_intr(INT_EINT7, EXTINT_GPIOB_30);
#endif

#if defined(CONFIG_TCC_BT_DEV)
	/* BT: use UART1 and TX DMA */
	platform_device_add_data(&tcc8930_uart1_device, &uart1_data_bt, sizeof(struct tcc_uart_platform_data));
#endif
#if defined(CONFIG_TCC_UART2_DMA)
	platform_device_add_data(&tcc8930_uart2_device, &uart2_data, sizeof(struct tcc_uart_platform_data));
#endif

#if defined(CONFIG_TCC_UART3_DMA)
	platform_device_add_data(&tcc8930_uart3_device, &uart3_data, sizeof(struct tcc_uart_platform_data));
#endif
#if defined(CONFIG_TCC_UART4_DMA)
	platform_device_add_data(&tcc8930_uart4_device, &uart4_data, sizeof(struct tcc_uart_platform_data));
#endif
#if defined(CONFIG_TOUCHSCREEN_TCCTS) || defined(CONFIG_TOUCHSCREEN_TCCTS_MODULE)
	tcc_add_touchscreen_device();
#endif

#if defined(CONFIG_I2C_TCC_CORE0)
    platform_device_add_data(&tcc8930_i2c_core0_device, &tcc8930_core0_platform_data, sizeof(struct tcc_i2c_platform_data));
#endif
#if defined(CONFIG_I2C_TCC_CORE1)
    platform_device_add_data(&tcc8930_i2c_core1_device, &tcc8930_core1_platform_data, sizeof(struct tcc_i2c_platform_data));
#endif
#if defined(CONFIG_I2C_TCC_CORE2)
    platform_device_add_data(&tcc8930_i2c_core2_device, &tcc8930_core2_platform_data, sizeof(struct tcc_i2c_platform_data));
#endif
#if defined(CONFIG_I2C_TCC_CORE3)
    platform_device_add_data(&tcc8930_i2c_core3_device, &tcc8930_core3_platform_data, sizeof(struct tcc_i2c_platform_data));
#endif
#if defined(CONFIG_I2C_TCC_SMU)
	platform_device_add_data(&tcc8930_i2c_smu_device, &tcc8930_smu_platform_data, sizeof(struct tcc_i2c_platform_data));
#endif

	tcc_add_nand_device();

	platform_add_devices(tcc8930_devices, ARRAY_SIZE(tcc8930_devices));
	platform_add_devices(tcc8930_gpsb_devices, ARRAY_SIZE(tcc8930_gpsb_devices));

	//D-Audio
	switch (main_ver)
	{
		case DAUDIO_BOARD_6TH:
		case DAUDIO_BOARD_5TH:
		default:
			platform_add_devices(daudio_4th_devices, ARRAY_SIZE(daudio_4th_devices));
			tcc_gpio_config_ext_intr(INT_EINT5, EXTINT_GPIOB_18);
			break;
	}
}


static void __init tcc8930_map_io(void)
{
	tcc_map_common_io();
}

static void __init tcc8930_init_early(void)
{
    tcc_clk_init();
}

static void __init tcc8930_mem_reserve(void)
{
	tcc_reserve_sdram();
}

extern struct sys_timer tcc893x_timer;

MACHINE_START(TCC893X, "tcc893x")
	/* Maintainer: Telechips Linux BSP Team <linux@telechips.com> */
	.boot_params    = PHYS_OFFSET + 0x00000100,
	.reserve        = tcc8930_mem_reserve,
	.map_io         = tcc8930_map_io,
	.init_early	    = tcc8930_init_early,
	.init_irq       = tcc8930_init_irq,
	.init_machine   = tcc8930_init_machine,
	.timer          = &tcc893x_timer,
MACHINE_END
