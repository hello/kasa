/*
 * iav_enc_warp.c
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

struct iav_aspect_ratio {
	u32 num;
	u32 den;
};

static struct iav_warp_main G_user_warp_main;
static struct iav_warp_main G_default_warp_main;
static struct iav_aspect_ratio G_warp_ar[IAV_SRCBUF_NUM][MAX_NUM_WARP_AREAS];

static inline int is_user_multi_warp_enable(struct ambarella_iav *iav)
{
	return iav->encode_mode == DSP_MULTI_REGION_WARP_MODE
		&& iav->warp == &G_user_warp_main;
}

static int check_warp_state(struct ambarella_iav* iav)
{
	if (!is_enc_work_state(iav)) {
		iav_error("Cannot cfg/get/apply warp while IAV is not in PREVIEW nor ENCODE.\n");
		return -1;
	}
	return 0;
}

static int check_warp_addr(struct ambarella_iav* iav,
	struct iav_warp_main *param)
{
	int enc_mode = iav->encode_mode;
	int lens_warp_enable = iav->system_config[enc_mode].lens_warp;
	int i = 0;

	if (lens_warp_enable) {
		// Lens distortion correction
		if ((param->area[0].h_map.data_addr_offset +
					MAX_WARP_TABLE_SIZE_LDC * sizeof(s16)) >= WARP_VECT_PART_SIZE ||
				(param->area[0].v_map.data_addr_offset +
					MAX_WARP_TABLE_SIZE_LDC * sizeof(s16)) >= WARP_VECT_PART_SIZE ||
				(param->area[0].me1_v_map.data_addr_offset +
					MAX_WARP_TABLE_SIZE_LDC * sizeof(s16)) >= WARP_VECT_PART_SIZE) {
			iav_error("Warp Addr Overflows In LDC.\n");
			return -EINVAL;
		}
	} else {
		// Multi-region warp
		for (i = 0; i < MAX_NUM_WARP_AREAS; i++) {
			if ((param->area[i].h_map.data_addr_offset +
						MAX_WARP_TABLE_SIZE * sizeof(s16)) >= WARP_VECT_PART_SIZE ||
					(param->area[i].v_map.data_addr_offset +
						MAX_WARP_TABLE_SIZE * sizeof(s16)) >= WARP_VECT_PART_SIZE) {
				iav_error("Area [%d] Warp Addr Overflows.\n", i);
				return -EINVAL;
			}
		}
	}

	return 0;
}

static int check_warp_main_state(struct ambarella_iav *iav)
{
	int enc_mode = iav->encode_mode;
	int lens_warp_enable = iav->system_config[enc_mode].lens_warp;

	if (!lens_warp_enable && enc_mode != DSP_MULTI_REGION_WARP_MODE) {
		iav_error("Please enable lens warp or enter 'warp mode'.\n");

		return -1;
	}
	return 0;
}

/******************************************************************************
 * Lens Warp
 ******************************************************************************/

static int check_lens_warp_param(struct ambarella_iav *iav,
	struct iav_warp_area *lens_warp)
{
	struct iav_rect *vin;
	int rval = 0;
	get_vin_win(iav, &vin, 0);

	if ((lens_warp->input.x + lens_warp->input.width > vin->width) ||
		(lens_warp->input.y + lens_warp->input.height > vin->height)) {
		iav_error("Input window exceeds VIN!\n");
		rval = -1;
	}
	return rval;
}

static int cfg_lens_warp_param(struct ambarella_iav *iav,
	struct iav_warp_area *lens_warp)
{
	if (check_lens_warp_param(iav, lens_warp) < 0) {
		return -1;
	}
	G_user_warp_main.area[0] = *lens_warp;
	iav->warp = &G_user_warp_main;
	return 0;
}

/******************************************************************************
 * Warp DPTZ
 ******************************************************************************/

static int check_warp_dptz_state(struct ambarella_iav *iav)
{
	int enc_mode = iav->encode_mode;
	int lens_warp_enable = iav->system_config[enc_mode].lens_warp;
	if (enc_mode != DSP_MULTI_REGION_WARP_MODE) {
		iav_error("Cannot set warp dptz in mode %d.\n", enc_mode);
		return -1;
	}
	if (lens_warp_enable) {
		iav_error("Cannot set warp dptz when lens warp is enabled.\n");
		return -1;
	}
	return 0;
}

static int check_warp_dptz_id(int buf_id)
{
	if (buf_id < IAV_SUB_SRCBUF_FIRST || buf_id >= IAV_SUB_SRCBUF_LAST) {
		iav_error("Invalid buffer id [%d] in warp dptz parameter.\n", buf_id);
		return -1;
	}

	return 0;
}

static int check_warp_dptz_param(struct ambarella_iav *iav, struct iav_warp_dptz *dptz)
{
	iav_no_check();

	if (check_warp_dptz_id(dptz->buf_id) < 0) {
		return -1;
	}
	return 0;
}

static void get_buffer_warp_win(struct ambarella_iav *iav,
	struct iav_buffer *buffer, struct iav_window *buf_win)
{
	struct amba_video_info *vout_info = NULL;
	struct amba_video_info *vout0_info = NULL;
	struct amba_video_info *vout1_info = NULL;
	u8 vout_swap = iav->system_config[iav->encode_mode].vout_swap;

	memset(buf_win, 0, sizeof(struct iav_window));

	if (vout_swap) {
		vout0_info = &G_voutinfo[1].video_info;
		vout1_info = &G_voutinfo[0].video_info;
	} else {
		vout0_info = &G_voutinfo[0].video_info;
		vout1_info = &G_voutinfo[1].video_info;
	}

	switch (buffer->type) {
		case IAV_SRCBUF_TYPE_ENCODE:
			*buf_win = buffer->win;
			break;
		case IAV_SRCBUF_TYPE_PREVIEW:
			if (buffer->id == IAV_SRCBUF_PB) {
				vout_info = vout1_info;
			} else if (buffer->id == IAV_SRCBUF_PA) {
				vout_info = vout0_info;
			}
			if (vout_info) {
				if (!vout_info->rotate) {
					buf_win->width = vout_info->width;
					buf_win->height = vout_info->height;
				} else {
					buf_win->width = vout_info->height;
					buf_win->height = vout_info->width;
				}
			}
			break;
		case IAV_SRCBUF_TYPE_OFF:
		default:
			break;
	}
}

static inline void calc_default_warp_dptz(struct iav_rect* input,
	struct iav_rect* output, struct iav_warp_area *area,
	struct iav_window *buf_win, struct iav_window *main_win)
{
	if (!area->enable) {
		memset(input, 0, sizeof(struct iav_rect));
		memset(output, 0, sizeof(struct iav_rect));
	} else {
		input->width = area->output.width;
		input->height = area->output.height;
		input->x = input->y = 0;

		output->x = ALIGN(area->output.x * buf_win->width / main_win->width, 2);
		if (output->x > buf_win->width) {
			output->x = buf_win->width;
		}
		output->width = ALIGN(area->output.width * buf_win->width
		    / main_win->width, 4);
		if (output->x + output->width > buf_win->width) {
			output->width = buf_win->width - output->x;
		}

		output->y = ALIGN(area->output.y * buf_win->height / main_win->height, 4);
		if (output->y > buf_win->height) {
			output->y = buf_win->height;
		}
		output->height = ALIGN(area->output.height * buf_win->height
		    / main_win->height, 4);
		if (output->y + output->height > buf_win->height) {
			output->height = buf_win->height - output->y;
		}

		if ((output->width == 0) || (output->height == 0)) {
			memset(input, 0, sizeof(struct iav_rect));
			memset(output, 0, sizeof(struct iav_rect));
		}
	}
}

static void cfg_default_warp_dptz(struct ambarella_iav *iav, u32 buf_map)
{
	int i, j, is_buf_unwarp, is_buf_off;
	struct iav_warp_dptz *dptz;
	struct iav_warp_area *area;
	struct iav_window buf_win;
	struct iav_window *main_win = &iav->main_buffer->win;

	for (i = IAV_SUB_SRCBUF_FIRST; i < IAV_SUB_SRCBUF_LAST; i++) {
		if ((buf_map & (1 << i)) && !iav->warp->keep_dptz[i]) {
			dptz = &iav->warp_dptz[i];
			is_buf_unwarp = iav->srcbuf[i].unwarp;
			is_buf_off = (iav->srcbuf[i].type == IAV_SRCBUF_TYPE_OFF);
			get_buffer_warp_win(iav, &iav->srcbuf[i], &buf_win);

			for (j = 0; j < MAX_NUM_WARP_AREAS; j++) {
				area = &iav->warp->area[j];
				if (area->enable && !is_buf_off && !is_buf_unwarp ) {
					calc_default_warp_dptz(&dptz->input[j], &dptz->output[j],
						area, &buf_win, main_win);
				} else {
					memset(&dptz->input[j], 0, sizeof(dptz->input[j]));
					memset(&dptz->output[j], 0, sizeof(dptz->output[j]));
				}
			}
		}
	}
}

static int get_warp_dptz_param(struct ambarella_iav *iav,
	struct iav_warp_dptz* param)
{
	int buf_id, i;

	memset(&param->input, 0, sizeof(param->input));
	memset(&param->output, 0, sizeof(param->output));

	buf_id = param->buf_id;
	if (check_warp_dptz_state(iav) < 0) {
		return -EPERM;
	}
	if (check_warp_dptz_id(buf_id) < 0) {
		return -EINVAL;
	}
	if (is_user_multi_warp_enable(iav)) {
		for (i = 0; i < MAX_NUM_WARP_AREAS; i++) {
			if (iav->warp->area[i].enable) {
				param->input[i] = iav->warp_dptz[buf_id].input[i];
				param->output[i] = iav->warp_dptz[buf_id].output[i];
			}
		}
	}

	return 0;
}

static int cfg_warp_dptz_param(struct ambarella_iav *iav,
	struct iav_warp_dptz* param)
{
	if (check_warp_dptz_state(iav) < 0) {
		return -EPERM;
	}
	if (check_warp_dptz_param(iav, param) < 0) {
		return -EINVAL;
	}
	iav->warp_dptz[param->buf_id] = *param;

	return 0;
}

/******************************************************************************
 * Multi Warp
 ******************************************************************************/
static int apply_multi_warp_ctrl(struct ambarella_iav *iav,
	struct amb_dsp_cmd *cmd)
{
	int active_id, i, j;
	int ret = 0;

	iav->curr_warp_index = iav->next_warp_index + UWARP_NUM;

	for (i = 0; i < MAX_NUM_WARP_AREAS; i++) {
		if (iav->warp->area[i].enable) {
			for (j = IAV_SUB_SRCBUF_FIRST; j < IAV_SUB_SRCBUF_LAST; j++) {
				if (!iav->srcbuf[j].unwarp) {
					update_warp_aspect_ratio(j, i, &iav->warp_dptz[j].input[i],
					    &iav->warp_dptz[j].output[i]);
				}
			}
		}
	}

	for (i = 0, active_id = 0; i < MAX_NUM_WARP_AREAS; i++) {
		if (iav->warp->area[i].enable) {
			cmd_set_region_warp_control(iav, i, active_id);
			++active_id;
		}
	}

	if (active_id > 0) {
		ret = cmd_set_warp_batch_cmds(iav, active_id, cmd);
	}

	//update warp index
	iav->next_warp_index = (iav->next_warp_index + 1) % IAV_PARTITION_TOGGLE_NUM;
	return ret;

}

static int prepare_default_warp_param(struct ambarella_iav *iav)
{
	int i, area_num;
	u16 input_w, output_w, input_h, output_h;
	struct iav_warp_area *area;
	struct iav_buffer *premain_buffer = &iav->srcbuf[IAV_SRCBUF_PMN];
	struct iav_system_config *config = &iav->system_config[DSP_MULTI_REGION_WARP_MODE];
	int max_input_width = config->max_warp_input_width;
	int max_output_width = config->max_warp_output_width;

	for (area_num = 1; area_num <= MAX_NUM_WARP_AREAS; area_num++) {
		input_w = premain_buffer->win.width / area_num;
		output_w = iav->main_buffer->win.width / area_num;
		if ((input_w <= max_input_width) && (output_w <= max_output_width)) {
			break;
		}
	}
	if (area_num > MAX_NUM_WARP_AREAS) {
		iav_error("Failed to prepare default warp parameters. "
			"Pre-main width [%d] / main buffer width [%d] is too large or "
			"max warp input width [%d] / output width [%d] is too small.\n",
			premain_buffer->win.width, iav->main_buffer->win.width,
			max_input_width, max_output_width);
		return -1;
	}
	memset(&G_default_warp_main, 0, sizeof(G_default_warp_main));
	input_h = premain_buffer->win.height;
	output_h = iav->main_buffer->win.height;
	for (i = 0; i < area_num; i++) {
		area = &G_default_warp_main.area[i];
		area->enable = 1;
		area->input.width = input_w;
		area->input.height = input_h;
		area->input.x = i * input_w;

		area->intermediate.width = ALIGN(input_w / 4, PIXEL_IN_MB) * 4;
		area->intermediate.height = ALIGN((input_h >= output_h ? input_h : output_h) /
			4, PIXEL_IN_MB) * 4;
		area->intermediate.x = i * area->intermediate.width;
		if ((area->intermediate.x + area->intermediate.width >
			config->v_warped_main_max_width) ||
			(area->intermediate.height > config->v_warped_main_max_height)) {
			iav_error("Insufficient Intermediate buffer. \n");
			return -1;
		}

		area->output.width = output_w;
		area->output.height = output_h;
		area->output.x = i * output_w;
	}
	return 0;
}


static int check_multi_warp_param(struct ambarella_iav *iav,
	struct iav_warp_main *warp_areas)
{
	int i = 0;
	struct iav_warp_area *area;

	iav_no_check();

	for (i = 0; i < MAX_NUM_WARP_AREAS; i++) {
		area = &warp_areas->area[i];
		if ((area->input.x & 0x01) || (area->input.y & 0x03)) {
			iav_error("Warp region [%d] input x/y [%d/%d] must be "
				"multiple of 2 and 4 for each.\n", i, area->input.x, area->input.y);
			return -1;
		}
		if ((area->intermediate.x & 0x01) || (area->intermediate.y & 0x03)) {
			iav_error("Warp region [%d] intermediate x/y [%d/%d] must be "
				"multiple of 2 and 4 for each.\n", i, area->intermediate.x, area->intermediate.y);
			return -1;
		}
		if ((area->output.x & 0x01) || (area->output.y & 0x03)) {
			iav_error("Warp region [%d] input x/y [%d/%d] must be "
				"multiple of 2 and 4 for each.\n", i, area->output.x, area->output.y);
			return -1;
		}
		if ((area->input.width & 0x0F) || (area->input.height & 0x07)) {
			iav_error("Warp region [%d] input w/h [%d/%d] must be "
				"multiple of 16 and 8 for each.\n", i, area->input.width, area->input.height);
			return -1;
		}
		if ((area->intermediate.width & 0x3F) || (area->intermediate.height & 0x0F)) {
			iav_error("Warp region [%d] intermediate w/h [%d/%d] must be "
				"multiple of 64 and 16 for each.\n", i, area->intermediate.width,
				area->intermediate.height);
			return -1;
		}
		if ((area->output.width & 0x07) || (area->output.height & 0x07)) {
			iav_error("Warp region [%d] output w/h [%d/%d] must be "
				"multiple of 8.\n", i, area->output.width, area->output.height);
			return -1;
		}
	}

	return 0;
}

static int cfg_user_multi_warp(struct ambarella_iav *iav,
	struct iav_warp_main *user_param)
{
	if (check_multi_warp_param(iav, user_param) < 0) {
		return -1;
	}
	G_user_warp_main = *user_param;
	iav->warp = &G_user_warp_main;

	return 0;
}

static int cfg_multi_warp_param(struct ambarella_iav *iav,
	struct iav_warp_main *user_warp)
{
	int i;
	int enabled_user_areas = 0;
	u32 buf_map = 0;

	for (i = 0; i < MAX_NUM_WARP_AREAS; i++) {
		enabled_user_areas += (user_warp->area[i].enable != 0);
	}

	if (enabled_user_areas > 0) {
		if (cfg_user_multi_warp(iav, user_warp) < 0) {
			return -1;
		}
	} else {
		if (cfg_default_multi_warp(iav) < 0) {
			return -1;
		}
	}
	// Reset warp dptz
	for (i = IAV_SUB_SRCBUF_FIRST; i < IAV_SUB_SRCBUF_LAST; i++) {
		buf_map |= (1 << i);
	}
	cfg_default_warp_dptz(iav, buf_map);
	return 0;
}

static int get_warp_main_param(struct ambarella_iav *iav,
	struct iav_warp_main* param)
{
	int enc_mode = iav->encode_mode;
	int lens_warp_enable = iav->system_config[enc_mode].lens_warp;
	struct iav_warp_main *warp = iav->warp;
	struct iav_window *main_win = &iav->main_buffer->win;
	memset(param, 0, sizeof(struct iav_warp_main));

	if (check_warp_state(iav) < 0) {
		return -EPERM;
	}
	if (lens_warp_enable) {
		param->area[0].enable = warp->area[0].enable;
		param->area[0].rotate_flip = 0;
		param->area[0].h_map = warp->area[0].h_map;
		param->area[0].v_map = warp->area[0].v_map;
		param->area[0].input = warp->area[0].input;
		// warp output = main size
		param->area[0].output.x = 0;
		param->area[0].output.y = 0;
		param->area[0].output.width = main_win->width;
		param->area[0].output.height = main_win->height;
	} else if (is_user_multi_warp_enable(iav)) {
		*param = *warp;
	}

	return 0;
}

static int check_min_warp_input_width(struct ambarella_iav *iav,
	struct iav_warp_main* warp_areas)
{
	int i = 0;
	struct iav_warp_area *area;

	iav_no_check();

	for (i = 0; i < MAX_NUM_WARP_AREAS; i++) {
		area = &warp_areas->area[i];
		if (area->enable && (area->input.width < MIN_WARP_INPUT_WIDTH)) {
			iav_error("warp area input width can not be less than %d!\n",
				MIN_WARP_INPUT_WIDTH);
			return -1;
		}
	}

	return 0;
}

static int cfg_warp_main_param(struct ambarella_iav *iav,
	struct iav_warp_main* param)
{
	int enc_mode = iav->encode_mode;
	int lens_warp_enable = iav->system_config[enc_mode].lens_warp;

	if (check_warp_main_state(iav) < 0) {
		return -EPERM;
	}

	if (check_warp_addr(iav, param) < 0) {
		return -EINVAL;
	}

	if (lens_warp_enable) {
		// Lens distortion correction by single pass warp control
		if (cfg_lens_warp_param(iav, &param->area[0]) < 0) {
			return -EINVAL;
		}
	} else {
		if (iav_check_warp_idsp_perf(iav, param) < 0) {
			return -EPERM;
		}
		if (check_min_warp_input_width(iav, param) < 0) {
			return -EINVAL;
		}
		// Multi-region warp control
		if (cfg_multi_warp_param(iav, param) < 0) {
			return -EINVAL;
		}
	}

	return 0;
}


/******************************************************************************
 * External APIs
 ******************************************************************************/
void update_warp_aspect_ratio(int buf_id, int area_id,
	struct iav_rect* input, struct iav_rect* output)
{
	int gcd, num, den;
	if (!input->width || !input->height) {
		num = 1;
		den = 1;
	} else {
		num = input->width;
		den = input->height;
	}
	if (output->width && output->height) {
		num *= output->height;
		den *= output->width;
	}
	if (num && den) {
		gcd = get_gcd(num, den);
		G_warp_ar[buf_id][area_id].num = num / gcd;
		G_warp_ar[buf_id][area_id].den = den / gcd;
	} else {
		G_warp_ar[buf_id][area_id].num = G_warp_ar[buf_id][area_id].den = 0;
	}
}

int cfg_default_multi_warp(struct ambarella_iav *iav)
{
	if (prepare_default_warp_param(iav) < 0) {
		return -1;
	}
	if (check_multi_warp_param(iav, &G_default_warp_main) < 0) {
		return -1;
	}

	iav->warp = &G_default_warp_main;
	return 0;
}

int clear_default_warp_dptz(struct ambarella_iav *iav, u32 buf_map)
{
	int i;
	struct iav_warp_dptz *dptz;

	for (i = IAV_SUB_SRCBUF_FIRST; i < IAV_SUB_SRCBUF_LAST; i++) {
		if (buf_map & (1 << i)) {
			dptz = &iav->warp_dptz[i];
			memset(dptz->output, 0, sizeof(dptz->output));
			memset(dptz->input, 0, sizeof(dptz->input));
		}
	}
	return apply_multi_warp_ctrl(iav, NULL);
}

int set_default_warp_dptz(struct ambarella_iav *iav, u32 buf_map,
	struct amb_dsp_cmd *cmd)
{
	cfg_default_warp_dptz(iav, buf_map);
	return apply_multi_warp_ctrl(iav, cmd);
}

void get_aspect_ratio_in_warp_mode(struct iav_stream *stream,
	u8 * aspect_ratio_idc, u16 * sar_width, u16 * sar_height)
{
	int i, num, den, gcd;
	struct ambarella_iav *iav = stream->iav;
	struct iav_rect *enc_win = &stream->format.enc_win;
	struct iav_rect *output;
	int buf_id = stream->srcbuf->id;

	num = den = 1;
	for (i = 0; i < MAX_NUM_WARP_AREAS; ++i) {
		 output = (buf_id == IAV_SRCBUF_MN ? &iav->warp->area[i].output :
			 &iav->warp_dptz[buf_id].output[i]);
		 if ((enc_win->x >= output->x)
			 && (enc_win->x <= output->x + output->width)
			 && (enc_win->y >= output->y)
			 && (enc_win->y <= output->y + output->height)) {
			 num = G_warp_ar[buf_id][i].num;
			 den = G_warp_ar[buf_id][i].den;
			 break;
		 }
	}
	num *= *sar_width;
	den *= *sar_height;

	if (num == den) {
		*aspect_ratio_idc = ENCODE_ASPECT_RATIO_1_1_SQUARE_PIXEL;
		*sar_width = 1;
		*sar_height = 1;
	} else {
		*aspect_ratio_idc = ENCODE_ASPECT_RATIO_CUSTOM;
		gcd = get_gcd(num, den);
		*sar_width = num / gcd;
		*sar_height = den / gcd;
	}
}


void iav_init_warp(struct ambarella_iav *iav)
{
	int buf_id, area_id;
	struct iav_aspect_ratio *ar;
	struct iav_warp_dptz *dptz;
	struct iav_system_config *config;

	memset(&G_default_warp_main, 0, sizeof(G_default_warp_main));
	memset(&G_user_warp_main, 0, sizeof(G_user_warp_main));
	memset(&iav->warp_dptz, 0, sizeof(iav->warp_dptz));
	iav->warp = &G_default_warp_main;
	config = &iav->system_config[DSP_MULTI_REGION_WARP_MODE];
	config->max_warp_input_width = MAX_PRE_WIDTH_IN_WARP;
	config->max_warp_output_width = MAX_PRE_WIDTH_IN_WARP;

	for (buf_id = IAV_SRCBUF_FIRST; buf_id < IAV_SRCBUF_LAST_PMN; buf_id++) {
		dptz = &iav->warp_dptz[buf_id];
		dptz->buf_id = buf_id;
		for (area_id = 0; area_id < MAX_NUM_WARP_AREAS; area_id++) {
			ar = &G_warp_ar[buf_id][area_id];
			ar->num = 1;
			ar->den = 1;
		}
	}
}

/******************************************************************************
 * IAV IOCTLs
 ******************************************************************************/

int iav_ioc_g_warp_ctrl(struct ambarella_iav* iav, void __user *arg)
{
	int rval = 0;
	struct iav_warp_ctrl warp_ctrl;

	if (copy_from_user(&warp_ctrl, arg, sizeof(warp_ctrl)))
		return -EFAULT;

	mutex_lock(&iav->iav_mutex);

	do {
		if (check_warp_state(iav) < 0) {
			rval = -EPERM;
			break;
		}

		switch (warp_ctrl.cid) {
			case IAV_WARP_CTRL_MAIN:
				rval = get_warp_main_param(iav, &warp_ctrl.arg.main);
				break;
			case IAV_WARP_CTRL_DPTZ:
				rval = get_warp_dptz_param(iav, &warp_ctrl.arg.dptz);
				break;
			default:
				iav_error("Unknown warp cid: %d\n", warp_ctrl.cid);
				rval = -EINVAL;
				break;
		}
	} while (0);

	mutex_unlock(&iav->iav_mutex);

	if (!rval) {
		rval = copy_to_user(arg, &warp_ctrl, sizeof(warp_ctrl));
	}

	return rval;
}

int iav_ioc_c_warp_ctrl(struct ambarella_iav* iav, void __user *arg)
{
	int rval = 0;
	struct iav_warp_ctrl warp_ctrl;

	if (copy_from_user(&warp_ctrl, arg, sizeof(warp_ctrl)))
		return -EFAULT;

	mutex_lock(&iav->iav_mutex);

	do {
		if (check_warp_state(iav) < 0) {
			rval = -EPERM;
			break;
		}

		switch (warp_ctrl.cid) {
			case IAV_WARP_CTRL_MAIN:
				rval = cfg_warp_main_param(iav, &warp_ctrl.arg.main);
				break;
			case IAV_WARP_CTRL_DPTZ:
				rval = cfg_warp_dptz_param(iav, &warp_ctrl.arg.dptz);
				break;
			default:
				iav_error("Unknown warp cid: %d\n", warp_ctrl.cid);
				rval = -EINVAL;
				break;
		}
	} while (0);

	mutex_unlock(&iav->iav_mutex);
	return rval;
}

int iav_ioc_a_warp_ctrl(struct ambarella_iav* iav, void __user *arg)
{
	int rval = 0;
	int enc_mode = iav->encode_mode;
	int lens_warp_enable = iav->system_config[enc_mode].lens_warp;
	u32 flags;
	u32 user_warp_virt, dsp_warp_virt;
	u32 user_warp_phys, dsp_warp_phys;
	struct iav_gdma_param gparam;
	struct iav_rect *vin;

	if (copy_from_user(&flags, arg, sizeof(flags)))
		return -EFAULT;

	mutex_lock(&iav->iav_mutex);

	do {
		if (check_warp_state(iav) < 0) {
			rval = -EPERM;
			break;
		}
		user_warp_virt = iav->mmap[IAV_BUFFER_WARP].virt;
		dsp_warp_virt = iav->mmap[IAV_BUFFER_WARP].virt + \
			(iav->next_warp_index + UWARP_NUM) * WARP_VECT_PART_SIZE;
		user_warp_phys = iav->mmap[IAV_BUFFER_WARP].phys;
		dsp_warp_phys = iav->mmap[IAV_BUFFER_WARP].phys +
			(iav->next_warp_index + UWARP_NUM) * WARP_VECT_PART_SIZE;

		// Use GDMA copy
		memset(&gparam, 0 , sizeof(gparam));
		gparam.dest_phys_addr = dsp_warp_phys;
		gparam.dest_virt_addr = dsp_warp_virt;
		gparam.src_phys_addr = user_warp_phys;
		gparam.src_virt_addr = user_warp_virt;
		gparam.src_pitch = KByte(1);
		gparam.dest_pitch = KByte(1);
		gparam.width = KByte(1);
		gparam.height = WARP_VECT_PART_SIZE / KByte(1);

		/* Fix me, the mmap type is non cached in iav_create_mmap_table */
		gparam.src_non_cached = 1;
		gparam.dest_non_cached = 1;

		if (dma_pitch_memcpy(&gparam) < 0) {
			iav_error("dma_pitch_memcpy error\n");
			rval = -EFAULT;
			break;
		}
		// End of GDMA copy

		if (lens_warp_enable) {
			// Apdate Aspect Ratio
			if (iav->warp->area[0].enable) {
				update_warp_aspect_ratio(IAV_SRCBUF_MN, 0,
					&iav->warp->area[0].input, &iav->warp->area[0].output);
			} else {
				get_vin_win(iav, &vin, 0);
				update_warp_aspect_ratio(IAV_SRCBUF_MN, 0, vin,
					(struct iav_rect *)&iav->srcbuf[IAV_SRCBUF_MN].win);
			}

			rval = cmd_set_warp_control(iav, NULL);
		} else {
			if ((flags & (1 << IAV_WARP_CTRL_MAIN))
			    || (flags & (1 << IAV_WARP_CTRL_DPTZ))) {
				rval = apply_multi_warp_ctrl(iav, NULL);
			}
		}
	} while (0);

	mutex_unlock(&iav->iav_mutex);

	return rval;
}

