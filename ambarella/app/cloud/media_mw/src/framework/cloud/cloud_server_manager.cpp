/*******************************************************************************
 * cloud_server_manager.cpp
 *
 * History:
 *    2013/12/02 - [Zhi He] create file
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
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "media_mw_if.h"
#include "media_mw_utils.h"
#include "framework_interface.h"
#include "mw_internal.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"
#include "cloud_server_manager.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

typedef struct {
    ICloudServer *server_info;
    SCloudContent *content;
} _SAddUploadingContent;

typedef struct {
    ICloudServer *server_info;
    void *p_filter;
} _SRemoveUploadingClient;

ICloudServerManager *gfCreateCloudServerManager(const volatile SPersistMediaConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
{
    return (ICloudServerManager *)CCloudServerManager::Create(pconfig, pMsgSink, p_system_clock_reference);
}

CCloudServerManager::CCloudServerManager(const volatile SPersistMediaConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
    : inherited("CCloudServerManager", 0)
    , mpPersistMediaConfig(pconfig)
    , mpMsgSink(pMsgSink)
    , mpSystemClockReference(p_system_clock_reference)
    , mpWorkQueue(NULL)
    , mbRun(1)
    , msState(EServerManagerState_idle)
    , mMaxFd(-1)
    , mpServerList(NULL)
    , mnServerNumber(0)
{
    mPipeFd[0] = mPipeFd[1] = -1;
}

CCloudServerManager::~CCloudServerManager()
{
}

ICloudServerManager *CCloudServerManager::Create(const volatile SPersistMediaConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
{
    CCloudServerManager *result = new CCloudServerManager(pconfig, pMsgSink, p_system_clock_reference);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CObject *CCloudServerManager::GetObject() const
{
    return (CObject *) this;
}

EECode CCloudServerManager::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleCloudServerManager);
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
    DASSERT_OK(err);

    //init
    FD_ZERO(&mAllSet);
    FD_SET(mPipeFd[0], &mAllSet);
    mMaxFd = mPipeFd[0];

    return EECode_OK;
}

void CCloudServerManager::PrintStatus0()
{
    PrintStatus();
}

void CCloudServerManager::Destroy()
{
    Delete();
}

void CCloudServerManager::Delete()
{
    if (mpServerList) {
        CIDoubleLinkedList::SNode *pnode;

        ICloudServer *p_server;
        pnode = mpServerList->FirstNode();
        while (pnode) {
            p_server = (ICloudServer *)(pnode->p_context);
            pnode = mpServerList->NextNode(pnode);
            if (p_server) {
                p_server->GetObject0()->Delete();
            } else {
                LOGM_FATAL("NULL pointer here, something would be wrong.\n");
            }
        }

        delete mpServerList;
        mpServerList = NULL;
    }
}

bool CCloudServerManager::processCmd(SCMD &cmd)
{
    LOGM_FLOW("cmd.code %d.\n", cmd.code);
    DASSERT(mpWorkQueue);
    ICloudServer *server_info;
    TSocketHandler server_handle;
    TChar char_buffer;

    gfOSReadPipe(mPipeFd[0], char_buffer);
    //LOGM_DEBUG("cmd.code %d, TChar %c.\n", cmd.code, char_buffer);

    switch (cmd.code) {

        case ECMDType_ExitRunning:
            mbRun = 0;
            msState = EServerManagerState_halt;
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        case ECMDType_Start:
            msState = EServerManagerState_noserver_alive;
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        case ECMDType_Stop:
            LOGM_INFO("CCloudServerManager: ECMDType_Stop, TODO\n");
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        case ECMDType_AddClient:
            DASSERT(cmd.pExtra);
            server_info = (ICloudServer *)cmd.pExtra;
            if (DLikely(server_info)) {
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
            server_info = (ICloudServer *)cmd.pExtra;
            if (DLikely(server_info)) {
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

        case ECMDType_AddContent: {
                _SAddUploadingContent *padd = (_SAddUploadingContent *)cmd.pExtra;
                if (DLikely(padd)) {
                    server_info = padd->server_info;
                    if (DLikely(server_info)) {
                        DASSERT(padd->content);
                        padd->server_info->AddCloudContent(padd->content);
                        LOGM_INFO("ECMDType_AddContent, content index %d\n", padd->content->content_index);
                        mpWorkQueue->CmdAck(EECode_OK);
                    } else {
                        LOGM_FATAL("NULL server_info\n");
                        mpWorkQueue->CmdAck(EECode_BadParam);
                    }
                } else {
                    LOGM_FATAL("NULL padd\n");
                    mpWorkQueue->CmdAck(EECode_BadParam);
                }
            }
            break;

        case ECMDType_RemoveClientSession: {
                _SRemoveUploadingClient *premove = (_SRemoveUploadingClient *)cmd.pExtra;
                if (DLikely(premove)) {
                    server_info = premove->server_info;
                    if (DLikely(server_info)) {
                        DASSERT(premove->p_filter);
                        premove->server_info->RemoveCloudClient(premove->p_filter);
                        LOGM_INFO("RemoveCloudClient\n");
                        mpWorkQueue->CmdAck(EECode_OK);
                    } else {
                        LOGM_FATAL("NULL server_info\n");
                        mpWorkQueue->CmdAck(EECode_BadParam);
                    }
                } else {
                    LOGM_FATAL("NULL premove\n");
                    mpWorkQueue->CmdAck(EECode_BadParam);
                }
            }
            break;

        default:
            LOGM_ERROR("wrong cmd.code: %d", cmd.code);
            break;
    }

    LOGM_FLOW("****CCloudServerManager::ProcessCmd, cmd.code %d end.\n", cmd.code);
    return false;
}

void CCloudServerManager::PrintStatus()
{
    if (!(mpPersistMediaConfig->runtime_print_mask & DLOG_MASK_TO_REMOTE)) {
        LOGM_PRINTF("heart beat %d, msState %d\n", mDebugHeartBeat, msState);
    }
    mDebugHeartBeat = 0;
}

void CCloudServerManager::OnRun()
{
    SCMD cmd;
    TUint alive_server_num = 0;
    TInt nready = 0;
    ICloudServer *p_server;

    EServerState server_state;
    TSocketHandler server_hander = 0;

    CIDoubleLinkedList::SNode *pnode;
    mpWorkQueue->CmdAck(EECode_OK);

    //signal(SIGPIPE,SIG_IGN);

    //msState = EServerManagerState_idle;

    while (mbRun) {

        LOGM_STATE("CCloudServerManager::OnRun start switch state %d, %s, mbRun %d.\n", msState, gfGetServerManagerStateString(msState), mbRun);

        switch (msState) {

            case EServerManagerState_idle:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EServerManagerState_noserver_alive:
                pnode = mpServerList->FirstNode();
                while (pnode) {
                    p_server = (ICloudServer *)pnode->p_context;
                    DASSERT(p_server);
                    if (DUnlikely(NULL == p_server)) {
                        LOGM_ERROR("Fatal error, No server? must not get here.\n");
                        break;
                    }

                    p_server->GetHandler(server_hander);
                    p_server->GetServerState(server_state);

                    if (EServerState_running == server_state) {
                        DASSERT(0 < server_hander);
                        if (server_hander >= 0) {
                            FD_SET(server_hander, &mAllSet);
                            mMaxFd = (mMaxFd < server_hander) ? server_hander : mMaxFd;
                            alive_server_num ++;
                        }
                    }

                    pnode = mpServerList->NextNode(pnode);
                }
                if (alive_server_num) {
                    LOGM_INFO("There's some(%d) server alive, transit to running state.\n", alive_server_num);
                    msState = EServerManagerState_running;
                    break;
                }
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EServerManagerState_running:
                DASSERT(alive_server_num > 0);
                mReadSet = mAllSet;

                //LOGM_INFO("[ServerManager]: before select.\n");
                nready = select(mMaxFd + 1, &mReadSet, NULL, NULL, NULL);
                if (DUnlikely(0 > nready)) {
                    LOGM_FATAL("select fail\n");
                    msState = EServerManagerState_error;
                    break;
                } else if (nready == 0) {
                    break;
                }

                LOGM_INFO("[ServerManager]: after select, nready %d.\n", nready);
                //process cmd
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
                    p_server = (ICloudServer *)pnode->p_context;
                    DASSERT(p_server);
                    if (DUnlikely(NULL == p_server)) {
                        LOGM_FATAL("Fatal error(NULL == p_server), must not get here.\n");
                        break;
                    }

                    p_server->GetHandler(server_hander);
                    DASSERT(0 <= server_hander);
                    if (DLikely(0 <= server_hander)) {
                        if (FD_ISSET(server_hander, &mReadSet)) {
                            nready --;
                            //new client's request comes
                            p_server->HandleServerRequest(mMaxFd, (void *)&mAllSet);
                            LOGM_INFO("HandleServerRequest p_server %p, server_hander %d.\n", p_server, server_hander);
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

EECode CCloudServerManager::RunServerManager()
{
    EECode err;
    DASSERT(mpWorkQueue);

    err = mpWorkQueue->Run();
    LOGM_INFO("CCloudServerManager::RunServerManager end, ret %d.\n", err);
    return err;
}

EECode CCloudServerManager::ExitServerManager()
{
    EECode err;
    DASSERT(mpWorkQueue);

    LOGM_INFO("CCloudServerManager::ExitServerManager start.\n");
    gfOSWritePipe(mPipeFd[1], 'b');

    err = mpWorkQueue->Exit();
    LOGM_INFO("CCloudServerManager::ExitServerManager end, ret %d.\n", err);
    return err;
}

EECode CCloudServerManager::Start()
{
    EECode err;
    DASSERT(mpWorkQueue);

    LOGM_INFO("CCloudServerManager::Start start.\n");
    gfOSWritePipe(mPipeFd[1], 'a');
    err = mpWorkQueue->SendCmd(ECMDType_Start, NULL);
    LOGM_INFO("CCloudServerManager::Start end, ret %d.\n", err);
    return err;
}

EECode CCloudServerManager::Stop()
{
    EECode err;
    DASSERT(mpWorkQueue);

    LOGM_INFO("CCloudServerManager::Stop start.\n");
    gfOSWritePipe(mPipeFd[1], 'b');

    err = mpWorkQueue->SendCmd(ECMDType_Stop, NULL);
    LOGM_INFO("CCloudServerManager::Stop end, ret %d.\n", err);
    return err;
}

ICloudServer *CCloudServerManager::AddServer(CloudServerType type, TU16 server_port)
{
    DASSERT(mpWorkQueue);
    ICloudServer *p_server = gfModuleFactoryCloudServer(type, mpPersistMediaConfig, mpMsgSink, server_port);
    if (NULL == p_server) {
        LOGM_ERROR("!!!NOT enough memory? in CCloudServerManager::AddServer.\n");
        return NULL;
    }

    gfOSWritePipe(mPipeFd[1], 'c');

    mpWorkQueue->SendCmd(ECMDType_AddClient, (void *)p_server);

    return p_server;
}

EECode CCloudServerManager::RemoveServer(ICloudServer *server)
{
    DASSERT(mpWorkQueue);
    gfOSWritePipe(mPipeFd[1], 'd');

    mpWorkQueue->SendCmd(ECMDType_RemoveClient, (void *)server);
    return EECode_OK;
}

EECode CCloudServerManager::AddCloudContent(ICloudServer *server_info, SCloudContent *content)
{
    _SAddUploadingContent add;

    add.content = content;
    add.server_info = server_info;

    gfOSWritePipe(mPipeFd[1], 'c');

    return mpWorkQueue->SendCmd(ECMDType_AddContent, (void *)&add);
}

EECode CCloudServerManager::RemoveCloudClient(ICloudServer *server_info, void *filter_owner)
{
    _SRemoveUploadingClient remove;
    remove.server_info = server_info;
    remove.p_filter = filter_owner;

    gfOSWritePipe(mPipeFd[1], 'c');

    return mpWorkQueue->SendCmd(ECMDType_RemoveClientSession, (void *)&remove);
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


