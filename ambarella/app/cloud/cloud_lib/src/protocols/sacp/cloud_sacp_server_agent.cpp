/*******************************************************************************
 * cloud_sacp_server_nvr_agent.cpp
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
#include "cloud_sacp_server_agent.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

ICloudServerAgent *gfCreateSACPCloudServerAgent()
{
    CSACPCloudServerAgent *thiz = CSACPCloudServerAgent::Create();
    return thiz;
}

//-----------------------------------------------------------------------
//
//  CSACPCloudServerAgent
//
//-----------------------------------------------------------------------

CSACPCloudServerAgent *CSACPCloudServerAgent::Create()
{
    CSACPCloudServerAgent *result = new CSACPCloudServerAgent();
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

EECode CSACPCloudServerAgent::Construct()
{
    mSenderBufferSize = 4 * 1024;
    mpSenderBuffer = (TU8 *) malloc(mSenderBufferSize);

    if (DUnlikely(!mpSenderBuffer)) {
        LOG_FATAL("no memory, request %ld\n", mSenderBufferSize);
        mSenderBufferSize = 0;
        return EECode_NoMemory;
    }

    mpSACPHeader = (SSACPHeader *)mpSenderBuffer;
    mpSACPPayload = (TU8 *)(mpSenderBuffer + sizeof(SSACPHeader));

    return EECode_OK;
}

CSACPCloudServerAgent::CSACPCloudServerAgent()
    : mbAccepted(0)
    , mbAuthenticated(0)
    , mbPeerClosed(0)
    , mSocket(-1)
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

CSACPCloudServerAgent::~CSACPCloudServerAgent()
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
    mpSACPPayload = NULL;
}

EECode CSACPCloudServerAgent::processClientCmd(ESACPClientCmdSubType sub_type, TU16 payload_size)
{
    EECode err = EECode_OK;
    TU32 cmd_type = sub_type;
    TU32 param1 = 0;
    TU32 param2 = 0;
    TU32 param3 = 0;
    TU32 param4 = 0;
    TU32 param5 = 0;

    //DASSERT(mpCmdCallback);
    //DASSERT(mpCmdCallbackOwner);
    if (DUnlikely(!mpCmdCallback || !mpCmdCallbackOwner)) {
        LOG_ERROR("cmd callback not set, callback %p, context %p\n", mpCmdCallback, mpCmdCallbackOwner);
        return EECode_BadState;
    }

    switch (sub_type) {

        case ESACPClientCmdSubType_SourceUpdateAudioParams: {
                ReadData(mpSACPPayload, payload_size);
                LOG_NOTICE("ESACPClientCmdSubType_SourceUpdateAudioParams, payload_size=%hu\n", payload_size);
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, mpSACPPayload,  payload_size);
            }
            break;

        case ESACPClientCmdSubType_SinkStartPlayback: {
                LOG_WARN("TO DO\n");
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader));
            }
            break;

        case ESACPClientCmdSubType_SinkStopPlayback: {
                LOG_WARN("TO DO\n");
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader));
            }
            break;

        case ESACPClientCmdSubType_SinkUpdateStoragePlan: {
                LOG_WARN("TO DO\n");
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader));
            }
            break;

        case ESACPClientCmdSubType_SinkSwitchSource: {
                LOG_WARN("TO DO\n");
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader));
            }
            break;

        case ESACPClientCmdSubType_SinkQueryRecordHistory: {
                LOG_WARN("TO DO\n");
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader));
            }
            break;

        case ESACPClientCmdSubType_SinkSeekTo: {
                LOG_WARN("TO DO\n");
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader));
            }
            break;

        case ESACPClientCmdSubType_SinkFastForward: {
                LOG_WARN("TO DO\n");
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader));
            }
            break;

        case ESACPClientCmdSubType_SinkFastBackward: {
                LOG_WARN("TO DO\n");
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader));
            }
            break;

        case ESACPClientCmdSubType_SinkTrickplay: {
                LOG_WARN("TO DO\n");
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader));
            }
            break;

        case ESACPClientCmdSubType_SinkQueryCurrentStoragePlan: {
                LOG_WARN("TO DO\n");
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader));
            }
            break;

        case ESACPClientCmdSubType_SinkPTZControl: {
                LOG_WARN("TO DO\n");
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader));
            }
            break;

        case ESACPClientCmdSubType_SinkUpdateFramerate: {
                ReadData(mpSACPPayload, payload_size);
                DBER32(param1, mpSACPPayload);
                LOG_NOTICE("ESACPClientCmdSubType_SinkUpdateFramerate, param1 %d\n", param1);
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPPayload, payload_size);
            }
            break;

        case ESACPClientCmdSubType_SinkUpdateBitrate: {
                ReadData(mpSACPPayload, payload_size);
                DBER32(param1, mpSACPPayload);
                LOG_NOTICE("ESACPClientCmdSubType_SinkUpdateBitrate, param1 %d\n", param1);
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPPayload, payload_size);
            }
            break;

        case ESACPClientCmdSubType_SinkUpdateResolution: {
                ReadData(mpSACPPayload, payload_size);
                LOG_NOTICE("ESACPClientCmdSubType_SinkUpdateBitrate\n");
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPPayload, payload_size);
            }
            break;

        case ESACPClientCmdSubType_SinkUpdateDisplayLayout: {
                DASSERT(8 == payload_size);
                ReadData(mpSACPPayload, 8);
                DBER32(param1, mpSACPPayload);
                DBER32(param2, (mpSACPPayload + 4));
                LOG_NOTICE("ESACPClientCmdSubType_SinkUpdateDisplayLayout, param1 %d, param2 %d\n", param1, param2);
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader) + 8);
            }
            break;

        case ESACPClientCmdSubType_SinkZoom: {
                DASSERT(16 == payload_size);
                ReadData(mpSACPPayload, 16);
                DBER32(param1, mpSACPPayload);
                DBER32(param2, (mpSACPPayload + 4));
                DBER32(param3, (mpSACPPayload + 8));
                DBER32(param4, (mpSACPPayload + 12));
                LOG_NOTICE("ESACPClientCmdSubType_SinkZoom, param1 %d, param2 %08x, param3 %08x, param4 %08x\n", param1, param2, param3, param4);
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader) + 16);
            }
            break;

        case ESACPClientCmdSubType_SinkUpdateDisplayWindow: {
                DASSERT(20 == payload_size);
                ReadData(mpSACPPayload, 20);
                DBER32(param1, mpSACPPayload);
                DBER32(param2, (mpSACPPayload + 4));
                DBER32(param3, (mpSACPPayload + 8));
                DBER32(param4, (mpSACPPayload + 12));
                DBER32(param5, (mpSACPPayload + 16));
                LOG_NOTICE("ESACPClientCmdSubType_SinkUpdateDisplayWindow, param1 %d, param2 %08x, param3 %08x, param4 %08x, param5 %08x\n", param1, param2, param3, param4, param5);
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader) + 20);
            }
            break;

        case ESACPClientCmdSubType_SinkFocusDisplayWindow: {
                LOG_WARN("TO DO\n");
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader));
            }
            break;

        case ESACPClientCmdSubType_SinkSelectVideoHDChannel: {
                DASSERT(4 == payload_size);
                ReadData(mpSACPPayload, 4);
                DBER32(param1, mpSACPPayload);
                LOG_NOTICE("ESACPClientCmdSubType_SinkSelectVideoHDChannel, param1 %d\n", param1);
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader) + 4);
            }
            break;

        case ESACPClientCmdSubType_SinkSelectAudioSourceChannel: {
                DASSERT(4 == payload_size);
                ReadData(mpSACPPayload, 4);
                DBER32(param1, mpSACPPayload);
                LOG_NOTICE("ESACPClientCmdSubType_SinkSelectAudioSourceChannel, param1 %d\n", param1);
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader) + 4);
            }
            break;

        case ESACPClientCmdSubType_SinkSelectAudioTargetChannel: {
                DASSERT(4 == payload_size);
                ReadData(mpSACPPayload, 4);
                DBER32(param1, mpSACPPayload);
                LOG_NOTICE("ESACPClientCmdSubType_SinkSelectAudioTargetChannel, param1 %d\n", param1);
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader) + 4);
            }
            break;

        case ESACPClientCmdSubType_SinkShowOtherWindows: {
                DASSERT(4 == payload_size);
                ReadData(mpSACPPayload, 4);
                DBER16(param1, mpSACPPayload);
                DBER16(param2, (mpSACPPayload + 2));
                LOG_NOTICE("ESACPClientCmdSubType_SinkShowOtherWindows, param1 %d, param2 %d\n", param1, param2);
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader) + 4);
            }
            break;

        case ESACPClientCmdSubType_SinkForceIDR: {
                DASSERT(4 == payload_size);
                ReadData(mpSACPPayload, 4);
                DBER32(param1, mpSACPPayload);
                LOG_NOTICE("ESACPClientCmdSubType_SinkForceIDR, param1 %d\n", param1);
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader) + 4);
            }
            break;

        case ESACPClientCmdSubType_SinkSwapWindowContent: {
                DASSERT(8 == payload_size);
                ReadData(mpSACPPayload, 8);
                DBER32(param1, mpSACPPayload);
                DBER32(param2, (mpSACPPayload + 4));
                LOG_NOTICE("ESACPClientCmdSubType_SinkSwapWindowContent, param1 %d, param2 %d\n", param1, param2);
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader) + 8);
            }
            break;

        case ESACPClientCmdSubType_SinkCircularShiftWindowContent: {
                DASSERT(8 == payload_size);
                ReadData(mpSACPPayload, 8);
                DBER32(param1, mpSACPPayload);
                DBER32(param2, (mpSACPPayload + 4));
                LOG_NOTICE("ESACPClientCmdSubType_SinkCircularShiftWindowContent, param1 %d, param2 %d\n", param1, param2);
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader) + 8);
            }
            break;

        case ESACPClientCmdSubType_SinkZoomV2: {
                DASSERT(20 == payload_size);
                ReadData(mpSACPPayload, 20);
                DBER32(param1, mpSACPPayload);
                DBER32(param2, (mpSACPPayload + 4));
                DBER32(param3, (mpSACPPayload + 8));
                DBER32(param4, (mpSACPPayload + 12));
                DBER32(param5, (mpSACPPayload + 16));
                LOG_NOTICE("ESACPClientCmdSubType_SinkZoomV2, param1 %d, param2 %08x, param3 %08x, param4 %08x, param5 %08x\n", param1, param2, param3, param4, param5);
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader) + 20);
            }
            break;

        case ESACPClientCmdSubType_SinkSwitchGroup: {
                DASSERT(4 == payload_size);
                ReadData(mpSACPPayload, 4);
                DBER32(param1, mpSACPPayload);
                LOG_NOTICE("ESACPClientCmdSubType_SinkSwitchGroup, param1 %d\n", param1);
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader) + 4);
            }
            break;

        case ESACPClientCmdSubType_QueryVersion: {
                DASSERT(8 == payload_size);
                ReadData(mpSACPPayload, 8);

                DBER32(param1, mpSACPPayload);
                DBER32(param2, (mpSACPPayload + 4));

                LOG_NOTICE("ESACPClientCmdSubType_QueryVersion, param1 %08x %08x\n", param1, param2);
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader) + 8);
            }
            break;

        case ESACPClientCmdSubType_QueryStatus: {
                DASSERT(0 == payload_size);

                LOG_NOTICE("ESACPClientCmdSubType_QueryStatus\n");
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader));
            }
            break;

        case ESACPClientCmdSubType_SyncStatus: {
                DASSERT(payload_size);

                LOG_NOTICE("ESACPClientCmdSubType_SyncStatus\n");
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader));
            }
            break;

        case ESACPClientCmdSubType_CustomizedCommand: {
                DASSERT(20 == payload_size);
                ReadData(mpSACPPayload, 20);
                DBER32(param1, mpSACPPayload);
                DBER32(param2, (mpSACPPayload + 4));
                DBER32(param3, (mpSACPPayload + 8));
                DBER32(param4, (mpSACPPayload + 12));
                DBER32(param5, (mpSACPPayload + 16));
                LOG_NOTICE("ESACPClientCmdSubType_CustomizedCommand, param1 %d, param2 %08x, param3 %08x, param4 %08x, param5 %08x\n", param1, param2, param3, param4, param5);
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader) + 20);
            }
            break;

        case ESACPDebugCmdSubType_PrintCloudPipeline: {
                DASSERT(4 == payload_size);
                ReadData(mpSACPPayload, 4);
                DBER32(param1, mpSACPPayload);
                LOG_NOTICE("ESACPDebugCmdSubType_PrintCloudPipeline, param1 %d\n", param1);
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader) + 4);
            }
            break;

        case ESACPDebugCmdSubType_PrintStreamingPipeline: {
                DASSERT(4 == payload_size);
                ReadData(mpSACPPayload, 4);
                DBER32(param1, mpSACPPayload);
                LOG_NOTICE("ESACPDebugCmdSubType_PrintStreamingPipeline, param1 %d\n", param1);
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader) + 4);
            }
            break;

        case ESACPDebugCmdSubType_PrintRecordingPipeline: {
                DASSERT(4 == payload_size);
                ReadData(mpSACPPayload, 4);
                DBER32(param1, mpSACPPayload);
                LOG_NOTICE("ESACPDebugCmdSubType_PrintRecordingPipeline, param1 %d\n", param1);
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader) + 4);
            }
            break;

        case ESACPDebugCmdSubType_PrintSingleChannel: {
                DASSERT(4 == payload_size);
                ReadData(mpSACPPayload, 4);
                DBER32(param1, mpSACPPayload);
                LOG_NOTICE("ESACPDebugCmdSubType_PrintSingleChannel, param1 %d\n", param1);
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader) + 4);
            }
            break;

        case ESACPDebugCmdSubType_PrintAllChannels: {
                DASSERT(4 == payload_size);
                ReadData(mpSACPPayload, 4);
                DBER32(param1, mpSACPPayload);
                LOG_NOTICE("ESACPDebugCmdSubType_PrintAllChannels, param1 %d\n", param1);
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader) + 4);
            }
            break;

        case ESACPClientCmdSubType_ResponceOnlyForRelay: {
                mpCmdCallback(mpCmdCallbackOwner, cmd_type, param1, param2, param3, param4, param5, (TU8 *)mpSACPHeader, sizeof(SSACPHeader) + payload_size);
            }
            break;

        default:
            err = EECode_NotSupported;
            LOG_WARN("NOT supported cmd type %d\n", sub_type);
            break;
    }

    return err;
}

EECode CSACPCloudServerAgent::processCmd(TU8 cat, TU16 sub_type, TU16 payload_size)
{
    if (ESACPTypeCategory_ClientCmdChannel == cat) {
        return processClientCmd((ESACPClientCmdSubType)sub_type, payload_size);
    } else {
        LOG_FATAL("BAD cmd categary %d\n", cat);
    }

    return EECode_BadParam;
}

EECode CSACPCloudServerAgent::Scheduling(TUint times, TU32 inout_mask)
{
    EECode err = EECode_OK;
    TInt size = 0;
    TU32 type = 0;

    TU32 payload_size = 0;
    TU16 header_ext_size = 0;

    TU8 has_header_ext = 0;

    if (DUnlikely((mbPeerClosed) || (!mbAccepted))) {
        LOG_INFO("peer closed or not accepted, mbPeerClosed %d, mbAccepted %d\n", mbPeerClosed, mbAccepted);
        return EECode_NotRunning;
    }

    size = sizeof(SSACPHeader);
    //LOG_INFO("read size %d start\n", size);
    size = gfNet_Recv_timeout(mSocket, (TU8 *)mpSACPHeader, size, DNETUTILS_RECEIVE_FLAG_READ_ALL, 4);
    //LOG_INFO("read size %d done\n", size);
    if (DLikely(size == sizeof(SSACPHeader))) {
        type = ((mpSACPHeader->type_1) << 24) | ((mpSACPHeader->type_2) << 16) | ((mpSACPHeader->type_3) << 8) | (mpSACPHeader->type_4);
        payload_size = ((TU32)mpSACPHeader->size_0 << 16) | ((TU32)mpSACPHeader->size_1 << 8) | ((TU32)mpSACPHeader->size_2);
        has_header_ext = mpSACPHeader->header_ext_type;
        header_ext_size = (mpSACPHeader->header_ext_size_1 << 8) | mpSACPHeader->header_ext_size_2;
        //DASSERT(EProtocolHeaderExtentionType_NoHeader == has_header_ext);
        //DASSERT(0 == header_ext_size);

        if (DUnlikely((EProtocolHeaderExtentionType_NoHeader != has_header_ext) || (0 != header_ext_size))) {
            LOG_FATAL("corrupted protocol, has_header_ext %d, header_ext_size %d\n", has_header_ext, header_ext_size);
            return EECode_Closed;
        }

        //DASSERT(!(type & DSACPTypeBit_Responce));
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
                TMemSize data_length = payload_size;
                //LOG_PRINTF("[server agent]: data length %ld, flag %02x\n", data_length, mpSACPHeader->flags);
                err = mpDataCallback(mpDataCallbackOwner, data_length, data_sub_type, mpSACPHeader->flags);
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

TInt CSACPCloudServerAgent::IsPassiveMode() const
{
    return 0;
}

TSchedulingHandle CSACPCloudServerAgent::GetWaitHandle() const
{
    return (TSchedulingHandle)mSocket;
}

TU8 CSACPCloudServerAgent::GetPriority() const
{
    return 1;
}

EECode CSACPCloudServerAgent::CheckTimeout()
{
    LOG_WARN("to do\n");
    return EECode_OK;
}

EECode CSACPCloudServerAgent::EventHandling(EECode err)
{
    if (EECode_Closed == err) {
        if (DLikely(mpCmdCallback && mpCmdCallbackOwner)) {
            mpCmdCallback(mpCmdCallbackOwner, ESACPClientCmdSubType_PeerClose, 0, 0, 0, 0, 0, NULL, 0);
        } else {
            LOG_ERROR("NULL mpCmdCallback %p or mpCmdCallbackOwner %p\n", mpCmdCallback, mpCmdCallbackOwner);
        }
        LOG_ERROR("peer close\n");
        mbPeerClosed = 1;
    }
    return EECode_OK;
}

TSchedulingUnit CSACPCloudServerAgent::HungryScore() const
{
    LOG_WARN("to do\n");
    return 1;
}

void CSACPCloudServerAgent::Destroy()
{
    Delete();
}

EECode CSACPCloudServerAgent::AcceptClient(TInt socket)
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

EECode CSACPCloudServerAgent::CloseConnection()
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

EECode CSACPCloudServerAgent::ReadData(TU8 *p_data, TMemSize size)
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

EECode CSACPCloudServerAgent::WriteData(TU8 *p_data, TMemSize size)
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

EECode CSACPCloudServerAgent::SetProcessCMDCallBack(void *owner, TServerAgentCmdCallBack callback)
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

EECode CSACPCloudServerAgent::SetProcessDataCallBack(void *owner, TServerAgentDataCallBack callback)
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

EECode CSACPCloudServerAgent::SetProcessCMDCallBackV2(void *owner, TServerAgentV2CmdCallBack callback)
{
    LOG_FATAL("not version 1\n");
    return EECode_NotSupported;
}

EECode CSACPCloudServerAgent::SetProcessDataCallBackV2(void *owner, TServerAgentV2DataCallBack callback)
{
    LOG_FATAL("not version 1\n");
    return EECode_NotSupported;
}

void CSACPCloudServerAgent::Delete()
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

