/**
 * common_schedular.h
 *
 * History:
 *  2013/07/12 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __COMMON_SCHEDULAR_H__
#define __COMMON_SCHEDULAR_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

class CIRunRobinScheduler: public CObject, virtual public IScheduler
{
    typedef CObject inherited;
public:
    static IScheduler *Create(const TChar *name, TUint index);
    virtual CObject *GetObject0() const;

protected:
    CIRunRobinScheduler(const TChar *name, TUint index);
    virtual ~CIRunRobinScheduler();
    EECode Construct();

public:
    virtual void Delete();

public:
    virtual void PrintStatus();

public:
    virtual EECode StartScheduling();
    virtual EECode StopScheduling();

public:
    virtual EECode AddScheduledCilent(IScheduledClient *client);
    virtual EECode RemoveScheduledCilent(IScheduledClient *client);

public:
    virtual TUint TotalNumberOfScheduledClient() const;
    virtual TUint CurrentNumberOfClient() const;
    virtual TSchedulingUnit CurrentHungryScore() const;

public:
    virtual TInt IsPassiveMode() const;


protected:
    // return receive's reply
    EECode sendCmd(SCMD &cmd);
    void getCmd(SCMD &cmd);
    TU8 peekCmd(SCMD &cmd);

    EECode postMsg(SCMD &cmd);
    void cmdAck(EECode result);

private:
    void processCmd(SCMD &cmd);
    void mainLoop();
    static EECode threadEntry(void *p);

private:
    CIQueue *mpMsgQ;
    IThread *mpThread;

private:
    TU8 mbStarted;
    TU8 mbThreadRun;
    TU8 mbRun;
    TU8 mbRunning;

    EModuleState msState;

    //TUint mnTotalClientNumber;

    TUint mnCurrentActiveClientNumber;
    TSchedulingUnit mCurrentHungryScore;

private:
    CIDoubleLinkedList *mpClientList;
};

class CIPreemptiveScheduler: virtual public CObject, virtual public IScheduler
{
    typedef CObject inherited;

public:
    static IScheduler *Create(const TChar *name, TUint index);
    virtual CObject *GetObject0() const;

protected:
    CIPreemptiveScheduler(const TChar *name, TUint index);
    virtual ~CIPreemptiveScheduler();
    EECode Construct();

public:
    virtual void Delete();

public:
    virtual void PrintStatus();

public:
    virtual EECode StartScheduling();
    virtual EECode StopScheduling();

public:
    virtual EECode AddScheduledCilent(IScheduledClient *client);
    virtual EECode RemoveScheduledCilent(IScheduledClient *client);

public:
    virtual TUint TotalNumberOfScheduledClient() const;
    virtual TUint CurrentNumberOfClient() const;
    virtual TSchedulingUnit CurrentHungryScore() const;

public:
    virtual TInt IsPassiveMode() const;


protected:
    // return receive's reply
    EECode sendCmd(SCMD &cmd);
    void getCmd(SCMD &cmd);
    TU8 peekCmd(SCMD &cmd);

    EECode postMsg(SCMD &cmd);
    void cmdAck(EECode result);

private:
    void processCmd(SCMD &cmd);
    void mainLoop();
    static EECode threadEntry(void *p);

    EECode insertClient(IScheduledClient *client);
    EECode removeClient(IScheduledClient *client);

private:
    CIQueue *mpMsgQ;
    IThread *mpThread;

private:
    TU8 mbStarted;
    TU8 mbThreadRun;
    TU8 mbRun;
    TU8 mbRunning;

    EModuleState msState;

    //TUint mnTotalClientNumber;

    TUint mnCurrentActiveClientNumber;
    //TSchedulingUnit mCurrentHungryScore;

private:
    CIDoubleLinkedList *mpClientList;

private:
    TU32 mDebugHeartBeat00;

private:
    TSocketHandler mPipeFd[2];
    TSocketHandler mMaxFd;

    fd_set mAllSet;
    fd_set mReadSet;
    //fd_set mErrorSet;
};

class CIPriorityPreemptiveScheduler: public CObject, virtual public IScheduler
{
    typedef CObject inherited;

public:
    static IScheduler *Create(const TChar *name, TUint index);
    virtual CObject *GetObject0() const;

protected:
    CIPriorityPreemptiveScheduler(const TChar *name, TUint index);
    virtual ~CIPriorityPreemptiveScheduler();
    EECode Construct();

public:
    virtual void Delete();

public:
    virtual void PrintStatus();

public:
    virtual EECode StartScheduling();
    virtual EECode StopScheduling();

public:
    virtual EECode AddScheduledCilent(IScheduledClient *client);
    virtual EECode RemoveScheduledCilent(IScheduledClient *client);

public:
    virtual TUint TotalNumberOfScheduledClient() const;
    virtual TUint CurrentNumberOfClient() const;
    virtual TSchedulingUnit CurrentHungryScore() const;

public:
    virtual TInt IsPassiveMode() const;


protected:
    // return receive's reply
    EECode sendCmd(SCMD &cmd);
    void getCmd(SCMD &cmd);
    TU8 peekCmd(SCMD &cmd);

    EECode postMsg(SCMD &cmd);
    //EECode run();
    //EECode stop();
    //EECode start();

    void cmdAck(EECode result);
    //CIQueue *msgQ() const;

private:
    void processCmd(SCMD &cmd);
    void mainLoop();
    TUint loopSlice(TUint index);
    static EECode threadEntry(void *p);

    EECode insertClient(IScheduledClient *client);
    EECode removeClient(IScheduledClient *client);

private:
    CIQueue *mpMsgQ;
    IThread *mpThread;

private:
    TU8 mbStarted;
    TU8 mbThreadRun;
    TU8 mbRun;
    TU8 mbRunning;

    EModuleState msState;

    TUint mnTotalClientNumber;

    TUint mnCurrentActiveClientNumber;
    //TSchedulingUnit mCurrentHungryScore;

private:
    TSocketHandler mPipeFd[2];

private:
    CIDoubleLinkedList *mpClientLists[4];
    TInt mMaxFds[4];
    fd_set mAllSets[4];
    fd_set mReadSets[4];
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

