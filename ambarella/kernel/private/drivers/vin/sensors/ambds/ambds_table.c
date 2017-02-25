/*
 * Filename : ambds_reg_tbl.c
 *
 * History:
 *    2015/01/30 - [Long Zhao] Create
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

static struct vin_video_pll ambds_plls[] = {
	/* please set master clock here, amba soc will output this frequency from CLK_SI */
	{0, 27000000, 72000000},
};

static struct vin_reg_16_8 ambds_pll_regs[][1] = {
	{
		/* please set pll registers here */
		{0xFFFF, 0xFF}
	},
};

static struct vin_reg_16_8 ambds_share_regs[] = {
	/* please set share/common registers here */
};

static struct vin_reg_16_8 ambds_mode_regs[][1] = {
	{
		/* set mode registers for device mode 0 */
		{0xFFFF, 0xFF}
	},
	{
		/* set mode registers for device mode 1 */
		{0xFFFF, 0xFF}
	},
};

static struct vin_video_format ambds_formats[] = {
	{
		.video_mode = AMBA_VIDEO_MODE_1080P,
		.def_start_x = 0,
		.def_start_y = 0,
		.def_width = 1920,
		.def_height = 1080,
		/* device_mode, you can use this index to set different mode registers in ambds_mode_regs */
		.device_mode = 0,
		.pll_idx = 0,
		.width = 1920,
		.height = 1080,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_YUV_656,
		.bits = AMBA_VIDEO_BITS_8,
		.ratio = AMBA_VIDEO_RATIO_16_9,
		.max_fps = AMBA_VIDEO_FPS_30,
		.default_fps = AMBA_VIDEO_FPS_29_97,
	},
	{
		.video_mode = AMBA_VIDEO_MODE_3840_2160,
		.def_start_x = 0,
		.def_start_y = 0,
		.def_width = 3840,
		.def_height = 2160,
		/* device_mode, you can use this index to set different mode registers in ambds_mode_regs */
		.device_mode = 1,
		.pll_idx = 0,
		.width = 3840,
		.height = 2160,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_YUV_656,
		.bits = AMBA_VIDEO_BITS_8,
		.ratio = AMBA_VIDEO_RATIO_16_9,
		.max_fps = AMBA_VIDEO_FPS_30,
		.default_fps = AMBA_VIDEO_FPS_29_97,
	},
};

