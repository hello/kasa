/*******************************************************************************
 * am_rtp_client_config.cpp
 *
 * History:
 *   2015-1-6 - [ypchang] created file
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

#include "am_rtp_client_config.h"
#include "am_configure.h"

AMRtpClientConfig::AMRtpClientConfig() :
  m_client_config(NULL),
  m_config(NULL)
{}

AMRtpClientConfig::~AMRtpClientConfig()
{
  delete m_client_config;
  delete m_config;
}

RtpClientConfig* AMRtpClientConfig::get_config(const std::string& conf)
{
  RtpClientConfig *config = NULL;

  do {
    if (AM_LIKELY(!m_client_config)) {
      m_client_config = new RtpClientConfig();
    }
    if (AM_UNLIKELY(!m_client_config)) {
      ERROR("Failed to create RtpClientConfig object!");
      break;
    }

    delete m_config;
    m_config = AMConfig::create(conf.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig &rtp = *m_config;
      if (AM_LIKELY(rtp["client_timeout"].exists())) {
        uint32_t client_timeout = rtp["client_timeout"].get<unsigned>(60);
        if (AM_UNLIKELY((client_timeout < 10) || (client_timeout > 100))) {
          WARN("Invalid client timeout value: %u seconds, reset to 60 seconds!",
               client_timeout);
          client_timeout = 60;
        }
        m_client_config->client_timeout = client_timeout;
      } else {
        NOTICE("\"client_timeout\" is not specified, use default!");
        m_client_config->client_timeout = 30;
      }

      if (AM_LIKELY(rtp["client_timeout_count"].exists())) {
        uint32_t client_timeout_count =
            rtp["client_timeout_count"].get<unsigned>(10);
        if (AM_UNLIKELY((client_timeout_count == 0) ||
                        (client_timeout_count > 20))) {
          WARN("Invalid client timeout count value: %u seconds, "
               "reset to 10 times!",
               client_timeout_count);
          client_timeout_count = 10;
        }
        m_client_config->client_timeout_count = client_timeout_count;
      } else {
        NOTICE("\"client_timeout_count\" is not specified, use default!");
        m_client_config->client_timeout_count = 10;
      }

      if (AM_LIKELY(rtp["wait_interval"].exists())) {
        uint32_t wait_interval = rtp["wait_interval"].get<unsigned>(100);
        if (AM_UNLIKELY((wait_interval > 1000) || (wait_interval < 1))) {
          WARN("Invalid wait interval %u ms, reset to 100ms", wait_interval);
          wait_interval = 100;
        }
        m_client_config->wait_interval = wait_interval;
      } else {
        NOTICE("\"wait_interval\" is not specified, use default!");
        m_client_config->wait_interval = 100;
      }

      if (AM_LIKELY(rtp["rtcp_sr_interval"].exists())) {
        uint32_t rtcp_sr_interval = rtp["rtcp_sr_interval"].get<unsigned>(5);
        if (AM_UNLIKELY((rtcp_sr_interval < 3) || (rtcp_sr_interval > 20))) {
          WARN("Invalid RTCP SR interval %u seconds, reset to 5 seconds",
               rtcp_sr_interval);
          rtcp_sr_interval = 5;
        }
        m_client_config->rtcp_sr_interval = rtcp_sr_interval;
      } else {
        NOTICE("\"rtcp_sr_interval\" is not specified, use default!");
        m_client_config->rtcp_sr_interval = 5;
      }

      if (AM_LIKELY(rtp["resend_count"].exists())) {
        uint32_t resend_count = rtp["resend_count"].get<unsigned>(30);
        if (AM_UNLIKELY((resend_count > 100) || (resend_count < 5))) {
          WARN("Invalid re-sending count: %u, reset to 30", resend_count);
          resend_count = 30;
        }
        m_client_config->resend_count = resend_count;
      } else {
        NOTICE("\"resend_count\" is not specified, use default!");
        m_client_config->resend_count = 30;
      }

      if (AM_LIKELY(rtp["resend_interval"].exists())) {
        uint32_t resend_interval = rtp["resend_interval"].get<unsigned>(5);
        if (AM_UNLIKELY((resend_interval > 20) || (resend_interval < 5))) {
          WARN("Invalid re-sending interval: %u MS, reset to 5MS",
               resend_interval);
          resend_interval = 5;
        }
        m_client_config->resend_interval = resend_interval;
      } else {
        NOTICE("\"resend_interval\" is not specified, use default!");
        m_client_config->resend_interval = 5;
      }

      if (AM_LIKELY(rtp["statistics_count"].exists())) {
        m_client_config->statistics_count =
            rtp["statistics_count"].get<unsigned>(20000);
      } else {
        NOTICE("\"statistics_count\" is not specified, use default!");
        m_client_config->statistics_count = 20000;
      }

      if (AM_LIKELY(rtp["send_buffer_size"].exists())) {
        uint32_t size = rtp["send_buffer_size"].get<unsigned>(8);
        if (AM_UNLIKELY((size > 1024) || (size < 160))) {
          WARN("Invalid buffer size %uKB, reset to 512KB!", size);
          size = 512;
        }
        m_client_config->send_buffer_size = size * 1024;
      } else {
        NOTICE("\"send_buffer_size\" is not specified, use default!");
        m_client_config->send_buffer_size = 512 * 1024;
      }

      if (AM_LIKELY(rtp["blocked"].exists())) {
        m_client_config->blocked = rtp["blocked"].get<bool>(false);
      } else {
        NOTICE("\"blocked\" is not specified, use default!");
        m_client_config->blocked = false;
      }

      if (AM_LIKELY(rtp["block_timeout"].exists())) {
        uint32_t timeout = rtp["block_timeout"].get<unsigned>(10);
        if (AM_UNLIKELY((timeout < 3) || (timeout > 30))) {
          WARN("Timeout value is too strange, reset to 10 seconds!", timeout);
          timeout = 10;
        }
        m_client_config->block_timeout = timeout;
      }

      if (AM_LIKELY(rtp["report_block_cnt"].exists())) {
        uint8_t count = rtp["report_block_cnt"].get<unsigned>(4);
        if (AM_UNLIKELY(count >31)) {
          WARN("Invalid report block count, reset to max value 31");
          count = 31;
        }
        m_client_config->report_block_cnt = count;
      } else {
        NOTICE("\"report_block_cnt\" is not specified, use default!");
        m_client_config->report_block_cnt = 4;
      }

      if (AM_LIKELY(rtp["print_rtcp_stat"].exists())) {
        m_client_config->print_rtcp_stat =
            rtp["print_rtcp_stat"].get<bool>(true);
      } else {
        NOTICE("\"print_rtcp_stat\" is not specified, use default!");
        m_client_config->print_rtcp_stat = true;
      }

      if (AM_LIKELY(rtp["rtcp_incremental_ts"].exists())) {
        m_client_config->rtcp_incremental_ts =
            rtp["rtcp_incremental_ts"].get<bool>(true);
      } else {
        NOTICE("\"rtcp_incremental_ts\" is not specified, use default!");
        m_client_config->rtcp_incremental_ts = true;
      }

      config = m_client_config;
    } else {
      ERROR("Failed to load configuration file: %s", conf.c_str());
    }
  }while(0);

  return config;
}
