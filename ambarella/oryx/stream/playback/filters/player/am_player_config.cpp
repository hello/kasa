/*******************************************************************************
 * am_player_config.cpp
 *
 * History:
 *   2014-9-11 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
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
