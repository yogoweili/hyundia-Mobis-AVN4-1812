#include <platform/reg_physical.h>
#include <platform/globals.h>

#ifndef __VIOC_LUT_H__
#define	__VIOC_LUT_H__


/* Interface APIs */

extern void VIOC_LUT_Set_value(VIOC_LUT *pLUT, unsigned int select,  unsigned int *LUT_Value);
extern void VIOC_LUT_Plugin (VIOC_LUT *pLUT, unsigned int lut_sel, unsigned int plug_in);
extern void VIOC_LUT_Enable (VIOC_LUT *pLUT, unsigned int lut_sel, unsigned int enable);
extern void VIOC_LUT_SetMain(VIOC_LUT *pLUT, unsigned int sel, unsigned int plugin, unsigned int inv, unsigned int bright_value, unsigned int onoff);

#endif
