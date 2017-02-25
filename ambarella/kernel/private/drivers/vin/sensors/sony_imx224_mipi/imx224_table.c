/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx224/imx224_table.c
 *
 * History:
 *    2014/08/05 - [Long Zhao] Create
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

static struct vin_video_pll imx224_plls[] = {
	{0, 37125000, 148500000},
};

static struct vin_reg_16_8 imx224_pll_regs[][4] = {
	{
		{IMX224_INCKSEL1, 0x20},
		{IMX224_INCKSEL2, 0x00},
		{IMX224_INCKSEL3, 0x20},
		{IMX224_INCKSEL4, 0x00},
	},
};

static struct vin_reg_16_8 imx224_mode_regs[][44] = {
	{	/* Quad VGAp30 12bits */
		{0x3005, 0x01}, /* ADBIT */
		{0x3006, 0x00}, /* MODE */
		{0x3007, 0x00}, /* WINMODE */
		{0x3009, 0x02}, /* FRSEL */
		{0x3018, 0x4C}, /* VMAX_LSB */
		{0x3019, 0x04}, /* VMAX_MSB */
		{0x301B, 0x94}, /* HMAX_LSB */
		{0x301C, 0x11}, /* HMAX_MSB */
		{0x3044, 0x01}, /* ODBIT_OPORTSEL */
		{0x300A, 0xF0}, /* BLKLEVEL_LSB */
		{0x300B, 0x00}, /* BLKLEVEL_MSB */

		{0x3344, 0x20}, /* REPETITION */
		{0x3346, 0x03}, /* LANE_NUM */
		{0x3353, 0x0E}, /* OB_SIZE_V */
		{0x336B, 0x27}, /* THSEXIT */
		{0x336C, 0x1F}, /* TCLKPRE */
		{0x337F, 0x03}, /* LANE_MODE */

		{0x3382, 0x57}, /* TCLKPOST */
		{0x3383, 0x20}, /* THSPREPARE */
		{0x3384, 0x27}, /* THSZERO */
		{0x3385, 0x20}, /* THSTRAIL */
		{0x3386, 0x0F}, /* TCLKTRAIL */
		{0x3387, 0x07}, /* TCLKPREPARE */
		{0x3388, 0x37}, /* TCLKZERO */
		{0x3389, 0x1F}, /* TLPX */

		{0x3380, 0x20}, /* INCK_FREQ1_LSB */
		{0x3381, 0x25}, /* INCK_FREQ1_MSB */
		{0x338D, 0xB4}, /* INCK_FREQ2_LSB */
		{0x338E, 0x01}, /* INCK_FREQ2_MSB */

		/* DOL related */
		{0x300C, 0x00}, /* IMX224_WDMODE */
		{0x3020, 0x00}, /* IMX224_SHS1_LSB */
		{0x3021, 0x00}, /* IMX224_SHS1_MSB */
		{0x3022, 0x00}, /* IMX224_SHS1_HSB */
		{0x3023, 0x00}, /* IMX224_SHS2_LSB */
		{0x3024, 0x00}, /* IMX224_SHS2_MSB */
		{0x3025, 0x00}, /* IMX224_SHS2_HSB */
		{0x3026, 0x00}, /* IMX224_SHS3_LSB */
		{0x3027, 0x00}, /* IMX224_SHS3_MSB */
		{0x3028, 0x00}, /* IMX224_SHS3_HSB */
		{0x3354, 0x01}, /* IMX224_NULL0_SIZE */
		{0x3357, 0xD1}, /* IMX224_PIC_SIZE_LSB */
		{0x3358, 0x03}, /* IMX224_PIC_SIZE_MSB */
		{0x3043, 0x01}, /* IMX224_DOL_PAT1 */
		{0x3109, 0x00}, /* IMX224_XVSCNT_INT */
	},
	{	/* HD720p30 12bits */
		{0x3005, 0x01}, /* ADBIT */
		{0x3006, 0x00}, /* MODE */
		{0x3007, 0x10}, /* WINMODE */
		{0x3009, 0x02}, /* FRSEL */
		{0x3018, 0xEE}, /* VMAX_LSB */
		{0x3019, 0x02}, /* VMAX_MSB */
		{0x301B, 0xC8}, /* HMAX_LSB */
		{0x301C, 0x19}, /* HMAX_MSB */
		{0x3044, 0x01}, /* ODBIT_OPORTSEL */
		{0x300A, 0xF0}, /* BLKLEVEL_LSB */
		{0x300B, 0x00}, /* BLKLEVEL_MSB */

		{0x3344, 0x20}, /* REPETITION */
		{0x3346, 0x03}, /* LANE_NUM */
		{0x3353, 0x04}, /* OB_SIZE_V */
		{0x336B, 0x27}, /* THSEXIT */
		{0x336C, 0x1F}, /* TCLKPRE */
		{0x337F, 0x03}, /* LANE_MODE */

		{0x3382, 0x57}, /* TCLKPOST */
		{0x3383, 0x20}, /* THSPREPARE */
		{0x3384, 0x27}, /* THSZERO */
		{0x3385, 0x20}, /* THSTRAIL */
		{0x3386, 0x0F}, /* TCLKTRAIL */
		{0x3387, 0x07}, /* TCLKPREPARE */
		{0x3388, 0x37}, /* TCLKZERO */
		{0x3389, 0x1F}, /* TLPX */

		{0x3380, 0x20}, /* INCK_FREQ1_LSB */
		{0x3381, 0x25}, /* INCK_FREQ1_MSB */
		{0x338D, 0xB4}, /* INCK_FREQ2_LSB */
		{0x338E, 0x01}, /* INCK_FREQ2_MSB */

		/* DOL related */
		{0x300C, 0x00}, /* IMX224_WDMODE */
		{0x3020, 0x00}, /* IMX224_SHS1_LSB */
		{0x3021, 0x00}, /* IMX224_SHS1_MSB */
		{0x3022, 0x00}, /* IMX224_SHS1_HSB */
		{0x3023, 0x00}, /* IMX224_SHS2_LSB */
		{0x3024, 0x00}, /* IMX224_SHS2_MSB */
		{0x3025, 0x00}, /* IMX224_SHS2_HSB */
		{0x3026, 0x00}, /* IMX224_SHS3_LSB */
		{0x3027, 0x00}, /* IMX224_SHS3_MSB */
		{0x3028, 0x00}, /* IMX224_SHS3_HSB */
		{0x3354, 0x01}, /* IMX224_NULL0_SIZE */
		{0x3357, 0xD9}, /* IMX224_PIC_SIZE_LSB */
		{0x3358, 0x02}, /* IMX224_PIC_SIZE_MSB */
		{0x3043, 0x01}, /* IMX224_DOL_PAT1 */
		{0x3109, 0x00}, /* IMX224_XVSCNT_INT */
	},
	{	/* 2x DOL Quad VGAp30 12bits */
		{0x3005, 0x01}, /* ADBIT */
		{0x3006, 0x00}, /* MODE */
		{0x3007, 0x00}, /* WINMODE */
		{0x3009, 0x01}, /* FRSEL */
		{0x3018, 0x4C}, /* VMAX_LSB */
		{0x3019, 0x04}, /* VMAX_MSB */
		{0x301B, 0xCA}, /* HMAX_LSB */
		{0x301C, 0x08}, /* HMAX_MSB */
		{0x3044, 0xE1}, /* ODBIT_OPORTSEL */
		{0x300A, 0xF0}, /* BLKLEVEL_LSB */
		{0x300B, 0x00}, /* BLKLEVEL_MSB */

		{0x3344, 0x10}, /* REPETITION */
		{0x3346, 0x03}, /* LANE_NUM */
		{0x3353, 0x0E}, /* OB_SIZE_V */
		{0x336B, 0x37}, /* THSEXIT */
		{0x336C, 0x1F}, /* TCLKPRE */
		{0x337F, 0x03}, /* LANE_MODE */

		{0x3382, 0x5F}, /* TCLKPOST */
		{0x3383, 0x17}, /* THSPREPARE */
		{0x3384, 0x37}, /* THSZERO */
		{0x3385, 0x17}, /* THSTRAIL */
		{0x3386, 0x17}, /* TCLKTRAIL */
		{0x3387, 0x17}, /* TCLKPREPARE */
		{0x3388, 0x4F}, /* TCLKZERO */
		{0x3389, 0x27}, /* TLPX */

		{0x3380, 0x20}, /* INCK_FREQ1_LSB */
		{0x3381, 0x25}, /* INCK_FREQ1_MSB */
		{0x338D, 0xB4}, /* INCK_FREQ2_LSB */
		{0x338E, 0x01}, /* INCK_FREQ2_MSB */

		/* DOL related */
		{0x300C, 0x11}, /* IMX224_WDMODE */
		{0x3020, 0x04}, /* IMX224_SHS1_LSB */
		{0x3021, 0x00}, /* IMX224_SHS1_MSB */
		{0x3022, 0x00}, /* IMX224_SHS1_HSB */
		{0x3023, 0x97}, /* IMX224_SHS2_LSB */
		{0x3024, 0x00}, /* IMX224_SHS2_MSB */
		{0x3025, 0x00}, /* IMX224_SHS2_HSB */
		{0x3026, 0x00}, /* IMX224_SHS3_LSB */
		{0x3027, 0x00}, /* IMX224_SHS3_MSB */
		{0x3028, 0x00}, /* IMX224_SHS3_HSB */
		{0x3354, 0x00}, /* IMX224_NULL0_SIZE */
		{0x3357, 0x6C}, /* IMX224_PIC_SIZE_LSB */
		{0x3358, 0x08}, /* IMX224_PIC_SIZE_MSB */
		{0x3043, 0x05}, /* IMX224_DOL_PAT1 */
		{0x3109, 0x01}, /* IMX224_XVSCNT_INT */
	},
};

static struct vin_reg_16_8 imx224_2lane_mode_regs[][31] = {
	{	/* Quad VGAp30 12bits */
		{0x3005, 0x01}, /* ADBIT */
		{0x3006, 0x00}, /* MODE */
		{0x3007, 0x00}, /* WINMODE */
		{0x3009, 0x02}, /* FRSEL */
		{0x3018, 0x4C}, /* VMAX_LSB */
		{0x3019, 0x04}, /* VMAX_MSB */
		{0x301B, 0x94}, /* HMAX_LSB */
		{0x301C, 0x11}, /* HMAX_MSB */
		{0x3044, 0x01}, /* ODBIT_OPORTSEL */
		{0x300A, 0xF0}, /* BLKLEVEL_LSB */
		{0x300B, 0x00}, /* BLKLEVEL_MSB */

		{0x3344, 0x10}, /* REPETITION */
		{0x3346, 0x01}, /* LANE_NUM */
		{0x3353, 0x0E}, /* OB_SIZE_V */
		{0x3357, 0xD1}, /* IMX224_PIC_SIZE_LSB */
		{0x3358, 0x03}, /* IMX224_PIC_SIZE_MSB */
		{0x336B, 0x37}, /* THSEXIT */
		{0x336C, 0x1F}, /* TCLKPRE */
		{0x337F, 0x01}, /* LANE_MODE */

		{0x3382, 0x5F}, /* TCLKPOST */
		{0x3383, 0x17}, /* THSPREPARE */
		{0x3384, 0x37}, /* THSZERO */
		{0x3385, 0x17}, /* THSTRAIL */
		{0x3386, 0x17}, /* TCLKTRAIL */
		{0x3387, 0x17}, /* TCLKPREPARE */
		{0x3388, 0x4F}, /* TCLKZERO */
		{0x3389, 0x27}, /* TLPX */

		{0x3380, 0x20}, /* INCK_FREQ1_LSB */
		{0x3381, 0x25}, /* INCK_FREQ1_MSB */
		{0x338D, 0xB4}, /* INCK_FREQ2_LSB */
		{0x338E, 0x01}, /* INCK_FREQ2_MSB */
	},
	{	/* HD720p30 12bits */
		{0x3005, 0x01}, /* ADBIT */
		{0x3006, 0x00}, /* MODE */
		{0x3007, 0x10}, /* WINMODE */
		{0x3009, 0x02}, /* FRSEL */
		{0x3018, 0xEE}, /* VMAX_LSB */
		{0x3019, 0x02}, /* VMAX_MSB */
		{0x301B, 0xC8}, /* HMAX_LSB */
		{0x301C, 0x19}, /* HMAX_MSB */
		{0x3044, 0x01}, /* ODBIT_OPORTSEL */
		{0x3357, 0xD9}, /* IMX224_PIC_SIZE_LSB */
		{0x3358, 0x02}, /* IMX224_PIC_SIZE_MSB */
		{0x300A, 0xF0}, /* BLKLEVEL_LSB */
		{0x300B, 0x00}, /* BLKLEVEL_MSB */

		{0x3344, 0x10}, /* REPETITION */
		{0x3346, 0x01}, /* LANE_NUM */
		{0x3353, 0x04}, /* OB_SIZE_V */
		{0x336B, 0x37}, /* THSEXIT */
		{0x336C, 0x1F}, /* TCLKPRE */
		{0x337F, 0x01}, /* LANE_MODE */

		{0x3382, 0x5F}, /* TCLKPOST */
		{0x3383, 0x17}, /* THSPREPARE */
		{0x3384, 0x37}, /* THSZERO */
		{0x3385, 0x17}, /* THSTRAIL */
		{0x3386, 0x17}, /* TCLKTRAIL */
		{0x3387, 0x17}, /* TCLKPREPARE */
		{0x3388, 0x4F}, /* TCLKZERO */
		{0x3389, 0x27}, /* TLPX */

		{0x3380, 0x20}, /* INCK_FREQ1_LSB */
		{0x3381, 0x25}, /* INCK_FREQ1_MSB */
		{0x338D, 0xB4}, /* INCK_FREQ2_LSB */
		{0x338E, 0x01}, /* INCK_FREQ2_MSB */
	},
};

static struct vin_video_format imx224_formats[] = {
	{
		.video_mode	= AMBA_VIDEO_MODE_1280_960,
		.def_start_x	= 4+4+8,
		.def_start_y	= 8,
		.def_width	= 1280,
		.def_height	= 960,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_LINEAR_MODE,
		.device_mode	= 0,
		.pll_idx	= 0,
		.width		= 1280,
		.height		= 960,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_30,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_720P,
		.def_start_x	= 4+4+8,
		.def_start_y	= 4,
		.def_width	= 1280,
		.def_height	= 720,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_LINEAR_MODE,
		.device_mode	= 1,
		.pll_idx	= 0,
		.width		= 1280,
		.height		= 720,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_30,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_1280_960,
		.def_start_x	= 4+4+8,
		.def_start_y	= (9+8)*2, /* FIXME: there are 9 OB lines for long-expo frame */
		.def_width	= 1280,
		.def_height	= 960 * 2 + (IMX224_960P_2X_RHS1 - 1), /* (960 + VBP1)*2 */
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1280,
		.act_height	= 960,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_2X_HDR_MODE,
		.device_mode	= 2,
		.pll_idx	= 0,
		.width		= IMX224_H_PIXEL*2+IMX224_HBLANK,
		.height		= 960,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_30,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
		.hdr_long_offset = 0,
		.hdr_short1_offset = (IMX224_960P_2X_RHS1 - 1) + 1,/* hdr_long_offset + 2 x VBP1 + 1 */
	},
};

static struct vin_video_format imx224_2lane_formats[] = {
	{
		.video_mode	= AMBA_VIDEO_MODE_1280_960,
		.def_start_x	= 4+4+8,
		.def_start_y	= 8,
		.def_width	= 1280,
		.def_height	= 960,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_LINEAR_MODE,
		.device_mode	= 0,
		.pll_idx	= 0,
		.width		= 1280,
		.height		= 960,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_30,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_720P,
		.def_start_x	= 4+4+8,
		.def_start_y	= 4,
		.def_width	= 1280,
		.def_height	= 720,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_LINEAR_MODE,
		.device_mode	= 1,
		.pll_idx	= 0,
		.width		= 1280,
		.height		= 720,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_30,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
};

static struct vin_reg_16_8 imx224_share_regs[] = {
	/* chip ID = 02h, do not change */
	{0x300F, 0x00},
	{0x3012, 0x2C},
	{0x3013, 0x01},
	{0x3016, 0x09},
	{0x301D, 0xC2},
	{0x3054, 0x66},
	{0x3070, 0x02},
	{0x3071, 0x01},
	{0x309E, 0x22},
	{0x30A5, 0xFB},
	{0x30A6, 0x02},
	{0x30B3, 0xFF},
	{0x30B4, 0x01},
	{0x30B5, 0x42},
	{0x30B8, 0x10},
	{0x30C2, 0x01},

	/* chip ID = 03h, do not change */
	{0x310F, 0x0F},
	{0x3110, 0x0E},
	{0x3111, 0xE7},
	{0x3112, 0x9C},
	{0x3113, 0x83},
	{0x3114, 0x10},
	{0x3115, 0x42},
	{0x3128, 0x1E},
	{0x31ED, 0x38},
	{0x31E9, 0x53},

	/* chip ID = 04h, do not change */
	{0x320C, 0xCF},
	{0x324C, 0x40},
	{0x324D, 0x03},
	{0x3261, 0xE0},
	{0x3262, 0x02},
	{0x326E, 0x2F},
	{0x326F, 0x30},
	{0x3270, 0x03},
	{0x3298, 0x00},
	{0x329A, 0x12},
	{0x329B, 0xF1},
	{0x329C, 0x0C},

	/* chip ID = 05h, do not change */
	{0x337D, 0x0C},
	{0x337E, 0x0C},
	{0x3380, 0x20},
	{0x3381, 0x25},
	{0x338d, 0xB4},
	{0x338e, 0x01},
};

/* Gain table */
/*0.0 dB - 72.0 dB /0.1 dB step */
#define IMX224_GAIN_MAX_DB  720

