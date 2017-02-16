/*
 * test_pwm.c
 *
 * History:
 *	2008/7/1 - [Anthony Ginger] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
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
#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <getopt.h>
#endif
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "basetypes.h"

#define MAX_DUTY = 1000;

static int pwm_proc = -1;
static int pwm_ch = 0;

static int high_flag = 0;
static int high = 0;

static int low_flag = 0;
static int low = 0;

static int duty_flag =  0;
static int duty = 0;

#define NO_ARG				0
#define HAS_ARG				1

static struct option long_options[] = {
	{"channel", HAS_ARG, 0, 'C'},
	{"high", HAS_ARG, 0, 'H'},
	{"low", HAS_ARG, 0, 'L'},
	{"duty", HAS_ARG, 0, 'D'},
	{0, 0, 0, 0}
};

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_options = "C:H:L:D:";

static const struct hint_s hint[] = {
	{"0~4", "\tselect pwm channel (default is 0)"},
	{"0~1023", "\tspecify high+1 clock cycles for logic level high"},
	{"0~1023", "\tspecify low+1 clock cycles for logic level low"},
	{"1~999", "\tspecify duty ratio (0.1% ~ 99.9%)"},
};

void usage(void)
{
	int i;

	printf("\ntest_pwm usage:\n");
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
	printf("\nNotice:\n");
	printf("\tRun 'modprobe pwm_bl' before to make sure pwm_bl driver has been loaded.\n"
		"\tSince PWM0~4 are sourced from PWM Backlight Module.\n");
	printf("\n");
}

static int my_itoa(int num, char *str, int base)
{
	char index[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	u32	unum;
	int i = 0, j, k;

	if (base == 10 && num < 0) {
		unum = (u32)-num;
		str[i++] = '-';
	} else {
		unum = (u32)num;
	}

	do {
		str[i++] = index[unum % (u32)base];
		unum /= base;
	} while (unum);
	str[i] = '\0';

	if (str[0] == '-')
		k = 1;
	else
		k = 0;

	for (j = k; j <= (i -1) / 2 + k; ++j) {
		num = str[j];
		str[j] = str[i-j-1+k];
		str[i-j-1+k] = num;
	}

	return 0;
}

int open_pwm_proc(int channel)
{
	char pwm_device[64];

	if (pwm_proc < 0) {
		sprintf(pwm_device, "/sys/class/backlight/%d.pwm_bl/brightness", channel);
		if ((pwm_proc = open(pwm_device, O_RDWR | O_TRUNC)) < 0) {
			printf("open %s failed!\n", pwm_device);
			return -1;
		}
	}

	return 0;
}

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	opterr = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
	              case 'C':
			pwm_ch = atoi(optarg);
			break;

		case 'H':
			high_flag = 1;
			high = atoi(optarg);
			break;

		case 'L':
			low_flag = 1;
			low = atoi(optarg);
			break;

		case 'D':
			duty_flag = 1;
			duty = atoi(optarg);
			break;

		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}

static int pwm_update(int channel, int duty_cycle)
{
	u32 n = 0;
	char value[16];

	my_itoa(duty_cycle, value, 10);

	if (pwm_proc > 0) {
		printf("pwm duty : %s\n", value);
		n = write(pwm_proc, value, strlen(value));
		if (n != strlen(value)) {
			printf("pwm duty [%s] set error [%d].\n", value, n);
			return -1;
		}
	} else {
		printf("pwm proc should be opened when dc iris is on!\n");
		return -1;
	}

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

	if (open_pwm_proc(pwm_ch) < 0)
		return -1;

	if(duty_flag) {
		pwm_update(pwm_ch, duty);
		close(pwm_proc);
		return 0;
	}

	if(high_flag ^ low_flag)
		printf("please input both high cycles and low cycles!\n");

	if(high_flag && low_flag) {
		duty = high * 1000 / (high + low);
		pwm_update(pwm_ch, duty);
	}

	close(pwm_proc);
	return 0;
}

