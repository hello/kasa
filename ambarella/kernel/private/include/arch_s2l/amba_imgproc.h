/*
 * kernel/private/include/arch_s2l/amba_imgproc.h
 *
 * History:
 *    2014/02/17 - [Jian Tang] Create
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


#ifndef __AMBA_IMGPROC_H__
#define __AMBA_IMGPROC_H__

struct iav_imgproc_info {
	u32	iav_state;
	u32	hdr_mode : 2;
	u32	vin_num : 3;
	u32	hdr_expo_num : 3;
	u32	reserved : 24;
	u32	cap_width;
	u32	cap_height;
	u32	img_size;
	unsigned long img_virt;
	unsigned long img_phys;
	unsigned long img_config_offset;
	unsigned long dsp_virt;
	unsigned long dsp_phys;
	struct mutex		*iav_mutex;
};

int amba_imgproc_msg(encode_msg_t *msg, u64 mono_pts);
int amba_imgproc_cmd(struct iav_imgproc_info *info, unsigned int cmd, unsigned long arg);

#ifdef CONFIG_PM
int amba_imgproc_suspend(int enc_mode);
int amba_imgproc_resume(int enc_mode, int fast_resume);
#endif

#endif	// __AMBA_IMGPROC_H__

