#include <daudio_test.h>
#include <stdlib.h>
#include <string.h>
#include <sdmmc/emmc.h>

#ifndef CRC_CHECK_EMMC
#define CRC_CHECK_EMMC_DEBUG
#endif

#ifdef CRC_CHECK_EMMC
#define BYTE_TO_SECTOR(X)			((X + 511) >> 9)

unsigned long long tcc_get_storage_capacity(void)
{
	unsigned long long ret;

	ret = emmc_get_capacity();

	if(ret == 0){
		printf("[STORAGE] Get Storage Capacity Failed \n");
		return 0;
	}
	return ret;
}

int tcc_read(unsigned long long data_addr, void* in, unsigned int data_len)
{
	int ret;

	ret = emmc_read(data_addr, data_len, in);

	if(ret < 0)
		printf("[STORAGE] Read Fail Form Target Storage \n");

	return ret;
}

static unsigned int crc_sum;

static unsigned int crc32(unsigned int prev_crc, unsigned char const *p, unsigned int len) 
{
        int i;
        unsigned int crc = prev_crc;
        
        while (len--) {
            crc ^= *p++;
            for (i = 0; i < 8; i++)
                crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320 : 0);
        }
        return crc;
}

static void crc_check_emmc(void)
{
    unsigned long long emmc_size = tcc_get_storage_capacity();
    unsigned page_size = flash_page_size();
    unsigned long long data_len = (emmc_size * page_size);
    unsigned long long data_addr = 0;
    unsigned char buffer[page_size];
    int ret;
#ifdef CRC_CHECK_EMMC_DEBUG  
    int i;
#endif

    crc_sum = 0;
    printf("===========================================\n");
    printf(" CRC cheking!!! Please wait!!!\n");
    printf(" eMMC size = %lld bytes (%lld sectors)\n", data_len, emmc_size);
    
    while(data_len){
        
        memset(buffer, 0, page_size);
        ret = tcc_read(data_addr, buffer, page_size);

        if (ret != 0)
        {
            printf(" eMMC read error!!!\n");
            printf("===========================================\n");
            data_len = 0;
            return;
        }
        
        data_len = data_len - page_size;
        data_addr++;

        crc_sum = crc32(crc_sum, buffer, page_size);
        
		if (data_addr % 100000 == 0)
			printf("... %lld sectors done \n", data_addr);

#ifdef CRC_CHECK_EMMC_DEBUG       
        for(i=0;i<page_size;i++)
        {
            if((i%16) == 0)
                {
                printf("\n");
                printf("addr 0x%x : ", i);
                }
#ifdef CRC_CHECK_EMMC_DEBUG
            if(buffer[i] < 0x10)
                printf("0%x ", buffer[i]);
            else
#endif
                printf("%x ", buffer[i]);
        }

#endif        

    }
    printf("\n");
    printf(" CRC  = 0x%x \n", crc_sum);
    printf(" CRC cheking Done!!! \n");
    printf("===========================================\n");
#ifdef CRC_CHECK_EMMC_DEBUG
	while(1){
        }
#endif
}
#endif

void daudio_test_start(void)
{
	int ret = 0;
	int ltc3676_test = 0;
	int is_test_mode = 1;

	dprintf(INFO, "===========================================\n");
	dprintf(INFO, "Select test mode\n");
	dprintf(INFO, "(1) PMIC DVB control & DDR over clock\n");
	dprintf(INFO, "(2) DDR over clock only\n");
	dprintf(INFO, "(3) PMIC DVB control & PMIC BUCK control\n");
	dprintf(INFO, "(4) TW8836 control\n");
#ifdef CRC_CHECK_EMMC
    dprintf(INFO, "(5) eMMC CRC Check\n");
#endif 
	dprintf(INFO, "===========================================\n");

	while (is_test_mode)
	{
		char input_val = 0;
		getc(&input_val);

		switch (input_val)
		{
			case '1':
				ltc3676_test = 1;
				ret = ltc3676_test_start();
				dram_overclock_test(0);
				ltc3676_test_stop();
				is_test_mode = 0;
				break;

			case '2':
				dram_overclock_test(0);
				is_test_mode = 0;
				break;

			case '3':
				ret = ltc3676_test_start();
				ltc3676_buck_control();
				is_test_mode = 0;
				break;

			case '4':
				tw8836_test_start();
				is_test_mode = 0;
				break;
#ifdef CRC_CHECK_EMMC
            case '5':
                crc_check_emmc();
                is_test_mode = 0;
                break;
#endif                            
		}
	}
}
