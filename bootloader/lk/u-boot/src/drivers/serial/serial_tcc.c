/*
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <serial.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/reg_physical.h>
#include <asm/arch/uart.h>

DECLARE_GLOBAL_DATA_PTR;

#ifndef BITCSET
#define BITCSET(X, CMASK, SMASK)	( (X) = ((((unsigned int)(X)) & ~((unsigned int)(CMASK))) | ((unsigned int)(SMASK))) )
#endif

struct uart_stat {
	unsigned long base;
	int ch;
};

static const struct uart_stat uart[] = {
	{ HwUART0_BASE, 0 },
};

extern struct uart_port_map_type uart_port_map[];
extern unsigned int NUM_UART_PORT;


void uart_set_port_mux(unsigned int ch, unsigned int port)
{
	unsigned int idx;
	PUARTPORTCFG pUARTPORTCFG = (PUARTPORTCFG)HwUART_PORTCFG_BASE;

	for (idx=0 ; idx<NUM_UART_PORT ; idx++) {
		if (uart_port_map[idx].id == port)
			break;
	}

	if (idx >= NUM_UART_PORT) {
		return;
	}

	if (ch < 4)
		BITCSET(pUARTPORTCFG->PCFG0.nREG, 0xFF<<(ch*8), port<<(ch*8));
	else if (ch < 8)
		BITCSET(pUARTPORTCFG->PCFG1.nREG, 0xFF<<((ch-4)*8), port<<((ch-4)*8));

	if (ch == CONFIG_DEBUG_UART) {
		gpio_config(uart_port_map[idx].tx_port, uart_port_map[idx].fn_sel);	// TX
		gpio_config(uart_port_map[idx].rx_port, uart_port_map[idx].fn_sel|GPIO_PULLUP);	// RX
	}
}

static void uart_set_gpio(void)
{
	PUARTPORTCFG pUARTPORTCFG = (PUARTPORTCFG)HwUART_PORTCFG_BASE;

	//Bruce, should be initialized to not used port.
	pUARTPORTCFG->PCFG0.nREG = 0xFFFFFFFF;
	pUARTPORTCFG->PCFG1.nREG = 0xFFFFFFFF;

#ifdef	CONFIG_TCC_SERIAL0_PORT 
	uart_set_port_mux(0, CONFIG_TCC_SERIAL0_PORT);
#endif
}

static void uart_init_port(int port, uint baud)
{
	uint16_t baud_divisor = (CONFIG_TCC_SERIAL_CLK/ 16 / baud);

	uart_reg_write(port, UART_LCR, LCR_EPS | LCR_STB | LCR_WLS_8);	/* 8 data, 1 stop, no parity */
	uart_reg_write(port, UART_IER, 0);
	uart_reg_write(port, UART_LCR, LCR_BKSE | LCR_EPS | LCR_STB | LCR_WLS_8);	/* 8 data, 1 stop, no parity */
	uart_reg_write(port, UART_DLL, baud_divisor & 0xff);
	uart_reg_write(port, UART_DLM, (baud_divisor >> 8) & 0xff);
	uart_reg_write(port, UART_FCR, FCR_FIFO_EN | FCR_RXFR | FCR_TXFR | Hw4 | Hw5);
	uart_reg_write(port, UART_LCR, LCR_EPS | LCR_STB | LCR_WLS_8);	/* 8 data, 1 stop, no parity */
}

int uart_putc(int port, char c )
{
	/* wait for the last char to get out */
	while (!(uart_reg_read(port, UART_LSR) & LSR_THRE))
		;
	uart_reg_write(port, UART_THR, c);
	return 0;
}

int uart_getc(int port, bool wait)  /* returns -1 if no data available */
{
	if (wait) {
		/* wait for data to show up in the rx fifo */
		while (!(uart_reg_read(port, UART_LSR) & LSR_DR))
			;
	} else {
		if (!(uart_reg_read(port, UART_LSR) & LSR_DR))
			return -1;
	}
	return uart_reg_read(port, UART_RBR);
}

void uart_flush_tx(int port)
{
	/* wait for the last char to get out */
	while (!(uart_reg_read(port, UART_LSR) & LSR_TEMT))
		;
}

void uart_flush_rx(int port)
{
	/* empty the rx fifo */
	while (uart_reg_read(port, UART_LSR) & LSR_DR) {
		volatile char c = uart_reg_read(port, UART_RBR);
		(void)c;
	}
}

static void serial_setbrg_dev(const int dev_index)
{
	uart_init_port(dev_index, gd->baudrate);
	uart_flush_rx(dev_index);
}

/*
 * Initialise the serial port with the given baudrate. The settings
 * are always 8 data bits, no parity, 1 stop bit, no start bits.
 */
static int serial_init_dev(const int dev_index)
{
	uart_set_gpio();
	serial_setbrg_dev(dev_index);

	return 0;
}

/*
 * Read a single byte from the serial port. Returns 1 on success, 0
 * otherwise. When the function is succesfull, the character read is
 * written into its argument c.
 */
static int serial_getc_dev(const int dev_index)
{
	return uart_getc(dev_index, 1);
}

/*
 * Output a single byte to the serial port.
 */
static void serial_putc_dev(const char c, const int dev_index)
{
	if (c == '\n')
		uart_putc(dev_index, '\r');
	uart_putc(dev_index, c);
}

/*
 * Test whether a character is in the RX buffer
 */
static int serial_tstc_dev(const int dev_index)
{
	return (uart_reg_read(dev_index, UART_LSR) & LSR_DR) != 0;
}

static void serial_puts_dev(const char *s, const int dev_index)
{
	while(*s != 0) {
		serial_putc_dev(*s++, dev_index);
	}
}

/* Multi serial device functions */
#define DECLARE_TCC_SERIAL_FUNCTIONS(port) \
static int tcc_serial##port##_init(void) { return serial_init_dev(port); } \
static void tcc_serial##port##_setbrg(void) { serial_setbrg_dev(port); } \
static int tcc_serial##port##_getc(void) { return serial_getc_dev(port); } \
static int tcc_serial##port##_tstc(void) { return serial_tstc_dev(port); } \
static void tcc_serial##port##_putc(const char c) { serial_putc_dev(c, port); } \
static void tcc_serial##port##_puts(const char *s) { serial_puts_dev(s, port); }

#define INIT_TCC_SERIAL_STRUCTURE(port, __name) {	\
	.name	= __name,				\
	.start	= tcc_serial##port##_init,		\
	.stop	= NULL,					\
	.setbrg	= tcc_serial##port##_setbrg,		\
	.getc	= tcc_serial##port##_getc,		\
	.tstc	= tcc_serial##port##_tstc,		\
	.putc	= tcc_serial##port##_putc,		\
	.puts	= tcc_serial##port##_puts,		\
}

DECLARE_TCC_SERIAL_FUNCTIONS(0);
struct serial_device tcc_serial0_device =
	INIT_TCC_SERIAL_STRUCTURE(0, "tccser0");

__weak struct serial_device *default_serial_console(void)
{
#if defined(CONFIG_TCC_SERIAL0_PORT) && (CONFIG_DEBUG_UART == 0)
	return &tcc_serial0_device;
#else
	#error "TCC Serial debug port configuration error"
#endif
}

void tcc_serial_initialize(void)
{
#ifdef CONFIG_TCC_SERIAL0_PORT
	serial_register(&tcc_serial0_device);
#else
	#error "TCC Serial Port not defined"
#endif

}
