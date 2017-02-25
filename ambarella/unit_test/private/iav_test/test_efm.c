/*
 * test_efm.c
 *
 * History:
 *  2015/07/01 - [Zhaoyang Chen] created file
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
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>
#include <signal.h>
#include <basetypes.h>
#include <pthread.h>
#include <semaphore.h>

#include "iav_ioctl.h"

typedef enum {
	YUV420_IYUV = 0,	// Pattern: YYYYYYYYUUVV
	YUV420_YV12 = 1,	// Pattern: YYYYYYYYVVUU
	YUV420_NV12 = 2,	// Pattern: YYYYYYYYUVUV
	YUV422_YU16 = 3,	// Pattern: YYYYYYYYUUUUVVVV
	YUV422_YV16 = 4,	// Pattern: YYYYYYYYVVVVUUUU
	YUV422_NV16 = 5,	// Pattern: YYYYYYYYUVUVUVUV
	RGB_RAW = 6,		// Bayer pattern RGB
	YUV_FORMAT_TOTAL_NUM,
	YUV_FORMAT_FIRST = YUV420_IYUV,
	YUV_FORMAT_LAST = YUV_FORMAT_TOTAL_NUM,
} YUV_FORMAT;

typedef enum {
	EFM_TYPE_YUV_FILE = 0,
	EFM_TYPE_YUV_CAPTURE = 1,
	EFM_TYPE_RAW_YUV = 2,
	EFM_TYPE_RAW_RGB = 3,
	EFM_TYPE_TOTAL_NUM,
	EFM_TYPE_FIRST = EFM_TYPE_YUV_FILE,
	EFM_TYPE_LAST = EFM_TYPE_TOTAL_NUM - 1,
} EFM_TYPE;

typedef enum {
	MEM_CPY_TYPE_GDMA = 0,
	MEM_CPY_TYPE_ARM = 1,
} MEM_CPY_TYPE;

#define	NO_ARG			0
#define	HAS_ARG			1

#define AUDIO_CLK		90000
#define RAW_BUF_NUM 	2

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

static int fd_iav = -1;
static int fd_yuv = -1;
static int fd_me1 = -1;
static int fd_raw = -1;
static int yuv_format = YUV420_NV12;
static char yuv_name[256];
static char me1_name[256];
static char raw_name[256];
static int frame_num = -1;
static struct iav_window frame_size;
static int type = 0;
static int frame_rate = -1;
static int buffer = 0;
static int verbose = 0;
static int exit_efm = 0;
static int mem_cpy_type = MEM_CPY_TYPE_GDMA;
static int me1_from_file = 0;

static u32 dsp_partition_map = 0;
static u8* dsp_mem;
static u32 dsp_size;
static u8* dsp_src_yuv_mem = NULL;
static u32 dsp_src_yuv_size = 0;
static u8* dsp_src_me1_mem = NULL;
static u32 dsp_src_me1_size = 0;
static u8* dsp_efm_yuv_mem = NULL;
static u32 dsp_efm_yuv_size = 0;
static u8* dsp_efm_me1_mem = NULL;
static u32 dsp_efm_me1_size = 0;
static u8* user_mem;
static u32 user_size;
static u32 user_buf_size;
static u32 raw_batch_num;

static u8* luma_buf = NULL;
static u8* me1_buf = NULL;

static sem_t write_sem;
static sem_t read_sem;
static u32 raw_buf_rd_idx;
static u32 raw_buf_wr_idx;
static pthread_t raw_thread_id;


struct iav_efm_get_pool_info efm_pool_info;

static struct option long_options[] = {
	{"type",	HAS_ARG,	0,	't'},
	{"buffer",	HAS_ARG,	0,	'b'},
	{"yuv-format",	HAS_ARG,	0,	'f'},
	{"yuv-input",	HAS_ARG,	0,	'y'},
	{"raw-input",	HAS_ARG,	0,	'i'},
	{"me1-input",	HAS_ARG,	0,	'm'},
	{"frame-num",	HAS_ARG,	0,	'n'},
	{"frame-size",	HAS_ARG,	0,	's'},
	{"frame-rate",	HAS_ARG,	0,	'r'},
	{"copy-method",	HAS_ARG,	0,	'c'},
	{"verbose",	NO_ARG,	0,	'v'},
	{0,	0,	0,	0},
};

static const char *short_options = "b:c:f:i:m:n:r:s:t:vy:";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"type", "\t\tSpecify the supported EFM type, 0: YUV File, 1: YUV Capture, 2: YUV RAW, 3: RGB RAW"},
	{"buffer", "\t\tSpecify the source buffer for YUV Capture"},
	{"yuv_format", "\tSpecify YUV format of the YUV input file for YUV File"},
	{"yuv_input", "\tSpecify YUV input file path for YUV File"},
	{"raw_input", "\tSpecify raw input (RGB or YUV) file path"},
	{"me1_input", "\tSpecify ME1 input file path for YUV File"},
	{">0", "\t\tSpecify frame number to be encoded"},
	{"resolution", "\tSpecify the frame resolution for YUV/RAW file"},
	{"frame_rate", "\tSpecify the frame rate of YUV input file for YUV File"},
	{"copy method", "\tSpecify the copy method for YUV Capture, 0: GDMA, 1: ARM"},
	{"", "\t\t\tPrint more information\n"},
};

static int map_efm_buffer(void)
{
	struct iav_query_subbuf query;

	query.buf_id = IAV_BUFFER_DSP;
	query.sub_buf_map = dsp_partition_map;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_SUB_BUF, &query);

	if (query.sub_buf_map & (1 << IAV_DSP_SUB_BUF_EFM_YUV)) {
		dsp_efm_yuv_size = query.length[IAV_DSP_SUB_BUF_EFM_YUV];
		dsp_efm_yuv_mem = mmap(NULL, dsp_efm_yuv_size, PROT_READ, MAP_SHARED,
			fd_iav, query.daddr[IAV_DSP_SUB_BUF_EFM_YUV]);
		if (dsp_efm_yuv_mem == MAP_FAILED) {
			perror("mmap (%d) failed: %s\n");
			return -1;
		}
	}
	if (query.sub_buf_map & (1 << IAV_DSP_SUB_BUF_EFM_ME1)) {
		dsp_efm_me1_size = query.length[IAV_DSP_SUB_BUF_EFM_ME1];
		dsp_efm_me1_mem = mmap(NULL, dsp_efm_me1_size, PROT_READ, MAP_SHARED,
			fd_iav, query.daddr[IAV_DSP_SUB_BUF_EFM_YUV]);
		if (dsp_efm_me1_mem == MAP_FAILED) {
			perror("mmap (%d) failed: %s\n");
			return -1;
		}
	}

	return 0;
}

static int map_src_buffer(u32 yuv_idx, u32 me1_idx)
{
	struct iav_query_subbuf query;

	query.buf_id = IAV_BUFFER_DSP;
	query.sub_buf_map = dsp_partition_map;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_SUB_BUF, &query);

	if (query.sub_buf_map & (1 << yuv_idx)) {
		dsp_src_yuv_size = query.length[yuv_idx];
		dsp_src_yuv_mem = mmap(NULL, dsp_efm_yuv_size, PROT_READ, MAP_SHARED,
				fd_iav, query.daddr[yuv_idx]);
		if (dsp_src_yuv_mem == MAP_FAILED) {
			perror("mmap (%d) failed: %s\n");
			return -1;
		}
	}
	if (query.sub_buf_map & (1 << me1_idx)) {
		dsp_src_me1_size = query.length[me1_idx];
		dsp_src_me1_mem = mmap(NULL, dsp_src_me1_size, PROT_READ, MAP_SHARED,
				fd_iav, query.daddr[me1_idx]);
		if (dsp_src_me1_mem == MAP_FAILED) {
			perror("mmap (%d) failed: %s\n");
			return -1;
		}
	}

	return 0;
}

static int map_dsp_buffer(void)
{
	struct iav_querybuf querybuf;

	querybuf.buf = IAV_BUFFER_DSP;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_BUF, &querybuf);

	dsp_size = querybuf.length;
	dsp_mem = mmap(NULL, dsp_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_iav,
		querybuf.offset);
	if (dsp_mem == MAP_FAILED) {
		perror("mmap (%d) failed: %s\n");
		return -1;
	}

	return 0;
}

static u32 get_dsp_sub_buf_idx(u32 buf_id, u32 is_yuv)
{
	u32 sub_buf_idx = 0;

	switch (buf_id) {
	case IAV_SRCBUF_MN:
		if (is_yuv) {
			sub_buf_idx = IAV_DSP_SUB_BUF_MAIN_YUV;
		} else {
			sub_buf_idx = IAV_DSP_SUB_BUF_MAIN_ME1;
		}
		break;
	case IAV_SRCBUF_PC:
		if (is_yuv) {
			sub_buf_idx = IAV_DSP_SUB_BUF_PREV_C_YUV;
		} else {
			sub_buf_idx = IAV_DSP_SUB_BUF_PREV_C_ME1;
		}
		break;
	case IAV_SRCBUF_PB:
		if (is_yuv) {
			sub_buf_idx = IAV_DSP_SUB_BUF_PREV_B_YUV;
		} else {
			sub_buf_idx = IAV_DSP_SUB_BUF_PREV_B_ME1;
		}
		break;
	case IAV_SRCBUF_PA:
		if (is_yuv) {
			sub_buf_idx = IAV_DSP_SUB_BUF_PREV_A_YUV;
		} else {
			sub_buf_idx = IAV_DSP_SUB_BUF_PREV_A_ME1;
		}
		break;
	default:
		printf("Incorrect buffer id %d for DSP sub buffer!\n", buf_id);
		break;
	}

	return sub_buf_idx;
}

static int map_buf_for_yuv(void)
{
	struct iav_system_resource resource;
	u32 idx_yuv, idx_me1;
	int ret = 0;

	memset(&resource, 0, sizeof(resource));
	resource.encode_mode = DSP_CURRENT_MODE;
	if (ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &resource) < 0) {
		perror("IAV_IOC_GET_SYSTEM_RESOURCE");
		return -1;
	}

	dsp_partition_map = resource.dsp_partition_map;

	if (!dsp_partition_map) {
		ret = map_dsp_buffer();
	} else {
		if (!(dsp_partition_map & 1 << IAV_DSP_SUB_BUF_EFM_YUV) ||
			!(dsp_partition_map & 1 << IAV_DSP_SUB_BUF_EFM_ME1)) {
			printf("Please enable EFM partition for dsp partition!\n");
			return -1;
		} else if (type == EFM_TYPE_YUV_CAPTURE) {
			idx_yuv = get_dsp_sub_buf_idx(buffer, 1);
			idx_me1 = get_dsp_sub_buf_idx(buffer, 0);
			if (!(dsp_partition_map & 1 << idx_yuv) ||
				!(dsp_partition_map & 1 << idx_me1)) {
				printf("Please enable Buffer %c YUV/ME1 for dsp partition!\n",
					'A' + buffer);
				return -1;
			} else {
				ret = map_src_buffer(idx_yuv, idx_me1);
				if (ret < 0) {
					return -1;
				}
			}
		}
		ret = map_efm_buffer();
	}

	return ret;
}

int map_buf_for_raw(void)
{
	struct iav_querybuf querybuf;

	querybuf.buf = IAV_BUFFER_USR;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_BUF, &querybuf);

	user_size = querybuf.length;
	if (user_size == 0) {
		printf("user size should be > 0 for raw!\n");
		return -1;
	}
	user_mem = mmap(NULL, user_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_iav,
		querybuf.offset);
	if (dsp_mem == MAP_FAILED) {
		perror("mmap (%d) failed: %s\n");
		return -1;
	}

	user_buf_size = user_size / RAW_BUF_NUM;

	return 0;
}

int map_buffer(void)
{
	int ret = 0;

	if (type == EFM_TYPE_YUV_FILE || type == EFM_TYPE_YUV_CAPTURE) {
		ret = map_buf_for_yuv();
	} else if (type == EFM_TYPE_RAW_YUV || type == EFM_TYPE_RAW_RGB) {
		ret = map_buf_for_raw();
	} else {
		ret = -1;
	}

	return ret;
}

static int get_arbitrary_resolution(const char *name, int *width, int *height)
{
	sscanf(name, "%dx%d", width, height);
	return 0;
}

static void usage(void)
{
	int i;
	printf("\ntest_efm usage:\n\n");
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
			"  1. Encode from efm buffer: Input from main buffer to EFM buffer\n"
			"    # test_encode -i1080p -V480i --cvbs --enc-mode 4 --enc-from-mem 1"
					" --efm-buf-num 8 --efm-size 1080p\n"
			"    # test_encode -A -h1080p -b 5 -e\n"
			"    # test_efm -t 1 -b 0\n"
			"    # test_encode -A -s\n"
			"  2. Encode from efm buffer: Input from YUV (NV12) file to EFM buffer\n"
			"    # test_encode -i1080p -V480i --cvbs --enc-mode 4 --enc-from-mem 1"
					" --efm-buf-num 8 --efm-size 1080p\n"
			"    # test_encode -A -h1080p -b 5 -e\n"
			"    # test_efm -t 0 -y 1080p_nv12.yuv -s 1920x1080\n"
			"    # test_encode -A -s\n"
			"  3. Encode from YUV RAW: Input from YUV (NV16:YYYYUVUV) to VIN\n"
			"    # test_efm -t 2 -i 1080p_nv16.yuv -s 1920x1080\n"
			"    # test_encode -i1080p -V480i --cvbs --enc-mode 0"
					" --enc-raw-yuv 1 --raw-size 1920x1080\n"
			"    # test_encode -A -h1080p -e\n"
			"    # test_encode -A -s\n"
			"  4. Encode from RGB RAW: Input from RGB (RAW) file to VIN\n"
			"    # test_efm -t 3 -i 1080p.raw -s 1920x1080\n"
			"    # test_encode -i1080p -V480i --cvbs --enc-mode 4 --raw-capture 1"
					" --enc-raw-rgb 1 --raw-size 1920x1080\n"
			"    # test_encode -A -h1080p -e\n"
			"    # test_encode -A -s\n");
	printf("\n");
}

int init_param(int argc, char **argv)
{
	int ch, value;
	int width, height;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options,
		&option_index)) != -1) {
		switch (ch) {
		case 't':
			value = atoi(optarg);
			if ((value < EFM_TYPE_FIRST) || (value > EFM_TYPE_LAST)) {
				printf("Invalid EFM type id %d, should be [%d, %d].\n",
					value, EFM_TYPE_FIRST, EFM_TYPE_LAST);
				return -1;
			}
			type = value;
			break;
		case 'f':
			value = atoi(optarg);
			yuv_format = value;
			break;
		case 'b':
			value = atoi(optarg);
			if ((value < IAV_SRCBUF_FIRST) || (value >= IAV_SRCBUF_LAST)) {
				printf("Invalid source buffer id : %d.\n", value);
				return -1;
			}
			buffer = value;
			break;
		case 'y':
			strcpy(yuv_name, optarg);
			break;
		case 'i':
			strcpy(raw_name, optarg);
			break;
		case 'm':
			strcpy(me1_name, optarg);
			break;
		case 'n':
			value = atoi(optarg);
			frame_num = value;
			break;
		case 's':
			get_arbitrary_resolution(optarg, &width, &height);
			frame_size.width = width;
			frame_size.height = height;
			break;
		case 'r':
			frame_rate = atoi(optarg);
			break;
		case 'c':
			mem_cpy_type = atoi(optarg);
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}

static void sigstop(int sig)
{
	exit_efm = 1;
}

static int generate_me1_file(void)
{
	u32 width, height, luma_size, me1_size, result;
	int i, j, k, l;
	u8 *start;

	printf("Start to generate me1 file from YUV!\n");
	fd_me1 = open(me1_name, O_CREAT | O_WRONLY);
	if (fd_me1 < 0) {
		printf("Failed to create me1 file %s!\n", me1_name);
		return -1;
	}

	me1_from_file = 1;
	width = efm_pool_info.yuv_size.width;
	height = efm_pool_info.yuv_size.height;
	luma_size = width * height;
	luma_buf = (u8 *)malloc(luma_size);
	if (luma_buf == NULL) {
		return -1;
	}
	width = efm_pool_info.me1_size.width;
	height = efm_pool_info.me1_size.height;
	me1_size = width * height;
	me1_buf = (u8 *)malloc(me1_size);
	if (me1_buf == NULL) {
		if (luma_buf != NULL) {
			free(luma_buf);
		}
		return -1;
	}

	for (i = 0; i < frame_num; ++i) {
		read(fd_yuv, luma_buf, luma_size);

		// ignore chroma;
		lseek(fd_yuv, luma_size >> 1, SEEK_CUR);
		width = efm_pool_info.yuv_size.width;
		for (j = 0; j < efm_pool_info.me1_size.height; j++) {
			for (k = 0; k < efm_pool_info.me1_size.width; k++) {
				result = 0;
				start = luma_buf + j * 4 * width + k * 4;
				for (l = 0; l < 4; l++) {
					result += start[l * width] + start[l * width + 1] +
						start[l * width + 2] + start[l * width + 3];
				}
				me1_buf[j * efm_pool_info.me1_size.width + k] = (result + 8) >> 4;
			}
			write(fd_me1, me1_buf + j * efm_pool_info.me1_size.width,
				efm_pool_info.me1_size.width);
		}
	}

	lseek(fd_yuv, 0L, SEEK_SET);

	printf("Saved me1 file to %s!\n", me1_name);
	if (fd_me1 >= 0) {
		close(fd_me1);
	}
	if (luma_buf != NULL) {
		free(luma_buf);
	}
	if (me1_buf != NULL) {
		free(me1_buf);
	}

	return 0;
}

int prepare_yuv_params(void)
{
	struct iav_system_resource resource;
	struct iav_srcbuf_setup setup;
	u32 single_size, total_size, state;
	int ret = 0;
	struct stat statbuff;

	AM_IOCTL(fd_iav, IAV_IOC_GET_IAV_STATE, &state);
	if ((state != IAV_STATE_PREVIEW) &&
		(state != IAV_STATE_ENCODING)) {
		printf("IAV state should be Preview/Encode for EFM!\n");
		return -1;
	}
	memset(&resource, 0, sizeof(resource));
	resource.encode_mode = DSP_CURRENT_MODE;
	AM_IOCTL(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &resource);
	if (!resource.enc_from_mem) {
		printf("Enc from mem not enabled, please enable it in test_encode!\n");
		return -1;
	}
	if (type == EFM_TYPE_YUV_FILE) {
		if (!frame_size.width && !frame_size.height) {
			printf("Frame size is not specified!\n");
			return -1;
		} else if (frame_size.width != efm_pool_info.yuv_size.width ||
			frame_size.height != efm_pool_info.yuv_size.height) {
			printf("Frame size is %dx%d, but EFM buffer size is %dx%d!\n",
				frame_size.width, frame_size.height,
				efm_pool_info.yuv_size.width, efm_pool_info.yuv_size.height);
			return -1;
		}
		if (yuv_name[0] == '\0') {
			printf("YUV file name not specified!\n");
		}
		fd_yuv = open(yuv_name, O_RDONLY);
		if (fd_yuv < 0) {
			printf("YUV file name: \"%s\" not existed!\n", yuv_name);
			return -1;
		}
		if (frame_rate == -1) {
			frame_rate = 30;
			printf("Use frame rate %d as default, if no speficied!\n",
				frame_rate);
		} else if (frame_rate == 0 || AUDIO_CLK % frame_rate){
			printf("Incorrect frame rate %d!\n", frame_rate);
			return -1;
		}
		// Fix Me:  Assume YUV420 always
		single_size = frame_size.width * frame_size.height * 3 >> 1;
		if (fstat(fd_yuv, &statbuff) < 0) {
			perror("fstat");
			return -1;
		}
		total_size = statbuff.st_size;
		lseek(fd_yuv, 0L, SEEK_SET);
		if (frame_num == -1) {
			frame_num = total_size / single_size;
			printf("There are %d frames in the YUV file!\n", frame_num);
		}
		if (me1_name[0] == '\0') {
			strcpy(me1_name, yuv_name);
			strcat(me1_name, ".me1");
			printf("Open me1_name %s first, ", me1_name);
			fd_me1 = open(me1_name, O_RDONLY);
			if (fd_me1 < 0) {
				printf("file not existed!\n");
				ret = generate_me1_file();
				if (ret < 0) {
					me1_from_file = 0;
				} else {
					me1_from_file = 1;
					fd_me1 = open(me1_name, O_RDONLY);
				}
			} else {
				printf("file opened!\n");
				me1_from_file = 1;
			}
		} else {
			me1_from_file = 1;
			fd_me1 = open(me1_name, O_RDONLY);
		}
	} else {
		memset(&setup, 0, sizeof(setup));
		AM_IOCTL(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP, &setup);
		if (setup.type[buffer] != IAV_SRCBUF_TYPE_ENCODE) {
			printf("Incorrect source buffer type %d, should be %d!\n",
				setup.type[buffer], IAV_SRCBUF_TYPE_ENCODE);
			return -1;
		}
		frame_size.width = setup.size[buffer].width;
		frame_size.height = setup.size[buffer].height;
		if (frame_size.width != efm_pool_info.yuv_size.width ||
			frame_size.height != efm_pool_info.yuv_size.height) {
			printf("Frame size is %dx%d, but EFM buffer size is %dx%d!\n",
				frame_size.width, frame_size.height,
				efm_pool_info.yuv_size.width, efm_pool_info.yuv_size.height);
			return -1;
		}
		yuv_format = YUV420_NV12;
	}
	if (yuv_format != YUV420_NV12) {
		printf("Incorrect YUV format: %d, should be %d!\n", yuv_format,
			YUV420_NV12);
		return -1;
	}
	if (frame_num == 0 || frame_num < -1) {
		printf("Incorrect frame number to be encoded: %d!\n", frame_num);
		return -1;
	}

	return 0;
}

static int get_efm_pool_info(void)
{
	memset(&efm_pool_info, 0, sizeof(efm_pool_info));
	AM_IOCTL(fd_iav, IAV_IOC_EFM_GET_POOL_INFO, &efm_pool_info);

	return 0;
}

static int gdma_copy(u8* src, u32 src_pitch, u8* dst, u32 dst_pitch,
	u32 width, u32 height)
{
	struct iav_gdma_copy param;

	param.src_offset = (u32)src;
	param.dst_offset = (u32)dst;
	param.src_mmap_type = IAV_BUFFER_DSP;
	param.dst_mmap_type = IAV_BUFFER_DSP;

	param.src_pitch = src_pitch;
	param.dst_pitch = dst_pitch;
	param.width = width;
	param.height = height;

	AM_IOCTL(fd_iav, IAV_IOC_GDMA_COPY, &param);

	return 0;
}

static int fill_yuv_from_capture(struct iav_querydesc *desc,
	struct iav_efm_request_frame *request)
{
	struct iav_yuv_cap *yuv_src;
	struct iav_me_cap *me_src;
	u8 *src, *dest;

	yuv_src = &desc->arg.bufcap.yuv[buffer];
	// fill YUV luma
	if (mem_cpy_type == MEM_CPY_TYPE_GDMA && !dsp_partition_map) {
		dest = (u8 *)request->yuv_luma_offset;
		src = (u8 *)yuv_src->y_addr_offset;
		gdma_copy(src, yuv_src->pitch, dest, efm_pool_info.yuv_pitch,
			yuv_src->width, yuv_src->height);
	} else if (dsp_partition_map) {
		dest = dsp_efm_yuv_mem + request->yuv_luma_offset;
		src = dsp_src_yuv_mem + yuv_src->y_addr_offset;
		memcpy(dest, src, yuv_src->pitch * yuv_src->height);
	} else {
		dest = dsp_mem + request->yuv_luma_offset;
		src = dsp_mem + yuv_src->y_addr_offset;
		sleep(1);
		memcpy(dest, src, yuv_src->pitch * yuv_src->height);

	}
	// fill YUV chroma
	if (mem_cpy_type == MEM_CPY_TYPE_GDMA && !dsp_partition_map) {
		dest = (u8 *)request->yuv_chroma_offset;
		src = (u8 *)yuv_src->uv_addr_offset;
		gdma_copy(src, yuv_src->pitch, dest, efm_pool_info.yuv_pitch,
			yuv_src->width, yuv_src->height >> 1);
	} else if (dsp_partition_map) {
		dest = dsp_efm_yuv_mem + request->yuv_chroma_offset;
		src = dsp_src_yuv_mem + yuv_src->uv_addr_offset;
		memcpy(dest, src, yuv_src->pitch * yuv_src->height >> 1);
	} else {
		dest = dsp_mem + request->yuv_chroma_offset;
		src = dsp_mem + yuv_src->uv_addr_offset;
		memcpy(dest, src, yuv_src->pitch * yuv_src->height >> 1);
	}
	me_src = &desc->arg.bufcap.me1[buffer];
	// fille me1
	if (mem_cpy_type == MEM_CPY_TYPE_GDMA && !dsp_partition_map) {
		dest = (u8 *)request->me1_offset;
		src = (u8 *)me_src->data_addr_offset;
		gdma_copy(src, me_src->pitch, dest, efm_pool_info.me1_pitch,
			me_src->width, me_src->height);
	} else if (dsp_partition_map) {
		dest = dsp_efm_me1_mem + request->me1_offset;
		src = dsp_src_me1_mem + me_src->data_addr_offset;
		memcpy(dest, src, me_src->pitch * me_src->height);
	} else {
		dest = dsp_mem + request->me1_offset;
		src = dsp_mem + me_src->data_addr_offset;
		memcpy(dest, src, me_src->pitch * me_src->height);
	}

	return 0;
}

static int fill_yuv_from_file(struct iav_efm_request_frame *request)
{
	u8 *luma_addr, *chroma_addr, *me1_addr;
	u32 size, i, j, k, pitch, result;
	u8 *start;
	struct timeval curr;

	if (verbose) {
		gettimeofday(&curr, NULL);
		printf("Before YUV: %u.%u!\n", (u32)curr.tv_sec, (u32)curr.tv_usec);
	}
	if (!dsp_partition_map) {
		luma_addr = dsp_mem + request->yuv_luma_offset;
		chroma_addr = dsp_mem + request->yuv_chroma_offset;
		me1_addr = dsp_mem + request->me1_offset;
	} else {
		luma_addr = dsp_efm_yuv_mem + request->yuv_luma_offset;
		chroma_addr = dsp_efm_yuv_mem + request->yuv_chroma_offset;
		me1_addr = dsp_efm_me1_mem + request->me1_offset;
	}
	if (efm_pool_info.yuv_pitch == efm_pool_info.yuv_size.width) {
		// just copy to dest buffer when pitch is the same as width
		size = efm_pool_info.yuv_pitch * efm_pool_info.yuv_size.height;
		read(fd_yuv, luma_addr, size);
		size >>= 1;
		read(fd_yuv, chroma_addr, size);
	} else {
		// copy to dest buffer line by line
		for (i = 0; i < efm_pool_info.yuv_size.height; i++) {
			read(fd_yuv, luma_addr + i * efm_pool_info.yuv_pitch,
				frame_size.width);
		}
		for (i = 0; i < efm_pool_info.yuv_size.height >> 1; i++) {
			read(fd_yuv, chroma_addr + i * efm_pool_info.yuv_pitch,
				frame_size.width);
		}
	}
	if (verbose) {
		gettimeofday(&curr, NULL);
		printf("Before ME1: %u.%u!\n", (u32)curr.tv_sec, (u32)curr.tv_usec);
	}
	if (me1_from_file == 0) {
		// generate me1 based on luma
		pitch = efm_pool_info.yuv_pitch;
		for (i = 0; i < efm_pool_info.me1_size.height; i++) {
			for (j = 0; j < efm_pool_info.me1_size.width; j++) {
				result = 0;
				start = luma_addr + i * 4 * pitch + j * 4;
				for (k = 0; k < 4; k++) {
					result += start[k * pitch] + start[k * pitch + 1] +
						start[k * pitch + 2] + start[k * pitch + 3];
				}
				me1_addr[i * efm_pool_info.me1_pitch + j] = (result + 8) >> 4;
			}
		}
	} else {
		if (efm_pool_info.me1_pitch == efm_pool_info.me1_size.width) {
			// just copy to dest buffer when pitch is the same as width
			size = efm_pool_info.me1_pitch * efm_pool_info.me1_size.height;
			read(fd_me1, me1_addr, size);
		} else {
		// copy to dest buffer line by line
			for (i = 0; i < efm_pool_info.me1_size.height; i++) {
				read(fd_me1, me1_addr + i * efm_pool_info.me1_pitch,
					efm_pool_info.me1_size.width);
			}
		}
	}
	if (verbose) {
		gettimeofday(&curr, NULL);
		printf("Done frame: %u.%u!\n", (u32)curr.tv_sec, (u32)curr.tv_usec);
	}

	return 0;
}

static int fill_yuv_frame(struct iav_efm_request_frame *req, u32* pts,
	int count)
{
	struct iav_querydesc desc;
	static u32 pts_file = 0;
	struct timeval start_time;
	struct timeval end_time;
	u32 time_intvl_us;

	if (type == EFM_TYPE_YUV_CAPTURE) {
		memset(&desc, 0, sizeof(desc));
		desc.qid = IAV_DESC_BUFCAP;
		desc.arg.bufcap.flag = IAV_BUFCAP_NONBLOCK;
		AM_IOCTL(fd_iav, IAV_IOC_QUERY_DESC, &desc);
		gettimeofday(&start_time, NULL);
		fill_yuv_from_capture(&desc, req);
		gettimeofday(&end_time, NULL);
		if (verbose) {
			time_intvl_us = (end_time.tv_sec - start_time.tv_sec) * 1000000 +
				end_time.tv_usec - start_time.tv_usec;
			printf("copy frame %d consume %u us,", count, time_intvl_us);
		}
		*pts = desc.arg.bufcap.yuv[buffer].dsp_pts;
	} else {
		fill_yuv_from_file(req);
		*pts = pts_file;
		pts_file += AUDIO_CLK / frame_rate;
	}

	return 0;
}

static int exit_yuv_loop(void)
{
	if (fd_yuv >= 0)
		close(fd_yuv);
	if (fd_me1 >= 0)
		close(fd_me1);

	return 0;
}

static int feed_yuv_loop(void)
{
	struct iav_efm_request_frame request;
	struct iav_efm_handshake_frame handshake;
	int count = 0, rval;
	u32 frame_pts = 0;

	if (get_efm_pool_info() < 0) {
		printf("Failed to get EFM pool info!\n");
		return -1;
	}
	if (prepare_yuv_params() < 0) {
		printf("Check EFM for YUV parameters failed!\n");
		return -1;
	}

	printf("START feeding YUV frames:\n");
	while(!exit_efm) {
		if (frame_num == -1 || count < frame_num) {
			printf("  Feed frame %8u,", count);
			memset(&request, 0, sizeof(request));
			while(ioctl(fd_iav, IAV_IOC_EFM_REQUEST_FRAME, &request)) {
				printf(".");
				usleep(10 * 1000);
			};
			fill_yuv_frame(&request, &frame_pts, count);
			printf("done!\t");
			// ensure once in 1 frame time
			rval = ioctl(fd_iav, IAV_IOC_WAIT_NEXT_FRAME, 0);
			if (rval < 0) {
				printf("Sleep 1 second to make sure >= 1 frame time!\n");
				sleep(1);
			}
			printf("Handshake frame %8d,", count);
			memset(&handshake, 0, sizeof(handshake));
			handshake.frame_idx = request.frame_idx;
			handshake.frame_pts = frame_pts;
			while(ioctl(fd_iav, IAV_IOC_EFM_HANDSHAKE_FRAME, &handshake)) {
				printf(".");
				usleep(10 * 1000);
			};
			printf("done!\n");

			count++;
		} else {
			printf("Finish feeding YUV frames!\n");
			exit_efm = 1;
		}
	}

	exit_yuv_loop();

	return 0;
}

static int prepare_raw_params(void)
{
	//struct iav_system_resource resource;
	u32 state, single_size, total_size;

	AM_IOCTL(fd_iav, IAV_IOC_GET_IAV_STATE, &state);
	if ((state != IAV_STATE_INIT) &&
		(state != IAV_STATE_IDLE)) {
		printf("IAV state should be INIT/IDLE for raw (YUV/RGB) encode!\n");
		return -1;
	}
	if (!frame_size.width || !frame_size.height) {
		printf("Incorrect frame size %ux%u!\n",
			frame_size.width, frame_size.height);
		return -1;
	}
	fd_raw = open(raw_name, O_RDONLY);
	if (fd_raw < 0) {
		printf("RAW file name: \"%s\" not existed!\n", raw_name);
		return -1;
	}
	// for RGB/YUV raw, bytes per pixel is 2. For YUV, this means YUV422.
	single_size = frame_size.width * frame_size.height * 2;
	total_size = lseek(fd_raw, 0L, SEEK_END);
	lseek(fd_raw, 0L, SEEK_SET);
	if (frame_num == -1) {
		frame_num = total_size / single_size;
		printf("There are %d frames in the RAW file!\n", frame_num);
	}

	sem_init(&read_sem, 0, 0);
	sem_init(&write_sem, 0, 2);
	raw_buf_rd_idx = 0;
	raw_buf_wr_idx = 0;
	single_size = ROUND_UP(frame_size.width, 16) * frame_size.height * 2;
	raw_batch_num = user_buf_size / single_size;
	if (raw_batch_num == 0) {
		printf("User memory size %u not enough for frame size %dx%d!\n",
			user_buf_size, frame_size.width, frame_size.height);
		return -1;
	} else if (frame_num < raw_batch_num) {
		raw_batch_num = frame_num;
	}

	return 0;
}

static int exit_raw_loop(void)
{
	if (raw_thread_id != 0) {
		pthread_join(raw_thread_id, NULL);
		raw_thread_id = 0;
	}
	if (fd_raw >= 0) {
		close(fd_raw);
	}

	return 0;
}

static void fill_raw_thread(void* args)
{
	u32 i, j;
	u32 pitch, line_size, total_size, single_size;
	u8* addr;

	while (!exit_efm) {
		sem_wait(&write_sem);
		if (type == EFM_TYPE_RAW_RGB) {
			line_size = frame_size.width << 1;
			pitch = ROUND_UP(line_size, 32);
			single_size = pitch * frame_size.height;
			if (pitch == line_size) {
				addr = user_mem + raw_buf_wr_idx * user_buf_size;
				total_size = raw_batch_num * single_size;
				read(fd_raw, addr, total_size);
			} else {
				// read line by line for Bayer pattern RGB raw
				for(i = 0; i < raw_batch_num; i++) {
					// read luma;
					addr = user_mem + raw_buf_wr_idx * user_buf_size +
						i * single_size;
					for (j = 0; j < frame_size.height; j++) {
						read(fd_raw, addr, line_size);
						addr += pitch;
					}
				}
			}
		} else {
			line_size = frame_size.width;
			pitch = ROUND_UP(line_size, 16);
			single_size = pitch * frame_size.height * 2;
			if (pitch == line_size) {
				addr = user_mem + raw_buf_wr_idx * user_buf_size;
				total_size = raw_batch_num * single_size;
				read(fd_raw, addr, total_size);
			} else {
				// read line by line for both luma and chroma
				for(i = 0; i < raw_batch_num; i++) {
					// read luma;
					addr = user_mem + raw_buf_wr_idx * user_buf_size +
						i * single_size;
					for (j = 0; j < frame_size.height; j++) {
						read(fd_raw, addr, frame_size.width);
						addr += pitch;
					}
					// read chroma, width is the same as luma (uv interleaved);
					for (j = 0; j < frame_size.height; j++) {
						read(fd_raw, addr, frame_size.width);
						addr += pitch;
					}
				}
			}
		}
		sem_post(&read_sem);
		raw_buf_wr_idx = (raw_buf_wr_idx + 1) % RAW_BUF_NUM;
	}

	return ;
}

static int feed_raw_loop(void)
{
	struct iav_raw_enc_setup setup;
	int count = 0, rval, batch_count;

	if (prepare_raw_params() < 0) {
		printf("Check EFM for RAW parameters failed!\n");
		return -1;
	}

	if (pthread_create(&raw_thread_id, NULL, (void*)fill_raw_thread, NULL)
		!= 0) {
		printf("Failed to create thread fill_raw_thread!\n");
		return -1;
	}

	memset(&setup, 0, sizeof(setup));
	if (type == EFM_TYPE_RAW_YUV) {
		setup.pitch = ROUND_UP(frame_size.width, 16);
		setup.raw_frame_size = setup.pitch * frame_size.height * 2;
	} else if (type == EFM_TYPE_RAW_RGB){
		setup.pitch = ROUND_UP(frame_size.width << 1, 32);
		setup.raw_frame_size = setup.pitch * frame_size.height;
	} else {
		printf("Incorrect EFM type %d for RAW!\n", type);
		return -1;
	}
	setup.raw_enc_src = RAW_ENC_SRC_USR;

	printf("START feeding RAW frames:\n");
	while(!exit_efm) {
		if (frame_num == -1 || count < frame_num) {
			sem_wait(&read_sem);
			if ((frame_num - count) > raw_batch_num) {
				batch_count = raw_batch_num;
			} else {
				batch_count = frame_num - count;
			}
			printf("  Feed frame %u ~ %u,", count, count + batch_count - 1);
			setup.raw_frame_num = batch_count;
			setup.raw_daddr_offset = raw_buf_rd_idx * user_buf_size;
			AM_IOCTL(fd_iav, IAV_IOC_SET_RAW_ENCODE, &setup);
			printf("done!\t");
			// ensure once in 1 frame time
			printf("Wait frame %u ~ %u,", count, count + batch_count - 1);
			rval = ioctl(fd_iav, IAV_IOC_WAIT_RAW_ENCODE, 0);
			if (rval < 0) {
				printf("Sleep 1 second to make sure >= 1 frame time!\n");
				sleep(1);
			}
			printf("done!\n");
			raw_buf_rd_idx = (raw_buf_rd_idx + 1) % RAW_BUF_NUM;
			sem_post(&write_sem);
			count += batch_count;
		}else {
			printf("Finish feeding RAW frames!\n");
			exit_efm = 1;
		}
	}

	exit_raw_loop();

	return 0;
}

int main(int argc, char **argv)
{
	//register signal handler for Ctrl+C, Ctrl+'\', and "kill" sys cmd
	signal(SIGINT, 	sigstop);
	signal(SIGQUIT,	sigstop);
	signal(SIGTERM,	sigstop);

	// open iav dev
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0) {
		return -1;
	}

	if (map_buffer() < 0) {
		printf("map buffer failed!\n");
		return -1;
	}

	if (type == EFM_TYPE_YUV_FILE || type == EFM_TYPE_YUV_CAPTURE) {
		feed_yuv_loop();
	} else if (type == EFM_TYPE_RAW_YUV || type == EFM_TYPE_RAW_RGB) {
		feed_raw_loop();
	} else {
		printf("Incorrect type: %d! So quit directly!", type);
	}

	if (fd_iav >= 0)
		close(fd_iav);

	return 0;
}

