/*******************************************************************************
 * am_file_demuxer_config.cpp
 *
 * History:
 *   2014-9-5 - [ypchang] created file
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
