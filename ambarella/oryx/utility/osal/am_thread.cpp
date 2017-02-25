/*******************************************************************************
 * am_thread.cpp
 *
 * History:
 *   2014-7-18 - [ypchang] created file
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
    m_thread_id(-1),
    m_thread_running(false),
    m_thread_created(false)
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
