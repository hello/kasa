/*
 * test_frame_sync.c
 *
 * History:
 *	2015/02/11 - [Zhaoyang Chen] Created file
 *
 * Copyright (C) 2015-2019, Ambarella, Inc.
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
#include <getopt.h>
#include <sched.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>

#include <basetypes.h>
#include <iav_ioctl.h>
#include <signal.h>


#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg)	\
		do { 						\
			if (ioctl(_filp, _cmd, _arg) < 0) {	\
				perror(#_cmd);		\
				return -1;			\
			}						\
		} while (0)
#endif

#ifndef ROUND_UP
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
#endif

#ifndef DIV_ROUND
#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )
#endif

#define MAX_ENCODE_STREAM_NUM		(IAV_STREAM_MAX_NUM_IMPL)
#define MAX_SRC_BUFFER_NUM			(IAV_SRCBUF_NUM)

// the device file handle
static int fd_iav = -1;
static u8 *dsp_mem = NULL;
static u32 dsp_size = 0;
static u8 *qp_matrix_addr = NULL;
static int stream_qp_matrix_size = 0;



#define VERIFY_STREAMID(x)   do {		\
			if (((x) < 0) || ((x) >= MAX_ENCODE_STREAM_NUM)) {	\
				printf ("stream id wrong %d \n", (x));			\
				return -1; 	\
			}	\
		} while (0)


// h.264 config
typedef struct h264_gop_param_s {
	int h264_N;
	int h264_N_flag;
	int h264_idr_interval;
	int h264_idr_interval_flag;
} h264_gop_param_t;

typedef struct h264_mv_threshold_param_s {
	u8	mv_threshold;
	u8	mv_threshold_flag;
} h264_mv_threshold_param_t;

typedef struct h264_qproi_param_t {
	int	type;
	int	type_flag;

	int	qp_quality;
	int	qp_quality_flag;

	int qp_offset;
	int qp_offset_flag;

	int	encode_width;
	int	encode_height;
} h264_qproi_param_t;

typedef struct h264_qp_limit_param_s {
	u8	qp_min_i;
	u8	qp_max_i;
	u8	qp_min_p;
	u8	qp_max_p;
	u8	qp_min_b;
	u8	qp_max_b;
	u8	adapt_qp;
	u8	i_qp_reduce;
	u8	p_qp_reduce;
	u8	skip_frame;

	u8	qp_i_flag;
	u8	qp_p_flag;
	u8	qp_b_flag;
	u8	adapt_qp_flag;
	u8	i_qp_reduce_flag;
	u8	p_qp_reduce_flag;
	u8	skip_frame_flag;
} h264_qp_limit_param_t;

typedef struct iav_qproi_data_s {
	u8 qp_quality;
	u8 qp_value;
	u8 reserved[2];
}  iav_qproi_data_t;

typedef struct h264_frame_drop_param_s {
	u8	frame_drop;
	u8	frame_drop_flag;
} h264_frame_drop_param_t;

typedef struct h264_enc_param_s {
	u16	intrabias_p;
	u16	intrabias_p_flag;

	u16	intrabias_b;
	u16	intrabias_b_flag;

	u8	user1_intra_bias;
	u8	user1_intra_bias_flag;
	u8	user1_direct_bias;
	u8	user1_direct_bias_flag;
	u8	user2_intra_bias;
	u8	user2_intra_bias_flag;
	u8	user2_direct_bias;
	u8	user2_direct_bias_flag;
	u8	user3_intra_bias;
	u8	user3_intra_bias_flag;
	u8	user3_direct_bias;
	u8	user3_direct_bias_flag;
} h264_enc_param_t;


static int current_stream = -1;

//encoding settings
static u32 force_idr_id_map = 0;

static u32 force_fast_seek_map = 0;

static h264_gop_param_t h264_gop_param[MAX_ENCODE_STREAM_NUM];
static u32 h264_gop_param_changed_map = 0;

static h264_mv_threshold_param_t h264_mv_threshold_param[MAX_ENCODE_STREAM_NUM];
static u32 h264_mv_threshold_changed_map = 0;

static h264_qproi_param_t h264_qproi_param[MAX_ENCODE_STREAM_NUM];
static u32 h264_qproi_changed_map = 0;

static h264_qp_limit_param_t h264_qp_limit_param[MAX_ENCODE_STREAM_NUM];
static u32 h264_qp_limit_changed_map = 0;

static h264_frame_drop_param_t h264_frame_drop_param[MAX_ENCODE_STREAM_NUM];
static u32 h264_frame_drop_map = 0;

static h264_enc_param_t h264_enc_param[MAX_ENCODE_STREAM_NUM];
static u32 h264_enc_param_map = 0;

static u32 force_update_map = 0;

static u32 show_params_map = 0;
static int sleep_time = 1;

#define	NO_ARG		0
#define	HAS_ARG		1

enum numeric_short_options {
	SPECIFY_FORCE_IDR = 0,
	SPECIFY_GOP_IDR,
	SPECIFY_IDR_INTERVAL,

	SPECIFY_MV_THRESHOLD,

	SPECIFY_QP_ROI_TYPE,
	SPECIFY_QP_ROI_QUALITY,
	SPECIFY_QP_ROI_OFFSET,

	SPECIFY_QP_LIMIT_I,
	SPECIFY_QP_LIMIT_P,
	SPECIFY_QP_LIMIT_B,
	SPECIFY_ADAPT_QP,
	SPECIFY_I_QP_REDUCE,
	SPECIFY_P_QP_REDUCE,
	SPECIFY_SKIP_FRAME_MODE,

	SPECIFY_FORCE_FAST_SEEK,
	SPECIFY_FRAME_DROP,
	SPECIFY_INTRABIAS_P,
	SPECIFY_INTRABIAS_B,
	SPECIFY_USER1_INTRABIAS,
	SPECIFY_USER1_DIRECTBIAS,
	SPECIFY_USER2_INTRABIAS,
	SPECIFY_USER2_DIRECTBIAS,
	SPECIFY_USER3_INTRABIAS,
	SPECIFY_USER3_DIRECTBIAS,

	SPECIFY_FORCE_UPDATE,

	SHOW_PARAMETERS,
};

static struct option long_options[] = {
	// -A xxxxx    means all following configs will be applied to stream A
	{"stream_A",	NO_ARG,			0,	'A' },
	{"stream_B",	NO_ARG,			0,	'B' },
	{"stream_C",	NO_ARG,			0,	'C' },
	{"stream_D",	NO_ARG,			0,	'D' },

	{"force-idr",	NO_ARG,			0,	SPECIFY_FORCE_IDR },

	// 264 params
	{"mv-threshold",	HAS_ARG,	0,	SPECIFY_MV_THRESHOLD},

	// h264 gop encode configurations
	{"N",		HAS_ARG,			0,	'N'},
	{"idr",		HAS_ARG,			0,	SPECIFY_GOP_IDR},

	// qp roi setting
	{"qproi-type",	HAS_ARG,		0,	 SPECIFY_QP_ROI_TYPE},
	{"qproi-quality",	HAS_ARG,	0,	 SPECIFY_QP_ROI_QUALITY},
	{"qproi-offset",	HAS_ARG,	0,	 SPECIFY_QP_ROI_OFFSET},

	// qp limit setting
	{"qp-limit-i",	HAS_ARG,		0,	SPECIFY_QP_LIMIT_I},
	{"qp-limit-p",	HAS_ARG,		0,	SPECIFY_QP_LIMIT_P},
	{"qp-limit-b",	HAS_ARG,		0,	SPECIFY_QP_LIMIT_B},
	{"adapt-qp",	HAS_ARG,		0,	SPECIFY_ADAPT_QP},
	{"i-qp-reduce",	HAS_ARG,		0,	SPECIFY_I_QP_REDUCE},
	{"p-qp-reduce",	HAS_ARG,		0,	SPECIFY_P_QP_REDUCE},
	{"skip-frame-mode",	HAS_ARG,	0,	SPECIFY_SKIP_FRAME_MODE},

	{"force-fast-seek",	NO_ARG,		0,	SPECIFY_FORCE_FAST_SEEK},
	{"frame-drop",	HAS_ARG, 		0,	SPECIFY_FRAME_DROP},
	{"intrabias-p",	HAS_ARG, 		0,	SPECIFY_INTRABIAS_P},
	{"intrabias-b",	HAS_ARG, 		0,	SPECIFY_INTRABIAS_B},
	{"user1-intrabias", HAS_ARG,	0,	SPECIFY_USER1_INTRABIAS},
	{"user1-directbias", HAS_ARG,	0,	SPECIFY_USER1_DIRECTBIAS},
	{"user2-intrabias", HAS_ARG,	0,	SPECIFY_USER2_INTRABIAS},
	{"user2-directbias", HAS_ARG,	0,	SPECIFY_USER2_DIRECTBIAS},
	{"user3-intrabias", HAS_ARG,	0,	SPECIFY_USER3_INTRABIAS},
	{"user3-directbias", HAS_ARG,	0,	SPECIFY_USER3_DIRECTBIAS},

	// force update frame sync or not
	{"force-update",	HAS_ARG,	0,	SPECIFY_FORCE_UPDATE},

	{"show-param",	NO_ARG,	0,	SHOW_PARAMETERS},
	{"sleep",	HAS_ARG,	0,	's' },

	{0, 0, 0, 0}
};

static const char *short_options = "ABCDN:s:";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"", "\t\tconfig for stream A"},
	{"", "\t\tconfig for stream B"},
	{"", "\t\tconfig for stream C"},
	{"", "\t\tconfig for stream D\n"},

	//immediate action, configure encode stream on the fly
	{"", "\t\tforce IDR at once for current stream"},

	{"0~255", "set zmv threshold for current stream, value 0 means disable it"},

	//h264 gop encode configurations
	{"1~4095", "\t\tH.264 GOP parameter N, must be multiple of M, can be changed during encoding"},
	{"1~128", "\tthe number of GOP per an IDR picture, can be changed during encoding"},
	{"0|1", "\tsetting qp roi type, 0:base type, 1: adv type, default 1"},
	{"0~3", "setting qp quality level for qp roi"},
	{"-51~51", "setting qp offset for qp roi"},

	{"0~51", "\tset I-frame qp limit range, 0:auto 1~51:qp limit range"},
	{"0~51", "\tset P-frame qp limit range, 0:auto 1~51:qp limit range"},
	{"0~51", "\tset B-frame qp limit range, 0:auto 1~51:qp limit range"},
	{"0~4", "\tset strength of adaptive qp"},
	{"1~10", "\tset diff of I QP less than P QP"},
	{"1~5", "\tset diff of P QP less than B QP"},
	{"0|1|2", "0: disable, 1: skip based on CPB size, 2: skip based on target bitrate and max QP"},

	//immediate action, configure encode stream on the fly
	{"", "\tforce fast seek frame at once for current stream, used in gop 8"},
	{"0~255", "\tSpecify how many frames encoder will drop, can update on the fly"},
	{"1~4000", "Specify intrabias for P frames of current stream"},
	{"1~4000", "Specify intrabias for B frames of current stream"},
	{"0~128", "Specify user1 intra bias strength, 0: no bias, 128: the strongest."},
	{"0~128", "Specify user1 direct bias strength, 0: no bias, 128: the strongest."},
	{"0~128", "Specify user2 intra bias strength, 0: no bias, 128: the strongest."},
	{"0~128", "Specify user2 direct bias strength, 0: no bias, 128: the strongest."},
	{"0~128", "Specify user3 intra bias strength, 0: no bias, 128: the strongest."},
	{"0~128", "Specify user3 direct bias strength, 0: no bias, 128: the strongest."},

	{"0|1", "\tSpecify force update encode parameters or not"},

	{"", "\t\tshow parameters.\n"},
	{"", "\t\tSleep time, unit:ms\n"},

};

static void usage(void)
{
	int i;

	printf("test_frame_sync usage:\n");
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
	printf("\nExamples:\n"
			"  Add force idr for the frame in stream A:\n"
			"    test_frame_sync -A --force-idr\n"
			"  Set qp offset = -10 for the left top region in steam A:\n"
			"    test_frame_sync -A --qproi-type 1 --qproi-offset -10\n"
			"  Update qp limit p in stream A :\n"
			"    test_frame_sync -A --qp-limit-p 20~20\n"
			"  Add force fast seek frame in stream A:\n"
			"    test_frame_sync -A --force-fast-seek\n"
			"  Force to drop the next 10 frames in stream A:\n"
			"    test_frame_sync -A --frame-drop 10\n"
			"  Set intra bias for P frame = 1000 in steam A:\n"
			"    test_frame_sync -A --intrabias-p 1000\n"
			"  Set adaptive qp to 4 in steam A, and update anyway:\n"
			"    test_frame_sync -A --adapt-qp 4 --force-update 1\n");
	printf("\n");

}

//first second value must in format "x~y" if delimiter is '~'
static int get_two_unsigned_int(const char *name, u32 *first, u32 *second, char delimiter)
{
	char tmp_string[16];
	char * separator;

	separator = strchr(name, delimiter);
	if (!separator) {
		printf("range should be like a%cb \n", delimiter);
		return -1;
	}

	strncpy(tmp_string, name, separator - name);
	tmp_string[separator - name] = '\0';
	*first = atoi(tmp_string);
	strncpy(tmp_string, separator + 1,  name + strlen(name) -separator);
	*second = atoi(tmp_string);

//	printf("input string %s,  first value %d, second value %d \n",name, *first, *second);
	return 0;
}

static int map_buffer(void)
{
	struct iav_querybuf querybuf;

	querybuf.buf = IAV_BUFFER_DSP;
	if (ioctl(fd_iav, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
		perror("IAV_IOC_QUERY_BUF");
		return -1;
	}

	dsp_size = querybuf.length;
	dsp_mem = mmap(NULL, dsp_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_iav, querybuf.offset);
	if (dsp_mem == MAP_FAILED) {
		perror("mmap (%d) failed: %s\n");
		return -1;
	}

	querybuf.buf = IAV_BUFFER_QPMATRIX;
	if (ioctl(fd_iav, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
		perror("IAV_IOC_QUERY_BUF");
		return -1;
	}

	qp_matrix_addr = mmap(NULL, querybuf.length, PROT_READ | PROT_WRITE,
		MAP_SHARED, fd_iav, querybuf.offset);
	if (qp_matrix_addr == MAP_FAILED) {
		perror("mmap (%d) failed: %s\n");
		return -1;
	}
	stream_qp_matrix_size = querybuf.length / MAX_ENCODE_STREAM_NUM;

	return 0;
}

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	u32 min_value, max_value;

	opterr = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {

		// handle all other options
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

			case SPECIFY_FORCE_IDR:
				VERIFY_STREAMID(current_stream);
				//force idr
				force_idr_id_map |= (1 << current_stream);
				break;

			case SPECIFY_MV_THRESHOLD:
				VERIFY_STREAMID(current_stream);
				min_value = atoi(optarg);
				if (min_value > 255 || min_value < 0) {
					printf("Invalid zmv threshold value [%d], please choose from [%d~%d].\n",
						min_value, 0, 255);
					return -1;
				}
				h264_mv_threshold_param[current_stream].mv_threshold = min_value;
				h264_mv_threshold_param[current_stream].mv_threshold_flag = 1;
				h264_mv_threshold_changed_map |= (1 << current_stream);
				break;

			case 'N':
				VERIFY_STREAMID(current_stream);
				h264_gop_param[current_stream].h264_N = atoi(optarg);
				h264_gop_param[current_stream].h264_N_flag = 1;
				h264_gop_param_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_GOP_IDR:
				//idr
				VERIFY_STREAMID(current_stream);
				h264_gop_param[current_stream].h264_idr_interval = atoi(optarg);
				h264_gop_param[current_stream].h264_idr_interval_flag = 1;
				h264_gop_param_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_QP_ROI_TYPE:
				VERIFY_STREAMID(current_stream);
				h264_qproi_param[current_stream].type = atoi(optarg);
				h264_qproi_param[current_stream].type_flag = 1;
				h264_qproi_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_QP_ROI_QUALITY:
				VERIFY_STREAMID(current_stream);
				h264_qproi_param[current_stream].qp_quality = atoi(optarg);
				h264_qproi_param[current_stream].qp_quality_flag = 1;
				h264_qproi_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_QP_ROI_OFFSET:
				VERIFY_STREAMID(current_stream);
				h264_qproi_param[current_stream].qp_offset = atoi(optarg);
				h264_qproi_param[current_stream].qp_offset_flag = 1;
				h264_qproi_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_QP_LIMIT_I:
				VERIFY_STREAMID(current_stream);
				if (get_two_unsigned_int(optarg, &min_value, &max_value, '~') < 0) {
					return -1;
				}
				h264_qp_limit_param[current_stream].qp_min_i = min_value;
				h264_qp_limit_param[current_stream].qp_max_i = max_value;
				h264_qp_limit_param[current_stream].qp_i_flag = 1;
				h264_qp_limit_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_QP_LIMIT_P:
				VERIFY_STREAMID(current_stream);
				if (get_two_unsigned_int(optarg, &min_value, &max_value, '~') < 0) {
					return -1;
				}
				h264_qp_limit_param[current_stream].qp_min_p = min_value;
				h264_qp_limit_param[current_stream].qp_max_p = max_value;
				h264_qp_limit_param[current_stream].qp_p_flag = 1;
				h264_qp_limit_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_QP_LIMIT_B:
				VERIFY_STREAMID(current_stream);
				if (get_two_unsigned_int(optarg, &min_value, &max_value, '~') < 0) {
					return -1;
				}
				h264_qp_limit_param[current_stream].qp_min_b= min_value;
				h264_qp_limit_param[current_stream].qp_max_b = max_value;
				h264_qp_limit_param[current_stream].qp_b_flag = 1;
				h264_qp_limit_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_ADAPT_QP:
				VERIFY_STREAMID(current_stream);
				h264_qp_limit_param[current_stream].adapt_qp = atoi(optarg);
				h264_qp_limit_param[current_stream].adapt_qp_flag = 1;
				h264_qp_limit_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_I_QP_REDUCE:
				VERIFY_STREAMID(current_stream);
				h264_qp_limit_param[current_stream].i_qp_reduce = atoi(optarg);
				h264_qp_limit_param[current_stream].i_qp_reduce_flag = 1;
				h264_qp_limit_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_P_QP_REDUCE:
				VERIFY_STREAMID(current_stream);
				h264_qp_limit_param[current_stream].p_qp_reduce = atoi(optarg);
				h264_qp_limit_param[current_stream].p_qp_reduce_flag = 1;
				h264_qp_limit_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_SKIP_FRAME_MODE:
				VERIFY_STREAMID(current_stream);
				h264_qp_limit_param[current_stream].skip_frame = atoi(optarg);
				h264_qp_limit_param[current_stream].skip_frame_flag = 1;
				h264_qp_limit_changed_map |= (1 << current_stream);
				break;

			case SPECIFY_FORCE_FAST_SEEK:
				VERIFY_STREAMID(current_stream);
				force_fast_seek_map |= (1 << current_stream);
				break;

			case SPECIFY_FRAME_DROP:
				VERIFY_STREAMID(current_stream);
				h264_frame_drop_param[current_stream].frame_drop = atoi(optarg);
				h264_frame_drop_param[current_stream].frame_drop_flag = 1;
				h264_frame_drop_map |= (1 << current_stream);
				break;

			case SPECIFY_INTRABIAS_P:
				VERIFY_STREAMID(current_stream);
				h264_enc_param[current_stream].intrabias_p = atoi(optarg);
				h264_enc_param[current_stream].intrabias_p_flag = 1;
				h264_enc_param_map |= (1 << current_stream);
				break;

			case SPECIFY_INTRABIAS_B:
				VERIFY_STREAMID(current_stream);
				h264_enc_param[current_stream].intrabias_b = atoi(optarg);
				h264_enc_param[current_stream].intrabias_b_flag = 1;
				h264_enc_param_map |= (1 << current_stream);
				break;

			case SPECIFY_USER1_INTRABIAS:
				VERIFY_STREAMID(current_stream);
				h264_enc_param[current_stream].user1_intra_bias = atoi(optarg);
				h264_enc_param[current_stream].user1_intra_bias_flag = 1;
				h264_enc_param_map |= (1 << current_stream);
				break;

			case SPECIFY_USER1_DIRECTBIAS:
				VERIFY_STREAMID(current_stream);
				h264_enc_param[current_stream].user1_direct_bias = atoi(optarg);
				h264_enc_param[current_stream].user1_direct_bias_flag = 1;
				h264_enc_param_map |= (1 << current_stream);
				break;

			case SPECIFY_USER2_INTRABIAS:
				VERIFY_STREAMID(current_stream);
				h264_enc_param[current_stream].user2_intra_bias = atoi(optarg);
				h264_enc_param[current_stream].user2_intra_bias_flag = 1;
				h264_enc_param_map |= (1 << current_stream);
				break;

			case SPECIFY_USER2_DIRECTBIAS:
				VERIFY_STREAMID(current_stream);
				h264_enc_param[current_stream].user2_direct_bias = atoi(optarg);
				h264_enc_param[current_stream].user2_direct_bias_flag = 1;
				h264_enc_param_map |= (1 << current_stream);
				break;

			case SPECIFY_USER3_INTRABIAS:
				VERIFY_STREAMID(current_stream);
				h264_enc_param[current_stream].user3_intra_bias = atoi(optarg);
				h264_enc_param[current_stream].user3_intra_bias_flag = 1;
				h264_enc_param_map |= (1 << current_stream);
				break;

			case SPECIFY_USER3_DIRECTBIAS:
				VERIFY_STREAMID(current_stream);
				h264_enc_param[current_stream].user3_direct_bias = atoi(optarg);
				h264_enc_param[current_stream].user3_direct_bias_flag = 1;
				h264_enc_param_map |= (1 << current_stream);
				break;

			case SPECIFY_FORCE_UPDATE:
				VERIFY_STREAMID(current_stream);
				min_value = atoi(optarg);
				if (min_value != 0 && min_value != 1) {
					printf("Invalide force update flag, should be [0|1].\n");
					return -1;
				}
				force_update_map = (min_value << current_stream);
				break;

			case SHOW_PARAMETERS:
				VERIFY_STREAMID(current_stream);
				show_params_map |= (1 << current_stream);
				break;

			case 's':
				sleep_time = atoi(optarg);
				break;

			default:
				printf("unknown option found: %c\n", ch);
				return -1;
			}
	}

	return 0;
}

static int cfg_sync_frame_gop_param(int stream)
{
	struct iav_stream_cfg sync_frame;
	h264_gop_param_t * param = &h264_gop_param[stream];
	struct iav_h264_gop *h264_gop = &sync_frame.arg.h264_gop;

	memset(&sync_frame, 0, sizeof(sync_frame));

	h264_gop->id = stream;

	sync_frame.id = stream;
	sync_frame.cid = IAV_H264_CFG_GOP;
	AM_IOCTL(fd_iav, IAV_IOC_GET_FRAME_SYNC_PROC, &sync_frame);

	if (param->h264_N_flag)
		h264_gop->N = param->h264_N;

	if (param->h264_idr_interval_flag)
		h264_gop->idr_interval = param->h264_idr_interval;

	AM_IOCTL(fd_iav, IAV_IOC_CFG_FRAME_SYNC_PROC, &sync_frame);

	return 0;
}

static int cfg_sync_frame_mv_threshold_param(int stream)
{
	struct iav_stream_cfg sync_frame;
	h264_mv_threshold_param_t * param = &h264_mv_threshold_param[stream];
	u32 *mv_threshold = &sync_frame.arg.mv_threshold;

	memset(&sync_frame, 0, sizeof(sync_frame));
	sync_frame.id = stream;
	sync_frame.cid = IAV_H264_CFG_ZMV_THRESHOLD;
	AM_IOCTL(fd_iav, IAV_IOC_GET_FRAME_SYNC_PROC, &sync_frame);

	if (param->mv_threshold_flag)
		*mv_threshold = param->mv_threshold;

	AM_IOCTL(fd_iav, IAV_IOC_CFG_FRAME_SYNC_PROC, &sync_frame);

	return 0;
}

static int check_for_qp_roi(int stream_id)
{
	struct iav_stream_info stream_info;
	struct iav_stream_format stream_format;

	VERIFY_STREAMID(stream_id);

	memset(&stream_info, 0, sizeof(stream_info));
	stream_info.id = stream_id;
	if (ioctl(fd_iav, IAV_IOC_GET_STREAM_INFO, &stream_info) < 0) {
		perror("IAV_IOC_GET_STREAM_INFO");
		return -1;
	}
	if (stream_info.state != IAV_STREAM_STATE_ENCODING) {
		printf("Stream %c shall be in ENCODE state.\n", 'A' + stream_id);
		return -1;
	}

	memset(&stream_format, 0, sizeof(stream_format));
	stream_format.id = stream_id;
	if (ioctl(fd_iav, IAV_IOC_GET_STREAM_FORMAT, &stream_format) < 0) {
		perror("IAV_IOC_GET_STREAM_FORMAT");
		return -1;
	}
	if (stream_format.type != IAV_STREAM_TYPE_H264) {
		printf("Stream %c encode format shall be H.264.\n", 'A' + stream_id);
		return -1;
	}
	h264_qproi_param[stream_id].encode_width = stream_format.enc_win.width;
	h264_qproi_param[stream_id].encode_height = stream_format.enc_win.height;

	return 0;
}

static int cfg_sync_frame_qp_limit_param(int stream)
{
	struct iav_stream_cfg sync_frame;
	struct iav_bitrate *h264_bitrate = &sync_frame.arg.h264_rc;
	h264_qp_limit_param_t *param = &h264_qp_limit_param[stream];

	sync_frame.id = stream;
	sync_frame.cid = IAV_H264_CFG_BITRATE;
	h264_bitrate->id = stream;
	AM_IOCTL(fd_iav, IAV_IOC_GET_FRAME_SYNC_PROC, &sync_frame);

	if (param->qp_i_flag) {
		h264_bitrate->qp_min_on_I = param->qp_min_i;
		h264_bitrate->qp_max_on_I = param->qp_max_i;
	}
	if (param->qp_p_flag) {
		h264_bitrate->qp_min_on_P = param->qp_min_p;
		h264_bitrate->qp_max_on_P = param->qp_max_p;
	}
	if (param->qp_b_flag) {
		h264_bitrate->qp_min_on_B = param->qp_min_b;
		h264_bitrate->qp_max_on_B = param->qp_max_b;
	}
	if (param->adapt_qp_flag) {
		h264_bitrate->adapt_qp = param->adapt_qp;
	}
	if (param->i_qp_reduce_flag) {
		h264_bitrate->i_qp_reduce = param->i_qp_reduce;
	}
	if (param->p_qp_reduce_flag) {
		h264_bitrate->p_qp_reduce = param->p_qp_reduce;
	}
	if (param->skip_frame_flag) {
		h264_bitrate->skip_flag = param->skip_frame;
	}

	AM_IOCTL(fd_iav, IAV_IOC_CFG_FRAME_SYNC_PROC, &sync_frame);

	return 0;
}

static int cfg_sync_frame_qproi_param(int stream)
{
	struct iav_stream_cfg sync_frame;
	struct iav_qpmatrix *h264_qp_param = &sync_frame.arg.h264_roi;
	struct iav_qproi_data *qproi_daddr = NULL;
	u8 *daddr;
	u32 size, i, j;
	u32 buf_width, buf_pitch, buf_height;
	u32 start_x, start_y, end_x, end_y;

	if (check_for_qp_roi(stream) < 0) {
		perror("check_for_qp_roi!\n");
		return -1;
	}

	sync_frame.id = stream;
	sync_frame.cid = IAV_H264_CFG_QP_ROI;
	h264_qp_param->id = stream;

	// QP matrix is MB level. One MB is 16x16 pixels.
	buf_width = ROUND_UP(h264_qproi_param[stream].encode_width, 16) / 16;
	buf_pitch = ROUND_UP(buf_width, 8);
	buf_height = ROUND_UP(h264_qproi_param[stream].encode_height, 16) / 16;
	size = buf_pitch * buf_height;

	h264_qp_param->size = size * sizeof(struct iav_qproi_data);

	h264_qp_param->qpm_no_update = 1;
	AM_IOCTL(fd_iav, IAV_IOC_GET_FRAME_SYNC_PROC, &sync_frame);

	/*	get the last qp roi setting	*/
	daddr = qp_matrix_addr + h264_qp_param->data_offset;
	qproi_daddr = (struct iav_qproi_data *)daddr;

	/* clear buf for qp roi */
	if (h264_qproi_param[stream].type == QPROI_TYPE_QP_OFFSET) {
		for (i = 0; i < size; i++) {
			qproi_daddr[i].qp_offset = 0;
		}
	} else if (h264_qproi_param[stream].type == QPROI_TYPE_QP_QUALITY) {
		for (i = 0; i < size; i++) {
			qproi_daddr[i].qp_quality = 0;
		}
	} else {
		perror("Invalid QP type for qproi!\n");
		return -1;
	}

	/* Just test with a fix area (left top)	*/
	start_x = 0;
	start_y = 0;
	end_x = buf_width / 2 + start_x;
	end_y =  buf_height / 2 + start_y;

	if (h264_qproi_param[stream].type == QPROI_TYPE_QP_OFFSET) {
		if (h264_qproi_param[stream].qp_offset_flag) {
			for (i = start_y; i < end_y && i < buf_height; i++) {
				for (j = start_x; j < end_x && j < buf_width; j++) {
					/* bit 8~15 is used for setting qp offset directly */
					qproi_daddr[i * buf_pitch + j].qp_offset =
						h264_qproi_param[stream].qp_offset;
				}
			}
		}
	} else {
		// setting qp quality
		for (i = start_y; i < end_y && i < buf_height; i++) {
			for (j = start_x; j < end_x && j < buf_width; j++) {
				qproi_daddr[i * buf_pitch + j].qp_quality =
					h264_qproi_param[stream].qp_quality;
			}
		}
	}

	/* update new addr */
	sync_frame.cid = IAV_H264_CFG_QP_ROI;
	h264_qp_param->enable = 1;
	h264_qp_param->qpm_no_check = 0;
	h264_qp_param->qpm_no_update = 0;
	h264_qp_param->type = h264_qproi_param[stream].type;

	AM_IOCTL(fd_iav, IAV_IOC_CFG_FRAME_SYNC_PROC, &sync_frame);

	return 0;
}

static int cfg_sync_frame_force_idr(int stream)
{
	struct iav_stream_cfg sync_frame;
	memset(&sync_frame, 0, sizeof(sync_frame));

	sync_frame.id = stream;
	sync_frame.cid = IAV_H264_CFG_FORCE_IDR;
	sync_frame.arg.h264_force_idr = 1;

	AM_IOCTL(fd_iav, IAV_IOC_CFG_FRAME_SYNC_PROC, &sync_frame);

	return 0;
}

static int cfg_sync_frame_force_fast_seek(int stream)
{
	struct iav_stream_cfg sync_frame;
	memset(&sync_frame, 0, sizeof(sync_frame));

	sync_frame.id = stream;
	sync_frame.cid = IAV_H264_CFG_FORCE_FAST_SEEK;
	sync_frame.arg.h264_force_fast_seek = 1;

	AM_IOCTL(fd_iav, IAV_IOC_CFG_FRAME_SYNC_PROC, &sync_frame);

	return 0;
}

static int cfg_sync_frame_drop(int stream)
{
	struct iav_stream_cfg sync_frame;
	h264_frame_drop_param_t *param = &h264_frame_drop_param[stream];

	memset(&sync_frame, 0, sizeof(sync_frame));
	sync_frame.id = stream;
	sync_frame.cid = IAV_H264_CFG_FRAME_DROP;
	sync_frame.arg.h264_drop_frames = param->frame_drop;

	AM_IOCTL(fd_iav, IAV_IOC_CFG_FRAME_SYNC_PROC, &sync_frame);

	return 0;
}

static int cfg_sync_frame_enc_param(int stream)
{
	struct iav_stream_cfg sync_frame;
	h264_enc_param_t * param = &h264_enc_param[stream];
	struct iav_h264_enc_param *h264_enc = &sync_frame.arg.h264_enc;

	memset(&sync_frame, 0, sizeof(sync_frame));

	h264_enc->id = stream;

	sync_frame.id = stream;
	sync_frame.cid = IAV_H264_CFG_ENC_PARAM;
	AM_IOCTL(fd_iav, IAV_IOC_GET_FRAME_SYNC_PROC, &sync_frame);

	if (param->intrabias_p_flag)
		h264_enc->intrabias_p = param->intrabias_p;

	if (param->intrabias_b_flag)
		h264_enc->intrabias_b = param->intrabias_b;

	if (param->user1_intra_bias_flag)
		h264_enc->user1_intra_bias = param->user1_intra_bias;

	if (param->user1_direct_bias_flag)
		h264_enc->user1_direct_bias = param->user1_direct_bias;

	if (param->user2_intra_bias_flag)
		h264_enc->user2_intra_bias = param->user2_intra_bias;

	if (param->user2_direct_bias_flag)
		h264_enc->user2_direct_bias = param->user2_direct_bias;

	if (param->user3_intra_bias_flag)
		h264_enc->user3_intra_bias = param->user3_intra_bias;

	if (param->user3_direct_bias_flag)
		h264_enc->user3_direct_bias = param->user3_direct_bias;

	AM_IOCTL(fd_iav, IAV_IOC_CFG_FRAME_SYNC_PROC, &sync_frame);

	return 0;
}

static int do_vca_on_yuv(struct iav_yuvbufdesc *info)
{
	struct iav_querydesc query_desc;
	struct iav_yuvbufdesc *yuv;
	u8* luma_addr;

	int i;
#define TEST_PATTERN	128

	if (info == NULL) {
		printf("The point is NULL\n");
		return -1;
	}
	memset(&query_desc, 0, sizeof(query_desc));
	yuv = &query_desc.arg.yuv;
	query_desc.qid = IAV_DESC_YUV;
	yuv->buf_id = info->buf_id;

	if (ioctl(fd_iav, IAV_IOC_QUERY_DESC, &query_desc)) {
		return -1;
	}
	*info = *yuv;

	/*	To do,  run VCA algorithm here	*/
	luma_addr = dsp_mem + yuv->y_addr_offset;
	for (i = 0; i < TEST_PATTERN; i++) {
		memset(luma_addr + (16 + i) * yuv->pitch, 0x0, TEST_PATTERN);
	}
	usleep(1000 * sleep_time);

	return 0;
}

static int apply_sync_frame(u32 dsp_pts)
{
	struct iav_apply_frame_sync apply;

	memset(&apply, 0, sizeof(apply));
	apply.dsp_pts = dsp_pts;
	apply.force_update = force_update_map;
	AM_IOCTL(fd_iav, IAV_IOC_APPLY_FRAME_SYNC_PROC, &apply);
	return 0;
}

static int do_iav_sync_frame(u32 sync_frame_map)
{
	int i,j;
	u32 state;
	struct iav_system_resource resource;
	struct iav_stream_info info;
	struct iav_stream_format format;
	struct iav_yuvbufdesc yuv_info;
	u32 dsp_pts;
	u32 apply_map = 0, buffer_map = 0;
	u32 stream_map[MAX_SRC_BUFFER_NUM] = {0};

	AM_IOCTL(fd_iav, IAV_IOC_GET_IAV_STATE, &state);
	if (state != IAV_STATE_ENCODING) {
		printf("Please start encoding first!\n");
		return -1;
	}

	memset(&resource, 0, sizeof(resource));
	resource.encode_mode = DSP_CURRENT_MODE;
	AM_IOCTL(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &resource);
	if (!resource.enc_dummy_latency) {
		printf("Please configure encode dummy latency with test_encode first,"
				"and the value should be > 0!\n");
		return -1;
	}

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		format.id = i;
		AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_FORMAT, &format);

		stream_map[format.buf_id] |= 1 << i;
	}

	for (j = 0; j < MAX_SRC_BUFFER_NUM; ++j) {
		if(stream_map[j] == 0) {
			continue;
		}

		buffer_map = 0;
		apply_map = 0;
		for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
			if(!(stream_map[j] & (1 << i))) {
				continue;
			}

			info.id = i;
			AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_INFO, &info);
			format.id = i;
			AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_FORMAT, &format);

			if ((format.type != IAV_STREAM_TYPE_H264) &&
				(info.state != IAV_STREAM_STATE_ENCODING)) {
				continue;
			}

			if (!(sync_frame_map & (1 << info.id))) {
				continue;
			}

			if (!(buffer_map & (1 << format.buf_id))) {
				yuv_info.buf_id = format.buf_id;
				if (do_vca_on_yuv(&yuv_info) < 0) {
					perror("do_vca_on_yuv");
					return -1;
				}
				buffer_map |= 1 << format.buf_id;
			}

			if (h264_qproi_changed_map & (1 << info.id)) {
				if (!h264_qproi_param[i].type_flag)
					h264_qproi_param[i].type = QPROI_TYPE_QP_OFFSET;
				if (h264_qproi_param[i].type == QPROI_TYPE_QP_OFFSET) {
					if (!h264_qproi_param[i].qp_offset_flag) {
						h264_qproi_param[i].qp_offset = 25;
					}
				} else {
					if (!h264_qproi_param[i].qp_quality_flag) {
						h264_qproi_param[i].qp_quality = 0;
					}
				}
				if (cfg_sync_frame_qproi_param(i) < 0) {
					return -1;
				}
				apply_map |= 1 << info.id;
			}
			if (h264_qp_limit_changed_map & (1 << info.id)) {
				if (cfg_sync_frame_qp_limit_param(i) < 0) {
					return -1;
				}
				apply_map |= 1 << info.id;
			}
			if (h264_mv_threshold_changed_map & (1 << info.id)) {
				if (cfg_sync_frame_mv_threshold_param(i) < 0) {
					return -1;
				}
				apply_map |= 1 << info.id;
			}
			if (h264_gop_param_changed_map & ( 1 << info.id)) {
				if (cfg_sync_frame_gop_param(i) < 0) {
					return -1;
				}
				apply_map |= 1 << info.id;
			}
			if (force_idr_id_map & (1 << info.id)) {
				if (cfg_sync_frame_force_idr(i) < 0) {
					return -1;
				}
				apply_map |= 1 << info.id;
			}
			if (force_fast_seek_map & (1 << info.id)) {
				if (cfg_sync_frame_force_fast_seek(i) < 0) {
					return -1;
				}
				apply_map |= 1 << info.id;
			}
			if (h264_frame_drop_map & ( 1 << info.id)) {
				if (cfg_sync_frame_drop(i) < 0) {
					return -1;
				}
				apply_map |= 1 << info.id;
			}
			if (h264_enc_param_map & ( 1 << info.id)) {
				if (cfg_sync_frame_enc_param(i) < 0) {
					return -1;
				}
				apply_map |= 1 << info.id;
			}
		}

		if (apply_map) {
			dsp_pts = (u32)yuv_info.dsp_pts;
			if (apply_sync_frame(dsp_pts) < 0) {
				perror("apply_sync_frame\n");
				return -1;
			}
		}
	}
	return 0;
}

static int show_iav_sync_frame(void)
{
	int i, j;
	struct iav_stream_cfg sync_frame;
	struct iav_bitrate *h264_rc;
	u32 buf_width, buf_pitch, buf_height;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		if (((1 << i) & show_params_map) == 0)
			continue;

		printf("===IAV_H264 frame sync parameters for stream[%c]===\n", 'A' + i);
		memset(&sync_frame, 0, sizeof(sync_frame));
		sync_frame.id = i;
		sync_frame.cid = IAV_H264_CFG_ZMV_THRESHOLD;
		AM_IOCTL(fd_iav, IAV_IOC_GET_FRAME_SYNC_PROC, &sync_frame);
		printf("\t mv_threshold:%d\n", sync_frame.arg.mv_threshold);

		sync_frame.cid = IAV_H264_CFG_QP_ROI;
		sync_frame.arg.h264_roi.id = ((1 << i) & show_params_map);

		buf_width = ROUND_UP(h264_qproi_param[i].encode_width, 16) / 16;
		buf_pitch = ROUND_UP(buf_width, 8);
		buf_height = ROUND_UP(h264_qproi_param[i].encode_height, 16) / 16;

		sync_frame.arg.h264_roi.size = buf_pitch * buf_height *
			sizeof(struct iav_qproi_data);
		sync_frame.arg.h264_roi.qpm_no_update = 1;
		AM_IOCTL(fd_iav, IAV_IOC_GET_FRAME_SYNC_PROC, &sync_frame);
		printf("\t qp roi: enable:%d\n", sync_frame.arg.h264_roi.enable);
		printf("\t qp roi: type:%d\n", sync_frame.arg.h264_roi.type);
		if (sync_frame.arg.h264_roi.type == QPROI_TYPE_QP_QUALITY) {
			for (j = 0; j < 4; j++) {
				printf("\t qp roi: qp delta of quality %d for I frame:%d\n",
					j, sync_frame.arg.h264_roi.qp_delta[QP_FRAME_I][j]);
			}
			for (j = 0; j < 4; j++) {
				printf("\t qp roi: qp delta of quality %d for P frame:%d\n",
					j, sync_frame.arg.h264_roi.qp_delta[QP_FRAME_P][j]);
			}
			for (j = 0; j < 4; j++) {
				printf("\t qp roi: qp delta of quality %d for B frame:%d\n",
					j, sync_frame.arg.h264_roi.qp_delta[QP_FRAME_B][j]);
			}
		}

		sync_frame.cid = IAV_H264_CFG_BITRATE;
		sync_frame.arg.h264_rc.id = ((1 << i) & show_params_map);
		AM_IOCTL(fd_iav, IAV_IOC_GET_FRAME_SYNC_PROC, &sync_frame);
		h264_rc = &sync_frame.arg.h264_rc;
		printf("\t qp limit: qp_limit_i:%d~%d\n",
			h264_rc->qp_min_on_I, h264_rc->qp_max_on_I);
		printf("\t qp limit: qp_limit_p:%d~%d\n",
			h264_rc->qp_min_on_P, h264_rc->qp_max_on_P);
		printf("\t qp limit: qp_limit_b:%d~%d\n",
			h264_rc->qp_min_on_B, h264_rc->qp_max_on_B);
		printf("\t qp limit: adapt_qp:%d\n", h264_rc->adapt_qp);
		printf("\t qp limit: i_qp_reduce:%d\n", h264_rc->i_qp_reduce);
		printf("\t qp limit: p_qp_reduce:%d\n", h264_rc->p_qp_reduce);
		printf("\t qp limit: skip_flag:%d\n", h264_rc->skip_flag);

		sync_frame.cid = IAV_H264_CFG_GOP;
		sync_frame.arg.h264_gop.id = ((1 << i) & show_params_map);
		AM_IOCTL(fd_iav, IAV_IOC_GET_FRAME_SYNC_PROC, &sync_frame);
		printf("\t Gop: N:%d\n", sync_frame.arg.h264_gop.N);
		printf("\t Gop: idr_interval:%d\n",
			sync_frame.arg.h264_gop.idr_interval);

		sync_frame.cid = IAV_H264_CFG_ENC_PARAM;
		sync_frame.arg.h264_enc.id = ((1 << i) & show_params_map);
		AM_IOCTL(fd_iav, IAV_IOC_GET_FRAME_SYNC_PROC, &sync_frame);
		printf("\t enc param: intrabias_p:%d\n",
			sync_frame.arg.h264_enc.intrabias_p);
		printf("\t enc param: intrabias_b:%d\n",
			sync_frame.arg.h264_enc.intrabias_b);
		printf("\t enc param: user1_intrabias:%d\n",
			sync_frame.arg.h264_enc.user1_intra_bias);
		printf("\t enc param: user1_directbias:%d\n",
			sync_frame.arg.h264_enc.user1_direct_bias);
		printf("\t enc param: user2_intrabias:%d\n",
			sync_frame.arg.h264_enc.user2_intra_bias);
		printf("\t enc param: user2_directbias:%d\n",
			sync_frame.arg.h264_enc.user2_direct_bias);
		printf("\t enc param: user3_intrabias:%d\n",
			sync_frame.arg.h264_enc.user3_intra_bias);
		printf("\t enc param: user3_directbias:%d\n",
			sync_frame.arg.h264_enc.user3_direct_bias);
	}

	return 0;
}

int main(int argc, char **argv)
{
	u32 sync_frame_map = 0;

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

	if (map_buffer() < 0) {
		return -1;
	}

	// updated streams
	sync_frame_map = h264_mv_threshold_changed_map |
		h264_gop_param_changed_map |
		h264_qproi_changed_map |
		h264_qp_limit_changed_map |
		force_idr_id_map |
		force_fast_seek_map |
		h264_frame_drop_map |
		h264_enc_param_map;

/********************************************************
 *  execution base on flag
 *******************************************************/

	//real time change encoding parameter
	if (sync_frame_map) {
		if (do_iav_sync_frame(sync_frame_map) < 0)
			return -1;
	}

	if (show_params_map) {
		if (show_iav_sync_frame() < 0)
			return -1;
	}

	if (fd_iav >= 0)
		close(fd_iav);

	return 0;
}

