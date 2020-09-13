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

#ifndef _TCC_CMMB_VIDEO_INTERFACE_H_
#define _TCC_CMMB_VIDEO_INTERFACE_H_

//#define FEATURE_CMMB_DISPLAY_TEST

#define ALIGNED_BUFF(buf, mul) 		(((unsigned int)buf + (mul-1)) & ~(mul-1))

typedef struct {
	unsigned int p_Y;  
	unsigned int p_Cb;
	unsigned int p_Cr;
} CMMB_VIDEO_BUFFER;

extern void tcc_cmmb_video_init_vioc_path(void);
extern irqreturn_t tcc_cmmb_video_irq_handler(int irq, void *client_data);
extern int tcc_cmmb_video_interface_start_stream(CMMB_PREVIEW_IMAGE_INFO* cmmb_info);
extern int tcc_cmmb_video_interface_stop_stream(void);
extern int tcc_cmmb_video_set_buffer_addr(CMMB_IMAGE_BUFFER_INFO* buf_info);
extern int tcc_cmmb_video_set_qbuf(void);
extern int tcc_cmmb_video_get_dqbuf(void);

#endif // _TCC_CMMB_VIDEO_INTERFACE_H_



