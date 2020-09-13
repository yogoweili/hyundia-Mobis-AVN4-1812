#
# (C) Copyright 2000-2003
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# SPDX-License-Identifier:	GPL-2.0+
#

extra-y	:= start.o

obj-y	+= cache_v7.o
obj-y	+= cpu.o
obj-y	+= syslib.o

ifneq (,$(filter tcc893x,$(SOC)))
obj-y += tcc-common/
endif
