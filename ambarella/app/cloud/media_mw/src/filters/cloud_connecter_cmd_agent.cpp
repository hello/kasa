/*******************************************************************************
 * cloud_connecter_server_agent.cpp
 *
 * History:
 *    2014/03/19 - [Zhi He] create file
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

#include "media_mw_if.h"
#include "media_mw_utils.h"
#include "mw_internal.h"

#include "framework_interface.h"
#include "filters_interface.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "cloud_lib_if.h"
#include "sacp_types.h"

#include "cloud_connecter_cmd_agent.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

static TU16 needRelayCmmand(ESACPClientCmdSubType cmd_type)
{
    switch (cmd_type) {

            //case ESACPClientCmdSubType_SourceRegister:
            //case ESACPClientCmdSubType_SourceUnRegister:
        case ESACPClientCmdSubType_SourceAuthentication:
        case ESACPClientCmdSubType_SinkRegister:
        case ESACPClientCmdSubType_SinkUnRegister:
        case ESACPClientCmdSubType_SinkAuthentication:
        case ESACPClientCmdSubType_SinkQuerySourcePrivilege:
        case ESACPClientCmdSubType_SourceUpdateAudioParams:
            LOG_WARN("not support yet, %d\n", cmd_type);
            return 0;
            break;

            //case ESACPClientCmdSubType_SinkReAuthentication:
        case ESACPClientCmdSubType_SinkStartPlayback:
        case ESACPClientCmdSubType_SinkStopPlayback:
        case ESACPClientCmdSubType_SinkUpdateStoragePlan:
        case ESACPClientCmdSubType_SinkSwitchSource:
        case ESACPClientCmdSubType_SinkQueryRecordHistory:
        case ESACPClientCmdSubType_SinkSeekTo:
        case ESACPClientCmdSubType_SinkFastForward:
        case ESACPClientCmdSubType_SinkFastBackward:
        case ESACPClientCmdSubType_SinkTrickplay:
        case ESACPClientCmdSubType_SinkQueryCurrentStoragePlan:
        case ESACPClientCmdSubType_SinkPTZControl:
        case ESACPClientCmdSubType_SinkUpdateFramerate:
        case ESACPClientCmdSubType_SinkUpdateBitrate:
        case ESACPClientCmdSubType_SinkUpdateResolution:

        case ESACPClientCmdSubType_SinkUpdateDisplayLayout:
        case ESACPClientCmdSubType_SinkZoom:
        case ESACPClientCmdSubType_SinkUpdateDisplayWindow:
        case ESACPClientCmdSubType_SinkFocusDisplayWindow:
        case ESACPClientCmdSubType_SinkSelectAudioSourceChannel:
        case ESACPClientCmdSubType_SinkSelectAudioTargetChannel:
        case ESACPClientCmdSubType_SinkSelectVideoHDChannel:
        case ESACPClientCmdSubType_SinkShowOtherWindows:
        case ESACPClientCmdSubType_SinkForceIDR:
        case ESACPClientCmdSubType_SinkSwapWindowContent:
        case ESACPClientCmdSubType_SinkCircularShiftWindowContent:
        case ESACPClientCmdSubType_SinkZoomV2:
        case ESACPClientCmdSubType_SinkSwitchGroup:
            LOG_CONFIGURATION("nvr cmd, relay, %d\n", cmd_type);
            return 1;
            break;

        case ESACPClientCmdSubType_PeerClose:
            LOG_CONFIGURATION("peer close, handle it\n");
            return 0;
            break;

        case ESACPClientCmdSubType_QueryVersion:
        case ESACPClientCmdSubType_QueryStatus:
        case ESACPClientCmdSubType_SyncStatus:
            LOG_CONFIGURATION("query nvr cmd, relay, %d\n", cmd_type);
            return 1;
            break;

        case ESACPClientCmdSubType_QuerySourceList:
        case ESACPClientCmdSubType_AquireChannelControl:
        case ESACPClientCmdSubType_ReleaseChannelControl:
            LOG_CONFIGURATION("query cloud cmd, handle it, %d\n", cmd_type);
            return 0;
            break;

            //for extention
        case ESACPClientCmdSubType_CustomizedCommand:
            LOG_CONFIGURATION("customized cmd\n");
            return 0;
            break;

            //debug device to nvr server
        case ESACPDebugCmdSubType_PrintCloudPipeline:
        case ESACPDebugCmdSubType_PrintStreamingPipeline:
        case ESACPDebugCmdSubType_PrintRecordingPipeline:
        case ESACPDebugCmdSubType_PrintSingleChannel:
        case ESACPDebugCmdSubType_PrintAllChannels:
            LOG_CONFIGURATION("debug related cmd, %d\n", cmd_type);
            return 0;
            break;

        case ESACPClientCmdSubType_ResponceOnlyForRelay:
            LOG_CONFIGURATION("responce for relay\n");
            return 1;
            break;

        default:
            break;

    }

    LOG_FATAL("unexpected %d\n", cmd_type);
    return 0;
}

IFilter *gfCreateCloudConnecterCmdAgentFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CCloudConnecterCmdAgent::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

//-----------------------------------------------------------------------
//
// CCloudConnecterCmdAgent
//
//-----------------------------------------------------------------------

IFilter *CCloudConnecterCmdAgent::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    CCloudConnecterCmdAgent *result = new CCloudConnecterCmdAgent(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

CObject *CCloudConnecterCmdAgent::GetObject0() const
{
    return (CObject *) this;
}

CCloudConnecterCmdAgent::CCloudConnecterCmdAgent(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
    : inherited(pName, index)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpEngineMsgSink(pEngineMsgSink)
    , mpScheduler(NULL)
    , mpAgent(NULL)
    , mProtocolType(EProtocolType_SACP)
    , mbRunning(0)
    , mbStarted(0)
    , mbSuspended(0)
    , mbAddedToScheduler(0)
    , mpSourceUrl(NULL)
    , mSourceUrlLength(0)
    , mbRelayCmd(1)
    , mRelaySeqNumber0(0)
    , mRelaySeqNumber1(0)
    , mpRelayTarget(NULL)
{

}

EECode CCloudConnecterCmdAgent::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleCloudConnecterServerAgent);

    mpScheduler = gfGetNetworkReceiverTCPScheduler(mpPersistMediaConfig, mIndex);

    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOGM_FATAL("gfCreateMutex() fail\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CCloudConnecterCmdAgent::~CCloudConnecterCmdAgent()
{
    LOGM_RESOURCE("~CCloudConnecterCmdAgent.\n");
    if (mpAgent) {
        mpAgent->Destroy();
        mpAgent = NULL;
    }

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    LOGM_RESOURCE("~CCloudConnecterCmdAgent done.\n");
}

void CCloudConnecterCmdAgent::Delete()
{
    LOGM_RESOURCE("CCloudConnecterCmdAgent::Delete(), before mpAgent->Delete().\n");
    if (mpAgent) {
        mpAgent->Destroy();
        mpAgent = NULL;
    }

    LOGM_RESOURCE("CCloudConnecterCmdAgent::Delete(), before inherited::Delete().\n");
    inherited::Delete();
}

EECode CCloudConnecterCmdAgent::Initialize(TChar *p_param)
{
    AUTO_LOCK(mpMutex);

    if (!p_param) {
        LOGM_INFO("CCloudConnecterCmdAgent::Initialize(NULL), not setup cloud connecter server agent at this time\n");
        return EECode_OK;
    }

    return EECode_OK;
}

EECode CCloudConnecterCmdAgent::Finalize()
{
    AUTO_LOCK(mpMutex);

    if (mpAgent && mpScheduler && mbAddedToScheduler) {
        LOGM_INFO("CCloudConnecterCmdAgent::Stop(), before RemoveScheduledCilent\n");
        mpScheduler->RemoveScheduledCilent((IScheduledClient *) mpAgent);
        mbAddedToScheduler = 0;
        LOGM_INFO("CCloudConnecterCmdAgent::Stop(), after RemoveScheduledCilent\n");
    }

    return EECode_OK;
}

EECode CCloudConnecterCmdAgent::Run()
{
    DASSERT(!mbRunning);
    DASSERT(!mbStarted);

    mbRunning = 1;
    return EECode_OK;
}

EECode CCloudConnecterCmdAgent::Exit()
{
    return EECode_OK;
}

EECode CCloudConnecterCmdAgent::Start()
{
    AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(!mbStarted);

    if (mpAgent && mpScheduler && (!mbAddedToScheduler)) {
        LOGM_INFO("CCloudConnecterCmdAgent::Start(), before AddScheduledCilent\n");
        mpScheduler->AddScheduledCilent((IScheduledClient *) mpAgent);
        mbAddedToScheduler = 1;
        LOGM_INFO("CCloudConnecterCmdAgent::Start(), after AddScheduledCilent\n");
    }
    mbStarted = 1;

    return EECode_OK;
}

EECode CCloudConnecterCmdAgent::Stop()
{
    AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    //DASSERT(mbStarted);

    if (mpAgent && mpScheduler && mbAddedToScheduler) {
        LOGM_INFO("CCloudConnecterCmdAgent::Stop(), before RemoveScheduledCilent\n");
        mpScheduler->RemoveScheduledCilent((IScheduledClient *) mpAgent);
        mbAddedToScheduler = 0;
        LOGM_INFO("CCloudConnecterCmdAgent::Stop(), after RemoveScheduledCilent\n");
    }
    mbStarted = 0;

    return EECode_OK;
}

void CCloudConnecterCmdAgent::Pause()
{
    DASSERT(mbRunning);
    DASSERT(mbStarted);

    LOGM_WARN("TO DO\n");

    return;
}

void CCloudConnecterCmdAgent::Resume()
{
    AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(mbStarted);

    LOGM_WARN("TO DO\n");

    return;
}

void CCloudConnecterCmdAgent::Flush()
{
    AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(mbStarted);

    LOGM_WARN("TO DO\n");

    return;
}

void CCloudConnecterCmdAgent::ResumeFlush()
{
    AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(mbStarted);

    LOGM_WARN("TO DO\n");

    return;
}

EECode CCloudConnecterCmdAgent::Suspend(TUint release_context)
{
    AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(mbStarted);

    LOGM_WARN("TO DO\n");

    return EECode_OK;
}

EECode CCloudConnecterCmdAgent::ResumeSuspend(TComponentIndex input_index)
{
    AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(mbStarted);

    LOGM_WARN("TO DO\n");

    return EECode_OK;
}

EECode CCloudConnecterCmdAgent::SwitchInput(TComponentIndex focus_input_index)
{
    LOGM_WARN("TO DO\n");

    return EECode_OK;
}

EECode CCloudConnecterCmdAgent::FlowControl(EBufferType flowcontrol_type)
{
    AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(mbStarted);

    LOGM_WARN("TO DO\n");

    return EECode_NoImplement;
}

EECode CCloudConnecterCmdAgent::SendCmd(TUint cmd)
{
    AUTO_LOCK(mpMutex);

    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

void CCloudConnecterCmdAgent::PostMsg(TUint cmd)
{
    AUTO_LOCK(mpMutex);

    LOGM_FATAL("TO DO\n");
    return;
}

EECode CCloudConnecterCmdAgent::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    AUTO_LOCK(mpMutex);

    LOGM_FATAL("TO DO\n");
    return EECode_BadState;
}

EECode CCloudConnecterCmdAgent::AddInputPin(TUint &index, TUint type)
{
    AUTO_LOCK(mpMutex);

    LOGM_FATAL("TO DO\n");
    return EECode_BadState;
}

IOutputPin *CCloudConnecterCmdAgent::GetOutputPin(TUint index, TUint sub_index)
{
    AUTO_LOCK(mpMutex);

    LOGM_FATAL("TO DO\n");

    return NULL;
}

IInputPin *CCloudConnecterCmdAgent::GetInputPin(TUint index)
{
    AUTO_LOCK(mpMutex);

    LOGM_FATAL("TO DO\n");
    return NULL;
}

EECode CCloudConnecterCmdAgent::FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
    AUTO_LOCK(mpMutex);

    EECode ret = EECode_OK;

    switch (property_type) {

        case EFilterPropertyType_update_source_url:
            break;

        case EFilterPropertyType_enable_relay_command:
            mbRelayCmd = set_or_get;
            break;

        case EFilterPropertyType_set_relay_target:
            mpRelayTarget = (IFilter *) p_param;
            break;

        case EFilterPropertyType_assign_cloud_agent: {
                SCloudAgentSetting *agent_setting = (SCloudAgentSetting *)p_param;
                if (DLikely(agent_setting)) {
                    DASSERT(agent_setting->server_agent);
                    if (DLikely(mpScheduler)) {
                        if (DUnlikely(mpAgent && mbAddedToScheduler)) {
                            mpScheduler->RemoveScheduledCilent((IScheduledClient *)mpAgent);
                            mbAddedToScheduler = 0;
                            mpAgent = NULL;
                        }
                        mpAgent = (ICloudServerAgent *)agent_setting->server_agent;
                        mpAgent->SetProcessCMDCallBack((void *)this, CmdCallback);
                        mpAgent->SetProcessDataCallBack((void *)this, DataCallback);
                        mpScheduler->AddScheduledCilent((IScheduledClient *)mpAgent);
                        mbAddedToScheduler = 1;

                    } else {
                        LOGM_FATAL("NULL mpScheduler\n");
                    }
                } else {
                    LOGM_FATAL("NULL p_param\n");
                    ret = EECode_BadParam;
                }
            }
            break;

        case EFilterPropertyType_remove_cloud_agent: {
                if (mpAgent && mpScheduler && mbAddedToScheduler) {
                    mpScheduler->RemoveScheduledCilent((IScheduledClient *)mpAgent);
                    mbAddedToScheduler = 0;
                    mpAgent = NULL;
                }
            }
            break;

        case EFilterPropertyType_relay_command: {
                if (mpAgent && p_param) {
                    SRelayData *data = (SRelayData *)p_param;
                    mpAgent->WriteData(data->p_data, data->data_size);
                } else {
                    LOGM_ERROR("NULL mpAgent %p or p_param %p\n", mpAgent, p_param);
                }
            }
            break;

        default:
            LOGM_FATAL("BAD property 0x%08x\n", property_type);
            ret = EECode_InternalLogicalBug;
            break;
    }

    return ret;
}

void CCloudConnecterCmdAgent::GetInfo(INFO &info)
{
    AUTO_LOCK(mpMutex);

    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = 0;
    info.nOutput = 0;

    info.pName = mpModuleName;
}

void CCloudConnecterCmdAgent::PrintStatus()
{
    LOGM_PRINTF("[CCloudConnecterCmdAgent::PrintStatus()]\n");
}

EECode CCloudConnecterCmdAgent::CmdCallback(void *owner, TU32 cmd_type, TU32 params0, TU32 params1, TU32 params2, TU32 params3, TU32 params4, TU8 *pcmd, TU32 cmdsize)
{
    CCloudConnecterCmdAgent *thiz = (CCloudConnecterCmdAgent *)owner;

    if (DLikely(thiz)) {
        return thiz->ProcessCmdCallback(cmd_type, params0, params1, params2, params3, params4, pcmd, cmdsize);
    }

    LOG_FATAL("NULL onwer\n");
    return EECode_BadState;
}

EECode CCloudConnecterCmdAgent::DataCallback(void *owner, TMemSize read_length, TU16 data_type, TU8 extra_flag)
{
    CCloudConnecterCmdAgent *thiz = (CCloudConnecterCmdAgent *)owner;

    if (DLikely(thiz)) {
        return thiz->ProcessDataCallback(read_length, data_type, extra_flag);
    }

    LOG_FATAL("NULL onwer\n");
    return EECode_BadState;
}

EECode CCloudConnecterCmdAgent::ProcessCmdCallback(TU32 cmd_type, TU32 params0, TU32 params1, TU32 params2, TU32 params3, TU32 params4, TU8 *pcmd, TU32 cmdsize)
{
    AUTO_LOCK(mpMutex);

    ECMDType type = gfSACPClientSubCmdTypeToGenericCmdType((TU16)cmd_type);
    TUint need_relay = needRelayCmmand((ESACPClientCmdSubType)cmd_type);

    if (DUnlikely(mbRelayCmd && need_relay)) {
        LOGM_CONFIGURATION("relay cmd %d, type %d\n", cmd_type, type);

        if (DLikely(mpRelayTarget)) {
            SRelayData relaydata;
            relaydata.p_data = pcmd;
            relaydata.data_size = cmdsize;
            return mpRelayTarget->FilterProperty(EFilterPropertyType_relay_command, 1, (void *)&relaydata);
        } else {
            LOGM_FATAL("no relay target\n");
        }
    }

    if (DLikely(mpEngineMsgSink)) {

        SMSG msg;
        mDebugHeartBeat ++;

        memset(&msg, 0x0, sizeof(SMSG));
        msg.code = type;
        msg.owner_index = mIndex;
        msg.owner_type = EGenericComponentType_CloudConnecterCmdAgent;

        msg.p_agent_pointer = (TULong)mpAgent;

        msg.p0 = params0;
        msg.p1 = params1;
        msg.p2 = params2;
        msg.p3 = params3;
        msg.p4 = params4;

        return mpEngineMsgSink->MsgProc(msg);
    }

    LOG_FATAL("NULL msg sink\n");
    return EECode_BadState;
}

EECode CCloudConnecterCmdAgent::ProcessDataCallback(TMemSize read_length, TU16 data_type, TU8 extra_flag)
{
    LOGM_ERROR("should not comes here\n");
    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif

