/*
 * am_key_input_config.cpp
 *
 * Histroy:
 *  2014-11-19 [Dongge Wu] Create file
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_key_input_config.h"

AMKeyInputConfig::AMKeyInputConfig() :
    m_config(NULL),
    m_key_input_config(NULL)
{

}

AMKeyInputConfig::~AMKeyInputConfig()
{
  delete m_config;
  delete m_key_input_config;

}

KeyInputConfig* AMKeyInputConfig::get_config(const std::string& config)
{
  do {
    delete m_config;
    m_config = NULL;
    m_config = AMConfig::create(config.c_str());
    if (!m_config) {
      ERROR("Create AMConfig failed!\n");
      break;
    }

    if (!m_key_input_config) {
      m_key_input_config = new KeyInputConfig();
      if (!m_key_input_config) {
        ERROR("new key input config failed!\n");
        break;
      }
    }

    AMConfig &key_input = *m_config;
    if (key_input["long_press_time"].exists()) {
      m_key_input_config->long_press_time =
          key_input["long_press_time"].get<int>(2000);
    } else {
      WARN("long_press_time is not specified, use default!\n");
      m_key_input_config->long_press_time = 2000;
    }
  } while (0);

  return m_key_input_config;
}

bool AMKeyInputConfig::set_config(KeyInputConfig *key_input_config,
                                  const std::string& config)
{
  bool ret = true;
  do {
    delete m_config;
    m_config = NULL;
    m_config = AMConfig::create(config.c_str());
    if (!m_config) {
      ERROR("Create AMConfig failed!\n");
      ret = false;
      break;
    }

    if (!m_key_input_config) {
      m_key_input_config = new KeyInputConfig();
      if (!m_key_input_config) {
        ERROR("new key input config failed!\n");
        ret = false;
        break;
      }
    }

    memcpy(m_key_input_config, key_input_config, sizeof(KeyInputConfig));
    AMConfig &key_input = *m_config;
    if (key_input["long_press_time"].exists()) {
      key_input["long_press_time"] =
          m_key_input_config->long_press_time;
    }

    key_input.save();
  } while (0);

  return ret;
}
