/**
 * osal_linux.h
 *
 * History:
 *  2015/07/27 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
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



