/********************************************************************
 * test_statistics.c
 *
 * History:
 *	2012/08/09 - [Jian Tang] created file
 *
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 ********************************************************************/

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

#define MAX_NUM_ENCODE_STREAMS	(2)


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

static const char *short_opts = "ABCDEFGHm:v";

static struct option long_opts[] = {
	{"stream-A",	NO_ARG, 0, 'A'},
	{"stream-B",	NO_ARG, 0, 'B'},
	{"stream-C",	NO_ARG, 0, 'C'},
	{"stream-D",	NO_ARG, 0, 'D'},
	{"stream-E",	NO_ARG, 0, 'E'},
	{"stream-F",	NO_ARG, 0, 'F'},
	{"stream-G",	NO_ARG, 0, 'G'},
	{"stream-H",	NO_ARG, 0, 'H'},
	{"mv-div",	HAS_ARG, 0, 'm'},
	{"verbose",	NO_ARG, 0, 'v'},

	{0, 0, 0, 0}
};

static const struct hint_s hint[] = {
	{"", "\tSet configuration for stream A"},
	{"", "\tSet configuration for stream B"},
	{"", "\tSet configuration for stream C"},
	{"", "\tSet configuration for stream D"},
	{"", "\tSet configuration for stream E"},
	{"", "\tSet configuration for stream F"},
	{"", "\tSet configuration for stream G"},
	{"", "\tSet configuration for stream H"},
	{"1~30", "\tSet the frequency of motion vectors dumped in DRAM, default is 3\n"},

	{"", "\tprint more messages"},
};


static int fd_iav = -1;
static int current_stream = -1;
static int mvdump_division_factor = 3;
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
	printf("  Dump MV statistics data per 5 frames:\n");
	printf("    test_statistics -A -m 5\n\n");
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
		case 'E':
			current_stream = 4;
			break;
		case 'F':
			current_stream = 5;
			break;
		case 'G':
			current_stream = 6;
			break;
		case 'H':
			current_stream = 7;
			break;
		case 'm':
			VERIFY_STREAMID(current_stream);
			value = atoi(optarg);
			if (value < 1 || value > MAX_MVDUMP_DIVISION_FACTOR) {
				printf("Please set the division factor [%d] from the range of [1~%d]\n",
					value, MAX_MVDUMP_DIVISION_FACTOR);
				return -1;
			}
			mvdump_division_factor = value;
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
	mvbuf_mem = mmap(NULL, mvbuf_size, PROT_READ, MAP_SHARED, fd_iav, querybuf.offset);
	if (mvbuf_mem == MAP_FAILED) {
		perror("mmap (%d) failed: %s\n");
		return -1;
	}

	return 0;
}

static int display_mv_statistics(struct iav_statisdesc *statisdesc)
{
	struct iav_motion_vector *mv_dump = NULL;
	u32 stream_pitch, stream_height;
	int i, j;

	stream_pitch = statisdesc->mvdump_pitch;
	stream_height = statisdesc->mvdump_unit_sz / statisdesc->mvdump_pitch;
	stream_pitch /= sizeof(struct iav_motion_vector);

	printf("Motion vector for stream [%d], frame [%d], PTS [%lld], pitch [%d],"
		" unit size [%d].\n", current_stream, statisdesc->frame_num,
		statisdesc->dsp_pts, stream_pitch, stream_height);

	mv_dump = (struct iav_motion_vector *)(mvbuf_mem + statisdesc->data_addr_offset);

	printf("Motion Vector X Matrix : \n");
	for (i = 0; i < stream_height; ++i) {
		for (j = 0; j < stream_pitch; ++j) {
			printf("%d ", mv_dump[i * stream_pitch + j].x);
		}
		printf("\n");
	}
	printf("Motion Vector Y Matrix : \n");
	for (i = 0; i < stream_height; ++i) {
		for (j = 0; j < stream_pitch; ++j) {
			printf("%d ", mv_dump[i * stream_pitch + j].y);
		}
		printf("\n");
	}
	printf("\n");
	return 0;
}

static int get_mv_statistics(void)
{
	struct iav_querydesc query_desc;
	//struct iav_statiscfg statiscfg;
	struct iav_statisdesc *statis_desc;

	//statiscfg.id = current_stream;
	//statiscfg.enable = 1;
	//statiscfg.mvdump_div = mvdump_division_factor;
	//AM_IOCTL(fd_iav, IAVIOC_S_STATISCFG, &statiscfg);

	query_desc.qid = IAV_DESC_STATIS;
	statis_desc = &query_desc.arg.statis;
	statis_desc->id = current_stream;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_DESC, &query_desc);
	if (display_mv_statistics(statis_desc) < 0) {
		printf("Failed to display motion vector!\n");
		return -1;
	}

	//statiscfg.enable = 0;
	//AM_IOCTL(fd_iav, IAVIOC_S_STATISCFG, &statiscfg);

	return 0;
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

	if (get_mv_statistics() < 0)
		return -1;

	return 0;
}


