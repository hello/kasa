/*
 * test_crypto.c
 *
 * History:
 *	2011/05/04 - [Jian Tang] created file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>

#include <sys/stat.h>
#include <sys/ioctl.h>

#include <basetypes.h>
#include <amba_crypto.h>


#define NO_ARG				(0)
#define HAS_ARG				(1)

#define PAGE_SIZE			(4 * 1024)
#define FILENAME_LENGTH	(256)
#define KEY_LENGTH			(32)
#define TEXT_LENGTH			(32)
#define BLOCK_SIZE			(16)

enum {
	CRYPTO_NONE = -1,
	CRYPTO_ENCRYPTE = 0,
	CRYPTO_DECRYPTE,
	CRYPTO_ENC_DEC,
	CRYPTO_SHA1,
	CRYPTO_MD5,
} CRYPTO_METHOD;

static int display = 0;

static int fd_crypto = -1;

static int crypto_method = CRYPTO_NONE;

static char filename[FILENAME_LENGTH] = "media/plain.txt";

static char algoname[256] = "aes";
static char default_algo[256] = "ecb(aes)";

static u8 aes_key[KEY_LENGTH] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static u8 manu_key[KEY_LENGTH] = {};

static u8 des_key[8] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static int autotest_flag = 0;
static int manukey = 0;

static struct option long_options[] = {
	{"algo", HAS_ARG, 0, 'a'},
	{"file", HAS_ARG, 0, 'f'},
	{"key", HAS_ARG, 0, 'k'},
	{"autotest", NO_ARG, 0, 'r'},
	{"display",NO_ARG,0,'d'},

	{0, 0, 0, 0}
};

static const char *short_options = "a:b:f:k:rd";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"algoname", "\tset algorithm name ('aes', 'des'), default is 'ecb(aes)'"},
	{"filename", "\tread text from file"},
	{"key-string", "\tdefault key length is 32bytes, if key length is less than 32 bytes, filled all 0xff in the end"},
	{"", "\trun auto-test function"},
	{"", "\tDisplay detail"},
};

static void usage(void)
{
	int i;

	printf("test_crypto usage:\n");
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}
	printf("\nExamples: \n");
	printf("\n  Auto test:\n    test_crypto -r\n");
	printf("\n");
}

static int open_crypto_dev(void)
{
	if ((fd_crypto = open("/dev/ambac", O_RDWR, 0)) < 0) {
		perror("/dev/ambac");
		return -1;
	}
	return 0;
}

static int close_crypto_dev(void)
{
	close(fd_crypto);
	return 0;
}

static int set_plain_text(u8 * arr, int n)
{
	int i;
	for (i = 0; i < n; ++i) {
		arr[i] = 0x30 + i;
	}
	return 0;
}

static int arraycpy(u8 *dst,u8 *src,int len)
{
	int i=0;
	for(i=0;i<len;i++) {
		*(dst+i) = *(src+i);
	}
	return 0;
}

static int set_cipher_cmp_text(u8 *arr,int n,char *algo_name)
{
	u8 cipher_aes[TEXT_LENGTH] = {0x50,0xA0,0xC4,0x34,0x05,0x19,0xCE,0x51,
								 0x89,0x3A,0xF9,0xB1,0x64,0x86,0x9F,0xF3,
								 0x66,0x57,0xFA,0xD5,0x14,0x28,0x4F,0xA7,
								 0x1F,0x0A,0x27,0x01,0xD2,0x39,0xC0,0xEA};
	u8 cipher_des[TEXT_LENGTH] = {
		0x67,0xCB,0x42,0x90,0xC1,0xC8,0x4A,0x52,0x7B,0x9A,0x8F,0xCF,0x7D,0x04,0xD6,
		0x5D,0x9C,0x12,0xD7,0x01,0x52,0x34,0x11,0xA0,0x51,0xE1,0x71,0x70,0xEB,0xE2,
		0xEC,0x58};

	if (strstr(algo_name,"aes") != NULL) {
		arraycpy(arr,cipher_aes,n);
	}else if (strstr(algo_name,"des") != NULL) {
		arraycpy(arr,cipher_des,n);
	}
	return 0;
}

static int print_u8_array(const char * prefix, u8 * arr, int n)
{
	int i;
	if (display == 0) {
		return 0;
	}
	printf("\n%s\n[", prefix);
	for (i = 0; i < n; ++i) {
		printf("0x%02X ", arr[i]);
	}
	printf("]");
	return 0;
}

static int prepare_cipher_algo(const char *algo_name)
{
	amba_crypto_algo_t algo;
	strcpy(algo.cipher_algo, algo_name);
	strcpy(algo.default_algo, default_algo);
	if (ioctl(fd_crypto, AMBA_IOC_CRYPTO_SET_ALGO, &algo) < 0) {
		perror("AMBA_IOC_CRYPTO_SET_ALGO");
		return -1;
	}
	return 0;
}

static int prepare_cipher_key(u8 * cipher_key, u32 key_len)
{
	amba_crypto_key_t key;
	key.key = cipher_key;
	key.key_len = key_len;
	if (ioctl(fd_crypto, AMBA_IOC_CRYPTO_SET_KEY, &key) < 0) {
		perror("AMBA_IOC_CRYPTO_SET_KEY");
		return -1;
	}
	print_u8_array("Cipher Key", cipher_key, key_len);
	return 0;
}

static int cipher_encrypt(u8 * text, u32 total_bytes)
{
	amba_crypto_info_t info;
	info.text = text;
	info.total_bytes = total_bytes;
	if (ioctl(fd_crypto, AMBA_IOC_CRYPTO_ENCRYPTE, &info) < 0) {
		perror("AMBA_IOC_CRYPTO_ENCRYPTE");
		return -1;
	}
	return 0;
}

static int cipher_decrypt(u8 * text, u32 total_bytes)
{
	amba_crypto_info_t info;
	info.text = text;
	info.total_bytes = total_bytes;
	if (ioctl(fd_crypto, AMBA_IOC_CRYPTO_DECRYPTE, &info) < 0) {
		perror("AMBA_IOC_CRYPTO_DECRYPTE");
		return -1;
	}
	return 0;
}

int auto_sha(u8 *in,u8 *out)
{
	u8 i;
	amba_crypto_sha_md5_t info;

	info.in=in;
	info.total_bytes =(u32)strlen((const char*)info.in);
	info.out = out;

	if (ioctl(fd_crypto, AMBA_IOC_CRYPTO_SHA, &info) < 0) {
		perror("AMBA_IOC_CRYPTO_SHA");
		return -1;
	}

	printf("SHA1=");
	for(i=0;i<20;i++){
		printf("%02X",out[i]);
	}
	printf("\r\n");

	return 0;
}

int auto_md5(u8 *in,u8 *out)
{
	u8 i;
	amba_crypto_sha_md5_t info;

	info.in=in;
	info.total_bytes =(u32)strlen((const char*)info.in);
	info.out = out;

	if (ioctl(fd_crypto, AMBA_IOC_CRYPTO_MD5, &info) < 0) {
		perror("AMBA_IOC_CRYPTO_MD5");
		return -1;
	}

	printf("MD5=");
	for(i=0;i<16;i++){
		printf("%02X",out[i]);
	}
	printf("\r\n");

	return 0;
}

//two array should have same length
static int arraycmp(u8 *array1,u8 *array2, int len)
{
	int i;
	for(i=0;i<len;i++) {
		if (array1[i] != array2[i]) {
			return -1;
		}
	}
	return 0;
}

static int prepare_key_byalgo(const char *algo_name,u32 *key_len,u8 **key)
{
	if (strcmp(algo_name,"ecb(aes)") == 0){
		*key=aes_key;
		*key_len = 32;
	}else if (strcmp(algo_name,"ecb(des)") == 0) {
		*key=des_key;
		*key_len = 8;
	}else {
		return -1;
	}
	return 0;

}

static int auto_test(char * algo_name)
{
	u8 plain[TEXT_LENGTH] = {0};
	u8 cipher[TEXT_LENGTH] = {0};
	u8 cipher_cmp[TEXT_LENGTH] = {0};
	u8 temp[TEXT_LENGTH] = {0};
	u32 key_len=0 ;
	u8 *key=NULL;
	u32 text_bytes = sizeof(plain);

	if (prepare_key_byalgo(algo_name,&key_len,&key) < 0){
		printf("prepare key failed !\r\n");
		return -1;
	}

	if (prepare_cipher_algo(algo_name) < 0) {
		printf("prepare_cipher_algo failed !\n");
		return -1;
	}
	printf("\n==algo method : %s\n",algo_name);

	if (prepare_cipher_key(key, key_len) < 0) {
		printf("prepare_cipher_key failed !\n");
		return -1;
	}

	set_plain_text(plain, sizeof(plain) - 4);
	set_cipher_cmp_text(cipher_cmp,sizeof(plain),algo_name);

	memcpy(cipher, plain, sizeof(plain));
	printf("\n== Before encryption ==");
	print_u8_array("Plain Text", plain, sizeof(plain));
	print_u8_array("Cipher Text", cipher, sizeof(cipher));
	if (cipher_encrypt(plain, text_bytes) < 0) {
		printf("cipher_encrypt failed !\n");
		return -1;
	}
	printf("\n== After encrytion ==");
	memcpy(temp, cipher, sizeof(temp));
	memcpy(cipher, plain, sizeof(cipher));
	memcpy(plain, temp, sizeof(plain));
	print_u8_array("Plain Text", plain, sizeof(plain));
	print_u8_array("Cipher Text", cipher, sizeof(cipher));
	if (arraycmp(cipher,cipher_cmp,sizeof(cipher)) == 0) {
		printf("\n****************ecrytion right************************\n");
	}else{
		printf("\n!!!!!!!!!!Err: encrytion wrong ");
		print_u8_array("the right answer should be \n",cipher_cmp,sizeof(cipher_cmp));
	}

	printf("\n== Before decryption ==");
	print_u8_array("Plain Text", plain, sizeof(plain));
	print_u8_array("Cipher Text", cipher, sizeof(cipher));
	if (cipher_decrypt(cipher, text_bytes) < 0) {
		printf("\ncipher_decrypt failed !\n");
		return -1;
	}
	printf("\n== After decrytion ==");
	print_u8_array("Plain Text", plain, sizeof(plain));
	print_u8_array("Cipher Text", cipher, sizeof(cipher));
	if (arraycmp(plain,cipher,sizeof(cipher)) == 0) {
		printf("\n****************decryption right************************\n");
	}else{
		printf("\n!!!!!!!!!!Err: decrypton wrong ");
		print_u8_array("the right answer should be \n",plain,sizeof(plain));
	}

	return 0;
}


static int auto_md5_sha1_test(void)
{
	u8 in[]="The quick brown fox jumps over the lazy dog";
	u8 out[21];
	u8 correctsha[]={0x2f,0xd4,0xe1,0xc6,0x7a,0x2d,0x28,0xfc,0xed,0x84,0x9e,0xe1,0xbb,0x76,0xe7,0x39,0x1b,0x93,0xeb,0x12,'\0'};
	u8 correctmd5[]={0x9e,0x10,0x7d,0x9d,0x37,0x2b,0xb6,0x82,0x6b,0xd8,0x1d,0x35,0x42,0xa4,0x19,0xd6,'\0'};

	printf("\n== TEST SHA1&MD5 ==\n");
	printf("in= %s\n",in);
	if(auto_sha(in,out) != 0){
		printf("SHA1 driver failed!\n");
	}
	out[20]='\0';
	if(strcmp((const char*)out,(const char*)correctsha) == 0){
		printf("SHA1 CORRECT!\n");
	}else{
		printf("SHA1 ERROR; The correct should be 2fd4e1c67a2d28fced849ee1bb76e7391b93eb12\n");
	}

	if(auto_md5(in,out) != 0){
		printf("MD5 driver failed!\n");
	}
	out[16]='\0';
	if(strcmp((const char*)out,(const char*)correctmd5) == 0){
		printf("MD5 CORRECT!\n");
	}else{
		printf("MD5 ERROR; The correct should be 9e107d9d372bb6826bd81d3542a419d6\n");
	}
	return 0;


}

static int get_file_size(int fd)
{
	struct stat stat;
	if (fstat(fd, &stat) < 0) {
		perror("fstat");
		return -1;
	}
	return stat.st_size;
}

static int init_param(int argc, char **argv)
{
	int ch, value;
	int option_index = 0;
	opterr = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'a':
			strcpy(algoname, optarg);
			break;
		case 'f':
			strcpy(filename, optarg);
			break;
		case 'k':
			value = (strlen(optarg) > KEY_LENGTH) ? KEY_LENGTH : strlen(optarg);
			strncpy((char *)manu_key, optarg, value);
			manukey = 1;
			break;
		case 'r':
			autotest_flag = 1;
			crypto_method = CRYPTO_ENC_DEC;
			break;
		case 'd':
			display = 1;
			break;
		default:
			printf("unknown command %s \n", optarg);
			usage();
			return -1;
			break;
		}
	}

	return 0;
}

int sha()
{
	int plain;
	u32 file_size;
	u8 in[PAGE_SIZE];
	u8 out[20];
	u8 i;
	amba_crypto_sha_md5_t info;
	if ((plain = open(filename,O_RDONLY,0)) < 0){
		printf("open file failed\r\n");
	}

	file_size = get_file_size(plain);
	read(plain,in,PAGE_SIZE);

	info.in=in;
	info.total_bytes = file_size;
	info.out = out;

	if (ioctl(fd_crypto, AMBA_IOC_CRYPTO_SHA, &info) < 0) {
		perror("AMBA_IOC_CRYPTO_SHA");
		return -1;
	}
	close(plain);

	printf("file size is %d\r\n",file_size);
	printf("SHA1=");
	for(i=0;i<20;i++){
		printf("%02X",out[i]);
	}
	printf("\r\n");

	return 0;
}

int md5()
{
	int plain;
	u32 file_size;
	u8 in[PAGE_SIZE];
	u8 out[16];
	u8 i;
	amba_crypto_sha_md5_t info;
	if ((plain = open(filename,O_RDONLY,0)) < 0){
		printf("open file failed\r\n");
	}

	file_size = get_file_size(plain);
	read(plain,in,PAGE_SIZE);

	info.in=in;
	info.total_bytes = file_size;
	info.out = out;

	if (ioctl(fd_crypto, AMBA_IOC_CRYPTO_MD5, &info) < 0) {
		perror("AMBA_IOC_CRYPTO_MD5");
		return -1;
	}
	close(plain);

	printf("file size is %d\r\n",file_size);
	printf("MD5=");
	for(i=0;i<16;i++){
		printf("%02X",out[i]);
	}
	printf("\r\n");

	return 0;
}


int main(int argc, char ** argv)
{
	int retv = 0;

	if (argc < 2) {
		usage();
		return 0;
	}

	if (init_param(argc, argv) < 0)
		return -1;

	if (open_crypto_dev() < 0)
		return -1;

	if (autotest_flag) {
		retv = auto_test("ecb(aes)");
		retv = auto_test("ecb(des)");
		retv = auto_md5_sha1_test();
		return retv;
	}

	close_crypto_dev();

	return 0;
}


