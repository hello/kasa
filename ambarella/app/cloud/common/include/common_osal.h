/*******************************************************************************
 * common_osal.h
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

#ifndef __COMMON_OSAL_H__
#define __COMMON_OSAL_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// IMutex
//
//-----------------------------------------------------------------------
class IMutex
{
public:
    virtual void Delete() = 0;

protected:
    virtual ~IMutex() {}

public:
    virtual void Lock() = 0;
    virtual void Unlock() = 0;

public:
    virtual void *GetContext() const = 0;
};

//-----------------------------------------------------------------------
//
// ICondition
//
//-----------------------------------------------------------------------
class ICondition
{
public:
    virtual void Delete() = 0;

protected:
    virtual ~ICondition() {}

public:
    virtual void Wait(IMutex *pMutex) = 0;
    //virtual EECode TimeoutWait(IMutex *pMutex, TU32 seconds) = 0;
    virtual void Signal() = 0;
    virtual void SignalAll() = 0;
};

//-----------------------------------------------------------------------
//
// CIAutoLock
//
//-----------------------------------------------------------------------
class CIAutoLock
{
public:
    CIAutoLock(IMutex *pMutex);
    ~CIAutoLock();

private:
    IMutex *_pMutex;
};

#define AUTO_LOCK(pMutex)   CIAutoLock __lock__(pMutex)
#define __LOCK(pMutex)      pMutex->Lock()
#define __UNLOCK(pMutex)    pMutex->Unlock()

//-----------------------------------------------------------------------
//
// IEvent
//
//-----------------------------------------------------------------------
class IEvent
{
public:
    virtual void Delete() = 0;

protected:
    virtual ~IEvent() {}

public:
    virtual EECode Wait(TInt ms = -1) = 0;
    virtual void Signal() = 0;
    virtual void Clear() = 0;
};

//-----------------------------------------------------------------------
//
// IThread
//
//-----------------------------------------------------------------------
enum {
    ESchedulePolicy_Other = 0,
    ESchedulePolicy_FIFO,
    ESchedulePolicy_RunRobin,
};

typedef EECode(*TF_THREAD_FUNC)(void *);
class IThread
{
public:
    virtual const TChar *Name() const = 0;
    virtual void Delete() = 0;

protected:
    virtual ~IThread() {}
};

extern TInt __atomic_inc(TAtomic *value);
extern TInt __atomic_dec(TAtomic *value);

extern EECode gfGetCurrentDateTime(SDateTime *datetime);
extern TTime gfGetNTPTime();
extern TTime gfGetRalativeTime();
extern void gfGetDateString(TChar *pbuffer, TUint size);

extern IMutex *gfCreateMutex();
extern ICondition *gfCreateCondition();
extern IEvent *gfCreateEvent();
extern IThread *gfCreateThread(const TChar *pName, TF_THREAD_FUNC entry, void *pParam, TUint schedule_policy = ESchedulePolicy_Other, TUint priority = 0, TUint affinity = 0);

extern EECode gfOSWritePipe(TSocketHandler fd, const TChar byte);
extern EECode gfOSReadPipe(TSocketHandler fd, TChar &byte);
extern EECode gfOSCreatePipe(TSocketHandler fd[2]);
extern void gfOSClosePipeFd(TSocketHandler fd);

extern EECode gfOSGetHostName(TChar *name, TU32 name_buffer_length);

extern EECode gfOSPollFdReadable(THandler fd);

extern void gfOSmsleep(TU32 ms);
extern void gfOSusleep(TU32 us);
extern void gfOSsleep(TU32 s);

extern void gfGetHostName(TChar *name, TU32 name_buffer_size);
extern void gfGetSTRFTimeString(TChar *time_string, TU32 time_string_buffer_length);

extern EECode gfGetSystemSettings(SSystemSettings *p_settings);
extern EECode gfGetSystemSoundInputDevices(SSystemSoundInputDevices *p_devices);

//extern TInt gfOsIsExist(const TChar *name);
//extern TInt gfOsIsDir(const TChar *file);
//extern TInt gfOsIsFile(const TChar *path);

EECode gfInitPlatform();
void gfDeInitPlatform();

void gfGlobalMutexLock();
void gfGlobalMutexUnLock();

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

