/*
*  atmel_mxt540E.c - Atmel maXTouch Touchscreen Controller
*
* May.2 2012
*  - refactorying
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/atmel_mxt540E.h>
#include <linux/uaccess.h>
#include "atmel_mxt540E_cfg.h"
#include "device_config_540.h"
#include <linux/gpio.h>
#include <linux/input/mt.h>

#ifdef CONFIG_ARCH_MXC
#include <mach/hardware.h>
#endif
#include <linux/miscdevice.h>

//valky - we use early suspend
//#undef CONFIG_HAS_EARLYSUSPEND

#undef MXT_ENABLE_RUN_CRC_CHECK

/* #define	TSP_BOOST */
#define	TS_100S_TIMER_INTERVAL	1

#define _MXT_REGUP_NOOP		0
#define _MXT_REGUP_RUNNING	1
#define _MXT_REGUP_END		2

#define AUTO_CAL_DISABLE 0
#define AUTO_CAL_ENABLE 1

/* amplitude limitation */
#define AMPLITUDE_LIMIT 250

//#define MXT_FACTORY_TEST

#define CLEIN_TOUCH_VERSION_BUF_SIZE 2048

#if defined(CONFIG_DAUDIO) && defined(CONFIG_TOUCHSCREEN_ATMEL_MXT_336S_AT)
#define HW_RESET_GPIO	TCC_GPF(2)
#endif

struct clein_touch_version{
	       char ver[CLEIN_TOUCH_VERSION_BUF_SIZE];
	       char conf[CLEIN_TOUCH_VERSION_BUF_SIZE];
	       char firm[CLEIN_TOUCH_VERSION_BUF_SIZE];
};


enum {
	MXT_UP_CMD_BEGIN  = 1,
	/* auto firmware upgrade */
	MXT_UP_AUTO = MXT_UP_CMD_BEGIN,
	/* lock for the user firmware upgrade */
	MXT_UP_LOCK,
	/* unlock for the user firmware upgrade */
	MXT_UP_UNLOCK,
	/* force fail test. this is for mxt_resume
	 * if you want to this test,
	 *  debug value should not be equal to DEBUG_NONE
	 * */
	/* check the current status and firmware upgrade */
	MXT_UP_CHECK_AND_AUTO,
	MXT_UP_CMD_END,
};

enum {
	MXT_FIRMUP_FLOW_LOCK = (1 << 1),
	MXT_FIRMUP_FLOW_UPGRADE = (1 << 2),
	MXT_FIRMUP_FLOW_UNLOCK = (1 << 3),
};

enum {
	MXT_FIRM_STATUS_STATBLE = 0,
	MXT_FIRM_STATUS_START_STABLE,
	MXT_FIRM_STATUS_START_UNSTABLE,
	MXT_FIRM_STATUS_SUCCESS,
	MXT_FIRM_STATUS_FAIL,
};

#ifdef MXT_FIRMUP_ENABLE
#define MXT_FIRM_CLEAR(status) (status = MXT_FIRM_STATUS_STATBLE)
#define MXT_FIRM_UPGRADING_STABLE(status) \
	(status == MXT_FIRM_STATUS_START_STABLE)
#define MXT_FIRM_UPGRADING_UNSTABLE(status) \
	(status == MXT_FIRM_STATUS_START_UNSTABLE)
#define MXT_FIRM_UPGRADING(status) (MXT_FIRM_UPGRADING_STABLE(status) || \
		MXT_FIRM_UPGRADING_UNSTABLE(status))
#define MXT_FIRM_STABLE(status) \
	(status == MXT_FIRM_STATUS_STATBLE || status == MXT_FIRM_STATUS_SUCCESS)
#else
#define MXT_FIRM_CLEAR(status)
#define MXT_FIRM_UPGRADING_STABLE(status) (0)
#define MXT_FIRM_UPGRADING_UNSTABLE(status) (0)
#define MXT_FIRM_UPGRADING(status)	(0)
#define MXT_FIRM_STABLE(status)		(1)
#endif

#undef TSP_DEBUG_MESSAGE

#define MXT_SW_RESET_TIME	400
#define I2C_RETRY_COUNT 3
#define MXT_INIT_FIRM_ERR 1

/* bool mxt_reconfig_flag; */
/* EXPORT_SYMBOL(mxt_reconfig_flag); */

/* static bool cal_check_flag; */
static u8 facesup_message_flag_T9;

struct clein_touch_version prev_ver;

static struct mxt_data g_mxt;

#define ABS(x, y)		((x < y) ? (y - x) : (x - y))

#ifdef TSP_DEBUG_MESSAGE
#define MAX_MSG_LOG_SIZE	512
struct {
	u8 id[MAX_MSG_LOG_SIZE];
	u8 status[MAX_MSG_LOG_SIZE];
	u16 xpos[MAX_MSG_LOG_SIZE];
	u16 ypos[MAX_MSG_LOG_SIZE];
	u8 area[MAX_MSG_LOG_SIZE];
	u8 amp[MAX_MSG_LOG_SIZE];
	u16 cnt;
} msg_log;
#endif

/* level of debugging messages */
#define DEBUG_NONE	0
#define DEBUG_INFO      1
#define DEBUG_MESSAGES  2
#define DEBUG_TRACE     3

#define REMOVE_DEBUG_CODE
#ifndef REMOVE_DEBUG_CODE
#define mxt_debug_info(mxt, fmt, arg...) \
	if (debug >= DEBUG_INFO) \
		dev_info(&mxt->client->dev, fmt, ## arg);

#define mxt_debug_msg(mxt, fmt, arg...) \
	if (debug >= DEBUG_MESSAGES) \
		dev_info(&mxt->client->dev, fmt, ## arg);

#define mxt_debug_trace(mxt, fmt, arg...) \
	if (debug >= DEBUG_TRACE) \
		dev_info(&mxt->client->dev, fmt, ## arg);
#else
#define mxt_debug_msg(p...) do {} while (0);
#define mxt_debug_info(p...) do {} while (0);
#define mxt_debug_trace(p...) do {} while (0);
#endif

/* for debugging,  enable DEBUG_TRACE */
static int debug = DEBUG_NONE;

module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "Activate debugging output");

#if 0 /* defined(TSP_BOOST) */
static bool clk_lock_state;
extern void tegra_cpu_lock_speed(int min_rate, int timeout_ms);
extern void tegra_cpu_unlock_speed(void);
#endif

static void mxt_release_all_fingers(struct mxt_data *mxt);

/* \brief check the chip's calibration */
static void check_chip_calibration(struct mxt_data *mxt);

/* \brief calibration may be good. and check the condition one more time */
static void cal_maybe_good(struct mxt_data *mxt);

/* \brief calibrate chip */
static int calibrate_chip(struct mxt_data *mxt);

/* \brief check the palm-suppression */
static int check_chip_palm(struct mxt_data *mxt);

/* \brief check the touch count */
static int check_touch_count(struct mxt_data *mxt, int *touch, int *anti_touch);

/* \breif get current runmode
 *
 * in MXT_CFG_OVERWRITE : rewrite registers in device_config_540.h
 * in MXT_CFG_DEBUG : no-rewite. in this mode, keep the registers from nvram
 *	if you want to update registers, use sysfile(store_registers)
 *	and if you want to back again, use sysfile(store_reg_mode)
 *
 * @param mxt struct mxt_data pointer
 * @return if normal-mode, return MXT_CFG_OVERWRITE,
 *	otherwise return MXT_CFG_DEBUG
 *   if error, return < 0,
 */
static int mxt_get_runmode(struct mxt_data *mxt);

/* \brief rewrite the runmode registers
 *
 * mxt_reload_registers are used in some functions like calibration
 * this function reload them again
 *
 * @param mxt struct mxt_data pointer
 * @param regs struct mxt_runmode_registers_t pointer to save them
 */
static void mxt_reload_registers(
		struct mxt_data *mxt, struct mxt_runmode_registers_t *regs);

/* \brief write the initial registers
 *
 * @param mxt struct mxt_data pointer
 * @return if success return 0, otherwise < 0
 */
static int mxt_write_init_registers(struct mxt_data *mxt);

/* \brief run re-cal if there'is no release msg */
static void mxt_supp_ops_dwork(struct work_struct *work);

/* \brief stop the suppression operation */
static inline void mxt_supp_ops_stop(struct mxt_data *mxt);

static void process_t48_message(u8 *message, struct mxt_data *mxt);

static void process_t6_message(u8 *message, struct mxt_data *mxt);

#if 0 /* ENABLE_NOISE_TEST_MODE */
/*botton_right, botton_left, center, top_right, top_left */
u16 test_node[5] = {12, 20, 104, 188, 196};
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mxt_early_suspend(struct early_suspend *h);
static void mxt_late_resume(struct early_suspend *h);
#endif

#if defined(MXT_FACTORY_TEST)
extern struct class *sec_class;
struct device *tsp_factory_mode;
#endif

#define MXT_100MS_TIMER_LIMIT	10
#define MXT_100MS_ERROR_LIMIT	5

#if TS_100S_TIMER_INTERVAL
static struct workqueue_struct *ts_100s_tmr_workqueue;
static void ts_100ms_timeout_handler(unsigned long data);
static void ts_100ms_timer_start(struct mxt_data *mxt);
static void ts_100ms_timer_stop(struct mxt_data *mxt);
static void ts_100ms_timer_init(struct mxt_data *mxt);
static void ts_100ms_tmr_work(struct work_struct *work);
#endif
static void ts_100ms_timer_clear(struct mxt_data *mxt);
static void ts_100ms_timer_enable(struct mxt_data *mxt);

#ifdef MXT_SELF_TEST
#define MXT_SELF_TEST_TIMER_TIME	(4000)
#define MXT_SELF_TEST_CHECK_TIME	(400)

static void mxt_self_test(struct work_struct *work);
static void mxt_self_test_check(struct work_struct *work);
static void mxt_self_test_timeout_handler(unsigned long data);
static void mxt_self_test_timer_stop(struct mxt_data *mxt);
static void mxt_self_test_timer_start(struct mxt_data *mxt);
static void mxt_run_self_test(struct mxt_data *mxt);
#endif

/* \brief initialize mxt540e
 * @param client struct i2c_client pointer
 * @param mxt struct mxt_data pointer
 * @return if < 0 error, 0 is ok, 1 is firmware upgrade
 */
static int mxt_initialize(struct i2c_client *client, struct mxt_data *mxt);
static int mxt_identify(struct i2c_client *client, struct mxt_data *mxt);
static int mxt_read_object_table(
		struct i2c_client *client, struct mxt_data *mxt);

/* \brief get the current configuration version */
static int mxt_get_version(
		struct mxt_data *mxt, struct clein_touch_version *ver);

/* \brief if the current configuration version is higher than previous one,
 *	bakcup to nvram */
static void mxt_update_backup_nvram(
		struct mxt_data *mxt, struct clein_touch_version *old_ver);

#ifdef MXT_FIRMUP_ENABLE
static int set_mxt_auto_update_exe(struct mxt_data *mxt, int cmd);
#endif

static int _check_recalibration(struct mxt_data *mxt);
static inline void _disable_auto_cal(struct mxt_data *mxt, int locked);
/* \brief delayed work
 * If enter hardware key, check touch, anti-touch
 * if tch !=0 and atch !=0, execute re-calibration function.
 */
static void mxt_cal_timer_dwork(struct work_struct *work);
static atomic_t cal_counter;
static int maybe_good_count;

/* Brief
 * It is used instead of T25(self test)
 * This will check touch ic status.
 */
#define MXT_CHK_TOUCH_IC_TIME		(4000)
static void mxt_check_touch_ic_timer_dwork(struct work_struct *work);
static void mxt_check_touch_ic_timer_start(struct mxt_data *mxt);
static inline void mxt_check_touch_ic_timer_stop(struct mxt_data *mxt);

const u8 *maxtouch_family = "maXTouch";
const u8 *mxt540_variant  = "mXT540E";

u8 *object_type_name[MXT_MAX_OBJECT_TYPES] = {
	[5] = "GEN_MESSAGEPROCESSOR_T5",
	[6] = "GEN_COMMANDPROCESSOR_T6",
	[7] = "GEN_POWERCONFIG_T7",
	[8] = "GEN_ACQUIRECONFIG_T8",
	[9] = "TOUCH_MULTITOUCHSCREEN_T9",
	[15] = "TOUCH_KEYARRAY_T15",
	[18] = "SPT_COMMSCONFIG_T18",
	[19] = "GPIOPWN_CONFIG_T19",
	[24] = "PROCI_ONETOUCHGESTUREPROCESSOR_T24",
	[25] = "SPT_SELFTEST_T25",
	[27] = "PROCI_TWOTOUCHGESTUREPROCESSOR_T27",
	[37] = "DEBUG_DIAGNOSTICS_T37",
	[38] = "USER_DATA_T38",
	[40] = "PROCI_GRIPSUPPRESSION_T40",
	[42] = "PROCI_TOUCHSUPPRESSION_T42",
	[43] = "HID_CONFI_T43",
	[44] = "MESSAGE_COUNTER_T44",
	[46] = "SPT_CTECONFIG_T46",
	[47] = "PROCI_STYLUS_T47",
	[48] = "PROCG_NOISESUPPRESSION_T48",
	[52] = "PROXIMITY_KEY_T52",
	[53] = "DATA_SOURCE_T53",
	[55] = "ADAPTIVETHRESHOLD_T55",
	[56] = "SHIEDLESS_T56",
	[57] = "EXTRATOUCHSCREENDATA_T57",
};

static struct multi_touch_info {
	uint16_t size;
	int16_t pressure;
		/* pressure(-1) : reported
		 * pressure(0) : released
		 * pressure(> 0) : pressed */
	int16_t x;
	int16_t y;
	/* int16_t component; */
} mtouch_info[MXT_MAX_NUM_TOUCHES];

static struct mxt_palm_info {
	u8 facesup_message_flag;
	bool palm_check_timer_flag;
	bool palm_release_flag;
} palm_info = {
	.palm_release_flag = true,
};

/* 128 byte is enough to mxt540e */
#define MXT_MSG_BUFF_SIZE 256
static u8 mxt_message_buf[MXT_MSG_BUFF_SIZE];

static int backup_to_nv(struct mxt_data *mxt)
{
	int ret;
	/* backs up settings to the non-volatile memory */
	/* = Command Processor T6 =
	 * BACKUPNV Field
	 * This field backs up settings to the non-volatile memory (NVM).
	 * Once the device has processed this command it generates a status
	 * message containing the new NVM checksum.
	 * Write value: 0x55
	 **/
	/* 2012_0504 :
	 *  - Configuration parameters maximum writes 10,000 */
	ret = mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6)+
			MXT_ADR_T6_BACKUPNV,
			0x55);
	/* 2012_0523 :
	 *  - ATMEL : please, log the below code */
	mxt_debug_info(mxt, "%s()\n", __func__);
	/* 2012_0515 :
	 *  - ATMEL : 100ms needs to backup to nvram */
	msleep(100);
	return ret;
}

static int mxt_get_runmode(struct mxt_data *mxt)
{
	u8 val;
	int ret, retry = I2C_RETRY_COUNT;
	struct i2c_client *client = mxt->client;

	/* run-mode
	 * MXT_CFG_OVERWRITE : when it boots up, read the configuration
	 *  and write them to the registers.
	 * MXT_CFG_DEBUG : when it boots up, the atmel IC reads the
	 *  configuration from the registers.
	 *
	 * MXT_ADR_T38_CFG_CTRL is 0'th address of MXT_USER_INFO_T38
	 */
retry_gr:
	ret = mxt_read_byte(client, MXT_BASE_ADDR(MXT_USER_INFO_T38) +
			MXT_ADR_T38_CFG_CTRL, &val);
	if (ret < 0) {
		if (retry--)
			goto retry_gr;
		dev_err(&client->dev, "%s() ret(%d)", __func__, ret);
		return ret;
	}

	if (unlikely((val != MXT_CFG_OVERWRITE) && (val != MXT_CFG_DEBUG))) {
		dev_info(&client->dev, "%s() runmode:0x%02X\n", __func__, val);
		val = MXT_CFG_OVERWRITE;
		backup_to_nv(mxt);
	}

	return (int)val;
}

static void mxt_reload_registers(
		struct mxt_data *mxt, struct mxt_runmode_registers_t *regs)
{
	memset(regs, 0, sizeof(struct mxt_runmode_registers_t));

	/* reload some registers for calibration */
	/* After updating new calibration, read some registers.
	 * see <runmode_regs> and mxt_config_save_runmode
	 * see also. calibrate_chip/cal_maybe_good */
	mxt_config_get_runmode_registers(regs);
}

void hw_reset_chip(struct mxt_data *mxt)
{
	struct mxt_platform_data *pdata;
	int error =0;

	//valky
	printk("%s\n", __func__);

	unsigned RESET_GPIO = TCC_GPA(23);
	#if defined(CONFIG_DAUDIO) && defined(CONFIG_TOUCHSCREEN_ATMEL_MXT_336S_AT)
		RESET_GPIO = HW_RESET_GPIO;
	#endif
	gpio_request(RESET_GPIO, NULL);
	gpio_direction_output(RESET_GPIO, 0);
	msleep(25);

	gpio_request(RESET_GPIO, NULL);
	gpio_direction_output(RESET_GPIO, 1);
	msleep(103);

	int ret = mxt_identify(mxt->client, mxt);
	if (ret >= 0) {
		mxt_debug_info(mxt, "[TSP] mxt_identify Sucess");
		ret = mxt_read_object_table(mxt->client, mxt);
		if (ret >= 0) {
			mxt_debug_info(mxt,
					"[TSP] mxt_read_object_table Sucess");
		} else
			dev_err(&mxt->client->dev,
					"[TSP] mxt_read_object_table Fail");
	} else
		dev_err(&mxt->client->dev,"[TSP] mxt_identify Fail ");
	mxt_release_all_fingers(mxt);
	calibrate_chip(mxt);
	mxt->cal_check_flag = 1;
}

int try_sw_reset_chip(struct mxt_data *mxt, u8 mode)
{
	int err;

	err = sw_reset_chip(mxt, mode);
	if (err) {
		hw_reset_chip(mxt);
		return 1;
	}

	return 0;
}

int sw_reset_chip(struct mxt_data *mxt, u8 mode)
{
	u8 data;
	u16 addr;
	int retry = I2C_RETRY_COUNT, err;
	struct i2c_client *client = mxt->client;

	mxt_debug_trace(mxt, "[TSP] Reset chip Reset mode (%d)", mode);

	/* Command Processor T6 (GEN_COMMANDPROCESSOR_T6)
	 * RESET Field
	 * This field forces a reset of the device if a nonzero value
	 * is written. If 0xA5 is written to this field, the device
	 * resets into bootloader mode.
	 * Write value: Nonzero (normal) or 0xA5 (bootloader) */

	if (mode == RESET_TO_NORMAL)
		data = 0x01;  /* non-zero value */
	else if (mode == RESET_TO_BOOTLOADER)
		data = 0xA5;
	else {
		dev_info(&client->dev, "[TSP] Invalid reset mode(%d)", mode);
		return -EINVAL;
	}

	if (MXT_FIRM_STABLE(mxt->firm_status_data) ||
			MXT_FIRM_UPGRADING_STABLE(mxt->firm_status_data))
		addr = MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6) +
			MXT_ADR_T6_RESET;
	else {
		/* if the firmware is not stable, we don't know about
		 * the address from the objects. so, 0x11F is hard-coding */
		addr = 0x11F + MXT_ADR_T6_RESET;
		return -EIO;
	}

	do {
		err = mxt_write_byte(client, addr, data);
		if (err == 0)
			break;
	} while (retry--);
	return err;
}

/* 2012_0504 : ATMEL recommends that the s/w reset after h/w reset
 * doesn't need to be run */
static int mxt_force_reset(struct mxt_data *mxt)
{
	int cnt;

	if (debug >= DEBUG_MESSAGES)
		pr_info("[TSP] %s has been called!", __func__);

#if 0/* ndef MXT_THREADED_IRQ */
	/* prevents the system from entering suspend during updating */
	wake_lock(&mxt->wakelock);
	/* disable interrupt */
	disable_irq(mxt->client->irq);
#endif
	hw_reset_chip(mxt);

	for (cnt = 10; cnt > 0; cnt--) {
		/* soft reset */
		if (sw_reset_chip(mxt, RESET_TO_NORMAL) == 0)
			break;
	}
	if (cnt == 0) {
		dev_err(&mxt->client->dev, "[TSP] mxt_force_reset failed!!!");
		return -1;
	}
	msleep(250);  /* 250ms */
#if 0/* ndef MXT_THREADED_IRQ */
	enable_irq(mxt->client->irq);	/* enable interrupt */
	wake_unlock(&mxt->wakelock);
#endif
	if (debug >= DEBUG_MESSAGES)
		pr_info("[TSP] %s success!!!", __func__);

	return 0;
}

#if defined(MXT_DRIVER_FILTER)
static void equalize_coordinate(bool detect, u8 id, u16 *px, u16 *py)
{
	static int tcount[MXT_MAX_NUM_TOUCHES] = {0, };
	static u16 pre_x[MXT_MAX_NUM_TOUCHES][4] = {{0}, };
	static u16 pre_y[MXT_MAX_NUM_TOUCHES][4] = {{0}, };
	int coff[4] = {0, };
	int distance = 0;

	if (detect)
		tcount[id] = 0;

	pre_x[id][tcount[id]%4] = *px;
	pre_y[id][tcount[id]%4] = *py;

	if (tcount[id] > 3) {
		distance = abs(pre_x[id][(tcount[id]-1)%4] - *px)
			+ abs(pre_y[id][(tcount[id]-1)%4] - *py);

		coff[0] = (u8)(4 + distance/5);
		if (coff[0] < 8) {
			coff[0] = max(4, coff[0]);
			coff[1] = min((10 - coff[0]), (coff[0]>>1)+1);
			coff[2] = min((10 - coff[0] - coff[1]), (coff[1]>>1)+1);
			coff[3] = 10 - coff[0] - coff[1] - coff[2];

			*px = (u16)((*px*(coff[0])
				+ pre_x[id][(tcount[id]-1)%4]*(coff[1])
				+ pre_x[id][(tcount[id]-2)%4]*(coff[2])
				+ pre_x[id][(tcount[id]-3)%4]*(coff[3]))/10);
			*py = (u16)((*py*(coff[0])
				+ pre_y[id][(tcount[id]-1)%4]*(coff[1])
				+ pre_y[id][(tcount[id]-2)%4]*(coff[2])
				+ pre_y[id][(tcount[id]-3)%4]*(coff[3]))/10);
		} else {
			*px = (u16)((*px*4 + pre_x[id][(tcount[id]-1)%4])/5);
			*py = (u16)((*py*4 + pre_y[id][(tcount[id]-1)%4])/5);
		}
	}
	tcount[id]++;
}
#endif  /* MXT_DRIVER_FILTER */

static void mxt_release_all_fingers(struct mxt_data *mxt)
{
	int id;

	mxt_debug_trace(mxt, "[TSP] %s\n", __func__);

	/* if there is any un-reported message, report it */
	for (id = 0 ; id < MXT_MAX_NUM_TOUCHES ; ++id) {
		if (mtouch_info[id].pressure == -1)
			continue;

		/* force release check*/
		mtouch_info[id].pressure = 0;

		/* ADD TRACKING_ID*/
		//REPORT_MT(id, mtouch_info[id].x, mtouch_info[id].y,
		//		mtouch_info[id].pressure, mtouch_info[id].size);
		input_report_key(mxt->input, BTN_TOUCH, 0);
		input_mt_sync(mxt->input);
#ifdef CONFIG_KERNEL_DEBUG_SEC
		mxt_debug_msg(mxt, "[TSP] r (%d,%d) %d\n",
					mtouch_info[id].x,
					mtouch_info[id].y, id);
#endif
		if (mtouch_info[id].pressure == 0)
			mtouch_info[id].pressure = -1;
	}

	input_sync(mxt->input);

#if 0 /* defined(TSP_BOOST) */
	if (clk_lock_state) {
		tegra_cpu_unlock_speed();
		clk_lock_state = false;
	}
#endif
}

#if 0/* def KEY_LED_CONTROL */
static void mxt_release_all_keys(struct mxt_data *mxt)
{
	if (debug >= DEBUG_INFO)
		pr_info("[TSP] %s, tsp_keystatus = %d\n",
				__func__, tsp_keystatus);

	if (tsp_keystatus != TOUCH_KEY_NULL) {
		if (mxt->pdata->board_rev <= 9) {
			switch (tsp_keystatus) {
			case TOUCH_2KEY_MENU:
				input_report_key(mxt->input,
						KEY_MENU, KEY_RELEASE);
				break;
			case TOUCH_2KEY_BACK:
				input_report_key(mxt->input,
						KEY_BACK, KEY_RELEASE);
				break;
			default:
				break;
			}
#ifdef CONFIG_KERNEL_DEBUG_SEC
			if (debug >= DEBUG_MESSAGES)
				pr_info("[TSP_KEY] r %s\n",
					tsp_2keyname[tsp_keystatus - 1]);
#endif
		} else {
			/* since board rev.10, touch key is 4 key array */
			switch (tsp_keystatus) {
			case TOUCH_4KEY_MENU:
				input_report_key(mxt->input,
						KEY_MENU, KEY_RELEASE);
				break;
			case TOUCH_4KEY_HOME:
				input_report_key(mxt->input,
						KEY_HOME, KEY_RELEASE);
				break;
			case TOUCH_4KEY_BACK:
				input_report_key(mxt->input,
						KEY_BACK, KEY_RELEASE);
				break;
			case TOUCH_4KEY_SEARCH:
				input_report_key(mxt->input,
						KEY_SEARCH, KEY_RELEASE);
				break;
			default:
				break;
			}
#ifdef CONFIG_KERNEL_DEBUG_SEC
			if (debug >= DEBUG_MESSAGES)
				pr_info("[TSP_KEY] r %s\n",
					tsp_4keyname[tsp_keystatus - 1]);
#endif
		}
		tsp_keystatus = TOUCH_KEY_NULL;
	}
}
#else
#define mxt_release_all_keys(mxt) do {} while (0);
#endif

static inline void mxt_prerun_reset(struct mxt_data *mxt, bool r_irq)
{
#if TS_100S_TIMER_INTERVAL
	ts_100ms_timer_stop(mxt);
#endif
#ifdef MXT_SELF_TEST
	mxt_self_test_timer_stop(mxt);
#endif
	mxt_check_touch_ic_timer_stop(mxt);
	mxt_supp_ops_stop(mxt);
	mxt_release_all_fingers(mxt);
	mxt_release_all_keys(mxt);

	if (r_irq)
		disable_irq(mxt->client->irq);
}

static inline void mxt_postrun_reset(struct mxt_data *mxt, bool r_irq)
{
	if (r_irq)
		enable_irq(mxt->client->irq);
}

static int _check_recalibration(struct mxt_data *mxt)
{
	int rc = 0;
	int tch_ch, atch_ch;

	mutex_lock(&mxt->msg_lock);
	rc = check_touch_count(mxt, &tch_ch, &atch_ch);
	if (!rc) {
		if ((tch_ch != 0) || (atch_ch != 0)){
			rc = -EPERM;
			dev_info(&mxt->client->dev, "tch_ch:%d, atch_ch:%d\n",
					tch_ch, atch_ch);
#if TS_100S_TIMER_INTERVAL
			ts_100ms_timer_stop(mxt);
#endif
#ifdef MXT_SELF_TEST
			mxt_self_test_timer_stop(mxt);
#endif
			mxt_check_touch_ic_timer_stop(mxt);
			mxt_supp_ops_stop(mxt);
			atomic_set(&cal_counter, 0);
			calibrate_chip(mxt);
		}
		else
			rc = 0;
	} else {
		atomic_set(&cal_counter, 0);
		dev_err(&mxt->client->dev, "failed to read touch count\n");
		if (rc != -ETIME) {
			/* i2c io error */
			mxt_prerun_reset(mxt, false);
			hw_reset_chip(mxt);
			mxt_postrun_reset(mxt, false);
		} else {
#if TS_100S_TIMER_INTERVAL
			ts_100ms_timer_stop(mxt);
#endif
#ifdef MXT_SELF_TEST
			mxt_self_test_timer_stop(mxt);
#endif
			mxt_check_touch_ic_timer_stop(mxt);
			mxt_supp_ops_stop(mxt);
			calibrate_chip(mxt);
		}
	}
	mutex_unlock(&mxt->msg_lock);

	return rc;
}

static inline void _disable_auto_cal(struct mxt_data *mxt, int locked)
{
	u16 addr;

	addr = MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8);
	if (!locked)
		mutex_lock(&mxt->msg_lock);
	mxt_write_byte(mxt->client, addr + MXT_ADR_T8_TCHAUTOCAL, 0);
	if (!locked)
		mutex_unlock(&mxt->msg_lock);
}

#if 0/* def ENABLE_CHARGER */
/* mode 1 = Charger connected */
/* mode 0 = Charger disconnected */
static void mxt_inform_charger_connection(struct mxt_callbacks *cb, int mode)
{
	struct mxt_data *mxt = container_of(cb, struct mxt_data, callbacks);

	mxt->set_mode_for_ta = !!mode;
	if (mxt->mxt_status && !work_pending(&mxt->ta_work))
		schedule_work(&mxt->ta_work);
}

static void mxt_ta_worker(struct work_struct *work)
{
	struct mxt_data *mxt = container_of(work, struct mxt_data, ta_work);
	int error;

	if (debug >= DEBUG_INFO)
		pr_info("[TSP] %s\n", __func__);
	disable_irq(mxt->client->irq);

	if (mxt->set_mode_for_ta) {
		/* CHRGON disable */
		mxt_write_byte(mxt->client,
				MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T48)
				+ MXT_ADR_T48_CALCFG,
				T48_CALCFG_TA);

		/* T48 config all wirte (TA) */
		mxt_noisesuppression_t48_config_for_TA(mxt);

		/* CHRGON enable */
		error = mxt_write_byte(mxt->client,
				MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T48)
				+ MXT_ADR_T48_CALCFG,
				T48_CALCFG_TA | T48_CHGON_BIT);
	} else {
		/* CHRGON disable */
		mxt_write_byte(mxt->client,
				MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T48)
				+ MXT_ADR_T48_CALCFG,
				T48_CALCFG);

		/* T48 config all wirte (BATT) */
		mxt_noisesuppression_t48_config(mxt);

		/* CHRGON enable */
		error = mxt_write_byte(mxt->client,
				MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T48)
				+ MXT_ADR_T48_CALCFG,
				T48_CALCFG | T48_CHGON_BIT);
	}

	if (error < 0)
		pr_err("[TSP] mxt TA/USB mxt_noise_suppression_config "
				"Error!!\n");
	else {
		if (debug >= DEBUG_INFO) {
			if (mxt->set_mode_for_ta) {
				pr_info("[TSP] mxt TA/USB mxt_noise_supp"
						"ression_config Success!!\n");
			} else {
				pr_info("[TSP] mxt BATTERY mxt_noise_sup"
						"pression_config Success!!\n");
			}
		}
	}

	enable_irq(mxt->client->irq);
	return;
}
#endif

#if 0
static void mxt_metal_suppression_off(struct work_struct *work)
{
	struct	mxt_data *mxt;
	mxt = container_of(work, struct mxt_data, timer_dwork.work);

	metal_suppression_chk_flag = false;
	if (debug >= DEBUG_INFO)
		pr_info("[TSP]%s, metal_suppression_chk_flag = %d\n",
				__func__, metal_suppression_chk_flag);
	disable_irq(mxt->client->irq);
	mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9)
			+ MXT_ADR_T9_AMPHYST, 30);
	enable_irq(mxt->client->irq);
}
#endif

#if 0
/* mxt540E reconfigration */
static void mxt_reconfigration_normal(struct work_struct *work)
{
	struct	mxt_data *mxt;
	int error, id;
	mxt = container_of(work, struct mxt_data, config_dwork.work);

	if (debug >= DEBUG_INFO)
		pr_info("[TSP]%s\n", __func__);

	disable_irq(mxt->client->irq);
	for (id = 0 ; id < MXT_MAX_NUM_TOUCHES ; ++id) {
		if (mtouch_info[id].pressure == -1)
			continue;
		schedule_delayed_work(&mxt->config_dwork, 300);
		pr_warning("[TSP] touch pressed!! %s "
				"didn't execute!!\n", __func__);
		enable_irq(mxt->client->irq);
		return;
	}

	/* 0331 Added for Palm recovery */

	/* T8_ATCHFRCCALTHR */
	error = mxt_write_byte(mxt->client,
		MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8)
		+ MXT_ADR_T8_ATCHFRCCALTHR,
		T8_ATCHFRCCALTHR);

	if (error < 0)
		pr_err("[TSP] %s, %d Error!!\n", __func__, __LINE__);

	/* T8_ATCHFRCCALRATIO */
	error = mxt_write_byte(mxt->client,
		MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8)
		+ MXT_ADR_T8_ATCHFRCCALRATIO,
		0);

	if (error < 0)
		pr_err("[TSP] %s, %d Error!!\n", __func__, __LINE__);

	/* T42_MAXNUMTCHS */
	error = mxt_write_byte(mxt->client,
		MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42)
		+ MXT_ADR_T42_MAXNUMTCHS,
		T42_MAXNUMTCHS);

	if (error < 0)
		pr_err("[TSP] %s, %d Error!!\n", __func__, __LINE__);

	mxt_reconfig_flag = true;
	enable_irq(mxt->client->irq);
	return;
}
#endif

/* Returns object address in mXT chip, or zero if object is not found */
u16 get_object_address(uint8_t object_type,
		       uint8_t instance,
		       struct mxt_object *object_table,
		       int max_objs, int *object_link)
{
	uint8_t object_table_index = 0;
	uint8_t address_found = 0;
	uint16_t address = 0;

	struct mxt_object obj;

	/* check by object_link */
	/* 2012_0508 : Insert the below code. It's faster than while loop */
	if (object_link) {
		obj = object_table[object_link[object_type]];
		if (obj.type == object_type) {
			if (obj.instances >= instance) {
				address = obj.chip_addr +
					(obj.size + 1) * instance;
				return address;
			}
		}
	}

	while ((object_table_index < max_objs) && !address_found) {
		obj = object_table[object_table_index];
		if (obj.type == object_type) {
			address_found = 1;
			/* Are there enough instances defined in the FW? */
			if (obj.instances >= instance) {
				address = obj.chip_addr +
					(obj.size + 1) * instance;
			} else {
				return 0;
			}
		}
		object_table_index++;
	}

	return address;
}

/* Returns object size in mXT chip, or zero if object is not found */
u16 get_object_size(uint8_t object_type, struct mxt_object *object_table,
		int max_objs, int *object_link)
{
	uint8_t object_table_index = 0;
	struct mxt_object obj;

	/* check by object_link */
	/* 2012_0508 : Insert the below code. It's faster than while loop */
	if (object_link) {
		obj = object_table[object_link[object_type]];
		if (obj.type == object_type)
			return obj.size;
	}

	while (object_table_index < max_objs) {
		obj = object_table[object_table_index];
		if (obj.type == object_type)
			return obj.size;
		object_table_index++;
	}
	return 0;
}

/*
* Reads one byte from given address from mXT chip (which requires
* writing the 16-bit address pointer first).
*/

int mxt_read_byte(struct i2c_client *client, u16 addr, u8 *value)
{
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg[2];
	__le16 le_addr = cpu_to_le16(addr);
	struct mxt_data *mxt;

	mxt = i2c_get_clientdata(client);

	msg[0].addr  = client->addr;
	msg[0].flags = 0x00;
	msg[0].len   = 2;
	msg[0].buf   = (u8 *) &le_addr;

	msg[1].addr  = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len   = 1;
	msg[1].buf   = (u8 *) value;
	if  (i2c_transfer(adapter, msg, 2)) {
		mxt->last_read_addr = addr;
		return 0;
	} else {
		/* In case the transfer failed, set last read addr to Invalid
		 * address, so that the next reads won't get confused.  */
		mxt->last_read_addr = -1;
		return -EIO;
	}
}

/*
* Reads a block of bytes from given address from mXT chip. If we are
* reading from message window, and previous read was from message window,
* there's no need to write the address pointer: the mXT chip will
* automatically set the address pointer back to message window start.
*/

int mxt_read_block(struct i2c_client *client,
		   u16 addr,
		   u16 length,
		   u8 *value)
{
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg[2];
	__le16	le_addr;
	struct mxt_data *mxt;
	int res = 0;

	mxt = i2c_get_clientdata(client);
	if (mxt != NULL) {
		if ((mxt->last_read_addr == addr) &&
			(addr == mxt->msg_proc_addr)) {
			if  (i2c_master_recv(client, value, length) == length)
				return 0;
			else {
				dev_err(&client->dev, "1. [TSP] read failed\n");
				return -EIO;
			}
		} else {
			mxt->last_read_addr = addr;
		}
	}

	le_addr = cpu_to_le16(addr);
	msg[0].addr  = client->addr;
	msg[0].flags = 0x00;
	msg[0].len   = 2;
	msg[0].buf   = (u8 *) &le_addr;

	msg[1].addr  = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len   = length;
	msg[1].buf   = (u8 *) value;

	if  ((res = i2c_transfer(adapter, msg, 2))) {
		mxt->last_read_addr = addr;
		return 0;
	} else {
		mxt->last_read_addr = -1;
		return -EIO;
	}
}

/* Reads a block of bytes from current address from mXT chip. */
int mxt_read_block_wo_addr(struct i2c_client *client,
			   u16 length,
			   u8 *value)
{
	if (i2c_master_recv(client, value, length) == length)
		return length;
	else {
		dev_err(&client->dev, "[TSP] read failed\n");
		return -EIO;
	}
}

/* Writes one byte to given address in mXT chip. */
int mxt_write_byte(struct i2c_client *client, u16 addr, u8 value)
{
	struct {
		__le16 le_addr;
		u8 data;
	} i2c_byte_transfer;

	struct mxt_data *mxt;

	mxt = i2c_get_clientdata(client);
	if (mxt != NULL)
		mxt->last_read_addr = -1;

	i2c_byte_transfer.le_addr = cpu_to_le16(addr);
	i2c_byte_transfer.data = value;

	int count = 0;

	if  (count = (i2c_master_send(client, (u8 *) &i2c_byte_transfer, 3)) == 3)
		return 0;
	else
	{
		printk("[%s] write byte error : %d \n",__func__,count );
		return -EIO;
	}
}


/* Writes a block of bytes (max 256) to given address in mXT chip. */
int mxt_write_block(struct i2c_client *client,
		    u16 addr,
		    u16 length,
		    u8 *value)
{
	int i;
	struct {
		__le16	le_addr;
		u8	data[256];

	} i2c_block_transfer;

	struct mxt_data *mxt;

	if (length > 256)
		return -EINVAL;

	mxt = i2c_get_clientdata(client);
	if (mxt != NULL)
		mxt->last_read_addr = -1;

	for (i = 0; i < length; i++)
		i2c_block_transfer.data[i] = *value++;


	i2c_block_transfer.le_addr = cpu_to_le16(addr);
	i = i2c_master_send(client, (u8 *) &i2c_block_transfer, length + 2);

	if (i == (length + 2))
		return length;
	else
	{
		printk("[%s] write block error : %d \n!!!\n",__func__, i );
		return -EIO;
	}
}

/* TODO: make all other access block until the read has been done? Otherwise
an arriving message for example could set the ap to message window, and then
the read would be done from wrong address! */

/* Writes the address pointer (to set up following reads). */
int mxt_write_ap(struct i2c_client *client, u16 ap)
{

	__le16	le_ap = cpu_to_le16(ap);
	struct mxt_data *mxt;

	mxt = i2c_get_clientdata(client);
	if (mxt != NULL)
		mxt->last_read_addr = -1;

	mxt_debug_info(mxt, "[TSP] Address pointer set to %d\n", ap);

	if (i2c_master_send(client, (u8 *) &le_ap, 2) == 2)
		return 0;
	else
		return -EIO;
}

#ifdef MXT_ENABLE_RUN_CRC_CHECK
static u32 mxt_CRC_24(u32 crc, u8 byte1, u8 byte2)
{
	static const u32 crcpoly = 0x80001B;
	u32 result;
	u16 data_word;

	data_word = (u16) ((u16) (byte2 << 8u) | byte1);
	result = ((crc << 1u) ^ (u32) data_word);
	if (result & 0x1000000)
		result ^= crcpoly;
	return result;
}

int calculate_infoblock_crc(u32 *crc_result, struct mxt_data *mxt)
{
	u32 crc = 0;
	u16 crc_area_size;
	u8 *mem;
	int i;

	int error;
	struct i2c_client *client;

	client = mxt->client;

	crc_area_size = MXT_ID_BLOCK_SIZE +
		mxt->device_info.num_objs * MXT_OBJECT_TABLE_ELEMENT_SIZE;

	mem = kmalloc(crc_area_size, GFP_KERNEL);

	if (mem == NULL) {
		dev_err(&client->dev, "[TSP] Error allocating memory\n");
		return -ENOMEM;
	}

	error = mxt_read_block(client, 0, crc_area_size, mem);
	if (error < 0) {
		kfree(mem);
		return error;
	}

	for (i = 0; i < (crc_area_size - 1); i = i + 2)
		crc = mxt_CRC_24(crc, *(mem + i), *(mem + i + 1));

	/* If uneven size, pad with zero */
	if (crc_area_size & 0x0001)
		crc = mxt_CRC_24(crc, *(mem + i), 0);

	kfree(mem);

	/* Return only 24 bits of CRC. */
	*crc_result = (crc & 0x00FFFFFF);
	return 1;
}
#endif

static void process_T9_message(u8 *message, struct mxt_data *mxt)
{
	struct input_dev *input;
	u8 status;
	u16 xpos = 0xFFFF;
	u16 ypos = 0xFFFF;
	u8 report_id;
	u8 touch_id;  /* to identify each touches. starts from 0 to 15 */
	u8 pressed_or_released = 0;
	static int prev_touch_id = -1;
	int i;
	u16 chkpress = 0;
	u8 touch_message_flag = 0;

	input = mxt->input;
	status = message[MXT_MSG_T9_STATUS];

	report_id = message[0];
	if (likely(mxt->report_id_count > 0))
		touch_id = report_id - mxt->rid_map[report_id].first_rid;
	else
		touch_id = report_id - 2;

	if (unlikely(touch_id >= MXT_MAX_NUM_TOUCHES)) {
		//dev_err(&mxt->client->dev,"[TSP] Invalid touch_id (toud_id=%d)",touch_id);
		return;
	}

	/* calculate positions of x, y */
	/* TOUCH_MULTITOUCHSCREEN_T9
	 * [2] XPOSMSB		X position MSByte
	 * [3] YPOSMSB		Y position MSByte
	 * [4] XYPOSLSB		X position lsbits | Y position lsbits
	 * */
	xpos = message[2];
	xpos = xpos << 4;
	xpos = xpos | (message[4] >> 4);
	xpos >>= 2;

	ypos = message[3];
	ypos = ypos << 4;
	ypos = ypos | (message[4] & 0x0F);
	ypos >>= 2;

	/* 2012_0612
	 * ATMEL suggested amplitude limitation
	 */
	mxt_debug_trace(mxt, "Touch amplitude value %d\n",
			       message[MXT_MSG_T9_TCHAMPLITUDE]);
	if (message[MXT_MSG_T9_TCHAMPLITUDE] >= AMPLITUDE_LIMIT) {
		dev_err(&mxt->client->dev, "amplitude is over:%d/%d\n",
				message[MXT_MSG_T9_TCHAMPLITUDE],
				AMPLITUDE_LIMIT);
		return;
	}

	/** 
	 * @author sjpark@cleinsoft
	 * @date 2013/10/07
	 * apply patch code to fix the release message loss.
	 **/
	if (status & MXT_MSGB_T9_RELEASE) {   /* case 2: released */
		pressed_or_released = 1;
		mtouch_info[touch_id].pressure = 0;
	}
	else if (status & MXT_MSGB_T9_DETECT) {   /* case 1: detected */
		touch_message_flag = 1;
		mtouch_info[touch_id].pressure =
			message[MXT_MSG_T9_TCHAMPLITUDE];
		mtouch_info[touch_id].x = (int16_t)xpos;
		mtouch_info[touch_id].y = (int16_t)ypos;

		if (status & MXT_MSGB_T9_PRESS) {
			pressed_or_released = 1;  /* pressed */
#if defined(MXT_DRIVER_FILTER)
			equalize_coordinate(1, touch_id,
					&mtouch_info[touch_id].x,
					&mtouch_info[touch_id].y);
#endif
		} else if (status & MXT_MSGB_T9_MOVE) {
#if defined(MXT_DRIVER_FILTER)
			equalize_coordinate(0, touch_id,
					&mtouch_info[touch_id].x,
					&mtouch_info[touch_id].y);
#endif
		}
	} else if (status & MXT_MSGB_T9_SUPPRESS) {   /* case 3: suppressed */
		mxt_debug_info(mxt, "[TSP] Palm(T9) Suppressed !!!\n");
		facesup_message_flag_T9 = 1;
		pressed_or_released = 1;
		mtouch_info[touch_id].pressure = 0;
	} else {
		dev_err(&mxt->client->dev,
				"[TSP] Unknown status (0x%x)", status);

		if (facesup_message_flag_T9 == 1) {
			facesup_message_flag_T9 = 0;
			mxt_debug_info(mxt, "[TSP] Palm(T92) Suppressed !!!\n");
		}
	}
	/* only get size , id would use TRACKING_ID*/
	/* TCHAREA Field
	 *  Reports the size of the touch area in terms of the number of
	 *  channels that are covered by the touch.
	 *  A reported size of zero indicates that the reported touch is
	 *  a stylus from the Stylus T47 object linked to this Multiple
	 *  Touch Touchscreen T9.
	 *  A reported size of 1 or greater indicates that the reported
	 *  touches are from a finger touch (or a large object, such as
	 *  a palm or a face). For example, the area ... */
	mtouch_info[touch_id].size = message[MXT_MSG_T9_TCHAREA];

	if (prev_touch_id >= touch_id || pressed_or_released) {
		for (i = 0; i <MXT_MAX_NUM_TOUCHES  ; ++i) {
			if (mtouch_info[i].pressure == -1)
				continue;

			mxt_debug_trace(mxt, "[TSP]id:%d, x:%d, y:%d\n", i,
						mtouch_info[i].x,
						mtouch_info[i].y);
			if (mtouch_info[i].pressure == 0) {
				mtouch_info[i].size = 0;
				input_report_key(mxt->input,BTN_TOUCH, 0);
				input_mt_report_slot_state(mxt->input,MT_TOOL_FINGER, false);
				input_report_abs(mxt->input, ABS_MT_POSITION_X,	mtouch_info[i].x);
				input_report_abs(mxt->input, ABS_MT_POSITION_Y,	mtouch_info[i].y);
				input_report_abs(mxt->input, ABS_MT_TOUCH_MAJOR,mtouch_info[i].size );
				input_report_abs(mxt->input, ABS_MT_PRESSURE,mtouch_info[i].pressure);
				mtouch_info[i].pressure = -1;
				//printk("[%s] released  :  size - %d / pressure - %d / touch - %d \n", __func__ , mtouch_info[i].size , mtouch_info[i].pressure, i);  
			} else {
				//printk("[%s] prossed  :  size - %d / pressure - %d / touch - %d\n", __func__ , mtouch_info[i].size , mtouch_info[i].pressure , i);  
				input_report_key(mxt->input,BTN_TOUCH, 1);
				input_mt_report_slot_state(mxt->input,MT_TOOL_FINGER, true);
				input_report_abs(mxt->input, ABS_MT_POSITION_X,	mtouch_info[i].x);
				input_report_abs(mxt->input, ABS_MT_POSITION_Y,	mtouch_info[i].y);
				input_report_abs(mxt->input, ABS_MT_TOUCH_MAJOR,mtouch_info[i].size);
				input_report_abs(mxt->input, ABS_MT_PRESSURE,mtouch_info[i].pressure);
				chkpress++;
			}
			input_mt_sync(mxt->input);
		   //	printk("por X : %d / por Y : %d \n",mtouch_info[i].x, mtouch_info[i].y);
		}
		input_sync(input);  /* TO_CHK: correct position? */
	}
	prev_touch_id = touch_id;

#ifdef CONFIG_KERNEL_DEBUG_SEC
	/* simple touch log */
	if (debug >= DEBUG_MESSAGES) {
		if (status & MXT_MSGB_T9_SUPPRESS) {
			pr_info("[TSP] Suppress\n");
			pr_info("[TSP] r (%d,%d) %d No.%d\n",
					xpos, ypos, touch_id, chkpress);
		} else {
			if (status & MXT_MSGB_T9_PRESS) {
				pr_info("[TSP] P (%d,%d) %d No.%d amp=%d "
					"area=%d\n",
					xpos, ypos, touch_id, chkpress,
					message[MXT_MSG_T9_TCHAMPLITUDE],
					message[MXT_MSG_T9_TCHAREA]);
			} else if (status & MXT_MSGB_T9_RELEASE) {
				pr_info("[TSP] r (%d,%d) %d No.%d\n",
						xpos, ypos, touch_id, chkpress);
			}
		}
	}

	/* detail touch log */
	if (debug >= DEBUG_TRACE) {
		char msg[64] = { 0};
		char info[64] = { 0};
		if (status & MXT_MSGB_T9_SUPPRESS) {
			strcpy(msg, "[TSP] Suppress: ");
		} else {
			if (status & MXT_MSGB_T9_DETECT) {
				strcpy(msg, "Detect(");
				if (status & MXT_MSGB_T9_PRESS)
					strcat(msg, "P");
				if (status & MXT_MSGB_T9_MOVE)
					strcat(msg, "M");
				if (status & MXT_MSGB_T9_AMP)
					strcat(msg, "A");
				if (status & MXT_MSGB_T9_VECTOR)
					strcat(msg, "V");
				strcat(msg, "): ");
			} else if (status & MXT_MSGB_T9_RELEASE) {
				strcpy(msg, "Release: ");
			} else {
				strcpy(msg, "[!] Unknown status: ");
			}
		}
		sprintf(info, "[TSP] (%d,%d) amp=%d, size=%d, id=%d", xpos,
				ypos,
				message[MXT_MSG_T9_TCHAMPLITUDE],
				message[MXT_MSG_T9_TCHAREA],
				touch_id);
		strcat(msg, info);
		pr_info("%s\n", msg);
	}
#endif

	if (mxt->cal_check_flag == 1) {
		/* If chip has recently calibrated and there are any touch or
		 * face suppression messages, we must confirm if
		 * the calibration is good. */
		if (touch_message_flag) {
			/* 2012_0517
			 *  - ATMEL : no need */
			/*if (mxt->ts_100ms_tmr.timer_flag == MXT_DISABLE)
				ts_100ms_timer_enable(mxt);*/
			if (mxt->mxt_time_point == 0)
				mxt->mxt_time_point = jiffies;
			check_chip_calibration(mxt);
		}
	}
	return;
}

#define process_T15_message(msg, mxt) do {} while (0)

static void process_T42_message(u8 *message, struct mxt_data *mxt)
{
	struct	input_dev *input;
	u8  status = false;

	input = mxt->input;
	status = message[MXT_MSG_T42_STATUS];

	/* Message Data for PROCI_TOUCHSUPPRESSION_T42 */
	/* Byte Field  Bit7   Bit6  Bit5    Bit4 Bit3   Bit2 Bit1    Bit0 */
	/* 1	STATUS <---- Reserved ------------------------->     TCHSUP */
	/* TCHSUP: Indicates that touch suppression has been applied to
	 * the linked Multiple Touch Touchscreen T9 object. This field
	 * is set to 1 if suppression is active, and 0 otherwise. */

	if (message[MXT_MSG_T42_STATUS] == MXT_MSGB_T42_TCHSUP) {
		int rc;
		palm_info.palm_release_flag = false;
		mxt_debug_info(mxt, "[TSP] Palm(T42) Suppressed !!!\n");
		/* 0506 LBK */
		if (palm_info.facesup_message_flag
				&& mxt->ts_100ms_tmr.timer_flag)
			return;

		rc = check_chip_palm(mxt);
		if (rc == -ETIME) {
			/* keep going, 4 is nothing execpt keep going */
			palm_info.facesup_message_flag = 4;
		} else if (rc < 0) {
			/* already reset */
			return;
		}

		if (palm_info.facesup_message_flag) {
#ifdef MXT_SELF_TEST
			mxt_self_test_timer_stop(mxt);
#endif
			mxt_check_touch_ic_timer_stop(mxt);
			/* 100ms timer Enable */
			ts_100ms_timer_enable(mxt);
			mxt_debug_info(mxt,
					"[TSP] Palm(T42) Timer start !!!\n");
		}
	} else {
		struct mxt_runmode_registers_t regs;
		mxt_debug_info(mxt, "[TSP] Palm(T42) Released !!!\n");

		mxt_reload_registers(mxt, &regs);
		if (regs.t38_supp_ops & 0x01) {
			/* clear suppression */
			mxt_supp_ops_stop(mxt);
		}

		palm_info.palm_release_flag = true;
		palm_info.facesup_message_flag = 0;

		/* 100ms timer disable */
		ts_100ms_timer_clear(mxt);
	}
	return;
}

static void process_t48_message(u8 *message, struct mxt_data *mxt)
{
	u8 state;
	int f_version;

	state = message[4];
	f_version = (mxt->device_info.major * 10) + (mxt->device_info.minor);

	if (state == 0x05) {
		int rc;
		u16 t38_addr, t48_addr;
		u8 buff[20] = {0, };
		struct i2c_client *client;

		client = mxt->client;
		/* error state */
		dev_err(&client->dev, "[TSP] NOISESUPPRESSION_T48 "
					"error(%d)\n", state);

		t38_addr = MXT_BASE_ADDR(MXT_USER_INFO_T38);
		t48_addr = MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T48);

		rc = mxt_read_byte(mxt->client, t38_addr + MXT_ADR_T38_USER46,
				&buff[0]);
		if (rc < 0)
			goto out_reset;

		if (buff[0] & 0x01) {
			int rot = 0;
			rc = mxt_read_block(mxt->client,
					t38_addr + MXT_ADR_T38_USER46,
					18, buff);
			if (rc < 0)
				goto out_reset;

			rot = (buff[0] & 0xFE) >> 1;
			if (rot >= 10)
				rot = 0;

			mxt_debug_trace(mxt, "rot:%d\n", rot);
			/*** T48 setting change!!! ***/
			/* BASEFREQ(Byte 3): using 10 values */
			mxt_write_byte(mxt->client,
					t48_addr + MXT_ADR_T48_BASEFREQ,
					buff[1 + rot]);
			rot++;
			/* MFFREQ(Byte 8): 1 */
			mxt_write_byte(mxt->client,
					t48_addr + MXT_ADR_T48_MFFREQ_2,
					buff[11]);
			/* MFFREQ(Byte 9): 2 */
			mxt_write_byte(mxt->client,
					t48_addr + MXT_ADR_T48_MFFREQ_3,
					buff[12]);
			/* GCMAXADCSPERX(Byte 17): 100 */
			mxt_write_byte(mxt->client,
					t48_addr + MXT_ADR_T48_GCMAXADCSPERX,
					buff[13]);
			/* MFINVLDDIFFTHR(Byte 22): 20 */
			mxt_write_byte(mxt->client,
					t48_addr + MXT_ADR_T48_MFINVLDDIFFTHR,
					buff[14]);
			if (f_version <= 10) {
				/* MFINCADCSPEXTHR(Byte 23): 2 */
				mxt_write_byte(mxt->client,
						t48_addr + MXT_ADR_T48_MFINCADCSPXTHR,
						buff[15]);
				/* MFERRORTHR(Byte 25): 46 */
				mxt_write_byte(mxt->client,
						t48_addr + MXT_ADR_T48_MFERRORTHR,
						buff[16]);
			}
			/* BLEN(Byte 34): original GAIN - 1 */
			/* 2012_0523
			 *  - ATMEL : remove the below code */
			/* mxt_write_byte(mxt->client,
					t48_addr + MXT_ADR_T48_BLEN,
					buff[17]); */

			/* enable/disable, rot */
			buff[0] &= 0x01;
			buff[0] |= (rot << 1);
			mxt_write_byte(mxt->client,
					t38_addr + MXT_ADR_T38_USER46,
					buff[0]);
		}

		return;
out_reset:
		dev_err(&client->dev, "[TSP] %s() hw-reset by (%d)\n",
				__func__, rc);
		mxt_prerun_reset(mxt, false);
		hw_reset_chip(mxt);
		mxt_postrun_reset(mxt, false);
	}
}

static void process_t6_message(u8 *message, struct mxt_data *mxt)
{
	u8 status;
	struct i2c_client *client;

	status = message[1];
	client = mxt->client;

	if (status & MXT_MSGB_T6_COMSERR) {
		dev_err(&client->dev,
				"[TSP] maXTouch checksum error\n");
	}
	if (status & MXT_MSGB_T6_CFGERR) {
cfgerr:
		dev_err(&client->dev, "[TSP] maXTouch "
				"configuration error\n");
		mxt_prerun_reset(mxt, false);
		/* 2012_0510 : ATMEL */
		/*  - keep the below flow */
		if (!try_sw_reset_chip(mxt, RESET_TO_NORMAL))
			msleep(MXT_SW_RESET_TIME);
		/* re-configurate */
		mxt_config_settings(mxt);
		mxt_config_save_runmode(mxt);
		/* backup to nv memory */
		backup_to_nv(mxt);
		/* forces a reset of the chipset */
		if (!try_sw_reset_chip(mxt, RESET_TO_NORMAL))
			msleep(MXT_SW_RESET_TIME);

		mxt_postrun_reset(mxt, false);
	}
	if (status & MXT_MSGB_T6_CAL) {
		mxt_debug_info(mxt, "[TSP] maXTouch calibration in "
					"progress(cal_check_flag = %d)"
					"\n", mxt->cal_check_flag);
		atomic_set(&cal_counter, 0);

		if (mxt->check_auto_cal_flag == AUTO_CAL_ENABLE)
			maybe_good_count = 0;

		if (mxt->cal_check_flag == 0) {
			/* ATMEL_DEBUG 0406 */
#ifdef MXT_SELF_TEST
			mxt_self_test_timer_stop(mxt);
#endif
			mxt_check_touch_ic_timer_stop(mxt);
			mxt->cal_check_flag = 1;
		} else if (mxt->cal_check_flag == 55) {
			/* fot SIGERR test */
			mxt->cal_check_flag = 0;
			status |= MXT_MSGB_T6_SIGERR;
			dev_info(&client->dev, "SIGERR test\n");
		} else if (mxt->cal_check_flag == 56) {
			mxt->cal_check_flag = 0;
			dev_info(&client->dev, "CFGERR test\n");
			goto cfgerr;
		}
	}
	if (status & MXT_MSGB_T6_SIGERR) {
		dev_err(&client->dev,
			"[TSP] maXTouch acquisition error\n");
		mxt_prerun_reset(mxt, false);
		hw_reset_chip(mxt);
		mxt_postrun_reset(mxt, false);
	}

	if (status & MXT_MSGB_T6_OFL)
		dev_err(&client->dev,
				"[TSP] maXTouch cycle overflow\n");

	if (status & MXT_MSGB_T6_RESET)
		mxt_debug_msg(mxt, "[TSP] maXTouch chip reset\n");

	if (status == MXT_MSG_T6_STATUS_NORMAL)
		mxt_debug_msg(mxt, "[TSP] maXTouch status normal\n");

	/* 2012_0523 :
	 * - ATMEL : please, log the below code */
	mxt_debug_msg(mxt, "T6[0~4] : %02X %02X %02X %02X %02X\n", message[0],
			message[1], message[2], message[3], message[4]);
}

int process_message(u8 *message, u8 object, struct mxt_data *mxt)
{

	struct i2c_client *client;

	u8  status;
	u16 xpos = 0xFFFF;
	u16 ypos = 0xFFFF;
	u8  event;
	u8  length;
	u8  report_id;

	client = mxt->client;
	length = mxt->message_size;
	report_id = message[0];

	mxt_debug_trace(mxt, "process_message 0: (0x%x) 1:(0x%x) object:(%d)",
				message[0], message[1], object);

	switch (object) {
	case MXT_PROCG_NOISESUPPRESSION_T48:
		process_t48_message(message, mxt);
		break;

	case MXT_GEN_COMMANDPROCESSOR_T6:
		process_t6_message(message, mxt);
		break;

	case MXT_TOUCH_MULTITOUCHSCREEN_T9:
		process_T9_message(message, mxt);
		break;

	case MXT_TOUCH_KEYARRAY_T15:
		process_T15_message(message, mxt);
		break;

	case MXT_PROCI_TOUCHSUPPRESSION_T42:
		process_T42_message(message, mxt);
		mxt_debug_msg(mxt, "[TSP] Receiving Touch "
					"suppression msg\n");
		break;

	case MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24:
		mxt_debug_msg(mxt, "[TSP] Receiving one-touch gesture msg\n");

		event = message[MXT_MSG_T24_STATUS] & 0x0F;
		xpos = message[MXT_MSG_T24_XPOSMSB] * 16 +
			((message[MXT_MSG_T24_XYPOSLSB] >> 4) & 0x0F);
		ypos = message[MXT_MSG_T24_YPOSMSB] * 16 +
			((message[MXT_MSG_T24_XYPOSLSB] >> 0) & 0x0F);
		xpos >>= 2;
		ypos >>= 2;

		switch (event) {
		case MT_GESTURE_RESERVED:
			break;
		case MT_GESTURE_PRESS:
			break;
		case MT_GESTURE_RELEASE:
			break;
		case MT_GESTURE_TAP:
			break;
		case MT_GESTURE_DOUBLE_TAP:
			break;
		case MT_GESTURE_FLICK:
			break;
		case MT_GESTURE_DRAG:
			break;
		case MT_GESTURE_SHORT_PRESS:
			break;
		case MT_GESTURE_LONG_PRESS:
			break;
		case MT_GESTURE_REPEAT_PRESS:
			break;
		case MT_GESTURE_TAP_AND_PRESS:
			break;
		case MT_GESTURE_THROW:
			break;
		default:
			break;
		}
		break;
	case MXT_SPT_SELFTEST_T25:
		mxt_debug_msg(mxt, "[TSP] Receiving Self-Test msg\n");
#ifdef MXT_SELF_TEST
		mxt->self_test.flag &= ~MXT_SELF_TEST_FLAG_BAD;

		if (message[MXT_MSG_T25_STATUS] == MXT_MSGR_T25_AVDD_FAULT) {
t25_fault:
			dev_err(&client->dev, "[TSP] AVdd fault\n");
			mxt_prerun_reset(mxt, false);
			hw_reset_chip(mxt);
			mxt_postrun_reset(mxt, false);
		} else {
			if (message[MXT_MSG_T25_STATUS] == MXT_MSGR_T25_OK) {
				mxt_debug_info(mxt, "[TSP] Self-Test OK\n");
				if (unlikely(mxt->self_test.flag & MXT_SELF_TEST_FLAG_T2))
					goto t25_fault;
			} else
				dev_info(&mxt->client->dev,
						"[TSP] Self-Test : 0x%02X\n",
						message[MXT_MSG_T25_STATUS]);
		}
#endif
		break;

	case MXT_SPT_CTECONFIG_T46:
		mxt_debug_msg(mxt, "[TSP] Receiving CTE message...\n");
		status = message[MXT_MSG_T46_STATUS];
		if (status & MXT_MSGB_T46_CHKERR)
			dev_err(&client->dev,
				"[TSP] maXTouch: Power-Up CRC failure\n");

		break;
	default:
		dev_info(&client->dev,
				"[TSP] maXTouch: Unknown message! %d\n", object);
		break;
	}

	return 0;
}


#ifdef MXT_THREADED_IRQ
/* Processes messages when the interrupt line (CHG) is asserted. */

static void mxt_threaded_irq_handler(struct mxt_data *mxt)
{
	struct	i2c_client *client;

	u8	*message = NULL;
	u16	message_length;
	u16	message_addr;
	u8	report_id = 0;
	u8	object;
	int	error;
	bool	need_reset = false;
	int	i;
	int	loop_cnt;

	client = mxt->client;
	message_addr = mxt->msg_proc_addr;
	message_length = mxt->message_size;
	if (likely(message_length < MXT_MSG_BUFF_SIZE)) {
		message = mxt_message_buf;
	} else {
		dev_err(&client->dev,
				"[TSP] Message length larger than 256 bytes "
				"not supported\n");
		return;
	}

	mutex_lock(&mxt->msg_lock);
	/* 2012_0517
	 *  - ATMEL : there is no case over 10 count
	 * 2012_0517
	 *  - tom : increase to 30 */
#define MXT_MSG_LOOP_COUNT 30
	loop_cnt = MXT_MSG_LOOP_COUNT;

	/* mxt_debug_trace(mxt, "[TSP] maXTouch worker active:\n"); */
	do {
		/* Read next message */
		mxt->message_counter++;
		/* Reread on failure! */
		for (i = I2C_RETRY_COUNT; i > 0; i--) {
			/* note: changed message_length to 8 in ver0.9 */
			/* 2012_0510 : ATMEL
			 * - if you want to reduce the message size,
			 *   you can do it.
			 * - but, recommend to use message_length
			 * - if want to reduce the size,
			 *    see MESSAGEPROCESSOR_T5 and MULTITOUCHSCREEN_T9 */
			error = mxt_read_block(client,
					message_addr,
					message_length, message);

			if (error >= 0)
			{
				//I2C success
				//printk("%s message: %c addr: %d length: %d\n", __func__, message, message_addr, message_length);
				break;
			}

			//valky
			printk("%s error!! message: %c addr: %d length: %d error: %d\n", __func__, message, message_addr, message_length, error);
			mxt->read_fail_counter++;
			/* Register read failed */
			dev_info(&client->dev, "[TSP] Failure reading "
					"maxTouch device(%d)\n", i);
		}
		if (i == 0) {
			dev_err(&client->dev, "[TSP] i2c error!\n");
			need_reset = true;
			break;
		}

		report_id = message[0];
		if (debug >= DEBUG_MESSAGES) {
			int p;
			char msg[64] = {0, };

			p = sprintf(msg, "(id:%d)[%d] ",
					report_id, mxt->message_counter);
			for (i = 0; i < message_length; i++)
				p += sprintf(msg + p, "%02X ", message[i]);
			sprintf(msg + p, "\n");

			//dev_info(&client->dev, "%s\n", msg);
		}

		if ((report_id != MXT_END_OF_MESSAGES) && (report_id != 0)) {
			if (unlikely(debug > DEBUG_NONE)) {
				/* this is for debugging.
				 * so, run only debug is higher than
				 *  DEBUG_NONE */
				for (i = 0; i < message_length; i++)
					mxt->last_message[i] = message[i];

				if (down_interruptible(&mxt->msg_sem)) {
					dev_warn(&client->dev,
						"[TSP] mxt_worker Interrupted "
						"while waiting for msg_sem!\n");
					kfree(message);
					mutex_unlock(&mxt->msg_lock);
					return;
				}
				mxt->new_msgs = 1;
				up(&mxt->msg_sem);
				wake_up_interruptible(&mxt->msg_queue);
			}
			/* Get type of object and process the message */
			object = mxt->rid_map[report_id].object;
			process_message(message, object, mxt);
		}

		if (!loop_cnt--) {
			dev_err(&client->dev, "%s() over %d count\n",
					__func__, MXT_MSG_LOOP_COUNT);
			/* there's no way except POR */
			mxt_prerun_reset(mxt, false);
			hw_reset_chip(mxt);
			mxt_postrun_reset(mxt, false);

			mutex_unlock(&mxt->msg_lock);
			return;
		}
	} while ((report_id != MXT_END_OF_MESSAGES) && (report_id != 0));

	mutex_unlock(&mxt->msg_lock);
	mxt_debug_trace(mxt, "loop_cnt : %d\n", MXT_MSG_LOOP_COUNT - loop_cnt);

	/* TSP reset */
	if (need_reset) {
		u8 val = 0;
		/* go to the boot mode for checking the current mode */
		mxt_change_i2c_addr(mxt, true);
		if (i2c_master_recv(client, &val, 1) != 1) {
			dev_err(&client->dev, "reset the chip\n");
			/* back to the application mode */
			mxt_change_i2c_addr(mxt, false);

			mxt_prerun_reset(mxt, false);
			hw_reset_chip(mxt);
			mxt_postrun_reset(mxt, false);
		} else {
			/* this chip needs to be upgraded.
			 * plase wait to upgarde by power on.
			 * there is only one possibility which makes
			 *  this problem.
			 *  - first, failed to upgrade(shutdown and so on)
			 *  - second, resume by FASTBOOT
			 *  because, irq has already registered, and h/w power
			 *  on reset generates the first interrupt.
			 * so, normal boot never happend.
			 * TODO : after fastboot is stable, I'll check again this.
			 * */
			dev_info(&client->dev, "wait to upgrade!\n");
			/* back to the application mode */
			mxt_change_i2c_addr(mxt, false);
		}
	}
}

static irqreturn_t mxt_threaded_irq(int irq, void *_mxt)
{
	struct	mxt_data *mxt = _mxt;
	mxt->irq_counter++;
	mxt_threaded_irq_handler(mxt);
	return IRQ_HANDLED;
}

#else
/* Processes messages when the interrupt line (CHG) is asserted. */

static void mxt_worker(struct work_struct *work)
{
	struct	mxt_data *mxt;
	struct	i2c_client *client;

	u8	*message;
	u16	message_length;
	u16	message_addr;
	u8	report_id;
	u8	object;
	int	error;
	int	i;

	message = NULL;
	mxt = container_of(work, struct mxt_data, dwork.work);
	client = mxt->client;
	message_addr = mxt->msg_proc_addr;
	message_length = mxt->message_size;

	if (message_length < 256) {
		message = kmalloc(message_length, GFP_KERNEL);
		if (message == NULL) {
			dev_err(&client->dev,
					"[TSP] Error allocating memory\n");
			return;
		}
	} else {
		dev_err(&client->dev,
			"[TSP] Message length larger than "
			"256 bytes not supported\n");
	}

	mxt_debug_trace(mxt, "[TSP] maXTouch worker active:\n");
	do {
		/* Read next message */
		mxt->message_counter++;
		/* Reread on failure! */
		for (i = 1; i < I2C_RETRY_COUNT; i++) {
			/* note: changed message_length to 8 in ver0.9 */
			error = mxt_read_block(client, message_addr,
					8/*message_length*/, message);
			if (error >= 0)
				break;
			mxt->read_fail_counter++;
			pr_alert("[TSP] mXT: message read failed!\n");
			/* Register read failed */
			dev_err(&client->dev,
				"[TSP] Failure reading maxTouch device\n");
		}

		report_id = message[0];
		if (debug >= DEBUG_TRACE) {
			pr_info("[TSP] %s message [%08x]:",
				REPORT_ID_TO_OBJECT_NAME(report_id),
				mxt->message_counter
				);
			for (i = 0; i < message_length; i++) {
				pr_info("0x%02x ", message[i]);
			}
			pr_info("\n");
		}

		if ((report_id != MXT_END_OF_MESSAGES) && (report_id != 0)) {

			for (i = 0; i < message_length; i++)
				mxt->last_message[i] = message[i];

			if (down_interruptible(&mxt->msg_sem)) {
				pr_warning("[TSP] mxt_worker Interrupted "
					"while waiting for msg_sem!\n");
				kfree(message);
				return;
			}
			mxt->new_msgs = 1;
			up(&mxt->msg_sem);
			wake_up_interruptible(&mxt->msg_queue);
			/* Get type of object and process the message */
			object = mxt->rid_map[report_id].object;
			process_message(message, object, mxt);
		}

	} while ((report_id != MXT_END_OF_MESSAGES) && (report_id != 0));

	kfree(message);
}

u8 mxt_valid_interrupt(void)
{
	/* TO_CHK: how to implement this function? */
	return 1;
}

/*
* The maXTouch device will signal the host about a new message by asserting
* the CHG line. This ISR schedules a worker routine to read the message when
* that happens.
*/

static irqreturn_t mxt_irq_handler(int irq, void *_mxt)
{
	struct	mxt_data *mxt = _mxt;
	unsigned long	flags;
	mxt->irq_counter++;
	spin_lock_irqsave(&mxt->lock, flags);

	if (mxt_valid_interrupt()) {
		/* Send the signal only if falling edge generated the irq. */
		cancel_delayed_work(&mxt->dwork);
		schedule_delayed_work(&mxt->dwork, 0);
		mxt->valid_irq_counter++;
	} else {
		mxt->invalid_irq_counter++;
	}
	spin_unlock_irqrestore(&mxt->lock, flags);

	return IRQ_HANDLED;
}
#endif


/* Function to write a block of data to any address on touch chip. */

#define I2C_PAYLOAD_SIZE 254

static ssize_t set_config(struct device *dev,
			  struct device_attribute *attr,
			  const char *buf,
			  size_t count)
{
	int i;

	u16 address;
	int whole_blocks;
	int last_block_size;

	struct i2c_client *client  = to_i2c_client(dev);

	address = *((u16 *) buf);
	address = cpu_to_be16(address);
	buf += 2;

	whole_blocks = (count - 2) / I2C_PAYLOAD_SIZE;
	last_block_size = (count - 2) % I2C_PAYLOAD_SIZE;

	for (i = 0; i < whole_blocks; i++) {
		mxt_write_block(client, address, I2C_PAYLOAD_SIZE, (u8 *) buf);
		address += I2C_PAYLOAD_SIZE;
		buf += I2C_PAYLOAD_SIZE;
	}

	mxt_write_block(client, address, last_block_size, (u8 *) buf);

	return count;

}

static ssize_t get_config(struct device *dev,
			  struct device_attribute *attr,
			  char *buf)
{
	int i;
	struct i2c_client *client  = to_i2c_client(dev);
	struct mxt_data *mxt = i2c_get_clientdata(client);

	pr_warning("[TSP] Reading %d bytes from current ap\n",
		mxt->bytes_to_read_ap);

	i = mxt_read_block_wo_addr(client, mxt->bytes_to_read_ap, (u8 *) buf);

	return (ssize_t) i;

}

/*
* Sets up a read from mXT chip. If we want to read config data from user space
* we need to use this first to tell the address and byte count, then use
* get_config to read the data.
*/

static ssize_t set_ap(struct device *dev,
		      struct device_attribute *attr,
		      const char *buf,
		      size_t count)
{

	int i;
	struct i2c_client *client;
	struct mxt_data *mxt;
	u16 ap;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

	if (count < 3) {
		/* Error, ap needs to be two bytes, plus 1 for size! */
		pr_err("[TSP] set_ap needs to arguments: address pointer "
			"and data size");
		return -EIO;
	}

	ap = (u16) *((u16 *)buf);
	i = mxt_write_ap(client, ap);
	mxt->bytes_to_read_ap = (u16) *(buf + 2);
	return count;

}

static ssize_t show_deltas(struct device *dev,
			   struct device_attribute *attr,
			   char *buf)
{
	struct i2c_client *client;
	struct mxt_data *mxt;
	s16     *delta, *pdelta;
	s16     size, read_size;
	u16     diagnostics;
	u16     debug_diagnostics;
	char    *bufp;
	int     x, y;
	int     error;
	u16     *val;
	struct mxt_object obj;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

	/* Allocate buffer for delta's */
	size = mxt->device_info.num_nodes * sizeof(__u16);
	pdelta = delta = kzalloc(size, GFP_KERNEL);
	if (!delta) {
		sprintf(buf, "insufficient memory\n");
		return strlen(buf);
	}

	diagnostics = T6_REG(MXT_ADR_T6_DIAGNOSTICS);
	obj = mxt->object_table[mxt->object_link[MXT_ADR_T6_DIAGNOSTICS]];
	if (obj.type == 0) {
		dev_err(&client->dev, "[TSP] maXTouch: Object T6 not found\n");
		kfree(pdelta);
		return 0;
	}

	debug_diagnostics = T37_REG(2);
	obj = mxt->object_table[mxt->object_link[MXT_DEBUG_DIAGNOSTICS_T37]];
	if (obj.type == 0) {
		kfree(pdelta);
		dev_err(&client->dev, "[TSP] maXTouch: Object T37 not found\n");
		return 0;
	}

	/* Configure T37 to show deltas */
	error = mxt_write_byte(client, diagnostics, MXT_CMD_T6_DELTAS_MODE);
	if (error) {
		dev_err(&client->dev, "[TSP] failed to write(%d)\n", error);
		kfree(pdelta);
		return error;
	}

	while (size > 0) {
		read_size = size > 128 ? 128 : size;
		error = mxt_read_block(client,
			debug_diagnostics,
			read_size,
			(__u8 *) delta);
		if (error < 0) {
			mxt->read_fail_counter++;
			dev_err(&client->dev,
				"[TSP] maXTouch: Error reading delta object\n");
		}
		delta += (read_size / 2);
		size -= read_size;
		/* Select next page */
		mxt_write_byte(client, diagnostics, MXT_CMD_T6_PAGE_UP);
	}

	bufp = buf;
	val  = (s16 *) delta;
	for (x = 0; x < mxt->device_info.x_size; x++) {
		for (y = 0; y < mxt->device_info.y_size; y++)
			bufp += sprintf(bufp, "%05d  ",
			(s16) le16_to_cpu(*val++));
		bufp -= 2;	/* No spaces at the end */
		bufp += sprintf(bufp, "\n");
	}
	bufp += sprintf(bufp, "\n");

	kfree(pdelta);
	return strlen(buf);
}

static ssize_t show_references(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct i2c_client *client;
	struct mxt_data *mxt;
	s16   *reference, *pref;
	s16   size, read_size;
	u16   diagnostics;
	u16   debug_diagnostics;
	char  *bufp;
	int   x, y;
	int   error;
	u16   *val;
	struct mxt_object obj;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);
	/* Allocate buffer for reference's */
	size = mxt->device_info.num_nodes * sizeof(u16);
	pref = reference = kzalloc(size, GFP_KERNEL);
	if (!reference) {
		sprintf(buf, "insufficient memory\n");
		return strlen(buf);
	}

	diagnostics =  T6_REG(MXT_ADR_T6_DIAGNOSTICS);
	obj = mxt->object_table[mxt->object_link[MXT_ADR_T6_DIAGNOSTICS]];
	if (obj.type == 0) {
		dev_err(&client->dev, "[TSP] maXTouch: Object T6 not found\n");
		kfree(pref);
		return 0;
	}
	debug_diagnostics = T37_REG(2);
	obj = mxt->object_table[mxt->object_link[MXT_DEBUG_DIAGNOSTICS_T37]];
	if (obj.type == 0) {
		dev_err(&client->dev, "[TSP] maXTouch: Object T37 not found\n");
		kfree(pref);
		return 0;
	}

	/* Configure T37 to show references */
	mxt_write_byte(client, diagnostics, MXT_CMD_T6_REFERENCES_MODE);
	/* Should check for error */
	while (size > 0) {
		read_size = size > 128 ? 128 : size;
		error = mxt_read_block(client,
			debug_diagnostics,
			read_size,
			(__u8 *) reference);
		if (error < 0) {
			mxt->read_fail_counter++;
			dev_err(&client->dev, "[TSP] maXTouch: Error "
					"reading reference object\n");
		}
		reference += (read_size / 2);
		size -= read_size;
		/* Select next page */
		mxt_write_byte(client, diagnostics, MXT_CMD_T6_PAGE_UP);
	}

	bufp = buf;
	val  = (u16 *) reference;

	for (x = 0; x < mxt->device_info.x_size; x++) {
		for (y = 0; y < mxt->device_info.y_size; y++)
			bufp += sprintf(bufp, "%05d  ", le16_to_cpu(*val++));
		bufp -= 2; /* No spaces at the end */
		bufp += sprintf(bufp, "\n");
	}
	bufp += sprintf(bufp, "\n");

	kfree(pref);
	return strlen(buf);
}

static ssize_t show_device_info(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct i2c_client *client;
	struct mxt_data *mxt;
	char *bufp;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

	bufp = buf;
	bufp += sprintf(bufp,
		"Family:\t\t\t[0x%02x] %s\n",
		mxt->device_info.family_id,
		mxt->device_info.family
		);
	bufp += sprintf(bufp,
		"Variant:\t\t[0x%02x] %s\n",
		mxt->device_info.variant_id,
		mxt->device_info.variant
		);
	bufp += sprintf(bufp,
		"Firmware version:\t[%d.%d], build 0x%02X\n",
		mxt->device_info.major,
		mxt->device_info.minor,
		mxt->device_info.build
		);
	bufp += sprintf(bufp,
		"%d Sensor nodes:\t[X=%d, Y=%d]\n",
		mxt->device_info.num_nodes,
		mxt->device_info.x_size,
		mxt->device_info.y_size
		);
	bufp += sprintf(bufp,
		"Reported resolution:\t[X=%d, Y=%d]\n",
		mxt->pdata->max_x,
		mxt->pdata->max_y
		);
	return strlen(buf);
}

static ssize_t show_stat(struct device *dev,
			 struct device_attribute *attr,
			 char *buf)
{
	struct i2c_client *client;
	struct mxt_data *mxt;
	char *bufp;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

	bufp = buf;
#ifndef MXT_THREADED_IRQ
	bufp += sprintf(bufp,
		"Interrupts:\t[VALID=%d ; INVALID=%d]\n",
		mxt->valid_irq_counter,
		mxt->invalid_irq_counter);
#endif
	bufp += sprintf(bufp, "Messages:\t[%d]\n", mxt->message_counter);
	bufp += sprintf(bufp, "Read Failures:\t[%d]\n", mxt->read_fail_counter);
	return strlen(buf);
}

/* \brief set objnum_show which is the number for showing
 *	the specific object block
 */
static ssize_t set_object_info(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int state;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	if (sscanf(buf, "%i", &state) != 1)
		return -EINVAL;

	mxt->objnum_show = state;

	return count;
}

static ssize_t show_object_info(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	int i, match_id = -1, size = 0;
	char *bufp;
	struct i2c_client *client;
	struct mxt_data	*mxt;
	struct mxt_object *object_table;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);
	object_table = mxt->object_table;

	bufp = buf;

	bufp += sprintf(bufp, "maXTouch: %d Objects\n",
		mxt->device_info.num_objs);

	for (i = 0; i < mxt->device_info.num_objs; i++) {
		if (object_table[i].type != 0) {
			bufp += sprintf(bufp,
				"Type:\t\t[%d]: %s\n",
				object_table[i].type,
				object_type_name[object_table[i].type]);
			bufp += sprintf(bufp,
				"Address:\t0x%04X\n",
				object_table[i].chip_addr);
			bufp += sprintf(bufp,
				"Size:\t\t%d Bytes\n",
				object_table[i].size);
			bufp += sprintf(bufp,
				"Instances:\t%d\n",
				object_table[i].instances);
			bufp += sprintf(bufp,
				"Report Id's:\t%d\n\n",
				object_table[i].num_report_ids);

			if (mxt->objnum_show == object_table[i].type) {
				match_id = mxt->objnum_show;
				size = object_table[i].size;
			}
		}
	}

	/* show registers of the object number */
	bufp += sprintf(bufp, "object number for show:%d\n", mxt->objnum_show);
	if (match_id != -1) {
		int reg_ok = 0;
		u8 *regbuf = kzalloc(size, GFP_KERNEL);

		if (0 == mxt_get_object_values(mxt, match_id)) {
			if (0 == mxt_copy_object(mxt, regbuf, match_id))
				reg_ok = 1;
		} else if (0 == mxt_get_objects(mxt, regbuf, size, match_id))
			reg_ok = 1;

		if (reg_ok) {
			bufp += sprintf(bufp, "   ");
			for (i = 0; i < 16; i++)
				bufp += sprintf(bufp, "%02X ", i);
			bufp += sprintf(bufp, "\n%02X:", 0);
			for (i = 0; i < size; i++) {
				if ((i != 0) && ((i % 16) == 0))
					bufp += sprintf(bufp, "\n%02X:",
							i / 16);
				bufp += sprintf(bufp, "%02X ", (u8)regbuf[i]);
			}
			bufp += sprintf(bufp, "\n");
		}
		kfree(regbuf);
	}

	return strlen(buf);
}

static ssize_t show_messages(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	struct i2c_client *client;
	struct mxt_data   *mxt;
	struct mxt_object *object_table;
	int   i;
	__u8  *message;
	__u16 message_len;
	__u16 message_addr;

	char  *bufp;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);
	object_table = mxt->object_table;

	if (debug <= DEBUG_NONE) {
		dev_info(&client->dev, "enable debug mode\n");
		return -EINVAL;
	}

	bufp = buf;

	message = kmalloc(mxt->message_size, GFP_KERNEL);
	if (message == NULL) {
		pr_warning("Error allocating memory!\n");
		return -ENOMEM;
	}

	message_addr = mxt->msg_proc_addr;
	message_len = mxt->message_size;
	bufp += sprintf(bufp,
		"Reading Message Window [0x%04x]\n",
		message_addr);

	/* Acquire the lock. */
	if (down_interruptible(&mxt->msg_sem)) {
		mxt_debug_info(mxt, "[TSP] mxt: Interrupted while "
				"waiting for mutex!\n");
		kfree(message);
		return -ERESTARTSYS;
	}

	while (mxt->new_msgs == 0) {
		/* Release the lock. */
		up(&mxt->msg_sem);
		if (wait_event_interruptible(mxt->msg_queue, mxt->new_msgs)) {
			mxt_debug_info(mxt, "[TSP] mxt: Interrupted while "
					"waiting for new msg!\n");
			kfree(message);
			return -ERESTARTSYS;
		}

		/* Acquire the lock. */
		if (down_interruptible(&mxt->msg_sem)) {
			mxt_debug_info(mxt, "[TSP] mxt: Interrupted while "
					"waiting for mutex!\n");
			kfree(message);
			return -ERESTARTSYS;
		}
	}

	for (i = 0; i < mxt->message_size; i++)
		message[i] = mxt->last_message[i];

	mxt->new_msgs = 0;

	/* Release the lock. */
	up(&mxt->msg_sem);

	for (i = 0; i < message_len; i++)
		bufp += sprintf(bufp, "0x%02x ", message[i]);
	bufp--;
	bufp += sprintf(bufp, "\t%s\n", REPORT_ID_TO_OBJECT_NAME(message[0]));

	kfree(message);
	return strlen(buf);
}


static ssize_t show_report_id(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	struct i2c_client    *client;
	struct mxt_data      *mxt;
	struct report_id_map *report_id;
	int                  i;
	int                  object;
	char                 *bufp;

	client    = to_i2c_client(dev);
	mxt       = i2c_get_clientdata(client);
	report_id = mxt->rid_map;

	bufp = buf;
	for (i = 0 ; i < mxt->report_id_count ; i++) {
		object = report_id[i].object;
		bufp += sprintf(bufp, "Report Id [%03d], object [%03d], "
			"instance [%03d]:\t%s\n",
			i,
			object,
			report_id[i].instance,
			object_type_name[object]);
	}
	return strlen(buf);
}

static ssize_t show_tch_atch(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	int rc = 0;
	int tch_ch, atch_ch;
	struct mxt_data *mxt;
	struct i2c_client *client;
	char *bufp;
	/* int self_timer_on = 0; */
	int check_touch_ic;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

#ifdef MXT_SELF_TEST
	self_timer_on = mxt->self_test.flag & MXT_SELF_TEST_FLAG_ON;
	if (self_timer_on)
		mxt_self_test_timer_stop(mxt);
#endif
	check_touch_ic = mxt->check_touch_ic_timer.flag &
		MXT_CHECK_TOUCH_IC_FLAG_ON;
	if (check_touch_ic)
		mxt_check_touch_ic_timer_stop(mxt);

	bufp = buf;
	rc = check_touch_count(mxt, &tch_ch, &atch_ch);
	if (!rc) {
		bufp += sprintf(bufp, "tch_ch:%d, atch_ch:%d\n",
				tch_ch, atch_ch);
	} else {
		dev_err(&client->dev, "failed to read touch count\n");
		return -EIO;
	}

#ifdef MXT_SELF_TEST
	if (self_timer_on)
		mxt_self_test_timer_start(mxt);
#endif
	if (check_touch_ic)
		mxt_check_touch_ic_timer_start(mxt);

	return strlen(buf);
}

static ssize_t check_re_calibration(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int state = 0;
	int counter;
	struct i2c_client *client;
	struct mxt_data *mxt;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

	counter = atomic_read(&cal_counter);
	sscanf(buf, "%d", &state);

	if ((state == 1) && (mxt->check_auto_cal_flag == AUTO_CAL_DISABLE) &&
			(counter > 0)) {
		cancel_delayed_work(&mxt->cal_timer.dwork);
		schedule_delayed_work(&mxt->cal_timer.dwork,
				msecs_to_jiffies(200));

	} else {
		dev_info(dev, "invalid parameter:%d\n", state);
		return -EINVAL;
	}

	return count;
}

static ssize_t set_debug(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int state;
	struct i2c_client *client;
	struct mxt_data *mxt;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

	sscanf(buf, "%d", &state);
	if (state >= DEBUG_NONE	&& state <= DEBUG_TRACE) {
		debug = state;
		pr_info("state -\n\tNONE : 0\n" "\tINFO : 1\n"
				"\tMESSAGES : 2\n" "\tTRACE : 3\n"
				"\tWrite to NVRAM : 4\n");
	} else if (state == 4) {
		u8 val;
		u16 t38_addr;

		t38_addr = MXT_BASE_ADDR(MXT_USER_INFO_T38);
		disable_irq(mxt->client->irq);
		val = mxt_read_byte(client,
				t38_addr + MXT_ADR_T38_USER25, &val);
		val &= 0xF0;
		val |= debug;
		val = mxt_write_byte(client,
				t38_addr + MXT_ADR_T38_USER25, val);
		backup_to_nv(mxt);
		enable_irq(mxt->client->irq);
	} else {
		return -EINVAL;
	}

	return count;
}

/* \brief : clear the whole registers */
static void mxt_clear_nvram(struct mxt_data *mxt)
{
	int i;
	struct i2c_client *client;

	client = mxt->client;
	//valky mxt_prerun_reset(mxt, true);

	for (i = 0; i < mxt->device_info.num_objs; i++) {
		int ret;
		u16 obj_addr, obj_size;
		u8 *dummy_data;

		obj_addr = mxt->object_table[i].chip_addr;
		obj_size = mxt->object_table[i].size;

		mxt_debug_trace(mxt, "obj_addr:%02X, size:%d\n",
				obj_addr, obj_size);

		dummy_data = kzalloc(obj_size, GFP_KERNEL);
		if (!dummy_data)
			dev_err(&client->dev, "%s()[%d] : no mem\n",
					__func__, __LINE__);
		ret = mxt_write_block(
				mxt->client, obj_addr, obj_size, dummy_data);
		if (ret < 0)
			dev_err(&client->dev, "%s()[%d] : i2c err(%d)\n",
					__func__, __LINE__, ret);
		kfree(dummy_data);
	}

	//valky mxt_postrun_reset(mxt, true);
}

static ssize_t set_test(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int state;
	struct i2c_client *client;
	struct mxt_data *mxt;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

	sscanf(buf, "%d", &state);

	if (state >= 1 && state <= 2)
		mxt_prerun_reset(mxt, true);

	if (state == 0) {
		/* calibration */
		calibrate_chip(mxt);
	} else if (state == 1) {
		/* s/w reset chip */
		sw_reset_chip(mxt, RESET_TO_NORMAL);
		msleep(MXT_SW_RESET_TIME);
	} else if (state == 2) {
		/* h/w reset chip */
		hw_reset_chip(mxt);
	} else if (state == 3) {
		/* nvram clear */
		mxt_clear_nvram(mxt);
		backup_to_nv(mxt);
	} else if (state == 4) {
		/* re-cal test */
		mxt->cal_check_flag = 1;
	} else if (state == 5) {
		/* i2c force error */
		/* invalid i2c-slave address */
		client->addr = 0x28;
	} else if (state == 6) {
		/* SIGERR test */
		mxt->cal_check_flag = 55;
		sw_reset_chip(mxt, RESET_TO_NORMAL);
		msleep(MXT_SW_RESET_TIME);
	} else if (state == 7) {
		u16 addr;
		/* auto-cal test */
		addr = MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8);
		/* 25 menas (25 * 200) ms */
		mxt_write_byte(client, addr + MXT_ADR_T8_TCHAUTOCAL, 25);
	} 
#ifdef MXT_SELF_TEST
	else if (state == 8 || state == 9 || state == 10) {
		mxt_self_test_timer_stop(mxt);
		mxt_self_test_timer_start(mxt);
		if (state == 8)
			mxt->self_test.flag |= MXT_SELF_TEST_FLAG_T1;
		else if (state == 9)
			mxt->self_test.flag |= MXT_SELF_TEST_FLAG_T2;
	}
#endif
	else if (state == 8) {
		mxt_check_touch_ic_timer_stop(mxt);
		mxt_check_touch_ic_timer_start(mxt);
	} else if (state == 9) {
		mxt_check_touch_ic_timer_stop(mxt);
	} else if (state == 10) {
		/* CFGERR test */
		mxt->cal_check_flag = 56;
		sw_reset_chip(mxt, RESET_TO_NORMAL);
		msleep(MXT_SW_RESET_TIME);
	} else if (state == 11) {
		/* palm test */
		mxt_write_byte(client, MXT_BASE_ADDR(MXT_USER_INFO_T38) +
				MXT_ADR_T38_USER26, 0x02);
		mxt_config_save_runmode(mxt);
	} else {
		pr_info("0:calibration\n1: s/w reset\n2: h/w reset\n"
				"3 : nvram clear\n4: cal_check_flag\n"
				"5 : i2c error\n6 : sigerr\n"
				"7 : auto-cal\n8-9 : touch ic timer\n10 : CFGERR\n"
				"11 : palm test\n");
		return -EINVAL;
	}

	if (state >= 1 && state <= 2)
		mxt_postrun_reset(mxt, true);

	return count;
}

static ssize_t disable_auto_cal(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int state = 0;
	struct i2c_client *client;
	struct mxt_data *mxt;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

	sscanf(buf, "%d", &state);

	if (state == 0) {
		/* auto-cal disable */
		dev_info(dev, "auto-calibration %d\n",
				mxt->check_auto_cal_flag);
		_disable_auto_cal(mxt, false);
		mxt->check_auto_cal_flag = AUTO_CAL_DISABLE;
	} else {
		dev_info(dev, "Enter! echo 0 > disable_auto_cal\n");
		return -EINVAL;
	}

	return count;
}

#ifdef MXT_FIRMUP_ENABLE
static ssize_t show_firmware(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mxt_data *mxt = dev_get_drvdata(dev);
	u8 val[7];

	mxt_read_block(mxt->client, MXT_ADDR_INFO_BLOCK, 7, (u8 *)val);
	mxt->device_info.major = ((val[2] >> 4) & 0x0F);
	mxt->device_info.minor = (val[2] & 0x0F);
	mxt->device_info.build	= val[3];

	return snprintf(buf, PAGE_SIZE,
		"Atmel %s Firmware version [%d.%d] Build %d\n",
		mxt540_variant,
		mxt->device_info.major,
		mxt->device_info.minor,
		mxt->device_info.build);
}

static ssize_t store_firmware(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int state = 0, ret = 0;
	int cmd;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	if (sscanf(buf, "%i", &state) != 1 ||
		(state < MXT_UP_CMD_BEGIN ||
					 state >= MXT_UP_CMD_END))
		return -EINVAL;

	if (state == MXT_UP_AUTO || state == MXT_UP_CHECK_AND_AUTO) {
		if (state == MXT_UP_CHECK_AND_AUTO) {
			/* test the chip status */
			if (MXT_FIRM_STABLE(mxt->firm_status_data)) {
				dev_info(dev, "[TSP] the firmware is stable\n");
				return count;
			}

			/* test whether firmware is out of order */
			mxt_change_i2c_addr(mxt, true);
			ret = mxt_identify(mxt->client, mxt);
			if (-EIO == ret) {
				dev_err(dev, "[TSP] there's no mxt540e "
						"in your board!\n");
				return ret;
			}
		}

		cmd = MXT_FIRMUP_FLOW_LOCK |
			MXT_FIRMUP_FLOW_UPGRADE | MXT_FIRMUP_FLOW_UNLOCK;
	}

	ret = set_mxt_auto_update_exe(mxt, cmd);

	if (ret < 0)
		count = ret;

	return count;
}
#endif

/* \breif to show the current runmode */
static ssize_t show_reg_mode(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u8 val;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	mxt_read_byte(mxt->client,
		MXT_BASE_ADDR(MXT_USER_INFO_T38) + MXT_ADR_T38_CFG_CTRL, &val);

	return snprintf(buf, PAGE_SIZE, "CFG_CTRL[0]:0x%02X\n", val);
}

/* \breif to change the current runmode */
static ssize_t store_reg_mode(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	u8 val = 0;
	int state;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	if (sscanf(buf, "%i", &state) != 1 || (state < 0 || state > 1))
		return -EINVAL;

	disable_irq(mxt->client->irq);

	if (state == 0)
		val = MXT_CFG_OVERWRITE;
	else
		val = MXT_CFG_DEBUG;

	mxt_write_byte(mxt->client,
		MXT_BASE_ADDR(MXT_USER_INFO_T38) + MXT_ADR_T38_CFG_CTRL, val);

	/* the below code should be called to save in NV */
	backup_to_nv(mxt);

	enable_irq(mxt->client->irq);

	return count;
}

static ssize_t show_firm_status(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mxt_data *mxt = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%s\n",
			MXT_FIRM_STABLE(mxt->firm_status_data) ? "stable" :
			"unstable");
}

static void reg_update_dwork(struct work_struct *work)
{
	struct	mxt_data *mxt = NULL;
	mxt = container_of(work, struct mxt_data, reg_update.dwork.work);

	mutex_lock(&mxt->reg_update.lock);
	if (mxt->reg_update.flag != _MXT_REGUP_NOOP) {
		del_timer(&mxt->reg_update.tmr);
		mxt->reg_update.flag = _MXT_REGUP_NOOP;

		enable_irq(mxt->client->irq);
		wake_unlock(&mxt->wakelock);
	}
	mutex_unlock(&mxt->reg_update.lock);

}

/* \brief to check whether the requested update is over or dead
 *
 * after trying to update the registers, this function would be called
 * after 10 seconds. if running still, change th flags and enable irq
 * */
static void reg_update_handler(unsigned long data)
{
	struct mxt_data *mxt = (struct mxt_data *)data;

	dev_info(&mxt->client->dev, "timeout!");

	schedule_delayed_work(&mxt->reg_update.dwork, 0);
}

/* \brief to update the new registers by atmel generated files
 * simple flow (..SECTION.. ; one section string)
 *	@ echo "[BEGIN]" > registers
 *		@ echo "[BEGINV]" > registers (for print register info)
 *	@ echo ..SECTION.. > registers
 *	@ echo ..SECTION.. > registers
 *	@ ..
 *	@ echo "[END]" > registers
 * */
static ssize_t store_registers(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mxt_data *mxt = dev_get_drvdata(dev);

#define _CHECK_UPDATE_FLAG(flag_, err)					\
	do {								\
		if (mxt->reg_update.flag != flag_) {			\
			count = 0;					\
			dev_err(dev, "%s", err);			\
			goto store_registers_err0;			\
		}							\
	} while (0);

	if (!MXT_FIRM_STABLE(mxt->firm_status_data)) {
		dev_err(dev, "[TSP] firmware is not stable");
		return -ENXIO;
	}

	mutex_lock(&mxt->reg_update.lock);
	if (!strncmp(buf, "[BEGIN]", strlen("[BEGIN]"))) {
		int len;
		_CHECK_UPDATE_FLAG(_MXT_REGUP_NOOP, "[TSP] alerady running\n");
		dev_info(dev, "[TSP] start the reigter update\n");
		len = strlen("[BEGIN]");
		/* "[BEGIN]V" means verbose */
		if ((strlen(buf) > len) && (buf[len] == 'V'))
			mxt->reg_update.verbose = 1;
		else
			mxt->reg_update.verbose = 0;
		mxt->reg_update.flag = _MXT_REGUP_RUNNING;

		mxt_get_version(mxt, &prev_ver);

		/* init a timer for time-out */
		init_timer(&(mxt->reg_update.tmr));
		mxt->reg_update.tmr.data = (unsigned long)(mxt);
		mxt->reg_update.tmr.function = reg_update_handler;
		mod_timer(&mxt->reg_update.tmr,
			jiffies + msecs_to_jiffies(1000*10));

#if TS_100S_TIMER_INTERVAL
		ts_100ms_timer_stop(mxt);
#endif
#ifdef MXT_SELF_TEST
		mxt_self_test_timer_stop(mxt);
#endif
		mxt_check_touch_ic_timer_stop(mxt);
		mxt_supp_ops_stop(mxt);

		wake_lock(&mxt->wakelock);
		disable_irq(mxt->client->irq);
	} else if (!strncmp(buf, "[END]", strlen("[END]"))) {
		_CHECK_UPDATE_FLAG(_MXT_REGUP_RUNNING,
				"[TSP] Now, no-running stats\n");
		mxt->reg_update.flag = _MXT_REGUP_NOOP;
		del_timer(&mxt->reg_update.tmr);
		dev_info(dev, "[TSP] end the reigter update\n");

		mxt_config_save_runmode(mxt);

		mxt_update_backup_nvram(mxt, &prev_ver);

		if (!try_sw_reset_chip(mxt, RESET_TO_NORMAL))
			msleep(MXT_SW_RESET_TIME);

		enable_irq(mxt->client->irq);
		wake_unlock(&mxt->wakelock);
	} else {
		_CHECK_UPDATE_FLAG(_MXT_REGUP_RUNNING, "[TSP] please, start\n");
		if (0 != mxt_load_registers(mxt, buf, count)) {
			count = 0;
			dev_err(dev, "[TSP] failed to update registers\n");
		}
	}

store_registers_err0:
	mutex_unlock(&mxt->reg_update.lock);
	return count;
}

#ifdef MXT_FIRMUP_ENABLE
static int set_mxt_auto_update_exe(struct mxt_data *mxt, int cmd)
{
	int ret = 0;
	struct i2c_client *client = mxt->client;

	mxt_debug_trace(mxt, "%s : %d\n", __func__, cmd);

	/* lock status */
	if (cmd & MXT_FIRMUP_FLOW_LOCK) {
		wake_lock(&mxt->wakelock);
		disable_irq(client->irq);
#if TS_100S_TIMER_INTERVAL
		ts_100ms_timer_stop(mxt);
#endif
#ifdef MXT_SELF_TEST
		mxt_self_test_timer_stop(mxt);
#endif
		mxt_check_touch_ic_timer_stop(mxt);
		mxt_supp_ops_stop(mxt);

		if (MXT_FIRM_STABLE(mxt->firm_status_data))
			mxt->firm_status_data = MXT_FIRM_STATUS_START_STABLE;
		else
			mxt->firm_status_data = MXT_FIRM_STATUS_START_UNSTABLE;
	}

	if (cmd & MXT_FIRMUP_FLOW_UPGRADE) {
		ret = mxt_load_firmware(&client->dev,
				MXT540E_FIRMWARE, MXT540E_FIRM_EXTERNAL);
	}
	if (cmd & MXT_FIRMUP_FLOW_UNLOCK) {

		if ((cmd & MXT_FIRMUP_FLOW_UPGRADE) && ret < 0) {
			enable_irq(client->irq);
			wake_unlock(&mxt->wakelock);
			return ret;
		}

		/* 2012.08.16
		 * mdelay(100)
		 * --> after firmware update, slave i2c addr bootloader mode.
		 * request delay time to ATMEL.
		 */
		mdelay(3000);

		/* anyway, It's success to upgrade the requested firmware
		 * even if identify/read_object_table will fail */
		mxt->firm_status_data = MXT_FIRM_STATUS_SUCCESS;

		/* chip reset and re-initialize */
		hw_reset_chip(mxt);

		ret = mxt_identify(client, mxt);
		if (ret >= 0) {
			mxt_debug_info(mxt, "[TSP] mxt_identify Sucess");

			ret = mxt_read_object_table(client, mxt);
			if (ret >= 0) {
				mxt_debug_info(mxt,
					"[TSP] mxt_read_object_table Sucess");
			} else
				dev_err(&client->dev,
					"[TSP] mxt_read_object_table Fail");
		} else
			dev_err(&client->dev,
					"[TSP] mxt_identify Fail ");

		if (ret >= 0) {
			/* mxt->mxt_status = true; */
			mxt_write_init_registers(mxt);
		}

		enable_irq(client->irq);
		wake_unlock(&mxt->wakelock);
	}

	return ret;
}
#endif

#if defined(MXT_FACTORY_TEST)/* def MXT_FACTORY_TEST */
static void set_mxt_update_exe(struct work_struct *work)
{
	struct	mxt_data *mxt;
	int ret, cnt;;
	mxt = container_of(work, struct mxt_data, firmup_dwork.work);

	/*client = mxt->client;*/
	if (debug >= DEBUG_INFO)
		pr_info("[TSP] set_mxt_update_exe\n");

	/* prevents the system from entering suspend during updating */
	/*wake_lock(&mxt->wakelock);*/
	disable_irq(mxt->client->irq);
	ret = mxt_load_firmware(&mxt->client->dev,
			MXT540E_FIRMWARE, MXT540E_FIRM_EXTERNAL);
	mdelay(100);

	/* chip reset and re-initialize */
	hw_reset_chip(mxt);

	ret = mxt_identify(mxt->client, mxt);
	if (ret >= 0) {
		if (debug >= DEBUG_INFO)
			pr_info("[TSP] mxt_identify Sucess ");
	} else {
		pr_err("[TSP] mxt_identify Fail ");
	}

	ret = mxt_read_object_table(mxt->client, mxt);
	if (ret >= 0) {
		if (debug >= DEBUG_INFO)
			pr_info("[TSP] mxt_read_object_table Sucess ");
	} else {
		pr_err("[TSP] mxt_read_object_table Fail ");
	}

	enable_irq(mxt->client->irq);
	/*wake_unlock(&mxt->wakelock);*/

	if (ret >= 0) {
		for (cnt = 10; cnt > 0; cnt--) {
			if (mxt->firm_normal_status_ack == 1) {
				/* firmware update success */
				mxt->firm_status_data = 2;
				if (debug >= DEBUG_INFO)
					pr_info("[TSP] Reprogram done : "
						"Firmware update Success~~~\n");
				break;
			} else {
				if (debug >= DEBUG_INFO)
					pr_info("[TSP] Reprogram done , "
						"but not yet normal status : "
						"3s delay needed\n");
				msleep(500);/* 3s delay */
			}
		}
		if (cnt == 0) {
			/* firmware update Fail */
			mxt->firm_status_data = 3;
			pr_err("[TSP] Reprogram done : Firmware update "
					"Fail ~~~~~~~~~~\n");
		}
	} else {
		/* firmware update Fail */
		mxt->firm_status_data = 3;
		pr_err("[TSP] Reprogram done : Firmware update Fail~~~~~~~\n");
	}
	mxt->firm_normal_status_ack = 0;
}

static ssize_t set_mxt_update_show(
		struct device *dev, struct device_attribute *attr, char *buf)
{
	int count;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	if (debug >= DEBUG_INFO)
		pr_info("[TSP] touch firmware update\n");
	/* start firmware updating */
	mxt->firm_status_data = 1;
	cancel_delayed_work(&mxt->firmup_dwork);
	schedule_delayed_work(&mxt->firmup_dwork, 0);

	if (mxt->firm_status_data == 3)
		count = sprintf(buf, "FAIL\n");
	else
		count = sprintf(buf, "OK\n");
	return count;
}

/*Current(Panel) Version*/
static ssize_t set_mxt_firm_version_read_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mxt_data *mxt = dev_get_drvdata(dev);
	int error, cnt;
	u8 val[7];
	u8 fw_current_version;

	for (cnt = 10; cnt > 0; cnt--) {
		error = mxt_read_block(mxt->client,
				MXT_ADDR_INFO_BLOCK, 7, (u8 *)val);
		if (error < 0) {
			pr_err("[TSP] Atmel touch version read fail it "
					"will try 2s later\n");
			msleep(2000);
		} else {
			break;
		}
	}
	if (cnt == 0) {
		pr_err("[TSP] set_mxt_firm_version_show failed!!!\n");
		fw_current_version = 0;
	}

	mxt->device_info.major = ((val[2] >> 4) & 0x0F);
	mxt->device_info.minor = (val[2] & 0x0F);
	mxt->device_info.build	= val[3];
	fw_current_version = val[2];
	if (debug >= DEBUG_INFO)
		pr_info("[TSP] Atmel %s Firmware version [%d.%d](%d) "
				"Build %d\n",
				mxt540_variant,
				mxt->device_info.major,
				mxt->device_info.minor,
				fw_current_version,
				mxt->device_info.build);

	return sprintf(buf, "Ver %d.%d Build 0x%x\n",
			mxt->device_info.major,
			mxt->device_info.minor,
			mxt->device_info.build);
}

/* Last(Phone) Version */
extern u8 firmware_latest[];
static ssize_t set_mxt_firm_version_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u8 fw_latest_version;
	fw_latest_version = firmware_latest[0];
	if (debug >= DEBUG_INFO)
		pr_info("[TSP] Atmel Last firmware version is %d\n",
				fw_latest_version);
	return sprintf(buf, "Ver %d.%d Build 0x%x\n",
			(firmware_latest[0]>>4) & 0x0f,
			firmware_latest[0] & 0x0f,
			firmware_latest[1]);
}

static ssize_t set_mxt_firm_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int count;
	struct mxt_data *mxt = dev_get_drvdata(dev);
	if (debug >= DEBUG_INFO)
		pr_info("[TSP] Enter firmware_status_show "
				"by Factory command\n");

	if (mxt->firm_status_data == 1)
		count = sprintf(buf, "Downloading\n");
	else if (mxt->firm_status_data == 2)
		count = sprintf(buf, "PASS\n");
	else if (mxt->firm_status_data == 3)
		count = sprintf(buf, "FAIL\n");
	else
		count = sprintf(buf, "PASS\n");

	return count;
}

static ssize_t key_threshold_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u8 val;
	struct mxt_data *mxt = dev_get_drvdata(dev);
	mxt_read_byte(mxt->client,
		MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9)
		+ MXT_ADR_T9_TCHTHR,
		&val);
	return sprintf(buf, "%d\n", val);
}

static ssize_t key_threshold_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct mxt_data *mxt = dev_get_drvdata(dev);
	int i;
	if (sscanf(buf, "%d", &i) == 1) {
		/* prevents the system from entering suspend during updating */
		wake_lock(&mxt->wakelock);
		/* disable interrupt */
		disable_irq(mxt->client->irq);
		mxt_write_byte(mxt->client,
				MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9)
				+ MXT_ADR_T9_TCHTHR, i);
		/* backup to nv memory */
		backup_to_nv(mxt);
		/* forces a reset of the chipset */
		sw_reset_chip(mxt, RESET_TO_NORMAL);
		msleep(MXT_SW_RESET_TIME);

		/* enable interrupt */
		enable_irq(mxt->client->irq);
		wake_unlock(&mxt->wakelock);
		if (debug >= DEBUG_INFO)
			pr_info("[TSP] threshold is changed to %d\n", i);
	} else
		pr_err("[TSP] threshold write error\n");

	return size;
}
#endif

#if 0/* ENABLE_NOISE_TEST_MODE */
uint8_t read_uint16_t(u16 Address, u16 *Data, struct mxt_data *mxt)
{
	uint8_t status;
	uint8_t temp[2];

	status = mxt_read_block(mxt->client, Address, 2, temp);
	*Data = ((uint16_t)temp[1] << 8) + (uint16_t)temp[0];

	return status;
}

int read_dbg_data(u8 dbg_mode, u8 node, u16 *dbg_data, struct mxt_data *mxt)
{
	int status;
	u8 mode, page, i;
	u8 read_page;
	u16 read_point;
	u16 diagnostics;
	u16 diagnostic_addr;

	diagnostic_addr = MXT_BASE_ADDR(MXT_DEBUG_DIAGNOSTICS_T37);
	diagnostics = T6_REG(MXT_ADR_T6_DIAGNOSTICS);

	read_page = node / 64;
	node %= 64;
	read_point = (node * 2) + 2;

	/* Page Num Clear */
	mxt_write_byte(mxt->client, diagnostics, MXT_CMD_T6_CTE_MODE);
	msleep(20);
	mxt_write_byte(mxt->client, diagnostics, dbg_mode);
	msleep(20);

	for (i = 0; i < 5; i++) {
		msleep(20);
		status = mxt_read_byte(mxt->client, diagnostic_addr, &mode);
		if (status == 0) {
			if (mode == dbg_mode)
				break;
		} else {
			pr_err("[TSP] read mode fail\n");
			return status;
		}
	}

	for (page = 0; page < read_page; page++) {
		mxt_write_byte(mxt->client, diagnostics, MXT_CMD_T6_PAGE_UP);
		msleep(10);
	}

	status = read_uint16_t(diagnostic_addr + read_point, dbg_data, mxt);

	msleep(10);

	return status;
}

static ssize_t set_refer0_mode_show(
		struct device *dev, struct device_attribute *attr, char *buf)
{
	int  status;
	u16 qt_refrence = 0;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	status = read_dbg_data(MXT_CMD_T6_REFERENCES_MODE,
			test_node[0], &qt_refrence, mxt);
	return sprintf(buf, "%u\n", qt_refrence);
}

static ssize_t set_refer1_mode_show(
		struct device *dev, struct device_attribute *attr, char *buf)
{
	int  status;
	u16 qt_refrence = 0;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	status = read_dbg_data(MXT_CMD_T6_REFERENCES_MODE,
			test_node[1], &qt_refrence, mxt);
	return sprintf(buf, "%u\n", qt_refrence);
}

static ssize_t set_refer2_mode_show(
		struct device *dev, struct device_attribute *attr, char *buf)
{
	int  status;
	u16 qt_refrence = 0;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	status = read_dbg_data(MXT_CMD_T6_REFERENCES_MODE,
			test_node[2], &qt_refrence, mxt);
	return sprintf(buf, "%u\n", qt_refrence);
}

static ssize_t set_refer3_mode_show(
		struct device *dev, struct device_attribute *attr, char *buf)
{
	int  status;
	u16 qt_refrence = 0;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	status = read_dbg_data(MXT_CMD_T6_REFERENCES_MODE,
			test_node[3], &qt_refrence, mxt);
	return sprintf(buf, "%u\n", qt_refrence);
}

static ssize_t set_refer4_mode_show(
		struct device *dev, struct device_attribute *attr, char *buf)
{
	int  status;
	u16 qt_refrence = 0;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	status = read_dbg_data(MXT_CMD_T6_REFERENCES_MODE,
			test_node[4], &qt_refrence, mxt);
	return sprintf(buf, "%u\n", qt_refrence);
}

static ssize_t set_delta0_mode_show(
		struct device *dev, struct device_attribute *attr, char *buf)
{
	int  status;
	u16 qt_delta = 0;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	status = read_dbg_data(MXT_CMD_T6_DELTAS_MODE,
			test_node[0], &qt_delta, mxt);
	if (qt_delta < 32767)
		return sprintf(buf, "%u\n", qt_delta);
	else {
		qt_delta = 65535 - qt_delta;
		return sprintf(buf, "-%u\n", qt_delta);
	}
}

static ssize_t set_delta1_mode_show(
		struct device *dev, struct device_attribute *attr, char *buf)
{
	int  status;
	u16 qt_delta = 0;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	status = read_dbg_data(MXT_CMD_T6_DELTAS_MODE, test_node[1],
			&qt_delta, mxt);
	if (qt_delta < 32767) {
		return sprintf(buf, "%u\n", qt_delta);
	} else	{
		qt_delta = 65535 - qt_delta;
		return sprintf(buf, "-%u\n", qt_delta);
	}
}

static ssize_t set_delta2_mode_show(
		struct device *dev, struct device_attribute *attr, char *buf)
{
	int  status;
	u16 qt_delta = 0;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	status = read_dbg_data(MXT_CMD_T6_DELTAS_MODE, test_node[2],
			&qt_delta, mxt);
	if (qt_delta < 32767) {
		return sprintf(buf, "%u\n", qt_delta);
	} else {
		qt_delta = 65535 - qt_delta;
		return sprintf(buf, "-%u\n", qt_delta);
	}
}

static ssize_t set_delta3_mode_show(
		struct device *dev, struct device_attribute *attr, char *buf)
{
	int  status;
	u16 qt_delta = 0;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	status = read_dbg_data(MXT_CMD_T6_DELTAS_MODE, test_node[3],
			&qt_delta, mxt);
	if (qt_delta < 32767) {
		return sprintf(buf, "%u\n", qt_delta);
	} else {
		qt_delta = 65535 - qt_delta;
		return sprintf(buf, "-%u\n", qt_delta);
	}
}

static ssize_t set_delta4_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int  status;
	u16 qt_delta = 0;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	status = read_dbg_data(MXT_CMD_T6_DELTAS_MODE, test_node[4],
			&qt_delta, mxt);
	if (qt_delta < 32767) {
		return sprintf(buf, "%u\n", qt_delta);
	} else {
		qt_delta = 65535 - qt_delta;
		return sprintf(buf, "-%u\n", qt_delta);
	}
}

static ssize_t set_threshold_mode_show(
		struct device *dev, struct device_attribute *attr, char *buf)
{
	u8 val;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	mxt_read_byte(mxt->client,
		MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9)
		+ MXT_ADR_T9_TCHTHR,
		&val);
	return sprintf(buf, "%d\n", val);
}
#endif


static int chk_obj(u8 type, int version)
{
	int f_ver;
	f_ver = version;
	if (f_ver <= 10) {
		switch (type) {
		case MXT_GEN_MESSAGEPROCESSOR_T5:
		case MXT_GEN_COMMANDPROCESSOR_T6:
		case MXT_GEN_POWERCONFIG_T7:
		case MXT_GEN_ACQUIRECONFIG_T8:
		case MXT_TOUCH_MULTITOUCHSCREEN_T9:
		case MXT_TOUCH_KEYARRAY_T15:
		case MXT_SPT_COMMSCONFIG_T18:
		case MXT_SPT_GPIOPWM_T19:
		case MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24:
		case MXT_SPT_SELFTEST_T25:
		case MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27:
		case MXT_DEBUG_DIAGNOSTICS_T37:
		case MXT_USER_INFO_T38:
		case MXT_PROCI_GRIPSUPPRESSION_T40:
		case MXT_PROCI_TOUCHSUPPRESSION_T42:
		case MXT_SPT_CTECONFIG_T46:
		case MXT_PROCI_STYLUS_T47:
		case MXT_PROCG_NOISESUPPRESSION_T48:
		case MXT_MESSAGECOUNT_T44:
		case MXT_TOUCH_PROXIMITY_T52:
			return 0;
		default:
			return -1;
		}
	} else if (f_ver >= 21) {
		switch (type) {
		case MXT_GEN_MESSAGEPROCESSOR_T5:
		case MXT_GEN_COMMANDPROCESSOR_T6:
		case MXT_GEN_POWERCONFIG_T7:
		case MXT_GEN_ACQUIRECONFIG_T8:
		case MXT_TOUCH_MULTITOUCHSCREEN_T9:
		case MXT_TOUCH_KEYARRAY_T15:
		case MXT_SPT_COMMSCONFIG_T18:
		case MXT_SPT_GPIOPWM_T19:
		case MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24:
		case MXT_SPT_SELFTEST_T25:
		case MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27:
		case MXT_DEBUG_DIAGNOSTICS_T37:
		case MXT_USER_INFO_T38:
		case MXT_PROCI_GRIPSUPPRESSION_T40:
		case MXT_PROCI_TOUCHSUPPRESSION_T42:
		case MXT_SPT_CTECONFIG_T46:
		case MXT_PROCI_STYLUS_T47:
		case MXT_PROCG_NOISESUPPRESSION_T48:
		case MXT_MESSAGECOUNT_T44:
		case MXT_TOUCH_PROXIMITY_T52:
		case MXT_DATA_SOURCE_T53:
		case MXT_ADAPTIVE_THRESHOLD_T55:
		case MXT_SHIELDLESS_T56:
		case MXT_EXTRA_TOUCH_SCREEN_DATA_T57:
			return 0;
		default:
			return -1;
		}
	}
	return 1;
}

static ssize_t show_message(struct device *dev,
			   struct device_attribute *attr,
			   char *buf)
{
#ifdef TSP_DEBUG_MESSAGE
	struct mxt_data *mxt = NULL;
	char *bufp = NULL;
	unsigned short msg_tail_cnt = 0;
	mxt = dev_get_drvdata(dev);

	bufp = buf;
	bufp += sprintf(bufp,
			"Show recently touch message, msg_log.cnt = %d\n",
			msg_log.cnt);
	msg_tail_cnt = msg_log.cnt + 1;
	msg_tail_cnt &= (MAX_MSG_LOG_SIZE - 1);

	do {
		bufp += sprintf(bufp, "%d,\t", msg_log.id[msg_tail_cnt]);
		bufp += sprintf(bufp, "%x,\t", msg_log.status[msg_tail_cnt]);
		bufp += sprintf(bufp, "%d,\t", msg_log.xpos[msg_tail_cnt]);
		bufp += sprintf(bufp, "%d,\t", msg_log.ypos[msg_tail_cnt]);
		bufp += sprintf(bufp, "%d,\t", msg_log.area[msg_tail_cnt]);
		bufp += sprintf(bufp, "%d\n", msg_log.amp[msg_tail_cnt]);

		msg_tail_cnt++;
	} while (msg_tail_cnt != msg_log.cnt);

	for (msg_log.cnt = 0; msg_log.cnt < 128; msg_log.cnt++) {
		msg_log.id[msg_log.cnt] = 0;
		msg_log.status[msg_log.cnt] = 0;
		msg_log.xpos[msg_log.cnt] = 0;
		msg_log.ypos[msg_log.cnt] = 0;
		msg_log.area[msg_log.cnt] = 0;
		msg_log.amp[msg_log.cnt] = 0;
	}
	msg_log.cnt = 0;
#endif
	return strlen(buf);

}


static ssize_t show_object(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mxt_data *mxt;
	struct mxt_object  *object_table;

	int count = 0;
	int i, j;
	int version;
	u8 val;

	mxt = dev_get_drvdata(dev);
	object_table = mxt->object_table;

	version = (mxt->device_info.major * 10) + (mxt->device_info.minor);

	pr_info("maXTouch: %d Objects\n",
			mxt->device_info.num_objs);

	for (i = 0; i < mxt->device_info.num_objs; i++) {
		u8 obj_type = object_table[i].type;

		if (chk_obj(obj_type, version))
			continue;

		count += sprintf(buf + count, "%s: %d addr %d bytes",
			object_type_name[obj_type], MXT_BASE_ADDR(obj_type),
			object_table[i].size);

		for (j = 0; j < object_table[i].size; j++) {
			mxt_read_byte(mxt->client,
					MXT_BASE_ADDR(obj_type) + (u16)j,
					&val);
			if (!(j % 8))
				count += sprintf(buf + count, "\n%02X : ", j);
			count += sprintf(buf + count, " %02X", val);
		}

		count += sprintf(buf + count, "\n\n");
	}

	/* debug only */
	/*
	count += sprintf(buf + count, "%s: %d bytes\n", "debug_config_T0", 32);

	for (j = 0; j < 32; j++) {
		count += sprintf(buf + count,
			"  Byte %2d: 0x%02x (%d)\n",
			j,
			mxt->debug_config[j],
			mxt->debug_config[j]);
	}
	* */

	return count;
}

static ssize_t store_object(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mxt_data *mxt;
	/*	struct mxt_object	*object_table;//TO_CHK: not used now */
	struct i2c_client *client;

	unsigned int type, offset, val;
	u8 val8;
	u16 chip_addr;
	int ret;

	mxt = dev_get_drvdata(dev);
	client = mxt->client;

	if ((sscanf(buf, "%u %u %u", &type, &offset, &val) != 3)
			|| (type >= MXT_MAX_OBJECT_TYPES)) {
		pr_err("Invalid values");
		return -EINVAL;
	}

	mxt_debug_info(mxt, "[TSP] Object type: %u, Offset: %u, Value: %u\n",
			type, offset, val);

	/* debug only */
	/*
	count += sprintf(buf + count, "%s: %d bytes\n", "debug_config_T0", 32);
	*/

	if (type == 0) {
		/* mxt->debug_config[offset] = (u8)val; */
	} else {
		chip_addr = get_object_address(type, 0, mxt->object_table,
			mxt->device_info.num_objs, mxt->object_link);
		if (chip_addr == 0) {
			dev_err(&client->dev,
					"[TSP] Invalid object type(%d)!", type);
			return -EIO;
		}

		ret = mxt_write_byte(client, chip_addr + (u16)offset, (u8)val);
		if (ret < 0) {
			dev_err(&client->dev,
					"[TSP] store_object result: (%d)\n",
					ret);
			return ret;
		}

		mxt_config_save_runmode(mxt);
		mxt_read_byte(client, MXT_BASE_ADDR(MXT_USER_INFO_T38) +
			MXT_ADR_T38_CFG_CTRL, &val8);

		if (val8 == MXT_CFG_DEBUG) {
			backup_to_nv(mxt);
			dev_info(&client->dev, "[TSP] backup_to_nv\n");
		}
	}

	return count;
}

#if 0  /* FOR_TEST */
static ssize_t test_suspend(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char *bufp;
	struct early_suspend  *fake;

	bufp = buf;
	bufp += sprintf(bufp, "Running early_suspend function...\n");

	fake = kzalloc(sizeof(struct early_suspend), GFP_KERNEL);
	mxt_early_suspend(fake);
	kfree(fake);

	return strlen(buf);
}

static ssize_t test_resume(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char *bufp;
	struct early_suspend  *fake;

	bufp = buf;
	bufp += sprintf(bufp, "Running late_resume function...\n");

	fake = kzalloc(sizeof(struct early_suspend), GFP_KERNEL);
	mxt_late_resume(fake);
	kfree(fake);

	return strlen(buf);
}
#endif

#if 0/* def KEY_LED_CONTROL */
static void key_led_on(struct mxt_data *mxt, u32 val)
{
	if (mxt->pdata->key_led_en1 != NULL)
		gpio_direction_output(mxt->pdata->key_led_en1,
				(val & 0x01) ? true : false);
	if (mxt->pdata->key_led_en2 != NULL)
		gpio_direction_output(mxt->pdata->key_led_en2,
				(val & 0x02) ? true : false);
	if (mxt->pdata->key_led_en3 != NULL)
		gpio_direction_output(mxt->pdata->key_led_en3,
				(val & 0x04) ? true : false);
	if (mxt->pdata->key_led_en4 != NULL)
		gpio_direction_output(mxt->pdata->key_led_en4,
				(val & 0x08) ? true : false);
}

static ssize_t key_led_store(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t size)
{
	struct mxt_data *mxt = dev_get_drvdata(dev);
	u32 i = 0;
	if (sscanf(buf, "%d", &i) != 1)
		pr_err("[TSP] keyled write error\n");

	if (mxt->mxt_status)
		key_led_on(mxt, i);

	if (debug >= DEBUG_INFO)
		pr_info("[TSP] Called value by HAL = %d\n", i);
	if (i)
		key_led_status = true;
	else
		key_led_status = false;

	return size;
}

static DEVICE_ATTR(brightness, S_IRUGO | S_IWUSR, NULL, key_led_store);
#endif


/* Register sysfs files */

static DEVICE_ATTR(deltas,      S_IRUGO, show_deltas,      NULL);
static DEVICE_ATTR(references,  S_IRUGO, show_references,  NULL);
static DEVICE_ATTR(device_info, S_IRUGO, show_device_info, NULL);
static DEVICE_ATTR(object_info, S_IWUSR | S_IRUSR,
		show_object_info, set_object_info);
static DEVICE_ATTR(messages,    S_IRUGO, show_messages,    NULL);
static DEVICE_ATTR(report_id,   S_IRUGO, show_report_id,   NULL);
static DEVICE_ATTR(stat,        S_IRUGO, show_stat,        NULL);
static DEVICE_ATTR(config,      S_IWUSR|S_IRUGO, get_config, set_config);
static DEVICE_ATTR(ap,          S_IWUSR, NULL,             set_ap);
static DEVICE_ATTR(debug, S_IWUSR, NULL, set_debug);
#ifdef MXT_FIRMUP_ENABLE
static DEVICE_ATTR(firmware_up, (S_IWUSR | S_IWGRP) | S_IRUGO,
		show_firmware, store_firmware);
#endif
static DEVICE_ATTR(object, S_IWUSR|S_IRUGO, show_object, store_object);
static DEVICE_ATTR(message,     S_IRUGO, show_message,      NULL);
static DEVICE_ATTR(registers,  (S_IRUSR | S_IWUSR) | (S_IRGRP | S_IWGRP),
		NULL, store_registers);
static DEVICE_ATTR(reg_mode,   (S_IRUSR | S_IWUSR) | (S_IRGRP | S_IWGRP),
		show_reg_mode, store_reg_mode);
#ifdef MXT_FIRMUP_ENABLE
static DEVICE_ATTR(firm_status,   S_IRUSR | S_IRGRP,
		show_firm_status, NULL);
#endif
static DEVICE_ATTR(test, S_IWUSR, NULL, set_test);
static DEVICE_ATTR(tch_atch, (S_IWUSR | S_IRUSR),
		show_tch_atch, check_re_calibration);
static DEVICE_ATTR(disable_auto_cal, S_IWUSR, NULL, disable_auto_cal);

/* static DEVICE_ATTR(suspend, S_IRUGO, test_suspend, NULL); */
/* static DEVICE_ATTR(resume, S_IRUGO, test_resume, NULL);  */

#if defined(MXT_FACTORY_TEST)/* def MXT_FACTORY_TEST */
/* firmware update */
static DEVICE_ATTR(tsp_firm_update, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH,
		set_mxt_update_show, NULL);
/* firmware update status return */
static DEVICE_ATTR(tsp_firm_update_status, S_IRUGO | S_IWUSR | S_IWOTH \
		| S_IXOTH, set_mxt_firm_status_show, NULL);
/* touch threshold return, store */
static DEVICE_ATTR(tsp_threshold, S_IRUGO | S_IWUSR, key_threshold_show,
		key_threshold_store);
/* firmware version resturn in phone driver version */
static DEVICE_ATTR(tsp_firm_version_phone, S_IRUGO | S_IWUSR | S_IWOTH \
		| S_IXOTH, set_mxt_firm_version_show, NULL);
/* firmware version resturn in TSP panel version */
static DEVICE_ATTR(tsp_firm_version_panel, S_IRUGO | S_IWUSR | S_IWOTH \
		| S_IXOTH, set_mxt_firm_version_read_show, NULL);
#endif

#if 0 /* ENABLE_NOISE_TEST_MODE */
static DEVICE_ATTR(set_refer0, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH,
		set_refer0_mode_show, NULL);
static DEVICE_ATTR(set_delta0, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH,
		set_delta0_mode_show, NULL);
static DEVICE_ATTR(set_refer1, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH,
		set_refer1_mode_show, NULL);
static DEVICE_ATTR(set_delta1, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH,
		set_delta1_mode_show, NULL);
static DEVICE_ATTR(set_refer2, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH,
		set_refer2_mode_show, NULL);
static DEVICE_ATTR(set_delta2, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH,
		set_delta2_mode_show, NULL);
static DEVICE_ATTR(set_refer3, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH,
		set_refer3_mode_show, NULL);
static DEVICE_ATTR(set_delta3, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH,
		set_delta3_mode_show, NULL);
static DEVICE_ATTR(set_refer4, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH,
		set_refer4_mode_show, NULL);
static DEVICE_ATTR(set_delta4, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH,
		set_delta4_mode_show, NULL);
static DEVICE_ATTR(set_threshould, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH,
		set_threshold_mode_show, NULL);
#endif


static struct attribute *maxTouch_attributes[] = {
	&dev_attr_deltas.attr,
	&dev_attr_references.attr,
	&dev_attr_device_info.attr,
	&dev_attr_object_info.attr,
	&dev_attr_messages.attr,
	&dev_attr_report_id.attr,
	&dev_attr_stat.attr,
	&dev_attr_config.attr,
	&dev_attr_ap.attr,
	&dev_attr_debug.attr,
#ifdef MXT_FIRMUP_ENABLE
	&dev_attr_firmware_up.attr,
#endif
	&dev_attr_object.attr,
	&dev_attr_message.attr,
	&dev_attr_registers.attr,
	&dev_attr_reg_mode.attr,
#ifdef MXT_FIRMUP_ENABLE
	&dev_attr_firm_status.attr,
#endif
	/* &dev_attr_suspend.attr, */
	/* &dev_attr_resume.attr, */
	&dev_attr_test.attr,
	&dev_attr_tch_atch.attr,
	&dev_attr_disable_auto_cal.attr,
	NULL,
};

static struct attribute_group maxtouch_attr_group = {
	.attrs = maxTouch_attributes,
};

#if defined(MXT_FACTORY_TEST)/* def MXT_FACTORY_TEST */
static struct attribute *maxTouch_facotry_attributes[] = {
	&dev_attr_tsp_firm_update.attr,
	&dev_attr_tsp_firm_update_status.attr,
	&dev_attr_tsp_threshold.attr,
	&dev_attr_tsp_firm_version_phone.attr,
	&dev_attr_tsp_firm_version_panel.attr,

#if ENABLE_NOISE_TEST_MODE
	&dev_attr_set_refer0.attr,
	&dev_attr_set_delta0.attr,
	&dev_attr_set_refer1.attr,
	&dev_attr_set_delta1.attr,
	&dev_attr_set_refer2.attr,
	&dev_attr_set_delta2.attr,
	&dev_attr_set_refer3.attr,
	&dev_attr_set_delta3.attr,
	&dev_attr_set_refer4.attr,
	&dev_attr_set_delta4.attr,
	&dev_attr_set_threshould.attr,
#endif
	NULL,
};
#endif

#if defined(MXT_FACTORY_TEST)/* def MXT_FACTORY_TEST */
static struct attribute_group maxtouch_factory_attr_group = {
	.attrs = maxTouch_facotry_attributes,
};
#endif

/* This function sends a calibrate command to the maXTouch chip.
* While calibration has not been confirmed as good, this function sets
* the ATCHCALST and ATCHCALSTHR to zero to allow a bad cal to always recover
* Returns WRITE_MEM_OK if writing the command to touch chip was successful.
*/
unsigned char not_yet_count;
/* extern gen_acquisitionconfig_t8_config_t acquisition_config; */
/* extern int mxt_acquisition_config(struct mxt_data *mxt); */

#ifdef MXT_SELF_TEST
static void mxt_run_self_test(struct mxt_data *mxt)
{
	u8 t_buff[2] = {0, };

	/* SPT_SELFTEST_T25
	 * CTRL Field
	 * [ENABLE]: Enables the object. The object is enabled if set to 1,
	 *  and disabled if set to 0.
	 * [RPTEN]: Allows the object to send status messages to the host
	 *  through the Message Processor T5 object.
	 *  Reporting is enabled if set to 1, and disabled if set to 0.
	 * CMD Field
	 *  This field is used to send test commands to the device.
	 *   Valid test commands are listed in Table 7-9
	 *   ~
	 *  0x01 : Test for AVdd power present
	 *   ~
	 * */
	t_buff[0] = 0x03;
	t_buff[1] = 0x01;

	mutex_lock(&mxt->msg_lock);
	if (!(mxt->self_test.flag & MXT_SELF_TEST_FLAG_ON))
		goto out;
	/* set to bad */
	mxt->self_test.flag |= MXT_SELF_TEST_FLAG_BAD;

	mxt_write_block(mxt->client, MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25),
			2, t_buff);
out:
	mutex_unlock(&mxt->msg_lock);
}

static void mxt_self_test(struct work_struct *work)
{
	struct mxt_data *mxt = NULL;
	mxt = container_of(work, struct mxt_data, self_test.dwork.work);

	mxt_run_self_test(mxt);

	cancel_delayed_work(&mxt->self_test.check_dwork);
	schedule_delayed_work(&mxt->self_test.check_dwork,
			msecs_to_jiffies(MXT_SELF_TEST_CHECK_TIME));
}

static void mxt_self_test_check(struct work_struct *work)
{
	int bad = 0;
	struct mxt_data *mxt = NULL;
	mxt = container_of(work, struct mxt_data, self_test.check_dwork.work);

	mutex_lock(&mxt->msg_lock);
	bad = mxt->self_test.flag & MXT_SELF_TEST_FLAG_BAD;
	if (unlikely(mxt->self_test.flag & MXT_SELF_TEST_FLAG_T1))
		bad = 1;
	mutex_unlock(&mxt->msg_lock);
	/* if still bad, reset */
	if (bad) {
		dev_err(&mxt->client->dev, "[TSP] chip is out of order!\n");
		mxt_prerun_reset(mxt, true);
		hw_reset_chip(mxt);
		mxt_postrun_reset(mxt, true);
	} else {
		if (mxt->self_test.flag & MXT_SELF_TEST_FLAG_ON)
			mod_timer(&mxt->self_test.timer,
				jiffies +
				msecs_to_jiffies(MXT_SELF_TEST_TIMER_TIME));
	}
}

static void mxt_self_test_timeout_handler(unsigned long data)
{
	struct mxt_data *mxt = (struct mxt_data *)data;

	schedule_delayed_work(&mxt->self_test.dwork, 0);
}

static void mxt_self_test_timer_stop(struct mxt_data *mxt)
{
	if (mxt->self_test.flag & MXT_SELF_TEST_FLAG_ON) {
		cancel_delayed_work(&mxt->self_test.check_dwork);
		cancel_delayed_work(&mxt->self_test.dwork);
		del_timer(&mxt->self_test.timer);
	}
	mxt->self_test.flag = MXT_SELF_TEST_FLAG_OFF;
}

static void mxt_self_test_timer_start(struct mxt_data *mxt)
{
	cancel_delayed_work(&mxt->self_test.check_dwork);
	cancel_delayed_work(&mxt->self_test.dwork);
	mxt->self_test.timer.expires =
		jiffies + msecs_to_jiffies(MXT_SELF_TEST_TIMER_TIME);
	add_timer(&mxt->self_test.timer);
	mxt->self_test.flag = MXT_SELF_TEST_FLAG_ON;
}

static void mxt_self_test_timer_init(struct mxt_data *mxt)
{
	mxt->self_test.flag = MXT_SELF_TEST_FLAG_OFF;
	init_timer(&(mxt->self_test.timer));
	mxt->self_test.timer.data = (unsigned long)(mxt);
	mxt->self_test.timer.function = mxt_self_test_timeout_handler;
}
#endif

static void mxt_supp_ops_dwork(struct work_struct *work)
{
	struct mxt_data *mxt = NULL;
	mxt = container_of(work, struct mxt_data, supp_ops.dwork.work);

	dev_info(&mxt->client->dev,
			"%s() suppression time-out\n", __func__);
	calibrate_chip(mxt);
}

static inline void mxt_supp_ops_stop(struct mxt_data *mxt)
{
	cancel_delayed_work(&mxt->supp_ops.dwork);
}

static void mxt_cal_timer_dwork(struct work_struct *work)
{
	int rc;
	struct mxt_data *mxt = NULL;
	mxt = container_of(work, struct mxt_data, cal_timer.dwork.work);

	dev_info(&mxt->client->dev,
			"%s() cal-timer start !!!\n", __func__);
	rc = _check_recalibration(mxt);

	if (rc == 0)
		atomic_dec(&cal_counter);
}

static void mxt_check_touch_ic_timer_dwork(struct work_struct *work)
{
	int rc = 0;
	u8 val;
	u16 t25_addr;
	struct mxt_data *mxt = NULL;
	mxt = container_of(work, struct mxt_data,
			check_touch_ic_timer.dwork.work);
	t25_addr = MXT_BASE_ADDR(MXT_SPT_SELFTEST_T25);

	if (mxt->check_touch_ic_timer.flag == MXT_CHECK_TOUCH_IC_FLAG_ON)
		rc = mxt_read_byte(mxt->client,
				t25_addr + MXT_ADR_T25_CMD, &val);
	if (rc < 0) {
		dev_err(&mxt->client->dev, "[TSP] %s() hw-reset by (%d)\n",
				__func__, rc);
		mxt_prerun_reset(mxt, true);
		hw_reset_chip(mxt);
		mxt_postrun_reset(mxt, true);
		return;
	} else
		mxt_debug_trace(mxt, "Touch IC OK !");

	cancel_delayed_work(&mxt->check_touch_ic_timer.dwork);
	schedule_delayed_work(&mxt->check_touch_ic_timer.dwork,
			msecs_to_jiffies(MXT_CHK_TOUCH_IC_TIME));
}

static void mxt_check_touch_ic_timer_start(struct mxt_data *mxt)
{
	cancel_delayed_work(&mxt->check_touch_ic_timer.dwork);
	schedule_delayed_work(&mxt->check_touch_ic_timer.dwork,
			msecs_to_jiffies(MXT_CHK_TOUCH_IC_TIME));
	mxt->check_touch_ic_timer.flag = MXT_CHECK_TOUCH_IC_FLAG_ON;
}

static inline void mxt_check_touch_ic_timer_stop(struct mxt_data *mxt)
{
	cancel_delayed_work(&mxt->check_touch_ic_timer.dwork);
	mxt->check_touch_ic_timer.flag = MXT_CHECK_TOUCH_IC_FLAG_OFF;
}

static int mxt_wait_touch_flag(struct mxt_data *mxt, uint16_t addr_t37,
		uint16_t addr_t6, unsigned char cmd)
{
	int rc = 0;
	unsigned char page = 0;
	char buff[2] = {0,};
	int retry_count = 50;
	struct i2c_client *client = mxt->client;
	unsigned char t48_tchthr;
	u16 t48_addr = MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T48);
	int delay = 5;

	/* run the pre-process */
	if (0xF3 != cmd) {
		rc = mxt_read_byte(client,
				t48_addr + MXT_ADR_T48_TCHTHR, &t48_tchthr);
		if (!rc)
			rc = mxt_write_byte(client,
					t48_addr + MXT_ADR_T48_TCHTHR, 20);

		if (rc < 0) {
			dev_err(&client->dev, "%s() cmd:%d, err:%d\n",
					__func__, cmd, rc);
			return rc;
		}

		/* 2012_0514
		 * - (13 + margin)ms is the time which takes to be an active
		 *   state after changing TCHTHR to 20
		 * */
		msleep(15);
	}

	/* run the command */
retry_cmd:
	if (0xF3 == cmd) {
	/* we have had the first touchscreen or face suppression message
	 * after a calibration - check the sensor state and try to confirm if
	 * cal was good or bad
	 * get touch flags from the chip using the diagnostic object
	 * write command to command processor to get touch flags -
	 * 0xF3 Command required to do this */
		rc = mxt_write_byte(client,
				addr_t6 + MXT_ADR_T6_DIAGNOSTICS, cmd);
		page = 0;
	} else if (0x01 == cmd /* || 0x02 == cmd */) {
		/* page-up or page-down */
		rc = mxt_write_byte(client,
				addr_t6 + MXT_ADR_T6_DIAGNOSTICS, cmd);
		page = 1;
	} else {
		WARN(1, "Invalid command:0x%02X\n", cmd);
		return rc;
	}
	/* wait time */
	/* 2012_0523 :
	 * ATMEL : try 5ms at first, and 13ms next time
	 * - first time : 5ms,
	 * - after : 13ms */
	if (retry_count < 50)
		delay = 13;

	msleep(delay);

	/* wait for diagnostic object to update */
	buff[0] = buff[1] = 0xFF;
	rc = mxt_read_block(client, addr_t37, 2, buff);
	if (!rc && (buff[0] == 0xF3 && buff[1] == page)) {
		/* success */
		goto out_ret;
	} else {
		if (retry_count <= 0 || rc) {
			if (retry_count <= 0) {
				rc = -ETIME;
				dev_err(&client->dev,
						"%s(): over the retry_count\n",
						__func__);
			}
			if (!rc)
				goto out_ret;
			else
				return rc;
		} else {
			dev_warn(&client->dev,
				"retry_count:%d, cmd:0x%02X, page:%d, rc:%d\n",
				retry_count, cmd, page, rc);
			retry_count--;
			goto retry_cmd;
		}

		/* wait time */
		/* 2012_0523 :
		 *  ATMEL : remove the below code */
		/* msleep(2); */
	}

out_ret:
	if (0x01 == cmd /* || 0x02 == cmd */) {
		rc = mxt_write_byte(client,
			t48_addr + MXT_ADR_T48_TCHTHR, t48_tchthr);
	}
	return rc;
}

static int mxt_read_touch_flag(struct mxt_data *mxt, uint16_t addr_t37,
		unsigned char page, char *buff, int buff_size)
{
	int rc = 0, i, j;
	int count = 0;
	unsigned char mask;
	struct i2c_client *client = mxt->client;

	rc = mxt_read_block(client, addr_t37, buff_size, buff);

	if (rc) {
		dev_err(&client->dev, "%s() rc:%d\n", __func__, rc);
		goto out;
	}

	if (buff[0] != 0xF3 || buff[1] != page) {
		dev_err(&client->dev, "%s() buff:0x%02X 0x%02X, page:%d\n",
				__func__, buff[0], buff[1], page);
		rc = -EPERM;
		goto out;
	}

	for (i = 2; i < buff_size; i++) {
		for (j = 0; j < 8; j++) {
			mask = 1 << j;
			if (mask & buff[i])
				count++;
		}
	}
	rc = count;

out:
	return rc;
}

static int check_touch_count(struct mxt_data *mxt, int *touch,
		int *anti_touch)
{
	int rc = 0, i;
	uint16_t addr_t6, addr_t37;
	int buff_size = 0;
	unsigned char buff[100] = {0, };

	addr_t6 = MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6);
	addr_t37 = MXT_BASE_ADDR(MXT_DEBUG_DIAGNOSTICS_T37);
	buff_size = 2 + T9_XSIZE * ((T9_YSIZE + 7) / 8);

	for (i = 0; i < 2; i++) {
		unsigned char cmd;
		if (0 == i)
			cmd = 0xF3;
		else
			cmd = 0x01;

		rc = mxt_wait_touch_flag(mxt, addr_t37, addr_t6, cmd);
		if (rc < 0) {
			dev_info(&mxt->client->dev, "1.rc:%d\n", rc);
			goto out;
		}

		rc = mxt_read_touch_flag(mxt, addr_t37,
				(unsigned char)i, buff, buff_size);
		if (rc < 0)
			goto out;

		if (0 == i)
			*touch = rc;
		else
			*anti_touch = rc;
	}

	/* send page up command so we can detect when data updates
	 * next time,
	 * page byte will sit at 1 until we next send F3 command */
	mxt_write_byte(mxt->client, addr_t6 + MXT_ADR_T6_DIAGNOSTICS, 0x01);

	return 0;
out:
	*touch = *anti_touch = 0;
	return rc;
}

static int calibrate_chip(struct mxt_data *mxt)
{
	int ret = 0;
	u8 data = 1u;
	u16 t8_addr;
	struct i2c_client *client = mxt->client;
	struct mxt_runmode_registers_t regs;

	mxt_debug_trace(mxt, "[TSP] %s()\n", __func__);

	palm_info.facesup_message_flag = 0;
	ts_100ms_timer_clear(mxt);
	not_yet_count = 0;
	t8_addr = MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8);

	mxt_reload_registers(mxt, &regs);
	mxt_debug_info(mxt, "ATCH : %02X %02X %02X %02X\n",
			regs.t38_atchcalst, regs.t38_atchcalsthr,
			regs.t38_atchfrccalthr, regs.t38_atchfrccalratio);

	/* 2012_0504
	 *  - the below parameters make calribration repeatly. so, set them 0 */
	ret = mxt_write_byte(client, t8_addr + MXT_ADR_T8_ATCHCALST,
			regs.t38_atchcalst);
	ret |= mxt_write_byte(client, t8_addr + MXT_ADR_T8_ATCHCALSTHR,
			regs.t38_atchcalsthr);
	ret |= mxt_write_byte(client, t8_addr + MXT_ADR_T8_ATCHFRCCALTHR,
			regs.t38_atchfrccalthr);
	ret |= mxt_write_byte(client, t8_addr + MXT_ADR_T8_ATCHFRCCALRATIO,
			regs.t38_atchfrccalratio);
	if (ret < 0) {
		dev_err(&client->dev, "[TSP] failed to set T8_ATCHXX\n");
		return ret;
	}

#if 0 
	ret = mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9)
			+ MXT_ADR_T9_NUMTOUCH, regs.t9_numtouch);
	ret = mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42)
			+ MXT_ADR_T42_MAXNUMTCHS, regs.t42_maxnumtchs);

	/* TSP, Touchscreen threshold */
	ret = mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9)
			+ MXT_ADR_T9_TCHTHR, regs.t9_tchthr);

	/* TSP, TouchKEY threshold */
	/* ret = mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_TOUCH_KEYARRAY_T15)
			+ MXT_ADR_T15_TCHTHR, T15_TCHTHR); */
#endif

	/* restore settings to the local structure so that when we confirm them
	 * cal is good we can correct them in the chip this must be done before
	 * returning send calibration command to the chip change calibration
	 * suspend settings to zero until calibration confirmed good */
	ret = mxt_write_byte(client, MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6)
			+ MXT_ADR_T6_CALIBRATE, data);
	if (ret < 0) {
		dev_err(&client->dev, "[TSP] faild to run CALRIBRATE!\n");
		return ret;
	} else {
		/* set flag for calibration lockup recovery if cal command was
		 * successful set flag to show we must still confirm if
		 * calibration was good or bad */
		mxt->cal_check_flag = 1u;
		/* 2012.08.02
		 * Request debugging log (ATMEL) */
		mxt_debug_info(mxt, "[TSP] Calibration request "
				"by host!\n");
	}

	/* 2012_0510
	 *  - ATMEL : It needs 1ms to settle chip.
	 *  but mxt224 takes 10ms. the below code is work-around of the case.
	 *  if the below code does not make some problem, use it */
	msleep(120);

	return ret;
}

static int check_chip_palm(struct mxt_data *mxt)
{
	int rc = 0;
	int tch_ch, atch_ch;
	struct i2c_client *client = mxt->client;

	rc = check_touch_count(mxt, &tch_ch, &atch_ch);
	mxt_debug_info(mxt, "rc:%d, tch_ch:%d, atch_ch:%d\n",
			rc, tch_ch, atch_ch);
	if (!rc) {
		struct mxt_runmode_registers_t regs;

		mxt_reload_registers(mxt, &regs);

		if ((tch_ch > 0) && (atch_ch > 0)) {
			/* multi-touch or palm */
			palm_info.facesup_message_flag = 1;
		} else if ((tch_ch > 0) && (atch_ch == 0)) {
			/* common case(single touch + small sized palm)
			 * if tch_ch is normal */
			palm_info.facesup_message_flag = 2;
		} else if ((tch_ch == 0) && (atch_ch > 0)) {
			/* abnoraml case */
			/* TODO : does it really need for us?? */
			palm_info.facesup_message_flag = 3;
			dev_warn(&client->dev, "%s() tch_ch:%d, atch_ch:%d\n",
					__func__, tch_ch, atch_ch);
		} else
			palm_info.facesup_message_flag = 4;

#define PALM_CHECK_ENABLE 0x01
#define PALM_CHECK_DEBUG  0x02
		if (regs.t38_palm_check_flag == PALM_CHECK_ENABLE) {
			/* 2012_0510 : ATMEL
			 *  - comment ~ (tch_ch < palm_param[0]) || ~ */
			/* 2012_0517 : ATMEL
			 *  - new condition */
			/* 2012_0523 : ATMEL
			 *  - new condition */
			/* ATMEL : Request new check condition */
			/* if (((tch_ch < regs.t38_palm_param[2])&&
			 * (tch_ch > regs.t38_palm_param[3])) ||
			 * ((tch_ch >= regs.t38_palm_param[0]) &&
			 * ((tch_ch - atch_ch) > regs.t38_palm_param[1]))) */
			if ((tch_ch > regs.t38_palm_param[2]) && (atch_ch == 0))
				palm_info.palm_check_timer_flag = true;
		} else if (regs.t38_palm_check_flag == PALM_CHECK_DEBUG) {
				/* TEST-MODE : always re-calibration.
				 * add the condition to the below code
				 * if any new condition is added */
				palm_info.facesup_message_flag = 2;
				palm_info.palm_check_timer_flag = true;
		} else {
			/* Nothing to do */
			dev_info(&client->dev, "[TSP] %s(): no check palm\n",
					__func__);
		}

		mxt_debug_info(mxt, "[TSP] Touch suppression State:%d/%d\n",
			palm_info.facesup_message_flag,
			palm_info.palm_check_timer_flag ? 1 : 0);
	} else {
		if (rc != -ETIME) {
			/* i2c io error */
			mxt_prerun_reset(mxt, false);
			hw_reset_chip(mxt);
			mxt_postrun_reset(mxt, false);
		} else {
			struct mxt_100ms_timer *p100ms_tmr = &mxt->ts_100ms_tmr;

			if ((p100ms_tmr->timer_ticks >= p100ms_tmr->timer_limit)
					&& (p100ms_tmr->error_limit <=
						MXT_100MS_ERROR_LIMIT)) {
				p100ms_tmr->timer_limit =
					p100ms_tmr->timer_ticks + 1;
				p100ms_tmr->error_limit++;
			}
		}

		return rc;
	}

	return 0;
}

static void check_chip_calibration(struct mxt_data *mxt)
{
	int tch_ch = 0, atch_ch = 0;
	struct i2c_client *client;
	uint8_t finger_cnt = 0;
	int rc = 0;

	mxt_debug_trace(mxt, "[TSP] %s()\n", __func__);

	client = mxt->client;

	rc = check_touch_count(mxt, &tch_ch, &atch_ch);
	if (!rc) {
		int i;
		struct mxt_runmode_registers_t regs;

		for (i = 0 ; i < MXT_MAX_NUM_TOUCHES ; ++i) {
			if (mtouch_info[i].pressure == -1)
				continue;
			finger_cnt++;
		}

		if (mxt->cal_check_flag != 1) {
			mxt_debug_info(mxt, "[TSP] %s() just return!! "
					"finger_cnt = %d\n", __func__,
					finger_cnt);
			return;
		}

		mxt_reload_registers(mxt, &regs);
		mxt_debug_info(mxt, "%s(), tch_ch:%d, atch_ch:%d\n"
				"\tfinger_cnt:%d, CAL_TH:%d, "
				"num_of_antitouch:%d\n", __func__, tch_ch,
				atch_ch, finger_cnt,
				regs.t38_cal_thr, regs.t38_num_of_antitouch);
		/* process counters and decide if we must re-calibrate
		 * or if cal was good */
		if ((tch_ch) && (atch_ch == 0)) {
			/* normal */
			if ((finger_cnt >= 2) && (tch_ch <= 3)) {
				/* tch_ch is the number of node.
				 * if finger_cnt is over 2 and tch_ch is
				 * less than 3,
				 * this means something is wrong. */
				calibrate_chip(mxt);
				/* 100ms timer disable */
				ts_100ms_timer_clear(mxt);
			} else {
				cal_maybe_good(mxt);
				not_yet_count = 0;
			}
		} else if (atch_ch > ((finger_cnt * regs.t38_num_of_antitouch) + 2)) {
			dev_info(&client->dev, "%s()\n FAIL1 tch_ch:%d, "
					"atch_ch:%d, finger_cnt:%d, "
					"num_of_antitouch : %d\n",
					__func__, tch_ch, atch_ch,
					finger_cnt, regs.t38_num_of_antitouch);
			/* atch_ch is too many!! (finger_cnt) */
			calibrate_chip(mxt);
			/* 100ms timer disable */
			ts_100ms_timer_clear(mxt);
		} else if ((tch_ch + regs.t38_cal_thr) <= atch_ch) {
			dev_info(&client->dev, "%s()\n FAIL2 tch_ch:%d, "
					"atch_ch:%d, cal_thr:%d", __func__,
					tch_ch, atch_ch, regs.t38_cal_thr);
			/* atch_ch is too many!! (except finger_cnt) */
			/* cal was bad -
			 *	must recalibrate and check afterwards */
			calibrate_chip(mxt);
			/* 100ms timer disable */
			ts_100ms_timer_clear(mxt);
		} else {
			mxt->cal_check_flag = 1u;
			/* 2012_0517
			 * ATMEL - no need */
#if 0
			if (mxt->ts_100ms_tmr.timer_flag == MXT_DISABLE) {
				/* 100ms timer enable */
				ts_100ms_timer_enable(mxt);
			}
#endif
			not_yet_count++;
			mxt_debug_info(mxt, "[TSP] calibration was not decided"
					" yet, not_yet_count = %d\n",
					not_yet_count);
			if ((tch_ch == 0) && (atch_ch == 0)) {
				not_yet_count = 0;
			} else if (not_yet_count >= 3) {
				/* 2012_0517 : kykim 30 -> 3 */
				dev_info(&client->dev,
						"[TSP] not_yet_count over "
						"3, re-calibrate!!\n");
				not_yet_count = 0;
				calibrate_chip(mxt);
				/* 100ms timer disable */
				ts_100ms_timer_clear(mxt);
			}
		}
	} else {
		if (rc != -ETIME) {
			/* i2c io error */
			mxt_prerun_reset(mxt, false);
			hw_reset_chip(mxt);
			mxt_postrun_reset(mxt, false);
		} else {
			/* FIXME: After meeting, we have to decide what to do */
			calibrate_chip(mxt);
		}
	}
}

static void cal_maybe_good(struct mxt_data *mxt)
{
	int ret;

	mxt_debug_trace(mxt, "[TSP] %s()\n", __func__);

	/* Check if the timer is enabled */
	if (mxt->mxt_time_point != 0) {
		/* Check if the timer timedout of 0.3seconds */
		unsigned long t_next, t_now;
		struct i2c_client *client = mxt->client;

		t_now = jiffies;
		t_next = mxt->mxt_time_point + msecs_to_jiffies(300);
		if (time_after(t_now, t_next)) {
			u16 t8_addr;
			struct mxt_runmode_registers_t regs;

			dev_info(&client->dev, "(%d)good(%d)\n",maybe_good_count
				,jiffies_to_msecs(t_now - mxt->mxt_time_point));
			/* Cal was good - don't need to check any more */
			mxt->mxt_time_point = 0;
			mxt->cal_check_flag = 0;
			/* Disable the timer */
			ts_100ms_timer_clear(mxt);

			/* Write back the normal acquisition config to chip. */
			mxt_reload_registers(mxt, &regs);

			t8_addr = MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8);
			ret = mxt_write_byte(client,
					t8_addr + MXT_ADR_T8_ATCHCALST,
					regs.t8_atchcalst);
			ret = mxt_write_byte(client,
					t8_addr + MXT_ADR_T8_ATCHCALSTHR,
					regs.t8_atchcalsthr);
			ret = mxt_write_byte(client,
					t8_addr + MXT_ADR_T8_ATCHFRCCALTHR,
					regs.t8_atchfrccalthr);
			ret = mxt_write_byte(client,
					t8_addr + MXT_ADR_T8_ATCHFRCCALRATIO,
					regs.t8_atchfrccalratio);

			if (maybe_good_count != 0)
				atomic_set(&cal_counter, 3);

			if (mxt->check_auto_cal_flag == AUTO_CAL_DISABLE)
				_disable_auto_cal(mxt, true);

			maybe_good_count = 1;
#if 0
			ret = mxt_write_byte(mxt->client,
					MXT_BASE_ADDR(
						MXT_TOUCH_MULTITOUCHSCREEN_T9)
					+ MXT_ADR_T9_NUMTOUCH,
					regs.t9_numtouch);
			ret = mxt_write_byte(mxt->client,
					MXT_BASE_ADDR(
						MXT_PROCI_TOUCHSUPPRESSION_T42)
					+ MXT_ADR_T42_MAXNUMTCHS,
					regs.t42_maxnumtchs);

			/* TSP, Touchscreen threshold */
			ret = mxt_write_byte(mxt->client,
					MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9)
					+ MXT_ADR_T9_TCHTHR, regs.t9_tchthr);

			/* TSP, TouchKEY threshold */
			/* ret = mxt_write_byte(mxt->client,
					MXT_BASE_ADDR(MXT_TOUCH_KEYARRAY_T15)
					+ MXT_ADR_T15_TCHTHR, 40); */

			mxt->check_auto_cal = 5;
			mxt_debug_info(mxt, "[TSP] Calibration success!!\n");
#endif

#ifdef MXT_SELF_TEST
			mxt_self_test_timer_stop(mxt);
			mxt_self_test_timer_start(mxt);
#endif
			mxt_check_touch_ic_timer_stop(mxt);
			mxt_check_touch_ic_timer_start(mxt);

#if 0
			if (metal_suppression_chk_flag == true) {
				/* after 20 seconds,
				 * metal coin checking disable */
				cancel_delayed_work(&mxt->timer_dwork);
				schedule_delayed_work(&mxt->timer_dwork, 2000);
			}
#endif
		} else {
			mxt->cal_check_flag = 1u;
		}
	} else {
		mxt->cal_check_flag = 1u;
	}
}

#if TS_100S_TIMER_INTERVAL
/******************************************************************************/
/* 0512 Timer Rework by LBK                            */
/******************************************************************************/
static void ts_100ms_timeout_handler(unsigned long data)
{
	struct mxt_data *mxt = (struct mxt_data *)data;
	mxt->ts_100ms_tmr.p_ts_timeout_tmr = NULL;
	queue_work(ts_100s_tmr_workqueue, &mxt->ts_100ms_tmr.tmr_work);
}

static void ts_100ms_timer_start(struct mxt_data *mxt)
{
	if (mxt->ts_100ms_tmr.p_ts_timeout_tmr != NULL)
		del_timer(mxt->ts_100ms_tmr.p_ts_timeout_tmr);
	mxt->ts_100ms_tmr.p_ts_timeout_tmr = NULL;

	/* 100ms */
	mxt->ts_100ms_tmr.ts_timeout_tmr.expires =
		jiffies + msecs_to_jiffies(100);
	mxt->ts_100ms_tmr.p_ts_timeout_tmr = &mxt->ts_100ms_tmr.ts_timeout_tmr;
	add_timer(&mxt->ts_100ms_tmr.ts_timeout_tmr);
}

static void ts_100ms_timer_stop(struct mxt_data *mxt)
{
	if (mxt->ts_100ms_tmr.p_ts_timeout_tmr)
		del_timer(mxt->ts_100ms_tmr.p_ts_timeout_tmr);
	mxt->ts_100ms_tmr.p_ts_timeout_tmr = NULL;
}

static void ts_100ms_timer_init(struct mxt_data *mxt)
{
	init_timer(&(mxt->ts_100ms_tmr.ts_timeout_tmr));
	mxt->ts_100ms_tmr.ts_timeout_tmr.data = (unsigned long)(mxt);
	mxt->ts_100ms_tmr.ts_timeout_tmr.function = ts_100ms_timeout_handler;
	mxt->ts_100ms_tmr.p_ts_timeout_tmr = NULL;
}

static void ts_100ms_tmr_work(struct work_struct *work)
{
	struct mxt_100ms_timer *p100ms_tmr = NULL;
	struct mxt_data *mxt = container_of(work,
			struct mxt_data, ts_100ms_tmr.tmr_work);

	p100ms_tmr = &mxt->ts_100ms_tmr;
	p100ms_tmr->timer_ticks++;

	mxt_debug_trace(mxt, "[TSP] 100ms T %d\n", p100ms_tmr->timer_ticks);

	mutex_lock(&mxt->msg_lock);
	/* Palm but Not touch message */
	if (palm_info.facesup_message_flag) {
		int rc;
		mxt_debug_info(mxt, "[TSP] facesup_message_flag = %d\n",
				palm_info.facesup_message_flag);
		rc = check_chip_palm(mxt);
		if (rc == -ETIME) {
			/* keep going, 4 is nothing execpt keep going */
			palm_info.facesup_message_flag = 4;
		} else if (rc < 0) {
			/* already reset */
			goto out;
		}
	}

	if ((p100ms_tmr->timer_flag == MXT_ENABLE) &&
			(p100ms_tmr->timer_ticks < p100ms_tmr->timer_limit)) {
		ts_100ms_timer_start(mxt);
		palm_info.palm_check_timer_flag = false;
	} else {
		int cal = 0;
		/* 2012_0523
		 *  - ATMEL : remove "palm_info.facesup_message_flag == 1" */
		if (palm_info.palm_check_timer_flag
				&& (/*(palm_info.facesup_message_flag == 1)
					||*/ (palm_info.facesup_message_flag == 2))
				&& (palm_info.palm_release_flag == false)) {
			dev_info(&mxt->client->dev,
					"[TSP](%s) calibrate_chip\n", __func__);
			calibrate_chip(mxt);
			palm_info.palm_check_timer_flag = false;
			cal = 1;
		}

		/* 2012_0521 :
		 *  - add the below code : there' is no way to recover if
		 *   the upper code don't filter the suppression */
		if (!cal && p100ms_tmr->timer_ticks > 0 &&
			(p100ms_tmr->timer_ticks >= p100ms_tmr->timer_limit)) {
			struct mxt_runmode_registers_t regs;

			mxt_debug_trace(mxt, "go in suppression wait state\n");
			mxt_reload_registers(mxt, &regs);
			if (regs.t38_supp_ops & 0x01) {
				int delay_time;

				mxt_supp_ops_stop(mxt);
				delay_time = (regs.t38_supp_ops & 0xFC) >> 2;
				delay_time *= (regs.t38_supp_ops & 0x02) ? 500 : 250;
				schedule_delayed_work(&mxt->supp_ops.dwork,
						msecs_to_jiffies(delay_time));
			}
		}

		p100ms_tmr->timer_flag = MXT_DISABLE;
		p100ms_tmr->timer_ticks = 0;
		p100ms_tmr->timer_limit = MXT_100MS_TIMER_LIMIT;
		p100ms_tmr->error_limit = 0;
	}
out:
	mutex_unlock(&mxt->msg_lock);
}
#endif

static void ts_100ms_timer_clear(struct mxt_data *mxt)
{
	mxt->ts_100ms_tmr.timer_flag = MXT_DISABLE;
	mxt->ts_100ms_tmr.timer_ticks = 0;
	mxt->ts_100ms_tmr.timer_limit = MXT_100MS_TIMER_LIMIT;
	mxt->ts_100ms_tmr.error_limit = 0;
#if TS_100S_TIMER_INTERVAL
	ts_100ms_timer_stop(mxt);
#endif
}

static void ts_100ms_timer_enable(struct mxt_data *mxt)
{
	mxt->ts_100ms_tmr.timer_flag = MXT_ENABLE;
	mxt->ts_100ms_tmr.timer_ticks = 0;
	mxt->ts_100ms_tmr.timer_limit = MXT_100MS_TIMER_LIMIT;
	mxt->ts_100ms_tmr.error_limit = 0;
#if TS_100S_TIMER_INTERVAL
	ts_100ms_timer_start(mxt);
#endif
}

/******************************************************************************/
/* Initialization of driver                                                   */
/******************************************************************************/

static int mxt_initialize(struct i2c_client *client, struct mxt_data *mxt)
{
	int error = 0;

	if (!client || !mxt) {
		dev_info(&client->dev, "client:%p, mxt:%p\n",
				client, mxt);
		return -EINVAL;
	}

	error = mxt_identify(client, mxt);

	if (error < 0) {
#ifdef MXT_FIRMUP_ENABLE
		/* change to i2c slave address for boot mode
		 * this is for checking the upgrade failure.
		 * If module is shutdowned during upgrade.
		 *  module's i2c address may has boot-mode slave address
		 * */
		/* change to boot-mode i2c slave address */
		mxt_change_i2c_addr(mxt, true);
		error = mxt_identify(client, mxt);
		if (-EIO == error) {
			dev_err(&client->dev, "[TSP] ATMEL Chip could not "
					"be identified. error = %d\n", error);
			goto mxt_init_out;
		} else {
			/* back to the application mode
			 * It has tow possibilities, one is another atmel device
			 * , the other is the upgrade-failure */
			mxt_change_i2c_addr(mxt, false);
		}

		return MXT_INIT_FIRM_ERR;
#else
		dev_err(&client->dev, "[TSP] ATMEL Chip could not "
				"be identified. error = %d\n", error);
		goto mxt_init_out;
#endif
	}

	error = mxt_read_object_table(client, mxt);
	if (error < 0)
		dev_err(&client->dev, "[TSP] ATMEL Chip could not read"
				" object table. error = %d\n", error);

mxt_init_out:
	return error;
}

static int mxt_identify(struct i2c_client *client, struct mxt_data *mxt)
{
	u8 buf[7];
	int error;
	int identified;
	int retry_count = I2C_RETRY_COUNT;
	int val;

	identified = 0;

retry_i2c:
	/* Read Device info to check if chip is valid */
	error = mxt_read_block(client, MXT_ADDR_INFO_BLOCK, 7, (u8 *)buf);

	if (error < 0) {
		mxt->read_fail_counter++;
		if (retry_count--) {
			dev_info(&client->dev, "[TSP] Warning: "
					"To wake up touch-ic in "
					"deep sleep, retry i2c "
					"communication!");
			msleep(30);  /* delay 30ms */
			goto retry_i2c;
		}
		dev_err(&client->dev, "[TSP] Failure accessing "
				"maXTouch device\n");
		return -EIO;
	}

/*
	printk("[%s] buff : %x \n",__func__, buf[0]);
	printk("[%s] buff : %x \n",__func__, buf[1]);
	printk("[%s] buff : %x \n",__func__, buf[2]);
	printk("[%s] buff : %x \n",__func__, buf[3]);
	printk("[%s] buff : %x \n",__func__, buf[4]);
	printk("[%s] buff : %x \n",__func__, buf[5]);
	printk("[%s] buff : %x \n",__func__, buf[6]);
*/

	mxt->device_info.family_id  = buf[0];
	mxt->device_info.variant_id = buf[1];
	mxt->device_info.major	    = ((buf[2] >> 4) & 0x0F);
	mxt->device_info.minor      = (buf[2] & 0x0F);
	mxt->device_info.build	    = buf[3];
	mxt->device_info.x_size	    = buf[4];
	mxt->device_info.y_size	    = buf[5];
	mxt->device_info.num_objs   = buf[6];
	mxt->device_info.num_nodes =
		mxt->device_info.x_size * mxt->device_info.y_size;

	//valky
	#if 0
	printk("[%s] buff: %x family_id: 0x%x\n",__func__, buf[0], mxt->device_info.family_id);
	printk("[%s] buff: %x variant_id: 0x%x\n",__func__, buf[1], mxt->device_info.variant_id);
	printk("[%s] buff: %x major: %d minor: %d\n",__func__, buf[2], mxt->device_info.major, mxt->device_info.minor);
	printk("[%s] buff: %x build: %d\n",__func__, buf[3], mxt->device_info.build);
	printk("[%s] buff: %x x_size: %d\n",__func__, buf[4], mxt->device_info.x_size);
	printk("[%s] buff: %x y_size: %d\n",__func__, buf[5], mxt->device_info.y_size);
	printk("[%s] buff: %x num_objs: %d\n",__func__, buf[6], mxt->device_info.num_objs);
	#endif

	if (mxt->device_info.family_id == MAXTOUCH_FAMILYID) {
		strcpy(mxt->device_info.family, maxtouch_family);
	} else {
		dev_err(&client->dev,
			"[TSP] maXTouch Family ID [0x%x] not supported\n",
			mxt->device_info.family_id);
		identified = -ENXIO;
	}

	val = mxt->device_info.major * 10 + mxt->device_info.minor;

	if (
		//((val <= 10) &&
		//(mxt->device_info.variant_id == MXT540_FIRM_VER1_VARIANTID)) ||
		((val >= 21) &&
		(mxt->device_info.variant_id == MXT540_FIRM_VER2_VARIANTID))) {
		sprintf(mxt->device_info.variant, "%s[%02X]", mxt540_variant,
				mxt->device_info.variant_id);
	} else {
		/*
		dev_err(&client->dev,
			"[TSP] maXTouch Variant ID [0x%x] not supported\n",
			mxt->device_info.variant_id);
		identified = -ENXIO;
		*/
	}

	mxt_debug_msg(mxt,
		"[TSP] Atmel %s.%s Firmware version [%d.%d] Build [%d]\n",
		mxt->device_info.family,
		mxt->device_info.variant,
		mxt->device_info.major,
		mxt->device_info.minor,
		mxt->device_info.build);
	mxt_debug_msg(mxt,
		"[TSP] Atmel %s.%s Configuration [X: %d] x [Y: %d]\n",
		mxt->device_info.family,
		mxt->device_info.variant,
		mxt->device_info.x_size,
		mxt->device_info.y_size);
	mxt_debug_msg(mxt, "[TSP] number of objects: %d\n",
			mxt->device_info.num_objs);

	return identified;
	//return 1;
}

/*
* Reads the object table from maXTouch chip to get object data like
* address, size, report id.
*/
static int mxt_read_object_table(struct i2c_client *client,
				 struct mxt_data *mxt)
{
	u16	report_id_count;
	u8	buf[MXT_OBJECT_TABLE_ELEMENT_SIZE];
	u8	object_type;
	u16	object_address;
	u16	object_size;
	u8	object_instances;
	u8	object_report_ids;
	u16	object_info_address;
#ifdef MXT_ENABLE_RUN_CRC_CHECK
	u32	crc;
	u32     crc_calculated;
#endif
	int	i;
	int	error;

	u8	object_instance;
	u8	object_report_id;
	u8	report_id;
	int     first_report_id;

	struct mxt_object *object_table;

	mxt_debug_trace(mxt, "[TSP] maXTouch driver get configuration\n");

	kfree(mxt->object_table);
	mxt->object_table = kzalloc(sizeof(struct mxt_object) *
		mxt->device_info.num_objs,
		GFP_KERNEL);
	if (mxt->object_table == NULL) {
		dev_err(&mxt->client->dev,
				"[TSP] maXTouch: Memory allocation failed!\n");
		return -ENOMEM;
	}
	object_table = mxt->object_table;

	object_info_address = MXT_ADDR_OBJECT_TABLE;

	report_id_count = 0;

	for (i = 0; i < mxt->device_info.num_objs; i++) {
		mxt_debug_trace(mxt, "[TSP] <%d>Reading maXTouch at [0x%04x]: ",
			i, object_info_address);

		error = mxt_read_block(client, object_info_address,
				MXT_OBJECT_TABLE_ELEMENT_SIZE, (u8 *)buf);

		if (error < 0) {
			mxt->read_fail_counter++;
			dev_err(&client->dev,
					"[TSP] maXTouch Object %d "
					"could not be read\n", i);
			return -EIO;
		}

		object_type		=  buf[0];
		object_address		= (buf[2] << 8) + buf[1];
		object_size		=  buf[3] + 1;
		object_instances	=  buf[4] + 1;
		object_report_ids	=  buf[5];

		//valky
		#if 0
		printk("%s object[%d] type: %d address: %d size: %d instances: %d report_ids: %d\n"
			, __func__, (i+1), object_type, object_address, object_size, 
			object_instances, object_report_ids);
		#endif

		mxt_debug_trace(mxt, "[TSP] Type=%03d, Address=0x%04x, "
			"Size=0x%02x, %d instances, %d report id's\n",
			object_type,
			object_address,
			object_size,
			object_instances,
			object_report_ids);

		if (object_type > MXT_MAX_OBJECT_TYPES) {
			/* Unknown object type */
			dev_err(&client->dev,
					"[TSP] maXTouch object type [%d] "
					"not recognized\n", object_type);
			return -ENXIO;
		}

		/* Save frequently needed info. */
		if (object_type == MXT_GEN_MESSAGEPROCESSOR_T5) {
			mxt->msg_proc_addr = object_address;
			mxt->message_size = object_size;
		}

		object_table[i].type            = object_type;
		object_table[i].chip_addr       = object_address;
		object_table[i].size            = object_size;
		object_table[i].instances       = object_instances;
		object_table[i].num_report_ids  = object_report_ids;
		report_id_count += object_instances * object_report_ids;
		mxt->object_link[object_type] = i;

		object_info_address += MXT_OBJECT_TABLE_ELEMENT_SIZE;
	}

	kfree(mxt->rid_map);
	/* allocate for report_id 0, even if not used */
	mxt->rid_map = kzalloc(sizeof(struct report_id_map) * \
			(report_id_count + 1), GFP_KERNEL);
	if (mxt->rid_map == NULL) {
		pr_warning("[TSP] maXTouch: Can't allocate memory!\n");
		return -ENOMEM;
	}

	kfree(mxt->last_message);
	mxt->last_message = kzalloc(mxt->message_size, GFP_KERNEL);
	if (mxt->last_message == NULL) {
		pr_warning("[TSP] maXTouch: Can't allocate memory!\n");
		return -ENOMEM;
	}

	mxt->report_id_count = report_id_count;
	if (report_id_count > 254) {	/* 0 & 255 are reserved */
		dev_err(&client->dev,
			"[TSP] Too many maXTouch report id's [%d]\n",
			report_id_count);
		return -ENXIO;
	}

	/* Create a mapping from report id to object type */
	report_id = 1; /* Start from 1, 0 is reserved. */

	/* Create table associating report id's with objects & instances */
	for (i = 0; i < mxt->device_info.num_objs; i++) {
		for (object_instance = 0;
				object_instance < object_table[i].instances;
				object_instance++) {
			first_report_id = report_id;
			for (object_report_id = 0;
					object_report_id <
					object_table[i].num_report_ids;
					object_report_id++) {
				mxt->rid_map[report_id].object =
					object_table[i].type;
				mxt->rid_map[report_id].instance =
					object_instance;
				mxt->rid_map[report_id].first_rid =
					first_report_id;
				report_id++;
			}
		}
	}

#ifdef MXT_ENABLE_RUN_CRC_CHECK
	/* Read 3 byte CRC */
	error = mxt_read_block(client, object_info_address, MXT_CRC_NUM, buf);
	if (error < 0) {
		mxt->read_fail_counter++;
		dev_err(&client->dev, "[TSP] Error reading CRC\n");
		return -EIO;
	}

	crc = (buf[2] << 16) | (buf[1] << 8) | buf[0];

	calculate_infoblock_crc(&crc_calculated, mxt);

	mxt_debug_trace(mxt, "[TSP] Reported info block CRC = 0x%6X\n\n", crc);
	mxt_debug_trace(mxt, "[TSP] Calculated info block CRC = 0x%6X\n\n",
				crc_calculated);

	if (crc == crc_calculated)
		mxt->info_block_crc = crc;
	else {
		mxt->info_block_crc = 0;
		dev_err(&mxt->client->dev,
				"[TSP] maXTouch: info block CRC invalid!\n");
	}
#endif

	if (debug >= DEBUG_MESSAGES) {
		dev_info(&client->dev, "[TSP] maXTouch: %d Objects\n",
			mxt->device_info.num_objs);

		for (i = 0; i < mxt->device_info.num_objs; i++) {
			dev_info(&client->dev, "[TSP] Type:\t\t\t[%d]: %s\n",
				object_table[i].type,
				object_type_name[object_table[i].type]);
			dev_info(&client->dev, "\tAddress:\t0x%04X\n",
				object_table[i].chip_addr);
			dev_info(&client->dev, "\tSize:\t\t%d Bytes\n",
				object_table[i].size);
			dev_info(&client->dev, "\tInstances:\t%d\n",
				object_table[i].instances);
			dev_info(&client->dev, "\tReport Id's:\t%d\n",
				object_table[i].num_report_ids);
		}
	}

	return 0;
}

#define MXT_ADR_T38_CFG_CONF_POS	MXT_ADR_T38_USER5
#define MXT_ADR_T38_CFG_CONF_SIZE	20
#define MXT_CONFIG_STRING		"XX.XXX.XX."
#define MXT_CONFIG_STR_LENG		10 /* XX.XXX.XX. is 10 */

static int mxt_get_version(struct mxt_data *mxt, struct clein_touch_version *ver)
{
	int ret = 0;
	int retry = I2C_RETRY_COUNT, version = 0;
	struct i2c_client *client = mxt->client;

	/* firmware version */
	version = mxt->device_info.major * 10;
	version += mxt->device_info.minor;
	sprintf(ver->firm, "XX.XXX.XX.XXXXXX.%03d", version);

	/* configuration version */
	/* see T38_USERDATA5 of device_config_540.h */
	do {
		ret = mxt_read_block(client, MXT_BASE_ADDR(MXT_USER_INFO_T38)
				+ MXT_ADR_T38_CFG_CONF_POS,
				MXT_ADR_T38_CFG_CONF_SIZE, ver->conf);
		if (!ret)
			break;
	} while (retry--);

	ver->conf[MXT_ADR_T38_CFG_CONF_SIZE] = '\0';
	if (ret < 0) {
		dev_err(&client->dev, "%s() ret=%d\n", __func__, ret);
		sprintf(ver->conf, "XX.XXX.XX.XXXXXX.FAIL");
	}

	return ret;
}

static int mxt_convert_version(const char *buff)
{
	int version = 0;

	if (!strncmp(buff, ""MXT_CONFIG_STRING"", MXT_CONFIG_STR_LENG)) {
		char *end = NULL;
		version = simple_strtoul(buff + MXT_CONFIG_STR_LENG, &end, 10);
		version *= 100;
		version += simple_strtoul(end + 1, &end, 10);
	} else
		pr_info("%s() invalid buff:%s\n", __func__, buff);

	return version;
}

static int mxt_get_conf_version(struct mxt_data *mxt)
{
	int version = 1;
	char buff[32] = {0, };

	if (T38_USERDATA5 != 'X') {
		dev_err(&mxt->client->dev, "T38_USERDATA5 : 0x%02X\n",
				T38_USERDATA5);
		return version;
	}

	buff[0] = T38_USERDATA5;
	buff[1] = T38_USERDATA6;
	buff[2] = T38_USERDATA7;
	buff[3] = T38_USERDATA8;
	buff[4] = T38_USERDATA9;
	buff[5] = T38_USERDATA10;
	buff[6] = T38_USERDATA11;
	buff[7] = T38_USERDATA12;
	buff[8] = T38_USERDATA13;
	buff[9] = T38_USERDATA14;
	buff[10] = T38_USERDATA15;
	buff[11] = T38_USERDATA16;
	buff[12] = T38_USERDATA17;
	buff[13] = T38_USERDATA18;
	buff[14] = T38_USERDATA19;
	buff[15] = T38_USERDATA20;
	buff[16] = T38_USERDATA21;
	buff[17] = T38_USERDATA22;
	buff[18] = T38_USERDATA23;
	buff[19] = T38_USERDATA24;

	return mxt_convert_version(buff);
}

static void mxt_update_backup_nvram(
		struct mxt_data *mxt, struct clein_touch_version *old_ver)
{
	int error;
	int old_version = 0, new_version = 0;
	struct clein_touch_version new_ver;

	if (old_ver->conf[17] == 'F' &&
			old_ver->conf[18] == 'A') {
		/* failed to read the configuration version */
		return;
	}

	/* '\0' means that this is the first time.
	 *  so, write the configuration to the NVRAM */
	if (old_ver->conf[0] == '\0') {
		mxt_debug_info(mxt, "[TSP] nvram is clear\n");
		backup_to_nv(mxt);
		return;
	}

	mxt_debug_trace(mxt, "[TSP] prev conf-version:%s\n", old_ver->conf);

	/* get the current version from T38 */
	error = mxt_get_version(mxt, &new_ver);
	if (!error) {
		char *end = NULL;

		/* old version */
		if (!strncmp(old_ver->conf,
					""MXT_CONFIG_STRING"",
					MXT_CONFIG_STR_LENG)) {
			old_version = simple_strtoul(
					old_ver->conf + MXT_CONFIG_STR_LENG,
					&end, 10);
			old_version *= 100;
			old_version += simple_strtoul(end + 1, &end, 10);
		}
		/* new version */
		if (!strncmp(new_ver.conf, ""MXT_CONFIG_STRING"",
					MXT_CONFIG_STR_LENG)) {
			new_version = simple_strtoul(
					new_ver.conf + MXT_CONFIG_STR_LENG,
					&end, 10);
			new_version *= 100;
			new_version += simple_strtoul(end + 1, &end, 10);
		}

		mxt_debug_trace(mxt,
				"[TSP] new conf-version:%s\n", new_ver.conf);

		if (new_version > old_version) {
			/* if new_version is higher than old_version,
			 * write the configuration to the NVRAM */
			mxt_debug_info(mxt, "backup to nvram\n");
			backup_to_nv(mxt);
		}
	}
}

static int mxt_write_init_registers(struct mxt_data *mxt)
{
	//valky
	printk("%s\n", __func__);
	int error;
	u8 runmode = MXT_CFG_OVERWRITE;
	int drv_version = 0, nvm_version = 0;
	struct clein_touch_version nvram_ver;
	struct i2c_client *client = mxt->client;

	error = mxt_get_runmode(mxt);
	if (error < 0)
		return error;
	runmode = (u8)error;

	/* get the current version from T38 registers */
	error = mxt_get_version(mxt, &nvram_ver);
	if (error < 0) {
		dev_err(&client->dev, "failt to read version\n");
		return error;
	}

	/* driver's configuration version */
	drv_version = mxt_get_conf_version(mxt);
	/* nvram's configuration version */
	if (nvram_ver.conf[0] != 'X')
		nvm_version = 0;
	else
		nvm_version = mxt_convert_version(nvram_ver.conf);

	mxt_debug_info(mxt, "nvram:%d, driver:%d\n",
			nvm_version, drv_version);
	if (drv_version > nvm_version &&
			(runmode == MXT_CFG_OVERWRITE)) {
		//valky add
		mxt_clear_nvram(mxt);
		backup_to_nv(mxt);
		msleep(400);

		error = mxt_config_settings(mxt);
		if (error < 0) {
			//valky
			printk("%s mxt_config_settings error\n", __func__);
			dev_err(&client->dev, "[TSP] (%s):"
					"failed to update config\n", __func__);
			return error;
		}
		backup_to_nv(mxt);
		//valky 
		msleep(100);
		hw_reset_chip(mxt);
		msleep(100);
	}

	/* read some registers for calibration */
	mxt_config_save_runmode(mxt);

	/* debug level */
	error = mxt_read_byte(client, MXT_BASE_ADDR(MXT_USER_INFO_T38) +
			MXT_ADR_T38_USER25, &runmode);
	if (!error && ((runmode & 0x0F) != 0))
		debug = runmode & 0x0F;

	calibrate_chip(mxt);

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mxt_early_suspend(struct early_suspend *h)
{
	struct	mxt_data *mxt = container_of(h, struct mxt_data, early_suspend);

#ifndef MXT_SLEEP_POWEROFF
	u8 cmd_sleep[2] = { 0};
	u16 addr;
#endif
	u8 i;

	mxt_debug_info("[TSP] mxt_early_suspend has been called!");
#if defined(MXT_FACTORY_TEST) || defined(MXT_FIRMUP_ENABLE)
	/*start firmware updating : not yet finished*/
	while (MXT_FIRM_UPGRADING(mxt->firm_status_data)) {
		mxt_debug_info("[TSP] mxt firmware is Downloading : "
				"mxt suspend must be delayed!");
		msleep(1000);
	}
#endif
	//valky - below code don't open !!!
	//cancel_delayed_work(&mxt->config_dwork);
	disable_irq(mxt->client->irq);
#if TS_100S_TIMER_INTERVAL
	ts_100ms_timer_stop(mxt);
#endif
#ifdef MXT_SELF_TEST
	mxt_self_test_timer_stop(mxt);
#endif
	mxt_check_touch_ic_timer_stop(mxt);
	mxt_supp_ops_stop(mxt);
	mxt_release_all_fingers(mxt);
	mxt_release_all_keys(mxt);

	/* global variable initialize */
	mxt->ts_100ms_tmr.timer_flag = MXT_DISABLE;
	mxt->ts_100ms_tmr.timer_ticks = 0;
	mxt->mxt_time_point = 0;
#ifdef MXT_SLEEP_POWEROFF
	if (mxt->pdata->suspend_platform_hw != NULL)
		mxt->pdata->suspend_platform_hw(mxt->pdata);
#else
		/*
		* a setting of zeros to IDLEACQINT and ACTVACQINT
		* forces the chip set to enter Deep Sleep mode.
	*/
	addr = get_object_address(MXT_GEN_POWERCONFIG_T7,
			0, mxt->object_table, mxt->device_info.num_objs,
			mxt->object_link);
	mxt_debug_info("[TSP] addr: 0x%02x, buf[0]=0x%x, buf[1]=0x%x",
				addr, cmd_sleep[0], cmd_sleep[1]);
	mxt_write_block(mxt->client, addr, 2, (u8 *)cmd_sleep);
#endif
}

static void mxt_late_resume(struct early_suspend *h)
{
	struct	mxt_data *mxt = container_of(h, struct mxt_data, early_suspend);

	mxt_debug_info("[TSP] mxt_late_resumehas been called!");

#ifdef MXT_SLEEP_POWEROFF
	if (mxt->pdata->resume_platform_hw != NULL)
		mxt->pdata->resume_platform_hw(mxt->pdata);
#else
	for (cnt = 10; cnt > 0; cnt--) {
		if (mxt_power_config(mxt) < 0)
			continue;
		if (sw_reset_chip(mxt, RESET_TO_NORMAL) == 0)  /* soft reset */
			break;
	}
	if (cnt == 0) {
		pr_err("[TSP] mxt_late_resume failed!!!");
		return;
	}
#endif

	msleep(MXT_SW_RESET_TIME);  /*typical value is 50ms*/

#if 1 /* MXT_SLEEP_POWEROFF */
	calibrate_chip(mxt);
#endif
#if 0 /* def ENABLE_CHARGER */
	if (mxt->set_mode_for_ta && !work_pending(&mxt->ta_work))
		schedule_work(&mxt->ta_work);
#endif
	/* metal_suppression_chk_flag = true; */
	/* mxt->mxt_status = true; */
	enable_irq(mxt->client->irq);
}
#endif

static int mxt540_fops_open(struct inode *inode, struct file *file)
{
	file->private_data = &g_mxt;
	return 0;
}

static int mxt540_fops_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations mxt540_misc_fops = {
	.open = mxt540_fops_open,
	.release = mxt540_fops_release,
};

static struct miscdevice mxt540_misc = {
	.name = "mxt_misc",
	.fops = &mxt540_misc_fops,
	.minor = MISC_DYNAMIC_MINOR,

	.mode = 0660,
};

static int __devinit mxt_probe(struct i2c_client *client,
			       const struct i2c_device_id *id)
{
	struct mxt_data          *mxt;
	struct mxt_platform_data *pdata;
	struct input_dev         *input;
	int error;
	int i;

	dev_info(&client->dev,
			"[TSP] maXTouch driver(%s), addr:%04x, irq:%d\n",
			client->name, client->addr, client->irq);

	unsigned RESET_GPIO = TCC_GPA(23);
	#if defined(CONFIG_DAUDIO) && defined(CONFIG_TOUCHSCREEN_ATMEL_MXT_336S_AT)
		RESET_GPIO = HW_RESET_GPIO;
	#endif
	gpio_request(RESET_GPIO, NULL);
	gpio_direction_output(RESET_GPIO, 1);
	
	msleep(103);
	printk("[%s] reset done !!!\n", __func__ );

	/* Allocate structure - we need it to identify device */
	mxt = &g_mxt;
	mxt->last_read_addr = -1;
	input = input_allocate_device();
	if (!mxt || !input) {
		dev_err(&client->dev, "[TSP] insufficient memory\n");
		error = -ENOMEM;
		goto err_after_kmalloc;
	}

	/* Initialize Platform data */
	pdata = client->dev.platform_data;
	if (pdata == NULL) {
		dev_err(&client->dev, "[TSP] platform data is required!\n");
		return -EINVAL;
	}
	mxt->pdata = pdata;

	mxt->message_counter   = 0;
	mxt->client = client;
	mxt->input  = input;
	/* Default-configuration for auto-cal is 'enable' */
	mxt->check_auto_cal_flag = AUTO_CAL_ENABLE;

	if (pdata->init_platform_hw)
		pdata->init_platform_hw(mxt);

	/* mxt540E regulator config */
#if 0 /* def ENABLE_CONTROL_REGULATOR */
	mxt->pdata->reg_vdd = regulator_get(NULL, mxt->pdata->reg_vdd_name);

	if (IS_ERR(mxt->pdata->reg_vdd)) {
		error = PTR_ERR(mxt->pdata->reg_vdd);
		pr_err("[TSP] [%s: %s]unable to get regulator %s: %d\n",
			__func__,
			mxt->pdata->platform_name,
			mxt->pdata->reg_vdd_name,
			error);
	}

	mxt->pdata->reg_avdd = regulator_get(NULL, mxt->pdata->reg_avdd_name);

	if (IS_ERR(mxt->pdata->reg_avdd)) {
		error = PTR_ERR(mxt->pdata->reg_avdd);
		pr_err("[TSP] [%s: %s]unable to get regulator %s: %d\n",
			__func__,
			mxt->pdata->platform_name,
			mxt->pdata->reg_avdd_name,
			error);
	}

	mxt->pdata->reg_vdd_lvsio =
		regulator_get(NULL, mxt->pdata->reg_vdd_lvsio_name);

	if (IS_ERR(mxt->pdata->reg_vdd_lvsio)) {
		error = PTR_ERR(mxt->pdata->reg_vdd_lvsio);
		pr_err("[TSP] [%s: %s]unable to get regulator %s: %d\n",
			__func__,
			mxt->pdata->platform_name,
			mxt->pdata->reg_vdd_lvsio_name,
			error);
	}

	/* TSP Power on */
	error = regulator_enable(mxt->pdata->reg_vdd);
	regulator_set_voltage(mxt->pdata->reg_vdd,
		mxt->pdata->reg_vdd_level,
		mxt->pdata->reg_vdd_level);

	error = regulator_enable(mxt->pdata->reg_avdd);
	regulator_set_voltage(mxt->pdata->reg_avdd,
		mxt->pdata->reg_avdd_level,
		mxt->pdata->reg_avdd_level);

	error = regulator_enable(mxt->pdata->reg_vdd_lvsio);
	regulator_set_voltage(mxt->pdata->reg_vdd_lvsio,
		mxt->pdata->reg_vdd_lvsio_level,
		mxt->pdata->reg_vdd_lvsio_level);
	msleep(250);
#endif

	i2c_set_clientdata(client, mxt);
	MXT_FIRM_CLEAR(mxt->firm_status_data);
	error = mxt_initialize(client, mxt);
	if (error < 0)
		goto err_after_get_regulator;
#ifdef MXT_FIRMUP_ENABLE
	else if (error == MXT_INIT_FIRM_ERR) {
		dev_info(&client->dev, "[TSP] invalid device or "
				"firmware crash!\n");
		mxt->firm_status_data = MXT_FIRM_STATUS_FAIL;
	}
#endif

#if 0/* ndef MXT_THREADED_IRQ */
	INIT_DELAYED_WORK(&mxt->dwork, mxt_worker);
#endif
#if 0/* def ENABLE_CHARGER */
	INIT_WORK(&mxt->ta_work, mxt_ta_worker);
#endif

#if defined(MXT_FACTORY_TEST)/* def MXT_FACTORY_TEST */
	INIT_DELAYED_WORK(&mxt->firmup_dwork, set_mxt_update_exe);
#endif

#if 0/* def ENABLE_CHARGER */
	/* Register callbacks */
	/* To inform tsp , charger connection status*/
	mxt->callbacks.inform_charger = mxt_inform_charger_connection;
	if (mxt->pdata->register_cb)
		mxt->pdata->register_cb(&mxt->callbacks);
#endif

	init_waitqueue_head(&mxt->msg_queue);
	sema_init(&mxt->msg_sem, 1);
#ifndef MXT_THREADED_IRQ
	spin_lock_init(&mxt->lock);
#endif
	mutex_init(&mxt->reg_update.lock);
	mutex_init(&mxt->msg_lock);

	snprintf(mxt->phys_name,
			sizeof(mxt->phys_name),
			"%s/input0",
			dev_name(&client->dev));

	input->name = "mxt540_ts";
	input->phys = mxt->phys_name;
	input->id.bustype = BUS_I2C;
	input->dev.parent = &client->dev;

	if (debug >= DEBUG_INFO) {
		dev_info(&client->dev,
				"[TSP] maXTouch name: \"%s\"\n", input->name);
		dev_info(&client->dev,
				"[TSP] maXTouch phys: \"%s\"\n", input->phys);
		dev_info(&client->dev, "[TSP] maXTouch driver "
				"setting abs parameters\n");
	}
	//__set_bit(BTN_TOUCH, input->keybit);
	//__set_bit(EV_ABS, input->evbit);
	/* single touch */

	input->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	input->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE);
#if 0
	input_set_abs_params(input, ABS_X, 0, mxt->pdata->max_x, 0, 0);
	input_set_abs_params(input, ABS_Y, 0, mxt->pdata->max_y, 0, 0);
	input_set_abs_params(input, ABS_PRESSURE, 0,
			MXT_MAX_REPORTED_PRESSURE, 0, 0);
	input_set_abs_params(input, ABS_TOOL_WIDTH, 0,
			MXT_MAX_REPORTED_WIDTH, 0, 0);
#endif

	/* multi touch */
	input_set_abs_params(input,
			ABS_MT_POSITION_X, 0, mxt->pdata->max_x, 0, 0);
	input_set_abs_params(input,
			ABS_MT_POSITION_Y, 0, mxt->pdata->max_y, 0, 0);
	input_set_abs_params(input,
			ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input,
			ABS_MT_TRACKING_ID, 0, MXT_MAX_NUM_TOUCHES - 1, 0, 0);
	input_set_abs_params(input,
			ABS_MT_WIDTH_MAJOR, 0, 30, 0, 0);
	//__set_bit(EV_SYN, input->evbit);
	//__set_bit(EV_KEY, input->evbit);

	input_set_drvdata(input, mxt);

	error = input_register_device(mxt->input);
	if (error < 0) {
		dev_err(&client->dev,
			"[TSP] Failed to register input device\n");
		goto err_after_get_regulator;
	}

	//valky
	int vvvv = MXT_FIRM_STABLE(mxt->firm_status_data);
	printk("%s MXT_FIRM_STABLE(mxt->firm_status_data): %d\n", __func__, vvvv);
	if (MXT_FIRM_STABLE(mxt->firm_status_data)
			&& mxt_write_init_registers(mxt) < 0)
		goto err_after_input_register;

	/*
	error = mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9) + MXT_ADR_T9_YRANGE, 0x1f);
	if (error < 0)
		pr_err("[TSP] %s, %d Error!!\n", __func__, __LINE__);
	error = mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9) + MXT_ADR_T9_YRANGE + 1, 0x03);
	if (error < 0)
		pr_err("[TSP] %s, %d Error!!\n", __func__, __LINE__);

	error = mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9) + MXT_ADR_T9_XRANGE, 0xdf);
	if (error < 0)
		pr_err("[TSP] %s, %d Error!!\n", __func__, __LINE__);

	error = mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9) + MXT_ADR_T9_XRANGE + 1, 0x01);
	if (error < 0)
		pr_err("[TSP] %s, %d Error!!\n", __func__, __LINE__);
	error = mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9) + MXT_ADR_T9_ORIENT, 5);
	if (error < 0)
		pr_err("[TSP] %s, %d Error!!\n", __func__, __LINE__);
*/

	/* _SUPPORT_MULTITOUCH_ */
	for (i = 0; i < MXT_MAX_NUM_TOUCHES ; i++)
		mtouch_info[i].pressure = -1;

	error = misc_register(&mxt540_misc);
	if (error < 0) {
		dev_err(&client->dev, "[TSP] failed to register"
				" misc driver(%d)\n", error);
		goto err_after_input_register;
	}

#if 0/* ndef MXT_THREADED_IRQ */
	/* Schedule a worker routine to read any messages that might have
	 * been sent before interrupts were enabled. */
	cancel_delayed_work(&mxt->dwork);
	schedule_delayed_work(&mxt->dwork, 0);
#endif

	/* Allocate the interrupt */
	mxt->irq = client->irq;
	mxt->irq_counter = 0;
	if (mxt->irq) {
		/* Try to request IRQ with falling edge first. This is
		 * not always supported. If it fails, try with any edge. */
#ifdef MXT_THREADED_IRQ
		error = request_threaded_irq(mxt->irq,
			NULL,
			mxt_threaded_irq,
			IRQF_TRIGGER_LOW | IRQF_ONESHOT,
			client->dev.driver->name,
			mxt);
		if (error < 0) {
			error = request_threaded_irq(mxt->irq,
				NULL,
				mxt_threaded_irq,
				IRQF_DISABLED,
				client->dev.driver->name,
				mxt);
		}
#else
		mxt->valid_irq_counter = 0;
		mxt->invalid_irq_counter = 0;
		error = request_irq(mxt->irq,
			mxt_irq_handler,
			IRQF_TRIGGER_FALLING,
			client->dev.driver->name,
			mxt);
		if (error < 0) {
			error = request_irq(mxt->irq,
				mxt_irq_handler,
				0,
				client->dev.driver->name,
				mxt);
		}
#endif
		if (error < 0) {
			dev_err(&client->dev,
					"[TSP] failed to allocate irq %d\n",
					mxt->irq);
			goto err_after_misc_register;
		}
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	mxt->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	mxt->early_suspend.suspend = mxt_early_suspend;
	mxt->early_suspend.resume = mxt_late_resume;
	register_early_suspend(&mxt->early_suspend);
#endif	/* CONFIG_HAS_EARLYSUSPEND */

	error = sysfs_create_group(&client->dev.kobj, &maxtouch_attr_group);
	if (error) {
		pr_err("[TSP] fail sysfs_create_group\n");
#ifdef CONFIG_HAS_EARLYSUSPEND
		goto err_after_earlysuspend;
#else
		goto err_after_interrupt_register;
#endif
	}

#if defined(MXT_FACTORY_TEST)/* def MXT_FACTORY_TEST */
	tsp_factory_mode =
		device_create(sec_class, NULL, 0, mxt, "sec_touchscreen");
	if (IS_ERR(tsp_factory_mode)) {
		pr_err("[TSP] Failed to create device!\n");
		error = -ENODEV;
		goto err_after_attr_group;
	}

	error = sysfs_create_group(&tsp_factory_mode->kobj,
			&maxtouch_factory_attr_group);
	if (error)
		goto err_after_attr_group;
#endif

#if TS_100S_TIMER_INTERVAL
	INIT_WORK(&mxt->ts_100ms_tmr.tmr_work, ts_100ms_tmr_work);

	ts_100s_tmr_workqueue =
		create_singlethread_workqueue("ts_100_tmr_workqueue");
	if (!ts_100s_tmr_workqueue) {
		pr_err("unabled to create touch tmr work queue\n");
		error = -1;
		goto err_after_attr_group;
	}
	ts_100ms_timer_init(mxt);
#endif
	INIT_DELAYED_WORK(&mxt->reg_update.dwork, reg_update_dwork);

#ifdef MXT_SELF_TEST
	INIT_DELAYED_WORK(&mxt->self_test.dwork, mxt_self_test);
	INIT_DELAYED_WORK(&mxt->self_test.check_dwork, mxt_self_test_check);
	mxt_self_test_timer_init(mxt);
#endif
	INIT_DELAYED_WORK(&mxt->supp_ops.dwork, mxt_supp_ops_dwork);
	INIT_DELAYED_WORK(&mxt->cal_timer.dwork, mxt_cal_timer_dwork);
	INIT_DELAYED_WORK(&mxt->check_touch_ic_timer.dwork,
			mxt_check_touch_ic_timer_dwork);

	wake_lock_init(&mxt->wakelock, WAKE_LOCK_SUSPEND, "touch");
	/* if (MXT_FIRM_STABLE(mxt->firm_status_data))
		mxt->mxt_status = true; */

	return 0;

err_after_attr_group:
	sysfs_remove_group(&client->dev.kobj, &maxtouch_attr_group);

#ifdef CONFIG_HAS_EARLYSUSPEND
err_after_earlysuspend:
	unregister_early_suspend(&mxt->early_suspend);
#endif

err_after_interrupt_register:
	if (mxt->irq)
		free_irq(mxt->irq, mxt);

err_after_misc_register:
	misc_deregister(&mxt540_misc);

err_after_input_register:
	input_free_device(input);

err_after_get_regulator:
#if 0/* def ENABLE_CONTROL_REGULATOR */
	regulator_disable(mxt->pdata->reg_vdd);
	regulator_disable(mxt->pdata->reg_avdd);
	regulator_disable(mxt->pdata->reg_vdd_lvsio);

	regulator_put(mxt->pdata->reg_vdd);
	regulator_put(mxt->pdata->reg_avdd);
	regulator_put(mxt->pdata->reg_vdd_lvsio);
#endif
err_after_kmalloc:
	if (mxt != NULL) {
		kfree(mxt->rid_map);
		kfree(mxt->object_table);
		kfree(mxt->last_message);

		/* if (mxt->pdata->exit_platform_hw != NULL) */
		/*	mxt->pdata->exit_platform_hw(); */
	}

	return error;
}

static int __devexit mxt_remove(struct i2c_client *client)
{
	struct mxt_data *mxt;

	mxt = i2c_get_clientdata(client);
#ifdef CONFIG_HAS_EARLYSUSPEND
	wake_lock_destroy(&mxt->wakelock);
	unregister_early_suspend(&mxt->early_suspend);
#endif	/* CONFIG_HAS_EARLYSUSPEND */
	/* Close down sysfs entries */
	sysfs_remove_group(&client->dev.kobj, &maxtouch_attr_group);

	/* Release IRQ so no queue will be scheduled */
	if (mxt->irq)
		free_irq(mxt->irq, mxt);
#if 0/* ndef MXT_THREADED_IRQ */
	cancel_delayed_work_sync(&mxt->dwork);
#endif

	/* deregister misc device */
	misc_deregister(&mxt540_misc);

	input_unregister_device(mxt->input);

	if (mxt != NULL) {
		kfree(mxt->rid_map);
		kfree(mxt->object_table);
		kfree(mxt->last_message);
	}

	i2c_set_clientdata(client, NULL);

	return 0;
}

static int mxt_resume_reset(struct i2c_client *client)
{
	int error;
	struct mxt_data *mxt = i2c_get_clientdata(client);

	dev_dbg(&client->dev, "resume multi-touch\n");

	disable_irq(mxt->client->irq);

	hw_reset_chip(mxt);

	/* init variables */
	mxt->read_fail_counter = 0;
	mxt->message_counter = 0;
	mxt->last_read_addr = -1;
#ifndef MXT_THREADED_IRQ
	mxt->valid_irq_counter = 0;
	mxt->invalid_irq_counter = 0;
#endif
	/* metal_suppression_chk_flag = false; */

	/* Default-configuration for auto-cal is 'enable' */
	mxt->check_auto_cal_flag = AUTO_CAL_ENABLE;
	atomic_set(&cal_counter, 0);
	maybe_good_count = 0;

	MXT_FIRM_CLEAR(mxt->firm_status_data);
	error = mxt_initialize(client, mxt);
	if (error < 0)
		goto err_resume;
#ifdef MXT_FIRMUP_ENABLE
	else if (error == MXT_INIT_FIRM_ERR) {
		dev_info(&client->dev,
				"[TSP] invalid device or firmware crash!\n");
		mxt->firm_status_data = MXT_FIRM_STATUS_FAIL;
	}
#endif

#ifndef MXT_TUNNING_ENABLE
	if (MXT_FIRM_STABLE(mxt->firm_status_data)
			&& mxt_write_init_registers(mxt) < 0) {
		dev_err(&client->dev, "[TSP] failed to initialize "
				"registers in resume\n");
		goto err_resume;
	}
#endif /* MXT_TUNNING_ENABLE */
	if (MXT_FIRM_STABLE(mxt->firm_status_data))
		enable_irq(mxt->client->irq);
	return 0;

err_resume:
	kfree(mxt->rid_map);
	kfree(mxt->object_table);
	kfree(mxt->last_message);
	return 0;

}

#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
static int mxt_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct mxt_data *mxt = i2c_get_clientdata(client);

#if TS_100S_TIMER_INTERVAL
	ts_100ms_timer_stop(mxt);
#endif
#ifdef MXT_SELF_TEST
	mxt_self_test_timer_stop(mxt);
#endif
	mxt_check_touch_ic_timer_stop(mxt);
	mxt_supp_ops_stop(mxt);

	while (MXT_FIRM_UPGRADING(mxt->firm_status_data)) {
		dev_info(&client->dev, "[TSP] mxt firmware is Downloading : "
				"mxt suspend must be delayed!");
		msleep(1000);
	}

	if (device_may_wakeup(&client->dev))
		enable_irq_wake(mxt->irq);

	return 0;
}

static int mxt_resume(struct i2c_client *client)
{
	struct mxt_data *mxt = i2c_get_clientdata(client);

	if (device_may_wakeup(&client->dev))
		disable_irq_wake(mxt->irq);

	mxt_resume_reset(client);
	return 0;
}
#else
#define mxt_suspend NULL

#if 1	//valky
#define mxt_resume	NULL
#else
static int mxt_resume(struct i2c_client *client)
{
	mxt_resume_reset(client);
	return 0;
}
#endif//valky
#endif

static const struct i2c_device_id mxt_idtable[] = {
	{"mxt540_ts", 0,},
	//{"mxt336s_at", 0,},
	{ }
};

MODULE_DEVICE_TABLE(i2c, mxt_idtable);

static struct i2c_driver mxt_driver = {
	.driver = {
		.name	= "mxt540_ts",
		//.name	= "mxt336s_at",
		.owner  = THIS_MODULE,
	},

	.id_table	= mxt_idtable,
	.probe		= mxt_probe,
	.remove		= __devexit_p(mxt_remove),
	.suspend	= mxt_suspend,
	.resume		= mxt_resume,
};

static int __init mxt_init(void)
{
	int err;
	err = i2c_add_driver(&mxt_driver);

	if (err)
		//pr_err("Adding mXT540E driver failed (errno = %d)\n", err);
		pr_err("Adding mXT336S-AT driver failed (errno = %d)\n", err);

	return err;
}
module_init(mxt_init);

static void __exit mxt_cleanup(void)
{
	i2c_del_driver(&mxt_driver);
}
module_exit(mxt_cleanup);

//MODULE_DESCRIPTION("Driver for Atmel mXT540E Touchscreen Controller");
MODULE_DESCRIPTION("Driver for Atmel mXT336S-AT Touchscreen Controller");
MODULE_AUTHOR("Cleinsoft");
MODULE_LICENSE("GPL");
