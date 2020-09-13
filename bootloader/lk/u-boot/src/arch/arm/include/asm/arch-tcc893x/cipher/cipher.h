#ifndef __CIPHER_H__
#define __CIPHER_H__

#include <asm/arch/cipher/cipher_reg.h>

// encrypt binary
#define ENC_PT_DAT			"encrypt_partition.dat"
#define ENC_LK_ROM			"encrypt_lk.rom"
#define ENC_BOOT_IMG		"encrypt_boot.img"
#define ENC_SYSTEM_IMG		"encrypt_system.img"
#define ENC_PSYSTEM_IMG 	"encrypt_psystem.img"
#define ENC_RECOVERY_IMG	"encrypt_recovery.img"
#define ENC_SPLASH_IMG		"encrypt_splash.img"
#define ENC_SNAPSHOT_IMG	"encrypt_empty_snapshot.img"
#define ENC_QB_DATA_IMG 	"encrypt_qb_data.img"

// AES define
#define KEY 		"1234567890123456"
#define VEC 		"aaaaaaaaaaaaaaaa"

#define AES_KEY_LEN 	16
#define AES_IV_LEN		16
#define AES_BLOCK_SIZE	4096

#define SPARSE_BLOCK_SIZE	4096

#define ACTION_SPARSE_PARSING	1
#define ACTION_CHUNK_PARSING	2
#define ACTION_GET_DATA 		3

#define WRITE_ENABLE			1
#define WRITE_DISABLE			0

// sparse define
#define CHUNK_TYPE_RAW			0xCAC1
#define CHUNK_TYPE_FILL 		0xCAC2
#define CHUNK_TYPE_DONT_CARE	0xCAC3
#define CHUNK_TYPE_CRC32		0xCAC4

typedef struct _SH {
	unsigned int	magic;			/* 0xed26ff3a file magic  */
	unsigned short	major_version;	/* (0x1) - reject images with higher major versions */
	unsigned short	minor_version;	/* (0x0) - allow images with higer minor versions */
	unsigned short	file_hdr_sz;	/* 28 bytes for first revision of the file format */
	unsigned short	chunk_hdr_sz;	/* 12 bytes for first revision of the file format */
	unsigned int	blk_sz; 		/* block size in bytes, must be a multiple of 4 (4096) */
	unsigned int	total_blks; 	/* total blocks in the non-sparse output image */
	unsigned int	total_chunks;	/* total chunks in the sparse input image */
	unsigned int	image_checksum; /* CRC32 checksum of the original data, counting "don't care" */
} SH;

typedef struct _CH {
	unsigned short	chunk_type; /* 0xCAC1 -> raw; 0xCAC2 -> fill; 0xCAC3 -> don't care */
	unsigned short	reserved1;	/* not use */
	unsigned int	chunk_sz;	/* data count (total data = chunk_hdr_sz * chunk_sz */
	unsigned int	total_sz;	/* head + total data */
} CH;

int tcc_cipher_set_algorithm(stCIPHER_ALGORITHM *stAlgorithm);
int tcc_cipher_set_key(stCIPHER_KEY *stKeyInfo);
int tcc_cipher_set_vector(stCIPHER_VECTOR *stVectorInfo);
int tcc_cipher_encrypt(stCIPHER_ENCRYPTION *stEncryptInfo);
int tcc_cipher_decrypt(stCIPHER_DECRYPTION *stDecryptInfo);
int tcc_cipher_open(void);
int tcc_cipher_release(void);

#endif //__CIPHER_H__
