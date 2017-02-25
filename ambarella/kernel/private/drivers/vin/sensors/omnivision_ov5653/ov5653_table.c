/*
 * Filename : ov5653_reg_tbl.c
 *
 * History:
 *    2014/08/18 - [Hao Zeng] Create
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

static struct vin_video_pll ov5653_plls[] = {
	{0, 24000000, 96000000},
	{0, 24000000, 95992800},
};

static struct vin_reg_16_8 ov5653_mode_regs[][46] = {
	{	/* 1280x720 60fps */
		{0x3613, 0xc4},/* BINNING related */
		{0x3621, 0xaf},/* BINNING related*/
		{0x3703, 0x9a},
		{0x3705, 0xdb},
		{0x370a, 0x81},
		{0x370C, 0x00},
		{0x370D, 0x42},
		{0x3713, 0x92},
		{0x3714, 0x17},
		{0x3800, 0x02},/* HREF horizontal start [11:8] */
		{0x3801, 0x22},/* HREF horizontal start [7:0] */
		{0x3802, 0x00},/* HREF vertical start [11:8] */
		{0x3803, 0x0c},/* HREF vertical start [7:0] */
		{0x3804, 0x05},/* HREF horizontal width [11:8] */
		{0x3805, 0x00},/* HREF horizontal width [7:0] */
		{0x3806, 0x02},/* HREF vertical height [11:8] */
		{0x3807, 0xd8},/* HREF vertical height [7:0] */
		{0x3808, 0x05},/* DVP output horizontal width [11:8] */
		{0x3809, 0x00},/* DVP output horizontal width [7:0] */
		{0x380A, 0x02},/* DVP output vertical height [11:8] */
		{0x380B, 0xd8},/* DVP output vertical height [7:0] */
		{0x380C, 0x08},/* Total horizontal size [11:8] */
		{0x380D, 0x72},/* Total horizontal size [7:0] */
		{0x380E, 0x02},/* Total vertical size [11:8] */
		{0x380F, 0xe4},/* Total vertical size [7:0] */
		{0x3818, 0xc1},/* BINNING related */
		{0x381A, 0x00},
		{0x381C, 0x30},
		{0x381D, 0xfa},
		{0x381E, 0x05},
		{0x381F, 0xd0},
		{0x3820, 0x00},
		{0x3821, 0x20},
		{0x3824, 0x23},
		{0x3825, 0x2a},
		{0x505a, 0x0a},
		{0x505b, 0x2e},
		{0x5002, 0x00},
		{0x5901, 0x00},
		{0x3815, 0x81},
		{0x401c, 0x42},
		{0x3010, 0x10},
		{0x3011, 0x10},
		{0x350C, 0x00},/* dummy lines [11:8] */
		{0x350D, 0x00},/* dummy lines [7:0] */
		{0x3810, 0x00},/* 20110126 note: add special for fpn upgrade function */
	},
	{	/* 1920x1080 30fps */
		{0x3613, 0x44},/* BINNING related */
		{0x3621, 0x2f},/* BINNING related */
		{0x3703, 0xE6},
		{0x3705, 0xda},
		{0x370a, 0x80},
		{0x370C, 0x00},
		{0x370D, 0x04},
		{0x3713, 0x22},
		{0x3714, 0x27},
		{0x3800, 0x02},/* HREF horizontal start [11:8] */
		{0x3801, 0x22},/* HREF horizontal start [7:0] */
		{0x3802, 0x00},/* HREF vertical start [11:8] */
		{0x3803, 0x0c},/* HREF vertical start [7:0] */
		{0x3804, 0x07},/* HREF horizontal width [11:8] */
		{0x3805, 0x80},/* HREF horizontal width [7:0] */
		{0x3806, 0x04},/* HREF vertical height [11:8] */
		{0x3807, 0x40},/* HREF vertical height [7:0] */
		{0x3808, 0x07},/* DVP output horizontal width [11:8] */
		{0x3809, 0x80},/* DVP output horizontal width [7:0] */
		{0x380A, 0x04},/* DVP output vertical height [11:8] */
		{0x380B, 0x40},/* DVP output vertical height [7:0] */
		{0x380C, 0x0B},/* Total horizontal size [11:8] */
		{0x380D, 0x52},/* Total horizontal size [7:0] */
		{0x380E, 0x04},/* Total vertical size [11:8] */
		{0x380F, 0x50},/* Total vertical size [7:0] */
		{0x3818, 0xc0},/* BINNING related */
		{0x381A, 0x1c},
		{0x381C, 0x31},
		{0x381D, 0xbe},
		{0x381E, 0x04},
		{0x381F, 0x50},
		{0x3820, 0x05},
		{0x3821, 0x18},
		{0x3824, 0x22},
		{0x3825, 0x94},
		{0x505a, 0x0a},
		{0x505b, 0x2e},
		{0x5002, 0x00},
		{0x5901, 0x00},
		{0x3815, 0x82},
		{0x401c, 0x46},
		{0x3010, 0x10},
		{0x3011, 0x10},
		{0x350C, 0x00},/* dummy lines [11:8] */
		{0x350D, 0x00},/* dummy lines [7:0] */
		{0x3810, 0x00},/* 20110126 note: add special for fpn upgrade function */
	},
	{	/* 1600x1200 30fps */
		{0x3613, 0x44},/* BINNING related */
		{0x3621, 0x2f},/* BINNING related */
		{0x3703, 0xE6},
		{0x3705, 0xda},
		{0x370a, 0x80},
		{0x370C, 0x00},
		{0x370D, 0x04},
		{0x3713, 0x22},
		{0x3714, 0x27},
		{0x3800, 0x02},/* HREF horizontal start [11:8] */
		{0x3801, 0x54},/* HREF horizontal start [7:0] */
		{0x3802, 0x00},/* HREF vertical start [11:8] */
		{0x3803, 0x0e},/* HREF vertical start [7:0] */
		{0x3804, 0x06},/* HREF horizontal width [11:8] */
		{0x3805, 0x40},/* HREF horizontal width [7:0] */
		{0x3806, 0x04},/* HREF vertical height [11:8] */
		{0x3807, 0xB0},/* HREF vertical height [7:0] */
		{0x3808, 0x06},/* DVP output horizontal width [11:8] */
		{0x3809, 0x40},/* DVP output horizontal width [7:0] */
		{0x380A, 0x04},/* DVP output vertical height [11:8] */
		{0x380B, 0xB0},/* DVP output vertical height [7:0] */
		{0x380C, 0x0A},/* Total horizontal size [11:8] */
		{0x380D, 0x47},/* Total horizontal size [7:0] */
		{0x380E, 0x04},/* Total vertical size [11:8] */
		{0x380F, 0xC0},/* Total vertical size [7:0] */
		{0x3818, 0xC0},/* BINNING related */
		{0x381A, 0x1C},
		{0x381C, 0x31},
		{0x381D, 0x88},
		{0x381E, 0x04},
		{0x381F, 0xD0},
		{0x3820, 0x07},
		{0x3821, 0x14},
		{0x3824, 0x22},
		{0x3825, 0x94},
		{0x505a, 0x0a},
		{0x505b, 0x2e},
		{0x5002, 0x00},
		{0x5901, 0x00},
		{0x3815, 0x82},
		{0x401c, 0x46},
		{0x3010, 0x10},
		{0x3011, 0x10},
		{0x350C, 0x00},/* dummy lines [11:8] */
		{0x350D, 0x00},/* dummy lines [7:0] */
		{0x3810, 0x40},/* 20110126 note: add special for fpn upgrade function */
	},
	{	/* 2048x1536 20fps */
		{0X3613, 0x44},/* BINNING related */
		{0X3621, 0x2f},/* BINNING related */
		{0X3703, 0xE6},
		{0X3705, 0xda},
		{0X370a, 0x80},
		{0X370C, 0x00},
		{0X370D, 0x04},
		{0X3713, 0x22},
		{0X3714, 0x27},
		{0X3800, 0x02},/* HREF horizontal start [11:8] */
		{0X3801, 0x22},/* HREF horizontal start [7:0] */
		{0X3802, 0x00},/* HREF vertical start [11:8] */
		{0X3803, 0x0C},/* HREF vertical start [7:0] */
		{0X3804, 0x08},/* HREF horizontal width [11:8] */
		{0X3805, 0x00},/* HREF horizontal width [7:0] */
		{0X3806, 0x06},/* HREF vertical height [11:8] */
		{0X3807, 0x00},/* HREF vertical height [7:0] */
		{0X3808, 0x08},/* DVP output horizontal width [11:8] */
		{0X3809, 0x00},/* DVP output horizontal width [7:0] */
		{0X380A, 0x06},/* DVP output vertical height [11:8] */
		{0X380B, 0x00},/* DVP output vertical height [7:0] */
		{0X380C, 0x0C},/* Total horizontal size [11:8] */
		{0X380D, 0x14},/* Total horizontal size [7:0] */
		{0X380E, 0x06},/* Total vertical size [11:8] */
		{0X380F, 0x10},/* Total vertical size [7:0] */
		{0X3818, 0xc0},/* BINNING related */
		{0X381A, 0x1c},
		{0X381C, 0x30},
		{0X381D, 0xDE},
		{0X381E, 0x06},
		{0X381F, 0x10},
		{0X3820, 0x04},
		{0X3821, 0x1A},
		{0X3824, 0x22},
		{0X3825, 0xA4},
		{0X505a, 0x0a},
		{0X505b, 0x2e},
		{0X5002, 0x00},
		{0X5901, 0x00},
		{0X3815, 0x82},
		{0X401c, 0x46},
		{0X3010, 0x10},
		{0X3011, 0x10},
		{0X350C, 0x00},/* dummy lines [11:8] */
		{0X350D, 0x00},/* dummy lines [7:0] */
		{0X3810, 0x00},/* 20110126 note: add special for fpn upgrade function */
	},
	{	/* 2560x1440 20fps */
		{0x3613, 0x44},/* BINNING related */
		{0x3621, 0x2f},/* BINNING related */
		{0x3703, 0xE6},
		{0x3705, 0xda},
		{0x370a, 0x80},
		{0x370C, 0x00},
		{0x370D, 0x04},
		{0x3713, 0x22},
		{0x3714, 0x27},
		{0x3800, 0x02},/* HREF horizontal start [11:8] */
		{0x3801, 0x22},/* HREF horizontal start [7:0] */
		{0x3802, 0x00},/* HREF vertical start [11:8] */
		{0x3803, 0x0C},/* HREF vertical start [7:0] */
		{0x3804, 0x0A},/* HREF horizontal width [11:8] */
		{0x3805, 0x00},/* HREF horizontal width [7:0] */
		{0x3806, 0x05},/* HREF vertical height [11:8] */
		{0x3807, 0xA0},/* HREF vertical height [7:0] */
		{0x3808, 0x0A},/* DVP output horizontal width [11:8] */
		{0x3809, 0x00},/* DVP output horizontal width [7:0] */
		{0x380A, 0x05},/* DVP output vertical height [11:8] */
		{0x380B, 0xA0},/* DVP output vertical height [7:0] */
		{0x380C, 0x0C},/* Total horizontal size [11:8] */
		{0x380D, 0xE0},/* Total horizontal size [7:0] */
		{0x380E, 0x05},/* Total vertical size [11:8] */
		{0x380F, 0xB0},/* Total vertical size [7:0] */
		{0x3818, 0xc0},/* BINNING related */
		{0x381A, 0x1c},
		{0x381C, 0x31},
		{0x381D, 0x0E},
		{0x381E, 0x05},
		{0x381F, 0xB0},
		{0x3820, 0x00},
		{0x3821, 0x20},
		{0x3824, 0x22},
		{0x3825, 0x64},
		{0x505a, 0x0a},
		{0x505b, 0x2e},
		{0x5002, 0x00},
		{0x5901, 0x00},
		{0x3815, 0x82},
		{0x401c, 0x46},
		{0x3010, 0x10},
		{0x3011, 0x10},
		{0x350C, 0x00},/* dummy lines [11:8] */
		{0x350D, 0x00},/* dummy lines [7:0] */
		{0x3810, 0x00},/* 20110126 note: add special for fpn upgrade function */
	},
	{	/* 2592x1944 15fps */
		{0X3613, 0x44},/* BINNING related */
		{0X3621, 0x2f},/* BINNING related */
		{0X3703, 0xE6},
		{0X3705, 0xda},
		{0X370a, 0x80},
		{0X370C, 0x00},
		{0X370D, 0x04},
		{0X3713, 0x22},
		{0X3714, 0x27},
		{0X3800, 0x02},/* HREF horizontal start [11:8] */
		{0X3801, 0x22},/* HREF horizontal start [7:0] */
		{0X3802, 0x00},/* HREF vertical start [11:8] */
		{0X3803, 0x0C},/* HREF vertical start [7:0] */
		{0X3804, 0x0A},/* HREF horizontal width [11:8] */
		{0X3805, 0x20},/* HREF horizontal width [7:0] */
		{0X3806, 0x07},/* HREF vertical height [11:8] */
		{0X3807, 0x98},/* HREF vertical height [7:0] */
		{0X3808, 0x0A},/* DVP output horizontal width [11:8] */
		{0X3809, 0x20},/* DVP output horizontal width [7:0] */
		{0X380A, 0x07},/* DVP output vertical height [11:8] */
		{0X380B, 0x98},/* DVP output vertical height [7:0] */
		{0X380C, 0x0C},/* Total horizontal size [11:8] */
		{0X380D, 0xC0},/* Total horizontal size [7:0] */
		{0X380E, 0x07},/* Total vertical size [11:8] */
		{0X380F, 0xAA},/* Total vertical size [7:0] */
		{0X3818, 0xc0},/* BINNING related */
		{0X381A, 0x1c},
		{0X381C, 0x30},
		{0X381D, 0x12},
		{0X381E, 0x07},
		{0X381F, 0xA8},
		{0X3820, 0x00},
		{0X3821, 0x20},
		{0X3824, 0x22},
		{0X3825, 0x54},
		{0X505a, 0x0a},
		{0X505b, 0x2e},
		{0X5002, 0x00},
		{0X5901, 0x00},
		{0X3815, 0x82},
		{0X401c, 0x46},
		{0X3010, 0x10},
		{0X3011, 0x10},
		{0X350C, 0x00},/* dummy lines [11:8] */
		{0X350D, 0x00},/* dummy lines [7:0] */
		{0X3810, 0x00},/* 20110126 note: add special for fpn upgrade function */
	},
};

static struct vin_video_format ov5653_formats[] = {
	{
		.video_mode	= AMBA_VIDEO_MODE_720P,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 1280,
		.def_height	= 720,
		/* sensor mode */
		.device_mode	= 0,
		.pll_idx	= 1,
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
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_BG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_1080P,
		.def_start_x	= 0,
		.def_start_y	= 0,
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
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_BG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_UXGA,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 1600,
		.def_height	= 1200,
		/* sensor mode */
		.device_mode	= 2,
		.pll_idx	= 0,
		.width		= 1600,
		.height		= 1200,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_BG,
	},
		{
		.video_mode	= AMBA_VIDEO_MODE_QXGA,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 2048,
		.def_height	= 1536,
		/* sensor mode */
		.device_mode	= 3,
		.pll_idx	= 0,
		.width		= 2048,
		.height		= 1536,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.max_fps	= AMBA_VIDEO_FPS_20,
		.default_fps	= AMBA_VIDEO_FPS_20,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_30,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_BG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_2560_1440,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 2560,
		.def_height	= 1440,
		/* sensor mode */
		.device_mode	= 4,
		.pll_idx	= 0,
		.width		= 2560,
		.height		= 1440,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_20,
		.default_fps	= AMBA_VIDEO_FPS_20,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_30,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_BG,
	},
		{
		.video_mode	= AMBA_VIDEO_MODE_QSXGA,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 2592,
		.def_height	= 1944,
		/* sensor mode */
		.device_mode	= 5,
		.pll_idx	= 0,
		.width		= 2592,
		.height		= 1944,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.max_fps	= AMBA_VIDEO_FPS_15,
		.default_fps	= AMBA_VIDEO_FPS_15,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_30,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_BG,
	},
};

static struct vin_reg_16_8 ov5653_share_regs[] = {
	{0x3008, 0x82},/* Register reset */
	{0x3008, 0x42},/* Register power down */
	{0x3103, 0x93},/* Clock from PAD */
	{0x3b07, 0x0c},

	/* IO control */
	{0x3017, 0xff},
	{0x3018, 0xfc},
	{0x3706, 0x41},
	{0x3630, 0x22},
	{0x3605, 0x04},
	{0x3606, 0x3f},
	{0x3712, 0x13},
	{0x370e, 0x00},
	{0x370b, 0x40},
	{0x3600, 0x54},
	{0x3601, 0x05},
	{0x3631, 0x22},
	{0x3612, 0x1a},
	{0x3604, 0x40},
	{0x3710, 0x28},
	{0x3702, 0x3a},
	{0x3704, 0x18},
	{0x3a18, 0x00},
	{0x3a19, 0xf8},
	{0x3a00, 0x38},
	{0x3830, 0x50},
	{0x3a08, 0x12},
	{0x3a09, 0x70},
	{0x3a0a, 0x0f},
	{0x3a0b, 0x60},
	{0x3a0d, 0x06},
	{0x3a0e, 0x06},
	{0x3a13, 0x54},
	{0x3815, 0x82},
	{0x5059, 0x80},
	{0x3615, 0x52},
	{0x3a08, 0x16},
	{0x3a09, 0x48},
	{0x3a0a, 0x12},
	{0x3a0b, 0x90},
	{0x3a0d, 0x03},
	{0x3a0e, 0x03},
	{0x3a1a, 0x06},
	{0x3623, 0x01},
	{0x3633, 0x24},
	{0x3c01, 0x34},
	{0x3c04, 0x28},
	{0x3c05, 0x98},
	{0x3c07, 0x07},
	{0x3c09, 0xc2},

	/* Black level calibration */
	{0x4000, 0x05},/* BLC control */
	{0x401d, 0x28},
	{0x4001, 0x02},
	{0x401c, 0x46},

	/* 20110208 : added to sync with old IQ table */
	{0x4006, 0x00},/* Black level target [9:8] */
	{0x4007, 0x04},/* Black level target [7:0], 1b */
	{0x401D, 0x08},/* 20090818: 0x28->0x08 (BLC trigger by gain change -> BLC trigger by every frame), 1b */
	{0x5046, 0x01},/* [3]AWB-gain_en, [0]ISP_en */
	{0x3810, 0x40},
	{0x3836, 0x41},
	{0x505f, 0x04},

	/* ISP control */
	{0x5000, 0x00},
	{0x5001, 0x00},
	{0x5000, 0x00},/* [7] LENC on, [2] Black pixel correct en, [1] white pixel correct en */
	{0x5001, 0x00},/* [0] AWB en */
	{0x503d, 0x00},/* Test pattern related register, refer to data sheet */
	{0x585a, 0x01},
	{0x585b, 0x2c},
	{0x585c, 0x01},
	{0x585d, 0x93},
	{0x585e, 0x01},
	{0x585f, 0x90},
	{0x5860, 0x01},
	{0x5861, 0x0d},
	{0x5180, 0xc0},
	{0x5184, 0x00},
	{0x470a, 0x00},
	{0x470b, 0x00},
	{0x470c, 0x00},
	{0x300f, 0x8e},
	{0x3603, 0xa7},
	{0x3632, 0x55},
	{0x3620, 0x56},
	{0x3631, 0x36},
	{0x3632, 0x5f},
	{0x3711, 0x24},
	{0x401f, 0x03},

	/* PLL setting: 24Mhz (/2 pre-div) (*16 multiplier) (/5 post-div) = 96Mhz  (Equation is TBD !!) */
	{0x3010, 0x10},/* [7:4]PLL DIVS divider, [3:0]PLL DIVM divider */
	{0x3011, 0x10},/* PLL multiplier */

	{0x3406, 0x01},
	{0x3400, 0x04},
	{0x3401, 0x00},
	{0x3402, 0x04},
	{0x3403, 0x00},
	{0x3404, 0x04},
	{0x3405, 0x00},
	{0x3503, 0x17},/* [2] VTS manual en, [1] AGC manual en, [0] AEC manual e */

	/* Temporally code: gain/shutter */
	{0x3500, 0x00},
	{0x3501, 0x18},
	{0x3502, 0x00},
	{0x350a, 0x00},
	{0x350b, 0x38},

	{0x4708, 0x03},/* change to 0x03 on S2L, do not reverse HREF */
};

/* OV5653 global gain table row size */
#define OV5653_GAIN_ROWS				(97)
#define OV5653_GAIN_COLS				(3)
#define OV5653_GAIN_0DB				(96)
#define OV5653_GAIN_DOUBLE			(16)

#define OV5653_GAIN_COL_AGC			(0)
#define OV5653_GAIN_COL_REG350A		(1)
#define OV5653_GAIN_COL_REG350B		(2)

const u16 ov5653_gains[OV5653_GAIN_ROWS][OV5653_GAIN_COLS] = {
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
