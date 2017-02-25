/*******************************************************************************
 * am_playback_engine.cpp
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

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_amf_types.h"
#include "am_amf_interface.h"
#include "am_amf_queue.h"
#include "am_amf_base.h"
#include "am_amf_engine_frame.h"
#include "am_playback_info.h"
#include "am_playback_engine_if.h"
#include "am_playback_engine.h"
#include "am_playback_engine_config.h"

#include "am_file_demuxer_if.h"
#include "am_audio_decoder_if.h"
#include "am_player_if.h"

#include "am_event.h"
#include "am_thread.h"
#include "am_plugin.h"

#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>

AMIPlaybackEngine* AMIPlaybackEngine::create()
{
  std::string conf(ORYX_CONF_DIR);
  conf.append("/stream/engine/playback-engine.acs");
  return (AMIPlaybackEngine*)AMPlaybackEngine::create(conf);
}

AMPlaybackEngine* AMPlaybackEngine::create(const std::string& config)
{
  AMPlaybackEngine *result = new AMPlaybackEngine();
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(config)))) {
    delete result;
    result = nullptr;
  }

  return result;
}

void* AMPlaybackEngine::get_interface(AM_REFIID ref_iid)
{
  return (IID_AMIPlaybackEngine == ref_iid) ? ((AMIPlaybackEngine*)this) :
      inherited::get_interface(ref_iid);
}

void AMPlaybackEngine::destroy()
{
  inherited::destroy();
}

AMIPlaybackEngine::AM_PLAYBACK_ENGINE_STATUS
AMPlaybackEngine::get_engine_status()
{
  AUTO_MEM_LOCK(m_lock);
  return m_status;
}

bool AMPlaybackEngine::create_graph()
{
  if (AM_LIKELY(!m_graph_created)) {
    bool ret = true;
    for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
      ConnectionConfig& connection = m_engine_config->connections[i];
      AMIPacketFilter *this_filter = get_filter_by_name(connection.filter);
      if (AM_LIKELY(this_filter)) {
        for (uint32_t j = 0; j < connection.input_number; ++ j) {
          /* This filter has up stream filter[s] */
          ConnectionConfig *&in_conn = connection.input[j];
          AMIPacketFilter *in_filter = get_filter_by_name(in_conn->filter);
          if (AM_LIKELY(in_filter)) {
            AMIPacketPin  *in_pin = this_filter->get_input_pin(j);
            AMIPacketPin *out_pin = nullptr;
            uint32_t k = 0;
            for (k = 0; k < in_conn->output_number; ++ k) {
              out_pin = in_filter->get_output_pin(k);
              if (AM_LIKELY(out_pin && !out_pin->is_connected())) {
                break;
              } else {
                out_pin = nullptr;
              }
            }
            if (AM_LIKELY(out_pin && in_pin)) {
              NOTICE("Filter[%20s] output pin[%u] ==> "
                     "Filter[%20s] input pin[%u]",
                     out_pin->filter_name(), k,
                     in_pin->filter_name(), j);
              ret = (AM_STATE_OK == create_connection(out_pin, in_pin));
            } else {
              if (AM_LIKELY(!out_pin)) {
                ERROR("Failed to get an available output pin from %s",
                      in_conn->filter.c_str());
              }
              if (AM_LIKELY(!in_pin)) {
                ERROR("Failed to get input pin[%u] from %s",
                      j, connection.filter.c_str());
              }
              ret = false;
            }
          } else {
            ERROR("Failed to get AMIPacketFilter interface from %s",
                  in_conn->filter.c_str());
            ret = false;
          }
          if (AM_UNLIKELY(!ret)) {
            break;
          }
        }
        if (AM_UNLIKELY(!ret)) {
          break;
        }
      } else {
        ERROR("Failed to get AMIPacketFilter interface from %s",
              connection.filter.c_str());
        ret = false;
        break;
      }
    }
    m_graph_created = ret;
    if (AM_LIKELY(m_graph_created)) {
      m_thread = AMThread::create("PlaybackENGLoop",
                                  static_mainloop,
                                  this);
      if (AM_UNLIKELY(!m_thread)) {
        delete_all_connections();
        remove_all_filters();
        m_graph_created = false;
      }
    }
  }

  return m_graph_created;
}

bool AMPlaybackEngine::add_uri(const AMPlaybackUri& uri)
{
  AUTO_MEM_LOCK(m_lock);
  bool ret = false;
  AMIFileDemuxer *demuxer =
      (AMIFileDemuxer*)get_filter_by_iid(IID_AMIFileDemuxer);
  //INFO("Add URI: %s", uri.c_str());
  if (AM_LIKELY(demuxer)) {
    ret = (AM_STATE_OK == demuxer->add_media(uri));
    if (ret && (m_status == AM_PLAYBACK_ENGINE_IDLE)) {
      NOTICE("Add uri, engine status is idle, change it into playing");
      m_status = AM_PLAYBACK_ENGINE_PLAYING;
    }
  } else {
    ERROR("Can NOT find filter IID_AMIFileDemuxer!");
  }

  return ret;
}

bool AMPlaybackEngine::play(AMPlaybackUri *uri)
{
  if(uri != NULL) {
    if(m_uri == NULL) {
      if(AM_UNLIKELY((m_uri = new AMPlaybackUri()) == NULL)) {
        ERROR("Failed to malloc memory for m_uri.");
        return false;
      }
    }
    memcpy(m_uri, uri, sizeof(AMPlaybackUri));
  } else {
    if(m_uri != NULL) {
      delete m_uri;
      m_uri = NULL;
    }
  }
  return send_engine_cmd(AM_ENGINE_CMD_START);
}

bool AMPlaybackEngine::stop()
{
  return send_engine_cmd(AM_ENGINE_CMD_STOP);
}

bool AMPlaybackEngine::pause(bool enabled)
{
  return enabled ? send_engine_cmd(AM_ENGINE_CMD_PAUSE) :
      send_engine_cmd(AM_ENGINE_CMD_START);
}

void AMPlaybackEngine::set_app_msg_callback(AMPlaybackCallback callback,
                                            void *data)
{
  m_app_callback = callback;
  m_app_data     = data;
}

void AMPlaybackEngine::set_aec_enabled(bool enabled)
{
  AMIPlayer *player = (AMIPlayer*)get_filter_by_iid(IID_AMIPlayer);
  if (AM_LIKELY(player)) {
    player->set_aec_enabled(enabled);
  }
}

void AMPlaybackEngine::static_app_msg_callback(void *context, AmMsg& msg)
{
  ((AMPlaybackEngine*)context)->app_msg_callback(
      ((AMPlaybackEngine*)context)->m_app_data, msg);
}

void AMPlaybackEngine::app_msg_callback(void *context, AmMsg& msg)
{
  if (AM_LIKELY(m_app_callback)) {
    m_app_msg.data = context;
    m_app_msg.msg  = AM_PLAYBACK_MSG(msg.code);
    m_app_callback(m_app_msg);
  }
}

void AMPlaybackEngine::msg_proc(AmMsg& msg)
{
  if (AM_LIKELY(is_session_msg(msg))) {
    switch(msg.code) {
      case ENG_MSG_ERROR: {
        m_status = AM_PLAYBACK_ENGINE_ERROR;
        post_app_msg(AM_PLAYBACK_MSG_ERROR);
      }break;
      case ENG_MSG_EOS: {
        if (AM_LIKELY(m_status == AM_PLAYBACK_ENGINE_STOPPING)) {
          post_app_msg(AM_PLAYBACK_MSG_STOP_OK);
        }
        m_status = AM_PLAYBACK_ENGINE_STOPPED;
        post_app_msg(AM_PLAYBACK_MSG_EOS);
      }break;
      case ENG_MSG_ABORT: {
        m_status = AM_PLAYBACK_ENGINE_ERROR;
        send_engine_cmd(AM_ENGINE_CMD_ABORT);
        post_app_msg(AM_PLAYBACK_MSG_ABORT);
      }break;
      case ENG_MSG_OK: {
        switch(m_status) {
          case AM_PLAYBACK_ENGINE_STARTING: {
            m_status = AM_PLAYBACK_ENGINE_PLAYING;
            post_app_msg(AM_PLAYBACK_MSG_START_OK);
          }break;
          case AM_PLAYBACK_ENGINE_PAUSING: {
            m_status = AM_PLAYBACK_ENGINE_PAUSED;
            post_app_msg(AM_PLAYBACK_MSG_PAUSE_OK);
          }break;
          case AM_PLAYBACK_ENGINE_STOPPING: {
            m_status = AM_PLAYBACK_ENGINE_STOPPED;
            post_app_msg(AM_PLAYBACK_MSG_STOP_OK);
          }break;
          case AM_PLAYBACK_ENGINE_PLAYING: {
            INFO("New file being played!");
          }break;
          default: break;
        }
      }break;
      case ENG_MSG_EOF: {
        post_app_msg(AM_PLAYBACK_MSG_EOF);
      }break;
      case ENG_MSG_EOL: {
        m_status = AM_PLAYBACK_ENGINE_IDLE;
        post_app_msg(AM_PLAYBACK_MSG_IDLE);
      }break;
      default: {
        ERROR("Invalid message!");
        post_app_msg(AM_PLAYBACK_MSG_NULL);
      }break;
    }
  }
}

bool AMPlaybackEngine::change_engine_status(
        AMIPlaybackEngine::AM_PLAYBACK_ENGINE_STATUS target_status,
        AMPlaybackUri* uri)
{
  bool ret = true;

  DEBUG("Target status is %u", target_status);
   do {
    switch(m_status) {
      case AM_PLAYBACK_ENGINE_ERROR: {
        stop_all_filters();
        purge_all_filters();
        m_status = AM_PLAYBACK_ENGINE_STOPPED;
        ret = false;
      }break;
      case AM_PLAYBACK_ENGINE_TIMEOUT: {
        post_app_msg(AM_PLAYBACK_MSG_TIMEOUT);
        m_status = AM_PLAYBACK_ENGINE_STOPPED;
        ret = false;
      }break;
      case AM_PLAYBACK_ENGINE_IDLE : {
        AMIPlayer *player = (AMIPlayer*)get_filter_by_iid(IID_AMIPlayer);
        if (AM_LIKELY(player)) {
          switch (target_status) {
            case AM_PLAYBACK_ENGINE_STOPPED : {
              m_status = AM_PLAYBACK_ENGINE_STOPPING;
              /* Just make sure "ret" is true,
               * player will post engine message,
               * let engine message handler change engine status
               */
              ret = (AM_STATE_OK == player->stop()) || true;
            } break;
            default : {
              ERROR("Invalid operation when engine is idle!");
              ret = false;
            } break;
          }
        } else {
          ERROR("Failed to get filter(IID_AMIPlayer)!");
          ret = false;
        }
      } break;
      case AM_PLAYBACK_ENGINE_PLAYING: { /* Engine status is playing */
        AMIPlayer *player = (AMIPlayer*)get_filter_by_iid(IID_AMIPlayer);
        if (AM_LIKELY(player)) {
          switch(target_status) {
            case AM_PLAYBACK_ENGINE_PAUSED: {
              m_status = AM_PLAYBACK_ENGINE_PAUSING;
              /* Just make sure "ret" is true,
               * player will post engine message,
               * let engine message handler change engine status
               */
              ret = (AM_STATE_OK == player->pause(true)) || true;
            }break;
            case AM_PLAYBACK_ENGINE_STOPPED: {
              m_status = AM_PLAYBACK_ENGINE_STOPPING;
              /* Just make sure "ret" is true,
               * player will post engine message,
               * let engine message handler change engine status
               */
              ret = (AM_STATE_OK == player->stop()) || true;
            }break;
            case AM_PLAYBACK_ENGINE_PLAYING: {
              NOTICE("Already playing!");
            }break;
            default: {
              ERROR("Invalid operation when engine is playing!");
              ret = false;
            }break;
          }
        } else {
          ERROR("Failed to get filter(IID_AMIPlayer)!");
          ret = false;
        }
      }break;
      case AM_PLAYBACK_ENGINE_PAUSED: { /* Engine status is paused */
        AMIPlayer *player = (AMIPlayer*)get_filter_by_iid(IID_AMIPlayer);
        if (AM_LIKELY(player)) {
          switch(target_status) {
            case AM_PLAYBACK_ENGINE_PLAYING: {
              if (AM_LIKELY(uri != NULL)) {
                /* New file is added, need to stop first */
                m_status = AM_PLAYBACK_ENGINE_STOPPING;
                ret = (AM_STATE_OK == player->stop()) || true;
              } else {
                m_status = AM_PLAYBACK_ENGINE_STARTING;
                ret = (AM_STATE_OK == player->pause(false)) || true;
              }
            }break;
            case AM_PLAYBACK_ENGINE_STOPPED: {
              m_status = AM_PLAYBACK_ENGINE_STOPPING;
              ret = (AM_STATE_OK == player->stop()) || true;
            }break;
            case AM_PLAYBACK_ENGINE_PAUSED: {
              NOTICE("Already paused!");
            }break;
            default: {
              ERROR("Invalid operation when engine is paused!");
              ret = false;
            }break;
          }
        } else {
          ERROR("Failed to get filter(IID_AMIPlayer)!");
          ret = false;
        }
      }break;
      case AM_PLAYBACK_ENGINE_STOPPED: { /* Engine status is stopped */
        switch(target_status) {
          case AM_PLAYBACK_ENGINE_PLAYING: {
            m_status = AM_PLAYBACK_ENGINE_STARTING;
            if (AM_LIKELY(AM_STATE_OK == run_all_filters())) {
              AMIFileDemuxer *demuxer =
                  (AMIFileDemuxer*)get_filter_by_iid(IID_AMIFileDemuxer);
              if (AM_LIKELY(demuxer)) {
                ret = (AM_STATE_OK == demuxer->play(uri));
                if (AM_UNLIKELY(!ret)) {
                  m_status = AM_PLAYBACK_ENGINE_ERROR;
                  ret = true;
                }
              } else {
                ERROR("Failed to get filter(IID_AMIFileDemuxer)!");
                m_status = AM_PLAYBACK_ENGINE_STOPPED;
                ret = false;
              }
            } else {
              ERROR("Failed to run all filters!");
              m_status = AM_PLAYBACK_ENGINE_ERROR;
              ret = true;
            }
          }break;
          case AM_PLAYBACK_ENGINE_STOPPED: {
            NOTICE("Already stopped!");
          }break;
          default: {
            ERROR("Invalid operation when engine is stopped!");
            m_status = AM_PLAYBACK_ENGINE_ERROR;
          }break;
        }
      }break;
      /* Intermediate status */
      case AM_PLAYBACK_ENGINE_STARTING: {
        uint32_t count = 0;
        do {
          if (AM_LIKELY((m_status == AM_PLAYBACK_ENGINE_PLAYING) ||
                        (m_status == AM_PLAYBACK_ENGINE_ERROR))) {
            if (AM_UNLIKELY(m_status == AM_PLAYBACK_ENGINE_ERROR)) {
              ERROR("Failed to start playing!");
            }
            break;
          }
          usleep(100000);
          ++ count;
        } while(count < m_engine_config->op_timeout*10);
        if (AM_UNLIKELY((count >= m_engine_config->op_timeout*10) &&
                        (m_status == AM_PLAYBACK_ENGINE_STARTING))) {
          ERROR("START operation timed out!");
          m_status = AM_PLAYBACK_ENGINE_TIMEOUT;
        }
      }break;
      case AM_PLAYBACK_ENGINE_PAUSING: {
        uint32_t count = 0;
        do {
          if (AM_LIKELY((m_status == AM_PLAYBACK_ENGINE_PAUSED) ||
                        (m_status == AM_PLAYBACK_ENGINE_ERROR))) {
            if (AM_UNLIKELY(m_status == AM_PLAYBACK_ENGINE_ERROR)) {
              ERROR("Failed to pause!");
            }
            break;
          }
          usleep(50000);
          ++ count;
        } while(count < m_engine_config->op_timeout*20);
        if (AM_UNLIKELY((count >= m_engine_config->op_timeout*20) &&
                        (m_status == AM_PLAYBACK_ENGINE_PAUSING))) {
          ERROR("PAUSE operation timed out!");
          m_status = AM_PLAYBACK_ENGINE_TIMEOUT;
        }
      }break;
      case AM_PLAYBACK_ENGINE_STOPPING: {
        uint32_t count = 0;
        do {
          if (AM_LIKELY((m_status == AM_PLAYBACK_ENGINE_STOPPED) ||
                        (m_status == AM_PLAYBACK_ENGINE_ERROR))) {
            if (AM_UNLIKELY(m_status == AM_PLAYBACK_ENGINE_ERROR)) {
              ERROR("Failed to stop!");
            }
            break;
          }
          usleep(50000);
          ++ count;
        } while(count < m_engine_config->op_timeout*20);
        if (AM_UNLIKELY((count >= m_engine_config->op_timeout*20) &&
                        (m_status == AM_PLAYBACK_ENGINE_STOPPING))) {
          ERROR("STOP operation timed out!");
          m_status = AM_PLAYBACK_ENGINE_TIMEOUT;
        }
      }break;
      default: {
        ret = false;
        ERROR("Invalid engine status!");
      }break;
    }
    if (AM_LIKELY(m_status == AM_PLAYBACK_ENGINE_STOPPED)) {
      /* Player is fully stopped
       * now stop all filters and purge them all
       */
      stop_all_filters();
      purge_all_filters();
    }
  } while(ret && (m_status != target_status));

  if (AM_LIKELY(ret && (m_status == target_status))) {
    DEBUG("Target %u finished!", target_status);
  } else {
    DEBUG("Current status %u, target status %u", m_status, target_status);
  }

  return ret;
}

AMPlaybackEngine::AMPlaybackEngine() :
    m_config(nullptr),
    m_engine_config(nullptr),
    m_engine_filter(nullptr),
    m_app_data(nullptr),
    m_thread(nullptr),
    m_event(nullptr),
    m_uri(nullptr),
    m_app_callback(nullptr),
    m_status(AMIPlaybackEngine::AM_PLAYBACK_ENGINE_STOPPED),
    m_graph_created(false),
    m_mainloop_run(false)
{
  MSG_R = -1;
  MSG_W = -1;
}

AMPlaybackEngine::~AMPlaybackEngine()
{
  send_engine_cmd(AM_ENGINE_CMD_EXIT);
  clear_graph(); /* Must be called in the destructor of sub-class */
  delete m_config;
  delete[] m_engine_filter;
  delete m_uri;
  AM_DESTROY(m_thread);
  AM_DESTROY(m_event);
  if (AM_LIKELY(MSG_R >= 0)) {
    close(MSG_R);
  }
  if (AM_LIKELY(MSG_W >= 0)) {
    close(MSG_W);
  }
  DEBUG("~AMPlaybackEngine");
}

AM_STATE AMPlaybackEngine::init(const std::string& config)
{
  AM_STATE state = AM_STATE_ERROR;

  do {
    if (AM_UNLIKELY(socketpair(AF_UNIX, SOCK_STREAM, IPPROTO_IP,
                               m_msg_ctrl) < 0)) {
      PERROR("socketpair");
      state = AM_STATE_ERROR;
      break;
    }
    m_event = AMEvent::create();
    if (AM_UNLIKELY(nullptr == m_event)) {
      ERROR("Failed to create event!");
      break;
    }
    m_config = new AMPlaybackEngineConfig();
    if (AM_UNLIKELY(nullptr == m_config)) {
      ERROR("Failed to create config module for playback engine!");
      state = AM_STATE_NO_MEMORY;
      break;
    }
    m_engine_config = m_config->get_config(config);
    if (AM_UNLIKELY(!m_engine_config)) {
      ERROR("Can not get configuration from file %s, please check!",
            config.c_str());
      break;
    } else {
      uint32_t connection_num = 0;
      for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
        connection_num += m_engine_config->connections[i].input_number;
      }
      state = inherited::init(m_engine_config->filter_num, connection_num);
      if (AM_UNLIKELY(AM_STATE_OK != state)) {
        ERROR("Failed to initialize base engine!");
        break;
      }
      state = load_all_filters();
      if (AM_UNLIKELY(AM_STATE_OK != state)) {
        ERROR("Failed to load filters needed by Playback Engine!");
        break;
      } else {
        for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
          AMIPacketFilter *filter = (AMIPacketFilter*)
              m_engine_filter[i].filter_obj->get_interface(IID_AMIPacketFilter);
          if (AM_UNLIKELY(AM_STATE_OK !=
              (state = add_filter(filter, m_engine_filter[i].so)))) {
            break;
          }
        }
        if (AM_UNLIKELY(AM_STATE_OK != state)) {
          break;
        }
      }
      state = inherited::set_app_msg_callback(static_app_msg_callback, this);
    }
  }while(0);

  return state;
}

AM_STATE AMPlaybackEngine::load_all_filters()
{
  AM_STATE state = AM_STATE_OK;

  delete[] m_engine_filter;
  m_engine_filter = new EngineFilter[m_engine_config->filter_num];
  if (AM_LIKELY(m_engine_filter)) {
    for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
      std::string filter = ORYX_FILTER_DIR;
      std::string filter_conf = ORYX_CONF_DIR;

      m_engine_filter[i].filter = m_engine_config->filters[i];
      filter.append("/filter-").append(m_engine_filter[i].filter).
          append(".so");
      filter_conf.append("/stream/filter/filter-").
          append(m_engine_filter[i].filter).append(".acs");
      m_engine_filter[i].so = AMPlugin::create(filter.c_str());

      if (AM_LIKELY(m_engine_filter[i].so)) {
        ConnectionConfig *con =
            get_connection_conf_by_name(m_engine_filter[i].filter);
        GetFilterObject create_filter =
            (GetFilterObject)m_engine_filter[i].so->get_symbol(GET_FILTER_OBJ);
        if (AM_LIKELY(create_filter)) {
          m_engine_filter[i].filter_obj = create_filter(this,
                                                        filter_conf.c_str(),
                                                        con->input_number,
                                                        con->output_number);
          DEBUG("Filter %s is at %p", m_engine_filter[i].filter.c_str(),
                m_engine_filter[i].filter_obj);
          if (AM_UNLIKELY(!m_engine_filter[i].filter_obj)) {
            ERROR("Failed to create filter: %s",
                  m_engine_filter[i].filter.c_str());
            state = AM_STATE_ERROR;
            break;
          }
        } else {
          ERROR("Invalid filter plugin: %s", filter.c_str());
          state = AM_STATE_ERROR;
          break;
        }
      } else {
        ERROR("Failed to load filter: %s", filter.c_str());
        state = AM_STATE_ERROR;
        break;
      }
    }
  } else {
    state = AM_STATE_NO_MEMORY;
  }

  return state;
}

AMIPacketFilter* AMPlaybackEngine::get_filter_by_name(std::string& filter_name)
{
  AMIPacketFilter *filter = nullptr;
  for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
    if (AM_LIKELY(m_engine_filter[i].filter == filter_name)) {
      filter = (AMIPacketFilter*)m_engine_filter[i].filter_obj->\
          get_interface(IID_AMIPacketFilter);
      break;
    }
  }
  return filter;
}

ConnectionConfig* AMPlaybackEngine::get_connection_conf_by_name(
    std::string& name)
{
  ConnectionConfig *connection = nullptr;
  for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
    if (AM_LIKELY(m_engine_config->connections[i].filter == name)) {
      connection = &m_engine_config->connections[i];
      break;
    }
  }

  return connection;
}

void* AMPlaybackEngine::get_filter_by_iid(AM_REFIID iid)
{
  void *filter = nullptr;

  for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
    filter = m_engine_filter[i].filter_obj->get_interface(iid);
    if (AM_LIKELY(filter)) {
      break;
    }
  }

  return filter;
}

void AMPlaybackEngine::static_mainloop(void *data)
{
  ((AMPlaybackEngine*)data)->mainloop();
}

void AMPlaybackEngine::mainloop()
{
  fd_set fdset;
  sigset_t mask;
  sigset_t mask_orig;

  int maxfd = MSG_R;

  /* Block out interrupts */
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);
  sigaddset(&mask, SIGQUIT);

  if (AM_UNLIKELY(sigprocmask(SIG_BLOCK, &mask, &mask_orig) < 0)) {
    PERROR("sigprocmask");
  }

  m_mainloop_run = true;

  while(m_mainloop_run) {
    char cmd[1] = {0};
    FD_ZERO(&fdset);
    FD_SET(MSG_R, &fdset);

    if (AM_LIKELY(pselect(maxfd + 1, &fdset,
                          nullptr, nullptr,
                          nullptr, &mask) > 0)) {
      if (AM_LIKELY(FD_ISSET(MSG_R, &fdset))) {
        if (AM_UNLIKELY(read(MSG_R, cmd, 1) < 0)) {
          ERROR("Failed to read command! ABORT!");
          cmd[0] = AM_ENGINE_CMD_ABORT;
        }
      }
    } else {
      if (AM_LIKELY(errno != EINTR)) {
        PERROR("select");
        cmd[0] = AM_ENGINE_CMD_ABORT;
      }
    }
    switch(cmd[0]) {
      case AM_ENGINE_CMD_ABORT : {
        change_engine_status(AM_PLAYBACK_ENGINE_STOPPED);
        //m_mainloop_run = false;
      }break;
      case AM_ENGINE_CMD_START : {
        change_engine_status(AM_PLAYBACK_ENGINE_PLAYING, m_uri);
      }break;
      case AM_ENGINE_CMD_STOP  : {
        change_engine_status(AM_PLAYBACK_ENGINE_STOPPED);
      }break;
      case AM_ENGINE_CMD_PAUSE : {
        change_engine_status(AM_PLAYBACK_ENGINE_PAUSED);
      }break;
      case AM_ENGINE_CMD_EXIT: {
        change_engine_status(AM_PLAYBACK_ENGINE_STOPPED);
        m_mainloop_run = false;
      }break;
    }
    m_event->signal();
  }
  INFO("Playback Engine Mainloop exits!");
}

bool AMPlaybackEngine::send_engine_cmd(AM_PLAYBACK_ENGINE_CMD cmd, bool block)
{
  AUTO_MEM_LOCK(m_lock);
  bool ret = true;
  char command = cmd;
  if (AM_UNLIKELY(write(MSG_W, &command, sizeof(command)) != sizeof(command))) {
    ERROR("Failed to send command '%c'", command);
    ret = false;
  } else if (block) {
    m_event->wait();
    switch (cmd) {
      case AM_ENGINE_CMD_START:
        ret = (m_status == AM_PLAYBACK_ENGINE_PLAYING); break;
      case AM_ENGINE_CMD_STOP:
        ret = (m_status == AM_PLAYBACK_ENGINE_STOPPED); break;
      case AM_ENGINE_CMD_PAUSE:
        ret = (m_status == AM_PLAYBACK_ENGINE_PAUSED);  break;
      case AM_ENGINE_CMD_EXIT:
      case AM_ENGINE_CMD_ABORT:
        ret = true; break;
    }
  }
  return ret;
}
