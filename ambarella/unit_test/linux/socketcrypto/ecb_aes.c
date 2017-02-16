#include "socket_crypto.h"
#include "ecb_aes_example.h"


static int do_cipher(char *cipher,unsigned char *key,int key_len,
		unsigned int op,unsigned char *in,int in_len,unsigned char *out)
{
	int opfd;
	int tfmfd;
	struct sockaddr_alg sa = {
		.salg_family = AF_ALG,
		.salg_type = "skcipher",
	};
	struct msghdr msg = {};
	struct cmsghdr *cmsg;
	char cbuf[CMSG_SPACE(4)];
	unsigned char buf[40]={0};
	struct iovec iov;

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


	iov.iov_base = in;
	iov.iov_len = in_len;

	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	sendmsg(opfd, &msg, 0);

	read(opfd, buf, 40);
	memcpy(out,buf,40*sizeof(unsigned char));

	close(opfd);
	close(tfmfd);

	return 0;


}

#define BLOCKSIZE 16


int ecb_aes(void)
{
	unsigned char *in =NULL;
	unsigned char *out=NULL;
	unsigned char *out1=NULL;
	unsigned char *encstand=NULL;
	unsigned char *key=NULL;
	int i,keylen;

	printf("===========================================================\n");
	out =(unsigned char*)malloc(3*BLOCKSIZE);
	out1 = (unsigned char*)malloc(3*BLOCKSIZE);


	for (i=0;i<3;i++){
		if (i==0){
			key = ecb_aes_128_key;
			in = ecb_aes_128_input_1;
			encstand = ecb_aes_128_output_1;
			keylen=16;
		}else if (i==1){
			key = ecb_aes_192_key;
			in = ecb_aes_192_input_1;
			encstand = ecb_aes_192_output_1;
			keylen = 24;
		}else if (i==2){
			key = ecb_aes_256_key;
			in = ecb_aes_256_input_1;
			encstand = ecb_aes_256_output_1;
			keylen = 32;
		}
		printf("ECB(AES) %dbit key test\n",keylen*8);
		printf_arr("----key",key,keylen);
		printf_arr("----input",in,40);
		do_cipher("ecb(aes)",key,keylen,ALG_OP_ENCRYPT,in,40,out);
		printf_arr("----encrypt",out,40);
		if (arr_compare(out,encstand,40)){
			printf("!!!!!!!! Encrypt ERR !!!!!!!!!\n");
			crypto_stop();
		}else{
			printf("Encrypt Right :)\n");
		}

		do_cipher("ecb(aes)",key,keylen,ALG_OP_DECRYPT,out,40,out1);
		printf_arr("----decrypt",out1,40);
		if (arr_compare(in,out1,40)){
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
