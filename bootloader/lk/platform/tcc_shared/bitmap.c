#include <bitmap.h>

#include <stdlib.h>
#include <dev/flash.h>
#include <string.h>
#include <malloc.h>
#include <sdmmc/emmc.h>

#define TAG_BITMAP		"[cleinsoft bitmap]"
#define DEBUG_BITMAP	1

#if (DEBUG_BITMAP)
#define ERRORP(fmt, args...) dprintf(CRITICAL, TAG_BITMAP " " fmt, ##args)
#define INFOP(fmt, args...) dprintf(INFO, TAG_BITMAP " " fmt, ##args)
#define DEBUGP(fmt, args...) dprintf(SPEW, TAG_BITMAP " " fmt, ##args)
#else
#define ERRORP(fmt, args...) do {} while(0)
#define INFOP(fmt, args...) do {} while(0)
#define DEBUGP(fmt, args...) do {} while(0)
#endif

/**
 * Copies characters between buffers.
 * @param	src Buffer to copy from (3byte - rgb)
 * @param	dest New buffer (4byte - argb)
 */
static void copy_bmp(char *src, char *dest) {
	*dest = *src;
	*(dest + 1) = *(src + 1);
	*(dest + 2) = *(src + 2);
}

/**
 * Get offset of logo type
 * @param type	Image type
 * @return Offset of image. -1 is failed.
 */
static long get_bitmap_offset(enum image_type type) {
	switch (type) {
		case BOOT_LOGO:
			return BYTE_TO_SECTOR(OFFSET_BOOT_LOGO);

		case PARKING_GUIDE_LOGO:
			return BYTE_TO_SECTOR(OFFSET_PARKING_GUIDE_LOGO);

		case MAKE_QUICKBOOT_LOGO:
			return BYTE_TO_SECTOR(OFFSET_MAKE_QUICKBOOT_LOGO);

		case RESERVED:
			return BYTE_TO_SECTOR(OFFSET_RESERVED);

		default:
			return -1;
	}
}

/**
 * Raw bitmap image data copy & convert to destination buffer.
 * @param	ptn_offset Offset of bitmap image.
 * @param	dbuf Pointer of dest_image_buffer.
 * @return	0 is success, -1 is failed.
 **/
static int bitmap_buffer_to_dest(long ptn_offset, struct dest_image_buffer *dbuf) {
	unsigned page_size = flash_page_size();
	unsigned long long ptn = 0;
	char *buf = 0, *dest_buf = 0, *bm_addr = 0;
	unsigned int logo_size, convert_logo_size, bm_width, bm_height, read_size;
	int width_count, total_count = 0;
	int bm_bpp, dest_bpp;
	long c_offset, start_addr;
	struct bm_header *bm_h;

	if (dbuf == NULL) {
		ERRORP("%s dest_image_buffer is NULL\n", __func__);
		goto error0;
	}

	ptn = emmc_ptn_offset("splash");
	if (ptn == 0) {
		ERRORP("%s ERROR : No splash partition found!\n", __func__);
		goto error0;
	}

	//malloc for bitmap header.
	buf = malloc(page_size);
	if (buf == NULL) {
		ERRORP("%s ERROR malloc failed!\n", __func__);
		goto error0;
	}

	DEBUGP("%s load boot logo started.\n", __func__);

	//read emmc
	emmc_read(ptn + ptn_offset, page_size, buf);

	//INFOP("%s splash image to temp buffer\n", __func__);

	//init bitmap header
	bm_h = malloc(sizeof(struct bm_header));
	if (bm_h == NULL) {
		ERRORP("%s ERROR malloc bm_header\n", __func__);
		goto error1;
	}
	memcpy(bm_h, buf, sizeof(struct bm_header));
	free(buf);

#if 0
	print_bm_header(bm_h);
	INFOP("%s ptn_offset: 0x%lx, dest_image_buffer: 0x%x\n",
			__func__, ptn_offset, (unsigned int)dbuf);
#endif

	//setting for config
	dest_bpp = dbuf->bytepp;
	convert_logo_size = dbuf->width * dbuf->height * dest_bpp;

	bm_width = bm_h->info.width;
	bm_height = bm_h->info.height;
	bm_bpp = bm_h->info.bit_count / 8;
	logo_size = bm_h->fheader.size;

	if (bm_h->fheader.type != BITMAP_TYPE) {
		ERRORP("%s not found bitmap. [0x%x]\n", __func__, bm_h->fheader.type);
		goto error1;
	}

	//read bitmap header + image data
	read_size = (page_size * (logo_size / page_size)) +
		(logo_size % page_size > 0 ? page_size : 0);

#if 0
	INFOP("%s dest_bpp: %d, convert_logo_size: %d\n",
		__func__, dest_bpp, convert_logo_size);
	INFOP("%s bm w: %d, bm h: %d, bm_bpp: %d, logo_size: %d\n",
		__func__, bm_width, bm_height, bm_bpp, logo_size);
	INFOP("%s read_size: %d\n", __func__, read_size);
#endif

	//malloc for buffer
	buf = malloc(read_size);
	if (buf == NULL) {
		ERRORP("%s ERROR malloc failed2!\n", __func__);
		goto error1;
	}

	dest_buf = malloc(convert_logo_size);
	if (dest_buf == NULL) {
		ERRORP("%s ERROR malloc failed3!\n", __func__);
		goto error2;
	}

	//init destination buffer
	memset(dest_buf, 0, convert_logo_size);

	emmc_read(ptn + ptn_offset, read_size, buf);

	//init image data default offset
	bm_addr = buf + bm_h->fheader.bm_offset;

	//read & convert bitmap image data
	//           Bitamp
	//ex) 8 * 4 bitamp raw data offset(24bit)
	//(3, 0), (3, 1), (3, 2),,, (3, 7)
	//(2, 0), (2, 1), (2, 2),,, (2, 7)
	//....
	//(0, 0), (0, 1), (0, 2),,, (0, 7)
	
	//			Convert buffer
	//ex) 8 * 4 buffer offset(32bit)
	//(0, 0), (0, 1), (0, 2),,, (0, 7)
	//....
	//(3, 0), (3, 1), (3, 2),,, (3, 7)

	while (bm_height > 0) {
		width_count = bm_width;
		start_addr = (bm_height * width_count * bm_bpp);

		while (width_count > 0) {
			c_offset = start_addr - (width_count * bm_bpp);
			copy_bmp(bm_addr + c_offset, dest_buf + (total_count * dest_bpp));
			total_count++; width_count--;
		}
		bm_height--;
	}
	DEBUGP("%s swap finished. count: %d\n", __func__, total_count);

	//converted image data to buffer
	memcpy(dbuf->dest_addr, dest_buf, convert_logo_size);
	DEBUGP("%s temp buffer to fb\n", __func__);

	free(buf);
	free(dest_buf);
	free(bm_h);

	INFOP("%s load logo finished.\n", __func__);

	return 0;

error2:
	free(buf);
error1:
	free(bm_h);
error0:
	return -1;
}

/**
 * Bitmap image copy to parking guide.
 * @param	type Image type for load
 * @param	pg_addr Base parking guide image address of CM3 to use
 * @return	0 is success, -1 is failed.
 */
int bitmap_to_addr(enum image_type type, unsigned int pg_addr) {
	struct dest_image_buffer dbuf;
	long ptn_offset = get_bitmap_offset(type);

	if (ptn_offset < 0 || pg_addr == 0) {
		ERRORP("%s Invalid args.\n", __func__);
		return -1;
	}

	dbuf.width = WIDTH_DEFAULT_DEST;
	dbuf.height = HEIGHT_DEFAULT_DEST;
	dbuf.bytepp = BPP_DEFAULT_DEST;
	dbuf.dest_addr = (void *)pg_addr;

	return bitmap_buffer_to_dest(ptn_offset, &dbuf);
}

/**
 * Bitmap image copy to frame buffer.
 * @param	type Image type for load
 * @param	fb_cfg Pointer of struct fbcon_config
 * @return	0 is success, -1 is failed.
 */
int bitmap_to_fb(enum image_type type, struct fbcon_config *fb_cfg) {
	struct dest_image_buffer dbuf;
	long ptn_offset = get_bitmap_offset(type);

	if (ptn_offset < 0 || fb_cfg == NULL) {
		ERRORP("%s Invalid args.\n", __func__);
		return -1;
	}

	dbuf.width = fb_cfg->width;
	dbuf.height = fb_cfg->height;
	dbuf.bytepp = fb_cfg->bpp / 8;
	dbuf.dest_addr = fb_cfg->base;

	return bitmap_buffer_to_dest(ptn_offset, &dbuf);
}

/**
 * Print Bitmap header info.
 * @param header Pointer of bitmap header structure.
 */
void print_bm_header(struct bm_header *header) {
	struct bm_fheader *fheader;
	struct bm_info *info;

	if (header == NULL) {
		ERRORP("%s bitmap header is null\n", __func__);
		return;
	}

	fheader = &header->fheader;
	info = &header->info;

	INFOP("%s Bitmap file header\n", __func__);
	INFOP("%s Type: 0x%x, Size: %ld, Bitmap offset: 0x%lx\n",
			__func__, fheader->type, fheader->size, fheader->bm_offset);
	INFOP("%s Bitmap info header\n", __func__);
	INFOP("%s Size: %ld, Width: %ld Height: %ld\n",
			__func__, info->size, info->width, info->height);
	INFOP("%s Planes: %hd, Bit Count: %hd Compression: %ld Image Size: %ld\n",
			__func__, info->planes, info->bit_count, info->compression, info->image_size);
	INFOP("%s X pixel per meter: %ld, Y pixel per meter: %ld\n",
			__func__, info->x_pixel_per_meter, info->y_pixel_per_meter);
	INFOP("%s Coloer used: %ld, Color important: %ld\n",
			__func__, info->color_used, info->color_important);
}
