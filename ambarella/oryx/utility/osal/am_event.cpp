/*******************************************************************************
 * am_event.cpp
 *
 * History:
 *   2014-7-22 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
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
