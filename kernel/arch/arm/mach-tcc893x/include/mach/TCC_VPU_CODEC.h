
#include "TCC_VPU_C7_CODEC.h"

#ifndef _TCC_VPU_CODEC_H_
#define _TCC_VPU_CODEC_H_

#define RETCODE_MULTI_CODEC_EXIT_TIMEOUT	99

#ifndef SZ_1M
#define SZ_1M           (1024*1024)
#endif
#define ARRAY_MBYTE(x)            ((((x) + (SZ_1M-1))>> 20) << 20)

#define VPU_ENC_PUT_HEADER_SIZE (16*1024)
#define VPU_SW_ACCESS_REGION_SIZE ARRAY_MBYTE(WORK_CODE_PARA_BUF_SIZE + VPU_ENC_PUT_HEADER_SIZE)

typedef struct dec_user_info_t
{
	unsigned int  bitrate_mbps;				//!< video BitRate (Mbps)
	unsigned int  frame_rate;				//!< video FrameRate (fps)
	unsigned int  m_bJpegOnly;				//!< If this is set, content is jpeg only file (ex. **.jpg)
	unsigned int  jpg_length;	
	unsigned int  jpg_ScaleRatio; 			////!< 0 ( Original Size )
											//!< 1 ( 1/2 Scaling Down )
											//!< 2 ( 1/4 Scaling Down )
											//!< 3 ( 1/8 Scaling Down )	
}dec_user_info_t;

#endif
