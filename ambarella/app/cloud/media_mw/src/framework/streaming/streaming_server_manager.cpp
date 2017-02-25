/*******************************************************************************
 * streaming_server_manager.cpp
 *
 * History:
 *    2012/11/15 - [Zhi He] create file
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
#include "streaming_server_manager.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

typedef struct {
    IStreamingServer *server_info;
    SStreamingSessionContent *content;
} _SAddStreamingContent;

typedef struct {
    IStreamingServer *server_info;
    SClientSessionInfo *session;
} _SRemoveStreamingSession;

IStreamingServerManager *gfCreateStreamingServerManager(const volatile SPersistMediaConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
{
    return (IStreamingServerManager *)CStreamingServerManager::Create(pconfig, pMsgSink, p_system_clock_reference);
}

CStreamingServerManager::CStreamingServerManager(const volatile SPersistMediaConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
    : inherited("CStreamingServerManager", 0)
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

CStreamingServerManager::~CStreamingServerManager()
{
}

IStreamingServerManager *CStreamingServerManager::Create(const volatile SPersistMediaConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
{
    CStreamingServerManager *result = new CStreamingServerManager(pconfig, pMsgSink, p_system_clock_reference);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CObject *CStreamingServerManager::GetObject() const
{
    return (CObject *) this;
}

EECode CStreamingServerManager::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleStreamingServerManager);
    //mConfigLogLevel = ELogLevel_Debug;
    //mConfigLogOption = 0xffffffff;

    mpServerList = new CIDoubleLinkedList();
    if (DUnlikely(!mpServerList)) {
        LOGM_ERROR("new CIDoubleLinkedList() fail.\n");
        return EECode_NoMemory;
    }

    if ((mpWorkQueue = CIWorkQueue::Create((IActiveObject *)this)) == NULL) {
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

void CStreamingServerManager::PrintStatus0()
{
    PrintStatus();
}

void CStreamingServerManager::Destroy()
{
    Delete();
}

void CStreamingServerManager::Delete()
{
    if (mpServerList) {
        CIDoubleLinkedList::SNode *pnode;

        //clean servers
        IStreamingServer *p_server;
        pnode = mpServerList->FirstNode();
        while (pnode) {
            p_server = (IStreamingServer *)(pnode->p_context);
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

bool CStreamingServerManager::processCmd(SCMD &cmd)
{
    LOGM_FLOW("cmd.code %d.\n", cmd.code);
    DASSERT(mpWorkQueue);
    IStreamingServer *server_info;
    TInt server_handle;
    TChar char_buffer;

    gfOSReadPipe(mPipeFd[0], char_buffer);
    //LOGM_DEBUG("cmd.code %d, char %c.\n", cmd.code, char_buffer);

    switch (cmd.code) {

        case ECMDType_ExitRunning:
            mbRun = 0;
            msState = EServerManagerState_halt;
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        case ECMDType_Start:
            LOGM_INFO("CStreamingServerManager: ECMDType_Start, TODO\n");
            msState = EServerManagerState_noserver_alive;
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        case ECMDType_Stop:
            LOGM_INFO("CStreamingServerManager: ECMDType_Stop, TODO\n");
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        case ECMDType_AddClient:
            DASSERT(cmd.pExtra);
            server_info = (IStreamingServer *)cmd.pExtra;
            if (server_info) {
                LOGM_INFO("add streaming server.\n");

                //need add socket into set
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

            if (DUnlikely(EServerManagerState_error == msState)) {
                LOGM_WARN("[error recover]: from msState %d to EServerManagerState_running\n", msState);
                msState = EServerManagerState_running;
            }
            break;

        case ECMDType_RemoveClient:
            DASSERT(cmd.pExtra);
            server_info = (IStreamingServer *)cmd.pExtra;
            if (server_info) {
                LOGM_INFO("remove streaming server\n");

                //need add socket into set
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

            if (DUnlikely(EServerManagerState_error == msState)) {
                LOGM_WARN("[error recover]: from msState %d to EServerManagerState_running\n", msState);
                msState = EServerManagerState_running;
            }
            break;

        case ECMDType_AddContent: {
                _SAddStreamingContent *padd = (_SAddStreamingContent *)cmd.pExtra;
                if (DLikely(padd)) {
                    server_info = padd->server_info;
                    if (DLikely(server_info)) {
                        DASSERT(padd->content);
                        padd->server_info->AddStreamingContent(padd->content);
                        LOGM_INFO("add streaming content, index %d\n", padd->content->content_index);
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

            if (DUnlikely(EServerManagerState_error == msState)) {
                LOGM_WARN("[error recover]: from msState %d to EServerManagerState_running\n", msState);
                msState = EServerManagerState_running;
            }
            break;

        case ECMDType_RemoveClientSession: {
                _SRemoveStreamingSession *remove = (_SRemoveStreamingSession *)cmd.pExtra;
                if (DLikely(remove->server_info && remove->session)) {
                    if (DLikely(0 < remove->session->tcp_socket)) {
                        FD_CLR(remove->session->tcp_socket, &mAllSet);
                    } else {
                        LOGM_ERROR("BAD remove->session->tcp_socket %d\n", remove->session->tcp_socket);
                    }

                    LOGM_DEBUG("remove client session name %s\n", remove->session->p_session_name);
                    remove->server_info->RemoveStreamingClientSession(remove->session);
                    mpWorkQueue->CmdAck(EECode_OK);
                } else {
                    LOGM_FATAL("NULL server or session\n");
                    mpWorkQueue->CmdAck(EECode_BadParam);
                }
            }

            if (DUnlikely(EServerManagerState_error == msState)) {
                LOGM_WARN("[error recover]: from msState %d to EServerManagerState_running\n", msState);
                msState = EServerManagerState_running;
            }

            break;

        default:
            LOGM_ERROR("wrong cmd.code: %d", cmd.code);
            break;
    }

    LOGM_FLOW("cmd.code %d end.\n", cmd.code);
    return false;
}

EECode CStreamingServerManager::RemoveSession(IStreamingServer *server_info, void *session)
{
    SCMD cmd;
    memset(&cmd, 0x0, sizeof(SCMD));

    _SRemoveStreamingSession remove;

    remove.server_info = server_info;
    remove.session = (SClientSessionInfo *)session;
    cmd.code = ECMDType_RemoveClientSession;
    cmd.needFreePExtra = 0;
    cmd.pExtra = &remove;

    gfOSWritePipe(mPipeFd[1], 'c');

    mpWorkQueue->SendCmd(cmd);

    return EECode_OK;
}

void CStreamingServerManager::PrintStatus()
{
    if (!(mpPersistMediaConfig->runtime_print_mask & DLOG_MASK_TO_REMOTE)) {
        LOGM_PRINTF("heart beat %d, msState %d\n", mDebugHeartBeat, msState);
    }
    mDebugHeartBeat = 0;
}

void CStreamingServerManager::OnRun()
{
    SCMD cmd;
    TUint alive_server_num = 0;
    TInt nready = 0;
    IStreamingServer *p_server;

    EServerState server_state;
    TInt server_hander = 0;

    CIDoubleLinkedList::SNode *pnode;
    mpWorkQueue->CmdAck(EECode_OK);

    //signal(SIGPIPE,SIG_IGN);

    //msState = EServerManagerState_idle;

    while (mbRun) {

        LOGM_STATE("start switch state %d, %s, mbRun %d.\n", msState, gfGetServerManagerStateString(msState), mbRun);

        switch (msState) {

            case EServerManagerState_idle:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                //LOGM_ERROR("debug 1, mbRun %d\n", mbRun);
                break;

            case EServerManagerState_noserver_alive:
                DASSERT(!alive_server_num);
                //start server needed running, todo
                pnode = mpServerList->FirstNode();
                while (pnode) {
                    p_server = (IStreamingServer *)pnode->p_context;
                    DASSERT(p_server);
                    if (NULL == p_server) {
                        LOGM_ERROR("Fatal error, No server? must not get here.\n");
                        break;
                    }

                    //add to FD set
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

                //LOGM_VERBOSE("[ServerManager]: before select.\n");
                nready = select(mMaxFd + 1, &mReadSet, NULL, NULL, NULL);
                if (0 > nready) {
                    LOGM_FATAL("select fail, %d\n", nready);
                    msState = EServerManagerState_error;
                    break;
                } else if (nready == 0) {
                    break;
                }

                LOGM_VERBOSE("after select, nready %d.\n", nready);
                //process cmd
                if (FD_ISSET(mPipeFd[0], &mReadSet)) {
                    //LOGM_DEBUG("[ServerManager]: from pipe fd, nready %d, mpWorkQueue->MsgQ() %d.\n", nready, mpWorkQueue->MsgQ()->GetDataCnt());
                    //some input from engine, process cmd first
                    //while (mpWorkQueue->MsgQ()->PeekMsg(&cmd,sizeof(cmd))) {
                    mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                    processCmd(cmd);
                    //}
                    nready --;
                    if (EServerManagerState_running != msState) {
                        LOGM_INFO(" transit from EServerManagerState_running to state %d.\n", msState);
                        break;
                    }

                    if (nready <= 0) {
                        //read done
                        LOGM_VERBOSE(" read done.\n");
                        break;
                    }
                }

                //is request to server?
                pnode = mpServerList->FirstNode();

                while (pnode) {
                    p_server = (IStreamingServer *)pnode->p_context;
                    DASSERT(p_server);
                    if (NULL == p_server) {
                        LOGM_FATAL("Fatal error(NULL == p_server), must not get here.\n");
                        break;
                    }

                    p_server->GetHandler(server_hander);
                    DASSERT(0 <= server_hander);
                    if (0 <= server_hander) {
                        if (FD_ISSET(server_hander, &mReadSet)) {
                            nready --;
                            //new client's request comes
                            p_server->HandleServerRequest(mMaxFd, (void *)&mAllSet);
                            //LOGM_VERBOSE("HandleServerRequest p_server %p, server_hander %d.\n", p_server, server_hander);
                        } else {
                            //search in current client list
                            p_server->ScanClientList(nready, &mReadSet, &mAllSet);
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

EECode CStreamingServerManager::RunServerManager()
{
    LOGM_INFO("before CStreamingServerManager:: mpWorkQueue->Run().\n");
    DASSERT(mpWorkQueue);
    EECode err = mpWorkQueue->Run();
    LOGM_INFO("after CStreamingServerManager:: mpWorkQueue->Run().\n");
    return err;
}

EECode CStreamingServerManager::ExitServerManager()
{
    DASSERT(mpWorkQueue);

    LOGM_INFO("CStreamingServerManager::ExitServerManager().\n");
    gfOSWritePipe(mPipeFd[1], 'b');

    EECode err = mpWorkQueue->Exit();
    DASSERT_OK(err);
    LOGM_INFO("CStreamingServerManager::ExitServerManager() end, ret %d.\n", err);
    return err;
}

EECode CStreamingServerManager::Start()
{
    DASSERT(mpWorkQueue);

    LOGM_INFO("CStreamingServerManager::Start().\n");
    gfOSWritePipe(mPipeFd[1], 'c');

    EECode err = mpWorkQueue->SendCmd(ECMDType_Start, NULL);
    DASSERT_OK(err);
    LOGM_INFO("CStreamingServerManager::Start() end, ret %d.\n", err);
    return err;
}

EECode CStreamingServerManager::Stop()
{
    DASSERT(mpWorkQueue);

    LOGM_INFO("CStreamingServerManager::Stop().\n");
    gfOSWritePipe(mPipeFd[1], 'd');

    EECode err = mpWorkQueue->SendCmd(ECMDType_Stop, NULL);
    DASSERT_OK(err);
    LOGM_INFO("CStreamingServerManager::Stop() end, ret %d.\n", err);
    return err;
}

IStreamingServer *CStreamingServerManager::AddServer(StreamingServerType type, StreamingServerMode mode, TU16 server_port, TU8 enable_video, TU8 enable_audio)
{
    DASSERT(mpWorkQueue);
    IStreamingServer *p_server = gfModuleFactoryStreamingServer(type, mode, mpPersistMediaConfig, mpMsgSink, server_port, enable_video, enable_audio);
    if (NULL == p_server) {
        LOGM_ERROR("!!!NOT enough memory? in CStreamingServerManager::AddServer.\n");
        return NULL;
    }

    gfOSWritePipe(mPipeFd[1], 'c');

    mpWorkQueue->SendCmd(ECMDType_AddClient, (void *)p_server);

    return p_server;
}

EECode CStreamingServerManager::CloseServer(IStreamingServer *server)
{
    LOGM_FATAL("TO DO\n");
    return EECode_OK;
}

EECode CStreamingServerManager::RemoveServer(IStreamingServer *server)
{
    DASSERT(mpWorkQueue);

    gfOSWritePipe(mPipeFd[1], 'd');

    mpWorkQueue->SendCmd(ECMDType_RemoveClient, (void *)server);
    return EECode_OK;
}

EECode CStreamingServerManager::AddStreamingContent(IStreamingServer *server_info, SStreamingSessionContent *content)
{
    _SAddStreamingContent add;

    add.content = content;
    add.server_info = server_info;

    gfOSWritePipe(mPipeFd[1], 'c');

    return mpWorkQueue->SendCmd(ECMDType_AddContent, (void *)&add);
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


