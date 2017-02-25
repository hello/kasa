/*
 * Filename : ar0141_reg_tbl.c
 *
 * History:
 *    2014/11/18 - [Long Zhao] Create
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

static struct vin_reg_16_16 ar0141_pll_regs[][6] = {
	{
		{0x302A, 0x0006}, /* VT_PIX_CLK_DIV  */
		{0x302C, 0x0001}, /* VT_SYS_CLK_DIV  */
		{0x302E, 0x0004}, /* PRE_PLL_CLK_DIV */
		{0x3030, 0x0042}, /* PLL_MULTIPLIER  */
		{0x3036, 0x000C}, /* OP_PIX_CLK_DIV  */
		{0x3038, 0x0002}, /* OP_SYS_CLK_DIV  */
	},
};

static struct vin_video_pll ar0141_plls[] = {
	{0, 27000000, 74250000},
};

static struct vin_reg_16_16 ar0141_mode_regs[][10] = {
	{	 /* 1280x800 */
		{0x3002, 0x001C}, /* Y_ADDR_START */
		{0x3004, 0x0016}, /* X_ADDR_START */
		{0x3006, 0x033B}, /* Y_ADDR_END */
		{0x3008, 0x0515}, /* X_ADDR_END */
		{0x300A, 0x05dc}, /* FRAME_LENGTH_LINES */
		{0x300C, 0x0672}, /* LINE_LENGTH_PCK */
		{0x3012, 0x0100}, /* COARSE_INTEGRATION_TIME */
		{0x3040, 0x0000}, /* READ_MODE */
		{0x30A2, 0x0001}, /* X_ODD_INC */
		{0x30A6, 0x0001}, /* Y_ODD_INC */
	},
	{ 	/* 1280x720 */
		{0x3002, 0x0040}, /* Y_ADDR_START */
		{0x3004, 0x0020}, /* X_ADDR_START */
		{0x3006, 0x030F}, /* Y_ADDR_END */
		{0x3008, 0x051F}, /* X_ADDR_END */
		{0x300A, 0x02EE}, /* FRAME_LENGTH_LINES */
		{0x300C, 0x0672}, /* LINE_LENGTH_PCK */
		{0x3012, 0x002D}, /* COARSE_INTEGRATION_TIME */
		{0x3040, 0x0000}, /* READ_MODE */
		{0x30A2, 0x0001}, /* X_ODD_INC */
		{0x30A6, 0x0001}, /* Y_ODD_INC */
	},
	{ 	/* 848x480 */
		{0x3002, 0x00B8}, /* Y_ADDR_START */
		{0x3004, 0x00F8}, /* X_ADDR_START */
		{0x3006, 0x0297}, /* Y_ADDR_END */
		{0x3008, 0x0447}, /* X_ADDR_END */
		{0x300A, 0x01F4}, /* FRAME_LENGTH_LINES */
		{0x300C, 0x0672}, /* LINE_LENGTH_PCK */
		{0x3012, 0x01EF}, /* COARSE_INTEGRATION_TIME */
		{0x3040, 0x0000}, /* READ_MODE */
		{0x30A2, 0x0001}, /* X_ODD_INC */
		{0x30A6, 0x0001}, /* Y_ODD_INC */
	},
};

static struct vin_video_format ar0141_formats[] = {
	{
		.video_mode	= AMBA_VIDEO_MODE_WXGA,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 1280,
		.def_height	= 800,
		/* sensor mode */
		.device_mode	= 0,
		.pll_idx	= 0,
		.width		= 1280,
		.height		= 800,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS(55),
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_30,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_GR,
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
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_GR,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_848_480,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 848,
		.def_height	= 480,
		/* sensor mode */
		.device_mode	= 2,
		.pll_idx	= 0,
		.width		= 848,
		.height		= 480,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS(90),
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_GR,
	},
};

static struct vin_reg_16_16 ar0141_share_regs[] = {
	/* 1280x800@30fps */
	{0x3054, 0x0800}, /* OTPM_EXPR */
	{0x304C, 0x0200}, /* OTPM_RECORD */
	{0x304A, 0x0010}, /* OTPM_CONTROL */
	{0xFFFF, 12},        /* DELAY=12 */
	{0x3054, 0x0800}, /* OTPM_EXPR */
	{0x304C, 0x2000}, /* OTPM_RECORD */
	{0x304A, 0x0210}, /* OTPM_CONTROL */
	{0xFFFF, 6},          /* DELAY=6 */
	{0x3088, 0x8000}, /* SEQ_CTRL_PORT */
	{0x3086, 0x4558}, /* SEQ_DATA_PORT */
	{0x3086, 0x6E9B}, /* SEQ_DATA_PORT */
	{0x3086, 0x4A31}, /* SEQ_DATA_PORT */
	{0x3086, 0x4342}, /* SEQ_DATA_PORT */
	{0x3086, 0x8E03}, /* SEQ_DATA_PORT */
	{0x3086, 0x2714}, /* SEQ_DATA_PORT */
	{0x3086, 0x4578}, /* SEQ_DATA_PORT */
	{0x3086, 0x7B3D}, /* SEQ_DATA_PORT */
	{0x3086, 0xFF3D}, /* SEQ_DATA_PORT */
	{0x3086, 0xFF3D}, /* SEQ_DATA_PORT */
	{0x3086, 0xEA27}, /* SEQ_DATA_PORT */
	{0x3086, 0x043D}, /* SEQ_DATA_PORT */
	{0x3086, 0x1027}, /* SEQ_DATA_PORT */
	{0x3086, 0x0527}, /* SEQ_DATA_PORT */
	{0x3086, 0x1535}, /* SEQ_DATA_PORT */
	{0x3086, 0x2705}, /* SEQ_DATA_PORT */
	{0x3086, 0x3D10}, /* SEQ_DATA_PORT */
	{0x3086, 0x4558}, /* SEQ_DATA_PORT */
	{0x3086, 0x2704}, /* SEQ_DATA_PORT */
	{0x3086, 0x2714}, /* SEQ_DATA_PORT */
	{0x3086, 0x3DFF}, /* SEQ_DATA_PORT */
	{0x3086, 0x3DFF}, /* SEQ_DATA_PORT */
	{0x3086, 0x3DEA}, /* SEQ_DATA_PORT */
	{0x3086, 0x2704}, /* SEQ_DATA_PORT */
	{0x3086, 0x6227}, /* SEQ_DATA_PORT */
	{0x3086, 0x288E}, /* SEQ_DATA_PORT */
	{0x3086, 0x0036}, /* SEQ_DATA_PORT */
	{0x3086, 0x2708}, /* SEQ_DATA_PORT */
	{0x3086, 0x3D64}, /* SEQ_DATA_PORT */
	{0x3086, 0x7A3D}, /* SEQ_DATA_PORT */
	{0x3086, 0x0444}, /* SEQ_DATA_PORT */
	{0x3086, 0x2C4B}, /* SEQ_DATA_PORT */
	{0x3086, 0x8F01}, /* SEQ_DATA_PORT */
	{0x3086, 0x4372}, /* SEQ_DATA_PORT */
	{0x3086, 0x719F}, /* SEQ_DATA_PORT */
	{0x3086, 0x4643}, /* SEQ_DATA_PORT */
	{0x3086, 0x166F}, /* SEQ_DATA_PORT */
	{0x3086, 0x9F92}, /* SEQ_DATA_PORT */
	{0x3086, 0x1244}, /* SEQ_DATA_PORT */
	{0x3086, 0x1646}, /* SEQ_DATA_PORT */
	{0x3086, 0x4316}, /* SEQ_DATA_PORT */
	{0x3086, 0x9326}, /* SEQ_DATA_PORT */
	{0x3086, 0x0426}, /* SEQ_DATA_PORT */
	{0x3086, 0x848E}, /* SEQ_DATA_PORT */
	{0x3086, 0x0327}, /* SEQ_DATA_PORT */
	{0x3086, 0xFC5C}, /* SEQ_DATA_PORT */
	{0x3086, 0x0D57}, /* SEQ_DATA_PORT */
	{0x3086, 0x5417}, /* SEQ_DATA_PORT */
	{0x3086, 0x0955}, /* SEQ_DATA_PORT */
	{0x3086, 0x5649}, /* SEQ_DATA_PORT */
	{0x3086, 0x5F53}, /* SEQ_DATA_PORT */
	{0x3086, 0x0553}, /* SEQ_DATA_PORT */
	{0x3086, 0x0728}, /* SEQ_DATA_PORT */
	{0x3086, 0x6C4C}, /* SEQ_DATA_PORT */
	{0x3086, 0x0928}, /* SEQ_DATA_PORT */
	{0x3086, 0x2C72}, /* SEQ_DATA_PORT */
	{0x3086, 0xA37C}, /* SEQ_DATA_PORT */
	{0x3086, 0x9728}, /* SEQ_DATA_PORT */
	{0x3086, 0xA879}, /* SEQ_DATA_PORT */
	{0x3086, 0x6026}, /* SEQ_DATA_PORT */
	{0x3086, 0x9C5C}, /* SEQ_DATA_PORT */
	{0x3086, 0x1B45}, /* SEQ_DATA_PORT */
	{0x3086, 0x4845}, /* SEQ_DATA_PORT */
	{0x3086, 0x0845}, /* SEQ_DATA_PORT */
	{0x3086, 0x8826}, /* SEQ_DATA_PORT */
	{0x3086, 0xBE8E}, /* SEQ_DATA_PORT */
	{0x3086, 0x0127}, /* SEQ_DATA_PORT */
	{0x3086, 0xF817}, /* SEQ_DATA_PORT */
	{0x3086, 0x0227}, /* SEQ_DATA_PORT */
	{0x3086, 0xFA17}, /* SEQ_DATA_PORT */
	{0x3086, 0x095C}, /* SEQ_DATA_PORT */
	{0x3086, 0x0B17}, /* SEQ_DATA_PORT */
	{0x3086, 0x1026}, /* SEQ_DATA_PORT */
	{0x3086, 0xBA5C}, /* SEQ_DATA_PORT */
	{0x3086, 0x0317}, /* SEQ_DATA_PORT */
	{0x3086, 0x1026}, /* SEQ_DATA_PORT */
	{0x3086, 0xB217}, /* SEQ_DATA_PORT */
	{0x3086, 0x065F}, /* SEQ_DATA_PORT */
	{0x3086, 0x2888}, /* SEQ_DATA_PORT */
	{0x3086, 0x9060}, /* SEQ_DATA_PORT */
	{0x3086, 0x27F2}, /* SEQ_DATA_PORT */
	{0x3086, 0x1710}, /* SEQ_DATA_PORT */
	{0x3086, 0x26A2}, /* SEQ_DATA_PORT */
	{0x3086, 0x26A3}, /* SEQ_DATA_PORT */
	{0x3086, 0x5F4D}, /* SEQ_DATA_PORT */
	{0x3086, 0x2808}, /* SEQ_DATA_PORT */
	{0x3086, 0x1A27}, /* SEQ_DATA_PORT */
	{0x3086, 0xFA84}, /* SEQ_DATA_PORT */
	{0x3086, 0x69A0}, /* SEQ_DATA_PORT */
	{0x3086, 0x785D}, /* SEQ_DATA_PORT */
	{0x3086, 0x2888}, /* SEQ_DATA_PORT */
	{0x3086, 0x8710}, /* SEQ_DATA_PORT */
	{0x3086, 0x8C82}, /* SEQ_DATA_PORT */
	{0x3086, 0x8926}, /* SEQ_DATA_PORT */
	{0x3086, 0xB217}, /* SEQ_DATA_PORT */
	{0x3086, 0x036B}, /* SEQ_DATA_PORT */
	{0x3086, 0x9C60}, /* SEQ_DATA_PORT */
	{0x3086, 0x9417}, /* SEQ_DATA_PORT */
	{0x3086, 0x2926}, /* SEQ_DATA_PORT */
	{0x3086, 0x8345}, /* SEQ_DATA_PORT */
	{0x3086, 0xA817}, /* SEQ_DATA_PORT */
	{0x3086, 0x0727}, /* SEQ_DATA_PORT */
	{0x3086, 0xFB17}, /* SEQ_DATA_PORT */
	{0x3086, 0x2945}, /* SEQ_DATA_PORT */
	{0x3086, 0x8820}, /* SEQ_DATA_PORT */
	{0x3086, 0x1708}, /* SEQ_DATA_PORT */
	{0x3086, 0x27FA}, /* SEQ_DATA_PORT */
	{0x3086, 0x5D87}, /* SEQ_DATA_PORT */
	{0x3086, 0x108C}, /* SEQ_DATA_PORT */
	{0x3086, 0x8289}, /* SEQ_DATA_PORT */
	{0x3086, 0x170E}, /* SEQ_DATA_PORT */
	{0x3086, 0x4826}, /* SEQ_DATA_PORT */
	{0x3086, 0x9A28}, /* SEQ_DATA_PORT */
	{0x3086, 0x884C}, /* SEQ_DATA_PORT */
	{0x3086, 0x0B79}, /* SEQ_DATA_PORT */
	{0x3086, 0x1730}, /* SEQ_DATA_PORT */
	{0x3086, 0x2692}, /* SEQ_DATA_PORT */
	{0x3086, 0x1709}, /* SEQ_DATA_PORT */
	{0x3086, 0x9160}, /* SEQ_DATA_PORT */
	{0x3086, 0x27F2}, /* SEQ_DATA_PORT */
	{0x3086, 0x1710}, /* SEQ_DATA_PORT */
	{0x3086, 0x2682}, /* SEQ_DATA_PORT */
	{0x3086, 0x2683}, /* SEQ_DATA_PORT */
	{0x3086, 0x5F4D}, /* SEQ_DATA_PORT */
	{0x3086, 0x2808}, /* SEQ_DATA_PORT */
	{0x3086, 0x1A27}, /* SEQ_DATA_PORT */
	{0x3086, 0xFA84}, /* SEQ_DATA_PORT */
	{0x3086, 0x69A1}, /* SEQ_DATA_PORT */
	{0x3086, 0x785D}, /* SEQ_DATA_PORT */
	{0x3086, 0x2888}, /* SEQ_DATA_PORT */
	{0x3086, 0x8710}, /* SEQ_DATA_PORT */
	{0x3086, 0x8C80}, /* SEQ_DATA_PORT */
	{0x3086, 0x8A26}, /* SEQ_DATA_PORT */
	{0x3086, 0x9217}, /* SEQ_DATA_PORT */
	{0x3086, 0x036B}, /* SEQ_DATA_PORT */
	{0x3086, 0x9D95}, /* SEQ_DATA_PORT */
	{0x3086, 0x2603}, /* SEQ_DATA_PORT */
	{0x3086, 0x5C01}, /* SEQ_DATA_PORT */
	{0x3086, 0x4558}, /* SEQ_DATA_PORT */
	{0x3086, 0x8E00}, /* SEQ_DATA_PORT */
	{0x3086, 0x2798}, /* SEQ_DATA_PORT */
	{0x3086, 0x170A}, /* SEQ_DATA_PORT */
	{0x3086, 0x4A0A}, /* SEQ_DATA_PORT */
	{0x3086, 0x4316}, /* SEQ_DATA_PORT */
	{0x3086, 0x0B43}, /* SEQ_DATA_PORT */
	{0x3086, 0x5B43}, /* SEQ_DATA_PORT */
	{0x3086, 0x1659}, /* SEQ_DATA_PORT */
	{0x3086, 0x4316}, /* SEQ_DATA_PORT */
	{0x3086, 0x8E03}, /* SEQ_DATA_PORT */
	{0x3086, 0x279C}, /* SEQ_DATA_PORT */
	{0x3086, 0x4578}, /* SEQ_DATA_PORT */
	{0x3086, 0x1707}, /* SEQ_DATA_PORT */
	{0x3086, 0x279D}, /* SEQ_DATA_PORT */
	{0x3086, 0x1722}, /* SEQ_DATA_PORT */
	{0x3086, 0x5D87}, /* SEQ_DATA_PORT */
	{0x3086, 0x1028}, /* SEQ_DATA_PORT */
	{0x3086, 0x0853}, /* SEQ_DATA_PORT */
	{0x3086, 0x0D8C}, /* SEQ_DATA_PORT */
	{0x3086, 0x808A}, /* SEQ_DATA_PORT */
	{0x3086, 0x4558}, /* SEQ_DATA_PORT */
	{0x3086, 0x1708}, /* SEQ_DATA_PORT */
	{0x3086, 0x8E01}, /* SEQ_DATA_PORT */
	{0x3086, 0x2798}, /* SEQ_DATA_PORT */
	{0x3086, 0x8E00}, /* SEQ_DATA_PORT */
	{0x3086, 0x76A2}, /* SEQ_DATA_PORT */
	{0x3086, 0x77A2}, /* SEQ_DATA_PORT */
	{0x3086, 0x4644}, /* SEQ_DATA_PORT */
	{0x3086, 0x1616}, /* SEQ_DATA_PORT */
	{0x3086, 0x967A}, /* SEQ_DATA_PORT */
	{0x3086, 0x2644}, /* SEQ_DATA_PORT */
	{0x3086, 0x5C05}, /* SEQ_DATA_PORT */
	{0x3086, 0x1244}, /* SEQ_DATA_PORT */
	{0x3086, 0x4B71}, /* SEQ_DATA_PORT */
	{0x3086, 0x759E}, /* SEQ_DATA_PORT */
	{0x3086, 0x8B86}, /* SEQ_DATA_PORT */
	{0x3086, 0x184A}, /* SEQ_DATA_PORT */
	{0x3086, 0x0343}, /* SEQ_DATA_PORT */
	{0x3086, 0x1606}, /* SEQ_DATA_PORT */
	{0x3086, 0x4316}, /* SEQ_DATA_PORT */
	{0x3086, 0x0743}, /* SEQ_DATA_PORT */
	{0x3086, 0x1604}, /* SEQ_DATA_PORT */
	{0x3086, 0x4316}, /* SEQ_DATA_PORT */
	{0x3086, 0x5843}, /* SEQ_DATA_PORT */
	{0x3086, 0x165A}, /* SEQ_DATA_PORT */
	{0x3086, 0x4316}, /* SEQ_DATA_PORT */
	{0x3086, 0x4558}, /* SEQ_DATA_PORT */
	{0x3086, 0x8E03}, /* SEQ_DATA_PORT */
	{0x3086, 0x279C}, /* SEQ_DATA_PORT */
	{0x3086, 0x4578}, /* SEQ_DATA_PORT */
	{0x3086, 0x7B17}, /* SEQ_DATA_PORT */
	{0x3086, 0x0727}, /* SEQ_DATA_PORT */
	{0x3086, 0x9D17}, /* SEQ_DATA_PORT */
	{0x3086, 0x2245}, /* SEQ_DATA_PORT */
	{0x3086, 0x5822}, /* SEQ_DATA_PORT */
	{0x3086, 0x1710}, /* SEQ_DATA_PORT */
	{0x3086, 0x8E01}, /* SEQ_DATA_PORT */
	{0x3086, 0x2798}, /* SEQ_DATA_PORT */
	{0x3086, 0x8E00}, /* SEQ_DATA_PORT */
	{0x3086, 0x1710}, /* SEQ_DATA_PORT */
	{0x3086, 0x1244}, /* SEQ_DATA_PORT */
	{0x3086, 0x4B8D}, /* SEQ_DATA_PORT */
	{0x3086, 0x602C}, /* SEQ_DATA_PORT */
	{0x3086, 0x2C2C}, /* SEQ_DATA_PORT */
	{0x3086, 0x2C00}, /* SEQ_DATA_PORT */

	{0x3044, 0x0400}, /* DARK_CONTROL */
	{0x3052, 0xA134}, /* OTPM_CFG */
	{0x3092, 0x010F}, /* ROW_NOISE_CONTROL */
	{0x30FE, 0x0080}, /* NOISE_PEDESTAL */
	{0x3ECE, 0x40FF}, /* DAC_LD_2_3 */
	{0x3ED0, 0xFF40}, /* DAC_LD_4_5 */
	{0x3ED2, 0xA906}, /* DAC_LD_6_7 */
	{0x3ED4, 0x001F}, /* DAC_LD_8_9 */
	{0x3ED6, 0x638F}, /* DAC_LD_10_11 */
	{0x3ED8, 0xCC99}, /* DAC_LD_12_13 */
	{0x3EDA, 0x0888}, /* DAC_LD_14_15 */
	{0x3EDE, 0x8878}, /* DAC_LD_18_19 */
	{0x3EE0, 0x7744}, /* DAC_LD_20_21 */
	{0x3EE2, 0x4463}, /* DAC_LD_22_23 */
	{0x3EE4, 0xAAE0}, /* DAC_LD_24_25 */
	{0x3EE6, 0x1400}, /* DAC_LD_26_27 */
	{0x3EEA, 0xA4FF}, /* DAC_LD_30_31 */
	{0x3EEC, 0x80F0}, /* DAC_LD_32_33 */
	{0x3EEE, 0x0000}, /* DAC_LD_34_35 */
	{0x31E0, 0x1701}, /* PIX_DEF_ID */
	{0x3082, 0x0009}, /* OPERATION_MODE_CTRL */
	{0x318C, 0x0000}, /* HDR_MC_CTRL2 */
	{0x3200, 0x0000}, /* ADACD_CONTROL */
	{0x31D0, 0x0000}, /* COMPANDING */
	{0x30B0, 0x0000}, /* DIGITAL_TEST */
	{0x30BA, 0x012C}, /* DIGITAL_CTRL */
	{0x31AC, 0x0C0C}, /* DATA_FORMAT_BITS */
	{0x31AE, 0x0304}, /* SERIAL_FORMAT */
	{0x31C6, 0x8002}, /* HISPI_CONTROL_STATUS, for ambarella, MSB first */
	{0x3064, 0x1802}, /* DISABLE EMBEDDED DATA */
	{0x301A, 0x0058}, /* RESET_REGISTER */
};

/** AR0141 global gain table row size */
#define AR0141_GAIN_ROWS		469
#define AR0141_GAIN_COLS 		3

#define AR0141_GAIN_48DB		468

#define AR0141_GAIN_COL_AGAIN	0
#define AR0141_GAIN_COL_DGAIN	1
#define AR0141_GAIN_COL_DCG	2

const u16 AR0141_GAIN_TABLE[AR0141_GAIN_ROWS][AR0141_GAIN_COLS] =
{
	/* analog gain 1.6x~12x, and if gain >=2.65, turn on DCG */
	{0x0009, 0x0083, 0x0000}, //index: 0, gain:4.082400db->x1.600000, DCG:0
	{0x0009, 0x0084, 0x0000}, //index: 1, gain:4.176150db->x1.617363, DCG:0
	{0x000a, 0x0080, 0x0000}, //index: 2, gain:4.269900db->x1.634914, DCG:0
	{0x000a, 0x0082, 0x0000}, //index: 3, gain:4.363650db->x1.652656, DCG:0
	{0x000a, 0x0083, 0x0000}, //index: 4, gain:4.457400db->x1.670590, DCG:0
	{0x000b, 0x0080, 0x0000}, //index: 5, gain:4.551150db->x1.688719, DCG:0
	{0x000b, 0x0081, 0x0000}, //index: 6, gain:4.644900db->x1.707045, DCG:0
	{0x000b, 0x0082, 0x0000}, //index: 7, gain:4.738650db->x1.725570, DCG:0
	{0x000b, 0x0084, 0x0000}, //index: 8, gain:4.832400db->x1.744295, DCG:0
	{0x000c, 0x0080, 0x0000}, //index: 9, gain:4.926150db->x1.763224, DCG:0
	{0x000c, 0x0082, 0x0000}, //index: 10, gain:5.019900db->x1.782358, DCG:0
	{0x000c, 0x0083, 0x0000}, //index: 11, gain:5.113650db->x1.801700, DCG:0
	{0x000d, 0x0080, 0x0000}, //index: 12, gain:5.207400db->x1.821252, DCG:0
	{0x000d, 0x0082, 0x0000}, //index: 13, gain:5.301150db->x1.841016, DCG:0
	{0x000d, 0x0083, 0x0000}, //index: 14, gain:5.394900db->x1.860994, DCG:0
	{0x000e, 0x0080, 0x0000}, //index: 15, gain:5.488650db->x1.881189, DCG:0
	{0x000e, 0x0081, 0x0000}, //index: 16, gain:5.582400db->x1.901604, DCG:0
	{0x000e, 0x0083, 0x0000}, //index: 17, gain:5.676150db->x1.922240, DCG:0
	{0x000f, 0x0080, 0x0000}, //index: 18, gain:5.769900db->x1.943099, DCG:0
	{0x000f, 0x0081, 0x0000}, //index: 19, gain:5.863650db->x1.964185, DCG:0
	{0x000f, 0x0083, 0x0000}, //index: 20, gain:5.957400db->x1.985500, DCG:0
	{0x0010, 0x0080, 0x0000}, //index: 21, gain:6.051150db->x2.007047, DCG:0
	{0x0010, 0x0081, 0x0000}, //index: 22, gain:6.144900db->x2.028827, DCG:0
	{0x0010, 0x0083, 0x0000}, //index: 23, gain:6.238650db->x2.050843, DCG:0
	{0x0010, 0x0084, 0x0000}, //index: 24, gain:6.332400db->x2.073099, DCG:0
	{0x0010, 0x0086, 0x0000}, //index: 25, gain:6.426150db->x2.095596, DCG:0
	{0x0010, 0x0087, 0x0000}, //index: 26, gain:6.519900db->x2.118337, DCG:0
	{0x0011, 0x0080, 0x0000}, //index: 27, gain:6.613650db->x2.141325, DCG:0
	{0x0011, 0x0082, 0x0000}, //index: 28, gain:6.707400db->x2.164562, DCG:0
	{0x0011, 0x0083, 0x0000}, //index: 29, gain:6.801150db->x2.188051, DCG:0
	{0x0011, 0x0085, 0x0000}, //index: 30, gain:6.894900db->x2.211796, DCG:0
	{0x0011, 0x0086, 0x0000}, //index: 31, gain:6.988650db->x2.235798, DCG:0
	{0x0012, 0x0080, 0x0000}, //index: 32, gain:7.082400db->x2.260060, DCG:0
	{0x0012, 0x0081, 0x0000}, //index: 33, gain:7.176150db->x2.284586, DCG:0
	{0x0012, 0x0083, 0x0000}, //index: 34, gain:7.269900db->x2.309378, DCG:0
	{0x0012, 0x0084, 0x0000}, //index: 35, gain:7.363650db->x2.334439, DCG:0
	{0x0012, 0x0086, 0x0000}, //index: 36, gain:7.457400db->x2.359772, DCG:0
	{0x0013, 0x0080, 0x0000}, //index: 37, gain:7.551150db->x2.385380, DCG:0
	{0x0013, 0x0081, 0x0000}, //index: 38, gain:7.644900db->x2.411265, DCG:0
	{0x0013, 0x0083, 0x0000}, //index: 39, gain:7.738650db->x2.437432, DCG:0
	{0x0013, 0x0084, 0x0000}, //index: 40, gain:7.832400db->x2.463883, DCG:0
	{0x0013, 0x0086, 0x0000}, //index: 41, gain:7.926150db->x2.490620, DCG:0
	{0x0014, 0x0080, 0x0000}, //index: 42, gain:8.019900db->x2.517648, DCG:0
	{0x0014, 0x0082, 0x0000}, //index: 43, gain:8.113650db->x2.544969, DCG:0
	{0x0014, 0x0083, 0x0000}, //index: 44, gain:8.207400db->x2.572587, DCG:0
	{0x0014, 0x0085, 0x0000}, //index: 45, gain:8.301150db->x2.600504, DCG:0
	{0x0015, 0x0080, 0x0000}, //index: 46, gain:8.394900db->x2.628724, DCG:0
	{0x0015, 0x0081, 0x0000}, //index: 47, gain:8.488650db->x2.657251, DCG:0
	{0x0015, 0x0082, 0x0000}, //index: 48, gain:8.582400db->x2.686087, DCG:0
	{0x0000, 0x0080, 0x0004}, //index: 49, gain:8.676150db->x2.715235, DCG:1
	{0x0000, 0x0082, 0x0004}, //index: 50, gain:8.769900db->x2.744701, DCG:1
	{0x0000, 0x0083, 0x0004}, //index: 51, gain:8.863650db->x2.774486, DCG:1
	{0x0000, 0x0084, 0x0004}, //index: 52, gain:8.957400db->x2.804594, DCG:1
	{0x0000, 0x0086, 0x0004}, //index: 53, gain:9.051150db->x2.835029, DCG:1
	{0x0000, 0x0087, 0x0004}, //index: 54, gain:9.144900db->x2.865794, DCG:1
	{0x0001, 0x0081, 0x0004}, //index: 55, gain:9.238650db->x2.896893, DCG:1
	{0x0001, 0x0082, 0x0004}, //index: 56, gain:9.332400db->x2.928330, DCG:1
	{0x0001, 0x0084, 0x0004}, //index: 57, gain:9.426150db->x2.960108, DCG:1
	{0x0001, 0x0085, 0x0004}, //index: 58, gain:9.519900db->x2.992230, DCG:1
	{0x0001, 0x0086, 0x0004}, //index: 59, gain:9.613650db->x3.024701, DCG:1
	{0x0002, 0x0080, 0x0004}, //index: 60, gain:9.707400db->x3.057525, DCG:1
	{0x0002, 0x0082, 0x0004}, //index: 61, gain:9.801150db->x3.090705, DCG:1
	{0x0002, 0x0083, 0x0004}, //index: 62, gain:9.894900db->x3.124244, DCG:1
	{0x0002, 0x0085, 0x0004}, //index: 63, gain:9.988650db->x3.158148, DCG:1
	{0x0002, 0x0086, 0x0004}, //index: 64, gain:10.082400db->x3.192420, DCG:1
	{0x0003, 0x0080, 0x0004}, //index: 65, gain:10.176150db->x3.227063, DCG:1
	{0x0003, 0x0082, 0x0004}, //index: 66, gain:10.269900db->x3.262083, DCG:1
	{0x0003, 0x0083, 0x0004}, //index: 67, gain:10.363650db->x3.297483, DCG:1
	{0x0003, 0x0085, 0x0004}, //index: 68, gain:10.457400db->x3.333266, DCG:1
	{0x0003, 0x0086, 0x0004}, //index: 69, gain:10.551150db->x3.369438, DCG:1
	{0x0004, 0x0081, 0x0004}, //index: 70, gain:10.644900db->x3.406003, DCG:1
	{0x0004, 0x0082, 0x0004}, //index: 71, gain:10.738650db->x3.442964, DCG:1
	{0x0004, 0x0083, 0x0004}, //index: 72, gain:10.832400db->x3.480327, DCG:1
	{0x0004, 0x0085, 0x0004}, //index: 73, gain:10.926150db->x3.518094, DCG:1
	{0x0005, 0x0080, 0x0004}, //index: 74, gain:11.019900db->x3.556272, DCG:1
	{0x0005, 0x0081, 0x0004}, //index: 75, gain:11.113650db->x3.594864, DCG:1
	{0x0005, 0x0083, 0x0004}, //index: 76, gain:11.207400db->x3.633875, DCG:1
	{0x0005, 0x0084, 0x0004}, //index: 77, gain:11.301150db->x3.673309, DCG:1
	{0x0006, 0x0080, 0x0004}, //index: 78, gain:11.394900db->x3.713171, DCG:1
	{0x0006, 0x0081, 0x0004}, //index: 79, gain:11.488650db->x3.753466, DCG:1
	{0x0006, 0x0082, 0x0004}, //index: 80, gain:11.582400db->x3.794198, DCG:1
	{0x0006, 0x0084, 0x0004}, //index: 81, gain:11.676150db->x3.835372, DCG:1
	{0x0006, 0x0085, 0x0004}, //index: 82, gain:11.769900db->x3.876993, DCG:1
	{0x0007, 0x0081, 0x0004}, //index: 83, gain:11.863650db->x3.919065, DCG:1
	{0x0007, 0x0082, 0x0004}, //index: 84, gain:11.957400db->x3.961594, DCG:1
	{0x0007, 0x0084, 0x0004}, //index: 85, gain:12.051150db->x4.004585, DCG:1
	{0x0007, 0x0085, 0x0004}, //index: 86, gain:12.144900db->x4.048042, DCG:1
	{0x0008, 0x0081, 0x0004}, //index: 87, gain:12.238650db->x4.091971, DCG:1
	{0x0008, 0x0082, 0x0004}, //index: 88, gain:12.332400db->x4.136376, DCG:1
	{0x0008, 0x0084, 0x0004}, //index: 89, gain:12.426150db->x4.181263, DCG:1
	{0x0009, 0x0080, 0x0004}, //index: 90, gain:12.519900db->x4.226637, DCG:1
	{0x0009, 0x0081, 0x0004}, //index: 91, gain:12.613650db->x4.272504, DCG:1
	{0x0009, 0x0083, 0x0004}, //index: 92, gain:12.707400db->x4.318869, DCG:1
	{0x0009, 0x0084, 0x0004}, //index: 93, gain:12.801150db->x4.365736, DCG:1
	{0x000a, 0x0080, 0x0004}, //index: 94, gain:12.894900db->x4.413113, DCG:1
	{0x000a, 0x0082, 0x0004}, //index: 95, gain:12.988650db->x4.461003, DCG:1
	{0x000a, 0x0083, 0x0004}, //index: 96, gain:13.082400db->x4.509413, DCG:1
	{0x000b, 0x0080, 0x0004}, //index: 97, gain:13.176150db->x4.558348, DCG:1
	{0x000b, 0x0081, 0x0004}, //index: 98, gain:13.269900db->x4.607815, DCG:1
	{0x000b, 0x0082, 0x0004}, //index: 99, gain:13.363650db->x4.657818, DCG:1
	{0x000b, 0x0084, 0x0004}, //index: 100, gain:13.457400db->x4.708364, DCG:1
	{0x000c, 0x0080, 0x0004}, //index: 101, gain:13.551150db->x4.759458, DCG:1
	{0x000c, 0x0082, 0x0004}, //index: 102, gain:13.644900db->x4.811107, DCG:1
	{0x000c, 0x0083, 0x0004}, //index: 103, gain:13.738650db->x4.863316, DCG:1
	{0x000d, 0x0080, 0x0004}, //index: 104, gain:13.832400db->x4.916092, DCG:1
	{0x000d, 0x0081, 0x0004}, //index: 105, gain:13.926150db->x4.969441, DCG:1
	{0x000d, 0x0083, 0x0004}, //index: 106, gain:14.019900db->x5.023368, DCG:1
	{0x000e, 0x0080, 0x0004}, //index: 107, gain:14.113650db->x5.077881, DCG:1
	{0x000e, 0x0081, 0x0004}, //index: 108, gain:14.207400db->x5.132985, DCG:1
	{0x000e, 0x0083, 0x0004}, //index: 109, gain:14.301150db->x5.188687, DCG:1
	{0x000f, 0x0080, 0x0004}, //index: 110, gain:14.394900db->x5.244994, DCG:1
	{0x000f, 0x0081, 0x0004}, //index: 111, gain:14.488650db->x5.301912, DCG:1
	{0x000f, 0x0083, 0x0004}, //index: 112, gain:14.582400db->x5.359447, DCG:1
	{0x0010, 0x0080, 0x0004}, //index: 113, gain:14.676150db->x5.417607, DCG:1
	{0x0010, 0x0081, 0x0004}, //index: 114, gain:14.769900db->x5.476398, DCG:1
	{0x0010, 0x0083, 0x0004}, //index: 115, gain:14.863650db->x5.535827, DCG:1
	{0x0010, 0x0084, 0x0004}, //index: 116, gain:14.957400db->x5.595901, DCG:1
	{0x0010, 0x0086, 0x0004}, //index: 117, gain:15.051150db->x5.656626, DCG:1
	{0x0010, 0x0087, 0x0004}, //index: 118, gain:15.144900db->x5.718011, DCG:1
	{0x0011, 0x0080, 0x0004}, //index: 119, gain:15.238650db->x5.780062, DCG:1
	{0x0011, 0x0082, 0x0004}, //index: 120, gain:15.332400db->x5.842786, DCG:1
	{0x0011, 0x0083, 0x0004}, //index: 121, gain:15.426150db->x5.906191, DCG:1
	{0x0011, 0x0085, 0x0004}, //index: 122, gain:15.519900db->x5.970284, DCG:1
	{0x0011, 0x0086, 0x0004}, //index: 123, gain:15.613650db->x6.035073, DCG:1
	{0x0012, 0x0080, 0x0004}, //index: 124, gain:15.707400db->x6.100564, DCG:1
	{0x0012, 0x0081, 0x0004}, //index: 125, gain:15.801150db->x6.166766, DCG:1
	{0x0012, 0x0083, 0x0004}, //index: 126, gain:15.894900db->x6.233687, DCG:1
	{0x0012, 0x0084, 0x0004}, //index: 127, gain:15.988650db->x6.301334, DCG:1
	{0x0012, 0x0086, 0x0004}, //index: 128, gain:16.082400db->x6.369715, DCG:1
	{0x0013, 0x0080, 0x0004}, //index: 129, gain:16.176150db->x6.438838, DCG:1
	{0x0013, 0x0081, 0x0004}, //index: 130, gain:16.269900db->x6.508711, DCG:1
	{0x0013, 0x0083, 0x0004}, //index: 131, gain:16.363650db->x6.579343, DCG:1
	{0x0013, 0x0084, 0x0004}, //index: 132, gain:16.457400db->x6.650740, DCG:1
	{0x0013, 0x0086, 0x0004}, //index: 133, gain:16.551150db->x6.722913, DCG:1
	{0x0014, 0x0080, 0x0004}, //index: 134, gain:16.644900db->x6.795869, DCG:1
	{0x0014, 0x0082, 0x0004}, //index: 135, gain:16.738650db->x6.869617, DCG:1
	{0x0014, 0x0083, 0x0004}, //index: 136, gain:16.832400db->x6.944164, DCG:1
	{0x0014, 0x0085, 0x0004}, //index: 137, gain:16.926150db->x7.019521, DCG:1
	{0x0015, 0x0080, 0x0004}, //index: 138, gain:17.019900db->x7.095696, DCG:1
	{0x0015, 0x0081, 0x0004}, //index: 139, gain:17.113650db->x7.172697, DCG:1
	{0x0015, 0x0082, 0x0004}, //index: 140, gain:17.207400db->x7.250534, DCG:1
	{0x0015, 0x0084, 0x0004}, //index: 141, gain:17.301150db->x7.329216, DCG:1
	{0x0015, 0x0085, 0x0004}, //index: 142, gain:17.394900db->x7.408751, DCG:1
	{0x0016, 0x0081, 0x0004}, //index: 143, gain:17.488650db->x7.489149, DCG:1
	{0x0016, 0x0082, 0x0004}, //index: 144, gain:17.582400db->x7.570420, DCG:1
	{0x0016, 0x0083, 0x0004}, //index: 145, gain:17.676150db->x7.652573, DCG:1
	{0x0016, 0x0085, 0x0004}, //index: 146, gain:17.769900db->x7.735618, DCG:1
	{0x0017, 0x0080, 0x0004}, //index: 147, gain:17.863650db->x7.819563, DCG:1
	{0x0017, 0x0082, 0x0004}, //index: 148, gain:17.957400db->x7.904420, DCG:1
	{0x0017, 0x0083, 0x0004}, //index: 149, gain:18.051150db->x7.990197, DCG:1
	{0x0017, 0x0085, 0x0004}, //index: 150, gain:18.144900db->x8.076905, DCG:1
	{0x0018, 0x0081, 0x0004}, //index: 151, gain:18.238650db->x8.164555, DCG:1
	{0x0018, 0x0082, 0x0004}, //index: 152, gain:18.332400db->x8.253155, DCG:1
	{0x0018, 0x0083, 0x0004}, //index: 153, gain:18.426150db->x8.342717, DCG:1
	{0x0018, 0x0085, 0x0004}, //index: 154, gain:18.519900db->x8.433250, DCG:1
	{0x0019, 0x0081, 0x0004}, //index: 155, gain:18.613650db->x8.524767, DCG:1
	{0x0019, 0x0082, 0x0004}, //index: 156, gain:18.707400db->x8.617276, DCG:1
	{0x0019, 0x0084, 0x0004}, //index: 157, gain:18.801150db->x8.710789, DCG:1
	{0x001a, 0x0080, 0x0004}, //index: 158, gain:18.894900db->x8.805317, DCG:1
	{0x001a, 0x0081, 0x0004}, //index: 159, gain:18.988650db->x8.900871, DCG:1
	{0x001a, 0x0083, 0x0004}, //index: 160, gain:19.082400db->x8.997462, DCG:1
	{0x001a, 0x0084, 0x0004}, //index: 161, gain:19.176150db->x9.095100, DCG:1
	{0x001b, 0x0081, 0x0004}, //index: 162, gain:19.269900db->x9.193799, DCG:1
	{0x001b, 0x0082, 0x0004}, //index: 163, gain:19.363650db->x9.293568, DCG:1
	{0x001b, 0x0083, 0x0004}, //index: 164, gain:19.457400db->x9.394421, DCG:1
	{0x001c, 0x0080, 0x0004}, //index: 165, gain:19.551150db->x9.496367, DCG:1
	{0x001c, 0x0082, 0x0004}, //index: 166, gain:19.644900db->x9.599420, DCG:1
	{0x001c, 0x0083, 0x0004}, //index: 167, gain:19.738650db->x9.703591, DCG:1
	{0x001d, 0x0080, 0x0004}, //index: 168, gain:19.832400db->x9.808893, DCG:1
	{0x001d, 0x0081, 0x0004}, //index: 169, gain:19.926150db->x9.915337, DCG:1
	{0x001d, 0x0083, 0x0004}, //index: 170, gain:20.019900db->x10.022937, DCG:1
	{0x001e, 0x0080, 0x0004}, //index: 171, gain:20.113650db->x10.131704, DCG:1
	{0x001e, 0x0081, 0x0004}, //index: 172, gain:20.207400db->x10.241652, DCG:1
	{0x001e, 0x0082, 0x0004}, //index: 173, gain:20.301150db->x10.352792, DCG:1
	{0x001f, 0x0080, 0x0004}, //index: 174, gain:20.394900db->x10.465139, DCG:1
	{0x001f, 0x0081, 0x0004}, //index: 175, gain:20.488650db->x10.578705, DCG:1
	{0x001f, 0x0082, 0x0004}, //index: 176, gain:20.582400db->x10.693503, DCG:1
	{0x0020, 0x0080, 0x0004}, //index: 177, gain:20.676150db->x10.809547, DCG:1
	{0x0020, 0x0081, 0x0004}, //index: 178, gain:20.769900db->x10.926850, DCG:1
	{0x0020, 0x0082, 0x0004}, //index: 179, gain:20.863650db->x11.045427, DCG:1
	{0x0020, 0x0084, 0x0004}, //index: 180, gain:20.957400db->x11.165290, DCG:1
	{0x0020, 0x0085, 0x0004}, //index: 181, gain:21.051150db->x11.286454, DCG:1
	{0x0020, 0x0087, 0x0004}, //index: 182, gain:21.144900db->x11.408932, DCG:1
	{0x0021, 0x0080, 0x0004}, //index: 183, gain:21.238650db->x11.532740, DCG:1
	{0x0021, 0x0082, 0x0004}, //index: 184, gain:21.332400db->x11.657891, DCG:1
	{0x0021, 0x0083, 0x0004}, //index: 185, gain:21.426150db->x11.784401, DCG:1
	{0x0021, 0x0084, 0x0004}, //index: 186, gain:21.519900db->x11.912283, DCG:1
	{0x0021, 0x0086, 0x0004}, //index: 187, gain:21.613650db->x12.041553, DCG:1
	{0x0022, 0x0080, 0x0004}, //index: 188, gain:21.707400db->x12.172226, DCG:1
	{0x0022, 0x0081, 0x0004}, //index: 189, gain:21.801150db->x12.304317, DCG:1
	{0x0022, 0x0083, 0x0004}, //index: 190, gain:21.894900db->x12.437841, DCG:1
	{0x0022, 0x0084, 0x0004}, //index: 191, gain:21.988650db->x12.572814, DCG:1
	{0x0022, 0x0085, 0x0004}, //index: 192, gain:22.082400db->x12.709252, DCG:1
	{0x0023, 0x0080, 0x0004}, //index: 193, gain:22.176150db->x12.847171, DCG:1
	{0x0023, 0x0081, 0x0004}, //index: 194, gain:22.269900db->x12.986586, DCG:1
	{0x0023, 0x0083, 0x0004}, //index: 195, gain:22.363650db->x13.127514, DCG:1
	{0x0023, 0x0084, 0x0004}, //index: 196, gain:22.457400db->x13.269972, DCG:1
	{0x0023, 0x0085, 0x0004}, //index: 197, gain:22.551150db->x13.413975, DCG:1
	{0x0024, 0x0080, 0x0004}, //index: 198, gain:22.644900db->x13.559541, DCG:1
	{0x0024, 0x0081, 0x0004}, //index: 199, gain:22.738650db->x13.706687, DCG:1
	{0x0024, 0x0083, 0x0004}, //index: 200, gain:22.832400db->x13.855430, DCG:1
	{0x0024, 0x0084, 0x0004}, //index: 201, gain:22.926150db->x14.005786, DCG:1
	{0x0024, 0x0086, 0x0004}, //index: 202, gain:23.019900db->x14.157775, DCG:1
	{0x0025, 0x0081, 0x0004}, //index: 203, gain:23.113650db->x14.311413, DCG:1
	{0x0025, 0x0082, 0x0004}, //index: 204, gain:23.207400db->x14.466717, DCG:1
	{0x0025, 0x0084, 0x0004}, //index: 205, gain:23.301150db->x14.623708, DCG:1
	{0x0025, 0x0085, 0x0004}, //index: 206, gain:23.394900db->x14.782402, DCG:1
	{0x0026, 0x0080, 0x0004}, //index: 207, gain:23.488650db->x14.942818, DCG:1
	{0x0026, 0x0082, 0x0004}, //index: 208, gain:23.582400db->x15.104975, DCG:1
	{0x0026, 0x0083, 0x0004}, //index: 209, gain:23.676150db->x15.268891, DCG:1
	{0x0026, 0x0085, 0x0004}, //index: 210, gain:23.769900db->x15.434587, DCG:1
	{0x0027, 0x0080, 0x0004}, //index: 211, gain:23.863650db->x15.602080, DCG:1
	{0x0027, 0x0082, 0x0004}, //index: 212, gain:23.957400db->x15.771391, DCG:1
	{0x0027, 0x0083, 0x0004}, //index: 213, gain:24.051150db->x15.942539, DCG:1
	{0x0027, 0x0084, 0x0004}, //index: 214, gain:24.144900db->x16.115545, DCG:1
	{0x0028, 0x0080, 0x0004}, //index: 215, gain:24.238650db->x16.290428, DCG:1
	{0x0028, 0x0082, 0x0004}, //index: 216, gain:24.332400db->x16.467209, DCG:1
	{0x0028, 0x0083, 0x0004}, //index: 217, gain:24.426150db->x16.645908, DCG:1
	{0x0028, 0x0084, 0x0004}, //index: 218, gain:24.519900db->x16.826547, DCG:1
	{0x0029, 0x0081, 0x0004}, //index: 219, gain:24.613650db->x17.009146, DCG:1
	{0x0029, 0x0082, 0x0004}, //index: 220, gain:24.707400db->x17.193726, DCG:1
	{0x0029, 0x0083, 0x0004}, //index: 221, gain:24.801150db->x17.380309, DCG:1
	{0x002a, 0x0080, 0x0004}, //index: 222, gain:24.894900db->x17.568917, DCG:1
	{0x002a, 0x0081, 0x0004}, //index: 223, gain:24.988650db->x17.759572, DCG:1
	{0x002a, 0x0082, 0x0004}, //index: 224, gain:25.082400db->x17.952296, DCG:1
	{0x002a, 0x0084, 0x0004}, //index: 225, gain:25.176150db->x18.147111, DCG:1
	{0x002b, 0x0080, 0x0004}, //index: 226, gain:25.269900db->x18.344040, DCG:1
	{0x002b, 0x0082, 0x0004}, //index: 227, gain:25.363650db->x18.543107, DCG:1
	{0x002b, 0x0083, 0x0004}, //index: 228, gain:25.457400db->x18.744333, DCG:1
	{0x002c, 0x0080, 0x0004}, //index: 229, gain:25.551150db->x18.947744, DCG:1
	{0x002c, 0x0081, 0x0004}, //index: 230, gain:25.644900db->x19.153361, DCG:1
	{0x002c, 0x0083, 0x0004}, //index: 231, gain:25.738650db->x19.361210, DCG:1
	{0x002c, 0x0084, 0x0004}, //index: 232, gain:25.832400db->x19.571315, DCG:1
	{0x002d, 0x0081, 0x0004}, //index: 233, gain:25.926150db->x19.783699, DCG:1
	{0x002d, 0x0082, 0x0004}, //index: 234, gain:26.019900db->x19.998388, DCG:1
	{0x002d, 0x0084, 0x0004}, //index: 235, gain:26.113650db->x20.215407, DCG:1
	{0x002e, 0x0081, 0x0004}, //index: 236, gain:26.207400db->x20.434782, DCG:1
	{0x002e, 0x0082, 0x0004}, //index: 237, gain:26.301150db->x20.656536, DCG:1
	{0x002e, 0x0083, 0x0004}, //index: 238, gain:26.394900db->x20.880697, DCG:1
	{0x002f, 0x0081, 0x0004}, //index: 239, gain:26.488650db->x21.107291, DCG:1
	{0x002f, 0x0082, 0x0004}, //index: 240, gain:26.582400db->x21.336344, DCG:1
	{0x002f, 0x0083, 0x0004}, //index: 241, gain:26.676150db->x21.567882, DCG:1
	{0x0030, 0x0081, 0x0004}, //index: 242, gain:26.769900db->x21.801933, DCG:1
	{0x0030, 0x0082, 0x0004}, //index: 243, gain:26.863650db->x22.038524, DCG:1
	{0x0030, 0x0084, 0x0004}, //index: 244, gain:26.957400db->x22.277682, DCG:1
	{0x0030, 0x0085, 0x0004}, //index: 245, gain:27.051150db->x22.519436, DCG:1
	{0x0030, 0x0086, 0x0004}, //index: 246, gain:27.144900db->x22.763813, DCG:1
	{0x0031, 0x0080, 0x0004}, //index: 247, gain:27.238650db->x23.010841, DCG:1
	{0x0031, 0x0081, 0x0004}, //index: 248, gain:27.332400db->x23.260551, DCG:1
	{0x0031, 0x0083, 0x0004}, //index: 249, gain:27.426150db->x23.512971, DCG:1
	{0x0031, 0x0084, 0x0004}, //index: 250, gain:27.519900db->x23.768129, DCG:1
	{0x0031, 0x0086, 0x0004}, //index: 251, gain:27.613650db->x24.026057, DCG:1
	{0x0031, 0x0087, 0x0004}, //index: 252, gain:27.707400db->x24.286783, DCG:1
	{0x0032, 0x0081, 0x0004}, //index: 253, gain:27.801150db->x24.550339, DCG:1
	{0x0032, 0x0082, 0x0004}, //index: 254, gain:27.894900db->x24.816755, DCG:1
	{0x0032, 0x0084, 0x0004}, //index: 255, gain:27.988650db->x25.086063, DCG:1
	{0x0032, 0x0085, 0x0004}, //index: 256, gain:28.082400db->x25.358292, DCG:1
	{0x0032, 0x0087, 0x0004}, //index: 257, gain:28.176150db->x25.633476, DCG:1
	{0x0033, 0x0081, 0x0004}, //index: 258, gain:28.269900db->x25.911646, DCG:1
	{0x0033, 0x0082, 0x0004}, //index: 259, gain:28.363650db->x26.192835, DCG:1
	{0x0033, 0x0084, 0x0004}, //index: 260, gain:28.457400db->x26.477075, DCG:1
	{0x0033, 0x0085, 0x0004}, //index: 261, gain:28.551150db->x26.764399, DCG:1
	{0x0034, 0x0080, 0x0004}, //index: 262, gain:28.644900db->x27.054842, DCG:1
	{0x0034, 0x0081, 0x0004}, //index: 263, gain:28.738650db->x27.348436, DCG:1
	{0x0034, 0x0083, 0x0004}, //index: 264, gain:28.832400db->x27.645217, DCG:1
	{0x0034, 0x0084, 0x0004}, //index: 265, gain:28.926150db->x27.945218, DCG:1
	{0x0034, 0x0085, 0x0004}, //index: 266, gain:29.019900db->x28.248475, DCG:1
	{0x0035, 0x0080, 0x0004}, //index: 267, gain:29.113650db->x28.555022, DCG:1
	{0x0035, 0x0082, 0x0004}, //index: 268, gain:29.207400db->x28.864896, DCG:1
	{0x0035, 0x0083, 0x0004}, //index: 269, gain:29.301150db->x29.178133, DCG:1
	{0x0035, 0x0085, 0x0004}, //index: 270, gain:29.394900db->x29.494769, DCG:1
	{0x0036, 0x0080, 0x0004}, //index: 271, gain:29.488650db->x29.814841, DCG:1
	{0x0036, 0x0081, 0x0004}, //index: 272, gain:29.582400db->x30.138387, DCG:1
	{0x0036, 0x0083, 0x0004}, //index: 273, gain:29.676150db->x30.465443, DCG:1
	{0x0036, 0x0084, 0x0004}, //index: 274, gain:29.769900db->x30.796049, DCG:1
	{0x0037, 0x0080, 0x0004}, //index: 275, gain:29.863650db->x31.130242, DCG:1
	{0x0037, 0x0081, 0x0004}, //index: 276, gain:29.957400db->x31.468062, DCG:1
	{0x0037, 0x0083, 0x0004}, //index: 277, gain:30.051150db->x31.809548, DCG:1
	{0x0037, 0x0084, 0x0004}, //index: 278, gain:30.144900db->x32.154740, DCG:1
	{0x0038, 0x0080, 0x0004}, //index: 279, gain:30.238650db->x32.503677, DCG:1
	{0x0038, 0x0081, 0x0004}, //index: 280, gain:30.332400db->x32.856402, DCG:1
	{0x0038, 0x0083, 0x0004}, //index: 281, gain:30.426150db->x33.212954, DCG:1
	{0x0038, 0x0084, 0x0004}, //index: 282, gain:30.519900db->x33.573375, DCG:1
	{0x0038, 0x0086, 0x0004}, //index: 283, gain:30.613650db->x33.937707, DCG:1
	{0x0038, 0x0087, 0x0004}, //index: 284, gain:30.707400db->x34.305993, DCG:1
	{0x0038, 0x0089, 0x0004}, //index: 285, gain:30.801150db->x34.678276, DCG:1
	{0x0038, 0x008a, 0x0004}, //index: 286, gain:30.894900db->x35.054599, DCG:1
	{0x0038, 0x008b, 0x0004}, //index: 287, gain:30.988650db->x35.435005, DCG:1
	{0x0038, 0x008d, 0x0004}, //index: 288, gain:31.082400db->x35.819540, DCG:1
	{0x0038, 0x008f, 0x0004}, //index: 289, gain:31.176150db->x36.208247, DCG:1
	{0x0038, 0x0090, 0x0004}, //index: 290, gain:31.269900db->x36.601173, DCG:1
	{0x0038, 0x0092, 0x0004}, //index: 291, gain:31.363650db->x36.998362, DCG:1
	{0x0038, 0x0093, 0x0004}, //index: 292, gain:31.457400db->x37.399862, DCG:1
	{0x0038, 0x0095, 0x0004}, //index: 293, gain:31.551150db->x37.805719, DCG:1
	{0x0038, 0x0096, 0x0004}, //index: 294, gain:31.644900db->x38.215980, DCG:1
	{0x0038, 0x0098, 0x0004}, //index: 295, gain:31.738650db->x38.630693, DCG:1
	{0x0038, 0x009a, 0x0004}, //index: 296, gain:31.832400db->x39.049907, DCG:1
	{0x0038, 0x009b, 0x0004}, //index: 297, gain:31.926150db->x39.473669, DCG:1
	{0x0038, 0x009d, 0x0004}, //index: 298, gain:32.019900db->x39.902031, DCG:1
	{0x0038, 0x009f, 0x0004}, //index: 299, gain:32.113650db->x40.335041, DCG:1
	{0x0038, 0x00a1, 0x0004}, //index: 300, gain:32.207400db->x40.772750, DCG:1
	{0x0038, 0x00a2, 0x0004}, //index: 301, gain:32.301150db->x41.215208, DCG:1
	{0x0038, 0x00a4, 0x0004}, //index: 302, gain:32.394900db->x41.662469, DCG:1
	{0x0038, 0x00a6, 0x0004}, //index: 303, gain:32.488650db->x42.114583, DCG:1
	{0x0038, 0x00a8, 0x0004}, //index: 304, gain:32.582400db->x42.571603, DCG:1
	{0x0038, 0x00aa, 0x0004}, //index: 305, gain:32.676150db->x43.033582, DCG:1
	{0x0038, 0x00ab, 0x0004}, //index: 306, gain:32.769900db->x43.500575, DCG:1
	{0x0038, 0x00ad, 0x0004}, //index: 307, gain:32.863650db->x43.972636, DCG:1
	{0x0038, 0x00af, 0x0004}, //index: 308, gain:32.957400db->x44.449819, DCG:1
	{0x0038, 0x00b1, 0x0004}, //index: 309, gain:33.051150db->x44.932181, DCG:1
	{0x0038, 0x00b3, 0x0004}, //index: 310, gain:33.144900db->x45.419777, DCG:1
	{0x0038, 0x00b5, 0x0004}, //index: 311, gain:33.238650db->x45.912665, DCG:1
	{0x0038, 0x00b7, 0x0004}, //index: 312, gain:33.332400db->x46.410901, DCG:1
	{0x0038, 0x00b9, 0x0004}, //index: 313, gain:33.426150db->x46.914544, DCG:1
	{0x0038, 0x00bb, 0x0004}, //index: 314, gain:33.519900db->x47.423653, DCG:1
	{0x0038, 0x00bd, 0x0004}, //index: 315, gain:33.613650db->x47.938286, DCG:1
	{0x0038, 0x00bf, 0x0004}, //index: 316, gain:33.707400db->x48.458504, DCG:1
	{0x0038, 0x00c1, 0x0004}, //index: 317, gain:33.801150db->x48.984367, DCG:1
	{0x0038, 0x00c3, 0x0004}, //index: 318, gain:33.894900db->x49.515937, DCG:1
	{0x0038, 0x00c5, 0x0004}, //index: 319, gain:33.988650db->x50.053275, DCG:1
	{0x0038, 0x00c7, 0x0004}, //index: 320, gain:34.082400db->x50.596445, DCG:1
	{0x0038, 0x00ca, 0x0004}, //index: 321, gain:34.176150db->x51.145508, DCG:1
	{0x0038, 0x00cc, 0x0004}, //index: 322, gain:34.269900db->x51.700531, DCG:1
	{0x0038, 0x00ce, 0x0004}, //index: 323, gain:34.363650db->x52.261576, DCG:1
	{0x0038, 0x00d0, 0x0004}, //index: 324, gain:34.457400db->x52.828709, DCG:1
	{0x0038, 0x00d2, 0x0004}, //index: 325, gain:34.551150db->x53.401997, DCG:1
	{0x0038, 0x00d5, 0x0004}, //index: 326, gain:34.644900db->x53.981506, DCG:1
	{0x0038, 0x00d7, 0x0004}, //index: 327, gain:34.738650db->x54.567304, DCG:1
	{0x0038, 0x00d9, 0x0004}, //index: 328, gain:34.832400db->x55.159459, DCG:1
	{0x0038, 0x00dc, 0x0004}, //index: 329, gain:34.926150db->x55.758040, DCG:1
	{0x0038, 0x00de, 0x0004}, //index: 330, gain:35.019900db->x56.363117, DCG:1
	{0x0038, 0x00e1, 0x0004}, //index: 331, gain:35.113650db->x56.974759, DCG:1
	{0x0038, 0x00e3, 0x0004}, //index: 332, gain:35.207400db->x57.593040, DCG:1
	{0x0038, 0x00e5, 0x0004}, //index: 333, gain:35.301150db->x58.218029, DCG:1
	{0x0038, 0x00e8, 0x0004}, //index: 334, gain:35.394900db->x58.849801, DCG:1
	{0x0038, 0x00eb, 0x0004}, //index: 335, gain:35.488650db->x59.488429, DCG:1
	{0x0038, 0x00ed, 0x0004}, //index: 336, gain:35.582400db->x60.133987, DCG:1
	{0x0038, 0x00f0, 0x0004}, //index: 337, gain:35.676150db->x60.786551, DCG:1
	{0x0038, 0x00f2, 0x0004}, //index: 338, gain:35.769900db->x61.446196, DCG:1
	{0x0038, 0x00f5, 0x0004}, //index: 339, gain:35.863650db->x62.112999, DCG:1
	{0x0038, 0x00f8, 0x0004}, //index: 340, gain:35.957400db->x62.787039, DCG:1
	{0x0038, 0x00fa, 0x0004}, //index: 341, gain:36.051150db->x63.468393, DCG:1
	{0x0038, 0x00fd, 0x0004}, //index: 342, gain:36.144900db->x64.157141, DCG:1
	{0x0038, 0x0100, 0x0004}, //index: 343, gain:36.238650db->x64.853363, DCG:1
	{0x0038, 0x0102, 0x0004}, //index: 344, gain:36.332400db->x65.557140, DCG:1
	{0x0038, 0x0105, 0x0004}, //index: 345, gain:36.426150db->x66.268555, DCG:1
	{0x0038, 0x0108, 0x0004}, //index: 346, gain:36.519900db->x66.987690, DCG:1
	{0x0038, 0x010b, 0x0004}, //index: 347, gain:36.613650db->x67.714628, DCG:1
	{0x0038, 0x010e, 0x0004}, //index: 348, gain:36.707400db->x68.449456, DCG:1
	{0x0038, 0x0111, 0x0004}, //index: 349, gain:36.801150db->x69.192257, DCG:1
	{0x0038, 0x0114, 0x0004}, //index: 350, gain:36.894900db->x69.943120, DCG:1
	{0x0038, 0x0117, 0x0004}, //index: 351, gain:36.988650db->x70.702130, DCG:1
	{0x0038, 0x011a, 0x0004}, //index: 352, gain:37.082400db->x71.469378, DCG:1
	{0x0038, 0x011d, 0x0004}, //index: 353, gain:37.176150db->x72.244951, DCG:1
	{0x0038, 0x0120, 0x0004}, //index: 354, gain:37.269900db->x73.028941, DCG:1
	{0x0038, 0x0123, 0x0004}, //index: 355, gain:37.363650db->x73.821438, DCG:1
	{0x0038, 0x0126, 0x0004}, //index: 356, gain:37.457400db->x74.622535, DCG:1
	{0x0038, 0x012a, 0x0004}, //index: 357, gain:37.551150db->x75.432326, DCG:1
	{0x0038, 0x012d, 0x0004}, //index: 358, gain:37.644900db->x76.250905, DCG:1
	{0x0038, 0x0130, 0x0004}, //index: 359, gain:37.738650db->x77.078366, DCG:1
	{0x0038, 0x0133, 0x0004}, //index: 360, gain:37.832400db->x77.914807, DCG:1
	{0x0038, 0x0137, 0x0004}, //index: 361, gain:37.926150db->x78.760325, DCG:1
	{0x0038, 0x013a, 0x0004}, //index: 362, gain:38.019900db->x79.615018, DCG:1
	{0x0038, 0x013d, 0x0004}, //index: 363, gain:38.113650db->x80.478987, DCG:1
	{0x0038, 0x0141, 0x0004}, //index: 364, gain:38.207400db->x81.352331, DCG:1
	{0x0038, 0x0144, 0x0004}, //index: 365, gain:38.301150db->x82.235152, DCG:1
	{0x0038, 0x0148, 0x0004}, //index: 366, gain:38.394900db->x83.127554, DCG:1
	{0x0038, 0x014b, 0x0004}, //index: 367, gain:38.488650db->x84.029639, DCG:1
	{0x0038, 0x014f, 0x0004}, //index: 368, gain:38.582400db->x84.941514, DCG:1
	{0x0038, 0x0153, 0x0004}, //index: 369, gain:38.676150db->x85.863285, DCG:1
	{0x0038, 0x0156, 0x0004}, //index: 370, gain:38.769900db->x86.795058, DCG:1
	{0x0038, 0x015a, 0x0004}, //index: 371, gain:38.863650db->x87.736943, DCG:1
	{0x0038, 0x015e, 0x0004}, //index: 372, gain:38.957400db->x88.689049, DCG:1
	{0x0038, 0x0162, 0x0004}, //index: 373, gain:39.051150db->x89.651488, DCG:1
	{0x0038, 0x0166, 0x0004}, //index: 374, gain:39.144900db->x90.624370, DCG:1
	{0x0038, 0x0169, 0x0004}, //index: 375, gain:39.238650db->x91.607810, DCG:1
	{0x0038, 0x016d, 0x0004}, //index: 376, gain:39.332400db->x92.601922, DCG:1
	{0x0038, 0x0171, 0x0004}, //index: 377, gain:39.426150db->x93.606822, DCG:1
	{0x0038, 0x0175, 0x0004}, //index: 378, gain:39.519900db->x94.622627, DCG:1
	{0x0038, 0x0179, 0x0004}, //index: 379, gain:39.613650db->x95.649455, DCG:1
	{0x0038, 0x017d, 0x0004}, //index: 380, gain:39.707400db->x96.687426, DCG:1
	{0x0038, 0x0182, 0x0004}, //index: 381, gain:39.801150db->x97.736661, DCG:1
	{0x0038, 0x0186, 0x0004}, //index: 382, gain:39.894900db->x98.797283, DCG:1
	{0x0038, 0x018a, 0x0004}, //index: 383, gain:39.988650db->x99.869414, DCG:1
	{0x0038, 0x018e, 0x0004}, //index: 384, gain:40.082400db->x100.953179, DCG:1
	{0x0038, 0x0193, 0x0004}, //index: 385, gain:40.176150db->x102.048705, DCG:1
	{0x0038, 0x0197, 0x0004}, //index: 386, gain:40.269900db->x103.156120, DCG:1
	{0x0038, 0x019b, 0x0004}, //index: 387, gain:40.363650db->x104.275553, DCG:1
	{0x0038, 0x01a0, 0x0004}, //index: 388, gain:40.457400db->x105.407133, DCG:1
	{0x0038, 0x01a4, 0x0004}, //index: 389, gain:40.551150db->x106.550993, DCG:1
	{0x0038, 0x01a9, 0x0004}, //index: 390, gain:40.644900db->x107.707265, DCG:1
	{0x0038, 0x01ae, 0x0004}, //index: 391, gain:40.738650db->x108.876086, DCG:1
	{0x0038, 0x01b2, 0x0004}, //index: 392, gain:40.832400db->x110.057590, DCG:1
	{0x0038, 0x01b7, 0x0004}, //index: 393, gain:40.926150db->x111.251916, DCG:1
	{0x0038, 0x01bc, 0x0004}, //index: 394, gain:41.019900db->x112.459203, DCG:1
	{0x0038, 0x01c1, 0x0004}, //index: 395, gain:41.113650db->x113.679590, DCG:1
	{0x0038, 0x01c5, 0x0004}, //index: 396, gain:41.207400db->x114.913222, DCG:1
	{0x0038, 0x01ca, 0x0004}, //index: 397, gain:41.301150db->x116.160240, DCG:1
	{0x0038, 0x01cf, 0x0004}, //index: 398, gain:41.394900db->x117.420791, DCG:1
	{0x0038, 0x01d4, 0x0004}, //index: 399, gain:41.488650db->x118.695021, DCG:1
	{0x0038, 0x01da, 0x0004}, //index: 400, gain:41.582400db->x119.983078, DCG:1
	{0x0038, 0x01df, 0x0004}, //index: 401, gain:41.676150db->x121.285114, DCG:1
	{0x0038, 0x01e4, 0x0004}, //index: 402, gain:41.769900db->x122.601279, DCG:1
	{0x0038, 0x01e9, 0x0004}, //index: 403, gain:41.863650db->x123.931727, DCG:1
	{0x0038, 0x01ee, 0x0004}, //index: 404, gain:41.957400db->x125.276612, DCG:1
	{0x0038, 0x01f4, 0x0004}, //index: 405, gain:42.051150db->x126.636092, DCG:1
	{0x0038, 0x01f9, 0x0004}, //index: 406, gain:42.144900db->x128.010325, DCG:1
	{0x0038, 0x01ff, 0x0004}, //index: 407, gain:42.238650db->x129.399471, DCG:1
	{0x0038, 0x0204, 0x0004}, //index: 408, gain:42.332400db->x130.803691, DCG:1
	{0x0038, 0x020a, 0x0004}, //index: 409, gain:42.426150db->x132.223150, DCG:1
	{0x0038, 0x0210, 0x0004}, //index: 410, gain:42.519900db->x133.658013, DCG:1
	{0x0038, 0x0215, 0x0004}, //index: 411, gain:42.613650db->x135.108446, DCG:1
	{0x0038, 0x021b, 0x0004}, //index: 412, gain:42.707400db->x136.574620, DCG:1
	{0x0038, 0x0221, 0x0004}, //index: 413, gain:42.801150db->x138.056704, DCG:1
	{0x0038, 0x0227, 0x0004}, //index: 414, gain:42.894900db->x139.554871, DCG:1
	{0x0038, 0x022d, 0x0004}, //index: 415, gain:42.988650db->x141.069296, DCG:1
	{0x0038, 0x0233, 0x0004}, //index: 416, gain:43.082400db->x142.600156, DCG:1
	{0x0038, 0x0239, 0x0004}, //index: 417, gain:43.176150db->x144.147628, DCG:1
	{0x0038, 0x023f, 0x0004}, //index: 418, gain:43.269900db->x145.711893, DCG:1
	{0x0038, 0x0245, 0x0004}, //index: 419, gain:43.363650db->x147.293133, DCG:1
	{0x0038, 0x024c, 0x0004}, //index: 420, gain:43.457400db->x148.891532, DCG:1
	{0x0038, 0x0252, 0x0004}, //index: 421, gain:43.551150db->x150.507278, DCG:1
	{0x0038, 0x0259, 0x0004}, //index: 422, gain:43.644900db->x152.140556, DCG:1
	{0x0038, 0x025f, 0x0004}, //index: 423, gain:43.738650db->x153.791559, DCG:1
	{0x0038, 0x0266, 0x0004}, //index: 424, gain:43.832400db->x155.460478, DCG:1
	{0x0038, 0x026c, 0x0004}, //index: 425, gain:43.926150db->x157.147509, DCG:1
	{0x0038, 0x0273, 0x0004}, //index: 426, gain:44.019900db->x158.852846, DCG:1
	{0x0038, 0x027a, 0x0004}, //index: 427, gain:44.113650db->x160.576689, DCG:1
	{0x0038, 0x0281, 0x0004}, //index: 428, gain:44.207400db->x162.319240, DCG:1
	{0x0038, 0x0288, 0x0004}, //index: 429, gain:44.301150db->x164.080700, DCG:1
	{0x0038, 0x028f, 0x0004}, //index: 430, gain:44.394900db->x165.861275, DCG:1
	{0x0038, 0x0296, 0x0004}, //index: 431, gain:44.488650db->x167.661173, DCG:1
	{0x0038, 0x029d, 0x0004}, //index: 432, gain:44.582400db->x169.480603, DCG:1
	{0x0038, 0x02a4, 0x0004}, //index: 433, gain:44.676150db->x171.319777, DCG:1
	{0x0038, 0x02ac, 0x0004}, //index: 434, gain:44.769900db->x173.178909, DCG:1
	{0x0038, 0x02b3, 0x0004}, //index: 435, gain:44.863650db->x175.058217, DCG:1
	{0x0038, 0x02bb, 0x0004}, //index: 436, gain:44.957400db->x176.957918, DCG:1
	{0x0038, 0x02c2, 0x0004}, //index: 437, gain:45.051150db->x178.878235, DCG:1
	{0x0038, 0x02ca, 0x0004}, //index: 438, gain:45.144900db->x180.819390, DCG:1
	{0x0038, 0x02d2, 0x0004}, //index: 439, gain:45.238650db->x182.781611, DCG:1
	{0x0038, 0x02d9, 0x0004}, //index: 440, gain:45.332400db->x184.765125, DCG:1
	{0x0038, 0x02e1, 0x0004}, //index: 441, gain:45.426150db->x186.770164, DCG:1
	{0x0038, 0x02e9, 0x0004}, //index: 442, gain:45.519900db->x188.796961, DCG:1
	{0x0038, 0x02f1, 0x0004}, //index: 443, gain:45.613650db->x190.845753, DCG:1
	{0x0038, 0x02fa, 0x0004}, //index: 444, gain:45.707400db->x192.916778, DCG:1
	{0x0038, 0x0302, 0x0004}, //index: 445, gain:45.801150db->x195.010277, DCG:1
	{0x0038, 0x030a, 0x0004}, //index: 446, gain:45.894900db->x197.126495, DCG:1
	{0x0038, 0x0313, 0x0004}, //index: 447, gain:45.988650db->x199.265677, DCG:1
	{0x0038, 0x031b, 0x0004}, //index: 448, gain:46.082400db->x201.428074, DCG:1
	{0x0038, 0x0324, 0x0004}, //index: 449, gain:46.176150db->x203.613936, DCG:1
	{0x0038, 0x032d, 0x0004}, //index: 450, gain:46.269900db->x205.823519, DCG:1
	{0x0038, 0x0335, 0x0004}, //index: 451, gain:46.363650db->x208.057080, DCG:1
	{0x0038, 0x033e, 0x0004}, //index: 452, gain:46.457400db->x210.314880, DCG:1
	{0x0038, 0x0347, 0x0004}, //index: 453, gain:46.551150db->x212.597180, DCG:1
	{0x0038, 0x0351, 0x0004}, //index: 454, gain:46.644900db->x214.904248, DCG:1
	{0x0038, 0x035a, 0x0004}, //index: 455, gain:46.738650db->x217.236351, DCG:1
	{0x0038, 0x0363, 0x0004}, //index: 456, gain:46.832400db->x219.593763, DCG:1
	{0x0038, 0x036c, 0x0004}, //index: 457, gain:46.926150db->x221.976756, DCG:1
	{0x0038, 0x0376, 0x0004}, //index: 458, gain:47.019900db->x224.385609, DCG:1
	{0x0038, 0x0380, 0x0004}, //index: 459, gain:47.113650db->x226.820603, DCG:1
	{0x0038, 0x0389, 0x0004}, //index: 460, gain:47.207400db->x229.282020, DCG:1
	{0x0038, 0x0393, 0x0004}, //index: 461, gain:47.301150db->x231.770149, DCG:1
	{0x0038, 0x039d, 0x0004}, //index: 462, gain:47.394900db->x234.285278, DCG:1
	{0x0038, 0x03a7, 0x0004}, //index: 463, gain:47.488650db->x236.827701, DCG:1
	{0x0038, 0x03b1, 0x0004}, //index: 464, gain:47.582400db->x239.397715, DCG:1
	{0x0038, 0x03bc, 0x0004}, //index: 465, gain:47.676150db->x241.995617, DCG:1
	{0x0038, 0x03c6, 0x0004}, //index: 466, gain:47.769900db->x244.621711, DCG:1
	{0x0038, 0x03d0, 0x0004}, //index: 467, gain:47.863650db->x247.276304, DCG:1
	{0x0038, 0x03db, 0x0004}, //index: 468, gain:47.957400db->x249.959703, DCG:1
};

