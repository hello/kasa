/*******************************************************************************
 * am_record_engine.cpp
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

#include <unistd.h>
#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_amf_types.h"
#include "am_amf_interface.h"
#include "am_amf_queue.h"
#include "am_amf_base.h"
#include "am_amf_engine_frame.h"

#include "am_record_engine_if.h"
#include "am_record_engine.h"
#include "am_record_engine_config.h"

#include "am_audio_source_if.h"
#include "am_video_source_if.h"
#include "am_event_sender_if.h"
#include "am_av_queue_if.h"
#include "am_muxer_if.h"

#include "am_mutex.h"
#include "am_event.h"
#include "am_thread.h"
#include "am_plugin.h"

AMIRecordEngine* AMIRecordEngine::create()
{
  std::string conf(ORYX_CONF_DIR);
  conf.append("/stream/engine/record-engine.acs");

  return (AMIRecordEngine*)AMRecordEngine::create(conf);
}

AMRecordEngine* AMRecordEngine::create(const std::string& config)
{
  AMRecordEngine *result = new AMRecordEngine();
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(config)))) {
    delete result;
    result = NULL;
  }

  return result;
}

void* AMRecordEngine::get_interface(AM_REFIID ref_iid)
{
  return (IID_AMIRecordEngine == ref_iid) ? ((AMIRecordEngine*)this) :
      inherited::get_interface(ref_iid);
}

void AMRecordEngine::destroy()
{
  inherited::destroy();
}

AMIRecordEngine::AM_RECORD_ENGINE_STATUS AMRecordEngine::get_engine_status()
{
  AUTO_SPIN_LOCK(m_lock);
  return m_status;
}

bool AMRecordEngine::create_graph()
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
            AMIPacketPin *out_pin = NULL;
            uint32_t k = 0;
            for (k = 0; k < in_conn->output_number; ++ k) {
              out_pin = in_filter->get_output_pin(k);
              if (AM_LIKELY(out_pin && !out_pin->is_connected())) {
                break;
              } else {
                out_pin = NULL;
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
      m_thread = AMThread::create("RecordENGLoop", static_mainloop, this);
      if (AM_UNLIKELY(!m_thread)) {
        delete_all_connections();
        remove_all_filters();
        m_graph_created = false;
      }
    }
  }

  return m_graph_created;
}

bool AMRecordEngine::record()
{
  bool ret = false;
  uint32_t count = 0;
  while(!m_mainloop_run && (count < 30)) {
    m_sem->wait(100000);
    ++ count;
  }
  if(AM_LIKELY(m_mainloop_run)) {
    ret = send_engine_cmd(AM_ENGINE_CMD_START);
  } else {
    ERROR("The thread of engine is not running.");
    ret = false;
  }
  return ret;
}

bool AMRecordEngine::stop()
{
  bool ret = true;
  if(AM_LIKELY(m_mainloop_run)) {
    ret = send_engine_cmd(AM_ENGINE_CMD_STOP);
  } else {
    NOTICE("The engine is already stopped.");
  }
  return ret;
}

bool AMRecordEngine::start_file_recording()
{
  INFO("start file recording in engine.");
  std::string filter_name = "file-muxer";
  AMIMuxer *muxer = get_muxer_filter_by_name(filter_name);
  if(!muxer) {
    ERROR("failed to get file muxer filter.");
  }
  return (muxer ? muxer->start_file_recording() : false);
}

bool AMRecordEngine::stop_file_recording()
{
  bool ret = true;
  INFO("stop file recording in engine.");
  std::string filter_name = "file-muxer";
  AMIMuxer *muxer = get_muxer_filter_by_name(filter_name);
  if(!muxer) {
    ERROR("failed to get file muxer filter.");
  } else {
    ret = muxer->stop_file_recording();
  }
  return ret;
}

void AMRecordEngine::set_app_msg_callback(AMRecordCallback callback, void *data)
{
  m_app_callback = callback;
  m_app_data     = data;
}

bool AMRecordEngine::is_ready_for_event()
{
  AMIAVQueue *avqueue =
      (AMIAVQueue*)get_filter_by_iid(IID_AMIAVQueue);
  return avqueue ? avqueue->is_ready_for_event() : false;
}

bool AMRecordEngine::send_event()
{
  AMIEventSender *sender =
      (AMIEventSender*)get_filter_by_iid(IID_AMIEventSender);
  INFO("begin to send event in record engine.");
  return sender ? sender->send_event() : false;
}

void AMRecordEngine::static_app_msg_callback(void *context, AmMsg& msg)
{
  ((AMRecordEngine*)context)->app_msg_callback(
      ((AMRecordEngine*)context)->m_app_data, msg);
}

void AMRecordEngine::app_msg_callback(void *context, AmMsg& msg)
{
  if (AM_LIKELY(m_app_callback)) {
    m_app_msg.data = context;
    m_app_msg.msg  = AM_RECORD_MSG(msg.code);
    m_app_callback(m_app_msg);
  }
}

void AMRecordEngine::msg_proc(AmMsg& msg)
{
  if (AM_LIKELY(is_session_msg(msg))) {
    const char *name = get_filter_name_by_pointer((AMIInterface*)msg.p0);
    switch(msg.code) {
      case ENG_MSG_ERROR: {
        NOTICE("Received error message from filter %s!");
        m_status = AM_RECORD_ENGINE_ERROR;
        post_app_msg(AM_RECORD_MSG_ERROR);
      }break;
      case ENG_MSG_EOS: {
        /* todo: report EOS of stream */
      }break;
      case ENG_MSG_ABORT: {
        NOTICE("Received abort request from filter %s!",
               name ? name : "Unknown");
        m_status = AM_RECORD_ENGINE_ERROR;
        send_engine_cmd(AM_ENGINE_CMD_ABORT, false);
        post_app_msg(AM_RECORD_MSG_ABORT);
      }break;
      case ENG_MSG_OK: {
        NOTICE("Received start message from filter %s!",
               name ? name : "Unknown");
      }break;
      default: {
        ERROR("Invalid message!");
        post_app_msg(AM_RECORD_MSG_NULL);
      }break;
    }
  }
}

bool AMRecordEngine::change_engine_status(
    AMIRecordEngine::AM_RECORD_ENGINE_STATUS targetStatus)
{
  bool ret = true;

  DEBUG("Target status is %u", targetStatus);
  do {
    switch(m_status) {
      case AM_RECORD_ENGINE_ERROR: {
        stop_all_filters();
        purge_all_filters();
        m_status = AM_RECORD_ENGINE_STOPPED;
        ret = false;
      }break;
      case AM_RECORD_ENGINE_TIMEOUT: {
        m_status = AM_RECORD_ENGINE_STOPPED;
        post_app_msg(AM_RECORD_MSG_TIMEOUT);
        ret = false;
      }break;
      case AM_RECORD_ENGINE_RECORDING: {
        switch(targetStatus) {
          case AM_RECORD_ENGINE_STOPPED: {
            ((AMIAudioSource*)get_filter_by_iid(IID_AMIAudioSource))->stop();
            ((AMIVideoSource*)get_filter_by_iid(IID_AMIVideoSource))->stop();
            m_status = AM_RECORD_ENGINE_STOPPING;
            stop_all_filters();
            purge_all_filters();
            m_status = AM_RECORD_ENGINE_STOPPED;
            post_app_msg(AM_RECORD_MSG_STOP_OK);
          }break;
          case AM_RECORD_ENGINE_RECORDING: {
            NOTICE("Already recording!");
          }break;
          default: {
            ERROR("Invalid operation when engine is recording!");
          }break;
        }
      }break;
      case AM_RECORD_ENGINE_STOPPED: {
        switch(targetStatus) {
          case AM_RECORD_ENGINE_RECORDING: {
            m_status = AM_RECORD_ENGINE_STARTING;
            if (AM_LIKELY(AM_STATE_OK == run_all_filters())) {
              AMIAudioSource *asrc =
                  (AMIAudioSource*)get_filter_by_iid(IID_AMIAudioSource);
              AMIVideoSource *vsrc =
                  (AMIVideoSource*)get_filter_by_iid(IID_AMIVideoSource);
              if (AM_LIKELY(asrc && vsrc)) {
                ret = ((AM_STATE_OK == asrc->start()) &&
                       (AM_STATE_OK == vsrc->start()));
                if (AM_UNLIKELY(!ret)) {
                  m_status = AM_RECORD_ENGINE_ERROR;
                  post_app_msg(AM_RECORD_MSG_ERROR);
                  ret = true;
                } else {
                  m_status = AM_RECORD_ENGINE_RECORDING;
                  post_app_msg(AM_RECORD_MSG_START_OK);
                }
              } else {
                if (AM_LIKELY(!asrc)) {
                  ERROR("Failed to get filter(IID_AMIAudioSource)!");
                }
                if (AM_LIKELY(!vsrc)) {
                  ERROR("Failed to get filter(IID_AMIVideoSource)!");
                }
                m_status = AM_RECORD_ENGINE_STOPPED;
                ret = false;
              }
            } else {
              ERROR("Failed to run all filters!");
              m_status = AM_RECORD_ENGINE_ERROR;
              ret = true;
            }
          }break;
          case AM_RECORD_ENGINE_STOPPED: {
            NOTICE("Already stopped!");
          }break;
          default: {
            ERROR("Invalid operation when engine is stopped!");
            m_status = AM_RECORD_ENGINE_ERROR;
          }break;
        }
      }break;
      /* Intermediate status */
      case AM_RECORD_ENGINE_STARTING: {
        uint32_t count = 0;
        do {
          if (AM_LIKELY((m_status == AM_RECORD_ENGINE_RECORDING) ||
                        (m_status == AM_RECORD_ENGINE_ERROR))) {
            if (AM_UNLIKELY(m_status == AM_RECORD_ENGINE_ERROR)) {
              ERROR("Failed to start recording!");
            }
            break;
          }
          usleep(100000);
          ++ count;
        }while(count < m_engine_config->op_timeout * 10);
        if (AM_UNLIKELY((count >= m_engine_config->op_timeout*10) &&
                        (m_status == AM_RECORD_ENGINE_STARTING))) {
          ERROR("START operation timed out!");
          m_status = AM_RECORD_ENGINE_TIMEOUT;
        }
      }break;
      case AM_RECORD_ENGINE_STOPPING: {
        uint32_t count = 0;
        do {
          if (AM_LIKELY((m_status == AM_RECORD_ENGINE_STOPPED) ||
                        (m_status == AM_RECORD_ENGINE_ERROR))) {
            if (AM_UNLIKELY(m_status == AM_RECORD_ENGINE_ERROR)) {
              ERROR("Failed to stop!");
            }
            break;
          }
          usleep(100000);
          ++ count;
        }while(count < m_engine_config->op_timeout * 10);
        if (AM_UNLIKELY((count >= m_engine_config->op_timeout*10) &&
                        (m_status == AM_RECORD_ENGINE_STOPPING))) {
          ERROR("STOP operation timed out!");
          m_status = AM_RECORD_ENGINE_TIMEOUT;
        }
      }break;
      default: {
        ret = false;
        ERROR("Invalid engine status!");
      }break;
    }
  } while(ret && (m_status != targetStatus));

  if (AM_LIKELY(ret && (m_status == targetStatus))) {
    DEBUG("Target %u finished!", targetStatus);
  } else {
    DEBUG("Current status %u, target status %u", m_status, targetStatus);
  }

  return ret;
}

AMRecordEngine::AMRecordEngine() :
    m_lock(NULL),
    m_config(NULL),
    m_engine_config(NULL),
    m_engine_filter(NULL),
    m_app_data(NULL),
    m_thread(NULL),
    m_event(NULL),
    m_sem(NULL),
    m_app_callback(NULL),
    m_status(AMIRecordEngine::AM_RECORD_ENGINE_STOPPED),
    m_graph_created(false),
    m_mainloop_run(false)
{
  MSG_R = -1;
  MSG_W = -1;
}

AMRecordEngine::~AMRecordEngine()
{
  send_engine_cmd(AM_ENGINE_CMD_EXIT, false);
  clear_graph();/* Must be called in the destructor of sub-class */
  delete m_config;
  delete[] m_engine_filter;
  AM_DESTROY(m_lock);
  AM_DESTROY(m_thread);
  AM_DESTROY(m_event);
  AM_DESTROY(m_sem);
  if (AM_LIKELY(MSG_R)) {
    close(MSG_R);
  }
  if (AM_LIKELY(MSG_W)) {
    close(MSG_W);
  }
  DEBUG("~AMRecordEngine");
}

AM_STATE AMRecordEngine::init(const std::string& config)
{
  AM_STATE state = AM_STATE_ERROR;

  do {
    if (AM_UNLIKELY(pipe(m_msg_ctrl) == -1)) {
      PERROR("pipe");
      state = AM_STATE_ERROR;
      break;
    }
    m_lock = AMSpinLock::create();
    if (AM_UNLIKELY(NULL == m_lock)) {
      ERROR("Failed to create spin lock!");
      break;
    }
    m_event = AMEvent::create();
    if (AM_UNLIKELY(NULL == m_event)) {
      ERROR("Failed to create event!");
      break;
    }
    m_sem = AMEvent::create();
    if (AM_UNLIKELY(NULL == m_sem)) {
      ERROR("Failed to create event!");
      break;
    }
    m_config = new AMRecordEngineConfig();
    if (AM_UNLIKELY(NULL == m_config)) {
      ERROR("Failed to create config module for record engine!");
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
        ERROR("Failed to load filters needed by Record Engine!");
        break;
      } else {
        for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
          AMIPacketFilter *filter = (AMIPacketFilter*)
              m_engine_filter[i].filter_obj->get_interface(IID_AMIPacketFilter);
          if (AM_UNLIKELY(AM_STATE_OK
              != (state = add_filter(filter, m_engine_filter[i].so)))) {
            break;
          }
        }
        if (AM_UNLIKELY(AM_STATE_OK != state)) {
          break;
        }
      }
      state = inherited::set_app_msg_callback(static_app_msg_callback, this);
    }
  } while (0);

  return state;
}

AM_STATE AMRecordEngine::load_all_filters()
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
        if (AM_LIKELY(create_filter && con)) {
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

AMIPacketFilter* AMRecordEngine::get_filter_by_name(std::string& name)
{
  AMIPacketFilter *filter = NULL;
  for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
    if (AM_LIKELY(m_engine_filter[i].filter == name)) {
      filter = (AMIPacketFilter*)m_engine_filter[i].filter_obj->\
          get_interface(IID_AMIPacketFilter);
      break;
    }
  }

  return filter;
}

AMIMuxer* AMRecordEngine::get_muxer_filter_by_name(std::string& name)
{
  AMIMuxer *filter = NULL;
  for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
    if (AM_LIKELY(m_engine_filter[i].filter == name)) {
      filter = (AMIMuxer*)m_engine_filter[i].filter_obj->\
          get_interface(IID_AMIMuxer);
      break;
    }
  }
  return filter;
}

const char* AMRecordEngine::get_filter_name_by_pointer(AMIInterface *filter)
{
  const char *name = NULL;
  for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
    if (AM_LIKELY(m_engine_filter[i].filter_obj->\
                  get_interface(IID_AMIInterface) == filter)) {
      name = m_engine_filter[i].filter.c_str();
      break;
    }
  }

  return name;
}

ConnectionConfig* AMRecordEngine::get_connection_conf_by_name(std::string& name)
{
  ConnectionConfig *connection = NULL;
  for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
    if (AM_LIKELY(m_engine_config->connections[i].filter == name)) {
      connection = &m_engine_config->connections[i];
      break;
    }
  }

  return connection;
}

void* AMRecordEngine::get_filter_by_iid(AM_REFIID iid)
{
  void *filter = NULL;
  for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
    filter = m_engine_filter[i].filter_obj->get_interface(iid);
    if (AM_LIKELY(filter)) {
      break;
    }
  }

  return filter;
}

void AMRecordEngine::static_mainloop(void *data)
{
  ((AMRecordEngine*)data)->mainloop();
}

void AMRecordEngine::mainloop()
{
  fd_set fdset;
  int maxfd = MSG_R;
  m_mainloop_run = true;
  m_sem->signal();

  while(m_mainloop_run) {
    char cmd[1] = {0};
    FD_ZERO(&fdset);
    FD_SET(MSG_R, &fdset);

    if (AM_LIKELY(select(maxfd + 1, &fdset, NULL, NULL, NULL) > 0)) {
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
        DEBUG("Received ABORT!");
        change_engine_status(AM_RECORD_ENGINE_STOPPED);
        m_mainloop_run = false;
      }break;
      case AM_ENGINE_CMD_START : {
        DEBUG("Received START!");
        change_engine_status(AM_RECORD_ENGINE_RECORDING);
      }break;
      case AM_ENGINE_CMD_STOP  : {
        DEBUG("Received STOP!");
        change_engine_status(AM_RECORD_ENGINE_STOPPED);
      }break;
      case AM_ENGINE_CMD_EXIT: {
        DEBUG("Received EXIT!");
        m_mainloop_run = false;
      }break;
    }
    m_event->signal();
  }
  INFO("Record Engine Mainloop exits!");
}

bool AMRecordEngine::send_engine_cmd(AM_RECORD_ENGINE_CMD cmd, bool block)
{
  AUTO_SPIN_LOCK(m_lock);
  bool ret = true;
  char command = cmd;
  if (AM_UNLIKELY(write(MSG_W, &command, sizeof(command)) != sizeof(command))) {
    ERROR("Failed to send command '%c'", command);
    ret = false;
  } else if (block) {
    m_event->wait();
    switch(cmd) {
      case AM_ENGINE_CMD_START :
        ret = (m_status == AM_RECORD_ENGINE_RECORDING); break;
      case AM_ENGINE_CMD_STOP  :
        ret = (m_status == AM_RECORD_ENGINE_STOPPED);   break;
      case AM_ENGINE_CMD_EXIT  :
      case AM_ENGINE_CMD_ABORT :
        ret = true;
        break;
    }
  }

  return ret;
}
