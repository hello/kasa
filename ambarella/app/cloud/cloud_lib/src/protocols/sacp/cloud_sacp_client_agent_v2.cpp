/*******************************************************************************
 * cloud_sacp_client_agent_v2.cpp
 *
 * History:
 *  2014/08/25 - [Zhi He] create file
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
#include "cloud_sacp_client_agent_v2.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

ICloudClientAgentV2 *gfCreateSACPCloudClientAgentV2()
{
    CSACPCloudClientAgentV2 *thiz = CSACPCloudClientAgentV2::Create();
    return thiz;
}

//-----------------------------------------------------------------------
//
//  CSACPCloudClientAgentV2
//
//-----------------------------------------------------------------------

CSACPCloudClientAgentV2 *CSACPCloudClientAgentV2::Create()
{
    CSACPCloudClientAgentV2 *result = new CSACPCloudClientAgentV2();
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

EECode CSACPCloudClientAgentV2::Construct()
{
    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOG_FATAL("no memory, gfCreateMutex() fail\n");
        return EECode_NoMemory;
    }

    mSenderBufferSize = DSACP_MAX_PAYLOAD_LENGTH + 256;
    mpSenderBuffer = (TU8 *) DDBG_MALLOC(mSenderBufferSize, "CASB");

    if (DUnlikely(!mpSenderBuffer)) {
        LOG_FATAL("no memory, request %ld\n", mSenderBufferSize);
        mSenderBufferSize = 0;
        return EECode_NoMemory;
    }

    mpSACPHeader = (SSACPHeader *)mpSenderBuffer;

    mVersion.native_major = DCloudLibVesionMajor;
    mVersion.native_minor = DCloudLibVesionMinor;
    mVersion.native_patch = DCloudLibVesionPatch;
    mVersion.native_date_year = DCloudLibVesionYear;
    mVersion.native_date_month = DCloudLibVesionMonth;
    mVersion.native_date_day = DCloudLibVesionDay;

    return EECode_OK;
}

CSACPCloudClientAgentV2::CSACPCloudClientAgentV2()
    : mLocalPort(0)
    , mServerPort(DDefaultSACPServerPort)
    , mbConnected(0)
    , mbAuthenticated(0)
    , mbFirstConnection(0)
    , mbGetPeerVersion(0)
    , mSocket(DInvalidSocketHandler)
    , mCmdSeqCount(0)
    , mbReceiveConnectStatus(1)
    , mEncryptionType1(EEncryptionType_None)
    , mEncryptionType2(EEncryptionType_None)
    , mpSenderBuffer(NULL)
    , mSenderBufferSize(0)
    , mpSACPHeader(NULL)
    , mLastErrorHint1(0)
    , mLastErrorHint2(0)
    , mLastErrorCode(EECode_OK)
{
}

CSACPCloudClientAgentV2::~CSACPCloudClientAgentV2()
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

EECode CSACPCloudClientAgentV2::ConnectToServer(TChar *server_url, TU16 server_port, TChar *account_name, TChar *password, TU16 local_port)
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ConnectionTag, 0);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;
    TU32 account_name_length = 0;

    AUTO_LOCK(mpMutex);

    if (DUnlikely(!server_url)) {
        LOG_ERROR("NULL server_url\n");
        return EECode_BadParam;
    } else {
        LOG_NOTICE("server_url %s, local_port %hu, server_port %hu, default local port %hu, server port %hu\n", server_url, local_port, server_port, mLocalPort, mServerPort);
    }

    if (DUnlikely((!account_name))) {
        LOG_ERROR("NULL account name\n");
        return EECode_BadParam;
    }
    account_name_length = strlen(account_name);

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

    gfSACPFillHeader(mpSACPHeader, type, account_name_length, EProtocolHeaderExtentionType_NoHeader, 0, 0, 0);
    if (DLikely(mbReceiveConnectStatus)) {
        mpSACPHeader->flags |= DSACPHeaderFlagBit_ReplyResult;
    } else {
        LOG_FATAL("need recieve connect status now\n");
        return EECode_BadState;
    }

    memcpy(p_payload, account_name, account_name_length);

    gfSocketSetTimeout(mSocket, 2, 0);

    ret = gfNet_Send_timeout(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + account_name_length), 0, 1);
    if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + account_name_length))) {
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = ret;
        mLastErrorHint2 = sizeof(SSACPHeader) + account_name_length;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_NetSendHeader_Fail;
    }

    if (DLikely(mbReceiveConnectStatus)) {
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

        EECode err = gfSACPConnectResultToErrorCode((ESACPConnectResult) mpSACPHeader->flags);
        if (DLikely(EECode_OK == err)) {
            LOG_NOTICE("connect success without hardware authentication\n");
        } else if (EECode_OK_NeedHardwareAuthenticate == err) {
            if (DUnlikely(!mfAuthenticateCallback)) {
                LOG_ERROR("server need authenticate, please invoke SetHardwareAuthenticateCallback(callback) first\n");
                return EECode_BadState;
            }

            //continue to receive hardware_authentication_input
            TU8 auth_input[DDynamicInputStringLength] = {0};
            ret = gfNet_Recv_timeout(mSocket, auth_input, DDynamicInputStringLength, DNETUTILS_RECEIVE_FLAG_READ_ALL, 20);
            if (DUnlikely(DDynamicInputStringLength != ret)) {
                LOG_WARN("ConnectToServer, recv dynamic_input %d not expected, expect %d, peer closed?\n", ret, DDynamicInputStringLength);
                mLastErrorCode = EECode_NetReceivePayload_Fail;
                mLastErrorHint1 = ret;
                mLastErrorHint2 = DDynamicInputStringLength;
                gfNetCloseSocket(mSocket);
                mSocket = DInvalidSocketHandler;
                return EECode_NetReceivePayload_Fail;
            }

            TU32 verify = mfAuthenticateCallback(auth_input, DDynamicInputStringLength);
            TU8 *ptmp = mpSenderBuffer + sizeof(SSACPHeader);
            DBEW32(verify, ptmp);
            ret = gfNet_Send_timeout(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + 4), 0, 2);
            if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + 4))) {
                mLastErrorCode = EECode_NetSendHeader_Fail;
                mLastErrorHint1 = ret;
                mLastErrorHint2 = sizeof(SSACPHeader) + 4;
                gfNetCloseSocket(mSocket);
                mSocket = DInvalidSocketHandler;
                return EECode_NetSendHeader_Fail;
            }

            err = gfSACPConnectResultToErrorCode((ESACPConnectResult) mpSACPHeader->flags);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("authentication fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
            LOG_NOTICE("connect success with hardware authentication\n");
        } else {
            LOG_ERROR("connect fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
            mbConnected = 0;
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return err;
        }
    }

    mbConnected = 1;
    return EECode_OK;
}

EECode CSACPCloudClientAgentV2::SetHardwareAuthenticateCallback(TClientAgentAuthenticateCallBack callback)
{
    AUTO_LOCK(mpMutex);

    mfAuthenticateCallback = callback;
    return EECode_OK;
}

EECode CSACPCloudClientAgentV2::DisconnectToServer()
{
    AUTO_LOCK(mpMutex);

    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    mbConnected = 0;

    return EECode_OK;
}

EECode CSACPCloudClientAgentV2::SetupVideoParams(ESACPDataChannelSubType video_format, TU32 framerate, TU32 res_width, TU32 res_height, TU32 bitrate, TU8 *extradata, TU16 extradata_size, TU32 gop)
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SourceUpdateVideoParams);
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
    DBEW16(ETLV16Type_DeviceVideoParams_Format, p_payload);
    p_payload += 2;
    DBEW16(1, p_payload);
    p_payload += 2;
    p_payload[0] = video_format;
    p_payload += 1;
    payload_len += 5;

    DBEW16(ETLV16Type_DeviceVideoParams_FrameRate, p_payload);
    p_payload += 2;
    DBEW16(4, p_payload);
    p_payload += 2;
    DBEW32(framerate, p_payload);
    p_payload += 4;
    payload_len += 8;

    TU32 fps = 0, den = 0;
    DParseVideoFrameRate(den, fps, framerate);
    LOG_PRINTF("SetupVideoParams, input framerate %08x, fps %d, den %d\n", framerate, fps, den);

    if (res_width && res_height) {
        DBEW16(ETLV16Type_DeviceVideoParams_Resolution, p_payload);
        p_payload += 2;
        DBEW16(8, p_payload);
        p_payload += 2;
        DBEW32(res_width, p_payload);
        p_payload += 4;
        DBEW32(res_height, p_payload);
        p_payload += 4;
        payload_len += 12;
    }

    DBEW16(ETLV16Type_DeviceVideoParams_Bitrate, p_payload);
    p_payload += 2;
    DBEW16(4, p_payload);
    p_payload += 2;
    DBEW32(bitrate, p_payload);
    p_payload += 4;
    payload_len += 8;

    if (extradata && extradata_size) {
        DBEW16(ETLV16Type_DeviceVideoParams_ExtraData, p_payload);
        p_payload += 2;
        DBEW16(extradata_size, p_payload);
        p_payload += 2;
        memcpy(p_payload, extradata, extradata_size);
        p_payload += extradata_size;
        payload_len += 4 + extradata_size;
    }

    DBEW16(ETLV16Type_DeviceVideoGOPStructure, p_payload);
    p_payload += 2;
    DBEW16(4, p_payload);
    p_payload += 2;
    DBEW32(gop, p_payload);
    p_payload += 4;
    payload_len += 8;

    DASSERT((TInt)(p_payload - mpSenderBuffer - sizeof(SSACPHeader)) == payload_len);

    gfSACPFillHeader(mpSACPHeader, type, payload_len, EProtocolHeaderExtentionType_NoHeader, 0, mCmdSeqCount, 0);
    mCmdSeqCount ++;

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

    return EECode_OK;
}

EECode CSACPCloudClientAgentV2::SetupAudioParams(ESACPDataChannelSubType audio_format, TU8 audio_channnel_number, TU32 audio_sample_frequency, TU32 audio_bitrate, TU8 *extradata, TU16 extradata_size)
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SourceUpdateAudioParams);
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
    DBEW16(extradata_size, p_payload);
    p_payload += 2;
    memcpy(p_payload, extradata, extradata_size);
    p_payload += extradata_size;

    payload_len = 30 + extradata_size;
    DASSERT((TInt)(p_payload - mpSenderBuffer - sizeof(SSACPHeader)) == payload_len);

    gfSACPFillHeader(mpSACPHeader, type, payload_len, EProtocolHeaderExtentionType_NoHeader, 0, mCmdSeqCount, 0);
    mCmdSeqCount ++;

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

    return EECode_OK;
}

EECode CSACPCloudClientAgentV2::Uploading(TU8 *p_data, TMemSize size, ESACPDataChannelSubType data_type, TU32 seq_number, TU8 extra_flag)
{
    TU32 type = DBuildSACPType(0, ESACPTypeCategory_DataChannel, (TU16)data_type);
    TInt ret = 0;
    TMemSize transfer_size = size;

    AUTO_LOCK(mpMutex);

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
    mpSACPHeader->header_ext_type = EProtocolHeaderExtentionType_NoHeader;
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

        mpSACPHeader->seq_count_0 = (seq_number >> 16) & 0xff;
        mpSACPHeader->seq_count_1 = (seq_number >> 8) & 0xff;
        mpSACPHeader->seq_count_2 = seq_number & 0xff;

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

EECode CSACPCloudClientAgentV2::VideoSync(TTime video_current_time, TU32 video_seq_number, ETimeUnit time_unit)
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_VideoSync);
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
    DBEW16(ETLV16Type_TimeUnit, p_payload);
    p_payload += 2;
    DBEW16(1, p_payload);
    p_payload += 2;
    p_payload[0] = time_unit;
    p_payload += 1;
    payload_len += 5;

    DBEW16(ETLV16Type_DeviceVideoSyncSeqNumber, p_payload);
    p_payload += 2;
    DBEW16(4, p_payload);
    p_payload += 2;
    DBEW32(video_seq_number, p_payload);
    p_payload += 4;
    payload_len += 8;

    DBEW16(ETLV16Type_DeviceVideoSyncTime, p_payload);
    p_payload += 2;
    DBEW16(8, p_payload);
    p_payload += 2;
    DBEW64(video_current_time, p_payload);
    p_payload += 8;
    payload_len += 12;

    DASSERT((TInt)(p_payload - mpSenderBuffer - sizeof(SSACPHeader)) == payload_len);

    gfSACPFillHeader(mpSACPHeader, type, payload_len, EProtocolHeaderExtentionType_NoHeader, 0, mCmdSeqCount, 0);
    mCmdSeqCount ++;

    ret = gfNet_Send_timeout(mSocket, mpSenderBuffer, sizeof(SSACPHeader) + payload_len, 0, 5);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send_timeout(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = (TU32)payload_len;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + payload_len))) {
        LOG_ERROR("gfNet_Send_timeout(), return %d, expected %lu\n", ret, (TULong) sizeof(SSACPHeader) + payload_len);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgentV2::AudioSync(TTime audio_current_time, TU32 audio_seq_number, ETimeUnit time_unit)
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_AudioSync);
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
    DBEW16(ETLV16Type_TimeUnit, p_payload);
    p_payload += 2;
    DBEW16(1, p_payload);
    p_payload += 2;
    p_payload[0] = time_unit;
    p_payload += 1;
    payload_len += 5;

    DBEW16(ETLV16Type_DeviceAudioSyncSeqNumber, p_payload);
    p_payload += 2;
    DBEW16(4, p_payload);
    p_payload += 2;
    DBEW32(audio_seq_number, p_payload);
    p_payload += 4;
    payload_len += 8;

    DBEW16(ETLV16Type_DeviceAudioSyncTime, p_payload);
    p_payload += 2;
    DBEW16(8, p_payload);
    p_payload += 2;
    DBEW64(audio_current_time, p_payload);
    p_payload += 8;
    payload_len += 12;

    DASSERT((TInt)(p_payload - mpSenderBuffer - sizeof(SSACPHeader)) == payload_len);

    gfSACPFillHeader(mpSACPHeader, type, payload_len, EProtocolHeaderExtentionType_NoHeader, 0, mCmdSeqCount, 0);
    mCmdSeqCount ++;

    ret = gfNet_Send_timeout(mSocket, mpSenderBuffer, sizeof(SSACPHeader) + payload_len, 0, 5);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send_timeout(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = (TU32)payload_len;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + payload_len))) {
        LOG_ERROR("gfNet_Send_timeout(), return %d, expected %lu\n", ret, (TULong) sizeof(SSACPHeader) + payload_len);
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgentV2::DiscontiousIndicator()
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_DiscIndicator);
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

    gfSACPFillHeader(mpSACPHeader, type, 0, EProtocolHeaderExtentionType_NoHeader, 0, mCmdSeqCount, 0);
    mCmdSeqCount ++;

    ret = gfNet_Send_timeout(mSocket, mpSenderBuffer, sizeof(SSACPHeader), 0, 5);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Send_timeout(), return 0, peer closed\n");
        mLastErrorCode = EECode_Closed;
        mLastErrorHint1 = 1;
        mLastErrorHint2 = 0;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)))) {
        LOG_ERROR("gfNet_Send_timeout(), return %d, expected %lu\n", ret, (TULong) sizeof(SSACPHeader));
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = 2;
        mLastErrorHint2 = ret;
        return EECode_NetSendHeader_Fail;
    }

    return EECode_OK;
}

EECode CSACPCloudClientAgentV2::UpdateVideoBitrate(TU32 bitrate)
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkUpdateBitrate);
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

    gfSACPFillHeader(mpSACPHeader, type, 4, EProtocolHeaderExtentionType_NoHeader, 0, mCmdSeqCount, 0);
    mCmdSeqCount ++;

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

    return EECode_OK;
}

EECode CSACPCloudClientAgentV2::UpdateVideoFramerate(TU32 framerate)
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkUpdateFramerate);
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

    gfSACPFillHeader(mpSACPHeader, type, 4, EProtocolHeaderExtentionType_NoHeader, 0, mCmdSeqCount, 0);
    mCmdSeqCount ++;

    DBEW32(framerate, p_payload);

    TU32 fps = 0, den = 0;
    DParseVideoFrameRate(den, fps, framerate);
    LOG_PRINTF("UpdateSourceFramerate, input framerate %08x, fps %d, den %d\n", framerate, fps, den);

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

    return EECode_OK;
}

void CSACPCloudClientAgentV2::Delete()
{
    LOG_NOTICE("CSACPCloudClientAgentV2::Delete() begin, mSocket %d\n", mSocket);

    if (DIsSocketHandlerValid(mSocket)) {
        LOG_NOTICE("CSACPCloudClientAgentV2::Delete() begin, before close(mSocket %d)\n", mSocket);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    if (mpSenderBuffer) {
        free(mpSenderBuffer);
        mpSenderBuffer = NULL;
    }

    mSenderBufferSize = 0;
    mpSACPHeader = NULL;

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

}

EECode CSACPCloudClientAgentV2::DebugPrintCloudPipeline(TU32 index, TU32 param_0)
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPDebugCmdSubType_PrintCloudPipeline);
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

    gfSACPFillHeader(mpSACPHeader, type, 4, EProtocolHeaderExtentionType_NoHeader, 0, mCmdSeqCount, 0);
    mCmdSeqCount ++;

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

    return EECode_OK;
}

EECode CSACPCloudClientAgentV2::DebugPrintStreamingPipeline(TU32 index, TU32 param_0)
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPDebugCmdSubType_PrintStreamingPipeline);
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

    gfSACPFillHeader(mpSACPHeader, type, 4, EProtocolHeaderExtentionType_NoHeader, 0, mCmdSeqCount, 0);
    mCmdSeqCount ++;

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

    return EECode_OK;
}

EECode CSACPCloudClientAgentV2::DebugPrintRecordingPipeline(TU32 index, TU32 param_0)
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPDebugCmdSubType_PrintRecordingPipeline);
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

    gfSACPFillHeader(mpSACPHeader, type, 4, EProtocolHeaderExtentionType_NoHeader, 0, mCmdSeqCount, 0);
    mCmdSeqCount ++;

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

    return EECode_OK;
}

EECode CSACPCloudClientAgentV2::DebugPrintChannel(TU32 index, TU32 param_0)
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPDebugCmdSubType_PrintSingleChannel);
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

    gfSACPFillHeader(mpSACPHeader, type, 4, EProtocolHeaderExtentionType_NoHeader, 0, mCmdSeqCount, 0);
    mCmdSeqCount ++;

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

    return EECode_OK;
}

EECode CSACPCloudClientAgentV2::DebugPrintAllChannels(TU32 param_0)
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPDebugCmdSubType_PrintAllChannels);
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

    gfSACPFillHeader(mpSACPHeader, type, 4, EProtocolHeaderExtentionType_NoHeader, 0, mCmdSeqCount, 0);
    mCmdSeqCount ++;

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

    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

