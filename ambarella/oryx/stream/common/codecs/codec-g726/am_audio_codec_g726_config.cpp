/*******************************************************************************
 * am_audio_codec_g726_config.cpp
 *
 * History:
 *   2014-11-12 - [ypchang] created file
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
#include "am_audio_type.h"

#include "am_audio_codec_g726_config.h"
#include "am_configure.h"

AMAudioCodecG726Config::AMAudioCodecG726Config() :
  m_config(NULL),
  m_g726_config(NULL)
{}

AMAudioCodecG726Config::~AMAudioCodecG726Config()
{
  delete m_config;
  delete m_g726_config;
}

AudioCodecG726Config* AMAudioCodecG726Config::get_config(
    const std::string& conf_file)
{
  AudioCodecG726Config *config = NULL;
  do {
    if (AM_LIKELY(!m_g726_config)) {
      m_g726_config = new AudioCodecG726Config();
    }
    if (AM_UNLIKELY(!m_g726_config)) {
      ERROR("Failed to create AudioCodecG726Config object!");
      break;
    }
    delete m_config;
    m_config = AMConfig::create(conf_file.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig &g726 = *m_config;

      if (AM_LIKELY(g726["decode_law"].exists())) {
        std::string decode_law = g726["decode_law"].get<std::string>("pcm16");
        if (is_str_equal(decode_law.c_str(), "pcm16")) {
          m_g726_config->decode_law = AM_SAMPLE_S16LE;
        } else if (is_str_equal(decode_law.c_str(), "alaw")) {
          m_g726_config->decode_law = AM_SAMPLE_ALAW;
        } else if (is_str_equal(decode_law.c_str(), "ulaw")) {
          m_g726_config->decode_law = AM_SAMPLE_ULAW;
        } else {
          NOTICE("Unsupported decoding law: %s, use default!",
                 decode_law.c_str());
          m_g726_config->decode_law = AM_SAMPLE_S16LE;
        }
      } else {
        NOTICE("\"decode_law\" is not specified, use default!");
        m_g726_config->decode_law = AM_SAMPLE_S16LE;
      }

      if (AM_LIKELY(g726["encode_rate"].exists())) {
        m_g726_config->encode_rate =
            G726_RATE(g726["encode_rate"].get<unsigned>(32));
        switch(m_g726_config->encode_rate) {
          case G726_16K:
          case G726_24K:
          case G726_32K:
          case G726_40K:break;
          default: {
            NOTICE("Invalid encode rate %u, reset to default(32)!",
                   m_g726_config->encode_rate);
            m_g726_config->encode_rate = G726_32K;
          }break;
        }
      } else {
        NOTICE("\"encode_rate\" is not specified, use default!");
        m_g726_config->encode_rate = G726_32K;
      }

      if (AM_LIKELY(g726["encode_frame_time_length"].exists())) {
        m_g726_config->encode_frame_time_length =
            g726["encode_frame_time_length"].get<unsigned>(80);
      } else {
        NOTICE("\"encode_frame_time_length\" is not specified, use default!");
        m_g726_config->encode_frame_time_length = 80;
      }

    } else {
      ERROR("Failed to load configuration file: %s!", conf_file.c_str());
      break;
    }
    config = m_g726_config;
  } while(0);

  return config;
}
