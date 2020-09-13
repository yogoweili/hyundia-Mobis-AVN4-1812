/******************************************************************************
*
*  (C)Copyright All Rights Reserved by Telechips Inc.
*
*  This material is confidential and shall remain as such.
*  Any unauthorized use, distribution, reproduction is strictly prohibited.
*
*******************************************************************************
*
*  File Name   : vioc_lut.c
*
*  Description : display control component control module
*
*******************************************************************************
*
*  yyyy/mm/dd     ver            descriptions                Author
*	---------	--------   ---------------       -----------------
*    2011/08/15     0.1            created                      hskim
*******************************************************************************/

#include "vioc_lut.h"
#if 1
#define dprintk(msg...)	 { printf( "tca_hdmi: " msg); }
#else
#define dprintk(msg...)	 
#endif

void VIOC_LUT_Set_value(VIOC_LUT *pLUT, unsigned int select,  unsigned int *LUT_Value)
{
	unsigned int	*pLUT_TABLE = (unsigned int *)HwVIOC_LUT_TAB;
	BITCSET(pLUT->uSEL.nREG, 0x0000000F, select);

	memcpy(pLUT_TABLE, LUT_Value, LUT_TABLE_SIZE * 4);

	dprintk("%s select: %d, lut_value: 0x%x, pLUT_TABLE: 0x%x LUT_TABLE_SIZE: %d\n"
			, __func__ , select, LUT_Value, pLUT_TABLE, LUT_TABLE_SIZE);
}

void VIOC_LUT_Plugin (VIOC_LUT *pLUT, unsigned int lut_sel, unsigned int plug_in)
{
	dprintk("%s lut_sel=%d plug_in=%d\n",__func__,lut_sel, plug_in );
	
	switch (lut_sel)
	{
		case (VIOC_LUT_DEV0)  :
			pLUT->uDEV0CFG.bREG.SEL	= plug_in;
			break;

		case (VIOC_LUT_DEV1)  :
			pLUT->uDEV1CFG.bREG.SEL	= plug_in;
			break;
			
		case (VIOC_LUT_DEV2)  :
			pLUT->uDEV2CFG.bREG.SEL	= plug_in;
			break;

		case (VIOC_LUT_COMP0) :
			//pLUT->uCOMP0CFG.bREG.SEL= plug_in;
			BITCSET(pLUT->uCOMP0CFG.nREG, 0x000000FF,  plug_in);

			break;
		case (VIOC_LUT_COMP1) :
			//pLUT->uCOMP1CFG.bREG.SEL= plug_in;
			BITCSET(pLUT->uCOMP1CFG.nREG, 0x000000FF,  plug_in);
			break;

		case (VIOC_LUT_COMP2) :
			BITCSET(pLUT->uCOMP2CFG.nREG, 0x000000FF,  plug_in);
			break;
		case (VIOC_LUT_COMP3) :
			BITCSET(pLUT->uCOMP3CFG.nREG, 0x000000FF,  plug_in);
			break;

		default:
			break;
	}
}

void VIOC_LUT_Enable (VIOC_LUT *pLUT, unsigned int lut_sel, unsigned int enable)
{
	dprintk("[%s ] lut_sel=[%d] enable=[%d] \n",__func__,lut_sel, enable );

	switch (lut_sel)
	{
		case (VIOC_LUT_DEV0)  :
			pLUT->uDEV0CFG.bREG.EN	= enable;
			break;

		case (VIOC_LUT_DEV1)  :
			pLUT->uDEV1CFG.bREG.EN	= enable;
			break;
			
		case (VIOC_LUT_DEV2)  :
			pLUT->uDEV2CFG.bREG.EN = enable;
			break;

		case (VIOC_LUT_COMP0) :
			BITCSET(pLUT->uCOMP0CFG.nREG, 0x80000000,  enable << 31);
			break;

		case (VIOC_LUT_COMP1) :
			BITCSET(pLUT->uCOMP1CFG.nREG, 0x80000000,  enable << 31);
			break;

		case (VIOC_LUT_COMP2) :
			BITCSET(pLUT->uCOMP2CFG.nREG, 0x80000000,  enable << 31);
			break;

		case (VIOC_LUT_COMP3) :
			BITCSET(pLUT->uCOMP3CFG.nREG, 0x80000000,  enable << 31);
			break;

		default:
			break;
	}
}

void VIOC_LUT_SetMain(VIOC_LUT *pLUT, unsigned int sel, unsigned int plugin, unsigned int inv, unsigned int bright_value, unsigned int onoff)
{
	dprintk("[%s ] sel=[%d] plugin=[%d] inv=[%d] bright_value=[%d] onoff=[%d] \n",__func__,sel, plugin, inv, bright_value, onoff );

	if(onoff){
		VIOC_LUT_Set(pLUT, sel, inv, bright_value);
		VIOC_LUT_Plugin(pLUT, sel, plugin);
		VIOC_LUT_Enable(pLUT, sel, TRUE);
	}
	else
	{
		VIOC_LUT_Set(pLUT, sel, inv, 0);
		VIOC_LUT_Plugin(pLUT, sel, plugin);
		VIOC_LUT_Enable(pLUT, sel, FALSE);
	}
}
