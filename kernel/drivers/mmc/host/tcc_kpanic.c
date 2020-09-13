#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/slab.h>

#include <linux/scatterlist.h>
#include <linux/swap.h>
#include <linux/list.h>

#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>
#include <linux/time.h>
#include <linux/kthread.h>

#include <linux/mmc/tcc_kpanic.h>

enum {
	TCC_MMC_KPANIC_TASK_UNINIT    = 1 << 0,
	TCC_MMC_KPANIC_TASK_SLEEPING  = 1 << 1,
	TCC_MMC_KPANIC_TASK_WAKINGUP  = 1 << 2,
	TCC_MMC_KPANIC_TASK_RUNNING   = 1 << 3,
	TCC_MMC_KPANIC_TASK_COMPLETED = 1 << 4,
};

int log_buf_copy(char *dest, int idx, int len);

static struct mmc_card *tcc_mmc_kpanic_card = NULL;
static DEFINE_MUTEX(tcc_mmc_kpanic_lock);
static unsigned int tcc_kpanic_base = 0xFFFFFFFF; /* sector-based addr */
static unsigned int tcc_kpanic_size = 0x0; /* sector-based size */
static atomic_t tcc_mmc_kpanic_state;
static struct task_struct *th = NULL;

volatile bool TCC_MMC_KPANIC_IS_IN_PROGRESS(void)
{
	if (likely(atomic_read(&tcc_mmc_kpanic_state) == TCC_MMC_KPANIC_TASK_UNINIT ||
			atomic_read(&tcc_mmc_kpanic_state) == TCC_MMC_KPANIC_TASK_SLEEPING))
		return false;
	return true;
}

void tcc_mmc_set_kpanic_card(struct mmc_card *mmc_card)
{
	tcc_mmc_kpanic_card = mmc_card;
}

struct mmc_card* tcc_mmc_get_kpanic_card(void)
{
	return tcc_mmc_kpanic_card;
}

static int mmc_test_busy(struct mmc_command *cmd)
{
	return !(cmd->resp[0] & R1_READY_FOR_DATA) ||
		(R1_CURRENT_STATE(cmd->resp[0]) == R1_STATE_PRG);
}

static int tcc_mmc_kpanic_check_result(struct tcc_mmc_kpanic_card *kpanic_card,
		struct mmc_request *mrq)
{
	int ret;

	BUG_ON(!mrq || !mrq->cmd || !mrq->data);

	ret = 0;

	if (!ret && mrq->cmd->error)
		ret = mrq->cmd->error;
	if (!ret && mrq->data->error)
		ret = mrq->data->error;
	if (!ret && mrq->stop && mrq->stop->error)
		ret = mrq->stop->error;
	if (!ret && mrq->data->bytes_xfered !=
			mrq->data->blocks * mrq->data->blksz)
		ret = RESULT_FAIL;

	if (ret == -EINVAL)
		ret = RESULT_UNSUP_HOST;

	return ret;
}


static int tcc_mmc_kpanic_wait_busy(struct tcc_mmc_kpanic_card *kpanic_card)
{
	int ret, busy;
	struct mmc_command cmd = {0};

	busy = 0;
	do {
		memset(&cmd, 0, sizeof(struct mmc_command));

		cmd.opcode = MMC_SEND_STATUS;
		cmd.arg = kpanic_card->card->rca << 16;
		cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;

		ret = mmc_wait_for_cmd(kpanic_card->card->host, &cmd, 0);
		if (ret)
			break;

		if (!busy && mmc_test_busy(&cmd)) {
			busy = 1;
			if (kpanic_card->card->host->caps & MMC_CAP_WAIT_WHILE_BUSY)
				printk(KERN_INFO "%s: Warning: Host did not "
						"wait for busy state to end.\n",
						mmc_hostname(kpanic_card->card->host));
		}
	} while (mmc_test_busy(&cmd));

	return ret;
}

static void tcc_mmc_kpanic_prepare_mrq(struct tcc_mmc_kpanic_card *kpanic_card,
		struct mmc_request *mrq, struct scatterlist *sg, unsigned sg_len,
		unsigned dev_addr, unsigned blocks, unsigned blksz, int write)
{
	BUG_ON(!mrq || !mrq->cmd || !mrq->data || !mrq->stop);

	if (blocks > 1) {
		mrq->cmd->opcode = write ?
			MMC_WRITE_MULTIPLE_BLOCK : MMC_READ_MULTIPLE_BLOCK;
	} else {
		mrq->cmd->opcode = write ?
			MMC_WRITE_BLOCK : MMC_READ_SINGLE_BLOCK;
	}

	mrq->cmd->arg = dev_addr;
	if (!mmc_card_blockaddr(kpanic_card->card))
		mrq->cmd->arg <<= 9;

	mrq->cmd->flags = MMC_RSP_R1 | MMC_CMD_ADTC;

	if (blocks == 1)
		mrq->stop = NULL;
	else {
		mrq->stop->opcode = MMC_STOP_TRANSMISSION;
		mrq->stop->arg = 0;
		mrq->stop->flags = MMC_RSP_R1B | MMC_CMD_AC;
	}

	mrq->data->blksz = blksz;
	mrq->data->blocks = blocks;
	mrq->data->flags = write ? MMC_DATA_WRITE : MMC_DATA_READ;
	mrq->data->sg = sg;
	mrq->data->sg_len = sg_len;

	mmc_set_data_timeout(mrq->data, kpanic_card->card);
}


static int tcc_mmc_kpanic_simple_transfer(struct tcc_mmc_kpanic_card *kpanic_card,
		struct scatterlist *sg, unsigned sg_len, unsigned dev_addr,
		unsigned blocks, unsigned blksz, int write)
{
	struct mmc_request mrq = {0};
	struct mmc_command cmd = {0};
	struct mmc_command stop = {0};
	struct mmc_data data = {0};

	mrq.cmd = &cmd;
	mrq.data = &data;
	mrq.stop = &stop;

	tcc_mmc_kpanic_prepare_mrq(kpanic_card, &mrq, sg, sg_len, dev_addr,
			blocks, blksz, write);

	mmc_wait_for_req(kpanic_card->card->host, &mrq);

	tcc_mmc_kpanic_wait_busy(kpanic_card);

	return tcc_mmc_kpanic_check_result(kpanic_card, &mrq);
}

static int tcc_mmc_kpanic_thread(void *dummy)
{
	struct tcc_mmc_kpanic_card *kpanic_card = NULL;
	struct tcc_mmc_kpanic_hdr kpanic_hdr;

	unsigned int offset = 0xFFFFFFFF;
	unsigned int tcc_kpanic_last_addr = 0x00000000;
	struct scatterlist sg;

	int saved_oip;
	int idx = 0;
	int rc, rc2, ret;
	unsigned int last_chunk = 0;
	
	struct timeval curr_timeval;
	struct tm curr_tm;

	struct sched_param param = { .sched_priority = 1 };
	sched_setscheduler(current, SCHED_FIFO, &param);

	printk("%s: %s:%d was born...\n", __func__, current->comm, current->pid);

	while (!kthread_should_stop() &&
			atomic_read(&tcc_mmc_kpanic_state) == TCC_MMC_KPANIC_TASK_SLEEPING) {
		set_current_state(TASK_INTERRUPTIBLE);

		if (kthread_should_stop() ||
				atomic_read(&tcc_mmc_kpanic_state) == TCC_MMC_KPANIC_TASK_WAKINGUP)
			break;
		schedule();
		set_current_state(TASK_RUNNING);
	} 
	if (kthread_should_stop())
		return 0;
	if (atomic_read(&tcc_mmc_kpanic_state) != TCC_MMC_KPANIC_TASK_WAKINGUP)
		return -1;

	set_current_state(TASK_RUNNING);

	printk("%s: %s:%d executed...\n", __func__, current->comm, current->pid);

	atomic_set(&tcc_mmc_kpanic_state, TCC_MMC_KPANIC_TASK_RUNNING);
	smp_wmb();

	kpanic_card = kzalloc(sizeof(struct tcc_mmc_kpanic_card), GFP_KERNEL);
	if (!kpanic_card)
		return -ENOMEM;
	kpanic_card->card = tcc_mmc_get_kpanic_card();

	kpanic_card->buffer = kzalloc(BUFFER_SIZE, GFP_KERNEL);

	memset(&kpanic_hdr, 0x0, sizeof(struct tcc_mmc_kpanic_hdr));

	if (kpanic_card->buffer)
	{
		mutex_lock(&tcc_mmc_kpanic_lock);
		mmc_claim_host(kpanic_card->card->host);

		if (tcc_kpanic_base == (unsigned int)0xFFFFFFFF) {
			goto out;
		}
		if (tcc_kpanic_size == (unsigned int)0x00000000) {
			goto out;
		}

		kpanic_card->card->ext_csd.part_config &= ~EXT_CSD_PART_CONFIG_ACC_MASK;
		kpanic_card->card->ext_csd.part_config |= 0;
		ret = mmc_switch(kpanic_card->card, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_PART_CONFIG, kpanic_card->card->ext_csd.part_config,
				kpanic_card->card->ext_csd.part_time);
		if (ret) {
			printk("%s : mmc partition switch failed(%d)\n", __func__, ret);
			return ret;
		}

		sg_init_one(&sg, kpanic_card->buffer, 512);

		offset = tcc_kpanic_base;
		tcc_kpanic_last_addr = tcc_kpanic_base + tcc_kpanic_size;

		strncpy(kpanic_hdr.magic, TCC_MMC_KPANIC_MAGIC, TCC_MMC_KPANIC_MAGIC_SIZE);
		kpanic_hdr.log_status = 1;
		kpanic_hdr.log_absolute_base = offset;
		kpanic_hdr.log_size = 0;

		do_gettimeofday(&curr_timeval);
		time_to_tm(curr_timeval.tv_sec, 0, &curr_tm);
		
		sprintf(kpanic_hdr.time, "%04ld_%02d_%02d_%02d_%02d_%02d", 
				curr_tm.tm_year + 1900, curr_tm.tm_mon + 1,
				curr_tm.tm_mday, curr_tm.tm_hour, curr_tm.tm_min, curr_tm.tm_sec);
		printk("%s : %s\n", __func__, kpanic_hdr.time);
#if 0
		sprintf(kpanic_hdr.time, "%s_%s", __DATE__, __TIME__);
		for (idx = 0; idx < strlen(kpanic_hdr.time); idx++)
		{
			if (kpanic_hdr.time[idx] == ' ' ||
				kpanic_hdr.time[idx] == ':')
				kpanic_hdr.time[idx] = '_';
		}
		idx = 0;

		printk("%s : DATE(%s), TIME(%s)\n", __func__, __DATE__, __TIME__);
		printk("%s : %s\n", __func__, kpanic_hdr.time);
#endif


		memcpy(kpanic_card->buffer, &kpanic_hdr, sizeof(struct tcc_mmc_kpanic_hdr)); 
		rc2 = tcc_mmc_kpanic_simple_transfer(kpanic_card, &sg, 1, offset++, 1, 512, 1);
		if (rc2 < 0) {
			printk(KERN_EMERG
					"%s: mmc header write failed (%d)\n", __func__, rc2);
			return -1;
		}

		while (!last_chunk)
		{
			if (tcc_kpanic_base == (unsigned int)0xFFFFFFFF) {
				break;
			}
			if (tcc_kpanic_size == (unsigned int)0x00000000) {
				break;
			}
			if (offset > tcc_kpanic_last_addr) {
				break;
			}

			saved_oip = oops_in_progress;
			oops_in_progress = 1;
			rc = log_buf_copy(kpanic_card->buffer, idx, 512);
			if (rc <= 0) break;

			if (rc != 512) {
				last_chunk = rc;
			}

			oops_in_progress = saved_oip;
			if (rc <= 0) break;
			if (rc != 512) memset(kpanic_card->buffer + rc, 0, 512 - rc);

			rc2 = tcc_mmc_kpanic_simple_transfer(kpanic_card, &sg, 1, offset, 1, 512, 1);
			if (rc2 < 0) {
				printk(KERN_EMERG
						"%s: mmc write failed (%d)\n", __func__, rc2);
				return idx;
			}
			if (!last_chunk)
				idx += 512;

			else
				idx += last_chunk;
			offset++;
		}

		kpanic_hdr.log_status = 2;
		kpanic_hdr.log_size = (unsigned int)idx;

		memcpy(kpanic_card->buffer, &kpanic_hdr, sizeof(struct tcc_mmc_kpanic_hdr)); 
		rc2 = tcc_mmc_kpanic_simple_transfer(kpanic_card, &sg, 1, 
				kpanic_hdr.log_absolute_base, 1, 512, 1);
		if (rc2 < 0) {
			printk(KERN_EMERG
					"%s: mmc header write failed (%d)\n", __func__, rc2);
			return -1;
		}
out:
		mmc_release_host(kpanic_card->card->host);
		mutex_unlock(&tcc_mmc_kpanic_lock);
	}
	kfree(kpanic_card->buffer);
	kfree(kpanic_card);

	atomic_set(&tcc_mmc_kpanic_state, TCC_MMC_KPANIC_TASK_COMPLETED);
	smp_wmb();

	return 0;
}

int tcc_mmc_kpanic_wakeup(void)
{
	if (atomic_read(&tcc_mmc_kpanic_state) == TCC_MMC_KPANIC_TASK_SLEEPING) {
		atomic_set(&tcc_mmc_kpanic_state, TCC_MMC_KPANIC_TASK_WAKINGUP);
		smp_wmb();
		printk("%s: ready to run kpanic thread, state(%d)\n",
				__func__, atomic_read(&tcc_mmc_kpanic_state));
		wake_up_process(th);
	} else if (atomic_read(&tcc_mmc_kpanic_state) == TCC_MMC_KPANIC_TASK_UNINIT) {
		printk("%s: there is no kpanic thread ... \n", __func__);
		return -1;
	}

	do {
		if (atomic_read(&tcc_mmc_kpanic_state) == TCC_MMC_KPANIC_TASK_COMPLETED) {
			printk("%s: kpanic thread finished.\n", __func__);
			break;
		}
		schedule();
	} while (1);

	return 0;
}

int init_tcc_mmc_kpanic(void)
{

	atomic_set(&tcc_mmc_kpanic_state, TCC_MMC_KPANIC_TASK_UNINIT);
	smp_wmb();

	th = kthread_run(tcc_mmc_kpanic_thread, NULL, "ktccmmckpanicd");
	if (!IS_ERR(th)) {
		atomic_set(&tcc_mmc_kpanic_state, TCC_MMC_KPANIC_TASK_SLEEPING);
		smp_wmb();
		printk("%s: succeeded to create thread, state(%d)\n",
				__func__, atomic_read(&tcc_mmc_kpanic_state));
	} else {
		printk("%s : failed to fork thread err(%ld)...\n", __func__, PTR_ERR(th));
		return -1;
	}

	return 0;
}

static int tcc_mmc_get_kpanic_base(char *str)
{
	unsigned int base_addr = 0xFFFFFFFF;

	if (sscanf(str, "%d", &base_addr) == 1)
		tcc_kpanic_base = base_addr + TCC_KPANIC_OFFSET;

	printk("\x1b[1;31m %s, str(%s), base(%d) \x1b[0m\n",
			__func__, str, tcc_kpanic_base);
	return 1;
}
__setup("tcc_kpanic_base=", tcc_mmc_get_kpanic_base);

static int tcc_mmc_get_kpanic_size(char *str)
{
	unsigned int size = 0x0;

	if (sscanf(str, "%d", &size) == 1)
		tcc_kpanic_size = size - TCC_KPANIC_OFFSET;

	printk("\x1b[1;31m %s, str(%s), size(%d) \x1b[0m\n",
			__func__, str, tcc_kpanic_size);
	return 1;
}
__setup("tcc_kpanic_size=", tcc_mmc_get_kpanic_size);
