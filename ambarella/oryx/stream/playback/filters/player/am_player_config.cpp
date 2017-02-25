/*******************************************************************************
 * am_player_config.cpp
 *
 * History:
 *   2014-9-11 - [ypchang] created file
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

#include "am_player_config.h"
#include "am_configure.h"
#include "pulse/pulseaudio.h"

AMPlayerConfig::AMPlayerConfig() :
  m_config(NULL),
  m_player_config(NULL)
{}

AMPlayerConfig::~AMPlayerConfig()
{
  delete m_config;
  delete m_player_config;
}

PlayerConfig* AMPlayerConfig::get_config(const std::string& conf_file)
{
  PlayerConfig *config = NULL;
  do {
    if (AM_LIKELY(NULL == m_player_config)) {
      m_player_config = new PlayerConfig();
    }
    if (AM_UNLIKELY(!m_player_config)) {
      ERROR("Failed to create PlayerConfig object!");
      break;
    }
    delete m_config;
    m_config = AMConfig::create(conf_file.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig& player = *m_config;

      if (AM_LIKELY(player["name"].exists())) {
        m_player_config->name = player["name"].get<std::string>("Player");
      } else {
        NOTICE("\"name\" is not specified for this filter, use default!");
        m_player_config->name = "Player";
      }
      if (AM_LIKELY(player["rt_config"].exists())) {
        m_player_config->real_time.enabled =
            player["rt_config"]["enabled"].get<bool>(false);
        m_player_config->real_time.priority =
            player["rt_config"]["priority"].get<unsigned>(10);
      } else {
        NOTICE("Thread priority configuration is not found, "
               "use default value(Realtime thread: disabled)!");
        m_player_config->real_time.enabled = false;
      }
      if (AM_LIKELY(player["pause_check_interval"].exists())) {
        m_player_config->pause_check_interval =
            player["pause_check_interval"].get<unsigned>(10000);
      } else {
        NOTICE("Invalid player configuration file, "
              "\"pause_check_interval\" doesn't exist, use default value!");
        m_player_config->pause_check_interval = 10000;
      }
      if (AM_LIKELY(player["audio"].exists())) {
        if (AM_LIKELY(player["audio"]["interface"].exists())) {
          m_player_config->audio.interface =
              player["audio"]["interface"].get<std::string>("pulse");
        } else {
          NOTICE("Invalid player configuration file, "
              "\"audio.interface\" section doesn't exist, use default value!");
          m_player_config->audio.interface = "pulse";
        }
        if (AM_LIKELY(player["audio"]["def_channel_num"].exists())) {
          m_player_config->audio.def_channel_num =
              player["audio"]["def_channel_num"].get<unsigned>(2);
          if (AM_UNLIKELY(m_player_config->audio.def_channel_num >
                          PA_CHANNELS_MAX)) {
            NOTICE("Channel number is too large, "
                   "maximum supported channel number is %u, reset to stero!",
                   PA_CHANNELS_MAX);
            m_player_config->audio.def_channel_num = 2;
          }
        } else {
          NOTICE("Invalid player configuration file, "
                 "\"audio.def_channel_num\" section doesn't exist, "
                 "use default value!");
          m_player_config->audio.def_channel_num = 2;
        }
        if (AM_LIKELY(player["audio"]["buffer_delay_ms"].exists())) {
          m_player_config->audio.buffer_delay_ms =
              player["audio"]["buffer_delay_ms"].get<unsigned>(100);
        } else {
          NOTICE("Invalid player configuration file, "
                 "\"audio.buffer_delay_ms\" section doesn't exist, "
                 "use default value!");
          m_player_config->audio.buffer_delay_ms = 100;
        }
        if (AM_LIKELY(player["audio"]["initial_volume"].exists())) {
          m_player_config->audio.initial_volume =
              player["audio"]["initial_volume"].get<int>(-1);
        } else {
          NOTICE("Invalid player configuration file, "
              "\"audio.initial_volume\" section doesn't exist, "
              "use default value!");
          m_player_config->audio.initial_volume = -1;
        }
        if (AM_LIKELY(player["audio"]["enable_aec"].exists())) {
          m_player_config->audio.enable_aec =
              player["audio"]["enable_aec"].get<bool>(true);
        } else {
          NOTICE("Invalid player configuration file, "
                 "\"audio.enable_aec\" is not specified, "
                 "use true as default!");
          m_player_config->audio.enable_aec = true;
        }
      } else {
        NOTICE("Invalid player configuration file, "
               "\"audio\" section doesn't exist, use default value!");
        m_player_config->audio.interface = "pulse";
        m_player_config->audio.buffer_delay_ms = 100;
        m_player_config->audio.initial_volume = -1;
      }
      config = m_player_config;
    } else {
      ERROR("Failed to load configuration file: %s", conf_file.c_str());
    }
  }while(0);

  return config;
}
