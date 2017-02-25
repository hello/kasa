/*******************************************************************************
 * am_audio_codec_g726_config.h
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
#ifndef AM_AUDIO_CODEC_G726_CONFIG_H_
#define AM_AUDIO_CODEC_G726_CONFIG_H_

#include "am_audio_define.h"

enum G726_RATE
{
  G726_16K = 16,
  G726_24K = 24,
  G726_32K = 32,
  G726_40K = 40,
};

struct AudioCodecG726Config
{
    AM_AUDIO_SAMPLE_FORMAT decode_law;
    G726_RATE              encode_rate;
    uint32_t               encode_frame_time_length;
    AudioCodecG726Config() :
      decode_law(AM_SAMPLE_S16LE),
      encode_rate(G726_32K),
      encode_frame_time_length(80)
    {}
};

class AMConfig;
class AMAudioCodecG726Config
{
  public:
    AMAudioCodecG726Config();
    virtual ~AMAudioCodecG726Config();
    AudioCodecG726Config* get_config(const std::string& conf_file);

  private:
    AMConfig             *m_config;
    AudioCodecG726Config *m_g726_config;
};

#endif /* AM_AUDIO_CODEC_G726_CONFIG_H_ */
