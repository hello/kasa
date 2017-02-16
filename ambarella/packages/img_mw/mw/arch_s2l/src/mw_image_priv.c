/**********************************************************************
 *
 * mw_image_priv.c
 *
 * History:
 *	2010/02/28 - [Jian Tang] Created this file
 *	2012/03/23 - [Jian Tang] Modified this file
 *
 * Copyright (C) 2007 - 2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 *********************************************************************/

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <basetypes.h>

#include <pthread.h>

#include "iav_common.h"
#include "iav_ioctl.h"
#include "iav_vin_ioctl.h"
#include "iav_vout_ioctl.h"
#include "ambas_imgproc_arch.h"

#include "img_adv_struct_arch.h"
#include "img_api_adv_arch.h"
#include "img_dev.h"
#include "ambas_imgproc_ioctl_arch.h"

#include "mw_aaa_params.h"
#include "mw_api.h"
#include "mw_pri_struct.h"
#include "mw_image_priv.h"
#include "mw_ir_led.h"

#include "AmbaDSP_Img3aStatistics.h"
#include "AmbaDSP_ImgUtility.h"
#include "img_customer_interface_arch.h"

typedef enum  {
	SENSOR_50HZ_LINES,
	SENSOR_60HZ_LINES,
	SENSOR_60P50HZ_LINES,
	SENSOR_60P60HZ_LINES,
	LINES_TOTAL_NUM,
} LINES_ID;

/***************************************
*
*  Sensor: ar0331
*
***************************************/

typedef enum  {
	AR0331_LINEAR_ADJ_PARAM,
	AR0331_ADJ_PARAM,
	AR0331_ADJ_PARAM_TOTAL_NUM,
} AR0331_ADJ_PARAM_ID;

typedef enum  {
	AR0331_LINEAR_AWB_PARAM,
	AR0331_AWB_PARAM,
	AR0331_AWB_PARAM_TOTAL_NUM,
} AR0331_AWB_PARAM_ID;

typedef enum  {
	AR0331_LINEAR_SENSOR_CONFIG,
	AR0331_SENSOR_CONFIG,
	AR0331_SENSOR_CONFIG_TOTAL_NUM,
} AR0331_SENSOR_CONFIG_ID;


#define CALI_FILES_PATH		"/ambarella/calibration"
#define	ADJ_PARAM_PATH	"/etc/idsp/adj_params"

#define PRELOAD_AWB_FILE		"/tmp/awb"
#define PRELOAD_AE_FILE		"/tmp/ae"

#define AWB_TILE_NUM_COL		(24)
#define AWB_TILE_NUM_ROW		(16)

extern _mw_global_config G_mw_config;
static line_t G_mw_ae_lines[MAX_AE_LINES_NUM];
static pthread_t ae_control_task_id = 0;

/*************************** AE lines **********************************/

static const u8 G_gain_table[] = {
	ISO_100,
	ISO_150,
	ISO_200,	//6db
	ISO_300,
	ISO_400,	//12db
	ISO_600,
	ISO_800,	//18db
	ISO_1600,	//24db
	ISO_3200 ,	//30db
	ISO_6400,	//36db
	ISO_12800,	//42db
	ISO_25600,	//48db
	ISO_51200,	//54db
	ISO_102400,//60db
};
#define	GAIN_TABLE_NUM		(sizeof(G_gain_table) / sizeof(u8))

static const line_t G_50HZ_LINES[] = {
	{
		{{SHUTTER_1BY16000, ISO_100, APERTURE_F11}},
		{{SHUTTER_1BY1024, ISO_100, APERTURE_F11}}
	},
	{
		{{SHUTTER_1BY1024, ISO_100, APERTURE_F11}},
		{{SHUTTER_1BY1024, ISO_100, APERTURE_F5P6}}
	},
	{
		{{SHUTTER_1BY1024, ISO_100, APERTURE_F5P6}},
		{{SHUTTER_1BY100, ISO_100, APERTURE_F5P6}}
	},
	{
		{{SHUTTER_1BY100, ISO_100, APERTURE_F5P6}},
		{{SHUTTER_1BY100, ISO_100, APERTURE_F2P8}}
	},
	{
		{{SHUTTER_1BY100, ISO_100, APERTURE_F2P8}},
		{{SHUTTER_1BY100, ISO_200, APERTURE_F2P8}}
	},
	{
		{{SHUTTER_1BY50, ISO_100, APERTURE_F2P8}},
		{{SHUTTER_1BY50, ISO_100, APERTURE_F1P2}}
	},
	{
		{{SHUTTER_1BY50, ISO_100, APERTURE_F1P2}},
		{{SHUTTER_1BY50, ISO_200, APERTURE_F1P2}}
	},
	{
		{{SHUTTER_1BY33, ISO_100, APERTURE_F1P2}},
		{{SHUTTER_1BY33, ISO_200, APERTURE_F1P2}},
	},
	{
		{{SHUTTER_1BY25, ISO_100, APERTURE_F1P2}},
		{{SHUTTER_1BY25, ISO_400, APERTURE_F1P2}}
	},
	{
		{{SHUTTER_1BY12P5, ISO_100, APERTURE_F1P2}},
		{{SHUTTER_1BY12P5, ISO_400, APERTURE_F1P2}}
	},
	{
		{{SHUTTER_1BY10, ISO_100, APERTURE_F1P2}},
		{{SHUTTER_1BY10, ISO_400, APERTURE_F1P2}}
	},
	{
		{{SHUTTER_INVALID, 0, 0}}, {{SHUTTER_INVALID, 0, 0}}
	}
};

static const line_t G_60HZ_LINES[] = {
	{
		{{SHUTTER_1BY16000, ISO_100, APERTURE_F11}},
		{{SHUTTER_1BY1024, ISO_100, APERTURE_F11}}
	},
	{
		{{SHUTTER_1BY1024, ISO_100, APERTURE_F11}},
		{{SHUTTER_1BY1024, ISO_100, APERTURE_F5P6}}
	},
	{
		{{SHUTTER_1BY1024, ISO_100, APERTURE_F5P6}},
		{{SHUTTER_1BY120, ISO_100, APERTURE_F5P6}}
	},
	{
		{{SHUTTER_1BY120, ISO_100, APERTURE_F5P6}},
		{{SHUTTER_1BY120, ISO_100, APERTURE_F2P8}}
	},
	{
		{{SHUTTER_1BY120, ISO_100, APERTURE_F2P8}},
		{{SHUTTER_1BY120, ISO_200, APERTURE_F2P8}}
	},
	{
		{{SHUTTER_1BY60, ISO_100, APERTURE_F2P8}},
		{{SHUTTER_1BY60, ISO_100, APERTURE_F1P2}}
	},
	{
		{{SHUTTER_1BY60, ISO_100, APERTURE_F1P2}},
		{{SHUTTER_1BY60, ISO_200, APERTURE_F1P2}}
	},
	{
		{{SHUTTER_1BY40, ISO_100, APERTURE_F1P2}},
		{{SHUTTER_1BY40, ISO_200, APERTURE_F1P2}}
	},
	{
		{{SHUTTER_1BY30, ISO_100, APERTURE_F1P2}},
		{{SHUTTER_1BY30, ISO_200, APERTURE_F1P2}}
	},
	{
		{{SHUTTER_1BY15, ISO_100, APERTURE_F1P2}},
		{{SHUTTER_1BY15, ISO_200, APERTURE_F1P2}}
	},
	{
		{{SHUTTER_1BY10, ISO_100, APERTURE_F1P2}},
		{{SHUTTER_1BY10, ISO_400, APERTURE_F1P2}}
	},
	{
		{{SHUTTER_INVALID, 0, 0}}, {{SHUTTER_INVALID, 0, 0}}
	}
};

static const line_t G_50HZ_SHORT_LINES[1] = {
	{
		{{SHUTTER_1BY16000, ISO_100, 0}},
		{{SHUTTER_1BY100, ISO_100,0}}
	},
};

static const line_t G_60HZ_SHORT_LINES[1] = {
	{
		{{SHUTTER_1BY16000, ISO_100, 0}},
		{{SHUTTER_1BY120, ISO_100,0}}
	},
};

static const line_t G_50HZ_MID_LINES[1] = {
	{
		{{SHUTTER_1BY16000, ISO_100, 0}},
		{{SHUTTER_1BY100, ISO_100,0}}
	},
};

static const line_t G_60HZ_MID_LINES[1] = {
	{
		{{SHUTTER_1BY16000, ISO_100, 0}},
		{{SHUTTER_1BY120, ISO_100,0}}
	},
};

/**********************************************************************
 *	  Internal functions
 *********************************************************************/
static int search_nearest(u32 key, u32 arr[], int size) 	//the arr is in reverse order
{
	int l = 0;
	int r = size - 1;
	int m = (l+r) / 2;

	while(1) {
		if (l == r) {
			return l;
		}
		if (key > arr[m]) {
			r = m;
		} else if  (key < arr[m]) {
			l = m + 1;
		} else {
			return m;
		}
		m = (l+r) / 2;
	}
	return -1;
}

extern u32 TIME_DATA_TABLE_128[SHUTTER_TIME_TABLE_LENGTH];
int shutter_q9_to_index(u32 shutter_time)
{
	int tmp_idx;
	tmp_idx = search_nearest(shutter_time, TIME_DATA_TABLE_128, SHUTTER_TIME_TABLE_LENGTH);

	return SHUTTER_TIME_TABLE_LENGTH - tmp_idx;
}

u32 shutter_index_to_q9(int index)
{
	if (index < 0 || index >= SHUTTER_TIME_TABLE_LENGTH) {
		MW_ERROR("The index is not in the range\n");
		return -1;
	}

	return TIME_DATA_TABLE_128[index];
}

extern const u32 IRIS_DATA_TABLE_128[IRIS_FNO_TABLE_LENGTH];

static int search_nearest_order(u32 key, const u32 arr[], int size) //the arr is in order
{
	int l = 0;
	int r = size - 1;
	int m = (l+r) / 2;

	while(1) {
		if (l == r)
			return l;
		if (key < arr[m]) {
			r = m;
		} else if  (key > arr[m]) {
			l = m + 1;
		} else {
			return m;
		}
		m = (l+r) / 2;
	}
	return -1;
}

static int iris_to_index(u32 lens_fno)
{
	int tmp_idx;

	tmp_idx = search_nearest_order(lens_fno,
		IRIS_DATA_TABLE_128, IRIS_FNO_TABLE_LENGTH);
	return IRIS_FNO_TABLE_LENGTH - tmp_idx;
}

static u32 iris_index_to_fno(int index)
{
	if (index < 0 || index >= IRIS_FNO_TABLE_LENGTH) {
		return 0;
	}

	return IRIS_DATA_TABLE_128[IRIS_FNO_TABLE_LENGTH_1 - index];
}

#define LENS_FNO(index)		(iris_index_to_fno(index))

inline int update_hdr_shutter_max(u32 fps, int expo_num)
{
	u32 shutter_max = 0;
	int i;
	const line_t *p_src = NULL;

	switch (expo_num) {
		case HDR_2X:
			shutter_max = HDR_2X_MAX_SHUTTER(fps);
			break;
		case HDR_3X:
			shutter_max = HDR_3X_MAX_SHUTTER(fps);
			break;
		default:
			MW_ERROR("Not support hdr expo num[%d]\n", expo_num);
			return -1;
	}

	p_src = (G_mw_config.ae_params.anti_flicker_mode == MW_ANTI_FLICKER_60HZ) ?
		G_60HZ_LINES : G_50HZ_LINES;

	for (i = 0; i < MAX_AE_LINES_NUM; ++i) {
		if (shutter_max < p_src[i].end.factor[MW_SHUTTER]) {
			shutter_max = p_src[i - 1].end.factor[MW_SHUTTER];
			break;
		}
		if (p_src[i].end.factor[MW_SHUTTER] == SHUTTER_INVALID) {
			shutter_max = p_src[i - 1].end.factor[MW_SHUTTER];
			break;
		}
	}

	if (G_mw_config.ae_params.shutter_time_max > shutter_max)
		G_mw_config.ae_params.shutter_time_max = shutter_max;

	return 0;
}

int get_sensor_aaa_params(_mw_global_config *pMw_info)
{
	u32 vin_fps = 0;
	_mw_sensor_param *sensor = NULL;
	mw_sys_res *resource = NULL;
	struct vindev_devinfo vin_info;
	struct vindev_video_info video_info;

	sensor = &(pMw_info->sensor);
	resource = &(pMw_info->res);

	vin_info.vsrc_id = 0;
	if (ioctl(pMw_info->fd, IAV_IOC_VIN_GET_DEVINFO, &vin_info) < 0) {
		perror("IAV_IOC_VIN_GET_DEVINFO error\n");
		return -1;
	}

	memset(&video_info, 0, sizeof(video_info));
	video_info.vsrc_id = 0;
	video_info.info.mode = AMBA_VIDEO_MODE_CURRENT;
	if (ioctl(pMw_info->fd, IAV_IOC_VIN_GET_VIDEOINFO, &video_info) < 0) {
		perror("IAV_IOC_VIN_GET_VIDEOINFO");
		return 0;
	}

	if (video_info.info.width != sensor->vin_width ||
		video_info.info.height != sensor->vin_height ||
		video_info.info.fps != sensor->default_fps) {
		pMw_info->init_params.reload_flag = 0;
	}
	sensor->vin_width = video_info.info.width;
	sensor->vin_height = video_info.info.height;
	sensor->raw_width = ROUND_UP(video_info.info.width, PIXEL_IN_MB);
	sensor->raw_height = ROUND_UP(video_info.info.height, PIXEL_IN_MB);
	sensor->raw_resolution = video_info.info.bits;
	sensor->is_rgb_sensor = (vin_info.dev_type == VINDEV_TYPE_SENSOR) &&
		((video_info.info.type == AMBA_VIDEO_TYPE_RGB_601) ||
		(video_info.info.type == AMBA_VIDEO_TYPE_RGB_RAW));

	if (!sensor->is_rgb_sensor) {
		return 0;
	}

	sensor->vin_aaa_info.vsrc_id = 0;
	if (ioctl(pMw_info->fd, IAV_IOC_VIN_GET_AAAINFO,
		&(sensor->vin_aaa_info)) < 0) {
		perror("IAV_IOC_VIN_GET_AAAINFO error\n");
		return -1;
	}
	get_vin_frame_rate(&vin_fps);
	sensor->default_fps = vin_fps;
	sensor->current_fps = vin_fps;

	if (resource->hdr_expo_num > HDR_1X) {
		if (update_hdr_shutter_max(vin_fps, resource->hdr_expo_num) < 0) {
			MW_ERROR("update_hdr_shutter_max error \n");
			return -1;
		}
	}

	switch (sensor->vin_aaa_info.sensor_id) {
		case SENSOR_MN34220PL:
			sprintf(sensor->name, "mn34220pl");
			break;

		case SENSOR_OV9710:
			sprintf(sensor->name, "ov9715");
			break;
		case SENSOR_OV9718:
			sprintf(sensor->name, "ov9718");
			break;
		case SENSOR_OV9750:
			sprintf(sensor->name, "ov9750");
			break;
		case SENSOR_OV4689:
			sprintf(sensor->name, "ov4689");
			break;
		case SENSOR_OV5658:
			sprintf(sensor->name, "ov5658");
			break;

		case SENSOR_IMX123:
			sprintf(sensor->name, "imx123");
			break;
		case SENSOR_IMX124:
			sprintf(sensor->name, "imx124");
			break;
		case SENSOR_IMX123_DCG:
			sprintf(sensor->name, "imx123dcg");
			break;
		case SENSOR_IMX178:
			sprintf(sensor->name, "imx178");
			break;
		case SENSOR_IMX224:
			sprintf(sensor->name, "imx224");
			break;
		case SENSOR_IMX290:
			sprintf(sensor->name, "imx290");
			break;
		case SENSOR_IMX291:
			sprintf(sensor->name, "imx291");
			break;
		case SENSOR_IMX322:
			sprintf(sensor->name, "imx322");
			break;

		case SENSOR_AR0141:
			sprintf(sensor->name, "ar0141");
			break;
		case SENSOR_AR0130:
			sprintf(sensor->name, "ar0130");
			break;
		case SENSOR_MT9T002:
			sprintf(sensor->name, "mt9t002");
			break;
		case SENSOR_AR0230:
			sprintf(sensor->name, "ar0230");
			break;
		default:
			MW_INFO("sensor is [%s]\n", vin_info.name);
			break;
	}

	/* fix me, need match the special files and the default different bin */
	if (strcmp(sensor->load_files.adj_file, "") == 0) {
		switch (resource->isp_pipeline) {
			case ISP_PIPELINE_ADV_LISO:
				sprintf(sensor->load_files.adj_file, "%s/%s_aliso_adj_param.bin",
					ADJ_PARAM_PATH, sensor->name);
				break;
			case ISP_PIPELINE_MID_LISO:
				if (resource->hdr_mode == HDR_PIPELINE_ADV) {
					sprintf(sensor->load_files.adj_file,
						"%s/%s_mliso_adj_param_hdr.bin",
						ADJ_PARAM_PATH, sensor->name);
					if (access(sensor->load_files.adj_file, 0) < 0) {
						sprintf(sensor->load_files.adj_file,
							"%s/%s_mliso_adj_param.bin",
							ADJ_PARAM_PATH, sensor->name);
						printf("NO HDR adj params for %s HDR mode, use Mid "
							"ISO adj params instead!\n", sensor->name);
					}
				} else {
					sprintf(sensor->load_files.adj_file,
						"%s/%s_mliso_adj_param.bin",
						ADJ_PARAM_PATH, sensor->name);
				}
				break;
			case ISP_PIPELINE_LISO:
				sprintf(sensor->load_files.adj_file, "%s/%s_liso_adj_param.bin",
					ADJ_PARAM_PATH, sensor->name);
				break;
			case ISP_PIPELINE_B_LISO:
				sprintf(sensor->load_files.adj_file, "%s/%s_bliso_adj_param.bin",
					ADJ_PARAM_PATH, sensor->name);
				break;
			default:
				sprintf(sensor->load_files.adj_file, "%s/%s_liso_adj_param.bin",
					ADJ_PARAM_PATH, sensor->name);
				break;
		}
	}

	if (strcmp(sensor->load_files.aeb_file, "") == 0) {
		sprintf(sensor->load_files.aeb_file, "%s/%s_aeb_param.bin",
			ADJ_PARAM_PATH, sensor->name);
	}

	return 0;
}

int mw_get_wb_cal(mw_wb_gain *pGain)
{
	if (pGain == NULL) {
		MW_ERROR("[mw_get_wb_cal] NULL pointer!\n");
		return -1;
	}

	return 0;
}

int mw_set_custom_gain(mw_wb_gain *pGain)
{
	if (pGain == NULL) {
		MW_ERROR("[mw_set_custom_gain] NULL pointer!\n");
		return -1;
	}

	return 0;
}

inline int get_vin_mode(u32 *vin_mode)
{
	struct vindev_mode vsrc_mode;

	vsrc_mode.vsrc_id = 0;
	if (ioctl(G_mw_config.fd, IAV_IOC_VIN_GET_MODE, &vsrc_mode) < 0) {
		perror("IAV_IOC_VIN_GET_MODE");
		return -1;
	}

	*vin_mode = vsrc_mode.video_mode;
	return 0;
}

inline int get_vin_frame_rate(u32 *pFrame_rate)
{
	struct vindev_fps vsrc_fps;

	vsrc_fps.vsrc_id = 0;
	if (ioctl(G_mw_config.fd, IAV_IOC_VIN_GET_FPS, &vsrc_fps) < 0) {
		perror("IAV_IOC_VIN_GET_FPS");
		return -1;
	}

	*pFrame_rate = vsrc_fps.fps;
	return 0;
}

static inline int set_vsync_vin_framerate(u32 frame_rate)
{
	int rval;
	struct vindev_fps vsrc_fps;

	ioctl(G_mw_config.fd, IAV_IOC_WAIT_NEXT_FRAME, 0);

	vsrc_fps.vsrc_id = 0;
	vsrc_fps.fps = frame_rate;
	if ((rval = ioctl(G_mw_config.fd, IAV_IOC_VIN_SET_FPS, &vsrc_fps)) < 0) {
		perror("IAV_IOC_VIN_SET_FPS");
		return rval;
	}

	return 0;
}

int get_dsp_mode_cfg(amba_img_dsp_mode_cfg_t *dsp_mode_cfg)
{
	if (dsp_mode_cfg == NULL) {
		return -1;
	}
	dsp_mode_cfg->Pipe = AMBA_DSP_IMG_PIPE_VIDEO;
	if (G_mw_config.res.hdr_mode != HDR_PIPELINE_OFF) {
		dsp_mode_cfg->FuncMode = AMBA_DSP_IMG_FUNC_MODE_HDR;
	}
	if (G_mw_config.res.isp_pipeline == ISP_PIPELINE_LISO) {
		dsp_mode_cfg->AlgoMode = AMBA_DSP_IMG_ALGO_MODE_FAST;
	} else if (G_mw_config.res.isp_pipeline == ISP_PIPELINE_ADV_LISO ||
		G_mw_config.res.isp_pipeline == ISP_PIPELINE_MID_LISO ||
		G_mw_config.res.isp_pipeline == ISP_PIPELINE_B_LISO) {
		dsp_mode_cfg->AlgoMode = AMBA_DSP_IMG_ALGO_MODE_LISO;
		dsp_mode_cfg->BatchId = 0xff;
	}

	return 0;
}

int load_bad_pixel_cali_data(char *file)
{
	int fd_bpc, count, rval;
	u8 *bpc_map_addr;
	u32 raw_pitch, bpc_map_size;
	AMBA_DSP_IMG_BYPASS_SBP_INFO_s sbp_corr;
	amba_img_dsp_cfa_noise_filter_t cfa_noise_filter;
	amba_img_dsp_dbp_correction_t bad_corr;
	static amba_img_dsp_mode_cfg_t dsp_mode_cfg;
	static amba_img_dsp_anti_aliasing_t AntiAliasing;
	memset(&AntiAliasing, 0, sizeof(AntiAliasing));
	memset(&dsp_mode_cfg, 0, sizeof(dsp_mode_cfg));
	memset(&cfa_noise_filter, 0, sizeof(cfa_noise_filter));
	bad_corr.DarkPixelStrength = 0;
	bad_corr.HotPixelStrength = 0;
	bad_corr.Enb = 0;

	if (get_dsp_mode_cfg(&dsp_mode_cfg) < 0) {
		return -1;
	}

	amba_img_dsp_set_cfa_noise_filter(G_mw_config.fd, &dsp_mode_cfg, &cfa_noise_filter);
	amba_img_dsp_set_dynamic_bad_pixel_correction(G_mw_config.fd, &dsp_mode_cfg, &bad_corr);
	amba_img_dsp_set_anti_aliasing(G_mw_config.fd, &dsp_mode_cfg, &AntiAliasing);

	fd_bpc = count = -1;
	if ((fd_bpc = open(file, O_RDONLY, 0)) < 0) {
		MW_ERROR("[%s] cannot be opened!\n", file);
		return -1;
	}
	bpc_map_addr = NULL;
	raw_pitch = ROUND_UP(G_mw_config.sensor.vin_width / 8, 32);
	bpc_map_size = raw_pitch * G_mw_config.sensor.vin_height;
	if ((bpc_map_addr = malloc(bpc_map_size)) == NULL) {
		MW_ERROR("CANNOT malloc memory for BPC map!\n");
		return -1;
	}
	memset(bpc_map_addr, 0, bpc_map_size);
	if ((count = read(fd_bpc, bpc_map_addr, bpc_map_size)) != bpc_map_size) {
		MW_ERROR("Read [%s] error!\n", file);
		rval = -1;
		goto free_bpc_map;
	}
	sbp_corr.Enable = 3;
	sbp_corr.PixelMapPitch = raw_pitch;
	sbp_corr.PixelMapWidth = G_mw_config.sensor.vin_width;
	sbp_corr.PixelMapHeight = G_mw_config.sensor.vin_height;
	sbp_corr.pMap = bpc_map_addr;
	rval = amba_img_dsp_set_static_bad_pixel_correction_bypass(G_mw_config.fd,
		&dsp_mode_cfg, &sbp_corr);
	MW_INFO("[DONE] load bad pixel calibration data [%dx%d] from [%s]!\n",
		G_mw_config.sensor.vin_width, G_mw_config.sensor.vin_height, file);

free_bpc_map:
	close(fd_bpc);
	free(bpc_map_addr);
	return rval;
}

static int get_multi_int_arg(char *optarg, int *arr, int count)
{
	int i;
	char *delim = ":\n";
	char *ptr = NULL;

	ptr = strtok(optarg, delim);
	arr[0] = atoi(ptr);

	for (i = 1; i < count; ++i) {
		if ((ptr = strtok(NULL, delim)) == NULL) {
			break;
		}
		arr[i] = atoi(ptr);
	}
	return (i < count) ? -1 : 0;
}


static int correct_awb_cali_data(int *correct_param)
{
	wb_gain_t orig[2], target[2];
	u16 th_r, th_b;
	int low_r, low_b, high_r, high_b;
	int i;

	for (i = 0; i < 2; ++i) {
		orig[i].r_gain = correct_param[i*3];
		orig[i].g_gain = correct_param[i*3+1];
		orig[i].b_gain = correct_param[i*3+2];
		target[i].r_gain = correct_param[i*3+6];
		target[i].g_gain = correct_param[i*3+7];
		target[i].b_gain = correct_param[i*3+8];
	}

	low_r = ABS(target[0].r_gain - orig[0].r_gain);
	high_r = ABS(target[1].r_gain - orig[1].r_gain);
	low_b = ABS(target[0].b_gain - orig[0].b_gain);
	high_b = ABS(target[1].b_gain - orig[1].b_gain);

	th_r = MAX(low_r, high_r);
	th_b = MAX(low_b, high_b);

	if (img_set_awb_cali_thre(&th_r, &th_b) < 0) {
		MW_ERROR("img_set_awb_cali_thre error!\n");
		return -1;
	}
	if (img_set_awb_cali_shift(orig, target) < 0) {
		MW_ERROR("img_set_awb_cali_shift error!\n");
		return -1;
	}

	return 0;
}

int load_mctf_bin(void)
{
	return 0;
}

int load_awb_cali_data(char *file)
{
	#define	TOTAL_PARAMS		(12)
	char content[128];
	int correct_param[TOTAL_PARAMS];
	int fd_wb;

	fd_wb = -1;
	if ((fd_wb = open(file, O_RDONLY, 0)) < 0) {
		MW_ERROR("[%s] cannot be opened!\n", file);
		return -1;
	}
	if (read(fd_wb, content, sizeof(content)) < 0) {
		MW_ERROR("Read [%s] error!\n", file);
		return -1;
	}
	if (get_multi_int_arg(content, correct_param, TOTAL_PARAMS) < 0) {
		MW_ERROR("Invalid white balance calibration data!\n");
		return -1;
	}

	if (correct_awb_cali_data(correct_param) < 0) {
		return -1;
	}
	MW_INFO("[DONE] load white balance calibration data!\n");
	close(fd_wb);
	return 0;
}

int load_lens_shading_cali_data(char *file)
{
	int fd_shading, count, total_count, rval = 0;
	u16 *shading_table;
	AMBA_DSP_IMG_BYPASS_VIGNETTE_INFO_s vignette_info = {0};
	u32 raw_width, raw_height;
	u32 lookup_shift = -1;
	amba_img_dsp_mode_cfg_t dsp_mode_cfg;
	memset(&dsp_mode_cfg, 0, sizeof(dsp_mode_cfg));
	if (get_dsp_mode_cfg(&dsp_mode_cfg) < 0) {
		return -1;
	}
	shading_table = NULL;
	total_count = 4 * VIGNETTE_MAX_SIZE * sizeof(u16);
	memset(&vignette_info, 0, sizeof(vignette_info));

	fd_shading = count = -1;

	if ((fd_shading = open(file, O_RDONLY, 0)) < 0) {
		MW_ERROR("[%s] cannot be opened!\n", file);
		return -1;
	}

	if ((shading_table = malloc(total_count)) == NULL) {
		MW_ERROR("CANNOT malloc memory for lens shading table!\n");
		return -1;
	}
	memset(shading_table, 0, total_count);
	if ((count = read(fd_shading, shading_table, total_count)) != total_count) {
		MW_ERROR("Read shading data from [%s] error!\n", file);
		rval = -1;
		goto free_shading_table;
	}
	if ((count = read(fd_shading, &lookup_shift, sizeof(u32))) != sizeof(u32)) {
		MW_ERROR("Read shading data from [%s] error!\n", file);
		rval = -1;
		goto free_shading_table;
	}

	if ((count = read(fd_shading,&raw_width,sizeof(raw_width))) != sizeof(raw_width)) {
		MW_ERROR("Read shading data from [%s] error!\n", file);
		rval = -1;
		goto free_shading_table;
	}
	if ((count = read(fd_shading,&raw_height,sizeof(raw_height))) != sizeof(raw_height)) {
		MW_ERROR("Read shading data from [%s] error!\n", file);
		rval = -1;
		goto free_shading_table;
	}

	vignette_info.Enable = 1;
	vignette_info.GainShift = (u16)lookup_shift;
	vignette_info.pRedGain = shading_table + 0*VIGNETTE_MAX_SIZE;
	vignette_info.pGreenEvenGain = shading_table + 1*VIGNETTE_MAX_SIZE;
	vignette_info.pGreenOddGain = shading_table + 2*VIGNETTE_MAX_SIZE;
	vignette_info.pBlueGain = shading_table + 3*VIGNETTE_MAX_SIZE;
	amba_img_dsp_set_vignette_compensation_bypass(G_mw_config.fd, &dsp_mode_cfg, &vignette_info);
	MW_INFO("[DONE] load lens shading calibration data!\n");

free_shading_table:
	close(fd_shading);
	free(shading_table);
	return rval;
}

int load_calibration_data(mw_cali_file *file, MW_CALIBRATION_TYPE type)
{
	int rval = 0;
	char file_name[256];
	switch (type) {
	case MW_CALIB_BPC:
		if (strlen(file->bad_pixel) > 0) {
			if (load_bad_pixel_cali_data(file->bad_pixel) < 0) {
				rval = -1;
			}
		} else {
			sprintf(file_name, "%s/%dx%d_cali_bad_pixel.bin", CALI_FILES_PATH,
				G_mw_config.sensor.vin_width, G_mw_config.sensor.vin_height);
			if (access(file_name, 0) < 0) {
				return 0;
			} else {
				if (load_bad_pixel_cali_data(file_name) < 0) {
					rval = -1;
				}
			}
		}
		break;
	case MW_CALIB_WB:
		if (strlen(file->wb) > 0) {
			if (load_awb_cali_data(file->wb) < 0) {
				rval = -1;
			}
		} else {
			sprintf(file_name, "%s/%dx%d_cali_awb.bin", CALI_FILES_PATH,
				G_mw_config.sensor.vin_width, G_mw_config.sensor.vin_height);
			if (access(file_name, 0) < 0) {
				return 0;
			} else {
				if (load_awb_cali_data(file_name) < 0) {
					rval = -1;
				}
			}
		}
		break;
	case MW_CALIB_LENS_SHADING:
		if (strlen(file->shading) > 0) {
			if (load_lens_shading_cali_data(file->shading)) {
				rval = -1;
			}
		} else {
			sprintf(file_name, "%s/%dx%d_cali_lens_shading.bin", CALI_FILES_PATH,
				G_mw_config.sensor.vin_width, G_mw_config.sensor.vin_height);
			if (access(file_name, 0) < 0) {
				return 0;
			} else {
				if (load_lens_shading_cali_data(file_name) < 0) {
					rval = -1;
				}
			}
		}
		break;
	default:
		rval = -1;
		break;
	}

	return rval;
}

static int load_default_image_param(void)
{
	u16 target_ratio[MAX_HDR_EXPOSURE_NUM];
	image_property_t image_prop;
	awb_control_mode_t wb_mode[MAX_HDR_EXPOSURE_NUM];

	// Get image parameters from imgproc lib
	img_get_img_property(&image_prop);
	G_mw_config.image_params.saturation = image_prop.saturation;
	G_mw_config.image_params.brightness = image_prop.brightness;
	G_mw_config.image_params.hue = image_prop.hue;
	G_mw_config.image_params.contrast = image_prop.contrast;

	// Get basic parameters from imgproc lib
	G_mw_config.ae_params.current_vin_fps = G_mw_config.sensor.current_fps;

	if (G_mw_config.ae_params.shutter_time_max > G_mw_config.sensor.default_fps) {
		G_mw_config.ae_params.shutter_time_max = G_mw_config.sensor.default_fps;
	}

	img_get_ae_target_ratio(target_ratio);
	G_mw_config.ae_params.ae_level[0] = (target_ratio[0] * 100) >> 10;
	img_get_awb_mode(wb_mode);

	G_mw_config.awb_params.wb_mode = wb_mode[0];

	if ((G_mw_config.sensor.lens_id != LENS_CMOUNT_ID) &&
		(strcmp(G_mw_config.sensor.load_files.lens_file, "") != 0)) {
		G_mw_config.ae_params.lens_aperture.aperture_min =
			G_mw_config.ae_params.lens_aperture.FNO_min;
		G_mw_config.ae_params.lens_aperture.aperture_max =
			G_mw_config.ae_params.lens_aperture.FNO_max;
	}

	MW_INFO("   saturation = %d.\n", G_mw_config.image_params.saturation);
	MW_INFO("   brightness = %d.\n", G_mw_config.image_params.brightness);
	MW_INFO("		  hue = %d.\n", G_mw_config.image_params.hue);
	MW_INFO("	 contrast = %d.\n", G_mw_config.image_params.contrast);
	MW_INFO("	sharpness = %d.\n", G_mw_config.image_params.sharpness);
	MW_INFO("    ae target = %d.\n", G_mw_config.ae_params.ae_level[0]);
	MW_INFO("MCTF strength = %d.\n", G_mw_config.enh_params.mctf_strength);

	return 0;
}

int reload_previous_params(void)
{
	u16 target_ratio[MAX_HDR_EXPOSURE_NUM];
	awb_control_mode_t wb_mode[MAX_HDR_EXPOSURE_NUM];

	//restores AE line
	if (G_mw_config.ae_params.slow_shutter_enable) {
		G_mw_config.sensor.current_fps = G_mw_config.ae_params.current_vin_fps;
	}
	load_ae_exp_lines(&G_mw_config.ae_params);
	//restore mctf strength
	if (img_set_mctf_strength(G_mw_config.enh_params.mctf_strength) < 0) {
		MW_ERROR("img_set_mctf_strength error!\n");
		return -1;
	}
	//restore AE exposure level
	target_ratio[0] = (u16)((G_mw_config.ae_params.ae_level[0] << 10) / 100);
	if (img_set_ae_target_ratio(target_ratio) < 0) {
		MW_ERROR("img_ae_set_target_ratio error\n");
		return -1;
	}

	//restores Awb
	if (G_mw_config.awb_params.wb_mode == MW_WB_MODE_HOLD) {
		if (img_enable_awb(0) < 0) {
			MW_ERROR("img_enable_awb error\n");
			return -1;
		}
	} else {
		if (img_enable_awb(1) < 0) {
			MW_ERROR("img_enable_awb error\n");
			return -1;
		}
		img_get_awb_mode(wb_mode);
		wb_mode[0] = G_mw_config.awb_params.wb_mode;
		if (img_set_awb_mode(wb_mode) < 0) {
			MW_ERROR("img_awb_set_mode error\n");
			return -1;
		}
	}

	// restore image parameters
	if (img_set_color_saturation(G_mw_config.image_params.saturation) < 0) {
		MW_ERROR("img_set_color_saturation error!\n");
		return -1;
	}

	if (img_set_color_brightness(G_mw_config.image_params.brightness) < 0) {
		MW_ERROR("img_set_color_brightness error!\n");
		return -1;
	}

	if (img_set_color_hue(G_mw_config.image_params.hue) < 0) {
		MW_ERROR("img_set_color_hue error!\n");
		return -1;
	}

	if (img_set_color_contrast(G_mw_config.image_params.contrast) < 0) {
		MW_ERROR("img_set_color_contrast error!\n");
		return -1;
		}
	if (img_set_sharpening_strength(G_mw_config.image_params.sharpness) < 0) {
		MW_ERROR("img_set_sharpness error!\n");
		return -1;
	}

	MW_INFO("   saturation = %d.\n", G_mw_config.image_params.saturation);
	MW_INFO("   brightness = %d.\n", G_mw_config.image_params.brightness);
	MW_INFO("          hue = %d.\n", G_mw_config.image_params.hue);
	MW_INFO("     contrast = %d.\n", G_mw_config.image_params.contrast);
	MW_INFO("    sharpness = %d.\n", G_mw_config.image_params.sharpness);
	MW_INFO("    ae target = %d.\n", G_mw_config.ae_params.ae_level[0]);

	return 0;
}

static int load_default_af_param(void)
{
	memset(&G_mw_config.af_info, 0, sizeof(G_mw_config.af_info));

	G_mw_config.af_info.lens_type = 0;
	G_mw_config.af_info.zm_dist = 1;
	G_mw_config.af_info.fs_dist = 1;
	G_mw_config.af_info.fs_near = 1;
	G_mw_config.af_info.fs_far = 1;

	return 0;
}

static inline int print_ae_lines(line_t *lines,
	int line_num, u16 line_belt, int enable_convert)
{
	int i;
	MW_INFO("===== [MW] ==== Automatic Generates AE lines =====\n");
	for (i = 0; i < line_num; ++i) {
		if (i == line_belt) {
			MW_INFO("===== [MW] ====== This is the line belt. =========\n");
		}
		if (enable_convert) {
			MW_INFO(" [%d] start (1/%d, %d, %3.2f) == end (1/%d, %d, %3.2f)\n",
				(i + 1), SHT_TIME(lines[i].start.factor[MW_SHUTTER]),
				lines[i].start.factor[MW_DGAIN], (
				float)LENS_FNO(lines[i].start.factor[MW_IRIS]) / LENS_FNO_UNIT,
				SHT_TIME(lines[i].end.factor[MW_SHUTTER]),
				lines[i].end.factor[MW_DGAIN],
				(float)LENS_FNO(lines[i].end.factor[MW_IRIS]) / LENS_FNO_UNIT);
		} else {
			MW_INFO(" [%d] start (%d, %d, %d) == end (%d, %d, %d)\n", (i + 1),
				lines[i].start.factor[MW_SHUTTER],
				lines[i].start.factor[MW_DGAIN], lines[i].start.factor[MW_IRIS],
				lines[i].end.factor[MW_SHUTTER],
				lines[i].end.factor[MW_DGAIN], lines[i].end.factor[MW_IRIS]);
		}
	}
	MW_INFO("======= [MW] ==== NUM [%d] ==== BELT [%d] ==========\n\n",
		line_num, line_belt);
	return 0;
}

static int generate_normal_ae_lines(mw_ae_param *p, line_t *lines,
	int *line_num, u16 *line_belt, int *default_line_belt)
{
	int dst, src, i;
	u32 s_max, s_min, g_max, g_min, p_index_max, p_index_min;
	const line_t *p_src = NULL;
	int flicker_off_shutter_time = 0;
	int longest_possible_shutter = 0, curr_belt = 0;
	int match_vin_shutter = 0;
	int match_vin_belt = 0;
	int calc_def_belt = 0;

	struct vindev_agc vsrc_agc;

	s_max = p->shutter_time_max;
	s_min = p->shutter_time_min;
	g_max = p->sensor_gain_max;
	if ((p->lens_aperture.aperture_min == APERTURE_AUTO) ||
		(p->lens_aperture.aperture_max == APERTURE_AUTO)) {
		p_index_max = 0;
		p_index_min = 0;
	} else {
		p_index_max = iris_to_index(p->lens_aperture.aperture_min);
		p_index_min = iris_to_index(p->lens_aperture.aperture_max);
	}

	p_src = (p->anti_flicker_mode == MW_ANTI_FLICKER_60HZ) ?
		G_60HZ_LINES : G_50HZ_LINES;
	flicker_off_shutter_time = (p->anti_flicker_mode == MW_ANTI_FLICKER_60HZ) ?
		SHUTTER_1BY120_SEC : SHUTTER_1BY100_SEC;
	dst = src = curr_belt = 0;

	// create line 1 - shutter line / digital gain line
	if (s_min < flicker_off_shutter_time) {
		// create shutter line
		while (s_min >= p_src[src].end.factor[MW_SHUTTER])
			++src;
		lines[dst] = p_src[src];
		lines[dst].start.factor[MW_SHUTTER] = s_min;

		if (s_max < flicker_off_shutter_time) {
			lines[dst].end.factor[MW_SHUTTER] = s_max;
			++dst;
			lines[dst].start.factor[MW_SHUTTER] = s_max;
			lines[dst].end.factor[MW_SHUTTER] = s_max;
			lines[dst].end.factor[MW_DGAIN] = g_max;
			++dst;
			curr_belt = dst;
			goto GENERATE_LINES_EXIT;
		}
		++dst;
		++src;
	} else {
		// create digital gain line
		while (s_min > p_src[src].start.factor[MW_SHUTTER])
			++src;
		lines[dst] = p_src[src];
		++dst;
		++src;
	}

	// create other lines - digital gain line
	while (s_max >= p_src[src].start.factor[MW_SHUTTER]) {
		if (p_src[src].start.factor[MW_SHUTTER] == SHUTTER_INVALID) {
			break;
		}
		/* skip iris lines with non-iris lens */
		if ((p_src[src].start.factor[MW_SHUTTER] <
			p_src[src].end.factor[MW_SHUTTER]) ||
			(p_src[src].start.factor[MW_DGAIN] !=
			p_src[src].end.factor[MW_DGAIN]) || p_index_max) {
			lines[dst] = p_src[src];
			++dst;
		}
		++src;
		if (src >= MAX_AE_LINES_NUM) {
			MW_ERROR("Fatal error: AE line number exceeds MAX_AE_LINES_NUM %d",
				MAX_AE_LINES_NUM);
			return -1;
		}
	}
	lines[dst - 1].end.factor[MW_DGAIN] = g_max;

	// change min gain from sensor driver
	get_sensor_agc_info(G_mw_config.fd, &vsrc_agc);
	g_min = vsrc_agc.agc_min >> 24;
	i = 0;
	while (G_gain_table[i] < g_min) {
		if (++i == GAIN_TABLE_NUM) {
			--i;
			break;
		}
	}
	g_min = G_gain_table[i];

	MW_INFO("---Min Gain: %d, Max Gain: %d\n", g_min, g_max);
	i = 0;
	while (i < dst) {
		if (lines[i].start.factor[MW_SHUTTER] != lines[i].end.factor[MW_SHUTTER]) {
			/* Shutter line: change min gain both for start and end points */
			if (lines[i].start.factor[MW_DGAIN] < g_min) {
				lines[i].start.factor[MW_DGAIN] = g_min;
			}
			if (lines[i].end.factor[MW_DGAIN] < g_min) {
				lines[i].end.factor[MW_DGAIN] = g_min;
			}
		} else {
			/* Gain line: change min gain for start point */
			if (lines[i].start.factor[MW_DGAIN] < g_min) {
				lines[i].start.factor[MW_DGAIN] = g_min;
			}
		}
		++i;
	}

	/* change the min/max piris lens size */
	if (p_index_min < lines[0].start.factor[MW_IRIS]) {
		 lines[0].start.factor[MW_IRIS] = p_index_min;
	}
	if (p_index_max > lines[dst].end.factor[MW_IRIS]) {
		 lines[dst].end.factor[MW_IRIS] = p_index_max;
	}
	i = 0;
	while (i < dst) {
		if (p_index_min > lines[i].start.factor[MW_IRIS]) {
			 lines[i].start.factor[MW_IRIS] = p_index_min;
		}
		if (p_index_min > lines[i].end.factor[MW_IRIS]) {
			 lines[i].end.factor[MW_IRIS] = p_index_min;
		}
		if (p_index_max < lines[i].start.factor[MW_IRIS]) {
			 lines[i].start.factor[MW_IRIS] = p_index_max;
		}
		if (p_index_max < lines[i].end.factor[MW_IRIS]) {
			 lines[i].end.factor[MW_IRIS] = p_index_max;
		}
		++i;
	}

	// calculate line belt of current and default
	curr_belt = dst;
	while (1) {
		longest_possible_shutter = lines[curr_belt - 1].end.factor[MW_SHUTTER];
		if (longest_possible_shutter <= G_mw_config.sensor.default_fps) {
			*default_line_belt = curr_belt;
		}
		if (p->slow_shutter_enable) {
			if (longest_possible_shutter <= G_mw_config.sensor.current_fps) {
				MW_INFO("\tSlow shutter is enabled, set curr_belt [%d] to"
					" current frame rate [%d].\n",
					curr_belt, G_mw_config.sensor.current_fps);
				calc_def_belt = 1;
				break;
			}
		} else {
			if (longest_possible_shutter <= G_mw_config.sensor.default_fps) {
				lines[curr_belt - 1].end.factor[MW_DGAIN] = g_max;
				MW_INFO("\tSlow shutter is disabled, restore curr_belt [%d] to"
					" default frame rate [%d].\n",
					curr_belt, G_mw_config.sensor.default_fps);
				break;
			}
		}
		--curr_belt;
	}
	if (calc_def_belt) {
		for (match_vin_belt = curr_belt; match_vin_belt > 1; match_vin_belt--) {
			match_vin_shutter = lines[match_vin_belt - 1].end.factor[MW_SHUTTER];
			if (match_vin_shutter <= G_mw_config.sensor.default_fps) {
				*default_line_belt = match_vin_belt;
				break;
			}
		}
		if (match_vin_belt <= 1) {
			*default_line_belt = 1;
		}
		calc_def_belt = 0;
	}
	MW_INFO("\tcurr_belt [%d] def_belt [%d] == VIN [%d fps]\n",
		curr_belt, *default_line_belt, G_mw_config.sensor.current_fps);

GENERATE_LINES_EXIT:
	*line_num = dst;
	*line_belt = curr_belt;

	print_ae_lines(lines, dst, curr_belt, 1);
	return 0;
}

static inline int generate_manual_shutter_lines(mw_ae_param *p,
	line_t *lines, int *line_num, u16 *line_belt, int *default_line_belt)
{
	int total_lines = 0;
	lines[total_lines].start.factor[MW_SHUTTER] = p->shutter_time_max;
	lines[total_lines].start.factor[MW_DGAIN] = 0;
	lines[total_lines].start.factor[MW_IRIS] = 0;
	lines[total_lines].end.factor[MW_SHUTTER] = p->shutter_time_max;
	lines[total_lines].end.factor[MW_DGAIN] = p->sensor_gain_max;
	lines[total_lines].end.factor[MW_IRIS] = 0;
	++total_lines;
	*line_num = total_lines;
	*line_belt = total_lines;
	*default_line_belt = total_lines;

	print_ae_lines(lines, total_lines, total_lines, 1);
	return 0;
}

static int generate_ae_lines(mw_ae_param *p,
	line_t *lines, int *line_num, u16 *line_belt, int *default_line_belt)
{
	int retv = 0;

	if (p->shutter_time_max != p->shutter_time_min) {
		retv = generate_normal_ae_lines(p, lines, line_num, line_belt, default_line_belt);
	} else {
		retv = generate_manual_shutter_lines(p, lines, line_num, line_belt, default_line_belt);
	}

	return retv;
}

static int load_ae_lines(line_t *line, u16 line_num, u16 line_belt, int default_line_belt)
{
	int i;
	line_t img_ae_lines[MAX_AE_LINES_NUM * MAX_HDR_EXPOSURE_NUM];
	u16 line_num_expo[MAX_HDR_EXPOSURE_NUM];
	u16 line_belt_expo[MAX_HDR_EXPOSURE_NUM];

	memcpy(img_ae_lines, line, sizeof(line_t) * line_num);
	//transfer q9 format to shutter index format
	for (i = 0; i < line_num; i++) {
		img_ae_lines[i].start.factor[MW_SHUTTER]
			= line[i].start.factor[MW_SHUTTER];
		img_ae_lines[i].end.factor[MW_SHUTTER]
			= line[i].end.factor[MW_SHUTTER];
	}

	line_num_expo[0] = line_num;
	line_belt_expo[0] = line_belt;

	if (G_mw_config.res.hdr_expo_num >= HDR_2X) {
		if (G_mw_config.ae_params.anti_flicker_mode == MW_ANTI_FLICKER_60HZ) {
			line_num_expo[1] = sizeof(G_60HZ_MID_LINES) / sizeof(line_t);
			memcpy(img_ae_lines + line_num,
				G_60HZ_MID_LINES, sizeof(G_60HZ_MID_LINES));
		} else {
			line_num_expo[1] = sizeof(G_50HZ_MID_LINES) / sizeof(line_t);
			memcpy(img_ae_lines + line_num,
				G_50HZ_MID_LINES, sizeof(G_50HZ_MID_LINES));
		}
		line_belt_expo[1] = line_num_expo[1];
		line_num += line_num_expo[1];
	}

	if (G_mw_config.res.hdr_expo_num >= HDR_3X) {
		if (G_mw_config.ae_params.anti_flicker_mode == MW_ANTI_FLICKER_60HZ) {
			line_num_expo[2] = sizeof(G_60HZ_SHORT_LINES) / sizeof(line_t);
			memcpy(img_ae_lines + line_num,
				G_60HZ_SHORT_LINES, sizeof(G_60HZ_SHORT_LINES));
		} else {
			line_num_expo[2] = sizeof(G_50HZ_SHORT_LINES) / sizeof(line_t);
			memcpy(img_ae_lines + line_num,
				G_50HZ_SHORT_LINES, sizeof(G_50HZ_SHORT_LINES));
		}
		line_belt_expo[2] = line_num_expo[2];
		line_num += line_num_expo[2];
	}

	pthread_mutex_lock(&G_mw_config.slow_shutter_lock);
	if (img_format_ae_line(G_mw_config.fd, img_ae_lines, line_num,
		G_mw_config.sensor.vin_aaa_info.agc_step) < 0) {
		MW_ERROR("[img_ae_load_exp_line error] : line_num [%d] line_belt [%d].\n",
			line_num, line_belt);
		return -1;
	}

	if (img_set_ae_exp_lines(img_ae_lines, line_num_expo, line_belt_expo) < 0) {
		MW_ERROR("[img_set_ae_exp_lines error] : line_num [%d] line_belt [%d].\n",
			line_num, line_belt);
		return -1;
	}
	ioctl(G_mw_config.fd, IAV_IOC_WAIT_NEXT_FRAME);//Wait two VIN frames to make sure AE lines becomes effective in image kernel
	ioctl(G_mw_config.fd, IAV_IOC_WAIT_NEXT_FRAME);
	G_mw_config.cur_status.ae_belt.line_num = line_num;
	G_mw_config.cur_status.ae_belt.belt_def = default_line_belt;
	memcpy(G_mw_ae_lines, line, sizeof(line_t) * line_num);
	pthread_mutex_unlock(&G_mw_config.slow_shutter_lock);

	return 0;
}

/**********************************************************************
 *	  External functions
 *********************************************************************/
inline int check_state(void)
{
	u32 state;

	memset(&state, 0, sizeof(state));
	if (ioctl(G_mw_config.fd, IAV_IOC_GET_IAV_STATE, &state) < 0) {
		perror("IAV_IOC_GET_IAV_STATE");
		return -1;
	}

	if (state == IAV_STATE_IDLE || state == IAV_STATE_INIT) {
		return -1;
	}

	return 0;
}

int check_iav_work(void)
{
	u32 state = 0;

	if (ioctl(G_mw_config.fd, IAV_IOC_GET_IAV_STATE, &state) < 0) {
		perror("IAV_IOC_GET_IAV_STATE");
		return -1;
	}
	if (state == IAV_STATE_PREVIEW || state == IAV_STATE_ENCODING) {
		return 1;
	}

	return 0;
}

inline int get_iav_cfg(_mw_global_config *mw_info)
{
	struct iav_system_resource system_resource;
	struct iav_srcbuf_setup srcbuf_setup;
	struct iav_enc_mode_cap mode_cap;

	memset(&system_resource, 0, sizeof(system_resource));
	memset(&srcbuf_setup, 0, sizeof(srcbuf_setup));
	memset(&mode_cap, 0, sizeof(mode_cap));

	system_resource.encode_mode = DSP_CURRENT_MODE;
	if (ioctl(mw_info->fd, IAV_IOC_GET_SYSTEM_RESOURCE, &system_resource) < 0) {
		MW_ERROR("IAV_IOC_GET_SYSTEM_RESOURCE error\n");
		return -1;
	}
	mw_info->res.hdr_expo_num = system_resource.exposure_num;
	mw_info->res.hdr_mode = system_resource.hdr_type;
	mw_info->res.isp_pipeline = system_resource.iso_type;
	mw_info->res.raw_pitch = system_resource.raw_pitch_in_bytes;

	if (ioctl(mw_info->fd, IAV_IOC_GET_SOURCE_BUFFER_SETUP, &srcbuf_setup) < 0) {
		MW_ERROR("IAV_IOC_GET_SOURCE_BUFFER_SETUP error\n");
		return -1;
	}
	mw_info->mw_iav.main_width = ROUND_UP(
		srcbuf_setup.size[IAV_SRCBUF_MN].width, PIXEL_IN_MB);
	mw_info->mw_iav.main_height = ROUND_UP(
		srcbuf_setup.size[IAV_SRCBUF_MN].height, PIXEL_IN_MB);

	mode_cap.encode_mode = DSP_CURRENT_MODE;
	if (ioctl(mw_info->fd, IAV_IOC_QUERY_ENC_MODE_CAP, &mode_cap) < 0) {
		MW_ERROR("IAV_IOC_QUERY_ENC_MODE_CAP");
		return -1;
	}
	mw_info->mw_iav.max_cr = mode_cap.max_chroma_radius;
	mw_info->mw_iav.max_wcr = mode_cap.max_wide_chroma_radius;
	mw_info->mw_iav.wcr_enable = mode_cap.wcr_possible;

	return 0;
}

int get_sensor_agc_info(int fd, struct vindev_agc *agc_info)
{
	agc_info->vsrc_id = 0;
	if (ioctl(fd, IAV_IOC_VIN_GET_AGC, agc_info) < 0) {
		MW_ERROR("IAV_IOC_VIN_GET_AGC error\n");
		return -1;
	}

	return 0;
}

inline int get_shutter_time(u32 *pShutter_time)
{
	int rval;
	struct vindev_shutter vsrc_shutter;

	vsrc_shutter.vsrc_id = 0;
	if ((rval = ioctl(G_mw_config.fd, IAV_IOC_VIN_GET_SHUTTER, &vsrc_shutter)) < 0) {
		perror("IAV_IOC_VIN_GET_SHUTTER");
		return rval;
	}

	*pShutter_time = vsrc_shutter.shutter;
	return 0;
}

int load_ae_exp_lines(mw_ae_param *ae)
{
	u16 ae_line_belt = 0;
	int default_line_belt = 0;
	int ae_lines_num;
	line_t ae_lines[MAX_AE_LINES_NUM];
	memset(ae_lines, 0, sizeof(ae_lines));

	if (ae->shutter_time_max < ae->shutter_time_min) {
		MW_INFO("shutter limit max [%d] is less than shutter min [%d]. Tie them to shutter min\n",
			ae->shutter_time_max, ae->shutter_time_min);
		ae->shutter_time_max = ae->shutter_time_min;
	}

	if (generate_ae_lines(ae, ae_lines,
		&ae_lines_num, &ae_line_belt, &default_line_belt) < 0) {
		MW_ERROR("generate_ae_lines error\n");
		return -1;
	}

	if (load_ae_lines(ae_lines, ae_lines_num, ae_line_belt, default_line_belt) < 0) {
		MW_MSG("load_ae_lines error! line_num [%d], line_belt [%d], default_line_belt [%d]\n",
			ae_lines_num, ae_line_belt, default_line_belt);
		return -1;
	}

	return 0;
}

int get_ae_exposure_lines(mw_ae_line *lines, u32 num)
{
	int i;

	for (i = 0; i < num && i < G_mw_config.cur_status.ae_belt.line_num; ++i) {
		lines[i].start.factor[MW_SHUTTER] = G_mw_ae_lines[i].start.factor[MW_SHUTTER];
		lines[i].start.factor[MW_DGAIN] = G_mw_ae_lines[i].start.factor[MW_DGAIN];
		lines[i].start.factor[MW_IRIS] = G_mw_ae_lines[i].start.factor[MW_IRIS];
		lines[i].end.factor[MW_SHUTTER] = G_mw_ae_lines[i].end.factor[MW_SHUTTER];
		lines[i].end.factor[MW_DGAIN] = G_mw_ae_lines[i].end.factor[MW_DGAIN];
		lines[i].end.factor[MW_IRIS] = G_mw_ae_lines[i].end.factor[MW_IRIS];
	}
	print_ae_lines(G_mw_ae_lines, i, G_mw_config.cur_status.ae_belt.belt_cur, 1);

	return 0;
}

int set_ae_exposure_lines(mw_ae_line *lines, u32 num)
{
	int i, curr_belt;
	int default_line_belt = 0;
	line_t ae_lines[MAX_AE_LINES_NUM];

	for (i = 0; i < num - 1; ++i) {
		if (lines[i+1].start.factor[MW_SHUTTER] == SHUTTER_INVALID) {
			break;
		}
		if (lines[i].start.factor[MW_SHUTTER] > lines[i+1].start.factor[MW_SHUTTER]) {
			MW_MSG("AE line [%d] shutter time [%d] must be larger than line [%d] shutter time [%d].\n",
				i+1, lines[i+1].start.factor[MW_SHUTTER], i, lines[i].start.factor[MW_SHUTTER]);
			return -1;
		}
	}
	memset(ae_lines, 0, sizeof(ae_lines));
	for (i = 0, curr_belt = 0; i < num; i++) {
		if (lines[i].start.factor[MW_SHUTTER] == SHUTTER_INVALID) {
			break;
		}
		ae_lines[i].start.factor[MW_SHUTTER] = lines[i].start.factor[MW_SHUTTER];
		ae_lines[i].start.factor[MW_DGAIN] = lines[i].start.factor[MW_DGAIN];
		ae_lines[i].start.factor[MW_IRIS] = lines[i].start.factor[MW_IRIS];
		ae_lines[i].end.factor[MW_SHUTTER] = lines[i].end.factor[MW_SHUTTER];
		ae_lines[i].end.factor[MW_DGAIN] = lines[i].end.factor[MW_DGAIN];
		ae_lines[i].end.factor[MW_IRIS] = lines[i].end.factor[MW_IRIS];
		if (lines[i].end.factor[MW_SHUTTER] <= G_mw_config.sensor.current_fps) {
			curr_belt = i + 1;
		}
		if (lines[i].end.factor[MW_SHUTTER] <= G_mw_config.sensor.default_fps) {
			default_line_belt = i + 1;
		}
	}
	default_line_belt = curr_belt;
	MW_DEBUG("vin [%d]\n", G_mw_config.sensor.current_fps);
	print_ae_lines(ae_lines, i, curr_belt, 1);

	if (load_ae_lines(ae_lines, i, curr_belt, default_line_belt) < 0) {
		MW_MSG("load_ae_lines error! line_num [%d], line_belt [%d], default_line_belt [%d]\n",
			i, curr_belt, default_line_belt);
		return -1;
	}

	return 0;
}

int set_ae_switch_points(mw_ae_point *points, u32 num)
{
	int i, j;
	joint_t *switch_point = NULL;
	line_info *ae_belt = &G_mw_config.cur_status.ae_belt;

	for (i = 0; i < num; ++i) {
		for (j = 0; j < ae_belt->line_num; j++) {
			if (points[i].factor[MW_SHUTTER] == SHUTTER_INVALID) {
				continue;
			}
			if (G_mw_ae_lines[j].start.factor[MW_SHUTTER] == points[i].factor[MW_SHUTTER]) {
				break;
			}
		}
		if (j == ae_belt->line_num) {
			MW_MSG("Invalid switch point [%d, %d].\n",
				points[i].factor[MW_SHUTTER], points[i].factor[MW_DGAIN]);
			continue;
		}
		switch_point = (points[i].pos == MW_AE_END_POINT) ? &G_mw_ae_lines[j].end
			: &G_mw_ae_lines[j].start;
		switch_point->factor[MW_DGAIN] = points[i].factor[MW_DGAIN];
	}

	if (load_ae_lines(G_mw_ae_lines, ae_belt->line_num, ae_belt->belt_cur, ae_belt->belt_def) < 0) {
		MW_MSG("load_ae_lines error! line_num [%d], line_belt_cur [%d], line_belt_def [%d]\n",
			ae_belt->line_num, ae_belt->belt_cur, ae_belt->belt_def);
		return -1;
	}

	return 0;
}

static void slow_shutter_control(int ae_cursor)
{
	#define	HISTORY_LENGTH		(3)
	static int transition_counter = 0;
	u32 curr_frame_time = 0, default_frame_time, target_frame_time;
	line_info *ae_belt = &G_mw_config.cur_status.ae_belt;
	line_t *ae_line = NULL;

	if (G_mw_config.ae_params.slow_shutter_enable) {
		pthread_mutex_lock(&G_mw_config.slow_shutter_lock);
		get_vin_frame_rate(&curr_frame_time);
		ae_belt->belt_cur = MAX(ae_cursor, ae_belt->belt_def);
		ae_belt->belt_cur = MIN(ae_belt->belt_cur, ae_belt->line_num);

		default_frame_time = G_mw_config.sensor.default_fps;
		ae_line = &G_mw_ae_lines[ae_belt->belt_cur - 1];
		if (ae_line->end.factor[0] != ae_line->start.factor[0]) {
			target_frame_time = ae_line->end.factor[0];
		} else {
			target_frame_time = ae_line->start.factor[0];
		}

		pthread_mutex_unlock(&G_mw_config.slow_shutter_lock);
		if (target_frame_time < default_frame_time) {
			target_frame_time = default_frame_time;
		}

		if (target_frame_time != curr_frame_time) {
			++transition_counter;
			if (transition_counter == HISTORY_LENGTH) {
				set_vsync_vin_framerate(target_frame_time);
				img_set_ae_loosen_belt(&ae_belt->belt_cur);
				G_mw_config.sensor.current_fps = target_frame_time;
				MW_INFO("[CHANGE]= def [%d], curr [%d], target [%d], belt [%d].\n",
					default_frame_time, curr_frame_time,
					target_frame_time, G_mw_config.cur_status.ae_belt.belt_cur);
				transition_counter = 0;
			}
		}
	} else {
		if (G_mw_config.sensor.current_fps > G_mw_config.sensor.default_fps) {
			set_vsync_vin_framerate(G_mw_config.sensor.default_fps);
			img_set_ae_loosen_belt(&ae_belt->belt_def);
		}
	}
}

void ae_control_task(void *arg)
{
	u8 ae_cursor;
	int ir_led_init_ok = -1;

	if (ir_led_is_supported()) {
		ir_led_init_ok = ir_led_init(1);
		if (ir_led_init_ok < 0) {
			MW_MSG("Failed to initiate IR LED!\n");
		}
	}

	if (ir_cut_is_supported()) {
		if (ir_cut_init(1) < 0) {
			MW_MSG("Failed to initiate IR cut!\n");
		}
	}

	while (G_mw_config.init_params.slow_shutter_active) {
		ioctl(G_mw_config.fd, IAV_IOC_WAIT_NEXT_FRAME, 0);

		img_get_ae_cursor(&ae_cursor);

		/* Start to run slow shutter control
		 */
		slow_shutter_control(ae_cursor);

		/* Start to run IR-led control
		 */
		if (ir_led_is_supported() && ir_led_init_ok >= 0) {
			ir_led_control(G_mw_config.ae_params.ir_led_mode);
		}
	}

	if (ir_cut_is_supported()) {
		if (ir_cut_init(0) < 0) {
			MW_MSG("Failed to de-initiate IR cut control GPIO!\n");
		}
	}

	if (ir_led_is_supported() && ir_led_init_ok >= 0) {
		if (ir_led_init(0) < 0) {
			MW_MSG("Failed to de-initiate IR LED control GPIO!\n");
		}
	}

	/* Exit slow shutter task, restore the sensor framerate to default.
	 */
	if (G_mw_config.ae_params.slow_shutter_enable) {
		if (G_mw_config.sensor.current_fps > G_mw_config.sensor.default_fps) {
			set_vsync_vin_framerate(G_mw_config.sensor.default_fps);
			img_set_ae_loosen_belt(&G_mw_config.cur_status.ae_belt.belt_def);
		}
	}
}

int create_ae_control_task(void)
{
	// create slow shutter thread
	G_mw_config.init_params.slow_shutter_active = 1;
	if (ae_control_task_id == 0) {
		if (pthread_create(&ae_control_task_id, NULL,
			(void *)ae_control_task, NULL) != 0) {
			MW_ERROR("Failed. Can't create thread <%s> !\n",
				MW_THREAD_NAME_AE_CTRL);
		}
		pthread_setname_np(ae_control_task_id, MW_THREAD_NAME_AE_CTRL);

		MW_INFO("Create thread <ae_control_task> successful !\n");
	}

	return 0;
}

int destroy_ae_control_task(void)
{
	G_mw_config.init_params.slow_shutter_active = 0;
	MW_MSG("Try pthread_cancel ae task \n");
	if (ae_control_task_id != 0) {
		if (pthread_join(ae_control_task_id, NULL) != 0) {
			MW_ERROR("Failed. Can't destroy thread <%s> !\n",
				MW_THREAD_NAME_AE_CTRL);
			perror("pthread_join in destory ae thread");
		}
		MW_INFO("Destroy thread <ae_control_task> successful !\n");
	}

	ae_control_task_id = 0;

	return 0;
}

int load_default_params(void)
{
	if (load_default_image_param() < 0) {
		MW_ERROR("load_default_image_param error!\n");
		return -1;
	}

	if (load_default_af_param() < 0) {
		MW_ERROR("load_default_af_param");
		return -1;
	}

	if (load_ae_exp_lines(&G_mw_config.ae_params) < 0) {
		MW_ERROR("load_ae_exp_lines");
		return -1;
	}

	return 0;
}

int check_af_params(mw_af_param *pAF)
{
	if ((pAF->fs_dist < FOCUS_MACRO || pAF->fs_dist > FOCUS_INFINITY) ||
		(pAF->fs_near < FOCUS_MACRO || pAF->fs_near > FOCUS_INFINITY) ||
		(pAF->fs_far < FOCUS_MACRO || pAF->fs_far > FOCUS_INFINITY)) {
		MW_MSG("[focus distance / focus near / focus far] :"
			" one of them is out of focus range [%d, %d] (cm)\n",
			FOCUS_MACRO, FOCUS_INFINITY);
		return -1;
	}

	if (pAF->fs_near > pAF->fs_far) {
		MW_MSG("focus near [%d] is bigger than focus far [%d]\n",
			pAF->fs_near, pAF->fs_far);
		return -1;
	}

	return 0;
}

int check_aaa_state(int fd)
{
	if (fd < 0) {
		MW_ERROR("invalid iav fd!\n");
		return -1;
	}

	if (check_state() < 0) {
		MW_MSG("====can't start aaa on idle state or unkown VIN state====\n");
		return -1;
	}

	return 0;
}

int set_chroma_noise_filter_max(int fd)
{
	return 0;
}

int load_binary_file(void)
{
	if (get_sensor_aaa_params_from_bin(&G_mw_config) < 0) {
		MW_ERROR("get_sensor_aaa_params_from_bin error!\n");
		return -1;
	}

/*	if (load_mctf_bin() < 0) {
		MW_ERROR("load_mctf_bin error!\n");
		return -1;
	}*/

	return 0;
}

static inline int load_calibration_file(void)
{
	if (load_calibration_data(&G_mw_config.cali_file, MW_CALIB_BPC) < 0) {
		MW_ERROR("Failed to load bad pixel calibration data!\n");
		return -1;
	}
	if (load_calibration_data(&G_mw_config.cali_file, MW_CALIB_LENS_SHADING) < 0) {
		MW_ERROR("Failed to load lens shading calibration data!\n");
		return -1;
	}
	if (load_calibration_data(&G_mw_config.cali_file, MW_CALIB_WB) < 0) {
		MW_ERROR("Failed to load white balance calibration data!\n");
		return -1;
	}

	return 0;
}

static int save_content_file(const char* filename, const char* content)
{
	int fd = -1;
	int ret = -1;

	fd = open(filename, O_CREAT | O_TRUNC | O_RDWR, 0666);
	if (fd < 0) {
		perror("open");
		return ret;
	}

	ret = write(fd, content, strlen(content));
	close(fd);
	fd = -1;

	return ret;
}

static void save_awb_ae_config(void)
{
	int ret = -1;
	int preload_agc = 0;
	int preload_shutter = 0;
	int preload_dgain = 0;
	char aaa_content[32];
	amba_img_dsp_wb_gain_t preload_wb_gain = {0, 0, 0, 0, 0};
	amba_img_dsp_mode_cfg_t dsp_mode_cfg;

	preload_agc = img_get_sensor_agc();
	preload_shutter = img_get_sensor_shutter();

	memset(&dsp_mode_cfg, 0, sizeof(dsp_mode_cfg));
	if (get_dsp_mode_cfg(&dsp_mode_cfg) < 0) {
		return;
	}

	amba_img_dsp_get_wb_gain(&dsp_mode_cfg, &preload_wb_gain);
	preload_dgain = preload_wb_gain.AeGain*preload_wb_gain.GlobalDGain>>12;

	/* AWB */
	memset(aaa_content, 0, sizeof(aaa_content));
	snprintf(aaa_content, sizeof(aaa_content), "%u,%d\n",
		preload_wb_gain.GainR, preload_wb_gain.GainB);

	ret = save_content_file(PRELOAD_AWB_FILE, aaa_content);
	if (ret > 0) {
		MW_DEBUG("Save AWB: r_gain=%d, b_gain=%d\n",
			preload_wb_gain.GainR, preload_wb_gain.GainB);
	}

	/* AE */
	memset(aaa_content, 0, sizeof(aaa_content));
	snprintf(aaa_content, sizeof(aaa_content), "%u,%d,%d\n",
		preload_dgain, preload_shutter, preload_agc);
	save_content_file(PRELOAD_AE_FILE, aaa_content);
	if (ret > 0) {
		MW_DEBUG("Save AE: d_gain=%d, shutter=%d, agc=%d\n",
			preload_dgain, preload_shutter, preload_agc);
	}
}

static void preload_awb_ae_config(void)
{
	int fd_ae = -1;
	int fd_awb = -1;
	int preload_agc = 0;
	int preload_shutter = 0;
	int preload_dgain = 0;
	int preload_iris_index = 0;
	amba_img_dsp_wb_gain_t preload_wb_gain = {0, 0, 0, 0, 0};
	wb_gain_t pre_wb_gain = {0, 0, 0};

	char content[32];
	char *content_c = NULL;

	fd_awb = open(PRELOAD_AWB_FILE, O_RDONLY | O_NONBLOCK);
	if (fd_awb >= 0) {
		memset(content, 0, sizeof(content));
		read(fd_awb, content, sizeof(content));
		close(fd_awb);
		fd_awb = -1;

		content_c = strtok(content, ",");
		if (content_c) {
			preload_wb_gain.GainR = atoi(content_c);
		}
		content_c = strtok(NULL, ",");
		if (content_c) {
			preload_wb_gain.GainB = atoi(content_c);
		}

		MW_DEBUG("Preload AWB: r_gain=%d, b_gain=%d\n",
			preload_wb_gain.GainR, preload_wb_gain.GainB);

		pre_wb_gain.r_gain = preload_wb_gain.GainR >> 2;
		pre_wb_gain.b_gain = preload_wb_gain.GainB >> 2;
		pre_wb_gain.g_gain = 1024;
		adj_pre_config();
		awb_flow_pre_config(&pre_wb_gain);
	}

	fd_ae = open(PRELOAD_AE_FILE, O_RDONLY | O_NONBLOCK);
	if (fd_ae >= 0) {
		memset(content, 0, sizeof(content));
		read(fd_ae, content, sizeof(content));
		close(fd_ae);
		fd_ae = -1;

		content_c = strtok(content, ",");
		if (content_c) {
			 preload_dgain = atoi(content_c);
		}
		content_c = strtok(NULL, ",");
		if (content_c) {
			 preload_shutter = atoi(content_c);
		}
		content_c = strtok(NULL, ",");
		if (content_c) {
			preload_agc = atoi(content_c);
		}

		MW_DEBUG("Preload AE: d_gain=%d, shutter=%d, agc=%d\n",
			preload_dgain, preload_shutter, preload_agc);
		ae_flow_pre_config(preload_agc, preload_shutter, preload_dgain,
			preload_iris_index);
	}
}

int do_prepare_aaa(void)
{
	img_config_info_t img_config;
	struct vindev_aaa_info *vin_aaa_info = NULL;

	if (G_mw_config.init_params.aaa_active == 1) {
		MW_MSG("aaa is working!\n");
		return 0;
	}

	memset(&G_mw_config.res, 0, sizeof(mw_sys_res));
	memset(&G_mw_config.mw_iav, 0, sizeof(_mw_pri_iav));

	if (get_iav_cfg(&G_mw_config) < 0) {
		MW_ERROR("get_system_resource error!\n");
		return -1;
	}
	if (get_sensor_aaa_params(&G_mw_config) < 0) {
		MW_ERROR("get_sensor_aaa_params error!\n");
		return -1;
	}
	memset(&img_config, 0, sizeof(img_config_info_t));
	img_config.isp_pipeline = G_mw_config.res.isp_pipeline;
	img_config.hdr_config.pipeline = G_mw_config.res.hdr_mode;
	img_config.hdr_config.expo_num = G_mw_config.res.hdr_expo_num;
	img_config.main_width = G_mw_config.mw_iav.main_width;
	img_config.main_height = G_mw_config.mw_iav.main_height;
	img_config.raw_width = G_mw_config.sensor.raw_width;
	img_config.raw_height = G_mw_config.sensor.raw_height;
	img_config.raw_pitch = G_mw_config.res.raw_pitch;
	img_config.raw_resolution = G_mw_config.sensor.raw_resolution;

	if (G_mw_config.sensor.is_rgb_sensor) {

		vin_aaa_info = &G_mw_config.sensor.vin_aaa_info;
		img_config.raw_bayer = vin_aaa_info->bayer_pattern;

		/* Preload 3A param from /tmp when start process, Just support LISO now */
		if (img_config.isp_pipeline == ISP_PIPELINE_LISO) {
			preload_awb_ae_config();
		}

		if (vin_aaa_info->dual_gain_mode) {
			img_config.hdr_config.method = HDR_DUAL_GAIN_METHOD;
		} else {
			switch (vin_aaa_info->hdr_mode) {
			case AMBA_VIDEO_LINEAR_MODE:
				img_config.hdr_config.method = HDR_NONE_METHOD;
				break;
			case AMBA_VIDEO_2X_HDR_MODE:
			case AMBA_VIDEO_3X_HDR_MODE:
			case AMBA_VIDEO_4X_HDR_MODE:
				if (vin_aaa_info->sensor_id == SENSOR_AR0230) {
					img_config.hdr_config.method = HDR_RATIO_FIX_LINE_METHOD;
				} else {
					img_config.hdr_config.method = HDR_RATIO_VARY_LINE_METHOD;
				}
				break;
			case AMBA_VIDEO_INT_HDR_MODE:
				img_config.hdr_config.method = HDR_BUILD_IN_METHOD;
				if (vin_aaa_info->sensor_id == SENSOR_AR0230) {
					G_mw_config.ae_params.shutter_time_min = SHUTTER_1BY480_SEC;
				}
				break;
			default :
				printf("Error: Invalid VIN HDR sensor info.\n");
				return -1;
				break;
			}
		}
	}

	MW_MSG("pipline:%d, hdr_mode:%d, expo_num:%d, raw_pitch:%d\n",
		img_config.isp_pipeline, img_config.hdr_config.pipeline,
		img_config.hdr_config.expo_num, img_config.raw_pitch);

	MW_MSG("main:size %dx%d, raw:%dx%d, resolution:%d\n", img_config.main_width,
		img_config.main_height, img_config.raw_width,  img_config.raw_height,
		img_config.raw_resolution);

	if (img_lib_init(G_mw_config.fd, img_config.defblc_enable,
		img_config.sharpen_b_enable) < 0) {
		MW_ERROR("img_lib_init!\n");
		return -1;
	}
	if (img_config_pipeline(G_mw_config.fd, &img_config) < 0) {
		MW_ERROR("img_config_pipeline error!\n");
		return -1;
	}

	if (G_mw_config.sensor.is_rgb_sensor) {
		if (load_binary_file() < 0) {
			MW_ERROR("load_binary_file error!\n");
			return -1;
		}
	}

	if (img_prepare_isp(G_mw_config.fd)< 0 ) {
		MW_ERROR("img_prepare_isp fail\n");
		return -1;
	}
	return 0;
}

int do_start_aaa(void)
{
	if (G_mw_config.init_params.aaa_active == 1) {
		MW_MSG(" aaa is working!\n");
		return 0;
	}

	if (!G_mw_config.sensor.is_rgb_sensor) {
		G_mw_config.init_params.aaa_active = 1;
		return 0;
	}

	if (check_aaa_state(G_mw_config.fd) < 0) {
		MW_MSG("check_aaa_state error\n");
		return -1;
	}

	if (img_start_aaa(G_mw_config.fd) < 0) {
		MW_ERROR("start_aaa error!\n");
		return -1;
	}
	if (img_set_work_mode(0)) {
		MW_ERROR("img_set_work_mode error!\n");
		return -1;
	}

	if (load_calibration_file() < 0) {
		MW_ERROR("load_calibration_file error!\n");
		return -1;
	}

	if (set_chroma_noise_filter_max(G_mw_config.fd) < 0) {
		MW_ERROR("set_chroma_mois_filter_max error!\n");
		return -1;
	}

	/*init mutex G_mw_config.slow_shutter_lock before loading AE lines and creating slow shutter control thread*/
	if (pthread_mutex_init(&G_mw_config.slow_shutter_lock, NULL) != 0) {
		MW_ERROR("pthread_mutex_init error!\n");
		return -1;
	}

	if (G_mw_config.init_params.reload_flag ||
		G_mw_config.init_params.load_cfg_file) {
		if (reload_previous_params() < 0) {
			MW_ERROR("reload_previous_params error!\n");
			return -1;
		}
	} else {
		if (load_default_params() < 0) {
			MW_ERROR("load_default_params error!\n");
			return -1;
		}
		G_mw_config.init_params.reload_flag = 1;
	}
	create_ae_control_task();

	G_mw_config.init_params.aaa_active = 1;
	signal_sem();
	return 0;
}

int do_stop_aaa(void)
{
	if (G_mw_config.init_params.aaa_active == 0) {
		MW_MSG(" aaa has stopped!\n");
		return 0;
	}

	if (G_mw_config.sensor.is_rgb_sensor) {
		/* Save 3A param to /tmp when start process */
		if (G_mw_config.res.isp_pipeline == ISP_PIPELINE_LISO) {
			save_awb_ae_config();
		}
		if (G_mw_config.ae_params.slow_shutter_enable) {
			destroy_ae_control_task();
		}
		/*destroy mutex G_mw_config.slow_shutter_lock after destroying slow shutter control thread*/
		pthread_mutex_destroy(&G_mw_config.slow_shutter_lock);
		if (img_stop_aaa() < 0) {
			MW_ERROR("stop aaa error!\n");
			return -1;
		}
		usleep(1000);
	}

	if (img_lib_deinit() < 0) {
		MW_ERROR("img_lib_deinit error!\n");
		return -1;
	}

	G_mw_config.sensor.load_files.adj_file[0]= '\0';

	G_mw_config.init_params.aaa_active = 0;
	return 0;
}

#define __END_OF_FILE__

