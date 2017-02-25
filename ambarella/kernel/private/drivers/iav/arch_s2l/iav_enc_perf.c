/*
 * iav_enc_perf.c
 *
 * History:
 *	2012/04/13 - [Jian Tang] created file
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

#include <config.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <iav_utils.h>
#include <iav_ioctl.h>
#include <dsp_api.h>
#include <dsp_format.h>
#include <vin_api.h>
#include <vout_api.h>
#include "iav.h"
#include "iav_dsp_cmd.h"
#include "iav_vin.h"
#include "iav_vout.h"
#include "iav_enc_api.h"
#include "iav_enc_utils.h"


#define	DRAM_BW_DSP_PER_S2L		(55)
#define	DRAM_BW_DSP_PER_S2LM	(57)
#define	DRAM_BW_MARGIN_PER	(0)

#define	DEFAULT_VIN_FPS		(30)
#define	DEFAULT_VOUT_FPS		(30)

#define	DRAM_SIZE_MARGIN_MIN		(1 << 20)

#define	DSP_CFG_SIZE		(803584)
#define	PJPEG_SIZE			(4194304)

#define	K_SHIFT	(10)


#define	Q9_TO_FPS(fps_q9, default_fps)		\
	((fps_q9) ? DIV_ROUND_CLOSEST(FPS_Q9_BASE, (fps_q9)) : (default_fps))

static struct iav_system_freq G_system_freq[IAV_CHIP_ID_S2L_NUM] = {
	[IAV_CHIP_ID_S2L_22M] = {
		.cortex_MHz = 600,
		.idsp_MHz = 144,
		.core_MHz = 144,
		.dram_MHz = 456,
	},
#if (!defined(CONFIG_BOARD_VERSION_S2LMKIWI_S2LMHC) && !defined(CONFIG_BOARD_VERSION_S2LMIRONMAN_S2L55MHC))
	[IAV_CHIP_ID_S2L_33M] = {
		.cortex_MHz = 600,
		.idsp_MHz = 216,
		.core_MHz = 216,
		.dram_MHz = 528,
	},
	[IAV_CHIP_ID_S2L_55M] = {
		.cortex_MHz = 600,
		.idsp_MHz = 216,
		.core_MHz = 216,
		.dram_MHz = 528,
	},
	[IAV_CHIP_ID_S2L_99M] = {
		.cortex_MHz = 600,
		.idsp_MHz = 216,
		.core_MHz = 216,
		.dram_MHz = 564,
	},
#else
	[IAV_CHIP_ID_S2L_33M] = {
		.cortex_MHz = 600,
#ifndef CONFIG_AMBARELLA_IAV_DRAM_VOUT_ONLY
		.idsp_MHz = 240,
#else
		.idsp_MHz = 312,
#endif
		.core_MHz = 288,
		.dram_MHz = 564,
	},
	[IAV_CHIP_ID_S2L_55M] = {
		.cortex_MHz = 600,
#ifndef CONFIG_AMBARELLA_IAV_DRAM_VOUT_ONLY
		.idsp_MHz = 240,
#else
		.idsp_MHz = 312,
#endif
		.core_MHz = 288,
		.dram_MHz = 564,
	},
	[IAV_CHIP_ID_S2L_99M] = {
		.cortex_MHz = 600,
#ifndef CONFIG_AMBARELLA_IAV_DRAM_VOUT_ONLY
		.idsp_MHz = 240,
#else
		.idsp_MHz = 312,
#endif
		.core_MHz = 288,
		.dram_MHz = 564,
	},
#endif
	[IAV_CHIP_ID_S2L_63] = {
		.cortex_MHz = 816,
		.idsp_MHz = 336,
		.core_MHz = 312,
		.dram_MHz = 588,
	},
	[IAV_CHIP_ID_S2L_66] = {
		.cortex_MHz = 1008,
		.idsp_MHz = 504,
		.core_MHz = 432,
		.dram_MHz = 660,
	},
	[IAV_CHIP_ID_S2L_88] = {
		.cortex_MHz = 1008,
		.idsp_MHz = 504,
		.core_MHz = 432,
		.dram_MHz = 660,
	},
	[IAV_CHIP_ID_S2L_99] = {
		.cortex_MHz = 1008,
		.idsp_MHz = 504,
		.core_MHz = 432,
		.dram_MHz = 660,
	},
#if (!defined(CONFIG_BOARD_VERSION_S2LMKIWI_S2LMHC) && !defined(CONFIG_BOARD_VERSION_S2LMIRONMAN_S2L55MHC))
	[IAV_CHIP_ID_S2L_TEST] = {
		.cortex_MHz = 600,
		.idsp_MHz = 216,
		.core_MHz = 216,
		.dram_MHz = 564,
	},
#else
	[IAV_CHIP_ID_S2L_TEST] = {
		.cortex_MHz = 600,
#ifndef CONFIG_AMBARELLA_IAV_DRAM_VOUT_ONLY
		.idsp_MHz = 240,
#else
		.idsp_MHz = 312,
#endif
		.core_MHz = 288,
		.dram_MHz = 564,
	},
#endif
	[IAV_CHIP_ID_S2L_22] = {
		.cortex_MHz = 816,
		.idsp_MHz = 240,
		.core_MHz = 288,
		.dram_MHz = 588,
	},
	[IAV_CHIP_ID_S2L_33MEX] = {
		.cortex_MHz = 600,
		.idsp_MHz = 144,
		.core_MHz = 144,
		.dram_MHz = 456,
	},
	[IAV_CHIP_ID_S2L_33EX] = {
		.cortex_MHz = 816,
		.idsp_MHz = 144,
		.core_MHz = 144,
		.dram_MHz = 456,
	},
};

struct iav_dram_coeff G_dram_buf_coeffs[DSP_ENCODE_MODE_TOTAL_NUM] = {
	[DSP_NORMAL_ISO_MODE] = {
		.vin = {
			{27, 32},
			{2, 1},
		},
		.dummy = {
			{27, 32},
			{2, 1},
		},
		.preblend = {
			1, 1
		},
		.lpf = {
			0, 1
		},
		.vout = {
			{2, 1},
			{2, 1},
		},
		.buf_yuv = {
			{3, 2},
			{3, 2},
			{3, 2},
			{3, 2},
			{0, 1},
		},
		.buf_me1 = {
			{1, 1},
			{1, 1},
			{1, 1},
			{1, 1},
			{0, 1},
		},
		.buf_me0 = {
			{0, 1},
			{0, 1},
			{0, 1},
			{0, 1},
			{0, 1},
		},
		.strm_yuv = {
			{3, 2},
			{3, 2},
			{3, 2},
			{3, 2},
		},
		.strm_me1 = {
			{1, 1},
			{1, 1},
			{1, 1},
			{1, 1},
		},
	},
	[DSP_MULTI_REGION_WARP_MODE] = {
		.vin = {
			{27, 32},
			{2, 1},
		},
		.dummy = {
			{27, 32},
			{2, 1},
		},
		.preblend = {
			0, 1
		},
		.lpf = {
			0, 1
		},
		.vout = {
			{2, 1},
			{2, 1},
		},
		.buf_yuv = {
			{3, 2},
			{3, 2},
			{3, 2},
			{3, 2},
			{3, 2},
		},
		.buf_me1 = {
			{1, 1},
			{1, 1},
			{1, 1},
			{1, 1},
			{1, 1},
		},
		.buf_me0 = {
			{0, 1},
			{0, 1},
			{0, 1},
			{0, 1},
			{0, 1},
		},
		.strm_yuv = {
			{3, 2},
			{3, 2},
			{3, 2},
			{3, 2},
		},
		.strm_me1 = {
			{1, 1},
			{1, 1},
			{1, 1},
			{1, 1},
		},
	},
	[DSP_BLEND_ISO_MODE] = {
		.vin = {
			{27, 32},
			{2, 1},
		},
		.dummy = {
			{27, 32},
			{2, 1},
		},
		.preblend = {
			1, 1
		},
		.lpf = {
			0, 1
		},
		.vout = {
			{2, 1},
			{2, 1},
		},
		.buf_yuv = {
			{3, 2},
			{3, 2},
			{3, 2},
			{3, 2},
			{0, 1},
		},
		.buf_me1 = {
			{1, 1},
			{1, 1},
			{1, 1},
			{1, 1},
			{0, 1},
		},
		.buf_me0 = {
			{0, 1},
			{0, 1},
			{0, 1},
			{0, 1},
			{0, 1},
		},
		.strm_yuv = {
			{3, 2},
			{3, 2},
			{3, 2},
			{3, 2},
		},
		.strm_me1 = {
			{1, 1},
			{1, 1},
			{1, 1},
			{1, 1},
		},
	},
	[DSP_SINGLE_REGION_WARP_MODE] = {
		.vin = {
			{0, 1},
			{0, 1},
		},
		.dummy = {
			{0, 1},
			{0, 1},
		},
		.preblend = {
			0, 1
		},
		.lpf = {
			0, 1
		},
		.vout = {
			{2, 1},
			{2, 1},
		},
		.buf_yuv = {
			{3, 2},
			{3, 2},
			{3, 2},
			{3, 2},
			{0, 1},
		},
		.buf_me1 = {
			{1, 1},
			{1, 1},
			{1, 1},
			{1, 1},
			{0, 1},
		},
		.buf_me0 = {
			{0, 1},
			{0, 1},
			{0, 1},
			{0, 1},
			{0, 1},
		},
		.strm_yuv = {
			{3, 2},
			{3, 2},
			{3, 2},
			{3, 2},
		},
		.strm_me1 = {
			{1, 1},
			{1, 1},
			{1, 1},
			{1, 1},
		},
	},
	[DSP_ADVANCED_ISO_MODE] = {
		.vin = {
			{27, 32},
			{2, 1},
		},
		.dummy = {
			{27, 32},
			{2, 1},
		},
		.preblend = {
			1, 1
		},
		.lpf = {
			0, 1
		},
		.vout = {
			{2, 1},
			{2, 1},
		},
		.buf_yuv = {
			{3, 2},
			{3, 2},
			{3, 2},
			{3, 2},
			{0, 1},
		},
		.buf_me1 = {
			{1, 1},
			{1, 1},
			{1, 1},
			{1, 1},
			{0, 1},
		},
		.buf_me0 = {
			{1, 1},
			{0, 1},
			{0, 1},
			{0, 1},
			{0, 1},
		},
		.strm_yuv = {
			{3, 2},
			{3, 2},
			{3, 2},
			{3, 2},
		},
		.strm_me1 = {
			{1, 1},
			{1, 1},
			{1, 1},
			{1, 1},
		},
	},
	[DSP_HDR_LINE_INTERLEAVED_MODE] = {
		.vin = {
			{27, 32},
			{2, 1},
		},
		.dummy = {
			{27, 32},
			{2, 1},
		},
		.preblend = {
			1, 1
		},
		.lpf = {
			1, 1
		},
		.vout = {
			{2, 1},
			{2, 1},
		},
		.buf_yuv = {
			{3, 2},
			{3, 2},
			{3, 2},
			{3, 2},
			{0, 1},
		},
		.buf_me1 = {
			{1, 1},
			{1, 1},
			{1, 1},
			{1, 1},
			{0, 1},
		},
		.buf_me0 = {
			{0, 1},
			{0, 1},
			{0, 1},
			{0, 1},
			{0, 1},
		},
		.strm_yuv = {
			{3, 2},
			{3, 2},
			{3, 2},
			{3, 2},
		},
		.strm_me1 = {
			{1, 1},
			{1, 1},
			{1, 1},
			{1, 1},
		},
	},
	[DSP_HISO_VIDEO_MODE] = {
		.vin = {
			{27, 32},
			{2, 1},
		},
		.dummy = {
			{27, 32},
			{2, 1},
		},
		.preblend = {
			0, 1
		},
		.lpf = {
			0, 1
		},
		.vout = {
			{2, 1},
			{2, 1},
		},
		.buf_yuv = {
			{9, 2},
			{3, 2},
			{3, 2},
			{3, 2},
		},
		.buf_me1 = {
			{1, 1},
			{1, 1},
			{1, 1},
			{1, 1},
		},
		.strm_yuv = {
			{3, 2},
			{3, 2},
			{3, 2},
			{3, 2},
		},
		.strm_me1 = {
			{1, 1},
			{1, 1},
			{1, 1},
			{1, 1},
		},
	},
};

static struct iav_coeff G_bw_mn_buf_num[DSP_ENCODE_MODE_TOTAL_NUM] = {
	[DSP_NORMAL_ISO_MODE] = {3, 1},
	[DSP_MULTI_REGION_WARP_MODE] = {1, 1},
	[DSP_BLEND_ISO_MODE] = {14, 3},
	[DSP_SINGLE_REGION_WARP_MODE] = {5, 1},
	[DSP_ADVANCED_ISO_MODE] = {19, 3},
	[DSP_HDR_LINE_INTERLEAVED_MODE] = {4, 1},
	[DSP_HISO_VIDEO_MODE] = {3, 1},
};

static struct iav_dram_buf_num G_sz_buf_num[DSP_ENCODE_MODE_TOTAL_NUM] = {
	[DSP_NORMAL_ISO_MODE] = {
		1, 0, 1, 0,
		{5, 5},
		{5, 9, 9, 9, 0},
		{15, 16, 16, 16, 0},
		{0, 0, 0, 0, 0},
		{1, 1, 1, 1},
		{1, 1, 1, 1},
	},
	[DSP_MULTI_REGION_WARP_MODE] = {
		1, 0, 1, 0,
		{5, 5},
		{4, 9, 9, 9, 6},
		{8, 16, 16, 16, 21},
		{0, 0, 0, 0, 0},
		{1, 1, 1, 1},
		{1, 1, 1, 1},
	},
	[DSP_BLEND_ISO_MODE] = {
		1, 0, 1, 0,
		{5, 5},
		{5, 9, 9, 9, 0},
		{7, 16, 16, 16, 0},
		{0, 0, 0, 0, 0},
		{1, 1, 1, 1},
		{1, 1, 1, 1},
	},
	[DSP_ADVANCED_ISO_MODE] = {
		1, 0, 1, 0,
		{5, 5},
		{8, 9, 9, 9, 0},
		{21, 16, 16, 16, 0},
		{2, 0, 0, 0, 0},
		{1, 1, 1, 1},
		{1, 1, 1, 1},
	},
	[DSP_HDR_LINE_INTERLEAVED_MODE] = {
		1, 0, 1, 0,
		{5, 5},
		{10, 9, 9, 9, 0},
		{16, 16, 16, 16, 0},
		{0, 0, 0, 0, 0},
		{1, 1, 1, 1},
		{1, 1, 1, 1},
	},
};


static u32 get_dsp_mem_warp_bandwidth(struct ambarella_iav *iav)
{
	u32 bandwidth_KBs = 0;
	int i = 0, area_rotate;
	struct iav_warp_area *area;
	u32 vwarp_in, vwarp_out, hwarp_out;

	for (i = 0; i < MAX_NUM_WARP_AREAS; i++) {
		area = &iav->warp->area[i];
		if (area->enable) {
			area_rotate = !!(area->rotate_flip & (1 << 2));
			vwarp_in = (1 + area_rotate) * area->input.width * area->input.height;
			vwarp_out = area_rotate ? ((area->input.width >= area->output.width ?
				area->input.width : area->output.width) * area->input.height) :
				((area->input.height >= area->output.height ? area->input.height :
				area->output.height) * area->input.width);
			hwarp_out = area->output.width * area->output.height;
			// Bandwidth in KB
			bandwidth_KBs += ((vwarp_in + 2 * vwarp_out + hwarp_out +
				iav->warp_dptz[IAV_SRCBUF_PA].output[i].width *
				iav->warp_dptz[IAV_SRCBUF_PA].output[i].height +
				iav->warp_dptz[IAV_SRCBUF_PC].output[i].width *
				iav->warp_dptz[IAV_SRCBUF_PC].output[i].height) * 3 / 2 +
				hwarp_out / 16) >> K_SHIFT;
		}
	}

	return bandwidth_KBs;
}

static u32 get_dsp_mem_mode_bandwidth(struct ambarella_iav *iav)
{
	u32 bandwidth_KBs = 0;
	u32 vin_width, vin_height;
	u32 dummy_width, dummy_height;
	u32 dummy_num, preblend_div, lpf_num;
	u32 main_pixels;
	u32 fps, expo_num, iso_type;
	u8 raw_flag = 0, me1_count = 0;
	u8 enc_mode = iav->encode_mode;
	struct iav_rect *vin_win;
	struct iav_dram_coeff *dram_coeffs = &G_dram_buf_coeffs[enc_mode];
	struct iav_vin_format *vin_format = &iav->vinc[0]->vin_format;
	struct iav_buffer *main_buf = &iav->srcbuf[IAV_SRCBUF_MN];
	struct iav_coeff *main_buf_num = &G_bw_mn_buf_num[enc_mode];
	struct iav_system_config *sys_cfg = &iav->system_config[enc_mode];

	raw_flag = sys_cfg->raw_capture;
	get_vin_win(iav, &vin_win, 1);
	vin_width = ALIGN(vin_win->width, PIXEL_IN_MB);
	vin_height = ALIGN(vin_win->height, PIXEL_IN_MB);
	dummy_width = ALIGN(main_buf->input.width, PIXEL_IN_MB);
	if (!dummy_width) {
		dummy_width = vin_width;
	}
	dummy_height = ALIGN(main_buf->input.height, PIXEL_IN_MB);
	if (!dummy_height) {
		dummy_height = vin_height;
	}
	main_pixels = ALIGN(main_buf->win.width, PIXEL_IN_MB) *
		ALIGN(main_buf->win.height, PIXEL_IN_MB);

	fps = Q9_TO_FPS(vin_format->frame_rate, DEFAULT_VIN_FPS);
	expo_num = sys_cfg->expo_num;

	me1_count = 0;
	switch (enc_mode) {
	case DSP_MULTI_REGION_WARP_MODE:
		bandwidth_KBs += get_dsp_mem_warp_bandwidth(iav);
		me1_count = 2;
		break;
	case DSP_ADVANCED_ISO_MODE:
		/* update main buffer num according to iso type */
		iso_type = get_iso_type(iav, sys_cfg->debug_enable_map);
		if (iso_type == ISO_TYPE_MIDDLE) {
			main_buf_num->multi = 4;
			main_buf_num->div = 1;
		} else {
			main_buf_num->multi = 19;
			main_buf_num->div = 3;
			/* Warning: If main buffer bandwidth exceeds the limit in advanced
			 * ISO mode, stream will drop frames. */
			if (((main_pixels >> 8) * fps) > LOAD_4MP30) {
				iav_warn("Main buffer bandwidth Macro Block/s [%d] exceeds "
					"the limit [%d], stream will drop frames.\n",
					((main_pixels >> 8) * fps), LOAD_4MP30);
			}
		}
		me1_count = 2;
		break;
	case DSP_HDR_LINE_INTERLEAVED_MODE:
		me1_count = 2;
		break;
	case DSP_BLEND_ISO_MODE:
		me1_count = 2;
		break;
	default:
		break;
	}

	switch (expo_num) {
	case 1:
		if (iav->encode_mode == DSP_BLEND_ISO_MODE) {
			dummy_num = 2;
		} else {
			dummy_num = 1;
		}
		preblend_div = 0;
		lpf_num = 1;
		bandwidth_KBs += ((vin_width * vin_height *
				dram_coeffs->vin[raw_flag].multi /
				dram_coeffs->vin[raw_flag].div +
				dummy_width * dummy_height *
				dram_coeffs->dummy[raw_flag].multi /
				dram_coeffs->dummy[raw_flag].div * dummy_num +
				dummy_width * dummy_height *
				dram_coeffs->preblend.multi /
				dram_coeffs->preblend.div * preblend_div +
				main_pixels * dram_coeffs->lpf.multi /
				dram_coeffs->lpf.div * lpf_num +
				main_pixels * dram_coeffs->buf_yuv[IAV_SRCBUF_MN].multi *
				main_buf_num->multi /
				dram_coeffs->buf_yuv[IAV_SRCBUF_MN].div /
				main_buf_num->div +
				((main_pixels * me1_count) >> 4)) *
				fps) >> K_SHIFT;
		break;
	case 2:
		switch (iav->encode_mode) {
		case DSP_HDR_LINE_INTERLEAVED_MODE:
			dummy_num = 3;
			lpf_num = 2;
			break;
		case DSP_BLEND_ISO_MODE:
			dummy_num = 4;
			lpf_num = 0;
			break;
		default:
			dummy_num = 2;
			lpf_num = 0;
			break;
		}
		preblend_div = 4;
		bandwidth_KBs += ((vin_width * vin_height *
				dram_coeffs->vin[raw_flag].multi /
				dram_coeffs->vin[raw_flag].div +
				dummy_width * dummy_height *
				dram_coeffs->dummy[raw_flag].multi /
				dram_coeffs->dummy[raw_flag].div * dummy_num +
				dummy_width * dummy_height *
				dram_coeffs->preblend.multi /
				dram_coeffs->preblend.div / preblend_div +
				main_pixels * dram_coeffs->lpf.multi /
				dram_coeffs->lpf.div * lpf_num +
				main_pixels * dram_coeffs->buf_yuv[IAV_SRCBUF_MN].multi *
				main_buf_num->multi /
				dram_coeffs->buf_yuv[IAV_SRCBUF_MN].div /
				main_buf_num->div +
				((main_pixels * me1_count) >> 4)) *
				fps) >> K_SHIFT;
		break;
	case 3:
		dummy_num = 6;
		preblend_div = 2;
		lpf_num = 2;
		bandwidth_KBs += ((vin_width * vin_height *
				dram_coeffs->vin[raw_flag].multi /
				dram_coeffs->vin[raw_flag].div +
				dummy_width * dummy_height *
				dram_coeffs->dummy[raw_flag].multi /
				dram_coeffs->dummy[raw_flag].div * dummy_num +
				dummy_width * dummy_height *
				dram_coeffs->preblend.multi /
				dram_coeffs->preblend.div / preblend_div +
				main_pixels * dram_coeffs->lpf.multi /
				dram_coeffs->lpf.div * lpf_num +
				main_pixels * dram_coeffs->buf_yuv[IAV_SRCBUF_MN].multi *
				main_buf_num->multi /
				dram_coeffs->buf_yuv[IAV_SRCBUF_MN].div /
				main_buf_num->div +
				((main_pixels * me1_count) >> 4)) *
				fps) >> K_SHIFT;
		break;
	case 4:
		dummy_num = 9;
		preblend_div = 8;
		lpf_num = 2;
		bandwidth_KBs += ((vin_width * vin_height *
				dram_coeffs->vin[raw_flag].multi /
				dram_coeffs->vin[raw_flag].div +
				dummy_width * dummy_height *
				dram_coeffs->dummy[raw_flag].multi /
				dram_coeffs->dummy[raw_flag].div * dummy_num +
				dummy_width * dummy_height *
				dram_coeffs->preblend.multi /
				dram_coeffs->preblend.div * 6 / preblend_div +
				main_pixels * dram_coeffs->lpf.multi /
				dram_coeffs->lpf.div * lpf_num +
				main_pixels * dram_coeffs->buf_yuv[IAV_SRCBUF_MN].multi *
				main_buf_num->multi /
				dram_coeffs->buf_yuv[IAV_SRCBUF_MN].div /
				main_buf_num->div +
				((main_pixels * me1_count) >> 4)) *
				fps) >> K_SHIFT;
		break;
	default:
		iav_error("Incorrect expo_num for memory bandwidth calculation!\n");
		break;
	}

	return bandwidth_KBs;
}

static u32 get_dsp_mem_buffer_bandwidth(struct ambarella_iav *iav)
{
	struct iav_dram_coeff *dram_coeffs = &G_dram_buf_coeffs[iav->encode_mode];
	struct amba_video_info * video = NULL;
	u32 bandwidth = 0;
	u32 pixel_ps, fps, vout[2];
	struct iav_window vout_win;
	int i;

	/* Dual VOUT */
	if (iav->system_config[iav->encode_mode].vout_swap) {
		vout[0] = (iav->srcbuf[IAV_SRCBUF_PB].type == IAV_SRCBUF_TYPE_PREVIEW);
		vout[1] = (iav->srcbuf[IAV_SRCBUF_PA].type == IAV_SRCBUF_TYPE_PREVIEW);
	} else {
		vout[0] = (iav->srcbuf[IAV_SRCBUF_PA].type == IAV_SRCBUF_TYPE_PREVIEW);
		vout[1] = (iav->srcbuf[IAV_SRCBUF_PB].type == IAV_SRCBUF_TYPE_PREVIEW);
	}
	for (i = 0; i < 2; ++i) {
		if (G_voutinfo[i].enabled && vout[i]) {
			video = &G_voutinfo[i].video_info;
			fps = Q9_TO_FPS(video->fps, DEFAULT_VOUT_FPS);
			pixel_ps = ALIGN(video->width, PIXEL_IN_MB) *
				ALIGN(video->height, PIXEL_IN_MB) * fps;

			/* If vout mode is an interlaced mode, bandwidth will reduce half. */
			if (video->format == AMBA_VIDEO_FORMAT_INTERLACE) {
				pixel_ps >>= 1;
			}
			bandwidth += pixel_ps * dram_coeffs->vout[i].multi /
				dram_coeffs->vout[i].div;
		}
	}

	/* Main source buffer */
	fps = Q9_TO_FPS(iav->vinc[0]->vin_format.frame_rate, DEFAULT_VIN_FPS);
	pixel_ps = ALIGN(iav->srcbuf[IAV_SRCBUF_MN].win.width, PIXEL_IN_MB) *
		ALIGN(iav->srcbuf[IAV_SRCBUF_MN].win.height, PIXEL_IN_MB) * fps;
	bandwidth += (pixel_ps >> 4) *
		dram_coeffs->buf_me1[IAV_SRCBUF_MN].multi /
		dram_coeffs->buf_me1[IAV_SRCBUF_MN].div;

	/* Sub source buffers */
	for (i = IAV_SUB_SRCBUF_FIRST; i < IAV_SUB_SRCBUF_LAST; ++i) {
		switch (iav->srcbuf[i].type) {
		case IAV_SRCBUF_TYPE_ENCODE:
		case IAV_SRCBUF_TYPE_VCA:
			pixel_ps = ALIGN(iav->srcbuf[i].win.width, PIXEL_IN_MB) *
				ALIGN(iav->srcbuf[i].win.height, PIXEL_IN_MB) * fps;
			bandwidth += pixel_ps * dram_coeffs->buf_yuv[i].multi /
				dram_coeffs->buf_yuv[i].div;
			bandwidth += (pixel_ps >> 4) * dram_coeffs->buf_me1[i].multi /
				dram_coeffs->buf_me1[i].div;
			break;
		case IAV_SRCBUF_TYPE_PREVIEW:
			if (get_vout_win(i, &vout_win) < 0) {
				vout_win.width = vout_win.height = 0;
			}
			pixel_ps = ALIGN(vout_win.width, PIXEL_IN_MB) *
				ALIGN(vout_win.height, PIXEL_IN_MB) * fps;
			bandwidth += (pixel_ps << 1);
			break;
		case IAV_SRCBUF_TYPE_OFF:
		default:
			break;
		}
	}

	return bandwidth >>= K_SHIFT;
}

static u32 get_dsp_mem_stream_bandwidth(struct ambarella_iav *iav,
	u32 stream_map)
{
	struct iav_dram_coeff *dram_coeffs = &G_dram_buf_coeffs[iav->encode_mode];
	struct iav_stream *stream;
	u32 bandwidth = 0;
	u32 fps, pixel_ps;
	u32 vin_fps;
	int i;

	vin_fps = Q9_TO_FPS(iav->vinc[0]->vin_format.frame_rate, DEFAULT_VIN_FPS);

	/* stream bandwidth */
	for (i = 0; i < IAV_STREAM_MAX_NUM_ALL; ++i) {
		stream = &iav->stream[i];
		if (!is_stream_in_idle(stream) || (stream_map & (1 << i))) {
			fps = vin_fps * stream->fps.fps_multi / stream->fps.fps_div;
			pixel_ps = ALIGN(stream->format.enc_win.width, PIXEL_IN_MB) *
				ALIGN(stream->format.enc_win.height, PIXEL_IN_MB) * fps;
			if (stream->format.type == IAV_STREAM_TYPE_H264) {
				bandwidth += pixel_ps *
					dram_coeffs->strm_yuv[i].multi /
					dram_coeffs->strm_yuv[i].div *
					(stream->h264_config.multi_ref_p +
					stream->format.rotate_cw + 3);
				bandwidth += (pixel_ps >> 4) *
					dram_coeffs->strm_me1[i].multi /
					dram_coeffs->strm_me1[i].div *
					(stream->format.rotate_cw + 1) *
					(stream->h264_config.multi_ref_p + 2);
			} else {
				bandwidth += pixel_ps *
					dram_coeffs->strm_yuv[i].multi /
					dram_coeffs->strm_yuv[i].div *
					(stream->format.rotate_cw + 1);
			}
		}
	}

	return bandwidth >>= K_SHIFT;
}

static int check_dsp_mem_bandwidth(struct ambarella_iav *iav,
	int iav_state, u32 stream_map)
{
	u32 bandwidth_KBs, limit_KBs;
	u32 chip_id;

	bandwidth_KBs = get_dsp_mem_mode_bandwidth(iav);
	bandwidth_KBs += get_dsp_mem_buffer_bandwidth(iav);
	if (iav_state == IAV_STATE_ENCODING) {
		bandwidth_KBs += get_dsp_mem_stream_bandwidth(iav, stream_map);
	}

	chip_id = get_chip_id(iav);
	if ((chip_id >= IAV_CHIP_ID_S2L_FIRST) &&
		(chip_id != IAV_CHIP_ID_S2L_TEST)) {
		/* S2L */
		limit_KBs = (G_system_freq[chip_id].dram_MHz * 8 * DRAM_BW_DSP_PER_S2L * 100u
			* (100 + DRAM_BW_MARGIN_PER)) >> K_SHIFT;
	} else {
		/* S2LM */
		limit_KBs = (G_system_freq[chip_id].dram_MHz * 4 * DRAM_BW_DSP_PER_S2LM * 100u
			* (100 + DRAM_BW_MARGIN_PER)) >> K_SHIFT;
	}

	iav_debug("S2L chip ID [%u], DSP memory bandwidth [%u MB/s],"
		" limit [%u MB/s].\n", chip_id, (bandwidth_KBs >> K_SHIFT), (limit_KBs >> K_SHIFT));
	if (bandwidth_KBs > limit_KBs) {
		iav_error("DSP memory bandwidth [%u MB/s] is out of the limit [%u MB/s]"
			" for mode [%d]. Please reduce VIN size or main buffer size!\n",
			(bandwidth_KBs >> K_SHIFT), (limit_KBs >> K_SHIFT), iav->encode_mode);
		return -1;
	}

	return 0;
}

static int get_dsp_mem_buffer_size(struct ambarella_iav *iav)
{
	u32 size = 0;
	u32 vin_width, vin_height;
	u32 dummy_width, dummy_height;
	u32 main_width, main_height;
	u32 buf_width, buf_height, buf_pixels;
	u32 expo_num;
	u32 enc_mode = iav->encode_mode;
	u32 raw_flag;
	u32 extra_top_row_flag;
	int i;
	struct iav_rect *vin_win;
	struct iav_dram_coeff *dram_coeffs = &G_dram_buf_coeffs[enc_mode];
	struct iav_dram_buf_num *buf_num = &G_sz_buf_num[enc_mode];
	struct iav_system_config *sys_config = &iav->system_config[enc_mode];
	struct iav_buffer *main_buf = &iav->srcbuf[IAV_SRCBUF_MN];
	struct iav_buffer *curr_buf;
	struct iav_window vout_win;

	raw_flag = sys_config->raw_capture;
	extra_top_row_flag = sys_config->extra_top_row_buf_enable;
	get_vin_win(iav, &vin_win, 1);
	vin_width = ALIGN(vin_win->width, PIXEL_IN_MB);
	vin_height = ALIGN(vin_win->height, PIXEL_IN_MB);

	/* dummy buffer allocated according to VIN size */
	dummy_width = vin_width;
	dummy_height = vin_height;

	main_width = ALIGN(main_buf->max.width, PIXEL_IN_MB);
	main_height = ALIGN(main_buf->max.height, PIXEL_IN_MB);

	expo_num = sys_config->expo_num;

	switch (expo_num) {
	case 1:
		size += vin_width * vin_height * buf_num->vin *
				dram_coeffs->vin[raw_flag].multi /
				dram_coeffs->vin[raw_flag].div * 3 +
				dummy_width * dummy_height * buf_num->dummy *
				dram_coeffs->dummy[raw_flag].multi /
				dram_coeffs->dummy[raw_flag].div +
				dummy_width * dummy_height * buf_num->preblend *
				dram_coeffs->preblend.multi /
				dram_coeffs->preblend.div * 0 +
				main_width * main_height * buf_num->lpf *
				dram_coeffs->lpf.multi /
				dram_coeffs->lpf.div;
		break;
	case 2:
		size += vin_width * vin_height * buf_num->vin *
				dram_coeffs->vin[raw_flag].multi /
				dram_coeffs->vin[raw_flag].div * 3 +
				dummy_width * dummy_height * buf_num->dummy *
				dram_coeffs->dummy[raw_flag].multi /
				dram_coeffs->dummy[raw_flag].div +
				dummy_width * dummy_height * buf_num->preblend *
				dram_coeffs->preblend.multi /
				dram_coeffs->preblend.div * 3 / 8 +
				main_width * main_height * buf_num->lpf *
				dram_coeffs->lpf.multi /
				dram_coeffs->lpf.div;
		break;
	case 3:
		size += vin_width * vin_height * buf_num->vin *
				dram_coeffs->vin[raw_flag].multi /
				dram_coeffs->vin[raw_flag].div * 4 +
				dummy_width * dummy_height * buf_num->dummy *
				dram_coeffs->dummy[raw_flag].multi /
				dram_coeffs->dummy[raw_flag].div +
				dummy_width * dummy_height * buf_num->preblend *
				dram_coeffs->preblend.multi /
				dram_coeffs->preblend.div * 3 / 8 +
				main_width * main_height * buf_num->lpf *
				dram_coeffs->lpf.multi /
				dram_coeffs->lpf.div;
		break;
	default:
		iav_error("Incorrect expo_num for memory size calculation!\n");
		break;
	}

	/* intermediate main */
	if (enc_mode == DSP_MULTI_REGION_WARP_MODE) {
		size += sys_config->v_warped_main_max_width *
			sys_config->v_warped_main_max_height * 3;
	}

	/* source buffer size */
	for (i = IAV_SRCBUF_FIRST; i < IAV_SRCBUF_LAST_PMN; ++i) {
		curr_buf = &iav->srcbuf[i];
		switch (curr_buf->type) {
		case IAV_SRCBUF_TYPE_ENCODE:
		case IAV_SRCBUF_TYPE_VCA:
			buf_width = ALIGN(curr_buf->max.width, PIXEL_IN_MB);
			buf_height = ALIGN(curr_buf->max.height, PIXEL_IN_MB);
			buf_pixels = buf_width * buf_height;
			size += buf_pixels * (buf_num->buf_yuv[i] +
				curr_buf->extra_dram_buf) *
				dram_coeffs->buf_yuv[i].multi /
				dram_coeffs->buf_yuv[i].div;
			buf_pixels = (buf_pixels >> 4);
			size += buf_pixels * (buf_num->buf_me1[i] +
				curr_buf->extra_dram_buf) *
				dram_coeffs->buf_me1[i].multi /
				dram_coeffs->buf_me1[i].div;
			/* me0 shares the same size as me1 */
			if (sys_config->me0_scale != ME0_SCALE_OFF) {
				size += buf_pixels * buf_num->buf_me0[i] *
					dram_coeffs->buf_me0[i].multi /
					dram_coeffs->buf_me0[i].div;
			}
			if (extra_top_row_flag) {
				buf_pixels = buf_width * PIXEL_IN_MB;
				size += buf_pixels * (buf_num->buf_yuv[i] +
					curr_buf->extra_dram_buf) *
					dram_coeffs->buf_yuv[i].multi /
					dram_coeffs->buf_yuv[i].div;
				buf_pixels = (buf_pixels >> 4);
				size += buf_pixels * (buf_num->buf_me1[i] +
					curr_buf->extra_dram_buf) *
					dram_coeffs->buf_me1[i].multi /
					dram_coeffs->buf_me1[i].div;
			}
			break;
		case IAV_SRCBUF_TYPE_PREVIEW:
			if (get_vout_win(i, &vout_win) < 0) {
				vout_win.width = vout_win.height = 0;
			}
			buf_pixels = ALIGN(vout_win.width, PIXEL_IN_MB) * ALIGN(vout_win.height, PIXEL_IN_MB);

			if (i == IAV_SRCBUF_PA) {
				size += buf_pixels * buf_num->vout[0] *
					dram_coeffs->vout[0].multi /
					dram_coeffs->vout[0].div;
			} else if (i == IAV_SRCBUF_PB){
				size += buf_pixels * buf_num->vout[1] *
					dram_coeffs->vout[1].multi /
					dram_coeffs->vout[1].div;
			}
			break;
		case IAV_SRCBUF_TYPE_OFF:
		default:
			break;
		}
	}

	size += DSP_CFG_SIZE;
	size += PJPEG_SIZE;

	return size;
}

static u32 get_dsp_mem_stream_size(struct ambarella_iav *iav)
{
	struct iav_dram_coeff *dram_coeffs = &G_dram_buf_coeffs[iav->encode_mode];
	struct iav_dram_buf_num *buf_num = &G_sz_buf_num[iav->encode_mode];
	u32 size = 0;
	u32 width, height;
	int i;

	/* stream size */
	for (i = 0; i < IAV_STREAM_MAX_NUM_ALL; ++i) {
		width = ALIGN(iav->stream[i].max.width, PIXEL_IN_MB);
		height = ALIGN(iav->stream[i].max.height, PIXEL_IN_MB);
		if (iav->stream[i].max_GOP_M) {
			/* allocate H264 stream buffer */
			size += width * height * buf_num->strm_yuv[i] *
				dram_coeffs->strm_yuv[i].multi /
				dram_coeffs->strm_yuv[i].div *
				(iav->stream[i].max_GOP_M + 1);
			size += width * height * buf_num->strm_col[i] *
				dram_coeffs->strm_me1[i].multi /
				dram_coeffs->strm_me1[i].div  / 32 *
				(iav->stream[i].max_GOP_M + 1);
		}
	}

	return size;
}

static int check_dsp_mem_size(struct ambarella_iav *iav)
{
	u32 size, limit;

	size = get_dsp_mem_buffer_size(iav);
	size += get_dsp_mem_stream_size(iav);
	size += DRAM_SIZE_MARGIN_MIN;

	limit = iav->dsp->buffer_size;
	iav_debug("DSP DRAM size used [%u MB], limit [%u MB].\n",
		(size >> 20), (limit >> 20));
	if (size> limit) {
		iav_error("DSP DRAM [%u MB] exceeds the limit [%u MB] for mode [%d]."
			" Please reduce VIN size / max buffer size / max stream size!\n",
			(size >> 20), (limit >> 20), iav->encode_mode);
		return -1;
	}

	return 0;
}

int iav_check_sys_mem_usage(struct ambarella_iav *iav, int iav_state, u32 stream_map)
{
	iav_no_check();

	if (check_dsp_mem_bandwidth(iav, iav_state, stream_map) < 0) {
		iav_error("DSP memory bandwidth is NOT enough in mode [%d].\n",
			iav->encode_mode);
		return -1;
	}
	if (check_dsp_mem_size(iav) < 0) {
		iav_error("DSP DRAM size is not enough in mode [%d].\n",
			iav->encode_mode);
		return -1;
	}

	return 0;
}

int iav_check_warp_idsp_perf(struct ambarella_iav *iav,
	struct iav_warp_main* warp_areas)
{
	int i = 0;
	struct iav_warp_area *area;
	u32 idsp_perf_KPs = 0, idsp_perf_limit_KPs = 0;
	u32 input, output;
	u32 chip_id;
	u32 dummy_pixel, main_pixel;
	u32 vin_fps;
	struct iav_rect *vin_win;
	struct iav_buffer *main_buf = &iav->srcbuf[IAV_SRCBUF_MN];
	struct iav_buffer *premain_buf = &iav->srcbuf[IAV_SRCBUF_PMN];

	iav_no_check();

	for (i = 0; i < MAX_NUM_WARP_AREAS; i++) {
		if (warp_areas) {
			area = &warp_areas->area[i];
		} else {
			area = &iav->warp->area[i];
		}
		if (area->enable) {
			input = area->input.width * area->input.height;
			output = area->output.width * area->output.height;
			idsp_perf_KPs += (input >= output ? input : output);
		}
	}

	chip_id = get_chip_id(iav);

	get_vin_win(iav, &vin_win, 1);
	dummy_pixel = premain_buf->input.width * premain_buf->input.height;
	main_pixel = main_buf->max.width * main_buf->max.height;
	vin_fps = Q9_TO_FPS(iav->vinc[0]->vin_format.frame_rate, DEFAULT_VIN_FPS);

	idsp_perf_limit_KPs = ((G_system_freq[chip_id].idsp_MHz << K_SHIFT) -
		(((main_pixel >= dummy_pixel ? main_pixel : dummy_pixel) * vin_fps) >> K_SHIFT)) / 2;
	idsp_perf_KPs = (idsp_perf_KPs >> K_SHIFT) * vin_fps;

	if (idsp_perf_KPs > idsp_perf_limit_KPs) {
		iav_error("IDSP Performace [%u KP/s] Exceeds Limit [%u KP/s] in Dewarp mode.\n",
			idsp_perf_KPs, idsp_perf_limit_KPs);
		return -1;
	}

	return 0;
}

int iav_check_sys_clock(struct ambarella_iav *iav)
{
	u32 chip_id, hz, hz_target;
	u32 delta = 100;

	chip_id = get_chip_id(iav);

	hz = (u32)clk_get_rate(clk_get(NULL, "gclk_cortex"));
	hz_target = G_system_freq[chip_id].cortex_MHz * 1000 * 1000;
	if (hz > hz_target + delta) {
		iav_error("Cortex clock: target [%d], real [%d].\n",
			hz_target, hz);
		return -1;
	} else if (hz < hz_target - delta) {
		iav_warn("Cortex clock real [%d] lower than target [%d].\n",
			hz, hz_target);
	}

	hz = (u32)clk_get_rate(clk_get(NULL, "gclk_idsp"));
	hz_target = G_system_freq[chip_id].idsp_MHz * 1000 * 1000;
	if (hz > hz_target + delta) {
		iav_error("IDSP clock: target [%d], real [%d].\n", hz_target, hz);
		return -1;
	} else if (hz < hz_target - delta) {
		iav_warn("IDSP clock real [%d] lower than target [%d].\n",
			hz, hz_target);
	}

	hz = (u32)clk_get_rate(clk_get(NULL, "gclk_core"));
	hz_target = G_system_freq[chip_id].core_MHz * 1000 * 1000;
	if (hz > hz_target + delta) {
		iav_error("DSP Core clock: target [%d], real [%d].\n", hz_target, hz);
		return -1;
	} else if (hz < hz_target - delta) {
		iav_warn("DSP Core clock real [%d] lower than target [%d].\n",
			hz, hz_target);
	}

	hz = (u32)clk_get_rate(clk_get(NULL, "gclk_ddr"));
	hz_target = G_system_freq[chip_id].dram_MHz * 1000 * 1000;
	if (hz > hz_target + delta) {
		iav_error("DRAM clock: target [%d], real [%d].\n", hz_target, hz);
		return -1;
	} else if (hz < hz_target - delta) {
		iav_warn("DRAM clock real [%d] lower than target [%d].\n",
			hz, hz_target);
	}

	hz = (u32)clk_get_rate(clk_get(NULL, "gclk_audio"));
	hz_target = AUDIO_CLK_KHZ * 1000;
	if (hz > hz_target + delta || hz < hz_target - delta) {
		iav_error("Audio clock [%d] must be around [%d].\n", hz, hz_target);
		return -1;
	}

	return 0;
}

