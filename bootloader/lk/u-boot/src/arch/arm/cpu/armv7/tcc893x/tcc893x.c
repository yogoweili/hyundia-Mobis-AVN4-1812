#include <common.h>
#include <asm/arch/globals.h>
#include <asm/arch/reg_physical.h>
#include <asm/arch/tcc_ckc.h>

extern void clock_init_early(void);
int arch_cpu_init(void)
{
	tca_ckc_init();
	clock_init_early();
    return 0;
}

void reset_cpu(ulong reboot_reason)
{
	unsigned int usts;
	PPMU pPMU = (PPMU)HwPMU_BASE;
	PIOBUSCFG pIOBUSCFG = (PIOBUSCFG)HwIOBUSCFG_BASE;

	usts = (reboot_reason == 0x77665500) ? 1 : 0;

	pPMU->PMU_USSTATUS.nREG = usts;
	pPMU->PMU_CONFIG.nREG = (pPMU->PMU_CONFIG.nREG & 0xCFFFFFFF);

	pIOBUSCFG->HCLKEN0.nREG = 0xFFFFFFFF;
	pIOBUSCFG->HCLKEN1.nREG = 0xFFFFFFFF;

	while(1) {
		pPMU->PMU_WDTCTRL.nREG = (1 << 31) | 1;
	}
}


#ifdef CONFIG_DISPLAY_CPUINFO
int print_cpuinfo(void)
{
#if defined(CONFIG_CHIP_TCC8930)
	printf("CPU : TCC8930\n");
#elif defined(CONFIG_CHIP_TCC8935)
	printf("CPU : TCC8935\n");
#else
	#error "TCC Not defined CPU"
#endif

	return 0;
}
#endif
