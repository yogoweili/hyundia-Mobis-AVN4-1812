#
# (C) Copyright 2000, 2001, 2002
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# (C) Copyright 2008
# Guennadi Liakhovetki, DENX Software Engineering, <lg@denx.de>
#
# SPDX-License-Identifier:	GPL-2.0+
#

obj-y	:= tcc893x_evm.o
#obj-y	+= tcc_chip_rev.o
obj-y	+= lowlevel_init.o
obj-y	+= atags.o
obj-y	+= clock.o
#obj-y	+= disk.o
#obj-y	+= sfl.o
obj-$(CONFIG_TCC_SERIAL)	+= uart.o
obj-$(CONFIG_SYS_I2C_INIT_BOARD)	+= i2c.o
obj-$(CONFIG_DESIGNWARE_ETH) += gmac.o
