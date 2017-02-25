/*
 * iav_enc_pts.c
 *
 * History:
 *	2014/04/28 - [Zhaoyang Chen] created file
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


#include <config.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/fs.h>
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

#define AMBA_HW_TIMER_FILE		"/proc/ambarella/ambarella_hwtimer"
#define MAX_LEADING_ZERO_FOR_HW_PTS	8


#if !defined(BUILD_AMBARELLA_HW_TIMER)
int get_hwtimer_output_freq(u32 *out_freq)
{
	*out_freq = 0;
	return 0;
}
int get_hwtimer_output_ticks(u64 *out_tick)
{
	*out_tick = 0;
	return 0;
}
#endif

u64 get_hw_pts(struct ambarella_iav *iav, u32 audio_pts)
{
	int audio_diff;
	u64 mono_pts = 0;
	static u32 leading = 1;
	static u32 count = 0;
	static u32 prev_pts = 0;
	struct iav_pts_info *pts_info = &iav->pts_info;
	struct iav_hwtimer_pts_info *hw_pts_info = &pts_info->hw_pts_info;

	audio_diff = abs(hw_pts_info->audio_tick - audio_pts);
	if (unlikely(!hw_pts_info->audio_freq)) {
		iav_debug("HW PTS error: invalid audio frequency: %u!\n",
			hw_pts_info->audio_freq);
	} else if (unlikely(!audio_pts)) {
		// check leading zero pts case and continuous zero pts case
		if (leading && ++count > MAX_LEADING_ZERO_FOR_HW_PTS) {
			iav_debug("HW PTS error: too many leading zero PTS, Count: %d!\n",
				count);
		} else if (!leading && !prev_pts) {
			iav_debug("HW PTS error: continuous 2 zero PTS!\n");
		}
	} else if (unlikely(audio_diff > (hw_pts_info->audio_freq << 1))) {
		iav_debug("HW PTS error: too big gap [%u] between sync point [%u]"
			" and pts [%u]!\n", audio_diff, hw_pts_info->audio_tick,
			audio_pts);
	} else {
		// got valid pts from dsp
		leading = 0;
		mono_pts = (u64)audio_diff * hw_pts_info->hwtimer_freq +
			(hw_pts_info->audio_freq >> 1);
		do_div(mono_pts, hw_pts_info->audio_freq);
		mono_pts = hw_pts_info->hwtimer_tick - mono_pts;
	}
	prev_pts = audio_pts;

	return mono_pts;
}

int hw_pts_init(struct ambarella_iav *iav)
{
	struct iav_pts_info *pts_info = &iav->pts_info;
	struct iav_hwtimer_pts_info *hw_pts_info = &pts_info->hw_pts_info;

	hw_pts_info->fp_timer = filp_open(AMBA_HW_TIMER_FILE, O_RDONLY, 0);
	if(IS_ERR(hw_pts_info->fp_timer)) {
		pts_info->hwtimer_enabled = 0;
		hw_pts_info->fp_timer = NULL;
		iav_debug("HW timer for pts is NOT enabled!\n");
	} else {
		pts_info->hwtimer_enabled = 1;
		filp_close(hw_pts_info->fp_timer, 0);
		iav_debug("HW timer for pts is enabled.\n");
	}

	if (pts_info->hwtimer_enabled == 1) {
		hw_pts_info->audio_freq = (u32)clk_get_rate(clk_get(NULL, "gclk_audio"));
		get_hwtimer_output_freq(&hw_pts_info->hwtimer_freq);
		iav_debug("Audio clock freq: %u, HW timer clock freq: %u!",
			hw_pts_info->audio_freq, hw_pts_info->hwtimer_freq);
	}

	return 0;
}

int hw_pts_deinit(void)
{
	return 0;
}

