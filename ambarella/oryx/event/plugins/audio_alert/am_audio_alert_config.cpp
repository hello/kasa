/*
 * am_audio_alert_config.cpp
 *
 * Histroy:
 *  2014-11-19 [Dongge Wu] create file
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_audio_alert_config.h"

AMAudioAlertConfig::AMAudioAlertConfig() :
    m_config(NULL),
    m_alert_config(NULL)
{

}

AMAudioAlertConfig::~AMAudioAlertConfig()
{
  delete m_config;
  delete m_alert_config;
}

AudioAlertConfig* AMAudioAlertConfig::get_config(const std::string& config)
{
  do {
    delete m_config;
    m_config = NULL;
    m_config = AMConfig::create(config.c_str());
    if (!m_config) {
      ERROR("Create AMConfig failed!\n");
      break;
    }

    if (!m_alert_config) {
      m_alert_config = new AudioAlertConfig();
      if (!m_alert_config) {
        ERROR("new audio alert config failed!\n");
        break;
      }
    }

    AMConfig &audio_alert = *m_config;
    if (audio_alert["enable"].exists()) {
      m_alert_config->enable_alert_detect =
          audio_alert["enable"].get<bool>(false);
    } else {
      m_alert_config->enable_alert_detect = false;
    }

    if (audio_alert["channel_num"].exists()) {
      m_alert_config->channel_num =
          audio_alert["channel_num"].get<int>(2);
      if (m_alert_config->channel_num != 1
          && m_alert_config->channel_num != 2) {
        WARN("Invalid parameter channel number : %d \n",
             m_alert_config->channel_num);
      }
    } else {
      WARN("channel_num is not specified, use default!\n");
      m_alert_config->channel_num = 2;
    }

    if (audio_alert["sample_rate"].exists()) {
      m_alert_config->sample_rate =
          audio_alert["sample_rate"].get<int>(48000);
      if ((48000 != m_alert_config->sample_rate)
          && (44100 != m_alert_config->sample_rate)
          && (32000 != m_alert_config->sample_rate)
          && (24000 != m_alert_config->sample_rate)
          && (22050 != m_alert_config->sample_rate)
          && (16000 != m_alert_config->sample_rate)
          && (12000 != m_alert_config->sample_rate)
          && (11025 != m_alert_config->sample_rate)
          && (8000 != m_alert_config->sample_rate)) {
        WARN("Invalid parameter sample rate : %d \n",
             m_alert_config->sample_rate);
      }
    } else {
      WARN("sample_rate is not specified, use default!\n");
      m_alert_config->sample_rate = 48000;
    }

    if (audio_alert["chunk_bytes"].exists()) {
      m_alert_config->chunk_bytes =
          audio_alert["chunk_bytes"].get<int>(2048);
    } else {
      m_alert_config->chunk_bytes = 2048;
    }

    if (audio_alert["sample_format"].exists()) {
      m_alert_config->sample_format = (AM_AUDIO_SAMPLE_FORMAT)
          audio_alert["sample_format"].get<unsigned>(3);
    } else {
      WARN("sample_format is not specified, use default!\n");
      m_alert_config->sample_format = AM_SAMPLE_S16LE;
    }

    if (audio_alert["sensitivity"].exists()) {
      m_alert_config->sensitivity =
          audio_alert["sensitivity"].get<int>(50);
      if (m_alert_config->sensitivity < 0
          || m_alert_config->sensitivity > 100) {
        WARN("Invalid parameter sensitivity : %d \n",
             m_alert_config->sensitivity);
      }
    } else {
      WARN("sensitivity is not specified, use default!\n");
      m_alert_config->sensitivity = 50;
    }

    if (audio_alert["threshold"].exists()) {
      m_alert_config->threshold = audio_alert["threshold"].get<
          int>(50);
      if (m_alert_config->threshold < 0 || m_alert_config->threshold > 100) {
        WARN("Invalid parameter threshold : %d \n",
             m_alert_config->threshold);
      }
    } else {
      WARN("threshold is not specified, use default!\n");
      m_alert_config->threshold = 50;
    }

    if (audio_alert["direction"].exists()) {
      m_alert_config->direction = audio_alert["direction"].get<
          int>(0);
      if (m_alert_config->direction < 0 || m_alert_config->direction > 1) {
        WARN("Invalid parameter direction : %d\n", m_alert_config->direction);
      }
    } else {
      WARN("direction is not specified, use default!\n");
      m_alert_config->direction = 0;
    }
  } while (0);

  return m_alert_config;
}

bool AMAudioAlertConfig::set_config(AudioAlertConfig *alert_config,
                                    const std::string& config)
{
  bool ret = true;
  do {
    delete m_config;
    m_config = NULL;

    m_config = AMConfig::create(config.c_str());
    if (!m_config) {
      ERROR("Create AMConfig failed!\n");
      ret = false;
      break;
    }

    if (!m_alert_config) {
      m_alert_config = new AudioAlertConfig();
      if (!m_alert_config) {
        ERROR("new audio alert config failed!\n");
        ret = false;
        break;
      }
    }

    memcpy(m_alert_config, alert_config, sizeof(AudioAlertConfig));
    AMConfig &audio_alert = *m_config;
    if (audio_alert["enable"].exists()) {
      audio_alert["enable"] =
          m_alert_config->enable_alert_detect;
    } else {
      ret = false;
      ERROR("Invalid parameter enable \n");
    }

    if (audio_alert["channel_num"].exists()) {
      audio_alert["channel_num"] = m_alert_config->channel_num;
    } else {
      ret = false;
      ERROR("Invalid parameter channel_num \n");
    }

    if (audio_alert["sample_rate"].exists()) {
      audio_alert["sample_rate"] = m_alert_config->sample_rate;
    } else {
      ret = false;
      ERROR("Invalid parameter sample rate \n");
    }

    if (audio_alert["chunk_bytes"].exists()) {
      audio_alert["chunk_bytes"] = m_alert_config->chunk_bytes;
    } else {
      ret = false;
      ERROR("Invalid parameter chunk bytes \n");
    }

    if (audio_alert["sample_format"].exists()) {
      audio_alert["sample_format"] =
          (uint32_t)m_alert_config->sample_format;
    } else {
      ret = false;
      ERROR("Invalid parameter sample format \n");
    }

    if (audio_alert["sensitivity"].exists()) {
      audio_alert["sensitivity"] = m_alert_config->sensitivity;
    } else {
      ret = false;
      ERROR("Invalid parameter sensitivity \n");
    }

    if (audio_alert["threshold"].exists()) {
      audio_alert["threshold"] = m_alert_config->threshold;
    } else {
      ret = false;
      ERROR("Invalid parameter enable \n");
    }

    if (audio_alert["direction"].exists()) {
      audio_alert["direction"] = m_alert_config->direction;
    } else {
      ret = false;
      ERROR("Invalid parameter enable \n");
    }

     ret = audio_alert.save();
  } while (0);

  return ret;
}

