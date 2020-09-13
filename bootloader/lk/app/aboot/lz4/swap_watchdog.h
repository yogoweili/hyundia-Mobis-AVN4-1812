
/*===================================================================
 *																	*
 *		 			WatchDog Setting Functions.						*
 *																	*
 *=================================================================*/

static uint64_t tccwdt_Timeout = CONFIG_WATCHDOG_TIMEOUT;	// WatchDog Timeout Setting. ( ms unit )


static volatile unsigned int cnt = 0;
#define nop_delay(x) for(cnt=0 ; cnt<x ; cnt++){ \
					__asm__ __volatile__ ("nop\n"); }

#define IO_OFFSET	0x00000000
#define io_p2v(pa)	((pa) - IO_OFFSET)
#define tcc_p2v(pa)         io_p2v(pa)

static PPMU pPMU;
static uint16_t wdtctrl_cnt = 0;	// pPMU->PMU_WDTCTRL.bREG.CNT's Value.
#if defined(PLATFORM_TCC893X)
	#define PMU_CLK		(24*1000*1000)	// PMU Clock : 24M
#endif

/*      Show WatchDog TimeOut ms.       */
static inline void show_watchdogTimeout(void)
{
#if defined(CONFIG_QB_WATCHDOG_ENABLE)
	dprintf(CRITICAL, "WatchDog TimeOut %lldms\n", tccwdt_Timeout);
#else
	dprintf(CRITICAL, "WatchDog is Disabled.\n");
#endif
}

/*		Convert Sec time to WatchDog Counter Register Value 	*/
static inline void tccwdt_counter(uint64_t msec)
{
	uint64_t cnt_reg = 0;

	/*	 2^16 * cnt_reg / PMU_CLK = WTD TimeOut sec.	*/
#if defined(PLATFORM_TCC893X)
	cnt_reg = (msec*(PMU_CLK/1000) +65536 -1)/65536;	// RoundUP cnt_reg at (uint16_t)cnt_reg.
#endif

	if(cnt_reg > 0xFFFF || cnt_reg <= 0) {
		// cnt_reg is OverFlowed or (-) or Zero( 0 ). SET WATCHDOG FULL Timer Count.
		cnt_reg = 0xFFFF;
#if defined(PLATFORM_TCC893X)
		tccwdt_Timeout = (65536 * cnt_reg)/PMU_CLK;
#endif
	}
	
	wdtctrl_cnt = (uint16_t)cnt_reg;
}

/*		Initialize WatchDog		*/
static inline void watchdog_init(void)
{
#if !defined(CONFIG_QB_WATCHDOG_ENABLE)
	return;
#endif
   	pPMU = (volatile PPMU)(tcc_p2v(HwPMU_BASE));
	tccwdt_counter(tccwdt_Timeout);	// SET wdtctrl_cnt register value from tccwdt_Timeout(sec).

#if defined(PLATFORM_TCC893X)
   /* remap to internal ROM */
   pPMU->PMU_CONFIG.bREG.REMAP = 0;

   /* PMU is not initialized with WATCHDOG */
   pPMU->PMU_ISOL.nREG = 0x00000000;
   nop_delay(100000);
   pPMU->PWRUP_MBUS.bREG.DATA = 0;
   nop_delay(100000);
   pPMU->PWRUP_VBUS.bREG.DATA = 0;
   nop_delay(100000);
   pPMU->PWRUP_GBUS.bREG.DATA = 0;
   nop_delay(100000);
   pPMU->PWRUP_DBUS.bREG.DATA = 0;
   nop_delay(100000);
   pPMU->PWRUP_HSBUS.bREG.DATA = 0;
   nop_delay(100000);

   pPMU->PMU_SYSRST.bREG.VB = 1;
   pPMU->PMU_SYSRST.bREG.GB = 1;
   pPMU->PMU_SYSRST.bREG.DB = 1;
   pPMU->PMU_SYSRST.bREG.HSB = 1;
   nop_delay(100000);

   pPMU->PMU_WDTCTRL.bREG.EN = 0;
   pPMU->PMU_WDTCTRL.bREG.CNT = wdtctrl_cnt;	
   pPMU->PMU_WDTCTRL.bREG.EN = 1;
#endif

   show_watchdogTimeout();	// dprintf WatchDog TimeOut
}

static inline void watchdog_clear(void)
{
#if !defined(CONFIG_QB_WATCHDOG_ENABLE)
	return;
#endif
#if defined(PLATFORM_TCC893X)
	pPMU->PMU_WDTCTRL.bREG.CLR = 1; // Clear watchdog
	pPMU->PMU_WDTCTRL.bREG.EN = 0;
	pPMU->PMU_WDTCTRL.bREG.CNT = wdtctrl_cnt;	
	pPMU->PMU_WDTCTRL.bREG.EN = 1;
#endif           
}


static inline void watchdog_disable(void)
{
#if !defined(CONFIG_QB_WATCHDOG_ENABLE)
	return;
#endif
#if defined(PLATFORM_TCC893X)
	pPMU->PMU_WDTCTRL.bREG.CLR = 1; // Clear watchdog
	pPMU->PMU_WDTCTRL.bREG.EN = 0;
#endif           
}

