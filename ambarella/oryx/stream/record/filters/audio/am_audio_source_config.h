/*******************************************************************************
 * am_audio_source_config.h
 *
 * History:
 *   2014-12-2 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
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
    std::string  audio_profile;
    std::string  interface;
    AMAudioMap   audio_type_map;
    AudioSourceConfig() :
      audio_type_num(0),
      initial_volume(90),
      enable_aec(true)
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
