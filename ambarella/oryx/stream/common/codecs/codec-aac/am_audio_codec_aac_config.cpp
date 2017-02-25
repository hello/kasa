/*******************************************************************************
 * am_audio_codec_aac_config.cpp
 *
 * History:
 *   2014-11-4 - [ypchang] created file
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

#include "am_audio_codec_aac_config.h"
#include "am_configure.h"

AMAudioCodecAacConfig::AMAudioCodecAacConfig() :
  m_config(NULL),
  m_aac_config(NULL)
{}

AMAudioCodecAacConfig::~AMAudioCodecAacConfig()
{
  delete m_config;
  delete m_aac_config;
}

AudioCodecAacConfig* AMAudioCodecAacConfig::get_config(
    const std::string& conf_file)
{
  AudioCodecAacConfig *config = NULL;
  do {
    if (AM_LIKELY(NULL == m_aac_config)) {
      m_aac_config = new AudioCodecAacConfig();
    }
    if (AM_UNLIKELY(!m_aac_config)) {
      ERROR("Failed to create AudioCodecAacConfig object!");
      break;
    }
    delete m_config;
    m_config = AMConfig::create(conf_file.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig &aac = *m_config;

      /* Encoder settings */
      if (AM_LIKELY(aac["encode"].exists())) {
        /* encode buffer size */
        if (AM_LIKELY(aac["encode"]["enc_buf_size"].exists())) {
          m_aac_config->encode.enc_buf_size =
              aac["encode"]["enc_buf_size"].get<unsigned>(106000);
        } else {
          NOTICE("\"enc_buf_size\" is not specified, use default!");
          m_aac_config->encode.enc_buf_size = 106000;
        }

        /* encode output size */
        if (AM_LIKELY(aac["encode"]["enc_out_buf_size"].exists())) {
          m_aac_config->encode.enc_out_buf_size =
              aac["encode"]["enc_out_buf_size"].get<unsigned>(1636);
        } else {
          NOTICE("\"enc_out_buf_size\" is not specified, use default!");
          m_aac_config->encode.enc_out_buf_size = 6144 / 8 * 2 + 100;
        }

        /* format */
        if (AM_LIKELY(aac["encode"]["format"].exists())) {
          std::string format = aac["encode"]["format"].get<std::string>("aac");
          if (is_str_equal(format.c_str(), "aac")) {
            m_aac_config->encode.format = AACPLAIN;
          } else if (is_str_equal(format.c_str(), "aacplus")) {
            m_aac_config->encode.format = AACPLUS;
          } else if (is_str_equal(format.c_str(), "aacplusps")) {
            m_aac_config->encode.format = AACPLUS_PS;
          } else {
            NOTICE("Unsupported format: %s, use default!");
            m_aac_config->encode.format = AACPLAIN;
          }
        } else {
          NOTICE("\"format\" is not specified, use default!");
          m_aac_config->encode.format = AACPLAIN;
        }

        /* sample rate */
        if (AM_LIKELY(aac["encode"]["sample_rate"].exists())) {
          uint32_t &sample_rate = m_aac_config->encode.sample_rate;
          sample_rate = aac["encode"]["sample_rate"].get<unsigned>(48000);
          switch(m_aac_config->encode.format) {
            case AACPLAIN: {
              if (AM_UNLIKELY((48000 != sample_rate) &&
                              (44100 != sample_rate) &&
                              (32000 != sample_rate) &&
                              (24000 != sample_rate) &&
                              (22050 != sample_rate) &&
                              (16000 != sample_rate) &&
                              (12000 != sample_rate) &&
                              (11025 != sample_rate) &&
                              (8000  != sample_rate))) {
                NOTICE("Invalid sample rate %u for AAC Plain encoding! "
                       "Reset to 48000", sample_rate);
                sample_rate = 48000;
              }
            }break;
            case AACPLUS:
            case AACPLUS_PS: {
              if (AM_UNLIKELY((48000 != sample_rate) &&
                              (44100 != sample_rate) &&
                              (32000 != sample_rate))) {
                NOTICE("Invalid sample rate %u for AAC Plus/AAC Plus PS "
                       "encoding! Reset to 48000");
                sample_rate = 48000;
              }
            }break;
            default:break;
          }
        } else {
          NOTICE("\"sample_rate\" is not specified, use default!");
          m_aac_config->encode.sample_rate = 48000;
        }

        /* output channel */
        if (AM_LIKELY(aac["encode"]["output_channel"].exists())) {
          std::string channel =
              aac["encode"]["output_channel"].get<std::string>("stereo");
          if (is_str_equal(channel.c_str(), "stereo")) {
            m_aac_config->encode.output_channel = AAC_STEREO;
            if (AM_LIKELY(m_aac_config->encode.format == AACPLUS_PS)) {
              NOTICE("AACPlusPs only supports MONO output! Reset to MONO!");
              m_aac_config->encode.output_channel = AAC_MONO;
            }
          } else if (is_str_equal(channel.c_str(), "mono")) {
            m_aac_config->encode.output_channel = AAC_MONO;
          } else {
            NOTICE("Invalid output channel setting: %s, use stereo as default",
                   channel.c_str());
            m_aac_config->encode.output_channel = AAC_STEREO;
          }
        } else {
          NOTICE("\"output_channel\" is not specified, use default!");
          m_aac_config->encode.output_channel =
              (m_aac_config->encode.format == AACPLUS_PS) ?
                  AAC_MONO : AAC_STEREO;
        }

        /* bitrate */
        if (AM_LIKELY(aac["encode"]["bitrate"].exists())) {
          m_aac_config->encode.bitrate =
              aac["encode"]["bitrate"].get<unsigned>(48000);
          switch(m_aac_config->encode.format) {
            case AACPLAIN: {
              uint32_t max = 0;
              uint32_t min = 0;
              switch(m_aac_config->encode.sample_rate) {
                case 48000:
                case 44100:
                case 32000:
                case 24000:
                case 22050:
                case 16000: {
                  min = m_aac_config->encode.output_channel * 16000;
                  max = (m_aac_config->encode.output_channel *
                      m_aac_config->encode.sample_rate * 6144) / 1024;
                }break;
                case 12000:
                case 11025: {
                  min = m_aac_config->encode.output_channel * 8000;
                  max = m_aac_config->encode.output_channel * 40000;
                }break;
                case 8000: {
                  min = m_aac_config->encode.output_channel * 8000;
                  max = m_aac_config->encode.output_channel * 12000;
                }break;
                default: {
                  max = 160000;
                  min = 16000;
                }break;
              }
              if (m_aac_config->encode.bitrate > max) {
                WARN("Bit rate exceeds AAC maximum bit rate(%u), "
                     "reset to %u", max, max);
                m_aac_config->encode.bitrate = max;
              } else if (m_aac_config->encode.bitrate < min) {
                WARN("Bit rate is less than AAC minimum bit rate(%u) "
                     "reset to %u", min, min);
                m_aac_config->encode.bitrate = min;
              }
            }break;
            case AACPLUS: {
              uint32_t min = m_aac_config->encode.output_channel * 14000;
              uint32_t max = m_aac_config->encode.output_channel * 64000;
              if (m_aac_config->encode.bitrate > max) {
                WARN("Bit rate exceeds AAC Plus maximum bit rate(%u) "
                     "reset to %u", max, max);
                m_aac_config->encode.bitrate = max;
              } else if (m_aac_config->encode.bitrate < min) {
                WARN("Bit rate is less than AAC Plus minimum bit rate(%u) "
                     "reset to %u");
                m_aac_config->encode.bitrate = min;
              }
            }break;
            case AACPLUS_PS: {
              uint32_t min = 14000;
              uint32_t max = 64000;
              if (m_aac_config->encode.bitrate > max) {
                WARN("Bit rate exceeds AAC Plus PS maximum bit rate(%u) "
                     "reset to %u", max, max);
                m_aac_config->encode.bitrate = max;
              } else if (m_aac_config->encode.bitrate < min) {
                WARN("Bit rate is less than AAC Plus PS minimum bit rate"
                     "(%u) reset to %u", min, min);
                m_aac_config->encode.bitrate = min;
              }
            }break;
            default:break;
          }
        } else {
          NOTICE("\"bitrate\" is not specified, use default!");
          m_aac_config->encode.bitrate = 48000;
        }

        /* fftype */
        if (AM_LIKELY(aac["encode"]["fftype"].exists())) {
          std::string fftype = aac["encode"]["fftype"].get<std::string>("t");
          const char *fftype_str = fftype.c_str();
          m_aac_config->encode.fftype = fftype_str[0];
        } else {
          NOTICE("\"fftype\" is not specified, use default!");
          m_aac_config->encode.fftype = 't';
        }

        /* tns */
        if (AM_LIKELY(aac["encode"]["tns"].exists())) {
          m_aac_config->encode.tns = aac["encode"]["tns"].get<unsigned>(1);
        } else {
          NOTICE("\"tns\" is not specified, use default!");
          m_aac_config->encode.tns = 1;
        }

        /* quantizer quality */
        if (AM_LIKELY(aac["encode"]["quantizer_quality"].exists())) {
          std::string quality =
              aac["encode"]["quantizer_quality"].get<std::string>("stereo");
          if (is_str_equal(quality.c_str(), "low")) {
            m_aac_config->encode.quantizer_quality = QUANTIZER_QUALITY_LOW;
          } else if (is_str_equal(quality.c_str(), "high")) {
            m_aac_config->encode.quantizer_quality = QUANTIZER_QUALITY_HIGH;
          } else if (is_str_equal(quality.c_str(), "highest")) {
            m_aac_config->encode.quantizer_quality = QUANTIZER_QUALITY_HIGHEST;
          } else {
            NOTICE("Invalid quantizer quality setting: %s, use high as default",
                   quality.c_str());
            m_aac_config->encode.quantizer_quality = QUANTIZER_QUALITY_HIGH;
          }
        } else {
          NOTICE("\"quantizer_quality\" is not specified, use default!");
          m_aac_config->encode.quantizer_quality = QUANTIZER_QUALITY_HIGH;
        }
      } else {
        ERROR("Invalid AAC configuration! Encoder settings not found!");
        break;
      }

      /* Decoder settings */
      if (AM_LIKELY(aac["decode"].exists())) {
        /* output resolution */
        if (AM_LIKELY(aac["decode"]["output_resolution"].exists())) {
          m_aac_config->decode.output_resolution =
              aac["decode"]["output_resolution"].get<unsigned>(16);
        } else {
          NOTICE("\"output_resolution\" is not specified, use default!");
          m_aac_config->decode.output_resolution = 16;
        }
        /* decode buffer size */
        if (AM_LIKELY(aac["decode"]["dec_buf_size"].exists())) {
          m_aac_config->decode.dec_buf_size =
              aac["decode"]["dec_buf_size"].get<unsigned>(25000);
        } else {
          NOTICE("\"dec_buf_size\" is not specified, use default!");
          m_aac_config->decode.dec_buf_size = 25000;
        }

        /* decode output buffer size */
        if (AM_LIKELY(aac["decode"]["dec_out_buf_size"].exists())) {
          m_aac_config->decode.dec_out_buf_size =
              aac["decode"]["dec_out_buf_size"].get<unsigned>(16384);
        } else {
          NOTICE("\"dec_buf_size\" is not specified, use default!");
          m_aac_config->decode.dec_out_buf_size = 16384;
        }

        /* output channel */
        if (AM_LIKELY(aac["decode"]["output_channel"].exists())) {
          std::string channel =
              aac["decode"]["output_channel"].get<std::string>("stereo");
          if (is_str_equal(channel.c_str(), "stereo")) {
            m_aac_config->decode.output_channel = AAC_STEREO;
          } else if (is_str_equal(channel.c_str(), "mono")) {
            m_aac_config->decode.output_channel = AAC_MONO;
          } else {
            NOTICE("Invalid output channel setting: %s, use stereo as default",
                   channel.c_str());
            m_aac_config->decode.output_channel = AAC_STEREO;
          }
        } else {
          NOTICE("\"output_channel\" is not specified, use default!");
          m_aac_config->decode.output_channel = AAC_STEREO;
        }
      } else {
        ERROR("Invalid AAC configuration! Decoder settings not found!");
        break;
      }
    } else {
      ERROR("Failed to load configuration file: %s!", conf_file.c_str());
      break;
    }

    config = m_aac_config;
  }while(0);

  return config;
}

