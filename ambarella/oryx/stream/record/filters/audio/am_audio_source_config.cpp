/*******************************************************************************
 * am_audio_source_config.cpp
 *
 * History:
 *   2014-12-4 - [ypchang] created file
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

#include "am_audio_source_config.h"
#include "am_configure.h"

AMAudioSourceConfig::AMAudioSourceConfig() :
  m_config(NULL),
  m_audio_source_config(NULL)
{}

AMAudioSourceConfig::~AMAudioSourceConfig()
{
  delete m_config;
  delete m_audio_source_config;
}

AudioSourceConfig* AMAudioSourceConfig::get_config(const std::string &conf_file)
{
  AudioSourceConfig *config = NULL;

  do {
    if (AM_LIKELY(NULL == m_audio_source_config)) {
      m_audio_source_config = new AudioSourceConfig();
    }
    if (AM_UNLIKELY(!m_audio_source_config)) {
      ERROR("Failed to create AudioSourceConfig object!");
      break;
    }

    delete m_config;
    m_config = AMConfig::create(conf_file.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig &asource = *m_config;

      if (AM_LIKELY(asource["name"].exists())) {
        m_audio_source_config->name = asource["name"].\
            get<std::string>("AudioSource");
      } else {
        NOTICE("\"name\" is not specified for this filter, use default!");
        m_audio_source_config->name = "AudioSource";
      }

      if (AM_LIKELY(asource["rt_config"].exists())) {
        m_audio_source_config->real_time.enabled =
            asource["rt_config"]["enabled"].get<bool>(false);
        m_audio_source_config->real_time.priority =
            asource["rt_config"]["priority"].get<unsigned>(10);
      } else {
        NOTICE("Thread priority configuration is not found, "
               "use default value(Realtime thread: disabled)!");
        m_audio_source_config->real_time.enabled = false;
      }

      if (AM_LIKELY(asource["packet_pool_size"].exists())) {
        m_audio_source_config->packet_pool_size =
            asource["packet_pool_size"].get<unsigned>(64);
      } else {
        NOTICE("\"packet_pool_size\" is not specified, use default!");
        m_audio_source_config->packet_pool_size = 64;
      }

      if (AM_LIKELY(asource["initial_volume"].exists())) {
        m_audio_source_config->initial_volume =
            asource["initial_volume"].get<signed>(90);
      } else {
        NOTICE("\"initial_volume\" is not specified, use default!");
        m_audio_source_config->initial_volume = 90;
      }

      m_audio_source_config->audio_type_num = 0;
      if (AM_LIKELY(asource["audio_profile"].exists())) {
        std::string &aprofile = m_audio_source_config->audio_profile;
        aprofile = asource["audio_profile"].get<std::string>("highest");
      } else {
        NOTICE("\"audio_profile\" is not specified, use default!");
        m_audio_source_config->audio_profile = "highest";
      }

      if (AM_LIKELY(asource["audio_type"].exists())) {
        std::string &aprofile = m_audio_source_config->audio_profile;
        if (asource["audio_type"][aprofile].exists()) {
          uint32_t audio_type_num = asource["audio_type"][aprofile].length();
          if (AM_LIKELY(audio_type_num)) {
            uint32_t num = 0;
            std::string audio_type[audio_type_num];
            for (uint32_t j = 0; j < audio_type_num; ++ j) {
              std::string temp = asource["audio_type"][aprofile][j].
                  get<std::string>();
              if (AM_LIKELY(!temp.empty())) {
                /* Remove redundant audio types */
                for (uint32_t k = 0; k < audio_type_num; ++ k) {
                  if (AM_LIKELY(!audio_type[k].empty() &&
                                (audio_type[k] == temp))) {
                    NOTICE("%s is already added!", temp.c_str());
                  } else if (AM_LIKELY(audio_type[k].empty())) {
                    audio_type[k] = temp;
                    ++ num;
                    break;
                  }
                }
              }
            }
            audio_type_num = num;
            if (AM_LIKELY(audio_type_num)) {
              m_audio_source_config->audio_type_map[aprofile] =
                  new AMStringList();
              if (AM_LIKELY(m_audio_source_config->audio_type_map[aprofile])) {
                for (uint32_t l = 0; l < audio_type_num; ++ l) {
                  m_audio_source_config->audio_type_map[aprofile]->\
                  push_back(audio_type[l]);
                }
              } else {
                ERROR("Failed to allocate memory for audio profile: %s!",
                      aprofile.c_str());
              }
            } else {
              NOTICE("No valid audio type is found, audio will be disabled!");
              m_audio_source_config->audio_profile = "none";
            }
          } else {
            NOTICE("audio type is not specified, audio will be disabled!");
            m_audio_source_config->audio_profile = "none";
          }
        } else {
          WARN("Audio profile %s is undefined, audio will be disabled!",
               aprofile.c_str());
          m_audio_source_config->audio_profile = "none";
        }
      } else {
        NOTICE("\"audio_type\" is not specified, audio will be disabled!");
        m_audio_source_config->audio_profile = "none";
      }

      m_audio_source_config->audio_type_num =
          m_audio_source_config->\
          audio_type_map[m_audio_source_config->audio_profile] ?
              m_audio_source_config->\
              audio_type_map[m_audio_source_config->audio_profile]->size() : 0;
      if (AM_LIKELY(asource["interface"].exists())) {
        m_audio_source_config->interface = asource["interface"].\
            get<std::string>("pulse");
      } else {
        NOTICE("\"interface\" is not specified for %s, "
               "use \"pulse\" as default!",
               m_audio_source_config->name.c_str());
        m_audio_source_config->interface = "pulse";
      }

      if (AM_LIKELY(asource["enable_aec"].exists())) {
        m_audio_source_config->enable_aec =
            asource["enable_aec"].get<bool>(true);
      } else {
        NOTICE("\"enable_aec\" is not specified for %s, enable by default!",
               m_audio_source_config->name.c_str());
        m_audio_source_config->enable_aec = true;
      }
      /*codec_enable*/
      if (AM_LIKELY(asource["codec_enable"].exists())) {
        m_audio_source_config->codec_enable =
            asource["codec_enable"].get<bool>(true);
      } else {
        NOTICE("\"codec_enable\" is not specified for %s, enable by default!",
               m_audio_source_config->name.c_str());
        m_audio_source_config->codec_enable = true;
      }
      config = m_audio_source_config;
    } else {
      ERROR("Failed to load configuration file: %s", conf_file.c_str());
    }
  }while(0);

  return config;
}
