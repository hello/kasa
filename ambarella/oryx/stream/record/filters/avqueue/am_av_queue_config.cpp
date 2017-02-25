/**
 * am_av_queue_config.cpp
 *
 *  History:
 *		Dec 29, 2014 - [Shupeng Ren] created file
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
 */

#include "am_base_include.h"
#include "am_define.h"
#include "am_amf_types.h"
#include "am_log.h"
#include <map>

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

    for (size_t i = 0; i < config["event"].length(); ++i) {
      if (config["event"][i]["enable"].exists()) {
        m_avqueue_config->event_config[i].enable =
            config["event"][i]["enable"].get<bool>(false);
      } else {
        NOTICE("\"event[%d] enable\" is not specified, use default!", i);
      }

      if (config["event"][i]["video_id"].exists()) {
        m_avqueue_config->event_config[i].video_id =
            config["event"][i]["video_id"].get<int>(0);
      } else {
        NOTICE("\"event[%d] video_id\" is not specified, use default!", i);
      }

      if (config["event"][i]["audio_type"].exists()) {
        string audio_type = config["event"][i]["audio_type"].get<string>("aac");
        if (is_str_equal(audio_type.c_str(), "aac")) {
          m_avqueue_config->event_config[i].audio_type = AM_AUDIO_AAC;
        } else if (is_str_equal(audio_type.c_str(), "opus")) {
          m_avqueue_config->event_config[i].audio_type = AM_AUDIO_OPUS;
        } else if (is_str_equal(audio_type.c_str(), "lpcm")) {
          m_avqueue_config->event_config[i].audio_type = AM_AUDIO_LPCM;
        } else if (is_str_equal(audio_type.c_str(), "bpcm")) {
          m_avqueue_config->event_config[i].audio_type = AM_AUDIO_BPCM;
        } else {
          m_avqueue_config->event_config[i].audio_type = AM_AUDIO_AAC;
        }
      } else {
        NOTICE("\"event[%d] audio_type\" is not specified, use default!", i);
      }

      if (config["event"][i]["history_duration"].exists()) {
        m_avqueue_config->event_config[i].history_duration =
            config["event"][i]["history_duration"].get<unsigned>(0);
        if (m_avqueue_config->event_config[i].history_duration >
        AM_AVQUEUE_MAX_HISTORY_DURATION) {
          NOTICE("event[%d] history duration can't exceed %ds. Set to %ds!",
                 i,
                 AM_AVQUEUE_MAX_HISTORY_DURATION,
                 AM_AVQUEUE_MAX_HISTORY_DURATION);
          m_avqueue_config->event_config[i].history_duration =
              AM_AVQUEUE_MAX_HISTORY_DURATION;
        }
      } else {
        NOTICE("\"event[%d] history_duration\" is not specified, use default!", i);
      }

      if (config["event"][i]["future_duration"].exists()) {
        m_avqueue_config->event_config[i].future_duration =
            config["event"][i]["future_duration"].get<unsigned>(0);
      } else {
        NOTICE("\"event[%d] future_duration\" is not specified, use default!", i);
      }
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
