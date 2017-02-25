/*******************************************************************************
 * am_amf_base.cpp
 *
 * History:
 *   2014-7-23 - [ypchang] created file
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
#include "am_amf_msgsys.h"

#include "am_thread.h"
#include "am_mutex.h"

/*
 * AMObject
 */

void* AMObject::get_interface(AM_REFIID refiid)
{
  return (refiid == IID_AMIInterface) ? (AMIInterface*)this : NULL;
}

void AMObject::destroy()
{
  delete this;
}

/*
 * AMWorkQueue
 */
AMWorkQueue* AMWorkQueue::create(AMIActiveObject *activeObj,
                                 bool RTPrio,
                                 int  prio)
{
  AMWorkQueue *result = new AMWorkQueue(activeObj);
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(RTPrio, prio)))) {
    delete result;
    result = NULL;
  }
  return result;
}

void AMWorkQueue::destroy()
{
  delete this;
}

AM_STATE AMWorkQueue::send_cmd(uint32_t cmd_id, void *data)
{
  AMIActiveObject::CMD cmd(cmd_id);
  cmd.data = data;
  return m_msg_q->send_msg(&cmd, sizeof(cmd));
}

void AMWorkQueue::get_cmd(AMIActiveObject::CMD& cmd)
{
  m_msg_q->get_msg(&cmd, sizeof(cmd));
}

bool AMWorkQueue::peek_cmd(AMIActiveObject::CMD& cmd)
{
  return m_msg_q->peek_msg(&cmd, sizeof(cmd));
}

bool AMWorkQueue::peek_msg(AmMsg& msg)
{
  return m_msg_q->peek_msg(&msg, sizeof(msg));
}

AM_STATE AMWorkQueue::run()
{
  return send_cmd(static_cast<uint32_t>(AMIActiveObject::CMD_RUN));
}

AM_STATE AMWorkQueue::stop()
{
  return send_cmd(static_cast<uint32_t>(AMIActiveObject::CMD_STOP));
}

void AMWorkQueue::ack(AM_STATE result)
{
  m_msg_q->reply(result);
}

AMQueue* AMWorkQueue::msgQ()
{
  return m_msg_q;
}

AMQueue::QTYPE AMWorkQueue::wait_data_msg(void *msg,
                                          uint32_t msgSize,
                                          AMQueue::WaitResult *result)
{
  return m_msg_q->wait_data_msg(msg, msgSize, result);
}

void AMWorkQueue::put_msg(void *msg, uint32_t msgSize)
{
  m_msg_q->put_msg(msg, msgSize);
}

AMWorkQueue::AMWorkQueue(AMIActiveObject *activeObj) :
    m_active_obj(activeObj),
    m_msg_q(NULL),
    m_thread(NULL)
{
}

AMWorkQueue::~AMWorkQueue()
{
  if (AM_LIKELY(m_thread)) {
    terminate();
    AM_DESTROY(m_thread);
  }
  AM_DESTROY(m_msg_q);
  DEBUG("~AMWorkQueue");
}

AM_STATE AMWorkQueue::init(bool RTPrio, int prio)
{
  AM_STATE state = AM_STATE_OK;

  do {
    m_msg_q = AMQueue::create(NULL, this, sizeof(AMIActiveObject::CMD), 0);
    if (AM_UNLIKELY(!m_msg_q)) {
      state = AM_STATE_NO_MEMORY;
      break;
    }

    m_thread = AMThread::create(m_active_obj->get_name(),
                                static_thread_entry,
                                this);
    if (AM_UNLIKELY(!m_thread)) {
      state = AM_STATE_OS_ERROR;
      break;
    }
    if (AM_LIKELY(RTPrio)) {
      if (AM_LIKELY(m_thread->set_priority(prio))) {
        NOTICE("Filter %s is changed to real time thread, priority is %u",
               m_active_obj->get_name(), prio);
      } else {
        ERROR("Failed to set real time priority to filter %s!",
              m_active_obj->get_name());
      }
    }
  } while(0);

  return state;
}

void AMWorkQueue::main_loop()
{
  bool run = true;
  AMIActiveObject::CMD cmd;

  while (run) {
    get_cmd(cmd);
    switch(cmd.code) {
      case AMIActiveObject::CMD_TERMINATE: {
        DEBUG("AMWorkQueue of %s received CMD_TERMINATE!",
              m_active_obj->get_name());
        ack(AM_STATE_OK);
        run = false;
        continue;
      }break;
      case AMIActiveObject::CMD_RUN: {
        DEBUG("AMWorkQueue of %s received CMD_RUN!",
                      m_active_obj->get_name());
        m_active_obj->on_run();
        DEBUG("AMWorkQueue of %s exits on_run() function!",
                      m_active_obj->get_name());
      }break;

      case AMIActiveObject::CMD_STOP: {
        DEBUG("AMWorkQueue of %s received CMD_STOP!",
                      m_active_obj->get_name());
        ack(AM_STATE_OK);
      }break;

      default: {
        m_active_obj->on_cmd(cmd);
      }break;
    }
  }
  INFO("AMWorkQueue of %s exits main_loop!", m_active_obj->get_name());
}

void AMWorkQueue::static_thread_entry(void *data)
{
  ((AMWorkQueue*)data)->main_loop();
}

void AMWorkQueue::terminate()
{
  if (AM_UNLIKELY(AM_STATE_OK !=
      send_cmd(static_cast<uint32_t>(AMIActiveObject::CMD_TERMINATE)))) {
    ERROR("Failed to send terminate command to filter!");
    AM_ABORT();
  }
}

/*
 * AMBaseEngine
 */
void* AMBaseEngine::get_interface(AM_REFIID refiid)
{
  return ((refiid == IID_AMIEngine) ? (AMIEngine*)this :
          ((refiid == IID_AMIMsgSink) ? (AMIMsgSink*)this :
           ((refiid == IID_AMIMediaControl) ? (AMIMediaControl*)this :
              inherited::get_interface(refiid))));
}

void AMBaseEngine::destroy()
{
  inherited::destroy();
}

AM_STATE AMBaseEngine::post_engine_msg(uint32_t code)
{
  AmMsg msg(code);
  return post_engine_msg(msg);
}

AM_STATE AMBaseEngine::post_engine_msg(AmMsg& msg)
{
  msg.session_id = m_session_id;
  return m_filter_msg_port->post_msg(msg);
}

AM_STATE AMBaseEngine::set_app_msg_sink(AMIMsgSink *appMsgSink)
{
  AUTO_MEM_LOCK(m_mutex);
  m_app_msg_sink = appMsgSink;
  return AM_STATE_OK;
}

AM_STATE AMBaseEngine::set_app_msg_callback(MsgProcType msgProc, void *data)
{
  AUTO_MEM_LOCK(m_mutex);
  m_app_msg_callback = msgProc;
  m_app_msg_data     = data;
  return AM_STATE_OK;
}

AM_STATE AMBaseEngine::post_app_msg(AmMsg& msg)
{
  msg.session_id = m_session_id;
  return m_msg_proxy->msg_port()->post_msg(msg);
}

AM_STATE AMBaseEngine::post_app_msg(uint32_t code)
{
  AmMsg msg(code);
  return post_app_msg(msg);
}

bool AMBaseEngine::is_session_msg(AmMsg& msg)
{
  return (msg.session_id == m_session_id);
}

AMBaseEngine::AMBaseEngine() :
    m_msg_sys(nullptr),
    m_app_msg_sink(nullptr),
    m_filter_msg_port(nullptr),
    m_msg_proxy(nullptr),
    m_app_msg_data(nullptr),
    m_app_msg_callback(nullptr),
    m_session_id(0)
{
}

AMBaseEngine::~AMBaseEngine()
{
  AM_DESTROY(m_msg_proxy);
  AM_DESTROY(m_filter_msg_port);
  AM_DESTROY(m_msg_sys);
  DEBUG("~AMBaseEngine");
}

AM_STATE AMBaseEngine::init()
{
  AM_STATE state = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(NULL == (m_msg_sys = AMMsgSys::create()))) {
      state = AM_STATE_NO_MEMORY;
      break;
    }
    m_filter_msg_port = AMMsgPort::create(((AMIMsgSink*)this), m_msg_sys);
    if (AM_UNLIKELY(!m_filter_msg_port)) {
      state = AM_STATE_NO_MEMORY;
      break;
    }
    m_msg_proxy = AMEngineMsgProxy::create(this);
    if (AM_UNLIKELY(!m_msg_proxy)) {
      state = AM_STATE_NO_MEMORY;
      break;
    }
  }while(0);

  return state;
}

void AMBaseEngine::on_app_msg(AmMsg& msg)
{
  AUTO_MEM_LOCK(m_mutex);
  if (AM_LIKELY(msg.session_id == m_session_id)) {
    if (AM_LIKELY(m_app_msg_sink)) {
      m_app_msg_sink->msg_proc(msg);
    } else if (m_app_msg_callback) {
      m_app_msg_callback(m_app_msg_data, msg);
    }
  }
}

/*
 * AMEngineMsgProxy
 */
AMEngineMsgProxy* AMEngineMsgProxy::create(AMBaseEngine* engine)
{
  AMEngineMsgProxy *result = new AMEngineMsgProxy(engine);
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init()))) {
    delete result;
    result = NULL;
  }

  return result;
}

void* AMEngineMsgProxy::get_interface(AM_REFIID refiid)
{
  return (refiid == IID_AMIMsgSink) ? (AMIMsgSink*)this :
      inherited::get_interface(refiid);
}

void AMEngineMsgProxy::destroy()
{
  inherited::destroy();
}

void AMEngineMsgProxy::msg_proc(AmMsg& msg)
{
  m_engine->on_app_msg(msg);
}

AMIMsgPort* AMEngineMsgProxy::msg_port()
{
  return m_msg_port;
}

AMEngineMsgProxy::AMEngineMsgProxy(AMBaseEngine *engine) :
    m_engine(engine),
    m_msg_port(NULL)
{
}

AMEngineMsgProxy::~AMEngineMsgProxy()
{
  AM_DESTROY(m_msg_port);
  DEBUG("~AMEngineMsgProxy");
}

AM_STATE AMEngineMsgProxy::init()
{
  AM_STATE state = AM_STATE_OK;

  m_msg_port = AMMsgPort::create((AMIMsgSink*)this, m_engine->m_msg_sys);
  if (AM_UNLIKELY(!m_msg_port)) {
    state = AM_STATE_NO_MEMORY;
  }

  return state;
}
