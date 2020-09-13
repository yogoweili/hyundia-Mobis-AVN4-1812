
/**********************************************************

  DEVICE   : mxT540E  0.5.6
  CUSTOMER : MOBIS
  PROJECT  : N1
  X SIZE   : X18
  Y SIZE   : Y11
  CHRGTIME : 2.59us
  X Pitch  :
  Y Pitch  :
***********************************************************/

#define __MXT540E_CONFIG__

/* SPT_USERDATA_T38 INSTANCE 0 */
#define T38_USERDATA0	0 /* over-write or debug */
#define T38_USERDATA1	9 /* T8_ATCHCALST */
#define T38_USERDATA2	5 /* T8_ATCHCALSTHR */
#define T38_USERDATA3	55 /* T8_ATCHFRCCALTHR */
#define T38_USERDATA4	25 /* T8_ATCHFRCCALRATIO */
#define T38_USERDATA5	'X' /* <- configuration version */
#define T38_USERDATA6	'X'
#define T38_USERDATA7	'.'
#define T38_USERDATA8	'X'
#define T38_USERDATA9	'X'
#define T38_USERDATA10	'X'
#define T38_USERDATA11	'.'
#define T38_USERDATA12	'X'
#define T38_USERDATA13	'X'
#define T38_USERDATA14	'.'
#define T38_USERDATA15	'1'
#define T38_USERDATA16	'2'
#define T38_USERDATA17	'0'
#define T38_USERDATA18	'8'
#define T38_USERDATA19	'2'
#define T38_USERDATA20	'0'
#define T38_USERDATA21	'.'
#define T38_USERDATA22	'0'
#define T38_USERDATA23	'0'
#define T38_USERDATA24	'2' /* configuration version -> */

#define T38_USERDATA25	0 /* reserved : low-4bytes : log level */
#define T38_USERDATA26	1 /* palm check flag(enable/disable, test mode) */
#define T38_USERDATA27	100 /* palm check parameter 1 */
#define T38_USERDATA28	90 /* palm check parameter 2 */
#define T38_USERDATA29	100 /* palm check parameter 3 */
#define T38_USERDATA30  10 /* palm check parameter 4 */
#define T38_USERDATA31	0 /* reserved */
#define T38_USERDATA32	10 /* check_chip_calibration : CAL_THR */
#define T38_USERDATA33	2 /* check_chip_calibration : num_of_antitouch */
#define T38_USERDATA34	81 /* Bit7 ~ Bit2 : time, Bit1 : unit, Bit0 : enable */
#define T38_USERDATA35	
#define T38_USERDATA36	
#define T38_USERDATA37	
#define T38_USERDATA38	
#define T38_USERDATA39	
#define T38_USERDATA40	
#define T38_USERDATA41	
#define T38_USERDATA42	
#define T38_USERDATA43	
#define T38_USERDATA44	
#define T38_USERDATA45
#define T38_USERDATA46	1 /* <-- T48 Noise suppression, enable/disable, count */
#define T38_USERDATA47	0 /* BASEFREQ1 */
#define T38_USERDATA48	0 /* BASEFREQ2 */
#define T38_USERDATA49	0 /* BASEFREQ3 */
#define T38_USERDATA50	0 /* BASEFREQ4 */
#define T38_USERDATA51	0 /* BASEFREQ5 */
#define T38_USERDATA52	0 /* BASEFREQ6 */
#define T38_USERDATA53	0 /* BASEFREQ7 */
#define T38_USERDATA54	0 /* BASEFREQ8 */
#define T38_USERDATA55	0 /* BASEFREQ9 */
#define T38_USERDATA56 	0 /* BASEFREQ10 */
#define T38_USERDATA57	1 /* MFFREQ1 */
#define T38_USERDATA58	2 /* MFFREQ2 */
#define T38_USERDATA59	100 /* GCMAXADCSPERX */
#define T38_USERDATA60	20 /* MFINVLDDIFFTHR */
#define T38_USERDATA61	2 /* MFINCADCSPEXTHR */
#define T38_USERDATA62	46 /* MFERRORTHR */
#define T38_USERDATA63	64 /* --> T48 Noise suppression, BLEN */

/* GEN_POWERCONFIG_T7 INSTANCE 0 */
#define T7_IDLEACQINT	255
#define T7_ACTVACQINT	255
#define T7_ACTV2IDLETO	255
#define T7_CFG				0


/* _GEN_ACQUISITIONCONFIG_T8 INSTANCE 0 */
#define T8_CHRGTIME		28      /* 6 - 60  * 83 ns */
#define T8_RESERVED		0
#define T8_TCHDRIFT		20
#define T8_DRIFTST		20
#define T8_TCHAUTOCAL		0
#define T8_SYNC			0
#define T8_ATCHCALST		5
#define T8_ATCHCALSTHR		0
#define T8_ATCHFRCCALTHR	20		/* V2.0 added */
#define T8_ATCHFRCCALRATIO	1		/* V2.0 added */



/* TOUCH_MULTITOUCHSCREEN_T9 INSTANCE 0 */
#define T9_CTRL                   143
#define T9_XORIGIN                0
#define T9_YORIGIN                0
#define T9_XSIZE                  24
#define T9_YSIZE                  14
#define T9_AKSCFG                 0
#define T9_BLEN                   80
#define T9_TCHTHR			64
#define T9_TCHDI                  0
#define T9_ORIENT                 2
#define T9_MRGTIMEOUT             0
#define T9_MOVHYSTI               3
#define T9_MOVHYSTN               2
#define T9_MOVFILTER              0
#define T9_NUMTOUCH               2
#define T9_MRGHYST                5
#define T9_MRGTHR                 5
#define T9_AMPHYST                0
#define T9_XRANGE                 799
#define T9_YRANGE                 479
#define T9_XLOCLIP                0
#define T9_XHICLIP                0
#define T9_YLOCLIP                0
#define T9_YHICLIP                0
#define T9_XEDGECTRL              0
#define T9_XEDGEDIST              0
#define T9_YEDGECTRL              0
#define T9_YEDGEDIST              0
#define T9_JUMPLIMIT              0
#define T9_TCHHYST                18	 /* V2.0 or MXT224E added */
#define T9_XPITCH                 65	 /* MXT224E added */
#define T9_YPITCH                 68	 /* MXT224E added */
//valky #define T9_NEXTTCHDI              10
#define T9_NEXTTCHDI              1


/* TOUCH_MULTITOUCHSCREEN_T9 INSTANCE 1 */
#if 1	//valky
#define T9_1_CTRL                   0
#define T9_1_XORIGIN                0
#define T9_1_YORIGIN                0
#define T9_1_XSIZE                  16
#define T9_1_YSIZE                  26
#define T9_1_AKSCFG                 0
#define T9_1_BLEN                   192
#define T9_1_TCHTHR		    50
#define T9_1_TCHDI                  2
#define T9_1_ORIENT                 0
#define T9_1_MRGTIMEOUT             0
#define T9_1_MOVHYSTI               3
#define T9_1_MOVHYSTN               1
#define T9_1_MOVFILTER              10
#define T9_1_NUMTOUCH               10
#define T9_1_MRGHYST                10
#define T9_1_MRGTHR                 10
#define T9_1_AMPHYST                0
#define T9_1_XRANGE                 0
#define T9_1_YRANGE                 0
#define T9_1_XLOCLIP                0
#define T9_1_XHICLIP                0
#define T9_1_YLOCLIP                0
#define T9_1_YHICLIP                0
#define T9_1_XEDGECTRL              0
#define T9_1_XEDGEDIST              0
#define T9_1_YEDGECTRL              0
#define T9_1_YEDGEDIST              0
#define T9_1_JUMPLIMIT              40
#define T9_1_TCHHYST                15	 /* V2.0 or MXT224E added */
#define T9_1_XPITCH                 0	 /* MXT224E added */
#define T9_1_YPITCH                 0	 /* MXT224E added */
#define T9_1_NEXTTCHDI              0
#else
#define T9_1_CTRL                   143
#define T9_1_XORIGIN                0
#define T9_1_YORIGIN                0
#define T9_1_XSIZE                  24
#define T9_1_YSIZE                  14
#define T9_1_AKSCFG                 0
#define T9_1_BLEN                   80
#define T9_1_TCHTHR		    		64
#define T9_1_TCHDI                  0
#define T9_1_ORIENT                 2
#define T9_1_MRGTIMEOUT             0
#define T9_1_MOVHYSTI               3
#define T9_1_MOVHYSTN               2
#define T9_1_MOVFILTER              0
#define T9_1_NUMTOUCH               2
#define T9_1_MRGHYST                5
#define T9_1_MRGTHR                 5
#define T9_1_AMPHYST                0
#define T9_1_XRANGE                 799
#define T9_1_YRANGE                 479
#define T9_1_XLOCLIP                0
#define T9_1_XHICLIP                0
#define T9_1_YLOCLIP                0
#define T9_1_YHICLIP                0
#define T9_1_XEDGECTRL              0
#define T9_1_XEDGEDIST              0
#define T9_1_YEDGECTRL              0
#define T9_1_YEDGEDIST              0
#define T9_1_JUMPLIMIT              0
#define T9_1_TCHHYST                18	 /* V2.0 or MXT224E added */
#define T9_1_XPITCH                 65	 /* MXT224E added */
#define T9_1_YPITCH                 68	 /* MXT224E added */
#define T9_1_NEXTTCHDI              1
#endif

/* TOUCH_KEYARRAY_T15 */
/* single key configuration*/  /* 0x03 = multi-key */
#define T15_CTRL                  0
#define T15_XORIGIN               0
#define T15_XORIGIN_4KEY	  0
#define T15_YORIGIN		  0
#define T15_XSIZE		  0
#define T15_XSIZE_4KEY		  0
#define T15_YSIZE                 0
#define T15_AKSCFG                0
#define T15_BLEN                  0
#define T15_TCHTHR                0
#define T15_TCHDI                 0
#define T15_RESERVED_0            0
#define T15_RESERVED_1            0

/* TOUCH_KEYARRAY_T15 */
/* single key configuration*/  /* 0x03 = multi-key */
#define T15_1_CTRL                  0
#define T15_1_XORIGIN               0
#define T15_1_XORIGIN_4KEY	    0
#define T15_1_YORIGIN		    0
#define T15_1_XSIZE		    0
#define T15_1_XSIZE_4KEY	    0
#define T15_1_YSIZE                 0
#define T15_1_AKSCFG                0
#define T15_1_BLEN                  0
#define T15_1_TCHTHR                0
#define T15_1_TCHDI                 0
#define T15_1_RESERVED_0            0
#define T15_1_RESERVED_1            0


/* SPT_COMMSCONFIG_T18 */
#define T18_CTRL                  0
#define T18_COMMAND               0



/* SPT_GPIOPWM_T19 INSTANCE 0 */
#define T19_CTRL                  0
#define T19_REPORTMASK            0
#define T19_DIR                   0
#define T19_INTPULLUP             0
#define T19_OUT                   0
#define T19_WAKE                  0

/* T24_[PROCI_ONETOUCHGESTUREPROCESSOR_T24 INSTANCE 0] */
#define T24_CTRL                  0
#define T24_NUMGEST               0
#define T24_GESTEN                0
#define T24_PROCESS               0
#define T24_TAPTO                 0
#define T24_FLICKTO               0
#define T24_DRAGTO                0
#define T24_SPRESSTO              0
#define T24_LPRESSTO              0
#define T24_REPPRESSTO            0
#define T24_FLICKTHR              0
#define T24_DRAGTHR               0
#define T24_TAPTHR                0
#define T24_THROWTHR              0

/* T24_[PROCI_ONETOUCHGESTUREPROCESSOR_T24 INSTANCE 0] */
#define T24_1_CTRL                  0
#define T24_1_NUMGEST               0
#define T24_1_GESTEN                0
#define T24_1_PROCESS               0
#define T24_1_TAPTO                 0
#define T24_1_FLICKTO               0
#define T24_1_DRAGTO                0
#define T24_1_SPRESSTO              0
#define T24_1_LPRESSTO              0
#define T24_1_REPPRESSTO            0
#define T24_1_FLICKTHR              0
#define T24_1_DRAGTHR               0
#define T24_1_TAPTHR                0
#define T24_1_THROWTHR              0

/* [SPT_SELFTEST_T25 INSTANCE 0] */
#define T25_CTRL                  0
#define T25_CMD                   0
#define T25_SIGLIM_0_UPSIGLIM     0 
#define T25_SIGLIM_0_LOSIGLIM     0 
#define T25_SIGLIM_1_UPSIGLIM     0
#define T25_SIGLIM_1_LOSIGLIM     0
#define T25_SIGLIM_2_UPSIGLIM     0
#define T25_SIGLIM_2_LOSIGLIM     0
#define T25_SIGLIM_3_UPSIGLIM     0
#define T25_SIGLIM_3_LOSIGLIM     0
#define T25_SIGLIM_4_UPSIGLIM     0
#define T25_SIGLIM_4_LOSIGLIM     0
#define T25_SIGLIM_5_UPSIGLIM     0
#define T25_SIGLIM_5_LOSIGLIM     0
#define T25_PINDWELLUS		  0


/* [SPT_SELFTEST_T27 INSTANCE 0] */
#define T27_CTRL                  0
#define T27_NUMGEST               0
#define T27_RESERVED              0
#define T27_GESTEN                0
#define T27_ROTATETHR             0
#define T27_ZOOMTHR               0

#define T27_1_CTRL                  0
#define T27_1_NUMGEST               0
#define T27_1_RESERVED              0
#define T27_1_GESTEN                0
#define T27_1_ROTATETHR             0
#define T27_1_ZOOMTHR               0


/* PROCI_GRIPSUPPRESSION_T40 */

#define T40_CTRL                  0
#define T40_XLOGRIP               0
#define T40_XHIGRIP               0
#define T40_YLOGRIP               0
#define T40_YHIGRIP               0

#define T40_1_CTRL                  0
#define T40_1_XLOGRIP               0
#define T40_1_XHIGRIP               0
#define T40_1_YLOGRIP               0
#define T40_1_YHIGRIP               0

/* PROCI_TOUCHSUPPRESSION_T42 */
#if 0	//valky
#define T42_CTRL                  3
#define T42_APPRTHR               35	/* 0 (TCHTHR/4), 1 to 255 */
#define T42_MAXAPPRAREA           32	/* 0 (40ch), 1 to 255 */
#define T42_MAXTCHAREA            24	/* 0 (35ch), 1 to 255 */
#define T42_SUPSTRENGTH           127	/* 0 (128), 1 to 255 */
/* 0 (never expires), 1 to 255 (timeout in cycles) */
#define T42_SUPEXTTO              0
/* 0 to 9 (maximum number of touches minus 1) */
#define T42_MAXNUMTCHS            5
#define T42_SHAPESTRENGTH         0	/* 0 (10), 1 to 31 */
/* add firmware 2.1 */
#define T42_SUPDIST			0
#define T42_DISTHYST			0
#else
#define T42_CTRL                  3
#define T42_APPRTHR               40	/* 0 (TCHTHR/4), 1 to 255 */
#define T42_MAXAPPRAREA           40	/* 0 (40ch), 1 to 255 */
#define T42_MAXTCHAREA            35	/* 0 (35ch), 1 to 255 */
#define T42_SUPSTRENGTH           0	/* 0 (128), 1 to 255 */
/* 0 (never expires), 1 to 255 (timeout in cycles) */
#define T42_SUPEXTTO              0
/* 0 to 9 (maximum number of touches minus 1) */
#define T42_MAXNUMTCHS            0
#define T42_SHAPESTRENGTH         0	/* 0 (10), 1 to 31 */
/* add firmware 2.1 */
#define T42_SUPDIST			0
#define T42_DISTHYST			0
#endif

#define T42_1_CTRL                  0
#define T42_1_APPRTHR               0   /* 0 (TCHTHR/4), 1 to 255 */
#define T42_1_MAXAPPRAREA           0   /* 0 (40ch), 1 to 255 */
#define T42_1_MAXTCHAREA            0   /* 0 (35ch), 1 to 255 */
#define T42_1_SUPSTRENGTH           0   /* 0 (128), 1 to 255 */
/* 0 (never expires), 1 to 255 (timeout in cycles) */
#define T42_1_SUPEXTTO              0
/* 0 to 9 (maximum number of touches minus 1) */
#define T42_1_MAXNUMTCHS            0
#define T42_1_SHAPESTRENGTH         0   /* 0 (10), 1 to 31 */
/* add firmware 2.1 */
#define T42_1_SUPDIST		    0
#define T42_1_DISTHYST		    0

/* SPT_CTECONFIG_T46  */
#define T46_CTRL                  12  /*Reserved */
/*0: 16X14Y, 1: 17X13Y, 2: 18X12Y, 3: 19X11Y, 4: 20X10Y, 5: 21X15Y, 6: 22X8Y */
#define T46_RESERVED              0
#define T46_IDLESYNCSPERX         16
#define T46_ACTVSYNCSPERX         24
#define T46_ADCSPERSYNC           0
#define T46_PULSESPERADC          0  /*0:1  1:2   2:3   3:4 pulses */
#define T46_XSLEW                 2  /*0:500nsec,1:350nsec,2:250nsec(firm2.1)*/
#define T46_SYNCDELAY		  0
/* add firmware2.1 */
#define T46_XVOLTAGE		  1

/* PROCI_STYLUS_T47 */
#if 1	//valky
#define T47_CTRL                  0
#define T47_CONTMIN               0
#define T47_CONTMAX               0
#define T47_STABILITY             0
#define T47_MAXTCHAREA            0
#define T47_AMPLTHR               0
#define T47_STYSHAPE              0
#define T47_HOVERSUP              0
#define T47_CONFTHR               0
#define T47_SYNCSPERX             0
#define T47_XPOSADJ								0
#define T47_YPOSADJ								0
#define T47_CFG										0
#define T47_RESERVED							0
#else
#define T47_CTRL				9
#define T47_CONTMIN				20
#define T47_CONTMAX				120
#define T47_STABILITY			0
#define T47_MAXTCHAREA			6
#define T47_AMPLTHR				32
#define T47_STYSHAPE			0
#define T47_HOVERSUP			254
#define T47_CONFTHR				0
#define T47_SYNCSPERX			64
#define T47_XPOSADJ				0
#define T47_YPOSADJ				0
#define T47_CFG					3
#define T47_RESERVED			0
#endif


/* PROCI_STYLUS_T47 */
#define T47_1_CTRL                  0
#define T47_1_CONTMIN               0
#define T47_1_CONTMAX               0
#define T47_1_STABILITY             0
#define T47_1_MAXTCHAREA            0
#define T47_1_AMPLTHR               0
#define T47_1_STYSHAPE              0
#define T47_1_HOVERSUP              0
#define T47_1_CONFTHR               0
#define T47_1_SYNCSPERX             0
#define T47_1_XPOSADJ								0
#define T47_1_YPOSADJ								0
#define T47_1_CFG										0
#define T47_1_RESERVED							0

/* PROCI_ADAPTIVETHRESHOLD_T55 */
#define T55_CTRL			0
#define T55_TARGETTHR			0
#define T55_THRADJLIM			0
#define T55_RESETSTEPTIME		0
#define T55_FORCECHGDIST		0
#define T55_FORCECHGTIME		0
#define T55_LOWESTTHR			0

/* PROCI_ADAPTIVETHRESHOLD_T55 */
#define T55_1_CTRL			0
#define T55_1_TARGETTHR			0
#define T55_1_THRADJLIM			0
#define T55_1_RESETSTEPTIME		0
#define T55_1_FORCECHGDIST		0
#define T55_1_FORCECHGTIME		0
#define T55_1_LOWESTTHR			0

/* PROCI_SHIELDLESS_T56 */
#define T56_CTRL			0
#define T56_RESERVED	0
#define T56_OPTINT			0
#define T56_INTTIME			0
#define T56_INTDELAY_INIT_VAL		0
#define T56_MULTICUTGC			0
#define T56_GCLIMIT			0
#define T56_NCNCL			0
#define T56_TOUCHBIAS			0
#define T56_BASESCALE			0
#define T56_SHIFTLIMIT			0
#define T56_YLONOISEMUL     0
#define T56_YLONOISEDIV			0
#define T56_YHINOISEMUL     0
#define T56_YHINOISEDIV			0
#define T56_RESERVERD				0


#if 0	//valky
#define T62_CTRL 						127
#define T62_CALCFG1					1
#define T62_CALCFG2					0
#define T62_CALCFG3					14
#define T62_CFG1						0
#define T62_RESERVED				0
#define T62_MINTHRADJ				0
#define T62_BASEFREQ				0
#define T62_MAXSELFREQ			0
#define T62_FREQ0						0
#define T62_FREQ1						0
#define T62_FREQ2						0
#define T62_FREQ3						0
#define T62_FREQ4						0
#define T62_HOPCNT					5
#define T62_RESERVED				0
#define T62_HOPCNTPER				10
#define T62_HOPEVALTO				5
#define T62_HOPST						5
#define T62_NLGAIN					86
#define T62_MINNLTHR				32
#define T62_INCNLTHR				30
#define T62_ADCPERXTHR			52
#define T62_NLTHRMARGIN			15
#define T62_MAXADCPERX			100
#define T62_ACTVADCSVLDNOD	6
#define T62_IDLEADCSVLDNOD	6
#define T62_MINGCLIMIT			10
#define T62_MAXGCLIMIT			64
#define T62_RESERVED0				0
#define T62_RESERVED1				0
#define T62_RESERVED2				0
#define T62_RESERVED3				0
#define T62_RESERVED4				0
#define T62_BLEN						48
#define T62_TCHTHR					64
#define T62_TCHDI  					0
#define T62_MOVHYSTI						3
#define T62_MOVHYSTN	2
#define T62_MOVFILTER	0
#define T62_NUMTOUCH 	2
#define T62_MRGHYST	5
#define T62_MRGTHR	5
#define T62_XLOCLIP	0
#define T62_XHICLIP	0
#define T62_YLOCLIP	0
#define T62_YHICLIP	0
#define T62_XEDGECTRL	0
#define T62_XEDGEDIST	0
#define T62_YEDGECTRL	0
#define T62_YEDGEDIST	0
#define T62_JUMPLIMIT	0
#define T62_TCHHYST	18
#define T62_NEXTTCHDI	1
#else
#define T62_CTRL 					3
#define T62_CALCFG1					1
#define T62_CALCFG2					0
#define T62_CALCFG3					14
#define T62_CFG1					0
#define T62_RESERVED0				0
#define T62_MINTHRADJ				0
#define T62_BASEFREQ				0
#define T62_MAXSELFREQ				0
#define T62_FREQ0					0
#define T62_FREQ1					0
#define T62_FREQ2					0
#define T62_FREQ3					0
#define T62_FREQ4					0
#define T62_HOPCNT					5
#define T62_RESERVED1				0
#define T62_HOPCNTPER				10
#define T62_HOPEVALTO				5
#define T62_HOPST					5
#define T62_NLGAIN					86
#define T62_MINNLTHR				32
#define T62_INCNLTHR				30
#define T62_ADCPERXTHR				52
#define T62_NLTHRMARGIN				15
#define T62_MAXADCPERX				100
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
#define T62_TCHTHR					64
#define T62_TCHDI  					0
#define T62_MOVHYSTI				3
#define T62_MOVHYSTN				2
#define T62_MOVFILTER				0
#define T62_NUMTOUCH 				2
#define T62_MRGHYST					5
#define T62_MRGTHR					5
#define T62_XLOCLIP					0
#define T62_XHICLIP					0
#define T62_YLOCLIP					0
#define T62_YHICLIP					0
#define T62_XEDGECTRL				0
#define T62_XEDGEDIST				0
#define T62_YEDGECTRL				0
#define T62_YEDGEDIST				0
#define T62_JUMPLIMIT				0
#define T62_TCHHYST					18
#define T62_NEXTTCHDI				1
#endif
