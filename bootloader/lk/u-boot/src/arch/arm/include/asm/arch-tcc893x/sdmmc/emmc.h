#include <asm/arch/sdmmc/sd_memory.h>
#include <asm/arch/fwdn/Disk.h>

void emmc_boot_main(void);
unsigned int emmc_write(char* write_target, unsigned long long data_addr , unsigned int data_len , void* in);
unsigned int emmc_read(unsigned long long adta_addr , unsigned data_len, void* in);
#if defined(FWDN_HIDDEN_KERNEL_MODE)
unsigned int emmc_boot_write(unsigned int data_len , unsigned int *data);
#endif
unsigned long long emmc_get_capacity(void);
int get_emmc_serial(char* Serial);
