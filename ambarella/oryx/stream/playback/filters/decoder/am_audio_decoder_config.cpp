/*******************************************************************************
 * am_audio_decoder_config.cpp
 *
 * History:
 *   2014-9-25 - [ypchang] created file
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

#include "am_audio_decoder_config.h"
#include "am_configure.h"

AMAudioDecoderConfig::AMAudioDecoderConfig() :
  m_config(NULL),
  m_decoder_config(NULL)
{}

AMAudioDecoderConfig::~AMAudioDecoderConfig()
{
  delete m_config;
  delete m_decoder_config;
}

AudioDecoderConfig* AMAudioDecoderConfig::get_config(const std::string& conf)
{
  AudioDecoderConfig *config = NULL;

  do {
    if (AM_LIKELY(NULL == m_decoder_config)) {
      m_decoder_config = new AudioDecoderConfig();
    }
    if (AM_UNLIKELY(!m_decoder_config)) {
      ERROR("Failed to create AudioDecoderConfig object!");
      break;
    }
    delete m_config;
    m_config = AMConfig::create(conf.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig& decoder = *m_config;
      if (AM_LIKELY(decoder["name"].exists())) {
        m_decoder_config->name =
            decoder["name"].get<std::string>("AudioDecoder");
      } else {
        NOTICE("\"name\" is not specified for this filter, use default!");
        m_decoder_config->name = "AudioDecoder";
      }

      if (AM_LIKELY(decoder["rt_config"].exists())) {
        m_decoder_config->real_time.enabled =
            decoder["rt_config"]["enabled"].get<bool>(false);
        m_decoder_config->real_time.priority =
            decoder["rt_config"]["priority"].get<unsigned>(10);
      } else {
        NOTICE("Thread priority configuration is not found, "
               "use default value(Realtime thread: disabled)!");
        m_decoder_config->real_time.enabled = false;
        m_decoder_config->real_time.priority = 10;
      }
      if (AM_LIKELY(decoder["packet_pool_size"].exists())) {
        m_decoder_config->packet_pool_size =
            decoder["packet_pool_size"].get<unsigned>(4);
      } else {
        ERROR("Invalid file demuxer configuration file, "
              "\"packet_pool_size\" doesn't exist!");
        break;
      }
      if (AM_LIKELY(decoder["decode_error_threshold"].exists())) {
        m_decoder_config->decode_error_threshold =
            decoder["decode_error_threshold"].get<unsigned>(30);
      } else {
        ERROR("\"decode_error_threshold\" is not specified, use default!");
        m_decoder_config->decode_error_threshold = 30;
      }

      config = m_decoder_config;
    } else {
      ERROR("Failed to load configuration file: %s", conf.c_str());
    }
  }while(0);

  return config;
}
