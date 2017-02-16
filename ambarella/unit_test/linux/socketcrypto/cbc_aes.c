#include "socket_crypto.h"
#include "cbc_aes_example.h"


static int do_cipher(char *cipher,unsigned char *key,int key_len,
			   unsigned char *siv, int sivlen,
                           unsigned int op,
			   unsigned char *in,int in_len,
			   unsigned char *out)
{
	int opfd;
	int tfmfd;
	struct sockaddr_alg sa = {
		.salg_family = AF_ALG,
		.salg_type = "skcipher",
	};
	struct msghdr msg = {};
	struct cmsghdr *cmsg;
	char cbuf[CMSG_SPACE(4) + CMSG_SPACE(20)] = {};
	struct aes_iv {
		unsigned int len;
		unsigned char iv[16];
	}*iv=NULL;
	unsigned char *buf=NULL;
	struct iovec iov;

	buf =(unsigned char*) malloc(in_len);
	strcpy((char *)sa.salg_name, cipher);


	tfmfd = socket(AF_ALG, SOCK_SEQPACKET, 0);

	bind(tfmfd, (struct sockaddr *)&sa, sizeof(sa));

	setsockopt(tfmfd, SOL_ALG, ALG_SET_KEY,
			key,key_len);

	opfd = accept(tfmfd, NULL, 0);

	msg.msg_control = cbuf;
	msg.msg_controllen = sizeof(cbuf);

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_ALG;
	cmsg->cmsg_type = ALG_SET_OP;
	cmsg->cmsg_len = CMSG_LEN(4);
	*(__u32 *)CMSG_DATA(cmsg) = op;

	cmsg = CMSG_NXTHDR(&msg,cmsg);
	cmsg->cmsg_level = SOL_ALG;
	cmsg->cmsg_type = ALG_SET_IV;
	cmsg->cmsg_len = CMSG_LEN(20);
	iv = (void *)CMSG_DATA(cmsg);
	iv->len = sivlen;

	memcpy(iv->iv,siv,16);

	iov.iov_base = in;
	iov.iov_len = in_len;

	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;


	sendmsg(opfd, &msg, 0);

	read(opfd, buf, in_len);
	memcpy(out,buf,in_len*sizeof(unsigned char));

	free(buf);
	close(opfd);
	close(tfmfd);

	return 0;


}

#define BLOCKSIZE 16


int cbc_aes(void)
{
	unsigned char *in =NULL;
	unsigned char *out=NULL;
	unsigned char *out1=NULL;
	unsigned char *encstand=NULL;
	unsigned char *key=NULL;
	unsigned char *iv=NULL;
	int i,keylen,ivlen,ilen;
	ivlen = 16;
	printf("===========================================================\n");
	out =(unsigned char*)malloc(4*BLOCKSIZE);
	out1 = (unsigned char*)malloc(4*BLOCKSIZE);


	for (i=0;i<4;i++){
		if (i==0){
			key = cbc_aes_128_key_1;
			iv = cbc_aes_128_iv_1;
			in = cbc_aes_128_input_1;
			ilen = cbc_aes_128_ilen_1;
			encstand = cbc_aes_128_output_1;
			keylen=16;
		}else if (i==1){
			key = cbc_aes_128_key_2;
			iv = cbc_aes_128_iv_2;
			in = cbc_aes_128_input_2;
			ilen = cbc_aes_128_ilen_2;
			encstand = cbc_aes_128_output_2;
			keylen=16;
		}else if (i==2){
			key = cbc_aes_192_key;
			iv = cbc_aes_192_iv;
			in = cbc_aes_192_input_1;
			ilen = cbc_aes_192_ilen;
			encstand = cbc_aes_192_output_1;
			keylen = 24;
		}else if (i==3){
			key = cbc_aes_256_key;
			iv = cbc_aes_256_iv;
			in = cbc_aes_256_input_1;
			ilen = cbc_aes_256_ilen;
			encstand = cbc_aes_256_output_1;
			keylen = 32;
		}
		printf("CBC(AES) %dbit key test\n",keylen*8);
		printf_arr("----key",key,keylen);
		printf_arr("----input",in,ilen);
		printf_arr("----iv",iv,ivlen);
		do_cipher("cbc(aes)",key,keylen,iv,ivlen,ALG_OP_ENCRYPT,in,ilen,out);
		printf_arr("----encrypt",out,ilen);
		if (arr_compare(out,encstand,ilen)){
			printf("!!!!!!!! Encrypt ERR !!!!!!!!!\n");
			crypto_stop();
		}else{
			printf("Encrypt Right :)\n");
		}

		do_cipher("cbc(aes)",key,keylen,iv,ivlen,ALG_OP_DECRYPT,out,ilen,out1);
		printf_arr("----decrypt",out1,ilen);
		if (arr_compare(in,out1,ilen)){
			printf("!!!!!!!!! Decrypt ERR !!!!!!!!!!!!!!!!!!\n");
			crypto_stop();
		}else{
			printf("Decrypt Right :)\n");
		}
		printf("\n");
	}

	free(out1);
	free(out);

	return 0;
}
