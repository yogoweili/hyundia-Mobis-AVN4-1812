LOCAL_DIR := $(GET_LOCAL_DIR)

ARCH := arm
ARM_CPU := cortex-a9
CPU := generic


ifeq ($(TCC_SNAPSHOT_USE_LZ4),1)
CACHE_PL310_USE := 1
else
ifeq ($(TARGET_BOARD_STB),true)
CACHE_PL310_USE := 1 
else
CACHE_PL310_USE := 1
endif
endif

INCLUDES += -I$(LOCAL_DIR)/include 

DEFINES += \
	WITH_CPU_EARLY_INIT=1

DEFINES += TCC893X=1

DEFINES += \
	_LINUX_ \
	NAND_BOOT_REV \
	FWDN_V7

ifneq ($(EMMC_BOOT),1)
################################################
##NAND
################################################
DEFINES += NAND_INCLUDE 
DEFINES += NAND_BOOT_INCLUDE 

ifneq ($(TNFTL_V8_USE),1)
DEFINES += INTERNAL_HIDDEN_STORAGE_INCLUDE
endif

else
################################################
##SDMMC DEFINES
################################################
DEFINES += BOOTSD_INCLUDE
DEFINES += BOOTSD_BOOT_INCLUDE
DEFINES += BOOTSD_KERNEL_INCLUDE
DEFINES += FEATURE_SDMMC_MMC43_BOOT
endif

ifeq ($(TSBM_INCLUDE),1)
LIBS += \
      $(LOCAL_DIR)/libs/atsb_lib.a
endif


MODULES += dev/fbcon
		
OBJS += \
	$(LOCAL_DIR)/cpu_early_init.Ao \
	$(LOCAL_DIR)/platform.o \
	$(LOCAL_DIR)/interrupts.Ao \
	$(LOCAL_DIR)/tcc_ddr.Ao \
	$(LOCAL_DIR)/clock.Ao \
	$(LOCAL_DIR)/gpio.o \
	$(LOCAL_DIR)/uart.o \

ifeq ($(TCC_CHIP_REV), 1)
OBJS += $(LOCAL_DIR)/tcc_chip_rev.o
endif


ifeq ($(TCC_I2C_USE), 1)
OBJS += $(LOCAL_DIR)/i2c.o
endif

ifeq ($(TCC_LCD_USE), 1)
OBJS += \
	$(LOCAL_DIR)/lcdc.o \
	$(LOCAL_DIR)/tcc_lcd_interface.o

OBJS += \
	$(LOCAL_DIR)/vioc/vioc_rdma.o \
	$(LOCAL_DIR)/vioc/vioc_wdma.o \
	$(LOCAL_DIR)/vioc/vioc_wmix.o \
	$(LOCAL_DIR)/vioc/vioc_outcfg.o \
	$(LOCAL_DIR)/vioc/vioc_disp.o \
	$(LOCAL_DIR)/vioc/vioc_lut.o
endif

ifeq ($(TCC_HDMI_USE), 1)
  ifeq ($(BOARD_HDMI_V1_3),1)
  OBJS += \
  	$(LOCAL_DIR)/hdmi_v1_3.o
  else
  OBJS += \
	$(LOCAL_DIR)/hdmi_v1_4.o
  endif
endif

ifeq ($(BOOT_REMOCON), 1)
OBJS += \
	$(LOCAL_DIR)/tcc_asm.Ao \
	$(LOCAL_DIR)/pm.Ao
endif

LINKER_SCRIPT += $(BUILDDIR)/system-onesegment.ld


ifneq ($(EMMC_BOOT),1)

ifeq ($(TCC_NAND_USE), 1)
ifeq ($(TNFTL_V8_USE),1)
LIBS += \
		$(LOCAL_DIR)/libs/libtnftl_v8_TCC893X_BL.lo 
else
LIBS += \
		$(LOCAL_DIR)/libs/libtnftl_V7081_TCC8920_BL.lo 
#		$(LOCAL_DIR)/libx/libkfs_TCC8800/.lo 
endif
endif

endif

ifeq ($(TCC_CM3_USE), 1)
OBJS += \
	$(LOCAL_DIR)/tcc_cm3_control.o \
	$(LOCAL_DIR)/cm3_lock.o 
endif

include platform/tcc_shared/rules.mk
