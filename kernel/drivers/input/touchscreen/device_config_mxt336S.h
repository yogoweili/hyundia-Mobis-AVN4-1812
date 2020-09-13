
/**********************************************************

  DEVICE   : mxT336S  0.5.6
  CUSTOMER : MOBIS
  PROJECT  : N1
  X SIZE   : X18
  Y SIZE   : Y11
  CHRGTIME : 2.59us
  X Pitch  :
  Y Pitch  :
***********************************************************/

#ifndef __MXT336S_CONFIG__
#define __MXT336S_CONFIG__

/* SPT_USERDATA_T38 INSTANCE 0 */
#define T38_USERDATA0	'1'
#define T38_USERDATA1	'5'
#define T38_USERDATA2	'0'
#define T38_USERDATA3	'5'
#define T38_USERDATA4	'0'
#define T38_USERDATA5	'8'
#define T38_USERDATA6	'0'
#define T38_USERDATA7	'0'


/* GEN_POWERCONFIG_T7 INSTANCE 0 */
#define T7_IDLEACQINT	255
#define T7_ACTVACQINT	255
#define T7_ACTV2IDLETO	255
#define T7_CFG			0


/* _GEN_ACQUISITIONCONFIG_T8 INSTANCE 0 */
#define T8_CHRGTIME			27      /* 6 - 60  * 83 ns */
#define T8_RESERVED			0
#define T8_TCHDRIFT			5
#define T8_DRIFTST			1
#define T8_TCHAUTOCAL		25
#define T8_SYNC				0
#define T8_ATCHCALST		5
#define T8_ATCHCALSTHR		0
#define T8_ATCHFRCCALTHR	50		/* V2.0 added */
#define T8_ATCHFRCCALRATIO	25		/* V2.0 added */


/* TOUCH_MULTITOUCHSCREEN_T9 INSTANCE 0 */
#define T9_CTRL					139
#define T9_XORIGIN				0
#define T9_YORIGIN				0
#define T9_XSIZE				24
#define T9_YSIZE				14
#define T9_AKSCFG				0
#define T9_BLEN					64
#define T9_TCHTHR				40
#define T9_TCHDI				2
#define T9_ORIENT				2
#define T9_MRGTIMEOUT			10
#define T9_MOVHYSTI				5
#define T9_MOVHYSTN				5
#define T9_MOVFILTER			0
#define T9_NUMTOUCH				2
#define T9_MRGHYST				5
#define T9_MRGTHR				5
#define T9_AMPHYST				20
#define T9_XRANGE				799
#define T9_YRANGE				479
#define T9_XLOCLIP				0
#define T9_XHICLIP				0
#define T9_YLOCLIP				0
#define T9_YHICLIP				0
#define T9_XEDGECTRL			192
#define T9_XEDGEDIST			0
#define T9_YEDGECTRL			192
#define T9_YEDGEDIST			0
#define T9_JUMPLIMIT			30
#define T9_TCHHYST				10	 /* V2.0 or MXT224E added */
#define T9_XPITCH				65	 /* MXT224E added */
#define T9_YPITCH				65	 /* MXT224E added */
#define T9_NEXTTCHDI			1
#define T9_CFG					0


/* PROCI_TOUCHSUPPRESSION_T42 INSTANCE 0 */
#define T42_CTRL					3
#define T42_APPRTHR					32	/* 0 (TCHTHR/4), 1 to 255 */
#define T42_MAXAPPRAREA				40	/* 0 (40ch), 1 to 255 */
#define T42_MAXTCHAREA				35	/* 0 (35ch), 1 to 255 */
#define T42_SUPSTRENGTH				0	/* 0 (128), 1 to 255 */
/* 0 (never expires), 1 to 255 (timeout in cycles) */
#define T42_SUPEXTTO				0
/* 0 to 9 (maximum number of touches minus 1) */
#define T42_MAXNUMTCHS				4
#define T42_SHAPESTRENGTH			0	/* 0 (10), 1 to 31 */
/* add firmware 2.1 */
#define T42_SUPDIST					0
#define T42_DISTHYST				0


/* SPT_CTECONFIG_T46 INSTANCE 0 */
#define T46_CTRL					0  /*Reserved */
/*0: 16X14Y, 1: 17X13Y, 2: 18X12Y, 3: 19X11Y, 4: 20X10Y, 5: 21X15Y, 6: 22X8Y */
#define T46_RESERVED				0
#define T46_IDLESYNCSPERX			16
#define T46_ACTVSYNCSPERX			16
#define T46_ADCSPERSYNC				0
#define T46_PULSESPERADC			0  /*0:1  1:2   2:3   3:4 pulses */
#define T46_XSLEW					1  /*0:500nsec,1:350nsec,2:250nsec(firm2.1)*/
#define T46_SYNCDELAY				0
/* add firmware2.1 */
#define T46_XVOLTAGE				1


/* PROCI_SHIELDLESS_T56 INSTANCE 0 */
#define T56_CTRL				3
#define T56_COMMAND				0
#define T56_OPTINT				1
#define T56_INTTIME				53	
#define T56_INTDELAY_0			24	
#define T56_INTDELAY_1			24
#define T56_INTDELAY_2			24
#define T56_INTDELAY_3			24
#define T56_INTDELAY_4			24
#define T56_INTDELAY_5			24
#define T56_INTDELAY_6			24
#define T56_INTDELAY_7			28
#define T56_INTDELAY_8			28
#define T56_INTDELAY_9			28
#define T56_INTDELAY_10			28
#define T56_INTDELAY_11			28
#define T56_INTDELAY_12			28
#define T56_INTDELAY_13			28
#define T56_INTDELAY_14			28
#define T56_INTDELAY_15			28
#define T56_INTDELAY_16			28
#define T56_INTDELAY_17			28
#define T56_INTDELAY_18			28
#define T56_INTDELAY_19			28
#define T56_INTDELAY_20			28
#define T56_INTDELAY_21			28
#define T56_INTDELAY_22			28
#define T56_INTDELAY_23			24
#define T56_MULTICUTGC			0	//2014.11.22
#define T56_GCLIMIT				0	//2014.11.22
#define T56_NCNCL				1
#define T56_TOUCHBIAS			0
#define T56_BASESCALE			20	//2014.11.22
#define T56_SHIFTLIMIT			4	//2014.11.22
#define T56_YLONOISEMUL     	0
#define T56_YLONOISEDIV			0
#define T56_YHINOISEMUL     	0
#define T56_YHINOISEDIV			0
#define T56_RESERVED			0


/* PROCG_NOISESUPPRESSION_T62 INSTANCE 0 */
#define T62_CTRL 					3
#define T62_CALCFG1					11
#define T62_CALCFG2					0
#define T62_CALCFG3					7
#define T62_CFG1					0
#define T62_RESERVED0				0
#define T62_MINTHRADJ				32
#define T62_BASEFREQ				0
#define T62_MAXSELFREQ				25
#define T62_FREQ0					5
#define T62_FREQ1					10
#define T62_FREQ2					15
#define T62_FREQ3					20
#define T62_FREQ4					24
#define T62_HOPCNT					5
#define T62_RESERVED1				0
#define T62_HOPCNTPER				20
#define T62_HOPEVALTO				5
#define T62_HOPST					5
#define T62_NLGAIN					70
#define T62_MINNLTHR				20
#define T62_INCNLTHR				20
#define T62_ADCPERXTHR				15
#define T62_NLTHRMARGIN				30
#define T62_MAXADCPERX				63
#define T62_ACTVADCSVLDNOD			6
#define T62_IDLEADCSVLDNOD			6
#define T62_MINGCLIMIT				10
#define T62_MAXGCLIMIT				64
#define T62_RESERVED2				0
#define T62_RESERVED3				0
#define T62_RESERVED4				0
#define T62_RESERVED5				0
#define T62_RESERVED6				0
#define T62_BLEN					48
#define T62_TCHTHR					40
#define T62_TCHDI  					2
#define T62_MOVHYSTI				1
#define T62_MOVHYSTN				1
#define T62_MOVFILTER				49
#define T62_NUMTOUCH 				2
#define T62_MRGHYST					10
#define T62_MRGTHR					10
#define T62_XLOCLIP					5
#define T62_XHICLIP					5
#define T62_YLOCLIP					7
#define T62_YHICLIP					7
#define T62_XEDGECTRL				242
#define T62_XEDGEDIST				55
#define T62_YEDGECTRL				168
#define T62_YEDGEDIST				45
#define T62_JUMPLIMIT				30
#define T62_TCHHYST					10
#define T62_NEXTTCHDI				1

#endif
