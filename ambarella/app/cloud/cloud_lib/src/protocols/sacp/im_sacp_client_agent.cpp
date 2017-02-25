/*******************************************************************************
 * im_sacp_client_agent.cpp
 *
 * History:
 *  2014/06/21 - [Zhi He] create file
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

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"
#include "common_network_utils.h"

#include "cloud_lib_if.h"

#include "security_utils_if.h"

#include "sacp_types.h"

#include "im_sacp_client_agent.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IIMClientAgent *gfCreateSACPIMClientAgent()
{
    CSACPIMClientAgent *thiz = CSACPIMClientAgent::Create();
    return thiz;
}

IIMClientAgentAsync *gfCreateSACPIMClientAgentAsync()
{
    CSACPIMClientAgentAsync *thiz = CSACPIMClientAgentAsync::Create();
    return thiz;
}

//-----------------------------------------------------------------------
//
//  CSACPIMClientAgentAsync
//
//-----------------------------------------------------------------------

CSACPIMClientAgentAsync *CSACPIMClientAgentAsync::Create()
{
    CSACPIMClientAgentAsync *result = new CSACPIMClientAgentAsync();
    if ((result) && (EECode_OK != result->Construct())) {
        result->Destroy();
        result = NULL;
    }
    return result;
}

EECode CSACPIMClientAgentAsync::Construct()
{
    //    mpMutex = CIMutex::Create(false);
    //    if (DUnlikely(!mpMutex)) {
    //        LOG_FATAL("no memory, CIMutex::Create(false) fail\n");
    //        return EECode_NoMemory;
    //    }

    EECode err = gfOSCreatePipe(mPipeFd);
    if (DUnlikely(err != EECode_OK)) {
        LOG_FATAL("pipe fail.\n");
        return EECode_Error;
    }

    mSenderBufferSize = DSACP_MAX_PAYLOAD_LENGTH;
    mpSenderBuffer = (TU8 *) DDBG_MALLOC(mSenderBufferSize + 128, "ICAS");

    if (DUnlikely(!mpSenderBuffer)) {
        LOG_FATAL("no memory, request %u\n", mSenderBufferSize);
        mSenderBufferSize = 0;
        return EECode_NoMemory;
    }

    mReadBufferSize = DSACP_MAX_PAYLOAD_LENGTH;
    mpReadBuffer = (TU8 *) DDBG_MALLOC(mReadBufferSize + 128, "ICAR");

    if (DUnlikely(!mpReadBuffer)) {
        LOG_FATAL("no memory, request %u\n", mReadBufferSize);
        mReadBufferSize = 0;
        return EECode_NoMemory;
    }

    mpSACPHeader = (SSACPHeader *)mpSenderBuffer;

    mpProtocolHeaderParser = gfCreatePotocolHeaderParser(EProtocolType_SACP, EProtocolHeaderExtentionType_SACP_IM);
    if (DUnlikely(mpProtocolHeaderParser == NULL)) {
        LOG_FATAL("gfCreatePotocolHeaderParser fail, no mem!\n");
        return EECode_NoMemory;
    }
    mnHeaderLength = mpProtocolHeaderParser->GetFixedHeaderLength();
    mnExtensionLength = sizeof(TUniqueID);
    /*
        mVersion.native_major = DCloudLibVesionMajor;
        mVersion.native_minor = DCloudLibVesionMinor;
        mVersion.native_patch = DCloudLibVesionPatch;
        mVersion.native_date_year = DCloudLibVesionYear;
        mVersion.native_date_month = DCloudLibVesionMonth;
        mVersion.native_date_day = DCloudLibVesionDay;
    */
    memset(mIdentityString, 0x0, sizeof(mIdentityString));
    return EECode_OK;
}

CSACPIMClientAgentAsync::CSACPIMClientAgentAsync()
    : mLocalPort(0)
    , mServerPort(0)
    , mbConnected(0)
    , mbAuthenticated(0)
    , mSocket(-1)
    , mSeqCount(0)
    , mDataType(ESACPDataChannelSubType_Invalid)
    , mnHeaderLength(0)
    , mEncryptionType1(EEncryptionType_None)
    , mEncryptionType2(EEncryptionType_None)
    , mDeviceIdentityStringLen(0)
    , mpSenderBuffer(NULL)
    , mSenderBufferSize(0)
    , mpReadBuffer(NULL)
    , mReadBufferSize(0)
    , mpPayloadBuffer(NULL)
    , mPayloadBufferSize(0)
    , mpSACPHeader(NULL)
    , mLastErrorHint1(0)
    , mLastErrorHint2(0)
    , mLastErrorCode(EECode_OK)
    , mpMessageCallbackOwner(NULL)
    , mpMessageCallback(NULL)
    , mpProtocolHeaderParser(NULL)
    , mnExtensionLength(0)
    , mUniqueID(0)
    , mpIDListBuffer(NULL)
    , mnIDListBufferLength(0)
{
}

CSACPIMClientAgentAsync::~CSACPIMClientAgentAsync()
{
    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    if (mpSenderBuffer) {
        DDBG_FREE(mpSenderBuffer, "ICAS");
        mpSenderBuffer = NULL;
    }

    if (mpReadBuffer) {
        DDBG_FREE(mpReadBuffer, "ICAR");
        mpReadBuffer = NULL;
    }

    if (mpPayloadBuffer) {
        DDBG_FREE(mpPayloadBuffer, "ICAP");
        mpPayloadBuffer = NULL;
    }

    if (mpIDListBuffer) {
        DDBG_FREE(mpIDListBuffer, "ICAL");
        mpIDListBuffer = NULL;
    }

    //if (mpMutex) {
    //    mpMutex->Delete();
    //    mpMutex = NULL;
    //}

    if (mpProtocolHeaderParser) {
        mpProtocolHeaderParser->Destroy();
        mpProtocolHeaderParser = NULL;
    }

    mSenderBufferSize = 0;
    mpSACPHeader = NULL;
}

void CSACPIMClientAgentAsync::fillHeader(TU32 type, TU32 size, TU16 ext_size, TU8 ext_type)
{
    mpSACPHeader->type_1 = (type >> 24) & 0xff;
    mpSACPHeader->type_2 = (type >> 16) & 0xff;
    mpSACPHeader->type_3 = (type >> 8) & 0xff;
    mpSACPHeader->type_4 = type & 0xff;

    //LOG_NOTICE("0x%08x \n", type);
    //LOG_NOTICE("%02x %02x %02x %02x\n", (type >> 24) & 0xff, (type >> 16) & 0xff, (type >> 8) & 0xff, type & 0xff);

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
    mpSACPHeader->header_ext_type = ext_type;
    mpSACPHeader->header_ext_size_1 = (ext_size >> 8) & 0xff;
    mpSACPHeader->header_ext_size_2 = ext_size & 0xff;
}

EECode CSACPIMClientAgentAsync::Register(TChar *name, TChar *password, const TChar *server_url, TU16 serverport)
{
    DASSERT(name);
    DASSERT(password);
    EECode err = EECode_OK;

    ETLV16Type type[2];
    type[0] = ETLV16Type_String;
    type[1] = ETLV16Type_String;

    TU16 length[2] = {0};
    length[0] = strlen(name) + 1;
    length[1] = strlen(password) + 1;

    void *value[2] = {NULL};
    value[0] = (void *)name;
    value[1] = (void *)password;

    TU16 payload_size = length[0] + length[1] + 2 * sizeof(STLV16Header);
    if (DUnlikely((payload_size + mnHeaderLength) >= mSenderBufferSize)) {
        LOG_FATAL("payload string is too long! payload size: %hu\n", payload_size);
        return EECode_BadParam;
    }

    TU8 *p_payload = mpSenderBuffer + mnHeaderLength;
    gfFillTLV16Struct(p_payload, type, length, value, 2);

    logout();

    initSendBuffer(ESACPClientCmdSubType_SinkRegister, EProtocolHeaderExtentionType_SACP_IM, payload_size);

    mSocket = gfNet_ConnectTo(server_url, serverport, SOCK_STREAM, IPPROTO_TCP);
    if (DUnlikely(mSocket == -1)) {
        LOG_FATAL("connect to server fail!\n");
        return EECode_Error;
    }

    err = sendData(mpSenderBuffer, (payload_size + mnHeaderLength));
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    err = recvData(mpReadBuffer, mnHeaderLength);
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    //parse reply from im server
    SSACPHeader *p_header = (SSACPHeader *)mpReadBuffer;
    ESACPConnectResult result = (ESACPConnectResult)p_header->flags;
    if (DUnlikely(result != ESACPConnectResult_OK)) {
        LOG_FATAL("Get reply from server, but result is not ok: 0x%x\n", p_header->flags);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Error;
    }

    mpProtocolHeaderParser->ParseTargetID(mpReadBuffer, mUniqueID);
    mbConnected = 1;

    startReadLoop();

    return EECode_OK;
}

EECode CSACPIMClientAgentAsync::AcquireOwnership(TChar *device_code)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(!device_code)) {
        LOG_FATAL("device code is NULL\n");
        return EECode_BadParam;
    }

    TU16 length = strlen(device_code) + 1;
    TU16 payload_size = length + sizeof(STLV16Header);
    ETLV16Type type = ETLV16Type_String;
    TU8 *p_payload = mpSenderBuffer + mnHeaderLength;
    if (DUnlikely((payload_size + mnHeaderLength) >= mSenderBufferSize)) {
        LOG_FATAL("payload string is too long! payload size: %hu\n", payload_size);
        return EECode_BadParam;
    }

    gfFillTLV16Struct(p_payload, &type, &length, (void **)&device_code);
    initSendBuffer(ESACPClientCmdSubType_SinkOwnSourceDevice, EProtocolHeaderExtentionType_SACP_IM, payload_size, ESACPTypeCategory_ClientCmdChannel);

    EECode err = EECode_OK;
    err = sendData(mpSenderBuffer, (payload_size + mnHeaderLength));
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    return EECode_OK;
}

EECode CSACPIMClientAgentAsync::DonateOwnership(TChar *donate_target, TChar *device_code)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(!donate_target || !device_code)) {
        return EECode_BadParam;
    }

    ETLV16Type type[2];
    type[0] = ETLV16Type_String;
    type[1] = ETLV16Type_String;

    TU16 length[2] = {0};
    length[0] = strlen(donate_target) + 1;
    length[1] = strlen(device_code) + 1;

    void *value[2] = {NULL};
    value[0] = (void *)donate_target;
    value[1] = (void *)device_code;

    TU16 payload_size = length[0] + length[1] + 2 * sizeof(STLV16Header);
    if (DUnlikely((payload_size + mnHeaderLength) >= mSenderBufferSize)) {
        LOG_FATAL("payload string is too long! payload size: %hu\n", payload_size);
        return EECode_BadParam;
    }

    TU8 *p_payload = mpSenderBuffer + mnHeaderLength;
    gfFillTLV16Struct(p_payload, type, length, value, 2);
    initSendBuffer(ESACPClientCmdSubType_SinkDonateSourceDevice, EProtocolHeaderExtentionType_SACP_IM, payload_size, ESACPTypeCategory_ClientCmdChannel);

    EECode err = EECode_OK;
    err = sendData(mpSenderBuffer, (payload_size + mnHeaderLength));
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    return EECode_OK;
}

EECode CSACPIMClientAgentAsync::GrantPrivilegeSingleDevice(TUniqueID device_id, TUniqueID usr_id, TU32 privilege)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    ETLV16Type type = ETLV16Type_AccountID;
    TU16 length = sizeof(TUniqueID);
    TU8 *p_payload = mpSenderBuffer + mnHeaderLength;
    TUniqueID *p_id = &device_id;
    TU16 payload_size = length + sizeof(STLV16Header);
    gfFillTLV16Struct(p_payload, &type, &length, (void **)&p_id);

    //todo
    //fill privilege
    initSendBuffer(ESACPClientCmdSubType_SinkUpdateFriendPrivilege, EProtocolHeaderExtentionType_SACP_IM, payload_size, ESACPTypeCategory_ClientCmdChannel);

    EECode err = EECode_OK;
    err = sendData(mpSenderBuffer, (payload_size + mnHeaderLength));
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    return EECode_OK;
}

EECode CSACPIMClientAgentAsync::GrantPrivilege(TUniqueID *device_id, TU32 device_count, TUniqueID usr_id, TU32 privilege)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(!device_id)) {
        LOG_FATAL("device id is NULL!\n");
        return EECode_BadParam;
    }
    ETLV16Type type = ETLV16Type_AccountID;
    TU16 length = sizeof(TUniqueID);
    TU8 *p_payload = mpSenderBuffer + mnHeaderLength;
    TU16 payload_size = length + sizeof(STLV16Header);
    void *p_value = (void *)&usr_id;
    gfFillTLV16Struct(p_payload, &type, &length, (void **)&p_value);
    p_payload += payload_size;

    type = ETLV16Type_IDList;
    length = sizeof(TUniqueID) * device_count;
    payload_size += length + sizeof(STLV16Header);
    gfFillTLV16Struct(p_payload, &type, &length, (void **)&device_id);
    if (DUnlikely((payload_size + mnHeaderLength) > mSenderBufferSize)) {
        //todo
        LOG_FATAL("sender buffer(%u) is not enough for sending id list(%u)!\n", mSenderBufferSize, (payload_size + mnHeaderLength));
        return EECode_InternalParamsBug;
    }

    initSendBuffer(ESACPClientCmdSubType_SinkUpdateFriendPrivilege, EProtocolHeaderExtentionType_SACP_IM, payload_size, ESACPTypeCategory_ClientCmdChannel);

    EECode err = EECode_OK;
    err = sendData(mpSenderBuffer, (payload_size + mnHeaderLength));
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    return EECode_OK;
}

EECode CSACPIMClientAgentAsync::AddFriend(TChar *friend_user_name)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(!friend_user_name)) {
        return EECode_BadParam;
    }

    TU16 length = strlen(friend_user_name) + 1;
    TU16 payload_size = length + sizeof(STLV16Header);
    ETLV16Type type = ETLV16Type_String;
    TU8 *p_payload = mpSenderBuffer + mnHeaderLength;
    if (DUnlikely((payload_size + mnHeaderLength) >= mSenderBufferSize)) {
        LOG_FATAL("payload string is too long! payload size: %hu\n", payload_size);
        return EECode_BadParam;
    }

    gfFillTLV16Struct(p_payload, &type, &length, (void **)&friend_user_name);
    initSendBuffer(ESACPClientCmdSubType_SinkInviteFriend, EProtocolHeaderExtentionType_SACP_IM, payload_size, ESACPTypeCategory_ClientCmdChannel);

    EECode err = EECode_OK;
    err = sendData(mpSenderBuffer, (payload_size + mnHeaderLength));
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    return EECode_OK;
}

EECode CSACPIMClientAgentAsync::AcceptFriend(TUniqueID friend_id)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(!friend_id)) {
        return EECode_BadParam;
    }

    TU16 length = sizeof(TUniqueID);
    TU16 payload_size = length + sizeof(STLV16Header);
    ETLV16Type type = ETLV16Type_AccountID;
    TU8 *p_payload = mpSenderBuffer + mnHeaderLength;
    if (DUnlikely((payload_size + mnHeaderLength) >= mSenderBufferSize)) {
        LOG_FATAL("payload string is too long! payload size: %hu\n", payload_size);
        return EECode_BadParam;
    }
    TUniqueID *p_id = &friend_id;
    gfFillTLV16Struct(p_payload, &type, &length, (void **)&p_id);
    initSendBuffer(ESACPClientCmdSubType_SinkAcceptFriend, EProtocolHeaderExtentionType_SACP_IM, payload_size, ESACPTypeCategory_ClientCmdChannel);


    EECode err = EECode_OK;
    err = sendData(mpSenderBuffer, (payload_size + mnHeaderLength));
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    return EECode_OK;
}

EECode CSACPIMClientAgentAsync::RemoveFriend(TUniqueID friend_id)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(!friend_id)) {
        return EECode_BadParam;
    }

    TU16 length = sizeof(TUniqueID);
    TU16 payload_size = length + sizeof(STLV16Header);
    ETLV16Type type = ETLV16Type_AccountID;
    TU8 *p_payload = mpSenderBuffer + mnHeaderLength;
    if (DUnlikely((payload_size + mnHeaderLength) >= mSenderBufferSize)) {
        LOG_FATAL("payload string is too long! payload size: %hu\n", payload_size);
        return EECode_BadParam;
    }
    TUniqueID *p_id = &friend_id;
    gfFillTLV16Struct(p_payload, &type, &length, (void **)&p_id);
    initSendBuffer(ESACPClientCmdSubType_SinkRemoveFriend, EProtocolHeaderExtentionType_SACP_IM, payload_size, ESACPTypeCategory_ClientCmdChannel);


    EECode err = EECode_OK;
    err = sendData(mpSenderBuffer, (payload_size + mnHeaderLength));
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    return EECode_OK;
}

EECode CSACPIMClientAgentAsync::RetrieveFriendList()
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    initSendBuffer(ESACPClientCmdSubType_SinkRetrieveFriendsList, EProtocolHeaderExtentionType_SACP_IM, 0, ESACPTypeCategory_ClientCmdChannel);

    //todo, tlv struct
    EECode err = EECode_OK;
    err = sendData(mpSenderBuffer, mnHeaderLength);
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    return EECode_OK;
}

EECode CSACPIMClientAgentAsync::RetrieveAdminDeviceList()
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    initSendBuffer(ESACPClientCmdSubType_SinkRetrieveAdminDeviceList, EProtocolHeaderExtentionType_SACP_IM, 0, ESACPTypeCategory_ClientCmdChannel);

    //todo, tlv struct
    EECode err = EECode_OK;
    err = sendData(mpSenderBuffer, mnHeaderLength);
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    return EECode_OK;
}

EECode CSACPIMClientAgentAsync::RetrieveSharedDeviceList()
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    initSendBuffer(ESACPClientCmdSubType_SinkRetrieveSharedDeviceList, EProtocolHeaderExtentionType_SACP_IM, 0, ESACPTypeCategory_ClientCmdChannel);

    EECode err = EECode_OK;
    err = sendData(mpSenderBuffer, mnHeaderLength);
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    return EECode_OK;
}

EECode CSACPIMClientAgentAsync::QueryDeivceInfo(TUniqueID device_id)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    ETLV16Type type = ETLV16Type_AccountID;
    TU16 length = sizeof(TUniqueID);
    TU8 *p_payload = mpSenderBuffer + mnHeaderLength;
    TU16 payload_size = length + sizeof(STLV16Header);
    TUniqueID *p_id = &device_id;
    gfFillTLV16Struct(p_payload, &type, &length, (void **)&p_id);
    initSendBuffer(ESACPClientCmdSubType_SinkQuerySource, EProtocolHeaderExtentionType_SACP_IM, payload_size, ESACPTypeCategory_ClientCmdChannel);

    EECode err = EECode_OK;
    err = sendData(mpSenderBuffer, mnHeaderLength + payload_size);
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    return EECode_OK;
}

void CSACPIMClientAgentAsync::SetIdentificationString(TChar *p_id_string, TU32 length)
{
    if (DUnlikely(!p_id_string || 0 == length || strlen(p_id_string) != length)) {
        LOG_ERROR("invalid params, p_id_string=%p(strlen=%ld), length=%u\n", p_id_string, (TULong)strlen(p_id_string), length);
        return;
    }
    if (length > DIdentificationStringLength) {
        LOG_ERROR("SetIdentificationString len=%u invalid(should<=%d)\n", length, DIdentificationStringLength);
        return;
    }
    memset(mIdentityString, 0x0, sizeof(mIdentityString));
    memcpy(mIdentityString, p_id_string, length);
    mDeviceIdentityStringLen = length;
    //LOG_INFO("[SHOULD HIDE]identity:%s, length=%d\n", mIdentityString, mDeviceIdentityStringLen);
}

EECode CSACPIMClientAgentAsync::Login(TUniqueID id, TChar *authencation_string, TChar *server_url, TU16 server_port, TU16 local_port, TChar *&cloud_server_url, TU16 &cloud_server_port, TU64 &dynamic_code_input)
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ConnectionTag, ESACPClientCmdSubType_SourceAuthentication);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    //AUTO_LOCK(mpMutex);

    mLocalPort = local_port;
    mServerPort = server_port;

    if (DUnlikely(!server_url)) {
        LOG_ERROR("NULL server_url\n");
        return EECode_BadParam;
    } else {
        LOG_NOTICE("server_url %s, local_port %hu, server_port %hu\n", server_url, mLocalPort, mServerPort);
    }

    if (DUnlikely(!authencation_string)) {
        LOG_ERROR("NULL authencation_string\n");
        return EECode_BadParam;
    }

    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    mSocket = gfNet_ConnectTo((const TChar *)server_url, mServerPort, SOCK_STREAM, IPPROTO_TCP);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("gfNet_ConnectTo(%s:%hu) fail\n", server_url, local_port);
        mLastErrorCode = EECode_NetConnectFail;
        mLastErrorHint1 = mSocket;
        mLastErrorHint2 = 0;
        return EECode_NetConnectFail;
    }

    TU32 payload_length = strlen(authencation_string) + sizeof(TUniqueID);
    TU16 ext_size = (TU16)sizeof(TUniqueID);
    TU8 ext_type = (TU8)EProtocolHeaderExtentionType_SACP_IM;

    fillHeader(type, payload_length, ext_size, ext_type);

    p_payload += ext_size;
    *p_payload++ = (id >> 56) & 0xff;
    *p_payload++ = (id >> 48) & 0xff;
    *p_payload++ = (id >> 40) & 0xff;
    *p_payload++ = (id >> 32) & 0xff;
    *p_payload++ = (id >> 24) & 0xff;
    *p_payload++ = (id >> 16) & 0xff;
    *p_payload++ = (id >> 8) & 0xff;
    *p_payload++ = id & 0xff;

    memcpy(p_payload, authencation_string, strlen(authencation_string));

    gfSocketSetTimeout(mSocket, 2, 0);

    /*
        LOG_NOTICE("before send, size %d, type %08x\n", (sizeof(SSACPHeader) + authentication_length), type);
        LOG_NOTICE("data %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x\n", \
            mpSenderBuffer[0], mpSenderBuffer[1], mpSenderBuffer[2], mpSenderBuffer[3], \
            mpSenderBuffer[4], mpSenderBuffer[5], mpSenderBuffer[6], mpSenderBuffer[7], \
            mpSenderBuffer[8], mpSenderBuffer[9], mpSenderBuffer[10], mpSenderBuffer[11], \
            mpSenderBuffer[12], mpSenderBuffer[13], mpSenderBuffer[14], mpSenderBuffer[15]);
    */

    ret = gfNet_Send_timeout(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + payload_length + ext_size), 0, 1);
    if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + payload_length + ext_size))) {
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = ret;
        mLastErrorHint2 = sizeof(SSACPHeader) + payload_length + ext_size;
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
        //continue to receive payload, generate dynamic_code1 from dynamic_input1
        ret = gfNet_Recv_timeout(mSocket, mpSenderBuffer, DDynamicInputStringLength, DNETUTILS_RECEIVE_FLAG_READ_ALL, 20);
        if (DUnlikely(DDynamicInputStringLength != ret)) {
            LOG_WARN("Login, recv dynamic_input1 %d not expected, expect %d, peer closed?\n", ret, DDynamicInputStringLength);
            mLastErrorCode = EECode_NetReceivePayload_Fail;
            mLastErrorHint1 = ret;
            mLastErrorHint2 = DDynamicInputStringLength;
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetReceivePayload_Fail;
        }
        strncpy((TChar *)(mpSenderBuffer + DDynamicInputStringLength), mIdentityString, DIdentificationStringLength);
        TU32 dynamic_code = gfHashAlgorithmV6(mpSenderBuffer, DDynamicInputStringLength + DIdentificationStringLength);

        //send dynamic_code1 to server
        payload_length = sizeof(dynamic_code);
        fillHeader(type, payload_length, ext_size, ext_type);
        p_payload = mpSenderBuffer + sizeof(SSACPHeader);
        *p_payload++ = (dynamic_code >> 24) & 0xff;
        *p_payload++ = (dynamic_code >> 16) & 0xff;
        *p_payload++ = (dynamic_code >> 8) & 0xff;
        *p_payload++ = dynamic_code & 0xff;

        ret = gfNet_Send_timeout(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + payload_length + ext_size), 0, 1);
        if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + payload_length + ext_size))) {
            mLastErrorCode = EECode_NetSendHeader_Fail;
            mLastErrorHint1 = ret;
            mLastErrorHint2 = sizeof(SSACPHeader) + payload_length + ext_size;
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetSendHeader_Fail;
        }

        //receive header of server url:port:dynamic_code_input2 first
        ret = gfNet_Recv_timeout(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 2;
            mLastErrorHint2 = sizeof(SSACPHeader);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)) || (ESACPConnectResult_OK != mpSACPHeader->flags))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = ret;
            mLastErrorHint2 = sizeof(SSACPHeader);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetReceiveHeader_Fail;
        }
        //receive cloud server url:port:dynamic_code_input2 from im server
        payload_length = ((TU32)mpSACPHeader->size_0 << 8) | ((TU32)mpSACPHeader->size_1 << 8) | (TU32)mpSACPHeader->size_2;
        ret = gfNet_Recv_timeout(mSocket, mpSenderBuffer, (TInt)payload_length, DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 2;
            mLastErrorHint2 = sizeof(SSACPHeader);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)payload_length)) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %hu\n", ret, payload_length);
            mLastErrorCode = EECode_NetReceivePayload_Fail;
            mLastErrorHint1 = ret;
            mLastErrorHint2 = payload_length;
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetReceivePayload_Fail;
        }
        TChar *p_data = NULL, *p_data1 = NULL;
        p_data = (TChar *) strchr((TChar *)mpSenderBuffer, ':');
        p_data1 = (TChar *) strchr(p_data + 1, ':');
        if (!p_data || !p_data1 || (sizeof(TU16) != p_data1 - (p_data + 1)) || (sizeof(TU64) != (TChar *)mpSenderBuffer + payload_length - (p_data1 + 1))) {
            LOG_ERROR("gfNet_Recv(), params format error, cloud_server_url len=%d, cloud_server_port len=%d(should be 2), dynamic_code_input len=%d(should be 8)\n",
                      (TInt)(p_data - (TChar *)mpSenderBuffer), (TInt)(p_data1 - (p_data + 1)), (TInt)((TChar *)mpSenderBuffer + payload_length - (p_data1 + 1)));
            mLastErrorCode = EECode_InternalParamsBug;
            mLastErrorHint1 = 0;
            mLastErrorHint2 = payload_length;
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_InternalParamsBug;
        }
        memset(mCloudServerUrl, 0x0, sizeof(mCloudServerUrl));
        strncpy(mCloudServerUrl, (TChar *)mpSenderBuffer, p_data - (TChar *)mpSenderBuffer);
        cloud_server_url = mCloudServerUrl;
        cloud_server_port = (((TU16)(p_data[1])) << 8) | (TU16)(p_data[2]);
        dynamic_code_input = (((TU64)(p_data1[1])) << 56) | (((TU64)(p_data1[2])) << 48) | (((TU64)(p_data1[3])) << 40) | (((TU64)(p_data1[4])) << 32) | \
                             (((TU64)(p_data1[5])) << 24) | (((TU64)(p_data1[6])) << 16) | (((TU64)(p_data1[7])) << 8) | (TU64)(p_data1[8]);
        mbConnected = 1;
        LOG_INFO("client recv: url=%s, port=%hu, dyanmic_input2=%llx\n", cloud_server_url, cloud_server_port, dynamic_code_input);
        LOG_NOTICE("connect success\n");
        return EECode_OK;
    } else if (ESACPConnectResult_Reject_NoSuchChannel == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_NoSuchChannel, %s\n", (TChar *)authencation_string);
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_NoSuchChannel;
    } else if (ESACPConnectResult_Reject_ChannelIsInUse == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_ChannelIsInUse, %s\n", (TChar *)authencation_string);
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_ChannelIsBusy;
    } else if (ESACPConnectResult_Reject_BadRequestFormat == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_BadRequestFormat, %s\n", (TChar *)authencation_string);
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_BadRequestFormat;
    } else if (ESACPConnectResult_Reject_CorruptedProtocol == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_CorruptedProtocol, %s\n", (TChar *)authencation_string);
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_CorruptedProtocol;
    } else if (ESACPConnectResult_Reject_AuthenticationDataTooLong == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_AuthenticationDataTooLong, %s\n", (TChar *)authencation_string);
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_AuthenticationDataTooLong;
    } else if (ESACPConnectResult_Reject_NotProperPassword == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_NotProperPassword, %s\n", (TChar *)authencation_string);
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_NotProperPassword;
    } else if (ESACPConnectResult_Reject_NotSupported == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_NotSupported, %s\n", (TChar *)authencation_string);
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_NotSupported;
    } else if (ESACPConnectResult_Reject_AuthenticationFail == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_AuthenticationFail, %s\n", (TChar *)authencation_string);
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_AuthenticationFail;
    } else {
        LOG_ERROR("unknown error, %s, mpSACPHeader->flags %08x\n", (TChar *)authencation_string, mpSACPHeader->flags);
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_Unknown;
    }

    mbConnected = 1;

    return EECode_OK;
}

EECode CSACPIMClientAgentAsync::LoginUserAccount(TChar *name, TChar *password, const TChar *server_url, TU16 serverport)
{
    //todo
    //authentication
    DASSERT(name);
    DASSERT(password);
    EECode err = EECode_OK;

    //todo, tlv struct
    ETLV16Type type[2];
    type[0] = ETLV16Type_String;
    type[1] = ETLV16Type_String;

    TU16 length[2] = {0};
    length[0] = strlen(name) + 1;
    length[1] = strlen(password) + 1;

    void *value[2] = {NULL};
    value[0] = (void *)name;
    value[1] = (void *)password;

    TU16 payload_size = length[0] + length[1] + 2 * sizeof(STLV16Header);
    if (DUnlikely((payload_size + mnHeaderLength) >= mSenderBufferSize)) {
        LOG_FATAL("payload string is too long! payload size: %hu\n", payload_size);
        return EECode_BadParam;
    }

    TU8 *p_payload = mpSenderBuffer + mnHeaderLength;
    gfFillTLV16Struct(p_payload, type, length, value, 2);
    initSendBuffer(ESACPClientCmdSubType_SinkAuthentication, EProtocolHeaderExtentionType_SACP_IM, payload_size);

    logout();

    mSocket = gfNet_ConnectTo(server_url, serverport, SOCK_STREAM, IPPROTO_TCP);
    if (DUnlikely(mSocket == -1)) {
        LOG_FATAL("connect to server fail!\n");
        return EECode_Error;
    }

    err = sendData(mpSenderBuffer, (payload_size + mnHeaderLength));
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    err = recvData(mpReadBuffer, mnHeaderLength);
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    //parse reply from im server
    SSACPHeader *p_header = (SSACPHeader *)mpReadBuffer;
    ESACPConnectResult result = (ESACPConnectResult)p_header->flags;
    if (DUnlikely(result != ESACPConnectResult_OK)) {
        LOG_FATAL("Get reply from server, but result is not ok: 0x%x\n", p_header->flags);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Error;
    }

    mpProtocolHeaderParser->ParseTargetID(mpReadBuffer, mUniqueID);
    mbConnected = 1;

    startReadLoop();

    return EECode_OK;
}

EECode CSACPIMClientAgentAsync::Logout()
{
    return logout();
}

EECode CSACPIMClientAgentAsync::logout()
{
    stopReadLoop();

    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    mbConnected = 0;

    return EECode_OK;
}

EECode CSACPIMClientAgentAsync::startReadLoop()
{
    if (mbStartReadLoop) {
        LOG_WARN("already started\n");
        return EECode_BadState;
    }

    mbStartReadLoop = 1;
    mpThread = gfCreateThread("CSACPIMClientAgentAsync", __Entry, (void *)(this));

    return EECode_OK;
}

void CSACPIMClientAgentAsync::stopReadLoop()
{
    if (!mbStartReadLoop) {
        LOG_WARN("not start yet\n");
        return;
    }
    mbStartReadLoop = 0;

    gfOSWritePipe(mPipeFd[1], 'q');

    mpThread->Delete();
    mpThread = NULL;
}

EECode CSACPIMClientAgentAsync::SetReceiveMessageCallBack(void *owner, TReceiveMessageCallBack callback)
{
    DASSERT(owner);
    DASSERT(callback);

    if (DLikely(owner && callback)) {
        if (DUnlikely(mpMessageCallbackOwner || mpMessageCallback)) {
            LOG_WARN("replace data callback, ori mpMessageCallbackOwner %p, mpMessageCallback %p, current owner %p callback %p\n", mpMessageCallbackOwner, mpMessageCallback, owner, callback);
        }
        mpMessageCallbackOwner = owner;
        mpMessageCallback = callback;
    } else {
        LOG_FATAL("NULL onwer %p, callback %p\n", owner, callback);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CSACPIMClientAgentAsync::PostMessage(TUniqueID id, TU8 *message, TU32 message_length)
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPDebugCmdSubType_Customized);

    TInt ret = 0;
    TU32 transfer_size = 0;

    if (DUnlikely(!message || !message_length)) {
        LOG_ERROR("BAD params %p, size %d\n", message, message_length);
        mLastErrorCode = EECode_BadParam;
        mLastErrorHint1 = 100;
        mLastErrorHint2 = (TU32)message_length;
        return EECode_BadParam;
    }

    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need login first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = EECodeHint_BadSocket;
        mLastErrorHint2 = (TU32)mSocket;
        return EECode_BadState;
    }

    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);

    mpSACPHeader->type_1 = (type >> 24) & 0xff;
    mpSACPHeader->type_2 = (type >> 16) & 0xff;
    mpSACPHeader->type_3 = (type >> 8) & 0xff;
    mpSACPHeader->type_4 = type & 0xff;

    mpSACPHeader->encryption_type_1 = mEncryptionType1;
    mpSACPHeader->encryption_type_2 = mEncryptionType2;

    //mpSACPHeader->flags = 0;
    mpSACPHeader->header_ext_type = EProtocolHeaderExtentionType_SACP_IM;
    mpSACPHeader->header_ext_size_1 = 0;
    mpSACPHeader->header_ext_size_2 = 0;

    mpSACPHeader->flags = DSACPHeaderFlagBit_PacketStartIndicator;

    *p_payload++ = (id >> 56) & 0xff;
    *p_payload++ = (id >> 48) & 0xff;
    *p_payload++ = (id >> 40) & 0xff;
    *p_payload++ = (id >> 32) & 0xff;
    *p_payload++ = (id >> 24) & 0xff;
    *p_payload++ = (id >> 16) & 0xff;
    *p_payload++ = (id >> 8) & 0xff;
    *p_payload++ = id & 0xff;

    do {

        if ((mSenderBufferSize - mnHeaderLength) >= message_length) {
            transfer_size = message_length;
            message_length = 0;
            mpSACPHeader->flags |= DSACPHeaderFlagBit_PacketEndIndicator;
        } else {
            transfer_size = mSenderBufferSize - mnHeaderLength;
            message_length -= transfer_size;
        }

        mpSACPHeader->size_1 = (transfer_size >> 8) & 0xff;
        mpSACPHeader->size_2 = transfer_size & 0xff;
        mpSACPHeader->size_0 = (transfer_size >> 16) & 0xff;

        mpSACPHeader->seq_count_0 = (mSeqCount >> 16) & 0xff;
        mpSACPHeader->seq_count_1 = (mSeqCount >> 8) & 0xff;
        mpSACPHeader->seq_count_2 = mSeqCount & 0xff;
        mSeqCount ++;

        ret = gfNet_Send_timeout(mSocket, mpSenderBuffer, (TInt)mnHeaderLength, 0, 2);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 5;
            mLastErrorHint2 = (TU32)mSocket;

            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;

            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)mnHeaderLength)) {
            LOG_ERROR("ret not expected %d, header %d\n", ret, mnHeaderLength);
            mLastErrorCode = EECode_NetSendHeader_Fail;
            mLastErrorHint1 = ret;
            mLastErrorHint2 = (TU32)mSocket;

            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;

            return EECode_NetSendHeader_Fail;
        }

        ret = gfNet_Send_timeout(mSocket, message, (TInt)(transfer_size), 0, 2);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 6;
            mLastErrorHint2 = (TU32)mSocket;

            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;

            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(transfer_size))) {
            LOG_ERROR("ret not expected %d, transfer_size %u\n", ret, transfer_size);
            mLastErrorCode = EECode_NetSendPayload_Fail;
            mLastErrorHint1 = ret;
            mLastErrorHint2 = (TU32)mSocket;

            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;

            return EECode_NetSendPayload_Fail;
        }
        message += transfer_size;

        //LOG_NOTICE("transfer_size %ld, remainning %ld, flag %02x\n", transfer_size, size, mpSACPHeader->flags);

        mpSACPHeader->flags &= ~(DSACPHeaderFlagBit_PacketStartIndicator);

    } while (message_length);

    return EECode_OK;
}

EECode CSACPIMClientAgentAsync::SendMessage(TUniqueID id, TU8 *message, TU32 message_length)
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request | DSACPTypeBit_NeedReply, ESACPTypeCategory_ClientCmdChannel, ESACPDebugCmdSubType_Customized);

    TInt ret = 0;
    TU32 transfer_size = 0;

    if (DUnlikely(!message || !message_length)) {
        LOG_ERROR("BAD params %p, size %d\n", message, message_length);
        mLastErrorCode = EECode_BadParam;
        mLastErrorHint1 = 100;
        mLastErrorHint2 = (TU32)message_length;
        return EECode_BadParam;
    }

    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need login first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = EECodeHint_BadSocket;
        mLastErrorHint2 = (TU32)mSocket;
        return EECode_BadState;
    }

    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);

    mpSACPHeader->type_1 = (type >> 24) & 0xff;
    mpSACPHeader->type_2 = (type >> 16) & 0xff;
    mpSACPHeader->type_3 = (type >> 8) & 0xff;
    mpSACPHeader->type_4 = type & 0xff;

    mpSACPHeader->encryption_type_1 = mEncryptionType1;
    mpSACPHeader->encryption_type_2 = mEncryptionType2;

    //mpSACPHeader->flags = 0;
    mpSACPHeader->header_ext_type = EProtocolHeaderExtentionType_SACP_IM;
    mpSACPHeader->header_ext_size_1 = 0;
    mpSACPHeader->header_ext_size_2 = 0;

    mpSACPHeader->flags = DSACPHeaderFlagBit_PacketStartIndicator;

    *p_payload++ = (id >> 56) & 0xff;
    *p_payload++ = (id >> 48) & 0xff;
    *p_payload++ = (id >> 40) & 0xff;
    *p_payload++ = (id >> 32) & 0xff;
    *p_payload++ = (id >> 24) & 0xff;
    *p_payload++ = (id >> 16) & 0xff;
    *p_payload++ = (id >> 8) & 0xff;
    *p_payload++ = id & 0xff;

    do {

        if ((mSenderBufferSize - mnHeaderLength) >= message_length) {
            transfer_size = message_length;
            message_length = 0;
            mpSACPHeader->flags |= DSACPHeaderFlagBit_PacketEndIndicator;
        } else {
            transfer_size = mSenderBufferSize - mnHeaderLength;
            message_length -= transfer_size;
        }

        mpSACPHeader->size_1 = (transfer_size >> 8) & 0xff;
        mpSACPHeader->size_2 = transfer_size & 0xff;
        mpSACPHeader->size_0 = (transfer_size >> 16) & 0xff;

        mpSACPHeader->seq_count_0 = (mSeqCount >> 16) & 0xff;
        mpSACPHeader->seq_count_1 = (mSeqCount >> 8) & 0xff;
        mpSACPHeader->seq_count_2 = mSeqCount & 0xff;
        mSeqCount ++;

        ret = gfNet_Send_timeout(mSocket, mpSenderBuffer, (TInt)mnHeaderLength, 0, 2);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 5;
            mLastErrorHint2 = (TU32)mSocket;

            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;

            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)mnHeaderLength)) {
            LOG_ERROR("ret not expected %d, header %d\n", ret, mnHeaderLength);
            mLastErrorCode = EECode_NetSendHeader_Fail;
            mLastErrorHint1 = ret;
            mLastErrorHint2 = (TU32)mSocket;

            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;

            return EECode_NetSendHeader_Fail;
        }

        ret = gfNet_Send_timeout(mSocket, message, (TInt)(transfer_size), 0, 2);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 6;
            mLastErrorHint2 = (TU32)mSocket;

            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;

            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(transfer_size))) {
            LOG_ERROR("ret not expected %d, transfer_size %u\n", ret, transfer_size);
            mLastErrorCode = EECode_NetSendPayload_Fail;
            mLastErrorHint1 = ret;
            mLastErrorHint2 = (TU32)mSocket;

            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;

            return EECode_NetSendPayload_Fail;
        }
        message += transfer_size;

        //LOG_NOTICE("transfer_size %ld, remainning %ld, flag %02x\n", transfer_size, size, mpSACPHeader->flags);

        mpSACPHeader->flags &= ~(DSACPHeaderFlagBit_PacketStartIndicator);

    } while (message_length);

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
        LOG_NOTICE("connect success\n");
        return EECode_OK;
    } else if (ESACPConnectResult_Reject_NoSuchChannel == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_NoSuchChannel\n");
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_NoSuchChannel;
    } else if (ESACPConnectResult_Reject_ChannelIsInUse == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_ChannelIsInUse\n");
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_ChannelIsBusy;
    } else if (ESACPConnectResult_Reject_BadRequestFormat == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_BadRequestFormat\n");
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_BadRequestFormat;
    } else if (ESACPConnectResult_Reject_CorruptedProtocol == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_CorruptedProtocol\n");
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_CorruptedProtocol;
    } else if (ESACPConnectResult_Reject_AuthenticationDataTooLong == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_AuthenticationDataTooLong\n");
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_AuthenticationDataTooLong;
    } else if (ESACPConnectResult_Reject_NotProperPassword == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_NotProperPassword\n");
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_NotProperPassword;
    } else if (ESACPConnectResult_Reject_NotSupported == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_NotSupported\n");
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_NotSupported;
    } else if (ESACPConnectResult_Reject_AuthenticationFail == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_AuthenticationFail\n");
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_AuthenticationFail;
    } else {
        LOG_ERROR("unknown error, mpSACPHeader->flags %08x\n", mpSACPHeader->flags);
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_Unknown;
    }

    return EECode_OK;
}

void CSACPIMClientAgentAsync::Destroy()
{
    delete this;
}

EECode CSACPIMClientAgentAsync::__Entry(void *param)
{
    CSACPIMClientAgentAsync *thiz = (CSACPIMClientAgentAsync *)param;
    thiz->ReadLoop(NULL);

    LOG_NOTICE("after ReadLoop\n");
    return EECode_OK;
}

EECode CSACPIMClientAgentAsync::ReadLoop(void *param)
{
    fd_set mAllSet;
    fd_set mReadSet;

    FD_ZERO(&mAllSet);
    FD_SET(mPipeFd[0], &mAllSet);
    FD_SET(mSocket, &mAllSet);
    if (mSocket > mPipeFd[0]) {
        mMaxFd = mSocket;
    } else {
        mMaxFd = mPipeFd[0];
    }

    EECode err = EECode_OK;
    TU32 payload_size = 0, reqres_bits = 0, cat = 0, sub_type = 0;
    TInt nready = 0;

    TChar char_buffer = 0;
    for (;;) {
        mReadSet = mAllSet;
        LOG_NOTICE("select IN\n");
        nready = select(mMaxFd + 1, &mReadSet, NULL, NULL, NULL);
        LOG_NOTICE("[Data thread]: after select. nready %d\n", nready);

        //process cmd
        if (FD_ISSET(mPipeFd[0], &mReadSet)) {
            //some input from engine, process cmd first
            //LOG_NOTICE("[Data thread]: from pipe fd.\n");
            nready --;
            gfOSReadPipe(mPipeFd[0], char_buffer);
            if ('q' == char_buffer) {
                //LOG_NOTICE("quit 1\n");
                return EECode_OK;
            } else {
                LOG_ERROR("Unknown cmd %c.\n", char_buffer);
                return EECode_Error;
            }
        }

        if (nready < 1) {
            continue;
        }

        if (DLikely(FD_ISSET(mSocket, &mReadSet))) {

            err = recvData(mpReadBuffer, mnHeaderLength);
            if (DLikely(err == EECode_OK)) {
                mpProtocolHeaderParser->Parse(mpReadBuffer, payload_size, reqres_bits, cat, sub_type);
                TU8 *p_payload = NULL;
                if (DUnlikely((payload_size + mnHeaderLength) >= mReadBufferSize)) {
                    if (mPayloadBufferSize <= payload_size) {
                        if (mpPayloadBuffer) { free(mpPayloadBuffer); }
                        mpPayloadBuffer = (TU8 *)DDBG_MALLOC(payload_size + 1, "ICAP");
                        if (DUnlikely(!mpPayloadBuffer)) {
                            LOG_FATAL("malloc payload buffer fail!\n");
                            return EECode_NoMemory;
                        }
                        mPayloadBufferSize = payload_size + 1;
                    }
                    p_payload = mpPayloadBuffer;
                } else if (payload_size > 0) {
                    p_payload = mpReadBuffer + mnHeaderLength;
                }

                if (DLikely(p_payload)) {
                    err = recvData(p_payload, payload_size);
                    if (DUnlikely(EECode_Closed == err)) {
                        LOG_FATAL("socket is closed!\n");
                        return err;
                    } else if (DUnlikely(EECode_TimeOut == err)) {
                        LOG_FATAL("recv data time out!\n");
                        continue;
                    }
                }

                err = handleMessage(reqres_bits, cat, sub_type, p_payload, payload_size);
                if (DUnlikely(err != EECode_OK)) {
                    LOG_FATAL("handleMsg fail! %s\n", gfGetErrorCodeString(err));
                    continue;
                }
            } else if (EECode_Closed == err) {
                LOG_NOTICE("recvData fail, socket is cloesd!\n");
                gfNetCloseSocket(mSocket);
                mSocket = DInvalidSocketHandler;
                return err;
            } else {
                LOG_FATAL("recvData fail, err %d, %s\n", err, gfGetErrorCodeString(err));
                gfNetCloseSocket(mSocket);
                mSocket = DInvalidSocketHandler;
                return err;
            }
        } else {
            LOG_FATAL("why comes here?\n");
            return EECode_InternalLogicalBug;
        }
    }

    return EECode_OK;
}

EECode CSACPIMClientAgentAsync::sendData(TU8 *data, TU16 size, TU8 maxcount)
{
    TInt sentlength = 0;

    LOG_NOTICE("CSACPIMClientAgentAsync: sendData, size %hu\n", size);

    sentlength = gfNet_Send_timeout(mSocket, data, (TInt)size, 0, maxcount);
    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, socket is closed!\n");
        return EECode_Closed;
    } else if (DUnlikely(sentlength < 0)) {
        LOG_FATAL("send data time out!max count: %d\n", maxcount);
        return EECode_TimeOut;
    }

    return EECode_OK;
}

EECode CSACPIMClientAgentAsync::recvData(TU8 *data, TU16 size, TU8 maxcount)
{
    TInt recvlength = 0;

    recvlength = gfNet_Recv_timeout(mSocket, data, (TInt)size, DNETUTILS_RECEIVE_FLAG_READ_ALL, maxcount);
    if (DUnlikely(recvlength == 0)) {
        LOG_FATAL("recv data fail, socket is closed.\n");
        return EECode_Closed;
    } else if (DUnlikely(recvlength < (TInt)size)) {
        LOG_FATAL("recv data time out! max count: %d\n", maxcount);
        return EECode_TimeOut;
    }

    return EECode_OK;
}

void CSACPIMClientAgentAsync::initSendBuffer(ESACPClientCmdSubType sub_type, EProtocolHeaderExtentionType ext_type, TU32 payload_size, ESACPTypeClientCategory cat_type)
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request, cat_type, sub_type);
    fillHeader(type, payload_size, mnExtensionLength, ext_type);
    if (ext_type == EProtocolHeaderExtentionType_SACP_IM) {
        mpProtocolHeaderParser->FillSourceID(mpSenderBuffer, 0);
    } else {
        mpProtocolHeaderParser->FillSourceID(mpSenderBuffer, mUniqueID);
    }
    return;
}

EECode CSACPIMClientAgentAsync::handleMessage(TU32 reqres_bits, TU32 cat, TU32 sub_type, TU8 *payload, TU16 payload_size)
{
#if 0
    EECode err = EECode_OK;
    TU8 *p_data = NULL;
    TU16 length = 0;
    TUniqueID id = 0;
    SSACPHeader *p_header = (SSACPHeader *)mpReadBuffer;
    ETLV16Type value_type = ETLV16Type_String;
    if ((cat == ESACPTypeCategory_ClientCmdChannel) && (reqres_bits == (TU32)DSACPTypeBit_Responce)) {

        //msg from im server
        if (DLikely(ESACPConnectResult_OK == p_header->flags)) {
            switch (sub_type) {
                case ESACPClientCmdSubType_SinkQuerySource: {
                        ETLV16Type type[2];
                        TU16 value_length[2] = {0};
                        void *value[4] = {NULL};
                        gfParseTLV16Struct(payload, type, value_length, value, 2);
                        if (DUnlikely(ETLV16Type_SourceDeviceDataServerAddress != type[0])) {
                            LOG_FATAL("handleMessage: cmd(SinkQuerySource), reply type is 0x%x, not ETLV16Type_SourceDeviceDataServerAddress\n", type[0]);
                            return EECode_Error;
                        }

                        if (DUnlikely(ETLV16Type_SourceDeviceStreamingTag != type[1])) {
                            LOG_FATAL("handleMessage: cmd(SinkQuerySource), reply type is 0x%x, not ETLV16Type_SourceDeviceStreamingTag\n", type[1]);
                            return EECode_Error;
                        }

                        err = mpMessageCallback(mpMessageCallbackOwner, EECode_OK, sub_type, reqres_bits, ETLV16Type_SourceDeviceDataServerAddress, (TU8 *)value[0], value_length[0]);
                        if (DUnlikely(EECode_OK != err)) {
                            LOG_FATAL("mpMessageCallback fail!\n");
                            //break;
                        }

                        err = mpMessageCallback(mpMessageCallbackOwner, EECode_OK, sub_type, reqres_bits, ETLV16Type_SourceDeviceStreamingTag, (TU8 *)value[1], value_length[1]);
                        if (DUnlikely(EECode_OK != err)) {
                            LOG_FATAL("mpMessageCallback fail!\n");
                            //break;
                        }

                        payload += value_length[0] + value_length[1] + sizeof(ETLV16Type) * 2;
                        TSocketPort cloud_port = 0, rtsp_streaming_port = 0;
                        value[0] = (void *)&cloud_port;
                        value[1] = (void *)&rtsp_streaming_port;
                        gfParseTLV16Struct(payload, type, value_length, value, 2);
                        DASSERT(ETLV16Type_SourceDeviceUploadingPort == type[0]);
                        DASSERT(ETLV16Type_SourceDeviceStreamingPort == type[1]);

                        err = mpMessageCallback(mpMessageCallbackOwner, EECode_OK, sub_type, reqres_bits, ETLV16Type_SourceDeviceUploadingPort, (TU8 *)value[0], value_length[0]);
                        if (DUnlikely(EECode_OK != err)) {
                            LOG_FATAL("mpMessageCallback fail!\n");
                            //break;
                        }

                        //last value
                        p_data = (TU8 *)value[1];
                        length = value_length[1];
                        value_type = ETLV16Type_SourceDeviceStreamingPort;
                    }
                    break;

                case ESACPClientCmdSubType_SinkQuerySourcePrivilege:
                    break;

                case ESACPClientCmdSubType_SinkOwnSourceDevice:
                case ESACPClientCmdSubType_SinkInviteFriend:
                    mpProtocolHeaderParser->ParseTargetID(mpReadBuffer, id);
                    p_data = (TU8 *)&id;
                    length = 1;
                    value_type = ETLV16Type_AccountID;
                    break;

                case ESACPClientCmdSubType_SinkDonateSourceDevice:
                case ESACPClientCmdSubType_SinkRemoveFriend:
                case ESACPClientCmdSubType_SinkUpdateFriendPrivilege:
                    p_data = NULL;
                    length = 0;
                    break;

                case ESACPClientCmdSubType_SinkRetrieveFriendsList:
                case ESACPClientCmdSubType_SinkRetrieveAdminDeviceList:
                case ESACPClientCmdSubType_SinkRetrieveSharedDeviceList: {
                        length = mnIDListBufferLength;
                        ETLV16Type type;
                        gfParseTLV16Struct(payload, &type, &length, (void **)&mpIDListBuffer);
                        DASSERT(type == ETLV16Type_IDList);

                        if (length > mnIDListBufferLength) {
                            mnIDListBufferLength = length;
                        }

                        p_data = (TU8 *)mpIDListBuffer;
                        value_type = type;
                    }
                    break;

                case ESACPClientCmdSubType_SinkQueryFriendInfo:
                    //todo
                    p_data = payload;
                    length = payload_size;
                    break;

                default:
                    LOG_FATAL("Unknown sub type: %x\n", sub_type);
                    break;
            }
        } else {
            LOG_WARN("handleMessage, reply from im server is not OK, 0x%x\n", p_header->flags);
        }
    } else if ((reqres_bits == (TU32)DSACPTypeBit_Request) || (cat == ESACPTypeCategory_ClientRelayChannel)) {
        //relay msg between client
        switch (sub_type) {
            case ESACPClientCmdSubType_SinkInviteFriend:
                mpProtocolHeaderParser->ParseTargetID(mpReadBuffer, id);
                p_data = (TU8 *)&id;
                length = 1;
                break;

            default:
                LOG_FATAL("Unknown sub type: %x\n", sub_type);
                break;
        }
    }

    err = gfSACPConnectResultToErrorCode((ESACPConnectResult)p_header->flags);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("err %d, %s\n", err, gfGetErrorCodeString(err));
    }
    err = mpMessageCallback(mpMessageCallbackOwner, err, sub_type, reqres_bits, value_type, p_data, length);
    if (DUnlikely(EECode_OK != err)) {
        LOG_FATAL("mpMessageCallback fail!\n");
    }

    return err;
#endif
    return EECode_OK;
}

//-----------------------------------------------------------------------
//
//  CSACPIMClientAgent
//
//-----------------------------------------------------------------------

CSACPIMClientAgent *CSACPIMClientAgent::Create()
{
    CSACPIMClientAgent *result = new CSACPIMClientAgent();
    if ((result) && (EECode_OK != result->Construct())) {
        result->Destroy();
        result = NULL;
    }
    return result;
}

CSACPIMClientAgent::CSACPIMClientAgent()
    : mLocalPort(0)
    , mServerPort(0)
    , mbConnected(0)
    , mbAuthenticated(0)
    , mSocket(-1)
    , mSeqCount(0)
    , mDataType(ESACPDataChannelSubType_Invalid)
    , mnHeaderLength(0)
    , mEncryptionType1(EEncryptionType_None)
    , mEncryptionType2(EEncryptionType_None)
    , mpSenderBuffer(NULL)
    , mSenderBufferSize(0)
    , mpReadBuffer(NULL)
    , mReadBufferSize(0)
    , mpPayloadBuffer(NULL)
    , mPayloadBufferSize(0)
    , mpSACPHeader(NULL)
    , mLastErrorHint1(0)
    , mLastErrorHint2(0)
    , mLastErrorCode(EECode_OK)
    , mpMessageCallbackOwner(NULL)
    , mpMessageCallback(NULL)
    , mpProtocolHeaderParser(NULL)
    , mnExtensionLength(0)
    , mUniqueID(0)
    , mpIDListBuffer(NULL)
    , mnIDListBufferLength(0)
    , mLastConnectErrorCode(ESACPConnectResult_OK)
{
}

CSACPIMClientAgent::~CSACPIMClientAgent()
{
    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    if (mpSenderBuffer) {
        free(mpSenderBuffer);
        mpSenderBuffer = NULL;
    }

    if (mpReadBuffer) {
        free(mpReadBuffer);
        mpReadBuffer = NULL;
    }

    if (mpPayloadBuffer) {
        free(mpPayloadBuffer);
        mpPayloadBuffer = NULL;
    }

    if (mIDListBuffer.p_id_list) {
        free(mIDListBuffer.p_id_list);
        mIDListBuffer.p_id_list = NULL;
    }

    if (mpProtocolHeaderParser) {
        mpProtocolHeaderParser->Destroy();
        mpProtocolHeaderParser = NULL;
    }

    mSenderBufferSize = 0;
    mpSACPHeader = NULL;
}

EECode CSACPIMClientAgent::Construct()
{
    mSenderBufferSize = DSACP_MAX_PAYLOAD_LENGTH;
    mpSenderBuffer = (TU8 *) malloc(mSenderBufferSize + 128);

    if (DUnlikely(!mpSenderBuffer)) {
        LOG_FATAL("no memory, request %u\n", mSenderBufferSize);
        mSenderBufferSize = 0;
        return EECode_NoMemory;
    }

    mReadBufferSize = DSACP_MAX_PAYLOAD_LENGTH;
    mpReadBuffer = (TU8 *) malloc(mReadBufferSize + 128);

    if (DUnlikely(!mpReadBuffer)) {
        LOG_FATAL("no memory, request %u\n", mReadBufferSize);
        mReadBufferSize = 0;
        return EECode_NoMemory;
    }

    mpSACPHeader = (SSACPHeader *)mpSenderBuffer;

    mpProtocolHeaderParser = gfCreatePotocolHeaderParser(EProtocolType_SACP, EProtocolHeaderExtentionType_SACP_IM);
    if (DUnlikely(mpProtocolHeaderParser == NULL)) {
        LOG_FATAL("gfCreatePotocolHeaderParser fail, no mem!\n");
        return EECode_NoMemory;
    }
    mnHeaderLength = mpProtocolHeaderParser->GetFixedHeaderLength();
    mnExtensionLength = sizeof(TUniqueID);
    memset(mIdentityString, 0x0, sizeof(mIdentityString));

    mIDListBuffer.p_id_list = NULL;
    mIDListBuffer.id_count = 0;
    mIDListBuffer.max_id_count = 0;

    return EECode_OK;
}

EECode CSACPIMClientAgent::Register(TChar *name, TChar *password, const TChar *server_url, TU16 serverport, TUniqueID &id)
{
    if (DUnlikely(mbConnected)) {
        LOG_FATAL("Should register before logout\n");
        return EECode_BadState;
    }

    if (DUnlikely(!name || !password || !server_url)) {
        LOG_FATAL("NULL name, password or server_url\n");
        return EECode_BadParam;
    }

    if (DUnlikely(strlen(name) >= DMAX_ACCOUNT_NAME_LENGTH_EX)) {
        LOG_FATAL("name too long, max %d\n", DMAX_ACCOUNT_NAME_LENGTH_EX - 1);
        return EECode_BadParam;
    }

    if (DUnlikely(strlen(password) >= DIdentificationStringLength)) {
        LOG_FATAL("password too long, max %d\n", DIdentificationStringLength - 1);
        return EECode_BadParam;
    }

    mSocket = gfNet_ConnectTo(server_url, serverport, SOCK_STREAM, IPPROTO_TCP);
    if (DUnlikely(!DIsSocketHandlerValid(mSocket))) {
        LOG_FATAL("connect to server fail!\n");
        return EECode_NetConnectFail;
    }

    TU8 *p_send_buffer = mpSenderBuffer;
    TU8 *p_cur = p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU32 length = 0;
    TU16 cur_size = 0;

    length = strlen(name);
    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_AccountName >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_AccountName) & 0xff;
    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
    p_tlv->length_low = (length + 1) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);
    memcpy(p_cur, name, length);
    p_cur += length;
    *p_cur++ = 0x0;
    cur_size += length + 1;

    length = strlen(password);
    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_AccountPassword >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_AccountPassword) & 0xff;
    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
    p_tlv->length_low = (length + 1) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);
    memcpy(p_cur, password, length);
    p_cur += length;
    *p_cur++ = 0x0;
    cur_size += length + 1;

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ConnectionTag, ESACPClientCmdSubType_SinkRegister);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount ++, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    gfSocketSetTimeout(mSocket, 2, 0);
    TInt ret = gfNet_Send_timeout(mSocket, p_send_buffer, (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID) + cur_size), 0, 1);
    if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID) + cur_size))) {
        LOG_ERROR("send register fail %ld\n", (TULong)sizeof(SSACPHeader) + sizeof(TUniqueID) + cur_size);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_NetSendHeader_Fail;
    }

    ret = gfNet_Recv_timeout(mSocket, p_send_buffer, (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)), DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("recieve, return 0, peer closed\n");
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)))) {
        LOG_ERROR("recieve, return %d, expected %lu\n", ret, (TULong)(sizeof(SSACPHeader) + sizeof(TUniqueID)));
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_NetReceiveHeader_Fail;
    }

    SSACPHeader *p_header = (SSACPHeader *)p_send_buffer;
    EECode err = gfSACPConnectResultToErrorCode((ESACPConnectResult)p_header->flags);

    if (DUnlikely(EECode_OK != err)) {
        LOG_FATAL("register fail, %d %s\n", err, gfGetErrorCodeString(err));
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    p_cur = p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
    DBER64(id, p_cur);

    mUniqueID = id;

    return EECode_OK;
}

EECode CSACPIMClientAgent::AcquireOwnership(TChar *device_code, TUniqueID &device_id)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(!device_code)) {
        LOG_FATAL("device code is NULL\n");
        return EECode_BadParam;
    }

    TU16 length = strlen(device_code) + 1;
    TU32 payload_size = length + sizeof(STLV16Header);
    ETLV16Type type = ETLV16Type_String;
    TU8 *p_payload = mpSenderBuffer + mnHeaderLength;
    if (DUnlikely((payload_size + mnHeaderLength) >= mSenderBufferSize)) {
        LOG_FATAL("payload string is too long! payload size: %u\n", payload_size);
        return EECode_BadParam;
    }

    gfFillTLV16Struct(p_payload, &type, &length, (void **)&device_code);
    initSendBuffer(ESACPClientCmdSubType_SinkOwnSourceDevice, EProtocolHeaderExtentionType_SACP_IM, payload_size, ESACPTypeCategory_ClientCmdChannel);

    EECode err = EECode_OK;
    gfSocketSetTimeout(mSocket, 2, 0);

    err = sendData(mpSenderBuffer, (payload_size + mnHeaderLength));
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    TU32 reqres_bits = 0, cat = 0, sub_type = 0;
    payload_size = 0;
    err = readLoop(payload_size, reqres_bits, cat, sub_type);
    if (DUnlikely(EECode_OK != err)) {
        LOG_FATAL("readLoop fail, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    SSACPHeader *p_header = (SSACPHeader *)mpReadBuffer;
    mLastConnectErrorCode = p_header->flags;
    TU32 seq_count = ((TU32)p_header->seq_count_0 << 16) | ((TU32)p_header->seq_count_1 << 8) | (TU32)p_header->seq_count_2;
    DASSERT((seq_count + 1) == mSeqCount);
    if (DUnlikely(ESACPConnectResult_OK != mLastConnectErrorCode)) {
        LOG_FATAL("Get reply from server, expect seq count is %hu, but got seq count is %hu\n", mSeqCount - 1, seq_count);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Error;
    }

    //check
    //if (cat != ESACPTypeCategory_ClientCmdChannel)
    //if (reqres_bits != (TU32)DSACPTypeBit_Responce)

    if (DUnlikely(ESACPClientCmdSubType_SinkOwnSourceDevice != sub_type)) {
        LOG_FATAL("Get reply from server, but sub type is not SinkOwnSourceDevice(0x%x), is 0x%x\n", ESACPClientCmdSubType_SinkOwnSourceDevice, sub_type);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Error;
    }

    //parse reply
    mpProtocolHeaderParser->ParseTargetID(mpReadBuffer, device_id);

    return EECode_OK;
}

EECode CSACPIMClientAgent::DonateOwnership(TChar *donate_target, TUniqueID device_id)
{
    LOG_ERROR("not implement\n");

    return EECode_OK;
}

EECode CSACPIMClientAgent::GrantPrivilegeSingleDevice(TUniqueID device_id, TUniqueID usr_id, TU32 privilege)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    ETLV16Type type = ETLV16Type_AccountID;
    TU16 length = sizeof(TUniqueID);
    TU8 *p_payload = mpSenderBuffer + mnHeaderLength;
    TUniqueID *p_id = &device_id;
    TU32 payload_size = length + sizeof(STLV16Header);
    gfFillTLV16Struct(p_payload, &type, &length, (void **)&p_id);

    //todo
    //fill privilege
    initSendBuffer(ESACPClientCmdSubType_SinkUpdateFriendPrivilege, EProtocolHeaderExtentionType_SACP_IM, payload_size, ESACPTypeCategory_ClientCmdChannel);

    EECode err = EECode_OK;
    err = sendData(mpSenderBuffer, (payload_size + mnHeaderLength));
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    TU32 reqres_bits = 0, cat = 0, sub_type = 0;
    payload_size = 0;
    err = readLoop(payload_size, reqres_bits, cat, sub_type);
    if (DUnlikely(EECode_OK != err)) {
        LOG_FATAL("readLoop fail, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    SSACPHeader *p_header = (SSACPHeader *)mpReadBuffer;
    mLastConnectErrorCode = p_header->flags;
    TU32 seq_count = ((TU32)p_header->seq_count_0 << 16) | ((TU32)p_header->seq_count_1 << 8) | (TU32)p_header->seq_count_2;
    DASSERT((seq_count + 1) == mSeqCount);
    if (DUnlikely(ESACPConnectResult_OK != mLastConnectErrorCode)) {
        LOG_FATAL("Get reply from server, expect seq count is %hu, but got seq count is %hu\n", mSeqCount - 1, seq_count);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Error;
    }

    //check
    //if (cat != ESACPTypeCategory_ClientCmdChannel)
    //if (reqres_bits != (TU32)DSACPTypeBit_Responce)

    if (DUnlikely(ESACPClientCmdSubType_SinkUpdateFriendPrivilege != sub_type)) {
        LOG_FATAL("Get reply from server, but sub type is not SinkUpdateFriendPrivilege(0x%x), is 0x%x\n", ESACPClientCmdSubType_SinkUpdateFriendPrivilege, sub_type);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Error;
    }

    return EECode_OK;
}

EECode CSACPIMClientAgent::GrantPrivilege(TUniqueID *device_id, TU32 device_count, TUniqueID usr_id, TU32 privilege)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(!device_id)) {
        LOG_FATAL("device id is NULL!\n");
        return EECode_BadParam;
    }
    ETLV16Type type = ETLV16Type_AccountID;
    TU16 length = sizeof(TUniqueID);
    TU8 *p_payload = mpSenderBuffer + mnHeaderLength;
    TU32 payload_size = length + sizeof(STLV16Header);
    void *p_value = (void *)&usr_id;
    gfFillTLV16Struct(p_payload, &type, &length, (void **)&p_value);
    p_payload += payload_size;

    type = ETLV16Type_IDList;
    length = sizeof(TUniqueID) * device_count;
    payload_size += length + sizeof(STLV16Header);
    gfFillTLV16Struct(p_payload, &type, &length, (void **)&device_id);
    if (DUnlikely((payload_size + mnHeaderLength) > mSenderBufferSize)) {
        //todo
        LOG_FATAL("sender buffer(%u) is not enough for sending id list(%u)!\n", mSenderBufferSize, (payload_size + mnHeaderLength));
        return EECode_InternalParamsBug;
    }

    initSendBuffer(ESACPClientCmdSubType_SinkUpdateFriendPrivilege, EProtocolHeaderExtentionType_SACP_IM, payload_size, ESACPTypeCategory_ClientCmdChannel);

    EECode err = EECode_OK;
    err = sendData(mpSenderBuffer, (payload_size + mnHeaderLength));
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    TU32 reqres_bits = 0, cat = 0, sub_type = 0;
    payload_size = 0;
    err = readLoop(payload_size, reqres_bits, cat, sub_type);
    if (DUnlikely(EECode_OK != err)) {
        LOG_FATAL("readLoop fail, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    SSACPHeader *p_header = (SSACPHeader *)mpReadBuffer;
    mLastConnectErrorCode = p_header->flags;
    TU32 seq_count = ((TU32)p_header->seq_count_0 << 16) | ((TU32)p_header->seq_count_1 << 8) | (TU32)p_header->seq_count_2;
    DASSERT((seq_count + 1) == mSeqCount);
    if (DUnlikely(ESACPConnectResult_OK != mLastConnectErrorCode)) {
        LOG_FATAL("Get reply from server, expect seq count is %hu, but got seq count is %hu\n", mSeqCount - 1, seq_count);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Error;
    }

    //check
    //if (cat != ESACPTypeCategory_ClientCmdChannel)
    //if (reqres_bits != (TU32)DSACPTypeBit_Responce)

    if (DUnlikely(ESACPClientCmdSubType_SinkUpdateFriendPrivilege != sub_type)) {
        LOG_FATAL("Get reply from server, but sub type is not SinkUpdateFriendPrivilege(0x%x), is 0x%x\n", ESACPClientCmdSubType_SinkUpdateFriendPrivilege, sub_type);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Error;
    }

    return EECode_OK;
}

EECode CSACPIMClientAgent::AddFriend(TChar *friend_user_name, TUniqueID &friend_id)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(!friend_user_name)) {
        return EECode_BadParam;
    }

    TU16 length = strlen(friend_user_name) + 1;
    TU32 payload_size = length + sizeof(STLV16Header);
    ETLV16Type type = ETLV16Type_String;
    TU8 *p_payload = mpSenderBuffer + mnHeaderLength;
    if (DUnlikely((payload_size + mnHeaderLength) >= mSenderBufferSize)) {
        LOG_FATAL("payload string is too long! payload size: %u\n", payload_size);
        return EECode_BadParam;
    }

    gfFillTLV16Struct(p_payload, &type, &length, (void **)&friend_user_name);
    initSendBuffer(ESACPClientCmdSubType_SinkInviteFriend, EProtocolHeaderExtentionType_SACP_IM, payload_size, ESACPTypeCategory_ClientCmdChannel);

    EECode err = EECode_OK;
    err = sendData(mpSenderBuffer, (payload_size + mnHeaderLength));
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    TU32 reqres_bits = 0, cat = 0, sub_type = 0;
    payload_size = 0;
    err = readLoop(payload_size, reqres_bits, cat, sub_type);
    if (DUnlikely(EECode_OK != err)) {
        LOG_FATAL("readLoop fail, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    SSACPHeader *p_header = (SSACPHeader *)mpReadBuffer;
    mLastConnectErrorCode = p_header->flags;
    TU32 seq_count = ((TU32)p_header->seq_count_0 << 16) | ((TU32)p_header->seq_count_1 << 8) | (TU32)p_header->seq_count_2;
    DASSERT((seq_count + 1) == mSeqCount);
    if (DUnlikely(ESACPConnectResult_OK != mLastConnectErrorCode)) {
        LOG_FATAL("Get reply from server, expect seq count is %hu, but got seq count is %hu\n", mSeqCount - 1, seq_count);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Error;
    }

    //check
    //if (cat != ESACPTypeCategory_ClientCmdChannel)
    //if (reqres_bits != (TU32)DSACPTypeBit_Responce)

    if (DUnlikely(ESACPClientCmdSubType_SinkInviteFriend != sub_type)) {
        LOG_FATAL("Get reply from server, but sub type is not SinkInviteFriend(0x%x), is 0x%x\n", ESACPClientCmdSubType_SinkInviteFriend, sub_type);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Error;
    }

    //parse reply
    mpProtocolHeaderParser->ParseTargetID(mpReadBuffer, friend_id);

    return EECode_OK;
}

EECode CSACPIMClientAgent::AcceptFriend(TUniqueID friend_id)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(!friend_id)) {
        return EECode_BadParam;
    }

    TU16 length = sizeof(TUniqueID);
    TU32 payload_size = length + sizeof(STLV16Header);
    ETLV16Type type = ETLV16Type_AccountID;
    TU8 *p_payload = mpSenderBuffer + mnHeaderLength;
    if (DUnlikely((payload_size + mnHeaderLength) >= mSenderBufferSize)) {
        LOG_FATAL("payload string is too long! payload size: %u\n", payload_size);
        return EECode_BadParam;
    }
    TUniqueID *p_id = &friend_id;
    gfFillTLV16Struct(p_payload, &type, &length, (void **)&p_id);
    initSendBuffer(ESACPClientCmdSubType_SinkAcceptFriend, EProtocolHeaderExtentionType_SACP_IM, payload_size, ESACPTypeCategory_ClientCmdChannel);


    EECode err = EECode_OK;
    err = sendData(mpSenderBuffer, (payload_size + mnHeaderLength));
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    TU32 reqres_bits = 0, cat = 0, sub_type = 0;
    payload_size = 0;
    err = readLoop(payload_size, reqres_bits, cat, sub_type);
    if (DUnlikely(EECode_OK != err)) {
        LOG_FATAL("readLoop fail, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    SSACPHeader *p_header = (SSACPHeader *)mpReadBuffer;
    mLastConnectErrorCode = p_header->flags;
    TU32 seq_count = ((TU32)p_header->seq_count_0 << 16) | ((TU32)p_header->seq_count_1 << 8) | (TU32)p_header->seq_count_2;
    DASSERT((seq_count + 1) == mSeqCount);
    if (DUnlikely(ESACPConnectResult_OK != mLastConnectErrorCode)) {
        LOG_FATAL("Get reply from server, expect seq count is %hu, but got seq count is %hu\n", mSeqCount - 1, seq_count);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Error;
    }

    //check
    //if (cat != ESACPTypeCategory_ClientCmdChannel)
    //if (reqres_bits != (TU32)DSACPTypeBit_Responce)

    if (DUnlikely(ESACPClientCmdSubType_SinkAcceptFriend != sub_type)) {
        LOG_FATAL("Get reply from server, but sub type is not SinkAcceptFriend(0x%x), is 0x%x\n", ESACPClientCmdSubType_SinkAcceptFriend, sub_type);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Error;
    }

    return EECode_OK;
}

EECode CSACPIMClientAgent::RemoveFriend(TUniqueID friend_id)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(!friend_id)) {
        return EECode_BadParam;
    }

    TU16 length = sizeof(TUniqueID);
    TU32 payload_size = length + sizeof(STLV16Header);
    ETLV16Type type = ETLV16Type_AccountID;
    TU8 *p_payload = mpSenderBuffer + mnHeaderLength;
    if (DUnlikely((payload_size + mnHeaderLength) >= mSenderBufferSize)) {
        LOG_FATAL("payload string is too long! payload size: %u\n", payload_size);
        return EECode_BadParam;
    }
    TUniqueID *p_id = &friend_id;
    gfFillTLV16Struct(p_payload, &type, &length, (void **)&p_id);
    initSendBuffer(ESACPClientCmdSubType_SinkRemoveFriend, EProtocolHeaderExtentionType_SACP_IM, payload_size, ESACPTypeCategory_ClientCmdChannel);


    EECode err = EECode_OK;
    err = sendData(mpSenderBuffer, (payload_size + mnHeaderLength));
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    TU32 reqres_bits = 0, cat = 0, sub_type = 0;
    payload_size = 0;
    err = readLoop(payload_size, reqres_bits, cat, sub_type);
    if (DUnlikely(EECode_OK != err)) {
        LOG_FATAL("readLoop fail, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    SSACPHeader *p_header = (SSACPHeader *)mpReadBuffer;
    mLastConnectErrorCode = p_header->flags;
    TU32 seq_count = ((TU32)p_header->seq_count_0 << 16) | ((TU32)p_header->seq_count_1 << 8) | (TU32)p_header->seq_count_2;
    DASSERT((seq_count + 1) == mSeqCount);
    if (DUnlikely(ESACPConnectResult_OK != mLastConnectErrorCode)) {
        LOG_FATAL("Get reply from server, expect seq count is %hu, but got seq count is %hu\n", mSeqCount - 1, seq_count);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Error;
    }

    //check
    //if (cat != ESACPTypeCategory_ClientCmdChannel)
    //if (reqres_bits != (TU32)DSACPTypeBit_Responce)

    if (DUnlikely(ESACPClientCmdSubType_SinkRemoveFriend != sub_type)) {
        LOG_FATAL("Get reply from server, but sub type is not SinkRemoveFriend(0x%x), is 0x%x\n", ESACPClientCmdSubType_SinkRemoveFriend, sub_type);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Error;
    }

    return EECode_OK;
}

EECode CSACPIMClientAgent::RetrieveFriendList(SAgentIDListBuffer *&p_id_list)
{
    return retrieveIdList(ESACPClientCmdSubType_SinkRetrieveFriendsList, p_id_list);
}

EECode CSACPIMClientAgent::RetrieveAdminDeviceList(SAgentIDListBuffer *&p_id_list)
{
    return retrieveIdList(ESACPClientCmdSubType_SinkRetrieveAdminDeviceList, p_id_list);
}

EECode CSACPIMClientAgent::RetrieveSharedDeviceList(SAgentIDListBuffer *&p_id_list)
{
    return retrieveIdList(ESACPClientCmdSubType_SinkRetrieveSharedDeviceList, p_id_list);
}

void CSACPIMClientAgent::FreeIDList(SAgentIDListBuffer *id_list)
{
    //not need to do something
}

EECode CSACPIMClientAgent::QueryDeivceProfile(TUniqueID device_id, SDeviceProfile *p_profile)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    ETLV16Type type = ETLV16Type_AccountID;
    TU16 length = sizeof(TUniqueID);
    TU8 *p_payload = mpSenderBuffer + mnHeaderLength;
    TU32 payload_size = length + sizeof(STLV16Header);
    TUniqueID *p_id = &device_id;
    gfFillTLV16Struct(p_payload, &type, &length, (void **)&p_id);
    initSendBuffer(ESACPClientCmdSubType_SinkQuerySource, EProtocolHeaderExtentionType_SACP_IM, payload_size, ESACPTypeCategory_ClientCmdChannel);

    EECode err = EECode_OK;
    err = sendData(mpSenderBuffer, mnHeaderLength + payload_size);
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    TU32 reqres_bits = 0, cat = 0, sub_type = 0;
    payload_size = 0;
    err = readLoop(payload_size, reqres_bits, cat, sub_type);
    if (DUnlikely(EECode_OK != err)) {
        LOG_FATAL("readLoop fail, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    SSACPHeader *p_header = (SSACPHeader *)mpReadBuffer;
    mLastConnectErrorCode = p_header->flags;
    TU32 seq_count = ((TU32)p_header->seq_count_0 << 16) | ((TU32)p_header->seq_count_1 << 8) | (TU32)p_header->seq_count_2;
    DASSERT((seq_count + 1) == mSeqCount);
    if (DUnlikely(ESACPConnectResult_OK != mLastConnectErrorCode)) {
        LOG_FATAL("Get reply from server, expect seq count is %hu, but got seq count is %hu\n", mSeqCount - 1, seq_count);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Error;
    }

    //check
    //if (cat != ESACPTypeCategory_ClientCmdChannel)
    //if (reqres_bits != (TU32)DSACPTypeBit_Responce)

    if (DUnlikely(ESACPClientCmdSubType_SinkQuerySource != sub_type)) {
        LOG_FATAL("Get reply from server, but sub type is not SinkQuerySource(0x%x), is 0x%x\n", ESACPClientCmdSubType_SinkQuerySource, sub_type);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Error;
    }

    //parse payload
    //todo
    p_payload = mpReadBuffer + mnHeaderLength;
    {
        ETLV16Type type[8] = {ETLV16Type_Invalid};
        TU16 value_length[8] = {0};
        void *value[8] = {NULL};
        TSocketPort uploading_port = 0, streaming_port = 0;
        TU64 dynamic_input = 0;
        value[2] = (void *)&uploading_port;
        value[3] = (void *)&streaming_port;
        value[4] = (void *)&dynamic_input;

        value[6] = (void *) & (p_profile->status);
        value[7] = (void *) & (p_profile->storage_capacity);
        gfParseTLV16Struct(p_payload, type, value_length, value, 8);

        DASSERT(type[0] == ETLV16Type_SourceDeviceDataServerAddress);
        DASSERT(type[1] == ETLV16Type_SourceDeviceStreamingTag);
        DASSERT(type[2] == ETLV16Type_SourceDeviceUploadingPort);
        DASSERT(type[3] == ETLV16Type_SourceDeviceStreamingPort);
        DASSERT(type[4] == ETLV16Type_DynamicInput);
        DASSERT(type[5] == ETLV16Type_DeviceNickName);
        DASSERT(type[6] == ETLV16Type_DeviceStatus);
        DASSERT(type[7] == ETLV16Type_DeviceStorageCapacity);

        snprintf(p_profile->dataserver_address, DMaxServerUrlLength, "%s", (TChar *)value[0]);
        snprintf(p_profile->name, DMAX_ACCOUNT_NAME_LENGTH_EX, "%s", (TChar *)value[1]);
        p_profile->rtsp_streaming_port = streaming_port;
        snprintf(p_profile->nickname, DMAX_ACCOUNT_NAME_LENGTH_EX, "%s", (TChar *)value[5]);
    }
    //dynamic code

    return EECode_OK;
}

#if 0
EECode CSACPIMClientAgent::Login(TUniqueID id, TChar *password, TChar *server_url, TU16 server_port, TChar *&cloud_server_url, TU16 &cloud_server_port, TU64 &dynamic_code_input)
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ConnectionTag, ESACPClientCmdSubType_SourceAuthentication);
    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;

    //AUTO_LOCK(mpMutex);

    mServerPort = server_port;

    if (DUnlikely(!server_url)) {
        LOG_ERROR("NULL server_url\n");
        return EECode_BadParam;
    } else {
        LOG_NOTICE("server_url %s, server_port %hu\n", server_url, mServerPort);
    }

    if (DUnlikely(!authencation_string)) {
        LOG_ERROR("NULL authencation_string\n");
        return EECode_BadParam;
    }

    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    mSocket = gfNet_ConnectTo((const TChar *)server_url, mServerPort, SOCK_STREAM, IPPROTO_TCP);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("gfNet_ConnectTo(%s:%hu) fail\n", server_url, local_port);
        mLastErrorCode = EECode_NetConnectFail;
        mLastErrorHint1 = mSocket;
        mLastErrorHint2 = 0;
        return EECode_NetConnectFail;
    }

    TU16 payload_length = strlen(authencation_string) + sizeof(TUniqueID);
    TU16 ext_size = (TU16)sizeof(TUniqueID);
    TU8 ext_type = (TU8)EProtocolHeaderExtentionType_SACP_IM;

    fillHeader(type, payload_length, ext_size, ext_type);

    p_payload += ext_size;
    *p_payload++ = (id >> 56) & 0xff;
    *p_payload++ = (id >> 48) & 0xff;
    *p_payload++ = (id >> 40) & 0xff;
    *p_payload++ = (id >> 32) & 0xff;
    *p_payload++ = (id >> 24) & 0xff;
    *p_payload++ = (id >> 16) & 0xff;
    *p_payload++ = (id >> 8) & 0xff;
    *p_payload++ = id & 0xff;

    memcpy(p_payload, authencation_string, strlen(authencation_string));

    gfSocketSetTimeout(mSocket, 2, 0);

    /*
        LOG_NOTICE("before send, size %d, type %08x\n", (sizeof(SSACPHeader) + authentication_length), type);
        LOG_NOTICE("data %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x\n", \
            mpSenderBuffer[0], mpSenderBuffer[1], mpSenderBuffer[2], mpSenderBuffer[3], \
            mpSenderBuffer[4], mpSenderBuffer[5], mpSenderBuffer[6], mpSenderBuffer[7], \
            mpSenderBuffer[8], mpSenderBuffer[9], mpSenderBuffer[10], mpSenderBuffer[11], \
            mpSenderBuffer[12], mpSenderBuffer[13], mpSenderBuffer[14], mpSenderBuffer[15]);
    */

    ret = gfNet_Send_timeout(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + payload_length + ext_size), 0, 1);
    if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + payload_length + ext_size))) {
        mLastErrorCode = EECode_NetSendHeader_Fail;
        mLastErrorHint1 = ret;
        mLastErrorHint2 = sizeof(SSACPHeader) + payload_length + ext_size;
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
        //continue to receive payload, generate dynamic_code1 from dynamic_input1
        ret = gfNet_Recv_timeout(mSocket, mpSenderBuffer, DDynamicInputStringLength, DNETUTILS_RECEIVE_FLAG_READ_ALL, 20);
        if (DUnlikely(DDynamicInputStringLength != ret)) {
            LOG_WARN("Login, recv dynamic_input1 %d not expected, expect %d, peer closed?\n", ret, DDynamicInputStringLength);
            mLastErrorCode = EECode_NetReceivePayload_Fail;
            mLastErrorHint1 = ret;
            mLastErrorHint2 = DDynamicInputStringLength;
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetReceivePayload_Fail;
        }
        strncpy((TChar *)(mpSenderBuffer + DDynamicInputStringLength), mIdentityString, DIdentificationStringLength);
        TU32 dynamic_code = gfHashAlgorithmV6(mpSenderBuffer, DDynamicInputStringLength + DIdentificationStringLength);

        //send dynamic_code1 to server
        payload_length = sizeof(dynamic_code);
        fillHeader(type, payload_length, ext_size, ext_type);
        p_payload = mpSenderBuffer + sizeof(SSACPHeader);
        *p_payload++ = (dynamic_code >> 24) & 0xff;
        *p_payload++ = (dynamic_code >> 16) & 0xff;
        *p_payload++ = (dynamic_code >> 8) & 0xff;
        *p_payload++ = dynamic_code & 0xff;

        ret = gfNet_Send_timeout(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + payload_length + ext_size), 0, 1);
        if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + payload_length + ext_size))) {
            mLastErrorCode = EECode_NetSendHeader_Fail;
            mLastErrorHint1 = ret;
            mLastErrorHint2 = sizeof(SSACPHeader) + payload_length + ext_size;
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetSendHeader_Fail;
        }

        //receive header of server url:port:dynamic_code_input2 first
        ret = gfNet_Recv_timeout(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader)), DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 2;
            mLastErrorHint2 = sizeof(SSACPHeader);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader)) || (ESACPConnectResult_OK != mpSACPHeader->flags))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            mLastErrorCode = EECode_NetReceiveHeader_Fail;
            mLastErrorHint1 = ret;
            mLastErrorHint2 = sizeof(SSACPHeader);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetReceiveHeader_Fail;
        }
        //receive cloud server url:port:dynamic_code_input2 from im server
        payload_length = (mpSACPHeader->size_1 << 8) | mpSACPHeader->size_2;
        ret = gfNet_Recv_timeout(mSocket, mpSenderBuffer, (TInt)payload_length, DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 2;
            mLastErrorHint2 = sizeof(SSACPHeader);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)payload_length)) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %hu\n", ret, payload_length);
            mLastErrorCode = EECode_NetReceivePayload_Fail;
            mLastErrorHint1 = ret;
            mLastErrorHint2 = payload_length;
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetReceivePayload_Fail;
        }
        TChar *p_data = NULL, *p_data1 = NULL;
        p_data = (TChar *) strchr((TChar *)mpSenderBuffer, ':');
        p_data1 = (TChar *) strchr(p_data + 1, ':');
        if (!p_data || !p_data1 || (sizeof(TU16) != p_data1 - (p_data + 1)) || (sizeof(TU64) != (TChar *)mpSenderBuffer + payload_length - (p_data1 + 1))) {
            LOG_ERROR("gfNet_Recv(), params format error, cloud_server_url len=%d, cloud_server_port len=%d(should be 2), dynamic_code_input len=%d(should be 8)\n",
                      (TInt)(p_data - (TChar *)mpSenderBuffer), (TInt)(p_data1 - (p_data + 1)), (TInt)((TChar *)mpSenderBuffer + payload_length - (p_data1 + 1)));
            mLastErrorCode = EECode_InternalParamsBug;
            mLastErrorHint1 = 0;
            mLastErrorHint2 = payload_length;
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_InternalParamsBug;
        }
        memset(mCloudServerUrl, 0x0, sizeof(mCloudServerUrl));
        strncpy(mCloudServerUrl, (TChar *)mpSenderBuffer, p_data - (TChar *)mpSenderBuffer);
        cloud_server_url = mCloudServerUrl;
        cloud_server_port = (((TU16)(p_data[1])) << 8) | (TU16)(p_data[2]);
        dynamic_code_input = (((TU64)(p_data1[1])) << 56) | (((TU64)(p_data1[2])) << 48) | (((TU64)(p_data1[3])) << 40) | (((TU64)(p_data1[4])) << 32) | \
                             (((TU64)(p_data1[5])) << 24) | (((TU64)(p_data1[6])) << 16) | (((TU64)(p_data1[7])) << 8) | (TU64)(p_data1[8]);
        mbConnected = 1;
        LOG_INFO("client recv: url=%s, port=%hu, dyanmic_input2=%llx\n", cloud_server_url, cloud_server_port, dynamic_code_input);
        LOG_NOTICE("connect success\n");
        return EECode_OK;
    } else if (ESACPConnectResult_Reject_NoSuchChannel == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_NoSuchChannel, %s\n", (TChar *)authencation_string);
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_NoSuchChannel;
    } else if (ESACPConnectResult_Reject_ChannelIsInUse == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_ChannelIsInUse, %s\n", (TChar *)authencation_string);
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_ChannelIsBusy;
    } else if (ESACPConnectResult_Reject_BadRequestFormat == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_BadRequestFormat, %s\n", (TChar *)authencation_string);
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_BadRequestFormat;
    } else if (ESACPConnectResult_Reject_CorruptedProtocol == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_CorruptedProtocol, %s\n", (TChar *)authencation_string);
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_CorruptedProtocol;
    } else if (ESACPConnectResult_Reject_AuthenticationDataTooLong == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_AuthenticationDataTooLong, %s\n", (TChar *)authencation_string);
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_AuthenticationDataTooLong;
    } else if (ESACPConnectResult_Reject_NotProperPassword == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_NotProperPassword, %s\n", (TChar *)authencation_string);
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_NotProperPassword;
    } else if (ESACPConnectResult_Reject_NotSupported == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_NotSupported, %s\n", (TChar *)authencation_string);
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_NotSupported;
    } else if (ESACPConnectResult_Reject_AuthenticationFail == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_AuthenticationFail, %s\n", (TChar *)authencation_string);
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_AuthenticationFail;
    } else {
        LOG_ERROR("unknown error, %s, mpSACPHeader->flags %08x\n", (TChar *)authencation_string, mpSACPHeader->flags);
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_Unknown;
    }

    mbConnected = 1;

    return EECode_OK;
}

EECode CSACPIMClientAgent::LoginUserAccount(TChar *name, TChar *password, const TChar *server_url, TU16 serverport)
{
    //todo
    //authentication
    DASSERT(name);
    DASSERT(password);

    if (strlen(password) > DIdentificationStringLength) {
        LOG_FATAL("LoginUserAccount: password length too long, max string length: %u\n", DIdentificationStringLength);
        return EECode_BadParam;
    }
    EECode err = EECode_OK;

    //todo, tlv struct
    ETLV16Type type[2];
    type[0] = ETLV16Type_String;
    type[1] = ETLV16Type_String;

    TU16 length[2] = {0};
    length[0] = strlen(name) + 1;
    length[1] = strlen(password) + 1;

    void *value[2] = {NULL};
    value[0] = (void *)name;
    value[1] = (void *)password;

    TU32 payload_size = length[0] + length[1] + 2 * sizeof(STLV16Header);
    if (DUnlikely((payload_size + mnHeaderLength) >= mSenderBufferSize)) {
        LOG_FATAL("payload string is too long! payload size: %u\n", payload_size);
        return EECode_BadParam;
    }

    TU8 *p_payload = mpSenderBuffer + mnHeaderLength;
    gfFillTLV16Struct(p_payload, type, length, value, 2);
    initSendBuffer(ESACPClientCmdSubType_SinkAuthentication, EProtocolHeaderExtentionType_SACP_IM, payload_size);

    logout();

    mSocket = gfNet_ConnectTo(server_url, serverport, SOCK_STREAM, IPPROTO_TCP);
    if (DUnlikely(mSocket == -1)) {
        LOG_FATAL("connect to server fail!\n");
        return EECode_Error;
    }

    err = sendData(mpSenderBuffer, (payload_size + mnHeaderLength));
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    err = recvData(mpReadBuffer, mnHeaderLength);
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    //parse reply from im server
    SSACPHeader *p_header = (SSACPHeader *)mpReadBuffer;
    TU32 seq_count = ((TU32)p_header->seq_count_0 << 16) | ((TU32)p_header->seq_count_1 << 8) | (TU32)p_header->seq_count_2;
    DASSERT((seq_count + 1) == mSeqCount);
    mLastConnectErrorCode = p_header->flags;
    if (DUnlikely(mLastConnectErrorCode != ESACPConnectResult_OK)) {
        LOG_FATAL("Get reply from server, but result is not ok: 0x%x\n", p_header->flags);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Error;
    }

    mpProtocolHeaderParser->ParseTargetID(mpReadBuffer, mUniqueID);
    mbConnected = 1;

    return EECode_OK;
}
#endif

EECode CSACPIMClientAgent::Login(TUniqueID id, TChar *password, TChar *server_url, TU16 server_port, TChar *&cloud_server_url, TU16 &cloud_server_port, TU64 &dynamic_code_input)
{
    if (DUnlikely(mbConnected)) {
        LOG_FATAL("Already Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely((!password) || (!server_url))) {
        LOG_FATAL("NULL password or NULL server_url\n");
        return EECode_BadParam;
    }

    if (DUnlikely(strlen(password) >= DIdentificationStringLength)) {
        LOG_FATAL("password too long, greater than %d\n", DIdentificationStringLength - 1);
        return EECode_BadParam;
    }

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ConnectionTag, ESACPClientCmdSubType_SourceAuthentication);
    TU8 *p_cur = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU16 tlv_type = 0, tlv_size = 0;
    TInt length = 0;

    mServerPort = server_port;

    if (DUnlikely(!server_url)) {
        LOG_ERROR("NULL server_url\n");
        return EECode_BadParam;
    } else {
        LOG_NOTICE("server_url %s, server_port %hu\n", server_url, mServerPort);
    }

    if (DIsSocketHandlerValid(mSocket)) {
        LOG_WARN("close previous socket\n");
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    mSocket = gfNet_ConnectTo((const TChar *)server_url, mServerPort, SOCK_STREAM, IPPROTO_TCP);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("gfNet_ConnectTo(%s:%hu) fail\n", server_url, mServerPort);
        return EECode_NetConnectFail;
    }

    memset(p_cur, 0x0, sizeof(TUniqueID));
    p_cur += (TU16)sizeof(TUniqueID);

    length = 0;
    tlv_size = sizeof(TUniqueID);
    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_AccountID >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_AccountID) & 0xff;
    p_tlv->length_high = ((tlv_size) >> 8) & 0xff;
    p_tlv->length_low = (tlv_size) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    DBEW64(id, p_cur);
    length = sizeof(STLV16HeaderBigEndian) + tlv_size;

    gfSACPFillHeader((SSACPHeader *)mpSenderBuffer, type, sizeof(TUniqueID), (TU8)EProtocolHeaderExtentionType_SACP_IM, (TU16)sizeof(TUniqueID), mSeqCount ++, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
    length += sizeof(SSACPHeader) + sizeof(TUniqueID);

    gfSocketSetTimeout(mSocket, 2, 0);
    ret = gfNet_Send_timeout(mSocket, mpSenderBuffer, length, 0, 1);
    if (DUnlikely(ret != length)) {
        LOG_ERROR("send fail\n");
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_NetSendHeader_Fail;
    }

    ret = gfNet_Recv_timeout(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)), DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)))) {
        LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + sizeof(TUniqueID));
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_NetReceiveHeader_Fail;
    }

    SSACPHeader *p_sacp_header = (SSACPHeader *)mpSenderBuffer;
    EECode err = gfSACPConnectResultToErrorCode((ESACPConnectResult)p_sacp_header->flags);
    length = ((TU32)p_sacp_header->size_0 << 16) | ((TU32)p_sacp_header->size_1 << 8) | ((TU32)p_sacp_header->size_2);
    if (DLikely(EECode_OK == err)) {
        DASSERT((DDynamicInputStringLength + sizeof(STLV16HeaderBigEndian)) == length);
        p_cur = mpSenderBuffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
        ret = gfNet_Recv_timeout(mSocket, p_cur, length, DNETUTILS_RECEIVE_FLAG_READ_ALL, 20);
        if (DUnlikely((TInt)length != ret)) {
            LOG_WARN("Login, recv dynamic_input1 %d not expected, expect %d, peer closed?\n", ret, length);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetReceivePayload_Fail;
        }
        p_tlv = (STLV16HeaderBigEndian *) p_cur;
        tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
        tlv_size = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
        if ((ETLV16Type_DynamicInput != tlv_type) || (DDynamicInputStringLength != tlv_size)) {
            LOG_WARN("dynamic code input not as expected, %d, %d\n", tlv_type, tlv_size);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_ServerReject_CorruptedProtocol;
        }
        p_cur += sizeof(STLV16HeaderBigEndian);

        memcpy(mIdentityString, p_cur, DDynamicInputStringLength);
        memset(mIdentityString + DDynamicInputStringLength, 0x0, DIdentificationStringLength);
        strcpy((TChar *)mIdentityString + DDynamicInputStringLength, password);
        TU32 dynamic_code = gfHashAlgorithmV6((TU8 *)mIdentityString, DDynamicInputStringLength + DIdentificationStringLength);

        gfSACPFillHeader((SSACPHeader *)mpSenderBuffer, type, sizeof(dynamic_code) + sizeof(STLV16HeaderBigEndian), (TU8)EProtocolHeaderExtentionType_SACP_IM, (TU16)sizeof(TUniqueID), mSeqCount ++, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
        p_cur = mpSenderBuffer + sizeof(SSACPHeader);
        memset(p_cur, 0x0, sizeof(TUniqueID));
        p_cur += (TU16)sizeof(TUniqueID);
        p_tlv = (STLV16HeaderBigEndian *) p_cur;
        p_tlv->type_high = (ETLV16Type_AuthenticationResult32 >> 8) & 0xff;
        p_tlv->type_low = (ETLV16Type_AuthenticationResult32) & 0xff;
        p_tlv->length_high = ((tlv_size + 1) >> 8) & 0xff;
        p_tlv->type_low = (tlv_size + 1) & 0xff;
        p_cur += sizeof(STLV16HeaderBigEndian);
        DBEW32(dynamic_code, p_cur);

        length = sizeof(SSACPHeader) + (TU16)sizeof(TUniqueID) + sizeof(dynamic_code) + sizeof(STLV16HeaderBigEndian);
        ret = gfNet_Send_timeout(mSocket, mpSenderBuffer, length, 0, 1);
        if (DUnlikely(ret != length)) {
            LOG_WARN("Login, send dynamic code fail, ret %d\n", ret);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetSendHeader_Fail;
        }

        ret = gfNet_Recv_timeout(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)), DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetReceiveHeader_Fail;
        }

        err = gfSACPConnectResultToErrorCode((ESACPConnectResult)p_sacp_header->flags);
        if (DLikely(EECode_OK == err)) {
            p_cur = mpReadBuffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
            p_tlv = (STLV16HeaderBigEndian *) p_cur;
            tlv_type = (p_tlv->type_high << 8) | p_tlv->type_low;
            tlv_size = (p_tlv->length_high << 8) | p_tlv->length_low;
            p_cur += sizeof(STLV16HeaderBigEndian);

            if (DUnlikely((ETLV16Type_DynamicInput != tlv_type) || (DDynamicInputStringLength != tlv_size))) {
                LOG_ERROR("not dynamic input?\n");
                gfNetCloseSocket(mSocket);
                mSocket = DInvalidSocketHandler;
                return EECode_ServerReject_CorruptedProtocol;
            }
            DBER64(dynamic_code_input, p_cur);
            p_cur += tlv_size;

            p_tlv = (STLV16HeaderBigEndian *) p_cur;
            tlv_type = (p_tlv->type_high << 8) | p_tlv->type_low;
            tlv_size = (p_tlv->length_high << 8) | p_tlv->length_low;
            p_cur += sizeof(STLV16HeaderBigEndian);
            if (DUnlikely((ETLV16Type_DataNodeUrl != tlv_type) || (DMaxServerUrlLength < tlv_size))) {
                LOG_ERROR("not data server url?\n");
                gfNetCloseSocket(mSocket);
                mSocket = DInvalidSocketHandler;
                return EECode_ServerReject_CorruptedProtocol;
            }
            memset(mCloudServerUrl, 0x0, sizeof(mCloudServerUrl));
            memcpy(mCloudServerUrl, p_cur, tlv_size);
            p_cur += tlv_size;

            p_tlv = (STLV16HeaderBigEndian *) p_cur;
            tlv_type = (p_tlv->type_high << 8) | p_tlv->type_low;
            tlv_size = (p_tlv->length_high << 8) | p_tlv->length_low;
            p_cur += sizeof(STLV16HeaderBigEndian);
            if (DUnlikely((ETLV16Type_DataNodeCloudPort != tlv_type) || (2 != tlv_size))) {
                LOG_ERROR("not cloud port?\n");
                gfNetCloseSocket(mSocket);
                mSocket = DInvalidSocketHandler;
                return EECode_ServerReject_CorruptedProtocol;
            }
            memset(mCloudServerUrl, 0x0, sizeof(mCloudServerUrl));
            memcpy(mCloudServerUrl, p_cur, tlv_size);
            DBER16(cloud_server_port, p_cur);
            p_cur += tlv_size;

        } else {
            LOG_ERROR("authenticate fail? server return, %d, %s\n", err, gfGetErrorCodeString(err));
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetReceiveHeader_Fail;
        }

        LOG_NOTICE("connect success, client recv: url=%s, port=%hu, dyanmic_input2=%llx\n", cloud_server_url, cloud_server_port, dynamic_code_input);
    } else {
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        LOG_ERROR("server reject, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    mbConnected = 1;

    return EECode_OK;
}

EECode CSACPIMClientAgent::LoginUserAccount(TChar *name, TChar *password, const TChar *server_url, TU16 server_port)
{
    if (DUnlikely(mbConnected)) {
        LOG_FATAL("Already Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely((!name) || (!password) || (!server_url))) {
        LOG_FATAL("NULL name or NULL password or NULL server_url\n");
        return EECode_BadParam;
    }

    if (DUnlikely(strlen(name) >= DMAX_ACCOUNT_NAME_LENGTH_EX)) {
        LOG_FATAL("name too long, greater than %d\n", DMAX_ACCOUNT_NAME_LENGTH_EX - 1);
        return EECode_BadParam;
    }

    if (DUnlikely(strlen(password) >= DIdentificationStringLength)) {
        LOG_FATAL("password too long, greater than %d\n", DIdentificationStringLength - 1);
        return EECode_BadParam;
    }

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ConnectionTag, ESACPClientCmdSubType_SinkAuthentication);
    TU8 *p_cur = mpSenderBuffer;
    TInt ret = 0;
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU16 tlv_type = 0, tlv_size = 0;
    TInt length = 0;

    mServerPort = server_port;

    if (DUnlikely(!server_url)) {
        LOG_ERROR("NULL server_url\n");
        return EECode_BadParam;
    } else {
        LOG_NOTICE("server_url %s, server_port %hu\n", server_url, mServerPort);
    }

    if (DIsSocketHandlerValid(mSocket)) {
        LOG_WARN("close previous socket\n");
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    mSocket = gfNet_ConnectTo((const TChar *)server_url, mServerPort, SOCK_STREAM, IPPROTO_TCP);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("gfNet_ConnectTo(%s:%hu) fail\n", server_url, mServerPort);
        return EECode_NetConnectFail;
    }

    memset(p_cur, 0x0, sizeof(TUniqueID));
    p_cur += (TU16)sizeof(TUniqueID);

    length = 0;
    tlv_size = strlen(name);
    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = ((TU16)ETLV16Type_AccountName >> 8) & 0xff;
    p_tlv->type_low = ((TU16)ETLV16Type_AccountName) & 0xff;
    p_tlv->length_high = ((tlv_size + 1) >> 8) & 0xff;
    p_tlv->length_low = (tlv_size + 1) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    length += sizeof(STLV16HeaderBigEndian);
    memcpy(p_cur, name, tlv_size);
    p_cur += tlv_size;
    *p_cur ++ = 0x0;
    length += tlv_size + 1;
    gfSACPFillHeader((SSACPHeader *)mpSenderBuffer, type, length, (TU8)EProtocolHeaderExtentionType_SACP_IM, (TU16)sizeof(TUniqueID), mSeqCount ++, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);

    length += sizeof(SSACPHeader) + sizeof(TUniqueID);
    gfSocketSetTimeout(mSocket, 2, 0);
    ret = gfNet_Send_timeout(mSocket, mpSenderBuffer, length, 0, 1);
    if (DUnlikely(ret != length)) {
        LOG_ERROR("send fail\n");
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_NetSendHeader_Fail;
    }

    ret = gfNet_Recv_timeout(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)), DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)))) {
        LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + sizeof(TUniqueID));
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_NetReceiveHeader_Fail;
    }

    SSACPHeader *p_sacp_header = (SSACPHeader *)mpSenderBuffer;
    EECode err = gfSACPConnectResultToErrorCode((ESACPConnectResult)p_sacp_header->flags);
    length = ((TU32)p_sacp_header->size_0 << 16) | ((TU32)p_sacp_header->size_1 << 8) | ((TU32)p_sacp_header->size_2);
    if (DLikely(EECode_OK == err)) {
        DASSERT((DDynamicInputStringLength + sizeof(STLV16HeaderBigEndian)) == length);
        p_cur = mpSenderBuffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
        ret = gfNet_Recv_timeout(mSocket, p_cur, length, DNETUTILS_RECEIVE_FLAG_READ_ALL, 20);
        if (DUnlikely((TInt)length != ret)) {
            LOG_WARN("Login, recv dynamic_input1 %d not expected, expect %d, peer closed?\n", ret, length);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetReceivePayload_Fail;
        }
        p_tlv = (STLV16HeaderBigEndian *) p_cur;
        tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
        tlv_size = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
        if ((ETLV16Type_DynamicInput != tlv_type) || (DDynamicInputStringLength != tlv_size)) {
            LOG_WARN("dynamic code input not as expected, %d, %d\n", tlv_type, tlv_size);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_ServerReject_CorruptedProtocol;
        }
        p_cur += sizeof(STLV16HeaderBigEndian);

        memcpy(mIdentityString, p_cur, DDynamicInputStringLength);
        memset(mIdentityString + DDynamicInputStringLength, 0x0, DIdentificationStringLength);
        strcpy((TChar *)mIdentityString + DDynamicInputStringLength, password);
        TU32 dynamic_code = gfHashAlgorithmV6((TU8 *)mIdentityString, DDynamicInputStringLength + DIdentificationStringLength);

        gfSACPFillHeader((SSACPHeader *)mpSenderBuffer, type, sizeof(dynamic_code) + sizeof(STLV16HeaderBigEndian), (TU8)EProtocolHeaderExtentionType_SACP_IM, (TU16)sizeof(TUniqueID), mSeqCount ++, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
        p_cur = mpSenderBuffer + sizeof(SSACPHeader);
        memset(p_cur, 0x0, sizeof(TUniqueID));
        p_cur += (TU16)sizeof(TUniqueID);
        p_tlv = (STLV16HeaderBigEndian *) p_cur;
        p_tlv->type_high = (ETLV16Type_AuthenticationResult32 >> 8) & 0xff;
        p_tlv->type_low = (ETLV16Type_AuthenticationResult32) & 0xff;
        p_tlv->length_high = ((tlv_size + 1) >> 8) & 0xff;
        p_tlv->type_low = (tlv_size + 1) & 0xff;
        p_cur += sizeof(STLV16HeaderBigEndian);
        DBEW32(dynamic_code, p_cur);

        length = sizeof(SSACPHeader) + (TU16)sizeof(TUniqueID) + sizeof(dynamic_code) + sizeof(STLV16HeaderBigEndian);
        ret = gfNet_Send_timeout(mSocket, mpSenderBuffer, length, 0, 1);
        if (DUnlikely(ret != length)) {
            LOG_WARN("Login, send dynamic code fail, ret %d\n", ret);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetSendHeader_Fail;
        }

        ret = gfNet_Recv_timeout(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)), DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetReceiveHeader_Fail;
        }

        err = gfSACPConnectResultToErrorCode((ESACPConnectResult)p_sacp_header->flags);
        if (DLikely(EECode_OK != err)) {
            LOG_ERROR("authenticate fail? server return, %d, %s\n", err, gfGetErrorCodeString(err));
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetReceiveHeader_Fail;
        }

        LOG_NOTICE("connect success\n");
    } else {
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        LOG_ERROR("server reject, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    mbConnected = 1;

    return EECode_OK;
}

EECode CSACPIMClientAgent::LoginByUserID(TUniqueID id, TChar *password, const TChar *server_url, TU16 server_port)
{
    if (DUnlikely(mbConnected)) {
        LOG_FATAL("Already Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely((!password) || (!server_url))) {
        LOG_FATAL("NULL password or NULL server_url\n");
        return EECode_BadParam;
    }

    if (DUnlikely(strlen(password) >= DIdentificationStringLength)) {
        LOG_FATAL("password too long, greater than %d\n", DIdentificationStringLength - 1);
        return EECode_BadParam;
    }

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ConnectionTag, ESACPClientCmdSubType_SourceAuthentication);
    TU8 *p_cur = mpSenderBuffer + sizeof(SSACPHeader);
    TInt ret = 0;
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU16 tlv_type = 0, tlv_size = 0;
    TInt length = 0;

    mServerPort = server_port;

    if (DUnlikely(!server_url)) {
        LOG_ERROR("NULL server_url\n");
        return EECode_BadParam;
    } else {
        LOG_NOTICE("server_url %s, server_port %hu\n", server_url, mServerPort);
    }

    if (DIsSocketHandlerValid(mSocket)) {
        LOG_WARN("close previous socket\n");
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    mSocket = gfNet_ConnectTo((const TChar *)server_url, mServerPort, SOCK_STREAM, IPPROTO_TCP);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("gfNet_ConnectTo(%s:%hu) fail\n", server_url, mServerPort);
        return EECode_NetConnectFail;
    }

    memset(p_cur, 0x0, sizeof(TUniqueID));
    p_cur += (TU16)sizeof(TUniqueID);

    length = 0;
    tlv_size = sizeof(TUniqueID);
    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_AccountID >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_AccountID) & 0xff;
    p_tlv->length_high = ((tlv_size + 1) >> 8) & 0xff;
    p_tlv->length_low = (tlv_size + 1) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    length = sizeof(STLV16HeaderBigEndian) + sizeof(TUniqueID);
    DBEW64(id, p_cur);
    gfSACPFillHeader((SSACPHeader *)mpSenderBuffer, type, length, (TU8)EProtocolHeaderExtentionType_SACP_IM, (TU16)sizeof(TUniqueID), mSeqCount ++, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);

    length += sizeof(SSACPHeader) + sizeof(TUniqueID);
    gfSocketSetTimeout(mSocket, 2, 0);
    ret = gfNet_Send_timeout(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID) + length), 0, 1);
    if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID) + length))) {
        LOG_ERROR("send fail\n");
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_NetSendHeader_Fail;
    }

    ret = gfNet_Recv_timeout(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)), DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)))) {
        LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + sizeof(TUniqueID));
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_NetReceiveHeader_Fail;
    }

    SSACPHeader *p_sacp_header = (SSACPHeader *)mpSenderBuffer;
    EECode err = gfSACPConnectResultToErrorCode((ESACPConnectResult)p_sacp_header->flags);
    length = ((TU32)p_sacp_header->size_0 << 16) | ((TU32)p_sacp_header->size_1 << 8) | ((TU32)p_sacp_header->size_2);
    if (DLikely(EECode_OK == err)) {
        DASSERT((DDynamicInputStringLength + sizeof(STLV16HeaderBigEndian)) == length);
        p_cur = mpSenderBuffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
        ret = gfNet_Recv_timeout(mSocket, p_cur, length, DNETUTILS_RECEIVE_FLAG_READ_ALL, 20);
        if (DUnlikely((TInt)length != ret)) {
            LOG_WARN("Login, recv dynamic_input1 %d not expected, expect %d, peer closed?\n", ret, length);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetReceivePayload_Fail;
        }
        p_tlv = (STLV16HeaderBigEndian *) p_cur;
        tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
        tlv_size = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
        if ((ETLV16Type_DynamicInput != tlv_type) || (DDynamicInputStringLength != tlv_size)) {
            LOG_WARN("dynamic code input not as expected, %d, %d\n", tlv_type, tlv_size);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_ServerReject_CorruptedProtocol;
        }
        p_cur += sizeof(STLV16HeaderBigEndian);

        memcpy(mIdentityString, p_cur, DDynamicInputStringLength);
        memset(mIdentityString + DDynamicInputStringLength, 0x0, DIdentificationStringLength);
        strcpy((TChar *)mIdentityString + DDynamicInputStringLength, password);
        TU32 dynamic_code = gfHashAlgorithmV6(mIdentityString, DDynamicInputStringLength + DIdentificationStringLength);

        gfSACPFillHeader((SSACPHeader *)mpSenderBuffer, type, sizeof(dynamic_code) + sizeof(STLV16HeaderBigEndian), (TU8)EProtocolHeaderExtentionType_SACP_IM, (TU16)sizeof(TUniqueID), mSeqCount ++, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
        p_cur = mpSenderBuffer + sizeof(SSACPHeader);
        memset(p_cur, 0x0, sizeof(TUniqueID));
        p_cur += (TU16)sizeof(TUniqueID);
        p_tlv = (STLV16HeaderBigEndian *) p_cur;
        p_tlv->type_high = (ETLV16Type_AuthenticationResult32 >> 8) & 0xff;
        p_tlv->type_low = (ETLV16Type_AuthenticationResult32) & 0xff;
        p_tlv->length_high = ((tlv_size + 1) >> 8) & 0xff;
        p_tlv->type_low = (tlv_size + 1) & 0xff;
        p_cur += sizeof(STLV16HeaderBigEndian);
        DBEW32(dynamic_code, p_cur);

        length = sizeof(SSACPHeader) + (TU16)sizeof(TUniqueID) + sizeof(dynamic_code) + sizeof(STLV16HeaderBigEndian);
        ret = gfNet_Send_timeout(mSocket, mpSenderBuffer, length, 0, 1);
        if (DUnlikely(ret != length)) {
            LOG_WARN("Login, send dynamic code fail, ret %d\n", ret);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetSendHeader_Fail;
        }

        ret = gfNet_Recv_timeout(mSocket, mpSenderBuffer, (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)), DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetReceiveHeader_Fail;
        }

        err = gfSACPConnectResultToErrorCode((ESACPConnectResult)p_sacp_header->flags);
        if (DLikely(EECode_OK != err)) {
            LOG_ERROR("authenticate fail? server return, %d, %s\n", err, gfGetErrorCodeString(err));
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetReceiveHeader_Fail;
        }

        LOG_NOTICE("connect success\n");
    } else {
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        LOG_ERROR("server reject, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }
    mbConnected = 1;

    return EECode_OK;
}

EECode CSACPIMClientAgent::Logout()
{
    return logout();
}


EECode CSACPIMClientAgent::PostMessage(TUniqueID id, TU8 *message, TU32 message_length)
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPDebugCmdSubType_Customized);

    TInt ret = 0;
    TU32 transfer_size = 0;

    if (DUnlikely(!message || !message_length)) {
        LOG_ERROR("BAD params %p, size %d\n", message, message_length);
        mLastErrorCode = EECode_BadParam;
        mLastErrorHint1 = 100;
        mLastErrorHint2 = (TU32)message_length;
        return EECode_BadParam;
    }

    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need login first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = EECodeHint_BadSocket;
        mLastErrorHint2 = (TU32)mSocket;
        return EECode_BadState;
    }

    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);

    mpSACPHeader->type_1 = (type >> 24) & 0xff;
    mpSACPHeader->type_2 = (type >> 16) & 0xff;
    mpSACPHeader->type_3 = (type >> 8) & 0xff;
    mpSACPHeader->type_4 = type & 0xff;

    mpSACPHeader->encryption_type_1 = mEncryptionType1;
    mpSACPHeader->encryption_type_2 = mEncryptionType2;

    //mpSACPHeader->flags = 0;
    mpSACPHeader->header_ext_type = EProtocolHeaderExtentionType_SACP_IM;
    mpSACPHeader->header_ext_size_1 = 0;
    mpSACPHeader->header_ext_size_2 = 0;

    mpSACPHeader->flags = DSACPHeaderFlagBit_PacketStartIndicator;

    *p_payload++ = (id >> 56) & 0xff;
    *p_payload++ = (id >> 48) & 0xff;
    *p_payload++ = (id >> 40) & 0xff;
    *p_payload++ = (id >> 32) & 0xff;
    *p_payload++ = (id >> 24) & 0xff;
    *p_payload++ = (id >> 16) & 0xff;
    *p_payload++ = (id >> 8) & 0xff;
    *p_payload++ = id & 0xff;

    do {

        if ((mSenderBufferSize - mnHeaderLength) >= message_length) {
            transfer_size = message_length;
            message_length = 0;
            mpSACPHeader->flags |= DSACPHeaderFlagBit_PacketEndIndicator;
        } else {
            transfer_size = mSenderBufferSize - mnHeaderLength;
            message_length -= transfer_size;
        }

        mpSACPHeader->size_1 = (transfer_size >> 8) & 0xff;
        mpSACPHeader->size_2 = transfer_size & 0xff;
        mpSACPHeader->size_0 = (transfer_size >> 16) & 0xff;

        mpSACPHeader->seq_count_0 = (mSeqCount >> 16) & 0xff;
        mpSACPHeader->seq_count_1 = (mSeqCount >> 8) & 0xff;
        mpSACPHeader->seq_count_2 = mSeqCount & 0xff;
        mSeqCount ++;

        ret = gfNet_Send_timeout(mSocket, mpSenderBuffer, (TInt)mnHeaderLength, 0, 2);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 5;
            mLastErrorHint2 = (TU32)mSocket;

            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;

            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)mnHeaderLength)) {
            LOG_ERROR("ret not expected %d, header %d\n", ret, mnHeaderLength);
            mLastErrorCode = EECode_NetSendHeader_Fail;
            mLastErrorHint1 = ret;
            mLastErrorHint2 = (TU32)mSocket;

            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;

            return EECode_NetSendHeader_Fail;
        }

        ret = gfNet_Send_timeout(mSocket, message, (TInt)(transfer_size), 0, 2);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 6;
            mLastErrorHint2 = (TU32)mSocket;

            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;

            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(transfer_size))) {
            LOG_ERROR("ret not expected %d, transfer_size %u\n", ret, transfer_size);
            mLastErrorCode = EECode_NetSendPayload_Fail;
            mLastErrorHint1 = ret;
            mLastErrorHint2 = (TU32)mSocket;

            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;

            return EECode_NetSendPayload_Fail;
        }
        message += transfer_size;

        //LOG_NOTICE("transfer_size %ld, remainning %ld, flag %02x\n", transfer_size, size, mpSACPHeader->flags);

        mpSACPHeader->flags &= ~(DSACPHeaderFlagBit_PacketStartIndicator);

    } while (message_length);

    return EECode_OK;
}

EECode CSACPIMClientAgent::SendMessage(TUniqueID id, TU8 *message, TU32 message_length)
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request | DSACPTypeBit_NeedReply, ESACPTypeCategory_ClientCmdChannel, ESACPDebugCmdSubType_Customized);

    TInt ret = 0;
    TU32 transfer_size = 0;

    if (DUnlikely(!message || !message_length)) {
        LOG_ERROR("BAD params %p, size %d\n", message, message_length);
        mLastErrorCode = EECode_BadParam;
        mLastErrorHint1 = 100;
        mLastErrorHint2 = (TU32)message_length;
        return EECode_BadParam;
    }

    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("BAD socket %d, need login first\n", mSocket);
        mLastErrorCode = EECode_BadState;
        mLastErrorHint1 = EECodeHint_BadSocket;
        mLastErrorHint2 = (TU32)mSocket;
        return EECode_BadState;
    }

    TU8 *p_payload = mpSenderBuffer + sizeof(SSACPHeader);

    mpSACPHeader->type_1 = (type >> 24) & 0xff;
    mpSACPHeader->type_2 = (type >> 16) & 0xff;
    mpSACPHeader->type_3 = (type >> 8) & 0xff;
    mpSACPHeader->type_4 = type & 0xff;

    mpSACPHeader->encryption_type_1 = mEncryptionType1;
    mpSACPHeader->encryption_type_2 = mEncryptionType2;

    //mpSACPHeader->flags = 0;
    mpSACPHeader->header_ext_type = EProtocolHeaderExtentionType_SACP_IM;
    mpSACPHeader->header_ext_size_1 = 0;
    mpSACPHeader->header_ext_size_2 = 0;

    mpSACPHeader->flags = DSACPHeaderFlagBit_PacketStartIndicator;

    *p_payload++ = (id >> 56) & 0xff;
    *p_payload++ = (id >> 48) & 0xff;
    *p_payload++ = (id >> 40) & 0xff;
    *p_payload++ = (id >> 32) & 0xff;
    *p_payload++ = (id >> 24) & 0xff;
    *p_payload++ = (id >> 16) & 0xff;
    *p_payload++ = (id >> 8) & 0xff;
    *p_payload++ = id & 0xff;

    do {

        if ((mSenderBufferSize - mnHeaderLength) >= message_length) {
            transfer_size = message_length;
            message_length = 0;
            mpSACPHeader->flags |= DSACPHeaderFlagBit_PacketEndIndicator;
        } else {
            transfer_size = mSenderBufferSize - mnHeaderLength;
            message_length -= transfer_size;
        }

        mpSACPHeader->size_0 = (transfer_size >> 16) & 0xff;
        mpSACPHeader->size_1 = (transfer_size >> 8) & 0xff;
        mpSACPHeader->size_2 = transfer_size & 0xff;

        mpSACPHeader->seq_count_0 = (mSeqCount >> 16) & 0xff;
        mpSACPHeader->seq_count_1 = (mSeqCount >> 8) & 0xff;
        mpSACPHeader->seq_count_2 = mSeqCount & 0xff;
        mSeqCount ++;

        ret = gfNet_Send_timeout(mSocket, mpSenderBuffer, (TInt)mnHeaderLength, 0, 2);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 5;
            mLastErrorHint2 = (TU32)mSocket;

            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;

            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)mnHeaderLength)) {
            LOG_ERROR("ret not expected %d, header %d\n", ret, mnHeaderLength);
            mLastErrorCode = EECode_NetSendHeader_Fail;
            mLastErrorHint1 = ret;
            mLastErrorHint2 = (TU32)mSocket;

            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;

            return EECode_NetSendHeader_Fail;
        }

        ret = gfNet_Send_timeout(mSocket, message, (TInt)(transfer_size), 0, 2);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Send(), return 0, peer closed\n");
            mLastErrorCode = EECode_Closed;
            mLastErrorHint1 = 6;
            mLastErrorHint2 = (TU32)mSocket;

            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;

            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(transfer_size))) {
            LOG_ERROR("ret not expected %d, transfer_size %u\n", ret, transfer_size);
            mLastErrorCode = EECode_NetSendPayload_Fail;
            mLastErrorHint1 = ret;
            mLastErrorHint2 = (TU32)mSocket;

            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;

            return EECode_NetSendPayload_Fail;
        }
        message += transfer_size;

        //LOG_NOTICE("transfer_size %ld, remainning %ld, flag %02x\n", transfer_size, size, mpSACPHeader->flags);

        mpSACPHeader->flags &= ~(DSACPHeaderFlagBit_PacketStartIndicator);

    } while (message_length);

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
        LOG_NOTICE("connect success\n");
        return EECode_OK;
    } else if (ESACPConnectResult_Reject_NoSuchChannel == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_NoSuchChannel\n");
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_NoSuchChannel;
    } else if (ESACPConnectResult_Reject_ChannelIsInUse == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_ChannelIsInUse\n");
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_ChannelIsBusy;
    } else if (ESACPConnectResult_Reject_BadRequestFormat == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_BadRequestFormat\n");
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_BadRequestFormat;
    } else if (ESACPConnectResult_Reject_CorruptedProtocol == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_CorruptedProtocol\n");
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_CorruptedProtocol;
    } else if (ESACPConnectResult_Reject_AuthenticationDataTooLong == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_AuthenticationDataTooLong\n");
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_AuthenticationDataTooLong;
    } else if (ESACPConnectResult_Reject_NotProperPassword == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_NotProperPassword\n");
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_NotProperPassword;
    } else if (ESACPConnectResult_Reject_NotSupported == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_NotSupported\n");
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_NotSupported;
    } else if (ESACPConnectResult_Reject_AuthenticationFail == mpSACPHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_AuthenticationFail\n");
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_AuthenticationFail;
    } else {
        LOG_ERROR("unknown error, mpSACPHeader->flags %08x\n", mpSACPHeader->flags);
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_Unknown;
    }

    return EECode_OK;
}

void CSACPIMClientAgent::GetLastConnectErrorCode(ESACPConnectResult &connect_error_code)
{
    connect_error_code = (ESACPConnectResult)mLastConnectErrorCode;
    return;
}

void CSACPIMClientAgent::Destroy()
{
    delete this;
}
////////////////////////////////////////////////////////////////////////////////
EECode CSACPIMClientAgent::logout()
{
    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    mbConnected = 0;

    return EECode_OK;
}

void CSACPIMClientAgent::fillHeader(TU32 type, TU32 size, TU16 ext_size, TU8 ext_type)
{
    mpSACPHeader->type_1 = (type >> 24) & 0xff;
    mpSACPHeader->type_2 = (type >> 16) & 0xff;
    mpSACPHeader->type_3 = (type >> 8) & 0xff;
    mpSACPHeader->type_4 = type & 0xff;

    //LOG_NOTICE("0x%08x \n", type);
    //LOG_NOTICE("%02x %02x %02x %02x\n", (type >> 24) & 0xff, (type >> 16) & 0xff, (type >> 8) & 0xff, type & 0xff);

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
    mpSACPHeader->header_ext_type = ext_type;
    mpSACPHeader->header_ext_size_1 = (ext_size >> 8) & 0xff;
    mpSACPHeader->header_ext_size_2 = ext_size & 0xff;
    return;
}

EECode CSACPIMClientAgent::sendData(TU8 *data, TU16 size, TU8 maxcount)
{
    TInt sentlength = 0;

    LOG_NOTICE("CSACPIMClientAgentAsync: sendData, size %hu\n", size);

    sentlength = gfNet_Send_timeout(mSocket, data, (TInt)size, 0, maxcount);
    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, socket is closed!\n");
        return EECode_Closed;
    } else if (DUnlikely(sentlength < 0)) {
        LOG_FATAL("send data time out!max count: %d\n", maxcount);
        return EECode_TimeOut;
    }

    return EECode_OK;
}

EECode CSACPIMClientAgent::recvData(TU8 *data, TU16 size, TU8 maxcount)
{
    TInt recvlength = 0;

    recvlength = gfNet_Recv_timeout(mSocket, data, (TInt)size, DNETUTILS_RECEIVE_FLAG_READ_ALL, maxcount);
    if (DUnlikely(recvlength == 0)) {
        LOG_FATAL("recv data fail, socket is closed.\n");
        return EECode_Closed;
    } else if (DUnlikely(recvlength < (TInt)size)) {
        LOG_FATAL("recv data time out! max count: %d\n", maxcount);
        return EECode_TimeOut;
    }

    return EECode_OK;
}

void CSACPIMClientAgent::initSendBuffer(ESACPClientCmdSubType sub_type, EProtocolHeaderExtentionType ext_type, TU32 payload_size, ESACPTypeClientCategory cat_type)
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request, cat_type, sub_type);
    fillHeader(type, payload_size, mnExtensionLength, ext_type);
    if (ext_type == EProtocolHeaderExtentionType_SACP_IM) {
        mpProtocolHeaderParser->FillSourceID(mpSenderBuffer, 0);
    } else {
        mpProtocolHeaderParser->FillSourceID(mpSenderBuffer, mUniqueID);
    }
    return;
}

EECode CSACPIMClientAgent::readLoop(TU32 &payload_size, TU32 &reqres_bits, TU32 &cat, TU32 &sub_type)
{
    EECode err = EECode_OK;

    err = recvData(mpReadBuffer, mnHeaderLength);
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    mpProtocolHeaderParser->Parse(mpReadBuffer, payload_size, reqres_bits, cat, sub_type);
    if (payload_size == 0) {
        return EECode_OK;
    }

    TU8 *p_payload = NULL;
    if (DUnlikely((payload_size + mnHeaderLength) >= mReadBufferSize)) {
        if (mPayloadBufferSize <= payload_size) {
            if (mpPayloadBuffer) { free(mpPayloadBuffer); }
            mpPayloadBuffer = (TU8 *)DDBG_MALLOC(payload_size + 1, "CAPB");
            if (DUnlikely(!mpPayloadBuffer)) {
                LOG_FATAL("malloc payload buffer fail!\n");
                return EECode_NoMemory;
            }
            mPayloadBufferSize = payload_size + 1;
        }
        p_payload = mpPayloadBuffer;
    } else if (payload_size > 0) {
        p_payload = mpReadBuffer + mnHeaderLength;
    }

    if (DLikely(p_payload)) {
        err = recvData(p_payload, payload_size);
        if (DUnlikely(EECode_Closed == err)) {
            LOG_FATAL("socket is closed!\n");
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return err;
        } else if (DUnlikely(EECode_TimeOut == err)) {
            LOG_FATAL("recv data time out!\n");
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return err;
        }
    }

    return EECode_OK;
}


EECode CSACPIMClientAgent::retrieveIdList(ESACPClientCmdSubType sub_type, SAgentIDListBuffer *&p_id_list)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    initSendBuffer(sub_type, EProtocolHeaderExtentionType_SACP_IM, 0, ESACPTypeCategory_ClientCmdChannel);

    //todo, tlv struct
    EECode err = EECode_OK;
    err = sendData(mpSenderBuffer, mnHeaderLength);
    if (DUnlikely(EECode_OK != err)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    TU32 reqres_bits = 0, cat = 0, sub_type_out = 0, payload_size = 0;
    err = readLoop(payload_size, reqres_bits, cat, sub_type_out);
    if (DUnlikely(EECode_OK != err)) {
        LOG_FATAL("readLoop fail, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    SSACPHeader *p_header = (SSACPHeader *)mpReadBuffer;
    mLastConnectErrorCode = p_header->flags;
    TU32 seq_count = ((TU32)p_header->seq_count_0 << 16) | ((TU32)p_header->seq_count_1 << 8) | (TU32)p_header->seq_count_2;
    DASSERT((seq_count + 1) == mSeqCount);
    if (DUnlikely(ESACPConnectResult_OK != mLastConnectErrorCode)) {
        LOG_FATAL("Get reply from server, expect seq count is %hu, but got seq count is %hu\n", mSeqCount - 1, seq_count);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Error;
    }

    //check
    //if (cat != ESACPTypeCategory_ClientCmdChannel)
    //if (reqres_bits != (TU32)DSACPTypeBit_Responce)

    if (DUnlikely(sub_type_out != (TU32)sub_type)) {
        LOG_FATAL("Get reply from server, but sub type is not (0x%x), is 0x%x\n", sub_type, sub_type_out);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Error;
    }

    TU16 length = mnIDListBufferLength;
    ETLV16Type type = ETLV16Type_Invalid;
    TU8 *p_payload = mpReadBuffer + mnHeaderLength;
    //mpIDListBuffer is malloced by gfParseTLV16Struct, free by agent
    gfParseTLV16Struct(p_payload, &type, &length, (void **) & (mIDListBuffer.p_id_list));
    DASSERT(type == ETLV16Type_IDList);
    if (length > mnIDListBufferLength) {
        mnIDListBufferLength = length;
    }
    mIDListBuffer.id_count = length / sizeof(TUniqueID);
    p_id_list = &mIDListBuffer;

    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif

