/*******************************************************************************
 * am_playback.h
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
#ifndef AM_PLAYBACK_H_
#define AM_PLAYBACK_H_

#include "am_playback_if.h"
#include <atomic>
#include <mutex>

class AMIPlaybackEngine;
class AMPlayback: public AMIPlayback
{
  public:
    static AMIPlayback* get_instance();

  public:
    virtual bool init();
    virtual bool reload();
    virtual void set_msg_callback(AMPlaybackCallback callback, void *data);
    virtual bool is_playing();
    virtual bool is_paused();

  public:
    virtual bool add_uri(const AMPlaybackUri& uri);
    virtual bool play(AMPlaybackUri *uri = NULL);
    virtual bool pause(bool enable);
    virtual bool stop();

  protected:
    virtual void release();
    virtual void inc_ref();

  private:
    explicit AMPlayback();
    virtual ~AMPlayback();

  private:
    AMIPlaybackEngine *m_engine;
    bool               m_is_initialized;
    std::atomic_int    m_ref_count;
};

#endif /* AM_PLAYBACK_H_ */
