#ifndef _TCC_UART_H_
#define _TCC_UART_H_

#define uart_reg_write(p, a, v) writel(v, uart[p].base + (a))
#define uart_reg_read(p, a)     readl(uart[p].base + (a))

#define UART_RBR    0x00        /* receiver buffer register */
#define UART_THR    0x00        /* transmitter holding register */
#define UART_DLL    0x00        /* divisor latch (LSB) */
#define UART_IER    0x04        /* interrupt enable register */
#define UART_DLM    0x04        /* divisor latch (MSB) */
#define UART_IIR    0x08        /* interrupt ident. register */
#define UART_FCR    0x08        /* FIFO control register */
#define UART_LCR    0x0C        /* line control register */
#define UART_MCR    0x10        /* MODEM control register */
#define UART_LSR    0x14        /* line status register */
#define UART_MSR    0x18        /* MODEM status register */
#define UART_SCR    0x1C        /* scratch register */
#define UART_AFT    0x20        /* AFC trigger level register */
#define UART_SIER   0x50        /* interrupt enable register */

#define LCR_WLS_MSK 0x03        /* character length select mask */
#define LCR_WLS_5   0x00        /* 5 bit character length */
#define LCR_WLS_6   0x01        /* 6 bit character length */
#define LCR_WLS_7   0x02        /* 7 bit character length */
#define LCR_WLS_8   0x03        /* 8 bit character length */
#define LCR_STB     0x04        /* Number of stop Bits, off = 1, on = 1.5 or 2) */
#define LCR_PEN     0x08        /* Parity eneble */
#define LCR_EPS     0x10        /* Even Parity Select */
#define LCR_STKP    0x20        /* Stick Parity */
#define LCR_SBRK    0x40        /* Set Break */
#define LCR_BKSE    0x80        /* Bank select enable */

#define FCR_FIFO_EN 0x01        /* FIFO enable */
#define FCR_RXFR    0x02        /* receiver FIFO reset */
#define FCR_TXFR    0x04        /* transmitter FIFO reset */

#define MCR_RTS     0x02

#define LSR_DR      0x01
#define LSR_THRE    0x20        /* transmitter holding register empty */
#define LSR_TEMT    0x40        /* transmitter empty */

struct uart_port_map_type {
	unsigned int    id;
	unsigned int    tx_port;
	unsigned int    rx_port;
	unsigned int    cts_port;
	unsigned int    rts_port;
	unsigned int    fn_sel;
};

#define NC_PORT 0xFFFFFFFF




#endif /*_TCC_UART_H_*/
