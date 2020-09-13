/*
 * linux/arch/arm/mach-tccxxxx/tcc_ddr.c
 *
 * Author:  <linux@telechips.com>
 * Created: November, 2010
 * Description: to change memory bus clock for Telechips chipset
 *
 * Copyright (C) Telechips, Inc.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You should have received a copy of the  GNU General Public License along
 * with this program; if not, write  to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*===========================================================================

                         INCLUDE FILES FOR MODULE

===========================================================================*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <mach/hardware.h>
#include <asm/tlbflush.h>
#include <asm/cacheflush.h>
#include <mach/bsp.h>
#include <mach/tcc_ddr.h>
#include <mach/tca_lcdc.h>
#if defined(CONFIG_DRAM_DDR3) && defined(CONFIG_CACHE_L2X0)
#include <asm/hardware/cache-l2x0.h>
#endif

#include <asm/mach-types.h>


/*===========================================================================

                  DEFINITIONS AND DECLARATIONS FOR MODULE

===========================================================================*/

extern unsigned int IO_ARM_ChangeStackSRAM(void);
extern void IO_ARM_RestoreStackSRAM(unsigned int);
static unsigned int get_cycle(unsigned int tCK, unsigned int ps, unsigned int ck);

#define addr_clk(b) (0x74000000+b)
#define addr_mem(b) (0x73000000+b)
#define time2cycle(time, tCK)		((int)((time + tCK - 1) / tCK))

#define denali_ctl(x) (*(volatile unsigned long *)addr_mem(0x500000+(x*4)))
#define denali_phy(x) (*(volatile unsigned long *)addr_mem(0x600000+(x*4)))
#define ddr_phy(x) (*(volatile unsigned long *)addr_mem(0x420400+(x*4)))

typedef void (*FuncPtr)(void);

#if defined(CONFIG_DRAM_DDR3) && defined(CONFIG_CACHE_L2X0)
#define L2CACHE_BASE   0xFB000000
#endif

#define OLD_SETTING
#define INIT_CONFIRM
#define PHY_MRS_SETTING
//#define CHANGE_MEM_PLL

/*===========================================================================

                                 MAP

===========================================================================*/

/*---------------------------------------------------------------------------
 1) TCC93xx
     << sram1 >>
     0xF0000000(0xF0000000) ------------------
                           |      Stack       | 0x80
                     0x0800 ------------------
                     0x1000 ------------------
                           | ckc change func  | 0x1000
                     0x1000  ------------------
                           | ckc change arg   | 0x80
                            ------------------

---------------------------------------------------------------------------*/

#define CKC_CHANGE_FUNC_ADDR       0xF0001000

#define CKC_CHANGE_FUNC_SIZE       0x1000

#define CKC_CHANGE_ARG_BASE        0xF0002000



/*===========================================================================

                          Clock Register Value

===========================================================================*/

#if defined(CHANGE_MEM_PLL)
typedef struct {
	unsigned int	pll;
	unsigned int	clkctrl;   
	unsigned int	freq;
} MEMCLK;

MEMCLK mem_clk_table[]	=
{
	{0x41019006, 0x00200011, 8000000},
	{0x41016806, 0x00200011, 7200000},
	{0x01014D06, 0x00200011, 6660000},
	{0x01014A06, 0x00200011, 6600000},
	{0x01013A06, 0x00200011, 6280000},
	{0x01012C06, 0x00200011, 6000000},
	{0x01011406, 0x00200011, 5520000},
	{0x01010A06, 0x00200011, 5320000},
	{0x4201F406, 0x00200011, 5000000},
	{0x42019006, 0x00200011, 4000000},
	{0x02013006, 0x00200011, 3040000},
};
#endif

/*===========================================================================

                          Clock Change Argument

===========================================================================*/

#define CKC_CHANGE_ARG(x)   (*(volatile unsigned long *)(CKC_CHANGE_ARG_BASE + (4*(x))))

#if defined(CONFIG_DRAM_DDR3)
enum {
	PLL_VALUE = 0,
	CKC_CTRL_VALUE,
	CLK_RATE,
	DENALI_CTL_2,
	DENALI_CTL_3,
	DENALI_CTL_4,
	DENALI_CTL_5,
	DENALI_CTL_6,
	DENALI_CTL_7,
	DENALI_CTL_8,
	DENALI_CTL_9,
	DENALI_CTL_10,
	DENALI_CTL_11,
	DENALI_CTL_12,
	DENALI_CTL_13,
	tRFC,
	DENALI_CTL_15,
	DENALI_CTL_16,
	DENALI_CTL_18,
	tCKSRX,
	tCKSRE,
	DENALI_CTL_101,
	MR0,
	MR1,
	MR2,
	MR3
};
#else
	#error "not selected ddr type.."
#endif


/*===========================================================================
FUNCTION
 - mem_freq : MHz unit
===========================================================================*/
static void get_ddr_param(unsigned int mem_freq)
{
#if defined(CONFIG_DRAM_DDR3)

	unsigned tck = 0;
	unsigned nCL, nCWL;
	unsigned tmp;

	tck = (1000000/mem_freq);

	if(tck >= 2500){ /* 2.5ns, 400MHz */
		nCL = 6;
		nCWL = 5;
	}else if(tck >= 1875){ // 1.875ns, 533..MHz
		nCL = 8;
		nCWL = 6;
	}else if(tck >= 1500){ // 1.5 ns, 666..MHz
		if(DDR3_MAX_SPEED < DDR3_1600)
			nCL = 9;
		else
			nCL = 10;
		nCWL = 7;
	}else if(tck >= 1250){ // 1.25 ns, 800MHz
		nCL = 11;
		nCWL = 8;
	}else if(tck >= 1070){ // 1.07 ns, 933..MHz
		nCL = 13;
		nCWL = 9;
	}else if(tck >= 935){ // 0.935 ns, 1066..MHz
		nCL = 14;
		nCWL = 10;
	}

#ifdef OLD_SETTING
	CKC_CHANGE_ARG(DENALI_CTL_2) = time2cycle(10000, tck); //TINIT[23:0]
	CKC_CHANGE_ARG(DENALI_CTL_3) = 0; //time2cycle(200000000, tck); //TRST_PWRON
	CKC_CHANGE_ARG(DENALI_CTL_4) = time2cycle(500000000, tck); //CKE_INACTIVE
	if(DDR3_AL == AL_DISABLED){ //nAL = 0;
		CKC_CHANGE_ARG(DENALI_CTL_5) = (0x1<<24 | (nCWL)<<16 | ((nCL<<1)|0)<<8 | 0x8);
	}
	else if(DDR3_AL == AL_CL_MINUS_ONE){ //nAL = nCL - 1;
		CKC_CHANGE_ARG(DENALI_CTL_5) = (0x1<<24 | (nCWL+nCL-1)<<16 | ((nCL<<1)|0)<<8 | 0x8);
	}	
	else if(DDR3_AL == AL_CL_MINUS_TWO){ //nAL = nCL - 2;
		CKC_CHANGE_ARG(DENALI_CTL_5) = (0x1<<24 | (nCWL+nCL-2)<<16 | ((nCL<<1)|0)<<8 | 0x8);
	}
#else
	CKC_CHANGE_ARG(DENALI_CTL_2) = 0x00000007; //TINIT[23:0]
	CKC_CHANGE_ARG(DENALI_CTL_3) = 0; //0x000000a0; //TRST_PWRON
	CKC_CHANGE_ARG(DENALI_CTL_4) = 0x00000090; //CKE_INACTIVE
	if(DDR3_AL == AL_DISABLED){ //nAL = 0;
		CKC_CHANGE_ARG(DENALI_CTL_5) = (0x4<<24 | (nCWL)<<16 | ((nCL<<1)|0)<<8 | 0x0);
	}
	else if(DDR3_AL == AL_CL_MINUS_ONE){ //nAL = nCL - 1;
		CKC_CHANGE_ARG(DENALI_CTL_5) = (0x4<<24 | (nCWL+nCL-1)<<16 | ((nCL<<1)|0)<<8 | 0x0);
	}	
	else if(DDR3_AL == AL_CL_MINUS_TWO){ //nAL = nCL - 2;
		CKC_CHANGE_ARG(DENALI_CTL_5) = (0x4<<24 | (nCWL+nCL-2)<<16 | ((nCL<<1)|0)<<8 | 0x0);
	}
#endif

	CKC_CHANGE_ARG(DENALI_CTL_6) = (get_cycle(tck, DDR3_tRAS_ps, DDR3_tRAS_ck)<<24 | get_cycle(tck, DDR3_tRC_ps, DDR3_tRC_ck)<<16 | get_cycle(tck, DDR3_tRRD_ps, DDR3_tRRD_ck)<<8 | DDR3_tCCD_ck);
	CKC_CHANGE_ARG(DENALI_CTL_7) = (DDR3_tMRD_ck<<24 | get_cycle(tck, DDR3_tRTP_ps, DDR3_tRTP_ck)<<16 | get_cycle(tck, DDR3_tRP_ps, DDR3_tRP_ck)<<8 | (get_cycle(tck, DDR3_tWTR_ps, DDR3_tWTR_ck)+1));
	CKC_CHANGE_ARG(DENALI_CTL_8) = (get_cycle(tck, DDR3_tRAS_MAX_ps, 1)<<8 | get_cycle(tck, DDR3_tMOD_ps, DDR3_tMOD_ck));
	CKC_CHANGE_ARG(DENALI_CTL_9) = ((get_cycle(tck, DDR3_tCKE_ps, DDR3_tCKE_ck)+1)<<8 | get_cycle(tck, DDR3_tCKE_ps, DDR3_tCKE_ck));
	CKC_CHANGE_ARG(DENALI_CTL_10) = (get_cycle(tck, DDR3_tWR_ps, DDR3_tWR_ck)<<24 | get_cycle(tck, DDR3_tRCD_ps, DDR3_tRCD_ck)<<16 | 1<<8 | 1);
	CKC_CHANGE_ARG(DENALI_CTL_11) = (1<<24 | DDR3_tDLLK_ck<<8 | (get_cycle(tck, DDR3_tWR_ps, DDR3_tWR_ck)+nCL));
	CKC_CHANGE_ARG(DENALI_CTL_12) = (1<<16 | get_cycle(tck, DDR3_tFAW_ps, DDR3_tFAW_ck)<<8 | 3);
	CKC_CHANGE_ARG(DENALI_CTL_13) = get_cycle(tck, DDR3_tRP_ps, DDR3_tRP_ck)+1;
	CKC_CHANGE_ARG(tRFC) = get_cycle(tck, DDR3_tRFC_ps, 1);
	CKC_CHANGE_ARG(DENALI_CTL_15) = get_cycle(tck, DDR3_tREFI_ps, 1);
	CKC_CHANGE_ARG(DENALI_CTL_16) = (get_cycle(tck, DDR3_tXPDLL_ps, DDR3_tXPDLL_ck)<<16 | get_cycle(tck, DDR3_tXP_ps, DDR3_tXP_ck)); // DDR3 Only
	CKC_CHANGE_ARG(DENALI_CTL_18) = (get_cycle(tck, DDR3_tXS_ps, DDR3_tXS_ck)<<16 | DDR3_tXSDLL_ck);
	CKC_CHANGE_ARG(tCKSRX) = get_cycle(tck, DDR3_tCKSRX_ps, DDR3_tCKSRX_ck);
	CKC_CHANGE_ARG(tCKSRE) = get_cycle(tck, DDR3_tCKSRE_ps, DDR3_tCKSRE_ck);

//--------------------------------------------------------------------------
// DFI Timing

	CKC_CHANGE_ARG(DENALI_CTL_101) = (0x00<<24|0x02<<16|(nCWL-3+(nCWL%2))<<8|(nCL-3+(nCL%2)));

//--------------------------------------------------------------------------
// MRS Setting

	// MRS0
	tmp = DDR3_BURST_LEN | (DDR3_READ_BURST_TYPE<<3);

	if(nCL < 5)
		tmp |= ((5-4)<<4);
	else if(nCL > 11)
		tmp |= ((11-4)<<4);
	else
		tmp |= ((nCL-4)<<4);

	if(get_cycle(tck, DDR3_tWR_ps, DDR3_tWR_ck) <= 5) // Write Recovery for autoprecharge
		tmp |= WR_5<<9;
	else if(get_cycle(tck, DDR3_tWR_ps, DDR3_tWR_ck) == 6)
		tmp |= WR_6<<9;
	else if(get_cycle(tck, DDR3_tWR_ps, DDR3_tWR_ck) == 7)
		tmp |= WR_7<<9;
	else if(get_cycle(tck, DDR3_tWR_ps, DDR3_tWR_ck) == 8)
		tmp |= WR_8<<9;
	else if((get_cycle(tck, DDR3_tWR_ps, DDR3_tWR_ck) == 9) || (get_cycle(tck, DDR3_tWR_ps, DDR3_tWR_ck) == 10))
		tmp |= WR_10<<9;
	else if(get_cycle(tck, DDR3_tWR_ps, DDR3_tWR_ck) >= 11)
		tmp |= WR_12<<9;

	tmp	|= (1<<8); // DLL Reset

	CKC_CHANGE_ARG(MR0) = tmp;

	// MRS1
	CKC_CHANGE_ARG(MR1) = ((DDR3_AL<<3) | (DDR3_ODT<<2) | (DDR3_DIC<<1));

	// MRS2
	tmp = 0;
	if(nCWL <= 5)
		tmp |= 0;
	else if(nCWL == 6)
		tmp |= 1<<3;
	else if(nCWL == 7)
		tmp |= 2<<3;
	else if(nCWL >= 8)
		tmp |= 3<<3;

	CKC_CHANGE_ARG(MR2) = tmp;

//	printk("mem_freq=%dMhz, nCL=%d, nCWL=%d\n", mem_freq, nCL, nCWL);
//	printk("T_REFI:0x%x, T_RFC:0x%x, T_RRD:0x%x, T_RP:0x%x, T_RCD:0x%x\n",CKC_CHANGE_ARG(T_REFI),CKC_CHANGE_ARG(T_RFC),CKC_CHANGE_ARG(T_RRD),CKC_CHANGE_ARG(T_RP),CKC_CHANGE_ARG(T_RCD));
//	printk("T_RC:0x%x, T_RAS:0x%x, T_WTR:0x%x, T_WR:0x%x, T_RTP:0x%x\n",CKC_CHANGE_ARG(T_RC),CKC_CHANGE_ARG(T_RAS),CKC_CHANGE_ARG(T_WTR),CKC_CHANGE_ARG(T_WR),CKC_CHANGE_ARG(T_RTP));
//	printk("CL:0x%x, WL:0x%x, RL:0x%x\n",CKC_CHANGE_ARG(CL),CKC_CHANGE_ARG(WL),CKC_CHANGE_ARG(RL));
//	printk("T_FAW:0x%x, T_XSR:0x%x, T_XP:0x%x, T_CKE:0x%x, T_MRD:0x%x\n",CKC_CHANGE_ARG(T_FAW),CKC_CHANGE_ARG(T_XSR),CKC_CHANGE_ARG(T_XP),CKC_CHANGE_ARG(T_CKE),CKC_CHANGE_ARG(T_MRD));
//	printk("MR0:0x%x, MR1:0x%x, MR2:0x%x, MR3:0x%x\n", CKC_CHANGE_ARG(MR0_DDR3), CKC_CHANGE_ARG(MR1_DDR3), CKC_CHANGE_ARG(MR2_DDR3), CKC_CHANGE_ARG(MR3_DDR3));

#else
	#error "not selected ddr type.."
#endif
}

/*===========================================================================
FUNCTION
===========================================================================*/
static void change_clock(void)
{
#if defined(CONFIG_DRAM_DDR3)

	volatile int i;
	volatile int tmp;

//--------------------------------------------------------------
// remap 0xF000000 to SRAM.
	*(volatile unsigned*)0xF4810010 |= 0x80000000;


// -------------------------------------------------------------------------
// mmu & cache off

	__asm__ volatile (
	"mrc p15, 0, r0, c1, c0, 0 \n"
	"bic r0, r0, #(1<<12) \n" //ICache
	"bic r0, r0, #(1<<2) \n" //DCache
	"bic r0, r0, #(1<<0) \n" //MMU
	"mcr p15, 0, r0, c1, c0, 0 \n"
	"nop \n"
	);

//--------------------------------------------------------------------------
// holding to access to dram

	denali_ctl(47) = 0xFFFFFFFF;
	BITSET(denali_ctl(44),0x1); //inhibit_dram_cmd=1

//--------------------------------------------------------------------------
//enter self-refresh

	BITSET(denali_phy(13), 0x1<<10); //lp_ext_req=1
	while(!(denali_phy(13)&(0x1<<26))); //until lp_ext_ack==1
	BITCSET(denali_phy(13), 0x000000FF, (2<<2)|(1<<1)|(0));
	BITSET(denali_phy(13), 0x1<<8); //lp_ext_cmd_strb=1
	while((denali_phy(13)&(0x003F0000)) != (0x25<<16)); //until lp_ext_state==0x25 : check self-refresh state
	BITCLR(denali_phy(13), 0x1<<8); //lp_ext_cmd_strb=0
	BITCLR(denali_phy(13), 0x1<<10); //lp_ext_req=0
	while(denali_phy(13)&(0x1<<26)); //until lp_ext_ack==0
	BITSET(denali_ctl(96), 0x3<<8); //DRAM_CLK_DISABLE[9:8] = [CS1, CS0] = 0x3
	BITCLR(denali_ctl(0),0x1); //START[0] = 0

	ddr_phy(0x0E) = 0xffffffff; // AC IO Config

//--------------------------------------------------------------------------
//Clock setting..

#if defined(CHANGE_MEM_PLL)
	*(volatile unsigned long *)addr_clk(0x000004) = 0x00200014;  // mem bus
	while((*(volatile unsigned long *)addr_clk(0x000004))&(1<<29)); // CFGREQ
	i=100; while(i--);
	*(volatile unsigned long *)addr_clk(0x000034) = CKC_CHANGE_ARG(PLL_VALUE) | 0x00100000; //pll1 for mem , lock_en
	*(volatile unsigned long *)addr_clk(0x000034) |= 0x80100000; //pll1 for mem , lock_en
	i=20; while(i--)while((*(volatile unsigned long *)addr_clk(0x000034) & 0x00200000) == 0);
#endif
	i=100; while(i--);
	*(volatile unsigned long *)addr_clk(0x000004) = CKC_CHANGE_ARG(CKC_CTRL_VALUE);  // mem bus
	while((*(volatile unsigned long *)addr_clk(0x000004))&(1<<29)); // CFGREQ
	i=10000; while(i--);

//--------------------------------------------------------------------------
// Timing Parameter

#if (1)
	denali_ctl(2) = CKC_CHANGE_ARG(DENALI_CTL_2);
	denali_ctl(3) = CKC_CHANGE_ARG(DENALI_CTL_3);
	denali_ctl(4) = CKC_CHANGE_ARG(DENALI_CTL_4);
	denali_ctl(5) = CKC_CHANGE_ARG(DENALI_CTL_5);
	denali_ctl(6) = CKC_CHANGE_ARG(DENALI_CTL_6);
	denali_ctl(7) = CKC_CHANGE_ARG(DENALI_CTL_7);
	denali_ctl(8) = CKC_CHANGE_ARG(DENALI_CTL_8);
	denali_ctl(9) = CKC_CHANGE_ARG(DENALI_CTL_9);
	denali_ctl(10) = CKC_CHANGE_ARG(DENALI_CTL_10);
	denali_ctl(11) = CKC_CHANGE_ARG(DENALI_CTL_11);
	denali_ctl(12) = CKC_CHANGE_ARG(DENALI_CTL_12);
	denali_ctl(13) = CKC_CHANGE_ARG(DENALI_CTL_13);
	denali_ctl(14) = (CKC_CHANGE_ARG(tRFC)<<16 | 1<<8 | 0);
	denali_ctl(15) = CKC_CHANGE_ARG(DENALI_CTL_15);
	denali_ctl(16) = CKC_CHANGE_ARG(DENALI_CTL_16);
	denali_ctl(18) = CKC_CHANGE_ARG(DENALI_CTL_18);
	denali_ctl(20) = (CKC_CHANGE_ARG(tCKSRX)<<16 | CKC_CHANGE_ARG(tCKSRE)<<8);

//--------------------------------------------------------------------------
// DFI Timing

	denali_ctl(101) = CKC_CHANGE_ARG(DENALI_CTL_101);

//--------------------------------------------------------------------------
// MRS Setting

	// MRS0
	denali_ctl(27) = CKC_CHANGE_ARG(MR0)<<8;
	BITCSET(denali_ctl(30), 0x0000FFFF, CKC_CHANGE_ARG(MR0));
#ifdef PHY_MRS_SETTING
	ddr_phy(0x15) = CKC_CHANGE_ARG(MR0);
#endif

	// MRS1
	BITCSET(denali_ctl(28), 0x0000FFFF, CKC_CHANGE_ARG(MR1));
	BITCSET(denali_ctl(30), 0xFFFF0000, CKC_CHANGE_ARG(MR1)<<16);
#ifdef PHY_MRS_SETTING
	ddr_phy(0x16) = CKC_CHANGE_ARG(MR1);
#endif

	// MRS2
	BITCSET(denali_ctl(28), 0xFFFF0000, CKC_CHANGE_ARG(MR2)<<16);
	BITCSET(denali_ctl(31), 0x0000FFFF, CKC_CHANGE_ARG(MR2));
#ifdef PHY_MRS_SETTING
	ddr_phy(0x17) = CKC_CHANGE_ARG(MR2);
#endif
#endif

//--------------------------------------------------------------------------
// Exit self-refresh

	ddr_phy(0x0E) = 0x30c01812; // AC IO Config

	BITCLR(denali_ctl(96), 0x3<<8); //DRAM_CLK_DISABLE[9:8] = [CS1, CS0] = 0x0
	BITCLR(denali_ctl(39), 0x3<<24); //ZQ_ON_SREF_EXIT = 0
 	BITSET(denali_ctl(0),0x1); //START[0] = 1

	//for(i=0;i<11;i++){
	//	BITSET(denali_ctl(123), 0x1<<16); //CTRLUPD_REQ = 1
	//	while(!(denali_ctl(46)&(0x20000)));
	//	BITSET(denali_ctl(47), 0x20000);
	//}

	BITCSET(denali_ctl(20), 0xFF000000, ((2<<2)|(0<<1)|(1))<<24);
	while(!(denali_ctl(46)&(0x40)));
	BITSET(denali_ctl(47), 0x40);

//--------------------------------------------------------------------------
// MRS Write

	// MRS2
	BITCSET(denali_ctl(29), 0x0000FFFF, (denali_ctl(28)&0xFFFF0000)>>16);
	BITCSET(denali_ctl(31), 0xFFFF0000, (denali_ctl(31)&0x0000FFFF)<<16);
	denali_ctl(26) = (2<<0)|(1<<23)|(1<<24)|(1<<25); // MR Select[7-0], MR enable[23], All CS[24], Trigger[25]
	while(!(denali_ctl(46)&(0x4000)));
	BITSET(denali_ctl(47), 0x4000);

	// MRS3
	BITCSET(denali_ctl(29), 0x0000FFFF, (denali_ctl(29)&0xFFFF0000)>>16);
	BITCSET(denali_ctl(31), 0xFFFF0000, (denali_ctl(32)&0x0000FFFF)<<16);
	denali_ctl(26) = (3<<0)|(1<<23)|(1<<24)|(1<<25); // MR Select[7-0], MR enable[23], All CS[24], Trigger[25]
	while(!(denali_ctl(46)&(0x4000)));
	BITSET(denali_ctl(47), 0x4000);

	// MRS1
	BITCSET(denali_ctl(29), 0x0000FFFF, (denali_ctl(28)&0x0000FFFF)>>0);
	BITCSET(denali_ctl(31), 0xFFFF0000, (denali_ctl(30)&0xFFFF0000)<<0);
	denali_ctl(26) = (1<<0)|(1<<23)|(1<<24)|(1<<25); // MR Select[7-0], MR enable[23], All CS[24], Trigger[25]
	while(!(denali_ctl(46)&(0x4000)));
	BITSET(denali_ctl(47), 0x4000);

	// MRS0
	BITCSET(denali_ctl(29), 0x0000FFFF, (denali_ctl(27)&0x00FFFF00)>>8);
	BITCSET(denali_ctl(31), 0xFFFF0000, (denali_ctl(30)&0x0000FFFF)<<16);
	denali_ctl(26) = (0<<0)|(1<<23)|(1<<24)|(1<<25); // MR Select[7-0], MR enable[23], All CS[24], Trigger[25]
	while(!(denali_ctl(46)&(0x4000)));
	BITSET(denali_ctl(47), 0x4000);

//--------------------------------------------------------------------------

	BITCLR(denali_ctl(40) ,0x1<<16); //ZQCS_ROTATE=0x0
	BITCSET(denali_ctl(39) ,0x3<<16, 0x2<<16); //ZQ_REQ=2 : 0x1=short calibration, 0x2=long calibration

//--------------------------------------------------------------------------
// release holding to access to dram

	i = 10;	while(i--) tmp = BITSET(denali_ctl(13), 1<<24); // AREFRESH = 1
	BITCLR(denali_ctl(44),0x1); //inhibit_dram_cmd=0

// -------------------------------------------------------------------------
// mmu & cache on

	__asm__ volatile (
	"mrc p15, 0, r0, c1, c0, 0 \n"
	"orr r0, r0, #(1<<12) \n" //ICache
	"orr r0, r0, #(1<<2) \n" //DCache
	"orr r0, r0, #(1<<0) \n" //MMU
	"mcr p15, 0, r0, c1, c0, 0 \n"
	"nop \n"
	);

//--------------------------------------------------------------
// remap 0xF000000 to DRAM.
	*(volatile unsigned*)0xF4810010 &= ~0x80000000;

//--------------------------------------------------------------------------
#endif	

//--------------------------------------------------------------------------
}



/*===========================================================================

                            Common Functions

===========================================================================*/

/*===========================================================================
FUNCTION
===========================================================================*/
static unsigned int get_cycle(unsigned int tCK, unsigned int ps, unsigned int ck)
{
	unsigned int ret;

	ret = time2cycle(ps, tCK);

	if(ret > ck)
		return ret;
	else
		return ck;
}



/*===========================================================================
FUNCTION
 - return value : MHz unit
===========================================================================*/
static unsigned int get_membus_ckc(unsigned int mem_freq)
{
	unsigned i = 0;

#if defined(CHANGE_MEM_PLL)
	for (i=0 ; i<ARRAY_SIZE(mem_clk_table) ; i++) {
		if (mem_clk_table[i].freq <= mem_freq)
			break;
	}

	if (i >= ARRAY_SIZE(mem_clk_table))
		i = (ARRAY_SIZE(mem_clk_table) - 1);

	CKC_CHANGE_ARG(PLL_VALUE) = mem_clk_table[i].pll;
	CKC_CHANGE_ARG(CKC_CTRL_VALUE) = mem_clk_table[i].clkctrl;
	CKC_CHANGE_ARG(CLK_RATE) = (mem_clk_table[i].freq/10000);

	return (mem_clk_table[i].freq/10000);
#else
	if (mem_freq == 6000000) {
		CKC_CHANGE_ARG(CKC_CTRL_VALUE) = 0x00200011;
		CKC_CHANGE_ARG(CLK_RATE) = 600;
	}
	else if (mem_freq == 3000000) {
		CKC_CHANGE_ARG(CKC_CTRL_VALUE) = 0x00200031;
		CKC_CHANGE_ARG(CLK_RATE) = 300;
	}
	return CKC_CHANGE_ARG(CLK_RATE);
#endif
}

/*===========================================================================
FUNCTION
===========================================================================*/
static void copy_change_clock(void)
{
	unsigned long  flags;
	unsigned int   stack;
	FuncPtr pFunc = (FuncPtr)(CKC_CHANGE_FUNC_ADDR);
	unsigned int	RDMA_sts = 0;
#if defined(CONFIG_DRAM_DDR3) && defined(CONFIG_CACHE_L2X0)
	unsigned way_mask;
#endif
	volatile PPIC	pPIC	= (volatile PPIC)tcc_p2v(HwPIC_BASE);


//--------------------------------------------------------------
// copy clock change function to sram
	memcpy((void *)CKC_CHANGE_FUNC_ADDR, (void*)change_clock, CKC_CHANGE_FUNC_SIZE);

//--------------------------------------------------------------
// disable LCD
#if (0)//defined(CONFIG_DRAM_DDR2)
	#if defined(CONFIG_MACH_TCC8930ST)
		DEV_LCDC_Wait_signal_Ext();
	#else
		DEV_LCDC_Wait_signal(0);
		DEV_LCDC_Wait_signal(1);
	#endif
#endif

//--------------------------------------------------------------
// disable irq
	local_irq_save(flags);
	local_irq_disable();

//--------------------------------------------------------------
// flush tlb
	local_flush_tlb_all();

//--------------------------------------------------------------
// flush internal cache(ICache, DCache)
//	flush_cache_all();	// it spend 1~2milisec. time

//--------------------------------------------------------------
// disable kernel tick timer
//	pPIC->IEN0.bREG.TC1 = 0;		/* Disable Timer1 interrupt */

//--------------------------------------------------------------
// L2 cache off
#if defined(CONFIG_DRAM_DDR3) && defined(CONFIG_CACHE_L2X0)
	if (*(volatile unsigned int *)(L2CACHE_BASE+L2X0_AUX_CTRL) & (1 << 16))
		way_mask = (1 << 16) - 1;
	else
		way_mask = (1 << 8) - 1;
	*(volatile unsigned int *)(L2CACHE_BASE+L2X0_CLEAN_WAY) = way_mask; //clean all
	while(*(volatile unsigned int *)(L2CACHE_BASE+L2X0_CLEAN_WAY)&way_mask); //wait for clean
	*(volatile unsigned int *)(L2CACHE_BASE+L2X0_CACHE_SYNC) = 0; //sync
	while(*(volatile unsigned int *)(L2CACHE_BASE+L2X0_CACHE_SYNC)&1); //wait for sync
	*(volatile unsigned int *)(L2CACHE_BASE+L2X0_CTRL) = 0; //cache off
#endif

//--------------------------------------------------------------
// change stack to sram
	stack = IO_ARM_ChangeStackSRAM();

////////////////////////////////////////////////////////////////
	pFunc();
////////////////////////////////////////////////////////////////

//--------------------------------------------------------------
// restore stack
	IO_ARM_RestoreStackSRAM(stack);

//--------------------------------------------------------------
// L2 cache on
#if defined(CONFIG_DRAM_DDR3) && defined(CONFIG_CACHE_L2X0)
	if (*(volatile unsigned int *)(L2CACHE_BASE+L2X0_AUX_CTRL) & (1 << 16))
		way_mask = (1 << 16) - 1;
	else
		way_mask = (1 << 8) - 1;
	*(volatile unsigned int *)(L2CACHE_BASE+L2X0_INV_WAY) = way_mask; //invalidate all
	while(*(volatile unsigned int *)(L2CACHE_BASE+L2X0_INV_WAY)&way_mask); //wait for invalidate
	*(volatile unsigned int *)(L2CACHE_BASE+L2X0_CACHE_SYNC) = 0; //sync
	while(*(volatile unsigned int *)(L2CACHE_BASE+L2X0_CACHE_SYNC)&1); //wait for sync
	*(volatile unsigned int *)(L2CACHE_BASE+L2X0_CTRL) = 1; //cache on
#endif

	
//--------------------------------------------------------------
// enable irq
	local_irq_restore(flags);

//--------------------------------------------------------------
// enable kernel tick timer interrupt
//	pPIC->IEN0.bREG.TC1 = 1;
}

/*===========================================================================
FUNCTION
===========================================================================*/
void tcc_ddr_set_clock(unsigned int freq)
{
#ifdef CONFIG_MEM_BUS_SCALING
	unsigned int   mem_freq;

	mem_freq = get_membus_ckc(freq*10);
	get_ddr_param(mem_freq);

	printk("**********mem clock change in %dMHz\n", mem_freq);
	copy_change_clock();
	printk("**********mem clock change out\n");
#endif
}

/*=========================================================================*/

EXPORT_SYMBOL(tcc_ddr_set_clock);

