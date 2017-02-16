#include "socket_crypto.h"
#include "md5_sha1_example.h"


int md5_sha1_do_cipher(char *type,char *cipher,unsigned char *in,int in_len,unsigned char *out,int out_len)


{
	int opfd;
	size_t tfmfd;
	struct sockaddr_alg sa = {
		.salg_family = AF_ALG,
	};
	char buf[20]={0};

	strcpy((char *)sa.salg_type,type);
	strcpy((char *)sa.salg_name, cipher);

	tfmfd = socket(AF_ALG, SOCK_SEQPACKET, 0);

	bind(tfmfd, (struct sockaddr *)&sa, sizeof(sa));

	opfd = accept(tfmfd, NULL, 0);

	write(opfd, in,in_len);
	read(opfd, buf, out_len);
	memcpy(out,buf,out_len);

	close(opfd);
	close(tfmfd);

	return 0;
}

#define SHA1_BLOCK 20
#define MD5_BLOCK 16
int md5_sha1(void)
{
	unsigned char *in = NULL;
	unsigned char *sha1out = NULL;
	unsigned char *md5out = NULL;

	in = md5_sha1_input;
	sha1out = (unsigned char*)malloc(SHA1_BLOCK);
	md5out = (unsigned char*)malloc(MD5_BLOCK);

	printf("==========================================================\n");
	printf("SHA1 : \n");
	printf("----Input=%s\n",in);
	md5_sha1_do_cipher("hash","sha1",in,strlen((const char*)in),sha1out,SHA1_BLOCK);
	printf_arr("----Output=",sha1out,SHA1_BLOCK);
	if (arr_compare(sha1out,sha1stand,SHA1_BLOCK)){
		printf("!!!!!!!!!!!! SHA1 ERR !!!!!!!!!!!!!!!\n");
		crypto_stop();
	}else{
		printf("SHA1 Right :)\n");
	}


	printf("==========================================================\n");
	printf("MD5 : \n");
	printf("----Input=%s\n",in);
	md5_sha1_do_cipher("hash","md5",in,strlen((const char*)in),md5out,MD5_BLOCK);
	printf_arr("----Output=",md5out,MD5_BLOCK);
	if (arr_compare(md5out,md5stand,MD5_BLOCK)){
		printf("!!!!!!!!!!!! MD5 ERR !!!!!!!!!!!!!!!\n");
		crypto_stop();
	}else{
		printf("MD5 Right :)\n");
	}
	free(sha1out);
	free(md5out);
	return 0;
}
