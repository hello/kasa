/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx291_mipi/imx291_table.c
 *
 * History:
 *    2016/08/23 - [Hao Zeng] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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

static struct vin_video_pll imx291_plls[] = {
	{0, 37125000, 148500000}, /* for 1080p */
	{0, 37125000, 148500000}, /* for 720p */
};

static struct vin_reg_16_8 imx291_pll_regs[][6] = {
	{	/* for 1080p */
		{IMX291_INCKSEL1, 0x18},
		{IMX291_INCKSEL2, 0x03},
		{IMX291_INCKSEL3, 0x20},
		{IMX291_INCKSEL4, 0x01},
		{IMX291_INCKSEL5, 0x1A},
		{IMX291_INCKSEL6, 0x1A},
	},
	{	/* for 720p */
		{IMX291_INCKSEL1, 0x20},
		{IMX291_INCKSEL2, 0x00},
		{IMX291_INCKSEL3, 0x20},
		{IMX291_INCKSEL4, 0x01},
		{IMX291_INCKSEL5, 0x1A},
		{IMX291_INCKSEL6, 0x1A},
	},
};

static struct vin_reg_16_8 imx291_mode_regs[][26] = {
	{	/* 1080p30 12bits */
		{IMX291_ADBIT,			0x01},
		{IMX291_WINMODE,		0x00},
		{IMX291_FRSEL,			0x02},
		{IMX291_VMAX_LSB,		0x65},
		{IMX291_VMAX_MSB,		0x04},
		{IMX291_VMAX_HSB,		0x00},
		{IMX291_HMAX_LSB,		0x30},
		{IMX291_HMAX_MSB,		0x11},
		{IMX291_ODBIT,			0x01},
		{IMX291_BLKLEVEL_LSB,	0xF0},
		{IMX291_BLKLEVEL_MSB,	0x00},
		/* mipi parameters */
		{IMX291_OPB_SIZE_V,		0x0A},
		{IMX291_Y_OUT_SIZE_LSB,	0x49},
		{IMX291_Y_OUT_SIZE_MSB,	0x04},
		{IMX291_X_OUT_SIZE_LSB,	0x9C},
		{IMX291_X_OUT_SIZE_MSB,	0x07},
		{IMX291_CSI_DT_FMT_LSB,	0x0C},
		{IMX291_CSI_DT_FMT_MSB,	0x0C},
		{IMX291_TCLKPOST_LSB,	0x57},
		{IMX291_THSZERO_LSB,	0x37},
		{IMX291_THSPREPARE_LSB,	0x1F},
		{IMX291_TCLKTRAIL_LSB,	0x1F},
		{IMX291_THSTRAIL_LSB,	0x1F},
		{IMX291_TCLKZERO_LSB,	0x77},
		{IMX291_TCLKPREPARE_LSB,0x1F},
		{IMX291_TLPX_LSB,		0x17},
	},
	{	/* 1080p30 10bits */
		{IMX291_ADBIT,			0x00},
		{IMX291_WINMODE,		0x00},
		{IMX291_FRSEL,			0x02},
		{IMX291_VMAX_LSB,		0x65},
		{IMX291_VMAX_MSB,		0x04},
		{IMX291_VMAX_HSB,		0x00},
		{IMX291_HMAX_LSB,		0x30},
		{IMX291_HMAX_MSB,		0x11},
		{IMX291_ODBIT,			0x00},
		{IMX291_BLKLEVEL_LSB,	0x3C},
		{IMX291_BLKLEVEL_MSB,	0x00},
		/* mipi parameters */
		{IMX291_OPB_SIZE_V,		0x0A},
		{IMX291_Y_OUT_SIZE_LSB,	0x49},
		{IMX291_Y_OUT_SIZE_MSB,	0x04},
		{IMX291_X_OUT_SIZE_LSB,	0x9C},
		{IMX291_X_OUT_SIZE_MSB,	0x07},
		{IMX291_CSI_DT_FMT_LSB,	0x0A},
		{IMX291_CSI_DT_FMT_MSB,	0x0A},
		{IMX291_TCLKPOST_LSB,	0x57},
		{IMX291_THSZERO_LSB,	0x37},
		{IMX291_THSPREPARE_LSB,	0x1F},
		{IMX291_TCLKTRAIL_LSB,	0x1F},
		{IMX291_THSTRAIL_LSB,	0x1F},
		{IMX291_TCLKZERO_LSB,	0x77},
		{IMX291_TCLKPREPARE_LSB,0x1F},
		{IMX291_TLPX_LSB,		0x17},
	},
	{	/* 720p30 12bits */
		{IMX291_ADBIT,			0x01},
		{IMX291_WINMODE,		0x10},
		{IMX291_FRSEL,			0x02},
		{IMX291_VMAX_LSB,		0xEE},
		{IMX291_VMAX_MSB,		0x02},
		{IMX291_VMAX_HSB,		0x00},
		{IMX291_HMAX_LSB,		0xC8},
		{IMX291_HMAX_MSB,		0x19},
		{IMX291_ODBIT,			0x01},
		{IMX291_BLKLEVEL_LSB,	0xF0},
		{IMX291_BLKLEVEL_MSB,	0x00},
		/* mipi parameters */
		{IMX291_OPB_SIZE_V,		0x04},
		{IMX291_Y_OUT_SIZE_LSB,	0xD9},
		{IMX291_Y_OUT_SIZE_MSB,	0x02},
		{IMX291_X_OUT_SIZE_LSB,	0x1C},
		{IMX291_X_OUT_SIZE_MSB,	0x05},
		{IMX291_CSI_DT_FMT_LSB,	0x0C},
		{IMX291_CSI_DT_FMT_MSB,	0x0C},
		{IMX291_TCLKPOST_LSB,	0x4F},
		{IMX291_THSZERO_LSB,	0x2F},
		{IMX291_THSPREPARE_LSB,	0x17},
		{IMX291_TCLKTRAIL_LSB,	0x17},
		{IMX291_THSTRAIL_LSB,	0x17},
		{IMX291_TCLKZERO_LSB,	0x57},
		{IMX291_TCLKPREPARE_LSB,0x17},
		{IMX291_TLPX_LSB,		0x17},
	},
	{	/* 720p30 10bits */
		{IMX291_ADBIT,			0x00},
		{IMX291_WINMODE,		0x10},
		{IMX291_FRSEL,			0x02},
		{IMX291_VMAX_LSB,		0xEE},
		{IMX291_VMAX_MSB,		0x02},
		{IMX291_VMAX_HSB,		0x00},
		{IMX291_HMAX_LSB,		0xC8},
		{IMX291_HMAX_MSB,		0x19},
		{IMX291_ODBIT,			0x00},
		{IMX291_BLKLEVEL_LSB,	0x3C},
		{IMX291_BLKLEVEL_MSB,	0x00},
		/* mipi parameters */
		{IMX291_OPB_SIZE_V,		0x04},
		{IMX291_Y_OUT_SIZE_LSB,	0xD9},
		{IMX291_Y_OUT_SIZE_MSB,	0x02},
		{IMX291_X_OUT_SIZE_LSB,	0x1C},
		{IMX291_X_OUT_SIZE_MSB,	0x05},
		{IMX291_CSI_DT_FMT_LSB,	0x0A},
		{IMX291_CSI_DT_FMT_MSB,	0x0A},
		{IMX291_TCLKPOST_LSB,	0x4F},
		{IMX291_THSZERO_LSB,	0x2F},
		{IMX291_THSPREPARE_LSB,	0x17},
		{IMX291_TCLKTRAIL_LSB,	0x17},
		{IMX291_THSTRAIL_LSB,	0x17},
		{IMX291_TCLKZERO_LSB,	0x57},
		{IMX291_TCLKPREPARE_LSB,0x17},
		{IMX291_TLPX_LSB,		0x17},
	},
};

static struct vin_reg_16_8 imx291_share_regs[] = {
	/* chip ID: 02h */
	{0x300F, 0x00},
	{0x3010, 0x21},
	{0x3012, 0x64},
	{0x3016, 0x08}, /* set to 0x08 to fix flare issue for switching HCG/LCG mode */
	{0x3070, 0x02},
	{0x3071, 0x11},
	{0x30A6, 0x20},
	{0x30A8, 0x20},
	{0x30AA, 0x20},
	{0x30AC, 0x20},

	/* chip ID: 03h */
	{0x310B, 0x01}, /* according to sony fae's suggestion, set to 1 to fix lowlight issue for 1080p */
	{0x3119, 0x9E},
	{0x311E, 0x08},
	{0x3128, 0x05},
	{0x3134, 0x0F},
	{0x313B, 0x50},
	{0x313D, 0x83},
	{0x317C, 0x00},
	{0x317F, 0x00},

	/* chip ID: 04h */
	{0x32B8, 0x50},
	{0x32B9, 0x10},
	{0x32BA, 0x00},
	{0x32BB, 0x04},
	{0x32C8, 0x50},
	{0x32C9, 0x10},
	{0x32CA, 0x00},
	{0x32CB, 0x04},

	/* chip ID: 05h */
	{0x332C, 0xD3},
	{0x332D, 0x10},
	{0x332E, 0x0D},
	{0x3358, 0x06},
	{0x3359, 0xE1},
	{0x335A, 0x11},
	{0x3360, 0x1E},
	{0x3361, 0x61},
	{0x3362, 0x10},
	{0x33B0, 0x08},
	{0x33B1, 0x30},
	{0x33B3, 0x04},

	/* chip ID: 06h */
	{0x3405, 0x10}, /* REPETITION */
	{0x3407, 0x01}, /* PHY_LANE_NUM */
	{0x3443, 0x01}, /* CSI_LANE_MODE */
	{0x3444, 0x20}, /* EXTCK_FREQ_LSB */
	{0x3445, 0x25}, /* EXTCK_FREQ_MSB */
};

#ifdef CONFIG_PM
static struct vin_reg_16_8 pm_regs[] = {
	{IMX291_SHS1_HSB, 0x00},
	{IMX291_SHS1_MSB, 0x00},
	{IMX291_SHS1_LSB, 0x00},
	{IMX291_GAIN, 0x00},
	{IMX291_FRSEL, 0x00},
};
#endif

static struct vin_video_format imx291_formats[] = {
	{
		.video_mode	= AMBA_VIDEO_MODE_1080P,
		.def_start_x	= 4+8,
		.def_start_y	= 8,
		.def_width	= 1920,
		.def_height	= 1080,
		/* sensor mode */
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
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_1080P,
		.def_start_x	= 4+8,
		.def_start_y	= 8,
		.def_width	= 1920,
		.def_height	= 1080,
		/* sensor mode */
		.device_mode	= 1,
		.pll_idx	= 0,
		.width		= 1920,
		.height		= 1080,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_720P,
		.def_start_x	= 4+8,
		.def_start_y	= 4,
		.def_width	= 1280,
		.def_height	= 720,
		/* sensor mode */
		.device_mode	= 2,
		.pll_idx	= 1,
		.width		= 1280,
		.height		= 720,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_720P,
		.def_start_x	= 4+8,
		.def_start_y	= 4,
		.def_width	= 1280,
		.def_height	= 720,
		/* sensor mode */
		.device_mode	= 3,
		.pll_idx	= 1,
		.width		= 1280,
		.height		= 720,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
};

#define IMX291_GAIN_MAX_DB  210 /* 63dB */

