/*
 * linux/arch/arm/mach-tcc893x/include/mach/vioc_plugin_tcc892x.h
 * Author:  <linux@telechips.com>
 * Created: June 10, 2008
 * Description: TCC VIOC h/w block 
 *
 * Copyright (C) 2008-2009 Telechips
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __VIOC_PLUGIN_H__
#define	__VIOC_PLUGIN_H__

////////////////////////////////////////////////////////////////////////////////
//
//	Plug-In/Out Related
//

#define	VIOC_SC0						(0)
#define	VIOC_SC1						(1)
#define	VIOC_SC2						(2)
#define	VIOC_SC3						(3)
#define	VIOC_SC_RDMA_00			(00)
#define	VIOC_SC_RDMA_01			(01)
#define	VIOC_SC_RDMA_02			(02)
#define	VIOC_SC_RDMA_03			(03)
#define	VIOC_SC_RDMA_04			(04)
#define	VIOC_SC_RDMA_05			(05)
#define	VIOC_SC_RDMA_06			(06)
#define	VIOC_SC_RDMA_07			(07)
#define	VIOC_SC_RDMA_08			(08)
#define	VIOC_SC_RDMA_09			(09)
#define	VIOC_SC_RDMA_10			(10)
#define	VIOC_SC_RDMA_11			(11)
#define	VIOC_SC_RDMA_12			(12)
#define	VIOC_SC_RDMA_13			(13)
#define	VIOC_SC_RDMA_14			(14)
#define	VIOC_SC_RDMA_15			(15)
#define	VIOC_SC_VIN_00 			(16)
#define	VIOC_SC_RDMA_16			(17)
#define	VIOC_SC_VIN_01 			(18)
#define	VIOC_SC_RDMA_17			(19)
#define	VIOC_SC_WDMA_00			(20)
#define	VIOC_SC_WDMA_01			(21)
#define	VIOC_SC_WDMA_02			(22)
#define	VIOC_SC_WDMA_03			(23)
#define	VIOC_SC_WDMA_04			(24)
#define	VIOC_SC_WDMA_05			(25)
#define	VIOC_SC_WDMA_06			(26)
#define	VIOC_SC_WDMA_07			(27)
#define	VIOC_SC_WDMA_08			(28)

#define	VIOC_VIQE						(4)
#if 1
#define	VIOC_VIQE_RDMA_00		(00)
#define	VIOC_VIQE_RDMA_01		(01)
#define	VIOC_VIQE_RDMA_02		(02)
#define	VIOC_VIQE_RDMA_03		(03)
#define	VIOC_VIQE_RDMA_06		(04)
#define	VIOC_VIQE_RDMA_07		(05)
#define	VIOC_VIQE_RDMA_10		(06)
#define	VIOC_VIQE_RDMA_11		(07)
#define	VIOC_VIQE_RDMA_12		(8)
#define	VIOC_VIQE_RDMA_14		(9)
#define	VIOC_VIQE_VIN_00 		(10)
#define	VIOC_VIQE_RDMA_16		(11)
#define	VIOC_VIQE_VIN_01 		(12)
#define	VIOC_VIQE_RDMA_17		(13)
#else
#define	VIOC_VIQE_RDMA_02		(00)
#define	VIOC_VIQE_RDMA_03		(01)
#define	VIOC_VIQE_RDMA_06		(02)
#define	VIOC_VIQE_RDMA_07		(03)
#define	VIOC_VIQE_RDMA_10		(04)
#define	VIOC_VIQE_RDMA_11		(05)
#define	VIOC_VIQE_RDMA_12		(06)
#define	VIOC_VIQE_VIN_00 		(07)
#define	VIOC_VIQE_VIN_01 		(08)
#endif

#define	VIOC_DEINTLS					(5)
#define	VIOC_DEINTLS_RDMA_00	(VIOC_VIQE_RDMA_00		)
#define	VIOC_DEINTLS_RDMA_01	(VIOC_VIQE_RDMA_01		)
#define	VIOC_DEINTLS_RDMA_02	(VIOC_VIQE_RDMA_02		)
#define	VIOC_DEINTLS_RDMA_03	(VIOC_VIQE_RDMA_03		)
#define	VIOC_DEINTLS_RDMA_06	(VIOC_VIQE_RDMA_06		)
#define	VIOC_DEINTLS_RDMA_07	(VIOC_VIQE_RDMA_07		)
#define	VIOC_DEINTLS_RDMA_10	(VIOC_VIQE_RDMA_10		)
#define	VIOC_DEINTLS_RDMA_11	(VIOC_VIQE_RDMA_11		)
#define	VIOC_DEINTLS_RDMA_12	(VIOC_VIQE_RDMA_12		)
#define	VIOC_DEINTLS_RDMA_14	(VIOC_VIQE_RDMA_14		)
#define	VIOC_DEINTLS_VIN_00 	(VIOC_VIQE_VIN_00 		)
#define	VIOC_DEINTLS_RDMA_16	(VIOC_VIQE_RDMA_16		)
#define	VIOC_DEINTLS_VIN_01 	(VIOC_VIQE_VIN_01 		)
#define	VIOC_DEINTLS_RDMA_17	(VIOC_VIQE_RDMA_17		)

#define	VIOC_FILT2D 					(6)
#define	VIOC_FILT2D_SC0_IN		(0)
#define	VIOC_FILT2D_SC0_OUT		(1)
#define	VIOC_FILT2D_SC1_IN		(2)
#define	VIOC_FILT2D_SC1_OUT		(3)
#define	VIOC_FILT2D_SC2_IN		(4)
#define	VIOC_FILT2D_SC2_OUT		(5)
#define	VIOC_FILT2D_SC3_IN		(6)
#define	VIOC_FILT2D_SC3_OUT		(7)

#define	VIOC_FCDEC0 					(7 )
#define	VIOC_FCDEC1 					(8 )
#define	VIOC_FCDEC2 					(9 )
#define	VIOC_FCDEC3 					(10)
#define	VIOC_FCDEC_RDMA_00		(00)
#define	VIOC_FCDEC_RDMA_01		(01)
#define	VIOC_FCDEC_RDMA_02		(02)
#define	VIOC_FCDEC_RDMA_03		(03)
#define	VIOC_FCDEC_RDMA_04		(04)
#define	VIOC_FCDEC_RDMA_05		(05)
#define	VIOC_FCDEC_RDMA_06		(06)
#define	VIOC_FCDEC_RDMA_07		(07)
#define	VIOC_FCDEC_RDMA_08		(08)
#define	VIOC_FCDEC_RDMA_09		(09)
#define	VIOC_FCDEC_RDMA_10		(10)
#define	VIOC_FCDEC_RDMA_11		(11)
#define	VIOC_FCDEC_RDMA_12		(12)
#define	VIOC_FCDEC_RDMA_13		(13)
#define	VIOC_FCDEC_RDMA_14		(14)
#define	VIOC_FCDEC_RDMA_15		(15)
#define	VIOC_FCDEC_RDMA_16		(16)
#define	VIOC_FCDEC_RDMA_17		(17)

#define	VIOC_FCENC0 					(11)
#define	VIOC_FCENC1 					(12)
#define	VIOC_FCENC_WDMA_00		(00)
#define	VIOC_FCENC_WDMA_01		(01)
#define	VIOC_FCENC_WDMA_02		(02)
#define	VIOC_FCENC_WDMA_03		(03)
#define	VIOC_FCENC_WDMA_04		(04)
#define	VIOC_FCENC_WDMA_05		(05)
#define	VIOC_FCENC_WDMA_06		(06)
#define	VIOC_FCENC_WDMA_07		(07)
#define	VIOC_FCENC_WDMA_08		(08)

#define	VIOC_FDELAY0					(13)
#define	VIOC_FDELAY1					(14)
#define	VIOC_FDELAY_SC0_OUT		(0)
#define	VIOC_FDELAY_SC1_OUT		(1)
#define	VIOC_FDELAY_SC2_OUT		(2)
#define	VIOC_FDELAY_SC3_OUT		(3)

#define	VIOC_DEBLOCK					(15)
#define	VIOC_DEBLOCK_FILT0_IN	(0)
#define	VIOC_DEBLOCK_FILT0_OUT	(1)
#define	VIOC_DEBLOCK_FILT1_IN	(2)
#define	VIOC_DEBLOCK_FILT1_OUT	(3)
#define	VIOC_DEBLOCK_FILT2_IN	(4)
#define	VIOC_DEBLOCK_FILT2_OUT	(5)
#define	VIOC_DEBLOCK_FILT3_IN	(6)
#define	VIOC_DEBLOCK_FILT3_OUT	(7)

/* VIOC DRIVER STATUS TYPE */
#define	VIOC_DEVICE_INVALID      	(-2)
#define	VIOC_DEVICE_BUSY 			(-1)
#define	VIOC_DEVICE_CONNECTED 		( 0)

/* VIOC DRIVER ERROR TYPE */
#define VIOC_DRIVER_ERR_INVALID 	(-3)
#define VIOC_DRIVER_ERR_BUSY 		(-2)
#define VIOC_DRIVER_ERR 			(-1)
#define VIOC_DRIVER_NOERR 			( 0)

/* VIOC PATH STATUS TYPE */
#define VIOC_PATH_DISCONNECTED			(0)		// 
#define VIOC_PATH_CONNECTING  			(1)		// 
#define VIOC_PATH_CONNECTED   			(2)		// 
#define VIOC_PATH_DISCONNECTING			(3)		// 

#define VIOC_WMIX0 						(0)
#define VIOC_WMIX1 						(1)
#define VIOC_WMIX2 						(2)
#define VIOC_WMIX3 						(3)
#define VIOC_WMIX4 						(4)
#define VIOC_WMIX5 						(5)
#define VIOC_WMIX6 						(6)
#define	VIOC_WMIX_RDMA_00		(00)
#define	VIOC_WMIX_RDMA_01		(01)
#define	VIOC_WMIX_RDMA_02		(02)
#define	VIOC_WMIX_RDMA_03		(03)
#define	VIOC_WMIX_RDMA_04		(04)
#define	VIOC_WMIX_RDMA_05		(05)
#define	VIOC_WMIX_RDMA_06		(06)
#define	VIOC_WMIX_RDMA_07		(07)
#define	VIOC_WMIX_RDMA_08		(08)
#define	VIOC_WMIX_RDMA_09		(09)
#define	VIOC_WMIX_RDMA_10		(10)
#define	VIOC_WMIX_RDMA_11		(11)
#define	VIOC_WMIX_RDMA_12		(12)
#define	VIOC_WMIX_RDMA_13		(13)
#define	VIOC_WMIX_RDMA_14		(14)
#define	VIOC_WMIX_RDMA_15		(15)
#define	VIOC_WMIX_VIN_00 		(16)
#define	VIOC_WMIX_RDMA_16		(17)
#define	VIOC_WMIX_VIN_01 		(18)
#define	VIOC_WMIX_RDMA_17		(19)
#define	VIOC_WMIX_WDMA_00		(20)
#define	VIOC_WMIX_WDMA_01		(21)
#define	VIOC_WMIX_WDMA_02		(22)
#define	VIOC_WMIX_WDMA_03		(23)
#define	VIOC_WMIX_WDMA_04		(24)
#define	VIOC_WMIX_WDMA_05		(25)
#define	VIOC_WMIX_WDMA_06		(26)
#define	VIOC_WMIX_WDMA_07		(27)
#define	VIOC_WMIX_WDMA_08		(28)

#define	VIOC_LUT_RDMA_00		(00)
#define	VIOC_LUT_RDMA_01		(01)
#define	VIOC_LUT_RDMA_02		(02)
#define	VIOC_LUT_RDMA_03		(03)
#define	VIOC_LUT_RDMA_04		(04)
#define	VIOC_LUT_RDMA_05		(05)
#define	VIOC_LUT_RDMA_06		(06)
#define	VIOC_LUT_RDMA_07		(07)
#define	VIOC_LUT_RDMA_08		(08)
#define	VIOC_LUT_RDMA_09		(09)
#define	VIOC_LUT_RDMA_10		(10)
#define	VIOC_LUT_RDMA_11		(11)
#define	VIOC_LUT_RDMA_12		(12)
#define	VIOC_LUT_RDMA_13		(13)
#define	VIOC_LUT_RDMA_14		(14)
#define	VIOC_LUT_RDMA_15		(15)
#define	VIOC_LUT_VIN_00 			(16)
#define	VIOC_LUT_RDMA_16		(17)
#define	VIOC_LUT_VIN_01 			(18)
#define	VIOC_LUT_RDMA_17		(19)
#define	VIOC_LUT_WDMA_00		(20)
#define	VIOC_LUT_WDMA_01		(21)
#define	VIOC_LUT_WDMA_02		(22)
#define	VIOC_LUT_WDMA_03		(23)
#define	VIOC_LUT_WDMA_04		(24)
#define	VIOC_LUT_WDMA_05		(25)
#define	VIOC_LUT_WDMA_06		(26)
#define	VIOC_LUT_WDMA_07		(27)
#define	VIOC_LUT_WDMA_08		(28)

//
//	End of Plug-In/Out
//
////////////////////////////////////////////////////////////////////////////////

#endif



