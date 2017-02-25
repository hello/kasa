/*******************************************************************************
 * sacp_cloud_server.cpp
 *
 * History:
 *    2013/11/15 - [Zhi He] create file
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
#include "common_network_utils.h"

#include "cloud_lib_if.h"

#include "security_utils_if.h"

#include "media_mw_if.h"
#include "media_mw_utils.h"
#include "framework_interface.h"
#include "mw_internal.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "sacp_types.h"
#include "sacp_cloud_server.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

ICloudServer *gfCreateSACPCloudServer(const TChar *name, CloudServerType type, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU16 server_port)
{
    return (ICloudServer *)CSACPCouldServer::Create(name, type, pPersistMediaConfig, pMsgSink, server_port);
}

EECode CSACPCouldServer::HandleServerRequest(TSocketHandler &max, void *all_set)
{
    TSocketHandler socket = 0;
    struct sockaddr_in client_addr;
    TSocketSize clilen = sizeof(struct sockaddr_in);
    EECode err = EECode_OK;

    LOGM_INFO("CSACPCouldServer::HandleServerRequest start.\n");
    mDebugHeartBeat ++;

do_retry:
    socket = gfNet_Accept(mSocket, (void *)&client_addr, (TSocketSize *)&clilen, err);
    if (DUnlikely(socket < 0)) {
        if (EECode_TryAgain == err) {
            LOGM_WARN("non-blocking call?\n");
            goto do_retry;
        }
#if 0
        TInt err = errno;
        if (err == EINTR || err == EAGAIN || err == EWOULDBLOCK) {
            LOGM_WARN("non-blocking call?(err == %d, EINTR %d)\n", err, EINTR);
            goto do_retry;
        }
#endif
        LOGM_ERROR("accept fail, return %d.\n", socket);
    } else {
        LOGM_PRINTF(" NEW client: %s, port %d, socket %d.\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), socket);
        gfSocketSetTimeout(socket, DPresetSocketTimeOutUintSeconds, DPresetSocketTimeOutUintUSeconds);
        SCloudContent *client_content = NULL;
        if (EECode_OK == (err = authentication(socket, client_content))) {
            if (DLikely(client_content)) {
                if (DUnlikely(client_content->client.b_connected)) {
                    LOGM_WARN("why it is connected?, current ip address %s, original ip address %s\n", inet_ntoa(client_addr.sin_addr), inet_ntoa(client_content->client.client_addr.sin_addr));
                    removeClient(client_content);
                }
                memcpy(&client_content->client.client_addr, &client_addr, sizeof(struct sockaddr_in));
            } else {
                LOGM_FATAL("NULL client_content\n");
            }
            EECode err = newClient(client_content, socket);
            if (DLikely(EECode_OK == err)) {
                SCloudAgentSetting setting;
                setting.server_agent = client_content->client.p_agent;
                setting.p_user_token = NULL;
                setting.user_token_length = 0;
                setting.uploading_url = NULL;
                DASSERT(client_content->p_cloud_agent_filter);
                return client_content->p_cloud_agent_filter->FilterProperty(EFilterPropertyType_assign_cloud_agent, 1, (void *)&setting);
            }
        } else {
            gfNetCloseSocket(socket);
            socket = DInvalidSocketHandler;
            LOGM_WARN("authentication fail\n");
            return err;
        }
    }

    return EECode_OK;
}

void CSACPCouldServer::replyToClient(TInt socket, EECode error_code, SSACPHeader *header, TInt length)
{
    SSACPHeader tmp_header;

    if (!header) {
        memset(&tmp_header, 0x0, sizeof(tmp_header));
        header = &tmp_header;
        length = (TInt)(sizeof(SSACPHeader));
    } else {
        if (!(header->flags & DSACPHeaderFlagBit_ReplyResult)) {
            LOGM_WARN("DSACPHeaderFlagBit_ReplyResult not set in header, will not reply to client.\n");
            return;
        }
    }

    switch (error_code) {

        case EECode_OK:
            header->flags = ESACPConnectResult_OK;
            break;

        case EECode_OK_NeedHardwareAuthenticate:
            header->flags = ESACPRequestResult_Server_NeedHardwareAuthenticate;
            break;

        case EECode_ServerReject_NoSuchChannel:
            header->flags = ESACPConnectResult_Reject_NoSuchChannel;
            LOGM_ERROR("EECode_ServerReject_NoSuchChannel\n");
            break;

        case EECode_ServerReject_ChannelIsBusy:
            header->flags = ESACPConnectResult_Reject_ChannelIsInUse;
            LOGM_ERROR("ESACPConnectResult_Reject_ChannelIsInUse\n");
            break;

        case EECode_ServerReject_BadRequestFormat:
            header->flags = ESACPConnectResult_Reject_BadRequestFormat;
            LOGM_ERROR("ESACPConnectResult_Reject_BadRequestFormat\n");
            break;

        case EECode_ServerReject_CorruptedProtocol:
            header->flags = ESACPConnectResult_Reject_CorruptedProtocol;
            LOGM_ERROR("ESACPConnectResult_Reject_CorruptedProtocol\n");
            break;

        default:
            header->flags = ESACPConnectResult_Reject_Unknown;
            LOGM_ERROR("ESACPConnectResult_Reject_Unknown, %d, %s\n", error_code, gfGetErrorCodeString(error_code));
            break;
    }

    LOGM_INFO("header->flags %08x\n", header->flags);
    TInt ret = gfNet_Send(socket, (TU8 *)header, length, 0);
    if (DUnlikely(ret != length)) {
        LOGM_FATAL("send reply error, ret %d(should be %d)\n", ret, length);
    }

}

EECode CSACPCouldServer::authentication(TInt socket, SCloudContent *&p_authen_content)
{
    CIDoubleLinkedList::SNode *p_node = mpContentList->FirstNode();
    SCloudContent *p_content = NULL;
    TMemSize string_size = 0, string_size1 = 0;
    SSACPHeader *p_header = (SSACPHeader *)mpReadBuffer;

    if (!p_node) {
        LOGM_ERROR("no content\n");
        replyToClient(socket, EECode_BadState, NULL, 0);
        return EECode_BadState;
    }

    p_content = (SCloudContent *)p_node->p_context;
    if (!p_content) {
        LOGM_FATAL("NULL p_context\n");
        replyToClient(socket, EECode_InternalLogicalBug, NULL, 0);
        return EECode_InternalLogicalBug;
    }

    DASSERT(0 < socket);
    TInt bytesRead = gfNet_Recv_timeout(socket, mpReadBuffer, sizeof(SSACPHeader), DNETUTILS_RECEIVE_FLAG_READ_ALL, 0);

    if (DUnlikely(((TInt)sizeof(SSACPHeader)) != bytesRead)) {
        LOGM_WARN("authentication, recv %d not expected, expect %lu\n", bytesRead, (TULong)sizeof(SSACPHeader));
        replyToClient(socket, EECode_Closed, p_header, (TInt)sizeof(SSACPHeader));
        return EECode_Closed;
    }

    DASSERT(bytesRead == sizeof(SSACPHeader));

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ConnectionTag, 0);
    TU32 cur_type = (p_header->type_1 << 24) | (p_header->type_2 << 16) | (p_header->type_3 << 8) | p_header->type_4;
    if (DLikely(cur_type == type)) {
        TU32 header_url_size = ((TU32)p_header->size_0 << 16) | ((TU32)p_header->size_1 << 8) | (TU32)p_header->size_2;
        if (!header_url_size) {
            LOGM_ERROR("no authenication string\n");
            replyToClient(socket, EECode_ServerReject_BadRequestFormat, p_header, (TInt)sizeof(SSACPHeader));
            return EECode_Closed;
        } else if (DUnlikely(header_url_size >= DMaxCloudAgentUrlLength)) {
            LOGM_WARN("too long input string %d\n", header_url_size);
            replyToClient(socket, EECode_ServerReject_BadRequestFormat, p_header, (TInt)sizeof(SSACPHeader));
            return EECode_Closed;
        } else {
            TChar tmp_string[DMaxCloudAgentUrlLength] = {0};
            TInt bytesRead = gfNet_Recv(socket, (TU8 *)&tmp_string[0], header_url_size, DNETUTILS_RECEIVE_FLAG_READ_ALL);
            if (DUnlikely(((TInt)header_url_size) != bytesRead)) {
                LOGM_WARN("authentication, recv %d not expected, expect %d\n", bytesRead, header_url_size);
                replyToClient(socket, EECode_ServerReject_BadRequestFormat, p_header, (TInt)sizeof(SSACPHeader));
                return EECode_Closed;
            }

            tmp_string[DMaxCloudAgentUrlLength - 1] = 0x0;
            string_size1 = strlen(tmp_string);

            LOGM_INFO("debug string %s, header_url_size %d, string_size1 %ld\n", tmp_string, header_url_size, string_size1);

            while (p_node) {
                p_content = (SCloudContent *)p_node->p_context;
                DASSERT(p_content);
                if (DLikely(p_content)) {
                    string_size = strlen(p_content->content_name);

                    LOGM_INFO("p_content->content_name %s, string_size %ld\n", p_content->content_name, string_size);
                    if (string_size1 == string_size) {
                        if (!memcmp(p_content->content_name, tmp_string, string_size1)) {
                            LOGM_INFO("find matched string %s, connected %d\n", p_content->content_name, p_content->client.b_connected);
                            p_authen_content = p_content;
                            if (DUnlikely(p_content->client.b_connected)) {
                                if (!strcmp(DRemoteControlString, tmp_string)) {
                                    p_node = mpContentList->NextNode(p_node);
                                    continue;
                                }
                                //LOGM_ERROR("channel is in use %s, multiple client try to connect it?\n", p_content->content_name);
                                //replyToClient(socket, EECode_ServerReject_ChannelIsBusy, &header);
                                //return EECode_Closed;

                                //replyToClient(socket, EECode_OK, &header);
                            } else {
                                //replyToClient(socket, EECode_OK, &header);
                            }
                            if (!mbEnableHardwareAuthenticate) {
                                replyToClient(socket, EECode_OK, p_header, (TInt)sizeof(SSACPHeader));
                            } else {
                                //generate hardware_authentication_input, send to client, then save hardware verify code from hashv3 to check in mpCloudClientAgent->HardwareAuthenticate cmd
                                TU8 randomseed[64] = {0};
                                gfGetCurrentDateTime((SDateTime *) randomseed);
                                randomseed[sizeof(SDateTime)] = 0x01;
                                randomseed[sizeof(SDateTime) + 1] = 0xab;
                                randomseed[sizeof(SDateTime) + 2] = 0x56;
                                randomseed[sizeof(SDateTime) + 3] = 0xef;
                                TU64 dynamic_input = gfHashAlgorithmV5(randomseed, sizeof(SDateTime) + 4);
                                TU8 *p = mpReadBuffer + sizeof(SSACPHeader);
                                p[0] = (dynamic_input >> 56) & 0xff;
                                p[1] = (dynamic_input >> 48) & 0xff;
                                p[2] = (dynamic_input >> 40) & 0xff;
                                p[3] = (dynamic_input >> 32) & 0xff;
                                p[4] = (dynamic_input >> 24) & 0xff;
                                p[5] = (dynamic_input >> 16) & 0xff;
                                p[6] = (dynamic_input >> 8) & 0xff;
                                p[7] = (dynamic_input) & 0xff;
                                TU32 hardware_verify_code = gfHashAlgorithmV3(p, DDynamicInputStringLength);
                                replyToClient(socket, EECode_OK_NeedHardwareAuthenticate, p_header, sizeof(SSACPHeader) + DDynamicInputStringLength);
                                //receive HardwareVerifyCode from client and finish the verification
                                bytesRead = gfNet_Recv_timeout(socket, mpReadBuffer, sizeof(SSACPHeader), DNETUTILS_RECEIVE_FLAG_READ_ALL, 0);
                                if (DUnlikely(((TInt)sizeof(SSACPHeader)) != bytesRead)) {
                                    LOGM_WARN("authentication, recv %d not expected, expect %lu\n", bytesRead, (TULong)sizeof(SSACPHeader));
                                    replyToClient(socket, EECode_Closed, p_header, (TInt)sizeof(SSACPHeader));
                                    return EECode_Closed;
                                }
                                header_url_size = ((TU32)p_header->size_0 << 16) | ((TU32)p_header->size_1 << 8) | (TU32)p_header->size_2;
                                if (DUnlikely((TU16)sizeof(hardware_verify_code) != header_url_size)) {
                                    LOGM_WARN("authentication, data header size %hu not expected, expect %zu\n", header_url_size, sizeof(hardware_verify_code));
                                    replyToClient(socket, EECode_ServerReject_BadRequestFormat, p_header, (TInt)sizeof(SSACPHeader));
                                    return EECode_Closed;
                                }
                                bytesRead = gfNet_Recv_timeout(socket, mpReadBuffer + sizeof(SSACPHeader), header_url_size, DNETUTILS_RECEIVE_FLAG_READ_ALL, 0);
                                if (DUnlikely((TInt)header_url_size != bytesRead)) {
                                    LOGM_WARN("authentication, recv %d not expected, expect %hu\n", bytesRead, header_url_size);
                                    replyToClient(socket, EECode_NetReceivePayload_Fail, p_header, (TInt)sizeof(SSACPHeader));
                                    return EECode_NetReceivePayload_Fail;
                                }
                                TU32 hardware_verify_code_rcved = (((TU32)(p[0])) << 24) | (((TU32)(p[1])) << 16) | (((TU32)(p[2])) << 8) | (TU32)(p[3]);
                                if (hardware_verify_code_rcved != hardware_verify_code) {
                                    LOGM_WARN("authentication, HardwareAuthenticate failed, hardware_verify_code_rcved=%u, hardware_verify_code=%u\n", hardware_verify_code_rcved, hardware_verify_code);
                                    replyToClient(socket, EECode_ServerReject_AuthenticationFail, p_header, (TInt)sizeof(SSACPHeader));
                                    return EECode_ServerReject_AuthenticationFail;
                                }
                                replyToClient(socket, EECode_OK, p_header, (TInt)sizeof(SSACPHeader));
                            }
                            return EECode_OK;
                        }
                    }
                } else {
                    LOGM_FATAL("NULL p_content\n");
                    replyToClient(socket, EECode_InternalLogicalBug, p_header, (TInt)sizeof(SSACPHeader));
                    return EECode_InternalLogicalBug;
                }
                p_node = mpContentList->NextNode(p_node);
            }

            if (!strcmp(DRemoteControlString, tmp_string)) {
                LOGM_ERROR("too many remote control connections\n");
                replyToClient(socket, EECode_ServerReject_ChannelIsBusy, p_header, (TInt)sizeof(SSACPHeader));
                return EECode_Closed;
            }

            LOGM_ERROR("do not find uploading string %s, header_url_size %d, string_size1 %ld, blow is what APP configured\n", tmp_string, header_url_size, string_size1);
            //debug use
            p_node = mpContentList->FirstNode();
            while (p_node) {
                p_content = (SCloudContent *)p_node->p_context;
                DASSERT(p_content);
                if (DLikely(p_content)) {
                    string_size = strlen(p_content->content_name);
                    LOGM_INFO("p_content(id %08x, index %d)->content_name %s, string_size %ld\n", p_content->id, p_content->content_index, p_content->content_name, string_size);
                } else {
                    LOGM_FATAL("NULL p_content\n");
                }
                p_node = mpContentList->NextNode(p_node);
            }

        }
    } else {
        LOGM_FATAL("client error, bad type when connecting, cur_type 0x%08x, type 0x%08x\n", cur_type, type);
        replyToClient(socket, EECode_ServerReject_CorruptedProtocol, p_header, (TInt)sizeof(SSACPHeader));
        return EECode_InternalLogicalBug;
    }

    replyToClient(socket, EECode_ServerReject_NoSuchChannel, p_header, (TInt)sizeof(SSACPHeader));
    return EECode_Closed;
}

CSACPCouldServer::CSACPCouldServer(const TChar *name, CloudServerType type, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU16 server_port)
    : inherited(name, 0)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpMsgSink(pMsgSink)
    , mpMutex(NULL)
    , mpReadBuffer(NULL)
    , mnMaxPayloadLength(0)
    , mId(0)
    , mIndex(0)
    , mListeningPort(server_port)
    , msState(EServerState_closed)
    , mSocket(-1)
    , mpContentList(NULL)
    , mbEnableHardwareAuthenticate(0)
{
    if (DLikely(mpPersistMediaConfig)) {
        mpSystemClockReference = mpPersistMediaConfig->p_system_clock_reference;
    } else {
        mpSystemClockReference = NULL;
        LOGM_FATAL("NULL mpPersistMediaConfig\n");
    }
}

CSACPCouldServer::~CSACPCouldServer()
{
    if (mpContentList) {
        delete mpContentList;
        mpContentList = NULL;
    }

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    if (mpReadBuffer) {
        free(mpReadBuffer);
        mpReadBuffer = NULL;
        mnMaxPayloadLength = 0;
    }
}

ICloudServer *CSACPCouldServer::Create(const TChar *name, CloudServerType type, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU16 server_port)
{
    CSACPCouldServer *result = new CSACPCouldServer(name, type, pPersistMediaConfig, pMsgSink, server_port);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CObject *CSACPCouldServer::GetObject0() const
{
    return (CObject *) this;
}

EECode CSACPCouldServer::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleCloudServer);
    //mConfigLogLevel = ELogLevel_Debug;
    //mConfigLogOption = 0xffffffff;

    mpContentList = new CIDoubleLinkedList();
    if (DUnlikely(!mpContentList)) {
        LOGM_FATAL("No Memory, new CIDoubleLinkedList() fail\n");
        return EECode_NoMemory;
    }

    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOGM_FATAL("No Memory\n");
        return EECode_NoMemory;
    }

    //DASSERT(DDefaultSACPServerPort == mListeningPort);

    mSocket = gfNet_SetupStreamSocket(INADDR_ANY, mListeningPort, 1);
    if (DUnlikely(0 > mSocket)) {
        LOGM_FATAL("setup socket fail, ret %d.\n", mSocket);
        return EECode_Error;
    }
    LOGM_INFO("gfNet_SetupStreamSocket, socket %d.\n", mSocket);

    gfSocketSetDeferAccept(mSocket, 2);

    mpReadBuffer = (TU8 *) DDBG_MALLOC(DSACP_MAX_PAYLOAD_LENGTH, "SVCR");
    if (DUnlikely(!mpReadBuffer)) {
        LOGM_FATAL("alloc mpReadBuffer fail.\n");
        return EECode_NoMemory;
    }
    mnMaxPayloadLength = DSACP_MAX_PAYLOAD_LENGTH;

    msState = EServerState_running;

    return EECode_OK;
}

EECode CSACPCouldServer::newClient(SCloudContent *p_content, TInt socket)
{
    DASSERT(p_content);
    DASSERT(!p_content->client.b_connected);
    DASSERT(0 >= p_content->client.socket);
    DASSERT(!p_content->client.p_agent);

    if (DLikely(p_content)) {
        ICloudServerAgent *p_agent = gfCreateCloudServerAgent(EProtocolType_SACP, mpPersistMediaConfig->engine_config.filter_version);

        if (DLikely(p_agent)) {
            p_agent->AcceptClient(socket);
            p_content->client.p_agent = (void *) p_agent;
            p_content->client.socket = socket;
            p_content->client.b_connected = 1;
            return EECode_OK;
        } else {
            LOGM_FATAL("gfCreateCloudServerNVRAgent fail\n");
            return EECode_NoMemory;
        }
    } else {
        LOGM_FATAL("NULL p_content\n");
    }

    return EECode_InternalLogicalBug;
}

void CSACPCouldServer::removeClient(SCloudContent *p_content)
{
    if (DLikely(p_content)) {
        if (p_content->client.b_connected) {
            DASSERT(0 <= p_content->client.socket);
            if (DLikely(p_content->p_cloud_agent_filter)) {
                if (p_content->client.p_agent) {
                    SCloudAgentSetting setting;
                    DASSERT(p_content->client.p_agent);
                    setting.server_agent = p_content->client.p_agent;
                    p_content->p_cloud_agent_filter->FilterProperty(EFilterPropertyType_remove_cloud_agent, 1, (void *)&setting);

                    ICloudServerAgent *p_agent = (ICloudServerAgent *) p_content->client.p_agent;
                    p_agent->CloseConnection();
                    p_agent->Destroy();
                    p_content->client.p_agent = NULL;
                    p_content->client.b_connected = 0;
                    p_content->client.socket = DInvalidSocketHandler;
                }
            } else {
                LOGM_FATAL("NULL p_content->p_cloud_agent_filter, name %s, id 0x%08x, p_server %p\n", p_content->content_name, p_content->id, p_content->p_server);
            }
        }
    } else {
        LOGM_FATAL("NULL p_content\n");
    }
}

void CSACPCouldServer::Delete()
{
    CIDoubleLinkedList::SNode *pnode;
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    if (mpReadBuffer) {
        free(mpReadBuffer);
        mpReadBuffer = NULL;
        mnMaxPayloadLength = 0;
    }

    //clean each client session
    SCloudContent *p_content;
    pnode = mpContentList->FirstNode();
    while (pnode) {
        p_content = (SCloudContent *)(pnode->p_context);
        pnode = mpContentList->NextNode(pnode);
        DASSERT(p_content);
        if (p_content->client.b_connected) {
            removeClient(p_content);
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
    }

    LOGM_INFO("CSACPCouldServer::deleteServer, before close mSocket %d.\n", mSocket);
    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

}

void CSACPCouldServer::PrintStatus()
{
    if (!(mpPersistMediaConfig->runtime_print_mask & DLOG_MASK_TO_REMOTE)) {
        LOGM_PRINTF("heart beat %d\n", mDebugHeartBeat);
    }
    mDebugHeartBeat = 0;

    CIDoubleLinkedList::SNode *p_node = mpContentList->FirstNode();
    SCloudContent *p_content = NULL;

    CIRemoteLogServer *p_log_server = mpPersistMediaConfig->p_remote_log_server;
    if (DLikely((p_log_server) && (mpPersistMediaConfig->runtime_print_mask & DLOG_MASK_TO_REMOTE))) {
        p_log_server->WriteLog("\033[40;35m\033[1m\n\tclients to cloud server:\n\033[0m");
    }
    TU32 print_count = 0;

    while (p_node) {
        p_content = (SCloudContent *)p_node->p_context;
        if (DLikely(p_content)) {
            if (p_content->client.b_connected) {
                if (!(mpPersistMediaConfig->runtime_print_mask & DLOG_MASK_TO_REMOTE)) {
                    LOGM_PRINTF("connected client: %s, port %d, socket %d. content index %d, %s\n", inet_ntoa(p_content->client.client_addr.sin_addr), ntohs(p_content->client.client_addr.sin_port), p_content->client.socket, p_content->content_index, p_content->content_name);
                }

                CIRemoteLogServer *p_log_server = mpPersistMediaConfig->p_remote_log_server;
                if (DLikely((p_log_server) && (mpPersistMediaConfig->runtime_print_mask & DLOG_MASK_TO_REMOTE))) {
                    //p_log_server->WriteLog("%s, port %d, socket %d. index %d, %s\n", inet_ntoa(p_content->client.client_addr.sin_addr), ntohs(p_content->client.client_addr.sin_port), p_content->client.socket, p_content->content_index, p_content->content_name);
                    if (!((print_count + 1) % 5)) {
                        p_log_server->WriteLog("%s:%d,%d,%s\n", inet_ntoa(p_content->client.client_addr.sin_addr), ntohs(p_content->client.client_addr.sin_port), p_content->content_index, p_content->content_name);
                    } else {
                        p_log_server->WriteLog("%s:%d,%d,%s\t", inet_ntoa(p_content->client.client_addr.sin_addr), ntohs(p_content->client.client_addr.sin_port), p_content->content_index, p_content->content_name);
                    }
                    print_count ++;
                }
            }
        } else {
            LOGM_FATAL("NULL p_content\n");
        }
        p_node = mpContentList->NextNode(p_node);
    }
}

EECode CSACPCouldServer::AddCloudContent(SCloudContent *content)
{
    LOGM_INFO("ECMDType_AddContent, content index %d, id 0x%08x, %s\n", content->content_index, content->id, content->content_name);

    AUTO_LOCK(mpMutex);

    mpContentList->InsertContent(NULL, (void *) content, 0);

    return EECode_OK;
}

EECode CSACPCouldServer::RemoveCloudContent(SCloudContent *content)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CSACPCouldServer::RemoveCloudClient(void *filter_owner)
{
    DASSERT(filter_owner);

    CIDoubleLinkedList::SNode *pnode = mpContentList->FirstNode();
    SCloudContent *p_content = NULL;
    while (pnode) {
        p_content = (SCloudContent *)(pnode->p_context);
        pnode = mpContentList->NextNode(pnode);
        DASSERT(p_content);
        if (filter_owner == p_content->p_cloud_agent_filter) {
            removeClient(p_content);
            return EECode_OK;
        }
    }

    LOGM_ERROR("do not find client related to filter %p\n", filter_owner);
    return EECode_BadParam;
}

EECode CSACPCouldServer::GetHandler(TSocketHandler &handle) const
{
    handle = mSocket;
    return EECode_OK;
}

EECode CSACPCouldServer::GetServerState(EServerState &state) const
{
    state = msState;
    return EECode_OK;
}

EECode CSACPCouldServer::SetServerState(EServerState state)
{
    msState = state;
    return EECode_OK;
}

EECode CSACPCouldServer::GetServerID(TGenericID &id, TComponentIndex &index, TComponentType &type) const
{
    id = mId;
    index = mIndex;
    type = EGenericComponentType_CloudServer;
    return EECode_OK;
}

EECode CSACPCouldServer::SetServerID(TGenericID id, TComponentIndex index, TComponentType type)
{
    mId = id;
    mIndex = index;
    DASSERT(EGenericComponentType_CloudServer == type);
    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


