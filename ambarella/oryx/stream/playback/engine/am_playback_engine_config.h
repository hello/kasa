/*******************************************************************************
 * am_playback_engine_config.h
 *
 * History:
 *   2014-8-15 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_PLAYBACK_ENGINE_CONFIG_H_
#define AM_PLAYBACK_ENGINE_CONFIG_H_

#include "am_engine_config.h"

class AMConfig;
class AMPlaybackEngine;
class AMPlaybackEngineConfig
{
    friend class AMPlaybackEngine;
  public:
    AMPlaybackEngineConfig();
    virtual ~AMPlaybackEngineConfig();
    EngineConfig* get_config(const std::string &config_file);

  private:
    AMConfig     *m_config;
    EngineConfig *m_engine_config;
};

#endif /* AM_PLAYBACK_ENGINE_CONFIG_H_ */
