/*
 * test_statistics.c
 *
 * History:
 *	2012/08/09 - [Jian Tang] created file
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
#include "img_abs_filter.h"
#include "mw_struct.h"
#include "mw_api.h"


#define NO_ARG		0
#define HAS_ARG		1

#define MAX_MVDUMP_DIVISION_FACTOR		(30)

#define MAX_NUM_ENCODE_STREAMS	(1)

typedef enum {
	DUMP_NONE = 0,
	DUMP_MV = 1,
	DUMP_TOTAL_NUM,

	DUMP_FIRST = DUMP_MV,
	DUMP_LAST = DUMP_TOTAL_NUM,
} DUMP_DATA_TYPE;

#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg)	\
		do { 						\
			if (ioctl(_filp, _cmd, _arg) < 0) {	\
				perror(#_cmd);		\
				return -1;			\
			}						\
		} while (0)
#endif

#ifndef	ROUND_UP
#define ROUND_UP(x, n)    ( ((x)+(n)-1u) & ~((n)-1u) )
#endif

#define VERIFY_STREAMID(x)	do {		\
			if ((x < 0) || (x >= MAX_NUM_ENCODE_STREAMS)) {	\
				printf("Stream id [%d] is wrong!\n", x);	\
				return -1;	\
			}	\
		} while (0)


struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_opts = "ABCDb:cd:v";

static struct option long_opts[] = {
	{"stream-A",	NO_ARG, 0, 'A'},
	{"stream-B",	NO_ARG, 0, 'B'},
	{"stream-C",	NO_ARG, 0, 'C'},
	{"stream-D",	NO_ARG, 0, 'D'},
	{"non-block",	HAS_ARG, 0, 'b'},
	{"non-stop",	NO_ARG,  0, 'c'},
	{"dump",		HAS_ARG, 0, 'd'},
	{"verbose",		NO_ARG, 0, 'v'},

	{0, 0, 0, 0}
};

static const struct hint_s hint[] = {
	{"", "\tSet configuration for stream A"},
	{"", "\tSet configuration for stream B"},
	{"", "\tSet configuration for stream C"},
	{"", "\tSet configuration for stream D"},
	{"0|1", "Select the read method, 0 is block call, 1 is non-block call. Default is block call."},
	{"",	"\tContinuous mv dump."},
	{"1~1", "\tDump data. 1: Dump MV statistics."},
	{"", "\tPrint more messages"},
};


static int fd_iav = -1;
static int current_stream = -1;
static int non_block_read = 0;
static int non_stop = 0;
static int dump_data_type = DUMP_NONE;
static int verbose = 0;
u8 *mvbuf_mem = NULL;
u32 mvbuf_size;

static void usage(void)
{
	u32 i;

	printf("test_statistics usage:\n\n");
	for (i = 0; i < sizeof(long_opts) / sizeof(long_opts[0]) - 1; ++i) {
		if (isalpha(long_opts[i].val))
			printf("-%c ", long_opts[i].val);
		else
			printf("   ");
		printf("--%s", long_opts[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}
	printf("\n");
	printf("Examples:\n");
	printf("  Dump MV statistics data with blocking way:\n");
	printf("    test_statistics -A -d 1 -b 0 \n\n");
}

static int init_param(int argc, char **argv)
{
	int ch, value;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_opts, long_opts, &option_index)) != -1) {
		switch (ch) {
		case 'A':
			current_stream = 0;
			break;
		case 'B':
			current_stream = 1;
			break;
		case 'C':
			current_stream = 2;
			break;
		case 'D':
			current_stream = 3;
			break;
		case 'b':
			value = atoi(optarg);
			if (value < 0 || value > 1) {
				printf("Read flag [%d] must be [0|1].\n", value);
				return -1;
			}
			non_block_read = value;
			break;
		case 'c':
			non_stop = 1;
			break;
		case 'd':
			value = atoi(optarg);
			if (value < DUMP_FIRST || value >= DUMP_LAST) {
				printf("Dump data type %d must be in the range of [%d~%d].\n",
					value, DUMP_FIRST, (DUMP_LAST - 1));
				return -1;
			}
			dump_data_type = value;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
			break;
		}
	}
	return 0;
}

static int map_buffer(void)
{
	struct iav_querybuf querybuf;

	querybuf.buf = IAV_BUFFER_MV;
	if (ioctl(fd_iav, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
		perror("IAV_IOC_QUERY_BUF");
		return -1;
	}
	mvbuf_size = querybuf.length;
	if (mvbuf_size == 0) {
		printf("MV data mem size cannot be zero!\n");
		return -1;
	}
	mvbuf_mem = mmap(NULL, mvbuf_size, PROT_READ, MAP_SHARED, fd_iav,
		querybuf.offset);
	if (mvbuf_mem == MAP_FAILED) {
		perror("mmap (%d) failed: %s\n");
		return -1;
	}
	if (verbose) {
		printf("MV data mem base addr: 0x%x, size: 0x%x.\n",
			(u32)mvbuf_mem, mvbuf_size);
	}

	return 0;
}

static int check_mv_statis(void)
{
	struct iav_stream_info info;

	info.id = current_stream;
	AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_INFO, &info);
	if (info.state != IAV_STREAM_STATE_ENCODING) {
		printf("The stream state should be encoding for MV statis!\n");
		return -1;
	}

	return 0;
}

static int display_mv_statis(struct iav_statisdesc *desc)
{
	struct iav_mv *mv = NULL;
	int i, j;

	printf("Motion Vector for stream [%d], frame num [%u], dsp PTS [%u], "
		"mono PTS [%llu], width [%u], pitch [%u], height [%u].\n",
		current_stream, desc->frame_num, desc->dsp_pts, desc->arm_pts,
		desc->width, desc->pitch, desc->height);

	mv = (struct iav_mv *)(mvbuf_mem + desc->data_addr_offset);
	printf("Motion Vector X Matrix : \n");
	for (i = 0; i < desc->height; ++i) {
		for (j = 0; j < desc->width; ++j) {
			printf("%d ", mv[i * desc->pitch + j].x);
		}
		printf("\n");
	}
	printf("Motion Vector Y Matrix : \n");
	for (i = 0; i < desc->height; ++i) {
		for (j = 0; j <  desc->width; ++j) {
			printf("%d ", mv[i * desc->pitch + j].y);
		}
		printf("\n");
	}
	printf("\n");
	return 0;
}

static int get_mv_statis(void)
{
	struct iav_querydesc query_desc;
	struct iav_statisdesc *desc;

	query_desc.qid = IAV_DESC_STATIS;
	desc = &query_desc.arg.statis;
	desc->id = current_stream;

	if (non_block_read == 0) {
		desc->time_ms = 0;
		AM_IOCTL(fd_iav, IAV_IOC_QUERY_DESC, &query_desc);
	} else {
		desc->time_ms = -1;
		// make sue valid MV statis for non blocking read
		while (ioctl(fd_iav, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
			if (errno != EAGAIN) {
				printf("Failed to query MV statis!\n");
				return -1;
			}
		}
	}
	if (display_mv_statis(desc) < 0) {
		printf("Failed to display Motion Vector!\n");
		return -1;
	}

	return 0;
}

static int config_mv_statis(int enable)
{
	struct iav_stream_cfg cfg;
	memset(&cfg, 0, sizeof(cfg));
	cfg.id = current_stream;
	cfg.cid = IAV_H264_CFG_STATIS;
	cfg.arg.h264_statis = enable;
	AM_IOCTL(fd_iav, IAV_IOC_SET_STREAM_CONFIG, &cfg);

	return 0;
}

static void quit_statis()
{
	if (dump_data_type == DUMP_MV) {
		if (config_mv_statis(0) < 0) {
			printf("Disable MV statis failed!\n");
		}
	}
	close(fd_iav);
	fd_iav = -1;
	exit(1);
}


int main(int argc, char **argv)
{
	if (argc < 2) {
		usage();
		return -1;
	}
	if (init_param(argc, argv) < 0) {
		usage();
		return -1;
	}

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (map_buffer() < 0) {
		printf("Failed to map buffer!\n");
		return -1;
	}

	signal(SIGINT, quit_statis);
	signal(SIGTERM, quit_statis);
	signal(SIGQUIT, quit_statis);

	switch(dump_data_type) {
	case DUMP_MV:
		if (check_mv_statis() < 0) {
			printf("Check MV statis failed!\n");
			return -1;
		}
		if (config_mv_statis(1) < 0) {
			printf("Enable MV statis failed!\n");
			return -1;
		}
		do {
			if (get_mv_statis() < 0) {
				printf("Get MV statis failed!\n");
				return -1;
			}
		} while (non_stop);
		if (config_mv_statis(0) < 0) {
			printf("Disable MV statis failed!\n");
			return -1;
		}
		break;
	default:
		printf("Invalid dump data type [%d].\n", dump_data_type);
		break;
	}

	close(fd_iav);
	fd_iav = -1;

	return 0;
}


