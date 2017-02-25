/*
 * include/arch_s2l/img_api_adv_arch.h
 *
 * History:
 *	2012/10/10 - [Cao Rongrong] created file
 *	2013/12/12 - [Jian Tang] modified file
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

#ifndef	IMG_API_ADV_ARCH_H
#define	IMG_API_ADV_ARCH_H

#ifdef __cplusplus
extern "C" {
#endif

//#include "config.h"
#include "basetypes.h"

// basic 3A related APIs
int img_lib_init(int fd_iav, u32 enable_defblc, u32 enable_sharpen_b);
int img_lib_deinit(void);
void img_algo_get_lib_version(img_lib_version_t* p_img_algo_info );
int img_config_pipeline(int fd_iav,img_config_info_t * p_pipe_config);
int img_prepare_isp(int fd_iav);//called before start aaa, after config pipline, load_cc_container
int img_start_aaa(int fd_iav);
int img_stop_aaa(void);
int img_register_aaa_algorithm(aaa_api_t custom_aaa_api);


int img_set_work_mode(int mode); //0 -preview
int img_config_stat_tiles(statistics_config_t *tile_cfg);
int img_adj_load_cc_binary(cc_binary_addr_t* addr);

int img_adj_init_containers_liso(fc_collection_t* src_fc);
int img_dynamic_load_containers(fc_collection_t* p_fc);
int img_dynamic_load_cc_bin(cc_binary_addr_t* addr);
//int img_adj_init_containers_hiso(fc_collection_hiso_t* src_fc_hiso);
int img_config_ae_tables(ae_cfg_tbl_t * ae_tbl,int tbl_num);
int img_config_sensor_info(sensor_config_t* config);
int img_config_stat_tiles(statistics_config_t * tile_cfg);
int img_config_awb_param(img_awb_param_t* p_awb_param);
int img_enable_ae(int enable);
int img_enable_awb(int enable);
int img_enable_af(int enable);
int img_enable_adj(int enable);
void img_get_3a_cntl_status(aaa_cntl_t *cntl);

// sensor normal
int img_get_sensor_agc();
int img_get_sensor_shutter();
int img_set_sensor_agc(int fd_iav,u32 gain_tbl_idx);
int img_set_sensor_shutter(int fd_iav,u32 shutter_row);

// sensor hdr
int img_set_hdr_sensor_shutter_agc_group(int fd_iav,hdr_sensor_gp_t * p_sensor_gp);
int img_get_hdr_sensor_agc_group(int fd_iav, hdr_gain_gp_t * p_gain_gp);
int img_get_hdr_sensor_shutter_group(int fd_iav, hdr_shutter_gp_t * p_sht_gp);

//ae
int img_set_ae_speed(u8* speed_level);//0-8, slow to fast
int img_set_ae_exp_lines(line_t *line, u16* line_num, u16* line_belt);
int img_set_ae_target_ratio(u16* target_ratio);
int img_set_ae_loosen_belt(u16* belt);
int img_set_ae_meter_mode(u8* mode);
int img_set_ae_meter_roi(int* roi);//valid only meter_mode =CUSTOMER_METERING_MODE
int img_set_ae_sync_delay(u8* shutter_delay, u8* agc_delay);
int img_set_ae_backlight(int* backlight_enable);
int img_set_ae_auto_knee(u8* auto_knee);// 0-255
int img_register_custom_aeflow_func(ae_flow_func_t* custom_ae_flow_func);
int img_get_ae_cfa_luma(u16* ae_cfa_luma);
int img_get_ae_rgb_luma(u16* ae_rgb_luma);
int img_get_ae_system_gain(u32* ae_system_gain);
int img_get_ae_stable(u8* ae_stable_flag);//return 0: AE adjusting; 1: AE stable; 2: AE reach to high light limit; 3: AE reach to low light limit.
int img_get_ae_cursor(u8* ae_cursor);
int img_get_ae_target_ratio(u16* ae_target_ratio);
int img_get_ae_target(u16* ae_target);
int img_get_ae_piris_info(piris_info_t* piris_info); //success: return 0, failed: return -1
int img_format_ae_line(int fd_iav, line_t* ae_lines,int line_num, u32 db_step);//shutter_index to shutter_row used in ae algo
int img_config_auto_knee_info(img_aeb_auto_knee_param_t* config);

//awb
int img_get_awb_manual_gain(wb_gain_t* p_wb_gain);
int img_set_awb_custom_gain(wb_gain_t* p_cus_gain);
int img_set_awb_cali_shift(wb_gain_t* p_wb_org_gain, wb_gain_t* p_wb_ref_gain);
int img_set_awb_cali_thre(u16 *p_r_th, u16 *p_b_th);
int img_set_awb_speed(u8* p_awb_spd);
int img_set_awb_failure_remedy(awb_failure_remedy_t* awb_failure_remedy);
int img_get_awb_mode(awb_control_mode_t *p_awb_mode);
int img_set_awb_mode(awb_control_mode_t *p_awb_mode);
int img_get_awb_method(awb_work_method_t *p_awb_method);
int img_set_awb_method(awb_work_method_t *p_awb_method);
int img_get_awb_env(awb_environment_t *p_awb_env);
int img_set_awb_env(awb_environment_t *p_awb_env);
int img_set_awb_manual_meter_roi(int* roi);


//af
int img_register_lens_drv(lens_dev_drv_t custom_lens_drv);
int img_lens_init(void);
int img_config_lens_info(lens_ID lensID);
int img_load_lens_param(lens_param_t *p_lens_param);
int img_config_lens_cali(lens_cali_t *p_lens_cali);
int img_af_set_range(af_range_t * p_af_range);
int img_af_set_mode(af_mode mode);
int img_af_set_roi(af_ROI_config_t* af_ROI);
int img_af_set_zoom_idx(u16 zoom_idx);
int img_af_set_focus_idx(s32 focus_idx);
int img_af_set_focus_offset(s8 focus_offset);
//int img_af_set_reset(void);
void img_af_load_param(void* p_af_param, void* p_zoom_map);
int img_af_set_statistics(int fd_iav, af_statistics_ex_t *paf_ex);
int img_af_set_brake(void);	//force brake while zoom in or out
int img_af_set_debug(s32 debug); //only for internal debug
int img_af_get_work_state(u16 zoom_idx); //get af working state on specific zoom index
int img_af_set_aux_zoom(u8 aux_zoom_enb); //set aux zoom enable/disable
int img_af_set_one_push(void); //only valid in SAF mode
int img_af_set_interval_time(u8 time_sec); //set IAF mode interval time in second (time_sec>0)
int img_af_set_sensitivity(u16 sensitivity); //set CAF mode AF trigger threshold (2000<sensitivity<16000)

// hdr
int img_set_hdr_blend_config(hdr_blend_info_t *p_blend_cfg);
int img_get_hdr_blend_config(hdr_blend_info_t *p_blend_cfg);
int img_set_hdr_linearization_mode(hdr_linearization_mode_t mode);
hdr_linearization_mode_t img_get_hdr_linearization_mode(void);
int img_set_hdr_enable(int enable);
int img_get_hdr_enable(void);
int img_set_hdr_alpha_value(hdr_alpha_value_t * p_alpha);
int img_get_hdr_alpha_value(hdr_alpha_value_t * p_alpha);
int img_set_hdr_threshold(hdr_threshold_t * p_thresh);
int img_get_hdr_threshold(hdr_threshold_t * p_thresh);
int img_set_hdr_batch(int fd_iav,amba_img_dsp_mode_cfg_t * p_ik_mode,img_config_info_t * p_img_cfg,hdr_proc_data_t * p_hdr_proc);
int img_get_hdr_batch(int fd_iav, hdr_proc_data_t * p_hdr_proc);

//wdr
int img_set_wdr_enable(int enable);
int img_get_wdr_enable(void);
int img_set_wdr_strength(int strength);
int img_get_wdr_strength(void);
int img_set_wdr_luma_config(wdr_luma_config_info_t *p_config);
int img_get_wdr_luma_config(wdr_luma_config_info_t *p_config);
int img_config_digit_wdr_info(img_aeb_digit_wdr_param_t * config);

//calibartion
int img_cal_vignette(vignette_cal_t *vig_detect_setup);
int img_cali_bad_pixel(int fd_iav, cali_badpix_setup_t *pCali_badpix_setup);

//property
int img_set_color_style(img_color_style mode);
int img_set_color_hue(int hue);			//-15 - +15: -30deg - +30 deg
int img_set_color_brightness(int brightness);	//-255 - +255
int img_set_color_contrast(int contrast);	//unit = 64, 0 ~ 128
int img_set_auto_color_contrast(int enable); //0 : disable, 1 : enable
int img_set_auto_color_contrast_strength(int strength); // strength: 0 ~ 128
int img_set_color_saturation(int saturation);	//unit = 64
int img_set_bw_mode(u8 mode);// 0: diable, 1: enable
int img_get_bw_mode();
int img_set_sharpening_strength(u8 str_level);//default =6, 0-11
int img_get_img_property(image_property_t *p_img_prop);
int img_set_mctf_strength(u8 str_level);//default = 6 ,0 ~11
int img_set_mctf_property_ratio(mctf_property_t* p_mctf_property);//unit: 64,64
int img_set_sharpen_property_ratio(sharpen_property_t* p_shp_property);//unit: 64,64,16,64,fractionalBits =1,2,3
int img_set_wb_property_ratio(u8 wb_r_ratio,u8 wb_b_ratio);
int img_get_wb_property_ratio(u8* p_wb_r_ratio,u8* p_wb_b_ratio);
int img_set_log_enable(u8 enable);//0 :close,1 open


#ifdef __cplusplus
}
#endif

#endif	// IMG_API_H

