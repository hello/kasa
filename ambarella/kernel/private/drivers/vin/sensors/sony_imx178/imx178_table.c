/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx178/imx178_table.c
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

static struct vin_video_pll imx178_plls[] = {
	{0, 74250000, 74250000},
	{0, 54054000, 54054000},
};

static struct vin_reg_16_8 imx178_pll_regs[][47] = {
	{ /* {0, 74250000, 594000000}, */
#ifdef CONFIG_SENSOR_IMX178_SPI
		{0x030C, 0x00},
		{0x05BE, 0x0C},
		{0x05BF, 0x0C},
		{0x05C0, 0x10},
		{0x05C1, 0x10},
		{0x05C2, 0x0C},
		{0x05C3, 0x10},
		{0x05C4, 0x10},
		{0x05C5, 0x00},
		{0x031C, 0x34},
		{0x031D, 0x28},
		{0x031E, 0xAB},
		{0x031F, 0x00},
		{0x0320, 0x95},
		{0x0321, 0x00},
		{0x0322, 0xB4},
		{0x0323, 0x00},
		{0x0324, 0x8C},
		{0x0325, 0x02},
		{0x032D, 0x03},
		{0x032E, 0x0C},
		{0x032F, 0x28},
		{0x0331, 0x2D},
		{0x0332, 0x00},
		{0x0333, 0xB4},
		{0x0334, 0x00},
		{0x0337, 0x50},
		{0x0338, 0x08},
		{0x0339, 0x00},
		{0x033A, 0x07},
		{0x033D, 0x05},
		{0x0340, 0x06},
		{0x0420, 0x8B},
		{0x0421, 0x00},
		{0x0422, 0x74},
		{0x0423, 0x00},
		{0x0426, 0xC2},
		{0x0427, 0x00},
		{0x04A9, 0x1B},
		{0x04AA, 0x00},
		{0x04B3, 0x0E},
		{0x04B4, 0x00},
		{0x05D6, 0x16},
		{0x05D7, 0x15},
		{0x05D8, 0x14},
		{0x05D9, 0x10},
		{0x05DA, 0x08},
#else
		{0x310C, 0x00},
		{0x33BE, 0x0C},
		{0x33BF, 0x0C},
		{0x33C0, 0x10},
		{0x33C1, 0x10},
		{0x33C2, 0x0C},
		{0x33C3, 0x10},
		{0x33C4, 0x10},
		{0x33C5, 0x00},
		{0x311C, 0x34},
		{0x311D, 0x28},
		{0x311E, 0xAB},
		{0x311F, 0x00},
		{0x3120, 0x95},
		{0x3121, 0x00},
		{0x3122, 0xB4},
		{0x3123, 0x00},
		{0x3124, 0x8C},
		{0x3125, 0x02},
		{0x312D, 0x03},
		{0x312E, 0x0C},
		{0x312F, 0x28},
		{0x3131, 0x2D},
		{0x3132, 0x00},
		{0x3133, 0xB4},
		{0x3134, 0x00},
		{0x3137, 0x50},
		{0x3138, 0x08},
		{0x3139, 0x00},
		{0x313A, 0x07},
		{0x313D, 0x05},
		{0x3140, 0x06},
		{0x3220, 0x8B},
		{0x3221, 0x00},
		{0x3222, 0x74},
		{0x3223, 0x00},
		{0x3226, 0xC2},
		{0x3227, 0x00},
		{0x32A9, 0x1B},
		{0x32AA, 0x00},
		{0x32B3, 0x0E},
		{0x32B4, 0x00},
		{0x33D6, 0x16},
		{0x33D7, 0x15},
		{0x33D8, 0x14},
		{0x33D9, 0x10},
		{0x33DA, 0x08},
#endif
	},
	{ /* {0, 54000000, 432000000}, */
#ifdef CONFIG_SENSOR_IMX178_SPI
		{0x030C, 0x01},
		{0x05BE, 0x21},
		{0x05BF, 0x21},
		{0x05C0, 0x2C},
		{0x05C1, 0x2C},
		{0x05C2, 0x21},
		{0x05C3, 0x2C},
		{0x05C4, 0x10},
		{0x05C5, 0x01},
		{0x031C, 0x1E},
		{0x031D, 0x15},
		{0x031E, 0x72},
		{0x031F, 0x00},
		{0x0320, 0x5C},
		{0x0321, 0x00},
		{0x0322, 0x72},
		{0x0323, 0x00},
		{0x0324, 0xC7},
		{0x0325, 0x01},
		{0x032D, 0x00},
		{0x032E, 0x01},
		{0x032F, 0x15},
		{0x0331, 0x10},
		{0x0332, 0x00},
		{0x0333, 0x72},
		{0x0334, 0x00},
		{0x0337, 0x38},
		{0x0338, 0x00},
		{0x0339, 0x00},
		{0x033A, 0x00},
		{0x033D, 0x00},
		{0x0340, 0x00},
		{0x0420, 0x89},
		{0x0421, 0x00},
		{0x0422, 0x54},
		{0x0423, 0x00},
		{0x0426, 0x8D},
		{0x0427, 0x00},
		{0x04A9, 0x14},
		{0x04AA, 0x00},
		{0x04B3, 0x0A},
		{0x04B4, 0x00},
		{0x05D6, 0x10},
		{0x05D7, 0x0F},
		{0x05D8, 0x0E},
		{0x05D9, 0x0C},
		{0x05DA, 0x06},
#else
		{0x310C, 0x01},
		{0x33BE, 0x21},
		{0x33BF, 0x21},
		{0x33C0, 0x2C},
		{0x33C1, 0x2C},
		{0x33C2, 0x21},
		{0x33C3, 0x2C},
		{0x33C4, 0x10},
		{0x33C5, 0x01},
		{0x311C, 0x1E},
		{0x311D, 0x15},
		{0x311E, 0x72},
		{0x311F, 0x00},
		{0x3120, 0x5C},
		{0x3121, 0x00},
		{0x3122, 0x72},
		{0x3123, 0x00},
		{0x3124, 0xC7},
		{0x3125, 0x01},
		{0x312D, 0x00},
		{0x312E, 0x01},
		{0x312F, 0x15},
		{0x3131, 0x10},
		{0x3132, 0x00},
		{0x3133, 0x72},
		{0x3134, 0x00},
		{0x3137, 0x38},
		{0x3138, 0x00},
		{0x3139, 0x00},
		{0x313A, 0x00},
		{0x313D, 0x00},
		{0x3140, 0x00},
		{0x3220, 0x89},
		{0x3221, 0x00},
		{0x3222, 0x54},
		{0x3223, 0x00},
		{0x3226, 0x8D},
		{0x3227, 0x00},
		{0x32A9, 0x14},
		{0x32AA, 0x00},
		{0x32B3, 0x0A},
		{0x32B4, 0x00},
		{0x33D6, 0x10},
		{0x33D7, 0x0F},
		{0x33D8, 0x0E},
		{0x33D9, 0x0C},
		{0x33DA, 0x06},
#endif
	},
};

static struct vin_reg_16_8 imx178_mode_regs_10lane[][15] = {
#ifdef CONFIG_SENSOR_IMX178_SPI
	{	/* 1080P 10lane (12bits)@120fps */
		{0x020E,  0x01},/* MODE */
		{0x020F,  0x00},/* WINMODE */
		{0x0210,  0x00},/* TCYCLE */
		{0x0266,  0x03},/* VNDMY */
		{0x022C,  0x65},/* VMAX_LSB */
		{0x022D,  0x04},/* VMAX_MSB */
		{0x022E,  0x00},/* VMAX_HSB */
		{0x022F,  0x26},/* HMAX_LSB */
		{0x0230,  0x02},/* HMAX_MSB */
		{0x020D,  0x05},/* ADBIT_ADBITFREQ, ADC 12bits */
		{0x0259,  0x01},/* ODBIT_OPORTSEL, 10ch, 12bits */
		{0x0204,  0x00},/* STBLVDS, 10ch active */
		{0x0301,  0x30},/* FREQ */
		{0x0215,  0xC8},/* BLKLEVEL_LSB */
		{0x0216,  0x00},/* BLKLEVEL_MSB */
	},
	{	/* 6.3M: 3072x2048 10lane (14bits)@30fps */
		{0x020E,  0x00},/* MODE */
		{0x020F,  0x00},/* WINMODE */
		{0x0210,  0x00},/* TCYCLE */
		{0x0266,  0x06},/* VNDMY */
		{0x022C,  0x61},/* VMAX_LSB */
		{0x022D,  0x08},/* VMAX_MSB */
		{0x022E,  0x00},/* VMAX_HSB */
		{0x022F,  0x48},/* HMAX_LSB */
		{0x0230,  0x03},/* HMAX_MSB */
		{0x020D,  0x02},/* ADBIT_ADBITFREQ, ADC 14bits */
		{0x0259,  0x02},/* ODBIT_OPORTSEL, 10ch, 14bits */
		{0x0204,  0x00},/* STBLVDS, 10ch active */
		{0x0301,  0x30},/* FREQ */
		{0x0215,  0x20},/* BLKLEVEL_LSB */
		{0x0216,  0x03},/* BLKLEVEL_MSB */
	},
	{	/* 5.0M: 2592x1944 10lane (14bits)@30fps */
		{0x020E,  0x00},/* MODE */
		{0x020F,  0x10},/* WINMODE */
		{0x0210,  0x00},/* TCYCLE */
		{0x0266,  0x06},/* VNDMY */
		{0x022C,  0x61},/* VMAX_LSB */
		{0x022D,  0x08},/* VMAX_MSB */
		{0x022E,  0x00},/* VMAX_HSB */
		{0x022F,  0x48},/* HMAX_LSB */
		{0x0230,  0x03},/* HMAX_MSB */
		{0x020D,  0x02},/* ADBIT_ADBITFREQ, ADC 14bits */
		{0x0259,  0x02},/* ODBIT_OPORTSEL, 10ch, 14bits */
		{0x0204,  0x00},/* STBLVDS, 10ch active */
		{0x0301,  0x30},/* FREQ */
		{0x0215,  0x20},/* BLKLEVEL_LSB */
		{0x0216,  0x03},/* BLKLEVEL_MSB */
	},
	{	/* 6.3M: 3072x2048 10lane (10bits)@60fps */
		{0x020E,  0x00},/* MODE */
		{0x020F,  0x00},/* WINMODE */
		{0x0210,  0x00},/* TCYCLE */
		{0x0266,  0x06},/* VNDMY */
		{0x022C,  0x61},/* VMAX_LSB */
		{0x022D,  0x08},/* VMAX_MSB */
		{0x022E,  0x00},/* VMAX_HSB */
		{0x022F,  0xA4},/* HMAX_LSB */
		{0x0230,  0x01},/* HMAX_MSB */
		{0x020D,  0x00},/* ADBIT_ADBITFREQ, ADC 10bits */
		{0x0259,  0x00},/* ODBIT_OPORTSEL, 10ch, 10bits */
		{0x0204,  0x00},/* STBLVDS, 10ch active */
		{0x0301,  0x30},/* FREQ */
		{0x0215,  0x32},/* BLKLEVEL_LSB */
		{0x0216,  0x00},/* BLKLEVEL_MSB */
	},
	{	/* 720P 10lane (12bits)@120fps */
		{0x020E,  0x24},/* MODE */
		{0x020F,  0x00},/* WINMODE */
		{0x0210,  0x01},/* TCYCLE */
		{0x0266,  0x04},/* VNDMY */
		{0x022C,  0x72},/* VMAX_LSB */
		{0x022D,  0x06},/* VMAX_MSB */
		{0x022E,  0x00},/* VMAX_HSB */
		{0x022F,  0x77},/* HMAX_LSB */
		{0x0230,  0x01},/* HMAX_MSB */
		{0x020D,  0x04},/* ADBIT_ADBITFREQ, ADC 12bits */
		{0x0259,  0x01},/* ODBIT_OPORTSEL, 10ch, 12bits */
		{0x0204,  0x00},/* STBLVDS, 10ch active */
		{0x0301,  0x31},/* FREQ */
		{0x0215,  0xC8},/* BLKLEVEL_LSB */
		{0x0216,  0x00},/* BLKLEVEL_MSB */
	},
	{	/* 5.2M: 2560x2048 10lane (14bits)@30fps */
		{0x020E,  0x00},/* MODE */
		{0x020F,  0x20},/* WINMODE */
		{0x0210,  0x00},/* TCYCLE */
		{0x0266,  0x06},/* VNDMY */
		{0x022C,  0x61},/* VMAX_LSB */
		{0x022D,  0x08},/* VMAX_MSB */
		{0x022E,  0x00},/* VMAX_HSB */
		{0x022F,  0x48},/* HMAX_LSB */
		{0x0230,  0x03},/* HMAX_MSB */
		{0x020D,  0x02},/* ADBIT_ADBITFREQ, ADC 14bits */
		{0x0259,  0x02},/* ODBIT_OPORTSEL, 10ch, 14bits */
		{0x0204,  0x00},/* STBLVDS, 10ch active */
		{0x0301,  0x30},/* FREQ */
		{0x0215,  0x20},/* BLKLEVEL_LSB */
		{0x0216,  0x03},/* BLKLEVEL_MSB */
	},
	{	/* 5.3M: 3072x1728 10lane (14bits)@30fps */
		{0x020E,  0x00},/* MODE */
		{0x020F,  0x30},/* WINMODE */
		{0x0210,  0x00},/* TCYCLE */
		{0x0266,  0x06},/* VNDMY */
		{0x022C,  0x98},/* VMAX_LSB */
		{0x022D,  0x08},/* VMAX_MSB */
		{0x022E,  0x00},/* VMAX_HSB */
		{0x022F,  0x65},/* HMAX_LSB */
		{0x0230,  0x04},/* HMAX_MSB */
		{0x020D,  0x02},/* ADBIT_ADBITFREQ, ADC 14bits */
		{0x0259,  0x02},/* ODBIT_OPORTSEL, 10ch, 14bits */
		{0x0204,  0x00},/* STBLVDS, 10ch active */
		{0x0301,  0x31},/* FREQ */
		{0x0215,  0x20},/* BLKLEVEL_LSB */
		{0x0216,  0x03},/* BLKLEVEL_MSB */
	},
	{	/* 5.3M: 3072x1728 10lane (12bits)@60fps */
		{0x020E,  0x00},/* MODE */
		{0x020F,  0x30},/* WINMODE */
		{0x0210,  0x00},/* TCYCLE */
		{0x0266,  0x06},/* VNDMY */
		{0x022C,  0xBC},/* VMAX_LSB */
		{0x022D,  0x07},/* VMAX_MSB */
		{0x022E,  0x00},/* VMAX_HSB */
		{0x022F,  0x71},/* HMAX_LSB */
		{0x0230,  0x02},/* HMAX_MSB */
		{0x020D,  0x05},/* ADBIT_ADBITFREQ, ADC 12bits */
		{0x0259,  0x01},/* ODBIT_OPORTSEL, 10ch, 12bits */
		{0x0204,  0x00},/* STBLVDS, 10ch active */
		{0x0301,  0x30},/* FREQ */
		{0x0215,  0xC8},/* BLKLEVEL_LSB */
		{0x0216,  0x00},/* BLKLEVEL_MSB */
	},
	{	/* 5.0M: 2592x1944 10lane (12bits)@60fps */
		{0x020E,  0x00},/* MODE */
		{0x020F,  0x10},/* WINMODE */
		{0x0210,  0x00},/* TCYCLE */
		{0x0266,  0x06},/* VNDMY */
		{0x022C,  0x61},/* VMAX_LSB */
		{0x022D,  0x08},/* VMAX_MSB */
		{0x022E,  0x00},/* VMAX_HSB */
		{0x022F,  0xA4},/* HMAX_LSB */
		{0x0230,  0x01},/* HMAX_MSB */
		{0x020D,  0x05},/* ADBIT_ADBITFREQ, ADC 12bits */
		{0x0259,  0x01},/* ODBIT_OPORTSEL, 10ch, 12bits */
		{0x0204,  0x00},/* STBLVDS, 10ch active */
		{0x0301,  0x30},/* FREQ */
		{0x0215,  0xC8},/* BLKLEVEL_LSB */
		{0x0216,  0x00},/* BLKLEVEL_MSB */
	},
#else
	{	/* 1080P 10lane (12bits)@120fps */
		{0x300E,  0x01},/* MODE */
		{0x300F,  0x00},/* WINMODE */
		{0x3010,  0x00},/* TCYCLE */
		{0x3066,  0x03},/* VNDMY */
		{0x302C,  0x65},/* VMAX_LSB */
		{0x302D,  0x04},/* VMAX_MSB */
		{0x302E,  0x00},/* VMAX_HSB */
		{0x302F,  0x26},/* HMAX_LSB */
		{0x3030,  0x02},/* HMAX_MSB */
		{0x300D,  0x05},/* ADBIT_ADBITFREQ, ADC 12bits */
		{0x3059,  0x01},/* ODBIT_OPORTSEL, 10ch, 12bits */
		{0x3004,  0x00},/* STBLVDS, 10ch active */
		{0x3101,  0x30},/* FREQ */
		{0x3015,  0xC8},/* BLKLEVEL_LSB */
		{0x3016,  0x00},/* BLKLEVEL_MSB */
	},
	{	/* 6.3M: 3072x2048 10lane (14bits)@30fps */
		{0x300E,  0x00},/* MODE */
		{0x300F,  0x00},/* WINMODE */
		{0x3010,  0x00},/* TCYCLE */
		{0x3066,  0x06},/* VNDMY */
		{0x302C,  0x61},/* VMAX_LSB */
		{0x302D,  0x08},/* VMAX_MSB */
		{0x302E,  0x00},/* VMAX_HSB */
		{0x302F,  0x48},/* HMAX_LSB */
		{0x3030,  0x03},/* HMAX_MSB */
		{0x300D,  0x02},/* ADBIT_ADBITFREQ, ADC 14bits */
		{0x3059,  0x02},/* ODBIT_OPORTSEL, 10ch, 14bits */
		{0x3004,  0x00},/* STBLVDS, 10ch active */
		{0x3101,  0x30},/* FREQ */
		{0x3015,  0x20},/* BLKLEVEL_LSB */
		{0x3016,  0x03},/* BLKLEVEL_MSB */
	},
	{	/* 5.0M: 2592x1944 10lane (14bits)@30fps */
		{0x300E,  0x00},/* MODE */
		{0x300F,  0x10},/* WINMODE */
		{0x3010,  0x00},/* TCYCLE */
		{0x3066,  0x06},/* VNDMY */
		{0x302C,  0x61},/* VMAX_LSB */
		{0x302D,  0x08},/* VMAX_MSB */
		{0x302E,  0x00},/* VMAX_HSB */
		{0x302F,  0x48},/* HMAX_LSB */
		{0x3030,  0x03},/* HMAX_MSB */
		{0x300D,  0x02},/* ADBIT_ADBITFREQ, ADC 14bits */
		{0x3059,  0x02},/* ODBIT_OPORTSEL, 10ch, 14bits */
		{0x3004,  0x00},/* STBLVDS, 10ch active */
		{0x3101,  0x30},/* FREQ */
		{0x3015,  0x20},/* BLKLEVEL_LSB */
		{0x3016,  0x03},/* BLKLEVEL_MSB */
	},
	{	/* 6.3M: 3072x2048 10lane (10bits)@60fps */
		{0x300E,  0x00},/* MODE */
		{0x300F,  0x00},/* WINMODE */
		{0x3010,  0x00},/* TCYCLE */
		{0x3066,  0x06},/* VNDMY */
		{0x302C,  0x61},/* VMAX_LSB */
		{0x302D,  0x08},/* VMAX_MSB */
		{0x302E,  0x00},/* VMAX_HSB */
		{0x302F,  0xA4},/* HMAX_LSB */
		{0x3030,  0x01},/* HMAX_MSB */
		{0x300D,  0x00},/* ADBIT_ADBITFREQ, ADC 10bits */
		{0x3059,  0x00},/* ODBIT_OPORTSEL, 10ch, 10bits */
		{0x3004,  0x00},/* STBLVDS, 10ch active */
		{0x3101,  0x30},/* FREQ */
		{0x3015,  0x32},/* BLKLEVEL_LSB */
		{0x3016,  0x00},/* BLKLEVEL_MSB */
	},
	{	/* 720P 10lane (12bits)@120fps */
		{0x300E,  0x24},/* MODE */
		{0x300F,  0x00},/* WINMODE */
		{0x3010,  0x01},/* TCYCLE */
		{0x3066,  0x04},/* VNDMY */
		{0x302C,  0x72},/* VMAX_LSB */
		{0x302D,  0x06},/* VMAX_MSB */
		{0x302E,  0x00},/* VMAX_HSB */
		{0x302F,  0x77},/* HMAX_LSB */
		{0x3030,  0x01},/* HMAX_MSB */
		{0x300D,  0x04},/* ADBIT_ADBITFREQ, ADC 12bits */
		{0x3059,  0x01},/* ODBIT_OPORTSEL, 10ch, 12bits */
		{0x3004,  0x00},/* STBLVDS, 10ch active */
		{0x3101,  0x31},/* FREQ */
		{0x3015,  0xC8},/* BLKLEVEL_LSB */
		{0x3016,  0x00},/* BLKLEVEL_MSB */
	},
	{	/* 5.2M: 2560x2048 10lane (14bits)@30fps */
		{0x300E,  0x00},/* MODE */
		{0x300F,  0x20},/* WINMODE */
		{0x3010,  0x00},/* TCYCLE */
		{0x3066,  0x06},/* VNDMY */
		{0x302C,  0x61},/* VMAX_LSB */
		{0x302D,  0x08},/* VMAX_MSB */
		{0x302E,  0x00},/* VMAX_HSB */
		{0x302F,  0x48},/* HMAX_LSB */
		{0x3030,  0x03},/* HMAX_MSB */
		{0x300D,  0x02},/* ADBIT_ADBITFREQ, ADC 14bits */
		{0x3059,  0x02},/* ODBIT_OPORTSEL, 10ch, 14bits */
		{0x3004,  0x00},/* STBLVDS, 10ch active */
		{0x3101,  0x30},/* FREQ */
		{0x3015,  0x20},/* BLKLEVEL_LSB */
		{0x3016,  0x03},/* BLKLEVEL_MSB */
	},
	{	/* 5.3M: 3072x1728 10lane (14bits)@30fps */
		{0x300E,  0x00},/* MODE */
		{0x300F,  0x30},/* WINMODE */
		{0x3010,  0x00},/* TCYCLE */
		{0x3066,  0x06},/* VNDMY */
		{0x302C,  0x98},/* VMAX_LSB */
		{0x302D,  0x08},/* VMAX_MSB */
		{0x302E,  0x00},/* VMAX_HSB */
		{0x302F,  0x65},/* HMAX_LSB */
		{0x3030,  0x04},/* HMAX_MSB */
		{0x300D,  0x02},/* ADBIT_ADBITFREQ, ADC 14bits */
		{0x3059,  0x02},/* ODBIT_OPORTSEL, 10ch, 14bits */
		{0x3004,  0x00},/* STBLVDS, 10ch active */
		{0x3101,  0x31},/* FREQ */
		{0x3015,  0x20},/* BLKLEVEL_LSB */
		{0x3016,  0x03},/* BLKLEVEL_MSB */
	},
	{	/* 5.3M: 3072x1728 10lane (12bits)@60fps */
		{0x300E,  0x00},/* MODE */
		{0x300F,  0x30},/* WINMODE */
		{0x3010,  0x00},/* TCYCLE */
		{0x3066,  0x06},/* VNDMY */
		{0x302C,  0xBC},/* VMAX_LSB */
		{0x302D,  0x07},/* VMAX_MSB */
		{0x302E,  0x00},/* VMAX_HSB */
		{0x302F,  0x71},/* HMAX_LSB */
		{0x3030,  0x02},/* HMAX_MSB */
		{0x300D,  0x05},/* ADBIT_ADBITFREQ, ADC 12bits */
		{0x3059,  0x01},/* ODBIT_OPORTSEL, 10ch, 12bits */
		{0x3004,  0x00},/* STBLVDS, 10ch active */
		{0x3101,  0x30},/* FREQ */
		{0x3015,  0xC8},/* BLKLEVEL_LSB */
		{0x3016,  0x00},/* BLKLEVEL_MSB */
	},
	{	/* 5.0M: 2592x1944 10lane (12bits)@60fps */
		{0x300E,  0x00},/* MODE */
		{0x300F,  0x10},/* WINMODE */
		{0x3010,  0x00},/* TCYCLE */
		{0x3066,  0x06},/* VNDMY */
		{0x302C,  0x61},/* VMAX_LSB */
		{0x302D,  0x08},/* VMAX_MSB */
		{0x302E,  0x00},/* VMAX_HSB */
		{0x302F,  0xA4},/* HMAX_LSB */
		{0x3030,  0x01},/* HMAX_MSB */
		{0x300D,  0x05},/* ADBIT_ADBITFREQ, ADC 12bits */
		{0x3059,  0x01},/* ODBIT_OPORTSEL, 10ch, 12bits */
		{0x3004,  0x00},/* STBLVDS, 10ch active */
		{0x3101,  0x30},/* FREQ */
		{0x3015,  0xC8},/* BLKLEVEL_LSB */
		{0x3016,  0x00},/* BLKLEVEL_MSB */
	},
#endif
};

static struct vin_reg_16_8 imx178_mode_regs_8lane[][15] = {
#ifdef CONFIG_SENSOR_IMX178_SPI
	{	/* 1080P 8lane (12bits)@120fps */
		{0x020E,  0x01},/* MODE */
		{0x020F,  0x00},/* WINMODE */
		{0x0210,  0x00},/* TCYCLE */
		{0x0266,  0x03},/* VNDMY */
		{0x022C,  0x65},/* VMAX_LSB */
		{0x022D,  0x04},/* VMAX_MSB */
		{0x022E,  0x00},/* VMAX_HSB */
		{0x022F,  0x26},/* HMAX_LSB */
		{0x0230,  0x02},/* HMAX_MSB */
		{0x020D,  0x05},/* ADBIT_ADBITFREQ, ADC 12bits */
		{0x0259,  0x11},/* ODBIT_OPORTSEL, 8ch, 12bits */
		{0x0204,  0x01},/* STBLVDS, 8ch active */
		{0x0301,  0x30},/* FREQ */
		{0x0215,  0xC8},/* BLKLEVEL_LSB */
		{0x0216,  0x00},/* BLKLEVEL_MSB */
	},
	{	/* 6.3M: 3072x2048 8lane (14bits)@30fps */
		{0x020E,  0x00},/* MODE */
		{0x020F,  0x00},/* WINMODE */
		{0x0210,  0x00},/* TCYCLE */
		{0x0266,  0x06},/* VNDMY */
		{0x022C,  0x61},/* VMAX_LSB */
		{0x022D,  0x08},/* VMAX_MSB */
		{0x022E,  0x00},/* VMAX_HSB */
		{0x022F,  0x48},/* HMAX_LSB */
		{0x0230,  0x03},/* HMAX_MSB */
		{0x020D,  0x02},/* ADBIT_ADBITFREQ, ADC 14bits */
		{0x0259,  0x12},/* ODBIT_OPORTSEL, 8ch, 14bits */
		{0x0204,  0x01},/* STBLVDS, 8ch active */
		{0x0301,  0x30},/* FREQ */
		{0x0215,  0x20},/* BLKLEVEL_LSB */
		{0x0216,  0x03},/* BLKLEVEL_MSB */
	},
	{	/* 5.0M: 2592x1944 8lane (14bits)@30fps */
		{0x020E,  0x00},/* MODE */
		{0x020F,  0x10},/* WINMODE */
		{0x0210,  0x00},/* TCYCLE */
		{0x0266,  0x06},/* VNDMY */
		{0x022C,  0x61},/* VMAX_LSB */
		{0x022D,  0x08},/* VMAX_MSB */
		{0x022E,  0x00},/* VMAX_HSB */
		{0x022F,  0x48},/* HMAX_LSB */
		{0x0230,  0x03},/* HMAX_MSB */
		{0x020D,  0x02},/* ADBIT_ADBITFREQ, ADC 14bits */
		{0x0259,  0x12},/* ODBIT_OPORTSEL, 8ch, 14bits */
		{0x0204,  0x01},/* STBLVDS, 8ch active */
		{0x0301,  0x30},/* FREQ */
		{0x0215,  0x20},/* BLKLEVEL_LSB */
		{0x0216,  0x03},/* BLKLEVEL_MSB */
	},
	{	/* 6.3M: 3072x2048 8lane (10bits)@60fps */
		{0x020E,  0x00},/* MODE */
		{0x020F,  0x00},/* WINMODE */
		{0x0210,  0x00},/* TCYCLE */
		{0x0266,  0x06},/* VNDMY */
		{0x022C,  0xF7},/* VMAX_LSB */
		{0x022D,  0x0D},/* VMAX_MSB */
		{0x022E,  0x00},/* VMAX_HSB */
		{0x022F,  0xF8},/* HMAX_LSB */
		{0x0230,  0x01},/* HMAX_MSB */
		{0x020D,  0x00},/* ADBIT_ADBITFREQ, ADC 10bits */
		{0x0259,  0x10},/* ODBIT_OPORTSEL, 8ch, 10bits */
		{0x0204,  0x01},/* STBLVDS, 8ch active */
		{0x0301,  0x30},/* FREQ */
		{0x0215,  0x32},/* BLKLEVEL_LSB */
		{0x0216,  0x00},/* BLKLEVEL_MSB */
	},
	{	/* 720P 8lane (12bits)@120fps */
		{0x020E,  0x24},/* MODE */
		{0x020F,  0x00},/* WINMODE */
		{0x0210,  0x01},/* TCYCLE */
		{0x0266,  0x04},/* VNDMY */
		{0x022C,  0x72},/* VMAX_LSB */
		{0x022D,  0x06},/* VMAX_MSB */
		{0x022E,  0x00},/* VMAX_HSB */
		{0x022F,  0x77},/* HMAX_LSB */
		{0x0230,  0x01},/* HMAX_MSB */
		{0x020D,  0x04},/* ADBIT_ADBITFREQ, ADC 10bits */
		{0x0259,  0x11},/* ODBIT_OPORTSEL, 8ch, 12bits */
		{0x0204,  0x01},/* STBLVDS, 8ch active */
		{0x0301,  0x31},/* FREQ */
		{0x0215,  0xC8},/* BLKLEVEL_LSB */
		{0x0216,  0x00},/* BLKLEVEL_MSB */
	},
	{	/* 5.2M: 2560x2048 8lane (14bits)@30fps */
		{0x020E,  0x00},/* MODE */
		{0x020F,  0x20},/* WINMODE */
		{0x0210,  0x00},/* TCYCLE */
		{0x0266,  0x06},/* VNDMY */
		{0x022C,  0x61},/* VMAX_LSB */
		{0x022D,  0x08},/* VMAX_MSB */
		{0x022E,  0x00},/* VMAX_HSB */
		{0x022F,  0x48},/* HMAX_LSB */
		{0x0230,  0x03},/* HMAX_MSB */
		{0x020D,  0x02},/* ADBIT_ADBITFREQ, ADC 14bits */
		{0x0259,  0x12},/* ODBIT_OPORTSEL, 8ch, 14bits */
		{0x0204,  0x01},/* STBLVDS, 8ch active */
		{0x0301,  0x30},/* FREQ */
		{0x0215,  0x20},/* BLKLEVEL_LSB */
		{0x0216,  0x03},/* BLKLEVEL_MSB */
	},
	{	/* 5.3M: 3072x1728 8lane (14bits)@30fps */
		{0x020E,  0x00},/* MODE */
		{0x020F,  0x30},/* WINMODE */
		{0x0210,  0x00},/* TCYCLE */
		{0x0266,  0x06},/* VNDMY */
		{0x022C,  0x98},/* VMAX_LSB */
		{0x022D,  0x08},/* VMAX_MSB */
		{0x022E,  0x00},/* VMAX_HSB */
		{0x022F,  0x65},/* HMAX_LSB */
		{0x0230,  0x04},/* HMAX_MSB */
		{0x020D,  0x02},/* ADBIT_ADBITFREQ, ADC 14bits */
		{0x0259,  0x12},/* ODBIT_OPORTSEL, 8ch, 14bits */
		{0x0204,  0x01},/* STBLVDS, 8ch active */
		{0x0301,  0x30},/* FREQ */
		{0x0215,  0x20},/* BLKLEVEL_LSB */
		{0x0216,  0x03},/* BLKLEVEL_MSB */
	},
	{	/* 5.3M: 3072x1728 8lane (12bits)@60fps */
		{0x020E,  0x00},/* MODE */
		{0x020F,  0x30},/* WINMODE */
		{0x0210,  0x00},/* TCYCLE */
		{0x0266,  0x06},/* VNDMY */
		{0x022C,  0xBC},/* VMAX_LSB */
		{0x022D,  0x07},/* VMAX_MSB */
		{0x022E,  0x00},/* VMAX_HSB */
		{0x022F,  0x71},/* HMAX_LSB */
		{0x0230,  0x02},/* HMAX_MSB */
		{0x020D,  0x05},/* ADBIT_ADBITFREQ, ADC 12bits */
		{0x0259,  0x11},/* ODBIT_OPORTSEL, 8ch, 12bits */
		{0x0204,  0x01},/* STBLVDS, 8ch active */
		{0x0301,  0x30},/* FREQ */
		{0x0215,  0xC8},/* BLKLEVEL_LSB */
		{0x0216,  0x00},/* BLKLEVEL_MSB */
	},
	{	/* 5.0M: 2592x1944 8lane (12bits)@30fps */
		{0x020E,  0x00},/* MODE */
		{0x020F,  0x10},/* WINMODE */
		{0x0210,  0x00},/* TCYCLE */
		{0x0266,  0x06},/* VNDMY */
		{0x022C,  0x89},/* VMAX_LSB */
		{0x022D,  0x0D},/* VMAX_MSB */
		{0x022E,  0x00},/* VMAX_HSB */
		{0x022F,  0x08},/* HMAX_LSB */
		{0x0230,  0x02},/* HMAX_MSB */
		{0x020D,  0x05},/* ADBIT_ADBITFREQ, ADC 12bits */
		{0x0259,  0x11},/* ODBIT_OPORTSEL, 8ch, 12bits */
		{0x0204,  0x01},/* STBLVDS, 8ch active */
		{0x0301,  0x30},/* FREQ */
		{0x0215,  0xC8},/* BLKLEVEL_LSB */
		{0x0216,  0x00},/* BLKLEVEL_MSB */
	},
#else
	{	/* 1080P 8lane (12bits)@120fps */
		{0x300E,  0x01},/* MODE */
		{0x300F,  0x00},/* WINMODE */
		{0x3010,  0x00},/* TCYCLE */
		{0x3066,  0x03},/* VNDMY */
		{0x302C,  0x65},/* VMAX_LSB */
		{0x302D,  0x04},/* VMAX_MSB */
		{0x302E,  0x00},/* VMAX_HSB */
		{0x302F,  0x26},/* HMAX_LSB */
		{0x3030,  0x02},/* HMAX_MSB */
		{0x300D,  0x05},/* ADBIT_ADBITFREQ, ADC 12bits */
		{0x3059,  0x11},/* ODBIT_OPORTSEL, 8ch, 12bits */
		{0x3004,  0x01},/* STBLVDS, 8ch active */
		{0x3101,  0x30},/* FREQ */
		{0x3015,  0xC8},/* BLKLEVEL_LSB */
		{0x3016,  0x00},/* BLKLEVEL_MSB */
	},
	{	/* 6.3M: 3072x2048 8lane (14bits)@30fps */
		{0x300E,  0x00},/* MODE */
		{0x300F,  0x00},/* WINMODE */
		{0x3010,  0x00},/* TCYCLE */
		{0x3066,  0x06},/* VNDMY */
		{0x302C,  0x61},/* VMAX_LSB */
		{0x302D,  0x08},/* VMAX_MSB */
		{0x302E,  0x00},/* VMAX_HSB */
		{0x302F,  0x48},/* HMAX_LSB */
		{0x3030,  0x03},/* HMAX_MSB */
		{0x300D,  0x02},/* ADBIT_ADBITFREQ, ADC 14bits */
		{0x3059,  0x12},/* ODBIT_OPORTSEL, 8ch, 14bits */
		{0x3004,  0x01},/* STBLVDS, 8ch active */
		{0x3101,  0x30},/* FREQ */
		{0x3015,  0x20},/* BLKLEVEL_LSB */
		{0x3016,  0x03},/* BLKLEVEL_MSB */
	},
	{	/* 5.0M: 2592x1944 8lane (14bits)@30fps */
		{0x300E,  0x00},/* MODE */
		{0x300F,  0x10},/* WINMODE */
		{0x3010,  0x00},/* TCYCLE */
		{0x3066,  0x06},/* VNDMY */
		{0x302C,  0x61},/* VMAX_LSB */
		{0x302D,  0x08},/* VMAX_MSB */
		{0x302E,  0x00},/* VMAX_HSB */
		{0x302F,  0x48},/* HMAX_LSB */
		{0x3030,  0x03},/* HMAX_MSB */
		{0x300D,  0x02},/* ADBIT_ADBITFREQ, ADC 14bits */
		{0x3059,  0x12},/* ODBIT_OPORTSEL, 8ch, 14bits */
		{0x3004,  0x01},/* STBLVDS, 8ch active */
		{0x3101,  0x30},/* FREQ */
		{0x3015,  0x20},/* BLKLEVEL_LSB */
		{0x3016,  0x03},/* BLKLEVEL_MSB */
	},
	{	/* 6.3M: 3072x2048 8lane (10bits)@60fps */
		{0x300E,  0x00},/* MODE */
		{0x300F,  0x00},/* WINMODE */
		{0x3010,  0x00},/* TCYCLE */
		{0x3066,  0x06},/* VNDMY */
		{0x302C,  0xF7},/* VMAX_LSB */
		{0x302D,  0x0D},/* VMAX_MSB */
		{0x302E,  0x00},/* VMAX_HSB */
		{0x302F,  0xF8},/* HMAX_LSB */
		{0x3030,  0x01},/* HMAX_MSB */
		{0x300D,  0x00},/* ADBIT_ADBITFREQ, ADC 10bits */
		{0x3059,  0x10},/* ODBIT_OPORTSEL, 8ch, 10bits */
		{0x3004,  0x01},/* STBLVDS, 8ch active */
		{0x3101,  0x30},/* FREQ */
		{0x3015,  0x32},/* BLKLEVEL_LSB */
		{0x3016,  0x00},/* BLKLEVEL_MSB */
	},
	{	/* 720P 8lane (12bits)@120fps */
		{0x300E,  0x24},/* MODE */
		{0x300F,  0x00},/* WINMODE */
		{0x3010,  0x01},/* TCYCLE */
		{0x3066,  0x04},/* VNDMY */
		{0x302C,  0x72},/* VMAX_LSB */
		{0x302D,  0x06},/* VMAX_MSB */
		{0x302E,  0x00},/* VMAX_HSB */
		{0x302F,  0x77},/* HMAX_LSB */
		{0x3030,  0x01},/* HMAX_MSB */
		{0x300D,  0x04},/* ADBIT_ADBITFREQ, ADC 10bits */
		{0x3059,  0x11},/* ODBIT_OPORTSEL, 8ch, 12bits */
		{0x3004,  0x01},/* STBLVDS, 8ch active */
		{0x3101,  0x31},/* FREQ */
		{0x3015,  0xC8},/* BLKLEVEL_LSB */
		{0x3016,  0x00},/* BLKLEVEL_MSB */
	},
	{	/* 5.2M: 2560x2048 8lane (14bits)@30fps */
		{0x300E,  0x00},/* MODE */
		{0x300F,  0x20},/* WINMODE */
		{0x3010,  0x00},/* TCYCLE */
		{0x3066,  0x06},/* VNDMY */
		{0x302C,  0x61},/* VMAX_LSB */
		{0x302D,  0x08},/* VMAX_MSB */
		{0x302E,  0x00},/* VMAX_HSB */
		{0x302F,  0x48},/* HMAX_LSB */
		{0x3030,  0x03},/* HMAX_MSB */
		{0x300D,  0x02},/* ADBIT_ADBITFREQ, ADC 14bits */
		{0x3059,  0x12},/* ODBIT_OPORTSEL, 8ch, 14bits */
		{0x3004,  0x01},/* STBLVDS, 8ch active */
		{0x3101,  0x30},/* FREQ */
		{0x3015,  0x20},/* BLKLEVEL_LSB */
		{0x3016,  0x03},/* BLKLEVEL_MSB */
	},
	{	/* 5.3M: 3072x1728 8lane (14bits)@30fps */
		{0x300E,  0x00},/* MODE */
		{0x300F,  0x30},/* WINMODE */
		{0x3010,  0x00},/* TCYCLE */
		{0x3066,  0x06},/* VNDMY */
		{0x302C,  0x98},/* VMAX_LSB */
		{0x302D,  0x08},/* VMAX_MSB */
		{0x302E,  0x00},/* VMAX_HSB */
		{0x302F,  0x65},/* HMAX_LSB */
		{0x3030,  0x04},/* HMAX_MSB */
		{0x300D,  0x02},/* ADBIT_ADBITFREQ, ADC 14bits */
		{0x3059,  0x12},/* ODBIT_OPORTSEL, 8ch, 14bits */
		{0x3004,  0x01},/* STBLVDS, 8ch active */
		{0x3101,  0x30},/* FREQ */
		{0x3015,  0x20},/* BLKLEVEL_LSB */
		{0x3016,  0x03},/* BLKLEVEL_MSB */
	},
	{	/* 5.3M: 3072x1728 8lane (12bits)@60fps */
		{0x300E,  0x00},/* MODE */
		{0x300F,  0x30},/* WINMODE */
		{0x3010,  0x00},/* TCYCLE */
		{0x3066,  0x06},/* VNDMY */
		{0x302C,  0xBC},/* VMAX_LSB */
		{0x302D,  0x07},/* VMAX_MSB */
		{0x302E,  0x00},/* VMAX_HSB */
		{0x302F,  0x71},/* HMAX_LSB */
		{0x3030,  0x02},/* HMAX_MSB */
		{0x300D,  0x05},/* ADBIT_ADBITFREQ, ADC 12bits */
		{0x3059,  0x11},/* ODBIT_OPORTSEL, 8ch, 12bits */
		{0x3004,  0x01},/* STBLVDS, 8ch active */
		{0x3101,  0x30},/* FREQ */
		{0x3015,  0xC8},/* BLKLEVEL_LSB */
		{0x3016,  0x00},/* BLKLEVEL_MSB */
	},
	{	/* 5.0M: 2592x1944 8lane (12bits)@30fps */
		{0x300E,  0x00},/* MODE */
		{0x300F,  0x10},/* WINMODE */
		{0x3010,  0x00},/* TCYCLE */
		{0x3066,  0x06},/* VNDMY */
		{0x302C,  0x89},/* VMAX_LSB */
		{0x302D,  0x0D},/* VMAX_MSB */
		{0x302E,  0x00},/* VMAX_HSB */
		{0x302F,  0x08},/* HMAX_LSB */
		{0x3030,  0x02},/* HMAX_MSB */
		{0x300D,  0x05},/* ADBIT_ADBITFREQ, ADC 12bits */
		{0x3059,  0x11},/* ODBIT_OPORTSEL, 8ch, 12bits */
		{0x3004,  0x01},/* STBLVDS, 8ch active */
		{0x3101,  0x30},/* FREQ */
		{0x3015,  0xC8},/* BLKLEVEL_LSB */
		{0x3016,  0x00},/* BLKLEVEL_MSB */
	},
#endif
};

static struct vin_video_format imx178_formats[] = {
	{
		.video_mode	= AMBA_VIDEO_MODE_3072_2048,
		.def_start_x	= 4+8,
		.def_start_y	= 1+6+8+8+8,
		.def_width	= 3072,
		.def_height	= 2048,
		/* sensor mode */
		.device_mode	= 1,
		.pll_idx	= 1,
		.width		= 3072,
		.height		= 2048,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_14,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_QSXGA,
		.def_start_x	= 4+8,
		.def_start_y	= 1+6+8+8+8,
		.def_width	= 2592,
		.def_height	= 1944,
		/* sensor mode */
		.device_mode	= 2,
		.pll_idx	= 1,
		.width		= 2592,
		.height		= 1944,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_14,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_1080P,
		.def_start_x	= 4+8,
		.def_start_y	= 1+6+8+8+8,
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
		.max_fps	= AMBA_VIDEO_FPS_120,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_720P,
		.def_start_x	= 4+8,
		.def_start_y	= 1+4+4+6+4,
		.def_width	= 1280,
		.def_height	= 720,
		/* sensor mode */
		.device_mode	= 4,
		.pll_idx	= 0,
		.width		= 1280,
		.height		= 720,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_120,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_120,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_3072_2048,
		.def_start_x	= 4+8,
		.def_start_y	= 1+6+8+8+8,
		.def_width	= 3072,
		.def_height	= 2048,
		/* sensor mode */
		.device_mode	= 3,
		.pll_idx	= 1,
		.width		= 3072,
		.height		= 2048,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.max_fps	= AMBA_VIDEO_FPS_60,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_2560_2048,
		.def_start_x	= 4+8,
		.def_start_y	= 1+6+8+8+8,
		.def_width	= 2560,
		.def_height	= 2048,
		/* sensor mode */
		.device_mode	= 5,
		.pll_idx	= 1,
		.width		= 2560,
		.height		= 2048,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_14,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_3072_1728,
		.def_start_x	= 4+8,
		.def_start_y	= 1+6+8+8+8,
		.def_width	= 3072,
		.def_height	= 1728,
		/* sensor mode */
		.device_mode	= 6,
		.pll_idx	= 0,
		.width		= 3072,
		.height		= 1728,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_14,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_3072_1728,
		.def_start_x	= 4+8,
		.def_start_y	= 1+6+8+8+8,
		.def_width	= 3072,
		.def_height	= 1728,
		/* sensor mode */
		.device_mode	= 7,
		.pll_idx	= 0,
		.width		= 3072,
		.height		= 1728,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_60,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_QSXGA,
		.def_start_x	= 4+8,
		.def_start_y	= 1+6+8+8+8,
		.def_width	= 2592,
		.def_height	= 1944,
		/* sensor mode */
		.device_mode	= 8,
		.pll_idx	= 1,
		.width		= 2592,
		.height		= 1944,
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
};

static struct vin_reg_16_8 imx178_share_regs[] = {
	/* do not change */
#ifdef CONFIG_SENSOR_IMX178_SPI
	{0x0211, 0x00},
	{0x021B, 0x00},
	{0x0237, 0x08},
	{0x0238, 0x00},
	{0x0239, 0x00},
	{0x02AD, 0x49},
	{0x02AF, 0x54},
	{0x02B0, 0x33},
	{0x02B3, 0x0A},
	{0x02C4, 0x30},
	{0x0303, 0x03},
	{0x0304, 0x08},
	{0x0307, 0x10},
	{0x030F, 0x01},
	{0x04E5, 0x06},
	{0x04E6, 0x00},
	{0x04E7, 0x1F},
	{0x04E8, 0x00},
	{0x04E9, 0x00},
	{0x04EA, 0x00},
	{0x04EB, 0x00},
	{0x04EC, 0x00},
	{0x04EE, 0x00},
	{0x04F2, 0x02},
	{0x04F4, 0x00},
	{0x04F5, 0x00},
	{0x04F6, 0x00},
	{0x04F7, 0x00},
	{0x04F8, 0x00},
	{0x04FC, 0x02},
	{0x0510, 0x11},
	{0x0538, 0x81},
	{0x053D, 0x00},
	{0x0562, 0x00},
	{0x056B, 0x02},
	{0x056E, 0x11},
	{0x05B4, 0xFE},
	{0x05B5, 0x06},
	{0x05B9, 0x00},
#else
	{0x3011, 0x00},
	{0x301B, 0x00},
	{0x3037, 0x08},
	{0x3038, 0x00},
	{0x3039, 0x00},
	{0x30AD, 0x49},
	{0x30AF, 0x54},
	{0x30B0, 0x33},
	{0x30B3, 0x0A},
	{0x30C4, 0x30},
	{0x3103, 0x03},
	{0x3104, 0x08},
	{0x3107, 0x10},
	{0x310F, 0x01},
	{0x32E5, 0x06},
	{0x32E6, 0x00},
	{0x32E7, 0x1F},
	{0x32E8, 0x00},
	{0x32E9, 0x00},
	{0x32EA, 0x00},
	{0x32EB, 0x00},
	{0x32EC, 0x00},
	{0x32EE, 0x00},
	{0x32F2, 0x02},
	{0x32F4, 0x00},
	{0x32F5, 0x00},
	{0x32F6, 0x00},
	{0x32F7, 0x00},
	{0x32F8, 0x00},
	{0x32FC, 0x02},
	{0x3310, 0x11},
	{0x3338, 0x81},
	{0x333D, 0x00},
	{0x3362, 0x00},
	{0x336B, 0x02},
	{0x336E, 0x11},
	{0x33B4, 0xFE},
	{0x33B5, 0x06},
	{0x33B9, 0x00},
#endif
};

#define IMX178_GAIN_MAX_DB	0x1E0 /* low light: 48DB, high light:51DB */
#define IMX178_GAIN_ROWS		481
