/**
 * common_osal_generic.h
 *
 * History:
 *  2014/07/24 - [Zhi He] create file
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

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

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

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

