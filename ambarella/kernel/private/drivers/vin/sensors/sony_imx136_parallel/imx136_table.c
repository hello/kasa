/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx136/imx136_table.c
 *
 * History:
 *    2012/02/21 - [Long Zhao] Create
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

static struct vin_video_pll imx136_plls[] = {
	{0, 37125000, 148500000},
};

static struct vin_reg_16_8 imx136_mode_regs[][15] = {
	{	/* 1080P(12bits)@30fps */
		{0x0205, 0x01},/* ADBIT */
		{0x0206, 0x00},/* MODE */
		{0x0207, 0x10},/* WINMODE */
		{0x0209, 0x02},/* FRSEL */
		{0x0218, 0x65},/* VMAX_LSB */
		{0x0219, 0x04},/* VMAX_MSB */
		{0x021A, 0x00},/* VMAX_HSB */
		{0x021B, 0x30},/* HMAX_LSB */
		{0x021C, 0x11},/* HMAX_MSB */
		{0x0244, 0x01},/* OUTCTRL */
		{0x025B, 0x01},/* INCKSEL0 */
		{0x025C, 0x20},/* INCKSEL1 */
		{0x025D, 0x06},/* INCKSEL2 */
		{0x025E, 0x30},/* INCKSEL3 */
		{0x025F, 0x04},/* INCKSEL4 */
	},
};

static struct vin_video_format imx136_formats[] = {
	{
		.video_mode	= AMBA_VIDEO_MODE_1080P,
		.def_start_x	= 4+4+8,
		.def_start_y	= 1+3+8+4+8,
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
};

static struct vin_reg_16_8 imx136_share_regs[] = {
	/* Chip ID=02h, do not change */
	{0x020A, 0xF0}, /* Black level */
	{0x020B, 0x00},

	{0x020C, 0x00}, /* 0:Normal mode, 2:Build-in WDR mode */
	{0x0210, 0x01},
	{0x0212, 0x00},
	{0x0249, 0x0A}, /* HSYNC/VSYNC output */
	{0x0265, 0x00},
	{0x0284, 0x10}, /* VRSET */
	{0x0286, 0x10},
	{0x02CF, 0xE1},
	{0x02D0, 0x30},
	{0x02D2, 0xC4},
	{0x02D3, 0x01},

	/* Chip ID=03h, do not change */
	{0x030F, 0x0E},
	{0x0316, 0x02},

	/* Chip ID=04h, do not change */
	{0x0436, 0x71},
	{0x0439, 0xF1},
	{0x0441, 0xF2},
	{0x0442, 0x21},
	{0x0443, 0x21},
	{0x0448, 0xF2},
	{0x0449, 0x21},
	{0x044A, 0x21},
	{0x0452, 0x01},
	{0x0454, 0xB1},
};

#define IMX136_GAIN_ROWS  421
#define IMX136_GAIN_42DB  0x1A4

