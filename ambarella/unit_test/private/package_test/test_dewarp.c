/*
 * test_dewarp.c
 *
 * History:
 *	2014/03/11 - [Qian Shen] created file
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

#include <basetypes.h>
#include <iav_ioctl.h>

#include "lib_dewarp_header.h"
#include "lib_dewarp.h"

#define NO_ARG          0
#define HAS_ARG         1
#define MODE_STRING_LENGTH		(64)
#define FILENAME_LENGTH			(256)
#define MAX_PIT_YAW_ANGLE			(90)
#define MAX_ROTATE_ANGLE			(90)
#define BOLD_PRINT(msg, arg...)		printf("\033[1m"msg"\033[22m", arg)
#define BOLD_PRINT0(msg, arg...)		printf("\033[1m"msg"\033[22m")

#define MAX_ZOOMOUT_FACTOR (2)
#define PIXEL_IN_MB (16)

#ifndef VERIFY_AREAID
#define VERIFY_AREAID(x)	do {		\
			if ((x < 0) || (x >= MAX_NUM_WARP_AREAS)) {	\
				printf("Warp area id [%d] is wrong!\n", x);	\
				return -1;	\
			}	\
		} while (0)
#endif

#ifndef VERIFY_WARP_DPTZ_BUFFER_ID
#define VERIFY_WARP_DPTZ_BUFFER_ID(x)		do {		\
			if ((x) < IAV_SRCBUF_2 || (x) > IAV_SRCBUF_4) {	\
				printf("Keep DPTZ area buffer id [%d] is wrong!\n", (x));	\
				return -1;	\
			}	\
		} while (0)
#endif

enum {
	MOUNT_WALL = 0,
	MOUNT_CEILING,
	MOUNT_DESKTOP,
};

enum {
	WARP_NO_TRANSFORM = 0,
	WARP_NORMAL = 1,
	WARP_PANOR = 2,
	WARP_SUBREGION = 3,

	MAX_FISHEYE_REGION_NUM = MAX_NUM_WARP_AREAS,
};

typedef struct point_mapping_s {
	fpoint_t region;
	fpoint_t fisheye;
	pantilt_angle_t angle;
} point_mapping_t;

static int fd_iav = -1;
static int current_buffer = -1;
static int clear_flag = 0;
static int verbose = 0;
static int log_level = AMBA_LOG_INFO;
static int current_region = -1;
static int file_flag = 0;
static int keep_dptz_flag = 0;
static int keep_dptz[IAV_SRCBUF_NUM] = { 0 };
static char file[FILENAME_LENGTH] = "vector";
static fisheye_config_t fisheye_config_param = { 0 };
static int fisheye_config_flag = 0;

static dewarp_init_t dewarp_init_param;
static int fisheye_mount;

static int warp_size;
static u8* warp_addr;

static int fisheye_mode[MAX_FISHEYE_REGION_NUM];

static warp_vector_t fisheye_vector[MAX_NUM_WARP_AREAS];
static warp_region_t fisheye_region[MAX_FISHEYE_REGION_NUM];

// No Transform
rect_in_main_t notrans[MAX_FISHEYE_REGION_NUM];

// [Ceiling / Desktop] Panorama
static degree_t hor_panor_range[MAX_FISHEYE_REGION_NUM];

// [Ceiling / Desktop] Normal / Panorama
static degree_t edge_angle[MAX_FISHEYE_REGION_NUM];
static int orient[MAX_FISHEYE_REGION_NUM];

// [Wall / Ceiling / Desktop] Subregion
static pantilt_angle_t pantilt[MAX_FISHEYE_REGION_NUM];
static int pantilt_flag[MAX_FISHEYE_REGION_NUM];
static fpoint_t roi[MAX_FISHEYE_REGION_NUM];
static point_mapping_t point_mapping_output[MAX_FISHEYE_REGION_NUM];
static int point_mapping_output_flag[MAX_FISHEYE_REGION_NUM];
static point_mapping_t point_mapping_fisheye[MAX_FISHEYE_REGION_NUM];
static int point_mapping_fisheye_flag[MAX_FISHEYE_REGION_NUM];

static int premain_width, premain_height, main_width, main_height;

static int toggle_id, toggle_num;
static u32 table_size, area_size ,toggle_size;
static int zero_enable = 0;
static int show_inter = 0;
static int max_warp_in_width = 0;
static int max_warp_in_height = 0;
static int max_warp_out_width = 0;
static int lazy_mode = 0;
static int inter_offset_x = 0;
static int inter_max_height = 0;
static int real_max_warp_in_width = 0;
static int real_max_warp_in_height = 0;

struct hint_s {
	const char *arg;
	const char *str;
};

enum numeric_short_options {
	SPECIFY_NOTRANS_OFFSET = 10,
	SPECIFY_NOTRANS_SIZE,
	SPECIFY_LUT_EFL,
	SPECIFY_LUT_FILE,
	SPECIFY_MAX_WALLNOR_AREAS,
	SPECIFY_MAX_WALLPANOR_AREAS,
	SPECIFY_MAX_CEILNOR_AREAS,
	SPECIFY_MAX_CEILSUB_AREAS,
	SPECIFY_MAX_CEILPANOR_AREAS,
	SPECIFY_MAX_DESKNOR_AREAS,
	SPECIFY_MAX_DESKSUB_AREAS,
	SPECIFY_MAX_DESKPANOR_AREAS,
	SPECIFY_POINT_MAPPING_OUTPUT,
	SPECIFY_POINT_MAPPING_FISHEYE,
	SPECIFY_SHOW_INTERMEDIATE,
	SPECIFY_PREMAIN_SIZE,
	SPECIFY_MAIN_SIZE,
	SPECIFY_MAX_WARP_INPUT_WIDTH,
	SPECIFY_MAX_WARP_INPUT_HEIGHT,
	SPECIFY_MAX_WARP_OUTPUT_WIDTH,
	SPECIFY_LAZY_MODE,
	SPECIFY_ROTATE_ANGLE,
};

static const char *short_opts = "M:R:F:L:C:a:m:s:o:z:h:p:t:r:G:NSWEcf:b:k:vl:Zi:P:Y:";

static struct option long_opts[] = {
	{ "mount", HAS_ARG, 0, 'M' },
	{ "max-radius", HAS_ARG, 0, 'R' },
	{ "max-fov", HAS_ARG, 0, 'F' },
	{ "lens", HAS_ARG, 0, 'L' },
	{ "center", HAS_ARG, 0, 'C' },
	{ "area", HAS_ARG, 0, 'a' },
	{ "mode", HAS_ARG, 0, 'm' },
	{ "pitch", HAS_ARG, 0, 'P' },
	{ "yaw", HAS_ARG, 0, 'Y' },
	{ "rotate", HAS_ARG, 0, SPECIFY_ROTATE_ANGLE },
	{ "output-size", HAS_ARG, 0, 's' },
	{ "output-offset", HAS_ARG, 0, 'o' },
	{ "inter-offset", HAS_ARG, 0, 'i' },
	{ "zoom", HAS_ARG, 0, 'z' },
	{ "hor-range", HAS_ARG, 0, 'h' },
	{ "pan", HAS_ARG, 0, 'p' },
	{ "tilt", HAS_ARG, 0, 't' },
	{ "roi", HAS_ARG, 0, 'r' },
	{ "no", HAS_ARG, 0, SPECIFY_NOTRANS_OFFSET },
	{ "ns", HAS_ARG, 0, SPECIFY_NOTRANS_SIZE },
	{ "output-point", HAS_ARG, 0, SPECIFY_POINT_MAPPING_OUTPUT },
	{ "fish-point", HAS_ARG, 0, SPECIFY_POINT_MAPPING_FISHEYE },
	{ "edge-angle", HAS_ARG, 0, 'G' },
	{ "north", NO_ARG, 0, 'N' },
	{ "south", NO_ARG, 0, 'S' },
	{ "west", NO_ARG, 0, 'W' },
	{ "east", NO_ARG, 0, 'E' },
	{ "clear", NO_ARG, 0, 'c' },
	{ "lut-efl", HAS_ARG, 0, SPECIFY_LUT_EFL },
	{ "lut-file", HAS_ARG, 0, SPECIFY_LUT_FILE },
	{ "max-wallnor-areas", HAS_ARG, 0, SPECIFY_MAX_WALLNOR_AREAS },
	{ "max-wallpanor-areas", HAS_ARG, 0, SPECIFY_MAX_WALLPANOR_AREAS },
	{ "max-ceilnor-areas", HAS_ARG, 0, SPECIFY_MAX_CEILNOR_AREAS },
	{ "max-ceilpanor-areas", HAS_ARG, 0, SPECIFY_MAX_CEILPANOR_AREAS },
	{ "max-ceilsub-areas", HAS_ARG, 0, SPECIFY_MAX_CEILSUB_AREAS },
	{ "max-desknor-areas", HAS_ARG, 0, SPECIFY_MAX_DESKNOR_AREAS },
	{ "max-deskpanor-areas", HAS_ARG, 0, SPECIFY_MAX_DESKPANOR_AREAS },
	{ "max-desksub-areas", HAS_ARG, 0, SPECIFY_MAX_DESKSUB_AREAS },
	{ "file", HAS_ARG, 0, 'f' },
	{ "buffer", HAS_ARG, 0, 'b' },
	{ "keep-dptz", HAS_ARG, 0, 'k' },
	{ "verbose", NO_ARG, 0, 'v' },
	{ "level", HAS_ARG, 0, 'l' },
	{ "zero", NO_ARG, 0, 'Z' },
	{ "show-inter", NO_ARG, 0, SPECIFY_SHOW_INTERMEDIATE },
	{ "premain", HAS_ARG, 0, SPECIFY_PREMAIN_SIZE },
	{ "main", HAS_ARG, 0, SPECIFY_MAIN_SIZE },
	{ "max-iw", HAS_ARG, 0, SPECIFY_MAX_WARP_INPUT_WIDTH },
	{ "max-ih", HAS_ARG, 0, SPECIFY_MAX_WARP_INPUT_HEIGHT},
	{ "max-ow", HAS_ARG, 0, SPECIFY_MAX_WARP_OUTPUT_WIDTH },
	{ "lazy", NO_ARG, 0, SPECIFY_LAZY_MODE },
	{ 0, 0, 0, 0 }
};

static const struct hint_s hint[] =
{
	{ "0~2", "\tLens mount. 0 (default): wall, 1: ceiling, 2: desktop" },
	{ "", "\t\tFull FOV circle radius (pixel) in pre main buffer" },
	{ "0~360", "\tLens full FOV in degree" },
	{ "0|1|2", "\tLens projection mode. 0: equidistant (Linear scaled, r = f * theta), 1: Stereographic (conform, r = 2 * f * tan(theta/2), 2: Look up table for r and theta" },
	{ "axb", "\tLens circle center in pre main." },
	{ "0~7", "\t\tFisheye correction region number" },
	{ "0~3", "\t\t0: No transform, 1: Normal, 2: Panorama, 3: Subregion" },
	{ "-90~90", "\tLens Pitch in degree" },
	{ "-90~90", "\tLens Yaw in degree" },
	{ "-10~10", "\tLens Rotate in degree" },
	{ "axb", "\tOutput size in main source buffer" },
	{ "axb", "Output offset to the main buffer, default is 0x0" },
	{ "axb", "Intermediate offset to the intermediate buffer, default is 0x0" },
	{ "a/b", "\t\tZoom factor. a<b: zoom out (Wall/Ceiling Normal, Wall Panorama), a>b: zoom in (for all mode expect no transform" },

	{ "0~180", "\t(Wall/Ceiling/Desktop panorama)Panorama horizontal angle." },
	{ "-180~180", "\t(Wall/Ceiling/Desktop Subregion)Pan angle. -180~180 for ceiling/desktop mount, -90~90 for wall mount." },
	{ "-90~90", "\t(Wall/Ceiling/Desktop Subregion)Tilt angle. -90~90 for wall mount, -90~0 for ceiling mount, 0~90 for desktop." },
	{ "axb", "\t\t(Wall/Ceiling/Desktop Subregion) ROI center offset to the circle center. Negative is left/top and positive is right/bottom" },
	{ "axb", "\t\t(No Transform) ROI offset in pre main buffer." },
	{ "axb", "\t\t(No Transform) ROI size in pre main buffer." },

	{ "axb", "\tGet the axis in fisheye domain converted from the output region.  axb is the axis to the output left top. Negative is left/top and positive is right/bottom" },
	{ "axb", "\tGet the axis in the output region converted from the fisheye domain.  axb is the axis to the fisheye center. Negative is left/top and positive is right/bottom" },

	{ "80~90", "\t(Ceiling Normal / Ceiling Panorama) Edge angle." },
	{ "", "\t\t(Ceiling Normal / Ceiling Panorama) North." },
	{ "", "\t\t(Ceiling Normal / Ceiling Panorama) South." },
	{ "", "\t\t(Ceiling Normal / Ceiling Panorama) West." },
	{ "", "\t\t(Ceiling Normal / Ceiling Panorama) East." },

	{ "", "\t\tClear all warp effect" },
	{ ">0", "\tEffective focal length in pixel for the lens using look up table for r and theta." },
	{ "file", "\tLook up table file." },
	{ "1~4", "Max warp area number for wall normal mode. Default is 4." },
	{ "1~2", "Max warp area number for wall panorama mode. Default is 2." },
	{ "1~2", "Max warp area number for ceiling normal mode. Default is 2." },
	{ "1~2", "Max warp area number for ceiling panorama mode. Default is 2." },
	{ "1~2", "Max warp area number for ceiling sub region mode. Default is 2." },
	{ "1~2", "Max warp area number for desktop normal mode. Default is 2." },
	{ "1~2", "Max warp area number for desktop panorama mode. Default is 2." },
	{ "1~2", "Max warp area number for desktop sub region mode. Default is 2." },
	{ "file", "\tFile prefix to save vector maps." },
	{ "1-3", "\tSpecify the sub source buffer id, 1 for 2nd buffer, 3 for 4th buffer."},
	{ "0|1", "\tFlag to keep previous DPTZ layout on 2nd / 4th buffer. Default (0) is to let 2nd / 4th buffer have same layout as main buffer." },
	{ "", "\t\tPrint area info" },
	{ "0|1|2", "\tLog level. 0: error, 1: info (default), 2: debug" },
	{ "", "\tForce to use zeroed warp table for debug use" },
	{ "", "\tShow intermediate buffer size for each region" },
	{ "", "\tSpecify Premain size" },
	{ "", "\tSpecify Main size" },
	{ "", "\tSpecify Max Warp Input Width" },
	{ "", "\tSpecify Max Warp Input Height" },
	{ "", "\tSpecify Max Warp Output Width" },
	{ "", "\t\tUse lazy mode for Intermediate buffer" },
	{ "", "\t\tPrint elapsed time" },
};

// first and second values must be in format of "AxB"
static int get_two_int(const char *name,
                       int *first,
                       int *second,
                       char delimiter)
{
	char tmp_string[16];
	char * separator;

	separator = strchr(name, delimiter);
	if (!separator) {
		printf("two int should be like A%cB.\n", delimiter);
		return -1;
	}

	strncpy(tmp_string, name, separator - name);
	tmp_string[separator - name] = '\0';
	*first = atoi(tmp_string);
	strncpy(tmp_string, separator + 1, name + strlen(name) - separator);
	*second = atoi(tmp_string);

	return 0;
}

// first and second values must be in format of "AxB"
static int get_two_float(const char *name,
                       float *first,
                       float *second,
                       char delimiter)
{
	char tmp_string[16];
	char * separator;

	separator = strchr(name, delimiter);
	if (!separator) {
		printf("two int should be like A%cB.\n", delimiter);
		return -1;
	}

	strncpy(tmp_string, name, separator - name);
	tmp_string[separator - name] = '\0';
	*first = atof(tmp_string);
	strncpy(tmp_string, separator + 1, name + strlen(name) - separator);
	*second = atof(tmp_string);

	return 0;
}

static void usage_examples(char * program_self)
{
	printf("\n");
	printf("Example:\n");
	BOLD_PRINT0("  Lens: Focusafe 1.25mm, CS mount, 1/3\", FOV 185 degree\n");
	BOLD_PRINT0("  Sensor: IMX178\n");
	BOLD_PRINT0("  Prepare Work:\n");
	BOLD_PRINT0("    test_encode -i1080p -V480p --hdmi -W 1920 -w 1920 -P --binputsize 0x0 --bsize 1080p --enc-mode 1 --enc-rotate-poss 0\n");
	BOLD_PRINT0("    test_idsp -a\n");

	printf("\n");
	printf("0. Clear warping effect\n");
	printf("    %s -c\n\n", program_self);
	printf("1.[Wall Normal]  Correct both of horizontal/vertical lines\n");
	printf("    %s -M %d -F 185 -R 736 -a0 -s 1920x1080 -m %d\n\n", program_self, MOUNT_WALL, WARP_NORMAL);
	printf("2.[Wall panorama]\n");
	printf("    %s -M %d -F 185 -R 736 -a0 -s1920x720 -h 180 -m%d\n\n", program_self, MOUNT_WALL, WARP_PANOR);
	printf("3.[Unwarp + Wall Sub Region + Wall panorama] Pan/Tilt the sub region and keep previous PTZ in other source buffers\n");
	printf("    %s -M %d -F 185 -R 736 -a0 -s960x540 --ns 1920x1080 -m%d -a1 -r -200x0 -s960x540 -o960x0 -z15/10 -m%d -a2 -s1920x540 -o0x540 -h 180 -m%d -b 1 -k 1 -b 3 -k 1\n\n",
	    program_self, MOUNT_WALL, WARP_NO_TRANSFORM, WARP_SUBREGION,
	    WARP_PANOR);
	printf("4.[Ceiling Normal North and South]  Correct the lines in the north or south.\n");
	printf("    %s -M %d -F 185 -R 736 -a0 -s1920x540 -N -m%d -a1 -s1920x540 -o 0x540 -S -m%d\n\n",
	    program_self, MOUNT_CEILING, WARP_NORMAL, WARP_NORMAL);
	printf("5.[Ceiling West and East] Correct the lines in the west or east.\n");
	printf("    %s -M %d -F 185 -R 736 -a0 -s1920x540 -W -m%d -a1 -s1920x540 -o 0x540 -E -m%d\n\n",
	    program_self, MOUNT_CEILING, WARP_NORMAL, WARP_NORMAL);
	printf("6.[Ceiling Panorama]\n");
	printf("    %s -M %d -F 185 -R 736 -a0 -s960x540 -N -h 90 -z3/2 -m%d -a1 -s960x540 -o 960x0 -E -h 90 -z3/2 -m%d -a2 -s960x540 -o 0x540 -S -h 90 -z3/2 -m%d -a3 -s960x540 -o 960x540 -W -h 90 -z3/2 -m%d\n\n",
	    program_self, MOUNT_CEILING, WARP_PANOR, WARP_PANOR,
	    WARP_PANOR, WARP_PANOR);
	printf("7.[Ceiling Surround] Correct north/south/west/east four regions\n");
	printf("    %s -M %d -F 185 -R 736 -a0 -s960x540 -t -40 -m%d -a1 -s960x540 -o960x0 -t -40 -p 90 -m%d -a2 -s960x540 -o0x540 -t -40 -p 180 -m%d -a3 -s960x540 -o960x540 -t -40 -p -90 -m%d\n\n",
	    program_self, MOUNT_CEILING, WARP_SUBREGION, WARP_SUBREGION,
	    WARP_SUBREGION, WARP_SUBREGION);
	printf("8.[Desktop Normal North and South]  Correct the lines in the north or south.\n");
	printf("    %s -M %d -F 185 -R 736 -a0 -s1920x540 -N -m%d -a1 -s1920x540 -o 0x540 -S -m%d\n\n",
	    program_self, MOUNT_DESKTOP, WARP_NORMAL, WARP_NORMAL);
	printf("9.[Desktop West and East] Correct the lines in the west or east.\n");
	printf("    %s -M %d -F 185 -R 736 -a0 -s1920x540 -W -m%d -a1 -s1920x540 -o 0x540 -E -m%d\n\n",
	    program_self, MOUNT_DESKTOP, WARP_NORMAL, WARP_NORMAL);
	printf("10.[Desktop Panorama]\n");
	printf("    %s -M %d -F 185 -R 736 -a0 -s960x540 -W -h 90 -z3/2 -m%d -a1 -s960x540 -o 960x0 -S -h 90 -z3/2 -m%d -a2 -s960x540 -o 0x540 -E -h 90 -z3/2 -m%d -a3 -s960x540 -o 960x540 -N -h 90 -z3/2 -m%d\n\n",
	    program_self, MOUNT_DESKTOP, WARP_PANOR, WARP_PANOR,
	    WARP_PANOR, WARP_PANOR);
	printf("11.[Desktop Surround] Correct north/south/west/east four regions\n");
	printf("    %s -M %d -F 185 -R 736 -a0 -s960x540 -t 40 -m%d -a1 -s960x540 -o960x0 -t 40 -p 90 -m%d -a2 -s960x540 -o0x540 -t 40 -p 180 -m%d -a3 -s960x540 -o960x540 -t 40 -p -90 -m%d\n\n",
	    program_self, MOUNT_DESKTOP, WARP_SUBREGION, WARP_SUBREGION,
	    WARP_SUBREGION, WARP_SUBREGION);
	printf("12. Point Mapping between fisheye and output region in wall normal mode.\n");
	printf("    %s -M %d -F 185 -R 736 -a0 -s 1920x1080 -m %d --output-point 100x200 --fish-point -471.774658x-186.515564\n\n", program_self, MOUNT_WALL, WARP_NORMAL);
	printf("13.[Wall Lens Pitch/Yaw]\n");
	printf("    %s -M %d -F 185 -R 736 -a0 -s1024x1024 -m%d --max-wallnor-areas 1 -P 10 -Y 10\n\n", program_self, MOUNT_WALL, WARP_NORMAL);
}

static void usage(void)
{
	u32 i;
	char * program_self = "test_dewarp";
	printf("\n%s usage:\n\n", program_self);
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
	usage_examples(program_self);
}

static int get_multi_arg_in_premain(char *input_str, int *argarr, int *argcnt, int maxcnt)
{
	int i = 0;
	char *delim = ",:-\n\t";
	char *ptr;
	*argcnt = 0;

	struct iav_srcbuf_setup srcbuf_setup;
	memset(&srcbuf_setup, 0, sizeof(srcbuf_setup));
	ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP, &srcbuf_setup);

	ptr = strtok(input_str, delim);
	if (ptr == NULL)
		return 0;
	argarr[i++] = (int)(atof(ptr) * srcbuf_setup.size[IAV_SRCBUF_PMN].width);

	while (1) {
		ptr = strtok(NULL, delim);
		if (ptr == NULL)
			break;
		argarr[i++] = (int)(atof(ptr) * srcbuf_setup.size[IAV_SRCBUF_PMN].width);
		if (i >= maxcnt) {
			break;
		}
	}

	*argcnt = i;
	return 0;
}

static int get_lut_data_from_file(const char* file)
{
	FILE* fp;
	char line[1024];
	int num = dewarp_init_param.max_fov / 2 + 1;
	int argcnt, left = num;
	int* param;

	dewarp_init_param.lut_radius = malloc(num * sizeof(int));

	if (!dewarp_init_param.lut_radius) {
		printf("Failed to malloc %d.\n", num);
		return -1;
	}
	param = dewarp_init_param.lut_radius;
	if ((fp = fopen(file, "r")) == NULL) {
		printf("Failed to open file [%s].\n", file);
		return -1;
	}

	while (fgets(line, sizeof(line), fp) != NULL && left > 0) {
		get_multi_arg_in_premain(line, param, &argcnt, left);
		param += argcnt;
		left -= argcnt;
	}
	fclose(fp);

	if (left > 0) {
		printf("Need %d elements in file [%s].\n", num, file);
		return -1;
	}

	return 0;
}

static int init_param(int argc, char **argv)
{
	int ch, value, first, second, i;
	float first_f, second_f;
	int option_index = 0;
	char lut_file[FILENAME_LENGTH];

	opterr = 0;
	for (i = 0; i < MAX_FISHEYE_REGION_NUM; ++i) {
		fisheye_mode[i] = -1;
		edge_angle[i] = 90;
	}

	while ((ch = getopt_long(argc, argv, short_opts, long_opts, &option_index))
			!= -1) {
		switch (ch) {
			case 'M':
				value = atoi(optarg);
				fisheye_mount = value;
				break;
			case 'F':
				value = atoi(optarg);
				dewarp_init_param.max_fov = value;
				break;
			case 'L':
				value = atoi(optarg);
				dewarp_init_param.projection_mode = value;
				break;
			case 'R':
				dewarp_init_param.max_radius = atoi(optarg);
				break;
			case 'C':
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				dewarp_init_param.lens_center_in_max_input.x = first;
				dewarp_init_param.lens_center_in_max_input.y = second;
				break;
			case 'a':
				value = atoi(optarg);
				VERIFY_AREAID(value);
				current_region = value;
				break;
			case 'P':
				value = atoi(optarg);
				if (value > MAX_PIT_YAW_ANGLE || value < -MAX_PIT_YAW_ANGLE) {
					ERROR("Pitch should be within [%d~%d]\n", -MAX_PIT_YAW_ANGLE,
						MAX_PIT_YAW_ANGLE);
					return -1;
				}
				fisheye_region[current_region].pitch = value;
				break;
			case 'Y':
				value = atoi(optarg);
				if (value > MAX_PIT_YAW_ANGLE || value < -MAX_PIT_YAW_ANGLE) {
					ERROR("Yaw should be within [%d~%d]\n", -MAX_PIT_YAW_ANGLE,
						MAX_PIT_YAW_ANGLE);
					return -1;
				}
				fisheye_region[current_region].yaw = value;
				break;
			case SPECIFY_ROTATE_ANGLE:
				value = atoi(optarg);
				if (value > MAX_ROTATE_ANGLE || value < -MAX_ROTATE_ANGLE) {
					ERROR("Rotate should be within [%d~%d]\n", -MAX_ROTATE_ANGLE,
						MAX_ROTATE_ANGLE);
					return -1;
				}
				fisheye_region[current_region].rotate = value;
				break;
			case 's':
				VERIFY_AREAID(current_region);
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				fisheye_region[current_region].output.width = first;
				fisheye_region[current_region].output.height = second;
				break;
			case 'o':
				VERIFY_AREAID(current_region);
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				fisheye_region[current_region].output.upper_left.x = first;
				fisheye_region[current_region].output.upper_left.y = second;
				break;
			case 'i':
				VERIFY_AREAID(current_region);
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				fisheye_region[current_region].inter_output.upper_left.x = first;
				fisheye_region[current_region].inter_output.upper_left.y = second;
				break;
			case 'm':
				VERIFY_AREAID(current_region);
				value = atoi(optarg);
				fisheye_mode[current_region] = value;
				break;
			case 'z':
				VERIFY_AREAID(current_region);
				if (get_two_int(optarg, &first, &second, '/') < 0)
					return -1;
				fisheye_region[current_region].zoom.denom = second;
				fisheye_region[current_region].zoom.num = first;
				break;
			case 'N':
				VERIFY_AREAID(current_region);
				orient[current_region] = CEILING_NORTH;
				break;
			case 'S':
				VERIFY_AREAID(current_region);
				orient[current_region] = CEILING_SOUTH;
				break;
			case 'W':
				VERIFY_AREAID(current_region);
				orient[current_region] = CEILING_WEST;
				break;
			case 'E':
				VERIFY_AREAID(current_region);
				orient[current_region] = CEILING_EAST;
				break;
			case 'f':
				VERIFY_AREAID(current_region);
				value = strlen(optarg);
				if (value >= FILENAME_LENGTH) {
					printf("Filename [%s] is too long [%d] (>%d).\n", optarg,
					    value, FILENAME_LENGTH);
					return -1;
				}
				file_flag = 1;
				strncpy(file, optarg, value);
				break;
			case 'G':
				VERIFY_AREAID(current_region);
				edge_angle[current_region] = (degree_t) atof(optarg);
				break;
			case 'h':
				VERIFY_AREAID(current_region);
				hor_panor_range[current_region] = (degree_t) atof(optarg);
				break;
			case 'p':
				VERIFY_AREAID(current_region);
				pantilt[current_region].pan = (degree_t) atof(optarg);
				pantilt_flag[current_region] = 1;
				break;
			case 't':
				VERIFY_AREAID(current_region);
				pantilt[current_region].tilt = (degree_t) atof(optarg);
				pantilt_flag[current_region] = 1;
				break;
			case 'r':
				VERIFY_AREAID(current_region);
				if (get_two_float(optarg, &first_f, &second_f, 'x') < 0)
					return -1;
				roi[current_region].x = first_f;
				roi[current_region].y = second_f;
				break;
			case SPECIFY_NOTRANS_OFFSET:
				VERIFY_AREAID(current_region);
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				notrans[current_region].upper_left.x = first;
				notrans[current_region].upper_left.y = second;
				break;
			case SPECIFY_NOTRANS_SIZE:
				VERIFY_AREAID(current_region);
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				notrans[current_region].width = first;
				notrans[current_region].height = second;
				break;

			case SPECIFY_POINT_MAPPING_OUTPUT:
				VERIFY_AREAID(current_region);
				if (get_two_float(optarg, &first_f, &second_f, 'x') < 0)
					return -1;
				point_mapping_output[current_region].region.x = first_f;
				point_mapping_output[current_region].region.y = second_f;
				point_mapping_output_flag[current_region] = 1;
				break;

			case SPECIFY_POINT_MAPPING_FISHEYE:
				VERIFY_AREAID(current_region);
				if (get_two_float(optarg, &first_f, &second_f, 'x') < 0)
					return -1;
				point_mapping_fisheye[current_region].fisheye.x = first_f;
				point_mapping_fisheye[current_region].fisheye.y = second_f;
				point_mapping_fisheye_flag[current_region] = 1;
				break;

			case SPECIFY_LUT_EFL:
				dewarp_init_param.lut_focal_length = atoi(optarg);
				break;
			case SPECIFY_LUT_FILE:
				snprintf(lut_file, sizeof(lut_file), "%s", optarg);
				break;
			case SPECIFY_MAX_WALLNOR_AREAS:
				fisheye_config_param.wall_normal_max_area_num = atoi(optarg);
				fisheye_config_flag = 1;
				break;
			case SPECIFY_MAX_WALLPANOR_AREAS:
				fisheye_config_param.wall_panor_max_area_num = atoi(optarg);
				fisheye_config_flag = 1;
				break;
			case SPECIFY_MAX_CEILNOR_AREAS:
				fisheye_config_param.ceiling_normal_max_area_num = atoi(optarg);
				fisheye_config_flag = 1;
				break;
			case SPECIFY_MAX_CEILPANOR_AREAS:
				fisheye_config_param.ceiling_panor_max_area_num = atoi(optarg);
				fisheye_config_flag = 1;
				break;
			case SPECIFY_MAX_CEILSUB_AREAS:
				fisheye_config_param.ceiling_sub_max_area_num = atoi(optarg);
				fisheye_config_flag = 1;
				break;
			case SPECIFY_MAX_DESKNOR_AREAS:
				fisheye_config_param.desktop_normal_max_area_num = atoi(optarg);
				fisheye_config_flag = 1;
				break;
			case SPECIFY_MAX_DESKPANOR_AREAS:
				fisheye_config_param.desktop_panor_max_area_num = atoi(optarg);
				fisheye_config_flag = 1;
				break;
			case SPECIFY_MAX_DESKSUB_AREAS:
				fisheye_config_param.desktop_sub_max_area_num = atoi(optarg);
				fisheye_config_flag = 1;
				break;
			case 'c':
				clear_flag = 1;
				break;
			case 'b':
				value = atoi(optarg);
				VERIFY_WARP_DPTZ_BUFFER_ID(value);
				current_buffer = value;
				break;
			case 'k':
				VERIFY_WARP_DPTZ_BUFFER_ID(current_buffer);
				first = atoi(optarg);
				if (first < 0 || first > 1) {
					printf("Invalid value [%d] for 'keep-flag' option [0|1].\n", first);
					return -1;
				}
				keep_dptz[current_buffer] = first;
				keep_dptz_flag = 1;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'l':
				log_level = atoi(optarg);
				break;
			case 'Z':
				zero_enable = 1;
				break;
			case SPECIFY_SHOW_INTERMEDIATE:
				show_inter = 1;
				break;
			case SPECIFY_PREMAIN_SIZE:
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				premain_width = first;
				premain_height = second;
				break;
			case SPECIFY_MAIN_SIZE:
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				main_width = first;
				main_height = second;
				break;
			case SPECIFY_MAX_WARP_INPUT_WIDTH:
				value = atoi(optarg);
				max_warp_in_width = value;
				break;
			case SPECIFY_MAX_WARP_INPUT_HEIGHT:
				value = atoi(optarg);
				max_warp_in_height = value;
				break;
			case SPECIFY_MAX_WARP_OUTPUT_WIDTH:
				value = atoi(optarg);
				max_warp_out_width = value;
				break;
			case SPECIFY_LAZY_MODE:
				lazy_mode = 1;
				break;
			default:
				printf("unknown option found: %d\n", ch);
				return -1;
				break;
		}
	}

	if (dewarp_init_param.projection_mode == PROJECTION_LOOKUPTABLE) {
		if (strlen(lut_file) <= 0) {
			printf("No look up table file!\n");
			return -1;
		}
		if (get_lut_data_from_file(lut_file) < 0) {
			printf("Failed to load lens degree-radius table from file [%s].\n",
			    lut_file);
			return -1;
		}
	}

	return 0;
}

static u32 get_area_data_offset(int toggle_id, int area_id, int offset)
{
	return toggle_size * toggle_id + area_size * area_id + offset * table_size;
}

static void save_warp_table_to_file(struct iav_warp_main *p_control,
                                    int area_id)
{
	FILE *fp;
	int i, j;
	struct iav_warp_area *p_area = &p_control->area[area_id];
	s16* addr;

	char fullname[FILENAME_LENGTH];

	snprintf(fullname, sizeof(fullname), "%s_hor_%d", file, area_id);
	if ((fp = fopen(fullname, "w+")) == NULL) {
		printf("### Cannot write file [%s].\n", fullname);
		return;
	}
	addr = (s16*)(warp_addr + p_area->h_map.data_addr_offset);
	for (i = 0; i < p_area->h_map.output_grid_row; i++) {
		for (j = 0; j < p_area->h_map.output_grid_col; j++) {
			fprintf(fp, "%d\t",
			    *(addr + i * p_area->h_map.output_grid_col + j));
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
	fp = NULL;
	printf("Save hor table to [%s].\n", fullname);

	snprintf(fullname, sizeof(fullname), "%s_ver_%d", file, area_id);
	if ((fp = fopen(fullname, "w+")) == NULL) {
		printf("### Cannot write file [%s].\n", fullname);
		return;
	}
	addr = (s16*)(warp_addr + p_area->v_map.data_addr_offset);
	for (i = 0; i < p_area->v_map.output_grid_row; i++) {
		for (j = 0; j < p_area->v_map.output_grid_col; j++) {
			fprintf(fp, "%d\t",
			    *(addr + i * p_area->v_map.output_grid_col + j));
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
	printf("Save ver table to [%s].\n", fullname);

	snprintf(fullname, sizeof(fullname), "%s_me1_ver_%d", file, area_id);
	if ((fp = fopen(fullname, "w+")) == NULL) {
		printf("### Cannot write file [%s].\n", fullname);
		return;
	}
	addr = (s16*)(warp_addr + p_area->me1_v_map.data_addr_offset);
	for (i = 0; i < p_area->me1_v_map.output_grid_row; i++) {
		for (j = 0; j < p_area->me1_v_map.output_grid_col; j++) {
			fprintf(fp, "%d\t",
					*(addr + i * p_area->me1_v_map.output_grid_col + j));
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
	printf("Save ver table to [%s].\n", fullname);

}

static int get_grid_spacing(const int spacing)
{
	int grid_space = GRID_SPACING_PIXEL_64;
	switch (spacing) {
		case 16:
			grid_space = GRID_SPACING_PIXEL_16;
			break;
		case 32:
			grid_space = GRID_SPACING_PIXEL_32;
			break;
		case 64:
			grid_space = GRID_SPACING_PIXEL_64;
			break;
		case 128:
			grid_space = GRID_SPACING_PIXEL_128;
			break;
		default:
			//		printf("NOT supported spacing [%d]. Use 'spacing of 64 pixels'!\n",
//			spacing);
			grid_space = GRID_SPACING_PIXEL_64;
			break;
	}
	return grid_space;
}

static void update_warp_area(struct iav_warp_main *p_control, int region_id, int area_id)
{
	struct iav_warp_area *p_area = &p_control->area[area_id];
	warp_vector_t *p_vector = &fisheye_vector[area_id];

	p_area->enable = 1;
	p_area->input.width = p_vector->input.width;
	p_area->input.height = p_vector->input.height;
	p_area->input.x = p_vector->input.upper_left.x;
	p_area->input.y = p_vector->input.upper_left.y;
	p_area->output.width = p_vector->output.width;
	p_area->output.height = p_vector->output.height;
	p_area->output.x = p_vector->output.upper_left.x;
	p_area->output.y = p_vector->output.upper_left.y;
	p_area->rotate_flip = p_vector->rotate_flip;

	if (lazy_mode) {
		p_area->intermediate.x = inter_offset_x;
		p_area->intermediate.y = 0;
		inter_offset_x += p_vector->inter_output.width;
		inter_max_height = p_vector->inter_output.height >= inter_max_height ?
			p_vector->inter_output.height : inter_max_height;
	} else {
		p_area->intermediate.x = p_vector->inter_output.upper_left.x;
		p_area->intermediate.y = p_vector->inter_output.upper_left.y;
	}
	p_area->intermediate.width = p_vector->inter_output.width;
	p_area->intermediate.height = p_vector->inter_output.height;

	p_area->h_map.enable = (p_vector->hor_map.rows > 0 && p_vector->hor_map.cols > 0);
	p_area->h_map.h_spacing = get_grid_spacing(p_vector->hor_map.grid_width);
	p_area->h_map.v_spacing = get_grid_spacing(p_vector->hor_map.grid_height);
	p_area->h_map.output_grid_row = p_vector->hor_map.rows;
	p_area->h_map.output_grid_col = p_vector->hor_map.cols;
	p_area->h_map.data_addr_offset = get_area_data_offset(toggle_id, area_id,
		WARP_AREA_H_OFFSET);

	p_area->v_map.enable = (p_vector->ver_map.rows > 0 && p_vector->ver_map.cols > 0);
	p_area->v_map.h_spacing = get_grid_spacing(p_vector->ver_map.grid_width);
	p_area->v_map.v_spacing = get_grid_spacing(p_vector->ver_map.grid_height);
	p_area->v_map.output_grid_row = p_vector->ver_map.rows;
	p_area->v_map.output_grid_col = p_vector->ver_map.cols;
	p_area->v_map.data_addr_offset = get_area_data_offset(toggle_id, area_id,
		WARP_AREA_V_OFFSET);

	p_area->me1_v_map.enable = (p_vector->me1_ver_map.rows > 0 && p_vector->me1_ver_map.cols > 0);
	p_area->me1_v_map.h_spacing = get_grid_spacing(p_vector->me1_ver_map.grid_width);
	p_area->me1_v_map.v_spacing = get_grid_spacing(p_vector->me1_ver_map.grid_height);
	p_area->me1_v_map.output_grid_row = p_vector->me1_ver_map.rows;
	p_area->me1_v_map.output_grid_col = p_vector->me1_ver_map.cols;
	p_area->me1_v_map.data_addr_offset = get_area_data_offset(toggle_id, area_id,
		WARP_AREA_ME1_V_OFFSET);

	if (show_inter) {
		printf("\t==Region [%d] Area[%d]==\n", region_id, area_id);
		printf("\tIntermediate size %dx%d\n", p_vector->inter_output.width,
			p_vector->inter_output.height);
		real_max_warp_in_width = p_area->input.width >= real_max_warp_in_width ?
			p_area->input.width : real_max_warp_in_width;
		real_max_warp_in_height = p_area->input.height >= real_max_warp_in_height ?
			p_area->input.height : real_max_warp_in_height;
	}

	if (verbose) {
		printf("\t==Area[%d]==\n", area_id);
		printf("\tInput size %dx%d, offset %dx%d\n", p_area->input.width,
		    p_area->input.height, p_area->input.x, p_area->input.y);
		printf("\tOutput size %dx%d, offset %dx%d\n", p_area->output.width,
		    p_area->output.height,
		    p_area->output.x, p_area->output.y);
		printf("\tRotate/Flip %d\n", p_area->rotate_flip);
		printf("\tHor Table %dx%d, grid %dx%d (%dx%d)\n",
		    p_area->h_map.output_grid_col, p_area->h_map.output_grid_row,
		    p_area->h_map.h_spacing, p_area->h_map.v_spacing,
		    p_vector->hor_map.grid_width, p_vector->hor_map.grid_height);
		printf("\tVer Table %dx%d, grid %dx%d (%dx%d)\n",
		    p_area->v_map.output_grid_col, p_area->v_map.output_grid_row,
		    p_area->v_map.h_spacing, p_area->v_map.v_spacing,
		    p_vector->ver_map.grid_width, p_vector->ver_map.grid_height);
		printf("\tME1 Ver Table %dx%d, grid %dx%d (%dx%d)\n",
			p_area->me1_v_map.output_grid_col, p_area->me1_v_map.output_grid_row,
			p_area->me1_v_map.h_spacing,
			p_area->me1_v_map.v_spacing,
			p_vector->me1_ver_map.grid_width, p_vector->me1_ver_map.grid_height);
	}
}

static int check_warp_area(struct iav_warp_main *p_control, int area_id)
{
	struct iav_warp_area * p_area = &p_control->area[area_id];
	u32 new_width, new_height;
	if (!p_area->input.height || !p_area->input.width || !p_area->output.width
	    || !p_area->output.height) {
		printf("Area_%d: input size %dx%d, output size %dx%d cannot be 0\n",
		    area_id, p_area->input.width, p_area->input.height,
		    p_area->output.width, p_area->output.height);
		return -1;
	}

	if ((p_area->input.x + p_area->input.width > premain_width)
	    || (p_area->input.y + p_area->input.height > premain_height)) {
		printf("Area_%d: input size %dx%d + offset %dx%d is out of "
			"pre main %dx%d.\n",
		    area_id, p_area->input.width, p_area->input.height,
		    p_area->input.x, p_area->input.y, premain_width, premain_height);
		return -1;
	}

	if ((p_area->output.x + p_area->output.width > main_width)
	    || (p_area->output.y + p_area->output.height  > main_height)) {
		new_width = (p_area->output.x + p_area->output.width > main_width) ?
		    (main_width - p_area->output.x) :  p_area->output.width;
		new_height = (p_area->output.y + p_area->output.height > main_height) ?
		    (main_height - p_area->output.y) : p_area->output.height;
		printf("Area_%d: output size %dx%d + offset %dx%d is out of main %dx%d."
			" Reset to size [%dx%d].\n",
		    area_id, p_area->output.width, p_area->output.height,
		    p_area->output.x, p_area->output.y,
		    main_width, main_height, new_width, new_height);
		p_area->output.width = new_width;
		p_area->output.height = new_height;
	}

	return 0;
}

static int create_wall_mount_warp_vector(int region_id,
	int area_id, char * mode_str, int *area_num)
{
	switch (fisheye_mode[region_id]) {
	case WARP_NORMAL:
		snprintf(mode_str, MODE_STRING_LENGTH, "Wall Normal");
		if ((*area_num = fisheye_wall_normal(&fisheye_region[region_id],
				&fisheye_vector[area_id])) <= 0)
			return -1;
		if (point_mapping_output_flag[region_id]) {
			if (point_mapping_wall_normal_to_fisheye(&fisheye_region[region_id],
					&point_mapping_output[region_id].region,
					&point_mapping_output[region_id].fisheye) < 0)
				return -1;
		}
		if (point_mapping_fisheye_flag[region_id]) {
			if (point_mapping_fisheye_to_wall_normal(&fisheye_region[region_id],
					&point_mapping_fisheye[region_id].fisheye,
					&point_mapping_fisheye[region_id].region) < 0)
				return -1;
		}
		break;

	case WARP_PANOR:
		snprintf(mode_str, MODE_STRING_LENGTH, "Wall Panorama");
		if ((*area_num = fisheye_wall_panorama(&fisheye_region[region_id],
				hor_panor_range[region_id], &fisheye_vector[area_id])) <= 0)
			return -1;
		if (point_mapping_output_flag[region_id]) {
			if (point_mapping_wall_panorama_to_fisheye(&fisheye_region[region_id],
					hor_panor_range[region_id],
					&point_mapping_output[region_id].region,
					&point_mapping_output[region_id].fisheye) < 0)
				return -1;
		}
		if (point_mapping_fisheye_flag[region_id]) {
			if (point_mapping_fisheye_to_wall_panorama(&fisheye_region[region_id],
					hor_panor_range[region_id],
					&point_mapping_fisheye[region_id].fisheye,
					&point_mapping_fisheye[region_id].region) < 0)
				return -1;
		}
		break;

	case WARP_SUBREGION:
		snprintf(mode_str, MODE_STRING_LENGTH, "Wall Subregion");
		if (pantilt_flag[region_id]) {
			if ((*area_num = fisheye_wall_subregion_angle(
					&fisheye_region[region_id], &pantilt[region_id],
					&fisheye_vector[area_id], &roi[region_id])) <= 0)
				return -1;
		} else {
			if ((*area_num = fisheye_wall_subregion_roi(
					&fisheye_region[region_id], &roi[region_id],
					&fisheye_vector[area_id], &pantilt[region_id])) <= 0)
				return -1;
		}
		if (point_mapping_output_flag[region_id]) {
			if (point_mapping_wall_subregion_to_fisheye(
					&fisheye_region[region_id], &roi[region_id],
					&point_mapping_output[region_id].region,
					&point_mapping_output[region_id].fisheye,
					&point_mapping_output[region_id].angle) < 0)
				return -1;
		}
		if (point_mapping_fisheye_flag[region_id]) {
			if (point_mapping_fisheye_to_wall_subregion(
					&fisheye_region[region_id], &roi[region_id],
					&point_mapping_fisheye[region_id].fisheye,
					&point_mapping_fisheye[region_id].region) < 0)
				return -1;
		}
		break;

	default:
		break;
	}
	return 0;
}

static int create_ceiling_mount_warp_vector(int region_id,
	int area_id, char * mode_str, int *area_num)
{
	switch (fisheye_mode[region_id]) {
	case WARP_NORMAL:
		snprintf(mode_str, MODE_STRING_LENGTH, "Ceiling Normal");
		if ((*area_num = fisheye_ceiling_normal(&fisheye_region[region_id],
				edge_angle[region_id], orient[region_id],
				&fisheye_vector[area_id])) <= 0)
			return -1;
		if (point_mapping_output_flag[region_id]) {
			if (point_mapping_ceiling_normal_to_fisheye(
					&fisheye_region[region_id], edge_angle[region_id],
					orient[region_id],
					&point_mapping_output[region_id].region,
					&point_mapping_output[region_id].fisheye) < 0)
				return -1;
		}
		if (point_mapping_fisheye_flag[region_id]) {
			if (point_mapping_fisheye_to_ceiling_normal(
					&fisheye_region[region_id], edge_angle[region_id],
					orient[region_id],
					&point_mapping_fisheye[region_id].fisheye,
					&point_mapping_fisheye[region_id].region) < 0)
				return -1;
		}
		break;

	case WARP_PANOR:
		snprintf(mode_str, MODE_STRING_LENGTH, "Ceiling Panorama");
		if ((*area_num = fisheye_ceiling_panorama(&fisheye_region[region_id],
				edge_angle[region_id],
				hor_panor_range[region_id], orient[region_id],
				&fisheye_vector[area_id])) <= 0)
			return -1;
		if (point_mapping_output_flag[region_id]) {
			if (point_mapping_ceiling_panorama_to_fisheye(
					&fisheye_region[region_id], edge_angle[region_id],
					hor_panor_range[region_id], orient[region_id],
					&point_mapping_output[region_id].region,
					&point_mapping_output[region_id].fisheye) < 0)
				return -1;
		}
		if (point_mapping_fisheye_flag[region_id]) {
			if (point_mapping_fisheye_to_ceiling_panorama(
					&fisheye_region[region_id], edge_angle[region_id],
					hor_panor_range[region_id], orient[region_id],
					&point_mapping_fisheye[region_id].fisheye,
					&point_mapping_fisheye[region_id].region) < 0)
				return -1;
		}
		break;

	case WARP_SUBREGION:
		snprintf(mode_str, MODE_STRING_LENGTH, "Ceiling Subregion");
		if (pantilt_flag[region_id]) {
			if ((*area_num = fisheye_ceiling_subregion_angle(
					&fisheye_region[region_id], &pantilt[region_id],
					&fisheye_vector[area_id], &roi[region_id])) <= 0)
				return -1;
		} else {
			if ((*area_num = fisheye_ceiling_subregion_roi(
					&fisheye_region[region_id], &roi[region_id],
					&fisheye_vector[area_id], &pantilt[region_id])) <= 0)
				return -1;
		}
		if (point_mapping_output_flag[region_id]) {
			if (point_mapping_ceiling_subregion_to_fisheye(
					&fisheye_region[region_id], &roi[region_id],
					&point_mapping_output[region_id].region,
					&point_mapping_output[region_id].fisheye,
					&point_mapping_output[region_id].angle) < 0)
				return -1;
		}
		if (point_mapping_fisheye_flag[region_id]) {
			if (point_mapping_fisheye_to_ceiling_subregion(
					&fisheye_region[region_id], &roi[region_id],
					&point_mapping_fisheye[region_id].fisheye,
					&point_mapping_fisheye[region_id].region) < 0)
				return -1;
		}
		break;

	default:
		break;
	}
	return 0;
}

static int create_desktop_mount_warp_vector(int region_id,
	int area_id, char * mode_str, int *area_num)
{
	switch (fisheye_mode[region_id]) {
	case WARP_NORMAL:
		snprintf(mode_str, MODE_STRING_LENGTH, "Desktop Normal");
		if ((*area_num = fisheye_desktop_normal(&fisheye_region[region_id],
				edge_angle[region_id], orient[region_id],
				&fisheye_vector[area_id])) <= 0)
			return -1;
		if (point_mapping_output_flag[region_id]) {
			if (point_mapping_desktop_normal_to_fisheye(
					&fisheye_region[region_id], edge_angle[region_id],
					orient[region_id], &point_mapping_output[region_id].region,
					&point_mapping_output[region_id].fisheye) < 0)
				return -1;
		}
		if (point_mapping_fisheye_flag[region_id]) {
			if (point_mapping_fisheye_to_desktop_normal(
					&fisheye_region[region_id], edge_angle[region_id],
					orient[region_id],
					&point_mapping_fisheye[region_id].fisheye,
					&point_mapping_fisheye[region_id].region) < 0)
				return -1;
		}
		break;

	case WARP_PANOR:
		snprintf(mode_str, MODE_STRING_LENGTH, "Desktop Panorama");
		if ((*area_num = fisheye_desktop_panorama(&fisheye_region[region_id],
				edge_angle[region_id], hor_panor_range[region_id],
				orient[region_id], &fisheye_vector[area_id])) <= 0)
			return -1;
		if (point_mapping_output_flag[region_id]) {
			if (point_mapping_desktop_panorama_to_fisheye(
					&fisheye_region[region_id], edge_angle[region_id],
					hor_panor_range[region_id], orient[region_id],
					&point_mapping_output[region_id].region,
					&point_mapping_output[region_id].fisheye) < 0)
				return -1;
		}
		if (point_mapping_fisheye_flag[region_id]) {
			if (point_mapping_fisheye_to_desktop_panorama(
					&fisheye_region[region_id], edge_angle[region_id],
					hor_panor_range[region_id], orient[region_id],
					&point_mapping_fisheye[region_id].fisheye,
					&point_mapping_fisheye[region_id].region) < 0)
				return -1;
		}
		break;

	case WARP_SUBREGION:
		snprintf(mode_str, MODE_STRING_LENGTH, "Desktop Subregion");
		if (pantilt_flag[region_id]) {
			if ((*area_num = fisheye_desktop_subregion_angle(
					&fisheye_region[region_id], &pantilt[region_id],
					&fisheye_vector[area_id], &roi[region_id])) <= 0)
				return -1;
		} else {
			if ((*area_num = fisheye_desktop_subregion_roi(
					&fisheye_region[region_id], &roi[region_id],
					&fisheye_vector[area_id], &pantilt[region_id])) <= 0)
				return -1;
		}
		if (point_mapping_output_flag[region_id]) {
			if (point_mapping_desktop_subregion_to_fisheye(
					&fisheye_region[region_id], &roi[region_id],
					&point_mapping_output[region_id].region,
					&point_mapping_output[region_id].fisheye,
					&point_mapping_output[region_id].angle) < 0)
				return -1;
		}
		if (point_mapping_fisheye_flag[region_id]) {
			if (point_mapping_fisheye_to_desktop_subregion(
					&fisheye_region[region_id], &roi[region_id],
					&point_mapping_fisheye[region_id].fisheye,
					&point_mapping_fisheye[region_id].region) < 0)
				return -1;
		}
		break;

	default:
		break;
	}
	return 0;
}

static int create_fisheye_warp_vector(int region_id, int area_id)
{
	int area_num = 0;
	char mode_str[MODE_STRING_LENGTH];
	struct timespec lasttime, curtime;

	if (verbose) {
		clock_gettime(CLOCK_REALTIME, &lasttime);
	}
	if (show_inter) {
		fisheye_region[region_id].zoom.num = 1;
		fisheye_region[region_id].zoom.denom = MAX_ZOOMOUT_FACTOR;
		switch (fisheye_mount) {
			case MOUNT_WALL:
				if (fisheye_mode[region_id] == WARP_SUBREGION) {
					fisheye_region[region_id].zoom.denom = 1;
				}
				break;
			case MOUNT_CEILING:
			case MOUNT_DESKTOP:
				if (fisheye_mode[region_id] == WARP_PANOR ||
					fisheye_mode[region_id] == WARP_SUBREGION) {
					fisheye_region[region_id].zoom.denom = 1;
				}
				break;
			default:
				break;
		}
	}

	if (fisheye_mode[region_id] == WARP_NO_TRANSFORM) {
		snprintf(mode_str, MODE_STRING_LENGTH, "No Transform");
		if ((area_num =
			fisheye_no_transform(&fisheye_region[region_id],
		    &notrans[region_id], &fisheye_vector[area_id])) <= 0)
			return -1;
	} else {
		switch (fisheye_mount) {
		case MOUNT_WALL:
			if (create_wall_mount_warp_vector(region_id,
					area_id, mode_str, &area_num) < 0) {
				return -1;
			}
			break;

		case MOUNT_CEILING:
			if (create_ceiling_mount_warp_vector(region_id,
					area_id, mode_str, &area_num) < 0) {
				return -1;
			}
			break;

		case MOUNT_DESKTOP:
			if (create_desktop_mount_warp_vector(region_id,
					area_id, mode_str, &area_num) < 0) {
				return -1;
			}
			break;

		default:
			break;
		}
	}

	if ((verbose || point_mapping_output_flag[region_id] ||
		point_mapping_fisheye_flag[region_id]) && area_num > 0) {
		printf("\nFisheye Region [%d]: %s uses %d warp area(s).\n", region_id,
		    mode_str, area_num);
		if (point_mapping_output_flag[region_id]) {
			printf("Point in region (%f, %f) => Point in fisheye (%f, %f)",
			    point_mapping_output[region_id].region.x,
			    point_mapping_output[region_id].region.y,
			    point_mapping_output[region_id].fisheye.x,
			    point_mapping_output[region_id].fisheye.y);
			if (fisheye_mode[region_id] == WARP_SUBREGION) {
				printf(", pan %f, tilt %f",
					point_mapping_output[region_id].angle.pan,
					point_mapping_output[region_id].angle.tilt);
			}
			printf("\n");
		}
		if (point_mapping_fisheye_flag[region_id]) {
			printf("Point in fisheye (%f, %f) => Point in region (%f, %f)\n",
			    point_mapping_fisheye[region_id].fisheye.x,
			    point_mapping_fisheye[region_id].fisheye.y,
			    point_mapping_fisheye[region_id].region.x,
			    point_mapping_fisheye[region_id].region.y);
		}
		if (verbose) {
			clock_gettime(CLOCK_REALTIME, &curtime);
			printf("Elapsed Time (ms): [%05ld]\n",
			    (curtime.tv_sec - lasttime.tv_sec) * 1000
			        + (curtime.tv_nsec - lasttime.tv_nsec) / 1000000);
			if (fisheye_mode[region_id] == WARP_SUBREGION) {
				if (pantilt_flag[region_id]) {
					printf("pan %f, tilt %f => ROI (%f, %f)\n",
					    pantilt[region_id].pan, pantilt[region_id].tilt,
					    roi[region_id].x, roi[region_id].y);
				} else {
					printf("ROI (%f, %f) => pan %f, tilt %f\n",
					    roi[region_id].x, roi[region_id].y,
					    pantilt[region_id].pan, pantilt[region_id].tilt);
				}
			}
		}
	}

	return area_num;
}

static int set_fisheye_warp(void)
{
	struct iav_warp_ctrl warp_control;
	struct iav_warp_main* warp_main = &warp_control.arg.main;
	int fy_id, i, j, area_num = 0, used_area_num = 0;
	u32 flags = (1 << IAV_WARP_CTRL_MAIN);
	int inter_height = 0;

	for (i = 0; i < MAX_NUM_WARP_AREAS; i++) {
		fisheye_vector[i].hor_map.addr =
		    (s16*)(warp_addr + get_area_data_offset(toggle_id, i, WARP_AREA_H_OFFSET));
		fisheye_vector[i].ver_map.addr =
			(s16*)(warp_addr + get_area_data_offset(toggle_id, i, WARP_AREA_V_OFFSET));
		fisheye_vector[i].me1_ver_map.addr =
			(s16*)(warp_addr + get_area_data_offset(toggle_id, i, WARP_AREA_ME1_V_OFFSET));
	}

	memset(&warp_control, 0, sizeof(warp_control));
	warp_control.cid = IAV_WARP_CTRL_MAIN;
	if (!clear_flag) {
		for (fy_id = 0, used_area_num = 0;
			fy_id < MAX_FISHEYE_REGION_NUM; fy_id++) {
			if ((area_num = create_fisheye_warp_vector(fy_id, used_area_num))
			    < 0) {
				return -1;
			}
			for (j = 0; j < area_num; j++) {
				update_warp_area(warp_main, fy_id, used_area_num + j);
				if (check_warp_area(warp_main, used_area_num + j) < 0) {
					return -1;
				}
				if (zero_enable) {
					memset(fisheye_vector[used_area_num + j].hor_map.addr, 0, table_size);
					memset(fisheye_vector[used_area_num + j].ver_map.addr, 0, table_size);
					memset(fisheye_vector[used_area_num + j].me1_ver_map.addr, 0, table_size);
				}
				if (file_flag) {
					save_warp_table_to_file(warp_main, used_area_num + j);
				}
			}
			used_area_num += area_num;
		}

		if (lazy_mode && show_inter) {
			inter_height = ROUND_UP(premain_height >= main_height ?
				premain_height : main_height, PIXEL_IN_MB);
			printf("Intermediate Buffer Total Size : %dx%d\n",
				inter_offset_x >= premain_width ? inter_offset_x : premain_width,
				inter_max_height >= inter_height ? inter_max_height : inter_height);
		}
		if (show_inter) {
			printf("Real Max warp_in_width : %d\n", real_max_warp_in_width);
			printf("Real Max warp_in_height : %d\n", real_max_warp_in_height);
		}
	}
	if (keep_dptz_flag) {
		for (i = IAV_SUB_SRCBUF_FIRST; i < IAV_SUB_SRCBUF_LAST; i++) {
			warp_main->keep_dptz[i] = keep_dptz[i];
		}
	}
	if (!show_inter) {
		if (ioctl(fd_iav, IAV_IOC_CFG_WARP_CTRL, &warp_control) < 0) {
			perror("IAV_IOC_CFG_WARP_CTRL");
			return -1;
		}

		if (ioctl(fd_iav, IAV_IOC_APPLY_WARP_CTRL, &flags) < 0) {
			perror("IAV_IOC_APPLY_WARP_CTRL");
			return -1;
		}
	}
	toggle_id = (toggle_id + 1) % toggle_num;
	return 0;
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

	table_size = MAX_WARP_TABLE_SIZE * sizeof(s16);
	area_size = table_size * WARP_AREA_VECTOR_NUM; // 3 tables per area
	toggle_size = area_size * MAX_NUM_WARP_AREAS;
	toggle_num = warp_size / toggle_size;
	printf("map_warp: start = %p, total size = 0x%x, toggle num = %d\n",
	       warp_addr, warp_size, toggle_num);

	return 0;
}

static int init_fisheye_warp(void)
{
	int fy_id, cur_area_id, need_area_num;
	version_t version;
	fisheye_config_t config;
	struct iav_srcbuf_setup buffer_setup;
	struct iav_system_resource sys_res;
	memset(&sys_res, 0, sizeof(sys_res));
	sys_res.encode_mode = DSP_CURRENT_MODE;

	if (map_warp() < 0) {
		return -1;
	}

	if (!clear_flag) {
		if (dewarp_get_version(&version) < 0)
			return -1;
		if (verbose) {
			printf("\nDewarp Library Version: %s-%d.%d.%d (Last updated: %x)\n\n",
			    version.description, version.major, version.minor,
			    version.patch, version.mod_time);
		}
		if (set_log(log_level, "stderr") < 0)
			return -1;

		if (fisheye_get_config(&config) < 0)
			return -1;

		if (fisheye_config_flag) {
			if (fisheye_config_param.wall_normal_max_area_num) {
				config.wall_normal_max_area_num =
				    fisheye_config_param.wall_normal_max_area_num;
			}
			if (fisheye_config_param.wall_panor_max_area_num) {
				config.wall_panor_max_area_num =
				    fisheye_config_param.wall_panor_max_area_num;
			}
			if (fisheye_config_param.ceiling_normal_max_area_num) {
				config.ceiling_normal_max_area_num =
				    fisheye_config_param.ceiling_normal_max_area_num;
			}
			if (fisheye_config_param.ceiling_panor_max_area_num) {
				config.ceiling_panor_max_area_num =
				    fisheye_config_param.ceiling_panor_max_area_num;
			}
			if (fisheye_config_param.ceiling_sub_max_area_num) {
				config.ceiling_sub_max_area_num =
				    fisheye_config_param.ceiling_sub_max_area_num;
			}
			if (fisheye_config_param.desktop_normal_max_area_num) {
				config.desktop_normal_max_area_num =
				    fisheye_config_param.desktop_normal_max_area_num;
			}
			if (fisheye_config_param.desktop_panor_max_area_num) {
				config.desktop_panor_max_area_num =
				    fisheye_config_param.desktop_panor_max_area_num;
			}
			if (fisheye_config_param.desktop_sub_max_area_num) {
				config.desktop_sub_max_area_num =
				    fisheye_config_param.desktop_sub_max_area_num;
			}
		}

		if (show_inter) {
			config.max_warp_input_width = max_warp_in_width;
			config.max_warp_input_height = max_warp_in_height;
			config.max_warp_output_width = max_warp_out_width;
		} else {
			if (ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &sys_res) < 0) {
				perror("IAV_IOC_GET_SYSTEM_RESOURCE");
				return -1;
			}
			config.max_warp_input_width = sys_res.max_warp_input_width;
			config.max_warp_input_height = sys_res.max_warp_input_height;
			config.max_warp_output_width = sys_res.max_warp_output_width;
		}

		if (fisheye_set_config(&config) < 0)
			return -1;

		for (fy_id = 0, cur_area_id = 0;
			fy_id < MAX_FISHEYE_REGION_NUM; fy_id++) {
			need_area_num = 0;
			switch (fisheye_mount) {
			case MOUNT_WALL:
				switch (fisheye_mode[fy_id]) {
				case WARP_NORMAL:
					need_area_num = config.wall_normal_max_area_num;
					break;
				case WARP_PANOR:
					need_area_num = config.wall_panor_max_area_num;
					break;
				case WARP_SUBREGION:
				case WARP_NO_TRANSFORM:
					need_area_num = 1;
					break;
				default:
					need_area_num = 0;
					break;
				}
				break;

			case MOUNT_CEILING:
				switch (fisheye_mode[fy_id]) {
				case WARP_NORMAL:
					need_area_num = config.ceiling_normal_max_area_num;
					break;
				case WARP_PANOR:
					need_area_num = config.ceiling_panor_max_area_num;
					break;
				case WARP_SUBREGION:
					need_area_num = config.ceiling_sub_max_area_num;
					break;
				case WARP_NO_TRANSFORM:
					need_area_num = 1;
					break;
				default:
					need_area_num = 0;
					break;
				}
				break;

			case MOUNT_DESKTOP:
				switch (fisheye_mode[fy_id]) {
				case WARP_NORMAL:
					need_area_num = config.desktop_normal_max_area_num;
					break;
				case WARP_PANOR:
					need_area_num = config.desktop_panor_max_area_num;
					break;
				case WARP_SUBREGION:
					need_area_num = config.desktop_sub_max_area_num;
					break;
				case WARP_NO_TRANSFORM:
					need_area_num = 1;
					break;
				default:
					need_area_num = 0;
					break;
				}
				break;

			default:
				printf("Unknown mount %d.\n", fisheye_mount);
				return -1;
				break;
			}

			if (cur_area_id + need_area_num > MAX_NUM_WARP_AREAS) {
				printf("Need %d areas (> max %d)\n", cur_area_id
				    + need_area_num, MAX_NUM_WARP_AREAS);
				return -1;
			}

			cur_area_id += need_area_num;
		}

		if (!show_inter) {
			if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP, &buffer_setup) < 0) {
				perror("IAV_IOC_GET_SOURCE_BUFFER_SETUP");
				return -1;
			}

			premain_width = buffer_setup.size[IAV_SRCBUF_PMN].width;
			premain_height = buffer_setup.size[IAV_SRCBUF_PMN].height;
			main_width = buffer_setup.size[IAV_SRCBUF_MN].width;
			main_height = buffer_setup.size[IAV_SRCBUF_MN].height;
		}

		dewarp_init_param.max_input_width = premain_width;
		dewarp_init_param.max_input_height = premain_height;

		if (!dewarp_init_param.lens_center_in_max_input.x
		    || !dewarp_init_param.lens_center_in_max_input.y) {
			dewarp_init_param.lens_center_in_max_input.x = premain_width / 2;
			dewarp_init_param.lens_center_in_max_input.y = premain_height / 2;
		}

		if (dewarp_init(&dewarp_init_param) < 0)
			return -1;
	}

	return 0;
}

static int deinit_fisheye_warp(void)
{
	dewarp_deinit();
	if (dewarp_init_param.lut_radius) {
		free(dewarp_init_param.lut_radius);
		dewarp_init_param.lut_radius = NULL;
	}
	return 0;
}

int main(int argc, char **argv)
{
	int ret = 0;

	if (argc < 2) {
		usage();
		return -1;
	}
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (init_param(argc, argv) < 0) {
		return -1;
	}

	if (init_fisheye_warp() < 0) {
		ret = -1;
	} else if (set_fisheye_warp() < 0) {
		ret = -1;
	}
	if (deinit_fisheye_warp() < 0) {
		ret = -1;
	}
	return ret;
}

