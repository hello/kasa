/*
 * dsp_print_cmd.c
 *
 * History:
 *	2012/11/02 - [Cao Rongrong] Created file
 *
 *
 * Copyright (c) 2015 Ambarella, Inc.
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


#include <config.h>
#include "dsp.h"

#ifdef CONFIG_PRINT_DSP_CMD

#ifndef DRV_PRINT
#ifdef BUILD_AMBARELLA_PRIVATE_DRV_MSG
#define DRV_PRINT	print_drv
#else
#define DRV_PRINT	printk
#endif
#endif

#ifdef CONFIG_PRINT_DSP_CMD_MORE
#if defined(CONFIG_PRINT_DSP_CMD_RAW)
#define dsp_cast_cmd(type)	type *dsp_cmd = (type *)cmd; cmd_size = sizeof(type)
#else
#define dsp_cast_cmd(type)	type *dsp_cmd = (type *)cmd
#endif
#define dsp_put_cmd_id(cmd)	DRV_PRINT(KERN_DEBUG "dsp_cmd: "#cmd" (0x%x)\n", cmd)
#define dsp_put_cmd_dec(arg)	DRV_PRINT(KERN_DEBUG "    "#arg" = %d\n", (u32)dsp_cmd->arg)
#define dsp_put_cmd_hex(arg)	DRV_PRINT(KERN_DEBUG "    "#arg" = 0x%08x\n", (u32)dsp_cmd->arg)
#define dsp_put_cmd_array(arg) \
	do { \
		int i; \
		for (i = 0; i < sizeof(dsp_cmd->arg)/sizeof(dsp_cmd->arg[0]); i++) \
			DRV_PRINT(KERN_DEBUG "    "#arg"[%d] = %d\n", i, (u32)dsp_cmd->arg[i]); \
	} while (0)
#define dsp_put_cmd_array_hex(arg) \
	do { \
		int i; \
		for (i = 0; i < sizeof(dsp_cmd->arg)/sizeof(dsp_cmd->arg[0]); i++) \
			DRV_PRINT(KERN_DEBUG "    "#arg"[%d] = 0x%08x\n", i, (u32)dsp_cmd->arg[i]); \
	} while (0)
#if defined(CONFIG_PRINT_DSP_CMD_RAW)
#define dsp_put_cmd_raw()	\
	do { \
		int i; \
		u32 *pcmd_raw = (u32 *)cmd; \
		DRV_PRINT(KERN_DEBUG "[%d] = {\n", cmd_size); \
		for (i = 0; i < ((cmd_size + 3) >> 2); i++) {\
			DRV_PRINT(KERN_DEBUG "0x%08X,\n", pcmd_raw[i]); \
		} \
		DRV_PRINT(KERN_DEBUG "}\n"); \
	} while (0)
#else
#define dsp_put_cmd_raw()	do {} while (0)
#endif
#else
#define dsp_cast_cmd(type)	do {} while (0)
#define dsp_put_cmd_id(cmd)	DRV_PRINT(KERN_DEBUG "dsp_cmd: "#cmd" (0x%x)\n", cmd)
#define dsp_put_cmd_dec(arg)	do {} while (0)
#define dsp_put_cmd_hex(arg)	do {} while (0)
#define dsp_put_cmd_array(arg)	do {} while (0)
#define dsp_put_cmd_array_hex(arg)	do {} while (0)
#define dsp_put_cmd_raw()	do {} while (0)
#endif

void dsp_print_cmd(void *cmd)
{
	u32 code = *(u32*)cmd;
#if defined(CONFIG_PRINT_DSP_CMD_RAW)
	u32 cmd_size = DSP_CMD_SIZE;
#endif

	switch (code & 0x3FFFFFFF) {
	case CHIP_SELECTION:
	{
		dsp_cast_cmd(chip_select_t);
		dsp_put_cmd_id(CHIP_SELECTION);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(chip_type);
		break;
	}
	case RESET_OPERATION:
	{
		dsp_cast_cmd(reset_operation_t);
		dsp_put_cmd_id(RESET_OPERATION);
		dsp_put_cmd_hex(cmd_code);
		break;
	}
	case SYSTEM_SETUP_INFO:
	{
		dsp_cast_cmd(system_setup_info_t);
		dsp_put_cmd_id(SYSTEM_SETUP_INFO);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(preview_A_type);
		dsp_put_cmd_dec(preview_B_type);
		dsp_put_cmd_dec(voutA_osd_blend_enabled);
		dsp_put_cmd_dec(voutB_osd_blend_enabled);
		dsp_put_cmd_dec(pip_vin_enabled);
		dsp_put_cmd_dec(raw_encode_enabled);
		dsp_put_cmd_dec(adv_iso_disabled);
		dsp_put_cmd_dec(sub_mode_sel.multiple_stream);
		dsp_put_cmd_dec(sub_mode_sel.power_mode);
		dsp_put_cmd_dec(lcd_3d);
		dsp_put_cmd_dec(iv_360);
		dsp_put_cmd_dec(mode_flags);
		dsp_put_cmd_dec(audio_clk_freq);
		dsp_put_cmd_dec(idsp_freq);
		dsp_put_cmd_dec(core_freq);
		dsp_put_cmd_dec(sensor_HB_pixel);
		dsp_put_cmd_dec(sensor_VB_pixel);
		dsp_put_cmd_dec(hdr_preblend_from_vin);
		dsp_put_cmd_dec(hdr_num_exposures_minus_1);
		dsp_put_cmd_dec(vout_swap);
		dsp_put_cmd_dec(vin_overflow_protection);
		break;
	}
	case SET_VIN_CAPTURE_WIN:
	{
		dsp_cast_cmd(vin_cap_win_t);
		dsp_put_cmd_id(SET_VIN_CAPTURE_WIN);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_hex(s_ctrl_reg);
		dsp_put_cmd_hex(s_inp_cfg_reg);
		dsp_put_cmd_hex(s_v_width_reg);
		dsp_put_cmd_hex(s_h_width_reg);
		dsp_put_cmd_hex(s_h_offset_top_reg);
		dsp_put_cmd_hex(s_h_offset_bot_reg);
		dsp_put_cmd_hex(s_v_reg);
		dsp_put_cmd_hex(s_h_reg);
		dsp_put_cmd_hex(s_min_v_reg);
		dsp_put_cmd_hex(s_min_h_reg);
		dsp_put_cmd_hex(s_trigger_0_start_reg);
		dsp_put_cmd_hex(s_trigger_0_end_reg);
		dsp_put_cmd_hex(s_trigger_1_start_reg);
		dsp_put_cmd_hex(s_trigger_1_end_reg);
		dsp_put_cmd_hex(s_vout_start_0_reg);
		dsp_put_cmd_hex(s_vout_start_1_reg);
		dsp_put_cmd_hex(s_cap_start_v_reg);
		dsp_put_cmd_hex(s_cap_start_h_reg);
		dsp_put_cmd_hex(s_cap_end_v_reg);
		dsp_put_cmd_hex(s_cap_end_h_reg);
		dsp_put_cmd_hex(s_blank_leng_h_reg);
		dsp_put_cmd_hex(vsync_timeout);
		dsp_put_cmd_hex(hsync_timeout);
		break;
	}
	case SET_VIN_CONFIG:
	{
		dsp_cast_cmd(set_vin_config_t);
		dsp_put_cmd_id(SET_VIN_CONFIG);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(vin_width);
		dsp_put_cmd_dec(vin_height);
		dsp_put_cmd_hex(vin_config_dram_addr);
		dsp_put_cmd_hex(config_data_size);
		dsp_put_cmd_dec(sensor_resolution);
		dsp_put_cmd_dec(sensor_bayer_pattern);
		dsp_put_cmd_dec(vin_set_dly);
		dsp_put_cmd_dec(vin_enhance_mode);
		dsp_put_cmd_dec(vin_panasonic_mode);
		break;
	}
	case SET_VIN_GLOBAL_CLK:
	{
		dsp_cast_cmd(set_vin_global_config_t);
		dsp_put_cmd_id(SET_VIN_GLOBAL_CLK);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(main_or_pip);
		dsp_put_cmd_dec(global_cfg_reg_word0);
		break;
	}
	case SET_VIN_MASTER_CLK:
	{
		dsp_cast_cmd(set_vin_master_t);
		dsp_put_cmd_id(SET_VIN_MASTER_CLK);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(master_sync_reg_word0);
		dsp_put_cmd_dec(master_sync_reg_word1);
		dsp_put_cmd_dec(master_sync_reg_word2);
		dsp_put_cmd_dec(master_sync_reg_word3);
		dsp_put_cmd_dec(master_sync_reg_word4);
		dsp_put_cmd_dec(master_sync_reg_word5);
		dsp_put_cmd_dec(master_sync_reg_word6);
		dsp_put_cmd_dec(master_sync_reg_word7);
		break;
	}
	case VIDEO_PREPROCESSING:
	{
		dsp_cast_cmd(video_preproc_t);
		dsp_put_cmd_id(VIDEO_PREPROCESSING);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(input_format);
		dsp_put_cmd_dec(sensor_id);
		dsp_put_cmd_dec(keep_states);
		dsp_put_cmd_dec(vin_frame_rate);
		dsp_put_cmd_dec(vidcap_w);
		dsp_put_cmd_dec(vidcap_h);
		dsp_put_cmd_dec(main_w);
		dsp_put_cmd_dec(main_h);
		dsp_put_cmd_dec(encode_w);
		dsp_put_cmd_dec(encode_h);
		dsp_put_cmd_dec(encode_x);
		dsp_put_cmd_dec(encode_y);
		dsp_put_cmd_dec(preview_w_A);
		dsp_put_cmd_dec(preview_h_A);
		dsp_put_cmd_dec(input_center_x);
		dsp_put_cmd_dec(input_center_y);
		dsp_put_cmd_dec(zoom_factor_x);
		dsp_put_cmd_dec(zoom_factor_y);
		dsp_put_cmd_dec(aaa_data_fifo_start);
		dsp_put_cmd_dec(sensor_readout_mode);
		dsp_put_cmd_dec(noise_filter_strength);
		dsp_put_cmd_dec(bit_resolution);
		dsp_put_cmd_dec(bayer_pattern);
		dsp_put_cmd_dec(preview_format_A);
		dsp_put_cmd_dec(preview_format_B);
		dsp_put_cmd_dec(no_pipelineflush);
		dsp_put_cmd_dec(preview_frame_rate_A);
		dsp_put_cmd_dec(preview_w_B);
		dsp_put_cmd_dec(preview_h_B);
		dsp_put_cmd_dec(preview_frame_rate_B);
		dsp_put_cmd_dec(preview_A_en);
		dsp_put_cmd_dec(preview_B_en);
		dsp_put_cmd_dec(vin_frame_rate_ext);
		dsp_put_cmd_dec(vdsp_int_factor);
		dsp_put_cmd_dec(main_out_frame_rate);
		dsp_put_cmd_dec(main_out_frame_rate_ext);
		dsp_put_cmd_dec(vid_skip);
		dsp_put_cmd_dec(EIS_delay_count);
		dsp_put_cmd_dec(Vert_WARP_enable);
		dsp_put_cmd_dec(force_stitching);
		dsp_put_cmd_dec(no_vin_reset_exiting);
		dsp_put_cmd_dec(cmdReadDly);
		dsp_put_cmd_dec(preview_src_w_A);
		dsp_put_cmd_dec(preview_src_h_A);
		dsp_put_cmd_dec(preview_src_x_offset_A);
		dsp_put_cmd_dec(preview_src_y_offset_A);
		dsp_put_cmd_dec(preview_src_w_B);
		dsp_put_cmd_dec(preview_src_h_B);
		dsp_put_cmd_dec(preview_src_x_offset_B);
		dsp_put_cmd_dec(preview_src_y_offset_B);
		dsp_put_cmd_hex(still_iso_config_daddr);
		break;
	}
	case SENSOR_INPUT_SETUP:
	{
		dsp_cast_cmd(sensor_input_setup_t);
		dsp_put_cmd_id(SENSOR_INPUT_SETUP);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(sensor_id);
		dsp_put_cmd_dec(field_format);
		dsp_put_cmd_dec(sensor_resolution);
		dsp_put_cmd_dec(sensor_pattern);
		dsp_put_cmd_dec(first_line_field_0);
		dsp_put_cmd_dec(first_line_field_1);
		dsp_put_cmd_dec(first_line_field_2);
		dsp_put_cmd_dec(first_line_field_3);
		dsp_put_cmd_dec(first_line_field_4);
		dsp_put_cmd_dec(first_line_field_5);
		dsp_put_cmd_dec(first_line_field_6);
		dsp_put_cmd_dec(first_line_field_7);
		dsp_put_cmd_dec(sensor_readout_mode);
		break;
	}
	case IPCAM_VIDEO_SYSTEM_SETUP:
	{
		dsp_cast_cmd(ipcam_video_system_setup_t);
		dsp_put_cmd_id(IPCAM_VIDEO_SYSTEM_SETUP);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(main_max_width);
		dsp_put_cmd_dec(main_max_height);
		dsp_put_cmd_dec(preview_A_max_width);
		dsp_put_cmd_dec(preview_A_max_height);
		dsp_put_cmd_dec(preview_B_max_width);
		dsp_put_cmd_dec(preview_B_max_height);
		dsp_put_cmd_dec(preview_C_max_width);
		dsp_put_cmd_dec(preview_C_max_height);
		dsp_put_cmd_dec(stream_0_max_GOP_M);
		dsp_put_cmd_dec(stream_1_max_GOP_M);
		dsp_put_cmd_dec(stream_2_max_GOP_M);
		dsp_put_cmd_dec(stream_3_max_GOP_M);
		dsp_put_cmd_dec(stream_0_max_width);
		dsp_put_cmd_dec(stream_0_max_height);
		dsp_put_cmd_dec(stream_1_max_width);
		dsp_put_cmd_dec(stream_1_max_height);
		dsp_put_cmd_dec(stream_2_max_width);
		dsp_put_cmd_dec(stream_2_max_height);
		dsp_put_cmd_dec(stream_3_max_width);
		dsp_put_cmd_dec(stream_3_max_height);
		dsp_put_cmd_dec(MCTF_possible);
		dsp_put_cmd_dec(max_num_streams);
		dsp_put_cmd_dec(max_num_cap_sources);
		dsp_put_cmd_dec(use_1Gb_DRAM_config);
		dsp_put_cmd_dec(raw_compression_disabled);
		dsp_put_cmd_dec(vwarp_blk_h);
		dsp_put_cmd_dec(extra_buf_main);
		dsp_put_cmd_dec(extra_buf_preview_a);
		dsp_put_cmd_dec(extra_buf_preview_b);
		dsp_put_cmd_dec(extra_buf_preview_c);
		dsp_put_cmd_dec(raw_max_width);
		dsp_put_cmd_dec(raw_max_height);
		dsp_put_cmd_dec(warped_main_max_width);
		dsp_put_cmd_dec(warped_main_max_height);
		dsp_put_cmd_dec(v_warped_main_max_width);
		dsp_put_cmd_dec(v_warped_main_max_height);
		dsp_put_cmd_dec(max_warp_region_input_width);
		dsp_put_cmd_dec(max_warp_region_input_height);
		dsp_put_cmd_dec(max_warp_region_output_width);
		dsp_put_cmd_dec(max_padding_width);
		dsp_put_cmd_dec(enc_rotation_possible);
		dsp_put_cmd_dec(enc_dummy_latency);
		dsp_put_cmd_dec(stream_0_LT_enable);
		dsp_put_cmd_dec(stream_1_LT_enable);
		dsp_put_cmd_dec(stream_2_LT_enable);
		dsp_put_cmd_dec(stream_3_LT_enable);
		dsp_put_cmd_dec(B_frame_enable_in_LT_gop);
		dsp_put_cmd_dec(vca_preview_id);
		dsp_put_cmd_dec(max_warp_region_input_height);
		dsp_put_cmd_dec(vca_frame_num);
		dsp_put_cmd_hex(vca_daddr_base);
		dsp_put_cmd_hex(vca_daddr_size);
		dsp_put_cmd_dec(enc_buf_extra_MB_row_at_top);
		break;
	}
	case IPCAM_OSD_INSERT:
	{
#if 0
		dsp_cast_cmd(ipcam_osd_insert_t);
		dsp_put_cmd_id(IPCAM_OSD_INSERT);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(vout_id);
		dsp_put_cmd_dec(osd_enable);
		dsp_put_cmd_dec(osd_num_regions);
		dsp_put_cmd_dec(osd_enable_ex);
		dsp_put_cmd_dec(osd_num_regions_ex);
		dsp_put_cmd_dec(osd_insert_always);
		dsp_put_cmd_hex(osd_clut_dram_address[0]);
		dsp_put_cmd_hex(osd_buf_dram_address[0]);
		dsp_put_cmd_dec(osd_buf_pitch[0]);
		dsp_put_cmd_dec(osd_win_offset_x[0]);
		dsp_put_cmd_dec(osd_win_offset_y[0]);
		dsp_put_cmd_dec(osd_win_w[0]);
		dsp_put_cmd_dec(osd_win_h[0]);
		dsp_put_cmd_hex(osd_clut_dram_address[1]);
		dsp_put_cmd_hex(osd_buf_dram_address[1]);
		dsp_put_cmd_dec(osd_buf_pitch[1]);
		dsp_put_cmd_dec(osd_win_offset_x[1]);
		dsp_put_cmd_dec(osd_win_offset_y[1]);
		dsp_put_cmd_dec(osd_win_w[1]);
		dsp_put_cmd_dec(osd_win_h[1]);
		dsp_put_cmd_hex(osd_clut_dram_address[2]);
		dsp_put_cmd_hex(osd_buf_dram_address[2]);
		dsp_put_cmd_dec(osd_buf_pitch[2]);
		dsp_put_cmd_dec(osd_win_offset_x[2]);
		dsp_put_cmd_dec(osd_win_offset_y[2]);
		dsp_put_cmd_dec(osd_win_w[2]);
		dsp_put_cmd_dec(osd_win_h[2]);
		dsp_put_cmd_hex(osd_clut_dram_address_ex[0]);
		dsp_put_cmd_hex(osd_buf_dram_address_ex[0]);
		dsp_put_cmd_dec(osd_buf_pitch_ex[0]);
		dsp_put_cmd_dec(osd_win_offset_x_ex[0]);
		dsp_put_cmd_dec(osd_win_offset_y_ex[0]);
		dsp_put_cmd_dec(osd_win_w_ex[0]);
		dsp_put_cmd_dec(osd_win_h_ex[0]);
		dsp_put_cmd_hex(osd_clut_dram_address_ex[1]);
		dsp_put_cmd_hex(osd_buf_dram_address_ex[1]);
		dsp_put_cmd_dec(osd_buf_pitch_ex[1]);
		dsp_put_cmd_dec(osd_win_offset_x_ex[1]);
		dsp_put_cmd_dec(osd_win_offset_y_ex[1]);
		dsp_put_cmd_dec(osd_win_w_ex[1]);
		dsp_put_cmd_dec(osd_win_h_ex[1]);
		dsp_put_cmd_hex(osd_clut_dram_address_ex[2]);
		dsp_put_cmd_hex(osd_buf_dram_address_ex[2]);
		dsp_put_cmd_dec(osd_buf_pitch_ex[2]);
		dsp_put_cmd_dec(osd_win_offset_x_ex[2]);
		dsp_put_cmd_dec(osd_win_offset_y_ex[2]);
		dsp_put_cmd_dec(osd_win_w_ex[2]);
		dsp_put_cmd_dec(osd_win_h_ex[2]);
#endif
		break;
	}
	case IPCAM_FAST_OSD_INSERT:
	{
		dsp_cast_cmd(ipcam_fast_osd_insert_t);
		dsp_put_cmd_id(IPCAM_FAST_OSD_INSERT);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(vout_id);
		dsp_put_cmd_dec(fast_osd_enable);
		dsp_put_cmd_dec(string_num_region);
		dsp_put_cmd_dec(osd_num_region);

		dsp_put_cmd_hex(font_index_addr);
		dsp_put_cmd_hex(font_map_addr);
		dsp_put_cmd_dec(font_map_pitch);
		dsp_put_cmd_dec(font_map_h);

		dsp_put_cmd_array_hex(string_addr);
		dsp_put_cmd_array(string_len);
		dsp_put_cmd_array_hex(string_output_addr);
		dsp_put_cmd_array(string_output_pitch);
		dsp_put_cmd_array_hex(clut_addr);
		dsp_put_cmd_array(string_win_offset_x);
		dsp_put_cmd_array(string_win_offset_y);
		break;
	}
	case VOUT_RESET:
	{
		dsp_cast_cmd(vout_reset_t);
		dsp_put_cmd_id(VOUT_RESET);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(vout_id);
		dsp_put_cmd_dec(reset_mixer);
		dsp_put_cmd_dec(reset_disp);
		break;
	}
	case VOUT_MIXER_SETUP:
	{
		dsp_cast_cmd(vout_mixer_setup_t);
		dsp_put_cmd_id(VOUT_MIXER_SETUP);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(vout_id);
		dsp_put_cmd_dec(interlaced);
		dsp_put_cmd_dec(frm_rate);
		dsp_put_cmd_dec(act_win_width);
		dsp_put_cmd_dec(act_win_height);
		dsp_put_cmd_dec(back_ground_v);
		dsp_put_cmd_dec(back_ground_u);
		dsp_put_cmd_dec(back_ground_y);
		dsp_put_cmd_dec(mixer_444);
		dsp_put_cmd_dec(highlight_v);
		dsp_put_cmd_dec(highlight_u);
		dsp_put_cmd_dec(highlight_y);
		dsp_put_cmd_dec(highlight_thresh);
		dsp_put_cmd_dec(reverse_en);
		dsp_put_cmd_dec(csc_en);
		dsp_put_cmd_array(csc_parms);
		break;
	}
	case VOUT_VIDEO_SETUP:
	{
		dsp_cast_cmd(vout_video_setup_t);
		dsp_put_cmd_id(VOUT_VIDEO_SETUP);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(vout_id);
		dsp_put_cmd_dec(en);
		dsp_put_cmd_dec(src);
		dsp_put_cmd_dec(flip);
		dsp_put_cmd_dec(rotate);
		dsp_put_cmd_dec(data_src);
		dsp_put_cmd_dec(win_offset_x);
		dsp_put_cmd_dec(win_offset_y);
		dsp_put_cmd_dec(win_width);
		dsp_put_cmd_dec(win_height);
		dsp_put_cmd_hex(default_img_y_addr);
		dsp_put_cmd_hex(default_img_uv_addr);
		dsp_put_cmd_dec(default_img_pitch);
		dsp_put_cmd_dec(default_img_repeat_field);
		break;
	}
	case VOUT_OSD_SETUP:
	{
		dsp_cast_cmd(vout_osd_setup_t);
		dsp_put_cmd_id(VOUT_OSD_SETUP);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(vout_id);
		dsp_put_cmd_dec(en);
		dsp_put_cmd_dec(src);
		dsp_put_cmd_dec(flip);
		dsp_put_cmd_dec(rescaler_en);
		dsp_put_cmd_dec(premultiplied);
		dsp_put_cmd_dec(global_blend);
		dsp_put_cmd_dec(win_offset_x);
		dsp_put_cmd_dec(win_offset_y);
		dsp_put_cmd_dec(win_width);
		dsp_put_cmd_dec(win_height);
		dsp_put_cmd_dec(rescaler_input_width);
		dsp_put_cmd_dec(rescaler_input_height);
		dsp_put_cmd_hex(osd_buf_dram_addr);
		dsp_put_cmd_dec(osd_buf_pitch);
		dsp_put_cmd_dec(osd_buf_repeat_field);
		dsp_put_cmd_dec(osd_direct_mode);
		dsp_put_cmd_dec(osd_transparent_color);
		dsp_put_cmd_dec(osd_transparent_color_en);
		dsp_put_cmd_dec(osd_swap_bytes);
		dsp_put_cmd_hex(osd_buf_info_dram_addr);
		dsp_put_cmd_dec(osd_video_en);
		dsp_put_cmd_dec(osd_video_mode);
		dsp_put_cmd_dec(osd_video_win_offset_x);
		dsp_put_cmd_dec(osd_video_win_offset_y);
		dsp_put_cmd_dec(osd_video_win_width);
		dsp_put_cmd_dec(osd_video_win_height);
		dsp_put_cmd_dec(osd_video_buf_pitch);
		dsp_put_cmd_hex(osd_video_buf_dram_addr);
		dsp_put_cmd_hex(osd_buf_dram_sync_addr);
		break;
	}
	case VOUT_DISPLAY_SETUP:
	{
		dsp_cast_cmd(vout_display_setup_t);
		dsp_put_cmd_id(VOUT_DISPLAY_SETUP);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(vout_id);
		dsp_put_cmd_dec(dual_vout_vysnc_delay_ms_x10);
		dsp_put_cmd_hex(disp_config_dram_addr);
		break;
	}
	case VOUT_DVE_SETUP:
	{
		dsp_cast_cmd(vout_dve_setup_t);
		dsp_put_cmd_id(VOUT_DVE_SETUP);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(vout_id);
		dsp_put_cmd_hex(dve_config_dram_addr);
		break;
	}
	case SET_WARP_CONTROL:
	{
		dsp_cast_cmd(set_warp_control_t);
		dsp_put_cmd_id(SET_WARP_CONTROL);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(zoom_x);
		dsp_put_cmd_dec(zoom_y);
		dsp_put_cmd_dec(x_center_offset);
		dsp_put_cmd_dec(y_center_offset);
		dsp_put_cmd_dec(actual_left_top_x);
		dsp_put_cmd_dec(actual_left_top_y);
		dsp_put_cmd_dec(actual_right_bot_x);
		dsp_put_cmd_dec(actual_right_bot_y);
		dsp_put_cmd_dec(dummy_window_x_left);
		dsp_put_cmd_dec(dummy_window_y_top);
		dsp_put_cmd_dec(dummy_window_width);
		dsp_put_cmd_dec(dummy_window_height);
		dsp_put_cmd_dec(cfa_output_width);
		dsp_put_cmd_dec(cfa_output_height);
		dsp_put_cmd_dec(warp_control);
		dsp_put_cmd_hex(warp_horizontal_table_address);
		dsp_put_cmd_hex(warp_vertical_table_address);
		dsp_put_cmd_dec(grid_array_width);
		dsp_put_cmd_dec(grid_array_height);
		dsp_put_cmd_dec(horz_grid_spacing_exponent);
		dsp_put_cmd_dec(vert_grid_spacing_exponent);
		dsp_put_cmd_dec(vert_warp_enable);
		dsp_put_cmd_dec(vert_warp_grid_array_width);
		dsp_put_cmd_dec(vert_warp_grid_array_height);
		dsp_put_cmd_dec(vert_warp_horz_grid_spacing_exponent);
		dsp_put_cmd_dec(vert_warp_vert_grid_spacing_exponent);
		dsp_put_cmd_dec(ME1_vwarp_grid_array_width);
		dsp_put_cmd_dec(ME1_vwarp_grid_array_height);
		dsp_put_cmd_dec(ME1_vwarp_horz_grid_spacing_exponent);
		dsp_put_cmd_dec(ME1_vwarp_vert_grid_spacing_exponent);
		dsp_put_cmd_hex(ME1_vwarp_table_address);
		break;
	}
	case H264_ENCODING_SETUP:
	{
		dsp_cast_cmd(h264_encode_setup_t);
		dsp_put_cmd_id(H264_ENCODING_SETUP);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(mode);
		dsp_put_cmd_dec(M);
		dsp_put_cmd_dec(N);
		dsp_put_cmd_dec(quality);
		dsp_put_cmd_dec(average_bitrate);
		dsp_put_cmd_dec(vbr_cntl);
		dsp_put_cmd_dec(vbr_setting);
		dsp_put_cmd_dec(allow_I_adv);
		dsp_put_cmd_dec(cpb_buf_idc);
		dsp_put_cmd_dec(en_panic_rc);
		dsp_put_cmd_dec(cpb_cmp_idc);
		dsp_put_cmd_dec(fast_rc_idc);
		dsp_put_cmd_dec(target_storage_space);
		dsp_put_cmd_hex(bits_fifo_base);
		dsp_put_cmd_hex(bits_fifo_limit);
		dsp_put_cmd_hex(info_fifo_base);
		dsp_put_cmd_hex(info_fifo_limit);
		dsp_put_cmd_dec(audio_in_freq);
		dsp_put_cmd_dec(vin_frame_rate);
		dsp_put_cmd_dec(encoder_frame_rate);
		dsp_put_cmd_dec(frame_sync);
		dsp_put_cmd_dec(initial_fade_in_gain);
		dsp_put_cmd_dec(final_fade_out_gain);
		dsp_put_cmd_dec(idr_interval);
		dsp_put_cmd_dec(cpb_user_size);
		dsp_put_cmd_dec(numRef_P);
		dsp_put_cmd_dec(numRef_B);
		dsp_put_cmd_dec(vin_frame_rate_ext);
		dsp_put_cmd_dec(encoder_frame_rate_ext);
		dsp_put_cmd_dec(N_msb);
		dsp_put_cmd_dec(fast_seek_interval);
		dsp_put_cmd_dec(vbr_cbp_rate);
		dsp_put_cmd_dec(frame_rate_division_factor);
		dsp_put_cmd_dec(force_intlc_tb_iframe);
		dsp_put_cmd_dec(session_id);
		dsp_put_cmd_dec(frame_rate_multiplication_factor);
		dsp_put_cmd_dec(hflip);
		dsp_put_cmd_dec(vflip);
		dsp_put_cmd_dec(rotate);
		dsp_put_cmd_dec(chroma_format);
		dsp_put_cmd_dec(custom_encoder_frame_rate);
		dsp_put_cmd_hex(mvdump_daddr);
		dsp_put_cmd_hex(mvdump_fifo_limit);
		dsp_put_cmd_dec(mvdump_fifo_unit_sz);
		dsp_put_cmd_dec(mvdump_dpitch);
		break;
	}
	case JPEG_ENCODING_SETUP:
	{
		dsp_cast_cmd(jpeg_encode_setup_t);
		dsp_put_cmd_id(JPEG_ENCODING_SETUP);
		dsp_put_cmd_dec(chroma_format);
		dsp_put_cmd_hex(bits_fifo_base);
		dsp_put_cmd_hex(bits_fifo_limit);
		dsp_put_cmd_hex(info_fifo_base);
		dsp_put_cmd_hex(info_fifo_limit);
		dsp_put_cmd_hex(quant_matrix_addr);
		dsp_put_cmd_hex(frame_rate);
		dsp_put_cmd_dec(mctf_mode);
		dsp_put_cmd_dec(is_mjpeg);
		dsp_put_cmd_dec(frame_rate_division_factor);
		dsp_put_cmd_dec(frame_rate_multiplication_factor);
		dsp_put_cmd_dec(session_id);
		dsp_put_cmd_dec(hflip);
		dsp_put_cmd_dec(vflip);
		dsp_put_cmd_dec(rotate);
		dsp_put_cmd_dec(targ_bits_pp);
		dsp_put_cmd_dec(initial_ql);
		dsp_put_cmd_dec(tolerance);
		dsp_put_cmd_dec(max_recode_lp);
		dsp_put_cmd_dec(total_sample_pts);
		dsp_put_cmd_dec(rate_curve_dram_addr);
		dsp_put_cmd_dec(screen_thumbnail_w);
		dsp_put_cmd_dec(screen_thumbnail_h);
		dsp_put_cmd_dec(screen_thumbnail_active_w);
		dsp_put_cmd_dec(screen_thumbnail_active_h);
		break;
	}
	case H264_ENCODE:
	{
		dsp_cast_cmd(h264_encode_t);
		dsp_put_cmd_id(H264_ENCODE);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_hex(bits_fifo_next);
		dsp_put_cmd_hex(info_fifo_next);
		dsp_put_cmd_dec(start_encode_frame_no);
		dsp_put_cmd_dec(encode_duration);
		dsp_put_cmd_dec(is_flush);
		dsp_put_cmd_dec(enable_slow_shutter);
		dsp_put_cmd_dec(res_rate_min);
		dsp_put_cmd_dec(alpha);
		dsp_put_cmd_dec(beta);
		dsp_put_cmd_dec(en_loop_filter);
		dsp_put_cmd_dec(max_upsampling_rate);
		dsp_put_cmd_dec(slow_shutter_upsampling_rate);
		dsp_put_cmd_dec(frame_cropping_flag);
		dsp_put_cmd_dec(profile);
		dsp_put_cmd_dec(frame_crop_left_offset);
		dsp_put_cmd_dec(frame_crop_right_offset);
		dsp_put_cmd_dec(frame_crop_top_offset);
		dsp_put_cmd_dec(frame_crop_bottom_offset);
		dsp_put_cmd_dec(num_ref_frame);
		dsp_put_cmd_dec(log2_max_frame_num_minus4);
		dsp_put_cmd_dec(log2_max_pic_order_cnt_lsb_minus4);
		dsp_put_cmd_dec(sony_avc);
		dsp_put_cmd_dec(gaps_in_frame_num_value_allowed_flag);
		dsp_put_cmd_dec(custom_bitstream_restriction_cfg);
		dsp_put_cmd_dec(height_mjpeg_h264_simultaneous);
		dsp_put_cmd_dec(width_mjpeg_h264_simultaneous);
		dsp_put_cmd_dec(vui_enable);
		dsp_put_cmd_dec(aspect_ratio_info_present_flag);
		dsp_put_cmd_dec(overscan_info_present_flag);
		dsp_put_cmd_dec(overscan_appropriate_flag);
		dsp_put_cmd_dec(video_signal_type_present_flag);
		dsp_put_cmd_dec(video_full_range_flag);
		dsp_put_cmd_dec(colour_description_present_flag);
		dsp_put_cmd_dec(chroma_loc_info_present_flag);
		dsp_put_cmd_dec(timing_info_present_flag);
		dsp_put_cmd_dec(fixed_frame_rate_flag);
		dsp_put_cmd_dec(nal_hrd_parameters_present_flag);
		dsp_put_cmd_dec(vcl_hrd_parameters_present_flag);
		dsp_put_cmd_dec(low_delay_hrd_flag);
		dsp_put_cmd_dec(pic_struct_present_flag);
		dsp_put_cmd_dec(bitstream_restriction_flag);
		dsp_put_cmd_dec(motion_vectors_over_pic_boundaries_flag);
		dsp_put_cmd_dec(SAR_width);
		dsp_put_cmd_dec(SAR_height);
		dsp_put_cmd_dec(video_format);
		dsp_put_cmd_dec(colour_primaries);
		dsp_put_cmd_dec(transfer_characteristics);
		dsp_put_cmd_dec(matrix_coefficients);
		dsp_put_cmd_dec(chroma_sample_loc_type_top_field);
		dsp_put_cmd_dec(chroma_sample_loc_type_bottom_field);
		dsp_put_cmd_dec(aspect_ratio_idc);
		dsp_put_cmd_dec(max_bytes_per_pic_denom);
		dsp_put_cmd_dec(max_bits_per_mb_denom);
		dsp_put_cmd_dec(log2_max_mv_length_horizontal);
		dsp_put_cmd_dec(log2_max_mv_length_vertical);
		dsp_put_cmd_dec(num_reorder_frames);
		dsp_put_cmd_dec(max_dec_frame_buffering);
		dsp_put_cmd_dec(I_IDR_sp_rc_handle_mask);
		dsp_put_cmd_dec(IDR_QP_adj_str);
		dsp_put_cmd_dec(IDR_class_adj_limit);
		dsp_put_cmd_dec(I_QP_adj_str);
		dsp_put_cmd_dec(I_class_adj_limit);
		dsp_put_cmd_dec(firstGOPstartB);
		dsp_put_cmd_dec(au_type);
		break;
	}
	case MJPEG_ENCODE:
	{
		dsp_cast_cmd(mjpeg_capture_t);
		dsp_put_cmd_id(MJPEG_ENCODE);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_hex(bits_fifo_next);
		dsp_put_cmd_hex(info_fifo_next);
		dsp_put_cmd_dec(start_encode_frame_no);
		dsp_put_cmd_dec(encode_duration);
		dsp_put_cmd_dec(framerate_control_M);
		dsp_put_cmd_dec(framerate_control_N);
		break;
	}
	case ENCODING_STOP:
	{
		dsp_cast_cmd(h264_encode_stop_t);
		dsp_put_cmd_id(ENCODING_STOP);
		dsp_put_cmd_dec(stop_method);
		break;
	}
	case IPCAM_VIDEO_ENCODE_SIZE_SETUP:
	{
		dsp_cast_cmd(ipcam_video_encode_size_setup_t);
		dsp_put_cmd_id(IPCAM_VIDEO_ENCODE_SIZE_SETUP);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(capture_source);
		dsp_put_cmd_dec(enc_x);
		dsp_put_cmd_dec(enc_y);
		dsp_put_cmd_dec(enc_width);
		dsp_put_cmd_dec(enc_height);
		break;
	}
	case IPCAM_REAL_TIME_ENCODE_PARAM_SETUP:
	{
		dsp_cast_cmd(ipcam_real_time_encode_param_setup_t);
		dsp_put_cmd_id(IPCAM_REAL_TIME_ENCODE_PARAM_SETUP);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_hex(enable_flags);
		dsp_put_cmd_dec(cbr_modify);
		dsp_put_cmd_dec(custom_encoder_frame_rate);
		dsp_put_cmd_dec(frame_rate_division_factor);
		dsp_put_cmd_dec(qp_min_on_I);
		dsp_put_cmd_dec(qp_max_on_I);
		dsp_put_cmd_dec(qp_min_on_P);
		dsp_put_cmd_dec(qp_max_on_P);
		dsp_put_cmd_dec(qp_min_on_B);
		dsp_put_cmd_dec(qp_max_on_B);
		dsp_put_cmd_dec(aqp);
		dsp_put_cmd_dec(frame_rate_multiplication_factor);
		dsp_put_cmd_dec(i_qp_reduce);
		dsp_put_cmd_hex(skip_flags);
		dsp_put_cmd_dec(M);
		dsp_put_cmd_dec(N);
		dsp_put_cmd_dec(p_qp_reduce);
		dsp_put_cmd_dec(intra_refresh_num_mb_row);
		dsp_put_cmd_dec(preview_A_frame_rate_divison_factor);
		dsp_put_cmd_dec(idr_interval);
		dsp_put_cmd_dec(custom_vin_frame_rate);
		dsp_put_cmd_hex(roi_daddr);
		dsp_put_cmd_array(roi_delta[0]);
		dsp_put_cmd_array(roi_delta[1]);
		dsp_put_cmd_array(roi_delta[2]);
		dsp_put_cmd_dec(panic_div);
		dsp_put_cmd_dec(is_monochrome);
		dsp_put_cmd_dec(scene_change_detect_on);
		dsp_put_cmd_dec(flat_area_improvement_on);
		dsp_put_cmd_dec(drop_frame);
		dsp_put_cmd_dec(mvdump_enable);
		dsp_put_cmd_dec(pic_size_control);
		dsp_put_cmd_hex(quant_matrix_addr);
		dsp_put_cmd_dec(P_IntraBiasAdd);
		dsp_put_cmd_dec(B_IntraBiasAdd);
		dsp_put_cmd_dec(zmv_threshold);
		dsp_put_cmd_dec(N_msb);
		dsp_put_cmd_dec(idsp_frame_rate_M);
		dsp_put_cmd_dec(idsp_frame_rate_N);
		dsp_put_cmd_dec(roi_daddr_p);
		dsp_put_cmd_dec(roi_daddr_b);
		dsp_put_cmd_dec(user1_intra_bias);
		dsp_put_cmd_dec(user1_direct_bias);
		dsp_put_cmd_dec(user2_intra_bias);
		dsp_put_cmd_dec(user2_direct_bias);
		dsp_put_cmd_dec(user3_intra_bias);
		dsp_put_cmd_dec(user3_direct_bias);
		dsp_put_cmd_dec(force_pskip_num_plus1);
		dsp_put_cmd_dec(set_I_size);
		dsp_put_cmd_dec(q_qp_reduce);
		dsp_put_cmd_dec(qp_min_on_Q);
		dsp_put_cmd_dec(qp_max_on_Q);
		dsp_put_cmd_dec(log_q_num_per_gop_plus_1);
		break;
	}
	case IPCAM_ENC_SYNC_CMD:
	{
		dsp_cast_cmd(enc_sync_cmd_t);
		dsp_put_cmd_id(IPCAM_ENC_SYNC_CMD);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(target_pts);
		dsp_put_cmd_hex(cmd_daddr);
		dsp_put_cmd_hex(enable_flag);
		dsp_put_cmd_hex(force_update_flag);
		break;
	}
	case IPCAM_GET_FRAME_BUF_POOL_INFO:
	{
		dsp_cast_cmd(ipcam_get_frame_buf_pool_info_t);
		dsp_put_cmd_id(IPCAM_GET_FRAME_BUF_POOL_INFO);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_hex(frm_buf_pool_info_req_bit_flag);
		break;
	}
	case IPCAM_VIDEO_CAPTURE_PREVIEW_SIZE_SETUP:
	{
		dsp_cast_cmd(ipcam_capture_preview_size_setup_t);
		dsp_put_cmd_id(IPCAM_VIDEO_CAPTURE_PREVIEW_SIZE_SETUP);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(preview_id);
		dsp_put_cmd_dec(output_scan_format);
		dsp_put_cmd_dec(deinterlace_mode);
		dsp_put_cmd_dec(disabled);
		dsp_put_cmd_dec(skip_interval);
		dsp_put_cmd_dec(cap_width);
		dsp_put_cmd_dec(cap_height);
		dsp_put_cmd_dec(input_win_offset_x);
		dsp_put_cmd_dec(input_win_offset_y);
		dsp_put_cmd_dec(input_win_width);
		dsp_put_cmd_dec(input_win_height);
		break;
	}
	case IPCAM_SET_PRIVACY_MASK:
	{
		dsp_cast_cmd(ipcam_set_privacy_mask_t);
		dsp_put_cmd_id(IPCAM_SET_PRIVACY_MASK);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_hex(enabled_flags_dram_address);
		dsp_put_cmd_dec(Y);
		dsp_put_cmd_dec(U);
		dsp_put_cmd_dec(V);
		break;
	}
	case IPCAM_EFM_CREATE_FRAME_BUF_POOL:
	{
		dsp_cast_cmd(ipcam_efm_creat_frm_buf_pool_cmd_t);
		dsp_put_cmd_id(IPCAM_EFM_CREATE_FRAME_BUF_POOL);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(buf_width);
		dsp_put_cmd_dec(buf_height);
		dsp_put_cmd_hex(fbp_type);
		dsp_put_cmd_dec(max_num_yuv);
		dsp_put_cmd_dec(max_num_me1);
		break;
	}
	case IPCAM_EFM_REQ_FRAME_BUF:
	{
		dsp_cast_cmd(ipcam_efm_req_frm_buf_cmd_t);
		dsp_put_cmd_id(IPCAM_EFM_REQ_FRAME_BUF);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_hex(fbp_type);
		break;
	}
	case IPCAM_EFM_HANDSHAKE:
	{
		dsp_cast_cmd(ipcam_efm_handshake_cmd_t);
		dsp_put_cmd_id(IPCAM_EFM_HANDSHAKE);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(yuv_fId);
		dsp_put_cmd_dec(me1_fId);
		dsp_put_cmd_dec(pts);
		break;
	}
	case RAW_ENCODE_VIDEO_SETUP:
	{
		dsp_cast_cmd(raw_encode_video_setup_cmd_t);
		dsp_put_cmd_id(RAW_ENCODE_VIDEO_SETUP);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_hex(sensor_raw_start_daddr);
		dsp_put_cmd_hex(daddr_offset);
		dsp_put_cmd_dec(dpitch);
		dsp_put_cmd_dec(num_frames);
		break;
	}
	case H264_ENC_USE_TIMER:
	{
		dsp_cast_cmd(vin_timer_mode_t);
		dsp_put_cmd_id(H264_ENC_USE_TIMER);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(timer_scaler);
		dsp_put_cmd_dec(display_opt);
		dsp_put_cmd_dec(video_term_opt);
		break;
	}

	case H264_DECODING_SETUP:
	{
		dsp_cast_cmd(h264_decode_setup_t);
		dsp_put_cmd_id(H264_DECODING_SETUP);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_hex(bits_fifo_base);
		dsp_put_cmd_hex(bits_fifo_limit);
		dsp_put_cmd_hex(fade_in_pic_addr);
		dsp_put_cmd_dec(fade_in_pic_pitch);
		dsp_put_cmd_dec(fade_in_alpha_start);
		dsp_put_cmd_dec(fade_in_alpha_step);
		dsp_put_cmd_dec(fade_in_total_frames);

		dsp_put_cmd_hex(fade_out_pic_addr);
		dsp_put_cmd_dec(fade_out_pic_pitch);

		dsp_put_cmd_dec(fade_out_alpha_start);
		dsp_put_cmd_dec(fade_out_alpha_step);
		dsp_put_cmd_dec(fade_out_total_frames);

		dsp_put_cmd_dec(cabac_to_recon_delay);
		dsp_put_cmd_dec(forced_max_fb_size);
		dsp_put_cmd_dec(iv_360);
		dsp_put_cmd_dec(dual_lcd);

		dsp_put_cmd_dec(bit_rate);
		dsp_put_cmd_dec(video_max_width);
		dsp_put_cmd_dec(video_max_height);
		dsp_put_cmd_dec(gop_n);

		dsp_put_cmd_dec(gop_m);
		dsp_put_cmd_dec(dpb_size);
		dsp_put_cmd_dec(customer_mem_usage);

		dsp_put_cmd_dec(trickplay_fw);
		dsp_put_cmd_dec(trickplay_ff_IP);
		dsp_put_cmd_dec(trickplay_ff_I);
		dsp_put_cmd_dec(trickplay_bw);
		dsp_put_cmd_dec(trickplay_fb_IP);
		dsp_put_cmd_dec(trickplay_fb_I);
		dsp_put_cmd_dec(trickplay_rsv);

		dsp_put_cmd_dec(ena_stitch_flow);
		dsp_put_cmd_dec(vout0_win_width);
		dsp_put_cmd_dec(vout0_win_height);
		dsp_put_cmd_dec(vout1_win_width);
		dsp_put_cmd_dec(vout1_win_height);
		break;
	}

	case DECODE_STOP:
	{
		dsp_cast_cmd(h264_decode_stop_t);
		dsp_put_cmd_id(DECODE_STOP);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(stop_flag);
		break;
	}

	case RESCALE_POSTPROCESSING:
	{
		dsp_cast_cmd(rescale_postproc_t);
		dsp_put_cmd_id(RESCALE_POSTPROCESSING);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(input_center_x);
		dsp_put_cmd_dec(input_center_y);
		dsp_put_cmd_dec(display_win_offset_x);
		dsp_put_cmd_dec(display_win_offset_y);
		dsp_put_cmd_dec(display_win_width);
		dsp_put_cmd_dec(display_win_height);
		dsp_put_cmd_dec(zoom_factor_x);
		dsp_put_cmd_dec(zoom_factor_y);
		dsp_put_cmd_dec(apply_yuv);
		dsp_put_cmd_dec(apply_luma);
		dsp_put_cmd_dec(apply_noise);
		dsp_put_cmd_dec(pip_enable);
		dsp_put_cmd_dec(pip_x_offset);
		dsp_put_cmd_dec(pip_y_offset);
		dsp_put_cmd_dec(pip_x_size);
		dsp_put_cmd_dec(pip_y_size);
		dsp_put_cmd_dec(sec_display_win_offset_x);
		dsp_put_cmd_dec(sec_display_win_offset_y);
		dsp_put_cmd_dec(sec_display_win_width);
		dsp_put_cmd_dec(sec_display_win_height);
		dsp_put_cmd_hex(sec_zoom_factor_x);
		dsp_put_cmd_hex(sec_zoom_factor_y);

		dsp_put_cmd_dec(postp_start_degree);
		dsp_put_cmd_dec(postp_end_degree);
		dsp_put_cmd_dec(horz_warp_enable);
		dsp_put_cmd_dec(vert_warp_enable);

		dsp_put_cmd_hex(warp_horizontal_table_address);
		dsp_put_cmd_hex(warp_vertical_table_address);

		dsp_put_cmd_dec(grid_array_width);
		dsp_put_cmd_dec(grid_array_height);
		dsp_put_cmd_dec(horz_grid_spacing_exponent);
		dsp_put_cmd_dec(vert_grid_spacing_exponent);

		dsp_put_cmd_dec(vert_warp_grid_array_width);
		dsp_put_cmd_dec(vert_warp_grid_array_height);
		dsp_put_cmd_dec(vert_warp_horz_grid_spacing_exponent);
		dsp_put_cmd_dec(vert_warp_vert_grid_spacing_exponent);

		dsp_put_cmd_dec(vout0_flip);
		dsp_put_cmd_dec(vout0_rotate);
		dsp_put_cmd_dec(vout0_win_offset_x);
		dsp_put_cmd_dec(vout0_win_offset_y);
		dsp_put_cmd_dec(vout0_win_width);
		dsp_put_cmd_dec(vout0_win_height);

		dsp_put_cmd_dec(vout1_flip);
		dsp_put_cmd_dec(vout1_rotate);
		dsp_put_cmd_dec(vout1_win_offset_x);
		dsp_put_cmd_dec(vout1_win_offset_y);
		dsp_put_cmd_dec(vout1_win_width);
		dsp_put_cmd_dec(vout1_win_height);

		dsp_put_cmd_dec(left_input_offset_x);
		dsp_put_cmd_dec(left_input_offset_y);
		dsp_put_cmd_dec(left_input_width);
		dsp_put_cmd_dec(left_input_height);
		dsp_put_cmd_dec(left_output_offset_x);
		dsp_put_cmd_dec(left_output_offset_y);
		dsp_put_cmd_dec(left_output_width);
		dsp_put_cmd_dec(left_output_height);

		dsp_put_cmd_dec(right_input_offset_x);
		dsp_put_cmd_dec(right_input_offset_y);
		dsp_put_cmd_dec(right_input_width);
		dsp_put_cmd_dec(right_input_height);
		dsp_put_cmd_dec(right_output_offset_x);
		dsp_put_cmd_dec(right_output_offset_y);
		dsp_put_cmd_dec(right_output_width);
		dsp_put_cmd_dec(right_output_height);

		dsp_put_cmd_dec(postp_start_radius);
		dsp_put_cmd_dec(postp_end_radius);
		break;
	}

	case H264_PLAYBACK_SPEED:
	{
		dsp_cast_cmd(h264_playback_speed_t);
		dsp_put_cmd_id(H264_PLAYBACK_SPEED);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_hex(speed);
		dsp_put_cmd_dec(scan_mode);
		dsp_put_cmd_dec(direction);
		break;
	}

	case H264_DECODE:
	{
		dsp_cast_cmd(h264_decode_t);
		dsp_put_cmd_id(H264_DECODE);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_hex(bits_fifo_start);
		dsp_put_cmd_hex(bits_fifo_end);
		dsp_put_cmd_dec(num_pics);
		dsp_put_cmd_dec(num_frame_decode);
		dsp_put_cmd_dec(first_frame_display);
		dsp_put_cmd_dec(fade_in_on);
		dsp_put_cmd_dec(fade_out_on);
		break;
	}

	case H264_DECODE_BITS_FIFO_UPDATE:
	{
#if 0
		dsp_cast_cmd(h264_decode_bits_fifo_update_t);
		dsp_put_cmd_id(H264_DECODE_BITS_FIFO_UPDATE);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_hex(bits_fifo_start);
		dsp_put_cmd_hex(bits_fifo_end);
		dsp_put_cmd_dec(num_pics);
#endif
		break;
	}

	case H264_TRICKPLAY:
	{
		dsp_cast_cmd(h264_trickplay_t);
		dsp_put_cmd_id(H264_TRICKPLAY);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(mode);
		break;
	}

	case BLACK_LEVEL_GLOBAL_OFFSET:
	{
		dsp_cast_cmd(black_level_global_offset_t);
		dsp_put_cmd_id(BLACK_LEVEL_GLOBAL_OFFSET);
		dsp_put_cmd_dec(global_offset_ee);
		dsp_put_cmd_dec(global_offset_eo);
		dsp_put_cmd_dec(global_offset_oe);
		dsp_put_cmd_dec(global_offset_oo);
		dsp_put_cmd_dec(black_level_offset_red);
		dsp_put_cmd_dec(black_level_offset_green);
		dsp_put_cmd_dec(black_level_offset_blue);
		dsp_put_cmd_dec(gain_depedent_offset_red);
		dsp_put_cmd_dec(gain_depedent_offset_green);
		dsp_put_cmd_dec(gain_depedent_offset_blue);
		dsp_put_cmd_dec(def_blk1_red);	 // for S2L
		dsp_put_cmd_dec(def_blk1_green); // for S2L
		dsp_put_cmd_dec(def_blk1_blue);  // for S2L
		break;
	}
	case CHROMA_NOISE_FILTER:
	{
		dsp_cast_cmd(chroma_noise_filter_t);
		dsp_put_cmd_id(CHROMA_NOISE_FILTER);
		dsp_put_cmd_dec(enable);
		dsp_put_cmd_dec(radius);
		dsp_put_cmd_dec(mode);
		dsp_put_cmd_dec(thresh_u);
		dsp_put_cmd_dec(thresh_v);
		dsp_put_cmd_dec(shift_center_u);
		dsp_put_cmd_dec(shift_center_v);
		dsp_put_cmd_dec(shift_ring1);
		dsp_put_cmd_dec(shift_ring2);
		dsp_put_cmd_dec(target_u);
		dsp_put_cmd_dec(target_v);
		break;
	}
	case DIGITAL_GAIN_SATURATION_LEVEL:
	{
		dsp_cast_cmd(digital_gain_level_t);
		dsp_put_cmd_id(DIGITAL_GAIN_SATURATION_LEVEL);
		dsp_put_cmd_dec(level_red);
		dsp_put_cmd_dec(level_green_even);
		dsp_put_cmd_dec(level_green_odd);
		dsp_put_cmd_dec(level_blue);
		break;
	}
	case IPCAM_SET_REGION_WARP_CONTROL:
	{
		dsp_cast_cmd(ipcam_set_region_warp_control_batch_t);
		dsp_put_cmd_id(IPCAM_SET_REGION_WARP_CONTROL);
		dsp_put_cmd_hex(cmd_code);
		dsp_put_cmd_dec(num_regions_minus_1);
		dsp_put_cmd_hex(warp_region_ctrl_daddr);
		break;
	}

	case FIXED_PATTERN_NOISE_CORRECTION:
	{
		dsp_cast_cmd(fixed_pattern_noise_correct_t);
		dsp_put_cmd_id(FIXED_PATTERN_NOISE_CORRECTION);
		dsp_put_cmd_dec(fpn_pixel_mode);
		dsp_put_cmd_dec(row_gain_enable);
		dsp_put_cmd_dec(column_gain_enable);
		dsp_put_cmd_dec(num_of_rows);
		dsp_put_cmd_dec(num_of_cols);
		dsp_put_cmd_dec(fpn_pitch);
		dsp_put_cmd_dec(fpn_pixels_buf_size);
		dsp_put_cmd_hex(fpn_pixels_addr);
		dsp_put_cmd_dec(bad_pixel_a5m_mode);
		break;
	}

	case BLACK_LEVEL_CORRECTION_CONFIG:
	{
		dsp_put_cmd_id(BLACK_LEVEL_CORRECTION_CONFIG);
		break;
	}
	case BLACK_LEVEL_STATE_TABLES:
	{
		dsp_put_cmd_id(BLACK_LEVEL_STATE_TABLES);
		break;
	}
	case BLACK_LEVEL_DETECTION_WINDOW:
	{
		dsp_put_cmd_id(BLACK_LEVEL_DETECTION_WINDOW);
		break;
	}
	case RGB_GAIN_ADJUSTMENT:
	{
		dsp_put_cmd_id(RGB_GAIN_ADJUSTMENT);
		break;
	}
	case VIGNETTE_COMPENSATION:
	{
		dsp_put_cmd_id(VIGNETTE_COMPENSATION);
		break;
	}
	case LOCAL_EXPOSURE:
	{
		dsp_put_cmd_id(LOCAL_EXPOSURE);
		break;
	}
	case COLOR_CORRECTION:
	{
		dsp_put_cmd_id(COLOR_CORRECTION);
		break;
	}
	case RGB_TO_YUV_SETUP:
	{
		dsp_put_cmd_id(RGB_TO_YUV_SETUP);
		break;
	}
	case CHROMA_SCALE:
	{
		dsp_put_cmd_id(CHROMA_SCALE);
		break;
	}
	case RGB_TO_YUV_ADV_SETUP:
	{
		dsp_put_cmd_id(RGB_TO_YUV_ADV_SETUP);
		break;
	}
	case BLACK_LEVEL_AMPLINEAR_HDR:
	{
		dsp_put_cmd_id(BLACK_LEVEL_AMPLINEAR_HDR);
		break;
	}
	case BLACK_LEVEL_GLOBAL_OFFSET_HDR:
	{
		dsp_put_cmd_id(BLACK_LEVEL_GLOBAL_OFFSET_HDR);
		break;
	}
	case BAD_PIXEL_CORRECT_SETUP:
	{
		dsp_cast_cmd(bad_pixel_correct_setup_t);
		dsp_put_cmd_id(BAD_PIXEL_CORRECT_SETUP);
		dsp_put_cmd_dec(correction_mode);
		dsp_put_cmd_hex(hot_pixel_thresh_addr);
		dsp_put_cmd_hex(dark_pixel_thresh_addr);
		break;
	}
	case CFA_NOISE_FILTER:
	{
		dsp_put_cmd_id(CFA_NOISE_FILTER);
		break;
	}
	case MCTF_MV_STAB_SETUP:
	{
		dsp_put_cmd_id(MCTF_MV_STAB_SETUP);
		break;
	}
	case DEMOASIC_FILTER:
	{
		dsp_put_cmd_id(DEMOASIC_FILTER);
		break;
	}
	case STRONG_GRGB_MISMATCH_FILTER:
	{
		dsp_put_cmd_id(STRONG_GRGB_MISMATCH_FILTER);
		break;
	}
	case CHROMA_MEDIAN_FILTER:
	{
		dsp_put_cmd_id(CHROMA_MEDIAN_FILTER);
		break;
	}
	case LUMA_SHARPENING:
	{
		dsp_put_cmd_id(LUMA_SHARPENING);
		break;
	}
	case LUMA_SHARPENING_LINEARIZATION:
	{
		dsp_put_cmd_id(LUMA_SHARPENING_LINEARIZATION);
		break;
	}
	case LUMA_SHARPENING_FIR_CONFIG:
	{
		dsp_put_cmd_id(LUMA_SHARPENING_FIR_CONFIG);
		break;
	}
	case LUMA_SHARPENING_LNL:
	{
		dsp_put_cmd_id(LUMA_SHARPENING_LNL);
		break;
	}
	case LUMA_SHARPENING_BLEND_CONTROL:
	{
		dsp_put_cmd_id(LUMA_SHARPENING_BLEND_CONTROL);
		break;
	}
	case LUMA_SHARPENING_LEVEL_CONTROL:
	{
		dsp_put_cmd_id(LUMA_SHARPENING_LEVEL_CONTROL);
		break;
	}
	case IPCAM_SET_HDR_PROC_CONTROL:
	{
		dsp_put_cmd_id(IPCAM_SET_HDR_PROC_CONTROL);
		break;
	}
	case VIDEO_HISO_CONFIG_UPDATE:
	{
#if 0
		dsp_put_cmd_id(VIDEO_HISO_CONFIG_UPDATE);
#endif
		break;
	}

	case 0x00000000:
		break;
	default:
		DRV_PRINT(KERN_DEBUG "dsp_cmd: cmd code is 0x%08x \n", code);
		break;
	}

	dsp_put_cmd_raw();
}

#else

void dsp_print_cmd(void *cmd)
{
}

#endif

