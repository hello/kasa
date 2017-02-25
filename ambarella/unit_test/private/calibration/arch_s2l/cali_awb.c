/*
 * cali_awb.c
 *
 * Histroy:
 *   Dec 5, 2013 - [Shupeng Ren] created file
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
#include <assert.h>
#include <getopt.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <basetypes.h>

#include "iav_common.h"

#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"

#include "img_adv_struct_arch.h"
#include "img_api_adv_arch.h"
#include "iav_vin_ioctl.h"
#include "AmbaDSP_ImgFilter.h"

#include "iav_ioctl.h"

#include <pthread.h>
#include "idsp_netlink.h"
#include "load_param.c"

#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
pthread_t nl_thread;
static img_config_info_t img_config;
static amba_img_dsp_mode_cfg_t ik_mode;

#define	IMGPROC_PARAM_PATH		"/etc/idsp"

#ifndef	ARRAY_SIZE
#define	ARRAY_SIZE(x)	(sizeof(x)/sizeof((x)[0]))
#endif

#ifndef ABS
#define ABS(a)	(((a) < 0) ? -(a) : (a))
#endif

#ifndef	MAX
#define MAX(a, b)	(((a) < (b)) ? (a) : (b))
#endif

#define CALI_AWB_FILENAME	"cali_awb.txt"

static const char *default_filename = CALI_AWB_FILENAME;
static char cali_awb_filename[256];
static char load_awb_filename[256];

static int fd_iav;
static int detect_flag = 0;
static int correct_flag = 0;
static int restore_flag = 0;
static int save_file_flag = 0;

static wb_gain_t wb_gain;
static int correct_param[12];

#define NO_ARG	0
#define HAS_ARG	1

static struct option long_options[] = {
	{"detect",   NO_ARG,  0, 'd'},
	{"correct",  HAS_ARG, 0, 'c'},
	{"restore",  NO_ARG, 0, 'r'},
	{"savefile", HAS_ARG, 0, 'f'},
	{"loadfile", HAS_ARG, 0, 'l'},
	{0, 0, 0, 0}
};

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_options = "dc:rf:l:";

static struct hint_s hint[] = {
	{"", "\t\tGet current AWB gain"},
	{"[v1,v2,...,v12]", "\t\torignal AWB gain in R/G/Bin low/high light, target WB gain in R/G/B in low/high light "},
	{"", "\t\tRestore AWB as default"},
	{"", "\t\tSpecify the file to save current AWB gain"},
	{"", "\t\tload the file for correction"},
};

static void sigstop(int signo)
{
	if (send_image_msg_exit() < 0) {
		perror("send_image_msg_stop_aaa\n");
		exit(-1);
	}
	pthread_join(nl_thread, NULL);
	nl_thread = -1;

	close(fd_iav);
	printf("Quit cali_wb.\n");
	exit(1);
}

static void usage(void)
{
	int i;
	printf("\ncali_app usage:\n");
	for (i = 0; i < ARRAY_SIZE(long_options) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}

	printf("\n\n");
	printf("The white balance calibration is defined as a gain correction for\n");
	printf("red, green and blue components by gain factors.\n");
	printf("You can use -d option with the perfect sensor and light to get the\n");
	printf("target AWB gain factors (1024 as a unit) for your reference.\n");
	printf("When applying calibration for imperfect sensor, specify the wrong\n");
	printf("gain and the target gain to do calibration by -c option.\n\n");
}

static int get_multi_arg(char *optarg, int *argarr, int argcnt)
{
	int i;
	char *delim = ",:-";
	char *ptr;

	ptr = strtok(optarg, delim);
	argarr[0] = atoi(ptr);

	for (i = 1; i < argcnt; i++) {
		ptr = strtok(NULL, delim);
		if (ptr == NULL)
			break;
		argarr[i] = atoi(ptr);
	}
	if (i < argcnt)
		return -1;
	return 0;
}

static int init_param(int argc, char **argv)
{
	int ch, i = 0, data[3];
	int option_index = 0;
	FILE *fp;
	char s[256];
	opterr = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'd':
			detect_flag = 1;
			break;
		case 'c':
			correct_flag = 1;
			if (get_multi_arg(optarg, correct_param, ARRAY_SIZE(correct_param)) < 0) {
				printf("Need %d args for opt %c !\n", ARRAY_SIZE(correct_param), ch);
				return -1;
			}
			while (i < ARRAY_SIZE(correct_param)) {
				if (correct_param[i] <= 0) {
					printf("args should be greater than 0!\n");
					return -1;
				}
				i++;
			}
			break;
		case 'r':
			restore_flag = 1;
			break;
		case 'f':
			save_file_flag = 1;
			strcpy(cali_awb_filename, optarg);
			break;
		case 'l':
			correct_flag = 1;
			strcpy(load_awb_filename, optarg);
			if (strlen(load_awb_filename) == 0) {
				printf("Please input the file path used for correction. It is generated by awb_calibration.sh.\n");
				return -1;
			}
			if ((fp = fopen(load_awb_filename, "r+")) == NULL) {
				printf("Open file %s error.\n", load_awb_filename);
				return -1;
			}
			i = 0;
			while (NULL != fgets(s, sizeof(s), fp) && (i < 4)) {
				if (get_multi_arg(s, data, 3) < 0) {
					printf("wrong data format in %s.\n", load_awb_filename);
					fclose(fp);
					return -1;
				}
				if (data[0] <= 0 || data[1] <= 0 || data[2] <= 0) {
					printf("Data in %s should be greater than 0!\n", load_awb_filename);
					fclose(fp);
					return -1;
				}
				correct_param[i*3] = data[0];
				correct_param[i*3+1] = data[1];
				correct_param[i*3+2] = data[2];
				i++;
			}
			fclose(fp);
			if (i < 3) {
				printf("Incomplete data in %s.\n", load_awb_filename);
				return -1;
			}
			break;
		default:
			printf("Unknown option found: %c\n", ch);
			return -1;
		}
	}
	return 0;
}

static int init_ik_mode(img_config_info_t *p_img_cfg,
	amba_img_dsp_mode_cfg_t* p_ik_mode, struct iav_enc_mode_cap *cap)
{
	amba_img_dsp_variable_range_t dsp_variable_range;

	memset(p_ik_mode, 0, sizeof(amba_img_dsp_mode_cfg_t));

	p_ik_mode->Pipe = AMBA_DSP_IMG_PIPE_VIDEO;
	if (p_img_cfg->isp_pipeline ==ISP_PIPELINE_B_LISO) {
		p_ik_mode->AlgoMode = AMBA_DSP_IMG_ALGO_MODE_HISO;
	       p_ik_mode->BatchId = 0xff;
	} else if (p_img_cfg->isp_pipeline ==ISP_PIPELINE_LISO) {
		p_ik_mode->AlgoMode = AMBA_DSP_IMG_ALGO_MODE_FAST;
	} else if (p_img_cfg->isp_pipeline ==ISP_PIPELINE_ADV_LISO||
		p_img_cfg->isp_pipeline ==ISP_PIPELINE_MID_LISO) {
		p_ik_mode->AlgoMode = AMBA_DSP_IMG_ALGO_MODE_LISO;
		p_ik_mode->BatchId = 0xff;
	}

	dsp_variable_range.max_chroma_radius = (1 << (5 + cap->max_chroma_radius));
	dsp_variable_range.max_wide_chroma_radius = (1 << (5 + cap->max_wide_chroma_radius));
	dsp_variable_range.inside_fpn_flag = 0;
	dsp_variable_range.wide_chroma_noise_filter_enable = cap->wcr_possible;
	amba_img_dsp_set_variable_range(p_ik_mode, &dsp_variable_range);

	return 0;
}

int check_iav_work(void)
{
	u32 state;

	memset(&state, 0, sizeof(state));
	if (ioctl(fd_iav, IAV_IOC_GET_IAV_STATE, &state) < 0) {
		perror("IAV_IOC_GET_IAV_STATE");
		return -1;
	}
	if (state == IAV_STATE_PREVIEW || state == IAV_STATE_ENCODING) {
		return 1;
	}

	return 0;
}

int do_init_netlink(void)
{
	init_netlink();
	pthread_create(&nl_thread, NULL, (void *)netlink_loop, (void *)NULL);

	while (1) {
		if (check_iav_work() > 0) {
			break;
		}
		usleep(10000);
	}

	return 0;
}


int do_prepare_aaa(void)
{
	#define	PIXEL_IN_MB			(16)
	struct vindev_video_info video_info;
	struct iav_enc_mode_cap mode_cap;
	struct iav_system_resource system_resource;
	struct iav_srcbuf_setup	srcbuf_setup;
	struct vindev_aaa_info vin_aaa_info;
	int sensor_id = 0x0;
	img_config_info_t* p_img_config = &img_config;

	memset(p_img_config, 0, sizeof(img_config_info_t));

	// video info
	memset(&video_info, 0, sizeof(video_info));
	video_info.vsrc_id = 0;
	video_info.info.mode = AMBA_VIDEO_MODE_CURRENT;
	if (ioctl(fd_iav, IAV_IOC_VIN_GET_VIDEOINFO, &video_info) < 0) {
		perror("IAV_IOC_VIN_GET_VIDEOINFO");
		return 0;
	}
	p_img_config->raw_width = ROUND_UP(video_info.info.width, PIXEL_IN_MB);
	p_img_config->raw_height = ROUND_UP(video_info.info.height, PIXEL_IN_MB);
	p_img_config->raw_resolution =video_info.info.bits;

	// encode mode capability
	memset(&mode_cap, 0, sizeof(mode_cap));
	mode_cap.encode_mode = DSP_CURRENT_MODE;
	if (ioctl(fd_iav, IAV_IOC_QUERY_ENC_MODE_CAP, &mode_cap)) {
		perror("IAV_IOC_QUERY_ENC_MODE_CAP");
		return -1;
	}

	// system resource
	memset(&system_resource, 0, sizeof(system_resource));
	system_resource.encode_mode = DSP_CURRENT_MODE;
	if (ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &system_resource) < 0) {
		perror("IAV_IOC_GET_SYSTEM_RESOURCE\n");
		return -1;
	}

	p_img_config->hdr_config.expo_num = system_resource.exposure_num;
	p_img_config->hdr_config.pipeline = system_resource.hdr_type;
	p_img_config->isp_pipeline = system_resource.iso_type;
	p_img_config->raw_pitch =system_resource.raw_pitch_in_bytes;

	// source buffer
	memset(&srcbuf_setup, 0, sizeof(srcbuf_setup));
	if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP, &srcbuf_setup) < 0) {
		printf("IAV_IOC_GET_SOURCE_BUFFER_SETUP error\n");
		return -1;
	}

	p_img_config->main_width = ROUND_UP(
		srcbuf_setup.size[IAV_SRCBUF_MN].width, PIXEL_IN_MB);
	p_img_config->main_height = ROUND_UP(
		srcbuf_setup.size[IAV_SRCBUF_MN].height, PIXEL_IN_MB);

	// vin aaa info
	memset(&vin_aaa_info, 0, sizeof(vin_aaa_info));
	vin_aaa_info.vsrc_id = 0;
	if (ioctl(fd_iav, IAV_IOC_VIN_GET_AAAINFO, &vin_aaa_info) < 0) {
		perror("IAV_IOC_VIN_GET_AAAINFO error\n");
		return -1;
	}

	p_img_config->raw_bayer = vin_aaa_info.bayer_pattern;
	sensor_id = vin_aaa_info.sensor_id;
	if(vin_aaa_info.dual_gain_mode){
		p_img_config->hdr_config.method = HDR_DUAL_GAIN_METHOD;
	}else{
		switch (vin_aaa_info.hdr_mode){
			case AMBA_VIDEO_LINEAR_MODE:
				p_img_config->hdr_config.method = HDR_NONE_METHOD;
				break;
			case AMBA_VIDEO_2X_HDR_MODE:
			case AMBA_VIDEO_3X_HDR_MODE:
			case AMBA_VIDEO_4X_HDR_MODE:
				if(sensor_id == SENSOR_AR0230 ){
					p_img_config->hdr_config.method = HDR_RATIO_FIX_LINE_METHOD;
				}else{
					p_img_config->hdr_config.method = HDR_RATIO_VARY_LINE_METHOD;
				}
				break;
			case AMBA_VIDEO_INT_HDR_MODE:
				p_img_config->hdr_config.method = HDR_BUILD_IN_METHOD;
				break;
			default:
				printf("error: invalid vin HDR sensor info.\n");
				return -1;
		}
	}

	img_lib_init(fd_iav, p_img_config->defblc_enable, p_img_config->sharpen_b_enable);
	img_config_pipeline(fd_iav, p_img_config);
	init_ik_mode(p_img_config, &ik_mode, &mode_cap);
	if(binding_param_with_sensor(fd_iav, &vin_aaa_info, p_img_config, &ik_mode) < 0)
		return -1;
	if(img_prepare_isp(fd_iav) < 0) {
		printf("prepare isp fail\n");
		return -1;
	}
	return 0;
}


int do_start_aaa(void)
{
	if (img_start_aaa(fd_iav) < 0) {
		printf("start_aaa error!\n");
		return -1;
	}
	if (img_set_work_mode(0)) {
		printf("img_set_work_mode error!\n");
		return -1;
	}
	return 0;
}

int do_stop_aaa(void)
{
	if (img_stop_aaa() < 0) {
		printf("stop aaa error!\n");
		return -1;
	}
	usleep(1000);
	if (img_lib_deinit() < 0) {
		printf("img_lib_deinit error!\n");
		return -1;
	}
	return 0;
}

int awb_cali_get_current_gain(void)
{
	awb_work_method_t m = AWB_MANUAL;
	if (img_set_awb_method(&m) < 0) {
		printf("img_set_awb_method error!\n");
		return -1;
	}
	printf("\nWait to get current White Balance Gain...\n");
	sleep(2);

	img_get_awb_manual_gain(&wb_gain);
	printf("Current Red Gain %d, Green Gain %d, Blue Gain %d.\n",
		wb_gain.r_gain, wb_gain.g_gain, wb_gain.b_gain);

	m = AWB_NORMAL;
	if (img_set_awb_method(&m) < 0) {
		printf("img_set_awb_method error!\n");
		return -1;
	}

	return 0;
}

int awb_cali_correct_gain(void)
{
	wb_gain_t orig[2], target[2];
	u16 thre_r, thre_b;
	int low_r, low_b, high_r, high_b;
	int i;

	for (i = 0; i < 2; i++) {
		orig[i].r_gain = correct_param[i*3];
		orig[i].g_gain = correct_param[i*3+1];
		orig[i].b_gain = correct_param[i*3+2];
	}

	for (i = 0; i < 2; i++) {
		target[i].r_gain = correct_param[i*3+6];
		target[i].g_gain = correct_param[i*3+7];
		target[i].b_gain = correct_param[i*3+8];
	}

	low_r = target[0].r_gain - orig[0].r_gain;
	low_r = ABS(low_r);
	high_r = target[1].r_gain - orig[1].r_gain;
	high_r = ABS(high_r);
	low_b = target[0].b_gain - orig[0].b_gain;
	low_b = ABS(low_b);
	high_b = target[1].b_gain-orig[1].b_gain;
	high_b = ABS(high_b);

	thre_r = (u16) MAX(low_r, high_r);
	thre_b = (u16) MAX(low_b, high_b);

	if (img_set_awb_cali_thre(&thre_r, &thre_b) < 0) {
		printf("img_set_awb_cali_thre error!\n");
		return -1;
	}
	if (img_set_awb_cali_shift(orig, target) < 0) {
		printf("img_set_awb_cali_shift error!\n");
		return -1;
	}

	return 0;
}


int main(int argc, char **argv)
{
	FILE *fp;

	signal(SIGINT,  sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0)
		return -1;
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}
	if (do_init_netlink() < 0)
		return -1;

	sleep(1);
	if (detect_flag) {
		if (awb_cali_get_current_gain() < 0)
			return -1;
		if (save_file_flag) {
			if ((fp = fopen(cali_awb_filename, "a+")) == NULL) {
				printf("Open file %s error. Save the result to default file %s.\n", cali_awb_filename, default_filename);

				if ((fp = fopen(default_filename, "a+")) == NULL) {
					printf("Save failed.\n");
				}
			}
			if (fp != NULL) {
				fprintf(fp, "%d:%d:%d\n", wb_gain.r_gain, wb_gain.g_gain, wb_gain.b_gain);
				fclose(fp);
			}
		}
	}

	if (correct_flag) {
		if (awb_cali_correct_gain() < 0)
			return -1;
	}



	if (correct_flag || restore_flag)
		while (1)
			sleep(10);

	return 0;
}


