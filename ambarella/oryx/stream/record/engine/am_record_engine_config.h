/*******************************************************************************
 * am_record_engine_config.h
 *
 * History:
 *   2014-12-30 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_STREAM_RECORD_ENGINE_AM_RECORD_ENGINE_CONFIG_H_
#define ORYX_STREAM_RECORD_ENGINE_AM_RECORD_ENGINE_CONFIG_H_

#include "am_engine_config.h"

class AMConfig;
class AMRecordEngine;
class AMRecordEngineConfig
{
    friend class AMRecordEngine;
  public:
    AMRecordEngineConfig();
    virtual ~AMRecordEngineConfig();
    EngineConfig* get_config(const std::string &conf_file);

  private:
    AMConfig     *m_config;
    EngineConfig *m_engine_config;
};

#endif /* ORYX_STREAM_RECORD_ENGINE_AM_RECORD_ENGINE_CONFIG_H_ */
