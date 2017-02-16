/*******************************************************************************
 * am_playback_interface.h
 *
 * History:
 *   2014-7-25 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_PLAYBACK_ENGINE_IF_H_
#define AM_PLAYBACK_ENGINE_IF_H_

#include "am_playback_msg.h"
#include "am_playback_info.h"
extern const AM_IID IID_AMIPlaybackEngine;

class AMIPlaybackEngine: public AMIInterface
{
  public:
    enum AM_PLAYBACK_ENGINE_STATUS {
      AM_PLAYBACK_ENGINE_ERROR,
      AM_PLAYBACK_ENGINE_TIMEOUT,
      AM_PLAYBACK_ENGINE_STARTING,
      AM_PLAYBACK_ENGINE_PLAYING,
      AM_PLAYBACK_ENGINE_PAUSING,
      AM_PLAYBACK_ENGINE_PAUSED,
      AM_PLAYBACK_ENGINE_STOPPING,
      AM_PLAYBACK_ENGINE_STOPPED
    };

    static AMIPlaybackEngine* create();

  public:
    DECLARE_INTERFACE(AMIPlaybackEngine, IID_AMIPlaybackEngine);
    virtual AM_PLAYBACK_ENGINE_STATUS get_engine_status() = 0;
    virtual bool create_graph()                           = 0;
    virtual bool add_uri(const AMPlaybackUri& uri)        = 0;
    virtual bool play(AMPlaybackUri* uri = NULL)          = 0;
    virtual bool stop()                                   = 0;
    virtual bool pause(bool enabled)                      = 0;
    virtual void set_app_msg_callback(AMPlaybackCallback callback,
                                      void *data)         = 0;
};

#endif /* AM_PLAYBACK_ENGINE_IF_H_ */
