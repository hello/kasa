/*******************************************************************************
 * am_thread.cpp
 *
 * History:
 *   2014-7-18 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#include "am_base_include.h"
#include "am_define.h"
#include "am_thread.h"
#include "am_log.h"

#include <unistd.h>
#include <sys/syscall.h>

#define gettid() syscall(__NR_gettid)

AMThread* AMThread::create(const char      *name,
                           AmThreadFuncType entry,
                           void            *data)
{
  AMThread *result = new AMThread();
  if (AM_UNLIKELY(result && !result->init(name, entry, data))) {
    delete result;
    result = NULL;
  }

  return result;
}

AMThread* AMThread::create(const std::string& name,
                           AmThreadFuncType entry,
                           void *data)
{
  return AMThread::create(name.c_str(), entry, data);
}

void AMThread::destroy()
{
  delete this;
}

bool AMThread::set_priority(int prio)
{
  bool ret = false;

  if (AM_LIKELY(m_thread_id != (pthread_t)-1)) {
    struct sched_param param;

    param.sched_priority = prio;

    if (pthread_setschedparam(m_thread_id, SCHED_RR, &param) < 0) {
      PERROR("sched_setscheduler");
    } else {
      ret = true;
    }
  } else {
    ERROR("Thread %s has invalid thread ID!", m_thread_name);
  }

  return ret;
}

AMThread::AMThread() :
    m_thread_entry(NULL),
    m_thread_name(NULL),
    m_thread_data(NULL),
    m_thread_running(false),
    m_thread_created(false),
    m_thread_id(-1)
{
}

AMThread::~AMThread()
{
  INFO("Destroying thread %s...", m_thread_name);
  if (AM_LIKELY(m_thread_created)) {
    pthread_join(m_thread_id, NULL);
  }
  INFO("Thread %s is destroyed...", m_thread_name);
  delete[] m_thread_name;
}

bool AMThread::init(const char *name, AmThreadFuncType entry, void *data)
{
  bool ret = false;

  if (AM_LIKELY(name && entry)) {
    delete[] m_thread_name;
    m_thread_name  = amstrdup(name);
    m_thread_entry = entry;
    m_thread_data  = data;
    m_thread_running = false;

    if (AM_UNLIKELY(0 != pthread_create(&m_thread_id,
                                        NULL,
                                        static_entry,
                                        (void*)this))) {
      ERROR("Failed to create thread %s: %s", name, strerror(errno));
    } else {
      if (AM_UNLIKELY(pthread_setname_np(m_thread_id, m_thread_name) < 0)) {
        ERROR("Failed to set thread %s name: %s",
              m_thread_name, strerror(errno));
      }
      ret = true;
      m_thread_created = true;
    }
  }

  return ret;
}

void* AMThread::static_entry(void* data)
{
  AMThread *thread = (AMThread*)data;

  INFO("Thread %s is created, tid is %ld", thread->m_thread_name, gettid());

  thread->m_thread_running = true;
  thread->m_thread_entry(thread->m_thread_data);
  thread->m_thread_running = false;

  INFO("Thread %s exits!", thread->m_thread_name);

  return NULL;
}
