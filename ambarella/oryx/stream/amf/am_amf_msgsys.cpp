/*******************************************************************************
 * am_amf_msgsys.cpp
 *
 * History:
 *   2014-7-22 - [ypchang] created file
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
#include "am_thread.h"
#include "am_log.h"

#include "am_amf_types.h"
#include "am_amf_interface.h"
#include "am_amf_msgsys.h"
#include "am_amf_queue.h"

/*
 * AMMsgPort
 */
AMMsgPort* AMMsgPort::create(AMIMsgSink *msgSink, AMMsgSys *msgSys)
{
  AMMsgPort* result = new AMMsgPort(msgSink);
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(msgSys)))) {
    delete result;
    result = NULL;
  }
  return result;
}

void* AMMsgPort::get_interface(AM_REFIID refiid)
{
  if (AM_LIKELY(refiid == IID_AMIMsgPort)) {
    return (AMIMsgPort*)this;
  } else if (AM_LIKELY(refiid == IID_AMIInterface)) {
    return (AMIInterface*)this;
  }
  return NULL;
}

void AMMsgPort::destroy()
{
  delete this;
}

AMMsgPort::AMMsgPort(AMIMsgSink *msgSink) :
    m_msg_sink(msgSink),
    m_queue(NULL)
{
}

AMMsgPort::~AMMsgPort()
{
  AM_DESTROY(m_queue);
  DEBUG("~AMMsgPort");
}

AM_STATE AMMsgPort::init(AMMsgSys *msgSys)
{
  AM_STATE state = AM_STATE_OK;
  m_queue = AMQueue::create(msgSys->m_main_q,
                            this,
                            sizeof(AmMsg),
                            1);
  if (AM_UNLIKELY(!m_queue)) {
    state = AM_STATE_NO_MEMORY;
  }

  return state;
}

AM_STATE AMMsgPort::post_msg(AmMsg& msg)
{
  return m_queue->put_data(&msg, sizeof(msg));
}

/*
 * AMMsgSys
 */
AMMsgSys* AMMsgSys::create()
{
  AMMsgSys* result = new AMMsgSys();
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init()))) {
    delete result;
    result = NULL;
  }
  return result;
}

void AMMsgSys::destroy()
{
  delete this;
}

AMMsgSys::AMMsgSys() :
    m_main_q(NULL),
    m_thread(NULL)
{
}

AMMsgSys::~AMMsgSys()
{
  uint32_t msg = 0;
  if (AM_UNLIKELY(m_main_q &&
                  (AM_STATE_OK != m_main_q->send_msg(&msg, sizeof(msg))))) {
    ERROR("Failed to send message to terminate message loop!");
  }
  AM_DESTROY(m_thread);
  /* todo: delete all sub queues */
  AM_DESTROY(m_main_q);
  DEBUG("~AMMsgSys");
}

AM_STATE AMMsgSys::init()
{
  AM_STATE state = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(NULL == (m_main_q = AMQueue::create(NULL,
                                                        this,
                                                        sizeof(uint32_t),
                                                        1)))) {
      state = AM_STATE_OS_ERROR;
      break;
    }
    if (AM_UNLIKELY(NULL == (m_thread = AMThread::create("AMMsgSys",
                                                         static_thread_entry,
                                                         this)))) {
      state = AM_STATE_OS_ERROR;
      break;
    }
  } while(0);

  return state;
}

void AMMsgSys::static_thread_entry(void *data)
{
  ((AMMsgSys*)data)->main_loop();
}

void AMMsgSys::main_loop()
{
  AmMsg msg;
  while (true) {
    uint32_t dummy;
    AMQueue::WaitResult result;
    AMQueue::QTYPE qtype = m_main_q->wait_data_msg(&dummy,
                                                   sizeof(dummy),
                                                   &result);
    if (AM_UNLIKELY(AMQueue::AM_Q_MSG == qtype)) {
      m_main_q->reply(AM_STATE_OK);
      break;
    }

    if (AM_LIKELY(AMQueue::AM_Q_DATA == qtype)) {
      if (AM_LIKELY(result.dataQ->peek_data(&msg, sizeof(msg)))) {
        AMIMsgSink *msgSink = ((AMMsgPort*)result.owner)->m_msg_sink;
        if (AM_LIKELY(msgSink)) {
          msgSink->msg_proc(msg);
        }
      }
    }
  }
}
