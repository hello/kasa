/*
 * test_mem.c
 *
 * History:
 *	2010/12/24 - [Louis Sun] created file
 *
 * Copyright (C) 2007-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "basetypes.h"
#include "openssl/md5.h"

#define NO_ARG	  0
#define HAS_ARG  1
#define MD5_REC 16

static struct option long_options[] = {
	{"lib_memcpy", NO_ARG, 0, 'l'},
	{"c_memcpy", NO_ARG, 0, 'c'},
	{"lib_memset", NO_ARG, 0, 's'},
	{"c_memset", NO_ARG, 0, 'S'},
	{"time", NO_ARG, 0, 't'},
	{"allocate",HAS_ARG, 0, 'a'},
	{"noaccess", NO_ARG, 0, 'n'},
   {"correct", NO_ARG, 0, 'C'},
   {"correct-size", HAS_ARG, 0, 'Z'},
   {"correct-time", HAS_ARG, 0, 'T'},
   {"error-inject", NO_ARG,  0, 'E'},
   {"allocfree", NO_ARG,  0, 'A'},
	{0, 0, 0, 0}
};

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_options = "lcCEsSta:nZ:T:A";
static int test_clib = 0;
static int test_clang = 0;

static int test_clib_memset = 0;
static int test_c_memset = 0;
static int test_time = 0;
static int test_allocate = 0;
static int test_allocfree = 0;
static int allocate_size = 0; //in bytes

static int test_noaccess = 0;

static int test_correct = 0;
static int test_correct_size = 0;
static int test_correct_time = 0;
static int error_inject = 0;

static const struct hint_s hint[] = {
	{"", "\ttest memcpy by C run time library"},
	{"", "\ttest memcpy by C language"},
	{"", "\ttest memset by C run time library"},
	{"", "\ttest memset by C language"},
	{"", "\ttest time to see if timer is correct"},
	{"1~n","allocate how many MB memory"},
	{"", "\tdo not read/write in the allocate test"},
   {"", "\ttest whether memory is stable or not"},
   {"", "size of memory(MB) to be tested"},
   {"2~n", "times to compute md5 value"},
   {"", "modify memory to get different md5 value"},
   {"", "perform memory alloc/free speed test"},
};


void usage(void)
{
	int i;

	printf("\ntest_mem usage:\n");
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
}

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	opterr = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
        case 'l':
			test_clib = 1;
			break;

		case 'c':
			test_clang = 1;
			break;

		case 's':
			test_clib_memset = 1;
			break;

		case 'S':
			test_c_memset = 1;
			break;

		case 't':
			test_time = 1;
			break;

		case 'a':
			test_allocate = 1;
			allocate_size =  atoi(optarg) *1024*1024;
			break;

		case 'n':
			test_noaccess = 1;
			break;

		case 'C':
			test_correct = 1;
         break;

		case 'Z':
			test_correct_size = atoi (optarg) * 1024 * 1024;
         break;

		case 'T':
			test_correct_time = atoi (optarg);
         break;

		case 'E':
			error_inject = 1;
			break;

		case 'A':
			test_allocfree = 1;
			break;

		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

   if ((test_correct == 1) && (test_correct_size <= 0)) {
      printf ("For correct test, size should be greater than 0!\n");
      return -1;
   }

   if ((test_correct == 1) && (test_correct_time <= 1)) {
      printf ("For correct test, times for compute md5 value should be greater than 1!\n");
      return -1;
   }

	return 0;
}

int test_clibrary()
{
	struct timeval time_point1, time_point2;
	char * buf1, *buf2;
	const int test_length = 32*1024*1024;
	const int interation_times = 50;
	int i;
	unsigned int time_interval_us;

	buf1 = (char *)malloc(test_length);
	buf2 = (char *)malloc(test_length);
	if (!buf1 || !buf2) {
		printf("buf allocation error\n");
		return -1;
	}

	memset(buf2, 0, test_length);
	gettimeofday(&time_point1, NULL);

	for (i = interation_times; i > 0; i--)
	  memcpy(buf1, buf2, test_length);

	gettimeofday(&time_point2, NULL);

	time_interval_us = (time_point2.tv_sec - time_point1.tv_sec) * 1000000 +
						time_point2.tv_usec - time_point1.tv_usec;

	printf("Test C lib, memcpy speed is %8.1f MByte/s \n",  (double)(test_length * interation_times)/ ((double)(1024 * 1024)*  (time_interval_us/1000000.f)));

	free(buf1);
	free(buf2);

	return 0;
}

static inline void m__memcpy(u32 * dst, u32 * src,  int count)
{
	int i;
	for(i = count >> 4; i > 0; i--) {
		*dst = *src; dst++;	src++;
		*dst = *src; dst++;	src++;
		*dst = *src; dst++;	src++;
		*dst = *src; dst++;	src++;
	}
}

static inline void m_pld_memcpy(u32 * dst, u32 * src,  int count)
{
  int i;
  for(i = count >> 4; i > 0; i--) {
    *dst = *src; dst++; src++;
    *dst = *src; dst++; src++;
    *dst = *src; dst++; src++;
    *dst = *src; dst++; src++;
    asm ("PLD [%0, #128]"::"r" (src));
  }
}

int test_clanguage()
{
	struct timeval time_point1, time_point2;
	char * buf1, *buf2;
	const int test_length = 32*1024*1024;
	const int interation_times = 50;
	int i;
	unsigned int time_interval_us;

	buf1 = (char *)malloc(test_length);
	buf2 = (char *)malloc(test_length);
	if (!buf1 || !buf2) {
		printf("buf allocation error\n");
		return -1;
	}

	memset(buf2, 0, test_length);
	gettimeofday(&time_point1, NULL);

	for (i = interation_times; i > 0; i--)
	  m__memcpy((u32*)buf1, (u32*)buf2, test_length);

	gettimeofday(&time_point2, NULL);

	time_interval_us = (time_point2.tv_sec - time_point1.tv_sec) * 1000000 +
	    time_point2.tv_usec - time_point1.tv_usec;

	printf("Test C Language, memcpy speed is %8.1f MByte/s \n",  (double)(test_length * interation_times)/ ((double)(1024 * 1024)*  (time_interval_us/1000000.f)));

	memset(buf2, 0, test_length);
	gettimeofday(&time_point1, NULL);

	for (i = interation_times; i > 0; i--)
	  m_pld_memcpy((u32*)buf1, (u32*)buf2, test_length);

	gettimeofday(&time_point2, NULL);

	time_interval_us = (time_point2.tv_sec - time_point1.tv_sec) * 1000000 +
	    time_point2.tv_usec - time_point1.tv_usec;

	printf("Test C Language, memcpy with PLD speed is %8.1f MByte/s \n",  (double)(test_length * interation_times)/ ((double)(1024 * 1024)*  (time_interval_us/1000000.f)));

	free(buf1);
	free(buf2);

	return 0;
}

int clib_memset()
{
	char * buf1;
	struct timeval time_point1, time_point2;
	unsigned int time_interval_us;
	const int test_length = 32*1024*1024;
	const int interation_times = 50;
	buf1 = (char *)malloc(test_length);
	int i;

	gettimeofday(&time_point1, NULL);
	for (i = interation_times; i > 0 ; i --) {
		memset(buf1, 0, test_length);
	}
	gettimeofday(&time_point2, NULL);

	time_interval_us = (time_point2.tv_sec - time_point1.tv_sec) * 1000000 +
						time_point2.tv_usec - time_point1.tv_usec;

	printf("Test C Lib, memset speed is %8.1f MByte/s \n",  (double)(test_length * interation_times)/ ((double)(1024 * 1024)*  (time_interval_us/1000000.f)));


	free(buf1);
	return 0;
}

int c_memset()
{
	char * buf1;
	struct timeval time_point1, time_point2;
	unsigned int time_interval_us;
	const int test_length = 32*1024*1024;
	const int interation_times = 50;
	buf1 = (char *)malloc(test_length);
	int i, j;
	unsigned int * pwrite;

	gettimeofday(&time_point1, NULL);
	for (i = interation_times; i > 0 ; i --) {
		pwrite = (unsigned int *)buf1;
		for (j = 0; j < test_length/4; j++)
			*pwrite++ = 0;
	}
	gettimeofday(&time_point2, NULL);
	time_interval_us = (time_point2.tv_sec - time_point1.tv_sec) * 1000000 +
						time_point2.tv_usec - time_point1.tv_usec;

	printf("Test C Language, memset speed is %8.1f MByte/s \n",  (double)(test_length * interation_times)/ ((double)(1024 * 1024)*  (time_interval_us/1000000.f)));

	free(buf1);
	return 0;


}

int time_test()
{
	struct timeval time_point1, time_point2;
	unsigned int time_interval_us;
	gettimeofday(&time_point1, NULL);
	sleep(10);
	gettimeofday(&time_point2, NULL);

	time_interval_us = (time_point2.tv_sec - time_point1.tv_sec) * 1000000 +
						time_point2.tv_usec - time_point1.tv_usec;

	printf("Sleep 10 seconds, get time of day %d useconds \n", 	time_interval_us);
	return 0;
}

int allocate_test(int size)
{
	u32 * buf;
	u32  test;
	int i;

	if (size <= 0) {
		printf("allocate size is wrong %d \n", size);
		return -1;
	}


	printf("prepare to allocate %d bytes of mem \n", size);
	buf = (u32 *)malloc(size);
	if (!buf) {
		printf("unable to allocate %d bytes of mem \n", size);
		return -1;
	}
	printf("malloc OK, now do memset \n");
	memset(buf, 0, size);

	printf("memset OK, Now start sequential access to the mem so that it's not easily swapped out\n");


	while (1) {
		if (test_noaccess) {
			usleep(1000000);
		}
		else {
			for (i = 0; i < size/sizeof(u32); i+= 1024) {
				test = buf[i];
				buf[i] = test + 1;
				usleep(1000);
			}
		}
	}
}

static inline int init_correct_test_region (unsigned char *test_region, int region_size)
{
   int i;

   if (test_region == NULL) {
      printf ("Array needs to be initialized is empty!\n");
      return -1;
   }

   srand ((unsigned int)time (NULL));
   for (i = 0; i < region_size; i++) {
      test_region[i] = rand () % 256;
   }

   return 0;
}

static int  allocfree_test()
{
	int i;
	int j;

	#define ALLOC_ARRAY_SIZE 1000
	char *alloc_array[ALLOC_ARRAY_SIZE];
	struct timeval start, end;
	printf ("Start Alloc/Free test \n");
	gettimeofday (&start, NULL);
	for (j = 0; j < 1000; j++) {
		for (i= 0; i< ALLOC_ARRAY_SIZE; i++)
		{
			alloc_array[i] = (char *)malloc(i+1);
			if (!alloc_array[i]) {
				printf("unable to malloc size =%d\n", i+1);
				return -1;
			}
			//write to force memory allocate
			*alloc_array[i] = '\0';
		}

		for (i= 0; i< ALLOC_ARRAY_SIZE; i++)
		{
			free(alloc_array[i]);
			alloc_array[i] = NULL;
		}

	}
	gettimeofday (&end, NULL);

	printf ("Alloc/Free test consumed time: %lu microseconds\n", (end.tv_sec - start.tv_sec)*1000000L + (end.tv_usec - start.tv_usec));

	return 0;
}


int correct_test ()
{
   int i, times = 0;
   unsigned char *buf = NULL;
   unsigned char md5_result[MD5_REC];
   unsigned char old_result[MD5_REC];
   struct timeval start, end;
   MD5_CTX md5_ctx;

   gettimeofday (&start, NULL);
   if ((buf = (unsigned char *)malloc (test_correct_size)) == NULL) {
      printf ("Failed to allocate memory!\n");
      return -1;
   }

   if (init_correct_test_region (buf, test_correct_size) < 0) {
      printf ("Failed to initialize array to be tested!\n");
      return -1;
   }

   MD5_Init (&md5_ctx);

   if (!MD5_Update (&md5_ctx, buf, test_correct_size)) {
      printf ("Failed to update md5!\n");
      return -1;
   }

   if (!MD5_Final (md5_result, &md5_ctx)) {
      printf ("Failed to get md5 value!\n");
      return -1;
   }

   memcpy (old_result, md5_result, MD5_REC);

   if (error_inject) {
      buf[0] = (buf[0] + 1) % 256;
   }

   for (; times < test_correct_time - 1; times++) {
      MD5_Init (&md5_ctx);

      if (!MD5_Update (&md5_ctx, buf, test_correct_size)) {
         printf ("Failed to update md5!\n");
         return -1;
      }

      if (!MD5_Final (md5_result, &md5_ctx)) {
         printf ("Failed to get md5 value!\n");
         return -1;
      }

      for (i = 0; i < MD5_REC; i++) {
         if (md5_result[i] != old_result[i]) {
            printf ("\nmemory is not stable!\n");
            return -1;
         }
      }

      printf ("times = %d\n", times);
      memcpy (old_result, md5_result, MD5_REC);
   }

   gettimeofday (&end, NULL);
   printf ("Consumed time: %lus\n", end.tv_sec - start.tv_sec);
   printf ("\nmemory is stable!\n");
   free (buf);
   return 0;
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		usage();
		return -1;
	}

   if (init_param(argc, argv) < 0)
		return -1;


	if (test_clib)
		test_clibrary();

	if (test_clang)
		test_clanguage();

	if (test_clib_memset)
		clib_memset();

	if (test_c_memset)
		c_memset();

	if (test_time)
		time_test();

	if (test_allocate)
		allocate_test(allocate_size);

    if (test_correct)
      correct_test ();

	if (test_allocfree)
		allocfree_test();

	return 0;
}

