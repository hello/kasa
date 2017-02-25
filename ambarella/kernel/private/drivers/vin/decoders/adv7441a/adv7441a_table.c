/*
 * Filename : adv7441a_table.c
 *
 * History:
 *    2011/01/12 - [Haowei Lo] Create
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


static struct vin_video_pll adv7441a_plls[] = {
	{0, 1485000000, 1485000000},
};


 static struct vin_video_format adv7441a_formats[] = {
 	{
		.video_mode	= AMBA_VIDEO_MODE_AUTO,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 0,
		.def_height	= 0,
		/* sensor mode */
		.device_mode	= 0,
		.pll_idx		= 0,
		.width		= 0,
		.height		= 0,
		.format		= 0,
		.type		= 0,
		.bits			= 0,
		.ratio		= 0,
		.max_fps		= 0,
		.default_fps	= 0,
 	},
};

 static const struct adv7441a_reg_table adv7441a_cvbs_share_regs[] = {
	{USER_MAP, 0x00, 0x03},
	{USER_MAP, 0x03, 0x0c},
	{USER_MAP, 0x3c, 0xad},
	{USER_MAP, 0x04, 0x47},
	{USER_MAP, 0x17, 0x41},
	{USER_MAP, 0x1d, 0x40},

	{USER_MAP, 0x31, 0x1a},
	{USER_MAP, 0x32, 0x81},
	{USER_MAP, 0x33, 0x84},
	{USER_MAP, 0x34, 0x00},
	{USER_MAP, 0x35, 0x00},
	{USER_MAP, 0x36, 0x7d},
	{USER_MAP, 0x37, 0xa1},

	{USER_MAP, 0x3a, 0x07},
	{USER_MAP, 0x3c, 0xa8},
	{USER_MAP, 0x47, 0x0a},
	{USER_MAP, 0xf3, 0x07},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_svideo_share_regs[] = {
	{USER_MAP, 0x03, 0x0c},
	{USER_MAP, 0x04, 0x57},
	{USER_MAP, 0x1d, 0x47},
	{USER_MAP, 0x0f, 0x40},
	{USER_MAP, 0xe8, 0x65},
	{USER_MAP, 0xea, 0x63},
	{USER_MAP, 0x3a, 0x13},
	{USER_MAP, 0x3d, 0xa2},
	{USER_MAP, 0x3e, 0x64},
	{USER_MAP, 0x3f, 0xe4},	/* 10 */
	{USER_MAP, 0x69, 0x03},	/* Y = AIN11, CVBS = AIN11 */
	{USER_MAP, 0xf3, 0x03},
	{USER_MAP, 0xf9, 0x03},
	{USER_MAP, 0x0e, 0x80},
	{USER_MAP, 0x81, 0x30},
	{USER_MAP, 0x90, 0xc9},
	{USER_MAP, 0x91, 0x40},
	{USER_MAP, 0x92, 0x3c},
	{USER_MAP, 0x93, 0xca},
	{USER_MAP, 0x94, 0xd5},	/* 20 */
	{USER_MAP, 0xb1, 0xff},
	{USER_MAP, 0xb6, 0x08},
	{USER_MAP, 0xc0, 0x9a},
	{USER_MAP, 0xcf, 0x50},
	{USER_MAP, 0xd0, 0x4e},
	{USER_MAP, 0xd6, 0xdd},
	{USER_MAP, 0xd7, 0xe2},
	{USER_MAP, 0xe5, 0x51},
	{USER_MAP, 0x0e, 0x00},	/* 30 */
	{USER_MAP, 0x10, 0xff},

	{USER_MAP, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_sdp_480i_fix_regs[] = {
	{USER_MAP, 0xe5, 0x41},
	{USER_MAP, 0xe6, 0x84},
	{USER_MAP, 0xe7, 0x06},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_sdp_576i_fix_regs[] = {
	{USER_MAP, 0xe8, 0x41},
	{USER_MAP, 0xe9, 0x84},
	{USER_MAP, 0xea, 0x06},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_ypbpr_share_regs[] = {
	{USER_MAP, 0x00, 0x09},
	{USER_MAP, 0x1d, 0x40},
	{USER_MAP, 0x3c, 0xa8},
	{USER_MAP, 0x47, 0x0a},
#if defined(ADV7441A_PREFER_EMBMODE)
	{USER_MAP, 0x6b, 0xd3},
#else
	{USER_MAP, 0x6b, 0xc3},
#endif
	{USER_MAP, 0x85, 0x19},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_ypbpr_480i_fix_regs[] = {
#if defined(ADV7441A_PREFER_EMBMODE)
	{USER_MAP, 0x03, 0x0c},
	{USER_MAP, 0x05, 0x00},
	{USER_MAP, 0x06, 0x0e},
	{USER_MAP, 0xc9, 0x0c},
#else
	{USER_MAP, 0x03, 0x08},
	{USER_MAP, 0x05, 0x00},
	{USER_MAP, 0x06, 0x0c},
	{USER_MAP, 0xc9, 0x04},
#endif
	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_ypbpr_576i_fix_regs[] = {
#if defined(ADV7441A_PREFER_EMBMODE)
	{USER_MAP, 0x03, 0x0c},
	{USER_MAP, 0x05, 0x00},
	{USER_MAP, 0x06, 0x0f},
	{USER_MAP, 0xc9, 0x0c},
#else
	{USER_MAP, 0x03, 0x08},
	{USER_MAP, 0x05, 0x00},
	{USER_MAP, 0x06, 0x0d},
	{USER_MAP, 0xc9, 0x04},
#endif
	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_ypbpr_480p_fix_regs[] = {
#if defined(ADV7441A_PREFER_EMBMODE)
	{USER_MAP, 0x03, 0x0c},
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x06},
	{USER_MAP, 0xc9, 0x0c},
#else
	{USER_MAP, 0x03, 0x08},
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x06},
	{USER_MAP, 0xc9, 0x04},
#endif
	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_ypbpr_576p_fix_regs[] = {
#if defined(ADV7441A_PREFER_EMBMODE)
	{USER_MAP, 0x03, 0x0c},
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x07},
	{USER_MAP, 0xc9, 0x0c},
#else
	{USER_MAP, 0x03, 0x08},
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x07},
	{USER_MAP, 0xc9, 0x04},
#endif
	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_ypbpr_720p60_fix_regs[] = {
	{USER_MAP, 0x03, 0x08},
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x0a},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_ypbpr_720p50_fix_regs[] = {
	{USER_MAP, 0x03, 0x08},
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x2a},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_ypbpr_1080i60_fix_regs[] = {
	{USER_MAP, 0x03, 0x08},
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x0c},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_ypbpr_1080i50_fix_regs[] = {
	{USER_MAP, 0x03, 0x08},
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x2c},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_ypbpr_1080p60_fix_regs[] = {
	{USER_MAP, 0x03, 0x08},
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x0b},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_ypbpr_1080p50_fix_regs[] = {
	{USER_MAP, 0x03, 0x08},
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x2b},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_hdmi_share_regs_656[] = {
	//{USER_MAP, 0x03, 0x08}, //SD_DUP_AV = 0
	{USER_MAP, 0x03, 0x09}, // SD_DUP_AV = 1
	{USER_MAP, 0x05, 0x06},
	{USER_MAP, 0x1d, 0x40},
	{USER_MAP, 0x68, 0xf0},
	{USER_MAP, 0x6b, 0xd3},
	{USER_MAP, 0xba, 0xa0},
	{USER_MAP, 0xc8, 0x08},
	{USER_MAP, 0xf4, 0x3f},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_hdmi_share_regs_601[] = {
	//{USER_MAP, 0x03, 0x08}, //SD_DUP_AV = 0
	{USER_MAP, 0x03, 0x09}, // SD_DUP_AV = 1
	{USER_MAP, 0x05, 0x06},
	{USER_MAP, 0x1d, 0x40},
	{USER_MAP, 0x68, 0xf0},
	{USER_MAP, 0x6b, 0xc3},
	{USER_MAP, 0xba, 0xa0},
	{USER_MAP, 0xc8, 0x08},
	{USER_MAP, 0xf4, 0x3f},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_hdmi_720p50_fix_regs[] = {
	{USER_MAP, 0x06, 0x0a},
	{USER_MAP, 0x1d, 0x47},
	{USER_MAP, 0x3a, 0x20},
	{USER_MAP, 0x3c, 0x5c},
	{USER_MAP, 0x6b, 0xc1},
	{USER_MAP, 0x85, 0x19},
	{USER_MAP, 0x87, 0xe7},
	{USER_MAP, 0x88, 0xbc},
	{USER_MAP, 0x8f, 0x02},
	{USER_MAP, 0x90, 0xfc},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_hdmi_1080i50_fix_regs[] = {
	{USER_MAP, 0x06, 0x0c},
	{USER_MAP, 0x1d, 0x47},
	{USER_MAP, 0x3a, 0x20},
	{USER_MAP, 0x3c, 0x5d},
	{USER_MAP, 0x6b, 0xc1},
	{USER_MAP, 0x85, 0x19},
	{USER_MAP, 0x87, 0xea},
	{USER_MAP, 0x88, 0x50},
	{USER_MAP, 0x8f, 0x03},
	{USER_MAP, 0x90, 0xfa},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_hdmi_1080p50_fix_regs[] = {
	{USER_MAP, 0x06, 0x2b},
	{USER_MAP, 0x1d, 0x47},
	{USER_MAP, 0x3a, 0x20},
	{USER_MAP, 0x3c, 0x5d},
	{USER_MAP, 0x6b, 0xc1},
	{USER_MAP, 0x85, 0x19},
	{USER_MAP, 0x87, 0xea},
	{USER_MAP, 0x88, 0x50},
	{USER_MAP, 0x8f, 0x03},
	{USER_MAP, 0x90, 0xfa},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_hdmi_1080p30_fix_regs[] = {
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x4C},
	{USER_MAP, 0x91, 0x10},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_hdmi_1080p25_fix_regs[] = {
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x6C},
	{USER_MAP, 0x91, 0x10},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_hdmi_1080p24_fix_regs[] = {
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x8C},
	{USER_MAP, 0x91, 0x10},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_vga_share_regs[] = {
	{USER_MAP, 0x03, 0x08},
	{USER_MAP, 0x05, 0x02},
	{USER_MAP, 0x1d, 0x47},

	{USER_MAP, 0x68, 0xFC},
	{USER_MAP, 0x6A, 0x00},
#if defined(ADV7441A_PREFER_EMBMODE)
	{USER_MAP, 0x6b, 0xd3},
#else
	{USER_MAP, 0x6b, 0xc3},
#endif
	{USER_MAP, 0x73, 0x90},
	{USER_MAP, 0xF4, 0x3F},
	{USER_MAP, 0xFA, 0x84},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_vga_vga_fix_regs[] = {
	{USER_MAP, 0x06, 0x08},
	{USER_MAP, 0x3A, 0x11},
	{USER_MAP, 0x3C, 0x5C},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_vga_svga_fix_regs[] = {
	{USER_MAP, 0x06, 0x01},
	{USER_MAP, 0x3A, 0x11},
	{USER_MAP, 0x3C, 0x5D},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_vga_xga_fix_regs[] = {
	{USER_MAP, 0x06, 0x0C},
	{USER_MAP, 0x3A, 0x21},
	{USER_MAP, 0x3C, 0x5D},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_vga_sxga_fix_regs[] = {
	{USER_MAP, 0x06, 0x05},
	{USER_MAP, 0x3A, 0x31},
	{USER_MAP, 0x3C, 0x5D},

	{0xff, 0xff, 0xff},
};


struct adv7441a_source_info adv7441a_source_table[] = {
//	{"YPbPr",	0x00,	ADV7441A_PROCESSOR_CP,	&adv7441a_ypbpr_share_regs[0], &adv7441a_ypbpr_share_regs[0]},
	{"HDMI-A",	0x10,	ADV7441A_PROCESSOR_HDMI,&adv7441a_hdmi_share_regs_656[0], &adv7441a_hdmi_share_regs_601[0]},
//	{"CVBS-Red",	0x03,	ADV7441A_PROCESSOR_SDP, &adv7441a_cvbs_share_regs[0], &adv7441a_cvbs_share_regs[0]},
//	{"VGA",		0x00,	ADV7441A_PROCESSOR_CP,	&adv7441a_vga_share_regs[0], &adv7441a_vga_share_regs[0]},
//	{"SVideo",	0x00,	ADV7403_PROCESSOR_SDP,	&adv7441a_svideo_share_regs[0], &adv7441a_svideo_share_regs[0]},
};

#define ADV7441A_SOURCE_NUM 			ARRAY_SIZE(adv7441a_source_table)


