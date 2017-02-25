/*
 * include/arch_s2l/img_customer_interface_arch.h
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

#ifndef __IMG_CUSTOMER_INTERFACE_H__
#define __IMG_CUSTOMER_INTERFACE_H__



#endif // __IMG_CUSTOMER_INTERFACE_H__

int img_adj_retrieve_filters(int fd_iav, amba_img_dsp_mode_cfg_t* ik_mode);
int img_adj_init_misc(int fd_iav, amba_img_dsp_mode_cfg_t *ik_mode,img_config_info_t* p_img_config);

int  parse_aaa_data(amba_dsp_aaa_statistic_data_t * p_data, hdr_pipeline_t hdr_pipe,
			img_aaa_stat_t * p_stat, aaa_tile_report_t * p_act_tile);
int config_parser_dgain(u32 dgain);
int img_runtime_adj(int fd_iav, amba_img_dsp_mode_cfg_t* ik_mode, int ev_index, wb_gain_t* wb);
int img_adj_pick_up_single_filter(void *filter_param, int filter_size, u8 id);
int adj_config_work_info(img_config_info_t* p_img_config);
int   amba_img_dsp_post_exe_cfg( int fd_iav, amba_img_dsp_mode_cfg_t     *pMode,
                    AMBA_DSP_IMG_CONFIG_EXECUTE_MODE_e ExeMode, UINT32 boot);
int hal_wait_enter_preview(int fd_iav);
int config_parser_stat_tiles(statistics_config_t* config);

int ae_flow_pre_config(int agc,int shutter,int dgain,int iris_gain);//to set ae start point.
//awb
void awb_flow_pre_config(wb_gain_t* wb_gain);//to set awb start point
int awb_flow_control_init(img_awb_param_t* p_awb_param);
int awb_flow_deinit();
int awb_flow_control(img_aaa_stat_t* p_aaa_data, awb_gain_t *p_awb_gain);
int awb_flow_get_manual_gain(wb_gain_t* p_wb_gain);
int awb_flow_set_custom_gain(wb_gain_t* p_wb_gain);
int awb_flow_set_cali_shift(wb_gain_t *p_wb_org_gain, wb_gain_t *p_wb_ref_gain);
int awb_flow_set_cali_thre(u16 *p_r_th, u16 *p_b_th);
int awb_flow_set_speed(u8* p_speed);
int awb_flow_set_failure_remedy(awb_failure_remedy_t* p_remedy);
int awb_flow_get_method(awb_work_method_t* p_method);
int awb_flow_set_method(awb_work_method_t* p_method);
int awb_flow_get_mode(awb_control_mode_t* p_mode);
int awb_flow_set_mode(awb_control_mode_t* p_mode);
int awb_flow_get_environment(awb_environment_t* p_env);
int awb_flow_set_environment(awb_environment_t* p_env);
int awb_flow_set_meter_roi(int* roi);

//adj

//dynamic load containers
int img_adj_update_idle_container(fc_collection_t* src_fc);
void img_adj_flip_containers(void);
int img_adj_sync_containers(void);
int img_adj_reset_filters(void);


//dynamic tone curve
void dynamic_tone_curve_enable(u8 enable);
u8 dynamic_tone_curve_get_status(void);
int dynamic_tone_curve_set_alpha(float alpha);
float dynamic_tone_curve_get_alpha();
int dynamic_tone_curve_control(amba_img_dsp_tone_curve_t* p_tone_curve, img_aaa_stat_t* p_aaa_stat);

//dynamic load cc
int img_adj_update_idle_cc(cc_binary_addr_t* addr);
void img_adj_flip_cc();
void adj_set_cc_reset();
//cc 3d interpolation
int cc_3d_interpolation(void *p_cc_out, void *p_cc_pre, void *p_cc_next, int pre_wgt, int next_wgt);//tips: pre_wgt+next_wgt =1<<16
int img_adj_reset_filter(int index);
int adj_set_saturation(int saturation);
int adj_set_contrast(int contrast);
int adj_set_brightness(int bright);
int adj_set_hue(int hue);
int adj_set_bw_mode(int enble);
int adj_get_bw_mode();
int adj_set_sharpening_level(int level);
int adj_set_mctf_level(int level);
void adj_set_cc_reset();
int adj_set_color_style(img_color_style style);
void  adj_set_mctf_property_ratio(mctf_property_t* p_mctf_property);
void  adj_set_sharpeness_property_ratio(sharpen_property_t* p_shp_property);
void adj_pre_config();
void adj_pre_config_ex(adj_pre_config_t* cfg);
//wdr
int light_optimise_set_enable(u8 enable);
int light_optimise_get_enable(void);
int light_optimise_set_strength(int strength);
int light_optimise_get_strength(void);
int light_optimise_control(amba_img_dsp_local_exposure_t * p_local_expo, img_aaa_stat_t * p_stat_info);

// hdr
int hdr_proc_init(img_config_info_t * p_img_cfg,raw_offset_config_t * p_raw_offset_cfg,hdr_proc_data_t * p_hdr_proc);
int hdr_proc_control(hdr_proc_data_t * p_hdr_proc, awb_gain_t * p_awb_gain, ae_output_t * p_ae_video);
int sync_hdr_raw_offset(ae_output_t * p_ae_video, amba_img_dsp_hdr_raw_offset_t *p_video_proc );
int set_hdr_linearization_mode(hdr_linearization_mode_t mode);
hdr_linearization_mode_t get_hdr_linearization_mode(void);
int set_hdr_blend_config(hdr_blend_info_t *p_blend_cfg);
int get_hdr_blend_config(hdr_blend_info_t *p_blend_cfg );
int set_hdr_alpha_value(hdr_alpha_value_t * p_alpha);
int get_hdr_alpha_value(hdr_alpha_value_t * p_alpha);
int set_hdr_threshold(hdr_threshold_t * p_thresh);
int get_hdr_threshold(hdr_threshold_t * p_thresh);
int config_hdr_bayer_interpolation(u8 enable);


