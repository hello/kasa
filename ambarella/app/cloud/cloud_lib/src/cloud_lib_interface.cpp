/*******************************************************************************
 * cloud_lib_interface.cpp
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

#include "cloud_lib_if.h"

#include "sacp_types.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

void gfGetCloudLibVersion(TU32 &major, TU32 &minor, TU32 &patch, TU32 &year, TU32 &month, TU32 &day)
{
    major = DCloudLibVesionMajor;
    minor = DCloudLibVesionMinor;
    patch = DCloudLibVesionPatch;

    year = DCloudLibVesionYear;
    month = DCloudLibVesionMonth;
    day = DCloudLibVesionDay;
}

ECMDType gfSACPClientSubCmdTypeToGenericCmdType(TU16 sub_type)
{
    switch (sub_type) {

        case ESACPClientCmdSubType_SourceUpdateAudioParams:
            return ECMDType_CloudSourceClient_UpdateAudioParams;
            break;

        case ESACPClientCmdSubType_SourceUpdateVideoParams:
            return ECMDType_CloudSourceClient_UpdateVideoParams;
            break;

        case ESACPClientCmdSubType_DiscIndicator:
            return ECMDType_CloudSourceClient_DiscIndicator;
            break;

        case ESACPClientCmdSubType_VideoSync:
            return ECMDType_CloudSourceClient_VideoSync;
            break;

        case ESACPClientCmdSubType_AudioSync:
            return ECMDType_CloudSourceClient_AudioSync;
            break;

        case ESACPClientCmdSubType_SinkUpdateBitrate:
            return ECMDType_CloudSinkClient_UpdateBitrate;
            break;

        case ESACPClientCmdSubType_SinkUpdateFramerate:
            return ECMDType_CloudSinkClient_UpdateFrameRate;
            break;

        case ESACPClientCmdSubType_SinkUpdateDisplayLayout:
            return ECMDType_CloudSinkClient_UpdateDisplayLayout;
            break;

        case ESACPClientCmdSubType_SinkZoom:
            return ECMDType_CloudSinkClient_Zoom;
            break;

        case ESACPClientCmdSubType_SinkSelectAudioSourceChannel:
            return ECMDType_CloudSinkClient_SelectAudioSource;
            break;

        case ESACPClientCmdSubType_SinkSelectAudioTargetChannel:
            return ECMDType_CloudSinkClient_SelectAudioTarget;
            break;

        case ESACPClientCmdSubType_SinkUpdateDisplayWindow:
            return ECMDType_CloudSinkClient_UpdateDisplayWindow;
            break;

        case ESACPClientCmdSubType_SinkSelectVideoHDChannel:
            return ECMDType_CloudSinkClient_SelectVideoSource;
            break;

        case ESACPClientCmdSubType_SinkShowOtherWindows:
            return ECMDType_CloudSinkClient_ShowOtherWindows;
            break;

        case ESACPClientCmdSubType_SinkForceIDR:
            return ECMDType_CloudSinkClient_DemandIDR;
            break;

        case ESACPClientCmdSubType_SinkSwapWindowContent:
            return ECMDType_CloudSinkClient_SwapWindowContent;
            break;

        case ESACPClientCmdSubType_SinkCircularShiftWindowContent:
            return ECMDType_CloudSinkClient_CircularShiftWindowContent;
            break;

        case ESACPClientCmdSubType_SinkZoomV2:
            return ECMDType_CloudSinkClient_ZoomV2;
            break;

        case ESACPClientCmdSubType_SinkSwitchGroup:
            return ECMDType_CloudSinkClient_SwitchGroup;
            break;

        case ESACPClientCmdSubType_PeerClose:
            return ECMDType_CloudClient_PeerClose;
            break;

        case ESACPClientCmdSubType_QueryVersion:
            return ECMDType_CloudClient_QueryVersion;
            break;

        case ESACPClientCmdSubType_QueryStatus:
            return ECMDType_CloudClient_QueryStatus;
            break;

        case ESACPClientCmdSubType_SyncStatus:
            return ECMDType_CloudClient_SyncStatus;
            break;

        case ESACPClientCmdSubType_QuerySourceList:
            return ECMDType_CloudClient_QuerySourceList;
            break;

        case ESACPClientCmdSubType_AquireChannelControl:
            return ECMDType_CloudClient_AquireChannelControl;
            break;

        case ESACPClientCmdSubType_ReleaseChannelControl:
            return ECMDType_CloudClient_ReleaseChannelControl;
            break;

        case ESACPClientCmdSubType_CustomizedCommand:
            return ECMDType_CloudClient_CustomizedCommand;
            break;

        case ESACPDebugCmdSubType_PrintCloudPipeline:
            return ECMDType_DebugClient_PrintCloudPipeline;
            break;

        case ESACPDebugCmdSubType_PrintStreamingPipeline:
            return ECMDType_DebugClient_PrintStreamingPipeline;
            break;

        case ESACPDebugCmdSubType_PrintRecordingPipeline:
            return ECMDType_DebugClient_PrintRecordingPipeline;
            break;

        case ESACPDebugCmdSubType_PrintSingleChannel:
            return ECMDType_DebugClient_PrintSingleChannel;
            break;

        case ESACPDebugCmdSubType_PrintAllChannels:
            return ECMDType_DebugClient_PrintAllChannels;
            break;

        case ESACPClientCmdSubType_SinkUpdateResolution:
            return ECMDType_CloudSinkClient_UpdateResolution;
            break;

        default:
            LOG_FATAL("not supported SACP client cmd %hx\n", sub_type);
            break;
    }

    return ECMDType_CloudSourceClient_UnknownCmd;
}

EECode gfSACPConnectResultToErrorCode(ESACPConnectResult result)
{
    switch (result) {

        case ESACPConnectResult_OK:
            //LOG_NOTICE("ESACPConnectResult_OK\n");
            return EECode_OK;
            break;

        case ESACPConnectResult_Reject_NoSuchChannel:
            LOG_ERROR("ESACPConnectResult_Reject_NoSuchChannel\n");
            return EECode_ServerReject_NoSuchChannel;
            break;

        case ESACPConnectResult_Reject_ChannelIsInUse:
            LOG_ERROR("ESACPConnectResult_Reject_ChannelIsInUse\n");
            return EECode_ServerReject_ChannelIsBusy;
            break;

        case ESACPConnectResult_Reject_BadRequestFormat:
            LOG_ERROR("ESACPConnectResult_Reject_BadRequestFormat\n");
            return EECode_ServerReject_BadRequestFormat;
            break;

        case ESACPConnectResult_Reject_CorruptedProtocol:
            LOG_ERROR("ESACPConnectResult_Reject_CorruptedProtocol\n");
            return EECode_ServerReject_CorruptedProtocol;
            break;

        case ESACPConnectResult_Reject_AuthenticationDataTooLong:
            LOG_ERROR("ESACPConnectResult_Reject_AuthenticationDataTooLong\n");
            return EECode_ServerReject_AuthenticationDataTooLong;
            break;

        case ESACPConnectResult_Reject_NotProperPassword:
            LOG_ERROR("ESACPConnectResult_Reject_NotProperPassword\n");
            return EECode_ServerReject_NotProperPassword;
            break;

        case ESACPConnectResult_Reject_NotSupported:
            LOG_ERROR("ESACPConnectResult_Reject_NotSupported\n");
            return EECode_ServerReject_NotSupported;
            break;

        case ESACPConnectResult_Reject_AuthenticationFail:
            LOG_ERROR("ESACPConnectResult_Reject_AuthenticationFail\n");
            return EECode_ServerReject_AuthenticationFail;
            break;

        case ESACPConnectResult_Reject_Unknown:
            LOG_ERROR("ESACPConnectResult_Reject_Unknown\n");
            return EECode_ServerReject_Unknown;
            break;

        case ESACPRequestResult_BadState:
            LOG_ERROR("ESACPRequestResult_BadState\n");
            return EECode_BadState;
            break;

        case ESACPRequestResult_NotExist:
            LOG_ERROR("ESACPRequestResult_NotExist\n");
            return EECode_NotExist;
            break;

        case ESACPRequestResult_BadParam:
            LOG_ERROR("ESACPRequestResult_BadParam\n");
            return EECode_BadParam;
            break;

        case ESACPRequestResult_ServerLogicalBug:
            LOG_ERROR("ESACPRequestResult_ServerLogicalBug\n");
            return EECode_InternalLogicalBug;
            break;

        case ESACPRequestResult_ServerNotSupport:
            LOG_ERROR("ESACPRequestResult_ServerNotSupport\n");
            return EECode_NotSupported;
            break;

#if 0
        case ESACPConnectResult_Reject_NoUsername:
            LOG_ERROR("ESACPConnectResult_Reject_NoUsername\n");
            return EECode_ServerReject_NoUsername;
            break;

        case ESACPConnectResult_Reject_TooLongUsername:
            LOG_ERROR("ESACPConnectResult_Reject_TooLongUsername\n");
            return EECode_ServerReject_TooLongUsername;
            break;

        case ESACPConnectResult_Reject_NoPassword:
            LOG_ERROR("ESACPConnectResult_Reject_NoPassword\n");
            return EECode_ServerReject_NoPassword;
            break;

        case ESACPConnectResult_Reject_TooLongPassword:
            LOG_ERROR("ESACPConnectResult_Reject_TooLongPassword\n");
            return EECode_ServerReject_TooLongPassword;
            break;
#endif

        case ESACPRequestResult_NotOnline:
            LOG_ERROR("ESACPRequestResult_NotOnline\n");
            return EECode_NotOnline;
            break;

        case ESACPRequestResult_AlreadyExist:
            LOG_ERROR("ESACPRequestResult_AlreadyExist\n");
            return EECode_AlreadyExist;
            break;

        case ESACPRequestResult_PossibleAttackerFromNetwork:
            return EECode_PossibleAttackFromNetwork;
            break;

        case ESACPRequestResult_Server_NeedHardwareAuthenticate:
            return EECode_OK_NeedHardwareAuthenticate;
            break;

        case ESACPRequestResult_NeedSetOwner:
            return EECode_OK_NeedSetOwner;
            break;

        default:
            LOG_FATAL("not supported result %hx\n", result);
            break;
    }

    return EECode_NotSupported;
}

ESACPConnectResult gfSACPErrorCodeToConnectResult(EECode code)
{
    switch (code) {

        case EECode_OK:
            return ESACPConnectResult_OK;
            break;

        case EECode_OK_NeedSetOwner:
            return ESACPRequestResult_NeedSetOwner;
            break;

        case EECode_OK_NeedHardwareAuthenticate:
            return ESACPRequestResult_Server_NeedHardwareAuthenticate;
            break;

        case EECode_ServerReject_NoSuchChannel:
            return ESACPConnectResult_Reject_NoSuchChannel;
            break;

        case EECode_ServerReject_ChannelIsBusy:
            return ESACPConnectResult_Reject_ChannelIsInUse;
            break;

        case EECode_ServerReject_BadRequestFormat:
            return ESACPConnectResult_Reject_BadRequestFormat;
            break;

        case EECode_ServerReject_CorruptedProtocol:
            return ESACPConnectResult_Reject_CorruptedProtocol;
            break;

        case EECode_ServerReject_AuthenticationDataTooLong:
            return ESACPConnectResult_Reject_AuthenticationDataTooLong;
            break;

        case EECode_ServerReject_NotProperPassword:
            return ESACPConnectResult_Reject_NotProperPassword;
            break;

        case EECode_ServerReject_NotSupported:
            return ESACPConnectResult_Reject_NotSupported;
            break;

        case EECode_ServerReject_AuthenticationFail:
            return ESACPConnectResult_Reject_AuthenticationFail;
            break;

        case EECode_ServerReject_Unknown:
            return ESACPConnectResult_Reject_Unknown;
            break;

        case EECode_BadState:
            return ESACPRequestResult_BadState;
            break;

        case EECode_NotExist:
            return ESACPRequestResult_NotExist;
            break;

        case EECode_NotOnline:
            return ESACPRequestResult_NotOnline;
            break;

        case EECode_BadParam:
            return ESACPRequestResult_BadParam;
            break;

        case EECode_InternalLogicalBug:
            return ESACPRequestResult_ServerLogicalBug;
            break;

        case EECode_NotSupported:
            return ESACPRequestResult_ServerNotSupport;
            break;

        case EECode_NoPermission:
            return ESACPRequestResult_NoPermission;
            break;

#if 0
        case EECode_ServerReject_NoUsername:
            return ESACPConnectResult_Reject_NoUsername;
            break;

        case EECode_ServerReject_TooLongUsername:
            return ESACPConnectResult_Reject_TooLongUsername;
            break;

        case EECode_ServerReject_NoPassword:
            return ESACPConnectResult_Reject_NoPassword;
            break;

        case EECode_ServerReject_TooLongPassword:
            return ESACPConnectResult_Reject_TooLongPassword;
            break;
#endif

        case EECode_AlreadyExist:
            return ESACPRequestResult_AlreadyExist;
            break;

        case EECode_PossibleAttackFromNetwork:
            return ESACPRequestResult_PossibleAttackerFromNetwork;
            break;

        default:
            LOG_FATAL("not supported error code %x\n", code);
            break;
    }

    return ESACPConnectResult_Reject_Unknown;
}

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE
extern ICloudClientAgent *gfCreateSACPCloudClientAgent(TU16 local_port, TU16 server_port);
extern ICloudServerAgent *gfCreateSACPCloudServerAgent();
extern IIMClientAgent *gfCreateSACPIMClientAgent();
extern IIMClientAgentAsync *gfCreateSACPIMClientAgentAsync();
#endif

extern ICloudClientAgentV2 *gfCreateSACPCloudClientAgentV2();
extern ICloudServerAgent *gfCreateSACPCloudServerAgentV2();
extern IProtocolHeaderParser *gfCreateSACPProtocolHeaderParser(EProtocolType type, EProtocolHeaderExtentionType extensiontype);
extern IIMClientAgent *gfCreateSACPIMClientAgentV2();

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE
ICloudClientAgent *gfCreateCloudClientAgent(EProtocolType type, TU16 local_port, TU16 server_port)
{
    ICloudClientAgent *thiz = NULL;

    switch (type) {

        case EProtocolType_SACP:
            thiz = gfCreateSACPCloudClientAgent(local_port, server_port);
            break;

        default:
            LOG_FATAL("EProtocolType %d not implemented\n", type);
            break;
    }

    return thiz;
}
#endif

ICloudClientAgentV2 *gfCreateCloudClientAgentV2(EProtocolType type)
{
    ICloudClientAgentV2 *thiz = NULL;

    switch (type) {

        case EProtocolType_SACP:
            thiz = gfCreateSACPCloudClientAgentV2();
            break;

        default:
            LOG_FATAL("EProtocolType %d not implemented\n", type);
            break;
    }

    return thiz;
}

ICloudServerAgent *gfCreateCloudServerAgent(EProtocolType type, TU32 version)
{
    ICloudServerAgent *thiz = NULL;

    switch (type) {

        case EProtocolType_SACP:
            thiz = gfCreateSACPCloudServerAgentV2();
            break;

        default:
            LOG_FATAL("EProtocolType %d not implemented\n", type);
            break;
    }

    return thiz;
}

IProtocolHeaderParser *gfCreatePotocolHeaderParser(EProtocolType type, EProtocolHeaderExtentionType extensiontype)
{
    IProtocolHeaderParser *thiz = NULL;

    switch (type) {

        case EProtocolType_SACP:
            thiz = gfCreateSACPProtocolHeaderParser(type, extensiontype);
            break;

        default:
            LOG_FATAL("EProtocolType %d not implemented\n", type);
            break;
    }

    return thiz;
}

IIMClientAgent *gfCreateIMClientAgent(EProtocolType type, TU32 version)
{
    IIMClientAgent *thiz = NULL;

    switch (type) {

        case EProtocolType_SACP:
            thiz = gfCreateSACPIMClientAgentV2();
            break;

        default:
            LOG_FATAL("EProtocolType %d not implemented\n", type);
            break;
    }

    return thiz;
}

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE
IIMClientAgentAsync *gfCreateIMClientAgentAsync(EProtocolType type)
{
    IIMClientAgentAsync *thiz = NULL;

    switch (type) {

        case EProtocolType_SACP:
            thiz = gfCreateSACPIMClientAgentAsync();
            break;

        default:
            LOG_FATAL("EProtocolType %d not implemented\n", type);
            break;
    }

    return thiz;
}
#endif

//utils functions
void gfSACPFillHeader(SSACPHeader *p_header, TU32 type, TU32 size, TU8 ext_type, TU16 ext_size, TU32 seq_count, TU8 flag)
{
    p_header->type_1 = (type >> 24) & 0xff;
    p_header->type_2 = (type >> 16) & 0xff;
    p_header->type_3 = (type >> 8) & 0xff;
    p_header->type_4 = type & 0xff;

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

const TChar *gfGetSACPClientCmdSubtypeString(ESACPClientCmdSubType sub_type)
{
    switch (sub_type) {
        case ESACPClientCmdSubType_Invalid:
            return "ESACPClientCmdSubType_Invalid";
            break;

        case ESACPClientCmdSubType_SourceAuthentication:
            return "ESACPClientCmdSubType_SourceAuthentication";
            break;

        case ESACPClientCmdSubType_SinkRegister:
            return "ESACPClientCmdSubType_SinkRegister";
            break;

        case ESACPClientCmdSubType_SinkUnRegister:
            return "ESACPClientCmdSubType_SinkUnRegister";
            break;

        case ESACPClientCmdSubType_SinkAuthentication:
            return "ESACPClientCmdSubType_SinkAuthentication";
            break;

        case ESACPClientCmdSubType_SinkQuerySource:
            return "ESACPClientCmdSubType_SinkQuerySource";
            break;

        case ESACPClientCmdSubType_SinkQuerySourcePrivilege:
            return "ESACPClientCmdSubType_SinkQuerySourcePrivilege";
            break;

        case ESACPClientCmdSubType_SinkOwnSourceDevice:
            return "ESACPClientCmdSubType_SinkOwnSourceDevice";
            break;

        case ESACPClientCmdSubType_SinkDonateSourceDevice:
            return "ESACPClientCmdSubType_SinkDonateSourceDevice";
            break;

        case ESACPClientCmdSubType_SinkAcceptDeviceDonation:
            return "ESACPClientCmdSubType_SinkAcceptDeviceDonation";
            break;

        case ESACPClientCmdSubType_SinkInviteFriend:
            return "ESACPClientCmdSubType_SinkInviteFriend";
            break;

        case ESACPClientCmdSubType_SinkAcceptFriend:
            return "ESACPClientCmdSubType_SinkAcceptFriend";
            break;

        case ESACPClientCmdSubType_SinkRemoveFriend:
            return "ESACPClientCmdSubType_SinkRemoveFriend";
            break;

        case ESACPClientCmdSubType_SinkUpdateFriendPrivilege:
            return "ESACPClientCmdSubType_SinkUpdateFriendPrivilege";
            break;

        case ESACPClientCmdSubType_SinkRetrieveFriendsList:
            return "ESACPClientCmdSubType_SinkRetrieveFriendsList";
            break;

        case ESACPClientCmdSubType_SinkRetrieveAdminDeviceList:
            return "ESACPClientCmdSubType_SinkRetrieveAdminDeviceList";
            break;

        case ESACPClientCmdSubType_SinkRetrieveSharedDeviceList:
            return "ESACPClientCmdSubType_SinkRetrieveSharedDeviceList";
            break;

        case ESACPClientCmdSubType_SinkQueryFriendInfo:
            return "ESACPClientCmdSubType_SinkQueryFriendInfo";
            break;

        case ESACPClientCmdSubType_SinkInviteFriendByID:
            return "ESACPClientCmdSubType_SinkInviteFriendByID";
            break;

        case ESACPClientCmdSubType_SourceUpdateAudioParams:
            return "ESACPClientCmdSubType_SourceUpdateAudioParams";
            break;

        case ESACPClientCmdSubType_SourceUpdateVideoParams:
            return "ESACPClientCmdSubType_SourceUpdateVideoParams";
            break;

        case ESACPClientCmdSubType_DiscIndicator:
            return "ESACPClientCmdSubType_DiscIndicator";
            break;

        case ESACPClientCmdSubType_VideoSync:
            return "ESACPClientCmdSubType_VideoSync";
            break;

        case ESACPClientCmdSubType_AudioSync:
            return "ESACPClientCmdSubType_AudioSync";
            break;

        case ESACPClientCmdSubType_SinkStartPlayback:
            return "ESACPClientCmdSubType_SinkStartPlayback";
            break;

        case ESACPClientCmdSubType_SinkStopPlayback:
            return "ESACPClientCmdSubType_SinkStopPlayback";
            break;

        case ESACPClientCmdSubType_SinkUpdateStoragePlan:
            return "ESACPClientCmdSubType_SinkUpdateStoragePlan";
            break;

        case ESACPClientCmdSubType_SinkSwitchSource:
            return "ESACPClientCmdSubType_SinkSwitchSource";
            break;

        case ESACPClientCmdSubType_SinkQueryRecordHistory:
            return "ESACPClientCmdSubType_SinkQueryRecordHistory";
            break;

        case ESACPClientCmdSubType_SinkSeekTo:
            return "ESACPClientCmdSubType_SinkSeekTo";
            break;

        case ESACPClientCmdSubType_SinkFastForward:
            return "ESACPClientCmdSubType_SinkFastForward";
            break;

        case ESACPClientCmdSubType_SinkFastBackward:
            return "ESACPClientCmdSubType_SinkFastBackward";
            break;

        case ESACPClientCmdSubType_SinkTrickplay:
            return "ESACPClientCmdSubType_SinkTrickplay";
            break;

        case ESACPClientCmdSubType_SinkQueryCurrentStoragePlan:
            return "ESACPClientCmdSubType_SinkQueryCurrentStoragePlan";
            break;

        case ESACPClientCmdSubType_SinkPTZControl:
            return "ESACPClientCmdSubType_SinkPTZControl";
            break;

        case ESACPClientCmdSubType_SinkUpdateFramerate:
            return "ESACPClientCmdSubType_SinkUpdateFramerate";
            break;

        case ESACPClientCmdSubType_SinkUpdateBitrate:
            return "ESACPClientCmdSubType_SinkUpdateBitrate";
            break;

        case ESACPClientCmdSubType_SinkUpdateResolution:
            return "ESACPClientCmdSubType_SinkUpdateResolution";
            break;

        case ESACPClientCmdSubType_SinkUpdateDisplayLayout:
            return "ESACPClientCmdSubType_SinkUpdateDisplayLayout";
            break;

        case ESACPClientCmdSubType_SinkZoom:
            return "ESACPClientCmdSubType_SinkZoom";
            break;

        case ESACPClientCmdSubType_SinkUpdateDisplayWindow:
            return "ESACPClientCmdSubType_SinkUpdateDisplayWindow";
            break;

        case ESACPClientCmdSubType_SinkFocusDisplayWindow:
            return "ESACPClientCmdSubType_SinkFocusDisplayWindow";
            break;

        case ESACPClientCmdSubType_SinkSelectAudioSourceChannel:
            return "ESACPClientCmdSubType_SinkSelectAudioSourceChannel";
            break;

        case ESACPClientCmdSubType_SinkSelectAudioTargetChannel:
            return "ESACPClientCmdSubType_SinkSelectAudioTargetChannel";
            break;

        case ESACPClientCmdSubType_SinkSelectVideoHDChannel:
            return "ESACPClientCmdSubType_SinkSelectVideoHDChannel";
            break;

        case ESACPClientCmdSubType_SinkShowOtherWindows:
            return "ESACPClientCmdSubType_SinkShowOtherWindows";
            break;

        case ESACPClientCmdSubType_SinkForceIDR:
            return "ESACPClientCmdSubType_SinkForceIDR";
            break;

        case ESACPClientCmdSubType_SinkSwapWindowContent:
            return "ESACPClientCmdSubType_SinkSwapWindowContent";
            break;

        case ESACPClientCmdSubType_SinkCircularShiftWindowContent:
            return "ESACPClientCmdSubType_SinkCircularShiftWindowContent";
            break;

        case ESACPClientCmdSubType_SinkZoomV2:
            return "ESACPClientCmdSubType_SinkZoomV2";
            break;

        case ESACPClientCmdSubType_SinkSwitchGroup:
            return "ESACPClientCmdSubType_SinkSwitchGroup";
            break;

        case ESACPClientCmdSubType_UpdateSinkNickname:
            return "ESACPClientCmdSubType_UpdateSinkNickname";
            break;

        case ESACPClientCmdSubType_UpdateSourceNickname:
            return "ESACPClientCmdSubType_UpdateSourceNickname";
            break;

        case ESACPClientCmdSubType_UpdateSinkPassword:
            return "ESACPClientCmdSubType_UpdateSinkPassword";
            break;

        case ESACPClientCmdSubType_SetSecureQuestion:
            return "ESACPClientCmdSubType_SetSecureQuestion";
            break;

        case ESACPClientCmdSubType_GetSecureQuestion:
            return "ESACPClientCmdSubType_GetSecureQuestion";
            break;

        case ESACPClientCmdSubType_ResetPassword:
            return "ESACPClientCmdSubType_ResetPassword";
            break;

        case ESACPClientCmdSubType_SourceSetOwner:
            return "ESACPClientCmdSubType_SourceSetOwner";
            break;

        case ESACPClientCmdSubType_PeerClose:
            return "ESACPClientCmdSubType_PeerClose";
            break;

        case ESACPClientCmdSubType_QueryVersion:
            return "ESACPClientCmdSubType_QueryVersion";
            break;

        case ESACPClientCmdSubType_QueryStatus:
            return "ESACPClientCmdSubType_QueryStatus";
            break;

        case ESACPClientCmdSubType_SyncStatus:
            return "ESACPClientCmdSubType_SyncStatus";
            break;

        case ESACPClientCmdSubType_QuerySourceList:
            return "ESACPClientCmdSubType_QuerySourceList";
            break;

        case ESACPClientCmdSubType_AquireChannelControl:
            return "ESACPClientCmdSubType_AquireChannelControl";
            break;

        case ESACPClientCmdSubType_ReleaseChannelControl:
            return "ESACPClientCmdSubType_ReleaseChannelControl";
            break;

        case ESACPClientCmdSubType_CustomizedCommand:
            return "ESACPClientCmdSubType_CustomizedCommand";
            break;

        case ESACPDebugCmdSubType_PrintCloudPipeline:
            return "ESACPDebugCmdSubType_PrintCloudPipeline";
            break;

        case ESACPDebugCmdSubType_PrintStreamingPipeline:
            return "ESACPDebugCmdSubType_PrintStreamingPipeline";
            break;

        case ESACPDebugCmdSubType_PrintRecordingPipeline:
            return "ESACPDebugCmdSubType_PrintRecordingPipeline";
            break;

        case ESACPDebugCmdSubType_PrintSingleChannel:
            return "ESACPDebugCmdSubType_PrintSingleChannel";
            break;

        case ESACPDebugCmdSubType_PrintAllChannels:
            return "ESACPDebugCmdSubType_PrintAllChannels";
            break;

        case ESACPClientCmdSubType_ResponceOnlyForRelay:
            return "ESACPClientCmdSubType_ResponceOnlyForRelay";
            break;

        case ESACPClientCmdSubType_SystemNotifyFriendInvitation:
            return "ESACPClientCmdSubType_SystemNotifyFriendInvitation";
            break;

        case ESACPClientCmdSubType_SystemNotifyDeviceDonation:
            return "ESACPClientCmdSubType_SystemNotifyDeviceDonation";
            break;

        case ESACPClientCmdSubType_SystemNotifyDeviceSharing:
            return "ESACPClientCmdSubType_SystemNotifyDeviceSharing";
            break;

        case ESACPClientCmdSubType_SystemNotifyUpdateOwnedDeviceList:
            return "ESACPClientCmdSubType_SystemNotifyUpdateOwnedDeviceList";
            break;

        case ESACPClientCmdSubType_SystemNotifyDeviceOwnedByUser:
            return "ESACPClientCmdSubType_SystemNotifyDeviceOwnedByUser";
            break;

        case ESACPClientCmdSubType_SinkAcquireDynamicCode:
            return "ESACPClientCmdSubType_SinkAcquireDynamicCode";
            break;

        case ESACPClientCmdSubType_SinkReleaseDynamicCode:
            return "ESACPClientCmdSubType_SinkReleaseDynamicCode";
            break;

        case ESACPClientCmdSubType_SystemNotifyTargetOffline:
            return "ESACPClientCmdSubType_SystemNotifyTargetOffline";
            break;

        case ESACPClientCmdSubType_SystemNotifyTargetNotReachable:
            return "ESACPClientCmdSubType_SystemNotifyTargetNotReachable";
            break;

        case ESACPCmdSubType_UserCustomizedDefaultTLV8:
            return "ESACPCmdSubType_UserCustomizedDefaultTLV8";
            break;

        case ESACPCmdSubType_UserCustomizedSmartSharing:
            return "ESACPCmdSubType_UserCustomizedSmartSharing";
            break;

        default:
            break;
    }

    return "unknown ESACPCmdSubType_xxx";
}

const TChar *gfGetSACPAdminCmdSubtypeString(ESACPAdminSubType sub_type)
{
    switch (sub_type) {

        case ESACPAdminSubType_Invalid:
            return "ESACPAdminSubType_Invalid";
            break;

        case ESACPAdminSubType_QuerySourceViaAccountName:
            return "ESACPAdminSubType_QuerySourceViaAccountName";
            break;

        case ESACPAdminSubType_QuerySourceViaAccountID:
            return "ESACPAdminSubType_QuerySourceViaAccountID";
            break;

        case ESACPAdminSubType_NewSourceAccount:
            return "ESACPAdminSubType_NewSourceAccount";
            break;

        case ESACPAdminSubType_DeleteSourceAccount:
            return "ESACPAdminSubType_DeleteSourceAccount";
            break;

        case ESACPAdminSubType_UpdateSourceAccount:
            return "ESACPAdminSubType_UpdateSourceAccount";
            break;

        case ESACPAdminSubType_QuerySinkViaAccountName:
            return "ESACPAdminSubType_QuerySinkViaAccountName";
            break;

        case ESACPAdminSubType_QuerySinkViaAccountID:
            return "ESACPAdminSubType_QuerySinkViaAccountID";
            break;

        case ESACPAdminSubType_QuerySinkViaAccountSecurityContact:
            return "ESACPAdminSubType_QuerySinkViaAccountSecurityContact";
            break;

        case ESACPAdminSubType_NewSinkAccount:
            return "ESACPAdminSubType_NewSinkAccount";
            break;

        case ESACPAdminSubType_DeleteSinkAccount:
            return "ESACPAdminSubType_DeleteSinkAccount";
            break;

        case ESACPAdminSubType_UpdateSinkAccount:
            return "ESACPAdminSubType_UpdateSinkAccount";
            break;

        case ESACPAdminSubType_NewDataNode:
            return "ESACPAdminSubType_NewDataNode";
            break;

        case ESACPAdminSubType_UpdateDataNode:
            return "ESACPAdminSubType_UpdateDataNode";
            break;

        case ESACPAdminSubType_QueryAllDataNode:
            return "ESACPAdminSubType_QueryAllDataNode";
            break;

        case ESACPAdminSubType_QueryDataNodeViaName:
            return "ESACPAdminSubType_QueryDataNodeViaName";
            break;

        case ESACPAdminSubType_QueryDataNodeViaID:
            return "ESACPAdminSubType_QueryDataNodeViaID";
            break;

        case ESACPAdminSubType_QueryDataServerStatus:
            return "ESACPAdminSubType_QueryDataServerStatus";
            break;

        case ESACPAdminSubType_AssignDataServerSourceDevice:
            return "ESACPAdminSubType_AssignDataServerSourceDevice";
            break;

        case ESACPAdminSubType_RemoveDataServerSourceDevice:
            return "ESACPAdminSubType_RemoveDataServerSourceDevice";
            break;

        case ESACPAdminSubType_SetSourceDynamicPassword:
            return "ESACPAdminSubType_SetSourceDynamicPassword";
            break;

        case ESACPAdminSubType_AddSinkDynamicPassword:
            return "ESACPAdminSubType_AddSinkDynamicPassword";
            break;

        case ESACPAdminSubType_RemoveSinkDynamicPassword:
            return "ESACPAdminSubType_RemoveSinkDynamicPassword";
            break;

        case ESACPAdminSubType_UpdateDeviceDynamicCode:
            return "ESACPAdminSubType_UpdateDeviceDynamicCode";
            break;

        case ESACPAdminSubType_UpdateUserDynamicCode:
            return "ESACPAdminSubType_UpdateUserDynamicCode";
            break;

        case ESACPAdminSubType_QuerySourceAccountStatus:
            return "ESACPAdminSubType_QuerySourceAccountStatus";
            break;

        case ESACPAdminSubType_QuerySinkAccountStatus:
            return "ESACPAdminSubType_QuerySinkAccountStatus";
            break;

        case ESACPAdminSubType_SetupDeviceFactoryInfo:
            return "ESACPAdminSubType_SetupDeviceFactoryInfo";
            break;
    }

    return "unknown ESACPAdminSubType_xxx";
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

