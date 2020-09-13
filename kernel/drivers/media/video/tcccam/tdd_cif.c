/************************************************************************
*    Telechips Multi Media Player
*    ------------------------------------------------
*
*    FUNCTION    : CAMERA INTERFACE API
*    MODEL        : DMP
*    CPU NAME    : TCCXXX
*    SOURCE        : tdd_cif.c
*
*    START DATE    : 2008. 4. 17.
*    MODIFY DATE :
*    DEVISION    : DEPT. SYSTEM 3-2 TEAM
*                : TELECHIPS, INC.
************************************************************************/
#include <linux/delay.h>
#include <asm/system.h>
#include <mach/hardware.h>
#include <asm/io.h>

#if defined(CONFIG_ARCH_TCC893X)
#include <mach/bsp.h>
#endif

#include "sensor_if.h"
#include "cam.h"
#include "cam_reg.h"
#include "tdd_cif.h"
#include <asm/mach-types.h>


void TCC_CIF_CLOCK_Get(void)
{
	CIF_Clock_Get();
}

void TCC_CIF_CLOCK_Put(void)
{
	CIF_Clock_Put();
}

void TDD_CIF_Initialize()
{
#if  defined(CONFIG_ARCH_TCC893X)
	//	In case of CAM4
	volatile PGPION pGPIO_F = (PGPION)tcc_p2v(HwGPIOF_BASE);
	volatile PGPION pGPIO_B = (PGPION)tcc_p2v(HwGPIOB_BASE);
	volatile PGPION pGPIO_C = (PGPION)tcc_p2v(HwGPIOC_BASE);
	volatile PGPION pGPIO_D = (PGPION)tcc_p2v(HwGPIOD_BASE);
	volatile PGPION pGPIO_E = (PGPION)tcc_p2v(HwGPIOE_BASE);
	volatile PGPION pGPIO_G = (PGPION)tcc_p2v(HwGPIOG_BASE);

		#if defined(CONFIG_ARCH_TCC893X)
			if(system_rev == 0x1000) { // TCC8930

				#if defined(CONFIG_DAUDIO)
/**
 * @author sjpark@cleinsoft
 * @date 2013/10/15
 * cif video data port map
 * valky@cleinsoft support Camera & CMMB
 **/
					#if defined(CONFIG_DAUDIO_20140220)
				//CMMB
				BITCSET(pGPIO_E->GPFN0.nREG, 0xFFFFFFFF, 0x44444444);
				BITCSET(pGPIO_E->GPFN1.nREG, 0x00000F00, 0x00000400);

				//Camera
				BITCSET(pGPIO_D->GPFN0.nREG, 0xFFFFFFFF, 0x44444444);
				BITCSET(pGPIO_D->GPFN1.nREG, 0x0000000F, 0x00000004);
					#else // CONFIG_DAUDIO_20130613
				BITCSET(pGPIO_F->GPFN0.nREG, 0xFFFFFFFF, 0x11111111);
				BITCSET(pGPIO_F->GPFN1.nREG, 0xFFFFFFFF, 0x0001B111); // GPIO_F13 -> 5M_CAM_RST , GPIO_F14 -> 1M_CAM_RST , GPIO_F15 -> 1M_CAM_STDBY
				BITCSET(pGPIO_F->GPFN3.nREG, 0x00F0000F, 0x00000000); // GPIO_F24 -> CAM_PWR , GPIO_F29 -> 5M_CAM_STDBY
					#endif
				#else
				BITCSET(pGPIO_F->GPFN0.nREG, 0xFFFFFFFF, 0x11111111);
				BITCSET(pGPIO_F->GPFN1.nREG, 0xFFFFFFFF, 0x0001B111); // GPIO_F13 -> 5M_CAM_RST , GPIO_F14 -> 1M_CAM_RST , GPIO_F15 -> 1M_CAM_STDBY
				BITCSET(pGPIO_F->GPFN3.nREG, 0x00F0000F, 0x00000000); // GPIO_F24 -> CAM_PWR , GPIO_F29 -> 5M_CAM_STDBY
				#endif

			} else if(system_rev == 0x2000 || system_rev == 0x3000) { // TCC8935, TCC8933
				BITCSET(pGPIO_F->GPFN0.nREG, 0xFFFFFFFF, 0x11111111);
				BITCSET(pGPIO_F->GPFN1.nREG, 0xF000FFFF, 0x0001B111); // GPIO_F15 -> 1M_CAM_RST
				BITCSET(pGPIO_D->GPFN0.nREG, 0x0000FFF0, 0x00000000); // GPIO_D1 -> 5M_CAM_RST , GPIO_D2 -> 1M_CAM_STDBY , GPIO_D3 -> CAM_PWR
				BITCSET(pGPIO_G->GPFN0.nREG, 0x000000F0, 0x00000000); // GPIO_G1 -> 5M_CAM_STDBY
			} else if(system_rev == 0x5000 || system_rev == 0x5001 || system_rev == 0x5002) { // M805_TCC8935
				BITCSET(pGPIO_F->GPFN0.nREG, 0xFFFFFFFF, 0x11111111); // GPIO_F00-> PCLK /F01-> HS /F02-> VS /F03 ~ 07->CAM_D0 ~ D4
				BITCSET(pGPIO_F->GPFN1.nREG, 0xF00FFFFF, 0x0001B111); // GPIO_F8 ~ 10->CAM_D5 ~ D7 / F11->MCLK / F12->D_EN / F13~14 ->SDA,SCL/F15->CAM_Reset
				BITCSET(pGPIO_D->GPFN0.nREG, 0x00000FF0, 0x00000000); // GPIO_D01->PWDN0 / D02->PWDN1
			}
		
			// todo : 
		#endif

		// Change to Driving Strength that GPIO_F
		BITCSET(pGPIO_F->GPCD0.nREG, 0x0FFFFFFF, 0x03FFFFFF); // GPIO_F0 ~ 12 max driving strength.(12mA)
#else // CONFIG_ARCH_TCC79X
	unsigned int	hpcsn= 0x03; // PORTCFG13 HPCSN Field. value 3 is SD_D1(1)
	
	HwPORTCFG11 &= ~0x01;
	HwPORTCFG12 = 0x00;
	//HwPORTCFG13 &= ~0x11100000;//HwPORTCFG13 = 0x00;
	HwPORTCFG13 = 0x00000000 | hpcsn;//HwPORTCFG13 = 0x00;	//MCC  //PORTCFG 13  4bit(LSB) is Write Only.
	//HwPORTCFG3 |= 0x100; //pwdn
	//HwPORTCFG10 |= (0x01<<8); //Reset
#endif
}

void TDD_CIF_Termination(void)
{
#if defined(CONFIG_ARCH_TCC893X)
	#if defined(CONFIG_DAUDIO)

	#else
	//	In case of CAM4
	volatile PGPION pGPIO_F = (PGPION)tcc_p2v(HwGPIOF_BASE);

	// Change to Functional GPIO that GPIO_D
	BITCSET(pGPIO_F->GPFN0.nREG, 0xFFFFFFFF, 0x00000000);  
	BITCSET(pGPIO_F->GPFN1.nREG, 0x0000FFFF, 0x00000000);	
	#endif
#else
	volatile PGPION pGPIO_E = (PGPION)tcc_p2v(HwGPIOE_BASE);

	// Change GPIO Setting From Functional Camera To General GPIO
	BITCSET(pGPIO_E->GPFN1, 0xFFFF0000, 0x00000000);
	BITCSET(pGPIO_E->GPFN2, 0xFFFFFFFF, 0x00000000);

	// GPIO Input High Setting!
	BITCSET(pGPIO_E->GPEN, 0x00FFFF00, 0x00000000);

	// Enable Pull Down in Camera 
	BITSET( pGPIO_E->GPPD0, 0xAA000000); // D0~D3+			
	BITSET( pGPIO_E->GPPD1, 0x0000AAAA); // D4~D7, VS, HS
#endif
}

void TDD_CIF_ONOFF(unsigned int uiOnOff)
{
	CIF_ONOFF(uiOnOff);
}

/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setInfo
//
//	DESCRIPTION
//    		set CIF register  : 	HwICPCR1
//
//	Parameters
//    	
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetInfo(unsigned int uiFlag, unsigned int uiBypass, unsigned int uiBypassBusSel,
                        unsigned int uiColorPattern, unsigned int uiPatternFormat, unsigned int uiRGBMode,
                        unsigned int uiRGBBitMode, unsigned int uiColorSequence, unsigned int uiBusOrder)
{
#if  defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else  // (CONFIG_ARCH_TCC79X)
	// 1.    HwICPCR1_BP        Hw15         // Bypass 
	if(uiFlag & SET_CIF_BYPASS_MODE)
	{
		BITCSET(HwICPCR1, HwICPCR1_BP, (uiBypass << 15));
	}
	
	// 2.   HwICPCR1_BBS_LSB8      Hw14         // When bypass 16bits mode, LSB 8bits are stored in first
	if(uiFlag & SET_CIF_BYPASS_BUS)
	{
		BITCSET(HwICPCR1, HwICPCR1_BBS_LSB8, (uiBypassBusSel << 14));
	}
	
	// 3.   HwICPCR1_CP_RGB        Hw12         // RGB(555,565,bayer) color pattern
	if(uiFlag & SET_CIF_COLOR_PATTERN)
	{
		BITCSET(HwICPCR1, HwICPCR1_CP_RGB, (uiColorPattern << 12));
	}
	
	// 4.
	// HwICPCR1_PF_444        HwZERO       // 4:4:4 format
	// HwICPCR1_PF_422        Hw10         // 4:2:2 format
	// HwICPCR1_PF_420        Hw11         // 4:2:0 format or RGB(555,565,bayer) mode
	if(uiFlag & SET_CIF_PATTERN_FORMAT)
	{
		BITCSET(HwICPCR1, (HwICPCR1_PF_420|HwICPCR1_PF_422), (uiPatternFormat << 10));
	}
	
	// 5.
	// HwICPCR1_RGBM_BAYER      HwZERO       // Bayer RGB Mode
	// HwICPCR1_RGBM_RGB555            Hw8          // RGB555 Mode
	// HwICPCR1_RGBM_RGB565            Hw9          // RGB565 Mode
	if(uiFlag & SET_CIF_RGB_MODE)
	{
		BITCSET(HwICPCR1, (HwICPCR1_RGBM_RGB555|HwICPCR1_RGBM_RGB565), (uiRGBMode << 8));
	}
	
	// 6.
	// HwICPCR1_RGBBM_16      HwZERO       // 16bit mode
	// HwICPCR1_RGBBM_8DISYNC                  Hw6          // 8bit disable sync
	// HwICPCR1_RGBBM_8      Hw7          // 8bit mode
	if(uiFlag & SET_CIF_RGBBIT_MODE)
	{
		BITCSET(HwICPCR1, (HwICPCR1_RGBBM_8DISYNC|HwICPCR1_RGBBM_8), (uiRGBBitMode << 6));
	}
	
	// 7.  CS
	// HwICPCR1_CS_RGBMG      HwZERO       // 555RGB:RGB(MG), 565RGB:RGB, 444/422/420:R/Cb/U first, Bayer RGB:BG->GR, CCIR656:YCbYCr
	// HwICPCR1_CS_RGBLG      Hw4          // 555RGB:RGB(LG), 565RGB:RGB, 444/422/420:R/Cb/U first, Bayer RGB:GR->BG, CCIR656:YCrYCb
	// HwICPCR1_CS_BGRMG      Hw5          // 555RGB:BGR(MG), 565RGB:BGR, 444/422/420:B/Cr/V first, Bayer RGB:RG->GB, CCIR656:CbYCrY
	// HwICPCR1_CS_BGRLG      (Hw5|Hw4)    // 555RGB:BGR(LG), 565RGB:BGR, 444/422/420:B/Cr/V first, Bayer RGB:GB->RG, CCIR656:CrYCbY
	if(uiFlag & SET_CIF_COLOR_SEQUENCE)
	{
		BITCSET(HwICPCR1, HwICPCR1_CS_BGRLG, (uiColorSequence << 4));
	}
	
	// 8.   HwICPCR1_BO_SW        Hw2          // Switch the MSB/LSB 8bit Bus
	if(uiFlag & SET_CIF_BUS_ORDER)
	{
		BITCSET(HwICPCR1, HwICPCR1_BO_SW, (uiBusOrder << 2));
	}

#endif
}

/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setCtrl
//
//	DESCRIPTION
//    		set CIF register  : 	HwICPCR1
//
//	Parameters
//    	
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetCtrl(unsigned int uiFlag, unsigned int uiPWDN, unsigned int uiBypass_Scaler,
						unsigned int uiPXCLK_POL, unsigned int uiSKPF, unsigned int uiM420_FC, unsigned int uiC656,
						bool bEnableOfATV)
{
#if  defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else //(CONFIG_ARCH_TCC79X)	
	// 1.  HwICPCR1_PWD      Power down mode in camera >> 0:Disable, 1:Power down mode , This power down mode is connected the PWDN of camera sensor  
	if(uiFlag & SET_CIF_PWDN)
	{
		BITCSET(HwICPCR1, HwICPCR1_PWD, (uiPWDN << 30));
	}
	
	// 2.  HwICPCR1_BPS                            Hw23         // Bypass Scaler >> 0:Non, 1:Bypass    //BP_SCA
	if(uiFlag & SET_CIF_BYPASS_SCALER)
	{
		BITCSET(HwICPCR1, HwICPCR1_BPS, (uiBypass_Scaler << 23));
	}
	
	// 3.  HwICPCR1_POL                            Hw21         // PXCLK Polarity >> 0:Positive edge, 1:Negative edge
	if(uiFlag & SET_CIF_PXCLK_POL)
	{
		BITCSET(HwICPCR1, HwICPCR1_POL, (uiPXCLK_POL << 21));
	}
	
	// 4.  HwICPCR1_SKPF                                        // Skip Frame >> 0~7 #Frames skips   [20:18]
	if(uiFlag & SET_CIF_SKPF)
	{
		BITCSET(HwICPCR1, HwICPCR1_SKPF, (uiSKPF << 18));
	}
	
	// 5.
	//  HwICPCR1_M420_ZERO                      HwZERO        // Format Convert (YUV422->YUV420) , Not-Convert
	//  HwICPCR1_M420_ODD                       Hw17             // converted in odd line skip   // 10
	//  HwICPCR1_M420_EVEN                      (Hw17|Hw16)  // converted in even line skip  //11
	if(uiFlag & SET_CIF_M42_FC)
	{
	    BITCSET(HwICPCR1, HwICPCR1_M420_EVEN_SKIP, (uiM420_FC << 16));
	}
	
	// 7.  #define HwICPCR1_C656                           Hw13         // Convert 656 format 0:Disable, 1:Enable  
	if(uiFlag & SET_CIF_C656)
	{
			BITCSET(HwICPCR1, HwICPCR1_C656, (uiC656 << 13)); // PCIF->ICPCR1
	}
#endif
}	

/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setTransfer
//
//	DESCRIPTION
//			CIF transef mode setting
//    		set CIF register  : 	HwCDCR1
//
//	Parameters
//    	
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetTransfer(unsigned int uiFlag, unsigned int uiBurst, unsigned int uiLock, unsigned int uiTransMode)
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.
#else // (CONFIG_ARCH_TCC79X)
	//HwCDCR1_TM_INC        Hw3                    // INC Transfer
	if(uiFlag & SET_CIF_TRANSFER_MODE)
	{
		BITCSET(HwCDCR1, HwCDCR1_TM_INC, (uiTransMode << 3));
	}
	
	// HwCDCR1_LOCK_ON        Hw2                    // Lock Transfer
	if(uiFlag & SET_CIF_TRANSFER_LOCK)
	{
		BITCSET(HwCDCR1, HwCDCR1_LOCK_ON, (uiLock << 2));
	}
	
	// HwCDCR1_BS_1    HwZERO      // The DMA transfers the image data as 1 word to memory
	// HwCDCR1_BS_2    Hw0            // The DMA transfers the image data as 2 word to memory
	// HwCDCR1_BS_4    Hw1            // The DMA transfers the image data as 4 word to memory
	// HwCDCR1_BS_8    (Hw1|Hw0)  // The DMA transfers the image data as 8 word to memory (default)
	if(uiFlag & SET_CIF_TRANSFER_BURST)
	{
		BITCSET(HwCDCR1, HwCDCR1_BS_8, uiBurst); // HwCIF->CDCR1
	}
#endif
}

/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_convertR2Y
//
//	DESCRIPTION
//    		set CIF register  : 	HwCR2Y
//
//	Parameters
//    	
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_R2Y_Convert(unsigned int uiFlag,  unsigned int uiFMT, unsigned int uiEN)
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else //(CONFIG_ARCH_TCC79X)
	//HwCR2Y_FMT                                  (Hw4|Hw3|Hw2|Hw1)  // FMT[4:1]  0000 -> Input format 16bit 565RGB(RGB sequence) ?ڼ??? ???? 750A CIF SPEC. 1-22
	if(uiFlag & SET_CIF_CR2Y_FMT)
	{
	    BITCSET(HwCR2Y, HwCR2Y_555GAR_BGR8, (uiFMT<<1));
	}
	
	//HwCR2Y_EN                                    Hw0          // R2Y Enable,   0:disable, 1:Enable
	if(uiFlag & SET_CIF_CR2Y_EN)
	{
			BITCSET(HwCR2Y, HwCR2Y_EN, (uiEN)); // HwCIF->CR2Y
	}

#endif
}

/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_overlayCtrl
//
//	DESCRIPTION
//    		set CIF register  : 	HwOCTRL1, HwOCTRL2
//
//	Parameters
//    	
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_OverlayCtrl(unsigned int uiFlag, unsigned int uiRgb)
{
#if  defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else //(CONFIG_ARCH_TCC79X)	
	if(uiFlag & SET_CIF_ALPHA_ENABLE)
	{
		BITSET(HwOCTRL1, HwOCTRL1_AEN_EN);
	}
	
	if(uiFlag & SET_CIF_ALPHA_DISABLE)
	{
		BITCLR(HwOCTRL1, HwOCTRL1_AEN_EN);
	} 
	if(uiFlag & SET_CIF_CHROMA_ENABLE)
	{
		BITSET(HwOCTRL1, HwOCTRL1_CEN_EN);
	}
	
	if(uiFlag & SET_CIF_CHROMA_DISABLE)
	{
		BITCLR(HwOCTRL1, HwOCTRL1_CEN_EN);
	}
	
	if(uiFlag & SET_CIF_OVERLAY_ENABLE)
	{    
		BITSET(HwOCTRL1, HwOCTRL1_OE_EN);
	}
	
	if(uiFlag & SET_CIF_OVERLAY_DISABLE)
	{    
		BITCLR(HwOCTRL1, HwOCTRL1_OE_EN);
	}
	
	if(uiFlag & SET_CIF_COLOR_CONV_ENABLE)
	{
		BITSET(HwOCTRL2, HwOCTRL2_CONV);
	}
	
	if(uiFlag & SET_CIF_COLOR_CONV_DISABLE)
	{
		BITCLR(HwOCTRL2, HwOCTRL2_CONV);
	}
	
	if(uiFlag & SET_CIF_COLOR_MODE_RGB)
	{
		BITSET(HwOCTRL2, HwOCTRL2_MD);
		BITCSET(HwOCTRL2, 0x00000006, (uiRgb << 1));        
	}
	
	if(uiFlag & SET_CIF_COLOR_MODE_YUV)
	{
		BITCLR(HwOCTRL2, HwOCTRL2_MD);
	}

#endif
}

/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_overlaySet
//
//	DESCRIPTION
//    		set CIF register  : 	HwOCTRL1
//
//	Parameters
//    	
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetOverlay(unsigned int uiFlag, unsigned int uiOverlayCNT, unsigned int uiOverlayMethod, 
							 unsigned int uiXOR1, unsigned int uiXOR0, unsigned int uiAlpha1, unsigned int uiAlpha0)
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else //(CONFIG_ARCH_TCC79X)
	if(uiFlag & SET_CIF_OVERLAY_COUNT)
	{
		BITCSET(HwOCTRL1, HwOCTRL1_OCNT_MAX, (uiOverlayCNT << 24));
	}

	if(uiFlag & SET_CIF_OVERLAY_METHOD)
	{
		BITCSET(HwOCTRL1, HwOCTRL1_OM_BLOCK, (uiOverlayMethod << 16));
	} 

	if(uiFlag & SET_CIF_OVERLAY_XOR0)
	{
		BITCSET(HwOCTRL1, HwOCTRL1_XR0_100, (uiXOR0 << 9));
	} 

	if(uiFlag & SET_CIF_OVERLAY_XOR1)
	{
		BITCSET(HwOCTRL1, HwOCTRL1_XR1_100, (uiXOR1 << 10));
	} 

	if(uiFlag & SET_CIF_OVERLAY_ALPHA0)
	{
		BITCSET(HwOCTRL1, HwOCTRL1_AP0_100, (uiAlpha0 << 4));
	}

	if(uiFlag & SET_CIF_OVERLAY_ALPHA1)
	{
		BITCSET(HwOCTRL1, HwOCTRL1_AP1_100, (uiAlpha1 << 6));
	}
#endif
}	

/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_overlaySetKey
//
//	DESCRIPTION
//    		set CIF register  : 	HwOCTRL3, HwOCTRL4
//
//	Parameters
//    	
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetOverlayKey(unsigned int uiFlag, unsigned int uiKEYR, unsigned int uiKEYG, unsigned int uiKEYB, 
								 unsigned int uiMKEYR, unsigned int uiMKEYG, unsigned int uiMKEYB)
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else //(CONFIG_ARCH_TCC79X)
	if(uiFlag & SET_CIF_OVERLAY_KEY)
	{
		BITCSET(HwOCTRL3, (HwOCTRL3_KEYR_MAX|HwOCTRL3_KEYG_MAX|HwOCTRL3_KEYB_MAX), ((uiKEYR << 16)|(uiKEYG << 8)|uiKEYB));
	} // HwCIF->OCTRL3

	if(uiFlag & SET_CIF_OVERLAY_MASKKEY)
	{
		BITCSET(HwOCTRL4, (HwOCTRL4_MKEYR_MAX|HwOCTRL4_MKEYG_MAX|HwOCTRL4_MKEYB_MAX), ((uiMKEYR << 16)|(uiMKEYG << 8)|uiMKEYB));
	} // HwCIF->OCTRL4
#endif
}

/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setSyncPol
//
//	DESCRIPTION
//    		set CIF register  : 	H and V sync polarity
//
//	Parameters
//    	
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetSyncPol(unsigned int uiHPolarity, unsigned int uiVpolarity)
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else
	BITCSET(HwICPCR1, HwICPCR1_HSP_HIGH, (uiHPolarity << 1));
	BITCSET(HwICPCR1, HwICPCR1_VSP_HIGH, uiVpolarity); // HwCIF->ICPCR1
#endif
}


/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setSyncPol
//
//	DESCRIPTION
//    		set CIF register  : 	HwIIS, HwIIW1, HwIIW2, 
// 							HwOIS, HwOIW1, HwOIW2
//
//	Parameters
//                CIF_SETIMG (Camera I/F setting image)
//                uiType                 : Input imge(INPUT_IMG), overlay image (OVER_IMG)
//                uiHsize                : Horizontal size
//                uiVSize                : vertical size
//                uiHorWindowingStart    : start X position of windowing image
//                uiHorWindowingEnd    : end X position of windowing image
//                uiVerWindowingStart    : start Y position of windowing image
//                uiVerWindowingEnd    : end Y position of windowing image
//               BaseAddress0            : Y channel base address In Overlay, overlay image adress
//               BaseAddress1            : U channel base address (don't use overlay image)
//                BaseAddress2            : V channel base address (don't use overlay image)
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void    TDD_CIF_SetImage(unsigned int uiType, unsigned int uiHsize, unsigned int uiVsize,
                              unsigned int uiHorWindowingStart, unsigned int uiHorWindowingEnd,
                              unsigned int uiVerWindowingStart, unsigned int uiVerWindowingEnd)
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else //(CONFIG_ARCH_TCC79X)
	if (uiType & INPUT_IMG)
	{
		BITCSET(HwIIS, 0xFFFFFFFF, ((uiHsize << 16) | uiVsize));
		BITCSET(HwIIW1, 0xFFFFFFFF, ((uiHorWindowingStart<< 16) | uiHorWindowingEnd));
		BITCSET(HwIIW2, 0xFFFFFFFF, ((uiVerWindowingStart<< 16) | uiVerWindowingEnd));
	}

	if(uiType & OVERLAY_IMG)
	{
		BITCSET(HwOIS, 0xFFFFFFFF, ((uiHsize << 16) | uiVsize));
		BITCSET(HwOIW1, 0xFFFFFFFF, ((uiHorWindowingStart<< 16) | uiHorWindowingEnd));
		BITCSET(HwOIW2, 0xFFFFFFFF, ((uiVerWindowingStart<< 16) | uiVerWindowingEnd));
	}
#endif	
}


/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setSensorOutImgSize
//
//	DESCRIPTION
//    		set CIF register  : 	HwCEIS
//
//	Parameters
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetSensorOutImgSize(unsigned int uiHsize, unsigned int uiVsize)
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else //(CONFIG_ARCH_TCC79X)
	BITCSET(HwCEIS, HwCEIS_HSIZE, (uiHsize<<16));
	BITCSET(HwCEIS, HwCEIS_VSIZE, (uiVsize));    // HwCEM->CEIS

#endif
}	


/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setBaseAddr
//
//	DESCRIPTION
//    		set CIF register  : 	HwCDCR2, HwCDCR3, HwCDCR4
//									HwCOBA
//									HwCDCR5, 
//									HwCDCR6, HwCDCR7, HwCDCR8
//									HwCESA
//	Parameters
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetBaseAddr (unsigned int uiType, unsigned int uiBaseAddr0, 
                                                   unsigned int uiBaseAddr1, unsigned int uiBaseAddr2)
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else //(CONFIG_ARCH_TCC79X)

	if (uiType & INPUT_IMG)
	{
		BITCSET(HwCDCR2, 0xFFFFFFFF, uiBaseAddr0);
		BITCSET(HwCDCR3, 0xFFFFFFFF, uiBaseAddr1);
		BITCSET(HwCDCR4, 0xFFFFFFFF, uiBaseAddr2);
	}

	if(uiType & OVERLAY_IMG)
	{
		BITCSET(HwCOBA, 0xFFFFFFFF, uiBaseAddr0);
	}

	if (uiType & IN_IMG_ROLLING)
	{
		BITCSET(HwCDCR5, 0xFFFFFFFF, uiBaseAddr0);
		BITCSET(HwCDCR6, 0xFFFFFFFF, uiBaseAddr1);
		BITCSET(HwCDCR7, 0xFFFFFFFF, uiBaseAddr2);
	}

	if(uiType & IN_ENC_START_ADDR)
	{
		BITCSET(HwCESA, 0xFFFFFFFF, uiBaseAddr0);
	}
#endif	
}	


void TDD_CIF_SetBaseAddr_offset(unsigned int uiType, unsigned int uiOFFSET_Y, unsigned int uiOFFSET_C)
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#endif
}

/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setInterrupt
//
//	DESCRIPTION
//    		set CIF register  : 	HwIEN, HwSEL, HwCIRQ
//							HwIEN, HwIRQSEL, HwCIRQ
//							HwCESA
//
//	Parameters
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetInterrupt(unsigned int uiFlag)
{
	// Interrupt Enable  0:interrupt disable, 1:interrupt enable    //Hw31
#if  defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else //(CONFIG_ARCH_TCC79X)
	if(uiFlag & SET_CIF_INT_ENABLE)
	{
		BITSET(HwIEN, HwINT_CAM);    
		BITSET(HwSEL, HwINT_CAM);
		BITSET(HwCIRQ, HwCIRQ_IEN_EN);    // Camera Interrupt enable
	}  
	if(uiFlag & SET_CIF_INT_DISABLE)
	{
		BITCLR(HwIEN, HwINT_CAM);    
		BITCLR(HwSEL, HwINT_CAM);
		BITCLR(HwCIRQ, HwCIRQ_IEN_EN);    // Camera Interrupt enable
	}  
#endif
	

#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else //(CONFIG_ARCH_TCC79X)
	// Update Register in VSYNC   0:Register is update without VSYNC , 1:When VSYNC is posedge, register is updated.  //Hw30
	if(uiFlag & SET_CIF_UPDATE_IN_VSYNC)
	{
		BITSET(HwCIRQ, HwCIRQ_URV);
	}
	
	if(uiFlag & SET_CIF_UPDATE_WITHOUT_VSYNC)
	{
		BITCLR(HwCIRQ, HwCIRQ_URV);
	}
	
	// Interrupt Type  0:Pulse type, 1:Hold-up type when respond signal(ICR) is high  //Hw29
	if(uiFlag & SET_CIF_INT_TYPE_HOLDUP)
	{
		BITSET(HwCIRQ, HwCIRQ_ITY);
	}
	
	if(uiFlag & SET_CIF_INT_TYPE_PULSE)
	{
		BITCLR(HwCIRQ, HwCIRQ_ITY);
	}
	
	// Interrupt Clear 0:.... , 1:Interrupt Clear (using ITY is Hold-up type)  //Hw28
	if(uiFlag & SET_CIF_INT_HOLD_CLEAR)
	{
		BITSET(HwCIRQ, HwCIRQ_ICR);
	}
#endif
}	


/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setMaskIntStatus
//
//	DESCRIPTION
// 			CIF interrupt status masking
//    		set CIF register  : 	HwCIRQ
//
//	Parameters
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetMaskIntStatus(unsigned int uiFlag)
{

#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else //(CONFIG_ARCH_TCC79X)
	// Hw26           // Mask interrupt of VS negative edge,  0:Don't mask, 1:Mask
	if(uiFlag & SET_CIF_INT_VS_NEGA_MASK)
	{
		BITSET(HwCIRQ, HwCIRQ_MVN);
	}
	
	if(uiFlag & SET_CIF_INT_VS_NEGA_NOT_MASK)
	{
		BITCLR(HwCIRQ, HwCIRQ_MVN);
	}
	
	// Hw25           // Mask interrupt of VS negative edge,  0:Don't mask, 1:Mask
	if(uiFlag & SET_CIF_INT_VS_POSI_MASK)
	{
		BITSET(HwCIRQ, HwCIRQ_MVP);
	}
	
	if(uiFlag & SET_CIF_INT_VS_POSI_NOT_MASK)
	{
		BITCLR(HwCIRQ, HwCIRQ_MVP);
	}
	
	//        Hw24           // Mask interrupt of VCNT Interrupt,  0:Don't mask, 1:Mask
	if(uiFlag & SET_CIF_INT_VCNT_MASK)
	{
		BITSET(HwCIRQ, HwCIRQ_MVIT);
	}
	
	if(uiFlag & SET_CIF_INT_VCNT_NOT_MASK)
	{
		BITCLR(HwCIRQ, HwCIRQ_MVIT);
	}
	
	//          Hw23           // Mask interrupt of Scaler Error,  0:Don't mask, 1:Mask
	if(uiFlag & SET_CIF_INT_SCALER_ERR_MASK)
	{
		BITSET(HwCIRQ, HwCIRQ_MSE);
	}
	
	if(uiFlag & SET_CIF_INT_SCALER_ERR_NOT_MASK)
	{
		BITCLR(HwCIRQ, HwCIRQ_MSE);
	}
	
	//          Hw22           // Mask interrupt of Scaler finish,  0:Don't mask, 1:Mask
	if(uiFlag & SET_CIF_INT_SCALER_FINISH_MASK)
	{
		BITSET(HwCIRQ, HwCIRQ_MSF);
	}
	
	if(uiFlag & SET_CIF_INT_SCALER_FINISH_NOT_MASK)
	{
		BITCLR(HwCIRQ, HwCIRQ_MSF);
	}
	
	//          Hw21           // Mask interrupt of Encoding start,  0:Don't mask, 1:Mask
	if(uiFlag & SET_CIF_INT_ENC_STRT_MASK)
	{
		BITSET(HwCIRQ, HwCIRQ_MENS);
	}
	
	if(uiFlag & SET_CIF_INT_ENC_STRT_NOT_MASK)
	{
		BITCLR(HwCIRQ, HwCIRQ_MENS);
	}
	
	//          Hw20           // Mask interrupt of Rolling V address,  0:Don't mask, 1:Mask
	if(uiFlag & SET_CIF_INT_ROLL_VADDR_MASK)
	{
		BITSET(HwCIRQ, HwCIRQ_MRLV);
	}
	
	if(uiFlag & SET_CIF_INT_ROLL_VADDR_NOT_MASK)
	{
		BITCLR(HwCIRQ, HwCIRQ_MRLV);
	}
	
	//          Hw19           // Mask interrupt of Rolling U address,  0:Don't mask, 1:Mask
	if(uiFlag & SET_CIF_INT_ROLL_UADDR_MASK)
	{
		BITSET(HwCIRQ, HwCIRQ_MRLU);
	}
	
	if(uiFlag & SET_CIF_INT_ROLL_UADDR_NOT_MASK)
	{
		BITCLR(HwCIRQ, HwCIRQ_MRLU);
	}
	
	//          Hw18           // Mask interrupt of Rolling Y address,  0:Don't mask, 1:Mask
	if(uiFlag & SET_CIF_INT_ROLL_YADDR_MASK)
	{
		BITSET(HwCIRQ, HwCIRQ_MRLY);
	}
	
	if(uiFlag & SET_CIF_INT_ROLL_YADDR_NOT_MASK)
	{
		BITCLR(HwCIRQ, HwCIRQ_MRLY);
	}
	
	//          Hw17           // Mask interrupt of Capture frame,  0:Don't mask, 1:Mask
	if(uiFlag & SET_CIF_INT_CAPTURE_FRM_MASK)
	{
		BITSET(HwCIRQ, HwCIRQ_MSCF);
	}
	
	if(uiFlag & SET_CIF_INT_CAPTURE_FRM_NOT_MASK)
	{
		BITCLR(HwCIRQ, HwCIRQ_MSCF);
	}
	
	//          Hw16           // Mask interrupt of Stored one frame,  0:Don't mask, 1:Mask
	if(uiFlag & SET_CIF_INT_STORE_1FRM_MASK)
	{
		BITSET(HwCIRQ, HwCIRQ_MSOF);
	}
	
	if(uiFlag & SET_CIF_INT_STORE_1FRM_NOT_MASK)
	{
		BITCLR(HwCIRQ, HwCIRQ_MSOF);
	}

	if(uiFlag & SET_CIF_INT_ALL_MASK)
	{
		//BITCSET(HwCIRQ, 0x07FF0000, 0x7FF<<16);
		BITSET(HwCIRQ, 0x7FF<<16);
	}
	
	if(uiFlag & SET_CIF_INT_ALL_CLEAR_MASK)
	{
		BITCLR(HwCIRQ, 0x7FF<<16);
	} // HwCIF->CIRQ

#endif
}

/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_getIntStatus
//
//	DESCRIPTION
//			get CIF status
//    		get CIF register  : 	HwCIRQ
//
//	Parameters
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
unsigned int TDD_CIF_GetIntStatus(unsigned int uiFlag)
{
    unsigned long uiRegStatus = 0;

#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.
#endif
	
    return (unsigned int)uiRegStatus;
}


/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_set656FormatConfig
//
//	DESCRIPTION
//    		set CIF register  : 	Hw656FCR1, Hw656FCR2
//
//	Parameters
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_Set656FormatConfig(unsigned int uiType, unsigned int uiPsl, unsigned int uiFpv,
										unsigned int uiSpv, unsigned int uiTpv, 
										unsigned int uiFb, unsigned int uiHb, unsigned int uiVb)
{
#if  defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#endif
}

/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setFIFOStatusClear
//
//	DESCRIPTION
//			clear FIFO Status
//    		set CIF register  : 	HwFIFOSTATE
//
//	Parameters
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetFIFOStatusClear(void)
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else //(CONFIG_ARCH_TCC92XX)
	BITSET(HwFIFOSTATE, HwFIFOSTATE_CLR); // HwCIF->FIFOSTATE
#endif
}


/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setFIFOStatusClear
//
//	DESCRIPTION
//    		get CIF register  : 	HwFIFOSTATE
//
//	Parameters
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
unsigned int TDD_CIF_GetFIFOStatus(unsigned int uiType)
{
    unsigned long uiRet = 0;

#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#endif
    return (unsigned int)uiRet;
}


/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setCaptureCtrl
//
//	DESCRIPTION
//			CIF Capture Control
//    		set CIF register  : 	HwCCM1, HwCCM2
//
//	Parameters
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetCaptureCtrl(unsigned int uiType,  unsigned int uiSkipNum, unsigned int uiVCnt, unsigned int uiFlag)
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#endif
}

/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_getCaptureStatus
//
//	DESCRIPTION
//    		get CIF register  : 	HwCCM1
//
//	Parameters
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
unsigned int TDD_CIF_GetCaptureStatus(unsigned int uiType)
{
  unsigned long uiCaptureStatus = 0;

#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#endif
  return (unsigned int)uiCaptureStatus;	
}


/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setEffectMode
//
//	DESCRIPTION
//			CIF effect setting
//    		set CIF register  : 	HwCEM
//
//	Parameters
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetEffectMode(unsigned int uiFlag)
{
	unsigned int  uiValue = 1;
	
#if  defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#endif
}

/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setEffectSepiaUV
//
//	DESCRIPTION
//			CIF effect setting
//    		set CIF register  : 	HwCSUV
//
//	Parameters
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetEffectSepiaUV(unsigned int uiSepiaU, unsigned int uiSepiaV)
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else //(CONFIG_ARCH_TCC92XX)
	BITCSET(HwCSUV, HwHwCSUV_SEPIA_U, (uiSepiaU<<8));
	BITCSET(HwCSUV, HwHwCSUV_SEPIA_V, (uiSepiaV));    // HwCEM->CSUV
#endif
}


/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setEffectColorSelect
//
//	DESCRIPTION
//			CIF effect setting
//    		set CIF register  : 	HwCCS
//
//	Parameters
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetEffectColorSelect(unsigned int uiUStart, unsigned int uiUEnd, 
										unsigned int uiVStart, unsigned int uiVEnd)
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else
	BITCSET(HwCCS, HwCCS_USTART, (uiUStart<<24));
	BITCSET(HwCCS, HwCCS_UEND, (uiUEnd<<16));
	BITCSET(HwCCS, HwCCS_VSTART, (uiVStart<<8));
	BITCSET(HwCCS, HwCCS_VEND, (uiVEnd)); // HwCEM->CCS
#endif
}


/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setEffectHFilterCoeff
//
//	DESCRIPTION
//			CIF effect setting
//    		set CIF register  : 	HwCHFC_COEF
//
//	Parameters
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetEffectHFilterCoeff(unsigned int uiCoeff0, unsigned int uiCoeff1, unsigned int uiCoeff2)
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else
	BITCSET(HwCHFC, HwCHFC_COEF0, (uiCoeff0<<16));
	BITCSET(HwCHFC, HwCHFC_COEF1, (uiCoeff1<<8));
	BITCSET(HwCHFC, HwCHFC_COEF2, (uiCoeff2));
#endif
}


/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setEffectSketchTh
//
//	DESCRIPTION
//			CIF effect setting
//    		set CIF register  : 	HwCST
//
//	Parameters
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetEffectSketchTh(unsigned int uithreshold)
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else
	BITCSET(HwCST, HwCST_THRESHOLD, (uithreshold)); // HwCEM->CST
#endif
}


/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setEffectClampTh
//
//	DESCRIPTION
//			CIF effect setting
//    		set CIF register  : 	HwCCT
//
//	Parameters
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetEffectClampTh(unsigned int uithreshold)
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else
	BITCSET(HwCCT, HwCCT_THRESHOLD, (uithreshold));         //HwCEM->CCT
#endif
}


/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setEffectBias
//
//	DESCRIPTION
//			CIF effect setting
//    		set CIF register  : 	HwCBR
//
//	Parameters
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetEffectBias(unsigned int uiYBias, unsigned int uiUBias, unsigned int uiVBias)
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else
	BITCSET(HwCBR, HwCBR_YBIAS, (uiYBias<<16));
	BITCSET(HwCBR, HwCBR_UBIAS, (uiUBias<<8));
	BITCSET(HwCBR, HwCBR_VBIAS, (uiVBias));        //PEFFECT->CBR
#endif
}


/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setInpathCtrl
//
//	DESCRIPTION
//    		get CIF register  : 	HwCIC
//
//	Parameters
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetInpathCtrl(unsigned int uiType, unsigned int uiHWait,
								unsigned int uiStrobeCycle, unsigned int uiInpathWait)
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else
	if(uiType & SET_CIF_INPATH_H_WAIT)
	{
		BITCSET(HwCIC, HwCIC_H2H_WAIT, (uiHWait << 16));
	}
	
	if(uiType & SET_CIF_INPATH_S_CYCLE)
	{
		BITCSET(HwCIC, HwCIC_STB_CYCLE, (uiStrobeCycle << 8));
	}
	
	if(uiType & SET_CIF_INPATH_I_WAIT)
	{
		BITCSET(HwCIC, HwCIC_INP_WAIT, (uiInpathWait << 4));
	}
	
	if(uiType & SET_CIF_INPATH_R_ENABLE)
	{
		BITSET(HwCIC, HwCIC_INPR);
	}
	
	if(uiType & SET_CIF_INPATH_R_DISABLE)
	{
		BITCLR(HwCIC, HwCIC_INPR);
	}
	
	if(uiType & SET_CIF_INPATH_FLUSH_ENABLE)
	{
		BITSET(HwCIC, HwCIC_FA);
	}
	
	if(uiType & SET_CIF_INPATH_FLUSH_DISABLE)
	{
		BITCLR(HwCIC, HwCIC_FA);
	}
	
	if(uiType & SET_CIF_INPATH_ENABLE)
	{
		BITSET(HwCIC, HwCIC_INE);
	}
	
	if(uiType & SET_CIF_INPATH_DISABLE)
	{
		BITCLR(HwCIC, HwCIC_INE);
	}
	
	if(uiType & SET_CIF_INPATH_MEM)
	{
		BITSET(HwCIC, HwCIC_INP);
	}
	
	if(uiType & SET_CIF_INPATH_CAM)
	{
		BITCLR(HwCIC, HwCIC_INP);
	}

#endif	
}


/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setInpathAddr
//
//	DESCRIPTION
//    		set CIF register  : 	HwCISA1,HwCISA2, HwCISA3
//
//	Parameters
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetInpathAddr(unsigned int uiSrcType, unsigned int uiSrcBase, unsigned int uiSrcBaseY, 
								 unsigned int uiSrcBaseU, unsigned int uiSrcBaseV)
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else
	BITCSET(HwCISA2, HwCISA2_SRCTYPE_420SEP, (uiSrcType<<28));
	BITCSET(HwCISA1, HwCISA1_SRC_BASE, (uiSrcBase << 28));  // HwCEM->CISA1
	//HwCISA1_SRC_BASE_Y         0x0FFFFFFF             // SRC_BASE_Y [27:0] Source base address in Y channel (27 down to 0 bit assign in bass address)
	BITCSET(HwCISA1, HwCISA1_SRC_BASE_Y, (uiSrcBaseY)); // HwCEM->CISA1
	//HwCISA2_SRC_BASE_U         0x0FFFFFFF              // SRC_BASE_U [27:0] Source base address in U channal (27 down to 0 bit assign in base address)
	BITCSET(HwCISA2, HwCISA2_SRC_BASE_U, (uiSrcBaseU)); // HwCEM->CISA2
	//HwCISA3_SRC_BASE_V           0x0FFFFFFF           // SRC_BASE_V [27:0] Source base address in V channal (27 down to 0 bit assign in base address)
	BITCSET(HwCISA3, HwCISA3_SRC_BASE_V, (uiSrcBaseV)); // HwCEM->CISA3
#endif
}


/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setInpathScale
//
//	DESCRIPTION
//    		set CIF register  : 	HwCISS, HwCISO, HwCIDS, HwCIS
//
//	Parameters
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetInpathScale(unsigned int uiType, unsigned int uiSrcHSize, unsigned int uiSrcVSize, 
								  unsigned int uiOffY, unsigned int uiOffC, unsigned int uiDstHSize,
								  unsigned int uiDstVSize, unsigned int uiHScale, unsigned int uiVScale)
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else
	if(uiType & SET_CIF_INPATH_SRC_SIZE)
	{
		BITCSET(HwCISS, HwCISS_SRC_HSIZE, (uiSrcHSize<<16)); // HwCEM->CISS
		BITCSET(HwCISS, HwCISS_SRC_VSIZE, (uiSrcVSize));    
	}
	
	if(uiType & SET_CIF_INPATH_SRC_OFFSET)
	{
		BITCSET(HwCISO, HwCISO_SRC_OFFSET_Y, (uiOffY<<16)); //HwCEM->CISO
		BITCSET(HwCISO, HwCISO_SRC_OFFSET_C, (uiOffC));    
	}
	
	if(uiType & SET_CIF_INPATH_DST_SIZE)
	{
		BITCSET(HwCIDS, HwCIDS_DST_HSIZE, (uiDstHSize<<16)); // HwCEM->CIDS
		BITCSET(HwCIDS, HwCIDS_DST_VSIZE, (uiDstVSize));    
	}
	
	if(uiType & SET_CIF_INPATH_SCALE)
	{
		BITCSET(HwCIS, HwCIS_HSCALE, (uiHScale<<16)); // HwCEM->CIS
		BITCSET(HwCIS, HwCIS_VSCALE, (uiVScale));    
	}
#endif
}

/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setDownScale
//
//	DESCRIPTION
//    		set CIF register  : 	HwCDS
//
//	Parameters
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetDownScale(unsigned int uiScaleEnable, unsigned int uiXScale, unsigned int uiYScale)
{
#if  defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else
	BITCSET(HwCDS, HwCDS_SEN_EN, uiScaleEnable); // HwCIF->CDS
	BITCSET(HwCDS, HwCDS_SFH_8, (uiXScale << 4));
	BITCSET(HwCDS, HwCDS_SFV_8, (uiYScale << 2));
#endif
}


/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setScalerCtrl
//
//	DESCRIPTION
// 			CIF Scaler control
//    		set CIF register  : 	HwSCC
//
//	Parameters
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetScalerCtrl(unsigned int uiEN)
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else
	if(uiEN & SET_CIF_SCALER_ENABLE) // HwCSC->CSC
	{
	        BITSET(HwCSC, HwCSC_EN);
	}
	
	if(uiEN & SET_CIF_SCALER_DISABLE)
	{
	        BITCLR(HwCSC, HwCSC_EN);
	}
#endif
}

/*------------------------------------------------------------------------*/
//	NAME : DEV_CIF_setFreeScale
//
//	DESCRIPTION
//			CIF scaler setting
//    		set CIF register  : 	HwSCSS, HwSCSO, HwSCDS, HwSCSF
//
//	Parameters
//
//	Returns
//    	
/* -----------------------------------------------------------------------*/
void TDD_CIF_SetFreeScale(unsigned int uiType, unsigned int uiSrcHSize, unsigned int uiSrcVSize,
								unsigned int uiOffH, unsigned int uiOffV, unsigned int uiDstHSize,
								unsigned int uiDstVSize, unsigned int uiHFactor, unsigned int uiVFactor)
{
#if defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else
	if(uiType & SET_CIF_SCALER_SRC_SIZE)
	{
              BITCSET(HwCSSS, HwCSSS_HSIZE, (uiSrcHSize<<16));
              BITCSET(HwCSSS, HwCSSS_VSIZE, (uiSrcVSize));    
	}
	
	if(uiType & SET_CIF_SCALER_SRC_OFFSET)
	{
              BITCSET(HwCSSO, HwCSSO_OFFSET_H, (uiOffH<<16));
              BITCSET(HwCSSO, HwCSSO_OFFSET_V, (uiOffV));
	}
	
	if(uiType & SET_CIF_SCALER_DST_SIZE)
	{
              BITCSET(HwCSDS, HwCSDS_HSIZE, (uiDstHSize<<16));
              BITCSET(HwCSDS, HwCSDS_VSIZE, (uiDstVSize));    
	}
	
	if(uiType & SET_CIF_SCALER_FACTOR)
	{
              BITCSET(HwCSSF, HwCSSF_HSCALE, (uiHFactor<<16));
              BITCSET(HwCSSF, HwCSSF_VSCALE, (uiVFactor));    
	}
#endif
}

#if defined(CONFIG_USE_ISP)
void ISP_INTERRUPT_SET(void)
{
	volatile PPIC pPIC = (PPIC)tcc_p2v(HwPIC_BASE);

 	//BITSET(pPIC->IEN0, HwINT0_ISP0);
	//BITSET(pPIC->SEL0, HwINT0_ISP0);

#ifndef	CONFIG_ARCH_TCC93XX
	BITSET(pPIC->IEN0, HwINT0_ISP1);
	BITSET(pPIC->SEL0, HwINT0_ISP1);

	BITSET(pPIC->IEN0, HwINT0_ISP2);
	BITSET(pPIC->SEL0, HwINT0_ISP2);
#elif	 defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else
	BITSET(pPIC->IEN0, HwINT1_ISP1);
	BITSET(pPIC->SEL0, HwINT1_ISP1);

	BITSET(pPIC->IEN0, HwINT0_ISP2);
	BITSET(pPIC->SEL0, HwINT0_ISP2);
#endif
	//BITSET(pPIC->IEN0, HwINT0_ISP3);
	//BITSET(pPIC->SEL0, HwINT0_ISP3);
}

void ISP_INTERRUPT_CLEAR(void)
{
	volatile PPIC pPIC = (PPIC)tcc_p2v(HwPIC_BASE);

 	//BITCLR(pPIC->IEN0, HwINT0_ISP0);
#ifndef	CONFIG_ARCH_TCC93XX
	BITCLR(pPIC->IEN0, HwINT0_ISP1);
	BITCLR(pPIC->IEN0, HwINT0_ISP2);
#elif defined(CONFIG_ARCH_TCC893X)
	//	In case of 892X, we have to add.	
#else
	BITCLR(pPIC->IEN0, HwINT1_ISP1);
	BITCLR(pPIC->IEN0, HwINT0_ISP2);
#endif
	//BITCLR(pPIC->IEN0, HwINT0_ISP3);
}
#endif

void TDD_CIF_SWReset(void)
{
	#if defined(CONFIG_ARCH_TCC893X)
		//	In case of 892X, we have to add.	
	#endif
}

/* end of file */

