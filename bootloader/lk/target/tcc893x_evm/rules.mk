LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += -I$(LOCAL_DIR)/include -I$(LK_TOP_DIR)/platform/tcc893x

PLATFORM := tcc893x

#------------------------------------------------------------------
# Define board revision
# 0x1000 : TCC8930_D3_08X4_SV0.1 - DDR3 1024MB(32Bit)
# 0x2000 : TCC8935X_D3_08X4_2CS_SV0.2 - DDR3 1024MB(16Bit)
# 0x3000 : TCC8933_D3_08X4_SV0.1 - DDR3 1024MB(32Bit)
HW_REV=0x1000
#HW_REV=0x2000
#HW_REV=0x3000

#DEFINES += JTAG_BOOT

#==================================================================
# Chipset Information
#==================================================================
#------------------------------------------------------------------
# CHIPSET TYPE
#------------------------------------------------------------------
ifneq ($(filter $(HW_REV),0x2000),)
 DEFINES += CONFIG_CHIP_TCC8935
else
 ifneq ($(filter $(HW_REV),0x3000),)
  DEFINES += CONFIG_CHIP_TCC8933
 else
  DEFINES += CONFIG_CHIP_TCC8930
 endif
endif

#------------------------------------------------------------------
# TCC8935S/TCC8933S/TCC8937S Sub SoC type
#------------------------------------------------------------------
ifeq ($(TCC8935S),1)
DEFINES += CONFIG_CHIP_TCC8935S
endif
ifeq ($(TCC8933S),1)
DEFINES += CONFIG_CHIP_TCC8933S
endif
ifeq ($(TCC8937S),1)
DEFINES += CONFIG_CHIP_TCC8937S
endif

#==================================================================
# Memory ODT Setting
#==================================================================
#DEFINES += CONFIG_ODT_TCC8931

#==================================================================
# System Setting
#==================================================================

#------------------------------------------------------------------
# Use Audio PLL Option
#------------------------------------------------------------------
ifneq ($(filter $(DEFINES),CONFIG_CHIP_TCC8930 CONFIG_CHIP_TCC8933 CONFIG_CHIP_TCC8935),)
#DEFINES += CONFIG_AUDIO_PLL_USE
endif

#------------------------------------------------------------------
# BASE Address
#------------------------------------------------------------------
BASE_ADDR        := 0x80000000
MEMBASE          := 0x82000000
MEMSIZE          := 0x01000000 # 16MB

#------------------------------------------------------------------
TAGS_ADDR        := BASE_ADDR+0x00000100
KERNEL_ADDR      := BASE_ADDR+0x00008000
RAMDISK_ADDR     := BASE_ADDR+0x01000000
SKERNEL_ADDR     := BASE_ADDR+0x04000000
#SCRATCH_ADDR     := BASE_ADDR+0x06000000
SCRATCH_ADDR     := BASE_ADDR+0x30000000

#==================================================================
# SDRAM Setting
#==================================================================
#------------------------------------------------------------------
# SDRAM CONTROLLER TYPE
#------------------------------------------------------------------
#TCC_MEM_TYPE := DRAM_LPDDR2
#TCC_MEM_TYPE := DRAM_DDR2
TCC_MEM_TYPE := DRAM_DDR3

#------------------------------------------------------------------
# Define memory bus width
#------------------------------------------------------------------
ifneq ($(filter $(HW_REV),0x2000),)
 DEFINES += CONFIG_DRAM_16BIT_USED
else
 DEFINES += CONFIG_DRAM_32BIT_USED
endif

#------------------------------------------------------------------
# Define memory auto training
#------------------------------------------------------------------
ifneq ($(filter $(DEFINES),CONFIG_DRAM_32BIT_USED),)
 ifeq ($(TCC_MEM_TYPE), DRAM_DDR3)
  DEFINES += CONFIG_DRAM_AUTO_TRAINING
 endif
endif

#------------------------------------------------------------------
# Define memory size in MB
#------------------------------------------------------------------
#TCC_MEM_SIZE := 512
TCC_MEM_SIZE := 1024
#TCC_MEM_SIZE := 2048

ifeq ($(TCC_MEM_SIZE), 256)
 DEFINES += CONFIG_TCC_MEM_256MB
endif
ifeq ($(TCC_MEM_SIZE), 512)
 DEFINES += CONFIG_TCC_MEM_512MB
endif
ifeq ($(TCC_MEM_SIZE), 1024)
 DEFINES += CONFIG_TCC_MEM_1024MB
endif
ifeq ($(TCC_MEM_SIZE), 2048)
 DEFINES += CONFIG_TCC_MEM_2048MB
endif

#------------------------------------------------------------------
# SDRAM DDR2 Vender
#------------------------------------------------------------------
ifeq ($(TCC_MEM_TYPE), DRAM_DDR2)
 #DEFINES += CONFIG_DDR2_CT83486C1
 DEFINES += CONFIG_DDR2_HY5PS1G831CFPS6
 #DEFINES += CONFIG_DDR2_HY5PS1G1631CFPS6
 #DEFINES += CONFIG_DDR2_HXB18T2G160AF
 #DEFINES += CONFIG_DDR2_K4T1G084QF_BCE7
 #DEFINES += CONFIG_DDR2_K4T1G164QE_HCF7
 #DEFINES += CONFIG_DDR2_MT47H256M8EB25E

 DEFINES += DRAM_DDR2
endif

#------------------------------------------------------------------
# SDRAM DDR3 Vender
#------------------------------------------------------------------
ifeq ($(TCC_MEM_TYPE), DRAM_DDR3)
 #DEFINES += CONFIG_DDR3_MT41K256M16HA_ITE
 #DEFINES += CONFIG_DDR3_H5TQ4G83AFR_PBC
 #DEFINES += CONFIG_DDR3_H5TC4G83MFR_H9A
 #DEFINES += CONFIG_DDR3_H5TQ1G83BFR_H9C
 #DEFINES += CONFIG_DDR3_H5TQ2G63BFR_H9C
 #DEFINES += CONFIG_DDR3_H5TQ2G63BFR_PBC
 #DEFINES += CONFIG_DDR3_H5TQ2G83BFR_H9C
 DEFINES += CONFIG_DDR3_H5TQ2G83BFR_PBC
 #DEFINES += CONFIG_DDR3_IN5CB256M8BN_CG
 #DEFINES += CONFIG_DDR3_K4B1G1646E_HCH9
 #DEFINES += CONFIG_DDR3_K4B2G1646C_HCK0
 #DEFINES += CONFIG_DDR3_NT5CB256M8GN_CG
 #DEFINES += CONFIG_DDR3_PRN256M8V89CG8GQF_15E

 DEFINES += DRAM_DDR3
endif

#------------------------------------------------------------------
ifeq ($(TCC_MEM_TYPE), DRAM_LPDDR2)
 DEFINES += CONFIG_LPDDR2_K4P4G324EB_AGC1

 DEFINES += DRAM_LPDDR2
endif


#==================================================================
# Device Driver Setting
#==================================================================
#------------------------------------------------------------------
# Power Managemant IC
#------------------------------------------------------------------
#DEFINES += ACT8810_PMIC
DEFINES += AXP192_PMIC
#DEFINES += AXP202_PMIC
#DEFINES += RN5T614_PMIC

#------------------------------------------------------------------
# Display Type
#------------------------------------------------------------------
DEFINES += DEFAULT_DISPLAY_LCD

# Define Default Splash
DEFINES += DISPLAY_SPLASH_SCREEN
#DEFINES += DISPLAY_SPLASH_SCREEN_DIRECT

DEFINES += FB_TCC_OVERLAY_EXT

# Support video displaying by hw vsyn interrupt
TCC_VIDEO_DISPLAY_BY_VSYNC_INT := true
#TCC_VIDEO_DISPLAY_BY_VSYNC_INT := false

ifeq ($(TCC_VIDEO_DISPLAY_BY_VSYNC_INT), true)
DEFINES += TCC_VIDEO_DISPLAY_BY_VSYNC_INT
endif

# Defines LCD panel
#DEFINES += DX08D11VM0AAA # 480x800
#DEFINES += LMS350DF01    # 320x480
#DEFINES += AT070TN93      # 800x480
#DEFINES += ED090NA       # 1280x800 COBY
#DEFINES += KR080PA2S     # 1024x768 EMDOOR 
#DEFINES += LMS480KF      # 800x480
#DEFINES += CLAA070NP01   # 1024_600 
#DEFINES += HV070WSA      # 1024_600 
DEFINES += FLD0800	  # 1024x600 
#DEFINES += CLAA070WP03	  # 800x1280 YECON MIT700
#DEFINES += LMS700KF23
#DEFINES += EJ070NA	  # 1024x600 LVDS
#DEFINES += LB070WV7	  # 800x480 LG

#------------------------------------------------------------------
# Keypad Type
#------------------------------------------------------------------
KEYS_USE_GPIO_KEYPAD := 1
#KEYS_USE_ADC_KEYPAD := 1


#------------------------------------------------------------------
MODULES += \
	dev/keys \
	dev/pmic \
	lib/ptable

DEFINES += \
	SDRAM_SIZE=$(MEMSIZE) \
	MEMBASE=$(MEMBASE) \
	BASE_ADDR=$(BASE_ADDR) \
	TAGS_ADDR=$(TAGS_ADDR) \
	KERNEL_ADDR=$(KERNEL_ADDR) \
	RAMDISK_ADDR=$(RAMDISK_ADDR) \
	SCRATCH_ADDR=$(SCRATCH_ADDR) \
	SKERNEL_ADDR=$(SKERNEL_ADDR) \
	TCC_MEM_SIZE=$(TCC_MEM_SIZE) \
	HW_REV=$(HW_REV)

# Defines debug level
DEBUG := 1

#------------------------------------------------------------------
OBJS += \
	$(LOCAL_DIR)/init.o \
	$(LOCAL_DIR)/clock.o \
	$(LOCAL_DIR)/gpio.o \
	$(LOCAL_DIR)/keypad.o \
	$(LOCAL_DIR)/atags.o


TCC_I2C_USE := 1
TCC_LCD_USE := 1
TCC_NAND_USE := 1
TCC_FWDN_USE := 1
TCC_USB_USE := 1
#TCC_GPSB_USE := 1
TCC_PCA953X_USE := 1
TCC_SPLASH_USE := 1
TCC_HDMI_USE := 1
TCC_CHIP_REV :=1
TCC_CM3_USE := 1

ifeq ($(TCC_I2C_USE), 1)
  DEFINES += TCC_I2C_USE
endif
ifeq ($(TCC_LCD_USE), 1)
  DEFINES += TCC_LCD_USE
endif
ifeq ($(TCC_NAND_USE), 1)
  DEFINES += TCC_NAND_USE
endif
ifeq ($(TCC_FWDN_USE), 1)
  DEFINES += TCC_FWDN_USE
endif
ifeq ($(TCC_USB_USE), 1)
  DEFINES += TCC_USB_USE
endif
ifeq ($(TCC_SPLASH_USE), 1)
  DEFINES += TCC_SPLASH_USE
endif
ifeq ($(TCC_GPSB_USE), 1)
  DEFINES += TCC_GPSB_USE
endif
ifeq ($(TCC_HDMI_USE), 1)
  DEFINES += TCC_HDMI_USE
  DEFINES += TCC_HDMI_USE_XIN_24MHZ

  ifeq ($(TCC8935S),1)
    DEFINES += TCC_HDMI_DRIVER_V1_3
    BOARD_HDMI_V1_3 := 1
  else
    ifeq ($(TCC8933S),1)
      DEFINES += TCC_HDMI_DRIVER_V1_3
      BOARD_HDMI_V1_3 := 1
    else
      ifeq ($(TCC8937S),1)
        DEFINES += TCC_HDMI_DRIVER_V1_3
        BOARD_HDMI_V1_3 := 1
      else
        DEFINES += TCC_HDMI_DRIVER_V1_4
        BOARD_HDMI_V1_4 := 1
      endif
    endif
  endif
endif
ifeq ($(TCC_PCA953X_USE), 1)
  DEFINES += TCC_PCA953X_USE
endif
ifeq ($(TCC_CHIP_REV), 1)
  DEFINES += TCC_CHIP_REV
endif
ifeq ($(TCC_CM3_USE), 1)
  DEFINES += TCC_CM3_USE
endif

DEFINES += TCC_GIC_USE

#--------------------------------------------------------------
# for USB 3.0 Device
#--------------------------------------------------------------
TCC_USB_30_USE := 1

ifeq ($(TCC_USB_30_USE), 1)
  DEFINES += TCC_USB_30_USE
endif

#==================================================================
# Snapshot Boot Option 
#==================================================================
#TCC_SNAPSHOT_USE := 1
TCC_SNAPSHOT_USE_LZ4 := 1

ifeq ($(TCC_SNAPSHOT_USE),1)
	DEFINES += CONFIG_SNAPSHOT_BOOT
endif

ifeq ($(TCC_SNAPSHOT_USE_LZ4),1)
	DEFINES += CONFIG_SNAPSHOT_BOOT
	DEFINES += CONFIG_SNAPSHOT_CHECKSUM
	DEFINES += CONFIG_QB_STEP_LOG
	DEFINES += CONFIG_QB_LOGO
	DEFINES += CONFIG_QB_WATCHDOG_ENABLE

CONFIG_WATCHDOG_TIMEOUT := 2000		# ms
DEFINES += CONFIG_WATCHDOG_TIMEOUT=$(CONFIG_WATCHDOG_TIMEOUT)

CONFIG_TCC_CPUFREQ_HIGHSPEED_CLOCK := 1000
#CONFIG_TCC_CPUFREQ_HIGHSPEED_CLOCK=1152
#CONFIG_TCC_CPUFREQ_HIGHSPEED_CLOCK=1112
#CONFIG_TCC_CPUFREQ_HIGHSPEED_CLOCK=1060
#CONFIG_TCC_CPUFREQ_HIGHSPEED_CLOCK=1000
#CONFIG_TCC_CPUFREQ_HIGHSPEED_CLOCK=910
#CONFIG_TCC_CPUFREQ_HIGHSPEED_CLOCK=850

  DEFINES += \
	  CONFIG_TCC_CPUFREQ_HIGHSPEED_CLOCK=$(CONFIG_TCC_CPUFREQ_HIGHSPEED_CLOCK)
endif

#DEFINES += ENABLE_EMMC_TZOS
