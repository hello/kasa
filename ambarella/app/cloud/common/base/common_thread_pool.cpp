/*******************************************************************************
 * common_thread_pool.cpp
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

//#ifdef BUILD_WITH_UNDER_DEVELOP_COMPONENT

#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "common_thread_pool.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

CISimpleWorkThread *CISimpleWorkThread::Create()
{
    CISimpleWorkThread *result = new CISimpleWorkThread();
    if (DUnlikely(result && result->Construct() != EECode_OK)) {
        LOG_ERROR("CISimpleWorkThread->Construct() fail\n");
        delete result;
        result = NULL;
    }
    return result;
}

void CISimpleWorkThread::Delete()
{
}

CISimpleWorkThread::CISimpleWorkThread()
    : msState(0)
    , mbRunning(1)
    , mpWorkloadQueue(NULL)
    //, mpOutputQueue(NULL)
    , mpOutputClosedQueue(NULL)
    , mpOutputErrorQueue(NULL)
{
}

CISimpleWorkThread::~CISimpleWorkThread()
{
}

EECode CISimpleWorkThread::Construct()
{
    return EECode_OK;
}

EECode CISimpleWorkThread::Start(ISimpleQueue *workload_queue, ISimpleQueue *output_queue, ISimpleQueue *output_closed_queue, ISimpleQueue *output_error_queue)
{
    mpWorkloadQueue = workload_queue;
    //mpOutputQueue = output_queue;
    mpOutputClosedQueue = output_closed_queue;
    mpOutputErrorQueue = output_error_queue;

    if (DLikely(0 == msState)) {
        msState = 1;
        mpThread = gfCreateThread("CISimpleWorkThread", ThreadEntry, (void *)this);
    } else {
        LOG_FATAL("bad state %d\n", msState);
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CISimpleWorkThread::Stop(TUint abort)
{
    if (DLikely(msState)) {
        if (abort) {
            mbRunning = 0;
            LOG_NOTICE("stop immediately\n");
        } else {
            mpWorkloadQueue->Enqueue(0);
            LOG_NOTICE("stop after task finished\n");
        }
        mpThread->Delete();
        mpThread = NULL;
        msState = 0;
        LOG_NOTICE("stop done\n");
        return EECode_OK;
    } else {
        LOG_WARN("already stopped\n");
    }

    return EECode_OK;
}

EECode CISimpleWorkThread::ThreadEntry(void *p)
{
    CISimpleWorkThread *thiz = (CISimpleWorkThread *)p;
    if (DLikely(p)) {
        thiz->MainLoop();
    } else {
        LOG_FATAL("NULL p\n");
        return EECode_BadParam;
    }

    return EECode_OK;
}

void CISimpleWorkThread::MainLoop()
{
    SSimpleWorkload *p_workload = NULL;
    ISimpleQueue *input_queue = mpWorkloadQueue;
    //ISimpleQueue* output_queue = mpOutputQueue;
    ISimpleQueue *output_closed_queue = mpOutputClosedQueue;
    ISimpleQueue *output_error_queue = mpOutputErrorQueue;

    if (DUnlikely(!input_queue /*|| !output_queue */)) {
        LOG_FATAL("NULL input_queue %p\n", input_queue);
        return;
    }

    if (DUnlikely(!output_closed_queue || !output_error_queue)) {
        LOG_FATAL("NULL output_closed_queue %p or output_error_queue %p\n", output_closed_queue, output_error_queue);
        return;
    }

    EECode err = EECode_OK;

    while (mbRunning) {
        p_workload = (SSimpleWorkload *)input_queue->Dequeue();
        if (DLikely(p_workload)) {
            if (DLikely(p_workload->client)) {
                err = p_workload->client->Scheduling();
                if (DLikely((EECode_OK == err) || (EECode_NotRunning == err))) {
                    //output_queue->enqueue(output_queue, (TULong)p_workload);
                } else if (DLikely(EECode_Closed == err)) {
                    //LOG_NOTICE("EECode_Closed\n");
                    p_workload->client->EventHandling(EECode_Closed);
                    output_closed_queue->Enqueue((TULong)p_workload);
                } else {
                    LOG_ERROR("return %d, %s\n", err, gfGetErrorCodeString(err));
                    p_workload->client->EventHandling(EECode_Closed);
                    output_error_queue->Enqueue((TULong)p_workload);
                }

            } else {
                LOG_NOTICE("thread quit\n");
                break;
            }
        } else {
            LOG_NOTICE("thread quit\n");
            break;
        }
    }

}

IThreadPool *CThreadPool::Create(const TChar *name, TUint index, TU32 max_thread_number)
{
    CThreadPool *result = new CThreadPool(name, index, max_thread_number);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CThreadPool::CThreadPool(const TChar *name, TUint index, TU32 max_thread_number)
    : inherited(name, index)
    , mCurrentThreadNumber(0)
    , mRunRobinCurrentThreadIndex(0)
    , mRunRobinMinThreadIndex(0)
    , mRunRobinMaxThreadIndex(0)
    , mRunRobinTotoalThreadNumber(5)
    , mMaxWorkloadCount(1024)
    //, mpOutputQueue(NULL)
    , mpOutputClosedQueue(NULL)
    , mpOutputErrorQueue(NULL)
    , mpMutex(NULL)
    , mbStarted(0)
    , mbSchedulerStarted(0)
    , mbRunning(1)
    , mnCurrentActiveClientNumber(0)
    , mpClientList(NULL)
    , mpTobeAddedClientList(NULL)
    , mEpollFd(-1)
    , mpFreeWorkloadList(NULL)
    , mDebugHeartBeat00(0)
{
    TU32 i = 0;
    for (i = 0; i < DMAX_THREAD_NUMBER_IN_THREAD_POOL; i++) {
        mpWorkingThread[i] = NULL;
        mpWorkloadQueue[i] = NULL;
    }

    if (max_thread_number < DMAX_THREAD_NUMBER_IN_THREAD_POOL) {
        mMaxThreadNumber = max_thread_number;
    } else {
        mMaxThreadNumber = DMAX_THREAD_NUMBER_IN_THREAD_POOL;
    }

}

CThreadPool::~CThreadPool()
{
    TUint i = 0;
    LOGM_RESOURCE("~CThreadPool start\n");

    if (mbStarted) {
        stopAllThreads(1);
    }

    for (i = 0; i < DMAX_THREAD_NUMBER_IN_THREAD_POOL; i ++) {
        if (mpWorkingThread[i]) {
            mpWorkingThread[i]->Delete();
            mpWorkingThread[i] = NULL;
        }
    }

    for (i = 0; i < DMAX_THREAD_NUMBER_IN_THREAD_POOL; i ++) {
        if (mpWorkloadQueue[i]) {
            mpWorkloadQueue[i]->Destroy();
            mpWorkloadQueue[i] = NULL;
        }
    }

    if (mpClientList) {
        delete mpClientList;
        mpClientList = NULL;
    }

    if (mpTobeAddedClientList) {
        delete mpTobeAddedClientList;
        mpTobeAddedClientList = NULL;
    }

    SSimpleWorkload *p_removed = mpFreeWorkloadList;
    while (p_removed) {
        mpFreeWorkloadList = p_removed->p_next;
        DDBG_FREE(p_removed, "TPRL");
        p_removed = mpFreeWorkloadList;
    }

    if (0 <= mEpollFd) {
        close(mEpollFd);
        mEpollFd = -1;
    }

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    LOGM_RESOURCE("~CThreadPool end\n");
}

EECode CThreadPool::Construct()
{
    TU32 i = 0;

    DSET_MODULE_LOG_CONFIG(ELogModuleThreadPool);

    mpMutex = gfCreateMutex();
    if (DUnlikely(mpMutex == NULL)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TInt ret = pipe(mPipeFd);
    if (DUnlikely(0 > ret)) {
        LOG_FATAL("pipe fail\n");
        return EECode_OSError;
    }

    mpOutputClosedQueue = gfCreateSimpleQueue(mMaxWorkloadCount);
    if (DUnlikely(!mpOutputClosedQueue)) {
        LOGM_FATAL("gfCreateSimpleQueue fail\n");
        return EECode_NoMemory;
    }

    mpOutputErrorQueue = gfCreateSimpleQueue(mMaxWorkloadCount);
    if (DUnlikely(!mpOutputErrorQueue)) {
        LOGM_FATAL("gfCreateSimpleQueue fail\n");
        return EECode_NoMemory;
    }

    if (DLikely(mMaxThreadNumber)) {
        for (i = 0; i < mMaxThreadNumber; i++) {
            mpWorkloadQueue[i] = gfCreateSimpleQueue(mMaxWorkloadCount);
            if (DUnlikely(!mpWorkloadQueue[i])) {
                LOGM_FATAL("gfCreateSimpleQueue fail\n");
                return EECode_NoMemory;
            }
        }

        for (i = 0; i < mMaxThreadNumber; i++) {
            mpWorkingThread[i] = CISimpleWorkThread::Create();
            if (DUnlikely(!mpWorkingThread[i])) {
                LOGM_FATAL("CISimpleWorkThread::Create() fail\n");
                return EECode_NoMemory;
            }
        }
    }

    mpClientList = new CIDoubleLinkedList();
    if (DUnlikely(!mpClientList)) {
        LOG_FATAL("No memory\n");
        return EECode_NoMemory;
    }

    mpTobeAddedClientList = new CIDoubleLinkedList();
    if (DUnlikely(!mpTobeAddedClientList)) {
        LOG_FATAL("No memory\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

void CThreadPool::PrintStatus()
{
    LOGM_PRINTF("heart beat %d, %d\n", mDebugHeartBeat, mDebugHeartBeat00);
    mDebugHeartBeat = 0;
}

void CThreadPool::Delete()
{
    TUint i = 0;

    if (mbStarted) {
        stopAllThreads(1);
    }

    stopSchedular();

    for (i = 0; i < DMAX_THREAD_NUMBER_IN_THREAD_POOL; i ++) {
        if (mpWorkingThread[i]) {
            mpWorkingThread[i]->Delete();
            mpWorkingThread[i] = NULL;
        }
    }

    for (i = 0; i < DMAX_THREAD_NUMBER_IN_THREAD_POOL; i ++) {
        if (mpWorkloadQueue[i]) {
            mpWorkloadQueue[i]->Destroy();
            mpWorkloadQueue[i] = NULL;
        }
    }

    if (mpClientList) {
        delete mpClientList;
        mpClientList = NULL;
    }

    if (mpTobeAddedClientList) {
        delete mpTobeAddedClientList;
        mpTobeAddedClientList = NULL;
    }

    SSimpleWorkload *p_removed = mpFreeWorkloadList;
    while (p_removed) {
        mpFreeWorkloadList = p_removed->p_next;
        DDBG_FREE(p_removed, "TPRL");
        p_removed = mpFreeWorkloadList;
    }

    if (0 <= mEpollFd) {
        close(mEpollFd);
        mEpollFd = -1;
    }
}

EECode CThreadPool::StartRunning(TU32 init_thread_number)
{
    TU32 i = 0;

    mCurrentThreadNumber = init_thread_number;
    if (DUnlikely(mCurrentThreadNumber > mMaxThreadNumber)) {
        mCurrentThreadNumber = mMaxThreadNumber;
        LOGM_WARN("init_thread_number %d exceed max value %d\n", init_thread_number, mMaxThreadNumber);
    }

    if (mCurrentThreadNumber > mRunRobinTotoalThreadNumber) {
        mRunRobinMaxThreadIndex = mCurrentThreadNumber - mRunRobinTotoalThreadNumber;
    } else {
        mRunRobinMaxThreadIndex = 0;
        mRunRobinTotoalThreadNumber = mCurrentThreadNumber;
    }

    for (i = 0; i < mCurrentThreadNumber; i++) {
        if (DLikely(mpWorkingThread[i])) {
            mpWorkingThread[i]->Start(mpWorkloadQueue[i], NULL, mpOutputClosedQueue, mpOutputErrorQueue);
        } else {
            LOGM_ERROR("NULL mpWorkingThread[%d]\n", i);
            return EECode_Error;
        }
    }

    startSchedular();

    return EECode_OK;
}

void CThreadPool::StopRunning(TU32 abort)
{
    LOGM_NOTICE("CThreadPool::StopRunning start.\n");
    stopSchedular();

    stopAllThreads(0);
}

EECode CThreadPool::EnqueueCilent(IScheduledClient *client)
{
    TChar wake_char = 'a';

    if (DUnlikely(!client)) {
        LOGM_FATAL("NULL client!\n");
        return EECode_BadParam;
    }

    __LOCK(mpMutex);
    mpTobeAddedClientList->InsertContent(NULL, (void *) client, 0);
    __UNLOCK(mpMutex);

    gfOSWritePipe(mPipeFd[1], wake_char);

    return EECode_OK;
}

EECode CThreadPool::ResetThreadNumber(TU32 target_thread_number)
{
    LOGM_FATAL("To Do\n");
    return EECode_OK;
}

EECode CThreadPool::ThreadEntry(void *p)
{
    //LOG_NOTICE("CThreadPool::threadEntry, p %p begin\n", p);
    ((CThreadPool *)p)->MainLoopUseEpoll();
    //LOG_NOTICE("CThreadPool::threadEntry, p %p end\n", p);
    return EECode_OK;
}

void CThreadPool::MainLoopUseEpoll()
{
    CIDoubleLinkedList::SNode *p_node = NULL;
    IScheduledClient *p_client = NULL;
    TSchedulingHandle handle = 0;
    SSimpleWorkload *p_workload = NULL;
    ISimpleQueue *queue = NULL;
    TInt ret = 0;
    TInt event_num = 0, event_index = 0;
    TULong ret_tmp = 0;

    if (DUnlikely(!mMaxWorkloadCount)) {
        LOGM_FATAL("zero mMaxWorkloadCount\n");
        return;
    }

    epoll_event ev;
    epoll_event *events = (epoll_event *) DDBG_MALLOC(mMaxWorkloadCount * sizeof(epoll_event), "TPEV");
    if (DUnlikely(!events)) {
        LOGM_FATAL("events alloc fail, mMaxWorkloadCount %d, request size %ld\n", mMaxWorkloadCount, (TULong)(mMaxWorkloadCount * sizeof(epoll_event)));
        return;
    }

    mEpollFd = epoll_create((int)mMaxWorkloadCount);

    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = (void *) this;
    ret = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, (TInt)mPipeFd[0], &ev);
    if (DUnlikely(0 > ret)) {
        LOGM_ERROR("EPOLL_CTL_ADD error\n");
        return;
    }

    while (mbRunning) {

        mDebugHeartBeat00 = 0;

        __LOCK(mpMutex);
        p_node = mpTobeAddedClientList->FirstNode();
        while (p_node) {
            p_client = (IScheduledClient *)p_node->p_context;
            if (DLikely(p_client)) {
                p_workload = allocWorkload();
                if (DUnlikely(!p_workload)) {
                    LOGM_FATAL("allocWorkload error\n");
                    mbRunning = 0;
                    break;
                }

                p_workload->client = p_client;
                p_workload->p_node_in_main_list = mpClientList->InsertContent(NULL, (void *) p_workload, 0);
                handle = p_client->GetWaitHandle();
                ev.events = EPOLLIN | EPOLLET;
                ev.data.ptr = (void *) p_workload;
                ret = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, (TInt)handle, &ev);
                if (DUnlikely(0 > ret)) {
                    LOGM_ERROR("EPOLL_CTL_ADD error\n");
                    mbRunning = 0;
                    break;
                }

            } else {
                LOGM_FATAL("NULL p_client?\n");
            }
            p_node = mpTobeAddedClientList->NextNode(p_node);
        }
        mpTobeAddedClientList->RemoveAllNodes();
        __UNLOCK(mpMutex);

        while (mpOutputClosedQueue->TryDequeue(ret_tmp)) {
            p_workload = (SSimpleWorkload *) ret_tmp;
            if (DLikely(p_workload && p_workload->client)) {

                //not invoke epoll_ctl
#if 0
                p_client = p_workload->client;
                handle = p_client->GetWaitHandle();

                ev.events = EPOLLIN | EPOLLET;
                ev.data.ptr = (void *)p_workload;

                ret = epoll_ctl(mEpollFd, EPOLL_CTL_DEL, (TInt)handle, &ev);
                if (DUnlikely(0 > ret)) {
                    LOGM_ERROR("EPOLL_CTL_DEL error\n");
                    perror("EPOLL_CTL_DEL:");
                    mbRunning = 0;
                    break;
                }
#endif

                if (DLikely(p_workload->p_node_in_main_list)) {
                    DASSERT(p_workload->p_node_in_main_list->p_context == (void *) p_workload);
                    mpClientList->FastRemoveContent(p_workload->p_node_in_main_list, (void *) p_workload);
                    releaseWorkload(p_workload);
                    p_workload = NULL;
                } else {
                    LOGM_FATAL("NULL p_workload->p_node_in_main_list\n");
                    mbRunning = 0;
                    break;
                }

            } else {
                break;
            }

        }

        while (mpOutputErrorQueue->TryDequeue(ret_tmp)) {
            p_workload = (SSimpleWorkload *) ret_tmp;
            if (DLikely(p_workload && p_workload->client)) {

                //not invoke epoll_ctl
#if 0
                p_client = p_workload->client;
                handle = p_client->GetWaitHandle();

                ev.events = EPOLLIN;
                ev.data.ptr = (void *)p_workload;

                ret = epoll_ctl(mEpollFd, EPOLL_CTL_DEL, (TInt)handle, &ev);
                if (DUnlikely(0 > ret)) {
                    LOGM_ERROR("EPOLL_CTL_DEL error\n");
                    mbRunning = 0;
                    break;
                }
#endif
                if (DLikely(p_workload->p_node_in_main_list)) {
                    DASSERT(p_workload->p_node_in_main_list->p_context == (void *) p_workload);
                    mpClientList->FastRemoveContent(p_workload->p_node_in_main_list, (void *) p_workload);
                    releaseWorkload(p_workload);
                    p_workload = NULL;
                } else {
                    LOGM_FATAL("NULL p_workload->p_node_in_main_list\n");
                    mbRunning = 0;
                    break;
                }

                LOGM_WARN("client encounter error\n");
            } else {
                break;
            }

        }

redo:
        event_num = epoll_wait(mEpollFd, events, mMaxWorkloadCount, -1);
        if (DUnlikely(0 > event_num)) {
            if (errno == EINTR)
            { goto redo; }

            perror("epoll_wait");
            LOGM_ERROR("epoll_wait error\n");
            mbRunning = 0;
            break;
        }

        event_index = 0;
        while (event_index < event_num) {
            if (DUnlikely(((void *) this) == events[event_index].data.ptr)) {
                processCmd();
                event_index ++;
                continue;
            }
            p_workload = (SSimpleWorkload *) events[event_index].data.ptr;
            if (DLikely(p_workload && p_workload->client)) {
                queue = findMinLoadSlot();
                queue->Enqueue((TULong)p_workload);
            }
            event_index ++;
        }

        mDebugHeartBeat ++;
        mDebugHeartBeat00 = 9;
    }

    DDBG_FREE(events, "TPEV");
}

#if 0
void CThreadPool::MainLoopUseSelect()
{
    CIDoubleLinkedList::SNode *p_node = NULL;
    IScheduledClient *p_client = NULL;
    TSchedulingHandle handle = 0;
    SSimpleWorkload *p_workload = NULL;
    ISimpleQueue *queue = NULL;
    TInt ret = 0;
    TULong ret_tmp = 0;

    if (DUnlikely(!mMaxWorkloadCount)) {
        LOGM_FATAL("zero mMaxWorkloadCount\n");
        return;
    }

    TInt retval = 0;

    TSocketHandler max_fd = mPipeFd[0];
    fd_set all_set;
    fd_set read_set;
    fd_set error_set;

    FD_ZERO(&all_set);
    FD_ZERO(&read_set);
    FD_ZERO(&error_set);
    FD_SET(mPipeFd[0], &all_set);

    while (mbRunning) {

        mDebugHeartBeat00 = 0;

        __LOCK(mpMutex);
        p_node = mpTobeAddedClientList->FirstNode();
        while (p_node) {
            p_client = (IScheduledClient *)p_node->p_context;
            if (DLikely(p_client)) {
                p_workload = allocWorkload();
                if (DUnlikely(!p_workload)) {
                    LOGM_FATAL("allocWorkload error\n");
                    mbRunning = 0;
                    break;
                }

                p_workload->client = p_client;
                p_workload->p_node_in_main_list = mpClientList->InsertContent(NULL, (void *) p_workload, 0);
                handle = p_client->GetWaitHandle();
                if (handle > max_fd) {
                    max_fd = handle;
                }

                ret = FD_SET(handle, &all_set);
                if (DUnlikely(0 > ret)) {
                    LOGM_ERROR("FD_SET(%d) error\n", handle);
                    mbRunning = 0;
                    break;
                }

            } else {
                LOGM_FATAL("NULL p_client?\n");
            }
            p_node = mpTobeAddedClientList->NextNode(p_node);
        }
        mpTobeAddedClientList->RemoveAllNodes();
        __UNLOCK(mpMutex);

        while (mpOutputClosedQueue->TryDequeue(ret_tmp)) {
            p_workload = (SSimpleWorkload *) ret_tmp;
            if (DLikely(p_workload && p_workload->client)) {
                if (DLikely(p_workload->p_node_in_main_list)) {
                    ret = FD_CLR(p_workload->client->GetWaitHandle(), &all_set);
                    DASSERT(p_workload->p_node_in_main_list->p_context == (void *) p_workload);
                    mpClientList->FastRemoveContent(p_workload->p_node_in_main_list, (void *) p_workload);
                    releaseWorkload(p_workload);
                    p_workload = NULL;
                } else {
                    LOGM_FATAL("NULL p_workload->p_node_in_main_list\n");
                    mbRunning = 0;
                    break;
                }

            } else {
                break;
            }

        }

        while (mpOutputErrorQueue->TryDequeue(ret_tmp)) {
            p_workload = (SSimpleWorkload *) ret_tmp;
            if (DLikely(p_workload && p_workload->client)) {
                if (DLikely(p_workload->p_node_in_main_list)) {
                    ret = FD_CLR(p_workload->client->GetWaitHandle(), &all_set);
                    DASSERT(p_workload->p_node_in_main_list->p_context == (void *) p_workload);
                    mpClientList->FastRemoveContent(p_workload->p_node_in_main_list, (void *) p_workload);
                    releaseWorkload(p_workload);
                    p_workload = NULL;
                } else {
                    LOGM_FATAL("NULL p_workload->p_node_in_main_list\n");
                    mbRunning = 0;
                    break;
                }

                LOGM_WARN("client encounter error\n");
            } else {
                break;
            }

        }

redo:
        read_set = all_set;
        retval = select(max_fd + 1, &read_set, NULL, NULL, NULL);
        if (DUnlikely(0 > retval)) {
            if (errno == EINTR) {
                goto redo;
            }

            error_set = all_set;
            retval = select(max_fd + 1, NULL, NULL, &error_set, NULL);

            perror("epoll_wait");
            LOGM_ERROR("epoll_wait error\n");
            mbRunning = 0;
            break;
        }

        event_index = 0;
        while (event_index < event_num) {
            if (DUnlikely(((void *) this) == events[event_index].data.ptr)) {
                processCmd();
                event_index ++;
                continue;
            }
            p_workload = (SSimpleWorkload *) events[event_index].data.ptr;
            if (DLikely(p_workload && p_workload->client)) {
                queue = findMinLoadSlot();
                queue->Enqueue((TULong)p_workload);
            }
            event_index ++;
        }

        mDebugHeartBeat ++;
        mDebugHeartBeat00 = 9;
    }

    DDBG_FREE(events, "TPEV");
}
#endif

void CThreadPool::startSchedular()
{
    if (mbSchedulerStarted) {
        LOGM_WARN("already started\n");
        return;
    }

    mbRunning = 1;

    mpThread = gfCreateThread("CThreadPool", ThreadEntry, (void *)this);
    mbSchedulerStarted = 1;
}

void CThreadPool::stopSchedular()
{
    if (!mbSchedulerStarted) {
        LOGM_WARN("already stopped\n");
        return;
    }

    mbRunning = 0;

    TChar wake_char = 's';
    gfOSWritePipe(mPipeFd[1], wake_char);

    if (mpThread) {
        mpThread->Delete();
        mpThread = NULL;
    }
    mbSchedulerStarted = 1;
}

void CThreadPool::stopAllThreads(TUint abort)
{
    TU32 i = 0;

    LOGM_NOTICE("stopAllThreads(%d) begin\n", abort);

    if (mbStarted) {
        mbStarted = 0;

        for (i = 0; i < DMAX_THREAD_NUMBER_IN_THREAD_POOL; i ++) {
            if (mpWorkingThread[i]) {
                LOGM_NOTICE("mpWorkingThread[%d]->Stop(abort) begin\n", i);
                mpWorkingThread[i]->Stop(abort);
                LOGM_NOTICE("mpWorkingThread[%d]->Stop(abort) end\n", i);
            }
        }
    }

    LOGM_NOTICE("stopAllThreads(%d) end\n", abort);
}

SSimpleWorkload *CThreadPool::allocWorkload()
{
    SSimpleWorkload *ptmp = mpFreeWorkloadList;

    if (ptmp) {
        mpFreeWorkloadList = ptmp->p_next;
        ptmp->p_next = NULL;
    } else {
        ptmp = (SSimpleWorkload *) DDBG_MALLOC(sizeof(SSimpleWorkload), "TPWL");
        if (DLikely(ptmp)) {
            ptmp->client = NULL;
            ptmp->p_node_in_main_list = NULL;
            ptmp->p_next = NULL;
        } else {
            LOGM_FATAL("no memory\n");
        }
    }

    return ptmp;
}

void CThreadPool::releaseWorkload(SSimpleWorkload *p)
{
    if (DLikely(p)) {
        if (p->client) {
            //p->client->Destroy();
            p->client = NULL;
        }
        p->p_node_in_main_list = NULL;
        p->p_next = mpFreeWorkloadList;
        mpFreeWorkloadList = p;
    } else {
        LOGM_FATAL("no memory\n");
    }
}

ISimpleQueue *CThreadPool::findMinLoadSlot()
{
    TU32 cur_load = 0, min_load = 0;
    TU16 i = 0, min_index = mRunRobinCurrentThreadIndex;
    DASSERT((mRunRobinCurrentThreadIndex + mRunRobinTotoalThreadNumber) <= mCurrentThreadNumber);

    min_load = mpWorkloadQueue[mRunRobinCurrentThreadIndex]->GetCnt();
    for (i = mRunRobinCurrentThreadIndex + 1; i < (mRunRobinCurrentThreadIndex + mRunRobinTotoalThreadNumber); i ++) {
        cur_load = mpWorkloadQueue[i]->GetCnt();
        if (cur_load < min_load) {
            min_load = cur_load;
            min_index = i;
        }
    }

    if ((mRunRobinCurrentThreadIndex + 1) < (mRunRobinMaxThreadIndex)) {
        mRunRobinCurrentThreadIndex ++;
    } else {
        mRunRobinCurrentThreadIndex = 0;
    }

    return mpWorkloadQueue[min_index];
}

void CThreadPool::processCmd()
{
    TChar char_buffer;
    gfOSReadPipe(mPipeFd[0], char_buffer);
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

//#endif

