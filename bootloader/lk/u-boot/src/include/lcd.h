/*
 * MPC823 and PXA LCD Controller
 *
 * Modeled after video interface by Paolo Scaffardi
 *
 *
 * (C) Copyright 2001
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _LCD_H_
#define _LCD_H_

extern char lcd_is_enabled;

extern int lcd_line_length;

extern struct vidinfo panel_info;

void lcd_ctrl_init(void *lcdbase);
void lcd_enable(void);

/* setcolreg used in 8bpp/16bpp; initcolregs used in monochrome */
void lcd_setcolreg(ushort regno, ushort red, ushort green, ushort blue);
void lcd_initcolregs(void);

int lcd_getfgcolor(void);

/* gunzip_bmp used if CONFIG_VIDEO_BMP_GZIP */
struct bmp_image *gunzip_bmp(unsigned long addr, unsigned long *lenp,
			     void **alloc_addr);
int bmp_display(ulong addr, int x, int y);

/**
 * Set whether we need to flush the dcache when changing the LCD image. This
 * defaults to off.
 *
 * @param flush		non-zero to flush cache after update, 0 to skip
 */
void lcd_set_flush_dcache(int flush);

#if defined CONFIG_MPC823
/*
 * LCD controller stucture for MPC823 CPU
 */
typedef struct vidinfo {
	ushort	vl_col;		/* Number of columns (i.e. 640) */
	ushort	vl_row;		/* Number of rows (i.e. 480) */
	ushort	vl_width;	/* Width of display area in millimeters */
	ushort	vl_height;	/* Height of display area in millimeters */

	/* LCD configuration register */
	u_char	vl_clkp;	/* Clock polarity */
	u_char	vl_oep;		/* Output Enable polarity */
	u_char	vl_hsp;		/* Horizontal Sync polarity */
	u_char	vl_vsp;		/* Vertical Sync polarity */
	u_char	vl_dp;		/* Data polarity */
	u_char	vl_bpix;	/* Bits per pixel, 0 = 1, 1 = 2, 2 = 4, 3 = 8 */
	u_char	vl_lbw;		/* LCD Bus width, 0 = 4, 1 = 8 */
	u_char	vl_splt;	/* Split display, 0 = single-scan, 1 = dual-scan */
	u_char	vl_clor;	/* Color, 0 = mono, 1 = color */
	u_char	vl_tft;		/* 0 = passive, 1 = TFT */

	/* Horizontal control register. Timing from data sheet */
	ushort	vl_wbl;		/* Wait between lines */

	/* Vertical control register */
	u_char	vl_vpw;		/* Vertical sync pulse width */
	u_char	vl_lcdac;	/* LCD AC timing */
	u_char	vl_wbf;		/* Wait between frames */
} vidinfo_t;

#elif defined(CONFIG_CPU_PXA25X) || defined(CONFIG_CPU_PXA27X) || \
	defined CONFIG_CPU_MONAHANS
/*
 * PXA LCD DMA descriptor
 */
struct pxafb_dma_descriptor {
	u_long	fdadr;		/* Frame descriptor address register */
	u_long	fsadr;		/* Frame source address register */
	u_long	fidr;		/* Frame ID register */
	u_long	ldcmd;		/* Command register */
};

/*
 * PXA LCD info
 */
struct pxafb_info {

	/* Misc registers */
	u_long	reg_lccr3;
	u_long	reg_lccr2;
	u_long	reg_lccr1;
	u_long	reg_lccr0;
	u_long	fdadr0;
	u_long	fdadr1;

	/* DMA descriptors */
	struct	pxafb_dma_descriptor *	dmadesc_fblow;
	struct	pxafb_dma_descriptor *	dmadesc_fbhigh;
	struct	pxafb_dma_descriptor *	dmadesc_palette;

	u_long	screen;		/* physical address of frame buffer */
	u_long	palette;	/* physical address of palette memory */
	u_int	palette_size;
};

/*
 * LCD controller stucture for PXA CPU
 */
typedef struct vidinfo {
	ushort	vl_col;		/* Number of columns (i.e. 640) */
	ushort	vl_row;		/* Number of rows (i.e. 480) */
	ushort	vl_width;	/* Width of display area in millimeters */
	ushort	vl_height;	/* Height of display area in millimeters */

	/* LCD configuration register */
	u_char	vl_clkp;	/* Clock polarity */
	u_char	vl_oep;		/* Output Enable polarity */
	u_char	vl_hsp;		/* Horizontal Sync polarity */
	u_char	vl_vsp;		/* Vertical Sync polarity */
	u_char	vl_dp;		/* Data polarity */
	u_char	vl_bpix;	/* Bits per pixel, 0 = 1, 1 = 2, 2 = 4, 3 = 8, 4 = 16 */
	u_char	vl_lbw;		/* LCD Bus width, 0 = 4, 1 = 8 */
	u_char	vl_splt;	/* Split display, 0 = single-scan, 1 = dual-scan */
	u_char	vl_clor;	/* Color, 0 = mono, 1 = color */
	u_char	vl_tft;		/* 0 = passive, 1 = TFT */

	/* Horizontal control register. Timing from data sheet */
	ushort	vl_hpw;		/* Horz sync pulse width */
	u_char	vl_blw;		/* Wait before of line */
	u_char	vl_elw;		/* Wait end of line */

	/* Vertical control register. */
	u_char	vl_vpw;		/* Vertical sync pulse width */
	u_char	vl_bfw;		/* Wait before of frame */
	u_char	vl_efw;		/* Wait end of frame */

	/* PXA LCD controller params */
	struct	pxafb_info pxa;
} vidinfo_t;

#elif defined(CONFIG_ATMEL_LCD) || defined(CONFIG_ATMEL_HLCD)

typedef struct vidinfo {
	ushort vl_col;		/* Number of columns (i.e. 640) */
	ushort vl_row;		/* Number of rows (i.e. 480) */
	u_long vl_clk;	/* pixel clock in ps    */

	/* LCD configuration register */
	u_long vl_sync;		/* Horizontal / vertical sync */
	u_long vl_bpix;		/* Bits per pixel, 0 = 1, 1 = 2, 2 = 4, 3 = 8, 4 = 16 */
	u_long vl_tft;		/* 0 = passive, 1 = TFT */
	u_long vl_cont_pol_low;	/* contrast polarity is low */
	u_long vl_clk_pol;	/* clock polarity */

	/* Horizontal control register. */
	u_long vl_hsync_len;	/* Length of horizontal sync */
	u_long vl_left_margin;	/* Time from sync to picture */
	u_long vl_right_margin;	/* Time from picture to sync */

	/* Vertical control register. */
	u_long vl_vsync_len;	/* Length of vertical sync */
	u_long vl_upper_margin;	/* Time from sync to picture */
	u_long vl_lower_margin;	/* Time from picture to sync */

	u_long	mmio;		/* Memory mapped registers */
} vidinfo_t;

#elif defined(CONFIG_EXYNOS_FB)

enum {
	FIMD_RGB_INTERFACE = 1,
	FIMD_CPU_INTERFACE = 2,
};

enum exynos_fb_rgb_mode_t {
	MODE_RGB_P = 0,
	MODE_BGR_P = 1,
	MODE_RGB_S = 2,
	MODE_BGR_S = 3,
};

typedef struct vidinfo {
	ushort vl_col;		/* Number of columns (i.e. 640) */
	ushort vl_row;		/* Number of rows (i.e. 480) */
	ushort vl_width;	/* Width of display area in millimeters */
	ushort vl_height;	/* Height of display area in millimeters */

	/* LCD configuration register */
	u_char vl_freq;		/* Frequency */
	u_char vl_clkp;		/* Clock polarity */
	u_char vl_oep;		/* Output Enable polarity */
	u_char vl_hsp;		/* Horizontal Sync polarity */
	u_char vl_vsp;		/* Vertical Sync polarity */
	u_char vl_dp;		/* Data polarity */
	u_char vl_bpix;		/* Bits per pixel */

	/* Horizontal control register. Timing from data sheet */
	u_char vl_hspw;		/* Horz sync pulse width */
	u_char vl_hfpd;		/* Wait before of line */
	u_char vl_hbpd;		/* Wait end of line */

	/* Vertical control register. */
	u_char	vl_vspw;	/* Vertical sync pulse width */
	u_char	vl_vfpd;	/* Wait before of frame */
	u_char	vl_vbpd;	/* Wait end of frame */
	u_char  vl_cmd_allow_len; /* Wait end of frame */

	unsigned int win_id;
	unsigned int init_delay;
	unsigned int power_on_delay;
	unsigned int reset_delay;
	unsigned int interface_mode;
	unsigned int mipi_enabled;
	unsigned int dp_enabled;
	unsigned int cs_setup;
	unsigned int wr_setup;
	unsigned int wr_act;
	unsigned int wr_hold;
	unsigned int logo_on;
	unsigned int logo_width;
	unsigned int logo_height;
	unsigned long logo_addr;
	unsigned int rgb_mode;
	unsigned int resolution;

	/* parent clock name(MPLL, EPLL or VPLL) */
	unsigned int pclk_name;
	/* ratio value for source clock from parent clock. */
	unsigned int sclk_div;

	unsigned int dual_lcd_enabled;
} vidinfo_t;

void init_panel_info(vidinfo_t *vid);

#elif defined(CONFIG_TCC893X_FB) || defined(CONFIG_TCC892X_FB)

#define ATAG_TCC_PANEL	0x54434364 /* TCCd */

enum {
	PANEL_ID_LMS350DF01,
	PANEL_ID_LMS480KF01,
	PANEL_ID_DX08D11VM0AAA,
	PANEL_ID_LB070WV6,
	PANEL_ID_CLAA104XA01CW,
	PANEL_ID_HT121WX2,
	PANEL_ID_TD043MGEB1,
	PANEL_ID_AT070TN93,
	PANEL_ID_TD070RDH11,
	PANEL_ID_N101L6,
	PANEL_ID_TW8816,
	PANEL_ID_CLAA102NA0DCW,
	PANEL_ID_ED090NA,
	PANEL_ID_KR080PA2S,
	PANEL_ID_CLAA070NP01,
	PANEL_ID_HV070WSA,
	PANEL_ID_FLD0800,
	PANEL_ID_CLAA070WP03,
	PANEL_ID_LMS700KF23,
	PANEL_ID_EJ070NA,
	PANEL_ID_LP101WX1,
	PANEL_ID_HDMI,
	PANEL_ID_HDMI_1280X720 = PANEL_ID_HDMI,
	PANEL_ID_HDMI_1920X1080,
	PANEL_ID_TVOUT_NTSC,
	PANEL_ID_TVOUT_PAL	
};

#define MAX_BACKLIGTH 		255
#define DEFAULT_BACKLIGTH 	150

#define ID_INVERT	0x01 	// Invered Data Enable(ACBIS pin)  anctive Low
#define IV_INVERT	0x02	// Invered Vertical sync  anctive Low
#define IH_INVERT	0x04	// Invered Horizontal sync	 anctive Low
#define IP_INVERT	0x08	// Invered Pixel Clock : anctive Low

typedef enum{
	TCC_LCDC_IMG_FMT_1BPP,
	TCC_LCDC_IMG_FMT_2BPP,
	TCC_LCDC_IMG_FMT_4BPP,
	TCC_LCDC_IMG_FMT_8BPP,
	TCC_LCDC_IMG_FMT_RGB332 = 8,
	TCC_LCDC_IMG_FMT_RGB444 = 9,
	TCC_LCDC_IMG_FMT_RGB565 = 10,
	TCC_LCDC_IMG_FMT_RGB555 = 11,
	TCC_LCDC_IMG_FMT_RGB888 = 12,
	TCC_LCDC_IMG_FMT_RGB666 = 13,
	TCC_LCDC_IMG_FMT_RGB888_3	= 14,		/* RGB888 - 3bytes aligned - B1[31:24],R0[23:16],G0[15:8],B0[7:0] newly supported : 3 bytes format*/
	TCC_LCDC_IMG_FMT_ARGB6666_3 = 15,		/* ARGB6666 - 3bytes aligned - A[23:18],R[17:12],G[11:6],B[5:0]newly supported : 3 bytes format */
	TCC_LCDC_IMG_FMT_COMP = 16,				// 4bytes
	TCC_LCDC_IMG_FMT_DECOMP	= (TCC_LCDC_IMG_FMT_COMP),
	TCC_LCDC_IMG_FMT_444SEP = 21,
	TCC_LCDC_IMG_FMT_UYVY = 22,		/* 2bytes	: YCbCr 4:2:2 Sequential format LSB [Y/U/Y/V] MSB : newly supported : 2 bytes format*/
	TCC_LCDC_IMG_FMT_VYUY = 23,		/* 2bytes	: YCbCr 4:2:2 Sequential format LSB [Y/V/Y/U] MSB : newly supported : 2 bytes format*/

	TCC_LCDC_IMG_FMT_YUV420SP = 24,	
	TCC_LCDC_IMG_FMT_YUV422SP = 25, 
	TCC_LCDC_IMG_FMT_YUYV = 26, 
	TCC_LCDC_IMG_FMT_YVYU = 27,
	
	TCC_LCDC_IMG_FMT_YUV420ITL0 = 28, 
	TCC_LCDC_IMG_FMT_YUV420ITL1 = 29, 
	TCC_LCDC_IMG_FMT_YUV422ITL0 = 30, 
	TCC_LCDC_IMG_FMT_YUV422ITL1 = 31, 
	TCC_LCDC_IMG_FMT_MAX
}TCC_LCDC_IMG_FMT_TYPE;

struct tcc_lcdc_image_update
{
	unsigned int Lcdc_layer;
	unsigned int enable;
	unsigned int Frame_width;
	unsigned int Frame_height;

	unsigned int Image_width;
	unsigned int Image_height;
	unsigned int offset_x; //position
	unsigned int offset_y; 	//position
	unsigned int addr0;
	unsigned int addr1;
	unsigned int addr2;
	TCC_LCDC_IMG_FMT_TYPE fmt;	//TCC_LCDC_IMG_FMT_TYPE
};

struct lcd_platform_data {
// power control pin
	unsigned power_on;
	unsigned display_on;
	unsigned bl_on;
	unsigned reset;
	unsigned lcdc_num;
	unsigned logo_data;
};

typedef struct vidinfo {
	const char *name;
	const char *manufacturer;

	struct lcd_platform_data dev;
	
	ushort id;			/* panel ID */
	ushort vl_col;		/* Number of columns (i.e. 160) */
	ushort vl_row;		/* Number of rows (i.e. 100) */
	u_char vl_bpix;		/* Bits per pixel, 0 = 1 */
	u_long clk_freq;
	u_long clk_div;
	u_long bus_width;
	u_long lpw;			/* line pulse width */
	u_long lpc;			/* line pulse count */
	u_long lswc;		/* line start wait clock */
	u_long lewc;		/* line end wait clock */
	u_long vdb;			/* back porch vsync delay */
	u_long vdf;			/* front porch vsync delay */
	u_long fpw1;		/* frame pulse width 1 */
	u_long flc1;		/* frame line count 1 */
	u_long fswc1;		/* frame start wait cycle 1 */
	u_long fewc1;		/* frame end wait cycle 1 */
	u_long fpw2;		/* frame pulse width 2 */
	u_long flc2;		/* frame line count 2 */
	u_long fswc2;		/* frame start wait cycle 2 */
	u_long fewc2;		/* frame end wait cycle 2 */
	u_long sync_invert;

	ushort	*cmap;		/* Pointer to the colormap */
	void	*priv;		/* Pointer to driver-specific data */	

	int (*init)(void);
	int (*set_power)(int on);
	int (*set_backlight_level)(int level);	
	unsigned int lcdbase;
} vidinfo_t;

void init_panel_info(vidinfo_t *vid);
void get_panel_info(vidinfo_t *vid);

#else

typedef struct vidinfo {
	ushort	vl_col;		/* Number of columns (i.e. 160) */
	ushort	vl_row;		/* Number of rows (i.e. 100) */

	u_char	vl_bpix;	/* Bits per pixel, 0 = 1 */

	ushort	*cmap;		/* Pointer to the colormap */

	void	*priv;		/* Pointer to driver-specific data */
} vidinfo_t;

#endif /* CONFIG_MPC823, CONFIG_CPU_PXA25X, CONFIG_MCC200, CONFIG_ATMEL_LCD */

extern vidinfo_t panel_info;

/* Video functions */

#if defined(CONFIG_RBC823)
void	lcd_disable(void);
#endif

void	lcd_putc(const char c);
void	lcd_puts(const char *s);
void	lcd_printf(const char *fmt, ...);
void	lcd_clear(void);
int	lcd_display_bitmap(ulong bmp_image, int x, int y);

/**
 * Get the width of the LCD in pixels
 *
 * @return width of LCD in pixels
 */
int lcd_get_pixel_width(void);

/**
 * Get the height of the LCD in pixels
 *
 * @return height of LCD in pixels
 */
int lcd_get_pixel_height(void);

/**
 * Get the number of text lines/rows on the LCD
 *
 * @return number of rows
 */
int lcd_get_screen_rows(void);

/**
 * Get the number of text columns on the LCD
 *
 * @return number of columns
 */
int lcd_get_screen_columns(void);

/**
 * Set the position of the text cursor
 *
 * @param col	Column to place cursor (0 = left side)
 * @param row	Row to place cursor (0 = top line)
 */
void lcd_position_cursor(unsigned col, unsigned row);

/* Allow boards to customize the information displayed */
void lcd_show_board_info(void);

/* Return the size of the LCD frame buffer, and the line length */
int lcd_get_size(int *line_length);

int lcd_dt_simplefb_add_node(void *blob);
int lcd_dt_simplefb_enable_existing_node(void *blob);

/************************************************************************/
/* ** BITMAP DISPLAY SUPPORT						*/
/************************************************************************/
#if defined(CONFIG_CMD_BMP) || defined(CONFIG_SPLASH_SCREEN)
# include <bmp_layout.h>
# include <asm/byteorder.h>
#endif

/*
 *  Information about displays we are using. This is for configuring
 *  the LCD controller and memory allocation. Someone has to know what
 *  is connected, as we can't autodetect anything.
 */
#define CONFIG_SYS_HIGH	0	/* Pins are active high			*/
#define CONFIG_SYS_LOW	1	/* Pins are active low			*/

#define LCD_MONOCHROME	0
#define LCD_COLOR2	1
#define LCD_COLOR4	2
#define LCD_COLOR8	3
#define LCD_COLOR16	4

/*----------------------------------------------------------------------*/
#if defined(CONFIG_LCD_INFO_BELOW_LOGO)
# define LCD_INFO_X		0
# define LCD_INFO_Y		(BMP_LOGO_HEIGHT + VIDEO_FONT_HEIGHT)
#elif defined(CONFIG_LCD_LOGO)
# define LCD_INFO_X		(BMP_LOGO_WIDTH + 4 * VIDEO_FONT_WIDTH)
# define LCD_INFO_Y		VIDEO_FONT_HEIGHT
#else
# define LCD_INFO_X		VIDEO_FONT_WIDTH
# define LCD_INFO_Y		VIDEO_FONT_HEIGHT
#endif

/* Default to 8bpp if bit depth not specified */
#ifndef LCD_BPP
# define LCD_BPP			LCD_COLOR8
#endif
#ifndef LCD_DF
# define LCD_DF			1
#endif

/* Calculate nr. of bits per pixel  and nr. of colors */
#define NBITS(bit_code)		(1 << (bit_code))
#define NCOLORS(bit_code)	(1 << NBITS(bit_code))

/************************************************************************/
/* ** CONSOLE CONSTANTS							*/
/************************************************************************/
#if LCD_BPP == LCD_MONOCHROME

/*
 * Simple black/white definitions
 */
# define CONSOLE_COLOR_BLACK	0
# define CONSOLE_COLOR_WHITE	1	/* Must remain last / highest	*/

#elif LCD_BPP == LCD_COLOR8

/*
 * 8bpp color definitions
 */
# define CONSOLE_COLOR_BLACK	0
# define CONSOLE_COLOR_RED	1
# define CONSOLE_COLOR_GREEN	2
# define CONSOLE_COLOR_YELLOW	3
# define CONSOLE_COLOR_BLUE	4
# define CONSOLE_COLOR_MAGENTA	5
# define CONSOLE_COLOR_CYAN	6
# define CONSOLE_COLOR_GREY	14
# define CONSOLE_COLOR_WHITE	15	/* Must remain last / highest	*/

#else

/*
 * 16bpp color definitions
 */
# define CONSOLE_COLOR_BLACK	0x0000
# define CONSOLE_COLOR_WHITE	0xffff	/* Must remain last / highest	*/

#endif /* color definitions */

/************************************************************************/
#ifndef PAGE_SIZE
# define PAGE_SIZE	4096
#endif

/************************************************************************/

#endif	/* _LCD_H_ */
