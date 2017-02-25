/*
 * Filename : ar0331_reg_tbl.c
 *
 * History:
 *    2015/02/06 - [Hao Zeng] Create
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

static struct vin_reg_16_16 ar0331_pll_regs[][6] = {
	{
		{0x302A, 0x0006}, /* VT_PIX_CLK_DIV  */
		{0x302C, 0x0001}, /* VT_SYS_CLK_DIV  */
		{0x302E, 0x0002}, /* PRE_PLL_CLK_DIV */
		{0x3030, 0x002C}, /* PLL_MULTIPLIER  */
		{0x3036, 0x000C}, /* OP_PIX_CLK_DIV  */
		{0x3038, 0x0001}, /* OP_SYS_CLK_DIV  */
	},
};

static struct vin_video_pll ar0331_plls[] = {
	{0, 20250000, 74250000},
};

static struct vin_reg_16_16 ar0331_mode_regs[][12] = {
	{	 /* 2048x1536 */
		{0x3002, 0x0004}, /* Y_ADDR_START */
		{0x3004, 0x0006}, /* X_ADDR_START */
		{0x3006, 1539}, /* Y_ADDR_END */
		{0x3008, 2053}, /* X_ADDR_END */
		{0x300A, 1657}, /* FRAME_LENGTH_LINES */
		{0x300C, 1120}, /* LINE_LENGTH_PCK */
		{0x3012, 0x031B}, /* COARSE_INTEGRATION_TIME */
		{0x3014, 0x0000}, /* FINE_INTEGRATION_TIME */
		{0x30A2, 0x0001}, /* X_ODD_INC */
		{0x30A6, 0x0001}, /* Y_ODD_INC */
		{0x3040, 0x0000}, /* READ MODE */
		{0x31AE, 0x0304}, /* SERIAL_FORMAT */
	},
	{ 	/* 1920x1080 */
		{0x3002, 228}, /* Y_ADDR_START */
		{0x3004, 66}, /* X_ADDR_START */
		{0x3006, 1307}, /* Y_ADDR_END */
		{0x3008, 1993}, /* X_ADDR_END */
		{0x300A, 1125}, /* FRAME_LENGTH_LINES */
		{0x300C, 2200}, /* LINE_LENGTH_PCK */
		{0x3012, 0x0464}, /* COARSE_INTEGRATION_TIME */
		{0x3014, 0x0000}, /* FINE_INTEGRATION_TIME */
		{0x30A2, 0x0001}, /* X_ODD_INC */
		{0x30A6, 0x0001}, /* Y_ODD_INC */
		{0x3040, 0x0000}, /* READ MODE */
		{0x31AE, 0x0304}, /* SERIAL_FORMAT */
	},
	{ 	/* 1280x720 */
		{0x3002, 412}, /* Y_ADDR_START */
		{0x3004, 390}, /* X_ADDR_START */
		{0x3006, 1131}, /* Y_ADDR_END */
		{0x3008, 1669}, /* X_ADDR_END */
		{0x300A, 757}, /* FRAME_LENGTH_LINES */
		{0x300C, 1634}, /* LINE_LENGTH_PCK */
		{0x3012, 0x02C6}, /* COARSE_INTEGRATION_TIME */
		{0x3014, 0x0000}, /* FINE_INTEGRATION_TIME */
		{0x30A2, 0x0001}, /* X_ODD_INC */
		{0x30A6, 0x0001}, /* Y_ODD_INC */
		{0x3040, 0x0000}, /* READ MODE */
		{0x31AE, 0x0304}, /* SERIAL_FORMAT */
	},
};

static struct vin_video_format ar0331_formats[] = {
	{
		.video_mode	= AMBA_VIDEO_MODE_QXGA,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 2048,
		.def_height	= 1540,
		.act_start_x	= 0,
		.act_start_y	= 2,
		.act_width	= 2048,
		.act_height	= 1536,
		/* sensor mode */
		.hdr_mode 	= AMBA_VIDEO_LINEAR_MODE,
		.device_mode	= 0,
		.pll_idx	= 0,
		.width		= 2048,
		.height		= 1536,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_30,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_GR,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_1080P,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 1920,
		.def_height	= 1084,
		.act_start_x	= 0,
		.act_start_y	= 2,
		.act_width	= 1920,
		.act_height	= 1080,
		/* sensor mode */
		.hdr_mode 	= AMBA_VIDEO_LINEAR_MODE,
		.device_mode	= 1,
		.pll_idx	= 0,
		.width		= 1920,
		.height		= 1080,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_30,
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
		.def_height	= 724,
		.act_start_x	= 0,
		.act_start_y	= 2,
		.act_width	= 1280,
		.act_height	= 720,
		/* sensor mode */
		.hdr_mode 	= AMBA_VIDEO_LINEAR_MODE,
		.device_mode	= 2,
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
	/* build-in wdr mode */
	{
		.video_mode	= AMBA_VIDEO_MODE_QXGA,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 2048,
		.def_height	= 1540,
		.act_start_x	= 0,
		.act_start_y	= 2,
		.act_width	= 2048,
		.act_height	= 1536,
		/* sensor mode */
		.hdr_mode 	= AMBA_VIDEO_INT_HDR_MODE,
		.device_mode	= 0,
		.pll_idx	= 0,
		.width		= 2048,
		.height		= 1536,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_30,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_GR,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_1080P,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 1920,
		.def_height	= 1084,
		.act_start_x	= 0,
		.act_start_y	= 2,
		.act_width	= 1920,
		.act_height	= 1080,
		/* sensor mode */
		.hdr_mode 	= AMBA_VIDEO_INT_HDR_MODE,
		.device_mode	= 1,
		.pll_idx	= 0,
		.width		= 1920,
		.height		= 1080,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_30,
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
		.def_height	= 724,
		.act_start_x	= 0,
		.act_start_y	= 2,
		.act_width	= 1280,
		.act_height	= 720,
		/* sensor mode */
		.hdr_mode 	= AMBA_VIDEO_INT_HDR_MODE,
		.device_mode	= 2,
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
};

static struct vin_reg_16_16 ar0331_linear_share_regs[] = {
	/* [AR0331 Linear sequencer load - 1.0] */
	{0x3088, 0x8000}, /* SEQ_CTRL_PORT */
	{0x3086, 0x4A03}, /* SEQ_DATA_PORT */
	{0x3086, 0x4316}, /* SEQ_DATA_PORT */
	{0x3086, 0x0443}, /* SEQ_DATA_PORT */
	{0x3086, 0x1645}, /* SEQ_DATA_PORT */
	{0x3086, 0x4045}, /* SEQ_DATA_PORT */
	{0x3086, 0x6017}, /* SEQ_DATA_PORT */
	{0x3086, 0x5045}, /* SEQ_DATA_PORT */
	{0x3086, 0x404B}, /* SEQ_DATA_PORT */
	{0x3086, 0x1244}, /* SEQ_DATA_PORT */
	{0x3086, 0x6134}, /* SEQ_DATA_PORT */
	{0x3086, 0x4A31}, /* SEQ_DATA_PORT */
	{0x3086, 0x4342}, /* SEQ_DATA_PORT */
	{0x3086, 0x4560}, /* SEQ_DATA_PORT */
	{0x3086, 0x2714}, /* SEQ_DATA_PORT */
	{0x3086, 0x3DFF}, /* SEQ_DATA_PORT */
	{0x3086, 0x3DFF}, /* SEQ_DATA_PORT */
	{0x3086, 0x3DEA}, /* SEQ_DATA_PORT */
	{0x3086, 0x2704}, /* SEQ_DATA_PORT */
	{0x3086, 0x3D10}, /* SEQ_DATA_PORT */
	{0x3086, 0x2705}, /* SEQ_DATA_PORT */
	{0x3086, 0x3D10}, /* SEQ_DATA_PORT */
	{0x3086, 0x2715}, /* SEQ_DATA_PORT */
	{0x3086, 0x3527}, /* SEQ_DATA_PORT */
	{0x3086, 0x053D}, /* SEQ_DATA_PORT */
	{0x3086, 0x1045}, /* SEQ_DATA_PORT */
	{0x3086, 0x4027}, /* SEQ_DATA_PORT */
	{0x3086, 0x0427}, /* SEQ_DATA_PORT */
	{0x3086, 0x143D}, /* SEQ_DATA_PORT */
	{0x3086, 0xFF3D}, /* SEQ_DATA_PORT */
	{0x3086, 0xFF3D}, /* SEQ_DATA_PORT */
	{0x3086, 0xEA62}, /* SEQ_DATA_PORT */
	{0x3086, 0x2728}, /* SEQ_DATA_PORT */
	{0x3086, 0x3627}, /* SEQ_DATA_PORT */
	{0x3086, 0x083D}, /* SEQ_DATA_PORT */
	{0x3086, 0x6444}, /* SEQ_DATA_PORT */
	{0x3086, 0x2C2C}, /* SEQ_DATA_PORT */
	{0x3086, 0x2C2C}, /* SEQ_DATA_PORT */
	{0x3086, 0x4B01}, /* SEQ_DATA_PORT */
	{0x3086, 0x432D}, /* SEQ_DATA_PORT */
	{0x3086, 0x4643}, /* SEQ_DATA_PORT */
	{0x3086, 0x1647}, /* SEQ_DATA_PORT */
	{0x3086, 0x435F}, /* SEQ_DATA_PORT */
	{0x3086, 0x4F50}, /* SEQ_DATA_PORT */
	{0x3086, 0x2604}, /* SEQ_DATA_PORT */
	{0x3086, 0x2694}, /* SEQ_DATA_PORT */
	{0x3086, 0x2027}, /* SEQ_DATA_PORT */
	{0x3086, 0xFC53}, /* SEQ_DATA_PORT */
	{0x3086, 0x0D5C}, /* SEQ_DATA_PORT */
	{0x3086, 0x0D57}, /* SEQ_DATA_PORT */
	{0x3086, 0x5417}, /* SEQ_DATA_PORT */
	{0x3086, 0x0955}, /* SEQ_DATA_PORT */
	{0x3086, 0x5649}, /* SEQ_DATA_PORT */
	{0x3086, 0x5307}, /* SEQ_DATA_PORT */
	{0x3086, 0x5303}, /* SEQ_DATA_PORT */
	{0x3086, 0x4D28}, /* SEQ_DATA_PORT */
	{0x3086, 0x6C4C}, /* SEQ_DATA_PORT */
	{0x3086, 0x0928}, /* SEQ_DATA_PORT */
	{0x3086, 0x2C28}, /* SEQ_DATA_PORT */
	{0x3086, 0x294E}, /* SEQ_DATA_PORT */
	{0x3086, 0x5C09}, /* SEQ_DATA_PORT */
	{0x3086, 0x4500}, /* SEQ_DATA_PORT */
	{0x3086, 0x4580}, /* SEQ_DATA_PORT */
	{0x3086, 0x26B6}, /* SEQ_DATA_PORT */
	{0x3086, 0x27F8}, /* SEQ_DATA_PORT */
	{0x3086, 0x1702}, /* SEQ_DATA_PORT */
	{0x3086, 0x27FA}, /* SEQ_DATA_PORT */
	{0x3086, 0x5C0B}, /* SEQ_DATA_PORT */
	{0x3086, 0x1718}, /* SEQ_DATA_PORT */
	{0x3086, 0x26B2}, /* SEQ_DATA_PORT */
	{0x3086, 0x5C03}, /* SEQ_DATA_PORT */
	{0x3086, 0x1744}, /* SEQ_DATA_PORT */
	{0x3086, 0x27F2}, /* SEQ_DATA_PORT */
	{0x3086, 0x1702}, /* SEQ_DATA_PORT */
	{0x3086, 0x2809}, /* SEQ_DATA_PORT */
	{0x3086, 0x1710}, /* SEQ_DATA_PORT */
	{0x3086, 0x1628}, /* SEQ_DATA_PORT */
	{0x3086, 0x084D}, /* SEQ_DATA_PORT */
	{0x3086, 0x1A26}, /* SEQ_DATA_PORT */
	{0x3086, 0x9316}, /* SEQ_DATA_PORT */
	{0x3086, 0x1627}, /* SEQ_DATA_PORT */
	{0x3086, 0xFA45}, /* SEQ_DATA_PORT */
	{0x3086, 0xA017}, /* SEQ_DATA_PORT */
	{0x3086, 0x0727}, /* SEQ_DATA_PORT */
	{0x3086, 0xFB17}, /* SEQ_DATA_PORT */
	{0x3086, 0x2945}, /* SEQ_DATA_PORT */
	{0x3086, 0x8017}, /* SEQ_DATA_PORT */
	{0x3086, 0x0827}, /* SEQ_DATA_PORT */
	{0x3086, 0xFA17}, /* SEQ_DATA_PORT */
	{0x3086, 0x285D}, /* SEQ_DATA_PORT */
	{0x3086, 0x170E}, /* SEQ_DATA_PORT */
	{0x3086, 0x2691}, /* SEQ_DATA_PORT */
	{0x3086, 0x5301}, /* SEQ_DATA_PORT */
	{0x3086, 0x1740}, /* SEQ_DATA_PORT */
	{0x3086, 0x5302}, /* SEQ_DATA_PORT */
	{0x3086, 0x1710}, /* SEQ_DATA_PORT */
	{0x3086, 0x2693}, /* SEQ_DATA_PORT */
	{0x3086, 0x2692}, /* SEQ_DATA_PORT */
	{0x3086, 0x484D}, /* SEQ_DATA_PORT */
	{0x3086, 0x4E28}, /* SEQ_DATA_PORT */
	{0x3086, 0x094C}, /* SEQ_DATA_PORT */
	{0x3086, 0x0B17}, /* SEQ_DATA_PORT */
	{0x3086, 0x5F27}, /* SEQ_DATA_PORT */
	{0x3086, 0xF217}, /* SEQ_DATA_PORT */
	{0x3086, 0x1428}, /* SEQ_DATA_PORT */
	{0x3086, 0x0816}, /* SEQ_DATA_PORT */
	{0x3086, 0x4D1A}, /* SEQ_DATA_PORT */
	{0x3086, 0x1616}, /* SEQ_DATA_PORT */
	{0x3086, 0x27FA}, /* SEQ_DATA_PORT */
	{0x3086, 0x2603}, /* SEQ_DATA_PORT */
	{0x3086, 0x5C01}, /* SEQ_DATA_PORT */
	{0x3086, 0x4540}, /* SEQ_DATA_PORT */
	{0x3086, 0x2798}, /* SEQ_DATA_PORT */
	{0x3086, 0x172A}, /* SEQ_DATA_PORT */
	{0x3086, 0x4A0A}, /* SEQ_DATA_PORT */
	{0x3086, 0x4316}, /* SEQ_DATA_PORT */
	{0x3086, 0x0B43}, /* SEQ_DATA_PORT */
	{0x3086, 0x279C}, /* SEQ_DATA_PORT */
	{0x3086, 0x4560}, /* SEQ_DATA_PORT */
	{0x3086, 0x1707}, /* SEQ_DATA_PORT */
	{0x3086, 0x279D}, /* SEQ_DATA_PORT */
	{0x3086, 0x1725}, /* SEQ_DATA_PORT */
	{0x3086, 0x4540}, /* SEQ_DATA_PORT */
	{0x3086, 0x1708}, /* SEQ_DATA_PORT */
	{0x3086, 0x2798}, /* SEQ_DATA_PORT */
	{0x3086, 0x5D53}, /* SEQ_DATA_PORT */
	{0x3086, 0x0D26}, /* SEQ_DATA_PORT */
	{0x3086, 0x455C}, /* SEQ_DATA_PORT */
	{0x3086, 0x014B}, /* SEQ_DATA_PORT */
	{0x3086, 0x1244}, /* SEQ_DATA_PORT */
	{0x3086, 0x5251}, /* SEQ_DATA_PORT */
	{0x3086, 0x1702}, /* SEQ_DATA_PORT */
	{0x3086, 0x6018}, /* SEQ_DATA_PORT */
	{0x3086, 0x4A03}, /* SEQ_DATA_PORT */
	{0x3086, 0x4316}, /* SEQ_DATA_PORT */
	{0x3086, 0x0443}, /* SEQ_DATA_PORT */
	{0x3086, 0x1658}, /* SEQ_DATA_PORT */
	{0x3086, 0x4316}, /* SEQ_DATA_PORT */
	{0x3086, 0x5943}, /* SEQ_DATA_PORT */
	{0x3086, 0x165A}, /* SEQ_DATA_PORT */
	{0x3086, 0x4316}, /* SEQ_DATA_PORT */
	{0x3086, 0x5B43}, /* SEQ_DATA_PORT */
	{0x3086, 0x4540}, /* SEQ_DATA_PORT */
	{0x3086, 0x279C}, /* SEQ_DATA_PORT */
	{0x3086, 0x4560}, /* SEQ_DATA_PORT */
	{0x3086, 0x1707}, /* SEQ_DATA_PORT */
	{0x3086, 0x279D}, /* SEQ_DATA_PORT */
	{0x3086, 0x1725}, /* SEQ_DATA_PORT */
	{0x3086, 0x4540}, /* SEQ_DATA_PORT */
	{0x3086, 0x1710}, /* SEQ_DATA_PORT */
	{0x3086, 0x2798}, /* SEQ_DATA_PORT */
	{0x3086, 0x1720}, /* SEQ_DATA_PORT */
	{0x3086, 0x224B}, /* SEQ_DATA_PORT */
	{0x3086, 0x1244}, /* SEQ_DATA_PORT */
	{0x3086, 0x2C2C}, /* SEQ_DATA_PORT */
	{0x3086, 0x2C2C}, /* SEQ_DATA_PORT */

	{0x301A, 0x0058}, /* FIELD_WR= RESET_REGISTER, 0x0058 */
	{0x30B0, 0x0000}, /* FIELD_WR= DIGITAL_TEST, 0x0000 */
	{0x3064, 0x1902}, /* FIELD_WR= SMIA_TEST, 0x1902 */
	{0x31AC, 0x0C0C}, /* FIELD_WR= DATA_FORMAT_BITS, 0x0C0C */

	{0x31C6, 0x8002},	/* MSB */
	/* [Linear Mode] */
	{0x3082, 0x0009}, /* FIELD_WR= OPERATION_MODE_CTRL, 0x0009 */
	{0x30BA, 0x06EC}, /* FIELD_WR= DIGITAL_CTRL, 0x06EC */
	/* [2D motion compensation OFF] */
	{0x320A, 0x0080}, /* REG= 0x320A, 0x0080 */
	{0x318C, 0x0000}, /* FIELD_WR= HDR_MC_CTRL2, 0x0000 */
	/* [ALTM Disabled] */
	{0x301E, 0x00A8}, /* FIELD_WR= DATA_PEDESTAL, 0X00A8 */
	{0x2450, 0x0000}, /* REG= 0x2450, 0x0000 */
	{0x2400, 0x0003}, /* FIELD_WR= ALTM_CONTROL, 0x0003 */
	/* [ADACD Disabled] */
	{0x3200, 0x0000}, /* REG= 0x3200, 0x0000 */
	/* [Linear Mode Setup] */
	{0x31D0, 0x0000}, /* FIELD_WR= COMPANDING, 0x0000  */
	{0x30FE, 0x0080}, /* REG= 0x30FE, 0x0080 */
	{0x3064, 0x1802}, /* FIELD_WR= SMIA_TEST, EMBEDDED_STATS_EN, 0 */
	{0x31AE, 0x0304}, /* FIELD_WR= SERIAL_FORMAT, 0x0304 */
	/* [Analog Settings] */
	{0x2440, 0x0002},
	{0x30F4, 0x4000},
	{0x3180, 0x8029},
	{0x3198, 0x061E},
	{0x3ED2, 0x3F46},
	{0x3EDA, 0x8899},
	{0x3EE6, 0x00F0},
	/* [Blooming] */
	{0x3ED4, 0x8F9C}, /* DAC_LD_8_9 */
	{0x3ED6, 0x99CC}, /* DAC_LD_10_11 */
	{0x3ED8, 0x8C42},

	{0x30CE, 0x0020}, /* frame start mode */

	{0xFFFF, 3},      /* delay 3ms */
};

static struct vin_reg_16_16 ar0331_hdr_share_regs[] = {
	/* AR0331 HDR sequencer load - 3.0 */
	{0x3088, 0x8000}, /* SEQ_CTRL_PORT */
	{0x3086, 0x4540}, /* SEQ_DATA_PORT */
	{0x3086, 0x6134}, /* SEQ_DATA_PORT */
	{0x3086, 0x4A31}, /* SEQ_DATA_PORT */
	{0x3086, 0x4342}, /* SEQ_DATA_PORT */
	{0x3086, 0x4560}, /* SEQ_DATA_PORT */
	{0x3086, 0x2714}, /* SEQ_DATA_PORT */
	{0x3086, 0x3DFF}, /* SEQ_DATA_PORT */
	{0x3086, 0x3DFF}, /* SEQ_DATA_PORT */
	{0x3086, 0x3DEA}, /* SEQ_DATA_PORT */
	{0x3086, 0x2704}, /* SEQ_DATA_PORT */
	{0x3086, 0x3D10}, /* SEQ_DATA_PORT */
	{0x3086, 0x2705}, /* SEQ_DATA_PORT */
	{0x3086, 0x3D10}, /* SEQ_DATA_PORT */
	{0x3086, 0x2715}, /* SEQ_DATA_PORT */
	{0x3086, 0x3527}, /* SEQ_DATA_PORT */
	{0x3086, 0x053D}, /* SEQ_DATA_PORT */
	{0x3086, 0x1045}, /* SEQ_DATA_PORT */
	{0x3086, 0x4027}, /* SEQ_DATA_PORT */
	{0x3086, 0x0427}, /* SEQ_DATA_PORT */
	{0x3086, 0x143D}, /* SEQ_DATA_PORT */
	{0x3086, 0xFF3D}, /* SEQ_DATA_PORT */
	{0x3086, 0xFF3D}, /* SEQ_DATA_PORT */
	{0x3086, 0xEA62}, /* SEQ_DATA_PORT */
	{0x3086, 0x2728}, /* SEQ_DATA_PORT */
	{0x3086, 0x3627}, /* SEQ_DATA_PORT */
	{0x3086, 0x083D}, /* SEQ_DATA_PORT */
	{0x3086, 0x6444}, /* SEQ_DATA_PORT */
	{0x3086, 0x2C2C}, /* SEQ_DATA_PORT */
	{0x3086, 0x2C2C}, /* SEQ_DATA_PORT */
	{0x3086, 0x4B00}, /* SEQ_DATA_PORT */
	{0x3086, 0x432D}, /* SEQ_DATA_PORT */
	{0x3086, 0x6343}, /* SEQ_DATA_PORT */
	{0x3086, 0x1664}, /* SEQ_DATA_PORT */
	{0x3086, 0x435F}, /* SEQ_DATA_PORT */
	{0x3086, 0x4F50}, /* SEQ_DATA_PORT */
	{0x3086, 0x2604}, /* SEQ_DATA_PORT */
	{0x3086, 0x2694}, /* SEQ_DATA_PORT */
	{0x3086, 0x27FC}, /* SEQ_DATA_PORT */
	{0x3086, 0x530D}, /* SEQ_DATA_PORT */
	{0x3086, 0x5C0D}, /* SEQ_DATA_PORT */
	{0x3086, 0x5754}, /* SEQ_DATA_PORT */
	{0x3086, 0x1709}, /* SEQ_DATA_PORT */
	{0x3086, 0x5556}, /* SEQ_DATA_PORT */
	{0x3086, 0x4953}, /* SEQ_DATA_PORT */
	{0x3086, 0x0753}, /* SEQ_DATA_PORT */
	{0x3086, 0x034D}, /* SEQ_DATA_PORT */
	{0x3086, 0x286C}, /* SEQ_DATA_PORT */
	{0x3086, 0x4C09}, /* SEQ_DATA_PORT */
	{0x3086, 0x282C}, /* SEQ_DATA_PORT */
	{0x3086, 0x2828}, /* SEQ_DATA_PORT */
	{0x3086, 0x261C}, /* SEQ_DATA_PORT */
	{0x3086, 0x4E5C}, /* SEQ_DATA_PORT */
	{0x3086, 0x0960}, /* SEQ_DATA_PORT */
	{0x3086, 0x4500}, /* SEQ_DATA_PORT */
	{0x3086, 0x4580}, /* SEQ_DATA_PORT */
	{0x3086, 0x26BE}, /* SEQ_DATA_PORT */
	{0x3086, 0x27F8}, /* SEQ_DATA_PORT */
	{0x3086, 0x1702}, /* SEQ_DATA_PORT */
	{0x3086, 0x27FA}, /* SEQ_DATA_PORT */
	{0x3086, 0x5C0B}, /* SEQ_DATA_PORT */
	{0x3086, 0x1712}, /* SEQ_DATA_PORT */
	{0x3086, 0x26BA}, /* SEQ_DATA_PORT */
	{0x3086, 0x5C03}, /* SEQ_DATA_PORT */
	{0x3086, 0x1713}, /* SEQ_DATA_PORT */
	{0x3086, 0x27F2}, /* SEQ_DATA_PORT */
	{0x3086, 0x171C}, /* SEQ_DATA_PORT */
	{0x3086, 0x5F28}, /* SEQ_DATA_PORT */
	{0x3086, 0x0817}, /* SEQ_DATA_PORT */
	{0x3086, 0x0360}, /* SEQ_DATA_PORT */
	{0x3086, 0x173C}, /* SEQ_DATA_PORT */
	{0x3086, 0x26B2}, /* SEQ_DATA_PORT */
	{0x3086, 0x1616}, /* SEQ_DATA_PORT */
	{0x3086, 0x5F4D}, /* SEQ_DATA_PORT */
	{0x3086, 0x1926}, /* SEQ_DATA_PORT */
	{0x3086, 0x9316}, /* SEQ_DATA_PORT */
	{0x3086, 0x1627}, /* SEQ_DATA_PORT */
	{0x3086, 0xFA45}, /* SEQ_DATA_PORT */
	{0x3086, 0xA017}, /* SEQ_DATA_PORT */
	{0x3086, 0x0527}, /* SEQ_DATA_PORT */
	{0x3086, 0xFB17}, /* SEQ_DATA_PORT */
	{0x3086, 0x1F45}, /* SEQ_DATA_PORT */
	{0x3086, 0x801F}, /* SEQ_DATA_PORT */
	{0x3086, 0x1705}, /* SEQ_DATA_PORT */
	{0x3086, 0x27FA}, /* SEQ_DATA_PORT */
	{0x3086, 0x171E}, /* SEQ_DATA_PORT */
	{0x3086, 0x5D17}, /* SEQ_DATA_PORT */
	{0x3086, 0x0C26}, /* SEQ_DATA_PORT */
	{0x3086, 0x9248}, /* SEQ_DATA_PORT */
	{0x3086, 0x4D4E}, /* SEQ_DATA_PORT */
	{0x3086, 0x269A}, /* SEQ_DATA_PORT */
	{0x3086, 0x2808}, /* SEQ_DATA_PORT */
	{0x3086, 0x4C0B}, /* SEQ_DATA_PORT */
	{0x3086, 0x6017}, /* SEQ_DATA_PORT */
	{0x3086, 0x0327}, /* SEQ_DATA_PORT */
	{0x3086, 0xF217}, /* SEQ_DATA_PORT */
	{0x3086, 0x2626}, /* SEQ_DATA_PORT */
	{0x3086, 0x9216}, /* SEQ_DATA_PORT */
	{0x3086, 0x165F}, /* SEQ_DATA_PORT */
	{0x3086, 0x4D19}, /* SEQ_DATA_PORT */
	{0x3086, 0x2693}, /* SEQ_DATA_PORT */
	{0x3086, 0x1616}, /* SEQ_DATA_PORT */
	{0x3086, 0x27FA}, /* SEQ_DATA_PORT */
	{0x3086, 0x2643}, /* SEQ_DATA_PORT */
	{0x3086, 0x5C01}, /* SEQ_DATA_PORT */
	{0x3086, 0x4540}, /* SEQ_DATA_PORT */
	{0x3086, 0x2798}, /* SEQ_DATA_PORT */
	{0x3086, 0x1720}, /* SEQ_DATA_PORT */
	{0x3086, 0x4A65}, /* SEQ_DATA_PORT */
	{0x3086, 0x4316}, /* SEQ_DATA_PORT */
	{0x3086, 0x6643}, /* SEQ_DATA_PORT */
	{0x3086, 0x5A43}, /* SEQ_DATA_PORT */
	{0x3086, 0x165B}, /* SEQ_DATA_PORT */
	{0x3086, 0x4327}, /* SEQ_DATA_PORT */
	{0x3086, 0x9C45}, /* SEQ_DATA_PORT */
	{0x3086, 0x6017}, /* SEQ_DATA_PORT */
	{0x3086, 0x0627}, /* SEQ_DATA_PORT */
	{0x3086, 0x9D17}, /* SEQ_DATA_PORT */
	{0x3086, 0x1C45}, /* SEQ_DATA_PORT */
	{0x3086, 0x4023}, /* SEQ_DATA_PORT */
	{0x3086, 0x1705}, /* SEQ_DATA_PORT */
	{0x3086, 0x2798}, /* SEQ_DATA_PORT */
	{0x3086, 0x5D26}, /* SEQ_DATA_PORT */
	{0x3086, 0x4417}, /* SEQ_DATA_PORT */
	{0x3086, 0x0E28}, /* SEQ_DATA_PORT */
	{0x3086, 0x0053}, /* SEQ_DATA_PORT */
	{0x3086, 0x014B}, /* SEQ_DATA_PORT */
	{0x3086, 0x5251}, /* SEQ_DATA_PORT */
	{0x3086, 0x1244}, /* SEQ_DATA_PORT */
	{0x3086, 0x4B01}, /* SEQ_DATA_PORT */
	{0x3086, 0x432D}, /* SEQ_DATA_PORT */
	{0x3086, 0x4643}, /* SEQ_DATA_PORT */
	{0x3086, 0x1647}, /* SEQ_DATA_PORT */
	{0x3086, 0x434F}, /* SEQ_DATA_PORT */
	{0x3086, 0x5026}, /* SEQ_DATA_PORT */
	{0x3086, 0x0426}, /* SEQ_DATA_PORT */
	{0x3086, 0x8427}, /* SEQ_DATA_PORT */
	{0x3086, 0xFC53}, /* SEQ_DATA_PORT */
	{0x3086, 0x0D5C}, /* SEQ_DATA_PORT */
	{0x3086, 0x0D57}, /* SEQ_DATA_PORT */
	{0x3086, 0x5417}, /* SEQ_DATA_PORT */
	{0x3086, 0x0955}, /* SEQ_DATA_PORT */
	{0x3086, 0x5649}, /* SEQ_DATA_PORT */
	{0x3086, 0x5307}, /* SEQ_DATA_PORT */
	{0x3086, 0x5303}, /* SEQ_DATA_PORT */
	{0x3086, 0x4D28}, /* SEQ_DATA_PORT */
	{0x3086, 0x6C4C}, /* SEQ_DATA_PORT */
	{0x3086, 0x0928}, /* SEQ_DATA_PORT */
	{0x3086, 0x2C28}, /* SEQ_DATA_PORT */
	{0x3086, 0x2826}, /* SEQ_DATA_PORT */
	{0x3086, 0x0C4E}, /* SEQ_DATA_PORT */
	{0x3086, 0x5C09}, /* SEQ_DATA_PORT */
	{0x3086, 0x6045}, /* SEQ_DATA_PORT */
	{0x3086, 0x0045}, /* SEQ_DATA_PORT */
	{0x3086, 0x8026}, /* SEQ_DATA_PORT */
	{0x3086, 0xAE27}, /* SEQ_DATA_PORT */
	{0x3086, 0xF817}, /* SEQ_DATA_PORT */
	{0x3086, 0x0227}, /* SEQ_DATA_PORT */
	{0x3086, 0xFA5C}, /* SEQ_DATA_PORT */
	{0x3086, 0x0B17}, /* SEQ_DATA_PORT */
	{0x3086, 0x1226}, /* SEQ_DATA_PORT */
	{0x3086, 0xAA5C}, /* SEQ_DATA_PORT */
	{0x3086, 0x0317}, /* SEQ_DATA_PORT */
	{0x3086, 0x0B27}, /* SEQ_DATA_PORT */
	{0x3086, 0xF217}, /* SEQ_DATA_PORT */
	{0x3086, 0x265F}, /* SEQ_DATA_PORT */
	{0x3086, 0x2808}, /* SEQ_DATA_PORT */
	{0x3086, 0x1703}, /* SEQ_DATA_PORT */
	{0x3086, 0x6017}, /* SEQ_DATA_PORT */
	{0x3086, 0x0326}, /* SEQ_DATA_PORT */
	{0x3086, 0xA216}, /* SEQ_DATA_PORT */
	{0x3086, 0x165F}, /* SEQ_DATA_PORT */
	{0x3086, 0x4D1A}, /* SEQ_DATA_PORT */
	{0x3086, 0x2683}, /* SEQ_DATA_PORT */
	{0x3086, 0x1616}, /* SEQ_DATA_PORT */
	{0x3086, 0x27FA}, /* SEQ_DATA_PORT */
	{0x3086, 0x45A0}, /* SEQ_DATA_PORT */
	{0x3086, 0x1705}, /* SEQ_DATA_PORT */
	{0x3086, 0x27FB}, /* SEQ_DATA_PORT */
	{0x3086, 0x171F}, /* SEQ_DATA_PORT */
	{0x3086, 0x4580}, /* SEQ_DATA_PORT */
	{0x3086, 0x2017}, /* SEQ_DATA_PORT */
	{0x3086, 0x0527}, /* SEQ_DATA_PORT */
	{0x3086, 0xFA17}, /* SEQ_DATA_PORT */
	{0x3086, 0x1E5D}, /* SEQ_DATA_PORT */
	{0x3086, 0x170C}, /* SEQ_DATA_PORT */
	{0x3086, 0x2682}, /* SEQ_DATA_PORT */
	{0x3086, 0x484D}, /* SEQ_DATA_PORT */
	{0x3086, 0x4E26}, /* SEQ_DATA_PORT */
	{0x3086, 0x8A28}, /* SEQ_DATA_PORT */
	{0x3086, 0x084C}, /* SEQ_DATA_PORT */
	{0x3086, 0x0B60}, /* SEQ_DATA_PORT */
	{0x3086, 0x1707}, /* SEQ_DATA_PORT */
	{0x3086, 0x27F2}, /* SEQ_DATA_PORT */
	{0x3086, 0x1738}, /* SEQ_DATA_PORT */
	{0x3086, 0x2682}, /* SEQ_DATA_PORT */
	{0x3086, 0x1616}, /* SEQ_DATA_PORT */
	{0x3086, 0x5F4D}, /* SEQ_DATA_PORT */
	{0x3086, 0x1A26}, /* SEQ_DATA_PORT */
	{0x3086, 0x8316}, /* SEQ_DATA_PORT */
	{0x3086, 0x1627}, /* SEQ_DATA_PORT */
	{0x3086, 0xFA26}, /* SEQ_DATA_PORT */
	{0x3086, 0x435C}, /* SEQ_DATA_PORT */
	{0x3086, 0x0145}, /* SEQ_DATA_PORT */
	{0x3086, 0x4027}, /* SEQ_DATA_PORT */
	{0x3086, 0x9817}, /* SEQ_DATA_PORT */
	{0x3086, 0x1F4A}, /* SEQ_DATA_PORT */
	{0x3086, 0x1244}, /* SEQ_DATA_PORT */
	{0x3086, 0x0343}, /* SEQ_DATA_PORT */
	{0x3086, 0x1604}, /* SEQ_DATA_PORT */
	{0x3086, 0x4316}, /* SEQ_DATA_PORT */
	{0x3086, 0x5843}, /* SEQ_DATA_PORT */
	{0x3086, 0x1659}, /* SEQ_DATA_PORT */
	{0x3086, 0x4316}, /* SEQ_DATA_PORT */
	{0x3086, 0x279C}, /* SEQ_DATA_PORT */
	{0x3086, 0x4560}, /* SEQ_DATA_PORT */
	{0x3086, 0x1705}, /* SEQ_DATA_PORT */
	{0x3086, 0x279D}, /* SEQ_DATA_PORT */
	{0x3086, 0x171D}, /* SEQ_DATA_PORT */
	{0x3086, 0x4540}, /* SEQ_DATA_PORT */
	{0x3086, 0x2217}, /* SEQ_DATA_PORT */
	{0x3086, 0x0527}, /* SEQ_DATA_PORT */
	{0x3086, 0x985D}, /* SEQ_DATA_PORT */
	{0x3086, 0x2645}, /* SEQ_DATA_PORT */
	{0x3086, 0x170E}, /* SEQ_DATA_PORT */
	{0x3086, 0x2800}, /* SEQ_DATA_PORT */
	{0x3086, 0x5301}, /* SEQ_DATA_PORT */
	{0x3086, 0x4B52}, /* SEQ_DATA_PORT */
	{0x3086, 0x5112}, /* SEQ_DATA_PORT */
	{0x3086, 0x4460}, /* SEQ_DATA_PORT */
	{0x3086, 0x2C2C}, /* SEQ_DATA_PORT */
	{0x3086, 0x2C2C}, /* SEQ_DATA_PORT */

	{0x301A, 0x0058}, /* RESET_REGISTER */
	{0x30B0, 0x0000}, /* DIGITAL_TEST */
	{0x30BA, 0x07EC}, /* DIGITAL_CTRL */
	{0x31AC, 0x100C}, /* DATA_FORMAT_BITS */

	/* HDR Mode 16x */
	{0x3082, 0x0008}, /* OPERATION_MODE_CTRL */
	{0x305E, 0x0080}, /* GLOBAL_GAIN */
	/* [ADACD Enabled] */
	{0x3202, 0x01FF}, /* REG= 0x3202, 0x00A0 */
	{0x3206, 0x1810}, /* REG= 0x3206, 0x1810 */
	{0x3208, 0x2820}, /* REG= 0x3208, 0x2820 */
	{0x3200, 0X0002}, /* REG= 0x3200, 0x0002 */
	/* [HDR Mode Setup] */
	/* [2D motion compensation OFF] */
	{0x318C, 0x4001}, /* FIELD_WR= HDR_MC_CTRL2, 0x0000 */
	{0x3198, 0x061E},
	/* [ALTM Enabled] */
	{0x301A, 0x0058}, /* RESET_REGISTER */
	{0x2442, 0x0080}, /* ALTM_CONTROL_KEY_K0 */
	{0x2444, 0x0000}, /* REG= 0x2444, 0x0000 */
	{0x2446, 0x0010}, /* REG= 0x2446, 0x0004 */
	{0x2440, 0x0004}, /* ALTM_CONTROL_DAMPER */
	{0x2438, 0x0010}, /* ALTM_CONTROL_MIN_FACTOR */
	{0x243A, 0x0020}, /* ALTM_CONTROL_MAX_FACTOR */
	{0x243C, 0x0000}, /* ALTM_CONTROL_DARK_FLOOR */
	{0x243E, 0x0200}, /* ALTM_CONTROL_BRIGHT_FLOOR */
	{0x31D0, 0x0000}, /* COMPANDING */
	{0x301E, 0x0010}, /* DATA_PEDESTAL */
	{0x2400, 0x0002}, /* ALTM_CONTROL */
	{0x2410, 0x0010},
	{0x2412, 0x0010},
	/* modify stat start&end perc */
	{0x314A, 0xfff0},
	{0x3148, 0x0002},
	{0x301A, 0x005C}, /* RESET_REGISTER */
	{0x30FE, 0x0000}, /* REG= 0x30FE, 0x0000 */
	{0x31E0, 0x0200}, /* PIX_DEF_ID */
	{0x320A, 0x0000}, /* REG= 0x320A, 0x0080 */
	{0x2450, 0x0010}, /* ALTM_OUT_PEDESTAL */
	{0x301E, 0x0010}, /* DATA_PEDESTAL */
	{0x318A, 0x0E10}, /* HDR_MC_CTRL1 */
	{0x3064, 0x1982}, /* FIELD_WR= SMIA_TEST, EMBEDDED_STATS_EN, 0 */
	{0x301A, 0x005E}, /* RESET_REGISTER */
	{0x3202, 0x01FF}, /* ADACD_NOISE_MODEL1 */
	{0x3206, 0x1810}, /* ADACD_NOISE_FLOOR1 */
	{0x3208, 0x2820}, /* ADACD_NOISE_FLOOR2 */
	{0x3200, 0x0002}, /* ADACD_CONTROL */
	{0x31AE, 0x0304}, /* FIELD_WR= SERIAL_FORMAT, 0x0304 */
	{0x306E, 0x9210}, /* DATAPATH_SELECT, HiVcm */
	/* Analog Settings */
	{0x3180, 0x8089}, /* DELTA_DK_CONTROL */
	{0x30F4, 0x4000}, /* ADC_COMMAND3_HS */
	{0x3EDA, 0x8899}, /* DAC_LD_14_15 */
	{0x3EE6, 0x00F0}, /* DAC_LD_26_27 */
	{0x3ED2, 0x9546}, /* DAC_LD_6_7 */

	{0x31C6, 0x8002}, /* MSB */
	{0x3060, 0x0006}, /* ANALOG_GAIN */

	/* [Blooming] */
	{0x3ED4, 0x8F9C}, /* DAC_LD_8_9 */
	{0x3ED6, 0x99CC}, /* DAC_LD_10_11 */
	{0x3ED8, 0x8C42},

	{0x30CE, 0x0020}, /* frame start mode */

	{0x301A, 0x005C}, /* RESET_REGISTER */
	{0xFFFF, 10},     /* delay 10ms */
};

#define AR0331_GAIN_COL_REG_AGAIN	(0)
#define AR0331_GAIN_COL_REG_DGAIN	(1)

/* AR0331 linear gain table row size */
#define AR0331_LINEAR_GAIN_ROWS		(858)
#define AR0331_LINEAR_GAIN_COLS		(2)
#define AR0331_LINEAR_GAIN_30DB		(601)
#define AR0331_LINEAR_GAIN_42DB		(857)

static const u16 AR0331_LINEAR_GAIN_TABLE[AR0331_LINEAR_GAIN_ROWS][AR0331_LINEAR_GAIN_COLS] =
{
	{0x06, 0x80}, // index: 0, gain:1.833000DB->x1.234952
	{0x06, 0x81}, // index: 1, gain:1.880000DB->x1.241652
	{0x06, 0x81}, // index: 2, gain:1.927000DB->x1.248389
	{0x06, 0x82}, // index: 3, gain:1.974000DB->x1.255163
	{0x06, 0x83}, // index: 4, gain:2.021000DB->x1.261973
	{0x06, 0x84}, // index: 5, gain:2.068000DB->x1.268820
	{0x06, 0x84}, // index: 6, gain:2.115000DB->x1.275704
	{0x07, 0x80}, // index: 7, gain:2.162000DB->x1.282626
	{0x07, 0x80}, // index: 8, gain:2.209000DB->x1.289585
	{0x07, 0x81}, // index: 9, gain:2.256000DB->x1.296582
	{0x07, 0x82}, // index: 10, gain:2.303000DB->x1.303617
	{0x07, 0x83}, // index: 11, gain:2.350000DB->x1.310690
	{0x07, 0x83}, // index: 12, gain:2.397000DB->x1.317802
	{0x07, 0x84}, // index: 13, gain:2.444000DB->x1.324952
	{0x07, 0x85}, // index: 14, gain:2.491000DB->x1.332140
	{0x07, 0x85}, // index: 15, gain:2.538000DB->x1.339368
	{0x08, 0x80}, // index: 16, gain:2.585000DB->x1.346635
	{0x08, 0x81}, // index: 17, gain:2.632000DB->x1.353942
	{0x08, 0x82}, // index: 18, gain:2.679000DB->x1.361288
	{0x08, 0x82}, // index: 19, gain:2.726000DB->x1.368674
	{0x08, 0x83}, // index: 20, gain:2.773000DB->x1.376100
	{0x08, 0x84}, // index: 21, gain:2.820000DB->x1.383566
	{0x09, 0x80}, // index: 22, gain:2.867000DB->x1.391073
	{0x09, 0x80}, // index: 23, gain:2.914000DB->x1.398621
	{0x09, 0x81}, // index: 24, gain:2.961000DB->x1.406209
	{0x09, 0x82}, // index: 25, gain:3.008000DB->x1.413839
	{0x09, 0x82}, // index: 26, gain:3.055000DB->x1.421510
	{0x09, 0x83}, // index: 27, gain:3.102000DB->x1.429223
	{0x09, 0x84}, // index: 28, gain:3.149000DB->x1.436978
	{0x09, 0x85}, // index: 29, gain:3.196000DB->x1.444774
	{0x0a, 0x80}, // index: 30, gain:3.243000DB->x1.452613
	{0x0a, 0x80}, // index: 31, gain:3.290000DB->x1.460495
	{0x0a, 0x81}, // index: 32, gain:3.337000DB->x1.468419
	{0x0a, 0x82}, // index: 33, gain:3.384000DB->x1.476386
	{0x0a, 0x83}, // index: 34, gain:3.431000DB->x1.484397
	{0x0a, 0x83}, // index: 35, gain:3.478000DB->x1.492451
	{0x0a, 0x84}, // index: 36, gain:3.525000DB->x1.500548
	{0x0a, 0x85}, // index: 37, gain:3.572000DB->x1.508690
	{0x0a, 0x85}, // index: 38, gain:3.619000DB->x1.516876
	{0x0b, 0x80}, // index: 39, gain:3.666000DB->x1.525106
	{0x0b, 0x81}, // index: 40, gain:3.713000DB->x1.533381
	{0x0b, 0x81}, // index: 41, gain:3.760000DB->x1.541700
	{0x0b, 0x82}, // index: 42, gain:3.807000DB->x1.550065
	{0x0b, 0x83}, // index: 43, gain:3.854000DB->x1.558476
	{0x0b, 0x83}, // index: 44, gain:3.901000DB->x1.566931
	{0x0b, 0x84}, // index: 45, gain:3.948000DB->x1.575433
	{0x0b, 0x85}, // index: 46, gain:3.995000DB->x1.583981
	{0x0b, 0x86}, // index: 47, gain:4.042000DB->x1.592575
	{0x0c, 0x80}, // index: 48, gain:4.089000DB->x1.601216
	{0x0c, 0x80}, // index: 49, gain:4.136000DB->x1.609904
	{0x0c, 0x81}, // index: 50, gain:4.183000DB->x1.618639
	{0x0c, 0x82}, // index: 51, gain:4.230000DB->x1.627421
	{0x0c, 0x82}, // index: 52, gain:4.277000DB->x1.636251
	{0x0c, 0x83}, // index: 53, gain:4.324000DB->x1.645129
	{0x0c, 0x84}, // index: 54, gain:4.371000DB->x1.654055
	{0x0c, 0x85}, // index: 55, gain:4.418000DB->x1.663030
	{0x0c, 0x85}, // index: 56, gain:4.465000DB->x1.672053
	{0x0c, 0x86}, // index: 57, gain:4.512000DB->x1.681125
	{0x0d, 0x80}, // index: 58, gain:4.559000DB->x1.690246
	{0x0d, 0x80}, // index: 59, gain:4.606000DB->x1.699417
	{0x0d, 0x81}, // index: 60, gain:4.653000DB->x1.708638
	{0x0d, 0x82}, // index: 61, gain:4.700000DB->x1.717908
	{0x0d, 0x82}, // index: 62, gain:4.747000DB->x1.727229
	{0x0d, 0x83}, // index: 63, gain:4.794000DB->x1.736601
	{0x0d, 0x84}, // index: 64, gain:4.841000DB->x1.746023
	{0x0d, 0x84}, // index: 65, gain:4.888000DB->x1.755497
	{0x0d, 0x85}, // index: 66, gain:4.935000DB->x1.765022
	{0x0d, 0x86}, // index: 67, gain:4.982000DB->x1.774598
	{0x0e, 0x80}, // index: 68, gain:5.029000DB->x1.784227
	{0x0e, 0x81}, // index: 69, gain:5.076000DB->x1.793907
	{0x0e, 0x81}, // index: 70, gain:5.123000DB->x1.803641
	{0x0e, 0x82}, // index: 71, gain:5.170000DB->x1.813427
	{0x0e, 0x83}, // index: 72, gain:5.217000DB->x1.823266
	{0x0e, 0x83}, // index: 73, gain:5.264000DB->x1.833158
	{0x0e, 0x84}, // index: 74, gain:5.311000DB->x1.843105
	{0x0e, 0x85}, // index: 75, gain:5.358000DB->x1.853105
	{0x0e, 0x85}, // index: 76, gain:5.405000DB->x1.863159
	{0x0e, 0x86}, // index: 77, gain:5.452000DB->x1.873268
	{0x0f, 0x80}, // index: 78, gain:5.499000DB->x1.883432
	{0x0f, 0x80}, // index: 79, gain:5.546000DB->x1.893651
	{0x0f, 0x81}, // index: 80, gain:5.593000DB->x1.903926
	{0x0f, 0x82}, // index: 81, gain:5.640000DB->x1.914256
	{0x0f, 0x83}, // index: 82, gain:5.687000DB->x1.924642
	{0x0f, 0x83}, // index: 83, gain:5.734000DB->x1.935085
	{0x0f, 0x84}, // index: 84, gain:5.781000DB->x1.945584
	{0x0f, 0x85}, // index: 85, gain:5.828000DB->x1.956140
	{0x0f, 0x85}, // index: 86, gain:5.875000DB->x1.966754
	{0x0f, 0x86}, // index: 87, gain:5.922000DB->x1.977425
	{0x0f, 0x87}, // index: 88, gain:5.969000DB->x1.988154
	{0x0f, 0x88}, // index: 89, gain:6.016000DB->x1.998941
	{0x10, 0x80}, // index: 90, gain:6.063000DB->x2.009787
	{0x10, 0x81}, // index: 91, gain:6.110000DB->x2.020691
	{0x10, 0x82}, // index: 92, gain:6.157000DB->x2.031655
	{0x10, 0x82}, // index: 93, gain:6.204000DB->x2.042678
	{0x10, 0x83}, // index: 94, gain:6.251000DB->x2.053761
	{0x10, 0x84}, // index: 95, gain:6.298000DB->x2.064905
	{0x10, 0x84}, // index: 96, gain:6.345000DB->x2.076108
	{0x10, 0x85}, // index: 97, gain:6.392000DB->x2.087373
	{0x10, 0x86}, // index: 98, gain:6.439000DB->x2.098698
	{0x10, 0x87}, // index: 99, gain:6.486000DB->x2.110085
	{0x10, 0x87}, // index: 100, gain:6.533000DB->x2.121534
	{0x10, 0x88}, // index: 101, gain:6.580000DB->x2.133045
	{0x12, 0x80}, // index: 102, gain:6.627000DB->x2.144618
	{0x12, 0x80}, // index: 103, gain:6.674000DB->x2.156254
	{0x12, 0x81}, // index: 104, gain:6.721000DB->x2.167954
	{0x12, 0x82}, // index: 105, gain:6.768000DB->x2.179716
	{0x12, 0x83}, // index: 106, gain:6.815000DB->x2.191543
	{0x12, 0x83}, // index: 107, gain:6.862000DB->x2.203434
	{0x12, 0x84}, // index: 108, gain:6.909000DB->x2.215389
	{0x12, 0x85}, // index: 109, gain:6.956000DB->x2.227409
	{0x12, 0x85}, // index: 110, gain:7.003000DB->x2.239494
	{0x12, 0x86}, // index: 111, gain:7.050000DB->x2.251645
	{0x12, 0x87}, // index: 112, gain:7.097000DB->x2.263862
	{0x12, 0x88}, // index: 113, gain:7.144000DB->x2.276145
	{0x14, 0x80}, // index: 114, gain:7.191000DB->x2.288495
	{0x14, 0x81}, // index: 115, gain:7.238000DB->x2.300912
	{0x14, 0x81}, // index: 116, gain:7.285000DB->x2.313396
	{0x14, 0x82}, // index: 117, gain:7.332000DB->x2.325948
	{0x14, 0x83}, // index: 118, gain:7.379000DB->x2.338568
	{0x14, 0x84}, // index: 119, gain:7.426000DB->x2.351256
	{0x14, 0x84}, // index: 120, gain:7.473000DB->x2.364014
	{0x14, 0x85}, // index: 121, gain:7.520000DB->x2.376840
	{0x14, 0x86}, // index: 122, gain:7.567000DB->x2.389736
	{0x14, 0x86}, // index: 123, gain:7.614000DB->x2.402702
	{0x14, 0x87}, // index: 124, gain:7.661000DB->x2.415739
	{0x14, 0x88}, // index: 125, gain:7.708000DB->x2.428846
	{0x14, 0x89}, // index: 126, gain:7.755000DB->x2.442024
	{0x14, 0x89}, // index: 127, gain:7.802000DB->x2.455274
	{0x14, 0x8a}, // index: 128, gain:7.849000DB->x2.468596
	{0x16, 0x80}, // index: 129, gain:7.896000DB->x2.481990
	{0x16, 0x81}, // index: 130, gain:7.943000DB->x2.495456
	{0x16, 0x82}, // index: 131, gain:7.990000DB->x2.508996
	{0x16, 0x82}, // index: 132, gain:8.037000DB->x2.522609
	{0x16, 0x83}, // index: 133, gain:8.084000DB->x2.536296
	{0x16, 0x84}, // index: 134, gain:8.131000DB->x2.550058
	{0x16, 0x84}, // index: 135, gain:8.178000DB->x2.563894
	{0x16, 0x85}, // index: 136, gain:8.225000DB->x2.577805
	{0x16, 0x86}, // index: 137, gain:8.272000DB->x2.591791
	{0x16, 0x87}, // index: 138, gain:8.319000DB->x2.605854
	{0x16, 0x87}, // index: 139, gain:8.366000DB->x2.619992
	{0x16, 0x88}, // index: 140, gain:8.413000DB->x2.634208
	{0x16, 0x89}, // index: 141, gain:8.460000DB->x2.648500
	{0x16, 0x89}, // index: 142, gain:8.507000DB->x2.662870
	{0x18, 0x80}, // index: 143, gain:8.554000DB->x2.677318
	{0x18, 0x81}, // index: 144, gain:8.601000DB->x2.691845
	{0x18, 0x81}, // index: 145, gain:8.648000DB->x2.706450
	{0x18, 0x82}, // index: 146, gain:8.695000DB->x2.721134
	{0x18, 0x83}, // index: 147, gain:8.742000DB->x2.735899
	{0x18, 0x83}, // index: 148, gain:8.789000DB->x2.750743
	{0x18, 0x84}, // index: 149, gain:8.836000DB->x2.765668
	{0x18, 0x85}, // index: 150, gain:8.883000DB->x2.780674
	{0x18, 0x86}, // index: 151, gain:8.930000DB->x2.795761
	{0x18, 0x86}, // index: 152, gain:8.977000DB->x2.810930
	{0x18, 0x87}, // index: 153, gain:9.024000DB->x2.826181
	{0x18, 0x88}, // index: 154, gain:9.071000DB->x2.841515
	{0x18, 0x88}, // index: 155, gain:9.118000DB->x2.856933
	{0x18, 0x89}, // index: 156, gain:9.165000DB->x2.872434
	{0x18, 0x8a}, // index: 157, gain:9.212000DB->x2.888019
	{0x18, 0x8b}, // index: 158, gain:9.259000DB->x2.903688
	{0x1a, 0x80}, // index: 159, gain:9.306000DB->x2.919443
	{0x1a, 0x81}, // index: 160, gain:9.353000DB->x2.935283
	{0x1a, 0x81}, // index: 161, gain:9.400000DB->x2.951209
	{0x1a, 0x82}, // index: 162, gain:9.447000DB->x2.967222
	{0x1a, 0x83}, // index: 163, gain:9.494000DB->x2.983321
	{0x1a, 0x83}, // index: 164, gain:9.541000DB->x2.999508
	{0x1a, 0x84}, // index: 165, gain:9.588000DB->x3.015782
	{0x1a, 0x85}, // index: 166, gain:9.635000DB->x3.032145
	{0x1a, 0x86}, // index: 167, gain:9.682000DB->x3.048597
	{0x1a, 0x86}, // index: 168, gain:9.729000DB->x3.065138
	{0x1a, 0x87}, // index: 169, gain:9.776000DB->x3.081768
	{0x1a, 0x88}, // index: 170, gain:9.823000DB->x3.098489
	{0x1a, 0x89}, // index: 171, gain:9.870000DB->x3.115301
	{0x1a, 0x89}, // index: 172, gain:9.917000DB->x3.132204
	{0x1a, 0x8a}, // index: 173, gain:9.964000DB->x3.149198
	{0x1a, 0x8b}, // index: 174, gain:10.011000DB->x3.166285
	{0x1a, 0x8c}, // index: 175, gain:10.058000DB->x3.183464
	{0x1c, 0x80}, // index: 176, gain:10.105000DB->x3.200737
	{0x1c, 0x80}, // index: 177, gain:10.152000DB->x3.218103
	{0x1c, 0x81}, // index: 178, gain:10.199000DB->x3.235564
	{0x1c, 0x82}, // index: 179, gain:10.246000DB->x3.253119
	{0x1c, 0x82}, // index: 180, gain:10.293000DB->x3.270770
	{0x1c, 0x83}, // index: 181, gain:10.340000DB->x3.288516
	{0x1c, 0x84}, // index: 182, gain:10.387000DB->x3.306359
	{0x1c, 0x84}, // index: 183, gain:10.434000DB->x3.324298
	{0x1c, 0x85}, // index: 184, gain:10.481000DB->x3.342335
	{0x1c, 0x86}, // index: 185, gain:10.528000DB->x3.360470
	{0x1c, 0x87}, // index: 186, gain:10.575000DB->x3.378703
	{0x1c, 0x87}, // index: 187, gain:10.622000DB->x3.397035
	{0x1c, 0x88}, // index: 188, gain:10.669000DB->x3.415466
	{0x1c, 0x89}, // index: 189, gain:10.716000DB->x3.433998
	{0x1c, 0x8a}, // index: 190, gain:10.763000DB->x3.452630
	{0x1c, 0x8a}, // index: 191, gain:10.810000DB->x3.471363
	{0x1c, 0x8b}, // index: 192, gain:10.857000DB->x3.490197
	{0x1c, 0x8c}, // index: 193, gain:10.904000DB->x3.509134
	{0x1c, 0x8d}, // index: 194, gain:10.951000DB->x3.528174
	{0x1c, 0x8d}, // index: 195, gain:10.998000DB->x3.547317
	{0x1e, 0x80}, // index: 196, gain:11.045000DB->x3.566564
	{0x1e, 0x80}, // index: 197, gain:11.092000DB->x3.585915
	{0x1e, 0x81}, // index: 198, gain:11.139000DB->x3.605371
	{0x1e, 0x82}, // index: 199, gain:11.186000DB->x3.624933
	{0x1e, 0x83}, // index: 200, gain:11.233000DB->x3.644601
	{0x1e, 0x83}, // index: 201, gain:11.280000DB->x3.664376
	{0x1e, 0x84}, // index: 202, gain:11.327000DB->x3.684258
	{0x1e, 0x85}, // index: 203, gain:11.374000DB->x3.704248
	{0x1e, 0x85}, // index: 204, gain:11.421000DB->x3.724346
	{0x1e, 0x86}, // index: 205, gain:11.468000DB->x3.744553
	{0x1e, 0x87}, // index: 206, gain:11.515000DB->x3.764870
	{0x1e, 0x88}, // index: 207, gain:11.562000DB->x3.785297
	{0x1e, 0x88}, // index: 208, gain:11.609000DB->x3.805835
	{0x1e, 0x89}, // index: 209, gain:11.656000DB->x3.826485
	{0x1e, 0x8a}, // index: 210, gain:11.703000DB->x3.847246
	{0x1e, 0x8b}, // index: 211, gain:11.750000DB->x3.868121
	{0x1e, 0x8b}, // index: 212, gain:11.797000DB->x3.889108
	{0x1e, 0x8c}, // index: 213, gain:11.844000DB->x3.910209
	{0x1e, 0x8d}, // index: 214, gain:11.891000DB->x3.931425
	{0x1e, 0x8e}, // index: 215, gain:11.938000DB->x3.952756
	{0x1e, 0x8e}, // index: 216, gain:11.985000DB->x3.974203
	{0x1e, 0x8f}, // index: 217, gain:12.032000DB->x3.995766
	{0x20, 0x80}, // index: 218, gain:12.079000DB->x4.017446
	{0x20, 0x81}, // index: 219, gain:12.126000DB->x4.039243
	{0x20, 0x81}, // index: 220, gain:12.173000DB->x4.061159
	{0x20, 0x82}, // index: 221, gain:12.220000DB->x4.083194
	{0x20, 0x83}, // index: 222, gain:12.267000DB->x4.105348
	{0x20, 0x84}, // index: 223, gain:12.314000DB->x4.127623
	{0x20, 0x84}, // index: 224, gain:12.361000DB->x4.150018
	{0x20, 0x85}, // index: 225, gain:12.408000DB->x4.172535
	{0x20, 0x86}, // index: 226, gain:12.455000DB->x4.195174
	{0x20, 0x86}, // index: 227, gain:12.502000DB->x4.217936
	{0x20, 0x87}, // index: 228, gain:12.549000DB->x4.240822
	{0x20, 0x88}, // index: 229, gain:12.596000DB->x4.263831
	{0x20, 0x89}, // index: 230, gain:12.643000DB->x4.286966
	{0x20, 0x89}, // index: 231, gain:12.690000DB->x4.310226
	{0x20, 0x8a}, // index: 232, gain:12.737000DB->x4.333612
	{0x20, 0x8b}, // index: 233, gain:12.784000DB->x4.357125
	{0x20, 0x8c}, // index: 234, gain:12.831000DB->x4.380765
	{0x20, 0x8c}, // index: 235, gain:12.878000DB->x4.404534
	{0x20, 0x8d}, // index: 236, gain:12.925000DB->x4.428432
	{0x20, 0x8e}, // index: 237, gain:12.972000DB->x4.452460
	{0x20, 0x8f}, // index: 238, gain:13.019000DB->x4.476618
	{0x20, 0x90}, // index: 239, gain:13.066000DB->x4.500907
	{0x20, 0x90}, // index: 240, gain:13.113000DB->x4.525327
	{0x20, 0x91}, // index: 241, gain:13.160000DB->x4.549881
	{0x24, 0x80}, // index: 242, gain:13.207000DB->x4.574567
	{0x24, 0x81}, // index: 243, gain:13.254000DB->x4.599387
	{0x24, 0x81}, // index: 244, gain:13.301000DB->x4.624343
	{0x24, 0x82}, // index: 245, gain:13.348000DB->x4.649433
	{0x24, 0x83}, // index: 246, gain:13.395000DB->x4.674660
	{0x24, 0x83}, // index: 247, gain:13.442000DB->x4.700023
	{0x24, 0x84}, // index: 248, gain:13.489000DB->x4.725524
	{0x24, 0x85}, // index: 249, gain:13.536000DB->x4.751164
	{0x24, 0x86}, // index: 250, gain:13.583000DB->x4.776942
	{0x24, 0x86}, // index: 251, gain:13.630000DB->x4.802861
	{0x24, 0x87}, // index: 252, gain:13.677000DB->x4.828920
	{0x24, 0x88}, // index: 253, gain:13.724000DB->x4.855120
	{0x24, 0x89}, // index: 254, gain:13.771000DB->x4.881463
	{0x24, 0x89}, // index: 255, gain:13.818000DB->x4.907949
	{0x24, 0x8a}, // index: 256, gain:13.865000DB->x4.934578
	{0x24, 0x8b}, // index: 257, gain:13.912000DB->x4.961352
	{0x24, 0x8c}, // index: 258, gain:13.959000DB->x4.988271
	{0x24, 0x8c}, // index: 259, gain:14.006000DB->x5.015336
	{0x24, 0x8d}, // index: 260, gain:14.053000DB->x5.042548
	{0x24, 0x8e}, // index: 261, gain:14.100000DB->x5.069907
	{0x24, 0x8f}, // index: 262, gain:14.147000DB->x5.097415
	{0x24, 0x8f}, // index: 263, gain:14.194000DB->x5.125072
	{0x24, 0x90}, // index: 264, gain:14.241000DB->x5.152880
	{0x24, 0x91}, // index: 265, gain:14.288000DB->x5.180838
	{0x24, 0x92}, // index: 266, gain:14.335000DB->x5.208948
	{0x24, 0x93}, // index: 267, gain:14.382000DB->x5.237210
	{0x24, 0x93}, // index: 268, gain:14.429000DB->x5.265626
	{0x24, 0x94}, // index: 269, gain:14.476000DB->x5.294196
	{0x24, 0x95}, // index: 270, gain:14.523000DB->x5.322921
	{0x28, 0x80}, // index: 271, gain:14.570000DB->x5.351802
	{0x28, 0x80}, // index: 272, gain:14.617000DB->x5.380839
	{0x28, 0x81}, // index: 273, gain:14.664000DB->x5.410034
	{0x28, 0x82}, // index: 274, gain:14.711000DB->x5.439388
	{0x28, 0x83}, // index: 275, gain:14.758000DB->x5.468900
	{0x28, 0x83}, // index: 276, gain:14.805000DB->x5.498573
	{0x28, 0x84}, // index: 277, gain:14.852000DB->x5.528407
	{0x28, 0x85}, // index: 278, gain:14.899000DB->x5.558403
	{0x28, 0x85}, // index: 279, gain:14.946000DB->x5.588561
	{0x28, 0x86}, // index: 280, gain:14.993000DB->x5.618883
	{0x28, 0x87}, // index: 281, gain:15.040000DB->x5.649370
	{0x28, 0x88}, // index: 282, gain:15.087000DB->x5.680022
	{0x28, 0x88}, // index: 283, gain:15.134000DB->x5.710840
	{0x28, 0x89}, // index: 284, gain:15.181000DB->x5.741826
	{0x28, 0x8a}, // index: 285, gain:15.228000DB->x5.772979
	{0x28, 0x8b}, // index: 286, gain:15.275000DB->x5.804302
	{0x28, 0x8b}, // index: 287, gain:15.322000DB->x5.835795
	{0x28, 0x8c}, // index: 288, gain:15.369000DB->x5.867458
	{0x28, 0x8d}, // index: 289, gain:15.416000DB->x5.899293
	{0x28, 0x8e}, // index: 290, gain:15.463000DB->x5.931301
	{0x28, 0x8e}, // index: 291, gain:15.510000DB->x5.963483
	{0x28, 0x8f}, // index: 292, gain:15.557000DB->x5.995840
	{0x28, 0x90}, // index: 293, gain:15.604000DB->x6.028371
	{0x28, 0x91}, // index: 294, gain:15.651000DB->x6.061080
	{0x28, 0x92}, // index: 295, gain:15.698000DB->x6.093966
	{0x28, 0x92}, // index: 296, gain:15.745000DB->x6.127030
	{0x28, 0x93}, // index: 297, gain:15.792000DB->x6.160274
	{0x28, 0x94}, // index: 298, gain:15.839000DB->x6.193698
	{0x28, 0x95}, // index: 299, gain:15.886000DB->x6.227303
	{0x28, 0x96}, // index: 300, gain:15.933000DB->x6.261091
	{0x28, 0x96}, // index: 301, gain:15.980000DB->x6.295062
	{0x28, 0x97}, // index: 302, gain:16.027000DB->x6.329217
	{0x28, 0x98}, // index: 303, gain:16.074000DB->x6.363558
	{0x28, 0x99}, // index: 304, gain:16.121000DB->x6.398085
	{0x2c, 0x80}, // index: 305, gain:16.168000DB->x6.432799
	{0x2c, 0x81}, // index: 306, gain:16.215000DB->x6.467702
	{0x2c, 0x81}, // index: 307, gain:16.262000DB->x6.502794
	{0x2c, 0x82}, // index: 308, gain:16.309000DB->x6.538077
	{0x2c, 0x83}, // index: 309, gain:16.356000DB->x6.573550
	{0x2c, 0x83}, // index: 310, gain:16.403000DB->x6.609217
	{0x2c, 0x84}, // index: 311, gain:16.450000DB->x6.645077
	{0x2c, 0x85}, // index: 312, gain:16.497000DB->x6.681131
	{0x2c, 0x86}, // index: 313, gain:16.544000DB->x6.717381
	{0x2c, 0x86}, // index: 314, gain:16.591000DB->x6.753828
	{0x2c, 0x87}, // index: 315, gain:16.638000DB->x6.790473
	{0x2c, 0x88}, // index: 316, gain:16.685000DB->x6.827316
	{0x2c, 0x89}, // index: 317, gain:16.732000DB->x6.864359
	{0x2c, 0x89}, // index: 318, gain:16.779000DB->x6.901603
	{0x2c, 0x8a}, // index: 319, gain:16.826000DB->x6.939050
	{0x2c, 0x8b}, // index: 320, gain:16.873000DB->x6.976699
	{0x2c, 0x8c}, // index: 321, gain:16.920000DB->x7.014553
	{0x2c, 0x8c}, // index: 322, gain:16.967000DB->x7.052612
	{0x2c, 0x8d}, // index: 323, gain:17.014000DB->x7.090878
	{0x2c, 0x8e}, // index: 324, gain:17.061000DB->x7.129351
	{0x2c, 0x8f}, // index: 325, gain:17.108000DB->x7.168033
	{0x2c, 0x8f}, // index: 326, gain:17.155000DB->x7.206925
	{0x2c, 0x90}, // index: 327, gain:17.202000DB->x7.246028
	{0x2c, 0x91}, // index: 328, gain:17.249000DB->x7.285343
	{0x2c, 0x92}, // index: 329, gain:17.296000DB->x7.324871
	{0x2c, 0x93}, // index: 330, gain:17.343000DB->x7.364614
	{0x2c, 0x93}, // index: 331, gain:17.390000DB->x7.404573
	{0x2c, 0x94}, // index: 332, gain:17.437000DB->x7.444748
	{0x2c, 0x95}, // index: 333, gain:17.484000DB->x7.485141
	{0x2c, 0x96}, // index: 334, gain:17.531000DB->x7.525754
	{0x2c, 0x97}, // index: 335, gain:17.578000DB->x7.566586
	{0x2c, 0x97}, // index: 336, gain:17.625000DB->x7.607641
	{0x2c, 0x98}, // index: 337, gain:17.672000DB->x7.648918
	{0x2c, 0x99}, // index: 338, gain:17.719000DB->x7.690419
	{0x2c, 0x9a}, // index: 339, gain:17.766000DB->x7.732145
	{0x2c, 0x9b}, // index: 340, gain:17.813000DB->x7.774098
	{0x2c, 0x9c}, // index: 341, gain:17.860000DB->x7.816278
	{0x2c, 0x9c}, // index: 342, gain:17.907000DB->x7.858687
	{0x2c, 0x9d}, // index: 343, gain:17.954000DB->x7.901326
	{0x2c, 0x9e}, // index: 344, gain:18.001000DB->x7.944197
	{0x2c, 0x9f}, // index: 345, gain:18.048000DB->x7.987300
	{0x30, 0x80}, // index: 346, gain:18.095000DB->x8.030637
	{0x30, 0x81}, // index: 347, gain:18.142000DB->x8.074209
	{0x30, 0x81}, // index: 348, gain:18.189000DB->x8.118018
	{0x30, 0x82}, // index: 349, gain:18.236000DB->x8.162064
	{0x30, 0x83}, // index: 350, gain:18.283000DB->x8.206349
	{0x30, 0x84}, // index: 351, gain:18.330000DB->x8.250875
	{0x30, 0x84}, // index: 352, gain:18.377000DB->x8.295642
	{0x30, 0x85}, // index: 353, gain:18.424000DB->x8.340652
	{0x30, 0x86}, // index: 354, gain:18.471000DB->x8.385906
	{0x30, 0x86}, // index: 355, gain:18.518000DB->x8.431406
	{0x30, 0x87}, // index: 356, gain:18.565000DB->x8.477153
	{0x30, 0x88}, // index: 357, gain:18.612000DB->x8.523147
	{0x30, 0x89}, // index: 358, gain:18.659000DB->x8.569392
	{0x30, 0x89}, // index: 359, gain:18.706000DB->x8.615887
	{0x30, 0x8a}, // index: 360, gain:18.753000DB->x8.662635
	{0x30, 0x8b}, // index: 361, gain:18.800000DB->x8.709636
	{0x30, 0x8c}, // index: 362, gain:18.847000DB->x8.756892
	{0x30, 0x8c}, // index: 363, gain:18.894000DB->x8.804405
	{0x30, 0x8d}, // index: 364, gain:18.941000DB->x8.852175
	{0x30, 0x8e}, // index: 365, gain:18.988000DB->x8.900205
	{0x30, 0x8f}, // index: 366, gain:19.035000DB->x8.948495
	{0x30, 0x8f}, // index: 367, gain:19.082000DB->x8.997047
	{0x30, 0x90}, // index: 368, gain:19.129000DB->x9.045863
	{0x30, 0x91}, // index: 369, gain:19.176000DB->x9.094943
	{0x30, 0x92}, // index: 370, gain:19.223000DB->x9.144290
	{0x30, 0x93}, // index: 371, gain:19.270000DB->x9.193905
	{0x30, 0x93}, // index: 372, gain:19.317000DB->x9.243788
	{0x30, 0x94}, // index: 373, gain:19.364000DB->x9.293943
	{0x30, 0x95}, // index: 374, gain:19.411000DB->x9.344369
	{0x30, 0x96}, // index: 375, gain:19.458000DB->x9.395070
	{0x30, 0x97}, // index: 376, gain:19.505000DB->x9.446045
	{0x30, 0x97}, // index: 377, gain:19.552000DB->x9.497297
	{0x30, 0x98}, // index: 378, gain:19.599000DB->x9.548826
	{0x30, 0x99}, // index: 379, gain:19.646000DB->x9.600636
	{0x30, 0x9a}, // index: 380, gain:19.693000DB->x9.652726
	{0x30, 0x9b}, // index: 381, gain:19.740000DB->x9.705100
	{0x30, 0x9c}, // index: 382, gain:19.787000DB->x9.757757
	{0x30, 0x9c}, // index: 383, gain:19.834000DB->x9.810700
	{0x30, 0x9d}, // index: 384, gain:19.881000DB->x9.863930
	{0x30, 0x9e}, // index: 385, gain:19.928000DB->x9.917450
	{0x30, 0x9f}, // index: 386, gain:19.975000DB->x9.971259
	{0x30, 0xa0}, // index: 387, gain:20.022000DB->x10.025361
	{0x30, 0xa1}, // index: 388, gain:20.069000DB->x10.079756
	{0x30, 0xa2}, // index: 389, gain:20.116000DB->x10.134446
	{0x30, 0xa3}, // index: 390, gain:20.163000DB->x10.189433
	{0x30, 0xa3}, // index: 391, gain:20.210000DB->x10.244718
	{0x30, 0xa4}, // index: 392, gain:20.257000DB->x10.300303
	{0x30, 0xa5}, // index: 393, gain:20.304000DB->x10.356190
	{0x30, 0xa6}, // index: 394, gain:20.351000DB->x10.412380
	{0x30, 0xa7}, // index: 395, gain:20.398000DB->x10.468875
	{0x30, 0xa8}, // index: 396, gain:20.445000DB->x10.525676
	{0x30, 0xa9}, // index: 397, gain:20.492000DB->x10.582786
	{0x30, 0xaa}, // index: 398, gain:20.539000DB->x10.640205
	{0x30, 0xab}, // index: 399, gain:20.586000DB->x10.697936
	{0x30, 0xac}, // index: 400, gain:20.633000DB->x10.755980
	{0x30, 0xad}, // index: 401, gain:20.680000DB->x10.814340
	{0x30, 0xad}, // index: 402, gain:20.727000DB->x10.873015
	{0x30, 0xae}, // index: 403, gain:20.774000DB->x10.932009
	{0x30, 0xaf}, // index: 404, gain:20.821000DB->x10.991324
	{0x30, 0xb0}, // index: 405, gain:20.868000DB->x11.050960
	{0x30, 0xb1}, // index: 406, gain:20.915000DB->x11.110919
	{0x30, 0xb2}, // index: 407, gain:20.962000DB->x11.171204
	{0x30, 0xb3}, // index: 408, gain:21.009000DB->x11.231817
	{0x30, 0xb4}, // index: 409, gain:21.056000DB->x11.292757
	{0x30, 0xb5}, // index: 410, gain:21.103000DB->x11.354029
	{0x30, 0xb6}, // index: 411, gain:21.150000DB->x11.415633
	{0x30, 0xb7}, // index: 412, gain:21.197000DB->x11.477571
	{0x30, 0xb8}, // index: 413, gain:21.244000DB->x11.539846
	{0x30, 0xb9}, // index: 414, gain:21.291000DB->x11.602458
	{0x30, 0xba}, // index: 415, gain:21.338000DB->x11.665410
	{0x30, 0xbb}, // index: 416, gain:21.385000DB->x11.728703
	{0x30, 0xbc}, // index: 417, gain:21.432000DB->x11.792340
	{0x30, 0xbd}, // index: 418, gain:21.479000DB->x11.856322
	{0x30, 0xbe}, // index: 419, gain:21.526000DB->x11.920652
	{0x30, 0xbf}, // index: 420, gain:21.573000DB->x11.985330
	{0x30, 0xc0}, // index: 421, gain:21.620000DB->x12.050359
	{0x30, 0xc1}, // index: 422, gain:21.667000DB->x12.115742
	{0x30, 0xc2}, // index: 423, gain:21.714000DB->x12.181478
	{0x30, 0xc3}, // index: 424, gain:21.761000DB->x12.247572
	{0x30, 0xc5}, // index: 425, gain:21.808000DB->x12.314024
	{0x30, 0xc6}, // index: 426, gain:21.855000DB->x12.380837
	{0x30, 0xc7}, // index: 427, gain:21.902000DB->x12.448012
	{0x30, 0xc8}, // index: 428, gain:21.949000DB->x12.515552
	{0x30, 0xc9}, // index: 429, gain:21.996000DB->x12.583458
	{0x30, 0xca}, // index: 430, gain:22.043000DB->x12.651732
	{0x30, 0xcb}, // index: 431, gain:22.090000DB->x12.720378
	{0x30, 0xcc}, // index: 432, gain:22.137000DB->x12.789395
	{0x30, 0xcd}, // index: 433, gain:22.184000DB->x12.858787
	{0x30, 0xce}, // index: 434, gain:22.231000DB->x12.928555
	{0x30, 0xcf}, // index: 435, gain:22.278000DB->x12.998702
	{0x30, 0xd1}, // index: 436, gain:22.325000DB->x13.069230
	{0x30, 0xd2}, // index: 437, gain:22.372000DB->x13.140140
	{0x30, 0xd3}, // index: 438, gain:22.419000DB->x13.211435
	{0x30, 0xd4}, // index: 439, gain:22.466000DB->x13.283117
	{0x30, 0xd5}, // index: 440, gain:22.513000DB->x13.355188
	{0x30, 0xd6}, // index: 441, gain:22.560000DB->x13.427650
	{0x30, 0xd8}, // index: 442, gain:22.607000DB->x13.500505
	{0x30, 0xd9}, // index: 443, gain:22.654000DB->x13.573755
	{0x30, 0xda}, // index: 444, gain:22.701000DB->x13.647402
	{0x30, 0xdb}, // index: 445, gain:22.748000DB->x13.721450
	{0x30, 0xdc}, // index: 446, gain:22.795000DB->x13.795899
	{0x30, 0xdd}, // index: 447, gain:22.842000DB->x13.870752
	{0x30, 0xdf}, // index: 448, gain:22.889000DB->x13.946011
	{0x30, 0xe0}, // index: 449, gain:22.936000DB->x14.021678
	{0x30, 0xe1}, // index: 450, gain:22.983000DB->x14.097756
	{0x30, 0xe2}, // index: 451, gain:23.030000DB->x14.174247
	{0x30, 0xe4}, // index: 452, gain:23.077000DB->x14.251153
	{0x30, 0xe5}, // index: 453, gain:23.124000DB->x14.328476
	{0x30, 0xe6}, // index: 454, gain:23.171000DB->x14.406219
	{0x30, 0xe7}, // index: 455, gain:23.218000DB->x14.484383
	{0x30, 0xe9}, // index: 456, gain:23.265000DB->x14.562972
	{0x30, 0xea}, // index: 457, gain:23.312000DB->x14.641986
	{0x30, 0xeb}, // index: 458, gain:23.359000DB->x14.721430
	{0x30, 0xec}, // index: 459, gain:23.406000DB->x14.801305
	{0x30, 0xee}, // index: 460, gain:23.453000DB->x14.881613
	{0x30, 0xef}, // index: 461, gain:23.500000DB->x14.962357
	{0x30, 0xf0}, // index: 462, gain:23.547000DB->x15.043538
	{0x30, 0xf2}, // index: 463, gain:23.594000DB->x15.125161
	{0x30, 0xf3}, // index: 464, gain:23.641000DB->x15.207226
	{0x30, 0xf4}, // index: 465, gain:23.688000DB->x15.289736
	{0x30, 0xf5}, // index: 466, gain:23.735000DB->x15.372695
	{0x30, 0xf7}, // index: 467, gain:23.782000DB->x15.456103
	{0x30, 0xf8}, // index: 468, gain:23.829000DB->x15.539964
	{0x30, 0xf9}, // index: 469, gain:23.876000DB->x15.624280
	{0x30, 0xfb}, // index: 470, gain:23.923000DB->x15.709053
	{0x30, 0xfc}, // index: 471, gain:23.970000DB->x15.794286
	{0x30, 0xfe}, // index: 472, gain:24.017000DB->x15.879982
	{0x30, 0xff}, // index: 473, gain:24.064000DB->x15.966142
	{0x30, 0x100}, // index: 474, gain:24.111000DB->x16.052771
	{0x30, 0x102}, // index: 475, gain:24.158000DB->x16.139869
	{0x30, 0x103}, // index: 476, gain:24.205000DB->x16.227440
	{0x30, 0x105}, // index: 477, gain:24.252000DB->x16.315485
	{0x30, 0x106}, // index: 478, gain:24.299000DB->x16.404009
	{0x30, 0x107}, // index: 479, gain:24.346000DB->x16.493013
	{0x30, 0x109}, // index: 480, gain:24.393000DB->x16.582500
	{0x30, 0x10a}, // index: 481, gain:24.440000DB->x16.672472
	{0x30, 0x10c}, // index: 482, gain:24.487000DB->x16.762933
	{0x30, 0x10d}, // index: 483, gain:24.534000DB->x16.853884
	{0x30, 0x10f}, // index: 484, gain:24.581000DB->x16.945329
	{0x30, 0x110}, // index: 485, gain:24.628000DB->x17.037270
	{0x30, 0x112}, // index: 486, gain:24.675000DB->x17.129710
	{0x30, 0x113}, // index: 487, gain:24.722000DB->x17.222651
	{0x30, 0x115}, // index: 488, gain:24.769000DB->x17.316097
	{0x30, 0x116}, // index: 489, gain:24.816000DB->x17.410049
	{0x30, 0x118}, // index: 490, gain:24.863000DB->x17.504512
	{0x30, 0x119}, // index: 491, gain:24.910000DB->x17.599487
	{0x30, 0x11b}, // index: 492, gain:24.957000DB->x17.694977
	{0x30, 0x11c}, // index: 493, gain:25.004000DB->x17.790985
	{0x30, 0x11e}, // index: 494, gain:25.051000DB->x17.887515
	{0x30, 0x11f}, // index: 495, gain:25.098000DB->x17.984568
	{0x30, 0x121}, // index: 496, gain:25.145000DB->x18.082147
	{0x30, 0x122}, // index: 497, gain:25.192000DB->x18.180256
	{0x30, 0x124}, // index: 498, gain:25.239000DB->x18.278898
	{0x30, 0x126}, // index: 499, gain:25.286000DB->x18.378074
	{0x30, 0x127}, // index: 500, gain:25.333000DB->x18.477789
	{0x30, 0x129}, // index: 501, gain:25.380000DB->x18.578045
	{0x30, 0x12a}, // index: 502, gain:25.427000DB->x18.678844
	{0x30, 0x12c}, // index: 503, gain:25.474000DB->x18.780191
	{0x30, 0x12e}, // index: 504, gain:25.521000DB->x18.882087
	{0x30, 0x12f}, // index: 505, gain:25.568000DB->x18.984537
	{0x30, 0x131}, // index: 506, gain:25.615000DB->x19.087542
	{0x30, 0x133}, // index: 507, gain:25.662000DB->x19.191106
	{0x30, 0x134}, // index: 508, gain:25.709000DB->x19.295232
	{0x30, 0x136}, // index: 509, gain:25.756000DB->x19.399923
	{0x30, 0x138}, // index: 510, gain:25.803000DB->x19.505182
	{0x30, 0x139}, // index: 511, gain:25.850000DB->x19.611012
	{0x30, 0x13b}, // index: 512, gain:25.897000DB->x19.717416
	{0x30, 0x13d}, // index: 513, gain:25.944000DB->x19.824398
	{0x30, 0x13e}, // index: 514, gain:25.991000DB->x19.931960
	{0x30, 0x140}, // index: 515, gain:26.038000DB->x20.040105
	{0x30, 0x142}, // index: 516, gain:26.085000DB->x20.148838
	{0x30, 0x144}, // index: 517, gain:26.132000DB->x20.258160
	{0x30, 0x145}, // index: 518, gain:26.179000DB->x20.368076
	{0x30, 0x147}, // index: 519, gain:26.226000DB->x20.478588
	{0x30, 0x149}, // index: 520, gain:26.273000DB->x20.589699
	{0x30, 0x14b}, // index: 521, gain:26.320000DB->x20.701413
	{0x30, 0x14d}, // index: 522, gain:26.367000DB->x20.813734
	{0x30, 0x14e}, // index: 523, gain:26.414000DB->x20.926664
	{0x30, 0x150}, // index: 524, gain:26.461000DB->x21.040207
	{0x30, 0x152}, // index: 525, gain:26.508000DB->x21.154365
	{0x30, 0x154}, // index: 526, gain:26.555000DB->x21.269143
	{0x30, 0x156}, // index: 527, gain:26.602000DB->x21.384544
	{0x30, 0x158}, // index: 528, gain:26.649000DB->x21.500571
	{0x30, 0x159}, // index: 529, gain:26.696000DB->x21.617228
	{0x30, 0x15b}, // index: 530, gain:26.743000DB->x21.734517
	{0x30, 0x15d}, // index: 531, gain:26.790000DB->x21.852443
	{0x30, 0x15f}, // index: 532, gain:26.837000DB->x21.971009
	{0x30, 0x161}, // index: 533, gain:26.884000DB->x22.090218
	{0x30, 0x163}, // index: 534, gain:26.931000DB->x22.210074
	{0x30, 0x165}, // index: 535, gain:26.978000DB->x22.330580
	{0x30, 0x167}, // index: 536, gain:27.025000DB->x22.451740
	{0x30, 0x169}, // index: 537, gain:27.072000DB->x22.573557
	{0x30, 0x16b}, // index: 538, gain:27.119000DB->x22.696035
	{0x30, 0x16d}, // index: 539, gain:27.166000DB->x22.819178
	{0x30, 0x16f}, // index: 540, gain:27.213000DB->x22.942989
	{0x30, 0x171}, // index: 541, gain:27.260000DB->x23.067472
	{0x30, 0x173}, // index: 542, gain:27.307000DB->x23.192630
	{0x30, 0x175}, // index: 543, gain:27.354000DB->x23.318467
	{0x30, 0x177}, // index: 544, gain:27.401000DB->x23.444987
	{0x30, 0x179}, // index: 545, gain:27.448000DB->x23.572194
	{0x30, 0x17b}, // index: 546, gain:27.495000DB->x23.700090
	{0x30, 0x17d}, // index: 547, gain:27.542000DB->x23.828681
	{0x30, 0x17f}, // index: 548, gain:27.589000DB->x23.957969
	{0x30, 0x181}, // index: 549, gain:27.636000DB->x24.087959
	{0x30, 0x183}, // index: 550, gain:27.683000DB->x24.218654
	{0x30, 0x185}, // index: 551, gain:27.730000DB->x24.350058
	{0x30, 0x187}, // index: 552, gain:27.777000DB->x24.482175
	{0x30, 0x189}, // index: 553, gain:27.824000DB->x24.615009
	{0x30, 0x18b}, // index: 554, gain:27.871000DB->x24.748564
	{0x30, 0x18e}, // index: 555, gain:27.918000DB->x24.882843
	{0x30, 0x190}, // index: 556, gain:27.965000DB->x25.017851
	{0x30, 0x192}, // index: 557, gain:28.012000DB->x25.153591
	{0x30, 0x194}, // index: 558, gain:28.059000DB->x25.290068
	{0x30, 0x196}, // index: 559, gain:28.106000DB->x25.427286
	{0x30, 0x199}, // index: 560, gain:28.153000DB->x25.565247
	{0x30, 0x19b}, // index: 561, gain:28.200000DB->x25.703958
	{0x30, 0x19d}, // index: 562, gain:28.247000DB->x25.843421
	{0x30, 0x19f}, // index: 563, gain:28.294000DB->x25.983641
	{0x30, 0x1a1}, // index: 564, gain:28.341000DB->x26.124621
	{0x30, 0x1a4}, // index: 565, gain:28.388000DB->x26.266367
	{0x30, 0x1a6}, // index: 566, gain:28.435000DB->x26.408881
	{0x30, 0x1a8}, // index: 567, gain:28.482000DB->x26.552169
	{0x30, 0x1ab}, // index: 568, gain:28.529000DB->x26.696234
	{0x30, 0x1ad}, // index: 569, gain:28.576000DB->x26.841081
	{0x30, 0x1af}, // index: 570, gain:28.623000DB->x26.986714
	{0x30, 0x1b2}, // index: 571, gain:28.670000DB->x27.133137
	{0x30, 0x1b4}, // index: 572, gain:28.717000DB->x27.280354
	{0x30, 0x1b6}, // index: 573, gain:28.764000DB->x27.428370
	{0x30, 0x1b9}, // index: 574, gain:28.811000DB->x27.577189
	{0x30, 0x1bb}, // index: 575, gain:28.858000DB->x27.726816
	{0x30, 0x1be}, // index: 576, gain:28.905000DB->x27.877255
	{0x30, 0x1c0}, // index: 577, gain:28.952000DB->x28.028509
	{0x30, 0x1c2}, // index: 578, gain:28.999000DB->x28.180585
	{0x30, 0x1c5}, // index: 579, gain:29.046000DB->x28.333485
	{0x30, 0x1c7}, // index: 580, gain:29.093000DB->x28.487215
	{0x30, 0x1ca}, // index: 581, gain:29.140000DB->x28.641780
	{0x30, 0x1cc}, // index: 582, gain:29.187000DB->x28.797183
	{0x30, 0x1cf}, // index: 583, gain:29.234000DB->x28.953429
	{0x30, 0x1d1}, // index: 584, gain:29.281000DB->x29.110522
	{0x30, 0x1d4}, // index: 585, gain:29.328000DB->x29.268469
	{0x30, 0x1d6}, // index: 586, gain:29.375000DB->x29.427272
	{0x30, 0x1d9}, // index: 587, gain:29.422000DB->x29.586937
	{0x30, 0x1db}, // index: 588, gain:29.469000DB->x29.747468
	{0x30, 0x1de}, // index: 589, gain:29.516000DB->x29.908870
	{0x30, 0x1e1}, // index: 590, gain:29.563000DB->x30.071147
	{0x30, 0x1e3}, // index: 591, gain:29.610000DB->x30.234306
	{0x30, 0x1e6}, // index: 592, gain:29.657000DB->x30.398349
	{0x30, 0x1e9}, // index: 593, gain:29.704000DB->x30.563283
	{0x30, 0x1eb}, // index: 594, gain:29.751000DB->x30.729111
	{0x30, 0x1ee}, // index: 595, gain:29.798000DB->x30.895839
	{0x30, 0x1f1}, // index: 596, gain:29.845000DB->x31.063472
	{0x30, 0x1f3}, // index: 597, gain:29.892000DB->x31.232015
	{0x30, 0x1f6}, // index: 598, gain:29.939000DB->x31.401472
	{0x30, 0x1f9}, // index: 599, gain:29.986000DB->x31.571848
	{0x30, 0x1fb}, // index: 600, gain:30.033000DB->x31.743148
	{0x30, 0x1fe}, // index: 601, gain:30.080000DB->x31.915379
	{0x30, 0x201}, // index: 602, gain:30.127000DB->x32.088543
	{0x30, 0x204}, // index: 603, gain:30.174000DB->x32.262647
	{0x30, 0x207}, // index: 604, gain:30.221000DB->x32.437696
	{0x30, 0x209}, // index: 605, gain:30.268000DB->x32.613695
	{0x30, 0x20c}, // index: 606, gain:30.315000DB->x32.790648
	{0x30, 0x20f}, // index: 607, gain:30.362000DB->x32.968562
	{0x30, 0x212}, // index: 608, gain:30.409000DB->x33.147441
	{0x30, 0x215}, // index: 609, gain:30.456000DB->x33.327290
	{0x30, 0x218}, // index: 610, gain:30.503000DB->x33.508115
	{0x30, 0x21b}, // index: 611, gain:30.550000DB->x33.689922
	{0x30, 0x21d}, // index: 612, gain:30.597000DB->x33.872714
	{0x30, 0x220}, // index: 613, gain:30.644000DB->x34.056499
	{0x30, 0x223}, // index: 614, gain:30.691000DB->x34.241281
	{0x30, 0x226}, // index: 615, gain:30.738000DB->x34.427065
	{0x30, 0x229}, // index: 616, gain:30.785000DB->x34.613857
	{0x30, 0x22c}, // index: 617, gain:30.832000DB->x34.801663
	{0x30, 0x22f}, // index: 618, gain:30.879000DB->x34.990488
	{0x30, 0x232}, // index: 619, gain:30.926000DB->x35.180337
	{0x30, 0x235}, // index: 620, gain:30.973000DB->x35.371217
	{0x30, 0x239}, // index: 621, gain:31.020000DB->x35.563132
	{0x30, 0x23c}, // index: 622, gain:31.067000DB->x35.756088
	{0x30, 0x23f}, // index: 623, gain:31.114000DB->x35.950091
	{0x30, 0x242}, // index: 624, gain:31.161000DB->x36.145147
	{0x30, 0x245}, // index: 625, gain:31.208000DB->x36.341262
	{0x30, 0x248}, // index: 626, gain:31.255000DB->x36.538440
	{0x30, 0x24b}, // index: 627, gain:31.302000DB->x36.736688
	{0x30, 0x24e}, // index: 628, gain:31.349000DB->x36.936012
	{0x30, 0x252}, // index: 629, gain:31.396000DB->x37.136417
	{0x30, 0x255}, // index: 630, gain:31.443000DB->x37.337910
	{0x30, 0x258}, // index: 631, gain:31.490000DB->x37.540495
	{0x30, 0x25b}, // index: 632, gain:31.537000DB->x37.744180
	{0x30, 0x25f}, // index: 633, gain:31.584000DB->x37.948971
	{0x30, 0x262}, // index: 634, gain:31.631000DB->x38.154872
	{0x30, 0x265}, // index: 635, gain:31.678000DB->x38.361890
	{0x30, 0x269}, // index: 636, gain:31.725000DB->x38.570032
	{0x30, 0x26c}, // index: 637, gain:31.772000DB->x38.779303
	{0x30, 0x26f}, // index: 638, gain:31.819000DB->x38.989710
	{0x30, 0x273}, // index: 639, gain:31.866000DB->x39.201258
	{0x30, 0x276}, // index: 640, gain:31.913000DB->x39.413954
	{0x30, 0x27a}, // index: 641, gain:31.960000DB->x39.627803
	{0x30, 0x27d}, // index: 642, gain:32.007000DB->x39.842814
	{0x30, 0x280}, // index: 643, gain:32.054000DB->x40.058990
	{0x30, 0x284}, // index: 644, gain:32.101000DB->x40.276340
	{0x30, 0x287}, // index: 645, gain:32.148000DB->x40.494869
	{0x30, 0x28b}, // index: 646, gain:32.195000DB->x40.714584
	{0x30, 0x28e}, // index: 647, gain:32.242000DB->x40.935491
	{0x30, 0x292}, // index: 648, gain:32.289000DB->x41.157596
	{0x30, 0x296}, // index: 649, gain:32.336000DB->x41.380906
	{0x30, 0x299}, // index: 650, gain:32.383000DB->x41.605429
	{0x30, 0x29d}, // index: 651, gain:32.430000DB->x41.831169
	{0x30, 0x2a0}, // index: 652, gain:32.477000DB->x42.058134
	{0x30, 0x2a4}, // index: 653, gain:32.524000DB->x42.286331
	{0x30, 0x2a8}, // index: 654, gain:32.571000DB->x42.515765
	{0x30, 0x2ab}, // index: 655, gain:32.618000DB->x42.746445
	{0x30, 0x2af}, // index: 656, gain:32.665000DB->x42.978376
	{0x30, 0x2b3}, // index: 657, gain:32.712000DB->x43.211565
	{0x30, 0x2b7}, // index: 658, gain:32.759000DB->x43.446020
	{0x30, 0x2ba}, // index: 659, gain:32.806000DB->x43.681747
	{0x30, 0x2be}, // index: 660, gain:32.853000DB->x43.918753
	{0x30, 0x2c2}, // index: 661, gain:32.900000DB->x44.157045
	{0x30, 0x2c6}, // index: 662, gain:32.947000DB->x44.396629
	{0x30, 0x2ca}, // index: 663, gain:32.994000DB->x44.637514
	{0x30, 0x2ce}, // index: 664, gain:33.041000DB->x44.879706
	{0x30, 0x2d1}, // index: 665, gain:33.088000DB->x45.123211
	{0x30, 0x2d5}, // index: 666, gain:33.135000DB->x45.368038
	{0x30, 0x2d9}, // index: 667, gain:33.182000DB->x45.614193
	{0x30, 0x2dd}, // index: 668, gain:33.229000DB->x45.861684
	{0x30, 0x2e1}, // index: 669, gain:33.276000DB->x46.110518
	{0x30, 0x2e5}, // index: 670, gain:33.323000DB->x46.360702
	{0x30, 0x2e9}, // index: 671, gain:33.370000DB->x46.612243
	{0x30, 0x2ed}, // index: 672, gain:33.417000DB->x46.865149
	{0x30, 0x2f1}, // index: 673, gain:33.464000DB->x47.119427
	{0x30, 0x2f6}, // index: 674, gain:33.511000DB->x47.375085
	{0x30, 0x2fa}, // index: 675, gain:33.558000DB->x47.632130
	{0x30, 0x2fe}, // index: 676, gain:33.605000DB->x47.890569
	{0x30, 0x302}, // index: 677, gain:33.652000DB->x48.150411
	{0x30, 0x306}, // index: 678, gain:33.699000DB->x48.411663
	{0x30, 0x30a}, // index: 679, gain:33.746000DB->x48.674332
	{0x30, 0x30f}, // index: 680, gain:33.793000DB->x48.938426
	{0x30, 0x313}, // index: 681, gain:33.840000DB->x49.203954
	{0x30, 0x317}, // index: 682, gain:33.887000DB->x49.470921
	{0x30, 0x31b}, // index: 683, gain:33.934000DB->x49.739338
	{0x30, 0x320}, // index: 684, gain:33.981000DB->x50.009211
	{0x30, 0x324}, // index: 685, gain:34.028000DB->x50.280548
	{0x30, 0x328}, // index: 686, gain:34.075000DB->x50.553357
	{0x30, 0x32d}, // index: 687, gain:34.122000DB->x50.827646
	{0x30, 0x331}, // index: 688, gain:34.169000DB->x51.103424
	{0x30, 0x336}, // index: 689, gain:34.216000DB->x51.380698
	{0x30, 0x33a}, // index: 690, gain:34.263000DB->x51.659476
	{0x30, 0x33f}, // index: 691, gain:34.310000DB->x51.939767
	{0x30, 0x343}, // index: 692, gain:34.357000DB->x52.221579
	{0x30, 0x348}, // index: 693, gain:34.404000DB->x52.504920
	{0x30, 0x34c}, // index: 694, gain:34.451000DB->x52.789798
	{0x30, 0x351}, // index: 695, gain:34.498000DB->x53.076222
	{0x30, 0x355}, // index: 696, gain:34.545000DB->x53.364200
	{0x30, 0x35a}, // index: 697, gain:34.592000DB->x53.653740
	{0x30, 0x35f}, // index: 698, gain:34.639000DB->x53.944851
	{0x30, 0x363}, // index: 699, gain:34.686000DB->x54.237542
	{0x30, 0x368}, // index: 700, gain:34.733000DB->x54.531821
	{0x30, 0x36d}, // index: 701, gain:34.780000DB->x54.827696
	{0x30, 0x372}, // index: 702, gain:34.827000DB->x55.125177
	{0x30, 0x376}, // index: 703, gain:34.874000DB->x55.424272
	{0x30, 0x37b}, // index: 704, gain:34.921000DB->x55.724990
	{0x30, 0x380}, // index: 705, gain:34.968000DB->x56.027339
	{0x30, 0x385}, // index: 706, gain:35.015000DB->x56.331329
	{0x30, 0x38a}, // index: 707, gain:35.062000DB->x56.636969
	{0x30, 0x38f}, // index: 708, gain:35.109000DB->x56.944266
	{0x30, 0x394}, // index: 709, gain:35.156000DB->x57.253231
	{0x30, 0x399}, // index: 710, gain:35.203000DB->x57.563872
	{0x30, 0x39e}, // index: 711, gain:35.250000DB->x57.876199
	{0x30, 0x3a3}, // index: 712, gain:35.297000DB->x58.190220
	{0x30, 0x3a8}, // index: 713, gain:35.344000DB->x58.505945
	{0x30, 0x3ad}, // index: 714, gain:35.391000DB->x58.823383
	{0x30, 0x3b2}, // index: 715, gain:35.438000DB->x59.142544
	{0x30, 0x3b7}, // index: 716, gain:35.485000DB->x59.463436
	{0x30, 0x3bc}, // index: 717, gain:35.532000DB->x59.786069
	{0x30, 0x3c1}, // index: 718, gain:35.579000DB->x60.110453
	{0x30, 0x3c6}, // index: 719, gain:35.626000DB->x60.436597
	{0x30, 0x3cc}, // index: 720, gain:35.673000DB->x60.764510
	{0x30, 0x3d1}, // index: 721, gain:35.720000DB->x61.094202
	{0x30, 0x3d6}, // index: 722, gain:35.767000DB->x61.425684
	{0x30, 0x3dc}, // index: 723, gain:35.814000DB->x61.758964
	{0x30, 0x3e1}, // index: 724, gain:35.861000DB->x62.094052
	{0x30, 0x3e6}, // index: 725, gain:35.908000DB->x62.430958
	{0x30, 0x3ec}, // index: 726, gain:35.955000DB->x62.769692
	{0x30, 0x3f1}, // index: 727, gain:36.002000DB->x63.110264
	{0x30, 0x3f7}, // index: 728, gain:36.049000DB->x63.452684
	{0x30, 0x3fc}, // index: 729, gain:36.096000DB->x63.796962
	{0x30, 0x402}, // index: 730, gain:36.143000DB->x64.143108
	{0x30, 0x407}, // index: 731, gain:36.190000DB->x64.491132
	{0x30, 0x40d}, // index: 732, gain:36.237000DB->x64.841044
	{0x30, 0x413}, // index: 733, gain:36.284000DB->x65.192855
	{0x30, 0x418}, // index: 734, gain:36.331000DB->x65.546574
	{0x30, 0x41e}, // index: 735, gain:36.378000DB->x65.902213
	{0x30, 0x424}, // index: 736, gain:36.425000DB->x66.259782
	{0x30, 0x429}, // index: 737, gain:36.472000DB->x66.619290
	{0x30, 0x42f}, // index: 738, gain:36.519000DB->x66.980749
	{0x30, 0x435}, // index: 739, gain:36.566000DB->x67.344169
	{0x30, 0x43b}, // index: 740, gain:36.613000DB->x67.709561
	{0x30, 0x441}, // index: 741, gain:36.660000DB->x68.076936
	{0x30, 0x447}, // index: 742, gain:36.707000DB->x68.446304
	{0x30, 0x44d}, // index: 743, gain:36.754000DB->x68.817676
	{0x30, 0x453}, // index: 744, gain:36.801000DB->x69.191063
	{0x30, 0x459}, // index: 745, gain:36.848000DB->x69.566475
	{0x30, 0x45f}, // index: 746, gain:36.895000DB->x69.943925
	{0x30, 0x465}, // index: 747, gain:36.942000DB->x70.323423
	{0x30, 0x46b}, // index: 748, gain:36.989000DB->x70.704979
	{0x30, 0x471}, // index: 749, gain:37.036000DB->x71.088606
	{0x30, 0x477}, // index: 750, gain:37.083000DB->x71.474315
	{0x30, 0x47d}, // index: 751, gain:37.130000DB->x71.862116
	{0x30, 0x484}, // index: 752, gain:37.177000DB->x72.252021
	{0x30, 0x48a}, // index: 753, gain:37.224000DB->x72.644042
	{0x30, 0x490}, // index: 754, gain:37.271000DB->x73.038190
	{0x30, 0x496}, // index: 755, gain:37.318000DB->x73.434476
	{0x30, 0x49d}, // index: 756, gain:37.365000DB->x73.832912
	{0x30, 0x4a3}, // index: 757, gain:37.412000DB->x74.233511
	{0x30, 0x4aa}, // index: 758, gain:37.459000DB->x74.636283
	{0x30, 0x4b0}, // index: 759, gain:37.506000DB->x75.041240
	{0x30, 0x4b7}, // index: 760, gain:37.553000DB->x75.448394
	{0x30, 0x4bd}, // index: 761, gain:37.600000DB->x75.857758
	{0x30, 0x4c4}, // index: 762, gain:37.647000DB->x76.269342
	{0x30, 0x4ca}, // index: 763, gain:37.694000DB->x76.683160
	{0x30, 0x4d1}, // index: 764, gain:37.741000DB->x77.099223
	{0x30, 0x4d8}, // index: 765, gain:37.788000DB->x77.517543
	{0x30, 0x4df}, // index: 766, gain:37.835000DB->x77.938133
	{0x30, 0x4e5}, // index: 767, gain:37.882000DB->x78.361005
	{0x30, 0x4ec}, // index: 768, gain:37.929000DB->x78.786172
	{0x30, 0x4f3}, // index: 769, gain:37.976000DB->x79.213645
	{0x30, 0x4fa}, // index: 770, gain:38.023000DB->x79.643438
	{0x30, 0x501}, // index: 771, gain:38.070000DB->x80.075563
	{0x30, 0x508}, // index: 772, gain:38.117000DB->x80.510032
	{0x30, 0x50f}, // index: 773, gain:38.164000DB->x80.946859
	{0x30, 0x516}, // index: 774, gain:38.211000DB->x81.386055
	{0x30, 0x51d}, // index: 775, gain:38.258000DB->x81.827635
	{0x30, 0x524}, // index: 776, gain:38.305000DB->x82.271611
	{0x30, 0x52b}, // index: 777, gain:38.352000DB->x82.717995
	{0x30, 0x532}, // index: 778, gain:38.399000DB->x83.166802
	{0x30, 0x539}, // index: 779, gain:38.446000DB->x83.618043
	{0x30, 0x541}, // index: 780, gain:38.493000DB->x84.071733
	{0x30, 0x548}, // index: 781, gain:38.540000DB->x84.527885
	{0x30, 0x54f}, // index: 782, gain:38.587000DB->x84.986511
	{0x30, 0x557}, // index: 783, gain:38.634000DB->x85.447626
	{0x30, 0x55e}, // index: 784, gain:38.681000DB->x85.911242
	{0x30, 0x566}, // index: 785, gain:38.728000DB->x86.377375
	{0x30, 0x56d}, // index: 786, gain:38.775000DB->x86.846036
	{0x30, 0x575}, // index: 787, gain:38.822000DB->x87.317240
	{0x30, 0x57c}, // index: 788, gain:38.869000DB->x87.791001
	{0x30, 0x584}, // index: 789, gain:38.916000DB->x88.267332
	{0x30, 0x58b}, // index: 790, gain:38.963000DB->x88.746248
	{0x30, 0x593}, // index: 791, gain:39.010000DB->x89.227762
	{0x30, 0x59b}, // index: 792, gain:39.057000DB->x89.711889
	{0x30, 0x5a3}, // index: 793, gain:39.104000DB->x90.198642
	{0x30, 0x5ab}, // index: 794, gain:39.151000DB->x90.688037
	{0x30, 0x5b2}, // index: 795, gain:39.198000DB->x91.180087
	{0x30, 0x5ba}, // index: 796, gain:39.245000DB->x91.674806
	{0x30, 0x5c2}, // index: 797, gain:39.292000DB->x92.172210
	{0x30, 0x5ca}, // index: 798, gain:39.339000DB->x92.672312
	{0x30, 0x5d2}, // index: 799, gain:39.386000DB->x93.175128
	{0x30, 0x5da}, // index: 800, gain:39.433000DB->x93.680673
	{0x30, 0x5e3}, // index: 801, gain:39.480000DB->x94.188960
	{0x30, 0x5eb}, // index: 802, gain:39.527000DB->x94.700005
	{0x30, 0x5f3}, // index: 803, gain:39.574000DB->x95.213822
	{0x30, 0x5fb}, // index: 804, gain:39.621000DB->x95.730428
	{0x30, 0x603}, // index: 805, gain:39.668000DB->x96.249836
	{0x30, 0x60c}, // index: 806, gain:39.715000DB->x96.772063
	{0x30, 0x614}, // index: 807, gain:39.762000DB->x97.297123
	{0x30, 0x61d}, // index: 808, gain:39.809000DB->x97.825032
	{0x30, 0x625}, // index: 809, gain:39.856000DB->x98.355806
	{0x30, 0x62e}, // index: 810, gain:39.903000DB->x98.889459
	{0x30, 0x636}, // index: 811, gain:39.950000DB->x99.426007
	{0x30, 0x63f}, // index: 812, gain:39.997000DB->x99.965467
	{0x30, 0x648}, // index: 813, gain:40.044000DB->x100.507854
	{0x30, 0x650}, // index: 814, gain:40.091000DB->x101.053184
	{0x30, 0x659}, // index: 815, gain:40.138000DB->x101.601472
	{0x30, 0x662}, // index: 816, gain:40.185000DB->x102.152735
	{0x30, 0x66b}, // index: 817, gain:40.232000DB->x102.706990
	{0x30, 0x674}, // index: 818, gain:40.279000DB->x103.264251
	{0x30, 0x67d}, // index: 819, gain:40.326000DB->x103.824536
	{0x30, 0x686}, // index: 820, gain:40.373000DB->x104.387861
	{0x30, 0x68f}, // index: 821, gain:40.420000DB->x104.954243
	{0x30, 0x698}, // index: 822, gain:40.467000DB->x105.523697
	{0x30, 0x6a1}, // index: 823, gain:40.514000DB->x106.096242
	{0x30, 0x6aa}, // index: 824, gain:40.561000DB->x106.671892
	{0x30, 0x6b4}, // index: 825, gain:40.608000DB->x107.250667
	{0x30, 0x6bd}, // index: 826, gain:40.655000DB->x107.832581
	{0x30, 0x6c6}, // index: 827, gain:40.702000DB->x108.417653
	{0x30, 0x6d0}, // index: 828, gain:40.749000DB->x109.005899
	{0x30, 0x6d9}, // index: 829, gain:40.796000DB->x109.597337
	{0x30, 0x6e3}, // index: 830, gain:40.843000DB->x110.191983
	{0x30, 0x6ec}, // index: 831, gain:40.890000DB->x110.789857
	{0x30, 0x6f6}, // index: 832, gain:40.937000DB->x111.390974
	{0x30, 0x6ff}, // index: 833, gain:40.984000DB->x111.995352
	{0x30, 0x709}, // index: 834, gain:41.031000DB->x112.603010
	{0x30, 0x713}, // index: 835, gain:41.078000DB->x113.213965
	{0x30, 0x71d}, // index: 836, gain:41.125000DB->x113.828235
	{0x30, 0x727}, // index: 837, gain:41.172000DB->x114.445837
	{0x30, 0x731}, // index: 838, gain:41.219000DB->x115.066791
	{0x30, 0x73b}, // index: 839, gain:41.266000DB->x115.691113
	{0x30, 0x745}, // index: 840, gain:41.313000DB->x116.318823
	{0x30, 0x74f}, // index: 841, gain:41.360000DB->x116.949939
	{0x30, 0x759}, // index: 842, gain:41.407000DB->x117.584479
	{0x30, 0x763}, // index: 843, gain:41.454000DB->x118.222462
	{0x30, 0x76d}, // index: 844, gain:41.501000DB->x118.863907
	{0x30, 0x778}, // index: 845, gain:41.548000DB->x119.508831
	{0x30, 0x782}, // index: 846, gain:41.595000DB->x120.157255
	{0x30, 0x78c}, // index: 847, gain:41.642000DB->x120.809198
	{0x30, 0x797}, // index: 848, gain:41.689000DB->x121.464677
	{0x30, 0x7a1}, // index: 849, gain:41.736000DB->x122.123713
	{0x30, 0x7ac}, // index: 850, gain:41.783000DB->x122.786325
	{0x30, 0x7b7}, // index: 851, gain:41.830000DB->x123.452532
	{0x30, 0x7c1}, // index: 852, gain:41.877000DB->x124.122353
	{0x30, 0x7cc}, // index: 853, gain:41.924000DB->x124.795809
	{0x30, 0x7d7}, // index: 854, gain:41.971000DB->x125.472919
	{0x30, 0x7e2}, // index: 855, gain:42.018000DB->x126.153702
	{0x30, 0x7ed}, // index: 856, gain:42.065000DB->x126.838180
	{0x30, 0x7f8}, // index: 857, gain:42.112000DB->x127.526371
};

#define AR0331_GAIN_1_23DB	(601)
#define AR0331_GAIN_ROWS		(602)
#define AR0331_GAIN_COLS		(2)
/* This is 64-step gain table, AR0331_GAIN_ROWS = 602, AR0331_GAIN_COLS = 2 */
static const u16 AR0331_GAIN_TABLE[AR0331_GAIN_ROWS][AR0331_GAIN_COLS] =
{
	{0x30, 0x1fe}, // index: 0, gain:30.080000DB->x31.915379
	{0x30, 0x1fb}, // index: 1, gain:30.033000DB->x31.743148
	{0x30, 0x1f9}, // index: 2, gain:29.986000DB->x31.571848
	{0x30, 0x1f6}, // index: 3, gain:29.939000DB->x31.401472
	{0x30, 0x1f3}, // index: 4, gain:29.892000DB->x31.232015
	{0x30, 0x1f1}, // index: 5, gain:29.845000DB->x31.063472
	{0x30, 0x1ee}, // index: 6, gain:29.798000DB->x30.895839
	{0x30, 0x1eb}, // index: 7, gain:29.751000DB->x30.729111
	{0x30, 0x1e9}, // index: 8, gain:29.704000DB->x30.563283
	{0x30, 0x1e6}, // index: 9, gain:29.657000DB->x30.398349
	{0x30, 0x1e3}, // index: 10, gain:29.610000DB->x30.234306
	{0x30, 0x1e1}, // index: 11, gain:29.563000DB->x30.071147
	{0x30, 0x1de}, // index: 12, gain:29.516000DB->x29.908870
	{0x30, 0x1db}, // index: 13, gain:29.469000DB->x29.747468
	{0x30, 0x1d9}, // index: 14, gain:29.422000DB->x29.586937
	{0x30, 0x1d6}, // index: 15, gain:29.375000DB->x29.427272
	{0x30, 0x1d4}, // index: 16, gain:29.328000DB->x29.268469
	{0x30, 0x1d1}, // index: 17, gain:29.281000DB->x29.110522
	{0x30, 0x1cf}, // index: 18, gain:29.234000DB->x28.953429
	{0x30, 0x1cc}, // index: 19, gain:29.187000DB->x28.797183
	{0x30, 0x1ca}, // index: 20, gain:29.140000DB->x28.641780
	{0x30, 0x1c7}, // index: 21, gain:29.093000DB->x28.487215
	{0x30, 0x1c5}, // index: 22, gain:29.046000DB->x28.333485
	{0x30, 0x1c2}, // index: 23, gain:28.999000DB->x28.180585
	{0x30, 0x1c0}, // index: 24, gain:28.952000DB->x28.028509
	{0x30, 0x1be}, // index: 25, gain:28.905000DB->x27.877255
	{0x30, 0x1bb}, // index: 26, gain:28.858000DB->x27.726816
	{0x30, 0x1b9}, // index: 27, gain:28.811000DB->x27.577189
	{0x30, 0x1b6}, // index: 28, gain:28.764000DB->x27.428370
	{0x30, 0x1b4}, // index: 29, gain:28.717000DB->x27.280354
	{0x30, 0x1b2}, // index: 30, gain:28.670000DB->x27.133137
	{0x30, 0x1af}, // index: 31, gain:28.623000DB->x26.986714
	{0x30, 0x1ad}, // index: 32, gain:28.576000DB->x26.841081
	{0x30, 0x1ab}, // index: 33, gain:28.529000DB->x26.696234
	{0x30, 0x1a8}, // index: 34, gain:28.482000DB->x26.552169
	{0x30, 0x1a6}, // index: 35, gain:28.435000DB->x26.408881
	{0x30, 0x1a4}, // index: 36, gain:28.388000DB->x26.266367
	{0x30, 0x1a1}, // index: 37, gain:28.341000DB->x26.124621
	{0x30, 0x19f}, // index: 38, gain:28.294000DB->x25.983641
	{0x30, 0x19d}, // index: 39, gain:28.247000DB->x25.843421
	{0x30, 0x19b}, // index: 40, gain:28.200000DB->x25.703958
	{0x30, 0x199}, // index: 41, gain:28.153000DB->x25.565247
	{0x30, 0x196}, // index: 42, gain:28.106000DB->x25.427286
	{0x30, 0x194}, // index: 43, gain:28.059000DB->x25.290068
	{0x30, 0x192}, // index: 44, gain:28.012000DB->x25.153591
	{0x30, 0x190}, // index: 45, gain:27.965000DB->x25.017851
	{0x30, 0x18e}, // index: 46, gain:27.918000DB->x24.882843
	{0x30, 0x18b}, // index: 47, gain:27.871000DB->x24.748564
	{0x30, 0x189}, // index: 48, gain:27.824000DB->x24.615009
	{0x30, 0x187}, // index: 49, gain:27.777000DB->x24.482175
	{0x30, 0x185}, // index: 50, gain:27.730000DB->x24.350058
	{0x30, 0x183}, // index: 51, gain:27.683000DB->x24.218654
	{0x30, 0x181}, // index: 52, gain:27.636000DB->x24.087959
	{0x30, 0x17f}, // index: 53, gain:27.589000DB->x23.957969
	{0x30, 0x17d}, // index: 54, gain:27.542000DB->x23.828681
	{0x30, 0x17b}, // index: 55, gain:27.495000DB->x23.700090
	{0x30, 0x179}, // index: 56, gain:27.448000DB->x23.572194
	{0x30, 0x177}, // index: 57, gain:27.401000DB->x23.444987
	{0x30, 0x175}, // index: 58, gain:27.354000DB->x23.318467
	{0x30, 0x173}, // index: 59, gain:27.307000DB->x23.192630
	{0x30, 0x171}, // index: 60, gain:27.260000DB->x23.067472
	{0x30, 0x16f}, // index: 61, gain:27.213000DB->x22.942989
	{0x30, 0x16d}, // index: 62, gain:27.166000DB->x22.819178
	{0x30, 0x16b}, // index: 63, gain:27.119000DB->x22.696035
	{0x30, 0x169}, // index: 64, gain:27.072000DB->x22.573557
	{0x30, 0x167}, // index: 65, gain:27.025000DB->x22.451740
	{0x30, 0x165}, // index: 66, gain:26.978000DB->x22.330580
	{0x30, 0x163}, // index: 67, gain:26.931000DB->x22.210074
	{0x30, 0x161}, // index: 68, gain:26.884000DB->x22.090218
	{0x30, 0x15f}, // index: 69, gain:26.837000DB->x21.971009
	{0x30, 0x15d}, // index: 70, gain:26.790000DB->x21.852443
	{0x30, 0x15b}, // index: 71, gain:26.743000DB->x21.734517
	{0x30, 0x159}, // index: 72, gain:26.696000DB->x21.617228
	{0x30, 0x158}, // index: 73, gain:26.649000DB->x21.500571
	{0x30, 0x156}, // index: 74, gain:26.602000DB->x21.384544
	{0x30, 0x154}, // index: 75, gain:26.555000DB->x21.269143
	{0x30, 0x152}, // index: 76, gain:26.508000DB->x21.154365
	{0x30, 0x150}, // index: 77, gain:26.461000DB->x21.040207
	{0x30, 0x14e}, // index: 78, gain:26.414000DB->x20.926664
	{0x30, 0x14d}, // index: 79, gain:26.367000DB->x20.813734
	{0x30, 0x14b}, // index: 80, gain:26.320000DB->x20.701413
	{0x30, 0x149}, // index: 81, gain:26.273000DB->x20.589699
	{0x30, 0x147}, // index: 82, gain:26.226000DB->x20.478588
	{0x30, 0x145}, // index: 83, gain:26.179000DB->x20.368076
	{0x30, 0x144}, // index: 84, gain:26.132000DB->x20.258160
	{0x30, 0x142}, // index: 85, gain:26.085000DB->x20.148838
	{0x30, 0x140}, // index: 86, gain:26.038000DB->x20.040105
	{0x30, 0x13e}, // index: 87, gain:25.991000DB->x19.931960
	{0x30, 0x13d}, // index: 88, gain:25.944000DB->x19.824398
	{0x30, 0x13b}, // index: 89, gain:25.897000DB->x19.717416
	{0x30, 0x139}, // index: 90, gain:25.850000DB->x19.611012
	{0x30, 0x138}, // index: 91, gain:25.803000DB->x19.505182
	{0x30, 0x136}, // index: 92, gain:25.756000DB->x19.399923
	{0x30, 0x134}, // index: 93, gain:25.709000DB->x19.295232
	{0x30, 0x133}, // index: 94, gain:25.662000DB->x19.191106
	{0x30, 0x131}, // index: 95, gain:25.615000DB->x19.087542
	{0x30, 0x12f}, // index: 96, gain:25.568000DB->x18.984537
	{0x30, 0x12e}, // index: 97, gain:25.521000DB->x18.882087
	{0x30, 0x12c}, // index: 98, gain:25.474000DB->x18.780191
	{0x30, 0x12a}, // index: 99, gain:25.427000DB->x18.678844
	{0x30, 0x129}, // index: 100, gain:25.380000DB->x18.578045
	{0x30, 0x127}, // index: 101, gain:25.333000DB->x18.477789
	{0x30, 0x126}, // index: 102, gain:25.286000DB->x18.378074
	{0x30, 0x124}, // index: 103, gain:25.239000DB->x18.278898
	{0x30, 0x122}, // index: 104, gain:25.192000DB->x18.180256
	{0x30, 0x121}, // index: 105, gain:25.145000DB->x18.082147
	{0x30, 0x11f}, // index: 106, gain:25.098000DB->x17.984568
	{0x30, 0x11e}, // index: 107, gain:25.051000DB->x17.887515
	{0x30, 0x11c}, // index: 108, gain:25.004000DB->x17.790985
	{0x30, 0x11b}, // index: 109, gain:24.957000DB->x17.694977
	{0x30, 0x119}, // index: 110, gain:24.910000DB->x17.599487
	{0x30, 0x118}, // index: 111, gain:24.863000DB->x17.504512
	{0x30, 0x116}, // index: 112, gain:24.816000DB->x17.410049
	{0x30, 0x115}, // index: 113, gain:24.769000DB->x17.316097
	{0x30, 0x113}, // index: 114, gain:24.722000DB->x17.222651
	{0x30, 0x112}, // index: 115, gain:24.675000DB->x17.129710
	{0x30, 0x110}, // index: 116, gain:24.628000DB->x17.037270
	{0x30, 0x10f}, // index: 117, gain:24.581000DB->x16.945329
	{0x30, 0x10d}, // index: 118, gain:24.534000DB->x16.853884
	{0x30, 0x10c}, // index: 119, gain:24.487000DB->x16.762933
	{0x30, 0x10a}, // index: 120, gain:24.440000DB->x16.672472
	{0x30, 0x109}, // index: 121, gain:24.393000DB->x16.582500
	{0x30, 0x107}, // index: 122, gain:24.346000DB->x16.493013
	{0x30, 0x106}, // index: 123, gain:24.299000DB->x16.404009
	{0x30, 0x105}, // index: 124, gain:24.252000DB->x16.315485
	{0x30, 0x103}, // index: 125, gain:24.205000DB->x16.227440
	{0x30, 0x102}, // index: 126, gain:24.158000DB->x16.139869
	{0x30, 0x100}, // index: 127, gain:24.111000DB->x16.052771
	{0x30, 0xff}, // index: 128, gain:24.064000DB->x15.966142
	{0x30, 0xfe}, // index: 129, gain:24.017000DB->x15.879982
	{0x30, 0xfc}, // index: 130, gain:23.970000DB->x15.794286
	{0x30, 0xfb}, // index: 131, gain:23.923000DB->x15.709053
	{0x30, 0xf9}, // index: 132, gain:23.876000DB->x15.624280
	{0x30, 0xf8}, // index: 133, gain:23.829000DB->x15.539964
	{0x30, 0xf7}, // index: 134, gain:23.782000DB->x15.456103
	{0x30, 0xf5}, // index: 135, gain:23.735000DB->x15.372695
	{0x30, 0xf4}, // index: 136, gain:23.688000DB->x15.289736
	{0x30, 0xf3}, // index: 137, gain:23.641000DB->x15.207226
	{0x30, 0xf2}, // index: 138, gain:23.594000DB->x15.125161
	{0x30, 0xf0}, // index: 139, gain:23.547000DB->x15.043538
	{0x30, 0xef}, // index: 140, gain:23.500000DB->x14.962357
	{0x30, 0xee}, // index: 141, gain:23.453000DB->x14.881613
	{0x30, 0xec}, // index: 142, gain:23.406000DB->x14.801305
	{0x30, 0xeb}, // index: 143, gain:23.359000DB->x14.721430
	{0x30, 0xea}, // index: 144, gain:23.312000DB->x14.641986
	{0x30, 0xe9}, // index: 145, gain:23.265000DB->x14.562972
	{0x30, 0xe7}, // index: 146, gain:23.218000DB->x14.484383
	{0x30, 0xe6}, // index: 147, gain:23.171000DB->x14.406219
	{0x30, 0xe5}, // index: 148, gain:23.124000DB->x14.328476
	{0x30, 0xe4}, // index: 149, gain:23.077000DB->x14.251153
	{0x30, 0xe2}, // index: 150, gain:23.030000DB->x14.174247
	{0x30, 0xe1}, // index: 151, gain:22.983000DB->x14.097756
	{0x30, 0xe0}, // index: 152, gain:22.936000DB->x14.021678
	{0x30, 0xdf}, // index: 153, gain:22.889000DB->x13.946011
	{0x30, 0xdd}, // index: 154, gain:22.842000DB->x13.870752
	{0x30, 0xdc}, // index: 155, gain:22.795000DB->x13.795899
	{0x30, 0xdb}, // index: 156, gain:22.748000DB->x13.721450
	{0x30, 0xda}, // index: 157, gain:22.701000DB->x13.647402
	{0x30, 0xd9}, // index: 158, gain:22.654000DB->x13.573755
	{0x30, 0xd8}, // index: 159, gain:22.607000DB->x13.500505
	{0x30, 0xd6}, // index: 160, gain:22.560000DB->x13.427650
	{0x30, 0xd5}, // index: 161, gain:22.513000DB->x13.355188
	{0x30, 0xd4}, // index: 162, gain:22.466000DB->x13.283117
	{0x30, 0xd3}, // index: 163, gain:22.419000DB->x13.211435
	{0x30, 0xd2}, // index: 164, gain:22.372000DB->x13.140140
	{0x30, 0xd1}, // index: 165, gain:22.325000DB->x13.069230
	{0x30, 0xcf}, // index: 166, gain:22.278000DB->x12.998702
	{0x30, 0xce}, // index: 167, gain:22.231000DB->x12.928555
	{0x30, 0xcd}, // index: 168, gain:22.184000DB->x12.858787
	{0x30, 0xcc}, // index: 169, gain:22.137000DB->x12.789395
	{0x30, 0xcb}, // index: 170, gain:22.090000DB->x12.720378
	{0x30, 0xca}, // index: 171, gain:22.043000DB->x12.651732
	{0x30, 0xc9}, // index: 172, gain:21.996000DB->x12.583458
	{0x30, 0xc8}, // index: 173, gain:21.949000DB->x12.515552
	{0x30, 0xc7}, // index: 174, gain:21.902000DB->x12.448012
	{0x30, 0xc6}, // index: 175, gain:21.855000DB->x12.380837
	{0x30, 0xc5}, // index: 176, gain:21.808000DB->x12.314024
	{0x30, 0xc3}, // index: 177, gain:21.761000DB->x12.247572
	{0x30, 0xc2}, // index: 178, gain:21.714000DB->x12.181478
	{0x30, 0xc1}, // index: 179, gain:21.667000DB->x12.115742
	{0x30, 0xc0}, // index: 180, gain:21.620000DB->x12.050359
	{0x30, 0xbf}, // index: 181, gain:21.573000DB->x11.985330
	{0x30, 0xbe}, // index: 182, gain:21.526000DB->x11.920652
	{0x30, 0xbd}, // index: 183, gain:21.479000DB->x11.856322
	{0x30, 0xbc}, // index: 184, gain:21.432000DB->x11.792340
	{0x30, 0xbb}, // index: 185, gain:21.385000DB->x11.728703
	{0x30, 0xba}, // index: 186, gain:21.338000DB->x11.665410
	{0x30, 0xb9}, // index: 187, gain:21.291000DB->x11.602458
	{0x30, 0xb8}, // index: 188, gain:21.244000DB->x11.539846
	{0x30, 0xb7}, // index: 189, gain:21.197000DB->x11.477571
	{0x30, 0xb6}, // index: 190, gain:21.150000DB->x11.415633
	{0x30, 0xb5}, // index: 191, gain:21.103000DB->x11.354029
	{0x30, 0xb4}, // index: 192, gain:21.056000DB->x11.292757
	{0x30, 0xb3}, // index: 193, gain:21.009000DB->x11.231817
	{0x30, 0xb2}, // index: 194, gain:20.962000DB->x11.171204
	{0x30, 0xb1}, // index: 195, gain:20.915000DB->x11.110919
	{0x30, 0xb0}, // index: 196, gain:20.868000DB->x11.050960
	{0x30, 0xaf}, // index: 197, gain:20.821000DB->x10.991324
	{0x30, 0xae}, // index: 198, gain:20.774000DB->x10.932009
	{0x30, 0xad}, // index: 199, gain:20.727000DB->x10.873015
	{0x30, 0xad}, // index: 200, gain:20.680000DB->x10.814340
	{0x30, 0xac}, // index: 201, gain:20.633000DB->x10.755980
	{0x30, 0xab}, // index: 202, gain:20.586000DB->x10.697936
	{0x30, 0xaa}, // index: 203, gain:20.539000DB->x10.640205
	{0x30, 0xa9}, // index: 204, gain:20.492000DB->x10.582786
	{0x30, 0xa8}, // index: 205, gain:20.445000DB->x10.525676
	{0x30, 0xa7}, // index: 206, gain:20.398000DB->x10.468875
	{0x30, 0xa6}, // index: 207, gain:20.351000DB->x10.412380
	{0x30, 0xa5}, // index: 208, gain:20.304000DB->x10.356190
	{0x30, 0xa4}, // index: 209, gain:20.257000DB->x10.300303
	{0x30, 0xa3}, // index: 210, gain:20.210000DB->x10.244718
	{0x30, 0xa3}, // index: 211, gain:20.163000DB->x10.189433
	{0x30, 0xa2}, // index: 212, gain:20.116000DB->x10.134446
	{0x30, 0xa1}, // index: 213, gain:20.069000DB->x10.079756
	{0x30, 0xa0}, // index: 214, gain:20.022000DB->x10.025361
	{0x30, 0x9f}, // index: 215, gain:19.975000DB->x9.971259
	{0x30, 0x9e}, // index: 216, gain:19.928000DB->x9.917450
	{0x30, 0x9d}, // index: 217, gain:19.881000DB->x9.863930
	{0x30, 0x9c}, // index: 218, gain:19.834000DB->x9.810700
	{0x30, 0x9c}, // index: 219, gain:19.787000DB->x9.757757
	{0x30, 0x9b}, // index: 220, gain:19.740000DB->x9.705100
	{0x30, 0x9a}, // index: 221, gain:19.693000DB->x9.652726
	{0x30, 0x99}, // index: 222, gain:19.646000DB->x9.600636
	{0x30, 0x98}, // index: 223, gain:19.599000DB->x9.548826
	{0x30, 0x97}, // index: 224, gain:19.552000DB->x9.497297
	{0x30, 0x97}, // index: 225, gain:19.505000DB->x9.446045
	{0x30, 0x96}, // index: 226, gain:19.458000DB->x9.395070
	{0x30, 0x95}, // index: 227, gain:19.411000DB->x9.344369
	{0x30, 0x94}, // index: 228, gain:19.364000DB->x9.293943
	{0x30, 0x93}, // index: 229, gain:19.317000DB->x9.243788
	{0x30, 0x93}, // index: 230, gain:19.270000DB->x9.193905
	{0x30, 0x92}, // index: 231, gain:19.223000DB->x9.144290
	{0x30, 0x91}, // index: 232, gain:19.176000DB->x9.094943
	{0x30, 0x90}, // index: 233, gain:19.129000DB->x9.045863
	{0x30, 0x8f}, // index: 234, gain:19.082000DB->x8.997047
	{0x30, 0x8f}, // index: 235, gain:19.035000DB->x8.948495
	{0x30, 0x8e}, // index: 236, gain:18.988000DB->x8.900205
	{0x30, 0x8d}, // index: 237, gain:18.941000DB->x8.852175
	{0x30, 0x8c}, // index: 238, gain:18.894000DB->x8.804405
	{0x30, 0x8c}, // index: 239, gain:18.847000DB->x8.756892
	{0x30, 0x8b}, // index: 240, gain:18.800000DB->x8.709636
	{0x30, 0x8a}, // index: 241, gain:18.753000DB->x8.662635
	{0x30, 0x89}, // index: 242, gain:18.706000DB->x8.615887
	{0x30, 0x89}, // index: 243, gain:18.659000DB->x8.569392
	{0x30, 0x88}, // index: 244, gain:18.612000DB->x8.523147
	{0x30, 0x87}, // index: 245, gain:18.565000DB->x8.477153
	{0x30, 0x86}, // index: 246, gain:18.518000DB->x8.431406
	{0x30, 0x86}, // index: 247, gain:18.471000DB->x8.385906
	{0x30, 0x85}, // index: 248, gain:18.424000DB->x8.340652
	{0x30, 0x84}, // index: 249, gain:18.377000DB->x8.295642
	{0x30, 0x84}, // index: 250, gain:18.330000DB->x8.250875
	{0x30, 0x83}, // index: 251, gain:18.283000DB->x8.206349
	{0x30, 0x82}, // index: 252, gain:18.236000DB->x8.162064
	{0x30, 0x81}, // index: 253, gain:18.189000DB->x8.118018
	{0x30, 0x81}, // index: 254, gain:18.142000DB->x8.074209
	{0x30, 0x80}, // index: 255, gain:18.095000DB->x8.030637
	{0x2c, 0x9f}, // index: 256, gain:18.048000DB->x7.987300
	{0x2c, 0x9e}, // index: 257, gain:18.001000DB->x7.944197
	{0x2c, 0x9d}, // index: 258, gain:17.954000DB->x7.901326
	{0x2c, 0x9c}, // index: 259, gain:17.907000DB->x7.858687
	{0x2c, 0x9c}, // index: 260, gain:17.860000DB->x7.816278
	{0x2c, 0x9b}, // index: 261, gain:17.813000DB->x7.774098
	{0x2c, 0x9a}, // index: 262, gain:17.766000DB->x7.732145
	{0x2c, 0x99}, // index: 263, gain:17.719000DB->x7.690419
	{0x2c, 0x98}, // index: 264, gain:17.672000DB->x7.648918
	{0x2c, 0x97}, // index: 265, gain:17.625000DB->x7.607641
	{0x2c, 0x97}, // index: 266, gain:17.578000DB->x7.566586
	{0x2c, 0x96}, // index: 267, gain:17.531000DB->x7.525754
	{0x2c, 0x95}, // index: 268, gain:17.484000DB->x7.485141
	{0x2c, 0x94}, // index: 269, gain:17.437000DB->x7.444748
	{0x2c, 0x93}, // index: 270, gain:17.390000DB->x7.404573
	{0x2c, 0x93}, // index: 271, gain:17.343000DB->x7.364614
	{0x2c, 0x92}, // index: 272, gain:17.296000DB->x7.324871
	{0x2c, 0x91}, // index: 273, gain:17.249000DB->x7.285343
	{0x2c, 0x90}, // index: 274, gain:17.202000DB->x7.246028
	{0x2c, 0x8f}, // index: 275, gain:17.155000DB->x7.206925
	{0x2c, 0x8f}, // index: 276, gain:17.108000DB->x7.168033
	{0x2c, 0x8e}, // index: 277, gain:17.061000DB->x7.129351
	{0x2c, 0x8d}, // index: 278, gain:17.014000DB->x7.090878
	{0x2c, 0x8c}, // index: 279, gain:16.967000DB->x7.052612
	{0x2c, 0x8c}, // index: 280, gain:16.920000DB->x7.014553
	{0x2c, 0x8b}, // index: 281, gain:16.873000DB->x6.976699
	{0x2c, 0x8a}, // index: 282, gain:16.826000DB->x6.939050
	{0x2c, 0x89}, // index: 283, gain:16.779000DB->x6.901603
	{0x2c, 0x89}, // index: 284, gain:16.732000DB->x6.864359
	{0x2c, 0x88}, // index: 285, gain:16.685000DB->x6.827316
	{0x2c, 0x87}, // index: 286, gain:16.638000DB->x6.790473
	{0x2c, 0x86}, // index: 287, gain:16.591000DB->x6.753828
	{0x2c, 0x86}, // index: 288, gain:16.544000DB->x6.717381
	{0x2c, 0x85}, // index: 289, gain:16.497000DB->x6.681131
	{0x2c, 0x84}, // index: 290, gain:16.450000DB->x6.645077
	{0x2c, 0x83}, // index: 291, gain:16.403000DB->x6.609217
	{0x2c, 0x83}, // index: 292, gain:16.356000DB->x6.573550
	{0x2c, 0x82}, // index: 293, gain:16.309000DB->x6.538077
	{0x2c, 0x81}, // index: 294, gain:16.262000DB->x6.502794
	{0x2c, 0x81}, // index: 295, gain:16.215000DB->x6.467702
	{0x2c, 0x80}, // index: 296, gain:16.168000DB->x6.432799
	{0x28, 0x99}, // index: 297, gain:16.121000DB->x6.398085
	{0x28, 0x98}, // index: 298, gain:16.074000DB->x6.363558
	{0x28, 0x97}, // index: 299, gain:16.027000DB->x6.329217
	{0x28, 0x96}, // index: 300, gain:15.980000DB->x6.295062
	{0x28, 0x96}, // index: 301, gain:15.933000DB->x6.261091
	{0x28, 0x95}, // index: 302, gain:15.886000DB->x6.227303
	{0x28, 0x94}, // index: 303, gain:15.839000DB->x6.193698
	{0x28, 0x93}, // index: 304, gain:15.792000DB->x6.160274
	{0x28, 0x92}, // index: 305, gain:15.745000DB->x6.127030
	{0x28, 0x92}, // index: 306, gain:15.698000DB->x6.093966
	{0x28, 0x91}, // index: 307, gain:15.651000DB->x6.061080
	{0x28, 0x90}, // index: 308, gain:15.604000DB->x6.028371
	{0x28, 0x8f}, // index: 309, gain:15.557000DB->x5.995840
	{0x28, 0x8e}, // index: 310, gain:15.510000DB->x5.963483
	{0x28, 0x8e}, // index: 311, gain:15.463000DB->x5.931301
	{0x28, 0x8d}, // index: 312, gain:15.416000DB->x5.899293
	{0x28, 0x8c}, // index: 313, gain:15.369000DB->x5.867458
	{0x28, 0x8b}, // index: 314, gain:15.322000DB->x5.835795
	{0x28, 0x8b}, // index: 315, gain:15.275000DB->x5.804302
	{0x28, 0x8a}, // index: 316, gain:15.228000DB->x5.772979
	{0x28, 0x89}, // index: 317, gain:15.181000DB->x5.741826
	{0x28, 0x88}, // index: 318, gain:15.134000DB->x5.710840
	{0x28, 0x88}, // index: 319, gain:15.087000DB->x5.680022
	{0x28, 0x87}, // index: 320, gain:15.040000DB->x5.649370
	{0x28, 0x86}, // index: 321, gain:14.993000DB->x5.618883
	{0x28, 0x85}, // index: 322, gain:14.946000DB->x5.588561
	{0x28, 0x85}, // index: 323, gain:14.899000DB->x5.558403
	{0x28, 0x84}, // index: 324, gain:14.852000DB->x5.528407
	{0x28, 0x83}, // index: 325, gain:14.805000DB->x5.498573
	{0x28, 0x83}, // index: 326, gain:14.758000DB->x5.468900
	{0x28, 0x82}, // index: 327, gain:14.711000DB->x5.439388
	{0x28, 0x81}, // index: 328, gain:14.664000DB->x5.410034
	{0x28, 0x80}, // index: 329, gain:14.617000DB->x5.380839
	{0x28, 0x80}, // index: 330, gain:14.570000DB->x5.351802
	{0x24, 0x95}, // index: 331, gain:14.523000DB->x5.322921
	{0x24, 0x94}, // index: 332, gain:14.476000DB->x5.294196
	{0x24, 0x93}, // index: 333, gain:14.429000DB->x5.265626
	{0x24, 0x93}, // index: 334, gain:14.382000DB->x5.237210
	{0x24, 0x92}, // index: 335, gain:14.335000DB->x5.208948
	{0x24, 0x91}, // index: 336, gain:14.288000DB->x5.180838
	{0x24, 0x90}, // index: 337, gain:14.241000DB->x5.152880
	{0x24, 0x8f}, // index: 338, gain:14.194000DB->x5.125072
	{0x24, 0x8f}, // index: 339, gain:14.147000DB->x5.097415
	{0x24, 0x8e}, // index: 340, gain:14.100000DB->x5.069907
	{0x24, 0x8d}, // index: 341, gain:14.053000DB->x5.042548
	{0x24, 0x8c}, // index: 342, gain:14.006000DB->x5.015336
	{0x24, 0x8c}, // index: 343, gain:13.959000DB->x4.988271
	{0x24, 0x8b}, // index: 344, gain:13.912000DB->x4.961352
	{0x24, 0x8a}, // index: 345, gain:13.865000DB->x4.934578
	{0x24, 0x89}, // index: 346, gain:13.818000DB->x4.907949
	{0x24, 0x89}, // index: 347, gain:13.771000DB->x4.881463
	{0x24, 0x88}, // index: 348, gain:13.724000DB->x4.855120
	{0x24, 0x87}, // index: 349, gain:13.677000DB->x4.828920
	{0x24, 0x86}, // index: 350, gain:13.630000DB->x4.802861
	{0x24, 0x86}, // index: 351, gain:13.583000DB->x4.776942
	{0x24, 0x85}, // index: 352, gain:13.536000DB->x4.751164
	{0x24, 0x84}, // index: 353, gain:13.489000DB->x4.725524
	{0x24, 0x83}, // index: 354, gain:13.442000DB->x4.700023
	{0x24, 0x83}, // index: 355, gain:13.395000DB->x4.674660
	{0x24, 0x82}, // index: 356, gain:13.348000DB->x4.649433
	{0x24, 0x81}, // index: 357, gain:13.301000DB->x4.624343
	{0x24, 0x81}, // index: 358, gain:13.254000DB->x4.599387
	{0x24, 0x80}, // index: 359, gain:13.207000DB->x4.574567
	{0x20, 0x91}, // index: 360, gain:13.160000DB->x4.549881
	{0x20, 0x90}, // index: 361, gain:13.113000DB->x4.525327
	{0x20, 0x90}, // index: 362, gain:13.066000DB->x4.500907
	{0x20, 0x8f}, // index: 363, gain:13.019000DB->x4.476618
	{0x20, 0x8e}, // index: 364, gain:12.972000DB->x4.452460
	{0x20, 0x8d}, // index: 365, gain:12.925000DB->x4.428432
	{0x20, 0x8c}, // index: 366, gain:12.878000DB->x4.404534
	{0x20, 0x8c}, // index: 367, gain:12.831000DB->x4.380765
	{0x20, 0x8b}, // index: 368, gain:12.784000DB->x4.357125
	{0x20, 0x8a}, // index: 369, gain:12.737000DB->x4.333612
	{0x20, 0x89}, // index: 370, gain:12.690000DB->x4.310226
	{0x20, 0x89}, // index: 371, gain:12.643000DB->x4.286966
	{0x20, 0x88}, // index: 372, gain:12.596000DB->x4.263831
	{0x20, 0x87}, // index: 373, gain:12.549000DB->x4.240822
	{0x20, 0x86}, // index: 374, gain:12.502000DB->x4.217936
	{0x20, 0x86}, // index: 375, gain:12.455000DB->x4.195174
	{0x20, 0x85}, // index: 376, gain:12.408000DB->x4.172535
	{0x20, 0x84}, // index: 377, gain:12.361000DB->x4.150018
	{0x20, 0x84}, // index: 378, gain:12.314000DB->x4.127623
	{0x20, 0x83}, // index: 379, gain:12.267000DB->x4.105348
	{0x20, 0x82}, // index: 380, gain:12.220000DB->x4.083194
	{0x20, 0x81}, // index: 381, gain:12.173000DB->x4.061159
	{0x20, 0x81}, // index: 382, gain:12.126000DB->x4.039243
	{0x20, 0x80}, // index: 383, gain:12.079000DB->x4.017446
	{0x1e, 0x8f}, // index: 384, gain:12.032000DB->x3.995766
	{0x1e, 0x8e}, // index: 385, gain:11.985000DB->x3.974203
	{0x1e, 0x8e}, // index: 386, gain:11.938000DB->x3.952756
	{0x1e, 0x8d}, // index: 387, gain:11.891000DB->x3.931425
	{0x1e, 0x8c}, // index: 388, gain:11.844000DB->x3.910209
	{0x1e, 0x8b}, // index: 389, gain:11.797000DB->x3.889108
	{0x1e, 0x8b}, // index: 390, gain:11.750000DB->x3.868121
	{0x1e, 0x8a}, // index: 391, gain:11.703000DB->x3.847246
	{0x1e, 0x89}, // index: 392, gain:11.656000DB->x3.826485
	{0x1e, 0x88}, // index: 393, gain:11.609000DB->x3.805835
	{0x1e, 0x88}, // index: 394, gain:11.562000DB->x3.785297
	{0x1e, 0x87}, // index: 395, gain:11.515000DB->x3.764870
	{0x1e, 0x86}, // index: 396, gain:11.468000DB->x3.744553
	{0x1e, 0x85}, // index: 397, gain:11.421000DB->x3.724346
	{0x1e, 0x85}, // index: 398, gain:11.374000DB->x3.704248
	{0x1e, 0x84}, // index: 399, gain:11.327000DB->x3.684258
	{0x1e, 0x83}, // index: 400, gain:11.280000DB->x3.664376
	{0x1e, 0x83}, // index: 401, gain:11.233000DB->x3.644601
	{0x1e, 0x82}, // index: 402, gain:11.186000DB->x3.624933
	{0x1e, 0x81}, // index: 403, gain:11.139000DB->x3.605371
	{0x1e, 0x80}, // index: 404, gain:11.092000DB->x3.585915
	{0x1e, 0x80}, // index: 405, gain:11.045000DB->x3.566564
	{0x1c, 0x8d}, // index: 406, gain:10.998000DB->x3.547317
	{0x1c, 0x8d}, // index: 407, gain:10.951000DB->x3.528174
	{0x1c, 0x8c}, // index: 408, gain:10.904000DB->x3.509134
	{0x1c, 0x8b}, // index: 409, gain:10.857000DB->x3.490197
	{0x1c, 0x8a}, // index: 410, gain:10.810000DB->x3.471363
	{0x1c, 0x8a}, // index: 411, gain:10.763000DB->x3.452630
	{0x1c, 0x89}, // index: 412, gain:10.716000DB->x3.433998
	{0x1c, 0x88}, // index: 413, gain:10.669000DB->x3.415466
	{0x1c, 0x87}, // index: 414, gain:10.622000DB->x3.397035
	{0x1c, 0x87}, // index: 415, gain:10.575000DB->x3.378703
	{0x1c, 0x86}, // index: 416, gain:10.528000DB->x3.360470
	{0x1c, 0x85}, // index: 417, gain:10.481000DB->x3.342335
	{0x1c, 0x84}, // index: 418, gain:10.434000DB->x3.324298
	{0x1c, 0x84}, // index: 419, gain:10.387000DB->x3.306359
	{0x1c, 0x83}, // index: 420, gain:10.340000DB->x3.288516
	{0x1c, 0x82}, // index: 421, gain:10.293000DB->x3.270770
	{0x1c, 0x82}, // index: 422, gain:10.246000DB->x3.253119
	{0x1c, 0x81}, // index: 423, gain:10.199000DB->x3.235564
	{0x1c, 0x80}, // index: 424, gain:10.152000DB->x3.218103
	{0x1c, 0x80}, // index: 425, gain:10.105000DB->x3.200737
	{0x1a, 0x8c}, // index: 426, gain:10.058000DB->x3.183464
	{0x1a, 0x8b}, // index: 427, gain:10.011000DB->x3.166285
	{0x1a, 0x8a}, // index: 428, gain:9.964000DB->x3.149198
	{0x1a, 0x89}, // index: 429, gain:9.917000DB->x3.132204
	{0x1a, 0x89}, // index: 430, gain:9.870000DB->x3.115301
	{0x1a, 0x88}, // index: 431, gain:9.823000DB->x3.098489
	{0x1a, 0x87}, // index: 432, gain:9.776000DB->x3.081768
	{0x1a, 0x86}, // index: 433, gain:9.729000DB->x3.065138
	{0x1a, 0x86}, // index: 434, gain:9.682000DB->x3.048597
	{0x1a, 0x85}, // index: 435, gain:9.635000DB->x3.032145
	{0x1a, 0x84}, // index: 436, gain:9.588000DB->x3.015782
	{0x1a, 0x83}, // index: 437, gain:9.541000DB->x2.999508
	{0x1a, 0x83}, // index: 438, gain:9.494000DB->x2.983321
	{0x1a, 0x82}, // index: 439, gain:9.447000DB->x2.967222
	{0x1a, 0x81}, // index: 440, gain:9.400000DB->x2.951209
	{0x1a, 0x81}, // index: 441, gain:9.353000DB->x2.935283
	{0x1a, 0x80}, // index: 442, gain:9.306000DB->x2.919443
	{0x18, 0x8b}, // index: 443, gain:9.259000DB->x2.903688
	{0x18, 0x8a}, // index: 444, gain:9.212000DB->x2.888019
	{0x18, 0x89}, // index: 445, gain:9.165000DB->x2.872434
	{0x18, 0x88}, // index: 446, gain:9.118000DB->x2.856933
	{0x18, 0x88}, // index: 447, gain:9.071000DB->x2.841515
	{0x18, 0x87}, // index: 448, gain:9.024000DB->x2.826181
	{0x18, 0x86}, // index: 449, gain:8.977000DB->x2.810930
	{0x18, 0x86}, // index: 450, gain:8.930000DB->x2.795761
	{0x18, 0x85}, // index: 451, gain:8.883000DB->x2.780674
	{0x18, 0x84}, // index: 452, gain:8.836000DB->x2.765668
	{0x18, 0x83}, // index: 453, gain:8.789000DB->x2.750743
	{0x18, 0x83}, // index: 454, gain:8.742000DB->x2.735899
	{0x18, 0x82}, // index: 455, gain:8.695000DB->x2.721134
	{0x18, 0x81}, // index: 456, gain:8.648000DB->x2.706450
	{0x18, 0x81}, // index: 457, gain:8.601000DB->x2.691845
	{0x18, 0x80}, // index: 458, gain:8.554000DB->x2.677318
	{0x16, 0x89}, // index: 459, gain:8.507000DB->x2.662870
	{0x16, 0x89}, // index: 460, gain:8.460000DB->x2.648500
	{0x16, 0x88}, // index: 461, gain:8.413000DB->x2.634208
	{0x16, 0x87}, // index: 462, gain:8.366000DB->x2.619992
	{0x16, 0x87}, // index: 463, gain:8.319000DB->x2.605854
	{0x16, 0x86}, // index: 464, gain:8.272000DB->x2.591791
	{0x16, 0x85}, // index: 465, gain:8.225000DB->x2.577805
	{0x16, 0x84}, // index: 466, gain:8.178000DB->x2.563894
	{0x16, 0x84}, // index: 467, gain:8.131000DB->x2.550058
	{0x16, 0x83}, // index: 468, gain:8.084000DB->x2.536296
	{0x16, 0x82}, // index: 469, gain:8.037000DB->x2.522609
	{0x16, 0x82}, // index: 470, gain:7.990000DB->x2.508996
	{0x16, 0x81}, // index: 471, gain:7.943000DB->x2.495456
	{0x16, 0x80}, // index: 472, gain:7.896000DB->x2.481990
	{0x14, 0x8a}, // index: 473, gain:7.849000DB->x2.468596
	{0x14, 0x89}, // index: 474, gain:7.802000DB->x2.455274
	{0x14, 0x89}, // index: 475, gain:7.755000DB->x2.442024
	{0x14, 0x88}, // index: 476, gain:7.708000DB->x2.428846
	{0x14, 0x87}, // index: 477, gain:7.661000DB->x2.415739
	{0x14, 0x86}, // index: 478, gain:7.614000DB->x2.402702
	{0x14, 0x86}, // index: 479, gain:7.567000DB->x2.389736
	{0x14, 0x85}, // index: 480, gain:7.520000DB->x2.376840
	{0x14, 0x84}, // index: 481, gain:7.473000DB->x2.364014
	{0x14, 0x84}, // index: 482, gain:7.426000DB->x2.351256
	{0x14, 0x83}, // index: 483, gain:7.379000DB->x2.338568
	{0x14, 0x82}, // index: 484, gain:7.332000DB->x2.325948
	{0x14, 0x81}, // index: 485, gain:7.285000DB->x2.313396
	{0x14, 0x81}, // index: 486, gain:7.238000DB->x2.300912
	{0x14, 0x80}, // index: 487, gain:7.191000DB->x2.288495
	{0x12, 0x88}, // index: 488, gain:7.144000DB->x2.276145
	{0x12, 0x87}, // index: 489, gain:7.097000DB->x2.263862
	{0x12, 0x86}, // index: 490, gain:7.050000DB->x2.251645
	{0x12, 0x85}, // index: 491, gain:7.003000DB->x2.239494
	{0x12, 0x85}, // index: 492, gain:6.956000DB->x2.227409
	{0x12, 0x84}, // index: 493, gain:6.909000DB->x2.215389
	{0x12, 0x83}, // index: 494, gain:6.862000DB->x2.203434
	{0x12, 0x83}, // index: 495, gain:6.815000DB->x2.191543
	{0x12, 0x82}, // index: 496, gain:6.768000DB->x2.179716
	{0x12, 0x81}, // index: 497, gain:6.721000DB->x2.167954
	{0x12, 0x80}, // index: 498, gain:6.674000DB->x2.156254
	{0x12, 0x80}, // index: 499, gain:6.627000DB->x2.144618
	{0x10, 0x88}, // index: 500, gain:6.580000DB->x2.133045
	{0x10, 0x87}, // index: 501, gain:6.533000DB->x2.121534
	{0x10, 0x87}, // index: 502, gain:6.486000DB->x2.110085
	{0x10, 0x86}, // index: 503, gain:6.439000DB->x2.098698
	{0x10, 0x85}, // index: 504, gain:6.392000DB->x2.087373
	{0x10, 0x84}, // index: 505, gain:6.345000DB->x2.076108
	{0x10, 0x84}, // index: 506, gain:6.298000DB->x2.064905
	{0x10, 0x83}, // index: 507, gain:6.251000DB->x2.053761
	{0x10, 0x82}, // index: 508, gain:6.204000DB->x2.042678
	{0x10, 0x82}, // index: 509, gain:6.157000DB->x2.031655
	{0x10, 0x81}, // index: 510, gain:6.110000DB->x2.020691
	{0x10, 0x80}, // index: 511, gain:6.063000DB->x2.009787
	{0x0f, 0x88}, // index: 512, gain:6.016000DB->x1.998941
	{0x0f, 0x87}, // index: 513, gain:5.969000DB->x1.988154
	{0x0f, 0x86}, // index: 514, gain:5.922000DB->x1.977425
	{0x0f, 0x85}, // index: 515, gain:5.875000DB->x1.966754
	{0x0f, 0x85}, // index: 516, gain:5.828000DB->x1.956140
	{0x0f, 0x84}, // index: 517, gain:5.781000DB->x1.945584
	{0x0f, 0x83}, // index: 518, gain:5.734000DB->x1.935085
	{0x0f, 0x83}, // index: 519, gain:5.687000DB->x1.924642
	{0x0f, 0x82}, // index: 520, gain:5.640000DB->x1.914256
	{0x0f, 0x81}, // index: 521, gain:5.593000DB->x1.903926
	{0x0f, 0x80}, // index: 522, gain:5.546000DB->x1.893651
	{0x0f, 0x80}, // index: 523, gain:5.499000DB->x1.883432
	{0x0e, 0x86}, // index: 524, gain:5.452000DB->x1.873268
	{0x0e, 0x85}, // index: 525, gain:5.405000DB->x1.863159
	{0x0e, 0x85}, // index: 526, gain:5.358000DB->x1.853105
	{0x0e, 0x84}, // index: 527, gain:5.311000DB->x1.843105
	{0x0e, 0x83}, // index: 528, gain:5.264000DB->x1.833158
	{0x0e, 0x83}, // index: 529, gain:5.217000DB->x1.823266
	{0x0e, 0x82}, // index: 530, gain:5.170000DB->x1.813427
	{0x0e, 0x81}, // index: 531, gain:5.123000DB->x1.803641
	{0x0e, 0x81}, // index: 532, gain:5.076000DB->x1.793907
	{0x0e, 0x80}, // index: 533, gain:5.029000DB->x1.784227
	{0x0d, 0x86}, // index: 534, gain:4.982000DB->x1.774598
	{0x0d, 0x85}, // index: 535, gain:4.935000DB->x1.765022
	{0x0d, 0x84}, // index: 536, gain:4.888000DB->x1.755497
	{0x0d, 0x84}, // index: 537, gain:4.841000DB->x1.746023
	{0x0d, 0x83}, // index: 538, gain:4.794000DB->x1.736601
	{0x0d, 0x82}, // index: 539, gain:4.747000DB->x1.727229
	{0x0d, 0x82}, // index: 540, gain:4.700000DB->x1.717908
	{0x0d, 0x81}, // index: 541, gain:4.653000DB->x1.708638
	{0x0d, 0x80}, // index: 542, gain:4.606000DB->x1.699417
	{0x0d, 0x80}, // index: 543, gain:4.559000DB->x1.690246
	{0x0c, 0x86}, // index: 544, gain:4.512000DB->x1.681125
	{0x0c, 0x85}, // index: 545, gain:4.465000DB->x1.672053
	{0x0c, 0x85}, // index: 546, gain:4.418000DB->x1.663030
	{0x0c, 0x84}, // index: 547, gain:4.371000DB->x1.654055
	{0x0c, 0x83}, // index: 548, gain:4.324000DB->x1.645129
	{0x0c, 0x82}, // index: 549, gain:4.277000DB->x1.636251
	{0x0c, 0x82}, // index: 550, gain:4.230000DB->x1.627421
	{0x0c, 0x81}, // index: 551, gain:4.183000DB->x1.618639
	{0x0c, 0x80}, // index: 552, gain:4.136000DB->x1.609904
	{0x0c, 0x80}, // index: 553, gain:4.089000DB->x1.601216
	{0x0b, 0x86}, // index: 554, gain:4.042000DB->x1.592575
	{0x0b, 0x85}, // index: 555, gain:3.995000DB->x1.583981
	{0x0b, 0x84}, // index: 556, gain:3.948000DB->x1.575433
	{0x0b, 0x83}, // index: 557, gain:3.901000DB->x1.566931
	{0x0b, 0x83}, // index: 558, gain:3.854000DB->x1.558476
	{0x0b, 0x82}, // index: 559, gain:3.807000DB->x1.550065
	{0x0b, 0x81}, // index: 560, gain:3.760000DB->x1.541700
	{0x0b, 0x81}, // index: 561, gain:3.713000DB->x1.533381
	{0x0b, 0x80}, // index: 562, gain:3.666000DB->x1.525106
	{0x0a, 0x85}, // index: 563, gain:3.619000DB->x1.516876
	{0x0a, 0x85}, // index: 564, gain:3.572000DB->x1.508690
	{0x0a, 0x84}, // index: 565, gain:3.525000DB->x1.500548
	{0x0a, 0x83}, // index: 566, gain:3.478000DB->x1.492451
	{0x0a, 0x83}, // index: 567, gain:3.431000DB->x1.484397
	{0x0a, 0x82}, // index: 568, gain:3.384000DB->x1.476386
	{0x0a, 0x81}, // index: 569, gain:3.337000DB->x1.468419
	{0x0a, 0x80}, // index: 570, gain:3.290000DB->x1.460495
	{0x0a, 0x80}, // index: 571, gain:3.243000DB->x1.452613
	{0x09, 0x85}, // index: 572, gain:3.196000DB->x1.444774
	{0x09, 0x84}, // index: 573, gain:3.149000DB->x1.436978
	{0x09, 0x83}, // index: 574, gain:3.102000DB->x1.429223
	{0x09, 0x82}, // index: 575, gain:3.055000DB->x1.421510
	{0x09, 0x82}, // index: 576, gain:3.008000DB->x1.413839
	{0x09, 0x81}, // index: 577, gain:2.961000DB->x1.406209
	{0x09, 0x80}, // index: 578, gain:2.914000DB->x1.398621
	{0x09, 0x80}, // index: 579, gain:2.867000DB->x1.391073
	{0x08, 0x84}, // index: 580, gain:2.820000DB->x1.383566
	{0x08, 0x83}, // index: 581, gain:2.773000DB->x1.376100
	{0x08, 0x82}, // index: 582, gain:2.726000DB->x1.368674
	{0x08, 0x82}, // index: 583, gain:2.679000DB->x1.361288
	{0x08, 0x81}, // index: 584, gain:2.632000DB->x1.353942
	{0x08, 0x80}, // index: 585, gain:2.585000DB->x1.346635
	{0x07, 0x85}, // index: 586, gain:2.538000DB->x1.339368
	{0x07, 0x85}, // index: 587, gain:2.491000DB->x1.332140
	{0x07, 0x84}, // index: 588, gain:2.444000DB->x1.324952
	{0x07, 0x83}, // index: 589, gain:2.397000DB->x1.317802
	{0x07, 0x83}, // index: 590, gain:2.350000DB->x1.310690
	{0x07, 0x82}, // index: 591, gain:2.303000DB->x1.303617
	{0x07, 0x81}, // index: 592, gain:2.256000DB->x1.296582
	{0x07, 0x80}, // index: 593, gain:2.209000DB->x1.289585
	{0x07, 0x80}, // index: 594, gain:2.162000DB->x1.282626
	{0x06, 0x84}, // index: 595, gain:2.115000DB->x1.275704
	{0x06, 0x84}, // index: 596, gain:2.068000DB->x1.268820
	{0x06, 0x83}, // index: 597, gain:2.021000DB->x1.261973
	{0x06, 0x82}, // index: 598, gain:1.974000DB->x1.255163
	{0x06, 0x81}, // index: 599, gain:1.927000DB->x1.248389
	{0x06, 0x81}, // index: 600, gain:1.880000DB->x1.241652
	{0x06, 0x80}, // index: 601, gain:1.833000DB->x1.234952
};


