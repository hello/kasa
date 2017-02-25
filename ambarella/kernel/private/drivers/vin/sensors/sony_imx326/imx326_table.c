/*
 * kernel/private/drivers/vin/sensors/sony_imx326/imx326_table.c
 *
 * History:
 *    2016/10/17 - [Hao Zeng] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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

static struct vin_video_pll imx326_plls[] = {
	{0, 72000000, 72000000},
};

static struct vin_reg_16_8 imx326_pll_regs[][6] = {
	{
		{IMX326_PLRD1_LSB, 0x80},
		{IMX326_PLRD1_MSB, 0x00},
		{IMX326_PLRD2, 0x03},
		{IMX326_PLRD3, 0x68},
		{IMX326_PLRD4, 0x03},
		{IMX326_PLRD5, 0x02},
	},
};

static struct vin_reg_16_8 imx326_linear_mode_regs[][39] = {
	{	/* mode 0: 3072x2048 6ch 12bits 30fps */
		{IMX326_REG_03, 0x22},
		{IMX326_MDSEL1, 0x00},
		{IMX326_MDSEL2, 0x07},
		{IMX326_MDSEL3, 0x00},
		{IMX326_MDSEL4, 0x02},
		{IMX326_HTRIMMING_EN, 0x01},
		{IMX326_HTRIMMING_START_LSB, 0x80},
		{IMX326_HTRIMMING_START_MSB, 0x01},
		{IMX326_HTRIMMING_END_LSB, 0x98},
		{IMX326_HTRIMMING_END_MSB, 0x0D},
		{IMX326_VWIDCUTEN, 0x01},
		{IMX326_VWIDCUT_LSB, 0x38},
		{IMX326_VWIDCUT_MSB, 0x00},
		{IMX326_VWINPOS_LSB, 0x1C},
		{IMX326_VWINPOS_MSB, 0x00},
		{IMX326_VCUTMODE, 0x00},
		{IMX326_PSMOVEN, 0x01},
		{IMX326_MDPLS01_LSB, 0xFF},
		{IMX326_MDPLS01_MSB, 0x01},
		{IMX326_MDPLS02_LSB, 0xFF},
		{IMX326_MDPLS02_MSB, 0x01},
		{IMX326_MDPLS03, 0x0F},
		{IMX326_MDPLS04, 0x00},
		{IMX326_MDPLS05, 0x00},
		{IMX326_MDPLS06, 0x00},
		{IMX326_MDPLS07, 0x00},
		{IMX326_MDPLS08, 0x00},
		{IMX326_MDPLS09, 0x1F},
		{IMX326_MDPLS10, 0x1F},
		{IMX326_MDPLS11, 0x0F},
		{IMX326_MDPLS12, 0x00},
		{IMX326_MDPLS13, 0x00},
		{IMX326_MDPLS14, 0x00},
		{IMX326_MDPLS15, 0x00},
		{IMX326_MDPLS16, 0x01},
		{IMX326_MDPLS17, 0x07},
		/* DOL related */
		{IMX326_DOLMODE, 0x00},
		{IMX326_DOLSET1, 0x30},
		{IMX326_DOLSET2, 0x00},
	},
	{	/* mode 1: 3072x2048 8ch 10bits 60fps */
		{IMX326_REG_03, 0x11},
		{IMX326_MDSEL1, 0x00},
		{IMX326_MDSEL2, 0x01},
		{IMX326_MDSEL3, 0x00},
		{IMX326_MDSEL4, 0x02},
		{IMX326_HTRIMMING_EN, 0x01},
		{IMX326_HTRIMMING_START_LSB, 0x80},
		{IMX326_HTRIMMING_START_MSB, 0x01},
		{IMX326_HTRIMMING_END_LSB, 0x98},
		{IMX326_HTRIMMING_END_MSB, 0x0D},
		{IMX326_VWIDCUTEN, 0x01},
		{IMX326_VWIDCUT_LSB, 0x38},
		{IMX326_VWIDCUT_MSB, 0x00},
		{IMX326_VWINPOS_LSB, 0x1C},
		{IMX326_VWINPOS_MSB, 0x00},
		{IMX326_VCUTMODE, 0x00},
		{IMX326_PSMOVEN, 0x01},
		{IMX326_MDPLS01_LSB, 0x0A},
		{IMX326_MDPLS01_MSB, 0x00},
		{IMX326_MDPLS02_LSB, 0x16},
		{IMX326_MDPLS02_MSB, 0x00},
		{IMX326_MDPLS03, 0x0E},
		{IMX326_MDPLS04, 0x1F},
		{IMX326_MDPLS05, 0x01},
		{IMX326_MDPLS06, 0x01},
		{IMX326_MDPLS07, 0x01},
		{IMX326_MDPLS08, 0x01},
		{IMX326_MDPLS09, 0x00},
		{IMX326_MDPLS10, 0x00},
		{IMX326_MDPLS11, 0x0E},
		{IMX326_MDPLS12, 0x1B},
		{IMX326_MDPLS13, 0x1A},
		{IMX326_MDPLS14, 0x19},
		{IMX326_MDPLS15, 0x17},
		{IMX326_MDPLS16, 0x01},
		{IMX326_MDPLS17, 0x05},
		/* DOL related */
		{IMX326_DOLMODE, 0x00},
		{IMX326_DOLSET1, 0x30},
		{IMX326_DOLSET2, 0x00},
	},
	{	/* mode 2: 2592x1944 6ch 12bits 30fps */
		{IMX326_REG_03, 0x22},
		{IMX326_MDSEL1, 0x00},
		{IMX326_MDSEL2, 0x07},
		{IMX326_MDSEL3, 0x00},
		{IMX326_MDSEL4, 0xA2},
		{IMX326_HTRIMMING_EN, 0x01},
		{IMX326_HTRIMMING_START_LSB, 0x70},
		{IMX326_HTRIMMING_START_MSB, 0x02},
		{IMX326_HTRIMMING_END_LSB, 0xA8},
		{IMX326_HTRIMMING_END_MSB, 0x0C},
		{IMX326_VWIDCUTEN, 0x01},
		{IMX326_VWIDCUT_LSB, 0x6C},
		{IMX326_VWIDCUT_MSB, 0x00},
		{IMX326_VWINPOS_LSB, 0x36 - 0x1C},
		{IMX326_VWINPOS_MSB, 0x00},
		{IMX326_VCUTMODE, 0x00},
		{IMX326_PSMOVEN, 0x01},
		{IMX326_MDPLS01_LSB, 0xFF},
		{IMX326_MDPLS01_MSB, 0x01},
		{IMX326_MDPLS02_LSB, 0xFF},
		{IMX326_MDPLS02_MSB, 0x01},
		{IMX326_MDPLS03, 0x0F},
		{IMX326_MDPLS04, 0x00},
		{IMX326_MDPLS05, 0x00},
		{IMX326_MDPLS06, 0x00},
		{IMX326_MDPLS07, 0x00},
		{IMX326_MDPLS08, 0x00},
		{IMX326_MDPLS09, 0x1F},
		{IMX326_MDPLS10, 0x1F},
		{IMX326_MDPLS11, 0x0F},
		{IMX326_MDPLS12, 0x00},
		{IMX326_MDPLS13, 0x00},
		{IMX326_MDPLS14, 0x00},
		{IMX326_MDPLS15, 0x00},
		{IMX326_MDPLS16, 0x01},
		{IMX326_MDPLS17, 0x07},
		/* DOL related */
		{IMX326_DOLMODE, 0x00},
		{IMX326_DOLSET1, 0x30},
		{IMX326_DOLSET2, 0x00},
	},
	{	/* mode 3: 2592x1944 8ch 10bits 60fps */
		{IMX326_REG_03, 0x11},
		{IMX326_MDSEL1, 0x00},
		{IMX326_MDSEL2, 0x01},
		{IMX326_MDSEL3, 0x00},
		{IMX326_MDSEL4, 0xA2},
		{IMX326_HTRIMMING_EN, 0x01},
		{IMX326_HTRIMMING_START_LSB, 0x70},
		{IMX326_HTRIMMING_START_MSB, 0x02},
		{IMX326_HTRIMMING_END_LSB, 0xA8},
		{IMX326_HTRIMMING_END_MSB, 0x0C},
		{IMX326_VWIDCUTEN, 0x01},
		{IMX326_VWIDCUT_LSB, 0x6C},
		{IMX326_VWIDCUT_MSB, 0x00},
		{IMX326_VWINPOS_LSB, 0x36 - 0x1C},
		{IMX326_VWINPOS_MSB, 0x00},
		{IMX326_VCUTMODE, 0x00},
		{IMX326_PSMOVEN, 0x01},
		{IMX326_MDPLS01_LSB, 0x0A},
		{IMX326_MDPLS01_MSB, 0x00},
		{IMX326_MDPLS02_LSB, 0x16},
		{IMX326_MDPLS02_MSB, 0x00},
		{IMX326_MDPLS03, 0x0E},
		{IMX326_MDPLS04, 0x1F},
		{IMX326_MDPLS05, 0x01},
		{IMX326_MDPLS06, 0x01},
		{IMX326_MDPLS07, 0x01},
		{IMX326_MDPLS08, 0x01},
		{IMX326_MDPLS09, 0x00},
		{IMX326_MDPLS10, 0x00},
		{IMX326_MDPLS11, 0x0E},
		{IMX326_MDPLS12, 0x1B},
		{IMX326_MDPLS13, 0x1A},
		{IMX326_MDPLS14, 0x19},
		{IMX326_MDPLS15, 0x17},
		{IMX326_MDPLS16, 0x01},
		{IMX326_MDPLS17, 0x05},
		/* DOL related */
		{IMX326_DOLMODE, 0x00},
		{IMX326_DOLSET1, 0x30},
		{IMX326_DOLSET2, 0x00},
	},
	{	/* mode 4: 3072x1728 6ch 12bits 30fps */
		{IMX326_REG_03, 0x22},
		{IMX326_MDSEL1, 0x00},
		{IMX326_MDSEL2, 0x07},
		{IMX326_MDSEL3, 0x00},
		{IMX326_MDSEL4, 0xA2},
		{IMX326_HTRIMMING_EN, 0x01},
		{IMX326_HTRIMMING_START_LSB, 0x80},
		{IMX326_HTRIMMING_START_MSB, 0x01},
		{IMX326_HTRIMMING_END_LSB, 0x98},
		{IMX326_HTRIMMING_END_MSB, 0x0D},
		{IMX326_VWIDCUTEN, 0x01},
		{IMX326_VWIDCUT_LSB, 0xD8},
		{IMX326_VWIDCUT_MSB, 0x00},
		{IMX326_VWINPOS_LSB, 0x6C - 0x1C},
		{IMX326_VWINPOS_MSB, 0x00},
		{IMX326_VCUTMODE, 0x00},
		{IMX326_PSMOVEN, 0x01},
		{IMX326_MDPLS01_LSB, 0xFF},
		{IMX326_MDPLS01_MSB, 0x01},
		{IMX326_MDPLS02_LSB, 0xFF},
		{IMX326_MDPLS02_MSB, 0x01},
		{IMX326_MDPLS03, 0x0F},
		{IMX326_MDPLS04, 0x00},
		{IMX326_MDPLS05, 0x00},
		{IMX326_MDPLS06, 0x00},
		{IMX326_MDPLS07, 0x00},
		{IMX326_MDPLS08, 0x00},
		{IMX326_MDPLS09, 0x1F},
		{IMX326_MDPLS10, 0x1F},
		{IMX326_MDPLS11, 0x0F},
		{IMX326_MDPLS12, 0x00},
		{IMX326_MDPLS13, 0x00},
		{IMX326_MDPLS14, 0x00},
		{IMX326_MDPLS15, 0x00},
		{IMX326_MDPLS16, 0x01},
		{IMX326_MDPLS17, 0x07},
		/* DOL related */
		{IMX326_DOLMODE, 0x00},
		{IMX326_DOLSET1, 0x30},
		{IMX326_DOLSET2, 0x00},
	},
	{	/* mode 5: 3072x1728 8ch 10bits 60fps */
		{IMX326_REG_03, 0x11},
		{IMX326_MDSEL1, 0x00},
		{IMX326_MDSEL2, 0x01},
		{IMX326_MDSEL3, 0x00},
		{IMX326_MDSEL4, 0xA2},
		{IMX326_HTRIMMING_EN, 0x01},
		{IMX326_HTRIMMING_START_LSB, 0x80},
		{IMX326_HTRIMMING_START_MSB, 0x01},
		{IMX326_HTRIMMING_END_LSB, 0x98},
		{IMX326_HTRIMMING_END_MSB, 0x0D},
		{IMX326_VWIDCUTEN, 0x01},
		{IMX326_VWIDCUT_LSB, 0xD8},
		{IMX326_VWIDCUT_MSB, 0x00},
		{IMX326_VWINPOS_LSB, 0x6C - 0x1C},
		{IMX326_VWINPOS_MSB, 0x00},
		{IMX326_VCUTMODE, 0x00},
		{IMX326_PSMOVEN, 0x01},
		{IMX326_MDPLS01_LSB, 0x0A},
		{IMX326_MDPLS01_MSB, 0x00},
		{IMX326_MDPLS02_LSB, 0x16},
		{IMX326_MDPLS02_MSB, 0x00},
		{IMX326_MDPLS03, 0x0E},
		{IMX326_MDPLS04, 0x1F},
		{IMX326_MDPLS05, 0x01},
		{IMX326_MDPLS06, 0x01},
		{IMX326_MDPLS07, 0x01},
		{IMX326_MDPLS08, 0x01},
		{IMX326_MDPLS09, 0x00},
		{IMX326_MDPLS10, 0x00},
		{IMX326_MDPLS11, 0x0E},
		{IMX326_MDPLS12, 0x1B},
		{IMX326_MDPLS13, 0x1A},
		{IMX326_MDPLS14, 0x19},
		{IMX326_MDPLS15, 0x17},
		{IMX326_MDPLS16, 0x01},
		{IMX326_MDPLS17, 0x05},
		/* DOL related */
		{IMX326_DOLMODE, 0x00},
		{IMX326_DOLSET1, 0x30},
		{IMX326_DOLSET2, 0x00},
	},
	{	/* mode 6: 2048x2048 6ch 12bits 30fps */
		{IMX326_REG_03, 0x22},
		{IMX326_MDSEL1, 0x00},
		{IMX326_MDSEL2, 0x07},
		{IMX326_MDSEL3, 0x00},
		{IMX326_MDSEL4, 0x02},
		{IMX326_HTRIMMING_EN, 0x01},
		{IMX326_HTRIMMING_START_LSB, 0x80},
		{IMX326_HTRIMMING_START_MSB, 0x03},
		{IMX326_HTRIMMING_END_LSB, 0x98},
		{IMX326_HTRIMMING_END_MSB, 0x0B},
		{IMX326_VWIDCUTEN, 0x01},
		{IMX326_VWIDCUT_LSB, 0x38},
		{IMX326_VWIDCUT_MSB, 0x00},
		{IMX326_VWINPOS_LSB, 0x1C},
		{IMX326_VWINPOS_MSB, 0x00},
		{IMX326_VCUTMODE, 0x00},
		{IMX326_PSMOVEN, 0x01},
		{IMX326_MDPLS01_LSB, 0xFF},
		{IMX326_MDPLS01_MSB, 0x01},
		{IMX326_MDPLS02_LSB, 0xFF},
		{IMX326_MDPLS02_MSB, 0x01},
		{IMX326_MDPLS03, 0x0F},
		{IMX326_MDPLS04, 0x00},
		{IMX326_MDPLS05, 0x00},
		{IMX326_MDPLS06, 0x00},
		{IMX326_MDPLS07, 0x00},
		{IMX326_MDPLS08, 0x00},
		{IMX326_MDPLS09, 0x1F},
		{IMX326_MDPLS10, 0x1F},
		{IMX326_MDPLS11, 0x0F},
		{IMX326_MDPLS12, 0x00},
		{IMX326_MDPLS13, 0x00},
		{IMX326_MDPLS14, 0x00},
		{IMX326_MDPLS15, 0x00},
		{IMX326_MDPLS16, 0x01},
		{IMX326_MDPLS17, 0x07},
		/* DOL related */
		{IMX326_DOLMODE, 0x00},
		{IMX326_DOLSET1, 0x30},
		{IMX326_DOLSET2, 0x00},
	},
	{	/* mode 7: 2048x2048 8ch 10bits 60fps */
		{IMX326_REG_03, 0x11},
		{IMX326_MDSEL1, 0x00},
		{IMX326_MDSEL2, 0x01},
		{IMX326_MDSEL3, 0x00},
		{IMX326_MDSEL4, 0x02},
		{IMX326_HTRIMMING_EN, 0x01},
		{IMX326_HTRIMMING_START_LSB, 0x80},
		{IMX326_HTRIMMING_START_MSB, 0x03},
		{IMX326_HTRIMMING_END_LSB, 0x98},
		{IMX326_HTRIMMING_END_MSB, 0x0B},
		{IMX326_VWIDCUTEN, 0x01},
		{IMX326_VWIDCUT_LSB, 0x38},
		{IMX326_VWIDCUT_MSB, 0x00},
		{IMX326_VWINPOS_LSB, 0x1C},
		{IMX326_VWINPOS_MSB, 0x00},
		{IMX326_VCUTMODE, 0x00},
		{IMX326_PSMOVEN, 0x01},
		{IMX326_MDPLS01_LSB, 0x0A},
		{IMX326_MDPLS01_MSB, 0x00},
		{IMX326_MDPLS02_LSB, 0x16},
		{IMX326_MDPLS02_MSB, 0x00},
		{IMX326_MDPLS03, 0x0E},
		{IMX326_MDPLS04, 0x1F},
		{IMX326_MDPLS05, 0x01},
		{IMX326_MDPLS06, 0x01},
		{IMX326_MDPLS07, 0x01},
		{IMX326_MDPLS08, 0x01},
		{IMX326_MDPLS09, 0x00},
		{IMX326_MDPLS10, 0x00},
		{IMX326_MDPLS11, 0x0E},
		{IMX326_MDPLS12, 0x1B},
		{IMX326_MDPLS13, 0x1A},
		{IMX326_MDPLS14, 0x19},
		{IMX326_MDPLS15, 0x17},
		{IMX326_MDPLS16, 0x01},
		{IMX326_MDPLS17, 0x05},
		/* DOL related */
		{IMX326_DOLMODE, 0x00},
		{IMX326_DOLSET1, 0x30},
		{IMX326_DOLSET2, 0x00},
	},
	{	/* mode 8: 2688x1520 6ch 12bits 30fps */
		{IMX326_REG_03, 0x22},
		{IMX326_MDSEL1, 0x00},
		{IMX326_MDSEL2, 0x07},
		{IMX326_MDSEL3, 0x00},
		{IMX326_MDSEL4, 0xA2},
		{IMX326_HTRIMMING_EN, 0x01},
		{IMX326_HTRIMMING_START_LSB, 0x40},
		{IMX326_HTRIMMING_START_MSB, 0x02},
		{IMX326_HTRIMMING_END_LSB, 0xD8},
		{IMX326_HTRIMMING_END_MSB, 0x0C},
		{IMX326_VWIDCUTEN, 0x01},
		{IMX326_VWIDCUT_LSB, 0x40},
		{IMX326_VWIDCUT_MSB, 0x01},
		{IMX326_VWINPOS_LSB, 0xA0 - 0x1C},
		{IMX326_VWINPOS_MSB, 0x00},
		{IMX326_VCUTMODE, 0x00},
		{IMX326_PSMOVEN, 0x01},
		{IMX326_MDPLS01_LSB, 0xFF},
		{IMX326_MDPLS01_MSB, 0x01},
		{IMX326_MDPLS02_LSB, 0xFF},
		{IMX326_MDPLS02_MSB, 0x01},
		{IMX326_MDPLS03, 0x0F},
		{IMX326_MDPLS04, 0x00},
		{IMX326_MDPLS05, 0x00},
		{IMX326_MDPLS06, 0x00},
		{IMX326_MDPLS07, 0x00},
		{IMX326_MDPLS08, 0x00},
		{IMX326_MDPLS09, 0x1F},
		{IMX326_MDPLS10, 0x1F},
		{IMX326_MDPLS11, 0x0F},
		{IMX326_MDPLS12, 0x00},
		{IMX326_MDPLS13, 0x00},
		{IMX326_MDPLS14, 0x00},
		{IMX326_MDPLS15, 0x00},
		{IMX326_MDPLS16, 0x01},
		{IMX326_MDPLS17, 0x07},
		/* DOL related */
		{IMX326_DOLMODE, 0x00},
		{IMX326_DOLSET1, 0x30},
		{IMX326_DOLSET2, 0x00},
	},
	{	/* mode 9: 2688x1520 8ch 10bits 60fps */
		{IMX326_REG_03, 0x11},
		{IMX326_MDSEL1, 0x00},
		{IMX326_MDSEL2, 0x01},
		{IMX326_MDSEL3, 0x00},
		{IMX326_MDSEL4, 0xA2},
		{IMX326_HTRIMMING_EN, 0x01},
		{IMX326_HTRIMMING_START_LSB, 0x40},
		{IMX326_HTRIMMING_START_MSB, 0x02},
		{IMX326_HTRIMMING_END_LSB, 0xD8},
		{IMX326_HTRIMMING_END_MSB, 0x0C},
		{IMX326_VWIDCUTEN, 0x01},
		{IMX326_VWIDCUT_LSB, 0x40},
		{IMX326_VWIDCUT_MSB, 0x01},
		{IMX326_VWINPOS_LSB, 0xA0 - 0x1C},
		{IMX326_VWINPOS_MSB, 0x00},
		{IMX326_VCUTMODE, 0x00},
		{IMX326_PSMOVEN, 0x01},
		{IMX326_MDPLS01_LSB, 0x0A},
		{IMX326_MDPLS01_MSB, 0x00},
		{IMX326_MDPLS02_LSB, 0x16},
		{IMX326_MDPLS02_MSB, 0x00},
		{IMX326_MDPLS03, 0x0E},
		{IMX326_MDPLS04, 0x1F},
		{IMX326_MDPLS05, 0x01},
		{IMX326_MDPLS06, 0x01},
		{IMX326_MDPLS07, 0x01},
		{IMX326_MDPLS08, 0x01},
		{IMX326_MDPLS09, 0x00},
		{IMX326_MDPLS10, 0x00},
		{IMX326_MDPLS11, 0x0E},
		{IMX326_MDPLS12, 0x1B},
		{IMX326_MDPLS13, 0x1A},
		{IMX326_MDPLS14, 0x19},
		{IMX326_MDPLS15, 0x17},
		{IMX326_MDPLS16, 0x01},
		{IMX326_MDPLS17, 0x05},
		/* DOL related */
		{IMX326_DOLMODE, 0x00},
		{IMX326_DOLSET1, 0x30},
		{IMX326_DOLSET2, 0x00},
	},
};

static struct vin_reg_16_8 imx326_hdr_mode_regs[][45] = {
	{	/* DOL mode 0: 3072x2048 8ch 10bits 30fps */
		{IMX326_REG_03, 0x11},
		{IMX326_MDSEL1, 0x05},
		{IMX326_MDSEL2, 0x01},
		{IMX326_MDSEL3, 0x00},
		{IMX326_MDSEL4, 0xA2},/* for VCUT offset, {IMX326_MDSEL4, 0x02}, */
		{IMX326_HTRIMMING_EN, 0x01},
		{IMX326_HTRIMMING_START_LSB, 0x80},
		{IMX326_HTRIMMING_START_MSB, 0x01},
		{IMX326_HTRIMMING_END_LSB, 0x98},
		{IMX326_HTRIMMING_END_MSB, 0x0D},
		{IMX326_VWIDCUTEN, 0x01},
		{IMX326_VWIDCUT_LSB, 0x38},
		{IMX326_VWIDCUT_MSB, 0x00},
		{IMX326_VWINPOS_LSB, 0x1C - 0x1C},
		{IMX326_VWINPOS_MSB, 0x00},
		{IMX326_VCUTMODE, 0x00},
		{IMX326_PSMOVEN, 0x01},
		{IMX326_MDPLS01_LSB, 0x0A},
		{IMX326_MDPLS01_MSB, 0x00},
		{IMX326_MDPLS02_LSB, 0x16},
		{IMX326_MDPLS02_MSB, 0x00},
		{IMX326_MDPLS03, 0x0E},
		{IMX326_MDPLS04, 0x1F},
		{IMX326_MDPLS05, 0x01},
		{IMX326_MDPLS06, 0x01},
		{IMX326_MDPLS07, 0x01},
		{IMX326_MDPLS08, 0x01},
		{IMX326_MDPLS09, 0x00},
		{IMX326_MDPLS10, 0x00},
		{IMX326_MDPLS11, 0x0E},
		{IMX326_MDPLS12, 0x1B},
		{IMX326_MDPLS13, 0x1A},
		{IMX326_MDPLS14, 0x19},
		{IMX326_MDPLS15, 0x17},
		{IMX326_MDPLS16, 0x01},
		{IMX326_MDPLS17, 0x05},
		/* DOL related */
		{IMX326_DOLMODE, 0x01},
		{IMX326_DOLSET1, 0x31},
		{IMX326_DOLSET2, 0x01},
		{IMX326_HCYCLE_LSB, IMX326_HCYCLE & 0xFF},
		{IMX326_HCYCLE_MSB, IMX326_HCYCLE >> 8},
		{IMX326_SHR_DOL1_LSB, 0x06},
		{IMX326_SHR_DOL1_MSB, 0x00},
		{IMX326_SHR_DOL2_LSB, 0xEE},
		{IMX326_SHR_DOL2_MSB, 0x02},
	},
	{	/* DOL mode 1: 2592x1944 8ch 10bits 30fps */
		{IMX326_REG_03, 0x11},
		{IMX326_MDSEL1, 0x05},
		{IMX326_MDSEL2, 0x01},
		{IMX326_MDSEL3, 0x00},
		{IMX326_MDSEL4, 0xA2},/* for VCUT offset, {IMX326_MDSEL4, 0x02}, */
		{IMX326_HTRIMMING_EN, 0x01},
		{IMX326_HTRIMMING_START_LSB, 0x70},
		{IMX326_HTRIMMING_START_MSB, 0x02},
		{IMX326_HTRIMMING_END_LSB, 0xA8},
		{IMX326_HTRIMMING_END_MSB, 0x0C},
		{IMX326_VWIDCUTEN, 0x01},
		{IMX326_VWIDCUT_LSB, 0x6C},
		{IMX326_VWIDCUT_MSB, 0x00},
		{IMX326_VWINPOS_LSB, 0x36 - 0x1C},
		{IMX326_VWINPOS_MSB, 0x00},
		{IMX326_VCUTMODE, 0x00},
		{IMX326_PSMOVEN, 0x01},
		{IMX326_MDPLS01_LSB, 0x0A},
		{IMX326_MDPLS01_MSB, 0x00},
		{IMX326_MDPLS02_LSB, 0x16},
		{IMX326_MDPLS02_MSB, 0x00},
		{IMX326_MDPLS03, 0x0E},
		{IMX326_MDPLS04, 0x1F},
		{IMX326_MDPLS05, 0x01},
		{IMX326_MDPLS06, 0x01},
		{IMX326_MDPLS07, 0x01},
		{IMX326_MDPLS08, 0x01},
		{IMX326_MDPLS09, 0x00},
		{IMX326_MDPLS10, 0x00},
		{IMX326_MDPLS11, 0x0E},
		{IMX326_MDPLS12, 0x1B},
		{IMX326_MDPLS13, 0x1A},
		{IMX326_MDPLS14, 0x19},
		{IMX326_MDPLS15, 0x17},
		{IMX326_MDPLS16, 0x01},
		{IMX326_MDPLS17, 0x05},
		/* DOL related */
		{IMX326_DOLMODE, 0x01},
		{IMX326_DOLSET1, 0x31},
		{IMX326_DOLSET2, 0x01},
		{IMX326_HCYCLE_LSB, IMX326_HCYCLE & 0xFF},
		{IMX326_HCYCLE_MSB, IMX326_HCYCLE >> 8},
		{IMX326_SHR_DOL1_LSB, 0x06},
		{IMX326_SHR_DOL1_MSB, 0x00},
		{IMX326_SHR_DOL2_LSB, 0xEE},
		{IMX326_SHR_DOL2_MSB, 0x02},
	},
	{	/* DOL mode 2: 3072x1728 8ch 10bits 30fps */
		{IMX326_REG_03, 0x11},
		{IMX326_MDSEL1, 0x05},
		{IMX326_MDSEL2, 0x01},
		{IMX326_MDSEL3, 0x00},
		{IMX326_MDSEL4, 0xA2},/* for VCUT offset, {IMX326_MDSEL4, 0x02}, */
		{IMX326_HTRIMMING_EN, 0x01},
		{IMX326_HTRIMMING_START_LSB, 0x80},
		{IMX326_HTRIMMING_START_MSB, 0x01},
		{IMX326_HTRIMMING_END_LSB, 0x98},
		{IMX326_HTRIMMING_END_MSB, 0x0D},
		{IMX326_VWIDCUTEN, 0x01},
		{IMX326_VWIDCUT_LSB, 0xD8},
		{IMX326_VWIDCUT_MSB, 0x00},
		{IMX326_VWINPOS_LSB, 0x6C - 0x1C},
		{IMX326_VWINPOS_MSB, 0x00},
		{IMX326_VCUTMODE, 0x00},
		{IMX326_PSMOVEN, 0x01},
		{IMX326_MDPLS01_LSB, 0x0A},
		{IMX326_MDPLS01_MSB, 0x00},
		{IMX326_MDPLS02_LSB, 0x16},
		{IMX326_MDPLS02_MSB, 0x00},
		{IMX326_MDPLS03, 0x0E},
		{IMX326_MDPLS04, 0x1F},
		{IMX326_MDPLS05, 0x01},
		{IMX326_MDPLS06, 0x01},
		{IMX326_MDPLS07, 0x01},
		{IMX326_MDPLS08, 0x01},
		{IMX326_MDPLS09, 0x00},
		{IMX326_MDPLS10, 0x00},
		{IMX326_MDPLS11, 0x0E},
		{IMX326_MDPLS12, 0x1B},
		{IMX326_MDPLS13, 0x1A},
		{IMX326_MDPLS14, 0x19},
		{IMX326_MDPLS15, 0x17},
		{IMX326_MDPLS16, 0x01},
		{IMX326_MDPLS17, 0x05},
		/* DOL related */
		{IMX326_DOLMODE, 0x01},
		{IMX326_DOLSET1, 0x31},
		{IMX326_DOLSET2, 0x01},
		{IMX326_HCYCLE_LSB, IMX326_HCYCLE & 0xFF},
		{IMX326_HCYCLE_MSB, IMX326_HCYCLE >> 8},
		{IMX326_SHR_DOL1_LSB, 0x06},
		{IMX326_SHR_DOL1_MSB, 0x00},
		{IMX326_SHR_DOL2_LSB, 0xEE},
		{IMX326_SHR_DOL2_MSB, 0x02},
	},
	{	/* DOL mode 3: 2048x2048 8ch 10bits 30fps */
		{IMX326_REG_03, 0x11},
		{IMX326_MDSEL1, 0x05},
		{IMX326_MDSEL2, 0x01},
		{IMX326_MDSEL3, 0x00},
		{IMX326_MDSEL4, 0xA2},/* for VCUT offset, {IMX326_MDSEL4, 0x02}, */
		{IMX326_HTRIMMING_EN, 0x01},
		{IMX326_HTRIMMING_START_LSB, 0x80},
		{IMX326_HTRIMMING_START_MSB, 0x03},
		{IMX326_HTRIMMING_END_LSB, 0x98},
		{IMX326_HTRIMMING_END_MSB, 0x0B},
		{IMX326_VWIDCUTEN, 0x01},
		{IMX326_VWIDCUT_LSB, 0x38},
		{IMX326_VWIDCUT_MSB, 0x00},
		{IMX326_VWINPOS_LSB, 0x1C - 0x1C},
		{IMX326_VWINPOS_MSB, 0x00},
		{IMX326_VCUTMODE, 0x00},
		{IMX326_PSMOVEN, 0x01},
		{IMX326_MDPLS01_LSB, 0x0A},
		{IMX326_MDPLS01_MSB, 0x00},
		{IMX326_MDPLS02_LSB, 0x16},
		{IMX326_MDPLS02_MSB, 0x00},
		{IMX326_MDPLS03, 0x0E},
		{IMX326_MDPLS04, 0x1F},
		{IMX326_MDPLS05, 0x01},
		{IMX326_MDPLS06, 0x01},
		{IMX326_MDPLS07, 0x01},
		{IMX326_MDPLS08, 0x01},
		{IMX326_MDPLS09, 0x00},
		{IMX326_MDPLS10, 0x00},
		{IMX326_MDPLS11, 0x0E},
		{IMX326_MDPLS12, 0x1B},
		{IMX326_MDPLS13, 0x1A},
		{IMX326_MDPLS14, 0x19},
		{IMX326_MDPLS15, 0x17},
		{IMX326_MDPLS16, 0x01},
		{IMX326_MDPLS17, 0x05},
		/* DOL related */
		{IMX326_DOLMODE, 0x01},
		{IMX326_DOLSET1, 0x31},
		{IMX326_DOLSET2, 0x01},
		{IMX326_HCYCLE_LSB, IMX326_HCYCLE & 0xFF},
		{IMX326_HCYCLE_MSB, IMX326_HCYCLE >> 8},
		{IMX326_SHR_DOL1_LSB, 0x06},
		{IMX326_SHR_DOL1_MSB, 0x00},
		{IMX326_SHR_DOL2_LSB, 0xEE},
		{IMX326_SHR_DOL2_MSB, 0x02},
	},
	{	/* DOL mode 4: 2688x1520 8ch 10bits 30fps */
		{IMX326_REG_03, 0x11},
		{IMX326_MDSEL1, 0x05},
		{IMX326_MDSEL2, 0x01},
		{IMX326_MDSEL3, 0x00},
		{IMX326_MDSEL4, 0xA2},/* for VCUT offset, {IMX326_MDSEL4, 0x02}, */
		{IMX326_HTRIMMING_EN, 0x01},
		{IMX326_HTRIMMING_START_LSB, 0x40},
		{IMX326_HTRIMMING_START_MSB, 0x02},
		{IMX326_HTRIMMING_END_LSB, 0xD8},
		{IMX326_HTRIMMING_END_MSB, 0x0C},
		{IMX326_VWIDCUTEN, 0x01},
		{IMX326_VWIDCUT_LSB, 0x40},
		{IMX326_VWIDCUT_MSB, 0x01},
		{IMX326_VWINPOS_LSB, 0xA0 - 0x1C},
		{IMX326_VWINPOS_MSB, 0x00},
		{IMX326_VCUTMODE, 0x00},
		{IMX326_PSMOVEN, 0x01},
		{IMX326_MDPLS01_LSB, 0x0A},
		{IMX326_MDPLS01_MSB, 0x00},
		{IMX326_MDPLS02_LSB, 0x16},
		{IMX326_MDPLS02_MSB, 0x00},
		{IMX326_MDPLS03, 0x0E},
		{IMX326_MDPLS04, 0x1F},
		{IMX326_MDPLS05, 0x01},
		{IMX326_MDPLS06, 0x01},
		{IMX326_MDPLS07, 0x01},
		{IMX326_MDPLS08, 0x01},
		{IMX326_MDPLS09, 0x00},
		{IMX326_MDPLS10, 0x00},
		{IMX326_MDPLS11, 0x0E},
		{IMX326_MDPLS12, 0x1B},
		{IMX326_MDPLS13, 0x1A},
		{IMX326_MDPLS14, 0x19},
		{IMX326_MDPLS15, 0x17},
		{IMX326_MDPLS16, 0x01},
		{IMX326_MDPLS17, 0x05},
		/* DOL related */
		{IMX326_DOLMODE, 0x01},
		{IMX326_DOLSET1, 0x31},
		{IMX326_DOLSET2, 0x01},
		{IMX326_HCYCLE_LSB, IMX326_HCYCLE & 0xFF},
		{IMX326_HCYCLE_MSB, IMX326_HCYCLE >> 8},
		{IMX326_SHR_DOL1_LSB, 0x06},
		{IMX326_SHR_DOL1_MSB, 0x00},
		{IMX326_SHR_DOL2_LSB, 0xEE},
		{IMX326_SHR_DOL2_MSB, 0x02},
	},
};

static struct vin_video_format imx326_formats[] = {
	{
		.video_mode	= AMBA_VIDEO_MODE_3072_2048,
		.def_start_x	= 12,
		.def_start_y	= 16 + 6 + 18,
		.def_width	= 3072,
		.def_height	= 2048,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_LINEAR_MODE,
		.device_mode	= 0,
		.pll_idx	= 0,
		.width		= 3072,
		.height		= 2048,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_3072_2048,
		.def_start_x	= 12,
		.def_start_y	= 16 + 6 + 18,
		.def_width	= 3072,
		.def_height	= 2048,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_LINEAR_MODE,
		.device_mode	= 1,
		.pll_idx	= 0,
		.width		= 3072,
		.height		= 2048,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_60,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_QSXGA,
		.def_start_x	= 12,
		.def_start_y	= 16 + 6 + 18,
		.def_width	= 2592,
		.def_height	= 1944,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_LINEAR_MODE,
		.device_mode	= 2,
		.pll_idx	= 0,
		.width		= 2592,
		.height		= 1944,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_QSXGA,
		.def_start_x	= 12,
		.def_start_y	= 16 + 6 + 18,
		.def_width	= 2592,
		.def_height	= 1944,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_LINEAR_MODE,
		.device_mode	= 3,
		.pll_idx	= 0,
		.width		= 2592,
		.height		= 1944,
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
		.video_mode	= AMBA_VIDEO_MODE_3072_1728,
		.def_start_x	= 12,
		.def_start_y	= 16 + 6 + 18,
		.def_width	= 3072,
		.def_height	= 1728,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_LINEAR_MODE,
		.device_mode	= 4,
		.pll_idx	= 0,
		.width		= 3072,
		.height		= 1728,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_3072_1728,
		.def_start_x	= 12,
		.def_start_y	= 16 + 6 + 18,
		.def_width	= 3072,
		.def_height	= 1728,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_LINEAR_MODE,
		.device_mode	= 5,
		.pll_idx	= 0,
		.width		= 3072,
		.height		= 1728,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_60,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_2048_2048,
		.def_start_x	= 12,
		.def_start_y	= 16 + 6 + 18,
		.def_width	= 2048,
		.def_height	= 2048,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_LINEAR_MODE,
		.device_mode	= 6,
		.pll_idx	= 0,
		.width		= 2048,
		.height		= 2048,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_1_1,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_2048_2048,
		.def_start_x	= 12,
		.def_start_y	= 16 + 6 + 18,
		.def_width	= 2048,
		.def_height	= 2048,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_LINEAR_MODE,
		.device_mode	= 7,
		.pll_idx	= 0,
		.width		= 2048,
		.height		= 2048,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_1_1,
		.max_fps	= AMBA_VIDEO_FPS_60,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_2688_1520,
		.def_start_x	= 12,
		.def_start_y	= 16 + 6 + 18,
		.def_width	= 2688,
		.def_height	= 1520,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_LINEAR_MODE,
		.device_mode	= 8,
		.pll_idx	= 0,
		.width		= 2688,
		.height		= 1520,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_12,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_2688_1520,
		.def_start_x	= 12,
		.def_start_y	= 16 + 6 + 18,
		.def_width	= 2688,
		.def_height	= 1520,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_LINEAR_MODE,
		.device_mode	= 9,
		.pll_idx	= 0,
		.width		= 2688,
		.height		= 1520,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_60,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
	},
	/* 2X DOL HDR MODE */
	{
		.video_mode	= AMBA_VIDEO_MODE_3072_2048,
		.def_start_x	= 12,
		.def_start_y	= (16 + 6 + 18) * 2,
		.def_width	= 3072,
		.def_height	= (2048 + IMX326_6M_2X_RHS1) * 2, /* (2048 + VBP1)*2 */
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 3072,
		.act_height	= 2048,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_2X_HDR_MODE,
		.device_mode	= 0,
		.pll_idx	= 0,
		.width		= 3072 * 2,
		.height		= 2048,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
		.hdr_long_offset = 0,
		.hdr_short1_offset = IMX326_6M_2X_RHS1 * 2 + 1,/* 2 x VBP1 + 1 */
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_QSXGA,
		.def_start_x	= 12,
		.def_start_y	= (16 + 6 + 18) * 2,
		.def_width	= 2592,
		.def_height	= (1944 + IMX326_5M_2X_RHS1) * 2, /* (1944 + VBP1)*2 */
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 2592,
		.act_height	= 1944,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_2X_HDR_MODE,
		.device_mode	= 1,
		.pll_idx	= 0,
		.width		= 2592 * 2,
		.height		= 1944,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_4_3,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
		.hdr_long_offset = 0,
		.hdr_short1_offset = IMX326_5M_2X_RHS1 * 2 + 1,/* 2 x VBP1 + 1 */
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_3072_1728,
		.def_start_x	= 12,
		.def_start_y	= (16 + 6 + 18) * 2,
		.def_width	= 3072,
		.def_height	= (1728 + IMX326_5_3M_2X_RHS1) * 2, /* (1728 + VBP1)*2 */
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 3072,
		.act_height	= 1728,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_2X_HDR_MODE,
		.device_mode	= 2,
		.pll_idx	= 0,
		.width		= 3072 * 2,
		.height		= 1728,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
		.hdr_long_offset = 0,
		.hdr_short1_offset = IMX326_5_3M_2X_RHS1 * 2 + 1,/* 2 x VBP1 + 1 */
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_2048_2048,
		.def_start_x	= 12,
		.def_start_y	= (16 + 6 + 18) * 2,
		.def_width	= 2048,
		.def_height	= (2048 + IMX326_4_2M_2X_RHS1) * 2, /* (2048 + VBP1)*2 */
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 2048,
		.act_height	= 2048,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_2X_HDR_MODE,
		.device_mode	= 3,
		.pll_idx	= 0,
		.width		= 2048 * 2,
		.height		= 2048,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_1_1,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
		.hdr_long_offset = 0,
		.hdr_short1_offset = IMX326_4_2M_2X_RHS1 * 2 + 1,/* 2 x VBP1 + 1 */
	},
	{
		.video_mode	= AMBA_VIDEO_MODE_2688_1520,
		.def_start_x	= 12,
		.def_start_y	= (16 + 6 + 18) * 2,
		.def_width	= 2688,
		.def_height	= (1520 + IMX326_4M_2X_RHS1) * 2, /* (1520 + VBP1)*2 */
		.act_start_x	= 0,
		.act_start_y	= 0,
		.act_width	= 2688,
		.act_height	= 1520,
		/* sensor mode */
		.hdr_mode = AMBA_VIDEO_2X_HDR_MODE,
		.device_mode	= 4,
		.pll_idx	= 0,
		.width		= 2688 * 2,
		.height		= 1520,
		.format		= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type		= AMBA_VIDEO_TYPE_RGB_RAW,
		.bits		= AMBA_VIDEO_BITS_10,
		.ratio		= AMBA_VIDEO_RATIO_16_9,
		.max_fps	= AMBA_VIDEO_FPS_30,
		.default_fps	= AMBA_VIDEO_FPS_29_97,
		.default_agc	= 0,
		.default_shutter_time	= AMBA_VIDEO_FPS_60,
		.default_bayer_pattern	= VINDEV_BAYER_PATTERN_RG,
		.hdr_long_offset = 0,
		.hdr_short1_offset = IMX326_4M_2X_RHS1 * 2 + 1,/* 2 x VBP1 + 1 */
	},
};

static struct vin_reg_16_8 imx326_share_regs[] = {
	{IMX326_SYS_MODE, 0x00},/* Sub-LVDS */
	{IMX326_REG_010B, 0x00},
	{IMX326_PLSTMG01, 0x00},
	{IMX326_PLSTMG02, 0x03},
	{IMX326_PLSTMG03_LSB, 0x1A},
	{IMX326_PLSTMG03_MSB, 0x00},
	{IMX326_PLSTMG04, 0x02},
	{IMX326_PLSTMG05, 0x0E},
	{IMX326_PLSTMG06, 0x0E},
	{IMX326_PLSTMG07, 0x0E},
	{IMX326_PLSTMG08, 0x0E},
	{IMX326_PLSTMG09, 0x0E},
	{IMX326_PLSTMG10, 0x00},
	{IMX326_PLSTMG11, 0x05},
	{IMX326_PLSTMG12, 0x05},
	{IMX326_PLSTMG13, 0x04},
	{IMX326_PLSTMG14, 0x76},
	{IMX326_PLSTMG15, 0x01},
	{IMX326_PLSTMG16, 0x0E},
	{IMX326_PLSTMG17, 0x0E},
	{IMX326_PLSTMG18, 0x0E},
	{IMX326_PLSTMG19, 0x0E},
	{IMX326_PLSTMG20, 0x0E},
	{IMX326_PLSTMG21, 0x00},
	{IMX326_PLSTMG22, 0x00},
	{IMX326_PLSTMG23, 0x00},
	{IMX326_PLSTMG24, 0x00},
	{IMX326_MDVREV, 0x00},
};

#ifdef CONFIG_PM
static struct vin_reg_16_8 pm_regs[] = {
	{IMX326_DGAIN, 0x00},
	{IMX326_PGC_LSB, 0x00},
	{IMX326_PGC_MSB, 0x00},
	{IMX326_SHR_LSB, 0x00},
	{IMX326_SHR_MSB, 0x00},
	{IMX326_SHR_DOL1_LSB, 0x00},
	{IMX326_SHR_DOL1_MSB, 0x00},
	{IMX326_SHR_DOL2_LSB, 0x00},
	{IMX326_SHR_DOL2_MSB, 0x00},
};
#endif

#define IMX326_GAIN_ROWS	673
#define IMX326_GAIN_COLS	2
#define IMX326_GAIN_MAXDB	672

#define IMX326_GAIN_COL_AGAIN	0
#define IMX326_GAIN_COL_DGAIN	1

/* For digital gain, the step is 6dB, Max = 36dB */
static const u16 IMX326_GAIN_TABLE[IMX326_GAIN_ROWS][IMX326_GAIN_COLS] = {
	{0x0000, 0x00}, /* index:0, gain:0.00000db, again:0.00000db, dgain:0db */
	{0x0016, 0x00}, /* index:1, gain:0.09375db, again:0.09375db, dgain:0db */
	{0x002c, 0x00}, /* index:2, gain:0.18750db, again:0.18750db, dgain:0db */
	{0x0041, 0x00}, /* index:3, gain:0.28125db, again:0.28125db, dgain:0db */
	{0x0057, 0x00}, /* index:4, gain:0.37500db, again:0.37500db, dgain:0db */
	{0x006c, 0x00}, /* index:5, gain:0.46875db, again:0.46875db, dgain:0db */
	{0x0080, 0x00}, /* index:6, gain:0.56250db, again:0.56250db, dgain:0db */
	{0x0095, 0x00}, /* index:7, gain:0.65625db, again:0.65625db, dgain:0db */
	{0x00a9, 0x00}, /* index:8, gain:0.75000db, again:0.75000db, dgain:0db */
	{0x00be, 0x00}, /* index:9, gain:0.84375db, again:0.84375db, dgain:0db */
	{0x00d2, 0x00}, /* index:10, gain:0.93750db, again:0.93750db, dgain:0db */
	{0x00e5, 0x00}, /* index:11, gain:1.03125db, again:1.03125db, dgain:0db */
	{0x00f9, 0x00}, /* index:12, gain:1.12500db, again:1.12500db, dgain:0db */
	{0x010c, 0x00}, /* index:13, gain:1.21875db, again:1.21875db, dgain:0db */
	{0x011f, 0x00}, /* index:14, gain:1.31250db, again:1.31250db, dgain:0db */
	{0x0132, 0x00}, /* index:15, gain:1.40625db, again:1.40625db, dgain:0db */
	{0x0145, 0x00}, /* index:16, gain:1.50000db, again:1.50000db, dgain:0db */
	{0x0157, 0x00}, /* index:17, gain:1.59375db, again:1.59375db, dgain:0db */
	{0x016a, 0x00}, /* index:18, gain:1.68750db, again:1.68750db, dgain:0db */
	{0x017c, 0x00}, /* index:19, gain:1.78125db, again:1.78125db, dgain:0db */
	{0x018e, 0x00}, /* index:20, gain:1.87500db, again:1.87500db, dgain:0db */
	{0x019f, 0x00}, /* index:21, gain:1.96875db, again:1.96875db, dgain:0db */
	{0x01b1, 0x00}, /* index:22, gain:2.06250db, again:2.06250db, dgain:0db */
	{0x01c2, 0x00}, /* index:23, gain:2.15625db, again:2.15625db, dgain:0db */
	{0x01d3, 0x00}, /* index:24, gain:2.25000db, again:2.25000db, dgain:0db */
	{0x01e4, 0x00}, /* index:25, gain:2.34375db, again:2.34375db, dgain:0db */
	{0x01f5, 0x00}, /* index:26, gain:2.43750db, again:2.43750db, dgain:0db */
	{0x0206, 0x00}, /* index:27, gain:2.53125db, again:2.53125db, dgain:0db */
	{0x0216, 0x00}, /* index:28, gain:2.62500db, again:2.62500db, dgain:0db */
	{0x0226, 0x00}, /* index:29, gain:2.71875db, again:2.71875db, dgain:0db */
	{0x0236, 0x00}, /* index:30, gain:2.81250db, again:2.81250db, dgain:0db */
	{0x0246, 0x00}, /* index:31, gain:2.90625db, again:2.90625db, dgain:0db */
	{0x0256, 0x00}, /* index:32, gain:3.00000db, again:3.00000db, dgain:0db */
	{0x0266, 0x00}, /* index:33, gain:3.09375db, again:3.09375db, dgain:0db */
	{0x0275, 0x00}, /* index:34, gain:3.18750db, again:3.18750db, dgain:0db */
	{0x0284, 0x00}, /* index:35, gain:3.28125db, again:3.28125db, dgain:0db */
	{0x0293, 0x00}, /* index:36, gain:3.37500db, again:3.37500db, dgain:0db */
	{0x02a2, 0x00}, /* index:37, gain:3.46875db, again:3.46875db, dgain:0db */
	{0x02b1, 0x00}, /* index:38, gain:3.56250db, again:3.56250db, dgain:0db */
	{0x02c0, 0x00}, /* index:39, gain:3.65625db, again:3.65625db, dgain:0db */
	{0x02ce, 0x00}, /* index:40, gain:3.75000db, again:3.75000db, dgain:0db */
	{0x02dc, 0x00}, /* index:41, gain:3.84375db, again:3.84375db, dgain:0db */
	{0x02ea, 0x00}, /* index:42, gain:3.93750db, again:3.93750db, dgain:0db */
	{0x02f8, 0x00}, /* index:43, gain:4.03125db, again:4.03125db, dgain:0db */
	{0x0306, 0x00}, /* index:44, gain:4.12500db, again:4.12500db, dgain:0db */
	{0x0314, 0x00}, /* index:45, gain:4.21875db, again:4.21875db, dgain:0db */
	{0x0321, 0x00}, /* index:46, gain:4.31250db, again:4.31250db, dgain:0db */
	{0x032f, 0x00}, /* index:47, gain:4.40625db, again:4.40625db, dgain:0db */
	{0x033c, 0x00}, /* index:48, gain:4.50000db, again:4.50000db, dgain:0db */
	{0x0349, 0x00}, /* index:49, gain:4.59375db, again:4.59375db, dgain:0db */
	{0x0356, 0x00}, /* index:50, gain:4.68750db, again:4.68750db, dgain:0db */
	{0x0363, 0x00}, /* index:51, gain:4.78125db, again:4.78125db, dgain:0db */
	{0x0370, 0x00}, /* index:52, gain:4.87500db, again:4.87500db, dgain:0db */
	{0x037c, 0x00}, /* index:53, gain:4.96875db, again:4.96875db, dgain:0db */
	{0x0389, 0x00}, /* index:54, gain:5.06250db, again:5.06250db, dgain:0db */
	{0x0395, 0x00}, /* index:55, gain:5.15625db, again:5.15625db, dgain:0db */
	{0x03a1, 0x00}, /* index:56, gain:5.25000db, again:5.25000db, dgain:0db */
	{0x03ad, 0x00}, /* index:57, gain:5.34375db, again:5.34375db, dgain:0db */
	{0x03b9, 0x00}, /* index:58, gain:5.43750db, again:5.43750db, dgain:0db */
	{0x03c5, 0x00}, /* index:59, gain:5.53125db, again:5.53125db, dgain:0db */
	{0x03d0, 0x00}, /* index:60, gain:5.62500db, again:5.62500db, dgain:0db */
	{0x03dc, 0x00}, /* index:61, gain:5.71875db, again:5.71875db, dgain:0db */
	{0x03e7, 0x00}, /* index:62, gain:5.81250db, again:5.81250db, dgain:0db */
	{0x03f2, 0x00}, /* index:63, gain:5.90625db, again:5.90625db, dgain:0db */
	{0x03fe, 0x00}, /* index:64, gain:6.00000db, again:6.00000db, dgain:0db */
	{0x0409, 0x00}, /* index:65, gain:6.09375db, again:6.09375db, dgain:0db */
	{0x0413, 0x00}, /* index:66, gain:6.18750db, again:6.18750db, dgain:0db */
	{0x041e, 0x00}, /* index:67, gain:6.28125db, again:6.28125db, dgain:0db */
	{0x0429, 0x00}, /* index:68, gain:6.37500db, again:6.37500db, dgain:0db */
	{0x0433, 0x00}, /* index:69, gain:6.46875db, again:6.46875db, dgain:0db */
	{0x043e, 0x00}, /* index:70, gain:6.56250db, again:6.56250db, dgain:0db */
	{0x0448, 0x00}, /* index:71, gain:6.65625db, again:6.65625db, dgain:0db */
	{0x0452, 0x00}, /* index:72, gain:6.75000db, again:6.75000db, dgain:0db */
	{0x045d, 0x00}, /* index:73, gain:6.84375db, again:6.84375db, dgain:0db */
	{0x0467, 0x00}, /* index:74, gain:6.93750db, again:6.93750db, dgain:0db */
	{0x0470, 0x00}, /* index:75, gain:7.03125db, again:7.03125db, dgain:0db */
	{0x047a, 0x00}, /* index:76, gain:7.12500db, again:7.12500db, dgain:0db */
	{0x0484, 0x00}, /* index:77, gain:7.21875db, again:7.21875db, dgain:0db */
	{0x048e, 0x00}, /* index:78, gain:7.31250db, again:7.31250db, dgain:0db */
	{0x0497, 0x00}, /* index:79, gain:7.40625db, again:7.40625db, dgain:0db */
	{0x04a0, 0x00}, /* index:80, gain:7.50000db, again:7.50000db, dgain:0db */
	{0x04aa, 0x00}, /* index:81, gain:7.59375db, again:7.59375db, dgain:0db */
	{0x04b3, 0x00}, /* index:82, gain:7.68750db, again:7.68750db, dgain:0db */
	{0x04bc, 0x00}, /* index:83, gain:7.78125db, again:7.78125db, dgain:0db */
	{0x04c5, 0x00}, /* index:84, gain:7.87500db, again:7.87500db, dgain:0db */
	{0x04ce, 0x00}, /* index:85, gain:7.96875db, again:7.96875db, dgain:0db */
	{0x04d7, 0x00}, /* index:86, gain:8.06250db, again:8.06250db, dgain:0db */
	{0x04df, 0x00}, /* index:87, gain:8.15625db, again:8.15625db, dgain:0db */
	{0x04e8, 0x00}, /* index:88, gain:8.25000db, again:8.25000db, dgain:0db */
	{0x04f0, 0x00}, /* index:89, gain:8.34375db, again:8.34375db, dgain:0db */
	{0x04f9, 0x00}, /* index:90, gain:8.43750db, again:8.43750db, dgain:0db */
	{0x0501, 0x00}, /* index:91, gain:8.53125db, again:8.53125db, dgain:0db */
	{0x0509, 0x00}, /* index:92, gain:8.62500db, again:8.62500db, dgain:0db */
	{0x0511, 0x00}, /* index:93, gain:8.71875db, again:8.71875db, dgain:0db */
	{0x0519, 0x00}, /* index:94, gain:8.81250db, again:8.81250db, dgain:0db */
	{0x0521, 0x00}, /* index:95, gain:8.90625db, again:8.90625db, dgain:0db */
	{0x0529, 0x00}, /* index:96, gain:9.00000db, again:9.00000db, dgain:0db */
	{0x0531, 0x00}, /* index:97, gain:9.09375db, again:9.09375db, dgain:0db */
	{0x0539, 0x00}, /* index:98, gain:9.18750db, again:9.18750db, dgain:0db */
	{0x0540, 0x00}, /* index:99, gain:9.28125db, again:9.28125db, dgain:0db */
	{0x0548, 0x00}, /* index:100, gain:9.37500db, again:9.37500db, dgain:0db */
	{0x0550, 0x00}, /* index:101, gain:9.46875db, again:9.46875db, dgain:0db */
	{0x0557, 0x00}, /* index:102, gain:9.56250db, again:9.56250db, dgain:0db */
	{0x055e, 0x00}, /* index:103, gain:9.65625db, again:9.65625db, dgain:0db */
	{0x0565, 0x00}, /* index:104, gain:9.75000db, again:9.75000db, dgain:0db */
	{0x056d, 0x00}, /* index:105, gain:9.84375db, again:9.84375db, dgain:0db */
	{0x0574, 0x00}, /* index:106, gain:9.93750db, again:9.93750db, dgain:0db */
	{0x057b, 0x00}, /* index:107, gain:10.03125db, again:10.03125db, dgain:0db */
	{0x0582, 0x00}, /* index:108, gain:10.12500db, again:10.12500db, dgain:0db */
	{0x0588, 0x00}, /* index:109, gain:10.21875db, again:10.21875db, dgain:0db */
	{0x058f, 0x00}, /* index:110, gain:10.31250db, again:10.31250db, dgain:0db */
	{0x0596, 0x00}, /* index:111, gain:10.40625db, again:10.40625db, dgain:0db */
	{0x059d, 0x00}, /* index:112, gain:10.50000db, again:10.50000db, dgain:0db */
	{0x05a3, 0x00}, /* index:113, gain:10.59375db, again:10.59375db, dgain:0db */
	{0x05aa, 0x00}, /* index:114, gain:10.68750db, again:10.68750db, dgain:0db */
	{0x05b0, 0x00}, /* index:115, gain:10.78125db, again:10.78125db, dgain:0db */
	{0x05b6, 0x00}, /* index:116, gain:10.87500db, again:10.87500db, dgain:0db */
	{0x05bd, 0x00}, /* index:117, gain:10.96875db, again:10.96875db, dgain:0db */
	{0x05c3, 0x00}, /* index:118, gain:11.06250db, again:11.06250db, dgain:0db */
	{0x05c9, 0x00}, /* index:119, gain:11.15625db, again:11.15625db, dgain:0db */
	{0x05cf, 0x00}, /* index:120, gain:11.25000db, again:11.25000db, dgain:0db */
	{0x05d5, 0x00}, /* index:121, gain:11.34375db, again:11.34375db, dgain:0db */
	{0x05db, 0x00}, /* index:122, gain:11.43750db, again:11.43750db, dgain:0db */
	{0x05e1, 0x00}, /* index:123, gain:11.53125db, again:11.53125db, dgain:0db */
	{0x05e7, 0x00}, /* index:124, gain:11.62500db, again:11.62500db, dgain:0db */
	{0x05ed, 0x00}, /* index:125, gain:11.71875db, again:11.71875db, dgain:0db */
	{0x05f2, 0x00}, /* index:126, gain:11.81250db, again:11.81250db, dgain:0db */
	{0x05f8, 0x00}, /* index:127, gain:11.90625db, again:11.90625db, dgain:0db */
	{0x05fe, 0x00}, /* index:128, gain:12.00000db, again:12.00000db, dgain:0db */
	{0x0603, 0x00}, /* index:129, gain:12.09375db, again:12.09375db, dgain:0db */
	{0x0609, 0x00}, /* index:130, gain:12.18750db, again:12.18750db, dgain:0db */
	{0x060e, 0x00}, /* index:131, gain:12.28125db, again:12.28125db, dgain:0db */
	{0x0613, 0x00}, /* index:132, gain:12.37500db, again:12.37500db, dgain:0db */
	{0x0619, 0x00}, /* index:133, gain:12.46875db, again:12.46875db, dgain:0db */
	{0x061e, 0x00}, /* index:134, gain:12.56250db, again:12.56250db, dgain:0db */
	{0x0623, 0x00}, /* index:135, gain:12.65625db, again:12.65625db, dgain:0db */
	{0x0628, 0x00}, /* index:136, gain:12.75000db, again:12.75000db, dgain:0db */
	{0x062d, 0x00}, /* index:137, gain:12.84375db, again:12.84375db, dgain:0db */
	{0x0632, 0x00}, /* index:138, gain:12.93750db, again:12.93750db, dgain:0db */
	{0x0637, 0x00}, /* index:139, gain:13.03125db, again:13.03125db, dgain:0db */
	{0x063c, 0x00}, /* index:140, gain:13.12500db, again:13.12500db, dgain:0db */
	{0x0641, 0x00}, /* index:141, gain:13.21875db, again:13.21875db, dgain:0db */
	{0x0646, 0x00}, /* index:142, gain:13.31250db, again:13.31250db, dgain:0db */
	{0x064a, 0x00}, /* index:143, gain:13.40625db, again:13.40625db, dgain:0db */
	{0x064f, 0x00}, /* index:144, gain:13.50000db, again:13.50000db, dgain:0db */
	{0x0654, 0x00}, /* index:145, gain:13.59375db, again:13.59375db, dgain:0db */
	{0x0658, 0x00}, /* index:146, gain:13.68750db, again:13.68750db, dgain:0db */
	{0x065d, 0x00}, /* index:147, gain:13.78125db, again:13.78125db, dgain:0db */
	{0x0661, 0x00}, /* index:148, gain:13.87500db, again:13.87500db, dgain:0db */
	{0x0666, 0x00}, /* index:149, gain:13.96875db, again:13.96875db, dgain:0db */
	{0x066a, 0x00}, /* index:150, gain:14.06250db, again:14.06250db, dgain:0db */
	{0x066f, 0x00}, /* index:151, gain:14.15625db, again:14.15625db, dgain:0db */
	{0x0673, 0x00}, /* index:152, gain:14.25000db, again:14.25000db, dgain:0db */
	{0x0677, 0x00}, /* index:153, gain:14.34375db, again:14.34375db, dgain:0db */
	{0x067b, 0x00}, /* index:154, gain:14.43750db, again:14.43750db, dgain:0db */
	{0x0680, 0x00}, /* index:155, gain:14.53125db, again:14.53125db, dgain:0db */
	{0x0684, 0x00}, /* index:156, gain:14.62500db, again:14.62500db, dgain:0db */
	{0x0688, 0x00}, /* index:157, gain:14.71875db, again:14.71875db, dgain:0db */
	{0x068c, 0x00}, /* index:158, gain:14.81250db, again:14.81250db, dgain:0db */
	{0x0690, 0x00}, /* index:159, gain:14.90625db, again:14.90625db, dgain:0db */
	{0x0694, 0x00}, /* index:160, gain:15.00000db, again:15.00000db, dgain:0db */
	{0x0698, 0x00}, /* index:161, gain:15.09375db, again:15.09375db, dgain:0db */
	{0x069c, 0x00}, /* index:162, gain:15.18750db, again:15.18750db, dgain:0db */
	{0x069f, 0x00}, /* index:163, gain:15.28125db, again:15.28125db, dgain:0db */
	{0x06a3, 0x00}, /* index:164, gain:15.37500db, again:15.37500db, dgain:0db */
	{0x06a7, 0x00}, /* index:165, gain:15.46875db, again:15.46875db, dgain:0db */
	{0x06ab, 0x00}, /* index:166, gain:15.56250db, again:15.56250db, dgain:0db */
	{0x06ae, 0x00}, /* index:167, gain:15.65625db, again:15.65625db, dgain:0db */
	{0x06b2, 0x00}, /* index:168, gain:15.75000db, again:15.75000db, dgain:0db */
	{0x06b6, 0x00}, /* index:169, gain:15.84375db, again:15.84375db, dgain:0db */
	{0x06b9, 0x00}, /* index:170, gain:15.93750db, again:15.93750db, dgain:0db */
	{0x06bd, 0x00}, /* index:171, gain:16.03125db, again:16.03125db, dgain:0db */
	{0x06c0, 0x00}, /* index:172, gain:16.12500db, again:16.12500db, dgain:0db */
	{0x06c3, 0x00}, /* index:173, gain:16.21875db, again:16.21875db, dgain:0db */
	{0x06c7, 0x00}, /* index:174, gain:16.31250db, again:16.31250db, dgain:0db */
	{0x06ca, 0x00}, /* index:175, gain:16.40625db, again:16.40625db, dgain:0db */
	{0x06ce, 0x00}, /* index:176, gain:16.50000db, again:16.50000db, dgain:0db */
	{0x06d1, 0x00}, /* index:177, gain:16.59375db, again:16.59375db, dgain:0db */
	{0x06d4, 0x00}, /* index:178, gain:16.68750db, again:16.68750db, dgain:0db */
	{0x06d7, 0x00}, /* index:179, gain:16.78125db, again:16.78125db, dgain:0db */
	{0x06db, 0x00}, /* index:180, gain:16.87500db, again:16.87500db, dgain:0db */
	{0x06de, 0x00}, /* index:181, gain:16.96875db, again:16.96875db, dgain:0db */
	{0x06e1, 0x00}, /* index:182, gain:17.06250db, again:17.06250db, dgain:0db */
	{0x06e4, 0x00}, /* index:183, gain:17.15625db, again:17.15625db, dgain:0db */
	{0x06e7, 0x00}, /* index:184, gain:17.25000db, again:17.25000db, dgain:0db */
	{0x06ea, 0x00}, /* index:185, gain:17.34375db, again:17.34375db, dgain:0db */
	{0x06ed, 0x00}, /* index:186, gain:17.43750db, again:17.43750db, dgain:0db */
	{0x06f0, 0x00}, /* index:187, gain:17.53125db, again:17.53125db, dgain:0db */
	{0x06f3, 0x00}, /* index:188, gain:17.62500db, again:17.62500db, dgain:0db */
	{0x06f6, 0x00}, /* index:189, gain:17.71875db, again:17.71875db, dgain:0db */
	{0x06f9, 0x00}, /* index:190, gain:17.81250db, again:17.81250db, dgain:0db */
	{0x06fb, 0x00}, /* index:191, gain:17.90625db, again:17.90625db, dgain:0db */
	{0x06fe, 0x00}, /* index:192, gain:18.00000db, again:18.00000db, dgain:0db */
	{0x0701, 0x00}, /* index:193, gain:18.09375db, again:18.09375db, dgain:0db */
	{0x0704, 0x00}, /* index:194, gain:18.18750db, again:18.18750db, dgain:0db */
	{0x0706, 0x00}, /* index:195, gain:18.28125db, again:18.28125db, dgain:0db */
	{0x0709, 0x00}, /* index:196, gain:18.37500db, again:18.37500db, dgain:0db */
	{0x070c, 0x00}, /* index:197, gain:18.46875db, again:18.46875db, dgain:0db */
	{0x070e, 0x00}, /* index:198, gain:18.56250db, again:18.56250db, dgain:0db */
	{0x0711, 0x00}, /* index:199, gain:18.65625db, again:18.65625db, dgain:0db */
	{0x0714, 0x00}, /* index:200, gain:18.75000db, again:18.75000db, dgain:0db */
	{0x0716, 0x00}, /* index:201, gain:18.84375db, again:18.84375db, dgain:0db */
	{0x0719, 0x00}, /* index:202, gain:18.93750db, again:18.93750db, dgain:0db */
	{0x071b, 0x00}, /* index:203, gain:19.03125db, again:19.03125db, dgain:0db */
	{0x071d, 0x00}, /* index:204, gain:19.12500db, again:19.12500db, dgain:0db */
	{0x0720, 0x00}, /* index:205, gain:19.21875db, again:19.21875db, dgain:0db */
	{0x0722, 0x00}, /* index:206, gain:19.31250db, again:19.31250db, dgain:0db */
	{0x0725, 0x00}, /* index:207, gain:19.40625db, again:19.40625db, dgain:0db */
	{0x0727, 0x00}, /* index:208, gain:19.50000db, again:19.50000db, dgain:0db */
	{0x0729, 0x00}, /* index:209, gain:19.59375db, again:19.59375db, dgain:0db */
	{0x072c, 0x00}, /* index:210, gain:19.68750db, again:19.68750db, dgain:0db */
	{0x072e, 0x00}, /* index:211, gain:19.78125db, again:19.78125db, dgain:0db */
	{0x0730, 0x00}, /* index:212, gain:19.87500db, again:19.87500db, dgain:0db */
	{0x0732, 0x00}, /* index:213, gain:19.96875db, again:19.96875db, dgain:0db */
	{0x0735, 0x00}, /* index:214, gain:20.06250db, again:20.06250db, dgain:0db */
	{0x0737, 0x00}, /* index:215, gain:20.15625db, again:20.15625db, dgain:0db */
	{0x0739, 0x00}, /* index:216, gain:20.25000db, again:20.25000db, dgain:0db */
	{0x073b, 0x00}, /* index:217, gain:20.34375db, again:20.34375db, dgain:0db */
	{0x073d, 0x00}, /* index:218, gain:20.43750db, again:20.43750db, dgain:0db */
	{0x073f, 0x00}, /* index:219, gain:20.53125db, again:20.53125db, dgain:0db */
	{0x0741, 0x00}, /* index:220, gain:20.62500db, again:20.62500db, dgain:0db */
	{0x0743, 0x00}, /* index:221, gain:20.71875db, again:20.71875db, dgain:0db */
	{0x0745, 0x00}, /* index:222, gain:20.81250db, again:20.81250db, dgain:0db */
	{0x0747, 0x00}, /* index:223, gain:20.90625db, again:20.90625db, dgain:0db */
	{0x0749, 0x00}, /* index:224, gain:21.00000db, again:21.00000db, dgain:0db */
	{0x074b, 0x00}, /* index:225, gain:21.09375db, again:21.09375db, dgain:0db */
	{0x074d, 0x00}, /* index:226, gain:21.18750db, again:21.18750db, dgain:0db */
	{0x074f, 0x00}, /* index:227, gain:21.28125db, again:21.28125db, dgain:0db */
	{0x0751, 0x00}, /* index:228, gain:21.37500db, again:21.37500db, dgain:0db */
	{0x0753, 0x00}, /* index:229, gain:21.46875db, again:21.46875db, dgain:0db */
	{0x0755, 0x00}, /* index:230, gain:21.56250db, again:21.56250db, dgain:0db */
	{0x0757, 0x00}, /* index:231, gain:21.65625db, again:21.65625db, dgain:0db */
	{0x0759, 0x00}, /* index:232, gain:21.75000db, again:21.75000db, dgain:0db */
	{0x075a, 0x00}, /* index:233, gain:21.84375db, again:21.84375db, dgain:0db */
	{0x075c, 0x00}, /* index:234, gain:21.93750db, again:21.93750db, dgain:0db */
	{0x075e, 0x00}, /* index:235, gain:22.03125db, again:22.03125db, dgain:0db */
	{0x0760, 0x00}, /* index:236, gain:22.12500db, again:22.12500db, dgain:0db */
	{0x0761, 0x00}, /* index:237, gain:22.21875db, again:22.21875db, dgain:0db */
	{0x0763, 0x00}, /* index:238, gain:22.31250db, again:22.31250db, dgain:0db */
	{0x0765, 0x00}, /* index:239, gain:22.40625db, again:22.40625db, dgain:0db */
	{0x0766, 0x00}, /* index:240, gain:22.50000db, again:22.50000db, dgain:0db */
	{0x0768, 0x00}, /* index:241, gain:22.59375db, again:22.59375db, dgain:0db */
	{0x076a, 0x00}, /* index:242, gain:22.68750db, again:22.68750db, dgain:0db */
	{0x076b, 0x00}, /* index:243, gain:22.78125db, again:22.78125db, dgain:0db */
	{0x076d, 0x00}, /* index:244, gain:22.87500db, again:22.87500db, dgain:0db */
	{0x076e, 0x00}, /* index:245, gain:22.96875db, again:22.96875db, dgain:0db */
	{0x0770, 0x00}, /* index:246, gain:23.06250db, again:23.06250db, dgain:0db */
	{0x0772, 0x00}, /* index:247, gain:23.15625db, again:23.15625db, dgain:0db */
	{0x0773, 0x00}, /* index:248, gain:23.25000db, again:23.25000db, dgain:0db */
	{0x0775, 0x00}, /* index:249, gain:23.34375db, again:23.34375db, dgain:0db */
	{0x0776, 0x00}, /* index:250, gain:23.43750db, again:23.43750db, dgain:0db */
	{0x0778, 0x00}, /* index:251, gain:23.53125db, again:23.53125db, dgain:0db */
	{0x0779, 0x00}, /* index:252, gain:23.62500db, again:23.62500db, dgain:0db */
	{0x077b, 0x00}, /* index:253, gain:23.71875db, again:23.71875db, dgain:0db */
	{0x077c, 0x00}, /* index:254, gain:23.81250db, again:23.81250db, dgain:0db */
	{0x077d, 0x00}, /* index:255, gain:23.90625db, again:23.90625db, dgain:0db */
	{0x077f, 0x00}, /* index:256, gain:24.00000db, again:24.00000db, dgain:0db */
	{0x0780, 0x00}, /* index:257, gain:24.09375db, again:24.09375db, dgain:0db */
	{0x0782, 0x00}, /* index:258, gain:24.18750db, again:24.18750db, dgain:0db */
	{0x0783, 0x00}, /* index:259, gain:24.28125db, again:24.28125db, dgain:0db */
	{0x0784, 0x00}, /* index:260, gain:24.37500db, again:24.37500db, dgain:0db */
	{0x0786, 0x00}, /* index:261, gain:24.46875db, again:24.46875db, dgain:0db */
	{0x0787, 0x00}, /* index:262, gain:24.56250db, again:24.56250db, dgain:0db */
	{0x0788, 0x00}, /* index:263, gain:24.65625db, again:24.65625db, dgain:0db */
	{0x0789, 0x00}, /* index:264, gain:24.75000db, again:24.75000db, dgain:0db */
	{0x078b, 0x00}, /* index:265, gain:24.84375db, again:24.84375db, dgain:0db */
	{0x078c, 0x00}, /* index:266, gain:24.93750db, again:24.93750db, dgain:0db */
	{0x078d, 0x00}, /* index:267, gain:25.03125db, again:25.03125db, dgain:0db */
	{0x078e, 0x00}, /* index:268, gain:25.12500db, again:25.12500db, dgain:0db */
	{0x0790, 0x00}, /* index:269, gain:25.21875db, again:25.21875db, dgain:0db */
	{0x0791, 0x00}, /* index:270, gain:25.31250db, again:25.31250db, dgain:0db */
	{0x0792, 0x00}, /* index:271, gain:25.40625db, again:25.40625db, dgain:0db */
	{0x0793, 0x00}, /* index:272, gain:25.50000db, again:25.50000db, dgain:0db */
	{0x0794, 0x00}, /* index:273, gain:25.59375db, again:25.59375db, dgain:0db */
	{0x0796, 0x00}, /* index:274, gain:25.68750db, again:25.68750db, dgain:0db */
	{0x0797, 0x00}, /* index:275, gain:25.78125db, again:25.78125db, dgain:0db */
	{0x0798, 0x00}, /* index:276, gain:25.87500db, again:25.87500db, dgain:0db */
	{0x0799, 0x00}, /* index:277, gain:25.96875db, again:25.96875db, dgain:0db */
	{0x079a, 0x00}, /* index:278, gain:26.06250db, again:26.06250db, dgain:0db */
	{0x079b, 0x00}, /* index:279, gain:26.15625db, again:26.15625db, dgain:0db */
	{0x079c, 0x00}, /* index:280, gain:26.25000db, again:26.25000db, dgain:0db */
	{0x079d, 0x00}, /* index:281, gain:26.34375db, again:26.34375db, dgain:0db */
	{0x079e, 0x00}, /* index:282, gain:26.43750db, again:26.43750db, dgain:0db */
	{0x079f, 0x00}, /* index:283, gain:26.53125db, again:26.53125db, dgain:0db */
	{0x07a0, 0x00}, /* index:284, gain:26.62500db, again:26.62500db, dgain:0db */
	{0x07a2, 0x00}, /* index:285, gain:26.71875db, again:26.71875db, dgain:0db */
	{0x07a3, 0x00}, /* index:286, gain:26.81250db, again:26.81250db, dgain:0db */
	{0x07a4, 0x00}, /* index:287, gain:26.90625db, again:26.90625db, dgain:0db */
	{0x07a5, 0x00}, /* index:288, gain:27.00000db, again:27.00000db, dgain:0db */
	{0x074b, 0x01}, /* index:289, gain:27.09375db, again:21.09375db, dgain:6db */
	{0x074d, 0x01}, /* index:290, gain:27.18750db, again:21.18750db, dgain:6db */
	{0x074f, 0x01}, /* index:291, gain:27.28125db, again:21.28125db, dgain:6db */
	{0x0751, 0x01}, /* index:292, gain:27.37500db, again:21.37500db, dgain:6db */
	{0x0753, 0x01}, /* index:293, gain:27.46875db, again:21.46875db, dgain:6db */
	{0x0755, 0x01}, /* index:294, gain:27.56250db, again:21.56250db, dgain:6db */
	{0x0757, 0x01}, /* index:295, gain:27.65625db, again:21.65625db, dgain:6db */
	{0x0759, 0x01}, /* index:296, gain:27.75000db, again:21.75000db, dgain:6db */
	{0x075a, 0x01}, /* index:297, gain:27.84375db, again:21.84375db, dgain:6db */
	{0x075c, 0x01}, /* index:298, gain:27.93750db, again:21.93750db, dgain:6db */
	{0x075e, 0x01}, /* index:299, gain:28.03125db, again:22.03125db, dgain:6db */
	{0x0760, 0x01}, /* index:300, gain:28.12500db, again:22.12500db, dgain:6db */
	{0x0761, 0x01}, /* index:301, gain:28.21875db, again:22.21875db, dgain:6db */
	{0x0763, 0x01}, /* index:302, gain:28.31250db, again:22.31250db, dgain:6db */
	{0x0765, 0x01}, /* index:303, gain:28.40625db, again:22.40625db, dgain:6db */
	{0x0766, 0x01}, /* index:304, gain:28.50000db, again:22.50000db, dgain:6db */
	{0x0768, 0x01}, /* index:305, gain:28.59375db, again:22.59375db, dgain:6db */
	{0x076a, 0x01}, /* index:306, gain:28.68750db, again:22.68750db, dgain:6db */
	{0x076b, 0x01}, /* index:307, gain:28.78125db, again:22.78125db, dgain:6db */
	{0x076d, 0x01}, /* index:308, gain:28.87500db, again:22.87500db, dgain:6db */
	{0x076e, 0x01}, /* index:309, gain:28.96875db, again:22.96875db, dgain:6db */
	{0x0770, 0x01}, /* index:310, gain:29.06250db, again:23.06250db, dgain:6db */
	{0x0772, 0x01}, /* index:311, gain:29.15625db, again:23.15625db, dgain:6db */
	{0x0773, 0x01}, /* index:312, gain:29.25000db, again:23.25000db, dgain:6db */
	{0x0775, 0x01}, /* index:313, gain:29.34375db, again:23.34375db, dgain:6db */
	{0x0776, 0x01}, /* index:314, gain:29.43750db, again:23.43750db, dgain:6db */
	{0x0778, 0x01}, /* index:315, gain:29.53125db, again:23.53125db, dgain:6db */
	{0x0779, 0x01}, /* index:316, gain:29.62500db, again:23.62500db, dgain:6db */
	{0x077b, 0x01}, /* index:317, gain:29.71875db, again:23.71875db, dgain:6db */
	{0x077c, 0x01}, /* index:318, gain:29.81250db, again:23.81250db, dgain:6db */
	{0x077d, 0x01}, /* index:319, gain:29.90625db, again:23.90625db, dgain:6db */
	{0x077f, 0x01}, /* index:320, gain:30.00000db, again:24.00000db, dgain:6db */
	{0x0780, 0x01}, /* index:321, gain:30.09375db, again:24.09375db, dgain:6db */
	{0x0782, 0x01}, /* index:322, gain:30.18750db, again:24.18750db, dgain:6db */
	{0x0783, 0x01}, /* index:323, gain:30.28125db, again:24.28125db, dgain:6db */
	{0x0784, 0x01}, /* index:324, gain:30.37500db, again:24.37500db, dgain:6db */
	{0x0786, 0x01}, /* index:325, gain:30.46875db, again:24.46875db, dgain:6db */
	{0x0787, 0x01}, /* index:326, gain:30.56250db, again:24.56250db, dgain:6db */
	{0x0788, 0x01}, /* index:327, gain:30.65625db, again:24.65625db, dgain:6db */
	{0x0789, 0x01}, /* index:328, gain:30.75000db, again:24.75000db, dgain:6db */
	{0x078b, 0x01}, /* index:329, gain:30.84375db, again:24.84375db, dgain:6db */
	{0x078c, 0x01}, /* index:330, gain:30.93750db, again:24.93750db, dgain:6db */
	{0x078d, 0x01}, /* index:331, gain:31.03125db, again:25.03125db, dgain:6db */
	{0x078e, 0x01}, /* index:332, gain:31.12500db, again:25.12500db, dgain:6db */
	{0x0790, 0x01}, /* index:333, gain:31.21875db, again:25.21875db, dgain:6db */
	{0x0791, 0x01}, /* index:334, gain:31.31250db, again:25.31250db, dgain:6db */
	{0x0792, 0x01}, /* index:335, gain:31.40625db, again:25.40625db, dgain:6db */
	{0x0793, 0x01}, /* index:336, gain:31.50000db, again:25.50000db, dgain:6db */
	{0x0794, 0x01}, /* index:337, gain:31.59375db, again:25.59375db, dgain:6db */
	{0x0796, 0x01}, /* index:338, gain:31.68750db, again:25.68750db, dgain:6db */
	{0x0797, 0x01}, /* index:339, gain:31.78125db, again:25.78125db, dgain:6db */
	{0x0798, 0x01}, /* index:340, gain:31.87500db, again:25.87500db, dgain:6db */
	{0x0799, 0x01}, /* index:341, gain:31.96875db, again:25.96875db, dgain:6db */
	{0x079a, 0x01}, /* index:342, gain:32.06250db, again:26.06250db, dgain:6db */
	{0x079b, 0x01}, /* index:343, gain:32.15625db, again:26.15625db, dgain:6db */
	{0x079c, 0x01}, /* index:344, gain:32.25000db, again:26.25000db, dgain:6db */
	{0x079d, 0x01}, /* index:345, gain:32.34375db, again:26.34375db, dgain:6db */
	{0x079e, 0x01}, /* index:346, gain:32.43750db, again:26.43750db, dgain:6db */
	{0x079f, 0x01}, /* index:347, gain:32.53125db, again:26.53125db, dgain:6db */
	{0x07a0, 0x01}, /* index:348, gain:32.62500db, again:26.62500db, dgain:6db */
	{0x07a2, 0x01}, /* index:349, gain:32.71875db, again:26.71875db, dgain:6db */
	{0x07a3, 0x01}, /* index:350, gain:32.81250db, again:26.81250db, dgain:6db */
	{0x07a4, 0x01}, /* index:351, gain:32.90625db, again:26.90625db, dgain:6db */
	{0x07a5, 0x01}, /* index:352, gain:33.00000db, again:27.00000db, dgain:6db */
	{0x074b, 0x02}, /* index:353, gain:33.09375db, again:21.09375db, dgain:12db */
	{0x074d, 0x02}, /* index:354, gain:33.18750db, again:21.18750db, dgain:12db */
	{0x074f, 0x02}, /* index:355, gain:33.28125db, again:21.28125db, dgain:12db */
	{0x0751, 0x02}, /* index:356, gain:33.37500db, again:21.37500db, dgain:12db */
	{0x0753, 0x02}, /* index:357, gain:33.46875db, again:21.46875db, dgain:12db */
	{0x0755, 0x02}, /* index:358, gain:33.56250db, again:21.56250db, dgain:12db */
	{0x0757, 0x02}, /* index:359, gain:33.65625db, again:21.65625db, dgain:12db */
	{0x0759, 0x02}, /* index:360, gain:33.75000db, again:21.75000db, dgain:12db */
	{0x075a, 0x02}, /* index:361, gain:33.84375db, again:21.84375db, dgain:12db */
	{0x075c, 0x02}, /* index:362, gain:33.93750db, again:21.93750db, dgain:12db */
	{0x075e, 0x02}, /* index:363, gain:34.03125db, again:22.03125db, dgain:12db */
	{0x0760, 0x02}, /* index:364, gain:34.12500db, again:22.12500db, dgain:12db */
	{0x0761, 0x02}, /* index:365, gain:34.21875db, again:22.21875db, dgain:12db */
	{0x0763, 0x02}, /* index:366, gain:34.31250db, again:22.31250db, dgain:12db */
	{0x0765, 0x02}, /* index:367, gain:34.40625db, again:22.40625db, dgain:12db */
	{0x0766, 0x02}, /* index:368, gain:34.50000db, again:22.50000db, dgain:12db */
	{0x0768, 0x02}, /* index:369, gain:34.59375db, again:22.59375db, dgain:12db */
	{0x076a, 0x02}, /* index:370, gain:34.68750db, again:22.68750db, dgain:12db */
	{0x076b, 0x02}, /* index:371, gain:34.78125db, again:22.78125db, dgain:12db */
	{0x076d, 0x02}, /* index:372, gain:34.87500db, again:22.87500db, dgain:12db */
	{0x076e, 0x02}, /* index:373, gain:34.96875db, again:22.96875db, dgain:12db */
	{0x0770, 0x02}, /* index:374, gain:35.06250db, again:23.06250db, dgain:12db */
	{0x0772, 0x02}, /* index:375, gain:35.15625db, again:23.15625db, dgain:12db */
	{0x0773, 0x02}, /* index:376, gain:35.25000db, again:23.25000db, dgain:12db */
	{0x0775, 0x02}, /* index:377, gain:35.34375db, again:23.34375db, dgain:12db */
	{0x0776, 0x02}, /* index:378, gain:35.43750db, again:23.43750db, dgain:12db */
	{0x0778, 0x02}, /* index:379, gain:35.53125db, again:23.53125db, dgain:12db */
	{0x0779, 0x02}, /* index:380, gain:35.62500db, again:23.62500db, dgain:12db */
	{0x077b, 0x02}, /* index:381, gain:35.71875db, again:23.71875db, dgain:12db */
	{0x077c, 0x02}, /* index:382, gain:35.81250db, again:23.81250db, dgain:12db */
	{0x077d, 0x02}, /* index:383, gain:35.90625db, again:23.90625db, dgain:12db */
	{0x077f, 0x02}, /* index:384, gain:36.00000db, again:24.00000db, dgain:12db */
	{0x0780, 0x02}, /* index:385, gain:36.09375db, again:24.09375db, dgain:12db */
	{0x0782, 0x02}, /* index:386, gain:36.18750db, again:24.18750db, dgain:12db */
	{0x0783, 0x02}, /* index:387, gain:36.28125db, again:24.28125db, dgain:12db */
	{0x0784, 0x02}, /* index:388, gain:36.37500db, again:24.37500db, dgain:12db */
	{0x0786, 0x02}, /* index:389, gain:36.46875db, again:24.46875db, dgain:12db */
	{0x0787, 0x02}, /* index:390, gain:36.56250db, again:24.56250db, dgain:12db */
	{0x0788, 0x02}, /* index:391, gain:36.65625db, again:24.65625db, dgain:12db */
	{0x0789, 0x02}, /* index:392, gain:36.75000db, again:24.75000db, dgain:12db */
	{0x078b, 0x02}, /* index:393, gain:36.84375db, again:24.84375db, dgain:12db */
	{0x078c, 0x02}, /* index:394, gain:36.93750db, again:24.93750db, dgain:12db */
	{0x078d, 0x02}, /* index:395, gain:37.03125db, again:25.03125db, dgain:12db */
	{0x078e, 0x02}, /* index:396, gain:37.12500db, again:25.12500db, dgain:12db */
	{0x0790, 0x02}, /* index:397, gain:37.21875db, again:25.21875db, dgain:12db */
	{0x0791, 0x02}, /* index:398, gain:37.31250db, again:25.31250db, dgain:12db */
	{0x0792, 0x02}, /* index:399, gain:37.40625db, again:25.40625db, dgain:12db */
	{0x0793, 0x02}, /* index:400, gain:37.50000db, again:25.50000db, dgain:12db */
	{0x0794, 0x02}, /* index:401, gain:37.59375db, again:25.59375db, dgain:12db */
	{0x0796, 0x02}, /* index:402, gain:37.68750db, again:25.68750db, dgain:12db */
	{0x0797, 0x02}, /* index:403, gain:37.78125db, again:25.78125db, dgain:12db */
	{0x0798, 0x02}, /* index:404, gain:37.87500db, again:25.87500db, dgain:12db */
	{0x0799, 0x02}, /* index:405, gain:37.96875db, again:25.96875db, dgain:12db */
	{0x079a, 0x02}, /* index:406, gain:38.06250db, again:26.06250db, dgain:12db */
	{0x079b, 0x02}, /* index:407, gain:38.15625db, again:26.15625db, dgain:12db */
	{0x079c, 0x02}, /* index:408, gain:38.25000db, again:26.25000db, dgain:12db */
	{0x079d, 0x02}, /* index:409, gain:38.34375db, again:26.34375db, dgain:12db */
	{0x079e, 0x02}, /* index:410, gain:38.43750db, again:26.43750db, dgain:12db */
	{0x079f, 0x02}, /* index:411, gain:38.53125db, again:26.53125db, dgain:12db */
	{0x07a0, 0x02}, /* index:412, gain:38.62500db, again:26.62500db, dgain:12db */
	{0x07a2, 0x02}, /* index:413, gain:38.71875db, again:26.71875db, dgain:12db */
	{0x07a3, 0x02}, /* index:414, gain:38.81250db, again:26.81250db, dgain:12db */
	{0x07a4, 0x02}, /* index:415, gain:38.90625db, again:26.90625db, dgain:12db */
	{0x07a5, 0x02}, /* index:416, gain:39.00000db, again:27.00000db, dgain:12db */
	{0x074b, 0x03}, /* index:417, gain:39.09375db, again:21.09375db, dgain:18db */
	{0x074d, 0x03}, /* index:418, gain:39.18750db, again:21.18750db, dgain:18db */
	{0x074f, 0x03}, /* index:419, gain:39.28125db, again:21.28125db, dgain:18db */
	{0x0751, 0x03}, /* index:420, gain:39.37500db, again:21.37500db, dgain:18db */
	{0x0753, 0x03}, /* index:421, gain:39.46875db, again:21.46875db, dgain:18db */
	{0x0755, 0x03}, /* index:422, gain:39.56250db, again:21.56250db, dgain:18db */
	{0x0757, 0x03}, /* index:423, gain:39.65625db, again:21.65625db, dgain:18db */
	{0x0759, 0x03}, /* index:424, gain:39.75000db, again:21.75000db, dgain:18db */
	{0x075a, 0x03}, /* index:425, gain:39.84375db, again:21.84375db, dgain:18db */
	{0x075c, 0x03}, /* index:426, gain:39.93750db, again:21.93750db, dgain:18db */
	{0x075e, 0x03}, /* index:427, gain:40.03125db, again:22.03125db, dgain:18db */
	{0x0760, 0x03}, /* index:428, gain:40.12500db, again:22.12500db, dgain:18db */
	{0x0761, 0x03}, /* index:429, gain:40.21875db, again:22.21875db, dgain:18db */
	{0x0763, 0x03}, /* index:430, gain:40.31250db, again:22.31250db, dgain:18db */
	{0x0765, 0x03}, /* index:431, gain:40.40625db, again:22.40625db, dgain:18db */
	{0x0766, 0x03}, /* index:432, gain:40.50000db, again:22.50000db, dgain:18db */
	{0x0768, 0x03}, /* index:433, gain:40.59375db, again:22.59375db, dgain:18db */
	{0x076a, 0x03}, /* index:434, gain:40.68750db, again:22.68750db, dgain:18db */
	{0x076b, 0x03}, /* index:435, gain:40.78125db, again:22.78125db, dgain:18db */
	{0x076d, 0x03}, /* index:436, gain:40.87500db, again:22.87500db, dgain:18db */
	{0x076e, 0x03}, /* index:437, gain:40.96875db, again:22.96875db, dgain:18db */
	{0x0770, 0x03}, /* index:438, gain:41.06250db, again:23.06250db, dgain:18db */
	{0x0772, 0x03}, /* index:439, gain:41.15625db, again:23.15625db, dgain:18db */
	{0x0773, 0x03}, /* index:440, gain:41.25000db, again:23.25000db, dgain:18db */
	{0x0775, 0x03}, /* index:441, gain:41.34375db, again:23.34375db, dgain:18db */
	{0x0776, 0x03}, /* index:442, gain:41.43750db, again:23.43750db, dgain:18db */
	{0x0778, 0x03}, /* index:443, gain:41.53125db, again:23.53125db, dgain:18db */
	{0x0779, 0x03}, /* index:444, gain:41.62500db, again:23.62500db, dgain:18db */
	{0x077b, 0x03}, /* index:445, gain:41.71875db, again:23.71875db, dgain:18db */
	{0x077c, 0x03}, /* index:446, gain:41.81250db, again:23.81250db, dgain:18db */
	{0x077d, 0x03}, /* index:447, gain:41.90625db, again:23.90625db, dgain:18db */
	{0x077f, 0x03}, /* index:448, gain:42.00000db, again:24.00000db, dgain:18db */
	{0x0780, 0x03}, /* index:449, gain:42.09375db, again:24.09375db, dgain:18db */
	{0x0782, 0x03}, /* index:450, gain:42.18750db, again:24.18750db, dgain:18db */
	{0x0783, 0x03}, /* index:451, gain:42.28125db, again:24.28125db, dgain:18db */
	{0x0784, 0x03}, /* index:452, gain:42.37500db, again:24.37500db, dgain:18db */
	{0x0786, 0x03}, /* index:453, gain:42.46875db, again:24.46875db, dgain:18db */
	{0x0787, 0x03}, /* index:454, gain:42.56250db, again:24.56250db, dgain:18db */
	{0x0788, 0x03}, /* index:455, gain:42.65625db, again:24.65625db, dgain:18db */
	{0x0789, 0x03}, /* index:456, gain:42.75000db, again:24.75000db, dgain:18db */
	{0x078b, 0x03}, /* index:457, gain:42.84375db, again:24.84375db, dgain:18db */
	{0x078c, 0x03}, /* index:458, gain:42.93750db, again:24.93750db, dgain:18db */
	{0x078d, 0x03}, /* index:459, gain:43.03125db, again:25.03125db, dgain:18db */
	{0x078e, 0x03}, /* index:460, gain:43.12500db, again:25.12500db, dgain:18db */
	{0x0790, 0x03}, /* index:461, gain:43.21875db, again:25.21875db, dgain:18db */
	{0x0791, 0x03}, /* index:462, gain:43.31250db, again:25.31250db, dgain:18db */
	{0x0792, 0x03}, /* index:463, gain:43.40625db, again:25.40625db, dgain:18db */
	{0x0793, 0x03}, /* index:464, gain:43.50000db, again:25.50000db, dgain:18db */
	{0x0794, 0x03}, /* index:465, gain:43.59375db, again:25.59375db, dgain:18db */
	{0x0796, 0x03}, /* index:466, gain:43.68750db, again:25.68750db, dgain:18db */
	{0x0797, 0x03}, /* index:467, gain:43.78125db, again:25.78125db, dgain:18db */
	{0x0798, 0x03}, /* index:468, gain:43.87500db, again:25.87500db, dgain:18db */
	{0x0799, 0x03}, /* index:469, gain:43.96875db, again:25.96875db, dgain:18db */
	{0x079a, 0x03}, /* index:470, gain:44.06250db, again:26.06250db, dgain:18db */
	{0x079b, 0x03}, /* index:471, gain:44.15625db, again:26.15625db, dgain:18db */
	{0x079c, 0x03}, /* index:472, gain:44.25000db, again:26.25000db, dgain:18db */
	{0x079d, 0x03}, /* index:473, gain:44.34375db, again:26.34375db, dgain:18db */
	{0x079e, 0x03}, /* index:474, gain:44.43750db, again:26.43750db, dgain:18db */
	{0x079f, 0x03}, /* index:475, gain:44.53125db, again:26.53125db, dgain:18db */
	{0x07a0, 0x03}, /* index:476, gain:44.62500db, again:26.62500db, dgain:18db */
	{0x07a2, 0x03}, /* index:477, gain:44.71875db, again:26.71875db, dgain:18db */
	{0x07a3, 0x03}, /* index:478, gain:44.81250db, again:26.81250db, dgain:18db */
	{0x07a4, 0x03}, /* index:479, gain:44.90625db, again:26.90625db, dgain:18db */
	{0x07a5, 0x03}, /* index:480, gain:45.00000db, again:27.00000db, dgain:18db */
	{0x074b, 0x04}, /* index:481, gain:45.09375db, again:21.09375db, dgain:24db */
	{0x074d, 0x04}, /* index:482, gain:45.18750db, again:21.18750db, dgain:24db */
	{0x074f, 0x04}, /* index:483, gain:45.28125db, again:21.28125db, dgain:24db */
	{0x0751, 0x04}, /* index:484, gain:45.37500db, again:21.37500db, dgain:24db */
	{0x0753, 0x04}, /* index:485, gain:45.46875db, again:21.46875db, dgain:24db */
	{0x0755, 0x04}, /* index:486, gain:45.56250db, again:21.56250db, dgain:24db */
	{0x0757, 0x04}, /* index:487, gain:45.65625db, again:21.65625db, dgain:24db */
	{0x0759, 0x04}, /* index:488, gain:45.75000db, again:21.75000db, dgain:24db */
	{0x075a, 0x04}, /* index:489, gain:45.84375db, again:21.84375db, dgain:24db */
	{0x075c, 0x04}, /* index:490, gain:45.93750db, again:21.93750db, dgain:24db */
	{0x075e, 0x04}, /* index:491, gain:46.03125db, again:22.03125db, dgain:24db */
	{0x0760, 0x04}, /* index:492, gain:46.12500db, again:22.12500db, dgain:24db */
	{0x0761, 0x04}, /* index:493, gain:46.21875db, again:22.21875db, dgain:24db */
	{0x0763, 0x04}, /* index:494, gain:46.31250db, again:22.31250db, dgain:24db */
	{0x0765, 0x04}, /* index:495, gain:46.40625db, again:22.40625db, dgain:24db */
	{0x0766, 0x04}, /* index:496, gain:46.50000db, again:22.50000db, dgain:24db */
	{0x0768, 0x04}, /* index:497, gain:46.59375db, again:22.59375db, dgain:24db */
	{0x076a, 0x04}, /* index:498, gain:46.68750db, again:22.68750db, dgain:24db */
	{0x076b, 0x04}, /* index:499, gain:46.78125db, again:22.78125db, dgain:24db */
	{0x076d, 0x04}, /* index:500, gain:46.87500db, again:22.87500db, dgain:24db */
	{0x076e, 0x04}, /* index:501, gain:46.96875db, again:22.96875db, dgain:24db */
	{0x0770, 0x04}, /* index:502, gain:47.06250db, again:23.06250db, dgain:24db */
	{0x0772, 0x04}, /* index:503, gain:47.15625db, again:23.15625db, dgain:24db */
	{0x0773, 0x04}, /* index:504, gain:47.25000db, again:23.25000db, dgain:24db */
	{0x0775, 0x04}, /* index:505, gain:47.34375db, again:23.34375db, dgain:24db */
	{0x0776, 0x04}, /* index:506, gain:47.43750db, again:23.43750db, dgain:24db */
	{0x0778, 0x04}, /* index:507, gain:47.53125db, again:23.53125db, dgain:24db */
	{0x0779, 0x04}, /* index:508, gain:47.62500db, again:23.62500db, dgain:24db */
	{0x077b, 0x04}, /* index:509, gain:47.71875db, again:23.71875db, dgain:24db */
	{0x077c, 0x04}, /* index:510, gain:47.81250db, again:23.81250db, dgain:24db */
	{0x077d, 0x04}, /* index:511, gain:47.90625db, again:23.90625db, dgain:24db */
	{0x077f, 0x04}, /* index:512, gain:48.00000db, again:24.00000db, dgain:24db */
	{0x0780, 0x04}, /* index:513, gain:48.09375db, again:24.09375db, dgain:24db */
	{0x0782, 0x04}, /* index:514, gain:48.18750db, again:24.18750db, dgain:24db */
	{0x0783, 0x04}, /* index:515, gain:48.28125db, again:24.28125db, dgain:24db */
	{0x0784, 0x04}, /* index:516, gain:48.37500db, again:24.37500db, dgain:24db */
	{0x0786, 0x04}, /* index:517, gain:48.46875db, again:24.46875db, dgain:24db */
	{0x0787, 0x04}, /* index:518, gain:48.56250db, again:24.56250db, dgain:24db */
	{0x0788, 0x04}, /* index:519, gain:48.65625db, again:24.65625db, dgain:24db */
	{0x0789, 0x04}, /* index:520, gain:48.75000db, again:24.75000db, dgain:24db */
	{0x078b, 0x04}, /* index:521, gain:48.84375db, again:24.84375db, dgain:24db */
	{0x078c, 0x04}, /* index:522, gain:48.93750db, again:24.93750db, dgain:24db */
	{0x078d, 0x04}, /* index:523, gain:49.03125db, again:25.03125db, dgain:24db */
	{0x078e, 0x04}, /* index:524, gain:49.12500db, again:25.12500db, dgain:24db */
	{0x0790, 0x04}, /* index:525, gain:49.21875db, again:25.21875db, dgain:24db */
	{0x0791, 0x04}, /* index:526, gain:49.31250db, again:25.31250db, dgain:24db */
	{0x0792, 0x04}, /* index:527, gain:49.40625db, again:25.40625db, dgain:24db */
	{0x0793, 0x04}, /* index:528, gain:49.50000db, again:25.50000db, dgain:24db */
	{0x0794, 0x04}, /* index:529, gain:49.59375db, again:25.59375db, dgain:24db */
	{0x0796, 0x04}, /* index:530, gain:49.68750db, again:25.68750db, dgain:24db */
	{0x0797, 0x04}, /* index:531, gain:49.78125db, again:25.78125db, dgain:24db */
	{0x0798, 0x04}, /* index:532, gain:49.87500db, again:25.87500db, dgain:24db */
	{0x0799, 0x04}, /* index:533, gain:49.96875db, again:25.96875db, dgain:24db */
	{0x079a, 0x04}, /* index:534, gain:50.06250db, again:26.06250db, dgain:24db */
	{0x079b, 0x04}, /* index:535, gain:50.15625db, again:26.15625db, dgain:24db */
	{0x079c, 0x04}, /* index:536, gain:50.25000db, again:26.25000db, dgain:24db */
	{0x079d, 0x04}, /* index:537, gain:50.34375db, again:26.34375db, dgain:24db */
	{0x079e, 0x04}, /* index:538, gain:50.43750db, again:26.43750db, dgain:24db */
	{0x079f, 0x04}, /* index:539, gain:50.53125db, again:26.53125db, dgain:24db */
	{0x07a0, 0x04}, /* index:540, gain:50.62500db, again:26.62500db, dgain:24db */
	{0x07a2, 0x04}, /* index:541, gain:50.71875db, again:26.71875db, dgain:24db */
	{0x07a3, 0x04}, /* index:542, gain:50.81250db, again:26.81250db, dgain:24db */
	{0x07a4, 0x04}, /* index:543, gain:50.90625db, again:26.90625db, dgain:24db */
	{0x07a5, 0x04}, /* index:544, gain:51.00000db, again:27.00000db, dgain:24db */
	{0x074b, 0x05}, /* index:545, gain:51.09375db, again:21.09375db, dgain:30db */
	{0x074d, 0x05}, /* index:546, gain:51.18750db, again:21.18750db, dgain:30db */
	{0x074f, 0x05}, /* index:547, gain:51.28125db, again:21.28125db, dgain:30db */
	{0x0751, 0x05}, /* index:548, gain:51.37500db, again:21.37500db, dgain:30db */
	{0x0753, 0x05}, /* index:549, gain:51.46875db, again:21.46875db, dgain:30db */
	{0x0755, 0x05}, /* index:550, gain:51.56250db, again:21.56250db, dgain:30db */
	{0x0757, 0x05}, /* index:551, gain:51.65625db, again:21.65625db, dgain:30db */
	{0x0759, 0x05}, /* index:552, gain:51.75000db, again:21.75000db, dgain:30db */
	{0x075a, 0x05}, /* index:553, gain:51.84375db, again:21.84375db, dgain:30db */
	{0x075c, 0x05}, /* index:554, gain:51.93750db, again:21.93750db, dgain:30db */
	{0x075e, 0x05}, /* index:555, gain:52.03125db, again:22.03125db, dgain:30db */
	{0x0760, 0x05}, /* index:556, gain:52.12500db, again:22.12500db, dgain:30db */
	{0x0761, 0x05}, /* index:557, gain:52.21875db, again:22.21875db, dgain:30db */
	{0x0763, 0x05}, /* index:558, gain:52.31250db, again:22.31250db, dgain:30db */
	{0x0765, 0x05}, /* index:559, gain:52.40625db, again:22.40625db, dgain:30db */
	{0x0766, 0x05}, /* index:560, gain:52.50000db, again:22.50000db, dgain:30db */
	{0x0768, 0x05}, /* index:561, gain:52.59375db, again:22.59375db, dgain:30db */
	{0x076a, 0x05}, /* index:562, gain:52.68750db, again:22.68750db, dgain:30db */
	{0x076b, 0x05}, /* index:563, gain:52.78125db, again:22.78125db, dgain:30db */
	{0x076d, 0x05}, /* index:564, gain:52.87500db, again:22.87500db, dgain:30db */
	{0x076e, 0x05}, /* index:565, gain:52.96875db, again:22.96875db, dgain:30db */
	{0x0770, 0x05}, /* index:566, gain:53.06250db, again:23.06250db, dgain:30db */
	{0x0772, 0x05}, /* index:567, gain:53.15625db, again:23.15625db, dgain:30db */
	{0x0773, 0x05}, /* index:568, gain:53.25000db, again:23.25000db, dgain:30db */
	{0x0775, 0x05}, /* index:569, gain:53.34375db, again:23.34375db, dgain:30db */
	{0x0776, 0x05}, /* index:570, gain:53.43750db, again:23.43750db, dgain:30db */
	{0x0778, 0x05}, /* index:571, gain:53.53125db, again:23.53125db, dgain:30db */
	{0x0779, 0x05}, /* index:572, gain:53.62500db, again:23.62500db, dgain:30db */
	{0x077b, 0x05}, /* index:573, gain:53.71875db, again:23.71875db, dgain:30db */
	{0x077c, 0x05}, /* index:574, gain:53.81250db, again:23.81250db, dgain:30db */
	{0x077d, 0x05}, /* index:575, gain:53.90625db, again:23.90625db, dgain:30db */
	{0x077f, 0x05}, /* index:576, gain:54.00000db, again:24.00000db, dgain:30db */
	{0x0780, 0x05}, /* index:577, gain:54.09375db, again:24.09375db, dgain:30db */
	{0x0782, 0x05}, /* index:578, gain:54.18750db, again:24.18750db, dgain:30db */
	{0x0783, 0x05}, /* index:579, gain:54.28125db, again:24.28125db, dgain:30db */
	{0x0784, 0x05}, /* index:580, gain:54.37500db, again:24.37500db, dgain:30db */
	{0x0786, 0x05}, /* index:581, gain:54.46875db, again:24.46875db, dgain:30db */
	{0x0787, 0x05}, /* index:582, gain:54.56250db, again:24.56250db, dgain:30db */
	{0x0788, 0x05}, /* index:583, gain:54.65625db, again:24.65625db, dgain:30db */
	{0x0789, 0x05}, /* index:584, gain:54.75000db, again:24.75000db, dgain:30db */
	{0x078b, 0x05}, /* index:585, gain:54.84375db, again:24.84375db, dgain:30db */
	{0x078c, 0x05}, /* index:586, gain:54.93750db, again:24.93750db, dgain:30db */
	{0x078d, 0x05}, /* index:587, gain:55.03125db, again:25.03125db, dgain:30db */
	{0x078e, 0x05}, /* index:588, gain:55.12500db, again:25.12500db, dgain:30db */
	{0x0790, 0x05}, /* index:589, gain:55.21875db, again:25.21875db, dgain:30db */
	{0x0791, 0x05}, /* index:590, gain:55.31250db, again:25.31250db, dgain:30db */
	{0x0792, 0x05}, /* index:591, gain:55.40625db, again:25.40625db, dgain:30db */
	{0x0793, 0x05}, /* index:592, gain:55.50000db, again:25.50000db, dgain:30db */
	{0x0794, 0x05}, /* index:593, gain:55.59375db, again:25.59375db, dgain:30db */
	{0x0796, 0x05}, /* index:594, gain:55.68750db, again:25.68750db, dgain:30db */
	{0x0797, 0x05}, /* index:595, gain:55.78125db, again:25.78125db, dgain:30db */
	{0x0798, 0x05}, /* index:596, gain:55.87500db, again:25.87500db, dgain:30db */
	{0x0799, 0x05}, /* index:597, gain:55.96875db, again:25.96875db, dgain:30db */
	{0x079a, 0x05}, /* index:598, gain:56.06250db, again:26.06250db, dgain:30db */
	{0x079b, 0x05}, /* index:599, gain:56.15625db, again:26.15625db, dgain:30db */
	{0x079c, 0x05}, /* index:600, gain:56.25000db, again:26.25000db, dgain:30db */
	{0x079d, 0x05}, /* index:601, gain:56.34375db, again:26.34375db, dgain:30db */
	{0x079e, 0x05}, /* index:602, gain:56.43750db, again:26.43750db, dgain:30db */
	{0x079f, 0x05}, /* index:603, gain:56.53125db, again:26.53125db, dgain:30db */
	{0x07a0, 0x05}, /* index:604, gain:56.62500db, again:26.62500db, dgain:30db */
	{0x07a2, 0x05}, /* index:605, gain:56.71875db, again:26.71875db, dgain:30db */
	{0x07a3, 0x05}, /* index:606, gain:56.81250db, again:26.81250db, dgain:30db */
	{0x07a4, 0x05}, /* index:607, gain:56.90625db, again:26.90625db, dgain:30db */
	{0x07a5, 0x05}, /* index:608, gain:57.00000db, again:27.00000db, dgain:30db */
	{0x074b, 0x06}, /* index:609, gain:57.09375db, again:21.09375db, dgain:36db */
	{0x074d, 0x06}, /* index:610, gain:57.18750db, again:21.18750db, dgain:36db */
	{0x074f, 0x06}, /* index:611, gain:57.28125db, again:21.28125db, dgain:36db */
	{0x0751, 0x06}, /* index:612, gain:57.37500db, again:21.37500db, dgain:36db */
	{0x0753, 0x06}, /* index:613, gain:57.46875db, again:21.46875db, dgain:36db */
	{0x0755, 0x06}, /* index:614, gain:57.56250db, again:21.56250db, dgain:36db */
	{0x0757, 0x06}, /* index:615, gain:57.65625db, again:21.65625db, dgain:36db */
	{0x0759, 0x06}, /* index:616, gain:57.75000db, again:21.75000db, dgain:36db */
	{0x075a, 0x06}, /* index:617, gain:57.84375db, again:21.84375db, dgain:36db */
	{0x075c, 0x06}, /* index:618, gain:57.93750db, again:21.93750db, dgain:36db */
	{0x075e, 0x06}, /* index:619, gain:58.03125db, again:22.03125db, dgain:36db */
	{0x0760, 0x06}, /* index:620, gain:58.12500db, again:22.12500db, dgain:36db */
	{0x0761, 0x06}, /* index:621, gain:58.21875db, again:22.21875db, dgain:36db */
	{0x0763, 0x06}, /* index:622, gain:58.31250db, again:22.31250db, dgain:36db */
	{0x0765, 0x06}, /* index:623, gain:58.40625db, again:22.40625db, dgain:36db */
	{0x0766, 0x06}, /* index:624, gain:58.50000db, again:22.50000db, dgain:36db */
	{0x0768, 0x06}, /* index:625, gain:58.59375db, again:22.59375db, dgain:36db */
	{0x076a, 0x06}, /* index:626, gain:58.68750db, again:22.68750db, dgain:36db */
	{0x076b, 0x06}, /* index:627, gain:58.78125db, again:22.78125db, dgain:36db */
	{0x076d, 0x06}, /* index:628, gain:58.87500db, again:22.87500db, dgain:36db */
	{0x076e, 0x06}, /* index:629, gain:58.96875db, again:22.96875db, dgain:36db */
	{0x0770, 0x06}, /* index:630, gain:59.06250db, again:23.06250db, dgain:36db */
	{0x0772, 0x06}, /* index:631, gain:59.15625db, again:23.15625db, dgain:36db */
	{0x0773, 0x06}, /* index:632, gain:59.25000db, again:23.25000db, dgain:36db */
	{0x0775, 0x06}, /* index:633, gain:59.34375db, again:23.34375db, dgain:36db */
	{0x0776, 0x06}, /* index:634, gain:59.43750db, again:23.43750db, dgain:36db */
	{0x0778, 0x06}, /* index:635, gain:59.53125db, again:23.53125db, dgain:36db */
	{0x0779, 0x06}, /* index:636, gain:59.62500db, again:23.62500db, dgain:36db */
	{0x077b, 0x06}, /* index:637, gain:59.71875db, again:23.71875db, dgain:36db */
	{0x077c, 0x06}, /* index:638, gain:59.81250db, again:23.81250db, dgain:36db */
	{0x077d, 0x06}, /* index:639, gain:59.90625db, again:23.90625db, dgain:36db */
	{0x077f, 0x06}, /* index:640, gain:60.00000db, again:24.00000db, dgain:36db */
	{0x0780, 0x06}, /* index:641, gain:60.09375db, again:24.09375db, dgain:36db */
	{0x0782, 0x06}, /* index:642, gain:60.18750db, again:24.18750db, dgain:36db */
	{0x0783, 0x06}, /* index:643, gain:60.28125db, again:24.28125db, dgain:36db */
	{0x0784, 0x06}, /* index:644, gain:60.37500db, again:24.37500db, dgain:36db */
	{0x0786, 0x06}, /* index:645, gain:60.46875db, again:24.46875db, dgain:36db */
	{0x0787, 0x06}, /* index:646, gain:60.56250db, again:24.56250db, dgain:36db */
	{0x0788, 0x06}, /* index:647, gain:60.65625db, again:24.65625db, dgain:36db */
	{0x0789, 0x06}, /* index:648, gain:60.75000db, again:24.75000db, dgain:36db */
	{0x078b, 0x06}, /* index:649, gain:60.84375db, again:24.84375db, dgain:36db */
	{0x078c, 0x06}, /* index:650, gain:60.93750db, again:24.93750db, dgain:36db */
	{0x078d, 0x06}, /* index:651, gain:61.03125db, again:25.03125db, dgain:36db */
	{0x078e, 0x06}, /* index:652, gain:61.12500db, again:25.12500db, dgain:36db */
	{0x0790, 0x06}, /* index:653, gain:61.21875db, again:25.21875db, dgain:36db */
	{0x0791, 0x06}, /* index:654, gain:61.31250db, again:25.31250db, dgain:36db */
	{0x0792, 0x06}, /* index:655, gain:61.40625db, again:25.40625db, dgain:36db */
	{0x0793, 0x06}, /* index:656, gain:61.50000db, again:25.50000db, dgain:36db */
	{0x0794, 0x06}, /* index:657, gain:61.59375db, again:25.59375db, dgain:36db */
	{0x0796, 0x06}, /* index:658, gain:61.68750db, again:25.68750db, dgain:36db */
	{0x0797, 0x06}, /* index:659, gain:61.78125db, again:25.78125db, dgain:36db */
	{0x0798, 0x06}, /* index:660, gain:61.87500db, again:25.87500db, dgain:36db */
	{0x0799, 0x06}, /* index:661, gain:61.96875db, again:25.96875db, dgain:36db */
	{0x079a, 0x06}, /* index:662, gain:62.06250db, again:26.06250db, dgain:36db */
	{0x079b, 0x06}, /* index:663, gain:62.15625db, again:26.15625db, dgain:36db */
	{0x079c, 0x06}, /* index:664, gain:62.25000db, again:26.25000db, dgain:36db */
	{0x079d, 0x06}, /* index:665, gain:62.34375db, again:26.34375db, dgain:36db */
	{0x079e, 0x06}, /* index:666, gain:62.43750db, again:26.43750db, dgain:36db */
	{0x079f, 0x06}, /* index:667, gain:62.53125db, again:26.53125db, dgain:36db */
	{0x07a0, 0x06}, /* index:668, gain:62.62500db, again:26.62500db, dgain:36db */
	{0x07a2, 0x06}, /* index:669, gain:62.71875db, again:26.71875db, dgain:36db */
	{0x07a3, 0x06}, /* index:670, gain:62.81250db, again:26.81250db, dgain:36db */
	{0x07a4, 0x06}, /* index:671, gain:62.90625db, again:26.90625db, dgain:36db */
	{0x07a5, 0x06}, /* index:672, gain:63.00000db, again:27.00000db, dgain:36db */
};

