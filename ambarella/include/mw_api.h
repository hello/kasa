/*
 * mw_api.h
 *
 * History:
 *	2013/03/12 - [Cao Rongrong] Created file
 *	2013/12/12 - [Jian Tang] Modified file
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

#ifndef __MW_API_H__
#define __MW_API_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "basetypes.h"


/********************** Image global configuration **********************/
int mw_start_aaa(int fd_iav);
int mw_stop_aaa(void);
int mw_init_mctf(int fd_iav);
int mw_get_iav_state(int fd, int *pState);
/********************** Exposure Control Settings **********************/
int mw_enable_ae(int enable);
int mw_get_ae_param(mw_ae_param *ae_param);
int mw_set_ae_param(mw_ae_param *ae_param);
int mw_get_exposure_level(int *pExposure_level);
int mw_get_shutter_time(int fd_iav, int *pShutter_time);
int mw_get_sensor_gain(int fd_iav, int *pGain_db);
int mw_get_ae_metering_mode(mw_ae_metering_mode *pMode);
int mw_set_ae_metering_mode(mw_ae_metering_mode mode);
int mw_get_ae_metering_table(mw_ae_metering_table *pAe_metering_table);
int mw_set_ae_metering_table(mw_ae_metering_table *pAe_metering_table);
int mw_is_dc_iris_supported(void);
int mw_enable_dc_iris_control(int enable);
int mw_set_dc_iris_pid_coef(mw_dc_iris_pid_coef *pPid_coef);
int mw_get_dc_iris_pid_coef(mw_dc_iris_pid_coef *pPid_coef);
int mw_is_ir_led_supported(void);
int mw_set_ir_led_param(mw_ir_led_control_param *pParam);
int mw_get_ir_led_param(mw_ir_led_control_param *pParam);
int mw_set_ir_led_brightness(int brightness);
int mw_get_ae_luma_value(mw_luma_value *pLuma);
int mw_get_ae_lines(mw_ae_line *lines, u32 line_num);
int mw_set_ae_lines(mw_ae_line *lines, u32 line_num);
int mw_get_ae_points(mw_ae_point *point_arr, u32 point_num);
int mw_set_ae_points(mw_ae_point *point_arr, u32 point_num);
int mw_set_ae_area(int enable);
int mw_set_sensor_gain(int fd_iav, int *pGain_db);
int mw_set_shutter_time(int fd_iav, int *shutter_time);
int mw_set_exposure_level(int *pExposure_level);

/********************** White Balance Settings **********************/
int mw_enable_awb(int enable);
int mw_get_awb_param(mw_awb_param *pAWB);
int mw_set_awb_param(mw_awb_param *pAWB);
int mw_get_awb_method(mw_white_balance_method *pWb_method);
int mw_set_awb_method(mw_white_balance_method wb_method);
int mw_set_white_balance_mode(mw_white_balance_mode wb_mode);
int mw_get_white_balance_mode(mw_white_balance_mode *pWb_mode);
int mw_set_awb_custom_gain(mw_wb_gain *pGain);
int mw_get_awb_manual_gain(mw_wb_gain *pGain);

/********************** Image Adjustment Settings **********************/
int mw_get_image_param(mw_image_param *pImage);
int mw_set_image_param(mw_image_param *pImage);
int mw_get_saturation(int *pSaturation);
int mw_set_saturation(int saturation);
int mw_get_brightness(int *pBrightness);
int mw_set_brightness(int brightness);
int mw_get_contrast(int *pContrast);
int mw_set_contrast(int contrast);
int mw_get_sharpness(int *pSharpness);
int mw_set_sharpness(int sharpness);

/********************** Image Enhancement Settings **********************/
int mw_get_mctf_strength(u32 *pStrength);
int mw_set_mctf_strength(u32 strength);
int mw_get_auto_local_exposure_mode(u32 *pMode);
int mw_set_auto_local_exposure_mode(u32 mode);
int mw_set_local_exposure_curve(int fd_iav, mw_local_exposure_curve *pLocal_exposure);
int mw_enable_backlight_compensation(int enable);
int mw_enable_day_night_mode(int enable);
int mw_set_auto_color_contrast(u32 enable);
int mw_get_auto_color_contrast(u32 *pEnable);
int mw_set_auto_color_contrast_strength (u32 value);
int mw_get_adj_status(int *pEnable);
int mw_enable_adj(int enable);
int mw_get_chroma_noise_strength(int *pStrength);
int mw_set_chroma_noise_strength(int strength);
int mw_get_auto_wdr_strength(int *pStrength);
int mw_set_auto_wdr_strength(int strength);
int mw_set_sharpen_filter(int sharpen);
int mw_get_sharpen_filter(void);

/********************** Image auto focus Settings **********************/
int mw_get_af_param(mw_af_param *pAF);
int mw_set_af_param(mw_af_param *pAF);

/********************** Calibration related APIs **********************/
int mw_load_calibration_file(mw_cali_file *pFile);

/********************** Motion detection related APIs **********************/
int mw_md_get_motion_event(int *p_motion_event);
int mw_md_callback_setup(alarm_handle_func alarm_handler);
int mw_md_get_roi_info(int roi_idx, mw_roi_info *roi_info);
int mw_md_set_roi_info(int roi_idx, const mw_roi_info *roi_info);
int mw_md_clear_roi(int roi_idx);
int mw_md_roi_region_display(void);
int mw_md_thread_start(void);
int mw_md_thread_stop(void);
void mw_md_print_event(int flag);

/**********************Others APIs **********************************/
int mw_get_version_info(mw_version_info *pVersion);
int mw_get_log_level(int *pLog);
int mw_set_log_level(mw_log_level log);
int mw_get_lib_version(mw_aaa_lib_version *pAlgo_lib_version,
	mw_aaa_lib_version *pDsp_lib_version);
int mw_display_adj_bin_version(void);
int mw_load_aaa_param_file(char *pFile_name, int type);
int mw_get_sys_info(mw_sys_info *pMw_sys_info);
int mw_get_image_statistics(mw_image_stat_info *pInfo);
int mw_load_config_file(char *filename);
int mw_save_config_file(char *filename);
int mw_load_adj_param(mw_adj_file_param *contents);
int mw_save_adj_param(mw_adj_file_param *contents);
int mw_set_lens_id(int id);
int mw_set_auto_knee_strength (int strength);
int mw_get_auto_knee_strength (int *pStrength);
int mw_enable_auto_dump_cfg(int enable);
int mw_set_hdr_blend_config(mw_hdr_blend_info *pBlend_cfg);
int mw_get_hdr_blend_config(mw_hdr_blend_info *pBlend_cfg);
int mw_set_wdr_luma_config(mw_wdr_luma_info *pWdr_luma_cfg);
int mw_get_wdr_luma_config(mw_wdr_luma_info *pWdr_luma_cfg);
#ifdef __cplusplus
}
#endif

#endif //  __MW_API_H__

