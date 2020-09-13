#include <asm/arch/cipher/cipher.h>
#include <asm/arch/cipher/tca_cipher.h>

int tcc_cipher_set_algorithm(stCIPHER_ALGORITHM *stAlgorithm)
{
	tca_cipher_set_opmode(stAlgorithm->uOperationMode, stAlgorithm->uDmaMode);
	tca_cipher_set_algorithm(stAlgorithm->uAlgorithm, stAlgorithm->uArgument1, stAlgorithm->uArgument2);

	return 0;
}

int tcc_cipher_set_key(stCIPHER_KEY *stKeyInfo)
{
	tca_cipher_set_key(stKeyInfo->pucData, stKeyInfo->uLength, stKeyInfo->uOption);

	return 0;
}

int tcc_cipher_set_vector(stCIPHER_VECTOR *stVectorInfo)
{
	tca_cipher_set_vector(stVectorInfo->pucData, stVectorInfo->uLength);

	return 0;
}

int tcc_cipher_encrypt(stCIPHER_ENCRYPTION *stEncryptInfo)
{
	tca_cipher_encrypt(stEncryptInfo->pucSrcAddr, stEncryptInfo->pucDstAddr, stEncryptInfo->uLength);

	return 0;
}

int tcc_cipher_decrypt(stCIPHER_DECRYPTION *stDecryptInfo)
{
	tca_cipher_decrypt(stDecryptInfo->pucSrcAddr, stDecryptInfo->pucDstAddr, stDecryptInfo->uLength);

	return 0;
}

int tcc_cipher_open(void)
{
	tca_cipher_open();

	return 0;
}

int tcc_cipher_release(void)
{
	tca_cipher_release();

	return 0;
}
