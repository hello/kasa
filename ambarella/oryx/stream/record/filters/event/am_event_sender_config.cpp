/**
 * am_event_sender_config.cpp
 *
 *  History:
 *    Mar 6, 2015 - [Shupeng Ren] created file
 *
 * Copyright (C) 2007-2015, Ambarella, Inc.
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
