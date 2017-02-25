/**
 * am_av_queue_builder.cpp
 *
 *  History:
 *		Dec 31, 2014 - [Shupeng Ren] created file
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
 */

#include <mutex>
#include <atomic>

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_amf_types.h"
#include "am_amf_packet.h"
#include "am_av_queue_builder.h"

#define AUTO_LOCK_AVQ(mtx) std::lock_guard<std::mutex> lck(mtx)

AMRingQueue* AMRingQueue::create(uint32_t count)
{
  AMRingQueue *result = new AMRingQueue();
  if (result && (result->construct(count) != AM_STATE_OK)) {
    delete result;
    result = nullptr;
    ERROR("Failed to construct AMRingQueue!");
  }
  return result;
}

AM_STATE AMRingQueue::construct(uint32_t count)
{
  AM_STATE state = AM_STATE_OK;

  do {
    if ((int)count <= 0) {
      state = AM_STATE_ERROR;
      ERROR("Packet count = %d, please check!", count);
      break;
    }

    if (!(m_payload = new ExPayload[count])) {
      state  = AM_STATE_NO_MEMORY;
      ERROR("Failed to new ExPayload!");
      break;
    }
    m_payload_num = count;
  } while (0);

  if (state != AM_STATE_OK) {
    if (m_payload) {
      delete[] m_payload;
      m_payload = nullptr;
    }
  }
  return state;
}

void AMRingQueue::destroy()
{
  delete this;
}

AMRingQueue::AMRingQueue() :
    m_payload(nullptr),
    m_payload_num(0),
    m_read_pos(0),
    m_read_pos_e(0),
    m_write_pos(0),
    m_write_count(0),
    m_in_use_payload_num(0),
    m_in_use_payload_num_e(0),
    m_readable_payload_num(0),
    m_readable_payload_num_e(0)
{
}

AMRingQueue::~AMRingQueue()
{
  for (int32_t i = 0; i < m_payload_num; ++i) {
    delete[] m_payload[i].m_data.m_buffer;
  }
  delete[] m_payload;
}

bool AMRingQueue::write(const AMPacket::Payload *payload, queue_mode mode)
{
  AUTO_LOCK_AVQ(m_mutex);
  bool ret = true;
  do {
    uint32_t cur_size = payload->m_data.m_size;
    uint8_t *cur_buffer = payload->m_data.m_buffer;
    //uint32_t pre_size = m_payload[m_write_pos].m_data.m_size;
    uint8_t *pre_buffer = m_payload[m_write_pos].m_data.m_buffer;

    delete pre_buffer;
    if (!(pre_buffer = new uint8_t[cur_size])) {
      ERROR("Failed to allocate buffer!");
      ret = false;
      break;
    }
    memcpy(pre_buffer, cur_buffer, cur_size);
    m_payload[m_write_pos] = *payload;
    m_payload[m_write_pos].m_data.m_size = cur_size;
    m_payload[m_write_pos].m_data.m_buffer = pre_buffer;
    m_write_pos = (m_write_pos + 1) % m_payload_num;
    ++m_readable_payload_num;
    if (m_write_count < m_payload_num) {
      ++m_write_count;
    }
    if (mode == queue_mode::event) {
      ++m_readable_payload_num_e;
    }
  } while (0);
  return ret;
}

ExPayload* AMRingQueue::get()
{
  AUTO_LOCK_AVQ(m_mutex);
  return &m_payload[m_read_pos];
}

ExPayload* AMRingQueue::event_get()
{
  AUTO_LOCK_AVQ(m_mutex);
  return &m_payload[m_read_pos_e];
}

ExPayload* AMRingQueue::event_get_prev()
{
  AUTO_LOCK_AVQ(m_mutex);
  return (m_read_pos_e == 0) ?
      &m_payload[m_payload_num - 1] : &m_payload[m_read_pos_e - 1];
}

void AMRingQueue::read_pos_inc(queue_mode mode)
{
  AUTO_LOCK_AVQ(m_mutex);
  switch(mode) {
    case queue_mode::normal: {
      ++m_payload[m_read_pos].m_normal_ref;
      m_read_pos = (m_read_pos + 1) % m_payload_num;
      --m_readable_payload_num;
      ++m_in_use_payload_num;
    }break;
    case queue_mode::event: {
      ++m_payload[m_read_pos_e].m_event_ref;
      m_read_pos_e = (m_read_pos_e + 1) % m_payload_num;
      --m_readable_payload_num_e;
      ++m_in_use_payload_num_e;
    }break;
    default: break;
  }
}

void AMRingQueue::event_backtrack()
{
  AUTO_LOCK_AVQ(m_mutex);
  if (m_read_pos_e == 0) {
    m_read_pos_e = m_payload_num - 1;
  } else {
    --m_read_pos_e;
  }
  ++m_readable_payload_num_e;
}

bool AMRingQueue::event_empty()
{
  AUTO_LOCK_AVQ(m_mutex);
  return m_readable_payload_num_e + m_in_use_payload_num_e >= m_write_count;
}

void AMRingQueue::normal_release(ExPayload *payload)
{
  AUTO_LOCK_AVQ(m_mutex);
  if ((payload->m_normal_ref > 0) && (--payload->m_normal_ref == 0)) {
    --m_in_use_payload_num;
  }
}

void AMRingQueue::event_release(ExPayload *payload)
{
  AUTO_LOCK_AVQ(m_mutex);
  if ((payload->m_event_ref > 0) && (--payload->m_event_ref == 0)) {
    --m_in_use_payload_num_e;
  }
}

void AMRingQueue::reset()
{
  AUTO_LOCK_AVQ(m_mutex);
  m_read_pos = 0;
  m_read_pos_e = 0;
  m_write_pos = 0;
  m_in_use_payload_num = 0;
  m_in_use_payload_num_e = 0;
  m_readable_payload_num = 0;
  m_readable_payload_num_e = 0;
}

void AMRingQueue::event_reset()
{
  AUTO_LOCK_AVQ(m_mutex);
  m_read_pos_e = m_write_pos;
  m_readable_payload_num_e = 0;
}

bool AMRingQueue::normal_in_use()
{
  AUTO_LOCK_AVQ(m_mutex);
  return m_in_use_payload_num;
}

bool AMRingQueue::event_in_use()
{
  AUTO_LOCK_AVQ(m_mutex);
  return m_in_use_payload_num_e;
}

bool AMRingQueue::in_use()
{
  AUTO_LOCK_AVQ(m_mutex);
  return (m_in_use_payload_num + m_in_use_payload_num_e) != 0;
}

bool AMRingQueue::full(queue_mode mode)
{
  AUTO_LOCK_AVQ(m_mutex);
  bool ret = false;
  do {
    if (m_readable_payload_num + m_in_use_payload_num >= m_payload_num) {
      ret = true;
      break;
    }
    if ((mode == queue_mode::event) &&
        (m_readable_payload_num_e + m_in_use_payload_num_e >= m_payload_num)) {
      ret = true;
      break;
    }
  } while (0);
  return ret;
}

bool AMRingQueue::empty(queue_mode mode)
{
  AUTO_LOCK_AVQ(m_mutex);
  return (mode == queue_mode::event) ?
      (m_readable_payload_num_e <= 0) : (m_readable_payload_num <= 0);
}

bool AMRingQueue::about_to_full(queue_mode mode)
{
  AUTO_LOCK_AVQ(m_mutex);
  return (mode == queue_mode::event) ?
      (m_readable_payload_num_e + m_in_use_payload_num_e >= m_payload_num * 4 / 5) :
      (m_readable_payload_num + m_in_use_payload_num >= m_payload_num * 4 / 5);
}

bool AMRingQueue::about_to_empty(queue_mode mode)
{
  AUTO_LOCK_AVQ(m_mutex);
  return (mode == queue_mode::event) ?
      (m_readable_payload_num_e + m_in_use_payload_num_e <= m_payload_num / 5) :
      (m_readable_payload_num + m_in_use_payload_num <= m_payload_num / 5);
}

int32_t AMRingQueue::get_free_payload_num()
{
  AUTO_LOCK_AVQ(m_mutex);
  return (m_payload_num - m_readable_payload_num - m_in_use_payload_num);
}
