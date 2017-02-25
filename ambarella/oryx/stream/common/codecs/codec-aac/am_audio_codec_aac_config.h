/*******************************************************************************
 * am_audio_codec_aac_config.h
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
#ifndef AM_AUDIO_CODEC_AAC_CONFIG_H_
#define AM_AUDIO_CODEC_AAC_CONFIG_H_

#include "aac_audio_enc.h"
#include "aac_audio_dec.h"

enum AAC_QUANTIZER_QUALITY
{
  QUANTIZER_QUALITY_LOW     = 0,
  QUANTIZER_QUALITY_HIGH    = 1,
  QUANTIZER_QUALITY_HIGHEST = 2,
};

enum AAC_CHANNEL
{
  AAC_MONO   = 1,
  AAC_STEREO = 2,
};

struct AacEncode
{
    uint32_t enc_buf_size;
    uint32_t enc_out_buf_size;
    uint32_t sample_rate;
    uint32_t bitrate;
    uint32_t tns;
    uint32_t quantizer_quality;
    uint32_t output_channel;
    AAC_MODE format;
    int8_t   fftype;
    AacEncode() :
      enc_buf_size(106000),
      enc_out_buf_size(1636),
      sample_rate(48000),
      bitrate(48000),
      tns(1),
      quantizer_quality(QUANTIZER_QUALITY_HIGH),
      output_channel(AAC_MONO),
      format(AACPLAIN),
      fftype('t')
    {}
};

struct AacDecode
{
    uint32_t output_resolution;
    uint32_t dec_buf_size;
    uint32_t dec_out_buf_size;
    uint32_t output_channel;
    AacDecode() :
      output_resolution(16),
      dec_buf_size(25000),
      dec_out_buf_size(16384),
      output_channel(AAC_STEREO)
    {}
};

struct AudioCodecAacConfig
{
    AacEncode encode;
    AacDecode decode;
};

class AMConfig;
class AMAudioCodecAac;
class AMAudioCodecAacConfig
{
    friend class AMAudioCodecAac;
  public:
    AMAudioCodecAacConfig();
    virtual ~AMAudioCodecAacConfig();
    AudioCodecAacConfig* get_config(const std::string& config);

  private:
    AMConfig            *m_config;
    AudioCodecAacConfig *m_aac_config;
};

#endif /* AM_AUDIO_CODEC_AAC_CONFIG_H_ */
