/*******************************************************************************
 * am_audio_codec_g726.cpp
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

#include "am_amf_types.h"

#include "am_audio_codec_if.h"
#include "am_audio_codec.h"
#include "am_audio_codec_g726.h"
#include "am_audio_codec_g726_config.h"

#include "g726.h"

AMIAudioCodec* get_audio_codec(const char *config)
{
  return AMAudioCodecG726::create(config);
}

AMIAudioCodec* AMAudioCodecG726::create(const char *config)
{
  AMAudioCodecG726 *g726 = new AMAudioCodecG726();
  if (AM_UNLIKELY(g726 && !g726->init(config))) {
    delete g726;
    g726 = NULL;
  }
  return ((AMIAudioCodec*)g726);
}

void AMAudioCodecG726::destroy()
{
  inherited::destroy();
}

bool AMAudioCodecG726::initialize(AM_AUDIO_INFO *srcAudioInfo,
                                  AM_AUDIO_CODEC_MODE mode)
{
  if (AM_LIKELY(srcAudioInfo && (!m_is_initialized ||
      (m_src_audio_info->sample_format != srcAudioInfo->sample_format)))) {
    memcpy(m_src_audio_info, srcAudioInfo, sizeof(*m_src_audio_info));
    memcpy(&m_audio_info[AUDIO_INFO_ENCODE],
           srcAudioInfo, sizeof(*srcAudioInfo));
    memcpy(&m_audio_info[AUDIO_INFO_DECODE],
           srcAudioInfo, sizeof(*srcAudioInfo));
    if (AM_UNLIKELY(m_is_initialized)) {
      NOTICE("Codec %s is already initialized to %s mode, re-initializing!",
             m_name.c_str(),
             codec_mode_string().c_str());
      finalize();
    }
    m_mode = mode;
    switch(m_mode) {
      case AM_AUDIO_CODEC_MODE_ENCODE: {
        AM_AUDIO_INFO &audioInfo = m_audio_info[AUDIO_INFO_ENCODE];
        if (AM_LIKELY((audioInfo.channels == 1) &&
                      (audioInfo.sample_rate == 8000) &&
                      (audioInfo.sample_format == AM_SAMPLE_S16LE))) {
          int ext_coding = G726_ENCODING_LINEAR;
          m_bits_per_sample = m_g726_config->encode_rate / 8;
          switch(m_g726_config->encode_rate) {
            case G726_16K: audioInfo.type = AM_AUDIO_G726_16; break;
            case G726_24K: audioInfo.type = AM_AUDIO_G726_24; break;
            case G726_32K: audioInfo.type = AM_AUDIO_G726_32; break;
            case G726_40K: audioInfo.type = AM_AUDIO_G726_40; break;
            default: {
              audioInfo.type = AM_AUDIO_G726_32;
              m_bits_per_sample = G726_32K / 8;
              WARN("Invalid bit rate, reset to 32k!");
            }break;
          }
          m_g726_state = g726_init(NULL, (m_g726_config->encode_rate * 1000),
                                   ext_coding, G726_PACKING_LEFT);
          if (AM_UNLIKELY(!m_g726_state)) {
            ERROR("Failed to initialize G726 codec for encoding!");
          }
          m_is_initialized = (NULL != m_g726_state);
        } else {
          if (AM_LIKELY(audioInfo.channels != 1)) {
            ERROR("Invalid channel number! 1 channel is required!");
            break;
          }
          if (AM_LIKELY(audioInfo.sample_rate != 8000)) {
            ERROR("Invalid sample rate! 8000 sample rate is required!");
          }
          if (AM_LIKELY((audioInfo.sample_format != AM_SAMPLE_S16LE))) {
            ERROR("Invalid input audio type, S16LE are required!");
          }
        }
      }break;
      case AM_AUDIO_CODEC_MODE_DECODE: {
        AM_AUDIO_INFO &audioInfo = m_audio_info[AUDIO_INFO_DECODE];
        if (AM_LIKELY((audioInfo.channels == 1) &&
                      (audioInfo.sample_rate = 8000) &&
                      ((audioInfo.type == AM_AUDIO_G726_16) ||
                       (audioInfo.type == AM_AUDIO_G726_24) ||
                       (audioInfo.type == AM_AUDIO_G726_32) ||
                       (audioInfo.type == AM_AUDIO_G726_40)))) {
          int ext_coding = -1;
          int rate = 32000;
          switch(audioInfo.type) {
            case AM_AUDIO_G726_16: rate = 16000; break;
            case AM_AUDIO_G726_24: rate = 24000; break;
            case AM_AUDIO_G726_32: rate = 32000; break;
            case AM_AUDIO_G726_40: rate = 40000; break;
            default: break;
          }
          audioInfo.sample_format = m_g726_config->decode_law;
          m_bits_per_sample = rate / 8000;
          switch(m_g726_config->decode_law) {
            case AM_SAMPLE_S16LE: {
              ext_coding = G726_ENCODING_LINEAR;
              audioInfo.type = AM_AUDIO_LPCM;
              audioInfo.sample_size = sizeof(int16_t);
            }break;
            case AM_SAMPLE_ALAW: {
              ext_coding = G726_ENCODING_ALAW;
              audioInfo.type = AM_AUDIO_G711A;
              audioInfo.sample_size = sizeof(int8_t);
            }break;
            case AM_SAMPLE_ULAW: {
              ext_coding = G726_ENCODING_ULAW;
              audioInfo.type = AM_AUDIO_G711U;
              audioInfo.sample_size = sizeof(int8_t);
            }break;
            default: {
              ext_coding = G726_ENCODING_LINEAR;
              audioInfo.type = AM_AUDIO_LPCM;
              audioInfo.sample_format = AM_SAMPLE_S16LE;
              audioInfo.sample_size = sizeof(int16_t);
              WARN("Invalid decoding law, reset to S16LE!");
            }break;
          }
          m_g726_state = g726_init(NULL, rate, ext_coding, G726_PACKING_LEFT);
          m_is_initialized = (NULL != m_g726_state);
          if (AM_UNLIKELY(!m_is_initialized)) {
            ERROR("Failed to initialize G.726 codec for decoding!");
          }
        } else {
          ERROR("Invalid G.726 audio!");
        }
      }break;
      default: ERROR("Invalid mode!"); break;
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

bool AMAudioCodecG726::finalize()
{
  if (AM_LIKELY(m_g726_state)) {
    g726_free(m_g726_state);
  }
  m_g726_state = NULL;
  m_is_initialized = false;
  m_mode = AM_AUDIO_CODEC_MODE_NONE;

  return true;
}

AM_AUDIO_INFO* AMAudioCodecG726::get_codec_audio_info()
{
  AM_AUDIO_INFO *info = NULL;
  switch(m_mode) {
    case AM_AUDIO_CODEC_MODE_ENCODE: {
      info = &m_audio_info[AUDIO_INFO_ENCODE];
    }break;
    case AM_AUDIO_CODEC_MODE_DECODE: {
      info = &m_audio_info[AUDIO_INFO_DECODE];
    }break;
    default: ERROR("G.726 audio codec is not initialized!"); break;
  }
  return info;
}

uint32_t AMAudioCodecG726::get_codec_output_size()
{
  uint32_t size = 0;
  switch(m_mode) {
    case AM_AUDIO_CODEC_MODE_ENCODE: {
      uint32_t &chunk_size  = m_audio_info[AUDIO_INFO_ENCODE].chunk_size;
      uint32_t &sample_size = m_audio_info[AUDIO_INFO_ENCODE].sample_size;
      size = ROUND_UP((chunk_size / sample_size * m_bits_per_sample) /
                      sizeof(int8_t) + 1, 4);
    }break;
    case AM_AUDIO_CODEC_MODE_DECODE: {
      uint32_t &chunk_size  = m_audio_info[AUDIO_INFO_DECODE].chunk_size;
      uint32_t &channel_num = m_audio_info[AUDIO_INFO_DECODE].channels;
      size = ROUND_UP((sizeof(int16_t) * channel_num *
                       (chunk_size * 8 / m_bits_per_sample + 1)), 4);
    }break;
    default: ERROR("G.726 audio codec is not initialized!"); break;
  }

  return size;
}

bool AMAudioCodecG726::get_encode_required_src_parameter(AM_AUDIO_INFO &info)
{
  bool ret = true;
  memset(&info, 0, sizeof(info));
  info.channels = 1;
  info.sample_format = AM_SAMPLE_S16LE;
  info.sample_rate = 8000;
  info.chunk_size = m_g726_config->encode_frame_time_length * info.sample_rate /
      1000 * info.channels * sizeof(int16_t);
  return ret;
}

uint32_t AMAudioCodecG726::encode(uint8_t *input,  uint32_t in_data_size,
                                  uint8_t *output, uint32_t *out_data_size)
{
  *out_data_size =
      g726_encode(m_g726_state, output,
                  (int16_t*)input, (in_data_size / sizeof(int16_t)));

  return *out_data_size;
}

uint32_t AMAudioCodecG726::decode(uint8_t *input,  uint32_t in_data_size,
                                  uint8_t *output, uint32_t *out_data_size)
{
  uint32_t used_sample = g726_decode(m_g726_state, (int16_t*)output,
                                     input, in_data_size);
  *out_data_size = (m_audio_info[AUDIO_INFO_DECODE].sample_size * used_sample);

  return (used_sample * m_bits_per_sample / 8);
}

AMAudioCodecG726::AMAudioCodecG726() :
    inherited(AM_AUDIO_CODEC_G726, "g726"),
    m_g726_config(NULL),
    m_config(NULL),
    m_g726_state(NULL),
    m_bits_per_sample(5)
{}

AMAudioCodecG726::~AMAudioCodecG726()
{
  finalize();
  delete m_config;
}

bool AMAudioCodecG726::init(const char *config)
{
  bool ret = true;
  std::string conf_file(config);

  do {
    ret = inherited::init();
    if (AM_UNLIKELY(!ret)) {
      break;
    }
    m_config = new AMAudioCodecG726Config();
    if (AM_UNLIKELY(!m_config)) {
      ret = false;
      ERROR("Failed to create G726 codec config module!");
      break;
    }
    m_g726_config = m_config->get_config(conf_file);
    if (AM_UNLIKELY(!m_g726_config)) {
      ERROR("Failed to get G726 codec config!");
      ret = false;
      break;
    }
  } while (0);

  return ret;
}
