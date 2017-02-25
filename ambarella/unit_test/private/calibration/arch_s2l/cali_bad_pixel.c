/*
 * cali_bad_pixel.c
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
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <basetypes.h>

#include "iav_common.h"
#include "iav_ioctl.h"

#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"

#include "img_adv_struct_arch.h"
#include "img_api_adv_arch.h"
#include "iav_vin_ioctl.h"
#include "AmbaDSP_ImgFilter.h"

#define WDR_SENSOR_HIST_LINE	4
#define BAD_PIXEL_CALI_FILENAME			"bpcmap.bin"
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))

static const char *default_filename = BAD_PIXEL_CALI_FILENAME;
static char filename[256];

static int fd_iav;

static int detect_flag = 0;
static int correct_flag = 0;
static int restore_flag = 0;
static int mask_flag = 0;

static int g_set_agc_flag = 0;
static int g_set_sht_flag = 0;
static int g_agc_idx;
static int g_sht_idx;

static cali_badpix_setup_t badpixel_detect_algo;
static AMBA_DSP_IMG_BYPASS_SBP_INFO_s sbp_corr;

static int detect_param[10];
static int correct_param[2];
static int restore_param[2];

#define NO_ARG			0
#define HAS_ARG			1

static struct option long_options[] = {
	{"detect", HAS_ARG, 0, 'd'},
	{"correct", HAS_ARG, 0, 'c'},
	{"restore", HAS_ARG, 0, 'r'},
	{"filename", HAS_ARG, 0, 'f'},
	{"privacy-mask", NO_ARG, 0, 'm'},
  {"set-agc-index", HAS_ARG, 0, 'A'},
  {"set-sht-index", HAS_ARG, 0, 'S'},
	{0, 0, 0, 0}
};

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_options = "d:c:r:f:mA:S:";

static const struct hint_s hint[] = {
	{"", "\t\t10 params for bad pixel detect"},
	{"", "\t\t2 params for bad pixel correct"},
	{"", "\t\t2 params for bad pixel restore"},
	{"filename", "specify location for saving calibration result"},
	{"", "\t\ttry add a privacy mask"},
  {"", "\t\tset agc index"},
  {"", "\t\tset sht index"},
};

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
	int ch;
	int option_index = 0;
	opterr = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'd':
			detect_flag = 1;
			if (get_multi_arg(optarg, detect_param, ARRAY_SIZE(detect_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(detect_param), ch);
				return -1;
			}
			break;

		case 'c':
			correct_flag = 1;
			if (get_multi_arg(optarg, correct_param, ARRAY_SIZE(correct_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(correct_param), ch);
				return -1;
			}
			break;

		case 'r':
			restore_flag = 1;
			if (get_multi_arg(optarg, restore_param, ARRAY_SIZE(restore_param)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(restore_param), ch);
				return -1;
			}
			break;

		case 'f':
			strcpy(filename, optarg);
			break;

		case 'm':
			mask_flag = 1;
			break;
    case 'A':
      g_set_agc_flag = 1;
      if (get_multi_arg(optarg, &g_agc_idx, 1) <0) {
        printf("need %d args for opt %c!\n", 1, ch);
        return -1;
      }
      break;
    case 'S':
      g_set_sht_flag = 1;
      if (get_multi_arg(optarg, &g_sht_idx, 1) <0) {
        printf("need %d args for opt %c!\n", 1, ch);
        return -1;
      }
      break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}
	return 0;
}

int get_vin_info(u32 *width, u32 *height, sensor_config_t *config)
{
	int vin_mode;
	struct vindev_mode video_info;
	memset(&video_info, 0, sizeof(video_info));
	video_info.vsrc_id = 0;
	if(ioctl(fd_iav, IAV_IOC_VIN_GET_MODE, &video_info)) {
		perror("IAV_IOC_VIN_GET_MODE");
		return -1;
	}
	vin_mode = video_info.video_mode;

	switch (vin_mode) {
	case AMBA_VIDEO_MODE_720P:
		*width = 1280;
		*height = 720;
		break;
	case AMBA_VIDEO_MODE_WXGA:
		*width = 1280;
		*height = 800;
		break;
	case AMBA_VIDEO_MODE_1080P:
		*width = 1920;
		*height = 1080;
		break;
	case AMBA_VIDEO_MODE_XGA:
		*width = 1024;
		*height = 768;
		break;
	case AMBA_VIDEO_MODE_QSXGA:
		*width = 2592;
		*height = 1944;
		break;
	case AMBA_VIDEO_MODE_QXGA:
		*width = 2048;
		*height = 1536;
		break;
	case AMBA_VIDEO_MODE_3840_2160:
		*width = 3840;
		*height = 2160;
		break;
	case AMBA_VIDEO_MODE_4096_2160:
		*width = 4096;
		*height = 2160;
		break;
	case AMBA_VIDEO_MODE_4000_3000:
		*width = 4000;
		*height = 3000;
		break;
	case AMBA_VIDEO_MODE_3072_2048:
		*width = 3072;
		*height = 2048;
		break;
	case AMBA_VIDEO_MODE_2304_1536:
		*width = 2304;
		*height = 1536;
		break;
	case AMBA_VIDEO_MODE_2304_1296:
		*width = 2304;
		*height = 1296;
		break;
	case AMBA_VIDEO_MODE_2688_1512:
		*width = 2688;
		*height = 1512;
		break;
	default:
		printf("unrecogized vin mode %d\n", vin_mode);
		return -1;
	}

	config->p_vindev_aaa_info->vsrc_id = 0;
	if (ioctl(fd_iav, IAV_IOC_VIN_GET_AAAINFO, config->p_vindev_aaa_info) < 0) {
		perror("IAV_IOC_VIN_GET_AAAINFO error\n");
		return -1;
	}
	config->max_gain_db = 48;	// only suitible for all sensors. the max 48dB.
	config->shutter_delay = 2;	// only suitible for all sensors. delay 2 frames.
	config->agc_delay = 2;	// only suitible for all sensors. delay 2 frames.
	return 0;
}

int main(int argc, char **argv)
{
	u32					cap_width;
	u32					cap_height;
	int 					fd_bpc = -1;
	u8* 					fpn_map_addr = NULL;
	u32					fpn_map_size = 0;
	u32 					bpc_num = 0;
	u32 					raw_pitch;
	u32 					width = 0;
	u32 					height = 0;
	int					count = -1;
	static amba_img_dsp_cfa_noise_filter_t	cfa_noise_filter;
	static amba_img_dsp_anti_aliasing_t        AntiAliasing;
	struct iav_querydesc raw_info;
	struct iav_rawbufdesc *p_rawbuf_desc = NULL;
	memset(&cfa_noise_filter, 0, sizeof(cfa_noise_filter));
	memset(&AntiAliasing, 0, sizeof(AntiAliasing));

	static amba_img_dsp_dbp_correction_t bad_corr;
	bad_corr.DarkPixelStrength = 0;
	bad_corr.HotPixelStrength = 0;
	bad_corr.Enb = 0;

	static amba_img_dsp_mode_cfg_t dsp_mode_cfg;
	memset(&dsp_mode_cfg, 0, sizeof(dsp_mode_cfg));
	dsp_mode_cfg.Pipe = AMBA_DSP_IMG_PIPE_VIDEO;
	dsp_mode_cfg.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_FAST;

	sensor_config_t sensor_config_info;
	struct vindev_aaa_info vindev_aaa_info;

	memset(&sensor_config_info, 0, sizeof(sensor_config_info));
	memset(&vindev_aaa_info, 0, sizeof(vindev_aaa_info));

	sensor_config_info.p_vindev_aaa_info = &vindev_aaa_info;

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

	if (g_set_agc_flag) {
		if (img_set_sensor_agc(fd_iav, g_agc_idx) < 0) {
			printf("failed to set agc index\n");
		}
		return 0;
	}

	if (g_set_sht_flag) {
		if (img_set_sensor_shutter(fd_iav, g_sht_idx) < 0) {
			printf("failed to set sht index\n");
		}
		return 0;
	}

	if(img_lib_init(fd_iav, 0, 0)<0) {
		perror("img_lib_init failed");
		return -1;
	}

	if (filename[0] == '\0')
		strcpy(filename, default_filename);

	if (get_vin_info(&cap_width, &cap_height, &sensor_config_info) < 0) {
		printf("get_vin_info failed\n");
		return -1;
	}
	if (img_config_sensor_info(&sensor_config_info) < 0) {
		return -1;
	}

	memset(&raw_info, 0, sizeof(raw_info));
	raw_info.qid = IAV_DESC_RAW;
	if (ioctl(fd_iav, IAV_IOC_QUERY_DESC, &raw_info) < 0) {
		if (errno == EINTR) {
			// skip to do nothing
		} else {
			perror("IAV_IOC_QUERY_DESC");
			return -1;
		}
	}
	p_rawbuf_desc = &raw_info.arg.raw;

	if (detect_flag) {
		width = p_rawbuf_desc->width;
		height = p_rawbuf_desc->height;
	} else if (correct_flag) {
		width = p_rawbuf_desc->width;
		height = p_rawbuf_desc->height;
	} else if (restore_flag) {
		width = p_rawbuf_desc->width;
		height = p_rawbuf_desc->height;
	} else {
		width = p_rawbuf_desc->width;
		height = p_rawbuf_desc->height;
	}

	if (width != cap_width || (height != cap_height &&
		(height != (cap_height + WDR_SENSOR_HIST_LINE)))) {
		printf("badpixel map size %dx%d doesn't match sensor capture window %dx%d!",
			width, height, cap_width, cap_height);
		return -1;
	}

	raw_pitch = ROUND_UP(width/8, 32);
	fpn_map_size = raw_pitch*height;
	fpn_map_addr = malloc(fpn_map_size);
	if (fpn_map_addr < 0) {
		printf("can not malloc memory for bpc map\n");
		return -1;
	}
	memset(fpn_map_addr,0,(fpn_map_size));

	if (detect_flag) {
		// disable dynamic bad pixel correction
		amba_img_dsp_set_cfa_noise_filter(fd_iav, &dsp_mode_cfg, &cfa_noise_filter);
		amba_img_dsp_set_dynamic_bad_pixel_correction(fd_iav, &dsp_mode_cfg, &bad_corr);
		amba_img_dsp_set_anti_aliasing(fd_iav, &dsp_mode_cfg, &AntiAliasing);

		badpixel_detect_algo.cap_width = width;
		badpixel_detect_algo.cap_height= height;
		badpixel_detect_algo.block_w= detect_param[2];
		badpixel_detect_algo.block_h = detect_param[3];
		badpixel_detect_algo.badpix_type = detect_param[4];
		badpixel_detect_algo.upper_thres= detect_param[5];
		badpixel_detect_algo.lower_thres= detect_param[6];
		badpixel_detect_algo.detect_times = detect_param[7];
		badpixel_detect_algo.agc_idx = detect_param[8];
		badpixel_detect_algo.shutter_idx= detect_param[9];
		badpixel_detect_algo.badpixmap_buf = fpn_map_addr;
		badpixel_detect_algo.cali_mode = 0; // 0 for video, 1 for still

		bpc_num = img_cali_bad_pixel(fd_iav, &badpixel_detect_algo);
		printf("Total number is %d\n", bpc_num);

		if((fd_bpc = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
			printf("map file open error!\n");
		}
		write(fd_bpc, fpn_map_addr, fpn_map_size);
		free(fpn_map_addr);
	}

	if (correct_flag || restore_flag) {
		// disable dynamic bad pixel correction
		amba_img_dsp_set_cfa_noise_filter(fd_iav, &dsp_mode_cfg, &cfa_noise_filter);
		amba_img_dsp_set_dynamic_bad_pixel_correction(fd_iav, &dsp_mode_cfg, &bad_corr);
		amba_img_dsp_set_anti_aliasing(fd_iav, &dsp_mode_cfg, &AntiAliasing);

		if (correct_flag) {
			if((fd_bpc = open(filename, O_RDONLY, 0)) < 0) {
				printf("bpcmap.bin cannot be opened\n");
				return -1;
			}
			if((count = read(fd_bpc, fpn_map_addr, fpn_map_size)) != fpn_map_size) {
				printf("read bpcmap.bin error\n");
				return -1;
			}
			sbp_corr.Enable = 3;
		} else if (restore_flag) {
			sbp_corr.Enable = 0;
		}

		sbp_corr.PixelMapPitch = raw_pitch;
		sbp_corr.PixelMapHeight = height;
		sbp_corr.PixelMapWidth = width;
		sbp_corr.pMap = fpn_map_addr;
		//TODO: pending for fix
		amba_img_dsp_set_static_bad_pixel_correction_bypass(fd_iav, &dsp_mode_cfg, &sbp_corr);
		free(fpn_map_addr);
	}

	if (mask_flag) {	//try add a area of privacy mask
		int i;
		memset(fpn_map_addr,0,(fpn_map_size));
		for (i = raw_pitch*400; i < raw_pitch*600; i++) {
			if ((i % raw_pitch) > 100 && (i % raw_pitch) < 150)
				fpn_map_addr[i] = 0xff;
		}
		sbp_corr.Enable = 3;
		sbp_corr.PixelMapPitch = raw_pitch;
		sbp_corr.PixelMapHeight = height;
		sbp_corr.PixelMapWidth = width;
		sbp_corr.pMap = fpn_map_addr;
		//TODO: pending for fix
		amba_img_dsp_set_static_bad_pixel_correction_bypass(fd_iav, &dsp_mode_cfg, &sbp_corr);
		free(fpn_map_addr);
	}

	return 0;
}

