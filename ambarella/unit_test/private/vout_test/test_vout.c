/*
 * test_vout.c
 *
 * History:
 *	2009/11/20 - [Zhenwu Xue] Initial Revision
 *
 * Copyright (C) 2007-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <getopt.h>
#include <sched.h>
#endif

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>
#include <signal.h>
#include <iav_ioctl.h>

// the device file handle
int fd_iav;

#include "vout_init.c"


#define NO_ARG				0
#define HAS_ARG				1
#define	VOUT_OPTIONS_BASE		20

enum numeric_short_options {
	//Vout
	VOUT_NUMERIC_SHORT_OPTIONS,
};

static struct option long_options[] = {
	VOUT_LONG_OPTIONS()
	{0, 0, 0, 0}
};

static const char *short_options = "ef:h:i:m:p:q:v:F:H:J:M:N:S:C:g:t:V:";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	VOUT_PARAMETER_HINTS()
};

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {

		VOUT_INIT_PARAMETERS()

		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}

void usage(void)
{
	int i;

	printf("test_vout usage:\n");
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
	printf("\n");

	printf("vout mode:  ");
	for (i = 0; i < sizeof(vout_res) / sizeof(vout_res[0]); i++)
		printf("%s  ", vout_res[i].name);
	printf("\n");
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0)
		return -1;

	// open the device
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	//Dynamically change vout
	if (dynamically_change_vout())
		return 0;

	//check vout
	if (check_vout() < 0)
		return -1;

	//Init vout
	if (vout_flag[VOUT_0] && init_vout(VOUT_0, 0) < 0)
		return -1;
	if (vout_flag[VOUT_1] && init_vout(VOUT_1, 0) < 0)
		return -1;

	return 0;
}

