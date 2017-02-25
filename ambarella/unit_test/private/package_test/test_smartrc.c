/*******************************************************************************
 * test_smartrc.c
 *
 * History:
 *   2015/11/23 - [Victor Xu] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
 ******************************************************************************/
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sched.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>
#include <basetypes.h>
#include <arm_neon.h>
#include "config.h"
#include "lib_smartrc.h"
#include "lib_smartrc_common.h"
#include "mdet_diff.h"
#include "datatx_lib.h"

#define NO_ARG (0)
#define HAS_ARG (1)
#define MM_DUMP_BASE (2020)
#define PC_DUMP_BASE (2025)
#define PC_DUMP_SIZE (128*30)
#define ADAPTIVE_STUDY_TIME (3600)
#define ADAPTIVE_MOTION_HIGH_THRESHOLD (120)
#define SHIFT_BIT (4)
#define MB_SIZE (16)

struct hint_s
{
	const char *arg;
	const char *str;
};

typedef struct smartrc_param_s {
	u32 stream_quality[SMARTRC_MAX_STREAM_NUM];
	u32 stream_quality_flag[SMARTRC_MAX_STREAM_NUM];

	u32 bitrate_ceiling[SMARTRC_MAX_STREAM_NUM];
	u32 bitrate_ceiling_flag[SMARTRC_MAX_STREAM_NUM];

	u32 style[SMARTRC_MAX_STREAM_NUM];
	u32 style_flag[SMARTRC_MAX_STREAM_NUM];

	u32 motion_level[SMARTRC_MAX_STREAM_NUM];
	u32 motion_level_flag[SMARTRC_MAX_STREAM_NUM];

	u32 noise_level[SMARTRC_MAX_STREAM_NUM];
	u32 noise_level_flag[SMARTRC_MAX_STREAM_NUM];

	u32 bitrate_gap_adjust[SMARTRC_MAX_STREAM_NUM];
	u32 bitrate_gap_adjust_flag[SMARTRC_MAX_STREAM_NUM];

	u32 dump_mot_mat[SMARTRC_MAX_STREAM_NUM];
	u32 dump_mot_mat_flag[SMARTRC_MAX_STREAM_NUM];

	u32 dump_para_cfg[SMARTRC_MAX_STREAM_NUM];
	u32 dump_para_cfg_flag[SMARTRC_MAX_STREAM_NUM];

	u32 adaptive_scenario[SMARTRC_MAX_STREAM_NUM];
	u32 adaptive_scenario_flag[SMARTRC_MAX_STREAM_NUM];

	u32 roi_interval[SMARTRC_MAX_STREAM_NUM];
	u32 roi_interval_flag[SMARTRC_MAX_STREAM_NUM];

	u32 vca_on_yuv;
	u32 vca_on_yuv_flag;

	u32 log_level;
	u32 log_level_flag;
} smartrc_param_t;

typedef struct file_dump_s {
	char file[128];
	s32 fd;
	u32 size;
	char *buffer;
} file_dump_t;

typedef struct me_buf_s {
	u32 size;
	u8 *buffer;
} me_buf_t;

static s32 fd_iav = -1;
static u8 *dsp_mem;
static u32 dsp_size;
static u32 mem_mapped = 0;

const char default_mt_mat_file[128] = "/tmp/mt_mat";
static file_dump_t g_mt_mat[SMARTRC_MAX_STREAM_NUM];
const char default_param_file[128] = "/tmp/param";
static file_dump_t g_param_cfg[SMARTRC_MAX_STREAM_NUM];

static s32 current_stream_id = -1;
static u32 stream_selection = 0;

static smartrc_param_t smartrc_params = {
	.log_level = 1,
};
static me_buf_t me_buf[SMARTRC_MAX_STREAM_NUM];
static encode_config_t g_enc_cfg[SMARTRC_MAX_STREAM_NUM];
static roi_session_t g_smartrc_session[SMARTRC_MAX_STREAM_NUM];

static mdet_session_t g_ms[SMARTRC_MAX_STREAM_NUM];
static mdet_cfg g_mdet_cfg[SMARTRC_MAX_STREAM_NUM];

static pthread_t smartrc_thread_id = 0;
static s32 smartrc_exit_flag = 0;

static u32 motion_value[SMARTRC_MAX_STREAM_NUM] = {0};
static u32 noise_value[SMARTRC_MAX_STREAM_NUM] = {0};
static u32 motion_high_cnt[SMARTRC_MAX_STREAM_NUM] = {0};

typedef enum {
	BUF_ME1_4 = 0,		// ME1 with 1/4 width/height of source buffer
	BUF_ME1_16 = 1,		// ME1 with 1/16 width/height of source buffer
	BUF_ME0_8 = 2,		// ME0 with 1/8 width/height of source buffer
	BUF_ME0_16 = 3,		// ME0 with 1/16 width/height of source buffer
	BUF_ME0_32 = 4,		// BUF_ME0_32 is 1/4 compression of BUF_ME0_8 for HEVC, since our CTB is 32x32
} buf_type_t;

static threshold_t g_threshold = {
	.motion_low = 700,
	.motion_mid = 10000,
	.motion_high = 50000,
	.noise_low = 18,
	.noise_high = 24,
};

static delay_t g_delay = {
	.motion_indicator = 1000000,
	.motion_none = 10,
	.motion_low = 10,
	.motion_mid = 10,
	.motion_high = 0,
	.noise_none = 120,
	.noise_low = 60,
	.noise_high = 0,
};
static const char *short_options = "q:b:m:n:g:t:l:d:f:c:F:a:v:i:ABCD";

static const struct option long_options[] = {
	{"quality",	HAS_ARG, 0, 'q'}, /*stream quality*/
	{"bitrate",	HAS_ARG, 0, 'b'}, /*bitrate ceiling*/
	{"motion",	HAS_ARG, 0, 'm'}, /*inject motion, with a value to indicate the motion level*/
	{"noise",	HAS_ARG, 0, 'n'}, /*inject noise, with a value to indicate the noise level*/
	{"gap",		HAS_ARG, 0, 'g'}, /*bitrate gap adjustment*/
	{"style",	HAS_ARG, 0, 't'}, /*style*/
	{"log",		HAS_ARG, 0, 'l'}, /*log level*/
	{"dumpmm",	HAS_ARG, 0, 'd'}, /*dump motion matrix*/
	{"mmfile",	HAS_ARG, 0, 'f'}, /*specify dump file name*/
	{"dumpcfg",	HAS_ARG, 0, 'c'}, /*dump stream config*/
	{"cfgfile",	HAS_ARG, 0, 'F'}, /*specify dump para cfg file*/
	{"adaptive",HAS_ARG, 0, 'a'}, /*adaptive scenario*/
	{"vca",		HAS_ARG, 0, 'v'}, /*do vca on yuv to display the motion on the fly*/
	{"interval",HAS_ARG, 0, 'i'}, /*specify the interval of implementing ROI*/
	{"stream0",	NO_ARG, 0, 'A'}, /*switch current config context to stream A*/
	{"stream1",	NO_ARG, 0, 'B'}, /*switch current config context to stream B*/
	{"stream2",	NO_ARG, 0, 'C'}, /*switch current config context to stream C*/
	{"stream3",	NO_ARG, 0, 'D'}, /*switch current config context to stream D*/
	{0, 0, 0, 0}
};

static const struct hint_s hint[] = {
	{"0~3", "stream quality, 0:low, 1:medium, 2:high, 3:ultimate, default: 0"},
	{"64000~n", "set bitrate ceiling (per GOP)"},
	{"0~3", "0:no motion, 1:small motion, 2:middle motion, 3:big motion, default: 3"},
	{"0~2", "0:no noise, 1:low noise, 2:high noise, default: 2"},
	{"0~1", "\tbitrate gap adjustment, 1:enable, 0:disable"},
	{"0~2", "0:full fps auto bitrate, 1:enable fps drop 2:security IPCAM style CBR, default: 0"},
	{"0~3", "\t0:err, 1:msg, 2:info, 3:dbg, default: 1"},
	{"0~1", "dump motion matrix, 1:enable, 0:disable, default: 0"},
	{"*.dat", "file name to store motion matrix, default:/tmp/mt_mat.dat"},
	{"0~1", "dump param config, 1:enable, 0:disable, default: 0"},
	{"*.cfg", "file name to store param cfg, default:/tmp/param.cfg"},
	{"0~1", "adaptive scenario, 1:enable, 0:disable, default: 0"},
	{"0~1", "\tdo vca on yuv to show motion on the fly, default: 0"},
	{"1~30", "specify the interval of implementing ROI, default: 1"},
	{"", "\tswitch current config context to stream A"},
	{"", "\tswitch current config context to stream B"},
	{"", "\tswitch current config context to stream C"},
	{"", "\tswitch current config context to stream D"},
};

static void usage(void)
{
	u32 i;
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val)) {
			printf("-%c ", long_options[i].val);
		} else {
			printf("	");
		}
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0) {
			printf(" [%s]", hint[i].arg);
		}
		printf("\t%s\n", hint[i].str);
	}
	printf("\nExamples:\n"
		"  Set stream quality = 2 in stream A:\n"
		"    test_smartrc -A -q 2\n\n"
		"  Set bitrate ceiling (bps per GOP) = 1000000 in steam A:\n"
		"    test_smartrc -A -b 1000000\n\n"
		"  Set initial motion = 2, initial noise = 1 in stream A :\n"
		"    test_smartrc -A -m 2 -n 1\n\n"
		"  Check MDET performance in stream A:\n"
		"    test_smartrc -A -v 1\n\n"
		"  Dump parameter configuration in stream A:\n"
		"    test_smartrc -A -c 1 -F /mnt/param_stream_A.cfg\n\n"
		"  Specify the interval of implementing ROI = 5 in stream A:\n"
		"    test_smartrc -A -i 5\n\n"
		"  Enable smartrc for multi-streams: A, B, C:\n"
		"    test_smartrc -A -q0 -t0 -B -q0 -t0 -C -q0 -t0 -l1\n\n");
	printf("\n");
}

static int init_param(int argc, char **argv)
{
	int ch, tmp;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'q':
			VERIFY_STREAM_ID(current_stream_id);
			tmp = atoi(optarg);
			if (tmp < QUALITY_FIRST || tmp >= QUALITY_LAST) {
				printf("Invalid smartrc stream quality!\n");
				return -1;
			}
			smartrc_params.stream_quality[current_stream_id] = tmp;
			smartrc_params.stream_quality_flag[current_stream_id] = 1;
			break;
		case 'b':
			VERIFY_STREAM_ID(current_stream_id);
			tmp = atoi(optarg);
			if ((tmp < 6400) || (tmp > 20 * 1024 * 1024)) {
				printf("lbr bitrate ceiling setting is wrong \n");
				return -1;
			}
			smartrc_params.bitrate_ceiling[current_stream_id] = tmp;
			smartrc_params.bitrate_ceiling_flag[current_stream_id] = 1;
			break;
		case 'm':
			VERIFY_STREAM_ID(current_stream_id);
			tmp = atoi(optarg);
			if (tmp < MOTION_FIRST || tmp >= MOTION_LAST) {
				printf("Invalid motion level.\n");
				return -1;
			}
			smartrc_params.motion_level[current_stream_id] = tmp;
			smartrc_params.motion_level_flag[current_stream_id] = 1;
			break;
		case 'n':
			VERIFY_STREAM_ID(current_stream_id);
			tmp = atoi(optarg);
			if (tmp < NOISE_FIRST || tmp >= NOISE_LAST) {
				printf("Invalid noise level.\n");
				return -1;
			}
			smartrc_params.noise_level[current_stream_id] = tmp;
			smartrc_params.noise_level_flag[current_stream_id] = 1;
			break;
		case 'g':
			VERIFY_STREAM_ID(current_stream_id);
			tmp = atoi(optarg);
			if (tmp < 0 || tmp > 1) {
				printf("Invalid bitrate gap adjust value.\n");
				return -1;
			}
			smartrc_params.bitrate_gap_adjust[current_stream_id] = tmp;
			smartrc_params.bitrate_gap_adjust_flag[current_stream_id] = 1;
			break;
		case 't':
			VERIFY_STREAM_ID(current_stream_id);
			tmp = atoi(optarg);
			if (tmp < STYLE_FIRST || tmp >= STYLE_LAST) {
				printf("Invalid smartrc style.\n");
				return -1;
			}
			smartrc_params.style[current_stream_id] = tmp;
			smartrc_params.style_flag[current_stream_id] = 1;
			break;
		case 'l':
			tmp = atoi(optarg);
			if (tmp < 0 || tmp > 3) {
				printf("Invalid log level \n");
				return -1;
			}
			smartrc_params.log_level = tmp;
			smartrc_params.log_level_flag = 1;
			break;
		case 'd':
			VERIFY_STREAM_ID(current_stream_id);
			tmp = atoi(optarg);
			if (tmp < 0 || tmp > 1) {
				printf("Invalid motion matrix value.\n");
				return -1;
			}
			smartrc_params.dump_mot_mat[current_stream_id] = tmp;
			smartrc_params.dump_mot_mat_flag[current_stream_id] = 1;
			break;
		case 'f':
			VERIFY_STREAM_ID(current_stream_id);
			strcpy(g_mt_mat[current_stream_id].file, optarg);
			break;
		case 'c':
			VERIFY_STREAM_ID(current_stream_id);
			tmp = atoi(optarg);
			if (tmp < 0 || tmp > 1) {
				printf("Invalid dump para config value.\n");
				return -1;
			}
			smartrc_params.dump_para_cfg[current_stream_id] = tmp;
			smartrc_params.dump_para_cfg_flag[current_stream_id] = 1;
			break;
		case 'F':
			VERIFY_STREAM_ID(current_stream_id);
			strcpy(g_param_cfg[current_stream_id].file, optarg);
			break;
		case 'a':
			VERIFY_STREAM_ID(current_stream_id);
			tmp = atoi(optarg);
			if (tmp < 0 || tmp > 1) {
				printf("Invalid adaptive scenario value.\n");
				return -1;
			}
			smartrc_params.adaptive_scenario[current_stream_id] = tmp;
			smartrc_params.adaptive_scenario_flag[current_stream_id] = 1;
			break;
		case 'v':
			tmp = atoi(optarg);
			if (tmp < 0 || tmp > 1) {
				printf("Invalid vca on yuv value.\n");
				return -1;
			}
			smartrc_params.vca_on_yuv = tmp;
			smartrc_params.vca_on_yuv_flag = 1;
			break;
		case 'i':
			VERIFY_STREAM_ID(current_stream_id);
			tmp = atoi(optarg);
			if (tmp < 1 || tmp > 30) {
				printf("Invalid interval value.\n");
				return -1;
			}
			smartrc_params.roi_interval[current_stream_id] = tmp;
			smartrc_params.roi_interval_flag[current_stream_id] = 1;
			break;
		case 'A':
			current_stream_id = 0;
			stream_selection |= (1<<current_stream_id);
			break;
		case 'B':
			current_stream_id = 1;
			stream_selection |= (1<<current_stream_id);
			break;
		case 'C':
			current_stream_id = 2;
			stream_selection |= (1<<current_stream_id);
			break;
		case 'D':
			current_stream_id = 3;
			stream_selection |= (1<<current_stream_id);
			break;
		default:
			printf("Unknown option found: %c\n", ch);
			return -1;
		}
	}
	return 0;
}

static void show_param_setting(void)
{
	motion_level_t motion;
	noise_level_t noise;
	quality_level_t quality;
	u32 log_level;

	printf("Current stream_id is %c\n", current_stream_id + 'A');
	smartrc_get_stream_quality(&quality, current_stream_id);
	printf("Current stream %c's stream quality is ", current_stream_id + 'A');
	switch (quality) {
	case 0:
		printf("Low Quality");
		break;
	case 1:
		printf("Medium Quality");
		break;
	case 2:
		printf("High Quality");
		break;
	case 3:
		printf("Ultimate Quality");
		break;
	default:
		printf("Invalid Value");
		break;
	}
	printf("\n");

	smartrc_get_motion_level(&motion, current_stream_id);
	printf("Current stream %c's motion level is ", current_stream_id + 'A');
	switch (motion) {
	case 0:
		printf("No Motion");
		break;
	case 1:
		printf("Small Motion");
		break;
	case 2:
		printf("Middle Motion");
		break;
	case 3:
		printf("Big Motion");
		break;
	default:
		printf("Invalid Value\n");
		break;
	}
	printf("\n");

	smartrc_get_noise_level(&noise, current_stream_id);
	printf("Current stream %c's noise level is ", current_stream_id + 'A');
	switch (noise) {
	case 0:
		printf("No Noise");
		break;
	case 1:
		printf("Low Noise");
		break;
	case 2:
		printf("High Noise");
		break;
	default:
		printf("Invalid Value\n");
		break;
	}
	printf("\n");

	smartrc_get_log_level(&log_level);
	printf("Current log level is %d\n", log_level);
}

static int show_menu(void)
{
	printf("\n================================================\n");
	printf("  s -- Show parameters\n");
	printf("  q -- Quit");
	printf("\n================================================\n\n");
	printf("> ");
	return 0;
}

static int init_iav(void)
{
	struct iav_querybuf querybuf;

	//open the device
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	querybuf.buf = IAV_BUFFER_DSP;
	if (ioctl(fd_iav, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
		perror("IAV_IOC_QUERY_BUF");
		return -1;
	}

	dsp_size = querybuf.length;
	dsp_mem = (u8 *)mmap(NULL, dsp_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_iav, querybuf.offset);
	if (dsp_mem == MAP_FAILED) {
		perror("mmap (%d) failed: %s\n");
		return -1;
	} else {
		mem_mapped = 1;
	}

	return 0;
}

static int exit_iav(void)
{
	if (fd_iav >= 0) {
		close(fd_iav);
	}
	if (!mem_mapped) {
		return 0;
	}
	if (munmap(dsp_mem, dsp_size) < 0) {
		printf("munmap failed!\n");
		return -1;
	} else {
		mem_mapped = 0;
	}

	return 0;
}

static int check_state(void)
{
	int state;

	if (ioctl(fd_iav, IAV_IOC_GET_IAV_STATE, &state) < 0) {
		perror("IAV_IOC_GET_STATE");
		return -1;
	}

	if ((state != IAV_STATE_ENCODING) && (state != IAV_STATE_PREVIEW)) {
		printf("IAV is not in encoding or pewview status, cannot control Smartrc!\n");
		return -1;
	}
	return 0;
}

static u32 get_ratio(u32 dividend, u32 divisor, u32 shift)
{
	return dividend / (divisor >> shift);
}

static u32 multiply_ratio(u32 value, u32 ratio, u32 shift)
{
	return (value * ratio) >> shift;
}

static int update_delay_param(u32 fps)
{
	int ratio;

	ratio = get_ratio(AMBA_VIDEO_FPS_29_97, fps, SHIFT_BIT);
	g_delay.motion_none = multiply_ratio(g_delay.motion_none, ratio, SHIFT_BIT);
	g_delay.motion_low = multiply_ratio(g_delay.motion_low, ratio, SHIFT_BIT);
	g_delay.motion_mid = multiply_ratio(g_delay.motion_mid, ratio, SHIFT_BIT);
	g_delay.motion_high = multiply_ratio(g_delay.motion_high, ratio, SHIFT_BIT);

	return 0;
}

static int get_transfer_ratio(buf_type_t type_in, buf_type_t type_out)
{
	u32 ratio;

	if (type_in == BUF_ME1_4 && type_out == BUF_ME0_8) {
		ratio = 2;
	} else if (type_in == BUF_ME1_4 && type_out == BUF_ME1_16) {
		ratio = 4;
	} else if (type_in == BUF_ME0_8 && type_out == BUF_ME0_16) {
		ratio = 2;
	} else if (type_in == BUF_ME0_8 && type_out == BUF_ME0_32) {
		ratio = 4;
	} else {
		printf("Invalid buffer type!\n");
		return -1;
	}
	return ratio;
}

static buf_type_t get_buf_type(u32 buf_id)
{
	int buf_type;

	if (buf_id < IAV_SRCBUF_1 || buf_id > IAV_SRCBUF_4) {
		printf("Invalid buffer id!\n");
		return -1;
	}

	//always use ME1 4x4 down-scaled data to do MDET
	//buf_type = BUF_ME0_16;
	buf_type = BUF_ME1_4;

	return buf_type;
}

static int get_me_buf_desc(struct iav_querydesc *query_desc, u32 buf_id)
{
	enum iav_srcbuf_id srcbuf_id;
	buf_type_t buf_type;

	if (!query_desc) {
		printf("Invalid query_desc ptr!\n");
		return -1;
	}

	srcbuf_id = (enum iav_srcbuf_id) buf_id;
	buf_type = get_buf_type(buf_id);
	switch (buf_type) {
		case BUF_ME1_4:
			query_desc->qid = IAV_DESC_ME1;
			query_desc->arg.me1.buf_id = srcbuf_id;
			query_desc->arg.me1.flag |= IAV_BUFCAP_NONBLOCK;
			break;
		case BUF_ME0_8:
		case BUF_ME0_16:
			query_desc->qid = IAV_DESC_ME0;
			query_desc->arg.me0.buf_id = srcbuf_id;
			query_desc->arg.me0.flag |= IAV_BUFCAP_NONBLOCK;
			break;
		default:
			printf("Buffer type %d not supported!\n", buf_type);
			break;
	}
	if (ioctl(fd_iav, IAV_IOC_QUERY_DESC, query_desc) < 0) {
		perror("IAV_IOC_QUERY_DESC");
		return -1;
	}
	return 0;
}

static int get_roi_size(unsigned int *p, unsigned int *w, unsigned int *h, u32 stream_id)
{
	struct iav_querydesc query_desc;
	struct iav_mebufdesc *me = NULL;
	buf_type_t buf_type;
	u32 trans_ratio = 1, buf_id;
	s32 tmp;

	if (!p || !w || !h) {
		printf("Invalid ptr for p or w or h!\n");
		return -1;
	}
	VERIFY_STREAM_ID(stream_id);
	if (!g_enc_cfg[stream_id].param_inited) {
		printf("smartrc has not been initialized!\n");
		return -1;
	}
	buf_id = g_enc_cfg[stream_id].src_buf_id;

	buf_type = get_buf_type(buf_id);

	memset(&query_desc, 0, sizeof(query_desc));
	get_me_buf_desc(&query_desc, buf_id);
	switch (buf_type) {
	case BUF_ME1_4:
		tmp = get_transfer_ratio(BUF_ME1_4, BUF_ME1_16);
		if (tmp < 0)
			break;
		trans_ratio = tmp;
		me = &query_desc.arg.me1;
		break;
	case BUF_ME0_8:
		tmp = get_transfer_ratio(BUF_ME0_8, BUF_ME0_16);
		if (tmp < 0)
			break;
		trans_ratio = tmp;
		me = &query_desc.arg.me0;
		break;
	case BUF_ME0_16:
		trans_ratio = 1;
		me = &query_desc.arg.me0;
		break;
	default:
		printf("Buffer type %d not supported!\n", buf_type);
		break;
	}

	*p = ROUND_UP(me->pitch, trans_ratio) / trans_ratio;
	*h = g_enc_cfg[stream_id].roi_height;
	*w = g_enc_cfg[stream_id].roi_width;

	return 0;
}

static inline int transfer_me_buf(u8 *buf_in, u32 pitch_in, u32 height_in, u32 width_in,
	u32 offset_w_in, u32 offset_h_in, u8 *buf_out, u32 pitch_out, u32 ratio)
{
	u32 add_num, shift;
	u32 i, j;
	int k;

	if (!buf_in || !buf_out) {
		printf("Invalid buf ptr!\n");
		return -1;
	}

	switch (ratio) {
	case 2:
		shift = 1;
		add_num = 2;
		for (j = 0; j < height_in; j += 2) {
			for (i = 0; i < width_in; i += 16) {
				uint16x4_t line00 = vpaddl_u8(vld1_u8(buf_in + (j + 0) * pitch_in + i + 0));
				uint16x4_t line01 = vpaddl_u8(vld1_u8(buf_in + (j + 0) * pitch_in + i + 8));

				uint16x4_t line10 = vpaddl_u8(vld1_u8(buf_in + (j + 1) * pitch_in + i + 0));
				uint16x4_t line11 = vpaddl_u8(vld1_u8(buf_in + (j + 1) * pitch_in + i + 8));

				uint16x4x2_t line = {{vadd_u16(line00, line10), vadd_u16(line01, line11)}};
#if defined(BUILD_AMBARELLA_APP_DEBUG)
				buf_out[(j >> shift) * pitch_out + (i >> shift) + 0] =
					(vget_lane_u16(line.val[0], 0) + add_num) >> ratio;
				buf_out[(j >> shift) * pitch_out + (i >> shift) + 1] =
					(vget_lane_u16(line.val[0], 1) + add_num) >> ratio;
				buf_out[(j >> shift) * pitch_out + (i >> shift) + 2] =
					(vget_lane_u16(line.val[0], 2) + add_num) >> ratio;
				buf_out[(j >> shift) * pitch_out + (i >> shift) + 3] =
					(vget_lane_u16(line.val[0], 3) + add_num) >> ratio;
				buf_out[(j >> shift) * pitch_out + (i >> shift) + 4] =
					(vget_lane_u16(line.val[1], 0) + add_num) >> ratio;
				buf_out[(j >> shift) * pitch_out + (i >> shift) + 5] =
					(vget_lane_u16(line.val[1], 1) + add_num) >> ratio;
				buf_out[(j >> shift) * pitch_out + (i >> shift) + 6] =
					(vget_lane_u16(line.val[1], 2) + add_num) >> ratio;
				buf_out[(j >> shift) * pitch_out + (i >> shift) + 7] =
					(vget_lane_u16(line.val[1], 3) + add_num) >> ratio;
#else
				for (k = 0; k < 8; ++ k) {
					buf_out[(j >> shift) * pitch_out + (i >> shift)+ k] =
						(vget_lane_u16(line.val[k / 4], (const int)k % 4) + add_num) >> ratio;
				}
#endif
			}
		}
		break;
	case 4:
		shift = 2;
		add_num = 8;
		for (j = 0; j < height_in; j += 4) {
			for (i = 0; i < width_in; i += 16) {
				uint16x4_t line00 = vpaddl_u8(vld1_u8(buf_in + j * pitch_in + i + 0));
				uint16x4_t line01 = vpaddl_u8(vld1_u8(buf_in + j * pitch_in + i + 8));
				uint16x4_t line0  = vpadd_u16(line00, line01);

				uint16x4_t line10 = vpaddl_u8(vld1_u8(buf_in + (j + 1) * pitch_in + i + 0));
				uint16x4_t line11 = vpaddl_u8(vld1_u8(buf_in + (j + 1) * pitch_in + i + 8));
				uint16x4_t line1  = vpadd_u16(line10, line11);

				uint16x4_t line20 = vpaddl_u8(vld1_u8(buf_in + (j + 2) * pitch_in + i + 0));
				uint16x4_t line21 = vpaddl_u8(vld1_u8(buf_in + (j + 2) * pitch_in + i + 8));
				uint16x4_t line2  = vpadd_u16(line20, line21);

				uint16x4_t line30 = vpaddl_u8(vld1_u8(buf_in + (j + 3) * pitch_in + i + 0));
				uint16x4_t line31 = vpaddl_u8(vld1_u8(buf_in + (j + 3) * pitch_in + i + 8));
				uint16x4_t line3  = vpadd_u16(line30, line31);

				uint16x4_t line = vadd_u16(vadd_u16(vadd_u16(line0, line1), line2), line3);
#if defined(BUILD_AMBARELLA_APP_DEBUG)
				buf_out[(j >> shift) * pitch_out + (i >> shift) + 0] =
						(vget_lane_u16(line, 0) + add_num) >> ratio;
				buf_out[(j >> shift) * pitch_out + (i >> shift) + 1] =
						(vget_lane_u16(line, 1) + add_num) >> ratio;
				buf_out[(j >> shift) * pitch_out + (i >> shift) + 2] =
						(vget_lane_u16(line, 2) + add_num) >> ratio;
				buf_out[(j >> shift) * pitch_out + (i >> shift) + 3] =
						(vget_lane_u16(line, 3) + add_num) >> ratio;
#else
				for (k = 0; k < 4; ++k) {
					buf_out[(j >> shift) * pitch_out + (i >> shift) + k] =
						(vget_lane_u16(line, (const int)k) + add_num) >> ratio;
				}
#endif
			}
		}
		break;
	default:
		printf("Incorrect ratio for me buffer!\n");
		return -1;
	}

	return 0;
}

static u8 *get_roi_addr(u32 stream_id)
{
	struct iav_querydesc query_desc;
	struct iav_mebufdesc *me = NULL;
	buf_type_t buf_type;
	u8 *data_addr = NULL, *buf_in = NULL, *buf_out = NULL;
	u32 pitch_in, width_in, height_in, offset_w_in, offset_h_in, pitch_out;
	u32 trans_ratio = 1, me_ratio = 1, buf_id, tmp_width, tmp_height;
	s32 tmp;

	if (stream_id < 0 || stream_id >= SMARTRC_MAX_STREAM_NUM) {
		printf("invalid stream id!\n");
		goto err;
	}
	if (!g_enc_cfg[stream_id].param_inited) {
		printf("smartrc has not been initialized!\n");
		goto err;
	}
	buf_id = g_enc_cfg[stream_id].src_buf_id;
	if (!me_buf[stream_id].buffer) {
		printf("Invalid me buffer!\n");
		goto err;
	}
	buf_type = get_buf_type(buf_id);

	memset(&query_desc, 0, sizeof(query_desc));
	get_me_buf_desc(&query_desc, buf_id);
	switch (buf_type) {
	case BUF_ME1_4:
		me_ratio = 4;
		tmp = get_transfer_ratio(BUF_ME1_4, BUF_ME1_16);
		if (tmp < 0)
			break;
		trans_ratio = tmp;
		me = &query_desc.arg.me1;
		break;
	case BUF_ME0_8:
		me_ratio = 8;
		tmp = get_transfer_ratio(BUF_ME0_8, BUF_ME0_16);
		if (tmp < 0)
			break;
		trans_ratio = tmp;
		me = &query_desc.arg.me0;
		break;
	case BUF_ME0_16:
		trans_ratio = 1;
		me = &query_desc.arg.me0;
		break;
	default:
		printf("Buffer type %d not supported!\n", buf_type);
		break;
	}

	//resolution is aligned to 16bit
	tmp_width = ROUND_UP(g_enc_cfg[stream_id].width, 16);
	tmp_height = ROUND_UP(g_enc_cfg[stream_id].height, 16);
	buf_in = (u8 *)(dsp_mem + me->data_addr_offset);
	pitch_in = me->pitch;
	height_in = ROUND_UP(tmp_height, me_ratio) / me_ratio;
	width_in = ROUND_UP(tmp_width, me_ratio) / me_ratio;
	offset_w_in = ROUND_UP(g_enc_cfg[stream_id].x, me_ratio) / me_ratio;
	offset_h_in = ROUND_UP(g_enc_cfg[stream_id].y, me_ratio) / me_ratio;
	buf_out = me_buf[stream_id].buffer;
	pitch_out = ROUND_UP(pitch_in, trans_ratio) / trans_ratio;

	transfer_me_buf(buf_in, pitch_in, height_in, width_in,
		offset_w_in, offset_h_in, buf_out, pitch_out, trans_ratio);
	data_addr = buf_out;

err:
	return data_addr;
}

static u32 get_iav_dsp_pts(void)
{
	struct iav_querydesc sync_desc;

	memset(&sync_desc, 0, sizeof(sync_desc));
	sync_desc.qid = IAV_DESC_YUV;
	sync_desc.arg.yuv.buf_id = IAV_SRCBUF_MN;
	sync_desc.arg.yuv.flag &= ~IAV_BUFCAP_NONBLOCK;

	if (ioctl(fd_iav, IAV_IOC_QUERY_DESC, &sync_desc) < 0) {
		perror("IAV_IOC_QUERY_DESC");
		return -1;
	}

	return (sync_desc.arg.yuv.dsp_pts);
}

static int do_vca_on_yuv(u32 stream_id)
{
	struct iav_querydesc sync_desc;
	struct iav_yuvbufdesc *yuv = NULL;
	u8 *luma_addr = NULL, *start = NULL;
	int i, j, k, ratio = MB_SIZE;
	u8 *matrix = NULL;
	u32 w, h, start_x, start_y;

	VERIFY_STREAM_ID(stream_id);
	if (!g_enc_cfg[stream_id].param_inited) {
		printf("smartrc has not been initialized!\n");
		return -1;
	}
	matrix = g_smartrc_session[stream_id].motion_matrix;

	w = ROUND_UP(g_enc_cfg[stream_id].width, ratio);
	h = ROUND_UP(g_enc_cfg[stream_id].height, ratio);
	//x and y have already been a multiple of ratio, otherwise it cannot use smartrc
	start_x = g_enc_cfg[stream_id].x;
	start_y = g_enc_cfg[stream_id].y;

	memset(&sync_desc, 0, sizeof(sync_desc));
	yuv = &sync_desc.arg.yuv;
	sync_desc.qid = IAV_DESC_YUV;
	yuv->flag |= IAV_BUFCAP_NONBLOCK;
	yuv->buf_id = g_enc_cfg[stream_id].src_buf_id;

	if (ioctl(fd_iav, IAV_IOC_QUERY_DESC, &sync_desc) < 0) {
		perror("IAV_IOC_QUERY_DESC");
		return -1;
	}

	luma_addr = dsp_mem + yuv->y_addr_offset;
	for (i = start_y; i < start_y + h; i += ratio) {
		for (j = start_x; j < start_x + w; j += ratio, matrix++) {
			start = luma_addr + i * yuv->pitch + j;
			if (*matrix == 1) {
				for (k = 0; k < ratio; k++) {
					memset(start + k * yuv->pitch, 0xff, ratio);
				}
			}
		}
	}

	return 0;
}

static int dump_mot_mat(u32 stream_id)
{
	u8 *motion_matrix = NULL;
	u32 w, h, i, j;
	char *ch;

	VERIFY_STREAM_ID(stream_id);
	if (g_mt_mat[stream_id].fd < 0 ||
		!g_mt_mat[stream_id].buffer ||
		!g_mt_mat[stream_id].size) {
		printf("invalid dump fd or buffer or size!\n");
		return -1;
	}
	if (!g_enc_cfg[stream_id].param_inited) {
		printf("smartrc has not been initialized!\n");
		return -1;
	}

	motion_matrix = g_smartrc_session[stream_id].motion_matrix;
	w = g_enc_cfg[stream_id].roi_width;
	h = g_enc_cfg[stream_id].roi_height;

	ch = g_mt_mat[stream_id].buffer;
	// QP matrix is MB level. One MB is 16x16 pixels.
	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++, motion_matrix++, ch++) {
			if (*motion_matrix == 1) {
				*ch = '*'; // '*' for '1'
			} else {
				*ch = ' '; // space for '0'
			}
		}
		*ch = '\n';
		ch++;
	}
	*ch = '\n';
	ch++;
	if (amba_transfer_write(g_mt_mat[stream_id].fd, g_mt_mat[stream_id].buffer,
		g_mt_mat[stream_id].size, TRANS_METHOD_NFS) < 0) {
		perror("Failed to dump motion matrix into file!\n");
		return -1;
	}

	return 0;
}

static int get_noise_value(void)
{
	struct vindev_agc vsrc_agc;

	vsrc_agc.vsrc_id = 0;
	if (ioctl(fd_iav, IAV_IOC_VIN_GET_AGC, &vsrc_agc) < 0) {
		perror("IAV_IOC_VIN_GET_AGC");
		return -1;
	}

	return (vsrc_agc.agc >> 24);
}

static int get_motion_value_start(u32 stream_id)
{
	int ret = 0;
	u32 p, w, h;
	mdet_cfg *mdet_cfg = NULL;
	mdet_session_t *ms = NULL;

	VERIFY_STREAM_ID(stream_id);
	do {
		if (get_roi_size(&p, &w, &h, stream_id) < 0) {
			printf("get roi size failed!\n");
			ret = -1;
			break;
		}
		printf("stream: %c p %d w %d h %d\n", 'A' + stream_id, p, w, h);

		mdet_cfg = &g_mdet_cfg[stream_id];
		mdet_cfg->threshold = 2; //64->2
		mdet_cfg->fm_dim.pitch = p;
		mdet_cfg->fm_dim.width = w;
		mdet_cfg->fm_dim.height = h;
		// config one ROI0 to cover the whole buffer
		mdet_cfg->roi_info.num_roi = 1;
		mdet_cfg->roi_info.roi[0].num_points = 4;
		mdet_cfg->roi_info.roi[0].points[0].x = 0;
		mdet_cfg->roi_info.roi[0].points[0].y = 0;
		mdet_cfg->roi_info.roi[0].points[1].x = w - 1;
		mdet_cfg->roi_info.roi[0].points[1].y = 0;
		mdet_cfg->roi_info.roi[0].points[2].x = w - 1;
		mdet_cfg->roi_info.roi[0].points[2].y = h - 1;
		mdet_cfg->roi_info.roi[0].points[3].x = 0;
		mdet_cfg->roi_info.roi[0].points[3].y = h - 1;

		ms = &g_ms[stream_id];
		//start motion detection
		if (mdet_start_diff(mdet_cfg, ms) < 0) {
			printf("md_start failed!\n");
			ret = -1;
			break;
		}
	} while (0);

	return ret;
}

static int get_motion_value_stop(u32 stream_id)
{
	mdet_cfg *mdet_cfg = NULL;
	mdet_session_t *ms = NULL;
	VERIFY_STREAM_ID(stream_id);

	mdet_cfg = &g_mdet_cfg[stream_id];
	ms = &g_ms[stream_id];
	mdet_stop_diff(mdet_cfg, ms);

	return 0;
}

static int smartrc_thread_stop(void)
{
	int ret;

	if (smartrc_thread_id == 0) {
		ret = 0;
	} else {
		smartrc_exit_flag = 1;
		ret = pthread_join(smartrc_thread_id, NULL);
		smartrc_thread_id = 0;
	}
	return ret;
}

static void *smartrc_mainloop(void *arg)
{
	u64 total_frames = 0;
	u8 *data = NULL;
	u32 i0_motion[SMARTRC_MAX_STREAM_NUM] = {0}, i1_motion[SMARTRC_MAX_STREAM_NUM] = {0};
	u32 i2_motion[SMARTRC_MAX_STREAM_NUM] = {0}, i3_motion[SMARTRC_MAX_STREAM_NUM] = {0};
	u32 i0_light[SMARTRC_MAX_STREAM_NUM] = {0}, i1_light[SMARTRC_MAX_STREAM_NUM] = {0};
	u32 i2_light[SMARTRC_MAX_STREAM_NUM] = {0};
	motion_level_t motion_level[SMARTRC_MAX_STREAM_NUM] = {MOTION_LOW};
	noise_level_t noise_level[SMARTRC_MAX_STREAM_NUM] = {NOISE_LOW};
	motion_level_t real_motion_level[SMARTRC_MAX_STREAM_NUM];
	noise_level_t real_noise_level[SMARTRC_MAX_STREAM_NUM];
	quality_level_t quality_level[SMARTRC_MAX_STREAM_NUM];
	struct timeval prev = {0, 0}, curr = {0, 0};
	u32 tmp, dsp_pts, quality_level_flag[SMARTRC_MAX_STREAM_NUM] = {0};
	int i;

	while (smartrc_exit_flag == 0) {
		//blocking IO for getting dsp pts, other IOs are all non-blocking
		dsp_pts = get_iav_dsp_pts();
		tmp = get_noise_value();
		total_frames++;
		for (i = 0; i < SMARTRC_MAX_STREAM_NUM; i++) {
			if (!(stream_selection & (1 << i)))
				continue;

			if (smartrc_params.adaptive_scenario_flag[i]
					&& smartrc_params.adaptive_scenario[i]) {
				gettimeofday(&curr, NULL);
				prev = curr;
			}

			noise_value[i] = tmp;
			data = get_roi_addr(i);
			mdet_update_frame_diff(&g_mdet_cfg[i], &g_ms[i], data, g_mdet_cfg[i].threshold);
			motion_value[i] = (int) ((double) g_ms[i].motion[0] * (double) g_delay.motion_indicator);

			//printf("motion[0]: %f, fg_pxls[0]: %d, pixels[0]: %d, motion_value: %d, noise_value: %d\n",
			//	g_ms.motion[0], g_ms.fg_pxls[0], g_ms.pixels[0], motion_value, noise_value);
			if (motion_value[i] < g_threshold.motion_low) { // no motion
				i0_motion[i]++;
				i1_motion[i] = 0;
				i2_motion[i] = 0;
				i3_motion[i] = 0;
				if (i0_motion[i] >= g_delay.motion_none) {
					if (motion_level[i] == MOTION_HIGH) {
						motion_level[i] = MOTION_MID;
					} else if (motion_level[i] == MOTION_MID) {
						motion_level[i] = MOTION_LOW;
					} else {
						motion_level[i] = MOTION_NONE;
					}
					i0_motion[i] = 0;
				}
			} else if (motion_value[i] < g_threshold.motion_mid) { // small motion
				i0_motion[i] = 0;
				i1_motion[i]++;
				i2_motion[i] = 0;
				i3_motion[i] = 0;
				if (i1_motion[i] >= g_delay.motion_low) {
					motion_level[i] = MOTION_LOW;
					i1_motion[i] = 0;
				}
			} else if (motion_value[i] < g_threshold.motion_high) { // middle motion
				i0_motion[i] = 0;
				i1_motion[i] = 0;
				i2_motion[i]++;
				i3_motion[i] = 0;
				if (i2_motion[i] >= g_delay.motion_mid) {
					motion_level[i] = MOTION_MID;
					i2_motion[i] = 0;
				}
			} else { // big motion
				i0_motion[i] = 0;
				i1_motion[i] = 0;
				i2_motion[i] = 0;
				i3_motion[i]++;
				if (i3_motion[i] >= g_delay.motion_high) {
					motion_level[i] = MOTION_HIGH;
					i3_motion[i] = 0;
					if (smartrc_params.adaptive_scenario_flag[i]
							&& smartrc_params.adaptive_scenario[i])
						motion_high_cnt[i]++;
				}
			}

			if (noise_value[i] < g_threshold.noise_low) {
				i0_light[i]++;
				i1_light[i] = 0;
				i2_light[i] = 0;
				if (i0_light[i] >= g_delay.noise_none) {
					noise_level[i] = NOISE_NONE;
					i0_light[i] = 0;
				}
			} else if (noise_value[i] < g_threshold.noise_high) {
				i0_light[i] = 0;
				i1_light[i]++;
				i2_light[i] = 0;
				if (i1_light[i] >= g_delay.noise_low) {
					noise_level[i] = NOISE_LOW;
					i1_light[i] = 0;
				}
			} else {
				i0_light[i] = 0;
				i1_light[i] = 0;
				i2_light[i]++;
				if (i2_light[i] >= g_delay.noise_high) {
					noise_level[i] = NOISE_HIGH;
					i2_light[i] = 0;
				}
			}

			if (smartrc_params.adaptive_scenario_flag[i]
					&& smartrc_params.adaptive_scenario[i]) {
				gettimeofday(&curr, NULL);
				if ((curr.tv_sec - prev.tv_sec) >= ADAPTIVE_STUDY_TIME) {
					if (motion_high_cnt[i] >= ADAPTIVE_MOTION_HIGH_THRESHOLD) {
						if (!quality_level_flag[i]) {
							smartrc_get_stream_quality(&quality_level[i], i);
							if (quality_level[i] < QUALITY_LAST - 1) {
								quality_level[i] += 1;
							}
							quality_level_flag[i] = 1;
							smartrc_set_stream_quality(quality_level[i], i);
							printf("update to quality %d.\n", quality_level[i]);
						}
					} else {
						if (quality_level_flag[i]) {
							smartrc_get_stream_quality(&quality_level[i], i);
							if (quality_level[i] > QUALITY_FIRST) {
								quality_level[i] -= 1;
							}
							quality_level_flag[i] = 0;
							smartrc_set_stream_quality(quality_level[i], i);
							printf("return to quality %d.\n", quality_level[i]);
						}
					}
					motion_high_cnt[i] = 0;
					prev = curr;
				}
			}
			//printf("current motion level = %d, current noise level = %d\n", motion_level, noise_level);
			smartrc_get_motion_level(&real_motion_level[i], i);
			smartrc_get_noise_level(&real_noise_level[i], i);
			if (motion_level[i] != real_motion_level[i]) {
				smartrc_set_motion_level(motion_level[i], i);
			}
			if (noise_level[i] != real_noise_level[i]) {
				smartrc_set_noise_level(noise_level[i], i);
			}

			//ROI
			roi_session_t *session;
			session = &g_smartrc_session[i];
			session->motion_matrix = g_ms[i].fg;
			session->dsp_pts = dsp_pts;
			if (smartrc_params.roi_interval_flag[i]) {
				if (total_frames % smartrc_params.roi_interval[i] == 0) {
					smartrc_update_roi(session, i);
				}
			} else {
				smartrc_update_roi(session, i);
			}

			if (smartrc_params.vca_on_yuv_flag &&
					smartrc_params.vca_on_yuv) {
				do_vca_on_yuv(i);
			}
			if (smartrc_params.dump_mot_mat_flag[i] &&
					smartrc_params.dump_mot_mat[i]) {
				dump_mot_mat(i);
			}
			if (smartrc_params.dump_para_cfg_flag[i] &&
					smartrc_params.dump_para_cfg[i]) {
				if (smartrc_get_para_cfg(g_param_cfg[i].buffer, g_param_cfg[i].size,
					dsp_pts, i) < 0) {
					printf("Failed to get parameter config!\n");
					return NULL;
				}
				if (amba_transfer_write(g_param_cfg[i].fd, g_param_cfg[i].buffer,
					g_param_cfg[i].size, TRANS_METHOD_NFS) < 0) {
					printf("Failed to dump para cfg into file!\n");
					return NULL;
				}
			}
		}

		if (smartrc_apply_sync_frame(dsp_pts) < 0) {
			perror("apply_sync_frame");
			return NULL;
		}
	}
	return NULL;
}

static int smartrc_thread_start(void)
{
	int ret = 0;
	smartrc_exit_flag = 0;

	if (smartrc_thread_id == 0) {
		ret = pthread_create(&smartrc_thread_id, NULL, smartrc_mainloop, NULL);
		if (ret) {
			perror("smartrc_control pthread create failed!\n");
		}
	}
	return ret;
}

static int smartrc_auto_run_test()
{
	int i;
	printf("Start smartrc auto control, do not manually set noise/motion level!\n");
	for (i = 0; i < SMARTRC_MAX_STREAM_NUM; i++) {
		if (!(stream_selection & (1<<i)))
			continue;

		if (get_motion_value_start(i) < 0) {
			printf("Smartrc: get motion level start failed!\n");
			return -1;
		}
	}
	if (smartrc_thread_start() < 0) {
		printf("Smartrc: control thread start failed!\n");
		return -1;
	}
	return 0;
}

static int init_smartrc_lib(void)
{
	int i;
	char file_name[256];
	version_t version;
	init_t init_param;
	param_config_t config;
	bitrate_target_t target;
	roi_session_t *session = NULL;
	struct iav_system_resource resource;
	struct iav_stream_info info;
	struct iav_stream_format format;
	struct iav_h264_cfg h26x_config;
	struct vindev_fps vsrc_fps;
	u32 roi_w, roi_h, roi_p;

	if (check_state() < 0) {
		return -1;
	}
	if (smartrc_get_version(&version) < 0) {
		return -1;
	} else {
		printf("\nSmart Rate Control Library Version: %s-%d.%d.%d (Last updated: %x)\n\n",
			version.description, version.major, version.minor, version.patch, version.mod_time);
	}

	memset(&init_param, 0, sizeof(init_param));
	init_param.fd_iav = fd_iav;
	if (smartrc_init(&init_param) < 0) {
		printf("Smartrc: init failed!\n");
		return -1;
	}

	memset(&config, 0, sizeof(param_config_t));
	for (i = 0; i < SMARTRC_MAX_STREAM_NUM; i++) {
		if (!(stream_selection & (1<<i)))
			continue;

		//do basic check on the stream
		memset(&resource, 0, sizeof(resource));
		resource.encode_mode = DSP_CURRENT_MODE;
		AM_IOCTL(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &resource);
		if (!resource.enc_dummy_latency) {
			printf("Please configure encode dummy latency with test_encode first,"
				"and the value should be in range of 2 ~ 5\n");
			return -1;
		}

		//check the input stream id to see whether it's in encoding state
		info.id = i;
		AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_INFO, &info);
		if (info.state != IAV_STREAM_STATE_ENCODING) {
			printf("stream %d state invalid\n", i);
			return -1;
		}
		format.id = i;
		AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_FORMAT, &format);
		if (format.type != IAV_STREAM_TYPE_H264) {
			printf("stream %d is not H.264\n", i);
			return -1;
		}
		if (format.type == IAV_STREAM_TYPE_H264) {
			if ((format.enc_win.x & 0x0F) || (format.enc_win.y & 0x0F)) {
				printf("stream offset x: %d, y: %d must be a mutiple of 16 for smartrc\n",
					format.enc_win.x, format.enc_win.y);
				return -1;
			}
		}
		config.enc_cfg.codec_type = format.type;

		//check encode width/height valid
		VERIFY_ENCODE_RESOLUTION(format.enc_win.width, format.enc_win.height);
		config.stream_id = i;
		if (format.rotate_cw == 0) {
			config.enc_cfg.width = format.enc_win.width;
			config.enc_cfg.height = format.enc_win.height;
			config.enc_cfg.x = format.enc_win.x;
			config.enc_cfg.y = format.enc_win.y;
		} else {
			config.enc_cfg.width = format.enc_win.height;
			config.enc_cfg.height = format.enc_win.width;
			config.enc_cfg.x = format.enc_win.y;
			config.enc_cfg.y = format.enc_win.x;
		}
		config.enc_cfg.roi_width = ROUND_UP(config.enc_cfg.width, MB_SIZE) / MB_SIZE;
		config.enc_cfg.roi_height = ROUND_UP(config.enc_cfg.height, MB_SIZE) / MB_SIZE;


		g_enc_cfg[i].codec_type = format.type;
		g_enc_cfg[i].src_buf_id = format.buf_id;
		g_enc_cfg[i].width = config.enc_cfg.width;
		g_enc_cfg[i].height = config.enc_cfg.height;
		g_enc_cfg[i].x = config.enc_cfg.x;
		g_enc_cfg[i].y = config.enc_cfg.y;
		g_enc_cfg[i].roi_width = config.enc_cfg.roi_width;
		g_enc_cfg[i].roi_height = config.enc_cfg.roi_height;
		g_enc_cfg[i].param_inited = 1;

		//cfg sync frame roi param
		memset(&h26x_config, 0, sizeof(h26x_config));
		h26x_config.id = i;
		AM_IOCTL(fd_iav, IAV_IOC_GET_H264_CONFIG, &h26x_config);
		config.enc_cfg.M = h26x_config.M;
		config.enc_cfg.N = h26x_config.N;

		if (smartrc_params.bitrate_gap_adjust_flag[i]) {
			config.bitrate_gap_adj = smartrc_params.bitrate_gap_adjust[i];
		}

		vsrc_fps.vsrc_id = 0;
		AM_IOCTL(fd_iav, IAV_IOC_VIN_GET_FPS, &vsrc_fps);
		config.enc_cfg.fps = vsrc_fps.fps;

		if (update_delay_param(vsrc_fps.fps) < 0) {
			printf("Update delay param failed!\n");
			return -1;
		}

		if (smartrc_param_config(&config) < 0) {
			printf("Smartrc: init param failed!\n");
			return -1;
		}

		//ROI
		session = &g_smartrc_session[i];
		if (smartrc_start_roi(session, i) < 0) {
			printf("Smartrc: session start failed!\n");
			return -1;
		}

		if (get_roi_size(&roi_p, &roi_w, &roi_h, i) < 0) {
			printf("get roi size failed!\n");
			return -1;
		}
		me_buf[i].size = roi_p * roi_h * sizeof(u8);
		me_buf[i].buffer = malloc(me_buf[i].size);
		if (me_buf[i].buffer == NULL) {
			printf("me1 buffer malloc failed!\n");
			return -1;
		}

		// dump motion matrix
		if (smartrc_params.dump_mot_mat_flag[i] &&
			smartrc_params.dump_mot_mat[i]) {
			if (g_mt_mat[i].file[0] == '\0') {
				sprintf(file_name, "%s_stream_%c.dat",
					default_mt_mat_file, 'A' + i);
				strcpy(g_mt_mat[i].file, file_name);
			}
			g_mt_mat[i].fd = amba_transfer_open(g_mt_mat[i].file, TRANS_METHOD_NFS, MM_DUMP_BASE + i);
			if (g_mt_mat[i].fd < 0) {
				printf("Cannot open file [%s].\n", g_mt_mat[i].file);
				return -1;
			}
			g_mt_mat[i].size = (roi_w + 1) * (roi_h + 1); // extra 1 row and column for '\n'
			g_mt_mat[i].buffer = (char *)malloc(g_mt_mat[i].size * sizeof(char));
			if (!g_mt_mat[i].buffer) {
				printf("Not enough memory for dump buffer!\n");
				return -1;
			} else {
				memset(g_mt_mat[i].buffer, 0, g_mt_mat[i].size*sizeof(char));
			}
		}

		//dump parameter configuration
		if (smartrc_params.dump_para_cfg_flag[i] &&
				smartrc_params.dump_para_cfg[i]) {
			if (g_param_cfg[i].file[0] == '\0') {
				sprintf(file_name, "%s_stream_%c.cfg",
					default_param_file, 'A' + i);
				strcpy(g_param_cfg[i].file, file_name);
			}
			g_param_cfg[i].fd = amba_transfer_open(g_param_cfg[i].file, TRANS_METHOD_NFS, PC_DUMP_BASE + i);
			if (g_param_cfg[i].fd < 0) {
				printf("Cannot open file [%s].\n", g_param_cfg[i].file);
				return -1;
			}
			g_param_cfg[i].size = PC_DUMP_SIZE;
			g_param_cfg[i].buffer = (char *) malloc(g_param_cfg[i].size * sizeof(char));
			if (!g_param_cfg[i].buffer) {
				printf("Not enough memory for dump buffer!\n");
				return -1;
			} else {
				memset(g_param_cfg[i].buffer, 0, g_param_cfg[i].size * sizeof(char));
			}
		}

		//apply initial cmdline setting here
		if (smartrc_params.stream_quality_flag[i]) {
			if (smartrc_set_stream_quality(smartrc_params.stream_quality[i], i) < 0) {
				printf("Smartrc: set stream quality %d for stream %d failed!\n",
					smartrc_params.stream_quality[i], i);
			}
		}
		if (smartrc_params.bitrate_ceiling_flag[i]) {
			target.auto_target = 0;
			target.bitrate_ceiling = smartrc_params.bitrate_ceiling[i];
			if (smartrc_set_bitrate_target(&target, i) < 0) {
				printf("Smartrc: set bitrate ceiling %d for stream %d failed\n",
					smartrc_params.bitrate_ceiling[i], i);
			}
		}

		if (smartrc_params.style_flag[i]) {
			if (smartrc_set_style(smartrc_params.style[i], i) < 0) {
				printf("Smartrc: set style %d for stream %d failed\n",
					smartrc_params.style[i], i);
			}
		}
		if (smartrc_params.motion_level_flag[i]) {
			if (smartrc_set_motion_level(smartrc_params.motion_level[i], i) < 0) {
				printf("Smartrc: set motion level %d for stream %d failed!\n",
					smartrc_params.motion_level[i], i);
			}
		}
		if (smartrc_params.noise_level_flag[i]) {
			if (smartrc_set_noise_level(smartrc_params.noise_level[i], i) < 0) {
				printf("Smartrc: set noise level %d for stream %d failed!\n",
					smartrc_params.noise_level[i], i);
			}
		}
	}
	return 0;
}

static int deinit_smartrc_lib(void)
{
	int i;
	roi_session_t *session = NULL;

	smartrc_thread_stop();
	for (i = 0; i < SMARTRC_MAX_STREAM_NUM; i++) {
		if (!(stream_selection & (1<<i)))
			continue;

		session = &g_smartrc_session[i];
		smartrc_stop_roi(session, i);
		get_motion_value_stop(i);
		if (me_buf[i].buffer) {
			free(me_buf[i].buffer);
			me_buf[i].buffer = NULL;
		}
		if (smartrc_params.dump_para_cfg[i]) {
			amba_transfer_close(g_param_cfg[i].fd, TRANS_METHOD_NFS);
			if (g_param_cfg[i].buffer) {
				free(g_param_cfg[i].buffer);
				g_param_cfg[i].buffer = NULL;
			}
		}
		if (smartrc_params.dump_mot_mat[i]) {
			amba_transfer_close(g_mt_mat[i].fd, TRANS_METHOD_NFS);
			if (g_mt_mat[i].buffer) {
				free(g_mt_mat[i].buffer);
				g_mt_mat[i].buffer = NULL;
			}
		}
		g_enc_cfg[i].param_inited = 0;
		smartrc_set_motion_level(MOTION_HIGH, i);
		smartrc_set_noise_level(NOISE_NONE, i);
	}
	smartrc_deinit();
	exit_iav();
	printf("Quit test smartrc\n");

	return 0;
}

static void sigstop(int signo)
{
	deinit_smartrc_lib();
	exit(0);
}

int main(int argc, char **argv)
{
	char ch, error_opt;
	int quit_flag;

	signal(SIGINT, sigstop);
	signal(SIGTERM, sigstop);
	signal(SIGQUIT, sigstop);

	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0) {
		return -1;
	}

	if (init_iav() < 0) {
		return -1;
	}

	if (smartrc_params.log_level_flag) {
		if (smartrc_set_log_level(smartrc_params.log_level) < 0) {
			printf("Set log level failed!\n");
		}
	}

	if (init_smartrc_lib() < 0 ) {
		return -1;
	}

	if (smartrc_auto_run_test() < 0) {
		return -1;
	}

	//interactive
	show_menu();
	ch = getchar();
	while (ch) {
		quit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 's':
			show_param_setting();
			break;
		case 'q':
			quit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (quit_flag)
			break;
		if (error_opt == 0)
			show_menu();
		ch = getchar();
	}

	if (deinit_smartrc_lib() < 0) {
		return -1;
	}

	return 0;
}
