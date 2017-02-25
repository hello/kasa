/*
 * test_dptz.c
 * the program can change digital ptz parameters
 *
 * History:
 *	2010/2/20 - [Louis Sun] created file
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

#define	ROUND_UP(x, n)			( ((x)+(n)-1u) & ~((n)-1u) )
#define	ROUND_DOWN(size, align)	((size) & ~((align) - 1))

#define	MAX_SOURCE_BUFFER_NUM	(4)
#define	MIN_PAN_TILT_STEP			(8)

#define	MAX_DZ_TYPE_I_ZOOM		(8)
#define	ZOOM_FACTOR_Y				(4 << 16)
#define	OFFSET_Y_MARGIN			(4)

// the device file handle
static int fd_iav;
static int current_source = -1;	// -1 is a invalid source buffer, for initialize data only

// digital zoom type II format
typedef struct dptz_format_s {
	int dz_numer;
	int dz_denom;
	int dz_times_flag;
	int width;
	int height;
	int resolution_flag;
	int offset_x;
	int offset_y;
	int offset_flag;
} dptz_format_t;

static dptz_format_t dptz_format[MAX_SOURCE_BUFFER_NUM];
static int dptz_format_change_id = 0;

//auto run zoom demo
static int autorun_zoom_I_flag = 0;
static int autorun_zoom_II_flag = 0;

// digital pan format
typedef struct pan_tilt_format_s {
	int offset_start_x;
	int offset_start_y;
	int offset_end_x;
	int offset_end_y;
} pan_tilt_format_t;

static pan_tilt_format_t pt_format[MAX_SOURCE_BUFFER_NUM];
static int pan_format_change_id = 0;

//auto run pan tilt demo
static int pan_tilt_step = MIN_PAN_TILT_STEP;
static int autorun_pan_I_flag = 0;
//static int autorun_pan_II_flag = 0;

// options and usage
#define NO_ARG		0
#define HAS_ARG		1
static struct option long_options[] = {
	{"main-buffer", NO_ARG, 0, 'X'},
	{"sub-buffer", NO_ARG, 0, 'B'},
	{"buffer2", NO_ARG, 0, 'Y'},
	{"buffer3", NO_ARG, 0, 'J'},
	{"buffer4", NO_ARG, 0, 'K'},
	{"default-zoom", HAS_ARG, 0, 'd'},
	{"pt-x-range", HAS_ARG, 0, 'm'},
	{"pt-y-range", HAS_ARG, 0, 'n'},
	{"pt-step", HAS_ARG, 0, 't'},
	{"auto-pt", NO_ARG, 0, 'p'},
	{"autozoom", NO_ARG, 0, 'r'},
	{"autozoom2", NO_ARG, 0, 'R'},
	{"zoom-window-size", HAS_ARG, 0, 's'},
	{"zoom-window-offset", HAS_ARG, 0, 'o'},
	{0, 0, 0, 0}
};

static const char *short_options = "B:d:m:n:t:prRXYJKs:o:";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"", "\tDPTZ type I config for main buffer"},
	{"1~3", "\tDPTZ type II config for sub buffer"},
	{"", "\t\tDPTZ type II config for second buffer"},
	{"", "\t\tDPTZ type II config for third buffer"},
	{"", "\t\tDPTZ type II config for fourth buffer"},
	{"m/n", "\tdefault zoom times from center. m > n: zoom in, m < n: zoom out, m = 0: default zoom from main buffer"},
	{"A~B", "\tdefault range is from left to right"},
	{"A~B", "\tdefault range is from top to bottom"},
	{"", "\t\tspecify steps in width and height direction, minimum step is 8"},
	{"", "\t\trun auto pan tilt for type I with specified width and height range"},
	{"", "\t\trun auto zoom type I from 1X to 8X"},
	{"", "\t\trun auto zoom type II"},
	{"", "\tDPTZ zoom out window size"},
	{"", "\tDPTZ zoom out window offset"},
};

void usage(void)
{
	int i;
	printf("test_dptz usage:\n");
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
	printf("\nExamples:\n  Type I:\n    test_dptz -X -d 2 -o 100x100\n");
	printf("    test_dptz -X -d 1 -m 0~600 -n 0~400 -p\n");
	printf("\n  Type II:\n    test_dptz -Y -s 720x480 -o 100x200\n");
	printf("\n  Type II:\n    test_dptz -Y -d 1 -o 100x200\n");
	printf("\n  Type II:\n    test_dptz -Y -d 1/2\n");
	printf("\n");
}

// first and second values must be in format of "AxB"
static int get_two_values(const char *name, int *first, int *second, char delimiter)
{
	char tmp_string[16];
	char * separator;

	separator = strchr(name, delimiter);
	if (!separator) {
		printf("two values should be like A%cB .\n", delimiter);
		return -1;
	}

	strncpy(tmp_string, name, separator - name);
	tmp_string[separator - name] = '\0';
	*first = atoi(tmp_string);
	strncpy(tmp_string, separator + 1,  name + strlen(name) - separator);
	*second = atoi(tmp_string);

//	printf("input string [%s], first value %d, second value %d.\n", name, *first, *second);
	return 0;
}

// first and second values must be in format of "AxB"
static int get_two_zoomfactor(const char *name, int *first, int *second, char delimiter)
{
	char tmp_string[16];
	char * separator;

	separator = strchr(name, delimiter);

	if (separator) {
		strncpy(tmp_string, name, separator - name);
		tmp_string[separator - name] = '\0';
		*first = atoi(tmp_string);
		strncpy(tmp_string, separator + 1,  name + strlen(name) - separator);
		*second = atoi(tmp_string);
	} else {
		*first = atoi(name);
		*second = 1;
	}
	return 0;
}


#define	VERIFY_BUFFERID(x)		do {		\
			if ((x) < 0 || ((x) >= MAX_SOURCE_BUFFER_NUM)) {	\
				printf("Wrong buffer id %d.\n", (x));		\
				return -1;	\
			}		\
		} while (0)

#define	VERIFY_SUB_BUFFERID(x)		do {		\
			if ((x) < 1 || ((x) >= MAX_SOURCE_BUFFER_NUM)) {	\
				printf("Wrong sub buffer id %d.\n", (x));		\
				return -1;	\
			}		\
		} while (0)


int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	int first, second;
	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		// parameters for DPTZ type I
		case 'X':
			current_source = 0;
			break;
		case 'd':
			VERIFY_BUFFERID(current_source);
			if (get_two_zoomfactor(optarg, &first, &second, '/') < 0)
				return -1;
			if (dptz_format[current_source].resolution_flag) {
				printf("Cannot run \"-d\" and \"-s\" together for buf [%d]!\n",
					current_source);
				return -1;
			}
			dptz_format[current_source].dz_numer = first;
			dptz_format[current_source].dz_denom = second;
			dptz_format[current_source].dz_times_flag = 1;
			dptz_format_change_id |= (1 << current_source);
			break;
		case 'm':
			if (get_two_values(optarg, &first, &second, '~') < 0)
				return -1;
			pt_format[0].offset_start_x = first;
			pt_format[0].offset_end_x = second;
			pan_format_change_id |= (1 << 0);
			break;
		case 'n':
			if (get_two_values(optarg, &first, &second, '~') < 0)
				return -1;
			pt_format[0].offset_start_y = first;
			pt_format[0].offset_end_y = second;
			pan_format_change_id |= (1 << 0);
			break;
		case 'p':
			autorun_pan_I_flag = 1;
			break;
		case 't':
			pan_tilt_step = atoi(optarg);
			break;
		case 'r':
			autorun_zoom_I_flag = 1;
			break;

		// parameters for DPTZ type II
		case 'B':
			current_source = atoi(optarg);
			VERIFY_SUB_BUFFERID(current_source);
			break;
		case 'Y':
			current_source = 1;
			break;
		case 'J':
			current_source = 2;
			break;
		case 'K':
			current_source = 3;
			break;
		case 's':
			VERIFY_BUFFERID(current_source);
			if (dptz_format[current_source].dz_times_flag) {
				printf("Cannot run \"-d\" and \"-s\" together for buf [%d]!\n",
					current_source);
				return -1;
			}
			if (get_two_values(optarg, &first, &second, 'x') < 0)
				return -1;
			dptz_format[current_source].width = first;
			dptz_format[current_source].height = second;
			dptz_format[current_source].resolution_flag = 1;
			dptz_format_change_id |= (1 << current_source);
			break;
		case 'o':
			VERIFY_BUFFERID(current_source);
			if (get_two_values(optarg, &first, &second, 'x') < 0)
				return -1;
			dptz_format[current_source].offset_x = first;
			dptz_format[current_source].offset_y = second;
			dptz_format[current_source].offset_flag = 1;
			dptz_format_change_id |= (1 << current_source);
			break;
		case 'R':
			VERIFY_SUB_BUFFERID(current_source);
			autorun_zoom_II_flag = current_source;
			break;

		default:
			printf("unknown command %s \n", optarg);
			return -1;
			break;
		}
	}
	return 0;
}


static int digital_ptz_for_main_buffer(void)
{
	struct vindev_video_info video_info;
	struct iav_srcbuf_setup setup;
	struct iav_digital_zoom digital_zoom;
	int cap_w, cap_h, numer, denom;

	memset(&setup, 0, sizeof(setup));
	if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP, &setup) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_SETUP");
		return -1;
	}
	memset(&digital_zoom, 0, sizeof(digital_zoom));
	digital_zoom.buf_id = IAV_SRCBUF_MN;
	if (ioctl(fd_iav, IAV_IOC_GET_DIGITAL_ZOOM, &digital_zoom) < 0) {
		perror("IAV_IOC_GET_DIGITAL_ZOOM");
		return -1;
	}
	if (dptz_format[0].dz_times_flag) {
		numer = dptz_format[0].dz_numer;
		denom = dptz_format[0].dz_denom;

		video_info.vsrc_id = 0;
		video_info.info.mode = AMBA_VIDEO_MODE_CURRENT;
		if (ioctl(fd_iav, IAV_IOC_VIN_GET_VIDEOINFO, &video_info) < 0) {
			perror("IAV_IOC_VIN_GET_VIDEOINFO");
			return -1;
		}
		if (numer == 0) {
			cap_w = video_info.info.width;
			cap_h = video_info.info.height;
		} else {
			cap_w = ROUND_UP(setup.size[0].width * denom / numer, 4);
			cap_h = ROUND_UP(setup.size[0].height * denom / numer, 4);
		}
		digital_zoom.input.width = cap_w;
		digital_zoom.input.height = cap_h;
		digital_zoom.input.x = ROUND_DOWN((dptz_format[0].offset_flag ?
				dptz_format[0].offset_x : (video_info.info.width - cap_w) / 2), 2);
		digital_zoom.input.y = ROUND_DOWN((dptz_format[0].offset_flag ?
				dptz_format[0].offset_y : (video_info.info.height - cap_h) / 2), 2);
	} else if (dptz_format[0].resolution_flag) {
		digital_zoom.input.width = dptz_format[0].width;
		digital_zoom.input.height = dptz_format[0].height;
	}
	if (dptz_format[0].offset_flag) {
		digital_zoom.input.x = dptz_format[0].offset_x;
		digital_zoom.input.y = dptz_format[0].offset_y;
	}

	printf("(DPTZ Type I) Zoom Window %dx%d, offset %dx%d.\n",
			digital_zoom.input.width, digital_zoom.input.height,
			digital_zoom.input.x, digital_zoom.input.y);
	if (ioctl(fd_iav, IAV_IOC_SET_DIGITAL_ZOOM, &digital_zoom) < 0) {
		perror("IAV_IOC_SET_DIGITAL_ZOOM");
		return -1;
	}

	return 0;
}

static int digital_ptz_for_preview_buffer(void)
{
	int i, numer, denom;
	struct iav_digital_zoom digital_zoom;
	struct iav_srcbuf_format buf_format, main_format;

	memset(&main_format, 0, sizeof(main_format));
	main_format.buf_id = IAV_SRCBUF_MN;
	if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT, &main_format) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT");
		return -1;
	}

	for (i = 1; i < MAX_SOURCE_BUFFER_NUM; i++) {
		if (dptz_format_change_id & (1 << i)) {
			memset(&digital_zoom, 0, sizeof(digital_zoom));
			digital_zoom.buf_id = i;
			if (ioctl(fd_iav, IAV_IOC_GET_DIGITAL_ZOOM, &digital_zoom) < 0) {
				perror("IAV_IOC_GET_DIGITAL_ZOOM");
				return -1;
			}
			memset(&buf_format, 0, sizeof(buf_format));
			buf_format.buf_id = i;
			if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT, &buf_format) < 0) {
				perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT");
				return -1;
			}
			if (dptz_format[i].dz_times_flag) {
				numer = dptz_format[i].dz_numer;
				denom = dptz_format[i].dz_denom;
				digital_zoom.input.width = numer ?
						ROUND_UP(buf_format.size.width * denom / numer, 2) :
						main_format.size.width;
				digital_zoom.input.height = numer ?
						ROUND_UP(buf_format.size.height * denom / numer, 4) :
						main_format.size.height;
			} else if (dptz_format[i].resolution_flag) {
				digital_zoom.input.width = dptz_format[i].width;
				digital_zoom.input.height = dptz_format[i].height;
			}
			if (dptz_format[i].offset_flag) {
				digital_zoom.input.x = dptz_format[i].offset_x;
				digital_zoom.input.y = dptz_format[i].offset_y;
			} else if (main_format.size.width >= digital_zoom.input.width
				&& main_format.size.height >= digital_zoom.input.height) {
				digital_zoom.input.x = ROUND_UP((main_format.size.width -
						digital_zoom.input.width) / 2, 2);
				digital_zoom.input.y = ROUND_UP((main_format.size.height -
						digital_zoom.input.height) / 2, 4);
			}
			printf("(DPTZ Type II) zoom out window size : %dx%d, offset : %dx%d\n",
				digital_zoom.input.width, digital_zoom.input.height,
				digital_zoom.input.x, digital_zoom.input.y);
			if (ioctl(fd_iav, IAV_IOC_SET_DIGITAL_ZOOM, &digital_zoom) < 0) {
				perror("IAV_IOC_SET_DIGITAL_ZOOM");
				return -1;
			}
		}
	}

	return 0;
}

static int check_pan_tilt_param(void)
{
	struct vindev_video_info video_info;
	struct iav_digital_zoom digital_zoom;
	struct iav_srcbuf_setup setup;
	int max_offset_x, max_offset_y, cap_w, cap_h, adjust = 0, zoom_y;

	memset(&digital_zoom, 0, sizeof(digital_zoom));
	digital_zoom.buf_id = IAV_SRCBUF_MN;
	if (ioctl(fd_iav, IAV_IOC_GET_DIGITAL_ZOOM, &digital_zoom) < 0) {
		perror("IAV_IOC_GET_DIGITAL_ZOOM");
		return -1;
	}
	memset(&video_info, 0, sizeof(video_info));
	video_info.vsrc_id = 0;
	video_info.info.mode = AMBA_VIDEO_MODE_CURRENT;
	if (ioctl(fd_iav, IAV_IOC_VIN_GET_VIDEOINFO, &video_info) < 0) {
		perror("IAV_IOC_VIN_GET_VIDEOINFO");
		return -1;
	}
	memset(&setup, 0, sizeof(setup));
	if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP, &setup) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_SETUP");
		return -1;
	}
	cap_w = digital_zoom.input.width;
	cap_h = digital_zoom.input.height;
	max_offset_x = video_info.info.width - cap_w;
	max_offset_y = video_info.info.height - cap_h;
	zoom_y = (setup.size[0].height << 16) / digital_zoom.input.height;
	if (zoom_y >= ZOOM_FACTOR_Y) {
		max_offset_y -= OFFSET_Y_MARGIN;
		if (pt_format[0].offset_start_y < OFFSET_Y_MARGIN) {
			pt_format[0].offset_start_y = OFFSET_Y_MARGIN;
		}
	}
	if (pt_format[0].offset_start_x < 0) {
		adjust = 1;
		pt_format[0].offset_start_x = 0;
	}
	if (pt_format[0].offset_end_x > max_offset_x) {
		adjust = 1;
		pt_format[0].offset_end_x = max_offset_x;
	}
	if (pt_format[0].offset_start_x > pt_format[0].offset_end_x) {
		adjust = 1;
		pt_format[0].offset_start_x = pt_format[0].offset_end_x;
	}

	if (pt_format[0].offset_start_y < 0) {
		adjust = 1;
		pt_format[0].offset_start_y = 0;
	}
	if (pt_format[0].offset_end_y > max_offset_y) {
		adjust = 1;
		pt_format[0].offset_end_y = max_offset_y;
	}
	if (pt_format[0].offset_start_y > pt_format[0].offset_end_y) {
		adjust = 1;
		pt_format[0].offset_start_y = pt_format[0].offset_end_y;
	}
	if (pan_tilt_step < MIN_PAN_TILT_STEP) {
		pan_tilt_step = MIN_PAN_TILT_STEP;
	}

	if (adjust) {
		printf("Adjust auto pan tilt param: center offset x range (%d, %d),"
			" center offset y range (%d, %d).\n",
			pt_format[0].offset_start_x, pt_format[0].offset_end_x,
			pt_format[0].offset_start_y, pt_format[0].offset_end_y);
	}
	return 0;
}

static int auto_pan_tilt_for_main(void)
{
	struct iav_digital_zoom digital_zoom;
	int x, y, dx, dy;
	int dir, time;

	memset(&digital_zoom, 0, sizeof(digital_zoom));
	digital_zoom.buf_id = IAV_SRCBUF_MN;
	if (ioctl(fd_iav, IAV_IOC_GET_DIGITAL_ZOOM, &digital_zoom) < 0) {
		perror("IAV_IOC_GET_DIGITAL_ZOOM");
		return -1;
	}
	if (check_pan_tilt_param() < 0) {
		printf("Failed to do auto pan / tilt for DPTZ type I!\n");
		return -1;
	}

	dx = (pt_format[0].offset_end_x - pt_format[0].offset_start_x) / pan_tilt_step;
	dy = (pt_format[0].offset_end_y - pt_format[0].offset_start_y) / pan_tilt_step * 2;
	x = pt_format[0].offset_start_x;
	y = pt_format[0].offset_start_y;
	dir = 1;
	while (1) {
		digital_zoom.input.x = (x & (~0x1));
		digital_zoom.input.y = (y & (~0x1));
//		printf("+++++++++++++++++++ center offset : %dx%d ==> %d x %d.\n", x, y,
//			digital_zoom.center_offset_x, digital_zoom.center_offset_y);
		if (ioctl(fd_iav, IAV_IOC_SET_DIGITAL_ZOOM, &digital_zoom) < 0) {
			perror("IAV_IOC_SET_DIGITAL_ZOOM");
			return -1;
		}
		time = 200 * 1000;
		x += dir * dx;
		if (x > pt_format[0].offset_end_x) {
			x = pt_format[0].offset_end_x;
			dir = -1;
			y += dy;
		}
		if (x < pt_format[0].offset_start_x) {
			x = pt_format[0].offset_start_x;
			dir = 1;
			y += dy;
		}
		if (y > pt_format[0].offset_end_y) {
			x = pt_format[0].offset_start_x;
			y = pt_format[0].offset_start_y;
			dir = 1;
		}
		usleep(time);
	}
	return 0;
}

static int auto_zoom_for_main(void)
{
	//multi step zoom from 1X to 10X and then back to 1X,  in multi steps
	int zoom_factor;
	int i, min_w, min_h, max_w, max_h, round_w, round_x;
	int align_size, align_offset;
	u32 vin_w, vin_h;
	struct iav_digital_zoom digital_zoom;
	struct iav_srcbuf_format main_buffer;
	struct iav_system_resource resource_setup;

	int cap_w, cap_h;
	struct iav_enc_buf_cap cap_buf;
	struct vindev_video_info video_info;

	video_info.vsrc_id = 0;
	video_info.info.mode = AMBA_VIDEO_MODE_CURRENT;
	if (ioctl(fd_iav, IAV_IOC_VIN_GET_VIDEOINFO, &video_info) < 0) {
		perror("IAV_IOC_VIN_GET_VIDEOINFO");
		return -1;
	}
	vin_w = video_info.info.width;
	vin_h = video_info.info.height;
	main_buffer.buf_id = IAV_SRCBUF_MN;
	if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT, &main_buffer) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT");
		return -1;
	}

	memset(&cap_buf, 0, sizeof(cap_buf));
	cap_buf.buf_id = IAV_SRCBUF_MN;
	if (ioctl(fd_iav, IAV_IOC_QUERY_ENC_BUF_CAP, &cap_buf) < 0) {
		perror("IAV_IOC_QUERY_ENCMODE_CAP");
		return -1;
	}

	resource_setup.encode_mode = DSP_CURRENT_MODE;
	if (ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &resource_setup) < 0) {
		perror("IAV_IOC_GET_SYSTEM_RESOURCE");
		return -1;
	}
	align_size = (resource_setup.encode_mode == DSP_HIGH_MP_LOW_DELAY_MODE ?
			32 : 4);
	align_offset = (resource_setup.encode_mode == DSP_HIGH_MP_LOW_DELAY_MODE ?
			32 : 2);

	min_w = ROUND_UP(main_buffer.size.width / cap_buf.max_zoom_out_factor, align_size);
	max_w = MIN(ROUND_DOWN(main_buffer.size.width *
		cap_buf.max_zoom_in_factor, align_size), vin_w);

	min_h = ROUND_UP(main_buffer.size.height / cap_buf.max_zoom_out_factor, 4);
	max_h = MIN(ROUND_DOWN(main_buffer.size.height *
		cap_buf.max_zoom_in_factor, 4),  vin_h);

	digital_zoom.buf_id = 0;
	for (i = 0; i <= (cap_buf.max_zoom_in_factor - 1) * 40; i++) {
		zoom_factor = 65536 + 65536 * i / 40;
		cap_w = (main_buffer.size.width << 16) / zoom_factor;
		cap_h = (main_buffer.size.height << 16) / zoom_factor;
		if (cap_w > min_w && cap_h > min_h && cap_w < max_w && cap_h < max_h) {
			round_w = ROUND_UP(cap_w, align_size);
			round_x = ROUND_DOWN((vin_w - round_w) / 2, align_offset);
			if (round_w == digital_zoom.input.width || round_x == digital_zoom.input.x) {
				continue;
			}
			digital_zoom.input.width = round_w;
			digital_zoom.input.x = round_x;
			digital_zoom.input.height = ROUND_UP(cap_h, 4);
			digital_zoom.input.y =
			    ROUND_DOWN((vin_h - digital_zoom.input.height) / 2, 2);
			if (ioctl(fd_iav, IAV_IOC_SET_DIGITAL_ZOOM, &digital_zoom) < 0) {
				perror("IAV_IOC_SET_DIGITAL_ZOOM");
				return -1;
			}
			usleep(1000 * 33);	//zoom at about 1 fps
		}
	}

	for (i = (cap_buf.max_zoom_out_factor - 1) * 40; i >= 0; i--) {
		zoom_factor = 65536 + 65536 * i / 40;
		cap_w = (main_buffer.size.width << 16) / zoom_factor;
		cap_h = (main_buffer.size.height << 16) / zoom_factor;
		if (cap_w < max_w && cap_h < max_h && cap_w > min_w && cap_h > min_h) {
			round_w = ROUND_UP(cap_w, align_size);
			round_x = ROUND_DOWN((vin_w - round_w) / 2, align_offset);
			if (round_w == digital_zoom.input.width || round_x == digital_zoom.input.x) {
				continue;
			}
			digital_zoom.input.width = round_w;
			digital_zoom.input.x = round_x;
			digital_zoom.input.height = ROUND_UP(cap_h, 4);
			digital_zoom.input.y =
			    ROUND_DOWN((vin_h - digital_zoom.input.height) / 2, 2);
			if (ioctl(fd_iav, IAV_IOC_SET_DIGITAL_ZOOM, &digital_zoom) < 0) {
				perror("IAV_IOC_SET_DIGITAL_ZOOM");
				return -1;
			}
			usleep(1000 * 33);	//zoom at about 1 fps
		}
	}

	return 0;
}

static int digital_zoom_autorun_for_prev(int buffer_id)
{
	struct iav_srcbuf_setup setup;
	struct iav_digital_zoom digital_zoom;
	u32 w, h;
	int x, y, dir, max_x, max_y, dx, dy;
	int zm_flag, countX, countY, zm_factor, time;// zm_out_flag

	if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP, &setup) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_SETUP");
		return -1;
	}
	memset(&digital_zoom, 0, sizeof(digital_zoom));
	digital_zoom.buf_id = buffer_id;;
	if (ioctl(fd_iav, IAV_IOC_GET_DIGITAL_ZOOM, &digital_zoom) < 0) {
		perror("IAV_IOC_GET_DIGITAL_ZOOM");
		return -1;
	}

	w = setup.size[buffer_id].width;
	h = setup.size[buffer_id].height;
	max_x = setup.size[0].width - w;
	max_y = setup.size[0].height - h;
	x = y = zm_flag = 0;
	countX = 20;
	countY = 10;
	zm_factor = 50;
	dir = 1;
	while (1) {
		digital_zoom.input.width = w & (~0x1);
		digital_zoom.input.height = h & (~0x3);
		digital_zoom.input.x = x & (~0x1);
		digital_zoom.input.y = y & (~0x3);
//		printf("(DPTZ Type II) zoom out window size :  %dx%d, offset : %dx%d\n",
//			digital_zoom.input_width, digital_zoom.input_height,
//			digital_zoom.input_offset_x, digital_zoom.input_offset_y);
		if (ioctl(fd_iav, IAV_IOC_SET_DIGITAL_ZOOM, &digital_zoom) < 0) {
			perror("IAV_IOC_SET_2ND_DIGITAL_ZOOM_EX");
		}
		if (!zm_flag) {
			time = 200 * 1000;
			x += dir * max_x / countX;
			if (x > max_x) {
				x = max_x;
				dir = -1;
				y += max_y / countY;
				sleep(2);
			}
			if (x <= 0) {
				x = 0;
				dir = 1;
				y += max_y / countY;
				sleep(2);
			}
			if (y > max_y) {
				x = max_x / 2;
				y = max_y / 2;
				zm_flag = 1;
				//zm_out_flag = 1;
				sleep(2);
			}
		} else {
			dy = max_y / zm_factor;
			dx = w * dy / h;
			x -= dx;
			y -= dy;
			w += 2 * dx;
			h += 2 * dy;
			if (h > setup.size[0].height || w > setup.size[0].width) {
				w = setup.size[buffer_id].width;
				h = setup.size[buffer_id].height;
				x = y = zm_flag = 0;
				dir = 1;
				sleep(4);
			}
			time = 200 * 1000;
		}
		usleep(time);
	}

	return 0;
}


int main(int argc, char **argv)
{
	int do_dptz_for_main_buffer = 0;
	int do_dptz_for_preview_buffer = 0;

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

	if (dptz_format_change_id & (1 << 0))
		do_dptz_for_main_buffer = 1;

	if (dptz_format_change_id & ~(1 << 0))
		do_dptz_for_preview_buffer = 1;

	if (do_dptz_for_main_buffer) {
		if (digital_ptz_for_main_buffer() < 0)
			return -1;
	}

	if (do_dptz_for_preview_buffer) {
		if (digital_ptz_for_preview_buffer() < 0)
			return -1;
	}

	if (autorun_pan_I_flag) {
		if (auto_pan_tilt_for_main() < 0)
			return -1;
	}

	if (autorun_zoom_I_flag) {
		if (auto_zoom_for_main() < 0)
			return -1;
	}

	if (autorun_zoom_II_flag) {
		if (digital_zoom_autorun_for_prev(autorun_zoom_II_flag) < 0)
			return -1;
	}

	close(fd_iav);
	return 0;
}

