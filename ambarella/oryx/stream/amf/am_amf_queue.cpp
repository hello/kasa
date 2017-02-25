/*******************************************************************************
 * am_amf_queue.cpp
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
#include "am_log.h"

#include "am_amf_types.h"
#include "am_amf_queue.h"

#include "am_mutex.h"

AMQueue* AMQueue::create(AMQueue *mainQ,
                         void    *owner,
                         uint32_t blockSize,
                         uint32_t reservedSlots)
{
  AMQueue *result = new AMQueue(mainQ, owner);
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(blockSize,
                                                         reservedSlots)))) {
    delete result;
    result = NULL;
  }

  return result;
}

void AMQueue::destroy()
{
  delete this;
}

AM_STATE AMQueue::post_msg(const void *msg, uint32_t msgSize)
{
  AM_ASSERT(is_main());
  AUTO_MTX_LOCK(m_mutex);

  AM_STATE state = AM_STATE_NO_MEMORY;
  List *node = alloc_node();
  if (AM_LIKELY(node)) {
    write_data(node, msg, msgSize);
    if (AM_LIKELY(m_num_get > 0)) {
      -- m_num_get;
      m_cond_get->signal();
    }
    state = AM_STATE_OK;
  }

  return state;
}

AM_STATE AMQueue::send_msg(const void *msg, uint32_t msgSize)
{
  AM_ASSERT(is_main());
  AUTO_MTX_LOCK(m_mutex);

  AM_STATE result = AM_STATE_ERROR;

  while (true) {
    if (AM_LIKELY(m_msg_result == NULL)) {
      write_data(m_send_buffer, msg, msgSize);

      if (AM_LIKELY(m_num_get > 0)) {
        -- m_num_get;
        m_cond_get->signal();
      }

      m_msg_result = &result;
      m_cond_reply->wait(m_mutex);
      m_msg_result = NULL;

      if (AM_LIKELY(m_num_send_msg > 0)) {
        -- m_num_send_msg;
        m_cond_send_msg->signal();
      }
      break;
    }

    ++ m_num_send_msg;
    m_cond_send_msg->wait(m_mutex);
  }

  return result;
}

void AMQueue::get_msg(void *msg, uint32_t msgSize)
{
  AM_ASSERT(is_main());
  AUTO_MTX_LOCK(m_mutex);

  while (true) {
    if (AM_LIKELY(m_num_data > 0)) {
      read_data(msg, msgSize);
      break;
    }
    ++ m_num_get;
    m_cond_get->wait(m_mutex);
  }
}

bool AMQueue::get_msg_non_block(void *msg, uint32_t msgSize)
{
  AM_ASSERT(is_main());
  AUTO_MTX_LOCK(m_mutex);
  bool ret = true;

  while (true) {
    if (AM_LIKELY(m_is_disabled)) {
      ret = false;
      break;
    }
    if (AM_LIKELY(m_num_data > 0)) {
      read_data(msg, msgSize);
      break;
    }
    ++ m_num_get;
    m_cond_get->wait(m_mutex);
  }
  return ret;
}

bool AMQueue::peek_msg(void *msg, uint32_t msgSize)
{
  AM_ASSERT(is_main());
  AUTO_MTX_LOCK(m_mutex);
  bool ret = false;

  if (AM_LIKELY(m_num_data > 0)) {
    if (AM_LIKELY(msg)) {
      read_data(msg, msgSize);
    }
    ret = true;
  }
  return ret;
}

void AMQueue::reply(AM_STATE result)
{
  AM_ASSERT(is_main());
  AM_ASSERT(m_msg_result);
  AUTO_MTX_LOCK(m_mutex);

  *m_msg_result = result;
  m_cond_reply->signal();
}

void AMQueue::enable(bool enabled)
{
  AUTO_MTX_LOCK(m_mutex);
  m_is_disabled = !enabled;

  if (AM_LIKELY(m_num_get > 0)) {
    m_num_get = 0;
    m_cond_get->signal_all();
  }
}

bool AMQueue::is_enable()
{
  AUTO_MTX_LOCK(m_mutex);
  return !m_is_disabled;
}

AM_STATE AMQueue::put_data(const void *buffer, uint32_t size)
{
  AM_ASSERT(is_sub());
  AUTO_MTX_LOCK(m_mutex);

  AM_STATE state = AM_STATE_NO_MEMORY;
  List *node = alloc_node();
  if (AM_LIKELY(node)) {
    write_data(node, buffer, size);
    m_main_q->m_msg_list->push(this);
    if (AM_LIKELY(m_main_q->m_num_get > 0)) {
      -- m_main_q->m_num_get;
      m_main_q->m_cond_get->signal();
    }
    state = AM_STATE_OK;
  }

  return state;
}

AMQueue::QTYPE AMQueue::wait_data_msg(void       *msg,
                                      uint32_t    msgSize,
                                      WaitResult *result)
{
  AM_ASSERT(is_main());
  AUTO_MTX_LOCK(m_mutex);
  AMQueue::QTYPE type = AM_Q_MSG;
  while (true) {
    bool got_result = false;
    if (AM_LIKELY(m_num_data > 0)) {
      read_data(msg, msgSize);
      break;
    }

    if (AM_LIKELY(!m_msg_list->empty())) {
      AMQueue *q = m_msg_list->front_pop();
      if (AM_LIKELY((q->m_num_data > 0) && !q->m_is_disabled)) {
        result->dataQ = q;
        result->owner = q->m_owner;
        result->block_size = q->m_block_size;
        type = AM_Q_DATA;
        got_result = true;
        break;
      }
    }
#if 0
    for (AMQueue *q = m_next_q; q != this; q = q->m_next_q) {
      if (AM_LIKELY((q->m_num_data > 0) && !q->m_is_disabled)) {
        result->dataQ = q;
        result->owner = q->m_owner;
        result->block_size = q->m_block_size;
        type = AM_Q_DATA;
        got_result = true;
        break;
      }
    }
#endif
    if (AM_LIKELY(got_result)) {
      break;
    }
    ++ m_num_get;
    m_cond_get->wait(m_mutex);
  }

  return type;
}

void AMQueue::put_msg(void *msg, uint32_t msgSize)
{
  AM_ASSERT(is_main());
  AUTO_MTX_LOCK(m_mutex);
  write_data(m_send_buffer, msg, msgSize);
  if (AM_LIKELY(m_num_get > 0)) {
    -- m_num_get;
    m_cond_get->signal();
  }
}

bool AMQueue::peek_data(void *buffer, uint32_t size)
{
  AM_ASSERT(is_sub());
  AUTO_MTX_LOCK(m_mutex);
  bool ret = false;

  if (AM_LIKELY(m_num_data > 0)) {
    read_data(buffer, size);
    ret = true;
  }

  return ret;
}

uint32_t AMQueue::get_available_data_number()
{
  return m_num_data;
}

bool AMQueue::is_main()
{
  return (NULL == m_main_q);
}

bool AMQueue::is_sub()
{
  return (NULL != m_main_q);
}

AMQueue::AMQueue(AMQueue *mainQ, void *owner) :
  m_owner(owner),
  m_main_q(mainQ)
{
}

AMQueue::~AMQueue()
{
  if (AM_LIKELY(m_mutex)) {
    m_mutex->lock();
  }
  AM_ASSERT(m_num_get == 0);
  AM_ASSERT(m_num_send_msg == 0);

  if (is_sub()) {
    m_prev_q->m_next_q = m_next_q;
    m_next_q->m_prev_q = m_prev_q;
  } else {
    AM_ASSERT(m_prev_q == this);
    AM_ASSERT(m_next_q == this);
    AM_ASSERT(m_msg_result == NULL);
  }

  if (m_head) {
    m_head->destroy();
  }
  if (m_free_list) {
    m_free_list->destroy();
  }
  delete[] m_reserved_mem;

  if (AM_LIKELY(m_mutex)) {
    m_mutex->unlock();
  }

  if (is_main()) {
    AM_DESTROY(m_cond_send_msg);
    AM_DESTROY(m_cond_reply);
    AM_DESTROY(m_cond_get);
    AM_DESTROY(m_mutex);
    if (AM_LIKELY(m_msg_list)) {
      while(!m_msg_list->empty()) {
        m_msg_list->pop();
      }
      delete m_msg_list;
    }
  }
}

AM_STATE AMQueue::init(uint32_t blockSize, uint32_t reservedSlots)
{
  AM_STATE state = AM_STATE_OK;
  m_block_size = ROUND_UP(blockSize, 4);
  do {
    uint32_t nodeSize = sizeof(List) + m_block_size;
    m_reserved_mem = new uint8_t[nodeSize * (reservedSlots + 1)];
    if (AM_UNLIKELY(!m_reserved_mem)) {
      state = AM_STATE_NO_MEMORY;
      break;
    }

    m_send_buffer = (List*)m_reserved_mem;
    m_send_buffer->next = NULL;
    m_send_buffer->allocated = false;

    for (List *node = (List*)(m_reserved_mem + nodeSize);
         reservedSlots > 0; -- reservedSlots) {
      node->allocated = false;
      node->next = m_free_list;
      m_free_list = node;
      node = (List*)(((uint8_t*)node) + nodeSize);
    }

    if (is_main()) {
      if (AM_UNLIKELY(NULL == (m_mutex = AMMutex::create(false)))) {
        state = AM_STATE_OS_ERROR;
        break;
      }
      if (AM_UNLIKELY(NULL == (m_cond_get = AMCondition::create()))) {
        state = AM_STATE_OS_ERROR;
        break;
      }
      if (AM_UNLIKELY(NULL == (m_cond_reply = AMCondition::create()))) {
        state = AM_STATE_OS_ERROR;
        break;
      }
      if (AM_UNLIKELY(NULL == (m_cond_send_msg = AMCondition::create()))) {
        state = AM_STATE_OS_ERROR;
        break;
      }
      m_msg_list = new AMQueueList();
      if (AM_UNLIKELY(!m_msg_list)) {
        state = AM_STATE_NO_MEMORY;
        break;
      }
    } else {
      m_mutex         = m_main_q->m_mutex;
      m_cond_get      = m_main_q->m_cond_get;
      m_cond_reply    = m_main_q->m_cond_reply;
      m_cond_send_msg = m_main_q->m_cond_send_msg;

      AUTO_MTX_LOCK(m_main_q->m_mutex);
      m_prev_q = m_main_q->m_prev_q;
      m_next_q = m_main_q;
      m_prev_q->m_next_q = this;
      m_next_q->m_prev_q = this;
    }
  } while(0);

  return state;
}

void AMQueue::List::destroy()
{
  List *node = this;
  while (node) {
    List *next = node->next;
    if (node->allocated) {
      delete[] ((uint8_t*)node);
    }
    node = next;
  }
}

void AMQueue::copy(void *to, const void *from, uint32_t bytes)
{
  if (bytes == sizeof(void*)) {
    *(void**)to = *(void**)from;
  } else {
    memcpy(to, from, bytes);
  }
}

AMQueue::List* AMQueue::alloc_node()
{
  List *node = NULL;
  if (AM_LIKELY(m_free_list)) {
    node = m_free_list;
    m_free_list = m_free_list->next;
    node->next = NULL;
  } else {
    node = (List*)(new uint8_t[sizeof(List) + m_block_size]);
    if (AM_LIKELY(node)) {
      node->next = NULL;
      node->allocated = true;
    }
  }

  return node;
}

#define NODE_BUFFER(_NODE) (((uint8_t*)_NODE) + sizeof(List))

void AMQueue::write_data(List *node, const void *buffer, uint32_t size)
{
  copy(NODE_BUFFER(node), buffer, AM_MIN(m_block_size, size));
  ++ m_num_data;

  node->next = NULL;
  m_tail->next = node;
  m_tail = node;
}

void AMQueue::read_data(void *buffer, uint32_t size)
{
  List *node = m_head;
  AM_ASSERT(node);

  m_head = m_head->next;
  if (AM_UNLIKELY(!m_head)) {
    m_tail = (List*)&m_head;
  }

  if (AM_LIKELY(node != m_send_buffer)) {
    node->next = m_free_list;
    m_free_list = node;
  }

  copy(buffer, NODE_BUFFER(node), AM_MIN(m_block_size, size));
  -- m_num_data;
}
