/*******************************************************************************
 * admin_sacp_agent.cpp
 *
 * History:
 *  2014/07/02 - [Zhi He] create file
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

#ifdef BUILD_OS_WINDOWS
#include <direct.h>
#include <io.h>
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

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
//  CICommunicationClientPort
//
//-----------------------------------------------------------------------
CICommunicationClientPort *CICommunicationClientPort::Create(void *p_context, TCommunicationClientReadCallBack callback)
{
    CICommunicationClientPort *thiz = new CICommunicationClientPort(p_context, callback);
    if (DLikely(thiz)) {
        EECode err = thiz->Construct();
        if (DUnlikely(EECode_OK != err)) {
            delete thiz;
            LOG_FATAL("CICommunicationClientPort::Construct() fail, ret %d %s.\n", err, gfGetErrorCodeString(err));
            return NULL;
        }
        return thiz;
    }

    LOG_FATAL("new CICommunicationClientPort() fail.\n");
    return NULL;
}

void CICommunicationClientPort::Destroy()
{
    delete this;
}

CICommunicationClientPort::CICommunicationClientPort(void *p_context, TCommunicationClientReadCallBack callback)
    : mbStartConnecting(0)
    , mbConnectMode(0)
    , mbConnected(0)
    , mpMutex(NULL)
    , mSocket(-1)
    , mfReadCallBack(callback)
    , mpReadCallBackContext(p_context)
    , mnRetryInterval(10)
    , mpServerUrl(NULL)
    , mnServerUrlBufferLength(0)
    , mServerPort(DDefaultSACPAdminPort)
    , mNativePort(0)
    , mpSACPWriteHeader(NULL)
    , mpWriteBuffer(NULL)
    , mnWriteBufferSize(0)
    , mpSACPReadHeader(NULL)
    , mpReadBuffer(NULL)
    , mnReadBufferSize(0)
    , mpProtocolParser(NULL)
    , mnHeaderLength(0)
{
    memset(mName, 0x0, sizeof(mName));
    memset(mPassword, 0x0, sizeof(mPassword));
}

EECode CICommunicationClientPort::Construct()
{
    mnWriteBufferSize = DSACP_MAX_PAYLOAD_LENGTH + 128;
    mpWriteBuffer = (TU8 *) DDBG_MALLOC(mnWriteBufferSize, "ACWB");
    if (DUnlikely(!mpWriteBuffer)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    mpSACPWriteHeader = (SSACPHeader *) mpWriteBuffer;

    mnReadBufferSize = DSACP_MAX_PAYLOAD_LENGTH + 128;
    mpReadBuffer = (TU8 *) DDBG_MALLOC(mnReadBufferSize, "ACRB");
    if (DUnlikely(!mpReadBuffer)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    mpSACPReadHeader = (SSACPHeader *) mpReadBuffer;

    mpProtocolParser = gfCreatePotocolHeaderParser(EProtocolType_SACP, EProtocolHeaderExtentionType_SACP_ADMIN);
    DASSERT(mpProtocolParser);

    mnHeaderLength = mpProtocolParser->GetFixedHeaderLength();

    return EECode_OK;
}

CICommunicationClientPort::~CICommunicationClientPort()
{
    if (mpWriteBuffer) {
        DDBG_FREE(mpWriteBuffer, "ACWB");
        mpWriteBuffer = NULL;
    }
    if (mpReadBuffer) {
        DDBG_FREE(mpReadBuffer, "ACRB");
        mpReadBuffer = NULL;
    }
}

EECode CICommunicationClientPort::SetIdentifyString(TChar *url)
{
    if (DUnlikely(!url)) {
        LOG_ERROR("NULL params\n");
        return EECode_BadParam;
    }
    if (strlen(url) > DIdentificationStringLength) {
        LOG_ERROR("SetIdentifyString len=%ld invalid(should<=%d)\n", (TULong)strlen(url), DIdentificationStringLength);
        return EECode_BadParam;
    }
    memset(mIdentificationString + DDynamicInputStringLength, 0x0, DIdentificationStringLength);
    memcpy(mIdentificationString + DDynamicInputStringLength, url, strlen(url));
    return EECode_OK;
}

EECode CICommunicationClientPort::PrepareConnection(TChar *url, TU16 port, TChar *name, TChar *password)
{
    AUTO_LOCK(mpMutex);

    if (DUnlikely(mbStartConnecting)) {
        LOG_WARN("already in connecting state?\n");
        return EECode_BadState;
    }

    if (DUnlikely((!url) || (!name) || (!password))) {
        LOG_ERROR("NULL params, %p, %p, %p\n", url, name, password);
        return EECode_BadParam;
    }

    TU32 length = strlen(name);
    if (DUnlikely(length >= DMAX_ACCOUNT_NAME_LENGTH_EX)) {
        LOG_ERROR("username too long? %d\n", length);
        return EECode_BadParam;
    }
    strcpy(mName, name);

    length = strlen(password);
    if (DUnlikely(length >= DMAX_ACCOUNT_NAME_LENGTH_EX)) {
        LOG_ERROR("username too long? %d\n", length);
        return EECode_BadParam;
    }
    strcpy(mPassword, password);

    length = strlen(url);
    if (DUnlikely(length > 1024)) {
        LOG_ERROR("url too long? %d\n", length);
        return EECode_BadParam;
    }

    if (mpServerUrl && (length < mnServerUrlBufferLength)) {
        strcpy(mpServerUrl, url);
    } else {
        if (mpServerUrl) {
            DDBG_FREE(mpServerUrl, "ACSU");
        }
        mnServerUrlBufferLength = length + 4;
        mpServerUrl = (TChar *) DDBG_MALLOC(mnServerUrlBufferLength, "ACSU");
        if (DLikely(mpServerUrl)) {
            memset(mpServerUrl, 0x0, mnServerUrlBufferLength);
            strcpy(mpServerUrl, url);
        } else {
            LOG_FATAL("no memory, request %d\n", mnServerUrlBufferLength);
            mnServerUrlBufferLength = 0;
            return EECode_NoMemory;
        }
    }

    mServerPort = port;

    return EECode_OK;
}

EECode CICommunicationClientPort::ConnectServer()
{
    AUTO_LOCK(mpMutex);
    if (DUnlikely(0 != mbConnectMode)) {
        LOG_ERROR("bad state\n");
        return EECode_BadState;
    }

    EECode err = connectServer();

    return err;
}

EECode CICommunicationClientPort::LoopConnectServer(TU32 interval)
{
    AUTO_LOCK(mpMutex);
    if (DUnlikely(0 != mbConnectMode)) {
        LOG_ERROR("bad state\n");
        return EECode_BadState;
    }

    mbConnectMode = 2;
    mbStartConnecting = 1;
    mnRetryInterval = interval;

    mpThread = gfCreateThread("CICommunicationClientPort", LoopConnectEntry, (void *)this);
    if (!mpThread) {
        LOG_FATAL("gfCreateThread fail\n");
        return EECode_OSError;
    }

    return EECode_OK;
}

EECode CICommunicationClientPort::LoopConnectEntry(void *param)
{
    CICommunicationClientPort *thiz = (CICommunicationClientPort *) param;
    if (thiz) {
        EECode err = thiz->ConnectLoop();
        if (DLikely(EECode_OK == err)) {
            LOG_NOTICE("ConnectLoop done\n");
            return err;
        }

        LOG_ERROR("ConnectLoop fail, ret %d, %s, how to handle it\n", err, gfGetErrorCodeString(err));
        return err;
    }

    LOG_ERROR("NULL param?\n");
    return EECode_BadParam;
}

EECode CICommunicationClientPort::ConnectLoop()
{
    mbLoopConnecting = 1;

tag_reconnect:

    while (mbLoopConnecting) {
        EECode err = connectServer();
        if (DLikely(EECode_OK != err)) {
            LOG_NOTICE("gfNet_ConnectTo(%s:%hu) fail, continue, ret %d, %s\n", mpServerUrl, mServerPort, err, gfGetErrorCodeString(err));
            gfOSsleep(mnRetryInterval);
        } else {
            mbConnected = 1;
            break;
        }
    }

    TInt tobe_read_length = 0;
    TInt read_length = 0;
    TInt tot_read_length = 0;
    TU8 *p_read_buf = NULL;

    while (mbLoopConnecting) {
        tobe_read_length = sizeof(SSACPHeader);
        p_read_buf = mpReadBuffer;
        tot_read_length = 0;
tag_read_header:
        read_length = gfNet_Recv_timeout(mSocket, p_read_buf, tobe_read_length, 0, 1);
        if (!read_length) {
            //peer close
            AUTO_LOCK(mpMutex);
            mbLoopConnecting = 0;
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            break;
        } else if (0 > read_length) {
            if (!mbLoopConnecting) {
                break;
            }
            goto tag_read_header;
        } else if ((tot_read_length + read_length) == tobe_read_length) {
            tobe_read_length = (mpSACPReadHeader->size_0 << 16) | (mpSACPReadHeader->size_1 << 8) | mpSACPReadHeader->size_2;
            if (((TU32)tobe_read_length + sizeof(SSACPHeader)) >= mnReadBufferSize) {
                TU32 tmp_new_size = tobe_read_length + sizeof(SSACPHeader) + 128;
                TU8 *tmp_new = (TU8 *) DDBG_MALLOC(tmp_new_size, "ACRB");
                if (!tmp_new) {
                    AUTO_LOCK(mpMutex);
                    mbLoopConnecting = 0;
                    gfNetCloseSocket(mSocket);
                    mSocket = DInvalidSocketHandler;
                    break;
                }
                if (mpReadBuffer) {
                    memcpy(tmp_new, mpReadBuffer, sizeof(SSACPHeader));
                    DDBG_FREE(mpReadBuffer, "ACRB");
                }
                mpReadBuffer = tmp_new;
                mnReadBufferSize = tmp_new_size;
            }
            p_read_buf = mpReadBuffer + sizeof(SSACPHeader);
            tot_read_length = 0;
tag_read_payload:
            read_length = gfNet_Recv_timeout(mSocket, p_read_buf, tobe_read_length, 0, 5);
            if (!read_length) {
                //peer close
                AUTO_LOCK(mpMutex);
                mbLoopConnecting = 0;
                gfNetCloseSocket(mSocket);
                mSocket = DInvalidSocketHandler;
                break;
            } else if (0 > read_length) {
                if (!mbLoopConnecting) {
                    break;
                }
                goto tag_read_payload;
            } else if ((tot_read_length + read_length) == tobe_read_length) {
                if (mpReadCallBackContext && mfReadCallBack) {
                    DASSERT(mpProtocolParser);
                    TU32 payload_size, ext_type, ext_size, type, cat, sub_type;
                    mpProtocolParser->Parse(mpReadBuffer, payload_size, ext_type, ext_size, type, cat, sub_type);
                    mfReadCallBack(mpReadCallBackContext, mpReadBuffer + sizeof(SSACPHeader), (TU32)tobe_read_length, type, cat, sub_type);
                }
            } else {
                tobe_read_length -= read_length;
                tot_read_length += read_length;
                p_read_buf = p_read_buf + tot_read_length;
                goto tag_read_header;
            }
        } else {
            tobe_read_length -= read_length;
            tot_read_length += read_length;
            p_read_buf = p_read_buf + tot_read_length;
            goto tag_read_header;
        }

    }

    if (mbLoopConnecting) {
        gfOSsleep(2);
        goto tag_reconnect;
    }

    return EECode_OK;
}

EECode CICommunicationClientPort::Disconnect()
{
    AUTO_LOCK(mpMutex);

    if (!mbStartConnecting) {
        LOG_WARN("not connected yet\n");
        return EECode_OK;
    }
    mbStartConnecting = 0;

    mbLoopConnecting = 0;
    if (2 == mbConnectMode) {
        mpThread->Delete();
        mpThread = NULL;
    }
    mbConnectMode = 0;

    if (DLikely(0 <= mSocket)) {
        LOG_WARN("close previous socket(%d)\n", mSocket);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }
    mbConnected = 0;

    return EECode_OK;

}

TInt CICommunicationClientPort::Write(TU8 *data, TInt size)
{
    return write(data, size);
}

EECode CICommunicationClientPort::UpdateUserDynamicCode(TChar *user_name, TChar *device_name, TU32 dynamic_code)
{
    return updateDynamicCode(ESACPAdminSubType_UpdateUserDynamicCode, user_name, device_name, dynamic_code);
}

EECode CICommunicationClientPort::UpdateDeviceDynamicCode(TChar *name, TU32 dynamic_code)
{
    return updateDynamicCode(ESACPAdminSubType_UpdateDeviceDynamicCode, NULL, name, dynamic_code);
}

EECode CICommunicationClientPort::AddDeviceChannel(TChar *name)
{
    if (DUnlikely(!name)) {
        return EECode_BadParam;
    }

    TU8 *p_payload = mpWriteBuffer + sizeof(SSACPHeader);
    ETLV16Type type = ETLV16Type_String;
    TU16 length = 0;
    void *value = NULL;

    length = strlen(name) + 1;
    value = (void *)name;
    gfFillTLV16Struct(p_payload, &type, &length, &value);

    TU32 payload_size = sizeof(STLV16Header) + length;
    initSendBuffer(ESACPAdminSubType_AssignDataServerSourceDevice, EProtocolHeaderExtentionType_SACP_ADMIN, payload_size, ESACPTypeCategory_AdminChannel);

    TInt ret  = write(mpWriteBuffer, mnHeaderLength + payload_size);
    if (DUnlikely(ret < 0)) {
        LOG_FATAL("AddDeviceChannel fail\n");
        return EECode_IOError;
    }

    return EECode_OK;
}

TInt CICommunicationClientPort::write(TU8 *data, TInt size)
{
    AUTO_LOCK(mpMutex);
    TInt ret = 0;
    if (DLikely(mbConnected && (0 <= mSocket))) {
        ret = gfNet_Send_timeout(mSocket, data, size, 0, 20);
        if (DUnlikely(ret != size)) {
            LOG_ERROR("write request fail\n");
            return (-3);
        }
    } else {
        LOG_ERROR("not connected server yet, mbConnected %d, mSocket %d\n", mbConnected, mSocket);
        return (-1);
    }

    return ret;
}

void CICommunicationClientPort::fillHeader(SSACPHeader *p_header, TU32 type, TU32 size, TU8 ext_type, TU16 ext_size, TU32 seq_count)
{
    p_header->type_1 = (type >> 24) & 0xff;
    p_header->type_2 = (type >> 16) & 0xff;
    p_header->type_3 = (type >> 8) & 0xff;
    p_header->type_4 = type & 0xff;

    //LOG_NOTICE("0x%08x \n", type);
    //LOG_NOTICE("%02x %02x %02x %02x\n", (type >> 24) & 0xff, (type >> 16) & 0xff, (type >> 8) & 0xff, type & 0xff);

    p_header->size_1 = (size >> 8) & 0xff;
    p_header->size_2 = size & 0xff;
    p_header->size_0 = (size >> 16) & 0xff;

    p_header->seq_count_0 = (seq_count >> 16) & 0xff;
    p_header->seq_count_1 = (seq_count >> 8) & 0xff;
    p_header->seq_count_2 = seq_count & 0xff;

    p_header->encryption_type_1 = 0;
    p_header->encryption_type_2 = 0;

    p_header->flags = 0;
    p_header->header_ext_type = ext_type;
    p_header->header_ext_size_1 = (ext_size >> 8) & 0xff;
    p_header->header_ext_size_2 = ext_size & 0xff;
}

EECode CICommunicationClientPort::connectServer()
{
    EECode err = EECode_OK;
    TU32 hash_result = 0;
    TU8 *tmp = NULL;
    TU32 type = 0;
    TInt len_account = 0, len_password = 0, tot_len = 0;
    TInt ret = 0;

    if (DUnlikely(DIsSocketHandlerValid(mSocket))) {
        LOG_WARN("close previous socket(%d)\n", mSocket);
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    mSocket = gfNet_ConnectTo((const TChar *)mpServerUrl, mServerPort, SOCK_STREAM, IPPROTO_TCP);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("gfNet_ConnectTo(%s:%hu) fail\n", mpServerUrl, mServerPort);
        err = EECode_NetConnectFail;
        goto tag_authentication_fail;
    }

    gfSocketSetTimeout(mSocket, 2, 0);

    type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ConnectionTag, 0);
    len_account = strlen(mName);
    len_password = strlen(mPassword);

    tot_len = len_account + len_password + 1;
    fillHeader(mpSACPWriteHeader, type, (TU32)tot_len, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0);

    memcpy(mpWriteBuffer + sizeof(SSACPHeader), mName, len_account);
    mpWriteBuffer[len_account + sizeof(SSACPHeader)] = ':';
    memcpy(mpWriteBuffer + len_account + sizeof(SSACPHeader) + 1, mPassword, len_password);

    tot_len += sizeof(SSACPHeader);
    ret = gfNet_Send_timeout(mSocket, mpWriteBuffer, tot_len, 0, 4);

    if (DLikely(ret != tot_len)) {
        LOG_ERROR("send authentication string fail\n");
        err = EECode_NetSendHeader_Fail;
        goto tag_authentication_fail;
    }

    tot_len = sizeof(SSACPHeader) + DDynamicInputStringLength;
    ret = gfNet_Recv_timeout(mSocket, mpReadBuffer, tot_len, 0, 10);

    if (DLikely(ret != tot_len)) {
        LOG_ERROR("receive dynamic input fail\n");
        err = EECode_NetReceiveHeader_Fail;
        goto tag_authentication_fail;
    }

    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
        err = EECode_Closed;
        goto tag_authentication_fail;
    } else if (DUnlikely(ret != tot_len)) {
        LOG_ERROR("receive dynamic input fail\n");
        err = EECode_NetReceiveHeader_Fail;
        goto tag_authentication_fail;
    }

    if (DLikely(ESACPConnectResult_OK == mpSACPReadHeader->flags)) {
    } else if (ESACPConnectResult_Reject_NoSuchChannel == mpSACPReadHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_NoSuchChannel\n");
        err = EECode_ServerReject_NoSuchChannel;
        goto tag_authentication_fail;
    } else if (ESACPConnectResult_Reject_ChannelIsInUse == mpSACPReadHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_ChannelIsInUse\n");
        err = EECode_AlreadyExist;
        goto tag_authentication_fail;
    } else if (ESACPConnectResult_Reject_BadRequestFormat == mpSACPReadHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_BadRequestFormat\n");
        err = EECode_ServerReject_BadRequestFormat;
        goto tag_authentication_fail;
    } else if (ESACPConnectResult_Reject_CorruptedProtocol == mpSACPReadHeader->flags) {
        LOG_ERROR("ESACPConnectResult_Reject_CorruptedProtocol\n");
        err = EECode_ServerReject_CorruptedProtocol;
        goto tag_authentication_fail;
    } else {
        LOG_ERROR("unknown error, mpSACPHeader->flags %08x\n", mpSACPReadHeader->flags);
        err = EECode_ServerReject_Unknown;
        goto tag_authentication_fail;
    }

    DASSERT(DDynamicInputStringLength == (((TU32)mpSACPReadHeader->size_0 << 16) | ((TU32)mpSACPReadHeader->size_1 << 8) | ((TU32)mpSACPReadHeader->size_2))) ;

    memcpy(mIdentificationString, mpReadBuffer + sizeof(SSACPHeader), DDynamicInputStringLength);
    hash_result = gfHashAlgorithmV6(mIdentificationString, DDynamicInputStringLength + DIdentificationStringLength);

    fillHeader(mpSACPWriteHeader, type, 4, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0);
    tmp = mpWriteBuffer + sizeof(SSACPHeader);
    tmp[0] = (hash_result >> 24) & 0xff;
    tmp[1] = (hash_result >> 16) & 0xff;
    tmp[2] = (hash_result >> 8) & 0xff;
    tmp[3] = (hash_result) & 0xff;

    tot_len = sizeof(SSACPHeader) + 4;
    ret = gfNet_Send_timeout(mSocket, mpWriteBuffer, tot_len, 0, 4);
    if (DLikely(ret != tot_len)) {
        LOG_ERROR("send dynamic code fail\n");
        err = EECode_NetSendHeader_Fail;
        goto tag_authentication_fail;
    }

    tot_len = sizeof(SSACPHeader);
    ret = gfNet_Recv_timeout(mSocket, mpReadBuffer, tot_len, 0, 20);
    if (DUnlikely(0 == ret)) {
        LOG_ERROR("gfNet_Recv(), return 0, peer closed\n");
        err = EECode_Closed;
        goto tag_authentication_fail;
    } else if (DUnlikely(ret != tot_len)) {
        LOG_ERROR("recv authetication result fail\n");
        return EECode_NetReceiveHeader_Fail;
    }

    if (DLikely(ESACPConnectResult_OK == mpSACPReadHeader->flags)) {

    } else if (ESACPConnectResult_Reject_AuthenticationFail == mpSACPReadHeader->flags) {
        LOG_ERROR("dynamic code check fail, ESACPConnectResult_Reject_AuthenticationFail\n");
        return EECode_ServerReject_AuthenticationFail;
    } else {
        LOG_ERROR("unknown error, mpSACPHeader->flags %08x\n", mpSACPReadHeader->flags);
        return EECode_ServerReject_Unknown;
    }

    mbConnectMode = 1;
    mbConnected = 1;
    mbStartConnecting = 1;

    return EECode_OK;

tag_authentication_fail:

    if (DUnlikely(DIsSocketHandlerValid(mSocket))) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    return err;
}

void CICommunicationClientPort::initSendBuffer(ESACPAdminSubType sub_type, EProtocolHeaderExtentionType ext_type, TU16 payload_size, ESACPTypeClientCategory cat_type)
{
    TU32 type = DBuildSACPType(DSACPTypeBit_Request, cat_type, sub_type);
    fillHeader(mpSACPWriteHeader, type, payload_size, ext_type, 0, 0);
    return;
}

EECode CICommunicationClientPort::updateDynamicCode(ESACPAdminSubType sub_type, TChar *user_name, TChar *device_name, TU32 dynamic_code)
{
    if (DUnlikely(!user_name && !device_name)) {
        return EECode_BadParam;
    }

    TU8 *p_payload = mpWriteBuffer + sizeof(SSACPHeader);
    ETLV16Type type = ETLV16Type_Invalid;
    TU16 length = 0;
    void *value = NULL;
    TU32 payload_size = 0;

    if (user_name) {
        type = ETLV16Type_AccountName;
        length = strlen(user_name) + 1;
        value = (void *)user_name;
        gfFillTLV16Struct(p_payload, &type, &length, &value, 1);
        payload_size += sizeof(STLV16Header) + length;
        p_payload += sizeof(STLV16Header) + length;
    }

    if (device_name) {
        type = ETLV16Type_DeviceName;
        length = strlen(device_name) + 1;
        value = (void *)device_name;
        gfFillTLV16Struct(p_payload, &type, &length, &value, 1);
        p_payload += sizeof(STLV16Header) + length;
        payload_size += sizeof(STLV16Header) + length;
    }

    type = ETLV16Type_DynamicCode;
    length = sizeof(dynamic_code);
    value = (void *)&dynamic_code;
    gfFillTLV16Struct(p_payload, &type, &length, &value, 1);
    payload_size += sizeof(STLV16Header) + length;

    initSendBuffer(sub_type, EProtocolHeaderExtentionType_SACP_ADMIN, payload_size, ESACPTypeCategory_AdminChannel);

    TInt ret  = write(mpWriteBuffer, mnHeaderLength + payload_size);
    if (DUnlikely(ret < 0)) {
        LOG_FATAL("sendAuthenticationInfo fail, type: 0x%x\n", sub_type);
        return EECode_IOError;
    }

    return EECode_OK;
}

//-----------------------------------------------------------------------
//
//  CICommunicationServerPort
//
//-----------------------------------------------------------------------
CICommunicationServerPort *CICommunicationServerPort::Create(void *p_context, TCommunicationServerReadCallBack callback, TU16 port)
{
    CICommunicationServerPort *thiz = new CICommunicationServerPort(p_context, callback, port);
    if (DLikely(thiz)) {
        EECode err = thiz->Construct();
        if (DUnlikely(EECode_OK != err)) {
            delete thiz;
            LOG_FATAL("CICommunicationServerPort::Construct() fail, ret %d %s.\n", err, gfGetErrorCodeString(err));
            return NULL;
        }
        return thiz;
    }

    LOG_FATAL("new CICommunicationServerPort() fail.\n");
    return NULL;
}

void CICommunicationServerPort::Destroy()
{
    delete this;
}

CObject *CICommunicationServerPort::GetObject() const
{
    return (CObject *) this;
}

CICommunicationServerPort::CICommunicationServerPort(void *p_context, TCommunicationServerReadCallBack callback, TU16 port)
    : inherited("CICommunicationServerPort", 0)
    , mpSourceList(NULL)
    , mpPersistCommonConfig(NULL)
    , mpMsgSink(NULL)
    , mpSystemClockReference(NULL)
    , mpWorkQueue(NULL)
    , mListenSocket(-1)
    , mPort(port)
    , mbRun(1)
    , msState(EServerManagerState_idle)
    , mMaxFd(0)
    , mfReadCallBack(callback)
    , mpReadCallBackContext(p_context)
    , mpSACPWriteHeader(NULL)
    , mpWriteBuffer(NULL)
    , mnWriteBufferSize(0)
    , mpSACPReadHeader(NULL)
    , mpReadBuffer(NULL)
    , mnReadBufferSize(0)
    , mpProtocolParser(NULL)
    , mnHeaderLength(0)
{

}

EECode CICommunicationServerPort::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleSACPAdminAgent);
    //mConfigLogLevel = ELogLevel_Debug;
    //mConfigLogOption = 0xffffffff;

    mnWriteBufferSize = DSACP_MAX_PAYLOAD_LENGTH + 128;
    mpWriteBuffer = (TU8 *) DDBG_MALLOC(mnWriteBufferSize, "ASWB");
    if (DUnlikely(!mpWriteBuffer)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    mpSACPWriteHeader = (SSACPHeader *) mpWriteBuffer;

    mnReadBufferSize = DSACP_MAX_PAYLOAD_LENGTH + 128;
    mpReadBuffer = (TU8 *) DDBG_MALLOC(mnReadBufferSize, "ASRB");
    if (DUnlikely(!mpReadBuffer)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    mpSourceList = new CIDoubleLinkedList();
    if (DUnlikely(!mpSourceList)) {
        LOG_FATAL("no memory, new CIDoubleLinkedList() fail\n");
        return EECode_NoMemory;
    }

    EECode err = gfOSCreatePipe(mPipeFd);
    DASSERT(err == EECode_OK);

    mpSACPReadHeader = (SSACPHeader *) mpReadBuffer;

    mListenSocket = gfNet_SetupStreamSocket(INADDR_ANY, mPort, 1);
    if (DUnlikely(0 > mListenSocket)) {
        LOG_FATAL("setup socket fail, ret %d.\n", mListenSocket);
        return EECode_Error;
    }
    //LOGM_NOTICE("gfNet_SetupStreamSocket, socket %d, port %d.\n", mListenSocket, mPort);

    //TInt val = 2;
    //setsockopt(mListenSocket, IPPROTO_TCP, TCP_DEFER_ACCEPT, &val, sizeof(val));

    mpWorkQueue = CIWorkQueue::Create((IActiveObject *)this);
    if (DUnlikely(!mpWorkQueue)) {
        LOGM_ERROR("Create CIWorkQueue fail.\n");
        return EECode_NoMemory;
    }

    //LOGM_INFO("before CICommunicationServerPort::mpWorkQueue->Run().\n");
    DASSERT(mpWorkQueue);
    mpWorkQueue->Run();
    //LOGM_INFO("after CICommunicationServerPort::mpWorkQueue->Run().\n");

    mpProtocolParser = gfCreatePotocolHeaderParser(EProtocolType_SACP, EProtocolHeaderExtentionType_SACP_ADMIN);
    DASSERT(mpProtocolParser);
    mnHeaderLength = mpProtocolParser->GetFixedHeaderLength();

    return EECode_OK;
}

CICommunicationServerPort::~CICommunicationServerPort()
{
    if (DUnlikely(DIsSocketHandlerValid(mListenSocket))) {
        gfNetCloseSocket(mListenSocket);
        mListenSocket = DInvalidSocketHandler;
    }

    if (mpWriteBuffer) {
        DDBG_FREE(mpWriteBuffer, "ASWB");
        mpWriteBuffer = NULL;
    }

    if (mpReadBuffer) {
        DDBG_FREE(mpReadBuffer, "ASRB");
        mpReadBuffer = NULL;
    }

    if (mpSourceList) {
        delete mpSourceList;
        mpSourceList = NULL;
    }
}

EECode CICommunicationServerPort::AddSource(TChar *source, TChar *password, void *p_source, TChar *p_identification_string, void *p_context, SCommunicationPortSource *&p_port)
{
    if (DUnlikely((!source) || (!password) || (!p_identification_string))) {
        LOG_ERROR("NULL params\n");
        return EECode_BadParam;
    }

    SCommunicationPortSource *p_new_port = (SCommunicationPortSource *) DDBG_MALLOC(sizeof(SCommunicationPortSource), "ASPS");
    if (DUnlikely(!p_new_port)) {
        LOG_ERROR("no memory\n");
        return EECode_NoMemory;
    }

    memset(p_new_port, 0x0, sizeof(SCommunicationPortSource));
    TInt length = strlen(source);
    if (DUnlikely(DMAX_ACCOUNT_NAME_LENGTH_EX < length)) {
        LOG_ERROR("too long source %s, %d\n", source, length);
        DDBG_FREE(p_new_port, "ASPS");
        p_port = NULL;
        return EECode_BadParam;
    }
    strcpy(p_new_port->source, source);
    strcpy(p_new_port->username_password_combo, source);

    length = strlen(password);
    if (DUnlikely(DMAX_ACCOUNT_NAME_LENGTH_EX < length)) {
        LOG_ERROR("too long password %s, %d\n", password, length);
        DDBG_FREE(p_new_port, "ASPS");
        p_port = NULL;
        return EECode_BadParam;
    }
    strcpy(p_new_port->password, password);
    p_new_port->username_password_combo[strlen(p_new_port->source)] = ':';
    strcpy(p_new_port->username_password_combo + strlen(p_new_port->source) + 1, password);

    length = strlen(p_identification_string);
    if (DUnlikely(DIdentificationStringLength < length)) {
        LOG_ERROR("not valid identification string %s, %d(should<=%d)\n", p_identification_string, length, DIdentificationStringLength);
        DDBG_FREE(p_new_port, "ASPS");
        p_port = NULL;
        return EECode_BadParam;
    }
    strcpy(p_new_port->id_string_combo + DDynamicInputStringLength, p_identification_string);

    p_new_port->p = p_context;
    p_new_port->socket = -1;

    mpSourceList->InsertContent(NULL, (void *)p_new_port, 1);

    p_port = p_new_port;

    return EECode_OK;
}

EECode CICommunicationServerPort::RemoveSource(SCommunicationPortSource *p_port)
{
    if (DLikely(p_port)) {
        mpSourceList->RemoveContent((void *) p_port);
        DDBG_FREE(p_port, "ASPS");
        return EECode_OK;
    }

    LOG_FATAL("NULL pointer\n");
    return EECode_BadParam;
}

EECode CICommunicationServerPort::Start(TU16 port)
{
    EECode err;
    DASSERT(mpWorkQueue);

    LOGM_INFO("Start() start.\n");
    //wake up server manager, post cmd to it
    TChar wake_char = 'a';
    size_t ret = 0;

    ret = write(mPipeFd[1], &wake_char, 1);
    DASSERT(1 == ret);

    err = mpWorkQueue->SendCmd(ECMDType_Start, NULL);
    LOGM_INFO("Start end, ret %d.\n", err);
    return err;
}

EECode CICommunicationServerPort::Stop()
{
    EECode err;
    DASSERT(mpWorkQueue);

    LOGM_INFO("CICommunicationServerPort::Stop() start.\n");
    //wake up server manager, post cmd to it
    TChar wake_char = 'b';
    size_t ret = 0;

    ret = write(mPipeFd[1], &wake_char, 1);
    DASSERT(1 == ret);

    err = mpWorkQueue->SendCmd(ECMDType_Stop, NULL);
    LOGM_INFO("CICommunicationServerPort::Stop() end, ret %d.\n", err);
    return err;
}

EECode CICommunicationServerPort::Reply(SCommunicationPortSource *p_port, TU8 *p_data, TU32 size)
{
    TInt ret = 0;
    if (DLikely(p_data && size && p_port)) {
        ret = gfNet_Send_timeout(p_port->socket, p_data, size, 0, 10);
        if (DUnlikely(((TU32)ret) != size)) {
            LOGM_FATAL("send reply fail\n");
            return EECode_NetSendPayload_Fail;
        }
    } else {
        LOGM_FATAL("BAD params\n");
        return EECode_BadParam;
    }

    return EECode_OK;
}

void CICommunicationServerPort::OnRun()
{
    SCMD cmd;
    TInt nready = 0;
    SCommunicationPortSource *p_port = NULL;
    CIDoubleLinkedList::SNode *pnode = NULL;
    TU32 payload_size, ext_type, ext_size, type, cat, sub_type;
    TInt length = 0;
    TInt ret = 0;
    TSocketHandler socket = 0;

    struct sockaddr_in client_addr;
    socklen_t   clilen = sizeof(struct sockaddr_in);
    fd_set mAllSet;
    fd_set mReadSet;

    FD_ZERO(&mAllSet);
    FD_ZERO(&mReadSet);

    FD_SET(mPipeFd[0], &mAllSet);
    FD_SET(mListenSocket, &mAllSet);
    if (mPipeFd[0] > mListenSocket) {
        mMaxFd = mPipeFd[0];
    } else {
        mMaxFd = mListenSocket;
    }

    mpWorkQueue->CmdAck(EECode_OK);

    //signal(SIGPIPE,SIG_IGN);

    //msState = EServerManagerState_idle;

    while (mbRun) {

        //LOGM_STATE("CICommunicationServerPort::OnRun start switch state %d, %s, mbRun %d.\n", msState, gfGetServerManagerStateString(msState), mbRun);

        switch (msState) {

            case EServerManagerState_idle:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EServerManagerState_running:
                mReadSet = mAllSet;

                //LOGM_NOTICE("!!before select\n");
                nready = select(mMaxFd + 1, &mReadSet, NULL, NULL, NULL);
                //LOGM_NOTICE("!!after select, %d\n", nready);
                if (DUnlikely(0 > nready)) {
                    LOGM_FATAL("select fail\n");
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

                if (FD_ISSET(mListenSocket, &mReadSet)) {
                    nready --;

                    socket = accept(mListenSocket, (struct sockaddr *)&client_addr, &clilen);
                    if (DUnlikely(socket < 0)) {
                        LOGM_ERROR("accept fail, return %d, how to handle it?\n", socket);
                        break;
                    } else {
                        LOGM_PRINTF(" NEW client: %s, port %d, socket %d.\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), socket);
                        gfSocketSetTimeout(socket, DPresetSocketTimeOutUintSeconds, DPresetSocketTimeOutUintUSeconds);
                        if (EECode_OK == authentication(socket, p_port)) {
                            if (DLikely(p_port)) {
                                p_port->socket = socket;
                                LOGM_NOTICE("!!add socket %d, mMaxFd %d\n", socket, mMaxFd);
                                FD_SET(socket, &mAllSet);
                                if (mMaxFd < socket) {
                                    mMaxFd = socket;
                                }
                            } else {
                                LOGM_FATAL("NULL p_port\n");
                            }
                        } else {
                            gfNetCloseSocket(socket);
                            socket = DInvalidSocketHandler;

                            LOGM_WARN("authentication fail\n");
                        }
                    }

                    if (nready <= 0) {
                        //read done
                        LOGM_INFO(" read done.\n");
                        break;
                    }
                }

                pnode = mpSourceList->FirstNode();

                while (pnode) {
                    p_port = (SCommunicationPortSource *)pnode->p_context;
                    if (DUnlikely(NULL == p_port)) {
                        LOGM_FATAL("Fatal error(NULL == p_port), must not get here.\n");
                        break;
                    }

                    if (DLikely(0 <= p_port->socket)) {
                        if (FD_ISSET(p_port->socket, &mReadSet)) {
                            nready --;
                            length = mnHeaderLength;
                            ret = gfNet_Recv_timeout(p_port->socket, mpReadBuffer, length, 0, 20);
                            if (DUnlikely(!ret)) {
                                LOGM_ERROR("peer closed\n");
                                gfNetCloseSocket(p_port->socket);
                                FD_CLR(p_port->socket, &mAllSet);
                                p_port->socket = DInvalidSocketHandler;
                                break;
                            } else if (DUnlikely(ret != length)) {
                                LOG_ERROR("recv header fail, how to handle it? ret %d, length %d\n", ret, length);
                                gfNetCloseSocket(p_port->socket);
                                FD_CLR(p_port->socket, &mAllSet);
                                p_port->socket = DInvalidSocketHandler;
                                break;
                            }
                            mpProtocolParser->Parse(mpReadBuffer, payload_size, ext_type, ext_size, type, cat, sub_type);

                            length = payload_size;
                            TU8 *p_payload = mpReadBuffer + mnHeaderLength;
                            ret = gfNet_Recv_timeout(p_port->socket, p_payload, length, 0, 20);
                            if (DUnlikely(ret != length)) {
                                LOG_ERROR("recv payload fail, how to handle it? ret %d, length %d\n", ret, length);
                                break;
                            }
                            mfReadCallBack((void *) mpReadCallBackContext, p_port->source, p_port->p, (void *) p_port, mpReadBuffer + sizeof(SSACPHeader), payload_size, type, cat, sub_type);
                        }
                    }

                    if (nready <= 0) {
                        //read done
                        break;
                    }
                    pnode = mpSourceList->NextNode(pnode);
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

    }

}

void CICommunicationServerPort::processCmd(SCMD &cmd)
{
    DASSERT(mpWorkQueue);
    TChar char_buffer;
    gfOSReadPipe(mPipeFd[0], char_buffer);

    switch (cmd.code) {
        case ECMDType_Start:
            //clear all running server, todo
            msState = EServerManagerState_running;
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        case ECMDType_Stop:
            msState = EServerManagerState_halt;
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        default:
            LOGM_ERROR("wrong cmd.code: %d", cmd.code);
            break;
    }

    LOG_PRINTF("****CICommunicationServerPort::ProcessCmd, cmd.code %d end.\n", cmd.code);
}

void CICommunicationServerPort::replyToClient(TInt socket, EECode error_code, SSACPHeader *header, TU32 reply_size)
{
    mpProtocolParser->SetReqResBits((TU8 *)header, DSACPTypeBit_Responce);

    switch (error_code) {

        case EECode_OK:
            header->flags = ESACPConnectResult_OK;
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

    LOGM_NOTICE("header->flags %08x\n", header->flags);
    TInt ret = gfNet_Send(socket, (TU8 *)header, reply_size, 0);
    if (DUnlikely(ret != (TInt)reply_size)) {
        LOGM_FATAL("send reply error, ret %d\n", ret);
    }

}

EECode CICommunicationServerPort::authentication(TInt socket, SCommunicationPortSource *&p_authen_port)
{
    CIDoubleLinkedList::SNode *p_node = mpSourceList->FirstNode();
    SCommunicationPortSource *p_content = NULL;
    TMemSize string_size = 0, string_size1 = 0;

    if (!p_node) {
        LOGM_ERROR("no content\n");
        replyToClient(socket, EECode_BadState, mpSACPReadHeader, sizeof(SSACPHeader) + sizeof(TU64));
        return EECode_BadState;
    }

    p_content = (SCommunicationPortSource *)p_node->p_context;
    if (!p_content) {
        LOGM_FATAL("NULL p_context\n");
        replyToClient(socket, EECode_InternalLogicalBug, mpSACPReadHeader, sizeof(SSACPHeader) + sizeof(TU64));
        return EECode_InternalLogicalBug;
    }

    DASSERT(0 < socket);
    TInt bytesRead = gfNet_Recv_timeout(socket, mpReadBuffer, sizeof(SSACPHeader), DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);

    if (DUnlikely(((TInt)sizeof(SSACPHeader)) != bytesRead)) {
        LOGM_WARN("authentication, recv %d not expected, expect %lu\n", bytesRead, (TULong)sizeof(SSACPHeader));
        replyToClient(socket, EECode_Closed, mpSACPReadHeader, sizeof(SSACPHeader) + sizeof(TU64));
        return EECode_Closed;
    }

    LOGM_PRINTF("authentication: receive header: %d\n", bytesRead);

    DASSERT(bytesRead == sizeof(SSACPHeader));
    TU32 type = DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_ConnectionTag, 0);
    TU32 cur_type = (mpSACPReadHeader->type_1 << 24) | (mpSACPReadHeader->type_2 << 16) | (mpSACPReadHeader->type_3 << 8) | mpSACPReadHeader->type_4;
    if (DLikely(cur_type == type)) {
        TU32 header_url_size = ((TU32)mpSACPReadHeader->size_0 << 16) | ((TU32)mpSACPReadHeader->size_1 << 8) | (TU32)mpSACPReadHeader->size_2;
        LOGM_PRINTF("authentication: header size: %d\n", header_url_size);
        if (!header_url_size) {
            LOGM_ERROR("no authenication string\n");
            replyToClient(socket, EECode_ServerReject_BadRequestFormat, mpSACPReadHeader, sizeof(SSACPHeader) + sizeof(TU64));
            return EECode_Closed;
        } else if (DUnlikely(header_url_size >= (DMAX_ACCOUNT_NAME_LENGTH_EX * 2 + 1))) {
            LOGM_WARN("too long input string %d\n", header_url_size);
            replyToClient(socket, EECode_ServerReject_BadRequestFormat, mpSACPReadHeader, sizeof(SSACPHeader) + sizeof(TU64));
            return EECode_Closed;
        } else {
            TChar tmp_string[DMAX_ACCOUNT_NAME_LENGTH_EX * 2 + 2] = {0};
            TInt bytesRead = gfNet_Recv_timeout(socket, (TU8 *)&tmp_string[0], header_url_size, DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
            if (DUnlikely(((TInt)header_url_size) != bytesRead)) {
                LOGM_WARN("authentication, recv %d not expected, expect %d\n", bytesRead, header_url_size);
                replyToClient(socket, EECode_ServerReject_BadRequestFormat, mpSACPReadHeader, sizeof(SSACPHeader) + sizeof(TU64));
                return EECode_Closed;
            }

            string_size1 = strlen(tmp_string);

            LOGM_PRINTF("authenication: read combo %s, header_url_size %d, string_size1 %ld\n", tmp_string, header_url_size, string_size1);

            while (p_node) {
                p_content = (SCommunicationPortSource *)p_node->p_context;
                if (DLikely(p_content)) {
                    string_size = strlen(p_content->username_password_combo);

                    LOGM_PRINTF("p_content->username_password_combo %s, string_size %ld\n", p_content->username_password_combo, string_size);
                    if (string_size1 == string_size) {
                        if (!memcmp(p_content->username_password_combo, tmp_string, string_size1)) {
                            LOGM_PRINTF("find matched string %s\n", p_content->username_password_combo);
                            p_authen_port = p_content;
                            if (DUnlikely(0 <= p_content->socket)) {
                                replyToClient(socket, EECode_ServerReject_ChannelIsBusy, mpSACPReadHeader, sizeof(SSACPHeader) + sizeof(TU64));
                                return EECode_ServerReject_ChannelIsBusy;
                            } else {
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

                                LOGM_PRINTF("send dynamic input string %ld\n", (TULong)(sizeof(SSACPHeader) + sizeof(TU64)));
                                mpSACPReadHeader->size_0 = 0;
                                mpSACPReadHeader->size_1 = 0;
                                mpSACPReadHeader->size_2 = DDynamicInputStringLength;

                                replyToClient(socket, EECode_OK, mpSACPReadHeader, sizeof(SSACPHeader) + sizeof(TU64));

                                memcpy(p_content->id_string_combo, p, DDynamicInputStringLength);
                                TU32 hash_verify = gfHashAlgorithmV6((TU8 *)p_content->id_string_combo, DDynamicInputStringLength + DIdentificationStringLength);
                                bytesRead = gfNet_Recv_timeout(socket, mpReadBuffer, sizeof(SSACPHeader) + 4, DNETUTILS_RECEIVE_FLAG_READ_ALL, 20);

                                if (DUnlikely(((TInt)sizeof(SSACPHeader) + 4) != bytesRead)) {
                                    LOGM_WARN("authentication, recv hash %d not expected, expect %lu, time out?\n", bytesRead, (TULong)sizeof(SSACPHeader) + 4);
                                    replyToClient(socket, EECode_ServerReject_TimeOut, mpSACPReadHeader, sizeof(SSACPHeader) + sizeof(TU64));
                                    return EECode_ServerReject_TimeOut;
                                }

                                header_url_size = ((TU32)mpSACPReadHeader->size_0 << 16) | ((TU32)mpSACPReadHeader->size_1 << 8) | (TU32)mpSACPReadHeader->size_2;
                                if (DLikely(4 == header_url_size)) {
                                    TU32 hash_recv = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
                                    if (hash_recv == hash_verify) {
                                        LOGM_PRINTF("verified, send %d\n", bytesRead);
                                        replyToClient(socket, EECode_OK, mpSACPReadHeader, sizeof(SSACPHeader));
                                    } else {
                                        LOG_ERROR("authentication fail, hash_recv %08x, hash_verify %08x\n", hash_recv, hash_verify);
                                        replyToClient(socket, EECode_ServerReject_AuthenticationFail, mpSACPReadHeader, sizeof(SSACPHeader));
                                        return EECode_ServerReject_AuthenticationFail;
                                    }
                                } else {
                                    LOG_ERROR("authentication, recv hash length %d not expected\n", header_url_size);
                                    replyToClient(socket, EECode_ServerReject_CorruptedProtocol, mpSACPReadHeader, sizeof(SSACPHeader));
                                    return EECode_ServerReject_CorruptedProtocol;
                                }
                            }

                            return EECode_OK;
                        }
                    }
                } else {
                    LOGM_FATAL("NULL p_content\n");
                    replyToClient(socket, EECode_InternalLogicalBug, mpSACPReadHeader, sizeof(SSACPHeader));
                    return EECode_InternalLogicalBug;
                }
                p_node = mpSourceList->NextNode(p_node);
            }
            LOGM_ERROR("do not find matched string %s, header_url_size %d, string_size1 %ld\n", tmp_string, header_url_size, string_size1);
        }
    } else {
        LOGM_FATAL("client error, bad type when connecting, cur_type 0x%08x, type 0x%08x\n", cur_type, type);
        LOGM_PRINTF("mpSACPReadHeader %p, mpReadBuffer %p\n", mpSACPReadHeader, mpReadBuffer);
        LOGM_PRINTF("%02x %02x %02x %02x, %02x %02x %02x %02x\n", mpReadBuffer[0], mpReadBuffer[1], mpReadBuffer[2], mpReadBuffer[3], mpReadBuffer[4], mpReadBuffer[5], mpReadBuffer[6], mpReadBuffer[7]);
        LOGM_PRINTF("%02x %02x %02x %02x\n", mpSACPReadHeader->type_1, mpSACPReadHeader->type_2, mpSACPReadHeader->type_3, mpSACPReadHeader->type_4);

        replyToClient(socket, EECode_ServerReject_CorruptedProtocol, mpSACPReadHeader, sizeof(SSACPHeader));
        return EECode_InternalLogicalBug;
    }

    replyToClient(socket, EECode_ServerReject_NoSuchChannel, mpSACPReadHeader, sizeof(SSACPHeader));
    return EECode_Closed;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

