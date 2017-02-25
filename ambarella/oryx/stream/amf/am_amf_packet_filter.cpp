/*******************************************************************************
 * am_amf_packet_filter.cpp
 *
 * History:
 *   2014-7-24 - [ypchang] created file
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
#include "am_amf_packet_filter.h"
#include "am_amf_packet_pin.h"

/*
 * AMPacketFilter
 */
void* AMPacketFilter::get_interface(AM_REFIID refiid)
{
  return (refiid == IID_AMIPacketFilter) ? (AMIPacketFilter*)this:
      inherited::get_interface(refiid);
}

void AMPacketFilter::destroy()
{
  if (0 == -- m_ref) {
    inherited::destroy();
  }
}

void AMPacketFilter::add_ref()
{
  ++ m_ref;
}

AM_STATE AMPacketFilter::post_engine_msg(AmMsg& msg)
{
  return m_engine->post_engine_msg(msg);
}

AM_STATE AMPacketFilter::post_engine_msg(uint32_t code)
{
  return m_engine->post_engine_msg(code);
}

AMPacketFilter::AMPacketFilter(AMIEngine *engine) :
    m_engine(engine),
    m_name(NULL),
    m_ref(1)
{
}

AMPacketFilter::~AMPacketFilter()
{
  delete[] m_name;
}

AM_STATE AMPacketFilter::init(const char *name)
{
  m_name = amstrdup(name ? name : "NoNameFilter");
  return m_name ? AM_STATE_OK : AM_STATE_NO_MEMORY;
}

/*
 * AMPacketActiveFilter
 */
void* AMPacketActiveFilter::get_interface(AM_REFIID refiid)
{
  return (refiid == IID_AMIActiveObject) ? (AMIActiveObject*)this :
      inherited::get_interface(refiid);
}

void AMPacketActiveFilter::destroy()
{
  inherited::destroy();
}

AM_STATE AMPacketActiveFilter::run()
{
  return m_work_q->run();
}

AM_STATE AMPacketActiveFilter::stop()
{
  return m_work_q->stop();
}

void AMPacketActiveFilter::purge()
{
  INFO info;
  get_info(info);
  for (uint32_t i = 0; i < info.num_in; ++ i) {
    AMIPacketPin *pin = get_input_pin(i);
    if (AM_LIKELY(pin)) {
      pin->purge();
    }
  }
}

const char* AMPacketActiveFilter::get_name()
{
  return m_name;
}

bool AMPacketActiveFilter::wait_input_packet(AMPacketQueueInputPin*& pin,
                                             AMPacket*& packet)
{
  bool ret = false;
  bool run = true;
  AMQueue::WaitResult result;
  CMD cmd;

  while(run) {
    AMQueue::QTYPE type = m_work_q->wait_data_msg(&cmd, sizeof(cmd), &result);
    switch(type) {
      case AMQueue::AM_Q_MSG: {
        if (AM_LIKELY(!process_cmd(cmd))) {
          if (AM_LIKELY(cmd.code == AMIActiveObject::CMD_STOP)) {
            //on_stop();
            /* Pass on the CMD so that main_loop of work_queue can receive it */
            m_work_q->put_msg(&cmd, sizeof(cmd));
            //ack(AM_STATE_OK);
            ret = false; /* return false here to indicate STOP */
            run = false;;
          }
        }
      }break;
      case AMQueue::AM_Q_DATA: {
        pin = (AMPacketQueueInputPin*)result.owner;
        if (AM_LIKELY(pin->peek_packet(packet))) {
          ret = true;
          run = false;
        } else {
          INFO("No packet!");
        }
      }break;
      default:break;
    }
  }

  return ret;
}

AMQueue* AMPacketActiveFilter::msgQ()
{
  return m_work_q->msgQ();
}

void AMPacketActiveFilter::get_cmd(AMIActiveObject::CMD& cmd)
{
  return m_work_q->get_cmd(cmd);
}

bool AMPacketActiveFilter::peek_cmd(AMIActiveObject::CMD& cmd)
{
  return m_work_q->peek_cmd(cmd);
}

bool AMPacketActiveFilter::peek_msg(AmMsg& msg)
{
  return m_work_q->peek_msg(msg);
}

void AMPacketActiveFilter::ack(AM_STATE result)
{
  m_work_q->ack(result);
}

AMPacketActiveFilter::AMPacketActiveFilter(AMIEngine *engine) :
    inherited(engine),
    m_work_q(NULL)
{
}

AMPacketActiveFilter::~AMPacketActiveFilter()
{
  AM_DESTROY(m_work_q);
  DEBUG("~AMPacketActiveFilter");
}

AM_STATE AMPacketActiveFilter::init(const char *name, bool RTPrio, int prio)
{
  AM_STATE state = inherited::init(name);
  if (AM_LIKELY(state == AM_STATE_OK)) {
    m_work_q = AMWorkQueue::create((AMIActiveObject*)this, RTPrio, prio);
    state = m_work_q ? AM_STATE_OK : AM_STATE_NO_MEMORY;
  } else {
    ERROR("Failed to initialize AMPacketFilter!");
  }

  return state;
}
