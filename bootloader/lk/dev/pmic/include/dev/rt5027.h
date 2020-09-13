/*
 * Copyright (c) 2013 Telechips, Inc.
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

#ifndef _DEV_PMIC_RT5027_H_
#define _DEV_PMIC_RT5027_H_

typedef enum {
	RT5027_ID_DCDC1 = 0,
	RT5027_ID_DCDC2,
	RT5027_ID_DCDC3,
	RT5027_ID_LDO1,
	RT5027_ID_LDO2,
	RT5027_ID_LDO3,
	RT5027_ID_LDO4,
	RT5027_ID_LDO5,
	RT5027_ID_LDO6,
	RT5027_ID_LDO7,
	RT5027_ID_LDO8,	
	RT5027_ID_BOOST,	
	RT5027_MAX
} rt5027_src_id;

extern void rt5027_init(int i2c_ch);
extern int rt5027_set_voltage(rt5027_src_id id, unsigned int mV);
extern int rt5027_set_power(rt5027_src_id id, unsigned int onoff);

#endif /* _DEV_PMIC_RT5027_H_ */

