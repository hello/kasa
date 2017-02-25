/*******************************************************************************
 * am_audio_codec_aac.cpp
 *
 * History:
 *   2014-11-3 - [ypchang] created file
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

#include "aac_audio_enc.h"
#include "aac_audio_dec.h"

#include "am_audio_define.h"
#include "am_audio_codec_if.h"
#include "am_audio_codec.h"
#include "am_audio_codec_aac.h"
#include "am_audio_codec_aac_config.h"

struct AacStatusMsg
{
    uint32_t    error;
    std::string message;
};

static AacStatusMsg encoder_status[] =
{
  {ENCODE_OK, "OK"},
  {ENCODE_INVALID_POINTER, "Invalid Pointer"},
  {ENCODE_FAILED, "Encode Failed"},
  {ENCODE_UNSUPPORTED_SAMPLE_RATE, "Unsupported Sample Rate"},
  {ENCODE_UNSUPPORTED_CH_CFG, "Unsupported Channel Configuration"},
  {ENCODE_UNSUPPORTED_BIT_RATE, "Unsupported Bit Rate"},
  {ENCODE_UNSUPPORTED_MODE, "Unsupported Mode"}
};

static AacStatusMsg decoder_status[] =
{
  {AAC_DEC_OK, "OK"},
  {MAIN_UCI_OUT_OF_MEMORY, "MAIN_UCI_OUT_OF_MEMORY"},
  {MAIN_UCI_HELP_MODE_ACTIV, "MAIN_UCI_HELP_MODE_ACTIV"},
  {MAIN_OPEN_BITSTREAM_FILE_FAILED, "MAIN_OPEN_BITSTREAM_FILE_FAILED"},
  {MAIN_OPEN_16_BIT_PCM_FILE_FAILED, "MAIN_OPEN_16_BIT_PCM_FILE_FAILED"},
  {MAIN_FRAME_COUNTER_REACHED_STOP_FRAME,
   "MAIN_FRAME_COUNTER_REACHED_STOP_FRAME"},
  {MAIN_TERMINATED_BY_ESC,      "MAIN_TERMINATED_BY_ESC"},
  {AAC_DEC_ADTS_SYNC_ERROR,     "AAC_DEC_ADTS_SYNC_ERROR"},
  {AAC_DEC_LOAS_SYNC_ERROR,     "AAC_DEC_LOAS_SYNC_ERROR"},
  {AAC_DEC_ADTS_SYNCWORD_ERROR, "AAC_DEC_ADTS_SYNCWORD_ERROR"},
  {AAC_DEC_LOAS_SYNCWORD_ERROR, "AAC_DEC_LOAS_SYNCWORD_ERROR"},
  {AAC_DEC_ADIF_SYNCWORD_ERROR, "AAC_DEC_ADIF_SYNCWORD_ERROR"},
  {AAC_DEC_UNSUPPORTED_FORMAT,  "AAC_DEC_UNSUPPORTED_FORMAT"},
  {AAC_DEC_DECODE_FRAME_ERROR,  "AAC_DEC_DECODE_FRAME_ERROR"},
  {AAC_DEC_CRC_CHECK_ERROR,     "AAC_DEC_CRC_CHECK_ERROR"},
  {AAC_DEC_INVALID_CODE_BOOK,   "AAC_DEC_INVALID_CODE_BOOK"},
  {AAC_DEC_UNSUPPORTED_WINOW_SHAPE, "AAC_DEC_UNSUPPORTED_WINOW_SHAPE"},
  {AAC_DEC_PREDICTION_NOT_SUPPORTED_IN_LC_AAC,
   "AAC_DEC_PREDICTION_NOT_SUPPORTED_IN_LC_AAC"},
  {AAC_DEC_UNIMPLEMENTED_CCE, "AAC_DEC_UNIMPLEMENTED_CCE"},
  {AAC_DEC_UNIMPLEMENTED_GAIN_CONTROL_DATA,
   "AAC_DEC_UNIMPLEMENTED_GAIN_CONTROL_DATA"},
  {AAC_DEC_UNIMPLEMENTED_EP_SPECIFIC_CONFIG_PARSE,
   "AAC_DEC_UNIMPLEMENTED_EP_SPECIFIC_CONFIG_PARSE"},
  {AAC_DEC_UNIMPLEMENTED_CELP_SPECIFIC_CONFIG_PARSE,
   "AAC_DEC_UNIMPLEMENTED_CELP_SPECIFIC_CONFIG_PARSE"},
  {AAC_DEC_UNIMPLEMENTED_HVXC_SPECIFIC_CONFIG_PARSE,
   "AAC_DEC_UNIMPLEMENTED_HVXC_SPECIFIC_CONFIG_PARSE"},
  {AAC_DEC_OVERWRITE_BITS_IN_INPUT_BUFFER,
   "AAC_DEC_OVERWRITE_BITS_IN_INPUT_BUFFER"},
  {AAC_DEC_CANNOT_REACH_BUFFER_FULLNESS,
   "AAC_DEC_CANNOT_REACH_BUFFER_FULLNESS"},
  {AAC_DEC_TNS_RANGE_ERROR,
   "AAC_DEC_TNS_RANGE_ERROR"},
  {AAC_DEC_NEED_MORE_DATA,
   "AAC_DEC_NEED_MORE_DATA"},
  {AAC_DEC_INSUFFICIENT_BACKUP_MEMORY,
   "AAC_DEC_INSUFFICIENT_BACKUP_MEMORY"},
};

static const char* aac_enc_strerror(uint32_t err)
{
  uint32_t len = sizeof(encoder_status) / sizeof(AacStatusMsg);
  uint32_t i = 0;
  for (i = 0; i < len; ++ i) {
    if (AM_LIKELY(encoder_status[i].error == err)) {
      break;
    }
  }

  return (i >= len) ? "Unknown Error" : encoder_status[i].message.c_str();
}

static const char* aac_dec_strerror(uint32_t err)
{
  uint32_t len = sizeof(decoder_status) / sizeof(AacStatusMsg);
  uint32_t i = 0;
  for (i = 0; i < len; ++ i) {
    if (AM_LIKELY(decoder_status[i].error == err)) {
      break;
    }
  }

  return (i >= len) ? "Unknown Error" : decoder_status[i].message.c_str();
}

AMIAudioCodec* get_audio_codec(const char *config)
{
  return AMAudioCodecAac::create(config);
}

AMIAudioCodec* AMAudioCodecAac::create(const char *config)
{
  AMAudioCodecAac *aac = new AMAudioCodecAac();
  if (AM_UNLIKELY(aac && !aac->init(config))) {
    delete aac;
    aac = NULL;
  }
  return ((AMIAudioCodec*)aac);
}

void AMAudioCodecAac::destroy()
{
  inherited::destroy();
}

bool AMAudioCodecAac::initialize(AM_AUDIO_INFO *srcAudioInfo,
                                 AM_AUDIO_CODEC_MODE mode)
{
  if (AM_LIKELY(srcAudioInfo && (!m_is_initialized ||
      (m_src_audio_info->channels != srcAudioInfo->channels) ||
      (m_src_audio_info->sample_rate != srcAudioInfo->sample_rate)))) {
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
        AM_AUDIO_INFO *audioInfo = &m_audio_info[AUDIO_INFO_ENCODE];
        m_is_initialized = false;
        do {
          if (AM_LIKELY(audioInfo->sample_format != AM_SAMPLE_S16LE)) {
            ERROR("Invalid input audio sample type! Only S16LE is supported!");
            break;
          }
          if (AM_LIKELY(!m_enc_buffer)) {
            m_enc_buffer = new uint8_t[m_aac_config->encode.enc_buf_size];
          }
          if (AM_UNLIKELY(!m_enc_buffer)) {
            ERROR("Failed to allocate AAC codec encode buffer!");
            break;
          }
          if (AM_LIKELY(!m_enc_conf)) {
            m_enc_conf = new au_aacenc_config_t;
          }
          if (AM_UNLIKELY(!m_enc_conf)) {
            ERROR("Failed to allocate AAC codec encode config structure!");
            break;
          }
          memset(m_enc_conf, 0, sizeof(*m_enc_conf));
          audioInfo->type = AM_AUDIO_AAC;
          m_enc_conf->sample_freq = audioInfo->sample_rate;
          m_enc_conf->Src_numCh   = audioInfo->channels;
          m_enc_conf->enc_mode    = m_aac_config->encode.format;
          /*m_enc_conf->Out_numCh = m_aac_config->encode.output_channel;*/
          m_enc_conf->tns         = m_aac_config->encode.tns;
          m_enc_conf->ffType      = m_aac_config->encode.fftype;
          m_enc_conf->bitRate     = m_aac_config->encode.bitrate;
          m_enc_conf->quantizerQuality =
              m_aac_config->encode.quantizer_quality;
          m_enc_conf->codec_lib_mem_adr = (uint32_t*)m_enc_buffer;
          if (AM_UNLIKELY((m_enc_conf->Src_numCh != 2) &&
                          (m_aac_config->encode.format == AACPLUS_PS))) {
            ERROR("AAC Plus PS requires stereo audio input, "
                "but source audio channel number is: %u",
                audioInfo->channels);
            break;
          }
          if (AM_LIKELY(m_aac_config->encode.format == AACPLUS_PS)) {
            /* AACPlus_PS's output channel number is always 1 */
            audioInfo->channels = 1;
          }
          aacenc_setup(m_enc_conf);
          aacenc_open(m_enc_conf);
          m_is_initialized = true;
          INFO("AAC codec is initialized for encoding!");
        }while(0);
        if (AM_UNLIKELY(!m_is_initialized)) {
          delete[] m_enc_buffer;
          delete[] m_enc_conf;
          m_enc_buffer = nullptr;
          m_enc_conf = nullptr;
        }
      }break;
      case AM_AUDIO_CODEC_MODE_DECODE: {
        AM_AUDIO_INFO *audioInfo = &m_audio_info[AUDIO_INFO_DECODE];
        m_is_initialized = false;
        do {
          if (AM_LIKELY(!m_dec_buffer)) {
            m_dec_buffer = new uint32_t[m_aac_config->decode.dec_buf_size];
          }
          if (AM_UNLIKELY(!m_dec_buffer)) {
            ERROR("Failed to allocate AAC codec decode buffer!");
            break;
          }
          if (AM_LIKELY(!m_dec_conf)) {
            m_dec_conf = new au_aacdec_config_t;
          }
          if (AM_UNLIKELY(!m_dec_conf)) {
            ERROR("Failed to allocate AAC decode config structure!");
            break;
          }
          if (AM_LIKELY(!m_dec_out_buffer)) {
            m_dec_out_buffer =
                new int32_t[m_aac_config->decode.dec_out_buf_size];
          }
          if (AM_LIKELY(!m_dec_out_buffer)) {
            ERROR("Failed to allocate AAC decode output buffer!");
            break;
          }
          memset(m_dec_conf, 0, sizeof(*m_dec_conf));
          m_dec_conf->srcNumCh = audioInfo->channels;
          m_dec_conf->outNumCh = m_aac_config->decode.output_channel;
          m_dec_conf->Out_res  = m_aac_config->decode.output_resolution;
          m_dec_conf->codec_lib_mem_addr   = m_dec_buffer;
          m_dec_conf->externalSamplingRate = audioInfo->sample_rate;
          if (AM_LIKELY(audioInfo->codec_info)) {
            if (is_str_equal((const char*)audioInfo->codec_info, "adts")) {
              m_dec_conf->bsFormat = ADTS_BSFORMAT;
            } else if (is_str_equal((const char*)audioInfo->codec_info,
                                    "raw")) {
              m_dec_conf->bsFormat = RAW_BSFORMAT;
            } else {
              ERROR("Invalid bit stream format! Neither ADTS nor RAW!");
              break;
            }
          } else {
            /* If codec info is not set, use default mode */
            m_dec_conf->bsFormat = ADTS_BSFORMAT;
          }
          switch(m_dec_conf->Out_res) {
            case 16: audioInfo->sample_format = AM_SAMPLE_S16LE; break;
            case 32: audioInfo->sample_format = AM_SAMPLE_S32LE; break;
            default:break;
          }
          audioInfo->sample_size = m_dec_conf->Out_res / 8;
          aacdec_setup(m_dec_conf);
          aacdec_open(m_dec_conf);
          m_is_initialized = true;
          INFO("AAC codec is initialized for decoding!");

        }while(0);
        if (AM_UNLIKELY(!m_is_initialized)) {
          delete m_dec_conf;
          delete[] m_dec_buffer;
          delete[] m_dec_out_buffer;
          m_dec_conf = nullptr;
          m_dec_buffer = nullptr;
          m_dec_out_buffer = nullptr;
        }
      }break;
      default: break;
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

bool AMAudioCodecAac::finalize()
{
  if (AM_LIKELY(m_is_initialized)) {
    switch(m_mode) {
      case AM_AUDIO_CODEC_MODE_ENCODE : aacenc_close(); break;
      case AM_AUDIO_CODEC_MODE_DECODE : aacdec_close(); break;
      default: break;
    }
    m_is_initialized = false;
    m_mode = AM_AUDIO_CODEC_MODE_NONE;
    INFO("AAC codec is finalized!");
  }

  return !m_is_initialized;
}

AM_AUDIO_INFO* AMAudioCodecAac::get_codec_audio_info()
{
  AM_AUDIO_INFO *info = NULL;
  switch(m_mode) {
    case AM_AUDIO_CODEC_MODE_ENCODE : {
      info = &m_audio_info[AUDIO_INFO_ENCODE];
    }break;
    case AM_AUDIO_CODEC_MODE_DECODE : {
      info = &m_audio_info[AUDIO_INFO_DECODE];
    }break;
    default: break;
  }
  return info;
}

uint32_t AMAudioCodecAac::get_codec_output_size()
{
  uint32_t size = 0;
  switch(m_mode) {
    case AM_AUDIO_CODEC_MODE_ENCODE:
      size = m_aac_config->encode.enc_out_buf_size;
      break;
    case AM_AUDIO_CODEC_MODE_DECODE:
      size = m_aac_config->decode.dec_out_buf_size;
      break;
    default: ERROR("AAC audio codec is not initialized!"); break;
  }

  return size;
}

bool AMAudioCodecAac::get_encode_required_src_parameter(AM_AUDIO_INFO &info)
{
  bool ret = true;
  memset(&info, 0, sizeof(info));
  info.sample_rate = m_aac_config->encode.sample_rate;
  info.sample_format = AM_SAMPLE_S16LE;
  info.channels = m_aac_config->encode.output_channel;
  switch(m_aac_config->encode.format) {
    case AACPLAIN: {
      info.chunk_size = 1024 * info.channels * sizeof(int16_t);
    }break;
    case AACPLUS:
    case AACPLUS_PS: {
      if (m_aac_config->encode.format == AACPLUS_PS) {
        /* AACPlus_PS requires stereo input */
        info.channels = 2;
      }
      info.chunk_size = 2048 * info.channels * sizeof(int16_t);
    }break;
    case AACPLUS_SPEECH:
    case AACPLUS_SPEECH_PS: ret = false; break;
  }

  return ret;
}

uint32_t AMAudioCodecAac::encode(uint8_t *input,  uint32_t in_data_size,
                                 uint8_t *output, uint32_t *out_data_size)
{
  *out_data_size = 0;
  if (AM_LIKELY(m_enc_conf)) {
    m_enc_conf->enc_rptr = (int32_t*)input;
    m_enc_conf->enc_wptr = output;
    aacenc_encode(m_enc_conf);
    if (AM_UNLIKELY(m_enc_conf->ErrorStatus)) {
      ERROR("AAC encoding error: %s, encode mode: %d!",
            aac_enc_strerror(m_enc_conf->ErrorStatus),
            m_enc_conf->enc_mode);
    } else {
      *out_data_size = (uint32_t)((m_enc_conf->nBitsInRawDataBlock + 7) >> 3);
    }
  } else {
    ERROR("AAC codec is not initialized!");
  }

  return *out_data_size;
}

uint32_t AMAudioCodecAac::decode(uint8_t *input,  uint32_t in_data_size,
                                 uint8_t *output, uint32_t *out_data_size)
{
  uint32_t ret = 0;
  if (AM_LIKELY(m_dec_conf)) {
    m_dec_conf->dec_rptr = input;
    m_dec_conf->dec_wptr = m_dec_out_buffer;
    m_dec_conf->interBufSize = in_data_size;
    m_dec_conf->consumedByte = 0;
    *out_data_size = 0;
    aacdec_set_bitstream_rp(m_dec_conf);
    aacdec_decode(m_dec_conf);

    if (AM_UNLIKELY(m_dec_conf->ErrorStatus)) {
      ERROR("AAC decoding error: %s, consumed %u bytes!",
            aac_dec_strerror((uint32_t)m_dec_conf->ErrorStatus),
            m_dec_conf->consumedByte);
      /* Skip broken data */
      m_dec_conf->consumedByte = (m_dec_conf->consumedByte == 0) ?
          in_data_size : m_dec_conf->consumedByte;
    }
    if (AM_LIKELY(m_dec_conf->frameCounter > 0)) {
      *out_data_size = m_dec_conf->frameSize *
          m_dec_conf->outNumCh * m_dec_conf->Out_res / 8;
      memcpy(output, m_dec_conf->dec_wptr, *out_data_size);
      /* This is the sample rate of decoded PCM audio data */
      m_audio_info[AUDIO_INFO_DECODE].sample_rate = m_dec_conf->fs_out;
      m_audio_info[AUDIO_INFO_DECODE].channels    = m_dec_conf->outNumCh;
#if 0
      if (AM_LIKELY(m_dec_conf->srcNumCh == 1)) {
        deinterleave((int16_t*)output, out_data_size, 0);
      }
#endif
    }
    ret = m_dec_conf->consumedByte;
  } else {
    ERROR("AAC codec is not initialized!");
  }

  return ret;
}

AMAudioCodecAac::AMAudioCodecAac() :
    inherited(AM_AUDIO_CODEC_AAC, "aac"),
    m_aac_config(NULL),
    m_config(NULL),
    m_enc_conf(NULL),
    m_dec_conf(NULL),
    m_enc_buffer(NULL),
    m_dec_buffer(NULL),
    m_dec_out_buffer(NULL)
{}

AMAudioCodecAac::~AMAudioCodecAac()
{
  finalize();
  delete m_config;
  delete   m_enc_conf;
  delete   m_dec_conf;
  delete[] m_enc_buffer;
  delete[] m_dec_buffer;
  delete[] m_dec_out_buffer;
}

bool AMAudioCodecAac::init(const char *config)
{
  bool ret = true;
  std::string conf_file(config);

  do {
    ret = inherited::init();
    if (AM_UNLIKELY(!ret)) {
      break;
    }
    m_config = new AMAudioCodecAacConfig();
    if (AM_UNLIKELY(!m_config)) {
      ret = false;
      ERROR("Failed to create AAC codec config module!");
      break;
    }
    m_aac_config = m_config->get_config(conf_file);
    if (AM_UNLIKELY(!m_aac_config)) {
      ERROR("Failed to get AAC codec config!");
      ret = false;
      break;
    }
  } while(0);

  return ret;
}
