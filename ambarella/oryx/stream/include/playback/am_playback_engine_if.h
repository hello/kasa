/*******************************************************************************
 * am_playback_interface.h
 *
 * History:
 *   2014-7-25 - [ypchang] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
      AM_PLAYBACK_ENGINE_STOPPED,
      AM_PLAYBACK_ENGINE_IDLE,
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
    virtual void set_aec_enabled(bool enabled)            = 0;
};

#endif /* AM_PLAYBACK_ENGINE_IF_H_ */
