/*******************************************************************************
 * am_sip_ua_server_config.cpp
 *
 * History:
 *   2015-1-28 - [Shiming Dong] created file
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

#include "am_configure.h"
#include "am_sip_ua_server_config.h"

AMSipUAServerConfig::AMSipUAServerConfig() :
  m_sip_config(nullptr),
  m_config(nullptr)
{

}

AMSipUAServerConfig::~AMSipUAServerConfig()
{
  delete m_sip_config;
  delete m_config;
}

SipUAServerConfig* AMSipUAServerConfig::get_config(const std::string &conf)
{
  SipUAServerConfig *config = nullptr;

  do {
    if (AM_UNLIKELY(!m_sip_config)) {
      m_sip_config = new SipUAServerConfig();
    }
    if (AM_UNLIKELY(!m_sip_config)) {
      ERROR("Failed to create SipUAServerConfig object!");
      break;
    }

    delete m_config;
    m_config = AMConfig::create(conf.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig &sip = *m_config;
      if (AM_LIKELY(sip["sip_unix_domain_name"].exists())) {
        m_sip_config->sip_unix_domain_name =
            sip["sip_unix_domain_name"].\
            get<std::string>("/run/oryx/sip-ua-server.socket");
      } else {
        NOTICE("\"sip_unix_domain_name\" is not specified, use default");
        m_sip_config->sip_unix_domain_name = "/run/oryx/sip-ua-server.socket";
      }

      if (AM_LIKELY(sip["sip_to_media_service_name"].exists())) {
        m_sip_config->sip_to_media_service_name =
            sip["sip_to_media_service_name"].\
            get<std::string>("/run/oryx/sip-to-media-service.socket");
      } else {
        NOTICE("\"sip_to_media_service_name\" is not specified, use default");
        m_sip_config->sip_to_media_service_name =
            "/run/oryx/sip-to-media-service.socket";
      }

      if (AM_LIKELY(sip["sip_retry_interval"].exists())) {
        m_sip_config->sip_retry_interval =
            sip["sip_retry_interval"].get<unsigned>(1000);
      } else {
        NOTICE("\"sip_retry_interval\" is not specified, use default!");
        m_sip_config->sip_retry_interval = 1000;
      }

      if (AM_LIKELY(sip["sip_send_retry_count"].exists())) {
        m_sip_config->sip_send_retry_count =
            sip["sip_send_retry_count"].get<unsigned>(5);
      } else {
        NOTICE("\"sip_send_retry_count\" is not specified, use default!");
        m_sip_config->sip_send_retry_count = 5;
      }

      if (AM_LIKELY(sip["sip_send_retry_interval"].exists())) {
        m_sip_config->sip_send_retry_interval =
            sip["sip_send_retry_interval"].get<unsigned>(100);
      } else {
        NOTICE("\"sip_send_retry_interval\" is not specified, use default!");
        m_sip_config->sip_send_retry_interval = 100;
      }

      if (AM_LIKELY(sip["username"].exists())) {
        m_sip_config->username =
            sip["username"].get<std::string>("1001");
      } else {
        NOTICE("\"username\" is not specified, use default!");
        m_sip_config->username = "1001";
      }

      if (AM_LIKELY(sip["password"].exists())) {
        m_sip_config->password =
            sip["password"].get<std::string>("1001");
      } else {
        NOTICE("\"password\" is not specified, use default!");
        m_sip_config->password = "1001";
      }

      if (AM_LIKELY(sip["server_url"].exists())) {
        m_sip_config->server_url =
            sip["server_url"].get<std::string>("10.0.0.3");
      } else {
        NOTICE("\"server_url\" is not specified, use default!");
        m_sip_config->server_url = "10.0.0.3";
      }

      if (AM_LIKELY(sip["server_port"].exists())) {
        m_sip_config->server_port =
            sip["server_port"].get<unsigned>(5060);
      } else {
        NOTICE("\"server_port\" is not specified, use default!");
        m_sip_config->server_port = 5060;
      }

      if (AM_LIKELY(sip["sip_port"].exists())) {
        m_sip_config->sip_port =
            sip["sip_port"].get<unsigned>(5060);
      } else {
        NOTICE("\"sip_port\" is not specified, use default!");
        m_sip_config->sip_port = 5060;
      }

      if (AM_LIKELY(sip["expires"].exists())) {
        m_sip_config->expires =
            sip["expires"].get<int32_t>(1800);
      } else {
        NOTICE("\"expires\" is not specified, use default!");
        m_sip_config->expires = 1800;
      }

      if (AM_LIKELY(sip["is_register"].exists())) {
        m_sip_config->is_register =
            sip["is_register"].get<bool>(true);
      } else {
        NOTICE("\"is_register\" is not specified, use default!");
        m_sip_config->is_register = true;
      }

      if (AM_LIKELY(sip["rtp_stream_port_base"].exists())) {
        m_sip_config->rtp_stream_port_base =
            sip["rtp_stream_port_base"].get<unsigned>(5004);
      } else {
        NOTICE("\"rtp_stream_port_base\" is not specified, use default!");
        m_sip_config->rtp_stream_port_base = 5004;
      }

      if (AM_LIKELY(sip["max_conn_num"].exists())) {
        m_sip_config->max_conn_num =
            sip["max_conn_num"].get<unsigned>(1);
      } else {
        NOTICE("\"max_conn_num\" is not specified, use default!");
        m_sip_config->max_conn_num = 1;
      }

      if (AM_LIKELY(sip["enable_video"].exists())) {
        m_sip_config->enable_video =
            sip["enable_video"].get<bool>(false);
      } else {
        NOTICE("\"enable_video\" is not specified, use default!");
        m_sip_config->enable_video = false;
      }

      if (AM_LIKELY(sip["audio_priority_list"].exists())) {
        std::string media_type;
        m_sip_config->audio_priority_list.clear();
        for (uint32_t i = 0; i < sip["audio_priority_list"].length(); ++i) {
          media_type = sip["audio_priority_list"][i].get<std::string>("PCMU");
          m_sip_config->audio_priority_list.push_back(media_type);
        }
      } else {
        ERROR("\"audio_priority_list\" is not specified!");
        break;
      }

      if (AM_LIKELY(sip["video_priority_list"].exists())) {
        std::string media_type;
        m_sip_config->video_priority_list.clear();
        for (uint32_t i = 0; i < sip["video_priority_list"].length(); ++i) {
          media_type = sip["video_priority_list"][i].get<std::string>("H264");
          m_sip_config->video_priority_list.push_back(media_type);
        }
      } else {
        ERROR("\"video_priority_list\" is not specified!");
        break;
      }

      config = m_sip_config;
    } else {
      ERROR("Failed to load configuration file: %s", conf.c_str());
    }
  } while(0);

  return config;
}
