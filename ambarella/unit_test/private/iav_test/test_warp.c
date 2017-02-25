/*
 * test_warp.c
 *
 * History:
 *  Feb 11, 2014 - [qianshen] created file
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
#include <basetypes.h>
#include <iav_ioctl.h>

#include <signal.h>

#define FILENAME_LENGTH        128

#define NO_ARG		0
#define HAS_ARG		1

enum numeric_short_options {
	SHOW_WARP_INFO = 0,
	DPTZ_INPUT_SIZE,
	DPTZ_INPUT_OFFSET,
	DPTZ_OUTPUT_SIZE,
	DPTZ_OUTPUT_OFFSET,
	ME1_VWARP_GRID,
	ME1_VWARP_SPACE,
	ME1_VWARP_FILE,
};

#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg)	\
		do { 						\
			if (ioctl(_filp, _cmd, _arg) < 0) {	\
				perror(#_cmd);		\
				return -1;			\
			}						\
		} while (0)
#endif
#ifndef VERIFY_AREAID
#define VERIFY_AREAID(x)	do {		\
			if (((x) < 0) || ((x) >= MAX_NUM_WARP_AREAS)) {	\
				printf("Warp area id [%d] is wrong!\n", (x));	\
				return -1;	\
			}	\
		} while (0)
#endif
#ifndef VERIFY_SUBBUFID
#define VERIFY_SUBBUFID(x)	do {		\
			if (((x) < IAV_SUB_SRCBUF_FIRST) || ((x) >= IAV_SUB_SRCBUF_LAST)) {	\
				printf("Warp dptz buffer id [%d] is wrong!\n", (x));	\
				return -1;	\
			}	\
		} while (0)
#endif

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_opts = "a:b:i:k:o:s:S:r:g:f:G:F:p:P:cv";

static struct option long_opts[] = {
	{"area",		HAS_ARG, 0, 'a'},
	{"input",		HAS_ARG, 0, 'i'},
	{"output",	HAS_ARG, 0, 'o'},
	{"in-offset",	HAS_ARG, 0, 's'},
	{"out-offset",	HAS_ARG, 0, 'S'},
	{"rotate",		HAS_ARG, 0, 'r'},
	{"hor-grid",	HAS_ARG, 0, 'g'},
	{"hor-file",	HAS_ARG, 0, 'f'},
	{"ver-grid",	HAS_ARG, 0, 'G'},
	{"ver-file",	HAS_ARG, 0, 'F'},
	{"hor-space",	HAS_ARG, 0, 'p'},
	{"ver-space",	HAS_ARG, 0, 'P'},
	// ME1 Vwarp
	{"me1-grid",	HAS_ARG, 0, ME1_VWARP_GRID},
	{"me1-space",	HAS_ARG, 0, ME1_VWARP_SPACE},
	{"me1-file",	HAS_ARG, 0, ME1_VWARP_FILE},

	{"buffer",		HAS_ARG, 0, 'b'},
	{"din",	HAS_ARG, 0, DPTZ_INPUT_SIZE},
	{"din-offset",	HAS_ARG, 0, DPTZ_INPUT_OFFSET},
	{"dout",	HAS_ARG, 0, DPTZ_OUTPUT_SIZE},
	{"dout-offset",	HAS_ARG, 0, DPTZ_OUTPUT_OFFSET},
	{"keep-dptz",		HAS_ARG, 0, 'k'},

	{"clear",		NO_ARG, 0, 'c'},
	{"show-info",	NO_ARG, 0, SHOW_WARP_INFO},
	{"verbose",	NO_ARG, 0, 'v'},

	{0, 0, 0, 0}
};

static const struct hint_s hint[] = {
	{"0~7", "\t\tspecify warp area number"},
	{"", "\t\tspecify input size, default is main source buffer size"},
	{"", "\t\tspecify output size, default is main source buffer size"},
	{"", "\t\tspecify input offset, default is 0x0"},
	{"", "\t\tspecify output offset, default is 0x0"},
	{"", "\t\tspecify rotate option, 0: non-rotate, 1: 90, 2: 180, 3: 270\n"},

	{"", "\t\tspecify horizontal map grid size, the maximum size is 32x48"},
	{"file", "\tspecify horizontal map file, must have 32x48 parameters\n"},

	{"", "\t\tspecify vertical map grid size, the maximum size is 32x48"},
	{"file", "\tspecify vertical map file, must have 32x48 parameters\n"},

	{"axb", "specify grid spacing for horizontal map. 0: 16, 1: 32, 2: 64, 3:128, 4: 256, 5: 512"},
	{"axb", "specify grid spacing for vertical map.  0: 16, 1: 32, 2: 64, 3:128, 4: 256, 5: 512\n"},

	{"axb", "specify ME1 vertical map grid size, the maximum size is 32x48\n"},
	{"axb", "specify grid spacing for ME1 vertical map.  0: 16, 1: 32, 2: 64, 3:128, 4: 256, 5: 512\n"},
	{"file", "specify ME1 vertical map file\n"},

	{"1~3", "\tset sub buffer id for the DPTZ area"},
	{"w x h", "\tspecify DPTZ area input size, default is the same area of main buffer"},
	{"X x Y", "\tspecify DPTZ area input offset, it's in the same area dimension of main buffer"},
	{"w x h", "\tspecify DPTZ area output size, default is the same ratio in 2nd / 4th buffer"},
	{"X x Y", "specify DPTZ area output offset, it's in the 2nd / 4th buffer dimension"},
	{"0|1", "\tflag to keep previous DPTZ layout on 2nd / 4th buffer. Default (0) is to let 2nd / 4th buffer have same layout as main buffer"},

	{"", "\t\tclear all warp effect\n"},
	{"", "\t\tshow warp area info"},
	{"", "\t\tprint more messages"},
};

typedef struct warp_area_param_s {
	int input_width;
	int input_height;
	int input_flag;

	int input_x;
	int input_y;
	int input_offset_flag;

	int output_width;
	int output_height;
	int output_flag;

	int output_x;
	int output_y;
	int output_offset_flag;

	int rotate;
	int rotate_flag;

	int hor_grid_col;
	int hor_grid_row;
	int hor_grid_flag;

	int hor_spacing_width;
	int hor_spacing_height;
	int hor_spacing_flag;

	char hor_file[FILENAME_LENGTH];
	char hor_file_flag;

	int hor_enable;

	int ver_grid_col;
	int ver_grid_row;
	int ver_grid_flag;

	int ver_spacing_width;
	int ver_spacing_height;
	int ver_spacing_flag;

	char ver_file[FILENAME_LENGTH];
	char ver_file_flag;

	int ver_enable;

	int me1_grid_col;
	int me1_grid_row;
	int me1_grid_flag;

	int me1_spacing_width;
	int me1_spacing_height;
	int me1_spacing_flag;

	char me1_file[FILENAME_LENGTH];
	char me1_file_flag;

	int me1_enable;

} warp_area_param_t;

typedef struct warp_dptz_param_s {
	int input_width;
	int input_height;
	int input_size_flag;

	int input_x;
	int input_y;
	int input_offset_flag;

	int output_width;
	int output_height;
	int output_size_flag;

	int output_x;
	int output_y;
	int output_offset_flag;
} warp_dptz_param_t;

static int fd_iav = -1;
static int clear_flag = 0;
static int show_info_flag = 0;
static int verbose = 0;
static int current_area = -1;
static int current_buffer = -1;
static warp_area_param_t area_param[MAX_NUM_WARP_AREAS];
static int warp_area_id_maps = 0;
static warp_dptz_param_t dptz_param[IAV_SRCBUF_NUM][MAX_NUM_WARP_AREAS];
static int warp_dptz_id_maps[IAV_SRCBUF_NUM] = {0};
static int warp_dptz_buf_maps = 0;
static int keep_dptz[IAV_SRCBUF_NUM] = {0};
static u32 apply_flags = 0;

// Memory for warp tables
static u32 warp_size;
static u8* warp_addr;

static struct iav_warp_ctrl warp_ctrl;
static int warp_area_num;
static int is_warp_dptz_supported;
static int max_table_size;

// first and second values must be in format of "AxB"
static int get_two_int(const char *name, int *first, int *second, char delimiter)
{
	char tmp_string[16];
	char * separator;

	separator = strchr(name, delimiter);
	if (!separator) {
		printf("two int should be like A%cB.\n", delimiter);
		return -1;
	}

	strncpy(tmp_string, name, separator - name);
	tmp_string[separator-name] = '\0';
	*first = atoi(tmp_string);
	strncpy(tmp_string, separator + 1, name + strlen(name) - separator);
	*second = atoi(tmp_string);

	return 0;
}

static int get_multi_s16_args(char *optarg, s16 *argarr, int argcnt)
{
	int i;
	char *delim = ", \n\t";
	char *ptr;

	ptr = strtok(optarg, delim);
	argarr[0] = atoi(ptr);

	for (i = 1; i < argcnt; ++i) {
		ptr = strtok(NULL, delim);
		if (ptr == NULL)
			break;
		argarr[i] = atoi(ptr);
	}
	if (i < argcnt) {
		printf("It's expected to have [%d] params, only get [%d].\n",
			argcnt, i);
		return -1;
	}
	return 0;
}

static int get_warp_table_from_file(const char * file, s16 * table_addr, int num)
{
	#define	MAX_FILE_LENGTH	(512 * MAX_GRID_HEIGHT)
	int fd;
	char line[MAX_FILE_LENGTH];

	if ((fd = open(file, O_RDONLY, 0)) < 0) {
		printf("### Cannot open file [%s].\n", file);
		return -1;
	}

	memset(line, 0, sizeof(line));
	read(fd, line, MAX_FILE_LENGTH);
	if (get_multi_s16_args(line, table_addr, num) < 0) {
		printf("!!! Failed to get warp table from file [%s]!\n", file);
		return -1;
	}
	printf(">>> Succeed to load warp table from file [%s].\n", file);
	return 0;
}

static void usage(void)
{
	u32 i;

	printf("test_warp usage:\n\n");
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
	printf("  Use warp to do zoom and multi regions:\n");
	printf("    test_warp -a 0 -i 960x540 -s 0x0 -o 1920x1080 -S 0x0\n\n");
	printf("  Load warp configuration from config file:\n");
	printf("    test_warp -l ./warp.cfg\n\n");
	printf("  Show warp area info:\n");
	printf("    test_warp --show-info\n\n");
}

static int init_param(int argc, char **argv)
{
	int ch, value, first, second;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_opts, long_opts, &option_index))
	    != -1) {
		switch (ch) {
			case 'a':
				value = atoi(optarg);
				VERIFY_AREAID(value);
				current_area = value;
				break;
			case 'i':
				VERIFY_AREAID(current_area);
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				area_param[current_area].input_width = first;
				area_param[current_area].input_height = second;
				area_param[current_area].input_flag = 1;
				warp_area_id_maps |= (1 << current_area);
				break;
			case 'o':
				VERIFY_AREAID(current_area);
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				area_param[current_area].output_width = first;
				area_param[current_area].output_height = second;
				area_param[current_area].output_flag = 1;
				warp_area_id_maps |= (1 << current_area);
				break;
			case 's':
				VERIFY_AREAID(current_area);
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				area_param[current_area].input_x = first;
				area_param[current_area].input_y = second;
				area_param[current_area].input_offset_flag = 1;
				warp_area_id_maps |= (1 << current_area);
				break;
			case 'S':
				VERIFY_AREAID(current_area);
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				area_param[current_area].output_x = first;
				area_param[current_area].output_y = second;
				area_param[current_area].output_offset_flag = 1;
				warp_area_id_maps |= (1 << current_area);
				break;
			case 'r':
				VERIFY_AREAID(current_area);
				value = atoi(optarg);
				area_param[current_area].rotate = value;
				area_param[current_area].rotate_flag = 1;
				warp_area_id_maps |= (1 << current_area);
				break;
			case 'g':
				VERIFY_AREAID(current_area);
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				area_param[current_area].hor_grid_col = first;
				area_param[current_area].hor_grid_row = second;
				area_param[current_area].hor_grid_flag = 1;
				area_param[current_area].hor_enable = 1;
				warp_area_id_maps |= (1 << current_area);
				break;
			case 'f':
				VERIFY_AREAID(current_area);
				value = strlen(optarg);
				if (value >= FILENAME_LENGTH) {
					printf("Filename [%s] exceeds max length %d.\n",
					    optarg, FILENAME_LENGTH);
					return -1;
				}
				strncpy(area_param[current_area].hor_file, optarg, value);
				area_param[current_area].hor_file_flag = 1;
				area_param[current_area].hor_enable = 1;
				warp_area_id_maps |= (1 << current_area);
				break;
			case 'G':
				VERIFY_AREAID(current_area);
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				area_param[current_area].ver_grid_col = first;
				area_param[current_area].ver_grid_row = second;
				area_param[current_area].ver_grid_flag = 1;
				area_param[current_area].ver_enable = 1;
				warp_area_id_maps |= (1 << current_area);
				break;
			case ME1_VWARP_GRID:
				VERIFY_AREAID(current_area);
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				area_param[current_area].me1_grid_col = first;
				area_param[current_area].me1_grid_row = second;
				area_param[current_area].me1_grid_flag = 1;
				area_param[current_area].me1_enable = 1;
				warp_area_id_maps |= (1 << current_area);
				break;
			case 'F':
				VERIFY_AREAID(current_area);
				value = strlen(optarg);
				if (value >= FILENAME_LENGTH) {
					printf("Filename [%s] exceeds max length %d.\n",
					    optarg, FILENAME_LENGTH);
					return -1;
				}
				strncpy(area_param[current_area].ver_file, optarg, value);
				area_param[current_area].ver_file_flag = 1;
				area_param[current_area].ver_enable = 1;
				warp_area_id_maps |= (1 << current_area);
				break;
			case ME1_VWARP_FILE:
				VERIFY_AREAID(current_area);
				value = strlen(optarg);
				if (value >= FILENAME_LENGTH) {
					printf("Filename [%s] exceeds max length %d.\n",
					    optarg, FILENAME_LENGTH);
					return -1;
				}
				strncpy(area_param[current_area].me1_file, optarg, value);
				area_param[current_area].me1_file_flag = 1;
				area_param[current_area].me1_enable = 1;
				warp_area_id_maps |= (1 << current_area);
				break;
			case 'p':
				VERIFY_AREAID(current_area);
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				area_param[current_area].hor_spacing_width = first;
				area_param[current_area].hor_spacing_height = second;
				area_param[current_area].hor_spacing_flag = 1;
				area_param[current_area].hor_enable = 1;
				warp_area_id_maps |= (1 << current_area);
				break;
			case 'P':
				VERIFY_AREAID(current_area);
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				area_param[current_area].ver_spacing_width = first;
				area_param[current_area].ver_spacing_height = second;
				area_param[current_area].ver_spacing_flag = 1;
				area_param[current_area].ver_enable = 1;
				warp_area_id_maps |= (1 << current_area);
				break;
			case ME1_VWARP_SPACE:
				VERIFY_AREAID(current_area);
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				area_param[current_area].me1_spacing_width = first;
				area_param[current_area].me1_spacing_height = second;
				area_param[current_area].me1_spacing_flag = 1;
				area_param[current_area].me1_enable = 1;
				warp_area_id_maps |= (1 << current_area);
				break;
			case 'c':
				clear_flag = 1;
				break;
			case 'b':
				value = atoi(optarg);
				VERIFY_SUBBUFID(value);
				current_buffer = value;
				break;
			case DPTZ_INPUT_SIZE:
				VERIFY_SUBBUFID(current_buffer);
				VERIFY_AREAID(current_area);
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				dptz_param[current_buffer][current_area].input_width = first;
				dptz_param[current_buffer][current_area].input_height = second;
				dptz_param[current_buffer][current_area].input_size_flag = 1;
				warp_dptz_id_maps[current_buffer] |= (1 << current_area);
				warp_dptz_buf_maps |= (1 << current_buffer);
				break;
			case DPTZ_INPUT_OFFSET:
				VERIFY_SUBBUFID(current_buffer);
				VERIFY_AREAID(current_area);
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				dptz_param[current_buffer][current_area].input_x = first;
				dptz_param[current_buffer][current_area].input_y = second;
				dptz_param[current_buffer][current_area].input_offset_flag = 1;
				warp_dptz_id_maps[current_buffer] |= (1 << current_area);
				warp_dptz_buf_maps |= (1 << current_buffer);
				break;
			case DPTZ_OUTPUT_SIZE:
				VERIFY_SUBBUFID(current_buffer);
				VERIFY_AREAID(current_area);
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				dptz_param[current_buffer][current_area].output_width = first;
				dptz_param[current_buffer][current_area].output_height = second;
				dptz_param[current_buffer][current_area].output_size_flag = 1;
				warp_dptz_id_maps[current_buffer] |= (1 << current_area);
				warp_dptz_buf_maps |= (1 << current_buffer);
				break;
			case DPTZ_OUTPUT_OFFSET:
				VERIFY_SUBBUFID(current_buffer);
				VERIFY_AREAID(current_area);
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				dptz_param[current_buffer][current_area].output_x = first;
				dptz_param[current_buffer][current_area].output_y = second;
				dptz_param[current_buffer][current_area].output_offset_flag = 1;
				warp_dptz_id_maps[current_buffer] |= (1 << current_area);
				warp_dptz_buf_maps |= (1 << current_buffer);
				break;
			case 'k':
				VERIFY_SUBBUFID(current_buffer);
				first = atoi(optarg);
				if (first < 0 || first > 1) {
					printf("Invalid value [%d] for 'keep-flag' option [0|1].\n",
					    first);
					return -1;
				}
				keep_dptz[current_buffer] = first;
				break;
			case SHOW_WARP_INFO:
				show_info_flag = 1;
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

static int get_spacing(int expo)
{
	return 1 << (4 + expo);
}

static int map_warp(void)
{
	struct iav_querybuf querybuf;

	querybuf.buf = IAV_BUFFER_WARP;
	if (ioctl(fd_iav, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
		perror("IAV_IOC_QUERY_BUF");
		return -1;
	}

	warp_size = querybuf.length;
	warp_addr = mmap(NULL, warp_size, PROT_WRITE, MAP_SHARED, fd_iav,
		querybuf.offset);
	printf("map_warp: start = %p, total size = 0x%x\n", warp_addr, warp_size);
	return 0;
}

static int get_warp_info(void)
{
	struct iav_system_resource iav_resource;
	memset(&iav_resource, 0, sizeof(iav_resource));
	iav_resource.encode_mode = DSP_CURRENT_MODE;
	AM_IOCTL(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &iav_resource);
	if (iav_resource.lens_warp_enable) {
		is_warp_dptz_supported = 0;
		warp_area_num = 1;
		max_table_size = MAX_WARP_TABLE_SIZE_LDC;
	} else {
		warp_area_num = MAX_NUM_WARP_AREAS;
		is_warp_dptz_supported = 1;
		max_table_size = MAX_WARP_TABLE_SIZE;
	}
	return 0;
}

static int show_warp_info(void)
{
	int i, j;
	struct iav_warp_main main_param;
	struct iav_warp_dptz dptz_param[IAV_SRCBUF_NUM];
	struct iav_warp_main *warp_main = &warp_ctrl.arg.main;
	struct iav_warp_dptz *warp_dptz = &warp_ctrl.arg.dptz;
	struct iav_warp_area *area;

	warp_ctrl.cid = IAV_WARP_CTRL_MAIN;
	AM_IOCTL(fd_iav, IAV_IOC_GET_WARP_CTRL, &warp_ctrl);
	main_param = *warp_main;

	if (is_warp_dptz_supported) {
		warp_ctrl.cid = IAV_WARP_CTRL_DPTZ;
		for (j = IAV_SUB_SRCBUF_FIRST; j < IAV_SUB_SRCBUF_LAST; j++) {
			warp_dptz->buf_id = j;
			AM_IOCTL(fd_iav, IAV_IOC_GET_WARP_CTRL, &warp_ctrl);
			dptz_param[j] = *warp_dptz;
		}
	}

	for (i = 0; i < warp_area_num; i++) {
		area = &main_param.area[i];
		printf("\t== Area[%d]: %s ==\n", i, area->enable ? "Enable" : "Disable");
		printf("\tinput size %dx%d, offset %dx%d\n", area->input.width,
		       area->input.height, area->input.x, area->input.y);
		printf("\toutput size %dx%d, offset %dx%d\n", area->output.width,
		       area->output.height, area->output.x, area->output.y);
		printf("\tRotate/Flip %d\n", area->rotate_flip);
		if (area->h_map.enable) {
			printf("\tHor Table %dx%d, grid %dx%d (%dx%d)\n",
			    area->h_map.output_grid_col, area->h_map.output_grid_row,
			    area->h_map.h_spacing, area->h_map.v_spacing,
			    get_spacing(area->h_map.h_spacing),
			    get_spacing(area->h_map.v_spacing));
		}
		if (area->v_map.enable) {
			printf("\tVer Table %dx%d, grid %dx%d (%dx%d)\n",
			    area->v_map.output_grid_col, area->v_map.output_grid_row,
			    area->v_map.h_spacing, area->v_map.v_spacing,
			    get_spacing(area->v_map.h_spacing),
			    get_spacing(area->v_map.v_spacing));
		}
		if (is_warp_dptz_supported) {
			for (j = IAV_SUB_SRCBUF_FIRST; j < IAV_SUB_SRCBUF_LAST; j++) {
				printf("Sub Buffer %d: keep_dptz %d, input %dx%d+%dx%d, output "
					"%dx%d+%dx%d\n",
				    j, main_param.keep_dptz[j], dptz_param[j].input[i].width,
				    dptz_param[j].input[i].height, dptz_param[j].input[i].x,
				    dptz_param[j].input[i].y, dptz_param[j].output[i].width,
				    dptz_param[j].output[i].height, dptz_param[j].output[i].x,
				    dptz_param[j].output[i].y);
			}
		}
		printf("\n");
	}
	return 0;
}

static int cfg_warp_main(void)
{
	int i;
	u32 addr_offset;
	struct iav_warp_main* warp_main = &warp_ctrl.arg.main;
	struct iav_warp_area *area;
	warp_area_param_t *param;
	warp_ctrl.cid = IAV_WARP_CTRL_MAIN;

	AM_IOCTL(fd_iav, IAV_IOC_GET_WARP_CTRL, &warp_ctrl);
	if (verbose) {
		printf("Before configuration:\n");
		show_warp_info();
	}
	for (i = 0; i < warp_area_num; i++) {
		area = &warp_main->area[i];
		param = &area_param[i];
		if (!clear_flag) {
			if (warp_area_id_maps & (1 << i)) {
				area->enable = 1;
				if (param->input_flag) {
					area->input.width = param->input_width;
					area->input.height = param->input_height;
				}
				if (param->input_offset_flag) {
					area->input.x = param->input_x;
					area->input.y = param->input_y;
				}
				if (param->output_flag) {
					area->output.width = param->output_width;
					area->output.height = param->output_height;
				}
				if (param->output_offset_flag) {
					area->output.x = param->output_x;
					area->output.y = param->output_y;
				}
				if (param->rotate_flag) {
					area->rotate_flip = param->rotate;
				}
				area->h_map.enable = param->hor_enable;
				if (param->hor_enable) {
					if (param->hor_grid_flag) {
						area->h_map.output_grid_col = param->hor_grid_col;
						area->h_map.output_grid_row = param->hor_grid_row;
					}
					if (param->hor_spacing_flag) {
						area->h_map.h_spacing = param->hor_spacing_width;
						area->h_map.v_spacing = param->hor_spacing_height;
					}
					if (param->hor_file_flag) {
						addr_offset = (i * WARP_AREA_VECTOR_NUM + WARP_AREA_H_OFFSET) * max_table_size
							* sizeof(s16);
						if (get_warp_table_from_file(param->hor_file,
						    (s16*)(warp_addr + addr_offset), area->h_map.output_grid_col * area->h_map.output_grid_row) < 0) {
							return -1;
						}
						area->h_map.data_addr_offset = addr_offset;
					}
				}
				area->v_map.enable = param->ver_enable;
				if (param->ver_enable) {
					if (param->ver_grid_flag) {
						area->v_map.output_grid_col = param->ver_grid_col;
						area->v_map.output_grid_row = param->ver_grid_row;
					}
					if (param->ver_spacing_flag) {
						area->v_map.h_spacing = param->ver_spacing_width;
						area->v_map.v_spacing = param->ver_spacing_height;
					}
					if (param->ver_file_flag) {
						addr_offset = (i * WARP_AREA_VECTOR_NUM + WARP_AREA_V_OFFSET) * max_table_size
							* sizeof(s16);
						if (get_warp_table_from_file(param->ver_file,
							(s16*)(warp_addr + addr_offset), area->v_map.output_grid_col * area->v_map.output_grid_row) < 0) {
							return -1;
						}
						area->v_map.data_addr_offset = addr_offset;
					}
				}

				// ME1 Vwarp
				area->me1_v_map.enable = param->me1_enable;
				if (param->me1_enable) {
					if (param->me1_grid_flag) {
						area->me1_v_map.output_grid_col = param->me1_grid_col;
						area->me1_v_map.output_grid_row = param->me1_grid_row;
					}
					if (param->me1_spacing_flag) {
						area->me1_v_map.h_spacing = param->me1_spacing_width;
						area->me1_v_map.v_spacing = param->me1_spacing_height;
					}
					if (param->me1_file_flag) {
						addr_offset = (i * WARP_AREA_VECTOR_NUM + WARP_AREA_ME1_V_OFFSET) * max_table_size
							* sizeof(s16);
						if (get_warp_table_from_file(param->me1_file,
							(s16*)(warp_addr + addr_offset), area->me1_v_map.output_grid_col * area->me1_v_map.output_grid_row) < 0) {
							return -1;
						}
						area->me1_v_map.data_addr_offset = addr_offset;
					}
				}

			}
		} else {
			area->enable = 0;
		}
	}
	if (verbose) {
		printf("\nAfter configuration:\n");
		show_warp_info();
	}

	for (i = IAV_SUB_SRCBUF_FIRST; i < IAV_SUB_SRCBUF_LAST; i++) {
		warp_main->keep_dptz[i] = keep_dptz[i];
	}
	AM_IOCTL(fd_iav, IAV_IOC_CFG_WARP_CTRL, &warp_ctrl);
	apply_flags |= (1 << IAV_WARP_CTRL_MAIN);
	return 0;
}

static int cfg_warp_dptz(void)
{
	int i, j;
	struct iav_warp_dptz* dptz = &warp_ctrl.arg.dptz;
	warp_ctrl.cid = IAV_WARP_CTRL_DPTZ;


	for (i = IAV_SUB_SRCBUF_FIRST; i < IAV_SUB_SRCBUF_LAST; i++) {
		if (warp_dptz_buf_maps & (1 << i)) {
			dptz->buf_id = i;
			AM_IOCTL(fd_iav, IAV_IOC_GET_WARP_CTRL, &warp_ctrl);

			for (j = 0; j < MAX_NUM_WARP_AREAS; j++) {
				if (warp_dptz_id_maps[i] & (1 << j)) {
					if (dptz_param[i][j].input_size_flag) {
						dptz->input[j].width = dptz_param[i][j].input_width;
						dptz->input[j].height = dptz_param[i][j].input_height;
					}
					if (dptz_param[i][j].input_offset_flag) {
						dptz->input[j].x = dptz_param[i][j].input_x;
						dptz->input[j].y = dptz_param[i][j].input_y;
					}
					if (dptz_param[i][j].output_size_flag) {
						dptz->output[j].width = dptz_param[i][j].output_width;
						dptz->output[j].height = dptz_param[i][j].output_height;
					}
					if (dptz_param[i][j].output_offset_flag) {
						dptz->output[j].x = dptz_param[i][j].output_x;
						dptz->output[j].y = dptz_param[i][j].output_y;
					}
				}
			}
			AM_IOCTL(fd_iav, IAV_IOC_CFG_WARP_CTRL, &warp_ctrl);
		}
	}
	apply_flags |= (1 << IAV_WARP_CTRL_DPTZ);
	return 0;
}

int apply_warp_control(void)
{
	if (apply_flags) {
		AM_IOCTL(fd_iav, IAV_IOC_APPLY_WARP_CTRL, &apply_flags);
	}
	return 0;
}

int main(int argc, char **argv)
{
	int ret = 0;
	if (argc < 2 || init_param(argc, argv) < 0) {
		usage();
		return -1;
	}

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	do {
		if (map_warp() < 0) {
			ret = -1;
			break;
		}

		if (get_warp_info() < 0) {
			ret = -1;
			break;
		}

		if (warp_area_id_maps || clear_flag) {
			if (cfg_warp_main() < 0) {
				ret = -1;
				break;
			}
		}

		if (warp_dptz_buf_maps) {
			if (cfg_warp_dptz() < 0) {
				ret = -1;
				break;
			}
		}

		ret = apply_warp_control();

		if (show_info_flag) {
			show_warp_info();
		}
	} while (0);

	close(fd_iav);
	return ret;
}
