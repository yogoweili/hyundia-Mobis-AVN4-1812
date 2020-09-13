#define RESULT_OK               0
#define RESULT_FAIL             1
#define RESULT_UNSUP_HOST       2
#define RESULT_UNSUP_CARD       3

#define BUFFER_ORDER            2
#define BUFFER_SIZE             (PAGE_SIZE << BUFFER_ORDER)

#define TCC_MMC_KPANIC_MAGIC "!!TCCKERNELLOG!!"
#define TCC_MMC_KPANIC_MAGIC_SIZE 16
#define TCC_MMC_KPANIC_TIME_SIZE 64

#define TCC_KPANIC_OFFSET 2048	/* sector-based addr */

struct tcc_mmc_kpanic_hdr 
{
	unsigned char magic[TCC_MMC_KPANIC_MAGIC_SIZE];
	unsigned int pad1[4];
	unsigned char time[TCC_MMC_KPANIC_TIME_SIZE];

	/*
	 * log_status 0x0000_0000 : There is no log.
	 * log_status 0x0000_0001 : There is a log, but it isn't completely saved.
	 * log_status 0x0000_0002 : A log is in the raw data partition.
	 * log_status      ...    : ...
	 */
	unsigned int log_status;
	unsigned int log_absolute_base;	/* sector address */
	unsigned int log_size;          /* size in bytes */

	unsigned int pad2;
} __attribute__ ((aligned(512)));

struct tcc_mmc_kpanic_card {
	struct mmc_card *card;
	u8              *buffer;
};

volatile bool TCC_MMC_KPANIC_IS_IN_PROGRESS(void);
void tcc_mmc_set_kpanic_card(struct mmc_card * kpanic_card);
struct mmc_card* tcc_mmc_get_kpanic_card(void);
int tcc_mmc_kpanic_wakeup(void);
int init_tcc_mmc_kpanic(void);
