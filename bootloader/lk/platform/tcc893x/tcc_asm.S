/****************************************************************************

 *   FileName    : tcc_asm.S

 *   Description : POWER MANAGEMENT FUNCTIONS

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


#ifndef __ALIGN
#define __ALIGN		.align 4,0x90
#define __ALIGN_STR	".align 4,0x90"
#endif


#define ALIGN __ALIGN

#ifndef ENTRY
#define ENTRY(name) \
  .globl name; \
  name:
#endif

//-------------------------------------------------------------------------------
//
//	FUNCTION :
//
//	DESCRIPTION :
//
//-------------------------------------------------------------------------------
ENTRY(IO_ARM_ChangeStackSRAM)

	mov		r0, r13
	ldr		r13, =0x10000800 //;SHUTDOWN_STACK_ADDR+SHUTDOWN_STACK_SIZE
	mov		pc, lr


//-------------------------------------------------------------------------------
//
//	FUNCTION :
//
//	DESCRIPTION :
//
//-------------------------------------------------------------------------------  
ENTRY(IO_ARM_RestoreStackSRAM)

    mov		r13, r0    
    mov		pc, lr

