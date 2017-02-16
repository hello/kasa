/*******************************************************************************
 * am_file_demuxer_config.cpp
 *
 * History:
 *   2014-9-5 - [ypchang] created file
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

#include "am_file_demuxer_config.h"
#include "am_configure.h"

AMFileDemuxerConfig::AMFileDemuxerConfig() :
  m_config(NULL),
  m_demuxer_config(NULL)
{}

AMFileDemuxerConfig::~AMFileDemuxerConfig()
{
  delete m_config;
  delete m_demuxer_config;
}

FileDemuxerConfig* AMFileDemuxerConfig::get_config(const std::string& conf_file)
{
  FileDemuxerConfig *config = NULL;
  do {
    if (AM_LIKELY(NULL == m_demuxer_config)) {
      m_demuxer_config = new FileDemuxerConfig();
    }
    if (AM_UNLIKELY(!m_demuxer_config)) {
      ERROR("Failed to create FileDemuxerConfig object!");
      break;
    }
    delete m_config;
    m_config = AMConfig::create(conf_file.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig& demuxer = *m_config;

      if (AM_LIKELY(demuxer["name"].exists())) {
        m_demuxer_config->name =
            demuxer["name"].get<std::string>("FileDemuxer");
      } else {
        NOTICE("\"name\" is not specified for this filter, use default!");
        m_demuxer_config->name = "FileDemuxer";
      }
      if (AM_LIKELY(demuxer["rt_config"].exists())) {
        m_demuxer_config->real_time.enabled =
            demuxer["rt_config"]["enabled"].get<bool>(false);
        m_demuxer_config->real_time.priority =
            demuxer["rt_config"]["priority"].get<unsigned>(10);
      } else {
        NOTICE("Thread priority configuration is not found, "
               "use default value(RealTime thread: disabled)!");
        m_demuxer_config->real_time.enabled = false;
        m_demuxer_config->real_time.priority = 10;
      }
      if (AM_LIKELY(demuxer["packet_pool_size"].exists())) {
        m_demuxer_config->packet_pool_size =
            demuxer["packet_pool_size"].get<unsigned>(4);
      } else {
        ERROR("Invalid file demuxer configuration file, "
            "\"packet_pool_size\" doesn't exist!");
        break;
      }
      if (AM_LIKELY(demuxer["packet_size"].exists())) {
        m_demuxer_config->packet_size = demuxer["packet_size"].get<unsigned>(0);
      } else {
        ERROR("Invalid file demuxer configuration file, "
            "\"packet_size\" doesn't exist!");
        break;
      }
      if (AM_LIKELY(demuxer["wait_count"].exists())) {
        m_demuxer_config->wait_count = demuxer["wait_count"].get<unsigned>(10);
      } else {
        ERROR("Invalid file demuxer configuration file, "
            "\"wait_count\" doesn't exist!");
        break;
      }
      if (AM_LIKELY(demuxer["file_empty_timeout"].exists())) {
        m_demuxer_config->file_empty_timeout =
            demuxer["file_empty_timeout"].get<unsigned>(100000);
      } else {
        ERROR("Invalid file demuxer configuration file, "
            "\"file_empty_timeout\" doesn't exist!");
        break;
      }
      if (AM_LIKELY(demuxer["packet_empty_timeout"].exists())) {
        m_demuxer_config->packet_empty_timeout =
            demuxer["packet_empty_timeout"].get<unsigned>(200000);
      } else {
        ERROR("Invalid file demuxer configuration file, "
            "\"packet_empty_timeout\" doesn't exist!");
        break;
      }
      config = m_demuxer_config;
    } else {
      ERROR("Failed to load configuration file: %s", conf_file.c_str());
    }
  }while(0);

  return config;
}
