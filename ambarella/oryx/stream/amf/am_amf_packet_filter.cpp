/*******************************************************************************
 * am_amf_packet_filter.cpp
 *
 * History:
 *   2014-7-24 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
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

void AMPacketActiveFilter::get_info(INFO& info)
{
  info.num_in = 0;
  info.num_out = 0;
  info.name = m_name;
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
            on_stop();
            ack(AM_STATE_OK);
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
