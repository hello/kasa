/*******************************************************************************
 * am_smartrc_config.h
 *
 * History:
 *   Jul 4, 2016 - [binwang] created file
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
#ifndef ORYX_VIDEO_CONTROL_BITRATE_SMARTRC_AM_SMARTRC_CONFIG_H_
#define ORYX_VIDEO_CONTROL_BITRATE_SMARTRC_AM_SMARTRC_CONFIG_H_

#include <mutex>
#include <atomic>
#include "am_video_types.h"

struct AMSmartRCStream
{
    uint32_t bitrate_ceiling;
    bool enable;
    bool dynamic_GOP_N;
    bool MB_level_control;
    bool frame_drop;
};

typedef std::map<AM_STREAM_ID, AMSmartRCStream> AMSmartRCStreamMap;

struct AMSmartRCParam
{
    uint32_t log_level;
    uint32_t noise_low_threshold;
    uint32_t noise_high_threshold;
    bool config_changed;
    AMSmartRCStreamMap stream_params;
    AMSmartRCParam() :
      log_level(0),
      noise_low_threshold(0),
      noise_high_threshold(0),
      config_changed(false)
      {
       stream_params.clear();
      }
};

class AMConfig;
class AMSmartRCConfig
{
  public:
    static AMSmartRCConfig *get_instance();
    AM_RESULT get_config(AMSmartRCParam &config);
    AM_RESULT set_config(const AMSmartRCParam &config);

    AM_RESULT load_config();
    AM_RESULT save_config();

    AMSmartRCConfig();
    virtual ~AMSmartRCConfig();
    void inc_ref();
    void release();

  private:
    static AMSmartRCConfig      *m_instance;
    static std::recursive_mutex m_mtx;
    AMSmartRCParam              m_config;
    std::atomic_int             m_ref_cnt;
};

#ifdef BUILD_AMBARELLA_ORYX_CONF_DIR
#define SMARTRC_CONF_DIR ((const char*)(BUILD_AMBARELLA_ORYX_CONF_DIR"/video"))
#else
#define SMARTRC_CONF_DIR ((const char*)"/etc/oryx/video")
#endif

#define SMARTRC_CONFIG_FILE "smartrc.acs"

#endif /* ORYX_VIDEO_CONTROL_BITRATE_SMARTRC_AM_SMARTRC_CONFIG_H_ */
