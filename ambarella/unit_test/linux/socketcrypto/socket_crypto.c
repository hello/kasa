/*
 * socketcrypto.c
 * History:
 *	2013/02/04 - [Johnson Diao] created file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */
#include "socket_crypto.h"
#include <getopt.h>
static int quite = 0;
int printf_arr(char *name,unsigned char *list,int len)
{
	int i;
	if (!quite){
		printf("%s=",name);
		for (i=0;i<len;i++){
			printf("%02x",list[i]);
		}
		printf("\n");
	}
	return 0;
}

int arr_compare(unsigned char *s1,unsigned char *s2,int len)
{
	int i;
	for (i=0;i<len;i++){
		if (s1[i] != s2[i]){
			return -1;
		}
	}
	return 0;
}

int main(int argc,char **argv)
{
	int ch;
	while((ch = getopt(argc,argv,"qh")) != -1){
		switch (ch){
			case 'q':
				quite = 1;
				break;
			case 'h':
				printf("\n -q: quite mode\n");
				return 0;
		}
	}

	ecb_aes();
	cbc_aes();
	ecb_des();
	md5_sha1();
	return 0;
}



