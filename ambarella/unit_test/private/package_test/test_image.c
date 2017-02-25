/*
 * test_image.c
 *
 * History:
 *	2010/03/25 - [Jian Tang] created file
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
#include <sys/shm.h>

#include <time.h>
#include <assert.h>

#include "iav_ioctl.h"
#include "iav_common.h"
#include "iav_vin_ioctl.h"

#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"

#include "img_adv_struct_arch.h"

#include "mw_struct.h"
#include "mw_api.h"
#include "AmbaDSP_Img3aStatistics.h"
#include "AmbaDSP_ImgUtility.h"
#include "img_customer_interface_arch.h"
#include "img_api_adv_arch.h"

#define WDR_LUMA_RADIUS_MIN			(0)
#define WDR_LUMA_RADIUS_MAX			(6)
#define WDR_LUMA_WEIGHT_MIN			(0)
#define WDR_LUMA_WEIGHT_MAX			(31)

#define	DIV_ROUND(divident, divider)    (((divident) + ((divider) >> 1)) / (divider))
#define	SENSOR_STEPS_PER_DB		6
#define	DEFAULT_CFG_FILE		"/etc/idsp/cfg/test_image.cfg"

#define SCANF_INT(addr)		do {	\
			if (scanf("%d", addr) < 0) {	\
				printf("input error.\n");	\
				return -1;					\
			}							\
		} while (0)

#define SCANF_STR(addr)		do {	\
			if (scanf("%s", addr) < 0) {	\
				printf("input error.\n");	\
				return -1;					\
			}							\
		} while (0)

typedef enum {
	FETCH_AAA_STATISTICS = 0,
} TEST_OPTION;

typedef enum {
	STATIS_AWB = 0,
	STATIS_AE,
	STATIS_AF,
	STATIS_HIST,
	STATIS_AF_HIST,
} STATISTICS_TYPE;

typedef enum {
	BACKGROUND_MODE = 0,
	INTERACTIVE_MODE,
	TEST_3A_LIB_MODE,
	WORK_MODE_NUM,
	WORK_MODE_FIRST = BACKGROUND_MODE,
	WORK_MODE_LAST = WORK_MODE_NUM,
} mw_work_mode;

static int fd_iav;
static int fd_vin = -1;
static int work_mode = INTERACTIVE_MODE;

//static int G_expo_num = 0;
static mw_sys_info G_mw_info;
static int G_log_level = 0;

typedef struct {
	u8 *cfa_aaa;
	u8 *rgb_aaa;
	u8 *cfa_pre;
	u8 *cfa_data_valid;
	u8 *rgb_data_valid;
	u8 *cfa_pre_hdr_data_valid;
	u32 *dsp_pts_data_addr;
	u64 *mono_pts_data_addr;
} statistic_data_t;

typedef struct {
	cfa_pre_hdr_histogram_t  cfa_pre_histogram[MAX_EXPOSURE_NUM];
	cfa_histogram_stat_t cfa_histogram;
	rgb_histogram_stat_t rgb_histogram;
} histogram_data_t;

/****************************************************
 *	AE Related Settings
 ****************************************************/
static mw_ae_param			ae_param;

static u32	dc_iris_enable		= 0;
static mw_dc_iris_pid_coef	pid_coef = {2500, 2, 5000};
static int	load_mctf_flag = 0;

static int auto_dump_cfg_flag = 0;

static int	G_lens_id = LENS_CMOUNT_ID;
static int	load_file_flag[FILE_TYPE_TOTAL_NUM] = {0};
static char	G_file_name[FILE_TYPE_TOTAL_NUM][FILE_NAME_LENGTH] = {""};
static char	image_save_file[FILE_NAME_LENGTH] = DEFAULT_CFG_FILE;

static mw_ae_line ae_60hz_lines[] = {
	{
		{{SHUTTER_1BY8000_SEC, ISO_100, 0}, MW_AE_START_POINT},
		{{SHUTTER_1BY120_SEC, ISO_100, 0}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY120_SEC, ISO_100, 0}, MW_AE_START_POINT},
		{{SHUTTER_1BY120_SEC, ISO_200, 0}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY60_SEC, ISO_100, 0}, MW_AE_START_POINT},
		{{SHUTTER_1BY60_SEC, ISO_200, 0}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY30_SEC, ISO_100, 0}, MW_AE_START_POINT},
		{{SHUTTER_1BY30_SEC, ISO_400, 0}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY15_SEC, ISO_200, 0}, MW_AE_START_POINT},
		{{SHUTTER_1BY15_SEC, ISO_400, 0}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY7P5_SEC, ISO_200, 0}, MW_AE_START_POINT},
		{{SHUTTER_1BY7P5_SEC, ISO_400, 0}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_INVALID, 0, 0}, MW_AE_START_POINT},
		{{SHUTTER_INVALID, 0, 0}, MW_AE_START_POINT}
	}
};

static mw_ae_line ae_50hz_lines[] = {
	{
		{{SHUTTER_1BY8000_SEC, ISO_100, 0}, MW_AE_START_POINT},
		{{SHUTTER_1BY100_SEC, ISO_100, 0}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY100_SEC, ISO_100, 0}, MW_AE_START_POINT},
		{{SHUTTER_1BY100_SEC, ISO_200, 0}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY50_SEC, ISO_100, 0}, MW_AE_START_POINT},
		{{SHUTTER_1BY50_SEC, ISO_200, 0}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY25_SEC, ISO_100, 0}, MW_AE_START_POINT},
		{{SHUTTER_1BY25_SEC, ISO_400, 0}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY15_SEC, ISO_200, 0}, MW_AE_START_POINT},
		{{SHUTTER_1BY15_SEC, ISO_400, 0}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY7P5_SEC, ISO_200, 0}, MW_AE_START_POINT},
		{{SHUTTER_1BY7P5_SEC, ISO_400, 0}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_INVALID, 0, 0}, MW_AE_START_POINT},
		{{SHUTTER_INVALID, 0, 0}, MW_AE_START_POINT}
	}
};

static mw_ae_line ae_customer_lines[] = {
	{
		{{SHUTTER_1BY8000_SEC, ISO_100, 0}, MW_AE_START_POINT},
		{{SHUTTER_1BY60_SEC, ISO_100, 0}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY60_SEC, ISO_100, 0}, MW_AE_START_POINT},
		{{SHUTTER_1BY60_SEC, ISO_150, 0}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY60_SEC, ISO_150, 0}, MW_AE_START_POINT},
		{{SHUTTER_1BY30_SEC, ISO_150, 0}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY30_SEC, ISO_150, 0}, MW_AE_START_POINT},
		{{SHUTTER_1BY30_SEC, ISO_3200, 0}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY30_SEC, ISO_3200, 0}, MW_AE_START_POINT},
		{{SHUTTER_1BY7P5_SEC, ISO_3200, 0}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_1BY7P5_SEC, ISO_3200, 0}, MW_AE_START_POINT},
		{{SHUTTER_1BY7P5_SEC, ISO_102400, 0}, MW_AE_END_POINT}
	},
	{
		{{SHUTTER_INVALID, 0, 0}, MW_AE_START_POINT},
		{{SHUTTER_INVALID, 0, 0}, MW_AE_START_POINT}
	}
};

/****************************************************
 *	AWB Related Settings
 ****************************************************/
//static u32	white_balance_mode	= MW_WB_AUTO;

/****************************************************
 *	Image Adjustment Settings
 ****************************************************/

#define SHARPEN_PROPERTY_MAX_NUM		12
static sharpen_property_t sharpen_property_array[SHARPEN_PROPERTY_MAX_NUM] =
{
	{192,64,0, 32,3},// 0
	{128,64,2,32,3},// 1
	{116,64,4,48,3},// 2
	{102,64,6,64,3}, // 3
	{92,64,8,64,3},// 4
	{80,64,12,64,3},// 5
	{64,64,16,64,3},// 6 //def
	{56,64,24,96,3},// 7
	{48,64,32,128,3},// 8
	{40,64,40,160,2},// 9
	{32,64,48,192,1},// 10
	{24,64,48,255,1},// 11
};

//SHARPENA
filter_container_p8_t liso_fc_1st_shpboth = {
	.header = {
		ADJ_1ST_SHPBOTH,
		1,
		6,//tbf
		0,
		15,
		0,
		{{0,},},
		},
//		          enb   mode edge_thres  w_edge_thres up5x5 down5x5
	.param[0] =  {{1,   2,   15,        1,      40,   40,}},// 0db
	.param[1] =  {{1,   2,   15,        1,      40,   40,}},// 0db
	.param[2] =  {{1,   2,   15,        1,      40,   40,}},// 0db
	.param[3] =  {{1,   2,   20,        1,      35,   35,}},// 6db
	.param[4] =  {{1,   2,   20,        1,      30,   30,}},// 12db
	.param[5] =  {{1,   2,   20,        1,      25,   25,}},// 18db
	.param[6] =  {{1,   2,   20,        1,      25,   25,}},// 24db
	.param[7] =  {{1,   2,   20,        1,      20,   20,}},// 30db
	.param[8] =  {{1,   2,   20,       1,      20,   20,}},// 36db
	.param[9] =  {{1,   2,   20,       1,      20,   20,}},// 42db
	.param[10] = {{1,   2,   20,       1,      20,   20,}},// 48db
	.param[11] = {{1,   2,   20,       1,      20,   20,}},// 54db
	.param[12] = {{1,   2,   20,       1,      20,   20,}},// 60db
	.param[13] = {{1,   2,   20,       1,      20,   20,}},// 66db
	.param[14] = {{1,   2,   20,       1,      20,   20,}},// 72db
	.param[15] = {{1,   2,   20,       1,      20,   20,}},// 78db
};

/***************************************************
 *	Test settings
 ***************************************************/
static u32 test_flag = 0;
static u32 test_option = 0;


/***************************************************************************************
	function:	int get_multi_arg(char *input_str, u16 *argarr, int *argcnt)
	input:	input_str: input string line for analysis
			argarr: buffer array accommodating the parsed arguments
			argcnt: number of the arguments parsed
	return value:	0: successful, -1: failed
	remarks:
***************************************************************************************/
static int get_multi_arg(char *input_str, u16 *argarr, int *argcnt)
{
	int i = 0;
	char *delim = ",:-\n\t";
	char *ptr;
	*argcnt = 0;

	ptr = strtok(input_str, delim);
	if (ptr == NULL)
		return 0;
	argarr[i++] = atoi(ptr);

	while (1) {
		ptr = strtok(NULL, delim);
		if (ptr == NULL)
			break;
		argarr[i++] = atoi(ptr);
	}

	*argcnt = i;
	return 0;
}

/***************************************************************************************
	function:	int input_value(int min, int max)
	return value: [min~max]

***************************************************************************************/
static int input_value(int min, int max)
{
	int retry, i, input = 0;
#define MAX_LENGTH	16
	char tmp[MAX_LENGTH];

	do {
		retry = 0;
		SCANF_STR(tmp);
		for (i = 0; i < MAX_LENGTH; i++) {
			if ((i == 0) && (tmp[i] == '-')) {
				continue;
			}
			if (tmp[i] == 0x0) {
				if (i == 0) {
					retry = 1;
				}
				break;
			}
			if ((tmp[i]  < '0') || (tmp[i] > '9')) {
				printf("error input:%s\n", tmp);
				retry = 1;
				break;
			}
		}

		input = atoi(tmp);
		if (input > max || input < min) {
			printf("\nInvalid. Enter a number (%d~%d): ", min, max);
			retry = 1;
		}

		if (retry) {
			printf("\nInput again: ");
			continue;
		}

	} while (retry);

	return input;
}

/***************************************************************************************
	function:	int load_local_exposure_curve(char *lect_filename, u16 *local_exposure_curve)
	input:	lect_filename: filename of local exposure curve table to be loaded
			local_exposure_curve: buffer structure accommodating the loaded local exposure curve
	return value:	0: successful, -1: failed
	remarks:
***************************************************************************************/
static int load_local_exposure_curve(char *lect_filename,
	mw_local_exposure_curve *local_exposure_curve)
{
	char key[64] = "local_exposure_curve";
	char line[1024];
	FILE *fp;
	int find_key = 0;
	u16 *param = &local_exposure_curve->gain_curve_table[0];
	int argcnt;

	if ((fp = fopen(lect_filename, "r")) == NULL) {
		printf("Open local exposure curve file [%s] failed!\n", lect_filename);
		return -1;
	}

	while (fgets(line, sizeof(line), fp) != NULL) {
		if (strstr(line, key) != NULL) {
			find_key = 1;
			break;
		}
	}

	if (find_key) {
		while ( fgets(line, sizeof(line), fp) != NULL) {
			get_multi_arg(line, param, &argcnt);
//			printf("argcnt %d\n", argcnt);
			param += argcnt;
			if (argcnt == 0)
				break;
		}
	}

	if (fp >= 0) {
		fclose(fp);
	}

	if (!find_key)
		return -1;

	return 0;
}

static int set_sensor_fps(u32 fps)
{
	static int init_flag = 0;
	static char vin_arr[8];
	struct vindev_fps vsrc_fps;

	vsrc_fps.vsrc_id = 0;

	switch (fps) {
	case 0:
		vsrc_fps.fps = AMBA_VIDEO_FPS_29_97;
		break;
	case 1:
		vsrc_fps.fps = AMBA_VIDEO_FPS_30;
		break;
	case 2:
		vsrc_fps.fps = AMBA_VIDEO_FPS_15;
		break;
	case 3:
		vsrc_fps.fps = AMBA_VIDEO_FPS_7_5;
		break;
	default:
		vsrc_fps.fps = AMBA_VIDEO_FPS_29_97;
		break;
	}
	if (init_flag == 0) {
		fd_vin =open("/proc/ambarella/vin0_vsync", O_RDWR);
		if (fd_vin < 0) {
			printf("CANNOT OPEN VIN FILE!\n");
			return -1;
		}
		init_flag = 1;
	}
	if (read(fd_vin, vin_arr, 8) < 0) {
		printf("read error.\n");
		return -1;
	}
	if (ioctl(fd_iav, IAV_IOC_VIN_SET_FPS, &vsrc_fps) < 0) {
		perror("IAV_IOC_VIN_SET_FPS");
		return -1;
	}
	printf("[Done] set sensor fps [%d] [%d].\n", fps, vsrc_fps.fps);
	return 0;
}

static int update_sensor_fps(void)
{
	printf("[To be done] update_vin_frame_rate !\n");
	return -1;
}

static int show_global_setting_menu(void)
{
	printf("\n================ Global Settings ================\n");
	printf("  s -- 3A library start and stop\n");
//	printf("  f -- Change Sensor fps\n");
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("G > ");
	return 0;
}

static int global_setting(int *imgproc_running)
{
	int i, exit_flag, error_opt;
	char ch;

	show_global_setting_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 's':
			printf("Imgproc library is %srunning.\n",
				*imgproc_running ? "" : "not ");
			printf("0 - Stop 3A library, 1 - Start 3A library\n> ");
			SCANF_INT(&i);
			if (i == 0) {
				if (*imgproc_running == 1) {
					mw_stop_aaa();
					*imgproc_running = 0;
				}
			} else if (i == 1) {
				if (*imgproc_running == 0) {
					mw_start_aaa(fd_iav);
					*imgproc_running = 1;
				}
			} else {
				printf("Invalid input for this option!\n");
			}
			break;
		case 'f':
			printf("Set sensor frame rate : 0 - 29.97, 1 - 30, 2 - 15, 3 - 7.5\n");
			SCANF_INT(&i);
			set_sensor_fps(i);
			update_sensor_fps();
			break;
		case 'q':
			exit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (exit_flag)
			break;
		if (error_opt == 0) {
			show_global_setting_menu();
		}
		ch = getchar();
	}
	return 0;
}

static int show_exposure_setting_menu(void)
{
	printf("\n================ Exposure Control Settings ================\n");
	printf("  E -- AE enable and disable\n");
	printf("  e -- Set exposure level\n");
	printf("  r -- Get current shutter time and sensor gain\n");
	printf("  G -- Manually set sensor gain\n");
	printf("  s -- Manually set shutter time\n");
	if(mw_is_dc_iris_supported()) {
		printf("  d -- DC iris PID coefficients\n");
		printf("  D -- DC iris enable and disable\n");
	}
	if(mw_is_ir_led_supported()) {
		printf("  i -- IR led control parameters\n");
		printf("  I -- IR led control mode\n");
	}
	if (G_mw_info.res.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
		printf("  a -- Anti-flicker mode\n");
		printf("  T -- Shutter time max\n");
		printf("  t -- Shutter time min\n");
		printf("  R -- Slow shutter enable and disable\n");
		printf("  g -- Sensor gain max\n");
		printf("  m -- Set AE metering mode\n");
		printf("  l -- Get AE current lines\n");
		//printf("  L -- Set AE customer lines\n");
		printf("  p -- Set AE switch point\n");
		printf("  C -- Setting ae area following pre-main input buffer enable and disable\n");
		printf("  y -- Get Luma value\n");
		printf("  P -- Piris lens aperture range\n");
	} else {
		printf("  x -- Set HDR blend exposure ratio\n");
		printf("  b -- Set HDR blend boost factor\n");
		printf("  X -- Get HDR blend exposure ratio and boost factor\n");
		printf("  c -- Tone Curve enable and disable\n");
	}
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("E > ");
	return 0;
}

static int exposure_setting(void)
{
	int i, exit_flag, error_opt;
	char ch;
	mw_dc_iris_pid_coef pid;
	mw_ir_led_control_param ir_led_param;
	mw_hdr_blend_info hdr_blend_cfg;

	mw_ae_metering_table	custom_ae_metering_table[2] = {
		{	//Left half window as ROI
			{1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,}
		},
		{	//Right half window as ROI
			{0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,}
		},
	};
	mw_ae_line lines[10];
	mw_ae_point switch_point;
	int value[MAX_HDR_EXPOSURE_NUM];
	mw_luma_value luma;
	int ret = 0;
	float lens_aperture[2];

	show_exposure_setting_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		if (mw_get_sys_info(&G_mw_info) < 0) {
			printf("mw_get_sensor_hdr_expo error\n");
			return -1;
		}
		switch (ch) {
		case 'E':
			printf("0 - AE disable  1 - AE enable\n> ");
			SCANF_INT(&i);
			if (mw_enable_ae(i) < 0) {
				printf("mw_enable_ae error\n");
			}
			break;
		case 'e':
			mw_get_exposure_level(value);
			printf("Current exposure level: \n");
			for (i = 0; i < G_mw_info.res.hdr_expo_num; i++) {
				printf(" Exposure frame Index[%d] = %d \n", i, value[i]);
			}
			if (G_mw_info.res.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
				printf("Input new exposure level: (range 25 ~ 400)\n> ");
				SCANF_INT(value);
			} else {
				printf("Select the exposure frame index[0~%d]:\n> ",
					G_mw_info.res.hdr_expo_num -1);
				SCANF_INT(&i);
				if (i >= G_mw_info.res.hdr_expo_num) {
					printf("Error:The value must be [0~%d]!\n",
						G_mw_info.res.hdr_expo_num -1);
					break;
				}
				printf("Input new exposure level for exposure frame index %d (25~400)\n> ", i);
				SCANF_INT(&value[i]);
			}
			if (mw_set_exposure_level(value) < 0) {
				printf("mw_set_exposure_level error\n");
			}
			break;
		case 'a':
			printf("Anti-flicker mode? 0 - 50Hz  1 - 60Hz\n> ");
			SCANF_INT(&i);
			mw_get_ae_param(&ae_param);
			ae_param.anti_flicker_mode = i;
			if (mw_set_ae_param(&ae_param) < 0) {
				printf("mw_set_ae_param error\n");
			}
			break;
		case 's':
			if (mw_get_shutter_time(fd_iav, value) < 0) {
				printf("mw_get_shutter_time error\n");
				return -1;
			}
			if (G_mw_info.res.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
				printf("Current shutter time is 1/%d sec.\n",
					DIV_ROUND(512000000, value[0]));
				printf("Input new shutter time in 1/n sec format: (range 1 ~ 8000)\n> ");
				SCANF_INT(&value[0]);
				value[0] = DIV_ROUND(512000000, value[0]);
				if (mw_set_shutter_time(fd_iav, value) < 0) {
					printf("mw_set_shutter_time error\n");
				}
			} else {
				printf("Current Row hdr shutter mode:\n");
				for (i = 0; i < G_mw_info.res.hdr_expo_num; i++) {
					printf("[%d]: %8d\t", i, DIV_ROUND(512000000, value[i]));
				}
				printf("\nInput the new Shutter Frame index[0~%d]:\n> ",
					G_mw_info.res.hdr_expo_num -1);
				SCANF_INT(&i);
				if (i >= G_mw_info.res.hdr_expo_num) {
					printf("Error:The value must be [0~%d]!\n",
						G_mw_info.res.hdr_expo_num -1);
					break;
				}
				printf("Input new Shutter for Frame index %d\n> ", i);
				SCANF_INT(&value[i]);
				value[i] = DIV_ROUND(512000000, value[i]);
				if  (mw_set_shutter_time(fd_iav, value) < 0) {
					printf("mw_set_shutter_time error\n");
				}
			}
			break;
		case 'T':
			mw_get_ae_param(&ae_param);
			printf("Current shutter time max is 1/%d sec\n",
				DIV_ROUND(512000000, ae_param.shutter_time_max));
			printf("Input new shutter time max in 1/n sec fomat? (Ex, 6, 10, 15...)(slow shutter)\n> ");
			SCANF_INT(&i);
			ae_param.shutter_time_max = DIV_ROUND(512000000, i);
			if (mw_set_ae_param(&ae_param) < 0) {
				printf("mw_set_ae_param error\n");
			}
			break;
		case 't':
			mw_get_ae_param(&ae_param);
			printf("Current shutter time min is 1/%d sec\n",
				DIV_ROUND(512000000, ae_param.shutter_time_min));
			printf("Input new shutter time min in 1/n sec fomat? (Ex, 120, 200, 400 ...)\n> ");
			SCANF_INT(&i);
			ae_param.shutter_time_min = DIV_ROUND(512000000, i);
			if (mw_set_ae_param(&ae_param) < 0) {
				printf("mw_set_ae_param error\n");
			}
			break;
		case 'R':
			mw_get_ae_param(&ae_param);
			printf("Current slow shutter mode is %s.\n",
				ae_param.slow_shutter_enable ? "enabled" : "disabled");
			printf("Input slow shutter mode? (0 - disable, 1 - enable)\n> ");
			SCANF_INT(&i);
			ae_param.slow_shutter_enable = !!i;
			if (mw_set_ae_param(&ae_param) < 0) {
				printf("mw_set_ae_param error\n");
			}
			break;
		case 'I':
			if (mw_is_ir_led_supported()) {
				mw_get_ae_param(&ae_param);
				printf("Current IR led mode is %d. (0 - off, 1 - on, 2 -- auto)\n",
					ae_param.ir_led_mode);
				printf("Input IR led mode? (0 - off, 1 - on, 2 -- auto)\n> ");
				SCANF_INT(&i);
				ae_param.ir_led_mode = i;
				if (ae_param.ir_led_mode == 1) {
					printf("Input IR led brightness? (1 ~ 99, 99 - max brightness)\n> ");
					SCANF_INT(&i);
					if (mw_set_ir_led_brightness(i) < 0) {
						printf("mw_set_ir_led_brightness error\n");
						break;
					}
				}
				if (mw_set_ae_param(&ae_param) < 0) {
					printf("mw_set_ae_param error\n");
				}
			} else {
				printf("Current board doesn't support IR LED!\n");
			}
			break;
		case 'i':
			if (mw_is_ir_led_supported()) {
				mw_get_ir_led_param(&ir_led_param);
				printf("Current IR led control parameters are:\n"
						"open_threshold=%d, close_threshold=%d\n"
						"threshold_1X=%d, threshold_2X=%d, threshold_3X=%d\n"
						"open_delay=%d(x0.5s), close_delay=%d(x0.5s)\n",
						ir_led_param.open_threshold, ir_led_param.close_threshold,
						ir_led_param.threshold_1X, ir_led_param.threshold_2X,
						ir_led_param.threshold_3X,
						ir_led_param.open_delay, ir_led_param.close_delay);
				printf("Input new open_threshold\n> ");
				SCANF_INT(&ir_led_param.open_threshold);
				printf("Input new close_threshold\n> ");
				SCANF_INT(&ir_led_param.close_threshold);
				printf("Input new threshold_1X\n> ");
				SCANF_INT(&ir_led_param.threshold_1X);
				printf("Input new threshold_2X\n> ");
				SCANF_INT(&ir_led_param.threshold_2X);
				printf("Input new threshold_3X\n> ");
				SCANF_INT(&ir_led_param.threshold_3X);
				printf("Input new open_delay\n> ");
				SCANF_INT(&ir_led_param.open_delay);
				printf("Input new close_delay\n> ");
				SCANF_INT(&ir_led_param.close_delay);
				if (mw_set_ir_led_param(&ir_led_param) < 0) {
					printf("mw_set_ir_led_param error\n");
				}
			} else {
				printf("Current board doesn't support IR LED!\n");
			}
			break;
		case 'g':
			mw_get_ae_param(&ae_param);
			printf("Current sensor gain max is %d dB\n",
				ae_param.sensor_gain_max);
			printf("Input new sensor gain max in dB? (Ex, 24, 36, 48 ...)\n> ");
			SCANF_INT(&i);
			ae_param.sensor_gain_max = i;
			if (mw_set_ae_param(&ae_param) < 0) {
				printf("mw_set_ae_param error\n");
			}
			break;
		case 'G':
			if (mw_get_sensor_gain(fd_iav, &value[0]) < 0) {
				printf("mw_get_sensor_gain error\n");
				break;
			}
			if (G_mw_info.res.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
				printf("Current sensor gain is %d dB\n", value[0] >> 24);
				printf("Input new sensor gain in dB: (range 0 ~ 36)\n> ");
				SCANF_INT(&value[0]);
				value[0] = value[0] << 24;
				if  (mw_set_sensor_gain(fd_iav, &value[0]) < 0) {
					printf("mw_set_sensor_gain error\n");
				}
			} else {
				printf("\nAgc dB:\n");
				for (i = 0; i < G_mw_info.res.hdr_expo_num; i++) {
					printf("[%d]: %8d\t", i,
						 (value[i] * G_mw_info.sensor.step) >> 24);
				}
				printf("\nInput the new AGC Frame index [0~%d]:\n> ",
					G_mw_info.res.hdr_expo_num -1);
				SCANF_INT(&i);
				if (i >= G_mw_info.res.hdr_expo_num) {
					printf("Error:The value must be [0~%d]!\n",
						G_mw_info.res.hdr_expo_num -1);
					break;
				}
				printf("Input new AGC for Frame index %d\n> ", i);
				SCANF_INT(&value[i]);
				value[i] = (value[i] << 24) / G_mw_info.sensor.step;
				if  (mw_set_sensor_gain(fd_iav, value) < 0) {
					printf("mw_set_sensor_gain error\n");
				}
			}
			break;
		case 'm':
			printf("0 - Spot Metering, 1 - Center Metering, 2 - Average Metering, 3 - Custom Metering\n");
			mw_get_ae_metering_mode(&ae_param.ae_metering_mode);
			printf("Current ae metering mode is %d\n", ae_param.ae_metering_mode);
			printf("Input new ae metering mode:\n> ");
			SCANF_INT((int *)&ae_param.ae_metering_mode);
			mw_set_ae_metering_mode(ae_param.ae_metering_mode);
			if (ae_param.ae_metering_mode != MW_AE_CUSTOM_METERING)
				break;
			printf("Please choose the AE window:\n");
			printf("0 - left half window, 1 - right half window\n> ");
			SCANF_INT(&i);
			if (mw_set_ae_metering_table(&custom_ae_metering_table[i]) < 0) {
				printf("mw_set_ae_metering_table error\n");
			}
			break;
		case 'l':
			printf("Get current AE lines:\n");
			mw_get_ae_lines(lines, 10);
			break;
		case 'L':
			printf("Set customer AE lines: (0: 60Hz, 1: 50Hz, 2: customize)\n> ");
			SCANF_INT(&i);
			if ((i != 0) && (i != 1) && (i != 2)) {
				printf("Invalid input : %d.\n", i);
				break;
			} else if (i == 0) {
				memcpy(lines, ae_60hz_lines, sizeof(ae_60hz_lines));
				ret = mw_set_ae_lines(lines, sizeof(ae_60hz_lines) / sizeof(mw_ae_line));

			} else if (i == 1) {
				memcpy(lines, ae_50hz_lines, sizeof(ae_50hz_lines));
				ret = mw_set_ae_lines(lines, sizeof(ae_50hz_lines) / sizeof(mw_ae_line));
			} else if (i == 2) {
				memcpy(lines, ae_customer_lines, sizeof(ae_customer_lines));
				ret = mw_set_ae_lines(lines, sizeof(ae_customer_lines) / sizeof(mw_ae_line));
			}
			if (ret < 0) {
				printf("mw_set_ae_lines error\n");
			}
			break;
		case 'p':
			printf("Please set switch point in AE lines :\n");
			printf("Input shutter time in 1/n sec format: (range 30 ~ 120)\n> ");
			SCANF_INT(&i);
			switch_point.factor[MW_SHUTTER] = DIV_ROUND(512000000, i);
			printf("Input switch point of AE line: (0 - start point; 1 - end point)\n> ");
			SCANF_INT(&i);
			switch_point.pos = i;
			printf("Input sensor gain in dB: (range 0 ~ 36)\n> ");
			SCANF_INT(&i);
			switch_point.factor[MW_DGAIN] = i;
			if (mw_set_ae_points(&switch_point, 1) < 0) {
				printf("mw_set_ae_points error\n");
			}
			break;
		case 'd':
			mw_get_dc_iris_pid_coef(&pid);
			printf("Current PID coefficients are: p_coef=%d,i_coef=%d,d_coef=%d\n",
					pid.p_coef, pid.i_coef, pid.d_coef);
			printf("Input new p_coef\n> ");
			SCANF_INT(&pid.p_coef);
			printf("Input new i_coef\n> ");
			SCANF_INT(&pid.i_coef);
			printf("Input new d_coef\n> ");
			SCANF_INT(&pid.d_coef);
			if (mw_set_dc_iris_pid_coef(&pid) < 0) {
				printf("mw_set_dc_iris_pid_coef error\n");
			}
			break;
		case 'D':
			if (mw_is_dc_iris_supported()) {
				printf("DC iris control? 0 - disable  1 - enable\n> ");
				SCANF_INT(&i);
				if (mw_enable_dc_iris_control(i) < 0) {
					printf("mw_enable_dc_iris_control error\n");
				}
			} else {
				printf("Current board doesn't support DC IRIS!\n");
			}
			break;
		case 'y':
			mw_get_ae_luma_value(&luma);
			printf("Current rgb luma value = %d, cfa_luma_value = %d\n", luma.rgb_luma, luma.cfa_luma);
			break;
		case 'r':
			if (mw_get_shutter_time(fd_iav, value) < 0) {
				printf("mw_get_shutter_time error\n");
				return -1;
			}
			if (G_mw_info.res.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
				printf("Current shutter time is 1/%d sec.\n",
					DIV_ROUND(512000000, value[0]));
				mw_get_sensor_gain(fd_iav, value);
				printf("Current sensor gain is %d dB.\n", (value[0] >> 24));
			} else {
				printf("HDR shutter time:\n");
				for (i = 0; i < G_mw_info.res.hdr_expo_num; i++) {
					printf("[%d]: 1/%ds\t", i, DIV_ROUND(512000000, value[i]));
				}
				printf("\n");
				mw_get_sensor_gain(fd_iav, value);
				printf("HDR sensor gain dB:\n");
				for (i = 0; i < G_mw_info.res.hdr_expo_num; i++) {
					printf("[%d]: %8d\t", i,
						(value[i] * G_mw_info.sensor.step) >> 24);
				}
				printf("\n");
			}
			break;
		case 'P':
			if (mw_get_ae_param(&ae_param) < 0) {
				printf("mw_get_piris_lens_param error\n");
				return -1;
			}
			printf("The current range is [F%3.2f ~ F%3.2f]\n",
				(float)ae_param.lens_aperture.aperture_min / LENS_FNO_UNIT,
				(float)ae_param.lens_aperture.aperture_max / LENS_FNO_UNIT);
			printf("The spec range must be in [F%3.2f ~ F%3.2f]\n",
				(float)(ae_param.lens_aperture.FNO_min) / LENS_FNO_UNIT,
				(float)(ae_param.lens_aperture.FNO_max) / LENS_FNO_UNIT);
			printf("Input the new range:(E.x, 1.2, 2.0)\n> ");

			if (scanf("%f,%f", &lens_aperture[0], &lens_aperture[1]) < 0) {
				printf("input error.\n");
				return -1;
			}

			ae_param.lens_aperture.aperture_min = (u32)(lens_aperture[0] * LENS_FNO_UNIT);
			ae_param.lens_aperture.aperture_max = (u32)(lens_aperture[1] * LENS_FNO_UNIT);

			if (mw_set_ae_param(&ae_param) < 0) {
				printf("mw_set_piris_lens_param error\n");
				return -1;
			}
			break;
		case 'x':
			if (mw_get_hdr_blend_config(&hdr_blend_cfg) < 0) {
				printf("mw_get_hdr_blend_config failed!\n");
				return -1;
			}
			printf("The current exposure ratio is %d\n", hdr_blend_cfg.expo_ratio);
			printf("Please input new exposure ratio:\n>");
			if (scanf("%hu", &hdr_blend_cfg.expo_ratio) < 0) {
				printf("input error.\n");
				return -1;
			}
			if (mw_set_hdr_blend_config(&hdr_blend_cfg) < 0) {
				printf("mw_set_hdr_blend_config failed!\n");
				return -1;
			}
			break;
		case 'b':
			if (mw_get_hdr_blend_config(&hdr_blend_cfg) < 0) {
				printf("mw_get_hdr_blend_config failed!\n");
				return -1;
			}
			printf("The current exposure boost is %d\n", hdr_blend_cfg.boost_factor);
			printf("Please input new boost factor: 0 ~ 256, 0 means no boost\n>");
			if (scanf("%hu", &hdr_blend_cfg.boost_factor) < 0) {
				printf("input error.\n");
				return -1;
			}
			if (mw_set_hdr_blend_config(&hdr_blend_cfg) < 0) {
				printf("mw_set_hdr_blend_config failed!\n");
				return -1;
			}
			break;
		case 'X':
			if (mw_get_hdr_blend_config(&hdr_blend_cfg) < 0) {
				printf("mw_get_hdr_blend_config failed!\n");
				return -1;
			}
			printf("The current exposure ratio is %d, boost factor is %d\n",
					hdr_blend_cfg.expo_ratio, hdr_blend_cfg.boost_factor);
			break;
		case 'c':
			mw_get_ae_param(&ae_param);
			printf("Current tone curver duration is %d.\n",
				ae_param.tone_curve_duration);
			printf("Input tone curver duration? (1~30 - Update tone curver every n frames)\n> ");
			SCANF_INT(&i);
			ae_param.tone_curve_duration = i;
			if (mw_set_ae_param(&ae_param) < 0) {
				printf("mw_set_ae_param error\n");
			}
			break;
		case 'q':
			exit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (exit_flag)
			break;
		if (error_opt == 0) {
			show_exposure_setting_menu();
		}
		ch = getchar();
	}
	return 0;
}

static int show_white_balance_menu(void)
{
	printf("\n================ White Balance Settings ================\n");
	printf("  W -- AWB enable and disable\n");
	printf("  m -- Select AWB mode(Need on AWB method: Normal method)\n");
	printf("  M -- Select AWB method\n");
	printf("  c -- Set RGB gain for custom mode (Don't need to turn off AWB)\n");
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("W > ");
	return 0;
}

static int white_balance_setting(void)
{
	int i, exit_flag, error_opt;
	char ch;
	mw_wb_gain wb_gain[MAX_HDR_EXPOSURE_NUM];
	mw_white_balance_method wb_method;
	mw_white_balance_mode wb_mode;

	show_white_balance_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'W':
			printf("0 - AWB disable  1 - AWB enable\n> ");
			SCANF_INT(&i);
			mw_enable_awb(i);
			break;
		case 'm':
			if (mw_get_white_balance_mode(&wb_mode) < 0) {
				printf("mw_get_white_balance_mode error\n");
				break;
			}
			printf("Current AWB mode is:%d \n", wb_mode);
			printf("Choose AWB mode (%d~%d):\n",
				MW_WB_AUTO, MW_WB_MODE_NUMBER -1);
			printf("0 - auto 1 - 2800K 2 - 4000K 3 - 5000K 4 - 6500K 5 - 7500K");
			printf(" ... 10 - custom\n> ");
			SCANF_INT(&i);
			if (mw_set_white_balance_mode(i) < 0) {
				printf("mw_set_white_balance_mode error\n");
			}
			break;
		case 'M':
			mw_get_awb_method(&wb_method);
			printf("Current AWB method is [%d].\n", wb_method);
			printf("Choose AWB method (0 - Normal, 1 - Custom, 2 - Grey world)\n> ");
			SCANF_INT(&i);
			if (mw_set_awb_method(i) < 0) {
				printf("mw_set_awb_method error\n");
			}
			break;
		case 'c':
			getchar();
			printf("Enter custom method\n");
			mw_set_awb_method(MW_WB_CUSTOM_METHOD);
			printf("Must put a gray card in the front of the lens at first\n");
			printf("Then press any button\n");
			getchar();
			mw_get_awb_manual_gain(wb_gain);
			printf("Current WB gain is %d, %d, %d\n", wb_gain[0].r_gain,
				wb_gain[0].g_gain, wb_gain[0].b_gain);
			printf("Input new RGB gain for normal method - custom mode \n");
			printf("(Ex, 1500,1024,1400) \n > ");
			if (scanf("%d,%d,%d", &wb_gain[0].r_gain, &wb_gain[0].g_gain,
				&wb_gain[0].b_gain) < 0) {
				printf("input error.\n");
				return -1;
			}

			printf("Enter normal method:custom mode \n");
			if (mw_set_awb_method(MW_WB_NORMAL_METHOD) < 0) {
				printf("mw_set_awb_method error\n");
				break;
			}
			if (mw_set_white_balance_mode(MW_WB_CUSTOM) < 0) {
				printf("mw_set_white_balance_mode error\n");
				break;
			}
			if (mw_set_awb_custom_gain(wb_gain) < 0) {
				printf("mw_set_awb_custom_gain error\n");
				break;
			}
			printf("The new RGB gain you set for custom mode is : %d, %d, %d\n",
				wb_gain[0].r_gain, wb_gain[0].g_gain, wb_gain[0].b_gain);
			break;
		case 'q':
			exit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (exit_flag)
			break;
		if (error_opt == 0) {
			show_white_balance_menu();
		}
		ch = getchar();
	}
	return 0;
}

static int show_adjustment_menu(void)
{
	printf("\n================ Image Adjustment Settings ================\n");
	printf("  a -- Saturation adjustment\n");
	if (G_mw_info.res.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
		printf("  b -- Brightness adjustment\n");
	}
	printf("  c -- Contrast adjustment\n");
	printf("  s -- Sharpness adjustment\n");
	printf("  P -- Sharpness property ratio adjustment\n");
	printf("  l -- Sample code for load 3A params dynamically\n");
	printf("  L -- Reload adj bin file\n");
	printf("  x -- Save adj bin file\n");
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("A > ");
	return 0;
}

static int reload_adj_bin_file(char *filename)
{
	mw_adj_file_param *contents = NULL;

	if (filename == NULL) {
		printf("The point is NULL\n");
		return -1;

	}
	contents = malloc(sizeof(mw_adj_file_param));

	if (contents == NULL) {
		printf("Malloc fail\n");
		return -1;
	}
	memset(contents, 0, sizeof(mw_adj_file_param));
	if ((strlen(filename) > 0) && (strlen(filename) < FILE_NAME_LENGTH)) {
		memcpy(contents->filename, filename, strlen(filename));
	} else {
		printf("the length of file name must be [0~63].\n");
		return -1;
	}

	/* Updaate filter: ADJ_AE_TARGET & ADJ_BLC for demo */
	contents->flag[ADJ_AE_TARGET].apply = 1;
	contents->flag[ADJ_BLC].apply = 1;
	if (mw_load_adj_param(contents) < 0) {
		printf("mw_reload_adj_param error\n");
		return -1;
	}
	if (contents->flag[ADJ_AE_TARGET].done) {
		printf("Load ADJ_AE_TARGET successfully\n");

	}
	if (contents->flag[ADJ_BLC].done) {
		printf("Load ADJ_BLC successfully\n");
	}

	if (contents != NULL) {
		free(contents);
		contents = NULL;
	}

	return 0;
}

static int save_adj_bin_file(char *filename)
{
	mw_adj_file_param *contents = NULL;

	if (filename == NULL) {
		printf("The point is NULL\n");
		return -1;

	}
	contents = malloc(sizeof(mw_adj_file_param));

	if (contents == NULL) {
		printf("Malloc fail\n");
		return -1;
	}
	memset(contents, 0, sizeof(mw_adj_file_param));
	if ((strlen(filename) > 0) && (strlen(filename) < FILE_NAME_LENGTH)) {
		memcpy(contents->filename, filename, strlen(filename));
	} else {
		printf("the length of file name must be [0~63].\n");
		return -1;
	}

	/* Updaate filter: ADJ_AE_TARGET & ADJ_BLC for demo */
	contents->flag[ADJ_AE_TARGET].apply = 1;
	contents->flag[ADJ_BLC].apply = 1;
	if (mw_save_adj_param(contents) < 0) {
		printf("mw_save_adj_param error\n");
		return -1;
	}
	if (contents->flag[ADJ_AE_TARGET].done) {
		printf("Save ADJ_AE_TARGET successfully\n");
	}
	if (contents->flag[ADJ_BLC].done) {
		printf("Save ADJ_BLC successfully\n");
	}

	if (contents != NULL) {
		free(contents);
		contents = NULL;
	}

	return 0;
}

static int adjustment_setting(void)
{
	int i, exit_flag, error_opt;
	char ch, str[FILE_NAME_LENGTH];
	fc_collection_t fcc;
	int ret = 0;

	show_adjustment_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'a':
			mw_get_saturation(&i);
			printf("Current saturation is %d\n", i);
			printf("Input new saturation: (range 0 ~ 255)\n> ");
			SCANF_INT(&i);
			if (mw_set_saturation(i) < 0) {
				printf("mw_set_saturation error\n");
			}
			break;
		case 'b':
			mw_get_brightness(&i);
			printf("Current brightness is %d\n", i);
			printf("Input new brightness: (range -255 ~ 255)\n> ");
			SCANF_INT(&i);
			if (mw_set_brightness(i) < 0) {
				printf("mw_set_brightness error\n");
			}
			break;
		case 'c':
			mw_get_contrast(&i);
			printf("Current contrast is %d\n", i);
			printf("Input new contrast: (range 0 ~ 128)\n> ");
			SCANF_INT(&i);
			if (mw_set_contrast(i) < 0) {
				printf("mw_set_contrast error\n");
			}
			break;
		case 's':
			mw_get_sharpness(&i);
			printf("Current sharpness is %d\n", i);
			printf("Input new sharpness: (range 0 ~ 11)\n> ");
			SCANF_INT(&i);
			if (mw_set_sharpness(i) < 0) {
				printf("mw_set_sharpness error\n");
			}
			break;
		case 'p':
			printf("Input new sharpness property ratio Index (range 0 ~ %d)\n> ",
				SHARPEN_PROPERTY_MAX_NUM - 1);
			SCANF_INT(&i);
			if (i  >= SHARPEN_PROPERTY_MAX_NUM || i < 0) {
				printf("The value must be(0 ~ %d)\n> ",
					SHARPEN_PROPERTY_MAX_NUM - 1);
			} else {
				printf("Set Sharpen property ratio: %d, %d, %d, %d, %d\n",
					sharpen_property_array[i].cfa_noise_level_ratio,
					sharpen_property_array[i].cfa_extent_regular_ratio,
					sharpen_property_array[i].both_max_change_ratio,
					sharpen_property_array[i].sharpen_CoringidxScale_ratio,
					sharpen_property_array[i].sharpen_fractionalBits);
				ret = img_set_sharpen_property_ratio(&sharpen_property_array[i]);
				if (ret < 0) {
					printf("img_set_sharpen_property_ratio error\n");
				}
			}
			break;
		case 'l':
			memset(&fcc, 0, sizeof(fcc));
			fcc.fc_1st_shpboth = &liso_fc_1st_shpboth;
			if (img_dynamic_load_containers(&fcc) < 0) {
				printf("img_dynamic_load_containers error\n");
			}
			break;
		case 'L':
			printf("Input the filename of adj param bin:\n> ");
			SCANF_STR(str);
			reload_adj_bin_file(str);
			break;
		case 'x':
			printf("Input the filename of adj param bin:\n> ");
			SCANF_STR(str);
			save_adj_bin_file(str);
			break;
		case 'q':
			exit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (exit_flag)
			break;
		if (error_opt == 0) {
			show_adjustment_menu();
		}
		ch = getchar();
	}
	return 0;
}

static int show_enhancement_menu(void)
{
	printf("\n================ Image Enhancement Settings ================\n");
	printf("  m -- Set MCTF 3D noise filter strength\n");
	if (G_mw_info.res.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
		printf("  L -- Set auto local exposure mode\n");
		printf("  l -- Load custom local exposure curve from file\n");
		printf("  b -- Enable backlight compensation\n");
		printf("  d -- Enable day and night mode\n");
		printf("  c -- Enable auto contrast mode\n");
		printf("  t -- Set auto contrast strength\n");
		printf("  n -- Set chroma noise filter strength\n");
		printf("  w -- Set auto wdr strength\n");
	}
	printf("  u -- Set wdr luma\n");
	printf("  j -- ADJ enable and disable\n");
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("H > ");
	return 0;
}

static int enhancement_setting(void)
{
	int i, exit_flag, error_opt;
	char ch;
	u32 value;
	char str[64];
	mw_local_exposure_curve local_exposure_curve;
	mw_wdr_luma_info wdr_luma_info;

	show_enhancement_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'm':
			mw_get_mctf_strength(&value);
			printf("Current mctf strength is %d\n", value);
			printf("Input new mctf strength: (range 0 ~ 11)\n> ");
			SCANF_INT(&value);
			if (mw_set_mctf_strength(value) < 0) {
				printf("mw_set_mctf_strength error\n");
			}
			break;
		case 'L':
			printf("Auto local exposure: 0 - Stop, 1 - Auto, 64~128 - weakest~strongest\n> ");
			SCANF_INT(&i);
			if (mw_set_auto_local_exposure_mode(i) < 0) {
				printf("mw_set_auto_local_exposure_mode error\n");
			}
			break;
		case 'l':
			printf("Input the filename of local exposure curve: (Ex, le_2x.txt)\n> ");
			SCANF_STR(str);
			if (load_local_exposure_curve(str, &local_exposure_curve) < 0) {
				printf("The curve file %s is either found or corrupted!\n", str);
				return -1;
			}
			if (mw_set_local_exposure_curve(fd_iav, &local_exposure_curve) < 0) {
				printf("mw_set_local_exposure_curve error\n");
			}
			break;
		case 'b':
			printf("Backlight compensation: 0 - disable  1 - enable\n> ");
			SCANF_INT(&i);
			if (mw_enable_backlight_compensation(i) < 0) {
				printf("mw_enable_backlight_compensation error\n");
			}
			break;
		case 'd':
			printf("Day and night mode: 0 - disable  1 - enable\n> ");
			SCANF_INT(&i);
			if (mw_enable_day_night_mode(i) < 0) {
				printf("mw_enable_day_night_mode error\n");
			}
			break;
		case 'c':
			mw_get_auto_color_contrast(&value);
			printf("Current auto contrast control mode is %s\n", value ? "enabled" : "disabled");
			printf("Auto contrast control mode: 0 - disable  1 - enable\n> ");
			SCANF_INT(&i);
			if (mw_set_auto_color_contrast(i) < 0) {
				printf("mw_set_auto_color_contrast error\n");
			}
			break;
		case 't':
			printf("Set auto contrast control strength: 0~128, 0: no effect, 128: full effect\n> ");
			SCANF_INT(&i);
			if (mw_set_auto_color_contrast_strength(i) < 0) {
				printf("mw_set_auto_color_contrast_strength error\n");
			}
			break;
		case 'j':
			mw_get_adj_status(&i);
			printf("Current ADJ is %s\n", i ? "enabled" : "disabled");
			printf("Change ADJ mode: 0 - disable  1 - enable\n> ");
			SCANF_INT(&i);
			if (mw_enable_adj(i) < 0) {
				printf("mw_enable_adj error\n");
			}
			break;
		case 'n':
			if (mw_get_chroma_noise_strength(&i) < 0) {
				printf("mw_get_chroma_noise_strength error");
				break;
			}
			printf("Current chroma_noise_filter strength is %d\n", i);
			printf("Change chroma_noise_filter strength: 0~256)\n> ");
			SCANF_INT(&i);
			if (mw_set_chroma_noise_strength(i) < 0) {
				printf("mw_set_chroma_noise_strength error");
			}
			break;
		case 'w':
			if (mw_get_auto_wdr_strength(&i) < 0) {
				printf("mw_get_auto_wdr_strength error");
				break;
			}
			printf("Current auto wdr strength is %d\n", i);
			printf("Change auto wdr strength: 0~128)\n> ");
			SCANF_INT(&i);
			if (mw_set_auto_wdr_strength(i) < 0) {
				printf("mw_set_auto_wdr_strength error");
			}
			break;
		case 'u':
			if (mw_get_wdr_luma_config(&wdr_luma_info) < 0) {
				printf("mw_get_wdr_luma_config error");
				break;
			}
			printf("Current wdr luma config: radius = %d\n", wdr_luma_info.radius);
			printf("Current wdr luma config: luma_weight_red = %d\n", wdr_luma_info.luma_weight_red);
			printf("Current wdr luma config: luma_weight_green = %d\n", wdr_luma_info.luma_weight_green);
			printf("Current wdr luma config: luma_weight_blue = %d\n", wdr_luma_info.luma_weight_blue);
			printf("Current wdr luma config: luma_weight_shift = %d\n\n\n", wdr_luma_info.luma_weight_shift);

			printf("Now input new wdr luma config:\n");
			printf("Radius: (%d~%d)\n> ", WDR_LUMA_RADIUS_MIN, WDR_LUMA_RADIUS_MAX);
			SCANF_INT(&i);
			if ((i >= WDR_LUMA_RADIUS_MIN) && (i <= WDR_LUMA_RADIUS_MAX)) {
				wdr_luma_info.radius = i;
			} else {
				printf("invalid radius: %d, keep unchanged\n", i);
			}
			printf("luma_weight_red: (%d~%d)\n> ", WDR_LUMA_WEIGHT_MIN, WDR_LUMA_WEIGHT_MAX);
			SCANF_INT(&i);
			if ((i >= WDR_LUMA_WEIGHT_MIN) && (i <= WDR_LUMA_WEIGHT_MAX)) {
				wdr_luma_info.luma_weight_red = i;
			} else {
				printf("invalid luma_weight_red: %d, keep unchanged\n", i);
			}
			printf("luma_weight_green: (%d~%d)\n> ", WDR_LUMA_WEIGHT_MIN, WDR_LUMA_WEIGHT_MAX);
			SCANF_INT(&i);
			if ((i >= WDR_LUMA_WEIGHT_MIN) && (i <= WDR_LUMA_WEIGHT_MAX)) {
				wdr_luma_info.luma_weight_green = i;
			} else {
				printf("invalid luma_weight_green: %d, keep unchanged\n", i);
			}
			printf("luma_weight_blue: (%d~%d)\n> ", WDR_LUMA_WEIGHT_MIN, WDR_LUMA_WEIGHT_MAX);
			SCANF_INT(&i);
			if ((i >= WDR_LUMA_WEIGHT_MIN) && (i <= WDR_LUMA_WEIGHT_MAX)) {
				wdr_luma_info.luma_weight_blue = i;
			} else {
				printf("invalid luma_weight_blue: %d, keep unchanged\n", i);
			}
			printf("luma_weight_shift: (%d~%d)\n> ", WDR_LUMA_WEIGHT_MIN, WDR_LUMA_WEIGHT_MAX);
			SCANF_INT(&i);
			if ((i >= WDR_LUMA_WEIGHT_MIN) && (i <= WDR_LUMA_WEIGHT_MAX)) {
				wdr_luma_info.luma_weight_shift = i;
			} else {
				printf("invalid luma_weight_shift: %d, keep unchanged\n", i);
			}
			if (mw_set_wdr_luma_config(&wdr_luma_info) < 0) {
				printf("mw_set_wdr_luma_config error");
			}
			break;
		case 'k':
			if (mw_get_auto_knee_strength(&i) < 0) {
				printf("mw_get_auto_knee_strength error");
				break;
			}
			printf("Current auto knee strength is %d\n", i);
			printf("Change auto knee strength: 0~255)\n> ");
			SCANF_INT(&i);
			if (mw_set_auto_knee_strength(i) < 0) {
				printf("mw_set_ae_auto_knee_strenght error\n");
			}
			break;
		case 'q':
			exit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (exit_flag)
			break;
		if (error_opt == 0) {
			show_enhancement_menu();
		}
		ch = getchar();
	}
	return 0;
}

static int show_misc_menu(void)
{
	printf("\n================ Misc Settings ================\n");
	printf("  v -- Set log level\n");
	printf("  m -- get mw lib version\n");
	printf("  V -- get aaa lib version\n");
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("M > ");
	return 0;
}

static int display_aaa_lib_version(void)
{
	mw_aaa_lib_version algo_lib_version;
	mw_aaa_lib_version dsp_lib_version;
	if (mw_get_lib_version(&algo_lib_version, &dsp_lib_version) < 0) {
		perror("display_aaa_lib_version");
		return -1;
	}
	printf("\n[Algo lib info]:\n");
	printf("   Algo lib Version : %s-%d.%d.%d (Last updated: %x)\n",
		algo_lib_version.description, algo_lib_version.major,
		algo_lib_version.minor, algo_lib_version.patch,
		algo_lib_version.mod_time);
	printf("\n[DSP lib info]:\n");
	printf("   DSP lib Version : %s-%d.%d.%d (Last updated: %x)\n",
		dsp_lib_version.description, dsp_lib_version.major,
		dsp_lib_version.minor, dsp_lib_version.patch,
		dsp_lib_version.mod_time);
	return 0;
}

static int display_mw_lib_version(void)
{
	mw_version_info mw_version;
	if (mw_get_version_info(&mw_version) < 0) {
		perror("mw_get_version_info error");
		return -1;
	}
	printf("\n[MW lib info]:\n");
	printf("   MW lib major : 0x%8x, minor: 0x%8x, patch: 0x%8x (Last updated: %x)\n",
		mw_version.major, mw_version.minor, mw_version.patch, mw_version.update_time);

	if (mw_display_adj_bin_version() < 0) {
		perror("mw_get_version_info error");
		return -1;
	}
	return 0;
}

static int misc_setting(void)
{
	int i, exit_flag, error_opt;
	char ch;

	show_misc_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'v':
			printf("Input log level: (0~2)\n> ");
			SCANF_INT(&i);
			mw_set_log_level(i);
			break;
		case 'V':
			if (display_aaa_lib_version() < 0) {
				perror("display_aaa_lib_version");
			}
			break;
		case 'm':
			if (display_mw_lib_version() < 0) {
				perror("display_mw_lib_version");
			}
			break;
		case 'q':
			exit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (exit_flag)
			break;
		if (error_opt == 0) {
			show_misc_menu();
		}
		ch = getchar();
	}
	return 0;
}

static int show_statistics_menu(void)
{
	printf("\n================ Statistics Data ================\n");
	if (G_mw_info.res.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
		printf("  b -- Get AWB statistics data once\n");
		printf("  e -- Get AE statistics data once\n");
		printf("  f -- Get AF statistics data once\n");
		printf("  h -- Get Histogram data once\n");
	}
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("S > ");
	return 0;
}

static int display_AWB_data(awb_data_t *pAwb_data,
	u16 tile_num_col, u16 tile_num_row)
{
	u32 i, j, sum_r, sum_g, sum_b;

	sum_r = sum_g = sum_b = 0;

	printf("== AWB CFA = [%dx%d] = [R : G : B]\n", tile_num_col, tile_num_row);

	for (i = 0; i < tile_num_row; ++i) {
		for (j = 0; j< tile_num_col; ++j) {
			sum_r += pAwb_data[i * tile_num_col + j].r_avg;
			sum_g += pAwb_data[i * tile_num_col + j].g_avg;
			sum_b += pAwb_data[i * tile_num_col + j].b_avg;
		}
	}
	printf("== AWB  CFA Total value == [R : G : B] -> [%d : %d : %d].\n",
		sum_r, sum_g, sum_b);
	sum_r = (sum_r << 10) / sum_g;
	sum_b = (sum_b << 10) / sum_g;
	printf("== AWB = Normalized to 1024 [%d : 1024 : %d].\n",
		sum_r, sum_b);

	return 0;
}

static int display_AE_data(ae_data_t *pAe_data,
	u16 tile_num_col, u16 tile_num_row)
{
	int i, j, ae_sum;

	ae_sum = 0;


	printf("== AE = [%dx%d] ==\n", tile_num_col, tile_num_row);
	for (i = 0; i < tile_num_row; ++i) {
		for (j = 0; j< tile_num_col; ++j) {
			printf("  %5d  ",  pAe_data[i * tile_num_col + j].lin_y);
			ae_sum += pAe_data[i * tile_num_col + j].lin_y;
	}
		printf("\n");
	}
	printf("== AE = [%dx%d] == Total lum value : [%d].\n",
		tile_num_col, tile_num_row, ae_sum);

	return 0;
}

static int display_AF_data(af_stat_t * pAf_data, u16 tile_num_col, u16 tile_num_row)
{
	int i, j;
	u32 sum_fv2, sum_lum;
	u16 tmp_fv2, tmp_lum;
	sum_fv2 = 0;
	sum_lum = 0;

	printf("== AF FV2 = [%dx%d] ==\n", tile_num_col, tile_num_row);

	for (i = 0; i < tile_num_row; ++i) {
		for (j = 0; j< tile_num_col; ++j) {
			tmp_fv2 = pAf_data[i * tile_num_col + j].sum_fv2;
			printf("%6d ", tmp_fv2);
			sum_fv2 += tmp_fv2;

			tmp_lum = pAf_data[i * tile_num_col + j].sum_fy;
			sum_lum += tmp_lum;
		}
		printf("\n");
	}

	printf("== AF: FV2, Lum = [%dx%d] == Total value :%d, %d\n",
		tile_num_col, tile_num_row, sum_fv2, sum_lum);

	return 0;
}

static int display_hist_data(cfa_histogram_stat_t *cfa,
	rgb_histogram_stat_t *rgb)
{
	const int total_bin_num = HIST_BIN_NUM;
	int bin_num;
	int total_rgb_y = 0;
	int total_cfg_y = 0;

	while (1) {
		printf("  Please choose the bin number (range 0~63) : ");
		SCANF_INT(&bin_num);
		if ((bin_num >= 0) && (bin_num < total_bin_num))
			break;
		printf("  Invalid bin number [%d], choose again!\n", bin_num);
	}
	printf("== HIST CFA Bin [%d] [Y : R : G : B] = [%d : %d : %d : %d]\n",
		bin_num, cfa->his_bin_y[bin_num], cfa->his_bin_r[bin_num],
		cfa->his_bin_g[bin_num], cfa->his_bin_b[bin_num]);
	printf("== HIST RGB Bin [%d] [Y : R : G : B] = [%d : %d : %d : %d]\n",
		bin_num, rgb->his_bin_y[bin_num], rgb->his_bin_r[bin_num],
		rgb->his_bin_g[bin_num], rgb->his_bin_b[bin_num]);

	bin_num = 0;
	while (bin_num < total_bin_num) {
		 total_rgb_y += rgb->his_bin_y[bin_num];
		 total_cfg_y += cfa->his_bin_y[bin_num];
		 bin_num++;
	}
	printf("== HIST total Y:   rgb[%d], cfa[%d] ==\n", total_rgb_y, total_cfg_y);

	return 0;
}

static int init_for_get_statistics(statistic_data_t * stat)
{
	int ret = 0, size;

	size = sizeof(struct cfa_aaa_stat) * MAX_SLICE_NUM;
	stat->cfa_aaa = malloc(size);
	if (stat->cfa_aaa != NULL) {
		memset(stat->cfa_aaa, 0, size);
	}
	size = sizeof(struct rgb_aaa_stat) * MAX_SLICE_NUM;
	stat->rgb_aaa = malloc(size);
	if (stat->rgb_aaa != NULL) {
		memset(stat->rgb_aaa, 0, size);
	}
	size = sizeof(struct cfa_pre_hdr_stat) * MAX_PRE_HDR_STAT_BLK_NUM;
	stat->cfa_pre = malloc(size);
	if (stat->cfa_pre != NULL) {
		memset(stat->cfa_pre, 0, size);
	}
	stat->cfa_data_valid = malloc(sizeof(u8));
	if (stat->cfa_data_valid != NULL) {
		memset(stat->cfa_data_valid, 0, sizeof(u8));
	}
	stat->rgb_data_valid = malloc(sizeof(u8));
	if (stat->rgb_data_valid != NULL) {
		memset(stat->rgb_data_valid, 0, sizeof(u8));
	}
	stat->cfa_pre_hdr_data_valid = malloc(sizeof(u8));
	if (stat->cfa_pre_hdr_data_valid != NULL) {
		memset(stat->cfa_pre_hdr_data_valid, 0, sizeof(u8));
	}
	stat->dsp_pts_data_addr = malloc(sizeof(u32));
	if (stat->dsp_pts_data_addr != NULL) {
		memset(stat->dsp_pts_data_addr, 0, sizeof(u32));
	}
	stat->mono_pts_data_addr = malloc(sizeof(u64));
	if (stat->mono_pts_data_addr != NULL) {
		memset(stat->mono_pts_data_addr, 0, sizeof(u64));
	}

	if (!(stat->cfa_aaa && stat->rgb_aaa && stat->cfa_pre
		&& stat->cfa_data_valid && stat->rgb_data_valid
		&& stat->cfa_pre_hdr_data_valid &&
		stat->dsp_pts_data_addr && stat->mono_pts_data_addr)) {
		ret = -1;
	}

	return ret;
}

static int deinit_for_get_statistics(statistic_data_t * stat)
{
	if (stat->cfa_aaa) {
		free(stat->cfa_aaa);
		stat->cfa_aaa = NULL;
	}
	if (stat->rgb_aaa) {
		free(stat->rgb_aaa);
		stat->rgb_aaa = NULL;
	}
	if (stat->cfa_pre) {
		free(stat->cfa_pre);
		stat->cfa_pre = NULL;
	}
	if (stat->cfa_data_valid) {
		free(stat->cfa_data_valid);
		stat->cfa_data_valid = NULL;
	}
	if (stat->rgb_data_valid) {
		free(stat->rgb_data_valid);
		stat->rgb_data_valid = NULL;
	}
	if (stat->cfa_pre_hdr_data_valid) {
		free(stat->cfa_pre_hdr_data_valid);
		stat->cfa_pre_hdr_data_valid = NULL;
	}
	if (stat->dsp_pts_data_addr) {
		free(stat->dsp_pts_data_addr);
		stat->dsp_pts_data_addr = NULL;
	}
	if (stat->mono_pts_data_addr) {
		free(stat->mono_pts_data_addr);
		stat->mono_pts_data_addr = NULL;
	}
	return 0;
}

void parse_hist_data(histogram_data_t* hist, img_aaa_stat_t* stat, int expo_num)
{
	int i = 0;
	for(i = 0; i < expo_num; i++) {
		memcpy(&hist->cfa_pre_histogram[i], &stat[i].cfa_pre_hdr_hist,
			sizeof(cfa_pre_hdr_histogram_t));
	}
	memcpy(&(hist->cfa_histogram), &stat[0].cfa_hist, sizeof(cfa_histogram_stat_t));
	memcpy(&(hist->rgb_histogram), &stat[0].rgb_hist, sizeof(rgb_histogram_stat_t));
}

static int get_statistics_data(STATISTICS_TYPE type)
{
	amba_dsp_aaa_statistic_data_t g_stat;
	amba_img_dsp_mode_cfg_t ik_mode;
	aaa_tile_report_t act_tile;
	histogram_data_t histo_data;
	statistic_data_t stat;
	int retv = 0;
	int i;
	int times;
	struct img_statistics mw_cmd;
	int test_get_satistics_flag =
		(test_flag & (test_option == FETCH_AAA_STATISTICS)) ? 1 : 0;
	printf("Get times [1~65535]:");
	times = input_value(1, 65535);

	memset(&ik_mode, 0, sizeof(ik_mode));

	img_aaa_stat_t hdr_statis_gp[MAX_HDR_EXPOSURE_NUM];

	memset(hdr_statis_gp, 0, MAX_HDR_EXPOSURE_NUM*sizeof(img_aaa_stat_t));

	if (init_for_get_statistics(&stat) < 0) {
		perror("init_for_get_statistics");
		deinit_for_get_statistics(&stat);
		return -1;
	}

	memset(&g_stat, 0, sizeof(amba_dsp_aaa_statistic_data_t));
	g_stat.CfaAaaDataAddr = (u32)(stat.cfa_aaa);
	g_stat.RgbAaaDataAddr = (u32)(stat.rgb_aaa);
	g_stat.CfaPreHdrDataAddr = (u32)(stat.cfa_pre);
	g_stat.RgbDataValid = (u32)(stat.rgb_data_valid);
	g_stat.CfaDataValid = (u32)(stat.cfa_data_valid);
	g_stat.CfaPreHdrDataValid = (u32)(stat.cfa_pre_hdr_data_valid);
	g_stat.DspPtsDataAddr = (u32)(stat.dsp_pts_data_addr);;
	g_stat.MonoPtsDataAddr = (u32)(stat.mono_pts_data_addr);;

	if (test_get_satistics_flag) {
		mw_cmd.rgb_statis = (void *)stat.rgb_aaa;
		mw_cmd.cfa_statis = (void *)stat.cfa_aaa;
		mw_cmd.cfa_hdr_statis = (void *)stat.cfa_pre;
		mw_cmd.hist_statis = NULL;

		mw_cmd.rgb_data_valid = stat.rgb_data_valid;
		mw_cmd.cfa_data_valid = stat.cfa_data_valid;
		mw_cmd.cfa_hdr_valid = stat.cfa_pre_hdr_data_valid;
		mw_cmd.hist_data_valid = NULL;

		mw_cmd.dsp_pts_data_addr = stat.dsp_pts_data_addr;
		mw_cmd.mono_pts_data_addr = stat.mono_pts_data_addr;
	}

	do {
		if (test_get_satistics_flag) {
			struct iav_system_resource system_resource;
			memset(&system_resource, 0, sizeof(system_resource));
			system_resource.encode_mode = DSP_CURRENT_MODE;
			if (ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &system_resource) < 0) {
				printf("IAV_IOC_GET_SYSTEM_RESOURCE error\n");
				retv = -1;
				break;
			}
			G_mw_info.res.hdr_expo_num = system_resource.exposure_num;
			G_mw_info.res.hdr_mode = system_resource.hdr_type;
			G_mw_info.res.isp_pipeline = system_resource.iso_type;

			retv = ioctl(fd_iav, IAV_IOC_IMG_GET_STATISTICS, &mw_cmd);
			if (retv < 0) {
				printf("error: IAV_IOC_IMG_GET_STATISTICS error\n");
				retv = -1;
				break;
			}
		} else {
			if (mw_get_sys_info(&G_mw_info) < 0) {
				printf("mw_get_sensor_hdr_expo error\n");
				return -1;
			}
			if(amba_img_dsp_3a_get_aaa_stat(fd_iav, &ik_mode, &g_stat)<0) {
				perror("amba_img_dsp_3a_get_aaa_stat");
				retv = -1;
				break;
			}
		}

		if(parse_aaa_data(&g_stat, G_mw_info.res.hdr_mode,
			hdr_statis_gp, &act_tile) < 0) {
			perror("parse_aaa_data");
			retv = -1;
			break;
		}
		parse_hist_data(&histo_data, hdr_statis_gp, G_mw_info.res.hdr_expo_num);

		switch (type) {
		case STATIS_AWB:
			for (i = 0; i < G_mw_info.res.hdr_expo_num; i++) {
				retv = display_AWB_data(hdr_statis_gp[i].awb_info,
					hdr_statis_gp[i].tile_info.awb_tile_num_col,
					hdr_statis_gp[i].tile_info.awb_tile_num_row);
			}
			printf("DSP PTS: %d, MONO PTS: %lld\n",
				*((u32 *)g_stat.DspPtsDataAddr), *((u64 *)g_stat.MonoPtsDataAddr));
			break;
		case STATIS_AE:
			for (i = 0; i < G_mw_info.res.hdr_expo_num; i++) {
				retv = display_AE_data(hdr_statis_gp[i].ae_info,
					hdr_statis_gp[i].tile_info.ae_tile_num_col,
					hdr_statis_gp[i].tile_info.ae_tile_num_row);
			}
			printf("DSP PTS: %d, MONO PTS: %lld\n",
				*((u32 *)g_stat.DspPtsDataAddr), *((u64 *)g_stat.MonoPtsDataAddr));
			break;
		case STATIS_AF:
			for (i = 0; i < G_mw_info.res.hdr_expo_num; i++) {
				retv = display_AF_data(hdr_statis_gp[i].af_info,
					hdr_statis_gp[i].tile_info.af_tile_num_col,
					hdr_statis_gp[i].tile_info.af_tile_num_row);
			}
			printf("DSP PTS: %d, MONO PTS: %lld\n",
				*((u32 *)g_stat.DspPtsDataAddr), *((u64 *)g_stat.MonoPtsDataAddr));
			break;
		case STATIS_HIST:
			retv = display_hist_data(&histo_data.cfa_histogram,
				&histo_data.rgb_histogram);
			printf("DSP PTS: %d, MONO PTS: %lld\n",
				*((u32 *)g_stat.DspPtsDataAddr), *((u64 *)g_stat.MonoPtsDataAddr));
			break;
		default:
			printf("Invalid statistics type !\n");
			retv = -1;
			break;
		}

	} while (--times);

	deinit_for_get_statistics(&stat);
	return retv;
}

static int statistics_data_setting(void)
{
	int exit_flag, error_opt;
	char ch;

	show_statistics_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'b':
			get_statistics_data(STATIS_AWB);
			break;
		case 'e':
			get_statistics_data(STATIS_AE);
			break;
		case 'f':
			get_statistics_data(STATIS_AF);
			break;
		case 'h':
			get_statistics_data(STATIS_HIST);
			break;
		case 'q':
			exit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (exit_flag)
			break;
		if (error_opt == 0) {
			show_statistics_menu();
		}
		ch = getchar();
	}

	return 0;
}

static int show_menu(void)
{
	printf("\n================================================\n");
//	printf("  g -- Global settings (3A library control, sensor fps, read 3A info)\n");
	printf("  e -- Exposure control settings (AE lines, shutter, gain, DC-iris, metering)\n");
	printf("  w -- White balance settings (AWB control, WB mode, RGB gain)\n");
	printf("  a -- Adjustment settings (Saturation, brightness, contrast, ...)\n");
	printf("  s -- Get statistics data (AE, AWB, AF, Histogram)\n");
	printf("  h -- Enhancement settings (3D de-noise, local exposure, backlight)\n");
	printf("  m -- Misc settings (Log level)\n");
	printf("  x -- Save cfg in %s\n", DEFAULT_CFG_FILE);
	printf("  q -- Quit");
	printf("\n================================================\n\n");
	printf("> ");
	return 0;
}

static void sigstop(int signo)
{
	if (!load_mctf_flag) {
		mw_stop_aaa();
	}
	close(fd_iav);
	exit(1);
}

#define NO_ARG	0
#define HAS_ARG	1

static const char* short_options = "ad:e:i:lt:m:f:L:p:";
static struct option long_options[] = {
	{"auto-dump", NO_ARG, 0, 'a'},
	{"mode", HAS_ARG, 0, 'i'},
	{"load", NO_ARG, 0, 'l'},
	{"aeb", HAS_ARG, 0, 'e'},
	{"adj", HAS_ARG, 0, 'd'},
	{"piris", HAS_ARG, 0, 'p'},
	{"lens", HAS_ARG, 0, 'L'},

	{"test", HAS_ARG, 0, 't'},
	{"log-level", HAS_ARG, 0, 'm'},
	{"file", HAS_ARG, 0, 'f'},

	{0, 0, 0, 0},
};
struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"", "\tauto dump idsp cfg file into /tmp directory when stop AAA under mode 4 in fastboot"},
	{"0|1|2", "0: background mode, 1:interactive mode, 2: auto test start/stop 3A mode"},
	{"", "\tload filter binary"},
	{"", "\tload aeb binary file for 3A params"},
	{"", "\tload adj binary file for 3A params"},
	{"", "\tload piris params binary file for 3A params"},
	{"", "\tconfig lens id (0: fixed lens; 2: p-iris M13VP288IR, 10: p-iris MZ128BP2810ICR)"},
	{"0", "\ttest option. 0: fetch 3A statistics continuously."},
	{"0~2", "set log level"},
	{"", "\tload config file for mw default vaule"},
	{0, 0},
};

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	int i;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'i':
			work_mode = atoi(optarg);
			if ((work_mode < WORK_MODE_FIRST)
				|| (work_mode >= WORK_MODE_LAST)) {
				printf("work mode must be in [%d~%d] \n",
					WORK_MODE_FIRST, WORK_MODE_LAST);
				return -1;
			}
			break;
		case 'l':
			load_mctf_flag = 1;
			break;
		case 'e':
			strncpy(G_file_name[FILE_TYPE_AEB], optarg,
				sizeof(G_file_name[FILE_TYPE_AEB]));
			G_file_name[FILE_TYPE_AEB][sizeof(G_file_name[FILE_TYPE_AEB]) - 1] = '\0';
			load_file_flag[FILE_TYPE_AEB] = 1;
			break;
		case 'd':
			strncpy(G_file_name[FILE_TYPE_ADJ], optarg,
				sizeof(G_file_name[FILE_TYPE_ADJ]));
			G_file_name[FILE_TYPE_ADJ][sizeof(G_file_name[FILE_TYPE_ADJ]) - 1] = '\0';
			load_file_flag[FILE_TYPE_ADJ] = 1;
			break;
		case 'p':
			strncpy(G_file_name[FILE_TYPE_PIRIS], optarg,
				sizeof(G_file_name[FILE_TYPE_PIRIS]));
			G_file_name[FILE_TYPE_PIRIS][sizeof(G_file_name[FILE_TYPE_PIRIS]) - 1] = '\0';
			load_file_flag[FILE_TYPE_PIRIS] = 1;
			break;
		case 'L':
			i = atoi(optarg);
			if ((i == LENS_M13VP288IR_ID) || (i == LENS_MZ128BP2810ICR_ID) ||
				(i == LENS_CMOUNT_ID)) {
				G_lens_id = i;
			} else {
				printf("No support\n");
				return -1;
			}
			break;
		case 't':
			test_option = !!atoi(optarg);
			test_flag = 1;
			break;
		case 'm':
			i = atoi(optarg);
			if (i < MW_ERROR_LEVEL || i >= (MW_LOG_LEVEL_NUM - 1)) {
				printf("Invalid log level, please set it in the range of (0~%d).\n",
					(MW_LOG_LEVEL_NUM - 1));
				return -1;
			}
			G_log_level = i;
			break;
		case 'f':
			strncpy(G_file_name[FILE_TYPE_CONFIG], optarg,
				sizeof(G_file_name[FILE_TYPE_CONFIG]));
			G_file_name[FILE_TYPE_CONFIG][sizeof(G_file_name[FILE_TYPE_CONFIG]) - 1] = '\0';
			load_file_flag[FILE_TYPE_CONFIG] = 1;
			break;
		case 'a':
			auto_dump_cfg_flag = 1;
			break;
		default:
			printf("unknown option %c\n", ch);
			return -1;
		}
	}
	return 0;
}

void usage(void){
	int i;

	printf("test_image usage:\n");
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
	printf("\n");
	printf("Example:\n");
	printf("0. run without interactive menu\n");
	printf("\ttest_image -i 0\n");
	printf("1. run with interactive menu\n");
	printf("\ttest_image -i 1\n");
	printf("2. run with test mode: auto test start/stop 3A\n");
	printf("\ttest_image -i 2\n");
	printf("3. only load filter binary...\n");
	printf("\ttest_image -l\n");
	printf("4. fetch 3A statistics data in a standalone process\n");
	printf("   for parse data, still need idsp is working (working in another process)\n");
	printf("\ttest_image -t 0\n");
}

int run_interactive_mode()
{
	char ch, error_opt;
	int quit_flag, imgproc_running_flag = 1;
	show_menu();
	ch = getchar();
	while (ch) {
		quit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'g':
			global_setting(&imgproc_running_flag);
			break;
		case 'e':
			exposure_setting();
			break;
		case 'w':
			white_balance_setting();
			break;
		case 'a':
			adjustment_setting();
			break;
		case 'h':
			enhancement_setting();
			break;
		case 's':
			statistics_data_setting();
			break;
		case 'm':
			misc_setting();
			break;
		case 'q':
			quit_flag = 1;
			break;
		case 'x':
			if (mw_save_config_file(image_save_file) < 0) {
				printf("Save image cfg in to: %s fail.\n", image_save_file);
				break;
			}
			printf("Save image cfg in to: %s success.\n", image_save_file);
			break;
		default:
			error_opt = 1;
			break;
		}
		if (quit_flag) {
			break;
		}
		if (error_opt == 0) {
			show_menu();
		}
		ch = getchar();
	}
	if (mw_stop_aaa() < 0) {
		perror("mw_stop_aaa");
		return -1;
	}
	return 0;
}

int start_aaa_main()
{
	if (mw_start_aaa(fd_iav) < 0) {
		perror("mw_start_aaa");
		return -1;
	}
	/* Exposure Control Setting */
	if (mw_get_sys_info(&G_mw_info) < 0) {
		printf("mw_get_sensor_hdr_expo error\n");
		return -1;
	}
	if (G_mw_info.res.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
		printf("===Sensor is Normal mode\n");
	} else {
		printf("===Sensor is %dX HDR mode", G_mw_info.res.hdr_expo_num);
	}

	if (mw_is_dc_iris_supported()) {
		mw_enable_dc_iris_control(dc_iris_enable);
		mw_set_dc_iris_pid_coef(&pid_coef);
	}

	return 0;
}

static int run_test_options(u32 option)
{
	int rval = 0;

	switch (option) {
	case FETCH_AAA_STATISTICS:
		statistics_data_setting();
		break;
	default:
		printf("Invalid test option [%d].\n", option);
		rval = -1;
		break;
	}

	return rval;
}

int main(int argc, char ** argv)
{
	signal(SIGINT, sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);
	int i = 0;
	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0)
		return -1;

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("open /dev/iav");
		return -1;
	}

	if (mw_set_log_level(G_log_level) < 0) {
		perror("mw_set_log_level");
		return -1;
	}

	for (i = FILE_TYPE_FIRST; i < FILE_TYPE_LAST; i++) {
		if (load_file_flag[i]) {
			if (i == FILE_TYPE_CONFIG) {
				if(mw_load_config_file(G_file_name[i]) < 0) {
					printf("load file:%s fail.\n", G_file_name[i]);
					return -1;
				}
			} else if (mw_load_aaa_param_file(G_file_name[i], i) < 0)
				return -1;
		}
	}

	if (auto_dump_cfg_flag) {
		mw_enable_auto_dump_cfg(auto_dump_cfg_flag);
	}
	if (load_mctf_flag) {
		if (mw_init_mctf(fd_iav) < 0) {
			perror("mw_load_mctf");
			return -1;
		}
	} else if (test_flag) {
		if (run_test_options(test_option) < 0) {
			printf("run_test_options error!\n");
			return -1;
		}
	} else {
		if (load_file_flag[FILE_TYPE_PIRIS] && (G_lens_id != LENS_CMOUNT_ID)) {
			if (mw_set_lens_id(G_lens_id) < 0)
				return -1;
		}
		if (start_aaa_main() < 0 ) {
			perror("start_aaa_main");
			return -1;
		}
		switch (work_mode) {
			case BACKGROUND_MODE:
				while (1) {
					sleep(2);
				}
				break;
			case INTERACTIVE_MODE:
				if (run_interactive_mode() < 0) {
					printf("run_interactive_mode error\n");
					return -1;
				}
				close(fd_iav);
				break;
			case TEST_3A_LIB_MODE:
				while (1) {
					sleep(3);
					mw_stop_aaa();
					sleep(3);
					mw_start_aaa(fd_iav);
				}
				break;
			default:
				printf("Unknown option!\n");
				break;
		}
	}

	return 0;
}

