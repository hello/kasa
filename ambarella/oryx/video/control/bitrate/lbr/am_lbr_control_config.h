/*******************************************************************************
 * am_lbr_control_config.h
 *
 * History:
 *   Nov 10, 2015 - [ypchang] created file
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
#ifndef AM_LBR_CONTROL_CONFIG_H_
#define AM_LBR_CONTROL_CONFIG_H_

#include "am_encode_types.h"
#include <map>

enum AM_LBR_PROFILE
{
  AM_LBR_PROFILE_STATIC                = 0,
  AM_LBR_PROFILE_MOTION_SMALL          = 1,
  AM_LBR_PROFILE_MOTION_BIG            = 2,
  AM_LBR_PROFILE_LOW_LIGHT             = 3,
  AM_LBR_PROFILE_BIG_MOTION_FRAME_DROP = 4,
  AM_LBR_PROFILE_NUM,
  AM_LBR_PROFILE_FIRST = AM_LBR_PROFILE_STATIC,
  AM_LBR_PROFILE_LAST  = AM_LBR_PROFILE_BIG_MOTION_FRAME_DROP,
};

struct AMLbrStream
{
    uint32_t bitrate_ceiling;
    bool enable_lbr;
    bool motion_control;
    bool noise_control;
    bool frame_drop;
    bool auto_target;
};

typedef std::map<AM_STREAM_ID, AMLbrStream> AMLbrStreamMap;

struct AMLbrConfig
{
    uint32_t log_level;
    uint32_t noise_low_threshold;
    uint32_t noise_high_threshold;
    bool config_changed;
    AMScaleFactor profile_bt_sf[AM_LBR_PROFILE_NUM];
    AMLbrStreamMap stream_params;
    AMLbrConfig() :
      log_level(0),
      noise_low_threshold(0),
      noise_high_threshold(0),
      config_changed(false)
    {
      stream_params.clear();
    }
};

class AMConfig;
class AMLbrControl;
class AMLbrControlConfig
{
    friend class AMLbrControl;
  public:
    AMLbrControlConfig();
    virtual ~AMLbrControlConfig();
    AMLbrConfig* get_config(const std::string& conf);
    bool save_config(const std::string& conf);
    bool sync_config(AMLbrConfig *cfg);

  private:
    AMLbrConfig *m_lbr_config;
    AMConfig    *m_config;
};

#ifdef BUILD_AMBARELLA_ORYX_CONF_DIR
#define LBR_CONF_DIR ((const char*)BUILD_AMBARELLA_ORYX_CONF_DIR "/video")
#else
#define LBR_CONF_DIR ((const char*)"/etc/oryx/video")
#endif

#endif /* AM_LBR_CONTROL_CONFIG_H_ */
