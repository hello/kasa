/*
 * Filename : mn34227pl_reg_tbl.c
 *
 * History:
 *	2015/05/27 - [Long Zhao] Create
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


static struct vin_video_pll mn34227pl_plls[] = {
	{0, 27000000, 74250000},/* for linear */
	{0, 27000000, 148500000},/* 2x wdr mode */
};

static struct vin_video_format mn34227pl_formats[] = {
	{
		.video_mode	= AMBA_VIDEO_MODE_1080P,
		.def_start_x	= 4,
		.def_start_y	= 2+10+4,
		.def_width	= 1920,
		.def_height	= 1080,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_LINEAR_MODE,
		.device_mode	= 0,
		.pll_idx	= 0,
		.width		= 1956,
		.height		= 1108,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_GR,
	},
	/* 2x hdr mode */
	{
		.video_mode	= AMBA_VIDEO_MODE_1080P,
		.def_start_x	= 4,
		.def_start_y	= (2+10+4)*2,
		.def_width	= 1920,
		.def_height	= 4500 - (2+10+4)*2, /* VMAX*2 - def_start_y*2 */
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 1920,
		.act_height	= 1080,
		.max_act_width = 1944,
		.max_act_height = 1092,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_2X_HDR_MODE,
		.device_mode	= 0,
		.pll_idx	= 1,
		.width		= 1956,
		.height		= 1108,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_15,
		.default_fps	= AMBA_VIDEO_FPS_15,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_15,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_GR,
	},
};

static struct vin_reg_16_16 mn34227pl_linear_mode_regs[] = {
	/* N111_S12_P4_FHD_V1125_12b_222MHz_30fps_vM17e_141006_Mst_I2C_d_Amb */
	/* VCYCLE:1125 HCYCLE:800 (@MCLK) */
	{0x300E, 0x0001},
	{0x300F, 0x0000},
	{0x0307, 0x0021},
	{0x3000, 0x0000},
	{0x3001, 0x0003},
	{0x0112, 0x000C},
	{0x0113, 0x000C},
	{0x3005, 0x0064},
	{0x3007, 0x0014},
	{0x3018, 0x0043},
	{0x3019, 0x0010},
	{0x301A, 0x00B9},
	{0x3000, 0x0000},
	{0x3001, 0x0053},
	{0x300B, 0x0000},
	{0x300E, 0x0000},
	{0x300F, 0x0000},
	{0x0202, 0x0004},
	{0x0203, 0x0063},
	{0x3036, 0x0000},
	{0x3039, 0x002E},
	{0x3058, 0x000F},
	{0x305D, 0x0040},
	{0x3066, 0x000A},
	{0x3067, 0x00B0},
	{0x3068, 0x0008},
	{0x3069, 0x0000},
	{0x306A, 0x000B},
	{0x306B, 0x0060},
	{0x306C, 0x0009},
	{0x306D, 0x00D0},
	{0x306E, 0x000C},
	{0x306F, 0x0000},
	{0x3074, 0x0001},
	{0x3098, 0x0000},
	{0x3099, 0x0000},
	{0x309A, 0x0001},
	{0x3104, 0x0004},
	{0x3106, 0x0000},
	{0x3107, 0x00C0},
	{0x3141, 0x0040},
	{0x3143, 0x0002},
	{0x3144, 0x0002},
	{0x3145, 0x0002},
	{0x3146, 0x0000},
	{0x3147, 0x0002},
	{0x314A, 0x0001},
	{0x314B, 0x0002},
	{0x314C, 0x0002},
	{0x314D, 0x0002},
	{0x314E, 0x0001},
	{0x314F, 0x0002},
	{0x3150, 0x0002},
	{0x3152, 0x0004},
	{0x3153, 0x00E3},
	{0x316F, 0x00C6},
	{0x3175, 0x0080},
	{0x318E, 0x0020},
	{0x318F, 0x0070},
	{0x3196, 0x0008},
	{0x3243, 0x00D7},
	{0x3247, 0x0038},
	{0x3248, 0x0003},
	{0x3249, 0x00E2},
	{0x324A, 0x0030},
	{0x324B, 0x0018},
	{0x324C, 0x0002},
	{0x3253, 0x00DE},
	{0x3259, 0x0068},
	{0x3272, 0x0046},
	{0x3280, 0x0030},
	{0x3288, 0x0001},
	{0x330E, 0x0005},
	{0x3310, 0x0002},
	{0x3315, 0x001F},
	{0x332C, 0x0002},
	{0x3339, 0x0002},
	{0x3000, 0x0000},
	{0x3001, 0x00D3},
	{0x0100, 0x0001},
	{0x0101, 0x0000},
};

static struct vin_reg_16_16 mn34227pl_2x_wdr_mode_regs[] = {
	/* NP10_S12_P4_FHD_WDRx2_V2250_12b_445MHz_15fps_vM17e_150518_Mst_I2C_d_Amb.txt */
	/* VCYCLE:2250 HCYCLE:800 (@MCLK) */
	{0x300E, 0x0001},
	{0x300F, 0x0000},
	{0x0307, 0x0021},
	{0x3000, 0x0000},
	{0x3001, 0x0003},
	{0x0112, 0x000C},
	{0x0113, 0x000C},
	{0x3005, 0x0064},
	{0x3018, 0x0043},
	{0x3019, 0x0010},
	{0x301A, 0x00B9},
	{0x3000, 0x0000},
	{0x3001, 0x0053},
	{0x300B, 0x0000},
	{0x300E, 0x0000},
	{0x300F, 0x0000},
	{0x0202, 0x0000},
	{0x0203, 0x0020},
	{0x0340, 0x0008},
	{0x0341, 0x00CA},
	{0x0342, 0x0011},
	{0x0343, 0x0030},
	{0x3036, 0x0000},
	{0x3039, 0x002E},
	{0x3058, 0x000F},
	{0x305D, 0x0042},
#if 0
	{0x3066, 0x000A},
	{0x3067, 0x00B0},
	{0x3068, 0x0008},
	{0x3069, 0x0000},
	{0x306A, 0x0009},
	{0x306B, 0x00D0},
	{0x306C, 0x0009},
	{0x306D, 0x00D0},
#endif
	{0x306E, 0x000C},
	{0x306F, 0x0000},
	{0x3074, 0x0001},
	{0x3078, 0x0001},
	{0x3098, 0x0000},
	{0x3099, 0x0000},
	{0x309A, 0x0001},
	{0x3101, 0x0001},
	{0x3104, 0x0004},
	{0x3106, 0x0000},
	{0x3107, 0x00C0},
	{0x312B, 0x0020},
	{0x312D, 0x0020},
	{0x312F, 0x0020},
	{0x3141, 0x0040},
	{0x3143, 0x0002},
	{0x3144, 0x0002},
	{0x3145, 0x0002},
	{0x3146, 0x0000},
	{0x3147, 0x0002},
	{0x314A, 0x0001},
	{0x314B, 0x0002},
	{0x314C, 0x0002},
	{0x314D, 0x0002},
	{0x314E, 0x0001},
	{0x314F, 0x0002},
	{0x3150, 0x0002},
	{0x3152, 0x0004},
	{0x3153, 0x00E3},
	{0x316F, 0x00C6},
	{0x3175, 0x0080},
	{0x318E, 0x0020},
	{0x318F, 0x0070},
	{0x3196, 0x0008},
	{0x323C, 0x0070},
	{0x323E, 0x0000},
	{0x3243, 0x00D7},
	{0x3247, 0x0079},
	{0x324A, 0x0030},
	{0x324B, 0x0018},
	{0x324C, 0x0002},
	{0x3253, 0x00DE},
	{0x3259, 0x0049},
	{0x3272, 0x0046},
	{0x3280, 0x0030},
	{0x3288, 0x0001},
	{0x330E, 0x0005},
	{0x3310, 0x0002},
	{0x3315, 0x001F},
	{0x332C, 0x0002},
	{0x3339, 0x0002},
	{0x3000, 0x0000},
	{0x3001, 0x00D3},
	{0x0100, 0x0001},
	{0x0101, 0x0000},
};

#define MN34227PL_GAIN_COLS		(2)
#define MN34227PL_GAIN_COL_AGAIN	(0)
#define MN34227PL_GAIN_COL_DGAIN	(1)

#if GAIN_160_STEPS
/** MN34227PL global gain table row size */
#define MN34227PL_GAIN_ROWS		(128 + 1)
#define MN34227PL_GAIN_0DB		(128)
#define MN34227PL_WDR_GAIN_18DB 	(48)
#define MN34227PL_WDR_GAIN_30DB	(80)

/* MN34227PL_GAIN_ROWS = 129, MN34227PL_GAIN_COLS = 2 */
const s16 MN34227PL_GAIN_TABLE[MN34227PL_GAIN_ROWS][MN34227PL_GAIN_COLS] =
{
	/* Analog Gain,Digital Gain */
	{0x240, 0x1c0}, /* index 128, gain_db=48.000000 DB, again_db=30.000000 DB, dgain_db=18.000000 DB */
	{0x240, 0x1bc}, /* index 127, gain_db=47.625000 DB, again_db=30.000000 DB, dgain_db=17.625000 DB */
	{0x240, 0x1b8}, /* index 126, gain_db=47.250000 DB, again_db=30.000000 DB, dgain_db=17.250000 DB */
	{0x240, 0x1b4}, /* index 125, gain_db=46.875000 DB, again_db=30.000000 DB, dgain_db=16.875000 DB */
	{0x240, 0x1b0}, /* index 124, gain_db=46.500000 DB, again_db=30.000000 DB, dgain_db=16.500000 DB */
	{0x240, 0x1ac}, /* index 123, gain_db=46.125000 DB, again_db=30.000000 DB, dgain_db=16.125000 DB */
	{0x240, 0x1a8}, /* index 122, gain_db=45.750000 DB, again_db=30.000000 DB, dgain_db=15.750000 DB */
	{0x240, 0x1a4}, /* index 121, gain_db=45.375000 DB, again_db=30.000000 DB, dgain_db=15.375000 DB */
	{0x240, 0x1a0}, /* index 120, gain_db=45.000000 DB, again_db=30.000000 DB, dgain_db=15.000000 DB */
	{0x240, 0x19c}, /* index 119, gain_db=44.625000 DB, again_db=30.000000 DB, dgain_db=14.625000 DB */
	{0x240, 0x198}, /* index 118, gain_db=44.250000 DB, again_db=30.000000 DB, dgain_db=14.250000 DB */
	{0x240, 0x194}, /* index 117, gain_db=43.875000 DB, again_db=30.000000 DB, dgain_db=13.875000 DB */
	{0x240, 0x190}, /* index 116, gain_db=43.500000 DB, again_db=30.000000 DB, dgain_db=13.500000 DB */
	{0x240, 0x18c}, /* index 115, gain_db=43.125000 DB, again_db=30.000000 DB, dgain_db=13.125000 DB */
	{0x240, 0x188}, /* index 114, gain_db=42.750000 DB, again_db=30.000000 DB, dgain_db=12.750000 DB */
	{0x240, 0x184}, /* index 113, gain_db=42.375000 DB, again_db=30.000000 DB, dgain_db=12.375000 DB */
	{0x240, 0x180}, /* index 112, gain_db=42.000000 DB, again_db=30.000000 DB, dgain_db=12.000000 DB */
	{0x240, 0x17c}, /* index 111, gain_db=41.625000 DB, again_db=30.000000 DB, dgain_db=11.625000 DB */
	{0x240, 0x178}, /* index 110, gain_db=41.250000 DB, again_db=30.000000 DB, dgain_db=11.250000 DB */
	{0x240, 0x174}, /* index 109, gain_db=40.875000 DB, again_db=30.000000 DB, dgain_db=10.875000 DB */
	{0x240, 0x170}, /* index 108, gain_db=40.500000 DB, again_db=30.000000 DB, dgain_db=10.500000 DB */
	{0x240, 0x16c}, /* index 107, gain_db=40.125000 DB, again_db=30.000000 DB, dgain_db=10.125000 DB */
	{0x240, 0x168}, /* index 106, gain_db=39.750000 DB, again_db=30.000000 DB, dgain_db=9.750000 DB */
	{0x240, 0x164}, /* index 105, gain_db=39.375000 DB, again_db=30.000000 DB, dgain_db=9.375000 DB */
	{0x240, 0x160}, /* index 104, gain_db=39.000000 DB, again_db=30.000000 DB, dgain_db=9.000000 DB */
	{0x240, 0x15c}, /* index 103, gain_db=38.625000 DB, again_db=30.000000 DB, dgain_db=8.625000 DB */
	{0x240, 0x158}, /* index 102, gain_db=38.250000 DB, again_db=30.000000 DB, dgain_db=8.250000 DB */
	{0x240, 0x154}, /* index 101, gain_db=37.875000 DB, again_db=30.000000 DB, dgain_db=7.875000 DB */
	{0x240, 0x150}, /* index 100, gain_db=37.500000 DB, again_db=30.000000 DB, dgain_db=7.500000 DB */
	{0x240, 0x14c}, /* index 99, gain_db=37.125000 DB, again_db=30.000000 DB, dgain_db=7.125000 DB */
	{0x240, 0x148}, /* index 98, gain_db=36.750000 DB, again_db=30.000000 DB, dgain_db=6.750000 DB */
	{0x240, 0x144}, /* index 97, gain_db=36.375000 DB, again_db=30.000000 DB, dgain_db=6.375000 DB */
	{0x240, 0x140}, /* index 96, gain_db=36.000000 DB, again_db=30.000000 DB, dgain_db=6.000000 DB */
	{0x240, 0x13c}, /* index 95, gain_db=35.625000 DB, again_db=30.000000 DB, dgain_db=5.625000 DB */
	{0x240, 0x138}, /* index 94, gain_db=35.250000 DB, again_db=30.000000 DB, dgain_db=5.250000 DB */
	{0x240, 0x134}, /* index 93, gain_db=34.875000 DB, again_db=30.000000 DB, dgain_db=4.875000 DB */
	{0x240, 0x130}, /* index 92, gain_db=34.500000 DB, again_db=30.000000 DB, dgain_db=4.500000 DB */
	{0x240, 0x12c}, /* index 91, gain_db=34.125000 DB, again_db=30.000000 DB, dgain_db=4.125000 DB */
	{0x240, 0x128}, /* index 90, gain_db=33.750000 DB, again_db=30.000000 DB, dgain_db=3.750000 DB */
	{0x240, 0x124}, /* index 89, gain_db=33.375000 DB, again_db=30.000000 DB, dgain_db=3.375000 DB */
	{0x240, 0x120}, /* index 88, gain_db=33.000000 DB, again_db=30.000000 DB, dgain_db=3.000000 DB */
	{0x240, 0x11c}, /* index 87, gain_db=32.625000 DB, again_db=30.000000 DB, dgain_db=2.625000 DB */
	{0x240, 0x118}, /* index 86, gain_db=32.250000 DB, again_db=30.000000 DB, dgain_db=2.250000 DB */
	{0x240, 0x114}, /* index 85, gain_db=31.875000 DB, again_db=30.000000 DB, dgain_db=1.875000 DB */
	{0x240, 0x110}, /* index 84, gain_db=31.500000 DB, again_db=30.000000 DB, dgain_db=1.500000 DB */
	{0x240, 0x10c}, /* index 83, gain_db=31.125000 DB, again_db=30.000000 DB, dgain_db=1.125000 DB */
	{0x240, 0x108}, /* index 82, gain_db=30.750000 DB, again_db=30.000000 DB, dgain_db=0.750000 DB */
	{0x240, 0x104}, /* index 81, gain_db=30.375000 DB, again_db=30.000000 DB, dgain_db=0.375000 DB */
	{0x240, 0x100}, /* index 80, gain_db=30.000000 DB, again_db=30.000000 DB, dgain_db=0.000000 DB */

	{0x23c, 0x100}, /* index 79, gain_db=29.625000 DB, again_db=29.625000 DB, dgain_db=0.000000 DB */
	{0x238, 0x100}, /* index 78, gain_db=29.250000 DB, again_db=29.250000 DB, dgain_db=0.000000 DB */
	{0x234, 0x100}, /* index 77, gain_db=28.875000 DB, again_db=28.875000 DB, dgain_db=0.000000 DB */
	{0x230, 0x100}, /* index 76, gain_db=28.500000 DB, again_db=28.500000 DB, dgain_db=0.000000 DB */
	{0x22c, 0x100}, /* index 75, gain_db=28.125000 DB, again_db=28.125000 DB, dgain_db=0.000000 DB */
	{0x228, 0x100}, /* index 74, gain_db=27.750000 DB, again_db=27.750000 DB, dgain_db=0.000000 DB */
	{0x224, 0x100}, /* index 73, gain_db=27.375000 DB, again_db=27.375000 DB, dgain_db=0.000000 DB */
	{0x220, 0x100}, /* index 72, gain_db=27.000000 DB, again_db=27.000000 DB, dgain_db=0.000000 DB */
	{0x21c, 0x100}, /* index 71, gain_db=26.625000 DB, again_db=26.625000 DB, dgain_db=0.000000 DB */
	{0x218, 0x100}, /* index 70, gain_db=26.250000 DB, again_db=26.250000 DB, dgain_db=0.000000 DB */
	{0x214, 0x100}, /* index 69, gain_db=25.875000 DB, again_db=25.875000 DB, dgain_db=0.000000 DB */
	{0x210, 0x100}, /* index 68, gain_db=25.500000 DB, again_db=25.500000 DB, dgain_db=0.000000 DB */
	{0x20c, 0x100}, /* index 67, gain_db=25.125000 DB, again_db=25.125000 DB, dgain_db=0.000000 DB */
	{0x208, 0x100}, /* index 66, gain_db=24.750000 DB, again_db=24.750000 DB, dgain_db=0.000000 DB */
	{0x204, 0x100}, /* index 65, gain_db=24.375000 DB, again_db=24.375000 DB, dgain_db=0.000000 DB */
	{0x200, 0x100}, /* index 64, gain_db=24.000000 DB, again_db=24.000000 DB, dgain_db=0.000000 DB */
	{0x1fc, 0x100}, /* index 63, gain_db=23.625000 DB, again_db=23.625000 DB, dgain_db=0.000000 DB */
	{0x1f8, 0x100}, /* index 62, gain_db=23.250000 DB, again_db=23.250000 DB, dgain_db=0.000000 DB */
	{0x1f4, 0x100}, /* index 61, gain_db=22.875000 DB, again_db=22.875000 DB, dgain_db=0.000000 DB */
	{0x1f0, 0x100}, /* index 60, gain_db=22.500000 DB, again_db=22.500000 DB, dgain_db=0.000000 DB */
	{0x1ec, 0x100}, /* index 59, gain_db=22.125000 DB, again_db=22.125000 DB, dgain_db=0.000000 DB */
	{0x1e8, 0x100}, /* index 58, gain_db=21.750000 DB, again_db=21.750000 DB, dgain_db=0.000000 DB */
	{0x1e4, 0x100}, /* index 57, gain_db=21.375000 DB, again_db=21.375000 DB, dgain_db=0.000000 DB */
	{0x1e0, 0x100}, /* index 56, gain_db=21.000000 DB, again_db=21.000000 DB, dgain_db=0.000000 DB */
	{0x1dc, 0x100}, /* index 55, gain_db=20.625000 DB, again_db=20.625000 DB, dgain_db=0.000000 DB */
	{0x1d8, 0x100}, /* index 54, gain_db=20.250000 DB, again_db=20.250000 DB, dgain_db=0.000000 DB */
	{0x1d4, 0x100}, /* index 53, gain_db=19.875000 DB, again_db=19.875000 DB, dgain_db=0.000000 DB */
	{0x1d0, 0x100}, /* index 52, gain_db=19.500000 DB, again_db=19.500000 DB, dgain_db=0.000000 DB */
	{0x1cc, 0x100}, /* index 51, gain_db=19.125000 DB, again_db=19.125000 DB, dgain_db=0.000000 DB */
	{0x1c8, 0x100}, /* index 50, gain_db=18.750000 DB, again_db=18.750000 DB, dgain_db=0.000000 DB */
	{0x1c4, 0x100}, /* index 49, gain_db=18.375000 DB, again_db=18.375000 DB, dgain_db=0.000000 DB */
	{0x1c0, 0x100}, /* index 48, gain_db=18.000000 DB, again_db=18.000000 DB, dgain_db=0.000000 DB */
	{0x1bc, 0x100}, /* index 47, gain_db=17.625000 DB, again_db=17.625000 DB, dgain_db=0.000000 DB */
	{0x1b8, 0x100}, /* index 46, gain_db=17.250000 DB, again_db=17.250000 DB, dgain_db=0.000000 DB */
	{0x1b4, 0x100}, /* index 45, gain_db=16.875000 DB, again_db=16.875000 DB, dgain_db=0.000000 DB */
	{0x1b0, 0x100}, /* index 44, gain_db=16.500000 DB, again_db=16.500000 DB, dgain_db=0.000000 DB */
	{0x1ac, 0x100}, /* index 43, gain_db=16.125000 DB, again_db=16.125000 DB, dgain_db=0.000000 DB */
	{0x1a8, 0x100}, /* index 42, gain_db=15.750000 DB, again_db=15.750000 DB, dgain_db=0.000000 DB */
	{0x1a4, 0x100}, /* index 41, gain_db=15.375000 DB, again_db=15.375000 DB, dgain_db=0.000000 DB */
	{0x1a0, 0x100}, /* index 40, gain_db=15.000000 DB, again_db=15.000000 DB, dgain_db=0.000000 DB */
	{0x19c, 0x100}, /* index 39, gain_db=14.625000 DB, again_db=14.625000 DB, dgain_db=0.000000 DB */
	{0x198, 0x100}, /* index 38, gain_db=14.250000 DB, again_db=14.250000 DB, dgain_db=0.000000 DB */
	{0x194, 0x100}, /* index 37, gain_db=13.875000 DB, again_db=13.875000 DB, dgain_db=0.000000 DB */
	{0x190, 0x100}, /* index 36, gain_db=13.500000 DB, again_db=13.500000 DB, dgain_db=0.000000 DB */
	{0x18c, 0x100}, /* index 35, gain_db=13.125000 DB, again_db=13.125000 DB, dgain_db=0.000000 DB */
	{0x188, 0x100}, /* index 34, gain_db=12.750000 DB, again_db=12.750000 DB, dgain_db=0.000000 DB */
	{0x184, 0x100}, /* index 33, gain_db=12.375000 DB, again_db=12.375000 DB, dgain_db=0.000000 DB */
	{0x180, 0x100}, /* index 32, gain_db=12.000000 DB, again_db=12.000000 DB, dgain_db=0.000000 DB */
	{0x17c, 0x100}, /* index 31, gain_db=11.625000 DB, again_db=11.625000 DB, dgain_db=0.000000 DB */
	{0x178, 0x100}, /* index 30, gain_db=11.250000 DB, again_db=11.250000 DB, dgain_db=0.000000 DB */
	{0x174, 0x100}, /* index 29, gain_db=10.875000 DB, again_db=10.875000 DB, dgain_db=0.000000 DB */
	{0x170, 0x100}, /* index 28, gain_db=10.500000 DB, again_db=10.500000 DB, dgain_db=0.000000 DB */
	{0x16c, 0x100}, /* index 27, gain_db=10.125000 DB, again_db=10.125000 DB, dgain_db=0.000000 DB */
	{0x168, 0x100}, /* index 26, gain_db=9.750000 DB, again_db=9.750000 DB, dgain_db=0.000000 DB */
	{0x164, 0x100}, /* index 25, gain_db=9.375000 DB, again_db=9.375000 DB, dgain_db=0.000000 DB */
	{0x160, 0x100}, /* index 24, gain_db=9.000000 DB, again_db=9.000000 DB, dgain_db=0.000000 DB */
	{0x15c, 0x100}, /* index 23, gain_db=8.625000 DB, again_db=8.625000 DB, dgain_db=0.000000 DB */
	{0x158, 0x100}, /* index 22, gain_db=8.250000 DB, again_db=8.250000 DB, dgain_db=0.000000 DB */
	{0x154, 0x100}, /* index 21, gain_db=7.875000 DB, again_db=7.875000 DB, dgain_db=0.000000 DB */
	{0x150, 0x100}, /* index 20, gain_db=7.500000 DB, again_db=7.500000 DB, dgain_db=0.000000 DB */
	{0x14c, 0x100}, /* index 19, gain_db=7.125000 DB, again_db=7.125000 DB, dgain_db=0.000000 DB */
	{0x148, 0x100}, /* index 18, gain_db=6.750000 DB, again_db=6.750000 DB, dgain_db=0.000000 DB */
	{0x144, 0x100}, /* index 17, gain_db=6.375000 DB, again_db=6.375000 DB, dgain_db=0.000000 DB */
	{0x140, 0x100}, /* index 16, gain_db=6.000000 DB, again_db=6.000000 DB, dgain_db=0.000000 DB */
	{0x13c, 0x100}, /* index 15, gain_db=5.625000 DB, again_db=5.625000 DB, dgain_db=0.000000 DB */
	{0x138, 0x100}, /* index 14, gain_db=5.250000 DB, again_db=5.250000 DB, dgain_db=0.000000 DB */
	{0x134, 0x100}, /* index 13, gain_db=4.875000 DB, again_db=4.875000 DB, dgain_db=0.000000 DB */
	{0x130, 0x100}, /* index 12, gain_db=4.500000 DB, again_db=4.500000 DB, dgain_db=0.000000 DB */
	{0x12c, 0x100}, /* index 11, gain_db=4.125000 DB, again_db=4.125000 DB, dgain_db=0.000000 DB */
	{0x128, 0x100}, /* index 10, gain_db=3.750000 DB, again_db=3.750000 DB, dgain_db=0.000000 DB */
	{0x124, 0x100}, /* index 9, gain_db=3.375000 DB, again_db=3.375000 DB, dgain_db=0.000000 DB */
	{0x120, 0x100}, /* index 8, gain_db=3.000000 DB, again_db=3.000000 DB, dgain_db=0.000000 DB */
	{0x11c, 0x100}, /* index 7, gain_db=2.625000 DB, again_db=2.625000 DB, dgain_db=0.000000 DB */
	{0x118, 0x100}, /* index 6, gain_db=2.250000 DB, again_db=2.250000 DB, dgain_db=0.000000 DB */
	{0x114, 0x100}, /* index 5, gain_db=1.875000 DB, again_db=1.875000 DB, dgain_db=0.000000 DB */
	{0x110, 0x100}, /* index 4, gain_db=1.500000 DB, again_db=1.500000 DB, dgain_db=0.000000 DB */
	{0x10c, 0x100}, /* index 3, gain_db=1.125000 DB, again_db=1.125000 DB, dgain_db=0.000000 DB */
	{0x108, 0x100}, /* index 2, gain_db=0.750000 DB, again_db=0.750000 DB, dgain_db=0.000000 DB */
	{0x104, 0x100}, /* index 1, gain_db=0.375000 DB, again_db=0.375000 DB, dgain_db=0.000000 DB */
	{0x100, 0x100}, /* index 0, gain_db=0.000000 DB, again_db=0.000000 DB, dgain_db=0.000000 DB */
};
#else
/** MN34227PL global gain table row size */
#define MN34227PL_GAIN_ROWS		(512 + 1)
#define MN34227PL_GAIN_0DB		(512)
#define MN34227PL_WDR_GAIN_18DB 	(192)
#define MN34227PL_WDR_GAIN_30DB	(320)

/* MN34227PL_GAIN_ROWS = 641, MN34227PL_GAIN_COLS = 2 */
const s16 MN34227PL_GAIN_TABLE[MN34227PL_GAIN_ROWS][MN34227PL_GAIN_COLS] =
{
	/* Analog Gain,Digital Gain */
	{0x240, 0x1c0}, /* index 512, gain_db=48.000000 DB, again_db=30.000000 DB, dgain_db=18.000000 DB */
	{0x240, 0x1bf}, /* index 511, gain_db=47.906250 DB, again_db=30.000000 DB, dgain_db=17.906250 DB */
	{0x240, 0x1be}, /* index 510, gain_db=47.812500 DB, again_db=30.000000 DB, dgain_db=17.812500 DB */
	{0x240, 0x1bd}, /* index 509, gain_db=47.718750 DB, again_db=30.000000 DB, dgain_db=17.718750 DB */
	{0x240, 0x1bc}, /* index 508, gain_db=47.625000 DB, again_db=30.000000 DB, dgain_db=17.625000 DB */
	{0x240, 0x1bb}, /* index 507, gain_db=47.531250 DB, again_db=30.000000 DB, dgain_db=17.531250 DB */
	{0x240, 0x1ba}, /* index 506, gain_db=47.437500 DB, again_db=30.000000 DB, dgain_db=17.437500 DB */
	{0x240, 0x1b9}, /* index 505, gain_db=47.343750 DB, again_db=30.000000 DB, dgain_db=17.343750 DB */
	{0x240, 0x1b8}, /* index 504, gain_db=47.250000 DB, again_db=30.000000 DB, dgain_db=17.250000 DB */
	{0x240, 0x1b7}, /* index 503, gain_db=47.156250 DB, again_db=30.000000 DB, dgain_db=17.156250 DB */
	{0x240, 0x1b6}, /* index 502, gain_db=47.062500 DB, again_db=30.000000 DB, dgain_db=17.062500 DB */
	{0x240, 0x1b5}, /* index 501, gain_db=46.968750 DB, again_db=30.000000 DB, dgain_db=16.968750 DB */
	{0x240, 0x1b4}, /* index 500, gain_db=46.875000 DB, again_db=30.000000 DB, dgain_db=16.875000 DB */
	{0x240, 0x1b3}, /* index 499, gain_db=46.781250 DB, again_db=30.000000 DB, dgain_db=16.781250 DB */
	{0x240, 0x1b2}, /* index 498, gain_db=46.687500 DB, again_db=30.000000 DB, dgain_db=16.687500 DB */
	{0x240, 0x1b1}, /* index 497, gain_db=46.593750 DB, again_db=30.000000 DB, dgain_db=16.593750 DB */
	{0x240, 0x1b0}, /* index 496, gain_db=46.500000 DB, again_db=30.000000 DB, dgain_db=16.500000 DB */
	{0x240, 0x1af}, /* index 495, gain_db=46.406250 DB, again_db=30.000000 DB, dgain_db=16.406250 DB */
	{0x240, 0x1ae}, /* index 494, gain_db=46.312500 DB, again_db=30.000000 DB, dgain_db=16.312500 DB */
	{0x240, 0x1ad}, /* index 493, gain_db=46.218750 DB, again_db=30.000000 DB, dgain_db=16.218750 DB */
	{0x240, 0x1ac}, /* index 492, gain_db=46.125000 DB, again_db=30.000000 DB, dgain_db=16.125000 DB */
	{0x240, 0x1ab}, /* index 491, gain_db=46.031250 DB, again_db=30.000000 DB, dgain_db=16.031250 DB */
	{0x240, 0x1aa}, /* index 490, gain_db=45.937500 DB, again_db=30.000000 DB, dgain_db=15.937500 DB */
	{0x240, 0x1a9}, /* index 489, gain_db=45.843750 DB, again_db=30.000000 DB, dgain_db=15.843750 DB */
	{0x240, 0x1a8}, /* index 488, gain_db=45.750000 DB, again_db=30.000000 DB, dgain_db=15.750000 DB */
	{0x240, 0x1a7}, /* index 487, gain_db=45.656250 DB, again_db=30.000000 DB, dgain_db=15.656250 DB */
	{0x240, 0x1a6}, /* index 486, gain_db=45.562500 DB, again_db=30.000000 DB, dgain_db=15.562500 DB */
	{0x240, 0x1a5}, /* index 485, gain_db=45.468750 DB, again_db=30.000000 DB, dgain_db=15.468750 DB */
	{0x240, 0x1a4}, /* index 484, gain_db=45.375000 DB, again_db=30.000000 DB, dgain_db=15.375000 DB */
	{0x240, 0x1a3}, /* index 483, gain_db=45.281250 DB, again_db=30.000000 DB, dgain_db=15.281250 DB */
	{0x240, 0x1a2}, /* index 482, gain_db=45.187500 DB, again_db=30.000000 DB, dgain_db=15.187500 DB */
	{0x240, 0x1a1}, /* index 481, gain_db=45.093750 DB, again_db=30.000000 DB, dgain_db=15.093750 DB */
	{0x240, 0x1a0}, /* index 480, gain_db=45.000000 DB, again_db=30.000000 DB, dgain_db=15.000000 DB */
	{0x240, 0x19f}, /* index 479, gain_db=44.906250 DB, again_db=30.000000 DB, dgain_db=14.906250 DB */
	{0x240, 0x19e}, /* index 478, gain_db=44.812500 DB, again_db=30.000000 DB, dgain_db=14.812500 DB */
	{0x240, 0x19d}, /* index 477, gain_db=44.718750 DB, again_db=30.000000 DB, dgain_db=14.718750 DB */
	{0x240, 0x19c}, /* index 476, gain_db=44.625000 DB, again_db=30.000000 DB, dgain_db=14.625000 DB */
	{0x240, 0x19b}, /* index 475, gain_db=44.531250 DB, again_db=30.000000 DB, dgain_db=14.531250 DB */
	{0x240, 0x19a}, /* index 474, gain_db=44.437500 DB, again_db=30.000000 DB, dgain_db=14.437500 DB */
	{0x240, 0x199}, /* index 473, gain_db=44.343750 DB, again_db=30.000000 DB, dgain_db=14.343750 DB */
	{0x240, 0x198}, /* index 472, gain_db=44.250000 DB, again_db=30.000000 DB, dgain_db=14.250000 DB */
	{0x240, 0x197}, /* index 471, gain_db=44.156250 DB, again_db=30.000000 DB, dgain_db=14.156250 DB */
	{0x240, 0x196}, /* index 470, gain_db=44.062500 DB, again_db=30.000000 DB, dgain_db=14.062500 DB */
	{0x240, 0x195}, /* index 469, gain_db=43.968750 DB, again_db=30.000000 DB, dgain_db=13.968750 DB */
	{0x240, 0x194}, /* index 468, gain_db=43.875000 DB, again_db=30.000000 DB, dgain_db=13.875000 DB */
	{0x240, 0x193}, /* index 467, gain_db=43.781250 DB, again_db=30.000000 DB, dgain_db=13.781250 DB */
	{0x240, 0x192}, /* index 466, gain_db=43.687500 DB, again_db=30.000000 DB, dgain_db=13.687500 DB */
	{0x240, 0x191}, /* index 465, gain_db=43.593750 DB, again_db=30.000000 DB, dgain_db=13.593750 DB */
	{0x240, 0x190}, /* index 464, gain_db=43.500000 DB, again_db=30.000000 DB, dgain_db=13.500000 DB */
	{0x240, 0x18f}, /* index 463, gain_db=43.406250 DB, again_db=30.000000 DB, dgain_db=13.406250 DB */
	{0x240, 0x18e}, /* index 462, gain_db=43.312500 DB, again_db=30.000000 DB, dgain_db=13.312500 DB */
	{0x240, 0x18d}, /* index 461, gain_db=43.218750 DB, again_db=30.000000 DB, dgain_db=13.218750 DB */
	{0x240, 0x18c}, /* index 460, gain_db=43.125000 DB, again_db=30.000000 DB, dgain_db=13.125000 DB */
	{0x240, 0x18b}, /* index 459, gain_db=43.031250 DB, again_db=30.000000 DB, dgain_db=13.031250 DB */
	{0x240, 0x18a}, /* index 458, gain_db=42.937500 DB, again_db=30.000000 DB, dgain_db=12.937500 DB */
	{0x240, 0x189}, /* index 457, gain_db=42.843750 DB, again_db=30.000000 DB, dgain_db=12.843750 DB */
	{0x240, 0x188}, /* index 456, gain_db=42.750000 DB, again_db=30.000000 DB, dgain_db=12.750000 DB */
	{0x240, 0x187}, /* index 455, gain_db=42.656250 DB, again_db=30.000000 DB, dgain_db=12.656250 DB */
	{0x240, 0x186}, /* index 454, gain_db=42.562500 DB, again_db=30.000000 DB, dgain_db=12.562500 DB */
	{0x240, 0x185}, /* index 453, gain_db=42.468750 DB, again_db=30.000000 DB, dgain_db=12.468750 DB */
	{0x240, 0x184}, /* index 452, gain_db=42.375000 DB, again_db=30.000000 DB, dgain_db=12.375000 DB */
	{0x240, 0x183}, /* index 451, gain_db=42.281250 DB, again_db=30.000000 DB, dgain_db=12.281250 DB */
	{0x240, 0x182}, /* index 450, gain_db=42.187500 DB, again_db=30.000000 DB, dgain_db=12.187500 DB */
	{0x240, 0x181}, /* index 449, gain_db=42.093750 DB, again_db=30.000000 DB, dgain_db=12.093750 DB */
	{0x240, 0x180}, /* index 448, gain_db=42.000000 DB, again_db=30.000000 DB, dgain_db=12.000000 DB */
	{0x240, 0x17f}, /* index 447, gain_db=41.906250 DB, again_db=30.000000 DB, dgain_db=11.906250 DB */
	{0x240, 0x17e}, /* index 446, gain_db=41.812500 DB, again_db=30.000000 DB, dgain_db=11.812500 DB */
	{0x240, 0x17d}, /* index 445, gain_db=41.718750 DB, again_db=30.000000 DB, dgain_db=11.718750 DB */
	{0x240, 0x17c}, /* index 444, gain_db=41.625000 DB, again_db=30.000000 DB, dgain_db=11.625000 DB */
	{0x240, 0x17b}, /* index 443, gain_db=41.531250 DB, again_db=30.000000 DB, dgain_db=11.531250 DB */
	{0x240, 0x17a}, /* index 442, gain_db=41.437500 DB, again_db=30.000000 DB, dgain_db=11.437500 DB */
	{0x240, 0x179}, /* index 441, gain_db=41.343750 DB, again_db=30.000000 DB, dgain_db=11.343750 DB */
	{0x240, 0x178}, /* index 440, gain_db=41.250000 DB, again_db=30.000000 DB, dgain_db=11.250000 DB */
	{0x240, 0x177}, /* index 439, gain_db=41.156250 DB, again_db=30.000000 DB, dgain_db=11.156250 DB */
	{0x240, 0x176}, /* index 438, gain_db=41.062500 DB, again_db=30.000000 DB, dgain_db=11.062500 DB */
	{0x240, 0x175}, /* index 437, gain_db=40.968750 DB, again_db=30.000000 DB, dgain_db=10.968750 DB */
	{0x240, 0x174}, /* index 436, gain_db=40.875000 DB, again_db=30.000000 DB, dgain_db=10.875000 DB */
	{0x240, 0x173}, /* index 435, gain_db=40.781250 DB, again_db=30.000000 DB, dgain_db=10.781250 DB */
	{0x240, 0x172}, /* index 434, gain_db=40.687500 DB, again_db=30.000000 DB, dgain_db=10.687500 DB */
	{0x240, 0x171}, /* index 433, gain_db=40.593750 DB, again_db=30.000000 DB, dgain_db=10.593750 DB */
	{0x240, 0x170}, /* index 432, gain_db=40.500000 DB, again_db=30.000000 DB, dgain_db=10.500000 DB */
	{0x240, 0x16f}, /* index 431, gain_db=40.406250 DB, again_db=30.000000 DB, dgain_db=10.406250 DB */
	{0x240, 0x16e}, /* index 430, gain_db=40.312500 DB, again_db=30.000000 DB, dgain_db=10.312500 DB */
	{0x240, 0x16d}, /* index 429, gain_db=40.218750 DB, again_db=30.000000 DB, dgain_db=10.218750 DB */
	{0x240, 0x16c}, /* index 428, gain_db=40.125000 DB, again_db=30.000000 DB, dgain_db=10.125000 DB */
	{0x240, 0x16b}, /* index 427, gain_db=40.031250 DB, again_db=30.000000 DB, dgain_db=10.031250 DB */
	{0x240, 0x16a}, /* index 426, gain_db=39.937500 DB, again_db=30.000000 DB, dgain_db=9.937500 DB */
	{0x240, 0x169}, /* index 425, gain_db=39.843750 DB, again_db=30.000000 DB, dgain_db=9.843750 DB */
	{0x240, 0x168}, /* index 424, gain_db=39.750000 DB, again_db=30.000000 DB, dgain_db=9.750000 DB */
	{0x240, 0x167}, /* index 423, gain_db=39.656250 DB, again_db=30.000000 DB, dgain_db=9.656250 DB */
	{0x240, 0x166}, /* index 422, gain_db=39.562500 DB, again_db=30.000000 DB, dgain_db=9.562500 DB */
	{0x240, 0x165}, /* index 421, gain_db=39.468750 DB, again_db=30.000000 DB, dgain_db=9.468750 DB */
	{0x240, 0x164}, /* index 420, gain_db=39.375000 DB, again_db=30.000000 DB, dgain_db=9.375000 DB */
	{0x240, 0x163}, /* index 419, gain_db=39.281250 DB, again_db=30.000000 DB, dgain_db=9.281250 DB */
	{0x240, 0x162}, /* index 418, gain_db=39.187500 DB, again_db=30.000000 DB, dgain_db=9.187500 DB */
	{0x240, 0x161}, /* index 417, gain_db=39.093750 DB, again_db=30.000000 DB, dgain_db=9.093750 DB */
	{0x240, 0x160}, /* index 416, gain_db=39.000000 DB, again_db=30.000000 DB, dgain_db=9.000000 DB */
	{0x240, 0x15f}, /* index 415, gain_db=38.906250 DB, again_db=30.000000 DB, dgain_db=8.906250 DB */
	{0x240, 0x15e}, /* index 414, gain_db=38.812500 DB, again_db=30.000000 DB, dgain_db=8.812500 DB */
	{0x240, 0x15d}, /* index 413, gain_db=38.718750 DB, again_db=30.000000 DB, dgain_db=8.718750 DB */
	{0x240, 0x15c}, /* index 412, gain_db=38.625000 DB, again_db=30.000000 DB, dgain_db=8.625000 DB */
	{0x240, 0x15b}, /* index 411, gain_db=38.531250 DB, again_db=30.000000 DB, dgain_db=8.531250 DB */
	{0x240, 0x15a}, /* index 410, gain_db=38.437500 DB, again_db=30.000000 DB, dgain_db=8.437500 DB */
	{0x240, 0x159}, /* index 409, gain_db=38.343750 DB, again_db=30.000000 DB, dgain_db=8.343750 DB */
	{0x240, 0x158}, /* index 408, gain_db=38.250000 DB, again_db=30.000000 DB, dgain_db=8.250000 DB */
	{0x240, 0x157}, /* index 407, gain_db=38.156250 DB, again_db=30.000000 DB, dgain_db=8.156250 DB */
	{0x240, 0x156}, /* index 406, gain_db=38.062500 DB, again_db=30.000000 DB, dgain_db=8.062500 DB */
	{0x240, 0x155}, /* index 405, gain_db=37.968750 DB, again_db=30.000000 DB, dgain_db=7.968750 DB */
	{0x240, 0x154}, /* index 404, gain_db=37.875000 DB, again_db=30.000000 DB, dgain_db=7.875000 DB */
	{0x240, 0x153}, /* index 403, gain_db=37.781250 DB, again_db=30.000000 DB, dgain_db=7.781250 DB */
	{0x240, 0x152}, /* index 402, gain_db=37.687500 DB, again_db=30.000000 DB, dgain_db=7.687500 DB */
	{0x240, 0x151}, /* index 401, gain_db=37.593750 DB, again_db=30.000000 DB, dgain_db=7.593750 DB */
	{0x240, 0x150}, /* index 400, gain_db=37.500000 DB, again_db=30.000000 DB, dgain_db=7.500000 DB */
	{0x240, 0x14f}, /* index 399, gain_db=37.406250 DB, again_db=30.000000 DB, dgain_db=7.406250 DB */
	{0x240, 0x14e}, /* index 398, gain_db=37.312500 DB, again_db=30.000000 DB, dgain_db=7.312500 DB */
	{0x240, 0x14d}, /* index 397, gain_db=37.218750 DB, again_db=30.000000 DB, dgain_db=7.218750 DB */
	{0x240, 0x14c}, /* index 396, gain_db=37.125000 DB, again_db=30.000000 DB, dgain_db=7.125000 DB */
	{0x240, 0x14b}, /* index 395, gain_db=37.031250 DB, again_db=30.000000 DB, dgain_db=7.031250 DB */
	{0x240, 0x14a}, /* index 394, gain_db=36.937500 DB, again_db=30.000000 DB, dgain_db=6.937500 DB */
	{0x240, 0x149}, /* index 393, gain_db=36.843750 DB, again_db=30.000000 DB, dgain_db=6.843750 DB */
	{0x240, 0x148}, /* index 392, gain_db=36.750000 DB, again_db=30.000000 DB, dgain_db=6.750000 DB */
	{0x240, 0x147}, /* index 391, gain_db=36.656250 DB, again_db=30.000000 DB, dgain_db=6.656250 DB */
	{0x240, 0x146}, /* index 390, gain_db=36.562500 DB, again_db=30.000000 DB, dgain_db=6.562500 DB */
	{0x240, 0x145}, /* index 389, gain_db=36.468750 DB, again_db=30.000000 DB, dgain_db=6.468750 DB */
	{0x240, 0x144}, /* index 388, gain_db=36.375000 DB, again_db=30.000000 DB, dgain_db=6.375000 DB */
	{0x240, 0x143}, /* index 387, gain_db=36.281250 DB, again_db=30.000000 DB, dgain_db=6.281250 DB */
	{0x240, 0x142}, /* index 386, gain_db=36.187500 DB, again_db=30.000000 DB, dgain_db=6.187500 DB */
	{0x240, 0x141}, /* index 385, gain_db=36.093750 DB, again_db=30.000000 DB, dgain_db=6.093750 DB */
	{0x240, 0x140}, /* index 384, gain_db=36.000000 DB, again_db=30.000000 DB, dgain_db=6.000000 DB */
	{0x240, 0x13f}, /* index 383, gain_db=35.906250 DB, again_db=30.000000 DB, dgain_db=5.906250 DB */
	{0x240, 0x13e}, /* index 382, gain_db=35.812500 DB, again_db=30.000000 DB, dgain_db=5.812500 DB */
	{0x240, 0x13d}, /* index 381, gain_db=35.718750 DB, again_db=30.000000 DB, dgain_db=5.718750 DB */
	{0x240, 0x13c}, /* index 380, gain_db=35.625000 DB, again_db=30.000000 DB, dgain_db=5.625000 DB */
	{0x240, 0x13b}, /* index 379, gain_db=35.531250 DB, again_db=30.000000 DB, dgain_db=5.531250 DB */
	{0x240, 0x13a}, /* index 378, gain_db=35.437500 DB, again_db=30.000000 DB, dgain_db=5.437500 DB */
	{0x240, 0x139}, /* index 377, gain_db=35.343750 DB, again_db=30.000000 DB, dgain_db=5.343750 DB */
	{0x240, 0x138}, /* index 376, gain_db=35.250000 DB, again_db=30.000000 DB, dgain_db=5.250000 DB */
	{0x240, 0x137}, /* index 375, gain_db=35.156250 DB, again_db=30.000000 DB, dgain_db=5.156250 DB */
	{0x240, 0x136}, /* index 374, gain_db=35.062500 DB, again_db=30.000000 DB, dgain_db=5.062500 DB */
	{0x240, 0x135}, /* index 373, gain_db=34.968750 DB, again_db=30.000000 DB, dgain_db=4.968750 DB */
	{0x240, 0x134}, /* index 372, gain_db=34.875000 DB, again_db=30.000000 DB, dgain_db=4.875000 DB */
	{0x240, 0x133}, /* index 371, gain_db=34.781250 DB, again_db=30.000000 DB, dgain_db=4.781250 DB */
	{0x240, 0x132}, /* index 370, gain_db=34.687500 DB, again_db=30.000000 DB, dgain_db=4.687500 DB */
	{0x240, 0x131}, /* index 369, gain_db=34.593750 DB, again_db=30.000000 DB, dgain_db=4.593750 DB */
	{0x240, 0x130}, /* index 368, gain_db=34.500000 DB, again_db=30.000000 DB, dgain_db=4.500000 DB */
	{0x240, 0x12f}, /* index 367, gain_db=34.406250 DB, again_db=30.000000 DB, dgain_db=4.406250 DB */
	{0x240, 0x12e}, /* index 366, gain_db=34.312500 DB, again_db=30.000000 DB, dgain_db=4.312500 DB */
	{0x240, 0x12d}, /* index 365, gain_db=34.218750 DB, again_db=30.000000 DB, dgain_db=4.218750 DB */
	{0x240, 0x12c}, /* index 364, gain_db=34.125000 DB, again_db=30.000000 DB, dgain_db=4.125000 DB */
	{0x240, 0x12b}, /* index 363, gain_db=34.031250 DB, again_db=30.000000 DB, dgain_db=4.031250 DB */
	{0x240, 0x12a}, /* index 362, gain_db=33.937500 DB, again_db=30.000000 DB, dgain_db=3.937500 DB */
	{0x240, 0x129}, /* index 361, gain_db=33.843750 DB, again_db=30.000000 DB, dgain_db=3.843750 DB */
	{0x240, 0x128}, /* index 360, gain_db=33.750000 DB, again_db=30.000000 DB, dgain_db=3.750000 DB */
	{0x240, 0x127}, /* index 359, gain_db=33.656250 DB, again_db=30.000000 DB, dgain_db=3.656250 DB */
	{0x240, 0x126}, /* index 358, gain_db=33.562500 DB, again_db=30.000000 DB, dgain_db=3.562500 DB */
	{0x240, 0x125}, /* index 357, gain_db=33.468750 DB, again_db=30.000000 DB, dgain_db=3.468750 DB */
	{0x240, 0x124}, /* index 356, gain_db=33.375000 DB, again_db=30.000000 DB, dgain_db=3.375000 DB */
	{0x240, 0x123}, /* index 355, gain_db=33.281250 DB, again_db=30.000000 DB, dgain_db=3.281250 DB */
	{0x240, 0x122}, /* index 354, gain_db=33.187500 DB, again_db=30.000000 DB, dgain_db=3.187500 DB */
	{0x240, 0x121}, /* index 353, gain_db=33.093750 DB, again_db=30.000000 DB, dgain_db=3.093750 DB */
	{0x240, 0x120}, /* index 352, gain_db=33.000000 DB, again_db=30.000000 DB, dgain_db=3.000000 DB */
	{0x240, 0x11f}, /* index 351, gain_db=32.906250 DB, again_db=30.000000 DB, dgain_db=2.906250 DB */
	{0x240, 0x11e}, /* index 350, gain_db=32.812500 DB, again_db=30.000000 DB, dgain_db=2.812500 DB */
	{0x240, 0x11d}, /* index 349, gain_db=32.718750 DB, again_db=30.000000 DB, dgain_db=2.718750 DB */
	{0x240, 0x11c}, /* index 348, gain_db=32.625000 DB, again_db=30.000000 DB, dgain_db=2.625000 DB */
	{0x240, 0x11b}, /* index 347, gain_db=32.531250 DB, again_db=30.000000 DB, dgain_db=2.531250 DB */
	{0x240, 0x11a}, /* index 346, gain_db=32.437500 DB, again_db=30.000000 DB, dgain_db=2.437500 DB */
	{0x240, 0x119}, /* index 345, gain_db=32.343750 DB, again_db=30.000000 DB, dgain_db=2.343750 DB */
	{0x240, 0x118}, /* index 344, gain_db=32.250000 DB, again_db=30.000000 DB, dgain_db=2.250000 DB */
	{0x240, 0x117}, /* index 343, gain_db=32.156250 DB, again_db=30.000000 DB, dgain_db=2.156250 DB */
	{0x240, 0x116}, /* index 342, gain_db=32.062500 DB, again_db=30.000000 DB, dgain_db=2.062500 DB */
	{0x240, 0x115}, /* index 341, gain_db=31.968750 DB, again_db=30.000000 DB, dgain_db=1.968750 DB */
	{0x240, 0x114}, /* index 340, gain_db=31.875000 DB, again_db=30.000000 DB, dgain_db=1.875000 DB */
	{0x240, 0x113}, /* index 339, gain_db=31.781250 DB, again_db=30.000000 DB, dgain_db=1.781250 DB */
	{0x240, 0x112}, /* index 338, gain_db=31.687500 DB, again_db=30.000000 DB, dgain_db=1.687500 DB */
	{0x240, 0x111}, /* index 337, gain_db=31.593750 DB, again_db=30.000000 DB, dgain_db=1.593750 DB */
	{0x240, 0x110}, /* index 336, gain_db=31.500000 DB, again_db=30.000000 DB, dgain_db=1.500000 DB */
	{0x240, 0x10f}, /* index 335, gain_db=31.406250 DB, again_db=30.000000 DB, dgain_db=1.406250 DB */
	{0x240, 0x10e}, /* index 334, gain_db=31.312500 DB, again_db=30.000000 DB, dgain_db=1.312500 DB */
	{0x240, 0x10d}, /* index 333, gain_db=31.218750 DB, again_db=30.000000 DB, dgain_db=1.218750 DB */
	{0x240, 0x10c}, /* index 332, gain_db=31.125000 DB, again_db=30.000000 DB, dgain_db=1.125000 DB */
	{0x240, 0x10b}, /* index 331, gain_db=31.031250 DB, again_db=30.000000 DB, dgain_db=1.031250 DB */
	{0x240, 0x10a}, /* index 330, gain_db=30.937500 DB, again_db=30.000000 DB, dgain_db=0.937500 DB */
	{0x240, 0x109}, /* index 329, gain_db=30.843750 DB, again_db=30.000000 DB, dgain_db=0.843750 DB */
	{0x240, 0x108}, /* index 328, gain_db=30.750000 DB, again_db=30.000000 DB, dgain_db=0.750000 DB */
	{0x240, 0x107}, /* index 327, gain_db=30.656250 DB, again_db=30.000000 DB, dgain_db=0.656250 DB */
	{0x240, 0x106}, /* index 326, gain_db=30.562500 DB, again_db=30.000000 DB, dgain_db=0.562500 DB */
	{0x240, 0x105}, /* index 325, gain_db=30.468750 DB, again_db=30.000000 DB, dgain_db=0.468750 DB */
	{0x240, 0x104}, /* index 324, gain_db=30.375000 DB, again_db=30.000000 DB, dgain_db=0.375000 DB */
	{0x240, 0x103}, /* index 323, gain_db=30.281250 DB, again_db=30.000000 DB, dgain_db=0.281250 DB */
	{0x240, 0x102}, /* index 322, gain_db=30.187500 DB, again_db=30.000000 DB, dgain_db=0.187500 DB */
	{0x240, 0x101}, /* index 321, gain_db=30.093750 DB, again_db=30.000000 DB, dgain_db=0.093750 DB */
	{0x240, 0x100}, /* index 320, gain_db=30.000000 DB, again_db=30.000000 DB, dgain_db=0.000000 DB */

	{0x23f, 0x100}, /* index 319, gain_db=29.906250 DB, again_db=29.906250 DB, dgain_db=0.000000 DB */
	{0x23e, 0x100}, /* index 318, gain_db=29.812500 DB, again_db=29.812500 DB, dgain_db=0.000000 DB */
	{0x23d, 0x100}, /* index 317, gain_db=29.718750 DB, again_db=29.718750 DB, dgain_db=0.000000 DB */
	{0x23c, 0x100}, /* index 316, gain_db=29.625000 DB, again_db=29.625000 DB, dgain_db=0.000000 DB */
	{0x23b, 0x100}, /* index 315, gain_db=29.531250 DB, again_db=29.531250 DB, dgain_db=0.000000 DB */
	{0x23a, 0x100}, /* index 314, gain_db=29.437500 DB, again_db=29.437500 DB, dgain_db=0.000000 DB */
	{0x239, 0x100}, /* index 313, gain_db=29.343750 DB, again_db=29.343750 DB, dgain_db=0.000000 DB */
	{0x238, 0x100}, /* index 312, gain_db=29.250000 DB, again_db=29.250000 DB, dgain_db=0.000000 DB */
	{0x237, 0x100}, /* index 311, gain_db=29.156250 DB, again_db=29.156250 DB, dgain_db=0.000000 DB */
	{0x236, 0x100}, /* index 310, gain_db=29.062500 DB, again_db=29.062500 DB, dgain_db=0.000000 DB */
	{0x235, 0x100}, /* index 309, gain_db=28.968750 DB, again_db=28.968750 DB, dgain_db=0.000000 DB */
	{0x234, 0x100}, /* index 308, gain_db=28.875000 DB, again_db=28.875000 DB, dgain_db=0.000000 DB */
	{0x233, 0x100}, /* index 307, gain_db=28.781250 DB, again_db=28.781250 DB, dgain_db=0.000000 DB */
	{0x232, 0x100}, /* index 306, gain_db=28.687500 DB, again_db=28.687500 DB, dgain_db=0.000000 DB */
	{0x231, 0x100}, /* index 305, gain_db=28.593750 DB, again_db=28.593750 DB, dgain_db=0.000000 DB */
	{0x230, 0x100}, /* index 304, gain_db=28.500000 DB, again_db=28.500000 DB, dgain_db=0.000000 DB */
	{0x22f, 0x100}, /* index 303, gain_db=28.406250 DB, again_db=28.406250 DB, dgain_db=0.000000 DB */
	{0x22e, 0x100}, /* index 302, gain_db=28.312500 DB, again_db=28.312500 DB, dgain_db=0.000000 DB */
	{0x22d, 0x100}, /* index 301, gain_db=28.218750 DB, again_db=28.218750 DB, dgain_db=0.000000 DB */
	{0x22c, 0x100}, /* index 300, gain_db=28.125000 DB, again_db=28.125000 DB, dgain_db=0.000000 DB */
	{0x22b, 0x100}, /* index 299, gain_db=28.031250 DB, again_db=28.031250 DB, dgain_db=0.000000 DB */
	{0x22a, 0x100}, /* index 298, gain_db=27.937500 DB, again_db=27.937500 DB, dgain_db=0.000000 DB */
	{0x229, 0x100}, /* index 297, gain_db=27.843750 DB, again_db=27.843750 DB, dgain_db=0.000000 DB */
	{0x228, 0x100}, /* index 296, gain_db=27.750000 DB, again_db=27.750000 DB, dgain_db=0.000000 DB */
	{0x227, 0x100}, /* index 295, gain_db=27.656250 DB, again_db=27.656250 DB, dgain_db=0.000000 DB */
	{0x226, 0x100}, /* index 294, gain_db=27.562500 DB, again_db=27.562500 DB, dgain_db=0.000000 DB */
	{0x225, 0x100}, /* index 293, gain_db=27.468750 DB, again_db=27.468750 DB, dgain_db=0.000000 DB */
	{0x224, 0x100}, /* index 292, gain_db=27.375000 DB, again_db=27.375000 DB, dgain_db=0.000000 DB */
	{0x223, 0x100}, /* index 291, gain_db=27.281250 DB, again_db=27.281250 DB, dgain_db=0.000000 DB */
	{0x222, 0x100}, /* index 290, gain_db=27.187500 DB, again_db=27.187500 DB, dgain_db=0.000000 DB */
	{0x221, 0x100}, /* index 289, gain_db=27.093750 DB, again_db=27.093750 DB, dgain_db=0.000000 DB */
	{0x220, 0x100}, /* index 288, gain_db=27.000000 DB, again_db=27.000000 DB, dgain_db=0.000000 DB */
	{0x21f, 0x100}, /* index 287, gain_db=26.906250 DB, again_db=26.906250 DB, dgain_db=0.000000 DB */
	{0x21e, 0x100}, /* index 286, gain_db=26.812500 DB, again_db=26.812500 DB, dgain_db=0.000000 DB */
	{0x21d, 0x100}, /* index 285, gain_db=26.718750 DB, again_db=26.718750 DB, dgain_db=0.000000 DB */
	{0x21c, 0x100}, /* index 284, gain_db=26.625000 DB, again_db=26.625000 DB, dgain_db=0.000000 DB */
	{0x21b, 0x100}, /* index 283, gain_db=26.531250 DB, again_db=26.531250 DB, dgain_db=0.000000 DB */
	{0x21a, 0x100}, /* index 282, gain_db=26.437500 DB, again_db=26.437500 DB, dgain_db=0.000000 DB */
	{0x219, 0x100}, /* index 281, gain_db=26.343750 DB, again_db=26.343750 DB, dgain_db=0.000000 DB */
	{0x218, 0x100}, /* index 280, gain_db=26.250000 DB, again_db=26.250000 DB, dgain_db=0.000000 DB */
	{0x217, 0x100}, /* index 279, gain_db=26.156250 DB, again_db=26.156250 DB, dgain_db=0.000000 DB */
	{0x216, 0x100}, /* index 278, gain_db=26.062500 DB, again_db=26.062500 DB, dgain_db=0.000000 DB */
	{0x215, 0x100}, /* index 277, gain_db=25.968750 DB, again_db=25.968750 DB, dgain_db=0.000000 DB */
	{0x214, 0x100}, /* index 276, gain_db=25.875000 DB, again_db=25.875000 DB, dgain_db=0.000000 DB */
	{0x213, 0x100}, /* index 275, gain_db=25.781250 DB, again_db=25.781250 DB, dgain_db=0.000000 DB */
	{0x212, 0x100}, /* index 274, gain_db=25.687500 DB, again_db=25.687500 DB, dgain_db=0.000000 DB */
	{0x211, 0x100}, /* index 273, gain_db=25.593750 DB, again_db=25.593750 DB, dgain_db=0.000000 DB */
	{0x210, 0x100}, /* index 272, gain_db=25.500000 DB, again_db=25.500000 DB, dgain_db=0.000000 DB */
	{0x20f, 0x100}, /* index 271, gain_db=25.406250 DB, again_db=25.406250 DB, dgain_db=0.000000 DB */
	{0x20e, 0x100}, /* index 270, gain_db=25.312500 DB, again_db=25.312500 DB, dgain_db=0.000000 DB */
	{0x20d, 0x100}, /* index 269, gain_db=25.218750 DB, again_db=25.218750 DB, dgain_db=0.000000 DB */
	{0x20c, 0x100}, /* index 268, gain_db=25.125000 DB, again_db=25.125000 DB, dgain_db=0.000000 DB */
	{0x20b, 0x100}, /* index 267, gain_db=25.031250 DB, again_db=25.031250 DB, dgain_db=0.000000 DB */
	{0x20a, 0x100}, /* index 266, gain_db=24.937500 DB, again_db=24.937500 DB, dgain_db=0.000000 DB */
	{0x209, 0x100}, /* index 265, gain_db=24.843750 DB, again_db=24.843750 DB, dgain_db=0.000000 DB */
	{0x208, 0x100}, /* index 264, gain_db=24.750000 DB, again_db=24.750000 DB, dgain_db=0.000000 DB */
	{0x207, 0x100}, /* index 263, gain_db=24.656250 DB, again_db=24.656250 DB, dgain_db=0.000000 DB */
	{0x206, 0x100}, /* index 262, gain_db=24.562500 DB, again_db=24.562500 DB, dgain_db=0.000000 DB */
	{0x205, 0x100}, /* index 261, gain_db=24.468750 DB, again_db=24.468750 DB, dgain_db=0.000000 DB */
	{0x204, 0x100}, /* index 260, gain_db=24.375000 DB, again_db=24.375000 DB, dgain_db=0.000000 DB */
	{0x203, 0x100}, /* index 259, gain_db=24.281250 DB, again_db=24.281250 DB, dgain_db=0.000000 DB */
	{0x202, 0x100}, /* index 258, gain_db=24.187500 DB, again_db=24.187500 DB, dgain_db=0.000000 DB */
	{0x201, 0x100}, /* index 257, gain_db=24.093750 DB, again_db=24.093750 DB, dgain_db=0.000000 DB */
	{0x200, 0x100}, /* index 256, gain_db=24.000000 DB, again_db=24.000000 DB, dgain_db=0.000000 DB */
	{0x1ff, 0x100}, /* index 255, gain_db=23.906250 DB, again_db=23.906250 DB, dgain_db=0.000000 DB */
	{0x1fe, 0x100}, /* index 254, gain_db=23.812500 DB, again_db=23.812500 DB, dgain_db=0.000000 DB */
	{0x1fd, 0x100}, /* index 253, gain_db=23.718750 DB, again_db=23.718750 DB, dgain_db=0.000000 DB */
	{0x1fc, 0x100}, /* index 252, gain_db=23.625000 DB, again_db=23.625000 DB, dgain_db=0.000000 DB */
	{0x1fb, 0x100}, /* index 251, gain_db=23.531250 DB, again_db=23.531250 DB, dgain_db=0.000000 DB */
	{0x1fa, 0x100}, /* index 250, gain_db=23.437500 DB, again_db=23.437500 DB, dgain_db=0.000000 DB */
	{0x1f9, 0x100}, /* index 249, gain_db=23.343750 DB, again_db=23.343750 DB, dgain_db=0.000000 DB */
	{0x1f8, 0x100}, /* index 248, gain_db=23.250000 DB, again_db=23.250000 DB, dgain_db=0.000000 DB */
	{0x1f7, 0x100}, /* index 247, gain_db=23.156250 DB, again_db=23.156250 DB, dgain_db=0.000000 DB */
	{0x1f6, 0x100}, /* index 246, gain_db=23.062500 DB, again_db=23.062500 DB, dgain_db=0.000000 DB */
	{0x1f5, 0x100}, /* index 245, gain_db=22.968750 DB, again_db=22.968750 DB, dgain_db=0.000000 DB */
	{0x1f4, 0x100}, /* index 244, gain_db=22.875000 DB, again_db=22.875000 DB, dgain_db=0.000000 DB */
	{0x1f3, 0x100}, /* index 243, gain_db=22.781250 DB, again_db=22.781250 DB, dgain_db=0.000000 DB */
	{0x1f2, 0x100}, /* index 242, gain_db=22.687500 DB, again_db=22.687500 DB, dgain_db=0.000000 DB */
	{0x1f1, 0x100}, /* index 241, gain_db=22.593750 DB, again_db=22.593750 DB, dgain_db=0.000000 DB */
	{0x1f0, 0x100}, /* index 240, gain_db=22.500000 DB, again_db=22.500000 DB, dgain_db=0.000000 DB */
	{0x1ef, 0x100}, /* index 239, gain_db=22.406250 DB, again_db=22.406250 DB, dgain_db=0.000000 DB */
	{0x1ee, 0x100}, /* index 238, gain_db=22.312500 DB, again_db=22.312500 DB, dgain_db=0.000000 DB */
	{0x1ed, 0x100}, /* index 237, gain_db=22.218750 DB, again_db=22.218750 DB, dgain_db=0.000000 DB */
	{0x1ec, 0x100}, /* index 236, gain_db=22.125000 DB, again_db=22.125000 DB, dgain_db=0.000000 DB */
	{0x1eb, 0x100}, /* index 235, gain_db=22.031250 DB, again_db=22.031250 DB, dgain_db=0.000000 DB */
	{0x1ea, 0x100}, /* index 234, gain_db=21.937500 DB, again_db=21.937500 DB, dgain_db=0.000000 DB */
	{0x1e9, 0x100}, /* index 233, gain_db=21.843750 DB, again_db=21.843750 DB, dgain_db=0.000000 DB */
	{0x1e8, 0x100}, /* index 232, gain_db=21.750000 DB, again_db=21.750000 DB, dgain_db=0.000000 DB */
	{0x1e7, 0x100}, /* index 231, gain_db=21.656250 DB, again_db=21.656250 DB, dgain_db=0.000000 DB */
	{0x1e6, 0x100}, /* index 230, gain_db=21.562500 DB, again_db=21.562500 DB, dgain_db=0.000000 DB */
	{0x1e5, 0x100}, /* index 229, gain_db=21.468750 DB, again_db=21.468750 DB, dgain_db=0.000000 DB */
	{0x1e4, 0x100}, /* index 228, gain_db=21.375000 DB, again_db=21.375000 DB, dgain_db=0.000000 DB */
	{0x1e3, 0x100}, /* index 227, gain_db=21.281250 DB, again_db=21.281250 DB, dgain_db=0.000000 DB */
	{0x1e2, 0x100}, /* index 226, gain_db=21.187500 DB, again_db=21.187500 DB, dgain_db=0.000000 DB */
	{0x1e1, 0x100}, /* index 225, gain_db=21.093750 DB, again_db=21.093750 DB, dgain_db=0.000000 DB */
	{0x1e0, 0x100}, /* index 224, gain_db=21.000000 DB, again_db=21.000000 DB, dgain_db=0.000000 DB */
	{0x1df, 0x100}, /* index 223, gain_db=20.906250 DB, again_db=20.906250 DB, dgain_db=0.000000 DB */
	{0x1de, 0x100}, /* index 222, gain_db=20.812500 DB, again_db=20.812500 DB, dgain_db=0.000000 DB */
	{0x1dd, 0x100}, /* index 221, gain_db=20.718750 DB, again_db=20.718750 DB, dgain_db=0.000000 DB */
	{0x1dc, 0x100}, /* index 220, gain_db=20.625000 DB, again_db=20.625000 DB, dgain_db=0.000000 DB */
	{0x1db, 0x100}, /* index 219, gain_db=20.531250 DB, again_db=20.531250 DB, dgain_db=0.000000 DB */
	{0x1da, 0x100}, /* index 218, gain_db=20.437500 DB, again_db=20.437500 DB, dgain_db=0.000000 DB */
	{0x1d9, 0x100}, /* index 217, gain_db=20.343750 DB, again_db=20.343750 DB, dgain_db=0.000000 DB */
	{0x1d8, 0x100}, /* index 216, gain_db=20.250000 DB, again_db=20.250000 DB, dgain_db=0.000000 DB */
	{0x1d7, 0x100}, /* index 215, gain_db=20.156250 DB, again_db=20.156250 DB, dgain_db=0.000000 DB */
	{0x1d6, 0x100}, /* index 214, gain_db=20.062500 DB, again_db=20.062500 DB, dgain_db=0.000000 DB */
	{0x1d5, 0x100}, /* index 213, gain_db=19.968750 DB, again_db=19.968750 DB, dgain_db=0.000000 DB */
	{0x1d4, 0x100}, /* index 212, gain_db=19.875000 DB, again_db=19.875000 DB, dgain_db=0.000000 DB */
	{0x1d3, 0x100}, /* index 211, gain_db=19.781250 DB, again_db=19.781250 DB, dgain_db=0.000000 DB */
	{0x1d2, 0x100}, /* index 210, gain_db=19.687500 DB, again_db=19.687500 DB, dgain_db=0.000000 DB */
	{0x1d1, 0x100}, /* index 209, gain_db=19.593750 DB, again_db=19.593750 DB, dgain_db=0.000000 DB */
	{0x1d0, 0x100}, /* index 208, gain_db=19.500000 DB, again_db=19.500000 DB, dgain_db=0.000000 DB */
	{0x1cf, 0x100}, /* index 207, gain_db=19.406250 DB, again_db=19.406250 DB, dgain_db=0.000000 DB */
	{0x1ce, 0x100}, /* index 206, gain_db=19.312500 DB, again_db=19.312500 DB, dgain_db=0.000000 DB */
	{0x1cd, 0x100}, /* index 205, gain_db=19.218750 DB, again_db=19.218750 DB, dgain_db=0.000000 DB */
	{0x1cc, 0x100}, /* index 204, gain_db=19.125000 DB, again_db=19.125000 DB, dgain_db=0.000000 DB */
	{0x1cb, 0x100}, /* index 203, gain_db=19.031250 DB, again_db=19.031250 DB, dgain_db=0.000000 DB */
	{0x1ca, 0x100}, /* index 202, gain_db=18.937500 DB, again_db=18.937500 DB, dgain_db=0.000000 DB */
	{0x1c9, 0x100}, /* index 201, gain_db=18.843750 DB, again_db=18.843750 DB, dgain_db=0.000000 DB */
	{0x1c8, 0x100}, /* index 200, gain_db=18.750000 DB, again_db=18.750000 DB, dgain_db=0.000000 DB */
	{0x1c7, 0x100}, /* index 199, gain_db=18.656250 DB, again_db=18.656250 DB, dgain_db=0.000000 DB */
	{0x1c6, 0x100}, /* index 198, gain_db=18.562500 DB, again_db=18.562500 DB, dgain_db=0.000000 DB */
	{0x1c5, 0x100}, /* index 197, gain_db=18.468750 DB, again_db=18.468750 DB, dgain_db=0.000000 DB */
	{0x1c4, 0x100}, /* index 196, gain_db=18.375000 DB, again_db=18.375000 DB, dgain_db=0.000000 DB */
	{0x1c3, 0x100}, /* index 195, gain_db=18.281250 DB, again_db=18.281250 DB, dgain_db=0.000000 DB */
	{0x1c2, 0x100}, /* index 194, gain_db=18.187500 DB, again_db=18.187500 DB, dgain_db=0.000000 DB */
	{0x1c1, 0x100}, /* index 193, gain_db=18.093750 DB, again_db=18.093750 DB, dgain_db=0.000000 DB */
	{0x1c0, 0x100}, /* index 192, gain_db=18.000000 DB, again_db=18.000000 DB, dgain_db=0.000000 DB */
	{0x1bf, 0x100}, /* index 191, gain_db=17.906250 DB, again_db=17.906250 DB, dgain_db=0.000000 DB */
	{0x1be, 0x100}, /* index 190, gain_db=17.812500 DB, again_db=17.812500 DB, dgain_db=0.000000 DB */
	{0x1bd, 0x100}, /* index 189, gain_db=17.718750 DB, again_db=17.718750 DB, dgain_db=0.000000 DB */
	{0x1bc, 0x100}, /* index 188, gain_db=17.625000 DB, again_db=17.625000 DB, dgain_db=0.000000 DB */
	{0x1bb, 0x100}, /* index 187, gain_db=17.531250 DB, again_db=17.531250 DB, dgain_db=0.000000 DB */
	{0x1ba, 0x100}, /* index 186, gain_db=17.437500 DB, again_db=17.437500 DB, dgain_db=0.000000 DB */
	{0x1b9, 0x100}, /* index 185, gain_db=17.343750 DB, again_db=17.343750 DB, dgain_db=0.000000 DB */
	{0x1b8, 0x100}, /* index 184, gain_db=17.250000 DB, again_db=17.250000 DB, dgain_db=0.000000 DB */
	{0x1b7, 0x100}, /* index 183, gain_db=17.156250 DB, again_db=17.156250 DB, dgain_db=0.000000 DB */
	{0x1b6, 0x100}, /* index 182, gain_db=17.062500 DB, again_db=17.062500 DB, dgain_db=0.000000 DB */
	{0x1b5, 0x100}, /* index 181, gain_db=16.968750 DB, again_db=16.968750 DB, dgain_db=0.000000 DB */
	{0x1b4, 0x100}, /* index 180, gain_db=16.875000 DB, again_db=16.875000 DB, dgain_db=0.000000 DB */
	{0x1b3, 0x100}, /* index 179, gain_db=16.781250 DB, again_db=16.781250 DB, dgain_db=0.000000 DB */
	{0x1b2, 0x100}, /* index 178, gain_db=16.687500 DB, again_db=16.687500 DB, dgain_db=0.000000 DB */
	{0x1b1, 0x100}, /* index 177, gain_db=16.593750 DB, again_db=16.593750 DB, dgain_db=0.000000 DB */
	{0x1b0, 0x100}, /* index 176, gain_db=16.500000 DB, again_db=16.500000 DB, dgain_db=0.000000 DB */
	{0x1af, 0x100}, /* index 175, gain_db=16.406250 DB, again_db=16.406250 DB, dgain_db=0.000000 DB */
	{0x1ae, 0x100}, /* index 174, gain_db=16.312500 DB, again_db=16.312500 DB, dgain_db=0.000000 DB */
	{0x1ad, 0x100}, /* index 173, gain_db=16.218750 DB, again_db=16.218750 DB, dgain_db=0.000000 DB */
	{0x1ac, 0x100}, /* index 172, gain_db=16.125000 DB, again_db=16.125000 DB, dgain_db=0.000000 DB */
	{0x1ab, 0x100}, /* index 171, gain_db=16.031250 DB, again_db=16.031250 DB, dgain_db=0.000000 DB */
	{0x1aa, 0x100}, /* index 170, gain_db=15.937500 DB, again_db=15.937500 DB, dgain_db=0.000000 DB */
	{0x1a9, 0x100}, /* index 169, gain_db=15.843750 DB, again_db=15.843750 DB, dgain_db=0.000000 DB */
	{0x1a8, 0x100}, /* index 168, gain_db=15.750000 DB, again_db=15.750000 DB, dgain_db=0.000000 DB */
	{0x1a7, 0x100}, /* index 167, gain_db=15.656250 DB, again_db=15.656250 DB, dgain_db=0.000000 DB */
	{0x1a6, 0x100}, /* index 166, gain_db=15.562500 DB, again_db=15.562500 DB, dgain_db=0.000000 DB */
	{0x1a5, 0x100}, /* index 165, gain_db=15.468750 DB, again_db=15.468750 DB, dgain_db=0.000000 DB */
	{0x1a4, 0x100}, /* index 164, gain_db=15.375000 DB, again_db=15.375000 DB, dgain_db=0.000000 DB */
	{0x1a3, 0x100}, /* index 163, gain_db=15.281250 DB, again_db=15.281250 DB, dgain_db=0.000000 DB */
	{0x1a2, 0x100}, /* index 162, gain_db=15.187500 DB, again_db=15.187500 DB, dgain_db=0.000000 DB */
	{0x1a1, 0x100}, /* index 161, gain_db=15.093750 DB, again_db=15.093750 DB, dgain_db=0.000000 DB */
	{0x1a0, 0x100}, /* index 160, gain_db=15.000000 DB, again_db=15.000000 DB, dgain_db=0.000000 DB */
	{0x19f, 0x100}, /* index 159, gain_db=14.906250 DB, again_db=14.906250 DB, dgain_db=0.000000 DB */
	{0x19e, 0x100}, /* index 158, gain_db=14.812500 DB, again_db=14.812500 DB, dgain_db=0.000000 DB */
	{0x19d, 0x100}, /* index 157, gain_db=14.718750 DB, again_db=14.718750 DB, dgain_db=0.000000 DB */
	{0x19c, 0x100}, /* index 156, gain_db=14.625000 DB, again_db=14.625000 DB, dgain_db=0.000000 DB */
	{0x19b, 0x100}, /* index 155, gain_db=14.531250 DB, again_db=14.531250 DB, dgain_db=0.000000 DB */
	{0x19a, 0x100}, /* index 154, gain_db=14.437500 DB, again_db=14.437500 DB, dgain_db=0.000000 DB */
	{0x199, 0x100}, /* index 153, gain_db=14.343750 DB, again_db=14.343750 DB, dgain_db=0.000000 DB */
	{0x198, 0x100}, /* index 152, gain_db=14.250000 DB, again_db=14.250000 DB, dgain_db=0.000000 DB */
	{0x197, 0x100}, /* index 151, gain_db=14.156250 DB, again_db=14.156250 DB, dgain_db=0.000000 DB */
	{0x196, 0x100}, /* index 150, gain_db=14.062500 DB, again_db=14.062500 DB, dgain_db=0.000000 DB */
	{0x195, 0x100}, /* index 149, gain_db=13.968750 DB, again_db=13.968750 DB, dgain_db=0.000000 DB */
	{0x194, 0x100}, /* index 148, gain_db=13.875000 DB, again_db=13.875000 DB, dgain_db=0.000000 DB */
	{0x193, 0x100}, /* index 147, gain_db=13.781250 DB, again_db=13.781250 DB, dgain_db=0.000000 DB */
	{0x192, 0x100}, /* index 146, gain_db=13.687500 DB, again_db=13.687500 DB, dgain_db=0.000000 DB */
	{0x191, 0x100}, /* index 145, gain_db=13.593750 DB, again_db=13.593750 DB, dgain_db=0.000000 DB */
	{0x190, 0x100}, /* index 144, gain_db=13.500000 DB, again_db=13.500000 DB, dgain_db=0.000000 DB */
	{0x18f, 0x100}, /* index 143, gain_db=13.406250 DB, again_db=13.406250 DB, dgain_db=0.000000 DB */
	{0x18e, 0x100}, /* index 142, gain_db=13.312500 DB, again_db=13.312500 DB, dgain_db=0.000000 DB */
	{0x18d, 0x100}, /* index 141, gain_db=13.218750 DB, again_db=13.218750 DB, dgain_db=0.000000 DB */
	{0x18c, 0x100}, /* index 140, gain_db=13.125000 DB, again_db=13.125000 DB, dgain_db=0.000000 DB */
	{0x18b, 0x100}, /* index 139, gain_db=13.031250 DB, again_db=13.031250 DB, dgain_db=0.000000 DB */
	{0x18a, 0x100}, /* index 138, gain_db=12.937500 DB, again_db=12.937500 DB, dgain_db=0.000000 DB */
	{0x189, 0x100}, /* index 137, gain_db=12.843750 DB, again_db=12.843750 DB, dgain_db=0.000000 DB */
	{0x188, 0x100}, /* index 136, gain_db=12.750000 DB, again_db=12.750000 DB, dgain_db=0.000000 DB */
	{0x187, 0x100}, /* index 135, gain_db=12.656250 DB, again_db=12.656250 DB, dgain_db=0.000000 DB */
	{0x186, 0x100}, /* index 134, gain_db=12.562500 DB, again_db=12.562500 DB, dgain_db=0.000000 DB */
	{0x185, 0x100}, /* index 133, gain_db=12.468750 DB, again_db=12.468750 DB, dgain_db=0.000000 DB */
	{0x184, 0x100}, /* index 132, gain_db=12.375000 DB, again_db=12.375000 DB, dgain_db=0.000000 DB */
	{0x183, 0x100}, /* index 131, gain_db=12.281250 DB, again_db=12.281250 DB, dgain_db=0.000000 DB */
	{0x182, 0x100}, /* index 130, gain_db=12.187500 DB, again_db=12.187500 DB, dgain_db=0.000000 DB */
	{0x181, 0x100}, /* index 129, gain_db=12.093750 DB, again_db=12.093750 DB, dgain_db=0.000000 DB */
	{0x180, 0x100}, /* index 128, gain_db=12.000000 DB, again_db=12.000000 DB, dgain_db=0.000000 DB */
	{0x17f, 0x100}, /* index 127, gain_db=11.906250 DB, again_db=11.906250 DB, dgain_db=0.000000 DB */
	{0x17e, 0x100}, /* index 126, gain_db=11.812500 DB, again_db=11.812500 DB, dgain_db=0.000000 DB */
	{0x17d, 0x100}, /* index 125, gain_db=11.718750 DB, again_db=11.718750 DB, dgain_db=0.000000 DB */
	{0x17c, 0x100}, /* index 124, gain_db=11.625000 DB, again_db=11.625000 DB, dgain_db=0.000000 DB */
	{0x17b, 0x100}, /* index 123, gain_db=11.531250 DB, again_db=11.531250 DB, dgain_db=0.000000 DB */
	{0x17a, 0x100}, /* index 122, gain_db=11.437500 DB, again_db=11.437500 DB, dgain_db=0.000000 DB */
	{0x179, 0x100}, /* index 121, gain_db=11.343750 DB, again_db=11.343750 DB, dgain_db=0.000000 DB */
	{0x178, 0x100}, /* index 120, gain_db=11.250000 DB, again_db=11.250000 DB, dgain_db=0.000000 DB */
	{0x177, 0x100}, /* index 119, gain_db=11.156250 DB, again_db=11.156250 DB, dgain_db=0.000000 DB */
	{0x176, 0x100}, /* index 118, gain_db=11.062500 DB, again_db=11.062500 DB, dgain_db=0.000000 DB */
	{0x175, 0x100}, /* index 117, gain_db=10.968750 DB, again_db=10.968750 DB, dgain_db=0.000000 DB */
	{0x174, 0x100}, /* index 116, gain_db=10.875000 DB, again_db=10.875000 DB, dgain_db=0.000000 DB */
	{0x173, 0x100}, /* index 115, gain_db=10.781250 DB, again_db=10.781250 DB, dgain_db=0.000000 DB */
	{0x172, 0x100}, /* index 114, gain_db=10.687500 DB, again_db=10.687500 DB, dgain_db=0.000000 DB */
	{0x171, 0x100}, /* index 113, gain_db=10.593750 DB, again_db=10.593750 DB, dgain_db=0.000000 DB */
	{0x170, 0x100}, /* index 112, gain_db=10.500000 DB, again_db=10.500000 DB, dgain_db=0.000000 DB */
	{0x16f, 0x100}, /* index 111, gain_db=10.406250 DB, again_db=10.406250 DB, dgain_db=0.000000 DB */
	{0x16e, 0x100}, /* index 110, gain_db=10.312500 DB, again_db=10.312500 DB, dgain_db=0.000000 DB */
	{0x16d, 0x100}, /* index 109, gain_db=10.218750 DB, again_db=10.218750 DB, dgain_db=0.000000 DB */
	{0x16c, 0x100}, /* index 108, gain_db=10.125000 DB, again_db=10.125000 DB, dgain_db=0.000000 DB */
	{0x16b, 0x100}, /* index 107, gain_db=10.031250 DB, again_db=10.031250 DB, dgain_db=0.000000 DB */
	{0x16a, 0x100}, /* index 106, gain_db=9.937500 DB, again_db=9.937500 DB, dgain_db=0.000000 DB */
	{0x169, 0x100}, /* index 105, gain_db=9.843750 DB, again_db=9.843750 DB, dgain_db=0.000000 DB */
	{0x168, 0x100}, /* index 104, gain_db=9.750000 DB, again_db=9.750000 DB, dgain_db=0.000000 DB */
	{0x167, 0x100}, /* index 103, gain_db=9.656250 DB, again_db=9.656250 DB, dgain_db=0.000000 DB */
	{0x166, 0x100}, /* index 102, gain_db=9.562500 DB, again_db=9.562500 DB, dgain_db=0.000000 DB */
	{0x165, 0x100}, /* index 101, gain_db=9.468750 DB, again_db=9.468750 DB, dgain_db=0.000000 DB */
	{0x164, 0x100}, /* index 100, gain_db=9.375000 DB, again_db=9.375000 DB, dgain_db=0.000000 DB */
	{0x163, 0x100}, /* index 99, gain_db=9.281250 DB, again_db=9.281250 DB, dgain_db=0.000000 DB */
	{0x162, 0x100}, /* index 98, gain_db=9.187500 DB, again_db=9.187500 DB, dgain_db=0.000000 DB */
	{0x161, 0x100}, /* index 97, gain_db=9.093750 DB, again_db=9.093750 DB, dgain_db=0.000000 DB */
	{0x160, 0x100}, /* index 96, gain_db=9.000000 DB, again_db=9.000000 DB, dgain_db=0.000000 DB */
	{0x15f, 0x100}, /* index 95, gain_db=8.906250 DB, again_db=8.906250 DB, dgain_db=0.000000 DB */
	{0x15e, 0x100}, /* index 94, gain_db=8.812500 DB, again_db=8.812500 DB, dgain_db=0.000000 DB */
	{0x15d, 0x100}, /* index 93, gain_db=8.718750 DB, again_db=8.718750 DB, dgain_db=0.000000 DB */
	{0x15c, 0x100}, /* index 92, gain_db=8.625000 DB, again_db=8.625000 DB, dgain_db=0.000000 DB */
	{0x15b, 0x100}, /* index 91, gain_db=8.531250 DB, again_db=8.531250 DB, dgain_db=0.000000 DB */
	{0x15a, 0x100}, /* index 90, gain_db=8.437500 DB, again_db=8.437500 DB, dgain_db=0.000000 DB */
	{0x159, 0x100}, /* index 89, gain_db=8.343750 DB, again_db=8.343750 DB, dgain_db=0.000000 DB */
	{0x158, 0x100}, /* index 88, gain_db=8.250000 DB, again_db=8.250000 DB, dgain_db=0.000000 DB */
	{0x157, 0x100}, /* index 87, gain_db=8.156250 DB, again_db=8.156250 DB, dgain_db=0.000000 DB */
	{0x156, 0x100}, /* index 86, gain_db=8.062500 DB, again_db=8.062500 DB, dgain_db=0.000000 DB */
	{0x155, 0x100}, /* index 85, gain_db=7.968750 DB, again_db=7.968750 DB, dgain_db=0.000000 DB */
	{0x154, 0x100}, /* index 84, gain_db=7.875000 DB, again_db=7.875000 DB, dgain_db=0.000000 DB */
	{0x153, 0x100}, /* index 83, gain_db=7.781250 DB, again_db=7.781250 DB, dgain_db=0.000000 DB */
	{0x152, 0x100}, /* index 82, gain_db=7.687500 DB, again_db=7.687500 DB, dgain_db=0.000000 DB */
	{0x151, 0x100}, /* index 81, gain_db=7.593750 DB, again_db=7.593750 DB, dgain_db=0.000000 DB */
	{0x150, 0x100}, /* index 80, gain_db=7.500000 DB, again_db=7.500000 DB, dgain_db=0.000000 DB */
	{0x14f, 0x100}, /* index 79, gain_db=7.406250 DB, again_db=7.406250 DB, dgain_db=0.000000 DB */
	{0x14e, 0x100}, /* index 78, gain_db=7.312500 DB, again_db=7.312500 DB, dgain_db=0.000000 DB */
	{0x14d, 0x100}, /* index 77, gain_db=7.218750 DB, again_db=7.218750 DB, dgain_db=0.000000 DB */
	{0x14c, 0x100}, /* index 76, gain_db=7.125000 DB, again_db=7.125000 DB, dgain_db=0.000000 DB */
	{0x14b, 0x100}, /* index 75, gain_db=7.031250 DB, again_db=7.031250 DB, dgain_db=0.000000 DB */
	{0x14a, 0x100}, /* index 74, gain_db=6.937500 DB, again_db=6.937500 DB, dgain_db=0.000000 DB */
	{0x149, 0x100}, /* index 73, gain_db=6.843750 DB, again_db=6.843750 DB, dgain_db=0.000000 DB */
	{0x148, 0x100}, /* index 72, gain_db=6.750000 DB, again_db=6.750000 DB, dgain_db=0.000000 DB */
	{0x147, 0x100}, /* index 71, gain_db=6.656250 DB, again_db=6.656250 DB, dgain_db=0.000000 DB */
	{0x146, 0x100}, /* index 70, gain_db=6.562500 DB, again_db=6.562500 DB, dgain_db=0.000000 DB */
	{0x145, 0x100}, /* index 69, gain_db=6.468750 DB, again_db=6.468750 DB, dgain_db=0.000000 DB */
	{0x144, 0x100}, /* index 68, gain_db=6.375000 DB, again_db=6.375000 DB, dgain_db=0.000000 DB */
	{0x143, 0x100}, /* index 67, gain_db=6.281250 DB, again_db=6.281250 DB, dgain_db=0.000000 DB */
	{0x142, 0x100}, /* index 66, gain_db=6.187500 DB, again_db=6.187500 DB, dgain_db=0.000000 DB */
	{0x141, 0x100}, /* index 65, gain_db=6.093750 DB, again_db=6.093750 DB, dgain_db=0.000000 DB */
	{0x140, 0x100}, /* index 64, gain_db=6.000000 DB, again_db=6.000000 DB, dgain_db=0.000000 DB */
	{0x13f, 0x100}, /* index 63, gain_db=5.906250 DB, again_db=5.906250 DB, dgain_db=0.000000 DB */
	{0x13e, 0x100}, /* index 62, gain_db=5.812500 DB, again_db=5.812500 DB, dgain_db=0.000000 DB */
	{0x13d, 0x100}, /* index 61, gain_db=5.718750 DB, again_db=5.718750 DB, dgain_db=0.000000 DB */
	{0x13c, 0x100}, /* index 60, gain_db=5.625000 DB, again_db=5.625000 DB, dgain_db=0.000000 DB */
	{0x13b, 0x100}, /* index 59, gain_db=5.531250 DB, again_db=5.531250 DB, dgain_db=0.000000 DB */
	{0x13a, 0x100}, /* index 58, gain_db=5.437500 DB, again_db=5.437500 DB, dgain_db=0.000000 DB */
	{0x139, 0x100}, /* index 57, gain_db=5.343750 DB, again_db=5.343750 DB, dgain_db=0.000000 DB */
	{0x138, 0x100}, /* index 56, gain_db=5.250000 DB, again_db=5.250000 DB, dgain_db=0.000000 DB */
	{0x137, 0x100}, /* index 55, gain_db=5.156250 DB, again_db=5.156250 DB, dgain_db=0.000000 DB */
	{0x136, 0x100}, /* index 54, gain_db=5.062500 DB, again_db=5.062500 DB, dgain_db=0.000000 DB */
	{0x135, 0x100}, /* index 53, gain_db=4.968750 DB, again_db=4.968750 DB, dgain_db=0.000000 DB */
	{0x134, 0x100}, /* index 52, gain_db=4.875000 DB, again_db=4.875000 DB, dgain_db=0.000000 DB */
	{0x133, 0x100}, /* index 51, gain_db=4.781250 DB, again_db=4.781250 DB, dgain_db=0.000000 DB */
	{0x132, 0x100}, /* index 50, gain_db=4.687500 DB, again_db=4.687500 DB, dgain_db=0.000000 DB */
	{0x131, 0x100}, /* index 49, gain_db=4.593750 DB, again_db=4.593750 DB, dgain_db=0.000000 DB */
	{0x130, 0x100}, /* index 48, gain_db=4.500000 DB, again_db=4.500000 DB, dgain_db=0.000000 DB */
	{0x12f, 0x100}, /* index 47, gain_db=4.406250 DB, again_db=4.406250 DB, dgain_db=0.000000 DB */
	{0x12e, 0x100}, /* index 46, gain_db=4.312500 DB, again_db=4.312500 DB, dgain_db=0.000000 DB */
	{0x12d, 0x100}, /* index 45, gain_db=4.218750 DB, again_db=4.218750 DB, dgain_db=0.000000 DB */
	{0x12c, 0x100}, /* index 44, gain_db=4.125000 DB, again_db=4.125000 DB, dgain_db=0.000000 DB */
	{0x12b, 0x100}, /* index 43, gain_db=4.031250 DB, again_db=4.031250 DB, dgain_db=0.000000 DB */
	{0x12a, 0x100}, /* index 42, gain_db=3.937500 DB, again_db=3.937500 DB, dgain_db=0.000000 DB */
	{0x129, 0x100}, /* index 41, gain_db=3.843750 DB, again_db=3.843750 DB, dgain_db=0.000000 DB */
	{0x128, 0x100}, /* index 40, gain_db=3.750000 DB, again_db=3.750000 DB, dgain_db=0.000000 DB */
	{0x127, 0x100}, /* index 39, gain_db=3.656250 DB, again_db=3.656250 DB, dgain_db=0.000000 DB */
	{0x126, 0x100}, /* index 38, gain_db=3.562500 DB, again_db=3.562500 DB, dgain_db=0.000000 DB */
	{0x125, 0x100}, /* index 37, gain_db=3.468750 DB, again_db=3.468750 DB, dgain_db=0.000000 DB */
	{0x124, 0x100}, /* index 36, gain_db=3.375000 DB, again_db=3.375000 DB, dgain_db=0.000000 DB */
	{0x123, 0x100}, /* index 35, gain_db=3.281250 DB, again_db=3.281250 DB, dgain_db=0.000000 DB */
	{0x122, 0x100}, /* index 34, gain_db=3.187500 DB, again_db=3.187500 DB, dgain_db=0.000000 DB */
	{0x121, 0x100}, /* index 33, gain_db=3.093750 DB, again_db=3.093750 DB, dgain_db=0.000000 DB */
	{0x120, 0x100}, /* index 32, gain_db=3.000000 DB, again_db=3.000000 DB, dgain_db=0.000000 DB */
	{0x11f, 0x100}, /* index 31, gain_db=2.906250 DB, again_db=2.906250 DB, dgain_db=0.000000 DB */
	{0x11e, 0x100}, /* index 30, gain_db=2.812500 DB, again_db=2.812500 DB, dgain_db=0.000000 DB */
	{0x11d, 0x100}, /* index 29, gain_db=2.718750 DB, again_db=2.718750 DB, dgain_db=0.000000 DB */
	{0x11c, 0x100}, /* index 28, gain_db=2.625000 DB, again_db=2.625000 DB, dgain_db=0.000000 DB */
	{0x11b, 0x100}, /* index 27, gain_db=2.531250 DB, again_db=2.531250 DB, dgain_db=0.000000 DB */
	{0x11a, 0x100}, /* index 26, gain_db=2.437500 DB, again_db=2.437500 DB, dgain_db=0.000000 DB */
	{0x119, 0x100}, /* index 25, gain_db=2.343750 DB, again_db=2.343750 DB, dgain_db=0.000000 DB */
	{0x118, 0x100}, /* index 24, gain_db=2.250000 DB, again_db=2.250000 DB, dgain_db=0.000000 DB */
	{0x117, 0x100}, /* index 23, gain_db=2.156250 DB, again_db=2.156250 DB, dgain_db=0.000000 DB */
	{0x116, 0x100}, /* index 22, gain_db=2.062500 DB, again_db=2.062500 DB, dgain_db=0.000000 DB */
	{0x115, 0x100}, /* index 21, gain_db=1.968750 DB, again_db=1.968750 DB, dgain_db=0.000000 DB */
	{0x114, 0x100}, /* index 20, gain_db=1.875000 DB, again_db=1.875000 DB, dgain_db=0.000000 DB */
	{0x113, 0x100}, /* index 19, gain_db=1.781250 DB, again_db=1.781250 DB, dgain_db=0.000000 DB */
	{0x112, 0x100}, /* index 18, gain_db=1.687500 DB, again_db=1.687500 DB, dgain_db=0.000000 DB */
	{0x111, 0x100}, /* index 17, gain_db=1.593750 DB, again_db=1.593750 DB, dgain_db=0.000000 DB */
	{0x110, 0x100}, /* index 16, gain_db=1.500000 DB, again_db=1.500000 DB, dgain_db=0.000000 DB */
	{0x10f, 0x100}, /* index 15, gain_db=1.406250 DB, again_db=1.406250 DB, dgain_db=0.000000 DB */
	{0x10e, 0x100}, /* index 14, gain_db=1.312500 DB, again_db=1.312500 DB, dgain_db=0.000000 DB */
	{0x10d, 0x100}, /* index 13, gain_db=1.218750 DB, again_db=1.218750 DB, dgain_db=0.000000 DB */
	{0x10c, 0x100}, /* index 12, gain_db=1.125000 DB, again_db=1.125000 DB, dgain_db=0.000000 DB */
	{0x10b, 0x100}, /* index 11, gain_db=1.031250 DB, again_db=1.031250 DB, dgain_db=0.000000 DB */
	{0x10a, 0x100}, /* index 10, gain_db=0.937500 DB, again_db=0.937500 DB, dgain_db=0.000000 DB */
	{0x109, 0x100}, /* index 9, gain_db=0.843750 DB, again_db=0.843750 DB, dgain_db=0.000000 DB */
	{0x108, 0x100}, /* index 8, gain_db=0.750000 DB, again_db=0.750000 DB, dgain_db=0.000000 DB */
	{0x107, 0x100}, /* index 7, gain_db=0.656250 DB, again_db=0.656250 DB, dgain_db=0.000000 DB */
	{0x106, 0x100}, /* index 6, gain_db=0.562500 DB, again_db=0.562500 DB, dgain_db=0.000000 DB */
	{0x105, 0x100}, /* index 5, gain_db=0.468750 DB, again_db=0.468750 DB, dgain_db=0.000000 DB */
	{0x104, 0x100}, /* index 4, gain_db=0.375000 DB, again_db=0.375000 DB, dgain_db=0.000000 DB */
	{0x103, 0x100}, /* index 3, gain_db=0.281250 DB, again_db=0.281250 DB, dgain_db=0.000000 DB */
	{0x102, 0x100}, /* index 2, gain_db=0.187500 DB, again_db=0.187500 DB, dgain_db=0.000000 DB */
	{0x101, 0x100}, /* index 1, gain_db=0.093750 DB, again_db=0.093750 DB, dgain_db=0.000000 DB */
	{0x100, 0x100}, /* index 0, gain_db=0.000000 DB, again_db=0.000000 DB, dgain_db=0.000000 DB */
};
#endif
