/*
 * test_dewarp_lens.c
 *
 * History:
 *	2014/02/25 - [Qian Shen] created file
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
#define MAX_ROTATE_ANGLE			(10)
#define BOLD_PRINT(msg, arg...)		printf("\033[1m"msg"\033[22m", arg)
#define BOLD_PRINT0(msg, arg...)		printf("\033[1m"msg"\033[22m")

#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg)	\
		do { 						\
			if (ioctl(_filp, _cmd, _arg) < 0) {	\
				perror(#_cmd);		\
				return -1;			\
			}						\
		} while (0)
#endif

enum {
	WARP_NO_TRANSFORM = 0,
	WARP_NORMAL = 1,
	WARP_PANOR = 2,
	WARP_SUBREGION = 3,
};

typedef struct point_mapping_s {
	fpoint_t region;
	fpoint_t fisheye;
	pantilt_angle_t angle;
} point_mapping_t;

typedef struct warp_debug_s {
	int force_zero;
	int h_disable;
	int v_disable;
} warp_debug_t;

static int fd_iav = -1;
static int clear_flag = 0;
static int verbose = 0;
static int log_level = AMBA_LOG_INFO;
static int file_flag = 0;
static char file[FILENAME_LENGTH] = "lens";

static dewarp_init_t dewarp_init_param;
static int lens_warp_mode = -1;

static warp_vector_t lens_warp_vector;
static warp_region_t lens_warp_region;

// No Transform
rect_in_main_t notrans;

// Panorama
static degree_t hor_panor_range;


// [Wall] Subregion]
static pantilt_angle_t pantilt;
static int pantilt_flag;
static fpoint_t roi;

struct iav_srcbuf_format main_buffer;
struct amba_video_info	vin;

static int warp_size;
static u8* warp_addr;

static warp_debug_t debug_info = {0, 0, 0};

struct hint_s {
	const char *arg;
	const char *str;
};

enum numeric_short_options {
	SPECIFY_NOTRANS_OFFSET = 10,
	SPECIFY_NOTRANS_SIZE,
	SPECIFY_LUT_EFL,
	SPECIFY_LUT_FILE,
	SPECIFY_FORCE_ZERO = 130,
	SPECIFY_H_DISABLE,
	SPECIFY_V_DISABLE,
	SPECIFY_H_ZOOM,
	SPECIFY_V_ZOOM,
	SPECIFY_ROTATE_ANGLE,
};

static const char *short_opts = "R:F:L:C:m:z:h:p:t:r:cf:vl:P:Y:";

static struct option long_opts[] = {
	{ "max-radius", HAS_ARG, 0, 'R' },
	{ "max-fov", HAS_ARG, 0, 'F' },
	{ "lens", HAS_ARG, 0, 'L' },
	{ "center", HAS_ARG, 0, 'C' },
	{ "mode", HAS_ARG, 0, 'm' },
	{ "pitch", HAS_ARG, 0, 'P' },
	{ "yaw", HAS_ARG, 0, 'Y' },
	{ "rotate", HAS_ARG, 0, SPECIFY_ROTATE_ANGLE},
	{ "zoom", HAS_ARG, 0, 'z' },
	{ "zh", HAS_ARG, 0, SPECIFY_H_ZOOM },
	{ "zv", HAS_ARG, 0, SPECIFY_V_ZOOM },
	{ "hor-range", HAS_ARG, 0, 'h' },
	{ "pan", HAS_ARG, 0, 'p' },
	{ "tilt", HAS_ARG, 0, 't' },
	{ "roi", HAS_ARG, 0, 'r' },
	{ "no", HAS_ARG, 0, SPECIFY_NOTRANS_OFFSET },
	{ "ns", HAS_ARG, 0, SPECIFY_NOTRANS_SIZE },
	{ "clear", NO_ARG, 0, 'c' },
	{ "lut-efl", HAS_ARG, 0, SPECIFY_LUT_EFL },
	{ "lut-file", HAS_ARG, 0, SPECIFY_LUT_FILE },
	{ "file", HAS_ARG, 0, 'f' },
	{ "verbose", NO_ARG, 0, 'v' },
	{ "level", HAS_ARG, 0, 'l' },
	{ "zero", NO_ARG, 0, SPECIFY_FORCE_ZERO },
	{ "h-disable", NO_ARG, 0, SPECIFY_H_DISABLE },
	{ "v-disable", NO_ARG, 0, SPECIFY_V_DISABLE },
	{ 0, 0, 0, 0 }
};

static const struct hint_s hint[] =
{
	{ "", "\t\tFull FOV circle radius (pixel) in vin" },
	{ "0~360", "\tLens full FOV in degree" },
	{ "0|1|2", "\tLens projection mode. 0: equidistant (Linear scaled, r = f * theta), 1: Stereographic (conform, r = 2 * f * tan(theta/2), 2: Look up table for r and theta" },
	{ "axb", "\tLens circle center in pre main." },
	{ "0~3", "\t\t0: No transform, 1: Normal, 2: Panorama, 3: Subregion" },
	{ "-90~90", "\tLens Pitch in degree" },
	{ "-90~90", "\tLens Yaw in degree" },
	{ "-10~10", "\tLens Rotate in degree" },
	{ "a/b", "\t\tZoom factor. a<b: zoom out (Wall/Ceiling Normal, Wall Panorama), a>b: zoom in (for all mode expect no transform" },
	{ "a/b", "\t\tSpecify Horizontal Zoom" },
	{ "a/b", "\t\tSpecify Vertical Zoom" },

	{ "0~180", "\t(Wall panorama)Panorama horizontal angle." },
	{ "-90~90", "\t(Wall Subregion)Pan angle. -90~90 for wall mount." },
	{ "-90~90", "\t(Wall Subregion)Tilt angle. -90~90 for wall mount." },
	{ "axb", "\t\t(Wall Subregion) ROI center offset to the circle center. Negative is left/top and positive is right/bottom" },
	{ "axb", "\t\t(No Transform) ROI offset in vin." },
	{ "axb", "\t\t(No Transform) ROI size in vin." },

	{ "", "\t\tClear all warp effect" },
	{ ">0", "\tEffective focal length in pixel for the lens using look up table for r and theta." },
	{ "file", "\tLook up table file." },

	{ "file", "\tFile prefix to save vector maps." },
	{ "", "\t\tPrint info" },
	{ "0|1|2", "\tLog level. 0: error, 1: info (default), 2: debug" },
	{ "", "\t\tForce warp table content as zero" },
	{ "", "\t\tDisable H warp" },
	{ "", "\t\tDisable V warp" },
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
	BOLD_PRINT0("  Sensor: ov4689\n");
	BOLD_PRINT0("  Prepare Work:\n");
	BOLD_PRINT0("    test_encode -i0 -f 30 -V480p --hdmi --enc-mode 4 -X --bsize 2688x1512 --bmaxsize 2688x1512 --lens-warp 1 --max-padding-width 256 -A --smaxsize 2688x1512\n");
	BOLD_PRINT0("    test_tuning -a\n");

	printf("\n");
	printf("0. Clear warping effect\n");
	printf("    %s -c\n\n", program_self);
	printf("1.[Wall Normal]  Correct both of horizontal/vertical lines\n");
	printf("    %s  -F 185 -R 736 -m %d\n\n", program_self, WARP_NORMAL);
	printf("2.[Wall Panorama]  Correct vertical lines\n");
	printf("    %s  -F 185 -R 736 -h180 -m %d\n\n", program_self, WARP_PANOR);
	printf("3.[Wall Lens Pitch/Yaw]  \n");
	printf("    %s -F 185 -R 736 -m %d -P 10 -Y 10\n\n", program_self, WARP_NORMAL);
	printf("4.[Panorama Lens Pitch/Yaw]  \n");
	printf("    %s -F 185 -R 736 -h180 -m %d -P 10 -Y 10\n\n", program_self, WARP_PANOR);
	printf("5.[Wall Normal]  Do vertical zoom and horizontal zoom separately\n");
	printf("    %s -F 185 -R 736 -m %d --zh 3/2 --zv 2/1\n\n", program_self, WARP_NORMAL);
}

static void usage(char* program_self)
{
	u32 i;
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
	int ch, value, first, second;
	float first_f, second_f;
	int option_index = 0;
	char lut_file[FILENAME_LENGTH];

	opterr = 0;

	while ((ch = getopt_long(argc, argv, short_opts, long_opts, &option_index))
			!= -1) {
		switch (ch) {
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
			case 'm':
				value = atoi(optarg);
				lens_warp_mode = value;
				break;
			case 'z':
				if (get_two_int(optarg, &first, &second, '/') < 0)
					return -1;
				lens_warp_region.zoom.denom = second;
				lens_warp_region.zoom.num = first;
				lens_warp_region.hor_zoom = lens_warp_region.zoom;
				lens_warp_region.vert_zoom = lens_warp_region.zoom;
				break;
			case 'P':
				value = atoi(optarg);
				if (value > MAX_PIT_YAW_ANGLE || value < -MAX_PIT_YAW_ANGLE) {
					ERROR("Pitch should be within [%d~%d]\n", -MAX_PIT_YAW_ANGLE,
						MAX_PIT_YAW_ANGLE);
					return -1;
				}
				lens_warp_region.pitch = value;
				break;
			case 'Y':
				value = atoi(optarg);
				if (value > MAX_PIT_YAW_ANGLE || value < -MAX_PIT_YAW_ANGLE) {
					ERROR("Yaw should be within [%d~%d]\n", -MAX_PIT_YAW_ANGLE,
						MAX_PIT_YAW_ANGLE);
					return -1;
				}
				lens_warp_region.yaw = value;
				break;
			case SPECIFY_ROTATE_ANGLE:
				value = atoi(optarg);
				if (value > MAX_ROTATE_ANGLE|| value < -MAX_ROTATE_ANGLE) {
					ERROR("Rotate should be within [%d~%d]\n", -MAX_ROTATE_ANGLE,
						MAX_ROTATE_ANGLE);
					return -1;
				}
				lens_warp_region.rotate = value;
				break;
			case 'f':
				value = strlen(optarg);
				if (value >= FILENAME_LENGTH) {
					printf("Filename [%s] is too long [%d] (>%d).\n", optarg,
					    value, FILENAME_LENGTH);
					return -1;
				}
				file_flag = 1;
				strncpy(file, optarg, value);
				break;
			case 'h':
				hor_panor_range = (degree_t) atof(optarg);
				break;
			case 'p':
				pantilt.pan = (degree_t) atof(optarg);
				pantilt_flag = 1;
				break;
			case 't':
				pantilt.tilt = (degree_t) atof(optarg);
				pantilt_flag = 1;
				break;
			case 'r':
				if (get_two_float(optarg, &first_f, &second_f, 'x') < 0)
					return -1;
				roi.x = first_f;
				roi.y = second_f;
				break;
			case SPECIFY_NOTRANS_OFFSET:
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				notrans.upper_left.x = first;
				notrans.upper_left.y = second;
				break;
			case SPECIFY_NOTRANS_SIZE:
				if (get_two_int(optarg, &first, &second, 'x') < 0)
					return -1;
				notrans.width = first;
				notrans.height = second;
				break;

			case SPECIFY_LUT_EFL:
				dewarp_init_param.lut_focal_length = atoi(optarg);
				break;
			case SPECIFY_LUT_FILE:
				snprintf(lut_file, sizeof(lut_file), "%s", optarg);
				break;

			case 'c':
				clear_flag = 1;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'l':
				log_level = atoi(optarg);
				break;
			case SPECIFY_FORCE_ZERO:
				debug_info.force_zero = 1;
				break;
			case SPECIFY_H_DISABLE:
				debug_info.h_disable = 1;
				break;
			case SPECIFY_V_DISABLE:
				debug_info.v_disable = 1;
				break;
			case SPECIFY_H_ZOOM:
				if (get_two_int(optarg, &first, &second, '/') < 0)
					return -1;
				lens_warp_region.hor_zoom.denom = second;
				lens_warp_region.hor_zoom.num = first;
				break;
			case SPECIFY_V_ZOOM:
				if (get_two_int(optarg, &first, &second, '/') < 0)
					return -1;
				lens_warp_region.vert_zoom.denom = second;
				lens_warp_region.vert_zoom.num = first;
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

static void save_warp_table_to_file(struct iav_warp_main* p_control)
{
	FILE *fp;
	int i, j;
	s16* addr;

	char fullname[FILENAME_LENGTH];

	snprintf(fullname, sizeof(fullname), "%s_hor", file);
	if ((fp = fopen(fullname, "w+")) == NULL) {
		printf("### Cannot write file [%s].\n", fullname);
		return;
	}
	addr = (s16*)warp_addr;
	for (i = 0; i < p_control->area[0].h_map.output_grid_row; i++) {
		for (j = 0; j < p_control->area[0].h_map.output_grid_col; j++) {
			fprintf(fp, "%d\t",
			    *(addr + i * p_control->area[0].h_map.output_grid_col + j));
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
	fp = NULL;
	printf("Save hor table to [%s].\n", fullname);

	snprintf(fullname, sizeof(fullname), "%s_ver", file);
	if ((fp = fopen(fullname, "w+")) == NULL) {
		printf("### Cannot write file [%s].\n", fullname);
		return;
	}
	addr = (s16*)warp_addr + MAX_WARP_TABLE_SIZE_LDC;
	for (i = 0; i < p_control->area[0].v_map.output_grid_row; i++) {
		for (j = 0; j < p_control->area[0].v_map.output_grid_col; j++) {
			fprintf(fp, "%d\t",
			    *(addr + i * p_control->area[0].v_map.output_grid_col + j));
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
	printf("Save ver table to [%s].\n", fullname);

	snprintf(fullname, sizeof(fullname), "%s_me1_ver", file);
	if ((fp = fopen(fullname, "w+")) == NULL) {
		printf("### Cannot write file [%s].\n", fullname);
		return;
	}
	addr = (s16*)warp_addr + MAX_WARP_TABLE_SIZE_LDC * 2;
	for (i = 0; i < lens_warp_vector.me1_ver_map.rows; i++) {
		for (j = 0; j < lens_warp_vector.me1_ver_map.cols; j++) {
			fprintf(fp, "%d\t",
			    *(addr + i * lens_warp_vector.me1_ver_map.cols + j));
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

static void update_warp_control(struct iav_warp_main* p_control)
{
	struct iav_warp_area* p_area = &p_control->area[0];
	warp_vector_t *p_vector = &lens_warp_vector;

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

	if (debug_info.h_disable) {
		p_area->h_map.enable = 0;
	} else {
		p_area->h_map.enable = (p_vector->hor_map.rows > 0
			&& p_vector->hor_map.cols > 0);
	}
	p_area->h_map.h_spacing =
		get_grid_spacing(p_vector->hor_map.grid_width);
	p_area->h_map.v_spacing =
		get_grid_spacing(p_vector->hor_map.grid_height);
	p_area->h_map.output_grid_row = p_vector->hor_map.rows;
	p_area->h_map.output_grid_col = p_vector->hor_map.cols;
	p_area->h_map.data_addr_offset = 0;

	if (debug_info.v_disable) {
		p_area->v_map.enable = 0;
	} else {
		p_area->v_map.enable = (p_vector->ver_map.rows > 0
			&& p_vector->ver_map.cols > 0);
	}
	p_area->v_map.h_spacing =
		get_grid_spacing(p_vector->ver_map.grid_width);
	p_area->v_map.v_spacing =
		get_grid_spacing(p_vector->ver_map.grid_height);
	p_area->v_map.output_grid_row = p_vector->ver_map.rows;
	p_area->v_map.output_grid_col = p_vector->ver_map.cols;
	p_area->v_map.data_addr_offset = MAX_WARP_TABLE_SIZE_LDC * sizeof(s16);

	// ME1 Warp grid / spacing
	p_area->me1_v_map.h_spacing =
		get_grid_spacing(p_vector->me1_ver_map.grid_width);
	p_area->me1_v_map.v_spacing =
		get_grid_spacing(p_vector->me1_ver_map.grid_height);
	p_area->me1_v_map.output_grid_row = p_vector->me1_ver_map.rows;
	p_area->me1_v_map.output_grid_col = p_vector->me1_ver_map.cols;
	p_area->me1_v_map.data_addr_offset = MAX_WARP_TABLE_SIZE_LDC * sizeof(s16) * 2;

	if (verbose) {
		printf("\tInput size %dx%d, offset %dx%d\n", p_area->input.width,
			p_area->input.height, p_area->input.x, p_area->input.y);
		printf("\tOutput size %dx%d, offset %dx%d\n", p_area->output.width,
			p_area->output.height,
			p_area->output.x, p_area->output.y);

		printf("\tHor Table %dx%d, grid %dx%d (%dx%d)\n",
			p_area->h_map.output_grid_col, p_area->h_map.output_grid_row,
			p_area->h_map.h_spacing,
			p_area->h_map.v_spacing,
			p_vector->hor_map.grid_width, p_vector->hor_map.grid_height);
		printf("\tVer Table %dx%d, grid %dx%d (%dx%d)\n",
			p_area->v_map.output_grid_col, p_area->v_map.output_grid_row,
			p_area->v_map.h_spacing,
			p_area->v_map.v_spacing,
			p_vector->ver_map.grid_width, p_vector->ver_map.grid_height);
		printf("\tME1 Ver Table %dx%d, grid %dx%d (%dx%d)\n",
			p_area->me1_v_map.output_grid_col, p_area->me1_v_map.output_grid_row,
			p_area->me1_v_map.h_spacing,
			p_area->me1_v_map.v_spacing,
			p_vector->me1_ver_map.grid_width, p_vector->me1_ver_map.grid_height);
	}
}

static int check_warp_control(struct iav_warp_main *p_control)
{
	struct iav_warp_area * p_area = &p_control->area[0];

	if (!p_area->input.height || !p_area->input.width || !p_area->output.width
	    ||
	    !p_area->output.height) {
		printf("input size %dx%d, output size %dx%d cannot be 0\n",
		    p_area->input.width, p_area->input.height,
		    p_area->output.width, p_area->output.height);
		return -1;
	}

	if ((p_area->input.x + p_area->input.width > vin.width)
	    ||
	    (p_area->input.y + p_area->input.height > vin.height)) {
		printf(
		    "input size %dx%d + offset %dx%d is out of vin %dx%d.\n",
		    p_area->input.width, p_area->input.height,
		    p_area->input.x, p_area->input.y,
		    vin.width, vin.height);
		return -1;
	}

	if ((p_area->output.x + p_area->output.width > main_buffer.size.width)
	    ||
	    (p_area->output.y + p_area->output.height > main_buffer.size.height)) {
			printf("output size %dx%d + offset %dx%d is out of main %dx%d.\n",
		    p_area->output.width, p_area->output.height,
		    p_area->output.x, p_area->output.y,
		    main_buffer.size.width, main_buffer.size.height);
	}

	return 0;
}

static int create_lens_warp_vector(void)
{
	char mode_str[MODE_STRING_LENGTH];
	struct timespec lasttime, curtime;

	if (verbose) {
		clock_gettime(CLOCK_REALTIME, &lasttime);
	}
	lens_warp_region.output.width = main_buffer.size.width;
	lens_warp_region.output.height = main_buffer.size.height;
	lens_warp_region.output.upper_left.x = 0;
	lens_warp_region.output.upper_left.y = 0;

	switch (lens_warp_mode) {
		case WARP_NO_TRANSFORM:
			snprintf(mode_str, MODE_STRING_LENGTH, "No Transform");
			if (lens_no_transform(&lens_warp_region,
			    &notrans, &lens_warp_vector) <= 0) {
				return -1;
			}
			break;
		case WARP_NORMAL:
			snprintf(mode_str, MODE_STRING_LENGTH, "Wall Normal");
			if (lens_wall_normal(&lens_warp_region, &lens_warp_vector) <= 0)
				return -1;
			break;

		case WARP_PANOR:
			snprintf(mode_str, MODE_STRING_LENGTH, "Wall Panorama");
			if (lens_wall_panorama(&lens_warp_region, hor_panor_range,
			    &lens_warp_vector) <= 0)
				return -1;
			break;

		case WARP_SUBREGION:
			snprintf(mode_str, MODE_STRING_LENGTH, "Wall Subregion");
			if (pantilt_flag) {
				if (lens_wall_subregion_angle(
				    &lens_warp_region, &pantilt,
				    &lens_warp_vector, &roi) <= 0)
					return -1;
			} else {
				if (lens_wall_subregion_roi(
				    &lens_warp_region, &roi,
				    &lens_warp_vector, &pantilt) <= 0)
					return -1;
			}
			break;

		default:
			break;
	}
	return 0;

	if (verbose) {
		printf("\nLens Warp: %s \n", mode_str);

		clock_gettime(CLOCK_REALTIME, &curtime);
		printf("Elapsed Time (ms): [%05ld]\n",
		    (curtime.tv_sec - lasttime.tv_sec) * 1000
		        + (curtime.tv_nsec - lasttime.tv_nsec) / 1000000);
		if (lens_warp_mode == WARP_SUBREGION) {
			if (pantilt_flag) {
				printf("pan %f, tilt %f => ROI (%f, %f)\n",
				    pantilt.pan, pantilt.tilt,
				    roi.x, roi.y);
			} else {
				printf("ROI (%f, %f) => pan %f, tilt %f\n",
				    roi.x, roi.y,
				    pantilt.pan, pantilt.tilt);
			}
		}
	}

	return 0;
}

static int set_lens_warp(void)
{
	struct iav_warp_ctrl warp_control;
	struct iav_warp_main* warp_main = &warp_control.arg.main;
	u32 flags = (1 << IAV_WARP_CTRL_MAIN);

	memset(&warp_control, 0, sizeof(struct iav_warp_ctrl));
	warp_control.cid = IAV_WARP_CTRL_MAIN;
	if (!clear_flag) {
		if (create_lens_warp_vector() < 0) {
			return -1;
		}
		update_warp_control(warp_main);
		if (check_warp_control(warp_main) < 0) {
			return -1;
		}
		if (debug_info.force_zero) {
			memset((void *)lens_warp_vector.hor_map.addr, 0, MAX_WARP_TABLE_SIZE_LDC);
			memset((void *)lens_warp_vector.ver_map.addr, 0, MAX_WARP_TABLE_SIZE_LDC);
			memset((void *)lens_warp_vector.me1_ver_map.addr, 0, MAX_WARP_TABLE_SIZE_LDC);
		}
		if (file_flag) {
			save_warp_table_to_file(warp_main);
		}
	}

	AM_IOCTL(fd_iav, IAV_IOC_CFG_WARP_CTRL, &warp_control);
	AM_IOCTL(fd_iav, IAV_IOC_APPLY_WARP_CTRL, &flags);
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
	printf("map_warp: start = %p, total size = 0x%x\n", warp_addr, warp_size);
	return 0;
}

static int init_lens_warp(void)
{
	version_t version;
	struct vindev_video_info video_info;

	if (!clear_flag) {
		if (map_warp() < 0) {
			return -1;
		}
		if (dewarp_get_version(&version) < 0) {
			return -1;
		}
		if (verbose) {
			printf(
			    "\nDewarp Library Version: %s-%d.%d.%d (Last updated: %x)\n\n",
			    version.description, version.major, version.minor,
			    version.patch, version.mod_time);
		}
		if (set_log(log_level, "stderr") < 0){
			return -1;
		}

		lens_warp_vector.hor_map.addr = (data_t*) warp_addr;
		lens_warp_vector.ver_map.addr = (data_t*) (warp_addr +
			MAX_WARP_TABLE_SIZE_LDC * sizeof(data_t));
		lens_warp_vector.me1_ver_map.addr = (data_t*) (warp_addr +
			MAX_WARP_TABLE_SIZE_LDC * sizeof(data_t) * 2);

		main_buffer.buf_id = 0;
		AM_IOCTL(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT, &main_buffer);
		vin.mode = AMBA_VIDEO_MODE_CURRENT;
		video_info.vsrc_id = 0;
		video_info.info.mode = vin.mode;
		AM_IOCTL(fd_iav, IAV_IOC_VIN_GET_VIDEOINFO, &video_info);
		memcpy(&vin, &video_info.info, sizeof(struct amba_video_info));
		dewarp_init_param.max_input_width = vin.width;
		dewarp_init_param.max_input_height = vin.height;

		if (!dewarp_init_param.lens_center_in_max_input.x
		    || !dewarp_init_param.lens_center_in_max_input.y) {
			dewarp_init_param.lens_center_in_max_input.x =
				dewarp_init_param.max_input_width / 2;
			dewarp_init_param.lens_center_in_max_input.y =
				dewarp_init_param.max_input_height / 2;
		}

		dewarp_init_param.main_buffer_width = main_buffer.size.width;

		if (dewarp_init(&dewarp_init_param) < 0)
			return -1;
	}

	return 0;
}

static int deinit_lens_warp(void)
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
		usage(argv[0]);
		return -1;
	}

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}
	if (init_param(argc, argv) < 0) {
		return -1;
	}

	if (init_lens_warp() < 0) {
		ret = -1;
	} else if (set_lens_warp() < 0) {
		ret = -1;
	}
	if (deinit_lens_warp() < 0) {
		ret = -1;
	}
	return ret;
}

