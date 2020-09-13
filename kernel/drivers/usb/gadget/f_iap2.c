/*
 * Gadget Function Driver for Apple IAP2 
 *
 * Copyright (C) 2013 Telechips, Inc.
 * Author: jmlee <jmlee@telechips.com>
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/freezer.h>

#include <linux/types.h>
#include <linux/file.h>
#include <linux/device.h>
#include <linux/miscdevice.h>

#include <linux/usb.h>
#include <linux/usb/ch9.h>

#define BULK_BUFFER_SIZE    512
#define INTR_BUFFER_SIZE    512

#define PROTOCOL_VERSION    2

/* String IDs */
#define INTERFACE_STRING_INDEX	0
#define INTERFACE_STRING_SERIAL_IDX		1
#define USB_MFI_SUBCLASS_VENDOR_SPEC 0xf0

/* number of tx and rx requests to allocate */
#define TX_REQ_MAX 4
#define RX_REQ_MAX 2

#define IAP2_SEND_EVT 0
#define IAP2_SEND_ZLP 1

struct iap2_event {
   /* size of the event */
   size_t      length;
   /* event data to send */
   void        *data;
};

struct iap_dev {
	struct usb_function function;
	struct usb_composite_dev *cdev;
	spinlock_t lock;

	struct usb_ep *ep_in;
	struct usb_ep *ep_out;
	//struct usb_ep *ep_intr;

	int state;
	/* set to 1 when we connect */
	int online;
	/* Set to 1 when we disconnect.
	 * Not cleared until our file is closed.
	 */
	int disconnected:1;

	/* for acc_complete_set_string */
	int string_index;

	/* set to 1 if we have a pending start request */
	int start_requested;

	int audio_mode;

	/* synchronize access to our device file */
	atomic_t open_excl;

	struct list_head tx_idle;
	struct list_head intr_idle;

	wait_queue_head_t read_wq;
	wait_queue_head_t write_wq;
	wait_queue_head_t intr_wq;
	struct usb_request *rx_req[RX_REQ_MAX];
	int rx_done;

	/* delayed work for handling iap2 start */
	struct delayed_work start_work;

};

static struct usb_interface_descriptor iap_interface_desc = {
	.bLength                = USB_DT_INTERFACE_SIZE,
	.bDescriptorType        = USB_DT_INTERFACE,
	.bInterfaceNumber       = 0,
	.bNumEndpoints          = 2,
	//.bNumEndpoints          = 3,
	.bInterfaceClass        = USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass     = USB_MFI_SUBCLASS_VENDOR_SPEC,
	.bInterfaceProtocol     = 0,
};

static struct usb_endpoint_descriptor iap_highspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(512),
};

static struct usb_endpoint_descriptor iap_highspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(512),
};

static struct usb_endpoint_descriptor iap_fullspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor iap_fullspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
   .bDescriptorType        = USB_DT_ENDPOINT,
   .bEndpointAddress       = USB_DIR_OUT,
   .bmAttributes           = USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor iap_intr_desc = {
   .bLength                = USB_DT_ENDPOINT_SIZE,
   .bDescriptorType        = USB_DT_ENDPOINT,
   .bEndpointAddress       = USB_DIR_IN,
   .bmAttributes           = USB_ENDPOINT_XFER_INT,
   .wMaxPacketSize         = __constant_cpu_to_le16(INTR_BUFFER_SIZE),
   .bInterval              = 1,
};


static struct usb_descriptor_header *fs_iap_descs[] = {
	(struct usb_descriptor_header *) &iap_interface_desc,
	(struct usb_descriptor_header *) &iap_fullspeed_in_desc,
	(struct usb_descriptor_header *) &iap_fullspeed_out_desc,
	//(struct usb_descriptor_header *) &iap_intr_desc,
	NULL,
};

static struct usb_descriptor_header *hs_iap_descs[] = {
	(struct usb_descriptor_header *) &iap_interface_desc,
	(struct usb_descriptor_header *) &iap_highspeed_in_desc,
	(struct usb_descriptor_header *) &iap_highspeed_out_desc,
	//(struct usb_descriptor_header *) &iap_intr_desc,
	NULL,
};

static struct usb_string iap_string_defs[] = {
	[INTERFACE_STRING_INDEX].s	= "iAP Interface",
	{  },	/* end of list */
};

static struct usb_gadget_strings iap_string_table = {
	.language		= 0x0409,	/* en-US */
	.strings		= iap_string_defs,
};

static struct usb_gadget_strings *iap_strings[] = {
	&iap_string_table,
	NULL,
};

/* temporary variable used between iap_open() and iap_gadget_bind() */
static struct iap_dev *_iap_dev;

static inline struct iap_dev *usb_func_to_dev(struct usb_function *f)
{
	return container_of(f, struct iap_dev, function);
}

static struct usb_request *iap_request_new(struct usb_ep *ep, int buffer_size)
{
	struct usb_request *req = usb_ep_alloc_request(ep, GFP_KERNEL);
	if (!req)
		return NULL;

	/* now allocate buffers for the requests */
	req->buf = kmalloc(buffer_size, GFP_KERNEL);
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		return NULL;
	}

	return req;
}

static void iap_request_free(struct usb_request *req, struct usb_ep *ep)
{
	if (req) {
		kfree(req->buf);
		usb_ep_free_request(ep, req);
	}
}

/* add a request to the tail of a list */
static void iap_req_put(struct iap_dev *dev, struct list_head *head,
		struct usb_request *req)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	list_add_tail(&req->list, head);
	spin_unlock_irqrestore(&dev->lock, flags);
}

/* remove a request from the head of a list */
static struct usb_request *iap_req_get(struct iap_dev *dev, struct list_head *head)
{
	unsigned long flags;
	struct usb_request *req;

	spin_lock_irqsave(&dev->lock, flags);
	if (list_empty(head)) {
		req = 0;
	} else {
		req = list_first_entry(head, struct usb_request, list);
		list_del(&req->list);
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return req;
}

static void iap_set_disconnected(struct iap_dev *dev)
{
	dev->online = 0;
	dev->disconnected = 1;
	printk(KERN_DEBUG "%s %d\n", __func__, dev->online);
}

static void iap_complete_in(struct usb_ep *ep, struct usb_request *req)
{
	struct iap_dev *dev = _iap_dev;

	if (req->status != 0) {
		printk(KERN_DEBUG "->%s %d\n", __func__, dev->online);
		iap_set_disconnected(dev);
	}

	iap_req_put(dev, &dev->tx_idle, req);

	wake_up(&dev->write_wq);
}

static void iap_complete_out(struct usb_ep *ep, struct usb_request *req)
{
	struct iap_dev *dev = _iap_dev;

	dev->rx_done = 1;
	if (req->status != 0) {
		printk(KERN_DEBUG "->%s %d\n", __func__, dev->online);
		iap_set_disconnected(dev);
	}

	wake_up(&dev->read_wq);
}

static void iap_complete_intr(struct usb_ep *ep, struct usb_request *req)
{
	struct iap_dev *dev = _iap_dev;

	if (req->status != 0)
		iap_set_disconnected(dev);

	iap_req_put(dev, &dev->intr_idle, req);

	wake_up(&dev->intr_wq);
}

static int __init create_iap_bulk_endpoints(struct iap_dev *dev,
				struct usb_endpoint_descriptor *in_desc,
				struct usb_endpoint_descriptor *out_desc
				/*struct usb_endpoint_descriptor *intr_desc*/)
{
	struct usb_composite_dev *cdev = dev->cdev;
	struct usb_request *req;
	struct usb_ep *ep;
	int i;

	DBG("create_bulk_endpoints dev: %p\n", dev);

	ep = usb_ep_autoconfig(cdev->gadget, in_desc);
	if (!ep) {
		printk("usb_ep_autoconfig for ep_in failed\n");
		return -ENODEV;
	}

	DBG("usb_ep_autoconfig for ep_in got %s\n", ep->name);
	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_in = ep;

	ep = usb_ep_autoconfig(cdev->gadget, out_desc);
	if (!ep) {
		printk("usb_ep_autoconfig for ep_out failed\n");
		return -ENODEV;
	}

	DBG("usb_ep_autoconfig for ep_out got %s\n", ep->name);

	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_out = ep;
#if 0
	ep = usb_ep_autoconfig(cdev->gadget, intr_desc);
	if (!ep) {
		printk("usb_ep_autoconfig for ep_intr failed\n");
		return -ENODEV;
	}

	DBG("usb_ep_autoconfig for iap ep_intr got %s\n", ep->name);

	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_intr = ep;
#endif

	/* now allocate requests for our endpoints */
	for (i = 0; i < TX_REQ_MAX; i++) {
		req = iap_request_new(dev->ep_in, BULK_BUFFER_SIZE);
		if (!req)
			goto fail;
		req->complete = iap_complete_in;
		iap_req_put(dev, &dev->tx_idle, req);
	}
	for (i = 0; i < RX_REQ_MAX; i++) {
		req = iap_request_new(dev->ep_out, BULK_BUFFER_SIZE);
		if (!req)
			goto fail;
		req->complete = iap_complete_out;
		dev->rx_req[i] = req;
	}
#if 0
	for (i = 0; i < INTR_REQ_MAX; i++) {
		req = iap_request_new(dev->ep_intr, INTR_BUFFER_SIZE);
		if (!req)
			goto fail;
		req->complete = iap_complete_intr;
		iap_req_put(dev, &dev->intr_idle, req);
	}
#endif
	return 0;

fail:
	pr_err("iap_bind() could not allocate requests\n");
	while ((req = iap_req_get(dev, &dev->tx_idle)))
		iap_request_free(req, dev->ep_in);
	for (i = 0; i < RX_REQ_MAX; i++)
		iap_request_free(dev->rx_req[i], dev->ep_out);
	return -1;
}

static ssize_t iap_read(struct file *fp, char __user *buf,
	size_t count, loff_t *pos)
{
	struct iap_dev *dev = fp->private_data;
	struct usb_request *req;
	int r = count, xfer;
	int ret = 0;

	DBG("iap_read(%d)\n", count);

	if (dev->disconnected) {
		return -ENODEV;
	}

	if (count > BULK_BUFFER_SIZE)
		count = BULK_BUFFER_SIZE;

	/* we will block until we're online */
	ret = wait_event_interruptible(dev->read_wq, (dev->online || !(dev->online)));
	if (ret < 0) {
		r = ret;
		goto done;
	}

requeue_req:
	/* queue a request */
	req = dev->rx_req[0];
	req->length = count;
	dev->rx_done = 0;
	ret = usb_ep_queue(dev->ep_out, req, GFP_KERNEL);
	if (ret < 0) {
		r = -EIO;
		goto done;
	} else {
		DBG("rx %p queue\n", req);
	}

	/* wait for a request to complete */
	ret = wait_event_interruptible(dev->read_wq, dev->rx_done);
	if (ret < 0) {
		r = ret;
		usb_ep_dequeue(dev->ep_out, req);
		goto done;
	}
	if (dev->online) {
		/* If we got a 0-len packet, throw it back and try again. */
		if (req->actual == 0)
			goto requeue_req;

		DBG("rx %p %d\n", req, req->actual);

#ifdef VERBOSE_DEBUG
		print_hex_dump(KERN_DEBUG, "", DUMP_PREFIX_OFFSET, 16, 1, req->buf, req->actual, 0); 
#endif

		xfer = (req->actual < count) ? req->actual : count;
		r = xfer;
		if (copy_to_user(buf, req->buf, xfer)) {
			r = -EFAULT;
		} else { 
			DBG("read ok.\n");
		}

	} else
		r = -EIO;

done:
	DBG("iap_read returning %d\n", r);
	return r;
}

static ssize_t iap_write(struct file *fp, const char __user *buf,
	size_t count, loff_t *pos)
{
	struct iap_dev *dev = fp->private_data;
	struct usb_request *req = 0;
	int r = count, xfer;
	int ret;

	DBG("iap_write(%d)\n", count);

#ifdef VERBOSE_DEBUG
	print_hex_dump(KERN_DEBUG, "", DUMP_PREFIX_OFFSET, 16, 1, buf, count, 0); 
#endif

	if (!dev->online || dev->disconnected)
		return -ENODEV;


	while (count > 0) {

		if (!dev->online) {
			pr_debug("iap_write dev->error\n");
			r = -EIO;
			break;
		}

		/* get an idle tx request to use */
		req = 0;
		ret = wait_event_interruptible(dev->write_wq,
			((req = iap_req_get(dev, &dev->tx_idle)) || !dev->online));
		if (!req) {
			r = ret;
			break;
		}

		if (count > BULK_BUFFER_SIZE)
			xfer = BULK_BUFFER_SIZE;
		else
			xfer = count;
		if (copy_from_user(req->buf, buf, xfer)) {
			r = -EFAULT;
			break;
		}

		req->length = xfer;
		ret = usb_ep_queue(dev->ep_in, req, GFP_KERNEL);
		if (ret < 0) {
			pr_debug("iap_write: xfer error %d\n", ret);
			r = -EIO;
			break;
		}

		buf += xfer;
		count -= xfer;

		/* zero this so we don't try to free it on error exit */
		req = 0;
	}

	if (req)
		iap_req_put(dev, &dev->tx_idle, req);

	DBG("iap_write returning %d\n", r);
	return r;
}

static int iap_send_event(struct iap_dev *dev, struct iap2_event *event)
{
	struct usb_request *req= NULL;
	int ret;
	int length = event->length;

	DBG("iap_send_event(%d)\n", event->length);

	if (length < 0 || length > INTR_BUFFER_SIZE)
		return -EINVAL;
	if (!dev->online)
		return -ENODEV;

	ret = wait_event_interruptible_timeout(dev->intr_wq,
			(req = iap_req_get(dev, &dev->intr_idle)),
			msecs_to_jiffies(1000));

	if (!req)
	    return -ETIME;

	if (copy_from_user(req->buf, (void __user *)event->data, length)) {
		iap_req_put(dev, &dev->intr_idle, req);
		return -EFAULT;
	}

	req->length = length;
#if 0
	ret = usb_ep_queue(dev->ep_intr, req, GFP_KERNEL);

	if (ret)
		iap_req_put(dev, &dev->intr_idle, req);
#endif
	return ret;
}


static int iap_send_zlp(struct iap_dev *dev)
{
	struct usb_request *req= NULL;
	int ret;
	int length = 0;

	DBG("iap_send_zlp\n");

	if (!dev->online)
		return -ENODEV;

	ret = wait_event_interruptible_timeout(dev->intr_wq,
			(req = iap_req_get(dev, &dev->intr_idle)),
			msecs_to_jiffies(1000));

	if (!req)
	    return -ETIME;

	req->length = length;
#if 0
	ret = usb_ep_queue(dev->ep_intr, req, GFP_KERNEL);

	if (ret)
		iap_req_put(dev, &dev->intr_idle, req);
#endif
	return ret;
}



static long iap_ioctl(struct file *fp, unsigned code, unsigned long value)
{
	struct iap_dev *dev = fp->private_data;
	char *src = NULL;
	int ret;

	DBG("iap_ioctl\n");

   switch (code) {
      case IAP2_SEND_EVT:
         {
            struct iap2_event  event;
            //ret = iap_send_event(dev, &event);
         }
      case IAP2_SEND_ZLP:
         {
            //ret = iap_send_zlp(dev);
         }
   }

	return ret;
}

extern bool entry;
static int iap_open(struct inode *ip, struct file *fp)
{
	printk(KERN_DEBUG "iap_open %d\n", entry);

	if (!entry) {
		printk("iap_open fail...\n");
		return -EBUSY;
	}
	if (atomic_xchg(&_iap_dev->open_excl, 1))
		return -EBUSY;

	_iap_dev->disconnected = 0;
	fp->private_data = _iap_dev;
	return 0;
}

extern void roleswap_notify(void);
static int iap_release(struct inode *ip, struct file *fp)
{
	printk(KERN_DEBUG "iap_release\n");

	WARN_ON(!atomic_xchg(&_iap_dev->open_excl, 0));
	_iap_dev->disconnected = 0;
	roleswap_notify();
	return 0;
}

/* file operations for /dev/usb_accessory */
static const struct file_operations iap_fops = {
	.owner = THIS_MODULE,
	.read = iap_read,
	.write = iap_write,
	.unlocked_ioctl = iap_ioctl,
	.open = iap_open,
	.release = iap_release,
};

static struct miscdevice iap_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "iap2",
	.fops = &iap_fops,
};

static int iap_ctrlrequest(struct usb_composite_dev *cdev,
				const struct usb_ctrlrequest *ctrl)
{
	struct iap_dev	*dev = _iap_dev;
	int	value = -EOPNOTSUPP;
	int offset;
	u8 b_requestType = ctrl->bRequestType;
	u8 b_request = ctrl->bRequest;
	u16	w_index = le16_to_cpu(ctrl->wIndex);
	u16	w_value = le16_to_cpu(ctrl->wValue);
	u16	w_length = le16_to_cpu(ctrl->wLength);
	unsigned long flags;

	DBG("iap_ctrlrequest %02x.%02x v%04x i%04x l%u\n", b_requestType, b_request, w_value, w_index, w_length);

	if (b_requestType == (USB_DIR_OUT | USB_TYPE_VENDOR)) {

	} else if (b_requestType == (USB_DIR_IN | USB_TYPE_VENDOR)) {

	}

	if (value >= 0) {
		cdev->req->zero = 0;
		cdev->req->length = value;
		value = usb_ep_queue(cdev->gadget->ep0, cdev->req, GFP_ATOMIC);
		//if (value < 0)
		//	ERROR(cdev, "%s setup response queue error\n", __func__);
	}

err:
	if (value == -EOPNOTSUPP)
		DBG("unknown class-specific control req %02x.%02x v%04x i%04x l%u\n", ctrl->bRequestType, ctrl->bRequest,
				w_value, w_index, w_length);
	return value;
}

static int
iap_function_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct iap_dev	*dev = usb_func_to_dev(f);
	int			id;
	int			ret;

	printk(KERN_DEBUG "CarPlay iap driver: %s %s\n", __DATE__, __TIME__);
	DBG("iap_function_bind\n");

	dev->start_requested = 0;

	/* allocate interface ID(s) */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	iap_interface_desc.bInterfaceNumber = id;

	/* allocate endpoints */
	ret = create_iap_bulk_endpoints(dev, &iap_fullspeed_in_desc,
			&iap_fullspeed_out_desc/*, &iap_intr_desc*/);
	if (ret)
		return ret;

	/* support high speed hardware */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		iap_highspeed_in_desc.bEndpointAddress =
			iap_fullspeed_in_desc.bEndpointAddress;
		iap_highspeed_out_desc.bEndpointAddress =
			iap_fullspeed_out_desc.bEndpointAddress;
	}

	DBG("IAP2 %s speed %s: IN/%s, OUT/%s\n",
				gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full",
				f->name, dev->ep_in->name, dev->ep_out->name);
	return 0;
}

static void
iap_function_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct iap_dev	*dev = usb_func_to_dev(f);
	struct usb_request *req;
	int i;

	DBG("iap_function_unbind\n");

	while ((req = iap_req_get(dev, &dev->tx_idle)))
		iap_request_free(req, dev->ep_in);
	for (i = 0; i < RX_REQ_MAX; i++)
		iap_request_free(dev->rx_req[i], dev->ep_out);
#if 0
	while ((req = iap_req_get(dev, &dev->intr_idle)))
		iap_request_free(req, dev->ep_intr);
#endif
}

static void iap_start_work(struct work_struct *data)
{
	char *envp[2] = { "IAP2=START", NULL };

	DBG("iap_start_work\n");
	kobject_uevent_env(&iap_device.this_device->kobj, KOBJ_CHANGE, envp);
}


static int iap_function_set_alt(struct usb_function *f,
		unsigned intf, unsigned alt)
{
	struct iap_dev	*dev = usb_func_to_dev(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	int ret;

	DBG("iap_function_set_alt intf: %d alt: %d\n", intf, alt);
	ret = config_ep_by_speed(cdev->gadget, f, dev->ep_in);
	if (ret)
		return ret;

   // ep in
	ret = usb_ep_enable(dev->ep_in);
	if (ret)
		return ret;

	ret = config_ep_by_speed(cdev->gadget, f, dev->ep_out);
	if (ret)
		return ret;

   // ep out
	ret = usb_ep_enable(dev->ep_out);
	if (ret) {
		usb_ep_disable(dev->ep_in);
		return ret;
	}
#if 0
	ret = config_ep_by_speed(cdev->gadget, f, dev->ep_intr);
	if (ret) {
		return ret;
   }

   // ep interrupt
	ret = usb_ep_enable(dev->ep_intr);
	if (ret) {
		usb_ep_disable(dev->ep_out);
		usb_ep_disable(dev->ep_in);
		return ret;
	}
#endif
	dev->online = 1;

	/* readers may be blocked waiting for us to go online */
	wake_up(&dev->read_wq);
	return 0;
}

static void iap_function_disable(struct usb_function *f)
{
	struct iap_dev	*dev = usb_func_to_dev(f);
	struct usb_composite_dev	*cdev = dev->cdev;

	printk(KERN_DEBUG "->%s %d\n", __func__, dev->online);
	dev->rx_done = 1; // dead lock 
	iap_set_disconnected(dev);
	usb_ep_disable(dev->ep_in);
	usb_ep_disable(dev->ep_out);
	//usb_ep_disable(dev->ep_intr);

	/* readers may be blocked waiting for us to go online */
	wake_up(&dev->read_wq);
	wake_up(&dev->write_wq);
}

static int iap_bind_config(struct usb_configuration *c)
{
	struct iap_dev *dev = _iap_dev;
	int ret;

	DBG("iap_bind_config\n");

	/* allocate a string ID for our interface */
	if (iap_string_defs[INTERFACE_STRING_INDEX].id == 0) {
		ret = usb_string_id(c->cdev);
		if (ret < 0)
			return ret;
		iap_string_defs[INTERFACE_STRING_INDEX].id = ret;
		iap_interface_desc.iInterface = ret;
	}

	dev->cdev = c->cdev;
	dev->function.name = "iap2";
	dev->function.strings = iap_strings,
	dev->function.descriptors = fs_iap_descs;
	dev->function.hs_descriptors = hs_iap_descs;
	dev->function.bind = iap_function_bind;
	dev->function.unbind = iap_function_unbind;
	dev->function.set_alt = iap_function_set_alt;
	dev->function.disable = iap_function_disable;

	return usb_add_function(c, &dev->function);
}

static int iap_setup(void)
{
	struct iap_dev *dev;
	int ret;

	DBG("iap_setup\n");

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	memset(dev, 0x0, sizeof( struct iap_dev));

	spin_lock_init(&dev->lock);
	init_waitqueue_head(&dev->read_wq);
	init_waitqueue_head(&dev->write_wq);
	init_waitqueue_head(&dev->intr_wq);
	atomic_set(&dev->open_excl, 0);
	INIT_LIST_HEAD(&dev->tx_idle);
	INIT_LIST_HEAD(&dev->intr_idle);
	//INIT_DELAYED_WORK(&dev->start_work, iap_start_work);

	_iap_dev = dev;

	ret = misc_register(&iap_device);
	if (ret)
		goto err;

	return 0;

err:
	kfree(dev);
	pr_err("USB IAP2 gadget driver failed to initialize\n");
	return ret;
}

static void iap_disconnect(void)
{
	DBG("iap_disconnect\n");
}

static void iap_cleanup(void)
{
	DBG("iap_cleanup\n");
	misc_deregister(&iap_device);
	kfree(_iap_dev);
	_iap_dev = NULL;
}
