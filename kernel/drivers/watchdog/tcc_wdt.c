/*
 * drivers/watchdog/tcc_wdt.c
 *
 * Author:  <leesh@telechips.com>
 * Created: June, 2011
 * Description: to use the watchdog for Telechips chipset
 *
 * Copyright (C) Telechips, Inc.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You should have received a copy of the  GNU General Public License along
 * with this program; if not, write  to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*===========================================================================

                         INCLUDE FILES FOR MODULE

===========================================================================*/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cpufreq.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <mach/bsp.h>
#include <mach/pms_table.h>


/*===========================================================================

                  DEFINITIONS AND DECLARATIONS FOR MODULE

===========================================================================*/

//#define KERNEL_FILE_INTERFACE_USED
//#define KERNEL_TIMER_USED
//#define MEASURE_PERFORMANCE

static DEFINE_SPINLOCK(wdt_lock);

static int debug;
static int wdt_pass=0;

module_param(debug, int, 0);

MODULE_PARM_DESC(debug, "Watchdog debug, set to >1 for debug (default 0)");

#define DBG(msg...) do { \
	if (debug) \
		printk(KERN_INFO msg); \
	} while (0)

static PPMU pPMU;
#ifndef KERNEL_TIMER_USED
static PTIMER pTMR;
#endif

#ifndef KERNEL_FILE_INTERFACE_USED
#ifdef KERNEL_TIMER_USED
static struct timer_list tccwdt_timer;
#endif
#endif

#define TCCWDT_RESET_TIME       30 // sec unit
#define TCCWDT_KICK_TIME_DIV    4
#define TCCWDT_CNT_UNIT			((XIN_CLK_RATE*100)/65536)

static int tccwdt_reset_time = TCCWDT_RESET_TIME;
static int watchdog_status = 0;

static int watchdog_status_pm = 0;

#ifdef CONFIG_QUICKBOOT_WATCHDOG
static int qb_wdt_flag = 0;
#endif

static void tccwdt_kickdog(void);


/*===========================================================================

                             Watchdog control

===========================================================================*/

#ifndef KERNEL_FILE_INTERFACE_USED
#ifdef KERNEL_TIMER_USED
/*===========================================================================
FUNCTION
===========================================================================*/
static void tccwdt_set_timer(struct timer_list *timer)
{
	DBG("%s\n", __func__);
	timer->expires = jiffies + msecs_to_jiffies((tccwdt_reset_time/TCCWDT_KICK_TIME_DIV)*1000);
	add_timer(timer);
}

/*===========================================================================
FUNCTION
===========================================================================*/
static void tccwdt_timer_cbf(unsigned long data)
{
	DBG("%s\n", __func__);
	tccwdt_kickdog();
	tccwdt_set_timer(&tccwdt_timer);
}
#else
/*===========================================================================
FUNCTION
===========================================================================*/
static irqreturn_t tccwdt_timer_handler(int irq, void *dev_id)
{
	DBG("%s\n", __func__);

#ifdef MEASURE_PERFORMANCE
	if((*(volatile unsigned long *)0xF03080C0) & Hw21)
		*(volatile unsigned long *)0xF03080C0 &= ~Hw21;
	else
		*(volatile unsigned long *)0xF03080C0 |= Hw21;
#endif

	tccwdt_kickdog();
	pTMR->TIREQ.nREG = 0x00002020;
	return IRQ_HANDLED;	
}
#endif
#endif

/*===========================================================================
FUNCTION
===========================================================================*/

void tccwdt_PowerDown(void)
{
	spin_lock(&wdt_lock);

	pPMU = (PPMU)tcc_p2v(HwPMU_BASE);
#ifndef KERNEL_TIMER_USED
	pTMR = (PCKC)tcc_p2v(HwTMR_BASE);
#endif

#ifdef CONFIG_TCC_WATCHDOG
#ifndef KERNEL_TIMER_USED
	pTMR->TCFG5.nREG = 0x00000000;
	pTMR->TIREQ.nREG = 0x00002020;
#endif
#endif
    BITCSET(pPMU->PMU_WDTCTRL.nREG, 0xFFFF, 5*TCCWDT_CNT_UNIT);
	BITSET(pPMU->PMU_WDTCTRL.nREG, Hw31);
	wdt_pass = 1;
	spin_unlock(&wdt_lock);
}

/*===========================================================================
FUNCTION
===========================================================================*/
static void tccwdt_start(void)
{
	spin_lock(&wdt_lock);

	BITCLR(pPMU->PMU_WDTCTRL.nREG, Hw31); // disable watchdog
	BITCSET(pPMU->PMU_WDTCTRL.nREG, 0xFFFF, tccwdt_reset_time*TCCWDT_CNT_UNIT); // disable watchdog
	BITSET(pPMU->PMU_WDTCTRL.nREG, Hw31); // enable watchdog
#ifndef KERNEL_FILE_INTERFACE_USED
#ifdef KERNEL_TIMER_USED
	tccwdt_set_timer(&tccwdt_timer); // start kick timer
#else
	{
		PCKC pCKC = (PCKC)tcc_p2v(HwCKC_BASE);

		pCKC->PCLKCTRL00.nREG = 0x24000000;
		pTMR->TCFG5.nREG = 0x00000059;
		pTMR->TREF5.nREG = (0x00002DC7*(tccwdt_reset_time/TCCWDT_KICK_TIME_DIV)); //0xFFFF : 5592ms, 0x2DC7 : 1s
		pTMR->TIREQ.nREG = 0x00002020;
	}
#endif
#endif
	DBG("%s : watchdog_time=%dsec, kick_time=%dsec\n", __func__, tccwdt_reset_time, tccwdt_reset_time/TCCWDT_KICK_TIME_DIV);

	watchdog_status_pm = 1;

	spin_unlock(&wdt_lock);
}

/*===========================================================================
FUNCTION
===========================================================================*/
static void tccwdt_stop(void)
{
	spin_lock(&wdt_lock);

#ifndef KERNEL_FILE_INTERFACE_USED
#ifdef KERNEL_TIMER_USED
	del_timer(&tccwdt_timer); // stop kick timer
#else
	pTMR->TCFG5.nREG = 0x00000000;
	pTMR->TCNT5.nREG = 0x00000000;	// Clear Timer Counter to change Timer setting.
	pTMR->TIREQ.nREG = 0x00002020;
#endif
#endif
	if(!(wdt_pass == 1))
		BITCLR(pPMU->PMU_WDTCTRL.nREG, Hw31); // disable watchdog
	wdt_pass = 0;
	DBG("%s\n", __func__);
 
	watchdog_status_pm = 0;

	spin_unlock(&wdt_lock);
}

/*===========================================================================
FUNCTION
===========================================================================*/
static int tccwdt_get_status(void)
{
	int stat;

	DBG("%s\n", __func__);

	spin_lock(&wdt_lock);
#if defined(CONFIG_CHIP_TCC8935S) || defined(CONFIG_CHIP_TCC8933S) || defined(CONFIG_CHIP_TCC8937S)
	stat = ISSET(pPMU->PMU_WDTCTRL.nREG, Hw31);
#else
	stat = ISSET(pPMU->PMU_WDTCTRL.nREG, Hw24);
#endif
	spin_unlock(&wdt_lock);

	return stat;
}

/*===========================================================================
FUNCTION
===========================================================================*/
static void tccwdt_kickdog(void)
{
	//spin_lock(&wdt_lock);
#if defined(CONFIG_CHIP_TCC8935S) || defined(CONFIG_CHIP_TCC8933S) || defined(CONFIG_CHIP_TCC8937S)
	BITSET(pPMU->PMU_WDTCTRL.nREG, Hw30); // Clear watchdog : if write 1, restart watchdog counter.
	BITCLR(pPMU->PMU_WDTCTRL.nREG, Hw30); // Clear watchdog : if write 1, restart watchdog counter.
#else
	BITSET(pPMU->PMU_WDTCTRL.nREG, Hw30); // Clear watchdog : if write 1, restart watchdog counter.
	BITSET(pPMU->PMU_WDTCTRL.nREG, Hw31); // Eanble watchdog
#endif
	//spin_unlock(&wdt_lock);
}


#ifdef KERNEL_FILE_INTERFACE_USED
/*===========================================================================

                            Kernel Interface

===========================================================================*/

/*===========================================================================
FUNCTION
===========================================================================*/
static int tccwdt_open(struct inode *inode, struct file *file)
{
	tccwdt_start();
	return nonseekable_open(inode, file);
}

/*===========================================================================
FUNCTION
===========================================================================*/
static int tccwdt_release(struct inode *inode, struct file *file)
{
	tccwdt_stop();
	return 0;
}

/*===========================================================================
FUNCTION
===========================================================================*/
static ssize_t tccwdt_write(struct file *file, const char __user *data,
				size_t len, loff_t *ppos)
{
	tccwdt_kickdog();
	return len;
}

/*===========================================================================
FUNCTION
===========================================================================*/
static long tccwdt_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int __user *p = argp;

	switch (cmd) {
	case WDIOC_KEEPALIVE:
		tccwdt_kickdog();
		return 0;
	case WDIOC_SETTIMEOUT:
		if (get_user(tccwdt_reset_time, p))
			return -EFAULT;
		tccwdt_stop();
		tccwdt_start();
		return put_user(tccwdt_reset_time, p);
	case WDIOC_GETTIMEOUT:
		return put_user(tccwdt_reset_time, p);
	default:
		return -ENOTTY;
	}
}

/*===========================================================================
VARIABLE
===========================================================================*/
static const struct file_operations tccwdt_fops = {
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.write		= tccwdt_write,
	.unlocked_ioctl	= tccwdt_ioctl,
	.open		= tccwdt_open,
	.release	= tccwdt_release,
};

static struct miscdevice tccwdt_miscdev = {
	.minor		= WATCHDOG_MINOR,
	.name		= "watchdog",
	.fops		= &tccwdt_fops,
};
#endif



/*===========================================================================

                         Platform Device Interface

===========================================================================*/

/*===========================================================================
FUNCTION
===========================================================================*/
static int __devinit tccwdt_probe(struct platform_device *pdev)
{
	int ret;
//88 5.0 board -> GPS_PWDN , J10D1 15 pin
#ifdef MEASURE_PERFORMANCE
	volatile PGPION	pGPIO	= (volatile PGPION)tcc_p2v(HwGPIOD_BASE);
#endif

	DBG("%s: probe=%p\n", __func__, pdev);

	pPMU = (PPMU)tcc_p2v(HwPMU_BASE);
#ifndef KERNEL_TIMER_USED
	pTMR = (PCKC)tcc_p2v(HwTMR_BASE);
#endif

#ifdef MEASURE_PERFORMANCE
	pGPIO->GPEN |= Hw21;
//	pGPIO->GPDAT &= ~Hw21;
#endif

#ifdef KERNEL_FILE_INTERFACE_USED
	ret = misc_register(&tccwdt_miscdev);
	if (ret) {
		printk("cannot register miscdev on minor=%d (%d)\n", WATCHDOG_MINOR, ret);
		return -1;
	}
#else
#ifdef KERNEL_TIMER_USED
	// Initialize Kernel s/w timer
	init_timer(&tccwdt_timer);
	tccwdt_timer.data = 0;
	tccwdt_timer.function = tccwdt_timer_cbf;
#else
	request_irq(INT_TC_TI5, tccwdt_timer_handler, IRQ_TYPE_LEVEL_HIGH|IRQF_SHARED, "TCC-WDT", &pdev->dev);
#endif
#ifdef CONFIG_TCC_WATCHDOG
	tccwdt_start();
#endif
#endif

	return 0;
}

/*===========================================================================
FUNCTION
===========================================================================*/
static int __devexit tccwdt_remove(struct platform_device *dev)
{
	DBG("%s: remove=%p\n", __func__, dev);

	tccwdt_stop();

	return 0;
}

/*===========================================================================
FUNCTION
===========================================================================*/
static void tccwdt_shutdown(struct platform_device *dev)
{
	DBG("%s: shutdown=%p\n", __func__, dev);

	tccwdt_stop();
}

#ifdef CONFIG_PM
/*===========================================================================
FUNCTION
===========================================================================*/
static int tccwdt_suspend(struct platform_device *dev, pm_message_t state)
{
	DBG("%s: suspend=%p\n", __func__, dev);
#ifdef CONFIG_QUICKBOOT_WATCHDOG
	// watchdog_status : TCC WatchDog, qb_wdt_flag : QB WatchDog
	if (!qb_wdt_flag) {
//		watchdog_status = tccwdt_get_status();
		watchdog_status = watchdog_status_pm;
		tccwdt_stop();
	}
#else
//	watchdog_status = tccwdt_get_status();
	watchdog_status = watchdog_status_pm;
	tccwdt_stop();
#endif
	return 0;
}

/*===========================================================================
FUNCTION
===========================================================================*/
static int tccwdt_resume(struct platform_device *dev)
{
	DBG("%s: resume=%p\n", __func__, dev);
#ifdef CONFIG_QUICKBOOT_WATCHDOG
	// watchdog_status : TCC WatchDog, qb_wdt_flag : QB WatchDog
	if(watchdog_status && !qb_wdt_flag)	// For Sleep Mode
		tccwdt_start();

	//  qb_wdt_flag : QB WatchDog
	if(qb_wdt_flag) {
		BITCLR(pPMU->PMU_WDTCTRL.nREG, Hw31); // disable watchdog
		BITCSET(pPMU->PMU_WDTCTRL.nREG, 0xFFFF, tccwdt_reset_time*TCCWDT_CNT_UNIT);
		// tccwdt_reset_time = TCCWDT_RESET_TIME_QB.
		tccwdt_kickdog();
	}
#else
	if(watchdog_status)
		tccwdt_start();
#endif

	return 0;
}
#else
#define tccwdt_suspend NULL
#define tccwdt_resume  NULL
#endif

/*===========================================================================
VARIABLE
===========================================================================*/
static struct platform_driver tccwdt_driver = {
	.probe		= tccwdt_probe,
	.remove		= __devexit_p(tccwdt_remove),
	.shutdown	= tccwdt_shutdown,
	.suspend	= tccwdt_suspend,
	.resume		= tccwdt_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "tcc-wdt",
	},
};

static char banner[] __initdata = KERN_INFO "TCC Watchdog Timer\n";

/*===========================================================================
FUNCTION
===========================================================================*/
static int __init watchdog_init(void)
{
	printk(banner);
	return platform_driver_register(&tccwdt_driver);
}

/*===========================================================================
FUNCTION
===========================================================================*/
static void __exit watchdog_exit(void)
{
	tccwdt_stop();
	platform_driver_unregister(&tccwdt_driver);
}

#ifdef CONFIG_QUICKBOOT_WATCHDOG
/*===========================================================================

                             QuickBoot Watchdog

===========================================================================*/

#define TCCWDT_RESET_TIME_QB       5 // sec unit

void tccwdt_qb_kickdog(void);

void tccwdt_qb_init(void)
{

#ifndef CONFIG_TCC_WATCHDOG
	/*		Initialize watchdog if TCC_WATCHDOG is not set. because probe was not called.		*/

	//88 5.0 board -> GPS_PWDN , J10D1 15 pin
#ifdef MEASURE_PERFORMANCE
	volatile PGPION	pGPIO	= (volatile PGPION)tcc_p2v(HwGPIOD_BASE);
#endif

	pPMU = (PPMU)tcc_p2v(HwPMU_BASE);
#ifndef KERNEL_TIMER_USED
	pTMR = (PCKC)tcc_p2v(HwTMR_BASE);
#endif

#ifdef MEASURE_PERFORMANCE
	pGPIO->GPEN |= Hw21;
//	pGPIO->GPDAT &= ~Hw21;
#endif

#endif


	watchdog_status = watchdog_status_pm;	// SAVE WatchDog STATUS
	tccwdt_kickdog();
	tccwdt_stop();	//	To Disable TCC WatchDog

	tccwdt_reset_time = TCCWDT_RESET_TIME_QB;	// SET QuickBoot WatchDog Time.

	BITCSET(pPMU->PMU_WDTCTRL.nREG, 0xFFFF, tccwdt_reset_time*TCCWDT_CNT_UNIT);//SET QB WATCHDOG
	BITSET(pPMU->PMU_WDTCTRL.nREG, Hw31); // Enable QuickBoot WatchDog

	qb_wdt_flag = 1;
	printk(KERN_INFO "Enable QuickBoot WatchDog. TimeOut : %dsec\n", tccwdt_reset_time);

	tccwdt_qb_kickdog();	// Kick QuickBoot WatchDog
}

void tccwdt_qb_exit(void)
{
	printk(KERN_INFO "*** Disable QuickBoot WatchDog ***\n");
	tccwdt_stop();	// Disable QuickBoot WatchDog

	tccwdt_reset_time = TCCWDT_RESET_TIME;	// SET TCC WatchDog Time.

	if(watchdog_status) {
		tccwdt_start();	// Enable TCC WatchDog
		printk(KERN_INFO "Enable TCC WatchDog. TimeOut : %dsec\n", tccwdt_reset_time);
	}

	qb_wdt_flag = 0;
}


void tccwdt_qb_kickdog(void)
{
	tccwdt_kickdog();
}
#else
void tccwdt_qb_init(void) { };
void tccwdt_qb_exit(void) { };
void tccwdt_qb_kickdog(void) { };
#endif
EXPORT_SYMBOL(tccwdt_qb_init);
EXPORT_SYMBOL(tccwdt_qb_exit);
EXPORT_SYMBOL(tccwdt_qb_kickdog);


module_init(watchdog_init);
module_exit(watchdog_exit);

MODULE_AUTHOR("Bruce <leesh@telechips.com>");
MODULE_DESCRIPTION("TCC Watchdog Device Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
MODULE_ALIAS("platform:tcc-wdt");

