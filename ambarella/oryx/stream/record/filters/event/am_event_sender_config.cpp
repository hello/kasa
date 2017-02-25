/**
 * am_event_sender_config.cpp
 *
 *  History:
 *    Mar 6, 2015 - [Shupeng Ren] created file
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

#include "am_event_sender_config.h"
#include "am_configure.h"

AMEventSenderConfig::AMEventSenderConfig() :
  m_config(nullptr),
  m_event_sender_config(nullptr)
{
}

AMEventSenderConfig::~AMEventSenderConfig()
{
  AM_DESTROY(m_config);
  delete m_event_sender_config;
}

EventSenderConfig* AMEventSenderConfig::get_config(const std::string& conf)
{
  EventSenderConfig *event_config = nullptr;

  do {
    if (nullptr == m_event_sender_config) {
      if (!(m_event_sender_config = new EventSenderConfig)) {
        ERROR("Failed to create AMEventSenderConfig!");
        break;
      }
    }
    delete m_config;
    if (!(m_config = AMConfig::create(conf.c_str()))) {
      ERROR("Failed to load configuration file: %s!", conf.c_str());
      break;
    }

    AMConfig &config = *m_config;

    if (config["name"].exists()) {
      m_event_sender_config->name =
          config["name"].get<std::string>("EventSender");
    } else {
      NOTICE("\"name\" is not specified for this filter, use default!");
      m_event_sender_config->name = "EventSender";
    }

    if (config["rt_config"].exists()) {
      m_event_sender_config->real_time.enabled =
          config["rt_config"]["enabled"].get<bool>(false);
      m_event_sender_config->real_time.priority =
          config["rt_config"]["priority"].get<unsigned>(10);
    } else {
      NOTICE("Thread priority configuration is not found, "
          "use default value(Realtime thread: disabled)!");
      m_event_sender_config->real_time.enabled = false;
    }

    if (config["packet_pool_size"].exists()) {
      m_event_sender_config->packet_pool_size =
          config["packet_pool_size"].get<unsigned>(64);
    } else {
      NOTICE("\"packet_pool_size\" is not specified, use default!");
      m_event_sender_config->packet_pool_size = 64;
    }
    event_config = m_event_sender_config;
  } while (0);

  if (!event_config) {
    AM_DESTROY(m_config);
    delete m_event_sender_config;
    m_event_sender_config = nullptr;
  }

  return event_config;
}
