/*
 * usbctrl.c: common usb control API
 *
 *  Copyright (C) 2011, Telechips, Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/cpufreq.h>

#include <asm/io.h>
#include <asm/mach-types.h>

#include <mach/bsp.h>
#include <mach/gpio.h>
#include <mach/tcc_board_power.h>

#if defined(CONFIG_TCC_CLOCK_TABLE)
extern const struct tcc_freq_table_t gtHSIONormalClockLimitTable;
#endif

static struct clk *g_pOHCIClk = NULL;
static struct clk *g_pEHCIClk = NULL; 
static struct clk *g_pXHCIClk = NULL; 
static int is_usb_otg = 0;
static unsigned int g_iExec = 0;


void tcc_ohci_clkset(int on)
{
	char *pDev = NULL;
	
	if (machine_is_tcc8900()){
		pDev = "usb_host"; 
	}else if (machine_is_tcc8800() || machine_is_tcc8800st()){
		pDev = "usb11h";
	}else if (machine_is_tcc8920()){
		pDev = "usb20h";
	}else if(machine_is_tcc8920st()){

	}else if(machine_is_m805_892x()){

	}else{
		pDev = "unknown";
	}
	
	if (on){
		if(g_pOHCIClk){
			//printk("~~> [WARNING] OHCI clock is already turned on\n");
			return;
		}

		g_pOHCIClk = clk_get(NULL, (const char*)pDev);
		if (IS_ERR(g_pOHCIClk)){
			printk(KERN_ERR "ERR - usb_host clk_get fail.\n");
			return;
		}else{
			clk_enable(g_pOHCIClk);
			clk_set_rate(g_pOHCIClk, 48*1000*1000);
		}
	}else{
		if (g_pOHCIClk) {
#if defined(CONFIG_TCC_DWC_OTG)
			if (!is_usb_otg)
#endif
			{
				if(g_pOHCIClk){
					clk_disable(g_pOHCIClk);
				}
				g_pOHCIClk = NULL;
			}
		}
	}
}

/* [id] 0:OHCI , 1:EHCI's OHCI , -1:OTG */
void tcc_ohci_clock_control(int id, int on)
{
	if (id == 1){
		return;
	}

	if(g_iExec){
		printk("tcc_ohci_clock_control is still running...(%d)\n", id);
		return;
	}
	
	g_iExec = 1;
	
	if (on) {
		tcc_ohci_clkset(1);
		is_usb_otg = 1;
	} else {
#if !defined(CONFIG_USB_OHCI_HCD) || !defined(CONFIG_USB_OHCI_HCD_MODULE)
		tcc_ohci_clkset(0);
#endif
		is_usb_otg = 0;
	}
	
	g_iExec = 0;
}

void tcc_ehci_clkset(int on)
{
	if (machine_is_tcc8800st() || machine_is_tcc8800() || 
		machine_is_m801_88() || machine_is_m803() || machine_is_tcc893x() ||
		machine_is_tcc8920()  || machine_is_tcc8920st() || machine_is_m805_892x() || 
		machine_is_m805_893x() || machine_is_tcc8930st()) 
	{
		if(on)
		{
			if(g_pEHCIClk){
				//printk("~~> [WARNING] EHCI clock is already turned on\n");
				return;
			}

#if defined(CONFIG_TCC_CLOCK_TABLE)
			tcc_cpufreq_set_limit_table((struct tcc_freq_table_t *)&gtHSIONormalClockLimitTable, TCC_FREQ_LIMIT_EHCI, 1);
#endif
			
			g_pEHCIClk = clk_get(NULL, "usb20h");
			if (IS_ERR(g_pEHCIClk)){
				printk("ERR - usb20h clk_get fail.\n");	
				return;
			}

			clk_enable(g_pEHCIClk);
			clk_set_rate(g_pEHCIClk, 48*1000*1000);
		}
		else
		{
			if(g_pEHCIClk){
				clk_disable(g_pEHCIClk);
			}

#if defined(CONFIG_TCC_CLOCK_TABLE)
			tcc_cpufreq_set_limit_table((struct tcc_freq_table_t *)&gtHSIONormalClockLimitTable, TCC_FREQ_LIMIT_EHCI, 0);
#endif
			g_pEHCIClk = NULL;
		}
	}
}

void tcc_xhci_clkset(int on)
{
   if(machine_is_m805_893x())
      return;

   g_pXHCIClk = clk_get(NULL, "usb_otg");
	if (IS_ERR(g_pXHCIClk)){
		printk("ERR - usb30h clk_get fail.\n");	
		return;
	}
	
	if(on)
	{
#if defined(CONFIG_TCC_CLOCK_TABLE)//taejin
		//tcc_cpufreq_set_limit_table((struct tcc_freq_table_t *)&gtHSIONormalClockLimitTable, TCC_FREQ_LIMIT_EHCI, 1);
#endif
		clk_enable(g_pXHCIClk);
		clk_set_rate(g_pXHCIClk, 48*1000*1000);
	}
	else
   {
      if(g_pXHCIClk){
         clk_disable(g_pXHCIClk);
      }
#if defined(CONFIG_TCC_CLOCK_TABLE)//taejin TODO
      //tcc_cpufreq_set_limit_table((struct tcc_freq_table_t *)&gtHSIONormalClockLimitTable, TCC_FREQ_LIMIT_EHCI, 0);
#endif
      g_pXHCIClk = NULL;
   }
}

void tcc_usb30_clkset(int on)
{
   if(machine_is_m805_893x())
      return;

   g_pXHCIClk = clk_get(NULL, "usb_otg");
   if (IS_ERR(g_pXHCIClk)){
      printk("ERR - usb30h clk_get fail.\n");   
      return;
   }

   if(on)
   {
#if defined(CONFIG_TCC_CLOCK_TABLE)//taejin
      //tcc_cpufreq_set_limit_table((struct tcc_freq_table_t *)&gtHSIONormalClockLimitTable, TCC_FREQ_LIMIT_EHCI, 1);
#endif
      clk_enable(g_pXHCIClk);
      clk_set_rate(g_pXHCIClk, 48*1000*1000);
   }
   else
   {
      struct clk *clk_hsiocfg = NULL;
      if(g_pXHCIClk){
         clk_disable(g_pXHCIClk);
      }
#if defined(CONFIG_TCC_CLOCK_TABLE)//taejin TODO
      //tcc_cpufreq_set_limit_table((struct tcc_freq_table_t *)&gtHSIONormalClockLimitTable, TCC_FREQ_LIMIT_EHCI, 0);
#endif
      return 0;
   }
}

/*************************************************************************************
OTG/OHCI/EHCI VBUS CONTROL
*************************************************************************************/
void tcc_otg_vbus_init(void)
{
	if(machine_is_tcc8800() || machine_is_tcc8920()){
		tcc_power_control(TCC_V5P_POWER, TCC_POWER_INIT);
		tcc_power_control(TCC_V5P_POWER, TCC_POWER_ON);
	}
}

void tcc_otg_vbus_exit(void)
{
	if(machine_is_tcc8800() || machine_is_tcc8920()){
		tcc_power_control(TCC_V5P_POWER, TCC_POWER_OFF);		
	}
}

/*
 * OTG VBUS Power Control. It is controlled GPIO_EXPAND DVBUS_ON.
 * - Host mode: DVBUS_ON is HIGH
 * - Device mode: DVBUS_ON is LOW
 */
void tcc_otg_vbus_ctrl(int port_index, int on)
{

}

/************************************************************************************/
int tcc_ohci_vbus_init(void)
{
	if (machine_is_tcc8800()){
		tcc_power_control(TCC_V5P_POWER, TCC_POWER_INIT);
		tcc_power_control(TCC_V5P_POWER, TCC_POWER_ON);
	}else if(machine_is_tcc8920()){

	}
	
	return 0;
	}

int tcc_ohci_vbus_exit(void)
{
	if (machine_is_tcc8800()){
		tcc_power_control(TCC_V5P_POWER, TCC_POWER_OFF);
	}else if(machine_is_tcc8920()){

	}
	
	return 0;
	}

int tcc_ohci_vbus_ctrl(int id, int on)
{
	if (id == 1){
		return 1;
	}else{
		if (machine_is_tcc8900()){
			int gpio_pwr_gp1, gpio_hvbus;

			gpio_pwr_gp1 = TCC_GPEXT3(4);
			gpio_hvbus = TCC_GPEXT3(1);

			gpio_request(gpio_pwr_gp1, "pwr_gp1");
			gpio_request(gpio_hvbus, "hvbus");

			if (on){
				gpio_direction_output(gpio_pwr_gp1, 1);
				gpio_direction_output(gpio_hvbus, 1);
			}else{
				gpio_direction_output(gpio_hvbus, 0);
			}
		}else if(machine_is_tcc8800()){
			int gpio_fs_host_en, gpio_host_bus_en;
			
			gpio_fs_host_en = TCC_GPEXT5(3);
			gpio_host_bus_en = TCC_GPEXT2(15);

			gpio_request(gpio_fs_host_en, "fs_host_en");
			gpio_request(gpio_host_bus_en, "host_bus_en");

			gpio_direction_output(gpio_fs_host_en, (on)?1:0);
			gpio_direction_output(gpio_host_bus_en, (on)?1:0);
		}
	}

	return 1;
}

/************************************************************************************/
int tcc_ehci_vbus_init(void)
{
   if (machine_is_tcc8800() || machine_is_tcc8920() || machine_is_tcc893x() || machine_is_m805_893x() || machine_is_m805_892x()){
		tcc_power_control(TCC_V5P_POWER, TCC_POWER_INIT);
		tcc_power_control(TCC_V5P_POWER, TCC_POWER_ON);
	}
	return 0;
}

int tcc_ehci_vbus_exit(void)
{
   if (machine_is_tcc8800() || machine_is_tcc8920() || machine_is_tcc893x() || machine_is_m805_893x() || machine_is_m805_892x()){
		tcc_power_control(TCC_V5P_POWER, TCC_POWER_OFF);
	}
	return 0;
}

int tcc_ehci_vbus_ctrl(int on)
{
	if (machine_is_tcc8800() || machine_is_tcc8920()|| machine_is_tcc893x())
	{
		int gpio_otg1_vbus_en, gpio_hs_host_en;

/**
 * @author valky@cleinsoft
 * @date 2013/07/25
 * D-Audio don't have gpio expander.
 * TODO: gpio port mapping.
 **/
#if defined(CONFIG_DAUDIO)
		gpio_hs_host_en     = TCC_GPA(4);
		gpio_otg1_vbus_en   = TCC_GPA(4);
#else
		gpio_hs_host_en = TCC_GPEXT5(2);
		gpio_otg1_vbus_en = TCC_GPEXT2(14);	
#endif
		
		gpio_request(gpio_hs_host_en, "hs_host_en");
		gpio_request(gpio_otg1_vbus_en, "otg1_vbus_en");		

		/* Don't control gpio_hs_host_en because this power also supported in USB core. */
		gpio_direction_output(gpio_hs_host_en, 1);	
		gpio_direction_output(gpio_otg1_vbus_en, (on)?1:0);

		return 0;
	}
	else if(machine_is_tcc8800st())
	{
		int gpio_host1_vbus_en;
		gpio_host1_vbus_en = TCC_GPB(16);
		gpio_request(gpio_host1_vbus_en, "host1_vbus_en");
		gpio_direction_output(gpio_host1_vbus_en, (on)?1:0);

		return 0;
	}
	else if(machine_is_tcc8920st() || machine_is_tcc8930st())
	{
		int gpio_host1_vbus_en;
		 	
		#if defined(CONFIG_STB_BOARD_HDB892F) || defined(CONFIG_STB_BOARD_YJ8935T) || defined(CONFIG_STB_BOARD_YJ8933T) || defined(CONFIG_STB_BOARD_UPC) || defined(CONFIG_STB_BOARD_UPC_TCC893X)
		gpio_host1_vbus_en = TCC_GPE(21);
		#else
		gpio_host1_vbus_en = TCC_GPC(25);
		#endif

		tcc_gpio_config(gpio_host1_vbus_en, GPIO_FN(0));
		gpio_request(gpio_host1_vbus_en, "host1_vbus_en");

		#if defined(CONFIG_STB_BOARD_UPC) || defined(CONFIG_STB_BOARD_UPC_TCC893X)
		gpio_direction_output(gpio_host1_vbus_en, (on)?0:1);
		#else
		gpio_direction_output(gpio_host1_vbus_en, (on)?1:0);
		#endif

		return 0;
	}
	else if(machine_is_m801_88() || machine_is_m803() || machine_is_m805_892x() || machine_is_m805_893x())
	{
		return 0;
	}

	return -1;
}
/************************************************************************************/
int tcc_xhci_vbus_init(void)
{
   if (machine_is_tcc8800() || machine_is_tcc8920() || machine_is_tcc893x() || machine_is_m805_892x()){
		tcc_power_control(TCC_V5P_POWER, TCC_POWER_INIT);
		tcc_power_control(TCC_V5P_POWER, TCC_POWER_ON);
	}

	return 0;
}

int tcc_xhci_vbus_exit(void)
{
   if (machine_is_tcc8800() || machine_is_tcc8920() || machine_is_tcc893x() || machine_is_m805_892x()){
		tcc_power_control(TCC_V5P_POWER, TCC_POWER_OFF);
	}

	return 0;
}

int tcc_xhci_vbus_ctrl(int on)
{
	if (machine_is_tcc893x())
	{	
		int gpio_xhci_vbus_en, gpio_xhci_en;

/**
 * @author valky@cleinsoft
 * @date 2013/07/25
 * D-Audio don't have gpio expander.
 * TODO: gpio port mapping.
 **/
#if defined(CONFIG_DAUDIO)
		gpio_xhci_en = TCC_GPA(4);
#else
		gpio_xhci_en = TCC_GPEXT5(1);		//power board expander
#endif

#if !(defined(CONFIG_TCC_USB_DRD) || defined(CONFIG_DAUDIO_USE_USB_MODE))
		gpio_xhci_vbus_en = TCC_GPEXT2(13);
#endif

		gpio_request(gpio_xhci_en, "gpio_xhci_en");
#if !(defined(CONFIG_TCC_USB_DRD) || defined(CONFIG_DAUDIO_USE_USB_MODE))
		gpio_request(gpio_xhci_vbus_en, "gpio_xhci_vbus_en");		
#endif

      /* Don't control gpio_hs_host_en because this power also supported in USB core. */
		gpio_direction_output(gpio_xhci_en, (on)?1:0);	
#if !(defined(CONFIG_TCC_USB_DRD) || defined(CONFIG_DAUDIO_USE_USB_MODE))
		gpio_direction_output(gpio_xhci_vbus_en, (on)?1:0);
#endif

		return 0;
	}else if (machine_is_tcc8930st()){
		int gpio_xhci_vbus_en;

		gpio_xhci_vbus_en = TCC_GPD(11);
      gpio_request(gpio_xhci_vbus_en, "gpio_xhci_vbus_en");
      gpio_direction_output(gpio_xhci_vbus_en, (on)?1:0);

      return 0;
   }else if(machine_is_m805_893x()){
      // usb ip power on/off
      // usb vbus en
      int gpio_otg0_vbus_en;
      gpio_otg0_vbus_en = TCC_GPD(3);
      gpio_request(gpio_otg0_vbus_en, "gpio_otg0_vbus_en");
      gpio_direction_output(gpio_otg0_vbus_en, (on)?1:0);

      return 0;
   }
   return -1;
}

/************************************************************************************/
int tcc_usb30_vbus_init(void)
{
   if (machine_is_tcc8800() || machine_is_tcc8920() || machine_is_tcc893x() || machine_is_m805_893x() || machine_is_m805_892x()){
      tcc_power_control(TCC_V5P_POWER, TCC_POWER_INIT);
      tcc_power_control(TCC_V5P_POWER, TCC_POWER_ON);
   }
   return 0;
}

int tcc_usb30_vbus_exit(void)
{
   if (machine_is_tcc8800() || machine_is_tcc8920() || machine_is_tcc893x() || machine_is_m805_893x() || machine_is_m805_892x()){
      tcc_power_control(TCC_V5P_POWER, TCC_POWER_OFF);
   }
   return 0;
}
#if defined(CONFIG_DAUDIO_20140220)
	#define GPIO_USB30_5V_CTL	TCC_GPD(11)
	#define GPIO_USB_MODE		TCC_GPB(4)
#endif

int tcc_dwc3_vbus_ctrl(int on)
{
	if (machine_is_tcc893x())
	{	
		int gpio_otg0_vbus_en, gpio_otg_en;
		gpio_otg_en = TCC_GPA(4);

//#if defined(CONFIG_DAUDIO)
#if 0
		gpio_request(GPIO_USB30_5V_CTL, "USB30_5V_CTL");
		gpio_request(GPIO_USB_MODE, "USB_MODE");

		if (on > 0)
		{
			gpio_direction_output(GPIO_USB_MODE, 0);
			gpio_direction_output(GPIO_USB30_5V_CTL, 0);
		}
		else
		{
			gpio_direction_output(GPIO_USB_MODE, 1);
			gpio_direction_output(GPIO_USB30_5V_CTL, 1);
		}
#else
		gpio_otg_en = TCC_GPEXT5(1);		//power board expander
#endif
		gpio_request(gpio_otg_en, "otg_host_en");

		/* Don't control gpio_hs_host_en because this power also supported in USB core. */
		gpio_direction_output(gpio_otg_en, (on) ? 1 : 0);	

		return 0;
	}

	return 0;
}

void tcc_usb30_phy_ctrl(int on)
{
   if(machine_is_m805_893x() && !on)
      return;

   tca_ckc_setippwdn(PMU_ISOL_USBOP, !on);
}

void tcc_usb30_phy_on(void)
{
   tcc_usb30_phy_ctrl(ON);
}

void tcc_usb30_phy_off(void)
{
   tcc_usb30_phy_ctrl(OFF);
}

void tcc_xhci_phy_ctrl(int on)
{
   if(machine_is_m805_893x() && !on)
      return;

   tca_ckc_setippwdn(PMU_ISOL_USBOP, !on);
}

void tcc_xhci_phy_on(void)
{
   tcc_usb30_phy_ctrl(ON);
}

void tcc_xhci_phy_off(void)
{
   tcc_usb30_phy_ctrl(OFF);
}


/*************************************************************************************
EXPORT DEFINES
*************************************************************************************/
EXPORT_SYMBOL(tcc_otg_vbus_init);
EXPORT_SYMBOL(tcc_otg_vbus_exit);

EXPORT_SYMBOL(tcc_ohci_vbus_init);
EXPORT_SYMBOL(tcc_ohci_vbus_exit);
EXPORT_SYMBOL(tcc_ohci_vbus_ctrl);

EXPORT_SYMBOL(tcc_ehci_vbus_init);
EXPORT_SYMBOL(tcc_ehci_vbus_exit);
EXPORT_SYMBOL(tcc_ehci_vbus_ctrl);

EXPORT_SYMBOL(tcc_xhci_vbus_init);
EXPORT_SYMBOL(tcc_xhci_vbus_exit);

EXPORT_SYMBOL(tcc_usb30_vbus_init);
EXPORT_SYMBOL(tcc_usb30_vbus_exit);
EXPORT_SYMBOL(tcc_xhci_vbus_ctrl);
EXPORT_SYMBOL(tcc_dwc3_vbus_ctrl);

EXPORT_SYMBOL(tcc_ohci_clock_control);
EXPORT_SYMBOL(tcc_ohci_clkset);
EXPORT_SYMBOL(tcc_ehci_clkset);
EXPORT_SYMBOL(tcc_xhci_clkset);
EXPORT_SYMBOL(tcc_usb30_clkset);

EXPORT_SYMBOL(tcc_usb30_phy_on);
EXPORT_SYMBOL(tcc_usb30_phy_off);
EXPORT_SYMBOL(tcc_usb30_phy_ctrl);

EXPORT_SYMBOL(tcc_xhci_phy_on);
EXPORT_SYMBOL(tcc_xhci_phy_off);
EXPORT_SYMBOL(tcc_xhci_phy_ctrl);

