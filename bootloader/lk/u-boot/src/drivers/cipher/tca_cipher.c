#include <common.h>
#include <malloc.h>
#include <stdio_dev.h>
#include <asm/arch/reg_physical.h>
#include <asm/arch/structures_hsio.h>
#include <asm/arch/structures_smu_pmu.h>
#include <asm/arch/cipher/tca_cipher.h>
#include <asm/arch/cipher/cipher_reg.h>

#include <asm/dma-mapping.h>

#define MAX_CIPHER_BUFFER_LENGTH	4096
#define MIN_CIPHER_BLOCK_SIZE		8

static int iDoneIrqHandled = FALSE;

static dma_addr_t SrcDma;	/* physical */
static u_char *pSrcCpu; 	/* virtual */
static dma_addr_t DstDma;	/* physical */
static u_char *pDstCpu; 	/* virtual */

static int CIPHER_DMA_TYPE = -1;
static unsigned char key_swapbuf[128];
static unsigned char iv_swapbuf[128];

#if !defined(CONFIG_ARCH_TCC92XX)
extern struct tcc_freq_table_t gtHSIOClockLimitTable;
#endif

void tca_test(void)
{
	printf("========= B O O T L O A D E R C I P H E R ================\n");

	return ;
}

static void tca_cipher_data_swap(unsigned char *srcdata, unsigned char *destdata, int size)
{
	unsigned int		i;
	for(i=0;i<=size; i+=4)
	{
		destdata[i] = srcdata[i+3];
		destdata[i+1] = srcdata[i+2];
		destdata[i+2] = srcdata[i+1];
		destdata[i+3] = srcdata[i];
	}
}

#if 1 //CJUNG
void tca_cipher_debug(unsigned int debug_number)
{
	PCIPHER pHwCIPHER = (volatile PCIPHER)HwCIPHER_BASE;
	PDMAX	pHwDMAX = (volatile PDMAX)HwDMAX_BASE;

	int i;
	int not_use;
	unsigned int *pDataAddr;

	switch(debug_number)
	{
		case 1: //cipher
			{
				pDataAddr = (unsigned int *)pHwCIPHER;
				printf("\n[ Cipher Register Setting ]\n");
				for(i=0 ; i<2*4 ; i+=4)
				{
					if(i == 0)
						printf("			CTRL	   TXBASE	  RXBASE	 PACKET\n");
					else if(i == 4)
						printf("			DMACTR	   DMASTR	  IRQCTR	 BLKNUL\n");

					printf("0x%p: 0x%08x 0x%08x 0x%08x 0x%08x\n", 
							pDataAddr + i, *(pDataAddr+i+0), *(pDataAddr+i+1), 
							*(pDataAddr+i+2), *(pDataAddr+i+3));
						
				}
			}
			break;
		case 2: //DMAX
			{
				pDataAddr = (unsigned int *)pHwDMAX;
				printf("\n[ DMAX Register Setting ]\n");
				for(i=0 ; i<3*4 ; i+=4)
				{
					if(i == 0)
						printf("			CTRL	   SBASE	  DBASE 	 SIZE\n");
					else if(i == 4)
						printf("			FIFO0	   FIFO1	  FIFO2 	 FIFO3\n");
					else if(i == 8)
						printf("			INTMSK	   INTSTS\n");

					printf("0x%p: 0x%08x 0x%08x 0x%08x 0x%08x\n", 
							pDataAddr + i, *(pDataAddr+i+0), *(pDataAddr+i+1), 
							*(pDataAddr+i+2), *(pDataAddr+i+3));
				}
			}
			break;
		default:
			printf("\n[ Wrong Number ]\n");
			break;
	}
}
#endif

void tca_cipher_dma_enable(unsigned uEnable, unsigned uEndian, unsigned uAddrModeTx, unsigned uAddrModeRx)
{
	PCIPHER pHwCIPHER = (volatile PCIPHER)HwCIPHER_BASE;

	if(uEnable)
	{
		/* Set the Byte Endian Mode */
		if(uEndian == TCC_CIPHER_DMA_ENDIAN_LITTLE)
			BITCLR(pHwCIPHER->DMACTR.nREG, HwCIPHER_DMACTR_ByteEndian);
		else
			BITSET(pHwCIPHER->DMACTR.nREG, HwCIPHER_DMACTR_ByteEndian);

		/* Set the Addressing Mode */
		BITCSET(pHwCIPHER->DMACTR.nREG, HwCIPHER_DMACTR_AddrModeTx_Mask, HwCIPHER_DMACTR_AddrModeTx(uAddrModeTx));
		BITCSET(pHwCIPHER->DMACTR.nREG, HwCIPHER_DMACTR_AddrModeRx_Mask, HwCIPHER_DMACTR_AddrModeRx(uAddrModeRx));

		/* DMA Enable */
		BITSET(pHwCIPHER->DMACTR.nREG, HwCIPHER_DMACTR_Enable);

		/* Clear Done Interrupt Flag */
		iDoneIrqHandled = 0;
	}
	else
	{
		/* DMA Disable */
		BITCLR(pHwCIPHER->DMACTR.nREG, HwCIPHER_DMACTR_Enable);
	}
}

void tca_cipher_dma_enable_request(unsigned uEnable)
{
	PCIPHER pHwCIPHER = (volatile PCIPHER)HwCIPHER_BASE;

	/* Clear TX/RX packet counter */
	BITSET(pHwCIPHER->DMACTR.nREG, HwCIPHER_DMACTR_ClearPacketCount);

	/* DMA Request Enable/Disable */
	if(uEnable)
		BITSET(pHwCIPHER->DMACTR.nREG, HwCIPHER_DMACTR_RequestEnable);
	else
		BITCLR(pHwCIPHER->DMACTR.nREG, HwCIPHER_DMACTR_RequestEnable);
}

void tca_cipher_interrupt_config(unsigned uTxSel, unsigned uDoneIrq, unsigned uPacketIrq)
{
	PCIPHER pHwCIPHER = (volatile PCIPHER)HwCIPHER_BASE;

	/* IRQ Select Direction */
	if(uTxSel)
		BITCLR(pHwCIPHER->IRQCTR.nREG, HwCIPHER_IRQCTR_SelectIrq);
	else
		BITSET(pHwCIPHER->IRQCTR.nREG, HwCIPHER_IRQCTR_SelectIrq);

	/* Enable for "Done" Interrupt */
	if(uDoneIrq)
		BITSET(pHwCIPHER->IRQCTR.nREG, HwCIPHER_IRQCTR_EnableDoneIrq);
	else
		BITCLR(pHwCIPHER->IRQCTR.nREG, HwCIPHER_IRQCTR_EnableDoneIrq);

	/* Enable for "Packet" Interrupt */
	if(uPacketIrq)
		BITSET(pHwCIPHER->IRQCTR.nREG, HwCIPHER_IRQCTR_EnablePacketIrq);
	else
		BITCLR(pHwCIPHER->IRQCTR.nREG, HwCIPHER_IRQCTR_EnablePacketIrq);
}

void tca_cipher_interrupt_enable(unsigned uEnable)
{
	PPIC pHwPIC = (volatile PPIC)HwPIC_BASE;
	//PCIPHER pHwCIPHER = (volatile PCIPHER)HwCIPHER_BASE;

	if(uEnable)
	{
		if(CIPHER_DMA_TYPE == TCC_CIPHER_DMA_DMAX)
		{
			/* DMAX Enable Start*/
			BITSET(pHwPIC->CLR0.nREG, Hw30);
			BITSET(pHwPIC->SEL0.nREG, Hw30);
			BITCLR(pHwPIC->POL0.nREG, Hw30);
			BITCLR(pHwPIC->MODE0.nREG, Hw30);
			BITSET(pHwPIC->INTMSK0.nREG, Hw30);
			BITSET(pHwPIC->SYNC0.nREG, Hw30);
			BITCLR(pHwPIC->MODEA0.nREG, Hw30);
			BITSET(pHwPIC->IEN0.nREG, Hw30);
			/* Cipher Block Enable End*/
		}
		else
		{
			/* Cipher Block Enable Start*/
			BITSET(pHwPIC->CLR1.nREG, Hw20);
			BITSET(pHwPIC->SEL1.nREG, Hw20);
			BITCLR(pHwPIC->POL1.nREG, Hw20);
			BITCLR(pHwPIC->MODE1.nREG, Hw20);
			BITSET(pHwPIC->IEN1.nREG, Hw20);
			/* Cipher Block Enable End*/
		}
	}
	else
	{
		if(CIPHER_DMA_TYPE == TCC_CIPHER_DMA_DMAX)
		{
			BITCLR(pHwPIC->INTMSK0.nREG, Hw30);
			BITCLR(pHwPIC->IEN0.nREG, Hw30);
		}
		else
		{
			BITCLR(pHwPIC->INTMSK1.nREG, Hw20);
			BITCLR(pHwPIC->IEN1.nREG, Hw20);
		}
	}
}

void tca_cipher_set_dmamode(unsigned uDmaMode)
{
	CIPHER_DMA_TYPE = uDmaMode;


	/* Enable Cipher Interrupt */
	tca_cipher_interrupt_enable(FALSE);
}

void tca_cipher_set_opmode(unsigned uOpMode, unsigned uDmaMode)
{
	PCIPHER pHwCIPHER = (volatile PCIPHER)HwCIPHER_BASE;

	switch(uOpMode)
	{
		case TCC_CIPHER_OPMODE_ECB:
			BITCSET(pHwCIPHER->CTRL.nREG, HwCIPHER_CTRL_OperationMode_Mask, HwCIPHER_CTRL_OperationMode_ECB);
			break;
		case TCC_CIPHER_OPMODE_CBC:
			BITCSET(pHwCIPHER->CTRL.nREG, HwCIPHER_CTRL_OperationMode_Mask, HwCIPHER_CTRL_OperationMode_CBC);
			break;
		case TCC_CIPHER_OPMODE_CFB:
			BITCSET(pHwCIPHER->CTRL.nREG, HwCIPHER_CTRL_OperationMode_Mask, HwCIPHER_CTRL_OperationMode_CFB);
			break;
		case TCC_CIPHER_OPMODE_OFB:
			BITCSET(pHwCIPHER->CTRL.nREG, HwCIPHER_CTRL_OperationMode_Mask, HwCIPHER_CTRL_OperationMode_OFB);
			break;
		case TCC_CIPHER_OPMODE_CTR:
		case TCC_CIPHER_OPMODE_CTR_1:
		case TCC_CIPHER_OPMODE_CTR_2:
		case TCC_CIPHER_OPMODE_CTR_3:
			BITCSET(pHwCIPHER->CTRL.nREG, HwCIPHER_CTRL_OperationMode_Mask, HwCIPHER_CTRL_OperationMode_CTR);
			break;
		default:
			printf("%s, Err: Invalid Operation Mode\n", __func__);
			break;
	}
	tca_cipher_set_dmamode(uDmaMode);
}

void tca_cipher_set_algorithm(unsigned uAlgorithm, unsigned uArg1, unsigned uArg2)
{
	PCIPHER pHwCIPHER = (volatile PCIPHER)HwCIPHER_BASE;

	switch(uAlgorithm)
	{
		case TCC_CIPHER_ALGORITHM_AES:
		{
			/* uArg1: The Key Length in AES */
			/* uArg2: None					*/
			BITCSET(pHwCIPHER->CTRL.nREG, HwCIPHER_CTRL_Algorithm_Mask, HwCIPHER_CTRL_Algorithm_AES);
			BITCSET(pHwCIPHER->CTRL.nREG, HwCIPHER_CTRL_KeyLength_Mask, HwCIPHER_CTRL_KeyLength(uArg1));
		}
		break;

		case TCC_CIPHER_ALGORITHM_DES:
		{
			/* uArg1: The Mode in DES	  */
			/* uArg2: Parity Bit Location */
			BITCSET(pHwCIPHER->CTRL.nREG, HwCIPHER_CTRL_Algorithm_Mask, HwCIPHER_CTRL_Algorithm_DES);
			BITCSET(pHwCIPHER->CTRL.nREG, HwCIPHER_CTRL_DESMode_Mask, HwCIPHER_CTRL_DESMode(uArg1));
			if(uArg2)
				BITSET(pHwCIPHER->CTRL.nREG, HwCIPHER_CTRL_ParityBit);
			else
				BITCLR(pHwCIPHER->CTRL.nREG, HwCIPHER_CTRL_ParityBit);
		}
		break;

		default:
			printf("%s, Err: Invalid Algorithm\n", __func__);
			break;
	}
}

void tca_cipher_set_baseaddr(unsigned uTxRx, unsigned char *pBaseAddr)
{
	PCIPHER pHwCIPHER = (volatile PCIPHER)HwCIPHER_BASE;

	if(uTxRx == TCC_CIPHER_BASEADDR_TX)
		pHwCIPHER->TXBASE.nREG = (unsigned long)pBaseAddr;
	else
		pHwCIPHER->RXBASE.nREG = (unsigned long)pBaseAddr;
}

void tca_cipher_set_packet(unsigned uCount, unsigned uSize)
{
	PCIPHER pHwCIPHER = (volatile PCIPHER)HwCIPHER_BASE;

	if(uCount)
		uCount -= 1;

	pHwCIPHER->PACKET.nREG = ((uCount & 0xFFFF) << 16) | (uSize & 0xFFFF);
}

void tca_cipher_set_key(unsigned char *pucData, unsigned uLength, unsigned uOption)
{
	PCIPHER pHwCIPHER = (volatile PCIPHER)HwCIPHER_BASE;
	unsigned long ulAlgorthm, ulAESKeyLength, ulDESMode;
	unsigned long *pulKeyData;
	unsigned char *ucp;
	int cnt;

	ulAlgorthm = pHwCIPHER->CTRL.nREG & HwCIPHER_CTRL_Algorithm_Mask;

	tca_cipher_data_swap(pucData, key_swapbuf, uLength);

	pulKeyData = (unsigned long *)key_swapbuf;

	ucp = (unsigned char *)pulKeyData;
	
	/* Write Key Data */
	switch(ulAlgorthm)
	{
		case HwCIPHER_CTRL_Algorithm_AES:
		{
			ulAESKeyLength = pHwCIPHER->CTRL.nREG & HwCIPHER_CTRL_KeyLength_Mask;

			if(ulAESKeyLength == HwCIPHER_CTRL_keyLength_128)
			{
				pHwCIPHER->KEY0.nREG = *pulKeyData++;
				pHwCIPHER->KEY1.nREG = *pulKeyData++;
				pHwCIPHER->KEY2.nREG = *pulKeyData++;
				pHwCIPHER->KEY3.nREG = *pulKeyData++;
			}
			else if(ulAESKeyLength == HwCIPHER_CTRL_KeyLength_192)
			{
				pHwCIPHER->KEY0.nREG = *pulKeyData++;
				pHwCIPHER->KEY1.nREG = *pulKeyData++;
				pHwCIPHER->KEY2.nREG = *pulKeyData++;
				pHwCIPHER->KEY3.nREG = *pulKeyData++;
				pHwCIPHER->KEY4.nREG = *pulKeyData++;
				pHwCIPHER->KEY5.nREG = *pulKeyData++;
			}
			else if(ulAESKeyLength == HwCIPHER_CTRL_KeyLength_256)
			{
				pHwCIPHER->KEY0.nREG = *pulKeyData++;
				pHwCIPHER->KEY1.nREG = *pulKeyData++;
				pHwCIPHER->KEY2.nREG = *pulKeyData++;
				pHwCIPHER->KEY3.nREG = *pulKeyData++;
				pHwCIPHER->KEY4.nREG = *pulKeyData++;
				pHwCIPHER->KEY5.nREG = *pulKeyData++;
				pHwCIPHER->KEY6.nREG = *pulKeyData++;
				pHwCIPHER->KEY7.nREG = *pulKeyData++;
			}
		}
		break;

		case HwCIPHER_CTRL_Algorithm_DES:
		{
			ulDESMode = pHwCIPHER->CTRL.nREG & HwCIPHER_CTRL_DESMode_Mask;

			if(ulDESMode == HwCIPHER_CTRL_DESMode_SingleDES)
			{
				pHwCIPHER->KEY0.nREG = *pulKeyData++;
				pHwCIPHER->KEY1.nREG = *pulKeyData++;
			}
			else if(ulDESMode == HwCIPHER_CTRL_DESMode_DoubleDES)
			{
				pHwCIPHER->KEY0.nREG = *pulKeyData++;
				pHwCIPHER->KEY1.nREG = *pulKeyData++;
				pHwCIPHER->KEY2.nREG = *pulKeyData++;
				pHwCIPHER->KEY3.nREG = *pulKeyData++;
			}
			else if(ulDESMode == HwCIPHER_CTRL_DESMode_TripleDES2)
			{
				pHwCIPHER->KEY0.nREG = *pulKeyData++;
				pHwCIPHER->KEY1.nREG = *pulKeyData++;
				pHwCIPHER->KEY2.nREG = *pulKeyData++;
				pHwCIPHER->KEY3.nREG = *pulKeyData++;
			}
			else if(ulDESMode == HwCIPHER_CTRL_DESMode_TripleDES3)
			{
				pHwCIPHER->KEY0.nREG = *pulKeyData++;
				pHwCIPHER->KEY1.nREG = *pulKeyData++;
				pHwCIPHER->KEY2.nREG = *pulKeyData++;
				pHwCIPHER->KEY3.nREG = *pulKeyData++;
				pHwCIPHER->KEY4.nREG = *pulKeyData++;
				pHwCIPHER->KEY5.nREG = *pulKeyData++;
			}
		}
		break;

		default:
			break;
	}

	/* Load Key Data */
	BITSET(pHwCIPHER->CTRL.nREG, HwCIPHER_CTRL_KeyDataLoad);
}

void tca_cipher_set_vector(unsigned char *pucData, unsigned uLength)
{
	PCIPHER pHwCIPHER = (volatile PCIPHER)HwCIPHER_BASE;
	unsigned long ulAlgorthm;
	unsigned long *pulVectorData;
	unsigned char *ucp;
	int cnt;

	ulAlgorthm = pHwCIPHER->CTRL.nREG & HwCIPHER_CTRL_Algorithm_Mask;

	if(CIPHER_DMA_TYPE == TCC_CIPHER_DMA_INTERNAL)
	{
		tca_cipher_data_swap(pucData, iv_swapbuf, uLength);
		pulVectorData = (unsigned long *)iv_swapbuf;
	}
	else
	{
		pulVectorData = (unsigned long *)pucData;
	}
	ucp = (unsigned char *)pulVectorData;

	/* Write Initial Vector */
	switch(ulAlgorthm)
	{
		case HwCIPHER_CTRL_Algorithm_AES:
		{
			pHwCIPHER->IV0.nREG = *pulVectorData++;
			pHwCIPHER->IV1.nREG = *pulVectorData++;
			pHwCIPHER->IV2.nREG = *pulVectorData++;
			pHwCIPHER->IV3.nREG = *pulVectorData++;
		}
		break;

		case HwCIPHER_CTRL_Algorithm_DES:
		{
			pHwCIPHER->IV0.nREG = *pulVectorData++;
			pHwCIPHER->IV1.nREG = *pulVectorData++;
		}
		break;

		default:
			break;
	}

	/* Load Initial Vector */
	BITSET(pHwCIPHER->CTRL.nREG, HwCIPHER_CTRL_InitVectorLoad);
}

int tca_cipher_get_packetcount(unsigned uTxRx)
{
	PCIPHER pHwCIPHER = (volatile PCIPHER)HwCIPHER_BASE;
	int iPacketCount;

	if(uTxRx == TCC_CIPHER_PACKETCOUNT_TX)
		iPacketCount = (pHwCIPHER->DMASTR.nREG & 0x00FF);
	else
		iPacketCount = (pHwCIPHER->DMASTR.nREG & 0xFF00) >> 16;

	return iPacketCount;
}

int tca_cipher_get_blocknum(void)
{
	PCIPHER pHwCIPHER = (volatile PCIPHER)HwCIPHER_BASE;

	return pHwCIPHER->BLKNUM.nREG;
}

void tca_cipher_clear_counter(unsigned uIndex)
{
	PCIPHER pHwCIPHER = (volatile PCIPHER)HwCIPHER_BASE;

	/* Clear Transmit FIFO Counter */
	if(uIndex & TCC_CIPHER_CLEARCOUNTER_TX)
		BITSET(pHwCIPHER->CTRL.nREG, HwCIPHER_CTRL_ClearTxFIFO);
	/* Clear Receive FIFO Counter */
	if(uIndex & TCC_CIPHER_CLEARCOUNTER_RX)
		BITSET(pHwCIPHER->CTRL.nREG, HwCIPHER_CTRL_ClearRxFIFO);
	/* Clear Block Counter */
	if(uIndex & TCC_CIPHER_CLEARCOUNTER_BLOCK)
		BITSET(pHwCIPHER->CTRL.nREG, HwCIPHER_CTRL_ClearBlkCount);
}

void tca_cipher_wait_done(void)
{
	PCIPHER pHwCIPHER = (volatile PCIPHER)HwCIPHER_BASE;

	/* Wait for Done Interrupt */
	while(1)
	{
		if(iDoneIrqHandled)
		{
			printf("Receive Done IRQ Handled\n");
			/* Clear Done IRQ Flag */
			iDoneIrqHandled = FALSE;
			break;
		}

		//msleep(1);	//warning
	}

	/* Wait for DMA to be Disabled */
	while(1)
	{
		if(!(pHwCIPHER->DMACTR.nREG & HwCIPHER_DMACTR_Enable))
		{
			printf("DMA Disabled\n");
			break;
		}

		//msleep(1);	//warning
	}
}

int tca_cipher_encrypt(unsigned char *pucSrcAddr, unsigned char *pucDstAddr, unsigned uLength)
{
	PCIPHER pHwCIPHER = (volatile PCIPHER)HwCIPHER_BASE;
	PDMAX	pHwDMAX = (volatile PDMAX)HwDMAX_BASE;
	int cnt;

	memset(pSrcCpu, 0, uLength);
	memset(pDstCpu, 0, uLength);

	memcpy(pSrcCpu, pucSrcAddr, uLength);

	/* Clear All Conunters */
	tca_cipher_clear_counter(TCC_CIPHER_CLEARCOUNTER_ALL);

	/* Select Encryption */
	BITSET(pHwCIPHER->CTRL.nREG, HwCIPHER_CTRL_Encrytion);

	/* Load Key & InitVector Value */
	BITSET(pHwCIPHER->CTRL.nREG, HwCIPHER_CTRL_KeyDataLoad);
	BITSET(pHwCIPHER->CTRL.nREG, HwCIPHER_CTRL_InitVectorLoad);

	if(CIPHER_DMA_TYPE == TCC_CIPHER_DMA_DMAX)
	{
		pHwDMAX->SBASE.nREG = pSrcCpu;
		pHwDMAX->DBASE.nREG = pDstCpu;

		pHwDMAX->SIZE.bREG.SIZE = uLength;
		pHwDMAX->CTRL.bREG.CH	= 0;
		pHwDMAX->CTRL.bREG.TYPE = 0;

		pHwCIPHER->ROUND.bREG.ROUND = 0x33;

		pHwDMAX->CTRL.bREG.EN = 1;

		tca_cipher_debug(1);
		tca_cipher_debug(2);

		while(!pHwDMAX->FIFO0.bREG.IDLE);

		pHwDMAX->INTSTS = pHwDMAX->INTSTS;
	}
	else		/*(CIPHER_DMA_TYPE == TCC_CIPHER_DMA_INTERNAL)*/
	{
		/* Set the Base Address */
		pHwCIPHER->TXBASE.nREG = pSrcCpu;
		pHwCIPHER->RXBASE.nREG = pDstCpu;

		tca_cipher_set_packet(1, uLength);
		/* Configure Interrupt - Receiving */
		tca_cipher_interrupt_config(FALSE, TRUE, FALSE);
		/* Request Enable DMA */
		tca_cipher_dma_enable_request(TRUE);
		/* Clear Packet Counter */
		BITSET(pHwCIPHER->DMACTR.nREG, HwCIPHER_DMACTR_ClearPacketCount);

		/* Enable DMA */ 
		if(uLength > MIN_CIPHER_BLOCK_SIZE)
			tca_cipher_dma_enable(TRUE, TCC_CIPHER_DMA_ENDIAN_LITTLE, TCC_CIPHER_DMA_ADDRMODE_MULTI, TCC_CIPHER_DMA_ADDRMODE_MULTI);	
		else
			tca_cipher_dma_enable(TRUE, TCC_CIPHER_DMA_ENDIAN_LITTLE, TCC_CIPHER_DMA_ADDRMODE_SINGLE, TCC_CIPHER_DMA_ADDRMODE_SINGLE);	

		while(0 == ((pHwCIPHER->IRQCTR.nREG) & HwCIPHER_IRQCTR_DoneIrqStatus));
		while(pHwCIPHER->DMACTR.nREG & HwCIPHER_DMACTR_Enable);

		tca_cipher_dma_enable_request(FALSE);
		tca_cipher_dma_enable(FALSE, 0, 0, 0);
	}

	memcpy(pucDstAddr, pDstCpu, uLength);

	return 0;
}

int tca_cipher_decrypt(unsigned char *pucSrcAddr, unsigned char *pucDstAddr, unsigned uLength)
{
	PCIPHER pHwCIPHER = (volatile PCIPHER)HwCIPHER_BASE;
	PDMAX	pHwDMAX = (volatile PDMAX)HwDMAX_BASE;

	memset(pSrcCpu, 0, uLength);
	memset(pDstCpu, 0, uLength);

	memcpy(pSrcCpu, pucSrcAddr, uLength);

	/* Clear All Conunters */
	tca_cipher_clear_counter(TCC_CIPHER_CLEARCOUNTER_ALL);

	/* Select Decryption */
	BITCLR(pHwCIPHER->CTRL.nREG, HwCIPHER_CTRL_Encrytion);

	/* Load Key & InitVector Value */
	BITSET(pHwCIPHER->CTRL.nREG, HwCIPHER_CTRL_KeyDataLoad);
	BITSET(pHwCIPHER->CTRL.nREG, HwCIPHER_CTRL_InitVectorLoad);

	if(CIPHER_DMA_TYPE == TCC_CIPHER_DMA_DMAX)
	{
		pHwDMAX->SBASE.nREG = pSrcCpu;
		pHwDMAX->DBASE.nREG = pDstCpu;

		pHwDMAX->SIZE.bREG.SIZE = uLength;
		pHwDMAX->CTRL.bREG.CH	= 0;
		pHwDMAX->CTRL.bREG.TYPE = 0;

		pHwCIPHER->ROUND.bREG.ROUND = 0x33;

		pHwDMAX->CTRL.bREG.EN = 1;

		while(!pHwDMAX->FIFO0.bREG.IDLE) ;

		pHwDMAX->INTSTS = pHwDMAX->INTSTS;
	}
	else		/*(CIPHER_DMA_TYPE == TCC_CIPHER_DMA_INTRRENAL)*/
	{
		/* Set the Base Address */
		pHwCIPHER->TXBASE.nREG = pSrcCpu;
		pHwCIPHER->RXBASE.nREG = pDstCpu;

		/* Set the Packet Information */
		tca_cipher_set_packet(1, uLength);
		/* Configure Interrupt - Receiving */
		tca_cipher_interrupt_config(FALSE, TRUE, FALSE);
		/* Request Enable DMA */
		tca_cipher_dma_enable_request(TRUE);
		/* Clear Packet Counter */
		BITSET(pHwCIPHER->DMACTR.nREG, HwCIPHER_DMACTR_ClearPacketCount);

		/* Enable DMA */ 
		if(uLength > MIN_CIPHER_BLOCK_SIZE)
			tca_cipher_dma_enable(TRUE, TCC_CIPHER_DMA_ENDIAN_LITTLE, TCC_CIPHER_DMA_ADDRMODE_MULTI, TCC_CIPHER_DMA_ADDRMODE_MULTI);	
		else
			tca_cipher_dma_enable(TRUE, TCC_CIPHER_DMA_ENDIAN_LITTLE, TCC_CIPHER_DMA_ADDRMODE_SINGLE, TCC_CIPHER_DMA_ADDRMODE_SINGLE);	

		while(0 == ((pHwCIPHER->IRQCTR.nREG) & HwCIPHER_IRQCTR_DoneIrqStatus));
		while(pHwCIPHER->DMACTR.nREG & HwCIPHER_DMACTR_Enable); 

		tca_cipher_dma_enable_request(FALSE);
		tca_cipher_dma_enable(FALSE, 0, 0, 0);	
	}

	memcpy(pucDstAddr, pDstCpu, uLength);

	return 0;
}

volatile PCKC pCKC = (PCKC)HwCKC_BASE;

int tca_cipher_open(void)
{
	volatile CLKCTRL_TYPE *pCLKCTRL;
	int err = 0;

	pCLKCTRL = (volatile CLKCTRL_TYPE *)(&pCKC->CLKCTRL7);

	 /* HSIO clock enable */
	pCLKCTRL->bREG.EN = 1;

	pSrcCpu = dma_alloc_coherent(MAX_CIPHER_BUFFER_LENGTH, &pSrcCpu);
	pDstCpu = dma_alloc_coherent(MAX_CIPHER_BUFFER_LENGTH, &pDstCpu);

	if((pSrcCpu == NULL) || (pDstCpu == NULL)) {
		err = -1;
	}

	return err;
}

int tca_cipher_release(void)
{
	volatile CLKCTRL_TYPE *pCLKCTRL;
	pCLKCTRL = (volatile CLKCTRL_TYPE *)(&pCKC->CLKCTRL7);

	//pCLKCTRL->bREG.EN = 0;

	return 0;
}

