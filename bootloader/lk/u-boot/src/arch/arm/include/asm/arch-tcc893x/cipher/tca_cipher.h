#ifndef __CIPHER_H__
#define __CIPHER_H__

#define TDEF    0

void tca_test(void);
void tca_cipher_debug(unsigned int debug_number);
void tca_cipher_dma_enable(unsigned uEnable, unsigned uEndian, unsigned uAddrModeTx, unsigned uAddrModeRx);
void tca_cipher_dma_enable_request(unsigned uEnable);
void tca_cipher_interrupt_config(unsigned uTxSel, unsigned uDoneIrq, unsigned uPacketIrq);
void tca_cipher_interrupt_enable(unsigned uEnable);
#if 0 //cjung : compile error
irqreturn_t tca_cipher_interrupt_handler(int irq, void *dev_id);
#endif
extern void tca_cipher_set_dmamode(unsigned uDmaMode);
extern void tca_cipher_set_opmode(unsigned uOpMode, unsigned uDmaMode);
extern void tca_cipher_set_algorithm(unsigned uAlgorithm, unsigned uArg1, unsigned uArg2);
extern void tca_cipher_set_baseaddr(unsigned uTxRx, unsigned char *pBaseAddr);
extern void tca_cipher_set_packet(unsigned uCount, unsigned uSize);
extern void tca_cipher_set_key(unsigned char *pucData, unsigned uLength, unsigned uOption);
extern void tca_cipher_set_vector(unsigned char *pucData, unsigned uLength);
extern int tca_cipher_get_packetcount(unsigned uTxRx);
extern int tca_cipher_get_blocknum(void);
extern void tca_cipher_clear_counter(unsigned uIndex);
extern void tca_cipher_wait_done(void);
extern int tca_cipher_encrypt(unsigned char *pucSrcAddr, unsigned char *pucDstAddr, unsigned uLength);
extern int tca_cipher_decrypt(unsigned char *pucSrcAddr, unsigned char *pucDstAddr, unsigned uLength);
extern int tca_cipher_open(void);
extern int tca_cipher_release(void);

#endif //__CIPHER_H__
