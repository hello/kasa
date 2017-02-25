/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx124/imx124_table.c
 *
 * History:
 *    2014/07/23 - [Long Zhao] Create
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

static struct vin_video_pll imx124_plls[] = {
	{0, 27000000, 72000000}, /* for 3M */
	{0, 27000000, 74250000}, /* for 1080p */
};

static struct vin_reg_16_8 imx124_pll_regs[][6] = {
	{	/* for 3M */
		{IMX124_INCKSEL1, 0xB1},
		{IMX124_INCKSEL2, 0x00},
		{IMX124_INCKSEL3, 0x2C},
		{IMX124_INCKSEL4, 0x09},
		{IMX124_INCKSEL5, 0x61},
		{IMX124_INCKSEL6, 0x30},
	},
	{	/* for 1080p */
		{IMX124_INCKSEL1, 0xA1},
		{IMX124_INCKSEL2, 0x00},
		{IMX124_INCKSEL3, 0x2C},
		{IMX124_INCKSEL4, 0x09},
		{IMX124_INCKSEL5, 0x60},
		{IMX124_INCKSEL6, 0x16},
	},
};

static struct vin_reg_16_8 imx124_mode_regs[][7] = {
	{	/* 4ch 3M linear */
		{IMX124_WINMODE,	0x08},
		{IMX124_FRSEL,		0x01},
		{IMX124_VMAX_LSB,	0x40},
		{IMX124_VMAX_MSB,	0x06},
		{IMX124_VMAX_HSB,	0x00},
		{IMX124_HMAX_LSB,	0xEE},
		{IMX124_HMAX_MSB,	0x02},
	},
	{	/* 4ch 1080p linear */
		{IMX124_WINMODE,	0x18},
		{IMX124_FRSEL,		0x01},
		{IMX124_VMAX_LSB,	0x65},
		{IMX124_VMAX_MSB,	0x04},
		{IMX124_VMAX_HSB,	0x00},
		{IMX124_HMAX_LSB,	0x4C},
		{IMX124_HMAX_MSB,	0x04},
	}
};

static struct vin_reg_16_8 imx124_share_regs[] = {
	/* chip ID: 02h */
	{0x3012, 0x0E},
	{0x3013, 0x01},
	{0x305A, 0x01},
	{0x309D, 0x82},
	{0x309F, 0x01},
	{0x30B3, 0x91},
	{0x30B8, 0x04},

	/* chip ID: 03h */
	{0x3106, 0x07},
	{0x3107, 0x00},
	{0x3108, 0x00},
	{0x3109, 0x00},
	{0x310A, 0x00},
	{0x310B, 0x00},
	{0x3110, 0xF2},
	{0x3111, 0x03},
	{0x3112, 0xEB},
	{0x3113, 0x07},
	{0x3114, 0xED},
	{0x3115, 0x07},
	{0x3126, 0x91},
	{0x3130, 0x42},
	{0x3133, 0x12},
	{0x3134, 0x10},
	{0x3135, 0x12},
	{0x3136, 0x10},
	{0x313A, 0x0C},
	{0x313B, 0x0C},
	{0x313C, 0x0C},
	{0x313D, 0x0C},
	{0x3140, 0x00},
	{0x3141, 0x00},
	{0x3144, 0x1E},
	{0x3149, 0x55},
	{0x314B, 0x99},
	{0x314C, 0x99},
	{0x3154, 0xE7},
	{0x315A, 0x04},
	{0x3179, 0x94},
	{0x317A, 0x06},
	{0x31EB, 0x3E},
	{0x31F6, 0x59},

	/* chip ID: 04h */
	{0x3201, 0x3C},
	{0x3202, 0x01},
	{0x3203, 0x0E},
	{0x3213, 0x05},
	{0x321F, 0x05},
	{0x325F, 0x03},
	{0x3269, 0x03},
	{0x32B6, 0x03},
	{0x32BA, 0x01},
	{0x32C4, 0x01},
	{0x32CB, 0x01},
	{0x32D9, 0x80},
	{0x32DC, 0xB0},
	{0x32DD, 0x13},

	/* chip ID: 05h */
	{0x332A, 0xFF},
	{0x332B, 0xFF},
	{0x332C, 0xFF},
	{0x332D, 0xFF},
	{0x332E, 0xFF},
	{0x332F, 0xFF},
	{0x3335, 0x50},
	{0x3336, 0x80},
	{0x3337, 0x1B},
	{0x333C, 0x01},
	{0x333D, 0x03},
};

static struct vin_video_format imx124_formats[] = {
	{
		.video_mode	= AMBA_VIDEO_MODE_QXGA,
		.def_start_x	= 8,
		.def_start_y	= 1+12+8,
		.def_width	= 2048,
		.def_height	= 1536,
		/* sensor mode */
		.device_mode	= 0,
		.pll_idx	= 0,
		.width		= 2048,
		.height		= 1536,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.max_fps	= AMBA_VIDEO_FPS_60,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_1080P,
		.def_start_x	= 8,
		.def_start_y	= 1+12+8,
		.def_width	= 1920,
		.def_height	= 1080,
		/* sensor mode */
		.device_mode	= 1,
		.pll_idx	= 1,
		.width		= 1920,
		.height		= 1080,
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

#define IMX124_GAIN_MAX_DB  510

