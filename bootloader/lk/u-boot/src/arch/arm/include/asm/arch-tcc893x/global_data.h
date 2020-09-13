#ifndef _TCC_GLOABAL_DATA_H_
#define _TCC_GLOABAL_DATA_H_

#include <common.h>
#include <asm/arch/globals.h>
#include <asm/arch/reg_physical.h>

#if defined(CONFIG_CHIP_TCC8933S) || defined(CONFIG_CHIP_TCC8935S) || defined(CONFIG_CHIP_TCC8937S)
#define MAX_TCC_PLL      4
#else
#define MAX_TCC_PLL      6
#endif

#define MAX_CLK_SRC             (MAX_TCC_PLL*2 + 1)

struct tcc_global_data{
#ifdef CONFIG_TCC893X
	unsigned long   st_ip_pwdn_reg;
#if !defined(CONFIG_CHIP_TCC8935S) && !defined(CONFIG_CHIP_TCC8933S) && !defined(CONFIG_CHIP_TCC8937S)
	PCLK_ZZZ_TYPE st_pkt_gen_reg[4];
#endif
	unsigned long   st_clk_src[MAX_TCC_PLL*2 + 1];
#endif
};

#endif /*_GLOABAL_DATA_H_*/

