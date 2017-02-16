/*******************************************************************************
 * am_player_config.h
 *
 * History:
 *   2014-9-11 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_PLAYER_CONFIG_H_
#define AM_PLAYER_CONFIG_H_

#include "am_filter_config.h"

struct AudioPlayerConfig
{
  std::string interface;
  uint32_t def_channel_num;
  uint32_t buffer_delay_ms;
  int32_t  initial_volume;
  bool     enable_aec;
  AudioPlayerConfig() :
    interface("pulse"),
    def_channel_num(2),
    buffer_delay_ms(100),
    initial_volume(75),
    enable_aec(true)
  {}
  AudioPlayerConfig(const AudioPlayerConfig &config) :
    interface(config.interface),
    def_channel_num(config.def_channel_num),
    buffer_delay_ms(config.buffer_delay_ms),
    initial_volume(config.initial_volume),
    enable_aec(config.enable_aec)
  {}
  ~AudioPlayerConfig(){}
};

struct PlayerConfig: public AMFilterConfig
{
    uint32_t pause_check_interval;
    AudioPlayerConfig audio;
    PlayerConfig() :
     pause_check_interval(0)
    {}
    ~PlayerConfig(){}
};

class AMConfig;
class AMPlayer;
class AMPlayerConfig
{
    friend class AMPlayer;
  public:
    AMPlayerConfig();
    virtual ~AMPlayerConfig();
    PlayerConfig* get_config(const std::string& config);

  private:
    AMConfig     *m_config;
    PlayerConfig *m_player_config;
};

#endif /* AM_PLAYER_CONFIG_H_ */
