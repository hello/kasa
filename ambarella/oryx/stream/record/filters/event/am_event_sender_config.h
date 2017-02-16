/**
 * am_event_sender_config.h
 *
 *  History:
 *    Mar 6, 2015 - [Shupeng Ren] created file
 *
 * Copyright (C) 2007-2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef _AM_EVENT_SENDER_CONFIG_H_
#define _AM_EVENT_SENDER_CONFIG_H_

#include "am_filter_config.h"

struct EventSenderConfig: public AMFilterConfig
{
    int32_t event_type;
    EventSenderConfig() :
      event_type(0)
    {}
};

class AMConfig;
class AMEventSenderConfig
{
  public:
    AMEventSenderConfig();
    virtual ~AMEventSenderConfig();
    EventSenderConfig* get_config(const std::string& conf_file);

  private:
    AMConfig            *m_config;
    EventSenderConfig   *m_event_sender_config;
};

#endif /* _AM_EVENT_SENDER_CONFIG_H_ */
