/*******************************************************************************
 * im_server.cpp
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

#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#endif

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"
#include "common_network_utils.h"

#include "security_utils_if.h"

#include "cloud_lib_if.h"
#include "sacp_types.h"

#include "im_mw_if.h"
#include "im_server.h"

#include "common_thread_pool.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IIMServer *gfCreateSACPIMServer(const TChar *name, EProtocolType type, const volatile SPersistCommonConfig *pPersistCommonConfig, IMsgSink *pMsgSink, IAccountManager *p_account_manager, TU16 server_port)
{
    return (IIMServer *)CIMServer::Create(name, type, pPersistCommonConfig, pMsgSink, p_account_manager, server_port);
}

EECode CIMServer::HandleServerRequest(TSocketHandler &max, void *all_set)
{
    TSocketHandler socket = 0;
    struct sockaddr_in client_addr;
    socklen_t   clilen = sizeof(struct sockaddr_in);

    LOGM_INFO("CIMServer::HandleServerRequest start.\n");

do_retry:
    socket = accept(mSocket, (struct sockaddr *)&client_addr, &clilen);
    if (DUnlikely(socket < 0)) {
#ifdef BUILD_OS_WINDOWS
        int err = WSAGetLastError();
        if ((err == WSAEINTR) || (err == WSAEWOULDBLOCK)) {
            LOGM_WARN("non-blocking call?(err == %d)\n", err);
            goto do_retry;
        }
#else
        TInt err = errno;
        if (err == EINTR || err == EAGAIN || err == EWOULDBLOCK) {
            LOGM_WARN("non-blocking call?(err == %d, EINTR %d)\n", err, EINTR);
            goto do_retry;
        }
#endif
        LOGM_ERROR("accept fail, return %d.\n", socket);
    } else {
        LOGM_PRINTF(" NEW client: %s, port %d, socket %d.\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), socket);
        gfSocketSetRecvBufferSize(mSocket, 8 * 1024);
        gfSocketSetSendBufferSize(mSocket, 8 * 1024);
        gfSocketSetNoDelay(mSocket);
        gfSocketSetLinger(mSocket, 0, 0, 20);
        gfSocketSetTimeout(socket, 1, 0);
        SAccountInfoRoot *account_info = NULL;
        EECode err = authentication(socket, account_info);
        if (EECode_OK == err) {

            DASSERT(account_info);
            void *pre_agent = account_info->p_agent;
            if (DUnlikely(pre_agent)) {
#ifdef BUILD_OS_WINDOWS
                mpSimpleScheduler->RemoveScheduledCilent(static_cast<IScheduledClient *>(account_info->p_agent));
#else
                ((CNetworkAgent *)account_info->p_agent)->EventHandling(EECode_MandantoryDelete);
#endif
            } else {
                account_info->p_agent = (void *) CNetworkAgent::Create(mpAccountManager, account_info, EProtocolType_SACP);
                if (DUnlikely(!account_info->p_agent)) {
                    LOGM_FATAL("CNetworkAgent::Create fail\n");
                    return EECode_NoMemory;
                }
            }

            ((CNetworkAgent *)(account_info->p_agent))->SetSocket(socket);

#ifndef BUILD_OS_WINDOWS
            EECode err = mpThreadPool->EnqueueCilent(static_cast<IScheduledClient *>(account_info->p_agent));
#else
            EECode err = mpSimpleScheduler->AddScheduledCilent(static_cast<IScheduledClient *>(account_info->p_agent));
#endif
            if (DUnlikely(EECode_OK != err)) {
                LOGM_FATAL("add scheduled client fail!\n");
                ((CNetworkAgent *)(account_info->p_agent))->Destroy();
                account_info->p_agent = NULL;
                return EECode_Error;
            }

        } else if (EECode_OK_IsolateAccess == err) {
            gfNetCloseSocket(socket);
            socket = DInvalidSocketHandler;
            return EECode_OK;
        } else {
            gfNetCloseSocket(socket);
            socket = DInvalidSocketHandler;
            LOGM_WARN("authentication fail\n");
            return err;
        }
    }

    return EECode_OK;
}

void CIMServer::replyToClient(TInt socket, EECode error_code, TU8 *header, TInt length)
{
    TInt ret = length - sizeof(SSACPHeader) - sizeof(TUniqueID);
    TU8 flag = gfSACPErrorCodeToConnectResult(error_code);

    DSetSACPSize(ret, header);
    DSetSACPRetBits(DSACPTypeBit_Responce, header);
    DSetSACPRetFlag(flag, header);

    ret = gfNet_Send(socket, header, length, 0);
    if (DUnlikely(ret != length)) {
        LOGM_FATAL("send reply error, ret %d\n", ret);
    }
}

EECode CIMServer::authentication(TInt socket, SAccountInfoRoot *&p_account_info)
{
    p_account_info = NULL;
    DASSERT(0 < socket);
    TU32 bytesRead = gfNet_Recv_timeout(socket, mpReadBuffer, mnHeaderLength, DNETUTILS_RECEIVE_FLAG_READ_ALL, 0);

    if (DUnlikely(mnHeaderLength != bytesRead)) {
        LOGM_WARN("authentication, recv %d not expected, expect %d\n", bytesRead, mnHeaderLength);
        replyToClient(socket, EECode_Closed, mpReadBuffer, mnHeaderLength);
        return EECode_Closed;
    }

    TU32 payload_size = 0, reqres_bits = 0, cat = 0, sub_type = 0;
    mpProtocolHeaderParser->Parse(mpReadBuffer, payload_size, reqres_bits, cat, sub_type);

    if (DUnlikely((payload_size + mnHeaderLength) > mnMaxPayloadLength)) {
        LOGM_WARN("authentication, payload(%d) too long, greater than %d\n", payload_size, mnMaxPayloadLength);
        replyToClient(socket, EECode_ServerReject_AuthenticationDataTooLong, mpReadBuffer, mnHeaderLength);
        return EECode_Closed;
    }

    if (DLikely((reqres_bits == DSACPTypeBit_Request) && (ESACPTypeCategory_ConnectionTag == cat))) {
        bytesRead = gfNet_Recv(socket, mpReadBuffer + mnHeaderLength, payload_size, DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(payload_size != bytesRead)) {
            LOGM_WARN("authentication, recv %d not expected, expect %d\n", bytesRead, payload_size);
            replyToClient(socket, EECode_ServerReject_BadRequestFormat, mpReadBuffer, mnHeaderLength);
            return EECode_Closed;
        }
    } else {
        replyToClient(socket, EECode_ServerReject_CorruptedProtocol, mpReadBuffer, mnHeaderLength);
        LOGM_FATAL("sacp header not as expected, reqres_bits %x, cat %d\n", reqres_bits, cat);
        return EECode_Closed;
    }

    TU32 cur_size = 0;
    TU8 *p_cur = mpReadBuffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU16 tlv_type = 0, tlv_length = 0;
    TU32 length = 0;
    SSACPHeader *p_header = NULL;
    TUniqueID id = 0;
    EECode err = EECode_OK;

    LOGM_NOTICE("[DEBUG]: authentication(%x, %s), read payload %d, cat %d\n", sub_type, gfGetSACPClientCmdSubtypeString((ESACPClientCmdSubType)sub_type), payload_size, cat);
    //gfPrintMemory(p_cur, payload_size);

    if (ESACPClientCmdSubType_SinkRegister == sub_type) {
        TChar name[DMAX_ACCOUNT_NAME_LENGTH_EX + 4] = {0};
        TChar password[DIdentificationStringLength + 4] = {0};

        p_tlv = (STLV16HeaderBigEndian *) p_cur;
        tlv_type = ((TU16)p_tlv->type_high << 8) | (p_tlv->type_low);
        tlv_length = ((TU16)p_tlv->length_high << 8) | (p_tlv->length_low);
        p_cur += sizeof(STLV16HeaderBigEndian);
        if (DLikely(ETLV16Type_AccountName == tlv_type)) {
            if (DUnlikely((DMAX_ACCOUNT_NAME_LENGTH_EX + 1) < tlv_length)) {
                replyToClient(socket, EECode_ServerReject_CorruptedProtocol, mpReadBuffer, mnHeaderLength);
                LOGM_ERROR("ETLV16Type_AccountName size(%d) not as expected\n", tlv_length);
                return EECode_ServerReject_CorruptedProtocol;
            }
            memcpy(name, p_cur, tlv_length);
        } else {
            replyToClient(socket, EECode_ServerReject_CorruptedProtocol, mpReadBuffer, mnHeaderLength);
            LOGM_ERROR("not ETLV16Type_AccountName, %x\n", tlv_type);
            return EECode_ServerReject_CorruptedProtocol;
        }
        p_cur += tlv_length;

        p_tlv = (STLV16HeaderBigEndian *) p_cur;
        tlv_type = ((TU16)p_tlv->type_high << 8) | (p_tlv->type_low);
        tlv_length = ((TU16)p_tlv->length_high << 8) | (p_tlv->length_low);
        p_cur += sizeof(STLV16HeaderBigEndian);
        if (DLikely(ETLV16Type_AccountPassword == tlv_type)) {
            if (DUnlikely((DIdentificationStringLength + 1) < tlv_length)) {
                replyToClient(socket, EECode_ServerReject_CorruptedProtocol, mpReadBuffer, mnHeaderLength);
                LOGM_ERROR("ETLV16Type_AccountPassword size(%d) not as expected\n", tlv_length);
                return EECode_ServerReject_CorruptedProtocol;
            }
            memcpy(password, p_cur, tlv_length);
        } else {
            replyToClient(socket, EECode_ServerReject_CorruptedProtocol, mpReadBuffer, mnHeaderLength);
            LOGM_ERROR("not ETLV16Type_AccountPassword\n");
            return EECode_ServerReject_CorruptedProtocol;
        }
        p_cur += tlv_length;

        SAccountInfoUser *p_user = NULL;
        err = mpAccountManager->NewUserAccount(name, password, id, EAccountCompany_Ambarella, p_user);
        if (DUnlikely(EECode_OK != err)) {
            replyToClient(socket, err, mpReadBuffer, mnHeaderLength);
            LOGM_ERROR("NewUserAccount fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        } else {
            p_cur = mpReadBuffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
            p_tlv = (STLV16HeaderBigEndian *) p_cur;
            p_tlv->type_high = (ETLV16Type_AccountID >> 8) & 0xff;
            p_tlv->type_low = (ETLV16Type_AccountID) & 0xff;
            p_tlv->length_high = (sizeof(TUniqueID) >> 8) & 0xff;
            p_tlv->length_low = (sizeof(TUniqueID)) & 0xff;
            p_cur += sizeof(STLV16HeaderBigEndian);
            DBEW64(id, p_cur);
            LOG_PRINTF("register (%s, %s) id %llx\n", name, password, id);
            replyToClient(socket, EECode_OK, mpReadBuffer, sizeof(SSACPHeader) + sizeof(TUniqueID) + sizeof(TUniqueID) + sizeof(STLV16HeaderBigEndian));
        }
        p_account_info = (SAccountInfoRoot *) p_user;
        return EECode_OK;
    } else if (ESACPClientCmdSubType_SourceAuthentication == sub_type) {
        SAccountInfoSourceDevice *p_device = NULL;
        p_tlv = (STLV16HeaderBigEndian *) p_cur;
        tlv_type = ((TU16)p_tlv->type_high << 8) | (p_tlv->type_low);
        tlv_length = ((TU16)p_tlv->length_high << 8) | (p_tlv->length_low);
        p_cur += sizeof(STLV16HeaderBigEndian);
        if (DLikely(ETLV16Type_AccountID == tlv_type)) {
            if (DUnlikely(sizeof(TUniqueID) != tlv_length)) {
                replyToClient(socket, EECode_ServerReject_CorruptedProtocol, mpReadBuffer, mnHeaderLength);
                LOGM_ERROR("ETLV16Type_AccountID size(%d) not as expected\n", tlv_length);
                return EECode_ServerReject_CorruptedProtocol;
            }
            DBER64(id, p_cur);
            err = mpAccountManager->QueryAccount(id, p_account_info);
            if (DUnlikely(EECode_OK != err)) {
                replyToClient(socket, EECode_ServerReject_NoSuchChannel, mpReadBuffer, mnHeaderLength);
                LOGM_ERROR("QueryAccount fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                return EECode_NotExist;
            }
            p_device = (SAccountInfoSourceDevice *) p_account_info;
        } else if (ETLV16Type_AccountName == tlv_type) {
            if (DUnlikely((DMAX_ACCOUNT_NAME_LENGTH_EX + 1) < tlv_length)) {
                replyToClient(socket, EECode_ServerReject_CorruptedProtocol, mpReadBuffer, mnHeaderLength);
                LOGM_ERROR("ETLV16Type_AccountName size(%d) not as expected\n", tlv_length);
                return EECode_ServerReject_CorruptedProtocol;
            }
            TChar name[DMAX_ACCOUNT_NAME_LENGTH_EX + 4] = {0};
            memcpy(name, p_cur, tlv_length);
            err = mpAccountManager->QuerySourceDeviceAccountByName(name, p_device);
            if (DUnlikely(EECode_OK != err)) {
                replyToClient(socket, EECode_ServerReject_NoSuchChannel, mpReadBuffer, mnHeaderLength);
                LOGM_ERROR("QuerySourceDeviceAccountByName fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                return EECode_NotExist;
            }
        } else {
            replyToClient(socket, EECode_ServerReject_CorruptedProtocol, mpReadBuffer, mnHeaderLength);
            LOGM_ERROR("not ETLV16Type_AccountName or ETLV16Type_AccountID\n");
            return EECode_ServerReject_CorruptedProtocol;
        }
        DASSERT(p_device);

        TU8 tmp_buffer[64] = {0};
        gfGetCurrentDateTime((SDateTime *) tmp_buffer);
        tmp_buffer[sizeof(SDateTime)] = 0x31;
        tmp_buffer[sizeof(SDateTime) + 1] = 0xab;
        tmp_buffer[sizeof(SDateTime) + 2] = 0x56;
        tmp_buffer[sizeof(SDateTime) + 3] = 0xef;
        TU64 dynamic_input = gfHashAlgorithmV5(tmp_buffer, sizeof(SDateTime) + 4);
        p_cur = mpReadBuffer + sizeof(SSACPHeader) + sizeof(TUniqueID);

        p_tlv = (STLV16HeaderBigEndian *) p_cur;
        p_tlv->type_high = (ETLV16Type_DynamicInput >> 8) & 0xff;
        p_tlv->type_low = (ETLV16Type_DynamicInput) & 0xff;
        p_tlv->length_high = (DDynamicInputStringLength >> 8) & 0xff;
        p_tlv->length_low = DDynamicInputStringLength & 0xff;
        p_cur += sizeof(STLV16HeaderBigEndian);
        DBEW64(dynamic_input, p_cur);

        replyToClient(socket, EECode_OK, mpReadBuffer, sizeof(SSACPHeader) + sizeof(TUniqueID) + DDynamicInputStringLength + sizeof(STLV16HeaderBigEndian));
        DBEW64(dynamic_input, tmp_buffer);
        memset(tmp_buffer + DDynamicInputStringLength, 0x0, DIdentificationStringLength);
        memcpy(tmp_buffer + DDynamicInputStringLength, p_device->root.base.password, strlen(p_device->root.base.password));
        //gfPrintMemory(tmp_buffer, DDynamicInputStringLength + DIdentificationStringLength);
        TU32 hash_verify = gfHashAlgorithmV6(tmp_buffer, DDynamicInputStringLength + DIdentificationStringLength);

        bytesRead = gfNet_Recv_timeout(socket, mpReadBuffer, sizeof(SSACPHeader) + sizeof(TUniqueID) + sizeof(hash_verify) + sizeof(STLV16HeaderBigEndian), DNETUTILS_RECEIVE_FLAG_READ_ALL, 20);

        if (DUnlikely(((TInt)sizeof(SSACPHeader) + sizeof(TUniqueID) + sizeof(hash_verify) + sizeof(STLV16HeaderBigEndian)) != bytesRead)) {
            LOGM_WARN("ESACPClientCmdSubType_SourceAuthentication, recv hash %d not expected, expect %zu, time out?\n", bytesRead, sizeof(SSACPHeader) + sizeof(TUniqueID) + sizeof(hash_verify) + sizeof(STLV16HeaderBigEndian));
            replyToClient(socket, EECode_ServerReject_TimeOut, mpReadBuffer, mnHeaderLength);
            return EECode_ServerReject_TimeOut;
        }
        p_header = (SSACPHeader *)mpReadBuffer;
        cur_size = ((TU32)p_header->size_0 << 16) | ((TU32)p_header->size_1 << 8) | (TU32)p_header->size_2;
        p_cur = mpReadBuffer + sizeof(SSACPHeader) + sizeof(TUniqueID);

        TU32 hash_recv = 0;
        p_tlv = (STLV16HeaderBigEndian *) p_cur;
        tlv_type = ((TU16)p_tlv->type_high << 8) | (p_tlv->type_low);
        tlv_length = ((TU16)p_tlv->length_high << 8) | (p_tlv->length_low);
        if (DUnlikely((ETLV16Type_AuthenticationResult32 != tlv_type) || (4 != tlv_length))) {
            LOGM_ERROR("ESACPClientCmdSubType_SourceAuthentication, recv ETLV16Type_AuthenticationResult32 fail, size(%d)/type(%d) not as expected\n", tlv_type, tlv_length);
            replyToClient(socket, EECode_ServerReject_CorruptedProtocol, mpReadBuffer, mnHeaderLength);
            return EECode_ServerReject_CorruptedProtocol;
        }

        p_cur += sizeof(STLV16HeaderBigEndian);
        DBER32(hash_recv, p_cur);
        if (hash_recv == hash_verify) {
            gfGetCurrentDateTime((SDateTime *) tmp_buffer);
            tmp_buffer[sizeof(SDateTime)] = 0x4d;
            tmp_buffer[sizeof(SDateTime) + 1] = 0x55;
            tmp_buffer[sizeof(SDateTime) + 2] = 0xcf;
            tmp_buffer[sizeof(SDateTime) + 3] = 0xa7;
            dynamic_input = gfHashAlgorithmV5(tmp_buffer, sizeof(SDateTime) + 4);

            p_cur = mpReadBuffer + sizeof(SSACPHeader) + sizeof(TUniqueID);

            length = DDynamicInputStringLength;
            p_tlv = (STLV16HeaderBigEndian *) p_cur;
            p_tlv->type_high = (ETLV16Type_DynamicInput >> 8) & 0xff;
            p_tlv->type_low = (ETLV16Type_DynamicInput) & 0xff;
            p_tlv->length_high = (length >> 8) & 0xff;
            p_tlv->length_low = length & 0xff;
            p_cur += sizeof(STLV16HeaderBigEndian);
            cur_size += sizeof(STLV16HeaderBigEndian);

            p_cur[0] = (dynamic_input >> 56) & 0xff;
            p_cur[1] = (dynamic_input >> 48) & 0xff;
            p_cur[2] = (dynamic_input >> 40) & 0xff;
            p_cur[3] = (dynamic_input >> 32) & 0xff;
            p_cur[4] = (dynamic_input >> 24) & 0xff;
            p_cur[5] = (dynamic_input >> 16) & 0xff;
            p_cur[6] = (dynamic_input >> 8) & 0xff;
            p_cur[7] = (dynamic_input) & 0xff;
            p_cur += DDynamicInputStringLength;
            cur_size += DDynamicInputStringLength;

            SAccountInfoCloudDataNode *p_datanode = NULL;
            TU32 data_node_index = (id & DExtIDDataNodeMask) >> DExtIDDataNodeShift;
            err = mpAccountManager->QueryDataNodeInfoByIndex(data_node_index, p_datanode);
            if (DUnlikely((EECode_OK != err) || !p_datanode)) {
                LOGM_ERROR("no data node?, index %d, err %d, %s\n", data_node_index, err, gfGetErrorCodeString(err));
                replyToClient(socket, EECode_NoRelatedComponent, mpReadBuffer, mnHeaderLength);
                return EECode_NoRelatedComponent;
            } else {
                length = strlen(p_datanode->ext.url);
                DASSERT(length < DMaxServerUrlLength);
                p_tlv = (STLV16HeaderBigEndian *) p_cur;
                p_tlv->type_high = (ETLV16Type_DataNodeUrl >> 8) & 0xff;
                p_tlv->type_low = (ETLV16Type_DataNodeUrl) & 0xff;
                p_tlv->length_high = ((length + 1) >> 8) & 0xff;
                p_tlv->length_low = (length + 1) & 0xff;
                p_cur += sizeof(STLV16HeaderBigEndian);
                cur_size += sizeof(STLV16HeaderBigEndian);
                memcpy(p_cur, p_datanode->ext.url, length);
                p_cur += length;
                *p_cur ++ = 0x0;
                cur_size += length + 1;

                length = 2;
                p_tlv = (STLV16HeaderBigEndian *) p_cur;
                p_tlv->type_high = (ETLV16Type_DataNodeCloudPort >> 8) & 0xff;
                p_tlv->type_low = (ETLV16Type_DataNodeCloudPort) & 0xff;
                p_tlv->length_high = (length >> 8) & 0xff;
                p_tlv->length_low = length & 0xff;
                p_cur += sizeof(STLV16HeaderBigEndian);
                cur_size += sizeof(STLV16HeaderBigEndian);
                p_cur[0] = (p_datanode->ext.cloud_port >> 8) & 0xff;
                p_cur[1] = p_datanode->ext.cloud_port & 0xff;
                cur_size += length;
#if 0
                //sync dynamic code to data node
                DBEW64(dynamic_input, tmp_buffer);
                memset(tmp_buffer + DDynamicInputStringLength, 0x0, DIdentificationStringLength);
                memcpy(tmp_buffer + DDynamicInputStringLength, p_device->root.base.password, strlen(p_device->root.base.password));
                TU32 dynamic_code = gfHashAlgorithmV6(tmp_buffer, DDynamicInputStringLength + DIdentificationStringLength);

                err = mpAccountManager->UpdateDeviceDynamicCode(p_datanode, p_device->root.base.name, dynamic_code);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("UpdateDeviceDynamicCode fail, dynamic_code %u\n", dynamic_code);
                    //TODO
                    //replyToClient(socket, EECode_UpdateDeviceDynamiccodeFail, mpReadBuffer, mnHeaderLength);
                    //return EECode_UpdateDeviceDynamiccodeFail;
                }
                LOG_PRINTF("UpdateDeviceDynamicCode done, dynamic_code %u\n", dynamic_code);
#endif
            }
            if (DUnlikely(DInvalidUniqueID == p_device->ext.ownerid)) {
                replyToClient(socket, EECode_OK_NeedSetOwner, mpReadBuffer, cur_size + sizeof(SSACPHeader) + sizeof(TUniqueID));
            } else {
                replyToClient(socket, EECode_OK, mpReadBuffer, cur_size + sizeof(SSACPHeader) + sizeof(TUniqueID));
            }
        } else {
            LOGM_ERROR("ESACPClientCmdSubType_SourceAuthentication fail, hash_recv=0x%x, hash_verify=0x%x\n", hash_recv, hash_verify);
            replyToClient(socket, EECode_ServerReject_AuthenticationFail, mpReadBuffer, mnHeaderLength);
            return EECode_ServerReject_AuthenticationFail;
        }

        p_account_info = (SAccountInfoRoot *)p_device;
        return EECode_OK;
    } else if (ESACPClientCmdSubType_SinkAuthentication == sub_type) {
        SAccountInfoUser *p_usr = NULL;
        TU32 return_id = 0;
        p_tlv = (STLV16HeaderBigEndian *) p_cur;
        tlv_type = ((TU16)p_tlv->type_high << 8) | (p_tlv->type_low);
        tlv_length = ((TU16)p_tlv->length_high << 8) | (p_tlv->length_low);
        p_cur += sizeof(STLV16HeaderBigEndian);
        //LOG_NOTICE("user login, tlv_type %x, tlv_length %d\n", tlv_type, tlv_length);
        if (DLikely(ETLV16Type_AccountID == tlv_type)) {
            if (DUnlikely(sizeof(TUniqueID) != tlv_length)) {
                replyToClient(socket, EECode_ServerReject_CorruptedProtocol, mpReadBuffer, mnHeaderLength);
                LOGM_ERROR("ETLV16Type_AccountID size(%d) not as expected\n", tlv_length);
                return EECode_ServerReject_CorruptedProtocol;
            }
            DBER64(id, p_cur);
            err = mpAccountManager->QueryAccount(id, p_account_info);
            if (DUnlikely(EECode_OK != err)) {
                replyToClient(socket, EECode_ServerReject_NoSuchChannel, mpReadBuffer, mnHeaderLength);
                LOGM_ERROR("QueryAccount fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                return EECode_NotExist;
            }
            p_usr = (SAccountInfoUser *) p_account_info;
            LOG_NOTICE("user id login [%llx], name [%s]\n", id, p_usr->root.base.name);
        } else if (ETLV16Type_AccountName == tlv_type) {
            if (DUnlikely((DMAX_ACCOUNT_NAME_LENGTH_EX + 1) < tlv_length)) {
                replyToClient(socket, EECode_ServerReject_CorruptedProtocol, mpReadBuffer, mnHeaderLength);
                LOGM_ERROR("ETLV16Type_AccountName size(%d) not as expected\n", tlv_length);
                return EECode_ServerReject_CorruptedProtocol;
            }
            TChar name[DMAX_ACCOUNT_NAME_LENGTH_EX + 4] = {0};
            memcpy(name, p_cur, tlv_length);
            err = mpAccountManager->QueryUserAccountByName(name, p_usr);
            if (DUnlikely(EECode_OK != err)) {
                replyToClient(socket, EECode_ServerReject_NoSuchChannel, mpReadBuffer, mnHeaderLength);
                LOGM_ERROR("QueryUserAccountByName(%s) fail, ret %d, %s\n", name, err, gfGetErrorCodeString(err));
                return EECode_NotExist;
            }
            p_account_info = (SAccountInfoRoot *) p_usr;
            return_id = 1;
            LOGM_NOTICE("user name login name [%s], password [%s], id [%llx]\n", p_usr->root.base.name, p_usr->root.base.password, p_usr->root.header.id);
        } else {
            replyToClient(socket, EECode_ServerReject_CorruptedProtocol, mpReadBuffer, mnHeaderLength);
            LOGM_ERROR("not ETLV16Type_AccountName or ETLV16Type_AccountID, %x\n", tlv_type);
            return EECode_ServerReject_CorruptedProtocol;
        }
        DASSERT(p_cur);

        TU8 tmp_buffer[64] = {0};
        gfGetCurrentDateTime((SDateTime *) tmp_buffer);
        tmp_buffer[sizeof(SDateTime)] = (tmp_buffer[7] ^ 0x31) ^ (0x54 ^ tmp_buffer[0]);
        tmp_buffer[sizeof(SDateTime) + 1] = (tmp_buffer[6] + 0xa4) ^ (0x12 ^ tmp_buffer[1]);
        tmp_buffer[sizeof(SDateTime) + 2] = (tmp_buffer[5] & 0xde) ^ (0x63 + tmp_buffer[2]);
        tmp_buffer[sizeof(SDateTime) + 3] = (tmp_buffer[4] | 0x3f) ^ (0x1d & tmp_buffer[3]);
        TU64 dynamic_input = gfHashAlgorithmV5(tmp_buffer, sizeof(SDateTime) + 4);
        p_cur = mpReadBuffer + sizeof(SSACPHeader) + sizeof(TUniqueID);

        p_tlv = (STLV16HeaderBigEndian *) p_cur;
        p_tlv->type_high = (ETLV16Type_DynamicInput >> 8) & 0xff;
        p_tlv->type_low = (ETLV16Type_DynamicInput) & 0xff;
        p_tlv->length_high = (DDynamicInputStringLength >> 8) & 0xff;
        p_tlv->length_low = DDynamicInputStringLength & 0xff;
        p_cur += sizeof(STLV16HeaderBigEndian);
        DBEW64(dynamic_input, p_cur);

        replyToClient(socket, EECode_OK, mpReadBuffer, sizeof(SSACPHeader) + sizeof(TUniqueID) + DDynamicInputStringLength + sizeof(STLV16HeaderBigEndian));
        DBEW64(dynamic_input, tmp_buffer);
        memcpy(tmp_buffer + DDynamicInputStringLength, p_usr->root.base.password, DIdentificationStringLength);
        TU32 hash_verify = gfHashAlgorithmV6(tmp_buffer, DDynamicInputStringLength + DIdentificationStringLength);

        bytesRead = gfNet_Recv_timeout(socket, mpReadBuffer, sizeof(SSACPHeader) + sizeof(TUniqueID) + sizeof(hash_verify) + sizeof(STLV16HeaderBigEndian), DNETUTILS_RECEIVE_FLAG_READ_ALL, 20);

        if (DUnlikely(((TInt)sizeof(SSACPHeader) + sizeof(TUniqueID) + sizeof(hash_verify) + sizeof(STLV16HeaderBigEndian)) != bytesRead)) {
            LOGM_WARN("ESACPClientCmdSubType_SinkAuthentication, recv hash %d not expected, expect %zu, time out?\n", bytesRead, sizeof(SSACPHeader) + sizeof(hash_verify));
            replyToClient(socket, EECode_ServerReject_TimeOut, mpReadBuffer, mnHeaderLength);
            return EECode_ServerReject_TimeOut;
        }
        p_header = (SSACPHeader *)mpReadBuffer;
        cur_size = ((TU32)p_header->size_0 << 16) | ((TU32)p_header->size_1 << 8) | (TU32)p_header->size_2;
        p_cur = mpReadBuffer + sizeof(SSACPHeader) + sizeof(TUniqueID);

        TU32 hash_recv = 0;
        p_tlv = (STLV16HeaderBigEndian *) p_cur;
        tlv_type = ((TU16)p_tlv->type_high << 8) | (p_tlv->type_low);
        tlv_length = ((TU16)p_tlv->length_high << 8) | (p_tlv->length_low);
        if (DUnlikely((ETLV16Type_AuthenticationResult32 != tlv_type) || (4 != tlv_length))) {
            LOGM_ERROR("ESACPClientCmdSubType_SinkAuthentication, recv ETLV16Type_AuthenticationResult32 fail, size(%d)/type(%x) not as expected\n", tlv_length, tlv_type);
            replyToClient(socket, EECode_ServerReject_CorruptedProtocol, mpReadBuffer, mnHeaderLength);
            return EECode_ServerReject_CorruptedProtocol;
        }

        p_cur += sizeof(STLV16HeaderBigEndian);
        DBER32(hash_recv, p_cur);
        if (hash_recv == hash_verify) {
            if (return_id) {
                p_cur = mpReadBuffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
                p_tlv = (STLV16HeaderBigEndian *) p_cur;
                p_tlv->type_high = (ETLV16Type_AccountID >> 8) & 0xff;
                p_tlv->type_low = (ETLV16Type_AccountID) & 0xff;
                p_tlv->length_high = 0;
                p_tlv->length_low = sizeof(TUniqueID);
                p_cur += sizeof(STLV16HeaderBigEndian);
                DBEW64(p_usr->root.header.id, p_cur);
                replyToClient(socket, EECode_OK, mpReadBuffer, sizeof(SSACPHeader) + sizeof(TUniqueID) + sizeof(TUniqueID) + sizeof(STLV16HeaderBigEndian));
            } else {
                replyToClient(socket, EECode_OK, mpReadBuffer, sizeof(SSACPHeader) + sizeof(TUniqueID));
            }
        } else {
            LOGM_ERROR("ESACPClientCmdSubType_SinkAuthentication fail, hash_recv=0x%x, hash_verify=0x%x\n", hash_recv, hash_verify);
            replyToClient(socket, EECode_ServerReject_AuthenticationFail, mpReadBuffer, mnHeaderLength);
            return EECode_ServerReject_AuthenticationFail;
        }

        return EECode_OK;
    } else if (ESACPClientCmdSubType_GetSecureQuestion == sub_type) {
        TChar name[DMAX_ACCOUNT_NAME_LENGTH_EX + 4] = {0};

        p_tlv = (STLV16HeaderBigEndian *) p_cur;
        tlv_type = ((TU16)p_tlv->type_high << 8) | (p_tlv->type_low);
        tlv_length = ((TU16)p_tlv->length_high << 8) | (p_tlv->length_low);
        p_cur += sizeof(STLV16HeaderBigEndian);
        if (DLikely(ETLV16Type_AccountName == tlv_type)) {
            if (DUnlikely((DMAX_ACCOUNT_NAME_LENGTH_EX + 1) < tlv_length)) {
                replyToClient(socket, EECode_ServerReject_CorruptedProtocol, mpReadBuffer, mnHeaderLength);
                LOGM_ERROR("ETLV16Type_AccountName size(%d) not as expected\n", tlv_length);
                return EECode_ServerReject_CorruptedProtocol;
            }
            memcpy(name, p_cur, tlv_length);
        } else {
            replyToClient(socket, EECode_ServerReject_CorruptedProtocol, mpReadBuffer, mnHeaderLength);
            LOGM_ERROR("not ETLV16Type_AccountName, %x\n", tlv_type);
            return EECode_ServerReject_CorruptedProtocol;
        }

        SAccountInfoUser *p_user = NULL;
        err = mpAccountManager->QueryUserAccountByName(name, p_user);
        if (DUnlikely(EECode_OK != err)) {
            replyToClient(socket, EECode_NotExist, mpReadBuffer, mnHeaderLength);
            LOGM_ERROR("no such user\n");
            return EECode_NotExist;
        } else {
            DASSERT(p_user);
            p_cur = mpReadBuffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
            cur_size = 0;

            p_tlv = (STLV16HeaderBigEndian *) p_cur;
            length = strlen(p_user->ext.secure_question1);
            DASSERT(length < DMAX_SECURE_QUESTION_LENGTH);
            if (DUnlikely(length >= DMAX_SECURE_QUESTION_LENGTH)) {
                length = DMAX_SECURE_QUESTION_LENGTH - 1;
            }
            p_tlv->type_high = (ETLV16Type_SecurityQuestion >> 8) & 0xff;
            p_tlv->type_low = (ETLV16Type_SecurityQuestion) & 0xff;
            p_tlv->length_high = ((length + 1) >> 8) & 0xff;
            p_tlv->length_low = (length + 1) & 0xff;
            p_cur += sizeof(STLV16HeaderBigEndian);
            cur_size += sizeof(STLV16HeaderBigEndian);
            memcpy(p_cur, p_user->ext.secure_question1, length);
            p_cur += length;
            *p_cur++ = 0x0;
            cur_size += length + 1;

            p_tlv = (STLV16HeaderBigEndian *) p_cur;
            length = strlen(p_user->ext.secure_question2);
            DASSERT(length < DMAX_SECURE_QUESTION_LENGTH);
            if (DUnlikely(length >= DMAX_SECURE_QUESTION_LENGTH)) {
                length = DMAX_SECURE_QUESTION_LENGTH - 1;
            }
            p_tlv->type_high = (ETLV16Type_SecurityQuestion >> 8) & 0xff;
            p_tlv->type_low = (ETLV16Type_SecurityQuestion) & 0xff;
            p_tlv->length_high = ((length + 1) >> 8) & 0xff;
            p_tlv->length_low = (length + 1) & 0xff;
            p_cur += sizeof(STLV16HeaderBigEndian);
            cur_size += sizeof(STLV16HeaderBigEndian);
            memcpy(p_cur, p_user->ext.secure_question2, length);
            p_cur += length;
            *p_cur++ = 0x0;
            cur_size += length + 1;

            gfSocketSetTimeout(socket, 3, 0);

            LOG_PRINTF("[%s, %llx] query secure question (%s, %s)\n", name, p_user->root.header.id, p_user->ext.secure_question1, p_user->ext.secure_question2);
            replyToClient(socket, EECode_OK, mpReadBuffer, sizeof(SSACPHeader) + sizeof(TUniqueID) + cur_size);
        }
        return EECode_OK_IsolateAccess;
    } else if (ESACPClientCmdSubType_ResetPassword == sub_type) {
        TChar name[DMAX_ACCOUNT_NAME_LENGTH_EX + 4] = {0};

        p_tlv = (STLV16HeaderBigEndian *) p_cur;
        tlv_type = ((TU16)p_tlv->type_high << 8) | (p_tlv->type_low);
        tlv_length = ((TU16)p_tlv->length_high << 8) | (p_tlv->length_low);
        p_cur += sizeof(STLV16HeaderBigEndian);
        if (DLikely(ETLV16Type_AccountName == tlv_type)) {
            if (DUnlikely((DMAX_ACCOUNT_NAME_LENGTH_EX + 1) < tlv_length)) {
                replyToClient(socket, EECode_ServerReject_CorruptedProtocol, mpReadBuffer, mnHeaderLength);
                LOGM_ERROR("ETLV16Type_AccountName size(%d) not as expected\n", tlv_length);
                return EECode_ServerReject_CorruptedProtocol;
            }
            memcpy(name, p_cur, tlv_length);
        } else {
            replyToClient(socket, EECode_ServerReject_CorruptedProtocol, mpReadBuffer, mnHeaderLength);
            LOGM_ERROR("not ETLV16Type_AccountName, %x\n", tlv_type);
            return EECode_ServerReject_CorruptedProtocol;
        }
        p_cur += tlv_length;

        SAccountInfoUser *p_user = NULL;
        err = mpAccountManager->QueryUserAccountByName(name, p_user);
        if (DUnlikely(EECode_OK != err)) {
            replyToClient(socket, EECode_NotExist, mpReadBuffer, mnHeaderLength);
            LOGM_ERROR("no such user\n");
            return EECode_NotExist;
        } else {
            DASSERT(p_user);
            TChar new_password[DIdentificationStringLength + 4] = {0};

            p_tlv = (STLV16HeaderBigEndian *) p_cur;
            tlv_type = ((TU16)p_tlv->type_high << 8) | (p_tlv->type_low);
            tlv_length = ((TU16)p_tlv->length_high << 8) | (p_tlv->length_low);
            p_cur += sizeof(STLV16HeaderBigEndian);
            if (DLikely(ETLV16Type_AccountPassword == tlv_type)) {
                if (DUnlikely((DIdentificationStringLength + 1) < tlv_length)) {
                    replyToClient(socket, EECode_ServerReject_CorruptedProtocol, mpReadBuffer, mnHeaderLength);
                    LOGM_ERROR("ETLV16Type_AccountPassword size(%d) not as expected\n", tlv_length);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                memcpy(new_password, p_cur, tlv_length);
            } else {
                replyToClient(socket, EECode_ServerReject_CorruptedProtocol, mpReadBuffer, mnHeaderLength);
                LOGM_ERROR("not ETLV16Type_AccountName, %x\n", tlv_type);
                return EECode_ServerReject_CorruptedProtocol;
            }

            p_cur = mpReadBuffer + sizeof(SSACPHeader) + sizeof(TUniqueID);

            p_tlv = (STLV16HeaderBigEndian *) p_cur;
            tlv_type = ((TU16)p_tlv->type_high << 8) | (p_tlv->type_low);
            tlv_length = ((TU16)p_tlv->length_high << 8) | (p_tlv->length_low);
            p_cur += sizeof(STLV16HeaderBigEndian);

            length = strlen(p_user->ext.secure_answer1);
            if (DUnlikely(length != ((TU32)tlv_length + 1))) {
                replyToClient(socket, EECode_ServerReject_AuthenticationFail, mpReadBuffer, mnHeaderLength);
                LOGM_ERROR("answer1 length wrong, [%s], [%s]\n", p_cur, p_user->ext.secure_answer1);
                return EECode_ServerReject_AuthenticationFail;
            }

            if (DUnlikely(strcmp((TChar *)p_cur, p_user->ext.secure_answer1))) {
                replyToClient(socket, EECode_ServerReject_AuthenticationFail, mpReadBuffer, mnHeaderLength);
                LOGM_ERROR("answer1 wrong, [%s], [%s]\n", p_cur, p_user->ext.secure_answer1);
                return EECode_ServerReject_AuthenticationFail;
            }
            p_cur += tlv_length;

            p_tlv = (STLV16HeaderBigEndian *) p_cur;
            tlv_type = ((TU16)p_tlv->type_high << 8) | (p_tlv->type_low);
            tlv_length = ((TU16)p_tlv->length_high << 8) | (p_tlv->length_low);
            p_cur += sizeof(STLV16HeaderBigEndian);

            length = strlen(p_user->ext.secure_answer2);
            if (DUnlikely(length != ((TU32)tlv_length + 1))) {
                replyToClient(socket, EECode_ServerReject_AuthenticationFail, mpReadBuffer, mnHeaderLength);
                LOGM_ERROR("answer2 length wrong, [%s], [%s]\n", p_cur, p_user->ext.secure_answer2);
                return EECode_ServerReject_AuthenticationFail;
            }

            if (DUnlikely(strcmp((TChar *)p_cur, p_user->ext.secure_answer2))) {
                replyToClient(socket, EECode_ServerReject_AuthenticationFail, mpReadBuffer, mnHeaderLength);
                LOGM_ERROR("answer1 wrong, [%s], [%s]\n", p_cur, p_user->ext.secure_answer2);
                return EECode_ServerReject_AuthenticationFail;
            }
            p_cur += tlv_length;

            memcpy(p_user->root.base.password, new_password, DIdentificationStringLength);

            gfSocketSetTimeout(socket, 3, 0);

            LOG_PRINTF("[%s, %llx] reset password done (%s)\n", name, p_user->root.header.id, new_password);

            p_cur = mpReadBuffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
            p_tlv = (STLV16HeaderBigEndian *) p_cur;
            p_tlv->type_high = (ETLV16Type_AccountID >> 8) & 0xff;
            p_tlv->type_low = (ETLV16Type_AccountID) & 0xff;
            p_tlv->length_high = 0;
            p_tlv->length_low = sizeof(TUniqueID);
            p_cur += sizeof(STLV16HeaderBigEndian);

            replyToClient(socket, EECode_OK, mpReadBuffer, sizeof(SSACPHeader) + sizeof(TUniqueID) + sizeof(TUniqueID) + sizeof(STLV16HeaderBigEndian));
        }
        return EECode_OK_IsolateAccess;
    } else if (ESACPClientCmdSubType_SinkUnRegister == sub_type) {
        LOGM_ERROR("not supported\n");
        replyToClient(socket, EECode_ServerReject_NotSupported, mpReadBuffer, mnHeaderLength);
        return EECode_NotSupported;
    } else {
        LOGM_ERROR("unknown cmd: 0x%08x\n", sub_type);
        return EECode_Error;
    }
}

CIMServer::CIMServer(const TChar *name, EProtocolType type, const volatile SPersistCommonConfig *pPersistCommonConfig, IMsgSink *pMsgSink, IAccountManager *pAccountManager, TU16 server_port)
    : inherited(name, 0)
    , mpPersistCommonConfig(pPersistCommonConfig)
    , mpMsgSink(pMsgSink)
    , mpAccountManager(pAccountManager)
    , mpSystemClockReference(NULL)
    , mpMutex(NULL)
    , mpThreadPool(NULL)
    , mpSimpleScheduler(NULL)
    , mnThreadNum(4)
    , mpProtocolHeaderParser(NULL)
    , mpReadBuffer(NULL)
    , mListeningPort(server_port)
    , msState(EServerState_closed)
    , mSocket(-1)
{

}

CIMServer::~CIMServer()
{
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }
}

IIMServer *CIMServer::Create(const TChar *name, EProtocolType type, const volatile SPersistCommonConfig *pPersistCommonConfig, IMsgSink *pMsgSink, IAccountManager *pAccountManager, TU16 server_port)
{
    CIMServer *result = new CIMServer(name, type, pPersistCommonConfig, pMsgSink, pAccountManager, server_port);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

EECode CIMServer::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleCloudServer);
    //mConfigLogLevel = ELogLevel_Debug;
    //mConfigLogOption = 0xffffffff;

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
    LOGM_NOTICE("gfNet_SetupStreamSocket, socket %d, port %d.\n", mSocket, mListeningPort);

    //TInt val = 2;
    //setsockopt(mSocket, IPPROTO_TCP, TCP_DEFER_ACCEPT, &val, sizeof(val));

#ifndef BUILD_OS_WINDOWS
    mpThreadPool = CThreadPool::Create("im_server", 0, mnThreadNum);
    if (DUnlikely(EECode_OK != mpThreadPool->StartRunning(mnThreadNum))) {
        LOGM_FATAL("Thread Pool StartRunning fail.\n");
        return EECode_Error;
    }
#else
    mpSimpleScheduler = gfSchedulerFactory(ESchedulerType_Preemptive, 0);
    if (DUnlikely(!mpSimpleScheduler)) {
        LOGM_FATAL("gfSchedulerFactory() fail.\n");
        return EECode_Error;
    }
    if (DUnlikely(EECode_OK != mpSimpleScheduler->StartScheduling())) {
        LOGM_FATAL("StartScheduling() fail.\n");
        return EECode_Error;
    }
#endif

    mpProtocolHeaderParser = gfCreatePotocolHeaderParser(EProtocolType_SACP, EProtocolHeaderExtentionType_SACP_IM);
    if (DUnlikely(!mpProtocolHeaderParser)) {
        LOGM_FATAL("gfCreatePotocolHeaderParser fail.\n");
        return EECode_Error;
    }
    mnHeaderLength = mpProtocolHeaderParser->GetFixedHeaderLength();

    mpReadBuffer = (TU8 *)malloc(DSACP_MAX_PAYLOAD_LENGTH);
    if (DUnlikely(!mpReadBuffer)) {
        LOGM_FATAL("malloc mpReadBuffer fail.\n");
        return EECode_NoMemory;
    }
    mnMaxPayloadLength = DSACP_MAX_PAYLOAD_LENGTH;

    msState = EServerState_running;

    return EECode_OK;
}

void CIMServer::Delete()
{
    if (mpThreadPool) {
        mpThreadPool->Delete();
        mpThreadPool->StopRunning();
        mpThreadPool = NULL;
    }

    if (mpSimpleScheduler) {
        mpSimpleScheduler->StopScheduling();
        mpSimpleScheduler->GetObject0()->Delete();
        mpSimpleScheduler = NULL;
    }

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    if (mpProtocolHeaderParser) {
        mpProtocolHeaderParser->Destroy();
        mpProtocolHeaderParser = NULL;
    }

    if (mpReadBuffer) {
        free(mpReadBuffer);
        mpReadBuffer = NULL;
    }

    delete this;

}

void CIMServer::Destroy()
{
    Delete();
}

void CIMServer::PrintStatus()
{
    LOGM_WARN("CIMServer::PrintStatus: TO DO\n");
}

EECode CIMServer::GetHandler(TInt &handle) const
{
    handle = mSocket;
    return EECode_OK;
}

EECode CIMServer::GetServerState(EServerState &state) const
{
    state = (EServerState)msState;
    return EECode_OK;
}

EECode CIMServer::SetServerState(EServerState state)
{
    msState = (EServerState)state;
    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


