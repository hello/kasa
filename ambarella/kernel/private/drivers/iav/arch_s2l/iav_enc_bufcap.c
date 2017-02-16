/*
 * iav_enc_bufcap.c
 *
 * History:
 *	2014/03/28 - [Zhaoyang Chen] created file
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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

#define	DEBUG_BUFCAP	(0)


static inline int is_invalid_dsp_addr(u32 addr)
{
	return ((addr == 0x0) || (addr == 0xdeadbeef) || (addr == 0xc0000000));
}

static void save_raw_data_info(struct ambarella_iav *iav, encode_msg_t *msg)
{
	struct iav_bufcap_info *buf_cap;
	struct iav_raw_data_info *raw_cap;
	struct iav_rect *vin;

	buf_cap = &iav->buf_cap;
	vin = &iav->vinc[0]->vin_format.vin_win;

	if (unlikely(is_invalid_dsp_addr(msg->raw_pic_addr))) {
		return ;
	}

	raw_cap = &buf_cap->raw_info[buf_cap->write_index];
	raw_cap->raw_addr = msg->raw_pic_addr;
	raw_cap->raw_width = vin->width;
	raw_cap->raw_height = vin->height;
	raw_cap->pitch = msg->raw_pic_pitch;

	buf_cap->raw_data_valid = 1;
	wake_up_interruptible_all(&buf_cap->raw_wq);
}

static void save_srcbuf_data_info(struct ambarella_iav *iav, encode_msg_t *msg)
{
	struct iav_bufcap_info *buf_cap;
	struct iav_srcbuf_data_info *srcbuf_cap;
	u32 seq_num;
	u64 mono_pts;

	buf_cap = &iav->buf_cap;
	seq_num = buf_cap->seq_num;
	mono_pts = buf_cap->mono_pts;

	/* main source buffer data info */
	srcbuf_cap = &buf_cap->srcbuf_info[IAV_SRCBUF_MN][buf_cap->write_index];
	srcbuf_cap->y_data_addr = msg->main_y_addr;
	srcbuf_cap->uv_data_addr = msg->main_uv_addr;
	srcbuf_cap->pitch = msg->main_y_pitch;
	srcbuf_cap->seq_num = seq_num;
	srcbuf_cap->mono_pts = mono_pts;
	srcbuf_cap->dsp_pts = msg->sw_pts;

	/* preview C source buffer data info */
	srcbuf_cap = &buf_cap->srcbuf_info[IAV_SRCBUF_PC][buf_cap->write_index];
	srcbuf_cap->y_data_addr = msg->preview_C_y_addr;
	srcbuf_cap->uv_data_addr = msg->preview_C_uv_addr;
	srcbuf_cap->pitch = msg->preview_C_y_pitch;
	srcbuf_cap->seq_num = seq_num;
	srcbuf_cap->mono_pts = mono_pts;
	srcbuf_cap->dsp_pts = msg->sw_pts;

	/* preview B source buffer data info */
	srcbuf_cap = &buf_cap->srcbuf_info[IAV_SRCBUF_PB][buf_cap->write_index];
	srcbuf_cap->y_data_addr = msg->preview_B_y_addr;
	srcbuf_cap->uv_data_addr = msg->preview_B_uv_addr;
	srcbuf_cap->pitch = msg->preview_B_y_pitch;
	srcbuf_cap->seq_num = seq_num;
	srcbuf_cap->mono_pts = mono_pts;
	srcbuf_cap->dsp_pts = msg->sw_pts;

	/* preview A source buffer data info */
	srcbuf_cap = &buf_cap->srcbuf_info[IAV_SRCBUF_PA][buf_cap->write_index];
	srcbuf_cap->y_data_addr = msg->preview_A_y_addr;
	srcbuf_cap->uv_data_addr = msg->preview_A_uv_addr;
	srcbuf_cap->pitch = msg->preview_A_y_pitch;
	srcbuf_cap->seq_num = seq_num;
	srcbuf_cap->mono_pts = mono_pts;
	srcbuf_cap->dsp_pts = msg->sw_pts;

	buf_cap->srcbuf_data_valid = 1;
	wake_up_interruptible_all(&buf_cap->srcbuf_wq);
}

static void save_me1_data_info(struct ambarella_iav *iav, encode_msg_t *msg)
{
	struct iav_bufcap_info *buf_cap;
	struct iav_me_data_info *me1_cap;
	u32 seq_num;
	u64 mono_pts;

	buf_cap = &iav->buf_cap;
	seq_num = buf_cap->seq_num;
	mono_pts = buf_cap->mono_pts;

	/* main me1 data info */
	me1_cap = &buf_cap->me1_info[IAV_SRCBUF_MN][buf_cap->write_index];
	me1_cap->data_addr = msg->main_me1_addr;
	me1_cap->pitch = msg->main_me1_pitch;
	me1_cap->seq_num = seq_num;
	me1_cap->mono_pts = mono_pts;
	me1_cap->dsp_pts = msg->sw_pts;

	/* preview C me1 data info */
	me1_cap = &buf_cap->me1_info[IAV_SRCBUF_PC][buf_cap->write_index];
	me1_cap->data_addr = msg->preview_C_me1_addr;
	me1_cap->pitch = msg->preview_C_me1_pitch;
	me1_cap->seq_num = seq_num;
	me1_cap->mono_pts = mono_pts;
	me1_cap->dsp_pts = msg->sw_pts;

	/* preview B me1 data info */
	me1_cap = &buf_cap->me1_info[IAV_SRCBUF_PB][buf_cap->write_index];
	me1_cap->data_addr = msg->preview_B_me1_addr;
	me1_cap->pitch = msg->preview_B_me1_pitch;
	me1_cap->seq_num = seq_num;
	me1_cap->mono_pts = mono_pts;
	me1_cap->dsp_pts = msg->sw_pts;

	/* preview A me1 data info */
	me1_cap = &buf_cap->me1_info[IAV_SRCBUF_PA][buf_cap->write_index];
	me1_cap->data_addr = msg->preview_A_me1_addr;
	me1_cap->pitch = msg->preview_A_me1_pitch;
	me1_cap->seq_num = seq_num;
	me1_cap->mono_pts = mono_pts;
	me1_cap->dsp_pts = msg->sw_pts;

	buf_cap->me1_data_valid = 1;
	wake_up_interruptible_all(&buf_cap->me1_wq);
}

static void save_me0_data_info(struct ambarella_iav *iav, encode_msg_t *msg)
{
	struct iav_bufcap_info *buf_cap;
	struct iav_me_data_info *me0_cap;
	u32 seq_num;
	u64 mono_pts;

	buf_cap = &iav->buf_cap;
	seq_num = buf_cap->seq_num;
	mono_pts = buf_cap->mono_pts;

	/* main me0 data info */
	me0_cap = &buf_cap->me0_info[IAV_SRCBUF_MN][buf_cap->write_index];
	me0_cap->data_addr = msg->main_me0_addr;
	/* me0 shares the same pitch with me1 */
	me0_cap->pitch = msg->main_me1_pitch;
	me0_cap->seq_num = seq_num;
	me0_cap->mono_pts = mono_pts;
	me0_cap->dsp_pts = msg->sw_pts;

	buf_cap->me0_data_valid = 1;
	wake_up_interruptible_all(&buf_cap->me0_wq);
}

static u32 get_dsp_sub_buf_idx(u32 buf_id, u32 is_yuv)
{
	u32 sub_buf_idx = 0;

	switch (buf_id) {
	case IAV_SRCBUF_MN:
		if (is_yuv) {
			sub_buf_idx = IAV_DSP_SUB_BUF_MAIN_YUV;
		} else {
			sub_buf_idx = IAV_DSP_SUB_BUF_MAIN_ME1;
		}
		break;
	case IAV_SRCBUF_PC:
		if (is_yuv) {
			sub_buf_idx = IAV_DSP_SUB_BUF_PREV_C_YUV;
		} else {
			sub_buf_idx = IAV_DSP_SUB_BUF_PREV_C_ME1;
		}
		break;
	case IAV_SRCBUF_PB:
		if (is_yuv) {
			sub_buf_idx = IAV_DSP_SUB_BUF_PREV_B_YUV;
		} else {
			sub_buf_idx = IAV_DSP_SUB_BUF_PREV_B_ME1;
		}
		break;
	case IAV_SRCBUF_PA:
		if (is_yuv) {
			sub_buf_idx = IAV_DSP_SUB_BUF_PREV_A_YUV;
		} else {
			sub_buf_idx = IAV_DSP_SUB_BUF_PREV_A_ME1;
		}
		break;
	default:
		iav_error("Incorrect buffer id %d for DSP sub buffer!\n", buf_id);
		break;
	}

	return sub_buf_idx;
}


static int read_yuv_info(struct ambarella_iav *iav, u32 buf_id,
	struct iav_yuv_cap *yuv_out)
{
	struct iav_bufcap_info *buf_cap;
	struct iav_srcbuf_data_info *data_info;
	u32 width, height;
	u32 format, rd_idx, sub_buf_idx;

	buf_cap = &iav->buf_cap;
	rd_idx = buf_cap->read_index;
	data_info = &buf_cap->srcbuf_info[buf_id][rd_idx];

	width = iav->srcbuf[buf_id].win.width;
	height = iav->srcbuf[buf_id].win.height;
	format = IAV_YUV_FORMAT_YUV420;

	switch (buf_id) {
	case IAV_SRCBUF_PC:
		if (data_info->pitch > ALIGN(PCBUF_LIMIT_MAX_WIDTH, 32)) {
			data_info->pitch = 0;
		}
		break;
	case IAV_SRCBUF_PB:
		if (iav->srcbuf[IAV_SRCBUF_PB].type == IAV_SRCBUF_TYPE_PREVIEW)  {
			width = G_voutinfo[1].active_mode.video_size.video_width;
			height = G_voutinfo[1].active_mode.video_size.video_height;
			format = IAV_YUV_FORMAT_YUV422;
		}
		if (data_info->pitch > ALIGN(PBBUF_LIMIT_MAX_WIDTH, 32)) {
			data_info->pitch = 0;
		}
		break;
	case IAV_SRCBUF_PA:
		if (iav->srcbuf[IAV_SRCBUF_PA].type == IAV_SRCBUF_TYPE_PREVIEW)  {
			width = G_voutinfo[0].active_mode.video_size.video_width;
			height = G_voutinfo[0].active_mode.video_size.video_height;
			format = IAV_YUV_FORMAT_YUV422;
		}
		if (data_info->pitch > ALIGN(PABUF_LIMIT_MAX_WIDTH, 32)) {
			data_info->pitch = 0;
		}
		break;
	default:
		break;
	}

	if (is_invalid_dsp_addr(data_info->y_data_addr) ||
			is_invalid_dsp_addr(data_info->uv_data_addr)) {
		yuv_out->y_addr_offset = 0;
		yuv_out->uv_addr_offset = 0;
	} else {
		if (!iav->dsp_partition_mapped) {
			yuv_out->y_addr_offset = DSP_TO_PHYS(data_info->y_data_addr) -
				iav->mmap[IAV_BUFFER_DSP].phys;
			yuv_out->uv_addr_offset = DSP_TO_PHYS(data_info->uv_data_addr) -
				iav->mmap[IAV_BUFFER_DSP].phys;
		} else {
			sub_buf_idx = get_dsp_sub_buf_idx(buf_id, 1);
			yuv_out->y_addr_offset = DSP_TO_PHYS(data_info->y_data_addr) -
				iav->mmap_dsp[sub_buf_idx].phys;
			yuv_out->uv_addr_offset = DSP_TO_PHYS(data_info->uv_data_addr) -
				iav->mmap_dsp[sub_buf_idx].phys;
		}
	}
	yuv_out->width = width;
	yuv_out->height = height;
	yuv_out->pitch = data_info->pitch;
	yuv_out->format = format;
	yuv_out->mono_pts = data_info->mono_pts;
	yuv_out->dsp_pts = data_info->dsp_pts;
	yuv_out->seq_num = data_info->seq_num;

#if DEBUG_BUFCAP
	iav_debug("YUV: size %ux%u, pitch %u, y_addr: 0x%08x, uv_addr: 0x%08x,"
		" format: %u, mono_pts: %llu, seq_num: %u.\n", width, height,
		data_info->pitch, data_info->y_data_addr, data_info->uv_data_addr,
		format, data_info->mono_pts, data_info->seq_num);
#endif

	return 0;
}

static int read_me1_info(struct ambarella_iav *iav, u32 buf_id,
	struct iav_me_cap *me1_out)
{
	struct iav_bufcap_info *buf_cap;
	struct iav_me_data_info *data_info;
	u32 rd_idx, sub_buf_idx;

	buf_cap = &iav->buf_cap;
	rd_idx = buf_cap->read_index;
	data_info = &buf_cap->me1_info[buf_id][rd_idx];

	if (is_invalid_dsp_addr(data_info->data_addr)) {
		me1_out->data_addr_offset = 0;
	} else {
		if (!iav->dsp_partition_mapped) {
			me1_out->data_addr_offset = DSP_TO_PHYS(data_info->data_addr) -
				iav->mmap[IAV_BUFFER_DSP].phys;
		} else {
			sub_buf_idx = get_dsp_sub_buf_idx(buf_id, 0);
			me1_out->data_addr_offset = DSP_TO_PHYS(data_info->data_addr) -
				iav->mmap_dsp[sub_buf_idx].phys;
		}
	}
	me1_out->width = iav->srcbuf[buf_id].win.width >> 2;
	me1_out->height = iav->srcbuf[buf_id].win.height >> 2;
	me1_out->pitch = data_info->pitch;
	me1_out->mono_pts = data_info->mono_pts;
	me1_out->dsp_pts = data_info->dsp_pts;
	me1_out->seq_num = data_info->seq_num;

#if DEBUG_BUFCAP
	iav_debug("ME1: size %ux%u, pitch %u, addr: 0x%08x, mono_pts: %llu,"
		" seq_num: %u.\n", me1_out->width, me1_out->height, data_info->pitch,
		data_info->data_addr, data_info->mono_pts, data_info->seq_num);
#endif

	return 0;
}

static int read_me0_info(struct ambarella_iav *iav, u32 buf_id,
	struct iav_me_cap *me0_out)
{
	struct iav_bufcap_info *buf_cap;
	struct iav_me_data_info *data_info;
	struct iav_system_config *sys_config;
	u32 rd_idx;

	buf_cap = &iav->buf_cap;
	rd_idx = buf_cap->read_index;
	data_info = &buf_cap->me0_info[buf_id][rd_idx];

	if (is_invalid_dsp_addr(data_info->data_addr)) {
		me0_out->data_addr_offset = 0;
	} else {
		if (!iav->dsp_partition_mapped) {
			me0_out->data_addr_offset = DSP_TO_PHYS(data_info->data_addr) -
				iav->mmap[IAV_BUFFER_DSP].phys;
		} else {
			// TODO: Support me0 for dsp partition case
			me0_out->data_addr_offset = 0;
		}
	}
	sys_config = &iav->system_config[iav->encode_mode];
	if (sys_config->me0_scale == ME0_SCALE_8X) {
		me0_out->width = ALIGN(iav->srcbuf[buf_id].win.width, 16) >> 3;
		me0_out->height = ALIGN(iav->srcbuf[buf_id].win.height, 16) >> 3;
	} else if (sys_config->me0_scale == ME0_SCALE_16X) {
		me0_out->width = ALIGN(iav->srcbuf[buf_id].win.width, 16) >> 4;
		me0_out->height = ALIGN(iav->srcbuf[buf_id].win.height, 16) >> 4;
	}
	me0_out->pitch = data_info->pitch;
	me0_out->mono_pts = data_info->mono_pts;
	me0_out->dsp_pts = data_info->dsp_pts;
	me0_out->seq_num = data_info->seq_num;

#if DEBUG_BUFCAP
	iav_debug("ME0: size %ux%u, pitch %u, addr: 0x%08x, mono_pts: %llu,"
		" seq_num: %u.\n", me0_out->width, me0_out->height, data_info->pitch,
		data_info->data_addr, data_info->mono_pts, data_info->seq_num);
#endif

	return 0;
}


void irq_update_bufcap(struct ambarella_iav *iav, encode_msg_t *msg)
{
	struct iav_bufcap_info *buf_cap = &iav->buf_cap;

	spin_lock(&iav->iav_lock);

	/* TODO: Refine hw pts for warp mode */
	if (iav->pts_info.hwtimer_enabled &&
		(!iav->system_config[iav->encode_mode].enc_raw_rgb) &&
		(!iav->system_config[iav->encode_mode].enc_raw_yuv)) {
		buf_cap->mono_pts = get_hw_pts(iav, msg->hw_pts);
		if (iav->encode_mode == DSP_MULTI_REGION_WARP_MODE) {
			buf_cap->sec2_out_mono_pts = get_hw_pts(iav, msg->hw_pts);
		}
	} else {
		buf_cap->mono_pts = get_monotonic_pts();
		if (iav->encode_mode == DSP_MULTI_REGION_WARP_MODE) {
			buf_cap->sec2_out_mono_pts = get_monotonic_pts();
		}
	}

	if (iav->system_config[iav->encode_mode].raw_capture) {
		save_raw_data_info(iav, msg);
	}

	save_srcbuf_data_info(iav, msg);
	save_me1_data_info(iav, msg);
	save_me0_data_info(iav, msg);

	++buf_cap->seq_num;

	/* always read out the latest frame */
	buf_cap->read_index = buf_cap->write_index;

	if (++buf_cap->write_index == MAX_BUFCAP_DATA_NUM)
		buf_cap->write_index = 0;

	spin_unlock(&iav->iav_lock);
}

void iav_init_bufcap(struct ambarella_iav *iav)
{
	struct iav_bufcap_info *buf_cap = &iav->buf_cap;

	memset(buf_cap, 0, sizeof(struct iav_bufcap_info));

	buf_cap->raw_data_valid = 0;
	init_waitqueue_head(&buf_cap->raw_wq);

	buf_cap->srcbuf_data_valid = 0;
	init_waitqueue_head(&buf_cap->srcbuf_wq);

	buf_cap->me1_data_valid = 0;
	init_waitqueue_head(&buf_cap->me1_wq);

	buf_cap->me0_data_valid = 0;
	init_waitqueue_head(&buf_cap->me0_wq);
}

int iav_query_rawdesc(struct ambarella_iav *iav,
	struct iav_rawbufdesc *rawdesc)
{
	struct iav_bufcap_info *buf_cap = &iav->buf_cap;
	u32 raw_addr;
	int rd_idx, rval = 0;

	if (iav->state == IAV_STATE_IDLE) {
		iav_error("Invalid IAV state: %d!\n", iav->state);
		return -EBADFD;
	}

	if (!iav->system_config[iav->encode_mode].raw_capture) {
		iav_error("Please enable raw capture before querying raw data!\n");
		return -EPERM;
	}

	if (!(rawdesc->flag & IAV_BUFCAP_NONBLOCK)) {
		rval = wait_event_interruptible_timeout(buf_cap->raw_wq,
			(buf_cap->raw_data_valid == 1), TWO_JIFFIES);
		if (rval <= 0) {
			iav_error("[TIMEOUT] Query RAW descriptor.\n");
			return -EAGAIN;
		} else {
			rval = 0;
		}
	}

	mutex_lock(&iav->iav_mutex);
	do {
		rd_idx = buf_cap->read_index;
		raw_addr = buf_cap->raw_info[rd_idx].raw_addr;
		if (unlikely(is_invalid_dsp_addr(raw_addr))) {
			rval = -EIO;
			break;
		}

		rawdesc->raw_addr_offset = DSP_TO_PHYS(raw_addr) -
			iav->mmap[IAV_BUFFER_DSP].phys;
		rawdesc->width = buf_cap->raw_info[rd_idx].raw_width;
		rawdesc->height = buf_cap->raw_info[rd_idx].raw_height;
		rawdesc->pitch = buf_cap->raw_info[rd_idx].pitch;

		/* clear data valid flag */
		spin_lock_irq(&iav->iav_lock);
		buf_cap->raw_data_valid = 0;
		spin_unlock_irq(&iav->iav_lock);
	} while (0);
	mutex_unlock(&iav->iav_mutex);

	return rval;
}

int iav_query_yuvdesc(struct ambarella_iav *iav,
	struct iav_yuvbufdesc *yuvdesc)
{
	struct iav_bufcap_info *buf_cap;
	struct iav_yuv_cap yuv_cap;
	int rval = 0;

	if (yuvdesc->buf_id >= IAV_SRCBUF_LAST) {
		iav_error("Invalid buffer ID: %d\n", yuvdesc->buf_id);
		return -EINVAL;
	}

	if (iav->state == IAV_STATE_IDLE) {
		iav_error("Invalid IAV state: %d!\n", iav->state);
		return -EBADFD;
	}

	buf_cap = &iav->buf_cap;

	if (!(yuvdesc->flag & IAV_BUFCAP_NONBLOCK)) {
		rval = wait_event_interruptible_timeout(buf_cap->srcbuf_wq,
			(buf_cap->srcbuf_data_valid == 1), TWO_JIFFIES);
		if (rval <= 0) {
			iav_error("[TIMEOUT] Query YUV descriptor.\n");
			return -EAGAIN;
		} else {
			rval = 0;
		}
	}

	mutex_lock(&iav->iav_mutex);
	do {
		rval = read_yuv_info(iav, yuvdesc->buf_id, &yuv_cap);
		if (unlikely(rval < 0)) {
			rval = -EIO;
			break;
		}

		yuvdesc->y_addr_offset = yuv_cap.y_addr_offset;
		yuvdesc->uv_addr_offset = yuv_cap.uv_addr_offset;;
		yuvdesc->width = yuv_cap.width;
		yuvdesc->height = yuv_cap.height;
		yuvdesc->pitch = yuv_cap.pitch;
		yuvdesc->seq_num = yuv_cap.seq_num;
		yuvdesc->format = yuv_cap.format;
		yuvdesc->mono_pts = yuv_cap.mono_pts;
		yuvdesc->dsp_pts = yuv_cap.dsp_pts;

		/* clear data valid flag */
		spin_lock_irq(&iav->iav_lock);
		buf_cap->srcbuf_data_valid = 0;
		spin_unlock_irq(&iav->iav_lock);
	} while (0);
	mutex_unlock(&iav->iav_mutex);

	return rval;
}

int iav_query_me1desc(struct ambarella_iav *iav,
	struct iav_mebufdesc *me1desc)
{
	struct iav_bufcap_info *buf_cap;
	struct iav_me_cap me1_cap;
	int rval = 0;

	if (me1desc->buf_id >= IAV_SRCBUF_LAST) {
		iav_error("Invalid buffer ID: %d\n", me1desc->buf_id);
		return -EINVAL;
	}

	if (iav->state == IAV_STATE_IDLE) {
		iav_error("Invalid IAV state: %d!\n", iav->state);
		return -EBADFD;
	}

	buf_cap = &iav->buf_cap;

	if (!(me1desc->flag & IAV_BUFCAP_NONBLOCK)) {
		rval = wait_event_interruptible_timeout(buf_cap->me1_wq,
			(buf_cap->me1_data_valid == 1), TWO_JIFFIES);
		if (rval <= 0) {
			iav_error("[TIMEOUT] Query ME1 descriptor.\n");
			return -EAGAIN;
		} else {
			rval = 0;
		}
	}

	mutex_lock(&iav->iav_mutex);
	do {
		rval = read_me1_info(iav, me1desc->buf_id, &me1_cap);
		if (unlikely(rval < 0)) {
			rval = -EIO;
			break;
		}

		me1desc->data_addr_offset = me1_cap.data_addr_offset;
		me1desc->width = me1_cap.width;
		me1desc->height = me1_cap.height;
		me1desc->pitch = me1_cap.pitch;
		me1desc->seq_num = me1_cap.seq_num;
		me1desc->mono_pts = me1_cap.mono_pts;
		me1desc->dsp_pts = me1_cap.dsp_pts;

		/* clear data valid flag */
		spin_lock_irq(&iav->iav_lock);
		buf_cap->me1_data_valid = 0;
		spin_unlock_irq(&iav->iav_lock);
	} while (0);
	mutex_unlock(&iav->iav_mutex);

	return rval;
}

int iav_query_me0desc(struct ambarella_iav *iav,
	struct iav_mebufdesc *me0desc)
{
	struct iav_bufcap_info *buf_cap;
	struct iav_me_cap me0_cap;
	int rval = 0;

	if (me0desc->buf_id > IAV_SRCBUF_MN) {
		iav_error("Invalid buffer ID: %d\n", me0desc->buf_id);
		return -EINVAL;
	}

	if (iav->state == IAV_STATE_IDLE) {
		iav_error("Invalid IAV state: %d!\n", iav->state);
		return -EBADFD;
	}

	buf_cap = &iav->buf_cap;

	if (!(me0desc->flag & IAV_BUFCAP_NONBLOCK)) {
		rval = wait_event_interruptible_timeout(buf_cap->me0_wq,
			(buf_cap->me0_data_valid == 1), TWO_JIFFIES);
		if (rval <= 0) {
			iav_error("[TIMEOUT] Query ME0 descriptor.\n");
			return -EAGAIN;
		} else {
			rval = 0;
		}
	}

	mutex_lock(&iav->iav_mutex);
	do {
		rval = read_me0_info(iav, me0desc->buf_id, &me0_cap);
		if (unlikely(rval < 0)) {
			rval = -EIO;
			break;
		}

		me0desc->data_addr_offset = me0_cap.data_addr_offset;
		me0desc->width = me0_cap.width;
		me0desc->height = me0_cap.height;
		me0desc->pitch = me0_cap.pitch;
		me0desc->seq_num = me0_cap.seq_num;
		me0desc->mono_pts = me0_cap.mono_pts;
		me0desc->dsp_pts = me0_cap.dsp_pts;

		/* clear data valid flag */
		spin_lock_irq(&iav->iav_lock);
		buf_cap->me0_data_valid = 0;
		spin_unlock_irq(&iav->iav_lock);
	} while (0);
	mutex_unlock(&iav->iav_mutex);

	return rval;
}

int iav_query_bufcapdesc(struct ambarella_iav *iav,
	struct iav_bufcapdesc *bufdesc)
{
	struct iav_bufcap_info *buf_cap;
	int rval = 0;
	int i;

	if (iav->state == IAV_STATE_IDLE) {
		iav_error("Invalid IAV state: %d!\n", iav->state);
		return -EBADFD;
	}

	buf_cap = &iav->buf_cap;

	if (!(bufdesc->flag & IAV_BUFCAP_NONBLOCK)) {
		rval = wait_event_interruptible_timeout(buf_cap->srcbuf_wq,
			(buf_cap->srcbuf_data_valid == 1), TWO_JIFFIES);
		if (rval <= 0) {
			iav_error("[TIMEOUT] Query buffer capture descriptor.\n");
			return -EAGAIN;
		} else {
			rval = 0;
		}
	}

	mutex_lock(&iav->iav_mutex);

	for(i = IAV_SRCBUF_FIRST; i < IAV_SRCBUF_LAST; ++i) {
		if (read_yuv_info(iav, i, &bufdesc->yuv[i])) {
			rval = -EIO;
			goto bufcapdesc_exit;
		}
		if (read_me1_info(iav, i, &bufdesc->me1[i])) {
			rval = -EIO;
			goto bufcapdesc_exit;
		}
		if (read_me0_info(iav, i, &bufdesc->me0[i])) {
			rval = -EIO;
			goto bufcapdesc_exit;
		}
	}

	/* clear data valid flag */
	spin_lock_irq(&iav->iav_lock);
	buf_cap->srcbuf_data_valid = 0;
	buf_cap->me1_data_valid = 0;
	buf_cap->me0_data_valid = 0;
	spin_unlock_irq(&iav->iav_lock);

bufcapdesc_exit:
	mutex_unlock(&iav->iav_mutex);

	return rval;
}

