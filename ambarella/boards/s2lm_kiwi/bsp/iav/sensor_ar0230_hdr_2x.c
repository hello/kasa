/**
 * boards/hawthorn/bsp/iav/sensor_ar0230.c
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


struct vin_reg_16_16 {
	u16 addr;
	u16 data;
};

#define BOOT_VIDEO_MODE  0x00020013 	// 0x00020013 = 1080p59.94
#define BOOT_HDR_MODE 1 				// 1 = 2X_HDR_MODE

static const struct vin_reg_16_16 ar0230_2x_hdr_regs[] = {
	/* 2x HDR 1080p30fps */
	{0x301A, 0x0001},
	{0x301A, 0x10D8},
	{0x2436, 0x000E},
	{0x320C, 0x0180},
	{0x320E, 0x0300},
	{0x3210, 0x0500},
	{0x3204, 0x0B6D},
	{0x30FE, 0x0080},
	{0x3ED8, 0x7B99},
	{0x3EDC, 0x9BA8},
	{0x3EDA, 0x9B9B},
	{0x3092, 0x006F},
	{0x3EEC, 0x1C04},
	{0x30BA, 0x779C},
	{0x3EF6, 0xA70F},
	{0x3044, 0x0410},
	{0x3ED0, 0xFF44},
	{0x3ED4, 0x031F},
	{0x30FE, 0x0080},
	{0x3EE2, 0x8866},
	{0x3EE4, 0x6623},
	{0x3EE6, 0x2263},
	{0x30E0, 0x4283},
	{0x30F0, 0x1283},
	{0x30B0, 0x0000}, /* DIGITAL_TEST, 0x0000 */
	{0x31AC, 0x100C}, /* DATA_FORMAT_BITS */
	{0x301A, 0x0059},
	{0x3088, 0x8000},
	{0x3086, 0x4558},
	{0x3086, 0x729B},
	{0x3086, 0x4A31},
	{0x3086, 0x4342},
	{0x3086, 0x8E03},
	{0x3086, 0x2A14},
	{0x3086, 0x4578},
	{0x3086, 0x7B3D},
	{0x3086, 0xFF3D},
	{0x3086, 0xFF3D},
	{0x3086, 0xEA2A},
	{0x3086, 0x043D},
	{0x3086, 0x102A},
	{0x3086, 0x052A},
	{0x3086, 0x1535},
	{0x3086, 0x2A05},
	{0x3086, 0x3D10},
	{0x3086, 0x4558},
	{0x3086, 0x2A04},
	{0x3086, 0x2A14},
	{0x3086, 0x3DFF},
	{0x3086, 0x3DFF},
	{0x3086, 0x3DEA},
	{0x3086, 0x2A04},
	{0x3086, 0x622A},
	{0x3086, 0x288E},
	{0x3086, 0x0036},
	{0x3086, 0x2A08},
	{0x3086, 0x3D64},
	{0x3086, 0x7A3D},
	{0x3086, 0x0444},
	{0x3086, 0x2C4B},
	{0x3086, 0x8F00},
	{0x3086, 0x430C},
	{0x3086, 0x2D63},
	{0x3086, 0x4316},
	{0x3086, 0x8E03},
	{0x3086, 0x2AFC},
	{0x3086, 0x5C1D},
	{0x3086, 0x5754},
	{0x3086, 0x495F},
	{0x3086, 0x5305},
	{0x3086, 0x5307},
	{0x3086, 0x4D2B},
	{0x3086, 0xF810},
	{0x3086, 0x164C},
	{0x3086, 0x0855},
	{0x3086, 0x562B},
	{0x3086, 0xB82B},
	{0x3086, 0x984E},
	{0x3086, 0x1129},
	{0x3086, 0x0429},
	{0x3086, 0x8429},
	{0x3086, 0x9460},
	{0x3086, 0x5C19},
	{0x3086, 0x5C1B},
	{0x3086, 0x4548},
	{0x3086, 0x4508},
	{0x3086, 0x4588},
	{0x3086, 0x29B6},
	{0x3086, 0x8E01},
	{0x3086, 0x2AF8},
	{0x3086, 0x3E02},
	{0x3086, 0x2AFA},
	{0x3086, 0x3F09},
	{0x3086, 0x5C1B},
	{0x3086, 0x29B2},
	{0x3086, 0x3F0C},
	{0x3086, 0x3E02},
	{0x3086, 0x3E13},
	{0x3086, 0x5C13},
	{0x3086, 0x3F11},
	{0x3086, 0x3E0B},
	{0x3086, 0x5F2B},
	{0x3086, 0x902A},
	{0x3086, 0xF22B},
	{0x3086, 0x803E},
	{0x3086, 0x043F},
	{0x3086, 0x0660},
	{0x3086, 0x29A2},
	{0x3086, 0x29A3},
	{0x3086, 0x5F4D},
	{0x3086, 0x192A},
	{0x3086, 0xFA29},
	{0x3086, 0x8345},
	{0x3086, 0xA83E},
	{0x3086, 0x072A},
	{0x3086, 0xFB3E},
	{0x3086, 0x2945},
	{0x3086, 0x8821},
	{0x3086, 0x3E08},
	{0x3086, 0x2AFA},
	{0x3086, 0x5D29},
	{0x3086, 0x9288},
	{0x3086, 0x102B},
	{0x3086, 0x048B},
	{0x3086, 0x1685},
	{0x3086, 0x8D48},
	{0x3086, 0x4D4E},
	{0x3086, 0x2B80},
	{0x3086, 0x4C0B},
	{0x3086, 0x603F},
	{0x3086, 0x282A},
	{0x3086, 0xF23F},
	{0x3086, 0x0F29},
	{0x3086, 0x8229},
	{0x3086, 0x8329},
	{0x3086, 0x435C},
	{0x3086, 0x155F},
	{0x3086, 0x4D19},
	{0x3086, 0x2AFA},
	{0x3086, 0x4558},
	{0x3086, 0x8E00},
	{0x3086, 0x2A98},
	{0x3086, 0x3F06},
	{0x3086, 0x1244},
	{0x3086, 0x4A04},
	{0x3086, 0x4316},
	{0x3086, 0x0543},
	{0x3086, 0x1658},
	{0x3086, 0x4316},
	{0x3086, 0x5A43},
	{0x3086, 0x1606},
	{0x3086, 0x4316},
	{0x3086, 0x0743},
	{0x3086, 0x168E},
	{0x3086, 0x032A},
	{0x3086, 0x9C45},
	{0x3086, 0x787B},
	{0x3086, 0x3F07},
	{0x3086, 0x2A9D},
	{0x3086, 0x3E2E},
	{0x3086, 0x4558},
	{0x3086, 0x253E},
	{0x3086, 0x068E},
	{0x3086, 0x012A},
	{0x3086, 0x988E},
	{0x3086, 0x0012},
	{0x3086, 0x444B},
	{0x3086, 0x0343},
	{0x3086, 0x2D46},
	{0x3086, 0x4316},
	{0x3086, 0xA343},
	{0x3086, 0x165D},
	{0x3086, 0x0D29},
	{0x3086, 0x4488},
	{0x3086, 0x102B},
	{0x3086, 0x0453},
	{0x3086, 0x0D8B},
	{0x3086, 0x1685},
	{0x3086, 0x448E},
	{0x3086, 0x032A},
	{0x3086, 0xFC5C},
	{0x3086, 0x1D8D},
	{0x3086, 0x6057},
	{0x3086, 0x5449},
	{0x3086, 0x5F53},
	{0x3086, 0x0553},
	{0x3086, 0x074D},
	{0x3086, 0x2BF8},
	{0x3086, 0x1016},
	{0x3086, 0x4C08},
	{0x3086, 0x5556},
	{0x3086, 0x2BB8},
	{0x3086, 0x2B98},
	{0x3086, 0x4E11},
	{0x3086, 0x2904},
	{0x3086, 0x2984},
	{0x3086, 0x2994},
	{0x3086, 0x605C},
	{0x3086, 0x195C},
	{0x3086, 0x1B45},
	{0x3086, 0x4845},
	{0x3086, 0x0845},
	{0x3086, 0x8829},
	{0x3086, 0xB68E},
	{0x3086, 0x012A},
	{0x3086, 0xF83E},
	{0x3086, 0x022A},
	{0x3086, 0xFA3F},
	{0x3086, 0x095C},
	{0x3086, 0x1B29},
	{0x3086, 0xB23F},
	{0x3086, 0x0C3E},
	{0x3086, 0x023E},
	{0x3086, 0x135C},
	{0x3086, 0x133F},
	{0x3086, 0x113E},
	{0x3086, 0x0B5F},
	{0x3086, 0x2B90},
	{0x3086, 0x2AF2},
	{0x3086, 0x2B80},
	{0x3086, 0x3E04},
	{0x3086, 0x3F06},
	{0x3086, 0x6029},
	{0x3086, 0xA229},
	{0x3086, 0xA35F},
	{0x3086, 0x4D1C},
	{0x3086, 0x2AFA},
	{0x3086, 0x2983},
	{0x3086, 0x45A8},
	{0x3086, 0x3E07},
	{0x3086, 0x2AFB},
	{0x3086, 0x3E29},
	{0x3086, 0x4588},
	{0x3086, 0x243E},
	{0x3086, 0x082A},
	{0x3086, 0xFA5D},
	{0x3086, 0x2992},
	{0x3086, 0x8810},
	{0x3086, 0x2B04},
	{0x3086, 0x8B16},
	{0x3086, 0x868D},
	{0x3086, 0x484D},
	{0x3086, 0x4E2B},
	{0x3086, 0x804C},
	{0x3086, 0x0B60},
	{0x3086, 0x3F28},
	{0x3086, 0x2AF2},
	{0x3086, 0x3F0F},
	{0x3086, 0x2982},
	{0x3086, 0x2983},
	{0x3086, 0x2943},
	{0x3086, 0x5C15},
	{0x3086, 0x5F4D},
	{0x3086, 0x1C2A},
	{0x3086, 0xFA45},
	{0x3086, 0x588E},
	{0x3086, 0x002A},
	{0x3086, 0x983F},
	{0x3086, 0x064A},
	{0x3086, 0x739D},
	{0x3086, 0x0A43},
	{0x3086, 0x160B},
	{0x3086, 0x4316},
	{0x3086, 0x8E03},
	{0x3086, 0x2A9C},
	{0x3086, 0x4578},
	{0x3086, 0x3F07},
	{0x3086, 0x2A9D},
	{0x3086, 0x3E12},
	{0x3086, 0x4558},
	{0x3086, 0x3F04},
	{0x3086, 0x8E01},
	{0x3086, 0x2A98},
	{0x3086, 0x8E00},
	{0x3086, 0x9176},
	{0x3086, 0x9C77},
	{0x3086, 0x9C46},
	{0x3086, 0x4416},
	{0x3086, 0x1690},
	{0x3086, 0x7A12},
	{0x3086, 0x444B},
	{0x3086, 0x4A00},
	{0x3086, 0x4316},
	{0x3086, 0x6343},
	{0x3086, 0x1608},
	{0x3086, 0x4316},
	{0x3086, 0x5043},
	{0x3086, 0x1665},
	{0x3086, 0x4316},
	{0x3086, 0x6643},
	{0x3086, 0x168E},
	{0x3086, 0x032A},
	{0x3086, 0x9C45},
	{0x3086, 0x783F},
	{0x3086, 0x072A},
	{0x3086, 0x9D5D},
	{0x3086, 0x0C29},
	{0x3086, 0x4488},
	{0x3086, 0x102B},
	{0x3086, 0x0453},
	{0x3086, 0x0D8B},
	{0x3086, 0x1686},
	{0x3086, 0x3E1F},
	{0x3086, 0x4558},
	{0x3086, 0x283E},
	{0x3086, 0x068E},
	{0x3086, 0x012A},
	{0x3086, 0x988E},
	{0x3086, 0x008D},
	{0x3086, 0x6012},
	{0x3086, 0x444B},
	{0x3086, 0x2C2C},
	{0x3086, 0x2C2C},
	{0x3082, 0x0008}, /* HDR 16x; 0: 4x; 4: 8x; 8: 16x; C: 32x */
	{0x2420, 0x0000},
	{0x2440, 0x0004},
	{0x2442, 0x0080},
	{0x301E, 0x0000},
	{0x2450, 0x0000},
	{0x320A, 0x0080},
	{0x31D0, 0x0000},
	{0x2400, 0x0002},
	{0x2410, 0x0005},
	{0x2412, 0x002D},
	{0x2444, 0xF400},
	{0x2446, 0x0001},
	{0x2438, 0x0010},
	{0x243A, 0x0012},
	{0x243C, 0xFFFF},
	{0x243E, 0x0100},
	{0x3206, 0x0B08},
	{0x3208, 0x1E13},
	{0x3202, 0x0080},
	{0x3200, 0x02  },
	{0x3190, 0x0000},
	{0x318A, 0x0E74},
	{0x318C, 0xC000},
	{0x3192, 0x0400},
	{0x3198, 0x183C},
	{0x3060, 0x000B},
	{0x3096, 0x0480},
	{0x3098, 0x0480},
	{0x3206, 0x0B08},
	{0x3208, 0x1E13},
	{0x3202, 0x0080},
	{0x3200, 0x02  },
	{0x3100, 0x0000}, /* DCG off */
	{0x31AE, 0x0304}, /* HiSpi 4 lane */
	{0x31C6, 0x8002}, /* HiSpi Steraming-S, 0x0402:HiSpi protocol Packetized-SP; MSB output */
	{0x306E, 0x9010}, /* datapath_select */
	{0x301A, 0x0058},
	{0x318C, 0x0000},
	{0x3200, 0x0000}, /* ADACD_CONTROL */
	{0x31D0, 0x0000}, /* COMPANDING */
	{0x2400, 0x0003},
	{0x301E, 0x00A8},
	{0x2450, 0x0000},
	{0x320A, 0x0080},

	/*pll regs*/
	{0x302A, 0x0006}, /* VT_PIX_CLK_DIV  */
	{0x302C, 0x0001}, /* VT_SYS_CLK_DIV  */
	{0x302E, 0x0004}, /* PRE_PLL_CLK_DIV */
	{0x3030, 0x0042}, /* PLL_MULTIPLIER  */
	{0x3036, 0x000C}, /* OP_PIX_CLK_DIV  */
	{0x3038, 0x0001}, /* OP_SYS_CLK_DIV  */

	{0x3004, 0x0000}, /* x_addr_start */
	{0x3008, 0x0787}, /* x_addr_end, 1927 */
	{0x3002, 0x0000}, /* y_addr_start */
	{0x3006, 0x0437}, /* y_addr_end, 1079 */
	{0x30A2, 0x0001}, /* x_odd_inc */
	{0x30A6, 0x0001}, /* y_odd_inc */
	{0x3040, 0x0000}, /* read mode */
	{0x300A, 0x04c8}, /* FRAME_LENGTH_LINES 29.97FPS */
	{0x300C, 2024  }, /* 2200 2032 LINE_LENGTH_PCK Venkatesh */
	{0x3012, 655   }, /* coarse_integration_time */
	{0x305E, 0x0085}, /* min sesnor digital gain 1.04x */
	{0x301A, 0x0058}, /* RESET_REGISTER */
	{0x3064, 0x1802}, /* SMIA_TEST */
	{0x318E, 0x1200}, /* HDR_MC_CTRL3 */
	{0x301A, 0x005C}, /* RESET_REGISTER */
};

static u32 set_vin_config_dram[] = {
	0xc00003d6, 0x32100000, 0x00987654, 0x00000000,
	0x00000000, 0x077f0000, 0x0002043b, 0x001101dc,
	0x00000000, 0x80008000, 0x8000ff00, 0x00000000,
	0xab000000, 0x00000000, 0x00000000, 0x00000000,
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
	.global_cfg_reg_word0 = 8
};

static set_vin_config_t set_vin_config = {
	.cmd_code = SET_VIN_CONFIG,
	.vin_width = 1920,
	.vin_height = 1084,
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
	.sensor_resolution = 12,
	.sensor_pattern = 2,
	.first_line_field_0 = 0,
	.first_line_field_1 = 0,
	.first_line_field_2 = 0,
	.first_line_field_3 = 0,
	.first_line_field_4 = 0,
	.first_line_field_5 = 0,
	.first_line_field_6 = 0,
	.first_line_field_7 = 0,
	.sensor_readout_mode = 0
};

static video_preproc_t video_preprocessing = {
	.cmd_code = VIDEO_PREPROCESSING,
	.input_format = 0,
	.sensor_id = 127,
	.keep_states = 0,
	.vin_frame_rate = 1,
	.vidcap_w = 1920,
	.vidcap_h = 1084,
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
	.bit_resolution = 12,
	.bayer_pattern = 2,
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
	.actual_right_bot_y = 71041024,
	.dummy_window_x_left = 0,
	.dummy_window_y_top = 0,
	.dummy_window_width = 1920,
	.dummy_window_height = 1084,
	.cfa_output_width = 1920,
	.cfa_output_height = 1084,
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
	.preview_C_max_width = 720,
	/* END */
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
	.raw_max_width = 1920,
	.raw_max_height = 1084,
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
	.en_loop_filter = 1,
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
	.SAR_width = 270,
	.SAR_height = 271,
	.video_format = 5,
	.colour_primaries = 1,
	.transfer_characteristics = 1,
	.matrix_coefficients = 1,
	.chroma_sample_loc_type_top_field = 0,
	.chroma_sample_loc_type_bottom_field = 0,
	.aspect_ratio_idc = 255,
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
	.P_IntraBiasAdd = 0,
	.B_IntraBiasAdd = 0,
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

#if defined(CONFIG_S2LMKIWI_DSP_LOG_CAPTURE)
static dsp_debug_level_setup_t dsp_debug_level_setup =  {
	.cmd_code = DSP_DEBUG_LEVEL_SETUP,
	.module = 0,
	.debug_level = 1,
	.coding_thread_printf_disable_mask = 0
};
#endif

static void rct_set_so_pll_ar0230(void)
{
	writel(CLK_SI_INPUT_MODE_REG, 0x0);
	writel(PLL_SENSOR_FRAC_REG, 0x40000863);
	writel(PLL_SENSOR_CTRL_REG, 0x131b1108);
	writel(PLL_SENSOR_CTRL_REG, 0x131b1109);
	writel(PLL_SENSOR_CTRL_REG, 0x131b1108);
	writel(SCALER_SENSOR_POST_REG, 0x00000021);
	writel(SCALER_SENSOR_POST_REG, 0x00000020);
	writel(PLL_SENSOR_CTRL2_REG, 0x3f770000);
	writel(PLL_SENSOR_CTRL3_REG, 0x00069300);
}

static int ar0230_init_hdr_2x(void)
{
	int i;

	/* set pll sent to sensor */
	rct_set_so_pll_ar0230();

	/* ar0230 can work with 400K I2C, so we use it to reduce timing */
	idc_bld_init(IDC_MASTER1, 400000);

	for (i = 0; i < ARRAY_SIZE(ar0230_2x_hdr_regs); i++) {
		idc_bld_write_16_16(IDC_MASTER1, 0x20,
				ar0230_2x_hdr_regs[i].addr,
				ar0230_2x_hdr_regs[i].data);
	}

	/* store vin video format in amboot, restore it after enter Linux IAV */
	memcpy((void *)(DSP_VIN_VIDEO_FORMAT_STORE_START),
		&vin_video_boot_format, sizeof(vin_video_boot_format));

	/* store dsp vin config in amboot, restore it after enter Linux IAV*/
	memcpy((void *)(DSP_VIN_CONFIG_STORE_START),
		set_vin_config_dram, sizeof(set_vin_config_dram));

	putstr_debug(__FUNCTION__);
	return 0;
}

static inline void prepare_vin_vout_dsp_cmd(void)
{
#if defined(CONFIG_S2LMKIWI_DSP_VOUT_CVBS)
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

#elif defined(CONFIG_S2LMKIWI_DSP_VOUT_TD043)
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
	putstr_debug("iav_boot_encode");
	return 0;
}

#if defined(CONFIG_S2LMKIWI_DSP_LOG_CAPTURE)
static int iav_boot_dsplog(void)
{
	add_dsp_cmd(&dsp_debug_level_setup, sizeof(dsp_debug_level_setup));
	return 0;
}
#endif

