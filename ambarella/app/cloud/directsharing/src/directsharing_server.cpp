/*******************************************************************************
 * directsharing_server.cpp
 *
 * History:
 *    2015/03/08 - [Zhi He] create file
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
#include "common_network_utils.h"

#include "directsharing_if.h"

#include "directsharing_server.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

enum {
    EDirectSharingServerState_idle = 0x00,
    EDirectSharingServerState_running = 0x01,
    EDirectSharingServerState_error = 0x02,
};

typedef struct {
    TSocketHandler socket;
    IMutex *mutex;
    CIDoubleLinkedList *dispatcher_list;
} __s_dsserver_handle_connection;

static TInt __reply_to_client(TSocketHandler socket, EDirectSharingStatusCode status_code, SDirectSharingHeader *header, TInt length)
{
    header->header_flag = DDirectSharingHeaderFlagIsReply | ((TU8)status_code);
    TInt ret = gfNet_Send(socket, (TU8 *)header, length, 0);
    if (DUnlikely(ret != length)) {
        LOG_FATAL("send reply fail, ret %d(should be %d)\n", ret, length);
        return (-1);
    }

    return 0;
}

static IDirectSharingDispatcher *__find_dispatcher(CIDoubleLinkedList *dispatcher_list, TU8 type, TU8 index)
{
    CIDoubleLinkedList::SNode *p_node = dispatcher_list->FirstNode();
    IDirectSharingDispatcher *dispatcher = NULL;
    SSharedResource *resource = NULL;

    while (p_node) {
        dispatcher = (IDirectSharingDispatcher *) p_node->p_context;
        if (DLikely(dispatcher)) {
            dispatcher->QueryResource(resource);
            if ((resource->type == type) && (resource->index == index)) {
                return dispatcher;
            }
        }
        p_node = dispatcher_list->NextNode(p_node);
    }

    return NULL;
}

static EECode __handle_connection(void *param)
{
    __s_dsserver_handle_connection *context = (__s_dsserver_handle_connection *) param;
    TSocketHandler socket = context->socket;
    CIDoubleLinkedList::SNode *p_node = NULL; //context->dispatcher->FirstNode();
    TU8 *mpReadBuffer = (TU8 *) DDBG_MALLOC(2048, "DSRB");
    SSharedResource *resource = NULL;
    SDirectSharingHeader *p_header = (SDirectSharingHeader *) mpReadBuffer;
    IDirectSharingDispatcher *dispatcher = NULL;

    DASSERT(0 < socket);
    DASSERT(mpReadBuffer);

    TInt bytesRead = gfNet_Recv_timeout(socket, (TU8 *)p_header, sizeof(SDirectSharingHeader), DNETUTILS_RECEIVE_FLAG_READ_ALL, 3);
    if (DUnlikely(((TInt)sizeof(SDirectSharingHeader)) != bytesRead)) {
        LOG_ERROR("read header fail, recv %d not expected, expect %lu\n", bytesRead, (TULong)sizeof(SDirectSharingHeader));
        __reply_to_client(socket, EDirectSharingStatusCode_CorruptedProtocol, p_header, sizeof(SDirectSharingHeader));
        DDBG_FREE(mpReadBuffer, "DSRB");
        return EECode_ServerReject_CorruptedProtocol;
    }

    if (EDirectSharingHeaderType_Connect != p_header->header_type) {
        LOG_ERROR("first packet is not connect? header_type %02x\n", p_header->header_type);
        __reply_to_client(socket, EDirectSharingStatusCode_CorruptedProtocol, p_header, sizeof(SDirectSharingHeader));
        DDBG_FREE(mpReadBuffer, "DSRB");
        return EECode_ServerReject_CorruptedProtocol;
    }

    if (0 > __reply_to_client(socket, EDirectSharingStatusCode_OK, p_header, sizeof(SDirectSharingHeader))) {
        LOG_ERROR("send reply fail\n");
        DDBG_FREE(mpReadBuffer, "DSRB");
        return EECode_NetSendHeader_Fail;
    }

    bytesRead = gfNet_Recv_timeout(socket, (TU8 *)p_header, sizeof(SDirectSharingHeader), DNETUTILS_RECEIVE_FLAG_READ_ALL, 3);
    if (DUnlikely(((TInt)sizeof(SDirectSharingHeader)) != bytesRead)) {
        LOG_ERROR("read header fail, recv %d not expected, expect %lu\n", bytesRead, (TULong)sizeof(SDirectSharingHeader));
        __reply_to_client(socket, EDirectSharingStatusCode_CorruptedProtocol, p_header, sizeof(SDirectSharingHeader));
        DDBG_FREE(mpReadBuffer, "DSRB");
        return EECode_ServerReject_CorruptedProtocol;
    }

    if (EDirectSharingHeaderType_QueryResource == p_header->header_type) {
        TInt write_len = 0;
        TU32 payload_length = 0;
        TChar *payload = (TChar *) mpReadBuffer + sizeof(SDirectSharingHeader);
        TInt resource_number = 0;
        p_node = context->dispatcher_list->FirstNode();
        while (p_node) {
            dispatcher = (IDirectSharingDispatcher *) p_node->p_context;
            if (DLikely(dispatcher)) {
                dispatcher->QueryResource(resource);
                if (resource->type == ESharedResourceType_File) {
                    write_len = snprintf(payload, DMAX_DIRECT_SHARING_STRING_SIZE, DDirectSharingFileDescripsionFormat, resource->index, resource->property.file.filesize, resource->description);
                    resource_number ++;
                } else if (resource->type == ESharedResourceType_LiveStreamVideo) {
                    write_len = snprintf(payload, DMAX_DIRECT_SHARING_STRING_SIZE, DDirectSharingLiveStreamVideoDescripsionFormat, resource->index, resource->property.video.format, \
                                         resource->property.video.width, resource->property.video.height, resource->property.video.framerate_num, resource->property.video.framerate_den, \
                                         resource->property.video.bitrate, resource->property.video.m, resource->property.video.n);
                    resource_number ++;
                } else if (resource->type == ESharedResourceType_LiveStreamAudio) {
                    write_len = snprintf(payload, DMAX_DIRECT_SHARING_STRING_SIZE, DDirectSharingLiveStreamAudioDescripsionFormat, resource->index, resource->property.audio.format, \
                                         resource->property.audio.channel_number, resource->property.audio.samplerate, resource->property.audio.framesize, resource->property.audio.bitrate);
                    resource_number ++;
                } else {
                    write_len = 0;
                    LOG_ERROR("bad resource type %d\n", resource->type);
                }
                payload_length += write_len;
                payload += write_len;
            }
            p_node = context->dispatcher_list->NextNode(p_node);
        }

        if (resource_number) {
            p_header->payload_length_0 = (payload_length >> 16) & 0xff;
            p_header->payload_length_1 = (payload_length >> 8) & 0xff;
            p_header->payload_length_2 = payload_length & 0xff;
            LOG_NOTICE("shared resource number %d\n", resource_number);
            if (0 == __reply_to_client(socket, EDirectSharingStatusCode_OK, p_header, (TInt)payload_length)) {
                DDBG_FREE(mpReadBuffer, "DSRB");
                return EECode_OK;
            } else {
                LOG_ERROR("send reply fail\n");
                DDBG_FREE(mpReadBuffer, "DSRB");
                return EECode_NetSendHeader_Fail;
            }

            bytesRead = gfNet_Recv_timeout(socket, (TU8 *)p_header, sizeof(SDirectSharingHeader), DNETUTILS_RECEIVE_FLAG_READ_ALL, 3);
            if (DUnlikely(((TInt)sizeof(SDirectSharingHeader)) != bytesRead)) {
                LOG_ERROR("read header fail, recv %d not expected, expect %lu\n", bytesRead, (TULong)sizeof(SDirectSharingHeader));
                __reply_to_client(socket, EDirectSharingStatusCode_CorruptedProtocol, p_header, sizeof(SDirectSharingHeader));
                DDBG_FREE(mpReadBuffer, "DSRB");
                return EECode_ServerReject_CorruptedProtocol;
            }

            if (EDirectSharingHeaderType_RequestResource != p_header->header_type) {
                LOG_ERROR("first packet is not request resource? header_type %02x\n", p_header->header_type);
                __reply_to_client(socket, EDirectSharingStatusCode_CorruptedProtocol, p_header, sizeof(SDirectSharingHeader));
                DDBG_FREE(mpReadBuffer, "DSRB");
                return EECode_ServerReject_CorruptedProtocol;
            }

            TU8 resource_type = p_header->val0;
            TU8 resource_index = p_header->val1;

            __LOCK(context->mutex);

            dispatcher = __find_dispatcher(context->dispatcher_list, resource_type, resource_index);
            if (dispatcher) {
                __reply_to_client(socket, EDirectSharingStatusCode_OK, p_header, sizeof(SDirectSharingHeader));
                IDirectSharingSender *sender = gfCreateDSSender(socket, resource_type);
                dispatcher->AddSender(sender);
                __UNLOCK(context->mutex);
            } else {
                __UNLOCK(context->mutex);
                LOG_ERROR("no such resource: type %d, index %d\n", resource_type, resource_index);
                __reply_to_client(socket, EDirectSharingStatusCode_NoSuchResource, p_header, sizeof(SDirectSharingHeader));
                DDBG_FREE(mpReadBuffer, "DSRB");
                return EECode_BadParam;
            }
        } else {
            p_header->payload_length_0 = 0;
            p_header->payload_length_1 = 0;
            p_header->payload_length_2 = 0;
            LOG_WARN("no avaiable resource?\n");
            if (0 == __reply_to_client(socket, EDirectSharingStatusCode_NoAvailableResource, p_header, sizeof(SDirectSharingHeader))) {
                DDBG_FREE(mpReadBuffer, "DSRB");
                return EECode_BadParam;
            } else {
                LOG_ERROR("send reply fail\n");
                DDBG_FREE(mpReadBuffer, "DSRB");
                return EECode_NetSendHeader_Fail;
            }
        }
    } else if (EDirectSharingHeaderType_RequestResource == p_header->header_type) {
        TU8 resource_type = p_header->val0;
        TU8 resource_index = p_header->val1;
        __LOCK(context->mutex);
        dispatcher = __find_dispatcher(context->dispatcher_list, resource_type, resource_index);
        if (dispatcher) {
            __reply_to_client(socket, EDirectSharingStatusCode_OK, p_header, sizeof(SDirectSharingHeader));
            IDirectSharingSender *sender = gfCreateDSSender(socket, resource_type);
            dispatcher->AddSender(sender);
            __UNLOCK(context->mutex);
        } else {
            __UNLOCK(context->mutex);
            LOG_ERROR("no such resource: type %d, index %d\n", resource_type, resource_index);
            __reply_to_client(socket, EDirectSharingStatusCode_NoSuchResource, p_header, sizeof(SDirectSharingHeader));
            DDBG_FREE(mpReadBuffer, "DSRB");
            return EECode_BadParam;
        }
    } else if (EDirectSharingHeaderType_ShareResource == p_header->header_type) {
        //to do
        LOG_ERROR("TO DO\n");
        if (0 == __reply_to_client(socket, EDirectSharingStatusCode_NotSuppopted, p_header, sizeof(SDirectSharingHeader))) {
            DDBG_FREE(mpReadBuffer, "DSRB");
            return EECode_NotSupported;
        } else {
            LOG_ERROR("send reply fail\n");
            DDBG_FREE(mpReadBuffer, "DSRB");
            return EECode_NetSendHeader_Fail;
        }
        return EECode_NotSupported;
    }

    DDBG_FREE(mpReadBuffer, "DSRB");
    return EECode_OK;
}

IDirectSharingServer *gfCreateDirectSharingServer(TSocketPort port, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
{
    return (IDirectSharingServer *)CIDirectSharingServer::Create(port, pconfig, pMsgSink, p_system_clock_reference);
}

CIDirectSharingServer::CIDirectSharingServer(TSocketPort port, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
    : inherited("CIDirectSharingServer", 0)
    , mpPersistCommonConfig(pconfig)
    , mpMsgSink(pMsgSink)
    , mpSystemClockReference(p_system_clock_reference)
    , mpWorkQueue(NULL)
    , mpMutex(NULL)
    , mbRun(1)
    , msState(EDirectSharingServerState_idle)
    , mbStarted(0)
    , mServerPort(port)
    , mSocket(DInvalidSocketHandler)
    , mpReadBuffer(NULL)
    , mnMaxPayloadLength(0)
    , mMaxFd(-1)
    , mpDispatcherList(NULL)
{
    mPipeFd[0] = mPipeFd[1] = -1;
}

CIDirectSharingServer::~CIDirectSharingServer()
{
}

CIDirectSharingServer *CIDirectSharingServer::Create(TSocketPort port, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
{
    CIDirectSharingServer *result = new CIDirectSharingServer(port, pconfig, pMsgSink, p_system_clock_reference);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CObject *CIDirectSharingServer::GetObject() const
{
    return (CObject *) this;
}

EECode CIDirectSharingServer::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleDirectSharingServer);
    //mConfigLogLevel = ELogLevel_Debug;
    //mConfigLogOption = 0xffffffff;

    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOGM_FATAL("gfCreateMutex() fail.\n");
        return EECode_NoMemory;
    }

    mpReadBuffer = (TU8 *) DDBG_MALLOC(2048, "DSRB");
    if (DUnlikely(!mpReadBuffer)) {
        LOGM_FATAL("alloc mpReadBuffer fail.\n");
        return EECode_NoMemory;
    }
    mnMaxPayloadLength = 2048;

    mpDispatcherList = new CIDoubleLinkedList();
    if (DUnlikely(!mpDispatcherList)) {
        LOGM_ERROR("new CIDoubleLinkedList() fail.\n");
        return EECode_NoMemory;
    }

    mSocket = gfNet_SetupStreamSocket(INADDR_ANY, mServerPort, 1);
    if (DUnlikely(0 > mSocket)) {
        LOGM_FATAL("setup socket fail, ret %d.\n", mSocket);
        return EECode_Error;
    }
    LOGM_INFO("gfNet_SetupStreamSocket, socket %d, port %d.\n", mSocket, mServerPort);

    mpWorkQueue = CIWorkQueue::Create((IActiveObject *)this);
    if (DUnlikely(!mpWorkQueue)) {
        LOGM_ERROR("Create CIWorkQueue fail.\n");
        return EECode_NoMemory;
    }

    EECode err = gfOSCreatePipe(mPipeFd);
    DASSERT(EECode_OK == err);

    //init
    FD_ZERO(&mAllSet);
    FD_SET(mPipeFd[0], &mAllSet);
    FD_SET(mSocket, &mAllSet);

    if (mSocket > mPipeFd[0]) {
        mMaxFd = mSocket;
    } else {
        mMaxFd = mPipeFd[0];
    }

    //LOGM_DEBUG("before Run().\n");
    DASSERT(mpWorkQueue);
    mpWorkQueue->Run();
    //LOGM_DEBUG("after Run().\n");
    mbStarted = 1;

    return EECode_OK;
}

void CIDirectSharingServer::Destroy()
{
    Delete();
}

void CIDirectSharingServer::Delete()
{
    if (mbStarted) {
        mbStarted = 0;
        EECode err;
        TChar wake_char = 'e';

        DASSERT(mpWorkQueue);
        LOGM_NOTICE("exit() begin.\n");

        gfOSWritePipe(mPipeFd[1], wake_char);
        err = mpWorkQueue->SendCmd(ECMDType_ExitRunning, NULL);
        LOGM_NOTICE("exit() end, ret %d.\n", err);
    }

    if (mpDispatcherList) {
        CIDoubleLinkedList::SNode *pnode;
        IDirectSharingDispatcher *p_dispatcher;
        pnode = mpDispatcherList->FirstNode();
        while (pnode) {
            p_dispatcher = (IDirectSharingDispatcher *)(pnode->p_context);
            pnode = mpDispatcherList->NextNode(pnode);
            if (p_dispatcher) {
                p_dispatcher->Destroy();
            } else {
                LOGM_FATAL("NULL pointer here, something would be wrong.\n");
            }
        }

        delete mpDispatcherList;
        mpDispatcherList = NULL;
    }

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    if (mpWorkQueue) {
        mpWorkQueue->Delete();
        mpWorkQueue = NULL;
    }
}

void CIDirectSharingServer::processCmd(SCMD &cmd)
{
    TChar char_buffer;

    gfOSReadPipe(mPipeFd[0], char_buffer);

    switch (cmd.code) {

        case ECMDType_ExitRunning:
            mbRun = 0;
            msState = EServerManagerState_halt;
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        case ECMDType_Start:
            msState = EDirectSharingServerState_running;
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        case ECMDType_Stop:
            msState = EDirectSharingServerState_idle;
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        case ECMDType_AddClient:
            if (cmd.pExtra) {
                mpDispatcherList->InsertContent(NULL, cmd.pExtra, 0);
                mpWorkQueue->CmdAck(EECode_OK);
            } else {
                LOGM_ERROR("NULL pointer here, must have errors.\n");
                mpWorkQueue->CmdAck(EECode_BadCommand);
            }
            break;

        case ECMDType_RemoveClient:
            if (cmd.pExtra) {
                mpDispatcherList->RemoveContent(cmd.pExtra);
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

    //LOGM_FLOW("****CIDirectSharingServer::ProcessCmd, cmd.code %d end.\n", cmd.code);
    return;
}

void CIDirectSharingServer::PrintStatus()
{
    mDebugHeartBeat = 0;
}

void CIDirectSharingServer::OnRun()
{
    SCMD cmd;
    TInt nready = 0;

    mpWorkQueue->CmdAck(EECode_OK);

    while (mbRun) {

        //LOGM_STATE("CIDirectSharingServer::OnRun start switch state %d, mbRun %d.\n", msState, mbRun);

        switch (msState) {

            case EDirectSharingServerState_idle:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EDirectSharingServerState_running:
                mReadSet = mAllSet;

                nready = select(mMaxFd + 1, &mReadSet, NULL, NULL, NULL);
                if (DUnlikely(0 > nready)) {
                    LOGM_FATAL("select fail\n");
                    msState = EDirectSharingServerState_error;
                    break;
                } else if (nready == 0) {
                    break;
                }

                if (FD_ISSET(mPipeFd[0], &mReadSet)) {
                    mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                    processCmd(cmd);
                    nready --;
                    if (EDirectSharingServerState_running != msState) {
                        LOGM_INFO(" transit from EDirectSharingServerState_running to state %d.\n", msState);
                        break;
                    }

                    if (nready <= 0) {
                        //read done
                        LOGM_INFO(" read done.\n");
                        break;
                    }
                }

                handleServerRequest();
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

EECode CIDirectSharingServer::StartServer()
{
    EECode err;
    TChar wake_char = 'a';

    DASSERT(mpWorkQueue);
    LOGM_INFO("StartServer() begin.\n");

    gfOSWritePipe(mPipeFd[1], wake_char);
    err = mpWorkQueue->SendCmd(ECMDType_Start, NULL);
    LOGM_INFO("StartServer() end, ret %d.\n", err);
    return err;
}

EECode CIDirectSharingServer::StopServer()
{
    EECode err;
    TChar wake_char = 'b';

    DASSERT(mpWorkQueue);
    LOGM_INFO("StopServer() begin.\n");

    gfOSWritePipe(mPipeFd[1], wake_char);
    err = mpWorkQueue->SendCmd(ECMDType_Stop, NULL);
    LOGM_INFO("StopServer() end, ret %d.\n", err);
    return err;
}

EECode CIDirectSharingServer::AddDispatcher(IDirectSharingDispatcher *p_dispatcher)
{
    TChar wake_char = 'c';
    DASSERT(mpWorkQueue);

    gfOSWritePipe(mPipeFd[1], wake_char);

    return mpWorkQueue->SendCmd(ECMDType_AddClient, (void *)p_dispatcher);
}

EECode CIDirectSharingServer::RemoveDispatcher(IDirectSharingDispatcher *p_dispatcher)
{
    DASSERT(mpWorkQueue);
    TChar wake_char = 'd';

    gfOSWritePipe(mPipeFd[1], wake_char);

    return mpWorkQueue->SendCmd(ECMDType_RemoveClient, (void *)p_dispatcher);
}

void CIDirectSharingServer::handleServerRequest()
{
    TSocketHandler socket = 0;
    struct sockaddr_in client_addr;
    TSocketSize clilen = sizeof(struct sockaddr_in);
    EECode err = EECode_OK;

    LOGM_INFO("CIDirectSharingServer::HandleServerRequest start.\n");
    mDebugHeartBeat ++;

do_retry:
    socket = gfNet_Accept(mSocket, (void *)&client_addr, (TSocketSize *)&clilen, err);
    if (DUnlikely(socket < 0)) {
        if (EECode_TryAgain == err) {
            LOGM_WARN("non-blocking call?\n");
            goto do_retry;
        }
        LOGM_ERROR("accept fail, return %d.\n", socket);
        return;
    } else {
        LOGM_PRINTF(" NEW client: %s, port %d, socket %d.\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), socket);
        //gfSocketSetTimeout(socket, 2, 0);
        gfSocketSetSendBufferSize(socket, 512 * 1024);
        IDirectSharingDispatcher *dispatcher = NULL;
        IDirectSharingSender *p_sender = NULL;
        IDirectSharingReceiver *p_receiver = NULL;

        if (0) {
            __s_dsserver_handle_connection *context = (__s_dsserver_handle_connection *) malloc(sizeof(__s_dsserver_handle_connection));
            context->dispatcher_list = mpDispatcherList;
            context->mutex = mpMutex;
            context->socket = socket;
            gfCreateThread("handshake_ds", __handle_connection, (void *) context);
        } else {
            EDirectSharingStatusCode ret_code = handshake(socket, dispatcher, p_sender, p_receiver);
            if (EDirectSharingStatusCode_OK != ret_code) {
                gfNetCloseSocket(socket);
                socket = DInvalidSocketHandler;
                LOGM_WARN("handshake fail\n");
                return;
            }
        }
    }

    return;
}

EDirectSharingStatusCode CIDirectSharingServer::handshake(TSocketHandler socket, IDirectSharingDispatcher *&dispatcher, IDirectSharingSender *&p_sender, IDirectSharingReceiver *&p_receiver)
{
    CIDoubleLinkedList::SNode *p_node = mpDispatcherList->FirstNode();
    SSharedResource *resource = NULL;
    SDirectSharingHeader *p_header = (SDirectSharingHeader *) mpReadBuffer;

    DASSERT(0 < socket);
    TInt bytesRead = gfNet_Recv_timeout(socket, (TU8 *)p_header, sizeof(SDirectSharingHeader), DNETUTILS_RECEIVE_FLAG_READ_ALL, 3);
    if (DUnlikely(((TInt)sizeof(SDirectSharingHeader)) != bytesRead)) {
        LOGM_ERROR("read header fail, recv %d not expected, expect %lu\n", bytesRead, (TULong)sizeof(SDirectSharingHeader));
        __reply_to_client(socket, EDirectSharingStatusCode_CorruptedProtocol, p_header, sizeof(SDirectSharingHeader));
        return EDirectSharingStatusCode_CorruptedProtocol;
    }

    if (EDirectSharingHeaderType_Connect != p_header->header_type) {
        LOGM_ERROR("first packet is not connect? header_type %02x\n", p_header->header_type);
        __reply_to_client(socket, EDirectSharingStatusCode_CorruptedProtocol, p_header, sizeof(SDirectSharingHeader));
        return EDirectSharingStatusCode_CorruptedProtocol;
    }

    if (0 > __reply_to_client(socket, EDirectSharingStatusCode_OK, p_header, sizeof(SDirectSharingHeader))) {
        LOGM_ERROR("send reply fail\n");
        return EDirectSharingStatusCode_SendReplyFail;
    }

    bytesRead = gfNet_Recv_timeout(socket, (TU8 *)p_header, sizeof(SDirectSharingHeader), DNETUTILS_RECEIVE_FLAG_READ_ALL, 3);
    if (DUnlikely(((TInt)sizeof(SDirectSharingHeader)) != bytesRead)) {
        LOGM_ERROR("read header fail, recv %d not expected, expect %lu\n", bytesRead, (TULong)sizeof(SDirectSharingHeader));
        __reply_to_client(socket, EDirectSharingStatusCode_CorruptedProtocol, p_header, sizeof(SDirectSharingHeader));
        return EDirectSharingStatusCode_CorruptedProtocol;
    }

    if (EDirectSharingHeaderType_QueryResource == p_header->header_type) {
        TInt write_len = 0;
        TU32 payload_length = 0;
        TChar *payload = (TChar *) mpReadBuffer + sizeof(SDirectSharingHeader);
        TInt resource_number = 0;
        while (p_node) {
            dispatcher = (IDirectSharingDispatcher *) p_node->p_context;
            if (DLikely(dispatcher)) {
                dispatcher->QueryResource(resource);
                if (resource->type == ESharedResourceType_File) {
                    write_len = snprintf(payload, DMAX_DIRECT_SHARING_STRING_SIZE, DDirectSharingFileDescripsionFormat, resource->index, resource->property.file.filesize, resource->description);
                    resource_number ++;
                } else if (resource->type == ESharedResourceType_LiveStreamVideo) {
                    write_len = snprintf(payload, DMAX_DIRECT_SHARING_STRING_SIZE, DDirectSharingLiveStreamVideoDescripsionFormat, resource->index, resource->property.video.format, \
                                         resource->property.video.width, resource->property.video.height, resource->property.video.framerate_num, resource->property.video.framerate_den, \
                                         resource->property.video.bitrate, resource->property.video.m, resource->property.video.n);
                    resource_number ++;
                } else if (resource->type == ESharedResourceType_LiveStreamAudio) {
                    write_len = snprintf(payload, DMAX_DIRECT_SHARING_STRING_SIZE, DDirectSharingLiveStreamAudioDescripsionFormat, resource->index, resource->property.audio.format, \
                                         resource->property.audio.channel_number, resource->property.audio.samplerate, resource->property.audio.framesize, resource->property.audio.bitrate);
                    resource_number ++;
                } else {
                    write_len = 0;
                    LOGM_ERROR("bad resource type %d\n", resource->type);
                }
                payload_length += write_len;
                payload += write_len;
            }
            p_node = mpDispatcherList->NextNode(p_node);
        }

        if (resource_number) {
            p_header->payload_length_0 = (payload_length >> 16) & 0xff;
            p_header->payload_length_1 = (payload_length >> 8) & 0xff;
            p_header->payload_length_2 = payload_length & 0xff;
            LOG_NOTICE("shared resource number %d\n", resource_number);
            if (0 == __reply_to_client(socket, EDirectSharingStatusCode_OK, p_header, (TInt)payload_length)) {
                return EDirectSharingStatusCode_OK;
            } else {
                LOG_ERROR("send reply fail\n");
                return EDirectSharingStatusCode_SendReplyFail;
            }

            bytesRead = gfNet_Recv_timeout(socket, (TU8 *)p_header, sizeof(SDirectSharingHeader), DNETUTILS_RECEIVE_FLAG_READ_ALL, 3);
            if (DUnlikely(((TInt)sizeof(SDirectSharingHeader)) != bytesRead)) {
                LOG_ERROR("read header fail, recv %d not expected, expect %lu\n", bytesRead, (TULong)sizeof(SDirectSharingHeader));
                __reply_to_client(socket, EDirectSharingStatusCode_CorruptedProtocol, p_header, sizeof(SDirectSharingHeader));
                return EDirectSharingStatusCode_CorruptedProtocol;
            }

            if (EDirectSharingHeaderType_RequestResource != p_header->header_type) {
                LOG_ERROR("first packet is not request resource? header_type %02x\n", p_header->header_type);
                __reply_to_client(socket, EDirectSharingStatusCode_CorruptedProtocol, p_header, sizeof(SDirectSharingHeader));
                return EDirectSharingStatusCode_CorruptedProtocol;
            }

            TU8 resource_type = p_header->val0;
            TU8 resource_index = p_header->val1;

            dispatcher = __find_dispatcher(mpDispatcherList, resource_type, resource_index);
            if (dispatcher) {
                __reply_to_client(socket, EDirectSharingStatusCode_OK, p_header, sizeof(SDirectSharingHeader));
                IDirectSharingSender *sender = gfCreateDSSender(socket, resource_type);
                dispatcher->AddSender(sender);
            } else {
                LOG_ERROR("no such resource: type %d, index %d\n", resource_type, resource_index);
                __reply_to_client(socket, EDirectSharingStatusCode_NoSuchResource, p_header, sizeof(SDirectSharingHeader));
                return EDirectSharingStatusCode_NoSuchResource;
            }
        } else {
            p_header->payload_length_0 = 0;
            p_header->payload_length_1 = 0;
            p_header->payload_length_2 = 0;
            LOG_WARN("no avaiable resource?\n");
            if (0 == __reply_to_client(socket, EDirectSharingStatusCode_NoAvailableResource, p_header, sizeof(SDirectSharingHeader))) {
                return EDirectSharingStatusCode_NoAvailableResource;
            } else {
                LOG_ERROR("send reply fail\n");
                return EDirectSharingStatusCode_SendReplyFail;
            }
        }
    } else if (EDirectSharingHeaderType_RequestResource == p_header->header_type) {
        TU8 resource_type = p_header->val0;
        TU8 resource_index = p_header->val1;

        dispatcher = __find_dispatcher(mpDispatcherList, resource_type, resource_index);
        if (dispatcher) {
            __reply_to_client(socket, EDirectSharingStatusCode_OK, p_header, sizeof(SDirectSharingHeader));
            IDirectSharingSender *sender = gfCreateDSSender(socket, resource_type);
            dispatcher->AddSender(sender);
        } else {
            LOG_ERROR("no such resource: type %d, index %d\n", resource_type, resource_index);
            __reply_to_client(socket, EDirectSharingStatusCode_NoSuchResource, p_header, sizeof(SDirectSharingHeader));
            return EDirectSharingStatusCode_NoSuchResource;
        }
    } else if (EDirectSharingHeaderType_ShareResource == p_header->header_type) {
        //to do
        LOG_ERROR("TO DO\n");
        if (0 == __reply_to_client(socket, EDirectSharingStatusCode_NotSuppopted, p_header, sizeof(SDirectSharingHeader))) {
            return EDirectSharingStatusCode_NotSuppopted;
        } else {
            LOG_ERROR("send reply fail\n");
            return EDirectSharingStatusCode_SendReplyFail;
        }
        return EDirectSharingStatusCode_NotSuppopted;
    }

    return EDirectSharingStatusCode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


