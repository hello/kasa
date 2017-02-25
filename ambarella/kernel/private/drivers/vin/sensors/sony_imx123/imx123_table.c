/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx123/imx123_table.c
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

static struct vin_video_pll imx123_plls[] = {
	/* for QXGA */
	{0, 27000000, 72000000},
	/* for 1080p */
	{0, 27000000, 74250000},
	/* for 3M 2X DOL */
	{0, 37125000, 49500000},
};

static struct vin_reg_16_8 imx123_pll_regs[][6] = {
	{	/* for QXGA */
		{0x3061, 0xB1}, /* IMX123_INCKSEL1 */
		{0x3062, 0x00}, /* IMX123_INCKSEL2 */
		{0x316C, 0x2C}, /* IMX123_INCKSEL3 */
		{0x316D, 0x09}, /* IMX123_INCKSEL4 */
		{0x3170, 0x60}, /* IMX123_INCKSEL5 */
		{0x3171, 0x18}, /* IMX123_INCKSEL6 */
	},
	{	/* for 1080p */
		{0x3061, 0xA1}, /* IMX123_INCKSEL1 */
		{0x3062, 0x00}, /* IMX123_INCKSEL2 */
		{0x316C, 0x2C}, /* IMX123_INCKSEL3 */
		{0x316D, 0x09}, /* IMX123_INCKSEL4 */
		{0x3170, 0x60}, /* IMX123_INCKSEL5 */
		{0x3171, 0x16}, /* IMX123_INCKSEL6 */
	},
	{	/* for 3M 2X DOL */
		{0x3061, 0xB1}, /* IMX123_INCKSEL1 */
		{0x3062, 0x00}, /* IMX123_INCKSEL2 */
		{0x316C, 0x20}, /* IMX123_INCKSEL3 */
		{0x316D, 0x09}, /* IMX123_INCKSEL4 */
		{0x3170, 0x61}, /* IMX123_INCKSEL5 */
		{0x3171, 0x18}, /* IMX123_INCKSEL6 */
	},
};

static struct vin_reg_16_8 imx123_linear_mode_regs[][11] = {
	{	/* 4 lane, QXGA 60fps */
		{0x3005, 0x02}, /* ADBIT */
		{0x3007, 0x00}, /* WINMODE */
		{0x3009, 0x01}, /* DRSEL */
		{0x3018, 0x40}, /* VMAX_LSB */
		{0x3019, 0x06}, /* VMAX_MSB */
		{0x301A, 0x00}, /* VMAX_HSB */
		{0x301B, 0xEE}, /* HMAX_LSB */
		{0x301C, 0x02}, /* HMAX_MSB */
		{0x3044, 0x25}, /* ODBIT */
		{0x3130, 0x4D}, /* HS_1 */
		{0x31EB, 0x44}, /* HS_2 */
	},
	{	/* 4 lane 1080p linear */
		{0x3005, 0x02}, /* ADBIT */
		{0x3007, 0x10}, /* WINMODE */
		{0x3009, 0x01}, /* DRSEL */
		{0x3018, 0x65}, /* VMAX_LSB */
		{0x3019, 0x04}, /* VMAX_MSB */
		{0x301A, 0x00}, /* VMAX_HSB */
		{0x301B, 0x4C}, /* HMAX_LSB */
		{0x301C, 0x04}, /* HMAX_MSB */
		{0x3044, 0x25}, /* ODBIT */
		{0x3130, 0x4D}, /* HS_1 */
		{0x31EB, 0x44}, /* HS_2 */
	},
};

static struct vin_reg_16_8 imx123_dual_gain_share_regs[] = {
	{0x3012, 0x0E},
	{0x3013, 0x01},
	{0x3016, 0x09},
	{0x30F0, 0x0E},
	{0x30F1, 0x01},
	{0x30F4, 0x0E},
	{0x30F5, 0x01},

	{0x31F6, 0x1C},
	{0x32D9, 0x80},
	{0x32DA, 0x08},
	{0x32DC, 0xD0},
	{0x32DD, 0x13},
};

static struct vin_reg_16_8 imx123_3m_2x_regs[] = {
	/* 2X WDR setting QXGA 40 fps */
	{0x3007, 0x00}, /* WINMODE */
	{0x3009, 0x00}, /* DRSEL */
	{0x300C, 0x14}, /* WDSEL */
	{0x3044, 0x35}, /* ODBIT */
	{0x3046, 0x44}, /* XVSMSKCNT/XHSMSKCNT */
	{0x30C8, 0xC0}, /* HBLANK1, Tentative */
	{0x30C9, 0x40}, /* HBLANK1, Tentative */
	{0x30CA, 0xCD}, /* HBLANK2, Tentative */
	{0x30CB, 0x20}, /* HBLANK2, XVSMSKCNT_INT */
	{0x3018, 0x72}, /* VMAX_LSB */
	{0x3019, 0x06}, /* VMAX_MSB */
	{0x301A, 0x00}, /* VMAX_HSB */
	{0x301B, 0x77}, /* HMAX_LSB */
	{0x301C, 0x01}, /* HMAX_MSB */

	/* SHS1 3Ah */
	{0x301E, 0x3A}, /* SHS1_LSB */
	{0x301F, 0x00}, /* SHS1_MSB */
	{0x3020, 0x00}, /* SHS1_HSB */
	/* SHS2 E4h */
	{0x3021, 0xE4}, /* SHS2_LSB */
	{0x3022, 0x00}, /* SHS2_MSB */
	{0x3023, 0x00}, /* SHS2_HSB */
};

static struct vin_reg_16_8 imx123_3m_3x_regs[] = {
	/* 3X WDR setting QXGA 30fps */
	{0x3007, 0x00}, /* WINMODE */
	{0x3009, 0x00}, /* DRSEL */
	{0x300C, 0x24}, /* WDSEL */
	{0x3044, 0x35}, /* ODBIT */
	{0x3046, 0x8C}, /* XVSMSKCNT/XHSMSKCNT */
	{0x30C8, 0xC0}, /* {0x30C8, 0xD0},HBLANK1, Tentative */
	{0x30C9, 0x40}, /* HBLANK1, Tentative */
	{0x30CA, 0xC0}, /* {0x30CA, 0xD0},HBLANK2, Tentative */
	{0x30CB, 0x60}, /* HBLANK2, XVSMSKCNT_INT */
	{0x3018, 0x40}, /* VMAX_LSB */
	{0x3019, 0x06}, /* VMAX_MSB */
	{0x301A, 0x00}, /* VMAX_HSB */
	{0x301B, 0x77}, /* HMAX_LSB */
	{0x301C, 0x01}, /* HMAX_MSB */
	/* SHS1 14h */
	{0x301E, 0x14}, /* SHS1_LSB */
	{0x301F, 0x00}, /* SHS1_MSB */
	{0x3020, 0x00}, /* SHS1_HSB */
	/* SHS2 DEh */
	{0x3021, 0xDE}, /* SHS2_LSB */
	{0x3022, 0x00}, /* SHS2_MSB */
	{0x3023, 0x00}, /* SHS2_HSB */
	/* SHS3 100h */
	{0x3024, 0x00}, /* SHS3_LSB */
	{0x3025, 0x01}, /* SHS3_MSB */
	{0x3026, 0x00}, /* SHS3_HSB */
};

static struct vin_video_format imx123_formats[] = {
	{
		.video_mode	= AMBA_VIDEO_MODE_QXGA,
		.def_start_x	= 8,
		.def_start_y	= 1+12+8,
		.def_width	= 2048,
		.def_height	= 1536,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_LINEAR_MODE,
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
		.hdr_mode = AMBA_VIDEO_LINEAR_MODE,
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
	{
		.video_mode	= AMBA_VIDEO_MODE_QXGA,
		.def_start_x	= 8,
		.def_start_y	= (1+12+8)*2,
		.def_width	= 2048,
		.def_height	= 1536 * 2 + (IMX123_QXGA_2X_RHS1 - 2),/* (1536 + VBP1)*2 */
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 2048,
		.act_height	= 1536,
		.max_act_width = 2048,
		.max_act_height = IMX123_QXGA_BRL,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_2X_HDR_MODE,
		.device_mode	= 2,
		.pll_idx	= 2,
		.width		= IMX123_QXGA_H_PIXEL,
		.height		= IMX123_QXGA_BRL * 2 + (IMX123_QXGA_2X_RHS1 - 2),
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
		.hdr_long_offset = 0,
		.hdr_short1_offset = (IMX123_QXGA_2X_RHS1 - 2) + 1, /* hdr_long_offset + 2 x VBP1 + 1 */
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_QXGA,
		.def_start_x	= 8,
		.def_start_y	= (1+12+8)*3,
		.def_width	= 2048,
		.def_height	= 1536 * 3 + (IMX123_QXGA_3X_RHS2 - 4), /* (1536 + VBP2)*3 */
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 2048,
		.act_height	= 1536,
		.max_act_width = 2048,
		.max_act_height = IMX123_QXGA_BRL,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_3X_HDR_MODE,
		.device_mode	= 3,
		.pll_idx	= 0,
		.width		= IMX123_QXGA_H_PIXEL,
		.height		= IMX123_QXGA_BRL * 3 + (IMX123_QXGA_3X_RHS2 - 4),
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
		.hdr_long_offset = 0,
		.hdr_short1_offset = (IMX123_QXGA_3X_RHS1 - 2) + 1, /* hdr_long_offset + 3 x VBP1 + 1 */
		.hdr_short2_offset = (IMX123_QXGA_3X_RHS2 - 4) + 2, /* hdr_long_offset + 3 x VBP2 + 2 */
	},
};

static struct vin_video_format imx123_dual_gain_formats[] = {
	{
		.video_mode	= AMBA_VIDEO_MODE_QXGA,
		.def_start_x	= 8,
		.def_start_y	= 1+8+16+16,
		.def_width	= 2048,
		.def_height	= 1536*2,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 2048,
		.act_height	= 1536,
		.max_act_width = 2048,
		.max_act_height = 1536,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_2X_HDR_MODE,
		.device_mode	= 0,
		.pll_idx	= 0,
		.width		= 2048,
		.height		= 1536*2,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
		.hdr_long_offset = 0,
		.hdr_short1_offset = 1,
		.dual_gain_mode = 1,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_1080P,
		.def_start_x	= 8,
		.def_start_y	= 1+8+16+16,
		.def_width	= 1920,
		.def_height	= 1080*2,
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1920,
		.act_height	= 1080,
		.max_act_width = 1920,
		.max_act_height = 1080,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_2X_HDR_MODE,
		.device_mode	= 1,
		.pll_idx	= 1,
		.width		= 1920,
		.height		= 1080*2,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
		.hdr_long_offset = 0,
		.hdr_short1_offset = 1,
		.dual_gain_mode = 1,
	},
};

static struct vin_reg_16_8 imx123_share_regs[] = {
	/* chip ID: 02h */
	{0x3012, 0x0E},
	{0x3013, 0x01},
	{0x309D, 0x82},
	{0x309F, 0x01},
	{0x30B3, 0x91},

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
	//{0x31F6, 0x11},

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

#define IMX123_GAIN_MAX_DB  0x1E0 //low light: 48DB, high light:51DB
#define IMX123_DUAL_GAIN_MAX_DB  510 //51DB

