/**
 * am_av_queue_config.cpp
 *
 *  History:
 *		Dec 29, 2014 - [Shupeng Ren] created file
 *
 * Copyright (C) 2007-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include "am_base_include.h"
#include "am_define.h"
#include "am_amf_types.h"
#include "am_log.h"

#include "am_av_queue_config.h"
#include "am_configure.h"

#define AM_AVQUEUE_MAX_HISTORY_DURATION 10

AMAVQueueConfig::AMAVQueueConfig() :
  m_config(nullptr),
  m_avqueue_config(nullptr)
{
}

AMAVQueueConfig::~AMAVQueueConfig()
{
  AM_DESTROY(m_config);
  delete m_avqueue_config;
}

AVQueueConfig* AMAVQueueConfig::get_config(const std::string& conf)
{
  AVQueueConfig *avq_config = nullptr;

  do {
    if (nullptr == m_avqueue_config) {
      if (!(m_avqueue_config = new AVQueueConfig())) {
        ERROR("Failed to create AMAVQueueConfig");
        break;
      }
    }

    delete m_config;
    if (!(m_config = AMConfig::create(conf.c_str()))) {
      ERROR("Failed to load configuration file: %s", conf.c_str());
      break;
    }

    AMConfig &config = *m_config;

    if (config["name"].exists()) {
      m_avqueue_config->name = config["name"].get<std::string>("AVQueue");
    } else {
      NOTICE("\"name\" is not specified for this filter, use default!");
      m_avqueue_config->name = "AVQueue";
    }

    if (config["rt_config"].exists()) {
      m_avqueue_config->real_time.enabled =
          config["rt_config"]["enabled"].get<bool>(false);
      m_avqueue_config->real_time.priority =
          config["rt_config"]["priority"].get<unsigned>(10);
    } else {
      NOTICE("Thread priority configuration is not found, "
          "use default value(Realtime thread: disabled)!");
      m_avqueue_config->real_time.enabled = false;
    }

    if (config["packet_pool_size"].exists()) {
      m_avqueue_config->packet_pool_size =
          config["packet_pool_size"].get<unsigned>(64);
    } else {
      NOTICE("\"packet_pool_size\" is not specified, use default!");
      m_avqueue_config->packet_pool_size = 64;
    }

    if (config["event_enable"].exists()) {
      m_avqueue_config->event_enable =
          config["event_enable"].get<bool>(false);
    } else {
      NOTICE("\"event_enable\" is not specified, use default!");
            m_avqueue_config->event_enable = false;
    }

    if (config["event_stream_id"].exists()) {
      m_avqueue_config->event_stream_id =
          config["event_stream_id"].get<unsigned>(0);
    } else {
      NOTICE("\"event_stream_id\" is not specified, use default!");
      m_avqueue_config->event_stream_id = 0;
    }

    if (config["event_history_duration"].exists()) {
      m_avqueue_config->event_history_duration =
          config["event_history_duration"].get<unsigned>(0);
      if (m_avqueue_config->event_history_duration >
      AM_AVQUEUE_MAX_HISTORY_DURATION) {
        NOTICE("event history duration can't exceed %ds. Set to %ds!",
               AM_AVQUEUE_MAX_HISTORY_DURATION,
               AM_AVQUEUE_MAX_HISTORY_DURATION);
        m_avqueue_config->event_history_duration =
            AM_AVQUEUE_MAX_HISTORY_DURATION;
      }
    } else {
      NOTICE("\"event_history_duration\" is not specified, use default!");
      m_avqueue_config->event_history_duration = 0;
    }

    if (config["event_future_duration"].exists()) {
      m_avqueue_config->event_future_duration =
          config["event_future_duration"].get<unsigned>(0);
    } else {
      NOTICE("\"event_future_duration\" is not specified, use default!");
      m_avqueue_config->event_future_duration = 0;
    }

    if (config["event_audio_type"].exists()) {
      std::string audio_type = config["event_audio_type"].get<std::string>("aac");
      if (is_str_equal(audio_type.c_str(), "aac")) {
        m_avqueue_config->event_audio_type = AM_AUDIO_AAC;
      } else if (is_str_equal(audio_type.c_str(), "opus")) {
        m_avqueue_config->event_audio_type = AM_AUDIO_OPUS;
      } else if (is_str_equal(audio_type.c_str(), "lpcm")) {
        m_avqueue_config->event_audio_type = AM_AUDIO_LPCM;
      } else if (is_str_equal(audio_type.c_str(), "bpcm")) {
        m_avqueue_config->event_audio_type = AM_AUDIO_BPCM;
      } else {
        m_avqueue_config->event_audio_type = AM_AUDIO_AAC;
      }
    } else {
      NOTICE("\"event_future_duration\" is not specified, use default!");
      m_avqueue_config->event_audio_type = AM_AUDIO_AAC;
    }
    avq_config = m_avqueue_config;
  } while (0);

  if (!avq_config) {
    AM_DESTROY(m_config);
    if (m_avqueue_config) {
      delete m_avqueue_config;
      m_avqueue_config = nullptr;
    }
  }

  return avq_config;
}
