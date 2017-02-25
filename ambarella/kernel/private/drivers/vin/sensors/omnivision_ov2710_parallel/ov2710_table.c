/*
 * Filename : ov2710_reg_tbl.c
 *
 * History:
 *    2009/06/19 - [Qiao Wang] Create
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

static struct vin_video_pll ov2710_plls[] = {
	{0, 23998464, 79994880},
	{0, 24045120, 80150400},
};

static struct vin_video_format ov2710_formats[] = {
	{
		.video_mode	= AMBA_VIDEO_MODE_720P,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 1280,
		.def_height	= 720,
		/* sensor mode */
		.device_mode	= 0,
		.pll_idx	= 0,
		.width		= 1280,
		.height		= 720,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_60,
		.default_fps	= AMBA_VIDEO_FPS_59_94,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_GB,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_1080P,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 1920,
		.def_height	= 1080,
		/* sensor mode */
		.device_mode	= 1,
		.pll_idx	= 1,
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
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_GB,
	},
};

static struct vin_reg_16_8 ov2710_1080p_share_regs[] = {
	//R2.2 Updated System Settings (v05)
	{0x3103, 0x93},
	{0x3008, 0x82},
	{0x3017, 0x7f},
	{0x3018, 0xfc},
	{0x3706, 0x61},
	{0x3712, 0x0c},
	{0x3630, 0x6d},
	{0x3801, 0xb4},
	{0x3621, 0x04},
	{0x3604, 0x60},
	{0x3603, 0xa7},
	{0x3631, 0x26},
	{0x3600, 0x04},
	{0x3620, 0x37},
	{0x3623, 0x00},
	{0x3702, 0x9e},
	{0x3703, 0x5c},
	{0x3704, 0x40},
	{0x370d, 0x0f},
	{0x3713, 0x9f},
	{0x3714, 0x4c},
	{0x3710, 0x9e},
	{0x3801, 0xc4},
	{0x3605, 0x05},
	{0x3606, 0x3f},
	{0x302d, 0x90},
	{0x370b, 0x40},
	{0x3716, 0x31},
	{0x3707, 0x52},
	{0x380d, 0x74},
	{0x5181, 0x20},
	{0x518f, 0x00},
	{0x4301, 0xff},
	{0x4303, 0x00},
	{0x3a00, 0x78},
	{0x300f, 0x88},
	{0x3011, 0x28},
	{0x3a1a, 0x06},
	{0x3a18, 0x00},
	{0x3a19, 0x7a},
	{0x3a13, 0x54},
	{0x382e, 0x0f},
	{0x381a, 0x1a},
	{0x401d, 0x02},
	{0x5688, 0x03},
	{0x5684, 0x07},
	{0x5685, 0xa0},
	{0x5686, 0x04},
	{0x5687, 0x43},
	{0x3a0f, 0x40},
	{0x3a10, 0x38},
	{0x3a1b, 0x48},
	{0x3a1e, 0x30},
	{0x3a11, 0x90},
	{0x3a1f, 0x10},

	{0x3010, 0x00},
	//1928x1088
	{0x3820, 0x00},
	{0x3821, 0x00},
	{0x381c, 0x00},
	{0x381d, 0x02},
	{0x381e, 0x04},
	{0x381f, 0x44},
	{0x3800, 0x01},
	{0x3801, 0xC4},
	{0x3804, 0x07},
	{0x3805, 0x88},
	{0x3802, 0x00},
	{0x3803, 0x0A},
	{0x3806, 0x04},
	{0x3807, 0x40},
	{0x3808, 0x07},
	{0x3809, 0x88},
	{0x380a, 0x04},
	{0x380b, 0x40},
	{0x380c, 0x09},
	{0x380d, 0x74},
	{0x380e, 0x04},
	{0x380f, 0x50},
	{0x3810, 0x08},
	{0x3811, 0x02},

	{0x370d, 0x0f},//Fix Black Shading issue from 0x7->0xF
	{0x3621, 0x04},
	{0x3622, 0x08},
	{0x3818, 0x80},
	{0x370d, 0x0f},//Fix Black Shading issue from 0x7->0xF

	//AVG windows
	{0x5688, 0x03},
	{0x5684, 0x07},
	{0x5685, 0x88},
	{0x5686, 0x04},
	{0x5687, 0x40},

	{0x3a08, 0x14},
	{0x3a09, 0xB3},
	{0x3a0a, 0x11},
	{0x3a0b, 0x40},
	{0x3a0e, 0x03},
	{0x3a0d, 0x04},

	{0x401c, 0x08},

	//off 3A
	{0x3503, 0x07},//off aec/agc
	{0x5001, 0x4e},//off awb
	{0x5000, 0x5f},//off lenc

	//mirror and flip here,for our new board */
	{ 0x3621, 0x14},
	{ 0x3803, 0x09},
	{ 0x3818, 0xE0},

	{ 0x4000, 0x05},//BLC control
	{ 0x4006, 0x00},//black level target [9:8]
	{ 0x4007, 0x00},//black level target [7:0]

	//{ 0x4704, 0x02},//VSYNC2 mode enable
};

static struct vin_reg_16_8 ov2710_720p_share_regs[] = {
	//R2.2 Updated System Settings (v05)
	{0x3103, 0x93},
	{0x3008, 0x82},
	{0x3017, 0x7f},
	{0x3018, 0xfc},
	{0x3706, 0x61},
	{0x3712, 0x0c},
	{0x3630, 0x6d},
	{0x3801, 0xb4},
	{0x3621, 0x04},
	{0x3604, 0x60},
	{0x3603, 0xa7},
	{0x3631, 0x26},
	{0x3600, 0x04},
	{0x3620, 0x37},
	{0x3623, 0x00},
	{0x3702, 0x9e},
	{0x3703, 0x5c},
	{0x3704, 0x40},
	{0x370d, 0x0f},
	{0x3713, 0x9f},
	{0x3714, 0x4c},
	{0x3710, 0x9e},
	{0x3801, 0xc4},
	{0x3605, 0x05},
	{0x3606, 0x3f},
	{0x302d, 0x90},
	{0x370b, 0x40},
	{0x3716, 0x31},
	{0x3707, 0x52},
	{0x380d, 0x74},
	{0x5181, 0x20},
	{0x518f, 0x00},
	{0x4301, 0xff},
	{0x4303, 0x00},
	{0x3a00, 0x78},
	{0x300f, 0x88},
	{0x3011, 0x28},
	{0x3a1a, 0x06},
	{0x3a18, 0x00},
	{0x3a19, 0x7a},
	{0x3a13, 0x54},
	{0x382e, 0x0f},
	{0x381a, 0x1a},
	{0x401d, 0x02},
	{0x381c, 0x10},
	{0x381d, 0xb8},
	{0x381e, 0x02},
	{0x381f, 0xdc},
	{0x3820, 0x0a},
	{0x3821, 0x29},
	{0x3804, 0x05},
	{0x3805, 0x00},
	{0x3806, 0x02},
	{0x3807, 0xd0},
	{0x3808, 0x05},
	{0x3809, 0x00},
	{0x380a, 0x02},
	{0x380b, 0xd0},
	{0x380e, 0x02},
	{0x380f, 0xe8},
	{0x380c, 0x07},
	{0x380d, 0x00},
	{0x5688, 0x03},
	{0x5684, 0x05},
	{0x5685, 0x00},
	{0x5686, 0x02},
	{0x5687, 0xd0},
	{0x3a08, 0x1b},
	{0x3a09, 0xe6},
	{0x3a0a, 0x17},
	{0x3a0b, 0x40},
	{0x3a0e, 0x01},
	{0x3a0d, 0x02},
	{0x3a0f, 0x40},
	{0x3a10, 0x38},
	{0x3a1b, 0x48},
	{0x3a1e, 0x30},
	{0x3a11, 0x90},
	{0x3a1f, 0x10},

	{0x3010, 0x00},

	//1288x728
	{0x3820, 0x09},

	{0x3821, 0x29},
	{0x381c, 0x10},
	{0x381d, 0xBE},
	{0x381e, 0x02},
	{0x381f, 0xDC},
	{0x3800, 0x01},
	{0x3801, 0xC8},
	{0x3804, 0x05},
	{0x3805, 0x08},
	{0x3802, 0x00},
	{0x3803, 0x0A},
	{0x3806, 0x02},
	{0x3807, 0xD8},
	{0x3808, 0x05},
	{0x3809, 0x08},
	{0x380a, 0x02},
	{0x380b, 0xD8},
	{0x380c, 0x07},
	{0x380d, 0x00},
	{0x380e, 0x02},
	{0x380f, 0xE8},
	{0x3810, 0x08},
	{0x3811, 0x02},

	{0x3503, 0x07},//off aec/agc
	{0x5001, 0x4e},//off awb
	{0x5000, 0x5f},//off lenc

	//mirror and flip here,for our new board */
	{ 0x3621, 0x14},
	{ 0x3803, 0x09},
	{ 0x3818, 0xE0},

	{ 0x4000, 0x05},//BLC control
	{ 0x4006, 0x00},//black level target [9:8]
	{ 0x4007, 0x00},//black level target [7:0]

	//{ 0x4704, 0x02},//VSYNC2 mode enable
};

/* OV2710 global gain table row size */
#define OV2710_GAIN_ROWS		(97)
#define OV2710_GAIN_COLS		(4)

#define OV2710_GAIN_COL_AGC		(0)
#define OV2710_GAIN_COL_FAGC		(1)
#define OV2710_GAIN_COL_REG300A		(2)
#define OV2710_GAIN_COL_REG300B		(3)
#define OV2710_GAIN_0DB (OV2710_GAIN_ROWS - 1)

/* This is 32-step gain table, OV2710_GAIN_ROWS = 162, OV2710_GAIN_COLS = 3 */
const s16 ov2710_gains[OV2710_GAIN_ROWS][OV2710_GAIN_COLS] = {
	{0x4000, 6144, 0x01, 0xFF},	/* index   0, gain = 36.123599 dB, actual gain = 35.847834 dB */
	{0x3D49, 6080, 0x01, 0xFE},	/* index   1, gain = 35.747312 dB, actual gain = 35.847834 dB */
	{0x3AB0, 6016, 0x01, 0xFD},	/* index   2, gain = 35.371024 dB, actual gain = 35.268560 dB */
	{0x3833, 5952, 0x01, 0xFC},	/* index   3, gain = 34.994737 dB, actual gain = 34.963761 dB */
	{0x35D1, 5888, 0x01, 0xFB},	/* index   4, gain = 34.618450 dB, actual gain = 34.647875 dB */
	{0x3389, 5824, 0x01, 0xFA},	/* index   5, gain = 34.242162 dB, actual gain = 34.320067 dB */
	{0x315A, 5760, 0x01, 0xF9},	/* index   6, gain = 33.865875 dB, actual gain = 33.979400 dB */
	{0x2F42, 5696, 0x01, 0xF8},	/* index   7, gain = 33.489587 dB, actual gain = 33.624825 dB */
	{0x2D41, 5632, 0x01, 0xF7},	/* index   8, gain = 33.113300 dB, actual gain = 33.255157 dB */
	{0x2B56, 5568, 0x01, 0xF6},	/* index   9, gain = 32.737012 dB, actual gain = 32.869054 dB */
	{0x2980, 5504, 0x01, 0xF5},	/* index  10, gain = 32.360725 dB, actual gain = 32.464986 dB */
	{0x27BD, 5440, 0x01, 0xF4},	/* index  11, gain = 31.984437 dB, actual gain = 32.041200 dB */
	{0x260E, 5376, 0x01, 0xF3},	/* index  12, gain = 31.608150 dB, actual gain = 31.595672 dB */
	{0x2471, 5312, 0x01, 0xF2},	/* index  13, gain = 31.231862 dB, actual gain = 31.126050 dB */
	{0x22E5, 5248, 0x01, 0xF1},	/* index  14, gain = 30.855575 dB, actual gain = 30.629578 dB */
	{0x216B, 5184, 0x01, 0xF1},	/* index  15, gain = 30.479287 dB, actual gain = 30.629578 dB */
	{0x2000, 5120, 0x00, 0xFF},	/* index  16, gain = 30.103000 dB, actual gain = 29.827234 dB */
	{0x1EA5, 5056, 0x00, 0xFE},	/* index  17, gain = 29.726712 dB, actual gain = 29.827234 dB */
	{0x1D58, 4992, 0x00, 0xFD},	/* index  18, gain = 29.350425 dB, actual gain = 29.247960 dB */
	{0x1C1A, 4928, 0x00, 0xFC},	/* index  19, gain = 28.974137 dB, actual gain = 28.943161 dB */
	{0x1AE9, 4864, 0x00, 0xFB},	/* index  20, gain = 28.597850 dB, actual gain = 28.627275 dB */
	{0x19C5, 4800, 0x00, 0xFA},	/* index  21, gain = 28.221562 dB, actual gain = 28.299467 dB */
	{0x18AD, 4736, 0x00, 0xF9},	/* index  22, gain = 27.845275 dB, actual gain = 27.958800 dB */
	{0x17A1, 4672, 0x00, 0xF8},	/* index  23, gain = 27.468987 dB, actual gain = 27.604225 dB */
	{0x16A1, 4608, 0x00, 0xF7},	/* index  24, gain = 27.092700 dB, actual gain = 27.234557 dB */
	{0x15AB, 4544, 0x00, 0xF6},	/* index  25, gain = 26.716412 dB, actual gain = 26.848454 dB */
	{0x14C0, 4480, 0x00, 0xF5},	/* index  26, gain = 26.340125 dB, actual gain = 26.444386 dB */
	{0x13DF, 4416, 0x00, 0xF4},	/* index  27, gain = 25.963837 dB, actual gain = 26.020600 dB */
	{0x1307, 4352, 0x00, 0xF3},	/* index  28, gain = 25.587550 dB, actual gain = 25.575072 dB */
	{0x1238, 4288, 0x00, 0xF2},	/* index  29, gain = 25.211262 dB, actual gain = 25.105450 dB */
	{0x1173, 4224, 0x00, 0xF1},	/* index  30, gain = 24.834975 dB, actual gain = 24.608978 dB */
	{0x10B5, 4160, 0x00, 0xF1},	/* index  31, gain = 24.458687 dB, actual gain = 24.608978 dB */
	{0x1000, 4096, 0x00, 0x7F},	/* index  32, gain = 24.082400 dB, actual gain = 23.806634 dB */
	{0x0F52, 4032, 0x00, 0x7E},	/* index  33, gain = 23.706112 dB, actual gain = 23.806634 dB */
	{0x0EAC, 3968, 0x00, 0x7D},	/* index  34, gain = 23.329825 dB, actual gain = 23.227360 dB */
	{0x0E0D, 3904, 0x00, 0x7C},	/* index  35, gain = 22.953537 dB, actual gain = 22.922561 dB */
	{0x0D74, 3840, 0x00, 0x7B},	/* index  36, gain = 22.577250 dB, actual gain = 22.606675 dB */
	{0x0CE2, 3776, 0x00, 0x7A},	/* index  37, gain = 22.200962 dB, actual gain = 22.278867 dB */
	{0x0C56, 3712, 0x00, 0x79},	/* index  38, gain = 21.824675 dB, actual gain = 21.938200 dB */
	{0x0BD1, 3648, 0x00, 0x78},	/* index  39, gain = 21.448387 dB, actual gain = 21.583625 dB */
	{0x0B50, 3584, 0x00, 0x77},	/* index  40, gain = 21.072100 dB, actual gain = 21.213957 dB */
	{0x0AD6, 3520, 0x00, 0x76},	/* index  41, gain = 20.695812 dB, actual gain = 20.827854 dB */
	{0x0A60, 3456, 0x00, 0x75},	/* index  42, gain = 20.319525 dB, actual gain = 20.423786 dB */
	{0x09EF, 3392, 0x00, 0x74},	/* index  43, gain = 19.943237 dB, actual gain = 20.000000 dB */
	{0x0983, 3328, 0x00, 0x73},	/* index  44, gain = 19.566950 dB, actual gain = 19.554472 dB */
	{0x091C, 3264, 0x00, 0x72},	/* index  45, gain = 19.190662 dB, actual gain = 19.084850 dB */
	{0x08B9, 3200, 0x00, 0x71},	/* index  46, gain = 18.814375 dB, actual gain = 18.588379 dB */
	{0x085B, 3136, 0x00, 0x71},	/* index  47, gain = 18.438087 dB, actual gain = 18.588379 dB */
	{0x0800, 3072, 0x00, 0x3F},	/* index  48, gain = 18.061800 dB, actual gain = 17.786034 dB */
	{0x07A9, 3008, 0x00, 0x3E},	/* index  49, gain = 17.685512 dB, actual gain = 17.786034 dB */
	{0x0756, 2944, 0x00, 0x3D},	/* index  50, gain = 17.309225 dB, actual gain = 17.206760 dB */
	{0x0706, 2880, 0x00, 0x3C},	/* index  51, gain = 16.932937 dB, actual gain = 16.901961 dB */
	{0x06BA, 2816, 0x00, 0x3B},	/* index  52, gain = 16.556650 dB, actual gain = 16.586075 dB */
	{0x0671, 2752, 0x00, 0x3A},	/* index  53, gain = 16.180362 dB, actual gain = 16.258267 dB */
	{0x062B, 2688, 0x00, 0x39},	/* index  54, gain = 15.804075 dB, actual gain = 15.917600 dB */
	{0x05E8, 2624, 0x00, 0x38},	/* index  55, gain = 15.427787 dB, actual gain = 15.563025 dB */
	{0x05A8, 2560, 0x00, 0x37},	/* index  56, gain = 15.051500 dB, actual gain = 15.193357 dB */
	{0x056B, 2496, 0x00, 0x36},	/* index  57, gain = 14.675212 dB, actual gain = 14.807254 dB */
	{0x0530, 2432, 0x00, 0x35},	/* index  58, gain = 14.298925 dB, actual gain = 14.403186 dB */
	{0x04F8, 2368, 0x00, 0x34},	/* index  59, gain = 13.922637 dB, actual gain = 13.979400 dB */
	{0x04C2, 2304, 0x00, 0x33},	/* index  60, gain = 13.546350 dB, actual gain = 13.533872 dB */
	{0x048E, 2240, 0x00, 0x32},	/* index  61, gain = 13.170062 dB, actual gain = 13.064250 dB */
	{0x045D, 2176, 0x00, 0x31},	/* index  62, gain = 12.793775 dB, actual gain = 12.567779 dB */
	{0x042D, 2112, 0x00, 0x31},	/* index  63, gain = 12.417487 dB, actual gain = 12.567779 dB */
	{0x0400, 2048, 0x00, 0x1F},	/* index  64, gain = 12.041200 dB, actual gain = 11.765434 dB */
	{0x03D5, 1984, 0x00, 0x1E},	/* index  65, gain = 11.664912 dB, actual gain = 11.765434 dB */
	{0x03AB, 1920, 0x00, 0x1D},	/* index  66, gain = 11.288625 dB, actual gain = 11.186160 dB */
	{0x0383, 1856, 0x00, 0x1C},	/* index  67, gain = 10.912337 dB, actual gain = 10.881361 dB */
	{0x035D, 1792, 0x00, 0x1B},	/* index  68, gain = 10.536050 dB, actual gain = 10.565476 dB */
	{0x0339, 1728, 0x00, 0x1A},	/* index  69, gain = 10.159762 dB, actual gain = 10.237667 dB */
	{0x0316, 1664, 0x00, 0x19},	/* index  70, gain = 9.783475 dB, actual gain = 9.897000 dB */
	{0x02F4, 1600, 0x00, 0x18},	/* index  71, gain = 9.407187 dB, actual gain = 9.542425 dB */
	{0x02D4, 1536, 0x00, 0x17},	/* index  72, gain = 9.030900 dB, actual gain = 9.172757 dB */
	{0x02B5, 1472, 0x00, 0x16},	/* index  73, gain = 8.654612 dB, actual gain = 8.786654 dB */
	{0x0298, 1408, 0x00, 0x15},	/* index  74, gain = 8.278325 dB, actual gain = 8.382586 dB */
	{0x027C, 1344, 0x00, 0x14},	/* index  75, gain = 7.902037 dB, actual gain = 7.958800 dB */
	{0x0261, 1280, 0x00, 0x13},	/* index  76, gain = 7.525750 dB, actual gain = 7.513272 dB */
	{0x0247, 1216, 0x00, 0x12},	/* index  77, gain = 7.149462 dB, actual gain = 7.043650 dB */
	{0x022E, 1152, 0x00, 0x11},	/* index  78, gain = 6.773175 dB, actual gain = 6.547179 dB */
	{0x0217, 1088, 0x00, 0x11},	/* index  79, gain = 6.396887 dB, actual gain = 6.547179 dB */
	{0x0200, 1024, 0x00, 0x0F},	/* index  80, gain = 6.020600 dB, actual gain = 5.744834 dB */
	{0x01EA,  960, 0x00, 0x0E},	/* index  81, gain = 5.644312 dB, actual gain = 5.744834 dB */
	{0x01D6,  896, 0x00, 0x0D},	/* index  82, gain = 5.268025 dB, actual gain = 5.165560 dB */
	{0x01C2,  832, 0x00, 0x0C},	/* index  83, gain = 4.891737 dB, actual gain = 4.860761 dB */
	{0x01AF,  768, 0x00, 0x0B},	/* index  84, gain = 4.515450 dB, actual gain = 4.544876 dB */
	{0x019C,  704, 0x00, 0x0A},	/* index  85, gain = 4.139162 dB, actual gain = 4.217067 dB */
	{0x018B,  640, 0x00, 0x09},	/* index  86, gain = 3.762875 dB, actual gain = 3.876401 dB */
	{0x017A,  576, 0x00, 0x08},	/* index  87, gain = 3.386587 dB, actual gain = 3.521825 dB */
	{0x016A,  512, 0x00, 0x07},	/* index  88, gain = 3.010300 dB, actual gain = 3.152157 dB */
	{0x015B,  448, 0x00, 0x06},	/* index  89, gain = 2.634012 dB, actual gain = 2.766054 dB */
	{0x014C,  384, 0x00, 0x05},	/* index  90, gain = 2.257725 dB, actual gain = 2.361986 dB */
	{0x013E,  320, 0x00, 0x04},	/* index  91, gain = 1.881437 dB, actual gain = 1.938200 dB */
	{0x0130,  256, 0x00, 0x03},	/* index  92, gain = 1.505150 dB, actual gain = 1.492672 dB */
	{0x0124,  192, 0x00, 0x02},	/* index  93, gain = 1.128862 dB, actual gain = 1.023050 dB */
	{0x0117,  128, 0x00, 0x01},	/* index  94, gain = 0.752575 dB, actual gain = 0.526579 dB */
	{0x010B,   64, 0x00, 0x01},	/* index  95, gain = 0.376287 dB, actual gain = 0.526579 dB */
	{0x0100,    0, 0x00, 0x00},	/* index  96, gain = 0.000000 dB, actual gain = 0.000000 dB */
};

