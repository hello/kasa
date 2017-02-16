
/**
 * directsharing_client.cpp
 *
 * History:
 *    2015/03/11 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#include "common_config.h"

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"
#include "common_network_utils.h"

#include "directsharing_if.h"

#include "directsharing_client.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IDirectSharingClient *gfCreateDirectSharingClient(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
{
    return (IDirectSharingClient *) CIDirectSharingClient::Create(pconfig, pMsgSink, p_system_clock_reference);
}

CIDirectSharingClient::CIDirectSharingClient(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
    : mpPersistCommonConfig(pconfig)
    , mpMsgSink(pMsgSink)
    , mpSystemClockReference(p_system_clock_reference)
    , mpWorkQueue(NULL)
    , mSocket(DInvalidSocketHandler)
{
}

CIDirectSharingClient::~CIDirectSharingClient()
{
}

CIDirectSharingClient *CIDirectSharingClient::Create(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
{
    CIDirectSharingClient *result = new CIDirectSharingClient(pconfig, pMsgSink, p_system_clock_reference);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

EECode CIDirectSharingClient::Construct()
{
    return EECode_OK;
}

void CIDirectSharingClient::Destroy()
{
    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }
}

EECode CIDirectSharingClient::ConnectServer(TChar *server_url, TSocketPort server_port, TChar *username, TChar *password)
{
    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    mSocket = gfNet_ConnectTo((const TChar *)server_url, server_port, SOCK_STREAM, IPPROTO_TCP);
    if (DUnlikely(0 > mSocket)) {
        LOG_ERROR("gfNet_ConnectTo(%s:%hu) fail\n", server_url, server_port);
        return EECode_NetConnectFail;
    }

    SDirectSharingHeader header;
    memset(&header, 0x0, sizeof(header));
    header.header_type = EDirectSharingHeaderType_Connect;

    TInt ret = gfNet_Send(mSocket, (TU8 *) &header, sizeof(header), 0);
    if (DUnlikely(((TInt)sizeof(header)) != ret)) {
        LOG_ERROR("send header fail, ret %d\n", ret);
        return EECode_NetSendHeader_Fail;
    }

    ret = gfNet_Recv_timeout(mSocket, (TU8 *) &header, sizeof(header), DNETUTILS_RECEIVE_FLAG_READ_ALL, 3);
    if (DUnlikely(((TInt)sizeof(header)) != ret)) {
        LOG_ERROR("read header fail, recv %d not expected, expect %lu\n", ret, (TULong)sizeof(header));
        return EECode_ServerReject_CorruptedProtocol;
    }

    TU8 status_code = header.header_flag & DDirectSharingHeaderFlagStatusCodeMask;
    if (EDirectSharingStatusCode_OK != status_code) {
        LOG_ERROR("request fail, ret %x\n", status_code);
        return EECode_ServerReject_Unknown;
    }

    return EECode_OK;
}

EDirectSharingStatusCode CIDirectSharingClient::QuerySharedResource(SSharedResource *&p_resource_list, TU32 &resource_number)
{
    if (DUnlikely(!DIsSocketHandlerValid(mSocket))) {
        LOG_FATAL("not connect yet?\n");
        return EDirectSharingStatusCode_BadState;
    }

    LOG_FATAL("to do\n");
    return EDirectSharingStatusCode_NotSuppopted;

    SDirectSharingHeader header;
    memset(&header, 0x0, sizeof(header));
    header.header_type = EDirectSharingHeaderType_QueryResource;

    TInt ret = gfNet_Send(mSocket, (TU8 *) &header, sizeof(header), 0);
    if (DUnlikely(((TInt)sizeof(header)) != ret)) {
        LOG_ERROR("send header fail, ret %d\n", ret);
        return EDirectSharingStatusCode_SendHeaderFail;
    }

#if 0
    ret = gfNet_Recv_timeout(mSocket, (TU8 *) &header, sizeof(header), DNETUTILS_RECEIVE_FLAG_READ_ALL, 10);
    if (DUnlikely(((TInt)sizeof(header)) != ret)) {
        LOG_ERROR("read header fail, recv %d not expected, expect %lu\n", ret, (TULong)sizeof(header));
        return EDirectSharingStatusCode_CorruptedProtocol;
    }

    TU8 status_code = header.header_flag & DDirectSharingHeaderFlagStatusCodeMask;
    if (EDirectSharingStatusCode_OK != status_code) {
        LOG_ERROR("request fail, ret %x\n", status_code);
        return (EDirectSharingStatusCode)status_code;
    }

    TU32 payload_length = header.payload_length_0;
#endif

    return EDirectSharingStatusCode_OK;
}

EDirectSharingStatusCode CIDirectSharingClient::RequestResource(TU8 type, TU16 index)
{
    if (DUnlikely(!DIsSocketHandlerValid(mSocket))) {
        LOG_FATAL("not connect yet?\n");
        return EDirectSharingStatusCode_BadState;
    }

    SDirectSharingHeader header;
    memset(&header, 0x0, sizeof(header));
    header.header_type = EDirectSharingHeaderType_RequestResource;
    header.val0 = type;
    header.val1 = (TU8)index;

    TInt ret = gfNet_Send(mSocket, (TU8 *) &header, sizeof(header), 0);
    if (DUnlikely(((TInt)sizeof(header)) != ret)) {
        LOG_ERROR("send header fail, ret %d\n", ret);
        return EDirectSharingStatusCode_SendHeaderFail;
    }

    ret = gfNet_Recv_timeout(mSocket, (TU8 *) &header, sizeof(header), DNETUTILS_RECEIVE_FLAG_READ_ALL, 3);
    if (DUnlikely(((TInt)sizeof(header)) != ret)) {
        LOG_ERROR("read header fail, recv %d not expected, expect %lu\n", ret, (TULong)sizeof(header));
        return EDirectSharingStatusCode_CorruptedProtocol;
    }

    TU8 status_code = header.header_flag & DDirectSharingHeaderFlagStatusCodeMask;
    if (EDirectSharingStatusCode_OK != status_code) {
        LOG_ERROR("request fail, ret %x\n", status_code);
        return (EDirectSharingStatusCode)status_code;
    }

    return EDirectSharingStatusCode_OK;
}

EDirectSharingStatusCode CIDirectSharingClient::ShareResource(SSharedResource *resource)
{
    LOG_FATAL("to do\n");
    return EDirectSharingStatusCode_NotSuppopted;
}

TSocketHandler CIDirectSharingClient::RetrieveSocketHandler()
{
    TSocketHandler ret = mSocket;
    mSocket = DInvalidSocketHandler;
    return ret;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


