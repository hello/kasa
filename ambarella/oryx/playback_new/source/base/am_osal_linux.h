/**
 * osal_linux.h
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

#ifndef __COMMON_OSAL_GENERIC_H__
#define __COMMON_OSAL_GENERIC_H__

//-----------------------------------------------------------------------
//
// CIMutex
//
//-----------------------------------------------------------------------
class CIMutex: public IMutex
{
public:
  static CIMutex *Create(bool bRecursive = true);
  virtual void Delete();

public:
  virtual void Lock();
  virtual void Unlock();

public:
  virtual void *GetContext() const;

private:
  CIMutex();
  EECode Construct(bool bRecursive);
  virtual ~CIMutex();

private:
  pthread_mutex_t mMutex;
};

//-----------------------------------------------------------------------
//
// CICondition
//
//-----------------------------------------------------------------------

class CICondition: public ICondition
{
public:
  static CICondition *Create();
  virtual void Delete();

public:
  virtual void Wait(IMutex *pMutex);
  //virtual EECode TimeoutWait(IMutex *pMutex, TU32 seconds);
  virtual void Signal();
  virtual void SignalAll();

private:
  CICondition();
  virtual ~CICondition();
  EECode Construct();

private:
  pthread_cond_t mCond;
};

//-----------------------------------------------------------------------
//
// CIEvent
//
//-----------------------------------------------------------------------
class CIEvent : public IEvent
{
public:
  static CIEvent *Create();
  virtual void Delete();

public:
  virtual EECode Wait(TInt ms = -1);
  virtual void Signal();
  virtual void Clear();

private:
  CIEvent();
  EECode Construct();
  virtual ~CIEvent();

private:
  sem_t   mEvent;
};

//-----------------------------------------------------------------------
//
// CIThread
//
//-----------------------------------------------------------------------
class CIThread: public IThread
{
public:
  static CIThread *Create(const TChar *pName, TF_THREAD_FUNC entry, void *pParam, TUint schedule_policy = ESchedulePolicy_Other, TUint priority = 0, TUint affinity = 0);
  virtual void Delete();

public:
  virtual const TChar *Name() const;

private:
  CIThread(const TChar *pName);
  EECode Construct(TF_THREAD_FUNC entry, void *pParam, TUint schedule_policy = ESchedulePolicy_Other, TUint priority = 0, TUint affinity = 0);
  virtual ~CIThread();

private:
  static void *__Entry(void *);

private:
  TU8 mbThreadCreated, mReserved0, mReserved1, mReserved2;
  pthread_t   mThread;
  const TChar  *mpName;

  TF_THREAD_FUNC  mEntry;
  void    *mpParam;
};

//-----------------------------------------------------------------------
//
// CIAutoLock
//
//-----------------------------------------------------------------------

CIAutoLock::CIAutoLock(IMutex *pMutex): _pMutex(pMutex)
{
  if (_pMutex) {
    _pMutex->Lock();
  }
}

CIAutoLock::~CIAutoLock()
{
  if (_pMutex) {
    _pMutex->Unlock();
  }
}

//-----------------------------------------------------------------------
//
// CIClockSourceGenericLinux
//
//-----------------------------------------------------------------------
class CIClockSourceGenericLinux: public CObject, public IClockSource
{
  typedef CObject inherited;

public:
  static IClockSource *Create();
  virtual EECode Construct();
  virtual void Delete();
  virtual CObject *GetObject0() const;

protected:
  CIClockSourceGenericLinux();
  ~CIClockSourceGenericLinux();

public:
  virtual TTime GetClockTime(TUint relative = 1) const;
  virtual TTime GetClockBase() const;

  virtual void SetClockFrequency(TUint num, TUint den);
  virtual void GetClockFrequency(TUint &num, TUint &den) const;

  virtual void SetClockState(EClockSourceState state);
  virtual EClockSourceState GetClockState() const;
  virtual void UpdateTime();

  void PrintStatus();

private:
  TU8 mbUseNativeUSecond;
  TU8 msState;
  TU8 mbGetBaseTime;
  TU8 mReserved2;

  TUint mTimeUintDen;
  TUint mTimeUintNum;

  TTime mSourceCurrentTimeTick;
  TTime mSourceBaseTimeTick;

  TTime mCurrentTimeTick;

private:
  IMutex *mpMutex;
};

#endif



