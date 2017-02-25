/*
 * iav_enc.c
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
#include <plat/ambcache.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <mach/init.h>
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
#include "iav_dec_api.h"
#include "amba_imgproc.h"

static struct iav_driver_version iav_driver_info =
{
	.arch = S2L,
	.model = 1,
	.major = 2,		/* SDK version 2.6.21 */
	.minor = 6,
	.patch = 21,
	.mod_time = 0x20161212,
	.description = "S2L Flexible Linux",
	.api_version = API_REVISION_U32,
	.idsp_version = IDSP_REVISION_U32,
};

static inline void irq_vsync_loss_detect(struct ambarella_iav *iav,
	encode_msg_t *msg)
{
	static u32 prev_sub_mode = ENC_UNKNOWN_MODE;

	spin_lock(&iav->iav_lock);
	do {
		if (msg->err_code == ERR_CODE_VSYNC_LOSS) {
			iav->err_vsync_lost = 1;
			if (!iav->err_vsync_handling) {
				iav_debug("Vsync loss detected!\n");
				wake_up_interruptible(&iav->err_wq);
				break;
			}
		} else {
			iav->err_vsync_lost = 0;
		}
		if ((VIDEO_MODE == msg->encode_operation_mode) &&
			(TIMER_MODE == prev_sub_mode)) {
			if (msg->err_code == ERR_CODE_VSYNC_LOSS) {
				iav_debug("Vsync loss error happens again!\n");
				iav->err_vsync_again = 1;
			} else {
				iav->err_vsync_again = 0;
			}
		}
	} while (0);
	spin_unlock(&iav->iav_lock);
	prev_sub_mode = msg->encode_operation_mode;

	return ;
}

static inline void irq_update_iav_enc_state(struct ambarella_iav *iav)
{
	int i;
	spin_lock(&iav->iav_lock);
	iav->state = IAV_STATE_PREVIEW;
	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		if (!is_stream_in_idle(&iav->stream[i])) {
			iav->state = IAV_STATE_ENCODING;
			break;
		}
	}
	spin_unlock(&iav->iav_lock);
}

static inline void irq_update_raw_enc_state(struct ambarella_iav *iav,
	encode_msg_t *enc_msg)
{
	struct iav_raw_encode *raw_enc = &iav->raw_enc;

	spin_lock(&iav->iav_lock);
	if (enc_msg->raw_pic_addr != 0) {
		if (enc_msg->raw_pic_addr != raw_enc->raw_curr_addr) {
			raw_enc->raw_curr_num++;
			raw_enc->raw_curr_addr = enc_msg->raw_pic_addr;
			if (raw_enc->raw_curr_num == raw_enc->raw_frame_num) {
				wake_up_interruptible(&raw_enc->raw_wq);
			}
		}
	}
	spin_unlock(&iav->iav_lock);

	return ;
}

static void handle_enc_msg(struct ambarella_iav *iav, DSP_MSG *msg)
{
	struct iav_stream *stream;
	struct iav_hwtimer_pts_info *hw_pts_info = &iav->pts_info.hw_pts_info;
	encode_msg_t *enc_msg = (encode_msg_t *)msg;
	int i, enc_state, wake_up;
	u32 fake_update_flag = 0;

	irq_vsync_loss_detect(iav, enc_msg);

	for (i = 0; i < IAV_STREAM_MAX_NUM_ALL; ++i) {
		stream = &iav->stream[i];
		if (unlikely(stream->fake_dsp_state == ENC_BUSY_STATE)) {
			fake_update_flag = 1;
			break;
		}
	}

	irq_update_frame_count(iav, enc_msg, fake_update_flag);

	for (i = 0; i < IAV_STREAM_MAX_NUM_ALL; ++i) {
		wake_up = 0;
		stream = &iav->stream[i];
		if (likely(stream->fake_dsp_state == ENC_IDLE_STATE)) {
			enc_state = enc_msg->encode_stream_state[i];
		} else {
			enc_state = ENC_BUSY_STATE;
		}
		spin_lock(&iav->iav_lock);
		switch (enc_state) {
		case ENC_BUSY_STATE:
			if (is_stream_in_starting(stream)) {
				wake_up = 1;
			}
			break;

		case ENC_IDLE_STATE:
			wake_up = irq_update_stopping_stream_state(stream);
			break;

		default:
			iav_error("Invalid state [%d] for stream %c.\n",
				enc_state, 'A' + i);
			break;
		}
		stream->dsp_state = enc_state;
		if (unlikely(stream->fake_dsp_state == ENC_BUSY_STATE)) {
			stream->fake_dsp_state = ENC_IDLE_STATE;
		}
		spin_unlock(&iav->iav_lock);
		if (wake_up) {
			wake_up_interruptible(&stream->venc_wq);
		}
	}
	if (iav->system_config[iav->encode_mode].enc_raw_rgb ||
		iav->system_config[iav->encode_mode].enc_raw_yuv) {
		irq_update_raw_enc_state(iav, enc_msg);
	}
	if (iav->pts_info.hwtimer_enabled) {
		hw_pts_info->audio_tick = enc_msg->vcap_audio_clk_counter;
		get_hwtimer_output_ticks(&hw_pts_info->hwtimer_tick);
	}
	if ((enc_msg->encode_operation_mode == VIDEO_MODE) &&
		(enc_msg->main_y_addr != 0x0)) {
		irq_update_iav_enc_state(iav);
		irq_update_bufcap(iav, enc_msg);
	}
	if (enc_msg->enc_sync_cmd_update_fail) {
		iav_error("Fail to update enc sync cmd!\n");
	}
	amba_imgproc_msg(enc_msg, iav->buf_cap.mono_pts);

	if (enc_msg->flexible_dram_used != iav->dsp_used_dram) {
		iav->dsp_used_dram = enc_msg->flexible_dram_used;
		iav_debug("DSP DRAM Used: %d MB, Limited: %d MB\n",
			(enc_msg->flexible_dram_used >> 20),
			((enc_msg->flexible_dram_free + enc_msg->flexible_dram_used) >> 20));
	}

	return ;
}

static void handle_hash_msg(struct ambarella_iav *iav, DSP_MSG *msg)
{
	hash_verification_msg_t *hash_msg = (hash_verification_msg_t *)msg;

	iav->hash_output[0] = hash_msg->hash_output[0];
	iav->hash_output[1] = hash_msg->hash_output[1];
	iav->hash_output[2] = hash_msg->hash_output[2];
	iav->hash_output[3] = hash_msg->hash_output[3];

	if (iav->wait_hash_msg) {
		iav->hash_msg_cnt ++;
		wake_up_interruptible(&iav->hash_wq);
	}

	return ;
}

static void handle_hash_msg_ex(struct ambarella_iav *iav, DSP_MSG *msg)
{
	hash_verification_msg_ex_t *hash_msg = (hash_verification_msg_ex_t *)msg;

	iav->hash_output[0] = hash_msg->hash_output[0];
	iav->hash_output[1] = hash_msg->hash_output[1];
	iav->hash_output[2] = hash_msg->hash_output[2];
	iav->hash_output[3] = hash_msg->hash_output[3];

	if (iav->wait_hash_msg) {
		iav->hash_msg_cnt ++;
		wake_up_interruptible(&iav->hash_wq);
	}

	return ;
}

static void handle_vcap_msg(void *data, DSP_MSG *msg)
{
	struct ambarella_iav *iav = data;

	switch (msg->msg_code) {
	case DSP_STATUS_MSG_ENCODE:
		handle_enc_msg(iav, msg);
		break;
	case DSP_STATUS_MSG_FBP_INFO:
		handle_mem_msg(iav, msg);
		break;
	case DSP_STATUS_MSG_EFM_CREATE_FBP:
	case DSP_STATUS_MSG_EFM_REQ_FB:
	case DSP_STATUS_MSG_EFM_HANDSHAKE:
		handle_efm_msg(iav, msg);
		break;
	case DSP_STATUS_MSG_HASH_VERIFICATION:
		handle_hash_msg(iav, msg);
		break;
	case DSP_STATUS_MSG_HASH_VERIFICATION_EX:
		handle_hash_msg_ex(iav, msg);
		break;
	case DSP_STATUS_MSG_DECODE:
	default:
		iav_error("Invalid message status [%u] for encode mode.\n",
			msg->msg_code);
		break;
	}
}


static int enter_preview_state(struct ambarella_iav *iav)
{
	struct dsp_device *dsp = iav->dsp;
	struct amb_dsp_cmd *first, *cmd;
	int i, max_cmd_num;
	u32 buf_map = 0;

	/* allocate 16 cmds for the cmd block to enter preview */
	max_cmd_num = 16;
	first = dsp->get_multi_cmds(dsp, max_cmd_num, DSP_CMD_FLAG_BLOCK);
	if (!first)
		return -ENOMEM;

	/* VOUT configuration. It must run in the beginning. */
	if (iav->dsp_enc_state != DSP_ENCODE_MODE || iav->resume_flag == 1) {
		iav_config_vout(VOUT_SRC_ENC);
	} else {
		iav_change_vout_src(VOUT_SRC_ENC);
	}

	cmd = first;
	cmd_system_info_setup(iav, cmd, 0);

	get_next_cmd(cmd, first);
	cmd_set_vin_global_config(iav, cmd);

	get_next_cmd(cmd, first);
	cmd_set_vin_config(iav, cmd);

	get_next_cmd(cmd, first);
	cmd_set_vin_master_config(iav, cmd);

	get_next_cmd(cmd, first);
	cmd_sensor_config(iav, cmd);

	get_next_cmd(cmd, first);
	cmd_video_preproc(iav, cmd);

	get_next_cmd(cmd, first);
	cmd_set_warp_control(iav, cmd);

	get_next_cmd(cmd, first);
	cmd_video_system_setup(iav, cmd);

	if (iav->system_config[iav->encode_mode].enc_raw_rgb ||
		iav->system_config[iav->encode_mode].enc_raw_yuv) {
		get_next_cmd(cmd, first);
		cmd_raw_encode_video_setup(iav, cmd);
		iav->raw_enc.cmd_send_flag = 1;
	}

	for (i = IAV_SUB_SRCBUF_FIRST; i < IAV_SUB_SRCBUF_LAST; ++i) {
		if (((iav->srcbuf[i].type == IAV_SRCBUF_TYPE_ENCODE) ||
			(iav->srcbuf[i].type == IAV_SRCBUF_TYPE_VCA)) &&
			iav->srcbuf[i].max.width) {
			get_next_cmd(cmd, first);
			cmd_capture_buffer_default_setup(iav, cmd, i);
		}
	}

	if (iav->encode_mode == DSP_MULTI_REGION_WARP_MODE) {
		get_next_cmd(cmd, first);
		for (i = IAV_SUB_SRCBUF_FIRST; i < IAV_SUB_SRCBUF_LAST; ++i) {
			buf_map |= (1 << i);
		}
		set_default_warp_dptz(iav, buf_map, cmd);
	}

	if (iav->dsp_enc_state != DSP_ENCODE_MODE) {
		/* enter preview from bootup stage or other mode, like DECODE */
		dsp->set_op_mode(dsp, DSP_ENCODE_MODE, first, iav->fast_resume);
		iav->dsp_enc_state = DSP_ENCODE_MODE;
	} else {
		/* re-enter preview again */
		dsp->set_enc_sub_mode(dsp, VIDEO_MODE, first, iav->fast_resume, 0);
	}

	/* Config capture buffer resolution after entering preview for SMEM */
	for (i = IAV_SUB_SRCBUF_FIRST; i < IAV_SUB_SRCBUF_LAST; ++i) {
		if (((iav->srcbuf[i].type == IAV_SRCBUF_TYPE_ENCODE) ||
			(iav->srcbuf[i].type == IAV_SRCBUF_TYPE_VCA)) &&
			iav->srcbuf[i].max.width) {
			cmd_capture_buffer_setup(iav, NULL, i);
		}
	}

	return 0;
}

static void update_default_param_before_preview(struct ambarella_iav *iav,
	struct iav_rect *vin)
{
	int i;
	struct iav_window *input;
	struct iav_buffer * main_buffer, * buffer;
	u32 encode_mode = iav->encode_mode;
	struct iav_system_config *config = &iav->system_config[encode_mode];
	struct iav_rect main_output;

	/* default pre main input = vin */
	buffer = &iav->srcbuf[IAV_SRCBUF_PMN];
	if (!buffer->input.width) {
		buffer->input.width = vin->width;
		buffer->input.x = 0;
	}
	if (!buffer->input.height) {
		buffer->input.height = vin->height;
		buffer->input.y = 0;
	}

	/* default main input = pre main input */
	main_buffer = &iav->srcbuf[IAV_SRCBUF_MN];
	if (is_warp_mode(iav)) {
		buffer->max = buffer->win;
		main_buffer->input.width = buffer->win.width;
		main_buffer->input.height = buffer->win.height;
		main_buffer->input.x = main_buffer->input.y = 0;
	} else {
		main_buffer->input = buffer->input;
	}

	/* default sub buffer input = pre-main or main size  */
	for (i = IAV_SUB_SRCBUF_FIRST; i < IAV_SUB_SRCBUF_LAST; ++i) {
		buffer = &iav->srcbuf[i];
		input = &main_buffer->win;
		if (!buffer->input.width) {
			buffer->input.width = input->width;
			buffer->input.x = 0;
		}
		if (!buffer->input.height) {
			buffer->input.height = input->height;
			buffer->input.y = 0;
		}
	}

	/* TODO: clear all dewarp params */
	memset(iav->warp, 0, sizeof(struct iav_warp_main));

	/* Update warp aspect ratio */
	switch (encode_mode) {
	case DSP_ADVANCED_ISO_MODE:
		if (config->lens_warp) {
			memset(&main_output, 0, sizeof(main_output));
			main_output.width = iav->srcbuf[IAV_SRCBUF_MN].win.width;
			main_output.height = iav->srcbuf[IAV_SRCBUF_MN].win.height;
			update_warp_aspect_ratio(IAV_SRCBUF_MN, 0, &main_buffer->input, &main_output);
		}
		break;
	default:
		break;
	}
}

static int iav_cross_check_vin(struct ambarella_iav *iav, struct iav_rect *vin)
{
	u32 buf_id, pm_unit;
	int is_pre_main, enc_mode;
	struct iav_window * main_win = NULL;
	struct iav_rect * input = NULL;
	struct iav_enc_limitation * limit = NULL;
	struct iav_system_config * config = NULL;
	u8 hdr_mode = iav->vinc[0]->dev_active->cur_format->hdr_mode;

	enc_mode = iav->encode_mode;
	limit = &G_encode_limit[enc_mode];

	/* error if VIN is high MP sensor while encode mode cannot support */
	if ((vin->width > MAX_WIDTH_IN_FULL_FPS) && !limit->high_mp_vin_supported) {
		iav_error("VIN width %d cannot be greater than %d in mode %d. "
			"It cannot support high MP sensor.\n", vin->width,
			MAX_WIDTH_IN_FULL_FPS, enc_mode);
		return -1;
	}


	/* check vin hdr-mode when set expo_num in iav */
	config = &iav->system_config[enc_mode];
	if (config->expo_num > MIN_HDR_EXPOSURES) {
		if ((config->expo_num - hdr_mode) != 1) {
			iav_error("The hdr-mode is [%d], the hdr-expo is [%d].\n", hdr_mode,
				config->expo_num);
			return -1;
		}
	}

	/* error if vin width are not aligned to 32 when raw_capture is disable */
	if (!config->raw_capture && !config->enc_raw_yuv && (vin->width & 0x1F)) {
		iav_error("vin width %d must be multiple of 32.\n", vin->width);
		return -1;
	}
	/* error if vin width are not aligned to 16 when raw_capture is enable */
	if (config->raw_capture && (vin->width & 0x0F)) {
		iav_error("vin width %d must be multiple of 16.\n", vin->width);
		return -1;
	}

	is_pre_main = (enc_mode == DSP_MULTI_REGION_WARP_MODE);
	buf_id = is_pre_main ? IAV_SRCBUF_PMN : IAV_SRCBUF_MN;
	main_win = &iav->srcbuf[buf_id].win;
	input = &iav->srcbuf[buf_id].input;

	if (!iav->system_config[enc_mode].enc_raw_yuv) {
		pm_unit = get_pm_unit(iav);
		/* error if VIN width is not 32 pixels aligned while PM is BPP type */
		if ((pm_unit == IAV_PM_UNIT_PIXEL) &&
			(vin->width & ((PIXEL_IN_MB << 1) - 1))) {
			iav_error("VIN width %d MUST be multiple of 32 pixels when PM is "
				"1bpp type in mode %d.\n", vin->width, enc_mode);
			return -1;
		}
		/* error if pre-main / main > VIN */
		if ((main_win->width > ALIGN(vin->width, PIXEL_IN_MB)) ||
			(main_win->height > ALIGN(vin->height, PIXEL_IN_MB))) {
			iav_error("%s %dx%d cannot be larger than VIN %dx%d in mode %d.\n",
				is_pre_main ? "Pre-main buffer" : "Source buffer [0]",
				main_win->width, main_win->height, ALIGN(vin->width, PIXEL_IN_MB),
				ALIGN(vin->height, PIXEL_IN_MB), iav->encode_mode);
			return -1;
		}
	}

	/* error if pre-main / main input > VIN */
	if ((input->width + input->x > vin->width) ||
		(input->height + input->y > vin->height)) {
		iav_error("%s input window %dx%d + offset %dx%d cannot be "
			"larger than VIN %dx%d in mode %d.\n",
			is_pre_main ? "Pre-main buffer" : "Source buffer [0]",
			input->width, input->height, input->x, input->y,
			vin->width, vin->height, iav->encode_mode);
		return -1;
	}

	if (check_main_buffer_input(is_pre_main, input, vin) < 0) {
		return -1;
	}

	return 0;
}

static int cross_check_source_buffer(struct ambarella_iav *iav, int id)
{
	struct iav_buffer * buffer = NULL;
	struct iav_window * max = NULL, * limit = NULL;

	buffer = &iav->srcbuf[id];
	max = &buffer->max;
	// error if buffer size > max size
	if ((buffer->win.width > max->width) || (buffer->win.height> max->height)) {
		iav_error("Source buffer [%d] size %dx%d cannot be greater than "
			"system resource limit %dx%d.\n", id, buffer->win.width,
			buffer->win.height, max->width, max->height);
		return -1;
	}
	if (buffer->type != IAV_SRCBUF_TYPE_OFF) {
		/* error if buffer max is 0x0 when it is not OFF type */
		if (!max->width || !max->height) {
			iav_error("Source buffer [%d] type must be OFF when max size "
				"is 0x0.\n", id);
			return -1;
		}
		/* error if buffer max < min limit */
		limit = &G_encode_limit[iav->encode_mode].min_main;
		if ((max->width < limit->width) || (max->height < limit->height)) {
			iav_error("Source buffer [%d] max size %dx%d cannot be smaller"
				" than the limit %dx%d in mode %d.\n", id, max->width,
				max->height, limit->width, limit->height, iav->encode_mode);
			return -1;
		}
	}

	return 0;
}

static int cross_check_resource_in_mode(struct ambarella_iav *iav)
{
	u32 encode_mode = iav->encode_mode;
	struct iav_system_config *config = &iav->system_config[encode_mode];

	switch (encode_mode) {
	case DSP_MULTI_REGION_WARP_MODE:
		if (config->max_warp_input_width > iav->srcbuf[IAV_SRCBUF_PMN].win.width) {
			iav_error("Max warp input width [%d] cannot be larger than the "
				"pre main %d in mode %d.\n", config->max_warp_input_width,
				iav->srcbuf[IAV_SRCBUF_PMN].win.width, iav->encode_mode);
			return -1;
		}
		if (cfg_default_multi_warp(iav) < 0) {
			iav_error("Failed to set default multi warp param!\n");
			return -1;
		}
		break;
	default:
		break;
	}
	return 0;
}

static int iav_cross_check_in_idle(struct ambarella_iav *iav, struct iav_rect *vin)
{
	int i;

	if (iav_cross_check_vin(iav, vin) < 0) {
		return -1;
	}

	/* cross check for source buffers */
	for (i = IAV_SRCBUF_FIRST; i < IAV_SRCBUF_LAST; ++i) {
		if (cross_check_source_buffer(iav, i) < 0)
			return -1;
	}

	if (cross_check_resource_in_mode(iav) < 0) {
		return -1;
	}

	return 0;
}

static int iav_check_vin(struct ambarella_iav *iav)
{
	if (!iav->vin_enabled) {
		iav_error("VIN is not enabled! Enable VIN first!\n");
		return -1;
	}
	if (!iav->vinc[0]->dev_active->cur_format) {
		iav_error("VIN is not activated yet. Enable VIN first!\n");
		return -1;
	}

	if (iav_vin_idsp_cross_check(iav) < 0) {
		return -1;
	}
	return 0;
}

static int iav_check_vout(struct ambarella_iav *iav)
{
	u8 vout_swap = iav->system_config[iav->encode_mode].vout_swap;
	int vout0_id = 0, vout1_id = 1;

	if (vout_swap) {
		vout0_id = 1;
		vout1_id = 0;
	}
	/* check VOUT 0 */
	if ((!iav->pvoutinfo[vout0_id]->enabled) &&
		(iav->srcbuf[IAV_SRCBUF_PA].type == IAV_SRCBUF_TYPE_PREVIEW)) {
		iav_error("preview A enabled, but VOUT 0 not configured \n");
		return -1;
	}

	/* check VOUT 1 */
	if ((!iav->pvoutinfo[vout1_id]->enabled) &&
		(iav->srcbuf[IAV_SRCBUF_PB].type == IAV_SRCBUF_TYPE_PREVIEW)) {
		iav_error("preview B enabled, but VOUT 1 not configured \n");
		return -1;
	}

#if 0	// Fixme: comment out the strictest check for VOUT and preview buffers
	if (iav->pvoutinfo[0]->enabled &&
		(iav->srcbuf[IAV_SRCBUF_PA].type != IAV_SRCBUF_TYPE_PREVIEW)) {
		iav_error("Set 4th buffer as preview when preview A is enabled!\n");
		return -EAGAIN;
	}
	if (iav->pvoutinfo[1]->enabled &&
		(iav->srcbuf[IAV_SRCBUF_PB].type != IAV_SRCBUF_TYPE_PREVIEW)) {
		iav_error("Set 3rd buffer as preview when preview B is enabled!\n");
		return -EAGAIN;
	}
#endif

	if (!iav->pvoutinfo[0]->enabled && !iav->pvoutinfo[1]->enabled) {
		iav_debug("VOUT are all disabled.\n");
	}

	return 0;
}

static int check_vin_resource_limit(struct ambarella_iav *iav)
{
	u32 chip_id, chip_idx;
	u32 vin_pps;
	u32 vin_pps_limit;
	int vin_fps;
	u32 vin_width, vin_height;
	struct iav_rect *vin_win;

	iav_no_check();

	get_vin_win(iav, &vin_win, 1);
	vin_width = ALIGN(vin_win->width, PIXEL_IN_MB);
	vin_height = ALIGN(vin_win->height, PIXEL_IN_MB);

	if (iav->vinc[0]->vin_format.frame_rate == 0) {
		iav_error("Divider of frame_rate can not be zero\n");
		return -1;
	}
	vin_fps = DIV_CLOSEST(512000000, iav->vinc[0]->vin_format.frame_rate);

	chip_id = get_chip_id(iav);
	if (chip_id < IAV_CHIP_ID_S2LM_FIRST ||
		chip_id >= IAV_CHIP_ID_S2L_LAST) {
		iav_error("Invalid S2L chip ID [%d].\n", chip_id);
		chip_id = IAV_CHIP_ID_S2L_99;
	}

	vin_pps = vin_width * vin_height * vin_fps;

	chip_idx = chip_id - IAV_CHIP_ID_S2LM_FIRST;
	vin_pps_limit = G_system_load[chip_idx].vin_pps;
	if (vin_pps > vin_pps_limit) {
		iav_warn("Total vin pps %d exceed except pps %d (%s).\n",
			vin_pps, vin_pps_limit, G_system_load[chip_idx].vin_pps_desc);
	}

	return 0;
}

static int iav_check_before_enter_preview(struct ambarella_iav *iav, struct iav_rect *vin)
{
	iav_no_check();

	if (iav_check_vin(iav) < 0) {
		return -1;
	}

	if (iav_check_vout(iav) < 0) {
		return -1;
	}

	if (iav_cross_check_in_idle(iav, vin) < 0) {
		return -1;
	}

	if (iav_check_sys_clock(iav) < 0) {
		return -1;
	}

	/* check total vin performance limit */
	if (check_vin_resource_limit(iav) < 0) {
		iav_error("Cannot start preview, system resource is not enough.\n");
		return -1;
	}

	if (iav_check_sys_mem_usage(iav, (int)IAV_STATE_PREVIEW, 0) < 0) {
		return -1;
	}

	if ((iav->encode_mode == DSP_MULTI_REGION_WARP_MODE) &&
		(iav_check_warp_idsp_perf(iav, NULL) < 0)) {
		return -1;
	}

	return 0;
}

static int update_default_param_after_preview(struct ambarella_iav *iav)
{
	int rval = 0;

	/* use iav mem info to fill dsp default binary address for encoding */
	if (!iav->resume_flag)
		iav_reset_pm(iav);

	/* DSP partitions physical address may be updated after preview*/
	if (iav->dsp_map_updated || iav->dsp_partition_mapped) {
		iav_unmap_dsp_partitions(iav);
		iav_map_dsp_partitions(iav);
	}

	if (iav->system_config[iav->encode_mode].enc_from_mem) {
		rval = iav_create_efm_pool(iav);
		if (rval < 0) {
			iav_error("Failed to create EFM buffer pool!\n");
			return rval;
		}
	}

	iav->state = IAV_STATE_PREVIEW;

	/* Resend IDSP upsampling config */
	rval = cmd_update_idsp_factor(iav, NULL);
	if (rval < 0) {
		iav_error("Failed to Update IDSP upsampling config!\n");
		return rval;
	}

	return rval;
}


int iav_enable_preview(struct ambarella_iav *iav)
{
	int rval = 0;
	struct iav_rect *vin;
	u8 enc_mode = iav->encode_mode;
	u8 eis_delay_count = iav->system_config[enc_mode].eis_delay_count;

	if (iav->state == IAV_STATE_PREVIEW)
		return 0;

	if (iav->state != IAV_STATE_IDLE) {
		iav_error("Please enter IDLE first: %d!\n", iav->state);
		return -EBUSY;
	}

	mutex_lock(&iav->iav_mutex);
	do {
		get_vin_win(iav, &vin, 0);
		update_default_param_before_preview(iav, vin);
		rval = iav_check_before_enter_preview(iav, vin);
		if (rval < 0) {
			rval = -EAGAIN;
			break;
		}

		if ((iav->nl_obj[NL_OBJ_IMAGE].nl_connected == 1) &&
			(iav->resume_flag == 0)) {
			rval = nl_send_request(&iav->nl_obj[NL_OBJ_IMAGE],
				NL_REQ_IMG_PREPARE_AAA);
			if (rval < 0) {
				rval = -EPERM;
				break;
			}
		}

		enter_preview_state(iav);
		if (unlikely(iav->err_vsync_again)) {
			iav_error("Vsync loss in entering preview. Abort rest cmds.\n");
			rval = -EAGAIN;
			break;
		}

		update_default_param_after_preview(iav);

		if (enc_mode == DSP_ADVANCED_ISO_MODE && eis_delay_count) {
			wait_vcap_count(iav, eis_delay_count);
		}

		if ((iav->nl_obj[NL_OBJ_IMAGE].nl_connected == 1) &&
			likely(!iav->err_vsync_again) &&
			(iav->resume_flag == 0)) {
			rval = nl_send_request(&iav->nl_obj[NL_OBJ_IMAGE],
				NL_REQ_IMG_START_AAA);
		}
	} while (0);
	mutex_unlock(&iav->iav_mutex);

	if (rval == 0) {
		iav_debug("preview enabled.\n");
	}

	return rval;
}

int iav_disable_preview(struct ambarella_iav *iav)
{
	struct dsp_device *dsp = iav->dsp;
	struct amb_dsp_cmd *cmd;
	int rval = 0;

	if (iav->state == IAV_STATE_IDLE)
		return 0;

	if (iav->state != IAV_STATE_PREVIEW) {
		iav_error("Improper IAV state: %d\n", iav->state);
		return -EBUSY;
	}
	cmd = dsp->get_cmd(dsp, DSP_CMD_FLAG_NORMAL);
	if (!cmd) {
		return -ENOMEM;
	}

	mutex_lock(&iav->iav_mutex);
	if (iav->nl_obj[NL_OBJ_IMAGE].nl_connected) {
		rval = nl_send_request(&iav->nl_obj[NL_OBJ_IMAGE],
			NL_REQ_IMG_STOP_AAA);
	}

	iav->state = IAV_STATE_EXITING_PREVIEW;
	cmd_vin_timer_mode(iav, cmd);
	/* Wait for DSP to enter IDLE mode */
	dsp->set_enc_sub_mode(dsp, TIMER_MODE, cmd, 0, 0);
	iav->state = IAV_STATE_IDLE;
	mutex_unlock(&iav->iav_mutex);

	return rval;
}

static inline int update_bsb_addr_in_dsp_init_data(struct ambarella_iav *iav)
{
	iav->dsp_enc_data_virt->dram_addr_cabac_out_bit_buffer =
		PHYS_TO_DSP(iav->mmap[IAV_BUFFER_BSB].phys);
	iav->dsp_enc_data_virt->dram_addr_jpeg_out_bit_buffer =
		PHYS_TO_DSP(iav->mmap[IAV_BUFFER_BSB].phys);
	iav->dsp_enc_data_virt->h264_out_bit_buf_sz = iav->mmap[IAV_BUFFER_BSB].size;
	iav->dsp_enc_data_virt->jpeg_out_bit_buf_sz = iav->mmap[IAV_BUFFER_BSB].size;

	iav->dsp_enc_data_virt->dram_addr_bit_info_buffer = PHYS_TO_DSP(iav->bsh_phys);
	iav->dsp_enc_data_virt->bit_info_buf_sz = iav->bsh_size;

	return 0;
}

inline int iav_boot_dsp_action(struct ambarella_iav *iav)
{
	update_bsb_addr_in_dsp_init_data(iav);
	iav->dsp->set_op_mode(iav->dsp, DSP_ENCODE_MODE, NULL, iav->resume_flag);
	iav->dsp_enc_state = DSP_ENCODE_MODE;
	iav->state = IAV_STATE_IDLE;

	return 0;
}

int iav_goto_timer_mode(struct ambarella_iav *iav)
{
	struct dsp_device *dsp = iav->dsp;
	struct amb_dsp_cmd *cmd;
	int rval = 0;

	mutex_lock(&iav->iav_mutex);
	// update bsb addr for encode mode
	update_bsb_addr_in_dsp_init_data(iav);
	rval = dsp->set_op_mode(dsp, DSP_ENCODE_MODE, NULL, 0);
	if (rval < 0) {
		iav_error("set_op_mode(DSP_ENCODE_MODE) fail\n");
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	iav->dsp_enc_state = DSP_ENCODE_MODE;

	cmd = dsp->get_cmd(dsp, DSP_CMD_FLAG_NORMAL);
	if (!cmd) {
		iav_error("no memory\n");
		mutex_unlock(&iav->iav_mutex);
		return -ENOMEM;
	}
	cmd_vin_timer_mode(iav, cmd);
	dsp->set_enc_sub_mode(dsp, TIMER_MODE, cmd, 0, 1);

	iav->state = IAV_STATE_IDLE;
	mutex_unlock(&iav->iav_mutex);

	return 0;
}


static int iav_goto_idle(struct ambarella_iav *iav)
{
	int rval = 0;

	switch (iav->state) {
	case IAV_STATE_INIT:
		iav_debug("Goto Idle from STATE_INIT.\n");
		rval = iav_boot_dsp_action(iav);
		break;

	case IAV_STATE_IDLE:
		iav_debug("Goto Idle from STATE_IDLE.\n");
		rval = 0;
		break;

	case IAV_STATE_ENCODING:
		iav_debug("Goto Idle from STATE_ENCODING.\n");
		rval = iav_ioc_stop_encode(iav, (void*)STREAM_ID_MASK);
		if (rval < 0)
			break;
		/* fall through */

	case IAV_STATE_PREVIEW:
		iav_debug("Goto Idle from STATE_PREVIEW.\n");
		rval = iav_disable_preview(iav);
		break;

	case IAV_STATE_STILL_CAPTURE:
		iav_debug("Goto Idle from STATE_STILL_CAP.\n");
		rval = -EPERM;
		break;

	case IAV_STATE_DECODING:
		iav_debug("Goto Idle from STATE_DECODING.\n");
		iav_clean_decode_stuff(iav);
		rval = iav_goto_timer_mode(iav);
		break;

	default:
		iav_debug("Goto Idle from STATE %d\n", iav->state);
		rval = -EPERM;
		break;
	}

	return rval;
}

static inline int iav_ioc_g_chip_id(struct ambarella_iav *iav, u32 __user *arg)
{
	struct dsp_device *dsp = iav->dsp;
	u32 chip_id;

	if (iav->state == IAV_STATE_INIT) {
		iav_error("Cannot get chip ID before system booted!\n");
		return -EPERM;
	}

	dsp->get_chip_id(dsp, &chip_id, NULL);

	return copy_to_user(arg, &chip_id, sizeof(u32)) ? -EFAULT : 0;
};

static inline int iav_ioc_g_drvinfo(void __user *arg)
{
	return copy_to_user(arg, &iav_driver_info,
		sizeof(struct iav_driver_version)) ? -EFAULT : 0;
}

static inline int iav_ioc_g_drvdspinfo(struct ambarella_iav *iav, void __user *arg)
{
	struct iav_driver_dsp_info iavdsp_info;
	struct dsp_device *dsp = iav->dsp;
	int ret = 0;
	u32 pre_hash_cnt = 0;

	if (copy_from_user(&iavdsp_info, arg, sizeof(iavdsp_info))) {
		return -EFAULT;
	}

	memcpy(&iavdsp_info.drv_ver, &iav_driver_info, sizeof(struct iav_driver_version));

	dsp->get_chip_id(dsp, &iavdsp_info.dspid, NULL);

	mutex_lock(&iav->hash_mutex);
	memset(iav->hash_output, 0x0, sizeof(iav->hash_output));

	iav->wait_hash_msg = 1;
	pre_hash_cnt = iav->hash_msg_cnt;

	if (0 > cmd_hash_verification(iav, iavdsp_info.dspin)) {
		iav->wait_hash_msg = 0;
		mutex_unlock(&iav->hash_mutex);
		return -EPERM;
	}

	ret = wait_event_interruptible_timeout(iav->hash_wq,
		(pre_hash_cnt != iav->hash_msg_cnt), TWO_JIFFIES);
	iav->wait_hash_msg = 0;
	if (0 > ret) {
		iav_error("can not wait hash msg\n");
		mutex_unlock(&iav->hash_mutex);
		return -EINTR;
	}

	memcpy(iavdsp_info.dspout, iav->hash_output, 4);
	mutex_unlock(&iav->hash_mutex);

	if (copy_to_user(arg, &iavdsp_info, sizeof(struct iav_driver_dsp_info))) {
		return -EFAULT;
	}

	return 0;
}

static inline int iav_ioc_get_dsp_hash(struct ambarella_iav *iav, void __user *arg)
{
	struct iav_dsp_hash dsp_hash;
	int ret = 0;
	u32 pre_hash_cnt = 0;

	if (copy_from_user(&dsp_hash, arg, sizeof(dsp_hash))) {
		return -EFAULT;
	}

	mutex_lock(&iav->hash_mutex);
	memset(iav->hash_output, 0x0, sizeof(iav->hash_output));

	iav->wait_hash_msg = 1;
	pre_hash_cnt = iav->hash_msg_cnt;

	if (0 > cmd_hash_verification_long(iav, dsp_hash.input)) {
		iav->wait_hash_msg = 0;
		mutex_unlock(&iav->hash_mutex);
		return -EPERM;
	}

	ret = wait_event_interruptible_timeout(iav->hash_wq,
		(pre_hash_cnt != iav->hash_msg_cnt), TWO_JIFFIES);
	iav->wait_hash_msg = 0;
	if (0 > ret) {
		iav_error("can not wait hash msg\n");
		mutex_unlock(&iav->hash_mutex);
		return -EINTR;
	}

	memcpy(dsp_hash.output, iav->hash_output, 4);
	mutex_unlock(&iav->hash_mutex);

	if (copy_to_user(arg, &dsp_hash, sizeof(dsp_hash))) {
		return -EFAULT;
	}

	return 0;
}

static int iav_ioc_s_dsp_clock_state(struct ambarella_iav *iav, void __user *arg)
{
	struct dsp_device *dsp = iav->dsp;
	u32 state = 0;

	state = (u32)arg;

	if (iav->state != IAV_STATE_IDLE) {
		iav_error("Only can set dsp clcok state in IDLE!\n");
		return -EPERM;
	}

	switch (state) {
	case DSP_CLOCK_NORMAL_STATE:
	case DSP_CLOCK_OFF_STATE:
		dsp->set_clock_state(DSP_CLK_TYPE_IDSP, !state);
		break;
	default:
		iav_error("Invalid DSP clock state: %d!", state);
		break;
	}

	return 0;
}

static inline int iav_ioc_wait_next_frame(struct ambarella_iav *iav)
{
	int rval = 0;
	if (!is_enc_work_state(iav)) {
		return -EPERM;
	}

	mutex_lock(&iav->iav_mutex);
	rval = wait_vcap_count(iav, 1);
	mutex_unlock(&iav->iav_mutex);

	return rval;
}

static inline int iav_ioc_g_iavstate(struct ambarella_iav *iav, void __user *arg)
{
	return copy_to_user(arg, &iav->state, sizeof(u32)) ? -EFAULT : 0;
}

static int iav_ioc_g_query_buf(struct ambarella_iav *iav, void __user *args)
{
	struct iav_querybuf querybuf;
	int rval = 0;

	if (copy_from_user(&querybuf, args, sizeof(struct iav_querybuf)))
		return -EFAULT;

	if (querybuf.buf >= IAV_BUFFER_NUM) {
		iav_error("Invalid buffer ID: %d!\n", querybuf.buf);
		return -EINVAL;
	}

	mutex_lock(&iav->iav_mutex);

	switch (querybuf.buf) {
	case IAV_BUFFER_BSB:
	case IAV_BUFFER_USR:
	case IAV_BUFFER_MV:
	case IAV_BUFFER_OVERLAY:
	case IAV_BUFFER_QUANT:
	case IAV_BUFFER_IMG:
	case IAV_BUFFER_VCA:
	case IAV_BUFFER_DSP:
		querybuf.offset = iav->mmap[querybuf.buf].phys;
		querybuf.length = iav->mmap[querybuf.buf].size;
		break;
	case IAV_BUFFER_QPMATRIX:
		querybuf.offset = iav->mmap[querybuf.buf].phys;
		querybuf.length = QP_MATRIX_SIZE;
		break;
	case IAV_BUFFER_WARP:
		querybuf.offset = iav->mmap[querybuf.buf].phys;
		querybuf.length = iav->mmap[querybuf.buf].size ? WARP_VECT_PART_SIZE : 0;
		break;
	case IAV_BUFFER_PM_BPC:
		querybuf.offset = iav->mmap[querybuf.buf].phys;
		querybuf.length = iav->mmap[querybuf.buf].size ? PM_BPC_PARTITION_SIZE : 0;
		break;
	case IAV_BUFFER_BPC:
		querybuf.offset =
			iav->mmap[querybuf.buf].size ? (iav->mmap[querybuf.buf].phys + PAGE_SIZE) : 0;
		querybuf.length =
			iav->mmap[querybuf.buf].size ? PM_BPC_PARTITION_SIZE : 0;
		break;
	case IAV_BUFFER_CMD_SYNC:
		querybuf.offset = iav->mmap[querybuf.buf].phys;
		querybuf.length = iav->mmap[querybuf.buf].size ? CMD_SYNC_TOTAL_SIZE : 0;
		break;
	case IAV_BUFFER_PM_MCTF:
		querybuf.offset = iav->mmap[querybuf.buf].phys;
		querybuf.length = iav->mmap[querybuf.buf].size ? PM_MCTF_TOTAL_SIZE : 0;
		break;
	case IAV_BUFFER_FB_DATA:
		querybuf.offset = iav->mmap[querybuf.buf].phys;
		querybuf.length = IAV_DRAM_FB_DATA;
		break;
	case IAV_BUFFER_FB_AUDIO:
		querybuf.offset = iav->mmap[querybuf.buf].phys;
		querybuf.length = IAV_DRAM_FB_AUDIO;
		break;

	default:
		querybuf.offset = 0;
		querybuf.length = 0;
		iav_error("Invalid buffer ID: %d!\n", querybuf.buf);
		break;
	}

	mutex_unlock(&iav->iav_mutex);

	if (copy_to_user(args, &querybuf, sizeof(struct iav_querybuf)))
		rval = -EFAULT;

	return rval;
}

static int iav_check_query_subbuf(struct ambarella_iav *iav,
	struct iav_query_subbuf *query)
{
	if (iav->state != IAV_STATE_PREVIEW &&
		iav->state != IAV_STATE_ENCODING) {
		iav_error("Incorrect IAV state: %d, should be Preview/Encoding!\n",
			iav->state);
		return -1;
	}
	if (query->sub_buf_map >= (1 << IAV_DSP_SUB_BUF_NUM)) {
		iav_error("Sub buffer map 0x%x, should be smaller than 0x%x\n",
			query->sub_buf_map, (1 << IAV_DSP_SUB_BUF_NUM));
		return -EINVAL;
	}

	return 0;
}

static int iav_query_dsp_subbuf(struct ambarella_iav *iav,
	struct iav_query_subbuf *query)
{
	int i;

	if ((query->sub_buf_map & iav->dsp_partition_mapped) !=
		query->sub_buf_map) {
		iav_error("Queried sub buffers should be mapped first!");
		return -EINVAL;
	}

	for (i = 0; i < IAV_DSP_SUB_BUF_NUM; ++i) {
		if (query->sub_buf_map & (1 << i)) {
			query->daddr[i] = iav->mmap_dsp[i].phys;
			query->length[i] = iav->mmap_dsp[i].size;
			query->offset[i] = iav->mmap_dsp[i].offset;
		}
	}

	return 0;
}

static int iav_ioc_g_query_subbuf(struct ambarella_iav *iav, void __user *args)
{
	struct iav_query_subbuf query;
	int rval = 0;

	if (copy_from_user(&query, args, sizeof(struct iav_query_subbuf)))
		return -EFAULT;

	rval = iav_check_query_subbuf(iav, &query);
	if (rval < 0) {
		return rval;
	}

	mutex_lock(&iav->iav_mutex);

	switch(query.buf_id) {
	case IAV_BUFFER_DSP:
		rval = iav_query_dsp_subbuf(iav, &query);
		break;
	default:
		iav_error("Unsupported buffer ID [%d] for sub buffer map!\n",
			query.buf_id);
		break;
	}

	mutex_unlock(&iav->iav_mutex);

	if (copy_to_user(args, &query, sizeof(struct iav_query_subbuf)))
		rval = -EFAULT;

	return rval;
}


static inline int check_sys_resource_mode(u8 encode_mode)
{
	iav_no_check();

	if ((encode_mode != DSP_CURRENT_MODE) &&
		(encode_mode != DSP_NORMAL_ISO_MODE) &&
		(encode_mode != DSP_MULTI_REGION_WARP_MODE) &&
		(encode_mode != DSP_BLEND_ISO_MODE) &&
		(encode_mode != DSP_ADVANCED_ISO_MODE) &&
		(encode_mode != DSP_HDR_LINE_INTERLEAVED_MODE)) {
		iav_error("Invalid encode mode %d.\n", encode_mode);
		return -1;
	}

	return 0;
}

static int check_sys_resource_general(struct iav_system_resource * resource)
{
	int i;
	u32 enc_mode = resource->encode_mode;
	struct iav_enc_limitation * limit = &G_encode_limit[enc_mode];

	/* error if features are not supported */
	if (resource->rotate_enable && !limit->rotate_supported) {
		iav_error("Encode mode %d cannot support rotation!\n", enc_mode);
		return -1;
	}
	if (resource->raw_capture_enable && !limit->rawcap_supported) {
		iav_error("Encode mode %d cannot support RAW capture!\n", enc_mode);
		return -1;
	}
	if (resource->lens_warp_enable && !limit->lens_warp_supported) {
		iav_error("Encode mode %d cannot support lens warp!\n", enc_mode);
		return -1;
	}
	if (resource->vout_swap_enable && !limit->vout_swap_supported) {
		iav_error("Encode mode %d cannot support Vout swap!\n", enc_mode);
		return -1;
	}
	if (resource->eis_delay_count < EIS_DELAY_COUNT_OFF ||
		resource->eis_delay_count > EIS_DELAY_COUNT_MAX) {
		iav_error("Invalid EIS delay count!\n");
		return -1;
	}
	if (resource->eis_delay_count > EIS_DELAY_COUNT_OFF &&
		resource->encode_mode != DSP_ADVANCED_ISO_MODE) {
		iav_error("EIS delay count is valid for mode [%d] only!\n",
			DSP_ADVANCED_ISO_MODE);
		return -1;
	}
	if (resource->idsp_upsample_type < IDSP_UPSAMPLE_TYPE_OFF ||
		resource->idsp_upsample_type >= IDSP_UPSAMPLE_TYPE_TOTAL_NUM){
		iav_error("Invaid idsp upsample type [%d]\n", resource->idsp_upsample_type);
		return -1;
	}
	if (resource->idsp_upsample_type > IDSP_UPSAMPLE_TYPE_OFF &&
		!limit->idsp_upsample_supported) {
		iav_error("Encode mode %d cannot support idsp upsample!\n", enc_mode);
		return -1;
	}
	/* error if mixer is invalid */
	if (resource->mixer_b_enable && !limit->mixer_b_supported) {
		iav_error("Encode mode %d cannot support Mixer B!\n", enc_mode);
		return -1;
	}
	if (resource->mixer_a_enable && resource->osd_from_mixer_a) {
		iav_error("Mixer A should be disabled when OSD from mixer A is used!\n");
		return -1;
	}
	if (resource->mixer_b_enable && resource->osd_from_mixer_b) {
		iav_error("Mixer B should be disabled when OSD from mixer B is used!\n");
		return -1;
	}
	if (resource->osd_from_mixer_a && resource->osd_from_mixer_b) {
		iav_error("OSD mixer A and B cannot be used for overlay at the same time!\n");
		return -1;
	}

	/* error if exposure number is invalid */
	if ((resource->exposure_num == MIN_HDR_EXPOSURES) &&
		!limit->linear_supported) {
		iav_error("Encode mode %d cannot support sensor linear mode.\n", enc_mode);
		return -1;
	}
	if ((resource->exposure_num > MIN_HDR_EXPOSURES) &&
		(limit->hdr_2x_supported == HDR_TYPE_OFF)) {
		iav_error("Encode mode %d cannot support multiple exposure HDR.\n", enc_mode);
		return -1;
	}
	if ((resource->exposure_num > MAX_HDR_2X) &&
		(limit->hdr_3x_supported == HDR_TYPE_OFF)) {
		iav_error("Encode mode %d only support up to 2X HDR.\n", enc_mode);
		return -1;
	}
	if (resource->exposure_num > MAX_HDR_EXPOSURES) {
		iav_error("HDR exposure number [%d] must be in the range [%d~%d].\n",
			resource->exposure_num, MIN_HDR_EXPOSURES, MAX_HDR_EXPOSURES);
		return -1;
	}

	/* error if max encode stream number > limit */
	if (resource->max_num_encode > limit->max_enc_num) {
		iav_error("Max encode num %d cannot be larger than %d in mode %d!\n",
			resource->max_num_encode, limit->max_enc_num, enc_mode);
		return -1;
	}
	if (!resource->max_num_encode) {
		iav_error("Max encode num %d cannot be zero!\n",
			resource->max_num_encode);
		return -1;
	}

	/* Extra DRAM buffer */
	i = IAV_SRCBUF_MN;
	if ((resource->extra_dram_buf[i] > IAV_EXTRA_DRAM_BUF_MAX) ||
		(resource->extra_dram_buf[i] < IAV_EXTRA_DRAM_BUF_MIN)) {
		iav_error("Main source buffer :Extra DRAM number [%d] must be in "
			"range of [%d, %d].\n", resource->extra_dram_buf[i],
			IAV_EXTRA_DRAM_BUF_MIN, IAV_EXTRA_DRAM_BUF_MAX);
		return -1;
	}
	for (i = IAV_SUB_SRCBUF_FIRST; i < IAV_SUB_SRCBUF_LAST; ++i) {
		if ((resource->extra_dram_buf[i] > IAV_EXTRA_DRAM_BUF_MAX) ||
			(resource->extra_dram_buf[i] < 0)) {
			iav_error("Source buffer [%d] :Extra DRAM number [%d] must be in "
				"range of [0, %d].\n", i, resource->extra_dram_buf[i],
				IAV_EXTRA_DRAM_BUF_MAX);
			return -1;
		}
	}

	if (resource->enc_dummy_latency > MAX_ENC_DUMMY_LATENCY) {
		iav_error("Encode dummy latency should be in the range of [0, %d].\n",
			MAX_ENC_DUMMY_LATENCY);
		return -1;
	}

	if (resource->mctf_pm_enable && !limit->pm_mctf_supported) {
		iav_error("Encode mode %d cannot support MCTF privacy mask!\n",
			enc_mode);
		return -1;
	}

	if ((resource->me0_scale && !limit->me0_supported) ||
		(resource->me0_scale > limit->me0_supported)) {
		iav_error("Encode mode %d cannot support ME0 scale %d!\n",
			enc_mode, resource->me0_scale);
		return -1;
	}
	if (resource->enc_from_mem) {
		if(!limit->enc_from_mem_supported) {
			iav_error("Encode mode %d cannot support encode from memory!\n",
				enc_mode);
			return -1;
		}
		if (resource->efm_buf_num > IAV_EFM_MAX_BUF_NUM ||
			resource->efm_buf_num < IAV_EFM_MIN_BUF_NUM) {
			iav_error("EFM buffer num should be in the range of [%d, %d].\n",
				IAV_EFM_MIN_BUF_NUM, IAV_EFM_MAX_BUF_NUM);
			return -1;
		}
		if (resource->efm_size.width > MAX_WIDTH_FOR_EFM ||
			resource->efm_size.width < MIN_WIDTH_FOR_EFM) {
			iav_error("EFM buffer width should be in the range of [%d, %d].\n",
				MIN_WIDTH_FOR_EFM, MAX_WIDTH_FOR_EFM);
			return -1;
		}
		if (resource->efm_size.height > MAX_HEIGHT_FOR_EFM ||
			resource->efm_size.height < MIN_HEIGHT_FOR_EFM) {
			iav_error("EFM buffer height should be in the range of [%d, %d].\n",
				MIN_HEIGHT_FOR_EFM, MAX_HEIGHT_FOR_EFM);
			return -1;
		}
	}
	if (resource->enc_raw_rgb || resource->enc_raw_yuv) {
		if (resource->enc_raw_rgb && !limit->enc_raw_rgb_supported) {
			iav_error("Encode mode %d cannot support encode RAW rgb!\n", enc_mode);
			return -1;
		}
		if (resource->enc_raw_yuv && !limit->enc_raw_yuv_supported) {
			iav_error("Encode mode %d cannot support encode RAW yuv!\n", enc_mode);
			return -1;
		}
		if (resource->enc_raw_rgb && resource->enc_raw_yuv) {
			iav_error("Cannot support encode RAW rgb and RAW yuv together!\n");
			return -1;
		}
		if (resource->raw_size.width > MAX_WIDTH_FOR_RAW||
			resource->raw_size.width < MIN_WIDTH_FOR_RAW) {
			iav_error("RAW buffer width should be in the range of [%d, %d].\n",
				MIN_WIDTH_FOR_RAW, MAX_WIDTH_FOR_RAW);
			return -1;
		}
		if (resource->raw_size.height > MAX_HEIGHT_FOR_RAW||
			resource->raw_size.height < MIN_HEIGHT_FOR_RAW) {
			iav_error("RAW buffer height should be in the range of [%d, %d].\n",
				MIN_HEIGHT_FOR_RAW, MAX_HEIGHT_FOR_RAW);
			return -1;
		}
	}

	/* error if debug option is enabled */
	if (resource->debug_enable_map & DEBUG_TYPE_ISO_TYPE) {
		switch (resource->encode_mode) {
		case DSP_ADVANCED_ISO_MODE:
			if (resource->debug_iso_type != ISO_TYPE_ADVANCED &&
				resource->debug_iso_type != ISO_TYPE_MIDDLE) {
				iav_error("Debug ISO type [%d] is invalid in mode [%d].\n",
					resource->debug_iso_type, resource->encode_mode);
				return -1;
			}
			break;
		case DSP_BLEND_ISO_MODE:
			if (resource->debug_iso_type != ISO_TYPE_BLEND) {
				iav_error("Debug ISO type [%d] is invalid in mode [%d].\n",
					resource->debug_iso_type, resource->encode_mode);
				return -1;
			}
			break;
		default:
			iav_error("Debug ISO type cannot be performed in mode [%d].\n",
				resource->encode_mode);
			return -1;
		}
	}
	if (resource->debug_enable_map & DEBUG_TYPE_STITCH) {
		if ((resource->encode_mode == DSP_SINGLE_REGION_WARP_MODE) &&
			resource->debug_stitched) {
			iav_error("Single region dewarp mode CANNOT support stitching.\n");
			return -1;
		}
	}
	if (resource->debug_enable_map & DEBUG_TYPE_CHIP_ID) {
		if (resource->debug_chip_id >= IAV_CHIP_ID_S2L_LAST) {
			iav_error("Incorrect Debug Chip ID [%d].\n",
				resource->debug_chip_id);
			return -1;
		}
	}

	/* stream check */
	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		/* stream max size check */
		if (resource->stream_max_size[i].width > MAX_WIDTH_IN_BUFFER ||
			resource->stream_max_size[i].height > MAX_HEIGHT_IN_BUFFER) {
			iav_error("Stream %c max size %dx%d cannot be larger than "
				"%dx%d.\n", 'A' + i, resource->stream_max_size[i].width,
				resource->stream_max_size[i].height, MAX_WIDTH_IN_BUFFER,
				MAX_HEIGHT_IN_BUFFER);
			return -1;
		}
		/* stream max M check */
		if (resource->stream_max_M[i] > 1) {
			if (i > 1) {
				iav_error("Stream %c cannot set max gop M [%d] larger "
					"than 1.\n", 'A' + i, resource->stream_max_M[i]);
				return -1;
			} else if (resource->stream_max_size[i].width >
						MAX_WIDTH_FOR_TWO_REF) {
				iav_error("Stream %c cannot set max gop M [%d] larger "
					"than 1, when max stream width %d > %d!\n",
					'A' + i, resource->stream_max_M[i],
					resource->stream_max_size[i].width,
					MAX_WIDTH_FOR_TWO_REF);
				return -1;
			} else if (resource->lens_warp_enable == 1 &&
					resource->stream_long_ref_enable[i] == 0) {
				iav_error("Stream %c cannot set max gop M [%d] larger "
					"than 1 to support B frame, when lens_warp is enabled.\n",
					'A' + i, resource->stream_max_M[i]);
				return -1;
			}
		}
		if (resource->stream_long_ref_enable[i] > 1) {
			iav_error("Stream %c's long term reference enable [%d] should be "
				"[0|1].\n", 'A' + i, resource->stream_long_ref_enable[i]);
			return -1;
		}
	}
	if (resource->long_ref_b_frame > 1) {
		iav_error("Long term B frame [%d] should be [0|1].\n",
			resource->long_ref_b_frame);
		return -1;
	} else {
		if (resource->long_ref_b_frame && !limit->long_ref_b_supported) {
			iav_error("Encode mode %d cannot support B frame for long term "
				"reference case!\n", enc_mode);
			return -1;
		}
	}
	if (resource->stream_max_M[0] > MAX_REF_FRAME_INTVL) {
		iav_error("Stream A's max GOP M [%d] must be no greater than %d in "
			"mode %d.\n", resource->stream_max_M[0],
			MAX_REF_FRAME_INTVL, resource->encode_mode);
		return -1;
	}

	return 0;
}

static int check_sys_resource_buffer(struct ambarella_iav *iav, struct iav_system_resource *resource)
{
	int buf_id;
	u32 enc_mode = resource->encode_mode;
	struct iav_enc_limitation * limit = &G_encode_limit[resource->encode_mode];
	struct iav_window * max, prop, * main_win;

	/* error if main max > max limit */
	buf_id = IAV_SRCBUF_MN;
	main_win = &resource->buf_max_size[buf_id];
	if ((main_win->width > ALIGN(limit->max_main.width, PIXEL_IN_MB)) ||
		(main_win->height > ALIGN(limit->max_main.height, PIXEL_IN_MB))) {
		iav_error("Source buffer [%d] max size %dx%d cannot be larger than "
			"the limit %dx%d in mode %d.\n", buf_id,
			main_win->width, main_win->height,
			ALIGN(limit->max_main.width, PIXEL_IN_MB),
			ALIGN(limit->max_main.height, PIXEL_IN_MB), enc_mode);
		return -1;
	}

	for (buf_id = IAV_SRCBUF_FIRST; buf_id < IAV_SRCBUF_LAST; ++buf_id) {
		max = &resource->buf_max_size[buf_id];
		prop = G_buffer_limit[buf_id].max_win;
		if ((buf_id == IAV_SRCBUF_PA) || (buf_id == IAV_SRCBUF_PB)) {
			/* Support 2560 width for PA&&PB when stitch is on */
			if (resource->debug_enable_map & DEBUG_TYPE_STITCH) {
				if (resource->debug_stitched == 1) {
					prop.width = (buf_id == IAV_SRCBUF_PA) ?
						PABUF_LIMIT_MAX_WIDTH_STITCH : PBBUF_LIMIT_MAX_WIDTH_STITCH;
				}
			} else {
				if (is_stitched_vin(iav, enc_mode)) {
					prop.width = (buf_id == IAV_SRCBUF_PA) ?
						PABUF_LIMIT_MAX_WIDTH_STITCH : PBBUF_LIMIT_MAX_WIDTH_STITCH;
				}
			}
		}
		/* error if buffer max > buffer limit */
		if ((max->width > prop.width) || (max->height > prop.height)) {
			iav_error("Source buffer [%d] max %dx%d cannot be larger than "
				"%dx%d.\n", buf_id, max->width, max->height,
				prop.width, prop.height);
			return -1;
		}
		/* error if sub buffer max > main buffer max */
		if ((buf_id != IAV_SRCBUF_MN) &&
			((max->width > main_win->width) || (max->height > main_win->height))) {
			iav_error("Source buffer [%d] max %dx%d cannot be larger than "
				"main buffer max %dx%d.\n", buf_id, max->width, max->height,
				main_win->width, main_win->height);
			return -1;
		}
	}

	return 0;
}

static int check_sys_resource_enc_mode(struct iav_system_resource *resource)
{
	/* check warp region width for dewarp mode */
	if (resource->encode_mode == DSP_MULTI_REGION_WARP_MODE) {
		if (resource->max_warp_output_width > MAX_WIDTH_IN_WARP_REGION ||
			resource->max_warp_input_width > MAX_WIDTH_IN_WARP_REGION ||
			resource->max_warp_input_height > MAX_HEIGHT_IN_WARP) {
			iav_error("Max warp output width [%d], input width [%d] and input height [%d]"
				"must be smaller than hard limit [%d] in mode %d.\n",
				resource->max_warp_output_width, resource->max_warp_input_width,
				resource->max_warp_input_height, MAX_WIDTH_IN_WARP_REGION, resource->encode_mode);
			return -1;
		}

		if ((resource->max_warp_input_width > resource->v_warped_main_max_width) ||
			(resource->max_warp_input_height > resource->v_warped_main_max_height)) {
			iav_error("Max Warp input window [%dx%d] can not exceed"
				" intermediate buffer [%dx%d]\n", resource->max_warp_input_width,
				resource->max_warp_input_height, resource->v_warped_main_max_width,
				resource->v_warped_main_max_height);
			return -1;
		}
	}

	/* check max padding width for advanced ISO mode when LDC is enabled */
	if (resource->encode_mode == DSP_ADVANCED_ISO_MODE) {
		if (resource->max_padding_width > LDC_PADDING_WIDTH_MAX) {
			iav_error("Max padding width [%d] can not be greater than hard "
				"limit [%d] in mode %d.\n", resource->max_padding_width,
				LDC_PADDING_WIDTH_MAX, resource->encode_mode);
			return -1;
		}
	}

	return 0;
}

static int iav_check_sys_resource_limit(struct ambarella_iav *iav,
	struct iav_system_resource *resource)
{
	iav_no_check();

	if (check_sys_resource_mode(resource->encode_mode) < 0) {
		return -1;
	} else if (check_sys_resource_general(resource) < 0) {
		return -1;
	} else if (check_sys_resource_buffer(iav, resource) < 0) {
		return -1;
	} else if (check_sys_resource_enc_mode(resource) < 0) {
		return -1;
	}

	return 0;
}

static int update_system_resource(struct ambarella_iav *iav,
	struct iav_system_resource * resource)
{
	int i;
	int enc_mode = resource->encode_mode;
	struct iav_rect *vin;
	struct iav_system_config *config = NULL;

	mutex_lock(&iav->iav_mutex);

	iav->encode_mode = enc_mode;
	config = &iav->system_config[enc_mode];
	config->rotatable = resource->rotate_enable;
	config->raw_capture = resource->raw_capture_enable;
	config->lens_warp = resource->lens_warp_enable;
	config->expo_num = resource->exposure_num;
	config->max_warp_input_width = resource->max_warp_input_width;
	config->max_warp_input_height = resource->max_warp_input_height;
	config->max_warp_output_width = resource->max_warp_output_width;
	config->max_padding_width = resource->max_padding_width;
	config->v_warped_main_max_width = resource->v_warped_main_max_width;
	config->v_warped_main_max_height = resource->v_warped_main_max_height;
	config->enc_dummy_latency = resource->enc_dummy_latency;
	config->idsp_upsample_type = resource->idsp_upsample_type;
	config->vout_swap = resource->vout_swap_enable;
	config->eis_delay_count = resource->eis_delay_count;
	config->me0_scale = resource->me0_scale;
	config->mctf_pm = resource->mctf_pm_enable;
	config->enc_from_mem = resource->enc_from_mem;
	iav->efm.req_buf_num = resource->efm_buf_num;
	iav->efm.efm_size = resource->efm_size;
	config->enc_raw_rgb = resource->enc_raw_rgb;
	config->enc_raw_yuv = resource->enc_raw_yuv;
	get_vin_win(iav, &vin, 1);
	if (config->enc_raw_rgb || config->enc_raw_yuv) {
		/* Fixme: revise VIN source window to raw size */
		vin->width = resource->raw_size.width;
		vin->height = resource->raw_size.height;
		iav->raw_enc.raw_size = resource->raw_size;
	}

	config->debug_enable_map = resource->debug_enable_map;
	if (config->debug_enable_map & DEBUG_TYPE_STITCH) {
		config->debug_stitched = resource->debug_stitched;
	}
	if (config->debug_enable_map & DEBUG_TYPE_ISO_TYPE) {
		config->debug_iso_type = resource->debug_iso_type;
	}
	if (config->debug_enable_map & DEBUG_TYPE_CHIP_ID) {
		config->debug_chip_id = resource->debug_chip_id;
	}

	/* update buffer config */
	for (i = IAV_SRCBUF_FIRST; i < IAV_SRCBUF_LAST; ++i) {
		iav->srcbuf[i].max = resource->buf_max_size[i];
		iav->srcbuf[i].extra_dram_buf = resource->extra_dram_buf[i];
	}

	/* update stream config */
	for (i = 0; i < IAV_STREAM_MAX_NUM_IMPL; ++i) {
		iav->stream[i].max = resource->stream_max_size[i];
		iav->stream[i].max_GOP_M = resource->stream_max_M[i];
		iav->stream[i].max_GOP_N = resource->stream_max_N[i];
		iav->stream[i].long_ref_enable = resource->stream_long_ref_enable[i];
	}
	config->long_ref_b_frame = resource->long_ref_b_frame;
	config->max_stream_num = resource->max_num_encode;
	iav->mixer_a_enable = resource->mixer_a_enable;
	iav->mixer_b_enable = resource->mixer_b_enable;
	iav->osd_from_mixer_a = resource->osd_from_mixer_a;
	iav->osd_from_mixer_b = resource->osd_from_mixer_b;
	iav->vin_overflow_protection = resource->vin_overflow_protection;

	spin_lock_irq(&iav->iav_lock);
	if (resource->dsp_partition_map != iav->dsp_partition_mapped) {
		iav->dsp_map_updated = 1;
		iav->dsp_partition_to_map = resource->dsp_partition_map;
	} else {
		iav->dsp_map_updated = 0;
	}

	config->extra_top_row_buf_enable = resource->extra_top_row_buf_enable;
	spin_unlock_irq(&iav->iav_lock);

	mutex_unlock(&iav->iav_mutex);

	return 0;
}

static int iav_ioc_s_system_resource(struct ambarella_iav *iav, void __user *arg)
{
	struct iav_system_resource resource;

	if (iav->state != IAV_STATE_IDLE) {
		iav_error("s_system_resouce must be called in IAV IDLE state!\n");
		return -EPERM;
	}

	if (copy_from_user(&resource, arg, sizeof(struct iav_system_resource)))
		return -EFAULT;

	if (iav_check_sys_resource_limit(iav, &resource) < 0) {
		return -EINVAL;
	}
	update_system_resource(iav, &resource);

	return 0;
}

static int iav_ioc_g_system_resource(struct ambarella_iav *iav, void __user *arg)
{
	int i, enc_mode;
	struct iav_system_resource param;
	struct iav_system_config * config = NULL;

	if (copy_from_user(&param, arg, sizeof(param)))
		return -EFAULT;

	if (check_sys_resource_mode(param.encode_mode) < 0) {
		return -1;
	}

	mutex_lock(&iav->iav_mutex);

	enc_mode = (param.encode_mode == DSP_CURRENT_MODE) ?
		iav->encode_mode : param.encode_mode;
	param.encode_mode = enc_mode;
	config = &iav->system_config[enc_mode];
	param.rotate_enable = config->rotatable;
	param.raw_capture_enable = config->raw_capture;
	param.lens_warp_enable = config->lens_warp;
	param.vout_swap_enable = config->vout_swap;
	param.dsp_partition_map = iav->dsp_partition_mapped;
	param.mixer_a_enable = iav->mixer_a_enable;
	param.mixer_b_enable = iav->mixer_b_enable;
	param.osd_from_mixer_a = iav->osd_from_mixer_a;
	param.osd_from_mixer_b = iav->osd_from_mixer_b;
	param.idsp_upsample_type = config->idsp_upsample_type;
	param.me0_scale = config->me0_scale;
	param.mctf_pm_enable = config->mctf_pm;
	param.enc_raw_rgb = config->enc_raw_rgb;
	param.enc_raw_yuv = config->enc_raw_yuv;
	param.eis_delay_count = enc_mode == DSP_ADVANCED_ISO_MODE ?
		config->eis_delay_count : 0;
	param.vin_overflow_protection = iav->vin_overflow_protection;
	if (config->enc_raw_rgb || config->enc_raw_yuv) {
		param.raw_size = iav->raw_enc.raw_size;
	}
	param.enc_from_mem = config->enc_from_mem;
	param.efm_buf_num = iav->efm.req_buf_num;
	param.efm_size = iav->efm.efm_size;
	param.extra_top_row_buf_enable = config->extra_top_row_buf_enable;

	for (i = IAV_SRCBUF_FIRST; i < IAV_SRCBUF_LAST; ++i) {
		param.buf_max_size[i] = iav->srcbuf[i].max;
		param.extra_dram_buf[i] = iav->srcbuf[i].extra_dram_buf;
	}
	param.max_num_cap_sources = 2;
	if ((iav->srcbuf[IAV_SRCBUF_PA].type == IAV_SRCBUF_TYPE_ENCODE) ||
		(iav->srcbuf[IAV_SRCBUF_PA].type == IAV_SRCBUF_TYPE_VCA))
		++param.max_num_cap_sources;
	if ((iav->srcbuf[IAV_SRCBUF_PB].type == IAV_SRCBUF_TYPE_ENCODE) ||
		(iav->srcbuf[IAV_SRCBUF_PB].type == IAV_SRCBUF_TYPE_VCA))
		++param.max_num_cap_sources;

	for (i = 0; i < IAV_STREAM_MAX_NUM_IMPL; ++i) {
		param.stream_max_size[i] = iav->stream[i].max;
		param.stream_max_M[i] = iav->stream[i].max_GOP_M;
		param.stream_max_N[i] = iav->stream[i].max_GOP_N;
		param.stream_max_advanced_quality_model[i] = IAV_GOP_LT_REF_P;
		param.stream_long_ref_enable[i] = iav->stream[i].long_ref_enable;
	}
	param.max_num_encode = config->max_stream_num;
	param.exposure_num = config->expo_num;
	if (enc_mode == DSP_MULTI_REGION_WARP_MODE) {
		param.max_warp_input_width = config->max_warp_input_width;
		param.max_warp_input_height = config->max_warp_input_height;
		param.max_warp_output_width = config->max_warp_output_width;
		param.v_warped_main_max_width = config->v_warped_main_max_width;
		param.v_warped_main_max_height = config->v_warped_main_max_height;
	}
	param.max_padding_width = (enc_mode == DSP_ADVANCED_ISO_MODE) ?
		LDC_PADDING_WIDTH_MAX : 0;
	param.enc_dummy_latency = config->enc_dummy_latency;
	param.long_ref_b_frame = config->long_ref_b_frame;

	/* Read only fields */
	param.raw_pitch_in_bytes = config->raw_capture ?
		(param.raw_size.width << 1) : 0;
	if(DSP_BSB_START > 0x10000000)
		param.total_memory_size = IAV_DRAM_SIZE_8Gb;
	else
		param.total_memory_size = IAV_DRAM_SIZE_2Gb;
	param.hdr_type = get_hdr_type(iav);
	param.iso_type = get_iso_type(iav, config->debug_enable_map);
	param.is_stitched = is_stitched_vin(iav, enc_mode);

	/* Debug fields */
	param.debug_enable_map = config->debug_enable_map;
	param.debug_stitched = config->debug_stitched;
	param.debug_chip_id = config->debug_chip_id;
	if (config->debug_enable_map & DEBUG_TYPE_ISO_TYPE) {
		param.debug_iso_type = config->debug_iso_type;
	} else {
		param.debug_iso_type = param.iso_type;
	}

	/* ARCH info */
	param.arch = S2L;

	mutex_unlock(&iav->iav_mutex);

	return copy_to_user(arg, &param, sizeof(param)) ? -EFAULT : 0;
}

static int iav_ioc_q_enc_mode_cap(struct ambarella_iav *iav, void __user *arg)
{
	int enc_mode;
	struct iav_enc_mode_cap param;
	struct iav_enc_limitation * limit;

	if (copy_from_user(&param, arg, sizeof(param)))
		return -EFAULT;

	if (check_sys_resource_mode(param.encode_mode) < 0) {
		return -1;
	}

	mutex_lock(&iav->iav_mutex);

	enc_mode = (param.encode_mode == DSP_CURRENT_MODE) ?
		iav->encode_mode : param.encode_mode;

	mutex_unlock(&iav->iav_mutex);

	limit = &G_encode_limit[enc_mode];
	param.encode_mode = enc_mode;
	param.max_streams_num = limit->max_enc_num;
	param.max_chroma_radius = limit->max_chroma_radius;
	param.max_wide_chroma_radius = limit->max_wide_chroma_radius;
	param.max_encode_MB = limit->max_enc_MB;
	param.max_main = limit->max_main;
	param.min_main = limit->min_main;
	param.min_enc = limit->min_enc;
	param.raw_cap_possible = limit->rawcap_supported;
	param.rotate_possible = limit->rotate_supported;
	param.vout_swap_possible = limit->vout_swap_supported;
	param.lens_warp_possible = limit->lens_warp_supported;
	param.iso_type_possible = get_iso_type(iav, 0);
	param.enc_raw_rgb_possible = limit->enc_raw_rgb_supported;
	param.high_mp_possible = limit->high_mp_vin_supported;
	param.linear_possible = limit->linear_supported;
	param.hdr_2x_possible = limit->hdr_2x_supported;
	param.hdr_3x_possible = limit->hdr_3x_supported;
	param.mixer_b_possible = limit->mixer_b_supported;
	param.pm_bpc_possible = limit->pm_bpc_supported;
	param.pm_mctf_possible = limit->pm_mctf_supported;
	param.wcr_possible = limit->wcr_supported;
	param.me0_possible = limit->me0_supported;
	param.enc_from_mem_possible = limit->enc_from_mem_supported;
	param.enc_raw_yuv_possible = limit->enc_raw_yuv_supported;

	return copy_to_user(arg, &param, sizeof(param)) ? -EFAULT : 0;
}

static int iav_check_enc_buf_cap(struct iav_enc_buf_cap * param)
{
	iav_no_check();

	if ((param->buf_id < IAV_SRCBUF_FIRST) ||
		(param->buf_id >= IAV_SRCBUF_LAST)) {
		iav_error("Invalid buffer id %d.\n", param->buf_id);
		return -1;
	}

	return 0;
}

static int iav_ioc_q_enc_buf_cap(struct ambarella_iav *iav, void __user *arg)
{
	struct iav_enc_buf_cap param;
	struct iav_buffer_limitation * limit;

	if (copy_from_user(&param, arg, sizeof(param)))
		return -EFAULT;

	if (iav_check_enc_buf_cap(&param) < 0) {
		return -EINVAL;
	}

	limit = &G_buffer_limit[param.buf_id];
	param.max = limit->max_win;
	param.max_zoom_in_factor = limit->max_zoomin_factor;
	param.max_zoom_out_factor = limit->max_zoomout_factor;

	return copy_to_user(arg, &param, sizeof(param)) ? -EFAULT : 0;
}

static int iav_ioc_s_raw_enc_setup(struct ambarella_iav *iav, void __user *arg)
{
	struct iav_raw_enc_setup raw_enc;

	if (copy_from_user(&raw_enc, arg, sizeof(struct iav_raw_enc_setup)))
		return -EFAULT;

	mutex_lock(&iav->iav_mutex);

	iav->raw_enc.raw_daddr_offset = raw_enc.raw_daddr_offset;
	iav->raw_enc.raw_frame_size = raw_enc.raw_frame_size;
	iav->raw_enc.raw_frame_num = raw_enc.raw_frame_num;
	iav->raw_enc.pitch = raw_enc.pitch;
	iav->raw_enc.raw_enc_src = raw_enc.raw_enc_src;

	iav->raw_enc.raw_curr_num = 0;
	iav->raw_enc.raw_curr_addr = 0;

	if (iav->raw_enc.cmd_send_flag) {
		cmd_raw_encode_video_setup(iav, NULL);
	}

	mutex_unlock(&iav->iav_mutex);

	return 0;
}

static int iav_ioc_g_raw_enc_setup(struct ambarella_iav *iav, void __user *arg)
{
	struct iav_raw_enc_setup raw_enc;

	mutex_lock(&iav->iav_mutex);

	raw_enc.raw_daddr_offset = iav->raw_enc.raw_daddr_offset;
	raw_enc.raw_frame_size = iav->raw_enc.raw_frame_size;
	raw_enc.raw_frame_num = iav->raw_enc.raw_frame_num;
	raw_enc.pitch = iav->raw_enc.pitch;
	raw_enc.raw_enc_src = iav->raw_enc.raw_enc_src;

	mutex_unlock(&iav->iav_mutex);

	return copy_to_user(arg, &raw_enc,
		sizeof(struct iav_raw_enc_setup)) ? -EFAULT : 0;
}

static int iav_ioc_wait_raw_enc(struct ambarella_iav *iav, void __user *arg)
{
	struct iav_raw_encode *raw_enc = &iav->raw_enc;

	if(wait_event_interruptible(raw_enc->raw_wq,
		(raw_enc->raw_frame_num == raw_enc->raw_curr_num)) < 0) {
		iav_error("Wait raw encode timeout!\n");
		return -EFAULT;
	}

	return 0;
}

static int iav_check_overlay_param(struct iav_stream *stream,
	struct iav_overlay_insert *info)
{
	int i, width, height;
	struct iav_overlay_area *area;
	u32 addr_size = 0;

	iav_no_check();

	width = stream->format.enc_win.width;
	height = stream->format.enc_win.height;
	addr_size = stream->iav->mmap[IAV_BUFFER_OVERLAY].size;
	for (i = 0; i < MAX_NUM_OVERLAY_AREA; ++i) {
		area = &info->area[i];
		if (area->enable) {
			if (!area->pitch || !area->width || !area->height) {
				iav_error("overlay area pitch/width/height cannot be zero.\n");
				return -1;
			}
			if (area->pitch < area->width) {
				iav_error("overlay area pitch must not be smaller"
						" than width.\n");
				return -1;
			}
			if (area->start_x & 0x1) {
				iav_error("overlay area start x must be multiple of 2.\n");
				return -1;
			}
			if (area->start_y & 0x3) {
				iav_error("overlay area start y must be multiple of 4.\n");
				return -1;
			}
			if (area->width & 0x1) {
				iav_error("overlay area width must be multiple of 2.\n");
				return -1;
			}
			if (area->height & 0x3) {
				iav_error("overlay area height must be multiple of 4.\n");
				return -1;
			}
			if (area->width > OVERLAY_AREA_MAX_WIDTH) {
				iav_error("overlay area width [%d] must be no greater"
						" than %d.\n", area->width, OVERLAY_AREA_MAX_WIDTH);
				return -1;
			}
			if ((area->start_x + area->width) > width) {
				iav_error("overlay area width [%d] with offset [%d] is out of"
						" stream width [%d].\n",
						area->width, area->start_x, width);
				return -1;
			}
			if ((area->start_y + area->height) > height) {
				iav_error("overlay area height [%d] with offset [%d] is out of"
						" stream height [%d].\n",
						area->height, area->start_y, height);
				return -1;
			}
			if ((area->clut_addr_offset < 0) ||
				(area->clut_addr_offset > addr_size - KByte(1))) {
				iav_error("overlay clut addr offset 0x%x must be in the range"
					" of [0, 0x%x].\n", area->clut_addr_offset,
					(addr_size - KByte(1)));
				return -1;
			}
			if ((area->data_addr_offset < 0) ||
				(area->data_addr_offset + area->total_size > addr_size)) {
				iav_error("overlay data addr offset 0x%x + 0x%x must be in the"
					" range of [0, 0x%x].\n", area->data_addr_offset,
					area->total_size, addr_size);
				return -1;
			}
		}
	}

	return 0;
}

static int iav_ioc_s_overlay_insert(struct ambarella_iav *iav,
	struct iav_overlay_insert __user * arg)
{
	struct iav_stream * stream;
	struct iav_overlay_insert info;
	int max_stream_num;

	if (copy_from_user(&info, arg, sizeof(info)))
		return -EFAULT;

	max_stream_num = iav->system_config[iav->encode_mode].max_stream_num;

	if (info.id >= max_stream_num) {
		iav_error("Invalid stream ID: %d!\n", info.id);
		return -EINVAL;
	}
	stream = &iav->stream[info.id];

	if (info.enable) {
		if (check_buffer_config_limit(iav, (1 << info.id)) <0) {
			iav_error("Invalid buffer configuration!\n");
			return -EFAULT;
		}
		if (check_stream_config_limit(iav, (1 << info.id)) <0) {
			iav_error("Invalid stream configuration!\n");
			return -EFAULT;
		}
		if (iav_check_overlay_param(stream, &info) < 0) {
			iav_error("Invalid overlay insert area format!\n");
			return -EFAULT;
		}
	}

	mutex_lock(&stream->osd_mutex);
	if (info.enable && is_stream_in_idle(stream)) {
		/* Insert encode size setup command for overlay only
		* when stream is not started yet. */
		cmd_encode_size_setup(stream, NULL);
	}
	cmd_overlay_insert(stream, NULL, &info);
	stream->osd = info;

	mutex_unlock(&stream->osd_mutex);

	return 0;
}

static int iav_ioc_g_overlay_insert(struct ambarella_iav *iav,
	struct iav_overlay_insert __user * arg)
{
	int i;
	struct iav_stream * stream;
	struct iav_overlay_insert info;
	int max_stream_num;

	if (copy_from_user(&info, arg, sizeof(info)))
		return -EFAULT;

	max_stream_num = iav->system_config[iav->encode_mode].max_stream_num;

	if (info.id >= max_stream_num) {
		iav_error("Invalid stream ID: %d!\n", info.id);
		return -EINVAL;
	}
	stream = &iav->stream[info.id];

	mutex_lock(&stream->osd_mutex);

	info.enable = stream->osd.enable;
	for (i = 0; i < MAX_NUM_OVERLAY_AREA; ++i) {
		info.area[i] = stream->osd.area[i];
	}
	info.osd_insert_always = stream->osd.osd_insert_always;

	mutex_unlock(&stream->osd_mutex);

	return copy_to_user(arg, &info, sizeof(info)) ? -EFAULT : 0;
}

int iav_overlay_resume(struct ambarella_iav *iav, struct iav_stream *stream)
{
	if (!is_enc_work_state(iav)) {
		iav_error("IAV must be in PREVIEW or ENCODE for overlay.\n");
		return -1;
	}

	mutex_lock(&stream->osd_mutex);

	cmd_overlay_insert(stream, NULL, &stream->osd);

	mutex_unlock(&stream->osd_mutex);

	return 0;
}

static int iav_cfg_vproc_cawarp(struct ambarella_iav *iav,
	struct iav_ca_warp *cawarp)
{
	iav_debug("CA warp will be supported later!\n");

	return 0;
}

int iav_ioc_cfg_vproc(struct ambarella_iav * iav, void __user * arg)
{
	struct iav_video_proc vproc;
	int rval = 0;

	if (copy_from_user(&vproc, arg, sizeof(vproc)))
		return -EFAULT;

	mutex_lock(&iav->iav_mutex);

	switch (vproc.cid) {
	case IAV_VIDEO_PROC_DPTZ:
		rval = iav_cfg_vproc_dptz(iav, &vproc.arg.dptz);
		break;
	case IAV_VIDEO_PROC_PM:
		rval = iav_cfg_vproc_pm(iav, &vproc.arg.pm);
		break;
	case IAV_VIDEO_PROC_CAWARP:
		rval = iav_cfg_vproc_cawarp(iav, &vproc.arg.cawarp);
		break;
	default:
		iav_error("Unsupported cid %d for video proc!\n", vproc.cid);
		break;
	}

	mutex_unlock(&iav->iav_mutex);

	return 0;
}

int iav_ioc_get_vproc(struct ambarella_iav * iav, void __user * arg)
{
	struct iav_video_proc vproc;

	if (copy_from_user(&vproc, arg, sizeof(vproc)))
		return -EFAULT;

	mutex_lock(&iav->iav_mutex);

	switch (vproc.cid) {
	case IAV_VIDEO_PROC_DPTZ:
		vproc.arg.dptz.buf_id = IAV_SRCBUF_MN;
		vproc.arg.dptz.input = iav->srcbuf[IAV_SRCBUF_MN].input;
		break;
	case IAV_VIDEO_PROC_PM:
		vproc.arg.pm = iav->pm;
		break;
	case IAV_VIDEO_PROC_CAWARP:
	default:
		iav_error("Unsupported cid %d for video proc!\n", vproc.cid);
		break;
	}

	mutex_unlock(&iav->iav_mutex);

	return copy_to_user(arg, &vproc, sizeof(vproc)) ? -EFAULT : 0;
}

int iav_ioc_apply_vproc(struct ambarella_iav * iav, void __user * arg)
{
	struct iav_apply_flag flags[IAV_VIDEO_PROC_NUM];
	int pm_latency = 0;
	u32 pm_unit;

	if (copy_from_user(flags, arg, sizeof(flags)))
			return -EFAULT;

	if (!is_enc_work_state(iav)) {
		iav_error("Apply vproc should be in PREVIEW/ENCODE state.\n");
		return -1;
	}

	switch(iav->encode_mode) {
	case DSP_NORMAL_ISO_MODE:
	case DSP_BLEND_ISO_MODE:
		pm_latency = 0;
		break;
	case DSP_ADVANCED_ISO_MODE:
	case DSP_HDR_LINE_INTERLEAVED_MODE:
		pm_latency = 1;
		break;
	default:
		iav_error("Incorrect encode mode %d for apply vproc.\n",
			iav->encode_mode);
		return -1;
	};

	mutex_lock(&iav->iav_mutex);

	if (flags[IAV_VIDEO_PROC_DPTZ].apply &&
		(flags[IAV_VIDEO_PROC_DPTZ].param & (1 << IAV_SRCBUF_MN))) {
		cmd_set_warp_control(iav, NULL);
	}

	if (flags[IAV_VIDEO_PROC_PM].apply) {
		pm_unit = get_pm_unit(iav);
		if (pm_unit == IAV_PM_UNIT_MB) {
			iav_set_pm_mctf(iav, &iav->pm, pm_latency);
		} else {
			iav_set_pm_bpc(iav, &iav->pm);
		}
	}

	mutex_unlock(&iav->iav_mutex);

	return 0;
}

static int iav_check_fastosd_param(struct iav_stream *stream,
	struct iav_fastosd_insert *info)
{
	int i, width, height;
	struct iav_fastosd_string_area *string_area;

	iav_no_check();

	if (info->string_num_region > MAX_NUM_FASTOSD_STRING_AREA) {
		iav_error("[fastosd]: string region(%d) exceed max\n",
			info->string_num_region);
		return -1;
	}
	if (info->osd_num_region > MAX_NUM_FASTOSD_OSD_AREA) {
		iav_error("[fastosd]: osd region(%d) exceed max\n",
			info->osd_num_region);
		return -1;
	}

	width = stream->format.enc_win.width;
	height = stream->format.enc_win.height;
	for (i = 0; i < MAX_NUM_FASTOSD_STRING_AREA; ++i) {
		string_area = &info->string_area[i];
		if (string_area->enable) {
			if (!string_area->string_output_pitch || !string_area->string_length) {
				iav_error("fastosd string area pitch/string length cannot be zero.\n");
				return -1;
			}
			if (string_area->string_length > MAX_FASTOSD_STRING_LENGTH) {
				iav_error("fastosd string length [%d] must not be greater than max [%d].\n",
					string_area->string_length, MAX_FASTOSD_STRING_LENGTH);
				return -1;
			}
			if ((info->font_map_height + string_area->offset_y) > height) {
				iav_error("fastosd height + offset_y [%d, %d] exceed stream height [%d].\n",
					info->font_map_height, string_area->offset_y, height);
				return -1;
			}
			if (string_area->overall_string_width > string_area->string_output_pitch) {
				iav_error("fastosd string width [%d] cannot be greater than pitch [%d].\n",
					string_area->overall_string_width, string_area->string_output_pitch);
				return -1;
			}
			if ((string_area->overall_string_width + string_area->offset_x) > width) {
				iav_error("string_area->overall_string_width + offset_x "
					"[%d, %d] excced stream->width [%d].\n",
					string_area->overall_string_width, string_area->offset_x, width);
				return -1;
			}
		}
	}

	return 0;
}

static int iav_ioc_s_fastosd_insert(struct ambarella_iav *iav,
	struct iav_fastosd_insert __user * arg)
{
	struct iav_stream * stream;
	struct iav_fastosd_insert info;
	int max_stream_num;

	if (copy_from_user(&info, arg, sizeof(info)))
		return -EFAULT;

	max_stream_num = iav->system_config[iav->encode_mode].max_stream_num;
	if (info.id >= max_stream_num) {
		iav_error("Invalid stream ID: %d!\n", info.id);
		return -EINVAL;
	}
	stream = &iav->stream[info.id];

	if (info.enable) {
		if (check_buffer_config_limit(iav, (1 << info.id)) < 0) {
			iav_error("Invalid buffer configuration!\n");
			return -EFAULT;
		}
		if (check_stream_config_limit(iav, (1 << info.id)) < 0) {
			iav_error("Invalid stream configuration!\n");
			return -EFAULT;
		}
		if (iav_check_fastosd_param(stream, &info) < 0) {
			iav_error("check_fastosd_param fail!\n");
			return -EFAULT;
		}
	}

	mutex_lock(&stream->osd_mutex);
	if (info.enable && is_stream_in_idle(stream)) {
		/* Insert encode size setup command for overlay only
		* when stream is not started yet. */
		cmd_encode_size_setup(stream, NULL);
	}
	cmd_fastosd_insert(stream, NULL, &info);
	stream->fastosd = info;

	mutex_unlock(&stream->osd_mutex);

	return 0;
}

static int iav_ioc_g_fastosd_insert(struct ambarella_iav *iav,
	struct iav_fastosd_insert __user * arg)
{
	struct iav_stream * stream;
	struct iav_fastosd_insert info;
	int max_stream_num;

	if (copy_from_user(&info, arg, sizeof(info)))
		return -EFAULT;

	max_stream_num = iav->system_config[iav->encode_mode].max_stream_num;

	if (info.id >= max_stream_num) {
		iav_error("Invalid stream ID: %d!\n", info.id);
		return -EINVAL;
	}
	stream = &iav->stream[info.id];

	mutex_lock(&stream->osd_mutex);

	info = stream->fastosd;
	info.enable = stream->fastosd.enable;

	mutex_unlock(&stream->osd_mutex);

	return copy_to_user(arg, &info, sizeof(info)) ? -EFAULT : 0;
}

int iav_set_dsplog_debug_level(struct ambarella_iav *iav,
	struct iav_dsplog_setup *dsplog_setup)
{
	if (dsplog_setup->args[1] > 3) {
		iav_debug("Set too high debug level, reduce to highest level [3].\n");
		dsplog_setup->args[1] = 3;
	}

	cmd_dsp_set_debug_level(iav, NULL, dsplog_setup->args[0],
		dsplog_setup->args[1], dsplog_setup->args[2]);
	return 0;
}

static int iav_ioc_s_dsplog(struct ambarella_iav *iav, void __user *arg)
{
	u32 param = (u32)arg;
	struct iav_dsplog_setup dsplog_setup;

	memset(&dsplog_setup, 0, sizeof(struct iav_dsplog_setup));
	dsplog_setup.args[0] = param & 0xFF; // module
	dsplog_setup.args[1] = (param >> 8) & 0xFF; // debug level
	dsplog_setup.args[2] = (param >> 16) & 0xFF; // mask
	iav_set_dsplog_debug_level(iav, &dsplog_setup);
	memcpy(&iav->dsplog_setup, &dsplog_setup, sizeof(struct iav_dsplog_setup));

	return 0;
}

#ifdef CONFIG_AMBARELLA_IAV_GUARD_VSYNC_LOSS
static int iav_guard_task(void *arg)
{
	struct ambarella_iav *iav = (struct ambarella_iav *)arg;

	while (1) {
		if (!iav->err_vsync_again) {
			if (!iav->err_vsync_handling) {
				wait_event_interruptible(iav->err_wq, iav->err_vsync_lost);
			}
		}
		if (!iav->err_vsync_handling) {
			mutex_lock(&iav->iav_mutex);
			iav->err_vsync_handling = 1;
			if (iav->nl_obj[NL_OBJ_VSYNC].nl_connected) {
				nl_send_request(&iav->nl_obj[NL_OBJ_VSYNC], NL_REQ_VSYNC_RESTORE);
				if(is_nl_request_responded(&iav->nl_obj[NL_OBJ_VSYNC],
					NL_REQ_VSYNC_RESTORE)) {
					iav->err_vsync_handling = 0;
				}
			} else {
				iav->err_vsync_handling = 0;
			}
			mutex_unlock(&iav->iav_mutex);
		} else {
			if(is_nl_request_responded(&iav->nl_obj[NL_OBJ_VSYNC],
				NL_REQ_VSYNC_RESTORE)) {
				iav->err_vsync_handling = 0;
			}
		}
	}

	return 0;
}
#endif

static int iav_guard_init(struct ambarella_iav *iav)
{
	init_waitqueue_head(&iav->err_wq);
	iav->err_vsync_again = 0;
	iav->err_vsync_handling = 0;
	iav->err_vsync_lost = 0;

#ifdef CONFIG_AMBARELLA_IAV_GUARD_VSYNC_LOSS
	kthread_run(iav_guard_task, iav, "iav_guard");
#endif

	return 0;
}

void iav_encode_exit(struct ambarella_iav *iav)
{
	if (iav->dsp_enc_data_virt) {
		iounmap((void __iomem *)iav->dsp_enc_data_virt);
	}

	if (iav->bsh_virt) {
		if (iav->probe_state == IAV_STATE_INIT) {
			dma_free_writecombine(NULL, iav->bsh_size, iav->bsh_virt, iav->bsh_phys);
		} else {
			iounmap((void __iomem *)iav->bsh_virt);
		}
		iav->bsh_virt = NULL;
	}

	if (iav->frame_queue_virt) {
		kfree(iav->frame_queue_virt);
		iav->frame_queue_virt = NULL;
	}
}

/* only run iav_encode_init once */
int iav_encode_init(struct ambarella_iav *iav)
{
	struct iav_frame_desc *frame_desc;
	int i;

	INIT_LIST_HEAD(&iav->frame_queue);
	INIT_LIST_HEAD(&iav->frame_free);
	init_waitqueue_head(&iav->frame_wq);
	init_waitqueue_head(&iav->mv_wq);

	iav->system_config = &G_system_config[0];
	iav->raw_enc.raw_size.width = MAX_WIDTH_IN_FULL_FPS;
	iav->raw_enc.raw_size.height = MAX_HEIGHT_IN_FULL_FPS;
	iav->raw_enc.cmd_send_flag = 0;
	init_waitqueue_head(&iav->raw_enc.raw_wq);

	/* Set up the frame descriptor list */
	iav->frame_queue_virt = (struct iav_frame_desc *)
		kzalloc(sizeof(struct iav_frame_desc) * NUM_BS_DESC, GFP_KERNEL);
	if (!iav->frame_queue_virt) {
		iav_error("No memory for frame desc!\n");
		goto encode_init_exit;
	}
	frame_desc = iav->frame_queue_virt;
	for (i = 0; i < NUM_BS_DESC; i++, ++frame_desc) {
		list_add_tail(&frame_desc->node, &iav->frame_free);
	}
	iav->frame_cnt.free_frame_cnt = NUM_BS_DESC;

	/* allocate and setup bit stream descriptors */
	if (iav->probe_state == IAV_STATE_INIT) {
		/* Fixme: To be removed. IAV is installed from INIT state */
		iav->bsh_size = PAGE_ALIGN(NUM_BS_DESC * sizeof(BIT_STREAM_HDR));
		iav->bsh_virt = dma_alloc_writecombine(NULL, iav->bsh_size,
			&iav->bsh_phys, GFP_KERNEL);
		if (!iav->bsh_virt) {
			iav_error("Out of memory for bits info !\n");
			goto encode_init_exit;
		}
		iav->bsh = iav->bsh_virt;
	} else {
		/* IAV is installed from other states:
		 * Fast boot: IAV is installed from preview / encode state. */
		iav->bsh_size = DSP_BSH_SIZE;
		iav->bsh_phys = DSP_BSH_START;
		iav->bsh_virt = ioremap_nocache(iav->bsh_phys, iav->bsh_size);
		if (!iav->bsh_virt) {
			iav_error("Failed to call ioremap() for BSH.\n");
			goto encode_init_exit;
		}
		iav->bsh = iav->bsh_virt;
	}

	init_waitqueue_head(&iav->dsp_map_wq);
	init_waitqueue_head(&iav->hash_wq);

	mutex_init(&iav->hash_mutex);
	iav->wait_hash_msg = 0;
	iav->hash_msg_cnt = 0;

	iav_guard_init(iav);

	return 0;

encode_init_exit:

	iav_encode_exit(iav);

	return -ENOMEM;
}

int iav_init_isr(struct ambarella_iav *iav)
{
	dsp_init_data_t *dsp_init_data;
	u32 data_addr_phys;
	int size;

	/* Set up encode interrupt handler */
	iav->dsp->msg_callback[CAT_ENC] = irq_iav_queue_frame;
	iav->dsp->msg_data[CAT_ENC] = iav;
	iav->dsp->msg_callback[CAT_VCAP] = handle_vcap_msg;
	iav->dsp->msg_data[CAT_VCAP] = iav;
	iav->dsp_used_dram = 0;
	// map dsp encode data table
	dsp_init_data = iav->dsp->get_dsp_init_data(iav->dsp);
	data_addr_phys = DSP_TO_PHYS(dsp_init_data->default_binary_data_ptr);
	size = PAGE_ALIGN(sizeof(default_enc_binary_data_t));
	iav->dsp_enc_data_virt = ioremap_nocache(data_addr_phys, size);
	if (!iav->dsp_enc_data_virt) {
		iav_error("Failed to call ioremap() for dsp_enc_data_virt.\n");
		iav_encode_exit(iav);
		return -ENOMEM;
	}

	return 0;
}

static int iav_check_gdma_copy(struct ambarella_iav *iav, struct iav_gdma_copy *param)
{
	iav_no_check();

	if ((param->src_offset & 0x3) || (param->dst_offset & 0x3)) {
		iav_error("src_offset or dst_offset not algin to 4bytes.\n");
		return -1;
	}
	if ((param->src_offset > iav->mmap[param->src_mmap_type].size) ||
		(param->src_offset < 0)) {
		iav_error("Out of range: [usr] src_offset: 0x%x, not in [0, 0x%x]\n",
			param->src_offset, iav->mmap[param->src_mmap_type].size);
		return -1;
	}
	if ((param->dst_offset > iav->mmap[param->dst_mmap_type].size) ||
		(param->dst_offset < 0)) {
		iav_error("Out of range: [usr] dst_offset: 0x%x, not in [0, 0x%x]\n",
			param->dst_offset, iav->mmap[param->dst_mmap_type].size);
		return -1;
	}
	if (((param->src_offset + param->src_pitch * param->height) >
			iav->mmap[param->src_mmap_type].size) ||
		((param->dst_offset + param->dst_pitch * param->height) >
			iav->mmap[param->dst_mmap_type].size)) {
		iav_error("There is not enough phys space\n");
		return -1;
	}
	if (param->dst_mmap_type == param->src_mmap_type) {
		if (param->dst_offset >= param->src_offset) {
			if (param->dst_offset < param->src_offset +
					param->src_pitch * param->height) {
				iav_error("Overlap [usr] src_offset: 0x%x, dst_offset: 0x%x, "
					"size: 0x%08x\n", param->src_offset, param->dst_offset,
					param->src_pitch * param->height);
				return -1;
			}
		} else {
			if (param->src_offset < param->dst_offset +
					param->dst_pitch * param->height) {
				iav_error("Overlap [usr] src_offset: 0x%x, dst_offset: 0x%x, "
					"size: 0x%08x\n", param->src_offset, param->dst_offset,
					param->dst_pitch * param->height);
				return -1;
			}
		}
	}

	return 0;
}

int iav_ioc_gmda_copy(struct ambarella_iav *iav, void __user *arg)
{
	int rval = 0;
	struct iav_gdma_copy param;
	struct iav_gdma_param gdma;
	struct iav_mem_block *src = NULL, *dst = NULL;

	if (copy_from_user(&param, arg, sizeof(param)))
		return -EFAULT;

	if (iav_check_gdma_copy(iav, &param) < 0)
		return -EINVAL;

	memset(&gdma, 0 , sizeof(gdma));
	src = &iav->mmap[param.src_mmap_type];
	dst = &iav->mmap[param.dst_mmap_type];

	mutex_lock(&iav->iav_mutex);
	/* Fixme, the mmap type is non cached in iav_create_mmap_table */
	gdma.src_non_cached = 1;
	gdma.dest_non_cached = 1;
	gdma.dest_phys_addr = dst->phys + param.dst_offset;
	gdma.dest_virt_addr = dst->virt + param.dst_offset;
	gdma.src_phys_addr = src->phys + param.src_offset;
	gdma.src_virt_addr = src->virt + param.src_offset;
	gdma.src_pitch =  param.src_pitch;
	gdma.dest_pitch = param.dst_pitch;
	gdma.width = param.width;
	gdma.height = param.height;
	if (dma_pitch_memcpy(&gdma) < 0) {
		iav_error("dma_pitch_memcpy error\n");
		rval = -EFAULT;
	}
	mutex_unlock(&iav->iav_mutex);

	return rval;
}

int iav_encode_ioctl(struct ambarella_iav *iav, unsigned int cmd, unsigned long arg)
{
	int rval = 0;

	switch (cmd) {
	/* DSP & driver ioctl */
	case IAV_IOC_GET_CHIP_ID:
		rval = iav_ioc_g_chip_id(iav, (void __user *)arg);
		break;
	case IAV_IOC_GET_DRIVER_INFO:
		rval = iav_ioc_g_drvinfo((void __user *)arg);
		break;
	case IAV_IOC_SET_DSP_LOG:
		rval = iav_ioc_s_dsplog(iav, (void __user *)arg);
		break;
	case IAV_IOC_DRV_DSP_INFO:
		rval = iav_ioc_g_drvdspinfo(iav, (void __user *)arg);
		break;
	case IAV_IOC_GET_DSP_HASH:
		rval = iav_ioc_get_dsp_hash(iav, (void __user *)arg);
		break;
	case IAV_IOC_SET_DSP_CLOCK_STATE:
		rval = iav_ioc_s_dsp_clock_state(iav, (void __user *)arg);
		break;

	/* state ioctl */
	case IAV_IOC_WAIT_NEXT_FRAME:
		rval = iav_ioc_wait_next_frame(iav);
		break;
	case IAV_IOC_GET_IAV_STATE:
		rval = iav_ioc_g_iavstate(iav, (void __user *)arg);
		break;
	case IAV_IOC_ENTER_IDLE:
		rval = iav_goto_idle(iav);
		break;
	case IAV_IOC_ENABLE_PREVIEW:
		rval = iav_enable_preview(iav);
		break;
	case IAV_IOC_START_ENCODE:
		rval = iav_ioc_start_encode(iav, (void __user *)arg);
		break;
	case IAV_IOC_STOP_ENCODE:
		rval = iav_ioc_stop_encode(iav, (void __user *)arg);
		break;
	case IAV_IOC_ABORT_ENCODE:
		rval = iav_ioc_abort_encode(iav, (void __user *)arg);
		break;

	/* system ioctl */
	case IAV_IOC_SET_SYSTEM_RESOURCE:
		rval = iav_ioc_s_system_resource(iav, (void __user *)arg);
		break;
	case IAV_IOC_GET_SYSTEM_RESOURCE:
		rval = iav_ioc_g_system_resource(iav, (void __user *)arg);
		break;
	case IAV_IOC_QUERY_ENC_MODE_CAP:
		rval = iav_ioc_q_enc_mode_cap(iav, (void __user *)arg);
		break;
	case IAV_IOC_QUERY_ENC_BUF_CAP:
		rval = iav_ioc_q_enc_buf_cap(iav, (void __user *)arg);
		break;
	case IAV_IOC_GDMA_COPY:
		rval = iav_ioc_gmda_copy(iav, (void __user *)arg);
		break;

	/* read data ioctl */
	case IAV_IOC_QUERY_BUF:
		rval = iav_ioc_g_query_buf(iav, (void __user *)arg);
		break;
	case IAV_IOC_QUERY_DESC:
		rval = iav_ioc_g_query_desc(iav, (void __user *)arg);
		break;
	case IAV_IOC_QUERY_SUB_BUF:
		rval = iav_ioc_g_query_subbuf(iav, (void __user *)arg);
		break;

	/* source buffer ioctl */
	case IAV_IOC_SET_SOURCE_BUFFER_SETUP:
		rval = iav_ioc_s_srcbuf_setup(iav, (void __user *)arg);
		break;
	case IAV_IOC_GET_SOURCE_BUFFER_SETUP:
		rval = iav_ioc_g_srcbuf_setup(iav, (void __user *)arg);
		break;
	case IAV_IOC_SET_SOURCE_BUFFER_FORMAT:
		rval = iav_ioc_s_srcbuf_format(iav, (void __user *)arg);
		break;
	case IAV_IOC_GET_SOURCE_BUFFER_FORMAT:
		rval = iav_ioc_g_srcbuf_format(iav, (void __user *)arg);
		break;
	case IAV_IOC_SET_DIGITAL_ZOOM:
		rval = iav_ioc_s_dptz(iav, (void __user *)arg);
		break;
	case IAV_IOC_GET_DIGITAL_ZOOM:
		rval = iav_ioc_g_dptz(iav, (void __user *)arg);
		break;
	case IAV_IOC_GET_SOURCE_BUFFER_INFO:
		rval = iav_ioc_g_srcbuf_info(iav, (void __user *)arg);
		break;

	/* stream ioctl */
	case IAV_IOC_SET_STREAM_FORMAT:
		rval = iav_ioc_s_stream_format(iav, (void __user *)arg);
		break;
	case IAV_IOC_GET_STREAM_FORMAT:
		rval = iav_ioc_g_stream_format(iav, (void __user *)arg);
		break;
	case IAV_IOC_GET_STREAM_INFO:
		rval = iav_ioc_g_stream_info(iav, (void __user *)arg);
		break;
	case IAV_IOC_SET_STREAM_CONFIG:
		rval = iav_ioc_s_stream_cfg(iav, (void __user *)arg);
		break;
	case IAV_IOC_GET_STREAM_CONFIG:
		rval = iav_ioc_g_stream_cfg(iav, (void __user *)arg);
		break;
	case IAV_IOC_SET_STREAM_FPS_SYNC:
		rval = iav_ioc_s_stream_fps_sync(iav, (void __user *)arg);
		break;
	case IAV_IOC_SET_H264_CONFIG:
		rval = iav_ioc_s_h264_cfg(iav, (void __user *)arg);
		break;
	case IAV_IOC_GET_H264_CONFIG:
		rval = iav_ioc_g_h264_cfg(iav, (void __user *)arg);
		break;
	case IAV_IOC_SET_MJPEG_CONFIG:
		rval = iav_ioc_s_mjpeg_cfg(iav, (void __user *)arg);
		break;
	case IAV_IOC_GET_MJPEG_CONFIG:
		rval = iav_ioc_g_mjpeg_cfg(iav, (void __user *)arg);
		break;

	/* encode ioctl */
	case IAV_IOC_SET_RAW_ENCODE:
		rval = iav_ioc_s_raw_enc_setup(iav, (void __user *)arg);
		break;
	case IAV_IOC_GET_RAW_ENCODE:
		rval = iav_ioc_g_raw_enc_setup(iav, (void __user *)arg);
		break;
	case IAV_IOC_WAIT_RAW_ENCODE:
		rval = iav_ioc_wait_raw_enc(iav, (void __user *)arg);
		break;

	/* overlay ioctl */
	case IAV_IOC_SET_OVERLAY_INSERT:
		rval = iav_ioc_s_overlay_insert(iav, (void __user *)arg);
		break;
	case IAV_IOC_GET_OVERLAY_INSERT:
		rval = iav_ioc_g_overlay_insert(iav, (void __user *)arg);
		break;

	/* fastosd ioctl */
	case IAV_IOC_SET_FASTOSD_INSERT:
		rval = iav_ioc_s_fastosd_insert(iav, (void __user *)arg);
		break;
	case IAV_IOC_GET_FASTOSD_INSERT:
		rval = iav_ioc_g_fastosd_insert(iav, (void __user *)arg);
		break;

	/* privacy mask ioctl */
	case IAV_IOC_GET_PRIVACY_MASK_INFO:
		rval = iav_ioc_g_pm_info(iav, (void __user *)arg);
		break;
	case IAV_IOC_SET_PRIVACY_MASK:
		rval = iav_ioc_s_pm(iav, (void __user *)arg);
		break;
	case IAV_IOC_GET_PRIVACY_MASK:
		rval = iav_ioc_g_pm(iav, (void __user *)arg);
		break;
	case IAV_IOC_SET_DIGITAL_ZOOM_PRIVACY_MASK:
		iav_error("This API is not implemented yet.!\n");
		break;

	/* warp ioctl */
	case IAV_IOC_CFG_WARP_CTRL:
		rval = iav_ioc_c_warp_ctrl(iav, (void __user *)arg);
		break;
	case IAV_IOC_GET_WARP_CTRL:
		rval = iav_ioc_g_warp_ctrl(iav, (void __user *)arg);
		break;
	case IAV_IOC_APPLY_WARP_CTRL:
		rval = iav_ioc_a_warp_ctrl(iav, (void __user *)arg);
		break;

	/* bad pixel correction ioctl */
	case IAV_IOC_CFG_BPC_SETUP:
		rval = iav_ioc_s_bpc_setup(iav, (void __user *)arg);
		break;
	case IAV_IOC_APPLY_STATIC_BPC:
		rval = iav_ioc_s_static_bpc(iav, (void __user *)arg);
		break;

	/* frame sync ioctl */
	case IAV_IOC_CFG_FRAME_SYNC_PROC:
		rval = iav_ioc_cfg_frame_sync(iav, (void __user *)arg);
		break;
	case IAV_IOC_GET_FRAME_SYNC_PROC:
		rval = iav_ioc_get_frame_sync(iav, (void __user *)arg);
		break;
	case IAV_IOC_APPLY_FRAME_SYNC_PROC:
		rval = iav_ioc_apply_frame_sync(iav, (void __user *)arg);
		break;

	case IAV_IOC_CFG_VIDEO_PROC:
		rval = iav_ioc_cfg_vproc(iav, (void __user *)arg);
		break;
	case IAV_IOC_GET_VIDEO_PROC:
		rval = iav_ioc_get_vproc(iav, (void __user *)arg);
		break;
	case IAV_IOC_APPLY_VIDEO_PROC:
		rval = iav_ioc_apply_vproc(iav, (void __user *)arg);
		break;

	/* efm ioctl */
	case IAV_IOC_EFM_GET_POOL_INFO:
		rval = iav_ioc_efm_get_pool_info(iav, (void __user *)arg);
		break;
	case IAV_IOC_EFM_REQUEST_FRAME:
		rval = iav_ioc_efm_request_frame(iav, (void __user *)arg);
		break;
	case IAV_IOC_EFM_HANDSHAKE_FRAME:
		rval = iav_ioc_efm_handshake_frame(iav, (void __user *)arg);
		break;

	/* IAV test */
	case IAV_IOC_SET_DEBUG_CONFIG:
		rval = iav_ioc_s_debug_cfg(iav, (void __user *)arg);
		break;
	case IAV_IOC_GET_DEBUG_CONFIG:
		rval = iav_ioc_g_debug_cfg(iav, (void __user *)arg);
		break;

	/* Customized IAV IOCTL */
	case IAV_IOC_CUSTOM_CMDS:
		rval = iav_ioc_custom_cmds(iav, (void __user *)arg);
		break;

	default:
		iav_error("Not supported cmd [%x].\n", cmd);
		rval = -ENOIOCTLCMD;
		break;
	}

	return rval;
}

int iav_restore_dsp_cmd(struct ambarella_iav *iav)
{
	return parse_dsp_cmd_to_iav(iav);
}

