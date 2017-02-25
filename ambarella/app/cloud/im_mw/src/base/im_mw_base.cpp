/*******************************************************************************
 * im_mw_base.cpp
 *
 * History:
 *  2014/06/11 - [Zhi He] create file
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
#include "im_mw_if.h"

#include "sacp_types.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

enum {
    ENetAgentState_Ready = 0,
    ENetAgentState_NotConnected,
    ENetAgentState_SocketError,
};

//-----------------------------------------------------------------------
//
// CNetworkAgent
//
//-----------------------------------------------------------------------

CNetworkAgent *CNetworkAgent::Create(IAccountManager *p_account_manager, SAccountInfoRoot *p_account, EProtocolType type)
{
    CNetworkAgent *result = new CNetworkAgent(p_account_manager, p_account);
    if ((result) && (EECode_OK != result->Construct(type))) {
        result->Destroy();
        result = NULL;
    }
    return result;
}

void CNetworkAgent::Destroy()
{
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    if (mpContectList) {
        delete mpContectList;
        mpContectList = NULL;
    }

    if (mpReadBuffer) {
        free(mpReadBuffer);
        mpReadBuffer = NULL;
    }

    if (mpProtocolHeaderParser) {
        mpProtocolHeaderParser->Destroy();
        mpProtocolHeaderParser = NULL;
    }

    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    delete this;
}

CNetworkAgent::CNetworkAgent(IAccountManager *p_account_manager, SAccountInfoRoot *p_account)
    : mpAccountManager(p_account_manager)
    , mpProtocolHeaderParser(NULL)
    , mpAccountInfo(p_account)
    , mpMutex(NULL)
    , mUniqueID(p_account->header.id)
    , mSocket(-1)
    , mpContectList(NULL)
    , mpReadBuffer(NULL)
    , mpCurrentReadPtr(NULL)
    , mnRemainingSize(0)
    , mpWriteBuffer(NULL)
    , mnWriteBufferLength(0)
    , mpIDListBuffer(NULL)
    , mnIDListBufferLength(0)
    , mbTobeMandatoryDelete(0)
{
}

CNetworkAgent::~CNetworkAgent()
{
    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    if (mpContectList) {
        delete mpContectList;
        mpContectList = NULL;
    }

    if (mpReadBuffer) {
        free(mpReadBuffer);
        mpReadBuffer = NULL;
    }

    if (mpWriteBuffer) {
        free(mpWriteBuffer);
        mpWriteBuffer = NULL;
    }

    if (mpIDListBuffer) {
        free(mpIDListBuffer);
        mpIDListBuffer = NULL;
    }

    if (mpProtocolHeaderParser) {
        mpProtocolHeaderParser->Destroy();
        mpProtocolHeaderParser = NULL;
    }
    //mpAccountInfo->p_agent = NULL;
}

EECode CNetworkAgent::Construct(EProtocolType type)
{
    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    mpContectList = new CIDoubleLinkedList();
    if (DUnlikely(!mpContectList)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    mpReadBuffer = (TU8 *) malloc(DSACP_MAX_PAYLOAD_LENGTH);
    if (DUnlikely(!mpReadBuffer)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    mpCurrentReadPtr = mpReadBuffer;
    mnRemainingSize = DSACP_MAX_PAYLOAD_LENGTH;

    mpWriteBuffer = (TU8 *) malloc(DSACP_MAX_PAYLOAD_LENGTH);
    if (DUnlikely(!mpWriteBuffer)) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }
    mnWriteBufferLength = DSACP_MAX_PAYLOAD_LENGTH;

    mpProtocolHeaderParser = gfCreatePotocolHeaderParser(type, EProtocolHeaderExtentionType_SACP_IM);
    if (DUnlikely(!mpProtocolHeaderParser)) {
        LOG_FATAL("gfCreatePotocolHeaderParser(%d) fail\n", type);
        return EECode_NotSupported;
    }

    mnHeaderLength = mpProtocolHeaderParser->GetFixedHeaderLength();

    return EECode_OK;
}

EAccountGroup CNetworkAgent::GetAccountGroup() const
{
    return (EAccountGroup)((mUniqueID & DExtIDTypeMask) >> DExtIDTypeShift);
}

TUniqueID CNetworkAgent::GetUniqueID() const
{
    return mUniqueID;
}


TSocketHandler CNetworkAgent::GetSocket() const
{
    return mSocket;
}

EECode CNetworkAgent::SetSocket(TSocketHandler socket)
{
    AUTO_LOCK(mpMutex);
    if (DUnlikely(DIsSocketHandlerValid(mSocket))) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    mSocket = socket;
    return EECode_OK;
}

EECode CNetworkAgent::SendData(TU8 *p_data, TU32 size)
{
    AUTO_LOCK(mpMutex);
    return sendData(p_data, size);
}

EECode CNetworkAgent::Scheduling(TUint times, TU32 inout_mask)
{
    AUTO_LOCK(mpMutex);

    TInt received = 0;
    EECode err = EECode_OK;
    TU32 total_size = 0;
    //TU8* p_parse = NULL;
    TU32 length = 0;
    TUniqueID target;

    //mpCurrentReadPtr = mpReadBuffer;
    //mnRemainingSize = DSACP_MAX_PAYLOAD_LENGTH;
    TU32 payload_size = 0, reqres_bits = 0, cat = 0, sub_type = 0;
    TU8 *p_payload = NULL;

    //LOG_NOTICE("Scheduling IN\n");
    if (DUnlikely(mbTobeMandatoryDelete)) {
        LOG_NOTICE("to be mandantory deleted\n");
        return EECode_Closed;
    }

    for (; ;) {

        err = EECode_OK;
        //LOG_NOTICE("gfNetDirect_Recv IN\n");
        received = gfNetDirect_Recv(mSocket, mpCurrentReadPtr, mnRemainingSize, err);
        //LOG_NOTICE("gfNetDirect_Recv OUT, received %d, mnRemainingSize %d\n", received, mnRemainingSize);
        if (EECode_Busy == err) {
            //LOG_NOTICE("Scheduling Quit normally!\n");
            return EECode_OK;
        } else if (DUnlikely(EECode_Closed == err)) {
            LOG_NOTICE("peer close, name [%s], password [%s], id [%llx], logout\n", mpAccountInfo->base.name, mpAccountInfo->base.password, mUniqueID);
            //gfNetCloseSocket(mSocket);
            //mSocket = DInvalidSocketHandler;
            notifyAllFriendsOffline();
            return EECode_Closed;
        } else if (DUnlikely(EECode_OK != err)) {
#ifdef BUILD_OS_WINDOWS
            //if (mpCurrentReadPtr == mpReadBuffer) {
            //     LOG_WARN("skip this case\n");
            //     return EECode_OK;
            //}
#endif
            LOG_ERROR("read socket %d, cur %ld\n", received, (TULong)(mpCurrentReadPtr - mpReadBuffer));
            notifyAllFriendsOffline();
            return EECode_Closed;
        }
        length = (mpCurrentReadPtr - mpReadBuffer) + received;

        if (DLikely(length >= mnHeaderLength)) {
            //LOG_NOTICE("before ParseWithTargetID\n");
            //gfPrintMemory(mpReadBuffer, mnHeaderLength);
            mpProtocolHeaderParser->ParseWithTargetID(mpReadBuffer, total_size, target);
            //LOG_NOTICE("after ParseWithTargetID, target %llx, total_size %d\n", target, total_size);
            //gfPrintMemory(mpReadBuffer, mnHeaderLength);
            if (length < total_size) {
                mpCurrentReadPtr += received;
                mnRemainingSize -= received;
                //LOG_NOTICE("continue read payload, received %d, cur %d, total_size %d, length %d\n", received, mpCurrentReadPtr - mpReadBuffer, total_size, length);
                continue;
            }
            //LOG_NOTICE("ParseWithTargetID, total_size: %u, target: %llx\n", total_size, target);

            if (target == 0/**/) {
                //request from client to im server
                mpProtocolHeaderParser->Parse(mpReadBuffer, payload_size, reqres_bits, cat, sub_type);
                //LOG_NOTICE("[DEBUG]: parse header, payload_size %d, total_size %d\n", payload_size, total_size);
                DASSERT(reqres_bits == DSACPTypeBit_Request);
                DASSERT(cat == ESACPTypeCategory_ClientCmdChannel);
                p_payload = mpReadBuffer + mnHeaderLength;
                memcpy(mpWriteBuffer, mpReadBuffer, mnHeaderLength);
                err = handleRequestFromClient(p_payload, payload_size, sub_type);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_FATAL("handleRequestFromClient fail\n");
                }
            } else {
                //msg from client to client
                __UNLOCK(mpMutex);
                err = relay(target, mUniqueID, mpReadBuffer, total_size);
                __LOCK(mpMutex);
                //LOG_NOTICE("[DEBUG]: msg relay (%llx to %llx), size %d\n", mUniqueID, target, total_size);
                //gfPrintMemory(mpReadBuffer, total_size);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_FATAL("relay fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                    //mpProtocolHeaderParser->FillSourceID(mpWriteBuffer, target);
                    notifyTargetNotReachable(target);
                }
            }

            if (length > total_size) {
                LOG_WARN("not one payload\n");
                memmove(mpReadBuffer, mpReadBuffer + total_size, length - total_size);
                mpCurrentReadPtr = mpReadBuffer + length - total_size;
                mnRemainingSize = DSACP_MAX_PAYLOAD_LENGTH - (length - total_size);
                continue;
            }
            DASSERT(length == total_size);
            mpCurrentReadPtr = mpReadBuffer;
            mnRemainingSize = DSACP_MAX_PAYLOAD_LENGTH;
            break;
        } else {
            mpCurrentReadPtr += received;
            mnRemainingSize -= received;
            //LOG_NOTICE("continue read header, received %d, cur %d\n", received, mpCurrentReadPtr - mpReadBuffer);
            continue;
        }

    }

    return EECode_OK;
}

TInt CNetworkAgent::IsPassiveMode() const
{
    return 0;
}

TSchedulingHandle CNetworkAgent::GetWaitHandle() const
{
    return mSocket;
}

TU8 CNetworkAgent::GetPriority() const
{
    return 1;
}

EECode CNetworkAgent::CheckTimeout()
{
    LOG_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CNetworkAgent::EventHandling(EECode err)
{
    if (EECode_MandantoryDelete == err) {
        if (DUnlikely(DIsSocketHandlerValid(mSocket))) {
            notifyToClient(ESACPRequestResult_Reject_AnotherLogin, mpReadBuffer, mnHeaderLength);
            DASSERT(!mbTobeMandatoryDelete);
            mbTobeMandatoryDelete = 1;
        }
        return EECode_OK;
    } else if (EECode_Closed == err) {
        if (DUnlikely(DIsSocketHandlerValid(mSocket))) {
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
        }
        return EECode_OK;
    }

    LOG_WARN("not handled, %d, %s\n", err, gfGetErrorCodeString(err));
    return EECode_NotSupported;
}

TSchedulingUnit CNetworkAgent::HungryScore() const
{
    return 1;
}

CNetworkAgent *CNetworkAgent::findTargetInRecentList(TUniqueID target)
{
    DASSERT(mpContectList);
    CIDoubleLinkedList::SNode *p_node = mpContectList->FirstNode();
    CNetworkAgent *p_target = NULL;

    while (p_node) {
        p_target = (CNetworkAgent *) p_node->p_context;
        if (p_target->GetUniqueID() == target) {
            return p_target;
        }
        p_node = mpContectList->NextNode(p_node);
    }

    return NULL;
}

EECode CNetworkAgent::relay(TUniqueID target, TUniqueID source, TU8 *p_data, TU32 size)
{
    //LOG_NOTICE("im server relay msg\n");
    CNetworkAgent *p_target = findTargetInRecentList(target);

    if (DUnlikely(!p_target)) {
        SAccountInfoRoot *info = NULL;
        EECode err = mpAccountManager->QueryAccount(target, info);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("not exist\n");
            return EECode_NotExist;
        }
        if (DLikely(info)) {
            if (!info->p_agent) {
                LOG_ERROR("not online, to do!\n");
                return EECode_NotOnline;
            }
            p_target = (CNetworkAgent *) info->p_agent;
        } else {
            LOG_FATAL("should not comes here\n");
            return EECode_InternalLogicalBug;
        }
    }
    //LOG_NOTICE("1\n");
    //gfPrintMemory(p_data, size);
    mpProtocolHeaderParser->FillSourceID(p_data, source);
    //LOG_NOTICE("2\n");
    //gfPrintMemory(p_data, size);
    return p_target->SendData(p_data, size);
}

EECode CNetworkAgent::sendData(TU8 *p_data, TU32 size)
{
    if (DLikely(0 < mSocket)) {
        TInt ret = gfNet_Send_timeout(mSocket, p_data, size, 0, 1);
        if (0 < ret) {
            return EECode_OK;
        } else if (DUnlikely(!ret)) {
            LOG_NOTICE("peer close\n");
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
            return EECode_Closed;
        }

        return EECode_NetSocketSend_Error;
    }

    return EECode_BadState;
}

EECode CNetworkAgent::handleRequestFromClient(TU8 *payload, TU16 payload_size, TU8 sub_type)
{
    STLV16HeaderBigEndian *p_tlv = (STLV16HeaderBigEndian *)payload;
    TU8 *p_cur = NULL;
    TU16 tlv_type = 0;
    TU16 tlv_length = 0;
    TU32 tot_length = 0;

    EECode err = EECode_OK;
    LOG_NOTICE("[DEBUG]: handleRequestFromClient, get subtype: %x, %s, payload_size %d\n", sub_type, gfGetSACPClientCmdSubtypeString((ESACPClientCmdSubType)sub_type), payload_size);

    switch (sub_type) {

        case ESACPClientCmdSubType_SinkOwnSourceDevice:

            if (DUnlikely(EAccountGroup_UserAccount != ((mUniqueID & DExtIDTypeMask) >> DExtIDTypeShift))) {
                LOG_FATAL("not user account? %llx, from attacker?\n", mUniqueID);
                replyToClient(EECode_PossibleAttackFromNetwork, mpWriteBuffer, mnHeaderLength);
                return EECode_PossibleAttackFromNetwork;
            }

            if (DLikely(payload_size > 0)) {
                TUniqueID device_id = 0;
                p_tlv = (STLV16HeaderBigEndian *)payload;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                tot_length = 0;
                if (DUnlikely(ETLV16Type_SourceDeviceProductCode != tlv_type)) {
                    LOG_FATAL("corrupted protocol\n");
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                DASSERT((sizeof(STLV16HeaderBigEndian) + tlv_length) == payload_size);
                p_cur = payload + sizeof(STLV16HeaderBigEndian);

                err = gfParseProductionCode((TChar *)p_cur, (TU32)tlv_length, device_id);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_FATAL("Bad production code\n");
                    replyToClient(err, mpWriteBuffer, mnHeaderLength);
                    return err;
                }

                err = mpAccountManager->OwnSourceDeviceByUserContext((SAccountInfoUser *) mpAccountInfo, device_id);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_FATAL("own device fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                    replyToClient(err, mpWriteBuffer, mnHeaderLength);
                    return err;
                }

                tot_length = 0;
                p_cur = mpWriteBuffer + mnHeaderLength;
                p_tlv = (STLV16HeaderBigEndian *) p_cur;
                tlv_length = sizeof(TUniqueID);
                p_tlv->type_high = ((TU16)ETLV16Type_AccountID >> 8) & 0xff;
                p_tlv->type_low = ((TU16)ETLV16Type_AccountID) & 0xff;
                p_tlv->length_high = ((TU16)(tlv_length) >> 8) & 0xff;
                p_tlv->length_low = ((TU16)(tlv_length)) & 0xff;
                p_cur += sizeof(STLV16HeaderBigEndian);
                tot_length += sizeof(STLV16HeaderBigEndian);
                DBEW64(device_id, p_cur);
                p_cur += tlv_length;
                tot_length += tlv_length;
                LOG_NOTICE("acquire device id %llx\n", device_id);
                //gfPrintMemory(mpWriteBuffer + mnHeaderLength, tot_length);
                replyToClient(EECode_OK, mpWriteBuffer, tot_length + mnHeaderLength);
                DSetSACPType(DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_SystemNotification, ESACPClientCmdSubType_SystemNotifyDeviceOwnedByUser), mpWriteBuffer);
                relay(device_id, 0, mpWriteBuffer, mnHeaderLength + tot_length);
            } else {
                LOG_FATAL("ESACPClientCmdSubType_SinkOwnSourceDevice: payload size is 0!\n");
                replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                return EECode_ServerReject_CorruptedProtocol;
            }
            break;

        case ESACPClientCmdSubType_SourceSetOwner:
            if (DUnlikely(EAccountGroup_SourceDevice != ((mUniqueID & DExtIDTypeMask) >> DExtIDTypeShift))) {
                LOG_FATAL("not source device account? %llx, from attacker?\n", mUniqueID);
                replyToClient(EECode_PossibleAttackFromNetwork, mpWriteBuffer, mnHeaderLength);
                return EECode_PossibleAttackFromNetwork;
            }
            if (DLikely(payload_size > 0)) {
                p_tlv = (STLV16HeaderBigEndian *)payload;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely((ETLV16Type_AccountName != tlv_type) || (DMAX_ACCOUNT_NAME_LENGTH_EX < tlv_length))) {
                    LOG_FATAL("corrupted protocol, type %x, length %d\n", tlv_type, tlv_length);
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                DASSERT((sizeof(STLV16HeaderBigEndian) + tlv_length) == payload_size);
                p_cur = payload + sizeof(STLV16HeaderBigEndian);

                SAccountInfoUser *p_user = NULL;
                err = mpAccountManager->QueryUserAccountByName((TChar *)p_cur, p_user);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_FATAL("no such user %s, ret %d, %s\n", p_cur, err, gfGetErrorCodeString(err));
                    replyToClient(EECode_ServerReject_NoSuchChannel, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_NoSuchChannel;
                }

                err = mpAccountManager->SourceSetOwner((SAccountInfoSourceDevice *)mpAccountInfo, p_user);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_FATAL("[%s, %llx] set owner fail, user [%s, %llx], ret %d, %s\n", mpAccountInfo->base.name, mpAccountInfo->header.id, p_user->root.base.name, p_user->root.header.id, err, gfGetErrorCodeString(err));
                    replyToClient(err, mpWriteBuffer, mnHeaderLength);
                    return err;
                }

                LOG_NOTICE("[%s, %llx] set owner done, user [%s, %llx]\n", mpAccountInfo->base.name, mpAccountInfo->header.id, p_user->root.base.name, p_user->root.header.id);
                replyToClient(EECode_OK, mpWriteBuffer, mnHeaderLength);
                DSetSACPType(DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_SystemNotification, ESACPClientCmdSubType_SystemNotifyUpdateOwnedDeviceList), mpWriteBuffer);
                relay(p_user->root.header.id, 0, mpWriteBuffer, mnHeaderLength);
                return EECode_OK;
            } else {
                LOG_FATAL("ESACPClientCmdSubType_SourceSetOwner: payload size is 0!\n");
                replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                return EECode_ServerReject_CorruptedProtocol;
            }
            break;

        case ESACPClientCmdSubType_SinkQuerySource:
            if (DUnlikely(EAccountGroup_UserAccount != ((mUniqueID & DExtIDTypeMask) >> DExtIDTypeShift))) {
                LOG_FATAL("not user account? %llx, from attacker?\n", mUniqueID);
                replyToClient(EECode_PossibleAttackFromNetwork, mpWriteBuffer, mnHeaderLength);
                return EECode_PossibleAttackFromNetwork;
            }
            if (DLikely(payload_size > 0)) {
                TUniqueID device_id = 0;
                p_tlv = (STLV16HeaderBigEndian *)payload;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely((ETLV16Type_AccountID != tlv_type) || (sizeof(TUniqueID) != tlv_length))) {
                    LOG_FATAL("corrupted protocol\n");
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                p_cur = payload + sizeof(STLV16HeaderBigEndian);
                DBER64(device_id, p_cur);

                SAccountInfoSourceDevice *p_device = NULL;
                err = mpAccountManager->PrivilegeQueryDevice(mUniqueID, device_id, p_device);
                if (DUnlikely(EECode_OK != err)) {
                    DASSERT(!p_device);
                    replyToClient(err, mpWriteBuffer, mnHeaderLength);
                    return err;
                }
                DASSERT(p_device);

                TU32 datanode_index = (device_id & DExtIDDataNodeMask) >> DExtIDDataNodeShift;
                SAccountInfoCloudDataNode *p_data_node = NULL;
                err = mpAccountManager->QueryDataNodeInfoByIndex(datanode_index, p_data_node);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_FATAL("QueryDataNodeInfoByIndex fail, index %d\n", datanode_index);
                    replyToClient(err, mpWriteBuffer, mnHeaderLength);
                    return err;
                }

                p_cur = mpWriteBuffer + mnHeaderLength;
                tot_length = 0;

                p_tlv = (STLV16HeaderBigEndian *) p_cur;
                tlv_length = strlen(p_data_node->ext.url);
                p_tlv->type_high = ((TU16)ETLV16Type_SourceDeviceDataServerAddress >> 8) & 0xff;
                p_tlv->type_low = ((TU16)ETLV16Type_SourceDeviceDataServerAddress) & 0xff;
                p_tlv->length_high = ((TU16)(tlv_length + 1) >> 8) & 0xff;
                p_tlv->length_low = ((TU16)(tlv_length + 1)) & 0xff;
                p_cur += sizeof(STLV16HeaderBigEndian);
                tot_length += sizeof(STLV16HeaderBigEndian);
                memcpy(p_cur, p_data_node->ext.url, tlv_length);
                p_cur[tlv_length] = 0x0;
                p_cur += tlv_length + 1;
                tot_length += tlv_length + 1;

                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_length = strlen(p_device->root.base.name);
                p_tlv->type_high = ((TU16)ETLV16Type_SourceDeviceStreamingTag >> 8) & 0xff;
                p_tlv->type_low = ((TU16)ETLV16Type_SourceDeviceStreamingTag) & 0xff;
                p_tlv->length_high = ((TU16)(tlv_length + 1) >> 8) & 0xff;
                p_tlv->length_low = ((TU16)(tlv_length + 1)) & 0xff;
                p_cur += sizeof(STLV16HeaderBigEndian);
                tot_length += sizeof(STLV16HeaderBigEndian);
                memcpy(p_cur, p_device->root.base.name, tlv_length);
                p_cur[tlv_length] = 0x0;
                p_cur += tlv_length + 1;
                tot_length += tlv_length + 1;

                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_length = sizeof(TSocketPort);
                p_tlv->type_high = ((TU16)ETLV16Type_SourceDeviceStreamingPort >> 8) & 0xff;
                p_tlv->type_low = ((TU16)ETLV16Type_SourceDeviceStreamingPort) & 0xff;
                p_tlv->length_high = ((TU16)(tlv_length) >> 8) & 0xff;
                p_tlv->length_low = ((TU16)(tlv_length)) & 0xff;
                p_cur += sizeof(STLV16HeaderBigEndian);
                tot_length += sizeof(STLV16HeaderBigEndian);
                DBEW16(p_data_node->ext.rtsp_streaming_port, p_cur);
                p_cur += tlv_length;
                tot_length += tlv_length;

                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_length = DDynamicInputStringLength;
                p_tlv->type_high = ((TU16)ETLV16Type_DynamicInput >> 8) & 0xff;
                p_tlv->type_low = ((TU16)ETLV16Type_DynamicInput) & 0xff;
                p_tlv->length_high = ((TU16)(tlv_length) >> 8) & 0xff;
                p_tlv->length_low = ((TU16)(tlv_length)) & 0xff;
                p_cur += sizeof(STLV16HeaderBigEndian);
                tot_length += sizeof(STLV16HeaderBigEndian);
                TU8 randomseed[64] = {0};
                randomseed[sizeof(SDateTime)] = 0x01;
                randomseed[sizeof(SDateTime) + 1] = 0xab;
                randomseed[sizeof(SDateTime) + 2] = 0x56;
                randomseed[sizeof(SDateTime) + 3] = 0xef;
                gfGetCurrentDateTime((SDateTime *)randomseed);
                TU64 dynamic_input = gfHashAlgorithmV5(randomseed, sizeof(SDateTime) + 4);
                DBEW64(dynamic_input, p_cur);
                p_cur += tlv_length;
                tot_length += tlv_length;

                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_length = strlen(p_device->root.base.nickname);
                p_tlv->type_high = ((TU16)ETLV16Type_DeviceNickName >> 8) & 0xff;
                p_tlv->type_low = ((TU16)ETLV16Type_DeviceNickName) & 0xff;
                p_tlv->length_high = ((TU16)(tlv_length + 1) >> 8) & 0xff;
                p_tlv->length_low = ((TU16)(tlv_length + 1)) & 0xff;
                p_cur += sizeof(STLV16HeaderBigEndian);
                tot_length += sizeof(STLV16HeaderBigEndian);
                memcpy(p_cur, p_device->root.base.nickname, tlv_length);
                p_cur[tlv_length] = 0x0;
                p_cur += tlv_length + 1;
                tot_length += tlv_length + 1;

                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_length = 1;
                p_tlv->type_high = ((TU16)ETLV16Type_DeviceStatus >> 8) & 0xff;
                p_tlv->type_low = ((TU16)ETLV16Type_DeviceStatus) & 0xff;
                p_tlv->length_high = 0;
                p_tlv->length_low = 1;
                p_cur += sizeof(STLV16HeaderBigEndian);
                tot_length += sizeof(STLV16HeaderBigEndian);
                if (p_device->root.p_agent) {
                    p_cur[0] = EDeviceStatus_streaming;//to do
                } else {
                    p_cur[0] = EDeviceStatus_offline;
                }
                p_cur ++;
                tot_length += 1;

                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_length = sizeof(TU32);
                p_tlv->type_high = ((TU16)ETLV16Type_DeviceStorageCapacity >> 8) & 0xff;
                p_tlv->type_low = ((TU16)ETLV16Type_DeviceStorageCapacity) & 0xff;
                p_tlv->length_high = ((TU16)(tlv_length) >> 8) & 0xff;
                p_tlv->length_low = ((TU16)(tlv_length)) & 0xff;
                p_cur += sizeof(STLV16HeaderBigEndian);
                tot_length += sizeof(STLV16HeaderBigEndian);
                DBEW32(p_device->ext.storage_capacity, p_cur);
                p_cur += tlv_length;
                tot_length += tlv_length;

                replyToClient(EECode_OK, mpWriteBuffer, mnHeaderLength + tot_length);
                return EECode_OK;
            } else {
                LOG_FATAL("ESACPClientCmdSubType_SinkQuerySource: payload size is 0!\n");
                replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                return EECode_ServerReject_CorruptedProtocol;
            }
            break;

        case ESACPClientCmdSubType_SinkQueryFriendInfo:
            if (DUnlikely(EAccountGroup_UserAccount != ((mUniqueID & DExtIDTypeMask) >> DExtIDTypeShift))) {
                LOG_FATAL("not user account? %llx, from attacker?\n", mUniqueID);
                replyToClient(EECode_PossibleAttackFromNetwork, mpWriteBuffer, mnHeaderLength);
                return EECode_PossibleAttackFromNetwork;
            }
            if (DLikely(payload_size > 0)) {
                TUniqueID friend_id = 0;
                p_tlv = (STLV16HeaderBigEndian *)payload;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely((ETLV16Type_AccountID != tlv_type) || (sizeof(TUniqueID) != tlv_length))) {
                    LOG_FATAL("corrupted protocol\n");
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                p_cur = payload + sizeof(STLV16HeaderBigEndian);
                DBER64(friend_id, p_cur);

                SAccountInfoUser *p_friend = NULL;
                err = mpAccountManager->PrivilegeQueryUser((SAccountInfoUser *) mpAccountInfo, friend_id, p_friend);
                if (DUnlikely(EECode_OK != err)) {
                    DASSERT(!p_friend);
                    replyToClient(err, mpWriteBuffer, mnHeaderLength);
                    return err;
                }
                DASSERT(p_friend);

                p_cur = mpWriteBuffer + mnHeaderLength;
                tot_length = 0;

                p_tlv = (STLV16HeaderBigEndian *) p_cur;
                p_tlv->type_high = ((TU16)ETLV16Type_UserOnline >> 8) & 0xff;
                p_tlv->type_low = ((TU16)ETLV16Type_UserOnline) & 0xff;
                p_tlv->length_high = 0;
                p_tlv->length_low = 1;
                p_cur += sizeof(STLV16HeaderBigEndian);
                tot_length += sizeof(STLV16HeaderBigEndian);
                if ((p_friend->root.p_agent) && (DInvalidSocketHandler != (((CNetworkAgent *)p_friend->root.p_agent)->GetSocket()))) {
                    p_cur[0] = 0x1;
                } else {
                    p_cur[0] = 0x0;
                }
                p_cur += 1;
                tot_length += 1;

                replyToClient(EECode_OK, mpWriteBuffer, mnHeaderLength + tot_length);
                return EECode_OK;
            } else {
                LOG_FATAL("ESACPClientCmdSubType_SinkQueryFriendInfo: payload size is 0!\n");
                replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                return EECode_ServerReject_CorruptedProtocol;
            }
            break;

        case ESACPClientCmdSubType_SinkQuerySourcePrivilege:
            if (DUnlikely(EAccountGroup_UserAccount != ((mUniqueID & DExtIDTypeMask) >> DExtIDTypeShift))) {
                LOG_FATAL("not user account? %llx, from attacker?\n", mUniqueID);
                replyToClient(EECode_PossibleAttackFromNetwork, mpWriteBuffer, mnHeaderLength);
                return EECode_PossibleAttackFromNetwork;
            }
            LOG_FATAL("ESACPClientCmdSubType_SinkQuerySourcePrivilege: TODO\n");
            break;

        case ESACPClientCmdSubType_SinkInviteFriend:
            if (DUnlikely(EAccountGroup_UserAccount != ((mUniqueID & DExtIDTypeMask) >> DExtIDTypeShift))) {
                LOG_FATAL("not user account? %llx, from attacker?\n", mUniqueID);
                replyToClient(EECode_PossibleAttackFromNetwork, mpWriteBuffer, mnHeaderLength);
                return EECode_PossibleAttackFromNetwork;
            }
            if (DLikely(payload_size > (sizeof(STLV16HeaderBigEndian)))) {
                TUniqueID friend_id = DInvalidUniqueID;
                p_tlv = (STLV16HeaderBigEndian *)payload;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely((ETLV16Type_AccountName != tlv_type) || (DMAX_ACCOUNT_NAME_LENGTH_EX <= tlv_length))) {
                    LOG_FATAL("corrupted protocol, tlv_type %x, tlv_length %d\n", tlv_type, tlv_length);
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                p_cur = payload + sizeof(STLV16HeaderBigEndian);
                TChar friend_name[DMAX_ACCOUNT_NAME_LENGTH_EX] = {0};
                memcpy(friend_name, p_cur, tlv_length);

                err = mpAccountManager->InviteFriend((SAccountInfoUser *)mpAccountInfo, friend_name, friend_id);
                if (DUnlikely((EECode_OK != err)) && (EECode_AlreadyExist != err)) {
                    LOG_FATAL("invite friend fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                    replyToClient(err, mpWriteBuffer, mnHeaderLength);
                    return err;
                } else {
                    LOG_NOTICE("[DEBUG]: add friend, %llx, [%s]\n", friend_id, friend_name);
                }

                p_cur = mpWriteBuffer + mnHeaderLength;

                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_length = sizeof(TUniqueID);
                p_tlv->type_high = ((TU16)ETLV16Type_AccountID >> 8) & 0xff;
                p_tlv->type_low = ((TU16)ETLV16Type_AccountID) & 0xff;
                p_tlv->length_high = ((TU16)(tlv_length) >> 8) & 0xff;
                p_tlv->length_low = ((TU16)(tlv_length)) & 0xff;
                p_cur += sizeof(STLV16HeaderBigEndian);
                DBEW64(friend_id, p_cur);

                replyToClient(err, mpWriteBuffer, mnHeaderLength + tlv_length + sizeof(STLV16HeaderBigEndian));
                if (EECode_OK == err) {
                    DSetSACPType(DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_SystemNotification, ESACPClientCmdSubType_SystemNotifyFriendInvitation), mpWriteBuffer);

                    p_cur = mpWriteBuffer + mnHeaderLength;
                    TU32 tot_size = 0;
                    p_tlv = (STLV16HeaderBigEndian *)p_cur;
                    tlv_length = sizeof(TUniqueID);
                    p_tlv->type_high = ((TU16)ETLV16Type_AccountID >> 8) & 0xff;
                    p_tlv->type_low = ((TU16)ETLV16Type_AccountID) & 0xff;
                    p_tlv->length_high = ((TU16)(tlv_length) >> 8) & 0xff;
                    p_tlv->length_low = ((TU16)(tlv_length)) & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    DBEW64(mUniqueID, p_cur);
                    p_cur += tlv_length;
                    tot_size += sizeof(STLV16HeaderBigEndian) + tlv_length;

                    p_tlv = (STLV16HeaderBigEndian *)p_cur;
                    tlv_length = strlen(mpAccountInfo->base.name);
                    p_tlv->type_high = ((TU16)ETLV16Type_AccountName >> 8) & 0xff;
                    p_tlv->type_low = ((TU16)ETLV16Type_AccountName) & 0xff;
                    p_tlv->length_high = ((TU16)(tlv_length + 1) >> 8) & 0xff;
                    p_tlv->length_low = ((TU16)(tlv_length + 1)) & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    memcpy(p_cur, mpAccountInfo->base.name, tlv_length);
                    p_cur += tlv_length;
                    p_cur[0] = 0x0;
                    tot_size += sizeof(STLV16HeaderBigEndian) + tlv_length + 1;

                    //LOG_NOTICE("tot_size %d\n", tot_size);
                    //gfPrintMemory(mpWriteBuffer + mnHeaderLength, tot_size);

                    mpProtocolHeaderParser->SetPayloadSize(mpWriteBuffer, tot_size);
                    relay(friend_id, 0, mpWriteBuffer, mnHeaderLength + tot_size);
                } else {
                    LOG_WARN("already exist\n");
                    err = EECode_OK;
                }
            } else {
                LOG_FATAL("ESACPClientCmdSubType_SinkInviteFriend: payload size is 0!\n");
                err = EECode_BadMsg;
            }
            break;

        case ESACPClientCmdSubType_SinkInviteFriendByID:
            if (DUnlikely(EAccountGroup_UserAccount != ((mUniqueID & DExtIDTypeMask) >> DExtIDTypeShift))) {
                LOG_FATAL("not user account? %llx, from attacker?\n", mUniqueID);
                replyToClient(EECode_PossibleAttackFromNetwork, mpWriteBuffer, mnHeaderLength);
                return EECode_PossibleAttackFromNetwork;
            }
            if (DLikely(payload_size > (sizeof(STLV16HeaderBigEndian)))) {
                TUniqueID friend_id = DInvalidUniqueID;
                p_tlv = (STLV16HeaderBigEndian *)payload;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely((ETLV16Type_AccountID != tlv_type) || (sizeof(TUniqueID) != tlv_length))) {
                    LOG_FATAL("corrupted protocol, tlv_type %x, tlv_length %d\n", tlv_type, tlv_length);
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                p_cur = payload + sizeof(STLV16HeaderBigEndian);
                DBER64(friend_id, p_cur);
                TChar friend_name[DMAX_ACCOUNT_NAME_LENGTH_EX] = {0};

                err = mpAccountManager->InviteFriendByID((SAccountInfoUser *)mpAccountInfo, friend_id, friend_name);
                if (DUnlikely((EECode_OK != err) && (EECode_AlreadyExist != err))) {
                    LOG_FATAL("invite friend by id fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                    replyToClient(err, mpWriteBuffer, mnHeaderLength);
                    return err;
                } else {
                    LOG_NOTICE("[DEBUG]: add friend id %llx, name [%s]\n", friend_id, friend_name);
                }

                p_cur = mpWriteBuffer + mnHeaderLength;

                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_length = strlen(friend_name) + 1;
                p_tlv->type_high = ((TU16)ETLV16Type_AccountName >> 8) & 0xff;
                p_tlv->type_low = ((TU16)ETLV16Type_AccountName) & 0xff;
                p_tlv->length_high = ((TU16)(tlv_length) >> 8) & 0xff;
                p_tlv->length_low = ((TU16)(tlv_length)) & 0xff;
                p_cur += sizeof(STLV16HeaderBigEndian);
                memcpy(p_cur, friend_name, tlv_length);

                replyToClient(err, mpWriteBuffer, mnHeaderLength + tlv_length + sizeof(STLV16HeaderBigEndian));
                if (EECode_OK == err) {
                    DSetSACPType(DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_SystemNotification, ESACPClientCmdSubType_SystemNotifyFriendInvitation), mpWriteBuffer);

                    p_cur = mpWriteBuffer + mnHeaderLength;
                    TU32 tot_size = 0;
                    p_tlv = (STLV16HeaderBigEndian *)p_cur;
                    tlv_length = sizeof(TUniqueID);
                    p_tlv->type_high = ((TU16)ETLV16Type_AccountID >> 8) & 0xff;
                    p_tlv->type_low = ((TU16)ETLV16Type_AccountID) & 0xff;
                    p_tlv->length_high = ((TU16)(tlv_length) >> 8) & 0xff;
                    p_tlv->length_low = ((TU16)(tlv_length)) & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    DBEW64(mUniqueID, p_cur);
                    p_cur += tlv_length;
                    tot_size += sizeof(STLV16HeaderBigEndian) + tlv_length;

                    p_tlv = (STLV16HeaderBigEndian *)p_cur;
                    tlv_length = strlen(mpAccountInfo->base.name);
                    p_tlv->type_high = ((TU16)ETLV16Type_AccountName >> 8) & 0xff;
                    p_tlv->type_low = ((TU16)ETLV16Type_AccountName) & 0xff;
                    p_tlv->length_high = ((TU16)(tlv_length + 1) >> 8) & 0xff;
                    p_tlv->length_low = ((TU16)(tlv_length + 1)) & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    memcpy(p_cur, mpAccountInfo->base.name, tlv_length);
                    p_cur += tlv_length;
                    p_cur[0] = 0x0;
                    tot_size += sizeof(STLV16HeaderBigEndian) + tlv_length + 1;

                    //LOG_NOTICE("tot_size %d\n", tot_size);
                    //gfPrintMemory(mpWriteBuffer + mnHeaderLength, tot_size);

                    mpProtocolHeaderParser->SetPayloadSize(mpWriteBuffer, tot_size);
                    relay(friend_id, 0, mpWriteBuffer, mnHeaderLength + tot_size);
                } else {
                    LOG_WARN("already exist\n");
                    err = EECode_OK;
                }
            } else {
                LOG_FATAL("ESACPClientCmdSubType_SinkInviteFriendByID: payload size is 0!\n");
                err = EECode_BadMsg;
            }
            break;

        case ESACPClientCmdSubType_SinkAcceptFriend:
            if (DUnlikely(EAccountGroup_UserAccount != ((mUniqueID & DExtIDTypeMask) >> DExtIDTypeShift))) {
                LOG_FATAL("not user account? %llx, from attacker?\n", mUniqueID);
                replyToClient(EECode_PossibleAttackFromNetwork, mpWriteBuffer, mnHeaderLength);
                return EECode_PossibleAttackFromNetwork;
            }
            if (DLikely(payload_size > 0)) {
                TUniqueID friend_id = 0;
                p_tlv = (STLV16HeaderBigEndian *)payload;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);

                if (DUnlikely((ETLV16Type_AccountID != tlv_type) || (sizeof(TUniqueID) != tlv_length))) {
                    LOG_FATAL("corrupted protocol, tlv_type %x, tlv_length %d\n", tlv_type, tlv_length);
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                p_cur = payload + sizeof(STLV16HeaderBigEndian);
                DBER64(friend_id, p_cur);

                err = mpAccountManager->AcceptFriend((SAccountInfoUser *)mpAccountInfo, friend_id);
                if (DUnlikely(EECode_OK != err)) {
                    replyToClient(err, mpWriteBuffer, mnHeaderLength);
                    return err;
                }

                replyToClient(err, mpWriteBuffer, mnHeaderLength);
            }  else {
                LOG_FATAL("ESACPClientCmdSubType_SinkAcceptFriend: payload size is 0!\n");
                err = EECode_BadMsg;
            }
            break;

        case ESACPClientCmdSubType_SinkRemoveFriend:
            if (DUnlikely(EAccountGroup_UserAccount != ((mUniqueID & DExtIDTypeMask) >> DExtIDTypeShift))) {
                LOG_FATAL("not user account? %llx, from attacker?\n", mUniqueID);
                replyToClient(EECode_PossibleAttackFromNetwork, mpWriteBuffer, mnHeaderLength);
                return EECode_PossibleAttackFromNetwork;
            }
            if (DLikely(payload_size > 0)) {
                TUniqueID friend_id = 0;
                p_tlv = (STLV16HeaderBigEndian *)payload;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);

                if (DUnlikely((ETLV16Type_AccountID != tlv_type) || (sizeof(TUniqueID) != tlv_length))) {
                    LOG_FATAL("corrupted protocol, tlv_type %x, tlv_length %d\n", tlv_type, tlv_length);
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                p_cur = payload + sizeof(STLV16HeaderBigEndian);
                DBER64(friend_id, p_cur);

                err = mpAccountManager->RemoveFriend((SAccountInfoUser *)mpAccountInfo, friend_id);
                if (DUnlikely(EECode_OK != err)) {
                    replyToClient(err, mpWriteBuffer, mnHeaderLength);
                    return err;
                }

                replyToClient(err, mpWriteBuffer, mnHeaderLength);
            }  else {
                LOG_FATAL("ESACPClientCmdSubType_SinkRemoveFriend: payload size is 0!\n");
                err = EECode_BadMsg;
            }
            break;

        case ESACPClientCmdSubType_SinkUpdateFriendPrivilege:
            if (DUnlikely(EAccountGroup_UserAccount != ((mUniqueID & DExtIDTypeMask) >> DExtIDTypeShift))) {
                LOG_FATAL("not user account? %llx, from attacker?\n", mUniqueID);
                replyToClient(EECode_PossibleAttackFromNetwork, mpWriteBuffer, mnHeaderLength);
                return EECode_PossibleAttackFromNetwork;
            }
            {
                TUniqueID share_dev_id = 0, share_user_id = 0;
                TU32 privilege_mask = 0;

                p_tlv = (STLV16HeaderBigEndian *)payload;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);

                if (DUnlikely((ETLV16Type_AccountID != tlv_type) || (sizeof(TUniqueID) != tlv_length))) {
                    LOG_FATAL("corrupted protocol, tlv_type %x, tlv_length %d\n", tlv_type, tlv_length);
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                p_cur = payload + sizeof(STLV16HeaderBigEndian);
                DBER64(share_user_id, p_cur);

                if (DUnlikely(EAccountGroup_UserAccount != ((share_user_id & DExtIDTypeMask) >> DExtIDTypeShift))) {
                    LOG_ERROR("not user account? %llx\n", share_user_id);
                    replyToClient(EECode_BadParam, mpWriteBuffer, mnHeaderLength);
                    return EECode_BadParam;
                }
                p_cur += tlv_length;
                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely((ETLV16Type_AccountID != tlv_type) || (sizeof(TUniqueID) != tlv_length))) {
                    LOG_FATAL("corrupted protocol, tlv_type %x, tlv_length %d\n", tlv_type, tlv_length);
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }

                p_cur += sizeof(STLV16HeaderBigEndian);
                DBER64(share_dev_id, p_cur);
                if (DUnlikely(EAccountGroup_SourceDevice != ((share_dev_id & DExtIDTypeMask) >> DExtIDTypeShift))) {
                    LOG_ERROR("not device account? %llx\n", share_dev_id);
                    replyToClient(EECode_BadParam, mpWriteBuffer, mnHeaderLength);
                    return EECode_BadParam;
                }

                p_cur += tlv_length;
                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely((ETLV16Type_UserPrivilegeMask != tlv_type) || (sizeof(TU32) != tlv_length))) {
                    LOG_FATAL("corrupted protocol, tlv_type %x, tlv_length %d\n", tlv_type, tlv_length);
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                p_cur += sizeof(STLV16HeaderBigEndian);
                DBER32(privilege_mask, p_cur);

                err = mpAccountManager->UpdateUserPrivilegeSingleDeviceByContext((SAccountInfoUser *)mpAccountInfo, share_user_id, privilege_mask, share_dev_id);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("UpdateUserPrivilegeSingleDevice(%llx, %llx, %x) fail, ret %d, %s\n", share_user_id, share_dev_id, privilege_mask, err, gfGetErrorCodeString(err));
                }
                replyToClient(err, mpWriteBuffer, mnHeaderLength);
                return err;
            }
            break;

        case ESACPClientCmdSubType_SinkRetrieveFriendsList:
            if (DUnlikely(EAccountGroup_UserAccount != ((mUniqueID & DExtIDTypeMask) >> DExtIDTypeShift))) {
                LOG_FATAL("not user account? %llx, from attacker?\n", mUniqueID);
                replyToClient(EECode_PossibleAttackFromNetwork, mpWriteBuffer, mnHeaderLength);
                return EECode_PossibleAttackFromNetwork;
            }
            if (DLikely(0 == payload_size)) {
                TUniqueID *p_id_list = NULL;
                TU32 count = 0, i = 0;
                err = mpAccountManager->RetrieveUserFriendList(mUniqueID, count, p_id_list);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_FATAL("ESACPClientCmdSubType_SinkRetrieveFriendsList fail\n");
                    break;
                }

                if (count && p_id_list) {

                    TU32 length = count * sizeof(TUniqueID);
                    DASSERT(length < (1 << 16));

                    if (mnWriteBufferLength < (mnHeaderLength + length + sizeof(STLV16HeaderBigEndian))) {
                        if (mpWriteBuffer) {
                            free(mpWriteBuffer);
                        }
                        mpWriteBuffer = (TU8 *)malloc(mnHeaderLength + length + sizeof(STLV16HeaderBigEndian) + 4);
                        if (DUnlikely(!mpWriteBuffer)) {
                            LOG_FATAL("No Memory!\n");
                            err = EECode_NoMemory;
                            break;
                        }
                        mnWriteBufferLength = mnHeaderLength + length + sizeof(STLV16HeaderBigEndian) + 4;
                    }

                    p_cur = mpWriteBuffer + mnHeaderLength;

                    p_tlv = (STLV16HeaderBigEndian *)p_cur;
                    tlv_length = length;
                    p_tlv->type_high = ((TU16)ETLV16Type_UserFriendList >> 8) & 0xff;
                    p_tlv->type_low = ((TU16)ETLV16Type_UserFriendList) & 0xff;
                    p_tlv->length_high = ((TU16)(tlv_length) >> 8) & 0xff;
                    p_tlv->length_low = ((TU16)(tlv_length)) & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);

                    LOG_NOTICE("[%llx]'s friend list:\n", mUniqueID);
                    for (i = 0; i < count; i++) {
                        DBEW64(p_id_list[i], p_cur);
                        LOG_PRINTF("id[%u], %llx\n", i, p_id_list[i]);
                        p_cur += sizeof(TUniqueID);
                    }
                    length += sizeof(STLV16HeaderBigEndian);

                    replyToClient(EECode_OK, mpWriteBuffer, mnHeaderLength + length);
                } else {
                    p_cur = mpWriteBuffer + mnHeaderLength;

                    p_tlv = (STLV16HeaderBigEndian *)p_cur;
                    tlv_length = 0;
                    p_tlv->type_high = ((TU16)ETLV16Type_UserFriendList >> 8) & 0xff;
                    p_tlv->type_low = ((TU16)ETLV16Type_UserFriendList) & 0xff;
                    p_tlv->length_high = ((TU16)(tlv_length) >> 8) & 0xff;
                    p_tlv->length_low = ((TU16)(tlv_length)) & 0xff;
                    LOG_NOTICE("[%llx] no friend\n", mUniqueID);
                    replyToClient(EECode_OK, mpWriteBuffer, mnHeaderLength + sizeof(STLV16HeaderBigEndian));
                }

                return EECode_OK;
            } else {
                LOG_FATAL("retrieve id list: payload size is 0!\n");
                replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                return EECode_ServerReject_CorruptedProtocol;
            }
            break;

        case ESACPClientCmdSubType_SinkRetrieveAdminDeviceList:
            if (DUnlikely(EAccountGroup_UserAccount != ((mUniqueID & DExtIDTypeMask) >> DExtIDTypeShift))) {
                LOG_FATAL("not user account? %llx, from attacker?\n", mUniqueID);
                replyToClient(EECode_PossibleAttackFromNetwork, mpWriteBuffer, mnHeaderLength);
                return EECode_PossibleAttackFromNetwork;
            }
            if (DLikely(0 == payload_size)) {
                TUniqueID *p_id_list = NULL;
                TU32 count = 0, i = 0;
                err = mpAccountManager->RetrieveUserOwnedDeviceList(mUniqueID, count, p_id_list);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_FATAL("ESACPClientCmdSubType_SinkRetrieveAdminDeviceList fail\n");
                    break;
                }

                if (count && p_id_list) {

                    TU32 length = count * sizeof(TUniqueID);
                    DASSERT(length < (1 << 16));

                    if (mnWriteBufferLength < (mnHeaderLength + length + sizeof(STLV16HeaderBigEndian))) {
                        if (mpWriteBuffer) {
                            free(mpWriteBuffer);
                        }
                        mpWriteBuffer = (TU8 *)malloc(mnHeaderLength + length + sizeof(STLV16HeaderBigEndian) + 4);
                        if (DUnlikely(!mpWriteBuffer)) {
                            LOG_FATAL("No Memory!\n");
                            err = EECode_NoMemory;
                            break;
                        }
                        mnWriteBufferLength = mnHeaderLength + length + sizeof(STLV16HeaderBigEndian) + 4;
                    }
                    p_cur = mpWriteBuffer + mnHeaderLength;

                    p_tlv = (STLV16HeaderBigEndian *)p_cur;
                    tlv_length = length;
                    p_tlv->type_high = ((TU16)ETLV16Type_UserOwnedDeviceList >> 8) & 0xff;
                    p_tlv->type_low = ((TU16)ETLV16Type_UserOwnedDeviceList) & 0xff;
                    p_tlv->length_high = ((TU16)(tlv_length) >> 8) & 0xff;
                    p_tlv->length_low = ((TU16)(tlv_length)) & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);

                    LOG_NOTICE("[%llx]'s owned device list:\n", mUniqueID);
                    for (i = 0; i < count; i++) {
                        DBEW64(p_id_list[i], p_cur);
                        LOG_PRINTF("id[%u], %llx\n", i, p_id_list[i]);
                        p_cur += sizeof(TUniqueID);
                    }

                    length += sizeof(STLV16HeaderBigEndian);
                    LOG_NOTICE("[DEBUG]: print admin device list, %d\n", length);
                    gfPrintMemory(mpWriteBuffer + mnHeaderLength, length);

                    replyToClient(EECode_OK, mpWriteBuffer, mnHeaderLength + length);
                } else {
                    p_cur = mpWriteBuffer + mnHeaderLength;

                    p_tlv = (STLV16HeaderBigEndian *)p_cur;
                    tlv_length = 0;
                    p_tlv->type_high = ((TU16)ETLV16Type_UserOwnedDeviceList >> 8) & 0xff;
                    p_tlv->type_low = ((TU16)ETLV16Type_UserOwnedDeviceList) & 0xff;
                    p_tlv->length_high = ((TU16)(tlv_length) >> 8) & 0xff;
                    p_tlv->length_low = ((TU16)(tlv_length)) & 0xff;

                    LOG_NOTICE("[%llx] no owned device\n", mUniqueID);
                    replyToClient(EECode_OK, mpWriteBuffer, mnHeaderLength + sizeof(STLV16HeaderBigEndian));
                }

                return EECode_OK;
            } else {
                LOG_FATAL("retrieve id list: payload size is 0!\n");
                replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                return EECode_ServerReject_CorruptedProtocol;
            }
            break;

        case ESACPClientCmdSubType_SinkRetrieveSharedDeviceList:
            if (DUnlikely(EAccountGroup_UserAccount != ((mUniqueID & DExtIDTypeMask) >> DExtIDTypeShift))) {
                LOG_FATAL("not user account? %llx, from attacker?\n", mUniqueID);
                replyToClient(EECode_PossibleAttackFromNetwork, mpWriteBuffer, mnHeaderLength);
                return EECode_PossibleAttackFromNetwork;
            }
            if (DLikely(0 == payload_size)) {
                TUniqueID *p_id_list = NULL;
                TU32 count = 0, i = 0;
                err = mpAccountManager->RetrieveUserSharedDeviceList(mUniqueID, count, p_id_list);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_FATAL("ESACPClientCmdSubType_SinkRetrieveSharedDeviceList fail\n");
                    break;
                }

                if (count && p_id_list) {

                    TU32 length = count * sizeof(TUniqueID);
                    DASSERT(length < (1 << 16));

                    if (mnWriteBufferLength < (mnHeaderLength + length + sizeof(STLV16HeaderBigEndian))) {
                        if (mpWriteBuffer) {
                            free(mpWriteBuffer);
                        }
                        mpWriteBuffer = (TU8 *)malloc(mnHeaderLength + length + sizeof(STLV16HeaderBigEndian) + 4);
                        if (DUnlikely(!mpWriteBuffer)) {
                            LOG_FATAL("No Memory!\n");
                            err = EECode_NoMemory;
                            break;
                        }
                        mnWriteBufferLength = mnHeaderLength + length + sizeof(STLV16HeaderBigEndian) + 4;
                    }

                    p_cur = mpWriteBuffer + mnHeaderLength;

                    p_tlv = (STLV16HeaderBigEndian *)p_cur;
                    tlv_length = length;
                    p_tlv->type_high = ((TU16)ETLV16Type_UserSharedDeviceList >> 8) & 0xff;
                    p_tlv->type_low = ((TU16)ETLV16Type_UserSharedDeviceList) & 0xff;
                    p_tlv->length_high = ((TU16)(tlv_length) >> 8) & 0xff;
                    p_tlv->length_low = ((TU16)(tlv_length)) & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);

                    LOG_NOTICE("[%llx]'s shared device list:\n", mUniqueID);
                    for (i = 0; i < count; i++) {
                        DBEW64(p_id_list[i], p_cur);
                        LOG_PRINTF("id[%u]: %llx\n", i, p_id_list[i]);
                        p_cur += sizeof(TUniqueID);
                    }
                    length += sizeof(STLV16HeaderBigEndian);

                    replyToClient(EECode_OK, mpWriteBuffer, mnHeaderLength + length);
                } else {
                    p_cur = mpWriteBuffer + mnHeaderLength;

                    p_tlv = (STLV16HeaderBigEndian *)p_cur;
                    tlv_length = 0;
                    p_tlv->type_high = ((TU16)ETLV16Type_UserSharedDeviceList >> 8) & 0xff;
                    p_tlv->type_low = ((TU16)ETLV16Type_UserSharedDeviceList) & 0xff;
                    p_tlv->length_high = ((TU16)(tlv_length) >> 8) & 0xff;
                    p_tlv->length_low = ((TU16)(tlv_length)) & 0xff;

                    LOG_NOTICE("[%llx] no shared device, count %d, p_id_list %p\n", mUniqueID, count, p_id_list);
                    replyToClient(EECode_OK, mpWriteBuffer, mnHeaderLength + sizeof(STLV16HeaderBigEndian));
                }

                return EECode_OK;
            } else {
                LOG_FATAL("retrieve id list: payload size is 0!\n");
                replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                return EECode_ServerReject_CorruptedProtocol;
            }
            break;

        case ESACPClientCmdSubType_UpdateSinkNickname:
            if (DUnlikely(EAccountGroup_UserAccount != ((mUniqueID & DExtIDTypeMask) >> DExtIDTypeShift))) {
                LOG_FATAL("not user account? %llx, from attacker?\n", mUniqueID);
                replyToClient(EECode_PossibleAttackFromNetwork, mpWriteBuffer, mnHeaderLength);
                return EECode_PossibleAttackFromNetwork;
            }
            if (DLikely(payload_size > (sizeof(STLV16HeaderBigEndian)))) {
                p_tlv = (STLV16HeaderBigEndian *)payload;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely((ETLV16Type_AccountNickName != tlv_type) || (DMAX_ACCOUNT_NAME_LENGTH_EX <= tlv_length))) {
                    LOG_FATAL("corrupted protocol, tlv_type %x, tlv_length %d\n", tlv_type, tlv_length);
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                p_cur = payload + sizeof(STLV16HeaderBigEndian);
                memcpy(mpAccountInfo->base.nickname, p_cur, tlv_length);

                replyToClient(EECode_OK, mpWriteBuffer, mnHeaderLength);
            } else {
                LOG_FATAL("ESACPClientCmdSubType_UpdateSinkNickname: payload size check fail\n");
                replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                return EECode_ServerReject_CorruptedProtocol;
            }
            break;

        case ESACPClientCmdSubType_UpdateSinkPassword:
            if (DUnlikely(EAccountGroup_UserAccount != ((mUniqueID & DExtIDTypeMask) >> DExtIDTypeShift))) {
                LOG_FATAL("not user account? %llx, from attacker?\n", mUniqueID);
                replyToClient(EECode_PossibleAttackFromNetwork, mpWriteBuffer, mnHeaderLength);
                return EECode_PossibleAttackFromNetwork;
            }
            if (DLikely(payload_size > 0)) {
                p_tlv = (STLV16HeaderBigEndian *)payload;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely((ETLV16Type_ClientDynamicInput != tlv_type) || (DDynamicInputStringLength != tlv_length))) {
                    LOG_FATAL("corrupted protocol, tlv_type %x, tlv_length %d\n", tlv_type, tlv_length);
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                p_cur = payload + sizeof(STLV16HeaderBigEndian);

                TU8 hash_input[DDynamicInputStringLength + DIdentificationStringLength] = {0};
                memcpy(hash_input, p_cur, DDynamicInputStringLength);
                memcpy(hash_input + DDynamicInputStringLength, &mpAccountInfo->base.password[0], DIdentificationStringLength);
                LOG_NOTICE("hash_input: %s\n", mpAccountInfo->base.password);
                gfPrintMemory(hash_input, DDynamicInputStringLength + DIdentificationStringLength);
                TU32 hash = gfHashAlgorithmV8(hash_input, DDynamicInputStringLength + DIdentificationStringLength);
                p_cur += tlv_length;

                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                p_cur += sizeof(STLV16HeaderBigEndian);

                if (DUnlikely((ETLV16Type_DynamicCode != tlv_type) || (sizeof(TU32) != tlv_length))) {
                    LOG_FATAL("corrupted protocol, tlv_type %x, tlv_length %d\n", tlv_type, tlv_length);
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                TU32 hash_recv = 0;
                LOG_NOTICE("hash_recv:\n");
                gfPrintMemory(p_cur, 4);

                DBER32(hash_recv, p_cur);
                p_cur += sizeof(TU32);
                if (hash != hash_recv) {
                    LOG_ERROR("password not correct, [%s], hash %08x, hash_recv %08x\n", mpAccountInfo->base.password, hash, hash_recv);
                    replyToClient(EECode_ServerReject_AuthenticationFail, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_AuthenticationFail;
                }

                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely((ETLV16Type_AccountPassword != tlv_type) || (DIdentificationStringLength <= tlv_length))) {
                    LOG_FATAL("corrupted protocol, tlv_type %x, tlv_length %d\n", tlv_type, tlv_length);
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                p_cur += sizeof(STLV16HeaderBigEndian);
                LOG_NOTICE("change password of [%s], %s -> %s\n", mpAccountInfo->base.name, mpAccountInfo->base.password, p_cur);
                memset(mpAccountInfo->base.password, 0x0, sizeof(mpAccountInfo->base.password));
                memcpy(mpAccountInfo->base.password, p_cur, tlv_length);

                replyToClient(EECode_OK, mpWriteBuffer, mnHeaderLength);
            } else {
                LOG_FATAL("ESACPClientCmdSubType_UpdateSinkPassword: payload size is 0!\n");
                replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                return EECode_ServerReject_CorruptedProtocol;
            }
            break;

        case ESACPClientCmdSubType_SetSecureQuestion:
            if (DUnlikely(EAccountGroup_UserAccount != ((mUniqueID & DExtIDTypeMask) >> DExtIDTypeShift))) {
                LOG_FATAL("not user account? %llx, from attacker?\n", mUniqueID);
                replyToClient(EECode_PossibleAttackFromNetwork, mpWriteBuffer, mnHeaderLength);
                return EECode_PossibleAttackFromNetwork;
            }
            if (DLikely(payload_size > 0)) {
                p_tlv = (STLV16HeaderBigEndian *)payload;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely((ETLV16Type_ClientDynamicInput != tlv_type) || (DDynamicInputStringLength != tlv_length))) {
                    LOG_FATAL("corrupted protocol, tlv_type %x, tlv_length %d\n", tlv_type, tlv_length);
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                p_cur = payload + sizeof(STLV16HeaderBigEndian);

                TU8 hash_input[DDynamicInputStringLength + DIdentificationStringLength] = {0};
                memcpy(hash_input, p_cur, DDynamicInputStringLength);
                memcpy(hash_input + DDynamicInputStringLength, mpAccountInfo->base.password, DIdentificationStringLength);
                TU32 hash = gfHashAlgorithmV8(hash_input, DDynamicInputStringLength + DIdentificationStringLength);
                p_cur += tlv_length;

                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                p_cur += sizeof(STLV16HeaderBigEndian);

                if (DUnlikely((ETLV16Type_DynamicCode != tlv_type) || (sizeof(TU32) != tlv_length))) {
                    LOG_FATAL("corrupted protocol, tlv_type %x, tlv_length %d\n", tlv_type, tlv_length);
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                TU32 hash_recv = 0;
                DBER32(hash_recv, p_cur);
                p_cur += sizeof(TU32);
                if (hash != hash_recv) {
                    LOG_ERROR("password not correct, [%s]\n", mpAccountInfo->base.password);
                    replyToClient(EECode_ServerReject_AuthenticationFail, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_AuthenticationFail;
                }

                SAccountInfoUser *p_usr = (SAccountInfoUser *)mpAccountInfo;

                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely((ETLV16Type_SecurityQuestion != tlv_type) || (DMAX_SECURE_QUESTION_LENGTH <= tlv_length))) {
                    LOG_FATAL("corrupted protocol, tlv_type %x, tlv_length %d\n", tlv_type, tlv_length);
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                p_cur += sizeof(STLV16HeaderBigEndian);

                memset(p_usr->ext.secure_question1, 0x0, sizeof(p_usr->ext.secure_question1));
                memcpy(p_usr->ext.secure_question1, p_cur, tlv_length);
                p_cur += tlv_length;

                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely((ETLV16Type_SecurityAnswer != tlv_type) || (DMAX_SECURE_ANSWER_LENGTH <= tlv_length))) {
                    LOG_FATAL("corrupted protocol, tlv_type %x, tlv_length %d\n", tlv_type, tlv_length);
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                p_cur += sizeof(STLV16HeaderBigEndian);

                memset(p_usr->ext.secure_answer1, 0x0, sizeof(p_usr->ext.secure_answer1));
                memcpy(p_usr->ext.secure_answer1, p_cur, tlv_length);
                p_cur += tlv_length;

                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely((ETLV16Type_SecurityQuestion != tlv_type) || (DMAX_SECURE_QUESTION_LENGTH <= tlv_length))) {
                    LOG_FATAL("corrupted protocol, tlv_type %x, tlv_length %d\n", tlv_type, tlv_length);
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                p_cur += sizeof(STLV16HeaderBigEndian);

                memset(p_usr->ext.secure_question2, 0x0, sizeof(p_usr->ext.secure_question2));
                memcpy(p_usr->ext.secure_question2, p_cur, tlv_length);
                p_cur += tlv_length;

                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely((ETLV16Type_SecurityAnswer != tlv_type) || (DMAX_SECURE_ANSWER_LENGTH <= tlv_length))) {
                    LOG_FATAL("corrupted protocol, tlv_type %x, tlv_length %d\n", tlv_type, tlv_length);
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                p_cur += sizeof(STLV16HeaderBigEndian);

                memset(p_usr->ext.secure_answer2, 0x0, sizeof(p_usr->ext.secure_answer2));
                memcpy(p_usr->ext.secure_answer2, p_cur, tlv_length);
                p_cur += tlv_length;

                replyToClient(EECode_OK, mpWriteBuffer, mnHeaderLength);
            } else {
                LOG_FATAL("ESACPClientCmdSubType_SetSecureQuestion: payload size is 0!\n");
                replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                return EECode_ServerReject_CorruptedProtocol;
            }
            break;

        case ESACPClientCmdSubType_SinkDonateSourceDevice:
            if (DUnlikely(EAccountGroup_UserAccount != ((mUniqueID & DExtIDTypeMask) >> DExtIDTypeShift))) {
                LOG_FATAL("not user account? %llx, from attacker?\n", mUniqueID);
                replyToClient(EECode_PossibleAttackFromNetwork, mpWriteBuffer, mnHeaderLength);
                return EECode_PossibleAttackFromNetwork;
            }
            if (DLikely(payload_size > 0)) {
                TUniqueID device_id = DInvalidUniqueID;
                TUniqueID user_id = DInvalidUniqueID;
                p_tlv = (STLV16HeaderBigEndian *)payload;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely((ETLV16Type_AccountName != tlv_type) || (DMAX_ACCOUNT_NAME_LENGTH_EX < tlv_length))) {
                    LOG_FATAL("corrupted protocol, tlv_type %x, tlv_length %d\n", tlv_type, tlv_length);
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                p_cur = payload + sizeof(STLV16HeaderBigEndian);

                if (DUnlikely(1 == tlv_length)) {
                    LOG_NOTICE("unbind\n");
                } else {
                    TChar target[DMAX_ACCOUNT_NAME_LENGTH_EX] = {0};
                    memcpy(target, p_cur, tlv_length);
                    SAccountInfoUser *p_target = NULL;
                    err = mpAccountManager->QueryUserAccountByName(target, p_target);
                    if ((EECode_OK != err) || !p_target) {
                        LOG_ERROR("not valid target\n");
                        replyToClient(EECode_BadParam, mpWriteBuffer, mnHeaderLength);
                        return EECode_BadParam;
                    }
                    user_id = p_target->root.header.id;
                }

                p_cur += tlv_length;
                p_tlv = (STLV16HeaderBigEndian *)p_cur;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                p_cur += sizeof(STLV16HeaderBigEndian);

                if (DUnlikely((ETLV16Type_AccountID != tlv_type) || (sizeof(TUniqueID) != tlv_length))) {
                    LOG_FATAL("corrupted protocol, tlv_type %x, tlv_length %d\n", tlv_type, tlv_length);
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                DBER64(device_id, p_cur);

                if (DUnlikely(EAccountGroup_SourceDevice != ((device_id & DExtIDTypeMask) >> DExtIDTypeShift))) {
                    LOG_FATAL("not device account? %llx\n", device_id);
                    replyToClient(EECode_BadParam, mpWriteBuffer, mnHeaderLength);
                    return EECode_BadParam;
                }

                err = mpAccountManager->DonateSourceDeviceOwnshipByContext((SAccountInfoUser *)mpAccountInfo, user_id, device_id);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("DonateSourceDeviceOwnshipByContext, %d, %s\n", err, gfGetErrorCodeString(err));
                    replyToClient(EECode_BadParam, mpWriteBuffer, mnHeaderLength);
                    return EECode_BadParam;
                }

                replyToClient(EECode_OK, mpWriteBuffer, mnHeaderLength);
            } else {
                LOG_FATAL("ESACPClientCmdSubType_SinkDonateSourceDevice: payload size is 0!\n");
                replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                return EECode_ServerReject_CorruptedProtocol;
            }
            break;

        case ESACPClientCmdSubType_SinkAcquireDynamicCode:
            if (DUnlikely(EAccountGroup_UserAccount != ((mUniqueID & DExtIDTypeMask) >> DExtIDTypeShift))) {
                LOG_FATAL("not user account? %llx, from attacker?\n", mUniqueID);
                replyToClient(EECode_PossibleAttackFromNetwork, mpWriteBuffer, mnHeaderLength);
                return EECode_PossibleAttackFromNetwork;
            }

            if (DLikely(payload_size > 0)) {
                //device id
                TUniqueID device_id = 0;
                p_tlv = (STLV16HeaderBigEndian *)payload;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely((ETLV16Type_DeviceID != tlv_type) || (sizeof(TUniqueID) != tlv_length))) {
                    LOG_FATAL("corrupted protocol\n");
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                p_cur = payload + sizeof(STLV16HeaderBigEndian);
                DBER64(device_id, p_cur);

                SAccountInfoSourceDevice *p_device = NULL;
                err = mpAccountManager->PrivilegeQueryDevice(mUniqueID, device_id, p_device);
                if (DUnlikely(EECode_OK != err)) {
                    DASSERT(!p_device);
                    replyToClient(err, mpWriteBuffer, mnHeaderLength);
                    return err;
                }
                DASSERT(p_device);

                SAccountInfoRoot *p_user = NULL;
                err = mpAccountManager->QueryAccount(mUniqueID, p_user);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_FATAL("QueryAccount by id fail\n");
                    DASSERT(!p_user);
                    replyToClient(err, mpWriteBuffer, mnHeaderLength);
                    return err;
                }
                DASSERT(p_user);

                TU64 dynamic_input = 0;
                TU8 tmp_buffer[64] = {0};
                gfGetCurrentDateTime((SDateTime *) tmp_buffer);
                tmp_buffer[sizeof(SDateTime)] = 0x4d;
                tmp_buffer[sizeof(SDateTime) + 1] = 0x55;
                tmp_buffer[sizeof(SDateTime) + 2] = 0xcf;
                tmp_buffer[sizeof(SDateTime) + 3] = 0xa7;
                dynamic_input = gfHashAlgorithmV5(tmp_buffer, sizeof(SDateTime) + 4);
                LOG_DEBUG("dynamic_input: %llx, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", dynamic_input,
                          tmp_buffer[0], tmp_buffer[1], tmp_buffer[2], tmp_buffer[3], tmp_buffer[4], tmp_buffer[5], tmp_buffer[6], tmp_buffer[7]);
                LOG_DEBUG("password: %s\n", p_user->base.password);

                DBEW64(dynamic_input, tmp_buffer);
                memcpy(tmp_buffer + DDynamicInputStringLength, p_user->base.password, strlen(p_user->base.password));
                TU32 dynamic_code = gfHashAlgorithmV6(tmp_buffer, DDynamicInputStringLength + DIdentificationStringLength);
                TU32 datanode_index = (device_id & DExtIDDataNodeMask) >> DExtIDDataNodeShift;
                err = mpAccountManager->UpdateUserDynamicCode(datanode_index, p_user->base.name, p_device->root.base.name, dynamic_code);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_FATAL("SinkAquireDynamicCode fail, user: %s, device: %s\n", p_user->base.name, p_device->root.base.name);
                    replyToClient(EECode_UpdateUserDynamiccodeFail, mpWriteBuffer, mnHeaderLength);
                    return err;
                }

                //send dynamic input to client
                tlv_length = DDynamicInputStringLength;
                p_tlv->type_high = (ETLV16Type_DynamicInput >> 8) & 0xff;
                p_tlv->type_low = (ETLV16Type_DynamicInput) & 0xff;
                p_tlv->length_high = (tlv_length >> 8) & 0xff;
                p_tlv->length_low = tlv_length & 0xff;
                p_cur = payload + sizeof(STLV16HeaderBigEndian);

                p_cur[0] = (dynamic_input >> 56) & 0xff;
                p_cur[1] = (dynamic_input >> 48) & 0xff;
                p_cur[2] = (dynamic_input >> 40) & 0xff;
                p_cur[3] = (dynamic_input >> 32) & 0xff;
                p_cur[4] = (dynamic_input >> 24) & 0xff;
                p_cur[5] = (dynamic_input >> 16) & 0xff;
                p_cur[6] = (dynamic_input >> 8) & 0xff;
                p_cur[7] = (dynamic_input) & 0xff;

                replyToClient(EECode_OK, mpWriteBuffer, mnHeaderLength + sizeof(STLV16HeaderBigEndian) + tlv_length);
            }
            break;

        case ESACPClientCmdSubType_SinkReleaseDynamicCode:
            if (DUnlikely(EAccountGroup_UserAccount != ((mUniqueID & DExtIDTypeMask) >> DExtIDTypeShift))) {
                LOG_FATAL("not user account? %llx, from attacker?\n", mUniqueID);
                replyToClient(EECode_PossibleAttackFromNetwork, mpWriteBuffer, mnHeaderLength);
                return EECode_PossibleAttackFromNetwork;
            }

            if (DLikely(payload_size > 0)) {
                //device id
                TUniqueID device_id = 0;
                p_tlv = (STLV16HeaderBigEndian *)payload;
                tlv_type = ((TU16)p_tlv->type_high << 8) | ((TU16)p_tlv->type_low);
                tlv_length = ((TU16)p_tlv->length_high << 8) | ((TU16)p_tlv->length_low);
                if (DUnlikely((ETLV16Type_AccountID != tlv_type) || (sizeof(TUniqueID) != tlv_length))) {
                    LOG_FATAL("corrupted protocol\n");
                    replyToClient(EECode_ServerReject_CorruptedProtocol, mpWriteBuffer, mnHeaderLength);
                    return EECode_ServerReject_CorruptedProtocol;
                }
                p_cur = payload + sizeof(STLV16HeaderBigEndian);
                DBER64(device_id, p_cur);

                SAccountInfoSourceDevice *p_device = NULL;
                err = mpAccountManager->PrivilegeQueryDevice(mUniqueID, device_id, p_device);
                if (DUnlikely(EECode_OK != err)) {
                    DASSERT(!p_device);
                    replyToClient(err, mpWriteBuffer, mnHeaderLength);
                    return err;
                }
                DASSERT(p_device);

                SAccountInfoRoot *p_user = NULL;
                err = mpAccountManager->QueryAccount(mUniqueID, p_user);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_FATAL("QueryAccount by id fail\n");
                    DASSERT(!p_user);
                    replyToClient(err, mpWriteBuffer, mnHeaderLength);
                    return err;
                }
                DASSERT(p_user);

                TU32 datanode_index = (device_id & DExtIDDataNodeMask) >> DExtIDDataNodeShift;
                err = mpAccountManager->UpdateUserDynamicCode(datanode_index, p_user->base.name, p_device->root.base.name, 0);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_FATAL("SinkReleaseDynamicCode fail, user: %s, device: %s\n", p_user->base.name, p_device->root.base.name);
                    replyToClient(EECode_UpdateUserDynamiccodeFail, mpWriteBuffer, mnHeaderLength);
                    return err;
                }

                replyToClient(EECode_OK, mpWriteBuffer, mnHeaderLength);
            }
            break;

        default:
            LOG_FATAL("unkhown sub type: 0x%08x\n", sub_type);
            err = EECode_BadMsg;
            break;
    }

    if (DUnlikely(err != EECode_OK)) {
        replyToClient(err, mpWriteBuffer, mnHeaderLength);
    }

    return err;
}

void CNetworkAgent::replyToClient(EECode error_code, TU8 *header, TU16 length)
{
    TU8 flag = gfSACPErrorCodeToConnectResult(error_code);
    mpProtocolHeaderParser->SetFlag(header, (TULong)flag);
    if (DUnlikely(EECode_OK != error_code)) {
        LOG_ERROR("reply to client %d, %s\n", error_code, gfGetErrorCodeString(error_code));
    }

    TU16 payload_size = length - mnHeaderLength;
    mpProtocolHeaderParser->SetPayloadSize(header, payload_size);
    //mpProtocolHeaderParser->FillSourceID(header, mUniqueID);
    mpProtocolHeaderParser->SetReqResBits(header, DSACPTypeBit_Responce);

    if (sendData(header, length) != EECode_OK) {
        LOG_FATAL("CNetworkAgent, sendData fail!\n");
    }
}

void CNetworkAgent::notifyToClient(TU8 error_code, TU8 *header, TU16 length)
{
    mpProtocolHeaderParser->SetFlag(header, (TULong)error_code);

    TU16 payload_size = length - mnHeaderLength;
    mpProtocolHeaderParser->SetPayloadSize(header, payload_size);
    //mpProtocolHeaderParser->FillSourceID(header, mUniqueID);
    mpProtocolHeaderParser->SetReqResBits(header, DSACPTypeBit_Responce);

    if (sendData(header, length) != EECode_OK) {
        LOG_FATAL("CNetworkAgent, sendData fail!\n");
    }
    return;
}

void CNetworkAgent::notifyTargetNotReachable(TUniqueID target_id)
{
    memset(mpWriteBuffer, 0x0, mnHeaderLength);
    DSetSACPType(DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_SystemNotification, ESACPClientCmdSubType_SystemNotifyTargetNotReachable), mpWriteBuffer);

    TU8 *p_cur = mpWriteBuffer + mnHeaderLength;
    STLV16HeaderBigEndian *p_tlv = (STLV16HeaderBigEndian *)p_cur;

    p_tlv->type_high = ((TU16)ETLV16Type_AccountID >> 8) & 0xff;
    p_tlv->type_low = ((TU16)ETLV16Type_AccountID) & 0xff;
    p_tlv->length_high = ((TU16)(sizeof(TUniqueID)) >> 8) & 0xff;
    p_tlv->length_low = ((TU16)(sizeof(TUniqueID))) & 0xff;
    p_cur += sizeof(STLV16HeaderBigEndian);

    DBEW64(target_id, p_cur);

    mpProtocolHeaderParser->SetPayloadSize(mpWriteBuffer, sizeof(TUniqueID) + sizeof(STLV16HeaderBigEndian));

    if (sendData(mpWriteBuffer, mnHeaderLength + sizeof(TUniqueID) + sizeof(STLV16HeaderBigEndian)) != EECode_OK) {
        LOG_FATAL("CNetworkAgent, sendData fail!\n");
    }

    return;
}

void CNetworkAgent::notifyAllFriendsOffline()
{
    EECode err = EECode_OK;
    DASSERT(mpAccountInfo->header.id == mUniqueID);
    EAccountGroup group = (EAccountGroup)((mUniqueID & DExtIDTypeMask) >> DExtIDTypeShift);

    if (EAccountGroup_UserAccount == group) {
        SAccountInfoUser *p_user = (SAccountInfoUser *) mpAccountInfo;
        SAccountInfoRoot *p_friend = NULL;
        CNetworkAgent *p_agent = NULL;
        TU32 i = 0;

        if (p_user->friendsnum) {
            memset(mpWriteBuffer, 0x0, mnHeaderLength);
            DSetSACPType(DBuildSACPType(DSACPTypeBit_Request, ESACPTypeCategory_SystemNotification, ESACPClientCmdSubType_SystemNotifyTargetOffline), mpWriteBuffer);

            TU8 *p_cur = mpWriteBuffer + mnHeaderLength;
            STLV16HeaderBigEndian *p_tlv = (STLV16HeaderBigEndian *)p_cur;

            p_tlv->type_high = ((TU16)ETLV16Type_AccountID >> 8) & 0xff;
            p_tlv->type_low = ((TU16)ETLV16Type_AccountID) & 0xff;
            p_tlv->length_high = ((TU16)(sizeof(TUniqueID)) >> 8) & 0xff;
            p_tlv->length_low = ((TU16)(sizeof(TUniqueID))) & 0xff;
            p_cur += sizeof(STLV16HeaderBigEndian);

            DBEW64(mUniqueID, p_cur);
            mpProtocolHeaderParser->SetPayloadSize(mpWriteBuffer, sizeof(TUniqueID) + sizeof(STLV16HeaderBigEndian));

            for (i = 0; i < p_user->friendsnum; i ++) {
                err = mpAccountManager->QueryAccount(p_user->p_friendsid[i], p_friend);
                if (EECode_OK != err) {
                    continue;
                }
                p_agent = (CNetworkAgent *) p_friend->p_agent;
                if (p_agent && (DInvalidSocketHandler != p_agent->GetSocket())) {
                    p_agent->SendData(mpWriteBuffer, mnHeaderLength + sizeof(TUniqueID) + sizeof(STLV16HeaderBigEndian));
                }
            }

        }

    }

    return;
}

//misc
extern IIMServer *gfCreateSACPIMServer(const TChar *name, EProtocolType type, const volatile SPersistCommonConfig *pPersistCommonConfig, IMsgSink *pMsgSink, IAccountManager *p_account_manager, TU16 server_port);

IIMServer *gfCreateIMServer(EProtocolType type, const volatile SPersistCommonConfig *pPersistCommonConfig, IMsgSink *pMsgSink, IAccountManager *p_account_manager, TU16 server_port)
{
    IIMServer *thiz = NULL;

    switch (type) {

        case EProtocolType_SACP:
            thiz = gfCreateSACPIMServer("gfCreateSACPIMServer", type, pPersistCommonConfig, pMsgSink, p_account_manager, server_port);
            break;

        default:
            LOG_FATAL("BAD EProtocolType %d\n", type);
            thiz = NULL;
            break;
    }

    return thiz;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

