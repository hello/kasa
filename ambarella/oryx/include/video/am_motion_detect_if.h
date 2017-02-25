/*******************************************************************************
 * am_motion_detect_if.h
 *
 * History:
 *   May 4, 2016 - [binwang] created file
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
#ifndef ORYX_INCLUDE_AM_MOTION_DETECT_IF_H_
#define ORYX_INCLUDE_AM_MOTION_DETECT_IF_H_

#include "am_motion_detect_types.h"

class AMIMotionDetect
{
  public:
    virtual bool set_config(AMMDConfig *pConfig) = 0;
    virtual bool get_config(AMMDConfig *pConfig) = 0;
    virtual bool set_md_callback(void *this_obj,
                                 AMRecvMD recv_md,
                                 AMMDMessage *msg) = 0;
    virtual bool start(AM_STREAM_ID id = AM_STREAM_ID_MAX) = 0;
    virtual bool stop(AM_STREAM_ID id = AM_STREAM_ID_MAX) = 0;
    virtual AM_RESULT load_config() = 0;
    virtual AM_RESULT save_config() = 0;

  protected:
    virtual ~AMIMotionDetect(){}
};

#define VIDEO_PLUGIN_MD    ("motion-detect")
#define VIDEO_PLUGIN_MD_SO ("video-motion-detect.so")

#endif /* ORYX_INCLUDE_AM_MOTION_DETECT_IF_H_ */
