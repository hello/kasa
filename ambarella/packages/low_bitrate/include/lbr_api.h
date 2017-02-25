/*******************************************************************************
 * lbr_api.h
 *
 * History:
 *   2014-02-17 - [Louis Sun] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
 ******************************************************************************/

#ifndef __LBR_API_H__
#define __LBR_API_H__

#ifdef __cplusplus
extern "C"
{
#endif
#include "lbr_common.h"

//LBR library does not have thread inside. it uses the caller 's thread

int lbr_open();
int lbr_init(lbr_init_t *rc_init);   //init lbr library with iav file handle and etc., must call init after open
int lbr_close();                     //close will also deinit it

/* All lbr APIs can be called on the fly
  If IAV state is not right, then the IAV call will fail. but no harm
*/

int lbr_set_style(LBR_STYLE style, int stream_id);           //let LBR to setup stream size and etc.
int lbr_get_style(LBR_STYLE *style, int stream_id);
int lbr_set_motion_level(LBR_MOTION_LEVEL motion_level, int stream_id);  //update lbr about motion level
int lbr_get_motion_level(LBR_MOTION_LEVEL *motion_level, int stream_id);
int lbr_set_noise_level(LBR_NOISE_LEVEL noise_level, int stream_id);    //update lbr about noise level
int lbr_get_noise_level(LBR_NOISE_LEVEL *noise_level, int stream_id);
int lbr_set_bitrate_ceiling(lbr_bitrate_target_t *bitrate_target, int stream_id);  //set max bitrate to get to
int lbr_get_bitrate_ceiling(lbr_bitrate_target_t *bitrate_target, int stream_id);
int lbr_get_log_level(int *pLog);
int lbr_set_log_level(int log);
u32 lbr_get_version();
int lbr_scale_profile_target_bitrate(LBR_PROFILE_TYPE profile, u32 numerator,
                                     u32 denominator);
#ifdef __cplusplus
}
#endif

#endif /* __LBR_API_H__ */

