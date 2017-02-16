#include <stdio.h>
#include <sys/socket.h>
#include <linux/if_alg.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define SOL_ALG		279

extern int md5_sha1(void);
extern int ecb_aes(void);
extern int cbc_aes(void);
extern int ecb_des(void);
extern int arr_compare(unsigned char *s1,unsigned char *s2,int len);
extern int printf_arr(char *name,unsigned char *list,int len);
#define  crypto_stop()     do{}while(1)
