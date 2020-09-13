#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/earlysuspend.h>

#include <mach/bsp.h>
#include <mach/io.h>
#include <mach/daudio_debug.h>


CLOG_INIT(CLOG_TAG_CLEINSOFT, CLOG_TAG_EYEPATTERN, CLOG_LEVEL_EYEPATTERN);



// HS_DC_LEVEL hs_dc_level
// SS_DEEM_35 ss_deem_35
// SS_DEEM_6 ss_deem_6
// SS_AMP ss_amp

#define TXVRT_SHIFT 6
#define TXVRT_MASK (0xF<<TXVRT_SHIFT)
#define TXRISE_SHIFT 10
#define TXRISE_MASK (0x3<<TXRISE_SHIFT)

static ssize_t show_hs_dc_level(struct device *dev, struct device_attribute *attr, char *buf)
{
	PUSBPHYCFG pUsbPhyCfg=(PUSBPHYCFG)tcc_p2v(HwUSBPHYCFG_BASE);
	int hdl=(pUsbPhyCfg->UPCR2&TXVRT_MASK)>>TXVRT_SHIFT;

	CLOG(CLL_TRACE, "show_hs_dc_level dev=%p, attr=%p, buf=%p\n", dev, attr, buf);
	CLOG(CLL_TRACE, "UPCR2=0x%X\n", pUsbPhyCfg->UPCR2);

	return sprintf(buf, "[%s] HS TX DC voltage level=%d(0x%x)\n", dev_name(dev), hdl, hdl);
}

static ssize_t store_hs_dc_level(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int hdl;
	CLOG(CLL_TRACE, "store_hs_dc_level dev=%p, attr=%p, buf=%p, count=%d\n", dev, attr, buf, count);

	if( 1==sscanf(buf, "%d", &hdl) )
	{
		CLOG(CLL_TRACE, "hs dc level=%d\n", hdl);
		if( 0<=hdl && hdl<=0xf )
		{
			PUSBPHYCFG pUsbPhyCfg=(PUSBPHYCFG)tcc_p2v(HwUSBPHYCFG_BASE);

			BITCSET(pUsbPhyCfg->UPCR2, TXVRT_MASK, hdl<<TXVRT_SHIFT);
			BITCSET(pUsbPhyCfg->UPCR2, TXRISE_MASK, 0x3<<TXRISE_SHIFT);
		}
		else
		{
			CLOG(CLL_ERROR, "[error]range over\n");
			count=-EINVAL;
		}
	}
	else
	{
		CLOG(CLL_ERROR, "[error]sscanf\n");
		count=-EINVAL;
	}

	return count;
}

static DEVICE_ATTR(hs_dc_level, S_IRUSR|S_IWUSR, show_hs_dc_level, store_hs_dc_level);



static ssize_t show_ss_deem_35(struct device *dev, struct device_attribute *attr, char *buf)
{
	PUSBPHYCFG pUsbPhyCfg=(PUSBPHYCFG)tcc_p2v(HwUSBPHYCFG_BASE);
	int sd35=(pUsbPhyCfg->UPCR3&(Hw21|Hw22|Hw23|Hw24|Hw25|Hw26))>>21;

	CLOG(CLL_TRACE, "show_ss_deem_35 dev=%p, attr=%p, buf=%p\n", dev, attr, buf);
	CLOG(CLL_TRACE, "UPCR3=0x%X\n", pUsbPhyCfg->UPCR3);

	return sprintf(buf, "[%s] SS TX driver de-emphasis at 3.5dB=%d(0x%x)\n", dev_name(dev), sd35, sd35);
}

static ssize_t store_ss_deem_35(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int sd35;
	CLOG(CLL_TRACE, "store_ss_deem_35 dev=%p, attr=%p, buf=%p\n", dev, attr, buf);

	if( 1==sscanf(buf, "%d", &sd35) )
	{
		CLOG(CLL_TRACE, "ss deem 35=%d\n", sd35);
		if( 0<=sd35 && sd35<=0x3f )
		{
			PUSBPHYCFG pUsbPhyCfg=(PUSBPHYCFG)tcc_p2v(HwUSBPHYCFG_BASE);
			BITCSET(pUsbPhyCfg->UPCR3, Hw21|Hw22|Hw23|Hw24|Hw25|Hw26, sd35<<21);
		}
		else
		{
			CLOG(CLL_ERROR, "[error]range over\n");
			count=-EINVAL;
		}
	}
	else
	{
		CLOG(CLL_ERROR, "[error]sscanf\n");
		count=-EINVAL;
	}

	return count;
}

static DEVICE_ATTR(ss_deem_35, S_IRUSR|S_IWUSR, show_ss_deem_35, store_ss_deem_35);


static ssize_t show_ss_deem_6(struct device *dev, struct device_attribute *attr, char *buf)
{
	PUSBPHYCFG pUsbPhyCfg=(PUSBPHYCFG)tcc_p2v(HwUSBPHYCFG_BASE);
	int sd6=(pUsbPhyCfg->UPCR3&(Hw15|Hw16|Hw17|Hw18|Hw19|Hw20))>>15;

	CLOG(CLL_TRACE, "show_ss_deem_6 dev=%p, attr=%p, buf=%p\n", dev, attr, buf);
	CLOG(CLL_TRACE, "UPCR3=0x%X\n", pUsbPhyCfg->UPCR3);

	return sprintf(buf, "[%s] SS Tx driver de-emphasis at 6dB=%d(0x%x)\n", dev_name(dev), sd6, sd6);
}

static ssize_t store_ss_deem_6(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int sd6;
	CLOG(CLL_TRACE, "store_ss_deem_6 dev=%p, attr=%p, buf=%p\n", dev, attr, buf);

	if( 1==sscanf(buf, "%d", &sd6) )
	{
		CLOG(CLL_TRACE, "ss deem 6=%d\n", sd6);
		if( 0<=sd6 && sd6<=0x3f )
		{
			PUSBPHYCFG pUsbPhyCfg=(PUSBPHYCFG)tcc_p2v(HwUSBPHYCFG_BASE);
			BITCSET(pUsbPhyCfg->UPCR3, Hw15|Hw16|Hw17|Hw18|Hw19|Hw20, sd6<<15);
		}
		else
		{
			CLOG(CLL_ERROR, "[error]range over\n");
			count=-EINVAL;
		}
	}
	else
	{
		CLOG(CLL_ERROR, "[error]sscanf\n");
		count=-EINVAL;
	}

	return count;
}

static DEVICE_ATTR(ss_deem_6, S_IRUSR|S_IWUSR, show_ss_deem_6, store_ss_deem_6);


static ssize_t show_ss_amp(struct device *dev, struct device_attribute *attr, char *buf)
{
	PUSBPHYCFG pUsbPhyCfg=(PUSBPHYCFG)tcc_p2v(HwUSBPHYCFG_BASE);
	int sa=(pUsbPhyCfg->UPCR3&(Hw8|Hw9|Hw10|Hw11|Hw12|Hw13|Hw14))>>8;

	CLOG(CLL_TRACE, "show_ss_amp dev=%p, attr=%p, buf=%p\n", dev, attr, buf);
	CLOG(CLL_TRACE, "UPCR3=0x%X\n", pUsbPhyCfg->UPCR3);

	return sprintf(buf, "[%s] SS Tx Amplitude=%d(0x%x)\n", dev_name(dev), sa, sa);
}

static ssize_t store_ss_amp(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int sa;
	CLOG(CLL_TRACE, "store_ss_amp dev=%p, attr=%p, buf=%p\n", dev, attr, buf);

	if( 1==sscanf(buf, "%d", &sa) )
	{
		CLOG(CLL_TRACE, "ss amp=%d\n", sa);
		if( 0<=sa && sa<=0x7f )
		{
			PUSBPHYCFG pUsbPhyCfg=(PUSBPHYCFG)tcc_p2v(HwUSBPHYCFG_BASE);
			BITCSET(pUsbPhyCfg->UPCR3, Hw8|Hw9|Hw10|Hw11|Hw12|Hw13|Hw14, sa<<8);
		}
		else
		{
			CLOG(CLL_ERROR, "[error]range over\n");
			count=-EINVAL;
		}
	}
	else
	{
		CLOG(CLL_ERROR, "[error]sscanf\n");
		count=-EINVAL;
	}

	return count;
}

static DEVICE_ATTR(ss_amp, S_IRUSR|S_IWUSR, show_ss_amp, store_ss_amp);


static long daudio_eyepattern_ioctl(struct file *pFile, unsigned int cmd, unsigned long arg)
{
	CLOG(CLL_TRACE, "static long daudio_eyepattern_ioctl(pFile=0x%p, cmd=%u, arg=%lu\n", pFile, cmd, arg);

	return 0;
}

static int daudio_eyepattern_open(struct inode *pInode, struct file *pFile)
{
	CLOG(CLL_TRACE, "static int daudio_eyepattern_open(pInode=0x%p, pFile=0x%p", pInode, pFile);

	return 0;
}

static int daudio_eyepattern_release(struct inode *pInode, struct file *pFile)
{
	CLOG(CLL_TRACE, "static int daudio_eyepattern_release(pInode=0x%p, pFile=0x%p", pInode, pFile);

	return 0;
}

static void eyepattern_early_suspend(struct early_suspend *h)
{
	CLOG(CLL_TRACE, "eyepattern_early_suspend\n");
}

static void eyepattern_late_resume(struct early_suspend *h)
{
	CLOG(CLL_TRACE, "eyepattern_late_resume\n");
}

static struct file_operations sFileOperationsDAudioEyePattern=
{
	.owner=THIS_MODULE,
	.unlocked_ioctl=daudio_eyepattern_ioctl,
	.open=daudio_eyepattern_open,
	.release=daudio_eyepattern_release,
};

static struct early_suspend daudio_eyepattern_early_suspend=
{
	.suspend=eyepattern_early_suspend,
	.resume=eyepattern_late_resume,
	.level=EARLY_SUSPEND_LEVEL_DISABLE_FB,
};


static struct class *spClassDAudioEyePattern;
static struct device *spDevice;

static const char *spDriverName="eye_pattern";
static const char *spSysDriverName="daudio_eyepattern";


static int eyepattern_init(void)
{
	int r;
	CLOG(CLL_TRACE, "static int eyepattern_init\n");

	r=register_chrdev(DAUDIO_EYEPATTERN_MAJOR, spSysDriverName, &sFileOperationsDAudioEyePattern);
	if( r<0 )
	{
		CLOG(CLL_ERROR, "register_chrdev=%d\n", r);
		r=-EIO;
	}
	else
	{
		spClassDAudioEyePattern=class_create(THIS_MODULE, spSysDriverName);
		if( IS_ERR(spClassDAudioEyePattern) )
		{
			CLOG(CLL_ERROR, "class_create=0x%p\n", spClassDAudioEyePattern);
			r=PTR_ERR(spClassDAudioEyePattern);
		}
		else
		{
			spDevice=device_create(spClassDAudioEyePattern, NULL, MKDEV(DAUDIO_EYEPATTERN_MAJOR, 0), NULL, spDriverName);
			if( IS_ERR(spDevice) )
			{
				CLOG(CLL_ERROR, "device_create=0x%p\n", spDevice);
				r=PTR_ERR(spDevice);
			}
			else
			{
				if( device_create_file(spDevice, &dev_attr_hs_dc_level) )
				{
					CLOG(CLL_ERROR, "[error]device_create_file(spDevice, &dev_attr_hs_dc_level)\n");
				}

				if( device_create_file(spDevice, &dev_attr_ss_deem_35) )
				{
					CLOG(CLL_ERROR, "[error]device_create_file(spDevice, &dev_attr_ss_deem_35)\n");
				}

				if( device_create_file(spDevice, &dev_attr_ss_deem_6) )
				{
					CLOG(CLL_ERROR, "[error]device_create_file(spDevice, &dev_attr_ss_deem_6)\n");
				}

				if( device_create_file(spDevice, &dev_attr_ss_amp) )
				{
					CLOG(CLL_ERROR, "[error]device_create_file(spDevice, &dev_attr_ss_amp)\n");
				}

				if( device_create_file_clog(spDevice) )
				{
					CLOG(CLL_ERROR, "[error]device_create_file_clog\n");
				}

				register_early_suspend(&daudio_eyepattern_early_suspend);

				r=0;
			}
		}
	}

	return r;
}

static void eyepattern_exit(void)
{
	CLOG(CLL_TRACE, "static void eyepattern_exit\n");

	device_remove_file_clog(spDevice);

	device_remove_file(spDevice, &dev_attr_hs_dc_level);
	device_remove_file(spDevice, &dev_attr_ss_deem_35);
	device_remove_file(spDevice, &dev_attr_ss_deem_6);
	device_remove_file(spDevice, &dev_attr_ss_amp);

	device_destroy(spClassDAudioEyePattern, MKDEV(DAUDIO_EYEPATTERN_MAJOR, 0));
	class_destroy(spClassDAudioEyePattern);

	unregister_chrdev(DAUDIO_EYEPATTERN_MAJOR, spDriverName);
}


module_init(eyepattern_init);
module_exit(eyepattern_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Cleinsoft");
MODULE_DESCRIPTION("D-Audio Eye Pattern Driver");

