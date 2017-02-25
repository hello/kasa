/*
 * am_audio_alert_config.h
 *
 * Histroy:
 *  2014-11-19 [Dongge Wu] create file
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
 */

#ifndef AM_AUDIO_ALERT_CONFIG_H
#define AM_AUDIO_ALERT_CONFIG_H

#include "am_configure.h"
#include "am_audio_define.h"

struct AudioAlertConfig
{
    bool enable_alert_detect;
    int32_t channel_num;
    int32_t sample_rate;
    int32_t chunk_bytes;
    AM_AUDIO_SAMPLE_FORMAT sample_format;
    int32_t sensitivity;
    int32_t threshold;
    int32_t direction;
    AudioAlertConfig() :
        enable_alert_detect(false),
        channel_num(2),
        sample_rate(48000),
        chunk_bytes(2048),
        sample_format(AM_SAMPLE_S16LE),
        sensitivity(50),
        threshold(50),
        direction(0)
    {
    }
};

class AMConfig;
class AMAudioAlertConfig
{
  public:
    AMAudioAlertConfig();
    virtual ~AMAudioAlertConfig();
    AudioAlertConfig* get_config(const std::string& config);
    bool set_config(AudioAlertConfig *alert_config, const std::string& config);

  private:
    AMConfig         *m_config;
    AudioAlertConfig *m_alert_config;
};

#endif
