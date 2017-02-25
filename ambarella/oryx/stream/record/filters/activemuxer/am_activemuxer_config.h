/*******************************************************************************
 * am_muxer_config.h
 *
 * History:
 *   2014-12-26 - [ypchang] created file
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
#ifndef ORYX_STREAM_RECORD_FILTERS_MUXER_AM_MUXER_CONFIG_H_
#define ORYX_STREAM_RECORD_FILTERS_MUXER_AM_MUXER_CONFIG_H_

#include "am_filter_config.h"
#include "am_audio_define.h"
#include <map>
#include <vector>

using std::map;
using std::pair;
using std::vector;
using std::string;

struct ActiveMuxerConfig: public AMFilterConfig
{
    string       *media_type                  = nullptr;
    uint32_t      media_type_num              = 0;
    uint32_t      avqueue_pkt_pool_size       = 0;
    AM_MUXER_TYPE type                        = AM_MUXER_TYPE_NONE;
    uint32_t      video_id                    = 0;
    uint32_t      event_max_history_duration  = 0;
    bool          auto_start                  = true;
    bool          event_enable                = true;
    bool          event_gsensor_enable        = false;
    pair<AM_AUDIO_TYPE, uint32_t> event_audio = {AM_AUDIO_NULL, 0};
    map<AM_AUDIO_TYPE, vector<uint32_t>> audio_types;
    ActiveMuxerConfig()
    {}
    ~ActiveMuxerConfig()
    {
      delete[] media_type;
    }
};

class AMConfig;
class AMActiveMuxerConfig
{

  public:
    AMActiveMuxerConfig();
    virtual ~AMActiveMuxerConfig();
    ActiveMuxerConfig* get_config(const std::string& conf_file);
  private :
    inline void parse_audio_type(AMConfig &muxer);
    inline void audio_type_operation(AM_AUDIO_TYPE type, uint32_t sample_rate);
    inline std::string audio_type_to_str(AM_AUDIO_TYPE type);
    inline AM_AUDIO_TYPE audio_str_to_type(std::string type);
  private:
    AMConfig    *m_config;
    ActiveMuxerConfig *m_muxer_config;
};


#endif /* ORYX_STREAM_RECORD_FILTERS_MUXER_AM_MUXER_CONFIG_H_ */
