/*******************************************************************************
 * am_mutex.cpp
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

#include "am_mutex.h"
#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"

#include <pthread.h>
#include <time.h>
#include <atomic>

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

