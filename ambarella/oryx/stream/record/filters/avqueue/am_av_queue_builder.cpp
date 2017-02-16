/**
 * am_av_queue_builder.cpp
 *
 *  History:
 *		Dec 31, 2014 - [Shupeng Ren] created file
 *
 * Copyright (C) 2007-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <mutex>
#include <atomic>

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_amf_types.h"
#include "am_amf_packet.h"
#include "am_av_queue_builder.h"

#ifndef AUTO_LOCK
#define AUTO_LOCK(mtx) std::lock_guard<std::mutex> lck (mtx)
#endif

AMRingQueue* AMRingQueue::create(uint32_t count, uint32_t size)
{
  AMRingQueue *result = new AMRingQueue();
  if (result && (result->construct(count, size) != AM_STATE_OK)) {
    delete result;
    result = nullptr;
    ERROR("Failed to construct AMRingQueue error!");
  }
  return result;
}

AM_STATE AMRingQueue::construct(uint32_t count, uint32_t size)
{
  AM_STATE state = AM_STATE_OK;

  do {
    if ((int)count <= 0 || (int)size <= 0) {
      state = AM_STATE_ERROR;
      ERROR("Count: %d or size: %d is zero, please check!",
            count, size);
      break;
    }

    if (!(m_payload = new ExPayload[count])) {
      state  = AM_STATE_NO_MEMORY;
      ERROR("Failed to new ExPayload!");
      break;
    }

    m_datasize = ROUND_UP(size, 4);
    if (!(m_mem = new uint8_t[m_datasize * (count + 1)])) {
      state = AM_STATE_NO_MEMORY;
      ERROR("Failed to allocate data memory!");
      break;
    }

    m_current = m_mem;
    m_mem_end = m_mem + (count + 1) * m_datasize;
    m_payload_num = count;
    m_free_mem = count * m_datasize;
  } while (0);

  if (state != AM_STATE_OK) {
    if (!m_payload) {
      delete[] m_payload;
      m_payload = nullptr;
    }
    if (!m_mem) {
      delete[] m_mem;
      m_mem = nullptr;
    }
  }

  return state;
}

void AMRingQueue::destroy()
{
  delete this;
}

AMRingQueue::AMRingQueue() :
    m_mem(nullptr),
    m_mem_end(nullptr),
    m_current(nullptr),
    m_free_mem(0),
    m_reserved_mem_size(0),
    m_datasize(0),
    m_read_pos(0),
    m_read_pos_e(0),
    m_write_pos(0),
    m_payload(nullptr),
    m_payload_num(0),
    m_in_use_payload_num(0),
    m_in_use_payload_num_e(0),
    m_readable_payload_num(0),
    m_readable_payload_num_e(0)
{

}

AMRingQueue::~AMRingQueue()
{
  delete[] m_mem;
  delete[] m_payload;
}

void AMRingQueue::write(AMPacket::Payload *payload, queue_mode mode)
{
  AUTO_LOCK(m_mutex);
  uint8_t *buffer = payload->m_data.m_buffer;
  uint32_t size = payload->m_data.m_size;
  m_payload[m_write_pos] = *payload;
  if (buffer && (size > 0)) {
    if (m_current + size > m_mem_end) {
      m_free_mem += m_reserved_mem_size;
      m_reserved_mem_size = m_mem_end - m_current + 1;
      m_free_mem -= m_reserved_mem_size;
      m_current = m_mem;
    }
    memcpy(m_current, buffer, size);
    m_payload[m_write_pos].m_data.m_buffer = m_current;
    m_current += size;
    m_payload[m_write_pos].m_mem_end = m_current - 1;
    m_free_mem -= size;
  } else {
    m_payload[m_write_pos].m_data.m_size = 0;
    m_payload[m_write_pos].m_data.m_buffer = nullptr;
  }
  m_write_pos = (m_write_pos + 1) % m_payload_num;
  ++m_readable_payload_num;
  if (mode == queue_mode::event) {
    ++m_readable_payload_num_e;
  }
}

ExPayload* AMRingQueue::get()
{
  AUTO_LOCK(m_mutex);
  return (m_payload + m_read_pos);
}

ExPayload* AMRingQueue::event_get()
{
  AUTO_LOCK(m_mutex);
  return (m_payload + m_read_pos_e);
}

ExPayload* AMRingQueue::event_get_prev()
{
  AUTO_LOCK(m_mutex);
  return (m_read_pos_e == 0) ?
      &m_payload[m_payload_num - 1] :
      &m_payload[m_read_pos_e - 1];
}

void AMRingQueue::read_pos_inc(queue_mode mode)
{
  AUTO_LOCK(m_mutex);
  if (mode == queue_mode::normal) {
    ++(m_payload[m_read_pos].m_ref);
    m_payload[m_read_pos].m_normal_use = true;
    m_read_pos = (m_read_pos + 1) % m_payload_num;
    --m_readable_payload_num;
    ++m_in_use_payload_num;
  } else {
    ++m_payload[m_read_pos_e].m_ref;
    m_payload[m_read_pos_e].m_event_use = true;
    m_read_pos_e = (m_read_pos_e + 1) % m_payload_num;
    --m_readable_payload_num_e;
    ++m_in_use_payload_num_e;
  }
}

void AMRingQueue::event_backtrack()
{
  AUTO_LOCK(m_mutex);
  if (m_read_pos_e == 0) {
    m_read_pos_e = m_payload_num - 1;
  } else {
    --m_read_pos_e;
  }
  ++m_readable_payload_num_e;
}

void AMRingQueue::release(ExPayload *payload)
{
  AUTO_LOCK(m_mutex);
  if (payload->m_ref.fetch_sub(1) == 1) {
    if (payload->m_normal_use) {
      payload->m_normal_use = false;
      m_free_mem += payload->m_data.m_size;
      --m_in_use_payload_num;
    }
    if (payload->m_event_use) {
      payload->m_event_use = false;
      --m_in_use_payload_num_e;
    }
  }
}

void AMRingQueue::drop()
{
  AUTO_LOCK(m_mutex);
  m_free_mem += m_payload[m_read_pos].m_data.m_size;
  m_payload[m_read_pos].m_data.m_size = 0;
  m_payload[m_read_pos].m_data.m_buffer = nullptr;
  m_read_pos = (m_read_pos + 1) % m_payload_num;
  --m_readable_payload_num;
}

void AMRingQueue::reset()
{
  m_current = m_mem;
  m_free_mem = m_payload_num * m_datasize;
  m_reserved_mem_size = 0;
  m_readable_payload_num = 0;
  m_readable_payload_num_e = 0;
  m_read_pos = 0;
  m_read_pos_e = 0;
  m_write_pos = 0;
  m_in_use_payload_num = 0;
  m_in_use_payload_num_e = 0;
}

void AMRingQueue::event_reset()
{
  m_read_pos_e = m_write_pos;
  if (m_read_pos_e > 0) {
    --m_read_pos_e;
  }
  m_readable_payload_num_e = 1;
  m_in_use_payload_num_e = 0;
}

bool AMRingQueue::is_in_use()
{
  AUTO_LOCK(m_mutex);
  return (m_in_use_payload_num + m_in_use_payload_num_e) != 0;
}

bool AMRingQueue::is_full(queue_mode mode, uint32_t payload_size)
{
  AUTO_LOCK(m_mutex);
  bool ret = false;
  int32_t free_mem = m_free_mem;
  do {
    ret = (mode == queue_mode::event) ?
        (m_readable_payload_num_e +
            m_in_use_payload_num_e >=
            (int32_t)m_payload_num)
            :
        (m_readable_payload_num +
            m_in_use_payload_num >=
            (int32_t)m_payload_num);

    if (ret) {
      break;
    }
    if (payload_size) {
      if (m_current + payload_size > m_mem_end) {
        int32_t reserved_mem = m_mem_end - m_current + 1;
        free_mem += m_reserved_mem_size - reserved_mem;
      }
      ret = (free_mem < (int32_t)payload_size ? true : false);
    }
  } while (0);
  return ret;
}

bool AMRingQueue::is_empty(queue_mode mode)
{
  AUTO_LOCK(m_mutex);
  return (mode == queue_mode::event) ?
      (m_readable_payload_num_e == 0) :
      (m_readable_payload_num == 0);
}

bool AMRingQueue::about_to_full(queue_mode mode)
{
  AUTO_LOCK(m_mutex);
  return (mode == queue_mode::event) ?
      (m_readable_payload_num_e + m_in_use_payload_num_e >=
          (int32_t)m_payload_num * 4 / 5) :
          (m_readable_payload_num + m_in_use_payload_num >=
              (int32_t)m_payload_num * 4 / 5);
}

bool AMRingQueue::about_to_empty(queue_mode mode)
{
  AUTO_LOCK(m_mutex);
  return (mode == queue_mode::event) ?
      (m_readable_payload_num_e + m_in_use_payload_num_e <=
          (int32_t)m_payload_num / 5) :
          (m_readable_payload_num + m_in_use_payload_num <=
              (int32_t)m_payload_num / 5);
}

bool AMRingQueue::is_event_sync()
{
  AUTO_LOCK(m_mutex);
  return (m_readable_payload_num >= m_readable_payload_num_e);
}

uint32_t AMRingQueue::get_free_mem_size()
{
  AUTO_LOCK(m_mutex);
  return m_free_mem;
}

uint32_t AMRingQueue::get_free_payload_num()
{
  AUTO_LOCK(m_mutex);
  return (m_payload_num -
      m_readable_payload_num - m_in_use_payload_num);
}
