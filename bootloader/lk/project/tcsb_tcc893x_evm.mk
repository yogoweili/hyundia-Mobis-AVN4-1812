# top level project rules for the tcc8920_evm project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

include $(LOCAL_DIR)/tcc893x_evm.mk

################################################
## SECURE BOOT
################################################
NAND_BOOT := 1
DEFINES += TSBM_ENABLE
TSBM_INCLUDE := 1


