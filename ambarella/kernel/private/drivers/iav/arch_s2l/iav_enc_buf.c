/*
 * iav_enc_buf.c
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

static inline int is_srcbuf(u32 buf_id)
{
	return ((buf_id >= IAV_SRCBUF_FIRST) && (buf_id < IAV_SRCBUF_LAST));
}

static inline int is_sub_srcbuf(u32 buf_id)
{
	return ((buf_id >= IAV_SUB_SRCBUF_FIRST) && (buf_id < IAV_SUB_SRCBUF_LAST));
}

static inline int check_zoom_limit(u32 buf_id, struct iav_rect *in,
	struct iav_window *out, char *str)
{
	u32 zm_in = G_buffer_limit[buf_id].max_zoomin_factor;
	u32 zm_out = G_buffer_limit[buf_id].max_zoomout_factor;

	if ((out->width * zm_out <= in->width) || (out->height * zm_out <= in->height)) {
		iav_error("%s downscaled from %dx%d to %dx%d, cannot be larger or "
			"equal to the max zoom out factor %d.\n", str, in->width, in->height,
			out->width, out->height, zm_out);
		return -1;
	}
	if ((in->width * zm_in < out->width) || (in->height * zm_in < out->height)) {
		iav_error("%s upscaled from %dx%d to %dx%d, out of the max zoom in "
			"factor %d.\n", str, in->width, in->height,
			out->width, out->height, zm_in);
		return -1;
	}

	return 0;
}

static int check_src_buf_type(u32 buf_id, enum iav_srcbuf_type t)
{
	switch (buf_id) {
	case IAV_SRCBUF_MN:
		if (t != IAV_SRCBUF_TYPE_ENCODE) {
			iav_error("Source buffer [0] type cannot be OFF, PREVIEW or VCA.\n");
			return -1;
		}
		break;
	case IAV_SRCBUF_PC:
		if (t == IAV_SRCBUF_TYPE_PREVIEW) {
			iav_error("Source buffer [1] type cannot be PREVIEW.\n");
			return -1;
		}
		break;
	default:
		break;
	}
	return 0;
}

static int check_main_buffer_size(struct ambarella_iav *iav,
	u32 is_pre_main, struct iav_window *size)
{
	char str[32];
	u32 max_w, max_h;
	struct iav_window *orig, *limit;

	sprintf(str, "%s", is_pre_main ? "Pre-main buffer" : "Source buffer [0]");

	/* error if main buffer is 0x0 */
	if (!size->width || !size->height) {
		iav_error("%s size %dx%d cannot be zero.\n",
			str, size->width, size->height);
		return -1;
	}

	/* error if it's not aligned */
	if ((size->width & 0xF) || (size->height & 0x7)) {
		iav_error("%s width %d must be multiple of 16 and height %d must be "
			"multiple of 8.\n", str, size->width, size->height);
		return -1;
	}

	if (is_pre_main) {
		/* error if pre main size > limit */
		max_w = MAX_PRE_WIDTH_IN_WARP;
		max_h = MAX_PRE_HEIGHT_IN_WARP;
		limit = &G_buffer_limit[IAV_SRCBUF_PMN].max_win;
		max_w = MIN(max_w, limit->width);
		max_h = MIN(max_h, limit->height);
		if ((size->width > max_w) || (size->height > max_h)) {
			iav_error("%s %dx%d is larger than limit %dx%d in mode %d.\n", str,
				size->width, size->height, max_w, max_h, iav->encode_mode);
			return -1;
		}
	}

	/* error if IAV is not IDLE or source buffer is busy */
	orig = is_pre_main ? &iav->srcbuf[IAV_SRCBUF_PMN].win :
		&iav->srcbuf[IAV_SRCBUF_MN].win;
	if ((size->width != orig->width) || (size->height != orig->height)) {
		if (iav->state != IAV_STATE_IDLE) {
			iav_error("Cannot change %s size while IAV is not IDLE.\n", str);
			return -1;
		}
		if (iav->srcbuf[IAV_SRCBUF_MN].state == IAV_SRCBUF_STATE_BUSY) {
			iav_error("Cannot change %s size while it's used.\n", str);
			return -1;
		}
	}

	return 0;
}

int check_main_buffer_input(u32 is_pre_main,
	struct iav_rect * input, struct iav_rect * vin)
{
	char str[32];

	/* default input window is VIN */
	if (!input->width) {
		input->width = vin->width;
		input->x = 0;
	}
	if (!input->height) {
		input->height = vin->height;
		input->y = 0;
	}

	sprintf(str, "%s", is_pre_main ? "Pre-main buffer" : "Source buffer [0]");

	/* error if main input is not aligned */
	if ((input->y & 0x1) || (input->x & 0x1)) {
		iav_error("%s input offset %dx%d is not multiple of 2.\n",
			str, input->x, input->y);
		return -1;
	}
	if ((input->height & 0x1) || (input->width & 0x1)) {
		iav_error("%s input width %d and height %d is not multiple of 2.\n",
			str, input->width, input->height);
		return -1;
	}

	/* error if main input > VIN */
	if ((input->width + input->x > vin->width) ||
		(input->height + input->y > vin->height)) {
		iav_error("%s input %dx%d + offset %dx%d cannot be larger than VIN "
			"%dx%d.\n", str, input->width, input->height, input->x, input->y,
			vin->width, vin->height);
		return -1;
	}

	// TODO: move pre-main input check to DZ type I

	return 0;
}

static int check_main_buffer_zoom(u32 is_pre_main,
	struct iav_rect * input, struct iav_window * main)
{
	char msg[32];

	if (!input->width || !input->height) {
		iav_error("Source buffer [0] input window %dx%d is invalid!\n",
			input->width, input->height);
		return -1;
	}
	sprintf(msg, "%s", is_pre_main ? "Pre main buffer" : "Source buffer [0]");
	if (check_zoom_limit(IAV_SRCBUF_MN, input, main, msg) < 0) {
		return -1;
	}

	// TODO: No V-upscale for main input in dewarp mode

	return 0;
}

static int check_sub_buf_size(struct ambarella_iav *iav,
	u32 id, struct iav_window *size)
{
	char str[32];
	struct iav_window *win;

	if (size->width && size->height) {
		sprintf(str, "Source buffer [%d]", id);

		/* error it's not aligned */
		if ((size->width & 0xF) || (size->height & 0x7)) {
			iav_error("%s width %d must be multiple of 16 and height %d must "
				"be multiple of 8.\n", str, size->width, size->height);
			return -1;
		}
		/* error if size < min limit */
		win = &G_encode_limit[iav->encode_mode].min_main;
		if ((size->width < win->width) || (size->height < win->height)) {
			iav_error("%s %dx%d cannot be smaller than limit %dx%d in mode "
				"%d.\n", str, size->width, size->height, win->width,
				win->height, iav->encode_mode);
			return -1;
		}
		/* error if size > buffer max */
		win = &iav->srcbuf[id].max;
		if ((size->width > win->width) || (size->height > win->height)) {
			iav_error("%s %dx%d cannot be larger than max %dx%d.\n", str,
				size->width, size->height, win->width, win->height);
			return -1;
		}
		/* error if buffer is in use */
		win = &iav->srcbuf[id].win;
		if ((size->width != win->width) || (size->height != win->height)) {
			if (iav->srcbuf[id].state == IAV_SRCBUF_STATE_BUSY) {
				iav_error("%s cannot change size while it's used.\n", str);
				return -1;
			}
		}

	}

	return 0;
}

static int check_sub_buf_input(u32 id, struct iav_rect * input,
	struct iav_window * main)
{
	/* default sub input = main */
	if (!input->width) {
		input->width = main->width;
		input->x = 0;
	}
	if (!input->height) {
		input->height = main->height;
		input->y = 0;
	}

	/* error if it's not aligned */
	if ((input->width & 0x1) || (input->x & 0x1)) {
		iav_error("Source buffer [%d] input width %d and offset x %d must be "
			"multiple of 2.\n", id, input->width, input->x);
		return -1;
	}
	if ((input->height & 0x3) || (input->y & 0x3)) {
		iav_error("Source buffer [%d] input height %d and offset y %d must be "
			"multiple of 4.\n", id, input->height, input->y);
		return -1;
	}

	/* error if input > main */
	if ((input->width + input->x > main->width) ||
		(input->height + input->y > main->height)) {
		iav_error("Source buffer [%d] input %dx%d + offset %dx%d is out of "
			"main %dx%d.\n", id, input->width, input->height, input->x,
			input->y, main->width, main->height);
		return -1;
	}

	return 0;
}

static int check_sub_buf_zoom(struct ambarella_iav *iav, u32 id, u8 unwarp,
	enum iav_srcbuf_type type, struct iav_window * win, struct iav_rect *input)
{
	char str[32];
	struct iav_window output;
	u8 vout_swap = iav->system_config[iav->encode_mode].vout_swap;
	struct amba_video_info *vout0_info = NULL, *vout1_info = NULL;

	if (type == IAV_SRCBUF_TYPE_OFF) {
		return 0;
	}

	sprintf(str, "Source buffer [%d]", id);
	if (win->width && win->height) {
		if (!input->width || !input->height) {
			iav_error("%s input %dx%d cannot be zero.\n",
				str, input->width, input->height);
			return -1;
		}

		switch (type) {
		case IAV_SRCBUF_TYPE_ENCODE:
		case IAV_SRCBUF_TYPE_VCA:
			output = *win;
			break;
		case IAV_SRCBUF_TYPE_PREVIEW:
			if (id == IAV_SRCBUF_PB) {
				if (vout_swap) {
					vout1_info = &iav->pvoutinfo[0]->video_info;
				} else {
					vout1_info = &iav->pvoutinfo[1]->video_info;
				}
				output.width = vout1_info->width;
				output.height = vout1_info->height;
			} else if (id == IAV_SRCBUF_PA) {
				if (vout_swap) {
					vout0_info = &iav->pvoutinfo[1]->video_info;
				} else {
					vout0_info = &iav->pvoutinfo[0]->video_info;
				}
				output.width = vout0_info->width;
				output.height = vout0_info->height;
			} else {
				iav_error("Buffer [%d] cannot be set as PREV.\n", id);
				return -1;
			}
			break;
		default:
			BUG();
			break;
		}
		if (check_zoom_limit(id, input, &output, str) < 0) {
			return -1;
		}
	}

	if (unwarp) {
		if (iav->encode_mode != DSP_MULTI_REGION_WARP_MODE) {
			iav_error("%s unwarp is not supported in mode %d.\n",
				str, iav->encode_mode);
		}
	}

	return 0;
}

static int check_vca_buf_param(struct ambarella_iav *iav, struct iav_srcbuf_setup *setup)
{

	int i;
	u32 vca_buf_size, idsp_out_frame_rate;
	u32 vca_buf_num = 0;
	u16 vca_dump_duration, vca_dump_interval;
	struct iav_window *win;
	struct iav_window *size;

	if (iav_vin_get_idsp_frame_rate(iav, &idsp_out_frame_rate) < 0) {
		return -1;
	}
	idsp_out_frame_rate = DIV_CLOSEST(FPS_Q9_BASE, idsp_out_frame_rate);

	for (i = IAV_SUB_SRCBUF_FIRST; i < IAV_SUB_SRCBUF_LAST; ++i) {
		vca_dump_duration = setup->dump_duration[i];
		vca_dump_interval = setup->dump_interval[i];
		if (vca_dump_duration > MAX_NUM_VCA_DUMP_DURATION) {
			iav_error("Dutation number [%d] not in range [0~%d] for buffer [%d].\n",
				vca_dump_duration, MAX_NUM_VCA_DUMP_DURATION, i);
			return -1;
		}

		if (vca_dump_interval > idsp_out_frame_rate) {
			iav_error("Interval number [%d] not in range [0~%d] for buffer [%d].\n",
				vca_dump_interval, idsp_out_frame_rate, i);
			return -1;
		}

		if (setup->type[i] == IAV_SRCBUF_TYPE_VCA) {
			++vca_buf_num;
			if (vca_buf_num > 1) {
				iav_error("Only one buffer can be set to VCA type.\n");
				return -1;
			}

			win = &iav->srcbuf[i].max;
			size = &setup->size[i];
			if (((win->width != size->width) || (win->height != size->height)) &&
				(size->width != 0 && size->height != 0)) {
				iav_error("Max resolution shoule be equal with real resolution for VCA buffer.\n");
				return -1;
			}

			if (vca_dump_duration == 0) {
				iav_error("Dutation number [%d] cannot be 0 for VCA buffer.\n",
					vca_dump_duration);
				return -1;
			}

			if (vca_dump_interval == 0) {
				iav_error("Interval number [%d] cannot be 0 for VCA buffer.\n",
					vca_dump_interval);
				return -1;
			}

			vca_buf_size = (ALIGN(win->width, PIXEL_IN_MB) *
				(1 + win->height + (win->height >> 1)) * vca_dump_duration);
			if (vca_buf_size > iav->mmap[IAV_BUFFER_VCA].size) {
				iav_error("Vca Buffer size should be 0x%x, more than max 0x%x.\n",
					vca_buf_size, iav->mmap[IAV_BUFFER_VCA].size);
				return -1;
			}
		}
	}

	return 0;
}


static int iav_check_srcbuf_setup(struct ambarella_iav *iav,
	struct iav_srcbuf_setup *setup)
{
	int i;
	u32 is_warp;
	struct iav_rect *main_input, *vin;
	struct iav_window *main_zoom, *main_buffer, *sub_input;

	iav_no_check();

	get_vin_win(iav, &vin, 0);
	is_warp = (iav->encode_mode == DSP_MULTI_REGION_WARP_MODE);

	main_buffer = &setup->size[IAV_SRCBUF_MN];
	if (is_warp) {
		/* check main buffer size in dewarp mode */
		if (check_main_buffer_size(iav, 0, main_buffer) < 0) {
			return -1;
		}

		main_zoom = &setup->size[IAV_SRCBUF_PMN];
		main_input = &setup->input[IAV_SRCBUF_PMN];
	} else {
		main_zoom = &setup->size[IAV_SRCBUF_MN];
		main_input = &setup->input[IAV_SRCBUF_MN];
	}

	/* check pre-main / main buffer */
	if (check_src_buf_type(IAV_SRCBUF_MN, setup->type[IAV_SRCBUF_MN]) < 0) {
		return -1;
	} else if (check_main_buffer_size(iav, is_warp, main_zoom) < 0) {
		return -1;
	} else if (check_main_buffer_input(is_warp, main_input, vin) < 0) {
		return -1;
	} else if (check_main_buffer_zoom(is_warp, main_input, main_zoom)) {
		return -1;
	}

	/* check sub source buffer */
	for (i = IAV_SUB_SRCBUF_FIRST; i < IAV_SUB_SRCBUF_LAST; ++i) {
		sub_input = setup->unwarp[i] ? main_zoom : main_buffer;
		if (check_src_buf_type(i, setup->type[i]) < 0) {
			return -1;
		} else if (check_sub_buf_size(iav, i, &setup->size[i]) < 0) {
			return -1;
		} else if (check_sub_buf_input(i, &setup->input[i], sub_input) < 0) {
			return -1;
		} else if (check_sub_buf_zoom(iav, i, setup->unwarp[i],
			setup->type[i], &setup->size[i], &setup->input[i]) < 0) {
			return -1;
		}
	}

	if (check_vca_buf_param(iav, setup) < 0) {
		return -1;
	}

	if (setup->type[IAV_SRCBUF_PMN] == IAV_SRCBUF_TYPE_ENCODE
		&& iav->encode_mode != DSP_MULTI_REGION_WARP_MODE) {
		iav_error("Premain buffer can only be set as encode fisheye mode!\n");
		return -1;
	}

	return 0;
}

static int iav_check_srcbuf_format(struct ambarella_iav *iav,
	struct iav_srcbuf_format *format)
{
	u32 buf = format->buf_id;
	struct iav_buffer *origin;
	struct iav_window *input;

	iav_no_check();

	if (!is_sub_srcbuf(buf)) {
		iav_error("Invalid sub buffer id [%d].\n", buf);
		return -1;
	}
	origin = &iav->srcbuf[buf];
	input = origin->unwarp ? &iav->srcbuf[IAV_SRCBUF_PMN].win :
		&iav->srcbuf[IAV_SRCBUF_MN].win;
	if (check_sub_buf_size(iav, buf, &format->size) < 0) {
		return -1;
	} else if (check_sub_buf_input(buf, &format->input, input) < 0) {
		return -1;
	} else if (check_sub_buf_zoom(iav, buf, origin->unwarp,
		origin->type, &format->size, &format->input) < 0) {
		return -1;
	}

	return 0;
}

static int iav_check_dptz_I(struct ambarella_iav *iav,
	struct iav_digital_zoom *dz)
{
	struct iav_rect *vin;
	struct iav_window *main_win;

	iav_no_check();

	get_vin_win(iav, &vin, 0);
	main_win = &iav->srcbuf[IAV_SRCBUF_MN].win;

	if (check_main_buffer_input(0, &dz->input, vin) < 0) {
		return -1;
	} else if (check_main_buffer_zoom(0, &dz->input, main_win) < 0) {
		return -1;
	} else {
		return 0;
	}
}

static int iav_check_dptz_II(struct ambarella_iav *iav,
	struct iav_digital_zoom *dz)
{
	u32 buf_id;
	struct iav_window * main_win;
	struct iav_buffer * sub;

	iav_no_check();

	buf_id = dz->buf_id;
	sub = &iav->srcbuf[buf_id];
	if (sub->type == IAV_SRCBUF_TYPE_OFF) {
		iav_error("DZ type II source buffer [%d] must not be in type OFF.\n",
			buf_id);
		return -1;
	}
	main_win = sub->unwarp ? &iav->srcbuf[IAV_SRCBUF_PMN].win :
		&iav->srcbuf[IAV_SRCBUF_MN].win;
	if (check_sub_buf_input(buf_id, &dz->input, main_win) < 0) {
		return -1;
	} else if (check_sub_buf_zoom(iav, buf_id, sub->unwarp,
			sub->type, &sub->win, &dz->input) < 0) {
		return -1;
	} else {
		return 0;
	}
}

inline void inc_srcbuf_ref(struct iav_buffer *srcbuf, u32 stream_id)
{
	srcbuf->ref_cnt |= (1 << stream_id);
	srcbuf->state = IAV_SRCBUF_STATE_BUSY;
}

inline void dec_srcbuf_ref(struct iav_buffer *srcbuf, u32 stream_id)
{
	if (srcbuf->ref_cnt) {
		srcbuf->ref_cnt &= ~(1 << stream_id);
		if (srcbuf->ref_cnt == 0) {
			srcbuf->state = IAV_SRCBUF_STATE_IDLE;
		}
	}
}


int iav_cfg_vproc_dptz(struct ambarella_iav *iav,
	struct iav_digital_zoom *dptz)
{
	if (dptz->buf_id != IAV_SRCBUF_MN) {
		iav_error("Incorrect buffer id %d for vproc dptz, should be %d!\n",
			dptz->buf_id, IAV_SRCBUF_MN);
		return -EINVAL;
	}

	if (iav_check_dptz_I(iav, dptz) < 0) {
		iav_error("Check DPTZ I failed for vproc dptz!\n");
		return -EINVAL;
	}

	iav->srcbuf[IAV_SRCBUF_MN].input = dptz->input;
	iav->srcbuf[IAV_SRCBUF_PMN].input = dptz->input;

	return 0;
}

int iav_ioc_s_srcbuf_setup(struct ambarella_iav *iav, void __user *arg)
{
	int i, rval = 0;
	struct iav_buffer *buf;
	struct iav_srcbuf_setup setup;

	if (iav->state != IAV_STATE_IDLE) {
		iav_error("Cannot setup source buffer while IAV is NOT in IDLE!\n");
		return -EPERM;
	}
	if (copy_from_user(&setup, arg, sizeof(setup)))
		return -EFAULT;

	mutex_lock(&iav->iav_mutex);
	do {
		if (iav_check_srcbuf_setup(iav, &setup) < 0) {
			rval = -EINVAL;
			break;
		}

		i = (iav->encode_mode == DSP_MULTI_REGION_WARP_MODE) ?
			IAV_SRCBUF_PMN : IAV_SRCBUF_MN;
		buf = &iav->srcbuf[IAV_SRCBUF_PMN];
		buf->win = setup.size[i];
		buf->input = setup.input[i];
		buf->type = IAV_SRCBUF_TYPE_ENCODE;

		for (i = IAV_SRCBUF_FIRST; i < IAV_SRCBUF_LAST; ++i) {
			buf = &iav->srcbuf[i];
			buf->win = setup.size[i];
			buf->input = setup.input[i];
			buf->type = setup.type[i];
//			buf->unwarp = setup.unwarp[i];
			buf->dump_interval = setup.dump_interval[i];
			buf->dump_duration = setup.dump_duration[i];
		}
	} while (0);
	mutex_unlock(&iav->iav_mutex);

	return rval;
}

int iav_ioc_g_srcbuf_setup(struct ambarella_iav *iav, void __user *arg)
{
	int i;
	struct iav_srcbuf_setup setup;
	struct iav_buffer * buffer = NULL;

	memset(&setup, 0, sizeof(setup));

	mutex_lock(&iav->iav_mutex);
	for (i = IAV_SRCBUF_FIRST; i < IAV_SRCBUF_LAST_PMN; ++i) {
		buffer = &iav->srcbuf[i];
		setup.type[i] = buffer->type;
		setup.input[i] = buffer->input;
		setup.unwarp[i] = buffer->unwarp;
		if (buffer->type != IAV_SRCBUF_TYPE_PREVIEW) {
			setup.size[i] = buffer->win;
		} else {
			if (get_vout_win(i, &setup.size[i]) < 0) {
				setup.size[i].width = 0;
				setup.size[i].height = 0;
			}
		}
		setup.dump_interval[i] = buffer->dump_interval;
		setup.dump_duration[i] = buffer->dump_duration;
	}
	i = IAV_SRCBUF_PMN;
	buffer = &iav->srcbuf[i];
	if (iav->encode_mode == DSP_MULTI_REGION_WARP_MODE) {
		setup.type[i] = IAV_SRCBUF_TYPE_ENCODE;
	} else {
		setup.type[i] = IAV_SRCBUF_TYPE_OFF;
	}
	setup.size[i] = buffer->win;
	setup.input[i] = buffer->input;
	mutex_unlock(&iav->iav_mutex);

	return copy_to_user(arg, &setup, sizeof(setup)) ? -EFAULT : 0;
}

int iav_ioc_s_srcbuf_format(struct ambarella_iav *iav, void __user *arg)
{
	u32 buf_id;
	struct iav_srcbuf_format format;

	if (!is_enc_work_state(iav)) {
		iav_error("Cannot set sub source buffer format while IAV is not in "
			"PREVIEW nor ENCODE.\n");
		return -EPERM;
	}
	if (copy_from_user(&format, arg, sizeof(format)))
		return -EFAULT;

	mutex_lock(&iav->iav_mutex);
	if (iav_check_srcbuf_format(iav, &format) < 0) {
		mutex_unlock(&iav->iav_mutex);
		return -EINVAL;
	}

	buf_id = format.buf_id;
	iav->srcbuf[buf_id].win = format.size;
	iav->srcbuf[buf_id].input = format.input;
	if ((iav->srcbuf[buf_id].type == IAV_SRCBUF_TYPE_ENCODE) ||
		(iav->srcbuf[buf_id].type == IAV_SRCBUF_TYPE_VCA)) {
		if (iav->encode_mode == DSP_MULTI_REGION_WARP_MODE) {
			clear_default_warp_dptz(iav, 1 << buf_id);
			/* Wait 3 frames to sync up the warp control and capture
			 * buffer setup commands.
			 */
			wait_vcap_count(iav, 3);
		}

		cmd_capture_buffer_setup(iav, NULL, buf_id);

		if (iav->encode_mode == DSP_MULTI_REGION_WARP_MODE) {
			set_default_warp_dptz(iav, 1 << buf_id, NULL);
		}
	}
	mutex_unlock(&iav->iav_mutex);

	return 0;
}

int iav_ioc_g_srcbuf_format(struct ambarella_iav *iav, void __user *arg)
{
	struct iav_srcbuf_format format;
	struct iav_buffer *buffer = NULL;

	if (copy_from_user(&format, arg, sizeof(format)))
		return -EFAULT;

	if (!is_srcbuf(format.buf_id)) {
		iav_error("Invalid source buffer id %d to get format.\n", format.buf_id);
		return -EINVAL;
	}

	mutex_lock(&iav->iav_mutex);
	buffer = &iav->srcbuf[format.buf_id];
	format.input = buffer->input;
	if (buffer->type != IAV_SRCBUF_TYPE_PREVIEW) {
		format.size = buffer->win;
	} else {
		if (get_vout_win(buffer->id, &format.size) < 0) {
			format.size.width = 0;
			format.size.height = 0;
		}
	}
	mutex_unlock(&iav->iav_mutex);

	return copy_to_user(arg, &format, sizeof(format)) ? -EFAULT : 0;
}

int iav_ioc_s_dptz(struct ambarella_iav *iav, void __user *arg)
{
	u32 buf_id;
	struct iav_digital_zoom dz;

	if (!is_enc_work_state(iav)) {
		iav_error("Cannot set DPTZ while IAV is not in PREVIEW nor ENCODE.\n");
		return -EPERM;
	}

	if (copy_from_user(&dz, arg, sizeof(dz)))
		return -EFAULT;

	mutex_lock(&iav->iav_mutex);
	buf_id = dz.buf_id;
	switch (buf_id) {
	case IAV_SRCBUF_MN:
		if (iav_check_dptz_I(iav, &dz) < 0) {
			mutex_unlock(&iav->iav_mutex);
			return -EINVAL;
		}
		iav->srcbuf[buf_id].input = dz.input;
		iav->srcbuf[IAV_SRCBUF_PMN].input = dz.input;
		cmd_set_warp_control(iav, NULL);
		break;
	case IAV_SRCBUF_PC:
	case IAV_SRCBUF_PB:
	case IAV_SRCBUF_PA:
		if (iav_check_dptz_II(iav, &dz) < 0) {
			mutex_unlock(&iav->iav_mutex);
			return -EINVAL;
		}
		iav->srcbuf[buf_id].input = dz.input;
		cmd_capture_buffer_setup(iav, NULL, buf_id);
		break;
	default:
		iav_error("Invalid source buffer id %d for digital zoom.\n", buf_id);
		mutex_unlock(&iav->iav_mutex);
		return -EINVAL;
		break;
	}
	mutex_unlock(&iav->iav_mutex);

	return 0;
}

int iav_ioc_g_dptz(struct ambarella_iav *iav, void __user *arg)
{
	struct iav_digital_zoom dz;

	if (copy_from_user(&dz, arg, sizeof(dz)))
		return -EFAULT;

	if (!is_srcbuf(dz.buf_id)) {
		iav_error("Invalid source buffer id %d to get digital zoom.\n", dz.buf_id);
		return -EINVAL;
	}

	mutex_lock(&iav->iav_mutex);
	dz.input = iav->srcbuf[dz.buf_id].input;
	mutex_unlock(&iav->iav_mutex);

	return copy_to_user(arg, &dz, sizeof(dz)) ? -EFAULT : 0;
}

int iav_ioc_g_srcbuf_info(struct ambarella_iav *iav, void __user *arg)
{
	struct iav_srcbuf_info info;

	if (copy_from_user(&info, arg, sizeof(info)))
		return -EFAULT;

	if (!is_srcbuf(info.buf_id)) {
		iav_error("Invalid source buffer id %d to get info.\n", info.buf_id);
		return -EINVAL;
	}

	mutex_lock(&iav->iav_mutex);
	info.state = iav->srcbuf[info.buf_id].state;
	mutex_unlock(&iav->iav_mutex);

	return copy_to_user(arg, &info, sizeof(info)) ? -EFAULT : 0;
}

void iav_init_source_buffer(struct ambarella_iav *iav)
{
	struct iav_buffer *buffer;

	/* init main source buffer */
	buffer = iav->main_buffer = &iav->srcbuf[IAV_SRCBUF_MN];
	buffer->id = IAV_SRCBUF_MN;
	buffer->type = IAV_SRCBUF_TYPE_ENCODE;
	buffer->id_dsp = DSP_CAPBUF_MN;
	buffer->ref_cnt = 0;
	buffer->unwarp = 0;
	buffer->extra_dram_buf = IAV_EXTRA_DRAM_BUF_DEFAULT;
	buffer->state = IAV_SRCBUF_STATE_IDLE;
	buffer->max.width = MNBUF_DEFAULT_MAX_WIDTH;
	buffer->max.height = MNBUF_DEFAULT_MAX_HEIGHT;

	/* we will setup input window of main buffer when
	 * VIN device registered. */
	buffer->input.x = 0;
	buffer->input.y = 0;
	buffer->input.width = 0;
	buffer->input.height = 0;
	buffer->win.width = 1920;
	buffer->win.height = 1080;
	/* sanity check */
	BUG_ON(buffer->max.width < buffer->win.width ||
		buffer->max.height < buffer->win.height);

	/* init preview C source buffer */
	buffer = &iav->srcbuf[IAV_SRCBUF_PC];
	buffer->id = IAV_SRCBUF_PC;
	buffer->type = IAV_SRCBUF_TYPE_ENCODE;
	buffer->id_dsp = DSP_CAPBUF_PC;
	buffer->ref_cnt = 0;
	buffer->unwarp = 0;
	buffer->extra_dram_buf = IAV_EXTRA_DRAM_BUF_DEFAULT;
	buffer->state = IAV_SRCBUF_STATE_IDLE;
	buffer->max.width = PCBUF_DEFAULT_MAX_WIDTH;
	buffer->max.height = PCBUF_DEFAULT_MAX_HEIGHT;
	/* the preview C source buffer is input from main buffer */
	buffer->input.x = 0;
	buffer->input.y = 0;
	buffer->input.width = 0;
	buffer->input.height = 0;
	buffer->win.width = 720;
	buffer->win.height = 480;
	/* sanity check */
	BUG_ON(buffer->max.width < buffer->win.width ||
		buffer->max.height < buffer->win.height);

	/* init preview B source buffer */
	buffer = &iav->srcbuf[IAV_SRCBUF_PB];
	buffer->id = IAV_SRCBUF_PB;
	buffer->type = IAV_SRCBUF_TYPE_PREVIEW;
	buffer->id_dsp = DSP_CAPBUF_PB;
	buffer->ref_cnt = 0;
	buffer->unwarp = 0;
	buffer->extra_dram_buf = IAV_EXTRA_DRAM_BUF_DEFAULT;
	buffer->state = IAV_SRCBUF_STATE_IDLE;
	buffer->max.width = PBBUF_DEFAULT_MAX_WIDTH;
	buffer->max.height = PBBUF_DEFAULT_MAX_HEIGHT;
	/* the preview B source buffer is input from main buffer */
	buffer->input.x = 0;
	buffer->input.y = 0;
	buffer->input.width = 0;
	buffer->input.height = 0;
	buffer->win.width = 720;
	buffer->win.height = 480;
	/* sanity check */
	BUG_ON(buffer->max.width < buffer->win.width ||
		buffer->max.height < buffer->win.height);

	/* init preview A source buffer */
	buffer = &iav->srcbuf[IAV_SRCBUF_PA];
	buffer->id = IAV_SRCBUF_PA;
	buffer->type = IAV_SRCBUF_TYPE_OFF;
	buffer->id_dsp = DSP_CAPBUF_PA;
	buffer->ref_cnt = 0;
	buffer->unwarp = 0;
	buffer->extra_dram_buf = IAV_EXTRA_DRAM_BUF_DEFAULT;
	buffer->state = IAV_SRCBUF_STATE_IDLE;
	buffer->max.width = PABUF_DEFAULT_MAX_WIDTH;
	buffer->max.height = PABUF_DEFAULT_MAX_HEIGHT;
	/* the preview A source buffer is input from main buffer */
	buffer->input.x = 0;
	buffer->input.y = 0;
	buffer->input.width = 0;
	buffer->input.height = 0;
	buffer->win.width = 0;
	buffer->win.height = 0;
	/* sanity check */
	BUG_ON(buffer->max.width < buffer->win.width ||
		buffer->max.height < buffer->win.height);

	/* init EFM source buffer */
	buffer = &iav->srcbuf[IAV_SRCBUF_EFM];
	buffer->id = IAV_SRCBUF_EFM;
	buffer->type = IAV_SRCBUF_TYPE_ENCODE;
	buffer->id_dsp = DSP_CAPBUF_EFM;
	buffer->ref_cnt = 0;
	buffer->extra_dram_buf = IAV_EXTRA_DRAM_BUF_DEFAULT;
	buffer->state = IAV_SRCBUF_STATE_IDLE;
	buffer->max.width = MAX_WIDTH_FOR_EFM;
	buffer->max.height = MAX_HEIGHT_FOR_EFM;
	buffer->win.width = 0;
	buffer->win.height = 0;
	/* sanity check */
	BUG_ON(buffer->max.width < buffer->win.width ||
		buffer->max.height < buffer->win.height);
}

