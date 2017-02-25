/*
 * Filename : ov9710_reg_tbl.c
 *
 * History:
 *    2014/11/11 - [Hao Zeng] Create
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

static struct vin_video_pll ov9710_plls[] = {
	/* for 30fps */
	{0, 24002537, 42004440},
	/* for 29.97fps */
	{0, 23978535, 41962436},
};

static struct vin_reg_16_8 ov9710_pll_regs[][3] = {
	{
		{OV9710_REG5C, 0x52},
		{OV9710_REG5D, 0x00},
		{OV9710_CLK,   0x01},
	},
	{
		{OV9710_REG5C, 0x52},
		{OV9710_REG5D, 0x00},
		{OV9710_CLK,   0x01},
	},
};

static struct vin_reg_16_8 ov9710_mode_regs[][22] = {
	{	/* 1280x800 */
		{OV9710_HSTART, 0x25},
		{OV9710_AHSIZE, 0xA2},
		{OV9710_VSTART, 0x01},
		{OV9710_AVSIZE, 0xCA},
		{OV9710_REG03, 0x0A},
		{OV9710_REG32, 0x07},
		{OV9710_DSP_CTRL_2, 0x00},
		{OV9710_DSP_CTRL_3, 0x00},
		{OV9710_DSP_CTRL_4, 0x00},
		{OV9710_REG57, 0x00},
		{OV9710_REG58, 0xC8},
		{OV9710_REG59, 0xA0},
		{OV9710_RSVD4C, 0x13},
		{OV9710_RSVD4B, 0x36},
		{OV9710_RENDL, 0x3C},
		{OV9710_RENDH, 0x03},
		{OV9710_YAVG_CTRL_0, 0xA0},
		{OV9710_YAVG_CTRL_1, 0xC8},
		{OV9710_COM7, 0x00},
		{OV9710_REG3B, 0x00},
		{OV9710_REG2A, 0x9B},
		{OV9710_REG2B, 0x06},
	},
};

static struct vin_video_format ov9710_formats[] = {
	{	/* 1280x800 */
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
		.bits = AMBA_VIDEO_BITS_10,
		.ratio = AMBA_VIDEO_RATIO_16_9,
		.max_fps = AMBA_VIDEO_FPS_30,
		.default_fps = AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_30,
		.default_bayer_pattern = VINDEV_BAYER_PATTERN_BG,
	},
	{	/* 1280x720 */
		.video_mode = AMBA_VIDEO_MODE_720P,
		.def_start_x = 0,
		.def_start_y = 40,
		.def_width = 1280,
		.def_height = 720,
		/* sensor mode */
		.device_mode = 0,
		.pll_idx = 0,
		.width = 1280,
		.height = 800,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_RGB_RAW,
		.bits = AMBA_VIDEO_BITS_10,
		.ratio = AMBA_VIDEO_RATIO_16_9,
		.max_fps = AMBA_VIDEO_FPS_30,
		.default_fps = AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_30,
		.default_bayer_pattern = VINDEV_BAYER_PATTERN_BG,
	},
};

static struct vin_reg_16_8 ov9710_share_regs[] = {
	{0x12, 0x80}, /* SW_RESET */
	{0x1e, 0x07},
	{0x5f, 0x18},
	{0x69, 0x04},
	{0x65, 0x2a},
	{0x68, 0x0a},
	{0x39, 0x28},
	{0x4d, 0x90},
	{0xc1, 0x80},

	{0x96, 0x01},
	{0xbc, 0x68},
	/* {0x12, 0x00}, */
	/* {0x3b, 0x00}, */
	{0x97, 0x80},
	{0x37, 0x02},
	{0x38, 0x10},
	{0x4e, 0x55},
	{0x4f, 0x55},
	{0x50, 0x55},
	{0x51, 0x55},
	{0x24, 0x55},
	{0x25, 0x40},
	{0x26, 0xa1},

	{0x13, 0x80},
	{0x14, 0x60},

	{0x41, 0x80},
	{0x6d, 0x02},	/* added for FPN optimum */
	{0x0C, 0x30},	/* performance optimization */

#if 0
	/* Vsync mode, run-time mirror/flip has bug under this mode */
	{0xc2, 0x41},
	{0xcb, 0x32},
	{0xc3, 0x21},
#else
	/* Vref mode, fix run-time mirror/flip feature */
	{0xc2, 0x81},
	{0xcb, 0xa6},
	{0xc3, 0x20},
#endif
};

/* Gain table */
/* OV9710 global gain table row size */
#define OV9710_GAIN_ROWS		(80 + 1)
#define OV9710_GAIN_0DB		(80)
#define OV9710_GAIN_COLS		(3)

#define OV9710_GAIN_COL_AGC	0
#define OV9710_GAIN_COL_FAGC	1
#define OV9710_GAIN_COL_REG	2

/* This is 16-step gain table, OV9710_GAIN_ROWS = 81, OV9710_GAIN_COLS = 3 */
const s16 OV9710_GLOBAL_GAIN_TABLE[OV9710_GAIN_ROWS][OV9710_GAIN_COLS] = {
	/* gain_value*256,  log2(gain)*1000,  register */
	/* error */
	{0x2000, 5000, 0xff},	/* index 0   0.000000   : 30dB */
	{0x1ea4, 4937, 0xff},	/* index 1   0.356695   */
	{0x1d58, 4875, 0xfe},	/* index 2   -0.344130  */
	{0x1c19, 4812, 0xfd},	/* index 3   -0.100035  */
	{0x1ae8, 4750, 0xfc},	/* index 4   0.091314   */
	{0x19c4, 4687, 0xfb},	/* index 5   0.232155   */
	{0x18ac, 4625, 0xfa},	/* index 6   0.324627   */
	{0x17a1, 4562, 0xf9},	/* index 7   0.370781   */
	{0x16a0, 4500, 0xf8},	/* index 8   0.372583   */
	{0x15ab, 4437, 0xf7},	/* index 9   0.331911   */
	{0x14bf, 4375, 0xf6},	/* index 10  0.250566   */
	{0x13de, 4312, 0xf5},	/* index 11  0.130276   */
	{0x1306, 4250, 0xf4},	/* index 12  -0.027313  */
	{0x1238, 4187, 0xf3},	/* index 13  -0.220617  */
	{0x1172, 4125, 0xf2},	/* index 14  -0.448124  */
	{0x10b5, 4062, 0xf1},	/* index 15  0.291620   */
	{0x1000, 4000, 0xf0},	/* index 16  0.000000   : 24dB */
	{0x0f52, 3937, 0x7f},	/* index 17  -0.321652  */
	{0x0eac, 3875, 0x7e},	/* index 18  0.327935   */
	{0x0e0c, 3812, 0x7d},	/* index 19  -0.050017  */
	{0x0d74, 3750, 0x7c},	/* index 20  -0.454343  */
	{0x0ce2, 3687, 0x7b},	/* index 21  0.116077   */
	{0x0c56, 3625, 0x7a},	/* index 22  -0.337687  */
	{0x0bd0, 3562, 0x79},	/* index 23  0.185390   */
	{0x0b50, 3500, 0x78},	/* index 24  -0.313708  */
	{0x0ad5, 3437, 0x77},	/* index 25  0.165956   */
	{0x0a5f, 3375, 0x76},	/* index 26  -0.374717  */
	{0x09ef, 3312, 0x75},	/* index 27  0.065138   */
	{0x0983, 3250, 0x74},	/* index 28  0.486343   */
	{0x091c, 3187, 0x73},	/* index 29  -0.110309  */
	{0x08b9, 3125, 0x72},	/* index 30  0.275938   */
	{0x085a, 3062, 0x71},	/* index 31  -0.354190  */
	{0x0800, 3000, 0x70},	/* index 32  0.000000   : 18dB */
	{0x07a9, 2937, 0x3f},	/* index 33  0.356695   */
	{0x0756, 2875, 0x3e},	/* index 34  -0.344130  */
	{0x0706, 2812, 0x3d},	/* index 35  -0.100035  */
	{0x06ba, 2750, 0x3c},	/* index 36  0.091314   */
	{0x0671, 2687, 0x3b},	/* index 37  0.232155   */
	{0x062b, 2625, 0x3a},	/* index 38  0.324627   */
	{0x05e8, 2562, 0x39},	/* index 39  0.370781   */
	{0x05a8, 2500, 0x38},	/* index 40  0.372583   */
	{0x056a, 2437, 0x37},	/* index 41  0.331911   */
	{0x052f, 2375, 0x36},	/* index 42  0.250566   */
	{0x04f7, 2312, 0x35},	/* index 43  0.130276   */
	{0x04c1, 2250, 0x34},	/* index 44  -0.027313  */
	{0x048e, 2187, 0x33},	/* index 45  -0.220617  */
	{0x045c, 2125, 0x32},	/* index 46  -0.448124  */
	{0x042d, 2062, 0x31},	/* index 47  0.291620   */
	{0x0400, 2000, 0x30},	/* index 48  0.000000   : 12dB */
	{0x03d4, 1937, 0x1f},	/* index 49  0.356695   */
	{0x03ab, 1875, 0x1e},	/* index 50  -0.344130  */
	{0x0383, 1812, 0x1d},	/* index 51  -0.100035  */
	{0x035d, 1750, 0x1c},	/* index 52  0.091314   */
	{0x0338, 1687, 0x1b},	/* index 53  0.232155   */
	{0x0315, 1625, 0x1a},	/* index 54  0.324627   */
	{0x02f4, 1562, 0x19},	/* index 55  0.370781   */
	{0x02d4, 1500, 0x18},	/* index 56  0.372583   */
	{0x02b5, 1437, 0x17},	/* index 57  0.331911   */
	{0x0297, 1375, 0x16},	/* index 58  0.250566   */
	{0x027b, 1312, 0x15},	/* index 59  0.130276   */
	{0x0260, 1250, 0x14},	/* index 60  -0.027313  */
	{0x0247, 1187, 0x13},	/* index 61  -0.220617  */
	{0x022e, 1125, 0x12},	/* index 62  -0.448124  */
	{0x0216, 1062, 0x11},	/* index 63  0.291620   */
	{0x0200, 1000, 0x10},	/* index 64  0.000000   : 6dB */
	{0x01ea, 937, 0x0f},	/* index 65  -0.321652  */
	{0x01d5, 875, 0x0e},	/* index 66  0.327935   */
	{0x01c1, 812, 0x0d},	/* index 67  -0.050017  */
	{0x01ae, 750, 0x0c},	/* index 68  -0.454343  */
	{0x019c, 687, 0x0b},	/* index 69  0.116077   */
	{0x018a, 625, 0x0a},	/* index 70  -0.337687  */
	{0x017a, 562, 0x09},	/* index 71  0.185390   */
	{0x016a, 500, 0x08},	/* index 72  -0.313708  */
	{0x015a, 437, 0x07},	/* index 73  0.165956   */
	{0x014b, 375, 0x06},	/* index 74  -0.374717  */
	{0x013d, 312, 0x05},	/* index 75  0.065138   */
	{0x0130, 250, 0x04},	/* index 76  0.486343   */
	{0x0123, 187, 0x03},	/* index 77  -0.110309  */
	{0x0117, 125, 0x02},	/* index 78  0.275938   */
	{0x010b, 62, 0x01},	/* index 79  -0.354190  */
	{0x0100, 0, 0x00},	/* index 80  0.000000   : 0dB */
};

