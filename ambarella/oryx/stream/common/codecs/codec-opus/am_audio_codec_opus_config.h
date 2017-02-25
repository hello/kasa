/*******************************************************************************
 * am_audio_codec_opus_config.h
 *
 * History:
 *   2014-11-10 - [ccjing] created file
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
#ifndef AM_AUDIO_CODEC_OPUS_CONFIG_H_
#define AM_AUDIO_CODEC_OPUS_CONFIG_H_

struct OpusEncode
{
    uint32_t opus_complexity;
    uint32_t opus_avg_bitrate;
    uint32_t sample_rate;
    uint32_t enc_output_size;
    uint32_t enc_frame_time_length;
    uint32_t output_channel;
    bool     repacketize;
    OpusEncode() :
        opus_complexity(1),
        opus_avg_bitrate(48000),
        sample_rate(48000),
        enc_output_size(4096),
        enc_frame_time_length(20),
        output_channel(2),
        repacketize(true)
    {
    }
};

struct OpusDecode
{
    uint32_t dec_output_max_time_length;
    uint32_t dec_output_sample_rate;
    uint32_t dec_output_channel;
    OpusDecode() :
        dec_output_max_time_length(120),
        dec_output_sample_rate(48000),
        dec_output_channel(2)
    {
    }
};

struct AudioCodecOpusConfig
{
    OpusEncode encode;
    OpusDecode decode;
};

class AMConfig;
class AMAudioCodecOpus;
class AMAudioCodecOpusConfig
{
    friend class AMAudioCodecOpus;
  public:
    AMAudioCodecOpusConfig();
    virtual ~AMAudioCodecOpusConfig();
    AudioCodecOpusConfig* get_config(const std::string& config);

  private:
    AMConfig *m_config;
    AudioCodecOpusConfig *m_opus_config;
};

#endif /* AM_AUDIO_CODEC_OPUS_CONFIG_H_ */
