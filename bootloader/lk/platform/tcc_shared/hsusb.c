/*
 * Copyright (c) 2010 Telechips, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the 
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <string.h>
#include <stdlib.h>
#include <debug.h>
#include <platform/iomap.h>
#include <platform/irqs.h>
#include <platform/interrupts.h>
#include <kernel/thread.h>
#include <reg.h>

#include <dev/udc.h>

#include <platform/reg_physical.h>
#include <platform/globals.h>

#include "usb/usb_defs.h"
#include "usb/usb_device.h"
#include "usb/usb_manager.h"
#include "usb/otgregs.h"
#include "usb/usbphy.h"
#include "usb/otgcore.h"
#include "usb/otgdev_io.h"

#if 1
#define DBG(x...) do {} while(0)
#else
#define DBG(x...) dprintf(INFO, x)
#endif

/* bits used in all the endpoint status registers */
#define EPT_TX(n) (1 << ((n) + 16))
#define EPT_RX(n) (1 << (n))

#define QUEUE_STATE_IDLE			0
#define QUEUE_STATE_ACTIVATE			1
#define QUEUE_STATE_DONE			2

struct ept_queue_head 
{
	unsigned int xfer_len;
	unsigned int acutal_len;
	unsigned int buf_size;
        unsigned char *buf;
	volatile unsigned char state;
};

struct usb_request {
	struct udc_request req;
	struct ept_queue_item *item;
};

struct udc_endpoint
{
	struct udc_endpoint *next;
	unsigned bit;
	struct ept_queue_head *head;
	struct usb_request *req;
	unsigned char num;
	unsigned char in;
	unsigned short maxpkt;
};

struct udc_endpoint *ept_list = 0;
struct ept_queue_head *epts = 0;

static unsigned ept_alloc_table = EPT_TX(0) | EPT_RX(0);

static struct udc_endpoint *ep0in, *ep0out;
static struct udc_request *ep0req;

static int usb_online = 0;
static int usb_highspeed = 0;

static struct udc_device *the_device;
static struct udc_gadget *the_gadget;

static inline struct udc_endpoint *udc_get_endpoint(int num, int in)
{
	struct udc_endpoint *ept;

	for (ept = ept_list; ept; ept = ept->next) {
		if ((ept->num == num) && (ept->in == in)) {
			return ept;
		}
	}
	return 0;
}

static struct udc_endpoint *_udc_endpoint_alloc(unsigned num, unsigned in, unsigned max_pkt)
{
	struct udc_endpoint *ept;

	ept = malloc(sizeof(*ept));

	ept->maxpkt = max_pkt;
	ept->num = num;
	ept->in = !!in;
	ept->req = 0;

	if (ept->in) {
		ept->bit = EPT_TX(ept->num);
	} else {
		ept->bit = EPT_RX(ept->num);
	}

	ept->head = epts + (num * 2) + (ept->in);
	ept->next = ept_list;
	ept_list = ept;

	DBG("ept%d %s @%p/%p max=%d bit=%x\n",
	    num, in ? "in":"out", ept, ept->head, max_pkt, ept->bit);

	return ept;
}

struct udc_endpoint *udc_endpoint_alloc(unsigned type, unsigned maxpkt)
{
	struct udc_endpoint *ept;
	unsigned n;
	unsigned in;

	if (type == UDC_TYPE_BULK_IN) {
		in = 1;
	} else if (type == UDC_TYPE_BULK_OUT) {
		in = 0;
	} else {
		return 0;
	}

	for (n = 1; n < 16; n++) {
		unsigned bit = in ? EPT_TX(n) : EPT_RX(n);
		if (ept_alloc_table & bit)
			continue;
		ept = _udc_endpoint_alloc(n, in, maxpkt);
		if (ept)
			ept_alloc_table |= bit;
		return ept;
	}
	return 0;
}

void udc_endpoint_free(struct udc_endpoint *ept)
{
	/* TODO */
}

struct udc_request *udc_request_alloc(void)
{
	struct usb_request *req;
	req = malloc(sizeof(*req));
	req->req.buf = 0;
	req->req.length = 0;
	req->item = dma_alloc(32, 32);
	return &req->req;
}

void udc_request_free(struct udc_request *req)
{
	free(req);
}

int udc_request_queue(struct udc_endpoint *ept, struct udc_request *_req)
{
	int actual;

	if (ept->in) {
		actual = FASTBOOT_SendData(_req->buf, _req->length);
	} else {
		actual = FASTBOOT_ReceiveData(_req->buf, _req->length);
	}
	_req->complete(_req, actual, 0);
	if (actual > 0) {
		return 0;
	} else {
		return -1;
	}
}

int udc_register_gadget(struct udc_gadget *gadget)
{
	if (the_gadget) {
		dprintf(CRITICAL, "only one gadget supported\n");
		return -1;
	}
	the_gadget = gadget;
	return 0;
}

int udc_init(struct udc_device *dev) 
{
	USB_Init(MODE_FASTBOOT);
	USBDEV_SetClass(USBDEV_CLASS_FASTBOOT);

	the_device = dev;
	return 0;
}

int udc_start(void)
{
	DBG("udc_start()\n");

	if (!the_device) {
		dprintf(CRITICAL, "udc cannot start before init\n");
		return -1;
	}
	if (!the_gadget) {
		dprintf(CRITICAL, "udc has no gadget registered\n");
		return -1;
	}

	/* Set device mode */
	USB_DEVICE_On();

#if defined(PLATFORM_TCC92XX)
	BITSET(HwPIC->SEL1, HwINT1_UOTG);
	BITCLR(HwPIC->POL1, HwINT1_UOTG);
	BITSET(HwPIC->MODE1, HwINT1_UOTG);
#endif

	return 0;
}

int udc_stop(void)
{
#if defined(USBPHY_INCLUDE)
	USBPHY_SetMode(USBPHY_MODE_RESET);
#endif
	return 0;
}

// XXX
void udc_notify(unsigned event)
{
	the_gadget->notify(the_gadget, UDC_EVENT_ONLINE);
}
