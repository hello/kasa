/*******************************************************************************
 * am_audio_codec_speex.cpp
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

#include "am_amf_types.h"

#include "am_audio_type.h"
#include "am_audio_define.h"
#include "am_audio_codec_if.h"
#include "am_audio_codec.h"
#include "am_audio_codec_speex.h"
#include "am_audio_codec_speex_config.h"

AMIAudioCodec* get_audio_codec(const char *config)
{
  return AMAudioCodecSpeex::create(config);
}

AMIAudioCodec* AMAudioCodecSpeex::create(const char *config)
{
  AMAudioCodecSpeex *spx = new AMAudioCodecSpeex();
  if (AM_UNLIKELY(spx && !spx->init(config))) {
    delete spx;
    spx = nullptr;
  }

  return ((AMIAudioCodec*)spx);
}

void AMAudioCodecSpeex::destroy()
{
  inherited::destroy();
}

bool AMAudioCodecSpeex::initialize(AM_AUDIO_INFO *src_audio_info,
                                   AM_AUDIO_CODEC_MODE mode)
{
  do {
    if (AM_LIKELY(!src_audio_info)) {
      ERROR("Invalid audio info, finalize!");
      finalize();
      m_is_initialized = false;
      break;
    }

    if (AM_UNLIKELY(m_is_initialized &&
                    (mode == AM_AUDIO_CODEC_MODE_ENCODE) &&
                    (src_audio_info->sample_rate ==
                        m_audio_info[AUDIO_INFO_ENCODE].sample_rate) &&
                    (src_audio_info->channels ==
                        m_audio_info[AUDIO_INFO_ENCODE].channels))) {
      NOTICE("Audio encode speex is already initialized and "
             "the audio information is not changed, Do not reinitialize.");
      break;
    } else if (AM_UNLIKELY(m_is_initialized)) {
      NOTICE("Codec %s is already initialized to %s mode, re-initializing!",
             m_name.c_str(),
             codec_mode_string().c_str());
      finalize();
    }

    memcpy(m_src_audio_info, src_audio_info, sizeof(*m_src_audio_info));
    memcpy(&m_audio_info[AUDIO_INFO_ENCODE],
           src_audio_info,
           sizeof(*src_audio_info));
    memcpy(&m_audio_info[AUDIO_INFO_DECODE],
           src_audio_info,
           sizeof(*src_audio_info));
    m_mode = mode;
    switch(m_mode) {
      /* Encode */
      case AM_AUDIO_CODEC_MODE_ENCODE: {
        int value = 0;
        AM_AUDIO_INFO &audioInfo = m_audio_info[AUDIO_INFO_ENCODE];
        if (AM_LIKELY(AM_SAMPLE_S16LE == audioInfo.sample_format)) {
          SpeexEncode &encode = m_speex_config->encode;
          uint32_t &sample_rate = encode.sample_rate;
          audioInfo.type = AM_AUDIO_SPEEX;
          audioInfo.channels = 1;
          m_mode_id = ((sample_rate > 25000) ? SPEEX_MODEID_UWB :
                      ((sample_rate > 12500) ? SPEEX_MODEID_WB :
                      ((sample_rate >= 6000) ? SPEEX_MODEID_NB : -1)));
          if (AM_UNLIKELY(m_mode_id == -1)) {
            ERROR("Invalid encode sample rate: %u!", sample_rate);
          } else {
            m_speex_mode = speex_lib_get_mode(m_mode_id);
            m_speex_state = speex_encoder_init(m_speex_mode);
          }
          if (AM_LIKELY(m_speex_state)) {
            speex_encoder_ctl(m_speex_state,
                              SPEEX_GET_FRAME_SIZE,
                              &m_speex_frame_size);
            m_speex_frame_bytes = m_speex_frame_size * sizeof(int16_t);

            speex_encoder_ctl(m_speex_state,
                              SPEEX_GET_FRAME_SIZE,
                              &m_speex_frame_size);
            speex_encoder_ctl(m_speex_state,
                              SPEEX_SET_COMPLEXITY,
                              &encode.spx_complexity);
            speex_encoder_ctl(m_speex_state,
                              SPEEX_SET_SAMPLING_RATE,
                              &sample_rate);
            if (AM_LIKELY(encode.spx_vbr)) {
              if (AM_LIKELY(encode.spx_vbr_max_bitrate > 0)) {
                speex_encoder_ctl(m_speex_state,
                                  SPEEX_SET_VBR_MAX_BITRATE,
                                  &encode.spx_vbr_max_bitrate);
              }
              speex_encoder_ctl(m_speex_state,
                                SPEEX_SET_VBR_QUALITY,
                                &encode.spx_quality);
              value = 1;
              speex_encoder_ctl(m_speex_state, SPEEX_SET_VBR, &value);
            } else {
              speex_encoder_ctl(m_speex_state,
                                SPEEX_SET_QUALITY,
                                &encode.spx_quality);
              if (AM_LIKELY(encode.spx_vad)) {
                value = 1;
                speex_encoder_ctl(m_speex_state, SPEEX_SET_VAD, &value);
              }
            }
            if (AM_LIKELY(encode.spx_dtx &&
                          (encode.spx_vbr || encode.spx_vad))) {
              value = 1;
              speex_encoder_ctl(m_speex_state, SPEEX_SET_DTX, &value);
            }
            if (AM_LIKELY(encode.spx_avg_bitrate > 0)) {
              speex_encoder_ctl(m_speex_state,
                                SPEEX_SET_ABR,
                                &encode.spx_avg_bitrate);
            }
            value = encode.spx_highpass ? 1 : 0;
            speex_encoder_ctl(m_speex_state, SPEEX_SET_HIGHPASS, &value);
            speex_encoder_ctl(m_speex_state, SPEEX_GET_LOOKAHEAD, &m_lookahead);
            speex_bits_init(&m_speex_bits);
            m_is_initialized = true;
          } else {
            ERROR("Failed to initialize speex encoder state!");
          }
        } else {
          ERROR("Invalid input audio format! S16LE audio is required!");
        }
      }break;
      /* Decode */
      case AM_AUDIO_CODEC_MODE_DECODE: {
        AM_AUDIO_INFO &ainfo = m_audio_info[AUDIO_INFO_DECODE];

        memcpy(&m_speex_header, ainfo.codec_info, sizeof(m_speex_header));

        if (AM_LIKELY(m_speex_config->decode.spx_output_channel == 0)) {
          ainfo.channels = m_speex_header.nb_channels;
        } else {
          ainfo.channels = m_speex_config->decode.spx_output_channel;
          NOTICE("%s decoder is forced to output audio to %u %s, "
                 "Audio's original channel number is %u.",
                 m_name.c_str(),
                 ainfo.channels,
                 ((ainfo.channels > 1) ? "channels" : "channel"),
                 m_speex_header.nb_channels);
        }
        ainfo.sample_rate = m_speex_header.rate;
        ainfo.sample_size = sizeof(int16_t);
        ainfo.sample_format = AM_SAMPLE_S16LE;

        m_mode_id = m_speex_header.mode;
        m_speex_mode = speex_lib_get_mode(m_mode_id);
        m_speex_state = speex_decoder_init(m_speex_mode);

        switch(m_mode_id) {
          case SPEEX_MODEID_NB: {
            INFO("Audio was encoded to narrow band!");
          }break;
          case SPEEX_MODEID_WB: {
            INFO("Audio was encoded to wide band!");
          }break;
          case SPEEX_MODEID_UWB: {
            INFO("Audio was encoded to ultra wide band!");
          }break;
          default:break;
        }
        if (AM_LIKELY(m_speex_state)) {
          int enhance = m_speex_config->decode.spx_perceptual_enhance ? 1 : 0;
          speex_decoder_ctl(m_speex_state, SPEEX_SET_ENH, &enhance);
          speex_decoder_ctl(m_speex_state,
                            SPEEX_GET_FRAME_SIZE,
                            &m_speex_frame_size);
          m_speex_frame_bytes = m_speex_frame_size * sizeof(int16_t);
          speex_decoder_ctl(m_speex_state,
                            SPEEX_SET_SAMPLING_RATE,
                            &ainfo.sample_rate);
          if (AM_LIKELY(ainfo.channels != 1)) {
            ainfo.channels = 2; /* Fixed to stereo */
            m_speex_stereo_state = SPEEX_STEREO_STATE_INIT;
            m_speex_callback.callback_id = SPEEX_INBAND_STEREO;
            m_speex_callback.func = speex_std_stereo_request_handler;
            m_speex_callback.data = &m_speex_stereo_state;
            speex_decoder_ctl(m_speex_state,
                              SPEEX_SET_HANDLER,
                              &m_speex_callback);
          }
          speex_bits_init(&m_speex_bits);
          m_is_initialized = true;
          NOTICE("Received audio: sample rate: %u, channels: %u!",
                 ainfo.sample_rate, ainfo.channels);
        } else {
          ERROR("Failed to initialize speex decoder state!");
        }
      }break;
      default: {
        ERROR("Invalid speex codec mode!");
      }break;
    }
  }while (0);

  return m_is_initialized;
}

bool AMAudioCodecSpeex::finalize()
{
  if (AM_LIKELY(m_speex_state)) {
    switch(m_mode) {
      case AM_AUDIO_CODEC_MODE_ENCODE:
        speex_encoder_destroy(m_speex_state);
        break;
      case AM_AUDIO_CODEC_MODE_DECODE:
        speex_decoder_destroy(m_speex_state);
        break;
      default:break;
    }
    speex_bits_destroy(&m_speex_bits);
  }
  m_speex_state = nullptr;

  return true;
}

AM_AUDIO_INFO* AMAudioCodecSpeex::get_codec_audio_info()
{
  AM_AUDIO_INFO *info = NULL;
  switch(m_mode) {
    case AM_AUDIO_CODEC_MODE_ENCODE: {
      info = &m_audio_info[AUDIO_INFO_ENCODE];
    }break;
    case AM_AUDIO_CODEC_MODE_DECODE: {
      info = &m_audio_info[AUDIO_INFO_DECODE];
    }break;
    default: ERROR("Speex audio codec is not initialized!"); break;
  }
  return info;
}

uint32_t AMAudioCodecSpeex::get_codec_output_size()
{
  uint32_t size = 0;
  switch(m_mode) {
    case AM_AUDIO_CODEC_MODE_ENCODE: {
      /* Speex always encode to mono */
      size = m_speex_frame_bytes;
    }break;
    case AM_AUDIO_CODEC_MODE_DECODE: {
      AM_AUDIO_INFO &ainfo = m_audio_info[AUDIO_INFO_DECODE];
      size = m_speex_frame_bytes * m_speex_header.frames_per_packet *
          ainfo.channels;
    }break;
    default: ERROR("Codec speex is not initialized!"); break;
  }
  return size;
}

bool AMAudioCodecSpeex::get_encode_required_src_parameter(AM_AUDIO_INFO &info)
{
  bool ret = true;
  uint32_t base = 0;
  uint32_t max_chunk = 0;
  uint32_t &sample_rate = m_speex_config->encode.sample_rate;

  memset(&info, 0, sizeof(info));
  info.channels = 1;
  info.sample_rate = sample_rate;
  info.sample_format = AM_SAMPLE_S16LE;
  base = 20 * sample_rate / 1000 * info.channels * sizeof(int16_t);
  max_chunk = m_speex_config->encode.enc_frame_time_length *
      sample_rate / 1000 * info.channels * sizeof(int16_t);

  info.chunk_size = 0;
  while ((info.chunk_size + base) <= max_chunk) {
    info.chunk_size += base;
  }

  return ret;
}

uint32_t AMAudioCodecSpeex::encode(uint8_t  *input,
                                   uint32_t  in_data_size,
                                   uint8_t  *output,
                                   uint32_t *out_data_size)
{
  *out_data_size = 0;
  /* Speex Raw data separator */
  output[0] = ' ';
  output[1] = 'S';
  output[2] = 'P';
  output[3] = 'X';

  /* Speex Raw data size */
  output[4] = 0;
  output[5] = 0;
  output[6] = 0;
  output[7] = 0;
  if (AM_LIKELY(0 == (in_data_size % m_speex_frame_bytes))) {
    uint32_t count = in_data_size / m_speex_frame_bytes;
    for (uint32_t i = 0; i < count; ++ i) {
      speex_encode_int(m_speex_state,
                       (spx_int16_t*)(input + i * m_speex_frame_bytes),
                       &m_speex_bits);
    }
    speex_bits_insert_terminator(&m_speex_bits);
    int ret = speex_bits_write(&m_speex_bits,
                               (char*)&output[8],
                               in_data_size);
    speex_bits_reset(&m_speex_bits);
    output[4] = (ret & 0xff000000) >> 24;
    output[5] = (ret & 0x00ff0000) >> 16;
    output[6] = (ret & 0x0000ff00) >>  8;
    output[7] = (ret & 0x000000ff);
    *out_data_size = ret + 8;
  } else {
    ERROR("Invalid input data length: %u, must be multiple of %u!",
          in_data_size, m_speex_frame_bytes);
  }

  return *out_data_size;
}

uint32_t AMAudioCodecSpeex::decode(uint8_t *input,
                                   uint32_t in_data_size,
                                   uint8_t *output,
                                   uint32_t *out_data_size)
{
  uint32_t ret = in_data_size;
  AM_AUDIO_INFO &ainfo = m_audio_info[AUDIO_INFO_DECODE];

  *out_data_size = 0;
  speex_bits_read_from(&m_speex_bits, (char*)input, in_data_size);

  for (int32_t i = 0; i < m_speex_header.frames_per_packet; ++ i) {
    spx_int16_t *out = (spx_int16_t*)(output + *out_data_size);
    bool ok = true;

    switch(speex_decode_int(m_speex_state, &m_speex_bits, out)) {
      case 0: {
        if (AM_LIKELY(ainfo.channels == 2)) {
          speex_decode_stereo_int(out, m_speex_frame_size,
                                  &m_speex_stereo_state);
        }
        *out_data_size += m_speex_frame_bytes * ainfo.channels;
      }break;
      case -1: {
        NOTICE("End of stream! %u bytes of data generated!", *out_data_size);
        ok = false;
      }break;
      case -2: {
        ERROR("Decoder Error: corrupted stream?");
        ret = (uint32_t)-1;
        ok = false;
      }break;
      default: break;
    }
    if (AM_UNLIKELY(!ok)) {
      break;
    }
  }

  return ret;
}

AMAudioCodecSpeex::AMAudioCodecSpeex() :
    inherited(AM_AUDIO_CODEC_SPEEX, "speex"),
    m_speex_config(nullptr),
    m_config(nullptr),
    m_speex_mode(nullptr),
    m_speex_state(nullptr),
    m_speex_frame_size(0),
    m_speex_frame_bytes(0),
    m_mode_id(-1),
    m_lookahead(0),
    m_speex_remain_frames(0),
    m_is_packet_start(true)
{}

AMAudioCodecSpeex::~AMAudioCodecSpeex()
{
  finalize();
  delete m_config;
}

bool AMAudioCodecSpeex::init(const char *config)
{
  bool ret = true;
  std::string conf_file(config);
  do {
    ret = inherited::init();

    if (AM_UNLIKELY(!ret)) {
      ERROR("inherited init failed!");
      break;
    }
    m_config = new AMAudioCodecSpeexConfig();
    if (AM_UNLIKELY(!m_config)) {
      ret = false;
      ERROR("Failed to create speex codec config module!");
      break;
    }
    m_speex_config = m_config->get_config(conf_file);
    if (AM_UNLIKELY(!m_speex_config)) {
      ERROR("Failed to get speex codec config!");
      ret = false;
      break;
    }
  } while(0);

  return ret;
}
