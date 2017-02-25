/*******************************************************************************
 * am_encode_warp_if.h
 *
 * History:
 *   Nov 6, 2015 - [zfgong] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents (“Software”) are protected by intellectual
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
#ifndef ORYX_VIDEO_INCLUDE_AM_ENCODE_WARP_IF_H_
#define ORYX_VIDEO_INCLUDE_AM_ENCODE_WARP_IF_H_

#include "am_video_types.h"
#include <string>

class AMIEncodeWarp
{
  public:
    virtual AM_RESULT set_ldc_mode(int region_id, AM_WARP_MODE mode)     = 0;
    virtual AM_RESULT get_ldc_mode(int region_id, AM_WARP_MODE &mode)    = 0;
    virtual AM_RESULT set_ldc_strength(int region_id, float strength)    = 0;
    virtual AM_RESULT get_ldc_strength(int region_id, float &strength)   = 0;
    virtual AM_RESULT set_max_radius(int region_id, int radius)          = 0;
    virtual AM_RESULT get_max_radius(int region_id, int &radius)         = 0;
    virtual AM_RESULT set_pano_hfov_degree(int region_id, float degree)  = 0;
    virtual AM_RESULT get_pano_hfov_degree(int region_id, float &degree) = 0;
    virtual AM_RESULT set_warp_region_yaw_pitch(int region_id,
                                                int yaw, int pitch)      = 0;
    virtual AM_RESULT get_warp_region_yaw_pitch(int region_id,
                                                int &yaw, int &pitch)    = 0;
    virtual AM_RESULT set_hor_ver_zoom(int region_id,
                                       AMFrac hor, AMFrac ver)           = 0;
    virtual AM_RESULT get_hor_ver_zoom(int region_id,
                                       AMFrac &hor, AMFrac &ver)         = 0;
    virtual AM_RESULT apply() = 0;
    virtual AM_RESULT load_config() = 0;
    virtual AM_RESULT save_config() = 0;

  protected:
    virtual ~AMIEncodeWarp(){}
};

#define VIDEO_PLUGIN_WARP    ("warp")
#define VIDEO_PLUGIN_WARP_SO ("video-warp.so")

#endif /* ORYX_VIDEO_INCLUDE_AM_ENCODE_WARP_IF_H_ */
