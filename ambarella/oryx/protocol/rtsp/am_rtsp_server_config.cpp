/*******************************************************************************
 * am_rtsp_config.cpp
 *
 * History:
 *   2015-1-20 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
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

#include "am_rtsp_server_config.h"
#include "am_configure.h"

#define RTSP_DEFAULT_USER_DB ((const char*)"/etc/webpass.txt")

AMRtspServerConfig::AMRtspServerConfig() :
  m_rtsp_config(nullptr),
  m_config(nullptr)
{}

AMRtspServerConfig::~AMRtspServerConfig()
{
  delete m_rtsp_config;
  delete m_config;
}

RtspServerConfig* AMRtspServerConfig::get_config(const std::string &conf)
{
  RtspServerConfig *config = nullptr;

  do {
    if (AM_UNLIKELY(!m_rtsp_config)) {
      m_rtsp_config = new RtspServerConfig();
    }
    if (AM_UNLIKELY(!m_rtsp_config)) {
      ERROR("Failed to create RtspServerConfig object!");
      break;
    }

    delete m_config;
    m_config = AMConfig::create(conf.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig &rtsp = *m_config;

      if (AM_LIKELY(rtsp["rtsp_server_name"].exists())) {
        m_rtsp_config->rtsp_server_name = rtsp["rtsp_server_name"].
            get<std::string>("Ambarella Oryx RTSP Server");
      } else {
        NOTICE("\"rtsp_server_name\" is not specified, use default!");
        m_rtsp_config->rtsp_server_name = "Ambarella Oryx RTSP Server";
      }

      if (AM_LIKELY(rtsp["rtsp_unix_domain_name"].exists())) {
        m_rtsp_config->rtsp_unix_domain_name =
            rtsp["rtsp_unix_domain_name"].\
            get<std::string>("rtsp-server.socket");
      } else {
        NOTICE("\"rtsp_unix_domain_name\" is not specified, use default!");
        m_rtsp_config->rtsp_unix_domain_name = "rtsp-server.socket";
      }

      if (AM_LIKELY(rtsp["rtsp_server_user_db"].exists())) {
        m_rtsp_config->rtsp_server_user_db =
            rtsp["rtsp_server_user_db"].\
            get<std::string>(RTSP_DEFAULT_USER_DB);
      } else {
        NOTICE("\"rtsp_server_user_db\" is not specified, use default!");
        m_rtsp_config->rtsp_server_user_db = RTSP_DEFAULT_USER_DB;
      }

      if (AM_LIKELY(rtsp["rtsp_retry_interval"].exists())) {
        m_rtsp_config->rtsp_retry_interval =
            rtsp["rtsp_retry_interval"].get<unsigned>(1000);
      } else {
        NOTICE("\"rtsp_retry_interval\" is not specified, use default!");
        m_rtsp_config->rtsp_retry_interval = 1000;
      }

      if (AM_LIKELY(rtsp["rtsp_server_port"].exists())) {
        m_rtsp_config->rtsp_server_port =
            rtsp["rtsp_server_port"].get<unsigned>(554);
      } else {
        NOTICE("\"rtsp_server_port\" is not specified, use default!");
        m_rtsp_config->rtsp_server_port = 554;
      }

      if (AM_LIKELY(rtsp["rtsp_send_retry_count"].exists())) {
        m_rtsp_config->rtsp_send_retry_count =
            rtsp["rtsp_send_retry_count"].get<unsigned>(5);
      } else {
        NOTICE("\"rtsp_send_retry_count\" is not specified, use default!");
        m_rtsp_config->rtsp_send_retry_count = 5;
      }

      if (AM_LIKELY(rtsp["rtsp_send_retry_interval"].exists())) {
        m_rtsp_config->rtsp_send_retry_interval =
            rtsp["rtsp_send_retry_interval"].get<unsigned>(100);
      } else {
        NOTICE("\"rtsp_send_retry_interval\" is not specified, use default!");
        m_rtsp_config->rtsp_send_retry_interval = 100;
      }

      if (AM_LIKELY(rtsp["rtp_stream_port_base"].exists())) {
        m_rtsp_config->rtp_stream_port_base =
            rtsp["rtp_stream_port_base"].get<unsigned>(50000);
      } else {
        NOTICE("\"rtp_stream_port_base\" is not specified, use default!");
        m_rtsp_config->rtp_stream_port_base = 50000;
      }

      if (AM_LIKELY(rtsp["need_auth"].exists())) {
        m_rtsp_config->need_auth = rtsp["need_auth"].get<bool>(false);
      } else {
        NOTICE("\"need_auth\" is not specified, use default!");
        m_rtsp_config->need_auth = false;
      }
      if (AM_LIKELY(rtsp["max_client_num"].exists())) {
        m_rtsp_config->max_client_num = rtsp["max_client_num"].get<unsigned>(5);
      } else {
        NOTICE("\"max_client_num\" is not specified, use default!");
        m_rtsp_config->max_client_num = 5;
      }
      if (AM_LIKELY(rtsp["stream"].exists())) {
        for (uint32_t i = 0; i < rtsp["stream"].length(); ++ i) {
          RtspServerStreamDefine stream;
          std::string def_video = "video" + std::to_string(i+1);
          stream.video = rtsp["stream"][i]["video"].get<std::string>(def_video);
          stream.audio = rtsp["stream"][i]["audio"].get<std::string>("aac");
          m_rtsp_config->stream.push_back(stream);
        }
      } else {
        ERROR("stream definition is not specified!");
        break;
      }
      config = m_rtsp_config;
    } else {
      ERROR("Failed to load configuration file: %s", conf.c_str());
    }
  }while(0);

  return config;
}
