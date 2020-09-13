# top level project rules for the tcc893x_evm project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := tcc893x_evm

MODULES += app/aboot

#DEFINES += WITH_DEBUG_JTAG=1
DEFINES += WITH_DEBUG_UART=1
#DEFINES += WITH_DEBUG_FBCON=1

TCC893X := 1

#TCC8935S := 1
#TCC8933S := 1
#TCC8937S := 1

################################################
##TNFTL V8 DEFINES
################################################
TNFTL_V8_USE := 1

DEFINES += TNFTL_V8_INCLUDE
