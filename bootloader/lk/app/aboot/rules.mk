LOCAL_DIR := $(GET_LOCAL_DIR)

OBJS += \
	$(LOCAL_DIR)/aboot.Ao \

ifeq ($(TCC_NAND_USE),1)
OBJS += \
	$(LOCAL_DIR)/recovery.o
endif

ifeq ($(TCC_USB_USE),1)
OBJS += \
	$(LOCAL_DIR)/fastboot.o
endif

ifeq ($(TCC_SNAPSHOT_USE_LZ4),1)
OBJS += \
	$(LOCAL_DIR)/lz4/tcc_memcpy.Ao \
	$(LOCAL_DIR)/lz4/lz4_decompress.o \
	$(LOCAL_DIR)/lz4/lz4_emmc.o \
	$(LOCAL_DIR)/lz4/lz4_nand.o 

LDFLAGS += $(LOCAL_DIR)/../../snapshot/libsnapshot.a
endif

ifeq ($(TCC_SNAPSHOT_USE),1)
OBJS += \
	$(LOCAL_DIR)/snappy/tcc_memcpy.Ao \
	$(LOCAL_DIR)/snappy/csnappy_main.o \
	$(LOCAL_DIR)/snappy/csnappy_decompress.o \
	$(LOCAL_DIR)/snappy/csnappy_emmc.o

LDFLAGS += $(LOCAL_DIR)/../../snapshot/libsnapshot.a
endif

OBJS += \
		$(LOCAL_DIR)/sha/rsa.o \
		$(LOCAL_DIR)/sha/rsa_e_3.o \
		$(LOCAL_DIR)/sha/rsa_e_f4.o \
		$(LOCAL_DIR)/sha/sha.o
