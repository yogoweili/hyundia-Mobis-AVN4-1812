#ifndef __DAUDIO_BITMAP__
#define __DAUDIO_BITMAP__

#include <dev/fbcon.h>

enum image_type {
	BOOT_LOGO,
	PARKING_GUIDE_LOGO,
	MAKE_QUICKBOOT_LOGO,
	RESERVED,
	IMAGE_TYPE_MAX,
};

#define BITMAP_TYPE					0x4D42

#define OFFSET_BOOT_LOGO			0x0
#define OFFSET_PARKING_GUIDE_LOGO	0x150000
#define OFFSET_MAKE_QUICKBOOT_LOGO	0x2A0000
#define OFFSET_RESERVED				0x3F0000

#define WIDTH_DEFAULT_DEST			800
#define HEIGHT_DEFAULT_DEST			480
#define BPP_DEFAULT_DEST			4

#pragma pack(1)

struct bm_fheader {
	unsigned short type;
	unsigned long size;
	unsigned short reserved1;
	unsigned short reserved2;
	unsigned long bm_offset;
};

struct bm_info {
	unsigned long size;
	unsigned long width;
	unsigned long height;
	unsigned short planes;
	unsigned short bit_count;
	unsigned long compression;
	unsigned long image_size;
	unsigned long x_pixel_per_meter;
	unsigned long y_pixel_per_meter;
	unsigned long color_used;
	unsigned long color_important;
};

struct bm_header {
	struct bm_fheader fheader;
	struct bm_info info;
};

struct dest_image_buffer {
	unsigned int width;
	unsigned int height;
	int bytepp;
	void *dest_addr;
};

#pragma pack()

int bitmap_to_addr(enum image_type type, unsigned int pg_addr);
int bitmap_to_fb(enum image_type type, struct fbcon_config *fb_cfg);
void print_bm_header(struct bm_header *header);

#endif
