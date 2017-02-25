/*******************************************************************************
 * im_sacp_client_agent_v2.cpp
 *
 * History:
 *  2014/08/08 - [Zhi He] create file
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

#include "sacp_types.h"
#include "im_sacp_client_agent_v2.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IIMClientAgent *gfCreateSACPIMClientAgentV2()
{
    CSACPIMClientAgentV2 *thiz = CSACPIMClientAgentV2::Create();
    return thiz;
}

//-----------------------------------------------------------------------
//
//  CSACPIMClientAgentV2
//
//-----------------------------------------------------------------------
CSACPIMClientAgentV2 *CSACPIMClientAgentV2::Create()
{
    CSACPIMClientAgentV2 *result = new CSACPIMClientAgentV2();
    if ((result) && (EECode_OK != result->Construct())) {
        result->Destroy();
        result = NULL;
    }
    return result;
}

EECode CSACPIMClientAgentV2::Construct()
{
    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOG_FATAL("no memory, gfCreateMutex fail\n");
        return EECode_NoMemory;
    }

    mpResourceMutex = gfCreateMutex();
    if (DUnlikely(!mpResourceMutex)) {
        LOG_FATAL("no memory, gfCreateMutex fail\n");
        return EECode_NoMemory;
    }

    EECode err = gfOSCreatePipe(mPipeFd);
    if (DUnlikely(err != EECode_OK)) {
        LOG_FATAL("pipe fail.\n");
        return EECode_Error;
    }

    mReadBufferSize = DSACP_MAX_PAYLOAD_LENGTH;
    mpReadBuffer = (TU8 *) DDBG_MALLOC(mReadBufferSize + 128, "ICAR");

    if (DUnlikely(!mpReadBuffer)) {
        LOG_FATAL("no memory, request %u\n", mReadBufferSize);
        mReadBufferSize = 0;
        return EECode_NoMemory;
    }

    mpSACPHeader = (SSACPHeader *)mpReadBuffer;
    memset(mIdentityString, 0x0, sizeof(mIdentityString));

    mpMsgSlotList = new CIDoubleLinkedList();
    if (DUnlikely(!mpMsgSlotList)) {
        LOG_FATAL("new CIDoubleLinkedList() fail, no memory!\n");
        return EECode_NoMemory;
    }

    mpFreeMsgSlotList = new CIDoubleLinkedList();
    if (DUnlikely(!mpFreeMsgSlotList)) {
        LOG_FATAL("new CIDoubleLinkedList() fail, no memory!\n");
        return EECode_NoMemory;
    }

    mpSendBufferList = new CIDoubleLinkedList();
    if (DUnlikely(!mpSendBufferList)) {
        LOG_FATAL("new CIDoubleLinkedList() fail, no memory!\n");
        return EECode_NoMemory;
    }

    mpIDListBufferList = new CIDoubleLinkedList();
    if (DUnlikely(!mpIDListBufferList)) {
        LOG_FATAL("new CIDoubleLinkedList() fail, no memory!\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CSACPIMClientAgentV2::CSACPIMClientAgentV2()
    : mpMutex(NULL)
    , mpResourceMutex(NULL)
    , mnHeaderLength(sizeof(SSACPHeader) + sizeof(TUniqueID))
    , mServerPort(DDefaultSACPIMServerPort)
    , mbConnected(0)
    , mbStartReadLoop(0)
    , mSeqCount(0)
    , mSocket(-1)
    , mpReadBuffer(NULL)
    , mReadBufferSize(0)
    , mpSACPHeader(NULL)
    , mpCallbackOwner(NULL)
    , mpMessageCallback(NULL)
    , mpSystemNotifyCallback(NULL)
    , mpErrorCallback(NULL)
    , mpThread(NULL)
    , mpMsgSlotList(NULL)
    , mpFreeMsgSlotList(NULL)
    , mpSendBufferList(NULL)
    , mpIDListBufferList(NULL)
{
    mPipeFd[0] = 0;
    mPipeFd[1] = 0;
    mMaxFd = 0;

}

CSACPIMClientAgentV2::~CSACPIMClientAgentV2()
{
    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    if (mpReadBuffer) {
        DDBG_FREE(mpReadBuffer, "ICAR");
        mpReadBuffer = NULL;
    }

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    if (mpResourceMutex) {
        mpResourceMutex->Delete();
        mpResourceMutex = NULL;
    }

    if (mpMsgSlotList) {
        delete mpMsgSlotList;
        mpMsgSlotList = NULL;
    }

    if (mpFreeMsgSlotList) {
        delete mpFreeMsgSlotList;
        mpFreeMsgSlotList = NULL;
    }

    if (mpSendBufferList) {
        delete mpSendBufferList;
        mpSendBufferList = NULL;
    }

    if (mpIDListBufferList) {
        delete mpIDListBufferList;
        mpIDListBufferList = NULL;
    }

}

EECode CSACPIMClientAgentV2::Register(TChar *name, TChar *password, const TChar *server_url, TSocketPort serverport, TUniqueID &id)
{
    AUTO_LOCK(mpMutex);

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

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;
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
    gfSocketSetRecvBufferSize(mSocket, 8 * 1024);
    gfSocketSetSendBufferSize(mSocket, 8 * 1024);
    gfSocketSetNoDelay(mSocket);
    gfSocketSetLinger(mSocket, 0, 0, 20);

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
    DGetSACPSize(length, p_send_buffer);
    DASSERT((sizeof(TUniqueID) + sizeof(STLV16HeaderBigEndian)) == length);
    ret = gfNet_Recv_timeout(mSocket, p_cur, (TInt)length, DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("recieve, return 0, peer closed\n");
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)length)) {
        LOG_ERROR("recieve, return %d, expected %d\n", ret, length);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_NetReceiveHeader_Fail;
    }

    TU16 tlv_type, tlv_length;
    DGetTLV16Type(tlv_type, p_cur);
    DGetTLV16Length(tlv_length, p_cur);
    if (DUnlikely((ETLV16Type_AccountID != tlv_type) || (sizeof(TUniqueID) != tlv_length))) {
        LOG_ERROR("no account id?\n");
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_NetReceiveHeader_Fail;
    }
    p_cur += sizeof(STLV16HeaderBigEndian);

    DBER64(id, p_cur);
    LOG_PRINTF("register (%s, %s), id %llx\n", name, password, id);

    releaseSendBuffer(send_buffer_struct);

    mbConnected = 1;
    startReadLoop();

    return EECode_OK;
}

EECode CSACPIMClientAgentV2::GetSecurityQuestion(TChar *name, SSecureQuestions *p_questions, const TChar *server_url, TSocketPort serverport)
{
    AUTO_LOCK(mpMutex);

    if (DUnlikely(mbConnected)) {
        LOG_FATAL("Should logout first\n");
        return EECode_BadState;
    }

    if (DUnlikely(!name || !server_url || !p_questions)) {
        LOG_FATAL("NULL name, or NULL server_url, or NULL p_questions\n");
        return EECode_BadParam;
    }

    if (DUnlikely(strlen(name) >= DMAX_ACCOUNT_NAME_LENGTH_EX)) {
        LOG_FATAL("name too long, max %d\n", DMAX_ACCOUNT_NAME_LENGTH_EX - 1);
        return EECode_BadParam;
    }

    mSocket = gfNet_ConnectTo(server_url, serverport, SOCK_STREAM, IPPROTO_TCP);
    if (DUnlikely(!DIsSocketHandlerValid(mSocket))) {
        LOG_FATAL("connect to server fail!\n");
        return EECode_NetConnectFail;
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;
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

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ConnectionTag, ESACPClientCmdSubType_GetSecureQuestion);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount ++, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    gfSocketSetTimeout(mSocket, 2, 0);
    gfSocketSetRecvBufferSize(mSocket, 8 * 1024);
    gfSocketSetSendBufferSize(mSocket, 8 * 1024);
    gfSocketSetNoDelay(mSocket);
    gfSocketSetLinger(mSocket, 0, 0, 20);

    TInt ret = gfNet_Send_timeout(mSocket, p_send_buffer, (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID) + cur_size), 0, 1);
    if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID) + cur_size))) {
        LOG_ERROR("send get secure question fail %ld\n", (TULong)sizeof(SSACPHeader) + sizeof(TUniqueID) + cur_size);
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
        LOG_FATAL("get secure question fail, %d %s\n", err, gfGetErrorCodeString(err));
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    p_cur = p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
    DGetSACPSize(length, p_send_buffer);
    ret = gfNet_Recv_timeout(mSocket, p_cur, (TInt)length, DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("recieve, return 0, peer closed\n");
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)length)) {
        LOG_ERROR("recieve, return %d, expected %d\n", ret, length);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_NetReceiveHeader_Fail;
    }

    TU16 tlv_type, tlv_length;
    DGetTLV16Type(tlv_type, p_cur);
    DGetTLV16Length(tlv_length, p_cur);
    if (DUnlikely((ETLV16Type_SecurityQuestion != tlv_type) || (DMAX_SECURE_QUESTION_LENGTH < tlv_length))) {
        LOG_ERROR("not question type %x or bad length %d\n", tlv_type, tlv_length);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_NetReceiveHeader_Fail;
    }
    p_cur += sizeof(STLV16HeaderBigEndian);
    memcpy(p_questions->question1, p_cur, tlv_length);
    p_cur += tlv_length;

    DGetTLV16Type(tlv_type, p_cur);
    DGetTLV16Length(tlv_length, p_cur);
    if (DUnlikely((ETLV16Type_SecurityQuestion != tlv_type) || (DMAX_SECURE_QUESTION_LENGTH < tlv_length))) {
        LOG_ERROR("not question type %x or bad length %d\n", tlv_type, tlv_length);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_NetReceiveHeader_Fail;
    }
    p_cur += sizeof(STLV16HeaderBigEndian);
    memcpy(p_questions->question2, p_cur, tlv_length);

    LOG_PRINTF("get question (%s, %s)\n", p_questions->question1, p_questions->question2);

    releaseSendBuffer(send_buffer_struct);

    gfNetCloseSocket(mSocket);
    mSocket = DInvalidSocketHandler;

    return EECode_OK;
}

EECode CSACPIMClientAgentV2::ResetPasswordBySecurityAnswer(TChar *name, SSecureQuestions *p_questions, TChar *new_password, TUniqueID &id, const TChar *server_url, TSocketPort serverport)
{
    AUTO_LOCK(mpMutex);

    if (DUnlikely(mbConnected)) {
        LOG_FATAL("Should logout first\n");
        return EECode_BadState;
    }

    if (DUnlikely(!name || !server_url || !p_questions || !new_password)) {
        LOG_FATAL("NULL name, or NULL server_url, or NULL p_questions, or NULL new_password\n");
        return EECode_BadParam;
    }

    if (DUnlikely(strlen(name) >= DMAX_ACCOUNT_NAME_LENGTH_EX)) {
        LOG_FATAL("name too long, max %d\n", DMAX_ACCOUNT_NAME_LENGTH_EX - 1);
        return EECode_BadParam;
    }

    if (DUnlikely(strlen(new_password) >= DIdentificationStringLength)) {
        LOG_FATAL("password too long, max %d\n", DIdentificationStringLength - 1);
        return EECode_BadParam;
    }

    if (DUnlikely(0x0 != p_questions->answer1[DMAX_SECURE_ANSWER_LENGTH - 1])) {
        LOG_FATAL("answer1 exceed max length %d\n", DMAX_SECURE_ANSWER_LENGTH - 1);
        return EECode_BadParam;
    }

    if (DUnlikely(0x0 != p_questions->answer2[DMAX_SECURE_ANSWER_LENGTH - 1])) {
        LOG_FATAL("answer2 exceed max length %d\n", DMAX_SECURE_ANSWER_LENGTH - 1);
        return EECode_BadParam;
    }

    mSocket = gfNet_ConnectTo(server_url, serverport, SOCK_STREAM, IPPROTO_TCP);
    if (DUnlikely(!DIsSocketHandlerValid(mSocket))) {
        LOG_FATAL("connect to server fail!\n");
        return EECode_NetConnectFail;
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;
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

    length = strlen(new_password);
    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_AccountPassword >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_AccountPassword) & 0xff;
    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
    p_tlv->length_low = (length + 1) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);
    memcpy(p_cur, new_password, length);
    p_cur += length;
    *p_cur++ = 0x0;
    cur_size += length + 1;

    length = strlen(p_questions->answer1);
    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_SecurityAnswer >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_SecurityAnswer) & 0xff;
    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
    p_tlv->length_low = (length + 1) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);
    memcpy(p_cur, p_questions->answer1, length);
    p_cur += length;
    *p_cur++ = 0x0;
    cur_size += length + 1;

    length = strlen(p_questions->answer2);
    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_SecurityAnswer >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_SecurityAnswer) & 0xff;
    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
    p_tlv->length_low = (length + 1) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);
    memcpy(p_cur, p_questions->answer2, length);
    p_cur += length;
    *p_cur++ = 0x0;
    cur_size += length + 1;

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ConnectionTag, ESACPClientCmdSubType_ResetPassword);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount ++, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    gfSocketSetTimeout(mSocket, 2, 0);
    gfSocketSetRecvBufferSize(mSocket, 8 * 1024);
    gfSocketSetSendBufferSize(mSocket, 8 * 1024);
    gfSocketSetNoDelay(mSocket);
    gfSocketSetLinger(mSocket, 0, 0, 20);

    TInt ret = gfNet_Send_timeout(mSocket, p_send_buffer, (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID) + cur_size), 0, 1);
    if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID) + cur_size))) {
        LOG_ERROR("send reset password fail %ld\n", (TULong)sizeof(SSACPHeader) + sizeof(TUniqueID) + cur_size);
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
        LOG_FATAL("reset password fail, %d %s\n", err, gfGetErrorCodeString(err));
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return err;
    }

    p_cur = p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
    DGetSACPSize(length, p_send_buffer);
    ret = gfNet_Recv_timeout(mSocket, p_cur, (TInt)length, DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("recieve, return 0, peer closed\n");
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)length)) {
        LOG_ERROR("recieve, return %d, expected %d\n", ret, length);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_NetReceiveHeader_Fail;
    }

    TU16 tlv_type, tlv_length;
    DGetTLV16Type(tlv_type, p_cur);
    DGetTLV16Length(tlv_length, p_cur);
    if (DUnlikely((ETLV16Type_AccountID != tlv_type) || (sizeof(TUniqueID) != tlv_length))) {
        LOG_ERROR("not id type %x or bad length %d\n", tlv_type, tlv_length);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_ServerReject_CorruptedProtocol;
    }
    p_cur += sizeof(STLV16HeaderBigEndian);
    DBER64(id, p_cur);

    LOG_PRINTF("reset password done [%s, %llx]: %s\n", name, id, new_password);

    releaseSendBuffer(send_buffer_struct);

    gfNetCloseSocket(mSocket);
    mSocket = DInvalidSocketHandler;

    return EECode_OK;
}

EECode CSACPIMClientAgentV2::AcquireOwnership(TChar *device_code, TUniqueID &id)
{
    id = DInvalidUniqueID;

    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(!device_code)) {
        LOG_FATAL("device code is NULL\n");
        return EECode_BadParam;
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;
    TU8 *p_cur = p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU32 length = strlen(device_code);
    TU16 cur_size = 0;

    //debug check
    if (DUnlikely(length > 512)) {
        LOG_FATAL("too long production code\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_BadParam;
    }

    CIConditionSlot *p_slot = allocMsgSlot();
    if (DUnlikely(!p_slot)) {
        LOG_FATAL("NULL p_slot\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_NoMemory;
    }

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_SourceDeviceProductCode >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_SourceDeviceProductCode) & 0xff;
    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
    p_tlv->length_low = (length + 1) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);
    memcpy(p_cur, device_code, length);
    p_cur += length;
    *p_cur++ = 0x0;
    cur_size += length + 1;

    mpMutex->Lock();

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkOwnSourceDevice);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);

    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    p_slot->Reset();
    p_slot->SetSessionNumber(mSeqCount ++);
    p_slot->SetCheckField((TU32)ESACPClientCmdSubType_SinkOwnSourceDevice);
    SDateTime time;
    gfGetCurrentDateTime(&time);
    p_slot->SetTime(&time);

    mpMsgSlotList->InsertContent(NULL, (void *) p_slot, 0);

    TInt payload_size = cur_size + sizeof(SSACPHeader) + sizeof(TUniqueID);
    TInt sentlength = gfNet_Send_timeout(mSocket, p_send_buffer, payload_size, 0, 2);

    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, peer socket is closed!\n");
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_Closed;
    } else if (DUnlikely(payload_size != sentlength)) {
        LOG_FATAL("send data fail, payload_size %d, sentlength %d\n", payload_size, sentlength);
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_NetSocketSend_Error;
    }

    TPointer reply_context = 0;
    TU32 reply_type = 0;
    TU32 reply_count = 0;
    EECode ret_code = EECode_OK;

    p_slot->Wait();
    p_slot->GetReplyCode(ret_code);
    p_slot->GetReplyContext(reply_context, reply_type, reply_count, id);
    LOG_PRINTF("acquire device id %llx\n", id);
    mpMutex->Unlock();

    releaseSendBuffer(send_buffer_struct);
    releaseMsgSlot(p_slot);

    return ret_code;
}

EECode CSACPIMClientAgentV2::DonateOwnership(TChar *donate_target, TUniqueID device_id)
{
    TU32 unbind = 0;

    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(!donate_target)) {
        unbind = 1;
        LOG_WARN("unbind is not safe, it's recommand to use donate\n");
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;
    TU8 *p_cur = p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU32 length = 0;
    if (!unbind) {
        length = strlen(donate_target);
    }
    TU16 cur_size = 0;

    //debug check
    if (DUnlikely(length > 32)) {
        LOG_FATAL("too long donate_target\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_BadParam;
    }

    CIConditionSlot *p_slot = allocMsgSlot();
    if (DUnlikely(!p_slot)) {
        LOG_FATAL("NULL p_slot\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_NoMemory;
    }

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_AccountName >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_AccountName) & 0xff;
    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
    p_tlv->length_low = (length + 1) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);
    if (!unbind) {
        memcpy(p_cur, donate_target, length);
    }
    p_cur += length;
    *p_cur++ = 0x0;
    cur_size += length + 1;

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_AccountID >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_AccountID) & 0xff;
    p_tlv->length_high = 0;
    p_tlv->length_low = sizeof(TUniqueID);
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);

    p_cur[0] = (device_id >> 56) & 0xff;
    p_cur[1] = (device_id >> 48) & 0xff;
    p_cur[2] = (device_id >> 40) & 0xff;
    p_cur[3] = (device_id >> 32) & 0xff;
    p_cur[4] = (device_id >> 24) & 0xff;
    p_cur[5] = (device_id >> 16) & 0xff;
    p_cur[6] = (device_id >> 8) & 0xff;
    p_cur[7] = (device_id) & 0xff;
    p_cur += sizeof(TUniqueID);
    cur_size += sizeof(TUniqueID);

    mpMutex->Lock();

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkDonateSourceDevice);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    p_slot->Reset();
    p_slot->SetSessionNumber(mSeqCount ++);
    p_slot->SetCheckField((TU32)ESACPClientCmdSubType_SinkDonateSourceDevice);
    SDateTime time;
    gfGetCurrentDateTime(&time);
    p_slot->SetTime(&time);

    mpMsgSlotList->InsertContent(NULL, (void *) p_slot, 0);

    TInt payload_size = cur_size + sizeof(SSACPHeader) + sizeof(TUniqueID);
    TInt sentlength = gfNet_Send_timeout(mSocket, p_send_buffer, payload_size, 0, 2);

    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, peer socket is closed!\n");
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_Closed;
    } else if (DUnlikely(payload_size != sentlength)) {
        LOG_FATAL("send data fail, payload_size %d, sentlength %d\n", payload_size, sentlength);
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_NetSocketSend_Error;
    }

    EECode ret_code = EECode_OK;

    p_slot->Wait();
    p_slot->GetReplyCode(ret_code);

    mpMutex->Unlock();

    releaseSendBuffer(send_buffer_struct);
    releaseMsgSlot(p_slot);

    return ret_code;
}

EECode CSACPIMClientAgentV2::GrantPrivilegeSingleDevice(TUniqueID device_id, TUniqueID usr_id, TU32 privilege)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely((DInvalidUniqueID == device_id) || (DInvalidUniqueID == usr_id))) {
        LOG_FATAL("bad device_id or usr_id\n");
        return EECode_BadParam;
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;
    TU8 *p_cur = p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU16 cur_size = 0;

    CIConditionSlot *p_slot = allocMsgSlot();
    if (DUnlikely(!p_slot)) {
        LOG_FATAL("NULL p_slot\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_NoMemory;
    }

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_AccountID >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_AccountID) & 0xff;
    p_tlv->length_high = 0;
    p_tlv->length_low = sizeof(TUniqueID);
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);

    p_cur[0] = (usr_id >> 56) & 0xff;
    p_cur[1] = (usr_id >> 48) & 0xff;
    p_cur[2] = (usr_id >> 40) & 0xff;
    p_cur[3] = (usr_id >> 32) & 0xff;
    p_cur[4] = (usr_id >> 24) & 0xff;
    p_cur[5] = (usr_id >> 16) & 0xff;
    p_cur[6] = (usr_id >> 8) & 0xff;
    p_cur[7] = (usr_id) & 0xff;
    p_cur += sizeof(TUniqueID);
    cur_size += sizeof(TUniqueID);

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_AccountID >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_AccountID) & 0xff;
    p_tlv->length_high = 0;
    p_tlv->length_low = sizeof(TUniqueID);
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);

    p_cur[0] = (device_id >> 56) & 0xff;
    p_cur[1] = (device_id >> 48) & 0xff;
    p_cur[2] = (device_id >> 40) & 0xff;
    p_cur[3] = (device_id >> 32) & 0xff;
    p_cur[4] = (device_id >> 24) & 0xff;
    p_cur[5] = (device_id >> 16) & 0xff;
    p_cur[6] = (device_id >> 8) & 0xff;
    p_cur[7] = (device_id) & 0xff;
    p_cur += sizeof(TUniqueID);
    cur_size += sizeof(TUniqueID);

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_UserPrivilegeMask >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_UserPrivilegeMask) & 0xff;
    p_tlv->length_high = 0;
    p_tlv->length_low = sizeof(TU32);
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);

    p_cur[0] = (privilege >> 24) & 0xff;
    p_cur[1] = (privilege >> 16) & 0xff;
    p_cur[2] = (privilege >> 8) & 0xff;
    p_cur[3] = (privilege) & 0xff;
    p_cur += sizeof(TU32);
    cur_size += sizeof(TU32);

    mpMutex->Lock();

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkUpdateFriendPrivilege);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    p_slot->Reset();
    p_slot->SetSessionNumber(mSeqCount ++);
    p_slot->SetCheckField((TU32)ESACPClientCmdSubType_SinkUpdateFriendPrivilege);
    SDateTime time;
    gfGetCurrentDateTime(&time);
    p_slot->SetTime(&time);

    mpMsgSlotList->InsertContent(NULL, (void *) p_slot, 0);

    TInt payload_size = cur_size + sizeof(SSACPHeader) + sizeof(TUniqueID);
    TInt sentlength = gfNet_Send_timeout(mSocket, p_send_buffer, payload_size, 0, 2);

    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, peer socket is closed!\n");
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_Closed;
    } else if (DUnlikely(payload_size != sentlength)) {
        LOG_FATAL("send data fail, payload_size %d, sentlength %d\n", payload_size, sentlength);
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_NetSocketSend_Error;
    }

    EECode ret_code = EECode_OK;

    p_slot->Wait();
    p_slot->GetReplyCode(ret_code);

    mpMutex->Unlock();

    releaseSendBuffer(send_buffer_struct);
    releaseMsgSlot(p_slot);

    return ret_code;
}

EECode CSACPIMClientAgentV2::GrantPrivilege(TUniqueID *device_id, TU32 device_count, TUniqueID usr_id, TU32 privilege)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely((!device_id) || (!device_count))) {
        LOG_FATAL("bad params\n");
        return EECode_BadParam;
    }

    if (DUnlikely(DInvalidUniqueID == usr_id)) {
        LOG_FATAL("bad usr_id\n");
        return EECode_BadParam;
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024 + (device_count * sizeof(TUniqueID)));
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;
    TU8 *p_cur = p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU32 length = 0;
    TU16 cur_size = 0;

    CIConditionSlot *p_slot = allocMsgSlot();
    if (DUnlikely(!p_slot)) {
        LOG_FATAL("NULL p_slot\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_NoMemory;
    }

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_AccountID >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_AccountID) & 0xff;
    p_tlv->length_high = 0;
    p_tlv->length_low = sizeof(TUniqueID);
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);

    p_cur[0] = (usr_id >> 56) & 0xff;
    p_cur[1] = (usr_id >> 48) & 0xff;
    p_cur[2] = (usr_id >> 40) & 0xff;
    p_cur[3] = (usr_id >> 32) & 0xff;
    p_cur[4] = (usr_id >> 24) & 0xff;
    p_cur[5] = (usr_id >> 16) & 0xff;
    p_cur[6] = (usr_id >> 8) & 0xff;
    p_cur[7] = (usr_id) & 0xff;
    p_cur += sizeof(TUniqueID);
    cur_size += sizeof(TUniqueID);

    length = device_count * sizeof(TUniqueID);
    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_DeviceIDList >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_DeviceIDList) & 0xff;
    p_tlv->length_high = (length >> 8) & 0xff;
    p_tlv->length_low = length & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);

    for (TU32 i = 0; i < device_count; i ++) {
        p_cur[0] = (device_id[i] >> 56) & 0xff;
        p_cur[1] = (device_id[i] >> 48) & 0xff;
        p_cur[2] = (device_id[i] >> 40) & 0xff;
        p_cur[3] = (device_id[i] >> 32) & 0xff;
        p_cur[4] = (device_id[i] >> 24) & 0xff;
        p_cur[5] = (device_id[i] >> 16) & 0xff;
        p_cur[6] = (device_id[i] >> 8) & 0xff;
        p_cur[7] = (device_id[i]) & 0xff;
        p_cur += sizeof(TUniqueID);
        cur_size += sizeof(TUniqueID);
    }

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_UserPrivilegeMask >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_UserPrivilegeMask) & 0xff;
    p_tlv->length_high = 0;
    p_tlv->length_low = sizeof(TU32);
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);

    p_cur[0] = (privilege >> 24) & 0xff;
    p_cur[1] = (privilege >> 16) & 0xff;
    p_cur[2] = (privilege >> 8) & 0xff;
    p_cur[3] = (privilege) & 0xff;
    p_cur += sizeof(TU32);
    cur_size += sizeof(TU32);

    mpMutex->Lock();

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkUpdateFriendPrivilege);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    p_slot->Reset();
    p_slot->SetSessionNumber(mSeqCount ++);
    p_slot->SetCheckField((TU32)ESACPClientCmdSubType_SinkUpdateFriendPrivilege);
    SDateTime time;
    gfGetCurrentDateTime(&time);
    p_slot->SetTime(&time);

    mpMsgSlotList->InsertContent(NULL, (void *) p_slot, 0);

    TInt payload_size = cur_size + sizeof(SSACPHeader) + sizeof(TUniqueID);
    TInt sentlength = gfNet_Send_timeout(mSocket, p_send_buffer, payload_size, 0, 2);

    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, peer socket is closed!\n");
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_Closed;
    } else if (DUnlikely(payload_size != sentlength)) {
        LOG_FATAL("send data fail, payload_size %d, sentlength %d\n", payload_size, sentlength);
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_NetSocketSend_Error;
    }

    EECode ret_code = EECode_OK;

    p_slot->Wait();
    p_slot->GetReplyCode(ret_code);

    mpMutex->Unlock();

    releaseSendBuffer(send_buffer_struct);
    releaseMsgSlot(p_slot);

    return ret_code;
}

EECode CSACPIMClientAgentV2::AddFriend(TChar *friend_user_name, TUniqueID &friend_id)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(!friend_user_name)) {
        LOG_FATAL("friend_user_name is NULL\n");
        return EECode_BadParam;
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;
    TU8 *p_cur = p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU32 length = strlen(friend_user_name);
    TU16 cur_size = 0;

    if (DUnlikely(length > (DMAX_ACCOUNT_NAME_LENGTH_EX - 1))) {
        LOG_FATAL("too long friend_user_name, %d\n", length);
        releaseSendBuffer(send_buffer_struct);
        return EECode_BadParam;
    }

    CIConditionSlot *p_slot = allocMsgSlot();
    if (DUnlikely(!p_slot)) {
        LOG_FATAL("NULL p_slot\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_NoMemory;
    }

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_AccountName >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_AccountName) & 0xff;
    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
    p_tlv->length_low = (length + 1) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);
    memcpy(p_cur, friend_user_name, length);
    p_cur += length;
    *p_cur++ = 0x0;
    cur_size += length + 1;

    mpMutex->Lock();

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkInviteFriend);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    p_slot->Reset();
    p_slot->SetSessionNumber(mSeqCount ++);
    p_slot->SetCheckField((TU32)ESACPClientCmdSubType_SinkInviteFriend);
    SDateTime time;
    gfGetCurrentDateTime(&time);
    p_slot->SetTime(&time);

    mpMsgSlotList->InsertContent(NULL, (void *) p_slot, 0);

    TInt payload_size = cur_size + sizeof(SSACPHeader) + sizeof(TUniqueID);
    TInt sentlength = gfNet_Send_timeout(mSocket, p_send_buffer, payload_size, 0, 2);

    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, peer socket is closed!\n");
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_Closed;
    } else if (DUnlikely(payload_size != sentlength)) {
        LOG_FATAL("send data fail, payload_size %d, sentlength %d\n", payload_size, sentlength);
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_NetSocketSend_Error;
    }

    EECode ret_code = EECode_OK;
    TPointer reply_context = 0;
    TU32 reply_type = 0, reply_count = 0;

    p_slot->Wait();
    p_slot->GetReplyCode(ret_code);
    p_slot->GetReplyContext(reply_context, reply_type, reply_count, friend_id);
    LOG_NOTICE("friend_id %llx, ret_code %d\n", friend_id, ret_code);

    mpMutex->Unlock();

    releaseSendBuffer(send_buffer_struct);
    releaseMsgSlot(p_slot);

    return ret_code;
}

EECode CSACPIMClientAgentV2::AddFriendByID(TUniqueID friend_id, TChar *friend_user_name)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(!friend_user_name)) {
        LOG_FATAL("friend_user_name is NULL\n");
        return EECode_BadParam;
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;
    TU8 *p_cur = p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU32 length = sizeof(TUniqueID);
    TU16 cur_size = 0;

    CIConditionSlot *p_slot = allocMsgSlot();
    if (DUnlikely(!p_slot)) {
        LOG_FATAL("NULL p_slot\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_NoMemory;
    }

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_AccountID >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_AccountID) & 0xff;
    p_tlv->length_high = ((length) >> 8) & 0xff;
    p_tlv->length_low = (length) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);
    DBEW64(friend_id, p_cur);
    p_cur += length;
    cur_size += length;

    mpMutex->Lock();

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkInviteFriendByID);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    p_slot->Reset();
    p_slot->SetReplyContext((TPointer)friend_user_name, 0, 0, DInvalidUniqueID);
    p_slot->SetSessionNumber(mSeqCount ++);
    p_slot->SetCheckField((TU32)ESACPClientCmdSubType_SinkInviteFriendByID);

    SDateTime time;
    gfGetCurrentDateTime(&time);
    p_slot->SetTime(&time);

    mpMsgSlotList->InsertContent(NULL, (void *) p_slot, 0);

    TInt payload_size = cur_size + sizeof(SSACPHeader) + sizeof(TUniqueID);
    TInt sentlength = gfNet_Send_timeout(mSocket, p_send_buffer, payload_size, 0, 2);

    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, peer socket is closed!\n");
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_Closed;
    } else if (DUnlikely(payload_size != sentlength)) {
        LOG_FATAL("send data fail, payload_size %d, sentlength %d\n", payload_size, sentlength);
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_NetSocketSend_Error;
    }

    EECode ret_code = EECode_OK;
    //TPointer reply_context = 0;
    //TU32 reply_type = 0, reply_count = 0;

    p_slot->Wait();
    p_slot->GetReplyCode(ret_code);

    mpMutex->Unlock();

    releaseSendBuffer(send_buffer_struct);
    releaseMsgSlot(p_slot);

    return ret_code;
}

EECode CSACPIMClientAgentV2::AcceptFriend(TUniqueID friend_id)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(DInvalidUniqueID == friend_id)) {
        LOG_FATAL("bad friend_id %llx\n", friend_id);
        return EECode_BadParam;
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;
    TU8 *p_cur = p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU16 cur_size = 0;

    CIConditionSlot *p_slot = allocMsgSlot();
    if (DUnlikely(!p_slot)) {
        LOG_FATAL("NULL p_slot\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_NoMemory;
    }

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_AccountID >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_AccountID) & 0xff;
    p_tlv->length_high = 0;
    p_tlv->length_low = sizeof(TUniqueID);
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);

    p_cur[0] = (friend_id >> 56) & 0xff;
    p_cur[1] = (friend_id >> 48) & 0xff;
    p_cur[2] = (friend_id >> 40) & 0xff;
    p_cur[3] = (friend_id >> 32) & 0xff;
    p_cur[4] = (friend_id >> 24) & 0xff;
    p_cur[5] = (friend_id >> 16) & 0xff;
    p_cur[6] = (friend_id >> 8) & 0xff;
    p_cur[7] = (friend_id) & 0xff;
    p_cur += sizeof(TUniqueID);
    cur_size += sizeof(TUniqueID);

    mpMutex->Lock();

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkAcceptFriend);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    p_slot->Reset();
    p_slot->SetSessionNumber(mSeqCount ++);
    p_slot->SetCheckField((TU32)ESACPClientCmdSubType_SinkAcceptFriend);
    SDateTime time;
    gfGetCurrentDateTime(&time);
    p_slot->SetTime(&time);

    mpMsgSlotList->InsertContent(NULL, (void *) p_slot, 0);

    TInt payload_size = cur_size + sizeof(SSACPHeader) + sizeof(TUniqueID);
    TInt sentlength = gfNet_Send_timeout(mSocket, p_send_buffer, payload_size, 0, 2);

    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, peer socket is closed!\n");
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_Closed;
    } else if (DUnlikely(payload_size != sentlength)) {
        LOG_FATAL("send data fail, payload_size %d, sentlength %d\n", payload_size, sentlength);
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_NetSocketSend_Error;
    }

    EECode ret_code = EECode_OK;

    p_slot->Wait();
    p_slot->GetReplyCode(ret_code);

    mpMutex->Unlock();

    releaseSendBuffer(send_buffer_struct);
    releaseMsgSlot(p_slot);

    return ret_code;
}

EECode CSACPIMClientAgentV2::RemoveFriend(TUniqueID friend_id)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(DInvalidUniqueID != friend_id)) {
        LOG_FATAL("bad friend_id\n");
        return EECode_BadParam;
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;
    TU8 *p_cur = p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU16 cur_size = 0;

    CIConditionSlot *p_slot = allocMsgSlot();
    if (DUnlikely(!p_slot)) {
        LOG_FATAL("NULL p_slot\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_NoMemory;
    }

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_AccountID >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_AccountID) & 0xff;
    p_tlv->length_high = 0;
    p_tlv->length_low = sizeof(TUniqueID);
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);

    p_cur[0] = (friend_id >> 56) & 0xff;
    p_cur[1] = (friend_id >> 48) & 0xff;
    p_cur[2] = (friend_id >> 40) & 0xff;
    p_cur[3] = (friend_id >> 32) & 0xff;
    p_cur[4] = (friend_id >> 24) & 0xff;
    p_cur[5] = (friend_id >> 16) & 0xff;
    p_cur[6] = (friend_id >> 8) & 0xff;
    p_cur[7] = (friend_id) & 0xff;
    p_cur += sizeof(TUniqueID);
    cur_size += sizeof(TUniqueID);

    mpMutex->Lock();

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkRemoveFriend);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    p_slot->Reset();
    p_slot->SetSessionNumber(mSeqCount ++);
    p_slot->SetCheckField((TU32)ESACPClientCmdSubType_SinkRemoveFriend);
    SDateTime time;
    gfGetCurrentDateTime(&time);
    p_slot->SetTime(&time);

    mpMsgSlotList->InsertContent(NULL, (void *) p_slot, 0);

    TInt payload_size = cur_size + sizeof(SSACPHeader) + sizeof(TUniqueID);
    TInt sentlength = gfNet_Send_timeout(mSocket, p_send_buffer, payload_size, 0, 2);

    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, peer socket is closed!\n");
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_Closed;
    } else if (DUnlikely(payload_size != sentlength)) {
        LOG_FATAL("send data fail, payload_size %d, sentlength %d\n", payload_size, sentlength);
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_NetSocketSend_Error;
    }

    EECode ret_code = EECode_OK;

    p_slot->Wait();
    p_slot->GetReplyCode(ret_code);

    mpMutex->Unlock();

    releaseSendBuffer(send_buffer_struct);
    releaseMsgSlot(p_slot);

    return ret_code;
}

EECode CSACPIMClientAgentV2::RetrieveFriendList(SAgentIDListBuffer *&id_list)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;

    CIConditionSlot *p_slot = allocMsgSlot();
    if (DUnlikely(!p_slot)) {
        LOG_FATAL("NULL p_slot\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_NoMemory;
    }

    mpMutex->Lock();

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkRetrieveFriendsList);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, 0, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    p_slot->Reset();
    p_slot->SetSessionNumber(mSeqCount ++);
    p_slot->SetCheckField((TU32)ESACPClientCmdSubType_SinkRetrieveFriendsList);
    SDateTime time;
    gfGetCurrentDateTime(&time);
    p_slot->SetTime(&time);

    mpMsgSlotList->InsertContent(NULL, (void *) p_slot, 0);

    TInt payload_size =  sizeof(SSACPHeader) + sizeof(TUniqueID);
    TInt sentlength = gfNet_Send_timeout(mSocket, p_send_buffer, payload_size, 0, 2);

    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, peer socket is closed!\n");
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_Closed;
    } else if (DUnlikely(payload_size != sentlength)) {
        LOG_FATAL("send data fail, payload_size %d, sentlength %d\n", payload_size, sentlength);
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_NetSocketSend_Error;
    }

    TPointer reply_context = 0;
    TU32 reply_type = 0;
    TU32 reply_count = 0;
    TUniqueID id = 0;
    EECode ret_code = EECode_OK;

    p_slot->Wait();
    p_slot->GetReplyCode(ret_code);
    p_slot->GetReplyContext(reply_context, reply_type, reply_count, id);

    mpMutex->Unlock();

    releaseSendBuffer(send_buffer_struct);
    releaseMsgSlot(p_slot);

    id_list = (SAgentIDListBuffer *)reply_context;

    return ret_code;
}

EECode CSACPIMClientAgentV2::RetrieveAdminDeviceList(SAgentIDListBuffer *&id_list)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;

    CIConditionSlot *p_slot = allocMsgSlot();
    if (DUnlikely(!p_slot)) {
        LOG_FATAL("NULL p_slot\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_NoMemory;
    }

    mpMutex->Lock();

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkRetrieveAdminDeviceList);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, 0, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    p_slot->Reset();
    p_slot->SetSessionNumber(mSeqCount ++);
    p_slot->SetCheckField((TU32)ESACPClientCmdSubType_SinkRetrieveAdminDeviceList);
    SDateTime time;
    gfGetCurrentDateTime(&time);
    p_slot->SetTime(&time);

    mpMsgSlotList->InsertContent(NULL, (void *) p_slot, 0);

    TInt payload_size =  sizeof(SSACPHeader) + sizeof(TUniqueID);
    TInt sentlength = gfNet_Send_timeout(mSocket, p_send_buffer, payload_size, 0, 2);

    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, peer socket is closed!\n");
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_Closed;
    } else if (DUnlikely(payload_size != sentlength)) {
        LOG_FATAL("send data fail, payload_size %d, sentlength %d\n", payload_size, sentlength);
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_NetSocketSend_Error;
    }

    TPointer reply_context = 0;
    TU32 reply_type = 0;
    TU32 reply_count = 0;
    TUniqueID id = 0;
    EECode ret_code = EECode_OK;

    p_slot->Wait();
    p_slot->GetReplyCode(ret_code);
    p_slot->GetReplyContext(reply_context, reply_type, reply_count, id);

    mpMutex->Unlock();

    releaseSendBuffer(send_buffer_struct);
    releaseMsgSlot(p_slot);

    id_list = (SAgentIDListBuffer *)reply_context;

    return ret_code;
}

EECode CSACPIMClientAgentV2::RetrieveSharedDeviceList(SAgentIDListBuffer *&id_list)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;

    CIConditionSlot *p_slot = allocMsgSlot();
    if (DUnlikely(!p_slot)) {
        LOG_FATAL("NULL p_slot\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_NoMemory;
    }

    mpMutex->Lock();

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkRetrieveSharedDeviceList);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, 0, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    p_slot->Reset();
    p_slot->SetSessionNumber(mSeqCount ++);
    p_slot->SetCheckField((TU32)ESACPClientCmdSubType_SinkRetrieveSharedDeviceList);
    SDateTime time;
    gfGetCurrentDateTime(&time);
    p_slot->SetTime(&time);

    mpMsgSlotList->InsertContent(NULL, (void *) p_slot, 0);

    TInt payload_size =  sizeof(SSACPHeader) + sizeof(TUniqueID);
    TInt sentlength = gfNet_Send_timeout(mSocket, p_send_buffer, payload_size, 0, 2);

    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, peer socket is closed!\n");
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_Closed;
    } else if (DUnlikely(payload_size != sentlength)) {
        LOG_FATAL("send data fail, payload_size %d, sentlength %d\n", payload_size, sentlength);
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_NetSocketSend_Error;
    }

    TPointer reply_context = 0;
    TU32 reply_type = 0;
    TU32 reply_count = 0;
    TUniqueID id = 0;
    EECode ret_code = EECode_OK;

    p_slot->Wait();
    p_slot->GetReplyCode(ret_code);
    p_slot->GetReplyContext(reply_context, reply_type, reply_count, id);

    mpMutex->Unlock();

    releaseSendBuffer(send_buffer_struct);
    releaseMsgSlot(p_slot);

    id_list = (SAgentIDListBuffer *)reply_context;

    return ret_code;
}

EECode CSACPIMClientAgentV2::QueryDeivceProfile(TUniqueID device_id, SDeviceProfile *p_profile)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(DInvalidUniqueID == device_id)) {
        LOG_FATAL("bad device_id\n");
        return EECode_BadParam;
    }

    if (DUnlikely(!p_profile)) {
        LOG_FATAL("NULL p_profile\n");
        return EECode_BadParam;
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;
    TU8 *p_cur = p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU16 cur_size = 0;

    CIConditionSlot *p_slot = allocMsgSlot();
    if (DUnlikely(!p_slot)) {
        LOG_FATAL("NULL p_slot\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_NoMemory;
    }

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_AccountID >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_AccountID) & 0xff;
    p_tlv->length_high = 0;
    p_tlv->length_low = sizeof(TUniqueID);
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);

    p_cur[0] = (device_id >> 56) & 0xff;
    p_cur[1] = (device_id >> 48) & 0xff;
    p_cur[2] = (device_id >> 40) & 0xff;
    p_cur[3] = (device_id >> 32) & 0xff;
    p_cur[4] = (device_id >> 24) & 0xff;
    p_cur[5] = (device_id >> 16) & 0xff;
    p_cur[6] = (device_id >> 8) & 0xff;
    p_cur[7] = (device_id) & 0xff;
    p_cur += sizeof(TUniqueID);
    cur_size += sizeof(TUniqueID);

    mpMutex->Lock();

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkQuerySource);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    p_slot->Reset();
    p_slot->SetSessionNumber(mSeqCount ++);
    p_slot->SetCheckField((TU32)ESACPClientCmdSubType_SinkQuerySource);
    SDateTime time;
    gfGetCurrentDateTime(&time);
    p_slot->SetTime(&time);
    p_slot->SetReplyContext((TPointer)p_profile, 0, 0, 0);

    mpMsgSlotList->InsertContent(NULL, (void *) p_slot, 0);

    TInt payload_size = cur_size + sizeof(SSACPHeader) + sizeof(TUniqueID);
    TInt sentlength = gfNet_Send_timeout(mSocket, p_send_buffer, payload_size, 0, 2);

    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, peer socket is closed!\n");
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_Closed;
    } else if (DUnlikely(payload_size != sentlength)) {
        LOG_FATAL("send data fail, payload_size %d, sentlength %d\n", payload_size, sentlength);
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_NetSocketSend_Error;
    }

    EECode ret_code = EECode_OK;

    p_slot->Wait();
    p_slot->GetReplyCode(ret_code);

    mpMutex->Unlock();

    releaseSendBuffer(send_buffer_struct);
    releaseMsgSlot(p_slot);

    return ret_code;
}

EECode CSACPIMClientAgentV2::QueryFriendInfo(TUniqueID friend_id, SFriendInfo *info)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(DInvalidUniqueID == friend_id)) {
        LOG_FATAL("bad friend_id\n");
        return EECode_BadParam;
    }

    if (DUnlikely(!info)) {
        LOG_FATAL("NULL info\n");
        return EECode_BadParam;
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;
    TU8 *p_cur = p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU16 cur_size = 0;

    CIConditionSlot *p_slot = allocMsgSlot();
    if (DUnlikely(!p_slot)) {
        LOG_FATAL("NULL p_slot\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_NoMemory;
    }

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_AccountID >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_AccountID) & 0xff;
    p_tlv->length_high = 0;
    p_tlv->length_low = sizeof(TUniqueID);
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);

    p_cur[0] = (friend_id >> 56) & 0xff;
    p_cur[1] = (friend_id >> 48) & 0xff;
    p_cur[2] = (friend_id >> 40) & 0xff;
    p_cur[3] = (friend_id >> 32) & 0xff;
    p_cur[4] = (friend_id >> 24) & 0xff;
    p_cur[5] = (friend_id >> 16) & 0xff;
    p_cur[6] = (friend_id >> 8) & 0xff;
    p_cur[7] = (friend_id) & 0xff;
    p_cur += sizeof(TUniqueID);
    cur_size += sizeof(TUniqueID);

    mpMutex->Lock();

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkQueryFriendInfo);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    p_slot->Reset();
    p_slot->SetSessionNumber(mSeqCount ++);
    p_slot->SetCheckField((TU32)ESACPClientCmdSubType_SinkQueryFriendInfo);
    SDateTime time;
    gfGetCurrentDateTime(&time);
    p_slot->SetTime(&time);
    p_slot->SetReplyContext((TPointer)info, 0, 0, 0);

    mpMsgSlotList->InsertContent(NULL, (void *) p_slot, 0);

    TInt payload_size = cur_size + sizeof(SSACPHeader) + sizeof(TUniqueID);
    TInt sentlength = gfNet_Send_timeout(mSocket, p_send_buffer, payload_size, 0, 2);

    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, peer socket is closed!\n");
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_Closed;
    } else if (DUnlikely(payload_size != sentlength)) {
        LOG_FATAL("send data fail, payload_size %d, sentlength %d\n", payload_size, sentlength);
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_NetSocketSend_Error;
    }

    EECode ret_code = EECode_OK;

    p_slot->Wait();
    p_slot->GetReplyCode(ret_code);

    mpMutex->Unlock();

    releaseSendBuffer(send_buffer_struct);
    releaseMsgSlot(p_slot);

    return ret_code;
}

void CSACPIMClientAgentV2::FreeIDList(SAgentIDListBuffer *id_list)
{
    releaseIDListBuffer(id_list);
}

EECode CSACPIMClientAgentV2::Login(TUniqueID id, TChar *password, TChar *server_url, TSocketPort server_port, TChar *&cloud_server_url, TSocketPort &cloud_server_port, TU64 &dynamic_code_input)
{
    AUTO_LOCK(mpMutex);
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

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ConnectionTag, ESACPClientCmdSubType_SourceAuthentication);
    TU8 *p_cur = send_buffer_struct->p_send_buffer + sizeof(SSACPHeader);
    TInt ret = 0;
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU16 tlv_type = 0, tlv_size = 0;
    TInt length = 0;

    mServerPort = server_port;

    if (DUnlikely(!server_url)) {
        LOG_ERROR("NULL server_url\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_BadParam;
    } else {
        LOG_NOTICE("server_url %s, server_port %hu\n", server_url, mServerPort);
    }

    if (DIsSocketHandlerValid(mSocket)) {
        LOG_WARN("close previous socket\n");
        gfNetCloseSocket(mSocket);
        releaseSendBuffer(send_buffer_struct);
        mSocket = DInvalidSocketHandler;
    }

    mSocket = gfNet_ConnectTo((const TChar *)server_url, mServerPort, SOCK_STREAM, IPPROTO_TCP);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("gfNet_ConnectTo(%s:%hu) fail\n", server_url, mServerPort);
        releaseSendBuffer(send_buffer_struct);
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

    gfSACPFillHeader((SSACPHeader *)send_buffer_struct->p_send_buffer, type, length, (TU8)EProtocolHeaderExtentionType_SACP_IM, (TU16)sizeof(TUniqueID), mSeqCount ++, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);

    length += sizeof(SSACPHeader) + sizeof(TUniqueID);
    gfSocketSetTimeout(mSocket, 2, 0);
    gfSocketSetRecvBufferSize(mSocket, 8 * 1024);
    gfSocketSetSendBufferSize(mSocket, 8 * 1024);
    gfSocketSetNoDelay(mSocket);
    gfSocketSetLinger(mSocket, 0, 0, 20);

    ret = gfNet_Send_timeout(mSocket, send_buffer_struct->p_send_buffer, length, 0, 1);
    if (DUnlikely(ret !=  length)) {
        LOG_ERROR("send fail\n");
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        releaseSendBuffer(send_buffer_struct);
        return EECode_NetSendHeader_Fail;
    }

    ret = gfNet_Recv_timeout(mSocket, send_buffer_struct->p_send_buffer, (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)), DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)))) {
        LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + sizeof(TUniqueID));
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        releaseSendBuffer(send_buffer_struct);
        return EECode_NetReceiveHeader_Fail;
    }

    SSACPHeader *p_sacp_header = (SSACPHeader *)send_buffer_struct->p_send_buffer;
    EECode err = gfSACPConnectResultToErrorCode((ESACPConnectResult)p_sacp_header->flags);
    length = ((TU32)p_sacp_header->size_0 << 8) | ((TU32)p_sacp_header->size_1 << 8) | ((TU32)p_sacp_header->size_2);
    if (DLikely(EECode_OK == err)) {
        DASSERT((DDynamicInputStringLength + sizeof(STLV16HeaderBigEndian)) == length);
        p_cur = send_buffer_struct->p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
        ret = gfNet_Recv_timeout(mSocket, p_cur, length, DNETUTILS_RECEIVE_FLAG_READ_ALL, 20);
        if (DUnlikely((TInt)length != ret)) {
            LOG_WARN("Login, recv dynamic_input1 %d not expected, expect %d, peer closed?\n", ret, length);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            releaseSendBuffer(send_buffer_struct);
            return EECode_NetReceivePayload_Fail;
        }
        p_tlv = (STLV16HeaderBigEndian *) p_cur;
        tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
        tlv_size = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
        if ((ETLV16Type_DynamicInput != tlv_type) || (DDynamicInputStringLength != tlv_size)) {
            LOG_WARN("dynamic code input not as expected, %d, %d\n", tlv_type, tlv_size);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            releaseSendBuffer(send_buffer_struct);
            return EECode_ServerReject_CorruptedProtocol;
        }
        p_cur += sizeof(STLV16HeaderBigEndian);

        memcpy(mIdentityString, p_cur, DDynamicInputStringLength);
        memset(mIdentityString + DDynamicInputStringLength, 0x0, DIdentificationStringLength);
        strcpy((TChar *)mIdentityString + DDynamicInputStringLength, password);
        //gfPrintMemory(mIdentityString, DDynamicInputStringLength + DIdentificationStringLength);
        TU32 dynamic_code = gfHashAlgorithmV6(mIdentityString, DDynamicInputStringLength + DIdentificationStringLength);

        gfSACPFillHeader((SSACPHeader *)send_buffer_struct->p_send_buffer, type, sizeof(dynamic_code) + sizeof(STLV16HeaderBigEndian), (TU8)EProtocolHeaderExtentionType_SACP_IM, (TU16)sizeof(TUniqueID), mSeqCount ++, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
        p_cur = send_buffer_struct->p_send_buffer + sizeof(SSACPHeader);
        memset(p_cur, 0x0, sizeof(TUniqueID));
        p_cur += (TU16)sizeof(TUniqueID);
        p_tlv = (STLV16HeaderBigEndian *) p_cur;
        tlv_size = sizeof(dynamic_code);
        p_tlv->type_high = (ETLV16Type_AuthenticationResult32 >> 8) & 0xff;
        p_tlv->type_low = (ETLV16Type_AuthenticationResult32) & 0xff;
        p_tlv->length_high = ((tlv_size) >> 8) & 0xff;
        p_tlv->length_low = (tlv_size) & 0xff;
        p_cur += sizeof(STLV16HeaderBigEndian);
        DBEW32(dynamic_code, p_cur);

        length = sizeof(SSACPHeader) + (TU16)sizeof(TUniqueID) + sizeof(dynamic_code) + sizeof(STLV16HeaderBigEndian);
        ret = gfNet_Send_timeout(mSocket, send_buffer_struct->p_send_buffer, length, 0, 1);
        if (DUnlikely(ret != length)) {
            LOG_WARN("Login, send dynamic code fail, ret %d\n", ret);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            releaseSendBuffer(send_buffer_struct);
            return EECode_NetSendHeader_Fail;
        }

        ret = gfNet_Recv_timeout(mSocket, send_buffer_struct->p_send_buffer, (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)), DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            releaseSendBuffer(send_buffer_struct);
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            releaseSendBuffer(send_buffer_struct);
            return EECode_NetReceiveHeader_Fail;
        }
        SSACPHeader *p_sacp_header = (SSACPHeader *)send_buffer_struct->p_send_buffer;
        length = ((TU32)p_sacp_header->size_0 << 8) | ((TU32)p_sacp_header->size_1 << 8) | ((TU32)p_sacp_header->size_2);
        p_cur = send_buffer_struct->p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
        ret = gfNet_Recv_timeout(mSocket, p_cur, length, DNETUTILS_RECEIVE_FLAG_READ_ALL, 20);
        if (DUnlikely((TInt)length != ret)) {
            LOG_WARN("Login, recv dynamic_input2 %d not expected, expect %d, peer closed?\n", ret, length);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            releaseSendBuffer(send_buffer_struct);
            return EECode_NetReceivePayload_Fail;
        }

        err = gfSACPConnectResultToErrorCode((ESACPConnectResult)p_sacp_header->flags);
        if (DLikely((EECode_OK == err) || (EECode_OK_NeedSetOwner))) {
            p_tlv = (STLV16HeaderBigEndian *) p_cur;
            tlv_type = (p_tlv->type_high << 8) | p_tlv->type_low;
            tlv_size = (p_tlv->length_high << 8) | p_tlv->length_low;
            p_cur += sizeof(STLV16HeaderBigEndian);

            if (DUnlikely((ETLV16Type_DynamicInput != tlv_type) || (DDynamicInputStringLength != tlv_size))) {
                LOG_ERROR("not dynamic input?(tlv_type=%hu, tlv_size=%hu)\n", tlv_type, tlv_size);
                gfNetCloseSocket(mSocket);
                mSocket = DInvalidSocketHandler;
                releaseSendBuffer(send_buffer_struct);
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
                releaseSendBuffer(send_buffer_struct);
                return EECode_ServerReject_CorruptedProtocol;
            }
            memset(mCloudServerUrl, 0x0, sizeof(mCloudServerUrl));
            memcpy(mCloudServerUrl, p_cur, tlv_size);
            cloud_server_url = mCloudServerUrl;
            p_cur += tlv_size;

            p_tlv = (STLV16HeaderBigEndian *) p_cur;
            tlv_type = (p_tlv->type_high << 8) | p_tlv->type_low;
            tlv_size = (p_tlv->length_high << 8) | p_tlv->length_low;
            p_cur += sizeof(STLV16HeaderBigEndian);
            if (DUnlikely((ETLV16Type_DataNodeCloudPort != tlv_type) || (2 != tlv_size))) {
                LOG_ERROR("not cloud port?\n");
                gfNetCloseSocket(mSocket);
                mSocket = DInvalidSocketHandler;
                releaseSendBuffer(send_buffer_struct);
                return EECode_ServerReject_CorruptedProtocol;
            }
            DBER16(cloud_server_port, p_cur);
            p_cur += tlv_size;

        } else {
            LOG_ERROR("authenticate fail? server return, %d, %s\n", err, gfGetErrorCodeString(err));
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            releaseSendBuffer(send_buffer_struct);
            return EECode_NetReceiveHeader_Fail;
        }

        LOG_NOTICE("connect success, client recv: url=%s, port=%hu, dyanmic_input2=%llx\n", cloud_server_url, cloud_server_port, dynamic_code_input);
    } else {
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        releaseSendBuffer(send_buffer_struct);
        LOG_ERROR("server reject, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    mbConnected = 1;
    startReadLoop();
    releaseSendBuffer(send_buffer_struct);

    return err;
}

EECode CSACPIMClientAgentV2::LoginUserAccount(TChar *name, TChar *password, const TChar *server_url, TSocketPort server_port, TUniqueID &id)
{
    AUTO_LOCK(mpMutex);
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

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ConnectionTag, ESACPClientCmdSubType_SinkAuthentication);
    TU8 *p_cur = send_buffer_struct->p_send_buffer + sizeof(SSACPHeader);
    TInt ret = 0;
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU16 tlv_type = 0, tlv_size = 0;
    TInt length = 0;

    mServerPort = server_port;

    if (DUnlikely(!server_url)) {
        LOG_ERROR("NULL server_url\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_BadParam;
    } else {
        LOG_NOTICE("server_url %s, server_port %hu\n", server_url, mServerPort);
    }

    if (DIsSocketHandlerValid(mSocket)) {
        LOG_WARN("close previous socket\n");
        gfNetCloseSocket(mSocket);
        releaseSendBuffer(send_buffer_struct);
        mSocket = DInvalidSocketHandler;
    }

    mSocket = gfNet_ConnectTo((const TChar *)server_url, mServerPort, SOCK_STREAM, IPPROTO_TCP);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("gfNet_ConnectTo(%s:%hu) fail\n", server_url, mServerPort);
        releaseSendBuffer(send_buffer_struct);
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
    gfSACPFillHeader((SSACPHeader *)send_buffer_struct->p_send_buffer, type, length, (TU8)EProtocolHeaderExtentionType_SACP_IM, (TU16)sizeof(TUniqueID), mSeqCount ++, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);

    LOG_NOTICE("user login, %s, payload length %d\n", name, length);
    //gfPrintMemory(send_buffer_struct->p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID), length);

    length += sizeof(SSACPHeader) + sizeof(TUniqueID);
    gfSocketSetTimeout(mSocket, 2, 0);
    gfSocketSetRecvBufferSize(mSocket, 8 * 1024);
    gfSocketSetSendBufferSize(mSocket, 8 * 1024);
    gfSocketSetNoDelay(mSocket);
    gfSocketSetLinger(mSocket, 0, 0, 20);

    ret = gfNet_Send_timeout(mSocket, send_buffer_struct->p_send_buffer, length, 0, 1);
    if (DUnlikely(ret != length)) {
        LOG_ERROR("send fail\n");
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        releaseSendBuffer(send_buffer_struct);
        return EECode_NetSendHeader_Fail;
    }

    ret = gfNet_Recv_timeout(mSocket, send_buffer_struct->p_send_buffer, (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)), DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)))) {
        LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + sizeof(TUniqueID));
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        releaseSendBuffer(send_buffer_struct);
        return EECode_NetReceiveHeader_Fail;
    }

    SSACPHeader *p_sacp_header = (SSACPHeader *)send_buffer_struct->p_send_buffer;
    EECode err = gfSACPConnectResultToErrorCode((ESACPConnectResult)p_sacp_header->flags);
    length = ((TU32)p_sacp_header->size_0 << 8) | ((TU32)p_sacp_header->size_1 << 8) | ((TU32)p_sacp_header->size_2);
    if (DLikely(EECode_OK == err)) {
        DASSERT((DDynamicInputStringLength + sizeof(STLV16HeaderBigEndian)) == length);
        p_cur = send_buffer_struct->p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
        ret = gfNet_Recv_timeout(mSocket, p_cur, length, DNETUTILS_RECEIVE_FLAG_READ_ALL, 20);
        if (DUnlikely((TInt)length != ret)) {
            LOG_WARN("Login, recv dynamic_input1 %d not expected, expect %d, peer closed?\n", ret, length);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            releaseSendBuffer(send_buffer_struct);
            return EECode_NetReceivePayload_Fail;
        }
        p_tlv = (STLV16HeaderBigEndian *) p_cur;
        tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
        tlv_size = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
        if ((ETLV16Type_DynamicInput != tlv_type) || (DDynamicInputStringLength != tlv_size)) {
            LOG_WARN("dynamic code input not as expected, %d, %d\n", tlv_type, tlv_size);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            releaseSendBuffer(send_buffer_struct);
            return EECode_ServerReject_CorruptedProtocol;
        }
        p_cur += sizeof(STLV16HeaderBigEndian);

        memcpy(mIdentityString, p_cur, DDynamicInputStringLength);
        memset(mIdentityString + DDynamicInputStringLength, 0x0, DIdentificationStringLength);
        strcpy((TChar *)mIdentityString + DDynamicInputStringLength, password);
        TU32 dynamic_code = gfHashAlgorithmV6(mIdentityString, DDynamicInputStringLength + DIdentificationStringLength);

        gfSACPFillHeader((SSACPHeader *)send_buffer_struct->p_send_buffer, type, sizeof(dynamic_code) + sizeof(STLV16HeaderBigEndian), (TU8)EProtocolHeaderExtentionType_SACP_IM, (TU16)sizeof(TUniqueID), mSeqCount ++, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
        p_cur = send_buffer_struct->p_send_buffer + sizeof(SSACPHeader);
        memset(p_cur, 0x0, sizeof(TUniqueID));
        p_cur += (TU16)sizeof(TUniqueID);
        p_tlv = (STLV16HeaderBigEndian *) p_cur;
        tlv_size = sizeof(dynamic_code);
        p_tlv->type_high = (ETLV16Type_AuthenticationResult32 >> 8) & 0xff;
        p_tlv->type_low = (ETLV16Type_AuthenticationResult32) & 0xff;
        p_tlv->length_high = ((tlv_size) >> 8) & 0xff;
        p_tlv->length_low = (tlv_size) & 0xff;
        p_cur += sizeof(STLV16HeaderBigEndian);
        DBEW32(dynamic_code, p_cur);

        length = sizeof(SSACPHeader) + (TU16)sizeof(TUniqueID) + sizeof(dynamic_code) + sizeof(STLV16HeaderBigEndian);
        ret = gfNet_Send_timeout(mSocket, send_buffer_struct->p_send_buffer, length, 0, 1);
        if (DUnlikely(ret != length)) {
            LOG_WARN("Login, send dynamic code fail, ret %d\n", ret);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            releaseSendBuffer(send_buffer_struct);
            return EECode_NetSendHeader_Fail;
        }

        ret = gfNet_Recv_timeout(mSocket, send_buffer_struct->p_send_buffer, (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)), DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            releaseSendBuffer(send_buffer_struct);
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            releaseSendBuffer(send_buffer_struct);
            return EECode_NetReceiveHeader_Fail;
        }

        err = gfSACPConnectResultToErrorCode((ESACPConnectResult)p_sacp_header->flags);
        if (DLikely(EECode_OK != err)) {
            LOG_ERROR("authenticate fail? server return, %d, %s\n", err, gfGetErrorCodeString(err));
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            releaseSendBuffer(send_buffer_struct);
            return err;
        }

        p_cur = send_buffer_struct->p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
        DGetSACPSize(length, send_buffer_struct->p_send_buffer);
        ret = gfNet_Recv_timeout(mSocket, p_cur, (TInt)length, DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("recieve, return 0, peer closed\n");
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)length)) {
            LOG_ERROR("recieve, return %d, expected %d\n", ret, length);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_NetReceiveHeader_Fail;
        }

        DGetTLV16Type(tlv_type, p_cur);
        DGetTLV16Length(tlv_size, p_cur);
        if (DUnlikely((ETLV16Type_AccountID != tlv_type) || (sizeof(TUniqueID) != tlv_size))) {
            LOG_ERROR("not id type %x or bad length %d\n", tlv_type, tlv_size);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_ServerReject_CorruptedProtocol;
        }
        p_cur += sizeof(STLV16HeaderBigEndian);
        DBER64(id, p_cur);

        LOG_NOTICE("connect success, id %llx\n", id);
    } else {
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        releaseSendBuffer(send_buffer_struct);
        LOG_ERROR("server reject, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    mbConnected = 1;
    startReadLoop();
    releaseSendBuffer(send_buffer_struct);

    return EECode_OK;
}

EECode CSACPIMClientAgentV2::LoginByUserID(TUniqueID id, TChar *password, const TChar *server_url, TSocketPort server_port)
{
    AUTO_LOCK(mpMutex);
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

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ConnectionTag, ESACPClientCmdSubType_SinkAuthentication);
    TU8 *p_cur = send_buffer_struct->p_send_buffer + sizeof(SSACPHeader);
    TInt ret = 0;
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU16 tlv_type = 0, tlv_size = 0;
    TInt length = 0;

    mServerPort = server_port;

    if (DUnlikely(!server_url)) {
        LOG_ERROR("NULL server_url\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_BadParam;
    } else {
        LOG_NOTICE("server_url %s, server_port %hu\n", server_url, mServerPort);
    }

    if (DIsSocketHandlerValid(mSocket)) {
        LOG_WARN("close previous socket\n");
        gfNetCloseSocket(mSocket);
        releaseSendBuffer(send_buffer_struct);
        mSocket = DInvalidSocketHandler;
    }

    mSocket = gfNet_ConnectTo((const TChar *)server_url, mServerPort, SOCK_STREAM, IPPROTO_TCP);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("gfNet_ConnectTo(%s:%hu) fail\n", server_url, mServerPort);
        releaseSendBuffer(send_buffer_struct);
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
    length = sizeof(STLV16HeaderBigEndian) + sizeof(TUniqueID);
    DBEW64(id, p_cur);

    gfSACPFillHeader((SSACPHeader *)send_buffer_struct->p_send_buffer, type, length, (TU8)EProtocolHeaderExtentionType_SACP_IM, (TU16)sizeof(TUniqueID), mSeqCount ++, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);

    length += sizeof(SSACPHeader) + sizeof(TUniqueID);

    gfSocketSetTimeout(mSocket, 2, 0);
    gfSocketSetRecvBufferSize(mSocket, 8 * 1024);
    gfSocketSetSendBufferSize(mSocket, 8 * 1024);
    gfSocketSetNoDelay(mSocket);
    gfSocketSetLinger(mSocket, 0, 0, 20);

    ret = gfNet_Send_timeout(mSocket, send_buffer_struct->p_send_buffer, length, 0, 1);
    if (DUnlikely(ret != length)) {
        LOG_ERROR("send fail\n");
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        releaseSendBuffer(send_buffer_struct);
        return EECode_NetSendHeader_Fail;
    }

    ret = gfNet_Recv_timeout(mSocket, send_buffer_struct->p_send_buffer, (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)), DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        return EECode_Closed;
    } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)))) {
        LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader) + sizeof(TUniqueID));
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        releaseSendBuffer(send_buffer_struct);
        return EECode_NetReceiveHeader_Fail;
    }

    SSACPHeader *p_sacp_header = (SSACPHeader *)send_buffer_struct->p_send_buffer;
    EECode err = gfSACPConnectResultToErrorCode((ESACPConnectResult)p_sacp_header->flags);
    length = ((TU32)p_sacp_header->size_0 << 8) | ((TU32)p_sacp_header->size_1 << 8) | ((TU32)p_sacp_header->size_2);
    if (DLikely(EECode_OK == err)) {
        DASSERT((DDynamicInputStringLength + sizeof(STLV16HeaderBigEndian)) == length);
        p_cur = send_buffer_struct->p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
        ret = gfNet_Recv_timeout(mSocket, p_cur, length, DNETUTILS_RECEIVE_FLAG_READ_ALL, 20);
        if (DUnlikely((TInt)length != ret)) {
            LOG_WARN("Login, recv dynamic_input1 %d not expected, expect %d, peer closed?\n", ret, length);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            releaseSendBuffer(send_buffer_struct);
            return EECode_NetReceivePayload_Fail;
        }
        p_tlv = (STLV16HeaderBigEndian *) p_cur;
        tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
        tlv_size = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
        if ((ETLV16Type_DynamicInput != tlv_type) || (DDynamicInputStringLength != tlv_size)) {
            LOG_WARN("dynamic code input not as expected, %d, %d\n", tlv_type, tlv_size);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            releaseSendBuffer(send_buffer_struct);
            return EECode_ServerReject_CorruptedProtocol;
        }
        p_cur += sizeof(STLV16HeaderBigEndian);

        memcpy(mIdentityString, p_cur, DDynamicInputStringLength);
        memset(mIdentityString + DDynamicInputStringLength, 0x0, DIdentificationStringLength);
        strcpy((TChar *)mIdentityString + DDynamicInputStringLength, password);
        TU32 dynamic_code = gfHashAlgorithmV6(mIdentityString, DDynamicInputStringLength + DIdentificationStringLength);

        gfSACPFillHeader((SSACPHeader *)send_buffer_struct->p_send_buffer, type, sizeof(dynamic_code) + sizeof(STLV16HeaderBigEndian), (TU8)EProtocolHeaderExtentionType_SACP_IM, (TU16)sizeof(TUniqueID), mSeqCount ++, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
        p_cur = send_buffer_struct->p_send_buffer + sizeof(SSACPHeader);
        memset(p_cur, 0x0, sizeof(TUniqueID));
        p_cur += (TU16)sizeof(TUniqueID);
        p_tlv = (STLV16HeaderBigEndian *) p_cur;
        tlv_size = sizeof(dynamic_code);
        p_tlv->type_high = (ETLV16Type_AuthenticationResult32 >> 8) & 0xff;
        p_tlv->type_low = (ETLV16Type_AuthenticationResult32) & 0xff;
        p_tlv->length_high = ((tlv_size) >> 8) & 0xff;
        p_tlv->length_low = (tlv_size) & 0xff;
        p_cur += sizeof(STLV16HeaderBigEndian);
        DBEW32(dynamic_code, p_cur);

        length = sizeof(SSACPHeader) + (TU16)sizeof(TUniqueID) + sizeof(dynamic_code) + sizeof(STLV16HeaderBigEndian);
        ret = gfNet_Send_timeout(mSocket, send_buffer_struct->p_send_buffer, length, 0, 1);
        if (DUnlikely(ret != length)) {
            LOG_WARN("Login, send dynamic code fail, ret %d\n", ret);
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            releaseSendBuffer(send_buffer_struct);
            return EECode_NetSendHeader_Fail;
        }

        ret = gfNet_Recv_timeout(mSocket, send_buffer_struct->p_send_buffer, (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)), DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
        if (DUnlikely(0 == ret)) {
            LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            releaseSendBuffer(send_buffer_struct);
            return EECode_Closed;
        } else if (DUnlikely(ret != (TInt)(sizeof(SSACPHeader) + sizeof(TUniqueID)))) {
            LOG_ERROR("gfNet_Recv(), return %d, expected %lu\n", ret, (TULong)sizeof(SSACPHeader));
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            releaseSendBuffer(send_buffer_struct);
            return EECode_NetReceiveHeader_Fail;
        }

        err = gfSACPConnectResultToErrorCode((ESACPConnectResult)p_sacp_header->flags);
        if (DLikely(EECode_OK != err)) {
            LOG_ERROR("authenticate fail? server return, %d, %s\n", err, gfGetErrorCodeString(err));
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            releaseSendBuffer(send_buffer_struct);
            return EECode_NetReceiveHeader_Fail;
        }

        LOG_NOTICE("connect success\n");
    } else {
        mbConnected = 0;
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
        releaseSendBuffer(send_buffer_struct);
        LOG_ERROR("server reject, err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }
    mbConnected = 1;
    startReadLoop();
    releaseSendBuffer(send_buffer_struct);

    return EECode_OK;
}

EECode CSACPIMClientAgentV2::Logout()
{
    if (!mbConnected) {
        LOG_WARN("not concected yet\n");
        return EECode_BadState;
    }

    stopReadLoop();

    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    mbConnected = 0;

    return EECode_OK;
}

EECode CSACPIMClientAgentV2::SetOwner(TChar *owner)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(!owner)) {
        LOG_FATAL("owner is NULL\n");
        return EECode_BadParam;
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;
    TU8 *p_cur = p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU32 length = strlen(owner);
    TU16 cur_size = 0;

    if (DUnlikely(length > (DMAX_ACCOUNT_NAME_LENGTH_EX - 1))) {
        LOG_FATAL("too long friend_user_name, %d\n", length);
        releaseSendBuffer(send_buffer_struct);
        return EECode_BadParam;
    }

    CIConditionSlot *p_slot = allocMsgSlot();
    if (DUnlikely(!p_slot)) {
        LOG_FATAL("NULL p_slot\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_NoMemory;
    }

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_AccountName >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_AccountName) & 0xff;
    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
    p_tlv->length_low = (length + 1) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);
    memcpy(p_cur, owner, length);
    p_cur += length;
    *p_cur++ = 0x0;
    cur_size += length + 1;

    mpMutex->Lock();

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SourceSetOwner);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    p_slot->Reset();
    p_slot->SetSessionNumber(mSeqCount ++);
    p_slot->SetCheckField((TU32)ESACPClientCmdSubType_SourceSetOwner);
    SDateTime time;
    gfGetCurrentDateTime(&time);
    p_slot->SetTime(&time);

    mpMsgSlotList->InsertContent(NULL, (void *) p_slot, 0);

    TInt payload_size = cur_size + sizeof(SSACPHeader) + sizeof(TUniqueID);
    TInt sentlength = gfNet_Send_timeout(mSocket, p_send_buffer, payload_size, 0, 2);

    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, peer socket is closed!\n");
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_Closed;
    } else if (DUnlikely(payload_size != sentlength)) {
        LOG_FATAL("send data fail, payload_size %d, sentlength %d\n", payload_size, sentlength);
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_NetSocketSend_Error;
    }

    EECode ret_code = EECode_OK;

    p_slot->Wait();
    p_slot->GetReplyCode(ret_code);

    mpMutex->Unlock();

    releaseSendBuffer(send_buffer_struct);
    releaseMsgSlot(p_slot);

    return ret_code;
}

EECode CSACPIMClientAgentV2::PostMessage(TUniqueID id, TU8 *message, TU32 message_length, TU8 user_protocol)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(DInvalidUniqueID == id)) {
        LOG_FATAL("invalid id\n");
        return EECode_BadParam;
    }

    if (DUnlikely((!message) || (!message_length))) {
        LOG_FATAL("bad params\n");
        return EECode_BadParam;
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(128 + message_length);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;
    TU8 *p_cur = p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);

    memcpy(p_cur, message, message_length);

    mpMutex->Lock();

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientRelayChannel, user_protocol);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, message_length, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
    if (user_protocol >= ESACPCmdSubType_UserCustomizedBase) {
        DBEW64(id, (p_send_buffer + sizeof(SSACPHeader)));
    } else {
        memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));
    }

    mSeqCount ++;

    TInt payload_size = message_length + sizeof(SSACPHeader) + sizeof(TUniqueID);
    TInt sentlength = gfNet_Send_timeout(mSocket, p_send_buffer, payload_size, 0, 2);

    //LOG_INFO("[DEBUG]: post msg (to %llx), size %d\n", id, payload_size);
    //gfPrintMemory(p_send_buffer, payload_size);

    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, peer socket is closed!\n");
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        return EECode_Closed;
    } else if (DUnlikely(payload_size != sentlength)) {
        LOG_FATAL("send data fail, payload_size %d, sentlength %d\n", payload_size, sentlength);
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        return EECode_NetSocketSend_Error;
    }

    mpMutex->Unlock();

    releaseSendBuffer(send_buffer_struct);

    return EECode_OK;
}

EECode CSACPIMClientAgentV2::SendMessage(TUniqueID id, TU8 *message, TU32 message_length, TU8 user_protocol)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(DInvalidUniqueID == id)) {
        LOG_FATAL("invalid id\n");
        return EECode_BadParam;
    }

    if (DUnlikely((!message) || (!message_length))) {
        LOG_FATAL("bad params\n");
        return EECode_BadParam;
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(128 + message_length);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;
    TU8 *p_cur = p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);

    CIConditionSlot *p_slot = allocMsgSlot();
    if (DUnlikely(!p_slot)) {
        LOG_FATAL("NULL p_slot\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_NoMemory;
    }

    memcpy(p_cur, message, message_length);

    mpMutex->Lock();

    TU32 type = DBuildSACPType(DSACPTypeBit_Request | DSACPTypeBit_NeedReply, ESACPTypeCategory_ClientRelayChannel, user_protocol);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, message_length, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    p_slot->Reset();
    p_slot->SetSessionNumber(mSeqCount ++);
    p_slot->SetCheckField((TU32)user_protocol);
    SDateTime time;
    gfGetCurrentDateTime(&time);
    p_slot->SetTime(&time);
    mpMsgSlotList->InsertContent(NULL, (void *) p_slot, 0);

    TInt payload_size = message_length + sizeof(SSACPHeader) + sizeof(TUniqueID);
    TInt sentlength = gfNet_Send_timeout(mSocket, p_send_buffer, payload_size, 0, 2);

    LOG_NOTICE("[DEBUG]: send msg (to %llx), size %d\n", id, payload_size);
    //gfPrintMemory(p_send_buffer, payload_size);

    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, peer socket is closed!\n");
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_Closed;
    } else if (DUnlikely(payload_size != sentlength)) {
        LOG_FATAL("send data fail, payload_size %d, sentlength %d\n", payload_size, sentlength);
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_NetSocketSend_Error;
    }

    TPointer reply_context = 0;
    TU32 reply_type = 0;
    TU32 reply_count = 0;
    EECode ret_code = EECode_OK;

    p_slot->Wait();
    p_slot->GetReplyCode(ret_code);
    p_slot->GetReplyContext(reply_context, reply_type, reply_count, id);

    mpMutex->Unlock();

    releaseSendBuffer(send_buffer_struct);
    releaseMsgSlot(p_slot);

    return ret_code;
}

EECode CSACPIMClientAgentV2::UpdateNickname(TChar *nickname)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(!nickname)) {
        LOG_FATAL("nickname is NULL\n");
        return EECode_BadParam;
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;
    TU8 *p_cur = p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU32 length = strlen(nickname);
    TU16 cur_size = 0;

    //debug check
    if (DUnlikely(length >= DMAX_ACCOUNT_NAME_LENGTH_EX)) {
        LOG_FATAL("too long nickname %d, max %d\n", length, DMAX_ACCOUNT_NAME_LENGTH_EX - 1);
        releaseSendBuffer(send_buffer_struct);
        return EECode_BadParam;
    }

    CIConditionSlot *p_slot = allocMsgSlot();
    if (DUnlikely(!p_slot)) {
        LOG_FATAL("NULL p_slot\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_NoMemory;
    }

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_AccountNickName >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_AccountNickName) & 0xff;
    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
    p_tlv->length_low = (length + 1) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);
    memcpy(p_cur, nickname, length);
    p_cur += length;
    *p_cur++ = 0x0;
    cur_size += length + 1;

    mpMutex->Lock();

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_UpdateSinkNickname);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);

    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    p_slot->Reset();
    p_slot->SetSessionNumber(mSeqCount ++);
    p_slot->SetCheckField((TU32)ESACPClientCmdSubType_UpdateSinkNickname);
    SDateTime time;
    gfGetCurrentDateTime(&time);
    p_slot->SetTime(&time);

    mpMsgSlotList->InsertContent(NULL, (void *) p_slot, 0);

    TInt payload_size = cur_size + sizeof(SSACPHeader) + sizeof(TUniqueID);
    TInt sentlength = gfNet_Send_timeout(mSocket, p_send_buffer, payload_size, 0, 2);

    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, peer socket is closed!\n");
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_Closed;
    } else if (DUnlikely(payload_size != sentlength)) {
        LOG_FATAL("send data fail, payload_size %d, sentlength %d\n", payload_size, sentlength);
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_NetSocketSend_Error;
    }

    EECode ret_code = EECode_OK;

    p_slot->Wait();
    p_slot->GetReplyCode(ret_code);
    LOG_PRINTF("update nickname [%s], ret %d, %s \n", nickname, ret_code, gfGetErrorCodeString(ret_code));
    mpMutex->Unlock();

    releaseSendBuffer(send_buffer_struct);
    releaseMsgSlot(p_slot);

    return ret_code;
}

EECode CSACPIMClientAgentV2::UpdateDeviceNickname(TUniqueID device_id, TChar *nickname)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(DInvalidUniqueID == device_id)) {
        LOG_FATAL("invalid device_id\n");
        return EECode_BadParam;
    }

    if (DUnlikely(!nickname)) {
        LOG_FATAL("nickname is NULL\n");
        return EECode_BadParam;
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;
    TU8 *p_cur = p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU32 length = strlen(nickname);
    TU16 cur_size = 0;

    //debug check
    if (DUnlikely(length >= DMAX_ACCOUNT_NAME_LENGTH_EX)) {
        LOG_FATAL("too long nickname %d, max %d\n", length, DMAX_ACCOUNT_NAME_LENGTH_EX - 1);
        releaseSendBuffer(send_buffer_struct);
        return EECode_BadParam;
    }

    CIConditionSlot *p_slot = allocMsgSlot();
    if (DUnlikely(!p_slot)) {
        LOG_FATAL("NULL p_slot\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_NoMemory;
    }

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_AccountNickName >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_AccountNickName) & 0xff;
    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
    p_tlv->length_low = (length + 1) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);
    memcpy(p_cur, nickname, length);
    p_cur += length;
    *p_cur++ = 0x0;
    cur_size += length + 1;

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    length = sizeof(TUniqueID);
    p_tlv->type_high = (ETLV16Type_AccountID >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_AccountID) & 0xff;
    p_tlv->length_high = (length >> 8) & 0xff;
    p_tlv->length_low = length & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);
    DBEW64(device_id, p_cur);
    p_cur += length;
    cur_size += length;

    mpMutex->Lock();

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_UpdateSourceNickname);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);

    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    p_slot->Reset();
    p_slot->SetSessionNumber(mSeqCount ++);
    p_slot->SetCheckField((TU32)ESACPClientCmdSubType_UpdateSourceNickname);
    SDateTime time;
    gfGetCurrentDateTime(&time);
    p_slot->SetTime(&time);

    mpMsgSlotList->InsertContent(NULL, (void *) p_slot, 0);

    TInt payload_size = cur_size + sizeof(SSACPHeader) + sizeof(TUniqueID);
    TInt sentlength = gfNet_Send_timeout(mSocket, p_send_buffer, payload_size, 0, 2);

    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, peer socket is closed!\n");
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_Closed;
    } else if (DUnlikely(payload_size != sentlength)) {
        LOG_FATAL("send data fail, payload_size %d, sentlength %d\n", payload_size, sentlength);
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_NetSocketSend_Error;
    }

    EECode ret_code = EECode_OK;

    p_slot->Wait();
    p_slot->GetReplyCode(ret_code);
    LOG_PRINTF("update source nickname [%s], ret %d, %s \n", nickname, ret_code, gfGetErrorCodeString(ret_code));
    mpMutex->Unlock();

    releaseSendBuffer(send_buffer_struct);
    releaseMsgSlot(p_slot);

    return ret_code;
}

EECode CSACPIMClientAgentV2::UpdatePassword(TChar *ori_password, TChar *password)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely(!ori_password)) {
        LOG_FATAL("ori_password is NULL\n");
        return EECode_BadParam;
    }

    if (DUnlikely(!password)) {
        LOG_FATAL("password is NULL\n");
        return EECode_BadParam;
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;
    TU8 *p_cur = p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU32 length = strlen(password);
    TU16 cur_size = 0;

    //debug check
    if (DUnlikely(length > DIdentificationStringLength)) {
        LOG_FATAL("too long password %d, max %d\n", length, DIdentificationStringLength);
        releaseSendBuffer(send_buffer_struct);
        return EECode_BadParam;
    }

    length = strlen(ori_password);
    if (DUnlikely(length > DIdentificationStringLength)) {
        LOG_FATAL("too long ori_password %d, max %d\n", length, DIdentificationStringLength);
        releaseSendBuffer(send_buffer_struct);
        return EECode_BadParam;
    }

    CIConditionSlot *p_slot = allocMsgSlot();
    if (DUnlikely(!p_slot)) {
        LOG_FATAL("NULL p_slot\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_NoMemory;
    }

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    length = DDynamicInputStringLength;
    p_tlv->type_high = (ETLV16Type_ClientDynamicInput >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_ClientDynamicInput) & 0xff;
    p_tlv->length_high = ((length) >> 8) & 0xff;
    p_tlv->length_low = (length) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);
    TU8 tmp_buffer[64] = {0};
    gfGetCurrentDateTime((SDateTime *) tmp_buffer);
    tmp_buffer[sizeof(SDateTime)] = (tmp_buffer[7] + 0x6d) ^ (0x7c & tmp_buffer[0]);
    tmp_buffer[sizeof(SDateTime) + 1] = (tmp_buffer[6] ^ 0x34) ^ (0xd2 + tmp_buffer[1]);
    tmp_buffer[sizeof(SDateTime) + 2] = (tmp_buffer[5] & 0xac) ^ (0x94 ^ tmp_buffer[2]);
    tmp_buffer[sizeof(SDateTime) + 3] = (tmp_buffer[4] | 0x5a) ^ (0xcc & tmp_buffer[3]);
    TU64 dynamic_input = gfHashAlgorithmV7(tmp_buffer, sizeof(SDateTime) + 4);
    DBEW64(dynamic_input, tmp_buffer);
    memcpy(p_cur, tmp_buffer, DDynamicInputStringLength);
    p_cur += length;
    cur_size += length;

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    length = sizeof(TU32);
    p_tlv->type_high = (ETLV16Type_DynamicCode >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_DynamicCode) & 0xff;
    p_tlv->length_high = ((length) >> 8) & 0xff;
    p_tlv->length_low = (length) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);
    memset(tmp_buffer + DDynamicInputStringLength, 0x0, DIdentificationStringLength);
    memcpy(tmp_buffer + DDynamicInputStringLength, ori_password, strlen(ori_password));
    LOG_NOTICE("hash_input:\n");
    //gfPrintMemory(tmp_buffer, DDynamicInputStringLength + DIdentificationStringLength);
    TU32 dynamic_code = gfHashAlgorithmV8(tmp_buffer, DDynamicInputStringLength + DIdentificationStringLength);
    DBEW32(dynamic_code, p_cur);
    p_cur += length;
    cur_size += length;

    LOG_NOTICE("hash_recv:, dynamic_code %x, dynamic_input %llx\n", dynamic_code, dynamic_input);
    //gfPrintMemory(p_cur, 4);

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    length = strlen(password);
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

    mpMutex->Lock();

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_UpdateSinkPassword);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);

    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    p_slot->Reset();
    p_slot->SetSessionNumber(mSeqCount ++);
    p_slot->SetCheckField((TU32)ESACPClientCmdSubType_UpdateSinkPassword);
    SDateTime time;
    gfGetCurrentDateTime(&time);
    p_slot->SetTime(&time);

    mpMsgSlotList->InsertContent(NULL, (void *) p_slot, 0);

    TInt payload_size = cur_size + sizeof(SSACPHeader) + sizeof(TUniqueID);
    TInt sentlength = gfNet_Send_timeout(mSocket, p_send_buffer, payload_size, 0, 2);

    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, peer socket is closed!\n");
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_Closed;
    } else if (DUnlikely(payload_size != sentlength)) {
        LOG_FATAL("send data fail, payload_size %d, sentlength %d\n", payload_size, sentlength);
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_NetSocketSend_Error;
    }

    EECode ret_code = EECode_OK;

    p_slot->Wait();
    p_slot->GetReplyCode(ret_code);
    LOG_PRINTF("update password [%s], ret %d, %s \n", password, ret_code, gfGetErrorCodeString(ret_code));
    mpMutex->Unlock();

    releaseSendBuffer(send_buffer_struct);
    releaseMsgSlot(p_slot);

    return ret_code;
}

EECode CSACPIMClientAgentV2::UpdateSecurityQuestion(SSecureQuestions *p_questions, TChar *password)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    if (DUnlikely((!password) || !p_questions)) {
        LOG_FATAL("password or p_questions is NULL\n");
        return EECode_BadParam;
    }

    if (DUnlikely(0x0 != p_questions->question1[DMAX_SECURE_QUESTION_LENGTH - 1])) {
        LOG_FATAL("question1 exceed max length %d\n", DMAX_SECURE_QUESTION_LENGTH - 1);
        return EECode_BadParam;
    }

    if (DUnlikely(0x0 != p_questions->question2[DMAX_SECURE_QUESTION_LENGTH - 1])) {
        LOG_FATAL("question2 exceed max length %d\n", DMAX_SECURE_QUESTION_LENGTH - 1);
        return EECode_BadParam;
    }

    if (DUnlikely(0x0 != p_questions->answer1[DMAX_SECURE_ANSWER_LENGTH - 1])) {
        LOG_FATAL("answer1 exceed max length %d\n", DMAX_SECURE_ANSWER_LENGTH - 1);
        return EECode_BadParam;
    }

    if (DUnlikely(0x0 != p_questions->answer2[DMAX_SECURE_ANSWER_LENGTH - 1])) {
        LOG_FATAL("answer2 exceed max length %d\n", DMAX_SECURE_ANSWER_LENGTH - 1);
        return EECode_BadParam;
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;
    TU8 *p_cur = p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU32 length = strlen(password);
    TU16 cur_size = 0;

    //debug check
    if (DUnlikely(length > DIdentificationStringLength)) {
        LOG_FATAL("too long password %d, max %d\n", length, DIdentificationStringLength);
        releaseSendBuffer(send_buffer_struct);
        return EECode_BadParam;
    }

    CIConditionSlot *p_slot = allocMsgSlot();
    if (DUnlikely(!p_slot)) {
        LOG_FATAL("NULL p_slot\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_NoMemory;
    }

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    length = DDynamicInputStringLength;
    p_tlv->type_high = (ETLV16Type_ClientDynamicInput >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_ClientDynamicInput) & 0xff;
    p_tlv->length_high = ((length) >> 8) & 0xff;
    p_tlv->length_low = (length) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);
    TU8 tmp_buffer[64] = {0};
    gfGetCurrentDateTime((SDateTime *) tmp_buffer);
    tmp_buffer[sizeof(SDateTime)] = (tmp_buffer[7] + 0x6d) ^ (0x7c & tmp_buffer[0]);
    tmp_buffer[sizeof(SDateTime) + 1] = (tmp_buffer[6] ^ 0x34) ^ (0xd2 + tmp_buffer[1]);
    tmp_buffer[sizeof(SDateTime) + 2] = (tmp_buffer[5] & 0xac) ^ (0x94 ^ tmp_buffer[2]);
    tmp_buffer[sizeof(SDateTime) + 3] = (tmp_buffer[4] | 0x5a) ^ (0xcc & tmp_buffer[3]);
    TU64 dynamic_input = gfHashAlgorithmV7(tmp_buffer, sizeof(SDateTime) + 4);
    DBEW64(dynamic_input, tmp_buffer);
    memcpy(p_cur, tmp_buffer, DDynamicInputStringLength);
    p_cur += length;
    cur_size += length;

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    length = sizeof(TU32);
    p_tlv->type_high = (ETLV16Type_DynamicCode >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_DynamicCode) & 0xff;
    p_tlv->length_high = ((length) >> 8) & 0xff;
    p_tlv->length_low = (length) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);
    memset(tmp_buffer + DDynamicInputStringLength, 0x0, DIdentificationStringLength);
    memcpy(tmp_buffer + DDynamicInputStringLength, password, DIdentificationStringLength);
    TU32 dynamic_code = gfHashAlgorithmV8(tmp_buffer, DDynamicInputStringLength + DIdentificationStringLength);
    DBEW32(dynamic_code, p_cur);
    p_cur += length;
    cur_size += length;

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    length = strlen(p_questions->question1);
    p_tlv->type_high = (ETLV16Type_SecurityQuestion >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_SecurityQuestion) & 0xff;
    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
    p_tlv->length_low = (length + 1) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);
    memcpy(p_cur, p_questions->question1, length);
    p_cur += length;
    *p_cur++ = 0x0;
    cur_size += length + 1;

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    length = strlen(p_questions->answer1);
    p_tlv->type_high = (ETLV16Type_SecurityAnswer >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_SecurityAnswer) & 0xff;
    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
    p_tlv->length_low = (length + 1) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);
    memcpy(p_cur, p_questions->answer1, length);
    p_cur += length;
    *p_cur++ = 0x0;
    cur_size += length + 1;

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    length = strlen(p_questions->question2);
    p_tlv->type_high = (ETLV16Type_SecurityQuestion >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_SecurityQuestion) & 0xff;
    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
    p_tlv->length_low = (length + 1) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);
    memcpy(p_cur, p_questions->question2, length);
    p_cur += length;
    *p_cur++ = 0x0;
    cur_size += length + 1;

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    length = strlen(p_questions->answer2);
    p_tlv->type_high = (ETLV16Type_SecurityAnswer >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_SecurityAnswer) & 0xff;
    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
    p_tlv->length_low = (length + 1) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);
    memcpy(p_cur, p_questions->answer2, length);
    p_cur += length;
    *p_cur++ = 0x0;
    cur_size += length + 1;

    mpMutex->Lock();

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_UpdateSinkPassword);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);

    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    p_slot->Reset();
    p_slot->SetSessionNumber(mSeqCount ++);
    p_slot->SetCheckField((TU32)ESACPClientCmdSubType_UpdateSinkPassword);
    SDateTime time;
    gfGetCurrentDateTime(&time);
    p_slot->SetTime(&time);

    mpMsgSlotList->InsertContent(NULL, (void *) p_slot, 0);

    TInt payload_size = cur_size + sizeof(SSACPHeader) + sizeof(TUniqueID);
    TInt sentlength = gfNet_Send_timeout(mSocket, p_send_buffer, payload_size, 0, 2);

    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, peer socket is closed!\n");
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_Closed;
    } else if (DUnlikely(payload_size != sentlength)) {
        LOG_FATAL("send data fail, payload_size %d, sentlength %d\n", payload_size, sentlength);
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_NetSocketSend_Error;
    }

    EECode ret_code = EECode_OK;

    p_slot->Wait();
    p_slot->GetReplyCode(ret_code);
    LOG_PRINTF("update security question1/answer1 [%s, %s], question2/answer2 [%s, %s], ret %d, %s \n", p_questions->question1, p_questions->answer1, p_questions->question2, p_questions->answer2, ret_code, gfGetErrorCodeString(ret_code));
    mpMutex->Unlock();

    releaseSendBuffer(send_buffer_struct);
    releaseMsgSlot(p_slot);

    return ret_code;
}

EECode CSACPIMClientAgentV2::AcquireDynamicCode(TUniqueID device_id, TChar *password, TU32 &dynamic_code)
{
    DASSERT(password);
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;
    TU8 *p_cur = p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU16 cur_size = 0;

    CIConditionSlot *p_slot = allocMsgSlot();
    if (DUnlikely(!p_slot)) {
        LOG_FATAL("NULL p_slot\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_NoMemory;
    }

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_DeviceID >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_DeviceID) & 0xff;
    p_tlv->length_high = 0;
    p_tlv->length_low = sizeof(TUniqueID);
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);

    p_cur[0] = (device_id >> 56) & 0xff;
    p_cur[1] = (device_id >> 48) & 0xff;
    p_cur[2] = (device_id >> 40) & 0xff;
    p_cur[3] = (device_id >> 32) & 0xff;
    p_cur[4] = (device_id >> 24) & 0xff;
    p_cur[5] = (device_id >> 16) & 0xff;
    p_cur[6] = (device_id >> 8) & 0xff;
    p_cur[7] = (device_id) & 0xff;
    p_cur += sizeof(TUniqueID);
    cur_size += sizeof(TUniqueID);

    mpMutex->Lock();

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkAcquireDynamicCode);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    p_slot->Reset();
    p_slot->SetSessionNumber(mSeqCount ++);
    p_slot->SetCheckField((TU32)ESACPClientCmdSubType_SinkAcquireDynamicCode);
    SDateTime time;
    gfGetCurrentDateTime(&time);
    p_slot->SetTime(&time);

    mpMsgSlotList->InsertContent(NULL, (void *) p_slot, 0);

    TInt payload_size = cur_size + sizeof(SSACPHeader) + sizeof(TUniqueID);
    TInt sentlength = gfNet_Send_timeout(mSocket, p_send_buffer, payload_size, 0, 2);

    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, peer socket is closed!\n");
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_Closed;
    } else if (DUnlikely(payload_size != sentlength)) {
        LOG_FATAL("send data fail, payload_size %d, sentlength %d\n", payload_size, sentlength);
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_NetSocketSend_Error;
    }

    EECode ret_code = EECode_OK;
    TPointer reply_context = 0;
    TU32 reply_type = 0;
    TU32 reply_count = 0;
    TUniqueID id = 0;

    p_slot->Wait();
    p_slot->GetReplyCode(ret_code);
    p_slot->GetReplyContext(reply_context, reply_type, reply_count, id);

    mpMutex->Unlock();

    releaseSendBuffer(send_buffer_struct);
    releaseMsgSlot(p_slot);

    if (ret_code == EECode_OK) {
        DASSERT(reply_type == ETLV16Type_DynamicInput);
        TU8 *p_input = (TU8 *)reply_context;
        TU8 tmp_buffer[64] = {0};
        memcpy(tmp_buffer, p_input, DDynamicInputStringLength);
        memcpy(tmp_buffer + DDynamicInputStringLength, password, strlen(password));
        LOG_DEBUG("dynamic_input: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
                  tmp_buffer[0], tmp_buffer[1], tmp_buffer[2], tmp_buffer[3], tmp_buffer[4], tmp_buffer[5], tmp_buffer[6], tmp_buffer[7]);
        LOG_DEBUG("password %s\n", password);
        dynamic_code = gfHashAlgorithmV6(tmp_buffer, DDynamicInputStringLength + DIdentificationStringLength);
        LOG_DEBUG("CSACPIMClientAgentV2::AcquireDynamicCode success, dynamic_code %u\n", dynamic_code);
    }

    return ret_code;
}

EECode CSACPIMClientAgentV2::ReleaseDynamicCode(TUniqueID device_id)
{
    if (DUnlikely(!mbConnected)) {
        LOG_FATAL("Not Login!\n");
        return EECode_BadState;
    }

    SAgentSendBuffer *send_buffer_struct = allocSendBuffer(1024);
    if (DUnlikely(NULL == send_buffer_struct)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    TU8 *p_send_buffer = send_buffer_struct->p_send_buffer;
    TU8 *p_cur = p_send_buffer + sizeof(SSACPHeader) + sizeof(TUniqueID);
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU16 cur_size = 0;

    CIConditionSlot *p_slot = allocMsgSlot();
    if (DUnlikely(!p_slot)) {
        LOG_FATAL("NULL p_slot\n");
        releaseSendBuffer(send_buffer_struct);
        return EECode_NoMemory;
    }

    p_tlv = (STLV16HeaderBigEndian *) p_cur;
    p_tlv->type_high = (ETLV16Type_DeviceID >> 8) & 0xff;
    p_tlv->type_low = (ETLV16Type_DeviceID) & 0xff;
    p_tlv->length_high = 0;
    p_tlv->length_low = sizeof(TUniqueID);
    p_cur += sizeof(STLV16HeaderBigEndian);
    cur_size += sizeof(STLV16HeaderBigEndian);

    p_cur[0] = (device_id >> 56) & 0xff;
    p_cur[1] = (device_id >> 48) & 0xff;
    p_cur[2] = (device_id >> 40) & 0xff;
    p_cur[3] = (device_id >> 32) & 0xff;
    p_cur[4] = (device_id >> 24) & 0xff;
    p_cur[5] = (device_id >> 16) & 0xff;
    p_cur[6] = (device_id >> 8) & 0xff;
    p_cur[7] = (device_id) & 0xff;
    p_cur += sizeof(TUniqueID);
    cur_size += sizeof(TUniqueID);

    mpMutex->Lock();

    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ClientCmdChannel, ESACPClientCmdSubType_SinkReleaseDynamicCode);
    gfSACPFillHeader((SSACPHeader *) p_send_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_IM, sizeof(TUniqueID), mSeqCount, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);
    memset(p_send_buffer + sizeof(SSACPHeader), 0x0, sizeof(TUniqueID));

    p_slot->Reset();
    p_slot->SetSessionNumber(mSeqCount ++);
    p_slot->SetCheckField((TU32)ESACPClientCmdSubType_SinkReleaseDynamicCode);
    SDateTime time;
    gfGetCurrentDateTime(&time);
    p_slot->SetTime(&time);

    mpMsgSlotList->InsertContent(NULL, (void *) p_slot, 0);

    TInt payload_size = cur_size + sizeof(SSACPHeader) + sizeof(TUniqueID);
    TInt sentlength = gfNet_Send_timeout(mSocket, p_send_buffer, payload_size, 0, 2);

    if (DUnlikely(sentlength == 0)) {
        LOG_FATAL("send data fail, peer socket is closed!\n");
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_Closed;
    } else if (DUnlikely(payload_size != sentlength)) {
        LOG_FATAL("send data fail, payload_size %d, sentlength %d\n", payload_size, sentlength);
        mpMutex->Unlock();
        releaseSendBuffer(send_buffer_struct);
        releaseMsgSlot(p_slot);
        return EECode_NetSocketSend_Error;
    }

    EECode ret_code = EECode_OK;
    p_slot->Wait();
    p_slot->GetReplyCode(ret_code);

    mpMutex->Unlock();

    releaseSendBuffer(send_buffer_struct);
    releaseMsgSlot(p_slot);

    return ret_code;
}

void CSACPIMClientAgentV2::SetCallBack(void *owner, TReceiveMessageCallBack callback, TSystemNotifyCallBack notify_callback, TErrorCallBack error_callback)
{
    mpCallbackOwner = owner;
    mpMessageCallback = callback;
    mpSystemNotifyCallback = notify_callback;
    mpErrorCallback = error_callback;
}

void CSACPIMClientAgentV2::Destroy()
{
    delete this;
}

EECode CSACPIMClientAgentV2::startReadLoop()
{
    if (mbStartReadLoop) {
        LOG_WARN("already started\n");
        return EECode_BadState;
    }

    mbStartReadLoop = 1;
    mpThread = gfCreateThread("CSACPIMClientAgentV2", __Entry, (void *)(this));

    return EECode_OK;
}

void CSACPIMClientAgentV2::stopReadLoop()
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

EECode CSACPIMClientAgentV2::__Entry(void *param)
{
    CSACPIMClientAgentV2 *thiz = (CSACPIMClientAgentV2 *)param;
    thiz->ReadLoop();

    //LOG_NOTICE("after ReadLoop\n");
    return EECode_OK;
}

EECode CSACPIMClientAgentV2::ReadLoop()
{
    if (DUnlikely((!DIsSocketHandlerValid(mPipeFd[0])) || (!DIsSocketHandlerValid(mSocket)))) {
        LOG_FATAL("not valid pipefd %d, or socket %d\n", mPipeFd[0], mSocket);
        return EECode_BadState;
    }

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

    TU32 payload_size = 0, cat = 0, sub_type = 0, session_number = 0, reqres_bits = 0, need_reply = 0;
    TInt nready = 0;
    TInt recvlength = 0;
    TU8 *p_payload = NULL;
    TU8 *p_source_id = mpReadBuffer + sizeof(SSACPHeader);

    TChar char_buffer = 0;
    for (;;) {
        mReadSet = mAllSet;
        //LOG_NOTICE("before select\n");
        nready = select(mMaxFd + 1, &mReadSet, NULL, NULL, NULL);
        //LOG_NOTICE("after select, %d\n", nready);

        if (FD_ISSET(mPipeFd[0], &mReadSet)) {
            nready --;
            gfOSReadPipe(mPipeFd[0], char_buffer);
            if ('q' == char_buffer) {
                LOG_NOTICE("quit\n");
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

            recvlength = gfNet_Recv_timeout(mSocket, mpReadBuffer, (TInt)mnHeaderLength, DNETUTILS_RECEIVE_FLAG_READ_ALL, 6);
            //LOG_NOTICE("[recieve header]: recvlength %d\n", recvlength);
            if (DUnlikely(recvlength == 0)) {
                LOG_ERROR("recv data fail, peer socket is closed\n");
                if (DLikely(mpCallbackOwner && mpErrorCallback)) {
                    mpErrorCallback(mpCallbackOwner, EECode_Closed);
                }
                gfNetCloseSocket(mSocket);
                mSocket = DInvalidSocketHandler;
                return EECode_Closed;
            } else if (DUnlikely(recvlength != (TInt)mnHeaderLength)) {
                LOG_FATAL("recv header data fail, recvlength %d, mnHeaderLength %d\n", recvlength, mnHeaderLength);
                if (DLikely(mpCallbackOwner && mpErrorCallback)) {
                    mpErrorCallback(mpCallbackOwner, EECode_NetReceiveHeader_Fail);
                }
                gfNetCloseSocket(mSocket);
                mSocket = DInvalidSocketHandler;
                return EECode_NetReceiveHeader_Fail;
            }

            payload_size = ((TU32)mpSACPHeader->size_0 << 16) | ((TU32)mpSACPHeader->size_1 << 8) | ((TU32)mpSACPHeader->size_2);
            //hard code here
            cat = mpSACPHeader->type_2;
            sub_type = ((TU32)mpSACPHeader->type_3 << 8) | ((TU32)mpSACPHeader->type_4);
            session_number = ((TU32)mpSACPHeader->seq_count_0 << 16) | ((TU32)mpSACPHeader->seq_count_1 << 8) | ((TU32)mpSACPHeader->seq_count_2);
            reqres_bits = (TU32)mpSACPHeader->type_1 << 24;
            if ((reqres_bits & DSACPTypeBit_Request) && (reqres_bits & DSACPTypeBit_NeedReply)) {
                need_reply = 1;
            } else {
                need_reply = 0;
            }

            if (0 == payload_size) {
                if (ESACPTypeCategory_ClientCmdChannel == cat) {
                    handleServerCmd(need_reply, sub_type, session_number, NULL, 0, (ESACPConnectResult) mpSACPHeader->flags);
                } else if (ESACPTypeCategory_SystemNotification == cat) {
                    handleSystemNotification(sub_type, NULL, 0);
                } else {
                    LOG_FATAL("bad cat %d\n", cat);
                }
                continue;
            }

            if (DUnlikely((payload_size + mnHeaderLength) >= mReadBufferSize)) {
                TU8 *ptmp = (TU8 *) DDBG_MALLOC(payload_size + mnHeaderLength + 32, "ICAP");
                if (DUnlikely(!ptmp)) {
                    LOG_FATAL("not enough memory\n");
                    return EECode_NoMemory;
                }
                mReadBufferSize = payload_size + mnHeaderLength + 32;
                memcpy(ptmp, mpReadBuffer, mnHeaderLength);
                DDBG_FREE(mpReadBuffer, "ICAP");
                mpReadBuffer = ptmp;
                mpSACPHeader = (SSACPHeader *)mpReadBuffer;
                p_source_id = mpReadBuffer + sizeof(SSACPHeader);
            }
            p_payload = mpReadBuffer + mnHeaderLength;

            recvlength = gfNet_Recv_timeout(mSocket, p_payload, (TInt)payload_size, DNETUTILS_RECEIVE_FLAG_READ_ALL, 6);
            LOG_NOTICE("[recieve payload]: payload_size %d, recvlength %d, cat %x\n", payload_size, recvlength, cat);
            if (DUnlikely(recvlength == 0)) {
                LOG_FATAL("recv data fail, peer socket is closed\n");
                if (DUnlikely(mpCallbackOwner && mpErrorCallback)) {
                    mpErrorCallback(mpCallbackOwner, EECode_Closed);
                }
                gfNetCloseSocket(mSocket);
                mSocket = DInvalidSocketHandler;
                return EECode_Closed;
            } else if (DUnlikely(recvlength != (TInt)payload_size)) {
                LOG_FATAL("recv payload data fail\n");
                if (DUnlikely(mpCallbackOwner && mpErrorCallback)) {
                    mpErrorCallback(mpCallbackOwner, EECode_NetReceivePayload_Fail);
                }
                gfNetCloseSocket(mSocket);
                mSocket = DInvalidSocketHandler;
                return EECode_NetReceivePayload_Fail;
            }

            if (ESACPTypeCategory_ClientRelayChannel == cat) {
                TUniqueID source_id = 0;
                DBER64(source_id, p_source_id);
                //LOG_NOTICE("[DEBUG]1: relay msg, %llx, size %d\n", source_id, payload_size + mnHeaderLength);
                //gfPrintMemory(mpReadBuffer, payload_size + mnHeaderLength);
                //LOG_NOTICE("[DEBUG]2: relay msg, size %d\n", payload_size);
                //gfPrintMemory(p_payload, payload_size);
                //LOG_NOTICE("[DEBUG]: receive relay msg\n");
                mpMessageCallback(mpCallbackOwner, source_id, p_payload, payload_size, need_reply);
            } else if (ESACPTypeCategory_ClientCmdChannel == cat) {
                //LOG_NOTICE("[DEBUG]: from cmd channel?\n");
                handleServerCmd(need_reply, sub_type, session_number, p_payload, payload_size, (ESACPConnectResult) mpSACPHeader->flags);
            } else if (ESACPTypeCategory_SystemNotification == cat) {
                //LOG_NOTICE("[DEBUG]: system notification?\n");
                handleSystemNotification(sub_type, p_payload, payload_size);
            } else {
                LOG_FATAL("bad cat %d\n", cat);
            }

        } else {
            LOG_FATAL("why comes here?\n");
            return EECode_InternalLogicalBug;
        }
    }

    return EECode_OK;
}

void CSACPIMClientAgentV2::handleServerCmd(TU32 need_reply, TU32 sub_type, TU32 session_number, TU8 *payload, TU32 payload_size, ESACPConnectResult ret_flag)
{
    STLV16HeaderBigEndian *p_tlv = (STLV16HeaderBigEndian *)payload;
    TU16 tlv_length = 0;
    TU16 tlv_type = 0;
    TUniqueID id = DInvalidUniqueID;
    CIConditionSlot *p_slot = NULL;
    EECode err = EECode_OK;
    SAgentIDListBuffer *id_list = NULL;
    TU32 count = 0;

    LOG_NOTICE("[DEBUG]: handleServerCmd(%x, %s), payload_size %d\n", sub_type, gfGetSACPClientCmdSubtypeString((ESACPClientCmdSubType)sub_type), payload_size);
    //gfPrintMemory(payload, payload_size);

    switch (sub_type) {

        case ESACPClientCmdSubType_SinkOwnSourceDevice:
            if (ESACPConnectResult_OK == ret_flag) {
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                DASSERT((sizeof(TUniqueID) + sizeof(STLV16HeaderBigEndian)) == payload_size);
                if (DLikely((ETLV16Type_AccountID == tlv_type) && (sizeof(TUniqueID) == tlv_length))) {
                    payload += sizeof(STLV16HeaderBigEndian);
                    DBER64(id, payload);
                    LOG_NOTICE("own id %llx\n", id);
                } else {
                    LOG_FATAL("corrupted protocol!\n");
                    err = EECode_ServerReject_CorruptedProtocol;
                }
            } else {
                err = gfSACPConnectResultToErrorCode(ret_flag);
            }

            mpMutex->Lock();
            p_slot = matchedMsgSlot(session_number, sub_type);
            if (DUnlikely(!p_slot)) {
                LOG_ERROR("no slot\n");
                mpMutex->Unlock();
                return;
            }
            p_slot->SetReplyCode(err);
            p_slot->SetReplyContext(0, 0, 0, id);
            p_slot->Signal();
            mpMutex->Unlock();
            break;

        case ESACPClientCmdSubType_SinkInviteFriend:
            if ((ESACPConnectResult_OK != ret_flag) && (ESACPRequestResult_AlreadyExist != ret_flag)) {
                err = gfSACPConnectResultToErrorCode(ret_flag);
            } else {
                if (payload) {
                    p_tlv = (STLV16HeaderBigEndian *)payload;
                    tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                    tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                    if (DLikely((sizeof(TUniqueID) == tlv_length) && (ETLV16Type_AccountID == tlv_type))) {
                        TU8 *p_cur = payload + sizeof(STLV16HeaderBigEndian);
                        DBER64(id, p_cur);
                    }
                } else {
                    id = DInvalidUniqueID;
                    err = EECode_InternalLogicalBug;
                    LOG_FATAL("check me here, in ESACPClientCmdSubType_SinkInviteFriend\n");
                }
            }

            mpMutex->Lock();
            p_slot = matchedMsgSlot(session_number, sub_type);
            if (DUnlikely(!p_slot)) {
                LOG_ERROR("no slot\n");
                mpMutex->Unlock();
                return;
            }
            p_slot->SetReplyCode(err);
            p_slot->SetReplyContext(0, 0, 0, id);
            p_slot->Signal();
            mpMutex->Unlock();
            break;

        case ESACPClientCmdSubType_SinkInviteFriendByID:
            if (ESACPConnectResult_OK != ret_flag) {
                err = gfSACPConnectResultToErrorCode(ret_flag);
            } else {
                p_tlv = (STLV16HeaderBigEndian *)payload;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);

                if (DLikely((DMAX_ACCOUNT_NAME_LENGTH_EX > tlv_length) && (ETLV16Type_AccountName == tlv_type))) {
                    TU8 *p_cur = payload + sizeof(STLV16HeaderBigEndian);
                    TPointer reply_context = (TPointer)NULL;
                    TU32 reply_type;
                    TU32 reply_count;
                    TUniqueID reply_id;
                    p_slot->GetReplyContext(reply_context, reply_type, reply_count, reply_id);
                    memcpy((void *)reply_context, p_cur, tlv_length);
                }
            }

            mpMutex->Lock();
            p_slot = matchedMsgSlot(session_number, sub_type);
            if (DUnlikely(!p_slot)) {
                LOG_ERROR("no slot\n");
                mpMutex->Unlock();
                return;
            }
            p_slot->SetReplyCode(err);
            p_slot->Signal();
            mpMutex->Unlock();
            break;

        case ESACPClientCmdSubType_SinkDonateSourceDevice:
        case ESACPClientCmdSubType_SinkUpdateFriendPrivilege:
        case ESACPClientCmdSubType_SinkRemoveFriend:
            DASSERT(!payload);
            DASSERT(!payload_size);
            if (ESACPConnectResult_OK != ret_flag) {
                err = gfSACPConnectResultToErrorCode(ret_flag);
            }

            mpMutex->Lock();
            p_slot = matchedMsgSlot(session_number, sub_type);
            if (DUnlikely(!p_slot)) {
                LOG_ERROR("no slot\n");
                mpMutex->Unlock();
                return;
            }
            p_slot->SetReplyCode(err);
            p_slot->Signal();
            mpMutex->Unlock();
            break;

        case ESACPClientCmdSubType_SinkRetrieveFriendsList:
            if (DLikely(ESACPConnectResult_OK == ret_flag)) {
                TU32 index = 0;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                DASSERT((tlv_length + sizeof(STLV16HeaderBigEndian)) == payload_size);
                if (DLikely(ETLV16Type_UserFriendList == tlv_type)) {
                    count = tlv_length / sizeof(TUniqueID);
                    payload += sizeof(STLV16HeaderBigEndian);
                    LOG_NOTICE("friend number %d\n", count);
                    if (count) {
                        id_list = allocIDListBuffer(count);
                        DASSERT(id_list);
                        id_list->id_count = count;
                        for (index = 0; index < count; index ++) {
                            DBER64(id_list->p_id_list[index], payload);
                            LOG_NOTICE("friend id[%d] %llx\n", index, id_list->p_id_list[index]);
                            payload += sizeof(TUniqueID);
                        }
                    }
                } else {
                    LOG_FATAL("corrupted protocol!\n");
                    err = EECode_ServerReject_CorruptedProtocol;
                }
            } else {
                err = gfSACPConnectResultToErrorCode(ret_flag);
            }

            mpMutex->Lock();
            p_slot = matchedMsgSlot(session_number, sub_type);
            if (DUnlikely(!p_slot)) {
                LOG_ERROR("no slot\n");
                mpMutex->Unlock();
                return;
            }
            p_slot->SetReplyCode(err);
            p_slot->SetReplyContext((TPointer)id_list, ETLV16Type_UserFriendList, count, DInvalidUniqueID);
            p_slot->Signal();
            mpMutex->Unlock();
            break;

        case ESACPClientCmdSubType_SinkRetrieveAdminDeviceList:
            LOG_NOTICE("[DEBUG]: print admin device list %d\n", payload_size);
            //gfPrintMemory(payload, payload_size);
            if (DLikely(ESACPConnectResult_OK == ret_flag)) {
                TU32 index = 0;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely((tlv_length + sizeof(STLV16HeaderBigEndian)) != payload_size)) {
                    LOG_NOTICE("tlv_length %d, payload_size %d\n", tlv_length, payload_size);
                }
                if (DLikely(ETLV16Type_UserOwnedDeviceList == tlv_type)) {
                    count = tlv_length / sizeof(TUniqueID);
                    payload += sizeof(STLV16HeaderBigEndian);
                    LOG_NOTICE("own device number %d\n", count);
                    if (count) {
                        id_list = allocIDListBuffer(count);
                        DASSERT(id_list);
                        id_list->id_count = count;
                        for (index = 0; index < count; index ++) {
                            DBER64(id_list->p_id_list[index], payload);
                            LOG_NOTICE("admin device id[%d] %llx\n", index, id_list->p_id_list[index]);
                            payload += sizeof(TUniqueID);
                        }
                    }
                } else {
                    LOG_FATAL("corrupted protocol!, %x\n", ETLV16Type_UserOwnedDeviceList);
                    err = EECode_ServerReject_CorruptedProtocol;
                }
            } else {
                err = gfSACPConnectResultToErrorCode(ret_flag);
            }

            mpMutex->Lock();
            p_slot = matchedMsgSlot(session_number, sub_type);
            if (DUnlikely(!p_slot)) {
                LOG_ERROR("no slot\n");
                mpMutex->Unlock();
                return;
            }
            p_slot->SetReplyCode(err);
            p_slot->SetReplyContext((TPointer)id_list, ETLV16Type_UserOwnedDeviceList, count, DInvalidUniqueID);
            p_slot->Signal();
            mpMutex->Unlock();
            break;

        case ESACPClientCmdSubType_SinkRetrieveSharedDeviceList:
            if (DLikely(ESACPConnectResult_OK == ret_flag)) {
                TU32 index = 0;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                DASSERT((tlv_length + sizeof(STLV16HeaderBigEndian)) == payload_size);
                if (DLikely(ETLV16Type_UserSharedDeviceList == tlv_type)) {
                    count = tlv_length / sizeof(TUniqueID);
                    payload += sizeof(STLV16HeaderBigEndian);
                    LOG_NOTICE("own device number %d\n", count);
                    if (count) {
                        id_list = allocIDListBuffer(count);
                        DASSERT(id_list);
                        id_list->id_count = count;
                        for (index = 0; index < count; index ++) {
                            DBER64(id_list->p_id_list[index], payload);
                            LOG_NOTICE("shared device id[%d] %llx\n", index, id_list->p_id_list[index]);
                            payload += sizeof(TUniqueID);
                        }
                    }
                } else {
                    LOG_FATAL("corrupted protocol!\n");
                    err = EECode_ServerReject_CorruptedProtocol;
                }
            } else {
                err = gfSACPConnectResultToErrorCode(ret_flag);
            }

            mpMutex->Lock();
            p_slot = matchedMsgSlot(session_number, sub_type);
            if (DUnlikely(!p_slot)) {
                LOG_ERROR("no slot\n");
                mpMutex->Unlock();
                return;
            }
            p_slot->SetReplyCode(err);
            p_slot->SetReplyContext((TPointer)id_list, ETLV16Type_UserSharedDeviceList, count, DInvalidUniqueID);
            p_slot->Signal();
            mpMutex->Unlock();
            break;

        case ESACPClientCmdSubType_SinkQuerySource:
            if (DLikely(ESACPConnectResult_OK == ret_flag)) {
                mpMutex->Lock();
                p_slot = matchedMsgSlot(session_number, sub_type);
                if (DUnlikely(!p_slot)) {
                    LOG_ERROR("no slot\n");
                    mpMutex->Unlock();
                    return;
                }

                TPointer reply_context;
                TU32 reply_type, reply_count;
                TUniqueID reply_id;
                p_slot->GetReplyContext(reply_context, reply_type, reply_count, reply_id);
                if (DUnlikely(!reply_context)) {
                    LOG_FATAL("no reply_context\n");
                    err = EECode_InternalLogicalBug;
                    p_slot->SetReplyCode(err);
                    p_slot->Signal();
                    mpMutex->Unlock();
                    return;
                }

                SDeviceProfile *p_profile = (SDeviceProfile *) reply_context;

                TU8 *p_cur = payload;
                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely(ETLV16Type_SourceDeviceDataServerAddress != tlv_type)) {
                    LOG_FATAL("no server url\n");
                    err = EECode_ServerReject_CorruptedProtocol;
                    p_slot->SetReplyCode(err);
                    p_slot->Signal();
                    mpMutex->Unlock();
                    return;
                }
                p_cur += sizeof(STLV16HeaderBigEndian);
                DASSERT(DMaxServerUrlLength >= tlv_length);
                memcpy(p_profile->dataserver_address, p_cur, tlv_length);
                p_cur += tlv_length;

                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely(ETLV16Type_SourceDeviceStreamingTag != tlv_type)) {
                    LOG_FATAL("no streaming tag\n");
                    err = EECode_ServerReject_CorruptedProtocol;
                    p_slot->SetReplyCode(err);
                    p_slot->Signal();
                    mpMutex->Unlock();
                    return;
                }
                p_cur += sizeof(STLV16HeaderBigEndian);
                DASSERT(DMAX_ACCOUNT_NAME_LENGTH_EX >= tlv_length);
                memcpy(p_profile->name, p_cur, tlv_length);
                p_cur += tlv_length;

                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely(ETLV16Type_SourceDeviceStreamingPort != tlv_type)) {
                    LOG_FATAL("no streaming port\n");
                    err = EECode_ServerReject_CorruptedProtocol;
                    p_slot->SetReplyCode(err);
                    p_slot->Signal();
                    mpMutex->Unlock();
                    return;
                }
                p_cur += sizeof(STLV16HeaderBigEndian);
                DASSERT(2 == tlv_length);
                DBER16(p_profile->rtsp_streaming_port, p_cur);
                p_cur += tlv_length;

                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely(ETLV16Type_DynamicInput != tlv_type)) {
                    LOG_FATAL("no dynamic input\n");
                    err = EECode_ServerReject_CorruptedProtocol;
                    p_slot->SetReplyCode(err);
                    p_slot->Signal();
                    mpMutex->Unlock();
                    return;
                }
                p_cur += sizeof(STLV16HeaderBigEndian);
                DASSERT(DDynamicInputStringLength == tlv_length);
                memcpy(p_profile->dynamic_code, p_cur, DDynamicInputStringLength);
                p_cur += tlv_length;

                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely(ETLV16Type_DeviceNickName != tlv_type)) {
                    LOG_FATAL("no nick name\n");
                    err = EECode_ServerReject_CorruptedProtocol;
                    p_slot->SetReplyCode(err);
                    p_slot->Signal();
                    mpMutex->Unlock();
                    return;
                }
                p_cur += sizeof(STLV16HeaderBigEndian);
                DASSERT(DMAX_ACCOUNT_NAME_LENGTH_EX >= tlv_length);
                memcpy(p_profile->nickname, p_cur, tlv_length);
                p_cur += tlv_length;

                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely(ETLV16Type_DeviceStatus != tlv_type)) {
                    LOG_FATAL("no device status\n");
                    err = EECode_ServerReject_CorruptedProtocol;
                    p_slot->SetReplyCode(err);
                    p_slot->Signal();
                    mpMutex->Unlock();
                    return;
                }
                p_cur += sizeof(STLV16HeaderBigEndian);
                DASSERT(1 == tlv_length);
                p_profile->status = p_cur[0];
                p_cur += tlv_length;

                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely(ETLV16Type_DeviceStorageCapacity != tlv_type)) {
                    LOG_FATAL("no storage capability\n");
                    err = EECode_ServerReject_CorruptedProtocol;
                    p_slot->SetReplyCode(err);
                    p_slot->Signal();
                    mpMutex->Unlock();
                    return;
                }
                p_cur += sizeof(STLV16HeaderBigEndian);
                DASSERT(4 == tlv_length);
                DBER32(p_profile->storage_capacity, p_cur);
                p_cur += tlv_length;
                p_slot->SetReplyCode(EECode_OK);
                p_slot->Signal();
                mpMutex->Unlock();
            } else {
                err = gfSACPConnectResultToErrorCode(ret_flag);
                mpMutex->Lock();
                p_slot = matchedMsgSlot(session_number, sub_type);
                if (DUnlikely(!p_slot)) {
                    LOG_ERROR("no slot\n");
                    mpMutex->Unlock();
                    return;
                }
                p_slot->SetReplyCode(err);
                p_slot->Signal();
                mpMutex->Unlock();
            }
            break;

        case ESACPClientCmdSubType_UpdateSinkNickname:
        case ESACPClientCmdSubType_UpdateSourceNickname:
        case ESACPClientCmdSubType_UpdateSinkPassword:
        case ESACPClientCmdSubType_SourceSetOwner:
            DASSERT(!payload);
            DASSERT(!payload_size);
            if (ESACPConnectResult_OK != ret_flag) {
                err = gfSACPConnectResultToErrorCode(ret_flag);
            }

            mpMutex->Lock();
            p_slot = matchedMsgSlot(session_number, sub_type);
            if (DUnlikely(!p_slot)) {
                LOG_ERROR("no slot\n");
                mpMutex->Unlock();
                return;
            }
            p_slot->SetReplyCode(err);
            p_slot->Signal();
            mpMutex->Unlock();
            break;

        case ESACPClientCmdSubType_SinkAcquireDynamicCode:
            if (ESACPConnectResult_OK == ret_flag) {
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                DASSERT((sizeof(TU64) + sizeof(STLV16HeaderBigEndian)) == payload_size);
                if (DUnlikely((ETLV16Type_DynamicInput != tlv_type) || (sizeof(TU64) != tlv_length))) {
                    LOG_FATAL("corrupted protocol!\n");
                    err = EECode_ServerReject_CorruptedProtocol;
                } else {
                    payload += sizeof(STLV16HeaderBigEndian);
                }
            } else {
                err = gfSACPConnectResultToErrorCode(ret_flag);
            }

            mpMutex->Lock();
            p_slot = matchedMsgSlot(session_number, sub_type);
            if (DUnlikely(!p_slot)) {
                LOG_ERROR("no slot\n");
                mpMutex->Unlock();
                return;
            }
            p_slot->SetReplyCode(err);
            p_slot->SetReplyContext((TPointer)payload, ETLV16Type_DynamicInput, 1, 0);
            p_slot->Signal();
            mpMutex->Unlock();
            break;

        case ESACPClientCmdSubType_SinkQueryFriendInfo:
            if (DLikely(ESACPConnectResult_OK == ret_flag)) {
                mpMutex->Lock();
                p_slot = matchedMsgSlot(session_number, sub_type);
                if (DUnlikely(!p_slot)) {
                    LOG_ERROR("no slot\n");
                    mpMutex->Unlock();
                    return;
                }

                TPointer reply_context;
                TU32 reply_type, reply_count;
                TUniqueID reply_id;
                p_slot->GetReplyContext(reply_context, reply_type, reply_count, reply_id);
                if (DUnlikely(!reply_context)) {
                    LOG_FATAL("no reply_context\n");
                    err = EECode_InternalLogicalBug;
                    p_slot->SetReplyCode(err);
                    p_slot->Signal();
                    mpMutex->Unlock();
                    return;
                }

                SFriendInfo *info = (SFriendInfo *)reply_context;

                TU8 *p_cur = payload;
                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely((ETLV16Type_UserOnline != tlv_type) || (1 != tlv_length))) {
                    LOG_FATAL("not online?\n");
                    err = EECode_ServerReject_CorruptedProtocol;
                    p_slot->SetReplyCode(err);
                    p_slot->Signal();
                    mpMutex->Unlock();
                    return;
                }
                p_cur += sizeof(STLV16HeaderBigEndian);
                info->online = p_cur[0];

                p_slot->SetReplyCode(EECode_OK);
                p_slot->Signal();
                mpMutex->Unlock();
            } else {
                err = gfSACPConnectResultToErrorCode(ret_flag);
                mpMutex->Lock();
                p_slot = matchedMsgSlot(session_number, sub_type);
                if (DUnlikely(!p_slot)) {
                    LOG_ERROR("no slot\n");
                    mpMutex->Unlock();
                    return;
                }
                p_slot->SetReplyCode(err);
                p_slot->Signal();
                mpMutex->Unlock();
            }
            break;

        case ESACPClientCmdSubType_SinkAcceptFriend:
            err = gfSACPConnectResultToErrorCode(ret_flag);
            mpMutex->Lock();
            p_slot = matchedMsgSlot(session_number, sub_type);
            if (DUnlikely(!p_slot)) {
                LOG_ERROR("no slot\n");
                mpMutex->Unlock();
                return;
            }

            p_slot->SetReplyCode(EECode_OK);
            p_slot->Signal();
            mpMutex->Unlock();
            break;

        default:
            LOG_FATAL("not expected sub_type %x\n", sub_type);
            break;
    }

}

void CSACPIMClientAgentV2::handleSystemNotification(TU32 sub_type, TU8 *payload, TU32 payload_size)
{
    STLV16HeaderBigEndian *p_tlv = (STLV16HeaderBigEndian *)payload;
    TU16 tlv_length = 0;
    TU16 tlv_type = 0;

    LOG_NOTICE("[DEBUG]: handleSystemNotification(%x, %s), payload_size %d\n", sub_type, gfGetSACPClientCmdSubtypeString((ESACPClientCmdSubType)sub_type), payload_size);
    //gfPrintMemory(payload, payload_size);

    switch (sub_type) {

        case ESACPClientCmdSubType_SystemNotifyFriendInvitation: {
                SSystemNotifyIMFriendInvitation c;
                TU8 *p_cur = payload;
                memset(&c, 0x0, sizeof(c));

                //LOG_NOTICE("payload_size %d\n", payload_size);
                //gfPrintMemory(payload, payload_size);

                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DLikely((ETLV16Type_AccountID == tlv_type) && (sizeof(TUniqueID) == tlv_length))) {
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    DBER64(c.invitor_id, p_cur);
                } else {
                    LOG_FATAL("corrupted protocol!\n");
                    break;
                }

                p_cur = payload + tlv_length + sizeof(STLV16HeaderBigEndian);

                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DLikely((ETLV16Type_AccountName == tlv_type) && (DMAX_ACCOUNT_NAME_LENGTH_EX >= tlv_length))) {
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    memcpy(c.invitor_name, p_cur, tlv_length);
                } else {
                    LOG_FATAL("corrupted protocol!, tlv_type %x, tlv_length %d\n", tlv_type, tlv_length);

                    break;
                }

                LOG_NOTICE("invitor id %llx, name %s\n", c.invitor_id, c.invitor_name);
                if (mpCallbackOwner && mpSystemNotifyCallback) {
                    mpSystemNotifyCallback(mpCallbackOwner, ESystemNotify_IM_FriendInvitation, (void *)&c);
                }
            }
            break;

        case ESACPClientCmdSubType_SystemNotifyDeviceDonation:
            if (mpCallbackOwner && mpSystemNotifyCallback) {
                mpSystemNotifyCallback(mpCallbackOwner, ESystemNotify_IM_DeviceDonation, NULL);
            }
            break;

        case ESACPClientCmdSubType_SystemNotifyDeviceSharing:
            tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
            tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
            DASSERT((sizeof(TUniqueID) + sizeof(STLV16HeaderBigEndian)) == payload_size);
            if (DLikely((ETLV16Type_AccountID == tlv_type) && (sizeof(TUniqueID) == tlv_length))) {
                payload += sizeof(STLV16HeaderBigEndian);
            } else {
                LOG_FATAL("corrupted protocol!\n");
                break;
            }
            if (mpCallbackOwner && mpSystemNotifyCallback) {
                mpSystemNotifyCallback(mpCallbackOwner, ESystemNotify_IM_DeviceSharing, NULL);
            }
            break;

        case ESACPClientCmdSubType_SystemNotifyUpdateOwnedDeviceList:
            if (mpCallbackOwner && mpSystemNotifyCallback) {
                mpSystemNotifyCallback(mpCallbackOwner, ESystemNotify_IM_UpdateOwnedDeviceList, NULL);
            }
            break;

        case ESACPClientCmdSubType_SystemNotifyDeviceOwnedByUser:
            if (mpCallbackOwner && mpSystemNotifyCallback) {
                mpSystemNotifyCallback(mpCallbackOwner, ESystemNotify_IM_DeviceOwnedByUser, NULL);
            }
            break;

        case ESACPClientCmdSubType_SystemNotifyTargetOffline:
            if (mpCallbackOwner && mpSystemNotifyCallback) {
                SSystemNotifyIMTargetOffline c;
                TU8 *p_cur = payload;

                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DLikely((ETLV16Type_AccountID == tlv_type) && (sizeof(TUniqueID) == tlv_length))) {
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    DBER64(c.target, p_cur);
                } else {
                    LOG_FATAL("corrupted protocol!\n");
                    break;
                }
                mpSystemNotifyCallback(mpCallbackOwner, ESystemNotify_IM_UserOffline, &c);
            }
            break;

        case ESACPClientCmdSubType_SystemNotifyTargetNotReachable:
            if (mpCallbackOwner && mpSystemNotifyCallback) {
                SSystemNotifyIMTargetNotReachable c;
                TU8 *p_cur = payload;

                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DLikely((ETLV16Type_AccountID == tlv_type) && (sizeof(TUniqueID) == tlv_length))) {
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    DBER64(c.target, p_cur);
                } else {
                    LOG_FATAL("corrupted protocol!\n");
                    break;
                }
                mpSystemNotifyCallback(mpCallbackOwner, ESystemNotify_IM_TargetNotReachable, &c);
            }
            break;

        default:
            LOG_FATAL("not expected sub_type %x\n", sub_type);
            break;
    }

}

CIConditionSlot *CSACPIMClientAgentV2::allocMsgSlot()
{
    AUTO_LOCK(mpResourceMutex);
    CIConditionSlot *slot = NULL;

    CIDoubleLinkedList::SNode *p_node = mpFreeMsgSlotList->FirstNode();
    if (p_node) {
        slot = (CIConditionSlot *) p_node->p_context;
        if (DLikely(slot)) {
            mpFreeMsgSlotList->RemoveContent((void *) slot);
            return slot;
        } else {
            LOG_FATAL("internal logic bug\n");
            return NULL;
        }
    } else {
        slot = CIConditionSlot::Create(mpMutex);
        if (DUnlikely(!slot)) {
            LOG_FATAL("CIConditionSlot::Create() fail\n");
            return NULL;
        }
    }

    return slot;
}

void CSACPIMClientAgentV2::releaseMsgSlot(CIConditionSlot *slot)
{
    AUTO_LOCK(mpResourceMutex);
    mpMsgSlotList->RemoveContent((void *) slot);
    mpFreeMsgSlotList->InsertContent(NULL, (void *) slot, 0);
}

SAgentSendBuffer *CSACPIMClientAgentV2::allocSendBuffer(TU32 max_buffer_size)
{
    AUTO_LOCK(mpResourceMutex);
    SAgentSendBuffer *buffer = NULL;

    CIDoubleLinkedList::SNode *p_node = mpSendBufferList->FirstNode();
    if (p_node) {
        buffer = (SAgentSendBuffer *) p_node->p_context;
        if (DLikely(buffer)) {
            if (buffer->send_buffer_size < max_buffer_size) {
                if (buffer->p_send_buffer) {
                    free(buffer->p_send_buffer);
                    buffer->p_send_buffer = NULL;
                }
                buffer->send_buffer_size = 0;
            }
            if (!buffer->p_send_buffer) {
                buffer->p_send_buffer = (TU8 *) DDBG_MALLOC(max_buffer_size, "ICAS");
                if (DUnlikely(!buffer->p_send_buffer)) {
                    LOG_FATAL("no memory, request size %d\n", max_buffer_size);
                    return NULL;
                }
                buffer->send_buffer_size = max_buffer_size;
            }
            mpSendBufferList->RemoveContent((void *) buffer);
            return buffer;
        } else {
            LOG_FATAL("internal logic bug\n");
            return NULL;
        }
    } else {
        buffer = (SAgentSendBuffer *) DDBG_MALLOC(sizeof(SAgentSendBuffer), "ICAS");
        if (DUnlikely(!buffer)) {
            LOG_FATAL("no memory, request size %ld\n", (TULong)sizeof(SAgentSendBuffer));
            return NULL;
        }

        buffer->p_send_buffer = (TU8 *) DDBG_MALLOC(max_buffer_size, "ICAs");
        if (DUnlikely(!buffer->p_send_buffer)) {
            LOG_FATAL("no memory, request size %d\n", max_buffer_size);
            DDBG_FREE(buffer, "ICSA");
            return NULL;
        }
        buffer->send_buffer_size = max_buffer_size;
    }

    return buffer;
}

void CSACPIMClientAgentV2::releaseSendBuffer(SAgentSendBuffer *buffer)
{
    AUTO_LOCK(mpResourceMutex);
    mpSendBufferList->InsertContent(NULL, (void *) buffer, 0);
}

SAgentIDListBuffer *CSACPIMClientAgentV2::allocIDListBuffer(TU32 max_id_number)
{
    AUTO_LOCK(mpResourceMutex);
    SAgentIDListBuffer *buffer = NULL;
    TU32 max_buffer_size = max_id_number * sizeof(TUniqueID);

    CIDoubleLinkedList::SNode *p_node = mpIDListBufferList->FirstNode();
    if (p_node) {
        buffer = (SAgentIDListBuffer *) p_node->p_context;
        if (DLikely(buffer)) {
            if ((buffer->max_id_count * sizeof(TUniqueID)) < max_buffer_size) {
                if (buffer->p_id_list) {
                    free(buffer->p_id_list);
                    buffer->p_id_list = NULL;
                }
                buffer->max_id_count = 0;
            }
            if (!buffer->p_id_list) {
                buffer->p_id_list = (TUniqueID *) DDBG_MALLOC(max_buffer_size, "ICAL");
                if (DUnlikely(!buffer->p_id_list)) {
                    LOG_FATAL("no memory, request size %d\n", max_buffer_size);
                    return NULL;
                }
                buffer->max_id_count = max_id_number;
            }
            mpIDListBufferList->RemoveContent((void *) buffer);
            buffer->id_count = 0;
            return buffer;
        } else {
            LOG_FATAL("internal logic bug\n");
            return NULL;
        }
    } else {
        buffer = (SAgentIDListBuffer *) DDBG_MALLOC(sizeof(SAgentIDListBuffer), "ICAB");
        if (DUnlikely(!buffer)) {
            LOG_FATAL("no memory, request size %ld\n", (TULong)sizeof(SAgentIDListBuffer));
            return NULL;
        }

        buffer->p_id_list = (TUniqueID *) DDBG_MALLOC(max_buffer_size, "ICLT");
        if (DUnlikely(!buffer->p_id_list)) {
            LOG_FATAL("no memory, request size %d\n", max_buffer_size);
            DDBG_FREE(buffer, "ICAL");
            return NULL;
        }
        buffer->id_count = 0;
        buffer->max_id_count = max_id_number;
    }

    return buffer;
}

void CSACPIMClientAgentV2::releaseIDListBuffer(SAgentIDListBuffer *buffer)
{
    AUTO_LOCK(mpResourceMutex);
    mpIDListBufferList->InsertContent(NULL, (void *) buffer, 0);
}

void CSACPIMClientAgentV2::releasePendingAPI()
{
    CIDoubleLinkedList::SNode *p_node = mpMsgSlotList->FirstNode();
    CIConditionSlot *p_slot = NULL;

    while (p_node) {
        p_slot = (CIConditionSlot *)p_node->p_context;
        if (DLikely(p_slot)) {
            p_slot->SetReplyCode(EECode_AbortSessionQuitAPI);
            p_slot->Signal();
            mpFreeMsgSlotList->InsertContent(NULL, (void *) p_slot, 0);
        } else {
            LOG_FATAL("NULL p_slot\n");
        }
        p_node = mpMsgSlotList->NextNode(p_node);
    }

    mpMsgSlotList->RemoveAllNodes();
}

void CSACPIMClientAgentV2::releaseExpiredPendingAPI(SDateTime *time)
{
    CIDoubleLinkedList::SNode *p_node = mpMsgSlotList->FirstNode();
    CIConditionSlot *p_slot = NULL;

    while (p_node) {
        p_slot = (CIConditionSlot *)p_node->p_context;
        if (DLikely(p_slot)) {
            //to do, timeout check
            p_slot->SetReplyCode(EECode_AbortTimeOutAPI);
            p_slot->Signal();
            p_node = mpMsgSlotList->NextNode(p_node);
            releaseMsgSlot(p_slot);
            continue;
        } else {
            LOG_FATAL("NULL p_slot\n");
        }
        p_node = mpMsgSlotList->NextNode(p_node);
    }
}

CIConditionSlot *CSACPIMClientAgentV2::matchedMsgSlot(TU32 session_number, TU32 check_field)
{
    CIDoubleLinkedList::SNode *p_node = mpMsgSlotList->FirstNode();
    CIConditionSlot *p_slot = NULL;
    TU32 cur_session_number = 0;
    TU32 cur_check_field = 0;

    while (p_node) {
        p_slot = (CIConditionSlot *)p_node->p_context;
        if (DLikely(p_slot)) {
            cur_session_number = p_slot->GetSessionNumber();
            cur_check_field = p_slot->GetCheckField();
            LOG_NOTICE("session number %x, %x, check field %x, %x\n", session_number, cur_session_number, check_field, cur_check_field);
            if ((cur_session_number == session_number) && (check_field == cur_check_field)) {
                return p_slot;
            }
        } else {
            LOG_FATAL("NULL p_slot\n");
        }
        p_node = mpMsgSlotList->NextNode(p_node);
    }

    LOG_ERROR("do not find matched slot(session_number %x, check_field %x)?\n", session_number, check_field);
    return NULL;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


