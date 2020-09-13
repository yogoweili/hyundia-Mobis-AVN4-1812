/****************************************************************************
One line to give the program's name and a brief idea of what it does.
Copyright (C) 2013 Telechips Inc.

This program is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place,
Suite 330, Boston, MA 02111-1307 USA
****************************************************************************/

#define IOCTL_TCC_CMMB_VIDEO_START 				0
#define IOCTL_TCC_CMMB_VIDEO_STOP 				1
#define IOCTL_TCC_CMMB_VIDEO_SET_ADDR 			2
#define IOCTL_TCC_CMMB_VIDEO_QBUF 				3
#define IOCTL_TCC_CMMB_VIDEO_DQBUF 				4


typedef enum {
	CMMB_PRE_INIT,
	CMMB_PRE_START,
	CMMB_PRE_STOP,
} CMMB_SENSOR_MODE;

typedef enum {
	CMMB_POLLING,
	CMMB_INTERRUPT,
	CMMB_NOWAIT
} CMMB_INTERRUPT_TYPE;

typedef enum {
	CMMB_IMG_FMT_YUV422_16BIT = 0,
	CMMB_IMG_FMT_YUV422_8BIT,
	CMMB_IMG_FMT_YUVK4444_16BIT,
	CMMB_IMG_FMT_YUVK4224_24BIT,
	CMMB_IMG_FMT_RGBK4444_16BIT = 8,
	CMMB_IMG_FMT_RGB444_24BIT,
	CMMB_IMG_FMT_MAX
} CMMB_IMAGE_FORMAT_TYPE;

typedef struct {
	unsigned int cmmb_interrupt;
	unsigned int cmmb_img_fmt;
	unsigned int cmmb_img_width;
	unsigned int cmmb_img_height;
	unsigned int cmmb_img_left;
	unsigned int cmmb_img_top;
	unsigned int cmmb_img_right;
	unsigned int cmmb_img_bottom;
	unsigned int cmmb_mix_mode;
} CMMB_PREVIEW_IMAGE_INFO;

typedef struct {
	unsigned int width;
	unsigned int height;
	unsigned int buffer_index;
	unsigned int buffer_addr;
} CMMB_IMAGE_BUFFER_INFO;
