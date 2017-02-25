/*
 * Filename : ar0130_reg_tbl.c
 *
 * History:
 *    2012/05/23 - [Long Zhao] Create
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

static struct vin_reg_16_16 ar0130_pll_regs[][4] = {
	{
		{AR0130_VT_PIX_CLK_DIV, 4},
		{AR0130_VT_SYS_CLK_DIV, 2},
		{AR0130_PRE_PLL_CLK_DIV, 2},
		{AR0130_PLL_MULTIPLIER, 44},
	},
};

static struct vin_video_pll ar0130_plls[] = {
	/* for 30/60fps */
	{0, 27036000, 74349000},
	/* for 29.97fps */
	{0, 27008964, 74274651},
};

static struct vin_precise_fps ar0130_precise_fps[] = {
	{AMBA_VIDEO_FPS_29_97, AMBA_VIDEO_MODE_1280_960, 1},
	{AMBA_VIDEO_FPS_29_97, AMBA_VIDEO_MODE_720P, 1},
	{AMBA_VIDEO_FPS_30, AMBA_VIDEO_MODE_1280_960, 0},
	{AMBA_VIDEO_FPS_30, AMBA_VIDEO_MODE_720P, 0},
	{AMBA_VIDEO_FPS_60, AMBA_VIDEO_MODE_720P, 0},
	{AMBA_VIDEO_FPS_29_97, 0, 1},
	{AMBA_VIDEO_FPS_30, 0, 0},
};

static struct vin_reg_16_16 ar0130_mode_regs[][7] = {
	{ /* 1280x960 */
		{0x300C, 0x0672}, //AR0130_LINE_LENGTH_PCK
		{0x300A, 0x03DE}, //AR0130_FRAME_LENGTH_LINES
		{0x3002, 0x0002}, //AR0130_Y_ADDR_START
		{0x3004, 0x0000}, //AR0130_X_ADDR_START
		{0x3006, 0x03C1}, //AR0130_Y_ADDR_END
		{0x3008, 0x04FF}, //AR0130_X_ADDR_END
		{0x30A6, 0x0001}, //AR0130_Y_ODD_INC
	},
	{ /* 1280x720 */
		{0x300C, 0x0672}, //AR0130_LINE_LENGTH_PCK
		{0x300A, 0x02EF}, //AR0130_FRAME_LENGTH_LINES
		{0x3002, 0x0002}, //AR0130_Y_ADDR_START
		{0x3004, 0x0000}, //AR0130_X_ADDR_START
		{0x3006, 0x02D1}, //AR0130_Y_ADDR_END
		{0x3008, 0x04FF}, //AR0130_X_ADDR_END
		{0x30A6, 0x0001}, //AR0130_Y_ODD_INC
	},
};

static struct vin_video_format ar0130_formats[] = {
	{
		.video_mode	= AMBA_VIDEO_MODE_1280_960,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 1280,
		.def_height	= 960,
		/* sensor mode */
		.device_mode	= 0,
		.pll_idx	= 0,
		.width		= 1280,
		.height		= 960,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.max_fps	= AMBA_VIDEO_FPS(45),
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_30,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_720P,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 1280,
		.def_height	= 720,
		/* sensor mode */
		.device_mode	= 1,
		.pll_idx	= 0,
		.width		= 1280,
		.height		= 720,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_60,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
};

static struct vin_reg_16_16 ar0130_share_regs_linear_mode[] = {
	/* AR0130 Rev1 Linear sequencer load 4-27-2010 */
	{0x3088, 0x8000}, // SEQ_CTRL_PORT
	{0x3086, 0x0225}, // SEQ_DATA_PORT
	{0x3086, 0x5050}, // SEQ_DATA_PORT
	{0x3086, 0x2D26}, // SEQ_DATA_PORT
	{0x3086, 0x0828}, // SEQ_DATA_PORT
	{0x3086, 0x0D17}, // SEQ_DATA_PORT
	{0x3086, 0x0926}, // SEQ_DATA_PORT
	{0x3086, 0x0028}, // SEQ_DATA_PORT
	{0x3086, 0x0526}, // SEQ_DATA_PORT
	{0x3086, 0xA728}, // SEQ_DATA_PORT
	{0x3086, 0x0725}, // SEQ_DATA_PORT
	{0x3086, 0x8080}, // SEQ_DATA_PORT
	{0x3086, 0x2917}, // SEQ_DATA_PORT
	{0x3086, 0x0525}, // SEQ_DATA_PORT
	{0x3086, 0x0040}, // SEQ_DATA_PORT
	{0x3086, 0x2702}, // SEQ_DATA_PORT
	{0x3086, 0x1616}, // SEQ_DATA_PORT
	{0x3086, 0x2706}, // SEQ_DATA_PORT
	{0x3086, 0x1736}, // SEQ_DATA_PORT
	{0x3086, 0x26A6}, // SEQ_DATA_PORT
	{0x3086, 0x1703}, // SEQ_DATA_PORT
	{0x3086, 0x26A4}, // SEQ_DATA_PORT
	{0x3086, 0x171F}, // SEQ_DATA_PORT
	{0x3086, 0x2805}, // SEQ_DATA_PORT
	{0x3086, 0x2620}, // SEQ_DATA_PORT
	{0x3086, 0x2804}, // SEQ_DATA_PORT
	{0x3086, 0x2520}, // SEQ_DATA_PORT
	{0x3086, 0x2027}, // SEQ_DATA_PORT
	{0x3086, 0x0017}, // SEQ_DATA_PORT
	{0x3086, 0x1E25}, // SEQ_DATA_PORT
	{0x3086, 0x0020}, // SEQ_DATA_PORT
	{0x3086, 0x2117}, // SEQ_DATA_PORT
	{0x3086, 0x1028}, // SEQ_DATA_PORT
	{0x3086, 0x051B}, // SEQ_DATA_PORT
	{0x3086, 0x1703}, // SEQ_DATA_PORT
	{0x3086, 0x2706}, // SEQ_DATA_PORT
	{0x3086, 0x1703}, // SEQ_DATA_PORT
	{0x3086, 0x1747}, // SEQ_DATA_PORT
	{0x3086, 0x2660}, // SEQ_DATA_PORT
	{0x3086, 0x17AE}, // SEQ_DATA_PORT
	{0x3086, 0x2500}, // SEQ_DATA_PORT
	{0x3086, 0x9027}, // SEQ_DATA_PORT
	{0x3086, 0x0026}, // SEQ_DATA_PORT
	{0x3086, 0x1828}, // SEQ_DATA_PORT
	{0x3086, 0x002E}, // SEQ_DATA_PORT
	{0x3086, 0x2A28}, // SEQ_DATA_PORT
	{0x3086, 0x081E}, // SEQ_DATA_PORT
	{0x3086, 0x0831}, // SEQ_DATA_PORT
	{0x3086, 0x1440}, // SEQ_DATA_PORT
	{0x3086, 0x4014}, // SEQ_DATA_PORT
	{0x3086, 0x2020}, // SEQ_DATA_PORT
	{0x3086, 0x1410}, // SEQ_DATA_PORT
	{0x3086, 0x1034}, // SEQ_DATA_PORT
	{0x3086, 0x1400}, // SEQ_DATA_PORT
	{0x3086, 0x1014}, // SEQ_DATA_PORT
	{0x3086, 0x0020}, // SEQ_DATA_PORT
	{0x3086, 0x1400}, // SEQ_DATA_PORT
	{0x3086, 0x4013}, // SEQ_DATA_PORT
	{0x3086, 0x1802}, // SEQ_DATA_PORT
	{0x3086, 0x1470}, // SEQ_DATA_PORT
	{0x3086, 0x7004}, // SEQ_DATA_PORT
	{0x3086, 0x1470}, // SEQ_DATA_PORT
	{0x3086, 0x7003}, // SEQ_DATA_PORT
	{0x3086, 0x1470}, // SEQ_DATA_PORT
	{0x3086, 0x7017}, // SEQ_DATA_PORT
	{0x3086, 0x2002}, // SEQ_DATA_PORT
	{0x3086, 0x1400}, // SEQ_DATA_PORT
	{0x3086, 0x2002}, // SEQ_DATA_PORT
	{0x3086, 0x1400}, // SEQ_DATA_PORT
	{0x3086, 0x5004}, // SEQ_DATA_PORT
	{0x3086, 0x1400}, // SEQ_DATA_PORT
	{0x3086, 0x2004}, // SEQ_DATA_PORT
	{0x3086, 0x1400}, // SEQ_DATA_PORT
	{0x3086, 0x5022}, // SEQ_DATA_PORT
	{0x3086, 0x0314}, // SEQ_DATA_PORT
	{0x3086, 0x0020}, // SEQ_DATA_PORT
	{0x3086, 0x0314}, // SEQ_DATA_PORT
	{0x3086, 0x0050}, // SEQ_DATA_PORT
	{0x3086, 0x2C2C}, // SEQ_DATA_PORT
	{0x3086, 0x2C2C}, // SEQ_DATA_PORT

	{0x309E, 0x0000}, // DCDS_PROG_START_ADDR
	{0x30E4, 0x6372}, // ADC_BITS_6_7
	{0x30E2, 0x7253}, // ADC_BITS_4_5
	{0x30E0, 0x5470}, // ADC_BITS_2_3
	{0x30E6, 0xC4CC}, // ADC_CONFIG1
	{0x30E8, 0x8050}, // ADC_CONFIG2

	{0xff00, 200}, //delay 200ms

	{0x3082, 0x0029}, // OPERATION_MODE_CTRL

	/* AR0130 Rev1 Optimized settings 8-2-2011 */
	/* improves anti eclipse performance */
	{0x301E, 0x00C8}, // set datapedestal to 200 to avoid clipping near saturation
	{0x3EDA, 0x0F03}, // set vln_dac to 0x3 as recommended by Sergey
	{0x3EDE, 0xC005},
	{0x3ED8, 0x09EF}, // improves eclipse performance at high converstion gain
	{0x3EE2, 0xA46B},
	{0x3EE0, 0x047D}, // enable anti eclipse and adjust setting for high conversion gain
	{0x3EDC, 0x0070}, // adjust anti eclipse setting for low conversion gain
	{0x3EE6, 0x8303}, // improves low light FPN
	{0x3EE4, 0xD208}, // enable analog row noise correction
	{0x3ED6, 0x00BD},
	{0x30B0, 0x1300}, // disable AGS, set Column gain to 1x
	{0x30D4, 0xE007}, // enable double sampling for column correction
	{0x301A, 0x10DC}, // RESET_REGISTER
	{0xff00, 500}, // delay 500ms
	{0x301A, 0x10D8}, // RESET_REGISTER
	{0xff00, 500}, // delay 500ms

	{0x3044, 0x0400}, // DARK_CONTROL
	{0x3012, 0x02A0}, // COARSE_INTEGRATION_TIME
	{0x3100, 0x0000}, // AE_CTRL_REG

	{0x301A, 0x10D8}, // RESET_REGISTER
	{0x31D0, 0x0001}, // HDR_COMP
	{0x3064, 0x1802}, // EMBEDDED_DATA_CTRL
	{0x30B0, 0x1300}, // DIGITAL_TEST
	{0x30BA, 0x0000}, // Disable ags/dcg colcumn FPN correction

	{0xff00, 100}, // delay 100ms

	{0x301A, 0x10DC}, 	// RESET_REGISTER
	{0xffff, 0xffff},
};

/** AR0130 global gain table row size */
#define AR0130_GAIN_ROWS		241
#define AR0130_GAIN_COLS 		3

#define AR0130_GAIN_DOUBLE	32
#define AR0130_GAIN_MAX_DB	0
#define AR0130_GAIN_0DB		(AR0130_GAIN_ROWS - 1)

#define AR0130_GAIN_COL_DCG			0
#define AR0130_GAIN_COL_COL_GAIN		1
#define AR0130_GAIN_COL_DIGITAL_GAIN	2


/* refer to MT9M023-M033_Registers.pdf about R0x3100 about dcg gain
	dcg set to 4 will manually enable special gain
 */
const u16 AR0130_GLOBAL_GAIN_TABLE[AR0130_GAIN_ROWS][AR0130_GAIN_COLS] =
{
	/* dcg,  col_gain,  dig_gain */
	/* 	0x0004 is high dcg gain,
		0x1330 is 8X column gain,
		0x00FF is digital gain. digital gain step is about 0.1875 db
	*/
	{ 0x0004, 0x1330, 0x00FF },  /* 45	dB */
	{ 0x0004, 0x1330, 0x00FB },  /* 35.8125 dB +9 dB */
	{ 0x0004, 0x1330, 0x00F5 },  /* 35.625	dB +9 dB */
	{ 0x0004, 0x1330, 0x00F0 },  /* 35.4375 dB +9 dB */
	{ 0x0004, 0x1330, 0x00EB },  /* 35.25	dB +9 dB */
	{ 0x0004, 0x1330, 0x00E6 },  /* 35.0625 dB +9 dB */
	{ 0x0004, 0x1330, 0x00E1 },  /* 34.875	dB +9 dB */
	{ 0x0004, 0x1330, 0x00DC },  /* 34.6875 dB +9 dB */
	{ 0x0004, 0x1330, 0x00D7 },  /* 34.5	dB +9 dB */
	{ 0x0004, 0x1330, 0x00D3 },  /* 34.3125 dB +9 dB */
	{ 0x0004, 0x1330, 0x00CE },  /* 34.125	dB +9 dB */
	{ 0x0004, 0x1330, 0x00CA },  /* 33.9375 dB +9 dB */
	{ 0x0004, 0x1330, 0x00C5 },  /* 33.75	dB +9 dB */
	{ 0x0004, 0x1330, 0x00C1 },  /* 33.5625 dB +9 dB */
	{ 0x0004, 0x1330, 0x00BD },  /* 33.375	dB +9 dB */
	{ 0x0004, 0x1330, 0x00B9 },  /* 33.1875 dB +9 dB */
	{ 0x0004, 0x1330, 0x00B5 },  /* 42	dB */

	{ 0x0004, 0x1330, 0x00B1 },  /* 32.8125 dB +9 dB */
	{ 0x0004, 0x1330, 0x00AD },  /* 32.625	dB +9 dB */
	{ 0x0004, 0x1330, 0x00AA },  /* 32.4375 dB +9 dB */
	{ 0x0004, 0x1330, 0x00A6 },  /* 32.25	dB +9 dB */
	{ 0x0004, 0x1330, 0x00A2 },  /* 32.0625 dB +9 dB */
	{ 0x0004, 0x1330, 0x009F },  /* 31.875	dB +9 dB */
	{ 0x0004, 0x1330, 0x009C },  /* 31.6875 dB +9 dB */
	{ 0x0004, 0x1330, 0x0098 },  /* 31.5	dB +9 dB */
	{ 0x0004, 0x1330, 0x0095 },  /* 31.3125 dB +9 dB */
	{ 0x0004, 0x1330, 0x0092 },  /* 31.125	dB +9 dB */
	{ 0x0004, 0x1330, 0x008F },  /* 30.9375 dB +9 dB */
	{ 0x0004, 0x1330, 0x008C },  /* 30.75	dB +9 dB */
	{ 0x0004, 0x1330, 0x0089 },  /* 30.5625 dB +9 dB */
	{ 0x0004, 0x1330, 0x0086 },  /* 30.375	dB +9 dB */
	{ 0x0004, 0x1330, 0x0083 },  /* 30.1875 dB +9 dB */
	{ 0x0004, 0x1330, 0x0080 },  /* 39	dB */
	{ 0x0004, 0x1330, 0x007D },  /* 29.8125 dB +9 dB */
	{ 0x0004, 0x1330, 0x007B },  /* 29.625	dB +9 dB */
	{ 0x0004, 0x1330, 0x0078 },  /* 29.4375 dB +9 dB */
	{ 0x0004, 0x1330, 0x0075 },  /* 29.25	dB +9 dB */
	{ 0x0004, 0x1330, 0x0073 },  /* 29.0625 dB +9 dB */
	{ 0x0004, 0x1330, 0x0070 },  /* 28.875	dB +9 dB */
	{ 0x0004, 0x1330, 0x006E },  /* 28.6875 dB +9 dB */
	{ 0x0004, 0x1330, 0x006C },  /* 28.5	dB +9 dB */
	{ 0x0004, 0x1330, 0x0069 },  /* 28.3125 dB +9 dB */
	{ 0x0004, 0x1330, 0x0067 },  /* 28.125	dB +9 dB */
	{ 0x0004, 0x1330, 0x0065 },  /* 27.9375 dB +9 dB */
	{ 0x0004, 0x1330, 0x0063 },  /* 27.75	dB +9 dB */
	{ 0x0004, 0x1330, 0x0061 },  /* 27.5625 dB +9 dB */
	{ 0x0004, 0x1330, 0x005F },  /* 27.375	dB +9 dB */
	{ 0x0004, 0x1330, 0x005C },  /* 27.1875 dB +9 dB */
	{ 0x0004, 0x1330, 0x005B },  /* 36	dB */

	{ 0x0004, 0x1330, 0x0059 },  /* 26.8125 dB +9 dB */
	{ 0x0004, 0x1330, 0x0057 },  /* 26.625	dB +9 dB */
	{ 0x0004, 0x1330, 0x0055 },  /* 26.4375 dB +9 dB */
	{ 0x0004, 0x1330, 0x0053 },  /* 26.25	dB +9 dB */
	{ 0x0004, 0x1330, 0x0051 },  /* 26.0625 dB +9 dB */
	{ 0x0004, 0x1330, 0x004F },  /* 25.875	dB +9 dB */
	{ 0x0004, 0x1330, 0x004E },  /* 25.6875 dB +9 dB */
	{ 0x0004, 0x1330, 0x004C },  /* 25.5	dB +9 dB */
	{ 0x0004, 0x1330, 0x004A },  /* 25.3125 dB +9 dB */
	{ 0x0004, 0x1330, 0x0049 },  /* 25.125	dB +9 dB */
	{ 0x0004, 0x1330, 0x0047 },  /* 24.9375 dB +9 dB */
	{ 0x0004, 0x1330, 0x0046 },  /* 24.75	dB +9 dB */
	{ 0x0004, 0x1330, 0x0044 },  /* 24.5625 dB +9 dB */
	{ 0x0004, 0x1330, 0x0043 },  /* 24.375	dB +9 dB */
	{ 0x0004, 0x1330, 0x0041 },  /* 24.1875 dB +9 dB */
	{ 0x0004, 0x1330, 0x0040 },  /* 	33	dB */
	{ 0x0004, 0x1330, 0x003F },  /* 	23.8125 dB +9 dB */
	{ 0x0004, 0x1330, 0x003D },  /* 	23.625	dB +9 dB */
	{ 0x0004, 0x1330, 0x003C },  /* 	23.4375 dB +9 dB */
	{ 0x0004, 0x1330, 0x003B },  /* 	23.25	dB +9 dB */
	{ 0x0004, 0x1330, 0x0039 },  /* 	23.0625 dB +9 dB */
	{ 0x0004, 0x1330, 0x0038 },  /* 	2.875	dB +9 dB */
	{ 0x0004, 0x1330, 0x0037 },  /* 	22.6875 dB +9 dB */
	{ 0x0004, 0x1330, 0x0036 },  /* 	22.5	dB +9 dB */
	{ 0x0004, 0x1330, 0x0035 },  /* 	22.3125 dB +9 dB */
	{ 0x0004, 0x1330, 0x0034 },  /* 	22.125	dB +9 dB */
	{ 0x0004, 0x1330, 0x0032 },  /* 	21.9375 dB +9 dB */
	{ 0x0004, 0x1330, 0x0031 },  /* 	21.75	dB +9 dB */
	{ 0x0004, 0x1330, 0x0030 },  /* 	21.5625 dB +9 dB */
	{ 0x0004, 0x1330, 0x002F },  /* 	21.375	dB +9 dB */
	{ 0x0004, 0x1330, 0x002E },  /* 	21.1875 dB +9 dB */
	{ 0x0004, 0x1330, 0x002D },  /* 	30	dB */

	{ 0x0004, 0x1320, 0x0059 },  /* 5.0625	dB */
	{ 0x0004, 0x1320, 0x0057 },  /* 4.6875	dB */
	{ 0x0004, 0x1320, 0x0055 },  /* 4.3125	dB */
	{ 0x0004, 0x1320, 0x0054 },  /* 4.125	dB */
	{ 0x0004, 0x1320, 0x0053 },  /* 4.125	dB */
	{ 0x0004, 0x1320, 0x0051 },  /* 3.75	dB */
	{ 0x0004, 0x1320, 0x0050 },  /* 3.5625	dB */
	{ 0x0004, 0x1320, 0x004F },  /* 0.375	dB */
	{ 0x0004, 0x1320, 0x004D },  /* 0	dB */
	{ 0x0004, 0x1320, 0x004B },  /* 5.625	dB *///2.8125
	{ 0x0004, 0x1320, 0x004A },  /* 5.625	dB */
	{ 0x0004, 0x1320, 0x0048 },  /* 5.25	dB */
	{ 0x0004, 0x1320, 0x0047 },  /* 4.125	dB */
	{ 0x0004, 0x1320, 0x0046 },  /* 5.0625	dB */
	{ 0x0004, 0x1320, 0x0044 },  /* 4.6875	dB */
	{ 0x0004, 0x1320, 0x0043 },  /* 4.5 dB */
	{ 0x0004, 0x1320, 0x0041 },  /* 4.125	dB *///1.875
	{ 0x0004, 0x1320, 0x0040 },  /* 4.125	dB */
	{ 0x0004, 0x1320, 0x003F },  /* 9+12+5.8125	dB */
	{ 0x0004, 0x1320, 0x003E },  /* 3.75	dB */
	{ 0x0004, 0x1320, 0x003D},  /* 3.5625	dB */
	{ 0x0004, 0x1320, 0x003B },  /* 3.1875	dB */
	{ 0x0004, 0x1320, 0x0039 },  /* 2.8125	dB */
	{ 0x0004, 0x1320, 0x0038 },  /* 2.625	dB */
	{ 0x0004, 0x1320, 0x0036 },  /* 2.0625	dB */
	{ 0x0004, 0x1320, 0x0035 },  /* 1.875	dB */
	{ 0x0004, 0x1320, 0x0033 },  /* 1.5 dB */
	{ 0x0004, 0x1320, 0x0032 },  /* 1.3125	dB */
	{ 0x0004, 0x1320, 0x0030 },  /* 0.75	dB */
	{ 0x0004, 0x1320, 0x002F },  /* 0.375	dB */
	{ 0x0004, 0x1320, 0x002E },  /* 12+0.09375 db*/
	{ 0x0004, 0x1320, 0x002D },  /* 9+12+3=24dB */

	{ 0x0004, 0x1310, 0x0059 },  /* 5.0625	dB */
	{ 0x0004, 0x1310, 0x0058 },  /* 4.875	dB */
	{ 0x0004, 0x1310, 0x0057 },  /* 4.6875	dB */
	{ 0x0004, 0x1310, 0x0056 },  /* 4.5 dB */
	{ 0x0004, 0x1310, 0x0055 },  /* 4.3125	dB */
	{ 0x0004, 0x1310, 0x0054 },  /* 4.125	dB */
	{ 0x0004, 0x1310, 0x0053 },  /* 4.125	dB */
	{ 0x0004, 0x1310, 0x0052 },  /* 3.9375	dB *///4.6875
	{ 0x0004, 0x1310, 0x0051 },  /* 3.75	dB */
	{ 0x0004, 0x1310, 0x0050 },  /* 3.5625	dB */
	{ 0x0004, 0x1310, 0x004E },  /* 0.09375	dB */
	{ 0x0004, 0x1310, 0x004C},  /* 5.8125	dB */
	{ 0x0004, 0x1310, 0x004B },  /* 5.625	dB *///2.8125
	{ 0x0004, 0x1310, 0x004A },  /* 5.625	dB */
	{ 0x0004, 0x1310, 0x0048 },  /* 5.25	dB */
	{ 0x0004, 0x1310, 0x0046 },  /* 5.0625	dB */
	{ 0x0004, 0x1310, 0x0044 },  /* 4.6875	dB */
	{ 0x0004, 0x1310, 0x0042 },  /* 4.3125	dB */
	{ 0x0004, 0x1310, 0x0040 },  /* 4.125	dB */
	{ 0x0004, 0x1310, 0x003F },  /* 9+6+5.8125	dB */
	{ 0x0004, 0x1310, 0x003D },  /* 5.625	dB */
	{ 0x0004, 0x1310, 0x003B },  /* 5.25	dB */
	{ 0x0004, 0x1310, 0x003A },  /* 4.125	dB */
	{ 0x0004, 0x1310, 0x0038 },  /* 4.875	dB */
	{ 0x0004, 0x1310, 0x0036 },  /* 4.5 dB */
	{ 0x0004, 0x1310, 0x0035 },  /* 4.3125	dB */
	{ 0x0004, 0x1310, 0x0034 },  /* 4.125	dB *///1.875
	{ 0x0004, 0x1310, 0x0032 },  /* 3.9375	dB */
	{ 0x0004, 0x1310, 0x0031 },  /* 3.75	dB */
	{ 0x0004, 0x1310, 0x0030 },  /* 3.5625	dB */
	{ 0x0004, 0x1310, 0x002E },  /* 3.1875	dB */
	{ 0x0004, 0x1310, 0x002D },  /* 9+6+3=18dB */

	{ 0x0004, 0x1300, 0x0059 },  /* 5.0625	dB */
	{ 0x0004, 0x1300, 0x0058 },  /* 4.875	dB */
	{ 0x0004, 0x1300, 0x0057 },  /* 4.6875	dB */
	{ 0x0004, 0x1300, 0x0055 },  /* 4.3125	dB */
	{ 0x0004, 0x1300, 0x0054 },  /* 4.125	dB */
	{ 0x0004, 0x1300, 0x0053 },  /* 4.125	dB */
	{ 0x0004, 0x1300, 0x0051 },  /* 3.75	dB */
	{ 0x0004, 0x1300, 0x0050 },  /* 3.5625	dB */
	{ 0x0004, 0x1300, 0x004F },  /* 0.375	dB */
	{ 0x0004, 0x1300, 0x004D },  /* 0	dB */
	{ 0x0004, 0x1300, 0x004C},  /* 5.8125	dB */
	{ 0x0004, 0x1300, 0x004A },  /* 5.625	dB */
	{ 0x0004, 0x1300, 0x0049 },  /* 5.4375	dB */
	{ 0x0004, 0x1300, 0x0048 },  /* 5.25	dB */
	{ 0x0004, 0x1300, 0x0047 },  /* 4.125	dB */
	{ 0x0004, 0x1300, 0x0046 },  /* 5.0625	dB */
	{ 0x0004, 0x1300, 0x0045 },  /* 4.875	dB */
	{ 0x0004, 0x1300, 0x0044 },  /* 4.6875	dB */
	{ 0x0004, 0x1300, 0x0043 },  /* 4.5 dB */
	{ 0x0004, 0x1300, 0x0042 },  /* 4.3125	dB */
	{ 0x0004, 0x1300, 0x0040 },  /* 4.125	dB */
	{ 0x0004, 0x1300, 0x003E },  /* 5.625	dB *///2.8125
	{ 0x0004, 0x1300, 0x003C },  /* 5.4375	dB */
	{ 0x0004, 0x1300, 0x003A },  /* 4.125	dB */
	{ 0x0004, 0x1300, 0x0038 },  /* 4.875	dB */
	{ 0x0004, 0x1300, 0x0036 },  /* 4.5 dB */
	{ 0x0004, 0x1300, 0x0035 },  /* 4.3125	dB */
	{ 0x0004, 0x1300, 0x0033 },  /* 4.125	dB */
	{ 0x0004, 0x1300, 0x0031 },  /* 3.75	dB */
	{ 0x0004, 0x1300, 0x0030 },  /* 3.5625	dB */
	{ 0x0004, 0x1300, 0x002E },  /* 3.1875	dB */
	{ 0x0004, 0x1300, 0x002D },  /* 9+3=12db	dB */

	{ 0x0000, 0x1310, 0x003F },  /* 6+5.8125 =11.8125	dB */
	{ 0x0000, 0x1310, 0x003E },  /* 5.625	dB *///2.8125
	{ 0x0000, 0x1310, 0x003D },  /* 5.625	dB */
	{ 0x0000, 0x1310, 0x003C },  /* 5.4375	dB */
	{ 0x0000, 0x1310, 0x003B },  /* 5.25	dB */
	{ 0x0000, 0x1310, 0x003A },  /* 4.125	dB */
	{ 0x0000, 0x1310, 0x0039 },  /* 5.0625	dB */
	{ 0x0000, 0x1310, 0x0038 },  /* 4.875	dB */
	{ 0x0000, 0x1310, 0x0037 },  /* 4.6875	dB */
	{ 0x0000, 0x1310, 0x0036 },  /* 4.5 dB */
	{ 0x0000, 0x1310, 0x0035 },  /* 4.3125	dB */
	{ 0x0000, 0x1310, 0x0034 },  /* 4.125	dB *///1.875
	{ 0x0000, 0x1310, 0x0033 },  /* 4.125	dB */
	{ 0x0000, 0x1310, 0x0032 },  /* 3.9375	dB */
	{ 0x0000, 0x1310, 0x0031 },  /* 3.75	dB */
	{ 0x0000, 0x1310, 0x0030 },  /* 3.5625	dB */
	{ 0x0000, 0x1310, 0x002F },  /* 3.375	dB */
	{ 0x0000, 0x1310, 0x002E },  /* 3.1875	dB */
	{ 0x0000, 0x1310, 0x002D },  /* 3	dB */
	{ 0x0000, 0x1310, 0x002C },  /* 2.8125	dB */
	{ 0x0000, 0x1310, 0x002B },  /* 2.625	dB */
	{ 0x0000, 0x1310, 0x002A },  /* 0.9375	dB */
	{ 0x0000, 0x1310, 0x0029 },  /* 2.0625	dB */
	{ 0x0000, 0x1310, 0x0028 },  /* 1.875	dB */
	{ 0x0000, 0x1310, 0x0027 },  /* 1.6875	dB */
	{ 0x0000, 0x1310, 0x0026 },  /* 1.5 dB */
	{ 0x0000, 0x1310, 0x0025 },  /* 1.3125	dB */
	{ 0x0000, 0x1310, 0x0024 },  /* 0.9375	dB */
	{ 0x0000, 0x1310, 0x0023 },  /* 0.75	dB */
	{ 0x0000, 0x1310, 0x0022 },  /* 0.375	dB */
	{ 0x0000, 0x1310, 0x0021 },  /* 0.09375	dB */
	{ 0x0000, 0x1310, 0x0020 },  /* 6	dB */

	{ 0x0000, 0x1300, 0x003F },  /* 5.8125	dB */
	{ 0x0000, 0x1300, 0x003E },  /* 5.625	dB *///2.8125
	{ 0x0000, 0x1300, 0x003D },  /* 5.625	dB */
	{ 0x0000, 0x1300, 0x003C },  /* 5.4375	dB */
	{ 0x0000, 0x1300, 0x003B },  /* 5.25	dB */
	{ 0x0000, 0x1300, 0x003A },  /* 4.125	dB */
	{ 0x0000, 0x1300, 0x0039 },  /* 5.0625	dB */
	{ 0x0000, 0x1300, 0x0038 },  /* 4.875	dB */
	{ 0x0000, 0x1300, 0x0037 },  /* 4.6875	dB */
	{ 0x0000, 0x1300, 0x0036 },  /* 4.5 dB */
	{ 0x0000, 0x1300, 0x0035 },  /* 4.3125	dB */
	{ 0x0000, 0x1300, 0x0034 },  /* 4.125	dB *///1.875
	{ 0x0000, 0x1300, 0x0033 },  /* 4.125	dB */
	{ 0x0000, 0x1300, 0x0032 },  /* 3.9375	dB */
	{ 0x0000, 0x1300, 0x0031 },  /* 3.75	dB */
	{ 0x0000, 0x1300, 0x0030 },  /* 3.5625	dB */
	{ 0x0000, 0x1300, 0x002F },  /* 3.375	dB */
	{ 0x0000, 0x1300, 0x002E },  /* 3.1875	dB */
	{ 0x0000, 0x1300, 0x002D },  /* 3	dB */
	{ 0x0000, 0x1300, 0x002C },  /* 2.8125	dB */
	{ 0x0000, 0x1300, 0x002B },  /* 2.625	dB */
	{ 0x0000, 0x1300, 0x002A },  /* 0.9375	dB */
	{ 0x0000, 0x1300, 0x0029 },  /* 2.0625	dB */
	{ 0x0000, 0x1300, 0x0028 },  /* 1.875	dB */
	{ 0x0000, 0x1300, 0x0027 },  /* 1.6875	dB */
	{ 0x0000, 0x1300, 0x0026 },  /* 1.5 dB */
	{ 0x0000, 0x1300, 0x0025 },  /* 1.3125	dB */
	{ 0x0000, 0x1300, 0x0024 },  /* 0.9375	dB */
	{ 0x0000, 0x1300, 0x0023 },  /* 0.75	dB */
	{ 0x0000, 0x1300, 0x0022 },  /* 0.375	dB */
	{ 0x0000, 0x1300, 0x0021 },  /* 0.09375	dB */
	{ 0x0000, 0x1300, 0x0020 },  /* 0	dB */
};

