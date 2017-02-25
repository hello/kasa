/*******************************************************************************
 * am_video_source_config.cpp
 *
 * History:
 *   2014-12-11 - [ypchang] created file
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
