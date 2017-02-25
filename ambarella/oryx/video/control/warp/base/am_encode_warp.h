/*******************************************************************************
 * am_warp.h
 *
 * History:
 *   Oct 15, 2015 - [zfgong] created file
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

#ifndef ORYX_VIDEO_INCLUDE_AM_WARP_H_
#define ORYX_VIDEO_INCLUDE_AM_WARP_H_

#include "am_encode_plugin_if.h"
#include "am_encode_warp_if.h"
#include "am_platform_if.h"
#include "am_video_camera_if.h"

#include "am_encode_config.h"

#define DISTORTION_LOOKUP_TABLE_ENTRY_NUM  256

class AMVin;
class AMEncodeStream;
class AMEncodeWarp: public AMIEncodePlugin, public AMIEncodeWarp
{
  public:
    virtual void destroy();
    virtual bool start(AM_STREAM_ID id = AM_STREAM_ID_MAX);
    virtual bool stop(AM_STREAM_ID id = AM_STREAM_ID_MAX);
    virtual std::string& name();

  public:
    virtual AM_RESULT set_ldc_mode(int region_id, AM_WARP_MODE mode);
    virtual AM_RESULT get_ldc_mode(int region_id, AM_WARP_MODE &mode);
    virtual AM_RESULT set_ldc_strength(int region_id, float strength);
    virtual AM_RESULT get_ldc_strength(int region_id, float &strength);
    virtual AM_RESULT set_max_radius(int region_id, int radius);
    virtual AM_RESULT get_max_radius(int region_id, int &radius);
    virtual AM_RESULT set_pano_hfov_degree(int region_id, float degree);
    virtual AM_RESULT get_pano_hfov_degree(int region_id, float &degree);
    virtual AM_RESULT set_warp_region_yaw_pitch(int region_id,
                                                int yaw, int pitch);
    virtual AM_RESULT get_warp_region_yaw_pitch(int region_id,
                                                int &yaw, int &pitch);
    virtual AM_RESULT set_hor_ver_zoom(int region_id, AMFrac hor, AMFrac ver);
    virtual AM_RESULT get_hor_ver_zoom(int region_id, AMFrac &hor, AMFrac &ver);
    virtual AM_RESULT apply();
    //for config files
    virtual AM_RESULT load_config();
    virtual AM_RESULT save_config();

    virtual bool is_ldc_enable();

  protected:
    AMEncodeWarp(const char *name);
    virtual ~AMEncodeWarp();
    AM_RESULT init(AMVin *vin, AMEncodeStream *stream);

  protected:
    double                m_pixel_width_um;
    AMVin                *m_vin;
    AMEncodeStream       *m_stream;
    int                   m_iav;
    int                   m_distortion_lut[DISTORTION_LOOKUP_TABLE_ENTRY_NUM];
    bool                  m_ldc_enable;
    bool                  m_ldc_checked;
    bool                  m_is_running;
    std::string           m_name;
    AMIPlatformPtr        m_platform;
    AMMemMapInfo          m_mem;
};



#endif /* ORYX_VIDEO_INCLUDE_AM_WARP_H_ */
