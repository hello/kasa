/*******************************************************************************
 * am_playback_engine.h
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
#ifndef AM_PLAYBACK_ENGINE_H_
#define AM_PLAYBACK_ENGINE_H_

#include "am_mutex.h"

struct EngineConfig;
struct EngineFilter;
struct ConnectionConfig;

class AMPlaybackEngineConfig;
class AMThread;
class AMEvent;

class AMPlaybackEngine: public AMEngineFrame, public AMIPlaybackEngine
{
    typedef AMEngineFrame inherited;

    enum AM_PLAYBACK_ENGINE_CMD
    {
      AM_ENGINE_CMD_START = 's',
      AM_ENGINE_CMD_STOP  = 'q',
      AM_ENGINE_CMD_ABORT = 'a',
      AM_ENGINE_CMD_PAUSE = 'p',
      AM_ENGINE_CMD_EXIT  = 'e',
    };

  public:
    static AMPlaybackEngine* create(const std::string& config);

  public:
    virtual void* get_interface(AM_REFIID ref_iid);
    virtual void destroy();

  public:
    virtual AMIPlaybackEngine::AM_PLAYBACK_ENGINE_STATUS get_engine_status();
    virtual bool create_graph();
    virtual bool add_uri(const AMPlaybackUri& uri);
    virtual bool play(AMPlaybackUri *uri = NULL);
    virtual bool stop();
    virtual bool pause(bool enabled);
    virtual void set_app_msg_callback(AMPlaybackCallback callback, void *data);
    virtual void set_aec_enabled(bool enabled);

  private:
    static void static_app_msg_callback(void *context, AmMsg& msg);
    void app_msg_callback(void *context, AmMsg& msg);
    virtual void msg_proc(AmMsg& msg);
    bool change_engine_status(
        AMIPlaybackEngine::AM_PLAYBACK_ENGINE_STATUS targetStatus,
        AMPlaybackUri* uri = NULL);

  private:
    AMPlaybackEngine();
    virtual ~AMPlaybackEngine();
    AM_STATE init(const std::string& config);
    AM_STATE load_all_filters();
    AMIPacketFilter* get_filter_by_name(std::string& name);
    ConnectionConfig* get_connection_conf_by_name(std::string& name);
    void* get_filter_by_iid(AM_REFIID iid);
    static void static_mainloop(void *data);
    void mainloop();
    bool send_engine_cmd(AM_PLAYBACK_ENGINE_CMD cmd, bool block = true);

  private:
    AMPlaybackEngineConfig   *m_config;
    EngineConfig             *m_engine_config; /* No need to delete */
    EngineFilter             *m_engine_filter;
    void                     *m_app_data;
    AMThread                 *m_thread;
    AMEvent                  *m_event;
    AMPlaybackUri            *m_uri;
    AMPlaybackCallback        m_app_callback;
    AM_PLAYBACK_ENGINE_STATUS m_status;
    int                       m_msg_ctrl[2];
    bool                      m_graph_created;
    bool                      m_mainloop_run;
    AMMemLock                 m_lock;
    AMPlaybackMsg             m_app_msg;
#define MSG_R m_msg_ctrl[0]
#define MSG_W m_msg_ctrl[1]
};

#endif /* AM_PLAYBACK_ENGINE_H_ */
