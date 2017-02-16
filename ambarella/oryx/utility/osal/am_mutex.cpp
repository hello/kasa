/*******************************************************************************
 * am_mutex.cpp
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

#include "am_mutex.h"
#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"

#include <pthread.h>
#include <time.h>
#include <atomic>

/*
 * AMSpinLock
 */
struct AMSpinLockData
{
    std::atomic_flag lock;
    AMSpinLockData() :
        lock(ATOMIC_FLAG_INIT)
    {
    }
};

AMSpinLock* AMSpinLock::create()
{
  AMSpinLock *result = new AMSpinLock();
  if (AM_UNLIKELY(result && !result->init())) {
    delete result;
    result = NULL;
  }
  return result;
}

void AMSpinLock::destroy()
{
  delete this;
}

void AMSpinLock::lock()
{
  while (m_lock->lock.test_and_set(std::memory_order_acquire)) {
    /* Spin Lock */
  }
}

void AMSpinLock::unlock()
{
  m_lock->lock.clear(std::memory_order_release);
}

AMSpinLock::AMSpinLock() :
    m_lock(NULL)
{
}

AMSpinLock::~AMSpinLock()
{
  delete m_lock;
}

bool AMSpinLock::init()
{
  m_lock = new AMSpinLockData();
  return (m_lock != NULL);
}

/*
 * AMMutex
 */
struct AMMutexData
{
    pthread_mutex_t m_mutex_data;
    AMMutexData(bool recursive)
    {
      if (recursive) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&m_mutex_data, &attr);
      } else {
        pthread_mutex_init(&m_mutex_data, NULL);
      }
    }
    ~AMMutexData()
    {
      pthread_mutex_destroy(&m_mutex_data);
    }
};

AMMutex* AMMutex::create(bool recursive)
{
  AMMutex *result = new AMMutex();
  if (AM_UNLIKELY(result && !result->init(recursive))) {
    delete result;
    result = NULL;
  }

  return result;
}

void AMMutex::destroy()
{
  delete this;
}

void AMMutex::lock()
{
  pthread_mutex_lock(&m_mutex->m_mutex_data);
}
void AMMutex::unlock()
{
  pthread_mutex_unlock(&m_mutex->m_mutex_data);
}
bool AMMutex::try_lock()
{
  return (0 == pthread_mutex_trylock(&m_mutex->m_mutex_data));
}

AMMutex::AMMutex() :
    m_mutex(NULL)
{
}

AMMutex::~AMMutex()
{
  delete m_mutex;
}

bool AMMutex::init(bool recursive)
{
  m_mutex = new AMMutexData(recursive);
  return (NULL != m_mutex);
}

/*
 * AMCondition
 */
struct AMConditionData
{
    pthread_cond_t m_cond_data;
    AMConditionData()
    {
      pthread_cond_init(&m_cond_data, NULL);
    }
    ~AMConditionData()
    {
      pthread_cond_destroy(&m_cond_data);
    }
};

AMCondition* AMCondition::create()
{
  AMCondition *result = new AMCondition();
  if (AM_UNLIKELY(result && !result->init())) {
    delete result;
    result = NULL;
  }

  return result;
}

void AMCondition::destroy()
{
  delete this;
}

bool AMCondition::wait(AMMutex *mutex, int64_t ms)
{
  bool ret = true;
  if (ms < 0) {
    pthread_cond_wait(&m_cond->m_cond_data, &mutex->m_mutex->m_mutex_data);
  } else {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += ms / 1000;
    ts.tv_nsec += (ms % 1000) * 1000000;
    ret = (pthread_cond_timedwait(&m_cond->m_cond_data,
                                  &mutex->m_mutex->m_mutex_data,
                                  &ts) == 0);
  }

  return ret;
}

void AMCondition::signal()
{
  pthread_cond_signal(&m_cond->m_cond_data);
}

void AMCondition::signal_all()
{
  pthread_cond_broadcast(&m_cond->m_cond_data);
}

AMCondition::AMCondition() :
    m_cond(NULL)
{
}

AMCondition::~AMCondition()
{
  delete m_cond;
}

bool AMCondition::init()
{
  m_cond = new AMConditionData();
  return (NULL != m_cond);
}

