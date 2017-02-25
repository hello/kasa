/*
 * test_hwtimer.c
 * This program can test basic function of hardware timer.
 *
 * History:
 *	2014/7/10 - [Zhaoyang Chen] create this file.
 *
 * Copyright (C) 2015 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>

#define NO_ARG 0
#define HAS_ARG 1

static int fd_hwtimer;

static int show_pts = 0;
static int auto_run = 0;

static char HW_TIMER[] = "/proc/ambarella/ambarella_hwtimer";
static char VIN_IDSP[] = "/proc/ambarella/vin0_idsp";

static struct option long_options[] = {
	{"pts",		NO_ARG,	0,	'P' },
	{"autorun",	NO_ARG,	0,	'r'},
	{0, 0, 0, 0},
};

static const char *short_options = "Pr";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"", "\t\tprint pts using hardware timer"},
	{"", "\t\trun timer automatically and check with system timer"},
};

void usage(void)
{
	int i;

	printf("test_hwtimer usage:\n");
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
}

static int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'P':
			show_pts = 1;
		break;
		case 'r':
			auto_run = 1;
		break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}
	return 0;
}

int output_pts()
{
	int fd_vin;
	char vin_int_arr[8];
	char pts_buf[32];
	unsigned long long hw_tick_curr = 0L;
	unsigned long long hw_tick_prev = 0L;
	int frame_cnt = 0;

	if ((fd_vin = open(VIN_IDSP, O_RDONLY)) < 0) {
		printf("CANNOT open [%s].\n", VIN_IDSP);
		return -1;
	}

	while (1) {
		read(fd_vin, vin_int_arr, 8);
		lseek(fd_vin, 0 , 0);
		read(fd_hwtimer, pts_buf, sizeof(pts_buf));
		lseek(fd_hwtimer, 0L, SEEK_SET);
		hw_tick_curr = strtoull(pts_buf,(char **)NULL, 10);
		if (!frame_cnt)
			hw_tick_prev = hw_tick_curr;
		printf("The PTS for frame %d is %llu.", frame_cnt, hw_tick_curr);
		printf("The frame time in 90K frequency is %llu.\n",
				(unsigned long long)(hw_tick_curr - hw_tick_prev));
		frame_cnt++;
		hw_tick_prev = hw_tick_curr;
	}

	return 0;
}

int autorun_hwtimer()
{
	int sleep_seconds;
	int interval_ms;
	struct timeval start_sys_time;
	unsigned long long start_sys_us;
	struct timeval end_sys_time;
	unsigned long long end_sys_us;
	unsigned long long sys_interval_us;
	unsigned long long sys_interval_90K;
	unsigned long long sys_interval_diff;

	unsigned long long precise_interval_90K;


	unsigned long long start_hw_tick;
	unsigned long long end_hw_tick;
	unsigned long long hw_interval;
	unsigned long long hw_interval_diff;

	char start_buf[100];
	char end_buf[100];

	srand(time(0));
	while(1) {
		sleep_seconds = 5 + (rand()%5);
		printf("Start to sleep %d seconds! (range from 5 to 10 seconds)\n",
				sleep_seconds);
		printf("..........................\n");
		usleep(sleep_seconds * 1000000);
		printf("Sleep %d seconds done!\n\n\n", sleep_seconds);

		interval_ms = 20 + (rand()%80);

		precise_interval_90K = interval_ms * 90000 / 1000;
		printf("The time interval is %d ms.\n\n", interval_ms);
		printf("The precise time interval in 90K frequency is %llu.\n\n",
				precise_interval_90K);

		gettimeofday(&start_sys_time, NULL);
		usleep(interval_ms * 1000);
		gettimeofday(&end_sys_time, NULL);

		start_sys_us = start_sys_time.tv_sec * 1000000 + start_sys_time.tv_usec;
		end_sys_us = end_sys_time.tv_sec * 1000000	+ end_sys_time.tv_usec;
		sys_interval_us = end_sys_us - start_sys_us;
		sys_interval_90K = (sys_interval_us * 90000 + 45000) / 1000000;
		if (sys_interval_90K > precise_interval_90K)
			sys_interval_diff = sys_interval_90K- precise_interval_90K;
		else
			sys_interval_diff = precise_interval_90K - sys_interval_90K;

		printf("The system timer interval is %llu us.\n", sys_interval_us);
		printf("The real system timer interval in 90K frequency is %llu.\n",
				sys_interval_90K);
		printf("The system timer difference  rate  is %3.4f%%,"
				" difference is %llu/%llu.\n\n",
				(float) sys_interval_diff * 100 / precise_interval_90K,
				sys_interval_diff, precise_interval_90K);

		read(fd_hwtimer, start_buf, sizeof(start_buf));
		lseek(fd_hwtimer, 0L, SEEK_SET);
		usleep(interval_ms * 1000);
		read(fd_hwtimer, end_buf, sizeof(end_buf));
		lseek(fd_hwtimer, 0L, SEEK_SET);

		start_hw_tick = strtoull(start_buf,(char **)NULL, 10);
		end_hw_tick = strtoull(end_buf,(char **)NULL, 10);
		hw_interval = end_hw_tick - start_hw_tick;
		if (hw_interval > precise_interval_90K)
			hw_interval_diff = hw_interval - precise_interval_90K;
		else
			hw_interval_diff = precise_interval_90K	- hw_interval;
		printf("The real hw timer interval in 90K freq is %llu.\n",
				hw_interval);
		printf("The hw timer difference  rate  is %3.4f%%,"
				" difference is %llu/%llu.\n\n",
				(float) hw_interval_diff * 100 / precise_interval_90K,
				hw_interval_diff, precise_interval_90K);

		printf("HW and system timer difference in 90K freq is %d.\n",
				abs((int)hw_interval_diff - (int)sys_interval_diff));
		printf("------------------------------------------------------------");
		printf("\n\n");
	}

	return 0;
}

int main(int argc, char **argv)
{
	if ((fd_hwtimer = open(HW_TIMER, O_RDONLY)) < 0) {
		perror(HW_TIMER);
		return -1;
	}

	if (argc < 2) {
		usage();
		return 0;
	}

	if (init_param(argc, argv) < 0)	{
		printf("init param failed \n");
		return -1;
	}

	if (!auto_run) {
		if (show_pts) {
			output_pts();
		}
	} else {
		autorun_hwtimer();
	}

	return 0;
}

