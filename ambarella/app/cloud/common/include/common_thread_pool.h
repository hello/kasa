/*******************************************************************************
 * common_thread_pool.h
 *
 * History:
 *  2014/05/28 - [Zhi He] create file
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

#ifndef __COMMON_THREAD_POOL_H__
#define __COMMON_THREAD_POOL_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

#define DMAX_THREAD_NUMBER_IN_THREAD_POOL 16

typedef struct _SSimpleWorkload {
    IScheduledClient *client;

    CIDoubleLinkedList::SNode *p_node_in_main_list;

    struct _SSimpleWorkload *p_next;
} SSimpleWorkload;

class CISimpleWorkThread
{
public:
    static CISimpleWorkThread *Create();
    void Delete();

protected:
    CISimpleWorkThread();
    ~CISimpleWorkThread();
    EECode Construct();

public:
    EECode Start(ISimpleQueue *workload_queue, ISimpleQueue *output_queue, ISimpleQueue *output_closed_queue, ISimpleQueue *output_error_queue);
    EECode Stop(TUint abort = 0);

public:
    static EECode ThreadEntry(void *p);
    void MainLoop();

private:
    TU8 msState;
    TU8 mbRunning;
    TU8 reserved1;
    TU8 reserved2;

private:
    IThread *mpThread;
    ISimpleQueue *mpWorkloadQueue;

    //ISimpleQueue* mpOutputQueue;
    ISimpleQueue *mpOutputClosedQueue;
    ISimpleQueue *mpOutputErrorQueue;
} ;

class CThreadPool: public CObject, public IThreadPool
{
    typedef CObject inherited;

public:
    static IThreadPool *Create(const TChar *name, TUint index, TU32 max_thread_number);

protected:
    CThreadPool(const TChar *name, TUint index, TU32 max_thread_number);
    virtual ~CThreadPool();
    EECode Construct();

public:
    virtual void Delete();

public:
    virtual void PrintStatus();

public:
    virtual EECode StartRunning(TU32 init_thread_number);
    virtual void StopRunning(TU32 abort = 0);

public:
    virtual EECode EnqueueCilent(IScheduledClient *client);

public:
    virtual EECode ResetThreadNumber(TU32 target_thread_number);

public:
    static EECode ThreadEntry(void *p);
    void MainLoopUseEpoll();

private:
    void startSchedular();
    void stopSchedular();
    void stopAllThreads(TUint abort);

    SSimpleWorkload *allocWorkload();
    void releaseWorkload(SSimpleWorkload *p);
    ISimpleQueue *findMinLoadSlot();
    void processCmd();

    //thread group
private:
    TU16 mMaxThreadNumber;
    TU16 mCurrentThreadNumber;
    TU16 mRunRobinCurrentThreadIndex;
    TU16 mRunRobinMinThreadIndex;
    TU16 mRunRobinMaxThreadIndex;
    TU16 mRunRobinTotoalThreadNumber;

    TU32 mMaxWorkloadCount;

    CISimpleWorkThread *mpWorkingThread[DMAX_THREAD_NUMBER_IN_THREAD_POOL];

    ISimpleQueue *mpWorkloadQueue[DMAX_THREAD_NUMBER_IN_THREAD_POOL];

    //ISimpleQueue* mpOutputQueue;
    ISimpleQueue *mpOutputClosedQueue;
    ISimpleQueue *mpOutputErrorQueue;

private:
    IMutex *mpMutex;

private:
    TU8 mbStarted;
    TU8 mbSchedulerStarted;
    TU8 mbRunning;
    TU8 mReserved0;

    TU32 mnCurrentActiveClientNumber;
    IThread *mpThread;

private:
    CIDoubleLinkedList *mpClientList;

private:
    CIDoubleLinkedList *mpTobeAddedClientList;

    //epoll
private:
    TInt mEpollFd;

    //control pipe
    TInt mPipeFd[2];

private:
    SSimpleWorkload *mpFreeWorkloadList;

private:
    TU32 mDebugHeartBeat00;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif



