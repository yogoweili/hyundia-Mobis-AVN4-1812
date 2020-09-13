#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <mach/gpio.h>
#include <mach/irqs.h>
#include <mach/bsp.h>
#include <asm/mach-types.h>

#if defined(CONFIG_CHIP_TCC8930)
/**
 * @author valky@cleinsoft
 * @date 2014/02/20
 * uart port mapping for daudio
 * sjpark : remap uart port
 * valky  : uart port mapping for daudio 2nd board.
 * valky  : uart port mapping for daudio 3rd board.
 **/
#if defined(CONFIG_DAUDIO)
#include <mach/daudio.h>
#include <mach/daudio_info.h>

#if defined(CONFIG_DAUDIO_20140220)
#define UART_PORT_COUNT	6

static int uart_port_map[UART_PORT_COUNT][5] = {
//      tx          rx          rts             cts         fn
	{TCC_GPF(30), TCC_GPF(31),           0,           0, GPIO_FN(9) },  // UART16 DEBUG
	{TCC_GPF(13), TCC_GPF(14), TCC_GPF(15), TCC_GPF(16), GPIO_FN(9) },  // UART14 BT
	{TCC_GPB(25), TCC_GPB(26),           0,           0, GPIO_FN(10)},  // UART08 Micom
	{TCC_GPB(11), TCC_GPB(12),           0,           0, GPIO_FN(10)},  // UART06 GPS
	{TCC_GPD(13), TCC_GPD(14),           0,           0, GPIO_FN(4) },  // UART03 SIRIUS
	{TCC_GPB(19), TCC_GPB(20),           0,           0, GPIO_FN(10)},  // UART07 FM1288
};
#endif
#endif
#endif

#if defined(CONFIG_DAUDIO)
static int uart_port_map_main4th[][5] = {
	//   tx           rx             rts         cts          fn
	{TCC_GPG(5), TCC_GPG(6),           0,           0, GPIO_FN(3)},  // UART21 EX_CDP
};
#endif

int uart_port_map_selection[8][5];
EXPORT_SYMBOL(uart_port_map_selection);

static int __init tcc8930_init_uart_map(void)
{
	int i;
#if defined(CONFIG_DAUDIO)
	int main_ver = daudio_main_version();
#endif

	printk(KERN_INFO "%s\n", __func__);

	for(i = 0; i < 8; i++) //initial data [0]
		uart_port_map_selection[i][0] = -1;

#if defined(CONFIG_CHIP_TCC8930)
	/**
	 * @author valky@cleinsoft
	 * @date 2014/02/20
	 * uart port mapping for daudio
	 * sjpark : remap uart port
	 * valky  : remap uart port
	 **/
	#if defined(CONFIG_DAUDIO)
	if (system_rev == 0x1000) {
		for (i = 0; i < 5; i++) {
			uart_port_map_selection[0][i] = uart_port_map[0][i]; //DEBUG
			uart_port_map_selection[1][i] = uart_port_map[1][i]; //BT
			uart_port_map_selection[2][i] = uart_port_map[2][i]; //MICOM
			uart_port_map_selection[3][i] = uart_port_map[3][i]; //GPS
			uart_port_map_selection[4][i] = uart_port_map[4][i]; //SIRIUS
		#if defined(CONFIG_DAUDIO_20140220)
			uart_port_map_selection[5][i] = uart_port_map[5][i]; //FM1288
		#endif

			switch (main_ver)
			{
				case DAUDIO_BOARD_6TH:
				case DAUDIO_BOARD_5TH:
				default:
					uart_port_map_selection[6][i] = uart_port_map_main4th[0][i]; //EX_CDP
					break;
			}
		}
	}
	#else
	if (system_rev == 0x1000) {
		for(i = 0; i < 5; i++) {
			uart_port_map_selection[0][i] = uart_port_map[28][i]; // console
			uart_port_map_selection[1][i] = uart_port_map[27][i]; // BT
			uart_port_map_selection[3][i] = uart_port_map[8][i]; // GPS
		}
	}
	#endif
#endif

	return 0;
}

device_initcall(tcc8930_init_uart_map);
