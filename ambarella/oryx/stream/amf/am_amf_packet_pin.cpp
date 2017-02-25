/*******************************************************************************
 * am_amf_packet_pin.cpp
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
#include "am_amf_packet.h"
#include "am_amf_packet_pin.h"

/*
 * AMPacketPin
 */
void* AMPacketPin::get_interface(AM_REFIID refiid)
{
  return ((refiid == IID_AMIPacketPool) ? m_packet_pool :
          ((refiid == IID_AMIPacketPin) ? (AMIPacketPin*)this :
              inherited::get_interface(refiid)));
}

void AMPacketPin::destroy()
{
  inherited::destroy();
}

AM_STATE AMPacketPin::connect(AMIPacketPin *peer)
{
  ERROR("NOT implemented, should be implemented in sub-class!");
  return AM_STATE_NOT_IMPL;
}

void AMPacketPin::disconnect()
{
  ERROR("NOT implemented, should be implemented in sub-class!");
}
void AMPacketPin::receive(AMPacket *packet)
{
  ERROR("NOT implemented, should be implemented in sub-class!");
}

void AMPacketPin::purge()
{
  ERROR("NOT implemented, should be implemented in sub-class!");
}

void AMPacketPin::enable(bool enabled)
{
  if (AM_LIKELY(m_packet_pool)) {
    m_packet_pool->enable(enabled);
  }
}

bool AMPacketPin::is_enable()
{
  return m_packet_pool && m_packet_pool->is_enable();
}

AMIPacketPin* AMPacketPin::get_peer()
{
  return m_peer;
}

AMIPacketFilter* AMPacketPin::get_filter()
{
  return m_filter;
}

bool AMPacketPin::is_connected()
{
  return (NULL != m_peer);
}

void AMPacketPin::on_disconnect()
{
  release_packet_pool();
}

void AMPacketPin::set_packet_pool(AMIPacketPool *packetPool)
{
  if (AM_LIKELY(packetPool)) {
    packetPool->add_ref();
  }

  if (AM_LIKELY(m_packet_pool)) {
    m_packet_pool->release();
  }
  m_packet_pool = packetPool;
  m_is_external_packet_pool = (NULL != packetPool);
}

void AMPacketPin::release_packet_pool()
{
  if (AM_LIKELY(m_packet_pool && !m_is_external_packet_pool)) {
    m_packet_pool->release();
    m_packet_pool = NULL;
  }
}

AM_STATE AMPacketPin::post_engine_msg(AmMsg& msg)
{
  return m_filter->post_engine_msg(msg);
}

AM_STATE AMPacketPin::post_engine_msg(uint32_t code)
{
  AmMsg msg(code);
  msg.p1 = (int_ptr)m_filter;
  return post_engine_msg(msg);
}

AM_STATE AMPacketPin::post_engine_error_msg(AM_STATE result)
{
  AmMsg msg(AMIEngine::ENG_MSG_ERROR);
  msg.p0 = result;
  msg.p1 = (int_ptr)m_filter;
  return post_engine_msg(msg);
}

const char* AMPacketPin::filter_name()
{
  AMIPacketFilter::INFO info;
  m_filter->get_info(info);
  return info.name;
}

AMPacketPin::AMPacketPin(AMPacketFilter *filter) :
    m_filter(filter),
    m_peer(NULL),
    m_packet_pool(NULL),
    m_is_external_packet_pool(false)
{
}

AMPacketPin::~AMPacketPin()
{
  set_packet_pool(NULL);
}

/*
 * AMPacketInputPin
 */
AM_STATE AMPacketInputPin::connect(AMIPacketPin *peer)
{
  AM_STATE state = AM_STATE_ERROR;
  if (AM_LIKELY(!m_peer)) {
    if (AM_LIKELY(AM_STATE_OK == (state = on_connect(peer)))) {
      m_peer = peer;
    } else {
      ERROR("Failed to connect pin!");
    }
  } else {
    ERROR("Input pin of %s is already connected!", filter_name());
    state = AM_STATE_BAD_STATE;
  }
  DEBUG("AMPacketInputPin::connect");

  return state;
}

void AMPacketInputPin::disconnect()
{
  if (AM_LIKELY(m_peer)) {
    on_disconnect();
    m_peer = NULL;
  }
}

AM_STATE AMPacketInputPin::on_connect(AMIPacketPin *peer)
{
  /* Allow empty packet pool,
   * which is useful when a filter is just a sink,
   * only pass packets to down streams
   */
  if (AM_LIKELY(!m_packet_pool)) {
    m_packet_pool = AMIPacketPool::get_interface_from(peer);
    if (AM_LIKELY(m_packet_pool)) {
      m_packet_pool->add_ref();
    } else {
      DEBUG("No packet pool found for pin of %s!", filter_name());
    }
  }

  return AM_STATE_OK;
}

/*
 * AMPacketQueueInputPin
 */
void AMPacketQueueInputPin::receive(AMPacket *packet)
{
  AM_ASSERT(m_packet_q);
  AM_ENSURE(AM_STATE_OK == m_packet_q->put_data(&packet, sizeof(packet)));
}

void AMPacketQueueInputPin::purge()
{
  if (AM_LIKELY(m_packet_q)) {
    AMPacket *packet;
    while(m_packet_q->peek_data((void*)&packet, sizeof(packet))) {
      packet->release();
    }
  }
}

bool AMPacketQueueInputPin::peek_packet(AMPacket*& packet)
{
  return m_packet_q->peek_data((void*)&packet, sizeof(packet));
}

void AMPacketQueueInputPin::enable(bool enabled)
{
  if (AM_LIKELY(m_packet_q)) {
    m_packet_q->enable(enabled);
  }
}

uint32_t AMPacketQueueInputPin::get_avail_packet_num()
{
  AM_ASSERT(m_packet_q);
  return m_packet_q->get_available_data_number();
}

AMPacketQueueInputPin::AMPacketQueueInputPin(AMPacketFilter *filter) :
    inherited(filter),
    m_packet_q(NULL)
{
}

AMPacketQueueInputPin::~AMPacketQueueInputPin()
{
  AM_DESTROY(m_packet_q);
  DEBUG("~AMPacketQueueInputPin");
}

AM_STATE AMPacketQueueInputPin::init(AMQueue *msgQ)
{
  AM_STATE state = AM_STATE_OK;
  m_packet_q = AMQueue::create(msgQ, this, sizeof(AMPacket*), INPUT_PIN_Q_LEN);
  if (AM_UNLIKELY(!m_packet_q)) {
    state = AM_STATE_NO_MEMORY;
  }
  return state;
}

/*
 * AMPacketActiveInputPin
 */
void* AMPacketActiveInputPin::get_interface(AM_REFIID refiid)
{
  return (refiid == IID_AMIActiveObject) ? (AMIActiveObject*)this :
      inherited::get_interface(refiid);
}

void AMPacketActiveInputPin::destroy()
{
  inherited::destroy();
}

const char* AMPacketActiveInputPin::get_name()
{
  return m_name;
}

void AMPacketActiveInputPin::on_run()
{
  AMQueue::WaitResult result;
  AMPacket *packet = NULL;
  CMD cmd;

  ack(AM_STATE_OK);

  INFO("%s starts to run!", m_name);
  while(true) {
    AMQueue::QTYPE type = m_work_q->wait_data_msg(&cmd, sizeof(cmd), &result);
    if (AM_LIKELY(type == AMQueue::AM_Q_MSG)) {
      if (AM_LIKELY(cmd.code == AMIActiveObject::CMD_STOP)) {
        ack(AM_STATE_OK);
        INFO("%s pin received stop command!", m_name);
        break;
      }
    } else {
      if (AM_LIKELY(peek_packet(packet))) {
        AM_STATE state = process_packet(packet);
        if (AM_LIKELY(state != AM_STATE_OK)) {
          if (AM_LIKELY(state != AM_STATE_CLOSED)) {
            post_engine_error_msg(state);
          }
          break;
        }
      }
    }
  }
}

AM_STATE AMPacketActiveInputPin::run()
{
  return m_work_q->run();
}

AM_STATE AMPacketActiveInputPin::stop()
{
  return m_work_q->stop();
}

AMQueue* AMPacketActiveInputPin::msgQ()
{
  return m_work_q->msgQ();
}

void AMPacketActiveInputPin::get_cmd(AMIActiveObject::CMD& cmd)
{
  m_work_q->get_cmd(cmd);
}

bool AMPacketActiveInputPin::peek_cmd(AMIActiveObject::CMD& cmd)
{
  return m_work_q->peek_cmd(cmd);
}

void AMPacketActiveInputPin::ack(AM_STATE result)
{
  m_work_q->ack(result);
}

AMPacketActiveInputPin::AMPacketActiveInputPin(AMPacketFilter *filter) :
    inherited(filter),
    m_work_q(NULL),
    m_name(NULL)
{
}

AMPacketActiveInputPin::~AMPacketActiveInputPin()
{
  /* AMPacketQueueInputPin's m_packet_q is m_work_q's sub queue,
   * which must be destroyed before main queue
   */
  AM_DESTROY(m_packet_q);
  AM_DESTROY(m_work_q);
  delete[] m_name;
}

AM_STATE AMPacketActiveInputPin::init(const char *name)
{
  AM_STATE state = AM_STATE_OK;
  do {
    m_name = amstrdup(name ? name : "NoNameInputPin");
    if (AM_UNLIKELY(!m_name)) {
      state = AM_STATE_NO_MEMORY;
      break;
    }
    m_work_q = AMWorkQueue::create((AMIActiveObject*)this);
    if (AM_UNLIKELY(!m_work_q)) {
      state = AM_STATE_NO_MEMORY;
      break;
    }
    state = inherited::init(m_work_q->msgQ());

  }while(0);

  return state;
}

/*
 * AMPacketOutputPin
 */
AM_STATE AMPacketOutputPin::connect(AMIPacketPin *peer)
{
  AM_STATE state = AM_STATE_ERROR;

  if (AM_LIKELY(!m_peer)) {
    state = peer->connect(this);
    if (AM_LIKELY(AM_STATE_OK == state)) {
      state = on_connect(peer);
      if (AM_LIKELY(AM_STATE_OK == state)) {
        m_peer = peer;
      } else {
        ERROR("Failed to connect output pin to input pin!");
        peer->disconnect();
      }
    } else {
      ERROR("Failed to connect input pin to this pin!");
    }
  } else {
    ERROR("Output pin of %s is already connected!", filter_name());
  }
  DEBUG("AMPacketOutputPin::connect");

  return state;
}

void AMPacketOutputPin::disconnect()
{
  if (AM_LIKELY(m_peer)) {
    on_disconnect();
    m_peer->disconnect();
    m_peer = NULL;
  }
}

bool AMPacketOutputPin::alloc_packet(AMPacket*& packet, uint32_t size)
{
  return m_packet_pool ? m_packet_pool->alloc_packet(packet, size) : false;
}

void AMPacketOutputPin::send_packet(AMPacket *packet)
{
  if (AM_LIKELY(m_peer)) {
    m_peer->receive(packet);
  } else {
    NOTICE("Pin is not connected, drop packet!");
    packet->release();
  }
}

uint32_t AMPacketOutputPin::get_avail_packet_num()
{
  return m_packet_pool->get_avail_packet_num();
}

AM_STATE AMPacketOutputPin::on_connect(AMIPacketPin *peer)
{
  /* Allow empty packet pool,
   * which is useful when a filter is just a sink,
   * only pass packets to down streams
   */
  AM_STATE state = AM_STATE_OK;
  if (AM_UNLIKELY(!m_packet_pool)) {
    m_packet_pool = AMIPacketPool::get_interface_from(peer);
    if (AM_LIKELY(m_packet_pool)) {
      m_packet_pool->add_ref();
    } else {
      DEBUG("No packet pool found for pin of %s", filter_name());
    }
  } else {
    AMIPacketPool *packetPool = AMIPacketPool::get_interface_from(peer);
    if (AM_UNLIKELY(packetPool != m_packet_pool)) {
      ERROR("Input pin of %s uses different packet pool from this output pin!",
            filter_name());
      release_packet_pool();
      state = AM_STATE_ERROR;
    }
  }

  return state;
}

/*
 * AMPacketActiveOutputPin
 */
void* AMPacketActiveOutputPin::get_interface(AM_REFIID refiid)
{
  return (refiid == IID_AMIActiveObject) ? (AMIActiveObject*)this :
      inherited::get_interface(refiid);
}

void AMPacketActiveOutputPin::destroy()
{
  inherited::destroy();
}

const char* AMPacketActiveOutputPin::get_name()
{
  return m_name;
}

AM_STATE AMPacketActiveOutputPin::run()
{
  return m_work_q->run();
}

AM_STATE AMPacketActiveOutputPin::stop()
{
  return m_work_q->stop();
}

AMQueue* AMPacketActiveOutputPin::msgQ()
{
  return m_work_q->msgQ();
}

void AMPacketActiveOutputPin::get_cmd(AMIActiveObject::CMD& cmd)
{
  m_work_q->get_cmd(cmd);
}

bool AMPacketActiveOutputPin::peek_cmd(AMIActiveObject::CMD& cmd)
{
  return m_work_q->peek_cmd(cmd);
}

void AMPacketActiveOutputPin::ack(AM_STATE result)
{
  m_work_q->ack(result);
}

AMPacketActiveOutputPin::AMPacketActiveOutputPin(AMPacketFilter *filter) :
    inherited(filter),
    m_work_q(NULL),
    m_name(NULL)
{
}

AMPacketActiveOutputPin::~AMPacketActiveOutputPin()
{
  ERROR("==============");
  AM_DESTROY(m_work_q);
  ERROR("--------------");
  delete[] m_name;
}

AM_STATE AMPacketActiveOutputPin::init(const char *name)
{
  AM_STATE state = AM_STATE_NO_MEMORY;
  m_name = amstrdup(name ? name : "NoNameOutputPin");
  if (AM_LIKELY(m_name)) {
    m_work_q = AMWorkQueue::create((AMIActiveObject*)this);
    if (AM_LIKELY(m_work_q)) {
      state = AM_STATE_OK;
    }
  }

  return state;
}
