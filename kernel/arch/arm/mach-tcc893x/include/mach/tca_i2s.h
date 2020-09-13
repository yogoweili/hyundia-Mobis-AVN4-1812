/*
 * arch/arm/mach-tcc88xx/include/mach/tca_i2s.h
 *
 */

#include <mach/bsp.h>

#define BASE_ADDR_PIC       HwPIC_BASE

#if defined(CONFIG_M805S_8923_0XA) || defined(CONFIG_M805S_8925_0XX)
#undef CONFIG_MACH_M805_892X
#endif

#if defined(CONFIG_MACH_M805_892X)
#define BASE_ADDR_ADMA      HwAUDIO1_ADMA_BASE
#define BASE_ADDR_DAI       HwAUDIO1_DAI_BASE
#define BASE_ADDR_SPDIFTX   HwAUDIO1_SPDIFTX_BASE
#define BASE_ADDR_SPDIFRX   HwAUDIO0_SPDIFRX_BASE	// Planet 20130709 S/PDIF_Rx
#define BASE_ADDR_CDIF      HwAUDIO1_CDIF_BASE		// Planet 20130801 CDIF

#define CLK_NAME_ADMA       "adma1"
#define CLK_NAME_DAI        "dai1"
#define CLK_NAME_SPDIF      "spdif1"

#else
#if defined(CONFIG_ARCH_TCC893X)
#define	BASE_ADDR_ADMA0      HwAUDIO0_ADMA_BASE
#define BASE_ADDR_DAI0       HwAUDIO0_DAI_BASE
#define BASE_ADDR_SPDIFTX0   HwAUDIO0_SPDIFTX_BASE
#define BASE_ADDR_SPDIFRX0   HwAUDIO0_SPDIFRX_BASE	// Planet 20130709 S/PDIF_Rx
#define BASE_ADDR_CDIF0      HwAUDIO0_CDIF_BASE		// Planet 20130801 CDIF

#define CLK_NAME_ADMA0       "adma0"
#define CLK_NAME_DAI0        "dai0"
#define CLK_NAME_DAM0        "dam0"
#define CLK_NAME_SPDIF0      "spdif0"

#define	BASE_ADDR_ADMA1      HwAUDIO1_ADMA_BASE
#define BASE_ADDR_DAI1       HwAUDIO1_DAI_BASE
#define BASE_ADDR_SPDIFTX1   HwAUDIO1_SPDIFTX_BASE
#define BASE_ADDR_CDIF1      HwAUDIO1_CDIF_BASE		// Planet 20130801 CDIF

#define CLK_NAME_ADMA1       "adma1"
#define CLK_NAME_DAI1        "dai1"
#define CLK_NAME_SPDIF1      "spdif1"
#define BASE_ADDR_IOBUSCFG   HwIOBUSCFG_BASE

#else
#define	BASE_ADDR_ADMA      HwAUDIO0_ADMA_BASE
#define BASE_ADDR_DAI       HwAUDIO0_DAI_BASE
#define BASE_ADDR_SPDIFTX   HwAUDIO0_SPDIFTX_BASE
#define BASE_ADDR_SPDIFRX   HwAUDIO0_SPDIFRX_BASE	// Planet 20130709 S/PDIF_Rx
#define BASE_ADDR_CDIF      HwAUDIO0_CDIF_BASE		// Planet 20130801 CDIF

#define CLK_NAME_ADMA       "adma0"
#define CLK_NAME_DAI        "dai0"
#define CLK_NAME_SPDIF      "spdif0"
#endif
#endif

