/*
 * iav_enc_stream.c
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
#include <linux/ambpriv_device.h>
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

#define	ENCODE_BITS_INFO_GET_STREAM_ID(stream_id)	(((stream_id)>>6) & 0x3)

static void create_session_id(struct iav_stream *stream)
{
	u32 random_data;

	get_random_bytes(&random_data, sizeof(random_data));
	stream->session_id = random_data;
}

inline int is_stream_in_starting(struct iav_stream *stream)
{
	return (stream->dsp_state == ENC_IDLE_STATE) &&
			(stream->op == IAV_STREAM_OP_START);
}

inline int is_stream_in_stopping(struct iav_stream *stream)
{
	return (stream->dsp_state == ENC_BUSY_STATE) &&
			(stream->op == IAV_STREAM_OP_STOP);
}

inline int is_stream_in_encoding(struct iav_stream *stream)
{
	return (stream->dsp_state == ENC_BUSY_STATE) &&
			(stream->op == IAV_STREAM_OP_START);
}

inline int is_stream_in_idle(struct iav_stream *stream)
{
	return (stream->dsp_state == ENC_IDLE_STATE) &&
			(stream->op == IAV_STREAM_OP_STOP);
}

static inline int is_end_frame(BIT_STREAM_HDR *bsh)
{
	if (unlikely((bsh->frmNo == 0xFFFFFFFF) && (bsh->start_addr == 0xFFFFFFFF)))
		return 1;
	else
		return 0;
}

static inline int is_stream_format_changed(struct iav_stream_format *from,
	struct iav_stream_format *to)
{
	return ((from->enc_win.x != to->enc_win.x) ||
			(from->enc_win.y != to->enc_win.y) ||
			(from->enc_win.width != to->enc_win.width) ||
			(from->enc_win.height != to->enc_win.height) ||
			(from->type != to->type) ||
			(from->buf_id != to->buf_id) ||
			(from->hflip != to->hflip) ||
			(from->vflip != to->vflip) ||
			(from->rotate_cw != to->rotate_cw) ||
			(from->duration != to->duration));
}

static inline int is_invalid_frame(BIT_STREAM_HDR *bsh)
{
	return (bsh->length == 0);
}

static inline void set_invalid_frame(BIT_STREAM_HDR *bsh)
{
	bsh->pts = INVALID_DSP_PTS_32;
	bsh->start_addr = bsh->length = 0;
}

static inline int is_stream_offset_changed_only(struct iav_stream_format *from,
	struct iav_stream_format *to)
{
	return ((from->enc_win.width == to->enc_win.width) &&
			(from->enc_win.height == to->enc_win.height) &&
			(from->type == to->type) &&
			(from->buf_id == to->buf_id) &&
			(from->hflip == to->hflip) &&
			(from->vflip == to->vflip) &&
			(from->rotate_cw == to->rotate_cw) &&
			(from->duration == to->duration));
}

static inline int is_valid_frame_desc(struct iav_frame_desc *frame_desc)
{
	return (frame_desc->desc.size && frame_desc->desc.reso.width &&
		frame_desc->desc.reso.height);
}

static inline void set_frame_desc_invalid(struct iav_frame_desc *frame_desc)
{
	frame_desc->desc.size = 0;
	frame_desc->desc.reso.width = 0;
	frame_desc->desc.reso.height = 0;
}

static inline int get_stream_encode_state(struct iav_stream *stream)
{
	int stream_state = IAV_STREAM_STATE_UNKNOWN;
	if (is_stream_in_encoding(stream)) {
		stream_state = IAV_STREAM_STATE_ENCODING;
	} else if (is_stream_in_idle(stream)) {
		stream_state = IAV_STREAM_STATE_IDLE;
	} else if (is_stream_in_starting(stream)) {
		stream_state = IAV_STREAM_STATE_STARTING;
	} else if (is_stream_in_stopping(stream)) {
		stream_state = IAV_STREAM_STATE_STOPPING;
	}

	return stream_state;
}

static struct iav_frame_desc *iav_get_free_frame_desc(struct ambarella_iav *iav)
{
	struct iav_frame_desc *frame_desc = NULL;

	spin_lock(&iav->iav_lock);
	/* reuse the oldest frame desc from frame queue */
	if (list_empty(&iav->frame_free) && !list_empty(&iav->frame_queue)) {
		frame_desc = list_first_entry(&iav->frame_queue,
			struct iav_frame_desc, node);
		list_move_tail(&frame_desc->node, &iav->frame_free);
		iav->bsb_free_bytes += ALIGN(frame_desc->desc.size, 32);
		++iav->frame_cnt.free_frame_cnt;
	}
	if (!list_empty(&iav->frame_free)) {
		frame_desc = list_first_entry(&iav->frame_free,
			struct iav_frame_desc, node);
		list_del(&frame_desc->node);
		memset(frame_desc, 0, sizeof(struct iav_frame_desc));
		--iav->frame_cnt.free_frame_cnt;
	}
	spin_unlock(&iav->iav_lock);

	if (frame_desc) {
		INIT_LIST_HEAD(&frame_desc->node);
	}

	return frame_desc;
}

static void do_enqueue_frame_desc(struct ambarella_iav * iav, BIT_STREAM_HDR * bsh,
	struct iav_stream * stream, struct iav_frame_desc * frame_desc, int clean_pts)
{
	struct iav_frame_desc * old_frame;
	u32 size;

	frame_desc->desc.id = ENCODE_BITS_INFO_GET_STREAM_ID(bsh->streamID);
	frame_desc->desc.session_id = stream->session_id;
	frame_desc->desc.reso.width = stream->format.enc_win.width;
	frame_desc->desc.reso.height = stream->format.enc_win.height;
	frame_desc->desc.pic_type = bsh->frameTy;
	frame_desc->desc.stream_type = stream->format.type;
	frame_desc->desc.stream_end = !!is_end_frame(bsh);
	frame_desc->desc.frame_num = bsh->frmNo;
	if (stream->srcbuf->id != IAV_SRCBUF_EFM) {
		if ((likely(!clean_pts)) && (likely(!is_end_frame(bsh)))) {
			if (iav->pts_info.hwtimer_enabled == 1) {
				frame_desc->desc.arm_pts = get_hw_pts(iav, bsh->hw_pts);
			} else {
				frame_desc->desc.arm_pts = get_monotonic_pts();
			}
		} else {
			frame_desc->desc.arm_pts = 0;
		}
	} else {
		frame_desc->desc.arm_pts = bsh->hw_pts;
	}
	get_hwtimer_output_ticks(&frame_desc->desc.enc_done_ts);
	frame_desc->desc.dsp_pts = bsh->pts;
	frame_desc->desc.data_addr_offset = DSP_TO_PHYS(bsh->start_addr) -
		iav->mmap[IAV_BUFFER_BSB].phys;
	frame_desc->desc.size = is_end_frame(bsh) ? 0 : bsh->length;
	frame_desc->desc.jpeg_quality =
		(frame_desc->desc.stream_type == IAV_STREAM_TYPE_MJPEG) ?
		stream->mjpeg_config.quality : 0;
	if (bsh->mvdump_curr_daddr) {
		frame_desc->desc.mv_data_offset = DSP_TO_PHYS(bsh->mvdump_curr_daddr) -
			iav->mmap[IAV_BUFFER_MV].phys;
	} else {
		frame_desc->desc.mv_data_offset = 0xFFFFFFFF;
	}

	size = ALIGN(frame_desc->desc.size, 32);
	/* If BSB is full, discard the oldest frame which is overwritten by DSP. */
	while (!list_empty(&iav->frame_queue)) {
		if (iav->bsb_free_bytes >= size)
			break;

		old_frame = list_first_entry(&iav->frame_queue,
			struct iav_frame_desc, node);
		list_move_tail(&old_frame->node, &iav->frame_free);
		iav->bsb_free_bytes += ALIGN(old_frame->desc.size, 32);
		++iav->frame_cnt.free_frame_cnt;
	}

	BUG_ON(iav->bsb_free_bytes < size);
	iav->bsb_free_bytes -= size;

	list_add_tail(&frame_desc->node, &iav->frame_queue);

}

static void iav_add_frame_desc(struct ambarella_iav *iav,
	int clean_pts_flag)
{
	#define	MAX_RETRY		(2)
	struct iav_sync_frame_cnt *frame_cnt = &iav->frame_cnt;
	struct iav_frame_desc *frame_desc;
	struct iav_stream *stream;
	BIT_STREAM_HDR *bsh;
	int stream_id, cnt;

	cnt = 0;
	do {
		bsh = iav->bsh;
		stream_id = ENCODE_BITS_INFO_GET_STREAM_ID(bsh->streamID);
		stream = &iav->stream[stream_id];

		frame_desc = iav_get_free_frame_desc(iav);
		if (!frame_desc) {
			iav_error("Can not find free frame desc for encoded frame!\n");
			break;
		} else if (is_invalid_frame(bsh)) {
			if (frame_cnt->sync_delta) {
				frame_cnt->sync_delta--;
				iav_debug("Ignore reading 1 encoded frame bits info!\n");
			} else {
				/* Pass through silently */
			}
			/* Return the frame to free list */
			spin_lock(&iav->iav_lock);
			++iav->frame_cnt.free_frame_cnt;
			list_add_tail(&frame_desc->node, &iav->frame_free);
			spin_unlock(&iav->iav_lock);
			break;
		}

		spin_lock(&iav->iav_lock);
		do_enqueue_frame_desc(iav, bsh, stream, frame_desc, clean_pts_flag);
		if (!is_end_frame(bsh)) {
			frame_cnt->total_frame_cnt++;
			if (unlikely((stream->format.duration == 1) &&
					(stream->op == IAV_STREAM_OP_START))) {
				stream->fake_dsp_state = ENC_BUSY_STATE;
			}
		}

		/* now this bits_info has been consumed totally, init it to 0 again */
		set_invalid_frame(bsh);
		iav->bsh++;
		if ((u32)iav->bsh >= (u32)iav->bsh_virt + iav->bsh_size)
			iav->bsh = iav->bsh_virt;

		/* Always update snapshot */
		stream->snapshot = *frame_desc;

		spin_unlock(&iav->iav_lock);
	} while (++cnt < MAX_RETRY);
}

int iav_vin_update_stream_framerate(struct ambarella_iav *iav)
{
	int i;
	u8 num, den;
	u32 curr_fps_q9;
	u32 abs_fps_1000, curr_fps_1000;
	u32 flag;
	struct iav_stream_fps *fr_fps;

	if (iav_vin_get_idsp_frame_rate(iav, &curr_fps_q9) < 0) {
		iav_error("iav_vin_get_idsp_frame_rate failed!\n");
		return -EINVAL;
	}
	for (i = 0; i < IAV_STREAM_MAX_NUM_ALL; i++) {
		fr_fps = &iav->stream[i].fps;
		if (fr_fps->abs_fps_enable) {
			abs_fps_1000 = fr_fps->abs_fps * 1000;
			curr_fps_1000 = DIV_ROUND_CLOSEST(512000000, (curr_fps_q9 / 1000));
			if (curr_fps_1000 <= abs_fps_1000) {
				num = 1;
				den = 1;
			} else {
				num = abs_fps_1000 * 240 / curr_fps_1000;
				den = 240;
			}
			if ((fr_fps->fps_div != num) || (fr_fps->fps_multi != den)) {
				fr_fps->fps_div = den;
				fr_fps->fps_multi = num;
				if (is_stream_in_encoding(&iav->stream[i])) {
					flag = REALTIME_PARAM_CUSTOM_FRAME_RATE_BIT |
						REALTIME_PARAM_CUSTOM_VIN_FPS_BIT;
					if (iav->stream[i].format.type == IAV_STREAM_TYPE_H264) {
						flag |= REALTIME_PARAM_CBR_MODIFY_BIT;
					}
					if (cmd_update_encode_params(&iav->stream[i], NULL, flag) < 0) {
						return -EIO;
					}
				}
			}
		}
	}
	return 0;
}

void irq_iav_queue_frame(void *data, DSP_MSG *msg)
{
	struct ambarella_iav *iav = data;

	iav_add_frame_desc(iav, 0);

	wake_up_interruptible(&iav->frame_wq);
	wake_up_interruptible(&iav->mv_wq);
}

/* This function is called by encode init stage only in fast boot mode. */
void iav_sync_bsh_queue(void *data)
{
	struct ambarella_iav *iav = data;

	while (iav->bsh && iav->bsh->start_addr) {
		iav_add_frame_desc(iav, 1);
	}
}

static inline void irq_sync_frame_count(struct ambarella_iav *iav)
{
	struct iav_sync_frame_cnt *frame_cnt = &iav->frame_cnt;
	int i, delta = 0;

	delta = frame_cnt->total_h264_cnt + frame_cnt->total_mjpeg_cnt -
		frame_cnt->total_frame_cnt;

	frame_cnt->sync_delta = 0;
	if (delta > 0) {
		frame_cnt->sync_delta = delta;
		iav_error("Add missing %d frames to the frame queue! "
			"DSP(H264: %u; MJEPG: %u), IAV(Total: %u)!\n",
			delta, frame_cnt->total_h264_cnt, frame_cnt->total_mjpeg_cnt,
			frame_cnt->total_frame_cnt);
		for (i = 0; i < delta; ++i) {
			irq_iav_queue_frame((void *)iav, NULL);
		}
	}
}

void irq_update_frame_count(struct ambarella_iav *iav, encode_msg_t *enc_msg,
	u32 force_update_flag)
{
	struct iav_sync_frame_cnt *frame_cnt = &iav->frame_cnt;
	int update_cnt;
	u32 h264_cnt, mjpeg_cnt;
	static int prev_state = ENC_IDLE_STATE;

	if (enc_msg->encode_operation_mode == VIDEO_MODE) {
		update_cnt = (enc_msg->encode_state == ENC_BUSY_STATE);
		update_cnt |= (enc_msg->encode_state == ENC_IDLE_STATE) &&
			(prev_state == ENC_BUSY_STATE);
		update_cnt |= force_update_flag;
		if (update_cnt) {
			h264_cnt = enc_msg->total_pic_encoded_h264_mode -
				frame_cnt->curr_h264_cnt;
			mjpeg_cnt = enc_msg->total_pic_encoded_mjpeg_mode -
				frame_cnt->curr_mjpeg_cnt;
			frame_cnt->curr_h264_cnt = enc_msg->total_pic_encoded_h264_mode;
			frame_cnt->curr_mjpeg_cnt = enc_msg->total_pic_encoded_mjpeg_mode;

			frame_cnt->total_h264_cnt += h264_cnt;
			frame_cnt->total_mjpeg_cnt += mjpeg_cnt;
		}
	}
	prev_state = enc_msg->encode_state;

	irq_sync_frame_count(iav);
}

int irq_update_stopping_stream_state(struct iav_stream * stream)
{
	u8 wake_up = 0;

	if (is_stream_in_stopping(stream)) {
		wake_up = 1;
		dec_srcbuf_ref(stream->srcbuf, stream->id_dsp);
	} else if (is_stream_in_encoding(stream)) {
		/* Encode duration is config as non-zero for stream, or last frame's case in EFM */
		if (stream->format.duration || (IAV_SRCBUF_EFM == stream->format.buf_id)) {
			wake_up = 1;
			stream->op = IAV_STREAM_OP_STOP;
			dec_srcbuf_ref(stream->srcbuf, stream->id_dsp);
		} else {
			iav_error("Incorrect DSP encode IDLE state for stream %c.\n",
				'A' + stream->id_dsp);
		}
	}

	return wake_up;
}

static struct iav_frame_desc *iav_find_frame_desc(struct ambarella_iav *iav,
	u32 id, u8 instant_fetch)
{
	struct iav_frame_desc *frame_desc = NULL;
	int found = 0;

	spin_lock_irq(&iav->iav_lock);
	if (instant_fetch) {
		frame_desc = &iav->stream[id].snapshot;
		if (is_valid_frame_desc(frame_desc)) {
			found = 1;
		}
	} else {
		if (!list_empty(&iav->frame_queue)) {
			if (likely(id == -1)) {
				frame_desc = list_first_entry(&iav->frame_queue,
					struct iav_frame_desc, node);
				list_del_init(&frame_desc->node);
				found = 1;
			} else {
				list_for_each_entry(frame_desc, &iav->frame_queue, node) {
					if (frame_desc->desc.id == id) {
						list_del_init(&frame_desc->node);
						found = 1;
						break;
					}
				}
			}
		}
	}
	spin_unlock_irq(&iav->iav_lock);

	return (found == 1)? frame_desc : NULL;
}

static int check_framedesc_param(struct ambarella_iav *iav,
	struct iav_framedesc *framedesc)
{
	struct iav_stream *stream;
	u32 enc_mode = iav->encode_mode;
	u8 max_stream_num = iav->system_config[enc_mode].max_stream_num;
	u8 instant_fetch = framedesc->instant_fetch;

	if (framedesc->id != -1 && framedesc->id >= max_stream_num) {
		iav_error("Invalid stream ID: %d!\n", framedesc->id);
		return -1;
	}

	stream = &iav->stream[framedesc->id];
	if (instant_fetch) {
		if (framedesc->id == -1) {
			iav_error("Invalid stream ID for instant fetch, should be [0,%d].\n",
				IAV_STREAM_MAX_NUM_ALL - 1);
			return -1;
		} else if (stream->format.type != IAV_STREAM_TYPE_MJPEG) {
			iav_error("Only support instant fetch for MJPEG stream!\n");
			return -1;
		}
	}

	return 0;
}

static int iav_query_framedesc(struct ambarella_iav *iav,
	struct iav_framedesc *framedesc)
{
	struct iav_frame_desc *frame_desc;
	struct iav_stream *stream;
	u32 stream_id;
	long time_jiffies;
	int rval = 0;
	u8 instant_fetch = framedesc->instant_fetch;

	if(check_framedesc_param(iav, framedesc) < 0) {
		return -EINVAL;
	}

	stream_id = framedesc->id;

	if (framedesc->time_ms == -1) {
		/* non blocking way */
		frame_desc = iav_find_frame_desc(iav, stream_id, instant_fetch);
	} else if (framedesc->time_ms == 0) {
		/* blocking way */
		wait_event_interruptible(iav->frame_wq,
			(frame_desc = iav_find_frame_desc(iav, stream_id, instant_fetch)));
	} else {
		/* with timeout in ms*/
		time_jiffies = msecs_to_jiffies(framedesc->time_ms);
		wait_event_interruptible_timeout(iav->frame_wq,
			(frame_desc = iav_find_frame_desc(iav, stream_id, instant_fetch)),
			time_jiffies);
	}

	if (frame_desc == NULL) {
		return -EAGAIN;
	}

	*framedesc = frame_desc->desc;
	stream_id = frame_desc->desc.id;

	stream = &iav->stream[stream_id];
	// Clean latest snapshot
	if (instant_fetch) {
		spin_lock_irq(&iav->iav_lock);
		set_frame_desc_invalid(&iav->stream[stream_id].snapshot);
		spin_unlock_irq(&iav->iav_lock);
		return rval;
	}

	spin_lock_irq(&iav->iav_lock);
	if (frame_desc->desc.stream_end) {
		iav_debug("Sent STREAM END null frame for stream %c.\n",
			'A' + stream_id);
		irq_update_stopping_stream_state(&iav->stream[stream_id]);
	}
	++iav->frame_cnt.free_frame_cnt;
	list_add_tail(&frame_desc->node, &iav->frame_free);
	/* BSB is big enough, so it should make sense to suppose the
	 * frame is consumed by user space ASAP after this ioctl. */
	iav->bsb_free_bytes += ALIGN(frame_desc->desc.size, 32);
	spin_unlock_irq(&iav->iav_lock);

	return rval;
}

static struct iav_frame_desc *iav_find_statis_desc(struct ambarella_iav *iav,
	u32 stream_id)
{
	struct iav_frame_desc *frame_desc;

	// use instant fetch for statis desc
	frame_desc = iav_find_frame_desc(iav, stream_id, 1);
	spin_lock_irq(&iav->iav_lock);
	if ((frame_desc != NULL) &&
		(frame_desc->desc.mv_data_offset != 0xFFFFFFFF)) {
		spin_unlock_irq(&iav->iav_lock);
		return frame_desc;
	}
	spin_unlock_irq(&iav->iav_lock);

	return NULL;
}

static int iav_query_statisdesc(struct ambarella_iav *iav,
	struct iav_statisdesc *statisdesc)
{
	struct iav_frame_desc *frame_desc;
	struct iav_framedesc *desc;
	struct iav_stream *stream;
	struct iav_stream_format *stream_format = NULL;
	u32 stream_id;
	long time_jiffies;
	int rval = 0;
	int enc_mode = iav->encode_mode;
	int max_stream_num = iav->system_config[enc_mode].max_stream_num;
	struct iav_window win;

	stream_id = statisdesc->id;
	if (stream_id != -1 && stream_id >= max_stream_num) {
		iav_error("Invalid stream ID: %d!\n", stream_id);
		return -EINVAL;
	}
	stream = &iav->stream[stream_id];
	stream_format = &stream->format;

	if (statisdesc->time_ms == -1) {
		/* non blocking way */
		frame_desc = iav_find_statis_desc(iav, stream_id);
	} else if (statisdesc->time_ms == 0) {
		/* blocking way */
		wait_event_interruptible(iav->mv_wq,
			(frame_desc = iav_find_statis_desc(iav, stream_id)));
	} else {
		/* with timeout in ms*/
		time_jiffies = msecs_to_jiffies(statisdesc->time_ms);
		wait_event_interruptible_timeout(iav->mv_wq,
			(frame_desc = iav_find_statis_desc(iav, stream_id)),
			time_jiffies);
	}

	spin_lock_irq(&iav->iav_lock);
	if (frame_desc == NULL) {
		spin_unlock_irq(&iav->iav_lock);
		return -EAGAIN;
	}

	desc = &frame_desc->desc;

	statisdesc->data_addr_offset = desc->mv_data_offset;
	get_stream_win_MB(stream_format, &win);
	statisdesc->width = win.width;
	statisdesc->height = win.height;
	statisdesc->pitch = ALIGN(win.width * sizeof(struct iav_mv), 32);
	statisdesc->arm_pts = desc->arm_pts;
	statisdesc->dsp_pts = desc->dsp_pts;
	statisdesc->frame_num = desc->frame_num;
	statisdesc->session_id = desc->session_id;

	set_frame_desc_invalid(&stream->snapshot);

	spin_unlock_irq(&iav->iav_lock);

	return rval;
}

/*
 * EXPORT_SYMBOL Interface for UVC
 */
int iav_fetch_bsbinfo(unsigned long *iav_bsb_virt, u32 *iav_bsb_size)
{
	struct ambarella_iav *iav = NULL;

	iav = ambpriv_get_drvdata(iav_device);
	if (iav == NULL) {
		iav_error("Get iav info failed\n");
		return -EFAULT;
	}

	*iav_bsb_virt = iav->mmap[IAV_BUFFER_BSB].virt;
	*iav_bsb_size = iav->mmap[IAV_BUFFER_BSB].size;

	return 0;
}
EXPORT_SYMBOL(iav_fetch_bsbinfo);

int iav_fetch_framedesc(struct iav_framedesc *framedesc)
{
	struct ambarella_iav *iav = NULL;
	int ret = 0;

	iav = ambpriv_get_drvdata(iav_device);
	if (iav == NULL) {
		iav_error("Get iav info failed\n");
		return -EFAULT;
	}

	ret = iav_query_framedesc(iav, framedesc);
	if (ret < 0) {
		iav_error("Get frame descriptor failed\n");
		return -EFAULT;
	}
	framedesc->data_addr_offset += iav->mmap[IAV_BUFFER_BSB].virt;

	return ret;
}
EXPORT_SYMBOL(iav_fetch_framedesc);

int iav_ioc_g_query_desc(struct ambarella_iav *iav, void __user *arg)
{
	struct iav_querydesc desc;
	int rval = 0;

	if (copy_from_user(&desc, arg, sizeof(desc)))
		return -EFAULT;

	switch (desc.qid) {
	case IAV_DESC_FRAME:
		rval = iav_query_framedesc(iav, &desc.arg.frame);
		break;
	case IAV_DESC_STATIS:
		rval = iav_query_statisdesc(iav, &desc.arg.statis);
		break;
	case IAV_DESC_RAW:
		rval = iav_query_rawdesc(iav, &desc.arg.raw);
		break;
	case IAV_DESC_YUV:
		rval = iav_query_yuvdesc(iav, &desc.arg.yuv);
		break;
	case IAV_DESC_ME1:
		rval = iav_query_me1desc(iav, &desc.arg.me1);
		break;
	case IAV_DESC_ME0:
		rval = iav_query_me0desc(iav, &desc.arg.me0);
		break;
	case IAV_DESC_BUFCAP:
		rval = iav_query_bufcapdesc(iav, &desc.arg.bufcap);
		break;
	case IAV_DESC_QP_HIST:
		rval = iav_query_qphistdesc(iav, &desc.arg.qphist);
		break;
	default:
		rval = -EINVAL;
		break;
	}

	if (!rval)
		rval = copy_to_user(arg, &desc, sizeof(desc)) ? -EFAULT : 0;

	return rval;
}

static void iav_reset_encode_obj(struct ambarella_iav *iav)
{
	struct iav_sync_frame_cnt * sync = NULL;
	struct iav_frame_desc *frame_desc = NULL;

	spin_lock_irq(&iav->iav_lock);

	sync = &iav->frame_cnt;
	iav->bsh = iav->bsh_virt;
	iav->bsb_free_bytes = iav->mmap[IAV_BUFFER_BSB].size;
	while (!list_empty(&iav->frame_queue)) {
		frame_desc = list_first_entry(&iav->frame_queue,
			struct iav_frame_desc, node);
		list_move_tail(&frame_desc->node, &iav->frame_free);
		sync->free_frame_cnt++;
	}
	sync->total_h264_cnt = 0;
	sync->total_mjpeg_cnt = 0;
	sync->total_frame_cnt = 0;
	sync->curr_h264_cnt = 0;
	sync->curr_mjpeg_cnt = 0;
	sync->sync_delta = 0;

	spin_unlock_irq(&iav->iav_lock);
}

static int check_encode_resource_limit(struct ambarella_iav *iav, u32 stream_map)
{
	u32 chip_id, chip_idx;
	u32 system_load, system_bitrate;
	u32 system_load_limit;
	u32 max_mb;
	int i, width, height;
	int idsp_fps, fps;
	struct iav_stream *stream;

	iav_no_check();

	iav_vin_get_idsp_frame_rate(iav, &idsp_fps);
	if (idsp_fps == 0) {
		iav_error("frame_rate of iDSP can not be zero\n");
		return -1;
	}
	idsp_fps = DIV_CLOSEST(FPS_Q9_BASE, idsp_fps);

	chip_id = get_chip_id(iav);
	if (chip_id < IAV_CHIP_ID_S2LM_FIRST ||
		chip_id >= IAV_CHIP_ID_S2L_LAST) {
		iav_error("Invalid S2L chip ID [%d].\n", chip_id);
		chip_id = IAV_CHIP_ID_S2L_99;
	}
	system_load = system_bitrate = 0;
	for (i = 0; i < IAV_STREAM_MAX_NUM_ALL; ++i) {
		stream = &iav->stream[i];
		if (!is_stream_in_idle(stream) || (stream_map & (1 << i))) {
			width = ALIGN(stream->format.enc_win.width,
				PIXEL_IN_MB) / PIXEL_IN_MB;
			height = ALIGN(stream->format.enc_win.height,
				PIXEL_IN_MB) / PIXEL_IN_MB;
			fps = idsp_fps * stream->fps.fps_multi / stream->fps.fps_div;
			system_load += width * height * fps;

			if (stream->format.type == IAV_STREAM_TYPE_H264) {
				system_bitrate += get_dsp_encode_bitrate(stream);
			}
		}
	}

	iav_debug("Check encode resource limit : iDSP [%d], system load %u.\n",
		idsp_fps, system_load);

	chip_idx = chip_id - IAV_CHIP_ID_S2LM_FIRST;
	system_load_limit = G_system_load[chip_idx].system_load;
	if (system_load > system_load_limit) {
		iav_error("Total system load %d exceed maximum %d (%s).\n",
			system_load, system_load_limit, G_system_load[chip_idx].desc);
		return -1;
	}

	max_mb = G_encode_limit[iav->encode_mode].max_enc_MB;
	if (system_load > max_mb) {
		iav_error("Total system load %d exceed maximum %d in mode [%u].\n",
				system_load, max_mb, iav->encode_mode);
		return -1;
	}

	if (system_bitrate > IAV_MAX_ENCODE_BITRATE) {
		iav_error("Total system bitrate [%d] exceed maximum [%d].\n",
			system_bitrate, IAV_MAX_ENCODE_BITRATE);
		return -1;
	}

	return 0;
}


static int check_encode_stream_state(struct ambarella_iav *iav, u32 stream_map)
{
	int i, encoding_counter, max_num;
	u32 chip_id, chip_idx;
	struct iav_stream *stream;

	chip_id = get_chip_id(iav);
	if (chip_id < IAV_CHIP_ID_S2LM_FIRST ||
		chip_id >= IAV_CHIP_ID_S2L_LAST) {
		iav_error("Invalid S2L chip ID [%d].\n", chip_id);
		chip_id = IAV_CHIP_ID_S2L_99;
	}

	encoding_counter = 0;
	for (i = 0; i < IAV_STREAM_MAX_NUM_ALL; ++i) {
		stream = &iav->stream[i];
		if (stream_map & (1 << i)) {
			if (stream->format.type != IAV_STREAM_TYPE_H264 &&
				stream->format.type != IAV_STREAM_TYPE_MJPEG) {
				iav_error("Encode type is not valid for stream %c.\n",
					'A' + i);
				return -EINVAL;
			}
			if (!is_stream_in_idle(stream)) {
				iav_error("Stream %c is not ready or already encoding.\n",
					'A' + i);
				return -EAGAIN;
			}
			++encoding_counter;
		} else {
			if (!is_stream_in_idle(stream))
				++encoding_counter;
		}
	}
	max_num = G_encode_limit[iav->encode_mode].max_enc_num;
	if (encoding_counter > max_num) {
		iav_error("Total encode stream num [%d] is more than max"
			" supported num [%d].\n", encoding_counter, max_num);
		return -EINVAL;
	}

	chip_idx = chip_id - IAV_CHIP_ID_S2LM_FIRST;
	if (encoding_counter > G_system_load[chip_idx].max_enc_num) {
		iav_error("Total encode stream num [%d] is more than max"
			" supported num [%d] for S2L chip [%d].\n", encoding_counter,
			G_system_load[chip_idx].max_enc_num, chip_id);
		return -EINVAL;
	}

	return 0;
}

int check_buffer_config_limit(struct ambarella_iav *iav, u32 stream_map)
{
	struct iav_stream *stream;
	struct iav_buffer *buffer;
	int i;

	iav_no_check();

	for (i = 0; i < IAV_STREAM_MAX_NUM_ALL; ++i) {
		stream = &iav->stream[i];
		if ((stream_map & (1 << i))
			&& (stream->format.buf_id != IAV_SRCBUF_EFM)) {
			buffer = stream->srcbuf;
			if (buffer->type != IAV_SRCBUF_TYPE_ENCODE) {
				iav_error("Please config the source buffer [%d] type for "
					"encoding first!\n", buffer->id);
				return -1;
			}
			if (buffer->win.width == 0 || buffer->win.height == 0) {
				iav_error("Source buffer [%d] is disabled, cannot start "
						"encoding from it!\n", buffer->id);
				return -1;
			}
		}
	}

	return 0;
}

static int check_stream_offset(struct ambarella_iav *iav, struct iav_stream_format *format)
{
	struct iav_window *buf_win;
	struct iav_rect *enc_win;

	iav_no_check();

	enc_win = &format->enc_win;
	if ((enc_win->width & 0x1) || (enc_win->height & 0x1) ||
		(enc_win->x & 0x1) || (enc_win->y & 0x1)) {
		iav_error("Stream %c: %dx%d + offset %dx%d must be even.\n",
			'A' + format->id, enc_win->width, enc_win->height,
			enc_win->x, enc_win->y);
		return -1;
	}
	if (format->buf_id != IAV_SRCBUF_EFM) {
		buf_win = &iav->stream[format->id].srcbuf->win;
	} else {
		buf_win = &iav->efm.efm_size;
	}
	if ((enc_win->width + enc_win->x > buf_win->width) ||
		(enc_win->height + enc_win->y > buf_win->height)) {
		iav_error("Stream %c: %dx%d + offset %dx%d is out of source buffer "
			"[%d] %dx%d.\n", 'A' + format->id, enc_win->width, enc_win->height,
			enc_win->x,	enc_win->y, format->buf_id,
			buf_win->width, buf_win->height);
		return -1;
	}

	return 0;
}

static int check_force_fast_seek(struct iav_stream *stream)
{
	iav_no_check();

	if(stream->long_ref_enable != 1){
		iav_error("Only support force fask seek frame when "
			"stream long ref enable = 1!\n");
		return -1;
	} else if (stream->h264_config.gop_structure != IAV_GOP_LT_REF_P) {
		iav_error("Only support force fask seek frame when "
			"gop = %d!\n", IAV_GOP_LT_REF_P);
		return -1;
	} else if (stream->h264_config.fast_seek_intvl == 0) {
		iav_error("Only support force fask seek frame when "
			"fast_seek_intvl > 0!\n");
		return -1;
	}

	return 0;
}

static int check_frame_drop(struct iav_stream *stream, u32 drop_count)
{
	iav_no_check();

	if (drop_count > MAX_FRAME_DROP_COUNT) {
		iav_error("Frame drop count %u should be smaller than %u!\n",
			drop_count, MAX_FRAME_DROP_COUNT);
		return -1;
	}

	return 0;
}

static int check_h264_enc_param(struct iav_stream *stream,
	struct iav_h264_enc_param *h264_enc)
{
	iav_no_check();

	if (h264_enc->intrabias_p > MAX_INTRABIAS ||
		h264_enc->intrabias_p < MIN_INTRABIAS) {
		iav_error("P frame intrabias %d is out of range [%u~%u]!\n",
			h264_enc->intrabias_p, MIN_INTRABIAS, MAX_INTRABIAS);
		return -1;
	}
	if (h264_enc->intrabias_b > MAX_INTRABIAS ||
		h264_enc->intrabias_b < MIN_INTRABIAS) {
		iav_error("B frame intrabias %d is out of range [%u~%u]!\n",
			h264_enc->intrabias_b, MIN_INTRABIAS, MAX_INTRABIAS);
		return -1;
	}
	if (h264_enc->user1_intra_bias > MAX_USER_BIAS ||
		h264_enc->user1_intra_bias < MIN_USER_BIAS) {
		iav_error("User1 intrabias %d is out of range [%u~%u]!\n",
			h264_enc->user1_intra_bias, MIN_USER_BIAS, MAX_USER_BIAS);
		return -1;
	}
	if (h264_enc->user1_direct_bias > MAX_USER_BIAS ||
		h264_enc->user1_direct_bias < MIN_USER_BIAS) {
		iav_error("User1 directbias %d is out of range [%u~%u]!\n",
			h264_enc->user1_direct_bias, MIN_USER_BIAS, MAX_USER_BIAS);
		return -1;
	}
	if (h264_enc->user2_intra_bias > MAX_USER_BIAS ||
		h264_enc->user2_intra_bias < MIN_USER_BIAS) {
		iav_error("User2 intrabias %d is out of range [%u~%u]!\n",
			h264_enc->user2_intra_bias, MIN_USER_BIAS, MAX_USER_BIAS);
		return -1;
	}
	if (h264_enc->user2_direct_bias > MAX_USER_BIAS ||
		h264_enc->user2_direct_bias < MIN_USER_BIAS) {
		iav_error("User2 directbias %d is out of range [%u~%u]!\n",
			h264_enc->user2_direct_bias, MIN_USER_BIAS, MAX_USER_BIAS);
		return -1;
	}
	if (h264_enc->user3_intra_bias > MAX_USER_BIAS ||
		h264_enc->user3_intra_bias < MIN_USER_BIAS) {
		iav_error("User3 intrabias %d is out of range [%u~%u]!\n",
			h264_enc->user3_intra_bias, MIN_USER_BIAS, MAX_USER_BIAS);
		return -1;
	}
	if (h264_enc->user3_direct_bias > MAX_USER_BIAS ||
		h264_enc->user3_direct_bias < MIN_USER_BIAS) {
		iav_error("User3 directbias %d is out of range [%u~%u]!\n",
			h264_enc->user3_direct_bias, MIN_USER_BIAS, MAX_USER_BIAS);
		return -1;
	}

	return 0;
}


static int iav_check_h264_config(struct ambarella_iav *iav, int id,
	struct iav_h264_config *h264)
{
	struct iav_stream *stream;

	iav_no_check();

	stream = &iav->stream[id];
	if (id >= IAV_STREAM_MAX_NUM_ALL) {
		iav_error("Invalid stream ID: %d!\n", id);
		return -1;
	}
	if (h264->profile > H264_PROFILE_HIGH){
		iav_error("Invalid H264 profile_idc: %d.\n", h264->profile);
		return -1;
	}
	if ((h264->gop_structure != IAV_GOP_SIMPLE) &&
		(h264->gop_structure != IAV_GOP_SVCT_2) &&
		(h264->gop_structure != IAV_GOP_SVCT_3) &&
		(h264->gop_structure != IAV_GOP_NON_REF_P) &&
		(h264->gop_structure != IAV_GOP_LT_REF_P)) {
		iav_error("Invalid H264 gop structure: %d.\n", h264->gop_structure);
		return -1;
	}
	if (h264->gop_structure == IAV_GOP_SVCT_3) {
		if(stream->long_ref_enable != 1){
			iav_error("Only support 3 level SVCT gop when "
				"stream long ref enable = 1!\n");
			return -1;
		}
	}
	if (!h264->M || !h264->N || h264->N % h264->M != 0 || h264->N > 0xFFF) {
		iav_error("Invalid H264 M/N: %d, %d\n", h264->M, h264->N);
		return -1;
	}
	if (h264->M > stream->max_GOP_M) {
		iav_error("H264 M %d should be no greater than max gop M %d!\n",
			h264->M, stream->max_GOP_M);
		return -1;
	}
	if (h264->chroma_format != H264_CHROMA_YUV420 &&
		h264->chroma_format != H264_CHROMA_MONO) {
		iav_error("Only support YUV420 and Mono H264.\n");
		return -1;
	}
	if (h264->au_type >= H264_AU_TYPE_NUM) {
		iav_error("Invalid H264 AU type %d.\n", h264->au_type);
		return -1;
	}
	if (h264->profile < H264_PROFILE_FIRST ||
		h264->profile >= H264_PROFILE_LAST) {
		iav_error("Invalid H264 profile %d, not in the range of [%d~%d].\n",
			h264->profile, H264_PROFILE_FIRST, H264_PROFILE_LAST);
		return -1;
	}
	if (h264->fast_seek_intvl >  FAST_SEEK_INTVL_MAX ||
		h264->fast_seek_intvl > h264->N) {
		iav_error("Invalid H264 fast seek interval %d, should be smaller "
			"than %d and N [%d].\n",
			h264->fast_seek_intvl, FAST_SEEK_INTVL_MAX, h264->N);
		return -1;
	} else if (h264->fast_seek_intvl != 0) {
		if(stream->long_ref_enable != 1){
			iav_error("Only support fast seek interval when "
				"stream long ref enable = 1!\n");
			return -1;
		} else if (h264->gop_structure != IAV_GOP_LT_REF_P) {
			iav_error("Only support fast seek interval when "
				"gop = %d!\n", IAV_GOP_LT_REF_P);
			return -1;
		} else if (h264->idr_interval != 1) {
			iav_error("Only support fast seek interval when "
				"idr_interval = 1!\n");
			return -1;
		}
	}
	if (h264->multi_ref_p > 0) {
		if(stream->long_ref_enable != 1){
			iav_error("Only support multi-ref P frame when stream long ref "
				"enable = 1!\n");
			return -1;
		} else if (h264->gop_structure != IAV_GOP_LT_REF_P) {
			iav_error("Only support multi-ref P frame when gop = %d!\n",
				IAV_GOP_LT_REF_P);
			return -1;
		} else if (stream->max_GOP_M <= 1) {
			iav_error("Only support multi-ref P frame when stream "
				"max_GOP_M > 1!\n");
			return -1;
		} else if (h264->M > 1) {
			if (!iav->system_config[iav->encode_mode].long_ref_b_frame) {
				iav_error("Only support multi-ref P frame with B frame when "
					"long ref B frame is enabled!\n");
				return -1;
			} else if (h264->fast_seek_intvl % h264->M) {
				iav_error("Only support multi-ref P frame with B frame when "
					"fast seek interval is a multiple of M!\n");
				return -1;
			}
		}
	}

	if (h264->deblocking_filter_alpha < IAV_DEBLOCKING_ALPHA_MIN ||
		h264->deblocking_filter_alpha > IAV_DEBLOCKING_ALPHA_MAX) {
		iav_error("deblocking filter alpha should be in range [%d, %d]!\n",
			IAV_DEBLOCKING_ALPHA_MIN, IAV_DEBLOCKING_ALPHA_MAX);
	}

	if (h264->deblocking_filter_beta < IAV_DEBLOCKING_BETA_MIN ||
		h264->deblocking_filter_beta > IAV_DEBLOCKING_BETA_MAX) {
		iav_error("deblocking filter beta should be in range [%d, %d]!\n",
			IAV_DEBLOCKING_BETA_MIN, IAV_DEBLOCKING_BETA_MAX);
	}

	if (h264->deblocking_filter_enable >= H264_DEBLOCKING_TYPE_NUM) {
		iav_error("deblocking filter enable flag should be in range [0, %d]!\n",
			H264_DEBLOCKING_TYPE_NUM);
	}

	stream = &iav->stream[id];
	if (stream->format.type != IAV_STREAM_TYPE_H264) {
		iav_error("Cannot change H264 config for MJPEG stream.\n");
		return -1;
	}

	if ((h264->frame_crop_left_offset & 1) || (h264->frame_crop_right_offset & 1)
		|| (h264->frame_crop_top_offset & 1) || (h264->frame_crop_bottom_offset & 1)) {
		iav_error("Crop only support even offset.\n");
		return -1;
	}

	if ((h264->frame_crop_left_offset + h264->frame_crop_right_offset) >
			(stream->format.rotate_cw ? stream->format.enc_win.height : stream->format.enc_win.width)
		|| (h264->frame_crop_top_offset + h264->frame_crop_bottom_offset) >
			(stream->format.rotate_cw ? stream->format.enc_win.width : stream->format.enc_win.height)) {
		iav_error("Crop can not bigger than stream resolution.\n");
		return -1;
	}

	return 0;
}

static inline void get_pic_info_h264(struct ambarella_iav *iav, int id,
	struct iav_h264_cfg *h264)
{
	struct iav_vin_format *vin_format;
	struct iav_stream *stream;
	u32 vfr = 0;

	stream = &iav->stream[id];

	h264->pic_info.width = stream->format.enc_win.width;
	h264->pic_info.height = stream->format.enc_win.height;

	vin_format = &iav->vinc[0]->vin_format;

	if (iav->vin_enabled) {
		vfr = amba_iav_fps_format_to_vfr(vin_format->frame_rate,
			vin_format->video_format, 0);
		if (vfr < 2) {
			h264->pic_info.rate = 6006;
		} else if (vfr < 4) {
			h264->pic_info.rate = 3003;
		} else {
			h264->pic_info.rate = 180000 / vfr;
		}
		h264->pic_info.scale = 180000;
	}
}

int check_stream_config_limit(struct ambarella_iav *iav, u32 stream_map)
{
	struct iav_stream *stream;
	int i;
	u16 enc_w, enc_h;
	int enc_mode = iav->encode_mode;
	u32 rotate_flag = iav->system_config[enc_mode].rotatable;
	struct iav_window *min_enc = &G_encode_limit[enc_mode].min_enc;

	iav_no_check();

	for (i = 0; i < IAV_STREAM_MAX_NUM_ALL; ++i) {
		if (stream_map & (1 << i)) {
			stream = &iav->stream[i];
			// check rotate flag
			if (stream->format.rotate_cw) {
				if (!rotate_flag) {
					iav_error("Cannot rotate stream %c when rotate_supported"
							" is disabled!\n", 'A' + i);
					return -1;
				}
			}
			// check stream offset with source buffer size
			if (check_stream_offset(iav, &stream->format) < 0) {
				return -1;
			}
			if (stream->format.rotate_cw) {
				enc_w = stream->format.enc_win.height;
				enc_h = stream->format.enc_win.width;
			} else {
				enc_w = stream->format.enc_win.width;
				enc_h = stream->format.enc_win.height;
			}
			// check min size of encode window
			if (enc_w < min_enc->width || enc_h < min_enc->height) {
				iav_error("Stream %c encoding size %dx%d is smaller than min "
					"size %dx%d. Please increase the stream size.\n",
					'A' + i, enc_w, enc_h, min_enc->width, min_enc->height);
				return -1;
			}
			// check max stream size in system resource limit
			if (enc_w > stream->max.width || enc_h > stream->max.height) {
				iav_error("Stream %c encoding size %dx%d is bigger than max "
					"size %dx%d. Please increase the stream max size by "
					"IAV_IOC_SET_SYSTEM_RESOURCE.\n", 'A' + i, enc_w, enc_h,
					stream->max.width, stream->max.height);
				return -1;
			}
			if (stream->format.type == IAV_STREAM_TYPE_H264) {
				if (iav_check_h264_config(iav, i, &stream->h264_config) < 0) {
					iav_error("Failed check_h264_config \n");
					return -1;
				}
			}else {
				if (enc_w & 0xF) {
					iav_error("MJPEG encode width %d (rotate : %d) must be multiple of 16.\n",
						enc_w, stream->format.rotate_cw);
					return -1;
				}
			}
		}
	}

	return 0;
}

static int iav_check_start_encode_state(struct ambarella_iav *iav, u32 stream_map)
{
	if (!is_enc_work_state(iav)) {
		iav_error("Invalid IAV state before encoding: %d!\n", iav->state);
		return -1;
	}

	iav_no_check();

	if (check_encode_stream_state(iav, stream_map) < 0) {
		return -1;
	}

	return 0;
}

static int iav_check_before_start_encode(struct ambarella_iav *iav, u32 stream_map)
{
	iav_no_check();

	/* check total encode performance limit */
	if (check_encode_resource_limit(iav, stream_map) < 0) {
		iav_error("Cannot start encode, system resource is not enough.\n");
		return -1;
	}

	/* check total DSP memory bandwidth/size */
	if (iav_check_sys_mem_usage(iav, (int)IAV_STATE_ENCODING, stream_map) < 0) {
		iav_error("Cannot start encode, DSP memory check failed.\n");
		return -1;
	}

	if ((iav->encode_mode == DSP_MULTI_REGION_WARP_MODE) &&
		(iav_check_warp_idsp_perf(iav, NULL) < 0)) {
		return -1;
	}

	if (check_buffer_config_limit(iav, stream_map) < 0) {
		iav_error("Cannot start encode, invalid buffer configuration!\n");
		return -1;
	}

	if (check_stream_config_limit(iav, stream_map) < 0) {
		iav_error("Cannot start encode, invalid stream configuration!\n");
		return -1;
	}

	return 0;
}

static int check_stream_fps(struct ambarella_iav *iav, struct iav_stream_fps *fps)
{
	u32 idsp_out_frame_rate = 0;

	if (fps->fps_div == 0) {
		iav_error("Invalid fps_div: %d\n", fps->fps_div);
		return -EINVAL;
	}

	iav_no_check();

	if (fps->fps_multi > fps->fps_div) {
		iav_error("Encode frame rate: %u/%u exceeds VIN frame rate.\n",
			fps->fps_multi, fps->fps_div);
		return -EINVAL;
	}

	if (iav_vin_get_idsp_frame_rate(iav, &idsp_out_frame_rate) < 0) {
		return -EINVAL;
	}
	idsp_out_frame_rate = DIV_CLOSEST(FPS_Q9_BASE, idsp_out_frame_rate);
	if ((idsp_out_frame_rate * fps->fps_multi) < fps->fps_div) {
		iav_error("Encode frame rate: %u x (%u/%u) is less than 1 fps.\n",
			idsp_out_frame_rate, fps->fps_multi, fps->fps_div);
		return -EINVAL;
	}

	return 0;
}

static int check_qp_matrix_param(struct iav_stream *stream,
	struct iav_qpmatrix *h264_roi)
{
	u8 *start_addr = NULL;
	int i, j, mb_pitch, total_size;
	struct iav_qproi_data *qp_data_addr;
	struct iav_window win;
	struct iav_stream_format *stream_format = NULL;

	iav_no_check();

	if (!h264_roi->enable && !h264_roi->qpm_no_check) {
		iav_error("Can not check qp matrix when qp matrix is disabled!\n");
		return -1;
	} else if (!h264_roi->qpm_no_check) {
		start_addr = (u8 *)(stream->iav->mmap[IAV_BUFFER_QPMATRIX].virt) +
			stream->id_dsp * STREAM_QP_MATRIX_SIZE;
		stream_format = &stream->format;
		get_stream_win_MB(stream_format, &win);
		mb_pitch = ALIGN(win.width, 8);
		total_size = mb_pitch * win.height;
		// Always use I frame buffer no matter in IPB mode or not
		qp_data_addr = (struct iav_qproi_data *)start_addr;
		for (i = 0; i < total_size; ++i) {
			if (qp_data_addr[i].qp_quality < QP_QUALITY_MIN ||
				qp_data_addr[i].qp_quality > QP_QUALITY_MAX) {
				iav_debug("Invalid QP quality value [%d] of element [%d] on "
					"stream %c.\n", qp_data_addr[i].qp_quality, i,
					'A' + stream->id_dsp);
				return -1;
			}
		}

		// Only P frame setting is valid
		qp_data_addr = (struct iav_qproi_data *)(start_addr +
			(STREAM_QP_MATRIX_NUM > 1 ? SINGLE_QP_MATRIX_SIZE : 0));
		for (i = 0; i < total_size; ++i) {
			if (qp_data_addr[i].zmv_threshold < ZMV_THRESHOLD_MIN ||
				qp_data_addr[i].zmv_threshold > ZMV_THRESHOLD_MAX) {
				iav_debug("Invalid zmv threshold value [%d] of element [%d] on "
					"stream %c.\n", qp_data_addr[i].zmv_threshold, i,
					'A' + stream->id_dsp);
				return -1;
			}
		}

		for (j = 0; j < STREAM_QP_MATRIX_NUM; ++j) {
			qp_data_addr = (struct iav_qproi_data *)(start_addr + j * SINGLE_QP_MATRIX_SIZE);
			for (i = 0; i < total_size; ++i) {
				if (qp_data_addr[i].qp_offset < QP_OFFSET_MIN ||
					qp_data_addr[i].qp_offset > QP_OFFSET_MAX) {
					iav_debug("Invalid QP offset value [%d] of element [%d] for %c frame on "
						"stream %c.\n", qp_data_addr[i].qp_offset, i,
						((j == 0) ? 'I' : ((j == 1) ? 'P' : 'B')),
						'A' + stream->id_dsp);
					return -1;
				}
			}
		}
	}

	return 0;
}

static inline int check_stream_chroma(struct iav_stream *stream,
	enum iav_chroma_format chroma)
{
	iav_no_check();

	if (stream->format.type == IAV_STREAM_TYPE_H264) {
		if (chroma != H264_CHROMA_YUV420 &&
			chroma != H264_CHROMA_MONO) {
			iav_error("unsupported chroma format %d for h264 stream.\n",
				chroma);
			return -1;
		}
		stream->h264_config.chroma_format = chroma;
	} else if (stream->format.type == IAV_STREAM_TYPE_MJPEG){
		if (chroma != JPEG_CHROMA_YUV420 &&
			chroma != JPEG_CHROMA_MONO) {
			iav_error("unsupported chroma format %d for mjpeg stream.\n",
				chroma);
			return -1;
		}
		stream->mjpeg_config.chroma_format = chroma;
	} else {
		iav_error("unsupported stream type!\n");
		return -1;
	}

	return 0;
}

static inline int check_h264_bitrate(struct iav_bitrate *h264_rc)
{
	iav_no_check();

	if (h264_rc->qp_max_on_I > H264_QP_MAX
		|| h264_rc->qp_min_on_I > h264_rc->qp_max_on_I
		|| h264_rc->qp_max_on_P > H264_QP_MAX
		|| h264_rc->qp_min_on_P > h264_rc->qp_max_on_P
		|| h264_rc->qp_max_on_B > H264_QP_MAX
		|| h264_rc->qp_min_on_B > h264_rc->qp_max_on_B
		|| h264_rc->qp_max_on_Q > H264_QP_MAX
		|| h264_rc->qp_min_on_Q > h264_rc->qp_max_on_Q
		|| h264_rc->adapt_qp > H264_AQP_MAX
		|| h264_rc->i_qp_reduce < H264_I_QP_REDUCE_MIN
		|| h264_rc->i_qp_reduce > H264_I_QP_REDUCE_MAX
		|| h264_rc->p_qp_reduce < H264_P_QP_REDUCE_MIN
		|| h264_rc->p_qp_reduce > H264_P_QP_REDUCE_MAX
		|| h264_rc->q_qp_reduce < H264_Q_QP_REDUCE_MIN
		|| h264_rc->q_qp_reduce > H264_Q_QP_REDUCE_MAX
		|| h264_rc->log_q_num_plus_1 > H264_LOG_Q_NUM_PLUS_1_MAX
		|| h264_rc->max_i_size_KB > H264_I_SIZE_KB_MAX) {
		iav_error("Invalid QP limit, out of range!\n");
		return -1;
	}

	return 0;
}

static int check_qproi(struct iav_qpmatrix *h264_qproi, u8 enable_flag)
{
	iav_no_check();

	if (!h264_qproi->qpm_no_update && !enable_flag) {
		iav_error("Can not update qp matrix when qp matrix is disabled!\n");
		return -1;
	}

	if (h264_qproi->size > STREAM_QP_MATRIX_SIZE) {
		iav_error("QP ROI size %d cannot be larger than %d for stream %c!\n",
			h264_qproi->size, STREAM_QP_MATRIX_SIZE, 'A' + h264_qproi->id);
		return -1;
	}

	return 0;
}

int iav_ioc_start_encode(struct ambarella_iav *iav, void __user *arg)
{
#define WAIT_VSYNC_BEFORE_ENCODING	(4)
	struct dsp_device *dsp = iav->dsp;
	struct amb_dsp_cmd *first, *cmd;
	struct iav_stream *stream;
	u32 update_flags, i, stream_map, vcap_count;
	int rval = 0;

	stream_map = (u32)arg;

	mutex_lock(&iav->enc_mutex);
	mutex_lock(&iav->iav_mutex);

	do {
		if (iav_check_start_encode_state(iav, stream_map) < 0) {
			rval = -EINVAL;
			break;
		}
		iav_vin_update_stream_framerate(iav);
		if (likely(!iav->resume_flag)) {
			vcap_count = (iav->encode_mode == DSP_MULTI_REGION_WARP_MODE) ?
				(WAIT_VSYNC_BEFORE_ENCODING + 1) : WAIT_VSYNC_BEFORE_ENCODING;
			wait_vcap_count(iav, vcap_count);
			iav_debug("Wait %d frames before encode for encode start delay.\n",
				vcap_count);
		}

		if (iav_check_before_start_encode(iav, stream_map) < 0) {
			rval = -EINVAL;
			break;
		}

		/* Start a new travel from preview state. */
		if (iav->state == IAV_STATE_PREVIEW) {
			iav_reset_encode_obj(iav);
			iav_debug("Init encoder when start encoding from preview state.\n");
		}

		/* Guarantee all encode start commans are issued in the same Vsync */
		first = dsp->get_multi_cmds(dsp, IAV_MAX_ENCODE_STREAMS_NUM * 4,
			DSP_CMD_FLAG_BLOCK);
		if (!first) {
			iav_error("Failed to get multiple commands for encode start!\n");
			rval = -ENOMEM;
			break;
		}
		/* set up all streams */
		for (i = 0, cmd = first; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
			if (stream_map & (1 << i)) {
				stream = &iav->stream[i];
				create_session_id(stream);
				cmd_encode_size_setup(stream, cmd);
				get_next_cmd(cmd, first);
				if (stream->format.type == IAV_STREAM_TYPE_H264) {
					cmd_h264_encode_setup(stream, cmd);
				} else {
					cmd_jpeg_encode_setup(stream, cmd);
				}
				get_next_cmd(cmd, first);
				stream->op = IAV_STREAM_OP_START;
			}
		}
		/* start all streams */
		for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
			if (stream_map & (1 << i)) {
				stream = &iav->stream[i];
				if (stream->format.type == IAV_STREAM_TYPE_H264) {
					cmd_h264_encode_start(stream, cmd);
				} else {
					cmd_jpeg_encode_start(stream, cmd);
				}
				get_next_cmd(cmd, first);
				spin_lock_irq(&iav->iav_lock);
				inc_srcbuf_ref(stream->srcbuf, i);
				spin_unlock_irq(&iav->iav_lock);
			}
		}
		/* update parameters for all streams */
		for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
			if (stream_map & (1 << i)) {
				stream = &iav->stream[i];
				update_flags = 0;
				if (stream->format.type == IAV_STREAM_TYPE_H264) {
					update_flags = REALTIME_PARAM_CBR_MODIFY_BIT |
						REALTIME_PARAM_QP_LIMIT_BIT |
						REALTIME_PARAM_INTRA_MB_ROWS_BIT |
						REALTIME_PARAM_INTRA_BIAS_BIT |
						REALTIME_PARAM_ZMV_THRESHOLD_BIT |
						REALTIME_PARAM_FLAT_AREA_BIT |
						REALTIME_PARAM_CUSTOM_VIN_FPS_BIT;
				}
				if (update_flags) {
					cmd_update_encode_params(stream, cmd, update_flags);
					get_next_cmd(cmd, first);
				}
			}
		}
		dsp->put_cmd(dsp, first, 0);

		/* wait untill all streams start up */
		for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
			if (stream_map & (1 << i)) {
				stream = &iav->stream[i];
				if (is_stream_in_starting(stream)) {
					mutex_unlock(&iav->iav_mutex);
					rval = wait_event_interruptible_timeout(stream->venc_wq,
						(stream->dsp_state == ENC_BUSY_STATE),
						FIVE_JIFFIES);
					mutex_lock(&iav->iav_mutex);
					if (rval <= 0) {
						iav_debug("stream %c: wait_event_interruptible_timeout\n",
							'A' + i);
						rval = -EAGAIN;
						break;
					} else {
						rval = 0;
					}
				}
			}
		}
	} while (0);

	mutex_unlock(&iav->iav_mutex);
	mutex_unlock(&iav->enc_mutex);

	return rval;
}

static int iav_check_stop_encode_state(struct ambarella_iav *iav,
	u32 *stream_map)
{
	u32 i, strm_map;
	struct iav_stream *stream;

	if (iav->state != IAV_STATE_ENCODING) {
		iav_error("Can't stop encode, not in encoding state!\n");
		return -EPERM;
	}

	strm_map = *stream_map;
	for (i = 0; i < IAV_STREAM_MAX_NUM_ALL; ++i) {
		if (strm_map & (1 << i)) {
			stream = &iav->stream[i];
			if (!is_stream_in_encoding(stream)) {
				iav_debug("stream %c is already stopped!\n", 'A' + i);
				strm_map &= (~(1 << i));
				continue;
			}
		}
	}
	*stream_map = strm_map;

	return 0;
}

int iav_ioc_stop_encode(struct ambarella_iav *iav, void __user *arg)
{
	struct dsp_device *dsp = iav->dsp;
	struct amb_dsp_cmd *cmd, *first;
	struct iav_stream *stream;
	u32 stream_map = (u32)arg;
	int i, rval = 0;

	mutex_lock(&iav->enc_mutex);
	mutex_lock(&iav->iav_mutex);

	do {
		if ((rval = iav_check_stop_encode_state(iav, &stream_map)) < 0) {
			break;
		}

		first = dsp->get_multi_cmds(dsp, IAV_MAX_ENCODE_STREAMS_NUM,
			DSP_CMD_FLAG_BLOCK);
		if (!first) {
			iav_error("Failed to get multiple commands for encode stop!\n");
			rval = -ENOMEM;
			break;
		}
		/* stop multiple encoding streams at same VSYNC interrupt */
		for (i = 0, cmd = first; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
			stream = &iav->stream[i];
			if ((stream_map & (1 << i)) && is_stream_in_encoding(stream)) {
				cmd_encode_stop(stream, cmd);
				get_next_cmd(cmd, first);

				spin_lock_irq(&iav->iav_lock);
				stream->op = IAV_STREAM_OP_STOP;
				spin_unlock_irq(&iav->iav_lock);
			} else {
				stream_map &= ~(1 << i);
			}
		}
		dsp->put_cmd(dsp, first, 0);

		/* wait for all streams in FREE state to stop */
		for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
			if (stream_map & (1 << i)) {
				stream = &iav->stream[i];
				if (is_stream_in_stopping(stream)) {
					mutex_unlock(&iav->iav_mutex);
					rval = wait_event_interruptible_timeout(stream->venc_wq,
						(stream->dsp_state == ENC_IDLE_STATE),
						FIVE_JIFFIES);
					mutex_lock(&iav->iav_mutex);
					if (rval <= 0) {
						iav_debug("stream %c: wait_event_interruptible_timeout\n",
							'A' + i);
						rval = -EAGAIN;
						break;
					} else {
						rval = 0;
					}
				}
			}
		}
	} while (0);

	mutex_unlock(&iav->iav_mutex);
	mutex_unlock(&iav->enc_mutex);

	return rval;
}

int iav_ioc_abort_encode(struct ambarella_iav *iav, void __user *arg)
{
	u32 i;
	struct iav_stream *stream;
	u32 stream_map;

	stream_map = (u32)arg;

	mutex_lock(&iav->enc_mutex);
	mutex_lock(&iav->iav_mutex);

	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		if (stream_map & (1 << i)) {
			stream = &iav->stream[i];
			if ((is_stream_in_stopping(stream)) ||
				(is_stream_in_starting(stream))) {
				spin_lock_irq(&iav->iav_lock);
				stream->dsp_state = ENC_IDLE_STATE;
				stream->op = IAV_STREAM_OP_STOP;
				stream->fake_dsp_state = ENC_IDLE_STATE;
				stream->session_id = 0;
				spin_unlock_irq(&iav->iav_lock);
			}
		}
	}

	mutex_unlock(&iav->iav_mutex);
	mutex_unlock(&iav->enc_mutex);

	return 0;
}

static int iav_check_stream_format(u32 enc_mode,
	struct iav_stream_format *format)
{
	u32 id, buf_id;
	u32 type, rotate;
	struct iav_rect *enc;

	iav_no_check();

	id = format->id;
	buf_id = format->buf_id;
	type = format->type;
	rotate = format->rotate_cw;
	enc = &format->enc_win;

	if (id >= IAV_STREAM_MAX_NUM_ALL) {
		iav_error("Invalid stream number %d.\n", id);
		return -1;
	}

	switch (type) {
	case IAV_STREAM_TYPE_NONE:
		break;
	case IAV_STREAM_TYPE_H264:
		if ((enc->width & 0xF) || (enc->height & 0x7)) {
			iav_error("H264 stream %c, width %d must be multiple of 16 and "
				"height %d must be multiple of 8.\n",
				'A' + id, enc->width, enc->height);
			return -1;
		}
		break;
	case IAV_STREAM_TYPE_MJPEG:
		if (rotate && ((enc->width & 0x7) || (enc->height & 0xF))) {
			iav_error("After rotation, MJPEG stream %c width %d must be multiple of "
				"16 and height %d must be multiple of 8.\n",
				'A' + id, enc->height, enc->width);
			return -1;
		} else if (!rotate && ((enc->width & 0xF) || (enc->height & 0x7))) {
			iav_error("MJPEG stream %c width %d must be multiple of 16 and height "
				"%d must be multiple of 8.\n", 'A' + id, enc->width, enc->height);
			return -1;
		}
		break;
	default:
		iav_error("Invalid encode type %d for stream %c.\n", type, 'A' + id);
		return -1;
		break;
	}

	if ((buf_id < IAV_SRCBUF_FIRST || buf_id >= IAV_SRCBUF_LAST) &&
		(buf_id != IAV_SRCBUF_EFM)) {
		iav_error("Invalid buffer id %d for stream %c.\n", buf_id, 'A' + id);
		return -1;
	}

	return 0;
}

int iav_ioc_s_stream_format(struct ambarella_iav *iav, void __user *arg)
{
	int rval = 0;
	u32 enc_mode = iav->encode_mode;
	int rotatable = iav->system_config[enc_mode].rotatable;
	struct iav_stream_format format, *orig;

	if (copy_from_user(&format, arg, sizeof(format)))
		return -EFAULT;

	if (iav_check_stream_format(enc_mode, &format) < 0) {
		return -EINVAL;
	}

	mutex_lock(&iav->iav_mutex);

	if (!rotatable && format.rotate_cw) {
		iav_error("Cannot rotate stream %c when rotation is disabled!\n",
			'A' + format.id);
		rval = -EINVAL;
		goto SET_STREAM_FORMAT_EXIT;
	}
	orig = &iav->stream[format.id].format;

	if (is_stream_format_changed(orig, &format)) {
		if (is_stream_in_encoding(&iav->stream[format.id])) {
			if (is_stream_offset_changed_only(orig, &format)) {
				if (check_stream_offset(iav, &format) < 0) {
					rval = -EINVAL;
					goto SET_STREAM_FORMAT_EXIT;
				}
				*orig = format;
				iav->stream[format.id].srcbuf = &iav->srcbuf[format.buf_id];
				cmd_encode_size_setup(&iav->stream[format.id], NULL);
			} else {
				iav_error("Cannot change stream %c format except offset when "
					"it's in encoding.\n", 'A' + format.id);
				rval = -EINVAL;
				goto SET_STREAM_FORMAT_EXIT;
			}
		} else {
			*orig = format;
			iav->stream[format.id].srcbuf = &iav->srcbuf[format.buf_id];
		}
	}

SET_STREAM_FORMAT_EXIT:
	mutex_unlock(&iav->iav_mutex);
	return rval;
}

int iav_ioc_g_stream_format(struct ambarella_iav *iav, void __user *arg)
{
	struct iav_stream_format format;

	if (copy_from_user(&format, arg, sizeof(format)))
		return -EFAULT;

	if (format.id >= IAV_STREAM_MAX_NUM_ALL) {
		iav_error("Invalid stream ID %d.\n", format.id);
		return -EINVAL;
	}

	mutex_lock(&iav->iav_mutex);
	format = iav->stream[format.id].format;
	mutex_unlock(&iav->iav_mutex);

	return copy_to_user(arg, &format, sizeof(format)) ? -EFAULT : 0;
}

int iav_ioc_g_stream_info(struct ambarella_iav *iav, void __user *arg)
{
	struct iav_stream_info info;

	if (copy_from_user(&info, arg, sizeof(struct iav_stream_info)))
		return -EFAULT;

	if (info.id >= IAV_STREAM_MAX_NUM_ALL) {
		iav_error("Invalid stream ID %d.\n", info.id);
		return -EINVAL;
	}

	mutex_lock(&iav->iav_mutex);
	info.state = get_stream_encode_state(&iav->stream[info.id]);
	mutex_unlock(&iav->iav_mutex);

	return copy_to_user(arg, &info, sizeof(info)) ? -EFAULT : 0;
}

static int iav_set_stream_fps(struct ambarella_iav *iav, u32 stream_id,
	struct iav_stream_fps *fps)
{
	struct iav_stream *stream;
	struct iav_stream_fps orig;
	u32 update_flags, stream_map;

	stream = &iav->stream[stream_id];
	orig = stream->fps;

	if ((fps->abs_fps_enable || orig.abs_fps_enable) &&
		((fps->fps_div != orig.fps_div) || (fps->fps_multi != orig.fps_multi))) {
		iav_error("Cannot set frame factor when abs fps enabled.\n");
		return -EINVAL;
	}

	if (check_stream_fps(iav, fps) < 0) {
		return -EINVAL;
	}
	stream->fps = *fps;

	if (is_stream_in_encoding(stream)) {
		if (fps->abs_fps_enable == 1) {
			iav_error("Stream [%d] is encoding, cannot set abs fps!\n", stream_id);
			stream->fps = orig;
			return -EINVAL;
		}
		stream_map = 1 << stream_id;
		if (check_encode_resource_limit(iav, stream_map) < 0) {
			iav_error("Change frame factor failed, system resource not enough!\n");
			stream->fps = orig;
			return -EINVAL;
		}
		// check total DSP memory bandwidth/size
		if (iav_check_sys_mem_usage(iav, (int)IAV_STATE_ENCODING, stream_map) < 0) {
			iav_error("Change frame factor failed, DSP memory check failed.\n");
			stream->fps = orig;
			return -EINVAL;
		}
		// apply stream fps
		update_flags = REALTIME_PARAM_CUSTOM_FRAME_RATE_BIT |
			REALTIME_PARAM_CUSTOM_VIN_FPS_BIT;
		if (stream->format.type == IAV_STREAM_TYPE_H264) {
			update_flags |= REALTIME_PARAM_CBR_MODIFY_BIT;
		}
		if (cmd_update_encode_params(stream, NULL, update_flags) < 0) {
			return -EIO;
		}
	}

	return 0;
}

static int iav_set_stream_offset(struct ambarella_iav *iav, u32 stream_id,
	struct iav_offset *offset)
{
	struct iav_stream *stream = &iav->stream[stream_id];
	struct iav_stream_format orig;

	orig = stream->format;
	stream->format.enc_win.x = offset->x;
	stream->format.enc_win.y = offset->y;
	if (check_stream_offset(iav, &stream->format) < 0) {
		stream->format = orig;
		return -EINVAL;
	}

	if (is_stream_in_encoding(stream)) {
		cmd_encode_size_setup(stream, NULL);
	}

	return 0;
}

static int iav_set_stream_chroma(struct ambarella_iav *iav, u32 stream_id,
	enum iav_chroma_format chroma)
{
	struct iav_stream *stream;

	stream = &iav->stream[stream_id];

	if (check_stream_chroma(stream, chroma) < 0) {
		return -EINVAL;
	}

	if (is_stream_in_encoding(stream)) {
		if (cmd_update_encode_params(stream, NULL,
			REALTIME_PARAM_MONOCHROME_BIT) < 0)
		return -EIO;
	}

	return 0;
}

static int iav_set_h264_gop(struct ambarella_iav *iav, u32 stream_id,
	struct iav_h264_gop *h264_gop, int cmd_delay)
{
	struct iav_stream *stream;
	struct iav_h264_config updated;

	stream = &iav->stream[stream_id];

	updated = stream->h264_config;
	updated.N = h264_gop->N;
	updated.idr_interval = h264_gop->idr_interval;

	if (iav_check_h264_config(iav, stream_id, &updated) < 0) {
		return -EINVAL;
	} else {
		stream->h264_config = updated;
	}

	if (is_stream_in_encoding(stream) &&
		(stream->format.type == IAV_STREAM_TYPE_H264)) {
		if (!cmd_delay) {
			if (cmd_update_encode_params(stream, NULL, REALTIME_PARAM_GOP_BIT) < 0)
				return -EIO;
		} else {
			if (update_sync_encode_params(stream, NULL, REALTIME_PARAM_GOP_BIT) < 0)
				return -EIO;
		}
	}

	return 0;
}

static int iav_set_h264_bitrate(struct ambarella_iav *iav, u32 stream_id,
	struct iav_bitrate *h264_rc, int cmd_delay)
{
	struct iav_stream *stream;
	struct iav_h264_config *h264_cfg;
	struct iav_h264_config orig;
	u32 update_flags, stream_map;

	if (check_h264_bitrate(h264_rc) < 0) {
		return -EINVAL;
	}

	stream = &iav->stream[stream_id];
	orig = stream->h264_config;
	h264_cfg = &stream->h264_config;
	switch (h264_rc->vbr_setting) {
	case IAV_BRC_CBR:
		h264_cfg->vbr_setting = IAV_BRC_CBR;
		break;
	case IAV_BRC_PCBR:
		h264_cfg->vbr_setting = IAV_BRC_PCBR;
		break;
	case IAV_BRC_SCBR:
		h264_cfg->vbr_setting = IAV_BRC_SCBR;
		break;
	case IAV_BRC_VBR:
		h264_cfg->vbr_setting = IAV_BRC_VBR;
		break;
	default:
		iav_error("Unknown rate control mode!\n");
		return -EINVAL;
		break;
	}

	h264_cfg->average_bitrate = h264_rc->average_bitrate;
	h264_cfg->qp_min_on_I = h264_rc->qp_min_on_I;
	h264_cfg->qp_max_on_I = h264_rc->qp_max_on_I;
	h264_cfg->qp_min_on_P = h264_rc->qp_min_on_P;
	h264_cfg->qp_max_on_P = h264_rc->qp_max_on_P;
	h264_cfg->qp_min_on_B = h264_rc->qp_min_on_B;
	h264_cfg->qp_max_on_B = h264_rc->qp_max_on_B;
	h264_cfg->qp_min_on_Q = h264_rc->qp_min_on_Q;
	h264_cfg->qp_max_on_Q = h264_rc->qp_max_on_Q;
	h264_cfg->i_qp_reduce = h264_rc->i_qp_reduce;
	h264_cfg->p_qp_reduce = h264_rc->p_qp_reduce;
	h264_cfg->q_qp_reduce = h264_rc->q_qp_reduce;
	h264_cfg->log_q_num_plus_1 = h264_rc->log_q_num_plus_1;
	h264_cfg->adapt_qp = h264_rc->adapt_qp;
	h264_cfg->skip_flag = h264_rc->skip_flag;
	h264_cfg->max_I_size_KB = h264_rc->max_i_size_KB;

	if (is_stream_in_encoding(stream)) {
		stream_map = 1 << stream_id;
		// check encode resource limit
		if (check_encode_resource_limit(iav, stream_map) < 0) {
			iav_error("Change bitrate failed, system resource not enough!\n");
			*h264_cfg = orig;
			return -EINVAL;
		}
		// apply stream bitrate change
		if (stream->format.type == IAV_STREAM_TYPE_H264) {
			update_flags = REALTIME_PARAM_CBR_MODIFY_BIT |
				REALTIME_PARAM_QP_LIMIT_BIT;
			if (!cmd_delay) {
				if (cmd_update_encode_params(stream, NULL, update_flags) < 0)
					return -EIO;
			} else {
				if (update_sync_encode_params(stream, NULL, update_flags) < 0)
					return -EIO;
			}
		}
	}

	return 0;
}

static int iav_set_h264_roi(struct ambarella_iav *iav, u32 stream_id,
	struct iav_qpmatrix *h264_roi, int cmd_delay)
{
	struct iav_stream *stream;
	struct iav_h264_config *config;
	struct iav_cmd_sync *cmd_sync;
	u32 src_offset, dst_offset, size, buff_id;
	int i;

	stream = &iav->stream[stream_id];
	config = &stream->h264_config;

	if (check_qp_matrix_param(stream, h264_roi) < 0) {
		iav_error("QP ROI matrix parameters error!\n");
		return -EINVAL;
	}

	config->qp_roi_enable = h264_roi->enable;
	for (i = 0; i < NUM_PIC_TYPES; ++i) {
		config->qp_delta[i][0] = h264_roi->qp_delta[i][0];
		config->qp_delta[i][1] = h264_roi->qp_delta[i][1];
		config->qp_delta[i][2] = h264_roi->qp_delta[i][2];
		config->qp_delta[i][3] = h264_roi->qp_delta[i][3];
	}

	/* now update QP matrix */
	if (is_stream_in_encoding(stream) &&
		(stream->format.type == IAV_STREAM_TYPE_H264)) {
		if (!cmd_delay) {
			cmd_update_encode_params(stream, NULL,
				REALTIME_PARAM_QP_ROI_MATRIX_BIT);
		} else {
			cmd_sync = &iav->cmd_sync;
			if (!h264_roi->qpm_no_update) {
				src_offset = stream_id * STREAM_QP_MATRIX_SIZE;
				dst_offset = cmd_sync->qpm_idx * QP_MATRIX_SIZE +
					stream_id * STREAM_QP_MATRIX_SIZE;
				buff_id = IAV_BUFFER_QPMATRIX;
				size = STREAM_QP_MATRIX_SIZE;
				if (iav_mem_copy(iav, buff_id, src_offset, dst_offset,
					size, KByte(1)) < 0) {
					return -EFAULT;
				}
				cmd_sync->qpm_flag = 1;
			}
			update_sync_encode_params(stream, NULL,
				REALTIME_PARAM_QP_ROI_MATRIX_BIT);
		}
	}

	return 0;
}

static int iav_get_h264_roi(struct ambarella_iav * iav, u32 stream_id,
	struct iav_qpmatrix *h264_roi, int cmd_delay)
{
	struct iav_stream *stream;
	struct iav_h264_config *h264_cfg;
	u32 src_offset, dest_offset, size, buff_id;
	int i, qpm_idx;

	stream = &iav->stream[stream_id];
	h264_cfg = &stream->h264_config;

	if (check_qproi(h264_roi, h264_cfg->qp_roi_enable) < 0) {
		return -EINVAL;
	}

	h264_roi->id = stream_id;
	h264_roi->enable = h264_cfg->qp_roi_enable;
	for (i = 0; i < NUM_PIC_TYPES; ++i) {
		h264_roi->qp_delta[i][0] = h264_cfg->qp_delta[i][0];
		h264_roi->qp_delta[i][1] = h264_cfg->qp_delta[i][1];
		h264_roi->qp_delta[i][2] = h264_cfg->qp_delta[i][2];
		h264_roi->qp_delta[i][3] = h264_cfg->qp_delta[i][3];
	}

	dest_offset  = stream_id * STREAM_QP_MATRIX_SIZE;
	h264_roi->data_offset = dest_offset;
	h264_roi->size = STREAM_QP_MATRIX_SIZE;
	/* only copy qp matrix when updated for frame sync case,
	 * for non frame sync case, no need to copy because the
	 * qp matrix buffer is shared
	 */
	if (cmd_delay && !h264_roi->qpm_no_update) {
		/* get the qp matrix that is already applied */
		qpm_idx = (iav->cmd_sync.qpm_idx + QP_MATRIX_TOGGLED_NUM - 2) %
			QP_MATRIX_TOGGLED_NUM + 1;
		src_offset  = qpm_idx * QP_MATRIX_SIZE +
			stream_id * STREAM_QP_MATRIX_SIZE;
		buff_id = IAV_BUFFER_QPMATRIX;
		size = STREAM_QP_MATRIX_SIZE;
		if (iav_mem_copy(iav, buff_id, src_offset, dest_offset,
			size, KByte(1)) < 0) {
			return -EFAULT;
		}
	}

	return 0;
}

static int iav_set_h264_force_idr(struct ambarella_iav *iav, u32 stream_id,
	int force_idr, int cmd_delay)
{
	struct iav_stream *stream = &iav->stream[stream_id];

	if (is_stream_in_encoding(stream) &&
		(stream->format.type == IAV_STREAM_TYPE_H264)) {
		if (!cmd_delay) {
			cmd_update_encode_params(stream, NULL,
				REALTIME_PARAM_FORCE_IDR_BIT);
		} else {
			update_sync_encode_params(stream, NULL,
				REALTIME_PARAM_FORCE_IDR_BIT);
		}
	}

	return 0;
}

static int iav_set_h264_force_fast_seek(struct ambarella_iav *iav, u32 stream_id,
	u32 fast_seek, int cmd_delay)
{
	struct iav_stream *stream = &iav->stream[stream_id];

	if (check_force_fast_seek(stream) < 0) {
		return -EINVAL;
	}

	if (is_stream_in_encoding(stream) &&
		(stream->format.type == IAV_STREAM_TYPE_H264)) {
		if (!cmd_delay) {
			cmd_update_encode_params(stream, NULL,
				REALTIME_PARAM_FORCE_FAST_SEEK_BIT);
		} else {
			update_sync_encode_params(stream, NULL,
				REALTIME_PARAM_FORCE_FAST_SEEK_BIT);
		}
	}

	return 0;
}

static int iav_set_h264_frame_drop(struct ambarella_iav *iav, u32 stream_id,
	u32 drop_frames, int cmd_delay)
{
	struct iav_stream *stream = &iav->stream[stream_id];

	if (check_frame_drop(stream, drop_frames) < 0) {
		return -EINVAL;
	}

	stream->h264_config.drop_frames = drop_frames;

	if (is_stream_in_encoding(stream) &&
		(stream->format.type == IAV_STREAM_TYPE_H264)) {
		if (!cmd_delay) {
			cmd_update_encode_params(stream, NULL,
				REALTIME_PARAM_FRAME_DROP_BIT);
		} else {
			update_sync_encode_params(stream, NULL,
				REALTIME_PARAM_FRAME_DROP_BIT);
		}
	}

	return 0;
}

static int iav_set_h264_zmv_threshold(struct ambarella_iav *iav, u32 stream_id,
	u32 mv_threshold, int cmd_delay)
{
	struct iav_stream *stream;

	stream = &iav->stream[stream_id];

	if (mv_threshold > MAX_ZMV_THRESHOLD) {
		iav_error("Invalid h264 mv threshold value: %u!\n", mv_threshold);
		return -EINVAL;
	}

	stream->h264_config.zmv_threshold = (u8)mv_threshold;

	if (is_stream_in_encoding(stream) &&
		(stream->format.type == IAV_STREAM_TYPE_H264)) {
		if (!cmd_delay) {
			cmd_update_encode_params(stream, NULL,
				REALTIME_PARAM_ZMV_THRESHOLD_BIT);
		} else {
			update_sync_encode_params(stream, NULL,
				REALTIME_PARAM_ZMV_THRESHOLD_BIT);
		}
	}

	return 0;
}

static int iav_set_h264_flat_area_improve(struct ambarella_iav *iav,
	u32 stream_id, int flat_area_improve, int cmd_delay)
{
	struct iav_stream *stream = &iav->stream[stream_id];

	stream->h264_config.flat_area_improve = flat_area_improve ? 1 : 0;

	if (is_stream_in_encoding(stream) &&
		(stream->format.type == IAV_STREAM_TYPE_H264)) {
		if (!cmd_delay) {
			cmd_update_encode_params(stream, NULL,
				REALTIME_PARAM_FLAT_AREA_BIT);
		} else {
			update_sync_encode_params(stream, NULL,
				REALTIME_PARAM_FLAT_AREA_BIT);
		}
	}

	return 0;
}

static int iav_set_h264_statis(struct ambarella_iav *iav,
	u32 stream_id, int statis, int cmd_delay)
{
	struct iav_stream *stream = &iav->stream[stream_id];

	stream->h264_config.statis = statis ? 1 : 0;

	if (is_stream_in_encoding(stream) &&
		(stream->format.type == IAV_STREAM_TYPE_H264)) {
		if (!cmd_delay) {
			cmd_update_encode_params(stream, NULL,
				REALTIME_PARAM_MV_DUMP_BIT);
		} else {
			update_sync_encode_params(stream, NULL,
				REALTIME_PARAM_MV_DUMP_BIT);
		}
	}

	return 0;
}

static int iav_set_h264_long_ref_p(struct ambarella_iav *iav,
	u32 stream_id, int cmd_delay)
{
	struct iav_stream *stream = &iav->stream[stream_id];

	if (stream->h264_config.gop_structure != IAV_GOP_LT_REF_P){
		iav_error("Only support Long ref P when gop = %d!\n", IAV_GOP_LT_REF_P);
		return -EINVAL;
	}

	if (is_stream_in_encoding(stream) &&
		(stream->format.type == IAV_STREAM_TYPE_H264)) {
		if (!cmd_delay) {
			cmd_update_encode_params(stream, NULL,
				REALTIME_PARAM_LONG_REF_P_BIT);
		} else {
			update_sync_encode_params(stream, NULL,
				REALTIME_PARAM_LONG_REF_P_BIT);
		}
	}

	return 0;
}

static int iav_set_h264_rc_strategy(struct ambarella_iav *iav,
	u32 stream_id, struct iav_rc_strategy *h264_rc_strategy)
{
	struct iav_stream *stream = &iav->stream[stream_id];

	if (!is_stream_in_idle(stream)) {
		iav_error("Stream %c is not in idle when setting bitrate_autosync_disable.\n",
			'A' + stream_id);
		return -EINVAL;
	}

	if (stream->format.type == IAV_STREAM_TYPE_H264) {
		stream->h264_config.abs_br_flag = (u8)h264_rc_strategy->abs_br_flag;
	}

	return 0;
}

static int iav_set_h264_force_pskip(struct ambarella_iav *iav, u32 stream_id,
	struct iav_h264_pskip *h264_pskip, int cmd_delay)
{
	struct iav_stream *stream;
	int rpt_enable, rpt_num;

	stream = &iav->stream[stream_id];
	rpt_enable = h264_pskip->repeat_enable;
	rpt_num = h264_pskip->repeat_num;

	if (rpt_enable && (rpt_num < 0 || rpt_num > MAX_PSKIP_REPEAT)) {
		iav_error("Repeat pskip number should between [0~%d).\n", MAX_PSKIP_REPEAT);
		return -EINVAL;
	}

	stream->h264_config.pskip_repeat_enable = rpt_enable;
	stream->h264_config.repeat_pskip_num = rpt_enable ? rpt_num : 0;

	if (is_stream_in_encoding(stream) &&
		(stream->format.type == IAV_STREAM_TYPE_H264)) {
		if (!cmd_delay) {
			cmd_update_encode_params(stream, NULL,
				REALTIME_PARAM_FORCE_PSKIP_BIT);
		} else {
			update_sync_encode_params(stream, NULL,
				REALTIME_PARAM_FORCE_PSKIP_BIT);
		}
	}

	return 0;
}

static int iav_set_h264_enc_param(struct ambarella_iav *iav, u32 stream_id,
	struct iav_h264_enc_param *h264_enc, int cmd_delay)
{
	struct iav_stream *stream;
	struct iav_h264_config *config;
	u32 update_flags = 0;


	stream = &iav->stream[stream_id];
	if (check_h264_enc_param(stream, h264_enc) < 0) {
		return -EINVAL;
	}

	config = &stream->h264_config;
	if (h264_enc->intrabias_p != config->intrabias_p ||
		h264_enc->intrabias_b != config->intrabias_b) {
		config->intrabias_p = h264_enc->intrabias_p;
		config->intrabias_b = h264_enc->intrabias_b;
		update_flags |= REALTIME_PARAM_INTRA_BIAS_BIT;
	}
	if (h264_enc->user1_intra_bias != config->user1_intra_bias ||
		h264_enc->user1_direct_bias != config->user1_direct_bias ||
		h264_enc->user2_intra_bias != config->user2_intra_bias ||
		h264_enc->user2_direct_bias != config->user2_direct_bias ||
		h264_enc->user3_intra_bias != config->user3_intra_bias ||
		h264_enc->user3_direct_bias != config->user3_direct_bias) {
		config->user1_intra_bias = h264_enc->user1_intra_bias;
		config->user1_direct_bias = h264_enc->user1_direct_bias;
		config->user2_intra_bias = h264_enc->user2_intra_bias;
		config->user2_direct_bias = h264_enc->user2_direct_bias;
		config->user3_intra_bias = h264_enc->user3_intra_bias;
		config->user3_direct_bias = h264_enc->user3_direct_bias;
		update_flags |= REALTIME_PARAM_QP_ROI_MATRIX_BIT;
	}
	/* FixMe, if no update, then update all */
	if (!update_flags) {
		update_flags = REALTIME_PARAM_INTRA_BIAS_BIT |
			REALTIME_PARAM_QP_ROI_MATRIX_BIT;
	}
	if (is_stream_in_encoding(stream) &&
		(stream->format.type == IAV_STREAM_TYPE_H264) &&
		update_flags) {
		if (!cmd_delay) {
			cmd_update_encode_params(stream, NULL, update_flags);
		} else {
			update_sync_encode_params(stream, NULL, update_flags);
		}
	}

	return 0;
}

static int iav_set_mjpeg_quality(struct ambarella_iav *iav, u32 stream_id,
	u32 quality)
{
	struct iav_stream *stream;

	stream = &iav->stream[stream_id];

	if (quality == 0 || quality > 100){
		iav_error("Invalid mjpeg quality value: %u!\n", quality);
		return -EINVAL;
	}

	stream->mjpeg_config.quality = quality;

	if (is_stream_in_encoding(stream)) {
		if(stream->format.type == IAV_STREAM_TYPE_MJPEG) {
			cmd_update_encode_params(stream, NULL,
				REALTIME_PARAM_QUANT_MATRIX_BIT);
		}
	}

	return 0;
}

int iav_ioc_s_stream_cfg(struct ambarella_iav *iav, void __user *arg)
{
	struct iav_stream_cfg cfg;
	int rval = 0;

	if (copy_from_user(&cfg, arg, sizeof(cfg)))
			return -EFAULT;

	if (cfg.id >= IAV_STREAM_MAX_NUM_ALL) {
		iav_error("Invalid stream ID: %d!\n", cfg.id);
		return -EINVAL;
	}

	mutex_lock(&iav->iav_mutex);

	switch (cfg.cid) {
	case IAV_STMCFG_FPS:
		rval = iav_set_stream_fps(iav, cfg.id, &cfg.arg.fps);
		break;
	case IAV_STMCFG_OFFSET:
		rval = iav_set_stream_offset(iav, cfg.id, &cfg.arg.enc_offset);
		break;
	case IAV_STMCFG_CHROMA:
		rval = iav_set_stream_chroma(iav, cfg.id, cfg.arg.chroma);
		break;
	case IAV_H264_CFG_GOP:
		rval = iav_set_h264_gop(iav, cfg.id, &cfg.arg.h264_gop, 0);
		break;
	case IAV_H264_CFG_BITRATE:
		rval = iav_set_h264_bitrate(iav, cfg.id, &cfg.arg.h264_rc, 0);
		break;
	case IAV_H264_CFG_QP_ROI:
		rval = iav_set_h264_roi(iav, cfg.id, &cfg.arg.h264_roi, 0);
		break;
	case IAV_H264_CFG_FORCE_IDR:
		rval = iav_set_h264_force_idr(iav, cfg.id,
			cfg.arg.h264_force_idr, 0);
		break;
	case IAV_H264_CFG_ZMV_THRESHOLD:
		rval = iav_set_h264_zmv_threshold(iav, cfg.id,
			cfg.arg.mv_threshold, 0);
		break;
	case IAV_H264_CFG_FLAT_AREA_IMPROVE:
		rval = iav_set_h264_flat_area_improve(iav, cfg.id,
			cfg.arg.h264_flat_area_improve, 0);
		break;
	case IAV_H264_CFG_FORCE_FAST_SEEK:
		rval = iav_set_h264_force_fast_seek(iav, cfg.id,
			cfg.arg.h264_force_fast_seek, 0);
		break;
	case IAV_H264_CFG_FRAME_DROP:
		rval = iav_set_h264_frame_drop(iav, cfg.id,
			cfg.arg.h264_drop_frames, 0);
		break;
	case IAV_H264_CFG_ENC_PARAM:
		rval = iav_set_h264_enc_param(iav, cfg.id,
			&cfg.arg.h264_enc, 0);
		break;
	case IAV_H264_CFG_STATIS:
		rval = iav_set_h264_statis(iav, cfg.id,
			cfg.arg.h264_statis, 0);
		break;
	case IAV_MJPEG_CFG_QUALITY:
		rval = iav_set_mjpeg_quality(iav, cfg.id, cfg.arg.mjpeg_quality);
		break;
	case IAV_H264_CFG_LONG_REF_P:
		rval = iav_set_h264_long_ref_p(iav, cfg.id, 0);
		break;
	case IAV_H264_CFG_RC_STRATEGY:
		rval = iav_set_h264_rc_strategy(iav, cfg.id, &cfg.arg.h264_rc_strategy);
		break;
	case IAV_H264_CFG_FORCE_PSKIP:
		rval = iav_set_h264_force_pskip(iav, cfg.id, &cfg.arg.h264_pskip, 0);
		break;
	default:
		iav_error("Unknown cid: 0x%X!\n", cfg.cid);
		rval = -EINVAL;
		break;
	}

	mutex_unlock(&iav->iav_mutex);

	return rval;
}

int iav_ioc_g_stream_cfg(struct ambarella_iav *iav, void __user *args)
{
	struct iav_stream_cfg cfg;
	struct iav_stream *stream;
	struct iav_bitrate *h264_rc;
	struct iav_h264_config *h264_cfg;
	struct iav_h264_enc_param *h264_enc;
	struct iav_qpmatrix *h264_roi;

	if (copy_from_user(&cfg, args, sizeof(cfg)))
		return -EFAULT;

	if (cfg.id >= IAV_STREAM_MAX_NUM_ALL) {
		iav_error("Invalid stream ID: %d!\n", cfg.id);
		return -EINVAL;
	}

	stream = &iav->stream[cfg.id];

	mutex_lock(&iav->iav_mutex);

	switch (cfg.cid) {
	case IAV_STMCFG_FPS:
		cfg.arg.fps = stream->fps;
		break;

	case IAV_STMCFG_OFFSET:
		cfg.arg.enc_offset.x = stream->format.enc_win.x;
		cfg.arg.enc_offset.y = stream->format.enc_win.y;
		break;

	case IAV_STMCFG_CHROMA:
		if (stream->format.type == IAV_STREAM_TYPE_H264) {
			cfg.arg.chroma = stream->h264_config.chroma_format;
		} else {
			cfg.arg.chroma = stream->mjpeg_config.chroma_format;
		}
		break;

	case IAV_H264_CFG_GOP:
		cfg.arg.h264_gop.id = cfg.id;
		cfg.arg.h264_gop.idr_interval = stream->h264_config.idr_interval;
		cfg.arg.h264_gop.N = stream->h264_config.N;
		break;

	case IAV_H264_CFG_BITRATE:
		h264_rc = &cfg.arg.h264_rc;
		h264_cfg = &stream->h264_config;

		h264_rc->id = cfg.id;
		h264_rc->vbr_setting = h264_cfg->vbr_setting;
		h264_rc->average_bitrate = h264_cfg->average_bitrate;
		h264_rc->qp_min_on_I = h264_cfg->qp_min_on_I;
		h264_rc->qp_max_on_I = h264_cfg->qp_max_on_I;
		h264_rc->qp_min_on_P = h264_cfg->qp_min_on_P;
		h264_rc->qp_max_on_P = h264_cfg->qp_max_on_P;
		h264_rc->qp_min_on_B = h264_cfg->qp_min_on_B;
		h264_rc->qp_max_on_B = h264_cfg->qp_max_on_B;
		h264_rc->qp_min_on_Q = h264_cfg->qp_min_on_Q;
		h264_rc->qp_max_on_Q = h264_cfg->qp_max_on_Q;
		h264_rc->i_qp_reduce = h264_cfg->i_qp_reduce;
		h264_rc->p_qp_reduce = h264_cfg->p_qp_reduce;
		h264_rc->q_qp_reduce = h264_cfg->q_qp_reduce;
		h264_rc->log_q_num_plus_1 = h264_cfg->log_q_num_plus_1;
		h264_rc->adapt_qp = h264_cfg->adapt_qp;
		h264_rc->skip_flag = h264_cfg->skip_flag;
		h264_rc->max_i_size_KB = h264_cfg->max_I_size_KB;
		break;

	case IAV_H264_CFG_ZMV_THRESHOLD:
		cfg.arg.mv_threshold = stream->h264_config.zmv_threshold;
		break;

	case IAV_H264_CFG_FLAT_AREA_IMPROVE:
		cfg.arg.h264_flat_area_improve = stream->h264_config.flat_area_improve;
		break;

	case IAV_H264_CFG_QP_ROI:
		h264_roi = &cfg.arg.h264_roi;
		iav_get_h264_roi(iav, cfg.id, h264_roi, 0);
		break;

	case IAV_H264_CFG_ENC_PARAM:
		h264_enc = &cfg.arg.h264_enc;
		h264_cfg = &stream->h264_config;

		h264_enc->id = cfg.id;
		h264_enc->intrabias_p = h264_cfg->intrabias_p;
		h264_enc->intrabias_b = h264_cfg->intrabias_b;
		h264_enc->user1_intra_bias = h264_cfg->user1_intra_bias;
		h264_enc->user1_direct_bias = h264_cfg->user1_direct_bias;
		h264_enc->user2_intra_bias = h264_cfg->user2_intra_bias;
		h264_enc->user2_direct_bias = h264_cfg->user2_direct_bias;
		h264_enc->user3_intra_bias = h264_cfg->user3_intra_bias;
		h264_enc->user3_direct_bias = h264_cfg->user3_direct_bias;
		break;

	case IAV_H264_CFG_STATIS:
		cfg.arg.h264_statis = stream->h264_config.statis;
		break;

	case IAV_MJPEG_CFG_QUALITY:
		cfg.arg.mjpeg_quality = stream->mjpeg_config.quality;
		break;

	case IAV_H264_CFG_RC_STRATEGY:
		h264_cfg = &stream->h264_config;
		cfg.arg.h264_rc_strategy.abs_br_flag = h264_cfg->abs_br_flag;
		break;

	case IAV_H264_CFG_FORCE_PSKIP:
		cfg.arg.h264_pskip.repeat_enable = stream->h264_config.pskip_repeat_enable;
		cfg.arg.h264_pskip.repeat_num = stream->h264_config.repeat_pskip_num;
		break;

	default:
		mutex_unlock(&iav->iav_mutex);
		iav_error("Invalid cid: 0x%X\n", cfg.cid);
		return -EINVAL;
	}

	mutex_unlock(&iav->iav_mutex);

	if (copy_to_user(args, &cfg, sizeof(cfg)))
		return -EFAULT;

	return 0;
}

int iav_ioc_s_stream_fps_sync(struct ambarella_iav *iav, void __user *arg)
{
	struct iav_stream_fps_sync fps_sync;
	struct iav_stream_fps orig[IAV_STREAM_MAX_NUM_ALL];
	struct iav_stream *stream;
	struct dsp_device *dsp = iav->dsp;
	struct amb_dsp_cmd *cmd, *first;
	int i;
	int error = 0;
	u32 update_flags;

	if (copy_from_user(&fps_sync, arg, sizeof(fps_sync)))
		return -EFAULT;

	mutex_lock(&iav->iav_mutex);

	for (i = 0; i < IAV_STREAM_MAX_NUM_ALL; ++i) {
		if (fps_sync.enable[i]) {
			stream = &iav->stream[i];
			if ((fps_sync.fps[i].abs_fps_enable || stream->fps.abs_fps_enable) &&
				((fps_sync.fps[i].fps_div != stream->fps.fps_div) ||
				(fps_sync.fps[i].fps_multi != stream->fps.fps_multi))) {
				iav_error("stream fps sync failed for stream [%d] abs fps enabled.\n", i);
				mutex_unlock(&iav->iav_mutex);
				return -EINVAL;
			}
			if (check_stream_fps(iav, &fps_sync.fps[i]) < 0) {
				iav_error("Invalid stream fps for Stream %c.\n", 'A' + i);
				mutex_unlock(&iav->iav_mutex);
				return -EINVAL;
			}
			orig[i] = stream->fps;
			stream->fps = fps_sync.fps[i];
		}
	}

	if (check_encode_resource_limit(iav, 0) < 0) {
		iav_error("Sync frame factor failed, system resource not enough!\n");
		error = 1;
	} else if (iav_check_sys_mem_usage(iav, (int)IAV_STATE_ENCODING, 0) < 0) {
		iav_error("Sync frame factor failed, DSP memory check failed.\n");
		error = 1;
	}

	if (error == 1) {
		for (i = 0; i < IAV_STREAM_MAX_NUM_ALL; ++i) {
			if (fps_sync.enable[i]) {
				stream = &iav->stream[i];
				stream->fps = orig[i];
			}
		}
		mutex_unlock(&iav->iav_mutex);
		return -EINVAL;
	}

	first = dsp->get_multi_cmds(dsp,
			IAV_MAX_ENCODE_STREAMS_NUM, DSP_CMD_FLAG_BLOCK);
	if (!first) {
		iav_error("Failed to get multiple commands for encode stop!\n");
		mutex_unlock(&iav->iav_mutex);
		return -ENOMEM;
	}

	for (i = 0, cmd = first; i < IAV_STREAM_MAX_NUM_ALL; ++i) {
		if (fps_sync.enable[i]) {
			stream = &iav->stream[i];
			update_flags = REALTIME_PARAM_CUSTOM_FRAME_RATE_BIT |
				REALTIME_PARAM_CUSTOM_VIN_FPS_BIT;
			if (stream->format.type == IAV_STREAM_TYPE_H264) {
				update_flags |= REALTIME_PARAM_CBR_MODIFY_BIT;
			}
			if (cmd_update_encode_params(stream, NULL, update_flags) < 0) {
				mutex_unlock(&iav->iav_mutex);
				return -EIO;
			}
			get_next_cmd(cmd, first);
		}
	}
	dsp->put_cmd(dsp,  first, 0);

	mutex_unlock(&iav->iav_mutex);

	return 0;
}

int iav_ioc_s_h264_cfg(struct ambarella_iav *iav, void __user *args)
{
	struct iav_h264_cfg h264;
	struct iav_h264_config updated;
	struct iav_stream *stream;

	if (copy_from_user(&h264, args, sizeof(h264)))
		return -EFAULT;

	mutex_lock(&iav->iav_mutex);

	stream = &iav->stream[h264.id];
	updated = stream->h264_config;

	updated.M = h264.M;
	updated.N = h264.N;
	updated.gop_structure = h264.gop_structure;
	updated.idr_interval = h264.idr_interval;
	updated.profile = h264.profile;
	updated.au_type = h264.au_type;
	updated.chroma_format = h264.chroma_format;
	updated.cpb_buf_idc = h264.cpb_buf_idc;
	updated.cpb_user_size = h264.cpb_user_size;
	updated.en_panic_rc = h264.en_panic_rc;
	updated.cpb_cmp_idc = h264.cpb_cmp_idc;
	updated.fast_rc_idc = h264.fast_rc_idc;
	updated.zmv_threshold = h264.mv_threshold;
	updated.flat_area_improve = h264.flat_area_improve;
	updated.fast_seek_intvl = h264.fast_seek_intvl;
	updated.multi_ref_p = h264.multi_ref_p;
	updated.intrabias_p = h264.intrabias_p;
	updated.intrabias_b = h264.intrabias_b;
	updated.user1_intra_bias = h264.user1_intra_bias;
	updated.user1_direct_bias = h264.user1_direct_bias;
	updated.user2_intra_bias = h264.user2_intra_bias;
	updated.user2_direct_bias = h264.user2_direct_bias;
	updated.user3_intra_bias = h264.user3_intra_bias;
	updated.user3_direct_bias = h264.user3_direct_bias;
	updated.deblocking_filter_alpha = h264.deblocking_filter_alpha;
	updated.deblocking_filter_beta = h264.deblocking_filter_beta;
	updated.deblocking_filter_enable = h264.deblocking_filter_enable;
	updated.frame_crop_left_offset = h264.frame_crop_left_offset;
	updated.frame_crop_right_offset = h264.frame_crop_right_offset;
	updated.frame_crop_top_offset = h264.frame_crop_top_offset;
	updated.frame_crop_bottom_offset = h264.frame_crop_bottom_offset;

	if (iav_check_h264_config(iav, h264.id, &updated) < 0) {
		mutex_unlock(&iav->iav_mutex);
		return -EINVAL;
	}

	stream->h264_config = updated;

	mutex_unlock(&iav->iav_mutex);

	return 0;
}

int iav_ioc_g_h264_cfg(struct ambarella_iav *iav, void __user *args)
{
	struct iav_h264_cfg h264;
	struct iav_stream *stream;

	if (copy_from_user(&h264, args, sizeof(h264)))
		return -EFAULT;

	if (h264.id >= IAV_STREAM_MAX_NUM_ALL) {
		iav_error("Invalid stream ID %d!\n", h264.id);
		return -EINVAL;
	}

	stream = &iav->stream[h264.id];

	mutex_lock(&iav->iav_mutex);

	h264.gop_structure = stream->h264_config.gop_structure;
	h264.M = stream->h264_config.M;
	h264.N = stream->h264_config.N;
	h264.idr_interval = stream->h264_config.idr_interval;
	h264.profile = stream->h264_config.profile;
	h264.au_type = stream->h264_config.au_type;
	h264.chroma_format = stream->h264_config.chroma_format;
	h264.cpb_buf_idc = stream->h264_config.cpb_buf_idc;
	h264.en_panic_rc = stream->h264_config.en_panic_rc;
	h264.cpb_cmp_idc = stream->h264_config.cpb_cmp_idc;
	h264.fast_rc_idc = stream->h264_config.fast_rc_idc;
	h264.cpb_user_size = stream->h264_config.cpb_user_size;
	h264.mv_threshold = stream->h264_config.zmv_threshold;
	h264.flat_area_improve = stream->h264_config.flat_area_improve;
	h264.fast_seek_intvl = stream->h264_config.fast_seek_intvl;
	h264.multi_ref_p = stream->h264_config.multi_ref_p;
	h264.intrabias_p = stream->h264_config.intrabias_p;
	h264.intrabias_b = stream->h264_config.intrabias_b;
	h264.user1_intra_bias = stream->h264_config.user1_intra_bias;
	h264.user1_direct_bias = stream->h264_config.user1_direct_bias;
	h264.user2_intra_bias = stream->h264_config.user2_intra_bias;
	h264.user2_direct_bias = stream->h264_config.user2_direct_bias;
	h264.user3_intra_bias = stream->h264_config.user3_intra_bias;
	h264.user3_direct_bias = stream->h264_config.user3_direct_bias;
	h264.deblocking_filter_alpha = stream->h264_config.deblocking_filter_alpha;
	h264.deblocking_filter_beta = stream->h264_config.deblocking_filter_beta;
	h264.deblocking_filter_enable = stream->h264_config.deblocking_filter_enable;
	h264.frame_crop_left_offset = stream->h264_config.frame_crop_left_offset;
	h264.frame_crop_right_offset = stream->h264_config.frame_crop_right_offset;
	h264.frame_crop_top_offset = stream->h264_config.frame_crop_top_offset;
	h264.frame_crop_bottom_offset = stream->h264_config.frame_crop_bottom_offset;

	get_pic_info_h264(iav, h264.id, &h264);

	mutex_unlock(&iav->iav_mutex);

	if (copy_to_user(args, &h264, sizeof(h264)))
		return -EFAULT;

	return 0;
}

static int iav_check_mjpeg_cfg(struct ambarella_iav *iav,
	struct iav_mjpeg_cfg *mjpeg)
{
	struct iav_stream *stream;

	iav_no_check();

	if (mjpeg->chroma_format != JPEG_CHROMA_YUV420
		&& mjpeg->chroma_format != JPEG_CHROMA_MONO) {
		iav_error("Invalid chroma format: %d\n", mjpeg->chroma_format);
		return -EINVAL;
	}

	if (mjpeg->quality == 0 || mjpeg->quality > 100){
		iav_error("Invalid mjpeg quality: %d\n", mjpeg->quality);
		return -EINVAL;
	}

	stream = &iav->stream[mjpeg->id];
	if (stream->format.type != IAV_STREAM_TYPE_MJPEG) {
		iav_error("Cannot change MJPEG config for H264 stream.\n");
		return -1;
	}

	return 0;
}

int iav_ioc_s_mjpeg_cfg(struct ambarella_iav *iav, void __user *args)
{
	struct iav_mjpeg_cfg mjpeg;
	struct iav_stream *stream;

	if (copy_from_user(&mjpeg, args, sizeof(mjpeg)))
		return -EFAULT;

	if (mjpeg.id >= IAV_STREAM_MAX_NUM_ALL) {
		iav_error("Invalid stream ID: %d!\n", mjpeg.id);
		return -EINVAL;
	}

	mutex_lock(&iav->iav_mutex);

	if (iav_check_mjpeg_cfg(iav, &mjpeg) < 0) {
		mutex_unlock(&iav->iav_mutex);
		return -EINVAL;
	}

	stream = &iav->stream[mjpeg.id];
	stream->mjpeg_config.chroma_format = mjpeg.chroma_format;
	stream->mjpeg_config.quality = mjpeg.quality;

	mutex_unlock(&iav->iav_mutex);

	return 0;
}

int iav_ioc_g_mjpeg_cfg(struct ambarella_iav *iav, void __user *args)
{
	struct iav_mjpeg_cfg mjpeg;
	struct iav_stream *stream;

	if (copy_from_user(&mjpeg, args, sizeof(mjpeg)))
		return -EFAULT;

	if (mjpeg.id >= IAV_STREAM_MAX_NUM_ALL) {
		iav_error("Invalid stream ID: %d!\n", mjpeg.id);
		return -EINVAL;
	}

	mutex_lock(&iav->iav_mutex);

	stream = &iav->stream[mjpeg.id];
	mjpeg.chroma_format = stream->mjpeg_config.chroma_format;
	mjpeg.quality = stream->mjpeg_config.quality;

	mutex_unlock(&iav->iav_mutex);

	if (copy_to_user(args, &mjpeg, sizeof(mjpeg)))
		return -EFAULT;

	return 0;
}

int iav_ioc_cfg_frame_sync(struct ambarella_iav * iav, void __user * arg)
{
	struct iav_stream_cfg cfg;
	int rval = 0;

	if (copy_from_user(&cfg, arg, sizeof(cfg)))
		return -EFAULT;

	if (cfg.id >= IAV_STREAM_MAX_NUM_ALL) {
		iav_error("Invalid stream ID: %d!\n", cfg.id);
		return -EINVAL;
	}

	mutex_lock(&iav->iav_mutex);

	switch (cfg.cid) {
	case IAV_H264_CFG_GOP:
		rval = iav_set_h264_gop(iav, cfg.id, &cfg.arg.h264_gop, 1);
		break;
	case IAV_H264_CFG_ZMV_THRESHOLD:
		rval = iav_set_h264_zmv_threshold(iav, cfg.id, cfg.arg.mv_threshold, 1);
		break;
	case IAV_H264_CFG_FORCE_IDR:
		rval = iav_set_h264_force_idr(iav, cfg.id, cfg.arg.h264_force_idr, 1);
		break;
	case IAV_H264_CFG_QP_ROI:
		rval = iav_set_h264_roi(iav, cfg.id, &cfg.arg.h264_roi, 1);
		break;
	case IAV_H264_CFG_BITRATE:
		rval = iav_set_h264_bitrate(iav,cfg.id, &cfg.arg.h264_rc, 1);
		break;
	case IAV_H264_CFG_FORCE_FAST_SEEK:
		rval = iav_set_h264_force_fast_seek(iav,cfg.id,
			cfg.arg.h264_force_fast_seek, 1);
		break;
	case IAV_H264_CFG_FRAME_DROP:
		rval = iav_set_h264_frame_drop(iav,cfg.id,
			cfg.arg.h264_drop_frames, 1);
		break;
	case IAV_H264_CFG_ENC_PARAM:
		rval = iav_set_h264_enc_param(iav,cfg.id,
			&cfg.arg.h264_enc, 1);
		break;
	case IAV_H264_CFG_LONG_REF_P:
		rval = iav_set_h264_long_ref_p(iav, cfg.id, 1);
		break;
	case IAV_H264_CFG_FORCE_PSKIP:
		rval = iav_set_h264_force_pskip(iav, cfg.id, &cfg.arg.h264_pskip, 1);
		break;
	default:
		iav_error("Unsupported frame sync cfg ID: %d!\n", cfg.cid);
		rval = -EINVAL;
		break;
	}

	mutex_unlock(&iav->iav_mutex);

	return rval;
}

int iav_ioc_get_frame_sync(struct ambarella_iav * iav, void __user * arg)
{
	struct iav_stream_cfg cfg;
	struct iav_stream *stream;
	struct iav_bitrate *h264_rc;
	struct iav_qpmatrix *h264_roi;
	struct iav_h264_enc_param *h264_enc;
	struct iav_h264_config *h264_cfg;
	int rval = 0;

	if (copy_from_user(&cfg, arg, sizeof(cfg)))
		return -EFAULT;

	if (cfg.id >= IAV_STREAM_MAX_NUM_ALL) {
		iav_error("Invalid stream ID: %d!\n", cfg.id);
		return -EINVAL;
	}

	stream = &iav->stream[cfg.id];

	mutex_lock(&iav->iav_mutex);

	switch (cfg.cid) {
	case IAV_H264_CFG_GOP:
		cfg.arg.h264_gop.id = cfg.id;
		cfg.arg.h264_gop.idr_interval = stream->h264_config.idr_interval;
		cfg.arg.h264_gop.N = stream->h264_config.N;
		break;
	case IAV_H264_CFG_ZMV_THRESHOLD:
		cfg.arg.mv_threshold = stream->h264_config.zmv_threshold;
		break;
	case IAV_H264_CFG_QP_ROI:
		h264_roi = &cfg.arg.h264_roi;
		rval = iav_get_h264_roi(iav, cfg.id, h264_roi, 1);
		break;
	case IAV_H264_CFG_BITRATE:
		h264_rc = &cfg.arg.h264_rc;
		h264_cfg = &stream->h264_config;
		h264_rc->id = cfg.id;
		h264_rc->vbr_setting = h264_cfg->vbr_setting;
		h264_rc->average_bitrate = h264_cfg->average_bitrate;
		h264_rc->qp_min_on_I = h264_cfg->qp_min_on_I;
		h264_rc->qp_max_on_I = h264_cfg->qp_max_on_I;
		h264_rc->qp_min_on_P = h264_cfg->qp_min_on_P;
		h264_rc->qp_max_on_P = h264_cfg->qp_max_on_P;
		h264_rc->qp_min_on_B = h264_cfg->qp_min_on_B;
		h264_rc->qp_max_on_B = h264_cfg->qp_max_on_B;
		h264_rc->qp_min_on_Q = h264_cfg->qp_min_on_Q;
		h264_rc->qp_max_on_Q = h264_cfg->qp_max_on_Q;
		h264_rc->i_qp_reduce = h264_cfg->i_qp_reduce;
		h264_rc->p_qp_reduce = h264_cfg->p_qp_reduce;
		h264_rc->q_qp_reduce = h264_cfg->q_qp_reduce;
		h264_rc->log_q_num_plus_1 = h264_cfg->log_q_num_plus_1;
		h264_rc->adapt_qp = h264_cfg->adapt_qp;
		h264_rc->skip_flag = h264_cfg->skip_flag;
		h264_rc->max_i_size_KB = h264_cfg->max_I_size_KB;
		break;
	case IAV_H264_CFG_ENC_PARAM:
		h264_enc = &cfg.arg.h264_enc;
		h264_cfg = &stream->h264_config;
		h264_enc->id = cfg.id;
		h264_enc->intrabias_p = h264_cfg->intrabias_p;
		h264_enc->intrabias_b = h264_cfg->intrabias_b;
		h264_enc->user1_intra_bias = h264_cfg->user1_intra_bias;
		h264_enc->user1_direct_bias = h264_cfg->user1_direct_bias;
		h264_enc->user2_intra_bias = h264_cfg->user2_intra_bias;
		h264_enc->user2_direct_bias = h264_cfg->user2_direct_bias;
		h264_enc->user3_intra_bias = h264_cfg->user3_intra_bias;
		h264_enc->user3_direct_bias = h264_cfg->user3_direct_bias;
		break;
	default:
		iav_error("Unsupported frame sync cfg ID: %d!\n", cfg.cid);
		rval = -EINVAL;
		break;
	}

	mutex_unlock(&iav->iav_mutex);

	if (rval < 0)
		return rval;

	if (copy_to_user(arg, &cfg, sizeof(cfg)))
		return -EFAULT;

	return 0;
}

static int iav_check_apply_frame_sync(struct ambarella_iav * iav,
	struct iav_apply_frame_sync *apply, u32* stream_map)
{
	struct iav_cmd_sync *cmd_sync;
	ipcam_real_time_encode_param_setup_t *cmd_data;
	u8 *addr = NULL;
	int i;

	cmd_sync = &iav->cmd_sync;
	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		addr = (u8 *)iav->mmap[IAV_BUFFER_CMD_SYNC].virt +
			cmd_sync->idx * CMD_SYNC_SIZE + i * DSP_ENC_CMD_SIZE;
		cmd_data = (ipcam_real_time_encode_param_setup_t *)addr;
		if (cmd_data->cmd_code) {
			// For each stream, remove frame sync commands with the same PTS
			if (cmd_sync->apply_pts[i] != apply->dsp_pts) {
				*stream_map |= (1 << i);
				cmd_sync->apply_pts[i] = apply->dsp_pts;
			} else {
				iav_debug("Remove frame sync apply flag for stream %c, "
					"PTS: %u!\n", 'A' + i, apply->dsp_pts);
			}
		}
	}

	if (*stream_map == 0)
		return -1;

	return 0;
}

int iav_ioc_apply_frame_sync(struct ambarella_iav * iav, void __user * arg)
{
	struct iav_apply_frame_sync apply;
	struct iav_cmd_sync *cmd_sync;
	u32 stream_map = 0;
	u8 *addr = NULL;

	if (copy_from_user(&apply, arg, sizeof(apply)))
		return -EFAULT;

	/* Check whether send apply cmd to dsp or not.
	 * If check failed, apply previous frame sync params to next apply cmd.
	 */
	if (iav_check_apply_frame_sync(iav, &apply, &stream_map) == 0) {
		cmd_sync = &iav->cmd_sync;
		cmd_apply_frame_sync_cmd(iav, NULL, &apply, stream_map);
		// update for next frame sync cmd
		cmd_sync->idx = (cmd_sync->idx + 1) % ENC_CMD_TOGGLED_NUM;
		addr = (u8 *)iav->mmap[IAV_BUFFER_CMD_SYNC].virt +
			cmd_sync->idx * CMD_SYNC_SIZE;
		memset(addr, 0, CMD_SYNC_SIZE);
		if (cmd_sync->qpm_flag) {
			cmd_sync->qpm_idx = (cmd_sync->qpm_idx % QP_MATRIX_TOGGLED_NUM) + 1;
		}
		cmd_sync->qpm_flag = 0;
	}

	return 0;
}

u32 iav_get_stream_map(struct ambarella_iav *iav)
{
	u32 i, stream_map = 0;
	struct iav_stream *stream;

	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; i++) {
		stream = &iav->stream[i];
		if (is_stream_in_encoding(stream) ||
			is_stream_in_starting(stream)) {
			stream_map |= 1 << i;
		}
	}

	return stream_map;
}

void iav_clear_stream_state(struct ambarella_iav *iav)
{
	u32 i;
	struct iav_stream *stream;

	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; i++) {
		stream = &iav->stream[i];
		stream->dsp_state = ENC_IDLE_STATE;
		stream->op = IAV_STREAM_OP_STOP;
		stream->fake_dsp_state = ENC_IDLE_STATE;
		stream->session_id = 0;
	}
}

void iav_init_streams(struct ambarella_iav *iav)
{
	u32 i, chip;
	struct iav_stream *stream;
	struct iav_h264_config *h264_config;
	struct iav_mjpeg_config *mjpeg_config;
	struct iav_stream_format *format;
	struct iav_cmd_sync *cmd_sync;
	struct iav_window max_enc_win[] = {
		{1920, 1080}, {1280, 720}, {720, 576}, {352, 288},
		{352, 240}, {352, 240}, {352, 240}, {352, 240},
	};
	s8 qp_delta[][4] = {{0,0,0,0},{0,0,0,0},{0,0,0,0}};

	iav->dsp->get_chip_id(iav->dsp, NULL, &chip);

	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; i++) {
		stream = &iav->stream[i];
		stream->iav = iav;
		stream->id_dsp = i;
		stream->session_id = 0;
		stream->fake_dsp_state = ENC_IDLE_STATE;
		stream->dsp_state = ENC_IDLE_STATE;
		stream->op = IAV_STREAM_OP_STOP;

		/* FIXME: Turn off 3/4th stream by default for S2LM to save dsp memory.
		 * User can turn on them manually.
		 */
		if ((chip < IAV_CHIP_ID_S2LM_LAST || chip == IAV_CHIP_ID_S2L_TEST) &&
			(i > 1)) {
			stream->max_GOP_M = 0;
			stream->max.width = 0;
			stream->max.height = 0;
		} else {
			stream->max_GOP_M = 1;
			stream->max.width = max_enc_win[i].width;
			stream->max.height = max_enc_win[i].height;
		}

		stream->max_GOP_N = 255;
		stream->long_ref_enable = 0;
		stream->fps.fps_multi = 1;
		stream->fps.fps_div = 1;
		init_waitqueue_head(&stream->venc_wq);


		if (i == 0) {
			stream->srcbuf = iav->main_buffer;
		} else {
			stream->srcbuf = &iav->srcbuf[IAV_SRCBUF_PC];
		}

		format = &stream->format;
		format->id = i;
		format->type = IAV_STREAM_TYPE_NONE;
		format->enc_win.x = 0;
		format->enc_win.y = 0;
		format->hflip = 0;
		format->vflip = 0;
		format->rotate_cw = 0;
		format->duration = 0;
		if (i == 0) {
			format->buf_id = IAV_SRCBUF_MN;
			format->enc_win.width = 1920;
			format->enc_win.height = 1080;
		} else {
			format->buf_id = IAV_SRCBUF_PC;
			format->enc_win.width = 720;
			format->enc_win.height = 480;
		}
		stream->prev_frmNo = -1;

		h264_config = &stream->h264_config;
		h264_config->profile = H264_PROFILE_MAIN;
		h264_config->M = 1;
		h264_config->N = 30;
		h264_config->gop_structure = IAV_GOP_SIMPLE;
		h264_config->idr_interval = 1;
		h264_config->vbr_setting = IAV_BRC_SCBR;
		if (i == 0)
			h264_config->average_bitrate = 8000000;
		else if (i == 1)
			h264_config->average_bitrate = 4000000;
		else
			h264_config->average_bitrate = 2000000;
		h264_config->qp_min_on_I = 14;
		h264_config->qp_max_on_I = 51;
		h264_config->qp_min_on_P = 17;
		h264_config->qp_max_on_P = 51;
		h264_config->qp_min_on_Q = 15;
		h264_config->qp_max_on_Q = 51;
		h264_config->qp_min_on_B = 21;
		h264_config->qp_max_on_B = 51;
		h264_config->adapt_qp = 2;
		h264_config->i_qp_reduce = 6;
		h264_config->p_qp_reduce = 3;
		h264_config->q_qp_reduce = 6;
		h264_config->log_q_num_plus_1 = 0;
		h264_config->skip_flag = 0;
		h264_config->max_I_size_KB = 0;
		h264_config->chroma_format = H264_CHROMA_YUV420;
		h264_config->au_type = 1;
		h264_config->qp_roi_enable = 0;
		h264_config->zmv_threshold = 0;
		h264_config->flat_area_improve = 0;
		h264_config->multi_ref_p = 0;
		h264_config->fast_seek_intvl = 0;
		h264_config->statis = 0;
		h264_config->intrabias_p = MIN_INTRABIAS;
		h264_config->intrabias_b = MIN_INTRABIAS;
		h264_config->user1_intra_bias = MIN_USER_BIAS;
		h264_config->user1_direct_bias = MIN_USER_BIAS;
		h264_config->user2_intra_bias = MIN_USER_BIAS;
		h264_config->user2_direct_bias = MIN_USER_BIAS;
		h264_config->user3_intra_bias = MIN_USER_BIAS;
		h264_config->user3_direct_bias = MIN_USER_BIAS;
		h264_config->deblocking_filter_enable = 1;
		h264_config->deblocking_filter_alpha = 0;
		h264_config->deblocking_filter_beta = 0;
		memcpy(h264_config->qp_delta[0], qp_delta, sizeof(qp_delta));

		mjpeg_config = &stream->mjpeg_config;
		mjpeg_config->chroma_format = JPEG_CHROMA_YUV420;
		mjpeg_config->quality = 50;
		mjpeg_config->jpeg_quant_matrix = (void *)iav->mmap[IAV_BUFFER_QUANT].virt +
			i * JPEG_QT_SIZE;

		mutex_init(&stream->osd_mutex);
	}

	// clear cmd sync area to 0
	cmd_sync = &iav->cmd_sync;
	memset((u8*)iav->mmap[IAV_BUFFER_CMD_SYNC].virt, 0,
		iav->mmap[IAV_BUFFER_CMD_SYNC].size);
	cmd_sync->idx = 0;
	// qp matrix idx 0 is reserved for app
	cmd_sync->qpm_idx = 1;
	cmd_sync->qpm_flag = 0;
	// clear qp matrix area to 0
	memset((u8*)iav->mmap[IAV_BUFFER_QPMATRIX].virt, 0,
		iav->mmap[IAV_BUFFER_QPMATRIX].size);
	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; i++) {
		cmd_sync->apply_pts[i] = INVALID_DSP_PTS_32;
	}
	// clear mv area to 0
	memset((u8*)iav->mmap[IAV_BUFFER_MV].virt, 0,
		iav->mmap[IAV_BUFFER_MV].size);

	iav->stream_num = 0;
	// init hw pts
	hw_pts_init(iav);
}


