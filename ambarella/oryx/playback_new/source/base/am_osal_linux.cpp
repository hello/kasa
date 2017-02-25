/**
 * am_osal_linux.cpp
 *
 * History:
 *  2015/07/27 - [Zhi He] create file
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
 */

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include <semaphore.h>
#include <pthread.h>
#include <linux/sched.h>
#include <unwind.h>
#include <stdint.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <poll.h>

#include "am_playback_new_if.h"

#include "am_native.h"
#include "am_native_log.h"

#include "am_internal.h"

#include "am_osal.h"
#include "am_osal_linux.h"

//-----------------------------------------------------------------------
//
// CIMutex
//
//-----------------------------------------------------------------------

CIMutex *CIMutex::Create(bool bRecursive)
{
  CIMutex *result = new CIMutex;
  if (result && (EECode_OK != result->Construct(bRecursive))) {
    delete result;
    result = NULL;
  }
  return result;
}

void CIMutex::Delete()
{
  delete this;
}

CIMutex::CIMutex()
{
}
CIMutex::~CIMutex()
{
  pthread_mutex_destroy(&mMutex);
}

EECode CIMutex::Construct(bool bRecursive)
{
  if (bRecursive) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mMutex, &attr);
  } else {
    // a littler faster than recursive
    pthread_mutex_init(&mMutex, NULL);
  }
  return EECode_OK;
}

void CIMutex::Lock()
{
  TInt ret = pthread_mutex_lock(&mMutex);
  DASSERT(0 == ret);
}

void CIMutex::Unlock()
{
  TInt ret = pthread_mutex_unlock(&mMutex);
  DASSERT(0 == ret);
}

void *CIMutex::GetContext() const
{
  return (void *)&mMutex;
}

//-----------------------------------------------------------------------
//
// CICondition
//
//-----------------------------------------------------------------------

CICondition *CICondition::Create()
{
  CICondition *result = new CICondition;
  if (result && result->Construct() != EECode_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

void CICondition::Delete()
{
  delete this;
}

CICondition::CICondition()
{
}
CICondition::~CICondition()
{
  pthread_cond_destroy(&mCond);
}

EECode CICondition::Construct()
{
  TInt ret = 0;
  ret = pthread_cond_init(&mCond, NULL);
  if (0 != ret) {
    LOG_FATAL("pthread_cond_init fail\n");
    return EECode_BadState;
  }
  return EECode_OK;
}

void CICondition::Wait(IMutex *pMutex)
{
  TInt ret = 0;
  ret = pthread_cond_wait(&mCond, (pthread_mutex_t *)pMutex->GetContext());
  DASSERT(0 == ret);
}

void CICondition::Signal()
{
  TInt ret = 0;
  ret = pthread_cond_signal(&mCond);
  DASSERT(0 == ret);
}

void CICondition::SignalAll()
{
  TInt ret = 0;
  ret = pthread_cond_broadcast(&mCond);
  DASSERT(0 == ret);
}

//-----------------------------------------------------------------------
//
// CIEvent
//
//-----------------------------------------------------------------------

CIEvent *CIEvent::Create()
{
  CIEvent *result = new CIEvent;
  if (result && (EECode_OK != result->Construct())) {
    delete result;
    result = NULL;
  }
  return result;
}

EECode CIEvent::Construct()
{
  sem_init(&mEvent, 0, 0);
  return EECode_OK;
}

void CIEvent::Delete()
{
  delete this;
}

EECode CIEvent::Wait(TInt ms)
{
  if (ms < 0) {
    sem_wait(&mEvent);
  } else {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += ms / 1000;
    ts.tv_nsec += (ms % 1000) * 1000000;
    return (sem_timedwait(&mEvent, &ts) == 0) ?
           EECode_OK : EECode_TimeOut;
  }
  return EECode_OK;
}

void CIEvent::Signal()
{
  sem_trywait(&mEvent);
  sem_post(&mEvent);
}

void CIEvent::Clear()
{
  sem_trywait(&mEvent);
}

CIEvent::CIEvent()
{
}

CIEvent::~CIEvent()
{
  sem_destroy(&mEvent);
}

//-----------------------------------------------------------------------
//
// CIThread
//
//-----------------------------------------------------------------------

CIThread *CIThread::Create(const TChar *pName, TF_THREAD_FUNC entry, void *pParam, TUint schedule, TUint priority, TUint affinity)
{
  CIThread *result = new CIThread(pName);
  if (result && (EECode_OK != result->Construct(entry, pParam, schedule, priority, affinity))) {
    delete result;
    result = NULL;
  }
  return result;
}

CIThread::CIThread(const char *pName)
  : mbThreadCreated(0)
  , mpName(pName)
  , mEntry(NULL)
  , mpParam(NULL)
{
#ifndef BUILD_OS_WINDOWS
  mThread = 0;
#endif
}

EECode CIThread::Construct(TF_THREAD_FUNC entry, void *pParam, TUint schedule_policy, TUint priority, TUint affinity)
{
  TInt ret = 0;
  pthread_attr_t attr;
  struct sched_param param;
  pthread_attr_init(&attr);
  if (ESchedulePolicy_RunRobin == schedule_policy) {
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
  } else if (ESchedulePolicy_FIFO == schedule_policy) {
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  } else if (ESchedulePolicy_Other == schedule_policy) {
    pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
  } else {
    LOG_FATAL("BAD schedule policy %d\n", schedule_policy);
    return EECode_BadParam;
  }
  param.sched_priority = priority;
  pthread_attr_setschedparam(&attr, &param);
#ifdef BUILD_OS_LINUX
  if (affinity & 0x3) {
    cpu_set_t cpu_info;
    CPU_ZERO(&cpu_info);
    for (ret = 0; ret < 2; ret ++) {
      if (affinity & (1 << ret)) {
        CPU_SET(ret, &cpu_info);
      }
    }
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu_info);
  }
#endif
  mEntry = entry;
  mpParam = pParam;
  ret = pthread_create(&mThread, NULL, __Entry, (void *)this);
  if (ret) {
    LOG_FATAL("pthread_create fail, return %d\n", ret);
    return EECode_OSError;
  }
  //ret = pthread_setname_np(mThread, mpName);
  //if (ret) {
    //LOG_ERROR("pthread_setname_np fail, return %d\n", ret);
    //return EECode_OSError;
  //}
  //LOG_NOTICE("thread %s created\n", mpName);
  mbThreadCreated = 1;
  return EECode_OK;
}

CIThread::~CIThread()
{
  if (mbThreadCreated) {
    pthread_join(mThread, NULL);
    mbThreadCreated = 0;
  }
}

void CIThread::Delete()
{
  delete this;
}

const TChar *CIThread::Name() const
{
  return mpName;
}

void *CIThread::__Entry(void *p)
{
  CIThread *pthis = (CIThread *)p;
  pthis->mEntry(pthis->mpParam);
  return NULL;
}

//-----------------------------------------------------------------------
//
// CIClockSourceGenericLinux
//
//-----------------------------------------------------------------------
IClockSource *gfCreateGenericClockSource()
{
  return CIClockSourceGenericLinux::Create();
}

IClockSource *CIClockSourceGenericLinux::Create()
{
  CIClockSourceGenericLinux *result = new CIClockSourceGenericLinux();
  if (result && (EECode_OK != result->Construct())) {
    delete result;
    result = NULL;
  }
  DASSERT(result);
  return result;
}

EECode CIClockSourceGenericLinux::Construct()
{
  mpMutex = gfCreateMutex();
  if (DUnlikely(!mpMutex)) {
    LOGM_FATAL("gfCreateMutex() fail\n");
    return EECode_NoMemory;
  }
  return EECode_OK;
}

void CIClockSourceGenericLinux::Delete()
{
  if (mpMutex) {
    mpMutex->Delete();
    mpMutex = NULL;
  }
  inherited::Delete();
  return;
}

CObject *CIClockSourceGenericLinux::GetObject0() const
{
  return (CObject *) this;
}

CIClockSourceGenericLinux::CIClockSourceGenericLinux()
  : inherited("CIClockSourceGenericLinux")
  , mbUseNativeUSecond(1)
  , msState(EClockSourceState_stopped)
  , mbGetBaseTime(0)
  , mTimeUintDen(1000000)
  , mTimeUintNum(1)
  , mSourceCurrentTimeTick(0)
  , mSourceBaseTimeTick(0)
  , mCurrentTimeTick(0)
  , mpMutex(NULL)
{
}

CIClockSourceGenericLinux::~CIClockSourceGenericLinux()
{
}


TTime CIClockSourceGenericLinux::GetClockTime(TUint relative) const
{
  AUTO_LOCK(mpMutex);
  if (DLikely(relative)) {
    if (DLikely(mbUseNativeUSecond)) {
      return mCurrentTimeTick;
    } else {
      return (mCurrentTimeTick) * 10000000 / mTimeUintDen * mTimeUintNum;
    }
  } else {
    if (DLikely(mbUseNativeUSecond)) {
      return mSourceCurrentTimeTick;
    } else {
      return mSourceCurrentTimeTick / mTimeUintDen * 1000000 * mTimeUintNum;
    }
  }
}

TTime CIClockSourceGenericLinux::GetClockBase() const
{
  AUTO_LOCK(mpMutex);
  if (DLikely(mbUseNativeUSecond)) {
    return mSourceBaseTimeTick;
  } else {
    return mSourceBaseTimeTick * 10000000 / mTimeUintDen * mTimeUintNum;
  }
}

void CIClockSourceGenericLinux::SetClockFrequency(TUint num, TUint den)
{
  AUTO_LOCK(mpMutex);
  DASSERT(num);
  DASSERT(den);
  if (num && den) {
    mTimeUintDen = num;
    mTimeUintNum = den;
    if ((1000000 != mTimeUintDen) || (1 != mTimeUintNum)) {
      LOGM_WARN("not u second!\n");
      mbUseNativeUSecond = 0;
    }
  }
}

void CIClockSourceGenericLinux::GetClockFrequency(TUint &num, TUint &den) const
{
  AUTO_LOCK(mpMutex);
  num = mTimeUintDen;
  den = mTimeUintNum;
}

void CIClockSourceGenericLinux::SetClockState(EClockSourceState state)
{
  AUTO_LOCK(mpMutex);
  msState = (TU8)state;
}

EClockSourceState CIClockSourceGenericLinux::GetClockState() const
{
  AUTO_LOCK(mpMutex);
  return (EClockSourceState)msState;
}

void CIClockSourceGenericLinux::UpdateTime()
{
  AUTO_LOCK(mpMutex);
  if (msState != EClockSourceState_running) {
    return;
  }
  struct timeval time;
  gettimeofday(&time, NULL);
  TTime tmp = (TTime)time.tv_sec * 1000000 + (TTime)time.tv_usec;
  if (DUnlikely(!mbGetBaseTime)) {
    if (DLikely(mbUseNativeUSecond)) {
      mSourceBaseTimeTick = mSourceCurrentTimeTick = tmp;
    } else {
      mSourceBaseTimeTick = mSourceCurrentTimeTick = tmp / mTimeUintDen * 1000000 * mTimeUintNum;
    }
    mbGetBaseTime = 1;
    mCurrentTimeTick = 0;
  } else {
    if (DLikely(mbUseNativeUSecond)) {
      mSourceCurrentTimeTick = tmp;
    } else {
      mSourceCurrentTimeTick = tmp / mTimeUintDen * 1000000 * mTimeUintNum;
    }
    mCurrentTimeTick = mSourceCurrentTimeTick - mSourceBaseTimeTick;
  }
}

void CIClockSourceGenericLinux::PrintStatus()
{
  AUTO_LOCK(mpMutex);
  LOGM_ALWAYS("mbGetBaseTime %d, mbUseNativeUSecond %d, msState %d\n", mbGetBaseTime, mbUseNativeUSecond, msState);
  LOGM_ALWAYS("mSourceBaseTimeTick %" DPrint64d ", mSourceCurrentTimeTick %" DPrint64d ", mCurrentTimeTick %" DPrint64d "\n", mSourceBaseTimeTick, mSourceCurrentTimeTick, mCurrentTimeTick);
}

static pthread_mutex_t g_mutex;
TInt __atomic_inc(TAtomic *value)
{
  TInt ret;
  pthread_mutex_lock(&g_mutex);
  ret = *value;
  *value = ret + 1;
  pthread_mutex_unlock(&g_mutex);
  return ret;
}

TInt __atomic_dec(TAtomic *value)
{
  TInt ret;
  pthread_mutex_lock(&g_mutex);
  ret = *value;
  *value = ret - 1;
  pthread_mutex_unlock(&g_mutex);
  return ret;
}

IMutex *gfCreateMutex()
{
  return CIMutex::Create(false);
}

ICondition *gfCreateCondition()
{
  return CICondition::Create();
}

IEvent *gfCreateEvent()
{
  return CIEvent::Create();
}

IThread *gfCreateThread(const TChar *pName, TF_THREAD_FUNC entry, void *pParam, TUint schedule, TUint priority, TUint affinity)
{
  return CIThread::Create(pName, entry, pParam, schedule, priority, affinity);
}

extern IClockSource *gfCreateGenericClockSource();

IClockSource *gfCreateClockSource(EClockSourceType type)
{
  IClockSource *thiz = NULL;
  switch (type) {
    case EClockSourceType_generic:
      thiz = gfCreateGenericClockSource();
      break;
    default:
      LOG_FATAL("BAD EClockSourceType %d\n", type);
      break;
  }
  return thiz;
}

EECode gfGetCurrentDateTime(SDateTime *datetime)
{
  time_t timer;
  struct tm *tblock = NULL;
  if (DUnlikely(!datetime)) {
    LOG_ERROR("NULL params\n");
    return EECode_BadParam;
  }
  timer = time(NULL);
  tblock = localtime(&timer);
  DASSERT(tblock);
  datetime->year = tblock->tm_year + 1900;
  datetime->month = tblock->tm_mon + 1;
  datetime->day = tblock->tm_mday;
  datetime->hour = tblock->tm_hour;
  datetime->minute = tblock->tm_min;
  datetime->seconds = tblock->tm_sec;
  datetime->weekday = tblock->tm_wday;
  return EECode_OK;
}

TTime gfGetNTPTime()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  TTime systime = (TTime)tv.tv_sec * 1000000 + tv.tv_usec;
#define NTP_OFFSET 2208988800ULL
#define NTP_OFFSET_US (NTP_OFFSET * 1000000ULL)
  TTime ntp_time = (systime / 1000 * 1000) + NTP_OFFSET_US;
  return ntp_time;
}

TTime gfGetRalativeTime()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((TTime)tv.tv_sec * 1000000 + tv.tv_usec);
}

void gfGetDateString(TChar *pbuffer, TUint size)
{
  time_t tt = time(NULL);
  strftime(pbuffer, size, "Date: %a, %b %d %Y %H:%M:%S GMT\r\n", gmtime(&tt));
}

EECode gfOSWritePipe(TSocketHandler fd, const TChar byte)
{
  size_t ret = 0;
  ret = write(fd, &byte, 1);
  if (DLikely(1 == ret)) {
    return EECode_OK;
  }
  return EECode_OSError;
}

EECode gfOSReadPipe(TSocketHandler fd, TChar &byte)
{
  size_t ret = 0;
  ret = read(fd, &byte, 1);
  if (DLikely(1 == ret)) {
    return EECode_OK;
  }
  return EECode_OSError;
}

EECode gfOSCreatePipe(TSocketHandler fd[2])
{
  TInt ret = pipe(fd);
  if (DLikely(0 == ret)) {
    return EECode_OK;
  }
  return EECode_OSError;
}

void gfOSClosePipeFd(TSocketHandler fd)
{
  close(fd);
}

EECode gfOSGetHostName(TChar *name, TU32 name_buffer_length)
{
  gethostname(name, name_buffer_length);
  return EECode_OK;
}

EECode gfOSPollFdReadable(TInt fd)
{
  struct pollfd fds;
  fds.fd = fd;
  fds.events = POLLIN;
  fds.revents = 0;
  if (poll(&fds, 1, 0) <= 0) {
    return EECode_TryAgain;
  }
  return EECode_OK;
}

void gfOSmsleep(TU32 ms)
{
  usleep(1000 * ms);
}

void gfOSusleep(TU32 us)
{
  usleep(us);
}

void gfOSsleep(TU32 s)
{
  sleep(s);
}

void gfGetSTRFTimeString(TChar *time_string, TU32 time_string_buffer_length)
{
  time_t cur_time = time(NULL);
  strftime(time_string, time_string_buffer_length, "%Z%Y%m%d_%H%M%S", gmtime(&cur_time));
}

