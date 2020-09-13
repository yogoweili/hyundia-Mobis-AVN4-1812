#
# Makefile for the touchscreen drivers.
#

# Each configuration option enables a list of files.

ifeq ($(CONFIG_TOUCHSCREEN_SOLOMON_SSD2532),y)
	obj-$(CONFIG_TOUCHSCREEN_SOLOMON_SSD2532)	+= ssd253x_ts.o
else
	obj-$(CONFIG_TOUCHSCREEN_ATMEL_MXT_336S_AT)	+= atmel_mxt336S.o atmel_mxt336S_cfg.o atmel_mxt336S_parse.o
endif
