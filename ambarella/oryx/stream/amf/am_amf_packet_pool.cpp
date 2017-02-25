/*******************************************************************************
 * am_amf_packet_pool.cpp
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
#include "am_amf_packet.h"
#include "am_amf_packet_pool.h"

/*
 * AMPacketPool
 */
AMPacketPool* AMPacketPool::create(const char *name, uint32_t count)
{
  AMPacketPool *result = new AMPacketPool();
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(name, count)))) {
    delete result;
    result = NULL;
  }

  return result;
}

void* AMPacketPool::get_interface(AM_REFIID refiid)
{
  return (refiid == IID_AMIPacketPool) ? (AMIPacketPool*)this :
      inherited::get_interface(refiid);
}

void AMPacketPool::destroy()
{
  inherited::destroy();
}

const char* AMPacketPool::get_name()
{
  return m_name;
}

void AMPacketPool::enable(bool enabled)
{
  m_packet_q->enable(enabled);
}

bool AMPacketPool::is_enable()
{
  return m_packet_q->is_enable();
}

bool AMPacketPool::alloc_packet(AMPacket*& packet, uint32_t size)
{
  bool ret = m_packet_q->get_msg_non_block(&packet, sizeof(packet));

  if (AM_LIKELY(ret)) {
    packet->m_ref_count = 1;
    packet->m_pool = this;
    packet->m_next = NULL;
    packet->set_packet_type(AMPacket::AM_PACKET_TYPE_NORMAL);
  }

  return ret;
}

uint32_t AMPacketPool::get_avail_packet_num()
{
  return m_packet_q->get_available_data_number();
}

void AMPacketPool::add_ref(AMPacket *packet)
{
  ++ packet->m_ref_count;
}

void AMPacketPool::release(AMPacket *packet)
{
  if (--packet->m_ref_count == 0) {
    AMPacket *next = packet->m_next;
    on_release_packet(packet);
    m_packet_q->post_msg((void*)&packet, sizeof(packet));

    if (AM_LIKELY(next)) {
      on_release_packet(next);
      m_packet_q->post_msg((void*)&next, sizeof(next));
    }
  }
}

void AMPacketPool::add_ref()
{
  ++ m_ref_count;
}

void AMPacketPool::release()
{
  if (--m_ref_count == 0) {
    inherited::destroy();
  }
}

AMPacketPool::AMPacketPool() :
    m_packet_q(nullptr),
    m_name(nullptr),
    m_ref_count(1)
{
}

AMPacketPool::~AMPacketPool()
{
  DEBUG("Packet pool %s is destroyed!", m_name);
  delete[] m_name;
  AM_DESTROY(m_packet_q);
  DEBUG("~AMPacketPool");
}

AM_STATE AMPacketPool::init(const char *name, uint32_t count)
{
  AM_STATE state = AM_STATE_OK;

  do {
    m_name = amstrdup(name);
    if (AM_UNLIKELY(!m_name)) {
      state = AM_STATE_NO_MEMORY;
      break;
    }

    m_packet_q = AMQueue::create(NULL, this, sizeof(AMQueue*), count);
    if (AM_UNLIKELY(!m_packet_q)) {
      state = AM_STATE_NO_MEMORY;
      break;
    }
  }while(0);

  return state;
}

/*
 * AMSimplePacketPool
 */
AMSimplePacketPool* AMSimplePacketPool::create(const char *name,
                                               uint32_t count,
                                               uint32_t objSize)
{
  AMSimplePacketPool *result = new AMSimplePacketPool();
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(name,
                                                         count,
                                                         objSize)))) {
    delete result;
    result = NULL;
  }
  return result;
}

AMSimplePacketPool::AMSimplePacketPool() :
    m_packet_mem(NULL)
{
}

AMSimplePacketPool::~AMSimplePacketPool()
{
  delete[] m_packet_mem;
}

AM_STATE AMSimplePacketPool::init(const char *name,
                                  uint32_t count,
                                  uint32_t objSize)
{
  AM_STATE state = AM_STATE_ERROR;
  if (AM_LIKELY((objSize == sizeof(AMPacket)) &&
                (AM_STATE_OK == inherited::init(name, count)))) {
    m_packet_mem = new AMPacket[count];
    if (AM_UNLIKELY(!m_packet_mem)) {
      state = AM_STATE_NO_MEMORY;
    } else {
      for (uint32_t i = 0; i < count; ++ i) {
        AMPacket *packet = &m_packet_mem[i];
        state = m_packet_q->post_msg(&packet, sizeof(packet));
        if (AM_UNLIKELY(AM_STATE_OK != state)) {
          break;
        }
      }
    }
  } else if (AM_LIKELY(objSize != sizeof(AMPacket))) {
    ERROR("Invalid packet size, simple packet pool can only support AMPacket!");
  } else {
    ERROR("Failed to initialize AMPacketPool!");
  }

  return state;
}

/*
 * AMFixedPacketPool
 */
AMFixedPacketPool* AMFixedPacketPool::create(const char *name,
                                             uint32_t count,
                                             uint32_t dataSize)
{
  AMFixedPacketPool *result = new AMFixedPacketPool();
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(name,
                                                         count,
                                                         dataSize)))) {
    delete result;
    result = NULL;
  }

  return result;
}

AMFixedPacketPool::AMFixedPacketPool() :
  m_payload_mem(NULL),
  m_max_data_size(0)
{
}

AMFixedPacketPool::~AMFixedPacketPool()
{
  delete[] m_payload_mem;
}

AM_STATE AMFixedPacketPool::init(const char *name,
                                 uint32_t count,
                                 uint32_t dataSize)
{
  AM_STATE state = inherited::init(name, count, sizeof(AMPacket));
  if (AM_LIKELY(AM_STATE_OK == state)) {
    uint32_t payloadSize = ROUND_UP((dataSize + sizeof(AMPacket::Payload)), 4);
    m_payload_mem = new uint8_t[payloadSize * count];
    if (AM_UNLIKELY(!m_payload_mem)) {
      state = AM_STATE_NO_MEMORY;
    } else {
      for (uint32_t i = 0; i < count; ++ i) {
        m_packet_mem[i].set_payload(m_payload_mem + i * payloadSize);
        m_packet_mem[i].set_data_ptr((dataSize > 0) ?
            (m_payload_mem + i * payloadSize + sizeof(AMPacket::Payload)) :
            NULL);
      }
      state = AM_STATE_OK;
    }
  }

  return state;
}
