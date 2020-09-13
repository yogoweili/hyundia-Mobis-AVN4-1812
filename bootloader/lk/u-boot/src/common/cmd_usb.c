/*
 * (C) Copyright 2001
 * Denis Peter, MPL AG Switzerland
 *
 * Most of this source has been derived from the Linux USB
 * project.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <asm/byteorder.h>
#include <asm/unaligned.h>
#include <part.h>
#include <usb.h>
#include <asm/arch/fwdn/Disk.h>
#include <asm/arch/sfl.h>
#include <asm/arch/cipher/cipher.h>

#ifdef DBG_FWUPDATE_DISPLAY
#define fbout(fmt, args...) fbprintf(fmt, ##args)
#else
#define fbout(fmt, args...) while(0){}
#endif

#define update_debug(fmt, args...)\
    printf(fmt, ##args);\
    fbout(fmt, ##args);\

#ifdef CONFIG_USB_STORAGE
static int usb_stor_curr_dev = -1; /* current device */
#endif
#ifdef CONFIG_USB_HOST_ETHER
static int usb_ether_curr_dev = -1; /* current ethernet device */
#endif

/* some display routines (info command) */
static char *usb_get_class_desc(unsigned char dclass)
{
	switch (dclass) {
	case USB_CLASS_PER_INTERFACE:
		return "See Interface";
	case USB_CLASS_AUDIO:
		return "Audio";
	case USB_CLASS_COMM:
		return "Communication";
	case USB_CLASS_HID:
		return "Human Interface";
	case USB_CLASS_PRINTER:
		return "Printer";
	case USB_CLASS_MASS_STORAGE:
		return "Mass Storage";
	case USB_CLASS_HUB:
		return "Hub";
	case USB_CLASS_DATA:
		return "CDC Data";
	case USB_CLASS_VENDOR_SPEC:
		return "Vendor specific";
	default:
		return "";
	}
}

static void usb_display_class_sub(unsigned char dclass, unsigned char subclass,
				  unsigned char proto)
{
	switch (dclass) {
	case USB_CLASS_PER_INTERFACE:
		printf("See Interface");
		break;
	case USB_CLASS_HID:
		printf("Human Interface, Subclass: ");
		switch (subclass) {
		case USB_SUB_HID_NONE:
			printf("None");
			break;
		case USB_SUB_HID_BOOT:
			printf("Boot ");
			switch (proto) {
			case USB_PROT_HID_NONE:
				printf("None");
				break;
			case USB_PROT_HID_KEYBOARD:
				printf("Keyboard");
				break;
			case USB_PROT_HID_MOUSE:
				printf("Mouse");
				break;
			default:
				printf("reserved");
				break;
			}
			break;
		default:
			printf("reserved");
			break;
		}
		break;
	case USB_CLASS_MASS_STORAGE:
		printf("Mass Storage, ");
		switch (subclass) {
		case US_SC_RBC:
			printf("RBC ");
			break;
		case US_SC_8020:
			printf("SFF-8020i (ATAPI)");
			break;
		case US_SC_QIC:
			printf("QIC-157 (Tape)");
			break;
		case US_SC_UFI:
			printf("UFI");
			break;
		case US_SC_8070:
			printf("SFF-8070");
			break;
		case US_SC_SCSI:
			printf("Transp. SCSI");
			break;
		default:
			printf("reserved");
			break;
		}
		printf(", ");
		switch (proto) {
		case US_PR_CB:
			printf("Command/Bulk");
			break;
		case US_PR_CBI:
			printf("Command/Bulk/Int");
			break;
		case US_PR_BULK:
			printf("Bulk only");
			break;
		default:
			printf("reserved");
			break;
		}
		break;
	default:
		printf("%s", usb_get_class_desc(dclass));
		break;
	}
}

static void usb_display_string(struct usb_device *dev, int index)
{
	ALLOC_CACHE_ALIGN_BUFFER(char, buffer, 256);

	if (index != 0) {
		if (usb_string(dev, index, &buffer[0], 256) > 0)
			printf("String: \"%s\"", buffer);
	}
}

static void usb_display_desc(struct usb_device *dev)
{
	if (dev->descriptor.bDescriptorType == USB_DT_DEVICE) {
		printf("%d: %s,  USB Revision %x.%x\n", dev->devnum,
		usb_get_class_desc(dev->config.if_desc[0].desc.bInterfaceClass),
				   (dev->descriptor.bcdUSB>>8) & 0xff,
				   dev->descriptor.bcdUSB & 0xff);

		if (strlen(dev->mf) || strlen(dev->prod) ||
		    strlen(dev->serial))
			printf(" - %s %s %s\n", dev->mf, dev->prod,
				dev->serial);
		if (dev->descriptor.bDeviceClass) {
			printf(" - Class: ");
			usb_display_class_sub(dev->descriptor.bDeviceClass,
					      dev->descriptor.bDeviceSubClass,
					      dev->descriptor.bDeviceProtocol);
			printf("\n");
		} else {
			printf(" - Class: (from Interface) %s\n",
			       usb_get_class_desc(
				dev->config.if_desc[0].desc.bInterfaceClass));
		}
		printf(" - PacketSize: %d  Configurations: %d\n",
			dev->descriptor.bMaxPacketSize0,
			dev->descriptor.bNumConfigurations);
		printf(" - Vendor: 0x%04x  Product 0x%04x Version %d.%d\n",
			dev->descriptor.idVendor, dev->descriptor.idProduct,
			(dev->descriptor.bcdDevice>>8) & 0xff,
			dev->descriptor.bcdDevice & 0xff);
	}

}

static void usb_display_conf_desc(struct usb_config_descriptor *config,
				  struct usb_device *dev)
{
	printf("   Configuration: %d\n", config->bConfigurationValue);
	printf("   - Interfaces: %d %s%s%dmA\n", config->bNumInterfaces,
	       (config->bmAttributes & 0x40) ? "Self Powered " : "Bus Powered ",
	       (config->bmAttributes & 0x20) ? "Remote Wakeup " : "",
		config->bMaxPower*2);
	if (config->iConfiguration) {
		printf("   - ");
		usb_display_string(dev, config->iConfiguration);
		printf("\n");
	}
}

static void usb_display_if_desc(struct usb_interface_descriptor *ifdesc,
				struct usb_device *dev)
{
	printf("     Interface: %d\n", ifdesc->bInterfaceNumber);
	printf("     - Alternate Setting %d, Endpoints: %d\n",
		ifdesc->bAlternateSetting, ifdesc->bNumEndpoints);
	printf("     - Class ");
	usb_display_class_sub(ifdesc->bInterfaceClass,
		ifdesc->bInterfaceSubClass, ifdesc->bInterfaceProtocol);
	printf("\n");
	if (ifdesc->iInterface) {
		printf("     - ");
		usb_display_string(dev, ifdesc->iInterface);
		printf("\n");
	}
}

static void usb_display_ep_desc(struct usb_endpoint_descriptor *epdesc)
{
	printf("     - Endpoint %d %s ", epdesc->bEndpointAddress & 0xf,
		(epdesc->bEndpointAddress & 0x80) ? "In" : "Out");
	switch ((epdesc->bmAttributes & 0x03)) {
	case 0:
		printf("Control");
		break;
	case 1:
		printf("Isochronous");
		break;
	case 2:
		printf("Bulk");
		break;
	case 3:
		printf("Interrupt");
		break;
	}
	printf(" MaxPacket %d", get_unaligned(&epdesc->wMaxPacketSize));
	if ((epdesc->bmAttributes & 0x03) == 0x3)
		printf(" Interval %dms", epdesc->bInterval);
	printf("\n");
}

/* main routine to diasplay the configs, interfaces and endpoints */
static void usb_display_config(struct usb_device *dev)
{
	struct usb_config *config;
	struct usb_interface *ifdesc;
	struct usb_endpoint_descriptor *epdesc;
	int i, ii;

	config = &dev->config;
	usb_display_conf_desc(&config->desc, dev);
	for (i = 0; i < config->no_of_if; i++) {
		ifdesc = &config->if_desc[i];
		usb_display_if_desc(&ifdesc->desc, dev);
		for (ii = 0; ii < ifdesc->no_of_ep; ii++) {
			epdesc = &ifdesc->ep_desc[ii];
			usb_display_ep_desc(epdesc);
		}
	}
	printf("\n");
}

static struct usb_device *usb_find_device(int devnum)
{
	struct usb_device *dev;
	int d;

	for (d = 0; d < USB_MAX_DEVICE; d++) {
		dev = usb_get_dev_index(d);
		if (dev == NULL)
			return NULL;
		if (dev->devnum == devnum)
			return dev;
	}

	return NULL;
}

static inline char *portspeed(int speed)
{
	char *speed_str;

	switch (speed) {
	case USB_SPEED_SUPER:
		speed_str = "5 Gb/s";
		break;
	case USB_SPEED_HIGH:
		speed_str = "480 Mb/s";
		break;
	case USB_SPEED_LOW:
		speed_str = "1.5 Mb/s";
		break;
	default:
		speed_str = "12 Mb/s";
		break;
	}

	return speed_str;
}

/* shows the device tree recursively */
static void usb_show_tree_graph(struct usb_device *dev, char *pre)
{
	int i, index;
	int has_child, last_child;

	index = strlen(pre);
	printf(" %s", pre);
	/* check if the device has connected children */
	has_child = 0;
	for (i = 0; i < dev->maxchild; i++) {
		if (dev->children[i] != NULL)
			has_child = 1;
	}
	/* check if we are the last one */
	last_child = 1;
	if (dev->parent != NULL) {
		for (i = 0; i < dev->parent->maxchild; i++) {
			/* search for children */
			if (dev->parent->children[i] == dev) {
				/* found our pointer, see if we have a
				 * little sister
				 */
				while (i++ < dev->parent->maxchild) {
					if (dev->parent->children[i] != NULL) {
						/* found a sister */
						last_child = 0;
						break;
					} /* if */
				} /* while */
			} /* device found */
		} /* for all children of the parent */
		printf("\b+-");
		/* correct last child */
		if (last_child)
			pre[index-1] = ' ';
	} /* if not root hub */
	else
		printf(" ");
	printf("%d ", dev->devnum);
	pre[index++] = ' ';
	pre[index++] = has_child ? '|' : ' ';
	pre[index] = 0;
	printf(" %s (%s, %dmA)\n", usb_get_class_desc(
					dev->config.if_desc[0].desc.bInterfaceClass),
					portspeed(dev->speed),
					dev->config.desc.bMaxPower * 2);
	if (strlen(dev->mf) || strlen(dev->prod) || strlen(dev->serial))
		printf(" %s  %s %s %s\n", pre, dev->mf, dev->prod, dev->serial);
	printf(" %s\n", pre);
	if (dev->maxchild > 0) {
		for (i = 0; i < dev->maxchild; i++) {
			if (dev->children[i] != NULL) {
				usb_show_tree_graph(dev->children[i], pre);
				pre[index] = 0;
			}
		}
	}
}

/* main routine for the tree command */
static void usb_show_tree(struct usb_device *dev)
{
	char preamble[32];

	memset(preamble, 0, 32);
	usb_show_tree_graph(dev, &preamble[0]);
}

static int usb_test(struct usb_device *dev, int port, char* arg)
{
	int mode;

	if (port > dev->maxchild) {
		printf("Device is no hub or does not have %d ports.\n", port);
		return 1;
	}

	switch (arg[0]) {
	case 'J':
	case 'j':
		printf("Setting Test_J mode");
		mode = USB_TEST_MODE_J;
		break;
	case 'K':
	case 'k':
		printf("Setting Test_K mode");
		mode = USB_TEST_MODE_K;
		break;
	case 'S':
	case 's':
		printf("Setting Test_SE0_NAK mode");
		mode = USB_TEST_MODE_SE0_NAK;
		break;
	case 'P':
	case 'p':
		printf("Setting Test_Packet mode");
		mode = USB_TEST_MODE_PACKET;
		break;
	case 'F':
	case 'f':
		printf("Setting Test_Force_Enable mode");
		mode = USB_TEST_MODE_FORCE_ENABLE;
		break;
	default:
		printf("Unrecognized test mode: %s\nAvailable modes: "
		       "J, K, S[E0_NAK], P[acket], F[orce_Enable]\n", arg);
		return 1;
	}

	if (port)
		printf(" on downstream facing port %d...\n", port);
	else
		printf(" on upstream facing port...\n");

	if (usb_control_msg(dev, usb_sndctrlpipe(dev, 0), USB_REQ_SET_FEATURE,
			    port ? USB_RT_PORT : USB_RECIP_DEVICE,
			    port ? USB_PORT_FEAT_TEST : USB_FEAT_TEST,
			    (mode << 8) | port,
			    NULL, 0, USB_CNTL_TIMEOUT) == -1) {
		printf("Error during SET_FEATURE.\n");
		return 1;
	} else {
		printf("Test mode successfully set. Use 'usb start' "
		       "to return to normal operation.\n");
		return 0;
	}
}

/******************************************************************************
 * usb boot command intepreter. Derived from diskboot
 */
#ifdef CONFIG_USB_STORAGE
static int do_usbboot(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return common_diskboot(cmdtp, "usb", argc, argv);
}
#endif /* CONFIG_USB_STORAGE */

/******************************************************************************
 * usb command intepreter
 */
int do_usb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{

	int i;
	struct usb_device *dev = NULL;
	extern char usb_started;
#ifdef CONFIG_USB_STORAGE
	block_dev_desc_t *stor_dev;
#endif

	if (argc < 2)
		return CMD_RET_USAGE;

	if ((strncmp(argv[1], "reset", 5) == 0) ||
		 (strncmp(argv[1], "start", 5) == 0)) {
		bootstage_mark_name(BOOTSTAGE_ID_USB_START, "usb_start");
		usb_stop();
		printf("\x1b[1;36m(Re)start USB...\x1b[1;0m\n");
		if (usb_init() >= 0) {
#ifdef CONFIG_USB_STORAGE
			/* try to recognize storage devices immediately */
			usb_stor_curr_dev = usb_stor_scan(1);
#endif
#ifdef CONFIG_USB_HOST_ETHER
			/* try to recognize ethernet devices immediately */
			usb_ether_curr_dev = usb_host_eth_scan(1);
#endif
#ifdef CONFIG_USB_KEYBOARD
			drv_usb_kbd_init();
#endif
		}
		return 0;
	}
	if (strncmp(argv[1], "stop", 4) == 0) {
#ifdef CONFIG_USB_KEYBOARD
		if (argc == 2) {
			if (usb_kbd_deregister() != 0) {
				printf("USB not stopped: usbkbd still"
					" using USB\n");
				return 1;
			}
		} else {
			/* forced stop, switch console in to serial */
			console_assign(stdin, "serial");
			usb_kbd_deregister();
		}
#endif
		printf("stopping USB..\n");
		usb_stop();
		return 0;
	}
	if (!usb_started) {
		printf("USB is stopped. Please issue 'usb start' first.\n");
		return 1;
	}
	if (strncmp(argv[1], "tree", 4) == 0) {
		puts("USB device tree:\n");
		for (i = 0; i < USB_MAX_DEVICE; i++) {
			dev = usb_get_dev_index(i);
			if (dev == NULL)
				break;
			if (dev->parent == NULL)
				usb_show_tree(dev);
		}
		return 0;
	}
	if (strncmp(argv[1], "inf", 3) == 0) {
		int d;
		if (argc == 2) {
			for (d = 0; d < USB_MAX_DEVICE; d++) {
				dev = usb_get_dev_index(d);
				if (dev == NULL)
					break;
				usb_display_desc(dev);
				usb_display_config(dev);
			}
			return 0;
		} else {
			i = simple_strtoul(argv[2], NULL, 10);
			printf("config for device %d\n", i);
			dev = usb_find_device(i);
			if (dev == NULL) {
				printf("*** No device available ***\n");
				return 0;
			} else {
				usb_display_desc(dev);
				usb_display_config(dev);
			}
		}
		return 0;
	}
	if (strncmp(argv[1], "test", 4) == 0) {
		if (argc < 5)
			return CMD_RET_USAGE;
		i = simple_strtoul(argv[2], NULL, 10);
		dev = usb_find_device(i);
		if (dev == NULL) {
			printf("Device %d does not exist.\n", i);
			return 1;
		}
		i = simple_strtoul(argv[3], NULL, 10);
		return usb_test(dev, i, argv[4]);
	}
#ifdef CONFIG_USB_STORAGE
	if (strncmp(argv[1], "stor", 4) == 0)
		return usb_stor_info();

	if (strncmp(argv[1], "part", 4) == 0) {
		int devno, ok = 0;
		if (argc == 2) {
			for (devno = 0; ; ++devno) {
				stor_dev = usb_stor_get_dev(devno);
				if (stor_dev == NULL)
					break;
				if (stor_dev->type != DEV_TYPE_UNKNOWN) {
					ok++;
					if (devno)
						printf("\n");
					debug("print_part of %x\n", devno);
					print_part(stor_dev);
				}
			}
		} else {
			devno = simple_strtoul(argv[2], NULL, 16);
			stor_dev = usb_stor_get_dev(devno);
			if (stor_dev != NULL &&
			    stor_dev->type != DEV_TYPE_UNKNOWN) {
				ok++;
				debug("print_part of %x\n", devno);
				print_part(stor_dev);
			}
		}
		if (!ok) {
			printf("\nno USB devices available\n");
			return 1;
		}
		return 0;
	}
	if (strcmp(argv[1], "read") == 0) {
		if (usb_stor_curr_dev < 0) {
			printf("no current device selected\n");
			return 1;
		}
		if (argc == 5) {
			unsigned long addr = simple_strtoul(argv[2], NULL, 16);
			unsigned long blk  = simple_strtoul(argv[3], NULL, 16);
			unsigned long cnt  = simple_strtoul(argv[4], NULL, 16);
			unsigned long n;
			printf("\nUSB read: device %d block # %ld, count %ld"
				" ... ", usb_stor_curr_dev, blk, cnt);
			stor_dev = usb_stor_get_dev(usb_stor_curr_dev);
			n = stor_dev->block_read(usb_stor_curr_dev, blk, cnt,
						 (ulong *)addr);
			printf("%ld blocks read: %s\n", n,
				(n == cnt) ? "OK" : "ERROR");
			if (n == cnt)
				return 0;
			return 1;
		}
	}
	if (strcmp(argv[1], "write") == 0) {
		if (usb_stor_curr_dev < 0) {
			printf("no current device selected\n");
			return 1;
		}
		if (argc == 5) {
			unsigned long addr = simple_strtoul(argv[2], NULL, 16);
			unsigned long blk  = simple_strtoul(argv[3], NULL, 16);
			unsigned long cnt  = simple_strtoul(argv[4], NULL, 16);
			unsigned long n;
			printf("\nUSB write: device %d block # %ld, count %ld"
				" ... ", usb_stor_curr_dev, blk, cnt);
			stor_dev = usb_stor_get_dev(usb_stor_curr_dev);
			n = stor_dev->block_write(usb_stor_curr_dev, blk, cnt,
						(ulong *)addr);
			printf("%ld blocks write: %s\n", n,
				(n == cnt) ? "OK" : "ERROR");
			if (n == cnt)
				return 0;
			return 1;
		}
	}
	if (strncmp(argv[1], "dev", 3) == 0) {
		if (argc == 3) {
			int dev = (int)simple_strtoul(argv[2], NULL, 10);
			printf("\nUSB device %d: ", dev);
			stor_dev = usb_stor_get_dev(dev);
			if (stor_dev == NULL) {
				printf("unknown device\n");
				return 1;
			}
			printf("\n    Device %d: ", dev);
			dev_print(stor_dev);
			if (stor_dev->type == DEV_TYPE_UNKNOWN)
				return 1;
			usb_stor_curr_dev = dev;
			printf("... is now current device\n");
			return 0;
		} else {
			printf("\nUSB device %d: ", usb_stor_curr_dev);
			stor_dev = usb_stor_get_dev(usb_stor_curr_dev);
			dev_print(stor_dev);
			if (stor_dev->type == DEV_TYPE_UNKNOWN)
				return 1;
			return 0;
		}
		return 0;
	}
#endif /* CONFIG_USB_STORAGE */
	return CMD_RET_USAGE;
}

#ifdef CONFIG_USB_UPDATE
#include <fs.h>

#define buf_size 102400

int usb_fat_file_read(const char *filename, unsigned long addr, unsigned long bytes, unsigned long pos)
{
	int len_read;

	if (fs_set_blk_dev("usb", "0:1", 1))
		return -1;

	len_read = fs_read(filename, addr, pos, bytes);
	if (len_read <= 0)
		return -1;

	return len_read;
}

int usb_fat_read(unsigned long addr, unsigned long bytes, unsigned long pos)
{
	const char *filename;
	int len_read;

	if (fs_set_blk_dev("usb", "0:1", 1))
		return -1;

	filename = UPDATE_FILE_NAME;

	len_read = fs_read(filename, addr, pos, bytes);
	if (len_read <= 0)
		return -1;

	return len_read;
}

int usb_fat_read_bootloader(unsigned long addr, unsigned long bytes, unsigned long pos)
{
	const char *filename;
	int len_read;

	if (fs_set_blk_dev("usb", "0:1", 1))
		return -1;

	filename = UPDATE_BOOTLOADER_NAME;

	len_read = fs_read(filename, addr, pos, bytes);
	if (len_read <= 0)
		return -1;

	return len_read;
}

#define OVER_FLOW 		1
#define NORM_COMPLETE 	0

static int write_DiskImage_to_eMMC(char* fileaddr, int imageSize, unsigned int usb_offset)
{
	char* cur_point = fileaddr;
	unsigned long long partSize, destAddress;
	unsigned int offset = 0;
	unsigned int file_size = getenv_hex("mobis_fai_size", 0);
	int ret = NORM_COMPLETE;
	static int i;

	while(imageSize > 0)
	{
		destAddress = 0;
		partSize = 0;

		memcpy(&destAddress, cur_point, 8);
		cur_point+=8;
		memcpy(&partSize, cur_point, 8);
		cur_point+=8;

		/*partition data insufficient*/
		if( (offset+partSize+16)>UPDATE_BUFFER_SIZE ) {
			if(!offset){	//first data overflow
				ret = OVER_FLOW;
				break;
			}
			return offset;
		}

		imageSize -= (partSize+16);

		if(partSize == 0){
			offset+=16;
			continue;
		}

		if(DISK_WriteSector(DISK_DEVICE_TRIFLASH, 0,
			BYTE_TO_SECTOR(destAddress), BYTE_TO_SECTOR(partSize), cur_point))
		{
			update_debug("eMMC write fail\n");
			return -1;
		}

		if(++i % 1000)
			update_debug("\r%u/%u bytes",usb_offset+offset, file_size);

		cur_point+=partSize;
		offset+=(partSize+16);
	}

	if(ret == OVER_FLOW)//offset == 0
	{
		usb_offset+=16;

		while(partSize>0){
			unsigned int unit_size = (partSize > UPDATE_BUFFER_SIZE)?UPDATE_BUFFER_SIZE:partSize;

			usb_fat_read((unsigned long)fileaddr,unit_size,usb_offset);

			if(DISK_WriteSector(DISK_DEVICE_TRIFLASH, 0,
				BYTE_TO_SECTOR(destAddress), BYTE_TO_SECTOR(unit_size), fileaddr))
			{
				update_debug("eMMC write fail\n");
				return -1;
			}

			if(++i % 1000)
				update_debug("\r%u/%u bytes",usb_offset, file_size);

			offset+=unit_size;
			usb_offset+=unit_size;
			partSize-=unit_size;
			destAddress+=unit_size;
		}
		return offset+16;
	}
	ret = offset;

	return ret;
}

/**
** @author sjpark@cleinsoft
** @date 2014/11/05
** add function to display RGB color on the screen
***/
char *fb_addr_for_bootloader = NULL;
static void update_screen(unsigned int rgb)
{
    int i;
	char *fb_addr = (char *)simple_strtoul(FB_BASE_ADDR, NULL, 16);
	fb_addr_for_bootloader = fb_addr;

    //printf("fb_addr_for_bootloader(0x%08X)\n", (unsigned int)fb_addr_for_bootloader);
    for(i=0; i<(DISP_WIDTH*DISP_HEIGHT); i++){
        *(unsigned int*)fb_addr_for_bootloader = rgb;
        fb_addr_for_bootloader += FB_BPP/8;
    }
    //printf("fb_addr_for_bootloader(0x%08X)\n", (unsigned int)fb_addr_for_bootloader);
}

char *buf_addr_for_bootloader = NULL;
static int do_usbupdate(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char * usb_argv[5];
	//char * fs_argv[5];
	char * buf_addr = (char *)simple_strtoul(argv[1], NULL, 16);
	//ulong loadaddr;
	unsigned int file_size;
	unsigned int remain_data;
	unsigned int headersize = 0;
	unsigned int offset = 0;
	unsigned long time;
    int err = 0;
	int bootloader_size = UPDATE_BOOTLOADER_DEFAULT_SIZE;
	buf_addr_for_bootloader = buf_addr;
	unsigned long long dummy = 0;
	update_debug("bootloader buffer address(0x%08X)\n", (unsigned int)buf_addr_for_bootloader);
	update_debug("\n=== USB update start ===\n");

	/*usb initailization*/
	usb_argv[0] = "usb";
	usb_argv[1] = "start";

	if(do_usb(NULL, 0, 2, usb_argv)){
		update_debug("USB init fail!\n");
		return -1;
	}

	/*get tag strings [0:7]*/
	update_debug("\nCheck update file valid\n");
	usb_fat_read((unsigned long )buf_addr, 8, offset);
	if (strncmp(buf_addr, "[HEADER]", 8)){
		buf_addr[8] = '\0';
		update_debug("Tag Reading Error! tag = %s\n", buf_addr);
		err = -1;
		goto bootloader_update;
	}
	offset += 8;

	/*get header size [8:11]*/
	usb_fat_read((unsigned long )buf_addr, 4, offset);
	memcpy(&headersize, buf_addr, 4);
	update_debug("Headersize = 0x%x\n", headersize);
	if(headersize != 0x60)
	{
		update_debug("Headersize is wrong! size = 0x%x\n",headersize);
		goto bootloader_update;
	}

	/*check the area name*/
	offset = 0x30;
	usb_fat_read((unsigned long)buf_addr, 7, offset);
	if (strncmp(buf_addr, "SD Data",7)){
		buf_addr[7] = '\0';
		update_debug("Area name is %s!\n",buf_addr);

		goto bootloader_update;
	}

	file_size = getenv_hex("mobis_fai_size", 0);
	update_debug("%s size = %u\n",UPDATE_FILE_NAME,file_size);

	offset = 0x60;
	remain_data = file_size-headersize;

	time = get_timer(0);

	/**
	 ** @author legolamp@cleinsoft
	 ** @date 2014/11/04
	 ** remove all emmc area for mobis.fai update
	 ***/
	if(remain_data){
		if( !erase_emmc(0, 0, 2) ) {
			update_debug("Erase eMMC finished!\n");
		} else {
			update_debug("Erase eMMC failed...\n");
			goto bootloader_update;
		}
	}

	update_debug("eMMC update start !!!\n");
	while(remain_data>0)
	{
		unsigned int writen_size = 0;
		unsigned int partial_size = (remain_data>UPDATE_BUFFER_SIZE)?UPDATE_BUFFER_SIZE:remain_data;

		if(usb_fat_read((unsigned long)buf_addr,partial_size,offset)<0){
			update_debug("FAT read error!\n");
			goto bootloader_update;
		}

		writen_size = write_DiskImage_to_eMMC(buf_addr, partial_size,offset);
		if(writen_size == -1){
			update_debug("\x1b[1;31mfunc: %s, line: %d  \x1b[0m\n",__func__,__LINE__);
			goto bootloader_update;
		}

		offset		+=	writen_size;
		remain_data	-=	writen_size;
		update_debug("\r%u/%u bytes",offset, file_size);
	}
	time = get_timer(time);
	update_debug("\r%u/%u bytes\n",file_size, file_size);
	update_debug("%d bytes update in %lu ms", file_size, time);
	if (time > 0) {
		puts(" (");
		print_size(file_size / time * 1000, "/s");
		puts(")");
	}
	puts("\n");
	update_debug("\n");
bootloader_update:
	update_debug("[BOOTLOADER UPDATE] Initial bootloader size : %d Byte \n", bootloader_size);
	bootloader_size = usb_fat_read_bootloader((unsigned long)buf_addr_for_bootloader, bootloader_size, 0);
	update_debug("[BOOTLOADER UPDATE] 1st:check bootloader size : %d Byte\n", bootloader_size);
	bootloader_size = usb_fat_read_bootloader((unsigned long)buf_addr_for_bootloader, bootloader_size, 0);
	update_debug("[BOOTLOADER UPDATE] 2nd:check bootloader size : %d Byte\n", bootloader_size);

	if (bootloader_size > 0)
		tcc_write("bootloader", dummy, bootloader_size, buf_addr_for_bootloader);
	else
	{
		update_debug("[BOOTLOADER UPDATE] There is no %s file.\n", UPDATE_BOOTLOADER_NAME);
		if(err < 0)
			return -1;
	}

	update_debug("=== USB update complete ===\n\n");

	return 0;
}

int usbupdate(void)
{
    char * argv[5];
	int ret = 0;
    //dcache_enable();
    argv[1] = UPDATE_BUFFER_ADDR;

	/**
	 ** @author sjpark@cleinsoft
	 ** @date 2014/11/05
	 ** display proper color to check update progress
	 ***/
    if(do_usbupdate((cmd_tbl_t *)NULL,0,0, argv) < 0){

		ret = -1;
		return -1;
    }else{
	}

	/**
	 ** @author arc@cleinsoft
	 ** @date 2015/01/06
	 ** notification for update succeeded
	 ***/
	if( 0==ret )
	{
		update_debug("Please disconnect USB connection.\n");
	}

	/**
	 ** @author legolamp@cleinsoft
	 ** @date 2014/11/06
	 ** after uboot update and reset cpu
	 ***/
	do_reset(NULL, 0, 0, NULL);

    return ret;

}

unsigned char checksum_img(unsigned char *ucp, unsigned int sz)
{
	unsigned char chk = 0;
	while(sz-- != 0)
		chk -= *ucp++;

	return chk;
}

int decrypt_and_partition_write(const char *filename)
{
	unsigned char aes_key[AES_KEY_LEN];
	unsigned char aes_iv[AES_IV_LEN];
	unsigned char in_buff[AES_BLOCK_SIZE];
	unsigned char out_buff[AES_BLOCK_SIZE];
	unsigned char br_buff[512];
	unsigned char test_buff[AES_BLOCK_SIZE];

	unsigned int max_cnt, max_rem;
	unsigned int dec_size, enc_size, rem_size, partial_size;	  // encrypt size
	int gap;

	unsigned int offset = 0;    //file offset
    unsigned int poffset = 0;   //partition offset
	unsigned long long destAddr = 0;

	stCIPHER_ALGORITHM	al;
	stCIPHER_KEY		key;
	stCIPHER_VECTOR 	iv;
	stCIPHER_ENCRYPTION encrypt;
	stCIPHER_DECRYPTION decrypt;

	int i, cnt, tcnt, pcnt;
	int ret;

	char *buf_addr = (char *)simple_strtoul(UPDATE_BUFFER_ADDR, NULL, 16);

	// cipher init & setting
	al.uDmaMode = TCC_CIPHER_DMA_INTERNAL;		// 0 : INTERNAL, 1 : DMAX
	al.uOperationMode = TCC_CIPHER_OPMODE_CBC;	// 0 : ECB, 1 : CBC, 2 : CFB, 3 : OFB, 4, 5,6,7 : CTR
	al.uAlgorithm = TCC_CIPHER_ALGORITHM_AES;	// 0 : AES, 1 : DES
	al.uArgument1 = TCC_CIPHER_KEYLEN_128;		// AES - 0 : 128bits. 1 : 192bits, 2,3 :  256bits
	al.uArgument2 = NOT_USE;					// AES - not use (DES use)

	memset(aes_key, 0, 16);
	memcpy(aes_key, KEY, 16);

	key.pucData = aes_key;
	key.uLength = 16;			// set key length
	key.uOption = NOT_USE;		// not use this version

	memset(aes_iv, 0, 16);
	memcpy(aes_iv, VEC, 16);

	iv.pucData = aes_iv;		// set init vector 16byte
	iv.uLength = 16;			// set vector length

	tcc_cipher_open();
	tcc_cipher_set_algorithm(&al);
	tcc_cipher_set_key(&key);
	tcc_cipher_set_vector(&iv);

	// get original size
	ret = usb_fat_file_read(filename, (unsigned long)(&dec_size), 4, offset);  // get orginal size
	if(ret < 0) {
		printf("file read failed!\n");
		return -1;
	}
	offset += 4 + 8;    //nou use dest addr

	// get encrypt size
	if(dec_size % 16 == 0)
		enc_size = dec_size;
	else
		enc_size = ((dec_size / 16) + 1) * 16;

	gap = enc_size - dec_size;

	printf("enc_size [%d], dec_size [%d], gap [%d]\n", enc_size, dec_size, gap);

	max_cnt = enc_size / AES_BLOCK_SIZE;
	max_rem = enc_size % AES_BLOCK_SIZE;
	rem_size = enc_size;

	printf("encrypt image cnt [%d], rem [%d]\n", max_cnt, max_rem);

	if(max_rem) {					// 16 character check
		if(max_rem % 16 != 0) {
			printf("wrong size!\n");
			return -1;
		}
	}

	cnt = 0;
	tcnt = 0;

	while(0 < rem_size)
	{
		partial_size = (rem_size > UPDATE_BUFFER_SIZE) ? UPDATE_BUFFER_SIZE : rem_size;
		printf("partial_size rem_size[%d], [%d]partial_size\n", rem_size, partial_size);

		// get 100Mbyte encrypt data
		ret = usb_fat_file_read(filename, (unsigned long)buf_addr, partial_size, offset);
		if(ret < 0)
			printf("usb read failed![%d]\n", ret);

		offset += partial_size;   // add offset (100Mbyte) or remin

		memset(in_buff, 0, AES_BLOCK_SIZE);
		memset(out_buff, 0, AES_BLOCK_SIZE);	
	
		decrypt.pucSrcAddr = in_buff;
		decrypt.pucDstAddr = out_buff;

		if(enc_size - (tcnt * AES_BLOCK_SIZE) >= AES_BLOCK_SIZE) {
			memcpy(in_buff, buf_addr + (cnt * AES_BLOCK_SIZE), AES_BLOCK_SIZE);
			decrypt.uLength = AES_BLOCK_SIZE;
		} else {
			decrypt.uLength = enc_size - (tcnt * AES_BLOCK_SIZE);
			memcpy(in_buff, buf_addr + (cnt * AES_BLOCK_SIZE), enc_size - (tcnt * AES_BLOCK_SIZE));
		}

		tcc_cipher_decrypt(&decrypt);   //decrypt check ok

		++cnt;
		++tcnt;

		pcnt = *((unsigned int *)(out_buff));   // br count ok
		poffset += 4;

		for(i=0;i<pcnt;++i)
		{
			memset(br_buff, 0, 512);
        
			memcpy(&destAddr, out_buff + poffset, 8);
			poffset += 8;

			memcpy(br_buff + 446, out_buff + poffset, 64);

			poffset += 64;
			//write signature
			br_buff[510] = 0x55;
			br_buff[511] = 0xAA;

			ret = DISK_WriteSector(DISK_DEVICE_TRIFLASH, 0, BYTE_TO_SECTOR(destAddr), 1, br_buff); // write emmc
		}
		rem_size = rem_size - partial_size;
	}

	printf("partition update end\n");

	return 0;
}

int decrypt_and_emmc_write(const char *filename)
{
	unsigned char aes_key[AES_KEY_LEN];
	unsigned char aes_iv[AES_IV_LEN];
	unsigned char in_buff[AES_BLOCK_SIZE];
	unsigned char out_buff[AES_BLOCK_SIZE];

	unsigned int max_cnt, max_rem;
	unsigned int dec_size, enc_size, rem_size, partial_size;	  // encrypt size
	int gap;

	unsigned int offset = 0;
	unsigned int destAddr = 0;
	unsigned long long destAddr2 = 0;

	stCIPHER_ALGORITHM	al;
	stCIPHER_KEY		key;
	stCIPHER_VECTOR 	iv;
	stCIPHER_ENCRYPTION encrypt;
	stCIPHER_DECRYPTION decrypt;

	int cnt, tcnt;
	int ret;

	char *buf_addr = (char *)simple_strtoul(UPDATE_BUFFER_ADDR, NULL, 16);

	// cipher init & setting
	al.uDmaMode = TCC_CIPHER_DMA_INTERNAL;		// 0 : INTERNAL, 1 : DMAX
	al.uOperationMode = TCC_CIPHER_OPMODE_CBC;	// 0 : ECB, 1 : CBC, 2 : CFB, 3 : OFB, 4, 5,6,7 : CTR
	al.uAlgorithm = TCC_CIPHER_ALGORITHM_AES;	// 0 : AES, 1 : DES
	al.uArgument1 = TCC_CIPHER_KEYLEN_128;		// AES - 0 : 128bits. 1 : 192bits, 2,3 :  256bits
	al.uArgument2 = NOT_USE;					// AES - not use (DES use)

	memset(aes_key, 0, 16);
	memcpy(aes_key, KEY, 16);

	key.pucData = aes_key;
	key.uLength = 16;			// set key length
	key.uOption = NOT_USE;		// not use this version

	memset(aes_iv, 0, 16);
	memcpy(aes_iv, VEC, 16);

	iv.pucData = aes_iv;		// set init vector 16byte
	iv.uLength = 16;			// set vector length

	tcc_cipher_open();
	tcc_cipher_set_algorithm(&al);
	tcc_cipher_set_key(&key);
	tcc_cipher_set_vector(&iv);

	// get original size
	ret = usb_fat_file_read(filename, (unsigned long)(&dec_size), 4, offset);  // get orginal size
	if(ret < 0) {
		printf("file read failed!\n");
		return -1;
	}
	offset += 4;

	// get partiotion address
	ret = usb_fat_file_read(filename, (unsigned long)(&destAddr2), 8, offset);  // get orginal size
	if(ret < 0) {
		printf("file read failed!\n");
		return -1;
	}
	offset += 8;

	printf("[%s] decrypt start[%d bytes]", filename, dec_size);


	destAddr = destAddr2;
	printf("[address %08X]\n", destAddr);

	// get encrypt size
	if(dec_size % 16 == 0)
		enc_size = dec_size;
	else
		enc_size = ((dec_size / 16) + 1) * 16;

	gap = enc_size - dec_size;

	printf("enc_size [%d], dec_size [%d], gap [%d]\n", enc_size, dec_size, gap);

	max_cnt = enc_size / AES_BLOCK_SIZE;
	max_rem = enc_size % AES_BLOCK_SIZE;
	rem_size = enc_size;

	printf("encrypt image cnt [%d], rem [%d]\n", max_cnt, max_rem);

	if(max_rem) {					// 16 character check
		if(max_rem % 16 != 0) {
			printf("wrong size!\n");
			return -1;
		}
	}

	cnt = 0;
	tcnt = 0;

	while(0 < rem_size)
	{
		partial_size = (rem_size > UPDATE_BUFFER_SIZE) ? UPDATE_BUFFER_SIZE : rem_size;
		printf("partial_size rem_size[%d], [%d]partial_size\n", rem_size, partial_size);

		// get 100Mbyte encrypt data
		ret = usb_fat_file_read(filename, (unsigned long)buf_addr, partial_size, offset);
		if(ret < 0)
			printf("usb read failed![%d]\n", ret);

		offset += partial_size;   // add offset (100Mbyte) or remin

		while(1)
		{
			// buffer init
			memset(in_buff, 0, AES_BLOCK_SIZE);
			memset(out_buff, 0, AES_BLOCK_SIZE);	
		
			decrypt.pucSrcAddr = in_buff;
			decrypt.pucDstAddr = out_buff;

			if(enc_size - (tcnt * AES_BLOCK_SIZE) >= AES_BLOCK_SIZE) {
				memcpy(in_buff, buf_addr + (cnt * AES_BLOCK_SIZE), AES_BLOCK_SIZE);
				decrypt.uLength = AES_BLOCK_SIZE;
			} else {
				decrypt.uLength = enc_size - (tcnt * AES_BLOCK_SIZE);
				memcpy(in_buff, buf_addr + (cnt * AES_BLOCK_SIZE), enc_size - (tcnt * AES_BLOCK_SIZE));
			}

			tcc_cipher_decrypt(&decrypt);

			++cnt;
			++tcnt;

			if(decrypt.uLength >= AES_BLOCK_SIZE) {
				ret = DISK_WriteSector(DISK_DEVICE_TRIFLASH, 0, BYTE_TO_SECTOR(destAddr), 8, out_buff); // write emmc
			} else {
				int rem_sector;
				rem_sector = decrypt.uLength / 512;

				if(max_rem % 512)
					++rem_sector;

				ret = DISK_WriteSector(DISK_DEVICE_TRIFLASH, 0, BYTE_TO_SECTOR(destAddr), rem_sector, out_buff); // write emmc
			}

			if(ret < 0)
				printf("emmc write failed!\n");

			destAddr += AES_BLOCK_SIZE;

			if((AES_BLOCK_SIZE * cnt) >= partial_size) {  // next data read (100MB or rem)
				cnt = 0;
				break;
			}
		}
		rem_size = rem_size - partial_size;
	}

	return 0;
}

int decrypt_and_parse_emmc_write(const char *filename)
{
	// data buffer
	unsigned char aes_key[AES_KEY_LEN]; 		  // aes key buffer
	unsigned char aes_iv[AES_IV_LEN];			  // aes init vector buffer
	unsigned char in_buff[AES_BLOCK_SIZE];		  // befor decrypt data
	unsigned char out_buff[AES_BLOCK_SIZE]; 	  // after decrypt data
	unsigned char emmc_buff[SPARSE_BLOCK_SIZE];   // parsing data
	unsigned char sparse_buff[28];		// sparse header data
	unsigned char chunk_buff[12];		// chunk header data

	unsigned short header_check;

	unsigned int max_cnt, max_rem;		// encrypt data cnt & rem
	unsigned int dec_size, enc_size, rem_size, partial_size;	// encrypt size
	int gap;

	unsigned int offset = 0;
    unsigned int destAddr = 0;
	unsigned long long  destAddr2 = 0;

	// struct for AES
	stCIPHER_ALGORITHM	al;
	stCIPHER_KEY		key;
	stCIPHER_VECTOR 	iv;
	stCIPHER_ENCRYPTION encrypt;
	stCIPHER_DECRYPTION decrypt;

	int cnt, tcnt;
	int ret;

	// need parsing & write
	int action; 			// next data parsing type (sparse, chunk, data)
	int bp; 				// buffer pointer
	int rem;				// remind data
	int blk_cnt;			// data block count
	int write;				// write enable check
	int end_check = 0;

	SH *sh; 				// sparse header
	CH *ch; 				// chunk header

	char *buf_addr = (char *)simple_strtoul(UPDATE_BUFFER_ADDR, NULL, 16);

	// cipher init & setting
	al.uDmaMode = TCC_CIPHER_DMA_INTERNAL;		// 0 : INTERNAL, 1 : DMAX
	al.uOperationMode = TCC_CIPHER_OPMODE_CBC;	// 0 : ECB, 1 : CBC, 2 : CFB, 3 : OFB, 4, 5,6,7 : CTR
	al.uAlgorithm = TCC_CIPHER_ALGORITHM_AES;	// 0 : AES, 1 : DES
	al.uArgument1 = TCC_CIPHER_KEYLEN_128;		// AES - 0 : 128bits. 1 : 192bits, 2,3 :  256bits
	al.uArgument2 = NOT_USE;					// AES - not use (DES use)

	memset(aes_key, 0, 16);
	memcpy(aes_key, KEY, 16);

	key.pucData = aes_key;
	key.uLength = 16;			// set key length
	key.uOption = NOT_USE;		// not use this version

	memset(aes_iv, 0, 16);
	memcpy(aes_iv, VEC, 16);

	iv.pucData = aes_iv;		// set init vector 16byte
	iv.uLength = 16;			// set vector length

	tcc_cipher_open();
	tcc_cipher_set_algorithm(&al);
	tcc_cipher_set_key(&key);
	tcc_cipher_set_vector(&iv);

	// get original size
	ret = usb_fat_file_read(filename, (unsigned long)(&dec_size), 4, offset);  // get orginal size
	if(ret < 0) {
		printf("file read failed!\n");
		return -1;
	}
	offset += 4;

	// get original size
	ret = usb_fat_file_read(filename, (unsigned long)(&destAddr2), 8, offset);  // get orginal size
	if(ret < 0) {
		printf("file read failed!\n");
		return -1;
	}
	offset += 8;

	printf("[%s] decrypt start[%d bytes]", filename, dec_size);

	destAddr = destAddr2;
	printf("[address %08X]\n", destAddr);

	// get encrypt size
	if(dec_size % 16 == 0)
		enc_size = dec_size;
	else
		enc_size = ((dec_size / 16) + 1) * 16;

	gap = enc_size - dec_size;

	printf("enc_size [%d], dec_size [%d], gap [%d]\n", enc_size, dec_size, gap);

	max_cnt = enc_size / AES_BLOCK_SIZE;
	max_rem = enc_size % AES_BLOCK_SIZE;
	rem_size = enc_size;

	printf("encrypt image cnt [%d], rem [%d]\n", max_cnt, max_rem);

	if(max_rem) {					// 16 character check
		if(max_rem % 16 != 0) {
			printf("wrong size!\n");
			return -1;
		}
	}

	action = ACTION_SPARSE_PARSING; 		// 0 - sparse header parsing (need first time)
	rem = 0;
	bp = 0; 		// buffer point in AES_BLOCK_SIZE buffer
	blk_cnt = 0;	// check sparse format data block count
	cnt = 0;		// check AES_BLOCK_SIZE times (in UPDATE_BUFFER_SIZE)
	tcnt = 0;

	while(0 < rem_size)
	{
		partial_size = (rem_size>UPDATE_BUFFER_SIZE)?UPDATE_BUFFER_SIZE:rem_size;

		ret = usb_fat_file_read(filename, (unsigned long)buf_addr, partial_size, offset);		 // get 100Mbyte encrypt data
		if(ret < 0)
			printf("usb read failed![%d]\n", ret);

		offset += partial_size;   // add offset (100Mbyte)

		memset(out_buff, 0, AES_BLOCK_SIZE);		// first block decrypt
		memset(in_buff, 0, AES_BLOCK_SIZE);
		memcpy(in_buff, buf_addr, AES_BLOCK_SIZE);
		decrypt.pucSrcAddr = in_buff;
		decrypt.pucDstAddr = out_buff;
		decrypt.uLength = AES_BLOCK_SIZE;

		tcc_cipher_decrypt(&decrypt);	// data decrypt

		cnt++;
		tcnt++;

		while(1)	// end partial_size
		{
			switch(action)
			{
				case ACTION_SPARSE_PARSING: 	// parsing sparse data (only first time)
					header_check = *((unsigned short *)(out_buff + 8)); // get sparse header length
					memcpy(sparse_buff, out_buff + bp, header_check);	// get sparse header
					sh = sparse_buff;
					bp += header_check; 								// add buffer point (+sparse header size)
					action = ACTION_CHUNK_PARSING;						// change action : next step chunk header parsing
					break;
				case ACTION_CHUNK_PARSING:		// parsing chunk data
					if(!rem)
					{
						if(AES_BLOCK_SIZE - bp >= sizeof(CH)) { 		// read enough buffer
							memcpy(chunk_buff, out_buff + bp, sh->chunk_hdr_sz);  // get chunk header
							ch = chunk_buff;
							bp += sh->chunk_hdr_sz; 							  // move buffer point (+chunk header size)
							action = ACTION_GET_DATA;							  // change action : next step get data
						} else {				// not enough buffer first action
							memcpy(chunk_buff, out_buff + bp, 4096 - bp);		  // get chunk header potion
							rem = sh->chunk_hdr_sz - (AES_BLOCK_SIZE - bp); 	  // check remainder
							bp += AES_BLOCK_SIZE - bp;							  // add buffer point
						}
					}
					else						// not enough buffer second action
					{
						memcpy(chunk_buff + ((sh->chunk_hdr_sz) - rem), out_buff + bp, rem);  // get chunk header remainder
						bp += rem;															// move buffer pointer
						rem = 0;															// remainder = 0 (chunk read compleate)
						action = ACTION_GET_DATA;										   // change action : next step get data
					}

					if(action == ACTION_GET_DATA)
					{
						if(ch->chunk_type == CHUNK_TYPE_DONT_CARE)	// chunk type check (skip area)
						{
							destAddr += ch->chunk_sz * sh->blk_sz;	// write address jump...
							action = ACTION_CHUNK_PARSING;		   // re parsing chunk data
							break;
						}
						blk_cnt = ch->chunk_sz; 					// get block count by chunk header
					}
					break;
				case ACTION_GET_DATA:
					if(!rem)
					{
						if(AES_BLOCK_SIZE - bp >= SPARSE_BLOCK_SIZE) {	 // read enough buffer
							memcpy(emmc_buff, out_buff + bp, AES_BLOCK_SIZE);	 // get data
							bp += AES_BLOCK_SIZE;								 // move buffer pointer
							write = WRITE_ENABLE;								 // write enable

						} else {				// not enough buffer first action
							memcpy(emmc_buff, out_buff + bp, AES_BLOCK_SIZE - bp);	// get data potion
							rem = AES_BLOCK_SIZE - (AES_BLOCK_SIZE - bp);			// check remainder
							bp += AES_BLOCK_SIZE - bp;								// add buffer point
						}
					}
					else						// not enough buffer second action
					{
						memcpy(emmc_buff + (AES_BLOCK_SIZE - rem), out_buff + bp, rem); // get data remainder
						bp += rem;														// move buffer point
						rem = 0;														// remainder = 0 (data read compleate)
						write = WRITE_ENABLE;											// write enable
					}

					if(write) // write 
					{
						if(decrypt.uLength >= AES_BLOCK_SIZE) {
							ret = DISK_WriteSector(DISK_DEVICE_TRIFLASH, 0, BYTE_TO_SECTOR(destAddr), 8, emmc_buff); // write emmc
						} else {
							int rem_sector;
							rem_sector = decrypt.uLength / 512;

							if(max_rem % 512)
								++rem_sector;

							ret = DISK_WriteSector(DISK_DEVICE_TRIFLASH, 0, BYTE_TO_SECTOR(destAddr), 8, emmc_buff); // write emmc

							end_check = 1;
						}

						if(ret < 0)
							printf("emmc write failed!\n");

						destAddr += SPARSE_BLOCK_SIZE;	// add emmc address
						--blk_cnt;						// sub data block count
						write = WRITE_DISABLE;			// write disable
					}

					if(blk_cnt == 0)					// check block count
						action = ACTION_CHUNK_PARSING;	// change action : next step chunk header parsing
					break;
			}

			if((bp >= AES_BLOCK_SIZE) || end_check) {					 // next data decrypt (4Kbyte)
				if((AES_BLOCK_SIZE * cnt) >= partial_size) {  // next data read (100MB or rem)
					cnt = 0;
					bp = 0;

					break;
				}

				// new data decrypt
				memset(in_buff, 0, AES_BLOCK_SIZE);
				memset(out_buff, 0, AES_BLOCK_SIZE);

				//decrypt.pucSrcAddr = buf_addr + (cnt + AES_BLOCK_SIZE);
				decrypt.pucSrcAddr = in_buff;
				decrypt.pucDstAddr = out_buff;

				if(enc_size - (tcnt * AES_BLOCK_SIZE) > AES_BLOCK_SIZE) {
					decrypt.uLength = AES_BLOCK_SIZE;
					memcpy(in_buff, buf_addr + (cnt * AES_BLOCK_SIZE), AES_BLOCK_SIZE);
				} else {
					decrypt.uLength = enc_size - (tcnt * AES_BLOCK_SIZE);
					memcpy(in_buff, buf_addr + (cnt * AES_BLOCK_SIZE), enc_size - (tcnt * AES_BLOCK_SIZE));
				}

				tcc_cipher_decrypt(&decrypt);	// data decrypt

				++cnt;
				++tcnt; 	// total count for remine
				bp = 0;
				//printf("next data process length (%d) cnt(%d) tcnt(%d)\n", decrypt.uLength, cnt, tcnt);
			}
		}

		rem_size = rem_size - partial_size; 		  // rem_size = 0 : process end
	}
	
	return 0;
}

int decrypt_and_bootloader_write(const char *filename)
{
	unsigned char aes_key[AES_KEY_LEN];
	unsigned char aes_iv[AES_IV_LEN];
	unsigned char in_buff[AES_BLOCK_SIZE];
	unsigned char out_buff[AES_BLOCK_SIZE];

	unsigned int max_cnt, max_rem;
	unsigned int dec_size, enc_size;	  // encrypt size
	int gap;

	unsigned int offset = 0;
	unsigned int destAddr = 0;
	unsigned long long destAddr2 = 0;

	stCIPHER_ALGORITHM	al;
	stCIPHER_KEY		key;
	stCIPHER_VECTOR 	iv;
	stCIPHER_ENCRYPTION encrypt;
	stCIPHER_DECRYPTION decrypt;

	int cnt, pcnt;
	int ret;

	char *buf_addr = (char *)simple_strtoul(UPDATE_BUFFER_ADDR, NULL, 16);

	unsigned long long bak_destAddr = destAddr;

	// cipher init & setting
	al.uDmaMode = TCC_CIPHER_DMA_INTERNAL;		// 0 : INTERNAL, 1 : DMAX
	al.uOperationMode = TCC_CIPHER_OPMODE_CBC;	// 0 : ECB, 1 : CBC, 2 : CFB, 3 : OFB, 4, 5,6,7 : CTR
	al.uAlgorithm = TCC_CIPHER_ALGORITHM_AES;	// 0 : AES, 1 : DES
	al.uArgument1 = TCC_CIPHER_KEYLEN_128;		// AES - 0 : 128bits. 1 : 192bits, 2,3 :  256bits
	al.uArgument2 = NOT_USE;					// AES - not use (DES use)

	memset(aes_key, 0, AES_KEY_LEN);
	memcpy(aes_key, KEY, AES_KEY_LEN);

	key.pucData = aes_key;
	key.uLength = AES_KEY_LEN;	// set key length
	key.uOption = NOT_USE;		// not use this version

	memset(aes_iv, 0, AES_IV_LEN);
	memcpy(aes_iv, VEC, AES_IV_LEN);

	iv.pucData = aes_iv;		// set init vector 16byte
	iv.uLength = AES_IV_LEN;	// set vector length

	tcc_cipher_open();
	tcc_cipher_set_algorithm(&al);
	tcc_cipher_set_key(&key);
	tcc_cipher_set_vector(&iv);

	// get original size
	ret = usb_fat_file_read(filename, (unsigned long)(&dec_size), 4, offset);  // get orginal size
	if(ret < 0) {
		printf("file read failed!\n");
		return -1;
	}
	offset += 4;

	ret = usb_fat_file_read(filename, (unsigned long)(&destAddr2), 8, offset);  // get orginal size
	if(ret < 0) {
		printf("file read failed!\n");
		return -1;
	}
	offset += 8;

	printf("[%s] decrypt start[%d bytes][address %d]\n", filename, dec_size, BYTE_TO_SECTOR(destAddr));

	destAddr = destAddr2;
	printf("addr [%08X]\n", destAddr);

	// get encrypt size
	if(dec_size % 16 == 0)
		enc_size = dec_size;
	else
		enc_size = ((dec_size / 16) + 1) * 16;

	gap = enc_size - dec_size;

	printf("enc_size [%d], dec_size [%d], gap [%d]\n", enc_size, dec_size, gap);

	max_cnt = enc_size / AES_BLOCK_SIZE;
	max_rem = enc_size % AES_BLOCK_SIZE;

	printf("encrypt image cnt [%d], remine [%d]\n", max_cnt, max_rem);

	if(max_rem) {					// 16 character check
		if(max_rem % 16 != 0) {
			printf("wrong size!\n");
			return -1;
		}
	}

	pcnt = 1;

	for(cnt=0;cnt<max_cnt;++cnt)
	{
		decrypt.pucSrcAddr = in_buff;
		decrypt.pucDstAddr = out_buff;
		decrypt.uLength = AES_BLOCK_SIZE;

		// read usb -> decrypt data -> write emmc
		ret = usb_fat_file_read(filename, (unsigned long)in_buff, AES_BLOCK_SIZE, offset);	// get first page
		if(ret < 0)
			printf("usb read failed!\n");

		offset += AES_BLOCK_SIZE;

		tcc_cipher_decrypt(&decrypt);

		//memcpy(bootloader + (destAddr), out_buff, AES_BLOCK_SIZE);
		memcpy(buf_addr + (destAddr), out_buff, AES_BLOCK_SIZE);

		destAddr += AES_BLOCK_SIZE;

 	}
	printf("\n");

	if(max_rem)
	{
		int rem_sector;

		decrypt.pucSrcAddr = in_buff;
		decrypt.pucDstAddr = out_buff;
		decrypt.uLength = max_rem;

		// read usb -> decrypt data -> write emmc
		ret = usb_fat_file_read(filename, (unsigned long)in_buff, max_rem, offset);  // get first page
		if(ret < 0)
			printf("usb read failed!\n");

		offset = offset + max_rem;

		tcc_cipher_decrypt(&decrypt);

		rem_sector = max_rem / 512;

		if(max_rem % 512)
			++rem_sector;

		memcpy(buf_addr + (destAddr), out_buff, max_rem);
	}

	buf_addr_for_bootloader = (char *)buf_addr;
	tcc_write("bootloader", BYTE_TO_SECTOR(bak_destAddr), enc_size, buf_addr_for_bootloader);	 // addr, size, buffer

	return 0;
}

int usbAESupdate(void)
{
	char *usb_argv[5];
	int temp;
	int ret;

	/*usb initailization*/
	usb_argv[0] = "usb";
	usb_argv[1] = "start";

#if 1	// usb init in do_usbupdate()...
	if(do_usb(NULL, 0, 2, usb_argv)) {
		update_debug("USB init fail!\n");
		return -1;
	}
#endif

	update_debug("\n=== USB Encrypt Update START ==\n");

	// update file check
	ret = usb_fat_file_read(ENC_BOOT_IMG, (unsigned long)(&temp), 4, 0);
	if(ret < 0) {
		printf("[%s] file not find\n", ENC_BOOT_IMG);
		goto aes_bootloader_update;
	}
	ret = usb_fat_file_read(ENC_SYSTEM_IMG, (unsigned long)(&temp), 4, 0);
	if(ret < 0) {
		printf("[%s] file not find\n", ENC_SYSTEM_IMG);
		goto aes_bootloader_update;
	}
	ret = usb_fat_file_read(ENC_RECOVERY_IMG, (unsigned long)(&temp), 4, 0);
	if(ret < 0) {
		printf("[%s] file not find\n", ENC_RECOVERY_IMG);
		goto aes_bootloader_update;
	}
	ret = usb_fat_file_read(ENC_SPLASH_IMG, (unsigned long)(&temp), 4, 0);
	if(ret < 0) {
		printf("[%s] file not find\n", ENC_SPLASH_IMG);
		goto aes_bootloader_update;
	}
	ret = usb_fat_file_read(ENC_QB_DATA_IMG, (unsigned long)(&temp), 4, 0);
	if(ret < 0) {
		printf("[%s] file not find\n", ENC_QB_DATA_IMG);
		goto aes_bootloader_update;
	}
	ret = usb_fat_file_read(ENC_PT_DAT, (unsigned long)(&temp), 4, 0);
	if(ret < 0) {
		printf("[%s] file not find\n", ENC_PT_DAT);
		goto aes_bootloader_update;
	}

	if( !erase_emmc(0, 0, 2) ) {
		update_debug("Erase eMMC finished!\n");
	} else {
		update_debug("Erase eMMC failed...\n");
		return -1;
	}

	ret = decrypt_and_emmc_write(ENC_BOOT_IMG);					// boot.img 
	ret = decrypt_and_parse_emmc_write(ENC_SYSTEM_IMG);			// system.img
	ret = decrypt_and_emmc_write(ENC_RECOVERY_IMG);				// recovery.img
	ret = decrypt_and_emmc_write(ENC_SPLASH_IMG);				// splash.img
	ret = decrypt_and_emmc_write(ENC_QB_DATA_IMG);				// qb_data.img
	ret = decrypt_and_partition_write(ENC_PT_DAT);				// patririon table

aes_bootloader_update:
	ret = usb_fat_file_read(ENC_LK_ROM, (unsigned long)(&temp), 4, 0);
	if(ret < 0) 
	{
		update_debug("USB Encrypt Update FAILED\n");
		return -1;
	}

	ret = decrypt_and_bootloader_write(ENC_LK_ROM);				// bootloader(lk.rom)

	update_debug("USB Encrypt Update SUCCESS\n");
	update_debug("Please disconnect USB connection.\n");

	do_reset(NULL, 0, 0, NULL);

	return 0;
}

U_BOOT_CMD(
	usbupdate,	3,	1,	do_usbupdate,
	"update from USB device to eMMC",
	"usbupdate [addr] [file-name]"
);
#endif /* CONFIG_USB_UPDATE */

U_BOOT_CMD(
	usb,	5,	1,	do_usb,
	"USB sub-system",
	"start - start (scan) USB controller\n"
	"usb reset - reset (rescan) USB controller\n"
	"usb stop [f] - stop USB [f]=force stop\n"
	"usb tree - show USB device tree\n"
	"usb info [dev] - show available USB devices\n"
	"usb test [dev] [port] [mode] - set USB 2.0 test mode\n"
	"    (specify port 0 to indicate the device's upstream port)\n"
	"    Available modes: J, K, S[E0_NAK], P[acket], F[orce_Enable]\n"
#ifdef CONFIG_USB_STORAGE
	"usb storage - show details of USB storage devices\n"
	"usb dev [dev] - show or set current USB storage device\n"
	"usb part [dev] - print partition table of one or all USB storage"
	"    devices\n"
	"usb read addr blk# cnt - read `cnt' blocks starting at block `blk#'\n"
	"    to memory address `addr'\n"
	"usb write addr blk# cnt - write `cnt' blocks starting at block `blk#'\n"
	"    from memory address `addr'"
#endif /* CONFIG_USB_STORAGE */
);


#ifdef CONFIG_USB_STORAGE
U_BOOT_CMD(
	usbboot,	3,	1,	do_usbboot,
	"boot from USB device",
	"loadAddr dev:part"
);
#endif /* CONFIG_USB_STORAGE */
