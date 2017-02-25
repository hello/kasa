/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx104/imx104_table.c
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

static struct vin_video_pll imx104_plls[] = {
	{0, 37125000, 148500000},
};

static struct vin_reg_16_8 imx104_mode_regs[][15] = {
	{
		{IMX104_ADBIT,        0x01},
		{IMX104_MODE,         0x00},
		{IMX104_WINMODE,      0x00},
		{IMX104_FRSEL,        0x00},
		{IMX104_VMAX_LSB,     0x4C},
		{IMX104_VMAX_MSB,     0x04},
		{IMX104_VMAX_HSB,     0x00},
		{IMX104_HMAX_LSB,     0xCA},
		{IMX104_HMAX_MSB,     0x08},
		{IMX104_OUTCTRL,      0xE1},
		{IMX104_INCKSEL1,     0x00},
		{IMX104_INCKSEL2,     0x00},
		{IMX104_INCKSEL3,     0x00},
		{IMX104_BLKLEVEL_LSB, 0xF0},
		{IMX104_BLKLEVEL_MSB, 0x00},
	},
	{
		{IMX104_ADBIT,        0x01},
		{IMX104_MODE,         0x00},
		{IMX104_WINMODE,      0x10},
		{IMX104_FRSEL,        0x01},
		{IMX104_VMAX_LSB,     0xEE},
		{IMX104_VMAX_MSB,     0x02},
		{IMX104_VMAX_HSB,     0x00},
		{IMX104_HMAX_LSB,     0xE4},
		{IMX104_HMAX_MSB,     0x0C},
		{IMX104_OUTCTRL,      0xE1},
		{IMX104_INCKSEL1,     0x00},
		{IMX104_INCKSEL2,     0x00},
		{IMX104_INCKSEL3,     0x00},
		{IMX104_BLKLEVEL_LSB, 0xF0},
		{IMX104_BLKLEVEL_MSB, 0x00},
	},
	{
		{IMX104_ADBIT,        0x00},
		{IMX104_MODE,         0x00},
		{IMX104_WINMODE,      0x10},
		{IMX104_FRSEL,        0x00},
		{IMX104_VMAX_LSB,     0xEE},
		{IMX104_VMAX_MSB,     0x02},
		{IMX104_VMAX_HSB,     0x00},
		{IMX104_HMAX_LSB,     0x72},
		{IMX104_HMAX_MSB,     0x06},
		{IMX104_OUTCTRL,      0xE0},
		{IMX104_INCKSEL1,     0x00},
		{IMX104_INCKSEL2,     0x00},
		{IMX104_INCKSEL3,     0x00},
		{IMX104_BLKLEVEL_LSB, 0x3C},
		{IMX104_BLKLEVEL_MSB, 0x00},
	}
};

static struct vin_video_format imx104_formats[] = {
	{
		.video_mode	= AMBA_VIDEO_MODE_SXGA,
		.def_start_x	= 4+4+8,
		.def_start_y	= 1+4+16+4+8,
		.def_width	= 1280,
		.def_height	= 1024,
		/* sensor mode */
		.device_mode	= 0,
		.pll_idx	= 0,
		.width		= 1280,
		.height		= 1024,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_60,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
		.readout_mode = 0,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_720P,
		.def_start_x	= 4+4+8,
		.def_start_y	= 1+4+4+2+4,
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
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
		.readout_mode = 0,
	},
};

static struct vin_reg_16_8 imx104_share_regs[] = {
	/* Chip ID=02h */
	{0x0217, 0x01},
	{0x021D, 0xFF},
	{0x021E, 0x01},
	{0x0254, 0x63},
	{0x02A1, 0x45},
	{0x02BF, 0x1F},

	/* Chip ID=03h, do not change */
	{0x0312, 0x00},
	{0x031D, 0x07},
	{0x0323, 0x07},
	{0x0326, 0xDF},
	{0x0347, 0x87},

	/* Chip ID=04h, do not change */
	{0x0403, 0xCD},
	{0x0407, 0x4B},
	{0x0409, 0xE9},
	{0x0413, 0x1B},
	{0x0415, 0xED},
	{0x0416, 0x01},
	{0x0418, 0x09},
	{0x041A, 0x19},
	{0x041B, 0xA1},
	{0x041C, 0x11},
	{0x0427, 0x00},
	{0x0428, 0x05},
	{0x0429, 0xEC},
	{0x042A, 0x40},
	{0x042B, 0x11},
	{0x042D, 0x22},
	{0x042E, 0x00},
	{0x042F, 0x05},
	{0x0431, 0xEC},
	{0x0432, 0x40},
	{0x0433, 0x11},
	{0x0435, 0x23},
	{0x0436, 0xB0},
	{0x0437, 0x04},
	{0x0439, 0x24},
	{0x043A, 0x30},
	{0x043B, 0x04},
	{0x043C, 0xED},
	{0x043D, 0xC0},
	{0x043E, 0x10},
	{0x0440, 0x44},
	{0x0441, 0xA0},
	{0x0442, 0x04},
	{0x0443, 0x0D},
	{0x0444, 0x31},
	{0x0445, 0x11},
	{0x0447, 0xEC},
	{0x0448, 0xD0},
	{0x0449, 0x1D},
	{0x0455, 0x03},
	{0x0456, 0x54},
	{0x0457, 0x60},
	{0x0458, 0x1F},
	{0x045A, 0xA9},
	{0x045B, 0x50},
	{0x045C, 0x0A},
	{0x045D, 0x25},
	{0x045E, 0x11},
	{0x045F, 0x12},
	{0x0461, 0x9B},
	{0x0466, 0xD0},
	{0x0467, 0x08},
	{0x046A, 0x20},
	{0x046B, 0x0A},
	{0x046E, 0x20},
	{0x046F, 0x0A},
	{0x0472, 0x20},
	{0x0473, 0x0A},
	{0x0475, 0xEC},
	{0x047D, 0xA5},
	{0x047E, 0x20},
	{0x047F, 0x0A},
	{0x0481, 0xEF},
	{0x0482, 0xC0},
	{0x0483, 0x0E},
	{0x0485, 0xF6},
	{0x048A, 0x60},
	{0x048B, 0x1F},
	{0x048D, 0xBB},
	{0x048E, 0x90},
	{0x048F, 0x0D},
	{0x0490, 0x39},
	{0x0491, 0xC1},
	{0x0492, 0x1D},
	{0x0494, 0xC9},
	{0x0495, 0x70},
	{0x0496, 0x0E},
	{0x0497, 0x47},
	{0x0498, 0xA1},
	{0x0499, 0x1E},
	{0x049B, 0xC5},
	{0x049C, 0xB0},
	{0x049D, 0x0E},
	{0x049E, 0x43},
	{0x049F, 0xE1},
	{0x04A0, 0x1E},
	{0x04A2, 0xBB},
	{0x04A3, 0x10},
	{0x04A4, 0x0C},
	{0x04A6, 0xB3},
	{0x04A7, 0x30},
	{0x04A8, 0x0A},
	{0x04A9, 0x29},
	{0x04AA, 0x91},
	{0x04AB, 0x11},
	{0x04AD, 0xB4},
	{0x04AE, 0x40},
	{0x04AF, 0x0A},
	{0x04B0, 0x2A},
	{0x04B1, 0xA1},
	{0x04B2, 0x11},
	{0x04B4, 0xAB},
	{0x04B5, 0xB0},
	{0x04B6, 0x0B},
	{0x04B7, 0x21},
	{0x04B8, 0x11},
	{0x04B9, 0x13},
	{0x04BB, 0xAC},
	{0x04BC, 0xC0},
	{0x04BD, 0x0B},
	{0x04BE, 0x22},
	{0x04BF, 0x21},
	{0x04C0, 0x13},
	{0x04C2, 0xAD},
	{0x04C3, 0x10},
	{0x04C4, 0x0B},
	{0x04C5, 0x23},
	{0x04C6, 0x71},
	{0x04C7, 0x12},
	{0x04C9, 0xB5},
	{0x04CA, 0x90},
	{0x04CB, 0x0B},
	{0x04CC, 0x2B},
	{0x04CD, 0xF1},
	{0x04CE, 0x12},
	{0x04D0, 0xBB},
	{0x04D1, 0x10},
	{0x04D2, 0x0C},
	{0x04D4, 0xE7},
	{0x04D5, 0x90},
	{0x04D6, 0x0E},
	{0x04D8, 0x45},
	{0x04D9, 0x11},
	{0x04DA, 0x1F},
	{0x04EB, 0xA4},
	{0x04EC, 0x60},
	{0x04ED, 0x1F},

	{0x020A, 0xF0},/* Black level */
	{0x020B, 0x00},

	{0x0211, 0x00},
};

#define IMX104_GAIN_ROWS  141
#define IMX104_GAIN_42DB  0x8C

