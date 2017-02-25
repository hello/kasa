/*******************************************************************************
 * am_audio_codec_speex_config.cpp
 *
 * History:
 *   Jul 24, 2015 - [ypchang] created file
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

#include "am_audio_codec_speex_config.h"
#include "am_configure.h"

AMAudioCodecSpeexConfig::AMAudioCodecSpeexConfig() :
  m_config(nullptr),
  m_speex_config(nullptr)
{}

AMAudioCodecSpeexConfig::~AMAudioCodecSpeexConfig()
{
  delete m_config;
  delete m_speex_config;
}

AudioCodecSpeexConfig* AMAudioCodecSpeexConfig::get_config(
    const std::string &conf)
{
  AudioCodecSpeexConfig *config = nullptr;
  do {
    if (AM_LIKELY(nullptr == m_speex_config)) {
      m_speex_config = new AudioCodecSpeexConfig();
    }
    if (AM_UNLIKELY(!m_speex_config)) {
      ERROR("Failed to create AudioCodecSpeexConfig object!");
      break;
    }
    delete m_config;
    m_config = AMConfig::create(conf.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig &spx = *m_config;

      /* Encoder settings */
      if (AM_LIKELY(!spx["encode"].exists())) {
        ERROR("Invalid speex configuration! Encoder setting not found!");
        break;
      }

      /* sample_rate*/
      if (AM_LIKELY(spx["encode"]["sample_rate"].exists())) {
        uint32_t &sample_rate = m_speex_config->encode.sample_rate;
        sample_rate = spx["encode"]["sample_rate"].get<unsigned>(16000);
        if (AM_UNLIKELY((8000  != sample_rate) &&
                        (16000 != sample_rate) &&
                        (48000 != sample_rate))) {
          NOTICE("Invalid sample rate %u for encoding speex, reset to 16000!",
                 sample_rate);
          sample_rate = 16000;
        }
      } else {
        NOTICE("\"sample_rate\" is not specified! Use default!");
        m_speex_config->encode.sample_rate = 16000;
      }

      /* enc_frame_time_length*/
      if (AM_LIKELY(spx["encode"]["enc_frame_time_length"].exists())) {
        uint32_t &time_len = m_speex_config->encode.enc_frame_time_length;
        time_len = spx["encode"]["enc_frame_time_length"].get<unsigned>(40);
        if (AM_UNLIKELY(!((0 == time_len%20) && (time_len <= 120)))) {
          NOTICE("Invalid encode time length %u for encoding speex, "
                 "reset to 40!", time_len);
          time_len = 40;
        }
      } else {
        NOTICE("\"enc_frame_time_length\" is not specified! Use default!");
        m_speex_config->encode.enc_frame_time_length = 40;
      }
      /* spx_complexity */
      if (AM_LIKELY(spx["encode"]["spx_complexity"].exists())) {
        uint32_t &spx_complexity = m_speex_config->encode.spx_complexity;
        spx_complexity = spx["encode"]["spx_complexity"].get<unsigned>(1);
        if (AM_UNLIKELY(spx_complexity > 10)) {
          NOTICE("Invalid complexity %u for encoding speex, reset to 1!",
                 spx_complexity);
          spx_complexity = 1;
        }
      } else {
        NOTICE("\"spx_complexity\" is not specified! Use default!");
        m_speex_config->encode.spx_complexity = 1;
      }

      /* spx_avg_bitrate */
      if (AM_LIKELY(spx["encode"]["spx_avg_bitrate"].exists())) {
        m_speex_config->encode.spx_avg_bitrate =
            spx["encode"]["spx_avg_bitrate"].get<unsigned>(32000);
      } else {
        NOTICE("\"spx_avg_bitrate\" is not specified! Use default!");
        m_speex_config->encode.spx_avg_bitrate = 32000;
      }

      /* spx_vbr_max_bitrate */
      if (AM_LIKELY(spx["encode"]["spx_vbr_max_bitrate"].exists())) {
        m_speex_config->encode.spx_vbr_max_bitrate =
            spx["encode"]["spx_vbr_max_bitrate"].get<unsigned>(48000);
      } else {
        NOTICE("\"spx_vbr_max_bitrate\" is not specified! Use default!");
        m_speex_config->encode.spx_vbr_max_bitrate = 48000;
      }


      /* spx_quality */
      if (AM_LIKELY(spx["encode"]["spx_quality"].exists())) {
        uint32_t &spx_quality = m_speex_config->encode.spx_quality;
        spx_quality = spx["encode"]["spx_quality"].get<unsigned>(1);
        if (AM_UNLIKELY(spx_quality > 10)) {
          NOTICE("Invalid frame number %u, reset to 1!", spx_quality);
          spx_quality = 1;
        }
      } else {
        NOTICE("\"spx_quality\" is not specified! Use default!");
        m_speex_config->encode.spx_quality = 1;
      }

      /* spx_vbr */
      if (AM_LIKELY(spx["encode"]["spx_vbr"].exists())) {
        m_speex_config->encode.spx_vbr =
            spx["encode"]["spx_vbr"].get<bool>(true);
      } else {
        NOTICE("\"spx_vbr\" is not specified! Use default!");
        m_speex_config->encode.spx_vbr = true;
      }

      /* spx_vad */
      if (AM_LIKELY(spx["encode"]["spx_vad"].exists())) {
        m_speex_config->encode.spx_vad =
            spx["encode"]["spx_vad"].get<bool>(true);
      } else {
        NOTICE("\"spx_vad\" is not specified! Use default!");
        m_speex_config->encode.spx_vad = true;
      }

      /* spx_dtx */
      if (AM_LIKELY(spx["encode"]["spx_dtx"].exists())) {
        m_speex_config->encode.spx_dtx =
            spx["encode"]["spx_dtx"].get<bool>(false);
      } else {
        NOTICE("\"spx_dtx\" is not specified! Use default!");
        m_speex_config->encode.spx_dtx = false;
      }

      /* spx_highpass */
      if (AM_LIKELY(spx["encode"]["spx_highpass"].exists())) {
        m_speex_config->encode.spx_highpass =
            spx["encode"]["spx_highpass"].get<bool>(false);
      } else {
        NOTICE("\"spx_highpass\" is not specified! Use default!");
        m_speex_config->encode.spx_highpass = false;
      }

      /* Decode settings */
      if (AM_LIKELY(!spx["decode"].exists())) {
        ERROR("Invalid speex configuration! Decoder setting not found!");
        break;
      }

      /* spx_output_channel */
      if (AM_LIKELY(spx["decode"]["spx_output_channel"].exists())) {
        std::string ch = spx["decode"]["spx_output_channel"].
            get<std::string>("none");
        if (is_str_equal(ch.c_str(), "none")) {
          m_speex_config->decode.spx_output_channel = 0;
        } else if (is_str_equal(ch.c_str(), "mono")) {
          m_speex_config->decode.spx_output_channel = 1;
        } else if (is_str_equal(ch.c_str(), "stereo")) {
          m_speex_config->decode.spx_output_channel = 2;
        } else {
          NOTICE("Invalid speex output channel: %s!"
                 "reset to \"none\"", ch.c_str());
          m_speex_config->decode.spx_output_channel = 0;
        }
      } else {
        NOTICE("\"spx_output_channel\" is not specified! Use default!");
        m_speex_config->decode.spx_output_channel = 0;
      }

      /* spx_perceptual_enhance */
      if (AM_LIKELY(spx["decode"]["spx_perceptual_enhance"].exists())) {
        m_speex_config->decode.spx_perceptual_enhance =
            spx["decode"]["spx_perceptual_enhance"].get<bool>(true);
      } else {
        NOTICE("\"spx_perceptual_enhance\" is not specified! Use default!");
        m_speex_config->decode.spx_perceptual_enhance = true;
      }
    }

    config = m_speex_config;
  }while(0);

  return config;
}

