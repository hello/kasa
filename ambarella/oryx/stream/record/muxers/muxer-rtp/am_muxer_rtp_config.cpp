/*******************************************************************************
 * am_muxer_rtp_config.cpp
 *
 * History:
 *   2015-1-9 - [ypchang] created file
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

#include "am_muxer_rtp_config.h"
#include "am_configure.h"

AMMuxerRtpConfig::AMMuxerRtpConfig() :
  m_muxer_config(nullptr),
  m_config(nullptr)
{}

AMMuxerRtpConfig::~AMMuxerRtpConfig()
{
  delete m_muxer_config;
  delete m_config;
}

MuxerRtpConfig* AMMuxerRtpConfig::get_config(const std::string& conf)
{
  MuxerRtpConfig *config = nullptr;

  do {
    if (AM_LIKELY(!m_muxer_config)) {
      m_muxer_config = new MuxerRtpConfig();
    }
    if (AM_UNLIKELY(!m_muxer_config)) {
      ERROR("Failed to create MuxerRtpConfig object!");
      break;
    }

    delete m_config;
    m_config = AMConfig::create(conf.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig &rtp = *m_config;
      if (AM_LIKELY(rtp["max_send_queue_size"].exists())) {
        m_muxer_config->max_send_queue_size =
            rtp["max_send_queue_size"].get<unsigned>(10);
      } else {
        NOTICE("\"max_send_queue_size\" is not specified, use default!");
        m_muxer_config->max_send_queue_size = 10;
      }

      if (AM_LIKELY(rtp["max_tcp_payload_size"].exists())) {
        m_muxer_config->max_tcp_payload_size =
            rtp["max_tcp_payload_size"].get<unsigned>(1448);
      } else {
        NOTICE("\"max_tcp_payload_size\" is not specified, use default!");
        m_muxer_config->max_tcp_payload_size = 1448;
      }

      if (AM_LIKELY(rtp["max_proto_client"].exists())) {
        m_muxer_config->max_proto_client =
            rtp["max_proto_client"].get<unsigned>(2);
      } else {
        NOTICE("\"max_proto_client\" is not specified, use default!");
        m_muxer_config->max_proto_client = 2;
      }

      if (AM_LIKELY(rtp["sock_timeout"].exists())) {
        m_muxer_config->sock_timeout =
            rtp["max_proto_client"].get<unsigned>(10);
        if (AM_UNLIKELY(m_muxer_config->sock_timeout > 30)) {
          NOTICE("sock_timeout is too large! reset to 30 seconds!");
          m_muxer_config->sock_timeout = 30;
        }
      } else {
        NOTICE("\"sock_timeout\" is not specified, use default!");
        m_muxer_config->sock_timeout = 10;
      }

      if (AM_LIKELY(rtp["dynamic_audio_pts_diff"].exists())) {
        m_muxer_config->dynamic_audio_pts_diff =
            rtp["dynamic_audio_pts_diff"].get<bool>(false);
      } else {
        NOTICE("\"dynamic_audio_pts_diff\" is not specified, use default!");
        m_muxer_config->dynamic_audio_pts_diff = false;
      }

      if (AM_LIKELY(rtp["ntp_use_hw_timer"].exists())) {
        m_muxer_config->ntp_use_hw_timer =
            rtp["ntp_use_hw_timer"].get<bool>(false);
      } else {
        NOTICE("\"ntp_use_hw_timer\" is not specified, use default!");
        m_muxer_config->ntp_use_hw_timer = false;
      }
      config = m_muxer_config;
    } else {
      ERROR("Failed to load configuration file: %s", conf.c_str());
    }
  }while(0);

  return config;
}
