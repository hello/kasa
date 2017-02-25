/*******************************************************************************
 * common_schedular.cpp
 *
 * History:
 *  2013/07/12 - [Zhi He] create file
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

#include "common_config.h"
#ifndef BUILD_OS_WINDOWS
#include <unistd.h>
#else
#include <direct.h>
#include <io.h>
#endif

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "common_schedular.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IScheduler *gfSchedulerFactory(ESchedulerType type, TUint index)
{
    IScheduler *p = NULL;

    switch (type) {

        case ESchedulerType_RunRobin:
            p = CIRunRobinScheduler::Create("CIRunRobinScheduler", index);
            break;

        case ESchedulerType_Preemptive:
            p = CIPreemptiveScheduler::Create("CIPreemptiveScheduler", index);
            break;

        case ESchedulerType_PriorityPreemptive:
            p = CIPriorityPreemptiveScheduler::Create("CIPriorityPreemptiveScheduler", index);
            break;

        default:
            LOG_FATAL("BAD ESchedulerType %d\n", type);
            break;
    }

    return p;
}

IScheduler *CIRunRobinScheduler::Create(const TChar *name, TUint index)
{
    CIRunRobinScheduler *result = new CIRunRobinScheduler(name, index);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CObject *CIRunRobinScheduler::GetObject0() const
{
    return (CObject *) this;
}

CIRunRobinScheduler::CIRunRobinScheduler(const TChar *name, TUint index)
    : inherited(name, index)
    , mpMsgQ(NULL)
    , mpThread(NULL)
    , mbStarted(0)
    , mbThreadRun(0)
    , mbRun(1)
    , mbRunning(0)
    , msState(EModuleState_Invalid)
    , mnCurrentActiveClientNumber(0)
    , mCurrentHungryScore(0)
    , mpClientList(NULL)
{
    ;
}

CIRunRobinScheduler::~CIRunRobinScheduler()
{
    LOGM_RESOURCE("~CIRunRobinScheduler start\n");

    if (mpThread) {
        if (mbRunning) {
            SCMD cmd;
            cmd.code = ECMDType_ExitRunning;
            cmd.pExtra = NULL;
            cmd.needFreePExtra = 0;
            EECode err = sendCmd(cmd);
            DASSERT_OK(err);
            mbRunning = 0;
        }
        mpThread->Delete();
        mpThread = NULL;
    }

    if (mpMsgQ) {
        mpMsgQ->Delete();
        mpMsgQ = NULL;
    }

    if (mpClientList) {
        delete mpClientList;
        mpClientList = NULL;
    }

    LOGM_RESOURCE("~CIRunRobinScheduler end\n");
}

EECode CIRunRobinScheduler::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleRunRobinScheduler);

    if ((mpMsgQ = CIQueue::Create(NULL, this, sizeof(SCMD), 16)) == NULL) {
        LOG_FATAL("No memory\n");
        return EECode_NoMemory;
    }

    if ((mpThread = gfCreateThread("CIRunRobinScheduler", threadEntry, this)) == NULL) {
        LOG_FATAL("No memory\n");
        return EECode_NoMemory;
    }

    mbRunning = 1;

    mpClientList = new CIDoubleLinkedList();

    if (!mpClientList) {
        LOG_FATAL("No memory\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

void CIRunRobinScheduler::PrintStatus()
{
    LOGM_PRINTF("heart beat %d, msState %d\n", mDebugHeartBeat, msState);
    mDebugHeartBeat = 0;
}

void CIRunRobinScheduler::Delete()
{
    LOGM_RESOURCE("CIRunRobinScheduler::Delete() start\n");

    if (mpThread) {
        if (mbRunning) {
            SCMD cmd;
            cmd.code = ECMDType_ExitRunning;
            cmd.pExtra = NULL;
            cmd.needFreePExtra = 0;
            EECode err = sendCmd(cmd);
            DASSERT_OK(err);
            mbRunning = 0;
        }
        mpThread->Delete();
        mpThread = NULL;
    }

    if (mpMsgQ) {
        mpMsgQ->Delete();
        mpMsgQ = NULL;
    }

    if (mpClientList) {
        delete mpClientList;
        mpClientList = NULL;
    }

    LOGM_RESOURCE("CIRunRobinScheduler::Delete() end\n");
}

EECode CIRunRobinScheduler::StartScheduling()
{
    //LOGM_DEBUG("[DEBUG]: CIRunRobinScheduler::StartScheduling() start\n");
    SCMD cmd;
    cmd.code = ECMDType_Start;
    cmd.pExtra = NULL;

    return sendCmd(cmd);
}

EECode CIRunRobinScheduler::StopScheduling()
{
    //LOGM_DEBUG("[DEBUG]: CIRunRobinScheduler::StopScheduling() start\n");
    SCMD cmd;
    cmd.code = ECMDType_Stop;
    cmd.pExtra = NULL;

    return sendCmd(cmd);
}

EECode CIRunRobinScheduler::AddScheduledCilent(IScheduledClient *client)
{
    SCMD cmd;

    if (!client) {
        LOGM_FATAL("NULL client!\n");
        return EECode_BadParam;
    }

    cmd.code = ECMDType_AddContent;
    cmd.pExtra = client;

    return sendCmd(cmd);
}

EECode CIRunRobinScheduler::RemoveScheduledCilent(IScheduledClient *client)
{
    SCMD cmd;

    if (!client) {
        LOGM_FATAL("NULL client!\n");
        return EECode_BadParam;
    }

    cmd.code = ECMDType_RemoveContent;
    cmd.pExtra = client;

    return sendCmd(cmd);
}

TUint CIRunRobinScheduler::TotalNumberOfScheduledClient() const
{
    DASSERT(mpClientList);
    if (mpClientList) {
        return mpClientList->NumberOfNodes();
    }

    return 0;
}

TUint CIRunRobinScheduler::CurrentNumberOfClient() const
{
    return mnCurrentActiveClientNumber;
}

TSchedulingUnit CIRunRobinScheduler::CurrentHungryScore() const
{
    return mCurrentHungryScore;
}

TInt CIRunRobinScheduler::IsPassiveMode() const
{
    return 1;
}

void CIRunRobinScheduler::processCmd(SCMD &cmd)
{
    LOGM_INFO("processCmd, cmd.code %d, %s, state %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code), msState, gfGetModuleStateString(msState));

    switch (cmd.code) {

        case ECMDType_StartRunning:
            DASSERT(EModuleState_WaitCmd == msState);
            if (mpClientList->NumberOfNodes()) {
                msState = EModuleState_Running;
            } else {
                msState = EModuleState_Idle;
            }
            cmdAck(EECode_OK);
            LOGM_INFO("processCmd, ECMDType_StartRunning.\n");
            break;

        case ECMDType_ExitRunning:
            mbRun = 0;
            cmdAck(EECode_OK);
            LOGM_INFO("processCmd, ECMDType_ExitRunning.\n");
            break;

        case ECMDType_Stop:
            msState = EModuleState_Pending;
            mbStarted = 0;
            cmdAck(EECode_OK);
            LOGM_INFO("processCmd, ECMDType_Stop.\n");
            break;

        case ECMDType_Start:
            DASSERT((EModuleState_WaitCmd == msState) || (EModuleState_Pending == msState));
            if (mpClientList->NumberOfNodes()) {
                msState = EModuleState_Running;
            } else {
                msState = EModuleState_Idle;
            }
            mbStarted = 1;
            cmdAck(EECode_OK);
            LOGM_INFO("processCmd, ECMDType_Start.\n");
            break;

        case ECMDType_AddContent:
            DASSERT(mpClientList);
            mpClientList->InsertContent(NULL, cmd.pExtra, 0);
            cmdAck(EECode_OK);
            LOGM_INFO("processCmd, ECMDType_AddContent.\n");
            break;

        case ECMDType_RemoveContent:
            DASSERT(mpClientList);
            mpClientList->RemoveContent(cmd.pExtra);
            cmdAck(EECode_OK);
            LOGM_INFO("processCmd, ECMDType_AddContent.\n");
            break;

        default:
            LOGM_ERROR("processCmd, wrong cmd %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code));
            break;
    }
}

EECode CIRunRobinScheduler::sendCmd(SCMD &cmd)
{
    return mpMsgQ->SendMsg(&cmd, sizeof(cmd));
}

void CIRunRobinScheduler::getCmd(SCMD &cmd)
{
    mpMsgQ->GetMsg(&cmd, sizeof(cmd));
}

TU8 CIRunRobinScheduler::peekCmd(SCMD &cmd)
{
    if (false == mpMsgQ->PeekMsg(&cmd, sizeof(cmd))) {
        return 0;
    }

    return 1;
}

EECode CIRunRobinScheduler::postMsg(SCMD &cmd)
{
    return mpMsgQ->PostMsg(&cmd, sizeof(cmd));
}

void CIRunRobinScheduler::cmdAck(EECode result)
{
    mpMsgQ->Reply(result);
}

EECode CIRunRobinScheduler::threadEntry(void *p)
{
    LOG_NOTICE("CIRunRobinScheduler::threadEntry, p %p begin\n", p);
    ((CIRunRobinScheduler *)p)->mainLoop();
    LOG_NOTICE("CIRunRobinScheduler::threadEntry, p %p end\n", p);
    return EECode_OK;
}

void CIRunRobinScheduler::mainLoop()
{
    SCMD cmd;

    TUint currentActiveClientNumber = 0;
    TSchedulingUnit currentHungryScore = 0;
    TSchedulingUnit score = 0;
    CIDoubleLinkedList::SNode *pnode = NULL;
    IScheduledClient *p_client = NULL;

    //EECode err = EECode_OK;

    LOGM_STATE("mainLoop, start\n");

    mbRun = 1;
    msState = EModuleState_WaitCmd;

    while (mbRun) {

        LOGM_STATE("OnRun: start switch, msState=%d, %s\n", msState, gfGetModuleStateString(msState));

        switch (msState) {

            case EModuleState_WaitCmd:
                getCmd(cmd);
                processCmd(cmd);
                break;

            case EModuleState_Error:
            case EModuleState_Pending:
                getCmd(cmd);
                processCmd(cmd);
                break;

            case EModuleState_Idle:
                getCmd(cmd);
                processCmd(cmd);

                if (mpClientList->NumberOfNodes()) {
                    msState = EModuleState_Running;
                }
                break;

            case EModuleState_Running:
                while (peekCmd(cmd)) {
                    processCmd(cmd);
                }

                if ((EModuleState_Running != msState) || (!mbRun)) {
                    break;
                }

                if (DUnlikely(!mpClientList->NumberOfNodes())) {
                    msState = EModuleState_Idle;
                    break;
                }

                //LOGM_VERBOSE("CIRunRobinScheduler::mainLoop: start client's work loop, total count %d.\n", mpClientList->NumberOfNodes());

                currentActiveClientNumber = 0;
                currentHungryScore = 0;

                pnode = mpClientList->FirstNode();
                while (pnode) {
                    p_client = (IScheduledClient *)(pnode->p_context);
                    pnode = mpClientList->NextNode(pnode);
                    DASSERT(p_client);
                    if (p_client) {
                        score = p_client->HungryScore();
                        if (score) {
                            currentHungryScore += score;
                            EECode err = p_client->Scheduling();
                            if (EECode_OK == err) {
                                currentActiveClientNumber ++;
                            } else {
                                //
                            }
                        }
                    } else {
                        LOGM_FATAL("NULL pointer here, something would be wrong.\n");
                    }
                }
                mnCurrentActiveClientNumber = currentActiveClientNumber;
                mCurrentHungryScore = currentHungryScore;

                if (!mnCurrentActiveClientNumber) {
                    DASSERT(!mCurrentHungryScore);
                    //LOGM_VERBOSE("idle loop\n");
                    gfOSusleep(100000);
                }
                break;

            default:
                LOGM_ERROR("OnRun: state invalid: %d\n", (TUint)msState);
                msState = EModuleState_Error;
                break;
        }

        mDebugHeartBeat ++;
    }
}

IScheduler *CIPreemptiveScheduler::Create(const TChar *name, TUint index)
{
    CIPreemptiveScheduler *result = new CIPreemptiveScheduler(name, index);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CObject *CIPreemptiveScheduler::GetObject0() const
{
    return (CObject *) this;
}

CIPreemptiveScheduler::CIPreemptiveScheduler(const TChar *name, TUint index)
    : inherited(name, index)
    , mpMsgQ(NULL)
    , mpThread(NULL)
    , mbStarted(0)
    , mbThreadRun(0)
    , mbRun(1)
    , mbRunning(0)
    , msState(EModuleState_Invalid)
    , mnCurrentActiveClientNumber(0)
    , mpClientList(NULL)
    , mDebugHeartBeat00(0)
{
    mPipeFd[0] = DInvalidSocketHandler;
    mPipeFd[1] = DInvalidSocketHandler;
    mMaxFd = DInvalidSocketHandler;
}

CIPreemptiveScheduler::~CIPreemptiveScheduler()
{
    LOGM_RESOURCE("~CIPreemptiveScheduler start\n");

    if (mpThread) {
        if (mbRunning) {
            SCMD cmd;
            TChar wake_char = 'c';

            gfOSWritePipe(mPipeFd[1], wake_char);

            cmd.code = ECMDType_ExitRunning;
            cmd.pExtra = NULL;
            cmd.needFreePExtra = 0;
            EECode err = sendCmd(cmd);
            DASSERT_OK(err);
            mbRunning = 0;
        }
        mpThread->Delete();
        mpThread = NULL;
    }

    if (mpMsgQ) {
        mpMsgQ->Delete();
        mpMsgQ = NULL;
    }

    if (mpClientList) {
        delete mpClientList;
        mpClientList = NULL;
    }

    LOGM_RESOURCE("~CIPreemptiveScheduler end\n");
}

EECode CIPreemptiveScheduler::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModulePreemptiveScheduler);

    if ((mpMsgQ = CIQueue::Create(NULL, this, sizeof(SCMD), 16)) == NULL) {
        LOG_FATAL("No memory\n");
        return EECode_NoMemory;
    }

    if ((mpThread = gfCreateThread("CIPreemptiveScheduler", threadEntry, this, ESchedulePolicy_RunRobin, 99, (1 << (mIndex & 0x1)))) == NULL) {
        LOG_FATAL("No memory\n");
        return EECode_NoMemory;
    }

    mbRunning = 1;

    mpClientList = new CIDoubleLinkedList();
    if (!mpClientList) {
        LOG_FATAL("No memory\n");
        return EECode_NoMemory;
    }

    EECode err = gfOSCreatePipe(mPipeFd);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("gfOSCreatePipe() return fail, err %d, %s\n", err, gfGetErrorCodeString(err));
        return EECode_OSError;
    }

    FD_ZERO(&mAllSet);
    FD_SET(mPipeFd[0], &mAllSet);
    mMaxFd = mPipeFd[0];

    return EECode_OK;
}

void CIPreemptiveScheduler::PrintStatus()
{
    LOGM_PRINTF("heart beat %d, %d, msState %d\n", mDebugHeartBeat, mDebugHeartBeat00, msState);
    mDebugHeartBeat = 0;
}

void CIPreemptiveScheduler::Delete()
{
    LOGM_RESOURCE("CIPreemptiveScheduler::Delete() start\n");

    if (mpThread) {
        if (mbRunning) {
            SCMD cmd;
            TChar wake_char = 'c';

            gfOSWritePipe(mPipeFd[1], wake_char);

            cmd.code = ECMDType_ExitRunning;
            cmd.pExtra = NULL;
            cmd.needFreePExtra = 0;
            EECode err = sendCmd(cmd);
            DASSERT_OK(err);
            mbRunning = 0;
        }
        mpThread->Delete();
        mpThread = NULL;
    }

    if (mpMsgQ) {
        mpMsgQ->Delete();
        mpMsgQ = NULL;
    }

    if (mpClientList) {
        delete mpClientList;
        mpClientList = NULL;
    }

    if (DIsSocketHandlerValid(mPipeFd[0])) {
        gfOSClosePipeFd(mPipeFd[0]);
        mPipeFd[0] = DInvalidSocketHandler;
    }
    if (DIsSocketHandlerValid(mPipeFd[1])) {
        gfOSClosePipeFd(mPipeFd[1]);
        mPipeFd[1] = DInvalidSocketHandler;
    }

    LOGM_RESOURCE("CIPreemptiveScheduler::Delete() end\n");
}

EECode CIPreemptiveScheduler::StartScheduling()
{
    TChar wake_char = 'c';
    SCMD cmd;

    cmd.code = ECMDType_Start;
    cmd.pExtra = NULL;

    gfOSWritePipe(mPipeFd[1], wake_char);

    return sendCmd(cmd);
}

EECode CIPreemptiveScheduler::StopScheduling()
{
    TChar wake_char = 'c';
    SCMD cmd;

    cmd.code = ECMDType_Stop;
    cmd.pExtra = NULL;

    gfOSWritePipe(mPipeFd[1], wake_char);

    return sendCmd(cmd);
}

EECode CIPreemptiveScheduler::AddScheduledCilent(IScheduledClient *client)
{
    TChar wake_char = 'c';
    SCMD cmd;

    if (!client) {
        LOGM_FATAL("NULL client!\n");
        return EECode_BadParam;
    }

    cmd.code = ECMDType_AddContent;
    cmd.pExtra = client;

    gfOSWritePipe(mPipeFd[1], wake_char);

    return sendCmd(cmd);
}

EECode CIPreemptiveScheduler::RemoveScheduledCilent(IScheduledClient *client)
{
    TChar wake_char = 'c';
    SCMD cmd;

    if (!client) {
        LOGM_FATAL("NULL client!\n");
        return EECode_BadParam;
    }

    cmd.code = ECMDType_RemoveContent;
    cmd.pExtra = client;

    gfOSWritePipe(mPipeFd[1], wake_char);

    return sendCmd(cmd);
}

TUint CIPreemptiveScheduler::TotalNumberOfScheduledClient() const
{
    DASSERT(mpClientList);
    if (mpClientList) {
        return mpClientList->NumberOfNodes();
    }

    return 0;
}

TUint CIPreemptiveScheduler::CurrentNumberOfClient() const
{
    return mnCurrentActiveClientNumber;
}

TSchedulingUnit CIPreemptiveScheduler::CurrentHungryScore() const
{
    return (TSchedulingUnit)mnCurrentActiveClientNumber;
}

TInt CIPreemptiveScheduler::IsPassiveMode() const
{
    return 0;
}

EECode CIPreemptiveScheduler::insertClient(IScheduledClient *client)
{
    DASSERT(client);
    DASSERT(mpClientList);

    if (client && mpClientList) {
        TInt fd = (TInt) client->GetWaitHandle();

        if ((0 != client->IsPassiveMode()) || (fd < 0)) {
            LOGM_FATAL("bad type of client, fd %d, passive mode %d\n", fd, client->IsPassiveMode());
            return EECode_BadParam;
        }

        if (fd > mMaxFd) {
            mMaxFd = fd;
        }
        //LOGM_DEBUG("insert, fd %d\n", fd);
        FD_SET(fd, &mAllSet);
        mpClientList->InsertContent(NULL, (void *)client, 0);
    } else {
        LOGM_FATAL("NULL client %p or mpClientList %p\n", client, mpClientList);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CIPreemptiveScheduler::removeClient(IScheduledClient *client)
{
    DASSERT(client);
    DASSERT(mpClientList);

    if (client && mpClientList) {
        TInt fd = (TInt) client->GetWaitHandle();

        if ((0 != client->IsPassiveMode()) || (fd < 0)) {
            LOGM_FATAL("bad type of client, fd %d, passive mode %d\n", fd, client->IsPassiveMode());
            return EECode_BadParam;
        }

        //LOGM_DEBUG("remove, fd %d\n", fd);
        FD_CLR(fd, &mAllSet);
        mpClientList->RemoveContent((void *)client);
    } else {
        LOGM_FATAL("NULL client %p or mpClientList %p\n", client, mpClientList);
        return EECode_BadParam;
    }

    return EECode_OK;
}

void CIPreemptiveScheduler::processCmd(SCMD &cmd)
{
    TChar char_buffer;
    LOGM_FLOW("processCmd, cmd.code %d, %s, state %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code), msState, gfGetModuleStateString(msState));

    gfOSReadPipe(mPipeFd[0], char_buffer);

    switch (cmd.code) {

        case ECMDType_StartRunning:
            DASSERT(EModuleState_WaitCmd == msState);
            msState = EModuleState_Running;
            cmdAck(EECode_OK);
            LOGM_INFO("processCmd, ECMDType_StartRunning.\n");
            break;

        case ECMDType_ExitRunning:
            mbRun = 0;
            cmdAck(EECode_OK);
            LOGM_INFO("processCmd, ECMDType_ExitRunning.\n");
            break;

        case ECMDType_Stop:
            DASSERT(EModuleState_Running == msState);
            msState = EModuleState_Pending;
            mbStarted = 0;
            cmdAck(EECode_OK);
            LOGM_INFO("processCmd, ECMDType_Stop.\n");
            break;

        case ECMDType_Start:
            DASSERT((EModuleState_WaitCmd == msState) || (EModuleState_Pending == msState));
            msState = EModuleState_Running;
            mbStarted = 1;
            cmdAck(EECode_OK);
            LOGM_INFO("processCmd, ECMDType_Start.\n");
            break;

        case ECMDType_AddContent:
            cmdAck(insertClient((IScheduledClient *)cmd.pExtra));
            LOGM_INFO("processCmd, ECMDType_AddContent.\n");
            break;

        case ECMDType_RemoveContent:
            cmdAck(removeClient((IScheduledClient *)cmd.pExtra));
            LOGM_INFO("processCmd, ECMDType_RemoveContent.\n");
            break;

        default:
            LOGM_ERROR("processCmd, wrong cmd %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code));
            break;
    }
}

EECode CIPreemptiveScheduler::sendCmd(SCMD &cmd)
{
    return mpMsgQ->SendMsg(&cmd, sizeof(cmd));
}

void CIPreemptiveScheduler::getCmd(SCMD &cmd)
{
    mpMsgQ->GetMsg(&cmd, sizeof(cmd));
}

TU8 CIPreemptiveScheduler::peekCmd(SCMD &cmd)
{
    if (false == mpMsgQ->PeekMsg(&cmd, sizeof(cmd))) {
        return 0;
    }

    return 1;
}

EECode CIPreemptiveScheduler::postMsg(SCMD &cmd)
{
    return mpMsgQ->PostMsg(&cmd, sizeof(cmd));
}

void CIPreemptiveScheduler::cmdAck(EECode result)
{
    mpMsgQ->Reply(result);
}

EECode CIPreemptiveScheduler::threadEntry(void *p)
{
    //LOG_NOTICE("CIPreemptiveScheduler::threadEntry, p %p begin\n", p);
    ((CIPreemptiveScheduler *)p)->mainLoop();
    //LOG_NOTICE("CIPreemptiveScheduler::threadEntry, p %p end\n", p);
    return EECode_OK;
}

void CIPreemptiveScheduler::mainLoop()
{
    SCMD cmd;
    TInt nready = 0;
    TSchedulingHandle cur_fd = 0;
    TUint currentActiveClientNumber = 0;
    CIDoubleLinkedList::SNode *pnode = NULL;
    IScheduledClient *p_client = NULL;
    EECode err;

    //signal(SIGPIPE,SIG_IGN);
    mbRun = 1;
    msState = EModuleState_WaitCmd;

    while (mbRun) {

        LOGM_STATE("mainLoop: start switch, msState=%d, %s\n", msState, gfGetModuleStateString(msState));
        mDebugHeartBeat00 = 10;

        switch (msState) {

            case EModuleState_WaitCmd:
                getCmd(cmd);
                processCmd(cmd);
                break;

            case EModuleState_Error:
            case EModuleState_Pending:
                getCmd(cmd);
                processCmd(cmd);
                break;

            case EModuleState_Running:
                //LOGM_VERBOSE("CIPreemptiveScheduler::mainLoop: start client's work loop, total count %d, mnCurrentActiveClientNumber %d, mMaxFd %d.\n", mpClientList->NumberOfNodes(), mnCurrentActiveClientNumber, mMaxFd);
                mReadSet = mAllSet;
                //mErrorSet = mAllSet;
                mDebugHeartBeat00 = 1;
                nready = select(mMaxFd + 1, &mReadSet, NULL, NULL, NULL);
                //LOGM_DEBUG("nready %d\n", nready);
                //process cmd
                if (FD_ISSET(mPipeFd[0], &mReadSet)) {
                    //LOGM_DEBUG("[CIPreemptiveScheduler]: from pipe fd.\n");
                    while (peekCmd(cmd)) {
                        processCmd(cmd);
                    }

                    if ((EModuleState_Running != msState) || (!mbRun)) {
                        LOGM_INFO("exit here\n");
                        break;
                    }
                    nready --;
                    if (nready <= 0) {
                        break;
                    }
                }
                mDebugHeartBeat00 = 2;
                currentActiveClientNumber = 0;

                pnode = mpClientList->FirstNode();
                while (pnode) {
                    p_client = (IScheduledClient *)(pnode->p_context);
                    pnode = mpClientList->NextNode(pnode);
                    if (DLikely(p_client)) {
                        cur_fd = p_client->GetWaitHandle();
                        if (FD_ISSET((TInt)cur_fd, &mReadSet)) {
                            mDebugHeartBeat00 = 3;
                            err = p_client->Scheduling();
                            mDebugHeartBeat00 = 4;
                            if (DLikely(EECode_OK == err)) {
                                currentActiveClientNumber ++;
                            } else if (EECode_Closed == err) {
                                LOGM_NOTICE("EECode_Closed\n");
                                mDebugHeartBeat00 = 5;
                                removeClient(p_client);
                                p_client->EventHandling(EECode_Closed);
                                mDebugHeartBeat00 = 6;
                            } else if (EECode_NotRunning == err) {

                            } else {
                                LOGM_ERROR("return %d, %s\n", err, gfGetErrorCodeString(err));
                                mDebugHeartBeat00 = 7;
                                err = p_client->EventHandling(EECode_Closed);
                                if (EECode_AutoDelete == err) {
                                    removeClient(p_client);
                                    p_client = NULL;
                                }
                                mDebugHeartBeat00 = 8;
                            }
                            nready --;
                        }
#if 0
                        else if (FD_ISSET((TInt)cur_fd, &mErrorSet)) {
                            LOGM_INFO("in error set, close it\n");
                            p_client->EventHandling(EECode_Closed);
                        }
#endif
                    } else {
                        LOGM_FATAL("NULL pointer here, something would be wrong.\n");
                    }

                    if (nready <= 0) {
                        break;
                    }
                }
                mnCurrentActiveClientNumber = currentActiveClientNumber;

                if (!mnCurrentActiveClientNumber) {
                    //LOGM_VERBOSE("idle loop\n");
                    gfOSusleep(1000);
                }
                break;

            default:
                LOGM_FATAL("OnRun: state invalid: %d\n", (TUint)msState);
                msState = EModuleState_Error;
                break;
        }

        mDebugHeartBeat ++;
        mDebugHeartBeat00 = 9;
    }
}

IScheduler *CIPriorityPreemptiveScheduler::Create(const TChar *name, TUint index)
{
    CIPriorityPreemptiveScheduler *result = new CIPriorityPreemptiveScheduler(name, index);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CObject *CIPriorityPreemptiveScheduler::GetObject0() const
{
    return (CObject *) this;
}

CIPriorityPreemptiveScheduler::CIPriorityPreemptiveScheduler(const TChar *name, TUint index)
    : inherited(name, index)
    , mpMsgQ(NULL)
    , mpThread(NULL)
    , mbStarted(0)
    , mbThreadRun(0)
    , mbRun(1)
    , mbRunning(0)
    , msState(EModuleState_Invalid)
    , mnTotalClientNumber(0)
    , mnCurrentActiveClientNumber(0)
{
    TUint i = 0;

    mPipeFd[0] = DInvalidSocketHandler;
    mPipeFd[1] = DInvalidSocketHandler;

    for (i = 0; i < 4; i ++) {
        mpClientLists[i] = NULL;
        mMaxFds[i] = 0;
    }
}

CIPriorityPreemptiveScheduler::~CIPriorityPreemptiveScheduler()
{
    TUint i = 0;

    LOGM_RESOURCE("~CIPriorityPreemptiveScheduler start\n");

    if (mpThread) {
        if (mbRunning) {
            SCMD cmd;
            TChar wake_char = 'c';

            gfOSWritePipe(mPipeFd[1], wake_char);

            cmd.code = ECMDType_ExitRunning;
            cmd.pExtra = NULL;
            cmd.needFreePExtra = 0;
            EECode err = sendCmd(cmd);
            DASSERT_OK(err);
            mbRunning = 0;
        }
        mpThread->Delete();
        mpThread = NULL;
    }

    if (mpMsgQ) {
        mpMsgQ->Delete();
        mpMsgQ = NULL;
    }

    for (i = 0; i < 4; i ++) {
        if (mpClientLists[i]) {
            delete mpClientLists[i];
            mpClientLists[i] = NULL;
        }
    }

    LOGM_RESOURCE("~CIPriorityPreemptiveScheduler end\n");
}

EECode CIPriorityPreemptiveScheduler::Construct()
{
    TUint i = 0;

    DSET_MODULE_LOG_CONFIG(ELogModulePreemptiveScheduler);

    if ((mpMsgQ = CIQueue::Create(NULL, this, sizeof(SCMD), 16)) == NULL) {
        LOG_FATAL("No memory\n");
        return EECode_NoMemory;
    }

    if ((mpThread = gfCreateThread("CIPriorityPreemptiveScheduler", threadEntry, this, ESchedulePolicy_RunRobin, 99, 0)) == NULL) {
        LOG_FATAL("No memory\n");
        return EECode_NoMemory;
    }

    mbRunning = 1;

    EECode err = gfOSCreatePipe(mPipeFd);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("gfOSCreatePipe() return fail, err %d, %s\n", err, gfGetErrorCodeString(err));
        return EECode_OSError;
    }

    for (i = 0; i < 4; i ++) {
        mpClientLists[i] = new CIDoubleLinkedList();

        if (!mpClientLists[i]) {
            LOG_FATAL("No memory\n");
            return EECode_NoMemory;
        }

        FD_ZERO(&mAllSets[i]);
        FD_SET(mPipeFd[0], &mAllSets[i]);
        mMaxFds[i] = mPipeFd[0];
    }

    return EECode_OK;
}

void CIPriorityPreemptiveScheduler::PrintStatus()
{
    LOGM_PRINTF("heart beat %d, msState %d\n", mDebugHeartBeat, msState);
    mDebugHeartBeat = 0;
}

void CIPriorityPreemptiveScheduler::Delete()
{
    TUint i = 0;

    LOGM_RESOURCE("CIPriorityPreemptiveScheduler::Delete() start\n");

    if (mpThread) {
        if (mbRunning) {
            SCMD cmd;
            TChar wake_char = 'c';

            gfOSWritePipe(mPipeFd[1], wake_char);

            cmd.code = ECMDType_ExitRunning;
            cmd.pExtra = NULL;
            cmd.needFreePExtra = 0;
            EECode err = sendCmd(cmd);
            DASSERT_OK(err);
            mbRunning = 0;
        }
        mpThread->Delete();
        mpThread = NULL;
    }

    if (mpMsgQ) {
        mpMsgQ->Delete();
        mpMsgQ = NULL;
    }

    for (i = 0; i < 4; i ++) {
        if (mpClientLists[i]) {
            delete mpClientLists[i];
            mpClientLists[i] = NULL;
        }
    }

    if (DIsSocketHandlerValid(mPipeFd[0])) {
        gfOSClosePipeFd(mPipeFd[0]);
        mPipeFd[0] = DInvalidSocketHandler;
    }
    if (DIsSocketHandlerValid(mPipeFd[1])) {
        gfOSClosePipeFd(mPipeFd[1]);
        mPipeFd[1] = DInvalidSocketHandler;
    }

    LOGM_RESOURCE("CIPriorityPreemptiveScheduler::Delete() end\n");
}

EECode CIPriorityPreemptiveScheduler::StartScheduling()
{
    TChar wake_char = 'c';
    SCMD cmd;

    cmd.code = ECMDType_Start;
    cmd.pExtra = NULL;

    gfOSWritePipe(mPipeFd[1], wake_char);

    return sendCmd(cmd);
}

EECode CIPriorityPreemptiveScheduler::StopScheduling()
{
    TChar wake_char = 'c';
    SCMD cmd;

    cmd.code = ECMDType_Stop;
    cmd.pExtra = NULL;

    gfOSWritePipe(mPipeFd[1], wake_char);

    return sendCmd(cmd);
}

EECode CIPriorityPreemptiveScheduler::AddScheduledCilent(IScheduledClient *client)
{
    TChar wake_char = 'c';
    SCMD cmd;

    if (!client) {
        LOGM_FATAL("NULL client!\n");
        return EECode_BadParam;
    }

    cmd.code = ECMDType_AddContent;
    cmd.pExtra = client;

    gfOSWritePipe(mPipeFd[1], wake_char);

    return sendCmd(cmd);
}

EECode CIPriorityPreemptiveScheduler::RemoveScheduledCilent(IScheduledClient *client)
{
    TChar wake_char = 'c';
    SCMD cmd;

    if (!client) {
        LOGM_FATAL("NULL client!\n");
        return EECode_BadParam;
    }

    cmd.code = ECMDType_RemoveContent;
    cmd.pExtra = client;

    gfOSWritePipe(mPipeFd[1], wake_char);

    return sendCmd(cmd);
}

TUint CIPriorityPreemptiveScheduler::TotalNumberOfScheduledClient() const
{
    return mnTotalClientNumber;
}

TUint CIPriorityPreemptiveScheduler::CurrentNumberOfClient() const
{
    return mnCurrentActiveClientNumber;
}

TSchedulingUnit CIPriorityPreemptiveScheduler::CurrentHungryScore() const
{
    return (TSchedulingUnit)mnCurrentActiveClientNumber;
}

TInt CIPriorityPreemptiveScheduler::IsPassiveMode() const
{
    return 0;
}

EECode CIPriorityPreemptiveScheduler::insertClient(IScheduledClient *client)
{
    DASSERT(client);

    if (client) {
        TInt fd = (TInt) client->GetWaitHandle();
        TUint priority = client->GetPriority();

        DASSERT(priority < 4);
        if (priority >= 4) {
            priority = 3;
        }

        DASSERT(mpClientLists[priority]);

        if ((0 != client->IsPassiveMode()) || (fd < 0)) {
            LOGM_FATAL("bad type of client, fd %d, passive mode %d\n", fd, client->IsPassiveMode());
            return EECode_BadParam;
        }

        if (fd > mMaxFds[priority]) {
            mMaxFds[priority] = fd;
        }
        //LOGM_INFO("insert, fd %d\n", fd);
        FD_SET(fd, &mAllSets[priority]);

        mpClientLists[priority]->InsertContent(NULL, (void *)client, 0);
        mnTotalClientNumber ++;

        if (priority) {
            if (fd > mMaxFds[0]) {
                mMaxFds[0] = fd;
            }
            //LOGM_INFO("insert, fd %d\n", fd);
            FD_SET(fd, &mAllSets[0]);
            mpClientLists[0]->InsertContent(NULL, (void *)client, 0);
        }
    } else {
        LOGM_FATAL("NULL client %p\n", client);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CIPriorityPreemptiveScheduler::removeClient(IScheduledClient *client)
{
    DASSERT(client);

    if (client) {
        TInt fd = (TInt) client->GetWaitHandle();
        TUint priority = client->GetPriority();

        DASSERT(priority < 4);
        if (priority >= 4) {
            priority = 3;
        }

        DASSERT(mpClientLists[priority]);

        if ((0 != client->IsPassiveMode()) || (fd < 0)) {
            LOGM_FATAL("bad type of client, fd %d, passive mode %d\n", fd, client->IsPassiveMode());
            return EECode_BadParam;
        }

        LOGM_INFO("remove, fd %d\n", fd);
        FD_CLR(fd, &mAllSets[priority]);
        mpClientLists[priority]->RemoveContent((void *)client);
        DASSERT(mnTotalClientNumber > 0);
        mnTotalClientNumber --;

        if (priority) {
            FD_CLR(fd, &mAllSets[0]);
            mpClientLists[0]->RemoveContent((void *)client);
        }

    } else {
        LOGM_FATAL("NULL client %p\n", client);
        return EECode_BadParam;
    }

    return EECode_OK;
}

void CIPriorityPreemptiveScheduler::processCmd(SCMD &cmd)
{
    TChar char_buffer;
    LOGM_FLOW("processCmd, cmd.code %d, %s, state %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code), msState, gfGetModuleStateString(msState));

    gfOSReadPipe(mPipeFd[0], char_buffer);

    switch (cmd.code) {

        case ECMDType_StartRunning:
            DASSERT(EModuleState_WaitCmd == msState);
            msState = EModuleState_Running;
            cmdAck(EECode_OK);
            LOGM_INFO("processCmd, ECMDType_StartRunning.\n");
            break;

        case ECMDType_ExitRunning:
            mbRun = 0;
            cmdAck(EECode_OK);
            LOGM_INFO("processCmd, ECMDType_ExitRunning.\n");
            break;

        case ECMDType_Stop:
            DASSERT(EModuleState_Running == msState);
            msState = EModuleState_Pending;
            mbStarted = 0;
            cmdAck(EECode_OK);
            LOGM_INFO("processCmd, ECMDType_Stop.\n");
            break;

        case ECMDType_Start:
            DASSERT((EModuleState_WaitCmd == msState) || (EModuleState_Pending == msState));
            msState = EModuleState_Running;
            mbStarted = 1;
            cmdAck(EECode_OK);
            LOGM_INFO("processCmd, ECMDType_Start.\n");
            break;

        case ECMDType_AddContent:
            cmdAck(insertClient((IScheduledClient *)cmd.pExtra));
            LOGM_INFO("processCmd, ECMDType_AddContent.\n");
            break;

        case ECMDType_RemoveContent:
            cmdAck(removeClient((IScheduledClient *)cmd.pExtra));
            LOGM_INFO("processCmd, ECMDType_RemoveContent.\n");
            break;

        default:
            LOGM_ERROR("processCmd, wrong cmd %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code));
            break;
    }
}

EECode CIPriorityPreemptiveScheduler::sendCmd(SCMD &cmd)
{
    return mpMsgQ->SendMsg(&cmd, sizeof(cmd));
}

void CIPriorityPreemptiveScheduler::getCmd(SCMD &cmd)
{
    mpMsgQ->GetMsg(&cmd, sizeof(cmd));
}

TU8 CIPriorityPreemptiveScheduler::peekCmd(SCMD &cmd)
{
    if (false == mpMsgQ->PeekMsg(&cmd, sizeof(cmd))) {
        return 0;
    }

    return 1;
}

EECode CIPriorityPreemptiveScheduler::postMsg(SCMD &cmd)
{
    return mpMsgQ->PostMsg(&cmd, sizeof(cmd));
}

void CIPriorityPreemptiveScheduler::cmdAck(EECode result)
{
    mpMsgQ->Reply(result);
}

EECode CIPriorityPreemptiveScheduler::threadEntry(void *p)
{
    LOG_NOTICE("CIPriorityPreemptiveScheduler::threadEntry, p %p begin\n", p);
    ((CIPriorityPreemptiveScheduler *)p)->mainLoop();
    LOG_NOTICE("CIPriorityPreemptiveScheduler::threadEntry, p %p end\n", p);
    return EECode_OK;
}

TUint CIPriorityPreemptiveScheduler::loopSlice(TUint index)
{
    SCMD cmd;
    TInt nready = 0;
    TSchedulingHandle cur_fd = 0;
    TUint currentActiveClientNumber = 0;
    CIDoubleLinkedList::SNode *pnode = NULL;
    IScheduledClient *p_client = NULL;
    EECode err;

    if (DUnlikely(index >= 4)) {
        LOGM_FATAL("array out of bounds\n");
        return 0;
    }

    mReadSets[index] = mAllSets[index];
    nready = select(mMaxFds[index] + 1, &mReadSets[index], NULL, NULL, NULL);
    //LOGM_INFO("nready %d\n", nready);

    //process cmd
    if (FD_ISSET(mPipeFd[0], &mReadSets[index])) {
        //LOGM_DEBUG("[CIPriorityPreemptiveScheduler]: from pipe fd.\n");
        while (peekCmd(cmd)) {
            processCmd(cmd);
        }

        if ((EModuleState_Running != msState) || (!mbRun)) {
            //LOGM_INFO("exit here\n");
            //err = EECode_NotRunning;
            return 0;
        }
        nready --;
        if (nready <= 0) {
            LOGM_INFO("exit here\n");
            //err = EECode_NotRunning;
            return 0;
        }
    }

    currentActiveClientNumber = 0;

    pnode = mpClientLists[index]->FirstNode();
    while (pnode) {
        p_client = (IScheduledClient *)(pnode->p_context);
        pnode = mpClientLists[index]->NextNode(pnode);
        DASSERT(p_client);
        if (p_client) {
            cur_fd = p_client->GetWaitHandle();
            if (FD_ISSET((TInt)cur_fd, &mReadSets[index])) {
                err = p_client->Scheduling();
                if (EECode_OK == err) {
                    currentActiveClientNumber ++;
                }
            }
        } else {
            LOGM_FATAL("NULL pointer here, something would be wrong.\n");
        }
    }

    //err = EECode_OK;

    return currentActiveClientNumber;
}

void CIPriorityPreemptiveScheduler::mainLoop()
{
    SCMD cmd;

    //signal(SIGPIPE,SIG_IGN);
    mbRun = 1;
    msState = EModuleState_WaitCmd;

    while (mbRun) {

        //LOGM_STATE("mainLoop: start switch, msState=%d, %s, mpClientLists[1]->NumberOfNodes() %d, mpClientLists[0]->NumberOfNodes() %d, mpClientLists[2]->NumberOfNodes() %d\n", msState, gfGetModuleStateString(msState), mpClientLists[1]->NumberOfNodes(), mpClientLists[0]->NumberOfNodes(), mpClientLists[2]->NumberOfNodes());

        switch (msState) {

            case EModuleState_WaitCmd:
                getCmd(cmd);
                processCmd(cmd);
                break;

            case EModuleState_Error:
            case EModuleState_Pending:
                getCmd(cmd);
                processCmd(cmd);
                break;

            case EModuleState_Running:

                //process cmd
                while (peekCmd(cmd)) {
                    processCmd(cmd);
                }

                if ((EModuleState_Running != msState) || (!mbRun)) {
                    LOGM_INFO("exit here\n");
                    //err = EECode_NotRunning;
                    break;
                }

                mnCurrentActiveClientNumber = 0;

                if (mpClientLists[2]->NumberOfNodes()) {
                    mnCurrentActiveClientNumber += loopSlice(2);
                    //LOGM_DEBUG("mainLoop: 2, mpClientLists[2]->NumberOfNodes() %d, mnCurrentActiveClientNumber %d\n", mpClientLists[2]->NumberOfNodes(), mnCurrentActiveClientNumber);
                }

                if (mpClientLists[1]->NumberOfNodes()) {
                    mnCurrentActiveClientNumber += loopSlice(1);
                    //LOGM_DEBUG("mainLoop: 1, mpClientLists[1]->NumberOfNodes() %d, mnCurrentActiveClientNumber %d\n", mpClientLists[1]->NumberOfNodes(), mnCurrentActiveClientNumber);
                }

                if (mpClientLists[2]->NumberOfNodes()) {
                    mnCurrentActiveClientNumber += loopSlice(2);
                    //LOGM_DEBUG("mainLoop: 2, mpClientLists[2]->NumberOfNodes() %d, mnCurrentActiveClientNumber %d\n", mpClientLists[2]->NumberOfNodes(), mnCurrentActiveClientNumber);
                }

                if (mpClientLists[0]->NumberOfNodes()) {
                    mnCurrentActiveClientNumber += loopSlice(0);
                    //LOGM_DEBUG("mainLoop: 0, mpClientLists[0]->NumberOfNodes() %d, mnCurrentActiveClientNumber %d\n", mpClientLists[0]->NumberOfNodes(), mnCurrentActiveClientNumber);
                }

                if (mpClientLists[2]->NumberOfNodes()) {
                    mnCurrentActiveClientNumber += loopSlice(2);
                    //LOGM_DEBUG("mainLoop: 2, mpClientLists[2]->NumberOfNodes() %d, mnCurrentActiveClientNumber %d\n", mpClientLists[2]->NumberOfNodes(), mnCurrentActiveClientNumber);
                }

                if (!mnCurrentActiveClientNumber) {
                    //LOGM_VERBOSE("idle loop\n");
                    gfOSusleep(1000);
                }
                break;

            default:
                LOGM_FATAL("OnRun: state invalid: %d\n", (TUint)msState);
                msState = EModuleState_Error;
                break;
        }

        mDebugHeartBeat ++;
    }
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

