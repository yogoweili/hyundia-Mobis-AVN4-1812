/* 
 * linux/drivers/i2c/busses/tcc93xx/i2c-tcc.c
 *
 * Author:  <linux@telechips.com>
 * Created: 10th Jun, 2008 
 * Description: Telechips I2C Controller
 *
 * Copyright (c) Telechips, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * ----------------------------
 * for TCC DIBCOM DVB-T module 
 * [M] i2c-core.c, i2c-dev.c
 * [M] include/linux/i2c-dev.h, include/linux/i2c.h
 *
*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <asm/io.h>

#include <mach/bsp.h>
#include <mach/tca_ckc.h>
#include <mach/tca_i2c.h>
//#include "tca_i2c.h"

#ifdef CONFIG_HIBERNATION
extern unsigned int do_hibernation;
#endif

#define NACK_STOP_SIGNAL 1
enum tcc_i2c_state {
	STATE_IDLE,
	STATE_START,
	STATE_READ,
	STATE_WRITE,
	STATE_STOP
};

struct tcc_i2c {
	spinlock_t                      lock;
	wait_queue_head_t               wait;
	volatile struct tcc_i2c_regs    *regs;
    volatile unsigned long          IRQSTR;

	int                 core;
    int                 ch;
    int                 smu_i2c_flag;
    unsigned int        i2c_clk_rate;
	unsigned int        core_clk_rate;
    char                core_clk_name[10];
	struct clk          *pclk;
	struct i2c_msg      *msg;
	unsigned int        msg_num;
	unsigned int        msg_idx;
	unsigned int        msg_ptr;

	enum tcc_i2c_state  state;
	struct device       *dev;
	struct i2c_adapter  adap;
#if defined(CONFIG_I2C_DEBUG_BUS_SYSFS)
	unsigned int sent;
	unsigned int send_fail;
	unsigned int recv;
	unsigned int recv_fail;
#endif
};


#if defined(CONFIG_I2C_DEBUG_BUS_SYSFS)
enum
{
	I2C_SENT, I2C_RECV, I2C_SEND_FAIL, I2C_RECV_FAIL
};

//i2c->sent += (UINT_MAX-i2c->msg->len >= i2c->sent)?i2c->msg->len:0;

/**
 * clear_all_rec_idx - clear all record index
 * @i2c: tcc_i2c
 */
static void clear_all_rec_idx(struct tcc_i2c *i2c)
{
	i2c->sent = i2c->send_fail = i2c->recv = i2c->recv_fail = 0;
}

/**
 * incr_sent_bytes- increse i2c sent byte
 * @i2c: tcc_i2c
 * It returns number of bytes sent to now.
 * If sent byte exceed the UINT_MAX, it will return 0.
 */
static unsigned int incr_sent_bytes(struct tcc_i2c *i2c)
{
	if(i2c->sent + i2c->msg->len >= UINT_MAX)
		return 0;

	return i2c->sent += i2c->msg->len;
}

/**
 * incr_send_fail_bytes - increse i2c send fail byte
 * @i2c: tcc_i2c
 * It returns number of bytes send failed to now.
 * If sent byte exceed the UINT_MAX, it will return 0.
 */
static unsigned int incr_send_fail_bytes(struct tcc_i2c *i2c)
{
	if(i2c->send_fail + i2c->msg->len >= UINT_MAX)
		return 0;

	return i2c->send_fail += i2c->msg->len;
}

/**
 * incr_send_fail_bytes - increse i2c received byte
 * @i2c: tcc_i2c
 * It returns number of bytes received to now.
 * If sent byte exceed the UINT_MAX, it will return 0.
 */
static unsigned int incr_recv_bytes(struct tcc_i2c *i2c)
{
	if(i2c->recv + i2c->msg->len >= UINT_MAX)
		return 0;

	return i2c->recv += i2c->msg->len;
}

/**
 * incr_recv_fail_bytes - increse i2c receive fail byte
 * @i2c: tcc_i2c
 * It returns number of bytes receive failed to now.
 * If sent byte exceed the UINT_MAX, it will return 0.
 */
static unsigned int incr_recv_fail_bytes(struct tcc_i2c *i2c)
{
	if(i2c->recv_fail + i2c->msg->len >= UINT_MAX)
		return 0;

	return i2c->recv_fail += i2c->msg->len;
}

/**
 * incr_trans_bytes - count send (fail) or receive (fail) bytes 
 * @i2c: tcc_i2c
 * @type: transfer type 
 */
static int incr_trans_bytes(struct tcc_i2c *i2c, int type)
{
	unsigned int ret = 0;
	switch(type){
		case I2C_SENT:
			ret = incr_sent_bytes(i2c);
			break;
		case I2C_RECV:
			ret = incr_recv_bytes(i2c);
			break;
		case I2C_SEND_FAIL:
			ret = incr_send_fail_bytes(i2c);
			break;
		case I2C_RECV_FAIL:
			ret = incr_recv_fail_bytes(i2c);
			break;
		default:
			return -EINVAL;
	}
	
	if(!ret){
		clear_all_rec_idx(i2c);
		return -EOVERFLOW;
	}

	return 0;
}
#endif

static int tcc_i2c_in_suspend = 0;

/* tcc_i2c_message_start
 *
 * put the start of a message onto the bus 
*/
static void tcc_i2c_message_start(struct tcc_i2c *i2c, struct i2c_msg *msg)
{
	unsigned int addr = (msg->addr & 0x7f) << 1;

	if (msg->flags & I2C_M_RD)
		addr |= 1;
	if (msg->flags & I2C_M_REV_DIR_ADDR)
		addr ^= 1;

	i2c->regs->TXR = addr;
	i2c->regs->CMD = Hw7 | Hw4;
}

static int wait_intr(struct tcc_i2c *i2c)
{
	unsigned long cnt = 0;

	if (!i2c->smu_i2c_flag) {
#if defined (CONFIG_ARCH_TCC893X)
		while (!(tcc_readl(i2c->IRQSTR) & (1<<i2c->core))) {
#else
		while (!(tcc_readl(i2c->IRQSTR) & (i2c->ch?Hw1:Hw0))) {
#endif
			cnt++;
			if (cnt > 100000) {
				printk("i2c-tcc: time out!  core%d ch%d\n", i2c->core, i2c->ch);
				return -1;
			}
		}
	} else {
		/* SMU_I2C wait */
		while (1) {
			cnt = i2c->regs->SR;
			if (!(cnt & Hw1)) break;
		}
		for (cnt = 0; cnt <15; cnt++);
	}

	return 0;
}
static int wait_ack_for_IPOD(struct tcc_i2c *i2c)
{
	unsigned long cnt = 0;
	if (!i2c->smu_i2c_flag) {
#if defined  (CONFIG_ARCH_TCC893X)
		while (!(tcc_readl(i2c->IRQSTR) & (1<<i2c->core))) {
#else
		while (!(tcc_readl(i2c->IRQSTR) & (i2c->ch?Hw1:Hw0))) {
#endif
			cnt++;
			if (cnt > 100000) {
				printk("i2c-tcc: time out!  core%d ch%d\n", i2c->core, i2c->ch);
				return -1;
			}
		}
		//printk("cnt1: %d\n", (int)cnt);
		for(cnt=0; cnt < 100000; cnt++){
			if(!(i2c->regs->SR & Hw7)){
				//printk("i2c-tcc: NACK!!! core%d ch%d\n", i2c->core, i2c->ch);
				//printk("cnt2: %d\n", (int)cnt);
				return 0;
				break;
			}
		}
		return -1;
	} else {
		/* SMU_I2C wait */
		while (1) {
			cnt = i2c->regs->SR;
			if (!(cnt & Hw1)) break;
		}
		for (cnt = 0; cnt <15; cnt++);
	}

	return 0;
}
static int recv_i2c(struct tcc_i2c *i2c)
{
	int ret, i;
	dev_dbg(&i2c->adap.dev, "READ [%x][%d]\n", i2c->msg->addr, i2c->msg->len);
	tcc_i2c_message_start(i2c, i2c->msg);

	if(i2c->msg->flags & I2C_M_TCC_IPOD){
		ret = wait_ack_for_IPOD(i2c);
	}else{
		ret = wait_intr(i2c);
	}
	if (ret != 0){
#if NACK_STOP_SIGNAL
	//	printk("NACK!! Stop signal enable!! %s - %d\n", __func__, __LINE__);
		i2c->regs->CMD = Hw6;
		ret = wait_intr(i2c);
		BITSET(i2c->regs->CMD, Hw0);
#endif
		return ret;
	}
	BITSET(i2c->regs->CMD, Hw0);	/* Clear a pending interrupt */

	for (i = 0; i < i2c->msg->len; i++) {
        if (i2c->msg->flags & I2C_M_WM899x_CODEC) { /* B090183 */
            if (i == 0)
                i2c->regs->CMD = Hw5;
            else
                i2c->regs->CMD = Hw5 | Hw3;
        } else {
    		if (i == (i2c->msg->len - 1)) 
    			i2c->regs->CMD = Hw5 | Hw3;
    		else
    			i2c->regs->CMD = Hw5;
        }

		ret = wait_intr(i2c);
		if (ret != 0)
			return ret;
		BITSET(i2c->regs->CMD, Hw0);

		i2c->msg->buf[i] = (unsigned char) i2c->regs->RXR;
	}

	i2c->regs->CMD = Hw6;

	ret = wait_intr(i2c);
	if (ret != 0)
		return ret;
	BITSET(i2c->regs->CMD, Hw0);
#if defined(CONFIG_I2C_DEBUG_BUS_SYSFS)
	incr_recv_bytes(i2c);
#endif

	return 1;
}

static int send_i2c(struct tcc_i2c *i2c)
{
	int ret, i, no_stop = 0;
	dev_dbg(&i2c->adap.dev, "SEND [%x][%d]", i2c->msg->addr, i2c->msg->len);

	tcc_i2c_message_start(i2c, i2c->msg);

	if(i2c->msg->flags & I2C_M_TCC_IPOD){
		ret = wait_ack_for_IPOD(i2c);
	}else{
		ret = wait_intr(i2c);
	}
	if (ret != 0){
#if NACK_STOP_SIGNAL
		//printk("NACK!! Stop signal enable!!\n");
		i2c->regs->CMD = Hw6;
		ret = wait_intr(i2c);
		BITSET(i2c->regs->CMD, Hw0);
#endif
		return ret;
	}
	BITSET(i2c->regs->CMD, Hw0);	/* Clear a pending interrupt */
#if 0
	if(i2c->core == 3){
		printk("SEND[%x][%d]:", i2c->msg->addr, i2c->msg->len);
		for (i = 0; i < i2c->msg->len; i++) {
			printk("0x%02x ", i2c->msg->buf[i]);
		}
		printk("\n");
	}
#endif
	for (i = 0; i < i2c->msg->len; i++) {
		i2c->regs->TXR = i2c->msg->buf[i];

        if(i2c->msg->flags == I2C_M_WM899x_CODEC)   /* B090183 */
            i2c->regs->CMD = Hw4 | Hw3;
        else
            {
                i2c->regs->CMD = Hw4;
            }
            if(i2c->msg->flags & I2C_M_TCC_IPOD){
                ret = wait_ack_for_IPOD(i2c);
            }else{
                ret = wait_intr(i2c);
            }
		if (ret != 0){
#if NACK_STOP_SIGNAL
		//printk("NACK!! Stop signal enable!! %s - %d\n", __func__, __LINE__);
			i2c->regs->CMD = Hw6;
			ret = wait_intr(i2c);
			BITSET(i2c->regs->CMD, Hw0);
#endif
			return ret;
		}
		BITSET(i2c->regs->CMD, Hw0);
		//i2c->msg->buf[i] = (unsigned char) i2c->regs->RXR;	//XXX
	}

	/*
	 *	Check no-stop operation condition (write -> read operation)
	 *	1. DxB
	 * 	2. msg_num == 2
	 */
	if (i2c->msg->flags & I2C_M_WR_RD)
		no_stop = 1;
	if (i2c->msg_num > 1)
		no_stop = 1;
	
	if (no_stop == 0) {
		i2c->regs->CMD = Hw6;
		ret = wait_intr(i2c);
		if (ret != 0)
			return ret;
	}
	BITSET(i2c->regs->CMD, Hw0);	/* Clear a pending interrupt */
#if defined(CONFIG_I2C_DEBUG_BUS_SYSFS)
	incr_trans_bytes(i2c, I2C_SENT);
#endif

	return 1;
}

/* tcc_i2c_doxfer
 *
 * this starts an i2c transfer
*/
static int tcc_i2c_doxfer(struct tcc_i2c *i2c, struct i2c_msg *msgs, int num)
{
	int ret=0, i=0, retry=0, retry1 = 0;
	
	if(msgs[0].flags & I2C_M_TCC_IPOD){
              
        	for (i = 0; i < num; i++) {
        		spin_lock_irq(&i2c->lock);
        		i2c->msg 		= &msgs[i];
        		i2c->msg->flags = msgs[i].flags;
        		i2c->msg_num 	= num;
        		i2c->msg_ptr 	= 0;
        		i2c->msg_idx 	= 0;
        		i2c->state = STATE_START;
        		spin_unlock_irq(&i2c->lock);

			if (i2c->msg->flags & I2C_M_RD) {
				for (retry = 0; retry < i2c->adap.retries; retry++) {
					ret = recv_i2c(i2c);
					if(ret == 1){
						/*
						if(retry > 0)
							printk("recv_i2c success! - addr(0x%02X) - retry %d\n", i2c->msg->addr, retry);
						*/
						break;
					}
					//msleep(1);
				    udelay(100);
				}
				if (ret != 1){
					printk("recv_i2c failed! - addr(0x%02X) - retry %d\n", i2c->msg->addr, i2c->adap.retries);
#if defined(CONFIG_I2C_DEBUG_BUS_SYSFS)
					incr_trans_bytes(i2c, I2C_RECV_FAIL);
#endif
					return ret;
				}
			} else {
				for (retry1 = 0; retry1 < i2c->adap.retries; retry1++) {
					ret = send_i2c(i2c);
					if(ret == 1){
						/*
						if(retry > 0)
							printk("send_i2c success! - addr(0x%02X) - retry %d\n", i2c->msg->addr, retry);
						*/
						break;
					}
					//msleep(1);
					udelay(100);
				}
				if (ret != 1){
					printk("send_i2c failed! - addr(0x%02X) - retry %d\n", i2c->msg->addr, i2c->adap.retries);
#if defined(CONFIG_I2C_DEBUG_BUS_SYSFS)
					incr_trans_bytes(i2c, I2C_SEND_FAIL);
#endif
					return ret;
				}
			}
		}
              //printk(" IPOD i2c retry addr(0x%02X) - retry %d retry1 %d\n", i2c->msg->addr, retry,retry1);
	}else{
		for (i = 0; i < num; i++) {
			spin_lock_irq(&i2c->lock);
			i2c->msg 		= &msgs[i];
			i2c->msg->flags = msgs[i].flags;
			i2c->msg_num 	= num;
			i2c->msg_ptr 	= 0;
			i2c->msg_idx 	= 0;
			i2c->state = STATE_START;
			spin_unlock_irq(&i2c->lock);
	
			if (i2c->msg->flags & I2C_M_RD) {
				ret = recv_i2c(i2c);
				if (ret != 1){
					printk("recv_i2c failed! - addr(0x%02X)\n", i2c->msg->addr);
#if defined(CONFIG_I2C_DEBUG_BUS_SYSFS)
					incr_trans_bytes(i2c, I2C_RECV_FAIL);
#endif
					break;
				 }
			} else {
				ret = send_i2c(i2c);
				if (ret != 1){
					printk("send_i2c failed! - addr(0x%02X)\n", i2c->msg->addr);
#if defined(CONFIG_I2C_DEBUG_BUS_SYSFS)
					incr_trans_bytes(i2c, I2C_SEND_FAIL);
#endif
					break;
				}
			}
		}
	}

	if(ret == 1)
		ret = i;

	return ret;
}

/* tcc_i2c_xfer
 *
 * first port of call from the i2c bus code when an message needs
 * transferring across the i2c bus.
*/
static int tcc_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	struct tcc_i2c *i2c = (struct tcc_i2c *)adap->algo_data;
	int retry;
	int ret;

	if (tcc_i2c_in_suspend)
		return -ENOSTR;

	if(msgs[0].flags & I2C_M_TCC_IPOD){
		ret = tcc_i2c_doxfer(i2c, msgs, num);
		if (ret != num){
                  printk("tcc_i2c_xfer failed! - addr(0x%02X) ret %d\n", i2c->msg->addr, ret);// wskim
		}
		return ret;
	}else{
		for (retry = 0; retry < adap->retries; retry++) {
		
        		ret = tcc_i2c_doxfer(i2c, msgs, num);

        		if (ret > 0){
        			return ret;
        		}

        		dev_dbg(&i2c->adap.dev, "Retrying transmission (%d)\n", retry);

			udelay(100);
              }
    }


	return -EREMOTEIO;
}

static u32 tcc_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

/* i2c bus registration info */
static const struct i2c_algorithm tcc_i2c_algorithm = {
	.master_xfer	= tcc_i2c_xfer,
	.functionality	= tcc_i2c_func,
};

#if defined(CONFIG_I2C_DEBUG_BUS_SYSFS)
/**
 * show_info - attribute show func 
 * @dev : device
 * @attr: device attribute
 * @buf : buffer to copy message
 * display i2c transfer information
 */
static ssize_t show_info(struct device *dev,struct device_attribute *attr, char *buf)
{
	struct tcc_i2c *i2c =  dev_get_drvdata(dev);
	
	if(!i2c) 
		return 0;

	return sprintf(buf, "i2c core : %d\n"
			"i2c ch : %d\n"
			"smu i2c flag : %d\n"
			"i2c clock rate : %d\n"
			"i2c core clock rate : %d\n"
			"i2c core clock name : %s\n"
			"sent : %d bytes / %d fail\n"
			"recv : %d bytes / %d fail\n",
			i2c->core, i2c->ch, i2c->smu_i2c_flag, i2c->i2c_clk_rate, i2c->core_clk_rate,
			i2c->core_clk_name, i2c->sent, i2c->send_fail, i2c->recv, i2c->recv_fail);
}

/**
 * store_info - attribute store func 
 * @dev : device
 * @attr: device attribute
 * @buf : buffer from user massage
 * process user command  
 */
ssize_t store_info(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct tcc_i2c *i2c =  dev_get_drvdata(dev);
	
	if(!i2c) 
		return 0;

	if(!strncmp(buf, "init", strlen("init"))){
		clear_all_rec_idx(i2c);
	}

	return count;
}

/**
 * send_sample_byte - func. to process i2c send 
 * @dev : device
 * @data : data
 * This func. transfers 8 byte to slave devices.
 */
static int send_sample_byte(struct device *dev, void *data)
{
	int cnt = 0;
	char tmp[8] = {0x0AA};
	struct i2c_client *client = i2c_verify_client(dev);

	if(!client)
		return -EFAULT;

	cnt = sizeof(tmp);

	printk(KERN_INFO "send 8 bytes to slave (addr:0x%02X)\n", client->addr);
	if(i2c_master_send(client, tmp, cnt) != cnt) 
		return -EIO;

	return 0;
}

/**
 * show_debug - attribute show func 
 * @dev : device
 * @attr: device attribute
 * @buf : buffer to copy message
 */
static ssize_t show_debug(struct device *dev,struct device_attribute *attr, char *buf)
{
	return 0;	
}

/**
 * store_debug - attribute store func 
 * @dev : device
 * @attr: device attribute
 * @buf : buffer from user massage
 * calls i2c sending function for each i2c child devices 
 */
ssize_t store_debug(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct tcc_i2c *i2c =  dev_get_drvdata(dev);
	
	if(!i2c) 
		return 0;

	if(device_for_each_child(&i2c->adap.dev, NULL, send_sample_byte) < 0)
		return 0;

	return count;
}

static DEVICE_ATTR(debug, S_IWUSR | S_IRUGO, show_debug, store_debug);
static DEVICE_ATTR(info, S_IWUSR | S_IRUGO, show_info, store_info);
#endif

/* tcc_i2c_init
 *
 * initialize the I2C controller, set the IO lines and frequency
 */
static int tcc_i2c_init(struct tcc_i2c *i2c)
{
	unsigned int prescale;
    volatile struct tcc_i2c_regs *i2c_reg = i2c->regs;

	if (!i2c->smu_i2c_flag)
	{
		/* I2C clock Enable */
		if(i2c->pclk == NULL) {
			i2c->pclk = clk_get(NULL, i2c->core_clk_name);
			if (IS_ERR(i2c->pclk)) {
				i2c->pclk = NULL;
				dev_err(i2c->dev, "can't get i2c clock\n");
				return -1;
			}
		}
		if(clk_enable(i2c->pclk) != 0) {
			dev_err(i2c->dev, "can't do i2c clock enable\n");
			return -1;
		}

		/* Set i2c core clock to 4MHz */
		clk_set_rate(i2c->pclk, i2c->core_clk_rate);

		prescale = ((clk_get_rate(i2c->pclk) / 1000) / (i2c->i2c_clk_rate * 5)) - 1;
		writel(prescale, &i2c_reg->PRES);
		writel(Hw7 | Hw6 | HwZERO, &i2c_reg->CTRL);		// start enable, stop enable, 8bit mode
		writel(Hw0 | readl(&i2c_reg->CMD), &i2c_reg->CMD);	// clear pending interrupt

		/* I2C GPIO setting */
		tca_i2c_setgpio(i2c->core, i2c->ch);

#if (0)
#ifdef CONFIG_HIBERNATION
		if(!do_hibernation)
#endif
			msleep(100);    // Why does it need ?
#endif
	} 
	else
	{
		/* SMU_I2C clock setting */
		tcc_writel(0x80000000, i2c->IRQSTR);

		i2c->regs->CTRL = 0;
		i2c->regs->PRES = 4;
		BITSET(i2c->regs->CTRL, Hw7|Hw6);
	}

	return 0;
}

/* tcc83xx_i2c_probe
 *
 * called by the bus driver when a suitable device is found
*/
static int tcc_i2c_probe(struct platform_device *pdev)
{
	int i, ret = 0;
	struct tcc_i2c *i2c;
    struct tcc_i2c_platform_data *tcc_platform_data = pdev->dev.platform_data;
	struct resource *resource[I2C_NUM_OF_CH];

	tcc_i2c_in_suspend = 0;

	i2c = kzalloc(sizeof(struct tcc_i2c) * I2C_NUM_OF_CH, GFP_KERNEL);
	if (!i2c) {
		ret = -ENOMEM;
		goto err_nockl;
	}

	for (i = 0; i < I2C_NUM_OF_CH; i++) {
		resource[i] = platform_get_resource(pdev, IORESOURCE_IO, i);
		if (!resource[i]) {
            printk("i2c%d: no io resource? (ch%d)\n", pdev->id, i);
			dev_err(&pdev->dev, "no io resource? (ch%d)\n", i);
			ret = -ENODEV;
			goto err_io;
		}

		i2c[i].adap.owner = THIS_MODULE;
		i2c[i].adap.algo = &tcc_i2c_algorithm;
		i2c[i].adap.retries = 2;
		sprintf(i2c[i].adap.name, "%s-ch%d",pdev->name, i);

		spin_lock_init(&i2c[i].lock);
		init_waitqueue_head(&i2c[i].wait);

		i2c[i].core     = pdev->id;
		i2c[i].ch       = i;
		i2c[i].regs     = (volatile struct tcc_i2c_regs *) IO_ADDRESS(resource[i]->start);
        i2c[i].IRQSTR   = resource[i]->end;
        i2c[i].smu_i2c_flag  = tcc_platform_data->smu_i2c_flag;
        i2c[i].i2c_clk_rate  = tcc_platform_data->i2c_ch_clk_rate[i];
        i2c[i].core_clk_rate = tcc_platform_data->core_clk_rate;
        memcpy(i2c[i].core_clk_name, tcc_platform_data->core_clk_name, sizeof(tcc_platform_data->core_clk_name));

        /* Set GPIO and Enable Clock */
		if(tcc_i2c_init(i2c + i) < 0) {
            goto err_io;
        }

		i2c[i].adap.algo_data = i2c + i;
		i2c[i].adap.dev.parent = &pdev->dev;
		i2c[i].adap.class = I2C_CLASS_HWMON | I2C_CLASS_SPD;
		i2c[i].adap.nr = pdev->id * I2C_NUM_OF_CH + i;

		ret = i2c_add_numbered_adapter(&i2c[i].adap);
		if (ret < 0) {
			printk("%s: Failed to add bus\n", i2c[i].adap.name);
			for (i--; i >= 0; i--)
				i2c_del_adapter(&i2c[i].adap);
			goto err_clk;
		}
	}

#if defined(CONFIG_I2C_DEBUG_BUS_SYSFS)
	ret = device_create_file(&pdev->dev, &dev_attr_info);
	if (ret)
		goto err_clk;
	ret = device_create_file(&pdev->dev, &dev_attr_debug);
	if (ret)
		goto err_dev;
#endif

	platform_set_drvdata(pdev, i2c);

	return 0;

#if defined(CONFIG_I2C_DEBUG_BUS_SYSFS)
err_dev:
	device_remove_file(&pdev->dev, &dev_attr_info);
#endif

err_clk:
    printk("~~~~~~ %s() err_clk\n", __func__);
	//TODO: I2C peri bus disable
	for(i = 0;i < I2C_NUM_OF_CH;i++) {
    	if(i2c[i].pclk != NULL) {
            clk_disable(i2c[i].pclk);
            clk_put(i2c[i].pclk);
            i2c[i].pclk = NULL;
        }
    }
err_io:
    printk("~~~~~~ %s() err_io\n", __func__);
	kfree(i2c);
err_nockl:
    printk("~~~~~~ %s() err_nock1\n", __func__);
	return ret;
}

static int tcc_i2c_remove(struct platform_device *pdev)
{
	int i;
	struct tcc_i2c *i2c = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);

    /* I2C clock Disable */
	for (i = 0; i < I2C_NUM_OF_CH; i++) {
		i2c_del_adapter(&i2c[i].adap);
        /* I2C bus & clock disable */
        clk_disable(i2c[i].pclk);
        clk_put(i2c[i].pclk);
    }

#if defined(CONFIG_I2C_DEBUG_BUS_SYSFS)
	device_remove_file(&pdev->dev, &dev_attr_info);
	device_remove_file(&pdev->dev, &dev_attr_debug);
#endif

	kfree(i2c);

	return 0;
}

#ifdef CONFIG_PM
static int tcc_i2c_suspend_late(struct platform_device *pdev, pm_message_t state)
{
    int i;
	struct tcc_i2c *i2c = platform_get_drvdata(pdev);

	tcc_i2c_in_suspend = 1;

	/* I2C Clock Disable */
	for(i = 0;i < I2C_NUM_OF_CH;i++) {
        if(i2c[i].pclk != NULL) {
            clk_disable(i2c[i].pclk);
            clk_put(i2c[i].pclk);
            i2c[i].pclk = NULL;
        }
    }
	return 0;
}

static int tcc_i2c_resume_early(struct platform_device *pdev)
{
	int i;
	struct tcc_i2c *i2c = platform_get_drvdata(pdev);

	/* Set gpio and I2C Clock Enable */
	for (i = 0; i < I2C_NUM_OF_CH; i++) {
		tcc_i2c_init(i2c + i);
	}

	tcc_i2c_in_suspend = 0;
	return 0;
}
#else
#define tcc_i2c_suspend_late	NULL
#define tcc_i2c_resume_early	NULL
#endif

/* device driver for platform bus bits */

static struct platform_driver tcc_i2c_driver = {
	.probe			= tcc_i2c_probe,
	.remove			= tcc_i2c_remove,
	.suspend		= tcc_i2c_suspend_late,
	.resume			= tcc_i2c_resume_early,
	.driver			= {
		.owner		= THIS_MODULE,
		.name		= "tcc-i2c",
	},
};

static int __init tcc_i2c_adap_init(void)
{
    return platform_driver_register(&tcc_i2c_driver);
}

static void __exit tcc_i2c_adap_exit(void)
{
    platform_driver_unregister(&tcc_i2c_driver);
}

subsys_initcall(tcc_i2c_adap_init);
module_exit(tcc_i2c_adap_exit);

MODULE_AUTHOR("Telechips Inc. SYS4-3 linux@telechips.com");
MODULE_DESCRIPTION("Telechips H/W I2C driver");
MODULE_LICENSE("GPL");
