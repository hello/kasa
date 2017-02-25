/*******************************************************************************
 * am_muxer_config.cpp
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

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_amf_types.h"
#include "am_amf_interface.h"
#include "am_muxer_if.h"

#include "am_activemuxer_config.h"
#include "am_configure.h"
#include <algorithm>

AMActiveMuxerConfig::AMActiveMuxerConfig() :
  m_config(NULL),
  m_muxer_config(NULL)
{}

AMActiveMuxerConfig::~AMActiveMuxerConfig()
{
  delete m_config;
  delete m_muxer_config;
}

ActiveMuxerConfig* AMActiveMuxerConfig::get_config(const std::string& conf)
{
  ActiveMuxerConfig *config = NULL;

  do {
    if (AM_LIKELY(!m_muxer_config)) {
      m_muxer_config = new ActiveMuxerConfig();
    }

    if (AM_UNLIKELY(!m_muxer_config)) {
      ERROR("Failed to create MuxerConfig object!");
      break;
    }

    delete m_config;
    m_config = AMConfig::create(conf.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig &muxer = *m_config;

      if (AM_LIKELY(muxer["name"].exists())) {
        m_muxer_config->name = muxer["name"].get<std::string>("Muxer");
      } else {
        NOTICE("\"name\" is not specified for this filter, use default!");
        m_muxer_config->name = "Muxer";
      }

      if (AM_LIKELY(muxer["rt_config"].exists())) {
        m_muxer_config->real_time.enabled =
            muxer["rt_config"]["enabled"].get<bool>(false);
        m_muxer_config->real_time.priority =
            muxer["rt_config"]["priority"].get<unsigned>(10);
      } else {
        NOTICE("Thread priority configuration is not found, "
               "use default value(Realtime thread: disabled)!");
        m_muxer_config->real_time.enabled = false;
        m_muxer_config->real_time.priority = 10;
      }
      if (AM_LIKELY(muxer["auto_start"].exists())) {
        m_muxer_config->auto_start = muxer["auto_start"].get<bool>(true);
      } else {
        NOTICE("Auto_start is not found, use true as default value.");
        m_muxer_config->auto_start = true;
      }
      if (AM_LIKELY(muxer["event_gsensor_enable"].exists())) {
        m_muxer_config->event_gsensor_enable =
            muxer["event_gsensor_enable"].get<bool>(false);
      } else {
        NOTICE("event_gsensor_enable is not found, use false as default value.");
        m_muxer_config->event_gsensor_enable = false;
      }
      if (AM_LIKELY(muxer["muxer_type"].exists())) {
        std::string type = muxer["muxer_type"].get<std::string>("");
        if (is_str_equal(type.c_str(), "direct")) {
          m_muxer_config->type = AM_MUXER_TYPE_DIRECT;
        } else if (is_str_equal(type.c_str(), "file")) {
          m_muxer_config->type = AM_MUXER_TYPE_FILE;
        } else {
          ERROR("Unknown muxer type: %s", type.c_str());
          m_muxer_config->type = AM_MUXER_TYPE_NONE;
        }
      } else {
        NOTICE("\"muxer_type\" is not specified!, use default!");
        m_muxer_config->type = AM_MUXER_TYPE_NONE;
      }

      if (AM_LIKELY(muxer["media_type"].exists())) {
        uint32_t &media_type_num = m_muxer_config->media_type_num;
        media_type_num = muxer["media_type"].length();
        if (AM_LIKELY(media_type_num)) {
          uint32_t num = 0;
          std::string media_type[media_type_num];
          for (uint32_t i = 0; i < media_type_num; ++ i) {
            std::string temp = muxer["media_type"][i].get<std::string>("rtp");
            /* Remove redundant media types */
            for (uint32_t j = 0; j < media_type_num; ++ j) {
              if (AM_LIKELY(!media_type[j].empty() &&
                            (media_type[j] == temp))) {
                NOTICE("%s is already added!", temp.c_str());
              } else if (AM_LIKELY(media_type[j].empty())) {
                media_type[j] = temp;
                ++ num;
                break;
              }
            }
          }
          media_type_num = num;
          m_muxer_config->media_type = new std::string[media_type_num];
          if (AM_LIKELY(m_muxer_config->media_type)) {
            for (uint32_t i = 0; i < media_type_num; ++ i) {
              m_muxer_config->media_type[i] = media_type[i];
            }
          } else {
            ERROR("Failed to allocate memory for media type!");
            media_type_num = 0;
            break;
          }
        } else {
          ERROR("Media type is not specified!");
          break;
        }
      } else {
        ERROR("\"media_type\" is not specified!");
        break;
      }
      /*audio_type*/
      if (AM_LIKELY(muxer["audio_type"].exists())) {
        parse_audio_type(muxer);
      } else {
        WARN("audio type is not specifed!");
      }
      /*video_id*/
      if (AM_LIKELY(muxer["video_id"].exists())) {
        m_muxer_config->video_id = muxer["video_id"].get<unsigned>(0);
      } else {
        WARN("The video id is not specified, set 0 as default.");
        m_muxer_config->video_id = 0;
      }
      /*avqueue_pkt_pool_size*/
      if (AM_LIKELY(muxer["avqueue_pkt_pool_size"].exists())) {
        m_muxer_config->avqueue_pkt_pool_size =
            muxer["avqueue_pkt_pool_size"].get<unsigned>(128);
      } else {
        WARN("The avqueue_pkt_pool_size is not specified, set 128 as default.");
        m_muxer_config->avqueue_pkt_pool_size = 128;
      }
      /*event_max_history_duration*/
      if (AM_LIKELY(muxer["event_max_history_duration"].exists())) {
        m_muxer_config->event_max_history_duration =
            muxer["event_max_history_duration"].get<unsigned>(0);
      } else {
        WARN("The event_max_history_duration is not specified, "
            "set 0 as default.");
        m_muxer_config->event_max_history_duration = 0;
      }
      /*event_enable*/
      if (AM_LIKELY(muxer["event_enable"].exists())) {
        m_muxer_config->event_enable = muxer["event_enable"].get<bool>(false);
      } else {
        WARN("event_enable is not found, use false as default value.");
        m_muxer_config->event_enable = false;
      }
      /*event_audio_type*/
      if (AM_LIKELY(muxer["event_audio_type"].exists())) {
        std::string event_audio_type =
            muxer["event_audio_type"].get<std::string>("null");
        m_muxer_config->event_audio.first = audio_str_to_type(event_audio_type);
      } else {
        WARN("Event audio is not specified.");
      }
      /*event_audio_sample_rate*/
      if (AM_LIKELY(muxer["event_audio_sample_rate"].exists())) {
        m_muxer_config->event_audio.second =
            muxer["event_audio_sample_rate"].get<unsigned>(0);
      } else {
        WARN("Event audio sample rate is not specified.");
      }
      config = m_muxer_config;
    } else {
      ERROR("Failed to load configuration file: %s", conf.c_str());
    }
  }while(0);

  return config;
}

void AMActiveMuxerConfig::parse_audio_type(AMConfig &muxer)
{
  uint32_t audio_type_num = muxer["audio_type"].length();
  if (audio_type_num) {
    for (uint32_t i = 0; i < audio_type_num; ++ i) {
      string temp = muxer["audio_type"][i].get<std::string>("");
      if (is_str_equal(temp.c_str(), "aac-48k")){
        audio_type_operation(AM_AUDIO_AAC, 48000);
      } else if (is_str_equal(temp.c_str(), "aac-16k")) {
        audio_type_operation(AM_AUDIO_AAC, 16000);
      } else if (is_str_equal(temp.c_str(), "aac-8k")) {
        audio_type_operation(AM_AUDIO_AAC, 8000);
      } else if (is_str_equal(temp.c_str(), "opus-48k")) {
        audio_type_operation(AM_AUDIO_OPUS, 48000);
      } else if (is_str_equal(temp.c_str(), "opus-16k")) {
        audio_type_operation(AM_AUDIO_OPUS, 16000);
      } else if (is_str_equal(temp.c_str(), "opus-8k")) {
        audio_type_operation(AM_AUDIO_OPUS, 8000);
      } else if (is_str_equal(temp.c_str(), "speex-48k")) {
        audio_type_operation(AM_AUDIO_SPEEX, 48000);
      } else if (is_str_equal(temp.c_str(), "speex-16k")) {
        audio_type_operation(AM_AUDIO_SPEEX, 16000);
      } else if (is_str_equal(temp.c_str(), "speex-8k")) {
        audio_type_operation(AM_AUDIO_SPEEX, 8000);
      } else if (is_str_equal(temp.c_str(), "g711A-16k")) {
        audio_type_operation(AM_AUDIO_G711A, 16000);
      } else if (is_str_equal(temp.c_str(), "g711U-16k")) {
        audio_type_operation(AM_AUDIO_G711U, 16000);
      } else if (is_str_equal(temp.c_str(), "g711A-8k")) {
        audio_type_operation(AM_AUDIO_G711A,8000);
      } else if (is_str_equal(temp.c_str(), "g711U-8k")) {
        audio_type_operation(AM_AUDIO_G711U, 8000);
      } else if (is_str_equal(temp.c_str(), "g726_16-8k")) {
        audio_type_operation(AM_AUDIO_G726_16, 8000);
      } else if (is_str_equal(temp.c_str(), "g726_24-8k")) {
        audio_type_operation(AM_AUDIO_G726_24, 8000);
      } else if (is_str_equal(temp.c_str(), "g726_32-8k")) {
        audio_type_operation(AM_AUDIO_G726_32, 8000);
      } else if (is_str_equal(temp.c_str(), "g726_40-8k")) {
        audio_type_operation(AM_AUDIO_G726_40, 8000);
      } else {
        ERROR("Invalid audio type : %s", temp.c_str());
      }
    }
  } else {
    WARN("audio type is not specifed!");
  }
}

void AMActiveMuxerConfig::audio_type_operation(AM_AUDIO_TYPE type,
                                               uint32_t sample_rate)
{
  if (m_muxer_config->audio_types.find(type) ==
      m_muxer_config->audio_types.end()) {
    m_muxer_config->audio_types[type].push_back(sample_rate);

  } else {
    if (std::find(m_muxer_config->audio_types[type].begin(),
             m_muxer_config->audio_types[type].end(), sample_rate) ==
        m_muxer_config->audio_types[type].end()) {
      m_muxer_config->audio_types[type].push_back(sample_rate);

    } else {
      WARN("The %s-%u element has been set in the config file, "
           "ignore.", audio_type_to_str(type).c_str());
    }
  }
}

string AMActiveMuxerConfig::audio_type_to_str(AM_AUDIO_TYPE type)
{
  string type_str;
  switch (type) {
    case  AM_AUDIO_NULL : {
      type_str = "Null";
    } break;
    case AM_AUDIO_LPCM : {
      type_str = "Lpcm";
    } break;
    case AM_AUDIO_BPCM : {
      type_str = "Bpcm";
    } break;
    case AM_AUDIO_G711A : {
      type_str = "g711A";
    } break;
    case AM_AUDIO_G711U : {
      type_str = "g711U";
    } break;
    case AM_AUDIO_G726_40 : {
      type_str = "g726_40";
    } break;
    case AM_AUDIO_G726_32 : {
      type_str = "g726_32";
    } break;
    case AM_AUDIO_G726_24 : {
      type_str = "g726_24";
    } break;
    case AM_AUDIO_G726_16 : {
      type_str = "g726_16";
    } break;
    case AM_AUDIO_AAC : {
      type_str = "aac";
    } break;
    case AM_AUDIO_OPUS : {
      type_str = "opus";
    } break;
    case AM_AUDIO_SPEEX : {
      type_str = "speex";
    } break;
    default : {
    ERROR("Invalid audio type.");
    } break;
  }
  return type_str;
}

AM_AUDIO_TYPE AMActiveMuxerConfig::audio_str_to_type(std::string type_str)
{
  AM_AUDIO_TYPE type = AM_AUDIO_NULL;
  if (is_str_equal(type_str.c_str(), "aac")) {
    type = AM_AUDIO_AAC;
  } else if (is_str_equal(type_str.c_str(), "null") ||
      is_str_equal(type_str.c_str(), "none")) {
    type = AM_AUDIO_NULL;
  } else if (is_str_equal(type_str.c_str(), "lpcm")) {
    type = AM_AUDIO_LPCM;
  } else if (is_str_equal(type_str.c_str(), "bpcm")) {
    type = AM_AUDIO_BPCM;
  } else if (is_str_equal(type_str.c_str(), "g711A")) {
    type = AM_AUDIO_G711A;
  } else if (is_str_equal(type_str.c_str(), "g711U")) {
    type = AM_AUDIO_G711U;
  } else if (is_str_equal(type_str.c_str(), "g726_40")) {
    type = AM_AUDIO_G726_40;
  } else if (is_str_equal(type_str.c_str(), "g726_32")) {
    type = AM_AUDIO_G726_32;
  } else if (is_str_equal(type_str.c_str(), "g726_24")) {
    type = AM_AUDIO_G726_24;
  } else if (is_str_equal(type_str.c_str(), "g726_16")) {
    type = AM_AUDIO_G726_16;
  } else if (is_str_equal(type_str.c_str(), "opus")) {
    type = AM_AUDIO_OPUS;
  } else if (is_str_equal(type_str.c_str(), "speex")) {
    type = AM_AUDIO_SPEEX;
  } else {
    ERROR("Invalid audio type : %s", type_str.c_str());
  }
  return type;
}
