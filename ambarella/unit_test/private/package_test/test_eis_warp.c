/*
 * test_eis_warp.c
 *
 * History:
 *  Oct 25, 2013 - [qianshen] created file
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
#include <signal.h>
#include <basetypes.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <iav_ioctl.h>
#include "utils.h"
#include "lib_eis.h"

#define FILENAME_LENGTH			(256)
#define DIV_ROUND(divident, divider)   (((divident)+((divider)>>1)) / (divider))

static int fd_iav = -1;
static int fd_eis = -1;
static int fd_hwtimer = -1;
static int cali_num = 3000;
static int debug_level = AMBA_LOG_INFO;
static char save_file[FILENAME_LENGTH] = {0};
static int save_file_flag = 0;
static FILE *fd_save_file = NULL;
static char input[16] = {0};
static int calibrate_done = 0;

eis_setup_t EIS_SETUP = { // MPU6000 on A5s board, AR0330, 2304x1296
        .accel_full_scale_range = 2,
        .accel_lsb = 16384,
        .gyro_full_scale_range = 250.0,
        .gyro_lsb = 131.0,
        .gyro_sample_rate_in_hz = 2000,
        .gyro_shift = {
                .xg = -594,
                .yg = -266,
                .zg = -23,
                .xa = 464,
                .ya = 40,
                .za = 14175,
        },
        .gravity_axis = AXIS_Z,
        .lens_focal_length_in_um = 8000,
        .threshhold = 0.00001,
        .frame_buffer_num = 60,
        .avg_mode = EIS_AVG_MA,
};

static int warp_size;
static u8* warp_addr;

unsigned int current_fps = 0;
static pthread_t fps_monitor_thread_id;

#define NO_ARG		0
#define HAS_ARG		1

static struct option long_options[] = {
	{"gravity", HAS_ARG, 0, 'g'},
	{"cali", HAS_ARG, 0, 'c'},
	{"focal", HAS_ARG, 0, 'F'},
	{"avg-mode", HAS_ARG, 0, 'm'},
	{"debug", HAS_ARG, 0, 'd'},
	{"save", HAS_ARG, 0, 's'},
	{"threshhold", HAS_ARG, 0, 't'},
	{"frame-buf-num", HAS_ARG, 0, 'f'},
	{"help", NO_ARG, 0, 'h'},

	{0, 0, 0, 0}
};

static const char *short_options = "g:c:F:d:s:t:f:m:";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"0|1|2", "\tThe axis of gravity. 0: Z, 1: Y, 2: X"},
	{">0", "\tData number to calibrate gyro shift. The default is 3000."},
	{">0", "\tLens focal length in um."},
	{"0|1", "\t0: Moving Average, 1: Absolute Value."},
	{"0~2", "\tlog level. 0: error, 1: info (default), 2: debug"},
	{"", "\tset file name to save gyro data"},
	{"", "\tset threshhold"},
	{"", "\tset Frame buffer num"},
	{"", "\thelp"},
};

static void usage(void)
{
	int i;

	printf("test_eis_warp usage:\n");
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
	printf("\n  test_encode -i 2304x1536 -V480p --hdmi -J --btype prev --enc-mode 4 -X --bsize 1920x1088 --bmaxsize 1920x1088 --max-padding-width 256 --lens-warp 1 -A --smaxsize 1080p --eis-delay-count 2\n");
	printf("\n");
	printf("\n  Gyro on main board:\n    test_eis_warp -c 5000\n");
	printf("\n  test_encode -A -h1080p --offset 0x4 -e\n");
	printf("\n");
}

static int init_param(int argc, char** argv)
{
	int ch;
	int option_index = 0;
	int file_name_len = 0;
	int avg_mode;
	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'g':
			EIS_SETUP.gravity_axis = atoi(optarg);
			break;
		case 'c':
			cali_num = atoi(optarg);
			break;
		case 'F':
			EIS_SETUP.lens_focal_length_in_um = atoi(optarg);
			break;
		case 'd':
			debug_level = atoi(optarg);
			break;
		case 's':
			file_name_len = strlen(optarg);
			strncpy(save_file, optarg, file_name_len > FILENAME_LENGTH ? FILENAME_LENGTH :file_name_len);
			save_file_flag = 1;
			break;
		case 't':
			EIS_SETUP.threshhold = atof(optarg);
			break;
		case 'f':
			EIS_SETUP.frame_buffer_num = atoi(optarg);
			break;
		case 'm':
			avg_mode = atoi(optarg);
			if (avg_mode < EIS_AVG_FIRST || avg_mode >= EIS_AVG_NUM) {
				ERROR("Invalid AVG mode!\n");
				return -1;
			}
			EIS_SETUP.avg_mode = avg_mode;
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
			break;
		}
	}
	return 0;
}

int set_eis_warp(const struct iav_warp_main* warp_main)
{
	struct iav_warp_ctrl warp_control;
	u32 flags = (1 << IAV_WARP_CTRL_MAIN);

	memset(&warp_control, 0, sizeof(struct iav_warp_ctrl));
	warp_control.cid = IAV_WARP_CTRL_MAIN;
	memcpy(&warp_control.arg.main, warp_main, sizeof(struct iav_warp_main));

	DEBUG("warp:  input %dx%d+%dx%d, output %dx%d, hor enable %d (%dx%d),"
			" ver enable %d (%dx%d)\n",
			warp_main->area[0].input.width,
			warp_main->area[0].input.height,
			warp_main->area[0].input.x,
			warp_main->area[0].input.y,
			warp_main->area[0].output.width,
			warp_main->area[0].output.height,
			warp_main->area[0].h_map.enable,
			warp_main->area[0].h_map.output_grid_row,
			warp_main->area[0].h_map.output_grid_col,
			warp_main->area[0].v_map.enable,
			warp_main->area[0].v_map.output_grid_row,
			warp_main->area[0].v_map.output_grid_col);

	if (ioctl(fd_iav, IAV_IOC_CFG_WARP_CTRL, &warp_control) < 0) {
		perror("IAV_IOC_CFG_WARP_CTRL");
		return -1;
	}

	if (ioctl(fd_iav, IAV_IOC_APPLY_WARP_CTRL, &flags) < 0) {
		perror("IAV_IOC_APPLY_WARP_CTRL");
		return -1;
	}

	return 0;
}

int get_eis_stat(amba_eis_stat_t* eis_stat)
{
	int i = 0;
	u64 hw_pts = 0;
	char pts_buf[32];
	if (ioctl(fd_eis, AMBA_IOC_EIS_GET_STAT, eis_stat) < 0) {
		perror("AMBA_IOC_EIS_GET_STAT\n");
		return -1;
	}
	if (unlikely(eis_stat->discard_flag || eis_stat->gyro_data_count == 0)) {
		DEBUG("stat discard\n");
		return -1;
	}

	if (save_file_flag && calibrate_done) {
		read(fd_hwtimer, pts_buf, sizeof(pts_buf));
		hw_pts = strtoull(pts_buf,(char **)NULL, 10);
		fprintf(fd_save_file, "===pts:%llu\n", hw_pts);
		lseek(fd_hwtimer, 0L, SEEK_SET);

		for (i = 0; i < eis_stat->gyro_data_count; i++) {
			fprintf(fd_save_file, "%d\t%d\t%d\t%d\t%d\t%d\n", eis_stat->gyro_data[i].xg,
				eis_stat->gyro_data[i].yg, eis_stat->gyro_data[i].zg, eis_stat->gyro_data[i].xa,
				eis_stat->gyro_data[i].ya, eis_stat->gyro_data[i].za);
		}
	}
	DEBUG("AMBA_IOC_EIS_GET_STAT: count %d\n", eis_stat->gyro_data_count);
	return 0;
}

static float change_fps_to_hz(u32 fps_q9)
{
	switch(fps_q9) {
	case AMBA_VIDEO_FPS_29_97:
		return 29.97f;
	case AMBA_VIDEO_FPS_59_94:
		return 59.94f;
	default:
		return DIV_ROUND(512000000, fps_q9);
	}

	return 0;
}

static int prepare(void)
{
	struct iav_srcbuf_setup buffer_setup;
	struct vindev_video_info vin_info;
	struct vindev_eisinfo vin_eis_info;
	struct vindev_fps vsrc_fps;

	memset(&buffer_setup, 0, sizeof(buffer_setup));
	memset(&vin_info, 0, sizeof(vin_info));
	memset(&vin_eis_info, 0, sizeof(vin_eis_info));

	if (fd_iav < 0 && (fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("open /dev/iav");
		return -1;
	}

	if (fd_eis < 0 && (fd_eis = open("/dev/eis", O_RDWR, 0)) < 0) {
		perror("open /dev/eis");
		return -1;
	}

	if (fd_hwtimer < 0 && (fd_hwtimer = open("/proc/ambarella/ambarella_hwtimer", O_RDWR, 0)) < 0) {
		perror("open /proc/ambarella/ambarella_hwtimer");
		return -1;
	}

	if (save_file_flag && !fd_save_file) {
		if ((fd_save_file = fopen(save_file, "w+")) == NULL) {
			perror(save_file);
			return -1;
		}
	}
	vin_info.vsrc_id = 0;
	vin_info.info.mode = AMBA_VIDEO_MODE_CURRENT;
	if (ioctl(fd_iav, IAV_IOC_VIN_GET_VIDEOINFO, &vin_info) < 0) {
		perror("IAV_IOC_VIN_GET_VIDEOINFO");
		return -1;
	}

	if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP, &buffer_setup) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_SETUP");
		return -1;
	}

	if (ioctl(fd_iav, IAV_IOC_VIN_GET_EISINFO, &vin_eis_info) < 0) {
		perror("IAV_IOC_VIN_GET_EISINFO");
		return -1;
	}


	vsrc_fps.vsrc_id = 0;
	if (ioctl(fd_iav, IAV_IOC_VIN_GET_FPS, &vsrc_fps) < 0) {
		perror("IAV_IOC_VIN_GET_FPS");
		return -1;
	}

	EIS_SETUP.vin_width = vin_info.info.width;
	EIS_SETUP.vin_height = vin_info.info.height;
	EIS_SETUP.vin_col_bin = vin_eis_info.column_bin;
	EIS_SETUP.vin_row_bin = vin_eis_info.row_bin;
	EIS_SETUP.vin_cell_width_in_um = (float) vin_eis_info.sensor_cell_width / 100.0;
	EIS_SETUP.vin_cell_height_in_um = (float) vin_eis_info.sensor_cell_height / 100.0;
	EIS_SETUP.vin_frame_rate_in_hz = change_fps_to_hz(vsrc_fps.fps);
	EIS_SETUP.vin_vblank_in_ms = vin_eis_info.vb_time / 1000000.0;

	EIS_SETUP.premain_input_width = vin_info.info.width;
	EIS_SETUP.premain_input_height = vin_info.info.height;
	EIS_SETUP.premain_input_offset_x = 0;
	EIS_SETUP.premain_input_offset_y = 0;

	EIS_SETUP.premain_width = vin_info.info.width;
	EIS_SETUP.premain_height = vin_info.info.height;
	EIS_SETUP.output_width = buffer_setup.size[0].width;
	EIS_SETUP.output_height = buffer_setup.size[0].height;

	current_fps = vsrc_fps.fps;

	return 0;
}

void calibrate(void)
{
	int i, n = 0;
	s32 sum_xg = 0, sum_yg = 0, sum_zg = 0, sum_xa = 0, sum_ya = 0, sum_za = 0;
	amba_eis_stat_t eis_stat;

	while (n < cali_num) {
		if (get_eis_stat(&eis_stat) == 0) {
			for (i = 0; i < eis_stat.gyro_data_count; i++) {
				sum_xg += eis_stat.gyro_data[i].xg;
				sum_yg += eis_stat.gyro_data[i].yg;
				sum_zg += eis_stat.gyro_data[i].zg;
				sum_xa += eis_stat.gyro_data[i].xa;
				sum_ya += eis_stat.gyro_data[i].ya;
				sum_za += eis_stat.gyro_data[i].za;
			}
			n += eis_stat.gyro_data_count;
		}
	}
	EIS_SETUP.gyro_shift.xg = sum_xg / n;
	EIS_SETUP.gyro_shift.yg = sum_yg / n;
	EIS_SETUP.gyro_shift.zg = sum_zg / n;
	EIS_SETUP.gyro_shift.xa = sum_xa / n;
	EIS_SETUP.gyro_shift.ya = sum_ya / n;
	EIS_SETUP.gyro_shift.za = sum_za / n;

	INFO("Calibration: accel ( %d, %d, %d ), gyro ( %d, %d, %d )\n",
	    EIS_SETUP.gyro_shift.xa, EIS_SETUP.gyro_shift.ya,
	    EIS_SETUP.gyro_shift.za, EIS_SETUP.gyro_shift.xg,
	    EIS_SETUP.gyro_shift.yg, EIS_SETUP.gyro_shift.zg);
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
	//printf("map_warp: start = %p, total size = 0x%x\n", warp_addr, warp_size);
	memset(warp_addr, 0, warp_size);
	EIS_SETUP.h_table_addr = (s16 *)warp_addr;
	EIS_SETUP.v_table_addr = (s16 *)(warp_addr + MAX_WARP_TABLE_SIZE_LDC * sizeof(s16));
	EIS_SETUP.me1_v_table_addr = (s16 *)(warp_addr + MAX_WARP_TABLE_SIZE_LDC * sizeof(s16) * 2);
	return 0;
}

static void sigstop()
{
	if (fd_save_file) {
		fclose(fd_save_file);
	}
	eis_enable(EIS_DISABLE);
	if (atoi(input)) {
		eis_close();
	}
	if (fd_hwtimer > 0) {
		close(fd_hwtimer);
	}
	exit(0);
}

static void* fps_monitor_func(void* arg)
{
	struct vindev_fps vsrc_fps;
	unsigned int last_fps;

	while (1) {
		last_fps = current_fps;

		vsrc_fps.vsrc_id = 0;
		if (ioctl(fd_iav, IAV_IOC_VIN_GET_FPS, &vsrc_fps) < 0) {
			perror("IAV_IOC_VIN_GET_FPS");
			ERROR("FPS monitor thread exit!\n");
			return NULL;
		}
		current_fps = vsrc_fps.fps;

		if (last_fps != current_fps) {
			eis_close();
			prepare();
			eis_setup(&EIS_SETUP, set_eis_warp, get_eis_stat);
			eis_open();
			eis_enable(atoi(input));
		}
	}
	return NULL;
}

int main(int argc,  char* argv[])
{
	version_t version;
	int eis_selection = 0;

	if (init_param(argc, argv) < 0) {
		usage();
		return -1;
	}

	set_log(debug_level, NULL);
	eis_version(&version);
	printf("\nLibrary Version: %s-%d.%d.%d (Last updated: %x)\n\n",
	    version.description, version.major, version.minor,
	    version.patch, version.mod_time);

	if (prepare() < 0)
		return -1;

	if (map_warp() < 0) {
		return -1;
	}

	if (cali_num > 0) {
		calibrate();
		calibrate_done = 1;
	}

	if (eis_setup(&EIS_SETUP, set_eis_warp, get_eis_stat) < 0)
		return -1;

	signal(SIGINT, sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	if (pthread_create(&fps_monitor_thread_id, NULL, fps_monitor_func, NULL) >= 0) {
		INFO("FPS monitor thread created.\n");
	} else {
		perror("pthread_create");
	}

	while (1) {
		printf("enable (5: eis_full, 4: rotate + pitch, 3: yaw, 2: rotate, 1: pitch), disable (0):");
		scanf("%s", input);
		eis_selection = atoi(input);

		switch (eis_selection) {
		case 0:
			eis_close();
			break;
		case 3:
		case 5:
			printf("Unsupported option.\n");
			break;
		case 1:
		case 2:
		case 4:
			if (eis_open() < 0) {
				return -1;
			}
			eis_enable(eis_selection);
			break;
		default:
			printf("Invalid param.\n");
			break;
		}
	}

	return 0;
}
