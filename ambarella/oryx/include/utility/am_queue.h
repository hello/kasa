/*******************************************************************************
 * am_queue.h
 *
 * History:
 *   Jun 18, 2016 - [ypchang] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
#ifndef ORYX_INCLUDE_UTILITY_AM_QUEUE_H_
#define ORYX_INCLUDE_UTILITY_AM_QUEUE_H_

#include <queue>
#include "am_mutex.h"

template <class T>
class AMSafeQueue
{
  public:
    AMSafeQueue()
    {
    }
    virtual ~AMSafeQueue()
    {
    }

  public:
    bool empty()
    {
      AUTO_MEM_LOCK(m_mutex);
      return m_queue.empty();
    }
    void push(T data)
    {
      AUTO_MEM_LOCK(m_mutex);
      m_queue.push(data);
    }
    T front_pop()
    {
      AUTO_MEM_LOCK(m_mutex);
      T data = m_queue.front();
      m_queue.pop();
      return data;
    }
    T front()
    {
      AUTO_MEM_LOCK(m_mutex);
      return m_queue.front();
    }
    void pop()
    {
      AUTO_MEM_LOCK(m_mutex);
      m_queue.pop();
    }
    size_t size()
    {
      AUTO_MEM_LOCK(m_mutex);
      return m_queue.size();
    }

  protected:
    AMMemLock     m_mutex;
    std::queue<T> m_queue;
};

template <class T>
class AMSafeDeque
{
  public:
    AMSafeDeque()
    {
    }
    virtual ~AMSafeDeque()
    {
    }

  public:
    bool empty()
    {
      AUTO_MEM_LOCK(m_mutex);
      return m_queue.empty();
    }
    T front()
    {
      AUTO_MEM_LOCK(m_mutex);
      return m_queue.front();
    }
    T front_and_pop()
    {
      AUTO_MEM_LOCK(m_mutex);
      T data = m_queue.front();
      m_queue.pop_front();
      return data;
    }
    T back()
    {
      AUTO_MEM_LOCK(m_mutex);
      return m_queue.back();
    }
    T back_and_pop()
    {
      AUTO_MEM_LOCK(m_mutex);
      T data = m_queue.back();
      m_queue.pop_back();
      return data;
    }
    void pop_front()
    {
      AUTO_MEM_LOCK(m_mutex);
      m_queue.pop_front();
    }
    void pop_back()
    {
      AUTO_MEM_LOCK(m_mutex);
      m_queue.pop_back();
    }
    void push_front(T data)
    {
      AUTO_MEM_LOCK(m_mutex);
      m_queue.push_front(data);
    }
    void push_back(T data)
    {
      AUTO_MEM_LOCK(m_mutex);
      m_queue.push_back(data);
    }
    size_t size()
    {
      AUTO_MEM_LOCK(m_mutex);
      return m_queue.size();
    }

  protected:
    std::deque<T> m_queue;
    AMMemLock     m_mutex;
};

#endif /* ORYX_INCLUDE_UTILITY_AM_QUEUE_H_ */
