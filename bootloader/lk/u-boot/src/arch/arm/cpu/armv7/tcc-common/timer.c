#include <common.h>
#include <div64.h>
#include <asm/io.h>
#include <pwm.h>
#include <asm/arch/reg_physical.h>

DECLARE_GLOBAL_DATA_PTR;

#define TIMER_TICK_RATE 12000000 //ZCLK

int timer_init(void)
{
	volatile PTIMER pTimer= (PTIMER)HwTMR_BASE;
	uint32_t ticks_per_interval = TIMER_TICK_RATE / CONFIG_SYS_HZ; // interval is in ms

	pTimer->TC32EN.nREG = 0;    /* stop the timer */
	pTimer->TC32LDV.nREG = 0;    	
	pTimer->TC32EN.nREG = ticks_per_interval | (1 << 24);    /* stop the timer */

	return 0;
}

/*
 * This function is derived from PowerPC code (read timebase as long long).
 * On ARM it just returns the timer value.
 */
unsigned long long get_ticks(void)
{
	volatile PTIMER pTimer= (PTIMER)HwTMR_BASE;

	return pTimer->TC32MCNT.bREG.MCNT;
}


/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On ARM it returns the number of timer ticks per second.
 */
unsigned long get_tbclk(void)
{
	return CONFIG_SYS_HZ;
}

