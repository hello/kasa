/*******************************************************************************
 * am_audio_source_config.h
 *
 * History:
 *   2014-12-2 - [ypchang] created file
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
#ifndef AM_AUDIO_SOURCE_CONFIG_H_
#define AM_AUDIO_SOURCE_CONFIG_H_

#include "am_filter_config.h"
#include <map>
#include <vector>

typedef std::vector<std::string> AMStringList;
typedef std::map<std::string, AMStringList*> AMAudioMap;

struct AudioSourceConfig: public AMFilterConfig
{
    uint32_t     audio_type_num;
    int32_t      initial_volume;
    bool         enable_aec;
    bool         codec_enable;
    std::string  audio_profile;
    std::string  interface;
    AMAudioMap   audio_type_map;
    AudioSourceConfig() :
      audio_type_num(0),
      initial_volume(90),
      enable_aec(true),
      codec_enable(true)
    {
      audio_profile.clear();
      interface.clear();
    }
    ~AudioSourceConfig()
    {
      for (AMAudioMap::iterator iter = audio_type_map.begin();
          iter != audio_type_map.end();
          ++ iter) {
        delete iter->second;
      }
    }
};

class AMConfig;
class AMAudioSource;
class AMAudioSourceConfig
{
    friend class AMAudioSource;
  public:
    AMAudioSourceConfig();
    virtual ~AMAudioSourceConfig();
    AudioSourceConfig* get_config(const std::string& conf_file);

  private:
    AMConfig          *m_config;
    AudioSourceConfig *m_audio_source_config;
};

#endif /* AM_AUDIO_SOURCE_CONFIG_H_ */
