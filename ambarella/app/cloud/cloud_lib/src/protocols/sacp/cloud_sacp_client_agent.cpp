/*******************************************************************************
 * cloud_sacp_client_agent.cpp
 *
 * History:
 *  2013/11/28 - [Zhi He] create file
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

#include "sacp_types.h"
#include "cloud_sacp_client_agent.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

ICloudClientAgent *gfCreateSACPCloudClientAgent(TU16 local_port, TU16 server_port)
{
    CSACPCloudClientAgent *thiz = CSACPCloudClientAgent::Create(local_port, server_port);
    return thiz;
}

//-----------------------------------------------------------------------
//
//  CSACPCloudClientAgent
//
//-----------------------------------------------------------------------

CSACPCloudClientAgent *CSACPCloudClientAgent::Create(TU16 local_port, TU16 server_port)
{
    CSACPCloudClientAgent *result = new CSACPCloudClientAgent(local_port, server_port);
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

EECode CSACPCloudClientAgent::Construct()
{
    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOG_FATAL("no memory, gfCreateMutex() fail\n");
        return EECode_NoMemory;
    }

    mSenderBufferSize = DSACP_MAX_PAYLOAD_LENGTH;
    mReservedBufferSize = mSenderBufferSize;
    mpSenderBuffer = (TU8 *) DDBG_MALLOC(mSenderBufferSize + mReservedBufferSize, "CASB");

    if (DUnlikely(!mpSenderBuffer)) {
        LOG_FATAL("no memory, request %ld\n", mSenderBufferSize);
        mSenderBufferSize = 0;
        return EECode_NoMemory;
    }
    mpReservedBuffer = mpSenderBuffer + mSenderBufferSize;

    mpSACPHeader = (SSACPHeader *)mpSenderBuffer;

    mVersion.native_major = DCloudLibVesionMajor;
    mVersion.native_minor = DCloudLibVesionMinor;
    mVersion.native_patch = DCloudLibVesionPatch;
    mVersion.native_date_year = DCloudLibVesionYear;
    mVersion.native_date_month = DCloudLibVesionMonth;
    mVersion.native_date_day = DCloudLibVesionDay;

    return EECode_OK;
}

CSACPCloudClientAgent::CSACPCloudClientAgent(TU16 local_port, TU16 server_port)
    : mLocalPort(local_port)
    , mServerPort(server_port)
    , mbConnected(0)
    , mbAuthenticated(0)
    , mbGetPeerVersion(0)
    , mSocket(-1)
    , mSeqCount(0)
    , mbReceiveConnectStatus(1)
    , mHeaderExtentionType(EProtocolHeaderExtentionType_NoHeader)
    , mEncryptionType1(EEncryptionType_None)
    , mEncryptionType2(EEncryptionType_None)
    , mpReservedBuffer(NULL)
    , mReservedDataSize(0)
    , mReservedBufferSize(0)
    , mpSenderBuffer(NULL)
    , mSenderBufferSize(0)
    , mpSACPHeader(NULL)
    , mLastErrorHint1(0)
    , mLastErrorHint2(0)
    , mLastErrorCode(EECode_OK)
{
}

CSACPCloudClientAgent::~CSACPCloudClientAgent()
{
    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    if (mpSenderBuffer) {
        DDBG_FREE(mpSenderBuffer, "CASB");
        mpSenderBuffer = NULL;
    }

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    mSenderBufferSize = 0;
    mpSACPHeader = NULL;
}

void CSACPCloudClientAgent::fillHeader(TU32 type, TU32 size)
{
    mpSACPHeader->type_1 = (type >> 24) & 0xff;
    mpSACPHeader->type_2 = (type >> 16) & 0xff;
    mpSACPHeader->type_3 = (type >> 8) & 0xff;
    mpSACPHeader->type_4 = type & 0xff;

    mpSACPHeader->size_1 = (size >> 8) & 0xff;
    mpSACPHeader->size_2 = size & 0xff;
    mpSACPHeader->size_0 = (size >> 16) & 0xff;

    mpSACPHeader->seq_count_0 = (mSeqCount >> 16) & 0xff;
    mpSACPHeader->seq_count_1 = (mSeqCount >> 8) & 0xff;
    mpSACPHeader->seq_count_2 = mSeqCount & 0xff;
    mSeqCount ++;

    mpSACPHeader->encryption_type_1 = mEncryptionType1;
    mpSACPHeader->encryption_type_2 = mEncryptionType2;

    mpSACPHeader->flags = 0;
    mpSACPHeader->header_ext_type = mHeaderExtentionType;
    mpSACPHeader->header_ext_size_1 = 0;
    mpSACPHeader->header_ext_size_2 = 0;
}

EECode CSACPCloudClientAgent::ConnectToServer(TChar *server_url, TU64 &hardware_authentication_input, TU16 local_port, TU16 server_port, TU8 *authentication_buffer, TU16 authentication_length)
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ConnectionTag, 0);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    if (DUnlikely(!server_url)) {
        LOG_ERROR("NULL server_url\n");
        return EECode_BadParam;
    } else {
        LOG_NOTICE("server_url %s, local_port %hu, server_port %hu, default local port %hu, server port %hu\n", server_url, local_port, server_port, mLocalPort, mServerPort);
    }

    if (DUnlikely(!authentication_buffer)) {
        LOG_ERROR("NULL authentication_buffer\n");
        return EECode_BadParam;
    }

    if (local_port) {
        mLocalPort = local_port;
        LOG_NOTICE("specify local_port %hu\n", local_port);
    }

    if (server_port) {
        mServerPort = server_port;
        LOG_NOTICE("specify server_port %hu\n", server_port);
    }

    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    mSocket = gfNet_ConnectTo((const TChar *)server_url, mServerPort, SOCK_STREAM, IPPROTO_TCP);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("gfNet_ConnectTo(%s:%hu) fail\n", server_url, server_port);
        mLastErrorCode = EECode_NetConnectFail;
        mLastErrorHint1 = mSocket;
        mLastErrorHint2 = 0;
        return EECode_NetConnectFail;
    }

    fillHeader(type, authentication_length);
    if (mbReceiveConnectStatus) {
        mpSACPHeader->flags |= DSACPHeaderFlagBit_ReplyResult;
    }
    memcpy(p_payload, authentication_buffer, authentication_length);

    gfSocketSetTimeout(mSocket, 2, 0);

    ret = gfNet_Send_timeout(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + authentication_length), 0, 1);
    if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + authentication_length))) {
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = ret;
        mLastErrorHint2 = sizeof(SSACPHeader) + authentication_length;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_NetSendHeader_Fail;
    }

    if (mbReceiveConnectStatus) {
        ret = gfNet_Recv_timeout(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 2;
            mLastErrorHint2 = sizeof(SSACPHeader);

            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;

            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = ret;
            mLastErrorHint2 = sizeof(SSACPHeader);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetReceiveHeader_Fail;
        }

        if (DLikely(ESACPConnectResult_OK == mpSACPHeader->flags)) {
            mbConnected = 1;
            return EECode_OK;
        } else if (ESACPRequestResult_Server_NeedHardwareAuthenticate == mpSACPHeader->flags) {
            mbConnected = 1;
            //continue to receive hardware_authentication_input
            ret = gfNet_Recv_timeout(mSocket, mpSenderBuffer, DDynamicInputStringLength, DNETUTILS_RECEIVE_FLAG_READ_ALL, 20);
            if (DUnlikely(DDynamicInputStringLength != ret)) {
                LOG_WARN("ConnectToServer, recv dynamic_input %d not expected, expect %d, peer closed?\n", ret, DDynamicInputStringLength);
                mLastErrorCode = EECode_NetReceivePayload_Fail;
                mLastErrorHint1 = ret;
                mLastErrorHint2 = DDynamicInputStringLength;
                gfNetCloseSocket(mSocket);
                mSocket = DInvalidSocketHandler;
                return EECode_NetReceivePayload_Fail;
            }
            hardware_authentication_input = (((TU64)(mpSenderBuffer[0])) << 56) | (((TU64)(mpSenderBuffer[1])) << 48) | (((TU64)(mpSenderBuffer[2])) << 40) | (((TU64)(mpSenderBuffer[3])) << 32) | \
                                            (((TU64)(mpSenderBuffer[4])) << 24) | (((TU64)(mpSenderBuffer[5])) << 16) | (((TU64)(mpSenderBuffer[6])) << 8) | (TU64)(mpSenderBuffer[7]);
            return EECode_OK_NeedHardwareAuthenticate;
        } else if (ESACPConnectResult_Reject_NoSuchChannel == mpSACPHeader->flags) {
            LOG_ERROR("ESACPConnectResult_Reject_NoSuchChannel, %s\n", (TChar *)authentication_buffer);
            mbConnected = 0;
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_ServerReject_NoSuchChannel;
        } else if (ESACPConnectResult_Reject_ChannelIsInUse == mpSACPHeader->flags) {
            LOG_ERROR("ESACPConnectResult_Reject_ChannelIsInUse, %s\n", (TChar *)authentication_buffer);
            mbConnected = 0;
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_ServerReject_ChannelIsBusy;
        } else if (ESACPConnectResult_Reject_BadRequestFormat == mpSACPHeader->flags) {
            LOG_ERROR("ESACPConnectResult_Reject_BadRequestFormat, %s\n", (TChar *)authentication_buffer);
            mbConnected = 0;
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_ServerReject_BadRequestFormat;
        } else if (ESACPConnectResult_Reject_CorruptedProtocol == mpSACPHeader->flags) {
            LOG_ERROR("ESACPConnectResult_Reject_CorruptedProtocol, %s\n", (TChar *)authentication_buffer);
            mbConnected = 0;
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_ServerReject_CorruptedProtocol;
        } else {
            LOG_ERROR("unknown error, %s, mpSACPHeader->flags %08x\n", (TChar *)authentication_buffer, mpSACPHeader->flags);
            mbConnected = 0;
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_ServerReject_Unknown;
        }
    }

    mbConnected = 1;
    return EECode_OK;
}

#if 0
EECode CSACPCloudClientAgent::HardwareAuthenticate(TU32 hash_value)
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SourceReAuthentication);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    fillHeader(type, 4);
    if (mbReceiveConnectStatus) {
        mpSACPHeader->flags |= DSACPHeaderFlagBit_ReplyResult;
    }

    p_payload[0] = (hash_value >> 24) & 0xff;
    p_payload[1] = (hash_value >> 16) & 0xff;
    p_payload[2] = (hash_value >> 8) & 0xff;
    p_payload[3] = (hash_value) & 0xff;

    ret = gfNet_Send_timeout(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + 4), 0, 2);
    if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 4))) {
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = ret;
        mLastErrorHint2 = sizeof(SSACPHeader) + 4;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_NetSendHeader_Fail;
    }

    ret = gfNet_Recv_timeout(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = sizeof(SSACPHeader);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
        LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
        mLastErrorCode = EECode_NetReceiveHeader_Fail;
        mLastErrorHint1 = ret;
        mLastErrorHint2 = sizeof(SSACPHeader);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_NetReceiveHeader_Fail;
    }

    if (DLikely(ESACPConnectResult_OK == mpSACPHeader->flags)) {
        mbConnected = 1;
        LOG_NOTICE("hardware authentication success\n");
        return EECode_OK;
    } else {
        LOG_ERROR("unknown error, mpSACPHeader->flags %08x\n", mpSACPHeader->flags);
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_Unknown;
    }

    return EECode_OK;
}
#endif

EECode CSACPCloudClientAgent::DisconnectToServer()
{
    AUTO_LOCK(mpMutex);

    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    mbConnected = 0;

    return EECode_OK;
}

EECode CSACPCloudClientAgent::Uploading(TU8 *p_data, TMemSize size, ESACPDataChannelSubType data_type, TU8 extra_flag)
{
    TU32 type = DBuildSACPType(0, ESACPTypeCategory_DataChannel, (TU16)data_type);
    TInt ret = 0;
    TMemSize transfer_size = size;

    if (DUnlikely(!p_data || !size)) {
        LOG_ERROR("BAD params %p, size %ld\n", p_data, size);
        mLastErrorCode = EECode_BadParam;
        mLastErrorHint1 = (TULong)p_data;
        mLastErrorHint2 = (TU32)size;
        return EECode_BadParam;
    }

    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = EECodeHint_BadSocket;
        mLastErrorHint2 = (TU32)mSocket;
        return EECode_BadState;
    }

    mpSACPHeader->type_1 = (type >> 24) & 0xff;
    mpSACPHeader->type_2 = (type >> 16) & 0xff;
    mpSACPHeader->type_3 = (type >> 8) & 0xff;
    mpSACPHeader->type_4 = type & 0xff;

    mpSACPHeader->encryption_type_1 = mEncryptionType1;
    mpSACPHeader->encryption_type_2 = mEncryptionType2;

    //mpSACPHeader->flags = 0;
    mpSACPHeader->header_ext_type = mHeaderExtentionType;
    mpSACPHeader->header_ext_size_1 = 0;
    mpSACPHeader->header_ext_size_2 = 0;

    mpSACPHeader->flags = DSACPHeaderFlagBit_PacketStartIndicator | extra_flag;

    do {

        if ((mSenderBufferSize - sizeof(SSACPHeader)) >= size) {
            transfer_size = size;
            size = 0;
            mpSACPHeader->flags = (TU8)(mpSACPHeader->flags | DSACPHeaderFlagBit_PacketEndIndicator);
        } else {
            transfer_size = mSenderBufferSize - sizeof(SSACPHeader);
            size -= transfer_size;
        }

        mpSACPHeader->size_1 = (transfer_size >> 8) & 0xff;
        mpSACPHeader->size_2 = transfer_size & 0xff;
        mpSACPHeader->size_0 = (transfer_size >> 16) & 0xff;

        mpSACPHeader->seq_count_0 = (mSeqCount >> 16) & 0xff;
        mpSACPHeader->seq_count_1 = (mSeqCount >> 8) & 0xff;
        mpSACPHeader->seq_count_2 = mSeqCount & 0xff;
        mSeqCount ++;

        ret = gfNet_Send_timeout(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), 0, 10);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 5;
            mLastErrorHint2 = (TU32)mSocket;

            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;

            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("ret not expected %d, sizeof(SSACPHeader) %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetSendHeader_Fail;
            mLastErrorHint1 = ret;
            mLastErrorHint2 = (TU32)mSocket;

            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetSendHeader_Fail;
        }

        ret = gfNet_Send_timeout(mSocket, p_data, (TInt)(transfer_size), 0, 10);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 6;
            mLastErrorHint2 = (TU32)mSocket;

            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(transfer_size))) {
            LOG_ERROR("ret not expected %d, transfer_size %ld\n", ret, transfer_size);
            mLastErrorCode = EECode_NetSendPayload_Fail;
            mLastErrorHint1 = ret;
            mLastErrorHint2 = (TU32)mSocket;

            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetSendPayload_Fail;
        }
        p_data += transfer_size;

        mpSACPHeader->flags &= ~(DSACPHeaderFlagBit_PacketStartIndicator);

    } while (size);

    return EECode_OK;
}

EECode CSACPCloudClientAgent::GetLastErrorHint(EECode &last_error_code, TU32 &error_hint1, TU32 &error_hint2)
{
    AUTO_LOCK(mpMutex);

    last_error_code = mLastErrorCode;
    error_hint1 = mLastErrorHint1;
    error_hint2 = mLastErrorHint2;

    return EECode_OK;
}

EECode CSACPCloudClientAgent::DemandIDR(TChar *source_channel, TU32 demand_param, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkForceIDR);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 4);

    DBEW32(demand_param, p_payload);

    LOG_PRINTF("DemandIDR, param %d\n", demand_param);
    ret = gfNet_Send(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + 4), 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 4;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 4))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 4);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::UpdateSourceBitrate(TChar *source_channel, TU32 bitrate, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkUpdateBitrate);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 4);

    DBEW32(bitrate, p_payload);

    LOG_PRINTF("UpdateSourceBitrate, bitrate %d\n", bitrate);
    ret = gfNet_Send(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + 4), 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 4;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 4))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 4);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::UpdateSourceFramerate(TChar *source_channel, TU32 framerate, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkUpdateFramerate);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 4);

    DBEW32(framerate, p_payload);

    LOG_PRINTF("UpdateSourceFramerate, framerate %d\n", framerate);

    ret = gfNet_Send(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + 4), 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 4;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 4))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 4);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::UpdateSourceDisplayLayout(TChar *source_channel, TU32 layout, TU32 focus_index, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkUpdateDisplayLayout);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 8);

    DBEW32(layout, p_payload);
    p_payload += 4;
    DBEW32(focus_index, p_payload);

    LOG_PRINTF("UpdateSourceDisplayLayout, layout %d, focus_index %d\n", layout, focus_index);

    ret = gfNet_Send(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + 8), 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 8;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 8))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 8);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::UpdateSourceAudioParams(TU8 audio_format, TU8 audio_channnel_number, TU32 audio_sample_frequency, TU32 audio_bitrate, TU8 *audio_extradata, TU16 audio_extradata_size, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SourceUpdateAudioParams);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt payload_len = 0;
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    //all params use TLV16
    DBEW16(ETLV16Type_DeviceAudioParams_Format, p_payload);
    p_payload += 2;
    DBEW16(1, p_payload);
    p_payload += 2;
    p_payload[0] = audio_format;
    p_payload += 1;

    DBEW16(ETLV16Type_DeviceAudioParams_ChannelNum, p_payload);
    p_payload += 2;
    DBEW16(1, p_payload);
    p_payload += 2;
    p_payload[0] = audio_channnel_number;
    p_payload += 1;

    DBEW16(ETLV16Type_DeviceAudioParams_SampleFrequency, p_payload);
    p_payload += 2;
    DBEW16(4, p_payload);
    p_payload += 2;
    DBEW32(audio_sample_frequency, p_payload);
    p_payload += 4;

    DBEW16(ETLV16Type_DeviceAudioParams_Bitrate, p_payload);
    p_payload += 2;
    DBEW16(4, p_payload);
    p_payload += 2;
    DBEW32(audio_bitrate, p_payload);
    p_payload += 4;

    DBEW16(ETLV16Type_DeviceAudioParams_ExtraData, p_payload);
    p_payload += 2;
    DBEW16(audio_extradata_size, p_payload);
    p_payload += 2;
    memcpy(p_payload, audio_extradata, audio_extradata_size);
    p_payload += audio_extradata_size;

    payload_len = 30 + audio_extradata_size;
    DASSERT((TInt)(p_payload - mpSenderBuffer - sizeof(SSACPHeader)) == payload_len);

    fillHeader(type, payload_len);

    ret = gfNet_Send(mSocket, mpSenderBuffer, sizeof(SSACPHeader) + payload_len, 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = (TU32)payload_len;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + payload_len))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %zu\n", ret, sizeof(SSACPHeader) + payload_len);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %zu\n", ret, sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::SwapWindowContent(TChar *source_channel, TU32 window_id_1, TU32 window_id_2, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkSwapWindowContent);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 8);

    DBEW32(window_id_1, p_payload);
    p_payload += 4;
    DBEW32(window_id_2, p_payload);
    p_payload += 4;

    LOG_PRINTF("SwapWindowContent, window_id_1 %d, window_id_2 %d\n", window_id_1, window_id_2);

    ret = gfNet_Send(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + 8), 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 8;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 8))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 8);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::CircularShiftWindowContent(TChar *source_channel, TU32 shift_count, TU32 is_ccw, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkCircularShiftWindowContent);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 8);

    DBEW32(shift_count, p_payload);
    p_payload += 4;
    DBEW32(is_ccw, p_payload);
    p_payload += 4;

    LOG_PRINTF("CircularShiftWindowContent, shift_count %d, is_ccw %d\n", shift_count, is_ccw);

    ret = gfNet_Send(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + 8), 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 8;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 8))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 8);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::SwitchVideoHDSource(TChar *source_channel, TU32 hd_index, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkSelectVideoHDChannel);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 4);

    DBEW32(hd_index, p_payload);

    LOG_PRINTF("SwitchVideoHDSource, hd_index %d\n", hd_index);

    ret = gfNet_Send(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + 4), 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 4;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 4))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 4);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::SwitchAudioSourceIndex(TChar *source_channel, TU32 audio_index, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkSelectAudioSourceChannel);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 4);

    DBEW32(audio_index, p_payload);

    ret = sizeof(SSACPHeader) + 4;
    LOG_PRINTF("SwitchAudioSourceIndex %d, send %d start\n", audio_index, ret);
    ret = gfNet_Send(mSocket, mpSenderBuffer, ret, 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 4;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 4))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 4);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::SwitchAudioTargetIndex(TChar *source_channel, TU32 audio_index, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkSelectAudioTargetChannel);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 4);

    DBEW32(audio_index, p_payload);

    LOG_PRINTF("SwitchAudioTargetIndex, hd_index %d\n", audio_index);

    ret = gfNet_Send(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + 4), 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 4;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 4))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 4);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::ZoomSource(TChar *source_channel, TU32 window_index, float zoom_factor, float zoom_offset_center_x, float zoom_offset_center_y, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkZoom);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    if (0 > zoom_factor) {
        LOG_ERROR("BAD zoom_factor %f\n", zoom_factor);
        return EECode_BadParam;
    }

    fillHeader(type, 16);

    DBEW32(window_index, p_payload);
    p_payload += 4;

    TU32 value = zoom_factor * DSACP_FIX_POINT_DEN;
    DBEW32(value, p_payload);
    p_payload += 4;

    if (zoom_offset_center_x < 0) {
        value = (0 - zoom_offset_center_x) * DSACP_FIX_POINT_DEN;
        value |= DSACP_FIX_POINT_SYGN_FLAG;
    } else {
        value = (zoom_offset_center_x) * DSACP_FIX_POINT_DEN;
    }
    DBEW32(value, p_payload);
    p_payload += 4;

    if (zoom_offset_center_y < 0) {
        value = (0 - zoom_offset_center_y) * DSACP_FIX_POINT_DEN;
        value |= DSACP_FIX_POINT_SYGN_FLAG;
    } else {
        value = (zoom_offset_center_y) * DSACP_FIX_POINT_DEN;
    }
    DBEW32(value, p_payload);
    p_payload += 4;

    LOG_PRINTF("ZoomSource, window_index %d, zoom_factor %f, zoom_input_center_x %f, zoom_input_center_y %f\n", window_index, zoom_factor, zoom_offset_center_x, zoom_offset_center_y);

    ret = gfNet_Send(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + 16), 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 16;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 16))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 16);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::ZoomSource2(TChar *source_channel, TU32 window_index, float width_factor, float height_factor, float input_center_x, float input_center_y, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkZoom);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    if (DUnlikely((0 > width_factor) || (0 > height_factor))) {
        LOG_ERROR("BAD width_factor %f, height_factor %f\n", width_factor, height_factor);
        return EECode_BadParam;
    }

    if (DUnlikely((0 > input_center_x) || (0 > input_center_y))) {
        LOG_ERROR("BAD input_center_x %f, input_center_y %f\n", input_center_x, input_center_y);
        return EECode_BadParam;
    }

    fillHeader(type, 20);

    DBEW32(window_index, p_payload);
    p_payload += 4;

    TU32 value = width_factor * DSACP_FIX_POINT_DEN;
    DBEW32(value, p_payload);
    p_payload += 4;

    value =  height_factor * DSACP_FIX_POINT_DEN;
    DBEW32(value, p_payload);
    p_payload += 4;

    value =  input_center_x * DSACP_FIX_POINT_DEN;
    DBEW32(value, p_payload);
    p_payload += 4;

    value =  input_center_y * DSACP_FIX_POINT_DEN;
    DBEW32(value, p_payload);
    p_payload += 4;

    LOG_PRINTF("ZoomSource, window_index %d, width_factor %f, height_factor %f, input_center_x %f, input_center_y %f\n", window_index, width_factor, height_factor, input_center_x, input_center_y);

    ret = gfNet_Send(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + 20), 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 20;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 20))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 20);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::UpdateSourceWindow(TChar *source_channel, TU32 window_index, float pos_x, float pos_y, float size_x, float size_y, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkUpdateDisplayWindow);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 20);

    DBEW32(window_index, p_payload);
    p_payload += 4;

    TU32 value = pos_x * DSACP_FIX_POINT_DEN;
    DBEW32(value, p_payload);
    p_payload += 4;

    value = pos_y * DSACP_FIX_POINT_DEN;
    DBEW32(value, p_payload);
    p_payload += 4;

    value = size_x * DSACP_FIX_POINT_DEN;
    DBEW32(value, p_payload);
    p_payload += 4;

    value = size_y * DSACP_FIX_POINT_DEN;
    DBEW32(value, p_payload);
    p_payload += 4;

    LOG_PRINTF("UpdateSourceWindow, window_index %d, pos_x %f, pos_y %f, size_x %f, size_y %f\n", window_index, pos_x, pos_y, size_x, size_y);

    ret = gfNet_Send(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + 20), 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 20;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 20))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 20);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::SourceWindowToTop(TChar *source_channel, TU32 window_index, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkFocusDisplayWindow);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 4);

    DBEW32(window_index, p_payload);

    LOG_PRINTF("SourceWindowToTop, window_index %d\n", window_index);

    ret = gfNet_Send(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + 4), 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 4;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 4))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 4);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::ShowOtherWindows(TChar *source_channel, TU32 window_index, TU32 show, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkShowOtherWindows);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 4);

    DBEW16(window_index, p_payload);
    p_payload += 2;
    DBEW16(show, p_payload);
    p_payload += 2;

    LOG_PRINTF("ShowOtherWindows, window_index %d, show %d\n", window_index, show);
    ret = gfNet_Send(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + 4), 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 4;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 4))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 4);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::WaitCmd(TU32 &cmd_type)
{
    AUTO_LOCK(mpMutex);

    LOG_WARN("CSACPCloudClientAgent::WaitCmd: TO DO\n");
    return EECode_OK;
}

EECode CSACPCloudClientAgent::PeekCmd(TU32 &type, TU32 &payload_size, TU16 &header_ext_size)
{
    AUTO_LOCK(mpMutex);

    TInt size = 0;
    TU8 has_header_ext = 0;

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    size = sizeof(SSACPHeader);
    //LOG_PRINTF("read size %d start\n", size);
    size = gfNet_Recv(mSocket, (TU8 *)mpSACPHeader, size, DNETUTILS_RECEIVE_FLAG_READ_ALL);
    //LOG_PRINTF("read size %d done\n", size);
    if (DLikely(size == sizeof(SSACPHeader))) {
        type = ((mpSACPHeader->type_1) << 24) | ((mpSACPHeader->type_2) << 16) | ((mpSACPHeader->type_3) << 8) | (mpSACPHeader->type_4);
        payload_size = (mpSACPHeader->size_1 << 8) | (mpSACPHeader->size_2);
        has_header_ext = mpSACPHeader->header_ext_type;
        header_ext_size = (mpSACPHeader->header_ext_size_1 << 8) | mpSACPHeader->header_ext_size_2;
        DASSERT(EProtocolHeaderExtentionType_NoHeader == has_header_ext);
        DASSERT(0 == header_ext_size);

        DASSERT(!(type & DSACPTypeBit_Responce));
        if (type & DSACPTypeBit_Request) {
            //LOG_PRINTF("[client agent]: receive cmd 0x%08x, payload_size %d\n", type, payload_size);
        } else {
            //LOG_PRINTF("[client agent]: receive data 0x%08x, payload_size %d\n", type, payload_size);
        }
    } else {
        if (DLikely(!size)) {
            LOG_WARN("peer close comes\n");
            return EECode_Closed;
        } else {
            LOG_ERROR("only recv(%d), please check code\n", size);
            return EECode_Closed;
        }
    }
    return EECode_OK;
}

EECode CSACPCloudClientAgent::ReadData(TU8 *p_data, TMemSize size)
{
    AUTO_LOCK(mpMutex);

    TInt ret = 0;

    if (DUnlikely(!p_data || !size)) {
        LOG_ERROR("BAD params %p, size %ld\n", p_data, size);
        return EECode_BadParam;
    }

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need assign a client first\n", mSocket);
        return EECode_BadState;
    }

    //LOG_NOTICE("[blocking read data] total size %ld start\n", size);
    ret = gfNet_Recv(mSocket, p_data, size, DNETUTILS_RECEIVE_FLAG_READ_ALL);
    if (DUnlikely(0 >= ret)) {
        LOG_ERROR("size %ld, ret %d\n", size, ret);
        return EECode_Closed;
    }
    //LOG_NOTICE("[blocking read data size]: %d done\n", ret);

    return EECode_OK;
}

EECode CSACPCloudClientAgent::WriteData(TU8 *p_data, TMemSize size)
{
    AUTO_LOCK(mpMutex);

    TInt ret = 0;

    if (DUnlikely(!p_data || !size)) {
        LOG_ERROR("BAD params %p, size %ld\n", p_data, size);
        return EECode_BadParam;
    }

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need assign a client first\n", mSocket);
        return EECode_BadState;
    }

    ret = gfNet_Send(mSocket, p_data, (TInt)(size), 0);
    DASSERT(ret == (TInt)(size));

    return EECode_OK;
}


void CSACPCloudClientAgent::Delete()
{
    LOG_NOTICE("CSACPCloudClientAgent::Delete() begin, mSocket %d\n", mSocket);

    if (DIsSocketHandlerValid(mSocket)) {
        LOG_NOTICE("CSACPCloudClientAgent::Delete() begin, before close(mSocket %d)\n", mSocket);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    if (mpSenderBuffer) {
        DDBG_FREE(mpSenderBuffer, "CASB");
        mpSenderBuffer = NULL;
    }

    mSenderBufferSize = 0;
    mpSACPHeader = NULL;

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

}

EECode CSACPCloudClientAgent::SwitchGroup(TUint group, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkSwitchGroup);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 4);

    DBEW32(group, p_payload);

    LOG_PRINTF("SwitchGroup, group %d\n", group);
    ret = gfNet_Send(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + 4), 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 4;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 4))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 4);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::SeekTo(SStorageTime *time, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkSeekTo);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    if (DUnlikely(!time)) {
        LOG_ERROR("NULL time\n");
        return EECode_BadParam;
    }

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 8);

    DBEW16(time->year, p_payload);
    p_payload += 2;
    *p_payload++ = time->month;
    *p_payload++ = time->day;
    *p_payload++ = time->hour;
    *p_payload++ = time->minute;
    *p_payload++ = time->second;
    *p_payload++ = time->frames;

    ret = sizeof(SSACPHeader) + 8;
    LOG_PRINTF("SeekTo\n");
    ret = gfNet_Send(mSocket, mpSenderBuffer, ret, 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 8;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 8))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 8);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::TrickPlay(ERemoteTrickplay trickplay, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkTrickplay);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 4);

    *p_payload++ = trickplay;

    ret = sizeof(SSACPHeader) + 4;
    LOG_PRINTF("TrickPlay\n");
    ret = gfNet_Send(mSocket, mpSenderBuffer, ret, 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 4;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 4))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 4);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::FastForward(TUint try_speed, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkFastForward);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 4);

    DBEW32(try_speed, p_payload);

    ret = sizeof(SSACPHeader) + 4;
    LOG_PRINTF("FastForward\n");
    ret = gfNet_Send(mSocket, mpSenderBuffer, ret, 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 4;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 4))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 4);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::FastBackward(TUint try_speed, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkFastBackward);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 4);

    DBEW32(try_speed, p_payload);

    ret = sizeof(SSACPHeader) + 4;
    LOG_PRINTF("FastBackward\n");
    ret = gfNet_Send(mSocket, mpSenderBuffer, ret, 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 4;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 4))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 4);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

#if 0
EECode CSACPCloudClientAgent::CustomizedCommand(TChar *source_channel, TU32 param1, TU32 param2, TU32 param3, TU32 param4, TU32 param5, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_CustomizedCommand);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 20);

    DBEW32(param1, p_payload);
    p_payload += 4;

    DBEW32(param2, p_payload);
    p_payload += 4;

    DBEW32(param3, p_payload);
    p_payload += 4;

    DBEW32(param4, p_payload);
    p_payload += 4;

    DBEW32(param5, p_payload);

    LOG_PRINTF("CustomizedCommand, param1 %d, param2 %d, param3 %d, param4 %d, param5 %d\n", param1, param2, param3, param4, param5);

    ret = gfNet_Send(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + 20), 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 20;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 20))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 20);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}
#endif

EECode CSACPCloudClientAgent::QueryVersion(SCloudAgentVersion *version)
{
    AUTO_LOCK(mpMutex);

    if (mbGetPeerVersion) {
        *version = mVersion;
    } else {
        TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_QueryVersion);
        TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
        TInt ret = 0;

        DASSERT(0 <= mSocket);
        if (DUnlikely(0 > mSocket)) {
            LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
            return EECode_BadState;
        }

        fillHeader(type, 8);

        DBEW16(mVersion.native_major, p_payload);
        p_payload += 2;
        *p_payload ++ = mVersion.native_minor;
        *p_payload ++ = mVersion.native_patch;
        DBEW16(mVersion.native_date_year, p_payload);
        p_payload += 2;
        *p_payload ++ = mVersion.native_date_month;
        *p_payload ++ = mVersion.native_date_day;

        ret = sizeof(SSACPHeader) + 8;
        LOG_PRINTF("QueryVersion\n");
        ret = gfNet_Send(mSocket, mpSenderBuffer, ret, 0);
        if (DLikely(ret == (TInt)(sizeof(SSACPHeader) + 8))) {
            ret = gfNet_Recv(mSocket, mpSenderBuffer, ret, DNETUTILS_RECEIVE_FLAG_READ_ALL);
            if (DLikely(ret == (TInt)(sizeof(SSACPHeader) + 8))) {
                p_payload = mpSenderBuffer + sizeof(SSACPHeader);

                DBER16(mVersion.peer_major, p_payload);
                p_payload += 2;
                mVersion.peer_minor = *p_payload ++;
                mVersion.peer_patch = *p_payload ++;
                DBER16(mVersion.peer_date_year, p_payload);
                p_payload += 2;
                mVersion.peer_date_month = *p_payload ++;
                mVersion.peer_date_day = *p_payload ++;
            } else if (DUnlikely(!ret)) {
                LOG_ERROR("peer close\n");
                return EECode_Closed;
            } else {
                LOG_ERROR("recv error, return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 8);
                return EECode_Closed;
            }
        } else if (DUnlikely(!ret)) {
            LOG_ERROR("peer close\n");
            return EECode_Closed;
        } else {
            LOG_ERROR("send error, return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 8);
            return EECode_Closed;
        }

        *version = mVersion;
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::QueryStatus(SSyncStatus *status, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    AUTO_LOCK(mpMutex);

    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_QueryStatus);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;
    TMemSize remain_payload_size = 0;
    TMemSize read_length = 0;

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 0);

    ret = sizeof(SSACPHeader);
    LOG_PRINTF("QueryStatus before send(seq count %d %d), %d\n", mpSACPHeader->seq_count_1, mpSACPHeader->seq_count_2, ret);
    ret = gfNet_Send(mSocket, mpSenderBuffer, ret, 0);

    if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
        LOG_ERROR("send header error, return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
        return EECode_Error;
    }

    LOG_PRINTF("QueryStatus before recv header\n");
    ret = gfNet_Recv(mSocket, mpSenderBuffer, sizeof(SSACPHeader), DNETUTILS_RECEIVE_FLAG_READ_ALL);
    if (DUnlikely(!ret)) {
        LOG_ERROR("peer close\n");
        return EECode_Closed;
    } else if (DUnlikely(((TUint)ret) < sizeof(SSACPHeader))) {
        LOG_ERROR("gfNet_Recv error, ret %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
        return EECode_Error;
    }

    //read payload here
    TInt expect_payload_size = ((TInt)mpSACPHeader->size_0 << 16) | ((TInt)mpSACPHeader->size_1 << 8) | (TInt)mpSACPHeader->size_2;
    LOG_PRINTF("QueryStatus read playload size, %d\n", expect_payload_size);
    ret = gfNet_Recv(mSocket, mpSenderBuffer, expect_payload_size, DNETUTILS_RECEIVE_FLAG_READ_ALL);
    if (DUnlikely(ret != expect_payload_size)) {
        LOG_ERROR("peer close, ret %d, expected %d\n", ret, expect_payload_size);
        return EECode_Closed;
    }
    p_payload = mpSenderBuffer;
    remain_payload_size = (TMemSize)expect_payload_size;

    TU32 param0, param1, param2, param3, param4, param5, param6, param7;
    ESACPElementType element_type;
    TU32 window_index = 0;
    TU32 zoom_index = 0;

    while (remain_payload_size) {

        read_length = gfReadSACPElement(p_payload, remain_payload_size, element_type, param0, param1, param2, param3, param4, param5, param6, param7);
        if (remain_payload_size < read_length) {
            LOG_ERROR("internal logic bug\n");
            return EECode_Error;
        }

        switch (element_type) {

            case ESACPElementType_GlobalSetting: {
                    status->channel_number_per_group = param0;
                    status->total_group_number = param1;
                    status->current_group_index = param2;
                    status->have_dual_stream = param3;
                    status->is_vod_enabled = param4;
                    status->is_vod_ready = param5;
                    status->total_window_number = param6;
                    status->current_display_window_number = param7;
                }
                break;

            case ESACPElementType_SyncFlags: {
                    status->update_group_info_flag = param0;
                    status->update_display_layout_flag = param1;
                    status->update_source_flag = param2;
                    status->update_display_flag = param3;
                    status->update_audio_flag = param4;
                    status->update_zoom_flag = param5;
                }
                break;

            case ESACPElementType_EncodingParameters:
                status->encoding_width = param0;
                status->encoding_height = param1;
                status->encoding_bitrate = param2;
                status->encoding_framerate = param3;
                break;

            case ESACPElementType_DisplaylayoutInfo:
                status->display_layout = param0;
                status->display_hd_channel_index = param1;
                status->audio_source_index = param2;
                status->audio_target_index = param3;
                break;

            case ESACPElementType_DisplayWindowInfo:
                if (DLikely(window_index < DQUERY_MAX_DISPLAY_WINDOW_NUMBER)) {
                    status->window[window_index].window_pos_x = param0;
                    status->window[window_index].window_pos_y = param1;
                    status->window[window_index].window_width = param2;
                    status->window[window_index].window_height = param3;
                    status->window[window_index].udec_index = param4;
                    status->window[window_index].render_index = param5;
                    status->window[window_index].display_disable = param6;
                    window_index ++;
                } else {
                    LOG_ERROR("too many windows\n");
                    //return EECode_Error;
                }
                break;

            case ESACPElementType_DisplayZoomInfo:
                if (DLikely(zoom_index < DQUERY_MAX_DISPLAY_WINDOW_NUMBER)) {
                    status->zoom[zoom_index].zoom_size_x = param0;
                    status->zoom[zoom_index].zoom_size_y = param1;
                    status->zoom[zoom_index].zoom_input_center_x = param2;
                    status->zoom[zoom_index].zoom_input_center_y = param3;
                    status->zoom[zoom_index].udec_index = param4;
                    status->zoom[zoom_index].is_valid = 1;
                    zoom_index ++;
                } else {
                    LOG_ERROR("too many windows\n");
                    //return EECode_Error;
                }
                break;

            default:
                LOG_FATAL("BAD ESACPElementType %d\n", element_type);
                return EECode_InternalLogicalBug;
                break;
        }

        remain_payload_size -= read_length;
        p_payload += read_length;
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::SyncStatus(SSyncStatus *status, TU32 reply)
{
    AUTO_LOCK(mpMutex);
    DASSERT(!reply);
    reply = 0;

    LOG_WARN("not supported\n");
    return EECode_OK;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SyncStatus);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;
    TMemSize remain_payload_size = 0;
    TMemSize write_length = 0;
    TMemSize total_payload_size = 0;

    DASSERT(0 <= mSocket);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        return EECode_BadState;
    }

    p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    remain_payload_size = mSenderBufferSize - sizeof(SSACPHeader);

    TU32 param0, param1, param2, param3, param4, param5, param6, param7;
    TU32 window_index = 0;
    TU32 zoom_index = 0;
    TU32 total_num = status->total_window_number;

    param0 = status->channel_number_per_group;
    param1 = status->total_group_number;
    param2 = status->current_group_index;
    param3 = status->have_dual_stream;
    param4 = status->is_vod_enabled;
    param5 = status->is_vod_ready;
    param6 = status->total_window_number;
    param7 = status->current_display_window_number;

    write_length = gfWriteSACPElement(p_payload, remain_payload_size, ESACPElementType_GlobalSetting, param0, param1, param2, param3, param4, param5, param6, param7);
    total_payload_size += write_length;
    remain_payload_size -= write_length;
    p_payload += write_length;

    param0 = status->update_group_info_flag;
    param1 = status->update_display_layout_flag;
    param2 = status->update_source_flag;
    param3 = status->update_display_flag;
    param4 = status->update_audio_flag;
    param5 = status->update_zoom_flag;

    write_length = gfWriteSACPElement(p_payload, remain_payload_size, ESACPElementType_SyncFlags, param0, param1, param2, param3, param4, param5, param6, param7);
    total_payload_size += write_length;
    remain_payload_size -= write_length;
    p_payload += write_length;

    param0 = status->encoding_width;
    param1 = status->encoding_height;
    param2 = status->encoding_bitrate;
    param3 = status->encoding_framerate;
    write_length = gfWriteSACPElement(p_payload, remain_payload_size, ESACPElementType_EncodingParameters, param0, param1, param2, param3, param4, param5, param6, param7);
    total_payload_size += write_length;
    remain_payload_size -= write_length;
    p_payload += write_length;

    param0 = status->display_layout;
    param1 = status->display_hd_channel_index;
    param2 = status->audio_source_index;
    param3 = status->audio_target_index;
    write_length = gfWriteSACPElement(p_payload, remain_payload_size, ESACPElementType_DisplaylayoutInfo, param0, param1, param2, param3, param4, param5, param6, param7);
    total_payload_size += write_length;
    remain_payload_size -= write_length;
    p_payload += write_length;

    for (window_index = 0; window_index < total_num; window_index ++) {
        //param0 = status->window[window_index].window_pos_x;
        //param1 = status->window[window_index].window_pos_y;
        //param2 = status->window[window_index].window_width;
        //param3 = status->window[window_index].window_height;
        param0 = status->window[window_index].float_window_pos_x * DSACP_FIX_POINT_DEN;
        param1 = status->window[window_index].float_window_pos_y * DSACP_FIX_POINT_DEN;
        param2 = status->window[window_index].float_window_width * DSACP_FIX_POINT_DEN;
        param3 = status->window[window_index].float_window_height * DSACP_FIX_POINT_DEN;
        param4 = status->window[window_index].udec_index;
        param5 = status->window[window_index].render_index;
        param6 = status->window[window_index].display_disable;

        write_length = gfWriteSACPElement(p_payload, remain_payload_size, ESACPElementType_DisplayWindowInfo, param0, param1, param2, param3, param4, param5, param6, param7);
        total_payload_size += write_length;
        remain_payload_size -= write_length;
        p_payload += write_length;
    }

    for (zoom_index = 0; zoom_index < total_num; zoom_index ++) {
        //param0 = status->zoom[zoom_index].zoom_size_x;
        //param1 = status->zoom[zoom_index].zoom_size_y;
        //param2 = status->zoom[zoom_index].zoom_input_center_x;
        //param3 = status->zoom[zoom_index].zoom_input_center_y;
        param0 = (TU32)(status->zoom[zoom_index].float_zoom_size_x * DSACP_FIX_POINT_DEN);
        param1 = (TU32)(status->zoom[zoom_index].float_zoom_size_y * DSACP_FIX_POINT_DEN);
        param2 = (TU32)(status->zoom[zoom_index].float_zoom_input_center_x * DSACP_FIX_POINT_DEN);
        param3 = (TU32)(status->zoom[zoom_index].float_zoom_input_center_y * DSACP_FIX_POINT_DEN);
        param4 = status->zoom[zoom_index].udec_index;

        write_length = gfWriteSACPElement(p_payload, remain_payload_size, ESACPElementType_DisplayZoomInfo, param0, param1, param2, param3, param4, param5, param6, param7);
        total_payload_size += write_length;
        remain_payload_size -= write_length;
        p_payload += write_length;
    }

    write_length += sizeof(SSACPHeader);
    fillHeader(type, (TU16)write_length);

    ret = sizeof(SSACPHeader);
    LOG_PRINTF("SyncStatus send\n");
    ret = gfNet_Send(mSocket, mpSenderBuffer, write_length, 0);

    DASSERT(ret == (TInt)(write_length));
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = write_length;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(write_length))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, write_length);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }
    return EECode_OK;
}

TInt CSACPCloudClientAgent::GetHandle() const
{
    return mSocket;
}

EECode CSACPCloudClientAgent::DebugPrintCloudPipeline(TU32 index, TU32 param_0, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPDebugCmdSubType_PrintCloudPipeline);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 4);

    //DBEW32(index, p_payload);
    DBEW32((((param_0 & 0xff) << 24) | (index & 0xffffff)), p_payload);
    ret = sizeof(SSACPHeader) + 4;
    LOG_PRINTF("ESACPDebugCmdSubType_PrintCloudPipeline, index %d\n", index);
    ret = gfNet_Send(mSocket, mpSenderBuffer, ret, 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 4;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 4))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 4);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::DebugPrintStreamingPipeline(TU32 index, TU32 param_0, TU32 reply)
{
    //DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPDebugCmdSubType_PrintStreamingPipeline);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 4);

    //DBEW32(index, p_payload);
    DBEW32((((param_0 & 0xff) << 24) | (index & 0xffffff)), p_payload);
    ret = sizeof(SSACPHeader) + 4;
    LOG_PRINTF("ESACPDebugCmdSubType_PrintStreamingPipeline, index %d\n", index);
    ret = gfNet_Send(mSocket, mpSenderBuffer, ret, 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 4;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 4))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 4);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::DebugPrintRecordingPipeline(TU32 index, TU32 param_0, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPDebugCmdSubType_PrintRecordingPipeline);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 4);

    //DBEW32(index, p_payload);
    DBEW32((((param_0 & 0xff) << 24) | (index & 0xffffff)), p_payload);

    ret = sizeof(SSACPHeader) + 4;
    LOG_PRINTF("ESACPDebugCmdSubType_PrintRecordingPipeline, index %d\n", index);
    ret = gfNet_Send(mSocket, mpSenderBuffer, ret, 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 4;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 4))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 4);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::DebugPrintChannel(TU32 index, TU32 param_0, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPDebugCmdSubType_PrintSingleChannel);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 4);

    //DBEW32(index, p_payload);
    DBEW32((((param_0 & 0xff) << 24) | (index & 0xffffff)), p_payload);

    ret = sizeof(SSACPHeader) + 4;
    LOG_PRINTF("ESACPDebugCmdSubType_PrintSingleChannel, index %d\n", index);
    ret = gfNet_Send(mSocket, mpSenderBuffer, ret, 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 4;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 4))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 4);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::DebugPrintAllChannels(TU32 param_0, TU32 reply)
{
    DASSERT(!reply);
    reply = 0;

    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPDebugCmdSubType_PrintAllChannels);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    fillHeader(type, 4);

    //DBEW32(index, p_payload);
    DBEW32(((param_0 & 0xff) << 24), p_payload);

    ret = sizeof(SSACPHeader) + 4;
    LOG_PRINTF("ESACPDebugCmdSubType_PrintAllChannels\n");
    ret = gfNet_Send(mSocket, mpSenderBuffer, ret, 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + 4;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 4))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 4);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }
    }

    return EECode_OK;
}

#if 0
EECode CSACPCloudClientAgent::SendStringJson(TChar *input, TU32 input_size, TChar *output, TU32 output_size, TU32 reply)
{
    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPDebugCmdSubType_TextableAPIJson);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    if (DUnlikely((input_size + 16) > (mSenderBufferSize))) {
        LOG_ERROR("size too big, input_size %d\n", input_size);
        return EECode_BadParam;
    }

    fillHeader(type, input_size);
    memcpy(p_payload, input, input_size);

    ret = sizeof(SSACPHeader) + input_size;
    LOG_PRINTF("ESACPDebugCmdSubType_TextableAPIJson\n");
    ret = gfNet_Send(mSocket, mpSenderBuffer, ret, 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + input_size;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 4))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 4);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }

        TU32 request_type = type;
        TU32 payload_size = 0;
        TU16 header_ext_size = 0;
        TU8 has_header_ext = 0;

        type = ((mpSACPHeader->type_1) << 24) | ((mpSACPHeader->type_2) << 16) | ((mpSACPHeader->type_3) << 8) | (mpSACPHeader->type_4);
        payload_size = ((TU32)mpSACPHeader->size_0 << 16) | ((TU32)mpSACPHeader->size_1 << 8) | ((TU32)mpSACPHeader->size_2);
        has_header_ext = mpSACPHeader->header_ext_type;
        header_ext_size = (mpSACPHeader->header_ext_size_1 << 8) | mpSACPHeader->header_ext_size_2;

        if (DUnlikely((EProtocolHeaderExtentionType_NoHeader != has_header_ext) || (0 != header_ext_size))) {
            LOG_FATAL("corrupted protocol, has_header_ext %d, header_ext_size %d\n", has_header_ext, header_ext_size);
            return EECode_DataCorruption;
        }

        DASSERT(type & DSACPTypeBit_Responce);
        if (DUnlikely((type & 0xffffff) != (request_type & 0xffffff))) {
            LOG_FATAL("corrupted protocol, type %08x, request_type %08x\n", type, request_type);
            return EECode_DataCorruption;
        }

        if (DUnlikely(output_size < payload_size)) {
            LOG_FATAL("corrupted protocol, type %08x, request_type %08x\n", type, request_type);
            return EECode_DataCorruption;
        }
        ret = gfNet_Recv(mSocket, (TU8 *)output, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);

    }

    return EECode_OK;
}

EECode CSACPCloudClientAgent::SendStringXml(TChar *input, TU32 input_size, TChar *output, TU32 output_size, TU32 reply)
{
    TU32 type = DBuildSACPType(reply ? (DSACPTypeBit_Request | DSACPTypeBit_NeedReply) : DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPDebugCmdSubType_TextableAPIXml);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    AUTO_LOCK(mpMutex);

    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need connect to server first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = 0;
        mLastErrorHint2 = mSocket;
        return EECode_BadState;
    }

    if (DUnlikely((input_size + 16) > (mSenderBufferSize))) {
        LOG_ERROR("size too big, input_size %d\n", input_size);
        return EECode_BadParam;
    }

    fillHeader(type, input_size);
    memcpy(p_payload, input, input_size);

    ret = sizeof(SSACPHeader) + input_size;
    LOG_PRINTF("ESACPDebugCmdSubType_TextableAPIJson\n");
    ret = gfNet_Send(mSocket, mpSenderBuffer, ret, 0);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = sizeof(SSACPHeader) + input_size;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 4))) {
        LOG_ERROR("gfNet_Send(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + 4);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    if (reply) {
        ret = gfNet_Recv(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 3;
            mLastErrorHint2 = 0;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = 4;
            mLastErrorHint2 = ret;
            return EECode_NetReceiveHeader_Fail;
        }

        TU32 request_type = type;
        TU32 payload_size = 0;
        TU16 header_ext_size = 0;
        TU8 has_header_ext = 0;

        type = ((mpSACPHeader->type_1) << 24) | ((mpSACPHeader->type_2) << 16) | ((mpSACPHeader->type_3) << 8) | (mpSACPHeader->type_4);
        payload_size = ((TU32)mpSACPHeader->size_0 << 16) | ((TU32)mpSACPHeader->size_1 << 8) | ((TU32)mpSACPHeader->size_2);
        has_header_ext = mpSACPHeader->header_ext_type;
        header_ext_size = (mpSACPHeader->header_ext_size_1 << 8) | mpSACPHeader->header_ext_size_2;

        if (DUnlikely((EProtocolHeaderExtentionType_NoHeader != has_header_ext) || (0 != header_ext_size))) {
            LOG_FATAL("corrupted protocol, has_header_ext %d, header_ext_size %d\n", has_header_ext, header_ext_size);
            return EECode_DataCorruption;
        }

        DASSERT(type & DSACPTypeBit_Responce);
        if (DUnlikely((type & 0xffffff) != (request_type & 0xffffff))) {
            LOG_FATAL("corrupted protocol, type %08x, request_type %08x\n", type, request_type);
            return EECode_DataCorruption;
        }

        if (DUnlikely(output_size < payload_size)) {
            LOG_FATAL("corrupted protocol, type %08x, request_type %08x\n", type, request_type);
            return EECode_DataCorruption;
        }
        ret = gfNet_Recv(mSocket, (TU8 *)output, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL);

    }

    return EECode_OK;
}
#endif

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

