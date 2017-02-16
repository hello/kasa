#include "socket_crypto.h"
#include "ecb_des_example.h"


static int des_do_cipher(char *cipher,unsigned char *key,int key_len,
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
	unsigned char buf[8]={0};
	struct iovec iov;
	int i;

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

	for (i=0;i<in_len;i+=8){
		read(opfd, buf, 8);
		memcpy(out+i,buf,8*sizeof(unsigned char));
	}

	close(opfd);
	close(tfmfd);

	return 0;


}

#define DES_BLOCKSIZE 8
int ecb_des(void)
{
	unsigned char *in =NULL;
	unsigned char *out=NULL;
	unsigned char *out1=NULL;
	unsigned char *encstand=NULL;
	unsigned char *key=NULL;
	int keylen = 8;

	printf("========================================================\n");
	out =(unsigned char*)malloc(DES_BLOCKSIZE);
	out1 = (unsigned char*)malloc(DES_BLOCKSIZE);


	key = ecb_des_key;
	in = ecb_des_input;
	encstand = ecb_des_output;

	printf("ECB(DES) %dbit key test\n",keylen);
	printf_arr("----key",key,keylen);
	printf_arr("----input",in,keylen);
	des_do_cipher("ecb(des)",key,keylen,ALG_OP_ENCRYPT,in,DES_BLOCKSIZE,out);
	printf_arr("----encrypt",out,8);
	if (arr_compare(out,encstand,8)){
		printf("!!!!!!!! Encrypt ERR !!!!!!!!!\n");
		crypto_stop();
	}else{
		printf("Encrypt Right :)\n");
	}

	des_do_cipher("ecb(des)",key,keylen,ALG_OP_DECRYPT,out,DES_BLOCKSIZE,out1);
	printf_arr("----decrypt",out1,8);
	if (arr_compare(in,out1,8)){
		printf("!!!!!!!!! Decrypt ERR !!!!!!!!!!!!!!!!!!\n");
		crypto_stop();
	}else{
		printf("Decrypt Right :)\n");
	}

	free(out1);
	free(out);

	return 0;
}
