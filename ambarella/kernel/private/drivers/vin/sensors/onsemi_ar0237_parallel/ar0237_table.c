/*
 * Filename : ar0237_table.c
 *
 * History:
 *    2015/09/07 - [Hao Zeng] Create
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

static struct vin_reg_16_16 ar0237_pll_regs[][6] = {
	{
		{0x302A, 0x0008}, /* VT_PIX_CLK_DIV  */
		{0x302C, 0x0001}, /* VT_SYS_CLK_DIV  */
		{0x302E, 0x0002}, /* PRE_PLL_CLK_DIV */
		{0x3030, 0x002C}, /* PLL_MULTIPLIER  */
		{0x3036, 0x000C}, /* OP_PIX_CLK_DIV  */
		{0x3038, 0x0001}, /* OP_SYS_CLK_DIV  */
	},
};

static struct vin_video_pll ar0237_plls[] = {
	{0, 27000000, 37125000},
};

static struct vin_video_format ar0237_formats[] = {
	{
		.video_mode	= AMBA_VIDEO_MODE_1080P,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 1920,
		.def_height	= 1080,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1920,
		.act_height	= 1080,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_LINEAR_MODE,
		.device_mode	= 0,
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
};

static struct vin_reg_16_16 ar0237_linear_share_regs[] = {
	/* 1080p@30fps-parallel mode(27M--74.25M) */
	{0x3088, 0x8000}, /* SEQ_CTRL_PORT */
	{0x3086, 0x4558}, /* SEQ_DATA_PORT */
	{0x3086, 0x72A6}, /* SEQ_DATA_PORT */
	{0x3086, 0x4A31}, /* SEQ_DATA_PORT */
	{0x3086, 0x4342}, /* SEQ_DATA_PORT */
	{0x3086, 0x8E03}, /* SEQ_DATA_PORT */
	{0x3086, 0x2A14}, /* SEQ_DATA_PORT */
	{0x3086, 0x4578}, /* SEQ_DATA_PORT */
	{0x3086, 0x7B3D}, /* SEQ_DATA_PORT */
	{0x3086, 0xFF3D}, /* SEQ_DATA_PORT */
	{0x3086, 0xFF3D}, /* SEQ_DATA_PORT */
	{0x3086, 0xEA2A}, /* SEQ_DATA_PORT */
	{0x3086, 0x043D}, /* SEQ_DATA_PORT */
	{0x3086, 0x102A}, /* SEQ_DATA_PORT */
	{0x3086, 0x052A}, /* SEQ_DATA_PORT */
	{0x3086, 0x1535}, /* SEQ_DATA_PORT */
	{0x3086, 0x2A05}, /* SEQ_DATA_PORT */
	{0x3086, 0x3D10}, /* SEQ_DATA_PORT */
	{0x3086, 0x4558}, /* SEQ_DATA_PORT */
	{0x3086, 0x2A04}, /* SEQ_DATA_PORT */
	{0x3086, 0x2A14}, /* SEQ_DATA_PORT */
	{0x3086, 0x3DFF}, /* SEQ_DATA_PORT */
	{0x3086, 0x3DFF}, /* SEQ_DATA_PORT */
	{0x3086, 0x3DEA}, /* SEQ_DATA_PORT */
	{0x3086, 0x2A04}, /* SEQ_DATA_PORT */
	{0x3086, 0x622A}, /* SEQ_DATA_PORT */
	{0x3086, 0x288E}, /* SEQ_DATA_PORT */
	{0x3086, 0x0036}, /* SEQ_DATA_PORT */
	{0x3086, 0x2A08}, /* SEQ_DATA_PORT */
	{0x3086, 0x3D64}, /* SEQ_DATA_PORT */
	{0x3086, 0x7A3D}, /* SEQ_DATA_PORT */
	{0x3086, 0x0444}, /* SEQ_DATA_PORT */
	{0x3086, 0x2C4B}, /* SEQ_DATA_PORT */
	{0x3086, 0xA403}, /* SEQ_DATA_PORT */
	{0x3086, 0x430D}, /* SEQ_DATA_PORT */
	{0x3086, 0x2D46}, /* SEQ_DATA_PORT */
	{0x3086, 0x4316}, /* SEQ_DATA_PORT */
	{0x3086, 0x2A90}, /* SEQ_DATA_PORT */
	{0x3086, 0x3E06}, /* SEQ_DATA_PORT */
	{0x3086, 0x2A98}, /* SEQ_DATA_PORT */
	{0x3086, 0x5F16}, /* SEQ_DATA_PORT */
	{0x3086, 0x530D}, /* SEQ_DATA_PORT */
	{0x3086, 0x1660}, /* SEQ_DATA_PORT */
	{0x3086, 0x3E4C}, /* SEQ_DATA_PORT */
	{0x3086, 0x2904}, /* SEQ_DATA_PORT */
	{0x3086, 0x2984}, /* SEQ_DATA_PORT */
	{0x3086, 0x8E03}, /* SEQ_DATA_PORT */
	{0x3086, 0x2AFC}, /* SEQ_DATA_PORT */
	{0x3086, 0x5C1D}, /* SEQ_DATA_PORT */
	{0x3086, 0x5754}, /* SEQ_DATA_PORT */
	{0x3086, 0x495F}, /* SEQ_DATA_PORT */
	{0x3086, 0x5305}, /* SEQ_DATA_PORT */
	{0x3086, 0x5307}, /* SEQ_DATA_PORT */
	{0x3086, 0x4D2B}, /* SEQ_DATA_PORT */
	{0x3086, 0xF810}, /* SEQ_DATA_PORT */
	{0x3086, 0x164C}, /* SEQ_DATA_PORT */
	{0x3086, 0x0955}, /* SEQ_DATA_PORT */
	{0x3086, 0x562B}, /* SEQ_DATA_PORT */
	{0x3086, 0xB82B}, /* SEQ_DATA_PORT */
	{0x3086, 0x984E}, /* SEQ_DATA_PORT */
	{0x3086, 0x1129}, /* SEQ_DATA_PORT */
	{0x3086, 0x9460}, /* SEQ_DATA_PORT */
	{0x3086, 0x5C19}, /* SEQ_DATA_PORT */
	{0x3086, 0x5C1B}, /* SEQ_DATA_PORT */
	{0x3086, 0x4548}, /* SEQ_DATA_PORT */
	{0x3086, 0x4508}, /* SEQ_DATA_PORT */
	{0x3086, 0x4588}, /* SEQ_DATA_PORT */
	{0x3086, 0x29B6}, /* SEQ_DATA_PORT */
	{0x3086, 0x8E01}, /* SEQ_DATA_PORT */
	{0x3086, 0x2AF8}, /* SEQ_DATA_PORT */
	{0x3086, 0x3E02}, /* SEQ_DATA_PORT */
	{0x3086, 0x2AFA}, /* SEQ_DATA_PORT */
	{0x3086, 0x3F09}, /* SEQ_DATA_PORT */
	{0x3086, 0x5C1B}, /* SEQ_DATA_PORT */
	{0x3086, 0x29B2}, /* SEQ_DATA_PORT */
	{0x3086, 0x3F0C}, /* SEQ_DATA_PORT */
	{0x3086, 0x3E03}, /* SEQ_DATA_PORT */
	{0x3086, 0x3E15}, /* SEQ_DATA_PORT */
	{0x3086, 0x5C13}, /* SEQ_DATA_PORT */
	{0x3086, 0x3F11}, /* SEQ_DATA_PORT */
	{0x3086, 0x3E0F}, /* SEQ_DATA_PORT */
	{0x3086, 0x5F2B}, /* SEQ_DATA_PORT */
	{0x3086, 0x902B}, /* SEQ_DATA_PORT */
	{0x3086, 0x803E}, /* SEQ_DATA_PORT */
	{0x3086, 0x062A}, /* SEQ_DATA_PORT */
	{0x3086, 0xF23F}, /* SEQ_DATA_PORT */
	{0x3086, 0x103E}, /* SEQ_DATA_PORT */
	{0x3086, 0x0160}, /* SEQ_DATA_PORT */
	{0x3086, 0x29A2}, /* SEQ_DATA_PORT */
	{0x3086, 0x29A3}, /* SEQ_DATA_PORT */
	{0x3086, 0x5F4D}, /* SEQ_DATA_PORT */
	{0x3086, 0x1C2A}, /* SEQ_DATA_PORT */
	{0x3086, 0xFA29}, /* SEQ_DATA_PORT */
	{0x3086, 0x8345}, /* SEQ_DATA_PORT */
	{0x3086, 0xA83E}, /* SEQ_DATA_PORT */
	{0x3086, 0x072A}, /* SEQ_DATA_PORT */
	{0x3086, 0xFB3E}, /* SEQ_DATA_PORT */
	{0x3086, 0x2945}, /* SEQ_DATA_PORT */
	{0x3086, 0x8824}, /* SEQ_DATA_PORT */
	{0x3086, 0x3E08}, /* SEQ_DATA_PORT */
	{0x3086, 0x2AFA}, /* SEQ_DATA_PORT */
	{0x3086, 0x5D29}, /* SEQ_DATA_PORT */
	{0x3086, 0x9288}, /* SEQ_DATA_PORT */
	{0x3086, 0x102B}, /* SEQ_DATA_PORT */
	{0x3086, 0x048B}, /* SEQ_DATA_PORT */
	{0x3086, 0x1686}, /* SEQ_DATA_PORT */
	{0x3086, 0x8D48}, /* SEQ_DATA_PORT */
	{0x3086, 0x4D4E}, /* SEQ_DATA_PORT */
	{0x3086, 0x2B80}, /* SEQ_DATA_PORT */
	{0x3086, 0x4C0B}, /* SEQ_DATA_PORT */
	{0x3086, 0x3F36}, /* SEQ_DATA_PORT */
	{0x3086, 0x2AF2}, /* SEQ_DATA_PORT */
	{0x3086, 0x3F10}, /* SEQ_DATA_PORT */
	{0x3086, 0x3E01}, /* SEQ_DATA_PORT */
	{0x3086, 0x6029}, /* SEQ_DATA_PORT */
	{0x3086, 0x8229}, /* SEQ_DATA_PORT */
	{0x3086, 0x8329}, /* SEQ_DATA_PORT */
	{0x3086, 0x435C}, /* SEQ_DATA_PORT */
	{0x3086, 0x155F}, /* SEQ_DATA_PORT */
	{0x3086, 0x4D1C}, /* SEQ_DATA_PORT */
	{0x3086, 0x2AFA}, /* SEQ_DATA_PORT */
	{0x3086, 0x4558}, /* SEQ_DATA_PORT */
	{0x3086, 0x8E00}, /* SEQ_DATA_PORT */
	{0x3086, 0x2A98}, /* SEQ_DATA_PORT */
	{0x3086, 0x3F0A}, /* SEQ_DATA_PORT */
	{0x3086, 0x4A0A}, /* SEQ_DATA_PORT */
	{0x3086, 0x4316}, /* SEQ_DATA_PORT */
	{0x3086, 0x0B43}, /* SEQ_DATA_PORT */
	{0x3086, 0x168E}, /* SEQ_DATA_PORT */
	{0x3086, 0x032A}, /* SEQ_DATA_PORT */
	{0x3086, 0x9C45}, /* SEQ_DATA_PORT */
	{0x3086, 0x783F}, /* SEQ_DATA_PORT */
	{0x3086, 0x072A}, /* SEQ_DATA_PORT */
	{0x3086, 0x9D3E}, /* SEQ_DATA_PORT */
	{0x3086, 0x305D}, /* SEQ_DATA_PORT */
	{0x3086, 0x2944}, /* SEQ_DATA_PORT */
	{0x3086, 0x8810}, /* SEQ_DATA_PORT */
	{0x3086, 0x2B04}, /* SEQ_DATA_PORT */
	{0x3086, 0x530D}, /* SEQ_DATA_PORT */
	{0x3086, 0x4558}, /* SEQ_DATA_PORT */
	{0x3086, 0x3E08}, /* SEQ_DATA_PORT */
	{0x3086, 0x8E01}, /* SEQ_DATA_PORT */
	{0x3086, 0x2A98}, /* SEQ_DATA_PORT */
	{0x3086, 0x8E00}, /* SEQ_DATA_PORT */
	{0x3086, 0x76A7}, /* SEQ_DATA_PORT */
	{0x3086, 0x77A7}, /* SEQ_DATA_PORT */
	{0x3086, 0x4644}, /* SEQ_DATA_PORT */
	{0x3086, 0x1616}, /* SEQ_DATA_PORT */
	{0x3086, 0xA57A}, /* SEQ_DATA_PORT */
	{0x3086, 0x1244}, /* SEQ_DATA_PORT */
	{0x3086, 0x4B18}, /* SEQ_DATA_PORT */
	{0x3086, 0x4A04}, /* SEQ_DATA_PORT */
	{0x3086, 0x4316}, /* SEQ_DATA_PORT */
	{0x3086, 0x0643}, /* SEQ_DATA_PORT */
	{0x3086, 0x1605}, /* SEQ_DATA_PORT */
	{0x3086, 0x4316}, /* SEQ_DATA_PORT */
	{0x3086, 0x0743}, /* SEQ_DATA_PORT */
	{0x3086, 0x1658}, /* SEQ_DATA_PORT */
	{0x3086, 0x4316}, /* SEQ_DATA_PORT */
	{0x3086, 0x5A43}, /* SEQ_DATA_PORT */
	{0x3086, 0x1645}, /* SEQ_DATA_PORT */
	{0x3086, 0x588E}, /* SEQ_DATA_PORT */
	{0x3086, 0x032A}, /* SEQ_DATA_PORT */
	{0x3086, 0x9C45}, /* SEQ_DATA_PORT */
	{0x3086, 0x787B}, /* SEQ_DATA_PORT */
	{0x3086, 0x3F07}, /* SEQ_DATA_PORT */
	{0x3086, 0x2A9D}, /* SEQ_DATA_PORT */
	{0x3086, 0x530D}, /* SEQ_DATA_PORT */
	{0x3086, 0x8B16}, /* SEQ_DATA_PORT */
	{0x3086, 0x863E}, /* SEQ_DATA_PORT */
	{0x3086, 0x2345}, /* SEQ_DATA_PORT */
	{0x3086, 0x5825}, /* SEQ_DATA_PORT */
	{0x3086, 0x3E10}, /* SEQ_DATA_PORT */
	{0x3086, 0x8E01}, /* SEQ_DATA_PORT */
	{0x3086, 0x2A98}, /* SEQ_DATA_PORT */
	{0x3086, 0x8E00}, /* SEQ_DATA_PORT */
	{0x3086, 0x3E10}, /* SEQ_DATA_PORT */
	{0x3086, 0x8D60}, /* SEQ_DATA_PORT */
	{0x3086, 0x1244}, /* SEQ_DATA_PORT */
	{0x3086, 0x4BB9}, /* SEQ_DATA_PORT */
	{0x3086, 0x2C2C}, /* SEQ_DATA_PORT */
	{0x3086, 0x2C2C}, /* SEQ_DATA_PORT */

	{0x30F0, 0x1283}, /* ADC_COMMAND1_HS */
	{0x3064, 0x1802}, /* SMIA_TEST */
	{0x3EEE, 0xA0AA}, /* DAC_LD_34_35 */
	{0x30BA, 0x762C}, /* DIGITAL_CTRL */
	{0x3FA4, 0x0F70},
	{0x309E, 0x016A}, /* ERS_PROG_START_ADDR */
	{0x3096, 0xF880}, /* ROW_NOISE_ADJUST_TOP */
	{0x3F32, 0xF880}, /* ROW_NOISE_ADJUST_TOP_T1 */
	{0x3092, 0x006F}, /* ROW_NOISE_CONTROL */
	{0x301A, 0x10D8}, /* RESET_REGISTER */
	{0x30B0, 0x1118}, /* DIGITAL_TEST(0x0118) */
	{0x31AC, 0x0C0C}, /* DATA_FORMAT_BITS */

	{0x302A, 0x0008}, /* VT_PIX_CLK_DIV */
	{0x302C, 0x0001}, /* VT_SYS_CLK_DIV */
	{0x302E, 0x0002}, /* PRE_PLL_CLK_DIV */
	{0x3030, 0x002C}, /* PLL_MULTIPLIER */
	{0x3036, 0x000C}, /* OP_PIX_CLK_DIV */
	{0x3038, 0x0001}, /* OP_SYS_CLK_DIV */

	{0x3002, 0x0004}, /* Y_ADDR_START */
	{0x3004, 0x000c}, /* X_ADDR_START */
	{0x3006, 0x043b}, /* Y_ADDR_END */
	{0x3008, 0x078b}, /* X_ADDR_END(0x0787) */
	{0x300A, 0x0465}, /* FRAME_LENGTH_LINES(1125) */
	{0x300C, 0x044C}, /* LINE_LENGTH_PCK(1100) */
	{0x3012, 0x0416}, /* COARSE_INTEGRATION_TIME */
	{0x30A2, 0x0001}, /* X_ODD_INC */
	{0x30A6, 0x0001}, /* Y_ODD_INC */
	{0x30AE, 0x0001}, /* X_ODD_INC_CB */
	{0x30A8, 0x0001}, /* Y_ODD_INC_CB */

	{0x3040, 0x0000}, /* READ_MODE */
	{0x31AE, 0x0301}, /* SERIAL_FORMAT */
	{0x3082, 0x0009}, /* OPERATION_MODE_CTRL */
	{0x30BA, 0x762C}, /* DIGITAL_CTRL */
	{0x3096, 0x0080}, /* ROW_NOISE_ADJUST_TOP */
	{0x3098, 0x0080}, /* ROW_NOISE_ADJUST_BTM */
	{0x3060, 0x000B}, /* ANALOG_GAIN */
	{0x3100, 0x0004}, /* AECTRLREG */
	{0x31D0, 0x0000}, /* COMPANDING */
	{0x3064, 0x1802}, /* SMIA_TEST */
	{0x301A, 0x10DC}, /* RESET_REGISTER */
};

/** AR0237 global gain table row size */
#define AR0237_GAIN_ROWS		475
#define AR0237_GAIN_COLS 		3

#define AR0237_GAIN_48DB		474
#define AR0237_GAIN_DCG_ON	54

#define AR0237_GAIN_COL_AGAIN		0
#define AR0237_GAIN_COL_DGAIN		1
#define AR0237_GAIN_COL_DCG		2

const u16 AR0237_GAIN_TABLE[AR0237_GAIN_ROWS][AR0237_GAIN_COLS] = {
	/* analog gain 1.52x~43.2x, and if gain >=2.7, turn on DCG */
	{0x000b, 0x0080, 0x0000}, /* index: 0, gain:3.636900db->x1.520005, DCG:0 */
	{0x000b, 0x0081, 0x0000}, /* index: 1, gain:3.730650db->x1.536500, DCG:0 */
	{0x000b, 0x0082, 0x0000}, /* index: 2, gain:3.824400db->x1.553174, DCG:0 */
	{0x000b, 0x0084, 0x0000}, /* index: 3, gain:3.918150db->x1.570028, DCG:0 */
	{0x000b, 0x0085, 0x0000}, /* index: 4, gain:4.011900db->x1.587066, DCG:0 */
	{0x000c, 0x0080, 0x0000}, /* index: 5, gain:4.105650db->x1.604289, DCG:0 */
	{0x000c, 0x0081, 0x0000}, /* index: 6, gain:4.199400db->x1.621698, DCG:0 */
	{0x000c, 0x0083, 0x0000}, /* index: 7, gain:4.293150db->x1.639296, DCG:0 */
	{0x000c, 0x0084, 0x0000}, /* index: 8, gain:4.386900db->x1.657086, DCG:0 */
	{0x000c, 0x0086, 0x0000}, /* index: 9, gain:4.480650db->x1.675068, DCG:0 */
	{0x000d, 0x0081, 0x0000}, /* index: 10, gain:4.574400db->x1.693246, DCG:0 */
	{0x000d, 0x0082, 0x0000}, /* index: 11, gain:4.668150db->x1.711621, DCG:0 */
	{0x000d, 0x0083, 0x0000}, /* index: 12, gain:4.761900db->x1.730195, DCG:0 */
	{0x000d, 0x0085, 0x0000}, /* index: 13, gain:4.855650db->x1.748971, DCG:0 */
	{0x000d, 0x0086, 0x0000}, /* index: 14, gain:4.949400db->x1.767950, DCG:0 */
	{0x000e, 0x0080, 0x0000}, /* index: 15, gain:5.043150db->x1.787136, DCG:0 */
	{0x000e, 0x0081, 0x0000}, /* index: 16, gain:5.136900db->x1.806529, DCG:0 */
	{0x000e, 0x0083, 0x0000}, /* index: 17, gain:5.230650db->x1.826133, DCG:0 */
	{0x000e, 0x0084, 0x0000}, /* index: 18, gain:5.324400db->x1.845950, DCG:0 */
	{0x000e, 0x0086, 0x0000}, /* index: 19, gain:5.418150db->x1.865982, DCG:0 */
	{0x000f, 0x0080, 0x0000}, /* index: 20, gain:5.511900db->x1.886232, DCG:0 */
	{0x000f, 0x0081, 0x0000}, /* index: 21, gain:5.605650db->x1.906701, DCG:0 */
	{0x000f, 0x0083, 0x0000}, /* index: 22, gain:5.699400db->x1.927392, DCG:0 */
	{0x000f, 0x0084, 0x0000}, /* index: 23, gain:5.793150db->x1.948307, DCG:0 */
	{0x000f, 0x0086, 0x0000}, /* index: 24, gain:5.886900db->x1.969450, DCG:0 */
	{0x000f, 0x0087, 0x0000}, /* index: 25, gain:5.980650db->x1.990822, DCG:0 */
	{0x0010, 0x0080, 0x0000}, /* index: 26, gain:6.074400db->x2.012426, DCG:0 */
	{0x0010, 0x0082, 0x0000}, /* index: 27, gain:6.168150db->x2.034265, DCG:0 */
	{0x0010, 0x0083, 0x0000}, /* index: 28, gain:6.261900db->x2.056340, DCG:0 */
	{0x0011, 0x0081, 0x0000}, /* index: 29, gain:6.355650db->x2.078655, DCG:0 */
	{0x0011, 0x0082, 0x0000}, /* index: 30, gain:6.449400db->x2.101213, DCG:0 */
	{0x0011, 0x0083, 0x0000}, /* index: 31, gain:6.543150db->x2.124015, DCG:0 */
	{0x0012, 0x0080, 0x0000}, /* index: 32, gain:6.636900db->x2.147064, DCG:0 */
	{0x0012, 0x0081, 0x0000}, /* index: 33, gain:6.730650db->x2.170364, DCG:0 */
	{0x0012, 0x0083, 0x0000}, /* index: 34, gain:6.824400db->x2.193916, DCG:0 */
	{0x0013, 0x0081, 0x0000}, /* index: 35, gain:6.918150db->x2.217724, DCG:0 */
	{0x0013, 0x0082, 0x0000}, /* index: 36, gain:7.011900db->x2.241790, DCG:0 */
	{0x0013, 0x0083, 0x0000}, /* index: 37, gain:7.105650db->x2.266118, DCG:0 */
	{0x0014, 0x0080, 0x0000}, /* index: 38, gain:7.199400db->x2.290709, DCG:0 */
	{0x0014, 0x0081, 0x0000}, /* index: 39, gain:7.293150db->x2.315568, DCG:0 */
	{0x0014, 0x0083, 0x0000}, /* index: 40, gain:7.386900db->x2.340696, DCG:0 */
	{0x0014, 0x0084, 0x0000}, /* index: 41, gain:7.480650db->x2.366097, DCG:0 */
	{0x0015, 0x0080, 0x0000}, /* index: 42, gain:7.574400db->x2.391773, DCG:0 */
	{0x0015, 0x0082, 0x0000}, /* index: 43, gain:7.668150db->x2.417728, DCG:0 */
	{0x0015, 0x0083, 0x0000}, /* index: 44, gain:7.761900db->x2.443965, DCG:0 */
	{0x0016, 0x0080, 0x0000}, /* index: 45, gain:7.855650db->x2.470487, DCG:0 */
	{0x0016, 0x0081, 0x0000}, /* index: 46, gain:7.949400db->x2.497296, DCG:0 */
	{0x0016, 0x0083, 0x0000}, /* index: 47, gain:8.043150db->x2.524396, DCG:0 */
	{0x0016, 0x0084, 0x0000}, /* index: 48, gain:8.136900db->x2.551790, DCG:0 */
	{0x0017, 0x0080, 0x0000}, /* index: 49, gain:8.230650db->x2.579482, DCG:0 */
	{0x0017, 0x0082, 0x0000}, /* index: 50, gain:8.324400db->x2.607474, DCG:0 */
	{0x0017, 0x0083, 0x0000}, /* index: 51, gain:8.418150db->x2.635770, DCG:0 */
	{0x0018, 0x0080, 0x0000}, /* index: 52, gain:8.511900db->x2.664373, DCG:0 */
	{0x0018, 0x0081, 0x0000}, /* index: 53, gain:8.605650db->x2.693286, DCG:0 */
	{0x0000, 0x0081, 0x0004}, /* index: 54, gain:8.699400db->x2.722513, DCG:1 */
	{0x0000, 0x0082, 0x0004}, /* index: 55, gain:8.793150db->x2.752057, DCG:1 */
	{0x0001, 0x0080, 0x0004}, /* index: 56, gain:8.886900db->x2.781922, DCG:1 */
	{0x0001, 0x0081, 0x0004}, /* index: 57, gain:8.980650db->x2.812111, DCG:1 */
	{0x0001, 0x0082, 0x0004}, /* index: 58, gain:9.074400db->x2.842628, DCG:1 */
	{0x0001, 0x0084, 0x0004}, /* index: 59, gain:9.168150db->x2.873476, DCG:1 */
	{0x0002, 0x0080, 0x0004}, /* index: 60, gain:9.261900db->x2.904658, DCG:1 */
	{0x0002, 0x0082, 0x0004}, /* index: 61, gain:9.355650db->x2.936179, DCG:1 */
	{0x0002, 0x0083, 0x0004}, /* index: 62, gain:9.449400db->x2.968042, DCG:1 */
	{0x0003, 0x0081, 0x0004}, /* index: 63, gain:9.543150db->x3.000250, DCG:1 */
	{0x0003, 0x0082, 0x0004}, /* index: 64, gain:9.636900db->x3.032809, DCG:1 */
	{0x0003, 0x0084, 0x0004}, /* index: 65, gain:9.730650db->x3.065720, DCG:1 */
	{0x0004, 0x0080, 0x0004}, /* index: 66, gain:9.824400db->x3.098989, DCG:1 */
	{0x0004, 0x0082, 0x0004}, /* index: 67, gain:9.918150db->x3.132618, DCG:1 */
	{0x0004, 0x0083, 0x0004}, /* index: 68, gain:10.011900db->x3.166613, DCG:1 */
	{0x0004, 0x0085, 0x0004}, /* index: 69, gain:10.105650db->x3.200977, DCG:1 */
	{0x0005, 0x0080, 0x0004}, /* index: 70, gain:10.199400db->x3.235713, DCG:1 */
	{0x0005, 0x0082, 0x0004}, /* index: 71, gain:10.293150db->x3.270826, DCG:1 */
	{0x0005, 0x0083, 0x0004}, /* index: 72, gain:10.386900db->x3.306321, DCG:1 */
	{0x0006, 0x0080, 0x0004}, /* index: 73, gain:10.480650db->x3.342201, DCG:1 */
	{0x0006, 0x0082, 0x0004}, /* index: 74, gain:10.574400db->x3.378469, DCG:1 */
	{0x0006, 0x0083, 0x0004}, /* index: 75, gain:10.668150db->x3.415132, DCG:1 */
	{0x0006, 0x0085, 0x0004}, /* index: 76, gain:10.761900db->x3.452192, DCG:1 */
	{0x0007, 0x0081, 0x0004}, /* index: 77, gain:10.855650db->x3.489655, DCG:1 */
	{0x0007, 0x0082, 0x0004}, /* index: 78, gain:10.949400db->x3.527524, DCG:1 */
	{0x0007, 0x0084, 0x0004}, /* index: 79, gain:11.043150db->x3.565804, DCG:1 */
	{0x0008, 0x0080, 0x0004}, /* index: 80, gain:11.136900db->x3.604500, DCG:1 */
	{0x0008, 0x0081, 0x0004}, /* index: 81, gain:11.230650db->x3.643615, DCG:1 */
	{0x0008, 0x0083, 0x0004}, /* index: 82, gain:11.324400db->x3.683155, DCG:1 */
	{0x0008, 0x0084, 0x0004}, /* index: 83, gain:11.418150db->x3.723124, DCG:1 */
	{0x0009, 0x0080, 0x0004}, /* index: 84, gain:11.511900db->x3.763527, DCG:1 */
	{0x0009, 0x0081, 0x0004}, /* index: 85, gain:11.605650db->x3.804368, DCG:1 */
	{0x0009, 0x0083, 0x0004}, /* index: 86, gain:11.699400db->x3.845652, DCG:1 */
	{0x0009, 0x0084, 0x0004}, /* index: 87, gain:11.793150db->x3.887385, DCG:1 */
	{0x0009, 0x0086, 0x0004}, /* index: 88, gain:11.886900db->x3.929570, DCG:1 */
	{0x000a, 0x0080, 0x0004}, /* index: 89, gain:11.980650db->x3.972213, DCG:1 */
	{0x000a, 0x0082, 0x0004}, /* index: 90, gain:12.074400db->x4.015319, DCG:1 */
	{0x000a, 0x0083, 0x0004}, /* index: 91, gain:12.168150db->x4.058892, DCG:1 */
	{0x000a, 0x0085, 0x0004}, /* index: 92, gain:12.261900db->x4.102938, DCG:1 */
	{0x000b, 0x0081, 0x0004}, /* index: 93, gain:12.355650db->x4.147463, DCG:1 */
	{0x000b, 0x0082, 0x0004}, /* index: 94, gain:12.449400db->x4.192470, DCG:1 */
	{0x000b, 0x0084, 0x0004}, /* index: 95, gain:12.543150db->x4.237966, DCG:1 */
	{0x000b, 0x0085, 0x0004}, /* index: 96, gain:12.636900db->x4.283956, DCG:1 */
	{0x000c, 0x0080, 0x0004}, /* index: 97, gain:12.730650db->x4.330445, DCG:1 */
	{0x000c, 0x0081, 0x0004}, /* index: 98, gain:12.824400db->x4.377438, DCG:1 */
	{0x000c, 0x0083, 0x0004}, /* index: 99, gain:12.918150db->x4.424941, DCG:1 */
	{0x000c, 0x0084, 0x0004}, /* index: 100, gain:13.011900db->x4.472960, DCG:1 */
	{0x000c, 0x0085, 0x0004}, /* index: 101, gain:13.105650db->x4.521500, DCG:1 */
	{0x000d, 0x0080, 0x0004}, /* index: 102, gain:13.199400db->x4.570566, DCG:1 */
	{0x000d, 0x0082, 0x0004}, /* index: 103, gain:13.293150db->x4.620165, DCG:1 */
	{0x000d, 0x0083, 0x0004}, /* index: 104, gain:13.386900db->x4.670302, DCG:1 */
	{0x000d, 0x0085, 0x0004}, /* index: 105, gain:13.480650db->x4.720984, DCG:1 */
	{0x000d, 0x0086, 0x0004}, /* index: 106, gain:13.574400db->x4.772215, DCG:1 */
	{0x000e, 0x0080, 0x0004}, /* index: 107, gain:13.668150db->x4.824002, DCG:1 */
	{0x000e, 0x0081, 0x0004}, /* index: 108, gain:13.761900db->x4.876351, DCG:1 */
	{0x000e, 0x0083, 0x0004}, /* index: 109, gain:13.855650db->x4.929269, DCG:1 */
	{0x000e, 0x0084, 0x0004}, /* index: 110, gain:13.949400db->x4.982760, DCG:1 */
	{0x000e, 0x0086, 0x0004}, /* index: 111, gain:14.043150db->x5.036832, DCG:1 */
	{0x000f, 0x0080, 0x0004}, /* index: 112, gain:14.136900db->x5.091491, DCG:1 */
	{0x000f, 0x0081, 0x0004}, /* index: 113, gain:14.230650db->x5.146743, DCG:1 */
	{0x000f, 0x0083, 0x0004}, /* index: 114, gain:14.324400db->x5.202595, DCG:1 */
	{0x000f, 0x0084, 0x0004}, /* index: 115, gain:14.418150db->x5.259052, DCG:1 */
	{0x000f, 0x0086, 0x0004}, /* index: 116, gain:14.511900db->x5.316123, DCG:1 */
	{0x000f, 0x0087, 0x0004}, /* index: 117, gain:14.605650db->x5.373812, DCG:1 */
	{0x0010, 0x0080, 0x0004}, /* index: 118, gain:14.699400db->x5.432128, DCG:1 */
	{0x0010, 0x0082, 0x0004}, /* index: 119, gain:14.793150db->x5.491077, DCG:1 */
	{0x0010, 0x0083, 0x0004}, /* index: 120, gain:14.886900db->x5.550665, DCG:1 */
	{0x0011, 0x0081, 0x0004}, /* index: 121, gain:14.980650db->x5.610900, DCG:1 */
	{0x0011, 0x0082, 0x0004}, /* index: 122, gain:15.074400db->x5.671788, DCG:1 */
	{0x0011, 0x0083, 0x0004}, /* index: 123, gain:15.168150db->x5.733337, DCG:1 */
	{0x0012, 0x0080, 0x0004}, /* index: 124, gain:15.261900db->x5.795555, DCG:1 */
	{0x0012, 0x0081, 0x0004}, /* index: 125, gain:15.355650db->x5.858447, DCG:1 */
	{0x0012, 0x0083, 0x0004}, /* index: 126, gain:15.449400db->x5.922022, DCG:1 */
	{0x0013, 0x0080, 0x0004}, /* index: 127, gain:15.543150db->x5.986287, DCG:1 */
	{0x0013, 0x0082, 0x0004}, /* index: 128, gain:15.636900db->x6.051249, DCG:1 */
	{0x0013, 0x0083, 0x0004}, /* index: 129, gain:15.730650db->x6.116916, DCG:1 */
	{0x0014, 0x0080, 0x0004}, /* index: 130, gain:15.824400db->x6.183295, DCG:1 */
	{0x0014, 0x0081, 0x0004}, /* index: 131, gain:15.918150db->x6.250396, DCG:1 */
	{0x0014, 0x0083, 0x0004}, /* index: 132, gain:16.011900db->x6.318224, DCG:1 */
	{0x0014, 0x0084, 0x0004}, /* index: 133, gain:16.105650db->x6.386788, DCG:1 */
	{0x0015, 0x0080, 0x0004}, /* index: 134, gain:16.199400db->x6.456096, DCG:1 */
	{0x0015, 0x0081, 0x0004}, /* index: 135, gain:16.293150db->x6.526157, DCG:1 */
	{0x0015, 0x0083, 0x0004}, /* index: 136, gain:16.386900db->x6.596977, DCG:1 */
	{0x0016, 0x0080, 0x0004}, /* index: 137, gain:16.480650db->x6.668567, DCG:1 */
	{0x0016, 0x0081, 0x0004}, /* index: 138, gain:16.574400db->x6.740933, DCG:1 */
	{0x0016, 0x0083, 0x0004}, /* index: 139, gain:16.668150db->x6.814084, DCG:1 */
	{0x0016, 0x0084, 0x0004}, /* index: 140, gain:16.761900db->x6.888030, DCG:1 */
	{0x0017, 0x0080, 0x0004}, /* index: 141, gain:16.855650db->x6.962777, DCG:1 */
	{0x0017, 0x0082, 0x0004}, /* index: 142, gain:16.949400db->x7.038336, DCG:1 */
	{0x0017, 0x0083, 0x0004}, /* index: 143, gain:17.043150db->x7.114715, DCG:1 */
	{0x0018, 0x0080, 0x0004}, /* index: 144, gain:17.136900db->x7.191923, DCG:1 */
	{0x0018, 0x0081, 0x0004}, /* index: 145, gain:17.230650db->x7.269968, DCG:1 */
	{0x0018, 0x0082, 0x0004}, /* index: 146, gain:17.324400db->x7.348860, DCG:1 */
	{0x0018, 0x0084, 0x0004}, /* index: 147, gain:17.418150db->x7.428609, DCG:1 */
	{0x0019, 0x0080, 0x0004}, /* index: 148, gain:17.511900db->x7.509223, DCG:1 */
	{0x0019, 0x0081, 0x0004}, /* index: 149, gain:17.605650db->x7.590712, DCG:1 */
	{0x0019, 0x0082, 0x0004}, /* index: 150, gain:17.699400db->x7.673085, DCG:1 */
	{0x0019, 0x0084, 0x0004}, /* index: 151, gain:17.793150db->x7.756352, DCG:1 */
	{0x0019, 0x0085, 0x0004}, /* index: 152, gain:17.886900db->x7.840522, DCG:1 */
	{0x001a, 0x0080, 0x0004}, /* index: 153, gain:17.980650db->x7.925606, DCG:1 */
	{0x001a, 0x0082, 0x0004}, /* index: 154, gain:18.074400db->x8.011614, DCG:1 */
	{0x001a, 0x0083, 0x0004}, /* index: 155, gain:18.168150db->x8.098554, DCG:1 */
	{0x001a, 0x0084, 0x0004}, /* index: 156, gain:18.261900db->x8.186438, DCG:1 */
	{0x001b, 0x0081, 0x0004}, /* index: 157, gain:18.355650db->x8.275276, DCG:1 */
	{0x001b, 0x0082, 0x0004}, /* index: 158, gain:18.449400db->x8.365078, DCG:1 */
	{0x001b, 0x0083, 0x0004}, /* index: 159, gain:18.543150db->x8.455854, DCG:1 */
	{0x001b, 0x0085, 0x0004}, /* index: 160, gain:18.636900db->x8.547616, DCG:1 */
	{0x001c, 0x0080, 0x0004}, /* index: 161, gain:18.730650db->x8.640373, DCG:1 */
	{0x001c, 0x0081, 0x0004}, /* index: 162, gain:18.824400db->x8.734137, DCG:1 */
	{0x001c, 0x0082, 0x0004}, /* index: 163, gain:18.918150db->x8.828918, DCG:1 */
	{0x001c, 0x0084, 0x0004}, /* index: 164, gain:19.011900db->x8.924728, DCG:1 */
	{0x001c, 0x0085, 0x0004}, /* index: 165, gain:19.105650db->x9.021578, DCG:1 */
	{0x001d, 0x0080, 0x0004}, /* index: 166, gain:19.199400db->x9.119478, DCG:1 */
	{0x001d, 0x0082, 0x0004}, /* index: 167, gain:19.293150db->x9.218441, DCG:1 */
	{0x001d, 0x0083, 0x0004}, /* index: 168, gain:19.386900db->x9.318478, DCG:1 */
	{0x001d, 0x0084, 0x0004}, /* index: 169, gain:19.480650db->x9.419601, DCG:1 */
	{0x001d, 0x0086, 0x0004}, /* index: 170, gain:19.574400db->x9.521821, DCG:1 */
	{0x001e, 0x0080, 0x0004}, /* index: 171, gain:19.668150db->x9.625150, DCG:1 */
	{0x001e, 0x0081, 0x0004}, /* index: 172, gain:19.761900db->x9.729600, DCG:1 */
	{0x001e, 0x0082, 0x0004}, /* index: 173, gain:19.855650db->x9.835184, DCG:1 */
	{0x001e, 0x0084, 0x0004}, /* index: 174, gain:19.949400db->x9.941914, DCG:1 */
	{0x001e, 0x0085, 0x0004}, /* index: 175, gain:20.043150db->x10.049802, DCG:1 */
	{0x001f, 0x0080, 0x0004}, /* index: 176, gain:20.136900db->x10.158861, DCG:1 */
	{0x001f, 0x0081, 0x0004}, /* index: 177, gain:20.230650db->x10.269103, DCG:1 */
	{0x001f, 0x0082, 0x0004}, /* index: 178, gain:20.324400db->x10.380541, DCG:1 */
	{0x001f, 0x0084, 0x0004}, /* index: 179, gain:20.418150db->x10.493189, DCG:1 */
	{0x001f, 0x0085, 0x0004}, /* index: 180, gain:20.511900db->x10.607059, DCG:1 */
	{0x001f, 0x0087, 0x0004}, /* index: 181, gain:20.605650db->x10.722165, DCG:1 */
	{0x0020, 0x0080, 0x0004}, /* index: 182, gain:20.699400db->x10.838520, DCG:1 */
	{0x0020, 0x0081, 0x0004}, /* index: 183, gain:20.793150db->x10.956138, DCG:1 */
	{0x0020, 0x0083, 0x0004}, /* index: 184, gain:20.886900db->x11.075032, DCG:1 */
	{0x0021, 0x0080, 0x0004}, /* index: 185, gain:20.980650db->x11.195217, DCG:1 */
	{0x0021, 0x0082, 0x0004}, /* index: 186, gain:21.074400db->x11.316705, DCG:1 */
	{0x0021, 0x0083, 0x0004}, /* index: 187, gain:21.168150db->x11.439512, DCG:1 */
	{0x0022, 0x0080, 0x0004}, /* index: 188, gain:21.261900db->x11.563652, DCG:1 */
	{0x0022, 0x0081, 0x0004}, /* index: 189, gain:21.355650db->x11.689138, DCG:1 */
	{0x0022, 0x0082, 0x0004}, /* index: 190, gain:21.449400db->x11.815987, DCG:1 */
	{0x0023, 0x0080, 0x0004}, /* index: 191, gain:21.543150db->x11.944212, DCG:1 */
	{0x0023, 0x0082, 0x0004}, /* index: 192, gain:21.636900db->x12.073828, DCG:1 */
	{0x0023, 0x0083, 0x0004}, /* index: 193, gain:21.730650db->x12.204852, DCG:1 */
	{0x0024, 0x0080, 0x0004}, /* index: 194, gain:21.824400db->x12.337296, DCG:1 */
	{0x0024, 0x0081, 0x0004}, /* index: 195, gain:21.918150db->x12.471179, DCG:1 */
	{0x0024, 0x0083, 0x0004}, /* index: 196, gain:22.011900db->x12.606514, DCG:1 */
	{0x0024, 0x0084, 0x0004}, /* index: 197, gain:22.105650db->x12.743317, DCG:1 */
	{0x0025, 0x0080, 0x0004}, /* index: 198, gain:22.199400db->x12.881606, DCG:1 */
	{0x0025, 0x0081, 0x0004}, /* index: 199, gain:22.293150db->x13.021395, DCG:1 */
	{0x0025, 0x0083, 0x0004}, /* index: 200, gain:22.386900db->x13.162701, DCG:1 */
	{0x0026, 0x0080, 0x0004}, /* index: 201, gain:22.480650db->x13.305540, DCG:1 */
	{0x0026, 0x0081, 0x0004}, /* index: 202, gain:22.574400db->x13.449929, DCG:1 */
	{0x0026, 0x0083, 0x0004}, /* index: 203, gain:22.668150db->x13.595886, DCG:1 */
	{0x0026, 0x0084, 0x0004}, /* index: 204, gain:22.761900db->x13.743426, DCG:1 */
	{0x0027, 0x0080, 0x0004}, /* index: 205, gain:22.855650db->x13.892567, DCG:1 */
	{0x0027, 0x0082, 0x0004}, /* index: 206, gain:22.949400db->x14.043327, DCG:1 */
	{0x0027, 0x0083, 0x0004}, /* index: 207, gain:23.043150db->x14.195722, DCG:1 */
	{0x0027, 0x0084, 0x0004}, /* index: 208, gain:23.136900db->x14.349772, DCG:1 */
	{0x0028, 0x0081, 0x0004}, /* index: 209, gain:23.230650db->x14.505493, DCG:1 */
	{0x0028, 0x0082, 0x0004}, /* index: 210, gain:23.324400db->x14.662904, DCG:1 */
	{0x0028, 0x0084, 0x0004}, /* index: 211, gain:23.418150db->x14.822024, DCG:1 */
	{0x0028, 0x0085, 0x0004}, /* index: 212, gain:23.511900db->x14.982870, DCG:1 */
	{0x0029, 0x0081, 0x0004}, /* index: 213, gain:23.605650db->x15.145461, DCG:1 */
	{0x0029, 0x0082, 0x0004}, /* index: 214, gain:23.699400db->x15.309817, DCG:1 */
	{0x0029, 0x0083, 0x0004}, /* index: 215, gain:23.793150db->x15.475956, DCG:1 */
	{0x0029, 0x0085, 0x0004}, /* index: 216, gain:23.886900db->x15.643899, DCG:1 */
	{0x002a, 0x0080, 0x0004}, /* index: 217, gain:23.980650db->x15.813664, DCG:1 */
	{0x002a, 0x0081, 0x0004}, /* index: 218, gain:24.074400db->x15.985271, DCG:1 */
	{0x002a, 0x0083, 0x0004}, /* index: 219, gain:24.168150db->x16.158740, DCG:1 */
	{0x002a, 0x0084, 0x0004}, /* index: 220, gain:24.261900db->x16.334092, DCG:1 */
	{0x002b, 0x0080, 0x0004}, /* index: 221, gain:24.355650db->x16.511347, DCG:1 */
	{0x002b, 0x0082, 0x0004}, /* index: 222, gain:24.449400db->x16.690525, DCG:1 */
	{0x002b, 0x0083, 0x0004}, /* index: 223, gain:24.543150db->x16.871648, DCG:1 */
	{0x002b, 0x0084, 0x0004}, /* index: 224, gain:24.636900db->x17.054736, DCG:1 */
	{0x002b, 0x0086, 0x0004}, /* index: 225, gain:24.730650db->x17.239811, DCG:1 */
	{0x002c, 0x0081, 0x0004}, /* index: 226, gain:24.824400db->x17.426894, DCG:1 */
	{0x002c, 0x0082, 0x0004}, /* index: 227, gain:24.918150db->x17.616008, DCG:1 */
	{0x002c, 0x0083, 0x0004}, /* index: 228, gain:25.011900db->x17.807174, DCG:1 */
	{0x002c, 0x0085, 0x0004}, /* index: 229, gain:25.105650db->x18.000414, DCG:1 */
	{0x002d, 0x0080, 0x0004}, /* index: 230, gain:25.199400db->x18.195752, DCG:1 */
	{0x002d, 0x0081, 0x0004}, /* index: 231, gain:25.293150db->x18.393209, DCG:1 */
	{0x002d, 0x0083, 0x0004}, /* index: 232, gain:25.386900db->x18.592809, DCG:1 */
	{0x002d, 0x0084, 0x0004}, /* index: 233, gain:25.480650db->x18.794575, DCG:1 */
	{0x002d, 0x0086, 0x0004}, /* index: 234, gain:25.574400db->x18.998530, DCG:1 */
	{0x002d, 0x0087, 0x0004}, /* index: 235, gain:25.668150db->x19.204699, DCG:1 */
	{0x002e, 0x0081, 0x0004}, /* index: 236, gain:25.761900db->x19.413105, DCG:1 */
	{0x002e, 0x0082, 0x0004}, /* index: 237, gain:25.855650db->x19.623772, DCG:1 */
	{0x002e, 0x0084, 0x0004}, /* index: 238, gain:25.949400db->x19.836726, DCG:1 */
	{0x002e, 0x0085, 0x0004}, /* index: 239, gain:26.043150db->x20.051991, DCG:1 */
	{0x002e, 0x0086, 0x0004}, /* index: 240, gain:26.136900db->x20.269592, DCG:1 */
	{0x002f, 0x0081, 0x0004}, /* index: 241, gain:26.230650db->x20.489554, DCG:1 */
	{0x002f, 0x0082, 0x0004}, /* index: 242, gain:26.324400db->x20.711903, DCG:1 */
	{0x002f, 0x0083, 0x0004}, /* index: 243, gain:26.418150db->x20.936665, DCG:1 */
	{0x002f, 0x0085, 0x0004}, /* index: 244, gain:26.511900db->x21.163866, DCG:1 */
	{0x002f, 0x0086, 0x0004}, /* index: 245, gain:26.605650db->x21.393532, DCG:1 */
	{0x0030, 0x0080, 0x0004}, /* index: 246, gain:26.699400db->x21.625691, DCG:1 */
	{0x0030, 0x0081, 0x0004}, /* index: 247, gain:26.793150db->x21.860370, DCG:1 */
	{0x0030, 0x0082, 0x0004}, /* index: 248, gain:26.886900db->x22.097595, DCG:1 */
	{0x0031, 0x0080, 0x0004}, /* index: 249, gain:26.980650db->x22.337394, DCG:1 */
	{0x0031, 0x0081, 0x0004}, /* index: 250, gain:27.074400db->x22.579795, DCG:1 */
	{0x0031, 0x0083, 0x0004}, /* index: 251, gain:27.168150db->x22.824827, DCG:1 */
	{0x0031, 0x0084, 0x0004}, /* index: 252, gain:27.261900db->x23.072518, DCG:1 */
	{0x0032, 0x0081, 0x0004}, /* index: 253, gain:27.355650db->x23.322897, DCG:1 */
	{0x0032, 0x0082, 0x0004}, /* index: 254, gain:27.449400db->x23.575993, DCG:1 */
	{0x0033, 0x0080, 0x0004}, /* index: 255, gain:27.543150db->x23.831836, DCG:1 */
	{0x0033, 0x0081, 0x0004}, /* index: 256, gain:27.636900db->x24.090455, DCG:1 */
	{0x0033, 0x0083, 0x0004}, /* index: 257, gain:27.730650db->x24.351880, DCG:1 */
	{0x0033, 0x0084, 0x0004}, /* index: 258, gain:27.824400db->x24.616143, DCG:1 */
	{0x0034, 0x0081, 0x0004}, /* index: 259, gain:27.918150db->x24.883273, DCG:1 */
	{0x0034, 0x0082, 0x0004}, /* index: 260, gain:28.011900db->x25.153302, DCG:1 */
	{0x0034, 0x0084, 0x0004}, /* index: 261, gain:28.105650db->x25.426261, DCG:1 */
	{0x0034, 0x0085, 0x0004}, /* index: 262, gain:28.199400db->x25.702182, DCG:1 */
	{0x0035, 0x0081, 0x0004}, /* index: 263, gain:28.293150db->x25.981098, DCG:1 */
	{0x0035, 0x0082, 0x0004}, /* index: 264, gain:28.386900db->x26.263040, DCG:1 */
	{0x0035, 0x0084, 0x0004}, /* index: 265, gain:28.480650db->x26.548042, DCG:1 */
	{0x0036, 0x0081, 0x0004}, /* index: 266, gain:28.574400db->x26.836137, DCG:1 */
	{0x0036, 0x0082, 0x0004}, /* index: 267, gain:28.668150db->x27.127358, DCG:1 */
	{0x0036, 0x0084, 0x0004}, /* index: 268, gain:28.761900db->x27.421739, DCG:1 */
	{0x0037, 0x0080, 0x0004}, /* index: 269, gain:28.855650db->x27.719315, DCG:1 */
	{0x0037, 0x0081, 0x0004}, /* index: 270, gain:28.949400db->x28.020121, DCG:1 */
	{0x0037, 0x0083, 0x0004}, /* index: 271, gain:29.043150db->x28.324190, DCG:1 */
	{0x0037, 0x0084, 0x0004}, /* index: 272, gain:29.136900db->x28.631559, DCG:1 */
	{0x0038, 0x0080, 0x0004}, /* index: 273, gain:29.230650db->x28.942264, DCG:1 */
	{0x0038, 0x0082, 0x0004}, /* index: 274, gain:29.324400db->x29.256340, DCG:1 */
	{0x0038, 0x0083, 0x0004}, /* index: 275, gain:29.418150db->x29.573825, DCG:1 */
	{0x0038, 0x0085, 0x0004}, /* index: 276, gain:29.511900db->x29.894755, DCG:1 */
	{0x0039, 0x0080, 0x0004}, /* index: 277, gain:29.605650db->x30.219168, DCG:1 */
	{0x0039, 0x0082, 0x0004}, /* index: 278, gain:29.699400db->x30.547101, DCG:1 */
	{0x0039, 0x0083, 0x0004}, /* index: 279, gain:29.793150db->x30.878593, DCG:1 */
	{0x0039, 0x0085, 0x0004}, /* index: 280, gain:29.886900db->x31.213682, DCG:1 */
	{0x003a, 0x0080, 0x0004}, /* index: 281, gain:29.980650db->x31.552407, DCG:1 */
	{0x003a, 0x0081, 0x0004}, /* index: 282, gain:30.074400db->x31.894809, DCG:1 */
	{0x003a, 0x0082, 0x0004}, /* index: 283, gain:30.168150db->x32.240925, DCG:1 */
	{0x003a, 0x0084, 0x0004}, /* index: 284, gain:30.261900db->x32.590798, DCG:1 */
	{0x003b, 0x0080, 0x0004}, /* index: 285, gain:30.355650db->x32.944468, DCG:1 */
	{0x003b, 0x0081, 0x0004}, /* index: 286, gain:30.449400db->x33.301976, DCG:1 */
	{0x003b, 0x0083, 0x0004}, /* index: 287, gain:30.543150db->x33.663363, DCG:1 */
	{0x003b, 0x0084, 0x0004}, /* index: 288, gain:30.636900db->x34.028672, DCG:1 */
	{0x003b, 0x0086, 0x0004}, /* index: 289, gain:30.730650db->x34.397945, DCG:1 */
	{0x003c, 0x0080, 0x0004}, /* index: 290, gain:30.824400db->x34.771226, DCG:1 */
	{0x003c, 0x0082, 0x0004}, /* index: 291, gain:30.918150db->x35.148557, DCG:1 */
	{0x003c, 0x0083, 0x0004}, /* index: 292, gain:31.011900db->x35.529983, DCG:1 */
	{0x003c, 0x0085, 0x0004}, /* index: 293, gain:31.105650db->x35.915548, DCG:1 */
	{0x003d, 0x0080, 0x0004}, /* index: 294, gain:31.199400db->x36.305298, DCG:1 */
	{0x003d, 0x0081, 0x0004}, /* index: 295, gain:31.293150db->x36.699276, DCG:1 */
	{0x003d, 0x0082, 0x0004}, /* index: 296, gain:31.386900db->x37.097530, DCG:1 */
	{0x003d, 0x0084, 0x0004}, /* index: 297, gain:31.480650db->x37.500106, DCG:1 */
	{0x003d, 0x0085, 0x0004}, /* index: 298, gain:31.574400db->x37.907051, DCG:1 */
	{0x003d, 0x0087, 0x0004}, /* index: 299, gain:31.668150db->x38.318412, DCG:1 */
	{0x003e, 0x0080, 0x0004}, /* index: 300, gain:31.761900db->x38.734237, DCG:1 */
	{0x003e, 0x0082, 0x0004}, /* index: 301, gain:31.855650db->x39.154574, DCG:1 */
	{0x003e, 0x0083, 0x0004}, /* index: 302, gain:31.949400db->x39.579472, DCG:1 */
	{0x003e, 0x0085, 0x0004}, /* index: 303, gain:32.043150db->x40.008982, DCG:1 */
	{0x003e, 0x0086, 0x0004}, /* index: 304, gain:32.136900db->x40.443152, DCG:1 */
	{0x003f, 0x0080, 0x0004}, /* index: 305, gain:32.230650db->x40.882034, DCG:1 */
	{0x003f, 0x0082, 0x0004}, /* index: 306, gain:32.324400db->x41.325679, DCG:1 */
	{0x003f, 0x0083, 0x0004}, /* index: 307, gain:32.418150db->x41.774138, DCG:1 */
	{0x003f, 0x0085, 0x0004}, /* index: 308, gain:32.511900db->x42.227464, DCG:1 */
	{0x003f, 0x0086, 0x0004}, /* index: 309, gain:32.605650db->x42.685709, DCG:1 */
	{0x003f, 0x0088, 0x0004}, /* index: 310, gain:32.699400db->x43.148927, DCG:1 */
	{0x0040, 0x0081, 0x0004}, /* index: 311, gain:32.793150db->x43.617172, DCG:1 */
	{0x0040, 0x0082, 0x0004}, /* index: 312, gain:32.886900db->x44.090498, DCG:1 */
	{0x0041, 0x0080, 0x0004}, /* index: 313, gain:32.980650db->x44.568960, DCG:1 */
	{0x0041, 0x0081, 0x0004}, /* index: 314, gain:33.074400db->x45.052615, DCG:1 */
	{0x0041, 0x0083, 0x0004}, /* index: 315, gain:33.168150db->x45.541518, DCG:1 */
	{0x0041, 0x0084, 0x0004}, /* index: 316, gain:33.261900db->x46.035726, DCG:1 */
	{0x0042, 0x0080, 0x0004}, /* index: 317, gain:33.355650db->x46.535298, DCG:1 */
	{0x0042, 0x0082, 0x0004}, /* index: 318, gain:33.449400db->x47.040291, DCG:1 */
	{0x0043, 0x0080, 0x0004}, /* index: 319, gain:33.543150db->x47.550764, DCG:1 */
	{0x0043, 0x0081, 0x0004}, /* index: 320, gain:33.636900db->x48.066777, DCG:1 */
	{0x0043, 0x0082, 0x0004}, /* index: 321, gain:33.730650db->x48.588389, DCG:1 */
	{0x0043, 0x0084, 0x0004}, /* index: 322, gain:33.824400db->x49.115662, DCG:1 */
	{0x0044, 0x0081, 0x0004}, /* index: 323, gain:33.918150db->x49.648656, DCG:1 */
	{0x0044, 0x0082, 0x0004}, /* index: 324, gain:34.011900db->x50.187435, DCG:1 */
	{0x0044, 0x0083, 0x0004}, /* index: 325, gain:34.105650db->x50.732060, DCG:1 */
	{0x0044, 0x0085, 0x0004}, /* index: 326, gain:34.199400db->x51.282596, DCG:1 */
	{0x0045, 0x0081, 0x0004}, /* index: 327, gain:34.293150db->x51.839106, DCG:1 */
	{0x0045, 0x0082, 0x0004}, /* index: 328, gain:34.386900db->x52.401655, DCG:1 */
	{0x0045, 0x0083, 0x0004}, /* index: 329, gain:34.480650db->x52.970308, DCG:1 */
	{0x0046, 0x0080, 0x0004}, /* index: 330, gain:34.574400db->x53.545133, DCG:1 */
	{0x0046, 0x0082, 0x0004}, /* index: 331, gain:34.668150db->x54.126195, DCG:1 */
	{0x0046, 0x0083, 0x0004}, /* index: 332, gain:34.761900db->x54.713563, DCG:1 */
	{0x0047, 0x0080, 0x0004}, /* index: 333, gain:34.855650db->x55.307305, DCG:1 */
	{0x0047, 0x0081, 0x0004}, /* index: 334, gain:34.949400db->x55.907491, DCG:1 */
	{0x0047, 0x0082, 0x0004}, /* index: 335, gain:35.043150db->x56.514189, DCG:1 */
	{0x0047, 0x0084, 0x0004}, /* index: 336, gain:35.136900db->x57.127471, DCG:1 */
	{0x0048, 0x0080, 0x0004}, /* index: 337, gain:35.230650db->x57.747409, DCG:1 */
	{0x0048, 0x0082, 0x0004}, /* index: 338, gain:35.324400db->x58.374073, DCG:1 */
	{0x0048, 0x0083, 0x0004}, /* index: 339, gain:35.418150db->x59.007539, DCG:1 */
	{0x0048, 0x0084, 0x0004}, /* index: 340, gain:35.511900db->x59.647878, DCG:1 */
	{0x0049, 0x0080, 0x0004}, /* index: 341, gain:35.605650db->x60.295167, DCG:1 */
	{0x0049, 0x0081, 0x0004}, /* index: 342, gain:35.699400db->x60.949479, DCG:1 */
	{0x0049, 0x0083, 0x0004}, /* index: 343, gain:35.793150db->x61.610892, DCG:1 */
	{0x0049, 0x0084, 0x0004}, /* index: 344, gain:35.886900db->x62.279483, DCG:1 */
	{0x0049, 0x0086, 0x0004}, /* index: 345, gain:35.980650db->x62.955329, DCG:1 */
	{0x004a, 0x0081, 0x0004}, /* index: 346, gain:36.074400db->x63.638510, DCG:1 */
	{0x004a, 0x0082, 0x0004}, /* index: 347, gain:36.168150db->x64.329104, DCG:1 */
	{0x004a, 0x0083, 0x0004}, /* index: 348, gain:36.261900db->x65.027192, DCG:1 */
	{0x004b, 0x0080, 0x0004}, /* index: 349, gain:36.355650db->x65.732856, DCG:1 */
	{0x004b, 0x0081, 0x0004}, /* index: 350, gain:36.449400db->x66.446177, DCG:1 */
	{0x004b, 0x0082, 0x0004}, /* index: 351, gain:36.543150db->x67.167240, DCG:1 */
	{0x004b, 0x0084, 0x0004}, /* index: 352, gain:36.636900db->x67.896127, DCG:1 */
	{0x004b, 0x0085, 0x0004}, /* index: 353, gain:36.730650db->x68.632924, DCG:1 */
	{0x004c, 0x0080, 0x0004}, /* index: 354, gain:36.824400db->x69.377716, DCG:1 */
	{0x004c, 0x0081, 0x0004}, /* index: 355, gain:36.918150db->x70.130591, DCG:1 */
	{0x004c, 0x0083, 0x0004}, /* index: 356, gain:37.011900db->x70.891636, DCG:1 */
	{0x004c, 0x0084, 0x0004}, /* index: 357, gain:37.105650db->x71.660940, DCG:1 */
	{0x004c, 0x0086, 0x0004}, /* index: 358, gain:37.199400db->x72.438592, DCG:1 */
	{0x004d, 0x0081, 0x0004}, /* index: 359, gain:37.293150db->x73.224683, DCG:1 */
	{0x004d, 0x0082, 0x0004}, /* index: 360, gain:37.386900db->x74.019304, DCG:1 */
	{0x004d, 0x0083, 0x0004}, /* index: 361, gain:37.480650db->x74.822549, DCG:1 */
	{0x004d, 0x0085, 0x0004}, /* index: 362, gain:37.574400db->x75.634510, DCG:1 */
	{0x004d, 0x0086, 0x0004}, /* index: 363, gain:37.668150db->x76.455283, DCG:1 */
	{0x004e, 0x0080, 0x0004}, /* index: 364, gain:37.761900db->x77.284962, DCG:1 */
	{0x004e, 0x0082, 0x0004}, /* index: 365, gain:37.855650db->x78.123645, DCG:1 */
	{0x004e, 0x0083, 0x0004}, /* index: 366, gain:37.949400db->x78.971430, DCG:1 */
	{0x004e, 0x0084, 0x0004}, /* index: 367, gain:38.043150db->x79.828414, DCG:1 */
	{0x004e, 0x0086, 0x0004}, /* index: 368, gain:38.136900db->x80.694698, DCG:1 */
	{0x004f, 0x0080, 0x0004}, /* index: 369, gain:38.230650db->x81.570383, DCG:1 */
	{0x004f, 0x0081, 0x0004}, /* index: 370, gain:38.324400db->x82.455570, DCG:1 */
	{0x004f, 0x0083, 0x0004}, /* index: 371, gain:38.418150db->x83.350364, DCG:1 */
	{0x004f, 0x0084, 0x0004}, /* index: 372, gain:38.511900db->x84.254868, DCG:1 */
	{0x004f, 0x0086, 0x0004}, /* index: 373, gain:38.605650db->x85.169187, DCG:1 */
	{0x004f, 0x0087, 0x0004}, /* index: 374, gain:38.699400db->x86.093428, DCG:1 */
	{0x004f, 0x0089, 0x0004}, /* index: 375, gain:38.793150db->x87.027699, DCG:1 */
	{0x004f, 0x008a, 0x0004}, /* index: 376, gain:38.886900db->x87.972108, DCG:1 */
	{0x004f, 0x008c, 0x0004}, /* index: 377, gain:38.980650db->x88.926766, DCG:1 */
	{0x004f, 0x008d, 0x0004}, /* index: 378, gain:39.074400db->x89.891784, DCG:1 */
	{0x004f, 0x008f, 0x0004}, /* index: 379, gain:39.168150db->x90.867274, DCG:1 */
	{0x004f, 0x0090, 0x0004}, /* index: 380, gain:39.261900db->x91.853350, DCG:1 */
	{0x004f, 0x0092, 0x0004}, /* index: 381, gain:39.355650db->x92.850127, DCG:1 */
	{0x004f, 0x0093, 0x0004}, /* index: 382, gain:39.449400db->x93.857720, DCG:1 */
	{0x004f, 0x0095, 0x0004}, /* index: 383, gain:39.543150db->x94.876248, DCG:1 */
	{0x004f, 0x0097, 0x0004}, /* index: 384, gain:39.636900db->x95.905828, DCG:1 */
	{0x004f, 0x0098, 0x0004}, /* index: 385, gain:39.730650db->x96.946582, DCG:1 */
	{0x004f, 0x009a, 0x0004}, /* index: 386, gain:39.824400db->x97.998629, DCG:1 */
	{0x004f, 0x009c, 0x0004}, /* index: 387, gain:39.918150db->x99.062093, DCG:1 */
	{0x004f, 0x009d, 0x0004}, /* index: 388, gain:40.011900db->x100.137098, DCG:1 */
	{0x004f, 0x009f, 0x0004}, /* index: 389, gain:40.105650db->x101.223768, DCG:1 */
	{0x004f, 0x00a1, 0x0004}, /* index: 390, gain:40.199400db->x102.322231, DCG:1 */
	{0x004f, 0x00a3, 0x0004}, /* index: 391, gain:40.293150db->x103.432614, DCG:1 */
	{0x004f, 0x00a4, 0x0004}, /* index: 392, gain:40.386900db->x104.555047, DCG:1 */
	{0x004f, 0x00a6, 0x0004}, /* index: 393, gain:40.480650db->x105.689660, DCG:1 */
	{0x004f, 0x00a8, 0x0004}, /* index: 394, gain:40.574400db->x106.836586, DCG:1 */
	{0x004f, 0x00aa, 0x0004}, /* index: 395, gain:40.668150db->x107.995958, DCG:1 */
	{0x004f, 0x00ac, 0x0004}, /* index: 396, gain:40.761900db->x109.167911, DCG:1 */
	{0x004f, 0x00ad, 0x0004}, /* index: 397, gain:40.855650db->x110.352582, DCG:1 */
	{0x004f, 0x00af, 0x0004}, /* index: 398, gain:40.949400db->x111.550109, DCG:1 */
	{0x004f, 0x00b1, 0x0004}, /* index: 399, gain:41.043150db->x112.760632, DCG:1 */
	{0x004f, 0x00b3, 0x0004}, /* index: 400, gain:41.136900db->x113.984290, DCG:1 */
	{0x004f, 0x00b5, 0x0004}, /* index: 401, gain:41.230650db->x115.221228, DCG:1 */
	{0x004f, 0x00b7, 0x0004}, /* index: 402, gain:41.324400db->x116.471589, DCG:1 */
	{0x004f, 0x00b9, 0x0004}, /* index: 403, gain:41.418150db->x117.735518, DCG:1 */
	{0x004f, 0x00bb, 0x0004}, /* index: 404, gain:41.511900db->x119.013164, DCG:1 */
	{0x004f, 0x00bd, 0x0004}, /* index: 405, gain:41.605650db->x120.304674, DCG:1 */
	{0x004f, 0x00bf, 0x0004}, /* index: 406, gain:41.699400db->x121.610199, DCG:1 */
	{0x004f, 0x00c1, 0x0004}, /* index: 407, gain:41.793150db->x122.929892, DCG:1 */
	{0x004f, 0x00c3, 0x0004}, /* index: 408, gain:41.886900db->x124.263906, DCG:1 */
	{0x004f, 0x00c5, 0x0004}, /* index: 409, gain:41.980650db->x125.612396, DCG:1 */
	{0x004f, 0x00c8, 0x0004}, /* index: 410, gain:42.074400db->x126.975520, DCG:1 */
	{0x004f, 0x00ca, 0x0004}, /* index: 411, gain:42.168150db->x128.353436, DCG:1 */
	{0x004f, 0x00cc, 0x0004}, /* index: 412, gain:42.261900db->x129.746305, DCG:1 */
	{0x004f, 0x00ce, 0x0004}, /* index: 413, gain:42.355650db->x131.154290, DCG:1 */
	{0x004f, 0x00d0, 0x0004}, /* index: 414, gain:42.449400db->x132.577553, DCG:1 */
	{0x004f, 0x00d3, 0x0004}, /* index: 415, gain:42.543150db->x134.016262, DCG:1 */
	{0x004f, 0x00d5, 0x0004}, /* index: 416, gain:42.636900db->x135.470583, DCG:1 */
	{0x004f, 0x00d7, 0x0004}, /* index: 417, gain:42.730650db->x136.940686, DCG:1 */
	{0x004f, 0x00da, 0x0004}, /* index: 418, gain:42.824400db->x138.426743, DCG:1 */
	{0x004f, 0x00dc, 0x0004}, /* index: 419, gain:42.918150db->x139.928926, DCG:1 */
	{0x004f, 0x00de, 0x0004}, /* index: 420, gain:43.011900db->x141.447410, DCG:1 */
	{0x004f, 0x00e1, 0x0004}, /* index: 421, gain:43.105650db->x142.982373, DCG:1 */
	{0x004f, 0x00e3, 0x0004}, /* index: 422, gain:43.199400db->x144.533993, DCG:1 */
	{0x004f, 0x00e6, 0x0004}, /* index: 423, gain:43.293150db->x146.102450, DCG:1 */
	{0x004f, 0x00e8, 0x0004}, /* index: 424, gain:43.386900db->x147.687929, DCG:1 */
	{0x004f, 0x00eb, 0x0004}, /* index: 425, gain:43.480650db->x149.290613, DCG:1 */
	{0x004f, 0x00ed, 0x0004}, /* index: 426, gain:43.574400db->x150.910688, DCG:1 */
	{0x004f, 0x00f0, 0x0004}, /* index: 427, gain:43.668150db->x152.548345, DCG:1 */
	{0x004f, 0x00f3, 0x0004}, /* index: 428, gain:43.761900db->x154.203773, DCG:1 */
	{0x004f, 0x00f5, 0x0004}, /* index: 429, gain:43.855650db->x155.877166, DCG:1 */
	{0x004f, 0x00f8, 0x0004}, /* index: 430, gain:43.949400db->x157.568717, DCG:1 */
	{0x004f, 0x00fb, 0x0004}, /* index: 431, gain:44.043150db->x159.278626, DCG:1 */
	{0x004f, 0x00fd, 0x0004}, /* index: 432, gain:44.136900db->x161.007090, DCG:1 */
	{0x004f, 0x0100, 0x0004}, /* index: 433, gain:44.230650db->x162.754311, DCG:1 */
	{0x004f, 0x0103, 0x0004}, /* index: 434, gain:44.324400db->x164.520492, DCG:1 */
	{0x004f, 0x0106, 0x0004}, /* index: 435, gain:44.418150db->x166.305840, DCG:1 */
	{0x004f, 0x0108, 0x0004}, /* index: 436, gain:44.511900db->x168.110562, DCG:1 */
	{0x004f, 0x010b, 0x0004}, /* index: 437, gain:44.605650db->x169.934869, DCG:1 */
	{0x004f, 0x010e, 0x0004}, /* index: 438, gain:44.699400db->x171.778972, DCG:1 */
	{0x004f, 0x0111, 0x0004}, /* index: 439, gain:44.793150db->x173.643088, DCG:1 */
	{0x004f, 0x0114, 0x0004}, /* index: 440, gain:44.886900db->x175.527432, DCG:1 */
	{0x004f, 0x0117, 0x0004}, /* index: 441, gain:44.980650db->x177.432226, DCG:1 */
	{0x004f, 0x011a, 0x0004}, /* index: 442, gain:45.074400db->x179.357689, DCG:1 */
	{0x004f, 0x011d, 0x0004}, /* index: 443, gain:45.168150db->x181.304048, DCG:1 */
	{0x004f, 0x0120, 0x0004}, /* index: 444, gain:45.261900db->x183.271528, DCG:1 */
	{0x004f, 0x0123, 0x0004}, /* index: 445, gain:45.355650db->x185.260358, DCG:1 */
	{0x004f, 0x0127, 0x0004}, /* index: 446, gain:45.449400db->x187.270772, DCG:1 */
	{0x004f, 0x012a, 0x0004}, /* index: 447, gain:45.543150db->x189.303002, DCG:1 */
	{0x004f, 0x012d, 0x0004}, /* index: 448, gain:45.636900db->x191.357285, DCG:1 */
	{0x004f, 0x0130, 0x0004}, /* index: 449, gain:45.730650db->x193.433861, DCG:1 */
	{0x004f, 0x0134, 0x0004}, /* index: 450, gain:45.824400db->x195.532971, DCG:1 */
	{0x004f, 0x0137, 0x0004}, /* index: 451, gain:45.918150db->x197.654861, DCG:1 */
	{0x004f, 0x013a, 0x0004}, /* index: 452, gain:46.011900db->x199.799777, DCG:1 */
	{0x004f, 0x013e, 0x0004}, /* index: 453, gain:46.105650db->x201.967970, DCG:1 */
	{0x004f, 0x0141, 0x0004}, /* index: 454, gain:46.199400db->x204.159691, DCG:1 */
	{0x004f, 0x0145, 0x0004}, /* index: 455, gain:46.293150db->x206.375197, DCG:1 */
	{0x004f, 0x0148, 0x0004}, /* index: 456, gain:46.386900db->x208.614744, DCG:1 */
	{0x004f, 0x014c, 0x0004}, /* index: 457, gain:46.480650db->x210.878595, DCG:1 */
	{0x004f, 0x014f, 0x0004}, /* index: 458, gain:46.574400db->x213.167013, DCG:1 */
	{0x004f, 0x0153, 0x0004}, /* index: 459, gain:46.668150db->x215.480265, DCG:1 */
	{0x004f, 0x0157, 0x0004}, /* index: 460, gain:46.761900db->x217.818619, DCG:1 */
	{0x004f, 0x015b, 0x0004}, /* index: 461, gain:46.855650db->x220.182349, DCG:1 */
	{0x004f, 0x015e, 0x0004}, /* index: 462, gain:46.949400db->x222.571729, DCG:1 */
	{0x004f, 0x0162, 0x0004}, /* index: 463, gain:47.043150db->x224.987039, DCG:1 */
	{0x004f, 0x0166, 0x0004}, /* index: 464, gain:47.136900db->x227.428559, DCG:1 */
	{0x004f, 0x016a, 0x0004}, /* index: 465, gain:47.230650db->x229.896574, DCG:1 */
	{0x004f, 0x016e, 0x0004}, /* index: 466, gain:47.324400db->x232.391372, DCG:1 */
	{0x004f, 0x0172, 0x0004}, /* index: 467, gain:47.418150db->x234.913243, DCG:1 */
	{0x004f, 0x0176, 0x0004}, /* index: 468, gain:47.511900db->x237.462480, DCG:1 */
	{0x004f, 0x017a, 0x0004}, /* index: 469, gain:47.605650db->x240.039382, DCG:1 */
	{0x004f, 0x017e, 0x0004}, /* index: 470, gain:47.699400db->x242.644248, DCG:1 */
	{0x004f, 0x0182, 0x0004}, /* index: 471, gain:47.793150db->x245.277381, DCG:1 */
	{0x004f, 0x0186, 0x0004}, /* index: 472, gain:47.886900db->x247.939088, DCG:1 */
	{0x004f, 0x018b, 0x0004}, /* index: 473, gain:47.980650db->x250.629680, DCG:1 */
	{0x004f, 0x018f, 0x0004}, /* index: 474, gain:48.074400db->x253.349470, DCG:1 */
};
