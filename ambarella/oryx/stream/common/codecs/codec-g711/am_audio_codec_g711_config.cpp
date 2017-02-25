/*******************************************************************************
 * am_audio_codec_g711_config.cpp
 *
 * History:
 *   2015-1-28 - [ypchang] created file
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

#include "am_audio_codec_g711_config.h"
#include "am_configure.h"

AMAudioCodecG711Config::AMAudioCodecG711Config() :
  m_config(NULL),
  m_g711_config(NULL)
{}

AMAudioCodecG711Config::~AMAudioCodecG711Config()
{
  delete m_config;
  delete m_g711_config;
}

AudioCodecG711Config* AMAudioCodecG711Config::get_config(
    const std::string& conf_file)
{
  AudioCodecG711Config *config = NULL;
  do {
    if (AM_LIKELY(!m_g711_config)) {
      m_g711_config = new AudioCodecG711Config();
    }
    if (AM_UNLIKELY(!m_g711_config)) {
      ERROR("Failed to create AudioCodecG711Config object!");
      break;
    }
    delete m_config;
    m_config = AMConfig::create(conf_file.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig &g711 = *m_config;

      if (AM_LIKELY(g711["encode_law"].exists())) {
        std::string encode_law = g711["encode_law"].get<std::string>("alaw");
        if (is_str_equal(encode_law.c_str(), "alaw")) {
          m_g711_config->encode_law = AM_SAMPLE_ALAW;
        } else if (is_str_equal(encode_law.c_str(), "ulaw")) {
          m_g711_config->encode_law = AM_SAMPLE_ULAW;
        } else {
          NOTICE("Unsupported decoding law: %s, use default!",
                 encode_law.c_str());
          m_g711_config->encode_law = AM_SAMPLE_ALAW;
        }
      } else {
        NOTICE("\"encode_law\" is not specified, use default!");
        m_g711_config->encode_law = AM_SAMPLE_ALAW;
      }


      if (AM_LIKELY(g711["encode_rate"].exists())) {
        m_g711_config->encode_rate =
            g711["encode_rate"].get<unsigned>(16000);
      } else {
        NOTICE("\"encode_rate\" is not specified, use default!");
        m_g711_config->encode_rate = 16000;
      }

      if (AM_LIKELY(g711["encode_frame_time_length"].exists())) {
        m_g711_config->encode_frame_time_length =
            g711["encode_frame_time_length"].get<unsigned>(80);
      } else {
        NOTICE("\"encode_frame_time_length\" is not specified, use default!");
        m_g711_config->encode_frame_time_length = 80;
      }

    } else {
      ERROR("Failed to load configuration file: %s!", conf_file.c_str());
      break;
    }
    config = m_g711_config;
  } while(0);

  return config;
}
