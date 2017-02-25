/*******************************************************************************
 * am_audio_codec_g711_config.h
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
#ifndef ORYX_STREAM_COMMON_CODECS_CODEC_G711_AM_AUDIO_CODEC_G711_CONFIG_H_
#define ORYX_STREAM_COMMON_CODECS_CODEC_G711_AM_AUDIO_CODEC_G711_CONFIG_H_

#include "am_audio_define.h"

struct AudioCodecG711Config
{
    AM_AUDIO_SAMPLE_FORMAT encode_law;
    uint32_t               encode_rate;
    uint32_t               encode_frame_time_length;
    AudioCodecG711Config() :
      encode_law(AM_SAMPLE_ALAW),
      encode_rate(16000),
      encode_frame_time_length(80)
    {}
};

class AMConfig;
class AMAudioCodecG711Config
{
  public:
    AMAudioCodecG711Config();
    virtual ~AMAudioCodecG711Config();
    AudioCodecG711Config* get_config(const std::string& conf_file);

  private:
    AMConfig             *m_config;
    AudioCodecG711Config *m_g711_config;
};

#endif /* ORYX_STREAM_COMMON_CODECS_CODEC_G711_AM_AUDIO_CODEC_G711_CONFIG_H_ */
