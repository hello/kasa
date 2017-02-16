/*******************************************************************************
 * am_video_source_config.cpp
 *
 * History:
 *   2014-12-11 - [ypchang] created file
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

#include "am_video_source_config.h"
#include "am_configure.h"

AMVideoSourceConfig::AMVideoSourceConfig() :
  m_config(NULL),
  m_video_source_config(NULL)
{
}

AMVideoSourceConfig::~AMVideoSourceConfig()
{
  delete m_config;
  delete m_video_source_config;
}

VideoSourceConfig* AMVideoSourceConfig::get_config(const std::string& conf_file)
{
  VideoSourceConfig *config = NULL;
  do {
    if (AM_LIKELY(NULL == m_video_source_config)) {
      m_video_source_config = new VideoSourceConfig();
    }
    if (AM_UNLIKELY(!m_video_source_config)) {
      ERROR("Failed to create VideoSourceConfig object!");
      break;
    }

    delete m_config;
    m_config = AMConfig::create(conf_file.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig &vsource = *m_config;

      if (AM_LIKELY(vsource["name"].exists())) {
        m_video_source_config->name = vsource["name"].\
            get<std::string>("VideoSource");
      } else {
        NOTICE("\"name\" is not specified for this filter, use default!");
        m_video_source_config->name = "VideoSource";
      }

      if (AM_LIKELY(vsource["rt_config"].exists())) {
        m_video_source_config->real_time.enabled =
            vsource["rt_config"]["enabled"].get<bool>(false);
        m_video_source_config->real_time.priority =
            vsource["rt_config"]["priority"].get<unsigned>(10);
      } else {
        NOTICE("Thread priority configuration is not found, "
               "use default value(Realtime thread: disabled!");
        m_video_source_config->real_time.enabled = false;
      }

      if (AM_LIKELY(vsource["packet_pool_size"].exists())) {
        m_video_source_config->packet_pool_size =
            vsource["packet_pool_size"].get<unsigned>(64);
      } else {
        NOTICE("\"packet_pool_size\" is not specified, use default!");
        m_video_source_config->packet_pool_size = 64;
      }

      if (AM_LIKELY(vsource["query_op_timeout"].exists())) {
        m_video_source_config->query_op_timeout =
            vsource["query_op_timeout"].get<signed>(1000);
      } else {
        NOTICE("\"query_op_timeout\" is not specified, use default!");
        m_video_source_config->query_op_timeout = 1000;
      }

      config = m_video_source_config;
    }
  }while(0);

  return config;
}
