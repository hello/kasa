/**
 * am_av_queue_config.h
 *
 *  History:
 *		Dec 29, 2014 - [Shupeng Ren] created file
 *
 * Copyright (C) 2007-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef _AM_AV_QUEUE_CONFIG_H_
#define _AM_AV_QUEUE_CONFIG_H_

#include "am_filter_config.h"
#include "am_audio_define.h"
struct AVQueueConfig: public AMFilterConfig
{
    bool     event_enable;
    uint32_t event_stream_id;
    uint32_t event_history_duration;
    uint32_t event_future_duration;
    AM_AUDIO_TYPE event_audio_type;
    AVQueueConfig() :
      event_enable(false),
      event_stream_id(0),
      event_history_duration(0),
      event_future_duration(0),
      event_audio_type(AM_AUDIO_AAC)
    {}
};

class AMConfig;

class AMAVQueueConfig
{
  public:
    AMAVQueueConfig();
    virtual ~AMAVQueueConfig();
    AVQueueConfig* get_config(const std::string& config);

  private:
    AMConfig          *m_config;
    AVQueueConfig     *m_avqueue_config;
};

#endif /* _AM_AV_QUEUE_CONFIG_H_ */
