/*******************************************************************************
 * common_osal_generic.cpp
 *
 * History:
 *  2012/12/07 - [Zhi He] create file
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

#include <semaphore.h>
#include <pthread.h>

//#include <sys/time.h>

#ifdef BUILD_OS_IOS
#include <sched.h>
#include <unistd.h>
#endif

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "common_osal_generic.h"

//#include "../linux/common_osal_linux.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

extern EECode gfInitPlatformNetwork();
extern void gfDeInitPlatformNetwork();

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

#if 0
EECode CICondition::TimeoutWait(IMutex *pMutex, TU32 seconds)
{
    struct timeval now;
    struct timespec timeout;
    TInt retcode = 0;
    gettimeofday(&now);

    timeout.tv_sec = now.tv_sec + seconds;
    timeout.tv_nsec = now.tv_usec * 1000;

    retcode = pthread_cond_timewait(&mCond, (pthread_mutex_t *)pMutex->GetContext(), &timeout);
    if (retcode == ETIMEDOUT) {
        return EECode_TimeOut;
    }

    return EECode_OK;
}
#endif

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
        return EECode_OK;
    } else {
#ifdef BUILD_OS_IOS
        // FIXME
        // workaround for replacement for sem_timedwait
        const TInt SLEEP_SLICE = 1; // ms
        TInt time = 0;
        while (time < ms) {
            if (sem_trywait(&mEvent) == 0) {
                return EECode_OK;
            }
            sleep(SLEEP_SLICE);
            time++;
        }
        return EECode_TimeTick;
#elif defined(BUILD_OS_LINUX)
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);

        ts.tv_sec += ms / 1000;
        ts.tv_nsec += (ms % 1000) * 1000000;
        return (sem_timedwait(&mEvent, &ts) == 0) ?
               EECode_OK : EECode_TimeTick;
#else
#if 0
        const TInt SLEEP_SLICE = 10;
        TInt time = 0;
        while (time < ms) {
            if (sem_trywait(&mEvent) == 0) {
                return EECode_OK;
            }
            gfOSmsleep(SLEEP_SLICE);
            time += SLEEP_SLICE;
        }
        return EECode_TimeTick;
#else
        //if (sem_trywait(&mEvent) == 0) {
        //    return EECode_OK;
        //}
        gfOSmsleep(1);
        return EECode_TimeTick;
#endif
#endif
    }
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

EECode gfInitPlatform()
{
    pthread_mutex_init(&g_mutex, NULL);

    EECode err = gfInitPlatformNetwork();
    if (DUnlikely(EECode_OK != err)) {
        LOG_FATAL("gfInitPlatformNetwork() fail\n");
        return err;
    }

    return EECode_OK;
}

void gfDeInitPlatform()
{
    gfDeInitPlatformNetwork();
    pthread_mutex_destroy(&g_mutex);
}

void gfGlobalMutexLock()
{
    pthread_mutex_lock(&g_mutex);
}

void gfGlobalMutexUnLock()
{
    pthread_mutex_unlock(&g_mutex);
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

