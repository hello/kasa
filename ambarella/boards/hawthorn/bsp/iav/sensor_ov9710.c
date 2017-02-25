/**
 * boards/hawthorn/bsp/iav/sensor_ov9710.c
 *
 * History:
 *    2014/08/11 - [Cao Rongrong] created file
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


#if defined(CONFIG_S2LMKIWI_IAV_OV9710)
struct vin_reg_8_8 {
	u8 addr;
	u8 data;
};

static struct vin_reg_8_8 ov9710_720p30_regs[] = {
	/*share regs*/
	{0x12, 0x80},
	{0x1e, 0x07},
	{0x5f, 0x18},
	{0x69, 0x04},
	{0x65, 0x2a},
	{0x68, 0x0a},
	{0x39, 0x28},
	{0x4d, 0x90},
	{0xc1, 0x80},
	{0x96, 0x01},
	{0xbc, 0x68},
	{0x97, 0x80},
	{0x37, 0x02},
	{0x38, 0x10},
	{0x4e, 0x55},
	{0x4f, 0x55},
	{0x50, 0x55},
	{0x51, 0x55},
	{0x24, 0x55},
	{0x25, 0x40},
	{0x26, 0xa1},
	{0x13, 0x80},
	{0x14, 0x60},
	{0x41, 0x80},
	{0x6d, 0x02},
	{0x0c, 0x30},
	{0xc2, 0x81},
	{0xcb, 0xa6},
	{0xc3, 0x20},

	/*pll regs*/
	{0x5c, 0x52},
	{0x5d, 0x00},
	{0x11, 0x01},

	/*mod regs*/
	{0x17, 0x25},
	{0x18, 0xa2},
	{0x19, 0x01},
	{0x1a, 0xca},
	{0x03, 0x0A},
	{0x32, 0x07},
	{0x98, 0x00},
	{0x99, 0x00},
	{0x9a, 0x00},
	{0x57, 0x00},
	{0x58, 0xc8},
	{0x59, 0xA0},
	{0x4c, 0x13},
	{0x4B, 0x36},
	{0x3d, 0x3c},
	{0x3e, 0x03},
	{0xbd, 0xa0},
	{0xbe, 0xc8},
	{0x12, 0x00},
	{0x3b, 0x00},
	{0x2a, 0x9b},
	{0x2b, 0x06},
};

static u32 set_vin_config_dram[] = {
	0x4010000f, 0x321052b6, 0x00987654, 0x00000000,
	0x00000000, 0x04ff0028, 0x000002f7, 0x000001cc,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00010000, 0x00010000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000,
};

static chip_select_t chip_selection = {
	.cmd_code = CHIP_SELECTION,
	.chip_type = 3
};

static system_setup_info_t system_setup_info = {
	.cmd_code = SYSTEM_SETUP_INFO,
	.preview_A_type = 0,
	.preview_B_type = 1,
	.voutA_osd_blend_enabled = 1,
	.voutB_osd_blend_enabled = 0,
	.pip_vin_enabled = 0,
	.raw_encode_enabled = 0,
	.adv_iso_disabled = 0,
	.sub_mode_sel.multiple_stream = 2,
	.sub_mode_sel.power_mode = 0,
	.lcd_3d = 0,
	.iv_360 = 0,
	.mode_flags = 0,
	.audio_clk_freq = 0,
	.idsp_freq = 0,
	.sensor_HB_pixel = 0,
	.sensor_VB_pixel = 0,
	.hdr_preblend_from_vin = 0,
	.hdr_num_exposures_minus_1 = 0
};

static set_vin_global_config_t set_vin_global_clk = {
	.cmd_code = SET_VIN_GLOBAL_CLK,
	.main_or_pip = 0,
	.global_cfg_reg_word0 = 9
};

static set_vin_config_t set_vin_config = {
	.cmd_code = SET_VIN_CONFIG,
	.vin_width = 1280,
	.vin_height = 720,
	.vin_config_dram_addr = (u32)set_vin_config_dram,
	.config_data_size = 0x00000072,
	.sensor_resolution = 0,
	.sensor_bayer_pattern = 0,
	.vin_set_dly = 0,
	.vin_enhance_mode = 0,
	.vin_panasonic_mode = 0
};

static set_vin_master_t set_vin_master_clk = {
	.cmd_code = SET_VIN_MASTER_CLK,
	.master_sync_reg_word0 = 0,
	.master_sync_reg_word1 = 0,
	.master_sync_reg_word2 = 0,
	.master_sync_reg_word3 = 0,
	.master_sync_reg_word4 = 0,
	.master_sync_reg_word5 = 0,
	.master_sync_reg_word6 = 0,
	.master_sync_reg_word7 = 0
};

static sensor_input_setup_t sensor_input_setup = {
	.cmd_code = SENSOR_INPUT_SETUP,
	.sensor_id = 17,
	.field_format = 1,
	.sensor_resolution = 10,
	.sensor_pattern = 1,
	.first_line_field_0 = 0,
	.first_line_field_1 = 0,
	.first_line_field_2 = 0,
	.first_line_field_3 = 0,
	.first_line_field_4 = 0,
	.first_line_field_5 = 0,
	.first_line_field_6 = 0,
	.first_line_field_7 = 0
};

static video_preproc_t video_preprocessing = {
	.cmd_code = VIDEO_PREPROCESSING,
	.input_format = 0,
	.sensor_id = 17,
	.keep_states = 0,
	.vin_frame_rate = 30,
	.vidcap_w = 1280,
	.vidcap_h = 720,
	.main_w = 1280,
	.main_h = 720,
	.encode_w = 1280,
	.encode_h = 720,
	.encode_x = 0,
	.encode_y = 0,
	.preview_w_A = 0,
	.preview_h_A = 0,
	.input_center_x = 0,
	.input_center_y = 0,
	.zoom_factor_x = 0,
	.zoom_factor_y = 0,
	.aaa_data_fifo_start = 0,
	.sensor_readout_mode = 0,
	.noise_filter_strength = 0,
	.bit_resolution = 10,
	.bayer_pattern = 1,
	.preview_format_A = 6,
	.preview_format_B = 1,
	.no_pipelineflush = 0,
	.preview_frame_rate_A = 0,
	.preview_w_B = 720,
	.preview_h_B = 480,
	.preview_frame_rate_B = 0,
	.preview_A_en = 0,
	.preview_B_en = 1,
	.vin_frame_rate_ext = 0,
	.vdsp_int_factor = 0,
	.main_out_frame_rate = 1,
	.main_out_frame_rate_ext = 0,
	.vid_skip = 8,
	.EIS_enable = 0,
	.DIS_enable = 0,
	.Vert_WARP_enable = 0,
	.force_stitching = 0,
	.no_vin_reset_exiting = 0,
	.cmdReadDly = 0,
	.preview_src_w_A = 0,
	.preview_src_h_A = 0,
	.preview_src_x_offset_A = 0,
	.preview_src_y_offset_A = 0,
	.preview_src_w_B = 0,
	.preview_src_h_B = 0,
	.preview_src_x_offset_B = 0,
	.preview_src_y_offset_B = 0,
	.still_iso_config_daddr = 0x00000000
};

static set_warp_control_t set_warp_control = {
	.cmd_code = SET_WARP_CONTROL,
	.zoom_x = 0,
	.zoom_y = 0,
	.x_center_offset = 0,
	.y_center_offset = 0,
	.actual_left_top_x = 0,
	.actual_left_top_y = 0,
	.actual_right_bot_x = 125829120,
	.actual_right_bot_y = 99090432,
	.dummy_window_x_left = 0,
	.dummy_window_y_top = 0,
	.dummy_window_width = 1280,
	.dummy_window_height = 720,
	.cfa_output_width = 1280,
	.cfa_output_height = 720,
	.warp_control = 0,
	.warp_horizontal_table_address = 0x00000000,
	.warp_vertical_table_address = 0x00000000,
	.grid_array_width = 0,
	.grid_array_height = 0,
	.horz_grid_spacing_exponent = 0,
	.vert_grid_spacing_exponent = 0,
	.vert_warp_enable = 0,
	.vert_warp_grid_array_width = 0,
	.vert_warp_grid_array_height = 0,
	.vert_warp_horz_grid_spacing_exponent = 0,
	.vert_warp_vert_grid_spacing_exponent = 0
};

static ipcam_video_system_setup_t ipcam_video_system_setup = {
	.cmd_code = IPCAM_VIDEO_SYSTEM_SETUP,
	.main_max_width = 1280,
	.main_max_height = 720,
	.preview_A_max_width = 0,
	.preview_A_max_height = 0,
	.preview_B_max_width = 720,
	.preview_B_max_height = 480,
	.preview_C_max_width = 720,
	.preview_C_max_height = 576,
	.stream_0_max_GOP_M = 1,
	.stream_1_max_GOP_M = 1,
	.stream_2_max_GOP_M = 0,
	.stream_3_max_GOP_M = 0,
	.stream_0_max_width = 1920,
	.stream_0_max_height = 1088,
	.stream_1_max_width = 1280,
	.stream_1_max_height = 720,
	.stream_2_max_width = 0,
	.stream_2_max_height = 0,
	.stream_3_max_width = 0,
	.stream_3_max_height = 0,
	.MCTF_possible = 1,
	.max_num_streams = 4,
	.max_num_cap_sources = 2,
	.use_1Gb_DRAM_config = 0,
	.raw_compression_disabled = 0,
	.vwarp_blk_h = 28,
	.raw_max_width = 1280,
	.raw_max_height = 720,
	.warped_main_max_width = 0,
	.warped_main_max_height = 0,
	.v_warped_main_max_width = 0,
	.v_warped_main_max_height = 0,
	.max_warp_region_input_width = 0,
	.max_warp_region_output_width = 0
};

static ipcam_capture_preview_size_setup_t ipcam_video_capture_preview_size_setup = {
	.cmd_code = IPCAM_VIDEO_CAPTURE_PREVIEW_SIZE_SETUP,
	.preview_id = 3,
	.output_scan_format = 0,
	.deinterlace_mode = 0,
	.disabled = 0,
	.cap_width = 720,
	.cap_height = 480,
	.input_win_offset_x = 0,
	.input_win_offset_y = 0,
	.input_win_width = 1280,
	.input_win_height = 720
};

static ipcam_video_encode_size_setup_t ipcam_video_encode_size_setup = {
	.cmd_code = IPCAM_VIDEO_ENCODE_SIZE_SETUP,
	.capture_source = 0,
	.enc_x = 0,
	.enc_y = 0,
	.enc_width = 1280,
	.enc_height = 720
};

static h264_encode_setup_t h264_encoding_setup = {
	.cmd_code = H264_ENCODING_SETUP,
	.mode = 1,
	.M = 1,
	.N = 30,
	.quality = 0,
	.average_bitrate = 2000000,
	.vbr_cntl = 0,
	.vbr_setting = 4,
	.allow_I_adv = 0,
	.cpb_buf_idc = 0,
	.en_panic_rc = 0,
	.cpb_cmp_idc = 0,
	.fast_rc_idc = 0,
	.target_storage_space = 0,
	.bits_fifo_base = 0x00000000,
	.bits_fifo_limit = 0x00000000,
	.info_fifo_base = 0x00000000,
	.info_fifo_limit = 0x00000000,
	.audio_in_freq = 2,
	.vin_frame_rate = 0,
	.encoder_frame_rate = 0,
	.frame_sync = 0,
	.initial_fade_in_gain = 0,
	.final_fade_out_gain = 0,
	.idr_interval = 1,
	.cpb_user_size = 0,
	.numRef_P = 0,
	.numRef_B = 0,
	.vin_frame_rate_ext = 0,
	.encoder_frame_rate_ext = 0,
	.vbr_cbp_rate = 0,
	.frame_rate_division_factor = 1,
	.force_intlc_tb_iframe = 0,
	.session_id = 0,
	.frame_rate_multiplication_factor = 1,
	.hflip = 0,
	.vflip = 0,
	.rotate = 0,
	.chroma_format = 0,
	.custom_encoder_frame_rate = 30000
};

static h264_encode_t h264_encode = {
	.cmd_code = H264_ENCODE,
	.bits_fifo_next = 0x00000000,
	.info_fifo_next = 0x00000000,
	.start_encode_frame_no = -1,
	.encode_duration = -1,
	.is_flush = 1,
	.enable_slow_shutter = 0,
	.res_rate_min = 40,
	.alpha = 0,
	.beta = 0,
	.en_loop_filter = 2,
	.max_upsampling_rate = 1,
	.slow_shutter_upsampling_rate = 0,
	.frame_cropping_flag = 1,
	.profile = 0,
	.frame_crop_left_offset = 0,
	.frame_crop_right_offset = 0,
	.frame_crop_top_offset = 0,
	.frame_crop_bottom_offset = 0,
	.num_ref_frame = 0,
	.log2_max_frame_num_minus4 = 0,
	.log2_max_pic_order_cnt_lsb_minus4 = 0,
	.sony_avc = 0,
	.height_mjpeg_h264_simultaneous = 0,
	.width_mjpeg_h264_simultaneous = 0,
	.vui_enable = 1,
	.aspect_ratio_info_present_flag = 1,
	.overscan_info_present_flag = 0,
	.overscan_appropriate_flag = 0,
	.video_signal_type_present_flag = 1,
	.video_full_range_flag = 0,
	.colour_description_present_flag = 1,
	.chroma_loc_info_present_flag = 0,
	.timing_info_present_flag = 1,
	.fixed_frame_rate_flag = 1,
	.nal_hrd_parameters_present_flag = 1,
	.vcl_hrd_parameters_present_flag = 1,
	.low_delay_hrd_flag = 0,
	.pic_struct_present_flag = 1,
	.bitstream_restriction_flag = 0,
	.motion_vectors_over_pic_boundaries_flag = 0,
	.SAR_width = 0,
	.SAR_height = 0,
	.video_format = 5,
	.colour_primaries = 1,
	.transfer_characteristics = 1,
	.matrix_coefficients = 1,
	.chroma_sample_loc_type_top_field = 0,
	.chroma_sample_loc_type_bottom_field = 0,
	.aspect_ratio_idc = 1,
	.max_bytes_per_pic_denom = 0,
	.max_bits_per_mb_denom = 0,
	.log2_max_mv_length_horizontal = 0,
	.log2_max_mv_length_vertical = 0,
	.num_reorder_frames = 0,
	.max_dec_frame_buffering = 0,
	.I_IDR_sp_rc_handle_mask = 0,
	.IDR_QP_adj_str = 0,
	.IDR_class_adj_limit = 0,
	.I_QP_adj_str = 0,
	.I_class_adj_limit = 0,
	.firstGOPstartB = 0,
	.au_type = 1,
	.SAR_height = 0,
	.SAR_height = 0
};

static ipcam_real_time_encode_param_setup_t ipcam_real_time_encode_param_setup = {
	.cmd_code = IPCAM_REAL_TIME_ENCODE_PARAM_SETUP,
	.enable_flags = 0x00004025,
	.cbr_modify = 2000000,
	.custom_encoder_frame_rate = 0,
	.frame_rate_division_factor = 0,
	.qp_min_on_I = 1,
	.qp_max_on_I = 51,
	.qp_min_on_P = 1,
	.qp_max_on_P = 51,
	.qp_min_on_B = 1,
	.qp_max_on_B = 51,
	.aqp = 2,
	.frame_rate_multiplication_factor = 0,
	.i_qp_reduce = 6,
	.skip_flags = 0x00000000,
	.M = 0,
	.N = 0,
	.p_qp_reduce = 3,
	.intra_refresh_num_mb_row = 0,
	.preview_A_frame_rate_divison_factor = 0,
	.idr_interval = 0,
	.custom_vin_frame_rate = 0,
	.roi_daddr = 0x00000000,
	.roi_delta[0][0] = 0,
	.roi_delta[0][1] = 0,
	.roi_delta[0][2] = 0,
	.roi_delta[0][3] = 0,
	.roi_delta[1][0] = 0,
	.roi_delta[1][1] = 0,
	.roi_delta[1][2] = 0,
	.roi_delta[1][3] = 0,
	.roi_delta[2][0] = 0,
	.roi_delta[2][1] = 0,
	.roi_delta[2][2] = 0,
	.roi_delta[2][3] = 0,
	.panic_div = 0,
	.is_monochrome = 0,
	.scene_change_detect_on = 0,
	.pic_size_control = 0,
	.quant_matrix_addr = 0x00000000,
	.P_IntraBiasAdd = 0,
	.B_IntraBiasAdd = 0,
	.zmv_threshold = 0
};

static int ov9710_init(void)
{
	int i;

	/* set pll sent to sensor */
	rct_set_so_pll();

	/* ov9710 can work with 400K I2C, so we use it to reduce timing */
	idc_bld_init(IDC_MASTER1, 400000);

	for (i = 0; i < ARRAY_SIZE(ov9710_720p30_regs) ; i++)
		idc_bld_write_8_8(IDC_MASTER1, 0x60,
				ov9710_720p30_regs[i].addr,
				ov9710_720p30_regs[i].data);

	/* store vin video format in amboot, restore it after enter Linux IAV */
	memcpy((void *)(DSP_VIN_VIDEO_FORMAT_STORE_START),
		&vin_video_boot_format, sizeof(vin_video_boot_format));

	putstr("ov9710_init\r\n");
	return 0;
}

#if defined(AMBOOT_IAV_DEBUG_DSP_CMD)
static dsp_debug_level_setup_t dsp_debug_level_setup =  {
	.cmd_code = DSP_DEBUG_LEVEL_SETUP,
	.module = 0,
	.debug_level = 3,
	.coding_thread_printf_disable_mask = 0
};
#endif

static inline void prepare_vin_vout_dsp_cmd(void)
{
#if defined(CONFIG_S2LMKIWI_IAV_CVBS)
	system_setup_info.preview_A_type = 0;
	system_setup_info.preview_B_type = 1;

	video_preprocessing.preview_w_A = 0;
	video_preprocessing.preview_h_A = 0;
	video_preprocessing.preview_format_A = 6;
	video_preprocessing.preview_format_B = 1;
	video_preprocessing.no_pipelineflush = 0;
	video_preprocessing.preview_frame_rate_A = 0;
	video_preprocessing.preview_w_B = 720;
	video_preprocessing.preview_h_B = 480;
	video_preprocessing.preview_frame_rate_B = 0;
	video_preprocessing.preview_A_en = 0;
	video_preprocessing.preview_B_en = 1;

	ipcam_video_system_setup.preview_A_max_width = 0;
	ipcam_video_system_setup.preview_A_max_height = 0;
	ipcam_video_system_setup.preview_B_max_width = 720;
	ipcam_video_system_setup.preview_B_max_height = 480;
#elif defined(CONFIG_S2LMKIWI_IAV_TD043)
	system_setup_info.preview_A_type = 2;
	system_setup_info.preview_B_type = 0;

	video_preprocessing.preview_w_A = 800;
	video_preprocessing.preview_h_A = 480;
	video_preprocessing.preview_format_A = 0;
	video_preprocessing.preview_format_B = 6;
	video_preprocessing.no_pipelineflush = 0;
	video_preprocessing.preview_frame_rate_A = 60;
	video_preprocessing.preview_w_B = 0;
	video_preprocessing.preview_h_B = 0;
	video_preprocessing.preview_frame_rate_B = 0;
	video_preprocessing.preview_A_en = 1;
	video_preprocessing.preview_B_en = 0;

	ipcam_video_system_setup.preview_A_max_width = 800;
	ipcam_video_system_setup.preview_A_max_height = 480;
	ipcam_video_system_setup.preview_B_max_width = 0;
	ipcam_video_system_setup.preview_B_max_height = 0;
#endif
}

static int iav_boot_preview(void)
{
	prepare_vin_vout_dsp_cmd();

	/* preview setup */
	add_dsp_cmd(&chip_selection, sizeof(chip_selection));
	add_dsp_cmd(&system_setup_info, sizeof(system_setup_info));
	add_dsp_cmd(&set_vin_global_clk, sizeof(set_vin_global_clk));
	add_dsp_cmd(&set_vin_config, sizeof(set_vin_config));
	add_dsp_cmd(&set_vin_master_clk, sizeof(set_vin_master_clk));
	add_dsp_cmd(&sensor_input_setup, sizeof(sensor_input_setup));
	add_dsp_cmd(&video_preprocessing, sizeof(video_preprocessing));
	add_dsp_cmd(&set_warp_control, sizeof(set_warp_control));
	add_dsp_cmd(&ipcam_video_system_setup, sizeof(ipcam_video_system_setup));
	add_dsp_cmd(&ipcam_video_capture_preview_size_setup,
				sizeof(ipcam_video_capture_preview_size_setup));

	return 0;
}

static int iav_boot_encode(void)
{
	/* encode setup */
	add_dsp_cmd(&ipcam_video_encode_size_setup,
		sizeof(ipcam_video_encode_size_setup));
	add_dsp_cmd(&h264_encoding_setup, sizeof(h264_encoding_setup));
	add_dsp_cmd(&h264_encode, sizeof(h264_encode));
	add_dsp_cmd(&ipcam_real_time_encode_param_setup,
		sizeof(ipcam_real_time_encode_param_setup));

#if defined(AMBOOT_IAV_DEBUG_DSP_CMD)
	add_dsp_cmd(&dsp_debug_level_setup, sizeof(dsp_debug_level_setup));
#endif

	return 0;
}

#endif

