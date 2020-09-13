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

#ifndef     _TCC_AVN_PROC_H_
#define     _TCC_AVN_PROC_H_

#include "tcc_cm3_control.h"

typedef enum {
	HW_TIMER_TEST 					= 0x70,
	GET_EARLY_CAMERA_STATUS 			= 0x71,
	SET_EARLY_CAMERA_STOP 			= 0x72,
	SET_EARLY_CAMERA_DISPLAY_OFF 	= 0x73,
	SET_REAR_CAMERA_RGEAR_DETECT 	= 0x74,
	SET_EARLY_CAMERA_HANDOVER 		= 0x75,
	SET_EARLY_CAMERA_BLACK_SCREEN 	= 0x76,
} CM3_AVN_CMD;


extern int CM3_AVN_Proc(unsigned long arg);
extern int tcc_early_camera_stop(void);
extern int tcc_early_camera_display_off(void);

#endif	//_TCC_AVN_PROC_H_

