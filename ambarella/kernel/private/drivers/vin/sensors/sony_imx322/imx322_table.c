/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx322/imx322_reg_tbl.c
 *
 * History:
 *    2014/08/15 - [Long Zhao] Create
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

static struct vin_video_pll imx322_plls[] = {
	{0, 37125000, 37125000},
};

static struct vin_reg_16_8 imx322_mode_regs[][18] = {
	{	/* 1080P 12bits 30fps */
#ifdef CONFIG_SENSOR_IMX322_SPI
		{0x0211, 0x00}, /* FRSEL */
		{0x0202, 0x0f}, /* HD1080p mode */
		{0x0216, 0x3c}, /* WINPV_LSB */
		{0x0222, 0x40}, /* 720PMODE */
		{0x022D, 0x40}, /* DCKDLY_BITSEL */
		{0x029a, 0x26}, /* 12B1080P_LSB */
		{0x029b, 0x02}, /* 12B1080P_MSB */
		{0x02ce, 0x16}, /* PRES */
		{0x02cf, 0x82}, /* DRES_LSB */
		{0x02d0, 0x00}, /* DRES_MSB */
		{0x0220, 0xf0}, /* BLKLEVEL */
		{0x0203, 0x4c}, /* HMAX_LSB */
		{0x0204, 0x04}, /* HMAX_MSB */
		{0x0205, 0x65}, /* VMAX_LSB */
		{0x0206, 0x04}, /* VMAX_MSB */
		{0x0212, 0x82}, /* ADRES */
		{0xffff, 0xff},
#else
		{0x3011, 0x00}, /* FRSEL */
		{0x3012, 0x82}, /* ADRES */
		{0x3002, 0x0f}, /* HD1080p mode */
		{0x3016, 0x3c}, /* WINPV_LSB */
		{0x3022, 0x40}, /* 720PMODE */
		{0x302D, 0x40}, /* DCKDLY_BITSEL */
		{0x309a, 0x26}, /* 12B1080P_LSB */
		{0x309b, 0x02}, /* 12B1080P_MSB */
		{0x30ce, 0x16}, /* PRES */
		{0x30cf, 0x82}, /* DRES_LSB */
		{0x30d0, 0x00}, /* DRES_MSB */
		{0x0009, 0xf0}, /* I2C BLKLEVEL */
		{0x0112, 0x0c}, /* I2C ADRES1 */
		{0x0113, 0x0c}, /* I2C ADRES2 */
		{0x0340, 0x04}, /* VMAX_MSB */
		{0x0341, 0x65}, /* VMAX_LSB */
		{0x0342, 0x04}, /* HMAX_MSB */
		{0x0343, 0x4c}, /* HMAX_LSB */
#endif
	},
	{	/* 720P 10bits 60fps */
#ifdef CONFIG_SENSOR_IMX322_SPI
		{0x0211, 0x00}, /* FRSEL */
		{0x0202, 0x01}, /* HD720p mode */
		{0x0216, 0xf0}, /* WINPV_LSB */
		{0x0222, 0xc0}, /* 720PMODE */
		{0x022D, 0x48}, /* DCKDLY_BITSEL */
		{0x029a, 0x4c}, /* 12B1080P_LSB */
		{0x029b, 0x04}, /* 12B1080P_MSB */
		{0x02ce, 0x00}, /* PRES */
		{0x02cf, 0x00}, /* DRES_LSB */
		{0x02d0, 0x00}, /* DRES_MSB */
		{0x0220, 0x3c}, /* BLKLEVEL */
		{0x0203, 0x39}, /* HMAX_LSB */
		{0x0204, 0x03}, /* HMAX_MSB */
		{0x0205, 0xee}, /* VMAX_LSB */
		{0x0206, 0x02}, /* VMAX_MSB */
		{0x0212, 0x80}, /* ADRES */
		{0xffff, 0xff},
#else
		{0x3011, 0x00}, /* FRSEL */
		{0x3012, 0x80}, /* ADRES */
		{0x3002, 0x01}, /* HD720p mode */
		{0x3016, 0xf0}, /* WINPV_LSB */
		{0x3022, 0xc0}, /* 720PMODE */
		{0x302D, 0x48}, /* DCKDLY_BITSEL */
		{0x309a, 0x4c}, /* 12B1080P_LSB */
		{0x309b, 0x04}, /* 12B1080P_MSB */
		{0x30ce, 0x00}, /* PRES */
		{0x30cf, 0x00}, /* DRES_LSB */
		{0x30d0, 0x00}, /* DRES_MSB */
		{0x0009, 0x3c}, /* I2C BLKLEVEL */
		{0x0112, 0x0a}, /* I2C ADRES1 */
		{0x0113, 0x0a}, /* I2C ADRES2 */
		{0x0340, 0x02}, /* VMAX_MSB */
		{0x0341, 0xee}, /* VMAX_LSB */
		{0x0342, 0x03}, /* HMAX_MSB */
		{0x0343, 0x39}, /* HMAX_LSB */
#endif
	},
	{	/* 720P 12bits 30fps */
#ifdef CONFIG_SENSOR_IMX322_SPI
		{0x0211, 0x01}, /* FRSEL */
		{0x0202, 0x01}, /* HD720p mode */
		{0x0216, 0xf0}, /* WINPV_LSB */
		{0x0222, 0xc0}, /* 720PMODE */
		{0x022D, 0x40}, /* DCKDLY_BITSEL */
		{0x029a, 0x4c}, /* 12B1080P_LSB */
		{0x029b, 0x04}, /* 12B1080P_MSB */
		{0x02ce, 0x40}, /* PRES */
		{0x02cf, 0x81}, /* DRES_LSB */
		{0x02d0, 0x01}, /* DRES_MSB */
		{0x0220, 0xf0}, /* BLKLEVEL */
		{0x0203, 0x72}, /* HMAX_LSB */
		{0x0204, 0x06}, /* HMAX_MSB */
		{0x0205, 0xee}, /* VMAX_LSB */
		{0x0206, 0x02}, /* VMAX_MSB */
		{0x0212, 0x82}, /* ADRES */
		{0xffff, 0xff},
#else
		{0x3011, 0x01}, /* FRSEL */
		{0x3012, 0x82}, /* ADRES */
		{0x3002, 0x01}, /* HD720p mode */
		{0x3016, 0xf0}, /* WINPV_LSB */
		{0x3022, 0xc0}, /* 720PMODE */
		{0x302D, 0x40}, /* DCKDLY_BITSEL */
		{0x309a, 0x4c}, /* 12B1080P_LSB */
		{0x309b, 0x04}, /* 12B1080P_MSB */
		{0x30ce, 0x40}, /* PRES */
		{0x30cf, 0x81}, /* DRES_LSB */
		{0x30d0, 0x01}, /* DRES_MSB */
		{0x0009, 0xf0}, /* I2C BLKLEVEL */
		{0x0112, 0x0c}, /* I2C ADRES1 */
		{0x0113, 0x0c}, /* I2C ADRES2 */
		{0x0340, 0x02}, /* VMAX_MSB */
		{0x0341, 0xee}, /* VMAX_LSB */
		{0x0342, 0x06}, /* HMAX_MSB */
		{0x0343, 0x72}, /* HMAX_LSB */
#endif
	},
};

static struct vin_video_format imx322_formats[] = {
	{
		.video_mode	= AMBA_VIDEO_MODE_1080P,
		.def_start_x	= 16+24+8,
		.def_start_y	= 1+4+8+4+8,
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
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_GB,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_720P,
		.def_start_x	= 16+24+8,
		.def_start_y	= 1+4+6+2+4,
		.def_width	= 1280,
		.def_height	= 720,
		/* sensor mode */
		.device_mode	= 1,
		.pll_idx	= 0,
		.width		= 1280,
		.height		= 720,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_60,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_GB,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_720P,
		.def_start_x	= 16+24+8,
		.def_start_y	= 1+4+6+2+4,
		.def_width	= 1280,
		.def_height	= 720,
		/* sensor mode */
		.device_mode	= 2,
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
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_GB,
	},
};

static struct vin_reg_16_8 imx322_share_regs[] = {
#ifdef CONFIG_SENSOR_IMX322_SPI
	{0x021f, 0x73},
	{0x0227, 0x20},
	{0x0317, 0x0d},
	{0x024f, 0x47}, /* SYNC2_EN */
	{0x0254, 0x13}, /* SYNCSEL */
#else
	{0x301f, 0x73},
	{0x3027, 0x20},
	{0x3117, 0x0d},
	{0x304f, 0x47}, /* SYNC2_EN */
	{0x3054, 0x13}, /* SYNCSEL */
#endif
};

#ifdef CONFIG_PM
static struct vin_reg_16_8 pm_regs[] = {
	{IMX322_SHS1_MSB, 0x00},
	{IMX322_SHS1_LSB, 0x00},
	{IMX322_REG_GAIN, 0x00},
};
#endif

#define IMX322_GAIN_MAX_DB	0x8C

