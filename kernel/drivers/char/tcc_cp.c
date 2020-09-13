/* 
 * linux/drivers/char/iap_gpio.c
 *
 * Author:  <android_ce@telechips.com>
 * Created: 7th May, 2013 
 * Description: Telechips Linux CP-GPIO DRIVER

One line to give the program's name and a brief idea of what it does.
Copyright (C) 2013 Telechips Inc.

This program is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place,
Suite 330, Boston, MA 02111-1307 USA
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/earlysuspend.h>
#include <asm/uaccess.h>

#include <asm/io.h>
#include <asm/gpio.h>
#include <mach/tcccp_ioctl.h>

#define CP_DEV_NAME		"tcc-cp"

/**
 * @author valky@cleinsoft
 * @date 2013/07/26
 * D-Audio don't have gpio expander.
 * valky@cleinsoft 13/11/04 CP_RESET_GPIO mapping
 **/
#if defined(CONFIG_DAUDIO)
#if defined(CONFIG_DAUDIO_20140220)
#define CP_PWR_GPIO     (TCC_GPA(4)) 
#define CP_RESET_GPIO   (TCC_GPG(16))
#else
#define CP_PWR_GPIO     (TCC_GPA(4)) 
#define CP_RESET_GPIO   (TCC_GPA(4))
#endif
#else
#define CP_PWR_GPIO                       (TCC_GPEXT1(9)) 
#define CP_RESET_GPIO                       (TCC_GPEXT2(3)) 
#endif

#define CP_GPIO_HI	        		1
#define CP_GPIO_LOW			0

#define CP_STATE_ENABLE	        		1
#define CP_STATE_DISABLE                        0

#define CP_DEBUG    0

#if CP_DEBUG
#define cp_dbg(fmt, arg...)     printk(fmt, ##arg)
#else
#define cp_dbg(arg...)
#endif

static struct class *cp_class;

unsigned int gCpVersion = 0;
unsigned int gCpI2cChannel = 0;
static unsigned int gCpState = 0;

void tcc_cp_gpio_init(void)
{
    cp_dbg("%s  \n", __FUNCTION__);
    gpio_request(CP_PWR_GPIO, NULL);
    #if defined(CONFIG_TCC_CP_20C)
    gpio_direction_output(CP_PWR_GPIO,0);
    #else
    gpio_direction_output(CP_PWR_GPIO,1);
    #endif

    gpio_request(CP_RESET_GPIO, NULL);
    #if defined(CONFIG_TCC_CP_20C)
    gpio_direction_output(CP_RESET_GPIO,0);
    #else
    gpio_direction_output(CP_RESET_GPIO,1);
    #endif
}

void tcc_cp_pwr_control(int value)
{
    cp_dbg("%s  \n", __FUNCTION__);
    if(value != CP_GPIO_LOW)
        gpio_set_value(CP_PWR_GPIO, CP_GPIO_HI);
    else
        gpio_set_value(CP_PWR_GPIO, CP_GPIO_LOW);
}

void tcc_cp_reset_control(int value)
{
    cp_dbg("%s  \n", __FUNCTION__);
    if(value != CP_GPIO_LOW)
        gpio_set_value(CP_RESET_GPIO, CP_GPIO_HI);
    else
        gpio_set_value(CP_RESET_GPIO, CP_GPIO_LOW);
}


long tcc_cp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    cp_dbg("%s  (0x%x)  \n", __FUNCTION__, cmd);
	
    switch (cmd)
    {
        case IOCTL_CP_CTRL_INIT:
        {
            tcc_cp_gpio_init();		
            cp_dbg(KERN_INFO "%s:  IOCTL_CP_SET_GPIO_INIT (0x%x) \n", __FUNCTION__, cmd);
        }
            break;
        case IOCTL_CP_CTRL_PWR:
        {
            if(arg != 0)
                tcc_cp_pwr_control(CP_GPIO_HI);
            else
                tcc_cp_pwr_control(CP_GPIO_LOW);
            cp_dbg(KERN_INFO "%s:  IOCTL_CP_SET_PWR arg(%d) \n", __FUNCTION__, arg);			
        }
            break;
        case IOCTL_CP_CTRL_RESET:
        {
            if(arg != 0)
                tcc_cp_reset_control(CP_GPIO_HI);
            else
                tcc_cp_reset_control(CP_GPIO_LOW);
            
            cp_dbg(KERN_INFO "%s:  IOCTL_CP_SET_PWR_OFF arg(%d) \n", __FUNCTION__, arg);			
        }
            break;  
        case IOCTL_CP_CTRL_ALL:
        {
            if(arg != 0){
                tcc_cp_pwr_control(CP_GPIO_HI);
                tcc_cp_reset_control(CP_GPIO_HI);
            } else{
                tcc_cp_pwr_control(CP_GPIO_LOW);
                tcc_cp_reset_control(CP_GPIO_LOW);
            }
            
            cp_dbg(KERN_INFO "%s:  IOCTL_CP_SET_PWR_OFF arg(%d) \n", __FUNCTION__, arg);			
        }
            break;              
        case IOCTL_CP_GET_VERSION:
            if(copy_to_user((unsigned int*) arg, &gCpVersion, sizeof(unsigned int))!=0)
            {
                printk(" %s copy version error~!\n",__func__);
            }			                
            break;
        case IOCTL_CP_GET_CHANNEL:
            if(copy_to_user((unsigned int*) arg, &gCpI2cChannel, sizeof(unsigned int))!=0)
            {
                printk(" %s copy channel error~!\n",__func__);
            }			                
            break;        
        case IOCTL_CP_SET_STATE:
            if(arg != 0){
                gCpState = CP_STATE_ENABLE;
            }else{
                gCpState = CP_STATE_DISABLE;
            }
            //printk(" %s set gCpState %d \n",__func__,gCpState);
            break;
        case IOCTL_CP_GET_STATE:
            if(copy_to_user((unsigned int*) arg, &gCpState, sizeof(unsigned int))!=0)
            {
                printk(" %s copy state error~!\n",__func__);
            }		
            //printk(" %s get gCpState %d \n",__func__,gCpState);
            break;
        default:
            cp_dbg("cp: unrecognized ioctl (0x%x)\n", cmd); 
            return -EINVAL;
            break;
    }
    return 0;
}

static int tcc_cp_release(struct inode *inode, struct file *filp)
{
    cp_dbg("%s \n", __FUNCTION__);

    return 0;
}

static int tcc_cp_open(struct inode *inode, struct file *filp)
{
    cp_dbg("%s : \n", __FUNCTION__);

    return 0;
}

struct file_operations tcc_cp_fops =
{
    .owner    = THIS_MODULE,
    .open     = tcc_cp_open,
    .release  = tcc_cp_release,
    .unlocked_ioctl = tcc_cp_ioctl,
};

int __init tcc_cp_init(void)
{
    int ret;

    cp_dbg(KERN_INFO "%s \n", __FUNCTION__);

    tcc_cp_gpio_init();

    ret = register_chrdev(CP_DEV_MAJOR_NUM, CP_DEV_NAME, &tcc_cp_fops);
    cp_dbg("%s: register_chrdev ret : %d\n", __func__, ret);
    if(ret >= 0)
    {
        cp_class = class_create(THIS_MODULE, CP_DEV_NAME);
        device_create(cp_class,NULL,MKDEV(CP_DEV_MAJOR_NUM,CP_DEV_MINOR_NUM),NULL,CP_DEV_NAME);
    }

    #if defined(CONFIG_TCC_CP_20C)
    gCpVersion = 0x20C;
    #else
    gCpVersion = 0x20B;
    #endif
    
    #if defined(CONFIG_TCC_CP_I2C3)
    gCpI2cChannel = 3;
    #else
    gCpI2cChannel = 0;
    #endif

    return ret;
}

void __exit tcc_cp_exit(void)
{
    cp_dbg(KERN_INFO "%s\n", __FUNCTION__);
    unregister_chrdev(CP_DEV_MAJOR_NUM, CP_DEV_NAME);

    class_destroy(cp_class);
    cp_class = NULL;
}

module_init(tcc_cp_init);
module_exit(tcc_cp_exit);

MODULE_AUTHOR("Telechips Inc. c2-g3-3 android_ce@telechips.com");
MODULE_DESCRIPTION("TCCxxx cp-gpio driver");
MODULE_LICENSE("GPL");



