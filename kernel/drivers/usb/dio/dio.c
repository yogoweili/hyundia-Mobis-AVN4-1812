/*
 * DIPO Filter driver - 1.0
 *
 * This driver is based on the 2.6.3 version of drivers/usb/usb-skeleton.c
 * but has been rewritten to be use in DiPO.
 *
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/usb.h>
#include <linux/mutex.h>

#define USB_DEVICE_VENDOR(vend, prod) \
         .match_flags = USB_DEVICE_ID_MATCH_VENDOR, \
         .idVendor = (vend), \
         .idProduct = (prod)

/* Define these values to match your devices */
#define USB_VENDOR_ID						0x05AC
#define USB_iPhone5_PRODUCT_ID	0x12A8
#define USB_iPhone4_PRODUCT_ID	0x1297
#define USB_iPhone4S_PRODUCT_ID	0x12A0
#define USB_iPad_PRODUCT_ID			0x12AB
#define USB_iPod_PRODUCT_ID			0x1261

/* table of devices that work with this driver */
static const struct usb_device_id dipo_table[] = {
	{ USB_DEVICE_VENDOR(USB_VENDOR_ID, USB_iPhone5_PRODUCT_ID) },
	{ USB_DEVICE_VENDOR(USB_VENDOR_ID, USB_iPhone4_PRODUCT_ID) },
	{ USB_DEVICE_VENDOR(USB_VENDOR_ID, USB_iPad_PRODUCT_ID) },
	{ }					/* Terminating entry */
};
MODULE_DEVICE_TABLE(usb, dipo_table);


/* Get a minor range for your devices from the usb maintainer */
#define USB_MINOR_BASE	192

/* our private defines. if this grows any larger, use your own .h file */
#define MAX_TRANSFER		(PAGE_SIZE - 512)
/* MAX_TRANSFER is chosen so that the VM is not stressed by
   allocations > PAGE_SIZE and the number of packets in a page
   is an integer 512 is the largest possible packet on EHCI */
#define WRITES_IN_FLIGHT	8
/* arbitrarily chosen */

/* Structure to hold all of our device specific stuff */
struct usb_dipo {
	struct usb_device	*udev;			/* the usb device for this device */
	struct usb_interface	*interface;		/* the interface for this device */
	struct semaphore	limit_sem;		/* limiting the number of writes in progress */
	struct usb_anchor	submitted;		/* in case we need to retract our submissions */
	struct urb		*bulk_in_urb;		/* the urb to read data with */
	unsigned char           *bulk_in_buffer;	/* the buffer to receive data */
	size_t			bulk_in_size;		/* the size of the receive buffer */
	size_t			bulk_in_filled;		/* number of bytes in the buffer */
	size_t			bulk_in_copied;		/* already copied to user space */
	__u8			bulk_in_endpointAddr;	/* the address of the bulk in endpoint */
	__u8			bulk_out_endpointAddr;	/* the address of the bulk out endpoint */
	int			errors;			/* the last request tanked */
	int			open_count;		/* count the number of openers */
	bool			ongoing_read;		/* a read is going on */
	bool			processed_urb;		/* indicates we haven't processed the urb */
	spinlock_t		err_lock;		/* lock for errors */
	struct kref		kref;
	struct mutex		io_mutex;		/* synchronize I/O with disconnect */
	struct completion	bulk_in_completion;	/* to wait for an ongoing read */
	spinlock_t	lock;
	bool dipo_connected;
	bool dipo_state;
	bool iap_connected;
	bool iap_state;
	struct work_struct work;	
	struct device *dev;	
	bool enabled;
};
#define to_dipo_dev(d) container_of(d, struct usb_dipo, kref)

static struct usb_driver dipo_driver;
static void dipo_draw_down(struct usb_dipo *dev);

static void dipo_delete(struct kref *kref)
{
	struct usb_dipo *dev = to_dipo_dev(kref);

	usb_free_urb(dev->bulk_in_urb);
	usb_put_dev(dev->udev);
	kfree(dev->bulk_in_buffer);
	kfree(dev);
}

static int dipo_open(struct inode *inode, struct file *file)
{
	struct usb_dipo *dev;
	struct usb_interface *interface;
	int subminor;
	int retval = 0;

	subminor = iminor(inode);

	interface = usb_find_interface(&dipo_driver, subminor);
	if (!interface) {
		err("%s - error, can't find device for minor %d",
		     __func__, subminor);
		retval = -ENODEV;
		goto exit;
	}

	dev = usb_get_intfdata(interface);
	if (!dev) {
		retval = -ENODEV;
		goto exit;
	}

	/* increment our usage count for the device */
	kref_get(&dev->kref);

	/* lock the device to allow correctly handling errors
	 * in resumption */
	mutex_lock(&dev->io_mutex);

	if (!dev->open_count++) {
		retval = usb_autopm_get_interface(interface);
			if (retval) {
				dev->open_count--;
				mutex_unlock(&dev->io_mutex);
				kref_put(&dev->kref, dipo_delete);
				goto exit;
			}
	} /* else { //uncomment this block if you want exclusive open
		retval = -EBUSY;
		dev->open_count--;
		mutex_unlock(&dev->io_mutex);
		kref_put(&dev->kref, dipo_delete);
		goto exit;
	} */
	/* prevent the device from being autosuspended */

	/* save our object in the file's private structure */
	file->private_data = dev;
	mutex_unlock(&dev->io_mutex);

exit:
	return retval;
}

static int dipo_release(struct inode *inode, struct file *file)
{
	struct usb_dipo *dev;

	dev = file->private_data;
	if (dev == NULL)
		return -ENODEV;

	/* allow the device to be autosuspended */
	mutex_lock(&dev->io_mutex);
	if (!--dev->open_count && dev->interface)
		usb_autopm_put_interface(dev->interface);
	mutex_unlock(&dev->io_mutex);

	/* decrement the count on our device */
	kref_put(&dev->kref, dipo_delete);
	return 0;
}

static int dipo_flush(struct file *file, fl_owner_t id)
{
	struct usb_dipo *dev;
	int res;

	dev = file->private_data;
	if (dev == NULL)
		return -ENODEV;

	/* wait for io to stop */
	mutex_lock(&dev->io_mutex);
	dipo_draw_down(dev);

	/* read out errors, leave subsequent opens a clean slate */
	spin_lock_irq(&dev->err_lock);
	res = dev->errors ? (dev->errors == -EPIPE ? -EPIPE : -EIO) : 0;
	dev->errors = 0;
	spin_unlock_irq(&dev->err_lock);

	cancel_work_sync(&dev->work);
	
	mutex_unlock(&dev->io_mutex);

	return res;
}

static void dipo_read_bulk_callback(struct urb *urb)
{
	struct usb_dipo *dev;

	dev = urb->context;

	spin_lock(&dev->err_lock);
	/* sync/async unlink faults aren't errors */
	if (urb->status) {
		if (!(urb->status == -ENOENT ||
		    urb->status == -ECONNRESET ||
		    urb->status == -ESHUTDOWN))
			err("%s - nonzero write bulk status received: %d",
			    __func__, urb->status);

		dev->errors = urb->status;
	} else {
		dev->bulk_in_filled = urb->actual_length;
	}
	dev->ongoing_read = 0;
	spin_unlock(&dev->err_lock);

	complete(&dev->bulk_in_completion);
}

static int dipo_do_read_io(struct usb_dipo *dev, size_t count)
{
	int rv;

	/* prepare a read */
	usb_fill_bulk_urb(dev->bulk_in_urb,
			dev->udev,
			usb_rcvbulkpipe(dev->udev,
				dev->bulk_in_endpointAddr),
			dev->bulk_in_buffer,
			min(dev->bulk_in_size, count),
			dipo_read_bulk_callback,
			dev);
	/* tell everybody to leave the URB alone */
	spin_lock_irq(&dev->err_lock);
	dev->ongoing_read = 1;
	spin_unlock_irq(&dev->err_lock);

	/* do it */
	rv = usb_submit_urb(dev->bulk_in_urb, GFP_KERNEL);
	if (rv < 0) {
		err("%s - failed submitting read urb, error %d",
			__func__, rv);
		dev->bulk_in_filled = 0;
		rv = (rv == -ENOMEM) ? rv : -EIO;
		spin_lock_irq(&dev->err_lock);
		dev->ongoing_read = 0;
		spin_unlock_irq(&dev->err_lock);
	}

	return rv;
}

static ssize_t dipo_read(struct file *file, char *buffer, size_t count,
			 loff_t *ppos)
{
	struct usb_dipo *dev;
	int rv;
	bool ongoing_io;

	dev = file->private_data;

	/* if we cannot read at all, return EOF */
	if (!dev->bulk_in_urb || !count)
		return 0;

	/* no concurrent readers */
	rv = mutex_lock_interruptible(&dev->io_mutex);
	if (rv < 0)
		return rv;

	if (!dev->interface) {		/* disconnect() was called */
		rv = -ENODEV;
		goto exit;
	}

	/* if IO is under way, we must not touch things */
retry:
	spin_lock_irq(&dev->err_lock);
	ongoing_io = dev->ongoing_read;
	spin_unlock_irq(&dev->err_lock);

	if (ongoing_io) {
		/* nonblocking IO shall not wait */
		if (file->f_flags & O_NONBLOCK) {
			rv = -EAGAIN;
			goto exit;
		}
		/*
		 * IO may take forever
		 * hence wait in an interruptible state
		 */
		rv = wait_for_completion_interruptible(&dev->bulk_in_completion);
		if (rv < 0)
			goto exit;
		/*
		 * by waiting we also semiprocessed the urb
		 * we must finish now
		 */
		dev->bulk_in_copied = 0;
		dev->processed_urb = 1;
	}

	if (!dev->processed_urb) {
		/*
		 * the URB hasn't been processed
		 * do it now
		 */
		wait_for_completion(&dev->bulk_in_completion);
		dev->bulk_in_copied = 0;
		dev->processed_urb = 1;
	}

	/* errors must be reported */
	rv = dev->errors;
	if (rv < 0) {
		/* any error is reported once */
		dev->errors = 0;
		/* to preserve notifications about reset */
		rv = (rv == -EPIPE) ? rv : -EIO;
		/* no data to deliver */
		dev->bulk_in_filled = 0;
		/* report it */
		goto exit;
	}

	/*
	 * if the buffer is filled we may satisfy the read
	 * else we need to start IO
	 */

	if (dev->bulk_in_filled) {
		/* we had read data */
		size_t available = dev->bulk_in_filled - dev->bulk_in_copied;
		size_t chunk = min(available, count);

		if (!available) {
			/*
			 * all data has been used
			 * actual IO needs to be done
			 */
			rv = dipo_do_read_io(dev, count);
			if (rv < 0)
				goto exit;
			else
				goto retry;
		}
		/*
		 * data is available
		 * chunk tells us how much shall be copied
		 */

		if (copy_to_user(buffer,
				 dev->bulk_in_buffer + dev->bulk_in_copied,
				 chunk))
			rv = -EFAULT;
		else
			rv = chunk;

		dev->bulk_in_copied += chunk;

		/*
		 * if we are asked for more than we have,
		 * we start IO but don't wait
		 */
		if (available < count)
			dipo_do_read_io(dev, count - chunk);
	} else {
		/* no data in the buffer */
		rv = dipo_do_read_io(dev, count);
		if (rv < 0)
			goto exit;
		else if (!(file->f_flags & O_NONBLOCK))
			goto retry;
		rv = -EAGAIN;
	}
exit:
	mutex_unlock(&dev->io_mutex);
	return rv;
}

static void dipo_write_bulk_callback(struct urb *urb)
{
	struct usb_dipo *dev;

	dev = urb->context;

	/* sync/async unlink faults aren't errors */
	if (urb->status) {
		if (!(urb->status == -ENOENT ||
		    urb->status == -ECONNRESET ||
		    urb->status == -ESHUTDOWN))
			err("%s - nonzero write bulk status received: %d",
			    __func__, urb->status);

		spin_lock(&dev->err_lock);
		dev->errors = urb->status;
		spin_unlock(&dev->err_lock);
	}

	/* free up our allocated buffer */
	usb_free_coherent(urb->dev, urb->transfer_buffer_length,
			  urb->transfer_buffer, urb->transfer_dma);
	up(&dev->limit_sem);
}

static ssize_t dipo_write(struct file *file, const char *user_buffer,
			  size_t count, loff_t *ppos)
{
	struct usb_dipo *dev;
	int retval = 0;
	struct urb *urb = NULL;
	char *buf = NULL;
	size_t writesize = min(count, (size_t)MAX_TRANSFER);

	dev = file->private_data;

	/* verify that we actually have some data to write */
	if (count == 0)
		goto exit;

	/*
	 * limit the number of URBs in flight to stop a user from using up all
	 * RAM
	 */
	if (!(file->f_flags & O_NONBLOCK)) {
		if (down_interruptible(&dev->limit_sem)) {
			retval = -ERESTARTSYS;
			goto exit;
		}
	} else {
		if (down_trylock(&dev->limit_sem)) {
			retval = -EAGAIN;
			goto exit;
		}
	}

	spin_lock_irq(&dev->err_lock);
	retval = dev->errors;
	if (retval < 0) {
		/* any error is reported once */
		dev->errors = 0;
		/* to preserve notifications about reset */
		retval = (retval == -EPIPE) ? retval : -EIO;
	}
	spin_unlock_irq(&dev->err_lock);
	if (retval < 0)
		goto error;

	/* create a urb, and a buffer for it, and copy the data to the urb */
	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb) {
		retval = -ENOMEM;
		goto error;
	}

	buf = usb_alloc_coherent(dev->udev, writesize, GFP_KERNEL,
				 &urb->transfer_dma);
	if (!buf) {
		retval = -ENOMEM;
		goto error;
	}

	if (copy_from_user(buf, user_buffer, writesize)) {
		retval = -EFAULT;
		goto error;
	}

	/* this lock makes sure we don't submit URBs to gone devices */
	mutex_lock(&dev->io_mutex);
	if (!dev->interface) {		/* disconnect() was called */
		mutex_unlock(&dev->io_mutex);
		retval = -ENODEV;
		goto error;
	}

	/* initialize the urb properly */
	usb_fill_bulk_urb(urb, dev->udev,
			  usb_sndbulkpipe(dev->udev, dev->bulk_out_endpointAddr),
			  buf, writesize, dipo_write_bulk_callback, dev);
	urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
	usb_anchor_urb(urb, &dev->submitted);

	/* send the data out the bulk port */
	retval = usb_submit_urb(urb, GFP_KERNEL);
	mutex_unlock(&dev->io_mutex);
	if (retval) {
		err("%s - failed submitting write urb, error %d", __func__,
		    retval);
		goto error_unanchor;
	}

	/*
	 * release our reference to this urb, the USB core will eventually free
	 * it entirely
	 */
	usb_free_urb(urb);


	return writesize;

error_unanchor:
	usb_unanchor_urb(urb);
error:
	if (urb) {
		usb_free_coherent(dev->udev, writesize, buf, urb->transfer_dma);
		usb_free_urb(urb);
	}
	up(&dev->limit_sem);

exit:
	return retval;
}

static const struct file_operations dipo_fops = {
	.owner =	THIS_MODULE,
	.read =		dipo_read,
	.write =	dipo_write,
	.open =		dipo_open,
	.release =	dipo_release,
	.flush =	dipo_flush,
	.llseek =	noop_llseek,
};

/*
 * usb class driver info in order to get a minor number from the usb core,
 * and to have the device registered with the driver core
 */
static struct usb_class_driver dipo_class = {
	.name =		"dipo%d",
	.fops =		&dipo_fops,
	.minor_base =	USB_MINOR_BASE,
};


static void dipo_work(struct work_struct *data)
{
	struct usb_dipo *dev = container_of(data, struct usb_dipo, work);
	/* DiPO, iAP1 USB Event */
	char *dipo_disconnected[2] = { "USB_STATE=DIPO_DISCONNECTED", NULL };
	char *dipo_connected[2]    = { "USB_STATE=DIPO_CONNECTED", NULL };
	char *iap_disconnected[2] = { "USB_STATE=IAP_DISCONNECTED", NULL };
	char *iap_connected[2]    = { "USB_STATE=IAP_CONNECTED", NULL };
	
	char **uevent_envp = NULL;
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	if (dev->dipo_connected != dev->dipo_state)	{
		uevent_envp = dev->dipo_connected ? dipo_connected : dipo_disconnected;
		dev->dipo_state = dev->dipo_connected;
	} else if (dev->iap_connected != dev->iap_state) {
		uevent_envp = dev->iap_connected ? iap_connected : iap_disconnected;
		dev->iap_state = dev->iap_connected;
	}
	spin_unlock_irqrestore(&dev->lock, flags);

	if (uevent_envp) {
		kobject_uevent_env(&dev->dev->kobj, KOBJ_CHANGE, uevent_envp);
		//pr_info("%s: DiPO sent uevent %s\n", __func__, uevent_envp[0]);
	} else {
		//pr_info("%s: DiPO did not send uevent (%d %d %d)\n", __func__,\
				dev->dipo_connected, dev->iap_connected, dev->);
	}
}

static struct class *_dipo_class;
static char mode_string[256];
static char def_mode_string[256];
static ssize_t enable_show(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct usb_dipo *dev = dev_get_drvdata(pdev);
	return sprintf(buf, "%d\n", dev->enabled);
}

static ssize_t enable_store(struct device *pdev, struct device_attribute *attr, const char *buff, size_t size)
{
   return size;
}

static ssize_t state_show(struct device *pdev, struct device_attribute *attr, char *buf)
{
	return 0;
}

#define DESCRIPTOR_ATTR(field, format_string)				\
static ssize_t								\
field ## _show(struct device *dev, struct device_attribute *attr,	\
		char *buf)						\
{									\
	return sprintf(buf, format_string, device_desc.field);		\
}									\
static ssize_t								\
field ## _store(struct device *dev, struct device_attribute *attr,	\
		const char *buf, size_t size)				\
{									\
	int value;							\
	if (sscanf(buf, format_string, &value) == 1) {			\
		device_desc.field = value;				\
		return size;						\
	}								\
	return -1;							\
}									\
static DEVICE_ATTR(field, S_IRUGO | S_IWUSR, field ## _show, field ## _store);

#define DESCRIPTOR_STRING_ATTR(field, buffer)				\
static ssize_t								\
field ## _show(struct device *dev, struct device_attribute *attr,	\
		char *buf)						\
{									\
	return sprintf(buf, "%s", buffer);				\
}									\
static ssize_t								\
field ## _store(struct device *dev, struct device_attribute *attr,	\
		const char *buf, size_t size)				\
{									\
	if (size >= sizeof(buffer))					\
		return -EINVAL;						\
	return strlcpy(buffer, buf, sizeof(buffer));			\
}									\
static DEVICE_ATTR(field, S_IRUGO | S_IWUSR, field ## _show, field ## _store);

DESCRIPTOR_STRING_ATTR(defmode, def_mode_string)

static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR, enable_show, enable_store);
static DEVICE_ATTR(state, S_IRUGO, state_show, NULL);

static struct device_attribute *dipo_usb_attributes[] = {
	&dev_attr_defmode,
	NULL
};

static int dipo_create_device(struct usb_dipo *dev)
{
	struct device_attribute **attrs = dipo_usb_attributes;
	struct device_attribute *attr;
	int err;

	dev->dev = device_create(_dipo_class, NULL, MKDEV(0, 0), NULL, "dipo");
	if (IS_ERR(dev->dev))
		return PTR_ERR(dev->dev);

	dev_set_drvdata(dev->dev, dev);

	while ((attr = *attrs++)) {
		err = device_create_file(dev->dev, attr);
		if (err) {
			device_destroy(_dipo_class, dev->dev->devt);
			return err;
		}
	}
	return 0;
}

extern int xhci_get_function(void);

static int dipo_probe(struct usb_interface *interface,
		      const struct usb_device_id *id)
{
	struct usb_dipo *dev;
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	struct usb_ctrlrequest *cr; /* control request */
	struct urb *cr_urb;		/* the urb to iPod device */
	unsigned long flags;	
	size_t buffer_size;
	char *buf = NULL;
	int i, p;
	int retval = -ENOMEM;


	/* allocate memory for our device state and initialize it */
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);

	if (!dev) {
		err("Out of memory");
		goto error;
	}

	kref_init(&dev->kref);
	sema_init(&dev->limit_sem, WRITES_IN_FLIGHT);
	INIT_WORK(&dev->work, dipo_work);	
	mutex_init(&dev->io_mutex);
	spin_lock_init(&dev->err_lock);
	init_usb_anchor(&dev->submitted);
	init_completion(&dev->bulk_in_completion);

	dev->udev = usb_get_dev(interface_to_usbdev(interface));
	dev->interface = interface;

	if (!strcmp(dev->udev->product, "iPod")) 
		goto error;


	/* set up the endpoint information */
	/* use only the first bulk-in and bulk-out endpoints */
	iface_desc = interface->cur_altsetting;
	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
		endpoint = &iface_desc->endpoint[i].desc;

		if (!dev->bulk_in_endpointAddr &&
				usb_endpoint_is_bulk_in(endpoint)) {
			/* we found a bulk in endpoint */
			buffer_size = usb_endpoint_maxp(endpoint);
			dev->bulk_in_size = buffer_size;
			dev->bulk_in_endpointAddr = endpoint->bEndpointAddress;
			dev->bulk_in_buffer = kmalloc(buffer_size, GFP_KERNEL);
			if (!dev->bulk_in_buffer) {
				err("Could not allocate bulk_in_buffer");
				goto error;
			}
			dev->bulk_in_urb = usb_alloc_urb(0, GFP_KERNEL);
			if (!dev->bulk_in_urb) {
				err("Could not allocate bulk_in_urb");
				goto error;
			}
		}

		if (!dev->bulk_out_endpointAddr &&
				usb_endpoint_is_bulk_out(endpoint)) {
			/* we found a bulk out endpoint */
			dev->bulk_out_endpointAddr = endpoint->bEndpointAddress;
		}
	}
	if (!(dev->bulk_in_endpointAddr && dev->bulk_out_endpointAddr)) {
		err("Could not find both bulk-in and bulk-out endpoints");
		goto error;
	}

	/* save our data pointer in this interface device */
	usb_set_intfdata(interface, dev);

	/* we can register the device now, as it is ready */
	retval = usb_register_dev(interface, &dipo_class);
	if (retval) {
		/* something prevented us from registering this driver */
		err("Not able to get a minor for this device.");
		usb_set_intfdata(interface, NULL);
		goto error;
	}

	dev->dipo_connected = false;
	dev->iap_connected = false;
	dev->dipo_state = false;
	dev->iap_state = false;

	//if (dev->udev->descriptor.idProduct == USB_iPhone5_PRODUCT_ID || dev->udev->descriptor.idProduct == USB_iPad_PRODUCT_ID) {



	{
		_dipo_class = class_create(THIS_MODULE, "dipo");

		if (IS_ERR(_dipo_class))
			return PTR_ERR(_dipo_class);

		retval = dipo_create_device(dev);

		if (retval) {
			class_destroy(_dipo_class);
			goto error;
		}

		p = xhci_get_function();

		if (p == 1) {
			printk(KERN_DEBUG "dipo enable\n");
		} else if (p == 0) {
			printk(KERN_DEBUG "dipo disable. go to iAP1\n");
			goto iPhone4;
		}

		if (dev->udev->descriptor.idProduct == USB_iPhone4_PRODUCT_ID || dev->udev->descriptor.idProduct == USB_iPhone4S_PRODUCT_ID) {
			printk(KERN_DEBUG "iPhone Old device Inserted!!\n");
			goto iPhone4;
		}

		retval = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
				0x51,
				0x40,
				0x0,
				0, /* index */
				NULL,
				0,
				100000);

		if (!retval) {
			/* let the user know what node this device is now attached to */
			printk(KERN_DEBUG "DiPO Supported Device now attached to DiPO-%d", interface->minor);
			spin_lock_irqsave(&dev->lock, flags);
			if (!dev->dipo_connected) {
				dev->dipo_connected = true;
				schedule_work(&dev->work);
			} 
			spin_unlock_irqrestore(&dev->lock, flags);
		} else if (retval < 0) {
iPhone4:			
			printk(KERN_DEBUG "DiPO Unsupported Device now attached to DiPO-%d, %x", interface->minor, dev->udev->descriptor.idProduct );
			spin_lock_irqsave(&dev->lock, flags);
			if (!dev->iap_connected) {
				dev->iap_connected = true;
				schedule_work(&dev->work);
			} 
			spin_unlock_irqrestore(&dev->lock, flags);
		} else {
			printk(KERN_DEBUG "Unknown Device\n");
			retval = -1;
			goto error;
		}
	}

	//} else {

	//	printk(KERN_DEBUG "DiPO Unsupported Device now attached to DIPO-%d, %x", interface->minor, dev->udev->descriptor.idProduct );
	//}

	msleep(500);
	return 0;

error:

	if (dev)
		/* this frees allocated memory */
		kref_put(&dev->kref, dipo_delete);
	return retval;
}

static void dipo_disconnect(struct usb_interface *interface)
{
	struct usb_dipo *dev;
	int minor = interface->minor;
	struct device_attribute **attrs = dipo_usb_attributes;
	struct device_attribute *attr;
	dev = usb_get_intfdata(interface);
	unsigned long flags;
	//printk(KERN_DEBUG "disconnect %s %d %d\n", __func__, dev->dipo_state, dev->iap_state);

	spin_lock_irqsave(&dev->lock, flags);
	if (dev->dipo_state) {
		printk(KERN_DEBUG "DiPO Device #%d now disconnected", minor);
		dev->dipo_connected = false;
		schedule_work(&dev->work);
	} 
	if (dev->iap_state) {
		printk(KERN_DEBUG "iPod Device #%d now disconnected", minor);
		dev->iap_connected = false;
		schedule_work(&dev->work);
	} 
	spin_unlock_irqrestore(&dev->lock, flags);

	usb_set_intfdata(interface, NULL);

	/* give back our minor */
	usb_deregister_dev(interface, &dipo_class);

	/* prevent more I/O from starting */
	mutex_lock(&dev->io_mutex);
	dev->interface = NULL;
	mutex_unlock(&dev->io_mutex);

	usb_kill_anchored_urbs(&dev->submitted);

	/* decrement our usage count */
	kref_put(&dev->kref, dipo_delete);

	while ((attr = *attrs++)) {
		device_destroy(_dipo_class, dev->dev->devt);
	}
	class_destroy(_dipo_class);			
	msleep(500);
	cancel_work_sync(&dev->work);	
	
	
}

static void dipo_draw_down(struct usb_dipo *dev)
{
	int time;

	time = usb_wait_anchor_empty_timeout(&dev->submitted, 1000);
	if (!time)
		usb_kill_anchored_urbs(&dev->submitted);
	usb_kill_urb(dev->bulk_in_urb);
}

static int dipo_suspend(struct usb_interface *intf, pm_message_t message)
{
	struct usb_dipo *dev = usb_get_intfdata(intf);

	if (!dev)
		return 0;
	dipo_draw_down(dev);
	return 0;
}

static int dipo_resume(struct usb_interface *intf)
{
	return 0;
}

static int dipo_pre_reset(struct usb_interface *intf)
{
	struct usb_dipo *dev = usb_get_intfdata(intf);

	mutex_lock(&dev->io_mutex);
	dipo_draw_down(dev);

	return 0;
}

static int dipo_post_reset(struct usb_interface *intf)
{
	struct usb_dipo *dev = usb_get_intfdata(intf);

	/* we are sure no URBs are active - no locking needed */
	dev->errors = -EPIPE;
	mutex_unlock(&dev->io_mutex);

	return 0;
}

static struct usb_driver dipo_driver = {
	.name =		"DIPO",
	.probe =	dipo_probe,
	.disconnect =	dipo_disconnect,
	.suspend =	dipo_suspend,
	.resume =	dipo_resume,
	.pre_reset =	dipo_pre_reset,
	.post_reset =	dipo_post_reset,
	.id_table =	dipo_table,
	.supports_autosuspend = 1,
};

static int __init usb_dipo_init(void)
{
	int result;
	struct usb_dipo *dev;


	/* register this driver with the USB subsystem */
	result = usb_register(&dipo_driver);
	if (result)
		err("usb_register failed. Error number %d", result);

	return result;
}

static void __exit usb_dipo_exit(void)
{
	/* deregister this driver with the USB subsystem */
	usb_deregister(&dipo_driver);
}

module_init(usb_dipo_init);
module_exit(usb_dipo_exit);

MODULE_LICENSE("GPL");
