/*
 * test_vin.c
 *
 * History:
 *	2009/08/05 - [Qiao Wang] create
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
static int fd_iav;

static int test_rn6243 = 0;
static int test_rn6243_mode = 0;
static int test_vin_fps_flag = 0;
static int test_vin_shutter_flag = 0;
static int shutter_time = 0;
static int test_agc_index_flag = 0;
static int agc_index = -1;
static u32 reg_op = 0;	/* 0: nothing; 1: read; 2: write  */
static u32 reg_addr = 0;
static u32 reg_data = 0;
static u32 reg_count = 1;

#include "vin_init.c"
#include "decoder/rn6243.c"


#define NO_ARG		0
#define HAS_ARG		1
static struct option long_options[] = {
	{"vin", HAS_ARG, 0, 'i'},
	{"src", HAS_ARG, 0, 'S'},
	{"frame-rate", HAS_ARG, 0, 'F'},
	{"mirror_pattern", HAS_ARG, 0, 42},
	{"bayer_pattern", HAS_ARG, 0, 44},
	{"anti_flicker", HAS_ARG, 0, 45},
	{"test-rn6243",   HAS_ARG,  0, 'R'},
	{"test-fps", NO_ARG, 0, 't'},
	{"read-reg", NO_ARG, 0, 'r'},
	{"write-reg", NO_ARG, 0, 'w'},
	{"reg-data", NO_ARG, 0, 'd'},
	{"reg-count", NO_ARG, 0, 'n'},
	{"shutter", HAS_ARG, 0, 's'},
	{"agc-index", HAS_ARG, 0, 'g'},
	{0, 0, 0, 0}
};

static const char *short_options = "i:S:F:r:tr:w:d:n:s:g:";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"vin mode", "\tchange vin mode"},
	{"source", "\tselect source"},
	{"frate", "\tset encoding frame rate"},
	{"0~3", "set vin mirror pattern"},
	{"0~3", "set vin bayer pattern "},
	{"0~2", "\tset anti-flicker only for sensor that output YUV data, 0:no-anti; 1:50Hz; 2:60Hz"},
	{"0~1",  "test rn6243 decoder, 0:default  1:WEAVE"},
	{"", "test vin fps"},
	{"addr", "read regisger"},
	{"addr", "write register"},
	{"data", "the register data to write"},
	{"count", "register count to read/write"},
	{"1-8000", "set shutter time in 1/n sec format"},
	{"0-agc max index", "set agc index, -n for index count"},
};

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {

		case 'i':
			vin_flag = 1;
			vin_mode = get_vin_mode(optarg);
			if (vin_mode < 0)
				return -1;
			break;

		case 'S':
			source = atoi(optarg);
			break;

		case 'F':
			vin_framerate_flag = 1;
			vin_framerate = atoi(optarg);
			break;

		case 42:
			mirror_pattern = atoi(optarg);
			if(mirror_pattern<0)
				return -1;
			mirror_mode.pattern = mirror_pattern;
			break;

		case 44:
			bayer_pattern = atoi(optarg);
			if(bayer_pattern<0)
				return -1;
			mirror_mode.bayer_pattern = bayer_pattern;
			break;
		case 'R':	//RN6243
			test_rn6243_mode = atoi(optarg);
			test_rn6243 = 1;

			printf("test_rn6243mode is %d , test is %d \n", test_rn6243_mode, test_rn6243);
			break;

		case 't':
			test_vin_fps_flag = 1;
			break;

		case 'r':
			reg_op = 1;
			reg_addr = strtoul(optarg, NULL, 0);
			break;

		case 'w':
			reg_op = 2;
			reg_addr = strtoul(optarg, NULL, 0);
			break;

		case 'd':
			reg_data = strtoul(optarg, NULL, 0);
			break;

		case 'n':
			reg_count = strtoul(optarg, NULL, 0);
			break;

		case 's':
			test_vin_shutter_flag = 1;
			shutter_time = atoi(optarg);
			if(shutter_time < 1 || shutter_time > 8000)
				return -1;
			break;

		case 'g':
			test_agc_index_flag = 1;
			agc_index = atoi(optarg);
			if (agc_index < 0)
				return -1;
			break;

		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}

static char irq_proc_name[256] = "/proc/ambarella/vin0_idsp";
static int irq_fd = -1;
#define MAX_FPS_CAL_TIME	100
#define SKIP_FRAME			10

int test_vin_fps()
{
	char vin_int_array[32];
	u32 fdiff_avg = 0, fdiff_max = 0, fdiff_min = 0, fdiff_cur = 0;
	int i;
	struct timeval  time1, time2;

	irq_fd = open(irq_proc_name, O_RDONLY);
	if (irq_fd < 0) {
		printf("\nCan't open %s.\n", irq_proc_name);
		return -1;
	}
	printf("\nOpen %s as source.\n", irq_proc_name);

	for (i = 0; i < MAX_FPS_CAL_TIME + SKIP_FRAME; i++)
	{
		read(irq_fd, vin_int_array, 8);
		lseek(irq_fd, 0 , 0);
		gettimeofday(&time1, NULL);

		if (i < SKIP_FRAME)
			continue;

		read(irq_fd, vin_int_array, 8);
		lseek(irq_fd, 0 , 0);
		gettimeofday(&time2, NULL);

		fdiff_cur = (time2.tv_sec- time1.tv_sec) * 1000000 + (time2.tv_usec - time1.tv_usec);//us

		if (i == SKIP_FRAME) {// first time, record the max and min value
			fdiff_max = fdiff_min = fdiff_cur;
		} else {
			fdiff_max = (fdiff_cur > fdiff_max) ? fdiff_cur: fdiff_max;
			fdiff_min = (fdiff_cur < fdiff_min) ? fdiff_cur: fdiff_min;
		}

		fdiff_avg += fdiff_cur;
	}

	fdiff_avg /= MAX_FPS_CAL_TIME;
	printf("AVG fps is %4.6f\n",  (double)1000000/(double)fdiff_avg);
	printf("MIN fps is %4.6f\n",  (double)1000000/(double)fdiff_max);
	printf("MAX fps is %4.6f\n",  (double)1000000/(double)fdiff_min);

	return 0;
}

int test_vin_shutter()
{
	struct vindev_shutter_row vin_shutter_row;
	struct vindev_agc_shutter vin_agc_shutter;
	struct vindev_shutter vin_shutter_time;
	int rval;

	vin_shutter_row.vsrc_id = 0;
	vin_shutter_row.shutter_row = (u32)DIV_ROUND(512000000, shutter_time);
	rval = ioctl(fd_iav, IAV_IOC_VIN_SHUTTER2ROW, &vin_shutter_row);
	if (rval < 0) {
		perror("IAV_IOC_VIN_SET_SHUTTER\n");
		return -1;
	}

	vin_agc_shutter.vsrc_id = 0;
	vin_agc_shutter.shutter_row = vin_shutter_row.shutter_row;
	vin_agc_shutter.mode = VINDEV_SET_SHT_ONLY;
	rval = ioctl(fd_iav, IAV_IOC_VIN_SET_AGC_SHUTTER, &vin_agc_shutter);
	if (rval < 0) {
		perror("IAV_IOC_VIN_SET_SHUTTER\n");
		return -1;
	}

	vin_shutter_time.vsrc_id = 0;
	rval = ioctl(fd_iav, IAV_IOC_VIN_GET_SHUTTER, &vin_shutter_time);
	if (rval < 0) {
		perror("IAV_IOC_VIN_GET_SHUTTER\n");
		return -1;
	}

	printf("current shutter time is %u\n", vin_shutter_time.shutter);

	return 0;
}

int test_agc_index()
{
	struct vindev_agc_shutter vin_agc_shutter;
	struct vindev_agc vin_agc;
	int i, rval;

	for (i = 0; i < reg_count; i++) {
		vin_agc_shutter.vsrc_id = 0;
		vin_agc_shutter.agc_idx = agc_index + i;
		vin_agc_shutter.mode = VINDEV_SET_AGC_ONLY;
		rval = ioctl(fd_iav, IAV_IOC_VIN_SET_AGC_SHUTTER, &vin_agc_shutter);
		if (rval < 0) {
			perror("IAV_IOC_VIN_SET_AGC_INDEX\n");
			return -1;
		}

		vin_agc.vsrc_id = 0;
		if (ioctl(fd_iav, IAV_IOC_VIN_GET_AGC, &vin_agc) < 0) {
			perror("IAV_IOC_VIN_GET_AGC");
			return -1;
		}

		printf("agc_index: %d, agc_db: %2.5fdb\n",
			vin_agc_shutter.agc_idx, (double)vin_agc.agc/16777216);//2^24
	}

	return 0;
}

static int vin_reg(void)
{
	struct vindev_reg reg;
	int i, rval;

	if (reg_op == 1) {
		for (i = 0; i < reg_count; i++) {
			reg.vsrc_id = 0;
			reg.addr = reg_addr + i;
			rval = ioctl(fd_iav, IAV_IOC_VIN_GET_REG, &reg);
			if (rval < 0) {
				perror("IAV_IOC_VIN_GET_REG");
				return -1;
			}
			printf("[0x%04x: 0x%04x] ", reg.addr, reg.data);
			if ((i + 1) % 4 == 0)
				printf("\n");
		}
		printf("\n");
	} else {
		for (i = 0; i < reg_count; i++) {
			reg.vsrc_id = 0;
			reg.addr = reg_addr + i;
			reg.data = reg_data;
			rval = ioctl(fd_iav, IAV_IOC_VIN_SET_REG, &reg);
			if (rval < 0) {
				perror("IAV_IOC_VIN_SET_REG\n");
				return -1;
			}
		}
	}

	return 0;
}


void usage(void)
{
	int i;

	printf("test_vin usage:\n");
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

	printf("vin mode:  ");
	for (i = 0; i < sizeof(__vin_modes)/sizeof(__vin_modes[0]); i++) {
		if (__vin_modes[i].name[0] == '\0') {
			printf("\n");
			printf("           ");
		} else
			printf("%s  ", __vin_modes[i].name);
	}
	printf("\n");
}

int main(int argc, char **argv)
{
	// open the device
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0)
		return -1;

	if (reg_op != 0) {
		return vin_reg();
	}

	//just fill rn6243 register
	if (test_rn6243) {
		if (init_rn6243(test_rn6243_mode) < 0)
			return -1;
		else
			return 0;
	}

	// set vin
	if (vin_flag) {
		if (init_vin(vin_mode, hdr_mode) < 0)
			return -1;
	}

	//test vin fps
	if (test_vin_fps_flag) {
		if(test_vin_fps() < 0)
			return -1;
	}

	if (test_vin_shutter_flag) {
		if(test_vin_shutter() < 0)
			return -1;
	}

	if (test_agc_index_flag) {
		if(test_agc_index() < 0)
			return -1;
	}

	return 0;
}

