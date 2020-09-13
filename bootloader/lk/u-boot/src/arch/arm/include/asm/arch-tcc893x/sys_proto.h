/*
 * Copyright (c) 2013 Xilinx Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _SYS_PROTO_H_
#define _SYS_PROTO_H_

struct pmic;


/* Driver extern functions */
extern int tcc_sdhci_init(u32 regbase, int index, int bus_width);
extern int pmic_set_power(struct pmic *p, unsigned int id, unsigned int on);
extern int pmic_set_voltage(struct pmic *p, unsigned int id, unsigned int mV);
extern int axp192_pmic_init(unsigned char bus);
extern int pca953x_set_val(uint8_t chip, uint mask, uint data);
extern int pca953x_set_dir(uint8_t chip, uint mask, uint data);

extern void udc_notify(unsigned event);
extern void *dma_alloc(unsigned int alignment, size_t size);
extern unsigned lcm(unsigned m, unsigned n);
extern unsigned int tca_ckc_setperi(unsigned int periname,unsigned int isenable, unsigned int freq);
extern void wait_phy_reset_timing (unsigned int count);

#endif /* _SYS_PROTO_H_ */

