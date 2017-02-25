/*
 * Filename : ov9718_reg_tbl.c
 *
 * History:
 *    2012/03/23 - [Long Zhao] Create
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

static struct vin_video_pll ov9718_plls[] = {
	{0, 24017940, 80059800},
};

static struct vin_video_format ov9718_formats[] = {
	{
		.video_mode = AMBA_VIDEO_MODE_WXGA,
		.def_start_x = 0,
		.def_start_y = 0,
		.def_width = 1280,
		.def_height = 800,
		/* sensor mode */
		.device_mode = 0,
		.pll_idx = 0,
		.width = 1280,
		.height = 800,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_12,
		.ratio = AMBA_VIDEO_RATIO_16_9,
		.max_fps = AMBA_VIDEO_FPS_60,
		.default_fps = AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern = VINDEV_BAYER_PATTERN_BG,
	},
	{
		.video_mode = AMBA_VIDEO_MODE_720P,
		.def_start_x = 0,
		.def_start_y = 0,
		.def_width = 1280,
		.def_height = 720,
		/* sensor mode */
		.device_mode = 0,
		.pll_idx = 0,
		.width = 1280,
		.height = 800,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_12,
		.ratio = AMBA_VIDEO_RATIO_16_9,
		.max_fps = AMBA_VIDEO_FPS_60,
		.default_fps = AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern = VINDEV_BAYER_PATTERN_BG,
	},
};

static struct vin_reg_16_8 ov9718_share_regs[] = {
	{0x0100, 0x00},
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x0100, 0x00},
	{0x0100, 0x00},
	{0x0100, 0x00},
	{0x0101, 0x00},
	{0x3001, 0x01},
	{0x3712, 0x7a},
	{0x3714, 0x90},
	{0x3729, 0x04},
	{0x3731, 0x14},
	{0x3624, 0x77},
	{0x3621, 0x44},
	{0x3623, 0xcc},
	{0x371e, 0x30},
	{0x3601, 0x44},
	{0x3603, 0x63},
	{0x3635, 0x05},
	{0x3632, 0xa7},
	{0x3634, 0x57},
	{0x370b, 0x01},
	{0x3711, 0x0e},
	{0x0340, 0x03},
	{0x0341, 0x40},
	{0x0342, 0x06},
	{0x0343, 0x3e},
	{0x0344, 0x00},
	{0x0345, 0x00},
	{0x0346, 0x00},
	{0x0347, 0x00},
	{0x0348, 0x05},
	{0x0349, 0x1f},
	{0x034a, 0x03},
	{0x034b, 0x2f},
	{0x034c, 0x05},
	{0x034d, 0x00},
	{0x034e, 0x03},
	{0x034f, 0x22},//{0x034f, 0x20},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x3002, 0x09},
	{0x3012, 0x20},
	{0x3013, 0x12},
	{0x3014, 0x84},
	{0x301f, 0x83},
	{0x3020, 0x02},
	{0x3024, 0x20},
	{0x3090, 0x01},
	{0x3091, 0x05},
	{0x3092, 0x03},
	{0x3093, 0x01},
	{0x3094, 0x01},
	{0x309a, 0x01},
	{0x309b, 0x01},
	{0x309c, 0x01},
	{0x30a3, 0x19},
	{0x30a4, 0x01},
	{0x30a5, 0x03},
	{0x30b3, 0x28},
	{0x30b5, 0x04},
	{0x30b6, 0x02},
	{0x30b7, 0x02},
	{0x30b8, 0x01},
	{0x3103, 0x00},
	{0x3200, 0x00},
	{0x3201, 0x03},
	{0x3202, 0x06},
	{0x3203, 0x08},
	{0x3208, 0x00},
	{0x3209, 0x00},
	{0x320a, 0x00},
	{0x320b, 0x01},
	{0x3500, 0x00},
	{0x3501, 0x20},
	{0x3502, 0x00},
	{0x3503, 0x07},
	{0x3504, 0x00},
	{0x3505, 0x20},
	{0x3506, 0x00},
	{0x3507, 0x00},
	{0x3508, 0x20},
	{0x3509, 0x00},
	{0x350a, 0x00},
	{0x350b, 0x10},
	{0x350c, 0x00},
	{0x350d, 0x10},
	{0x350f, 0x10},
	{0x3628, 0x12},
	{0x3633, 0x14},
	{0x3636, 0x08},/* disable internal LDO */
	{0x3641, 0x83},
	{0x3643, 0x14},
	{0x3660, 0x00},
	{0x3662, 0x10},
	{0x3667, 0x00},
	{0x370d, 0xc0},
	{0x370e, 0x80},
	{0x370a, 0x31},
	{0x3714, 0x80},
	{0x371f, 0xa0},
	{0x3730, 0x86},
	{0x3732, 0x14},
	{0x3811, 0x10},
	{0x3813, 0x08},
	{0x3820, 0x80},
	{0x3821, 0x00},
	{0x382a, 0x00},
	{0x3832, 0x80},
	{0x3902, 0x08},
	{0x3903, 0x00},
	{0x3912, 0x80},
	{0x3913, 0x00},
	{0x3907, 0x80},
	{0x3903, 0x00},
	{0x3b00, 0x00},
	{0x3b02, 0x00},
	{0x3b03, 0x00},
	{0x3b04, 0x00},
	{0x3b40, 0x00},
	{0x3b41, 0x00},
	{0x3b42, 0x00},
	{0x3b43, 0x00},
	{0x3b44, 0x00},
	{0x3b45, 0x00},
	{0x3b46, 0x00},
	{0x3b47, 0x00},
	{0x3b48, 0x00},
	{0x3b49, 0x00},
	{0x3b4a, 0x00},
	{0x3b4b, 0x00},
	{0x3b4c, 0x00},
	{0x3b4d, 0x00},
	{0x3b4e, 0x00},
	{0x3e00, 0x00},
	{0x3e01, 0x00},
	{0x3e02, 0x0f},
	{0x3e03, 0xdb},
	{0x3e06, 0x03},
	{0x3e07, 0x20},
	{0x3f04, 0xd0},
	{0x3f05, 0x00},
	{0x3f00, 0x03},
	{0x3f11, 0x06},
	{0x3f13, 0x20},
	{0x4140, 0x98},
	{0x4141, 0x41},
	{0x4143, 0x03},
	{0x4144, 0x54},
	{0x4240, 0x02},
	{0x4300, 0xff},
	{0x4301, 0x00},
	{0x4307, 0x30},
	{0x4315, 0x00},
	{0x4316, 0x30},
	{0x4500, 0xa2},
	{0x4501, 0x14},
	{0x4502, 0x60},
	{0x4580, 0x00},
	{0x4583, 0x83},
	{0x4826, 0x32},
	{0x4837, 0x10},
	{0x4602, 0xf2},
	{0x4142, 0x24},
	{0x4001, 0x06},
	{0x4002, 0x45},
	{0x4004, 0x04},
	{0x4005, 0x40},
	{0x5000, 0xf1},//[0xff to 0xf1] disable lens shading correction and bad pixel correction to save power --cddiao 2013/10/10
	{0x5001, 0x1f},
	{0x5003, 0x00},
	{0x500a, 0x85},
	{0x5080, 0x00},
	{0x5091, 0xa0},
	{0x5300, 0x05},
	//{0x0100, 0x01},

	{0x3501, 0x33},
	{0x3502, 0xc0},
	{0x3a02, 0x40},
	{0x3a03, 0x30},
	{0x3a04, 0x03},
	{0x3a05, 0x3c},

	{0x4800, 0x24},
#if 0
	//30FPS KEY
	{0x3094, 0x01},
	{0x30b3, 0x14},
	{0x4837, 0x21},
#else
	//60FPS KEY
	{0x3094, 0x02},
	{0x30b3, 0x28},
	{0x4837, 0x10},
#endif
};

/* Gain table */
/* OV9718 global gain table row size */
#define OV9718_GAIN_ROWS		(96 + 1)
#define OV9718_GAIN_0DB		(96)
#define OV9718_GAIN_COLS		(3)

#define OV9718_GAIN_COL_AGC		(0)
#define OV9718_GAIN_COL_REG350A	(1)
#define OV9718_GAIN_COL_REG350B	(2)

/* This is 16-step gain table, OV9718_GAIN_ROWS = 81, OV9718_GAIN_COLS = 3 */
const s16 OV9718_GAIN_TABLE[OV9718_GAIN_ROWS][OV9718_GAIN_COLS] = {
	{0x4000, 0x01, 0xFF},	/* index   0, gain = 36.123599 dB, actual gain = 35.847834 dB */
	{0x3D49, 0x01, 0xFE},	/* index   1, gain = 35.747312 dB, actual gain = 35.847834 dB */
	{0x3AB0, 0x01, 0xFD},	/* index   2, gain = 35.371024 dB, actual gain = 35.268560 dB */
	{0x3833, 0x01, 0xFC},	/* index   3, gain = 34.994737 dB, actual gain = 34.963761 dB */
	{0x35D1, 0x01, 0xFB},	/* index   4, gain = 34.618450 dB, actual gain = 34.647875 dB */
	{0x3389, 0x01, 0xFA},	/* index   5, gain = 34.242162 dB, actual gain = 34.320067 dB */
	{0x315A, 0x01, 0xF9},	/* index   6, gain = 33.865875 dB, actual gain = 33.979400 dB */
	{0x2F42, 0x01, 0xF8},	/* index   7, gain = 33.489587 dB, actual gain = 33.624825 dB */
	{0x2D41, 0x01, 0xF7},	/* index   8, gain = 33.113300 dB, actual gain = 33.255157 dB */
	{0x2B56, 0x01, 0xF6},	/* index   9, gain = 32.737012 dB, actual gain = 32.869054 dB */
	{0x2980, 0x01, 0xF5},	/* index  10, gain = 32.360725 dB, actual gain = 32.464986 dB */
	{0x27BD, 0x01, 0xF4},	/* index  11, gain = 31.984437 dB, actual gain = 32.041200 dB */
	{0x260E, 0x01, 0xF3},	/* index  12, gain = 31.608150 dB, actual gain = 31.595672 dB */
	{0x2471, 0x01, 0xF2},	/* index  13, gain = 31.231862 dB, actual gain = 31.126050 dB */
	{0x22E5, 0x01, 0xF1},	/* index  14, gain = 30.855575 dB, actual gain = 30.629578 dB */
	{0x216B, 0x01, 0xF1},	/* index  15, gain = 30.479287 dB, actual gain = 30.629578 dB */
	{0x2000, 0x00, 0xFF},	/* index  16, gain = 30.103000 dB, actual gain = 29.827234 dB */
	{0x1EA5, 0x00, 0xFE},	/* index  17, gain = 29.726712 dB, actual gain = 29.827234 dB */
	{0x1D58, 0x00, 0xFD},	/* index  18, gain = 29.350425 dB, actual gain = 29.247960 dB */
	{0x1C1A, 0x00, 0xFC},	/* index  19, gain = 28.974137 dB, actual gain = 28.943161 dB */
	{0x1AE9, 0x00, 0xFB},	/* index  20, gain = 28.597850 dB, actual gain = 28.627275 dB */
	{0x19C5, 0x00, 0xFA},	/* index  21, gain = 28.221562 dB, actual gain = 28.299467 dB */
	{0x18AD, 0x00, 0xF9},	/* index  22, gain = 27.845275 dB, actual gain = 27.958800 dB */
	{0x17A1, 0x00, 0xF8},	/* index  23, gain = 27.468987 dB, actual gain = 27.604225 dB */
	{0x16A1, 0x00, 0xF7},	/* index  24, gain = 27.092700 dB, actual gain = 27.234557 dB */
	{0x15AB, 0x00, 0xF6},	/* index  25, gain = 26.716412 dB, actual gain = 26.848454 dB */
	{0x14C0, 0x00, 0xF5},	/* index  26, gain = 26.340125 dB, actual gain = 26.444386 dB */
	{0x13DF, 0x00, 0xF4},	/* index  27, gain = 25.963837 dB, actual gain = 26.020600 dB */
	{0x1307, 0x00, 0xF3},	/* index  28, gain = 25.587550 dB, actual gain = 25.575072 dB */
	{0x1238, 0x00, 0xF2},	/* index  29, gain = 25.211262 dB, actual gain = 25.105450 dB */
	{0x1173, 0x00, 0xF1},	/* index  30, gain = 24.834975 dB, actual gain = 24.608978 dB */
	{0x10B5, 0x00, 0xF1},	/* index  31, gain = 24.458687 dB, actual gain = 24.608978 dB */
	{0x1000, 0x00, 0x7F},	/* index  32, gain = 24.082400 dB, actual gain = 23.806634 dB */
	{0x0F52, 0x00, 0x7E},	/* index  33, gain = 23.706112 dB, actual gain = 23.806634 dB */
	{0x0EAC, 0x00, 0x7D},	/* index  34, gain = 23.329825 dB, actual gain = 23.227360 dB */
	{0x0E0D, 0x00, 0x7C},	/* index  35, gain = 22.953537 dB, actual gain = 22.922561 dB */
	{0x0D74, 0x00, 0x7B},	/* index  36, gain = 22.577250 dB, actual gain = 22.606675 dB */
	{0x0CE2, 0x00, 0x7A},	/* index  37, gain = 22.200962 dB, actual gain = 22.278867 dB */
	{0x0C56, 0x00, 0x79},	/* index  38, gain = 21.824675 dB, actual gain = 21.938200 dB */
	{0x0BD1, 0x00, 0x78},	/* index  39, gain = 21.448387 dB, actual gain = 21.583625 dB */
	{0x0B50, 0x00, 0x77},	/* index  40, gain = 21.072100 dB, actual gain = 21.213957 dB */
	{0x0AD6, 0x00, 0x76},	/* index  41, gain = 20.695812 dB, actual gain = 20.827854 dB */
	{0x0A60, 0x00, 0x75},	/* index  42, gain = 20.319525 dB, actual gain = 20.423786 dB */
	{0x09EF, 0x00, 0x74},	/* index  43, gain = 19.943237 dB, actual gain = 20.000000 dB */
	{0x0983, 0x00, 0x73},	/* index  44, gain = 19.566950 dB, actual gain = 19.554472 dB */
	{0x091C, 0x00, 0x72},	/* index  45, gain = 19.190662 dB, actual gain = 19.084850 dB */
	{0x08B9, 0x00, 0x71},	/* index  46, gain = 18.814375 dB, actual gain = 18.588379 dB */
	{0x085B, 0x00, 0x71},	/* index  47, gain = 18.438087 dB, actual gain = 18.588379 dB */
	{0x0800, 0x00, 0x3F},	/* index  48, gain = 18.061800 dB, actual gain = 17.786034 dB */
	{0x07A9, 0x00, 0x3E},	/* index  49, gain = 17.685512 dB, actual gain = 17.786034 dB */
	{0x0756, 0x00, 0x3D},	/* index  50, gain = 17.309225 dB, actual gain = 17.206760 dB */
	{0x0706, 0x00, 0x3C},	/* index  51, gain = 16.932937 dB, actual gain = 16.901961 dB */
	{0x06BA, 0x00, 0x3B},	/* index  52, gain = 16.556650 dB, actual gain = 16.586075 dB */
	{0x0671, 0x00, 0x3A},	/* index  53, gain = 16.180362 dB, actual gain = 16.258267 dB */
	{0x062B, 0x00, 0x39},	/* index  54, gain = 15.804075 dB, actual gain = 15.917600 dB */
	{0x05E8, 0x00, 0x38},	/* index  55, gain = 15.427787 dB, actual gain = 15.563025 dB */
	{0x05A8, 0x00, 0x37},	/* index  56, gain = 15.051500 dB, actual gain = 15.193357 dB */
	{0x056B, 0x00, 0x36},	/* index  57, gain = 14.675212 dB, actual gain = 14.807254 dB */
	{0x0530, 0x00, 0x35},	/* index  58, gain = 14.298925 dB, actual gain = 14.403186 dB */
	{0x04F8, 0x00, 0x34},	/* index  59, gain = 13.922637 dB, actual gain = 13.979400 dB */
	{0x04C2, 0x00, 0x33},	/* index  60, gain = 13.546350 dB, actual gain = 13.533872 dB */
	{0x048E, 0x00, 0x32},	/* index  61, gain = 13.170062 dB, actual gain = 13.064250 dB */
	{0x045D, 0x00, 0x31},	/* index  62, gain = 12.793775 dB, actual gain = 12.567779 dB */
	{0x042D, 0x00, 0x31},	/* index  63, gain = 12.417487 dB, actual gain = 12.567779 dB */
	{0x0400, 0x00, 0x1F},	/* index  64, gain = 12.041200 dB, actual gain = 11.765434 dB */
	{0x03D5, 0x00, 0x1E},	/* index  65, gain = 11.664912 dB, actual gain = 11.765434 dB */
	{0x03AB, 0x00, 0x1D},	/* index  66, gain = 11.288625 dB, actual gain = 11.186160 dB */
	{0x0383, 0x00, 0x1C},	/* index  67, gain = 10.912337 dB, actual gain = 10.881361 dB */
	{0x035D, 0x00, 0x1B},	/* index  68, gain = 10.536050 dB, actual gain = 10.565476 dB */
	{0x0339, 0x00, 0x1A},	/* index  69, gain = 10.159762 dB, actual gain = 10.237667 dB */
	{0x0316, 0x00, 0x19},	/* index  70, gain = 9.783475 dB, actual gain = 9.897000 dB */
	{0x02F4, 0x00, 0x18},	/* index  71, gain = 9.407187 dB, actual gain = 9.542425 dB */
	{0x02D4, 0x00, 0x17},	/* index  72, gain = 9.030900 dB, actual gain = 9.172757 dB */
	{0x02B5, 0x00, 0x16},	/* index  73, gain = 8.654612 dB, actual gain = 8.786654 dB */
	{0x0298, 0x00, 0x15},	/* index  74, gain = 8.278325 dB, actual gain = 8.382586 dB */
	{0x027C, 0x00, 0x14},	/* index  75, gain = 7.902037 dB, actual gain = 7.958800 dB */
	{0x0261, 0x00, 0x13},	/* index  76, gain = 7.525750 dB, actual gain = 7.513272 dB */
	{0x0247, 0x00, 0x12},	/* index  77, gain = 7.149462 dB, actual gain = 7.043650 dB */
	{0x022E, 0x00, 0x11},	/* index  78, gain = 6.773175 dB, actual gain = 6.547179 dB */
	{0x0217, 0x00, 0x11},	/* index  79, gain = 6.396887 dB, actual gain = 6.547179 dB */
	{0x0200, 0x00, 0x0F},	/* index  80, gain = 6.020600 dB, actual gain = 5.744834 dB */
	{0x01EA, 0x00, 0x0E},	/* index  81, gain = 5.644312 dB, actual gain = 5.744834 dB */
	{0x01D6, 0x00, 0x0D},	/* index  82, gain = 5.268025 dB, actual gain = 5.165560 dB */
	{0x01C2, 0x00, 0x0C},	/* index  83, gain = 4.891737 dB, actual gain = 4.860761 dB */
	{0x01AF, 0x00, 0x0B},	/* index  84, gain = 4.515450 dB, actual gain = 4.544876 dB */
	{0x019C, 0x00, 0x0A},	/* index  85, gain = 4.139162 dB, actual gain = 4.217067 dB */
	{0x018B, 0x00, 0x09},	/* index  86, gain = 3.762875 dB, actual gain = 3.876401 dB */
	{0x017A, 0x00, 0x08},	/* index  87, gain = 3.386587 dB, actual gain = 3.521825 dB */
	{0x016A, 0x00, 0x07},	/* index  88, gain = 3.010300 dB, actual gain = 3.152157 dB */
	{0x015B, 0x00, 0x06},	/* index  89, gain = 2.634012 dB, actual gain = 2.766054 dB */
	{0x014C, 0x00, 0x05},	/* index  90, gain = 2.257725 dB, actual gain = 2.361986 dB */
	{0x013E, 0x00, 0x04},	/* index  91, gain = 1.881437 dB, actual gain = 1.938200 dB */
	{0x0130, 0x00, 0x03},	/* index  92, gain = 1.505150 dB, actual gain = 1.492672 dB */
	{0x0124, 0x00, 0x02},	/* index  93, gain = 1.128862 dB, actual gain = 1.023050 dB */
	{0x0117, 0x00, 0x01},	/* index  94, gain = 0.752575 dB, actual gain = 0.526579 dB */
	{0x010B, 0x00, 0x01},	/* index  95, gain = 0.376287 dB, actual gain = 0.526579 dB */
	{0x0100, 0x00, 0x00},	/* index  96, gain = 0.000000 dB, actual gain = 0.000000 dB */
};
