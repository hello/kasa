/*******************************************************************************
 * am_audio_codec_g711.cpp
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

#include "am_amf_types.h"

#include "am_audio_codec_if.h"
#include "am_audio_codec.h"
#include "am_audio_codec_g711.h"
#include "am_audio_codec_g711_config.h"

#include "g711.h"

AMIAudioCodec* get_audio_codec(const char *config)
{
  return AMAudioCodecG711::create(config);
}

AMIAudioCodec* AMAudioCodecG711::create(const char *config)
{
  AMAudioCodecG711 *g711 = new AMAudioCodecG711();
  if (AM_LIKELY(g711 && !g711->init(config))) {
    delete g711;
    g711 = nullptr;
  }

  return g711;
}

void AMAudioCodecG711::destroy()
{
  inherited::destroy();
}

bool AMAudioCodecG711::initialize(AM_AUDIO_INFO *srcAudioInfo,
                                  AM_AUDIO_CODEC_MODE mode)
{
  if (AM_LIKELY(srcAudioInfo && (!m_is_initialized ||
      (m_src_audio_info->sample_format != srcAudioInfo->sample_format)))) {
    memcpy(m_src_audio_info, srcAudioInfo, sizeof(*m_src_audio_info));
    memcpy(&m_audio_info[AUDIO_INFO_ENCODE],
           srcAudioInfo,
           sizeof(*srcAudioInfo));
    memcpy(&m_audio_info[AUDIO_INFO_DECODE],
           srcAudioInfo,
           sizeof(*srcAudioInfo));
    if (AM_UNLIKELY(m_is_initialized)) {
      NOTICE("Codec %s is already initialized to %s mode, re-initializing!",
             m_name.c_str(),
             codec_mode_string().c_str());
      finalize();
    }
    m_mode = mode;
    switch (m_mode) {
      case AM_AUDIO_CODEC_MODE_ENCODE: {
        AM_AUDIO_INFO &audioInfo = m_audio_info[AUDIO_INFO_ENCODE];
        if (AM_LIKELY((audioInfo.channels == 1) &&
                      (audioInfo.sample_format == AM_SAMPLE_S16LE))) {
          int g711_mode = G711_ALAW;
          switch(m_g711_config->encode_law) {
            case AM_SAMPLE_ALAW: {
              g711_mode = G711_ALAW;
              audioInfo.type = AM_AUDIO_G711A;
              audioInfo.sample_format = AM_SAMPLE_ALAW;
            }break;
            case AM_SAMPLE_ULAW: {
              g711_mode = G711_ULAW;
              audioInfo.type = AM_AUDIO_G711U;
              audioInfo.sample_format = AM_SAMPLE_ULAW;
            }break;
            default: {
              WARN("Unknown mode: %u, use default!", m_g711_config->encode_law);
              g711_mode = G711_ALAW;
            }break;
          }
          m_g711_state = g711_init(nullptr, g711_mode);
          if (AM_UNLIKELY(!m_g711_state)) {
            ERROR("Failed to initialize G.711 codec for encoding!");
          }
          m_is_initialized = (nullptr != m_g711_state);
        } else {
          if (AM_LIKELY(audioInfo.channels != 1)) {
            ERROR("Invalid channel number! 1 channel is required!");
            break;
          }
          if (AM_LIKELY((audioInfo.sample_format != AM_SAMPLE_S16LE))) {
            ERROR("Invalid input audio type, S16LE are required!");
          }
        }
      }break;
      case AM_AUDIO_CODEC_MODE_DECODE: {
        AM_AUDIO_INFO &audioInfo = m_audio_info[AUDIO_INFO_DECODE];
        if (AM_LIKELY((audioInfo.channels == 1) &&
                      ((audioInfo.type == AM_AUDIO_G711A) ||
                       (audioInfo.type == AM_AUDIO_G711U)))) {
          int32_t g711_mode = -1;
          switch(audioInfo.sample_format) {
            case AM_SAMPLE_ALAW: g711_mode = G711_ALAW; break;
            case AM_SAMPLE_ULAW: g711_mode = G711_ULAW; break;
            default: {
              ERROR("Unknown mode: %u!", audioInfo.sample_format);
            }break;
          }
          /* default decode to S16LE */
          audioInfo.type = AM_AUDIO_LPCM;
          audioInfo.sample_size = sizeof(int16_t);
          audioInfo.sample_format = AM_SAMPLE_S16LE;

          if (AM_LIKELY(g711_mode != -1)) {
            m_g711_state = g711_init(nullptr, g711_mode);
          }
          m_is_initialized = (nullptr != m_g711_state);
          if (AM_UNLIKELY(!m_is_initialized)) {
            ERROR("Failed to initialize G.711 codec for decoding!");
          }
        } else {
          ERROR("Invalid G.711 audio!");
        }
      }break;
      default: {
        ERROR("Invalid mode!");
      }break;
    }
  } else if (AM_LIKELY(!srcAudioInfo)) {
    ERROR("Invalid audio info!");
    finalize();
    m_is_initialized = false;
    memset(m_src_audio_info, 0, sizeof(*m_src_audio_info));
  }

  if (AM_UNLIKELY(!m_is_initialized)) {
    m_mode = AM_AUDIO_CODEC_MODE_NONE;
  }
  NOTICE("Audio codec %s is initialized to mode %s",
         m_name.c_str(), codec_mode_string().c_str());

  return m_is_initialized;
}

bool AMAudioCodecG711::finalize()
{
  if (AM_LIKELY(m_g711_state)) {
    g711_release((g711_state_t*)m_g711_state);
  }
  m_g711_state = nullptr;
  m_is_initialized = false;
  m_mode = AM_AUDIO_CODEC_MODE_NONE;

  return true;
}

AM_AUDIO_INFO* AMAudioCodecG711::get_codec_audio_info()
{
  AM_AUDIO_INFO *info = NULL;
  switch(m_mode) {
    case AM_AUDIO_CODEC_MODE_ENCODE: {
      info = &m_audio_info[AUDIO_INFO_ENCODE];
    }break;
    case AM_AUDIO_CODEC_MODE_DECODE: {
      info = &m_audio_info[AUDIO_INFO_DECODE];
    }break;
    default: ERROR("G.711 audio codec is not initialized!"); break;
  }
  return info;
}
uint32_t AMAudioCodecG711::get_codec_output_size()
{
  uint32_t size = 0;
  switch(m_mode) {
    case AM_AUDIO_CODEC_MODE_ENCODE: {
      uint32_t &chunk_size  = m_audio_info[AUDIO_INFO_ENCODE].chunk_size;
      size = ROUND_UP(chunk_size / 2, 4);
    }break;
    case AM_AUDIO_CODEC_MODE_DECODE: {
      uint32_t &chunk_size  = m_audio_info[AUDIO_INFO_DECODE].chunk_size;
      size = ROUND_UP(chunk_size * 2, 4);
    }break;
    default: ERROR("G.711 audio codec is not initialized!"); break;
  }

  return size;
}

bool AMAudioCodecG711::get_encode_required_src_parameter(AM_AUDIO_INFO &info)
{
  bool ret = true;
  memset(&info, 0, sizeof(info));
  info.channels = 1;
  info.sample_format = AM_SAMPLE_S16LE;
  info.sample_rate = m_g711_config->encode_rate;
  info.chunk_size = m_g711_config->encode_frame_time_length * info.sample_rate /
      1000 * info.channels * sizeof(int16_t);
  return ret;
}

uint32_t AMAudioCodecG711::encode(uint8_t *input,
                                  uint32_t in_data_size,
                                  uint8_t *output,
                                  uint32_t *out_data_size)
{
  *out_data_size = g711_encode((g711_state_t*)m_g711_state, output,
                               (int16_t*)input,
                               (in_data_size / sizeof(int16_t)));
  return *out_data_size;
}

uint32_t AMAudioCodecG711::decode(uint8_t *input,
                                  uint32_t in_data_size,
                                  uint8_t *output,
                                  uint32_t *out_data_size)
{
  uint32_t decoded_bytes = g711_decode((g711_state_t*)m_g711_state,
                                       (int16_t*)output,
                                       input, in_data_size);
  *out_data_size = (decoded_bytes *
      m_audio_info[AUDIO_INFO_DECODE].sample_size);

  return decoded_bytes;
}

AMAudioCodecG711::AMAudioCodecG711() :
    inherited(AM_AUDIO_CODEC_G711, "g711"),
    m_g711_config(nullptr),
    m_config(nullptr),
    m_g711_state(nullptr)
{}

AMAudioCodecG711::~AMAudioCodecG711()
{
  finalize();
  delete m_config;
}

bool AMAudioCodecG711::init(const char *config)
{
  bool ret = true;

  do {
    ret = inherited::init();
    if (AM_UNLIKELY(!ret)) {
      break;
    }
    m_config = new AMAudioCodecG711Config();
    if (AM_UNLIKELY(!m_config)) {
      ret = false;
      ERROR("Failed to create G711 codec config module!");
      break;
    }
    m_g711_config = m_config->get_config(config);
    if (AM_UNLIKELY(!m_g711_config)) {
      ERROR("Failed to get G711 codec config!");
      ret = false;
      break;
    }
  }while(0);

  return ret;
}
