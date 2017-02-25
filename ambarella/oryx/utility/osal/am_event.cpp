/*******************************************************************************
 * am_event.cpp
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

#include "am_event.h"
#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include <semaphore.h>
#include <time.h>

struct AMEventData
{
    sem_t m_event_data;
    AMEventData()
    {
      sem_init(&m_event_data, 0, 0);
    }
    ~AMEventData()
    {
      sem_destroy(&m_event_data);
    }
};

AMEvent* AMEvent::create()
{
  AMEvent *result = new AMEvent();
  if (AM_UNLIKELY(result && !result->init())) {
    delete result;
    result = NULL;
  }
  return result;
}

void AMEvent::destroy()
{
  delete this;
}

bool AMEvent::wait(int64_t ms)
{
  bool ret = true;
  if (ms < 0) {
    sem_wait(&m_event->m_event_data);
  } else {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += ms / 1000;
    ts.tv_nsec += (ms % 1000) * 1000000;
    ret = (sem_timedwait(&m_event->m_event_data, &ts) == 0);
  }

  return ret;
}

void AMEvent::signal()
{
  sem_trywait(&m_event->m_event_data);
  sem_post(&m_event->m_event_data);
}

void AMEvent::clear()
{
  sem_trywait(&m_event->m_event_data);
}

AMEvent::AMEvent() :
    m_event(NULL)
{
}

AMEvent::~AMEvent()
{
  delete m_event;
}

bool AMEvent::init()
{
  m_event = new AMEventData();
  return (NULL != m_event);
}
