/****************************************************************************

 *   FileName    : tcc_cm3_control.h

 *   Description : Telechips Cortex M3 Driver

 ****************************************************************************

 *

 *   TCC Version 1.0

 *   Copyright (c) Telechips Inc.

 *   All rights reserved 

 

This source code contains confidential information of Telechips.

Any unauthorized use without a written permission of Telechips including not limited to re-distribution in source or binary form is strictly prohibited.

This source code is provided ¡°AS IS¡± and nothing contained in this source code shall constitute any express or implied warranty of any kind, including without limitation, any warranty of merchantability, fitness for a particular purpose or non-infringement of any patent, copyright or other third party intellectual property right. No warranty is made, express or implied, regarding the information¡¯s accuracy, completeness, or performance. 

In no event shall Telechips be liable for any claim, damages or other liability arising from, out of or in connection with this source code or the use in the source code. 

This source code is provided subject to the terms of a Mutual Non-Disclosure Agreement between Telechips and Company.

*

****************************************************************************/


#ifndef     _TCC_CM3_CONTROL_H_
#define     _TCC_CM3_CONTROL_H_

#define IOCTL_CM3_CTRL_OFF		    0
#define IOCTL_CM3_CTRL_ON			1
#define IOCTL_CM3_CTRL_RESET    	2
#define IOCTL_CM3_CTRL_CMD			3

#define DEVICE_NAME	"mailbox"


typedef struct{
    int iOpCode;
    int* pHandle;
    void* pParam1;
    void* pParam2; 
}t_cm3_avn_cmd;

typedef struct
{
    unsigned int uiCmdId;
    unsigned int uiParam0;
    unsigned int uiParam1;
    unsigned int uiParam2;
    unsigned int uiParam3;
    unsigned int uiParam4;
    unsigned int uiParam5;
    unsigned int uiParam6;
}ARG_CM3_CMD;

//extern int CM3_SEND_COMMAND(ARG_CM3_CMD *pInARG, ARG_CM3_CMD *pOutARG);
extern long tcc_cm3_ctrl_ioctl(unsigned int cmd, unsigned long arg);

#endif	//_TCC_CM3_CONTROL_H_

