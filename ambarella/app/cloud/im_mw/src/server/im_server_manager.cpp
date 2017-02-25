/*******************************************************************************
 * im_server_manager.cpp
 *
 * History:
 *    2014/06/17 - [Zhi He] create file
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
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#else
#include <io.h>
#endif

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "cloud_lib_if.h"

#include "im_mw_if.h"
#include "im_server_manager.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IIMServerManager *gfCreateIMServerManager(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
{
    return (IIMServerManager *)CIMServerManager::Create(pconfig, pMsgSink, p_system_clock_reference);
}

CIMServerManager::CIMServerManager(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
    : inherited("CIMServerManager", 0)
    , mpPersistCommonConfig(pconfig)
    , mpMsgSink(pMsgSink)
    , mpSystemClockReference(p_system_clock_reference)
    , mpWorkQueue(NULL)
    , mbRun(1)
    , msState(EServerManagerState_idle)
    , mnServerNumber(0)
    , mMaxFd(-1)
    , mpServerList(NULL)
{
    mPipeFd[0] = mPipeFd[1] = -1;
}

CIMServerManager::~CIMServerManager()
{
}

IIMServerManager *CIMServerManager::Create(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
{
    CIMServerManager *result = new CIMServerManager(pconfig, pMsgSink, p_system_clock_reference);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CObject *CIMServerManager::GetObject() const
{
    return (CObject *) this;
}

EECode CIMServerManager::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleIMServerManager);
    //mConfigLogLevel = ELogLevel_Debug;
    //mConfigLogOption = 0xffffffff;

    mpServerList = new CIDoubleLinkedList();
    if (DUnlikely(!mpServerList)) {
        LOGM_ERROR("new CIDoubleLinkedList() fail.\n");
        return EECode_NoMemory;
    }

    mpWorkQueue = CIWorkQueue::Create((IActiveObject *)this);
    if (DUnlikely(!mpWorkQueue)) {
        LOGM_ERROR("Create CIWorkQueue fail.\n");
        return EECode_NoMemory;
    }

    EECode err = gfOSCreatePipe(mPipeFd);
    DASSERT(EECode_OK == err);

    LOGM_INFO("before CIMServerManager:: mpWorkQueue->Run().\n");
    DASSERT(mpWorkQueue);
    mpWorkQueue->Run();
    LOGM_INFO("after CIMServerManager:: mpWorkQueue->Run().\n");

    //init
    FD_ZERO(&mAllSet);
    FD_SET(mPipeFd[0], &mAllSet);
    mMaxFd = mPipeFd[0];

    return EECode_OK;
}

void CIMServerManager::PrintStatus0()
{
    PrintStatus();
}

void CIMServerManager::Destroy()
{
    Delete();
}

void CIMServerManager::Delete()
{
    if (mpServerList) {
        CIDoubleLinkedList::SNode *pnode;
        IIMServer *p_server;
        pnode = mpServerList->FirstNode();
        while (pnode) {
            p_server = (IIMServer *)(pnode->p_context);
            pnode = mpServerList->NextNode(pnode);
            if (p_server) {
                p_server->Destroy();
            } else {
                LOGM_FATAL("NULL pointer here, something would be wrong.\n");
            }
        }

        delete mpServerList;
        mpServerList = NULL;
    }
}

void CIMServerManager::processCmd(SCMD &cmd)
{
    IIMServer *server_info;
    TInt server_handle;
    TChar char_buffer;

    gfOSReadPipe(mPipeFd[0], char_buffer);

    switch (cmd.code) {
        case ECMDType_Start:
            msState = EServerManagerState_noserver_alive;
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        case ECMDType_Stop:
            msState = EServerManagerState_halt;
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        case ECMDType_AddClient:
            DASSERT(cmd.pExtra);
            server_info = (IIMServer *)cmd.pExtra;
            if (server_info) {
                server_info->GetHandler(server_handle);
                DASSERT(0 <= server_handle);
                FD_SET(server_handle, &mAllSet);

                server_info->SetServerState(EServerState_running);

                mpServerList->InsertContent(NULL, (void *)server_info, 0);
                mpWorkQueue->CmdAck(EECode_OK);
            } else {
                LOGM_ERROR("NULL pointer here, must have errors.\n");
                mpWorkQueue->CmdAck(EECode_BadCommand);
            }
            break;

        case ECMDType_RemoveClient:
            DASSERT(cmd.pExtra);
            server_info = (IIMServer *)cmd.pExtra;
            if (server_info) {
                server_info->GetHandler(server_handle);
                DASSERT(0 <= server_handle);
                FD_CLR(server_handle, &mAllSet);

                server_info->SetServerState(EServerState_closed);

                mpServerList->RemoveContent((void *)server_info);
                mpWorkQueue->CmdAck(EECode_OK);
            } else {
                LOGM_ERROR("NULL pointer here, must have errors.\n");
                mpWorkQueue->CmdAck(EECode_BadState);
            }
            break;

        default:
            LOGM_ERROR("wrong cmd.code: %d", cmd.code);
            break;
    }

    //LOGM_FLOW("****CIMServerManager::ProcessCmd, cmd.code %d end.\n", cmd.code);
    return;
}

void CIMServerManager::PrintStatus()
{
    mDebugHeartBeat = 0;
}

void CIMServerManager::OnRun()
{
    SCMD cmd;
    TUint alive_server_num = 0;
    TInt nready = 0;
    IIMServer *p_server;

    EServerState server_state;
    TInt server_hander = 0;

    CIDoubleLinkedList::SNode *pnode;
    mpWorkQueue->CmdAck(EECode_OK);

    while (mbRun) {

        //LOGM_STATE("CIMServerManager::OnRun start switch state %d, %s, mbRun %d.\n", msState, gfGetServerManagerStateString((EServerManagerState) msState), mbRun);

        switch (msState) {

            case EServerManagerState_idle:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EServerManagerState_noserver_alive:
                pnode = mpServerList->FirstNode();
                while (pnode) {
                    p_server = (IIMServer *)pnode->p_context;
                    DASSERT(p_server);
                    if (DUnlikely(NULL == p_server)) {
                        LOGM_ERROR("Fatal error, No server? must not get here.\n");
                        break;
                    }

                    p_server->GetHandler(server_hander);
                    p_server->GetServerState(server_state);

                    if (EServerState_running == server_state) {
                        DASSERT(0 < server_hander);
                        LOGM_NOTICE("add: server_hander %d, mMaxFd %d\n", server_hander, mMaxFd);
                        if (server_hander >= 0) {
                            FD_SET(server_hander, &mAllSet);
                            mMaxFd = (mMaxFd < server_hander) ? server_hander : mMaxFd;
                            alive_server_num ++;
                            LOGM_NOTICE("after add: server_hander %d, mMaxFd %d\n", server_hander, mMaxFd);
                        }
                    }

                    pnode = mpServerList->NextNode(pnode);
                }
                if (alive_server_num) {
                    LOGM_NOTICE("There's some(%d) server alive, transit to running state.\n", alive_server_num);
                    msState = EServerManagerState_running;
                    break;
                }
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EServerManagerState_running:
                DASSERT(alive_server_num > 0);
                mReadSet = mAllSet;

                nready = select(mMaxFd + 1, &mReadSet, NULL, NULL, NULL);
                if (DUnlikely(0 > nready)) {
                    LOGM_FATAL("select fail, nready %d\n", nready);
                    msState = EServerManagerState_error;
                    break;
                } else if (nready == 0) {
                    break;
                }

                if (FD_ISSET(mPipeFd[0], &mReadSet)) {
                    mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                    processCmd(cmd);
                    nready --;
                    if (EServerManagerState_running != msState) {
                        LOGM_INFO(" transit from EServerManagerState_running to state %d.\n", msState);
                        break;
                    }

                    if (nready <= 0) {
                        //read done
                        LOGM_INFO(" read done.\n");
                        break;
                    }
                }

                pnode = mpServerList->FirstNode();

                while (pnode) {
                    p_server = (IIMServer *)pnode->p_context;
                    DASSERT(p_server);
                    if (DUnlikely(NULL == p_server)) {
                        LOGM_FATAL("Fatal error(NULL == p_server), must not get here.\n");
                        break;
                    }

                    p_server->GetHandler(server_hander);
                    if (DLikely(0 <= server_hander)) {
                        if (FD_ISSET(server_hander, &mReadSet)) {
                            nready --;
                            //new client's request comes
                            p_server->HandleServerRequest(mMaxFd, (void *)&mAllSet);
                        }
                    }

                    if (nready <= 0) {
                        //read done
                        break;
                    }
                    pnode = mpServerList->NextNode(pnode);
                }
                break;

            case EServerManagerState_halt:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EServerManagerState_error:
                //todo
                LOGM_ERROR("NEED implement this case.\n");
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            default:
                mbRun = 0;
                LOGM_FATAL("bad state %d.\n", msState);
                break;
        }

        mDebugHeartBeat ++;

    }

}

EECode CIMServerManager::StartServerManager()
{
    EECode err;
    DASSERT(mpWorkQueue);

    LOGM_INFO("CIMServerManager::StartServerManager start.\n");
    //wake up server manager, post cmd to it
    TChar wake_char = 'a';

    gfOSWritePipe(mPipeFd[1], wake_char);

    err = mpWorkQueue->SendCmd(ECMDType_Start, NULL);
    LOGM_INFO("CIMServerManager::StartServerManager end, ret %d.\n", err);
    return err;
}

EECode CIMServerManager::StopServerManager()
{
    EECode err;
    DASSERT(mpWorkQueue);

    LOGM_INFO("CIMServerManager::StopServerManager start.\n");
    //wake up server manager, post cmd to it
    TChar wake_char = 'b';

    gfOSWritePipe(mPipeFd[1], wake_char);

    err = mpWorkQueue->SendCmd(ECMDType_Stop, NULL);
    LOGM_INFO("CIMServerManager::StopServerManager end, ret %d.\n", err);
    return err;
}

EECode CIMServerManager::AddServer(IIMServer *p_server)
{
    DASSERT(mpWorkQueue);

    //wake up server manager, send cmd to it
    TChar wake_char = 'c';

    gfOSWritePipe(mPipeFd[1], wake_char);

    return mpWorkQueue->SendCmd(ECMDType_AddClient, (void *)p_server);
}

EECode CIMServerManager::RemoveServer(IIMServer *server)
{
    DASSERT(mpWorkQueue);
    //wake up server manager, post cmd to it
    TChar wake_char = 'd';

    gfOSWritePipe(mPipeFd[1], wake_char);

    return mpWorkQueue->SendCmd(ECMDType_RemoveClient, (void *)server);
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


