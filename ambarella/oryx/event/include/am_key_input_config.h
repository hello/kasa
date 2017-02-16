/*
 * am_key_input_config.h
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

#ifndef AM_KEY_INPUT_CONFIG_H_
#define AM_KEY_INPUT_CONFIG_H_

#include "am_configure.h"

struct KeyInputConfig
{
    int32_t long_press_time;
};

class AMConfig;
class AMKeyInputConfig
{
  public:
    AMKeyInputConfig();
    virtual ~AMKeyInputConfig();
    KeyInputConfig* get_config(const std::string& config);
    bool set_config(KeyInputConfig *key_input_config,
                    const std::string& config);

  private:
    AMConfig *m_config;
    KeyInputConfig *m_key_input_config;
};

#endif /* AM_KEY_INPUT_CONFIG_H_ */
