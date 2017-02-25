/**
 * boards/hawthorn/bsp/iav/sensor_ov4689.c
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


struct vin_reg_16_8 {
	u16 addr;
	u8 data;
};

#define BOOT_VIDEO_MODE  45 		// 45 = 2688x1512
#define BOOT_HDR_MODE 0			// 0 = LINEAR

static const struct vin_reg_16_8 ov4689_4lane_4m_regs[] = {
	//@@ 0 10 RES_2688x1520_default(90fps)
	{0x0103, 0x01}, {0x3638, 0x00}, {0x0300, 0x00}, {0x0302, 0x22},
	{0x0304, 0x03}, {0x030b, 0x00}, {0x030d, 0x1e}, {0x030e, 0x04},
	{0x030f, 0x01}, {0x0312, 0x01}, {0x031e, 0x00}, {0x3000, 0x20},
	{0x3002, 0x00}, {0x3018, 0x72}, {0x3020, 0x93}, {0x3021, 0x03},
	{0x3022, 0x01}, {0x3031, 0x0a}, {0x303f, 0x0c}, {0x3305, 0xf1},
	{0x3307, 0x04}, {0x3309, 0x29}, {0x3500, 0x00}, {0x3501, 0x60},
	{0x3502, 0x00}, {0x3503, 0x04}, {0x3504, 0x00}, {0x3505, 0x00},
	{0x3506, 0x00}, {0x3507, 0x00}, {0x3508, 0x00}, {0x3509, 0x80},
	{0x350a, 0x00}, {0x350b, 0x00}, {0x350c, 0x00}, {0x350d, 0x00},
	{0x350e, 0x00}, {0x350f, 0x80}, {0x3510, 0x00}, {0x3511, 0x00},
	{0x3512, 0x00}, {0x3513, 0x00}, {0x3514, 0x00}, {0x3515, 0x80},
	{0x3516, 0x00}, {0x3517, 0x00}, {0x3518, 0x00}, {0x3519, 0x00},
	{0x351a, 0x00}, {0x351b, 0x80}, {0x351c, 0x00}, {0x351d, 0x00},
	{0x351e, 0x00}, {0x351f, 0x00}, {0x3520, 0x00}, {0x3521, 0x80},
	{0x3522, 0x08}, {0x3524, 0x08}, {0x3526, 0x08}, {0x3528, 0x08},
	{0x352a, 0x08}, {0x3602, 0x00}, {0x3603, 0x40}, {0x3604, 0x02},
	{0x3605, 0x00}, {0x3606, 0x00}, {0x3607, 0x00}, {0x3609, 0x12},
	{0x360a, 0x40}, {0x360c, 0x08}, {0x360f, 0xe5}, {0x3608, 0x8f},
	{0x3611, 0x00}, {0x3613, 0xf7}, {0x3616, 0x58}, {0x3619, 0x99},
	{0x361b, 0x60}, {0x361c, 0x7a}, {0x361e, 0x79}, {0x361f, 0x02},
	{0x3632, 0x00}, {0x3633, 0x10}, {0x3634, 0x10}, {0x3635, 0x10},
	{0x3636, 0x15}, {0x3646, 0x86}, {0x364a, 0x0b}, {0x3700, 0x17},
	{0x3701, 0x22}, {0x3703, 0x10}, {0x370a, 0x37}, {0x3705, 0x00},
	{0x3706, 0x63}, {0x3709, 0x3c}, {0x370b, 0x01}, {0x370c, 0x30},
	{0x3710, 0x24}, {0x3711, 0x0c}, {0x3716, 0x00}, {0x3720, 0x28},
	{0x3729, 0x7b}, {0x372a, 0x84}, {0x372b, 0xbd}, {0x372c, 0xbc},
	{0x372e, 0x52}, {0x373c, 0x0e}, {0x373e, 0x33}, {0x3743, 0x10},
	{0x3744, 0x88}, {0x3745, 0xc0}, {0x374a, 0x43}, {0x374c, 0x00},
	{0x374e, 0x23}, {0x3751, 0x7b}, {0x3752, 0x84}, {0x3753, 0xbd},
	{0x3754, 0xbc}, {0x3756, 0x52}, {0x375c, 0x00}, {0x3760, 0x00},
	{0x3761, 0x00}, {0x3762, 0x00}, {0x3763, 0x00}, {0x3764, 0x00},
	{0x3767, 0x04}, {0x3768, 0x04}, {0x3769, 0x08}, {0x376a, 0x08},
	{0x376b, 0x20}, {0x376c, 0x00}, {0x376d, 0x00}, {0x376e, 0x00},
	{0x3773, 0x00}, {0x3774, 0x51}, {0x3776, 0xbd}, {0x3777, 0xbd},
	{0x3781, 0x18}, {0x3783, 0x25}, {0x3798, 0x1b}, {0x3800, 0x00},
	{0x3801, 0x08}, {0x3802, 0x00}, {0x3803, 0x04}, {0x3804, 0x0a},
	{0x3805, 0x97}, {0x3806, 0x05}, {0x3807, 0xfb}, {0x3808, 0x0a},
	{0x3809, 0x80}, {0x380a, 0x05}, {0x380b, 0xf0}, {0x380c, 0x03},
	{0x380d, 0x5c}, {0x380e, 0x0c}, {0x380f, 0x27}, {0x3810, 0x00},
	{0x3811, 0x08}, {0x3812, 0x00}, {0x3813, 0x04}, {0x3814, 0x01},
	{0x3815, 0x01}, {0x3819, 0x01}, {0x3820, 0x00}, {0x3821, 0x06},
	{0x3829, 0x00}, {0x382a, 0x01}, {0x382b, 0x01}, {0x382d, 0x7f},
	{0x3830, 0x04}, {0x3836, 0x01}, {0x3837, 0x00}, {0x3841, 0x02},
	{0x3846, 0x08}, {0x3847, 0x07}, {0x3d85, 0x36}, {0x3d8c, 0x71},
	{0x3d8d, 0xcb}, {0x3f0a, 0x00}, {0x4000, 0x71}, {0x4001, 0x40},
	{0x4002, 0x04}, {0x4003, 0x14}, {0x400e, 0x00}, {0x4011, 0x00},
	{0x401a, 0x00}, {0x401b, 0x00}, {0x401c, 0x00}, {0x401d, 0x00},
	{0x401f, 0x00}, {0x4020, 0x00}, {0x4021, 0x10}, {0x4022, 0x07},
	{0x4023, 0xcf}, {0x4024, 0x09}, {0x4025, 0x60}, {0x4026, 0x09},
	{0x4027, 0x6f}, {0x4028, 0x00}, {0x4029, 0x02}, {0x402a, 0x06},
	{0x402b, 0x04}, {0x402c, 0x02}, {0x402d, 0x02}, {0x402e, 0x0e},
	{0x402f, 0x04}, {0x4302, 0xff}, {0x4303, 0xff}, {0x4304, 0x00},
	{0x4305, 0x00}, {0x4306, 0x00}, {0x4308, 0x02}, {0x4500, 0x6c},
	{0x4501, 0xc4}, {0x4502, 0x40}, {0x4503, 0x01}, {0x4601, 0x04},
	{0x4800, 0x24}, {0x4813, 0x08}, {0x481f, 0x40}, {0x4829, 0x78},
	{0x4837, 0x14}, {0x4b00, 0x2a}, {0x4b0d, 0x00}, {0x4d00, 0x04},
	{0x4d01, 0x42}, {0x4d02, 0xd1}, {0x4d03, 0x93}, {0x4d04, 0xf5},
	{0x4d05, 0xc1}, {0x5000, 0xf3}, {0x5001, 0x11}, {0x5004, 0x00},
	{0x500a, 0x00}, {0x500b, 0x00}, {0x5032, 0x00}, {0x5040, 0x00},
	{0x5050, 0x0c}, {0x5500, 0x00}, {0x5501, 0x10}, {0x5502, 0x01},
	{0x5503, 0x0f}, {0x8000, 0x00}, {0x8001, 0x00}, {0x8002, 0x00},
	{0x8003, 0x00}, {0x8004, 0x00}, {0x8005, 0x00}, {0x8006, 0x00},
	{0x8007, 0x00}, {0x8008, 0x00}, {0x3638, 0x00}, {0x0100, 0x01},

	//@@ 10 12 RES_2688x1520_30fps
	{0x0100, 0x00}, {0x380c, 0x05}, {0x380d, 0x05}, {0x0100, 0x01},
};

static u32 set_vin_config_dram[] = {
	0xC00003CE, 0x32100000, 0x00987654, 0x00000000,
	0x00000000, 0x0A7F0004, 0x000005EB, 0x000001CC,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x03F30000, 0x00000010, 0x00000000,
	0x00000000, 0x00010000, 0x00010000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000,
};

static struct vin_video_format vin_video_boot_format = {
	.video_mode = BOOT_VIDEO_MODE,
	.device_mode = 0,
	.pll_idx = 0,
	.width = 0,
	.height = 0,

	.def_start_x = 0,
	.def_start_y = 0,
	.def_width = 0,
	.def_height = 0,
	.format = 0,
	.type = 0,
	.bits = 0,
	.ratio = 0,
	.bayer_pattern = 0,
	.hdr_mode = BOOT_HDR_MODE,
	.readout_mode = 0,
	.sync_start = 0,

	.max_fps = 0,
	.default_fps = 0,
	.default_agc = 0,
	.default_shutter_time = 0,
	.default_bayer_pattern = 0,

	/* hdr mode related */
	.act_start_x = 0,
	.act_start_y = 0,
	.act_width = 0,
	.act_height = 0,
	.max_act_width = 0,
	.max_act_height = 0,
	.hdr_long_offset = 0,
	.hdr_short1_offset = 0,
	.hdr_short2_offset = 0,
	.hdr_short3_offset = 0,
	.dual_gain_mode = 0
};

static chip_select_t chip_selection = {
	.cmd_code = CHIP_SELECTION,
	.chip_type = 3
};

static system_setup_info_t system_setup_info = {
	.cmd_code = SYSTEM_SETUP_INFO,
	/* Final value define in prepare_vin_vout_dsp_cmd() */
	.preview_A_type = 0,
	.preview_B_type = 1,
	.voutA_osd_blend_enabled = 1,
	.voutB_osd_blend_enabled = 0,
	/* End */
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
	.core_freq = 216000000,
	.sensor_HB_pixel = 0,
	.sensor_VB_pixel = 0,
	.hdr_preblend_from_vin = 0,
	.hdr_num_exposures_minus_1 = 0,
	.vout_swap = 0
};

static set_vin_global_config_t set_vin_global_clk = {
	.cmd_code = SET_VIN_GLOBAL_CLK,
	.main_or_pip = 0,
	.global_cfg_reg_word0 = 10
};

static set_vin_config_t set_vin_config = {
	.cmd_code = SET_VIN_CONFIG,
	.vin_width = 2688,
	.vin_height = 1512,
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
	.sensor_id = 127,
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
	.first_line_field_7 = 0,
	.sensor_readout_mode = 0,
};

static video_preproc_t video_preprocessing = {
	.cmd_code = VIDEO_PREPROCESSING,
	.input_format = 0,
	.sensor_id = 127,
	.keep_states = 0,
	.vin_frame_rate = 1,
	.vidcap_w = 2688,
	.vidcap_h = 1512,
	.main_w = 1920,
	.main_h = 1080,
	.encode_w = 1920,
	.encode_h = 1080,
	.encode_x = 0,
	.encode_y = 0,
	/* Final value define in prepare_vin_vout_dsp_cmd() */
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
	/* END */
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
	.vid_skip = 2,
	.EIS_delay_count = 0,
	.Vert_WARP_enable = 0,
	.force_stitching = 1,
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
	.dummy_window_width = 2688,
	.dummy_window_height = 1512,
	.cfa_output_width = 1920,
	.cfa_output_height = 1512,
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
	.vert_warp_vert_grid_spacing_exponent = 0,
	.ME1_vwarp_grid_array_width = 0,
	.ME1_vwarp_grid_array_height = 0,
	.ME1_vwarp_horz_grid_spacing_exponent = 0,
	.ME1_vwarp_vert_grid_spacing_exponent = 0,
	.ME1_vwarp_table_address = 0x00000000
};

static ipcam_video_system_setup_t ipcam_video_system_setup = {
	.cmd_code = IPCAM_VIDEO_SYSTEM_SETUP,
	.main_max_width = 1920,
	.main_max_height = 1088,
	/* Final value define in prepare_vin_vout_dsp_cmd() */
	.preview_A_max_width = 0,
	.preview_A_max_height = 0,
	.preview_B_max_width = 720,
	.preview_B_max_height = 480,
	/* END */
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
	.extra_buf_main = 0,
	.extra_buf_preview_a = 0,
	.extra_buf_preview_b = 0,
	.extra_buf_preview_c = 0,
	.raw_max_width = 2688,
	.raw_max_height = 1512,
	.warped_main_max_width = 0,
	.warped_main_max_height = 0,
	.v_warped_main_max_width = 0,
	.v_warped_main_max_height = 0,
	.max_warp_region_input_width = 0,
	.max_warp_region_input_height = 0,
	.max_warp_region_output_width = 0,
	.max_padding_width = 0,
	.enc_rotation_possible = 0,
	.enc_dummy_latency = 0,
	.stream_0_LT_enable = 0,
	.stream_1_LT_enable = 0,
	.stream_2_LT_enable = 0,
	.stream_3_LT_enable = 0
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
	.input_win_width = 1920,
	.input_win_height = 1080
};

static ipcam_video_encode_size_setup_t ipcam_video_encode_size_setup = {
	.cmd_code = IPCAM_VIDEO_ENCODE_SIZE_SETUP,
	.capture_source = 0,
	.enc_x = 0,
	.enc_y = 0,
	.enc_width = 1920,
	.enc_height = 1088
};

static h264_encode_setup_t h264_encoding_setup = {
	.cmd_code = H264_ENCODING_SETUP,
	.mode = 1,
	.M = 1,
	.N = 30,
	.quality = 0,
	.average_bitrate = 4000000,
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
	.N_msb = 0,
	.fast_seek_interval = 0,
	.vbr_cbp_rate = 0,
	.frame_rate_division_factor = 1,
	.force_intlc_tb_iframe = 0,
	.session_id = 0,
	.frame_rate_multiplication_factor = 1,
	.hflip = 0,
	.vflip = 0,
	.rotate = 0,
	.chroma_format = 0,
	.custom_encoder_frame_rate = 1073771824
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
	.frame_crop_bottom_offset = 4,
	.num_ref_frame = 0,
	.log2_max_frame_num_minus4 = 0,
	.log2_max_pic_order_cnt_lsb_minus4 = 0,
	.sony_avc = 0,
	.gaps_in_frame_num_value_allowed_flag = 0,
	.height_mjpeg_h264_simultaneous = 0,
	.width_mjpeg_h264_simultaneous = 0,
	.vui_enable = 1,
	.aspect_ratio_info_present_flag = 1,
	.overscan_info_present_flag = 0,
	.overscan_appropriate_flag = 0,
	.video_signal_type_present_flag = 1,
	.video_full_range_flag = 1,
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
	.au_type = 1
};

static ipcam_real_time_encode_param_setup_t ipcam_real_time_encode_param_setup = {
	.cmd_code = IPCAM_REAL_TIME_ENCODE_PARAM_SETUP,
	.enable_flags = 0x0000c835,
	.cbr_modify = 4000000,
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
	.custom_vin_frame_rate = 1073771824,
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
	.flat_area_improvement_on = 0,
	.drop_frame = 0,
	.pic_size_control = 0,
	.quant_matrix_addr = 0x00000000,
	.P_IntraBiasAdd = 1,
	.B_IntraBiasAdd = 1,
	.zmv_threshold = 0,
	.N_msb = 0,
	.idsp_frame_rate_M = 30,
	.idsp_frame_rate_N = 30,
	.roi_daddr_p = 0,
	.roi_daddr_b = 0,
	.user1_intra_bias = 0,
	.user1_direct_bias = 0,
	.user2_intra_bias = 0,
	.user2_direct_bias = 0,
	.user3_intra_bias = 0,
	.user3_direct_bias = 0,
};

#if defined(AMBOOT_IAV_DEBUG_DSP_CMD)
static dsp_debug_level_setup_t dsp_debug_level_setup =  {
	.cmd_code = DSP_DEBUG_LEVEL_SETUP,
	.module = 0,
	.debug_level = 3,
	.coding_thread_printf_disable_mask = 0
};
#endif

static int ov4689_init(void)
{
	int i;

	/* set pll sent to sensor */
	rct_set_so_pll();

	/* ov4689 can work with 800K I2C, so we use it to reduce timing */
	idc_bld_init(IDC_MASTER1, 800000);

	for (i = 0; i < ARRAY_SIZE(ov4689_4lane_4m_regs); i++) {
		idc_bld_write_16_8(IDC_MASTER1, 0x6c,
				ov4689_4lane_4m_regs[i].addr,
				ov4689_4lane_4m_regs[i].data);
	}

	/* store vin video format in amboot, restore it after enter Linux IAV */
	memcpy((void *)(DSP_VIN_VIDEO_FORMAT_STORE_START),
		&vin_video_boot_format, sizeof(vin_video_boot_format));

	/* store dsp vin config in amboot, restore it after enter Linux IAV*/
	memcpy((void *)(DSP_VIN_CONFIG_STORE_START),
		set_vin_config_dram, sizeof(set_vin_config_dram));

	putstr_debug("ov4689_init");
	return 0;
}

static inline void prepare_vin_vout_dsp_cmd(void)
{
#if defined(CONFIG_S2LMIRONMAN_IAV_CVBS)
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

	system_setup_info.voutA_osd_blend_enabled = 1;
	system_setup_info.voutB_osd_blend_enabled = 0;
#elif defined(CONFIG_S2LMIRONMAN_IAV_TD043)
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

	system_setup_info.voutA_osd_blend_enabled = 0;
	system_setup_info.voutB_osd_blend_enabled = 1;
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

	putstr_debug("iav_boot_preview");
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

	putstr_debug("iav_boot_encode");
	return 0;
}

