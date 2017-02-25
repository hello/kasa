/*
 * iav_dec_api.c
 *
 * History:
 *	2015/01/21 - [Zhi He] created file
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
#include "iav_dec_api.h"

#define	DIAV_DEC_MIN_GAP_IN_BSB		(256)
#define	DIAV_DEC_MAX_CMD_IN_TICK	(12)
#define	DCACHE_LINE_SIZE		(32)

#if 0
#define DEC_DEBUG(format, args...) (void)0
#define DEC_ERROR(format, args...) (void)0
#else
#define DEC_DEBUG(format, args...) 	do { \
		printk(KERN_DEBUG format,##args); \
	} while (0)

#define DEC_ERROR(format, args...) do { \
		printk("!!Error at file %s, function %s, line %d:\n", __FILE__, __FUNCTION__, __LINE__); \
		printk(format, ##args); \
	} while (0)
#endif

static u8 __is_interlace_vout_mode(u32 mode)
{
	switch (mode) {
		case AMBA_VIDEO_MODE_480I:
		case AMBA_VIDEO_MODE_576I:
		case AMBA_VIDEO_MODE_1080I48:
		case AMBA_VIDEO_MODE_1080I50:
		case AMBA_VIDEO_MODE_1080I:
		case AMBA_VIDEO_MODE_1080I60:
			return 1;
			break;
		default:
			break;
	}
	return 0;
}

static int __check_enter_decode_mode_params(struct iav_decode_mode_config* mode_config)
{
	if (!mode_config->num_decoder) {
		DEC_ERROR("num_decoder %d is zero\n",
			mode_config->num_decoder);
		return -EINVAL;
	}

	if (mode_config->num_decoder > DIAV_MAX_DECODER_NUMBER) {
		DEC_ERROR("num_decoder %d exceed max value %d\n",
			mode_config->num_decoder, DIAV_MAX_DECODER_NUMBER);
		return -EINVAL;
	}

	if (!mode_config->max_frm_width || !mode_config->max_frm_height) {
		DEC_ERROR("not specify max frame width %d or height %d\n",
			mode_config->max_frm_width, mode_config->max_frm_height);
		return -EINVAL;
	}

	return 0;
}

static int __check_decode_state_valid(struct ambarella_iav *iav, u8 decoder_id)
{
	if (decoder_id >= iav->decode_context.mode_config.num_decoder) {
		DEC_ERROR("bad decoder_id %d.\n", decoder_id);
		return -EINVAL;
	}

	if (!iav->decode_context.decoder_config[decoder_id].setup_done) {
		DEC_ERROR("decoder_id %d is not setup.\n", decoder_id);
		return -EPERM;
	}

	return 0;
}

static void __clean_cache_aligned(u32 kernel_start, u32 size)
{
	u32 offset = kernel_start & (DCACHE_LINE_SIZE - 1);
	kernel_start -= offset;
	size += offset;
	clean_d_cache((void*) kernel_start, size);
}

static void __clean_fifo_cache(u32 start_addr, u32 end_addr, u32 buffer_start, u32 buffer_end)
{
	if (end_addr > start_addr) {
		__clean_cache_aligned(start_addr, end_addr - start_addr);
	} else {
		u32 size = buffer_end - start_addr;
		if (size > 0) {
			__clean_cache_aligned(start_addr, size);
		}

		size = end_addr - buffer_start;
		if (size > 0) {
			__clean_cache_aligned(buffer_start, size);
		}
	}
}

static inline void __dec_notify_waiters(struct iav_decoder_current_status *decoder_status)
{
	while (decoder_status->num_waiters > 0) {
		decoder_status->num_waiters --;
		up(&decoder_status->sem);
	}
}

static int wait_dec_msg(struct ambarella_iav *iav,
	struct iav_decoder_current_status *dec_status)
{
	mutex_unlock(&iav->iav_mutex);
	if (down_interruptible(&dec_status->sem)) {
		spin_lock(&iav->iav_lock);
		dec_status->num_waiters --;
		spin_unlock(&iav->iav_lock);
		mutex_lock(&iav->iav_mutex);
		return -EINTR;
	}
	mutex_lock(&iav->iav_mutex);
	return 0;
}

static int wait_dec_state(struct ambarella_iav *iav,
	struct iav_decoder_current_status *dec_status, u32 target_state)
{
	int times = 10;

	while (times > 0) {
		spin_lock(&iav->iav_lock);

		if (dec_status->decode_state == target_state) {
			spin_unlock(&iav->iav_lock);
			DEC_DEBUG("[iav flow]: wait decode state to %d done\n", target_state);
			return 0;
		}

		dec_status->num_waiters ++;
		spin_unlock(&iav->iav_lock);

		if (wait_dec_msg(iav, dec_status) < 0) {
			DEC_ERROR("interrupt signal\n");
			return -EINTR;
		}

		times --;
	}

	DEC_ERROR("wait decode_state %d time out, cur state %d\n", target_state, dec_status->decode_state);
	return 1;
}

static void handle_dec_msg(void *data, DSP_MSG *msg)
{
	struct ambarella_iav *iav = data;
	struct iav_decoder_current_status *decoder_status = &iav->decode_context.decoder_current_status[0];
	decode_msg_t *p_dsp_msg = (decode_msg_t *) msg;

	spin_lock(&iav->iav_lock);

	if (decoder_status->decode_state != p_dsp_msg->decode_state) {
		DEC_DEBUG("[decode_state]: %d --> %d\n",
			decoder_status->decode_state, p_dsp_msg->decode_state);
		decoder_status->decode_state = p_dsp_msg->decode_state;
	}
	if (decoder_status->decode_state) {
		u32 error_status = p_dsp_msg->error_status & 0x7fffffff;
		decoder_status->current_bsb_read_offset = p_dsp_msg->h264_bits_fifo_next -
		decoder_status->current_bsb_addr_dsp_base;
		if (ERR_NO_4BYTEHEADER != error_status) {
			decoder_status->error_status = error_status;
		} else {
			decoder_status->error_status = 0;
		}
		decoder_status->total_error_count = p_dsp_msg->total_error_count;
		decoder_status->decoded_pic_number = p_dsp_msg->decoded_pic_number;
		decoder_status->last_pts = p_dsp_msg->latest_pts;
		decoder_status->yuv422_y_addr = p_dsp_msg->yuv422_y_addr;
		decoder_status->yuv422_uv_addr = p_dsp_msg->yuv422_uv_addr;
		decoder_status->accumulated_dec_cmd_number = 0;
	}

	decoder_status->irq_count ++;
	__dec_notify_waiters(decoder_status);

	spin_unlock(&iav->iav_lock);

	return;
}

static inline int set_dsp_to_decode_mode(struct ambarella_iav *iav)
{
	int ret = 0;
	struct dsp_device *dsp = iav->dsp;

	ret = dsp->set_op_mode(dsp, DSP_DECODE_MODE, NULL, 0);
	if (!ret) {
		iav->state = IAV_STATE_DECODING;
	} else {
		DEC_ERROR("enter decode mode fail, (iav state %d)\n", iav->state);
	}

	return ret;
}

static int setup_h264_decoder(struct ambarella_iav *iav, u8 decoder_id)
{
	struct dsp_device *dsp = iav->dsp;
	struct iav_decoder_info *decoder = &iav->decode_context.decoder_config[decoder_id];
	struct amb_dsp_cmd *first, *cmd;
	u8 i = 0;
	struct iav_decode_vout_config *pvout_config = NULL;
	u8 is_interlace = 0;

	if (1 != decoder->num_vout) {
		DEC_ERROR("only support single vout\n");
		decoder->num_vout = 1;
	}

	iav->decode_context.b_interlace_vout = __is_interlace_vout_mode(decoder->vout_configs[0].vout_mode);
	first = dsp->get_multi_cmds(dsp, 3 + decoder->num_vout, DSP_CMD_FLAG_BLOCK);
	if (!first) {
		DEC_ERROR("no memory\n");
		return -ENOMEM;
	}

	cmd = first;
	cmd_system_info_setup(iav, cmd, 1);
	get_next_cmd(cmd, first);
	cmd_h264_decoder_setup(iav, cmd, decoder_id);
	for (i = 0; i < decoder->num_vout; i ++) {
		pvout_config = &decoder->vout_configs[i];
		is_interlace = __is_interlace_vout_mode(pvout_config->vout_mode);
		get_next_cmd(cmd, first);
		cmd_vout_video_setup(iav, cmd, pvout_config, VOUT_SRC_DEC, is_interlace);
	}
	get_next_cmd(cmd, first);
	cmd_rescale_postp(iav, cmd, decoder_id, (decoder->width >> 1), (decoder->height >> 1));

	dsp->put_cmd(dsp, first, 0);

	return 0;
}

static void stop_decoder(struct ambarella_iav *iav, u8 decoder_id)
{
	struct iav_decoder_current_status* dec_status = NULL;

	dec_status = &iav->decode_context.decoder_current_status[decoder_id];
	if (dec_status->b_send_first_decode_cmd) {
		cmd_decode_stop(iav, decoder_id, 0);
		dec_status->b_send_stop_cmd = 1;
		wait_dec_state(iav, dec_status, HDEC_OPM_IDLE);
	}
}

static void stop_all_decoders(struct ambarella_iav *iav)
{
	u8 decoder_id = 0;
	for (decoder_id = 0; decoder_id < iav->decode_context.mode_config.num_decoder; ++decoder_id) {
		stop_decoder(iav, decoder_id);
	}
}

static void reset_decoder_status(struct iav_decoder_current_status* dec_status)
{
	dec_status->current_bsb_write_offset = 0;
	dec_status->current_bsb_read_offset = 0;
	dec_status->current_bsb_remain_free_size = dec_status->current_bsb_size;

	dec_status->last_pts = 0;
	dec_status->irq_count = 0;

	dec_status->accumulated_dec_cmd_number = 0;

	dec_status->speed = 0x100;
	dec_status->scan_mode = IAV_PB_SCAN_MODE_ALL_FRAMES;
	dec_status->direction = IAV_PB_DIRECTION_FW;

	dec_status->current_trickplay = 0;

	dec_status->is_started = 0;
	dec_status->b_send_first_decode_cmd = 0;
	dec_status->b_send_stop_cmd = 0;
}

static void init_decoder_setting(struct iav_decoder_info* dec_config,
	struct iav_decoder_current_status* dec_status, u8 decoder_id)
{
	dec_config->decoder_id = decoder_id;
	dec_config->decoder_type = IAV_DECODER_TYPE_INVALID;
	dec_config->num_vout = 0;
	dec_config->setup_done = 0;
	dec_config->bsb_start_offset = 0;
	dec_config->bsb_size = 0;

	dec_status->current_bsb_addr_dsp_base = 0;
	dec_status->current_bsb_addr_phy_base = 0;
	dec_status->current_bsb_size = 0;

	dec_status->current_bsb_write_offset = 0;
	dec_status->current_bsb_read_offset = 0;
	dec_status->current_bsb_remain_free_size = 0;

	dec_status->last_pts = 0;

	dec_status->num_waiters = 0;

	dec_status->accumulated_dec_cmd_number = 0;

	dec_status->speed = 0x100;
	dec_status->scan_mode = IAV_PB_SCAN_MODE_ALL_FRAMES;
	dec_status->direction = IAV_PB_DIRECTION_FW;

	dec_status->current_trickplay = 0;

	dec_status->is_started = 0;
	dec_status->b_send_first_decode_cmd = 0;
	dec_status->b_send_stop_cmd = 0;

	dec_status->error_status = 0;
	dec_status->total_error_count = 0;
	dec_status->decoded_pic_number = 0;

	dec_status->irq_count = 0;
	dec_status->yuv422_y_addr = 0;
	dec_status->yuv422_uv_addr = 0;
}

static void init_all_decoder_setting(struct ambarella_iav *iav)
{
	struct iav_decode_context* dec_context = &iav->decode_context;
	struct iav_decoder_current_status* dec_status = NULL;
	struct iav_decoder_info* dec_config = NULL;

	u8 decoder_id = 0;
	for (decoder_id = 0; decoder_id < DIAV_MAX_DECODER_NUMBER; decoder_id ++) {
		dec_status = &dec_context->decoder_current_status[decoder_id];
		dec_config = &dec_context->decoder_config[decoder_id];
		init_decoder_setting(dec_config, dec_status, decoder_id);
	}
}

static void init_decoder_status(struct iav_decoder_current_status *status)
{
	status->num_waiters = 0;
	sema_init(&status->sem, 0);

	status->accumulated_dec_cmd_number = 0;

	status->speed = 0x100;
	status->scan_mode = IAV_PB_SCAN_MODE_ALL_FRAMES;
	status->direction= IAV_PB_DIRECTION_FW;

	status->decode_state = 0;
	status->error_status = 0;
	status->total_error_count = 0;
	status->decoded_pic_number = 0;

	status->irq_count = 0;
	status->yuv422_y_addr = 0;
	status->yuv422_uv_addr = 0;

}

static int iav_enter_decode_mode(struct ambarella_iav *iav)
{
	int rval = 0;

	mutex_lock(&iav->iav_mutex);

	do {
		if (IAV_STATE_DECODING == iav->state) {
			DEC_DEBUG("already in decode mode.\n");
			break;
		} else if (IAV_STATE_INIT == iav->state) {
			rval = set_dsp_to_decode_mode(iav);
			if (rval) {
				break;
			}
			iav_config_vout(VOUT_SRC_DEC);
			DEC_DEBUG("[iav flow]: boot dsp to dec mode, iav state (%d)\n", iav->state);
		} else if (IAV_STATE_IDLE != iav->state) {
			DEC_ERROR("(%d) not in idle state, should goto idle first.\n", iav->state);
			rval = -EPERM;
			break;
		} else {
			DEC_DEBUG("[iav flow]: set dsp to decode mode, from iav state (%d)...\n", iav->state);
			rval = set_dsp_to_decode_mode(iav);
			if (rval) {
				break;
			}
			iav_config_vout(VOUT_SRC_DEC);
			DEC_DEBUG("[iav flow]: set dsp to decode mode done, iav state (%d)\n", iav->state);
		}
	} while (0);

	mutex_unlock(&iav->iav_mutex);

	return rval;
}

static int iav_ioc_enter_decode_mode(struct ambarella_iav *iav, void __user * arg)
{
	struct iav_decode_context* dec_context = &iav->decode_context;
	struct iav_decode_mode_config mode_config;
	int rval = 0;

	if (copy_from_user(&mode_config, arg, sizeof(struct iav_decode_mode_config))) {
		DEC_ERROR("copy_from_user fail.\n");
		return -EFAULT;
	}

	rval = __check_enter_decode_mode_params(&mode_config);
	if (rval) {
		return rval;
	}
	memcpy(&dec_context->mode_config, &mode_config, sizeof(mode_config));
	DEC_DEBUG("num %d, support ff %d, drpoc %d, max video %dx%d, vout0 %dx%d, vout1 %dx%d\n",
		dec_context->mode_config.num_decoder,
		dec_context->mode_config.b_support_ff_fb_bw, dec_context->mode_config.debug_use_dproc,
		dec_context->mode_config.max_frm_width, dec_context->mode_config.max_frm_height,
		dec_context->mode_config.max_vout0_width, dec_context->mode_config.max_vout0_height,
		dec_context->mode_config.max_vout1_width, dec_context->mode_config.max_vout1_height);

	rval = iav_enter_decode_mode(iav);

	return rval;
}

extern int iav_goto_timer_mode(struct ambarella_iav *iav);
static int iav_ioc_leave_decode_mode(struct ambarella_iav *iav)
{
	DEC_DEBUG("[iav flow]: leave decode_mode, from state (%d)\n", iav->state);
	if (IAV_STATE_DECODING == iav->state) {
		iav_clean_decode_stuff(iav);
		iav_goto_timer_mode(iav);
		DEC_DEBUG("[iav flow]: after goto idle (timer mode), state %d\n", iav->state);
	}
	return 0;
}

static int iav_ioc_create_decoder(struct ambarella_iav *iav, void __user * arg)
{
	struct iav_decode_context* dec_context = &iav->decode_context;
	struct iav_decoder_current_status * dec_status = NULL;
	struct iav_decoder_info decoder_info;

	mutex_lock(&iav->iav_mutex);

	if (IAV_STATE_DECODING != iav->state) {
		DEC_ERROR("not in decode mode.\n");
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	if (copy_from_user(&decoder_info, arg, sizeof(struct iav_decoder_info))) {
		DEC_ERROR("copy_from_user fail.\n");
		mutex_unlock(&iav->iav_mutex);
		return -EFAULT;
	}

	if (decoder_info.decoder_id >= dec_context->mode_config.num_decoder) {
		DEC_ERROR("bad decoder_id %d.\n", decoder_info.decoder_id);
		mutex_unlock(&iav->iav_mutex);
		return -EINVAL;
	}

	if ((decoder_info.width > dec_context->mode_config.max_frm_width)
		|| (decoder_info.height > dec_context->mode_config.max_frm_height)) {
		DEC_ERROR("exceed max video resolution %dx%d, max %dx%d.\n",
			decoder_info.width, decoder_info.height,
			dec_context->mode_config.max_frm_width, dec_context->mode_config.max_frm_height);
		mutex_unlock(&iav->iav_mutex);
		return -EINVAL;
	}

	if (dec_context->decoder_config[decoder_info.decoder_id].setup_done) {
		DEC_ERROR("decoder_id %d is already setup.\n", decoder_info.decoder_id);
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	dec_status = &dec_context->decoder_current_status[decoder_info.decoder_id];
	init_decoder_status(dec_status);

	if (IAV_DECODER_TYPE_H264 == decoder_info.decoder_type) {
		memcpy(&dec_context->decoder_config[decoder_info.decoder_id],
			&decoder_info, sizeof(struct iav_decoder_info));
		setup_h264_decoder(iav, decoder_info.decoder_id);
		dec_context->decoder_config[decoder_info.decoder_id].setup_done = 1;
	} else {
		DEC_ERROR("decoder_type %d not support.\n", decoder_info.decoder_type);
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	dec_status->current_bsb_addr_phy_base = iav->mmap[IAV_BUFFER_BSB].phys;
	dec_status->current_bsb_addr_dsp_base = PHYS_TO_DSP(iav->mmap[IAV_BUFFER_BSB].phys);
	dec_status->current_bsb_addr_virt_base = iav->mmap[IAV_BUFFER_BSB].virt;
	dec_status->current_bsb_addr_virt_end = iav->mmap[IAV_BUFFER_BSB].virt + iav->mmap[IAV_BUFFER_BSB].size;
	dec_status->current_bsb_size = iav->mmap[IAV_BUFFER_BSB].size;

#if 0
	DEC_DEBUG("[decoder, bsb]: size %d, %08x\n",
		dec_status->current_bsb_size, dec_status->current_bsb_size);
	DEC_DEBUG("[decoder, bsb]: virt %08x, phy %08x, dsp %08x\n",
		dec_status->current_bsb_addr_virt_base, dec_status->current_bsb_addr_phy_base,
		dec_status->current_bsb_addr_dsp_base);
#endif

	iav->dsp->msg_callback[CAT_DEC] = handle_dec_msg;
	iav->dsp->msg_data[CAT_DEC] = iav;

	decoder_info.bsb_start_offset = 0;
	decoder_info.bsb_size = dec_status->current_bsb_size;

	mutex_unlock(&iav->iav_mutex);

	if (copy_to_user(arg, &decoder_info, sizeof(decoder_info))) {
		DEC_ERROR("copy_to_user fail.\n");
		return -EFAULT;
	}

	DEC_DEBUG("[iav flow]: create decoder(%d)\n", decoder_info.decoder_id);

	return 0;
}

static int iav_ioc_destroy_decoder(struct ambarella_iav *iav, u8 decoder_id)
{
	mutex_lock(&iav->iav_mutex);

	if (IAV_STATE_DECODING != iav->state) {
		DEC_ERROR("not in decode mode.\n");
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	if (decoder_id >= iav->decode_context.mode_config.num_decoder) {
		DEC_ERROR("bad decoder_id %d.\n", decoder_id);
		mutex_unlock(&iav->iav_mutex);
		return -EINVAL;
	}

	if (!iav->decode_context.decoder_config[decoder_id].setup_done) {
		DEC_ERROR("decoder_id %d not setup yet.\n", decoder_id);
		mutex_unlock(&iav->iav_mutex);
		return -EINVAL;
	}

	stop_decoder(iav, decoder_id);

	iav->decode_context.decoder_config[decoder_id].setup_done = 0;
	DEC_DEBUG("[iav flow]: destory decoder(%d)\n", decoder_id);
	reset_decoder_status(&iav->decode_context.decoder_current_status[decoder_id]);

	mutex_unlock(&iav->iav_mutex);

	return 0;
}

static int iav_ioc_decode_start(struct ambarella_iav *iav, u8 decoder_id)
{
	struct iav_decode_context* dec_context = &iav->decode_context;
	int rval = 0;

	mutex_lock(&iav->iav_mutex);

	if (IAV_STATE_DECODING != iav->state) {
		DEC_ERROR("not in decode mode.\n");
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	rval = __check_decode_state_valid(iav, decoder_id);
	if (0 > rval) {
		mutex_unlock(&iav->iav_mutex);
		return rval;
	}

	reset_decoder_status(&dec_context->decoder_current_status[decoder_id]);
	dec_context->decoder_current_status[decoder_id].is_started = 1;
	DEC_DEBUG("[iav flow]: start decode(id %d)\n", decoder_id);

	mutex_unlock(&iav->iav_mutex);

	return 0;
}

static int iav_ioc_decode_stop(struct ambarella_iav *iav, void __user * arg)
{
	struct iav_decode_context* dec_context = &iav->decode_context;
	struct iav_decoder_current_status *dec_status = NULL;
	struct iav_decode_stop stop;
	int rval = 0;

	mutex_lock(&iav->iav_mutex);

	if (IAV_STATE_DECODING != iav->state) {
		DEC_ERROR("not in decode mode.\n");
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	if (copy_from_user(&stop, arg, sizeof(stop))) {
		DEC_ERROR("copy_from_user fail.\n");
		mutex_unlock(&iav->iav_mutex);
		return -EFAULT;
	}

	rval = __check_decode_state_valid(iav, stop.decoder_id);
	if (0 > rval) {
		mutex_unlock(&iav->iav_mutex);
		return rval;
	}

	dec_status = &dec_context->decoder_current_status[stop.decoder_id];
	if (!dec_status->is_started) {
		DEC_DEBUG("decode(%d) not started!\n", stop.decoder_id);
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	if (0 == dec_status->error_status) {
		if (!dec_status->b_send_stop_cmd) {
			/* always stop 1, stop 0 need re-setup decoder */
			stop.stop_flag = 1;
			cmd_decode_stop(iav, stop.decoder_id, stop.stop_flag);
			dec_status->b_send_stop_cmd = 1;
			if (0 == stop.stop_flag) {
				DEC_DEBUG("[iav flow]: stop decode(id %d, flag 0), wait dec state to 0\n", stop.decoder_id);
				wait_dec_state(iav, dec_status, HDEC_OPM_IDLE);
			} else {
				DEC_DEBUG("[iav flow]: stop decode(id %d, flag 1), wait dec state to 2\n", stop.decoder_id);
				wait_dec_state(iav, dec_status, HDEC_OPM_VDEC_IDLE);
			}
		} else {
			DEC_ERROR("already stopped.\n");
		}
	} else {
		if (!dec_status->b_send_stop_cmd) {
			/* stop 0, to clean errors */
			cmd_decode_stop(iav, stop.decoder_id, 0);
			dec_status->b_send_stop_cmd = 1;
			DEC_DEBUG("[iav flow]: stop decode(id %d, flag 0), wait dec state to 0\n", stop.decoder_id);
			wait_dec_state(iav, dec_status, HDEC_OPM_IDLE);
		} else {
			DEC_ERROR("already stopped.\n");
		}
	}

	//DEC_DEBUG("before awake waiters(%d), %p.\n", dec_context->decoder_current_status[stop.decoder_id].num_waiters, &dec_context->decoder_current_status[stop.decoder_id]);
	spin_lock(&iav->iav_lock);
	__dec_notify_waiters(dec_status);
	spin_unlock(&iav->iav_lock);
	DEC_DEBUG("after awake waiters.\n");

	mutex_unlock(&iav->iav_mutex);

	return 0;
}

static int iav_ioc_decode_video(struct ambarella_iav *iav, void __user * arg)
{
	struct iav_decode_context *dec_context = &iav->decode_context;
	struct iav_decode_video decode_video;
	struct iav_decoder_current_status *status = NULL;
	int rval = 0;

	mutex_lock(&iav->iav_mutex);

	if (IAV_STATE_DECODING != iav->state) {
		DEC_ERROR("not in decode mode.\n");
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	if (copy_from_user(&decode_video, arg, sizeof(struct iav_decode_video))) {
		DEC_ERROR("copy_from_user fail.\n");
		mutex_unlock(&iav->iav_mutex);
		return -EFAULT;
	}

	rval = __check_decode_state_valid(iav, decode_video.decoder_id);
	if (0 > rval) {
		mutex_unlock(&iav->iav_mutex);
		return rval;
	}

	status = &dec_context->decoder_current_status[decode_video.decoder_id];
	if (!status->is_started) {
		DEC_DEBUG("decode(%d) not started!\n", decode_video.decoder_id);
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	if (status->b_send_stop_cmd) {
		DEC_DEBUG("decode(%d) already stopped!\n", decode_video.decoder_id);
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	if (0 != status->error_status) {
		DEC_ERROR("decoder met error, status %d\n", status->error_status);
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	spin_lock(&iav->iav_lock);
	status->current_bsb_write_offset = decode_video.end_ptr_offset;
	spin_unlock(&iav->iav_lock);

	if (!status->b_send_first_decode_cmd) {
		status->b_send_first_decode_cmd = 1;
		DEC_DEBUG("[iav flow]: first decode cmd(%d), first pts %d\n",
			decode_video.decoder_id, decode_video.first_frame_display);
		__clean_fifo_cache(decode_video.start_ptr_offset + status->current_bsb_addr_virt_base,
			decode_video.end_ptr_offset + status->current_bsb_addr_virt_base,
			status->current_bsb_addr_virt_base, status->current_bsb_addr_virt_end);
		if (0 != decode_video.end_ptr_offset) {
			cmd_h264_decode(iav, NULL, decode_video.decoder_id,
				decode_video.start_ptr_offset + status->current_bsb_addr_dsp_base,
				decode_video.end_ptr_offset + status->current_bsb_addr_dsp_base - 1,
				decode_video.num_frames, decode_video.first_frame_display);
		} else {
			cmd_h264_decode(iav, NULL, decode_video.decoder_id,
				decode_video.start_ptr_offset + status->current_bsb_addr_dsp_base,
				status->current_bsb_size + status->current_bsb_addr_dsp_base - 1,
				decode_video.num_frames, decode_video.first_frame_display);
		}

		spin_lock(&iav->iav_lock);
		status->num_waiters++;
		spin_unlock(&iav->iav_lock);
		rval = wait_dec_msg(iav, status);

		mutex_unlock(&iav->iav_mutex);

		return 0;
	}

	spin_lock(&iav->iav_lock);
	if (status->accumulated_dec_cmd_number > DIAV_DEC_MAX_CMD_IN_TICK) {
		rval = 1;
	} else {
		rval = 0;
	}
	spin_unlock(&iav->iav_lock);

	if (rval) {
		spin_lock(&iav->iav_lock);
		status->num_waiters++;
		spin_unlock(&iav->iav_lock);
		//DEC_DEBUG("[iav flow]: too many cmd, wait next interrupt\n");
		rval = wait_dec_msg(iav, status);
		//DEC_DEBUG("[iav flow]: wait next interrupt done\n");
	}

	if (status->b_send_stop_cmd) {
		DEC_DEBUG("decode(%d) stopped!\n", decode_video.decoder_id);
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	__clean_fifo_cache(decode_video.start_ptr_offset + status->current_bsb_addr_virt_base,
		decode_video.end_ptr_offset + status->current_bsb_addr_virt_base,
		status->current_bsb_addr_virt_base, status->current_bsb_addr_virt_end);
	if (0 != decode_video.end_ptr_offset) {
		cmd_h264_decode_fifo_update(iav, NULL, decode_video.decoder_id,
			decode_video.start_ptr_offset + status->current_bsb_addr_dsp_base,
			decode_video.end_ptr_offset + status->current_bsb_addr_dsp_base - 1,
			decode_video.num_frames);
	} else {
		cmd_h264_decode_fifo_update(iav, NULL, decode_video.decoder_id,
			decode_video.start_ptr_offset + status->current_bsb_addr_dsp_base - 1,
			status->current_bsb_size + status->current_bsb_addr_dsp_base,
			decode_video.num_frames);
	}

	mutex_unlock(&iav->iav_mutex);

	spin_lock(&iav->iav_lock);
	status->accumulated_dec_cmd_number ++;
	spin_unlock(&iav->iav_lock);

	return 0;
}

static int iav_ioc_decode_jpeg(struct ambarella_iav *iav, void __user * arg)
{
	DEC_ERROR("not support decode jpeg now.\n");
	return -EPERM;
}

static int iav_ioc_wait_decode_bsb(struct ambarella_iav *iav, void __user * arg)
{
	struct iav_decode_bsb bsb;
	struct iav_decoder_current_status* status = NULL;
	int rval = 0;
	u32 room = 0;

	mutex_lock(&iav->iav_mutex);

	if (IAV_STATE_DECODING != iav->state) {
		DEC_ERROR("not in decode mode.\n");
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	if (copy_from_user(&bsb, arg, sizeof(struct iav_decode_bsb))) {
		DEC_ERROR("copy_from_user fail.\n");
		mutex_unlock(&iav->iav_mutex);
		return -EFAULT;
	}

	rval = __check_decode_state_valid(iav, bsb.decoder_id);
	if (0 > rval) {
		mutex_unlock(&iav->iav_mutex);
		return rval;
	}

	status = &iav->decode_context.decoder_current_status[bsb.decoder_id];

	if (0 != status->error_status) {
		DEC_ERROR("decoder met error, status 0x%08x, %d\n", status->error_status, status->error_status);
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	if ((bsb.room + DIAV_DEC_MIN_GAP_IN_BSB) >= status->current_bsb_size) {
		DEC_ERROR("request size (%d) + DIAV_DEC_MIN_GAP_IN_BSB "
			"exceed total bsb size (%d).\n", bsb.room, status->current_bsb_size);
		mutex_unlock(&iav->iav_mutex);
		return -EFAULT;
	}

	//DEC_DEBUG("[iav flow]: wait bsb start\n");

	while (1) {
		spin_lock(&iav->iav_lock);

		if (status->current_bsb_read_offset > status->current_bsb_write_offset) {
			room = status->current_bsb_read_offset - status->current_bsb_write_offset;
		} else {
			room = status->current_bsb_size + status->current_bsb_read_offset -
				status->current_bsb_write_offset;
		}

		if (room > (bsb.room + DIAV_DEC_MIN_GAP_IN_BSB)) {
			bsb.room = room - DIAV_DEC_MIN_GAP_IN_BSB;
			bsb.start_offset = status->current_bsb_write_offset;
			break;
		} else {
			//printk("[debug]: wait bsb: room %d, request room %d, status->num_waiters %d\n", room, bsb.room, status->num_waiters);
		}

		status->num_waiters ++;
		spin_unlock(&iav->iav_lock);

		//printk("[debug]: wait bsb: room %d, request room %d, status->num_waiters %d, %p\n", room, bsb.room, status->num_waiters, status);
		if (wait_dec_msg(iav, status) < 0) {
			mutex_unlock(&iav->iav_mutex);
			DEC_ERROR("interrupt signal\n");
			return -EINTR;
		}

		if (status->b_send_stop_cmd) {
			mutex_unlock(&iav->iav_mutex);
			DEC_DEBUG("stopped\n");
			return -EPERM;
		}
	}
	spin_unlock(&iav->iav_lock);

	mutex_unlock(&iav->iav_mutex);

	if (copy_to_user(arg, &bsb, sizeof(bsb))) {
		DEC_ERROR("copy_to_user fail.\n");
		return -EFAULT;
	}

	//DEC_DEBUG("[iav flow]: wait bsb end\n");

	return 0;
}

static int iav_ioc_decode_trick_play(struct ambarella_iav *iav, void __user * arg)
{
	struct iav_decode_trick_play trickplay;
	struct iav_decode_context* dec_context = &iav->decode_context;
	int rval = 0;

	mutex_lock(&iav->iav_mutex);

	if (IAV_STATE_DECODING != iav->state) {
		DEC_ERROR("not in decode mode.\n");
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	if (copy_from_user(&trickplay, arg, sizeof(struct iav_decode_trick_play))) {
		DEC_ERROR("copy_from_user fail.\n");
		mutex_unlock(&iav->iav_mutex);
		return -EFAULT;
	}

	rval = __check_decode_state_valid(iav, trickplay.decoder_id);
	if (0 > rval) {
		mutex_unlock(&iav->iav_mutex);
		return rval;
	}

	if (0 != dec_context->decoder_current_status[trickplay.decoder_id].error_status) {
		DEC_ERROR("decoder met error, status %d\n", dec_context->decoder_current_status[trickplay.decoder_id].error_status);
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	if (!dec_context->decoder_current_status[trickplay.decoder_id].is_started) {
		DEC_DEBUG("decode(%d) not started!\n", trickplay.decoder_id);
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	if (dec_context->decoder_current_status[trickplay.decoder_id].b_send_stop_cmd) {
		DEC_DEBUG("decode(%d) already stopped!\n", trickplay.decoder_id);
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	switch (trickplay.trick_play) {
	case IAV_TRICK_PLAY_PAUSE:
		cmd_h264_trick_play(iav, NULL, trickplay.decoder_id, IAV_TRICK_PLAY_PAUSE);
		break;
	case IAV_TRICK_PLAY_RESUME:
		cmd_h264_trick_play(iav, NULL, trickplay.decoder_id, IAV_TRICK_PLAY_RESUME);
		break;
	case IAV_TRICK_PLAY_STEP:
		cmd_h264_trick_play(iav, NULL, trickplay.decoder_id, IAV_TRICK_PLAY_STEP);
		break;
	default:
		DEC_DEBUG("decode(%d) bad trick play value %d\n", trickplay.decoder_id, trickplay.trick_play);
		mutex_unlock(&iav->iav_mutex);
		return -EINTR;
		break;
	}
	dec_context->decoder_current_status[trickplay.decoder_id].current_trickplay = trickplay.trick_play;
	mutex_unlock(&iav->iav_mutex);

	return 0;
}

static int iav_ioc_decode_speed(struct ambarella_iav *iav, void __user * arg)
{
	struct iav_decode_speed speed;
	struct iav_decode_context* dec_context = &iav->decode_context;
	int rval = 0;

	mutex_lock(&iav->iav_mutex);

	if (IAV_STATE_DECODING != iav->state) {
		DEC_ERROR("not in decode mode.\n");
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	if (copy_from_user(&speed, arg, sizeof(struct iav_decode_speed))) {
		DEC_ERROR("copy_from_user fail.\n");
		mutex_unlock(&iav->iav_mutex);
		return -EFAULT;
	}

	rval = __check_decode_state_valid(iav, speed.decoder_id);
	if (0 > rval) {
		mutex_unlock(&iav->iav_mutex);
		return rval;
	}

	if (0 != dec_context->decoder_current_status[speed.decoder_id].error_status) {
		DEC_ERROR("decoder met error, status %x\n", dec_context->decoder_current_status[speed.decoder_id].error_status);
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	if (!dec_context->decoder_current_status[speed.decoder_id].is_started) {
		DEC_DEBUG("decode(%d) not started!\n", speed.decoder_id);
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	if (dec_context->decoder_current_status[speed.decoder_id].b_send_stop_cmd) {
		DEC_DEBUG("decode(%d) already stopped!\n", speed.decoder_id);
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	cmd_playback_speed(iav, NULL, speed.decoder_id, speed.speed, speed.scan_mode, speed.direction);

	dec_context->decoder_current_status[speed.decoder_id].speed = speed.speed;
	dec_context->decoder_current_status[speed.decoder_id].scan_mode = speed.scan_mode;
	dec_context->decoder_current_status[speed.decoder_id].direction = speed.direction;
	mutex_unlock(&iav->iav_mutex);

	return 0;
}

static int iav_ioc_query_decode_info(struct ambarella_iav *iav, void __user * arg)
{
	struct iav_decode_status dec_status;
	struct iav_decoder_current_status* status = NULL;
	int rval = 0;
	u32 room = 0;

	mutex_lock(&iav->iav_mutex);

	if (IAV_STATE_DECODING != iav->state) {
		DEC_ERROR("not in decode mode.\n");
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	if (copy_from_user(&dec_status, arg, sizeof(dec_status))) {
		DEC_ERROR("copy_from_user fail.\n");
		mutex_unlock(&iav->iav_mutex);
		return -EFAULT;
	}

	rval = __check_decode_state_valid(iav, dec_status.decoder_id);
	if (0 > rval) {
		mutex_unlock(&iav->iav_mutex);
		return rval;
	}
	mutex_unlock(&iav->iav_mutex);

	status = &iav->decode_context.decoder_current_status[dec_status.decoder_id];

	spin_lock(&iav->iav_lock);
	if (status->current_bsb_read_offset > status->current_bsb_write_offset) {
		room = status->current_bsb_read_offset - status->current_bsb_write_offset;
	} else {
		room = status->current_bsb_size + status->current_bsb_read_offset -
			status->current_bsb_write_offset;
	}

	dec_status.is_started = status->is_started;
	dec_status.is_send_stop_cmd = status->b_send_stop_cmd;

	dec_status.write_offset = status->current_bsb_write_offset;
	if (room > DIAV_DEC_MIN_GAP_IN_BSB) {
		dec_status.room = room - DIAV_DEC_MIN_GAP_IN_BSB;
	} else {
		dec_status.room = 0;
	}

	dec_status.dsp_read_offset = status->current_bsb_read_offset;
	dec_status.free_room = room;

	dec_status.decode_state = status->decode_state;
	dec_status.error_status = status->error_status;
	dec_status.total_error_count = status->total_error_count;
	dec_status.decoded_pic_number = status->decoded_pic_number;
	dec_status.last_pts = status->last_pts;
	dec_status.irq_count = status->irq_count;
	dec_status.yuv422_y_addr = status->yuv422_y_addr;
	dec_status.yuv422_uv_addr = status->yuv422_uv_addr;

	spin_unlock(&iav->iav_lock);

	if (copy_to_user(arg, &dec_status, sizeof(dec_status))) {
		DEC_ERROR("copy_to_user fail.\n");
		return -EFAULT;
	}

	return 0;
}

static int iav_ioc_query_decode_bsb(struct ambarella_iav *iav, void __user * arg)
{
	struct iav_decode_bsb bsb;
	struct iav_decoder_current_status* status = NULL;
	int rval = 0;
	u32 room = 0;

	mutex_lock(&iav->iav_mutex);

	if (IAV_STATE_DECODING != iav->state) {
		DEC_ERROR("not in decode mode.\n");
		mutex_unlock(&iav->iav_mutex);
		return -EPERM;
	}

	if (copy_from_user(&bsb, arg, sizeof(bsb))) {
		DEC_ERROR("copy_from_user fail.\n");
		mutex_unlock(&iav->iav_mutex);
		return -EFAULT;
	}

	rval = __check_decode_state_valid(iav, bsb.decoder_id);
	if (0 > rval) {
		mutex_unlock(&iav->iav_mutex);
		return rval;
	}
	mutex_unlock(&iav->iav_mutex);

	status = &iav->decode_context.decoder_current_status[bsb.decoder_id];

	spin_lock(&iav->iav_lock);
	if (status->current_bsb_read_offset > status->current_bsb_write_offset) {
		room = status->current_bsb_read_offset - status->current_bsb_write_offset;
	} else {
		room = status->current_bsb_size + status->current_bsb_read_offset -
			status->current_bsb_write_offset;
	}

	bsb.start_offset = status->current_bsb_write_offset;
	if (room > DIAV_DEC_MIN_GAP_IN_BSB) {
		bsb.room = room - DIAV_DEC_MIN_GAP_IN_BSB;
	} else {
		bsb.room = 0;
	}

	bsb.dsp_read_offset = status->current_bsb_read_offset;
	bsb.free_room = room;
	spin_unlock(&iav->iav_lock);

	if (copy_to_user(arg, &bsb, sizeof(bsb))) {
		DEC_ERROR("copy_to_user fail.\n");
		return -EFAULT;
	}

	return 0;
}

void iav_clean_decode_stuff(struct ambarella_iav *iav)
{
	DEC_DEBUG("[iav flow]: leave decode_mode\n");

	mutex_lock(&iav->iav_mutex);
	if (IAV_STATE_DECODING == iav->state) {
		stop_all_decoders(iav);
		init_all_decoder_setting(iav);
	}
	mutex_unlock(&iav->iav_mutex);
}

int iav_decode_ioctl(struct ambarella_iav *iav, unsigned int cmd, unsigned long arg)
{
	int rval = 0;

	switch (cmd) {
	case IAV_IOC_ENTER_DECODE_MODE:
		rval = iav_ioc_enter_decode_mode(iav, (void __user *)arg);
		break;

	case IAV_IOC_LEAVE_DECODE_MODE:
		rval = iav_ioc_leave_decode_mode(iav);
		break;

	case IAV_IOC_CREATE_DECODER:
		rval = iav_ioc_create_decoder(iav, (void __user *)arg);
		break;

	case IAV_IOC_DESTROY_DECODER:
		rval = iav_ioc_destroy_decoder(iav, (u8)arg);
		break;

	case IAV_IOC_DECODE_START:
		rval = iav_ioc_decode_start(iav, (u8) arg);
		break;

	case IAV_IOC_DECODE_STOP:
		rval = iav_ioc_decode_stop(iav, (void __user *)arg);
		break;

	case IAV_IOC_DECODE_VIDEO:
		rval = iav_ioc_decode_video(iav, (void __user *)arg);
		break;

	case IAV_IOC_DECODE_JPEG:
		rval = iav_ioc_decode_jpeg(iav, (void __user *)arg);
		break;

	case IAV_IOC_WAIT_DECODE_BSB:
		rval = iav_ioc_wait_decode_bsb(iav, (void __user *)arg);
		break;

	case IAV_IOC_DECODE_TRICK_PLAY:
		rval = iav_ioc_decode_trick_play(iav, (void __user *)arg);
		break;

	case IAV_IOC_DECODE_SPEED:
		rval = iav_ioc_decode_speed(iav, (void __user *)arg);
		break;

	case IAV_IOC_QUERY_DECODE_STATUS:
		rval = iav_ioc_query_decode_info(iav, (void __user *)arg);
		break;

	case IAV_IOC_QUERY_DECODE_BSB:
		rval = iav_ioc_query_decode_bsb(iav, (void __user *)arg);
		break;

	default:
		DEC_ERROR("Not supported cmd [%x].\n", cmd);
		rval = -ENOIOCTLCMD;
		break;
	}

	return rval;
}


