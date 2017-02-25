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

static struct vin_reg_16_8 imx224_mode_regs[][27] = {
	{	/* Quad VGAp120 10bits */
		{0x3005, 0x00}, /* ADBIT */
		{0x3006, 0x00}, /* MODE */
		{0x3007, 0x00}, /* WINMODE */
		{0x3009, 0x00}, /* FRSEL */
		{0x3018, 0x4C}, /* VMAX_LSB */
		{0x3019, 0x04}, /* VMAX_MSB */
		{0x301B, 0x65}, /* HMAX_LSB */
		{0x301C, 0x04}, /* HMAX_MSB */
		{0x3044, 0xE0}, /* ODBIT_OPORTSEL */
		{0x300A, 0x3C}, /* BLKLEVEL_LSB */
		{0x300B, 0x00}, /* BLKLEVEL_MSB */
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
		{0x310A, 0x00}, /* IMX224_DOL_PAT2 */
		{0x3109, 0x00}, /* IMX224_XVSCNT_INT */
	},
	{	/* 720p120 10bits */
		{0x3005, 0x00}, /* ADBIT */
		{0x3006, 0x00}, /* MODE */
		{0x3007, 0x10}, /* WINMODE */
		{0x3009, 0x00}, /* FRSEL */
		{0x3018, 0xEE}, /* VMAX_LSB */
		{0x3019, 0x02}, /* VMAX_MSB */
		{0x301B, 0x72}, /* HMAX_LSB */
		{0x301C, 0x06}, /* HMAX_MSB */
		{0x3044, 0xE0}, /* ODBIT_OPORTSEL */
		{0x300A, 0x3C}, /* BLKLEVEL_LSB */
		{0x300B, 0x00}, /* BLKLEVEL_MSB */
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
		{0x310A, 0x00}, /* IMX224_DOL_PAT2 */
		{0x3109, 0x00}, /* IMX224_XVSCNT_INT */
	},
	{	/* 640x480p60 12bits 2x2binning */
		{0x3005, 0x00}, /* ADBIT */
		{0x3006, 0x22}, /* MODE */
		{0x3007, 0x00}, /* WINMODE */
		{0x3009, 0x00}, /* FRSEL */
		{0x3018, 0x26}, /* VMAX_LSB */
		{0x3019, 0x02}, /* VMAX_MSB */
		{0x301B, 0x94}, /* HMAX_LSB */
		{0x301C, 0x11}, /* HMAX_MSB */
		{0x3044, 0xE1}, /* ODBIT_OPORTSEL */
		{0x300A, 0xF0}, /* BLKLEVEL_LSB */
		{0x300B, 0x00}, /* BLKLEVEL_MSB */
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
		{0x310A, 0x00}, /* IMX224_DOL_PAT2 */
		{0x3109, 0x00}, /* IMX224_XVSCNT_INT */
	},
	{	/* 2x DOL Quad VGAp60 10bits */
		{0x3005, 0x00}, /* ADBIT */
		{0x3006, 0x00}, /* MODE */
		{0x3007, 0x00}, /* WINMODE */
		{0x3009, 0x00}, /* FRSEL */
		{0x3018, 0x4C}, /* VMAX_LSB */
		{0x3019, 0x04}, /* VMAX_MSB */
		{0x301B, 0x65}, /* HMAX_LSB */
		{0x301C, 0x04}, /* HMAX_MSB */
		{0x3044, 0xE0}, /* ODBIT_OPORTSEL */
		{0x300A, 0x3C}, /* BLKLEVEL_LSB */
		{0x300B, 0x00}, /* BLKLEVEL_MSB */
		/* DOL related */
		{0x300C, 0x11}, /* IMX224_WDMODE */
		{0x3020, 0x04}, /* IMX224_SHS1_LSB */
		{0x3021, 0x00}, /* IMX224_SHS1_MSB */
		{0x3022, 0x00}, /* IMX224_SHS1_HSB */
		{0x3023, 0x57}, /* IMX224_SHS2_LSB */
		{0x3024, 0x00}, /* IMX224_SHS2_MSB */
		{0x3025, 0x00}, /* IMX224_SHS2_HSB */
		{0x3026, 0x00}, /* IMX224_SHS3_LSB */
		{0x3027, 0x00}, /* IMX224_SHS3_MSB */
		{0x3028, 0x00}, /* IMX224_SHS3_HSB */
		{0x3354, 0x00}, /* IMX224_NULL0_SIZE */
		{0x3357, 0xA2}, /* IMX224_PIC_SIZE_LSB */
		{0x3358, 0x07}, /* IMX224_PIC_SIZE_MSB */
		{0x3043, 0x00}, /* IMX224_DOL_PAT1 */
		{0x310A, 0x01}, /* IMX224_DOL_PAT2 */
		{0x3109, 0x01}, /* IMX224_XVSCNT_INT */
	},
	{	/* 3x DOL Quad VGAp30 10bits */
		{0x3005, 0x00}, /* ADBIT */
		{0x3006, 0x00}, /* MODE */
		{0x3007, 0x00}, /* WINMODE */
		{0x3009, 0x00}, /* FRSEL */
		{0x3018, 0x4C}, /* VMAX_LSB */
		{0x3019, 0x04}, /* VMAX_MSB */
		{0x301B, 0x65}, /* HMAX_LSB */
		{0x301C, 0x04}, /* HMAX_MSB */
		{0x3044, 0xE0}, /* ODBIT_OPORTSEL */
		{0x300A, 0x3C}, /* BLKLEVEL_LSB */
		{0x300B, 0x00}, /* BLKLEVEL_MSB */
		/* DOL related */
		{0x300C, 0x21}, /* IMX224_WDMODE */
		{0x3020, 0x04}, /* IMX224_SHS1_LSB */
		{0x3021, 0x00}, /* IMX224_SHS1_MSB */
		{0x3022, 0x00}, /* IMX224_SHS1_HSB */
		{0x3023, 0x89}, /* IMX224_SHS2_LSB */
		{0x3024, 0x00}, /* IMX224_SHS2_MSB */
		{0x3025, 0x00}, /* IMX224_SHS2_HSB */
		{0x3026, 0x2F}, /* IMX224_SHS3_LSB */
		{0x3027, 0x01}, /* IMX224_SHS3_MSB */
		{0x3028, 0x00}, /* IMX224_SHS3_HSB */
		{0x3354, 0x00}, /* IMX224_NULL0_SIZE */
		{0x3357, 0x73}, /* IMX224_PIC_SIZE_LSB */
		{0x3358, 0x0B}, /* IMX224_PIC_SIZE_MSB */
		{0x3043, 0x00}, /* IMX224_DOL_PAT1 */
		{0x310A, 0x01}, /* IMX224_DOL_PAT2 */
		{0x3109, 0x03}, /* IMX224_XVSCNT_INT */
	},
};

static struct vin_video_format imx224_formats[] = {
	{
		.video_mode	= AMBA_VIDEO_MODE_1280_960,
		.def_start_x	= 4+4+8,
		.def_start_y	= 1+2+14+8,
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
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.max_fps	= AMBA_VIDEO_FPS_120,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_720P,
		.def_start_x	= 4+4+8,
		.def_start_y	= 1+2+4+2,
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
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_120,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_VGA,
		.def_start_x	= 2+2+4,
		.def_start_y	= 1+2+6+4,
		.def_width	= 640,
		.def_height	= 480,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_LINEAR_MODE,
		.device_mode	= 2,
		.pll_idx	= 0,
		.width		= 640,
		.height		= 480,
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
		.video_mode	= AMBA_VIDEO_MODE_1280_960,
		.def_start_x	= 4+4+8,
		.def_start_y	= (1+2+14+8)*2,
		.def_width	= 1280,
		.def_height	= 960 * 2 + (IMX224_960P_2X_RHS1 - 1), /* (960 + VBP1)*2 */
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1280,
		.act_height	= 960,
		.max_act_width = 1280,
		.max_act_height = IMX224_QVGA_BRL,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_2X_HDR_MODE,
		.device_mode	= 3,
		.pll_idx	= 0,
		.width		= IMX224_H_PIXEL*2+IMX224_HBLANK,
		.height		= 960,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
#ifdef USE_960P_2X_30FPS
		.max_fps	= AMBA_VIDEO_FPS_30,
#else
		.max_fps	= AMBA_VIDEO_FPS_60,
#endif
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
		.hdr_long_offset = 0,
		.hdr_short1_offset = (IMX224_960P_2X_RHS1 - 1) + 1,/* hdr_long_offset + 2 x VBP1 + 1 */
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_1280_960,
		.def_start_x	= 4+4+8,
		.def_start_y	= (1+2+14+8)*3,
		.def_width	= 1280,
		.def_height	= 960 * 3 + (IMX224_960P_3X_RHS2 - 2), /* (960 + VBP2)*3 */
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1280,
		.act_height	= 960,
		.max_act_width = 1280,
		.max_act_height = IMX224_QVGA_BRL,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_3X_HDR_MODE,
		.device_mode	= 4,
		.pll_idx	= 0,
		.width		= IMX224_H_PIXEL*3+IMX224_HBLANK*2,
		.height		= 960,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
		.hdr_long_offset = 0,
		.hdr_short1_offset = (IMX224_960P_3X_RHS1 - 1) + 1,/* hdr_long_offset + 3 x VBP1 + 1 */
		.hdr_short2_offset = (IMX224_960P_3X_RHS2 - 2) + 2,/* hdr_long_offset + 3 x VBP2 + 2 */
	},
};

static struct vin_reg_16_8 imx224_share_regs[] = {
	/* chip ID = 02h, do not change */
	{0x300F, 0x00},
	{0x3012, 0x2C},
	{0x3013, 0x01},
	{0x3016, 0x09},
	{0x301D, 0xC2},
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
};

/* Gain table */
/*0.0 dB - 72.0 dB /0.1 dB step */
#define IMX224_GAIN_MAX_DB  720

