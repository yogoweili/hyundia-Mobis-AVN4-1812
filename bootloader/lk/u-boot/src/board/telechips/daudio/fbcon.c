/*
 * Copyright (c) 2008, Google Inc.
 * All rights reserved.
 *
 * Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
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

#include "fbcon.h"
#include "font5x12.h"
#include <configs/daudio.h>

struct pos {
	int x;
	int y;
};

static struct fbcon_config fb_config = {
  (void*)FB_BASE_ADDRH,
  DISP_WIDTH,
  DISP_HEIGHT,
  DISP_WIDTH,
  FB_BPP,
  FB_FORMAT_RGB565
};
static struct fbcon_config *config = NULL;

#define RGB565_WHITE            0x00FFFFFF
#define RGB565_BLACK            0x00000000

#define FONT_WIDTH              5
#define FONT_HEIGHT             12

static struct pos               cur_pos;
static struct pos               max_pos;
static unsigned int BGCOLOR;
static unsigned int FGCOLOR;
 
static void fbcon_flush(void){}

static void fbcon_drawglyph(unsigned int *pixels, unsigned int paint, unsigned stride,
			    unsigned *glyph)
{
	unsigned x, y, data;
	stride -= FONT_WIDTH;

	data = glyph[0];
	for (y = 0; y < (FONT_HEIGHT / 2); ++y) {
		for (x = 0; x < FONT_WIDTH; ++x) {
			if (data & 1)
				*pixels = paint;
			data >>= 1;
			pixels++;
		}
		pixels += stride;
	}

	data = glyph[1];
	for (y = 0; y < (FONT_HEIGHT / 2); y++) {
		for (x = 0; x < FONT_WIDTH; x++) {
			if (data & 1)
				*pixels = paint;
			data >>= 1;
			pixels++;
		}
		pixels += stride;
	}
}

static void fbcon_scroll_up(void)
{
	unsigned int *dst = config->base;
	unsigned int *src = dst + (config->width *2* FONT_HEIGHT);
	unsigned count = config->width *2 * (config->height - FONT_HEIGHT);

	while(count--){
		*dst++ = *src++;
	}

	count = config->width * FONT_HEIGHT;
	while(count--) {
		*dst++ = BGCOLOR;
	}

	fbcon_flush();
}

void fbcon_clear(void)
{
	unsigned int *dst = config->base;
	unsigned count = config->width * config->height;

	if (config->bpp == 32)
		count *= 2;

	cur_pos.x = 0;
	cur_pos.y = 0;

	while (count--)
		*dst++ = BGCOLOR;
}

void fbcon_clear_line(void)
{
	unsigned int *dst = config->base;
	unsigned count = config->width * FONT_HEIGHT;

	if (config->bpp == 32)
		count *= 2;

    dst += cur_pos.y * FONT_HEIGHT * config->width;
	cur_pos.x = 0;

	while (count--)
		*dst++ = BGCOLOR;
}

static void fbcon_set_colors(unsigned bg, unsigned fg)
{
	BGCOLOR = bg;
	FGCOLOR = fg;
}

static int fbcon_setup(struct fbcon_config *_config)
{
	unsigned int bg;
	unsigned int fg;

	config = _config;
    if (config == NULL) {
        return -1;
    }

	switch (config->format) {
	case FB_FORMAT_RGB565:
		bg = RGB565_BLACK;
		fg = RGB565_WHITE;
		break;

	default:
		printf("unknown framebuffer pixel format\n");
		break;
	}

	fbcon_set_colors(bg, fg);

	fbcon_clear();

	cur_pos.x = cur_pos.y = 0;
	max_pos.x = config->width / (FONT_WIDTH + 1);
	max_pos.y = (config->height - 1) / FONT_HEIGHT;

    return 0;
}

struct fbcon_config* fbcon_display(void)
{
    return config;
}

void fbcon_putc(char c)
{
	unsigned int *pixels;

	if((unsigned char)c > 127){
		return;
    }
	if((unsigned char)c < 32) {
		if(c == '\n')
			goto newline;
		else if (c == '\r'){
            fbcon_clear_line();
        }
		return;
	}

	pixels = config->base;
	pixels += cur_pos.y * FONT_HEIGHT * config->width;
	pixels += cur_pos.x * (FONT_WIDTH + 1);
	fbcon_drawglyph(pixels, FGCOLOR, config->stride,
			font5x12 + (c - 32) * 2);

	cur_pos.x++;
	if (cur_pos.x < max_pos.x)
		return;

newline:
	cur_pos.y++;
	cur_pos.x = 0;
	if(cur_pos.y >= max_pos.y) {
		cur_pos.y = max_pos.y - 1;
		fbcon_scroll_up();
	} else
		fbcon_flush();
}

int fbcon_init()
{
    int ret = 0;
    if((ret = fbcon_setup(&fb_config)) < 0){
        return ret;
    }
    return 0;
}

int fbcon_printf(const char *str)
{
    if(str == NULL || config == NULL)
        return -1;

    while(*str != 0){
        fbcon_putc(*str++);
    }
    return 0;
}

