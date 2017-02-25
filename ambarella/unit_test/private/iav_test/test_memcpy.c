/*
 * test_memcpy.c
 *
 * History:
 *    2010/9/6 - [Louis Sun] create it to test memcpy by GDMA
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
#include <getopt.h>
#include <sched.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>

#include <signal.h>
#include <basetypes.h>
#include <iav_ioctl.h>

#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg)	\
		do { 						\
			if (ioctl(_filp, _cmd, _arg) < 0) {	\
				perror(#_cmd);		\
				return -1;			\
			}						\
		} while (0)
#endif

int fd_iav;
u8 *bsb_mem;
u32 bsb_size;
int test_option = 0;


#define NO_ARG		0
#define HAS_ARG		1

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_options = "t";

static struct option long_options[] = {
	{"test", 		NO_ARG, 0, 't'},
	{0, 0, 0, 0}
};

static const struct hint_s hint[] = {
	{"", "\t test memcpy & gdma_copy speed"},
};

static void usage(void)
{
	u32 i;
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

	printf("    >test_memcpy  -t  \n" );

}

static int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 't':
			test_option = 1;
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}

static int query_buffer(void)
{
	struct iav_querybuf buf;

	printf("\n[DRAM Layout info]:\n");

	memset(&buf, 0, sizeof(buf));
	buf.buf = IAV_BUFFER_BSB;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_BUF, &buf);
	printf("        [BSB] Base Address: [0x%08X], Size [%8d KB].\n",
		buf.offset, (buf.length >> 10));

	bsb_size = buf.length;
	bsb_mem = mmap(NULL, bsb_size, PROT_WRITE, MAP_SHARED,
		fd_iav, buf.offset);
	printf("bsb_mem = 0x%x, size = 0x%x\n", (u32)bsb_mem, bsb_size);

	if (bsb_mem == MAP_FAILED) {
		perror("mmap (%d) failed: %s\n");
		return -1;
	}

	return 0;
}

static int do_gdma_cpy_test(int size)
{
	int i;
	u32 input_size = 0x200000;
	u32 offset = 0;
	struct timeval pre_time, curr_time;
	u32 time_interval_us;
	struct iav_gdma_copy gdma_param;

	//prepare data
	for (i = 0 ; i< 0x10000; i++) {
		bsb_mem[i] = i & 0xFF;
	}

	gdma_param.src_offset = offset;
	gdma_param.dst_offset = offset + input_size;
	gdma_param.src_mmap_type = IAV_BUFFER_BSB;
	gdma_param.dst_mmap_type = IAV_BUFFER_BSB;

	gdma_param.src_pitch = 1024;
	gdma_param.dst_pitch = 1024;
	gdma_param.width = 1024;
	gdma_param.height = size;
	gettimeofday(&pre_time, NULL);

	//copy to new position
	if (ioctl(fd_iav, IAV_IOC_GDMA_COPY, &gdma_param) < 0) {
		printf("iav gdma copy failed \n");
		return -1;
	}
	gettimeofday(&curr_time, NULL);
	time_interval_us = (curr_time.tv_sec - pre_time.tv_sec) * 1000000 +
		curr_time.tv_usec - pre_time.tv_usec;
	printf("gdma copy finished, size:%dK, time diff: %d us\n",
		(gdma_param.width * gdma_param.height) >> 10, time_interval_us);

	return 0;
}

static int do_memcpy_test(int size)
{
	int i;
	u32 input_size = 0x200000;
	u32 offset = 0;
	struct timeval pre_time, curr_time;
	u32 time_interval_us;

	struct iav_gdma_copy gdma_param;

	//prepare data
	for (i = 0 ; i< 0x10000; i++) {
		bsb_mem[i] = i & 0xFF;
	}
	i = 0;
	gdma_param.src_offset = offset;
	gdma_param.dst_offset = offset + input_size;
	gdma_param.src_mmap_type = IAV_BUFFER_BSB;
	gdma_param.dst_mmap_type = IAV_BUFFER_BSB;

	gdma_param.src_pitch = 1024;
	gdma_param.dst_pitch = 1024;
	gdma_param.width = 1024;
	gdma_param.height = size;
	gettimeofday(&pre_time, NULL);
	//copy to new position
	memcpy(&bsb_mem[gdma_param.dst_offset], &bsb_mem[gdma_param.src_offset],
		gdma_param.width * size);
	gettimeofday(&curr_time, NULL);

	time_interval_us = (curr_time.tv_sec - pre_time.tv_sec) * 1000000 +
		curr_time.tv_usec - pre_time.tv_usec;
	printf("memcpy finished, size:%dK, time diff: %d us\n",
		(gdma_param.width * gdma_param.height) >> 10, time_interval_us);

	return 0;

}

int main(int argc, char **argv)
{
	int i = 0;
	if (argc < 2) {
		usage();
		return -1;
	}
	if (init_param(argc, argv) < 0) {
		usage();
		return -1;
	}

	// open the device
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (query_buffer() < 0)
		return -1;

	if (test_option) {
		sleep(1);
		for (i  = 1; i <= 2048; i = i * 2) {
			if (do_memcpy_test(i) < 0)
				return -1;
			if (do_gdma_cpy_test(i) < 0)
				return -1;
		}
	}
	return 0;
}

