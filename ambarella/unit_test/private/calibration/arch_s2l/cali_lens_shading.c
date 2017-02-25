/*
 * cali_lens_shading.c
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
#include <sys/mman.h>

#include "iav_common.h"
#include "iav_ioctl.h"

#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"

#include "img_adv_struct_arch.h"
#include "img_api_adv_arch.h"
#include "iav_vin_ioctl.h"
#include "AmbaDSP_ImgFilter.h"

#include "mw_struct.h"
#include "mw_api.h"

#define WDR_SENSOR_HIST_LINE	4
#define ABS(x)  ((x) > 0 ? (x) : -(x))
#define Q9_TIME(t) ((uint32_t) ((t)*512000000))
#define Q9_DELTA_TIME ((uint32_t) (0.0001*512000000))
#define LENS_SHADING_CALI_FILENAME "lens_shading.bin"
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

static const char *default_filename = LENS_SHADING_CALI_FILENAME;
static char filename[256];

static int fd_iav;

static int detect_flag = 0;
static int correct_flag = 0;
static int restore_flag = 0;

static int flicker_mode = 50;
static uint16_t ae_target = 1024;

mw_ae_param ae_param = {
	.anti_flicker_mode = MW_ANTI_FLICKER_50HZ,
	.shutter_time_min  = SHUTTER_1BY8000_SEC,
	.shutter_time_max  = SHUTTER_1BY60_SEC,
	.sensor_gain_max   = ISO_6400,
	.tone_curve_duration = MAX_TONE_CURVE_DURATION,
};

#define NO_ARG  0
#define HAS_ARG 1


static struct option long_options[] = {
	{"detect", NO_ARG, 0, 'd'},
	{"correct", NO_ARG, 0, 'c'},
	{"restore", NO_ARG, 0, 'r'},
	{"anti-flicker", HAS_ARG, 0, 'a'},
	{"ae-target", HAS_ARG, 0, 't'},
	{"filename", HAS_ARG, 0, 'f'},
	{0, 0, 0, 0}
};

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_options = "dcra:t:f:";

static const struct hint_s hint[] = {
	{"", "\t\tlens shading detect"},
	{"", "\t\tlens shading correct"},
	{"", "\t\tlens shading restore"},
	{"50|60", "specify anti-flicker mode"},
	{"1024~4096", "specify AE target value"},
	{"filename", "specify location for saving calibration result"},
};

static void usage(void)
{
	int i;

	printf("\ncali_lens_shading usage:\n");
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
	printf("\nExamples:\n");
	printf("\n  detect lens shading:\n      cali_lens_shading -d -a 60 -t 1024 -f lens_shading.bin\n");
	printf("\n  correct lens shading:\n     cali_lens_shading -c\n");
	printf("\n  restore lens shading:\n     cali_lens_shading -r\n");
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
			break;

		case 'c':
			correct_flag = 1;
			break;

		case 'r':
			restore_flag = 1;
			break;
		case 'a':
			flicker_mode = atoi(optarg);
			break;

		case 't':
			ae_target = atoi(optarg);
			break;
		case 'f':
			strcpy(filename, optarg);
			break;

		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}
	return 0;
}


u8 *dsp_mem = NULL;
u32 dsp_size = 0;

static int map_buffer(void)
{
	struct iav_querybuf querybuf;

	querybuf.buf = IAV_BUFFER_DSP;
	if (ioctl(fd_iav, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
		perror("IAV_IOC_QUERY_BUF");
		return -1;
	}

	dsp_size = querybuf.length;
	dsp_mem = mmap(NULL, dsp_size, PROT_READ, MAP_SHARED, fd_iav, querybuf.offset);
	if (dsp_mem == MAP_FAILED) {
		perror("mmap (%d) failed: %s\n");
		return -1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	u8 *raw_buff = NULL;
	u32 lookup_shift;
	struct iav_querydesc raw_info;
	struct iav_rawbufdesc *p_rawbuf_desc = NULL;
	u16 vignette_table[33*33*4] = {0};
	amba_img_dsp_black_correction_t blc;
	vignette_cal_t vig_detect_setup;
	AMBA_DSP_IMG_BYPASS_VIGNETTE_INFO_s vignette_info = {0};

	amba_img_dsp_mode_cfg_t dsp_mode_cfg;
	memset(&dsp_mode_cfg, 0, sizeof(dsp_mode_cfg));
	dsp_mode_cfg.Pipe = AMBA_DSP_IMG_PIPE_VIDEO;
	dsp_mode_cfg.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_FAST;


	int fd_lenshading = -1;
	int rval = -1;
	u32 vin_width, vin_height, buffer_size;
	int count;
	static u32 gain_shift;

	struct vindev_shutter sht_info;

	struct vindev_video_info vin_info;
	struct vindev_mirror vin_mirror;
	int value[1] = {0};
	int sensor_gain = 0;

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

	if (map_buffer() < 0) {
		return -1;
	}

	if (filename[0] == '\0')
		strcpy(filename, default_filename);

/*
	if (img_lib_init(1, 1) < 0) {
		printf("img_lib_init fail\n");
		return -1;
	}
	*/
	if (mw_start_aaa(fd_iav) < 0) {
		perror("mw_start_aaa");
		return -1;
	}

	if (flicker_mode == 50)
		ae_param.anti_flicker_mode = MW_ANTI_FLICKER_50HZ;
	else
		ae_param.anti_flicker_mode = MW_ANTI_FLICKER_60HZ;

	mw_set_ae_param(&ae_param);
	img_set_ae_target_ratio(&ae_target);
	if (detect_flag) {
		printf("wait 5 seconds...\n");
		sleep(5);
		while (1) {
			memset(&sht_info, 0, sizeof(sht_info));
			sht_info.vsrc_id = 0;
			if (ioctl(fd_iav, IAV_IOC_VIN_GET_SHUTTER, &sht_info) < 0) {
				perror("ioctl IAV_IOC_VIN_GET_SHUTTER error");
				exit(1);
			}
			mw_get_sensor_gain(fd_iav, value);
			sensor_gain = (value[0] >> 24);
			printf("sensor gain = %d\n", sensor_gain);
			if (sensor_gain > 11) {  //agc 12dB
				printf("Light is too low !Please turn the light box a little strong. \n");
				sleep(1);
				continue;
			} else {
				printf("Light strength is okay.\n");
				break;
			}
		}

		img_enable_ae(0);
		img_enable_awb(0);
		img_enable_af(0);
		img_enable_adj(0);
		sleep(1);

		//Capture raw here
		printf("Raw capture started\n");
//		img_ae_set_target_ratio(1024);
		memset(&vin_info, 0, sizeof(vin_info));
		vin_info.vsrc_id = 0;
		vin_info.info.mode = AMBA_VIDEO_MODE_CURRENT;
		if (ioctl(fd_iav, IAV_IOC_VIN_GET_VIDEOINFO, &vin_info) < 0) {
			perror("IAV_IOC_VIN_GET_VIDEOINFO");
			goto vignette_cal_exit;
		}
		vin_width = vin_info.info.width;
		vin_height = vin_info.info.height;

		memset(&raw_info, 0, sizeof(raw_info));
		raw_info.qid = IAV_DESC_RAW;
		if (ioctl(fd_iav, IAV_IOC_QUERY_DESC, &raw_info) < 0) {
			if (errno == EINTR) {
				// skip to do nothing
			} else {
				perror("IAV_IOC_QUERY_DESC");
				goto vignette_cal_exit;
			}
		}
		p_rawbuf_desc = &raw_info.arg.raw;

		buffer_size = (p_rawbuf_desc->width) * (p_rawbuf_desc->height) * 2;
		raw_buff = (u8 *)malloc(buffer_size);
		if (raw_buff == NULL) {
			printf("Not enough memory for read out raw buffer!\n");
			goto vignette_cal_exit;
		}
		memset(raw_buff, 0, buffer_size);

		if ((p_rawbuf_desc->width != vin_width) ||
				((p_rawbuf_desc->height != vin_height) &&
				(p_rawbuf_desc->height != (vin_height + WDR_SENSOR_HIST_LINE)))) {
			printf("VIN resolution %dx%d, raw data info %dx%d is incorrect!\n",
				vin_width, vin_height, p_rawbuf_desc->width,
				p_rawbuf_desc->height);
			goto vignette_cal_exit;
		}
		memcpy(raw_buff, (u8 *) (dsp_mem + p_rawbuf_desc->raw_addr_offset), buffer_size);

		vin_mirror.vsrc_id = 0;
		if (ioctl(fd_iav, IAV_IOC_VIN_GET_MIRROR, &vin_mirror) < 0) {
			perror("ioctl IAV_IOC_VIN_GET_MIRROR error");
			goto vignette_cal_exit;
		}

//save_jpeg_stream();
		/*input*/
		vig_detect_setup.raw_addr = (u16 *) raw_buff;
		vig_detect_setup.raw_w = p_rawbuf_desc->width;
		vig_detect_setup.raw_h = p_rawbuf_desc->height;
		vig_detect_setup.bp = vin_mirror.bayer_pattern;
		vig_detect_setup.threshold = 8192;
		vig_detect_setup.compensate_ratio = 896;
		vig_detect_setup.lookup_shift = 255;
		/*output*/
		vig_detect_setup.r_tab = vignette_table + 0*VIGNETTE_MAX_SIZE;
		vig_detect_setup.ge_tab = vignette_table + 1*VIGNETTE_MAX_SIZE;
		vig_detect_setup.go_tab = vignette_table + 2*VIGNETTE_MAX_SIZE;
		vig_detect_setup.b_tab = vignette_table + 3*VIGNETTE_MAX_SIZE;
		amba_img_dsp_get_static_black_level(&dsp_mode_cfg, &blc);
		vig_detect_setup.blc.BlackR = blc.BlackR;
		vig_detect_setup.blc.BlackGr = blc.BlackGr;
		vig_detect_setup.blc.BlackGb = blc.BlackGb;
		vig_detect_setup.blc.BlackB = blc.BlackB;
		rval = img_cal_vignette(&vig_detect_setup);
		if (rval < 0) {
			printf("img_cal_vignette error!\n");
			goto vignette_cal_exit;
		}
		lookup_shift = vig_detect_setup.lookup_shift;
		if ((fd_lenshading = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
			printf("vignette table file open error!\n");
			goto vignette_cal_exit;
		}
		rval = write(fd_lenshading, vignette_table, (4*VIGNETTE_MAX_SIZE*sizeof(u16)));
		if (rval < 0) {
			printf("vignette table file write error!\n");
			goto vignette_cal_exit;
		}
		rval = write(fd_lenshading, &lookup_shift, sizeof(lookup_shift));
		if (rval < 0) {
			printf("vignette table file write error!\n");
			goto vignette_cal_exit;
		}
		rval = write(fd_lenshading, &p_rawbuf_desc->width, sizeof(p_rawbuf_desc->width));
		if (rval < 0) {
			printf("vignette table file write error!\n");
			goto vignette_cal_exit;
		}
		rval = write(fd_lenshading, &p_rawbuf_desc->height, sizeof(&p_rawbuf_desc->height));
		if (rval < 0) {
			printf("vignette table file write error!\n");
			goto vignette_cal_exit;
		}

		close(fd_lenshading);
		rval = 0;

vignette_cal_exit:
		if(raw_buff != NULL) {
			free(raw_buff);
		}
		if (mw_stop_aaa() < 0) {
			perror("mw_start_aaa");
			return -1;
		}
		return rval;
	}

	if (correct_flag || restore_flag) {
		sleep(5);
		if (correct_flag) {
			if((fd_lenshading = open(filename, O_RDONLY, 0)) < 0) {
				printf("lens_shading.bin cannot be opened\n");
				return -1;
			}
			count = read(fd_lenshading, vignette_table, 4*VIGNETTE_MAX_SIZE*sizeof(u16));
			if (count != 4*VIGNETTE_MAX_SIZE*sizeof(u16)) {
				printf("read lens_shading.bin error\n");
				return -1;
			}
			count = read(fd_lenshading, &gain_shift, sizeof(u32));
			if (count != sizeof(u32)) {
				printf("read lens_shading.bin error\n");
				return -1;
			}
			vignette_info.Enable = 1;
		} else if (restore_flag) {
			vignette_info.Enable = 0;
		}


		vignette_info.GainShift = (u8)gain_shift;
		vignette_info.pRedGain = vignette_table + 0*VIGNETTE_MAX_SIZE;
		vignette_info.pGreenEvenGain = vignette_table + 1*VIGNETTE_MAX_SIZE;
		vignette_info.pGreenOddGain = vignette_table + 2*VIGNETTE_MAX_SIZE;
		vignette_info.pBlueGain = vignette_table + 3*VIGNETTE_MAX_SIZE;
		amba_img_dsp_set_vignette_compensation_bypass(fd_iav, &dsp_mode_cfg, &vignette_info);

		if(fd_lenshading > 0){
			close(fd_lenshading);
			fd_lenshading = -1;
		}
		if (mw_stop_aaa() < 0) {
			perror("mw_start_aaa");
			return -1;
		}
	}
	return 0;
}

