/*
 * iav_enc_efm.c
 *
 * History:
 *	2015/07/01 - [Zhaoyang Chen] created file
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
#include <linux/module.h>
#include <linux/ambpriv_device.h>
#include <linux/mman.h>
#include <linux/hrtimer.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include <plat/ambcache.h>
#include <mach/hardware.h>
#include <iav_utils.h>
#include <iav_ioctl.h>
#include <dsp_api.h>
#include <dsp_format.h>
#include <vin_api.h>
#include <vout_api.h>
#include "iav.h"
#include "iav_dsp_cmd.h"
#include "iav_enc_api.h"
#include "iav_enc_utils.h"


static int wait_efm_msg(struct ambarella_iav *iav, IAV_EFM_STATE state)
{
	struct iav_efm_info *efm = &iav->efm;
	int rval = 0;

	rval = wait_event_interruptible_timeout(efm->wq,
		(efm->state == state), TWO_JIFFIES);
	if (rval <= 0) {
		iav_error("[TIMEOUT] Wait EFM msg, target state: %d, "
			"currrent state: %d!\n", state, efm->state);
		rval = -EAGAIN;
	}

	return rval;
}

static int check_efm(struct ambarella_iav *iav)
{
	if (!iav->system_config[iav->encode_mode].enc_from_mem) {
		iav_error("Please enable efm first!\n");
		return -1;
	}

	return 0;
}

void handle_efm_msg(struct ambarella_iav *iav, DSP_MSG *msg)
{
	struct iav_efm_info *efm;
	struct iav_efm_buf_pool *pool;
	efm_creat_frm_buf_pool_msg_t *create_msg;
	efm_req_frm_buf_msg_t *request_msg;
	efm_handshake_msg_t *handshake_msg;

	efm = &iav->efm;
	switch (msg->msg_code) {
	case DSP_STATUS_MSG_EFM_CREATE_FBP:
		create_msg = (efm_creat_frm_buf_pool_msg_t *)msg;
		if (create_msg->err_code) {
			iav_error("EFM: Failed to create EFM buffer pool!\n");
			return ;
		}
		if (create_msg->max_num_yuv != efm->req_buf_num) {
			iav_warn("EFM: YUV buf for EFM not enough; req: %d, ret: %d!\n",
				efm->req_buf_num, create_msg->max_num_yuv);
		}
		efm->yuv_buf_num = create_msg->max_num_yuv;
		efm->yuv_pitch = create_msg->yuv_pitch;
		if (create_msg->max_num_me1 != efm->req_buf_num) {
			iav_warn("EFM: YUV me1 num for EFM not enough; req: %d, ret: %d!\n",
				efm->req_buf_num, create_msg->max_num_me1);
		}
		efm->me1_buf_num = create_msg->max_num_me1;
		efm->me1_pitch = create_msg->me1_pitch;
		if (create_msg->max_num_yuv & create_msg->max_num_me1) {
			efm->state = EFM_STATE_CREATE_OK;
		}
		wake_up_interruptible(&efm->wq);
		break;
	case DSP_STATUS_MSG_EFM_REQ_FB:
		request_msg = (efm_req_frm_buf_msg_t *)msg;
		if (request_msg->err_code) {
			iav_error("EFM: Failed to request frame addr!\n");
			return ;
		}
		pool = &efm->buf_pool;
		if (pool->requested[pool->req_idx]) {
			iav_warn("EFM: Overwrite the previous requested frame addr!\n");
			pool->requested[pool->req_idx] = 0;
		}
		pool->yuv_fId[pool->req_idx] = request_msg->yuv_fId;
		pool->yuv_luma_addr[pool->req_idx] = request_msg->y_addr;
		pool->yuv_chroma_addr[pool->req_idx] = request_msg->uv_addr;
		pool->me1_fId[pool->req_idx] = request_msg->me1_fId;
		pool->me1_addr[pool->req_idx] = request_msg->me1_addr;
		pool->requested[pool->req_idx] = 1;
		pool->req_idx = (pool->req_idx + 1) % efm->req_buf_num;
		pool->req_msg_num++;
		if (pool->req_num > 1 && pool->req_msg_num == pool->req_num) {
			pool->req_num = 1;
			efm->state = EFM_STATE_WORKING_OK;
			wake_up_interruptible(&efm->wq);
		}
		break;
	case DSP_STATUS_MSG_EFM_HANDSHAKE:
		handshake_msg = (efm_handshake_msg_t *)msg;
		if (handshake_msg->err_code) {
			iav_error("EFM: Failed to handshake with DSP!\n");
			return ;
		}
		break;
	default:
		iav_error("Unrecognized EFM msg code: %d!\n", msg->msg_code);
		break;
	}

	return ;
}

int iav_ioc_efm_get_pool_info(struct ambarella_iav * iav, void __user * arg)
{
	struct iav_efm_get_pool_info info;

	if (copy_from_user(&info, arg, sizeof(info)))
		return -EFAULT;

	if (check_efm(iav) < 0) {
		return -EINVAL;
	}

	mutex_lock(&iav->iav_mutex);

	info.yuv_buf_num = iav->efm.yuv_buf_num;
	info.yuv_pitch = iav->efm.yuv_pitch;
	info.yuv_size.width = iav->efm.efm_size.width;
	info.yuv_size.height = iav->efm.efm_size.height;

	info.me1_buf_num = iav->efm.me1_buf_num;
	info.me1_pitch = iav->efm.me1_pitch;
	info.me1_size.width = iav->efm.efm_size.width >> 2;
	info.me1_size.height = iav->efm.efm_size.height >> 2;

	mutex_unlock(&iav->iav_mutex);

	return copy_to_user(arg, &info, sizeof(info)) ? -EFAULT : 0;
}

int iav_ioc_efm_request_frame(struct ambarella_iav * iav, void __user * arg)
{
	struct iav_efm_info *efm = &iav->efm;
	struct iav_efm_buf_pool *pool = &efm->buf_pool;
	struct iav_efm_request_frame request;
	u32 idx, i;

	if (copy_from_user(&request, arg, sizeof(request)))
		return -EFAULT;

	if(check_efm(iav) < 0) {
		return -EINVAL;
	}

	mutex_lock(&iav->iav_mutex);

	pool->req_msg_num = 0;
	if (pool->req_num == IAV_EFM_INIT_REQ_NUM) {
		// request frames from DSP for the first time
		for (i = 0; i < pool->req_num; i++) {
			cmd_efm_req_frame_buf(iav, NULL);
		}
		if (wait_efm_msg(iav, EFM_STATE_WORKING_OK) < 0) {
			mutex_unlock(&iav->iav_mutex);
			return -1;
		}
	} else {
		cmd_efm_req_frame_buf(iav, NULL);
	}
	idx = pool->hs_idx;
	if (!pool->requested[idx]) {
		iav_error("Failed to request frame, idx: %d!\n", idx);
		mutex_unlock(&iav->iav_mutex);
		return -EAGAIN;
	}
	if (!iav->dsp_partition_mapped) {
		request.yuv_luma_offset = DSP_TO_PHYS(pool->yuv_luma_addr[idx]) -
			iav->mmap[IAV_BUFFER_DSP].phys;
		request.yuv_chroma_offset = DSP_TO_PHYS(pool->yuv_chroma_addr[idx]) -
			iav->mmap[IAV_BUFFER_DSP].phys;
		request.me1_offset = DSP_TO_PHYS(pool->me1_addr[idx]) -
			iav->mmap[IAV_BUFFER_DSP].phys;
	} else {
		request.yuv_luma_offset = DSP_TO_PHYS(pool->yuv_luma_addr[idx]) -
			iav->mmap_dsp[IAV_DSP_SUB_BUF_EFM_YUV].phys;
		request.yuv_chroma_offset = DSP_TO_PHYS(pool->yuv_chroma_addr[idx]) -
			iav->mmap[IAV_DSP_SUB_BUF_EFM_YUV].phys;
		request.me1_offset = DSP_TO_PHYS(pool->me1_addr[idx]) -
			iav->mmap_dsp[IAV_DSP_SUB_BUF_EFM_ME1].phys;
	}
	request.frame_idx = idx;

	mutex_unlock(&iav->iav_mutex);

	return copy_to_user(arg, &request, sizeof(request)) ? -EFAULT : 0;
}

int iav_ioc_efm_handshake_frame(struct ambarella_iav * iav, void __user * arg)
{
	struct iav_efm_info *efm = &iav->efm;
	struct iav_efm_buf_pool *pool = &efm->buf_pool;
	struct iav_efm_handshake_frame handshake;
	int ret = 0;
	u64 hwpts = 0;

	if (copy_from_user(&handshake, arg, sizeof(handshake)))
		return -EFAULT;

	if(check_efm(iav) < 0) {
		return -EPERM;
	}

	if (handshake.frame_idx != pool->hs_idx) {
		iav_error("Invalid frame idx for handshake, req: %u, curr: %u!\n",
			pool->hs_idx, handshake.frame_idx);
		return -EINVAL;
	}

	mutex_lock(&iav->iav_mutex);

	if (handshake.use_hw_pts) {
		get_hwtimer_output_ticks(&hwpts);
	} else {
		hwpts = handshake.frame_pts;
	}
	efm->curr_pts = hwpts & 0xFFFFFFFF;

	ret = cmd_efm_handshake(iav, NULL, handshake.is_last_frame);

	pool->requested[pool->hs_idx] = 0;
	pool->hs_idx = (pool->hs_idx + 1) % efm->req_buf_num;
	efm->valid_num++;

	mutex_unlock(&iav->iav_mutex);

	return ret;
}

static int iav_reset_efm(struct ambarella_iav *iav)
{
	struct iav_efm_info *efm = &iav->efm;
	struct iav_efm_buf_pool *pool = &efm->buf_pool;
	int i;

	efm->yuv_buf_num = 0;
	efm->me1_buf_num = 0;
	efm->yuv_pitch = 0;
	efm->me1_pitch = 0;
	efm->curr_pts = 0;
	efm->valid_num = 0;
	init_waitqueue_head(&efm->wq);
	pool->req_num = IAV_EFM_INIT_REQ_NUM;
	pool->req_msg_num = 0;
	pool->req_idx = 0;
	pool->hs_idx = 0;
	for (i = 0; i < IAV_EFM_MAX_BUF_NUM; i++) {
		pool->requested[i] = 0;
	}

	efm->state = EFM_STATE_INIT_OK;

	return 0;
}

int iav_create_efm_pool(struct ambarella_iav *iav)
{
	if(check_efm(iav) < 0) {
		return -EPERM;
	}

	iav_reset_efm(iav);

	cmd_efm_creat_frm_buf_pool(iav, NULL);

	if (wait_efm_msg(iav, EFM_STATE_CREATE_OK) < 0) {
		return -1;
	}

	return 0;
}

