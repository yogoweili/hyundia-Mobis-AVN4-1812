#include <debug.h>
#include <malloc.h>
#include <string.h>
#include <dev/flash.h>
#include <lib/ptable.h>
#include <dev/fbcon.h>
#include <splash/splash.h>

#ifdef BOOTSD_INCLUDE
#include <fwdn/Disk.h>
#include <sdmmc/sd_memory.h>
#include <sdmmc/emmc.h>
#endif

#ifdef TARGET_BOARD_DAUDIO
#include <bitmap.h>

static char *splash_image_names[] =
{
	"bootlogo",		//BOOT_LOGO
	"pg001",		//PARKING_GUIDE_LOGO
	"quickboot",	//MAKE_QUICKBOOT_LOGO
	"reserved",		//RESERVED
	"error",
};
#endif

#define ROUND_TO_PAGE(x,y) (((x) + (y)) & (~(y)))
#define BYTE_TO_SECTOR(x) (x) / 512

unsigned char *splash[16384];

/**
 * @author valky@cleinsoft
 * @date 2013/11/14
 * TCC splash boot logo patch
 **/
#if defined(TCC_CM3_USE) && !defined(DISPLAY_SPLASH_SCREEN_DIRECT)
int get_splash_index(SPLASH_IMAGE_Header_info_t *splash_hdr, char *part)
#else
static int get_splash_index(SPLASH_IMAGE_Header_info_t *splash_hdr, char *part)
#endif
{
	unsigned int idx = 0;

	for(idx = 0; idx < splash_hdr->ulNumber ; idx ++){

		dprintf(SPEW, "part name : %s idx : %d\n" , splash_hdr->SPLASH_IMAGE[idx].ucImageName , idx);

		if(!strcmp((splash_hdr->SPLASH_IMAGE[idx].ucImageName) , part)){
			return idx;
		}
	}

	return -1;
}

static int splash_image_nand_v7(char *partition){

	SPLASH_IMAGE_Header_info_t *splash_hdr = (void*)splash;
	struct fbcon_config *fb_display = NULL;
	int img_idx = 0;
	unsigned int page_size = flash_page_size();
	unsigned int page_mask = page_size -1;

	dprintf(SPEW, "partition : %s\n", partition);

	struct ptentry *ptn; struct ptable *ptable;
	ptable = flash_get_ptable();
	if(ptable == NULL){
		dprintf(CRITICAL, "ERROR : partition table not found!\n");
		return -1;
	}

	ptn = ptable_find(ptable, "splash");
	if(ptn == NULL){
		dprintf(CRITICAL, "ERROR : No splash partition found !\n");
		return -1;
	}else{

		fb_display = fbcon_display();

		if(fb_display){

			if(!flash_read(ptn, 0, splash_hdr, page_size)){
				if(strcmp(splash_hdr->ucPartition, SPLASH_TAG)){
					dprintf(CRITICAL, "Splash TAG Is Mismatched\n");
					return -1;
				}
			}
			img_idx = get_splash_index(splash_hdr, partition);
			dprintf(SPEW, "image idx = %d \n" ,img_idx);

			if(img_idx < 0){
				dprintf(CRITICAL, " there is no image [%s]\n", partition);
			}else{

				if((fb_display->width != splash_hdr->SPLASH_IMAGE[img_idx].ulImageWidth) 
						&& (fb_display->height != splash_hdr->SPLASH_IMAGE[img_idx].ulImageHeight)){

					fb_display->width = splash_hdr->SPLASH_IMAGE[img_idx].ulImageWidth;
					fb_display->height = splash_hdr->SPLASH_IMAGE[img_idx].ulImageHeight;
					display_splash_logo(fb_display);
				}

				if(flash_read(ptn , ROUND_TO_PAGE(splash_hdr->SPLASH_IMAGE[img_idx].ulImageAddr , page_mask),
							fb_display->base, ROUND_TO_PAGE((splash_hdr->SPLASH_IMAGE[img_idx].ulImageSize) , page_mask))){
					fbcon_clear();
				}
			}
		}
	}

	return 0;

}

static int splash_image_nand_v8(char *partition){

	SPLASH_IMAGE_Header_info_t *splash_hdr = (void*)splash;
	struct fbcon_config *fb_display = NULL;
	int img_idx = 0;
	unsigned int page_size = flash_page_size();
	unsigned int page_mask = page_size -1;

	unsigned long long ptn = 0;

	dprintf(SPEW, "partition : %s\n", partition);


	ptn = flash_ptn_offset("splash");
	if(ptn == 0){
		dprintf(CRITICAL, "ERROR : No splash partition found !\n");
		return -1;
	}else{

		fb_display = fbcon_display();

		if(fb_display){

			if(!flash_read_tnftl_v8(ptn, page_size, splash_hdr)){
				if(strcmp(splash_hdr->ucPartition, SPLASH_TAG)){
					dprintf(CRITICAL, "Splash TAG Is Mismatched\n");
					return -1;
				}

			}
			
			img_idx = get_splash_index(splash_hdr, partition);
			dprintf(SPEW, "image idx = %d \n" ,img_idx);


			if(img_idx < 0){
				dprintf(CRITICAL, " there is no image [%s]\n", partition);
			}else{

				if((fb_display->width != splash_hdr->SPLASH_IMAGE[img_idx].ulImageWidth) 
						&& (fb_display->height != splash_hdr->SPLASH_IMAGE[img_idx].ulImageHeight)){

					fb_display->width = splash_hdr->SPLASH_IMAGE[img_idx].ulImageWidth;
					fb_display->height = splash_hdr->SPLASH_IMAGE[img_idx].ulImageHeight;
					display_splash_logo(fb_display);
				}

				if(flash_read_tnftl_v8(ptn+BYTE_TO_SECTOR(splash_hdr->SPLASH_IMAGE[img_idx].ulImageAddr),
							splash_hdr->SPLASH_IMAGE[img_idx].ulImageSize, fb_display->base)){
					fbcon_clear();
				}
			}
		}
	}

	return 0;

}

static int splash_image_sdmmc(char *partition){

	SPLASH_IMAGE_Header_info_t *splash_hdr = (void*)splash;
	struct fbcon_config *fb_display = NULL;
	int img_idx = 0;
	unsigned int page_size = flash_page_size();
	unsigned int page_mask = page_size -1;

	unsigned long long ptn = 0;

	dprintf(SPEW, "partition : %s\n", partition);

	ptn = emmc_ptn_offset("splash");
	if(ptn == 0){
		dprintf(CRITICAL, "ERROR : No splash partition found !\n");
		return -1;
	}else{

		fb_display = fbcon_display();

		if(fb_display){

			if(!emmc_read(ptn, page_size, splash_hdr)){
				if(strcmp(splash_hdr->ucPartition, SPLASH_TAG)){
					dprintf(CRITICAL, "Splash TAG Is Mismatched\n");
					return -1;
				}
			}
			img_idx = get_splash_index(splash_hdr, partition);
			dprintf(SPEW, "image idx = %d \n" ,img_idx);

			if(img_idx < 0){
				dprintf(CRITICAL, " there is no image [%s]\n", partition);
			}else{

				if((fb_display->width != splash_hdr->SPLASH_IMAGE[img_idx].ulImageWidth) 
						&& (fb_display->height != splash_hdr->SPLASH_IMAGE[img_idx].ulImageHeight)){

					fb_display->width = splash_hdr->SPLASH_IMAGE[img_idx].ulImageWidth;
					fb_display->height = splash_hdr->SPLASH_IMAGE[img_idx].ulImageHeight;
					display_splash_logo(fb_display);
				}

				if(emmc_read(ptn + BYTE_TO_SECTOR(splash_hdr->SPLASH_IMAGE[img_idx].ulImageAddr),
							splash_hdr->SPLASH_IMAGE[img_idx].ulImageSize, fb_display->base)){

					fbcon_clear();
				}
			}
		}
	}

	return 0;

}

void splash_image(char *partition) {
#if defined(BOOTSD_INCLUDE)
	splash_image_sdmmc(partition);
#elif defined(TNFTL_V8_INCLUDE)
	splash_image_nand_v8(partition);
#else
	splash_image_nand_v7(partition);
#endif
}

#if defined(BOOTSD_INCLUDE)
#if defined(TARGET_BOARD_DAUDIO)
//for convert D-audio custom splash (bitmap images...)
static int get_splash_image_type(char *name)
{
	int i, size = sizeof(splash_image_names) / sizeof(int);

	for (i = 0; i < size; i++)
	{
		if (strcmp(name, splash_image_names[i]) == 0)
			break;	//success to find image type
	}

	return i;
}

int splash_image_load_sdmmc(char *partition, struct fbcon_config *fb_cfg)
{
	int type = get_splash_image_type(partition);

	dprintf(INFO, "%s image type[%d] %s \n", __func__, type, splash_image_names[type]);

	if (type != IMAGE_TYPE_MAX)
		return bitmap_to_fb(type, fb_cfg);
	else return -1;
}
#else	//TCC origin
int splash_image_load_sdmmc(char *partition, struct fbcon_config *fb_cfg)
{
	SPLASH_IMAGE_Header_info_t *splash_hdr = (void*)splash;
	struct fbcon_config *fb_display = NULL;
	int img_idx = 0;
	unsigned int page_size = flash_page_size();
	unsigned int page_mask = page_size -1;

	unsigned long long ptn = 0;

	dprintf(INFO, "partition : %s\n", partition);

	ptn = emmc_ptn_offset("splash");
	if(ptn == 0){
		dprintf(CRITICAL, "ERROR : No splash partition found !\n");
		return -1;
	}else{

		if(!emmc_read(ptn, page_size, splash_hdr)){
			if(strcmp(splash_hdr->ucPartition, SPLASH_TAG)){
				dprintf(CRITICAL, "Splash TAG Is Mismatched\n");
				return -1;
			}
		}
		img_idx = get_splash_index(splash_hdr, partition);
		dprintf(SPEW, "image idx = %d \n" ,img_idx);

		if(img_idx < 0){
			dprintf(CRITICAL, " there is no image [%s]\n", partition);
		}else{

			fb_cfg->width = splash_hdr->SPLASH_IMAGE[img_idx].ulImageWidth;
			fb_cfg->height = splash_hdr->SPLASH_IMAGE[img_idx].ulImageHeight;

			emmc_read(ptn + BYTE_TO_SECTOR(splash_hdr->SPLASH_IMAGE[img_idx].ulImageAddr),
					splash_hdr->SPLASH_IMAGE[img_idx].ulImageSize, fb_cfg->base);
		}

	}

	return 0;
}
#endif
#endif
