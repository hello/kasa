/*******************************************************************************
 * am_audio_codec_speex_config.h
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
#ifndef AM_AUDIO_CODEC_SPEEX_CONFIG_H_
#define AM_AUDIO_CODEC_SPEEX_CONFIG_H_

struct SpeexEncode
{
    uint32_t sample_rate;
    uint32_t enc_frame_time_length;
    uint32_t spx_complexity;
    uint32_t spx_avg_bitrate;
    uint32_t spx_vbr_max_bitrate;
    uint32_t spx_quality;
    bool     spx_vbr;
    bool     spx_vad;
    bool     spx_dtx;
    bool     spx_highpass;
    SpeexEncode() :
      sample_rate(16000),
      enc_frame_time_length(40),
      spx_complexity(1),
      spx_avg_bitrate(32000),
      spx_vbr_max_bitrate(48000),
      spx_quality(1),
      spx_vbr(true),
      spx_vad(true),
      spx_dtx(false),
      spx_highpass(false)
    {}
};

struct SpeexDecode
{
    uint32_t    spx_output_channel;
    bool        spx_perceptual_enhance;
};

struct AudioCodecSpeexConfig
{
    SpeexEncode encode;
    SpeexDecode decode;
};

class AMConfig;
class AMAudioCodecSpeex;
class AMAudioCodecSpeexConfig
{
    friend class AMAudioCodecSpeex;
  public:
    AMAudioCodecSpeexConfig();
    virtual ~AMAudioCodecSpeexConfig();
    AudioCodecSpeexConfig* get_config(const std::string& config);

  private:
    AMConfig              *m_config;
    AudioCodecSpeexConfig *m_speex_config;
};

#endif /* AM_AUDIO_CODEC_SPEEX_CONFIG_H_ */
