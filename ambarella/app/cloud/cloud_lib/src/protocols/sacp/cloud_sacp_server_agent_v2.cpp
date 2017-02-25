/*******************************************************************************
 * cloud_sacp_server_agent_v2.cpp
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
#include "cloud_sacp_server_agent_v2.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

ICloudServerAgent *gfCreateSACPCloudServerAgentV2()
{
    CSACPCloudServerAgentV2 *thiz = CSACPCloudServerAgentV2::Create();
    return thiz;
}

//-----------------------------------------------------------------------
//
//  CSACPCloudServerAgentV2
//
//-----------------------------------------------------------------------

CSACPCloudServerAgentV2 *CSACPCloudServerAgentV2::Create()
{
    CSACPCloudServerAgentV2 *result = new CSACPCloudServerAgentV2();
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

EECode CSACPCloudServerAgentV2::Construct()
{
    mSenderBufferSize = 4 * 1024;
    mpSenderBuffer = (TU8 *) DDBG_MALLOC(mSenderBufferSize, "SASB");

    if (DUnlikely(!mpSenderBuffer)) {
        LOG_FATAL("no memory, request %d\n", mSenderBufferSize);
        mSenderBufferSize = 0;
        return EECode_NoMemory;
    }

    mpSACPHeader = (SSACPHeader *)mpSenderBuffer;
    mpSACPPayload = (TU8 *)(mpSenderBuffer + sizeof(SSACPHeader));

    return EECode_OK;
}

CSACPCloudServerAgentV2::CSACPCloudServerAgentV2()
    : mbAccepted(0)
    , mbAuthenticated(0)
    , mbPeerClosed(0)
    , mSocket(DInvalidSocketHandler)
    , mLastSeqCount(0)
    , mDataType(ESACPDataChannelSubType_Invalid)
    , mHeaderExtentionType(EProtocolHeaderExtentionType_NoHeader)
    , mEncryptionType1(EEncryptionType_None)
    , mEncryptionType2(EEncryptionType_None)
    , mEncryptionType3(EEncryptionType_None)
    , mEncryptionType4(EEncryptionType_None)
    , mpCmdCallbackOwner(NULL)
    , mpCmdCallback(NULL)
    , mpDataCallbackOwner(NULL)
    , mpDataCallback(NULL)
    , mpSenderBuffer(NULL)
    , mSenderBufferSize(0)
    , mpSACPHeader(NULL)
    , mpSACPPayload(NULL)
    , mDataSubType(ESACPDataChannelSubType_Invalid)
{
}

CSACPCloudServerAgentV2::~CSACPCloudServerAgentV2()
{
    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    if (mpSenderBuffer) {
        DDBG_FREE(mpSenderBuffer, "SASB");
        mpSenderBuffer = NULL;
    }

    mSenderBufferSize = 0;
    mpSACPHeader = NULL;
    mpSACPPayload = NULL;
}

EECode CSACPCloudServerAgentV2::processClientCmd(ESACPClientCmdSubType sub_type, TU16 payload_size)
{
    if (DUnlikely(!mpCmdCallback || !mpCmdCallbackOwner)) {
        LOG_ERROR("cmd callback not set, callback %p, context %p\n", mpCmdCallback, mpCmdCallbackOwner);
        return EECode_BadState;
    }

    if (DLikely(payload_size)) {
        ReadData(mpSACPPayload, payload_size);
    }

    //LOG_NOTICE("sub_type %d, payload_size=%hu\n", sub_type, payload_size);
    return mpCmdCallback(mpCmdCallbackOwner, (TU32)sub_type, mpSACPPayload,  payload_size);
}

EECode CSACPCloudServerAgentV2::processCmd(TU8 cat, TU16 sub_type, TU16 payload_size)
{
    if (ESACPTypeCategory_ClientCmdChannel == cat) {
        return processClientCmd((ESACPClientCmdSubType)sub_type, payload_size);
    } else {
        LOG_FATAL("BAD cmd categary %d\n", cat);
    }

    return EECode_BadParam;
}

EECode CSACPCloudServerAgentV2::Scheduling(TUint times, TU32 inout_mask)
{
    EECode err = EECode_OK;
    TInt size = 0;
    TU32 type = 0;

    TU32 payload_size = 0;
    TU32 seq_num = 0;
    TU16 header_ext_size = 0;

    TU8 has_header_ext = 0;

    if (DUnlikely((mbPeerClosed) || (!mbAccepted))) {
        LOG_NOTICE("peer closed or not accepted, mbPeerClosed %d, mbAccepted %d\n", mbPeerClosed, mbAccepted);
        return EECode_NotRunning;
    }

    size = sizeof(SSACPHeader);
    //LOG_INFO("read size %d start\n", size);
    size = gfNet_Recv_timeout(mSocket, (TU8 *)mpSACPHeader, size, DNETUTILS_RECEIVE_FLAG_READ_ALL, 4);
    //LOG_INFO("read size %d done\n", size);
    if (DLikely(size == sizeof(SSACPHeader))) {
        type = ((mpSACPHeader->type_1) << 24) | ((mpSACPHeader->type_2) << 16) | ((mpSACPHeader->type_3) << 8) | (mpSACPHeader->type_4);
        payload_size = ((TU32)mpSACPHeader->size_0 << 16) | ((TU32)mpSACPHeader->size_1 << 8) | ((TU32)mpSACPHeader->size_2);
        seq_num = ((TU32)mpSACPHeader->seq_count_0 << 16) | ((TU32)mpSACPHeader->seq_count_1 << 8) | ((TU32)mpSACPHeader->seq_count_2);
        has_header_ext = mpSACPHeader->header_ext_type;
        header_ext_size = (mpSACPHeader->header_ext_size_1 << 8) | mpSACPHeader->header_ext_size_2;

        if (DUnlikely((EProtocolHeaderExtentionType_NoHeader != has_header_ext) || (0 != header_ext_size))) {
            LOG_FATAL("corrupted protocol, has_header_ext %d, header_ext_size %d\n", has_header_ext, header_ext_size);
            return EECode_Closed;
        }

        if (type & DSACPTypeBit_Request) {
            //LOG_PRINTF("[server agent]: receive cmd 0x%08x, payload_size %d, seq (%d, %d)\n", type, payload_size, mpSACPHeader->seq_count_1, mpSACPHeader->seq_count_2);
            if (DLikely(mpCmdCallbackOwner && mpCmdCallback)) {
                TU8 cmd_category = (type >> DSACPTypeBit_CatTypeBits) & DSACPTypeBit_CatTypeMask;
                TU16 cmd_sub_type = type & DSACPTypeBit_SubTypeMask;
                //LOG_INFO("[agent]: cmd, payload size %d\n", payload_size);
                err = processCmd(cmd_category, cmd_sub_type, payload_size);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_FATAL("%d, %s, un expected data recieved? close connection\n", err, gfGetErrorCodeString(err));
                    return EECode_Closed;
                }
            } else {
                LOG_WARN("no data call back %p, %p\n", mpCmdCallbackOwner, mpCmdCallback);
                return EECode_Closed;
            }

            if (type & DSACPTypeBit_NeedReply) {
                LOG_ERROR("should not use this flag!\n");
                mpSACPHeader->type_1 &= ~(DSACPTypeBit_Request >> 24);
                mpSACPHeader->type_1 |= (DSACPTypeBit_Responce >> 24);
                //LOG_PRINTF("[server agent]: send reply length %d\n", sizeof(SSACPHeader));
                size = gfNet_Send(mSocket, (TU8 *)mpSACPHeader, (TInt)(sizeof(SSACPHeader)), 0);
                if (DUnlikely(size != (TInt)(sizeof(SSACPHeader)))) {
                    LOG_ERROR("send return %d\n", size);
                    return EECode_Closed;
                }
            }

            //LOG_PRINTF("[server agent]: process cmd %08x done\n", type);
        } else if (DUnlikely(type & DSACPTypeBit_Responce)) {
            TU8 cmd_category = (type >> DSACPTypeBit_CatTypeBits) & DSACPTypeBit_CatTypeMask;
            TU16 cmd_sub_type = type & DSACPTypeBit_SubTypeMask;
            DASSERT(ESACPTypeCategory_ClientCmdChannel == cmd_category);
            ReadData(mpSACPPayload, (TMemSize)payload_size);
            err = processCmd(ESACPTypeCategory_ClientCmdChannel, cmd_sub_type, payload_size);
            if (DUnlikely(EECode_OK != err)) {
                LOG_FATAL("%d, %s, un expected data recieved? close connection\n", err, gfGetErrorCodeString(err));
                return EECode_Closed;
            }
        } else {
            if (DLikely(mpDataCallbackOwner && mpDataCallback)) {
                TU16 data_sub_type = type & DSACPTypeBit_SubTypeMask;
                //LOG_PRINTF("[server agent]: data length %ld, flag %02x\n", payload_size, mpSACPHeader->flags);
                err = mpDataCallback(mpDataCallbackOwner, payload_size, data_sub_type, seq_num, mpSACPHeader->flags);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_FATAL("mpDataCallback return %d %s\n", size, gfGetErrorCodeString(err));
                    return EECode_Closed;
                }
            } else {
                LOG_WARN("no data call back %p, %p\n", mpDataCallbackOwner, mpDataCallback);
                return EECode_Closed;
            }
        }
    } else {
        if (DLikely(!size)) {
            LOG_WARN("peer close comes\n");
            return EECode_Closed;
        } else {
            LOG_WARN("only recv(%d), please check code\n", size);
            return EECode_Closed;
        }
    }

    return err;
}

TInt CSACPCloudServerAgentV2::IsPassiveMode() const
{
    return 0;
}

TSchedulingHandle CSACPCloudServerAgentV2::GetWaitHandle() const
{
    return (TSchedulingHandle)mSocket;
}

TU8 CSACPCloudServerAgentV2::GetPriority() const
{
    return 1;
}

EECode CSACPCloudServerAgentV2::EventHandling(EECode err)
{
    if (EECode_Closed == err) {
        if (DLikely(mpCmdCallback && mpCmdCallbackOwner)) {
            mpCmdCallback(mpCmdCallbackOwner, ESACPClientCmdSubType_PeerClose, NULL, 0);
        } else {
            LOG_ERROR("NULL mpCmdCallback %p or mpCmdCallbackOwner %p\n", mpCmdCallback, mpCmdCallbackOwner);
        }
        mbPeerClosed = 1;
    }
    return EECode_OK;
}

TSchedulingUnit CSACPCloudServerAgentV2::HungryScore() const
{
    LOG_WARN("CSACPCloudServerAgentV2::HungryScore, TO DO\n");
    return 1;
}

void CSACPCloudServerAgentV2::Destroy()
{
    Delete();
}

EECode CSACPCloudServerAgentV2::AcceptClient(TInt socket)
{
    DASSERT(0 <= socket);

    if (DLikely(!mbAccepted)) {
        DASSERT(0 > mSocket);
        mSocket = socket;
        mbAccepted = 1;
        mbPeerClosed = 0;
    } else {
        LOG_ERROR("already accept a client, socket %d, mSocket %d\n", socket, mSocket);
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CSACPCloudServerAgentV2::CloseConnection()
{
    if (DLikely(mbAccepted)) {
        if (DIsSocketHandlerValid(mSocket)) {
            gfNetCloseSocket(mSocket);
            mSocket = DInvalidSocketHandler;
        } else {
            LOG_ERROR("BAD mSocket %d\n", mSocket);
            return EECode_BadState;
        }
        mbAccepted = 0;
    } else {
        LOG_WARN("do not accept a client yet? mSocket %d\n", mSocket);
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CSACPCloudServerAgentV2::ReadData(TU8 *p_data, TMemSize size)
{
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
    ret = gfNet_Recv_timeout(mSocket, p_data, size, DNETUTILS_RECEIVE_FLAG_READ_ALL, 12);
    if (DUnlikely(0 >= ret)) {
        LOG_ERROR("size %ld, ret %d\n", size, ret);
        return EECode_Closed;
    }
    //LOG_NOTICE("[blocking read data size]: %d done\n", ret);

    return EECode_OK;
}

EECode CSACPCloudServerAgentV2::WriteData(TU8 *p_data, TMemSize size)
{
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

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE
EECode CSACPCloudServerAgentV2::SetProcessCMDCallBack(void *owner, TServerAgentCmdCallBack callback)
{
    LOG_FATAL("not version 2\n");
    return EECode_InternalLogicalBug;
}

EECode CSACPCloudServerAgentV2::SetProcessDataCallBack(void *owner, TServerAgentDataCallBack callback)
{
    LOG_FATAL("not version 2\n");
    return EECode_InternalLogicalBug;
}
#endif

EECode CSACPCloudServerAgentV2::SetProcessCMDCallBackV2(void *owner, TServerAgentV2CmdCallBack callback)
{
    DASSERT(owner);
    DASSERT(callback);

    if (DLikely(owner && callback)) {
        if (DUnlikely(mpCmdCallbackOwner || mpCmdCallback)) {
            LOG_WARN("replace cmd callback, ori mpCmdCallbackOwner %p, mpCmdCallback %p, current owner %p callback %p\n", mpCmdCallbackOwner, mpCmdCallback, owner, callback);
        }
        mpCmdCallbackOwner = owner;
        mpCmdCallback = callback;
    } else {
        LOG_FATAL("NULL onwer %p, callback %p\n", owner, callback);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CSACPCloudServerAgentV2::SetProcessDataCallBackV2(void *owner, TServerAgentV2DataCallBack callback)
{
    DASSERT(owner);
    DASSERT(callback);

    if (DLikely(owner && callback)) {
        if (DUnlikely(mpDataCallbackOwner || mpDataCallback)) {
            LOG_WARN("replace data callback, ori mpDataCallbackOwner %p, mpDataCallback %p, current owner %p callback %p\n", mpDataCallbackOwner, mpDataCallback, owner, callback);
        }
        mpDataCallbackOwner = owner;
        mpDataCallback = callback;
    } else {
        LOG_FATAL("NULL onwer %p, callback %p\n", owner, callback);
        return EECode_BadParam;
    }

    return EECode_OK;
}

void CSACPCloudServerAgentV2::Delete()
{
    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    if (mpSenderBuffer) {
        free(mpSenderBuffer);
        mpSenderBuffer = NULL;
    }

    mSenderBufferSize = 0;
    mpSACPHeader = NULL;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

