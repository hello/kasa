/*******************************************************************************
 * active_generic_engine.cpp
 *
 * History:
 *    2012/12/21 - [Zhi He] create file
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

#include "cloud_lib_if.h"
#include "sacp_types.h"

#include "media_mw_if.h"

#include "media_mw_utils.h"
#include "mw_internal.h"

#include "framework_interface.h"
#include "filters_interface.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"


#ifdef BUILD_CONFIG_WITH_LICENCE
#include "licence_lib_if.h"
#endif

#include "active_generic_engine.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

#ifdef BUILD_CONFIG_WITH_LICENCE
#include "licence.check"
#endif

static EFilterType __filterTypeFromGenericComponentType(TU8 generic_component_type, TU8 scheduled_video_decoder, TU8 scheduled_muxer)
{
    EFilterType type = EFilterType_Invalid;

    switch (generic_component_type) {
        case EGenericComponentType_Demuxer:
            type = EFilterType_Demuxer;
            break;

        case EGenericComponentType_VideoDecoder:
            if (!scheduled_video_decoder) {
                type = EFilterType_VideoDecoder;
            } else {
                type = EFilterType_ScheduledVideoDecoder;
            }
            break;

        case EGenericComponentType_AudioDecoder:
            type = EFilterType_AudioDecoder;
            break;

        case EGenericComponentType_VideoEncoder:
            type = EFilterType_VideoEncoder;
            break;

        case EGenericComponentType_AudioEncoder:
            type = EFilterType_AudioEncoder;
            break;

        case EGenericComponentType_VideoPostProcessor:
            type = EFilterType_VideoPostProcessor;
            break;

        case EGenericComponentType_AudioPostProcessor:
            type = EFilterType_AudioPostProcessor;
            break;

        case EGenericComponentType_VideoPreProcessor:
            type = EFilterType_VideoPreProcessor;
            break;

        case EGenericComponentType_AudioPreProcessor:
            type = EFilterType_AudioPreProcessor;
            break;

        case EGenericComponentType_VideoCapture:
            type = EFilterType_VideoCapture;
            break;

        case EGenericComponentType_AudioCapture:
            type = EFilterType_AudioCapture;
            break;

        case EGenericComponentType_VideoRenderer:
            type = EFilterType_VideoRenderer;
            break;

        case EGenericComponentType_AudioRenderer:
            type = EFilterType_AudioRenderer;
            break;

        case EGenericComponentType_Muxer:
            if (!scheduled_muxer) {
                type = EFilterType_Muxer;
            } else {
                type = EFilterType_ScheduledMuxer;
            }
            break;

        case EGenericComponentType_StreamingTransmiter:
            type = EFilterType_RTSPTransmiter;
            break;

        case EGenericComponentType_ConnecterPinMuxer:
            type = EFilterType_ConnecterPinmuxer;
            break;

        case EGenericComponentType_CloudConnecterServerAgent:
            type = EFilterType_CloudConnecterServerAgent;
            break;

        case EGenericComponentType_CloudConnecterClientAgent:
            type = EFilterType_CloudConnecterClientAgent;
            break;

        case EGenericComponentType_StreamingServer:
            type = EFilterType_RTSPTransmiter;
            break;

        case EGenericComponentType_StreamingContent:
            type = EFilterType_RTSPTransmiter;
            break;

        default:
            LOG_FATAL("(%d)not a filter type.\n", generic_component_type);
            break;
    }

    return type;
}

IGenericEngineControl *CreateGenericEngine()
{
    return (IGenericEngineControl *)CActiveGenericEngine::Create("CActiveGenericEngine", 0);
}

//-----------------------------------------------------------------------
//
// CActiveGenericEngine
//
//-----------------------------------------------------------------------
CActiveGenericEngine *CActiveGenericEngine::Create(const TChar *pname, TUint index)
{
    CActiveGenericEngine *result = new CActiveGenericEngine(pname, index);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CActiveGenericEngine::CActiveGenericEngine(const TChar *pname, TUint index)
    : inherited(pname, index)
    , mpDSPAPI(NULL)
    , mIavFd(-1)
    , mnDemuxerNum(0)
    , mnMuxerNum(0)
    , mnVideoDecoderNum(0)
    , mnVideoEncoderNum(0)
    , mnAudioDecoderNum(0)
    , mnAudioEncoderNum(0)
    , mnStreamingTransmiterNum(0)
    , mnRTSPStreamingServerNum(0)
    , mnHLSStreamingServerNum(0)
    , mbAutoNamingRTSPContextTag(0)
    , mbEnginePipelineRunning(0)
    , mpClockManager(NULL)
    , mpSystemClockReference(NULL)
    , mpClockSource(NULL)
    , mbRun(1)
    , mbGraghBuilt(0)
    , mbPipelineInfoGet(0)
    , mbSchedulersCreated(0)
    , mpMutex(NULL)
    , msState(EEngineState_not_alive)
    , mpUniqueVideoPostProcessor(NULL)
    , mpUniqueAudioPostProcessor(NULL)
    , mpUniqueVideoRenderer(NULL)
    , mpUniqueAudioRenderer(NULL)
    , mpUniqueAudioDecoder(NULL)
    , mpUniqueRTSPStreamingTransmiter(NULL)
    , mpUniqueVideoCapture(NULL)
    , mpUniqueAudioCapture(NULL)
    , mpUniqueAudioPinMuxer(NULL)
    , mpUniqueVideoPostProcessorComponent(NULL)
    , mpUniqueAudioPostProcessorComponent(NULL)
    , mpUniqueVideoRendererComponent(NULL)
    , mpUniqueAudioRendererComponent(NULL)
    , mpUniqueAudioDecoderComponent(NULL)
    , mpUniqueRTSPStreamingTransmiterComponent(NULL)
    , mpUniqueVideoCaptureComponent(NULL)
    , mpUniqueAudioCaptureComponent(NULL)
    , mpUniqueAudioPinMuxerComponent(NULL)
    , mRequestWorkingMode(EMediaDeviceWorkingMode_Invalid)
    , mRequestDSPMode(EDSPOperationMode_Invalid)
    , mPresetPlaybackMode(EPlaybackMode_Invalid)
    , mRequestVoutMask(0x0)
    , mVoutStartIndex(0)
    , mVoutNumber(2)

    , mMaxVoutWidth(1280)
    , mMaxVoutHeight(720)
    , mbAllVideoDisabled(0)
    , mbAllAudioDisabled(0)
    , mbAllPridataDisabled(0)
    , mbAllStreamingDisabled(0)

    , mTotalMuxerNumber(0)
    , mTotalVideoDecoderNumber(0)
    , mMultipleWindowDecoderNumber(0)
    , mSingleWindowDecoderNumber(0)

    , mInitialVideoDisplayMask(DCAL_BITMASK(EDisplayDevice_HDMI))
    , mInitialVideoDisplayPrimaryIndex(EDisplayDevice_HDMI)
    , mInitialVideoDisplayCurrentMask(DCAL_BITMASK(EDisplayDevice_HDMI))
    , mInitialVideoDisplayLayerNumber(2)
    , mpStreamingServerManager(NULL)
    , mnCurrentStreamingContentNumber(0)
    , mnMaxStreamingContentNumber(0)
    , mpStreamingContent(NULL)
    , mpCloudServerManager(NULL)
    , mnCurrentCloudReceiverContentNumber(0)
    , mnMaxCloudReceiverContentNumber(0)
    , mpCloudReceiverContent(NULL)

    , mnPlaybackPipelinesNumber(0)
    , mpPlaybackPipelines(NULL)
    , mCurSourceGroupIndex(0)
    , mCurHDWindowIndex(0xff)

    , mbSuspendAudioPlaybackPipelineInFwBw(1)
    , mbSuspendBackgoundVideoPlaybackPipeline(1)
    , mbChangePBSpeedWithFlush(1)
    , mbChangePBSpeedWithSeek(0)

    , mbDisplayAllInstance(0)
    , mbMultipleWindowView(0)
    , mbSingleWindowView(0)
    , mSingleViewDecIndex(0)

    , mnUserTimerNumber(0)
    , mnUserTimerMaxNumber(128)

    , mpCommunicationBuffer(NULL)
    , mCommunicationBufferSize(0)

    , mDailyCheckTime(0)
#ifdef BUILD_CONFIG_WITH_LICENCE
    , mpLicenceClient(NULL)
#endif
    , mpDailyCheckTimer(NULL)
{
    TUint i;
    EECode err;

    memset((void *)&mPersistMediaConfig, 0, sizeof(mPersistMediaConfig));
    memset((void *)&mRecordingPipelines, 0, sizeof(SRecordingPipelines));
    memset((void *)&mRTSPStreamingPipelines, 0, sizeof(SStreamingPipelines));

    for (i = 0; i < EComponentMaxNumber_Muxer; i ++) {
        memset(&mMuxerConfig[i], 0, sizeof(SMuxerConfig));
        //some default value
        mMuxerConfig[i].mSavingStrategy = MuxerSavingFileStrategy_ToTalFile;
        mMuxerConfig[i].mSavingCondition = MuxerSavingCondition_InputPTS;
        mMuxerConfig[i].mConditionParam = DefaultAutoSavingParam;
        mMuxerConfig[i].mAutoNaming = MuxerAutoFileNaming_ByNumber;

        mMuxerConfig[i].mMaxTotalFileNumber = DefaultTotalFileNumber;
    }

    for (i = 0; i < DMaxDisplayLayerNumber; i ++) {
        mInitialVideoDisplayLayoutType[i] = EDisplayLayout_Invalid;
        mInitialVideoDisplayLayerWindowNumber[i] = 0;
    }

    //initialize
    for (i = EGenericComponentType_TAG_filter_start; i < EGenericComponentType_TAG_filter_end; i ++) {
        mnComponentFiltersNumbers[i] = 0;
    }

    for (i = 0; i < (EGenericComponentType_TAG_proxy_end - EGenericComponentType_TAG_proxy_start); i ++) {
        mnComponentProxysNumbers[i] = 0;
    }
    mnComponentFiltersMaxNumbers[EGenericComponentType_Demuxer] = EComponentMaxNumber_Demuxer;
    mnComponentFiltersMaxNumbers[EGenericComponentType_VideoDecoder] = EComponentMaxNumber_VideoDecoder;
    mnComponentFiltersMaxNumbers[EGenericComponentType_AudioDecoder] = EComponentMaxNumber_AudioDecoder;
    mnComponentFiltersMaxNumbers[EGenericComponentType_VideoEncoder] = EComponentMaxNumber_VideoEncoder;
    mnComponentFiltersMaxNumbers[EGenericComponentType_AudioEncoder] = EComponentMaxNumber_AudioEncoder;
    mnComponentFiltersMaxNumbers[EGenericComponentType_VideoPostProcessor] = EComponentMaxNumber_VideoPostProcessor;
    mnComponentFiltersMaxNumbers[EGenericComponentType_AudioPostProcessor] = EComponentMaxNumber_AudioPostProcessor;
    mnComponentFiltersMaxNumbers[EGenericComponentType_VideoCapture] = EComponentMaxNumber_VideoCapture;
    mnComponentFiltersMaxNumbers[EGenericComponentType_AudioCapture] = EComponentMaxNumber_AudioCapture;
    mnComponentFiltersMaxNumbers[EGenericComponentType_VideoRenderer] = EComponentMaxNumber_VideoRenderer;
    mnComponentFiltersMaxNumbers[EGenericComponentType_AudioRenderer] = EComponentMaxNumber_AudioRenderer;
    mnComponentFiltersMaxNumbers[EGenericComponentType_Muxer] = EComponentMaxNumber_Muxer;
    mnComponentFiltersMaxNumbers[EGenericComponentType_StreamingTransmiter] = EComponentMaxNumber_RTSPStreamingTransmiter;
    mnComponentFiltersMaxNumbers[EGenericComponentType_ConnecterPinMuxer] = EComponentMaxNumber_AudioPinMuxer;
    mnComponentFiltersMaxNumbers[EGenericComponentType_CloudConnecterServerAgent] = EComponentMaxNumber_CloudConnecterServerAgent;
    mnComponentFiltersMaxNumbers[EGenericComponentType_CloudConnecterClientAgent] = EComponentMaxNumber_CloudConnecterClientAgent;

    mnComponentProxysMaxNumbers[EGenericComponentType_StreamingServer - EGenericComponentType_TAG_proxy_start] = EComponentMaxNumber_RTSPStreamingServer;
    mnComponentProxysMaxNumbers[EGenericComponentType_StreamingContent - EGenericComponentType_TAG_proxy_start] = EComponentMaxNumber_RTSPStreamingContent;
    mnComponentProxysMaxNumbers[EGenericComponentType_ConnectionPin - EGenericComponentType_TAG_proxy_start] = EComponentMaxNumber_ConnectionPin;
    mnComponentProxysMaxNumbers[EGenericComponentType_UserTimer - EGenericComponentType_TAG_proxy_start] = EComponentMaxNumber_UserTimer;
    mnComponentProxysMaxNumbers[EGenericComponentType_CloudServer - EGenericComponentType_TAG_proxy_start] = EComponentMaxNumber_CloudServerNVR;
    mnComponentProxysMaxNumbers[EGenericComponentType_CloudReceiverContent - EGenericComponentType_TAG_proxy_start] = EComponentMaxNumber_CloudReceiverContent;

#if 0
    mpUniqueVideoCapture = NULL;
    mpUniqueAudioCapture = NULL;
    mpUniqueVideoPostProcessorComponent = NULL;
    mpUniqueAudioPostProcessorComponent = NULL;
    mpUniqueVideoRendererComponent = NULL;
    mpUniqueAudioRendererComponent = NULL;
    mpUniqueRTSPStreamingTransmiterComponent = NULL;
    mpUniqueVideoCaptureComponent = NULL;
    mpUniqueAudioCaptureComponent = NULL;
#endif

    gfOpenLogFile((TChar *)"mw_cg.log");

    gfLoadDefaultMediaConfig(&mPersistMediaConfig);

    mPersistMediaConfig.engine_config.engine_version = 1;
    mPersistMediaConfig.engine_config.filter_version = 1;

    LOGM_PRINTF("try load log.config\n");
    err = gfLoadLogConfigFile(ERunTimeConfigType_XML, "log.config", &gmModuleLogConfig[0], ELogModuleListEnd);
    if (DUnlikely(EECode_NotSupported == err)) {
        LOGM_WARN("no xml reader\n");
    } else if (DUnlikely(EECode_OK != err)) {
        LOGM_WARN("load log.config fail ret %d, %s, write a correct default one\n", err, gfGetErrorCodeString(err));
        gfLoadDefaultLogConfig(&gmModuleLogConfig[0], ELogModuleListEnd);
        err = gfSaveLogConfigFile(ERunTimeConfigType_XML, "log.config", &gmModuleLogConfig[0], ELogModuleListEnd);
        DASSERT_OK(err);
    } else {
        LOGM_PRINTF("load log.config done\n");
        //gfPrintLogConfig(&gmModuleLogConfig[0], ELogModuleListEnd);
    }

    if (EECode_OK != err) {
        TUint loglevel = ELogLevel_Notice, logoption = 0, logoutput = 0;
        err = gfLoadSimpleLogConfigFile((TChar *)"simple_log.config", loglevel, logoption, logoutput);
        if (EECode_OK == err) {
            gfSetLogConfigLevel((ELogLevel)loglevel, &gmModuleLogConfig[0], ELogModuleListEnd);
        }
    }

    new(&mUserTimerList) CIDoubleLinkedList();
    new(&mListProxys) CIDoubleLinkedList();
    new(&mUserTimerFreeList) CIDoubleLinkedList();

    memset(&mCurrentSyncStatus, 0x0, sizeof(mCurrentSyncStatus));
    memset(&mInputSyncStatus, 0x0, sizeof(mInputSyncStatus));

    //print version
    TU32 major, minor, patch;
    TU32 year;
    TU32 month, day;

    LOG_PRINTF("current version:\n");
    gfGetCommonVersion(major, minor, patch, year, month, day);
    LOG_PRINTF("\tlibmwcg_common version:\t\t%d.%d.%d\t\tdate:\t%04d-%02d-%02d\n", major, minor, patch, year, month, day);
    gfGetCloudLibVersion(major, minor, patch, year, month, day);
    LOG_PRINTF("\tlibmwcg_cloud_lib version:\t%d.%d.%d\t\tdate:\t%04d-%02d-%02d\n", major, minor, patch, year, month, day);
    gfGetMediaMWVersion(major, minor, patch, year, month, day);
    LOG_PRINTF("\tlibmwcg_media_mw version:\t%d.%d.%d\t\tdate:\t%04d-%02d-%02d\n", major, minor, patch, year, month, day);

}

EECode CActiveGenericEngine::Construct()
{
    EECode ret = inherited::Construct();
    if (ret != EECode_OK) {
        LOGM_FATAL("CActiveGenericEngine::Construct, inherited::Construct fail, ret %d\n", ret);
        return ret;
    }

    DSET_MODULE_LOG_CONFIG(ELogModuleGenericEngine);

#ifdef BUILD_CONFIG_WITH_LICENCE
    mpLicenceClient = gfCreateLicenceClient(ELicenceType_StandAlone);
    if (DUnlikely(!mpLicenceClient)) {
        LOG_FATAL("gfCreateLicenceClient fail\n");
        return EECode_NoMemory;
    }

    ret = mpLicenceClient->Authenticate((TChar *)"./licence.txt", gLicenceCheckString, strlen(gLicenceCheckString));
    if (EECode_OK != ret) {
        LOG_ERROR("Licence Authenticate fail\n");
        return EECode_Error;
    }
#endif

    //    mConfigLogLevel = ELogLevel_Info;
    //    mConfigLogOption = DCAL_BITMASK(ELogOption_State) | DCAL_BITMASK(ELogOption_Resource);

    mCommunicationBufferSize = 4 * 1024;

    mpCommunicationBuffer = (TU8 *) DDBG_MALLOC(mCommunicationBufferSize, "E0CM");

    if (DUnlikely(!mpCommunicationBuffer)) {
        LOG_FATAL("no memory, request %ld\n", mCommunicationBufferSize);
        mCommunicationBufferSize = 0;
        return EECode_NoMemory;
    }

    if ((mpMutex = gfCreateMutex()) == NULL) {
        LOGM_FATAL("gfCreateMutex() fail\n");
        return EECode_NoMemory;
    }

    if ((mpClockManager = CIClockManager::Create()) == NULL) {
        LOGM_FATAL("CIClockManager::Create() fail\n");
        return EECode_NoMemory;
    }

    if ((mpSystemClockReference = CIClockReference::Create()) == NULL) {
        LOGM_FATAL("CIClockReference::Create() fail\n");
        return EECode_NoMemory;
    }

    mpClockSource = gfCreateClockSource(EClockSourceType_generic);
    if (!mpClockSource) {
        LOGM_FATAL("gfCreateClockSource() fail\n");
        return EECode_NoMemory;
    }

    ret = mpClockManager->SetClockSource(mpClockSource);
    DASSERT_OK(ret);

    ret = mpClockManager->Start();
    DASSERT_OK(ret);

    ret = mpClockManager->RegisterClockReference(mpSystemClockReference);
    DASSERT_OK(ret);

    mPersistMediaConfig.p_system_clock_reference = mpSystemClockReference;

    DASSERT(mpWorkQ);
    mpWorkQ->Run();

    mpStreamingContent = (SStreamingSessionContent *)DDBG_MALLOC(mnComponentProxysMaxNumbers[EGenericComponentType_StreamingContent - EGenericComponentType_TAG_proxy_start] * sizeof(SStreamingSessionContent), "E0SC");
    if (DLikely(mpStreamingContent)) {
        memset(mpStreamingContent, 0x0, mnComponentProxysMaxNumbers[EGenericComponentType_StreamingContent - EGenericComponentType_TAG_proxy_start] * sizeof(SStreamingSessionContent));
    } else {
        LOGM_FATAL("No Memory\n");
        return EECode_NoMemory;
    }

    mpCloudReceiverContent = (SCloudContent *)DDBG_MALLOC(mnComponentProxysMaxNumbers[EGenericComponentType_CloudReceiverContent - EGenericComponentType_TAG_proxy_start] * sizeof(SCloudContent), "E0CR");
    if (DLikely(mpCloudReceiverContent)) {
        memset(mpCloudReceiverContent, 0x0, mnComponentProxysMaxNumbers[EGenericComponentType_CloudReceiverContent - EGenericComponentType_TAG_proxy_start] * sizeof(SCloudContent));
    } else {
        LOGM_FATAL("No Memory\n");
        return EECode_NoMemory;
    }

    mDailyCheckTime = mpSystemClockReference->GetCurrentTime();
    mpDailyCheckTimer = mpSystemClockReference->AddTimer(this, EClockTimerType_once, mDailyCheckTime, (TTime)24 * 60 * 60 * 1000000, 1);
    mpDailyCheckTimer->user_context = (TULong)&mDailyCheckContext;

    return EECode_OK;
}

CActiveGenericEngine::~CActiveGenericEngine()
{
    CIDoubleLinkedList::SNode *pnode = NULL;

    LOGM_RESOURCE("CActiveGenericEngine::~CActiveGenericEngine before delete mpClockManager %p.\n", mpClockManager);

    if (mpClockManager) {
        mpClockManager->Stop();
        mpClockManager->Delete();
        mpClockManager = NULL;
    }

    if (mpSystemClockReference) {
        mpSystemClockReference->Delete();
        mpSystemClockReference = NULL;
    }

    if (mpClockSource) {
        mpClockSource->Delete();
        mpClockSource = NULL;
    }

    LOGM_RESOURCE("CActiveGenericEngine::~CActiveGenericEngine, before delete mpPrivateDataConfigList.\n");

    //private durations
    SPrivateDataConfig *p_config = mpPrivateDataConfigList;
    SPrivateDataConfig *p_tmp;
    while (p_config) {
        p_tmp = p_config;
        p_config = p_config->p_next;
        DDBG_FREE(p_tmp, "E0PDs");
    }
    mpPrivateDataConfigList = NULL;

    if (mpStreamingServerManager) {
        mpStreamingServerManager->ExitServerManager();
        mpStreamingServerManager->Destroy();
        mpStreamingServerManager = NULL;
    }

    if (mpCloudServerManager) {
        mpCloudServerManager->ExitServerManager();
        mpCloudServerManager->Destroy();
        mpCloudServerManager = NULL;
    }

    LOGM_RESOURCE("CActiveGenericEngine::~CActiveGenericEngine before clearGraph().\n");
    clearGraph();
    LOGM_RESOURCE("CActiveGenericEngine::~CActiveGenericEngine before destroySchedulers().\n");
    destroySchedulers();

    if (mpStreamingContent) {
        DDBG_FREE(mpStreamingContent, "E0SC");
        mpStreamingContent = NULL;
    }

    if (mpCloudReceiverContent) {
        DDBG_FREE(mpCloudReceiverContent, "E0CR");
        mpCloudReceiverContent = NULL;
    }

    pnode = mUserTimerList.FirstNode();
    while (pnode) {
        SUserTimerContext *pcontext = (SUserTimerContext *)pnode->p_context;
        if (DLikely(pcontext)) {
            DDBG_FREE(pcontext, "E0UT");
        }
        pnode = mUserTimerList.NextNode(pnode);
    }

    pnode = mUserTimerFreeList.FirstNode();
    while (pnode) {
        SUserTimerContext *pcontext = (SUserTimerContext *)pnode->p_context;
        if (DLikely(pcontext)) {
            DDBG_FREE(pcontext, "E0UT");
        }
        pnode = mUserTimerFreeList.NextNode(pnode);
    }

    if (mpCommunicationBuffer) {
        DDBG_FREE(mpCommunicationBuffer, "E0CM");
        mpCommunicationBuffer = NULL;
    }
    mCommunicationBufferSize = 0;

#ifdef BUILD_CONFIG_WITH_LICENCE
    if (mpLicenceClient) {
        mpLicenceClient->Destroy();
        mpLicenceClient = NULL;
    }
#endif

    gfCloseLogFile();

    LOGM_RESOURCE("CActiveGenericEngine::~CActiveGenericEngine exit.\n");
}

void CActiveGenericEngine::Delete()
{
    LOGM_RESOURCE("CActiveGenericEngine::Delete, before mpWorkQ->SendCmd(CMD_STOP);.\n");

    if (mpWorkQ) {
        LOGM_INFO("before send stop.\n");
        mpWorkQ->SendCmd(ECMDType_ExitRunning);
        LOGM_INFO("after send stop.\n");
    }

    LOGM_RESOURCE("CActiveGenericEngine::Delete, before delete mpClockManager %p.\n", mpClockManager);

    if (mpClockManager) {
        mpClockManager->Stop();
        mpClockManager->Delete();
        mpClockManager = NULL;
    }

    if (mpSystemClockReference) {
        mpSystemClockReference->Delete();
        mpSystemClockReference = NULL;
    }

    if (mpClockSource) {
        mpClockSource->Delete();
        mpClockSource = NULL;
    }

    LOGM_RESOURCE("CActiveGenericEngine::Delete, before delete mpPrivateDataConfigList.\n");

    //private durations
    SPrivateDataConfig *p_config = mpPrivateDataConfigList;
    SPrivateDataConfig *p_tmp;
    while (p_config) {
        p_tmp = p_config;
        p_config = p_config->p_next;
        free(p_tmp);
    }
    mpPrivateDataConfigList = NULL;

    if (mpStreamingServerManager) {
        mpStreamingServerManager->ExitServerManager();
        mpStreamingServerManager->Destroy();
        mpStreamingServerManager = NULL;
    }

    if (mpCloudServerManager) {
        mpCloudServerManager->ExitServerManager();
        mpCloudServerManager->Destroy();
        mpCloudServerManager = NULL;
    }

    LOGM_RESOURCE("CActiveGenericEngine::Delete, before inherited::Delete().\n");

    if (mpStreamingContent) {
        DDBG_FREE(mpStreamingContent, "E0SC");
        mpStreamingContent = NULL;
    }

    if (mpCloudReceiverContent) {
        DDBG_FREE(mpCloudReceiverContent, "E0CR");
        mpCloudReceiverContent = NULL;
    }

    if (mpCommunicationBuffer) {
        DDBG_FREE(mpCommunicationBuffer, "E0CM");
        mpCommunicationBuffer = NULL;
    }
    mCommunicationBufferSize = 0;

    inherited::Delete();

    LOGM_RESOURCE("CActiveGenericEngine::Delete, exit.\n");
}

EECode CActiveGenericEngine::SetWorkingMode(EMediaWorkingMode preferedWorkingMode, EPlaybackMode playback_mode)
{
    LOGM_CONFIGURATION("preferedWorkingMode %d, %s, playback_mode %d, %s\n", preferedWorkingMode, gfGetMediaDeviceWorkingModeString(preferedWorkingMode), playback_mode, gfGetPlaybackModeString(playback_mode));
    mRequestWorkingMode = preferedWorkingMode;
    mPresetPlaybackMode = playback_mode;
    return EECode_OK;
}

EECode CActiveGenericEngine::SetDisplayDevice(TUint deviceMask)
{
    mRequestVoutMask = deviceMask;
    return EECode_OK;
}

EECode CActiveGenericEngine::SetInputDevice(TUint deviceMask)
{
    LOGM_FATAL("TO DO\n");
    return EECode_OK;
}

EECode CActiveGenericEngine::SetEngineLogLevel(ELogLevel level)
{
    gfSetLogConfigLevel(level, &gmModuleLogConfig[0], ELogModuleListEnd);

    //change itself
    mConfigLogLevel = level;

    if (mpClockManager) {
        mpClockManager->SetLogLevel(level);
    }

    if (mpSystemClockReference) {
        mpSystemClockReference->SetLogLevel(level);
    }

    //PrintLogConfig(&gmModuleLogConfig[0], ELogModuleListEnd);
    return EECode_OK;
}

EECode CActiveGenericEngine::PrintCurrentStatus(TGenericID id, TULong print_flag)
{
    EECode err = EECode_OK;
    SCMD cmd;

    cmd.code = ECMDType_PrintCurrentStatus;
    cmd.res32_1 = id;
    cmd.res64_1 = print_flag;
    err = mpWorkQ->PostMsg(cmd);

    return err;
}

EECode CActiveGenericEngine::SaveCurrentLogSetting(const TChar *logfile)
{
    EECode err;
    if (!logfile) {
        err = gfSaveLogConfigFile(ERunTimeConfigType_XML, "log.config", &gmModuleLogConfig[0], ELogModuleListEnd);
    } else {
        err = gfSaveLogConfigFile(ERunTimeConfigType_XML, logfile, &gmModuleLogConfig[0], ELogModuleListEnd);
    }
    DASSERT_OK(err);

    return err;
}

bool CActiveGenericEngine::allFiltersFlag(TUint flag, TGenericID id)
{
    CIDoubleLinkedList::SNode *pnode = mListFilters.FirstNode();
    SComponent_Obsolete *p_component = NULL;

    if (!IS_VALID_COMPONENT_ID(id)) {
        DASSERT(0 == id);
        while (pnode) {
            p_component = (SComponent_Obsolete *)(pnode->p_context);
            DASSERT(p_component);
            if (p_component) {
                if (!(flag & p_component->status_flag)) {
                    return false;
                }
            } else {
                DASSERT("NULL pointer here, something would be wrong.\n");
            }
            pnode = mListFilters.NextNode(pnode);
        }
        return true;
    } else {
        while (pnode) {
            p_component = (SComponent_Obsolete *)(pnode->p_context);
            DASSERT(p_component);
            if (p_component) {
                DASSERT(p_component->p_scur_pipeline);
                if ((id == p_component->p_scur_pipeline->pipeline_id) && (!(flag & p_component->status_flag))) {
                    return false;
                }
            } else {
                DASSERT("NULL pointer here, something would be wrong.\n");
            }
            pnode = mListFilters.NextNode(pnode);
        }
        return true;
    }
}

void CActiveGenericEngine::clearAllFiltersFlag(TUint flag, TGenericID id)
{
    CIDoubleLinkedList::SNode *pnode = mListFilters.FirstNode();
    SComponent_Obsolete *p_component = NULL;

    if (!IS_VALID_COMPONENT_ID(id)) {
        DASSERT(0 == id);
        while (pnode) {
            p_component = (SComponent_Obsolete *)(pnode->p_context);
            DASSERT(p_component);
            if (p_component) {
                p_component->status_flag &= (~ flag);
            } else {
                DASSERT("NULL pointer here, something would be wrong.\n");
            }
            pnode = mListFilters.NextNode(pnode);
        }
    } else {
        while (pnode) {
            p_component = (SComponent_Obsolete *)(pnode->p_context);
            DASSERT(p_component);
            if (p_component) {
                DASSERT(p_component->p_scur_pipeline);
                if (id == p_component->p_scur_pipeline->pipeline_id) {
                    p_component->status_flag &= (~ flag);
                }
            } else {
                DASSERT("NULL pointer here, something would be wrong.\n");
            }
            pnode = mListFilters.NextNode(pnode);
        }
    }
}

void CActiveGenericEngine::setFiltersEOS(IFilter *pfilter)
{
    CIDoubleLinkedList::SNode *pnode = mListFilters.FirstNode();
    SComponent_Obsolete *p_component = NULL;

    while (pnode) {
        p_component = (SComponent_Obsolete *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            if (pfilter == p_component->p_filter) {
                p_component->status_flag |= DCAL_BITMASK(DFLAG_STATUS_EOS);
                return;
            }
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mListFilters.NextNode(pnode);
    }

    LOGM_WARN("do not find filter %p\n", pfilter);
}

void CActiveGenericEngine::reconnectAllStreamingServer()
{
    CIDoubleLinkedList::SNode *pnode = mListFilters.FirstNode();
    SComponent_Obsolete *p_component = NULL;

    while (pnode) {
        p_component = (SComponent_Obsolete *)(pnode->p_context);
        DASSERT(p_component);
        if (EGenericComponentType_Demuxer == p_component->type) {
            if (p_component->p_filter) {
                p_component->p_filter->FilterProperty(EFilterPropertyType_demuxer_reconnect_server, 1, NULL);
            }
        }
        pnode = mListFilters.NextNode(pnode);
    }
}

bool CActiveGenericEngine::processGenericMsg(SMSG &msg)
{
    //    EECode err;
    SMSG msg0;
    SComponent_Obsolete *p_component = NULL;

    LOGM_FLOW("[MSG flow]: code %d, msg.session id %d, session %d, before AUTO_LOCK.\n", msg.code, msg.sessionID, mSessionID);

    AUTO_LOCK(mpMutex);

    if (!IsSessionMsg(msg)) {
        LOGM_ERROR("!!!!not session msg, code %d, msg.session id %d, session %d.\n", msg.code, msg.sessionID, mSessionID);
        return true;
    }

    //check components
    if (msg.p_owner) {
        p_component = findComponentFromFilter((IFilter *)msg.p_owner);
        DASSERT(p_component);
        if (!p_component) {
            LOGM_ERROR("Internal ERROR, NULL pointer, please check code, msg.code %d\n", msg.code);
            return true;
        }
    } else if (msg.owner_id) {
        p_component = queryFilter(msg.owner_id);
        DASSERT(p_component);
        if (!p_component) {
            LOGM_ERROR("Internal ERROR, NULL pointer, please check code, msg.code %d\n", msg.code);
            return true;
        }
    }

    switch (msg.code) {

        case EMSGType_Timeout:
            LOGM_INFO("[MSG flow]: Demuxer timeout msg comes, type %d, index %d, GenericID 0x%08x.\n", p_component->type, p_component->index, p_component->id);

            DASSERT(p_component->p_filter);
            if (p_component->p_filter) {
                if (EGenericComponentType_Demuxer == p_component->type) {
                    LOGM_INFO("[DEBUG]: EFilterPropertyType_demuxer_reconnect_server \n");
                    p_component->p_filter->FilterProperty(EFilterPropertyType_demuxer_reconnect_server, 1, NULL);
                } else {
                    LOGM_ERROR("need handle here\n");
                }
            }
            break;

        case EMSGType_RecordingEOS:
            LOGM_INFO("[MSG flow]: Recorder EOS msg comes, IFilter %p, filter type %d, filter index %d, GenericID 0x%08x.\n", (IFilter *)msg.p1, p_component->type, p_component->index, p_component->id);

            DASSERT(p_component->p_scur_pipeline);
            DASSERT(EGenericPipelineType_Recording == p_component->p_scur_pipeline->pipeline_type);

            setFiltersEOS((IFilter *)msg.p_owner);
            if (true == allFiltersFlag(DCAL_BITMASK(DFLAG_STATUS_EOS), p_component->p_scur_pipeline->pipeline_id)) {
                LOGM_INFO("[MSG flow]: before PostAppMsg(EMSGType_RecordingEOS).\n");
                p_component->status_flag |= DCAL_BITMASK(DFLAG_STATUS_EOS);
                memset(&msg0, 0x0, sizeof(msg0));
                msg0.code = EMSGType_RecordingEOS;
                msg0.sessionID = mSessionID;
                msg0.owner_id = p_component->p_scur_pipeline->pipeline_id;
                PostAppMsg(msg0);
                LOGM_INFO("[MSG flow]: PostAppMsg(EMSGType_RecordingEOS) done.\n");
            }
            break;

        case EMSGType_PlaybackEOS:
            LOGM_INFO("[MSG flow]: Playback EOS msg comes, IFilter %p, filter type %d, filter index %d, GenericID 0x%08x.\n", (IFilter *)msg.p1, p_component->type, p_component->index, p_component->id);

            DASSERT(p_component->p_scur_pipeline);
            DASSERT(EGenericPipelineType_Playback == p_component->p_scur_pipeline->pipeline_type);

            setFiltersEOS((IFilter *)msg.p_owner);
            if (true == allFiltersFlag(DCAL_BITMASK(DFLAG_STATUS_EOS), p_component->p_scur_pipeline->pipeline_id)) {
                LOGM_INFO("[MSG flow]: before PostAppMsg(EMSGType_PlaybackEOS).\n");
                p_component->status_flag |= DCAL_BITMASK(DFLAG_STATUS_EOS);
                memset(&msg0, 0x0, sizeof(msg0));
                msg0.code = EMSGType_PlaybackEOS;
                msg0.sessionID = mSessionID;
                msg0.owner_id = p_component->p_scur_pipeline->pipeline_id;
                PostAppMsg(msg0);
                LOGM_INFO("[MSG flow]: PostAppMsg(EMSGType_PlaybackEOS) done.\n");
            }
            break;

        case EMSGType_StorageError:
            //post msg to app
            LOGM_ERROR("[MSG flow]: EMSGType_StorageError msg comes, IFilter %p, type %d, index %d.\n", (IFilter *)msg.p_owner, p_component->type, p_component->index);
            memset(&msg0, 0x0, sizeof(msg0));
            msg0.code = EMSGType_StorageError;
            msg0.sessionID = mSessionID;
            msg0.owner_id = p_component->id;
            PostAppMsg(msg0);
            //abort recording, the muxed file maybe corrupted
            LOGM_ERROR("EMSGType_StorageError. TODO\n");
            break;

        case EMSGType_SystemError:
            LOGM_ERROR("[MSG flow]: EMSGType_SystemError msg comes, IFilter %p, type %d, index %d.\n", (IFilter *)msg.p_owner, p_component->type, p_component->index);
            memset(&msg0, 0x0, sizeof(msg0));
            msg0.code = EMSGType_SystemError;
            msg0.sessionID = mSessionID;
            msg0.owner_id = p_component->id;
            PostAppMsg(msg0);
            break;

        case EMSGType_IOError:
            LOGM_ERROR("[MSG flow]: EMSGType_IOError msg comes, IFilter %p, type %d, index %d.\n", (IFilter *)msg.p_owner, p_component->type, p_component->index);
            memset(&msg0, 0x0, sizeof(msg0));
            msg0.code = EMSGType_IOError;
            msg0.sessionID = mSessionID;
            msg0.owner_id = p_component->id;
            PostAppMsg(msg0);
            break;

        case EMSGType_DeviceError:
            LOGM_ERROR("[MSG flow]: EMSGType_DeviceError msg comes, IFilter %p, type %d, index %d.\n", (IFilter *)msg.p_owner, p_component->type, p_component->index);
            memset(&msg0, 0x0, sizeof(msg0));
            msg0.code = EMSGType_DeviceError;
            msg0.sessionID = mSessionID;
            msg0.owner_id = p_component->id;
            PostAppMsg(msg0);
            break;

        case EMSGType_StreamingError_TCPSocketConnectionClose:
            LOGM_NOTICE("EMSGType_StreamingError_TCPSocketConnectionClose msg comes, session %08lx, transmiter filter %08lx, streaming server %08lx\n", msg.p0, msg.p1, msg.p2);
            if (mpStreamingServerManager) {
                mpStreamingServerManager->RemoveSession((IStreamingServer *) msg.p2, (void *) msg.p0);
            }
            break;

        case EMSGType_StreamingError_UDPSocketInvalidArgument:
            LOGM_NOTICE("EMSGType_StreamingError_UDPSocketInvalidArgument msg comes, session %08lx, transmiter filter %08lx, streaming server %08lx\n", msg.p0, msg.p1, msg.p2);
            if (mpStreamingServerManager) {
                mpStreamingServerManager->RemoveSession((IStreamingServer *) msg.p2, (void *) msg.p0);
            }
            break;

        case EMSGType_NotifyNewFileGenerated:
            LOGM_INFO("EMSGType_NotifyNewFileGenerated msg comes, IFilter %p, filter type %d, index %d, current index %ld\n", (IFilter *)msg.p_owner, p_component->type, p_component->index, msg.p2);

            memset(&msg0, 0x0, sizeof(msg0));
            msg0.code = EMSGType_NotifyNewFileGenerated;
            msg0.sessionID = mSessionID;
            msg0.owner_id = p_component->id;
            PostAppMsg(msg0);
            break;

        case EMSGType_NotifyUDECStateChanges:
            LOGM_FLOW("EMSGType_NotifyUDECStateChanges msg comes, udec_id %lu\n", msg.p3);

            DASSERT(mpUniqueVideoRenderer);
            if (mpUniqueVideoRenderer) {
                SDSPControlParams params;
                params.u8_param[0] = msg.p3;
                mpUniqueVideoRenderer->FilterProperty(EFilterPropertyType_vrenderer_wake_vout, 1, &params);
            }
            break;

            //external cmd:
        case ECMDType_CloudSourceClient_UnknownCmd:
            LOGM_ERROR("ECMDType_CloudSourceClient_UnknownCmd, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            break;

        case ECMDType_CloudSourceClient_ReAuthentication:
            LOGM_NOTICE("ECMDType_CloudSourceClient_ReAuthentication, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            break;

        case ECMDType_CloudSinkClient_UpdateBitrate:
            LOGM_NOTICE("ECMDType_CloudSinkClient_UpdateBitrate, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            break;

        case ECMDType_CloudSinkClient_UpdateFrameRate:
            //LOGM_NOTICE("ECMDType_CloudSinkClient_UpdateFrameRate, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            break;

        case ECMDType_CloudSinkClient_UpdateDisplayLayout:
            LOGM_NOTICE("ECMDType_CloudSinkClient_UpdateDisplayLayout, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            processUpdateMudecDisplayLayout((TU32)msg.p0, (TU32)msg.p1);
            break;

        case ECMDType_CloudSinkClient_SelectAudioSource:
            LOGM_NOTICE("ECMDType_CloudSinkClient_SelectAudioSource, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            break;

        case ECMDType_CloudSinkClient_SelectAudioTarget:
            LOGM_NOTICE("ECMDType_CloudSinkClient_SelectAudioTarget, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            break;

        case ECMDType_CloudSinkClient_Zoom:
            LOGM_NOTICE("ECMDType_CloudSinkClient_Zoom, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            //processZoom((TU32)msg.p0, (TU32)msg.p1, (TU32)msg.p1, (TU32)msg.p2, (TU32)msg.p3);
            processZoom((TU32)msg.p0, (TU32)msg.p1, (TU32)msg.p2, (TU32)msg.p3);
            break;

        case ECMDType_CloudSinkClient_ZoomV2:
            LOGM_NOTICE("ECMDType_CloudSinkClient_ZoomV2, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            processZoom((TU32)msg.p0, (TU32)msg.p1, (TU32)msg.p2, (TU32)msg.p3, (TU32)msg.p4);
            break;

        case ECMDType_CloudSinkClient_UpdateDisplayWindow:
            LOGM_NOTICE("ECMDType_CloudSinkClient_UpdateDisplayWindow, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            processMoveWindow((TU32)msg.p0, (TU32)msg.p1, (TU32)msg.p2, (TU32)msg.p3, (TU32)msg.p4);
            break;

        case ECMDType_CloudSinkClient_SelectVideoSource:
            LOGM_NOTICE("ECMDType_CloudSinkClient_SelectVideoSource, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            break;

        case ECMDType_CloudSinkClient_ShowOtherWindows:
            LOGM_NOTICE("ECMDType_CloudSinkClient_ShowOtherWindows, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            processShowOtherWindows((TU32)msg.p0, (TU32)msg.p1);
            break;

        case ECMDType_CloudSinkClient_DemandIDR:
            LOGM_NOTICE("ECMDType_CloudSinkClient_DemandIDR, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            processDemandIDR((TU32)msg.p0);
            break;

        case ECMDType_CloudSinkClient_SwapWindowContent:
            LOGM_NOTICE("ECMDType_CloudSinkClient_SwapWindowContent, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            processSwapWindowContent((TU32)msg.p0, (TU32)msg.p1);
            break;

        case ECMDType_CloudSinkClient_CircularShiftWindowContent:
            LOGM_NOTICE("ECMDType_CloudSinkClient_CircularShiftWindowContent, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            processCircularShiftWindowContent((TU32)msg.p0, (TU32)msg.p1);
            break;

        case ECMDType_CloudClient_PeerClose:
            LOGM_NOTICE("ECMDType_CloudClient_PeerClose, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            p_component = findComponentFromID(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(msg.owner_type, msg.owner_index));
            if (DLikely(p_component && mpCloudServerManager)) {
                CIDoubleLinkedList::SNode *p_node = mCloudServerList.FirstNode();
                if (DLikely(p_node && p_node->p_context)) {
                    mpCloudServerManager->RemoveCloudClient((ICloudServer *)p_node->p_context, p_component->p_filter);
                } else {
                    LOGM_ERROR("NULL point\n");
                }
            } else {
                LOGM_ERROR("NULL point\n");
            }
            break;

        case ECMDType_CloudClient_QueryVersion:
            LOGM_NOTICE("ECMDType_CloudClient_QueryVersion, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            DASSERT(msg.p_agent_pointer);
            sendCloudAgentVersion((ICloudServerAgent *)msg.p_agent_pointer, (TU32)msg.p0, (TU32)msg.p1);
            break;

        case ECMDType_CloudClient_QueryStatus:
            LOGM_NOTICE("ECMDType_CloudClient_QueryStatus, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            DASSERT(msg.p_agent_pointer);
            sendSyncStatus((ICloudServerAgent *)msg.p_agent_pointer);
            break;

        case ECMDType_CloudClient_SyncStatus:
            LOGM_NOTICE("ECMDType_CloudClient_SyncStatus, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            DASSERT(msg.p_agent_pointer);
            //receiveSyncStatus((ICloudServerAgent*)msg.p_agent_pointer);
            break;

        default:
            LOGM_ERROR("unknown msg code %d\n", msg.code);
            break;
    }

    return true;
}

bool CActiveGenericEngine::processGenericCmd(SCMD &cmd)
{
    EECode err = EECode_OK;
    LOGM_FLOW("[cmd flow]: cmd.code %d, state %d.\n", cmd.code, msState);

    switch (cmd.code) {

        case ECMDType_Start:
            LOGM_FLOW("[cmd flow]: ECMDType_Start cmd, start.\n");
            err = doStartProcess();
            mpWorkQ->CmdAck(err);
            LOGM_FLOW("[cmd flow]: ECMDType_Start cmd, end.\n");
            break;

        case ECMDType_Stop:
            LOGM_FLOW("[cmd flow]: ECMDType_Stop cmd, start.\n");
            err = doStopProcess();
            mpWorkQ->CmdAck(err);
            LOGM_FLOW("[cmd flow]: ECMDType_Stop cmd, end.\n");
            break;

        case ECMDType_PrintCurrentStatus:
            LOGM_FLOW("[cmd flow]: ECMDType_PrintCurrentStatus cmd, start.\n");
            doPrintCurrentStatus((TGenericID)cmd.res32_1, (TULong)cmd.res64_1);
            LOGM_FLOW("[cmd flow]: ECMDType_PrintCurrentStatus cmd, end.\n");
            break;

        case ECMDType_ExitRunning:
            LOGM_FLOW("[cmd flow]: ECMDType_ExitRunning cmd, start.\n");
            mbRun = false;
            LOGM_FLOW("[cmd flow]: ECMDType_ExitRunning cmd, end.\n");
            break;

        case ECMDType_MiscCheckLicence: {
#ifdef BUILD_CONFIG_WITH_LICENCE
                EECode err = mpLicenceClient->DailyCheck();
                if (EECode_OK != err) {
                    LOGM_ERROR("DailyCheck fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                    err = doStopProcess();
                }
#endif
            }
            break;

        default:
            LOGM_ERROR("unknown cmd %d.\n", cmd.code);
            return false;
    }

    return true;
}

void CActiveGenericEngine::processCMD(SCMD &oricmd)
{
    TUint remaingCmd;
    SCMD cmd1;
    DASSERT(mpCmdQueue);

    if (oricmd.repeatType == CMD_TYPE_SINGLETON) {
        processGenericCmd(oricmd);
        return;
    }

    remaingCmd = mpCmdQueue->GetDataCnt();
    if (remaingCmd == 0) {
        processGenericCmd(oricmd);
        return;
    } else if (remaingCmd == 1) {
        processGenericCmd(oricmd);
        mpCmdQueue->PeekMsg(&cmd1, sizeof(cmd1));
        processGenericCmd(cmd1);
        return;
    }

    //process multi cmd
    if (oricmd.repeatType == CMD_TYPE_REPEAT_LAST) {
        while (mpCmdQueue->PeekMsg(&cmd1, sizeof(cmd1))) {
            if (cmd1.code != oricmd.code) {
                processGenericCmd(oricmd);
                processGenericCmd(cmd1);
                return;
            }
        }
        processGenericCmd(cmd1);
        return;
    } else if (oricmd.repeatType == CMD_TYPE_REPEAT_CNT) {
        //to do
        processGenericCmd(oricmd);
        return;
    } else if (oricmd.repeatType == CMD_TYPE_REPEAT_AVATOR) {
        while (mpCmdQueue->PeekMsg(&cmd1, sizeof(cmd1))) {
            if (cmd1.code != oricmd.code) {
                processGenericCmd(oricmd);
                processGenericCmd(cmd1);
                return;
            } else {
                //to do
                processGenericCmd(cmd1);
                return;
            }
        }
    } else {
        LOGM_FATAL("must not come here.\n");
        return;
    }
}

void CActiveGenericEngine::PrintStatus()
{
    LOGM_PRINTF("[status]: CActiveGenericEngine's state is %d.\n", msState);

    if (mpStreamingServerManager) {
        //mpStreamingServerManager->PrintStatus();
    }
}

EECode CActiveGenericEngine::BeginConfigProcessPipeline(TUint customized_pipeline)
{
    DASSERT(EEngineState_not_alive == msState);
    DASSERT(false == mbGraghBuilt);

    return EECode_OK;
}

EECode CActiveGenericEngine::NewComponent(TUint component_type, TGenericID &component_id)
{
    SComponent_Obsolete *component = NULL;
    EECode err = EECode_OK;

    if (DLikely((EGenericComponentType_TAG_filter_start <= component_type) && (EGenericComponentType_TAG_filter_end > component_type))) {

        if (isUniqueComponentCreated(component_type)) {
            LOGM_FATAL("Component is unique, must not comes here\n");
            return EECode_BadParam;
        }

        DASSERT(mnComponentFiltersNumbers[component_type] < mnComponentFiltersMaxNumbers[component_type]);
        if (mnComponentFiltersNumbers[component_type] < mnComponentFiltersMaxNumbers[component_type]) {
            component_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(component_type, mnComponentFiltersNumbers[component_type]);

            component = newComponent((TComponentType)component_type, (TComponentIndex)mnComponentFiltersNumbers[component_type]);
            if (component) {
                mListFilters.InsertContent(NULL, (void *)component, 0);
            } else {
                LOGM_ERROR("NO memory\n");
                return EECode_NoMemory;
            }

            mnComponentFiltersNumbers[component_type] ++;
        } else {
            LOGM_ERROR("too many components %d now, max value %d, component type %d.\n", mnComponentFiltersNumbers[component_type], mnComponentFiltersMaxNumbers[component_type], component_type);
            return EECode_OutOfCapability;
        }

    } else {

        switch (component_type) {
            case EGenericComponentType_StreamingServer: {
                    err = addStreamingServer(component_id,  StreamingServerType_RTSP, StreamingServerMode_Unicast);
                    if (DUnlikely(EECode_OK != err)) {
                        LOGM_ERROR("addStreamingServer fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                    } else {
                        component = newComponent((TComponentType)component_type,  DCOMPONENT_INDEX_FROM_GENERIC_ID(component_id));
                        if (DLikely(component)) {
                            component->id = component_id;
                            component->type = DCOMPONENT_TYPE_FROM_GENERIC_ID(component_id);
                            component->index = DCOMPONENT_INDEX_FROM_GENERIC_ID(component_id);
                            mListProxys.InsertContent(NULL, (void *)component, 0);
                        } else {
                            LOGM_ERROR("NO memory\n");
                            return EECode_NoMemory;
                        }
                    }
                }
                break;

            case EGenericComponentType_StreamingContent: {
                    err = newStreamingContent(component_id);
                    if (DUnlikely(EECode_OK != err)) {
                        LOGM_ERROR("newStreamingContent fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                    } else {
                        component = newComponent((TComponentType)component_type,  DCOMPONENT_INDEX_FROM_GENERIC_ID(component_id));
                        if (DLikely(component)) {
                            component->id = component_id;
                            component->type = DCOMPONENT_TYPE_FROM_GENERIC_ID(component_id);
                            component->index = DCOMPONENT_INDEX_FROM_GENERIC_ID(component_id);
                            mListProxys.InsertContent(NULL, (void *)component, 0);
                        } else {
                            LOGM_ERROR("NO memory\n");
                            return EECode_NoMemory;
                        }
                    }
                }
                break;

            case EGenericComponentType_CloudServer: {
                    TU16 port = DDefaultSACPServerPort;
                    if (mPersistMediaConfig.sacp_cloud_server_config.sacp_listen_port) {
                        port = mPersistMediaConfig.sacp_cloud_server_config.sacp_listen_port;
                    }
                    LOGM_CONFIGURATION("sacp server port %d\n", port);
                    err = addCloudServer(component_id, CloudServerType_SACP, port);
                    if (DUnlikely(EECode_OK != err)) {
                        LOGM_ERROR("addCloudServer fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                    } else {
                        component = newComponent((TComponentType)component_type,  DCOMPONENT_INDEX_FROM_GENERIC_ID(component_id));
                        if (DLikely(component)) {
                            component->id = component_id;
                            component->type = DCOMPONENT_TYPE_FROM_GENERIC_ID(component_id);
                            component->index = DCOMPONENT_INDEX_FROM_GENERIC_ID(component_id);
                            mListProxys.InsertContent(NULL, (void *)component, 0);
                        } else {
                            LOGM_ERROR("NO memory\n");
                            return EECode_NoMemory;
                        }
                    }
                }
                break;

            case EGenericComponentType_CloudReceiverContent: {
                    err = newCloudReceiverContent(component_id);
                    if (DUnlikely(EECode_OK != err)) {
                        LOGM_ERROR("newCloudReceiverContent fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                    } else {
                        component = newComponent((TComponentType)component_type,  DCOMPONENT_INDEX_FROM_GENERIC_ID(component_id));
                        if (DLikely(component)) {
                            component->id = component_id;
                            component->type = DCOMPONENT_TYPE_FROM_GENERIC_ID(component_id);
                            component->index = DCOMPONENT_INDEX_FROM_GENERIC_ID(component_id);
                            mListProxys.InsertContent(NULL, (void *)component, 0);
                        } else {
                            LOGM_ERROR("NO memory\n");
                            return EECode_NoMemory;
                        }
                    }
                }
                break;

            default:
                LOGM_ERROR("BAD component type %d\n", component_type);
                return EECode_BadParam;
                break;
        }

    }

    LOGM_PRINTF("[Gragh]: NewComponent(%s), id 0x%08x\n", gfGetComponentStringFromGenericComponentType((TComponentType)component_type), component_id);

    return err;
}

EECode CActiveGenericEngine::ConnectComponent(TGenericID &connection_id, TGenericID upper_component_id, TGenericID down_component_id, StreamType pin_type)
{
    SConnection_Obsolete *connection;
    SComponent_Obsolete *up_component = NULL, *down_component = NULL;

    TU8 up_type = DCOMPONENT_TYPE_FROM_GENERIC_ID(upper_component_id);
    TU8 down_type = DCOMPONENT_TYPE_FROM_GENERIC_ID(down_component_id);
    TComponentIndex up_index = DCOMPONENT_INDEX_FROM_GENERIC_ID(upper_component_id);
    TComponentIndex down_index = DCOMPONENT_INDEX_FROM_GENERIC_ID(down_component_id);

    //debug assert
    DASSERT(upper_component_id != down_component_id);
    if (upper_component_id == down_component_id) {
        LOGM_ERROR("upper_component_id == down_component_id? please check code\n");
        return EECode_BadParam;
    }
    DASSERT(up_type < EGenericComponentType_TAG_filter_end);
    DASSERT(down_type < EGenericComponentType_TAG_filter_end);
    DASSERT(up_index < mnComponentFiltersNumbers[up_type]);
    DASSERT(down_index < mnComponentFiltersNumbers[down_type]);
    if (DUnlikely((up_type >= EGenericComponentType_TAG_filter_end) || (down_type >= EGenericComponentType_TAG_filter_end) || (up_index >= mnComponentFiltersNumbers[up_type]) || (down_index >= mnComponentFiltersNumbers[down_type]))) {
        LOGM_ERROR("BAD parameters up type %d, down type %d, up index %d, down index %d\n", up_type, down_type, up_index, down_index);
        return EECode_BadParam;
    }

    up_component = findComponentFromID(upper_component_id);
    down_component = findComponentFromID(down_component_id);

    if ((!up_component) || (!down_component)) {
        LOGM_ERROR("BAD component id, cannot find component, upper %08x, down %08x, upper %p, down %p\n", upper_component_id, down_component_id, up_component, down_component);
        return EECode_BadParam;
    }

    connection = newConnetion(upper_component_id, down_component_id, up_component, down_component);
    if (connection) {
        mListConnections.InsertContent(NULL, (void *)connection, 0);
    } else {
        LOGM_ERROR("NO memory\n");
        return EECode_NoMemory;
    }

    connection_id = connection->connection_id;
    connection->pin_type = pin_type;

    LOGM_PRINTF("[Gragh]: ConnectComponent(%s[%d] to %s[%d]), pin type %d, connection id 0x%08x\n", gfGetComponentStringFromGenericComponentType((TComponentType)up_type), up_index, gfGetComponentStringFromGenericComponentType((TComponentType)down_type), down_index, pin_type, connection_id);

    return EECode_OK;
}

TUint CActiveGenericEngine::isUniqueComponentCreated(TU8 type)
{
    switch (type) {

        case EGenericComponentType_VideoPostProcessor:
            if (mpUniqueVideoPostProcessor) {
                LOGM_FATAL("mpUniqueVideoPostProcessor(should be unique) already created!\n");
                return 1;
            }
            break;

        case EGenericComponentType_AudioPostProcessor:
            if (mpUniqueAudioPostProcessor) {
                LOGM_FATAL("mpUniqueAudioPostProcessor(should be unique) already created!\n");
                return 1;
            }
            break;

        case EGenericComponentType_VideoRenderer:
            if (mpUniqueVideoRenderer) {
                LOGM_FATAL("mpUniqueVideoRenderer(should be unique) already created!\n");
                return 1;
            }
            break;

        case EGenericComponentType_AudioRenderer:
            if (mpUniqueAudioRenderer) {
                LOGM_FATAL("mpUniqueAudioRenderer(should be unique) already created!\n");
                return 1;
            }
            break;

        case EGenericComponentType_StreamingTransmiter:
            if (mpUniqueRTSPStreamingTransmiter) {
                LOGM_FATAL("mpUniqueRTSPStreamingTransmiter(should be unique) already created!\n");
                return 1;
            }
            break;

        default:
            DASSERT((EGenericComponentType_TAG_filter_end - EGenericComponentType_TAG_filter_start) > type);
            if ((EGenericComponentType_TAG_filter_end - EGenericComponentType_TAG_filter_start) <= type) {
                LOGM_FATAL("BAD input type %d\n", type);
                return 2;
            }
            break;
    }

    return 0;
}

TUint CActiveGenericEngine::setUniqueComponent(TU8 type, IFilter *pFilter, SComponent_Obsolete *p_component)
{
    DASSERT(pFilter);
    DASSERT(p_component);

    switch (type) {

        case EGenericComponentType_AudioDecoder:
            if (mpUniqueAudioDecoder) {
                LOGM_FATAL("mpUniqueAudioDecoder(should be unique) already created!\n");
                return 1;
            }
            mpUniqueAudioDecoder = pFilter;
            mpUniqueAudioDecoderComponent = p_component;
            break;

        case EGenericComponentType_VideoPostProcessor:
            if (mpUniqueVideoPostProcessor) {
                LOGM_FATAL("mpUniqueVideoPostProcessor(should be unique) already created!\n");
                return 1;
            }
            mpUniqueVideoPostProcessor = pFilter;
            mpUniqueVideoPostProcessorComponent = p_component;
            break;

        case EGenericComponentType_AudioPostProcessor:
            if (mpUniqueAudioPostProcessor) {
                LOGM_FATAL("mpUniqueAudioPostProcessor(should be unique) already created!\n");
                return 1;
            }
            mpUniqueAudioPostProcessor = pFilter;
            mpUniqueAudioPostProcessorComponent = p_component;
            break;

        case EGenericComponentType_VideoRenderer:
            if (mpUniqueVideoRenderer) {
                LOGM_FATAL("mpUniqueVideoRenderer(should be unique) already created!\n");
                return 1;
            }
            mpUniqueVideoRenderer = pFilter;
            mpUniqueVideoRendererComponent = p_component;
            mpUniqueVideoPostProcessor = pFilter;
            mpUniqueVideoPostProcessorComponent = p_component;
            break;

        case EGenericComponentType_AudioRenderer:
            if (mpUniqueAudioRenderer) {
                LOGM_FATAL("mpUniqueAudioRenderer(should be unique) already created!\n");
                return 1;
            }
            mpUniqueAudioRenderer = pFilter;
            mpUniqueAudioRendererComponent = p_component;
            break;

        case EGenericComponentType_StreamingTransmiter:
            if (mpUniqueRTSPStreamingTransmiter) {
                LOGM_FATAL("mpUniqueRTSPStreamingTransmiter(should be unique) already created!\n");
                return 1;
            }
            mpUniqueRTSPStreamingTransmiter = pFilter;
            mpUniqueRTSPStreamingTransmiterComponent = p_component;
            break;

        case EGenericComponentType_ConnecterPinMuxer:
            if (mpUniqueAudioPinMuxer) {
                LOGM_FATAL("mpUniqueAudioPinMuxer(should be unique) already created!\n");
                return 1;
            }
            mpUniqueAudioPinMuxer = pFilter;
            mpUniqueAudioPinMuxerComponent = p_component;
            break;

        default:
            DASSERT(EGenericComponentType_TAG_filter_end > type);
            if (EGenericComponentType_TAG_filter_end <= type) {
                LOGM_FATAL("BAD input type %d\n", type);
                return 2;
            }
            break;
    }

    return 0;
}

SComponent_Obsolete *CActiveGenericEngine::newComponent(TComponentType type, TComponentIndex index)
{
    SComponent_Obsolete *p = (SComponent_Obsolete *) DDBG_MALLOC(sizeof(SComponent_Obsolete), "E0CC");
    //CIDoubleLinkedList* ptmp;

    if (p) {
        memset(p, 0x0, sizeof(SComponent_Obsolete));
        p->id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(type, index);
        p->type = type;
        p->index = index;
        p->cur_state = EComponentState_not_alive;

        //placement new
        new(&p->pipelines) CIDoubleLinkedList();

        //placement destroy
        //p->pipelines.~CIDoubleLinkedList();
    }

    return p;
}

void CActiveGenericEngine::deleteComponent(SComponent_Obsolete *p)
{
    CIDoubleLinkedList *p_list;

    if (p) {
        if (p->url) {
            free(p->url);
            p->url = NULL;
        }

        p_list = &p->pipelines;
        p_list->~CIDoubleLinkedList();
        free(p);
    }
}

SComponent_Obsolete *CActiveGenericEngine::findComponentFromID(TGenericID id)
{
    CIDoubleLinkedList::SNode *pnode = mListFilters.FirstNode();
    SComponent_Obsolete *p_component = NULL;

    while (pnode) {
        p_component = (SComponent_Obsolete *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            if (p_component->id == id) {
                return p_component;
            }
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mListFilters.NextNode(pnode);
    }

    pnode = mListProxys.FirstNode();
    while (pnode) {
        p_component = (SComponent_Obsolete *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            if (p_component->id == id) {
                return p_component;
            }
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mListProxys.NextNode(pnode);
    }

    LOGM_ERROR("cannot find component, id 0x%08x\n", id);
    return NULL;
}

SComponent_Obsolete *CActiveGenericEngine::findComponentFromTypeIndex(TU8 type, TU8 index)
{
    TGenericID id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(type, index);

    return findComponentFromID(id);
}

SComponent_Obsolete *CActiveGenericEngine::findComponentFromFilter(IFilter *p_filter)
{
    CIDoubleLinkedList::SNode *pnode = mListFilters.FirstNode();
    SComponent_Obsolete *p_component = NULL;

    while (pnode) {
        p_component = (SComponent_Obsolete *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            if (p_component->p_filter == p_filter) {
                return p_component;
            }
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mListFilters.NextNode(pnode);
    }

    LOGM_WARN("cannot find component, by filter %p\n", p_filter);
    return NULL;
}

SConnection_Obsolete *CActiveGenericEngine::newConnetion(TGenericID up_id, TGenericID down_id, SComponent_Obsolete *up_component, SComponent_Obsolete *down_component)
{
    SConnection_Obsolete *p = (SConnection_Obsolete *) DDBG_MALLOC(sizeof(SConnection_Obsolete), "E0Cc");
    if (p) {
        memset(p, 0x0, sizeof(SConnection_Obsolete));
        p->connection_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_ConnectionPin, mnComponentProxysNumbers[EGenericComponentType_ConnectionPin - EGenericComponentType_TAG_proxy_start]++);
        p->up_id = up_id;
        p->down_id = down_id;

        p->up = up_component;
        p->down = down_component;

        p->state = EConnectionState_not_alive;
    }

    return p;
}

void CActiveGenericEngine::deleteConnection(SConnection_Obsolete *p)
{
    if (p) {
        DDBG_FREE(p, "E0Cc");
    }
}

SPlaybackPipelines *CActiveGenericEngine::newPlaybackPipelines()
{
    SPlaybackPipelines *pipelines = (SPlaybackPipelines *)DDBG_MALLOC(sizeof(SPlaybackPipelines), "E0PP");
    if (pipelines) {
        memset((void *)pipelines, 0x0, sizeof(SPlaybackPipelines));
    }
    return pipelines;
}

void CActiveGenericEngine::deletePlaybackPipelines(SPlaybackPipelines *pipelines)
{
    if (pipelines) {
        DDBG_FREE(pipelines, "E0PP");
    }
}

SGenericPipeline *CActiveGenericEngine::queryRecordingPipeline(TGenericID sink_id)
{
    SGenericPipeline *p;
    TUint index = 0, current_rec_pipeline = mRecordingPipelines.rec_pipeline_number;

    DASSERT(mRecordingPipelines.rec_pipeline_number <= EPipelineMaxNumber_Recording);

    while (index < current_rec_pipeline) {
        p = &mRecordingPipelines.p_rec_pipeline_head[index];
        if (sink_id == p->rec_sink_id) {
            return p;
        }
        index ++;
    }

    LOGM_ERROR("cannot find recording pipeline(id 0x%08x)\n", sink_id);
    return NULL;
}

SGenericPipeline *CActiveGenericEngine::queryPlaybackPipelineFromSourceID(TGenericID source_id, TU8 index)
{
    SPlaybackPipelines *p = mpPlaybackPipelines;
    TU8 source_index = DCOMPONENT_INDEX_FROM_GENERIC_ID(source_id);

    if (!p) {
        LOGM_ERROR("No playback engines!!\n");
        return NULL;
    }

    while (p) {
        if (p->source_index == source_index) {
            LOGM_INFO("find it\n");
            DASSERT(p->pb_pipeline_number);
            DASSERT(index < p->pb_pipeline_number);
            if ((p->pb_pipeline_number) && (index < p->pb_pipeline_number)) {
                DASSERT(source_id == p->p_pb_pipeline_head[index].pb_source_id);
                return &p->p_pb_pipeline_head[index];
            }

            LOGM_ERROR("bad index %d ,p->pb_pipeline_number %d?\n", index, p->pb_pipeline_number);
            return NULL;
        }
        p = p->p_next;
    }

    LOGM_ERROR("cannot find playback pipeline(id 0x%08x)\n", source_id);
    return NULL;
}

SGenericPipeline *CActiveGenericEngine::queryPlaybackPipeline(TGenericID pipeline_id)
{
    TUint index = 0;
    SPlaybackPipelines *p = mpPlaybackPipelines;

    if (!p) {
        LOGM_ERROR("No playback engines!!\n");
        return NULL;
    }

    while (p) {
        for (index = 0; index < p->pb_pipeline_number; index ++) {
            if (pipeline_id == p->p_pb_pipeline_head[index].pipeline_id) {
                return &p->p_pb_pipeline_head[index];
            }
        }
        p = p->p_next;
    }

    LOGM_ERROR("cannot find playback pipeline(id 0x%08x)\n", pipeline_id);
    return NULL;
}

SGenericPipeline *CActiveGenericEngine::queryRTSPStreamingPipeline(TGenericID content_id)
{
    SGenericPipeline *p = NULL;
    TUint index = 0, current_streaming_pipeline = mRTSPStreamingPipelines.streaming_pipeline_number;

    DASSERT(current_streaming_pipeline <= EPipelineMaxNumber_Streaming);
    while (index < current_streaming_pipeline) {
        p = &mRTSPStreamingPipelines.p_streaming_pipeline_head[index];
        //LOGM_VERBOSE("[DEBUG]: index %d, current_streaming_pipeline %d, content_id 0x%08x, p->rtsp_content_id 0x%08x\n", index, current_streaming_pipeline, content_id, p->rtsp_content_id);
        if (content_id == p->rtsp_content_id) {
            return p;
        }
        index ++;
    }

    LOGM_ERROR("cannot find streaming pipeline(id 0x%08x)\n", content_id);
    return NULL;
}

SGenericPipeline *CActiveGenericEngine::queryPipeline(TGenericID pipeline_id)
{
    TUint i = 0;

    if (EGenericPipelineType_Playback == DCOMPONENT_TYPE_FROM_GENERIC_ID(pipeline_id)) {
        SPlaybackPipelines *p = mpPlaybackPipelines;

        if (!p) {
            LOGM_ERROR("No playback engines!!\n");
            return NULL;
        }

        while (p) {
            for (i = 0; i < p->pb_pipeline_number; i++) {
                if ((p->p_pb_pipeline_head[i].pipeline_id) == (pipeline_id)) {
                    return &p->p_pb_pipeline_head[i];
                }
            }
            p = p->p_next;
        }
    } else if (EGenericPipelineType_Recording == DCOMPONENT_TYPE_FROM_GENERIC_ID(pipeline_id)) {
        SGenericPipeline *p;
        TUint current_rec_pipeline = mRecordingPipelines.rec_pipeline_number;

        DASSERT(mRecordingPipelines.rec_pipeline_number <= EPipelineMaxNumber_Recording);

        while (i < current_rec_pipeline) {
            p = &mRecordingPipelines.p_rec_pipeline_head[i];
            if (p->pipeline_id == pipeline_id) {
                return p;
            }
            i ++;
        }
    }

    LOGM_ERROR("cannot find pipeline(id 0x%08x)\n", pipeline_id);
    return NULL;
}

SComponent_Obsolete *CActiveGenericEngine::queryFilter(TGenericID filter_id)
{
    CIDoubleLinkedList::SNode *pnode = mListFilters.FirstNode();
    SComponent_Obsolete *p_component = NULL;

    while (pnode) {
        p_component = (SComponent_Obsolete *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component->id == filter_id) {
            return p_component;
        }
        pnode = mListFilters.NextNode(pnode);
    }

    return NULL;
}

SComponent_Obsolete *CActiveGenericEngine::queryFilter(TComponentType type, TComponentIndex index)
{
    CIDoubleLinkedList::SNode *pnode = mListFilters.FirstNode();
    SComponent_Obsolete *p_component = NULL;

    while (pnode) {
        p_component = (SComponent_Obsolete *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component->type == type
                && p_component->index == index) {
            return p_component;
        }
        pnode = mListFilters.NextNode(pnode);
    }

    return NULL;
}

SConnection_Obsolete *CActiveGenericEngine::queryConnection(TGenericID connection_id)
{
    CIDoubleLinkedList::SNode *pnode = mListConnections.FirstNode();
    SConnection_Obsolete *p_connection = NULL;

    while (pnode) {
        p_connection = (SConnection_Obsolete *)(pnode->p_context);
        DASSERT(p_connection);
        if (p_connection->connection_id == connection_id) {
            return p_connection;
        }
        pnode = mListConnections.NextNode(pnode);
    }

    return NULL;
}

SConnection_Obsolete *CActiveGenericEngine::queryConnection(TGenericID up_id, TGenericID down_id, CIDoubleLinkedList::SNode *&start_node, StreamType type)
{
    CIDoubleLinkedList::SNode *pnode = NULL;
    SConnection_Obsolete *p_connection = NULL;

    //LOGM_DEBUG("queryConnection: up_id 0x%08x, down_id 0x%08x, type %d\n", up_id, down_id, type);

    if (NULL == start_node) {
        pnode = mListConnections.FirstNode();
    } else {
        pnode = mListConnections.NextNode(start_node);
    }

    while (pnode) {
        p_connection = (SConnection_Obsolete *)(pnode->p_context);
        DASSERT(p_connection);
        //LOGM_DEBUG("p_connection %p: p_connection->up_id 0x%08x, p_connection->down_id 0x%08x, type %d\n", p_connection, p_connection->up_id, p_connection->down_id, p_connection->pin_type);
        //LOGM_DEBUG("up_id 0x%08x, down_id 0x%08x, type %d\n", up_id, down_id, type);
        if ((p_connection->up_id == up_id) && (p_connection->down_id == down_id)) {
            if (StreamType_Invalid == type) {
                start_node = pnode;
                return p_connection;
            } else if (p_connection->pin_type == type) {
                start_node = pnode;
                return p_connection;
            } else {
                //LOGM_DEBUG("pin type not match, %d, %d\n", p_connection->pin_type, type);
            }
        }
        pnode = mListConnections.NextNode(pnode);
    }

    return NULL;
}

EECode CActiveGenericEngine::QueryPipelineID(TU8 pipeline_type, TGenericID source_sink_id, TGenericID &pipeline_id, TU8 index)
{
    SGenericPipeline *pipeline = NULL;
    if (EGenericPipelineType_Playback == pipeline_type) {
        pipeline = queryPlaybackPipelineFromSourceID(source_sink_id, index);
        if (pipeline) {
            pipeline_id = pipeline->pipeline_id;
        } else {
            LOGM_ERROR("cannot query playback pipeline, please check parameters, source_sink_id 0x%08x, index %d\n", source_sink_id, index);
            return EECode_BadParam;
        }
    } else if (EGenericPipelineType_Recording == pipeline_type) {
        pipeline = queryRecordingPipeline(source_sink_id);
        if (pipeline) {
            pipeline_id = pipeline->pipeline_id;
        } else {
            LOGM_ERROR("cannot query recording pipeline, please check parameters, source_sink_id 0x%08x\n", source_sink_id);
            return EECode_BadParam;
        }
    } else if (EGenericPipelineType_Streaming == pipeline_type) {
        pipeline = queryRTSPStreamingPipeline(source_sink_id);
        if (pipeline) {
            pipeline_id = pipeline->pipeline_id;
        } else {
            LOGM_ERROR("cannot query streaming pipeline, please check parameters, source_sink_id 0x%08x\n", source_sink_id);
            return EECode_BadParam;
        }
    } else {
        LOGM_ERROR("BAD pipeline_type %d\n", pipeline_type);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::QueryPipelineID(TU8 pipeline_type, TU8 source_sink_index, TGenericID &pipeline_id, TU8 index)
{
    SGenericPipeline *pipeline = NULL;
    TGenericID source_sink_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(pipeline_type, source_sink_index);

    if (EGenericPipelineType_Playback == pipeline_type) {
        pipeline = queryPlaybackPipelineFromSourceID(source_sink_id, index);
        if (pipeline) {
            pipeline_id = pipeline->pipeline_id;
        } else {
            LOGM_ERROR("cannot query playback pipeline, please check parameters, source_sink_id 0x%08x, index %d\n", source_sink_id, index);
            return EECode_BadParam;
        }
    } else if (EGenericPipelineType_Recording == pipeline_type) {
        pipeline = queryRecordingPipeline(source_sink_id);
        if (pipeline) {
            pipeline_id = pipeline->pipeline_id;
        } else {
            LOGM_ERROR("cannot query recording pipeline, please check parameters, source_sink_id 0x%08x\n", source_sink_id);
            return EECode_BadParam;
        }
    } else if (EGenericPipelineType_Streaming == pipeline_type) {
        pipeline = queryRTSPStreamingPipeline(source_sink_id);
        if (pipeline) {
            pipeline_id = pipeline->pipeline_id;
        } else {
            LOGM_ERROR("cannot query streaming pipeline, please check parameters, source_sink_id 0x%08x\n", source_sink_id);
            return EECode_BadParam;
        }
    } else {
        LOGM_ERROR("BAD pipeline_type %d\n", pipeline_type);
        return EECode_BadParam;
    }

    return EECode_OK;
}

SGenericPipeline *CActiveGenericEngine::getRecordingPipeline(TComponentIndex sink_index)
{
    SGenericPipeline *p;
    TUint index = 0, current_rec_pipeline = mRecordingPipelines.rec_pipeline_number;

    DASSERT(current_rec_pipeline <= EPipelineMaxNumber_Recording);

    while (index < current_rec_pipeline) {
        p = &mRecordingPipelines.p_rec_pipeline_head[index];
        if (sink_index == p->source_sink_index) {
            LOGM_INFO("getRecordingPipeline, find it, sink_index %d\n", sink_index);
            return p;
        }
        index ++;
    }

    if (EPipelineMaxNumber_Recording > mRecordingPipelines.rec_pipeline_number) {
        index = mRecordingPipelines.rec_pipeline_number;
        p = &mRecordingPipelines.p_rec_pipeline_head[index];
        mRecordingPipelines.rec_pipeline_number ++;

        p->pipeline_type = EGenericPipelineType_Recording;
        p->pipeline_index = index;
        p->pipeline_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(p->pipeline_type, p->pipeline_index);
        return p;
    } else if (EPipelineMaxNumber_Recording == mRecordingPipelines.rec_pipeline_number) {
        LOGM_ERROR("reach max recording engine number %d\n", EPipelineMaxNumber_Recording);
    } else {
        LOGM_ERROR("!!!Internal error, please check code, mRecordingPipelines.rec_pipeline_number %d\n", mRecordingPipelines.rec_pipeline_number);
    }

    return NULL;
}

SPlaybackPipelines *CActiveGenericEngine::getPlaybackPipelines(TComponentIndex source_index)
{
    SPlaybackPipelines *p = mpPlaybackPipelines;
    SPlaybackPipelines *tail = NULL;

    if (!p) {
        DASSERT(0 == mnPlaybackPipelinesNumber);
        p = newPlaybackPipelines();
        LOGM_INFO("alloc first playback pipelines, %p\n", p);
        if (!p) {
            LOGM_ERROR("NO memory\n");
            return NULL;
        }

        p->source_index = source_index;
        mpPlaybackPipelines = p;

        return mpPlaybackPipelines;
    }

    tail = p;
    while (p) {
        if (p->source_index == source_index) {
            LOGM_INFO("find it\n");
            return p;
        }
        tail = p;
        p = p->p_next;
    }

    //create new
    DASSERT(!p);
    p = newPlaybackPipelines();
    LOGM_INFO("alloc first playback pipelines, %p\n", p);
    if (!p) {
        LOGM_ERROR("NO memory\n");
        return NULL;
    }

    p->source_index = source_index;
    tail->p_next = p;
    return p;
}

SGenericPipeline *CActiveGenericEngine::getStreamingPipeline(TGenericID id)
{
    SGenericPipeline *p;
    TComponentIndex index = 0, current_streaming_pipeline = mRTSPStreamingPipelines.streaming_pipeline_number;

    DASSERT(current_streaming_pipeline <= EPipelineMaxNumber_Streaming);
    //LOGM_DEBUG("getStreamingPipeline(0x%08x), current_streaming_pipeline %d\n", id, current_streaming_pipeline);
    while (index < current_streaming_pipeline) {
        p = &mRTSPStreamingPipelines.p_streaming_pipeline_head[index];
        //LOGM_DEBUG("p->streaming_content_id(0x%08x)\n", p->streaming_content_id);
        if (id == p->streaming_content_id) {
            return p;
        }
        index ++;
    }

    if (EPipelineMaxNumber_Streaming > mRTSPStreamingPipelines.streaming_pipeline_number) {
        index = mRTSPStreamingPipelines.streaming_pipeline_number;
        p = &mRTSPStreamingPipelines.p_streaming_pipeline_head[index];
        mRTSPStreamingPipelines.streaming_pipeline_number ++;

        p->streaming_content_id = id;
        p->pipeline_type = EGenericPipelineType_Streaming;
        p->pipeline_index = index;
        p->pipeline_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(p->pipeline_type, p->pipeline_index);
        //LOGM_INFO("p->pipeline_id 0x%08x\n", p->pipeline_id);
        return p;
    } else if (EPipelineMaxNumber_Streaming == mRTSPStreamingPipelines.streaming_pipeline_number) {
        LOGM_ERROR("reach max rtsp streaming engine number %d\n", EPipelineMaxNumber_Streaming);
    } else {
        LOGM_ERROR("!!!Internal error, please check code, mRTSPStreamingPipelines.streaming_pipeline_number %d\n", mRTSPStreamingPipelines.streaming_pipeline_number);
    }

    return NULL;
}

void CActiveGenericEngine::setupInternalPlaybackPipelineInfos(SComponent_Obsolete *p_component)
{
    //based on video path
    TGenericID first_audio_id = 0;
    TGenericID first_audio_pin_id = 0;

    TInt has_audio = 0;
    CIDoubleLinkedList::SNode *pnode = mListFilters.FirstNode();
    SConnection_Obsolete *p_connection = NULL;

    SPlaybackPipelines *pb_pipelines = NULL;

    //debug assert
    DASSERT(p_component);

    if (p_component) {

        if (EGenericComponentType_Demuxer != p_component->type) {
            LOGM_ERROR("pb engine should start from a source component(%d)\n", p_component->type);
            return;
        }

        //find audio first
        pnode = mListConnections.FirstNode();
        while (pnode) {
            p_connection = (SConnection_Obsolete *)(pnode->p_context);
            DASSERT(p_connection);
            if ((p_connection) && (p_connection->up_id == p_component->id)) {
                if (EGenericComponentType_AudioDecoder == DCOMPONENT_TYPE_FROM_GENERIC_ID(p_connection->down_id)) {
                    has_audio = 1;
                    first_audio_id = p_connection->down_id;
                    first_audio_pin_id = p_connection->connection_id;
                    break;
                } else if ((EGenericComponentType_ConnecterPinMuxer == DCOMPONENT_TYPE_FROM_GENERIC_ID(p_connection->down_id)) && mpUniqueAudioDecoderComponent) {
                    has_audio = 1;
                    first_audio_id = mpUniqueAudioDecoderComponent->id;
                    first_audio_pin_id = p_connection->connection_id;
                    break;
                }
            }
            pnode = mListConnections.NextNode(pnode);
        }

        //find pipelines
        pb_pipelines = getPlaybackPipelines(p_component->index);
        DASSERT(pb_pipelines);
        if (!pb_pipelines) {
            return;
        }

        //find each video path
        pnode = mListConnections.FirstNode();
        while (pnode) {
            p_connection = (SConnection_Obsolete *)(pnode->p_context);
            DASSERT(p_connection);
            if ((p_connection) && (p_connection->up_id == p_component->id)) {
                if (EGenericComponentType_VideoDecoder == DCOMPONENT_TYPE_FROM_GENERIC_ID(p_connection->down_id)) {

                    //add a pb engine
                    DASSERT(pb_pipelines->pb_pipeline_number <= EMaxSinkNumber_Demuxer);
                    if (pb_pipelines->pb_pipeline_number < (EMaxSinkNumber_Demuxer - 1)) {

                        pb_pipelines->p_pb_pipeline_head[pb_pipelines->pb_pipeline_number].pipeline_type = EGenericPipelineType_Playback;
                        pb_pipelines->p_pb_pipeline_head[pb_pipelines->pb_pipeline_number].pipeline_index = mnPlaybackPipelinesNumber;
                        pb_pipelines->p_pb_pipeline_head[pb_pipelines->pb_pipeline_number].pipeline_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Playback, mnPlaybackPipelinesNumber);

                        pb_pipelines->p_pb_pipeline_head[pb_pipelines->pb_pipeline_number].source_sink_index = p_component->index;
                        pb_pipelines->p_pb_pipeline_head[pb_pipelines->pb_pipeline_number].pipeline_state = EGenericPipelineState_build_gragh;

                        pb_pipelines->p_pb_pipeline_head[pb_pipelines->pb_pipeline_number].pb_source_id = p_component->id;
                        pb_pipelines->p_pb_pipeline_head[pb_pipelines->pb_pipeline_number].pb_video_decoder_id = p_connection->down_id;
                        pb_pipelines->p_pb_pipeline_head[pb_pipelines->pb_pipeline_number].pb_vd_inputpin_id = p_connection->connection_id;

                        pb_pipelines->p_pb_pipeline_head[pb_pipelines->pb_pipeline_number].mapped_dsp_dec_instance_id = DCOMPONENT_INDEX_FROM_GENERIC_ID(pb_pipelines->p_pb_pipeline_head[pb_pipelines->pb_pipeline_number].pb_video_decoder_id);
                        pb_pipelines->p_pb_pipeline_head[pb_pipelines->pb_pipeline_number].occupied_dsp_dec_instance = 1;
#if 0
                        if (mpUniqueVideoPostProcessorComponent) {
                            pb_pipelines->p_pb_pipeline_head[pb_pipelines->pb_pipeline_number].pb_video_postp_id = mpUniqueVideoPostProcessorComponent->id;
                        }
#endif

                        if (mpUniqueVideoRendererComponent) {
                            pb_pipelines->p_pb_pipeline_head[pb_pipelines->pb_pipeline_number].pb_video_renderer_id = mpUniqueVideoRendererComponent->id;
                        }

                        if (has_audio) {
                            pb_pipelines->p_pb_pipeline_head[pb_pipelines->pb_pipeline_number].pb_audio_decoder_id = first_audio_id;
                            pb_pipelines->p_pb_pipeline_head[pb_pipelines->pb_pipeline_number].pb_ad_inputpin_id = first_audio_pin_id;

                            pb_pipelines->p_pb_pipeline_head[pb_pipelines->pb_pipeline_number].mapped_audio_dec_instance_id = DCOMPONENT_INDEX_FROM_GENERIC_ID(pb_pipelines->p_pb_pipeline_head[pb_pipelines->pb_pipeline_number].pb_audio_decoder_id);

                            if (mpUniqueAudioPostProcessorComponent) {
                                pb_pipelines->p_pb_pipeline_head[pb_pipelines->pb_pipeline_number].pb_audio_postp_id = mpUniqueAudioPostProcessorComponent->id;
                            }

                            if (mpUniqueAudioRendererComponent) {
                                pb_pipelines->p_pb_pipeline_head[pb_pipelines->pb_pipeline_number].pb_audio_renderer_id = mpUniqueAudioRendererComponent->id;
                            }
                        }

                        pb_pipelines->pb_pipeline_number ++;
                        mnPlaybackPipelinesNumber ++;
                    } else if (EMaxSinkNumber_Demuxer == pb_pipelines->pb_pipeline_number) {
                        LOGM_ERROR("only support %d playback instance from same source\n", EMaxSinkNumber_Demuxer);
                    } else {
                        LOGM_ERROR("!!! Internal error(pb_pipelines->pb_pipeline_number %d), please check code.\n", pb_pipelines->pb_pipeline_number);
                    }
                }

            } else {
                DASSERT("NULL pointer here, something would be wrong.\n");
            }
            pnode = mListConnections.NextNode(pnode);
        }

    } else {
        LOGM_ERROR("NULL p_component!\n");
        return;
    }

}

void CActiveGenericEngine::setupInternalRecordingPipelineInfos(SComponent_Obsolete *p_component)
{
    CIDoubleLinkedList::SNode *pnode = mListFilters.FirstNode();
    SConnection_Obsolete *p_connection = NULL;

    SGenericPipeline *rec_pipeline = NULL;

    LOGM_INFO("setupInternalRecordingPipelineInfos start\n");

    //debug assert
    DASSERT(p_component);

    if (p_component) {

        if (EGenericComponentType_Muxer != p_component->type) {
            LOGM_ERROR("recording engine should end to a sink component(%d)\n", p_component->type);
            return;
        }

        //find pipeline
        rec_pipeline = getRecordingPipeline(p_component->index);
        if (!rec_pipeline) {
            LOGM_FATAL("ret NULL, getRecordingPipeline(%d)\n", p_component->index);
            return;
        }

        LOGM_INFO("setupInternalRecordingPipelineInfos, getRecordingPipeline return %p, pipeline_id 0x%08x\n", rec_pipeline, rec_pipeline->pipeline_id);

        if (EGenericPipelineState_not_inited == rec_pipeline->pipeline_state) {
            DASSERT(EGenericPipelineType_Recording == rec_pipeline->pipeline_type);

            rec_pipeline->source_sink_index = p_component->index;
            rec_pipeline->rec_sink_id = p_component->id;
            rec_pipeline->rec_source_number = 0;
        } else if (EGenericPipelineState_build_gragh != rec_pipeline->pipeline_state) {
            LOGM_ERROR("BAD pipeline state %d\n", rec_pipeline->pipeline_state);
            return;
        } else {
            LOGM_ERROR("why comes here? rec_pipeline->pipeline_state %d\n", rec_pipeline->pipeline_state);
        }

        //find each video path
        pnode = mListConnections.FirstNode();
        while (pnode) {
            p_connection = (SConnection_Obsolete *)(pnode->p_context);
            DASSERT(p_connection);
            if ((p_connection) && (p_connection->down_id == p_component->id)) {
                DASSERT(rec_pipeline->rec_source_number <= DGENERIC_ENGINE_MAX_SOURCE_TO_SAME_MUXER);
                if (rec_pipeline->rec_source_number < DGENERIC_ENGINE_MAX_SOURCE_TO_SAME_MUXER) {
                    rec_pipeline->rec_source_id[rec_pipeline->rec_source_number] = p_connection->up->id;
                    rec_pipeline->rec_source_pin_id[rec_pipeline->rec_source_number] = p_connection->connection_id;
                    rec_pipeline->rec_source_number ++;
                } else if (DGENERIC_ENGINE_MAX_SOURCE_TO_SAME_MUXER == rec_pipeline->rec_source_number) {
                    LOGM_ERROR("exceed max source for muxer\n");
                } else {
                    LOGM_ERROR("Internal error!!! please check code\n");;
                }
            } else {
                DASSERT("NULL pointer here, something would be wrong.\n");
            }
            pnode = mListConnections.NextNode(pnode);
        }
    } else {
        LOGM_ERROR("NULL p_component!\n");
        return;
    }
}

void CActiveGenericEngine::setupInternalRTSPStreamingPipelineInfos(SComponent_Obsolete *p_component)
{
    SConnection_Obsolete *p_connection = NULL;

    SGenericPipeline *streaming_pipeline = NULL;
    TGenericID transmiter_id = 0;

    //LOGM_DEBUG("setupInternalRTSPStreamingPipelineInfos a EGenericComponentType_StreamingContent(%d)\n", p_component->type);

    //debug assert
    DASSERT(p_component);

    if (p_component) {

        if (EGenericComponentType_StreamingContent != p_component->type) {
            LOGM_ERROR("rtsp streaming pipeline should be a EGenericComponentType_StreamingContent(%d)\n", p_component->type);
            return;
        }

        //find pipeline
        streaming_pipeline = getStreamingPipeline(p_component->id);
        DASSERT(streaming_pipeline);
        if (!streaming_pipeline) {
            LOGM_FATAL("getStreamingPipeline fail\n");
            return;
        }

        if (EGenericPipelineState_not_inited == streaming_pipeline->pipeline_state) {
            DASSERT(EGenericPipelineType_Streaming == streaming_pipeline->pipeline_type);

            streaming_pipeline->source_sink_index = p_component->index;
            streaming_pipeline->rtsp_content_id = p_component->id;
            streaming_pipeline->rtsp_streaming_source_number = 2;
            streaming_pipeline->rtsp_source_id[0] = p_component->streaming_video_source;
            streaming_pipeline->rtsp_source_id[1] = p_component->streaming_audio_source;

            streaming_pipeline->rtsp_data_transmiter_id = p_component->streaming_transmiter_id;
            streaming_pipeline->rtsp_server_id = p_component->streaming_server_id;
        } else if (EGenericPipelineState_build_gragh != streaming_pipeline->pipeline_state) {
            LOGM_ERROR("BAD pipeline state %d\n", streaming_pipeline->pipeline_state);
            return;
        }

        transmiter_id = streaming_pipeline->rtsp_data_transmiter_id;
        CIDoubleLinkedList::SNode *pnode_tmp = NULL;
        p_connection = queryConnection(streaming_pipeline->rtsp_source_id[0], transmiter_id, pnode_tmp, StreamType_Video);
        if (p_connection) {
            streaming_pipeline->rtsp_source_pin_id[0] = p_connection->connection_id;
        }
        pnode_tmp = NULL;
        p_connection = queryConnection(streaming_pipeline->rtsp_source_id[1], transmiter_id, pnode_tmp, StreamType_Audio);
        if (p_connection) {
            streaming_pipeline->rtsp_source_pin_id[1] = p_connection->connection_id;
        }

    } else {
        LOGM_ERROR("NULL p_component!\n");
        return;
    }
}

void CActiveGenericEngine::setupInternalPipelineInfos()
{
    CIDoubleLinkedList::SNode *pnode = mListFilters.FirstNode();
    SComponent_Obsolete *p_component = NULL;

    LOGM_INFO("CActiveGenericEngine::setupInternalPipelineInfos start.\n");

    if (0 != mbPipelineInfoGet) {
        LOGM_ERROR("already get pipeline infos\n");
        return;
    }

    while (pnode) {
        p_component = (SComponent_Obsolete *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            LOGM_INFO("setupInternalPipelineInfos, type %d, index %d\n", p_component->type, p_component->index);
            switch (p_component->type) {
                case EGenericComponentType_Demuxer:
                    if (ENoPlaybackInstance != mPresetPlaybackMode) {
                        setupInternalPlaybackPipelineInfos(p_component);
                    }
                    break;
                case EGenericComponentType_VideoDecoder:
                    break;
                case EGenericComponentType_VideoEncoder:
                    break;
                case EGenericComponentType_AudioDecoder:
                    break;
                case EGenericComponentType_Muxer:
                    setupInternalRecordingPipelineInfos(p_component);
                    break;
                case EGenericComponentType_VideoPostProcessor:
                    break;
                case EGenericComponentType_AudioPostProcessor:
                    break;
                case EGenericComponentType_VideoRenderer:
                    break;
                case EGenericComponentType_AudioRenderer:
                    break;
                case EGenericComponentType_StreamingServer:
                    break;
                case EGenericComponentType_StreamingTransmiter:
                    break;
                case EGenericComponentType_ConnecterPinMuxer:
                    break;
                default:
                    LOGM_ERROR("unknown type %d\n", p_component->type);
                    return;
            }
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mListFilters.NextNode(pnode);
    }

    pnode = mListProxys.FirstNode();
    while (pnode) {
        p_component = (SComponent_Obsolete *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            LOGM_INFO("setupInternalPipelineInfos, type %d, index %d\n", p_component->type, p_component->index);
            switch (p_component->type) {
                case EGenericComponentType_StreamingServer:
                    break;
                case EGenericComponentType_StreamingContent:
                    setupInternalRTSPStreamingPipelineInfos(p_component);
                    break;
                case EGenericComponentType_CloudServer:
                    break;
                case EGenericComponentType_CloudReceiverContent:
                    break;
                default:
                    LOGM_ERROR("unknown type %d\n", p_component->type);
                    break;
            }
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mListProxys.NextNode(pnode);
    }

    mbPipelineInfoGet = 1;
    return;
}

void CActiveGenericEngine::mappingPlaybackPipeline2Component()
{
    TUint i = 0;
    SPlaybackPipelines *p = mpPlaybackPipelines;
    SComponent_Obsolete *p_component;
    TGenericID id;

    if (!p) {
        LOGM_ERROR("No playback engines!!\n");
        return;
    }

    while (p) {
        for (i = 0; i < p->pb_pipeline_number; i ++) {

            //source filter
            id = p->p_pb_pipeline_head[i].pb_source_id;

            if (IS_VALID_COMPONENT_ID(id)) {
                p_component = queryFilter(id);

                if (p_component) {
                    DASSERT(EGenericComponentType_Demuxer == p_component->type);

                    if (!p_component->p_scur_pipeline) {
                        p_component->p_scur_pipeline = &p->p_pb_pipeline_head[i];
                    }

                    if (!p_component->belong2playback_pipeline) {
                        p_component->belong2playback_pipeline = 1;
                    }
                    p_component->number_of_playback_pipeline ++;

                    p_component->pipelines.InsertContent(NULL, (void *) &p->p_pb_pipeline_head[i], 0);
                } else {
                    LOGM_FATAL("NULL component, id 0x%08x\n", id);
                }
            } else {
                LOGM_WARN("no demuxer(id 0x%08x) in playback pipeline 0x%08x\n", id, p->p_pb_pipeline_head[i].pipeline_id);
            }

            //video decoder filter
            id = p->p_pb_pipeline_head[i].pb_video_decoder_id;
            if (IS_VALID_COMPONENT_ID(id)) {
                p_component = queryFilter(id);
                if (p_component) {
                    DASSERT(EGenericComponentType_VideoDecoder == p_component->type);

                    if (!p_component->p_scur_pipeline) {
                        p_component->p_scur_pipeline = &p->p_pb_pipeline_head[i];
                    }

                    if (!p_component->belong2playback_pipeline) {
                        p_component->belong2playback_pipeline = 1;
                        p->p_pb_pipeline_head[i].occupied_dsp_dec_instance = 1;
                    }
                    p_component->number_of_playback_pipeline ++;

                    p_component->pipelines.InsertContent(NULL, (void *) &p->p_pb_pipeline_head[i], 0);
                } else {
                    LOGM_FATAL("NULL component, id 0x%08x\n", id);
                }
            } else {
                LOGM_INFO("no video decoder(id 0x%08x) in playback pipeline 0x%08x\n", id, p->p_pb_pipeline_head[i].pipeline_id);
            }

            //audio decoder filter
            id = p->p_pb_pipeline_head[i].pb_audio_decoder_id;
            if (IS_VALID_COMPONENT_ID(id)) {
                p_component = queryFilter(id);

                if (p_component) {
                    DASSERT(EGenericComponentType_AudioDecoder == p_component->type);

                    if (!p_component->p_scur_pipeline) {
                        p_component->p_scur_pipeline = &p->p_pb_pipeline_head[i];
                    }

                    if (!p_component->belong2playback_pipeline) {
                        p_component->belong2playback_pipeline = 1;
                        p->p_pb_pipeline_head[i].occupied_audio_dec_instance = 1;
                    }
                    p_component->number_of_playback_pipeline ++;

                    p_component->pipelines.InsertContent(NULL, (void *) &p->p_pb_pipeline_head[i], 0);
                } else {
                    LOGM_FATAL("NULL component, id 0x%08x\n", id);
                }
            } else {
                LOGM_INFO("no audio decoder(id 0x%08x) in playback pipeline 0x%08x\n", id, p->p_pb_pipeline_head[i].pipeline_id);
            }

#if 0
            //video post-processor filter
            id = p->p_pb_pipeline_head[i].pb_video_postp_id;
            if (IS_VALID_COMPONENT_ID(id)) {
                p_component = queryFilter(id);

                if (p_component) {
                    DASSERT(EGenericComponentType_VideoPostProcessor == p_component->type);

                    if (!p_component->p_scur_pipeline) {
                        p_component->p_scur_pipeline = &p->p_pb_pipeline_head[i];
                    }
                    p_component->pipelines.InsertContent(NULL, (void *) &p->p_pb_pipeline_head[i], 0);
                } else {
                    LOGM_FATAL("NULL component, id 0x%08x\n", id);
                }
            } else {
                LOGM_WARN("no video post-processor(id 0x%08x) in playback pipeline 0x%08x\n", id, p->p_pb_pipeline_head[i].pipeline_id);
            }

            //audio post-processor filter
            id = p->p_pb_pipeline_head[i].pb_audio_postp_id;
            if (IS_VALID_COMPONENT_ID(id)) {
                p_component = queryFilter(id);

                if (p_component) {
                    DASSERT(EGenericComponentType_VideoPostProcessor == p_component->type);

                    if (!p_component->p_scur_pipeline) {
                        p_component->p_scur_pipeline = &p->p_pb_pipeline_head[i];
                    }
                    p_component->pipelines.InsertContent(NULL, (void *) &p->p_pb_pipeline_head[i], 0);
                } else {
                    LOGM_FATAL("NULL component, id 0x%08x\n", id);
                }
            } else {
                LOGM_INFO("no audio post-processor(id 0x%08x) in playback pipeline 0x%08x\n", id, p->p_pb_pipeline_head[i].pipeline_id);
            }
#endif

            //video renderer filter
            id = p->p_pb_pipeline_head[i].pb_video_renderer_id;
            if (IS_VALID_COMPONENT_ID(id)) {
                p_component = queryFilter(id);

                if (p_component) {
                    DASSERT(EGenericComponentType_VideoRenderer == p_component->type);

                    if (!p_component->p_scur_pipeline) {
                        p_component->p_scur_pipeline = &p->p_pb_pipeline_head[i];
                    }

                    if (!p_component->belong2playback_pipeline) {
                        p_component->belong2playback_pipeline = 1;
                    }
                    p_component->number_of_playback_pipeline ++;

                    p_component->pipelines.InsertContent(NULL, (void *) &p->p_pb_pipeline_head[i], 0);
                } else {
                    LOGM_FATAL("NULL component, id 0x%08x\n", id);
                }
            } else {
                LOGM_INFO("no video renderer(id 0x%08x) in playback pipeline 0x%08x\n", id, p->p_pb_pipeline_head[i].pipeline_id);
            }

            //audio renderer filter
            id = p->p_pb_pipeline_head[i].pb_audio_renderer_id;
            if (IS_VALID_COMPONENT_ID(id)) {
                p_component = queryFilter(id);

                if (p_component) {
                    DASSERT(EGenericComponentType_AudioRenderer == p_component->type);

                    if (!p_component->p_scur_pipeline) {
                        p_component->p_scur_pipeline = &p->p_pb_pipeline_head[i];
                    }

                    if (!p_component->belong2playback_pipeline) {
                        p_component->belong2playback_pipeline = 1;
                    }
                    p_component->number_of_playback_pipeline ++;

                    p_component->pipelines.InsertContent(NULL, (void *) &p->p_pb_pipeline_head[i], 0);
                } else {
                    LOGM_FATAL("NULL component, id 0x%08x\n", id);
                }
            } else {
                LOGM_INFO("no audio renderer(id 0x%08x) in playback pipeline 0x%08x\n", id, p->p_pb_pipeline_head[i].pipeline_id);
            }

        }
        p = p->p_next;
    }

}

void CActiveGenericEngine::mappingRecordingPipeline2Component()
{
    TUint i = 0, ii = 0;
    TGenericID id;
    SGenericPipeline *p;
    SComponent_Obsolete *p_component;

    DASSERT(mRecordingPipelines.rec_pipeline_number <= EPipelineMaxNumber_Recording);

    LOGM_INFO("mappingRecordingPipeline2Component start.\n");

    while (i < mRecordingPipelines.rec_pipeline_number) {
        p = &mRecordingPipelines.p_rec_pipeline_head[i];

        //sink filter
        id = p->rec_sink_id;
        if (IS_VALID_COMPONENT_ID(id)) {
            p_component = queryFilter(id);

            if (p_component) {
                DASSERT(EGenericComponentType_Muxer == p_component->type);

                if (!p_component->p_scur_pipeline) {
                    p_component->p_scur_pipeline = p;
                }

                if (!p_component->belong2recording_pipeline) {
                    p_component->belong2recording_pipeline = 1;
                }
                p_component->number_of_recording_pipeline ++;

                p_component->pipelines.InsertContent(NULL, (void *) p, 0);
            } else {
                LOGM_FATAL("NULL component, id 0x%08x\n", id);
            }
        } else {
            LOGM_ERROR("no sink filter(id 0x%08x) in recording pipeline 0x%08x?\n", id, p->pipeline_id);
        }

        //source filters
        for (ii = 0; ii < p->rec_source_number; ii ++) {
            id = p->rec_source_id[ii];

            if (IS_VALID_COMPONENT_ID(id)) {
                p_component = queryFilter(id);

                if (p_component) {
                    if (!p_component->p_scur_pipeline) {
                        p_component->p_scur_pipeline = p;
                    }

                    if (!p_component->belong2recording_pipeline) {
                        p_component->belong2recording_pipeline = 1;
                    }
                    p_component->number_of_recording_pipeline ++;

                    p_component->pipelines.InsertContent(NULL, (void *) p, 0);
                } else {
                    LOGM_FATAL("NULL component, id 0x%08x\n", id);
                }
            } else {
                LOGM_ERROR("BAD id in recording pipeline 0x%08x, rec_source_id[%d] = (id 0x%08x)?\n", p->pipeline_id, ii, id);
            }
        }

        i ++;
    }

    LOGM_INFO("mappingRecordingPipeline2Component end.\n");

}

void CActiveGenericEngine::mappingRTSPStreamingPipeline2Component()
{
    TUint i = 0, ii = 0;
    TGenericID id;
    SGenericPipeline *p;
    SComponent_Obsolete *p_component;

    DASSERT(mRTSPStreamingPipelines.streaming_pipeline_number <= EPipelineMaxNumber_Streaming);

    while (i < mRTSPStreamingPipelines.streaming_pipeline_number) {
        p = &mRTSPStreamingPipelines.p_streaming_pipeline_head[i];

        //sink filter
        id = p->rtsp_data_transmiter_id;
        if (IS_VALID_COMPONENT_ID(id)) {
            p_component = queryFilter(id);

            if (p_component) {
                DASSERT(EGenericComponentType_StreamingTransmiter == p_component->type);

                if (!p_component->p_scur_pipeline) {
                    p_component->p_scur_pipeline = p;
                }

                if (!p_component->belong2streaming_pipeline) {
                    p_component->belong2streaming_pipeline = 1;
                }
                p_component->number_of_streaming_pipeline ++;

                p_component->pipelines.InsertContent(NULL, (void *) p, 0);
            } else {
                LOGM_FATAL("NULL component, id 0x%08x\n", id);
            }
        } else {
            LOGM_ERROR("no RTSP data transmiter(0x%08x) filter in RTSP streaming pipeline 0x%08x?\n", id, p->pipeline_id);
        }

        //source filters
        for (ii = 0; ii < p->rec_source_number; ii ++) {
            id = p->rtsp_source_id[ii];

            if (IS_VALID_COMPONENT_ID(id)) {
                p_component = queryFilter(id);

                if (p_component) {
                    if (!p_component->p_scur_pipeline) {
                        p_component->p_scur_pipeline = p;
                    }

                    if (!p_component->belong2streaming_pipeline) {
                        p_component->belong2streaming_pipeline = 1;
                    }
                    p_component->number_of_streaming_pipeline ++;

                    p_component->pipelines.InsertContent(NULL, (void *) p, 0);
                } else {
                    LOGM_FATAL("NULL component, id 0x%08x\n", id);
                }
            } else {
                LOGM_ERROR("BAD id(0x%08x) in RTSP streaming pipeline 0x%08x, rtsp_source_id[%d]?\n", id, p->pipeline_id, ii);
            }
        }

        i ++;
    }

}

EECode CActiveGenericEngine::FinalizeConfigProcessPipeline()
{
    EECode err;

    DASSERT(false == mbGraghBuilt);

    if (false == mbGraghBuilt) {

        DASSERT(EEngineState_not_alive == msState);
        componentNumberStatistics();

        if ((EMediaDeviceWorkingMode_NVR_RTSP_Streaming  != mRequestWorkingMode) && (ENoPlaybackInstance != mPresetPlaybackMode)) {
            //device related
            if (EMediaDeviceWorkingMode_Invalid == mRequestWorkingMode) {
                //auto select dsp mode
                err = autoSelectDSPMode();
                DASSERT(EECode_OK == err);
                LOGM_CONFIGURATION("DSPMode is not set, auto select %d, %s\n", mRequestDSPMode, gfGetDSPOperationModeString(mRequestDSPMode));
            } else {
                err = selectDSPMode();
                DASSERT(EECode_OK == err);
                LOGM_CONFIGURATION("select DSPMode %d, %s, request mode %d, %s\n", mRequestDSPMode, gfGetDSPOperationModeString(mRequestDSPMode), mRequestWorkingMode, gfGetMediaDeviceWorkingModeString(mRequestWorkingMode));
            }

            if (EPlaybackMode_Invalid == mPresetPlaybackMode) {
                if (EMediaDeviceWorkingMode_SingleInstancePlayback == mRequestWorkingMode) {
                    mPresetPlaybackMode = EPlaybackMode_1x1080p;
                } else if (EMediaDeviceWorkingMode_MultipleInstancePlayback == mRequestWorkingMode) {
                    mPresetPlaybackMode = EPlaybackMode_1x1080p_4xD1;
                } else {
                    LOGM_FATAL("TO DO\n");
                }
                LOGM_WARN("guess mode, mPresetPlaybackMode %d, %s\n", mPresetPlaybackMode, gfGetPlaybackModeString(mPresetPlaybackMode));
            }

            LOGM_PRINTF("before presetPlaybackModeForDSP, mode %d, %s\n", mPresetPlaybackMode, gfGetPlaybackModeString(mPresetPlaybackMode));

            if (EPlaybackMode_1x1080p_4xD1 == mPresetPlaybackMode) {
                presetPlaybackModeForDSP(4, 1, 720, 480, 1920, 1080);
            } else if (EPlaybackMode_1x1080p_6xD1 == mPresetPlaybackMode) {
                presetPlaybackModeForDSP(6, 1, 720, 480, 1920, 1080);
            } else if (EPlaybackMode_1x1080p_8xD1 == mPresetPlaybackMode) {
                presetPlaybackModeForDSP(8, 1, 720, 480, 1920, 1080);
            } else if (EPlaybackMode_1x1080p_9xD1 == mPresetPlaybackMode) {
                presetPlaybackModeForDSP(9, 1, 720, 480, 1920, 1080);
            } else if (EPlaybackMode_1x1080p == mPresetPlaybackMode) {
                presetPlaybackModeForDSP(0, 1, 0, 0, 1920, 1080);
            } else if (EPlaybackMode_4x720p == mPresetPlaybackMode) {
                presetPlaybackModeForDSP(4, 0, 1280, 720, 0, 0);
            } else if (EPlaybackMode_4xD1 == mPresetPlaybackMode) {
                presetPlaybackModeForDSP(4, 0, 720, 480, 0, 0);
            } else if (EPlaybackMode_6xD1 == mPresetPlaybackMode) {
                presetPlaybackModeForDSP(6, 0, 720, 480, 0, 0);
            } else if (EPlaybackMode_8xD1 == mPresetPlaybackMode) {
                presetPlaybackModeForDSP(8, 0, 720, 480, 0, 0);
            } else if (EPlaybackMode_9xD1 == mPresetPlaybackMode) {
                presetPlaybackModeForDSP(9, 0, 720, 480, 0, 0);
            } else if (EPlaybackMode_1x3M_4xD1 == mPresetPlaybackMode) {
                presetPlaybackModeForDSP(4, 1, 720, 480, 2048, 1536);
            } else if (EPlaybackMode_1x3M_6xD1 == mPresetPlaybackMode) {
                presetPlaybackModeForDSP(6, 1, 720, 480, 2048, 1536);
            } else if (EPlaybackMode_1x3M_8xD1 == mPresetPlaybackMode) {
                presetPlaybackModeForDSP(8, 1, 720, 480, 2048, 1536);
            } else if (EPlaybackMode_1x3M_9xD1 == mPresetPlaybackMode) {
                presetPlaybackModeForDSP(9, 1, 720, 480, 2048, 1536);
            } else if (EPlaybackMode_1x3M == mPresetPlaybackMode) {
                presetPlaybackModeForDSP(0, 1, 0, 0, 2048, 1536);
            } else if (EPlaybackMode_AutoDetect != mPresetPlaybackMode) {
                LOGM_FATAL("BAD mPresetPlaybackMode %d\n", mPresetPlaybackMode);
                return EECode_InternalLogicalBug;
            }

            mbDisplayAllInstance = 0;
            mbMultipleWindowView = 1;
            mbSingleWindowView = 0;
            mSingleViewDecIndex = 0;

            //prepare things here
            DASSERT(!mpDSPAPI);
            DASSERT((-1) == mIavFd);

            if (!mpDSPAPI) {
                mpDSPAPI = gfDSPAPIFactory((const SPersistMediaConfig *)&mPersistMediaConfig);
            }

            if (!mpDSPAPI) {
                LOGM_ERROR("CreateSimpleDSPAPI() fail\n");
                return EECode_Error;
            }

            err = mpDSPAPI->OpenDevice(mIavFd);
            DASSERT_OK(err);
            DASSERT(mIavFd >= 0);

            err = mpDSPAPI->QueryVoutSettings(&mPersistMediaConfig.dsp_config);
            DASSERT_OK(err);

            summarizeVoutParams();

            mPersistMediaConfig.dsp_config.device_fd = mIavFd;
            mPersistMediaConfig.dsp_config.p_dsp_handler = (void *)mpDSPAPI;
            mPersistMediaConfig.dsp_config.dsp_type = mpDSPAPI->QueryDSPPlatform();

            LOGM_PRINTF("get platform 0x%04hx, handle_id %d, %s\n", mPersistMediaConfig.dsp_config.dsp_type, mPersistMediaConfig.dsp_config.device_fd, gfGetDSPPlatformString(mpDSPAPI->QueryDSPPlatform()));

            mPersistMediaConfig.dsp_config.modeConfig.pp_max_frm_width = mMaxVoutWidth;
            mPersistMediaConfig.dsp_config.modeConfig.pp_max_frm_height = mMaxVoutHeight;

            //activate device to working mode
            err = setupDSPConfig(mRequestDSPMode);
            DASSERT_OK(err);
            if (EECode_OK != err) {
                LOGM_ERROR("setupDSPConfig(%hu, %s) fail, ret %d, %s\n", mRequestDSPMode, gfGetDSPOperationModeString(mRequestDSPMode), err, gfGetErrorCodeString(err));
                return err;
            }

            err = activateWorkingMode(mRequestDSPMode);
            DASSERT_OK(err);
            if (EECode_OK != err) {
                LOGM_ERROR("activateWorkingMode(%hu, %s) fail, ret %d, %s\n", mRequestDSPMode, gfGetDSPOperationModeString(mRequestDSPMode), err, gfGetErrorCodeString(err));
                return err;
            }
            mPersistMediaConfig.dsp_config.request_dsp_mode = mRequestDSPMode;

        }

        err = createSchedulers();
        if (EECode_OK != err) {
            LOGM_ERROR("createSchedulers() fail, err %d\n", err);
            return err;
        }

        err = createGraph();
        if (EECode_OK != err) {
            LOGM_ERROR("createGraph() fail, err %d\n", err);
            return err;
        }

        setupInternalPipelineInfos();

        if (ENoPlaybackInstance != mPresetPlaybackMode) {
            mappingPlaybackPipeline2Component();
        }

        mappingRecordingPipeline2Component();
        mappingRTSPStreamingPipeline2Component();

        msState = EEngineState_idle;
    }

    msState = EEngineState_idle;
    return EECode_OK;
}

EECode CActiveGenericEngine::SetDataSource(TGenericID component_id, TChar *url)
{
    SComponent_Obsolete *component = NULL;
    TUint size = 0;

    if (!url) {
        LOGM_WARN("NULL pointer in SetDataSource\n");
    } else {
        size = strlen(url);
        DASSERT(size < 1024);
        if (size >= 1024) {
            LOGM_ERROR("url too long(size %d) in SetDataSource, not support\n", size);
            return EECode_BadParam;
        }
    }

    component = findComponentFromID(component_id);

    if (component) {
        if (url) {
            if (component->url) {
                LOGM_ERROR("already set url? %s\n", component->url);
                DDBG_FREE(component->url, "E0UR");
                component->url = NULL;
            }

            component->url = (char *)DDBG_MALLOC(size + 4, "E0UR");
            if (!component->url) {
                LOGM_ERROR("NO memory\n");
                return EECode_BadParam;
            }
            strcpy(component->url, url);
        } else {
            LOGM_ERROR("NO memory\n");
            return EECode_BadParam;
        }

        if (mbEnginePipelineRunning) {
            LOGM_NOTICE("SetDataSource(%s) runtime, demuxer id 0x%08x\n", url, component_id);
            if (DLikely(component->p_filter)) {
                component->p_filter->FilterProperty(EFilterPropertyType_update_source_url, 1, url);
            } else {
                LOGM_ERROR("NULL component->p_filter\n");
            }
        }
    } else {
        LOGM_FATAL("BAD component id 0x%08x\n", component_id);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::SetSinkDst(TGenericID component_id, TChar *file_dst)
{
    SComponent_Obsolete *component = NULL;
    TUint size = 0;

    if (!file_dst) {
        LOGM_WARN("NULL pointer in SetSinkDst\n");
    } else {
        size = strlen(file_dst);
        DASSERT(size < 1024);
        if (size >= 1024) {
            LOGM_ERROR("url too long(size %d) in SetSinkDst, not support\n", size);
            return EECode_BadParam;
        }
    }

    component = findComponentFromID(component_id);

    if (component) {
        if (file_dst) {
            DASSERT(NULL == component->url);
            if (component->url) {
                LOGM_ERROR("already set url? %s\n", component->url);
                DDBG_FREE(component->url, "E0UR");
                component->url = NULL;
            }

            component->url = (TChar *)DDBG_MALLOC(size + 4, "E0UR");
            if (!component->url) {
                LOGM_ERROR("NO memory\n");
                return EECode_BadParam;
            }
            strcpy(component->url, file_dst);
        }

        if (mbEnginePipelineRunning) {
            LOGM_NOTICE("SetSinkDst(%s) runtime, muxer id 0x%08x\n", file_dst, component_id);
            if (DLikely(component->p_filter)) {
                component->p_filter->FilterProperty(EFilterPropertyType_update_sink_url, 1, file_dst);
            } else {
                LOGM_ERROR("NULL component->p_filter\n");
            }
        } else {
            LOGM_NOTICE("SetSinkDst, mbEnginePipelineRunning=%u, not do EFilterPropertyType_update_sink_url yet.\n", mbEnginePipelineRunning);
        }

    } else {
        LOGM_FATAL("BAD component id 0x%08x\n", component_id);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::SetupStreamingContent(TGenericID content_id, TGenericID server_id, TGenericID transmiter_id, TGenericID video_source_component_id, TGenericID audio_source_component_id, TChar *streaming_tag)
{
    SComponent_Obsolete *component = NULL;
    TUint size = 0;

    if (streaming_tag) {
        LOGM_NOTICE("SetupStreamingContent(content id %08x, server_id %08x, transmiter_id %08x, video source id %08x, audio source id %08x, tag %s)\n", content_id, server_id, transmiter_id, video_source_component_id, audio_source_component_id, streaming_tag);
    } else {
        LOGM_NOTICE("SetupStreamingContent(content id %08x, server_id %08x, transmiter_id %08x, video source id %08x, audio source id %08x)\n", content_id, server_id, transmiter_id, video_source_component_id, audio_source_component_id);
    }

    //debug assert
    DASSERT(IS_VALID_COMPONENT_ID(content_id));
    DASSERT(IS_VALID_COMPONENT_ID(server_id));
    DASSERT(IS_VALID_COMPONENT_ID(transmiter_id));
    DASSERT(EGenericComponentType_StreamingContent == DCOMPONENT_TYPE_FROM_GENERIC_ID(content_id));
    DASSERT(EGenericComponentType_StreamingServer == DCOMPONENT_TYPE_FROM_GENERIC_ID(server_id));
    DASSERT(EGenericComponentType_StreamingTransmiter == DCOMPONENT_TYPE_FROM_GENERIC_ID(transmiter_id));
    DASSERT(mnComponentProxysNumbers[EGenericComponentType_StreamingContent - EGenericComponentType_TAG_proxy_start] > DCOMPONENT_INDEX_FROM_GENERIC_ID(content_id));
    DASSERT(mnComponentProxysNumbers[EGenericComponentType_StreamingServer - EGenericComponentType_TAG_proxy_start] > DCOMPONENT_INDEX_FROM_GENERIC_ID(server_id));
    DASSERT(mnComponentFiltersNumbers[EGenericComponentType_StreamingTransmiter] > DCOMPONENT_INDEX_FROM_GENERIC_ID(transmiter_id));

    if (!streaming_tag) {
        LOGM_NOTICE("NULL streaming_tag in SetupStreamingContent\n");
    } else {
        size = strlen(streaming_tag);
        DASSERT(size < 1024);
        if (size >= 1024) {
            LOGM_ERROR("url too long(size %d) in SetupStreamingContent, not support\n", size);
            return EECode_BadParam;
        }
    }

    component = findComponentFromID(content_id);

    if (component) {

        if (streaming_tag) {
            DASSERT(NULL == component->url);
            if (component->url) {
                LOGM_ERROR("already set url? %s\n", component->url);
                DDBG_FREE(component->url, "E0UR");
                component->url = NULL;
            }

            component->url = (TChar *)DDBG_MALLOC(size + 4, "E0UR");
            if (!component->url) {
                LOGM_ERROR("NO memory\n");
                return EECode_BadParam;
            }
            strcpy(component->url, streaming_tag);
        } else {
            TInt index = DCOMPONENT_INDEX_FROM_GENERIC_ID(content_id);
            component->url = (TChar *)DDBG_MALLOC(32, "E0UR");
            memset(component->url, 0x0, 32);
            sprintf(component->url, "%d_0", index + 42);
        }

    } else {
        LOGM_ERROR("BAD content id 0x%08x\n", content_id);
        return EECode_BadParam;
    }

    component->streaming_video_source = video_source_component_id;
    component->streaming_audio_source = audio_source_component_id;

    component->streaming_server_id = server_id;
    component->streaming_transmiter_id = transmiter_id;
    return EECode_OK;
}

EECode CActiveGenericEngine::SetStreamingUrl(TGenericID content_id, TChar *streaming_tag)
{
    SComponent_Obsolete *component = NULL;
    TUint size = 0;

    if (streaming_tag) {
        LOGM_NOTICE("SetStreamingUrl(content id %08x, streaming_tag %s)\n", content_id, streaming_tag);
    } else {
        LOGM_NOTICE("SetStreamingUrl(content id %08x)\n", content_id);
    }

    //debug assert
    DASSERT(IS_VALID_COMPONENT_ID(content_id));
    DASSERT(EGenericComponentType_StreamingContent == DCOMPONENT_TYPE_FROM_GENERIC_ID(content_id));
    DASSERT(mnComponentProxysNumbers[EGenericComponentType_StreamingContent - EGenericComponentType_TAG_proxy_start] > DCOMPONENT_INDEX_FROM_GENERIC_ID(content_id));

    if (!streaming_tag) {
        LOGM_NOTICE("NULL pointer in SetStreamingTag\n");
    } else {
        size = strlen(streaming_tag);
        DASSERT(size < 1024);
        if (size >= 1024) {
            LOGM_ERROR("url too long(size %d) in SetRTSPStreamingTag, not support\n", size);
            return EECode_BadParam;
        }
    }

    component = findComponentFromID(content_id);

    if (component) {

        if (streaming_tag) {
            DASSERT(NULL == component->url);
            if (component->url) {
                LOGM_ERROR("already set url? %s\n", component->url);
                DDBG_FREE(component->url, "E0UR");
                component->url = NULL;
            }

            component->url = (TChar *)DDBG_MALLOC(size + 4, "E0UR");
            if (!component->url) {
                LOGM_ERROR("NO memory\n");
                return EECode_BadParam;
            }
            strcpy(component->url, streaming_tag);
        }

        if (mbEnginePipelineRunning) {
            SStreamingSessionContent *p_content = NULL;
            TComponentIndex content_index = DCOMPONENT_INDEX_FROM_GENERIC_ID(content_id);

            LOGM_NOTICE("SetStreamingUrl(%s) runtime, content id 0x%08x\n", streaming_tag, content_id);
            if (DUnlikely(!mpStreamingContent)) {
                LOGM_FATAL("NULL mpStreamingContent\n");
                return EECode_BadState;
            }

            //parameters check
            if (DUnlikely(content_index >= mnComponentProxysNumbers[EGenericComponentType_StreamingContent - EGenericComponentType_TAG_proxy_start])) {
                LOGM_FATAL("content index %d exceed max %d\n", content_index, mnComponentProxysNumbers[EGenericComponentType_StreamingContent - EGenericComponentType_TAG_proxy_start]);
                return EECode_BadParam;
            }

            p_content = mpStreamingContent + content_index;

            snprintf(p_content->session_name, DMaxStreamContentUrlLength - 1, "%s", streaming_tag);
            p_content->session_name[DMaxStreamContentUrlLength - 1] = 0x0;
        }

    } else {
        LOGM_FATAL("BAD component id 0x%08x\n", content_id);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::SetupUploadingReceiverContent(TGenericID content_id, TGenericID server_id, TGenericID receiver_id, TChar *receiver_tag)
{
    SComponent_Obsolete *component = NULL;
    TUint size = 0;

    if (receiver_tag) {
        LOGM_NOTICE("SetupUploadingReceiverContent(content id %08x, server_id %08x, receiver_id %08x, receiver_tag %s)\n", content_id, server_id, receiver_id, receiver_tag);
    } else {
        LOGM_NOTICE("SetupUploadingReceiverContent(content id %08x, server_id %08x, receiver_id %08x)\n", content_id, server_id, receiver_id);
    }

    //debug assert
    DASSERT(IS_VALID_COMPONENT_ID(content_id));
    DASSERT(IS_VALID_COMPONENT_ID(server_id));
    DASSERT(IS_VALID_COMPONENT_ID(receiver_id));
    DASSERT(EGenericComponentType_CloudReceiverContent == DCOMPONENT_TYPE_FROM_GENERIC_ID(content_id));
    DASSERT(EGenericComponentType_CloudServer == DCOMPONENT_TYPE_FROM_GENERIC_ID(server_id));
    DASSERT(EGenericComponentType_Demuxer == DCOMPONENT_TYPE_FROM_GENERIC_ID(receiver_id));
    DASSERT(mnComponentProxysNumbers[EGenericComponentType_CloudReceiverContent - EGenericComponentType_TAG_proxy_start] > DCOMPONENT_INDEX_FROM_GENERIC_ID(content_id));
    DASSERT(mnComponentProxysNumbers[EGenericComponentType_CloudServer - EGenericComponentType_TAG_proxy_start] > DCOMPONENT_INDEX_FROM_GENERIC_ID(server_id));
    DASSERT(mnComponentFiltersNumbers[EGenericComponentType_Demuxer] > DCOMPONENT_INDEX_FROM_GENERIC_ID(receiver_id));

    if (!receiver_tag) {
        LOGM_NOTICE("NULL receiver_tag in SetupUploadingReceiverContent\n");
    } else {
        size = strlen(receiver_tag);
        DASSERT(size < 1024);
        if (size >= 1024) {
            LOGM_ERROR("url too long(size %d) in SetupUploadingReceiverContent, not support\n", size);
            return EECode_BadParam;
        }
    }

    component = findComponentFromID(content_id);

    if (component) {

        if (receiver_tag) {
            DASSERT(NULL == component->url);
            if (component->url) {
                LOGM_ERROR("already set url? %s\n", component->url);
                DDBG_FREE(component->url, "E0UR");
                component->url = NULL;
            }

            component->url = (TChar *)DDBG_MALLOC(size + 4, "E0UR");
            if (!component->url) {
                LOGM_ERROR("NO memory\n");
                return EECode_BadParam;
            }
            strcpy(component->url, receiver_tag);
        }

    } else {
        LOGM_ERROR("BAD content id 0x%08x\n", content_id);
        return EECode_BadParam;
    }

    component->cloud_server_id = server_id;
    component->cloud_receiver_id = receiver_id;
    return EECode_OK;
}

EECode CActiveGenericEngine::SetCloudClientRecieverTag(TGenericID receiver_id, TChar *receiver_tag)
{
    SComponent_Obsolete *component = NULL;
    TUint size = 0;

    if (receiver_tag) {
        LOGM_NOTICE("SetCloudClientRecieverTag(receiver_id %08x, receiver_tag %s)\n", receiver_id, receiver_tag);
    } else {
        LOGM_NOTICE("SetCloudClientRecieverTag(receiver_id %08x)\n",  receiver_id);
    }

    //debug assert
    DASSERT(IS_VALID_COMPONENT_ID(receiver_id));
    DASSERT(EGenericComponentType_CloudReceiverContent == DCOMPONENT_TYPE_FROM_GENERIC_ID(receiver_id));
    DASSERT(mnComponentProxysNumbers[EGenericComponentType_CloudReceiverContent - EGenericComponentType_TAG_proxy_start] > DCOMPONENT_INDEX_FROM_GENERIC_ID(receiver_id));

    if (!receiver_tag) {
        LOGM_NOTICE("NULL pointer in SetCloudClientRecieverTag\n");
    } else {
        size = strlen(receiver_tag);
        DASSERT(size < 1024);
        if (size >= 1024) {
            LOGM_ERROR("url too long(size %d) in SetCloudClientRecieverTag, not support\n", size);
            return EECode_BadParam;
        }
    }

    component = findComponentFromID(receiver_id);

    if (component) {

        if (receiver_tag) {
            DASSERT(NULL == component->url);
            if (component->url) {
                LOGM_ERROR("already set url? %s\n", component->url);
                DDBG_FREE(component->url, "E0UR");
                component->url = NULL;
            }

            component->url = (TChar *)DDBG_MALLOC(size + 4, "E0UR");
            if (!component->url) {
                LOGM_ERROR("NO memory\n");
                return EECode_BadParam;
            }
            strcpy(component->url, receiver_tag);
        }

        if (mbEnginePipelineRunning) {
            LOGM_NOTICE("SetCloudClientRecieverTag(%s) runtime, receiver id 0x%08x\n", component->url, receiver_id);
            if (DLikely(component->p_filter)) {
                component->p_filter->FilterProperty(EFilterPropertyType_update_source_url, 1, component->url);
            } else {
                LOGM_ERROR("NULL component->p_filter\n");
            }

            SCloudContent *p_content = NULL;
            TComponentIndex content_index = DCOMPONENT_INDEX_FROM_GENERIC_ID(receiver_id);

            if (DUnlikely(!mpCloudReceiverContent)) {
                LOGM_FATAL("NULL mpCloudReceiverContent\n");
                return EECode_BadState;
            }

            //parameters check
            if (DUnlikely(content_index >= mnComponentProxysNumbers[EGenericComponentType_CloudReceiverContent - EGenericComponentType_TAG_proxy_start])) {
                LOGM_FATAL("content index %d exceed max %d\n", content_index, mnComponentProxysNumbers[EGenericComponentType_CloudReceiverContent - EGenericComponentType_TAG_proxy_start]);
                return EECode_BadParam;
            }

            p_content = mpCloudReceiverContent + content_index;

            snprintf(p_content->content_name, DMaxStreamContentUrlLength - 1, "%s", receiver_tag);
            p_content->content_name[DMaxStreamContentUrlLength - 1] = 0x0;

        }

    } else {
        LOGM_FATAL("BAD component id 0x%08x\n", receiver_id);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::EnableConnection(TGenericID connection_id, TUint enable)
{
    LOGM_ERROR("add implement\n");
    return EECode_OK;
}

EECode CActiveGenericEngine::selectDSPMode()
{
    switch (mRequestWorkingMode) {
        case EMediaDeviceWorkingMode_SingleInstancePlayback:
            mRequestDSPMode = EDSPOperationMode_UDEC;
            break;

        case EMediaDeviceWorkingMode_MultipleInstancePlayback:
            mRequestDSPMode = EDSPOperationMode_MultipleWindowUDEC;
            break;

        case EMediaDeviceWorkingMode_MultipleInstancePlayback_Transcoding:
            mRequestDSPMode = EDSPOperationMode_MultipleWindowUDEC_Transcode;
            break;
        case EMediaDeviceWorkingMode_MultipleInstanceRecording:
            mRequestDSPMode = EDSPOperationMode_CameraRecording;
            break;

        case EMediaDeviceWorkingMode_FullDuplex_SingleRecSinglePlayback:
            mRequestDSPMode = EDSPOperationMode_FullDuplex;
            break;

        default:
            LOGM_FATAL("TODO mRequestWorkingMode %d\n", mRequestWorkingMode);
            return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::componentNumberStatistics()
{
    CIDoubleLinkedList::SNode *pnode = mListFilters.FirstNode();
    SComponent_Obsolete *p_component = NULL;

    mnDemuxerNum = 0;
    mnMuxerNum = 0;
    mnVideoDecoderNum = 0;
    mnVideoEncoderNum = 0;
    mnAudioDecoderNum = 0;
    mnAudioEncoderNum = 0;

    while (pnode) {
        p_component = (SComponent_Obsolete *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            switch (p_component->type) {
                case EGenericComponentType_Demuxer:
                    mnDemuxerNum ++;
                    break;

                case EGenericComponentType_VideoDecoder:
                    mnVideoDecoderNum ++;
                    break;

                case EGenericComponentType_AudioDecoder:
                    mnAudioDecoderNum ++;
                    break;

                case EGenericComponentType_VideoEncoder:
                    mnVideoEncoderNum ++;
                    break;

                case EGenericComponentType_AudioEncoder:
                    mnAudioEncoderNum ++;
                    break;

                case EGenericComponentType_VideoPostProcessor:
                    break;

                case EGenericComponentType_AudioPostProcessor:
                    break;

                case EGenericComponentType_VideoPreProcessor:
                    LOGM_NOTICE("TO DO\n");
                    break;

                case EGenericComponentType_AudioPreProcessor:
                    LOGM_NOTICE("TO DO\n");
                    break;

                case EGenericComponentType_VideoCapture:
                    LOGM_NOTICE("TO DO\n");
                    break;

                case EGenericComponentType_AudioCapture:
                    LOGM_NOTICE("TO DO\n");
                    break;

                case EGenericComponentType_VideoRenderer:
                    break;

                case EGenericComponentType_AudioRenderer:
                    break;

                case EGenericComponentType_Muxer:
                    mnMuxerNum ++;
                    break;

                case EGenericComponentType_StreamingServer:
                    mnRTSPStreamingServerNum ++;
                    break;

                case EGenericComponentType_StreamingTransmiter:
                    mnStreamingTransmiterNum ++;
                    break;

                case EGenericComponentType_ConnecterPinMuxer:
                    break;

                case EGenericComponentType_StreamingContent:
                    LOGM_NOTICE("TO DO\n");
                    break;

                    //                case EGenericComponentType_CommonicationMsgPort:
                    //                    LOGM_NOTICE("TO DO\n");
                    //                    break;

                case EGenericComponentType_ConnectionPin:
                    LOGM_NOTICE("TO DO\n");
                    break;

                default:
                    LOGM_ERROR("unknown type %d\n", p_component->type);
                    return EECode_Error;
            }

        } else {
            LOGM_FATAL("NULL pointer here, something would be wrong.\n");
        }
        pnode = mListFilters.NextNode(pnode);
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::autoSelectDSPMode()
{
    if ((mnVideoDecoderNum >= 2) && (0 == mnVideoEncoderNum)) {
        mRequestDSPMode = EDSPOperationMode_MultipleWindowUDEC;
        LOGM_NOTICE("select M UDEC mode\n");
    } else if (1 == mnVideoDecoderNum) {
        mRequestDSPMode = EDSPOperationMode_UDEC;
        LOGM_NOTICE("select UDEC mode\n");
    }

    return EECode_OK;
}

void CActiveGenericEngine::summarizeVoutParams()
{
    TInt i = 0;

    mMaxVoutWidth = 0;
    mMaxVoutHeight = 0;
    //parse voutconfig
    mVoutNumber = 0;
    mVoutStartIndex = 0;

    for (i = 0; i < EDisplayDevice_TotCount; i++) {

        if (mPersistMediaConfig.dsp_config.voutConfigs.voutConfig[i].width > mMaxVoutWidth) {
            mMaxVoutWidth = mPersistMediaConfig.dsp_config.voutConfigs.voutConfig[i].width;
        }
        if (mPersistMediaConfig.dsp_config.voutConfigs.voutConfig[i].height > mMaxVoutHeight) {
            mMaxVoutHeight = mPersistMediaConfig.dsp_config.voutConfigs.voutConfig[i].height;
        }

        if (!(mRequestVoutMask & DCAL_BITMASK(i))) {
            LOGM_INFO(" not config vout %d.\n", i);
            continue;
        }

        if (mPersistMediaConfig.dsp_config.voutConfigs.voutConfig[i].failed) {
            mRequestVoutMask &= ~ DCAL_BITMASK(i);
            LOGM_ERROR("vout %d failed.\n", i);
            continue;
        }

        mVoutNumber ++;

    }

    if (mRequestVoutMask & DCAL_BITMASK(EDisplayDevice_LCD)) {
        mVoutStartIndex = EDisplayDevice_LCD;
    } else if (mRequestVoutMask & DCAL_BITMASK(EDisplayDevice_HDMI)) {
        mVoutStartIndex = EDisplayDevice_HDMI;
    } else {
        mVoutStartIndex = EDisplayDevice_LCD;
        mVoutNumber = 0;
    }
    LOGM_INFO("mVoutStartIndex %d, mVoutNumber %d, mRequestVoutMask 0x%x.\n", mVoutStartIndex, mVoutNumber, mRequestVoutMask);
}

EECode CActiveGenericEngine::setupMUdecDSPSettings()
{
    TUint i;
    EECode err;

    DASSERT(EDSPOperationMode_MultipleWindowUDEC == mRequestDSPMode ||
            EDSPOperationMode_MultipleWindowUDEC_Transcode == mRequestDSPMode);
    if (EDSPOperationMode_MultipleWindowUDEC != mRequestDSPMode &&
            EDSPOperationMode_MultipleWindowUDEC_Transcode != mRequestDSPMode) {
        return EECode_BadState;
    }

    DASSERT(mTotalVideoDecoderNumber < 11);

    //udec settings
    if (mTotalVideoDecoderNumber >= 11) {
        mTotalVideoDecoderNumber = 10;
    }

    memset(&mInitalVideoPostPGlobalSetting, 0x0, sizeof(mInitalVideoPostPGlobalSetting));
    memset(&mInitalVideoPostPDisplayLayout, 0x0, sizeof(mInitalVideoPostPDisplayLayout));
    memset(&mInitialVideoPostPConfig, 0x0, sizeof(mInitialVideoPostPConfig));
    mbCertainSDWindowSwitchedtoHD = 0;

    //set initial display setting
    mInitalVideoPostPDisplayLayout.cur_window_start_index = 0;
    mInitalVideoPostPDisplayLayout.cur_render_start_index = 0;
    mInitalVideoPostPDisplayLayout.cur_decoder_start_index = 0;

    switch (mPresetPlaybackMode) {

        case EPlaybackMode_1x1080p:
        case EPlaybackMode_1x3M:
            mInitalVideoPostPDisplayLayout.layer_number = 1;
            mInitalVideoPostPDisplayLayout.layer_window_number[0] = 1;
            mInitalVideoPostPDisplayLayout.total_window_number = 1;
            mInitalVideoPostPDisplayLayout.total_render_number = 1;
            mInitalVideoPostPDisplayLayout.total_decoder_number = 1;
            mInitalVideoPostPDisplayLayout.cur_window_number = 1;
            mInitalVideoPostPDisplayLayout.cur_render_number = 1;
            mInitalVideoPostPDisplayLayout.cur_decoder_number = 1;
            break;

        case EPlaybackMode_1x1080p_4xD1:
        case EPlaybackMode_1x3M_4xD1:
            mInitalVideoPostPDisplayLayout.layer_number = 2;
            mInitalVideoPostPDisplayLayout.layer_window_number[0] = 4;
            mInitalVideoPostPDisplayLayout.layer_window_number[1] = 1;
            mInitalVideoPostPDisplayLayout.total_window_number = 5;
            mInitalVideoPostPDisplayLayout.total_render_number = 5;
            mInitalVideoPostPDisplayLayout.total_decoder_number = 5;
            mInitalVideoPostPDisplayLayout.cur_window_number = 5;
            mInitalVideoPostPDisplayLayout.cur_render_number = 4;
            mInitalVideoPostPDisplayLayout.cur_decoder_number = 5;
            break;

        case EPlaybackMode_1x1080p_6xD1:
        case EPlaybackMode_1x3M_6xD1:
            mInitalVideoPostPDisplayLayout.layer_number = 2;
            mInitalVideoPostPDisplayLayout.layer_window_number[0] = 6;
            mInitalVideoPostPDisplayLayout.layer_window_number[1] = 1;
            mInitalVideoPostPDisplayLayout.total_window_number = 7;
            mInitalVideoPostPDisplayLayout.total_render_number = 7;
            mInitalVideoPostPDisplayLayout.total_decoder_number = 7;
            mInitalVideoPostPDisplayLayout.cur_window_number = 7;
            mInitalVideoPostPDisplayLayout.cur_render_number = 6;
            mInitalVideoPostPDisplayLayout.cur_decoder_number = 7;
            break;

        case EPlaybackMode_1x1080p_8xD1:
        case EPlaybackMode_1x3M_8xD1:
            mInitalVideoPostPDisplayLayout.layer_number = 2;
            mInitalVideoPostPDisplayLayout.layer_window_number[0] = 8;
            mInitalVideoPostPDisplayLayout.layer_window_number[1] = 1;
            mInitalVideoPostPDisplayLayout.total_window_number = 9;
            mInitalVideoPostPDisplayLayout.total_render_number = 9;
            mInitalVideoPostPDisplayLayout.total_decoder_number = 9;
            mInitalVideoPostPDisplayLayout.cur_window_number = 9;
            mInitalVideoPostPDisplayLayout.cur_render_number = 8;
            mInitalVideoPostPDisplayLayout.cur_decoder_number = 9;
            break;

        case EPlaybackMode_1x1080p_9xD1:
        case EPlaybackMode_1x3M_9xD1:
            mInitalVideoPostPDisplayLayout.layer_number = 2;
            mInitalVideoPostPDisplayLayout.layer_window_number[0] = 9;
            mInitalVideoPostPDisplayLayout.layer_window_number[1] = 1;
            mInitalVideoPostPDisplayLayout.total_window_number = 10;
            mInitalVideoPostPDisplayLayout.total_render_number = 10;
            mInitalVideoPostPDisplayLayout.total_decoder_number = 10;
            mInitalVideoPostPDisplayLayout.cur_window_number = 10;
            mInitalVideoPostPDisplayLayout.cur_render_number = 9;
            mInitalVideoPostPDisplayLayout.cur_decoder_number = 10;
            break;

        case EPlaybackMode_4x720p:
            mInitalVideoPostPDisplayLayout.layer_number = 1;
            mInitalVideoPostPDisplayLayout.layer_window_number[0] = 4;
            mInitalVideoPostPDisplayLayout.total_window_number = 4;
            mInitalVideoPostPDisplayLayout.total_render_number = 4;
            mInitalVideoPostPDisplayLayout.total_decoder_number = 4;
            mInitalVideoPostPDisplayLayout.cur_window_number = 4;
            mInitalVideoPostPDisplayLayout.cur_render_number = 4;
            mInitalVideoPostPDisplayLayout.cur_decoder_number = 4;
            break;

        case EPlaybackMode_4xD1:
            mInitalVideoPostPDisplayLayout.layer_number = 1;
            mInitalVideoPostPDisplayLayout.layer_window_number[0] = 4;
            mInitalVideoPostPDisplayLayout.total_window_number = 4;
            mInitalVideoPostPDisplayLayout.total_render_number = 4;
            mInitalVideoPostPDisplayLayout.total_decoder_number = 4;
            mInitalVideoPostPDisplayLayout.cur_window_number = 4;
            mInitalVideoPostPDisplayLayout.cur_render_number = 4;
            mInitalVideoPostPDisplayLayout.cur_decoder_number = 4;
            break;

        case EPlaybackMode_6xD1:
            mInitalVideoPostPDisplayLayout.layer_number = 1;
            mInitalVideoPostPDisplayLayout.layer_window_number[0] = 6;
            mInitalVideoPostPDisplayLayout.total_window_number = 6;
            mInitalVideoPostPDisplayLayout.total_render_number = 6;
            mInitalVideoPostPDisplayLayout.total_decoder_number = 6;
            mInitalVideoPostPDisplayLayout.cur_window_number = 6;
            mInitalVideoPostPDisplayLayout.cur_render_number = 6;
            mInitalVideoPostPDisplayLayout.cur_decoder_number = 6;
            break;

        case EPlaybackMode_8xD1:
            mInitalVideoPostPDisplayLayout.layer_number = 1;
            mInitalVideoPostPDisplayLayout.layer_window_number[0] = 8;
            mInitalVideoPostPDisplayLayout.total_window_number = 8;
            mInitalVideoPostPDisplayLayout.total_render_number = 8;
            mInitalVideoPostPDisplayLayout.total_decoder_number = 8;
            mInitalVideoPostPDisplayLayout.cur_window_number = 8;
            mInitalVideoPostPDisplayLayout.cur_render_number = 8;
            mInitalVideoPostPDisplayLayout.cur_decoder_number = 8;
            break;

        case EPlaybackMode_9xD1:
            mInitalVideoPostPDisplayLayout.layer_number = 1;
            mInitalVideoPostPDisplayLayout.layer_window_number[0] = 9;
            mInitalVideoPostPDisplayLayout.total_window_number = 9;
            mInitalVideoPostPDisplayLayout.total_render_number = 9;
            mInitalVideoPostPDisplayLayout.total_decoder_number = 9;
            mInitalVideoPostPDisplayLayout.cur_window_number = 9;
            mInitalVideoPostPDisplayLayout.cur_render_number = 9;
            mInitalVideoPostPDisplayLayout.cur_decoder_number = 9;
            break;

        default:
            LOGM_FATAL("BAD mPresetPlaybackMode %d\n", mPresetPlaybackMode);
            return EECode_InternalLogicalBug;
            break;
    }
    mInitalVideoPostPDisplayLayout.layer_layout_type[0] = EDisplayLayout_Rectangle;
    mInitalVideoPostPDisplayLayout.layer_layout_type[1] = EDisplayLayout_Rectangle;

    //to do, hard code here
    mInitialVideoDisplayMask = mRequestVoutMask;
    if (mRequestVoutMask & DCAL_BITMASK(EDisplayDevice_HDMI)) {
        mInitialVideoDisplayPrimaryIndex = EDisplayDevice_HDMI;
    } else if (mRequestVoutMask & DCAL_BITMASK(EDisplayDevice_LCD)) {
        mInitialVideoDisplayPrimaryIndex = EDisplayDevice_LCD;
    } else {
        LOGM_INFO("Disable vout, set the primary vout to be HDMI!\n");
        mInitialVideoDisplayPrimaryIndex = EDisplayDevice_HDMI;
    }
    LOGM_PRINTF("mRequestVoutMask 0x%08x, mInitialVideoDisplayPrimaryIndex %d\n", mRequestVoutMask, mInitialVideoDisplayPrimaryIndex);
    mInitialVideoDisplayCurrentMask = mInitialVideoDisplayMask;

    mPersistMediaConfig.dsp_config.modeConfig.enable_transcode = (EDSPOperationMode_MultipleWindowUDEC_Transcode == mRequestDSPMode);
    mPersistMediaConfig.dsp_config.modeConfig.postp_mode = 3;
    mPersistMediaConfig.dsp_config.enable_deint = 0;
    mPersistMediaConfig.dsp_config.modeConfig.pp_chroma_fmt_max = 2;
    mPersistMediaConfig.dsp_config.modeConfig.pp_max_frm_num = 5;
    mPersistMediaConfig.dsp_config.modeConfig.pp_max_frm_width = mMaxVoutWidth;
    mPersistMediaConfig.dsp_config.modeConfig.pp_max_frm_height = mMaxVoutHeight;

    mPersistMediaConfig.dsp_config.modeConfig.vout_mask = mRequestVoutMask;
    mPersistMediaConfig.dsp_config.modeConfig.primary_vout = mInitialVideoDisplayPrimaryIndex;

    mPersistMediaConfig.dsp_config.modeConfigMudec.av_sync_enabled = 0;

    if (mRequestVoutMask & (DCAL_BITMASK(EDisplayDevice_LCD))) {
        mPersistMediaConfig.dsp_config.modeConfigMudec.voutA_enabled = 1;
    }

    if (mRequestVoutMask & (DCAL_BITMASK(EDisplayDevice_HDMI))) {
        mPersistMediaConfig.dsp_config.modeConfigMudec.voutB_enabled = 1;
    }
    //DASSERT((mPersistMediaConfig.dsp_config.modeConfigMudec.voutA_enabled) || (mPersistMediaConfig.dsp_config.modeConfigMudec.voutB_enabled));

    mPersistMediaConfig.dsp_config.modeConfigMudec.audio_on_win_id = 0;

    mPersistMediaConfig.dsp_config.modeConfigMudec.video_win_width = mPersistMediaConfig.dsp_config.voutConfigs.voutConfig[mInitialVideoDisplayPrimaryIndex].width;
    mPersistMediaConfig.dsp_config.modeConfigMudec.video_win_height = mPersistMediaConfig.dsp_config.voutConfigs.voutConfig[mInitialVideoDisplayPrimaryIndex].height;

    for (i = 0; i < mTotalVideoDecoderNumber; i++) {
        mPersistMediaConfig.dsp_config.udecInstanceConfig[i].tiled_mode = mVideoDecoder[i].tiled_mode;
        mPersistMediaConfig.dsp_config.udecInstanceConfig[i].frm_chroma_fmt_max = mVideoDecoder[i].frm_chroma_fmt_max;
        mPersistMediaConfig.dsp_config.udecInstanceConfig[i].dec_types = 0x17;
        mPersistMediaConfig.dsp_config.udecInstanceConfig[i].frm_chroma_fmt_max = mVideoDecoder[i].frm_chroma_fmt_max;
        mPersistMediaConfig.dsp_config.udecInstanceConfig[i].max_frm_width = mVideoDecoder[i].max_frm_width;
        mPersistMediaConfig.dsp_config.udecInstanceConfig[i].max_frm_height = mVideoDecoder[i].max_frm_height;
        mPersistMediaConfig.dsp_config.udecInstanceConfig[i].max_fifo_size = mVideoDecoder[i].max_fifo_size;
        mPersistMediaConfig.dsp_config.udecInstanceConfig[i].bits_fifo_size = mVideoDecoder[i].bits_fifo_size;
    }

    DASSERT(mInitialVideoDisplayPrimaryIndex < EDisplayDevice_TotCount);
    mInitalVideoPostPGlobalSetting.display_mask = mInitialVideoDisplayMask;
    mInitalVideoPostPGlobalSetting.primary_display_index = mInitialVideoDisplayPrimaryIndex;
    mInitalVideoPostPGlobalSetting.cur_display_mask = mInitialVideoDisplayCurrentMask;
    if (mInitialVideoDisplayPrimaryIndex < EDisplayDevice_TotCount) {
        mInitalVideoPostPGlobalSetting.display_width = mPersistMediaConfig.dsp_config.voutConfigs.voutConfig[mInitialVideoDisplayPrimaryIndex].width;
        mInitalVideoPostPGlobalSetting.display_height = mPersistMediaConfig.dsp_config.voutConfigs.voutConfig[mInitialVideoDisplayPrimaryIndex].height;
    } else {
        LOGM_FATAL("Internal bug, BAD mInitialVideoDisplayPrimaryIndex %d\n", mInitialVideoDisplayPrimaryIndex);
        return EECode_BadParam;
    }

    err = gfPresetDisplayLayout(&mInitalVideoPostPGlobalSetting, &mInitalVideoPostPDisplayLayout, &mInitialVideoPostPConfig);
    DASSERT_OK(err);

    err = gfDefaultRenderConfig((SDSPMUdecRender *)&mInitialVideoPostPConfig.render[0], mInitialVideoPostPConfig.cur_render_number, mInitialVideoPostPConfig.total_render_number - mInitialVideoPostPConfig.cur_render_number);
    DASSERT_OK(err);

    DASSERT(mInitialVideoPostPConfig.total_window_number);//debug assert
    DASSERT(mInitialVideoPostPConfig.total_window_number <= DMaxPostPWindowNumber);//debug assert
    //window config
    for (i = 0; i < mInitialVideoPostPConfig.total_window_number; i++) {
        mPersistMediaConfig.dsp_config.modeConfigMudec.windows_config[i].win_config_id = mInitialVideoPostPConfig.window[i].win_config_id;
        mPersistMediaConfig.dsp_config.modeConfigMudec.windows_config[i].input_offset_x = mInitialVideoPostPConfig.window[i].input_offset_x;
        mPersistMediaConfig.dsp_config.modeConfigMudec.windows_config[i].input_offset_y = mInitialVideoPostPConfig.window[i].input_offset_y;
        mPersistMediaConfig.dsp_config.modeConfigMudec.windows_config[i].input_width = mInitialVideoPostPConfig.window[i].input_width;
        mPersistMediaConfig.dsp_config.modeConfigMudec.windows_config[i].input_height = mInitialVideoPostPConfig.window[i].input_height;

        mPersistMediaConfig.dsp_config.modeConfigMudec.windows_config[i].target_win_offset_x = mInitialVideoPostPConfig.window[i].target_win_offset_x;
        mPersistMediaConfig.dsp_config.modeConfigMudec.windows_config[i].target_win_offset_y = mInitialVideoPostPConfig.window[i].target_win_offset_y;
        mPersistMediaConfig.dsp_config.modeConfigMudec.windows_config[i].target_win_width = mInitialVideoPostPConfig.window[i].target_win_width;
        mPersistMediaConfig.dsp_config.modeConfigMudec.windows_config[i].target_win_height = mInitialVideoPostPConfig.window[i].target_win_height;
    }

    DASSERT(mInitialVideoPostPConfig.total_render_number);//debug assert
    DASSERT(mInitialVideoPostPConfig.total_render_number <= DMaxPostPRenderNumber);//debug assert
    //render config
    for (i = 0; i < mInitialVideoPostPConfig.total_render_number; i++) {
        mPersistMediaConfig.dsp_config.modeConfigMudec.render_config[i].render_id = mInitialVideoPostPConfig.render[i].render_id;
        mPersistMediaConfig.dsp_config.modeConfigMudec.render_config[i].win_config_id = mInitialVideoPostPConfig.render[i].win_config_id;
        mPersistMediaConfig.dsp_config.modeConfigMudec.render_config[i].win_config_id_2nd = mInitialVideoPostPConfig.render[i].win_config_id_2nd;
        mPersistMediaConfig.dsp_config.modeConfigMudec.render_config[i].udec_id = mInitialVideoPostPConfig.render[i].udec_id;
        mPersistMediaConfig.dsp_config.modeConfigMudec.render_config[i].first_pts_low = mInitialVideoPostPConfig.render[i].first_pts_low;
        mPersistMediaConfig.dsp_config.modeConfigMudec.render_config[i].first_pts_high = mInitialVideoPostPConfig.render[i].first_pts_high;
        mPersistMediaConfig.dsp_config.modeConfigMudec.render_config[i].input_source_type = mInitialVideoPostPConfig.render[i].input_source_type;
    }

    mPersistMediaConfig.dsp_config.modeConfig.num_udecs = mTotalVideoDecoderNumber;

    mPersistMediaConfig.dsp_config.modeConfigMudec.total_num_render_configs = mInitialVideoPostPConfig.cur_render_number;
    mPersistMediaConfig.dsp_config.modeConfigMudec.total_num_win_configs = mInitialVideoPostPConfig.total_window_number;
    mPersistMediaConfig.dsp_config.modeConfigMudec.max_num_windows = mInitialVideoPostPConfig.total_window_number + 1;
    LOGM_PRINTF("udec_num %u, win_num %u, render_num %u, voutmask %08x.\n", mPersistMediaConfig.dsp_config.modeConfig.num_udecs, mPersistMediaConfig.dsp_config.modeConfigMudec.total_num_win_configs, mPersistMediaConfig.dsp_config.modeConfigMudec.total_num_render_configs, mRequestVoutMask);

    return EECode_OK;
}

EECode CActiveGenericEngine::setupUdecDSPSettings()
{
    DASSERT(EDSPOperationMode_UDEC == mRequestDSPMode);
    if (EDSPOperationMode_UDEC != mRequestDSPMode) {
        return EECode_BadState;
    }

    mPersistMediaConfig.dsp_config.modeConfig.num_udecs = 1;

    mPersistMediaConfig.dsp_config.modeConfig.postp_mode = 2;
    mPersistMediaConfig.dsp_config.enable_deint = 0;
    mPersistMediaConfig.dsp_config.modeConfig.pp_chroma_fmt_max = 2;
    mPersistMediaConfig.dsp_config.modeConfig.pp_max_frm_num = 5;
    mPersistMediaConfig.dsp_config.modeConfig.pp_max_frm_width = mMaxVoutWidth;
    mPersistMediaConfig.dsp_config.modeConfig.pp_max_frm_height = mMaxVoutHeight;

    mPersistMediaConfig.dsp_config.modeConfig.vout_mask = mRequestVoutMask;

    mPersistMediaConfig.dsp_config.udecInstanceConfig[0].tiled_mode = mVideoDecoder[0].tiled_mode;
    mPersistMediaConfig.dsp_config.udecInstanceConfig[0].frm_chroma_fmt_max = mVideoDecoder[0].frm_chroma_fmt_max;
    mPersistMediaConfig.dsp_config.udecInstanceConfig[0].dec_types = 0x3f;
    mPersistMediaConfig.dsp_config.udecInstanceConfig[0].frm_chroma_fmt_max = mVideoDecoder[0].frm_chroma_fmt_max;
    mPersistMediaConfig.dsp_config.udecInstanceConfig[0].max_frm_num = mVideoDecoder[0].max_frm_num;
    mPersistMediaConfig.dsp_config.udecInstanceConfig[0].max_frm_width = mVideoDecoder[0].max_frm_width;
    mPersistMediaConfig.dsp_config.udecInstanceConfig[0].max_frm_height = mVideoDecoder[0].max_frm_height;
    mPersistMediaConfig.dsp_config.udecInstanceConfig[0].max_fifo_size = mVideoDecoder[0].max_fifo_size;
    mPersistMediaConfig.dsp_config.udecInstanceConfig[0].bits_fifo_size = mVideoDecoder[0].bits_fifo_size;

    mPersistMediaConfig.dsp_config.voutConfigs.src_pos_x = 0;
    mPersistMediaConfig.dsp_config.voutConfigs.src_pos_y = 0;
    mPersistMediaConfig.dsp_config.voutConfigs.src_size_x = mVideoDecoder[0].max_frm_width;
    mPersistMediaConfig.dsp_config.voutConfigs.src_size_y = mVideoDecoder[0].max_frm_height;

    LOGM_NOTICE("[setupUdecDSPSettings]: udec_num %u, voutmask %08x\n", mPersistMediaConfig.dsp_config.modeConfig.num_udecs, mPersistMediaConfig.dsp_config.modeConfig.vout_mask);
    LOGM_NOTICE("[setupUdecDSPSettings]: mPersistMediaConfig.dsp_config.udecInstanceConfig[0].max_frm_width %u, mPersistMediaConfig.dsp_config.udecInstanceConfig[0].max_frm_height %u.\n", mPersistMediaConfig.dsp_config.udecInstanceConfig[0].max_frm_width, mPersistMediaConfig.dsp_config.udecInstanceConfig[0].max_frm_height);

    return EECode_OK;
}

EECode CActiveGenericEngine::setupDSPConfig(TUint mode)
{
    EECode err = EECode_OK;
    switch (mode) {
        case EDSPOperationMode_MultipleWindowUDEC:
        case EDSPOperationMode_MultipleWindowUDEC_Transcode:
            err = setupMUdecDSPSettings();
            break;
        case EDSPOperationMode_UDEC:
            err = setupUdecDSPSettings();
            break;
        default:
            err = EECode_BadParam;
            LOGM_ERROR("Unsupported dsp mode\n");
            break;
    }
    return err;
}

EECode CActiveGenericEngine::activateWorkingMode(TUint mode)
{
    EECode err;

    //debug assert
    DASSERT(mpDSPAPI);
    DASSERT(0 <= mIavFd);

    if (EDSPOperationMode_MultipleWindowUDEC == mode ||
            EDSPOperationMode_MultipleWindowUDEC_Transcode == mode) {
        if (mpDSPAPI) {

            err = mpDSPAPI->DSPModeConfig((const SPersistMediaConfig *)&mPersistMediaConfig);
            DASSERT(EECode_OK == err);
            err = mpDSPAPI->EnterDSPMode(mode);
            DASSERT(EECode_OK == err);
            if (EECode_OK == err) {
                return EECode_OK;
            } else {
                LOGM_ERROR("mpDSPAPI->EnterDSPMode(%d) fail, return %d\n", mode, err);
                return EECode_OSError;
            }
        } else {
            LOGM_ERROR("NULL mpDSPAPI\n");
            return EECode_BadParam;
        }
    } else if (EDSPOperationMode_UDEC == mode) {
        if (mpDSPAPI) {
            err = mpDSPAPI->DSPModeConfig((const SPersistMediaConfig *)&mPersistMediaConfig);
            DASSERT(EECode_OK == err);
            err = mpDSPAPI->EnterDSPMode(mode);
            DASSERT(EECode_OK == err);
            if (EECode_OK == err) {
                return EECode_OK;
            } else {
                LOGM_ERROR("mpDSPAPI->EnterDSPMode(%d) fail, return %d\n", mode, err);
                return EECode_OSError;
            }
        } else {
            LOGM_ERROR("NULL mpDSPAPI\n");
            return EECode_BadParam;
        }
    } else if (EDSPOperationMode_FullDuplex == mode) {
        LOGM_ERROR("Need implement for dsp mode(%d) request\n", mode);
        return EECode_NotSupported;
    } else if (EDSPOperationMode_CameraRecording == mode) {
        LOGM_ERROR("Need implement for dsp mode(%d) request\n", mode);
        return EECode_NotSupported;
    } else {
        LOGM_ERROR("BAD dsp mode(%d) request\n", mode);
        return EECode_BadParam;
    }

    LOGM_NOTICE("[flow]: activateWorkingMode(%d) success.\n", mode);
    return EECode_OK;
}

EECode CActiveGenericEngine::initializeFilter(SComponent_Obsolete *component, TU8 type)
{
    EECode err = EECode_OK;
    IFilter *p_filter = NULL;
    DASSERT(component);

    p_filter = component->p_filter;
    DASSERT(p_filter);

    switch (type) {

        case EGenericComponentType_Demuxer:
            err = p_filter->Initialize(component->url);
            DASSERT_OK(err);
            break;

        case EGenericComponentType_VideoDecoder:
            err = p_filter->Initialize((TChar *)"AMBA");
            DASSERT_OK(err);
            break;
        case EGenericComponentType_VideoEncoder:
            err = p_filter->Initialize((TChar *)"AMBA");
            DASSERT_OK(err);
            break;

        case EGenericComponentType_AudioDecoder:
            if (EAudioDecoderModuleID_FFMpeg == mPersistMediaConfig.prefer_module_setting.mPreferedAudioDecoderModule) {
                err = p_filter->Initialize((TChar *)"FFMpeg");
            } else if (EAudioDecoderModuleID_AAC == mPersistMediaConfig.prefer_module_setting.mPreferedAudioDecoderModule) {
                err = p_filter->Initialize((TChar *)"AAC");
            } else {
                err = p_filter->Initialize((TChar *)"FFMpeg");
            }
            DASSERT_OK(err);
            break;

        case EGenericComponentType_VideoPostProcessor:
            err = p_filter->Initialize((TChar *)"AMBA");
            DASSERT_OK(err);
            break;

        case EGenericComponentType_AudioPostProcessor:
            err = p_filter->Initialize((TChar *)"FFMpeg");
            DASSERT_OK(err);
            break;

        case EGenericComponentType_Muxer:
            if (EMuxerModuleID_FFMpeg == mPersistMediaConfig.prefer_module_setting.mPreferedMuxerModule) {
                err = p_filter->Initialize((TChar *)"FFMpeg");
            } else if (EMuxerModuleID_PrivateTS == mPersistMediaConfig.prefer_module_setting.mPreferedMuxerModule) {
                err = p_filter->Initialize((TChar *)"TS");
            } else {
                err = p_filter->Initialize((TChar *)"TS");
            }
            DASSERT_OK(err);

            DASSERT(component->url);
            LOGM_INFO("init EGenericComponentType_Muxer, url=%s\n", component->url);
            err = p_filter->FilterProperty(EFilterPropertyType_update_sink_url, 1, component->url);
            DASSERT_OK(err);
            break;

        case EGenericComponentType_VideoRenderer:
            err = p_filter->Initialize((TChar *)"AMBA");
            DASSERT_OK(err);
            break;

        case EGenericComponentType_AudioRenderer:
            err = p_filter->Initialize((TChar *)"ALSA");
            DASSERT_OK(err);
            break;

        case EGenericComponentType_StreamingTransmiter:
            err = p_filter->Initialize((TChar *)"RTSP");
            DASSERT_OK(err);
            break;

        case EGenericComponentType_ConnecterPinMuxer:
            err = p_filter->Initialize(NULL);
            DASSERT_OK(err);
            break;

        default:
            LOGM_FATAL("unknown type %d\n", type);
            break;
    }

    return err;
}

EECode CActiveGenericEngine::initializeAllFilters(TGenericID pipeline_id)
{
    CIDoubleLinkedList::SNode *pnode = mListFilters.FirstNode();
    SComponent_Obsolete *p_component = NULL;
    IFilter *p_filter = NULL;
    EECode err = EECode_OK;

    if (!IS_VALID_COMPONENT_ID(pipeline_id)) {
        DASSERT(0 == pipeline_id);
        LOGM_INFO(" initialize all filters begin, filters number %d\n", mListFilters.NumberOfNodes());
        while (pnode) {
            p_component = (SComponent_Obsolete *)(pnode->p_context);
            DASSERT(p_component);
            DASSERT(p_component->p_filter);
            if (DUnlikely(!p_component->p_scur_pipeline)) {
                LOGM_NOTICE("p_component(%s) id 0x%08x do not have a pipeline?\n", gfGetComponentStringFromGenericComponentType(p_component->type), p_component->id);
            }
            DASSERT(EComponentState_not_alive == p_component->cur_state);

            p_filter = (IFilter *)p_component->p_filter;
            if (p_filter) {
                err = initializeFilter(p_component, p_component->type);
                DASSERT_OK(err);
                if (EECode_OK == err) {
                    p_component->cur_state = EComponentState_idle;
                } else {
                    LOGM_ERROR("initializeFilter(%s) fail, index %d, pipeline_id 0x%08x\n", gfGetComponentStringFromGenericComponentType(p_component->type), p_component->index, pipeline_id);
                    return err;
                }
            }
            pnode = mListFilters.NextNode(pnode);
        }
        LOGM_INFO(" initialize all filters end\n");
    } else {
        LOGM_INFO(" initialize all filters for pipeline 0x%08x begin, NumberOfNodes %d\n", pipeline_id, mListFilters.NumberOfNodes());
        while (pnode) {
            p_component = (SComponent_Obsolete *)(pnode->p_context);
            DASSERT(p_component);
            DASSERT(p_component->p_filter);
            if (DUnlikely(!p_component->p_scur_pipeline)) {
                LOGM_NOTICE("p_component(%s) id 0x%08x do not have a pipeline?\n", gfGetComponentStringFromGenericComponentType(p_component->type), p_component->id);
            } else {
                if (pipeline_id == p_component->p_scur_pipeline->pipeline_id) {
                    if (EComponentState_not_alive == p_component->cur_state) {
                        if (p_component->p_filter) {
                            err = initializeFilter(p_component, p_component->type);
                            DASSERT_OK(err);
                            if (err == EECode_OK) {
                                p_component->cur_state = EComponentState_idle;
                            }
                        }
                    } else {
                        DASSERT(EComponentState_idle == p_component->cur_state);
                    }
                }
            }
            pnode = mListFilters.NextNode(pnode);
        }
        LOGM_INFO(" initialize all filters for pipeline 0x%08x, NumberOfNodes %d end\n", pipeline_id, mListFilters.NumberOfNodes());
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::finalizeAllFilters(TGenericID pipeline_id)
{
    CIDoubleLinkedList::SNode *pnode = mListFilters.FirstNode();
    SComponent_Obsolete *p_component = NULL;
    IFilter *p_filter = NULL;
    EECode err = EECode_OK;

    if (!IS_VALID_COMPONENT_ID(pipeline_id)) {
        DASSERT(0 == pipeline_id);
        LOGM_INFO(" finalizeAllFilters begin, filters number %d\n", mListFilters.NumberOfNodes());
        while (pnode) {
            p_component = (SComponent_Obsolete *)(pnode->p_context);
            DASSERT(p_component);
            DASSERT(p_component->p_filter);
            if (DUnlikely(!p_component->p_scur_pipeline)) {
                LOGM_NOTICE("p_component(%s) id 0x%08x do not have a pipeline?\n", gfGetComponentStringFromGenericComponentType(p_component->type), p_component->id);
            }
            DASSERT(EComponentState_idle == p_component->cur_state);
            p_filter = (IFilter *)p_component->p_filter;
            if (p_filter) {
                err = p_filter->Finalize();
                DASSERT_OK(err);
            }
            pnode = mListFilters.NextNode(pnode);
        }
        LOGM_INFO(" finalizeAllFilters end\n");
    } else {
        LOGM_INFO(" finalizeAllFilters for pipeline 0x%08x begin, NumberOfNodes %d\n", pipeline_id, mListFilters.NumberOfNodes());
        while (pnode) {
            p_component = (SComponent_Obsolete *)(pnode->p_context);
            DASSERT(p_component);
            DASSERT(p_component->p_filter);
            if (DUnlikely(!p_component->p_scur_pipeline)) {
                LOGM_NOTICE("p_component(%s) id 0x%08x do not have a pipeline?\n", gfGetComponentStringFromGenericComponentType(p_component->type), p_component->id);
            } else {
                if (pipeline_id == p_component->p_scur_pipeline->pipeline_id) {
                    if (EComponentState_idle == p_component->cur_state) {
                        if (p_component->p_filter) {
                            err = p_filter->Finalize();
                            DASSERT_OK(err);
                        }
                    } else {
                        DASSERT(EComponentState_running == p_component->cur_state);
                    }
                }
            }
            pnode = mListFilters.NextNode(pnode);
        }
        LOGM_INFO(" finalizeAllFilters for pipeline 0x%08x, NumberOfNodes %d end\n", pipeline_id, mListFilters.NumberOfNodes());
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::exitAllFilters(TGenericID pipeline_id)
{
    CIDoubleLinkedList::SNode *pnode = mListFilters.FirstNode();
    SComponent *p_component = NULL;
    IFilter *p_filter = NULL;

    if (DLikely(!IS_VALID_COMPONENT_ID(pipeline_id))) {
        DASSERT(0 == pipeline_id);
        LOGM_INFO(" exit all fiters begin, filters number %d\n", mListFilters.NumberOfNodes());
        while (pnode) {
            p_component = (SComponent *)(pnode->p_context);
            DASSERT(p_component);
            DASSERT(p_component->p_filter);
            DASSERT(EComponentState_initialized == p_component->cur_state);
            p_filter = (IFilter *)p_component->p_filter;
            if (p_filter) {
                p_filter->Exit();
                p_component->cur_state = EComponentState_activated;
            }
            pnode = mListFilters.NextNode(pnode);
        }
        LOGM_INFO(" exit all fiters end\n");
    } else {
        LOGM_ERROR("exitAllFilters: TODO\n");
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::startAllFilters(TGenericID pipeline_id)
{
    CIDoubleLinkedList::SNode *pnode = mListFilters.FirstNode();
    SComponent_Obsolete *p_component = NULL;
    IFilter *p_filter = NULL;

    if (!IS_VALID_COMPONENT_ID(pipeline_id)) {
        DASSERT(0 == pipeline_id);
        LOGM_INFO(" start all fiters begin, filters number %d\n", mListFilters.NumberOfNodes());
        while (pnode) {
            p_component = (SComponent_Obsolete *)(pnode->p_context);
            DASSERT(p_component);
            DASSERT(p_component->p_filter);
            if (DUnlikely(!p_component->p_scur_pipeline)) {
                LOGM_NOTICE("p_component(%s) id 0x%08x do not have a pipeline?\n", gfGetComponentStringFromGenericComponentType(p_component->type), p_component->id);
            }
            DASSERT(EComponentState_running == p_component->cur_state);
            p_filter = (IFilter *)p_component->p_filter;
            if (p_filter) {
                p_filter->Start();
            }
            pnode = mListFilters.NextNode(pnode);
        }
        LOGM_INFO(" start all fiters end\n");
    } else {
        LOGM_INFO(" start all fiters for pipeline 0x%08x begin, NumberOfNodes %d\n", pipeline_id, mListFilters.NumberOfNodes());
        while (pnode) {
            p_component = (SComponent_Obsolete *)(pnode->p_context);
            DASSERT(p_component);
            DASSERT(p_component->p_filter);
            if (DUnlikely(!p_component->p_scur_pipeline)) {
                LOGM_NOTICE("p_component(%s) id 0x%08x do not have a pipeline?\n", gfGetComponentStringFromGenericComponentType(p_component->type), p_component->id);
            } else {
                if (pipeline_id == p_component->p_scur_pipeline->pipeline_id) {
                    if (EComponentState_running == p_component->cur_state) {
                        if (p_component->p_filter) {
                            ((IFilter *)(p_component->p_filter))->Start();
                        }
                    } else {
                        LOGM_ERROR("Start() need to be in EComponentState_running, cur state %d\n", p_component->cur_state);
                    }
                }
            }
            pnode = mListFilters.NextNode(pnode);
        }
        LOGM_INFO(" start all fiters for pipeline 0x%08x, NumberOfNodes %d end\n", pipeline_id, mListFilters.NumberOfNodes());
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::runAllFilters(TGenericID pipeline_id)
{
    CIDoubleLinkedList::SNode *pnode = mListFilters.FirstNode();
    SComponent_Obsolete *p_component = NULL;
    IFilter *p_filter = NULL;

    if (!IS_VALID_COMPONENT_ID(pipeline_id)) {
        DASSERT(0 == pipeline_id);
        LOGM_INFO(" run all fiters begin, filters number %d\n", mListFilters.NumberOfNodes());
        while (pnode) {
            p_component = (SComponent_Obsolete *)(pnode->p_context);
            DASSERT(p_component);
            DASSERT(p_component->p_filter);
            if (DUnlikely(!p_component->p_scur_pipeline)) {
                LOGM_NOTICE("p_component(%s) id 0x%08x do not have a pipeline?\n", gfGetComponentStringFromGenericComponentType(p_component->type), p_component->id);
            }
            DASSERT(EComponentState_idle == p_component->cur_state);
            if (!p_component->p_scur_pipeline) {
                LOGM_INFO("p_component(%s)->id 0x%08x, has NULL p_scur_pipeline\n", gfGetComponentStringFromGenericComponentType(DCOMPONENT_TYPE_FROM_GENERIC_ID(p_component->id)), p_component->id);
            }
            p_filter = (IFilter *)p_component->p_filter;
            if (p_filter) {
                p_filter->Run();
                p_component->cur_state = EComponentState_running;
            }
            pnode = mListFilters.NextNode(pnode);
        }
        LOGM_INFO(" run all fiters end\n");
    } else {
        LOGM_INFO(" run all fiters for pipeline 0x%08x begin, NumberOfNodes %d\n", pipeline_id, mListFilters.NumberOfNodes());
        while (pnode) {
            p_component = (SComponent_Obsolete *)(pnode->p_context);
            DASSERT(p_component);
            DASSERT(p_component->p_filter);
            if (DUnlikely(!p_component->p_scur_pipeline)) {
                LOGM_NOTICE("p_component(%s) id 0x%08x do not have a pipeline?\n", gfGetComponentStringFromGenericComponentType(p_component->type), p_component->id);
            } else {
                if (pipeline_id == p_component->p_scur_pipeline->pipeline_id) {
                    if (EComponentState_idle == p_component->cur_state) {
                        if (p_component->p_filter) {
                            ((IFilter *)(p_component->p_filter))->Run();
                            p_component->cur_state = EComponentState_running;
                        }
                    } else {
                        LOGM_ERROR("Run() need to be in EComponentState_idle, cur state %d\n", p_component->cur_state);
                    }
                }
            }
            pnode = mListFilters.NextNode(pnode);
        }
        LOGM_INFO(" run all fiters for pipeline 0x%08x, NumberOfNodes %d end\n", pipeline_id, mListFilters.NumberOfNodes());
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::stopAllFilters(TGenericID pipeline_id)
{
    CIDoubleLinkedList::SNode *pnode = mListFilters.FirstNode();
    SComponent_Obsolete *p_component = NULL;
    IFilter *p_filter = NULL;

    if (!IS_VALID_COMPONENT_ID(pipeline_id)) {
        DASSERT(0 == pipeline_id);
        LOGM_INFO(" stop all fiters begin, filters number %d\n", mListFilters.NumberOfNodes());
        while (pnode) {
            p_component = (SComponent_Obsolete *)(pnode->p_context);
            DASSERT(p_component);
            DASSERT(p_component->p_filter);
            //DASSERT(p_component->p_scur_pipeline);
            DASSERT(EComponentState_running == p_component->cur_state);
            if (!p_component->p_scur_pipeline) {
                LOGM_INFO("p_component(%s)->id 0x%08x, has NULL p_scur_pipeline\n", gfGetComponentStringFromGenericComponentType(DCOMPONENT_TYPE_FROM_GENERIC_ID(p_component->id)), p_component->id);
            }
            p_filter = (IFilter *)p_component->p_filter;
            if (p_filter) {
                p_filter->Stop();
                p_component->cur_state = EComponentState_idle;
            }
            pnode = mListFilters.NextNode(pnode);
        }
        LOGM_INFO(" stop all fiters end\n");
    } else {
        LOGM_INFO(" stop all fiters for pipeline 0x%08x begin, NumberOfNodes %d\n", pipeline_id, mListFilters.NumberOfNodes());
        while (pnode) {
            p_component = (SComponent_Obsolete *)(pnode->p_context);
            DASSERT(p_component);
            DASSERT(p_component->p_filter);
            DASSERT(p_component->p_scur_pipeline);
            if (!p_component->p_scur_pipeline) {
                LOGM_INFO("p_component(%s)->id 0x%08x, has NULL p_scur_pipeline\n", gfGetComponentStringFromGenericComponentType(DCOMPONENT_TYPE_FROM_GENERIC_ID(p_component->id)), p_component->id);
            }
            if (pipeline_id == p_component->p_scur_pipeline->pipeline_id) {
                if (EComponentState_running == p_component->cur_state) {
                    if (p_component->p_filter) {
                        ((IFilter *)(p_component->p_filter))->Stop();
                        p_component->cur_state = EComponentState_idle;
                    }
                } else {
                    DASSERT(EComponentState_idle == p_component->cur_state);
                }
            }
            pnode = mListFilters.NextNode(pnode);
        }
        LOGM_INFO(" stop all fiters for pipeline 0x%08x end, NumberOfNodes %d\n", pipeline_id, mListFilters.NumberOfNodes());
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::StartProcess()
{
    EECode err = EECode_OK;
    SCMD cmd;

    cmd.code = ECMDType_Start;
    err = mpWorkQ->SendCmd(cmd);

    if (DLikely(EECode_OK == err)) {
        mbEnginePipelineRunning = 1;
    } else {
        LOGM_ERROR("StartProcess fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
    }

    return err;
}

EECode CActiveGenericEngine::resumeSuspendPlaybackPipelines(TUint start_index, TUint end_index, TComponentIndex focus_input_index)
{
    SGenericPipeline *p = NULL;
    SComponent_Obsolete *p_component = NULL;
    TUint i = 0;
    EECode err = EECode_OK;
    //LOGM_DEBUG("[DEBUG]: resumeSuspendPlaybackPipelines %d, %d, %d\n", start_index, end_index, focus_input_index);

    for (i = start_index; i < end_index; i++) {
        p = queryPlaybackPipeline(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Playback, i));
        if (DLikely(p)) {
            //LOGM_DEBUG("i %d, p->occupied_dsp_dec_instance %d, p->pb_video_suspended %d\n", i, p->occupied_dsp_dec_instance, p->pb_video_suspended);
            if (DLikely(p->occupied_dsp_dec_instance)) {
                p_component = queryFilter(p->pb_video_decoder_id);
                DASSERT(p_component);
                if (p_component) {
                    if (DLikely(p_component->p_filter)) {
                        if (p->pb_video_suspended) {
                            //LOGM_DEBUG("ResumeSuspend video decoder for pipeline index %d, focus_input_index %d\n", i, focus_input_index);
                            err = p_component->p_filter->ResumeSuspend(focus_input_index);
                            if (EECode_OK != err) {
                                return err;
                            }
                            p->video_stopped = 0;
                            p->pb_video_suspended = 0;
                        } else {
                            LOGM_INFO("[SwitchInput] of video decoder for pipeline index %d, focus_input_index %d\n", i, focus_input_index);
                            err = p_component->p_filter->SwitchInput(focus_input_index);
                            if (EECode_OK != err) {
                                return err;
                            }
                        }
                    } else {
                        LOGM_ERROR("no filter, pipeline %d\n", i);
                    }
                }
            } else {
                LOGM_ERROR("not occupied dsp dec instance %d\n", i);
            }
        } else {
            LOGM_ERROR("no playback pipeline %d\n", i);
        }
    }
    return EECode_OK;
}

void CActiveGenericEngine::suspendPlaybackPipelines(TUint start_index, TUint end_index, TUint release_context)
{
    SGenericPipeline *p = NULL;
    SComponent_Obsolete *p_component = NULL;
    TUint i = 0;

    //LOGM_DEBUG("[DEBUG]: suspendPlaybackPipelines %d, %d, %d\n", start_index, end_index, release_context);

    for (i = start_index; i < end_index; i++) {
        p = queryPlaybackPipeline(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Playback, i));
        DASSERT(p);
        if (DLikely(p)) {
            //LOGM_DEBUG("i %d, p->occupied_dsp_dec_instance %d, p->pb_video_suspended %d\n", i, p->occupied_dsp_dec_instance, p->pb_video_suspended);
            if (p->occupied_dsp_dec_instance) {
                p_component = queryFilter(p->pb_video_decoder_id);
                if (DLikely(p_component)) {
                    DASSERT(p_component->p_filter);
                    if (DLikely(p_component->p_filter)) {
                        if (!p->pb_video_suspended) {
                            //LOGM_DEBUG("Suspend video decoder for pipeline index %d, release_context %d\n", i, release_context);
                            p_component->p_filter->Suspend(release_context);
                            p->video_stopped = 1;
                            p->pb_video_suspended = 1;
                        }
                    } else {
                        LOGM_ERROR("NULL p_filter\n");
                    }
                } else {
                    LOGM_ERROR("NULL p_component\n");
                }
            }
        } else {
            LOGM_ERROR("no playback pipeline %d\n", i);
        }
    }
}

EECode CActiveGenericEngine::resumeSuspendPlaybackPipelines(TUint start_index_for_vid_decode, TUint end_index_for_vid_decode, TUint start_index_for_vid_demux, TUint end_index_for_vid_demux, TComponentIndex focus_input_index)
{
    SGenericPipeline *p = NULL;
    SComponent_Obsolete *p_component = NULL;
    TUint i = 0;
    EECode err = EECode_OK;
    //LOGM_DEBUG("[DEBUG]: resumeSuspendPlaybackPipelines %d, %d, %d\n", start_index, end_index, focus_input_index);

    for (i = start_index_for_vid_decode; i < end_index_for_vid_decode; i++) {
        p = queryPlaybackPipeline(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Playback, i));
        if (DLikely(p)) {
            //LOGM_DEBUG("i %d, p->occupied_dsp_dec_instance %d, p->pb_video_suspended %d\n", i, p->occupied_dsp_dec_instance, p->pb_video_suspended);
            if (DLikely(p->occupied_dsp_dec_instance)) {
                p_component = queryFilter(p->pb_video_decoder_id);
                DASSERT(p_component);
                if (p_component) {
                    if (DLikely(p_component->p_filter)) {
                        if (p->pb_video_suspended) {
                            //LOGM_DEBUG("ResumeSuspend video decoder for pipeline index %d, focus_input_index %d\n", i, focus_input_index);
                            err = p_component->p_filter->ResumeSuspend(focus_input_index);
                            if (EECode_OK != err) {
                                return err;
                            }
                            p->video_stopped = 0;
                            p->pb_video_suspended = 0;
                        } else {
                            LOGM_INFO("[SwitchInput] of video decoder for pipeline index %d, focus_input_index %d\n", i, focus_input_index);
                            err = p_component->p_filter->SwitchInput(focus_input_index);
                            if (EECode_OK != err) {
                                return err;
                            }
                        }
                    } else {
                        LOGM_ERROR("no filter, pipeline %d\n", i);
                        return EECode_InternalLogicalBug;
                    }
                } else {
                    LOGM_ERROR("no component pipeline %d\n", i);
                    return EECode_InternalLogicalBug;
                }
            } else {
                LOGM_ERROR("not occupied dsp dec instance pipeline %d\n", i);
                return EECode_InternalLogicalBug;
            }
        } else {
            LOGM_ERROR("no playback pipeline %d\n", i);
            return EECode_InternalLogicalBug;
        }
    }

    //demuxer resume suspend operation
    for (i = start_index_for_vid_demux; i < end_index_for_vid_demux; i++) {
        p = queryPlaybackPipeline(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Playback, i));
        if (DLikely(p)) {
            p_component = queryFilter(p->pb_source_id);
            DASSERT(p_component);
            if (DLikely(p_component)) {
                if (DLikely(p_component->p_filter)) {
                    LOGM_ALWAYS("ResumeSuspend demuxer for pipeline index %d\n", i);
                    err = p_component->p_filter->ResumeSuspend();
                    if (EECode_OK != err) {
                        LOGM_ERROR("ResumeSuspend demuxer for pipeline index %d failed\n", i);
                        //return err;//TODO: here not return err for rtsp_demuxer not implement suspend/resume_suspend
                    }
                } else {
                    LOGM_ERROR("no filter, pipeline %d\n", i);
                    return EECode_InternalLogicalBug;
                }
            } else {
                LOGM_ERROR("no component, pipeline %d\n", i);
                return EECode_InternalLogicalBug;
            }
        } else {
            LOGM_ERROR("no playback pipeline %d\n", i);
            return EECode_InternalLogicalBug;
        }
    }
    return EECode_OK;
}

void CActiveGenericEngine::suspendPlaybackPipelines(TUint start_index_for_vid_decode, TUint end_index_for_vid_decode, TUint start_index_for_vid_demux, TUint end_index_for_vid_demux, TUint release_context)
{
    SGenericPipeline *p = NULL;
    SComponent_Obsolete *p_component = NULL;
    TUint i = 0;
    EECode err = EECode_OK;

    //LOGM_DEBUG("[DEBUG]: suspendPlaybackPipelines %d, %d, %d\n", start_index, end_index, release_context);

    for (i = start_index_for_vid_decode; i < end_index_for_vid_decode; i++) {
        p = queryPlaybackPipeline(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Playback, i));
        DASSERT(p);
        if (DLikely(p)) {
            //LOGM_DEBUG("i %d, p->occupied_dsp_dec_instance %d, p->pb_video_suspended %d\n", i, p->occupied_dsp_dec_instance, p->pb_video_suspended);
            if (p->occupied_dsp_dec_instance) {
                p_component = queryFilter(p->pb_video_decoder_id);
                if (DLikely(p_component)) {
                    DASSERT(p_component->p_filter);
                    if (DLikely(p_component->p_filter)) {
                        if (!p->pb_video_suspended) {
                            //LOGM_DEBUG("Suspend video decoder for pipeline index %d, release_context %d\n", i, release_context);
                            p_component->p_filter->Suspend(release_context);
                            p->video_stopped = 1;
                            p->pb_video_suspended = 1;
                        }
                    } else {
                        LOGM_ERROR("NULL p_filter\n");
                    }
                } else {
                    LOGM_ERROR("NULL p_component\n");
                }
            }
        } else {
            LOGM_ERROR("no playback pipeline %d\n", i);
        }
    }

    //demuxer suspend operation
    for (i = start_index_for_vid_demux; i < end_index_for_vid_demux; i++) {
        p = queryPlaybackPipeline(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Playback, i));
        DASSERT(p);
        if (DLikely(p)) {
            p_component = queryFilter(p->pb_source_id);
            if (DLikely(p_component)) {
                DASSERT(p_component->p_filter);
                if (DLikely(p_component->p_filter)) {
                    LOGM_ALWAYS("Suspend demuxer for pipeline index %d, release_context %d\n", i, release_context);
                    err = p_component->p_filter->Suspend(release_context);
                    if (EECode_OK != err) {
                        LOGM_ERROR("Suspend demuxer for pipeline index %d failed\n", i);
                    }
                } else {
                    LOGM_ERROR("NULL p_filter\n");
                }
            } else {
                LOGM_ERROR("NULL p_component\n");
            }
        } else {
            LOGM_ERROR("no playback pipeline %d\n", i);
        }
    }
}
void CActiveGenericEngine::autoStopBackgroudVideoPlayback()
{
    TUint suspend_pb_pipeline_start_index = 0;
    TUint suspend_pb_pipeline_end_index = 0;

    LOGM_NOTICE("autoStopBackgroudVideoPlayback, mPresetPlaybackMode %d\n", mPresetPlaybackMode);

    switch (mPresetPlaybackMode) {

        case EPlaybackMode_1x1080p_4xD1:
            suspend_pb_pipeline_start_index = 4;
            suspend_pb_pipeline_end_index = 5;
            break;

        case EPlaybackMode_1x1080p_6xD1:
            suspend_pb_pipeline_start_index = 6;
            suspend_pb_pipeline_end_index = 7;
            break;

        case EPlaybackMode_1x1080p_8xD1:
            suspend_pb_pipeline_start_index = 8;
            suspend_pb_pipeline_end_index = 9;
            break;

        case EPlaybackMode_1x1080p_9xD1:
            suspend_pb_pipeline_start_index = 9;
            suspend_pb_pipeline_end_index = 10;
            break;

        default:
            return;
            break;
    }

    LOGM_NOTICE("autoStopBackgroudVideoPlayback, mPresetPlaybackMode %d, suspend_pb_pipeline_start_index %d, suspend_pb_pipeline_end_index %d, demuxer suspend %d~%d\n", mPresetPlaybackMode, suspend_pb_pipeline_start_index, suspend_pb_pipeline_end_index, suspend_pb_pipeline_start_index, mnPlaybackPipelinesNumber);
    suspendPlaybackPipelines(suspend_pb_pipeline_start_index, suspend_pb_pipeline_end_index, suspend_pb_pipeline_start_index, mnPlaybackPipelinesNumber, IFilter::EReleaseFlag_None);
}

EECode CActiveGenericEngine::initialSetting()
{
    EECode err = EECode_OK;

    //some intial settings
    if (EDSPOperationMode_MultipleWindowUDEC == mRequestDSPMode
            || EDSPOperationMode_MultipleWindowUDEC_Transcode == mRequestDSPMode) {
        DASSERT(mpUniqueVideoPostProcessor);
        if (mpUniqueVideoPostProcessor) {
            err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_global_setting, 1, (void *)&mInitalVideoPostPGlobalSetting);
            DASSERT_OK(err);
            err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_initial_configure, 1, (void *)&mInitalVideoPostPDisplayLayout);
            DASSERT_OK(err);
        }
    }

    autoStopBackgroudVideoPlayback();

    return err;
}

EECode CActiveGenericEngine::doStartProcess()
{
    EECode err;

    DASSERT(true == mbGraghBuilt);
    if (false == mbGraghBuilt) {
        LOGM_ERROR("processing gragh not built yet\n");
        return EECode_BadState;
    }

    LOGM_INFO("doStartProcess(), start\n");

    if (EEngineState_idle == msState) {
        //start running
        LOGM_INFO("doStartProcess(), before initializeAllFilters(0)\n");
        err = initializeAllFilters(0);
        if (err != EECode_OK) {
            LOGM_ERROR("initializeAllFilters() fail, err %d\n", err);
            return err;
        }
        LOGM_INFO("doStartProcess(), after initializeAllFilters(0), before runAllFilters(0)\n");

        err = runAllFilters(0);
        if (err != EECode_OK) {
            LOGM_ERROR("runAllFilters() fail, err %d\n", err);
            return err;
        }
        LOGM_INFO("doStartProcess(), after runAllFilters(0), before startAllFilters(0)\n");

        err = startAllFilters(0);
        if (err != EECode_OK) {
            LOGM_ERROR("startAllFilters() fail, err %d\n", err);
            stopAllFilters(0);
            exitAllFilters(0);
            return err;
        }
        LOGM_INFO("doStartProcess(), after startAllFilters(0)\n");

        msState = EEngineState_running;
    } else if (EEngineState_running == msState) {
        //already running
        LOGM_ERROR("engine is already running\n");
    } else {
        //bad state
        LOGM_ERROR("BAD engine state %d\n", msState);
        return EECode_BadState;
    }

    err = initialSetting();
    initializeSyncStatus();
    LOGM_INFO("doStartProcess(), after initialSetting()\n");

    return EECode_OK;
}

EECode CActiveGenericEngine::StopProcess()
{
    EECode err = EECode_OK;
    SCMD cmd;

    cmd.code = ECMDType_Stop;
    err = mpWorkQ->SendCmd(cmd);

    if (DLikely(EECode_OK == err)) {
        mbEnginePipelineRunning = 0;
    } else {
        LOGM_ERROR("StopProcess fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
    }

    return err;
}

EECode CActiveGenericEngine::SetAppMsgCallback(void (*MsgProc)(void *, SMSG &), void *context)
{
    return inherited::SetAppMsgCallback(MsgProc, context);
}

EECode CActiveGenericEngine::doStopProcess()
{
    if (false == mbGraghBuilt) {
        LOGM_ERROR("gragh is not built yet\n");
        return EECode_OK;
    }

    if (EEngineState_running == msState) {
        mPersistMediaConfig.app_start_exit = 1;
        stopServers();
        stopAllFilters(0);
        exitAllFilters(0);
        finalizeAllFilters(0);
        if (mpDSPAPI) {
            mpDSPAPI->EnterDSPMode(EDSPOperationMode_IDLE);
        }
        msState = EEngineState_idle;
    } else {
        LOGM_ERROR("BAD state %d\n", msState);
        return EECode_BadState;
    }

    return EECode_OK;
}

void CActiveGenericEngine::doPrintCurrentStatus(TGenericID id, TULong print_flag)
{
    CIDoubleLinkedList::SNode *pnode = mListFilters.FirstNode();
    SComponent_Obsolete *p_component = NULL;

    if (print_flag & DPrintFlagBit_logConfigure) {
        gfPrintLogConfig(&gmModuleLogConfig[0], ELogModuleListEnd);
    }

    if (print_flag & DPrintFlagBit_dataPipeline) {
        if (!IS_VALID_COMPONENT_ID(id)) {
            DASSERT(0 == id);
            while (pnode) {
                p_component = (SComponent_Obsolete *)(pnode->p_context);
                DASSERT(p_component);
                if (p_component) {
                    p_component->p_filter->PrintStatus();
                } else {
                    DASSERT("NULL pointer here, something would be wrong.\n");
                }
                pnode = mListFilters.NextNode(pnode);
            }
        } else {
            while (pnode) {
                p_component = (SComponent_Obsolete *)(pnode->p_context);
                DASSERT(p_component);
                if (p_component) {
                    DASSERT(p_component->p_scur_pipeline);
                    if (id == p_component->p_scur_pipeline->pipeline_id) {
                        p_component->p_filter->PrintStatus();
                    }
                } else {
                    DASSERT("NULL pointer here, something would be wrong.\n");
                }
                pnode = mListFilters.NextNode(pnode);
            }
        }

        if (mpClockSource) {
            mpClockSource->PrintStatus();
        }

        if (mpClockManager) {
            mpClockManager->PrintStatus();
        }

        if (mpSystemClockReference) {
            mpSystemClockReference->PrintStatus();
        }
    }

}

EECode CActiveGenericEngine::createGraph(void)
{
    CIDoubleLinkedList::SNode *pnode = mListFilters.FirstNode();
    SComponent_Obsolete *p_component = NULL;
    SConnection_Obsolete *p_connection = NULL;
    IFilter *pfilter = NULL;
    EECode err;
    TUint uppin_index, uppin_sub_index, downpin_index;

    LOGM_INFO("CActiveGenericEngine::createGraph start.\n");

    //create all filters
    while (pnode) {
        p_component = (SComponent_Obsolete *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            LOGM_INFO("create component, type %d, %s, index %d\n", p_component->type, gfGetComponentStringFromGenericComponentType(p_component->type), p_component->index);
            pfilter = gfFilterFactory(__filterTypeFromGenericComponentType(p_component->type, mPersistMediaConfig.number_scheduler_video_decoder, mPersistMediaConfig.number_scheduler_io_writer), (const SPersistMediaConfig *)&mPersistMediaConfig, (IMsgSink *)this, (TU32)p_component->index, mPersistMediaConfig.engine_config.filter_version);
            DASSERT(pfilter);
            if (pfilter) {
                p_component->p_filter = pfilter;
            } else {
                LOGM_FATAL("CActiveGenericEngine::createGraph gfFilterFactory(type %d) fail.\n", p_component->type);
                return EECode_InternalLogicalBug;
            }
            setUniqueComponent(p_component->type, pfilter, p_component);
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mListFilters.NextNode(pnode);
    }

    LOGM_INFO("CActiveGenericEngine::createGraph filters construct done, start connection.\n");

    pnode = mListConnections.FirstNode();
    //implement connetions
    while (pnode) {
        p_connection = (SConnection_Obsolete *)(pnode->p_context);
        DASSERT(p_connection);
        if (p_connection) {
            uppin_sub_index = 0;
            //debug assert
            DASSERT((p_connection->up) && (p_connection->down));
            //DASSERT(p_connection->up_id == ((TU32)DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(p_connection->up->type, p_connection->up->index)));
            //DASSERT(p_connection->down_id == ((TU32)DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(p_connection->down->type, p_connection->down->index)));
            LOGM_INFO("Start connect, upper: type(%s) %d, index %d, down: type(%s) %d, index %d\n", gfGetComponentStringFromGenericComponentType(p_connection->up->type), p_connection->up->type, p_connection->up->index, gfGetComponentStringFromGenericComponentType(p_connection->down->type), p_connection->down->type, p_connection->down->index);

            DASSERT((StreamType_Video == p_connection->pin_type) || (StreamType_Audio == p_connection->pin_type));
            err = p_connection->up->p_filter->AddOutputPin(uppin_index, uppin_sub_index, p_connection->pin_type);
            DASSERT(EECode_OK == err);
            err = p_connection->down->p_filter->AddInputPin(downpin_index, p_connection->pin_type);
            DASSERT(EECode_OK == err);

            if ((err = Connect(p_connection->up->p_filter, uppin_index, p_connection->down->p_filter, downpin_index, uppin_sub_index)) != EECode_OK) {
                LOGM_ERROR("Connect failed\n");
                return err;
            }

            p_connection->up_pin_index = (TU8)uppin_index;
            p_connection->down_pin_index = (TU8)downpin_index;
            p_connection->up_pin_sub_index = (TU8)uppin_sub_index;
            LOGM_INFO("Connect success, up index %d, sub index %d, down index %d\n", uppin_index, uppin_sub_index, downpin_index);
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mListConnections.NextNode(pnode);
    }

    pnode = mListProxys.FirstNode();
    while (pnode) {
        p_component = (SComponent_Obsolete *)(pnode->p_context);
        DASSERT(p_component);
        if (DLikely(p_component)) {

            switch (p_component->type) {

                case EGenericComponentType_StreamingServer:
                    DASSERT(EGenericComponentType_StreamingServer == DCOMPONENT_TYPE_FROM_GENERIC_ID(p_component->id));
                    break;

                case EGenericComponentType_StreamingContent: {
                        TComponentIndex index = 0;

                        DASSERT(EGenericComponentType_StreamingContent == DCOMPONENT_TYPE_FROM_GENERIC_ID(p_component->id));
                        index = DCOMPONENT_INDEX_FROM_GENERIC_ID(p_component->id);
                        if (DLikely(index < mnComponentProxysNumbers[EGenericComponentType_StreamingContent - EGenericComponentType_TAG_proxy_start])) {
                            //SStreamingSessionContent* p_session_content = mpStreamingContent + index;
                            CIDoubleLinkedList::SNode *pnode_tmp = NULL;
#if 0
                            TComponentIndex track_index[4] = {0};
                            //TU8 pintype[4] = {StreamType_Invalid};
                            TComponentIndex number_of_track = 0;
                            if (DLikely(p_component->streaming_video_source == p_component->streaming_audio_source)) {

                                while ((p_connection = queryConnection(p_component->streaming_video_source, p_component->streaming_transmiter_id, pnode_tmp))) {
                                    if (DUnlikely(number_of_track > 3)) {
                                        LOGM_WARN("number_of_track %d?\n", number_of_track);
                                        break;
                                    }
                                    track_index[number_of_track] = p_connection->down_pin_index;
                                    //pintype[number_of_track] = p_connection->pin_type;
                                    number_of_track ++;
                                }
                                if (DLikely(number_of_track)) {
                                    setupStreamingContent(p_component->id, p_component->streaming_server_id, p_component->streaming_transmiter_id, p_component->url, number_of_track, track_index[0], track_index[1], track_index[2]);
                                } else {
                                    LOGM_FATAL("internal logic bug\n");
                                }
                            } else {
                                pnode_tmp = NULL;
                                p_connection = queryConnection(p_component->streaming_video_source, p_component->streaming_transmiter_id, pnode_tmp, StreamType_Video);
                                if (p_connection) {
                                    track_index[0] = p_connection->down_pin_index;
                                    //pintype[1] = p_connection->pin_type;
                                    number_of_track ++;
                                }
                                pnode_tmp = NULL;
                                p_connection = queryConnection(p_component->streaming_audio_source, p_component->streaming_transmiter_id, pnode_tmp, StreamType_Audio);
                                if (p_connection) {
                                    track_index[1] = p_connection->down_pin_index;
                                    //pintype[1] = p_connection->pin_type;
                                    number_of_track ++;
                                }
                                if (DLikely(number_of_track)) {
                                    setupStreamingContent(p_component->id, p_component->streaming_server_id, p_component->streaming_transmiter_id, p_component->url, number_of_track, track_index[0], track_index[1], track_index[2]);
                                } else {
                                    LOGM_FATAL("internal logic bug\n");
                                }
                            }
#else
                            TComponentIndex video_track_index = 0, audio_track_index = 0;
                            TU8 has_audio = 0, has_video = 0;

                            p_connection = queryConnection(p_component->streaming_video_source, p_component->streaming_transmiter_id, pnode_tmp, StreamType_Video);
                            LOGM_INFO("[streaming content]: p_component->id 0x%08x, streaming_video_source 0x%08x, streaming_audio_source 0x%08x, streaming_server_id 0x%08x\n", p_component->id, p_component->streaming_video_source, p_component->streaming_audio_source, p_component->streaming_server_id);
                            LOGM_INFO("[streaming content]: p_connection %p, url %s\n", p_connection, p_component->url);
                            if (p_connection) {
                                video_track_index = p_connection->down_pin_index;
                                has_video = 1;
                                if (p_component->url) {
                                    LOGM_INFO("streaming content %s, id 0x%08x\n", p_component->url, p_component->streaming_transmiter_id);
                                } else {
                                    LOGM_INFO("streaming content NULL, id 0x%08x\n", p_component->streaming_transmiter_id);
                                }
                                LOGM_INFO("     video track index %d\n", video_track_index);
                            } else {
                                LOGM_INFO("p_component->url %s, id 0x%08x: no video content\n", p_component->url, p_component->id);
                            }
                            pnode_tmp = NULL;
                            p_connection = queryConnection(p_component->streaming_audio_source, p_component->streaming_transmiter_id, pnode_tmp, StreamType_Audio);
                            if (p_connection) {
                                audio_track_index = p_connection->down_pin_index;
                                has_audio = 1;
                                if (p_component->url) {
                                    LOGM_INFO("streaming content %s, id 0x%08x\n", p_component->url, p_component->streaming_transmiter_id);
                                } else {
                                    LOGM_INFO("streaming content NULL, id 0x%08x\n", p_component->streaming_transmiter_id);
                                }
                                LOGM_INFO("     audio track index %d\n", video_track_index);
                            } else {
                                LOGM_INFO("p_component->url %s, id 0x%08x: no audio content\n", p_component->url, p_component->id);
                            }

                            if (DLikely(has_video || has_audio)) {
                                setupStreamingContent(p_component->id, p_component->streaming_server_id, p_component->streaming_transmiter_id, p_component->url, has_video, has_audio, video_track_index, audio_track_index);
                            } else {
                                LOGM_FATAL("internal logic bug, has_video %d, has_audio %d\n", has_video, has_audio);
                            }
#endif
                        } else {
                            LOGM_FATAL("BAD ID 0x%08x, index %d, max %d\n", p_component->id, index, mnComponentProxysNumbers[EGenericComponentType_StreamingContent - EGenericComponentType_TAG_proxy_start]);
                        }
                    }
                    break;

                case EGenericComponentType_CloudServer:
                    DASSERT(EGenericComponentType_CloudServer == DCOMPONENT_TYPE_FROM_GENERIC_ID(p_component->id));
                    break;

                case EGenericComponentType_CloudReceiverContent: {
                        DASSERT(EGenericComponentType_CloudReceiverContent == DCOMPONENT_TYPE_FROM_GENERIC_ID(p_component->id));

                        TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(p_component->id);
                        if (DLikely(index < mnComponentProxysNumbers[EGenericComponentType_CloudReceiverContent - EGenericComponentType_TAG_proxy_start])) {
                            setupCloudReceiverContent(p_component->id, p_component->cloud_server_id, p_component->cloud_receiver_id, p_component->url);
                        } else {
                            LOGM_FATAL("BAD ID 0x%08x, index %d, max %d\n", p_component->id, index, mnComponentProxysNumbers[EGenericComponentType_CloudReceiverContent - EGenericComponentType_TAG_proxy_start]);
                        }
                    }
                    break;

                default:
                    LOGM_FATAL("BAD type %d, id 0x%08x\n", p_component->type, p_component->id);
                    break;
            }

        }
        pnode = mListProxys.NextNode(pnode);
    }

    //auto complete staff
    //streaming
    if (mbAutoNamingRTSPContextTag && mpUniqueRTSPStreamingTransmiter) {
        LOGM_INFO("start auto build streaming related.\n");
        LOGM_FATAL("TO DO\n");
    }

    LOGM_INFO("CActiveGenericEngine::createGraph done.\n");
    mbGraghBuilt = true;

    return EECode_OK;
}

EECode CActiveGenericEngine::clearGraph(void)
{
    CIDoubleLinkedList::SNode *pnode = mListFilters.FirstNode();
    SComponent_Obsolete *p_component = NULL;
    SConnection_Obsolete *p_connection = NULL;

    LOGM_INFO("clearGraph start.\n");

    //delete all filters
    while (pnode) {
        p_component = (SComponent_Obsolete *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            LOGM_INFO("destroy component, type %d, index %d\n", p_component->type, p_component->index);
            deleteComponent(p_component);
            pnode->p_context = NULL;
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mListFilters.NextNode(pnode);
    }

    //delete all proxys
    pnode = mListProxys.FirstNode();
    while (pnode) {
        p_component = (SComponent_Obsolete *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            LOGM_INFO("destroy component, type %d, index %d\n", p_component->type, p_component->index);
            deleteComponent(p_component);
            pnode->p_context = NULL;
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mListProxys.NextNode(pnode);
    }

    //delete all connections
    pnode = mListConnections.FirstNode();
    while (pnode) {
        p_connection = (SConnection_Obsolete *)(pnode->p_context);
        DASSERT(p_connection);
        if (p_connection) {
            LOGM_INFO("destroy connection, pin type %d, id %08x\n", p_connection->pin_type, p_connection->connection_id);
            deleteConnection(p_connection);
            pnode->p_context = NULL;
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mListConnections.NextNode(pnode);
    }

    LOGM_INFO("clearGraph end.\n");

    return EECode_OK;
}

EECode CActiveGenericEngine::createSchedulers(void)
{
    TUint i = 0, number = 0;

    DASSERT(!mbSchedulersCreated);
    if (!mbSchedulersCreated) {

        if (mPersistMediaConfig.number_scheduler_network_reciever) {
            number = mPersistMediaConfig.number_scheduler_network_reciever;
            DASSERT(number <= DMaxSchedulerGroupNumber);
            if (number > DMaxSchedulerGroupNumber) {
                number = DMaxSchedulerGroupNumber;
            }

            for (i = 0; i < number; i ++) {
                DASSERT(!mPersistMediaConfig.p_scheduler_network_reciever[i]);
                LOGM_INFO("p_scheduler_network_reciever, gfSchedulerFactory(ESchedulerType_PriorityPreemptive, %d)\n", i);
                mPersistMediaConfig.p_scheduler_network_reciever[i] = gfSchedulerFactory(ESchedulerType_Preemptive, i);
                //mPersistMediaConfig.p_scheduler_network_reciever[i] = gfSchedulerFactory(ESchedulerType_PriorityPreemptive, i);
                if (mPersistMediaConfig.p_scheduler_network_reciever[i]) {
                    mPersistMediaConfig.p_scheduler_network_reciever[i]->StartScheduling();
                }
            }
        }

        mPersistMediaConfig.number_scheduler_network_tcp_reciever = 1;//hard code here
        if (mPersistMediaConfig.number_scheduler_network_tcp_reciever) {
            number = mPersistMediaConfig.number_scheduler_network_tcp_reciever;
            DASSERT(number <= DMaxSchedulerGroupNumber);
            if (number > DMaxSchedulerGroupNumber) {
                number = DMaxSchedulerGroupNumber;
            }

            for (i = 0; i < number; i ++) {
                DASSERT(!mPersistMediaConfig.p_scheduler_network_tcp_reciever[i]);
                LOGM_INFO("p_scheduler_network_tcp_reciever, gfSchedulerFactory(ESchedulerType_PriorityPreemptive, %d)\n", i);
                mPersistMediaConfig.p_scheduler_network_tcp_reciever[i] = gfSchedulerFactory(ESchedulerType_Preemptive, i);
                //mPersistMediaConfig.p_scheduler_network_reciever[i] = gfSchedulerFactory(ESchedulerType_PriorityPreemptive, i);
                if (mPersistMediaConfig.p_scheduler_network_tcp_reciever[i]) {
                    mPersistMediaConfig.p_scheduler_network_tcp_reciever[i]->StartScheduling();
                }
            }
        }

        if (mPersistMediaConfig.number_scheduler_network_sender) {
            number = mPersistMediaConfig.number_scheduler_network_sender;
            DASSERT(number <= DMaxSchedulerGroupNumber);
            if (number > DMaxSchedulerGroupNumber) {
                number = DMaxSchedulerGroupNumber;
            }

            for (i = 0; i < number; i ++) {
                DASSERT(!mPersistMediaConfig.p_scheduler_network_sender[i]);
                LOGM_INFO("p_scheduler_network_sender, gfSchedulerFactory(ESchedulerType_RunRobin, %d)\n", i);
                mPersistMediaConfig.p_scheduler_network_sender[i] = gfSchedulerFactory(ESchedulerType_RunRobin, i);
                if (mPersistMediaConfig.p_scheduler_network_sender[i]) {
                    mPersistMediaConfig.p_scheduler_network_sender[i]->StartScheduling();
                }
            }
        }

        if (mPersistMediaConfig.number_scheduler_io_reader) {
            number = mPersistMediaConfig.number_scheduler_io_reader;
            DASSERT(number <= DMaxSchedulerGroupNumber);
            if (number > DMaxSchedulerGroupNumber) {
                number = DMaxSchedulerGroupNumber;
            }

            for (i = 0; i < number; i ++) {
                DASSERT(!mPersistMediaConfig.p_scheduler_io_reader[i]);
                LOGM_INFO("p_scheduler_io_reader, gfSchedulerFactory(ESchedulerType_RunRobin, %d)\n", i);
                mPersistMediaConfig.p_scheduler_io_reader[i] = gfSchedulerFactory(ESchedulerType_RunRobin, i);
                if (mPersistMediaConfig.p_scheduler_io_reader[i]) {
                    mPersistMediaConfig.p_scheduler_io_reader[i]->StartScheduling();
                }
            }
        }

        if (mPersistMediaConfig.number_scheduler_io_writer) {
            number = mPersistMediaConfig.number_scheduler_io_writer;
            DASSERT(number <= DMaxSchedulerGroupNumber);
            if (number > DMaxSchedulerGroupNumber) {
                number = DMaxSchedulerGroupNumber;
            }

            for (i = 0; i < number; i ++) {
                DASSERT(!mPersistMediaConfig.p_scheduler_io_writer[i]);
                LOGM_INFO("p_scheduler_io_writer, gfSchedulerFactory(ESchedulerType_RunRobin, %d)\n", i);
                mPersistMediaConfig.p_scheduler_io_writer[i] = gfSchedulerFactory(ESchedulerType_RunRobin, i);
                if (mPersistMediaConfig.p_scheduler_io_writer[i]) {
                    mPersistMediaConfig.p_scheduler_io_writer[i]->StartScheduling();
                }
            }
        }

        if (mPersistMediaConfig.number_scheduler_video_decoder) {
            number = mPersistMediaConfig.number_scheduler_video_decoder;
            DASSERT(number <= DMaxSchedulerGroupNumber);
            if (number > DMaxSchedulerGroupNumber) {
                number = DMaxSchedulerGroupNumber;
            }

            for (i = 0; i < number; i ++) {
                DASSERT(!mPersistMediaConfig.p_scheduler_video_decoder[i]);
                LOGM_INFO("p_scheduler_video_decoder, gfSchedulerFactory(ESchedulerType_RunRobin, %d)\n", i);
                mPersistMediaConfig.p_scheduler_video_decoder[i] = gfSchedulerFactory(ESchedulerType_RunRobin, i);
                if (mPersistMediaConfig.p_scheduler_video_decoder[i]) {
                    mPersistMediaConfig.p_scheduler_video_decoder[i]->StartScheduling();
                }
            }
        }

        mbSchedulersCreated = 1;
    } else {
        LOGM_ERROR("already created?\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::destroySchedulers(void)
{
    TUint i = 0;

    DASSERT(mbSchedulersCreated);

    for (i = 0; i < DMaxSchedulerGroupNumber; i ++) {
        if (mPersistMediaConfig.p_scheduler_network_reciever[i]) {
            mPersistMediaConfig.p_scheduler_network_reciever[i]->StopScheduling();
            mPersistMediaConfig.p_scheduler_network_reciever[i]->Delete();
            mPersistMediaConfig.p_scheduler_network_reciever[i] = NULL;
        }
    }

    for (i = 0; i < DMaxSchedulerGroupNumber; i ++) {
        if (mPersistMediaConfig.p_scheduler_network_tcp_reciever[i]) {
            mPersistMediaConfig.p_scheduler_network_tcp_reciever[i]->StopScheduling();
            mPersistMediaConfig.p_scheduler_network_tcp_reciever[i]->Delete();
            mPersistMediaConfig.p_scheduler_network_tcp_reciever[i] = NULL;
        }
    }

    for (i = 0; i < DMaxSchedulerGroupNumber; i ++) {
        if (mPersistMediaConfig.p_scheduler_network_sender[i]) {
            mPersistMediaConfig.p_scheduler_network_sender[i]->StopScheduling();
            mPersistMediaConfig.p_scheduler_network_sender[i]->Delete();
            mPersistMediaConfig.p_scheduler_network_sender[i] = NULL;
        }
    }

    for (i = 0; i < DMaxSchedulerGroupNumber; i ++) {
        if (mPersistMediaConfig.p_scheduler_io_reader[i]) {
            mPersistMediaConfig.p_scheduler_io_reader[i]->StopScheduling();
            mPersistMediaConfig.p_scheduler_io_reader[i]->Delete();
            mPersistMediaConfig.p_scheduler_io_reader[i] = NULL;
        }
    }

    for (i = 0; i < DMaxSchedulerGroupNumber; i ++) {
        if (mPersistMediaConfig.p_scheduler_io_writer[i]) {
            mPersistMediaConfig.p_scheduler_io_writer[i]->StopScheduling();
            mPersistMediaConfig.p_scheduler_io_writer[i]->Delete();
            mPersistMediaConfig.p_scheduler_io_writer[i] = NULL;
        }
    }

    mbSchedulersCreated = 0;

    return EECode_OK;
}

//flush pipeline, seek to somewhere, change speed, resume pipeline
EECode CActiveGenericEngine::PBSpeed(TGenericID pb_pipeline, TU16 speed, TU8 backward, DecoderFeedingRule feeding_rule)
{
    SGenericPipeline *pipeline = queryPipeline(pb_pipeline);
    TU8 need_suspend_audio = 0;
    TU8 has_demuxer = 1;
    TU8 has_audio_decoder = 0;
    TU8 has_video_decoder = 0;
    SDSPControlStatus status = {0};

    //debug assert
    DASSERT(pipeline);
    DASSERT(mpDSPAPI);

    if (!mpDSPAPI || !pipeline) {
        LOGM_ERROR("NULL pointer mpDSPAPI %p or pipeline %p, pipeline_id 0x%08x\n", mpDSPAPI, pipeline, pb_pipeline);
        return EECode_BadParam;
    }

    DASSERT(EGenericPipelineType_Playback == pipeline->pipeline_type);
    if (EGenericPipelineType_Playback != pipeline->pipeline_type) {
        LOGM_ERROR("you should input a pb_pipeline id, 0x%08x is not a pb_pipeline id\n", pb_pipeline);
        return EECode_BadParam;
    }

    DASSERT(pipeline->occupied_dsp_dec_instance);
    if (!pipeline->occupied_dsp_dec_instance) {
        LOGM_WARN("the pb_pipeline 0x%08x does not occupied dsp decoder instance, in PBSpeed\n", pb_pipeline);
    }

    SComponent_Obsolete *p_demuxer;
    SComponent_Obsolete *p_audio_decoder;
    SComponent_Obsolete *p_video_decoder;

    SConnection_Obsolete *p_video_pin;
    SConnection_Obsolete *p_audio_pin;

    p_demuxer = queryFilter(pipeline->pb_source_id);
    DASSERT(p_demuxer);
    DASSERT(EComponentState_running == p_demuxer->cur_state);
    if (EComponentState_running != p_demuxer->cur_state) {
        LOGM_ERROR("pb pipeline source component BAD state %d\n", p_demuxer->cur_state);
        return EECode_BadState;
    }

    p_video_decoder = queryFilter(pipeline->pb_video_decoder_id);
    DASSERT(p_video_decoder);

    p_audio_decoder = queryFilter(pipeline->pb_audio_decoder_id);
    DASSERT(p_audio_decoder);

    p_video_pin = queryConnection(pipeline->pb_vd_inputpin_id);
    DASSERT(p_video_pin);
    p_audio_pin = queryConnection(pipeline->pb_ad_inputpin_id);
    DASSERT(p_audio_pin);

    if (mbChangePBSpeedWithSeek) {
        //get last pts
        status.decoder_id = pipeline->pb_video_decoder_id;
        mpDSPAPI->DSPControl(EDSPControlType_UDEC_get_status, (void *)&status);
    }

    if ((1 == speed) && (!speed_frac) && (!backward)) {
        need_suspend_audio = 0;
    } else {
        if (mbSuspendAudioPlaybackPipelineInFwBw) {
            need_suspend_audio = 1;
            LOGM_INFO("need suspend_audio, speed %d, speed_frac %d, backward %d\n", speed, speed_frac, backward);
        } else {
            need_suspend_audio = 0;
        }
    }

    if (need_suspend_audio) {
        if (pipeline->occupied_audio_dec_instance) {
            if (p_audio_decoder && p_audio_decoder->p_filter) {
                DASSERT(!pipeline->audio_stopped);
                if (!pipeline->audio_stopped) {
                    p_audio_decoder->p_filter->Suspend();
                    pipeline->audio_stopped = 1;
                }
            } else {
                LOGM_ERROR("NULL p_audio_decoder %p?\n", p_audio_decoder);
                if (p_audio_decoder) {
                    LOGM_ERROR("NULL p_audio_decoder->p_filter %p!\n", p_audio_decoder->p_filter);
                }
            }
        }
    }

    if (mbChangePBSpeedWithFlush) {
        //stop iav
        /*SDSPControlClear stop={0};
        stop.decoder_id = pipeline->mapped_dsp_dec_instance_id;
        stop.flag = 1;
        mpDSPAPI->DSPControl(EDSPControlType_UDEC_clear, (void*)&stop);*///TODO: 2013.12.18 roy temp mdf, confilct with op in p_video_decoder->p_filter->Flush()

        //flush filters
        if (p_demuxer && p_demuxer->p_filter) {
            p_demuxer->p_filter->Flush();
            has_demuxer = 1;
        } else {
            LOGM_ERROR("NULL p_demuxer %p?\n", p_demuxer);
            if (p_demuxer) {
                LOGM_ERROR("NULL p_demuxer->p_filter %p!\n", p_demuxer->p_filter);
            }
            return EECode_BadParam;
        }

        if ((!need_suspend_audio) && pipeline->occupied_audio_dec_instance) {
            if (p_audio_decoder && p_audio_decoder->p_filter) {
                p_audio_decoder->p_filter->Flush();
                has_audio_decoder = 1;
            } else {
                LOGM_ERROR("NULL p_audio_decoder %p?\n", p_audio_decoder);
                if (p_audio_decoder) {
                    LOGM_ERROR("NULL p_audio_decoder->p_filter %p!\n", p_audio_decoder->p_filter);
                }
            }
        }

        if (pipeline->occupied_dsp_dec_instance) {
            if (p_video_decoder && p_video_decoder->p_filter) {
                p_video_decoder->p_filter->Flush();
                has_video_decoder = 1;
            } else {
                LOGM_ERROR("NULL p_video_decoder %p?\n", p_video_decoder);
                if (p_video_decoder) {
                    LOGM_ERROR("NULL p_video_decoder->p_filter %p!\n", p_video_decoder->p_filter);
                }
                return EECode_BadParam;
            }
        }
        /*
                //flush filters
                if (has_demuxer) {
                    p_demuxer->p_filter->Flush();
                }

                if (has_audio_decoder) {
                    p_audio_decoder->p_filter->Flush();
                }

                if (has_video_decoder) {
                    p_video_decoder->p_filter->Flush();
                }*/
    }

    if (mbChangePBSpeedWithSeek) {
        //seek
        //LOGM_ERROR("to do seek!!!!\n");
        SPbSeek seek;
        seek.target = status.last_pts * 1000 / (TimeUnitDen_90khz);
        p_demuxer->p_filter->FilterProperty(EFilterPropertyType_demuxer_seek, 1, (void *)&seek);
    }

    //change speed
    SPbFeedingRule pbfeedingrule;
    pbfeedingrule.speed = speed;
    pbfeedingrule.direction = backward;
    pbfeedingrule.feeding_rule = (TU8)feeding_rule;
    p_demuxer->p_filter->FilterProperty(EFilterPropertyType_demuxer_pb_speed_feedingrule, 1, (void *)&pbfeedingrule);
    p_video_decoder->p_filter->FilterProperty(EFilterPropertyType_vdecoder_pb_speed_feedingrule, 1, (void *)&pbfeedingrule);

    if (mbChangePBSpeedWithFlush) {
        //clear iav status
        SDSPControlClear clear = {0};
        clear.decoder_id = pipeline->mapped_dsp_dec_instance_id;
        clear.flag = 0xff;
        mpDSPAPI->DSPControl(EDSPControlType_UDEC_clear, (void *)&clear);

        //resume_flush all filters
        if (has_demuxer) {
            p_demuxer->p_filter->ResumeFlush();
            //            p_demuxer->p_filter->Resume();
        }

        if (has_audio_decoder) {
            p_audio_decoder->p_filter->ResumeFlush();
            //            p_audio_decoder->p_filter->Resume();
        }

        if (has_video_decoder) {
            p_video_decoder->p_filter->ResumeFlush();
            //            p_video_decoder->p_filter->Resume();
        }
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::GenericControl(EGenericEngineConfigure config_type, void *param)
{
    EECode err = EECode_OK;

    if (!param) {
        LOGM_NOTICE("NULL param for type 0x%08x\n", config_type);
    }

    switch (config_type) {

        case EGenericEngineConfigure_VideoDisplay_UpdateDisplayDeviceMask: {
                SVideoPostPDisplayMask *display_mask = (SVideoPostPDisplayMask *)param;
                DASSERT(EGenericEngineConfigure_VideoDisplay_UpdateDisplayDeviceMask == display_mask->check_field);

                if (mpUniqueVideoPostProcessor) {
                    err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_update_display_mask, 1, param);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_vpostp_update_display_mask return %d\n", err);
                    }
                } else {
                    LOGM_FATAL("No video processor\n");
                    err = EECode_BadState;
                }
            }
            break;

        case EGenericEngineConfigure_VideoDisplay_StreamSwitch: {
                SVideoPostPStreamSwitch *stream_switch = (SVideoPostPStreamSwitch *)param;
                DASSERT(EGenericEngineConfigure_VideoDisplay_StreamSwitch == stream_switch->check_field);

                if (mpUniqueVideoPostProcessor) {
                    err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_stream_switch, 1, param);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_vpostp_stream_switch return %d\n", err);
                    }
                } else {
                    LOGM_FATAL("No video processor\n");
                    err = EECode_BadState;
                }
            }
            break;

        case EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayout: {
                SVideoPostPDisplayLayout *layout = (SVideoPostPDisplayLayout *)param;
                TU8 pre_layout_demuxer_tot_num = mInitalVideoPostPDisplayLayout.layer_window_number[0] * 2;
                DASSERT(EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayout == layout->check_field);

                if (mInitalVideoPostPDisplayLayout.layer_layout_type[0] == layout->layer_layout_type[0]) {//same layout
                    if (EDisplayLayout_TelePresence != mInitalVideoPostPDisplayLayout.layer_layout_type[0]) {
                        LOGM_NOTICE("EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayout, layout=%u not changed, do nothing.\n", mInitalVideoPostPDisplayLayout.layer_layout_type[0]);
                        break;
                    }
                    SVideoPostPDisplayHD hd;
                    hd.check_field = EGenericEngineConfigure_VideoDisplay_SwitchWindowToHD;
                    hd.tobeHD = layout->is_window0_HD;
                    hd.sd_window_index_for_hd = 0;
                    hd.flush_udec = 1;
                    err = GenericControl(EGenericEngineConfigure_VideoDisplay_SwitchWindowToHD, &hd);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayout, EGenericEngineConfigure_VideoDisplay_SwitchWindowToHD return %d\n", err);
                    }
                    break;
                }

                LOGM_FLOW("before EFilterPropertyType_vpostp_update_display_layer\n");
                if (mpUniqueVideoPostProcessor) {
                    err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_update_display_layout, 1, param);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_vpostp_update_display_layout return %d\n", err);
                        break;
                    } else {
                        mInitalVideoPostPDisplayLayout.layer_layout_type[0] = layout->layer_layout_type[0];
                        mInitalVideoPostPDisplayLayout.layer_layout_type[1] = layout->layer_layout_type[1];
                        mInitalVideoPostPDisplayLayout.is_window0_HD = layout->is_window0_HD;
                    }
                } else {
                    LOGM_FATAL("No video processor\n");
                    err = EECode_BadState;
                    break;
                }
                //switch window 0 to HD for EDisplayLayout_TelePresence on layer 0
                if (EDisplayLayout_TelePresence == layout->layer_layout_type[0] && layout->is_window0_HD && !mbCertainSDWindowSwitchedtoHD && mpUniqueVideoPostProcessor) {
                    TU8 check_layer_index = 0;
                    TU8 sd_window_index_for_hd = 0;
                    LOGM_INFO("suspend vid-decode [%d ~ %d], vid-demux [%d ~ %d]\n", sd_window_index_for_hd, sd_window_index_for_hd + 1,
                              sd_window_index_for_hd + mCurSourceGroupIndex * layout->layer_window_number[check_layer_index] * 2, sd_window_index_for_hd + 1 + mCurSourceGroupIndex * layout->layer_window_number[check_layer_index] * 2);
                    suspendPlaybackPipelines(sd_window_index_for_hd, sd_window_index_for_hd + 1,
                                             sd_window_index_for_hd + mCurSourceGroupIndex * layout->layer_window_number[check_layer_index] * 2, sd_window_index_for_hd + 1 + mCurSourceGroupIndex * layout->layer_window_number[check_layer_index] * 2, 1);
                    LOGM_INFO("resume suspend vid-decode [%d ~ %d], vid-demux [%d ~ %d], layer->sd_window_index_for_hd %d, focus_input_index = %u\n",
                              layout->layer_window_number[check_layer_index],
                              layout->layer_window_number[check_layer_index] + 1,
                              layout->layer_window_number[check_layer_index] + sd_window_index_for_hd + mCurSourceGroupIndex * layout->layer_window_number[check_layer_index] * 2,
                              layout->layer_window_number[check_layer_index] + sd_window_index_for_hd + 1 + mCurSourceGroupIndex * layout->layer_window_number[check_layer_index] * 2,
                              sd_window_index_for_hd,
                              (mCurSourceGroupIndex * layout->layer_window_number[check_layer_index]) + sd_window_index_for_hd);
                    err = resumeSuspendPlaybackPipelines(layout->layer_window_number[check_layer_index], layout->layer_window_number[check_layer_index] + 1,
                                                         layout->layer_window_number[check_layer_index] + sd_window_index_for_hd + mCurSourceGroupIndex * layout->layer_window_number[check_layer_index] * 2, layout->layer_window_number[check_layer_index] + sd_window_index_for_hd + 1 + mCurSourceGroupIndex * layout->layer_window_number[check_layer_index] * 2,
                                                         (mCurSourceGroupIndex * layout->layer_window_number[check_layer_index]) + sd_window_index_for_hd);
                    if (EECode_OK != err) {
                        LOGM_FATAL("resumeSuspendPlaybackPipelines() return %d\n", err);
                        break;
                    }
                    SVideoPostPDisplayHD hd;
                    hd.check_field = EGenericEngineConfigure_VideoDisplay_SwitchWindowToHD;
                    hd.tobeHD = 1;
                    hd.sd_window_index_for_hd = sd_window_index_for_hd;
                    hd.flush_udec = 1;
                    err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_switch_window_to_HD, 1, &hd);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_vpostp_switch_window_to_HD return %d\n", err);
                        break;
                    }
                    mbCertainSDWindowSwitchedtoHD = 1;
                } else {
                    if (layout->cur_decoder_start_index) {
                        LOGM_INFO("suspend vid-decode [%d ~ %d]\n", 0, layout->cur_decoder_start_index);
                        suspendPlaybackPipelines(0, layout->cur_decoder_start_index, 0);
                    }
                    if (pre_layout_demuxer_tot_num) {
                        LOGM_INFO("suspend vid-demux [%d ~ %d]\n", 0, pre_layout_demuxer_tot_num);
                        suspendPlaybackPipelines(0, 0, 0 + mCurSourceGroupIndex * layout->layer_window_number[0] * 2, pre_layout_demuxer_tot_num + mCurSourceGroupIndex * layout->layer_window_number[0] * 2, 0);
                    }
                    LOGM_INFO("resume suspend vid-decode [%d ~ %d], vid-demux [%d ~ %d], focus_input_index = %u\n",
                              layout->cur_decoder_start_index,
                              layout->cur_decoder_start_index + layout->cur_decoder_number,
                              0 + mCurSourceGroupIndex * layout->layer_window_number[0] * 2,
                              layout->layer_window_number[0] + mCurSourceGroupIndex * layout->layer_window_number[0] * 2,
                              mCurSourceGroupIndex);
                    err = resumeSuspendPlaybackPipelines(layout->cur_decoder_start_index, layout->cur_decoder_start_index + layout->cur_decoder_number,
                                                         0 + mCurSourceGroupIndex * layout->layer_window_number[0] * 2, layout->layer_window_number[0] + mCurSourceGroupIndex * layout->layer_window_number[0] * 2, mCurSourceGroupIndex);
                    if (EECode_OK != err) {
                        LOGM_FATAL("resumeSuspendPlaybackPipelines() return %d\n", err);
                        break;
                    }
                    if ((layout->cur_decoder_start_index + layout->cur_decoder_number) < layout->total_decoder_number) {
                        LOGM_INFO("suspend %d ~ %d\n", layout->cur_decoder_start_index + layout->cur_decoder_number, layout->total_decoder_number);
                        suspendPlaybackPipelines(layout->cur_decoder_start_index + layout->cur_decoder_number, layout->total_decoder_number, 0);
                    }
                    mbCertainSDWindowSwitchedtoHD = 0;
                }
                LOGM_FLOW("after suspend/resume-suspend according playback pipelines\n");

            }
            break;

        case EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer: {
                SVideoPostPDisplayLayer *layer = (SVideoPostPDisplayLayer *)param;
                DASSERT(EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer == layer->check_field);

                if (mpUniqueVideoPostProcessor) {
                    SVideoPostPConfiguration video_postp_info;
                    TUint tobe_suspend_number_1 = 0, tobe_suspend_number_2 = 0, tobe_resumed_number = 0, ii = 0;

                    memset(&video_postp_info, 0x0, sizeof(video_postp_info));
                    err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_query_display, 1, &video_postp_info);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_vpostp_query_display return %d\n", err);
                        break;
                    }

                    if (layer->request_display_layer >= video_postp_info.layer_number) {
                        LOGM_ERROR("request_display_layer=%d out of range(max is %d)\n", layer->request_display_layer, video_postp_info.layer_number);
                        err = EECode_BadParam;
                        break;
                    }

                    DASSERT(video_postp_info.layer_number <= DMaxDisplayLayerNumber);
                    for (ii = 0; ii < video_postp_info.layer_number; ii ++) {
                        if (ii < layer->request_display_layer) {
                            tobe_suspend_number_1 += video_postp_info.layer_window_number[ii];
                        } else if (ii > layer->request_display_layer) {
                            tobe_suspend_number_2 += video_postp_info.layer_window_number[ii];
                        }
                    }
                    tobe_resumed_number = video_postp_info.layer_window_number[layer->request_display_layer];

                    if (tobe_suspend_number_1) {
                        LOGM_INFO("suspend vid-decode [%d ~ %d], vid-demux [%d ~ %d]\n", 0, tobe_suspend_number_1,
                                  0 + mCurSourceGroupIndex * video_postp_info.layer_window_number[0] * 2, tobe_suspend_number_1 + mCurSourceGroupIndex * video_postp_info.layer_window_number[0] * 2);
                        suspendPlaybackPipelines(0, tobe_suspend_number_1,
                                                 0 + mCurSourceGroupIndex * video_postp_info.layer_window_number[0] * 2, tobe_suspend_number_1 + mCurSourceGroupIndex * video_postp_info.layer_window_number[0] * 2, layer->flush_udec);
                    }

                    if (tobe_resumed_number) {
                        if (!layer->request_display_layer) {
                            LOGM_INFO("resume suspend vid-decode [%d ~ %d], vid-demux [%d ~ %d], layer->request_display_layer %d, layer->sd_window_index_for_hd %d, focus_input_index = %u\n",
                                      tobe_suspend_number_1,
                                      tobe_suspend_number_1 + tobe_resumed_number,
                                      tobe_suspend_number_1 + mCurSourceGroupIndex * video_postp_info.layer_window_number[0] * 2,
                                      tobe_suspend_number_1 + tobe_resumed_number + mCurSourceGroupIndex * video_postp_info.layer_window_number[0] * 2,
                                      layer->request_display_layer,
                                      layer->sd_window_index_for_hd,
                                      mCurSourceGroupIndex);
                            err = resumeSuspendPlaybackPipelines(tobe_suspend_number_1, tobe_suspend_number_1 + tobe_resumed_number,
                                                                 tobe_suspend_number_1 + mCurSourceGroupIndex * video_postp_info.layer_window_number[0] * 2, tobe_suspend_number_1 + tobe_resumed_number + mCurSourceGroupIndex * video_postp_info.layer_window_number[0] * 2, mCurSourceGroupIndex);
                            if (EECode_OK != err) {
                                LOGM_FATAL("resumeSuspendPlaybackPipelines() return %d\n", err);
                                break;
                            }
                            if (EDisplayLayout_TelePresence == mInitalVideoPostPDisplayLayout.layer_layout_type[0] && mInitalVideoPostPDisplayLayout.is_window0_HD) {
                                mbCertainSDWindowSwitchedtoHD = 0;
                                SVideoPostPDisplayHD hd;
                                hd.check_field = EGenericEngineConfigure_VideoDisplay_SwitchWindowToHD;
                                hd.tobeHD = 1;
                                hd.sd_window_index_for_hd = 0;
                                hd.flush_udec = 1;
                                err = GenericControl(EGenericEngineConfigure_VideoDisplay_SwitchWindowToHD, &hd);
                                if (EECode_OK != err) {
                                    LOGM_FATAL("EGenericEngineConfigure_VideoPlayback_SwitchSourceGroup, EGenericEngineConfigure_VideoDisplay_SwitchWindowToHD return %d\n", err);
                                    break;
                                }
                            }
                        } else {
                            LOGM_INFO("resume suspend vid-decode [%d ~ %d], vid-demux [%d ~ %d], layer->request_display_layer %d, layer->sd_window_index_for_hd %d, focus_input_index = %u\n",
                                      tobe_suspend_number_1,
                                      tobe_suspend_number_1 + tobe_resumed_number,
                                      tobe_suspend_number_1 + layer->sd_window_index_for_hd + mCurSourceGroupIndex * video_postp_info.layer_window_number[0] * 2,
                                      tobe_suspend_number_1 + layer->sd_window_index_for_hd + tobe_resumed_number + mCurSourceGroupIndex * video_postp_info.layer_window_number[0] * 2,
                                      layer->request_display_layer,
                                      layer->sd_window_index_for_hd,
                                      ((mCurSourceGroupIndex * video_postp_info.layer_window_number[0]) + layer->sd_window_index_for_hd));
                            err = resumeSuspendPlaybackPipelines(tobe_suspend_number_1, tobe_suspend_number_1 + tobe_resumed_number,
                                                                 tobe_suspend_number_1 + layer->sd_window_index_for_hd + mCurSourceGroupIndex * video_postp_info.layer_window_number[0] * 2, tobe_suspend_number_1 + layer->sd_window_index_for_hd + tobe_resumed_number + mCurSourceGroupIndex * video_postp_info.layer_window_number[0] * 2,
                                                                 (mCurSourceGroupIndex * video_postp_info.layer_window_number[0]) + layer->sd_window_index_for_hd);
                            if (EECode_OK != err) {
                                LOGM_FATAL("resumeSuspendPlaybackPipelines() return %d\n", err);
                                break;
                            }
                            if (0xff != mCurHDWindowIndex) {
                                suspendPlaybackPipelines(0, 0,
                                                         tobe_suspend_number_1 + mCurHDWindowIndex + mCurSourceGroupIndex * video_postp_info.layer_window_number[0] * 2, tobe_suspend_number_1 + mCurHDWindowIndex + tobe_resumed_number + mCurSourceGroupIndex * video_postp_info.layer_window_number[0] * 2,
                                                         0);
                            }
                            mCurHDWindowIndex = layer->sd_window_index_for_hd;//update for suspend HD demuxer
                        }
                    }

                    if (EDisplayLayout_TelePresence == mInitalVideoPostPDisplayLayout.layer_layout_type[0]  && mInitalVideoPostPDisplayLayout.is_window0_HD && 0 == mCurHDWindowIndex) {
                        LOGM_NOTICE("EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer to 0(EDisplayLayout_TelePresence), will not suspend HD demuxer for window 0\n");
                    } else if (tobe_suspend_number_2) {
                        LOGM_INFO("suspend vid-decode [%d ~ %d], vid-demux [%d ~ %d]\n", tobe_suspend_number_1 + tobe_resumed_number, tobe_suspend_number_1 + tobe_resumed_number + tobe_suspend_number_2,
                                  tobe_suspend_number_1 + tobe_resumed_number + mCurHDWindowIndex + mCurSourceGroupIndex * video_postp_info.layer_window_number[0] * 2, tobe_suspend_number_1 + tobe_resumed_number + tobe_suspend_number_2 + mCurHDWindowIndex + mCurSourceGroupIndex * video_postp_info.layer_window_number[0] * 2);
                        suspendPlaybackPipelines(tobe_suspend_number_1 + tobe_resumed_number, tobe_suspend_number_1 + tobe_resumed_number + tobe_suspend_number_2,
                                                 tobe_suspend_number_1 + tobe_resumed_number + mCurHDWindowIndex + mCurSourceGroupIndex * video_postp_info.layer_window_number[0] * 2, tobe_suspend_number_1 + tobe_resumed_number + tobe_suspend_number_2 + mCurHDWindowIndex + mCurSourceGroupIndex * video_postp_info.layer_window_number[0] * 2,
                                                 layer->flush_udec);
                    }
                    if (!layer->request_display_layer) {//multi-windows
                        mCurHDWindowIndex = 0xff;
                    }

                    err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_update_display_layer, 1, param);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_vpostp_update_display_layer return %d\n", err);
                        break;
                    }
                } else {
                    LOGM_FATAL("No video processor\n");
                    err = EECode_BadState;
                }
                LOGM_NOTICE("after EFilterPropertyType_vpostp_update_display_layer\n");
            }
            break;

        case EGenericEngineConfigure_VideoDisplay_SwapWindowContent: {
                SVideoPostPSwap *swap = (SVideoPostPSwap *)param;
                DASSERT(EGenericEngineConfigure_VideoDisplay_SwapWindowContent == swap->check_field);

                LOGM_FATAL("TO DO!\n");
                err = EECode_NoImplement;
            }
            break;

        case EGenericEngineConfigure_VideoPlayback_Capture: {
                SPlaybackCapture *playback_capture = (SPlaybackCapture *)param;
                DASSERT(EGenericEngineConfigure_VideoPlayback_Capture == playback_capture->check_field);

                err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_playback_capture, 1, param);
            }
            break;

        case EGenericEngineConfigure_VideoPlayback_Zoom: {
                SPlaybackZoom *playback_zoom = (SPlaybackZoom *)param;
                DASSERT(EGenericEngineConfigure_VideoPlayback_Zoom == playback_zoom->check_field);

                err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_playback_zoom, 1, param);
            }
            break;

        case EGenericEngineConfigure_VideoDisplay_ShiftWindowContent: {
                SVideoPostPShift *shift = (SVideoPostPShift *)param;
                DASSERT(EGenericEngineConfigure_VideoDisplay_ShiftWindowContent == shift->check_field);

                LOGM_FATAL("TO DO!\n");
                err = EECode_NoImplement;
            }
            break;

        case EGenericEngineConfigure_VideoDisplay_UpdateUDECSource: {
                SUpdateUDECSource *update_source = (SUpdateUDECSource *)param;
                DASSERT(EGenericEngineConfigure_VideoDisplay_UpdateUDECSource == update_source->check_field);

                LOGM_FATAL("TO DO!\n");
                err = EECode_NoImplement;
            }
            break;

        case EGenericEngineConfigure_VideoDisplay_SingleViewMode: {
                SVideoPostPDisplaySingleViewMode *single_view = (SVideoPostPDisplaySingleViewMode *)param;
                DASSERT(EGenericEngineConfigure_VideoDisplay_SingleViewMode == single_view->check_field);

                LOGM_FATAL("TO DO!\n");
                err = EECode_NoImplement;
            }
            break;

        case EGenericEngineConfigure_VideoDisplay_UpdateHighLightenWindowSize: {
                SVideoPostPDisplayHighLightenWindowSize *size = (SVideoPostPDisplayHighLightenWindowSize *)param;
                DASSERT(EGenericEngineConfigure_VideoDisplay_UpdateHighLightenWindowSize == size->check_field);

                if (mpUniqueVideoPostProcessor) {
                    err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_update_display_highlightenwindowsize, 1, param);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_vpostp_update_display_highlightenwindowsize return %d\n", err);
                    }
                } else {
                    LOGM_FATAL("No video processor\n");
                    err = EECode_BadState;
                }
            }
            break;

        case EGenericEngineConfigure_VideoPlayback_LoopMode: {
                SGenericPipeline *pipeline;
                SComponent_Obsolete *p_demuxer;
                SPipeLineLoopMode *loop_mode = (SPipeLineLoopMode *)param;
                DASSERT(loop_mode);
                DASSERT(EGenericEngineConfigure_VideoPlayback_LoopMode == loop_mode->check_field);

                pipeline = queryPipeline(loop_mode->pb_pipeline);
                DASSERT(pipeline);
                p_demuxer = queryFilter(pipeline->pb_source_id);
                DASSERT(p_demuxer);
                DASSERT(EComponentState_running == p_demuxer->cur_state);
                if (EComponentState_running != p_demuxer->cur_state) {
                    LOGM_ERROR("pb pipeline source component BAD state %d\n", p_demuxer->cur_state);
                    return EECode_BadState;
                }

                p_demuxer->p_filter->FilterProperty(EFilterPropertyType_playback_loop_mode, 1, (void *)&loop_mode->loop_mode);
            }
            break;

        case EGenericEngineConfigure_VideoPlayback_CompensateDelayFromJitter: {
                mPersistMediaConfig.compensate_from_jitter = 1;
            }
            break;

        case EGenericEngineConfigure_VideoPlayback_SwitchSourceGroup: {
                SPipeLineSwitchSourceGroup *switch_source_group = (SPipeLineSwitchSourceGroup *)param;
                DASSERT(switch_source_group);
                DASSERT(EGenericEngineConfigure_VideoPlayback_SwitchSourceGroup == switch_source_group->check_field);
                if (mCurSourceGroupIndex == switch_source_group->target_source_group_index) {
                    LOGM_NOTICE("EGenericEngineConfigure_VideoPlayback_SwitchSourceGroup, target_source_group_index %u not change, do nothing.\n", switch_source_group->target_source_group_index);
                    break;
                }
                if (mpUniqueVideoPostProcessor) {
                    SVideoPostPConfiguration video_postp_info;
                    memset(&video_postp_info, 0x0, sizeof(video_postp_info));
                    err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_query_display, 1, &video_postp_info);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EGenericEngineConfigure_VideoPlayback_SwitchSourceGroup, EFilterPropertyType_vpostp_query_display return %d\n", err);
                        break;
                    }
                    if (video_postp_info.layer_window_number[0]) {
                        LOGM_INFO("resumeSuspendPlaybackPipelines %d ~ %d, focus_input_index = %u\n",
                                  0,
                                  video_postp_info.layer_window_number[0] * 2,
                                  switch_source_group->target_source_group_index);
                        err = resumeSuspendPlaybackPipelines(0, video_postp_info.layer_window_number[0] * 2/*sd+hd*/, switch_source_group->target_source_group_index);
                        if (EECode_OK != err) {
                            LOGM_FATAL("resumeSuspendPlaybackPipelines() return %d\n", err);
                            break;
                        }
                        suspendPlaybackPipelines(0, 0, mCurSourceGroupIndex * video_postp_info.layer_window_number[0] * 2, mCurSourceGroupIndex * video_postp_info.layer_window_number[0] * 2 + video_postp_info.layer_window_number[0] * 2/*sd+hd*/, 0);
                        mCurSourceGroupIndex = switch_source_group->target_source_group_index;
                        resumeSuspendPlaybackPipelines(0, 0, mCurSourceGroupIndex * video_postp_info.layer_window_number[0] * 2, mCurSourceGroupIndex * video_postp_info.layer_window_number[0] * 2 + video_postp_info.layer_window_number[0]/*sd*/, 0);
                        if (EDisplayLayout_TelePresence == mInitalVideoPostPDisplayLayout.layer_layout_type[0] && mInitalVideoPostPDisplayLayout.is_window0_HD) {
                            mbCertainSDWindowSwitchedtoHD = 0;
                            SVideoPostPDisplayHD hd;
                            hd.check_field = EGenericEngineConfigure_VideoDisplay_SwitchWindowToHD;
                            hd.tobeHD = 1;
                            hd.sd_window_index_for_hd = 0;
                            hd.flush_udec = 1;
                            err = GenericControl(EGenericEngineConfigure_VideoDisplay_SwitchWindowToHD, &hd);
                            if (EECode_OK != err) {
                                LOGM_FATAL("EGenericEngineConfigure_VideoPlayback_SwitchSourceGroup, EGenericEngineConfigure_VideoDisplay_SwitchWindowToHD return %d\n", err);
                                break;
                            }
                        }
                    } else {
                        LOGM_ERROR("EGenericEngineConfigure_VideoPlayback_SwitchSourceGroup failed, video_postp_info.layer_window_number[0]=%u\n", video_postp_info.layer_window_number[0]);
                        err = EECode_BadParam;
                    }
                } else {
                    LOGM_FATAL("EGenericEngineConfigure_VideoPlayback_SwitchSourceGroup, No video processor\n");
                    err = EECode_BadState;
                }
            }
            break;
            //not support here
        case EGenericEngineConfigure_VideoPlayback_WindowPause: {
                SVideoPostPDisplayTrickPlay *trickplay = (SVideoPostPDisplayTrickPlay *)param;
                SVideoPostPDisplayMWMapping mapping;
                DASSERT(EGenericEngineConfigure_VideoPlayback_WindowPause == trickplay->check_field);

                LOGM_FATAL("This API to be deprecated!\n");

                LOGM_FLOW("before EFilterPropertyType_vrenderer_trickplay_pause\n");
                if (mpUniqueVideoRenderer) {
                    if (mpUniqueVideoPostProcessor) {
                        mapping.window_id = trickplay->id;
                        err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_query_window, 0, (void *)&mapping);
                        if (EECode_OK != err) {
                            LOGM_ERROR("EFilterPropertyType_vpostp_query_window return %d, %s\n", err, gfGetErrorCodeString(err));
                            return err;
                        }
                        if (DINVALID_VALUE_TAG == mapping.decoder_id) {
                            LOGM_ERROR("EFilterPropertyType_vpostp_query_window %d, this window id mapping to none(decoder)!\n", trickplay->id);
                            return EECode_BadParam;
                        }
                    } else {
                        LOGM_FATAL("NULL mpUniqueVideoPostProcessor.\n");
                        return EECode_InternalLogicalBug;
                    }
                    trickplay->id = mapping.decoder_id;
                    err = mpUniqueVideoRenderer->FilterProperty(EFilterPropertyType_vrenderer_trickplay_pause, 1, param);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_vrenderer_trickplay_pause return %d\n", err);
                    }
                } else {
                    LOGM_FATAL("No video renderer\n");
                    err = EECode_BadState;
                }
                LOGM_FLOW("after EFilterPropertyType_vrenderer_trickplay_pause\n");
            }
            break;

            //not support here
        case EGenericEngineConfigure_VideoPlayback_WindowResume: {
                SVideoPostPDisplayTrickPlay *trickplay = (SVideoPostPDisplayTrickPlay *)param;
                SVideoPostPDisplayMWMapping mapping;
                DASSERT(EGenericEngineConfigure_VideoPlayback_WindowResume == trickplay->check_field);

                LOGM_FATAL("This API to be deprecated!\n");

                LOGM_FLOW("before EFilterPropertyType_vrenderer_trickplay_resume\n");
                if (mpUniqueVideoRenderer) {
                    if (mpUniqueVideoPostProcessor) {
                        mapping.window_id = trickplay->id;
                        err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_query_window, 0, (void *)&mapping);
                        if (EECode_OK != err) {
                            LOGM_ERROR("EFilterPropertyType_vpostp_query_window return %d, %s\n", err, gfGetErrorCodeString(err));
                            return err;
                        }
                        if (DINVALID_VALUE_TAG == mapping.decoder_id) {
                            LOGM_ERROR("EFilterPropertyType_vpostp_query_window %d, this window id mapping to none(decoder)!\n", trickplay->id);
                            return EECode_BadParam;
                        }
                    } else {
                        LOGM_FATAL("NULL mpUniqueVideoPostProcessor.\n");
                        return EECode_InternalLogicalBug;
                    }
                    trickplay->id = mapping.decoder_id;
                    err = mpUniqueVideoRenderer->FilterProperty(EFilterPropertyType_vrenderer_trickplay_resume, 1, param);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_vrenderer_trickplay_resume return %d\n", err);
                    }
                } else {
                    LOGM_FATAL("No video renderer\n");
                    err = EECode_BadState;
                }
                LOGM_FLOW("after EFilterPropertyType_vrenderer_trickplay_resume\n");
            }
            break;

            //not support here
        case EGenericEngineConfigure_VideoPlayback_WindowStepPlay: {
                SVideoPostPDisplayTrickPlay *trickplay = (SVideoPostPDisplayTrickPlay *)param;
                SVideoPostPDisplayMWMapping mapping;
                DASSERT(EGenericEngineConfigure_VideoPlayback_WindowStepPlay == trickplay->check_field);

                LOGM_FATAL("This API to be deprecated!\n");

                LOGM_FLOW("before EFilterPropertyType_vrenderer_trickplay_step\n");
                if (mpUniqueVideoRenderer) {
                    if (mpUniqueVideoPostProcessor) {
                        mapping.window_id = trickplay->id;
                        err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_query_window, 0, (void *)&mapping);
                        if (EECode_OK != err) {
                            LOGM_ERROR("EFilterPropertyType_vpostp_query_window return %d, %s\n", err, gfGetErrorCodeString(err));
                            return err;
                        }
                        if (DINVALID_VALUE_TAG == mapping.decoder_id) {
                            LOGM_ERROR("EFilterPropertyType_vpostp_query_window %d, this window id mapping to none(decoder)!\n", trickplay->id);
                            return EECode_BadParam;
                        }
                    } else {
                        LOGM_FATAL("NULL mpUniqueVideoPostProcessor.\n");
                        return EECode_InternalLogicalBug;
                    }
                    trickplay->id = mapping.decoder_id;
                    err = mpUniqueVideoRenderer->FilterProperty(EFilterPropertyType_vrenderer_trickplay_step, 1, param);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_vrenderer_trickplay_step return %d\n", err);
                    }
                } else {
                    LOGM_FATAL("No video renderer\n");
                    err = EECode_BadState;
                }
                LOGM_FLOW("after EFilterPropertyType_vrenderer_trickplay_step\n");
            }
            break;

        case EGenericEngineConfigure_VideoPlayback_UDECPause: {
                SVideoPostPDisplayTrickPlay *trickplay = (SVideoPostPDisplayTrickPlay *)param;
                DASSERT(EGenericEngineConfigure_VideoPlayback_UDECPause == trickplay->check_field);

                LOGM_FLOW("before EFilterPropertyType_vrenderer_trickplay_pause\n");
                if (mpUniqueVideoRenderer) {
                    err = mpUniqueVideoRenderer->FilterProperty(EFilterPropertyType_vrenderer_trickplay_pause, 1, param);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_vrenderer_trickplay_pause return %d\n", err);
                    }
                } else {
                    LOGM_FATAL("No video renderer\n");
                    err = EECode_BadState;
                }
                LOGM_FLOW("after EFilterPropertyType_vrenderer_trickplay_pause\n");

                if (mpUniqueAudioRenderer) {
                    err = mpUniqueAudioRenderer->FilterProperty(EFilterPropertyType_arenderer_pause, 1, param);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_arenderer_pause return %d\n", err);
                    }
                } else {
                    LOGM_FATAL("No audio renderer\n");
                    err = EECode_BadState;
                }
            }
            break;

        case EGenericEngineConfigure_VideoPlayback_UDECResume: {
                SVideoPostPDisplayTrickPlay *trickplay = (SVideoPostPDisplayTrickPlay *)param;
                DASSERT(EGenericEngineConfigure_VideoPlayback_UDECResume == trickplay->check_field);

                LOGM_FLOW("before EFilterPropertyType_vrenderer_trickplay_resume\n");
                if (mpUniqueVideoRenderer) {
                    err = mpUniqueVideoRenderer->FilterProperty(EFilterPropertyType_vrenderer_trickplay_resume, 1, param);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_vrenderer_trickplay_resume return %d\n", err);
                    }
                } else {
                    LOGM_FATAL("No video renderer\n");
                    err = EECode_BadState;
                }
                LOGM_FLOW("after EFilterPropertyType_vrenderer_trickplay_resume\n");

                if (mpUniqueAudioRenderer) {
                    err = mpUniqueAudioRenderer->FilterProperty(EFilterPropertyType_arenderer_resume, 1, param);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_arenderer_resume return %d\n", err);
                    }
                } else {
                    LOGM_FATAL("No audio renderer\n");
                    err = EECode_BadState;
                }
            }
            break;

        case EGenericEngineConfigure_VideoPlayback_UDECStepPlay: {
                SVideoPostPDisplayTrickPlay *trickplay = (SVideoPostPDisplayTrickPlay *)param;
                DASSERT(EGenericEngineConfigure_VideoPlayback_UDECStepPlay == trickplay->check_field);

                LOGM_FLOW("before EFilterPropertyType_vrenderer_trickplay_step\n");
                if (mpUniqueVideoRenderer) {
                    err = mpUniqueVideoRenderer->FilterProperty(EFilterPropertyType_vrenderer_trickplay_step, 1, param);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_vrenderer_trickplay_resume return %d\n", err);
                    }
                } else {
                    LOGM_FATAL("No video renderer\n");
                    err = EECode_BadState;
                }
                LOGM_FLOW("after EFilterPropertyType_vrenderer_trickplay_step\n");
            }
            break;

        case EGenericEngineQuery_VideoDisplayWindow: {
                SVideoPostPDisplayMWMapping *mapping = (SVideoPostPDisplayMWMapping *)param;
                DASSERT(EGenericEngineQuery_VideoDisplayWindow == mapping->check_field);

                LOGM_FLOW("before EFilterPropertyType_vpostp_query_window\n");
                if (mpUniqueVideoRenderer) {
                    err = mpUniqueVideoRenderer->FilterProperty(EFilterPropertyType_vpostp_query_window, 1, param);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_vpostp_query_window return %d\n", err);
                    }
                } else {
                    LOGM_FATAL("No video renderer\n");
                    err = EECode_BadState;
                }
                LOGM_FLOW("after EFilterPropertyType_vpostp_query_window\n");
            }
            break;

        case EGenericEngineQuery_VideoDisplayRender: {
                SVideoPostPDisplayMWMapping *mapping = (SVideoPostPDisplayMWMapping *)param;
                DASSERT(EGenericEngineQuery_VideoDisplayRender == mapping->check_field);

                LOGM_FLOW("before EFilterPropertyType_vpostp_query_render\n");
                if (mpUniqueVideoRenderer) {
                    err = mpUniqueVideoRenderer->FilterProperty(EFilterPropertyType_vpostp_query_render, 1, param);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_vpostp_query_render return %d\n", err);
                    }
                } else {
                    LOGM_FATAL("No video renderer\n");
                    err = EECode_BadState;
                }
                LOGM_FLOW("after EFilterPropertyType_vpostp_query_render\n");
            }
            break;

        case EGenericEngineQuery_VideoDisplayLayout: {
                SQueryVideoDisplayLayout *layout = (SQueryVideoDisplayLayout *)param;
                DASSERT(EGenericEngineQuery_VideoDisplayLayout == layout->check_field);
                DASSERT(layout->layer_index < DMaxDisplayLayerNumber);
                if (layout->layer_index >= DMaxDisplayLayerNumber) {
                    LOGM_ERROR("EGenericEngineQuery_VideoDisplayLayout, params invalid, layer_index=%u(max %u)\n", layout->layer_index, DMaxDisplayLayerNumber - 1);
                    return EECode_BadParam;
                }
                layout->layout_type = mInitalVideoPostPDisplayLayout.layer_layout_type[layout->layer_index];
            }
            break;

        case EGenericEngineConfigure_ReConnectServer: {
                SGenericReconnectStreamingServer *reconnect = (SGenericReconnectStreamingServer *)param;
                DASSERT(EGenericEngineConfigure_ReConnectServer == reconnect->check_field);
                DASSERT(reconnect);

                LOGM_NOTICE("before EGenericEngineConfigure_ReConnectServer\n");

                if (DLikely(!reconnect->all_demuxer)) {
                    SComponent_Obsolete *pfilter = queryFilter(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_Demuxer, reconnect->index));
                    if (pfilter && pfilter->p_filter) {
                        err = pfilter->p_filter->FilterProperty(EFilterPropertyType_demuxer_reconnect_server, 1, param);
                        DASSERT_OK(err);
                        if (EECode_OK != err) {
                            LOGM_FATAL("EFilterPropertyType_demuxer_reconnect_server return %d\n", err);
                        }
                    } else {
                        LOGM_FATAL("No demuxer component\n");
                        err = EECode_BadParam;
                    }
                } else {
                    reconnectAllStreamingServer();
                }
                LOGM_NOTICE("after EGenericEngineConfigure_ReConnectServer\n");
            }
            break;

        case EGenericEngineQuery_VideoDecoder: {
                SVideoPostPDisplayMWMapping *mapping = (SVideoPostPDisplayMWMapping *)param;
                DASSERT(EGenericEngineQuery_VideoDecoder == mapping->check_field);

                LOGM_FLOW("before EFilterPropertyType_vpostp_query_dsp_decoder\n");
                if (mpUniqueVideoRenderer) {
                    err = mpUniqueVideoRenderer->FilterProperty(EFilterPropertyType_vpostp_query_dsp_decoder, 1, param);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_vpostp_query_dsp_decoder return %d\n", err);
                    }
                } else {
                    LOGM_FATAL("No video renderer\n");
                    err = EECode_BadState;
                }
                LOGM_FLOW("after EFilterPropertyType_vpostp_query_dsp_decoder\n");
            }
            break;

        case EGenericEngineConfigure_AudioPlayback_Mute: {
                LOGM_FLOW("EGenericEngineConfigure_AudioPlayback_Mute start\n");
                if (mpUniqueAudioDecoder) {
                    err = mpUniqueAudioDecoder->FilterProperty(EFilterPropertyType_arenderer_mute, 1, NULL);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_ERROR("EFilterPropertyType_arenderer_mute return %d\n", err);
                    }
                } else {
                    LOGM_ERROR("No audio decoder\n");
                    err = EECode_BadState;
                }
                if (mpUniqueAudioRenderer) {
                    err = mpUniqueAudioRenderer->FilterProperty(EFilterPropertyType_arenderer_mute, 1, NULL);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_arenderer_mute return %d\n", err);
                    }
                } else {
                    LOGM_FATAL("No audio renderer\n");
                    err = EECode_BadState;
                }
                LOGM_FLOW("EGenericEngineConfigure_AudioPlayback_Mute end\n");
            }
            break;

        case EGenericEngineConfigure_AudioPlayback_UnMute: {
                LOGM_FLOW("EGenericEngineConfigure_AudioPlayback_UnMute start\n");
                if (mpUniqueAudioDecoder) {
                    err = mpUniqueAudioDecoder->FilterProperty(EFilterPropertyType_arenderer_umute, 1, NULL);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_ERROR("EFilterPropertyType_arenderer_umute return %d\n", err);
                    }
                } else {
                    LOGM_ERROR("No audio decoder\n");
                    err = EECode_BadState;
                }
                if (mpUniqueAudioRenderer) {
                    err = mpUniqueAudioRenderer->FilterProperty(EFilterPropertyType_arenderer_umute, 1, NULL);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_arenderer_umute return %d\n", err);
                    }
                } else {
                    LOGM_FATAL("No audio renderer\n");
                    err = EECode_BadState;
                }
                LOGM_FLOW("EGenericEngineConfigure_AudioPlayback_UnMute end\n");
            }
            break;

        case EGenericEngineConfigure_AudioPlayback_SelectAudioSource: {
                SGenericSelectAudioChannel *audio_channel = (SGenericSelectAudioChannel *)param;
                LOGM_FLOW("EGenericEngineConfigure_AudioPlayback_SelectAudioSource start, mpUniqueAudioPinMuxer %p, mpUniqueAudioDecoder %p\n", mpUniqueAudioPinMuxer, mpUniqueAudioDecoder);
                if (mpUniqueAudioPinMuxer) {
                    err = mpUniqueAudioPinMuxer->SwitchInput((TComponentIndex)audio_channel->channel);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_ERROR("mpUniqueAudioPinMuxer->SwitchInput(%d) return %d, %s\n", audio_channel->channel, err, gfGetErrorCodeString(err));
                    }
                } else if (mpUniqueAudioDecoder) {
                    err = mpUniqueAudioDecoder->SwitchInput((TComponentIndex)audio_channel->channel);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_ERROR("mpUniqueAudioDecoder->SwitchInput(%d) return %d, %s\n", audio_channel->channel, err, gfGetErrorCodeString(err));
                    }
                } else {
                    LOGM_ERROR("No audio muxer pin or decoder\n");
                    err = EECode_BadState;
                }
                LOGM_FLOW("EGenericEngineConfigure_AudioPlayback_SelectAudioSource end\n");
            }
            break;

        case EGenericEngineConfigure_DSP_Suspend: {
                DASSERT(!mbDSPSuspended);
                if (!mbDSPSuspended) {
                    mbDSPSuspended = 1;
                    LOGM_INFO("suspend %d ~ %d\n", 0, mMultipleWindowDecoderNumber + mSingleWindowDecoderNumber);
                    suspendPlaybackPipelines(0, mMultipleWindowDecoderNumber + mSingleWindowDecoderNumber, IFilter::EReleaseFlag_Destroy);

                    if (mpDSPAPI) {
                        mpDSPAPI->EnterDSPMode(EDSPOperationMode_IDLE);
                    }
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_ERROR("EnterDSPMode(%hu, %s) fail, ret %d, %s\n", EDSPOperationMode_IDLE, gfGetDSPOperationModeString(EDSPOperationMode_IDLE), err, gfGetErrorCodeString(err));
                        return err;
                    }
                }
            }
            break;

        case EGenericEngineConfigure_DSP_Resume: {
                DASSERT(mbDSPSuspended);
                if (mbDSPSuspended) {
                    mbDSPSuspended = 0;

                    err = activateWorkingMode(mRequestDSPMode);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_ERROR("activateWorkingMode(%hu, %s) fail, ret %d, %s\n", mRequestDSPMode, gfGetDSPOperationModeString(mRequestDSPMode), err, gfGetErrorCodeString(err));
                        return err;
                    }

                    LOGM_INFO("resume suspend %d ~ %d, focus_input_index = %u\n",
                              0,
                              mMultipleWindowDecoderNumber,
                              mCurSourceGroupIndex);
                    err = resumeSuspendPlaybackPipelines(0, mMultipleWindowDecoderNumber, mCurSourceGroupIndex);
                    if (EECode_OK != err) {
                        LOGM_FATAL("resumeSuspendPlaybackPipelines() return %d\n", err);
                        break;
                    }
                }
            }
            break;

        case EGenericEngineQuery_VideoCodecInfo: {
                SVideoCodecInfo *info = (SVideoCodecInfo *)param;
                TUint dec_index = 0;

                DASSERT(EGenericEngineQuery_VideoCodecInfo == info->check_field);

                dec_index = info->dec_id;
                if (dec_index < DMaxUDECInstanceNumber) {
                    info->pic_width = mPersistMediaConfig.dsp_config.udecInstanceConfig[dec_index].max_frm_width;
                    info->pic_height = mPersistMediaConfig.dsp_config.udecInstanceConfig[dec_index].max_frm_height;
                } else {
                    LOGM_ERROR("BAD info->dec_id %d\n", info->dec_id);
                    return EECode_BadParam;
                }
            }
            break;

        case EGenericEngineQuery_VideoDisplayDeviceInfo: {
                SVideoDisplayDeviceInfo *info = (SVideoDisplayDeviceInfo *)param;
                DASSERT(EGenericEngineQuery_VideoDisplayDeviceInfo == info->check_field);

                if ((EEngineState_idle == msState) || (EEngineState_running == msState)) {
                    info->vout_mask = mPersistMediaConfig.dsp_config.modeConfig.vout_mask;
                    info->primary_vout_index = mInitialVideoDisplayPrimaryIndex;
                    info->primary_vout_width = mPersistMediaConfig.dsp_config.voutConfigs.voutConfig[mInitialVideoDisplayPrimaryIndex].width;
                    info->primary_vout_height = mPersistMediaConfig.dsp_config.voutConfigs.voutConfig[mInitialVideoDisplayPrimaryIndex].height;
                } else {
                    LOGM_ERROR("BAD msState %d\n", msState);
                    return EECode_BadState;
                }
            }
            break;

        case EGenericEngineQuery_VideoDSPStatus: {
                SDSPControlStatus status = {0};
                DASSERT(mpDSPAPI);
                if (!mpDSPAPI || !param) {
                    LOGM_ERROR("NULL pointer mpDSPAPI %p or param %p.\n", mpDSPAPI, param);
                    return EECode_BadParam;
                }
                status.decoder_id = *((TU32 *)param);
                err = mpDSPAPI->DSPControl(EDSPControlType_UDEC_get_status, (void *)&status);
                if (EECode_OK == err) {
                    LOGM_ALWAYS("     udec(%u) info:\n", status.decoder_id);
                    LOGM_ALWAYS("     udec_state(%d), vout_state(%d), error_code(0x%08x).\n", status.dec_state, status.vout_state, status.error_code);
                    LOGM_ALWAYS("     bsb info: phys start addr 0x%08x, total size %u(0x%08x), free space %u\n", status.bits_fifo_phys_start, status.bits_fifo_total_size, status.bits_fifo_total_size, status.bits_fifo_free_size);

                    LOGM_ALWAYS("     dsp read pointer from msg: (phys) %p, diff from start %p, map to usr space %p.\n", status.dsp_current_read_bitsfifo_addr_phys, status.dsp_current_read_bitsfifo_addr_phys - status.bits_fifo_phys_start, status.dsp_current_read_bitsfifo_addr);
                    LOGM_ALWAYS("     last arm write pointer: (phys) %p, diff from start %p, map to usr space %p.\n", status.arm_last_write_bitsfifo_addr_phys, status.arm_last_write_bitsfifo_addr_phys - status.bits_fifo_phys_start, status.arm_last_write_bitsfifo_addr);

                    LOGM_ALWAYS("     tot send decode cmd %d, tot frame count %d.\n", status.tot_decode_cmd_cnt, status.tot_decode_frame_cnt);
                }
            }
            break;

        case EGenericEngineConfigure_VideoEncoder_Params: {
                SComponent_Obsolete *p_video_encoder;
                SGenericVideoEncoderParam *videoEncParas = (SGenericVideoEncoderParam *)param;
                DASSERT(videoEncParas);
                DASSERT(EGenericEngineConfigure_VideoEncoder_Params == videoEncParas->check_field);

                p_video_encoder = queryFilter(EGenericComponentType_VideoEncoder, 0);
                DASSERT(p_video_encoder);
                DASSERT(EComponentState_running == p_video_encoder->cur_state);
                if (EComponentState_running != p_video_encoder->cur_state) {
                    LOGM_ERROR("component BAD state %d\n", p_video_encoder->cur_state);
                    return EECode_BadState;
                }
                DASSERT(p_video_encoder->p_filter);
                p_video_encoder->p_filter->FilterProperty(EFilterPropertyType_video_parameters, videoEncParas->set_or_get, (void *)&videoEncParas->video_params);
            }
            break;

        case EGenericEngineConfigure_VideoEncoder_Bitrate: {
                SComponent_Obsolete *p_video_encoder;
                SGenericVideoEncoderParam *videoEncParas = (SGenericVideoEncoderParam *)param;
                DASSERT(videoEncParas);
                DASSERT(EGenericEngineConfigure_VideoEncoder_Bitrate == videoEncParas->check_field);

                p_video_encoder = queryFilter(EGenericComponentType_VideoEncoder, 0);
                //DASSERT(p_video_encoder);
                if (DUnlikely(!p_video_encoder)) {
                    LOGM_ERROR("no video encoder filter found\n");
                    return EECode_BadState;
                }
                DASSERT(EComponentState_running == p_video_encoder->cur_state);
                if (EComponentState_running != p_video_encoder->cur_state) {
                    LOGM_ERROR("component BAD state %d\n", p_video_encoder->cur_state);
                    return EECode_BadState;
                }
                DASSERT(p_video_encoder->p_filter);
                p_video_encoder->p_filter->FilterProperty(EFilterPropertyType_video_bitrate, videoEncParas->set_or_get, (void *)&videoEncParas->video_params);
            }
            break;

        case EGenericEngineConfigure_VideoEncoder_Framerate: {
                SComponent_Obsolete *p_video_encoder;
                SGenericVideoEncoderParam *videoEncParas = (SGenericVideoEncoderParam *)param;
                DASSERT(videoEncParas);
                DASSERT(EGenericEngineConfigure_VideoEncoder_Framerate == videoEncParas->check_field);

                p_video_encoder = queryFilter(EGenericComponentType_VideoEncoder, 0);
                DASSERT(p_video_encoder);
                DASSERT(EComponentState_running == p_video_encoder->cur_state);
                if (EComponentState_running != p_video_encoder->cur_state) {
                    LOGM_ERROR("component BAD state %d\n", p_video_encoder->cur_state);
                    return EECode_BadState;
                }
                DASSERT(p_video_encoder->p_filter);
                p_video_encoder->p_filter->FilterProperty(EFilterPropertyType_video_framerate, videoEncParas->set_or_get, (void *)&videoEncParas->video_params);
            }
            break;

        case EGenericEngineConfigure_VideoEncoder_DemandIDR: {
                SComponent_Obsolete *p_video_encoder;
                SGenericVideoEncoderParam *videoEncParas = (SGenericVideoEncoderParam *)param;
                DASSERT(videoEncParas);
                DASSERT(EGenericEngineConfigure_VideoEncoder_DemandIDR == videoEncParas->check_field);

                p_video_encoder = queryFilter(EGenericComponentType_VideoEncoder, 0);
                DASSERT(p_video_encoder);
                DASSERT(EComponentState_running == p_video_encoder->cur_state);
                if (EComponentState_running != p_video_encoder->cur_state) {
                    LOGM_ERROR("component BAD state %d\n", p_video_encoder->cur_state);
                    return EECode_BadState;
                }
                DASSERT(p_video_encoder->p_filter);
                p_video_encoder->p_filter->FilterProperty(EFilterPropertyType_video_demandIDR, videoEncParas->set_or_get, (void *)&videoEncParas->video_params);
            }
            break;

        case EGenericEngineConfigure_VideoEncoder_GOP: {
                SComponent_Obsolete *p_video_encoder;
                SGenericVideoEncoderParam *videoEncParas = (SGenericVideoEncoderParam *)param;
                DASSERT(videoEncParas);
                DASSERT(EGenericEngineConfigure_VideoEncoder_GOP == videoEncParas->check_field);

                p_video_encoder = queryFilter(EGenericComponentType_VideoEncoder, 0);
                DASSERT(p_video_encoder);
                DASSERT(EComponentState_running == p_video_encoder->cur_state);
                if (EComponentState_running != p_video_encoder->cur_state) {
                    LOGM_ERROR("component BAD state %d\n", p_video_encoder->cur_state);
                    return EECode_BadState;
                }
                DASSERT(p_video_encoder->p_filter);
                p_video_encoder->p_filter->FilterProperty(EFilterPropertyType_video_gop, videoEncParas->set_or_get, (void *)&videoEncParas->video_params);
            }
            break;

        case EGenericEngineConfigure_Record_Saving_Strategy: {
                SGenericRecordSavingStrategy *rec = (SGenericRecordSavingStrategy *)param;
                DASSERT(rec);
                DASSERT(EGenericEngineConfigure_Record_Saving_Strategy == rec->check_field);

                err = RECSavingStrategy(rec->rec_pipeline_id, rec->rec_saving_strategy.strategy, rec->rec_saving_strategy.condition, rec->rec_saving_strategy.naming, rec->rec_saving_strategy.param);
            }
            break;

        case EGenericEngineConfigure_VideoDisplay_SwitchWindowToHD: {
                SVideoPostPDisplayHD *phd = (SVideoPostPDisplayHD *)param;
                TU8 check_layer_index = 0;
                DASSERT(phd);
                DASSERT(EGenericEngineConfigure_VideoDisplay_SwitchWindowToHD == phd->check_field);
                DASSERT(EDisplayLayout_TelePresence == mInitalVideoPostPDisplayLayout.layer_layout_type[check_layer_index]);
                DASSERT(0 == phd->sd_window_index_for_hd);
                if (EDisplayLayout_TelePresence != mInitalVideoPostPDisplayLayout.layer_layout_type[check_layer_index]
                        || 0 != phd->sd_window_index_for_hd) {
                    LOGM_ERROR("EGenericEngineConfigure_VideoDisplay_SwitchWindowToHD only supported on sd window 0 in EDisplayLayout_TelePresence.\n");
                    return EECode_BadParam;
                }
                if (mbCertainSDWindowSwitchedtoHD == phd->tobeHD) {
                    LOGM_ERROR("EGenericEngineConfigure_VideoDisplay_SwitchWindowToHD, sd window already in %s mode, no need to switch again.\n", mbCertainSDWindowSwitchedtoHD ? "HD" : "SD");
                    return EECode_BadState;
                }

                if (mpUniqueVideoPostProcessor) {
                    SVideoPostPConfiguration video_postp_info;

                    memset(&video_postp_info, 0x0, sizeof(video_postp_info));
                    err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_query_display, 1, &video_postp_info);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_vpostp_query_display return %d\n", err);
                        break;
                    }

                    DASSERT(video_postp_info.layer_number <= DMaxDisplayLayerNumber);
                    if (phd->tobeHD) {
                        LOGM_INFO("suspend vid-decode [%d ~ %d], vid-demux [%d ~ %d]\n", phd->sd_window_index_for_hd, phd->sd_window_index_for_hd + 1,
                                  phd->sd_window_index_for_hd + mCurSourceGroupIndex * video_postp_info.layer_window_number[check_layer_index] * 2, phd->sd_window_index_for_hd + 1 + mCurSourceGroupIndex * video_postp_info.layer_window_number[check_layer_index] * 2);
                        suspendPlaybackPipelines(phd->sd_window_index_for_hd, phd->sd_window_index_for_hd + 1,
                                                 phd->sd_window_index_for_hd + mCurSourceGroupIndex * video_postp_info.layer_window_number[check_layer_index] * 2, phd->sd_window_index_for_hd + 1 + mCurSourceGroupIndex * video_postp_info.layer_window_number[check_layer_index] * 2, phd->flush_udec);
                        LOGM_INFO("resume suspend vid-decode [%d ~ %d], vid-demux [%d ~ %d], layer->sd_window_index_for_hd %d, focus_input_index = %u\n",
                                  video_postp_info.layer_window_number[check_layer_index],
                                  video_postp_info.layer_window_number[check_layer_index] + 1,
                                  video_postp_info.layer_window_number[check_layer_index] + phd->sd_window_index_for_hd + mCurSourceGroupIndex * video_postp_info.layer_window_number[check_layer_index] * 2,
                                  video_postp_info.layer_window_number[check_layer_index] + phd->sd_window_index_for_hd + 1 + mCurSourceGroupIndex * video_postp_info.layer_window_number[check_layer_index] * 2,
                                  phd->sd_window_index_for_hd,
                                  (mCurSourceGroupIndex * video_postp_info.layer_window_number[check_layer_index]) + phd->sd_window_index_for_hd);
                        err = resumeSuspendPlaybackPipelines(video_postp_info.layer_window_number[check_layer_index], video_postp_info.layer_window_number[check_layer_index] + 1,
                                                             video_postp_info.layer_window_number[check_layer_index] + phd->sd_window_index_for_hd + mCurSourceGroupIndex * video_postp_info.layer_window_number[check_layer_index] * 2, video_postp_info.layer_window_number[check_layer_index] + phd->sd_window_index_for_hd + 1 + mCurSourceGroupIndex * video_postp_info.layer_window_number[check_layer_index] * 2,
                                                             (mCurSourceGroupIndex * video_postp_info.layer_window_number[check_layer_index]) + phd->sd_window_index_for_hd);
                        if (EECode_OK != err) {
                            LOGM_FATAL("resumeSuspendPlaybackPipelines() return %d\n", err);
                            break;
                        }
                    } else {
                        LOGM_INFO("suspend vid-decode [%d ~ %d], vid-demux [%d ~ %d]\n",
                                  video_postp_info.layer_window_number[check_layer_index], video_postp_info.layer_window_number[check_layer_index] + 1,
                                  video_postp_info.layer_window_number[check_layer_index] + phd->sd_window_index_for_hd + mCurSourceGroupIndex * video_postp_info.layer_window_number[check_layer_index] * 2, video_postp_info.layer_window_number[check_layer_index] + phd->sd_window_index_for_hd + 1 + mCurSourceGroupIndex * video_postp_info.layer_window_number[check_layer_index] * 2);
                        suspendPlaybackPipelines(video_postp_info.layer_window_number[check_layer_index], video_postp_info.layer_window_number[check_layer_index] + 1,
                                                 video_postp_info.layer_window_number[check_layer_index] + phd->sd_window_index_for_hd + mCurSourceGroupIndex * video_postp_info.layer_window_number[check_layer_index] * 2, video_postp_info.layer_window_number[check_layer_index] + phd->sd_window_index_for_hd + 1 + mCurSourceGroupIndex * video_postp_info.layer_window_number[check_layer_index] * 2,
                                                 phd->flush_udec);
                        LOGM_INFO("resume suspend vid-decode [%d ~ %d], vid-demux [%d ~ %d], layer->sd_window_index_for_hd %d, focus_input_index = %u\n",
                                  phd->sd_window_index_for_hd,
                                  phd->sd_window_index_for_hd + 1,
                                  phd->sd_window_index_for_hd + mCurSourceGroupIndex * video_postp_info.layer_window_number[check_layer_index] * 2,
                                  phd->sd_window_index_for_hd + 1 + mCurSourceGroupIndex * video_postp_info.layer_window_number[check_layer_index] * 2,
                                  phd->sd_window_index_for_hd,
                                  mCurSourceGroupIndex);
                        err = resumeSuspendPlaybackPipelines(phd->sd_window_index_for_hd, phd->sd_window_index_for_hd + 1,
                                                             phd->sd_window_index_for_hd + mCurSourceGroupIndex * video_postp_info.layer_window_number[check_layer_index] * 2, phd->sd_window_index_for_hd + 1 + mCurSourceGroupIndex * video_postp_info.layer_window_number[check_layer_index] * 2, mCurSourceGroupIndex);
                        if (EECode_OK != err) {
                            LOGM_FATAL("resumeSuspendPlaybackPipelines() return %d\n", err);
                            break;
                        }
                    }
                    err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_switch_window_to_HD, 1, param);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_vpostp_switch_window_to_HD return %d\n", err);
                        break;
                    }
                    mbCertainSDWindowSwitchedtoHD = phd->tobeHD;
                } else {
                    LOGM_FATAL("No video processor\n");
                    err = EECode_BadState;
                }
                LOGM_NOTICE("after EFilterPropertyType_vpostp_switch_window_to_HD\n");
            }
            break;

        case EGenericEngineConfigure_VideoDisplay_PostPConfiguration: {
                SGenericVideoPostPConfiguration *ppconfig = (SGenericVideoPostPConfiguration *)param;
                DASSERT(ppconfig);
                DASSERT(EGenericEngineConfigure_VideoDisplay_PostPConfiguration == ppconfig->check_field);
                DASSERT(ppconfig->p_video_postp_info);
                if (!ppconfig->p_video_postp_info) {
                    LOGM_ERROR("EGenericEngineConfigure_VideoDisplay_PostPConfiguration invalid para, p_video_postp_info=%p.\n", ppconfig->p_video_postp_info);
                    return EECode_BadParam;
                }

                if (mpUniqueVideoPostProcessor) {
                    err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_configure, ppconfig->set_or_get, ppconfig->p_video_postp_info);
                    DASSERT_OK(err);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_vpostp_configure %s return %d\n", ppconfig->set_or_get ? "set" : "get", err);
                        break;
                    }
                } else {
                    LOGM_FATAL("No video processor\n");
                    err = EECode_BadState;
                }
                LOGM_NOTICE("after EGenericEngineConfigure_VideoDisplay_PostPConfiguration %s\n", ppconfig->set_or_get ? "set" : "get");
            }
            break;

        default: {
                LOGM_FATAL("Unknown config_type 0x%8x\n", (TUint)config_type);
                err = EECode_BadCommand;
            }
            break;
    }

    return err;
}

EECode CActiveGenericEngine::GenericPreConfigure(EGenericEnginePreConfigure config_type, void *param, TUint is_get)
{
    EECode err = EECode_OK;

    DASSERT(param);
    if (!param) {
        LOGM_FATAL("NULL param\n");
        return EECode_BadParam;
    }

    switch (config_type) {

        case EGenericEnginePreConfigure_NetworkReceiverSchedulerNumber: {
                SGenericPreConfigure *pre_config = (SGenericPreConfigure *) param;
                DASSERT(EGenericEnginePreConfigure_NetworkReceiverSchedulerNumber == pre_config->check_field);

                if (!is_get) {
                    mPersistMediaConfig.number_scheduler_network_reciever = pre_config->number;
                } else {
                    pre_config->number = mPersistMediaConfig.number_scheduler_network_reciever;
                }
            }
            break;

        case EGenericEnginePreConfigure_NetworkTransmiterSchedulerNumber: {
                SGenericPreConfigure *pre_config = (SGenericPreConfigure *) param;
                DASSERT(EGenericEnginePreConfigure_NetworkTransmiterSchedulerNumber == pre_config->check_field);

                if (!is_get) {
                    mPersistMediaConfig.number_scheduler_network_sender = pre_config->number;
                } else {
                    pre_config->number = mPersistMediaConfig.number_scheduler_network_sender;
                }
            }
            break;

        case EGenericEnginePreConfigure_IOWriterSchedulerNumber: {
                SGenericPreConfigure *pre_config = (SGenericPreConfigure *) param;
                DASSERT(EGenericEnginePreConfigure_IOWriterSchedulerNumber == pre_config->check_field);

                if (!is_get) {
                    mPersistMediaConfig.number_scheduler_io_writer = pre_config->number;
                } else {
                    pre_config->number = mPersistMediaConfig.number_scheduler_io_writer;
                }
            }
            break;

        case EGenericEnginePreConfigure_IOReaderSchedulerNumber: {
                SGenericPreConfigure *pre_config = (SGenericPreConfigure *) param;
                DASSERT(EGenericEnginePreConfigure_IOReaderSchedulerNumber == pre_config->check_field);

                if (!is_get) {
                    mPersistMediaConfig.number_scheduler_io_reader = pre_config->number;
                } else {
                    pre_config->number = mPersistMediaConfig.number_scheduler_io_reader;
                }
            }
            break;

        case EGenericEnginePreConfigure_VideoDecoderSchedulerNumber: {
                SGenericPreConfigure *pre_config = (SGenericPreConfigure *) param;
                DASSERT(EGenericEnginePreConfigure_VideoDecoderSchedulerNumber == pre_config->check_field);

                if (!is_get) {
                    mPersistMediaConfig.number_scheduler_video_decoder = pre_config->number;
                } else {
                    pre_config->number = mPersistMediaConfig.number_scheduler_video_decoder;
                }
            }
            break;

        case EGenericEnginePreConfigure_PlaybackSetPreFetch: {
                SGenericPreConfigure *pre_config = (SGenericPreConfigure *) param;
                DASSERT(EGenericEnginePreConfigure_PlaybackSetPreFetch == pre_config->check_field);

                if (!is_get) {
                    mPersistMediaConfig.playback_prefetch_number = pre_config->number;
                    LOGM_CONFIGURATION("EGenericEnginePreConfigure_PlaybackSetPreFetch %d\n", mPersistMediaConfig.playback_prefetch_number);
                } else {
                    pre_config->number = mPersistMediaConfig.playback_prefetch_number;
                }
            }
            break;

        case EGenericEnginePreConfigure_PlaybackSetDSPDecoderBufferNumber: {
                SGenericPreConfigure *pre_config = (SGenericPreConfigure *) param;
                TUint i = 0;
                DASSERT(EGenericEnginePreConfigure_PlaybackSetDSPDecoderBufferNumber == pre_config->check_field);

                if (!is_get) {
                    if (mPersistMediaConfig.dsp_config.max_frm_num != pre_config->number) {
                        if ((pre_config->number < 7) || (pre_config->number > 20)) {
                            LOGM_ERROR("buffer number(%d) should not excced 7~20\n", pre_config->number);
                            return EECode_BadParam;
                        }
                        mPersistMediaConfig.dsp_config.max_frm_num = pre_config->number;
                        LOGM_CONFIGURATION("EGenericEnginePreConfigure_PlaybackSetDSPDecoderBufferNumber %d\n", mPersistMediaConfig.dsp_config.max_frm_num);
                        for (i = 0; i < DMaxUDECInstanceNumber; i ++) {
                            mPersistMediaConfig.dsp_config.udecInstanceConfig[i].max_frm_num = mPersistMediaConfig.dsp_config.max_frm_num;
                        }

                        if (mPersistMediaConfig.dsp_config.modeConfigMudec.enable_buffering_ctrl) {
                            if ((mPersistMediaConfig.dsp_config.modeConfigMudec.pre_buffer_len + 5) > (mPersistMediaConfig.dsp_config.max_frm_num)) {
                                mPersistMediaConfig.dsp_config.modeConfigMudec.pre_buffer_len = mPersistMediaConfig.dsp_config.max_frm_num - 5;
                                LOGM_WARN("pre buffer len exceed, set it to %d\n", mPersistMediaConfig.dsp_config.modeConfigMudec.pre_buffer_len);
                            }
                        }
                    }
                } else {
                    pre_config->number = mPersistMediaConfig.dsp_config.max_frm_num;
                }
            }
            break;

        case EGenericEnginePreConfigure_PlaybackSetDSPPrebuffer: {
                SGenericPreConfigure *pre_config = (SGenericPreConfigure *) param;
                DASSERT(EGenericEnginePreConfigure_PlaybackSetDSPPrebuffer == pre_config->check_field);

                if (!is_get) {
                    if (pre_config->number) {
                        if ((pre_config->number + 5) <= (mPersistMediaConfig.dsp_config.max_frm_num)) {
                            mPersistMediaConfig.dsp_config.modeConfigMudec.pre_buffer_len = pre_config->number;
                            LOGM_CONFIGURATION("pre buffer len %d\n", mPersistMediaConfig.dsp_config.modeConfigMudec.pre_buffer_len);
                        } else {
                            LOGM_ERROR("pre_config->number (%d) should not excced max buffer number(%d) - 5\n", pre_config->number, mPersistMediaConfig.dsp_config.max_frm_num);
                            return EECode_BadParam;
                        }
                        mPersistMediaConfig.dsp_config.modeConfigMudec.enable_buffering_ctrl = 1;
                    } else {
                        mPersistMediaConfig.dsp_config.modeConfigMudec.enable_buffering_ctrl = 0;
                        mPersistMediaConfig.dsp_config.modeConfigMudec.pre_buffer_len = 0;
                        LOGM_CONFIGURATION("disable dsp prebuffer\n");
                    }
                } else {
                    pre_config->number = mPersistMediaConfig.dsp_config.modeConfigMudec.pre_buffer_len;
                }
            }
            break;

        case EGenericEnginePreConfigure_SetStreamingTimeoutThreashold: {
                SGenericPreConfigure *pre_config = (SGenericPreConfigure *) param;
                DASSERT(EGenericEnginePreConfigure_SetStreamingTimeoutThreashold == pre_config->check_field);

                if (!is_get) {
                    mPersistMediaConfig.streaming_timeout_threashold = 1000000 * pre_config->number;
                    LOGM_CONFIGURATION("EGenericEnginePreConfigure_SetStreamingTimeoutThreashold %d\n", pre_config->number);
                } else {
                    pre_config->number = mPersistMediaConfig.streaming_timeout_threashold / 1000000;
                }
            }
            break;

        case EGenericEnginePreConfigure_StreamingRetryConnectServerInterval: {
                SGenericPreConfigure *pre_config = (SGenericPreConfigure *) param;
                DASSERT(EGenericEnginePreConfigure_StreamingRetryConnectServerInterval == pre_config->check_field);

                if (!is_get) {
                    mPersistMediaConfig.streaming_autoconnect_interval = pre_config->number * 1000000;
                    LOGM_CONFIGURATION("EGenericEnginePreConfigure_StreamingRetryConnectServerInterval %d\n", pre_config->number);
                } else {
                    pre_config->number = mPersistMediaConfig.streaming_autoconnect_interval / 1000000;
                }
            }
            break;

        case EGenericEnginePreConfigure_RTSPStreamingServerPort: {
                SGenericPreConfigure *pre_config = (SGenericPreConfigure *) param;
                DASSERT(EGenericEnginePreConfigure_RTSPStreamingServerPort == pre_config->check_field);

                if (!is_get) {
                    mPersistMediaConfig.rtsp_server_config.rtsp_listen_port = pre_config->number;
                    LOGM_CONFIGURATION("EGenericEnginePreConfigure_RTSPStreamingServerPort %d\n", pre_config->number);
                } else {
                    pre_config->number = mPersistMediaConfig.rtsp_server_config.rtsp_listen_port;
                }
            }
            break;

        case EGenericEnginePreConfigure_RTSPClientTryTCPModeFirst: {
                SGenericPreConfigure *pre_config = (SGenericPreConfigure *) param;
                DASSERT(EGenericEnginePreConfigure_RTSPClientTryTCPModeFirst == pre_config->check_field);

                if (!is_get) {
                    mPersistMediaConfig.rtsp_client_config.use_tcp_mode_first = pre_config->number;
                    LOGM_CONFIGURATION("EGenericEnginePreConfigure_RTSPClientTryTCPModeFirst %d\n", pre_config->number);
                } else {
                    pre_config->number = mPersistMediaConfig.rtsp_client_config.use_tcp_mode_first;
                }
            }
            break;

        case EGenericEnginePreConfigure_SACPCloudServerPort: {
                SGenericPreConfigure *pre_config = (SGenericPreConfigure *) param;
                DASSERT(EGenericEnginePreConfigure_SACPCloudServerPort == pre_config->check_field);

                if (!is_get) {
                    mPersistMediaConfig.sacp_cloud_server_config.sacp_listen_port = pre_config->number;
                    LOGM_CONFIGURATION("EGenericEnginePreConfigure_SACPCloudServerPort %d\n", pre_config->number);
                } else {
                    pre_config->number = mPersistMediaConfig.sacp_cloud_server_config.sacp_listen_port;
                }
            }

        case EGenericEnginePreConfigure_DSPAddUDECWrapper: {
                SGenericPreConfigure *pre_config = (SGenericPreConfigure *) param;
                DASSERT(EGenericEnginePreConfigure_DSPAddUDECWrapper == pre_config->check_field);

                if (!is_get) {
                    mPersistMediaConfig.dsp_config.not_add_udec_wrapper = pre_config->number;
                    LOGM_CONFIGURATION("EGenericEnginePreConfigure_DSPAddUDECWrapper, mPersistMediaConfig.dsp_config.not_add_udec_wrapper %d\n", mPersistMediaConfig.dsp_config.not_add_udec_wrapper);
                } else {
                    pre_config->number = mPersistMediaConfig.dsp_config.not_add_udec_wrapper;
                }
            }
            break;

        case EGenericEnginePreConfigure_DSPFeedPTS: {
                SGenericPreConfigure *pre_config = (SGenericPreConfigure *) param;
                DASSERT(EGenericEnginePreConfigure_DSPFeedPTS == pre_config->check_field);

                if (!is_get) {
                    mPersistMediaConfig.dsp_config.not_feed_pts = pre_config->number;
                    LOGM_CONFIGURATION("EGenericEnginePreConfigure_DSPFeedPTS, mPersistMediaConfig.dsp_config.not_feed_pts %d\n", mPersistMediaConfig.dsp_config.not_feed_pts);
                } else {
                    pre_config->number = mPersistMediaConfig.dsp_config.not_feed_pts;
                }
            }
            break;

        case EGenericEnginePreConfigure_DSPSpecifyFPS: {
                SGenericPreConfigure *pre_config = (SGenericPreConfigure *) param;
                DASSERT(EGenericEnginePreConfigure_DSPSpecifyFPS == pre_config->check_field);

                if (!is_get) {
                    mPersistMediaConfig.dsp_config.specified_fps = pre_config->number;
                    LOGM_CONFIGURATION("EGenericEnginePreConfigure_DSPSpecifyFPS, mPersistMediaConfig.dsp_config.specified_fps %d\n", mPersistMediaConfig.dsp_config.specified_fps);
                } else {
                    pre_config->number = mPersistMediaConfig.dsp_config.specified_fps;
                }
            }
            break;

        case EGenericEnginePreConfigure_PreferModule_Muxer_FFMpeg: {
                mPersistMediaConfig.prefer_module_setting.mPreferedMuxerModule = EMuxerModuleID_FFMpeg;
            }
            break;

        case EGenericEnginePreConfigure_PreferModule_Muxer_TS: {
                mPersistMediaConfig.prefer_module_setting.mPreferedMuxerModule = EMuxerModuleID_PrivateTS;
            }
            break;

        case EGenericEnginePreConfigure_PreferModule_AudioDecoder_FFMpeg: {
                mPersistMediaConfig.prefer_module_setting.mPreferedAudioDecoderModule = EAudioDecoderModuleID_FFMpeg;
            }
            break;

        case EGenericEnginePreConfigure_PreferModule_AudioDecoder_AAC: {
                mPersistMediaConfig.prefer_module_setting.mPreferedAudioDecoderModule = EAudioDecoderModuleID_AAC;
            }
            break;

        case EGenericEnginePreConfigure_PreferModule_AudioEncoder_FFMpeg: {
                mPersistMediaConfig.prefer_module_setting.mPreferedAudioEncoderModule = EAudioEncoderModuleID_FFMpeg;
            }
            break;

        case EGenericEnginePreConfigure_PreferModule_AudioEncoder_AAC: {
                mPersistMediaConfig.prefer_module_setting.mPreferedAudioEncoderModule = EAudioEncoderModuleID_PrivateLibAAC;
            }
            break;

        default: {
                LOGM_FATAL("Unknown config_type 0x%8x\n", (TUint)config_type);
                err = EECode_BadCommand;
            }
            break;
    }

    return err;
}

EECode CActiveGenericEngine::RECStop(TGenericID rec_pipeline)
{
    SGenericPipeline *pipeline = queryPipeline(rec_pipeline);
    TUint i = 0;

    SComponent_Obsolete *p_component = NULL;
    SConnection_Obsolete *p_connection = NULL;
    EECode err;

    //debug assert
    DASSERT(pipeline);

    if (!pipeline) {
        LOGM_ERROR("NULL pointer pipeline %p, pipeline_id %0x08x\n", pipeline, rec_pipeline);
        return EECode_BadParam;
    }

    DASSERT(EGenericPipelineType_Recording == pipeline->pipeline_type);
    if (EGenericPipelineType_Recording != pipeline->pipeline_type) {
        LOGM_ERROR("you should input a rec_pipeline id, 0x%08x is not a rec_pipeline id\n", rec_pipeline);
        return EECode_BadParam;
    }

    if (EGenericPipelineState_running != pipeline->pipeline_state) {
        LOGM_ERROR("recording pipeline is not in running state, state %d\n", pipeline->pipeline_state);
        return EECode_BadState;
    }

    LOGM_INFO("CActiveGenericEngine::RECStop start.\n");

    //disconnect pins
    for (i = 0; i < pipeline->rec_source_number; i ++) {
        p_connection = queryConnection(pipeline->rec_source_pin_id[i]);
        p_component = queryFilter(pipeline->rec_source_id[i]);
        DASSERT(p_connection);
        DASSERT(p_component);
        if (p_component && p_connection) {
            DASSERT(p_connection->up_id == pipeline->rec_source_id[i]);
            DASSERT(p_connection->up == p_component);
            DASSERT(p_component->p_filter);

            SEnableConnection enable_connection;
            enable_connection.up_pin_index = p_connection->up_pin_index;
            enable_connection.up_pin_sub_index = p_connection->up_pin_sub_index;
            err = p_component->p_filter->FilterProperty(EFilterPropertyType_enable_output_pin, 1, (void *)&enable_connection);
            DASSERT(EECode_OK == err);
        }
    }

    //stop muxer
    p_component = queryFilter(pipeline->rec_sink_id);
    DASSERT(p_component);
    if (p_component) {
        DASSERT(p_component->p_filter);

        err = p_component->p_filter->FilterProperty(EFilterPropertyType_muxer_stop, 1, NULL);
        DASSERT(EECode_OK == err);
    }

    LOGM_INFO("CActiveGenericEngine::RECStop done\n");
    //pipeline->pipeline_state = EGenericPipelineState_idle;
    return EECode_OK;
}

EECode CActiveGenericEngine::RECStart(TGenericID rec_pipeline)
{
    SGenericPipeline *pipeline = queryPipeline(rec_pipeline);
    TUint i = 0;

    SComponent_Obsolete *p_component = NULL;
    SConnection_Obsolete *p_connection = NULL;
    EECode err;

    //debug assert
    DASSERT(pipeline);

    if (!pipeline) {
        LOGM_ERROR("NULL pointer pipeline %p, pipeline_id %0x08x\n", pipeline, rec_pipeline);
        return EECode_BadParam;
    }

    DASSERT(EGenericPipelineType_Recording == pipeline->pipeline_type);
    if (EGenericPipelineType_Recording != pipeline->pipeline_type) {
        LOGM_ERROR("you should input a rec_pipeline id, 0x%08x is not a rec_pipeline id\n", rec_pipeline);
        return EECode_BadParam;
    }

    //if (EGenericPipelineState_idle != pipeline->pipeline_state) {
    //    LOGM_ERROR("recording pipeline is not in idle state, state %d\n", pipeline->pipeline_state);
    //    return EECode_BadState;
    //}

    LOGM_INFO("CActiveGenericEngine::RECStart start.\n");

    //disconnect pins
    for (i = 0; i < pipeline->rec_source_number; i ++) {
        p_connection = queryConnection(pipeline->rec_source_pin_id[i]);
        p_component = queryFilter(pipeline->rec_source_id[i]);
        DASSERT(p_connection);
        DASSERT(p_component);
        if (p_component && p_connection) {
            DASSERT(p_connection->up_id == pipeline->rec_source_id[i]);
            DASSERT(p_connection->up == p_component);
            DASSERT(p_component->p_filter);

            SEnableConnection enable_connection;
            enable_connection.up_pin_index = p_connection->up_pin_index;
            enable_connection.up_pin_sub_index = p_connection->up_pin_sub_index;
            err = p_component->p_filter->FilterProperty(EFilterPropertyType_enable_output_pin, 1, (void *)&enable_connection);
            DASSERT_OK(err);
        }
    }

    //start muxer
    p_component = queryFilter(pipeline->rec_sink_id);
    DASSERT(p_component);
    if (p_component) {
        DASSERT(p_component->p_filter);

        err = p_component->p_filter->FilterProperty(EFilterPropertyType_muxer_start, 1, NULL);
        DASSERT(EECode_OK == err);
    }

    LOGM_INFO("CActiveGenericEngine::RECStart done\n");
    pipeline->pipeline_state = EGenericPipelineState_running;
    return EECode_OK;
}

EECode CActiveGenericEngine::RECMuxerFinalizeFile(TGenericID rec_pipeline)
{
    SGenericPipeline *pipeline = queryPipeline(rec_pipeline);
    SComponent_Obsolete *p_component = NULL;

    EECode err;

    //debug assert
    DASSERT(pipeline);

    if (!pipeline) {
        LOGM_ERROR("NULL pointer pipeline %p, pipeline_id %0x08x\n", pipeline, rec_pipeline);
        return EECode_BadParam;
    }

    DASSERT(EGenericPipelineType_Recording == pipeline->pipeline_type);
    if (EGenericPipelineType_Recording != pipeline->pipeline_type) {
        LOGM_ERROR("you should input a rec_pipeline id, 0x%08x is not a rec_pipeline id\n", rec_pipeline);
        return EECode_BadParam;
    }

    if (EGenericPipelineState_running != pipeline->pipeline_state) {
        LOGM_ERROR("recording pipeline is not in idle state, state %d\n", pipeline->pipeline_state);
        return EECode_BadState;
    }

    p_component = queryFilter(pipeline->rec_sink_id);
    DASSERT(p_component);
    if (p_component) {
        DASSERT(p_component->p_filter);

        err = p_component->p_filter->FilterProperty(EFilterPropertyType_muxer_finalize_file, 1, NULL);
        DASSERT(EECode_OK == err);
    }

    LOGM_INFO("CActiveGenericEngine::RECMuxerFinalizeFile done\n");
    return EECode_OK;
}

EECode CActiveGenericEngine::RECSavingStrategy(TGenericID rec_pipeline, MuxerSavingFileStrategy strategy, MuxerSavingCondition condition, MuxerAutoFileNaming naming, TUint param)
{
    SGenericPipeline *pipeline = queryPipeline(rec_pipeline);
    SComponent_Obsolete *p_component = NULL;

    EECode err;

    //debug assert
    DASSERT(pipeline);

    if (!pipeline) {
        LOGM_ERROR("NULL pointer pipeline %p, pipeline_id %0x08x\n", pipeline, rec_pipeline);
        return EECode_BadParam;
    }

    DASSERT(EGenericPipelineType_Recording == pipeline->pipeline_type);
    if (EGenericPipelineType_Recording != pipeline->pipeline_type) {
        LOGM_ERROR("you should input a rec_pipeline id, 0x%08x is not a rec_pipeline id\n", rec_pipeline);
        return EECode_BadParam;
    }

    /*if (EGenericPipelineState_running != pipeline->pipeline_state) {
        LOGM_ERROR("recording pipeline is not in running state, state %d\n", pipeline->pipeline_state);
        return EECode_BadState;
    }*///TODO: no RECStart now, so this check temp removed

    p_component = queryFilter(pipeline->rec_sink_id);
    DASSERT(p_component);
    if (p_component) {
        DASSERT(p_component->p_filter);

        SRecSavingStrategy rec_strategy;
        rec_strategy.strategy = strategy;
        rec_strategy.condition = condition;
        rec_strategy.naming = naming;
        rec_strategy.param = param;

        err = p_component->p_filter->FilterProperty(EFilterPropertyType_muxer_saving_strategy, 1, (void *)&rec_strategy);
        DASSERT(EECode_OK == err);
    }

    LOGM_INFO("CActiveGenericEngine::RECMuxerFinalizeFile done\n");
    return EECode_OK;
}

//pipeline related
EECode CActiveGenericEngine::SuspendPipeline(TGenericID pipeline_id)
{
    SGenericPipeline *pipeline = queryPipeline(pipeline_id);

    //debug assert
    DASSERT(pipeline);

    LOGM_FATAL("TO DO!\n");

    if (!pipeline) {
        LOGM_ERROR("NULL pointer pipeline %p, pipeline_id %0x08x\n", pipeline, pipeline_id);
        return EECode_BadParam;
    }

    if (EGenericPipelineState_suspended == pipeline->pipeline_state) {
        ;
    }

    if (EGenericPipelineType_Playback == pipeline->pipeline_type) {

    } else if (EGenericPipelineType_Recording == pipeline->pipeline_type) {

    }

    return EECode_OK;
}

EECode CActiveGenericEngine::ResumePipeline(TGenericID pipeline)
{
    return EECode_OK;
}

void CActiveGenericEngine::OnRun()
{
    SCMD cmd, nextCmd;
    SMSG msg;
    CIQueue::QType type;
    CIQueue::WaitResult result;
    DASSERT(mpFilterMsgQ);
    DASSERT(mpCmdQueue);
    mpWorkQ->CmdAck(EECode_OK);

    while (mbRun) {
        LOGM_FLOW("CActiveGenericEngine: msg cnt from filters %d.\n", mpFilterMsgQ->GetDataCnt());

        //wait user/app set data source
        type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);
        LOGM_DEBUG("wait data msg done\n");
        if (type == CIQueue::Q_MSG) {
            LOGM_DEBUG("processCMD\n");
            processCMD(cmd);
            LOGM_DEBUG("processCMD done\n");
        } else if (type == CIQueue::Q_DATA) {
            //msg from filters
            DASSERT(mpFilterMsgQ == result.pDataQ);
            if (result.pDataQ->PeekData(&msg, sizeof(msg))) {
                LOGM_DEBUG("processGenericMsg\n");
                if (!processGenericMsg(msg)) {
                    //need implement, process the msg
                    LOGM_ERROR("not-processed cmd %d.\n", msg.code);
                }
                LOGM_DEBUG("processGenericMsg done\n");
            }
        } else {
            LOGM_ERROR("Fatal error here.\n");
        }
    }

    LOGM_STATE("CActiveGenericEngine: start Clear gragh for safe.\n");
    NewSession();
    mpWorkQ->CmdAck(EECode_OK);
}

EECode CActiveGenericEngine::getParametersFromRecConfig()
{
    TUint out_index = 0, num = mPersistMediaConfig.tot_muxer_number;
    TUint stream_index;
    volatile STargetRecorderConfig *pConfig;
    if (num == 0) {
        LOGM_ERROR("Nothing read from rec.config? must not comes here.\n");
        return EECode_Error;
    }

    LOGM_INFO("Basic info: number of muxer %d\n", mPersistMediaConfig.tot_muxer_number);

    for (out_index = 0; out_index < num; out_index++) {

        pConfig = &mPersistMediaConfig.target_recorder_config[out_index];

        //video related, stream 0
        stream_index = 0;
        mMuxerConfig[out_index].stream_info[stream_index].stream_type = StreamType_Video;
        mMuxerConfig[out_index].stream_info[stream_index].stream_format = (StreamFormat) pConfig->video_format;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.pic_width = pConfig->pic_width;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.pic_height = pConfig->pic_height;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.framerate_num = pConfig->video_framerate_num;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.framerate_den = pConfig->video_framerate_den;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.sample_aspect_ratio_num = 1;//hard code
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.sample_aspect_ratio_den = 1;//hard code
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.bitrate = pConfig->video_bitrate;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.lowdelay = pConfig->video_lowdelay;

        //audio related, stream 1
        stream_index = 1;
        mMuxerConfig[out_index].stream_info[stream_index].stream_type = StreamType_Audio;
        mMuxerConfig[out_index].stream_info[stream_index].stream_format = (StreamFormat)pConfig->audio_format;
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.channel_layout = 0;//hard code
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.channel_number = pConfig->channel_number;
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.sample_rate = pConfig->sample_rate;
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.sample_format = (AudioSampleFMT) pConfig->sample_format;
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.frame_size = AUDIO_CHUNK_SIZE;//need set here?
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.bitrate = pConfig->audio_bitrate;
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.need_skip_adts_header = 0;

        //others streams, todo
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::defaultVideoParameters(TUint out_index, TUint stream_index, TUint ucode_stream_index)
{
    //safe check
    DASSERT((EEngineState_idle == msState) || (EEngineState_not_alive == msState));
    if ((EEngineState_idle != msState) && (EEngineState_not_alive == msState)) {
        LOGM_FATAL("defaultVideoParameters in bad engine state %d.\n", msState);
        return EECode_InternalLogicalBug;
    }

    DASSERT(out_index < mTotalMuxerNumber);
    if (out_index >= mTotalMuxerNumber) {
        LOGM_ERROR("BAD parameters in CActiveGenericEngine::defaultVideoParameters, out_index %d, stream index %d.\n", out_index, stream_index);
        return EECode_BadParam;
    }

    //if have input from external(rec.config, etc)
    if (out_index < mPersistMediaConfig.tot_muxer_number) {
        volatile STargetRecorderConfig *pConfig = &mPersistMediaConfig.target_recorder_config[out_index];
        mMuxerConfig[out_index].stream_info[stream_index].stream_type = StreamType_Video;
        mMuxerConfig[out_index].stream_info[stream_index].stream_format = (StreamFormat) pConfig->video_format;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.pic_width = pConfig->pic_width;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.pic_height = pConfig->pic_height;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.framerate_num = pConfig->video_framerate_num;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.framerate_den = pConfig->video_framerate_den;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.sample_aspect_ratio_num = 1;//hard code
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.sample_aspect_ratio_den = 1;//hard code
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.bitrate = pConfig->video_bitrate;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.lowdelay = pConfig->video_lowdelay;
        //LOGM_INFO("****!!!!pConfig->entropy_type %d.\n", pConfig->entropy_type);
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.entropy_type = pConfig->entropy_type;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.M = pConfig->M;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.N = pConfig->N;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.IDRInterval = pConfig->IDRInterval;
        return EECode_OK;
    }

    DASSERT(0);
    LOGM_WARN("should not comes here.\n");
    //if have no input from external(rec.config, etc), use some hard coded default value
    if (ucode_stream_index == 0) {
        //a5s main stream
        mMuxerConfig[out_index].stream_info[stream_index].stream_type = StreamType_Video;
        mMuxerConfig[out_index].stream_info[stream_index].stream_format = StreamFormat_H264;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.pic_width = DefaultMainVideoWidth;//1080p
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.pic_height = DefaultMainVideoHeight;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.framerate_num = DDefaultVideoFramerateNum;//29.97 fps
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.framerate_den = DDefaultVideoFramerateDen;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.sample_aspect_ratio_num = 1;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.sample_aspect_ratio_den = 1;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.bitrate = 8000000;//8M
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.lowdelay = 0;//have B picture
    } else {
        //a5s sub stream
        mMuxerConfig[out_index].stream_info[stream_index].stream_type = StreamType_Video;
        mMuxerConfig[out_index].stream_info[stream_index].stream_format = StreamFormat_H264;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.pic_width = DefaultPreviewCWidth;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.pic_height = DefaultPreviewCHeight;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.framerate_num = DDefaultVideoFramerateNum;//29.97 fps
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.framerate_den = DDefaultVideoFramerateDen;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.sample_aspect_ratio_num = 1;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.sample_aspect_ratio_den = 1;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.bitrate = 2000000;// 2M
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.lowdelay = 0;//have B picture
    }

    mMuxerConfig[out_index].stream_info[stream_index].spec.video.M = DefaultH264M;
    mMuxerConfig[out_index].stream_info[stream_index].spec.video.N = DefaultH264N;
    mMuxerConfig[out_index].stream_info[stream_index].spec.video.IDRInterval = DefaultH264IDRInterval;
    mMuxerConfig[out_index].stream_info[stream_index].spec.video.entropy_type = EntropyType_NOTSet;
    //LOGM_INFO("****!!!!3333 pConfig->entropy_type %d.\n", mMuxerConfig[out_index].stream_info[stream_index].spec.video.entropy_type);
    return EECode_OK;
}

EECode CActiveGenericEngine::defaultAudioParameters(TUint out_index, TUint stream_index)
{
    //safe check
    DASSERT((EEngineState_idle == msState) || (EEngineState_not_alive == msState));
    if ((EEngineState_idle != msState) && (EEngineState_not_alive == msState)) {
        LOGM_FATAL("defaultVideoParameters in bad engine state %d.\n", msState);
        return EECode_InternalLogicalBug;
    }

    DASSERT(out_index < mTotalMuxerNumber);
    if (out_index >= mTotalMuxerNumber) {
        LOGM_ERROR("BAD parameters in CActiveGenericEngine::defaultAudioParameters, out_index %d, stream index %d.\n", out_index, stream_index);
        return EECode_BadParam;
    }

    //hard code here, should configed by audio pts's generator
    mMuxerConfig[out_index].stream_info[stream_index].spec.audio.pts_unit_num = 1;
    mMuxerConfig[out_index].stream_info[stream_index].spec.audio.pts_unit_den = TimeUnitDen_90khz;

    //if have input from external(rec.config, etc)
    if (out_index < mPersistMediaConfig.tot_muxer_number) {
        LOGM_DEBUG("&&&&& mMuxerConfig[out_index %d].stream_info[stream_index %d].spec.audio.sample_rate %d.\n", out_index, stream_index, mMuxerConfig[out_index].stream_info[stream_index].spec.audio.sample_rate);

        volatile STargetRecorderConfig *pConfig = &mPersistMediaConfig.target_recorder_config[out_index];
        mMuxerConfig[out_index].stream_info[stream_index].stream_type = StreamType_Audio;
        mMuxerConfig[out_index].stream_info[stream_index].stream_format = (StreamFormat)pConfig->audio_format;
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.channel_layout = 0;//hard code
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.channel_number = pConfig->channel_number;
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.sample_rate = pConfig->sample_rate;
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.sample_format = (AudioSampleFMT) pConfig->sample_format;
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.frame_size = AUDIO_CHUNK_SIZE;//need set here?
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.bitrate = pConfig->audio_bitrate;
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.need_skip_adts_header = 0;
        return EECode_OK;
    }

    DASSERT(0);
    LOGM_WARN("&&&&& should not comes here.\n");
    //if have no input from external(rec.config, etc), use some hard coded default value
    mMuxerConfig[out_index].stream_info[stream_index].stream_type = StreamType_Audio;
    mMuxerConfig[out_index].stream_info[stream_index].stream_format = StreamFormat_AAC;
    mMuxerConfig[out_index].stream_info[stream_index].spec.audio.channel_layout = 0;
    mMuxerConfig[out_index].stream_info[stream_index].spec.audio.channel_number = DDefaultAudioChannelNumber;
    mMuxerConfig[out_index].stream_info[stream_index].spec.audio.sample_rate = DDefaultAudioSampleRate;
    mMuxerConfig[out_index].stream_info[stream_index].spec.audio.sample_format = AudioSampleFMT_S16;
    mMuxerConfig[out_index].stream_info[stream_index].spec.audio.frame_size = AUDIO_CHUNK_SIZE;//need set here?
    mMuxerConfig[out_index].stream_info[stream_index].spec.audio.bitrate = 128000;
    mMuxerConfig[out_index].stream_info[stream_index].spec.audio.need_skip_adts_header = 0;

    return EECode_OK;
}

void CActiveGenericEngine::setupStreamingServerManger()
{
    DASSERT(!mpStreamingServerManager);
    if (mpStreamingServerManager) {
        return;
    }

    mpStreamingServerManager = gfCreateStreamingServerManager(&mPersistMediaConfig, (IMsgSink *)this, mpSystemClockReference);
    if (!mpStreamingServerManager) {
        LOGM_ERROR("Create CStreammingServerManager fail.\n");
        return;
    }

    mpStreamingServerManager->RunServerManager();
}

EECode CActiveGenericEngine::addStreamingServer(TGenericID &server_id, StreamingServerType type, StreamingServerMode mode)
{
    IStreamingServer *p_server;
    server_id = 0;

    if (!mpStreamingServerManager) {
        setupStreamingServerManger();
        if (!mpStreamingServerManager) {
            LOGM_ERROR("setupStreamingServerManger() fail\n");
            return EECode_Error;
        }
    }

    //add new server
    p_server = mpStreamingServerManager->AddServer(type, mode, mPersistMediaConfig.rtsp_server_config.rtsp_listen_port, mPersistMediaConfig.rtsp_streaming_video_enable, mPersistMediaConfig.rtsp_streaming_audio_enable);

    if (DLikely(p_server)) {
        server_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_StreamingServer, mnComponentProxysNumbers[EGenericComponentType_StreamingServer - EGenericComponentType_TAG_proxy_start]);
        p_server->SetServerID(server_id, mnComponentProxysNumbers[EGenericComponentType_StreamingServer - EGenericComponentType_TAG_proxy_start], EGenericComponentType_StreamingServer);
        mnComponentProxysNumbers[EGenericComponentType_StreamingServer - EGenericComponentType_TAG_proxy_start] ++;
        mStreamingServerList.InsertContent(NULL, (void *)p_server, 0);
        return EECode_OK;
    } else {
        LOGM_ERROR("AddServer(%d, %d) fail\n", type, mode);
    }

    return EECode_Error;
}

EECode CActiveGenericEngine::removeStreamingServer(TGenericID server_id)
{
    if (DUnlikely(!mpStreamingServerManager)) {
        LOGM_ERROR("NULL mpStreamingServerManager, but someone invoke CActiveGenericEngine::removeStreamingServer?\n");
        return EECode_Error;
    }

    IStreamingServer *p_server = findStreamingServer(server_id);
    if (DLikely(p_server)) {
        mStreamingServerList.RemoveContent((void *)p_server);
        mpStreamingServerManager->RemoveServer(p_server);
    } else {
        LOGM_ERROR("NO Server's id is 0x%08x.\n", server_id);
        return EECode_BadParam;
    }

    return EECode_OK;
}

IStreamingServer *CActiveGenericEngine::findStreamingServer(TGenericID id)
{
    //find if the server is exist
    CIDoubleLinkedList::SNode *pnode;
    IStreamingServer *p_server;
    TGenericID id1;
    TComponentIndex index;
    TComponentType type;

    pnode = mStreamingServerList.FirstNode();
    while (pnode) {
        p_server = (IStreamingServer *) pnode->p_context;
        p_server->GetServerID(id1, index, type);
        if (id1 == id) {
            LOGM_INFO("find streaming server in list(id 0x%08x).\n", id);
            return p_server;
        }
        pnode = mStreamingServerList.NextNode(pnode);
    }

    LOGM_WARN("cannot find streaming server in list(id 0x%08x).\n", id);
    return NULL;
}

void CActiveGenericEngine::setupCloudServerManger()
{
    DASSERT(!mpCloudServerManager);
    if (mpCloudServerManager) {
        return;
    }

    mpCloudServerManager = gfCreateCloudServerManager(&mPersistMediaConfig, (IMsgSink *)this, mpSystemClockReference);
    if (!mpCloudServerManager) {
        LOGM_ERROR("Create gfCreateCloudServerManager fail.\n");
        return;
    }

    mpCloudServerManager->RunServerManager();
}

EECode CActiveGenericEngine::addCloudServer(TGenericID &server_id, CloudServerType type, TU16 port)
{
    ICloudServer *p_server;
    server_id = 0;

    if (!mpCloudServerManager) {
        setupCloudServerManger();
        if (!mpCloudServerManager) {
            LOGM_ERROR("setupCloudServerManger() fail\n");
            return EECode_Error;
        }
    }

    //add new server
    p_server = mpCloudServerManager->AddServer(type, port);

    if (DLikely(p_server)) {
        server_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_CloudServer, mnComponentProxysNumbers[EGenericComponentType_CloudServer - EGenericComponentType_TAG_proxy_start]);
        p_server->SetServerID(server_id, mnComponentProxysNumbers[EGenericComponentType_CloudServer - EGenericComponentType_TAG_proxy_start], EGenericComponentType_CloudServer);
        mnComponentProxysNumbers[EGenericComponentType_CloudServer - EGenericComponentType_TAG_proxy_start] ++;
        mCloudServerList.InsertContent(NULL, (void *)p_server, 0);
        return EECode_OK;
    } else {
        LOGM_ERROR("AddServer(%d, %d) fail\n", type, port);
    }

    return EECode_Error;
}

EECode CActiveGenericEngine::removeCloudServer(TGenericID server_id)
{
    if (DUnlikely(!mpCloudServerManager)) {
        LOGM_ERROR("NULL mpCloudServerManager, but someone invoke CActiveGenericEngine::removeCloudServer?\n");
        return EECode_Error;
    }

    ICloudServer *p_server = findCloudServer(server_id);
    if (DLikely(p_server)) {
        mCloudServerList.RemoveContent((void *)p_server);
        mpCloudServerManager->RemoveServer(p_server);
    } else {
        LOGM_ERROR("NO Server's id is 0x%08x.\n", server_id);
        return EECode_BadParam;
    }

    return EECode_OK;
}

ICloudServer *CActiveGenericEngine::findCloudServer(TGenericID id)
{
    //find if the server is exist
    CIDoubleLinkedList::SNode *pnode;
    ICloudServer *p_server;
    TGenericID id1;
    TComponentIndex index;
    TComponentType type;

    pnode = mCloudServerList.FirstNode();
    while (pnode) {
        p_server = (ICloudServer *) pnode->p_context;
        p_server->GetServerID(id1, index, type);
        if (id1 == id) {
            LOGM_INFO("find cloud server in list(id 0x%08x).\n", id);
            return p_server;
        }
        pnode = mCloudServerList.NextNode(pnode);
    }

    LOGM_WARN("cannot find cloud server in list(id 0x%08x).\n", id);
    return NULL;
}

void CActiveGenericEngine::setupDefaultVideoSubsession(SStreamingSubSessionContent *p_content, StreamFormat format, TComponentIndex input_index)
{
    DASSERT(p_content);
    p_content->transmiter_input_pin_index = input_index;
    p_content->type = StreamType_Video;
    p_content->format = format;

    p_content->enabled = 1;
    p_content->data_comes = 0;
    p_content->parameters_setup = 0;

    p_content->video_bit_rate = 1 * 1024 * 1024;
    p_content->video_framerate = 30;
    p_content->video_width = 720;
    p_content->video_height = 480;
}

void CActiveGenericEngine::setupDefaultAudioSubsession(SStreamingSubSessionContent *p_content, StreamFormat format, TComponentIndex input_index)
{
    DASSERT(p_content);
    p_content->transmiter_input_pin_index = input_index;
    p_content->type = StreamType_Audio;
    p_content->format = format;

    p_content->enabled = 1;
    p_content->data_comes = 0;
    p_content->parameters_setup = 0;

    p_content->audio_channel_number = DDefaultAudioChannelNumber;
    p_content->audio_sample_rate = DDefaultAudioSampleRate;
    p_content->audio_bit_rate = 128000;
    p_content->audio_sample_format = AudioSampleFMT_S16;
}

EECode CActiveGenericEngine::newStreamingContent(TGenericID &content_id)
{
    SStreamingSessionContent *p_content;

    if (DUnlikely(!mpStreamingContent)) {
        LOGM_FATAL("NULL mpStreamingContent\n");
        return EECode_InternalLogicalBug;
    }


    if (DUnlikely(mnComponentProxysNumbers[EGenericComponentType_StreamingContent - EGenericComponentType_TAG_proxy_start] >= mnComponentProxysMaxNumbers[EGenericComponentType_StreamingContent - EGenericComponentType_TAG_proxy_start])) {
        LOGM_FATAL("too many content number %d, max number %d\n", mnComponentProxysNumbers[EGenericComponentType_StreamingContent - EGenericComponentType_TAG_proxy_start], mnComponentProxysMaxNumbers[EGenericComponentType_StreamingContent - EGenericComponentType_TAG_proxy_start]);
        return EECode_BadParam;
    }

    p_content = mpStreamingContent + mnComponentProxysNumbers[EGenericComponentType_StreamingContent - EGenericComponentType_TAG_proxy_start];

    p_content->id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_StreamingContent, mnComponentProxysNumbers[EGenericComponentType_StreamingContent - EGenericComponentType_TAG_proxy_start]);
    p_content->content_index = mnComponentProxysNumbers[EGenericComponentType_StreamingContent - EGenericComponentType_TAG_proxy_start];
    p_content->b_content_setup = 0;

    mnComponentProxysNumbers[EGenericComponentType_StreamingContent - EGenericComponentType_TAG_proxy_start] ++;

    content_id = p_content->id;

    return EECode_OK;
}

EECode CActiveGenericEngine::setupStreamingContent(TGenericID content_id, TGenericID server_id, TGenericID transmiter_id, TChar *url, TU8 has_video, TU8 has_audio, TComponentIndex track_video_input_index, TComponentIndex track_audio_input_index)
{
    SStreamingSessionContent *p_content;
    IStreamingServer *p_server;
    SComponent_Obsolete *p_component;
    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(content_id);
    TComponentType type = DCOMPONENT_TYPE_FROM_GENERIC_ID(content_id);

    if (DUnlikely(!mpStreamingContent)) {
        LOGM_FATAL("NULL mpStreamingContent\n");
        return EECode_InternalLogicalBug;
    }

    //parameters check
    if (DUnlikely(!IS_VALID_COMPONENT_ID(content_id))) {
        LOGM_FATAL("BAD content_id 0x%08x.\n", content_id);
        return EECode_BadParam;
    }

    if (DUnlikely(EGenericComponentType_StreamingContent != type)) {
        LOGM_FATAL("BAD content_id 0x%08x.\n", content_id);
        return EECode_BadParam;
    }

    if (DUnlikely(index >= mnComponentProxysNumbers[EGenericComponentType_StreamingContent - EGenericComponentType_TAG_proxy_start])) {
        LOGM_FATAL("content index %d exceed max value %d, bad content_id 0x%08x\n", index, mnComponentProxysNumbers[EGenericComponentType_StreamingContent - EGenericComponentType_TAG_proxy_start], content_id);
        return EECode_BadParam;
    }

    p_server = findStreamingServer(server_id);
    if (DUnlikely(!p_server)) {
        LOGM_FATAL("Bad server_id 0x%08x.\n", server_id);
        return EECode_BadParam;
    }

    p_component = findComponentFromID(transmiter_id);
    if (DUnlikely(!p_component)) {
        LOGM_FATAL("NULL transmiter component, or BAD transmiter id 0x%08x?\n", transmiter_id);
        return EECode_BadParam;
    }

    p_content = mpStreamingContent + index;

    p_content->id = content_id;//DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_StreamingContent, mnCurrentStreamingContentNumber);
    p_content->content_index = index;//mnCurrentStreamingContentNumber;

    p_content->p_server = p_server;

    //snprintf(&p_content->session_name[0], DMaxStreamContentUrlLength - 1, "_%04d_%s", index, url);
    snprintf(&p_content->session_name[0], DMaxStreamContentUrlLength - 1, "%s", url);
    p_content->session_name[DMaxStreamContentUrlLength - 1] = 0x0;

    p_content->enabled = 1;
    p_content->sub_session_count = 0;

    if (DLikely(mpUniqueRTSPStreamingTransmiter)) {
        EECode reterr = EECode_OK;
        SQueryInterface query;

        if (has_video) {
            query.index = track_video_input_index;
            query.p_pointer = 0;
            query.type = StreamType_Video;
            query.format = StreamFormat_Invalid;
            query.p_agent_pointer = 0;

            reterr = mpUniqueRTSPStreamingTransmiter->FilterProperty(EFilterPropertyType_streaming_query_subsession_sink, 0, (void *) &query);
            DASSERT_OK(reterr);
            DASSERT(query.p_pointer);
            DASSERT(query.p_agent_pointer);

            p_content->sub_session_content[ESubSession_video] = (SStreamingSubSessionContent *)query.p_agent_pointer;
            DASSERT(query.p_pointer == ((TPointer)p_content->sub_session_content[ESubSession_video]->p_transmiter));
            p_content->sub_session_count ++;
        }

        if (has_audio) {
            query.index = track_audio_input_index;
            query.p_pointer = 0;
            query.type = StreamType_Audio;
            query.format = StreamFormat_Invalid;
            query.p_agent_pointer = 0;

            reterr = mpUniqueRTSPStreamingTransmiter->FilterProperty(EFilterPropertyType_streaming_query_subsession_sink, 0, (void *) &query);
            DASSERT_OK(reterr);
            DASSERT(query.p_pointer);
            DASSERT(query.p_agent_pointer);

            p_content->sub_session_content[ESubSession_audio] = (SStreamingSubSessionContent *)query.p_agent_pointer;
            DASSERT(query.p_pointer == ((TPointer)p_content->sub_session_content[ESubSession_audio]->p_transmiter));
            p_content->sub_session_count ++;
        }
    } else {
        LOGM_FATAL("NULL mpUniqueRTSPStreamingTransmiter\n");
    }

    p_content->has_video = has_video;
    if (has_video) {
        p_content->sub_session_content[ESubSession_video]->enabled = 1;
        //    setupDefaultVideoSubsession(&p_content->sub_session_content[ESubSession_video], StreamFormat_H264, track_video_input_index);
    }

    p_content->has_audio = has_audio;
    if (has_audio) {
        p_content->sub_session_content[ESubSession_audio]->enabled = 1;
        //    setupDefaultAudioSubsession(&p_content->sub_session_content[ESubSession_audio], StreamFormat_AAC, track_audio_input_index);
    }

    if (DLikely(mpStreamingServerManager)) {
        LOGM_INFO("add streaming content %s, has video %d, has audio %d\n", p_content->session_name, p_content->has_video, p_content->has_audio);
        p_content->p_streaming_transmiter_filter = mpUniqueRTSPStreamingTransmiter;
        mpStreamingServerManager->AddStreamingContent(p_server, p_content);
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::updateStreamingContent(TGenericID content_id, TChar *name, TUint enable)
{
    SStreamingSessionContent *p_content;
    TComponentIndex index = 0;
    TComponentType type = 0;

    if (DUnlikely(!mpStreamingContent)) {
        LOGM_FATAL("NULL mpStreamingContent\n");
        return EECode_InternalLogicalBug;
    }

    //parameters check
    if (DUnlikely(!IS_VALID_COMPONENT_ID(content_id))) {
        LOGM_FATAL("BAD content_id 0x%08x.\n", content_id);
        return EECode_BadParam;
    }

    type = DCOMPONENT_TYPE_FROM_GENERIC_ID(content_id);
    if (DUnlikely(EGenericComponentType_StreamingContent != type)) {
        LOGM_FATAL("BAD content_id 0x%08x.\n", content_id);
        return EECode_BadParam;
    }

    index = DCOMPONENT_INDEX_FROM_GENERIC_ID(content_id);
    if (DUnlikely(index >= mnComponentProxysMaxNumbers[EGenericComponentType_StreamingContent - EGenericComponentType_TAG_proxy_start])) {
        LOGM_FATAL("content index %d exceed max %d, bad content_id 0x%08x\n", index, mnComponentProxysMaxNumbers[EGenericComponentType_StreamingContent - EGenericComponentType_TAG_proxy_start], content_id);
        return EECode_BadParam;
    }

    p_content = mpStreamingContent + index;

    if (name) {
        LOGM_INFO("update name %s, content_id 0x%08x\n", name, content_id);
        snprintf(&p_content->session_name[0], DMaxStreamContentUrlLength - 1, "_%04d_%s", index, name);
        p_content->session_name[DMaxStreamContentUrlLength - 1] = 0x0;
        return EECode_BadParam;
    }

    p_content->enabled = enable;

    return EECode_OK;
}

EECode CActiveGenericEngine::newCloudReceiverContent(TGenericID &content_id)
{
    SCloudContent *p_content;

    if (DUnlikely(!mpCloudReceiverContent)) {
        LOGM_FATAL("NULL mpCloudReceiverContent\n");
        return EECode_InternalLogicalBug;
    }


    if (DUnlikely(mnComponentProxysNumbers[EGenericComponentType_CloudReceiverContent - EGenericComponentType_TAG_proxy_start] >= mnComponentProxysMaxNumbers[EGenericComponentType_CloudReceiverContent - EGenericComponentType_TAG_proxy_start])) {
        LOGM_FATAL("too many content number %d, max number %d\n", mnComponentProxysNumbers[EGenericComponentType_CloudReceiverContent - EGenericComponentType_TAG_proxy_start], mnComponentProxysMaxNumbers[EGenericComponentType_CloudReceiverContent - EGenericComponentType_TAG_proxy_start]);
        return EECode_BadParam;
    }

    p_content = mpCloudReceiverContent + mnComponentProxysNumbers[EGenericComponentType_CloudReceiverContent - EGenericComponentType_TAG_proxy_start];

    p_content->id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_CloudReceiverContent, mnComponentProxysNumbers[EGenericComponentType_CloudReceiverContent - EGenericComponentType_TAG_proxy_start]);
    p_content->content_index = mnComponentProxysNumbers[EGenericComponentType_CloudReceiverContent - EGenericComponentType_TAG_proxy_start];
    p_content->b_content_setup = 0;

    mnComponentProxysNumbers[EGenericComponentType_CloudReceiverContent - EGenericComponentType_TAG_proxy_start] ++;

    content_id = p_content->id;

    return EECode_OK;
}

EECode CActiveGenericEngine::setupCloudReceiverContent(TGenericID content_id, TGenericID server_id, TGenericID receiver_id, TChar *url)
{
    SCloudContent *p_content;
    ICloudServer *p_server;
    SComponent_Obsolete *p_component;
    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(content_id);
    TComponentType type = DCOMPONENT_TYPE_FROM_GENERIC_ID(content_id);

    if (DUnlikely(!mpCloudReceiverContent)) {
        LOGM_FATAL("NULL mpCloudReceiverContent\n");
        return EECode_InternalLogicalBug;
    }

    //parameters check
    if (DUnlikely(!IS_VALID_COMPONENT_ID(content_id))) {
        LOGM_FATAL("BAD content_id 0x%08x.\n", content_id);
        return EECode_BadParam;
    }

    if (DUnlikely(EGenericComponentType_CloudReceiverContent != type)) {
        LOGM_FATAL("BAD content_id 0x%08x.\n", content_id);
        return EECode_BadParam;
    }

    if (DUnlikely(index >= mnComponentProxysNumbers[EGenericComponentType_CloudReceiverContent - EGenericComponentType_TAG_proxy_start])) {
        LOGM_FATAL("content index %d exceed max value %d, bad content_id 0x%08x\n", index, mnComponentProxysNumbers[EGenericComponentType_CloudReceiverContent - EGenericComponentType_TAG_proxy_start], content_id);
        return EECode_BadParam;
    }

    p_server = findCloudServer(server_id);
    if (DUnlikely(!p_server)) {
        LOGM_FATAL("Bad server_id 0x%08x.\n", server_id);
        return EECode_BadParam;
    }

    p_component = findComponentFromID(receiver_id);
    if (DUnlikely(!p_component)) {
        LOGM_FATAL("NULL receiver component, or BAD receiver_id id 0x%08x?\n", receiver_id);
        return EECode_BadParam;
    } else if (DUnlikely(!p_component->p_filter)) {
        LOGM_FATAL("NULL receiver component->p_filter, or BAD receiver_id id 0x%08x?\n", receiver_id);
        return EECode_BadParam;
    }

    p_content = mpCloudReceiverContent + index;

    p_content->id = content_id;//DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_CloudReceiverContent, mnCurrentCloudReceiverContentNumber);
    p_content->content_index = index;//mnCurrentCloudReceiverContentNumber;

    //DASSERT(p_component->p_filter);
    p_content->p_server = p_server;
    p_content->p_cloud_agent_filter = p_component->p_filter;

    DASSERT(url);
    snprintf(&p_content->content_name[0], DMaxCloudAgentUrlLength - 1, "%s", url);
    p_content->content_name[DMaxCloudAgentUrlLength - 1] = 0x0;

    p_content->enabled = 1;
    p_content->sub_session_count = 0;

    p_content->has_video = 0;
    p_content->has_audio = 0;

    if (DLikely(mpCloudServerManager)) {
        LOGM_INFO("add cloud receiver content %s\n", p_content->content_name);
        mpCloudServerManager->AddCloudContent(p_server, p_content);
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::presetPlaybackModeForDSP(TU16 num_of_instances_0, TU16 num_of_instances_1, TUint max_width_0, TUint max_height_0, TUint max_width_1, TUint max_height_1)
{
    TU16 i;

    if ((num_of_instances_0 + num_of_instances_1) > DMaxUDECInstanceNumber) {
        LOGM_FATAL("BAD request num_of_instances_0 %d, num_of_instances_1 %d, exceed max %d\n", num_of_instances_0, num_of_instances_1, DMaxUDECInstanceNumber);
        return EECode_BadParam;
    }

    mTotalVideoDecoderNumber = num_of_instances_0 + num_of_instances_1;
    mMultipleWindowDecoderNumber = num_of_instances_0;
    mSingleWindowDecoderNumber = num_of_instances_1;

    for (i = 0; i < num_of_instances_0; i ++) {
        mVideoDecoder[i].tiled_mode = mPersistMediaConfig.dsp_config.tiled_mode;
        mVideoDecoder[i].frm_chroma_fmt_max = mPersistMediaConfig.dsp_config.frm_chroma_fmt_max;
        mVideoDecoder[i].max_frm_num = mPersistMediaConfig.dsp_config.max_frm_num;
        //mVideoDecoder[i].max_frm_width = mPersistMediaConfig.dsp_config.max_frm_width;
        //mVideoDecoder[i].max_frm_height = mPersistMediaConfig.dsp_config.max_frm_height;
        mVideoDecoder[i].max_fifo_size = mPersistMediaConfig.dsp_config.max_fifo_size;
        mVideoDecoder[i].enable_pp = mPersistMediaConfig.dsp_config.enable_pp;
        mVideoDecoder[i].enable_deint = mPersistMediaConfig.dsp_config.enable_deint;
        mVideoDecoder[i].interlaced_out = mPersistMediaConfig.dsp_config.interlaced_out;
        mVideoDecoder[i].packed_out = mPersistMediaConfig.dsp_config.packed_out;
        mVideoDecoder[i].enable_err_handle = mPersistMediaConfig.dsp_config.enable_err_handle;

        mVideoDecoder[i].bits_fifo_size = mPersistMediaConfig.dsp_config.bits_fifo_size;
        mVideoDecoder[i].ref_cache_size = mPersistMediaConfig.dsp_config.ref_cache_size;
        mVideoDecoder[i].concealment_mode = mPersistMediaConfig.dsp_config.concealment_mode;
        mVideoDecoder[i].concealment_ref_frm_buf_id = mPersistMediaConfig.dsp_config.concealment_ref_frm_buf_id;

        mVideoDecoder[i].max_frm_width = max_width_0;
        mVideoDecoder[i].max_frm_height = max_height_0;
        LOGM_INFO("[%d] decoder, max_frm_width %d, max_frm_height %d\n", i, mVideoDecoder[i].max_frm_width, mVideoDecoder[i].max_frm_height);
    }

    mPersistMediaConfig.tmp_number_of_d1 = num_of_instances_0;

    for (i = num_of_instances_0; i < mTotalVideoDecoderNumber; i ++) {
        mVideoDecoder[i].tiled_mode = mPersistMediaConfig.dsp_config.tiled_mode;
        mVideoDecoder[i].frm_chroma_fmt_max = mPersistMediaConfig.dsp_config.frm_chroma_fmt_max;
        mVideoDecoder[i].max_frm_num = mPersistMediaConfig.dsp_config.max_frm_num;
        //mVideoDecoder[i].max_frm_width = mPersistMediaConfig.dsp_config.max_frm_width;
        //mVideoDecoder[i].max_frm_height = mPersistMediaConfig.dsp_config.max_frm_height;
        mVideoDecoder[i].max_fifo_size = mPersistMediaConfig.dsp_config.max_fifo_size;
        mVideoDecoder[i].enable_pp = mPersistMediaConfig.dsp_config.enable_pp;
        mVideoDecoder[i].enable_deint = mPersistMediaConfig.dsp_config.enable_deint;
        mVideoDecoder[i].interlaced_out = mPersistMediaConfig.dsp_config.interlaced_out;
        mVideoDecoder[i].packed_out = mPersistMediaConfig.dsp_config.packed_out;
        mVideoDecoder[i].enable_err_handle = mPersistMediaConfig.dsp_config.enable_err_handle;

        mVideoDecoder[i].bits_fifo_size = mPersistMediaConfig.dsp_config.bits_fifo_size;
        mVideoDecoder[i].ref_cache_size = mPersistMediaConfig.dsp_config.ref_cache_size;
        mVideoDecoder[i].concealment_mode = mPersistMediaConfig.dsp_config.concealment_mode;
        mVideoDecoder[i].concealment_ref_frm_buf_id = mPersistMediaConfig.dsp_config.concealment_ref_frm_buf_id;

        mVideoDecoder[i].max_frm_width = max_width_1;
        mVideoDecoder[i].max_frm_height = max_height_1;
        LOGM_INFO("[%d] decoder, max_frm_width %d, max_frm_height %d\n", i, mVideoDecoder[i].max_frm_width, mVideoDecoder[i].max_frm_height);
    }

    mPersistMediaConfig.dsp_config.modeConfig.num_udecs = mTotalVideoDecoderNumber;

    return EECode_OK;
}

TTime CActiveGenericEngine::UserGetCurrentTime() const
{
    if (DLikely(mpSystemClockReference)) {
        return mpSystemClockReference->GetCurrentTime();
    }

    return 0;
}

TGenericID CActiveGenericEngine::AddUserTimer(TFTimerCallback cb, void *thiz, void *param, EClockTimerType type, TTime time, TTime interval, TUint count)
{
    AUTO_LOCK(mpMutex);

    SUserTimerContext *context = allocUserTimerContext();
    if (DLikely(context)) {
        context->cb = cb;
        context->user_context = thiz;
        context->user_parameters = param;
        context->mpClockListener = mpSystemClockReference->AddTimer(this, type, time, interval, count);
        context->mpClockListener->user_context = (TULong)context;
        return context->id;
    }

    LOGM_FATAL("alloc user timer fail\n");
    return 0;
}

EECode CActiveGenericEngine::RemoveUserTimer(TGenericID user_timer_id)
{
    AUTO_LOCK(mpMutex);

    if (DUnlikely(!IS_VALID_COMPONENT_ID(user_timer_id))) {
        LOGM_ERROR("not valid user timer id 0x%08x\n", user_timer_id);
        return EECode_BadParam;
    }

    if (DUnlikely(EGenericComponentType_UserTimer != DCOMPONENT_TYPE_FROM_GENERIC_ID(user_timer_id))) {
        LOGM_ERROR("bad user timer id type, 0x%08x\n", user_timer_id);
        return EECode_BadParam;
    }

    if (DUnlikely(mnUserTimerNumber <= DCOMPONENT_INDEX_FROM_GENERIC_ID(user_timer_id))) {
        LOGM_ERROR("bad user timer id index, 0x%08x\n", user_timer_id);
        return EECode_BadParam;
    }

    CIDoubleLinkedList::SNode *pnode = mUserTimerList.FirstNode();
    SUserTimerContext *context = NULL;
    while (pnode) {
        context = (SUserTimerContext *)pnode->p_context;
        if (context->id == user_timer_id) {
            mUserTimerList.RemoveContent((void *) context);
            mUserTimerFreeList.InsertContent(NULL, (void *) context, 1);
            return EECode_OK;
        }
        pnode = mUserTimerList.NextNode(pnode);
    }

    LOGM_ERROR("not found user timer 0x%08x\n", user_timer_id);
    return EECode_BadParam;
}

SUserTimerContext *CActiveGenericEngine::allocUserTimerContext()
{
    SUserTimerContext *context = NULL;
    CIDoubleLinkedList::SNode *pnode = NULL;
    if (mUserTimerFreeList.NumberOfNodes()) {
        pnode = mUserTimerFreeList.LastNode();
        context = (SUserTimerContext *)pnode->p_context;
        mUserTimerFreeList.RemoveContent((void *)context);
        mUserTimerList.InsertContent(NULL, context, 1);
        return context;
    } else {
        if (DLikely(mnUserTimerNumber > mnUserTimerMaxNumber)) {
            LOGM_WARN("too many user timers %d, max %d\n", mnUserTimerNumber, mnUserTimerMaxNumber);
        }

        context = (SUserTimerContext *)DDBG_MALLOC(sizeof(SUserTimerContext), "E0UT");
        if (DLikely(context)) {
            context->id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_UserTimer, mnUserTimerNumber);
            context->index = mnUserTimerNumber;
            context->mpClockListener = NULL;
            context->user_context = NULL;
            context->user_parameters = NULL;
            mnUserTimerNumber ++;
            mUserTimerList.InsertContent(NULL, context, 1);
            return context;
        } else {
            LOGM_FATAL("No memory\n");
        }
    }

    return NULL;
}

void CActiveGenericEngine::releaseUserTimerContext(SUserTimerContext *context)
{
    if (DLikely(mUserTimerList.NumberOfNodes())) {
        mUserTimerList.RemoveContent((void *)context);
        mUserTimerFreeList.InsertContent(NULL, (void *)context, 1);
    } else {
        LOGM_FATAL("internal logic bug\n");
    }
}

void CActiveGenericEngine::stopServers()
{
    if (mpStreamingServerManager) {
        mpStreamingServerManager->Stop();
        mpStreamingServerManager->ExitServerManager();
        mpStreamingServerManager->Destroy();
        mpStreamingServerManager = NULL;
    }

    if (mpCloudServerManager) {
        mpStreamingServerManager->Stop();
        mpCloudServerManager->ExitServerManager();
        mpCloudServerManager->Destroy();
        mpCloudServerManager = NULL;
    }
}

void CActiveGenericEngine::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
    //AUTO_LOCK(mpMutex);
    if (DLikely(param2)) {
        if (DLikely(EEventType_Timer == type)) {
            SUserTimerContext *context = (SUserTimerContext *) param2;
            if (((TULong)&mDailyCheckContext) == ((TULong)context)) {
                mpDailyCheckTimer = NULL;
                SMSG msg;
                msg.pExtra = NULL;
                msg.code = ECMDType_MiscCheckLicence;
                PostEngineMsg(msg);
                mDailyCheckTime += (TTime)24 * 60 * 60 * 1000000;
                mpDailyCheckTimer = mpSystemClockReference->AddTimer(this, EClockTimerType_once, mDailyCheckTime, (TTime)24 * 60 * 60 * 1000000, 1);
            } else if (context) {
                context->cb(context->user_context, context->user_parameters, (TTime)param1);
            }
        }
    }
}

void CActiveGenericEngine::Destroy()
{
    Delete();
}

void CActiveGenericEngine::initializeSyncStatus()
{
    TU32 index = 0;

    mCurrentSyncStatus.version.native_major = DCloudLibVesionMajor;
    mCurrentSyncStatus.version.native_minor = DCloudLibVesionMinor;
    mCurrentSyncStatus.version.native_patch = DCloudLibVesionPatch;

    mCurrentSyncStatus.version.native_date_year = DCloudLibVesionYear;
    mCurrentSyncStatus.version.native_date_month = DCloudLibVesionMonth;
    mCurrentSyncStatus.version.native_date_day = DCloudLibVesionDay;
    LOGM_NOTICE("mInitalVideoPostPDisplayLayout.layer_number %d\n", mInitalVideoPostPDisplayLayout.layer_number);
    if (1 == mInitalVideoPostPDisplayLayout.layer_number) {
        mCurrentSyncStatus.channel_number_per_group = mInitalVideoPostPDisplayLayout.total_decoder_number;
    } else if (2 == mInitalVideoPostPDisplayLayout.layer_number) {
        mCurrentSyncStatus.channel_number_per_group = mInitalVideoPostPDisplayLayout.total_decoder_number - 1;
    } else {
        LOGM_ERROR("BAD mInitalVideoPostPDisplayLayout.layer_number %d\n", mInitalVideoPostPDisplayLayout.layer_number);
    }

    LOGM_NOTICE("mCurrentSyncStatus.channel_number_per_group %d\n", mCurrentSyncStatus.channel_number_per_group);

    mCurrentSyncStatus.total_group_number = 1;
    mCurrentSyncStatus.current_group_index = 0;
    mCurrentSyncStatus.have_dual_stream = 1;

    mCurrentSyncStatus.is_vod_enabled = 0;
    mCurrentSyncStatus.is_vod_ready = 0;

    mCurrentSyncStatus.audio_source_index = 0;
    mCurrentSyncStatus.audio_target_index = 0;

    mCurrentSyncStatus.encoding_width = mInitalVideoPostPGlobalSetting.display_width;
    mCurrentSyncStatus.encoding_height = mInitalVideoPostPGlobalSetting.display_height;
    mCurrentSyncStatus.encoding_bitrate = 1024 * 1024; //hard code here
    mCurrentSyncStatus.encoding_framerate = 30;//hard code here

    mCurrentSyncStatus.display_layout = EDisplayLayout_Rectangle;
    mCurrentSyncStatus.display_hd_channel_index = 0;

    mCurrentSyncStatus.total_window_number = mInitalVideoPostPDisplayLayout.total_window_number;
    mCurrentSyncStatus.current_display_window_number = mInitalVideoPostPDisplayLayout.layer_window_number[0];

    for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
        LOGM_NOTICE("sd %d\n", index);
        mCurrentSyncStatus.window[index].window_width = mInitialVideoPostPConfig.window[index].target_win_width;
        mCurrentSyncStatus.window[index].window_height = mInitialVideoPostPConfig.window[index].target_win_height;
        mCurrentSyncStatus.window[index].window_pos_x = mInitialVideoPostPConfig.window[index].target_win_offset_x;
        mCurrentSyncStatus.window[index].window_pos_y = mInitialVideoPostPConfig.window[index].target_win_offset_y;
        mCurrentSyncStatus.window[index].udec_index = index;
        mCurrentSyncStatus.window[index].render_index = index;
        mCurrentSyncStatus.window[index].display_disable = 0;

        mCurrentSyncStatus.zoom[index].zoom_size_x = 720;
        mCurrentSyncStatus.zoom[index].zoom_size_y = 480;
        mCurrentSyncStatus.zoom[index].zoom_input_center_x = 720 / 2;
        mCurrentSyncStatus.zoom[index].zoom_input_center_y = 480 / 2;
        mCurrentSyncStatus.zoom[index].udec_index = index;

        mCurrentSyncStatus.source_info[index].pic_width = 720;
        mCurrentSyncStatus.source_info[index].pic_height = 480;
        mCurrentSyncStatus.source_info[index].bitrate = 1 * 1024 * 1024;
        mCurrentSyncStatus.source_info[index].framerate = 30;

        mCurrentSyncStatus.source_info[index + mCurrentSyncStatus.channel_number_per_group].pic_width = 1920;
        mCurrentSyncStatus.source_info[index + mCurrentSyncStatus.channel_number_per_group].pic_height = 1080;
        mCurrentSyncStatus.source_info[index + mCurrentSyncStatus.channel_number_per_group].bitrate = 4 * 1024 * 1024;
        mCurrentSyncStatus.source_info[index + mCurrentSyncStatus.channel_number_per_group].framerate = 30;
    }

    for (; index < mCurrentSyncStatus.total_window_number; index ++) {
        LOGM_NOTICE("hd %d\n", index);
        mCurrentSyncStatus.window[index].window_width = mInitialVideoPostPConfig.window[index].target_win_width;
        mCurrentSyncStatus.window[index].window_height = mInitialVideoPostPConfig.window[index].target_win_height;
        mCurrentSyncStatus.window[index].window_pos_x = mInitialVideoPostPConfig.window[index].target_win_offset_x;
        mCurrentSyncStatus.window[index].window_pos_y = mInitialVideoPostPConfig.window[index].target_win_offset_y;
        mCurrentSyncStatus.window[index].udec_index = index;
        mCurrentSyncStatus.window[index].render_index = index;
        mCurrentSyncStatus.window[index].display_disable = 1;

        mCurrentSyncStatus.zoom[index].zoom_size_x = 1920;
        mCurrentSyncStatus.zoom[index].zoom_size_y = 1080;
        mCurrentSyncStatus.zoom[index].zoom_input_center_x = 1920 / 2;
        mCurrentSyncStatus.zoom[index].zoom_input_center_y = 1080 / 2;
        mCurrentSyncStatus.zoom[index].udec_index = index;
    }

    resetZoomParameters();

    mCurrentVideoPostPConfig = mInitialVideoPostPConfig;
    mCurrentVideoPostPGlobalSetting = mInitalVideoPostPGlobalSetting;
    mCurrentDisplayLayout = mInitalVideoPostPDisplayLayout;
}

void CActiveGenericEngine::initializeUrlGroup()
{
    LOGM_WARN("initializeUrlGroup. TODO\n");
}

EECode CActiveGenericEngine::receiveSyncStatus(ICloudServerAgent *p_agent, TMemSize payload_size)
{
    TU8 *p_payload = mpCommunicationBuffer;
    TMemSize read_length = 0;

    if (DUnlikely((!p_agent) || (!payload_size))) {
        LOGM_FATAL("NULL p_agent %p or zero size %ld\n", p_agent, payload_size);
        return EECode_InternalLogicalBug;
    }

    if (DUnlikely(payload_size > mCommunicationBufferSize)) {
        LOGM_WARN("buffer size not enough, request %ld, re-alloc\n", payload_size);
        if (mpCommunicationBuffer) {
            DDBG_FREE(mpCommunicationBuffer, "E0CM");
            mpCommunicationBuffer = NULL;
        }

        mCommunicationBufferSize = payload_size + 128;
        mpCommunicationBuffer = (TU8 *)DDBG_MALLOC(mCommunicationBufferSize, "E0CM");
        if (DUnlikely(!mpCommunicationBuffer)) {
            LOGM_FATAL("no memory, request size %ld\n", mCommunicationBufferSize);
            return EECode_NoMemory;
        }
    }

    EECode err = p_agent->ReadData(mpCommunicationBuffer, payload_size);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("p_agent->ReadData(, %ld) error, ret %s\n", payload_size, gfGetErrorCodeString(err));
        return err;
    }

    TU32 param0, param1, param2, param3, param4, param5, param6, param7;
    ESACPElementType element_type;
    TU32 window_index = 0;
    TU32 zoom_index = 0;

    while (payload_size) {

        read_length = gfReadSACPElement(p_payload, payload_size, element_type, param0, param1, param2, param3, param4, param5, param6, param7);
        if (payload_size < read_length) {
            LOG_ERROR("internal logic bug\n");
            return EECode_Error;
        }

        switch (element_type) {

            case ESACPElementType_GlobalSetting: {
                    mInputSyncStatus.channel_number_per_group = param0;
                    mInputSyncStatus.total_group_number = param1;
                    mInputSyncStatus.current_group_index = param2;
                    mInputSyncStatus.have_dual_stream = param3;
                    mInputSyncStatus.is_vod_enabled = param4;
                    mInputSyncStatus.is_vod_ready = param5;
                    mInputSyncStatus.total_window_number = param6;
                    mInputSyncStatus.current_display_window_number = param7;
                }
                break;

            case ESACPElementType_SyncFlags: {
                    mInputSyncStatus.update_group_info_flag = param0;
                    mInputSyncStatus.update_display_layout_flag = param1;
                    mInputSyncStatus.update_source_flag = param2;
                    mInputSyncStatus.update_display_flag = param3;
                    mInputSyncStatus.update_audio_flag = param4;
                    mInputSyncStatus.update_zoom_flag = param5;
                }
                break;

            case ESACPElementType_EncodingParameters:
                mInputSyncStatus.encoding_width = param0;
                mInputSyncStatus.encoding_height = param1;
                mInputSyncStatus.encoding_bitrate = param2;
                mInputSyncStatus.encoding_framerate = param3;
                break;

            case ESACPElementType_DisplaylayoutInfo:
                mInputSyncStatus.display_layout = param0;
                mInputSyncStatus.display_hd_channel_index = param1;
                mInputSyncStatus.audio_source_index = param2;
                mInputSyncStatus.audio_target_index = param3;
                break;

            case ESACPElementType_DisplayWindowInfo:
                if (DLikely(window_index < DQUERY_MAX_DISPLAY_WINDOW_NUMBER)) {
                    mInputSyncStatus.window[window_index].window_pos_x = (TU32)((((TU64)param0) * mInputSyncStatus.encoding_width) / DSACP_FIX_POINT_DEN);
                    mInputSyncStatus.window[window_index].window_pos_y = (TU32)((((TU64)param1) * mInputSyncStatus.encoding_height) / DSACP_FIX_POINT_DEN);
                    mInputSyncStatus.window[window_index].window_width = (TU32)((((TU64)param2) * mInputSyncStatus.encoding_width) / DSACP_FIX_POINT_DEN);
                    mInputSyncStatus.window[window_index].window_height = (TU32)((((TU64)param3) * mInputSyncStatus.encoding_height) / DSACP_FIX_POINT_DEN);
                    mInputSyncStatus.window[window_index].udec_index = param4;
                    mInputSyncStatus.window[window_index].render_index = param5;
                    mInputSyncStatus.window[window_index].display_disable = param6;
                    window_index ++;
                } else {
                    LOG_ERROR("too many windows\n");
                    //return EECode_Error;
                }
                break;

            case ESACPElementType_DisplayZoomInfo:
                if (DLikely(zoom_index < DQUERY_MAX_DISPLAY_WINDOW_NUMBER)) {
                    mInputSyncStatus.zoom[zoom_index].zoom_size_x = (TU32)((((TU64)param0) * mInputSyncStatus.source_info[zoom_index].pic_width) / DSACP_FIX_POINT_DEN);
                    mInputSyncStatus.zoom[zoom_index].zoom_size_y = (TU32)((((TU64)param1) * mInputSyncStatus.source_info[zoom_index].pic_height) / DSACP_FIX_POINT_DEN);
                    mInputSyncStatus.zoom[zoom_index].zoom_input_center_x = (TU32)((((TU64)param2) * mInputSyncStatus.source_info[zoom_index].pic_width) / DSACP_FIX_POINT_DEN);
                    mInputSyncStatus.zoom[zoom_index].zoom_input_center_y = (TU32)((((TU64)param0) * mInputSyncStatus.source_info[zoom_index].pic_height) / DSACP_FIX_POINT_DEN);
                    mInputSyncStatus.zoom[zoom_index].udec_index = param4;
                    mInputSyncStatus.zoom[zoom_index].is_valid = 1;
                    zoom_index ++;
                } else {
                    LOG_ERROR("too many windows\n");
                    //return EECode_Error;
                }
                break;

            default:
                LOG_FATAL("BAD ESACPElementType %d\n", element_type);
                break;
        }

        payload_size -= read_length;
        p_payload += read_length;
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::sendSyncStatus(ICloudServerAgent *p_agent)
{
    DASSERT(mpCommunicationBuffer);
    DASSERT(mCommunicationBufferSize);

    TU8 *p_payload = mpCommunicationBuffer + sizeof(SSACPHeader);
    SSACPHeader *p_header = (SSACPHeader *) mpCommunicationBuffer;
    TMemSize write_length = 0;
    TMemSize total_payload_size = 0;
    TMemSize remain_payload_size = mCommunicationBufferSize - sizeof(SSACPHeader);

    memset(p_header, 0x0, sizeof(SSACPHeader));

    TU32 param0, param1, param2, param3, param4, param5, param6, param7;
    TU32 window_index = 0;
    TU32 zoom_index = 0;
    TU32 total_num = mCurrentSyncStatus.total_window_number;

    param0 = mCurrentSyncStatus.channel_number_per_group;
    param1 = mCurrentSyncStatus.total_group_number;
    param2 = mCurrentSyncStatus.current_group_index;
    param3 = mCurrentSyncStatus.have_dual_stream;
    param4 = mCurrentSyncStatus.is_vod_enabled;
    param5 = mCurrentSyncStatus.is_vod_ready;
    param6 = mCurrentSyncStatus.total_window_number;
    param7 = mCurrentSyncStatus.current_display_window_number;

    write_length = gfWriteSACPElement(p_payload, remain_payload_size, ESACPElementType_GlobalSetting, param0, param1, param2, param3, param4, param5, param6, param7);
    total_payload_size += write_length;
    remain_payload_size -= write_length;
    p_payload += write_length;

    param0 = mCurrentSyncStatus.update_group_info_flag;
    param1 = mCurrentSyncStatus.update_display_layout_flag;
    param2 = mCurrentSyncStatus.update_source_flag;
    param3 = mCurrentSyncStatus.update_display_flag;
    param4 = mCurrentSyncStatus.update_audio_flag;
    param5 = mCurrentSyncStatus.update_zoom_flag;

    write_length = gfWriteSACPElement(p_payload, remain_payload_size, ESACPElementType_SyncFlags, param0, param1, param2, param3, param4, param5, param6, param7);
    total_payload_size += write_length;
    remain_payload_size -= write_length;
    p_payload += write_length;

    param0 = mCurrentSyncStatus.encoding_width;
    param1 = mCurrentSyncStatus.encoding_height;
    param2 = mCurrentSyncStatus.encoding_bitrate;
    param3 = mCurrentSyncStatus.encoding_framerate;
    write_length = gfWriteSACPElement(p_payload, remain_payload_size, ESACPElementType_EncodingParameters, param0, param1, param2, param3, param4, param5, param6, param7);
    total_payload_size += write_length;
    remain_payload_size -= write_length;
    p_payload += write_length;

    param0 = mCurrentSyncStatus.display_layout;
    param1 = mCurrentSyncStatus.display_hd_channel_index;
    param2 = mCurrentSyncStatus.audio_source_index;
    param3 = mCurrentSyncStatus.audio_target_index;
    write_length = gfWriteSACPElement(p_payload, remain_payload_size, ESACPElementType_DisplaylayoutInfo, param0, param1, param2, param3, param4, param5, param6, param7);
    total_payload_size += write_length;
    remain_payload_size -= write_length;
    p_payload += write_length;

    for (window_index = 0; window_index < total_num; window_index ++) {
        param0 = mCurrentSyncStatus.window[window_index].window_pos_x;
        param1 = mCurrentSyncStatus.window[window_index].window_pos_y;
        param2 = mCurrentSyncStatus.window[window_index].window_width;
        param3 = mCurrentSyncStatus.window[window_index].window_height;
        param4 = mCurrentSyncStatus.window[window_index].udec_index;
        param5 = mCurrentSyncStatus.window[window_index].render_index;
        param6 = mCurrentSyncStatus.window[window_index].display_disable;

        write_length = gfWriteSACPElement(p_payload, remain_payload_size, ESACPElementType_DisplayWindowInfo, param0, param1, param2, param3, param4, param5, param6, param7);
        total_payload_size += write_length;
        remain_payload_size -= write_length;
        p_payload += write_length;
    }

    for (zoom_index = 0; zoom_index < total_num; zoom_index ++) {
        param0 = mCurrentSyncStatus.zoom[zoom_index].zoom_size_x;
        param1 = mCurrentSyncStatus.zoom[zoom_index].zoom_size_y;
        param2 = mCurrentSyncStatus.zoom[zoom_index].zoom_input_center_x;
        param3 = mCurrentSyncStatus.zoom[zoom_index].zoom_input_center_y;
        param4 = mCurrentSyncStatus.zoom[zoom_index].udec_index;

        write_length = gfWriteSACPElement(p_payload, remain_payload_size, ESACPElementType_DisplayZoomInfo, param0, param1, param2, param3, param4, param5, param6, param7);
        total_payload_size += write_length;
        remain_payload_size -= write_length;
        p_payload += write_length;
    }

    p_header->type_1 |= (DSACPTypeBit_Responce >> 24);

    p_header->size_1 = (total_payload_size >> 8) & 0xff;
    p_header->size_2 = total_payload_size & 0xff;
    p_header->size_0 = (total_payload_size >> 16) & 0xff;

    total_payload_size += sizeof(SSACPHeader);
    LOG_PRINTF("sendSyncStatus, total_payload_size %ld\n", total_payload_size);

    EECode err = p_agent->WriteData(mpCommunicationBuffer, total_payload_size);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("WriteData error %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }
    LOG_PRINTF("sendSyncStatus end\n");

    return EECode_OK;
}

EECode CActiveGenericEngine::updateSyncStatus()
{
    LOGM_WARN("updateSyncStatus TODO\n");
    return EECode_NoImplement;
}

EECode CActiveGenericEngine::sendCloudAgentVersion(ICloudServerAgent *p_agent, TU32 param0, TU32 param1)
{
    if (DUnlikely(!p_agent)) {
        LOGM_ERROR("NULL p_agent\n");
        return EECode_InternalLogicalBug;
    }

    if (DUnlikely((!mpCommunicationBuffer) || (!mCommunicationBufferSize))) {
        LOGM_ERROR("NULL mpCommunicationBuffer %p or zero mCommunicationBufferSize %ld\n", mpCommunicationBuffer, mCommunicationBufferSize);
        return EECode_InternalLogicalBug;
    }

    mCurrentSyncStatus.version.peer_major = (param0 >> 16);
    mCurrentSyncStatus.version.peer_minor = (param0 >> 8) & 0xff;
    mCurrentSyncStatus.version.peer_patch = param0 & 0xff;

    mCurrentSyncStatus.version.peer_date_year = (param1 >> 16);
    mCurrentSyncStatus.version.peer_date_month = (param1 >> 8) & 0xff;
    mCurrentSyncStatus.version.peer_date_day = param1 & 0xff;

    //write back
    TU8 *p_payload = mpCommunicationBuffer + sizeof(SSACPHeader);
    SSACPHeader *p_header = (SSACPHeader *) mpCommunicationBuffer;
    TMemSize write_length = 0;

    memset(p_header, 0x0, sizeof(SSACPHeader));

    DBEW16(mCurrentSyncStatus.version.native_major, p_payload);
    p_payload += 2;
    *p_payload ++ = mCurrentSyncStatus.version.native_minor;
    *p_payload ++ = mCurrentSyncStatus.version.native_patch;
    DBEW16(mCurrentSyncStatus.version.native_date_year, p_payload);
    p_payload += 2;
    *p_payload ++ = mCurrentSyncStatus.version.native_date_month;
    *p_payload ++ = mCurrentSyncStatus.version.native_date_day;

    write_length = 8;

    p_header->type_1 |= (DSACPTypeBit_Responce >> 24);

    p_header->size_1 = (write_length >> 8) & 0xff;
    p_header->size_2 = write_length & 0xff;
    p_header->size_0 = (write_length >> 16) & 0xff;

    write_length += sizeof(SSACPHeader);
    LOG_PRINTF("sendCloudAgentVersion, write_length %ld\n", write_length);

    EECode err = p_agent->WriteData(mpCommunicationBuffer, write_length);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("WriteData error %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }
    LOG_PRINTF("sendCloudAgentVersion end\n");

    return EECode_OK;
}

EECode CActiveGenericEngine::processUpdateMudecDisplayLayout(TU32 layout, TU32 focus_index)
{
    EECode err = EECode_OK;
    TUint index = 0;
    TUint udec_index = 0;
    TUint hd_udec_index = mCurrentSyncStatus.channel_number_per_group;
    TUint update_window_params = 1;

    LOGM_NOTICE("[DEBUG] processUpdateMudecDisplayLayout, input (layout %d, focus_index %d)\n", layout, focus_index);

    if (mCurrentSyncStatus.display_layout == layout) {
        if (mCurrentSyncStatus.display_hd_channel_index == focus_index) {
            LOGM_WARN("not changed, return\n");
        }
        LOGM_PRINTF("update hd index\n");
        update_window_params = 0;
    } else {
        resetZoomParameters();
    }

    if (EDisplayLayout_TelePresence == layout) {

        if (update_window_params) {
            calculateDisplayLayoutWindow(&mCurrentVideoPostPConfig.window[0], mCurrentVideoPostPGlobalSetting.display_width, mCurrentVideoPostPGlobalSetting.display_height, (TU8)layout, (TU8)hd_udec_index, 0);
            for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
                mCurrentSyncStatus.window[index].window_width = mCurrentVideoPostPConfig.window[index].target_win_width;
                mCurrentSyncStatus.window[index].window_height = mCurrentVideoPostPConfig.window[index].target_win_height;
                mCurrentSyncStatus.window[index].window_pos_x = mCurrentVideoPostPConfig.window[index].target_win_offset_x;
                mCurrentSyncStatus.window[index].window_pos_y = mCurrentVideoPostPConfig.window[index].target_win_offset_y;
            }

            mCurrentVideoPostPConfig.total_num_win_configs = mCurrentSyncStatus.channel_number_per_group + 1;
            mCurrentVideoPostPConfig.total_window_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.total_render_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.total_decoder_number = mCurrentSyncStatus.channel_number_per_group + 1;

            mCurrentVideoPostPConfig.cur_window_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.cur_render_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.cur_decoder_number = mCurrentSyncStatus.channel_number_per_group + 1;

            mCurrentVideoPostPConfig.cur_window_start_index = 0;
            mCurrentVideoPostPConfig.cur_render_start_index = 0;
            mCurrentVideoPostPConfig.cur_decoder_start_index  = 0;
        }

        mCurrentSyncStatus.window[0].udec_index = hd_udec_index;
        mCurrentSyncStatus.window[0].render_index = 0;
        mCurrentSyncStatus.window[0].display_disable = 0;

        mCurrentVideoPostPConfig.render[0].render_id = 0;
        mCurrentVideoPostPConfig.render[0].udec_id = hd_udec_index;
        mCurrentVideoPostPConfig.render[0].win_config_id = 0;

        for (index = 1, udec_index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++, udec_index ++) {
            mCurrentVideoPostPConfig.render[index].render_id = index;
            if (udec_index == focus_index) {
                udec_index ++;
            }
            mCurrentVideoPostPConfig.render[index].udec_id = udec_index;
            mCurrentVideoPostPConfig.render[index].win_config_id = index;

            mCurrentSyncStatus.window[index].udec_index = udec_index;
            mCurrentSyncStatus.window[index].render_index = index;
            mCurrentSyncStatus.window[index].display_disable = 0;
        }
        mCurrentSyncStatus.window[index].display_disable = 1;

        mCurrentSyncStatus.current_display_window_number = mCurrentSyncStatus.channel_number_per_group;

        if (mpUniqueVideoPostProcessor) {
            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 1;
            mCurrentVideoPostPConfig.single_view_mode = 0;
            err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_configure, 1, (void *)&mCurrentVideoPostPConfig);
            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 0;
            DASSERT_OK(err);
            if (EECode_OK != err) {
                LOGM_FATAL("EFilterPropertyType_vpostp_configure return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOGM_FATAL("NULL mpUniqueVideoPostProcessor\n");
            return EECode_BadState;
        }

        resumeSuspendPlaybackPipelines(hd_udec_index, hd_udec_index + 1, focus_index);
        suspendPlaybackPipelines(focus_index, focus_index + 1, 0);

        for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
            if (index != focus_index) {
                resumeSuspendPlaybackPipelines(index, index + 1, 0);
            }
        }

    } else if (EDisplayLayout_Rectangle == layout) {
        if (update_window_params) {
            calculateDisplayLayoutWindow(&mCurrentVideoPostPConfig.window[0], mCurrentVideoPostPGlobalSetting.display_width, mCurrentVideoPostPGlobalSetting.display_height, (TU8)layout, (TU8)hd_udec_index, 0);

            for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
                mCurrentSyncStatus.window[index].window_width = mCurrentVideoPostPConfig.window[index].target_win_width;
                mCurrentSyncStatus.window[index].window_height = mCurrentVideoPostPConfig.window[index].target_win_height;
                mCurrentSyncStatus.window[index].window_pos_x = mCurrentVideoPostPConfig.window[index].target_win_offset_x;
                mCurrentSyncStatus.window[index].window_pos_y = mCurrentVideoPostPConfig.window[index].target_win_offset_y;
            }

            mCurrentVideoPostPConfig.total_num_win_configs = mCurrentSyncStatus.channel_number_per_group + 1;
            mCurrentVideoPostPConfig.total_window_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.total_render_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.total_decoder_number = mCurrentSyncStatus.channel_number_per_group + 1;

            mCurrentVideoPostPConfig.cur_window_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.cur_render_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.cur_decoder_number = mCurrentSyncStatus.channel_number_per_group + 1;

            mCurrentVideoPostPConfig.cur_window_start_index = 0;
            mCurrentVideoPostPConfig.cur_render_start_index = 0;
            mCurrentVideoPostPConfig.cur_decoder_start_index  = 0;
        }

        for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
            mCurrentVideoPostPConfig.render[index].render_id = index;
            mCurrentVideoPostPConfig.render[index].udec_id = index;
            mCurrentVideoPostPConfig.render[index].win_config_id = index;

            mCurrentSyncStatus.window[index].udec_index = index;
            mCurrentSyncStatus.window[index].render_index = index;
            mCurrentSyncStatus.window[index].display_disable = 0;
        }
        mCurrentSyncStatus.window[index].display_disable = 1;

        mCurrentSyncStatus.current_display_window_number = mCurrentSyncStatus.channel_number_per_group;

        if (mpUniqueVideoPostProcessor) {
            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 1;
            mCurrentVideoPostPConfig.single_view_mode = 0;
            err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_configure, 1, (void *)&mCurrentVideoPostPConfig);
            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 0;
            DASSERT_OK(err);
            if (EECode_OK != err) {
                LOGM_FATAL("EFilterPropertyType_vpostp_configure return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOGM_FATAL("NULL mpUniqueVideoPostProcessor\n");
            return EECode_BadState;
        }

        suspendPlaybackPipelines(hd_udec_index, hd_udec_index + 1, 0);

        for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
            resumeSuspendPlaybackPipelines(index, index + 1, 0);
        }
    } else if (EDisplayLayout_BottomLeftHighLighten == layout) {
        if (update_window_params) {
            calculateDisplayLayoutWindow(&mCurrentVideoPostPConfig.window[0], mCurrentVideoPostPGlobalSetting.display_width, mCurrentVideoPostPGlobalSetting.display_height, (TU8)layout, (TU8)hd_udec_index, 0);

            for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
                mCurrentSyncStatus.window[index].window_width = mCurrentVideoPostPConfig.window[index].target_win_width;
                mCurrentSyncStatus.window[index].window_height = mCurrentVideoPostPConfig.window[index].target_win_height;
                mCurrentSyncStatus.window[index].window_pos_x = mCurrentVideoPostPConfig.window[index].target_win_offset_x;
                mCurrentSyncStatus.window[index].window_pos_y = mCurrentVideoPostPConfig.window[index].target_win_offset_y;
            }

            mCurrentVideoPostPConfig.total_num_win_configs = mCurrentSyncStatus.channel_number_per_group + 1;
            mCurrentVideoPostPConfig.total_window_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.total_render_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.total_decoder_number = mCurrentSyncStatus.channel_number_per_group + 1;

            mCurrentVideoPostPConfig.cur_window_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.cur_render_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.cur_decoder_number = mCurrentSyncStatus.channel_number_per_group + 1;

            mCurrentVideoPostPConfig.cur_window_start_index = 0;
            mCurrentVideoPostPConfig.cur_render_start_index = 0;
            mCurrentVideoPostPConfig.cur_decoder_start_index  = 0;
        }

        for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
            mCurrentVideoPostPConfig.render[index].render_id = index;
            mCurrentVideoPostPConfig.render[index].udec_id = index;
            mCurrentVideoPostPConfig.render[index].win_config_id = index;

            mCurrentSyncStatus.window[index].udec_index = index;
            mCurrentSyncStatus.window[index].render_index = index;
            mCurrentSyncStatus.window[index].display_disable = 0;
        }
        mCurrentSyncStatus.window[index].display_disable = 1;

        mCurrentSyncStatus.current_display_window_number = mCurrentSyncStatus.channel_number_per_group;

        if (mpUniqueVideoPostProcessor) {
            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 1;
            mCurrentVideoPostPConfig.single_view_mode = 0;
            err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_configure, 1, (void *)&mCurrentVideoPostPConfig);
            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 0;
            DASSERT_OK(err);
            if (EECode_OK != err) {
                LOGM_FATAL("EFilterPropertyType_vpostp_configure return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOGM_FATAL("NULL mpUniqueVideoPostProcessor\n");
            return EECode_BadState;
        }

        suspendPlaybackPipelines(hd_udec_index, hd_udec_index + 1, 0);

        for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
            resumeSuspendPlaybackPipelines(index, index + 1, 0);
        }
    } else if (EDisplayLayout_SingleWindowView == layout) {
        mCurrentVideoPostPConfig.window[0].win_config_id = 0;

        mCurrentVideoPostPConfig.window[0].input_width = 0;
        mCurrentVideoPostPConfig.window[0].input_height = 0;
        mCurrentVideoPostPConfig.window[0].input_offset_x = 0;
        mCurrentVideoPostPConfig.window[0].input_offset_y = 0;

        mCurrentVideoPostPConfig.window[0].target_win_width = mCurrentVideoPostPGlobalSetting.display_width;
        mCurrentVideoPostPConfig.window[0].target_win_height = mCurrentVideoPostPGlobalSetting.display_height;
        mCurrentVideoPostPConfig.window[0].target_win_offset_x = 0;
        mCurrentVideoPostPConfig.window[0].target_win_offset_y = 0;

        mCurrentSyncStatus.window[0].window_width = mCurrentVideoPostPConfig.window[0].target_win_width;
        mCurrentSyncStatus.window[0].window_height = mCurrentVideoPostPConfig.window[0].target_win_height;
        mCurrentSyncStatus.window[0].window_pos_x = mCurrentVideoPostPConfig.window[0].target_win_offset_x;
        mCurrentSyncStatus.window[0].window_pos_y = mCurrentVideoPostPConfig.window[0].target_win_offset_y;

        mCurrentVideoPostPConfig.total_num_win_configs = 2;
        mCurrentVideoPostPConfig.total_window_number = 1;
        mCurrentVideoPostPConfig.total_render_number = 1;
        mCurrentVideoPostPConfig.total_decoder_number = mCurrentSyncStatus.channel_number_per_group + 1;

        mCurrentVideoPostPConfig.cur_window_number = 1;
        mCurrentVideoPostPConfig.cur_render_number = 1;
        mCurrentVideoPostPConfig.cur_decoder_number = mCurrentSyncStatus.channel_number_per_group + 1;

        mCurrentVideoPostPConfig.cur_window_start_index = 0;
        mCurrentVideoPostPConfig.cur_render_start_index = 0;
        mCurrentVideoPostPConfig.cur_decoder_start_index  = 0;

        for (index = 1; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
            mCurrentSyncStatus.window[index].display_disable = 1;
        }
        mCurrentSyncStatus.window[index].display_disable = 1;

        mCurrentSyncStatus.current_display_window_number = 1;

        if (mpUniqueVideoPostProcessor) {
            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 1;
            err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_configure, 1, (void *)&mCurrentVideoPostPConfig);
            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 0;
            DASSERT_OK(err);
            if (EECode_OK != err) {
                LOGM_FATAL("EFilterPropertyType_vpostp_configure return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOGM_FATAL("NULL mpUniqueVideoPostProcessor\n");
            return EECode_BadState;
        }

        resumeSuspendPlaybackPipelines(hd_udec_index, hd_udec_index + 1, focus_index);

        for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
            suspendPlaybackPipelines(index, index + 1, 0);
        }
    } else {
        LOGM_WARN("processUpdateMudecDisplayLayout. TODO\n");
    }

    LOGM_NOTICE("[DEBUG] processUpdateMudecDisplayLayout end, input (layout %d, focus_index %d)\n", layout, focus_index);
    mCurrentSyncStatus.display_layout = layout;
    mCurrentSyncStatus.display_hd_channel_index = focus_index;

    return EECode_OK;
}

void CActiveGenericEngine::resetZoomParameters()
{
    TU32 i = 0;

    for (i = 0; i < DQUERY_MAX_DISPLAY_WINDOW_NUMBER; i ++) {
        mCurrentSyncStatus.window_zoom_history[i].zoom_factor = 1;
        mCurrentSyncStatus.window_zoom_history[i].center_x = 0.5;
        mCurrentSyncStatus.window_zoom_history[i].center_y = 0.5;
    }
}

EECode CActiveGenericEngine::processZoom(TU32 window_id, TU32 width, TU32 height, TU32 input_center_x, TU32 input_center_y)
{
    TU32 render_id = 0;
    TU32 udec_id = 0;
    SDSPControlParams params;

    LOGM_NOTICE("[DEBUG] processZoom, input (window_id %d, width %08x, height %08x, center_x %08x, center_y %08x)\n", render_id, width, height, input_center_x, input_center_y);

    if (window_id >= mCurrentSyncStatus.current_display_window_number) {
        LOGM_ERROR("BAD window id %d\n", window_id);
        return EECode_BadParam;
    }

    render_id = mCurrentSyncStatus.window[window_id].render_index;
    udec_id = mCurrentSyncStatus.window[window_id].udec_index;

    input_center_x = (TU32)(((TU64)input_center_x * (TU64)mCurrentSyncStatus.source_info[udec_id].pic_width) / DSACP_FIX_POINT_DEN);
    input_center_y = (TU32)(((TU64)input_center_y * (TU64)mCurrentSyncStatus.source_info[udec_id].pic_height) / DSACP_FIX_POINT_DEN);
    width = (TU32)(((TU64)width * (TU64)mCurrentSyncStatus.source_info[udec_id].pic_width) / DSACP_FIX_POINT_DEN);
    height = (TU32)(((TU64)height * (TU64)mCurrentSyncStatus.source_info[udec_id].pic_height) / DSACP_FIX_POINT_DEN);

    if (DUnlikely((2 * input_center_x) < width)) {
        LOGM_WARN("input_center_x (%d) too small, set to width(%d)/2, %d\n", input_center_x, width, width / 2);
        input_center_x = width / 2;
    } else if (DUnlikely((input_center_x + width / 2) > mCurrentSyncStatus.source_info[udec_id].pic_width)) {
        LOGM_WARN("input_center_x (%d) too large, set to tot_width(%d) - width(%d)/2, %d\n", input_center_x, mCurrentSyncStatus.source_info[udec_id].pic_width, width, mCurrentSyncStatus.source_info[udec_id].pic_width - width / 2);
        input_center_x = mCurrentSyncStatus.source_info[udec_id].pic_width - width / 2;
    }

    if (DUnlikely((2 * input_center_y) < height)) {
        LOGM_WARN("input_center_y (%d) too small, set to height(%d)/2, %d\n", input_center_y, height, height / 2);
        input_center_y = height / 2;
    } else if (DUnlikely((input_center_y + height / 2) > mCurrentSyncStatus.source_info[udec_id].pic_height)) {
        LOGM_WARN("input_center_y (%d) too large, set to tot_height(%d) - height(%d)/2, %d\n", input_center_y, mCurrentSyncStatus.source_info[udec_id].pic_height, height, mCurrentSyncStatus.source_info[udec_id].pic_height - height / 2);
        input_center_y = mCurrentSyncStatus.source_info[udec_id].pic_height - height / 2;
    }

    params.u8_param[0] = render_id;
    params.u8_param[1] = 0;
    params.u16_param[0] = (TU16)input_center_x;
    params.u16_param[1] = (TU16)input_center_y;
    params.u16_param[2] = (TU16)width;
    params.u16_param[3] = (TU16)height;

    mCurrentSyncStatus.zoom[udec_id].zoom_size_x = width;
    mCurrentSyncStatus.zoom[udec_id].zoom_size_y = height;
    mCurrentSyncStatus.zoom[udec_id].zoom_input_center_x = input_center_x;
    mCurrentSyncStatus.zoom[udec_id].zoom_input_center_y = input_center_y;

    LOGM_NOTICE("[DEBUG] processZoom, after convert params (render_id %d, udec_id %d, width %d, height %d, center_x %d, center_y %d)\n", render_id, udec_id, width, height, input_center_x, input_center_y);

    if (DLikely(mpDSPAPI)) {
        //LOGM_ERROR("fake zoom, return directly\n");
        //return EECode_OK;
        mpDSPAPI->DSPControl(EDSPControlType_UDEC_zoom, (void *)&params);
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::processZoom(TU32 window_id, TU32 zoom_factor, TU32 offset_center_x, TU32 offset_center_y)
{
    TU32 render_id = 0;
    TU32 udec_id = 0;
    SDSPControlParams params;

    LOGM_PRINTF("[DEBUG] processZoom, input (window_id %d, zoom_factor %08x, center_x %08x, center_y %08x)\n", render_id, zoom_factor, offset_center_x, offset_center_y);

    if (window_id >= mCurrentSyncStatus.current_display_window_number) {
        LOGM_ERROR("BAD window id %d\n", window_id);
        return EECode_BadParam;
    }

    render_id = mCurrentSyncStatus.window[window_id].render_index;
    udec_id = mCurrentSyncStatus.window[window_id].udec_index;

    float cur_zoom_factor = mCurrentSyncStatus.window_zoom_history[window_id].zoom_factor;
    float cur_center_x = mCurrentSyncStatus.window_zoom_history[window_id].center_x, cur_center_y = mCurrentSyncStatus.window_zoom_history[window_id].center_y;

    LOGM_PRINTF("[DEBUG], current zoom_factor %f, center_x %f, center_y %f\n", cur_zoom_factor, cur_center_x, cur_center_y);
    if (offset_center_x & DSACP_FIX_POINT_SYGN_FLAG) {
        offset_center_x &= ~(DSACP_FIX_POINT_SYGN_FLAG);
        float offset = ((float)offset_center_x / (float)DSACP_FIX_POINT_DEN) / cur_zoom_factor;
        cur_center_x += offset;
        LOGM_PRINTF("to right, offset %f, after %f\n", offset, cur_center_x);
    } else {
        float offset = ((float)offset_center_x / (float)DSACP_FIX_POINT_DEN) / cur_zoom_factor;
        LOGM_PRINTF("to left, offset %f, after %f\n", offset, cur_center_x);
        cur_center_x -= offset;
    }
    if (offset_center_y & DSACP_FIX_POINT_SYGN_FLAG) {
        offset_center_y &= ~(DSACP_FIX_POINT_SYGN_FLAG);
        float offset = ((float)offset_center_y / (float)DSACP_FIX_POINT_DEN) / cur_zoom_factor;
        cur_center_y += offset;
        LOGM_PRINTF("to down, offset %f, after %f\n", offset, cur_center_y);
    } else {
        float offset = ((float)offset_center_y / (float)DSACP_FIX_POINT_DEN) / cur_zoom_factor;
        cur_center_y -= offset;
        LOGM_PRINTF("to up, offset %f, after %f\n", offset, cur_center_y);
    }
    cur_zoom_factor *= (float)zoom_factor / (float)DSACP_FIX_POINT_DEN;
    LOGM_PRINTF("[DEBUG], after caluculate: zoom_factor %f, center_x %f, center_y %f\n", cur_zoom_factor, cur_center_x, cur_center_y);

    if (DUnlikely(cur_zoom_factor < 1)) {
        LOGM_NOTICE("zoom factor (%f) less than 1, reset to original size\n", cur_zoom_factor);
        cur_center_x = 0.5;
        cur_center_y = 0.5;
        cur_zoom_factor = 1.0;
    } else {
        float cur_half_size = 1.0 / cur_zoom_factor / 2.0;
        if (cur_half_size > cur_center_x) {
            cur_center_x = cur_half_size;
        }
        if ((cur_half_size + cur_center_x) > 1) {
            cur_center_x = 1 - cur_half_size;
        }

        if (cur_half_size > cur_center_y) {
            cur_center_y = cur_half_size;
        }

        if ((cur_half_size + cur_center_y) > 1) {
            cur_center_y = 1 - cur_half_size;
        }
    }
    LOGM_PRINTF("[DEBUG], after adjust: zoom_factor %f, center_x %f, center_y %f\n", cur_zoom_factor, cur_center_x, cur_center_y);

    TU32 c_size_x = mCurrentSyncStatus.source_info[udec_id].pic_width / cur_zoom_factor;
    TU32 c_size_y = mCurrentSyncStatus.source_info[udec_id].pic_height / cur_zoom_factor;
    TU32 c_offset_x = mCurrentSyncStatus.source_info[udec_id].pic_width * cur_center_x;
    TU32 c_offset_y = mCurrentSyncStatus.source_info[udec_id].pic_height * cur_center_y;

    LOGM_PRINTF("[DEBUG], udec %d source info: width %d, height %d\n", udec_id, mCurrentSyncStatus.source_info[udec_id].pic_width, mCurrentSyncStatus.source_info[udec_id].pic_height);

    c_offset_x &= ~(0x1);
    c_size_x &= ~(0x1);

    if (DUnlikely(c_offset_x > mCurrentSyncStatus.source_info[udec_id].pic_width)) {
        LOGM_ERROR("reset!\n");
        c_offset_x = mCurrentSyncStatus.source_info[udec_id].pic_width / 2;
    }

    if (DUnlikely(c_offset_x < ((c_size_x + 1) / 2))) {
        c_size_x = c_offset_x * 2;
    }

    if (DUnlikely((c_offset_x + ((c_size_x + 1) / 2)) > mCurrentSyncStatus.source_info[udec_id].pic_width)) {
        LOGM_WARN("x size exceed\n");
        c_size_x = (mCurrentSyncStatus.source_info[udec_id].pic_width - c_offset_x) * 2;
    }

    if (DUnlikely(c_offset_y > mCurrentSyncStatus.source_info[udec_id].pic_height)) {
        LOGM_ERROR("reset!\n");
        c_offset_y = mCurrentSyncStatus.source_info[udec_id].pic_height / 2;
    }

    if (DUnlikely(c_offset_y < ((c_size_y + 1) / 2))) {
        c_size_y = c_offset_y * 2;
    }

    if (DUnlikely((c_offset_y + ((c_size_y + 1) / 2)) > mCurrentSyncStatus.source_info[udec_id].pic_height)) {
        LOGM_WARN("y size exceed\n");
        c_size_y = (mCurrentSyncStatus.source_info[udec_id].pic_height - c_offset_y) * 2;
    }

    float zoom_factor_y = (float)mCurrentSyncStatus.source_info[udec_id].pic_height / (float)c_size_y;
    float zoom_factor_x = (float)mCurrentSyncStatus.source_info[udec_id].pic_width / (float)c_size_x;

    if ((zoom_factor_y + 0.1) < zoom_factor_x) {
        cur_zoom_factor = zoom_factor_y;
    } else {
        cur_zoom_factor = zoom_factor_x;
    }

    mCurrentSyncStatus.window_zoom_history[window_id].zoom_factor = cur_zoom_factor;
    mCurrentSyncStatus.window_zoom_history[window_id].center_x = cur_center_x;
    mCurrentSyncStatus.window_zoom_history[window_id].center_y = cur_center_y;

    params.u8_param[0] = render_id;
    params.u8_param[1] = 0;
    params.u16_param[0] = (TU16)c_offset_x;
    params.u16_param[1] = (TU16)c_offset_y;
    params.u16_param[2] = (TU16)c_size_x;
    params.u16_param[3] = (TU16)c_size_y;

    mCurrentSyncStatus.zoom[udec_id].zoom_size_x = c_size_x;
    mCurrentSyncStatus.zoom[udec_id].zoom_size_y = c_size_y;
    mCurrentSyncStatus.zoom[udec_id].zoom_input_center_x = c_offset_x;
    mCurrentSyncStatus.zoom[udec_id].zoom_input_center_y = c_offset_y;

    LOGM_PRINTF("[DEBUG] processZoom, after convert params (render_id %d, udec_id %d, width %d, height %d, center_x %d, center_y %d)\n", render_id, udec_id, c_size_x, c_size_y, c_offset_x, c_offset_y);

    if (DLikely(mpDSPAPI)) {
        //LOGM_ERROR("fake zoom, return directly\n");
        //return EECode_OK;
        mpDSPAPI->DSPControl(EDSPControlType_UDEC_zoom, (void *)&params);
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::processMoveWindow(TU32 window_id, TU32 pos_x, TU32 pos_y, TU32 width, TU32 height)
{
    LOGM_NOTICE("[DEBUG] processMoveWindow, input (window_id %d, width %08x, height %08x, pos_x %08x, pos_y %08x)\n", window_id, width, height, pos_x, pos_y);

    if (window_id >= mCurrentSyncStatus.current_display_window_number) {
        LOGM_ERROR("BAD window id %d\n", window_id);
        return EECode_BadParam;
    }

    width = (((TU64)mCurrentSyncStatus.encoding_width) * (TU64)width) / DSACP_FIX_POINT_DEN;
    height = (((TU64)mCurrentSyncStatus.encoding_height) * (TU64)height) / DSACP_FIX_POINT_DEN;
    pos_x = (((TU64)mCurrentSyncStatus.encoding_width) * (TU64)pos_x) / DSACP_FIX_POINT_DEN;
    pos_y = (((TU64)mCurrentSyncStatus.encoding_height) * (TU64)pos_y) / DSACP_FIX_POINT_DEN;

    pos_x = (pos_x + 8) & (~15);
    width = (width + 8) & (~15);

    mCurrentVideoPostPConfig.window[window_id].target_win_width = width;
    mCurrentVideoPostPConfig.window[window_id].target_win_height = height;
    mCurrentVideoPostPConfig.window[window_id].target_win_offset_x = pos_x;
    mCurrentVideoPostPConfig.window[window_id].target_win_offset_y = pos_y;

    mCurrentSyncStatus.window[window_id].window_width = width;
    mCurrentSyncStatus.window[window_id].window_height = height;
    mCurrentSyncStatus.window[window_id].window_pos_x = pos_x;
    mCurrentSyncStatus.window[window_id].window_pos_y = pos_y;

    LOGM_NOTICE("[DEBUG] processMoveWindow, after convert (window_id %d, width %d, height %d, pos_x %d, pos_y %d)\n", window_id, width, height, pos_x, pos_y);

#if 0
    if (mpUniqueVideoPostProcessor) {
        mCurrentVideoPostPConfig.set_config_direct_to_dsp = 1;
        EECode err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_configure, 1, (void *)&mCurrentVideoPostPConfig);
        mCurrentVideoPostPConfig.set_config_direct_to_dsp = 0;
        DASSERT_OK(err);
        if (EECode_OK != err) {
            LOGM_FATAL("EFilterPropertyType_vpostp_configure return %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    } else {
        LOGM_FATAL("NULL mpUniqueVideoPostProcessor\n");
        return EECode_BadState;
    }
#else
    if (DLikely(mpDSPAPI)) {
        mCurrentVideoPostPConfig.input_param_update_window_index = window_id;
        EECode err = mpDSPAPI->DSPControl(EDSPControlType_UDEC_one_window_config, (void *)&mCurrentVideoPostPConfig);
        mCurrentVideoPostPConfig.input_param_update_window_index = 0;
        DASSERT_OK(err);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_FATAL("EDSPControlType_UDEC_window_config return %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    } else {
        LOGM_FATAL("NULL mpDSPAPI\n");
        return EECode_BadState;
    }
#endif

    return EECode_OK;
}

EECode CActiveGenericEngine::processShowOtherWindows(TU32 window_index, TU32 show)
{
    EECode err = EECode_OK;

    if (window_index >= mCurrentSyncStatus.current_display_window_number) {
        LOGM_ERROR("BAD window id %d\n", window_index);
        return EECode_BadParam;
    }

    if (!show) {
        LOGM_NOTICE("hide others, window index %d\n", window_index);
        mCurrentVideoPostPConfig.single_view_mode = 1;
        mCurrentVideoPostPConfig.single_view_render.render_id = 0;
        mCurrentVideoPostPConfig.single_view_render.win_config_id = 0;
        mCurrentVideoPostPConfig.single_view_render.win_config_id_2nd = 0xff;
        mCurrentVideoPostPConfig.single_view_render.input_source_type = 1;
        mCurrentVideoPostPConfig.single_view_render.first_pts_low = 0;
        mCurrentVideoPostPConfig.single_view_render.first_pts_high = 0;
        mCurrentVideoPostPConfig.single_view_render.udec_id = mCurrentSyncStatus.window[window_index].udec_index;

        mCurrentVideoPostPConfig.single_view_window.win_config_id = 0;
        mCurrentVideoPostPConfig.single_view_window.input_offset_x = 0;
        mCurrentVideoPostPConfig.single_view_window.input_offset_y = 0;
        mCurrentVideoPostPConfig.single_view_window.input_width = 0;
        mCurrentVideoPostPConfig.single_view_window.input_height = 0;

        mCurrentVideoPostPConfig.single_view_window.target_win_offset_x = mCurrentSyncStatus.window[window_index].window_pos_x;
        mCurrentVideoPostPConfig.single_view_window.target_win_offset_y = mCurrentSyncStatus.window[window_index].window_pos_y;
        mCurrentVideoPostPConfig.single_view_window.target_win_width = mCurrentSyncStatus.window[window_index].window_width;
        mCurrentVideoPostPConfig.single_view_window.target_win_height = mCurrentSyncStatus.window[window_index].window_height;
    } else {
        LOGM_NOTICE("show others, window index %d\n", window_index);
        mCurrentVideoPostPConfig.single_view_mode = 0;
    }

    if (mpUniqueVideoPostProcessor) {
        mCurrentVideoPostPConfig.set_config_direct_to_dsp = 1;
        err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_configure, 1, (void *)&mCurrentVideoPostPConfig);
        mCurrentVideoPostPConfig.set_config_direct_to_dsp = 0;
        DASSERT_OK(err);
        if (EECode_OK != err) {
            LOGM_FATAL("EFilterPropertyType_vpostp_configure return %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    } else {
        LOGM_FATAL("NULL mpUniqueVideoPostProcessor\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::processDemandIDR(TU32 param1)
{
    EECode err = EECode_OK;

    if (mpDSPAPI) {
        SVideoEncoderParam encoder_params;

        memset(&encoder_params, 0x0, sizeof(encoder_params));
        encoder_params.index = 0;//hard code here
        encoder_params.demand_idr_strategy = 1;//hard code here

        err = mpDSPAPI->DSPControl(EDSPControlType_UDEC_TRANSCODER_demand_IDR, (void *)&encoder_params);
        if (EECode_OK != err) {
            LOGM_FATAL("EDSPControlType_UDEC_TRANSCODER_demand_IDR return %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    } else {
        LOGM_FATAL("NULL mpDSPAPI\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::processUpdateBitrate(TU32 param1)
{
    EECode err = EECode_OK;

    if (mpDSPAPI) {
        SVideoEncoderParam encoder_params;

        memset(&encoder_params, 0x0, sizeof(encoder_params));
        encoder_params.index = 0;//hard code here
        encoder_params.bitrate = param1;
        encoder_params.bitrate_pts = 0;

        err = mpDSPAPI->DSPControl(EDSPControlType_UDEC_TRANSCODER_update_bitrate, (void *)&encoder_params);
        if (EECode_OK != err) {
            LOGM_FATAL("EDSPControlType_UDEC_TRANSCODER_update_bitrate return %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    } else {
        LOGM_FATAL("NULL mpDSPAPI\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::processUpdateFramerate(TU32 param1)
{
    EECode err = EECode_OK;

    if (mpDSPAPI) {
        SVideoEncoderParam encoder_params;

        memset(&encoder_params, 0x0, sizeof(encoder_params));
        encoder_params.index = 0;//hard code here
        encoder_params.framerate_reduce_factor = 1;
        encoder_params.framerate = 0;

        //err = mpDSPAPI->DSPControl(EDSPControlType_UDEC_TRANSCODER_update_framerate, (void*)&encoder_params);
        if (EECode_OK != err) {
            LOGM_FATAL("EDSPControlType_UDEC_TRANSCODER_update_framerate return %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    } else {
        LOGM_FATAL("NULL mpDSPAPI\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::processSwapWindowContent(TU32 window_id_1, TU32 window_id_2)
{
    TComponentIndex udec_index_tmp_1 = 0;
    TComponentIndex udec_index_tmp_2 = 0;
    TComponentIndex hd_udec_index = mCurrentSyncStatus.channel_number_per_group;

    LOGM_NOTICE("[DEBUG] processSwapWindowContent (window_id_1 %d, window_id_2 %d)\n", window_id_1, window_id_2);

    if (DUnlikely(window_id_1 == window_id_2)) {
        LOGM_ERROR("window_id is the same? %d %d\n", window_id_1, window_id_2);
        return EECode_BadParam;
    }

    if (DUnlikely(window_id_1 >= mCurrentSyncStatus.channel_number_per_group)) {
        LOGM_ERROR("window_id_1 %d exceed max value %d\n", window_id_1, mCurrentSyncStatus.channel_number_per_group);
        return EECode_BadParam;
    }

    if (DUnlikely(window_id_2 >= mCurrentSyncStatus.channel_number_per_group)) {
        LOGM_ERROR("window_id_2 %d exceed max value %d\n", window_id_2, mCurrentSyncStatus.channel_number_per_group);
        return EECode_BadParam;
    }

    if (EDisplayLayout_TelePresence == mCurrentSyncStatus.display_layout) {
        LOGM_NOTICE("in EDisplayLayout_TelePresence layout\n");
        udec_index_tmp_1 = mCurrentSyncStatus.window[window_id_1].udec_index;
        udec_index_tmp_2 = mCurrentSyncStatus.window[window_id_2].udec_index;

        if ((udec_index_tmp_1 != hd_udec_index) && (udec_index_tmp_2 != hd_udec_index)) {
            mCurrentSyncStatus.window[window_id_1].udec_index = udec_index_tmp_2;
            mCurrentSyncStatus.window[window_id_2].udec_index = udec_index_tmp_1;

            mCurrentVideoPostPConfig.render[mCurrentSyncStatus.window[window_id_1].render_index].udec_id = udec_index_tmp_2;
            mCurrentVideoPostPConfig.render[mCurrentSyncStatus.window[window_id_2].render_index].udec_id = udec_index_tmp_1;

        } else if (udec_index_tmp_1 == hd_udec_index) {
            resumeSuspendPlaybackPipelines(hd_udec_index, hd_udec_index + 1, udec_index_tmp_2);

            mCurrentSyncStatus.window[window_id_2].udec_index = mCurrentSyncStatus.display_hd_channel_index;
            mCurrentVideoPostPConfig.render[mCurrentSyncStatus.window[window_id_2].render_index].udec_id = mCurrentSyncStatus.display_hd_channel_index;

            mCurrentSyncStatus.display_hd_channel_index = udec_index_tmp_2;
            suspendPlaybackPipelines(udec_index_tmp_2, udec_index_tmp_2 + 1, 0);
            resumeSuspendPlaybackPipelines(mCurrentSyncStatus.window[window_id_2].udec_index, mCurrentSyncStatus.window[window_id_2].udec_index + 1, 0);
        } else if (udec_index_tmp_2 == hd_udec_index) {
            resumeSuspendPlaybackPipelines(hd_udec_index, hd_udec_index + 1, udec_index_tmp_1);

            mCurrentSyncStatus.window[window_id_1].udec_index = mCurrentSyncStatus.display_hd_channel_index;
            mCurrentVideoPostPConfig.render[mCurrentSyncStatus.window[window_id_1].render_index].udec_id = mCurrentSyncStatus.display_hd_channel_index;

            mCurrentSyncStatus.display_hd_channel_index = udec_index_tmp_1;
            suspendPlaybackPipelines(udec_index_tmp_1, udec_index_tmp_1 + 1, 0);
            resumeSuspendPlaybackPipelines(mCurrentSyncStatus.window[window_id_1].udec_index, mCurrentSyncStatus.window[window_id_1].udec_index + 1, 0);
        } else {
            LOGM_ERROR("why comes here, udec_index_tmp_1 %d, udec_index_tmp_2 %d, hd_udec_index %d, mCurrentSyncStatus.display_hd_channel_index %d\n", udec_index_tmp_1, udec_index_tmp_2, hd_udec_index, mCurrentSyncStatus.display_hd_channel_index);
            return EECode_BadParam;
        }

        if (mpUniqueVideoPostProcessor) {
            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 1;
            EECode err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_configure, 1, (void *)&mCurrentVideoPostPConfig);
            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 0;
            DASSERT_OK(err);
            if (EECode_OK != err) {
                LOGM_FATAL("EFilterPropertyType_vpostp_configure return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOGM_FATAL("NULL mpUniqueVideoPostProcessor\n");
            return EECode_BadState;
        }

    } else if ((EDisplayLayout_Rectangle == mCurrentSyncStatus.display_layout) || (EDisplayLayout_BottomLeftHighLighten == mCurrentSyncStatus.display_layout)) {
        udec_index_tmp_1 = mCurrentSyncStatus.window[window_id_1].udec_index;
        udec_index_tmp_2 = mCurrentSyncStatus.window[window_id_2].udec_index;

        if ((udec_index_tmp_1 != hd_udec_index) && (udec_index_tmp_2 != hd_udec_index)) {
            mCurrentSyncStatus.window[window_id_1].udec_index = udec_index_tmp_2;
            mCurrentSyncStatus.window[window_id_2].udec_index = udec_index_tmp_1;

            mCurrentVideoPostPConfig.render[mCurrentSyncStatus.window[window_id_1].render_index].udec_id = udec_index_tmp_2;
            mCurrentVideoPostPConfig.render[mCurrentSyncStatus.window[window_id_2].render_index].udec_id = udec_index_tmp_1;

        } else {
            LOGM_ERROR("why comes here, udec_index_tmp_1 %d, udec_index_tmp_2 %d, hd_udec_index %d\n", udec_index_tmp_1, udec_index_tmp_2, hd_udec_index);
            return EECode_BadParam;
        }

        if (mpUniqueVideoPostProcessor) {
            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 1;
            EECode err = mpUniqueVideoPostProcessor->FilterProperty(EFilterPropertyType_vpostp_configure, 1, (void *)&mCurrentVideoPostPConfig);
            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 0;
            DASSERT_OK(err);
            if (EECode_OK != err) {
                LOGM_FATAL("EFilterPropertyType_vpostp_configure return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOGM_FATAL("NULL mpUniqueVideoPostProcessor\n");
            return EECode_BadState;
        }

    } else {
        LOGM_ERROR("not expected %d\n", mCurrentSyncStatus.display_layout);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CActiveGenericEngine::processCircularShiftWindowContent(TU32 shift, TU32 is_ccw)
{
    LOGM_WARN("processCircularShiftWindowContent.....TODO\n");

    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif



