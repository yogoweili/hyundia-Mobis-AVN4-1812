#Makefile to generate mkseimg

SECURE_DIR  := platform/tcc_shared/tcsb
COMPILER := gcc

SECURE_SRC     := $(SECURE_DIR)/tcsb_mkblimg.c

ifeq ($(EMMC_BOOT),1)
DEFINE_SE += -DEMMC_USED
else
DEFINE_SE += -DNAND_USED
ifeq ($(TNFTL_V8_USE),1)
DEFINE_SE += -DTNFTL_V8_INCLUDE
endif

endif

$(BUILDDIR)/tcsb_mkblimg: $(SECURE_SRC)
	$(COMPILER) -o $(BUILDDIR)/tcsb_mkblimg $(SECURE_SRC) $(DEFINE_SE)
