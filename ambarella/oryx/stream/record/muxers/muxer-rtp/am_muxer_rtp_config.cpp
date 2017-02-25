/*******************************************************************************
 * am_muxer_rtp_config.cpp
 *
 * History:
 *   2015-1-9 - [ypchang] created file
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

      if (AM_LIKELY(rtp["muxer_id"].exists())) {
        m_muxer_config->muxer_id = rtp["muxer_id"].get<unsigned>(0);
      } else {
        NOTICE("\"muxer_id\" is not specified, use default!");
        m_muxer_config->muxer_id = 0;
      }

      if (AM_LIKELY(rtp["max_alloc_wait_count"].exists())) {
        m_muxer_config->max_alloc_wait_count =
            rtp["max_alloc_wait_count"].get<unsigned>(20);
      } else {
        NOTICE("\"max_alloc_wait_count\" is not specified, use default!");
        m_muxer_config->max_alloc_wait_count = 20;
      }

      if (AM_LIKELY(rtp["alloc_wait_interval"].exists())) {
        m_muxer_config->alloc_wait_interval =
            rtp["alloc_wait_interval"].get<unsigned>(10);
        if (AM_UNLIKELY(m_muxer_config->alloc_wait_interval > 100)) {
          WARN("Wait too long to check packet queue! Reset to 10 MS");
          m_muxer_config->alloc_wait_interval = 10;
        }
      } else {
        NOTICE("\"alloc_wait_interval\" is not specified, use default!");
        m_muxer_config->alloc_wait_interval = 10;
      }

      if (AM_LIKELY(rtp["kill_session_timeout"].exists())) {
        m_muxer_config->kill_session_timeout =
            rtp["kill_session_timeout"].get<unsigned>(4000);
        if (AM_UNLIKELY(m_muxer_config->kill_session_timeout > 5000)) {
          WARN("Session wait too long to be killed! Reset to 4000 MS");
          m_muxer_config->kill_session_timeout = 4000;
        }
      } else {
        NOTICE("\"kill_session_timeout\" is not specified, use default!");
        m_muxer_config->kill_session_timeout = 4000;
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

      if (AM_LIKELY(rtp["del_buffer_on_release"].exists())) {
        m_muxer_config->del_buffer_on_release =
            rtp["del_buffer_on_release"].get<bool>(false);
      } else {
        NOTICE("\"del_buffer_on_release\" is not specified, use default!");
        m_muxer_config->del_buffer_on_release = false;
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

      if (AM_LIKELY(rtp["session_type"].exists())) {
        uint32_t size = rtp["session_type"].length();
        for (uint32_t i = 0; i < size; ++ i) {
          int32_t type = rtp["session_type"][i]["type"].get<int>(-1);
          if (AM_LIKELY(type >= 0)) {
            RtpSessionType session_type;
            session_type.codec = rtp["session_type"][i]["codec"].
                                 get<std::string>();
            session_type.name  = rtp["session_type"][i]["name"].
                                 get<std::string>();
            m_muxer_config->rtp_session_map[type] = session_type;
          }
        }
      } else {
        ERROR("\"session_type\" is not specified!");
        break;
      }
      config = m_muxer_config;
    } else {
      ERROR("Failed to load configuration file: %s", conf.c_str());
    }
  }while(0);

  return config;
}
