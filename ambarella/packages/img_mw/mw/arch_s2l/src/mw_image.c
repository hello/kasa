/*
 *
 * mw_image.c
 *
 * History:
 *	2010/02/28 - [Jian Tang] Created this file
 *	2011/06/20 - [Jian Tang] Modified this file
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
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <basetypes.h>

#include "iav_common.h"
#include "iav_ioctl.h"
#include "iav_vin_ioctl.h"
#include "iav_vout_ioctl.h"
#include "ambas_imgproc_arch.h"

#include "img_adv_struct_arch.h"
#include "img_api_adv_arch.h"
#include "img_dev.h"

#include "mw_aaa_params.h"
#include "mw_api.h"
#include "mw_pri_struct.h"
#include "mw_image_priv.h"
#include "mw_ir_led.h"
#include "mw_dc_iris.h"

_mw_global_config G_mw_config = {
	.fd = -1,
	.init_params = {
		.mw_enable           = 0,
		.is_rgb_sensor       = 0,
		.nl_enable           = 0,
		.reload_flag         = 0,
		.aaa_active          = 0,
		.slow_shutter_active = 0,
		.load_cfg_file       = 0,
		.wait_3a             = 0,
	},
	.sensor = {
		.load_files = {
			.adj_file = "",
			.aeb_file = "",
			.lens_file = "",
		},
		.lens_id = LENS_CMOUNT_ID,
		.sensor_slow_shutter = 0,
	},
	.ae_params = {
		.anti_flicker_mode    = MW_ANTI_FLICKER_60HZ,
		.shutter_time_min     = SHUTTER_1BY16000_SEC,
		.shutter_time_max     = SHUTTER_1BY60_SEC,
		.sensor_gain_max      = ISO_6400,
		.slow_shutter_enable  = 0,
		.ir_led_mode          = MW_IR_LED_MODE_AUTO,
		.current_vin_fps      = AMBA_VIDEO_FPS_60,
		.ae_metering_mode     = MW_AE_CENTER_METERING,
		.ae_metering_table    = {
			.metering_weight  = {			//AE_METER_CENTER
				1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 2, 3, 3, 2, 1, 1, 1, 1,
				1, 1, 1, 2, 3, 5, 5, 3, 2, 1, 1, 1,
				1, 1, 1, 2, 3, 5, 5, 3, 2, 1, 1, 1,
				1, 1, 2, 3, 4, 5, 5, 4, 3, 2, 1, 1,
				1, 2, 3, 4, 4, 4, 4, 4, 4, 3, 2, 1,
				2, 3, 4, 4, 4, 4, 4, 4, 4, 4, 3, 2,
			},
		},
		.ae_level             = {
		100,
		},
		.tone_curve_duration    = MAX_TONE_CURVE_DURATION,
	},
	.iris_params = {
		.iris_type      = 0,
		.iris_enable    = 0,
		.aperture_min   = APERTURE_AUTO,
		.aperture_max   = APERTURE_AUTO,
	},
	.image_params = {
		.saturation    = 64,
		.brightness    = 0,
		.hue           = 0,
		.contrast      = 64,
		.sharpness     = 6,
	},
	.awb_params = {
		.enable        = 1,
		.wb_mode       = MW_WB_AUTO,
		.wb_method     = MW_WB_NORMAL_METHOD,
	},
	.enh_params = {
		.auto_contrast    = 0,
		.auto_wdr_str     = 0,
		.dn_mode          = 0,
		.mctf_strength    = 64,
		.sharpen_strength = 0,
		.chroma_noise_str = 64,
		.local_expo_mode  = 0,
		.bl_compensation  = 0,
		.le_off_curve     = {
			.gain_curve_table = {
				1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
				1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
				1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
				1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
				1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
				1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
				1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
				1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
				1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
				1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
				1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
				1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
				1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
				1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
				1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
				1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024
			},
		},
		.auto_knee_strength = 100,
		.auto_dump_cfg = 0,
	},
};

/*************************************************
 *
 *		Public Functions, for external use
 *
 *************************************************/

int mw_load_calibration_file(mw_cali_file *pFile)
{
	if (pFile == NULL) {
		MW_ERROR("mw_load_calibration_file pointer is NULL!\n");
		return -1;
	}
	memset(&G_mw_config.cali_file, 0, sizeof(G_mw_config.cali_file));
	if (strlen(pFile->bad_pixel) > 0) {
		strncpy(G_mw_config.cali_file.bad_pixel, pFile->bad_pixel,
			sizeof(G_mw_config.cali_file.bad_pixel));
	}
	if (strlen(pFile->wb) > 0) {
		strncpy(G_mw_config.cali_file.wb, pFile->wb,
			sizeof(G_mw_config.cali_file.wb));
	}
	if (strlen(pFile->shading) > 0) {
		strncpy(G_mw_config.cali_file.shading, pFile->shading,
			sizeof(G_mw_config.cali_file.shading));
	}
	return 0;
}

int mw_set_sharpen_filter(int sharpen)
{
	return 0;
}

int mw_get_sharpen_filter(void)
{
	return 0;
}


int mw_get_iav_state(int fd, int *pState)
{
	u32 info;

	if (ioctl(fd, IAV_IOC_GET_IAV_STATE, &info) < 0) {
		perror("IAV_IOC_GET_IAV_STATE");
		return -1;
	}

	*pState = info;
	return 0;
}

int mw_get_sys_info(mw_sys_info *pMw_sys_info)
{
	if (pMw_sys_info == NULL) {
		MW_ERROR("The point is null\n\n");
		return -1;
	}

	memcpy(&(pMw_sys_info->res), &G_mw_config.res, sizeof(mw_sys_res));
	pMw_sys_info->sensor.step = G_mw_config.sensor.vin_aaa_info.agc_step;
	return 0;
}

int mw_start_aaa(int fd)
{
	int retry = 3;
	if (G_mw_config.init_params.mw_enable == 1) {
		MW_MSG("Already start!\n");
		return MW_AAA_SUCCESS;
	}

	G_mw_config.fd = fd;

	if (init_netlink() < 0) {
		MW_ERROR("init_netlink \n");
		return MW_AAA_NL_INIT_FAIL;
	}

	G_mw_config.init_params.mw_enable = 1;
	G_mw_config.init_params.wait_3a = 1;
	while (G_mw_config.init_params.wait_3a) {
		if (0 != wait_sem(0)) {
			MW_ERROR("can't wait sem from netlink loop\n");
			--retry;
			if (retry == 0) {
				MW_ERROR("reach max retry times, break loop\n");
				break;
			}
		} else {
			break;
		}
	}
	G_mw_config.init_params.wait_3a = 0;

	MW_MSG("======= [DONE] mw_start_aaa ======= \n");
	if (!G_mw_config.init_params.aaa_active) {
		return MW_AAA_INTERRUPTED;
	}
	return MW_AAA_SUCCESS;
}

int mw_stop_aaa(void)
{
	int ret = 0;
	if (G_mw_config.init_params.mw_enable == 0) {
		MW_INFO("3A is not enabled. Quit silently.\n");
		return MW_AAA_SUCCESS;
	}

	if (G_mw_config.init_params.wait_3a) {
		G_mw_config.init_params.wait_3a = 0;
		signal_sem();
	}

	if ((ret = send_image_msg_stop_aaa()) < 0) {
		MW_ERROR("stop_aaa_task\n");
	}
	deinit_netlink();
	G_mw_config.init_params.mw_enable = 0;

	MW_MSG("======= [DONE] mw_stop_aaa ======= \n");

	if (ret < 0) {
		return MW_AAA_SET_FAIL;
	}

	return MW_AAA_SUCCESS;
}

int mw_init_mctf(int fd)
{
	if (G_mw_config.init_params.mw_enable == 1) {
		MW_MSG("Already start!\n");
		return -1;
	}
	G_mw_config.fd = fd;

	if (check_aaa_state(G_mw_config.fd) < 0) {
		MW_MSG("check_aaa_state error\n");
		return -1;
	}
	if (img_lib_init(G_mw_config.fd, 0, 0) < 0) {
		MW_ERROR("/dev/iav device!\n");
		return -1;
	}
	if (load_binary_file() < 0) {
		MW_ERROR("mw_load_mctf error!\n");
		return -1;
	}
	if (img_lib_deinit() < 0) {
		MW_ERROR("img_lib_deinit error!\n");
		return -1;
	}
	MW_MSG("======= [DONE] init mctf ======= \n");

	return 0;
}

int mw_get_af_param(mw_af_param *pAF)
{
	if (pAF == NULL) {
		MW_MSG("mw_get_af_param pointer is NULL!\n");
		return -1;
	}
	pAF->lens_type = G_mw_config.af_info.lens_type;
	pAF->af_mode = G_mw_config.af_info.af_mode;
	pAF->af_tile_mode = G_mw_config.af_info.af_tile_mode;
	pAF->zm_dist = G_mw_config.af_info.zm_dist;
	if (G_mw_config.af_info.fs_dist == 0)
		G_mw_config.af_info.fs_dist = 1;
	pAF->fs_dist = FOCUS_INFINITY / G_mw_config.af_info.fs_dist;
	if (G_mw_config.af_info.fs_near == 0)
		G_mw_config.af_info.fs_near = 1;
	pAF->fs_near = FOCUS_INFINITY / G_mw_config.af_info.fs_near;
	if (G_mw_config.af_info.fs_far == 0)
		G_mw_config.af_info.fs_far = 1;
	pAF->fs_far = FOCUS_INFINITY / G_mw_config.af_info.fs_far;
	return 0;
}

int mw_set_af_param(mw_af_param *pAF)
{
	static af_range_t af_range;
	int focus_idx = 0;

	if (pAF == NULL) {
		MW_MSG("mw_set_af_param pointer is NULL!\n");
		return -1;
	}
	if (G_mw_config.sensor.is_rgb_sensor == 0) {
		return 0;
	}

	// fixed lens ID, disable all af function
	G_mw_config.af_info.lens_type = pAF->lens_type;
	if (G_mw_config.af_info.lens_type == LENS_CMOUNT_ID) {
		return 0;
	} else {
		if (img_config_lens_info(G_mw_config.af_info.lens_type) < 0) {
			MW_ERROR("img_config_lens_info error!\n");
			return -1;
		}
	}

	if (check_af_params(pAF) < 0) {
		MW_ERROR("check_af_params error\n");
		return -1;
	}

	// set af mode (CAF, SAF, MF, CALIB)
	if (G_mw_config.af_info.af_mode != pAF->af_mode) {
		if (img_af_set_mode(pAF->af_mode) < 0) {
			MW_ERROR("img_af_set_mode error\n");
			return -1;
		}
		G_mw_config.af_info.af_mode = pAF->af_mode;
	}

	// set af tile mode
	if (G_mw_config.af_info.af_tile_mode != pAF->af_tile_mode) {
		G_mw_config.af_info.af_tile_mode = pAF->af_tile_mode;
	}

	// set zoom index
	if (G_mw_config.af_info.zm_dist != pAF->zm_dist) {
		if (img_af_set_zoom_idx(pAF->zm_dist) < 0) {
			MW_ERROR("img_af_set_zoom_idx error\n");
			return -1;
		}
		G_mw_config.af_info.zm_dist = pAF->zm_dist;
	}

	// set focus index
	if (pAF->af_mode == MANUAL) {
		focus_idx = FOCUS_INFINITY / pAF->fs_dist;
		if (G_mw_config.af_info.fs_dist != focus_idx) {
			if (img_af_set_focus_idx(focus_idx) < 0) {
				MW_ERROR("img_af_set_focus_idx error\n");
				return -1;
			}
			G_mw_config.af_info.fs_dist = focus_idx;
		}
	} else {
		af_range.near_bd = FOCUS_INFINITY / pAF->fs_near;
		af_range.far_bd = FOCUS_INFINITY / pAF->fs_far;
		if ((G_mw_config.af_info.fs_near != af_range.near_bd) ||
			(G_mw_config.af_info.fs_far != af_range.far_bd)) {
			if (img_af_set_range(&af_range) < 0) {
				MW_ERROR("img_af_set_range error\n");
				return -1;
			}
			G_mw_config.af_info.fs_near = af_range.near_bd;
			G_mw_config.af_info.fs_far = af_range.far_bd;
		}
	}
	MW_MSG("[DONE] mw_set_af_param");

	return 0;
}

int mw_get_image_statistics(mw_image_stat_info *pInfo)
{
	if (pInfo == NULL) {
		MW_MSG("mw_get_image_statistics pointer is NULL!\n");
		return -1;
	}

	MW_MSG("Not support\n");

	return 0;
}

/* ***************************************************************
	Exposure Control Settings
******************************************************************/

int mw_enable_ae(int enable)
{
	img_enable_ae(enable);
	if (enable) {
		if (load_ae_exp_lines(&G_mw_config.ae_params) < 0) {
			MW_ERROR("load ae exp lines error!\n");
			return -1;
		}
	}

	return 0;
}

int mw_set_ae_param(mw_ae_param *pAe_param)
{
	u64 hdr_shutter_max = 0;
	hdr_blend_info_t hdr_blend_cfg;

	if (pAe_param == NULL) {
		MW_ERROR("mw_set_ae_param pointer is NULL!\n");
		return -1;
	}
	if (G_mw_config.sensor.is_rgb_sensor == 0) {
		return 0;
	}

	if ((G_mw_config.sensor.lens_id != LENS_CMOUNT_ID) &&
		(strcmp(G_mw_config.sensor.load_files.lens_file, "") != 0)) {
		if ((pAe_param->lens_aperture.aperture_min <
			G_mw_config.ae_params.lens_aperture.FNO_min) ||
			(pAe_param->lens_aperture.aperture_max >
			G_mw_config.ae_params.lens_aperture.FNO_max) ||
			(pAe_param->lens_aperture.aperture_min >
			pAe_param->lens_aperture.aperture_max)) {
			MW_ERROR("The params are not correct, use the default value!\n");
			pAe_param->lens_aperture.aperture_min =
				G_mw_config.ae_params.lens_aperture.FNO_min;
			pAe_param->lens_aperture.aperture_max =
				G_mw_config.ae_params.lens_aperture.FNO_max;
		}
	}

	if (0 == memcmp(&G_mw_config.ae_params, pAe_param,
		sizeof(G_mw_config.ae_params))) {
		return 0;
	}

	if (G_mw_config.ae_params.current_vin_fps != pAe_param->current_vin_fps) {
		if (pAe_param->slow_shutter_enable) {
			if (pAe_param->current_vin_fps != 0) {
				G_mw_config.sensor.current_fps = pAe_param->current_vin_fps;
			} else {
				pAe_param->current_vin_fps = G_mw_config.ae_params.current_vin_fps;
			}
		} else {
			/* If slow shutter task is disabled, sensor frame rate will keep
			 * the "initial" value when 3A process starts. Any attempt to
			 * change sensor frame rate will be restored to "initial" value.
			 */

			if (pAe_param->shutter_time_max > G_mw_config.ae_params.current_vin_fps) {
				pAe_param->shutter_time_max = G_mw_config.ae_params.current_vin_fps;
			}
			pAe_param->current_vin_fps = G_mw_config.ae_params.current_vin_fps;
		}
	}
	if (pAe_param->slow_shutter_enable && (G_mw_config.res.hdr_expo_num > HDR_1X)) {
		MW_ERROR("Not support slow shutter in hdr mode\n");
		return -1;
	}

	if (G_mw_config.res.hdr_expo_num == HDR_2X) {
		if ((pAe_param->tone_curve_duration > MAX_TONE_CURVE_DURATION) ||
			(pAe_param->tone_curve_duration == 0)) {
			MW_ERROR("Only support tone curver in 2X hdr mode with duration [1~%d]\n",
				MAX_TONE_CURVE_DURATION);
			return -1;
		}
	}

	if (G_mw_config.res.hdr_expo_num == HDR_2X) {
		img_get_hdr_blend_config(&hdr_blend_cfg);
		hdr_shutter_max = HDR_2X_MAX_SHUTTER((u64)pAe_param->current_vin_fps, (u64)hdr_blend_cfg.expo_ratio);
		if (pAe_param->shutter_time_max > hdr_shutter_max) {
			MW_ERROR("Shutter time max [1/%d s] is longer than HDR shutter limit "
				"[1/%llu s]. Ignore this change!\n",
				SHT_TIME(pAe_param->shutter_time_max),
				SHT_TIME(hdr_shutter_max));
			return -1;
		}
		if (pAe_param->shutter_time_min > hdr_shutter_max) {
			MW_ERROR("Shutter time min [1/%d s] is longer than HDR shutter limit "
				"[1/%llu s]. Ignore this change!\n",
				SHT_TIME(pAe_param->shutter_time_min),
				SHT_TIME(hdr_shutter_max));
			return -1;
		}
	} else if (G_mw_config.res.hdr_expo_num == HDR_3X) {
		img_get_hdr_blend_config(&hdr_blend_cfg);
		hdr_shutter_max = HDR_3X_MAX_SHUTTER((u64)pAe_param->current_vin_fps, (u64)hdr_blend_cfg.expo_ratio);
		if (pAe_param->shutter_time_max > hdr_shutter_max) {
			MW_ERROR("Shutter time max [1/%d s] is longer than HDR shutter limit "
				"[1/%llu s]. Ignore this change!\n",
				SHT_TIME(pAe_param->shutter_time_max),
				SHT_TIME(hdr_shutter_max));
			return -1;
		}
		if (pAe_param->shutter_time_min > hdr_shutter_max) {
			MW_ERROR("Shutter time min [1/%d s] is longer than HDR shutter limit "
				"[1/%llu s]. Ignore this change!\n",
				SHT_TIME(pAe_param->shutter_time_min),
				SHT_TIME(hdr_shutter_max));
			return -1;
		}
	}

	if (pAe_param->shutter_time_min > pAe_param->current_vin_fps) {
		MW_ERROR("Shutter time min [1/%d s] is longer than VIN frametime [1/%d s]."
			" Ignore this change!\n", SHT_TIME(pAe_param->shutter_time_min),
			SHT_TIME(pAe_param->current_vin_fps));
		return -1;
	}
	//Disable and re-enable dc iris when loading ae line.
	if (G_mw_config.iris_params.iris_enable) {
		if (disable_dc_iris() < 0) {
			MW_ERROR("dc_iris_enable error\n");
			return -1;
		}
	}

	if (load_ae_exp_lines(pAe_param) < 0) {
		MW_ERROR("load_ae_exp_lines error\n");
		return -1;
	}
	G_mw_config.ae_params = *pAe_param;

	if (G_mw_config.iris_params.iris_enable) {
		if (enable_dc_iris() < 0) {
			MW_ERROR("dc_iris_enable error\n");
			return -1;
		}
	}

	MW_DEBUG(" === Exposure mode          [%d]\n", pAe_param->anti_flicker_mode);
	MW_DEBUG(" === Shutter time min       [%d]\n", pAe_param->shutter_time_min);
	MW_DEBUG(" === Shutter time max       [%d]\n", pAe_param->shutter_time_max);
	MW_DEBUG(" === Slow shutter enable    [%d]\n", pAe_param->slow_shutter_enable);
	MW_DEBUG(" === Max sensor gain        [%d]\n", pAe_param->sensor_gain_max);

	return 0;
}

int mw_get_ae_param(mw_ae_param *pAe_param)
{
	*pAe_param = G_mw_config.ae_params;
	return 0;
}

int mw_set_exposure_level(int *pExposure_level)
{
	u16 ae_target[MAX_HDR_EXPOSURE_NUM];
	int i = 0;
	int is_same = 0;
	if (pExposure_level == NULL) {
		MW_ERROR("mw_set_exposure_level pointer is NULL\n");
		return -1;
	}

	for (i = 0; i < G_mw_config.res.hdr_expo_num; i++) {
		if (pExposure_level[i] < MW_EXPOSURE_LEVEL_MIN) {
			pExposure_level[i] = MW_EXPOSURE_LEVEL_MIN;
		}
		if (pExposure_level[i] > MW_EXPOSURE_LEVEL_MAX - 1) {
			pExposure_level[i] = MW_EXPOSURE_LEVEL_MAX - 1;
		}
		ae_target[i] = (u16)((pExposure_level[i] << 10) / 100);
		if (G_mw_config.ae_params.ae_level[i] == pExposure_level[i]) {
			is_same++;
			continue;
		}
		G_mw_config.ae_params.ae_level[i] = pExposure_level[i];
	}
	if (is_same == G_mw_config.res.hdr_expo_num) {
		MW_DEBUG("The value is same!\n");
		return 0;
	}
	if (img_set_ae_target_ratio(ae_target) < 0) {
		MW_ERROR("img_set_ae_target_ratio failed!\n");
		return -1;
	}

	return 0;
}

int mw_get_exposure_level(int *pExposure_level)
{
	u16 target_ratio;

	if (pExposure_level == NULL) {
		MW_ERROR("mw_get_exposure_level pointer is NULL\n");
		return -1;
	}
	img_get_ae_target_ratio(&target_ratio);
	*pExposure_level = (target_ratio * 100) >> 10;
	G_mw_config.ae_params.ae_level[0] = *pExposure_level;

	return 0;
}

int mw_set_ae_metering_mode(mw_ae_metering_mode mode)
{
	u8 metering_mode[MAX_HDR_EXPOSURE_NUM];
	if (G_mw_config.res.hdr_expo_num > MIN_HDR_EXPOSURE_NUM) {
		MW_ERROR("Not support in HDR mode\n");
		return -1;
	}
	metering_mode[0] = mode;
	if (img_set_ae_meter_mode(metering_mode) < 0) {
		MW_ERROR("img_ae_set_meter_mode error\n");
		return -1;
	}
	G_mw_config.ae_params.ae_metering_mode = metering_mode[0];
	return 0;
}

int mw_get_ae_metering_mode(mw_ae_metering_mode *pMode)
{
	*pMode = G_mw_config.ae_params.ae_metering_mode;
	return 0;
}

int mw_set_ae_metering_table(mw_ae_metering_table *pAe_metering_table)
{
	G_mw_config.ae_params.ae_metering_table = *pAe_metering_table;
	img_set_ae_meter_roi(&G_mw_config.ae_params.ae_metering_table.metering_weight[0]);
	return 0;
}

int mw_get_ae_metering_table(mw_ae_metering_table *pAe_metering_table)
{
	*pAe_metering_table = G_mw_config.ae_params.ae_metering_table;
	return 0;
}

int mw_set_shutter_time(int fd, int *shutter_time)
{
	struct vindev_shutter vsrc_shutter;
	hdr_sensor_gp_t sht_agc_gp;
	int i;

	if (shutter_time == NULL) {
		MW_ERROR("mw_set_shutter_time pointer is NULL!\n");
		return -1;
	}

	if (G_mw_config.res.hdr_expo_num > MIN_HDR_EXPOSURE_NUM) {
		img_get_hdr_sensor_agc_group(fd, &sht_agc_gp.agc_gp);

		for (i = 0; i < G_mw_config.res.hdr_expo_num; ++i) {
			shutter_time[i] = shutter_time[i] /
				G_mw_config.sensor.vin_aaa_info.line_time;
		}
		memcpy(&sht_agc_gp.shutter_gp, shutter_time,
			G_mw_config.res.hdr_expo_num * sizeof(u32));
		img_set_hdr_sensor_shutter_agc_group(fd, &sht_agc_gp);
	} else {
		vsrc_shutter.vsrc_id = 0;
		vsrc_shutter.shutter = *shutter_time;
		if (ioctl(fd, IAV_IOC_VIN_SET_SHUTTER, &vsrc_shutter) < 0) {
			perror("IAV_IOC_VIN_SET_SHUTTER");
			return -1;
		}
	}
	return 0;
}

int mw_get_shutter_time(int fd, int *pShutter_time)
{
	struct vindev_shutter vsrc_shutter;
	hdr_shutter_gp_t vsrc_sht_gp;
	int i;

	if (pShutter_time == NULL) {
		MW_ERROR("mw_get_shutter_time pointer is NULL!\n");
		return -1;
	}

	if (G_mw_config.res.hdr_expo_num > MIN_HDR_EXPOSURE_NUM) {
		if (img_get_hdr_sensor_shutter_group(fd, &vsrc_sht_gp) < 0) {
			MW_ERROR("img_get_hdr_sensor_shutter_group \n");
			return -1;
		}
		memcpy(pShutter_time, &vsrc_sht_gp.shutter_info,
			G_mw_config.res.hdr_expo_num * sizeof(u32));

		for (i = 0; i < G_mw_config.res.hdr_expo_num; ++i) {
			pShutter_time[i] = pShutter_time[i] *
				G_mw_config.sensor.vin_aaa_info.line_time;
		}
	} else {
		vsrc_shutter.vsrc_id = 0;
		if (ioctl(fd, IAV_IOC_VIN_GET_SHUTTER, &vsrc_shutter) < 0) {
			perror("IAV_IOC_VIN_GET_SHUTTER");
			return -1;
		}
		*pShutter_time = vsrc_shutter.shutter;
	}

	if (pShutter_time[0] == 0) {
		MW_ERROR("AE is not ready or get incorrect value!\n");
		return -1;
	}

	return 0;
}

int mw_set_sensor_gain(int fd, int *pGain_db)
{
	struct vindev_agc vsrc_agc;
	hdr_sensor_gp_t sht_agc_gp;

	if (G_mw_config.res.hdr_expo_num > MIN_HDR_EXPOSURE_NUM) {
		img_get_hdr_sensor_shutter_group(fd, &sht_agc_gp.shutter_gp);
		memcpy(&sht_agc_gp.agc_gp, pGain_db,
			G_mw_config.res.hdr_expo_num * sizeof(u32));
		img_set_hdr_sensor_shutter_agc_group(fd, &sht_agc_gp);
	} else {
		vsrc_agc.vsrc_id = 0;
		vsrc_agc.agc = *pGain_db;
		if (ioctl(fd, IAV_IOC_VIN_SET_AGC, &vsrc_agc) < 0) {
			perror("IAV_IOC_VIN_SET_AGC");
			return -1;
		}

	}
	return 0;
}

int mw_get_sensor_gain(int fd, int *pGain_db)
{
	struct vindev_agc vsrc_agc;
	struct vindev_wdr_gp_s vsrc_ag_gp, vsrc_dg_gp;
	int i;

	if (pGain_db == NULL) {
		MW_ERROR("mw_get_sensor_gain pointer is NULL!\n");
		return -1;
	}
	if (G_mw_config.res.hdr_expo_num > MIN_HDR_EXPOSURE_NUM) {
		vsrc_ag_gp.vsrc_id = 0;
		if (ioctl(fd, IAV_IOC_VIN_GET_WDR_AGAIN_INDEX_GP, &vsrc_ag_gp)<0) {
			perror("IAV_IOC_VIN_GET_WDR_AGAIN_INDEX_GP");
			return -1;
		}
		vsrc_dg_gp.vsrc_id = 0;
		if (ioctl(fd, IAV_IOC_VIN_GET_WDR_DGAIN_INDEX_GP, &vsrc_dg_gp) < 0) {
			perror("IAV_IOC_VIN_GET_WDR_DGAIN_INDEX_GP");
			return -1;
		}

		for (i = 0; i < G_mw_config.res.hdr_expo_num; i++) {
			switch (i) {
				case 0:
					*pGain_db = vsrc_ag_gp.l + vsrc_dg_gp.l;//add digital gain
					MW_DEBUG("Gain:[%d], Again:%d, Dgain: %d\t", i,
						vsrc_ag_gp.l, vsrc_dg_gp.l);
					break;
				case 1:
					*(pGain_db + 1) = vsrc_ag_gp.s1 + vsrc_dg_gp.s1;//add digital gain
					MW_DEBUG("Gain:[%d], Again:%d, Dgain: %d\t", i,
						vsrc_ag_gp.s1, vsrc_dg_gp.s1);
					break;
				case 2:
					*(pGain_db + 2) = vsrc_ag_gp.s2 + vsrc_dg_gp.s2;//add digital gain
					MW_DEBUG("Gain:[%d], Again:%d, Dgain: %d\t", i,
						vsrc_ag_gp.s2, vsrc_dg_gp.s2);
					break;
				case 3:
					*(pGain_db + 3) += vsrc_ag_gp.s3 + vsrc_dg_gp.s3;//add digital gain
					MW_DEBUG("Gain:[%d], Again:%d, Dgain: %d\t", i,
						vsrc_ag_gp.s3, vsrc_dg_gp.s3);
					break;
				default:
					break;
			}
			MW_DEBUG("\n");
		}
	} else {
		get_sensor_agc_info(fd, &vsrc_agc);
		*pGain_db = vsrc_agc.agc;
	}

	return 0;
}

int mw_is_dc_iris_supported(void)
{
	int supported = dc_iris_is_supported();

	return supported;
}

int mw_enable_dc_iris_control(int enable)
{
	if (enable) {
		if (enable_dc_iris() < 0) {
			MW_ERROR("dc_iris_enable error\n");
			return -1;
		}
	} else {
		if (disable_dc_iris() < 0) {
			MW_ERROR("dc_iris_enable error\n");
			return -1;
		}
	}

	G_mw_config.iris_params.iris_enable = enable;
	return 0;
}

int mw_set_dc_iris_pid_coef(mw_dc_iris_pid_coef * pPid_coef)
{

	if (dc_iris_set_pid_coef(pPid_coef) < 0) {
		MW_ERROR("dc_iris_set_pid_coef error\n");
		return -1;
	}
	return 0;
}

int mw_get_dc_iris_pid_coef(mw_dc_iris_pid_coef * pPid_coef)
{
	if (dc_iris_get_pid_coef(pPid_coef) < 0) {
		MW_ERROR("dc_iris_get_pid_coef error\n");
		return -1;
	}
	return 0;
}

int mw_is_ir_led_supported(void)
{
	int supported = ir_led_is_supported();

	return supported;
}

int mw_set_ir_led_param(mw_ir_led_control_param * pParam)
{

	if (ir_led_set_param(pParam) < 0) {
		MW_ERROR("ir_led_set_param error\n");
		return -1;
	}
	return 0;
}

int mw_get_ir_led_param(mw_ir_led_control_param * pParam)
{
	if (ir_led_get_param(pParam) < 0) {
		MW_ERROR("ir_led_get_param error\n");
		return -1;
	}
	return 0;
}

int mw_set_ir_led_brightness(int brightness)
{
	if (set_led_brightness(brightness) < 0) {
		MW_ERROR("ir_led_set_brightness error\n");
		return -1;
	}
	return 0;
}

int mw_get_ae_luma_value(mw_luma_value *pLuma)
{
	if (pLuma == NULL) {
		MW_ERROR("mw_get_ae_luma_value pointer is NULL!\n");
		return -1;
	}
	if (img_get_ae_rgb_luma(&pLuma->rgb_luma) < 0) {
		MW_ERROR("img_get_ae_rgb_luma failed!\n");
		return -1;
	}
	if (img_get_ae_cfa_luma(&pLuma->cfa_luma) < 0) {
		MW_ERROR("img_get_ae_cfa_luma failed!\n");
		return -1;
	}

	return 0;
}

int mw_get_ae_lines(mw_ae_line *lines, u32 line_num)
{
	if (lines == NULL) {
		MW_ERROR("mw_get_ae_lines pointer is NULL!\n");
		return -1;
	}
	if (get_ae_exposure_lines(lines, line_num) < 0) {
		MW_ERROR("get_ae_exposure_lines error\n");
		return -1;
	}
	return 0;
}

int mw_set_ae_lines(mw_ae_line *lines, u32 line_num)
{
	if (lines == NULL) {
		MW_ERROR("mw_set_ae_lines pointer is NULL!\n");
		return -1;
	}
	if (set_ae_exposure_lines(lines, line_num) < 0) {
		MW_ERROR("set_ae_exposure_lines error\n");
		return -1;
	}
	return 0;
}

int mw_get_ae_points(mw_ae_point *point_arr, u32 point_num)
{
	if (point_arr == NULL) {
		MW_ERROR("mw_get_ae_points pointer is NULL!\n");
		return -1;
	}
	return 0;
}

int mw_set_ae_points(mw_ae_point *point_arr, u32 point_num)
{
	if (point_arr == NULL) {
		MW_ERROR("mw_set_ae_points pointer is NULL!\n");
		return -1;
	}

	return 0;
}

/* ***************************************************************
	White Balance Control Settings
******************************************************************/

int mw_enable_awb(int enable)
{
	if (img_enable_awb(enable) < 0) {
		MW_ERROR("img_enable_awb failed!\n");
		return -1;
	};

	return 0;
}

int mw_get_awb_param(mw_awb_param *pAWB)
{
	if (pAWB == NULL) {
		MW_ERROR("mw_get_awb_param pointer is NULL!\n");
		return -1;
	}
	pAWB->wb_mode = G_mw_config.awb_params.wb_mode;

	MW_DEBUG(" === Get AWB Param ===\n");
	MW_DEBUG(" === wb_mode [%d]\n", pAWB->wb_mode);

	return 0;
}

int mw_set_awb_param(mw_awb_param *pAWB)
{
	if (pAWB == NULL) {
		MW_ERROR("mw_set_awb_param pointer is NULL");
		return -1;
	}
	if (G_mw_config.sensor.is_rgb_sensor == 0) {
		return 0;
	}

	if (pAWB->wb_mode == MW_WB_MODE_HOLD) {
		img_enable_awb(0);
		goto awb_exit;
	}

	if (pAWB->wb_mode >= WB_MODE_NUMBER) {
		MW_MSG("Wrong WB mode [%d].\n", pAWB->wb_mode);
		return -1;
	}

	if (pAWB->wb_mode != G_mw_config.awb_params.wb_mode) {
		if (img_enable_awb(1) < 0) {
			return -1;
		}
		if (img_set_awb_mode((awb_control_mode_t*)&pAWB->wb_mode) < 0) {
			return -1;
		}
		G_mw_config.awb_params.wb_mode = pAWB->wb_mode;
	}

awb_exit:

	MW_DEBUG(" === Set AWB Param ===\n");
	MW_DEBUG(" === wb_mode [%d]\n", pAWB->wb_mode);

	return 0;
}

int mw_get_awb_method(mw_white_balance_method *pWb_method)
{
	awb_work_method_t img_wb_method;
	if (pWb_method == NULL) {
		MW_ERROR("[mw_get_awb_method] NULL pointer!\n");
		return -1;
	}
	if ((img_get_awb_method(&img_wb_method)) < 0) {
		MW_ERROR("[img_get_awb_method] failed!\n");
		return -1;
	}
	*pWb_method = img_wb_method;
	G_mw_config.awb_params.wb_method = *pWb_method;

	return 0;
}

int mw_set_awb_method(mw_white_balance_method wb_method)
{
	aaa_cntl_t aaa_cntl;
	img_get_3a_cntl_status(&aaa_cntl);
	if (aaa_cntl.awb_enable == 0) {
		MW_ERROR("[mw_set_awb_method] cannot work when AWB is disabled!\n");
		return -1;
	}
	if (G_mw_config.awb_params.wb_method == wb_method) {
		return 0;
	}
	if (wb_method < 0 || wb_method >= MW_WB_METHOD_NUMBER) {
		MW_ERROR("Invalid AWB method [%d].\n", wb_method);
		return -1;
	}
	awb_work_method_t img_wb_method = wb_method;
	if (img_set_awb_method(&img_wb_method) < 0) {
		return -1;
	}
	G_mw_config.awb_params.wb_method = wb_method;

	return 0;
}

int mw_set_white_balance_mode(mw_white_balance_mode wb_mode)
{
	aaa_cntl_t aaa_cntl;
	img_get_3a_cntl_status(&aaa_cntl);
	if (aaa_cntl.awb_enable == 0) {
		MW_ERROR("[mw_set_white_balance_mode] cannot work when AWB is disabled!\n");
		return -1;
	}
	if (G_mw_config.awb_params.wb_method != MW_WB_NORMAL_METHOD) {
		MW_ERROR("only set in Normal method.\n");
		return -1;
	}
	if (G_mw_config.awb_params.wb_mode == wb_mode) {
		return 0;
	}
	if (wb_mode < 0 || wb_mode >= MW_WB_MODE_NUMBER) {
		MW_ERROR("Invalid WB mode [%d].\n", wb_mode);
		return -1;
	}
	awb_control_mode_t img_wb_mode = wb_mode;
	if (img_set_awb_mode(&img_wb_mode) < 0) {
		return -1;
	}
	G_mw_config.awb_params.wb_mode = wb_mode;

	return 0;
}

int mw_get_white_balance_mode(mw_white_balance_mode *pWb_mode)
{
	awb_control_mode_t img_wb_mode;
	if (pWb_mode == NULL) {
		MW_ERROR("[mw_get_white_balance_mode] NULL pointer!\n");
		return -1;
	}
	if ((img_get_awb_mode(&img_wb_mode)) < 0) {
		MW_ERROR("[img_get_awb_mode] failed!\n");
		return -1;
	}
	*pWb_mode = img_wb_mode;
	G_mw_config.awb_params.wb_mode = *pWb_mode;

	return 0;
}

int mw_get_awb_manual_gain(mw_wb_gain *pGain)
{
	wb_gain_t wb_gain;

	if (pGain == NULL) {
		MW_ERROR("[mw_get_awb_manual_gain] NULL pointer!\n");
		return -1;
	}
	if (img_get_awb_manual_gain(&wb_gain) < 0) {
		MW_ERROR("img_get_awb_manual_gain error!\n");
		return -1;
	}
	pGain->r_gain = wb_gain.r_gain;
	pGain->g_gain = wb_gain.g_gain;
	pGain->b_gain = wb_gain.b_gain;

	return 0;
}

int mw_set_awb_custom_gain(mw_wb_gain *pGain)
{
	wb_gain_t wb_gain;

	if (pGain == NULL) {
		MW_ERROR("[mw_set_awb_custom_gain] NULL pointer!\n");
		return -1;
	}

	wb_gain.r_gain = pGain->r_gain;
	wb_gain.g_gain = pGain->g_gain;
	wb_gain.b_gain = pGain->b_gain;

	if (img_set_awb_custom_gain(&wb_gain) < 0) {
		MW_ERROR("img_set_awb_custom_gain error!\n");
		return -1;
	}

	return 0;
}


/* ***************************************************************
	Image Adjustment Settings
******************************************************************/

int mw_get_image_param(mw_image_param *pImage)
{
	if (pImage == NULL) {
		MW_ERROR("mw_get_image_param pointer is NULL!\n");
		return -1;
	}
	pImage->saturation = G_mw_config.image_params.saturation;
	pImage->brightness = G_mw_config.image_params.brightness;
	pImage->hue = G_mw_config.image_params.hue;
	pImage->contrast = G_mw_config.image_params.contrast;
	pImage->sharpness = G_mw_config.image_params.sharpness;

	MW_DEBUG(" === Get Image Param ===\n");
	MW_DEBUG(" === Saturation [%d]\n", pImage->saturation);
	MW_DEBUG(" === Brightness [%d]\n", pImage->brightness);
	MW_DEBUG(" === Hue        [%d]\n", pImage->hue);
	MW_DEBUG(" === Contrast   [%d]\n", pImage->contrast);
	MW_DEBUG(" === Sharpness  [%d]\n", pImage->sharpness);

	return 0;
}

int mw_set_image_param(mw_image_param *pImage)
{
	if (pImage == NULL) {
		MW_ERROR("mw_set_image_param pointer is NULL!\n");
		return -1;
	}
	if (G_mw_config.sensor.is_rgb_sensor == 0) {
		return 0;
	}
	if (pImage->saturation != G_mw_config.image_params.saturation) {
		if (img_set_color_saturation(pImage->saturation) < 0) {
			return -1;
		}
		G_mw_config.image_params.saturation = pImage->saturation;
	}
	if (pImage->brightness != G_mw_config.image_params.brightness) {
		if (img_set_color_brightness(pImage->brightness) < 0) {
			return -1;
		}
		G_mw_config.image_params.brightness = pImage->brightness;
	}
	if (pImage->hue != G_mw_config.image_params.hue) {
		if (img_set_color_hue(pImage->hue) < 0) {
			return -1;
		}
		G_mw_config.image_params.hue = pImage->hue;
	}
	if (pImage->contrast != G_mw_config.image_params.contrast) {
		if (img_set_color_contrast(pImage->contrast) < 0) {
			return -1;
		}
		G_mw_config.image_params.contrast = pImage->contrast;
	}
	if (pImage->sharpness != G_mw_config.image_params.sharpness) {
		if (img_set_sharpening_strength(pImage->sharpness) < 0) {
			return -1;
		}
		G_mw_config.image_params.sharpness = pImage->sharpness;
	}

	MW_DEBUG(" === Set Image Param ===\n");
	MW_DEBUG(" === Saturation [%d]\n", pImage->saturation);
	MW_DEBUG(" === Brightness [%d]\n", pImage->brightness);
	MW_DEBUG(" === Hue        [%d]\n", pImage->hue);
	MW_DEBUG(" === Contrast   [%d]\n", pImage->contrast);
	MW_DEBUG(" === Sharpness  [%d]\n", pImage->sharpness);

	return 0;
}

int mw_get_saturation(int *pSaturation)
{
	*pSaturation = G_mw_config.image_params.saturation;
	return 0;
}

int mw_set_saturation(int saturation)
{
	if (G_mw_config.image_params.saturation == saturation) {
		return 0;
	}
	if (saturation < MW_SATURATION_MIN || saturation > MW_SATURATION_MAX) {
		return -1;
	}
	if (img_set_color_saturation(saturation) < 0) {
		return -1;
	}
	G_mw_config.image_params.saturation = saturation;

	return 0;
}

int mw_get_brightness(int *pBrightness)
{
	*pBrightness = G_mw_config.image_params.brightness;
	return 0;
}

int mw_set_brightness(int brightness)
{
	if (G_mw_config.image_params.brightness == brightness) {
		return 0;
	}

	if (brightness < MW_BRIGHTNESS_MIN || brightness > MW_BRIGHTNESS_MAX) {
		return -1;
	}
	if (img_set_color_brightness(brightness) < 0) {
		return -1;
	}
	G_mw_config.image_params.brightness = brightness;

	return 0;
}

int mw_get_contrast(int *pContrast)
{
	*pContrast = G_mw_config.image_params.contrast;
	return 0;
}

int mw_set_contrast(int contrast)
{
	if (G_mw_config.image_params.contrast == contrast) {
		return 0;
	}

	if (contrast < MW_CONTRAST_MIN || contrast > MW_CONTRAST_MAX) {
		return -1;
	}
	if (img_set_color_contrast(contrast) < 0) {
		return -1;
	}
	G_mw_config.image_params.contrast = contrast;

	return 0;
}

int mw_get_sharpness(int *pSharpness)
{
	*pSharpness = G_mw_config.image_params.sharpness;
	return 0;
}

int mw_set_sharpness(int sharpness)
{
	if (G_mw_config.image_params.sharpness == sharpness) {
		return 0;
	}
	if (sharpness < MW_SHARPNESS_MIN || sharpness > MW_SHARPNESS_MAX) {
		return -1;
	}
	if (img_set_sharpening_strength(sharpness) < 0) {
		MW_ERROR("[img_set_sharpening_strength] failed!\n");
		return -1;
	}
	G_mw_config.image_params.sharpness = sharpness;

	return 0;
}

/* ***************************************************************
	Image Enhancement Settings
******************************************************************/

int mw_set_mctf_strength(u32 strength)
{
	if (G_mw_config.enh_params.mctf_strength == strength) {
		return 0;
	}

	if (strength < MW_MCTF_STRENGTH_MIN || strength > MW_MCTF_STRENGTH_MAX) {
		return -1;
	}
	if (img_set_mctf_strength(strength) < 0) {
		MW_ERROR("img_set_mctf_strength error!\n");
		return -1;
	}
	G_mw_config.enh_params.mctf_strength = strength;

	return 0;
}

int mw_get_mctf_strength(u32 *pStrength)
{
	*pStrength = G_mw_config.enh_params.mctf_strength;
	return 0;
}

int mw_set_auto_local_exposure_mode(u32 mode)
{
	MW_INFO("Local exposure mode : [%d].\n", mode);
	if (mode == 0 || mode ==1) {
		if (img_set_wdr_enable(mode) < 0) {
			MW_ERROR("img_set_wdr_enable error/n");
			return -1;
		}
		if (mode == 0) {//restore local exposure to off(no WDR)
			if (mw_set_local_exposure_curve(G_mw_config.fd,
				&G_mw_config.enh_params.le_off_curve) < 0) {
				MW_ERROR("Restore local exposure to off failed\n");
				return -1;
			}
		}
	} else if (mode <= MW_LE_STRENGTH_MAX) {
		if (G_mw_config.enh_params.local_expo_mode == 0) {
			if (img_set_wdr_enable(1) < 0) {
				MW_ERROR("img_set_wdr_enable error/n");
				return -1;
			}
		}
		if (img_set_wdr_strength(mode) < 0) {
			MW_ERROR("img_set_wdr_strength error/n");
			return -1;
		}
	} else {
		MW_ERROR("Invalide mode %d, range is [%d, %d]\n",
				mode, MW_LE_STRENGTH_MIN, MW_LE_STRENGTH_MAX);
		return -1;
	}
	G_mw_config.enh_params.local_expo_mode = mode;
	return 0;
}

int mw_get_auto_local_exposure_mode(u32 *pMode)
{
	if (pMode == NULL) {
		MW_ERROR("mw_get_auto_local_exposure_mode pointer is NULL!\n");
		return -1;
	}
	*pMode = G_mw_config.enh_params.local_expo_mode;
	return 0;
}

int mw_set_local_exposure_curve(int fd, mw_local_exposure_curve *pLocal_exposure)
{
	amba_img_dsp_mode_cfg_t dsp_mode_cfg;
	amba_img_dsp_local_exposure_t local_exposure;

	memset(&dsp_mode_cfg, 0, sizeof(dsp_mode_cfg));

	if (get_dsp_mode_cfg(&dsp_mode_cfg) < 0) {
		return -1;
	}

	local_exposure.Enb = 1;
	local_exposure.Radius = 0;
	local_exposure.LumaWeightRed = 16;
	local_exposure.LumaWeightGreen = 16;
	local_exposure.LumaWeightBlue = 16;
	local_exposure.LumaWeightShift = 0;
	memcpy(local_exposure.GainCurveTable, pLocal_exposure->gain_curve_table,
		sizeof(local_exposure.GainCurveTable));

	return amba_img_dsp_set_local_exposure(fd, &dsp_mode_cfg, &local_exposure);
}

int mw_enable_backlight_compensation(int enable)
{
	if (img_set_ae_backlight(&G_mw_config.enh_params.bl_compensation) < 0) {
		MW_ERROR("img_ae_set_backlight error\n");
		return -1;
	}
	G_mw_config.enh_params.bl_compensation = enable;

	return 0;
}

int mw_enable_day_night_mode(int enable)
{
	if (img_set_bw_mode(enable) < 0) {
		MW_ERROR("img_set_bw_mode error.\n");
		return -1;
	}
	G_mw_config.enh_params.dn_mode = enable;

	return 0;
}

int mw_get_adj_status(int *pEnable)
{
	if (pEnable == NULL) {
		MW_ERROR("%s error: pEnable is null\n", __func__);
		return -1;
	}
	aaa_cntl_t aaa_status;
	img_get_3a_cntl_status(&aaa_status);
	*pEnable = aaa_status.adj_enable;
	return 0;
}

int mw_enable_adj(int enable)
{
	if (enable > 1 || enable < 0) {
		MW_ERROR("%s error: the value must be 0 | 1\n", __func__);
		return -1;
	}
	img_enable_adj(enable);

	return 0;
}

int mw_get_chroma_noise_strength(int *pStrength)
{
	if (pStrength == NULL) {
		MW_ERROR("%s error: pStrength is null\n", __func__);
		return -1;
	}
	*pStrength = G_mw_config.enh_params.chroma_noise_str;
	return 0;
}

int mw_set_chroma_noise_strength(int strength)
{
	if (strength > 256 || strength < 0) {
		MW_ERROR("%s error: the value must be [0~256]\n", __func__);
		return -1;
	}
	G_mw_config.enh_params.chroma_noise_str = strength;

	return 0;
}

int mw_get_lib_version(mw_aaa_lib_version *pAlgo_lib_version,
	mw_aaa_lib_version *pDsp_lib_version)
{
	if ((pAlgo_lib_version == NULL) || (pDsp_lib_version == NULL)) {
		MW_ERROR("mw_get_lib_version error: algo_lib_version or dsp_lib_version is null\n");
		return -1;
	}

	img_algo_get_lib_version((img_lib_version_t *)pAlgo_lib_version);
	return 0;
}

int mw_set_auto_color_contrast(u32 enable)
{
	if (enable > 1 || enable < 0) {
		MW_ERROR("The value must be 0 or 1\n");
		return -1;
	}
	if (img_set_auto_color_contrast(enable) < 0) {
		MW_ERROR("img_set_auto_color_contrast failed\n");
		return -1;
	}

	G_mw_config.enh_params.auto_contrast = enable;
	return 0;
}

int mw_get_auto_color_contrast(u32 *pEnable)
{
	if (pEnable == NULL) {
		MW_ERROR("%s error: pEnable is null\n", __func__);
		return -1;
	}
	*pEnable = G_mw_config.enh_params.auto_contrast;
	return 0;
}

int mw_set_auto_color_contrast_strength (u32 value)
{
	if (!G_mw_config.enh_params.auto_contrast) {
		MW_ERROR("\nPlease enable auto color contrast mode first\n");
		return -1;
	}
	if (value > 128) {
		MW_ERROR("\nThe value must be 0~128\n");
		return -1;
	}
	if (img_set_auto_color_contrast_strength(value) < 0) {
		MW_ERROR("img_set_auto_color_contrast_strength failed\n");
		return -1;
	}

	return 0;
}

#define	IMAGE_AREA_WIDTH		4096
#define	IMAGE_AREA_HEIGHT		4096

int mw_set_ae_area(int enable)
{
	struct iav_srcbuf_setup srcbuf_setup;
	statistics_config_t config;

	memset(&srcbuf_setup, 0, sizeof(struct iav_srcbuf_setup));

	if (ioctl(G_mw_config.fd, IAV_IOC_GET_SOURCE_BUFFER_SETUP, &srcbuf_setup) < 0) {
		MW_ERROR("IAV_IOC_GET_SOURCE_BUFFER_SETUP\n");
		return -1;
	}

	if (enable) {
		config.ae_tile_col_start = srcbuf_setup.input[IAV_SRCBUF_PMN].x *
			IMAGE_AREA_WIDTH / srcbuf_setup.size[IAV_SRCBUF_MN].width;
		config.ae_tile_row_start = srcbuf_setup.input[IAV_SRCBUF_PMN].y *
			IMAGE_AREA_HEIGHT / srcbuf_setup.size[IAV_SRCBUF_MN].height;
		config.ae_tile_width = (srcbuf_setup.size[IAV_SRCBUF_PMN].width /
			config.ae_tile_num_col) * IMAGE_AREA_WIDTH /
			srcbuf_setup.size[IAV_SRCBUF_MN].width;
		config.ae_tile_height = (srcbuf_setup.size[IAV_SRCBUF_PMN].height/
			config.ae_tile_num_row) * IMAGE_AREA_HEIGHT/
			srcbuf_setup.size[IAV_SRCBUF_MN].height;
	} else {
		config.ae_tile_col_start = 0;
		config.ae_tile_row_start = 0;
		config.ae_tile_width = IMAGE_AREA_WIDTH / config.ae_tile_num_col;
		config.ae_tile_height = IMAGE_AREA_HEIGHT / config.ae_tile_num_row;
	}

	return 0;
}

int mw_get_auto_wdr_strength(int *pStrength)
{
	if (pStrength == NULL) {
		MW_ERROR("%s error: pStrength is null\n", __func__);
		return -1;
	}
	*pStrength = G_mw_config.enh_params.auto_wdr_str;
	return 0;
}
int mw_set_auto_wdr_strength(int strength)
{
	if (strength > 128 || strength < 0) {
		MW_ERROR("%s error:The value must be in 0~128\n", __func__);
		return -1;
	}

	if (G_mw_config.enh_params.auto_wdr_str == strength) {
		MW_MSG("The strength is %d already\n", strength);
		return 0;
	}

	if (strength == 0) {
		img_set_wdr_enable(strength);
		//restore local exposure to off(no WDR)
		if (mw_set_local_exposure_curve(G_mw_config.fd,
			&G_mw_config.enh_params.le_off_curve) < 0) {
			MW_ERROR("Restore local exposure to off failed\n");
			return -1;
		}
	} else {
		img_set_wdr_enable(1);
		img_set_wdr_strength(strength);
	}

	G_mw_config.enh_params.auto_wdr_str = strength;

	return 0;
}

int mw_load_adj_param(mw_adj_file_param *contents)
{
	int i;
	int file_name_len = 0;

	if (contents->filename == NULL) {
		MW_ERROR("%s error: pFile_name is null\n", __func__);
		return -1;
	}
	file_name_len = strlen(contents->filename);

	if (file_name_len > (FILE_NAME_LENGTH - 1)) {
		MW_ERROR("%s error: the file name length must be < %d\n",
			__func__, FILE_NAME_LENGTH);
		return -1;

	}

	memcpy(G_mw_config.adj.filename, contents->filename, file_name_len);
	G_mw_config.adj.filename[file_name_len] = '\0';
	for (i = 0; i < ADJ_FILTER_NUM; i++) {
		G_mw_config.adj.flag[i].apply = contents->flag[i].apply;
		G_mw_config.adj.flag[i].done = 0;
	}

	if (load_adj_file(&G_mw_config) < 0) {
		MW_ERROR("%s error.\n", __func__);
		return -1;
	}

	for (i = 0; i < ADJ_FILTER_NUM; i++) {
		contents->flag[i].done =
			G_mw_config.adj.flag[i].done;
	}

	return 0;
}


int mw_save_adj_param(mw_adj_file_param *contents)
{
	int i;
	int file_name_len = 0;

	if (contents->filename == NULL) {
		MW_ERROR("%s error: pFile_name is null\n", __func__);
		return -1;
	}
	file_name_len = strlen(contents->filename);

	if (file_name_len > (FILE_NAME_LENGTH - 1)) {
		MW_ERROR("%s error: the file name length must be < %d\n",
			__func__, FILE_NAME_LENGTH);
		return -1;

	}

	memcpy(G_mw_config.adj.filename, contents->filename, file_name_len);
	G_mw_config.adj.filename[file_name_len] = '\0';
	for (i = 0; i < ADJ_FILTER_NUM; i++) {
		G_mw_config.adj.flag[i].apply = contents->flag[i].apply;
		G_mw_config.adj.flag[i].done = 0;
	}


	if (save_adj_file(&G_mw_config) < 0) {
		MW_ERROR("%s error.\n", __func__);
		return -1;
	}

	for (i = 0; i < ADJ_FILTER_NUM; i++) {
		contents->flag[i].done =
			G_mw_config.adj.flag[i].done;
	}

	return 0;
}

int mw_load_aaa_param_file(char *pFile_name, int type)
{
	int file_name_len = 0;
	if (pFile_name == NULL) {
		MW_ERROR("%s error: pFile_name is null\n", __func__);
		return -1;
	}
	file_name_len = strlen(pFile_name);

	if (file_name_len > (FILE_NAME_LENGTH - 1)) {
		MW_ERROR("%s error: the file name length must be < %d\n",
			__func__, FILE_NAME_LENGTH);
		return -1;

	}

	switch (type) {
	case FILE_TYPE_ADJ:
		memcpy(G_mw_config.sensor.load_files.adj_file, pFile_name,
			sizeof(G_mw_config.sensor.load_files.adj_file));
		G_mw_config.sensor.load_files.adj_file[file_name_len] = '\0';
		break;
	case FILE_TYPE_AEB:
		memcpy(G_mw_config.sensor.load_files.aeb_file, pFile_name,
			sizeof(G_mw_config.sensor.load_files.aeb_file));
		G_mw_config.sensor.load_files.aeb_file[file_name_len] = '\0';
		break;
	case FILE_TYPE_PIRIS:
		memcpy(G_mw_config.sensor.load_files.lens_file, pFile_name,
			sizeof(G_mw_config.sensor.load_files.lens_file));
		G_mw_config.sensor.load_files.lens_file[file_name_len] = '\0';
		break;
	default:
		MW_ERROR("No the type\n");
		break;
	}
	return 0;
}

int mw_set_lens_id(int lens_id)
{
	G_mw_config.sensor.lens_id = lens_id;
	return 0;
}

int mw_set_auto_knee_strength (int strength)
{
	u8 auto_knee_strength = 0;
	if (G_mw_config.enh_params.auto_knee_strength == strength) {
		return 0;
	}

	if (strength < MW_AUTO_KNEE_MIN || strength > MW_AUTO_KNEE_MAX) {
		return -1;
	} else {
		auto_knee_strength = strength;
	}
	if (img_set_ae_auto_knee(&auto_knee_strength) < 0) {
		return -1;
	}
	G_mw_config.enh_params.auto_knee_strength = strength;

	return 0;
}

int mw_get_auto_knee_strength (int *pStrength)
{
	if (pStrength == NULL) {
		MW_ERROR("NULL pointer, mw_get_auto_knee_strength failed\n");
		return -1;
	}
	*pStrength = G_mw_config.enh_params.auto_knee_strength;
	return 0;
}

int mw_enable_auto_dump_cfg(int enable)
{
	G_mw_config.enh_params.auto_dump_cfg = !!enable;
	return 0;
}

int mw_set_hdr_blend_config(mw_hdr_blend_info *pBlend_cfg)
{
	hdr_blend_info_t hdr_blend_cfg;
	if (pBlend_cfg->boost_factor < MW_HDR_BLEND_BOOST_FACTOR_MIN ||
			pBlend_cfg->boost_factor > MW_HDR_BLEND_BOOST_FACTOR_MAX) {
		MW_ERROR("Invalid param: boost factor %d is out of range [%d, %d]",
				pBlend_cfg->boost_factor, MW_HDR_BLEND_BOOST_FACTOR_MIN,
				MW_HDR_BLEND_BOOST_FACTOR_MAX);
		return -1;
	}
	if (pBlend_cfg->expo_ratio < MW_HDR_BLEND_EXPO_RATIO_MIN ||
			pBlend_cfg->expo_ratio > MW_HDR_BLEND_EXPO_RATIO_MAX) {
		MW_ERROR("Invalid param: exposure ratio %d is out of range [%d, %d]",
				pBlend_cfg->expo_ratio, MW_HDR_BLEND_EXPO_RATIO_MIN,
				MW_HDR_BLEND_EXPO_RATIO_MAX);
		return -1;
	}
	hdr_blend_cfg.boost_factor = pBlend_cfg->boost_factor;
	hdr_blend_cfg.expo_ratio = pBlend_cfg->expo_ratio;
	if (img_set_hdr_blend_config(&hdr_blend_cfg) < 0) {
		MW_ERROR("img_set_hdr_blend_config failed!\n");
		return -1;
	}

	return 0;
}

int mw_get_hdr_blend_config(mw_hdr_blend_info *pBlend_cfg)
{
	hdr_blend_info_t hdr_blend_cfg;
	if (img_get_hdr_blend_config(&hdr_blend_cfg) < 0) {
		MW_ERROR("img_get_hdr_blend_config failed!\n");
		return -1;
	}
	pBlend_cfg->boost_factor = hdr_blend_cfg.boost_factor;
	pBlend_cfg->expo_ratio = hdr_blend_cfg.expo_ratio;

	return 0;
}

int mw_set_wdr_luma_config(mw_wdr_luma_info *pWdr_luma_cfg)
{
	wdr_luma_config_info_t wdr_luma_cfg;
	int retv = -1;

	if (pWdr_luma_cfg == NULL) {
		MW_ERROR("NULL pointer, mw_set_wdr_luma_config failed\n");
		retv = -1;
		goto EXIT;
	}

	if ((pWdr_luma_cfg->radius < MW_WDR_LUMA_RADIUS_MIN) || (pWdr_luma_cfg->radius > MW_WDR_LUMA_RADIUS_MAX)) {
		MW_ERROR("Invalid param: radius %d is out of range [%d, %d]",
				pWdr_luma_cfg->radius, MW_WDR_LUMA_RADIUS_MIN,
				MW_WDR_LUMA_RADIUS_MAX);
		retv = -1;
		goto EXIT;
	}

	if ((pWdr_luma_cfg->luma_weight_red < MW_WDR_LUMA_WEIGHT_MIN) || (pWdr_luma_cfg->luma_weight_red > MW_WDR_LUMA_WEIGHT_MAX)) {
		MW_ERROR("Invalid param: luma_weight_red %d is out of range [%d, %d]",
				pWdr_luma_cfg->luma_weight_red, MW_WDR_LUMA_WEIGHT_MIN,
				MW_WDR_LUMA_WEIGHT_MAX);
		retv = -1;
		goto EXIT;
	}

	if ((pWdr_luma_cfg->luma_weight_green < MW_WDR_LUMA_WEIGHT_MIN) || (pWdr_luma_cfg->luma_weight_green > MW_WDR_LUMA_WEIGHT_MAX)) {
		MW_ERROR("Invalid param: luma_weight_green %d is out of range [%d, %d]",
				pWdr_luma_cfg->luma_weight_green, MW_WDR_LUMA_WEIGHT_MIN,
				MW_WDR_LUMA_WEIGHT_MAX);
		retv = -1;
		goto EXIT;
	}

	if ((pWdr_luma_cfg->luma_weight_blue < MW_WDR_LUMA_WEIGHT_MIN) || (pWdr_luma_cfg->luma_weight_blue > MW_WDR_LUMA_WEIGHT_MAX)) {
		MW_ERROR("Invalid param: luma_weight_blue %d is out of range [%d, %d]",
				pWdr_luma_cfg->luma_weight_blue, MW_WDR_LUMA_WEIGHT_MIN,
				MW_WDR_LUMA_WEIGHT_MAX);
		retv = -1;
		goto EXIT;
	}

	if ((pWdr_luma_cfg->luma_weight_shift < MW_WDR_LUMA_WEIGHT_MIN) || (pWdr_luma_cfg->luma_weight_shift > MW_WDR_LUMA_WEIGHT_MAX)) {
		MW_ERROR("Invalid param: luma_weight_shift %d is out of range [%d, %d]",
				pWdr_luma_cfg->luma_weight_shift, MW_WDR_LUMA_WEIGHT_MIN,
				MW_WDR_LUMA_WEIGHT_MAX);
		retv = -1;
		goto EXIT;
	}

	wdr_luma_cfg.radius = pWdr_luma_cfg->radius;
	wdr_luma_cfg.luma_weight_red = pWdr_luma_cfg->luma_weight_red;
	wdr_luma_cfg.luma_weight_green = pWdr_luma_cfg->luma_weight_green;
	wdr_luma_cfg.luma_weight_blue = pWdr_luma_cfg->luma_weight_blue;
	wdr_luma_cfg.luma_weight_shift = pWdr_luma_cfg->luma_weight_shift;

	if (img_set_wdr_luma_config(&wdr_luma_cfg) < 0) {
		MW_ERROR("img_set_wdr_luma_config failed!\n");
		retv = -1;
		goto EXIT;
	}

	retv = 0;

EXIT:
	return retv;
}

int mw_get_wdr_luma_config(mw_wdr_luma_info *pWdr_luma_cfg)
{
	wdr_luma_config_info_t wdr_luma_cfg;
	int retv = -1;

	if (pWdr_luma_cfg == NULL) {
		MW_ERROR("NULL pointer, mw_get_wdr_luma_config failed\n");
		retv = -1;
		goto EXIT;
	}

	if (img_get_wdr_luma_config(&wdr_luma_cfg) < 0) {
		MW_ERROR("img_get_wdr_luma_config failed!\n");
		retv = -1;
		goto EXIT;
	}

	pWdr_luma_cfg->radius = wdr_luma_cfg.radius;
	pWdr_luma_cfg->luma_weight_red = wdr_luma_cfg.luma_weight_red;
	pWdr_luma_cfg->luma_weight_green = wdr_luma_cfg.luma_weight_green;
	pWdr_luma_cfg->luma_weight_blue = wdr_luma_cfg.luma_weight_blue;
	pWdr_luma_cfg->luma_weight_shift = wdr_luma_cfg.luma_weight_shift;

	retv = 0;

EXIT:
	return retv;
}

#define __END_OF_FILE__

