/*
 * test_privacymask.c
 *
 * History:
 *	2010/05/28 - [Louis Sun] created for A5s
 *	2011/07/04 - [Jian Tang] modified for A5s
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

static struct amba_video_info vin_info;
static u32 vin_frame_time;
static int fd_privacy_mask;
static u8 *pm_addr = NULL;
static int pm_size = 0;
struct iav_privacy_mask privacy_mask;

//auto-run privacy mask flag
static int auto_run_flag = 0;
static int auto_run_interval = 3;


//mask rect
typedef struct privacy_mask_rect_s{
	int start_x;
	int start_y;
	int width;
	int height;
} privacy_mask_rect_t;

static int mask_rect_remove = 0;
static int mask_rect_set = 0;
static privacy_mask_rect_t	mask_rect;

#define NO_ARG		0
#define HAS_ARG		1
#define OPTIONS_BASE		0

enum numeric_short_options {
	DISABLE_PRIVACY_MASK = OPTIONS_BASE,
};

#ifndef DIV_ROUND
#define DIV_ROUND(x, len) (((x) + (len) - 1) / (len))
#endif

#ifndef ROUND_UP
#define ROUND_UP(x, n)  ( ((x)+(n)-1u) & ~((n)-1u) )
#endif

static struct option long_options[] = {
	{"xstart", HAS_ARG, 0, 'x'},
	{"ystart", HAS_ARG, 0, 'y'},
	{"width", HAS_ARG, 0, 'w'},
	{"height", HAS_ARG, 0, 'h'},
	{"remove", NO_ARG, 0, 'd'},

	//turn it off
	{"disable", NO_ARG, 0, DISABLE_PRIVACY_MASK},
	{"auto", HAS_ARG, 0, 'r'},

	{0, 0, 0, 0}
};

static const char *short_options = "x:y:w:h:dr:";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"", "\tset offset x"},
	{"", "\tset offset y"},
	{"", "\tset mask width"},
	{"", "\tset mask height"},
	{"", "\tremove mask"},


	{"", "\tclear all masks"},
	{"1~30", "auto run privacy mask every N frames, default is 3 frames"},
};

static void usage(void)
{
	int i;

	printf("test_privacymask usage:\n");
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
	printf("\nExamples:");
	printf("\n  Add include a privacy mask:\n    test_privacymask -x100 -y100 -w256 -h256\n");
	printf("\n  Add exclude a privacy mask:\n    test_privacymask -x100 -y100 -w256 -h256 -d\n");
	printf("\n  Clear all privacy masks:\n    test_privacymask --disable\n");
	printf("\n  Auto run privacy masks every 4 frames:\n    test_privacymask -r 4\n");
	printf("\n");
}

static int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	memset(&privacy_mask, 0, sizeof(privacy_mask));
	privacy_mask.enable = 1;
	privacy_mask.y = 0;
	privacy_mask.u = 0;
	privacy_mask.v = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'x':
			mask_rect.start_x = atoi(optarg);
			mask_rect_set = 1;
			break;
		case 'y':
			mask_rect.start_y = atoi(optarg);
			mask_rect_set = 1;
			break;
		case 'w':
			mask_rect.width = atoi(optarg);
			mask_rect_set = 1;
			break;
		case 'h':
			mask_rect.height = atoi(optarg);
			mask_rect_set = 1;
			break;
		case 'd':
			mask_rect_remove = 1;
			break;
		case DISABLE_PRIVACY_MASK:
			privacy_mask.enable = 0;
			break;
		case 'r':
			auto_run_flag = 1;
			auto_run_interval = atoi(optarg);
			if (auto_run_interval < 1 || auto_run_interval > 30) {
				printf("Invalid auto run interval value [%d], please choose from 1~30.\n",
					auto_run_interval);
				return -1;
			}
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
			break;
		}
	}

	if (mask_rect.start_x + mask_rect.width > vin_info.width ||
		mask_rect.start_y + mask_rect.height > vin_info.height){
		return -1;
	}
	return 0;
}

static void sigstop()
{
	memset(pm_addr, 0, pm_size);
	if (ioctl(fd_privacy_mask, IAV_IOC_SET_PRIVACY_MASK, &privacy_mask) < 0) {
		perror("IAV_IOC_SET_PRIVACY_MASK");
	}
	exit(1);
}

static inline int get_vin_info(){
	struct vindev_fps vsrc_fps;
	struct vindev_video_info video_info;

	video_info.vsrc_id = 0;
	video_info.info.mode = AMBA_VIDEO_MODE_CURRENT;
	vin_info.mode = AMBA_VIDEO_MODE_CURRENT;
	if (ioctl(fd_privacy_mask, IAV_IOC_VIN_GET_VIDEOINFO, &video_info) < 0) {
		perror("IAV_IOC_VIN_GET_VIDEOINFO");
		return -1;
	}

	memcpy(&vin_info, &video_info.info, sizeof(struct amba_video_info));

	vsrc_fps.vsrc_id = 0;
	if (ioctl(fd_privacy_mask, IAV_IOC_VIN_GET_FPS, &vsrc_fps) < 0) {
		perror("IAV_IOC_VIN_GET_FPS");
		return -1;
	}

	vin_frame_time = vsrc_fps.fps;

	return 0;
}

static int map_privacymask(void)
{
	struct iav_querybuf querybuf;

	querybuf.buf = IAV_BUFFER_PM_BPC;
	if (ioctl(fd_privacy_mask, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
		perror("IAV_IOC_QUERY_BUF");
		return -1;
	}

	pm_size = querybuf.length;
	pm_addr = mmap(NULL, pm_size, PROT_WRITE, MAP_SHARED, fd_privacy_mask, querybuf.offset);

	return 0;
}

int fill_pm_mem(privacy_mask_rect_t pm_rect, int pm_remove)
{
	int i, j;
	int pitch = ROUND_UP(vin_info.width / 8, 32);
	u8 *row = pm_addr + pitch * pm_rect.start_y;
	u8 *column;
	u8 value;
	int left = pm_rect.start_x / 8;
	int right = (pm_rect.start_x + pm_rect.width) / 8;
	int left_start = pm_rect.start_x % 8;
	int right_remain = (pm_rect.start_x + pm_rect.width) % 8;

	if (pm_remove) {
		value = 0x00;
	} else {
		value =0xff;
	}
	for (j = 0; j < pm_rect.height; j++) {

		if (left == right && (left_start + pm_rect.width < 8)) {
			/*This is for small PM width is within 8 pixel*/
			column = row + left;
			if (pm_remove) {
				value = 0xFF;
				value <<= (left_start + pm_rect.width);
				value |= (0xFF >> (8 - left_start));
				*column &= value;
			} else {
				value = 0xFF;
				value <<= left_start;
				value &= (0xFF >> (8 - left_start - pm_rect.width));
				*column |= value;
			}
		}	else {
			/* This is for PM ocupies more than one byte(8 pixel) */
			column = row + left + 1;
			for (i = 0; i < right - left - 1; i++) {
				*column = value;
				column++;
			}

			column = row + left;
			if (pm_remove) {
				*column &= (0xFF >> (8 - left_start));
			} else {
				*column |= (0xFF << left_start);
			}

			column = row + right;
			if (right_remain) {
				if (pm_remove) {
					*column &= (0xFF << right_remain);
				} else {
					*column |= (0xFF >> (8 - right_remain));
				}
			}
		}

		/*Switch to next line*/
		row += pitch;
	}

	if (ioctl(fd_privacy_mask, IAV_IOC_SET_PRIVACY_MASK, &privacy_mask) < 0) {
		perror("IAV_IOC_SET_PRIVACY_MASK");
		return -1;
	}

	return 0;
}

int auto_run_test(int interval)
{
	static u32 sleep_time;
	privacy_mask_rect_t random_pm;
	sleep_time = interval * 1000000 / (DIV_ROUND(512000000, vin_frame_time));

	random_pm.start_x = rand() % vin_info.width;
	random_pm.start_y = rand() % vin_info.height;
	random_pm.width = rand() % vin_info.width;
	random_pm.height = rand() % vin_info.height;

	if (random_pm.start_x + random_pm.width > vin_info.width){
		random_pm.start_x -= random_pm.start_x + random_pm.width - vin_info.width;
	}

	if (random_pm.start_y + random_pm.height > vin_info.height){
		random_pm.start_y -= random_pm.start_y + random_pm.height - vin_info.height;
	}

	fill_pm_mem(random_pm, 0);

	usleep(sleep_time);

	memset(pm_addr, 0, pm_size);
	if (ioctl(fd_privacy_mask, IAV_IOC_SET_PRIVACY_MASK, &privacy_mask) < 0) {
		perror("IAV_IOC_SET_PRIVACY_MASK");
		return -1;
	}

	usleep(sleep_time);

	return 0;

}
int main(int argc, char **argv)
{
	//register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
	signal(SIGINT, 	sigstop);
	signal(SIGQUIT,	sigstop);
	signal(SIGTERM,	sigstop);

	if ((fd_privacy_mask = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (argc < 2) {
		usage();
		return 0;
	}

	/* PM type check */
	struct iav_system_resource resource;
	memset(&resource, 0, sizeof(resource));
	resource.encode_mode = DSP_CURRENT_MODE;
	if(ioctl(fd_privacy_mask, IAV_IOC_GET_SYSTEM_RESOURCE, &resource) < 0) {
		perror("IAV_IOC_GET_SYSTEM_RESOURCE");
		return -1;
	}
	if (resource.mctf_pm_enable) {
		printf("Please switch PM type to BPC based PM as test_privacymask is always using "
				"BPC based PM!\n");
		return -1;
	}

	get_vin_info();

	if (init_param(argc, argv) < 0)	{
		printf("init param failed \n");
		return -1;
	}

	if (map_privacymask() < 0) {
		printf("map privacy mask failed \n");
		return -1;
	}

	if (auto_run_flag == 0){
		//disable all PM
		if (!privacy_mask.enable) {
			memset(pm_addr, 0, pm_size);
			if (ioctl(fd_privacy_mask, IAV_IOC_SET_PRIVACY_MASK, &privacy_mask) < 0) {
				perror("IAV_IOC_SET_PRIVACY_MASK");
				return -1;
			}
			return 0;
		}

		if (fill_pm_mem(mask_rect, mask_rect_remove) < 0){
			perror("fill_pm_mem");
		}

	} else {
		while (1) {
			auto_run_test(auto_run_interval);
			//autorun case will be added soon.
		}
	}

	return 0;
}

