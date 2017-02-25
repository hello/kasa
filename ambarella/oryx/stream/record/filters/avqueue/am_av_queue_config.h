/**
 * am_av_queue_config.h
 *
 *  History:
 *		Dec 29, 2014 - [Shupeng Ren] created file
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
#ifndef _AM_AV_QUEUE_CONFIG_H_
#define _AM_AV_QUEUE_CONFIG_H_

#include "am_filter_config.h"
#include "am_audio_define.h"

using std::map;
using std::string;
struct AVQueueEventConfig
{
    uint32_t      video_id         = 0;
    AM_AUDIO_TYPE audio_type       = AM_AUDIO_AAC;
    uint32_t      history_duration = 0;
    uint32_t      future_duration  = 0;
    bool          enable           = false;
};

typedef map<uint32_t, AVQueueEventConfig> AVQueueEventConfigMap;
struct AVQueueConfig: public AMFilterConfig
{
    AVQueueEventConfigMap event_config;
};

class AMConfig;

class AMAVQueueConfig
{
  public:
    AMAVQueueConfig();
    virtual ~AMAVQueueConfig();
    AVQueueConfig* get_config(const string& config);

  private:
    AMConfig          *m_config;
    AVQueueConfig     *m_avqueue_config;
};

#endif /* _AM_AV_QUEUE_CONFIG_H_ */
