/*******************************************************************************
 * active_generic_engine_v3.cpp
 *
 * History:
 *    2014/10/13 - [Zhi He] create file
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

//#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"
#include "common_network_utils.h"

#include "common_hash_table.h"

#include "cloud_lib_if.h"
#include "sacp_types.h"

#include "storage_lib_if.h"
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

#include "engine_internal.h"
#include "active_generic_engine_v3.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

#ifdef BUILD_CONFIG_WITH_LICENCE
#include "licence.check"
#endif

IGenericEngineControlV3 *CreateGenericEngineV3(IStorageManagement *p_storage, SSharedComponent *p_shared_component)
{
    return (IGenericEngineControlV3 *)CActiveGenericEngineV3::Create("CActiveGenericEngineV3", 0, p_storage, p_shared_component);
}

//-----------------------------------------------------------------------
//
// CActiveGenericEngineV3
//
//-----------------------------------------------------------------------
CActiveGenericEngineV3 *CActiveGenericEngineV3::Create(const TChar *pname, TUint index, IStorageManagement *pstorage, SSharedComponent *p_shared_component)
{
    CActiveGenericEngineV3 *result = new CActiveGenericEngineV3(pname, index, pstorage);
    if (result && result->Construct(p_shared_component) != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CActiveGenericEngineV3::CActiveGenericEngineV3(const TChar *pname, TUint index, IStorageManagement *pstorage)
    : inherited(pname, index)
    , mnPlaybackPipelinesMaxNumber(EPipelineMaxNumber_Playback)
    , mnPlaybackPipelinesCurrentNumber(0)
    , mnRecordingPipelinesMaxNumber(EPipelineMaxNumber_Recording)
    , mnRecordingPipelinesCurrentNumber(0)
    , mnStreamingPipelinesMaxNumber(EPipelineMaxNumber_Streaming)
    , mnStreamingPipelinesCurrentNumber(0)
    , mnCloudPipelinesMaxNumber(EPipelineMaxNumber_CloudPush)
    , mnCloudPipelinesCurrentNumber(0)
    , mnNativeCapturePipelinesMaxNumber(EPipelineMaxNumber_NativeCapture)
    , mnNativeCapturePipelinesCurrentNumber(0)
    , mnNativePushPipelinesMaxNumber(EPipelineMaxNumber_NativePush)
    , mnNativePushPipelinesCurrentNumber(0)
    , mnCloudUploadPipelinesMaxNumber(EPipelineMaxNumber_CloudUpload)
    , mnCloudUploadPipelinesCurrentNumber(0)
    , mnVODPipelinesMaxNumber(EPipelineMaxNumber_VOD)
    , mnVODPipelinesCurrentNumber(0)
    , mpPlaybackPipelines(NULL)
    , mpRecordingPipelines(NULL)
    , mpStreamingPipelines(NULL)
    , mpCloudPipelines(NULL)
    , mpNativeCapturePipelines(NULL)
    , mpNativePushPipelines(NULL)
    , mpCloudUploadPipelines(NULL)
    , mpVODPipelines(NULL)
    , mpDSPAPI(NULL)
    , mIavFd(-1)
    , mbSetMediaConfig(0)
    , mbDeviceOpened(0)
    , mbDeviceActivated(0)
    , mbExternalClock(0)
    , mpClockManager(NULL)
    , mpSystemClockReference(NULL)
    , mpClockSource(NULL)
    , mpStorageManager(pstorage)
    , mbRun(1)
    , mbGraghBuilt(0)
    , mbSchedulersCreated(0)
    , mbEnginePipelineRunning(0)
    , mpMutex(NULL)
    , msState(EEngineState_not_alive)
    , mpStreamingServerManager(NULL)
{
    TUint i = 0;
    EECode err = EECode_OK;

    mpUniqueVideoPostProcessor = NULL;
    mpUniqueAudioPostProcessor = NULL;
    mpUniqueVideoRenderer = NULL;
    mpUniqueAudioRenderer = NULL;
    mpUniqueAudioDecoder = NULL;
    mpUniqueVideoCapture = NULL;
    mpUniqueAudioCapture = NULL;
    mpUniqueAudioPinMuxer = NULL;
    mpUniqueVideoPostProcessorComponent = NULL;
    mpUniqueAudioPostProcessorComponent = NULL;
    mpUniqueVideoRendererComponent = NULL;
    mpUniqueAudioRendererComponent = NULL;
    mpUniqueAudioDecoderComponent = NULL;
    mpUniqueVideoCaptureComponent = NULL;
    mpUniqueAudioCaptureComponent = NULL;
    mpUniqueAudioPinMuxerComponent = NULL;

    mpStreamingContent = NULL;
    mpVodContent = NULL;
    mpCloudServerManager = NULL;
    mpCloudContent = NULL;
    mpCloudCmdContent = NULL;
    mpDebugLastCloudServer = NULL;
    mpDebugLastStreamingServer = NULL;
    mDailyCheckTime = 0;
    mDailyCheckContext = 0;
#ifdef BUILD_CONFIG_WITH_LICENCE
    mpLicenceClient = NULL;
#endif
    mpDailyCheckTimer = NULL;
    mCurrentLogFileIndex = 0;
    mCurrentSnapshotFileIndex = 0;
    mpExtraDataParser = NULL;
    mpSimpleHashTable = NULL;

    memset((void *)&mPersistMediaConfig, 0, sizeof(mPersistMediaConfig));

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
    mnComponentFiltersMaxNumbers[EGenericComponentType_CloudConnecterCmdAgent] = EComponentMaxNumber_CloudConnecterCmdAgent;
    mnComponentFiltersMaxNumbers[EGenericComponentType_FlowController] = EComponentMaxNumber_FlowController;
    mnComponentFiltersMaxNumbers[EGenericComponentType_VideoOutput] = EComponentMaxNumber_VideoOutput;
    mnComponentFiltersMaxNumbers[EGenericComponentType_AudioOutput] = EComponentMaxNumber_AudioOutput;

    mnComponentProxysMaxNumbers[EGenericComponentType_StreamingServer - EGenericComponentType_TAG_proxy_start] = EComponentMaxNumber_RTSPStreamingServer;
    mnComponentProxysMaxNumbers[EGenericComponentType_StreamingContent - EGenericComponentType_TAG_proxy_start] = EComponentMaxNumber_RTSPStreamingContent;
    mnComponentProxysMaxNumbers[EGenericComponentType_ConnectionPin - EGenericComponentType_TAG_proxy_start] = EComponentMaxNumber_ConnectionPin;
    mnComponentProxysMaxNumbers[EGenericComponentType_UserTimer - EGenericComponentType_TAG_proxy_start] = EComponentMaxNumber_UserTimer;
    mnComponentProxysMaxNumbers[EGenericComponentType_CloudServer - EGenericComponentType_TAG_proxy_start] = EComponentMaxNumber_CloudServer;
    mnComponentProxysMaxNumbers[EGenericComponentType_CloudReceiverContent - EGenericComponentType_TAG_proxy_start] = EComponentMaxNumber_CloudReceiverContent;

    memset(&mLastCheckDate, 0x0, sizeof(mLastCheckDate));
    if (0 == isLogFileOpened()) {
        mbSelfLogFile = 1;
        nextLogFile();
    } else {
        mbSelfLogFile = 0;
    }

    gfLoadDefaultMediaConfig(&mPersistMediaConfig);

    mPersistMediaConfig.engine_config.engine_version = 3;
    mPersistMediaConfig.engine_config.filter_version = 2;

    LOGM_PRINTF("try load log.config\n");
    err = gfLoadLogConfigFile(ERunTimeConfigType_XML, "log.config", &gmModuleLogConfig[0], ELogModuleListEnd);
    if (DUnlikely(EECode_NotSupported == err)) {
        LOGM_NOTICE("no xml reader\n");
    } else if (DUnlikely(EECode_OK != err)) {
        LOGM_WARN("load log.config fail ret %d, %s, write a correct default one\n", err, gfGetErrorCodeString(err));
        gfLoadDefaultLogConfig(&gmModuleLogConfig[0], ELogModuleListEnd);
        err = gfSaveLogConfigFile(ERunTimeConfigType_XML, "log.config", &gmModuleLogConfig[0], ELogModuleListEnd);
        DASSERT_OK(err);
    } else {
        LOGM_PRINTF("load log.config done\n");
        //gfPrintLogConfig(&gmModuleLogConfig[0], ELogModuleListEnd);
    }

#if 0
    if (EECode_OK != err) {
        TUint loglevel = ELogLevel_Notice, logoption = 0, logoutput = 0;
        err = gfLoadSimpleLogConfigFile((TChar *)"simple_log.config", loglevel, logoption, logoutput);
        if (EECode_OK == err) {
            gfSetLogConfigLevel((ELogLevel)loglevel, &gmModuleLogConfig[0], ELogModuleListEnd);
        }
    }
#endif

    mpListFilters = NULL;
    mpListProxys = NULL;
    mpListConnections = NULL;
    mpStreamingServerList = NULL;
    mpCloudServerList = NULL;

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

EECode CActiveGenericEngineV3::Construct(SSharedComponent *p_shared_component)
{
    DSET_MODULE_LOG_CONFIG(ELogModuleGenericEngine);

    EECode ret = inherited::Construct();
    if (DUnlikely(ret != EECode_OK)) {
        LOGM_FATAL("CActiveGenericEngineV3::Construct, inherited::Construct fail, ret %d\n", ret);
        return ret;
    }

    mpListFilters = new CIDoubleLinkedList();
    if (DUnlikely(!mpListFilters)) {
        LOGM_FATAL("new CIDoubleLinkedList() fail\n");
        return EECode_NoMemory;
    }

    mpListProxys = new CIDoubleLinkedList();
    if (DUnlikely(!mpListProxys)) {
        LOGM_FATAL("new CIDoubleLinkedList() fail\n");
        return EECode_NoMemory;
    }

    mpListConnections = new CIDoubleLinkedList();
    if (DUnlikely(!mpListConnections)) {
        LOGM_FATAL("new CIDoubleLinkedList() fail\n");
        return EECode_NoMemory;
    }

    mpStreamingServerList = new CIDoubleLinkedList();
    if (DUnlikely(!mpStreamingServerList)) {
        LOGM_FATAL("new CIDoubleLinkedList() fail\n");
        return EECode_NoMemory;
    }

    mpCloudServerList = new CIDoubleLinkedList();
    if (DUnlikely(!mpCloudServerList)) {
        LOGM_FATAL("new CIDoubleLinkedList() fail\n");
        return EECode_NoMemory;
    }

#ifdef BUILD_CONFIG_WITH_LICENCE
    mpLicenceClient = gfCreateLicenceClient(ELicenceType_StandAlone);
    if (DUnlikely(!mpLicenceClient)) {
        LOG_FATAL("gfCreateLicenceClient fail\n");
        return EECode_NoMemory;
    }

    FILE *p_licence_check_file = fopen((TChar *)"./licence.check.txt", "r");
    if (p_licence_check_file) {
        TChar *p_licenc_check_string = (TChar *)DDBG_MALLOC(4096, "E3LC");
        TU32 check_string_length = 0;
        if (DUnlikely(!p_licenc_check_string)) {
            LOG_FATAL("alloc(4096) fail\n");
            fclose(p_licence_check_file);
            p_licence_check_file = NULL;
            return EECode_NoMemory;
        }
        check_string_length = fread(p_licenc_check_string, 1, 4096, p_licence_check_file);
        DASSERT(check_string_length < 4090);
        p_licenc_check_string[check_string_length] = 0;
        ret = mpLicenceClient->Authenticate((TChar *)"./licence.txt", p_licenc_check_string, check_string_length);
        if (EECode_OK != ret) {
            LOG_ERROR("Licence Authenticate(ext) fail\n");
            ret = mpLicenceClient->Authenticate((TChar *)"./licence.txt", gLicenceCheckString, strlen(gLicenceCheckString));
            if (EECode_OK != ret) {
                LOG_ERROR("Licence Authenticate fail\n");
                fclose(p_licence_check_file);
                p_licence_check_file = NULL;
                DDBG_FREE(p_licenc_check_string, "E3LC");
                p_licenc_check_string = NULL;
                return EECode_Error;
            }
        }
        fclose(p_licence_check_file);
        p_licence_check_file = NULL;
        DDBG_FREE(p_licenc_check_string, "E3LC");
        p_licenc_check_string = NULL;
    } else {
        ret = mpLicenceClient->Authenticate((TChar *)"./licence.txt", gLicenceCheckString, strlen(gLicenceCheckString));
        if (EECode_OK != ret) {
            LOG_ERROR("Licence Authenticate fail\n");
            return EECode_Error;
        }
    }

    TU32 maxchannel = 0;
    ret = mpLicenceClient->GetCapability(maxchannel);
    if (DUnlikely(EECode_OK != ret)) {
        LOG_ERROR("Licence GetCapability fail\n");
        return ret;
    }
    DASSERT(maxchannel);
    LOGM_CONFIGURATION("max channel %d\n", maxchannel);

    if ((maxchannel + 6) < mnComponentFiltersMaxNumbers[EGenericComponentType_CloudConnecterServerAgent]) {
        mnComponentFiltersMaxNumbers[EGenericComponentType_CloudConnecterServerAgent] = maxchannel + 6;
    }

    if ((maxchannel + 6) < mnComponentFiltersMaxNumbers[EGenericComponentType_Muxer]) {
        mnComponentFiltersMaxNumbers[EGenericComponentType_Muxer] = maxchannel + 6;
    }
#endif

    //    mConfigLogLevel = ELogLevel_Info;
    //    mConfigLogOption = DCAL_BITMASK(ELogOption_State) | DCAL_BITMASK(ELogOption_Resource);

    mPersistMediaConfig.p_remote_log_server = CIRemoteLogServer::Create();
    if (DUnlikely(!mPersistMediaConfig.p_remote_log_server)) {
        LOG_FATAL("CIRemoteLogServer::Create() fail\n");
        //return EECode_NoMemory;
    } else {
        ret = mPersistMediaConfig.p_remote_log_server->CreateContext((TChar *)"/tmp/remote_bridge", 0, 0);
        if (DUnlikely(EECode_OK != ret)) {
            LOG_ERROR("CreateContext() fail, ret %d, %s\n", ret, gfGetErrorCodeString(ret));
            mPersistMediaConfig.p_remote_log_server->Destroy();
            mPersistMediaConfig.p_remote_log_server = NULL;
            //return ret;
        }
    }

    TUint request_memsize = sizeof(SPlaybackPipeline) * mnPlaybackPipelinesMaxNumber;
    mpPlaybackPipelines = (SPlaybackPipeline *) DDBG_MALLOC(request_memsize, "E3PP");
    if (DLikely(mpPlaybackPipelines)) {
        memset(mpPlaybackPipelines, 0x0, request_memsize);
    } else {
        mnPlaybackPipelinesMaxNumber = 0;
        LOG_FATAL("no memory, request %d\n", request_memsize);
        return EECode_NoMemory;
    }

    request_memsize = sizeof(SRecordingPipeline) * mnRecordingPipelinesMaxNumber;
    mpRecordingPipelines = (SRecordingPipeline *) DDBG_MALLOC(request_memsize, "E3RP");
    if (DLikely(mpRecordingPipelines)) {
        memset(mpRecordingPipelines, 0x0, request_memsize);
    } else {
        mnRecordingPipelinesMaxNumber = 0;
        LOG_FATAL("no memory, request %d\n", request_memsize);
        return EECode_NoMemory;
    }

    request_memsize = sizeof(SStreamingPipeline) * mnStreamingPipelinesMaxNumber;
    mpStreamingPipelines = (SStreamingPipeline *) DDBG_MALLOC(request_memsize, "E3SP");
    if (DLikely(mpStreamingPipelines)) {
        memset(mpStreamingPipelines, 0x0, request_memsize);
    } else {
        mnStreamingPipelinesMaxNumber = 0;
        LOG_FATAL("no memory, request %d\n", request_memsize);
        return EECode_NoMemory;
    }

    request_memsize = sizeof(SCloudPipeline) * mnCloudPipelinesMaxNumber;
    mpCloudPipelines = (SCloudPipeline *) DDBG_MALLOC(request_memsize, "E3CP");
    if (DLikely(mpCloudPipelines)) {
        memset(mpCloudPipelines, 0x0, request_memsize);
    } else {
        mnCloudPipelinesMaxNumber = 0;
        LOG_FATAL("no memory, request %d\n", request_memsize);
        return EECode_NoMemory;
    }

    request_memsize = sizeof(SNativeCapturePipeline) * mnNativeCapturePipelinesMaxNumber;
    mpNativeCapturePipelines = (SNativeCapturePipeline *) DDBG_MALLOC(request_memsize, "E3NC");
    if (DLikely(mpNativeCapturePipelines)) {
        memset(mpNativeCapturePipelines, 0x0, request_memsize);
    } else {
        mnNativeCapturePipelinesMaxNumber = 0;
        LOG_FATAL("no memory, request %d\n", request_memsize);
        return EECode_NoMemory;
    }

    request_memsize = sizeof(SNativePushPipeline) * mnNativePushPipelinesMaxNumber;
    mpNativePushPipelines = (SNativePushPipeline *) DDBG_MALLOC(request_memsize, "E3NP");
    if (DLikely(mpNativePushPipelines)) {
        memset(mpNativePushPipelines, 0x0, request_memsize);
    } else {
        mnNativePushPipelinesMaxNumber = 0;
        LOG_FATAL("no memory, request %d\n", request_memsize);
        return EECode_NoMemory;
    }

    request_memsize = sizeof(SCloudUploadPipeline) * mnCloudUploadPipelinesMaxNumber;
    mpCloudUploadPipelines = (SCloudUploadPipeline *)DDBG_MALLOC(request_memsize, "E3CU");
    if (DLikely(mpCloudUploadPipelines)) {
        memset(mpCloudUploadPipelines, 0x0, request_memsize);
    } else {
        mnCloudUploadPipelinesMaxNumber = 0;
        LOG_FATAL("no memory, request %d\n", request_memsize);
        return EECode_NoMemory;
    }

    request_memsize = sizeof(SVODPipeline) * mnVODPipelinesMaxNumber;
    mpVODPipelines = (SVODPipeline *)DDBG_MALLOC(request_memsize, "E3VP");
    if (DLikely(mpVODPipelines)) {
        memset(mpVODPipelines, 0x0, request_memsize);
    } else {
        mnVODPipelinesMaxNumber = 0;
        LOG_FATAL("no memory, request %d\n", request_memsize);
        return EECode_NoMemory;
    }

    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOGM_FATAL("gfCreateMutex() fail\n");
        return EECode_NoMemory;
    }

    if (p_shared_component && p_shared_component->p_clock_manager && p_shared_component->p_clock_source) {
        mbExternalClock = 1;
        mpClockManager = p_shared_component->p_clock_manager;
        mpClockSource = p_shared_component->p_clock_source;
        if (NULL == mpClockManager->GetClockSource()) {
            LOGM_WARN("not set clock source?\n");
            ret = mpClockManager->SetClockSource(mpClockSource);
            DASSERT_OK(ret);
            ret = mpClockManager->Start();
            DASSERT_OK(ret);
        }
    } else {
        mpClockManager = CIClockManager::Create();
        if (DUnlikely(!mpClockManager)) {
            LOGM_FATAL("CIClockManager::Create() fail\n");
            return EECode_NoMemory;
        }

        mpClockSource = gfCreateClockSource(EClockSourceType_generic);
        if (DUnlikely(!mpClockSource)) {
            LOGM_FATAL("gfCreateClockSource() fail\n");
            return EECode_NoMemory;
        }

        ret = mpClockManager->SetClockSource(mpClockSource);
        DASSERT_OK(ret);

        ret = mpClockManager->Start();
        DASSERT_OK(ret);
    }

    mpSystemClockReference = CIClockReference::Create();
    if (DUnlikely(!mpSystemClockReference)) {
        LOGM_FATAL("CIClockReference::Create() fail\n");
        return EECode_NoMemory;
    }

    ret = mpClockManager->RegisterClockReference(mpSystemClockReference);
    DASSERT_OK(ret);

    mPersistMediaConfig.p_clock_source = mpClockSource;
    mPersistMediaConfig.p_system_clock_manager = mpClockManager;
    mPersistMediaConfig.p_system_clock_reference = mpSystemClockReference;

    DASSERT(mpWorkQ);
    mpWorkQ->Run();

    mpStreamingContent = (SStreamingSessionContent *)DDBG_MALLOC((mnStreamingPipelinesMaxNumber + mnVODPipelinesMaxNumber) * sizeof(SStreamingSessionContent), "E3SC");
    if (DLikely(mpStreamingContent)) {
        memset(mpStreamingContent, 0x0, (mnStreamingPipelinesMaxNumber + mnVODPipelinesMaxNumber) * sizeof(SStreamingSessionContent));
    } else {
        LOGM_FATAL("No Memory\n");
        return EECode_NoMemory;
    }
    mpVodContent = mpStreamingContent + mnStreamingPipelinesMaxNumber;

    mpCloudContent = (SCloudContent *)DDBG_MALLOC(mnComponentFiltersMaxNumbers[EGenericComponentType_CloudConnecterServerAgent] * sizeof(SCloudContent), "E3CC");
    if (DLikely(mpCloudContent)) {
        memset(mpCloudContent, 0x0, mnComponentFiltersMaxNumbers[EGenericComponentType_CloudConnecterServerAgent] * sizeof(SCloudContent));
    } else {
        LOGM_FATAL("No Memory\n");
        return EECode_NoMemory;
    }

    mpCloudCmdContent = (SCloudContent *)DDBG_MALLOC(mnComponentFiltersMaxNumbers[EGenericComponentType_CloudConnecterCmdAgent] * sizeof(SCloudContent), "E3cc");
    if (DLikely(mpCloudCmdContent)) {
        memset(mpCloudCmdContent, 0x0, mnComponentFiltersMaxNumbers[EGenericComponentType_CloudConnecterCmdAgent] * sizeof(SCloudContent));
    } else {
        LOGM_FATAL("No Memory\n");
        return EECode_NoMemory;
    }

    mDailyCheckTime = mpSystemClockReference->GetCurrentTime();
    mDailyCheckTime += (TTime)24 * 60 * 60 * 1000000;
    mpDailyCheckTimer = mpSystemClockReference->AddTimer(this, EClockTimerType_once, mDailyCheckTime, (TTime)24 * 60 * 60 * 1000000, 1);
    mpDailyCheckTimer->user_context = (TULong)&mDailyCheckContext;
    //mpStorageManager = gfCreateStorageManagement(EStorageMnagementType_Simple);
    //if (DUnlikely(!mpStorageManager)) {
    //LOGM_FATAL("create storage management fail!\n");
    //return EECode_NoMemory;
    //}
    mPersistMediaConfig.p_storage_manager = mpStorageManager;

    mpExtraDataParser = gfFilterFactory(EFilterType_Demuxer, (const SPersistMediaConfig *)&mPersistMediaConfig, (IMsgSink *)this, 0xffff, mPersistMediaConfig.engine_config.filter_version);
    if (DUnlikely(mpExtraDataParser == NULL)) {
        LOGM_FATAL("create extra data parser fail\n");
        return EECode_NoMemory;
    }
    TUint index, sub_index;
    mpExtraDataParser->AddOutputPin(index, sub_index, StreamType_Video);
    mpExtraDataParser->AddOutputPin(index, sub_index, StreamType_Audio);

    mPersistMediaConfig.p_engine_v3 = (IGenericEngineControlV3 *) this;

    mpSimpleHashTable = CSimpleHashTable::Create(CSimpleHashTable::EHashTableKeyType_String, 20);//hardcode
    if (DUnlikely(!mpSimpleHashTable)) {
        LOGM_FATAL("CSimpleHashTable::Create fail\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CActiveGenericEngineV3::~CActiveGenericEngineV3()
{
    if (mbSelfLogFile) {
        gfCloseLogFile();
    }

    LOGM_RESOURCE("CActiveGenericEngineV3::~CActiveGenericEngineV3 exit.\n");
}

void CActiveGenericEngineV3::Delete()
{
    LOGM_RESOURCE("CActiveGenericEngineV3::Delete, before mpWorkQ->SendCmd(CMD_STOP).\n");

    if (mpWorkQ) {
        LOGM_INFO("before send stop.\n");
        mpWorkQ->SendCmd(ECMDType_ExitRunning);
        LOGM_INFO("after send stop.\n");
    }

    LOGM_RESOURCE("CActiveGenericEngineV3::Delete, before delete mpClockManager %p.\n", mpClockManager);

    if (DLikely(mPersistMediaConfig.p_remote_log_server)) {
        mPersistMediaConfig.p_remote_log_server->DestroyContext();
        mPersistMediaConfig.p_remote_log_server->Destroy();
        mPersistMediaConfig.p_remote_log_server = NULL;
    }

    if (mpSystemClockReference) {
        mpSystemClockReference->Delete();
        mpSystemClockReference = NULL;
    }

    if (!mbExternalClock) {
        if (mpClockManager) {
            mpClockManager->Delete();
            mpClockManager = NULL;
        }

        if (mpClockSource) {
            mpClockSource->GetObject0()->Delete();
            mpClockSource = NULL;
        }
    }

    LOGM_RESOURCE("CActiveGenericEngineV3::Delete, before StopServerManager.\n");

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

    LOGM_RESOURCE("CActiveGenericEngineV3::~CActiveGenericEngineV3 before clearGraph().\n");
    clearGraph();
    LOGM_RESOURCE("CActiveGenericEngineV3::~CActiveGenericEngineV3 before destroySchedulers().\n");
    destroySchedulers();

    LOGM_RESOURCE("CActiveGenericEngineV3::Delete, before inherited::Delete().\n");

    if (mpStreamingContent) {
        DDBG_FREE(mpStreamingContent, "E3SC");
        mpStreamingContent = NULL;
        mpVodContent = NULL;
    }

    if (mpCloudContent) {
        DDBG_FREE(mpCloudContent, "E3CC");
        mpCloudContent = NULL;
    }

    if (mpCloudCmdContent) {
        DDBG_FREE(mpCloudCmdContent, "E3cc");
        mpCloudCmdContent = NULL;
    }

    if (mpPlaybackPipelines) {
        DDBG_FREE(mpPlaybackPipelines, "E3PP");
        mpPlaybackPipelines = NULL;
    }

    if (mpRecordingPipelines) {
        DDBG_FREE(mpRecordingPipelines, "E3RP");
        mpRecordingPipelines = NULL;
    }

    if (mpStreamingPipelines) {
        DDBG_FREE(mpStreamingPipelines, "E3SP");
        mpStreamingPipelines = NULL;
    }

    if (mpCloudPipelines) {
        DDBG_FREE(mpCloudPipelines, "E3CP");
        mpCloudPipelines = NULL;
    }

    if (mpNativeCapturePipelines) {
        DDBG_FREE(mpNativeCapturePipelines, "E3NC");
        mpNativeCapturePipelines = NULL;
    }

    if (mpNativePushPipelines) {
        DDBG_FREE(mpNativePushPipelines, "E3NP");
        mpNativePushPipelines = NULL;
    }

    if (mpCloudUploadPipelines) {
        DDBG_FREE(mpCloudUploadPipelines, "E3CU");
        mpCloudUploadPipelines = NULL;
    }

    if (mpVODPipelines) {
        DDBG_FREE(mpVODPipelines, "E3VP");
        mpVODPipelines = NULL;
    }

#ifdef BUILD_CONFIG_WITH_LICENCE
    if (mpLicenceClient) {
        mpLicenceClient->Destroy();
        mpLicenceClient = NULL;
    }
#endif

    if (mpStorageManager) {
        //mpStorageManager->Delete();
        mpStorageManager = NULL;
    }

    if (mpExtraDataParser) {
        mpExtraDataParser->GetObject0()->Delete();
        mpExtraDataParser = NULL;
    }

    if (mpSimpleHashTable) {
        mpSimpleHashTable->Delete();
        mpSimpleHashTable = NULL;
    }

    if (mpListFilters) {
        delete mpListFilters;
        mpListFilters = NULL;
    }

    if (mpListProxys) {
        delete mpListProxys;
        mpListProxys = NULL;
    }

    if (mpListConnections) {
        delete mpListConnections;
        mpListConnections = NULL;
    }

    if (mpStreamingServerList) {
        delete mpStreamingServerList;
        mpStreamingServerList = NULL;
    }

    if (mpCloudServerList) {
        delete mpCloudServerList;
        mpCloudServerList = NULL;
    }

    inherited::Delete();

    LOGM_RESOURCE("CActiveGenericEngineV3::Delete, exit.\n");
}

EECode CActiveGenericEngineV3::PostEngineMsg(SMSG &msg)
{
    return inherited::PostEngineMsg(msg);
}

EECode CActiveGenericEngineV3::InitializeDevice()
{
    EECode err;
    //prepare things here
    DASSERT((-1) == mIavFd);
    DASSERT(NULL == mpDSPAPI);

    LOGM_INFO("[API flow]: InitializeDevice() start.\n");

    if (DUnlikely(mbDeviceOpened)) {
        DASSERT(0 < mIavFd);
        DASSERT(mpDSPAPI);
        LOGM_ERROR("Aleady initialized?\n");
        return EECode_OK;
    }

    if (DLikely(!mpDSPAPI)) {
#ifndef BUILD_OS_WINDOWS
        mpDSPAPI = gfDSPAPIFactory((const SPersistMediaConfig *)&mPersistMediaConfig);
#endif
    } else {
        LOGM_FATAL("Why mpDSPAPI(%p) is not NULL\n", mpDSPAPI);
        return EECode_OSError;
    }

    if (DUnlikely(!mpDSPAPI)) {
        LOGM_ERROR("CreateSimpleDSPAPI() fail\n");
        return EECode_Error;
    }

    err = mpDSPAPI->OpenDevice(mIavFd);
    if (DUnlikely((EECode_OK != err) || (0 > mIavFd))) {
        LOGM_ERROR("OpenDevice() fail, ret %d, %s, fd %d\n", err, gfGetErrorCodeString(err), mIavFd);
        return err;
    }

    err = mpDSPAPI->QueryVoutSettings(&mPersistMediaConfig.dsp_config);
    DASSERT_OK(err);

    mPersistMediaConfig.dsp_config.device_fd = mIavFd;
    mPersistMediaConfig.dsp_config.p_dsp_handler = (void *)mpDSPAPI;
    mPersistMediaConfig.dsp_config.dsp_type = mpDSPAPI->QueryDSPPlatform();

    LOGM_INFO("[API flow]: InitializeDevice() done.\n");
    mbDeviceOpened = 1;
    return EECode_OK;
}

EECode CActiveGenericEngineV3::ActivateDevice(TUint mode)
{
    EECode err = EECode_OK;

    //debug assert
    DASSERT(mpDSPAPI);
    DASSERT(0 <= mIavFd);
    DASSERT(1 == mbSetMediaConfig);
    DASSERT(1 == mbDeviceOpened);

    LOGM_INFO("[API flow]: ActivateDevice(%d) start.\n", mode);

    if (DUnlikely((mbDeviceActivated) || (!mbDeviceOpened))) {
        LOGM_ERROR("mbDeviceActivated %d, or mbDeviceOpened %d\n", mbDeviceActivated, mbDeviceOpened);
        return EECode_BadState;
    }

    DASSERT((EDSPOperationMode_MultipleWindowUDEC == mode) || (EDSPOperationMode_MultipleWindowUDEC_Transcode == mode) || (EDSPOperationMode_UDEC == mode));

    if (DLikely(mpDSPAPI)) {
        err = mpDSPAPI->DSPModeConfig((const SPersistMediaConfig *)&mPersistMediaConfig);
        err = mpDSPAPI->EnterDSPMode(mode);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("mpDSPAPI->EnterDSPMode(%d) fail, return %d, %s\n", mode, err, gfGetErrorCodeString(err));
            return err;
        }
    } else {
        LOGM_ERROR("NULL mpDSPAPI\n");
        return EECode_InternalLogicalBug;
    }

    mbDeviceActivated = 1;
    mPersistMediaConfig.dsp_config.request_dsp_mode = mode;

    LOGM_INFO("[API flow]: ActivateDevice(%d) done.\n", mode);
    return EECode_OK;
}

EECode CActiveGenericEngineV3::DeActivateDevice()
{
    EECode err = EECode_OK;

    //debug assert
    DASSERT(mpDSPAPI);
    DASSERT(0 <= mIavFd);
    DASSERT(1 == mbDeviceOpened);

    LOGM_INFO("[API flow]: DeActivateDevice() start.\n");

    if (DUnlikely((!mbDeviceActivated) || (!mbDeviceOpened))) {
        LOGM_ERROR("mbDeviceActivated %d, or mbDeviceOpened %d\n", mbDeviceActivated, mbDeviceOpened);
        return EECode_BadState;
    }

    err = mpDSPAPI->EnterDSPMode(EDSPOperationMode_IDLE);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("mpDSPAPI->EnterDSPMode(EDSPOperationMode_IDLE) fail, return %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    mbDeviceActivated = 0;
    LOGM_INFO("[API flow]: DeActivateDevice() done.\n");
    return EECode_OK;
}

EECode CActiveGenericEngineV3::GetMediaConfig(volatile SPersistMediaConfig *&pconfig) const
{
    pconfig = (volatile SPersistMediaConfig *)&mPersistMediaConfig;
    return EECode_OK;
}

EECode CActiveGenericEngineV3::SetMediaConfig()
{
    mbSetMediaConfig = 1;
    return EECode_OK;
}

EECode CActiveGenericEngineV3::SetEngineLogLevel(ELogLevel level)
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

EECode CActiveGenericEngineV3::SetRuntimeLogConfig(TGenericID id, TU32 level, TU32 option, TU32 output, TU32 is_add)
{
    EECode err = EECode_OK;
    SCMD cmd;

    cmd.code = ECMDType_SetRuntimeLogConfig;
    cmd.res32_1 = id;
    cmd.res64_1 = ((TU64)level) | (((TU64)option) << 32) ;
    cmd.res64_2 = ((TU64)output) | (((TU64)is_add) << 32) ;
    err = mpWorkQ->PostMsg(cmd);
    return err;
}

EECode CActiveGenericEngineV3::PrintCurrentStatus(TGenericID id, TULong print_flag)
{
    EECode err = EECode_OK;
    SCMD cmd;

    cmd.code = ECMDType_PrintCurrentStatus;
    cmd.res32_1 = id;
    cmd.res64_1 = print_flag;
    err = mpWorkQ->PostMsg(cmd);

    return err;
}

EECode CActiveGenericEngineV3::SaveCurrentLogSetting(const TChar *logfile)
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

EECode CActiveGenericEngineV3::BeginConfigProcessPipeline(TUint customized_pipeline)
{
    DASSERT(EEngineState_not_alive == msState);
    DASSERT(0 == mbGraghBuilt);

    if (DUnlikely(mbGraghBuilt)) {
        LOGM_FATAL("mbGraghBuilt %d yet?\n", mbGraghBuilt);
        return EECode_BadState;
    }

    if (DUnlikely(EEngineState_not_alive != msState)) {
        LOGM_FATAL("Current engine status is %u, not EEngineState_not_alive\n", msState);
        return EECode_BadState;
    }
    return EECode_OK;
}

EECode CActiveGenericEngineV3::NewComponent(TUint component_type, TGenericID &component_id, const TChar *prefer_string)
{
    SComponent *component = NULL;
    EECode err = EECode_OK;

    if (DLikely((EGenericComponentType_TAG_filter_start <= component_type) && (EGenericComponentType_TAG_filter_end > component_type))) {

        if (isUniqueComponentCreated(component_type)) {
            LOGM_FATAL("Component is unique, must not comes here\n");
            return EECode_BadCommand;
        }

        DASSERT(mnComponentFiltersNumbers[component_type] < mnComponentFiltersMaxNumbers[component_type]);
        if (mnComponentFiltersNumbers[component_type] < mnComponentFiltersMaxNumbers[component_type]) {
            component_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(component_type, mnComponentFiltersNumbers[component_type]);

            component = newComponent((TComponentType)component_type, (TComponentIndex)mnComponentFiltersNumbers[component_type], prefer_string);
            if (component) {
                mpListFilters->InsertContent(NULL, (void *)component, 0);
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
                    err = addStreamingServer(component_id, StreamingServerType_RTSP, StreamingServerMode_Unicast);
                    if (DUnlikely(EECode_OK != err)) {
                        LOGM_ERROR("addStreamingServer fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                    } else {
                        component = newComponent((TComponentType)component_type, DCOMPONENT_INDEX_FROM_GENERIC_ID(component_id), prefer_string);
                        if (DLikely(component)) {
                            component->id = component_id;
                            component->type = DCOMPONENT_TYPE_FROM_GENERIC_ID(component_id);
                            component->index = DCOMPONENT_INDEX_FROM_GENERIC_ID(component_id);
                            mpListProxys->InsertContent(NULL, (void *)component, 0);
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
                        component = newComponent((TComponentType)component_type,  DCOMPONENT_INDEX_FROM_GENERIC_ID(component_id), prefer_string);
                        if (DLikely(component)) {
                            component->id = component_id;
                            component->type = DCOMPONENT_TYPE_FROM_GENERIC_ID(component_id);
                            component->index = DCOMPONENT_INDEX_FROM_GENERIC_ID(component_id);
                            mpListProxys->InsertContent(NULL, (void *)component, 0);
                        } else {
                            LOGM_ERROR("NO memory\n");
                            return EECode_NoMemory;
                        }
                    }
                }
                break;

            default:
                LOGM_ERROR("BAD component type %d, %s\n", component_type, gfGetComponentStringFromGenericComponentType(component_type));
                return EECode_BadParam;
                break;
        }

    }

    LOGM_PRINTF("[Gragh]: NewComponent(%s), id 0x%08x\n", gfGetComponentStringFromGenericComponentType((TComponentType)component_type), component_id);

    return err;
}

EECode CActiveGenericEngineV3::ConnectComponent(TGenericID &connection_id, TGenericID upper_component_id, TGenericID down_component_id, StreamType pin_type)
{
    SConnection *connection;
    SComponent *up_component = NULL, *down_component = NULL;

    TU8 up_type = DCOMPONENT_TYPE_FROM_GENERIC_ID(upper_component_id);
    TU8 down_type = DCOMPONENT_TYPE_FROM_GENERIC_ID(down_component_id);
    TComponentIndex up_index = DCOMPONENT_INDEX_FROM_GENERIC_ID(upper_component_id);
    TComponentIndex down_index = DCOMPONENT_INDEX_FROM_GENERIC_ID(down_component_id);

    if (DUnlikely(upper_component_id == down_component_id)) {
        LOGM_ERROR("upper_component_id == down_component_id? please check code\n");
        return EECode_BadCommand;
    }
    //DASSERT(up_type < EGenericComponentType_TAG_filter_end);
    //DASSERT(down_type < EGenericComponentType_TAG_filter_end);
    //DASSERT(up_index < mnComponentFiltersNumbers[up_type]);
    //DASSERT(down_index < mnComponentFiltersNumbers[down_type]);
    if (DUnlikely((up_type >= EGenericComponentType_TAG_filter_end) || (down_type >= EGenericComponentType_TAG_filter_end) || (up_index >= mnComponentFiltersNumbers[up_type]) || (down_index >= mnComponentFiltersNumbers[down_type]))) {
        LOGM_ERROR("BAD parameters up type %d, down type %d, up index %d, down index %d, mnComponentFiltersNumbers[up_type] %d, mnComponentFiltersNumbers[down_type] %d\n", up_type, down_type, up_index, down_index, mnComponentFiltersNumbers[up_type], mnComponentFiltersNumbers[down_type]);
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
        mpListConnections->InsertContent(NULL, (void *)connection, 0);
    } else {
        LOGM_ERROR("NO memory\n");
        return EECode_NoMemory;
    }

    connection_id = connection->connection_id;
    connection->pin_type = pin_type;

    LOGM_PRINTF("[Gragh]: ConnectComponent(%s[%d] to %s[%d]), pin type %d, connection id 0x%08x\n", gfGetComponentStringFromGenericComponentType((TComponentType)up_type), up_index, gfGetComponentStringFromGenericComponentType((TComponentType)down_type), down_index, pin_type, connection_id);

    return EECode_OK;
}

EECode CActiveGenericEngineV3::SetupPlaybackPipeline(TGenericID &playback_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID video_decoder_id, TGenericID audio_decoder_id, TGenericID video_renderer_id, TGenericID audio_renderer_id, TGenericID audio_pinmuxer_id, TU8 running_at_startup)
{
    if (DLikely(mpPlaybackPipelines)) {
        if (DLikely((mnPlaybackPipelinesCurrentNumber + 1) < mnPlaybackPipelinesMaxNumber)) {
            SPlaybackPipeline *p_pipeline = mpPlaybackPipelines + mnPlaybackPipelinesCurrentNumber;
            TUint video_path_enable = 0;
            TUint audio_path_enable = 0;

            if (video_source_id) {

                if (DUnlikely(0 == isComponentIDValid_3(video_source_id, EGenericComponentType_Demuxer, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_CloudConnecterClientAgent))) {
                    LOGM_ERROR("not valid video source id %08x\n", video_source_id);
                    return EECode_BadParam;
                }

                if (DUnlikely(0 == isComponentIDValid(video_decoder_id, EGenericComponentType_VideoDecoder))) {
                    LOGM_ERROR("not valid video decoder id %08x\n", video_decoder_id);
                    return EECode_BadParam;
                }

                if (DUnlikely(0 == isComponentIDValid_2(video_renderer_id, EGenericComponentType_VideoRenderer, EGenericComponentType_VideoOutput))) {
                    LOGM_ERROR("not valid video renderer/output id %08x\n", video_renderer_id);
                    return EECode_BadParam;
                }

                video_path_enable = 1;
            } else {
                DASSERT(!video_decoder_id);
                DASSERT(!video_renderer_id);
            }

            if (audio_source_id) {
                if (DUnlikely(0 == isComponentIDValid_4(audio_source_id, EGenericComponentType_Demuxer, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_CloudConnecterClientAgent, EGenericComponentType_AudioCapture))) {
                    LOGM_ERROR("not valid audio source id %08x\n", audio_source_id);
                    return EECode_BadParam;
                }

                if (audio_decoder_id) {
                    if (DUnlikely(0 == isComponentIDValid(audio_decoder_id, EGenericComponentType_AudioDecoder))) {
                        LOGM_ERROR("not valid audio decoder id %08x\n", audio_decoder_id);
                        return EECode_BadParam;
                    }
                } else {
                    LOGM_NOTICE("no audio decoder component, PCM raw mode?\n");
                }

                if (DUnlikely(0 == isComponentIDValid_2(audio_renderer_id, EGenericComponentType_AudioRenderer, EGenericComponentType_AudioOutput))) {
                    LOGM_ERROR("not valid audio renderer/output id %08x\n", audio_renderer_id);
                    return EECode_BadParam;
                }

                if (audio_pinmuxer_id) {
                    if (DUnlikely(0 == isComponentIDValid(audio_pinmuxer_id, EGenericComponentType_ConnecterPinMuxer))) {
                        LOGM_ERROR("not valid audio pin muxer id %08x\n", audio_pinmuxer_id);
                        return EECode_BadParam;
                    }
                }

                audio_path_enable = 1;
            } else {
                DASSERT(!audio_decoder_id);
                DASSERT(!audio_renderer_id);
                DASSERT(!audio_pinmuxer_id);
            }

            if (DLikely(video_path_enable || audio_path_enable)) {
                p_pipeline->pipeline_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Playback, mnPlaybackPipelinesCurrentNumber);
                p_pipeline->index = mnPlaybackPipelinesCurrentNumber;
                p_pipeline->type = EGenericPipelineType_Playback;
                p_pipeline->pipeline_state = EGenericPipelineState_not_inited;
                p_pipeline->config_running_at_startup = running_at_startup;

                if (video_path_enable) {
                    p_pipeline->video_source_id = video_source_id;
                    p_pipeline->video_decoder_id = video_decoder_id;
                    p_pipeline->video_renderer_id = video_renderer_id;

                    p_pipeline->video_enabled = 1;
                }

                if (audio_path_enable) {
                    p_pipeline->audio_source_id = audio_source_id;
                    p_pipeline->audio_decoder_id = audio_decoder_id;
                    p_pipeline->audio_renderer_id = audio_renderer_id;
                    p_pipeline->audio_pinmuxer_id = audio_pinmuxer_id;

                    p_pipeline->audio_enabled = 1;
                }

                p_pipeline->video_suspended = 1;
                p_pipeline->audio_suspended = 1;

                mnPlaybackPipelinesCurrentNumber ++;
                playback_pipeline_id = p_pipeline->pipeline_id;

            } else {
                LOGM_ERROR("video and audio path are both disabled?\n");
                return EECode_BadParam;
            }

        } else {
            LOGM_ERROR("too many playback pipeline %d, max value %d\n", mnPlaybackPipelinesCurrentNumber, mnPlaybackPipelinesMaxNumber);
            return EECode_BadParam;
        }
    } else {
        LOGM_ERROR("must not comes here, mpPlaybackPipelines %p\n", mpPlaybackPipelines);
        return EECode_InternalLogicalBug;
    }

    LOGM_PRINTF("[Gragh]: SetupPlaybackPipeline() done, id 0x%08x\n", playback_pipeline_id);

    return EECode_OK;
}

EECode CActiveGenericEngineV3::SetupRecordingPipeline(TGenericID &recording_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID sink_id, TU8 running_at_startup, TChar *p_channelname)
{
    if (DLikely(mpRecordingPipelines)) {
        if (DLikely((mnRecordingPipelinesCurrentNumber + 1) < mnRecordingPipelinesMaxNumber)) {
            SRecordingPipeline *p_pipeline = mpRecordingPipelines + mnRecordingPipelinesCurrentNumber;
            TUint video_path_enable = 0;
            TUint audio_path_enable = 0;

            if (DUnlikely(0 == isComponentIDValid(sink_id, EGenericComponentType_Muxer))) {
                LOGM_ERROR("not valid sink_id %08x\n", sink_id);
                return EECode_BadParam;
            }

            if (video_source_id) {
                if (DUnlikely(0 == isComponentIDValid_3(video_source_id, EGenericComponentType_Demuxer, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_CloudConnecterClientAgent))) {
                    LOGM_ERROR("not valid video source id %08x\n", video_source_id);
                    return EECode_BadParam;
                }
                video_path_enable = 1;
            }

            if (audio_source_id) {
                if (DUnlikely(0 == isComponentIDValid_3(audio_source_id, EGenericComponentType_Demuxer, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_CloudConnecterClientAgent))) {
                    LOGM_ERROR("not valid audio source id %08x\n", audio_source_id);
                    return EECode_BadParam;
                }
                audio_path_enable = 1;
            }

            LOGM_NOTICE("video_path_enable %u, audio_path_enable %u, name %s\n", video_path_enable, audio_path_enable, p_channelname);

            if (DLikely(video_path_enable || audio_path_enable)) {
                p_pipeline->pipeline_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Recording, mnRecordingPipelinesCurrentNumber);
                p_pipeline->index = mnRecordingPipelinesCurrentNumber;
                p_pipeline->type = EGenericPipelineType_Recording;
                p_pipeline->pipeline_state = EGenericPipelineState_not_inited;
                p_pipeline->config_running_at_startup = running_at_startup;
                DASSERT(running_at_startup);

                if (video_path_enable) {
                    p_pipeline->video_source_id[0] = video_source_id;
                    p_pipeline->video_source_number = 1;
                    p_pipeline->video_enabled = 1;
                }

                if (audio_path_enable) {
                    p_pipeline->audio_source_id[0] = audio_source_id;
                    p_pipeline->audio_source_number = 1;
                    p_pipeline->audio_enabled = 1;
                }

                p_pipeline->sink_id = sink_id;
                p_pipeline->channel_name = p_channelname;

                mnRecordingPipelinesCurrentNumber ++;
                recording_pipeline_id = p_pipeline->pipeline_id;
            } else {
                LOGM_ERROR("video and audio path are both disabled?\n");
                return EECode_BadParam;
            }

        } else {
            LOGM_ERROR("too many recording pipeline %d, max value %d\n", mnRecordingPipelinesCurrentNumber, mnRecordingPipelinesMaxNumber);
            return EECode_BadParam;
        }
    } else {
        LOGM_ERROR("must not comes here, mpRecordingPipelines %p\n", mpRecordingPipelines);
        return EECode_InternalLogicalBug;
    }

    LOGM_PRINTF("[Gragh]: SetupRecordingPipeline() done, id 0x%08x\n", recording_pipeline_id);

    return EECode_OK;
}

EECode CActiveGenericEngineV3::SetupStreamingPipeline(TGenericID &streaming_pipeline_id, TGenericID streaming_transmiter_id, TGenericID streaming_server_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID audio_pinmuxer_id, TU8 running_at_startup)
{
    if (DLikely(mpStreamingPipelines)) {
        if (DLikely((mnStreamingPipelinesCurrentNumber + 1) < mnStreamingPipelinesMaxNumber)) {
            SStreamingPipeline *p_pipeline = mpStreamingPipelines + mnStreamingPipelinesCurrentNumber;
            TUint video_path_enable = 0;
            TUint audio_path_enable = 0;

            if (DUnlikely(0 == isComponentIDValid(streaming_transmiter_id, EGenericComponentType_StreamingTransmiter))) {
                LOGM_ERROR("not valid streaming_transmiter_id %08x\n", streaming_transmiter_id);
                return EECode_BadParam;
            }

            if (DUnlikely(0 == isComponentIDValid(streaming_server_id, EGenericComponentType_StreamingServer))) {
                LOGM_ERROR("not valid streaming_server_id %08x\n", streaming_server_id);
                return EECode_BadParam;
            }

            if (video_source_id) {
                if (DUnlikely(0 == isComponentIDValid_4(video_source_id, EGenericComponentType_Demuxer, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_CloudConnecterClientAgent, EGenericComponentType_VideoEncoder))) {
                    LOGM_ERROR("not valid video source id %08x\n", video_source_id);
                    return EECode_BadParam;
                }
                video_path_enable = 1;
            }

            if (audio_source_id) {
                if (DUnlikely(0 == isComponentIDValid_3(audio_source_id, EGenericComponentType_Demuxer, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_CloudConnecterClientAgent))) {
                    LOGM_ERROR("not valid audio source id %08x\n", audio_source_id);
                    return EECode_BadParam;
                }
                if (audio_pinmuxer_id) {
                    if (DUnlikely(0 == isComponentIDValid(audio_pinmuxer_id, EGenericComponentType_ConnecterPinMuxer))) {
                        LOGM_ERROR("not valid audio pin muxer id %08x\n", audio_pinmuxer_id);
                        return EECode_BadParam;
                    }
                }
                audio_path_enable = 1;
            } else if (audio_pinmuxer_id) {
                if (DUnlikely(0 == isComponentIDValid(audio_pinmuxer_id, EGenericComponentType_ConnecterPinMuxer))) {
                    LOGM_ERROR("not valid audio pin muxer id %08x\n", audio_pinmuxer_id);
                    return EECode_BadParam;
                }
                audio_path_enable = 1;
            }

            if (DLikely(video_path_enable || audio_path_enable)) {
                p_pipeline->pipeline_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Streaming, mnStreamingPipelinesCurrentNumber);
                p_pipeline->index = mnStreamingPipelinesCurrentNumber;
                p_pipeline->type = EGenericPipelineType_Streaming;
                p_pipeline->pipeline_state = EGenericPipelineState_not_inited;
                p_pipeline->config_running_at_startup = running_at_startup;
                DASSERT(running_at_startup);

                if (video_path_enable) {
                    p_pipeline->video_source_id[0] = video_source_id;
                    p_pipeline->video_source_number = 1;
                    p_pipeline->video_enabled = 1;
                }

                if (audio_path_enable) {
                    p_pipeline->audio_source_id[0] = audio_source_id;
                    p_pipeline->audio_pinmuxer_id = audio_pinmuxer_id;
                    p_pipeline->audio_source_number = 1;
                    p_pipeline->audio_enabled = 1;
                }

                p_pipeline->data_transmiter_id = streaming_transmiter_id;
                p_pipeline->server_id = streaming_server_id;

                mnStreamingPipelinesCurrentNumber ++;
                streaming_pipeline_id = p_pipeline->pipeline_id;
            } else {
                LOGM_ERROR("video and audio path are both disabled?\n");
                return EECode_BadParam;
            }

        } else {
            LOGM_ERROR("too many streaming pipeline %d, max value %d\n", mnStreamingPipelinesCurrentNumber, mnStreamingPipelinesMaxNumber);
            return EECode_BadParam;
        }
    } else {
        LOGM_ERROR("must not comes here, mpStreamingPipelines %p\n", mpStreamingPipelines);
        return EECode_InternalLogicalBug;
    }

    LOGM_PRINTF("[Gragh]: SetupStreamingPipeline() done, id 0x%08x\n", streaming_pipeline_id);

    return EECode_OK;
}

EECode CActiveGenericEngineV3::SetupCloudPipeline(TGenericID &cloud_pipeline_id, TGenericID cloud_agent_id, TGenericID cloud_server_id, TGenericID video_sink_id, TGenericID audio_sink_id, TGenericID cmd_sink_id, TU8 running_at_startup)
{
    if (DLikely(mpCloudPipelines)) {
        if (DLikely((mnCloudPipelinesCurrentNumber + 1) < mnCloudPipelinesMaxNumber)) {
            SCloudPipeline *p_pipeline = mpCloudPipelines + mnCloudPipelinesCurrentNumber;
            TUint video_path_enable = 0;
            TUint audio_path_enable = 0;

            if (DUnlikely(0 == isComponentIDValid_2(cloud_agent_id, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_CloudConnecterCmdAgent))) {
                LOGM_ERROR("not valid cloud_agent_id %08x\n", cloud_agent_id);
                return EECode_BadParam;
            }

            if (DUnlikely(0 == isComponentIDValid(cloud_server_id, EGenericComponentType_CloudServer))) {
                LOGM_ERROR("not valid cloud_server_id %08x\n", cloud_server_id);
                return EECode_BadParam;
            }

            if (video_sink_id) {
                if (DUnlikely(0 == isComponentIDValid_4(video_sink_id, EGenericComponentType_Muxer, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_VideoDecoder, EGenericComponentType_StreamingTransmiter))) {
                    LOGM_ERROR("not valid video_sink_id %08x\n", video_sink_id);
                    return EECode_BadParam;
                }
                video_path_enable = 1;
            }

            if (audio_sink_id) {
                if (DUnlikely(0 == isComponentIDValid_4(audio_sink_id, EGenericComponentType_Muxer, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_AudioDecoder, EGenericComponentType_StreamingTransmiter))) {
                    LOGM_ERROR("not valid audio_sink_id %08x\n", audio_sink_id);
                    return EECode_BadParam;
                }
                audio_path_enable = 1;
            }

            if (video_path_enable || audio_path_enable) {
                LOGM_NOTICE("[API flow]: no upload audio video path, should be cmd channel\n");
            } else {
                LOGM_NOTICE("[API flow]: audio_path_enable %d, video_path_enable %d\n", audio_path_enable, video_path_enable);
            }

            p_pipeline->pipeline_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Cloud, mnCloudPipelinesCurrentNumber);
            p_pipeline->index = mnCloudPipelinesCurrentNumber;
            p_pipeline->type = EGenericPipelineType_Cloud;
            p_pipeline->pipeline_state = EGenericPipelineState_not_inited;
            p_pipeline->config_running_at_startup = running_at_startup;
            //DASSERT(running_at_startup);

            if (video_path_enable) {
                p_pipeline->video_sink_id[0] = video_sink_id;
                p_pipeline->video_sink_number = 1;
                p_pipeline->video_enabled = 1;
            }

            if (audio_path_enable) {
                p_pipeline->audio_sink_id[0] = audio_sink_id;
                p_pipeline->audio_sink_number = 1;
                p_pipeline->audio_enabled = 1;
            }

            p_pipeline->agent_id = cloud_agent_id;
            p_pipeline->server_id = cloud_server_id;

            mnCloudPipelinesCurrentNumber ++;
            cloud_pipeline_id = p_pipeline->pipeline_id;

        } else {
            LOGM_ERROR("too many cloud pipeline %d, max value %d\n", mnCloudPipelinesCurrentNumber, mnCloudPipelinesMaxNumber);
            return EECode_BadParam;
        }
    } else {
        LOGM_ERROR("must not comes here, mpCloudPipelines %p\n", mpCloudPipelines);
        return EECode_InternalLogicalBug;
    }

    LOGM_PRINTF("[Gragh]: SetupCloudPipeline() done, id 0x%08x\n", cloud_pipeline_id);

    return EECode_OK;
}

EECode CActiveGenericEngineV3::SetupNativeCapturePipeline(TGenericID &capture_pipeline_id, TGenericID video_capture_source_id, TGenericID audio_capture_source_id, TGenericID video_encoder_id, TGenericID audio_encoder_id, TU8 running_at_startup)
{
    if (DLikely(mpNativeCapturePipelines)) {
        if (DLikely((mnNativeCapturePipelinesCurrentNumber + 1) < mnNativeCapturePipelinesMaxNumber)) {
            SNativeCapturePipeline *p_pipeline = mpNativeCapturePipelines + mnNativeCapturePipelinesCurrentNumber;
            TUint video_path_enable = 0;
            TUint audio_path_enable = 0;

            if (DLikely(0 == video_capture_source_id)) {
            } else {
                LOGM_FATAL("SetupNativeCapturePipeline...TODO\n");
            }

            if (DUnlikely(0 == audio_capture_source_id)) {
                LOGM_FATAL("SetupNativeCapturePipeline.. TODO\n");
                return EECode_BadParam;
            } else {
                if (DUnlikely(0 == isComponentIDValid(audio_capture_source_id, EGenericComponentType_AudioCapture))) {
                    LOGM_ERROR("not valid audio_capture_id %08x\n", audio_capture_source_id);
                    return EECode_BadParam;
                }
                audio_path_enable = 1;
            }

            if (DLikely(0 == video_encoder_id)) {
            } else {
                LOGM_FATAL("SetupNativeCapturePipeline: TODO\n");
            }

            if (DUnlikely(0 == audio_encoder_id)) {
            } else {
                DASSERT(audio_path_enable);
                if (DUnlikely(0 == isComponentIDValid(audio_encoder_id, EGenericComponentType_AudioEncoder))) {
                    LOGM_ERROR("not valid audio_encoder_id %08x\n", audio_encoder_id);
                    return EECode_BadParam;
                }
            }

            LOGM_NOTICE("[API flow]: audio_path_enable %d, video_path_enable %d, running_at_startup %d\n", audio_path_enable, video_path_enable, running_at_startup);

            p_pipeline->pipeline_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_NativeCapture, mnNativeCapturePipelinesCurrentNumber);
            p_pipeline->index = mnNativeCapturePipelinesCurrentNumber;
            p_pipeline->type = EGenericPipelineType_NativeCapture;
            p_pipeline->pipeline_state = EGenericPipelineState_not_inited;
            p_pipeline->config_running_at_startup = running_at_startup;

            if (video_path_enable) {
                p_pipeline->video_capture_id = video_capture_source_id;
                p_pipeline->video_encoder_id = video_encoder_id;
                p_pipeline->video_enabled = 1;
            }

            if (audio_path_enable) {
                p_pipeline->audio_capture_id = audio_capture_source_id;
                p_pipeline->audio_encoder_id = audio_encoder_id;
                p_pipeline->audio_enabled = 1;
            }

            mnNativeCapturePipelinesCurrentNumber ++;
            capture_pipeline_id = p_pipeline->pipeline_id;

        } else {
            LOGM_ERROR("too many native capture pipeline %d, max value %d\n", mnNativeCapturePipelinesCurrentNumber, mnNativeCapturePipelinesMaxNumber);
            return EECode_BadParam;
        }
    } else {
        LOGM_FATAL("NULL mpNativeCapturePipelines\n");
        return EECode_InternalLogicalBug;
    }

    LOGM_PRINTF("[Gragh]: SetupNativeCapturePipeline() done, id 0x%08x\n", capture_pipeline_id);

    return EECode_OK;
}

EECode CActiveGenericEngineV3::SetupNativePushPipeline(TGenericID &push_pipeline_id, TGenericID push_source_id, TGenericID video_sink_id, TGenericID audio_sink_id, TU8 running_at_startup)
{
    if (DLikely(mpNativePushPipelines)) {
        if (DLikely((mnNativePushPipelinesCurrentNumber + 1) < mnNativePushPipelinesMaxNumber)) {
            SNativePushPipeline *p_pipeline = mpNativePushPipelines + mnNativePushPipelinesCurrentNumber;
            TUint video_path_enable = 0;
            TUint audio_path_enable = 0;

            if (DLikely(push_source_id)) {
                if (DUnlikely(0 == isComponentIDValid_4(push_source_id, EGenericComponentType_VideoCapture, EGenericComponentType_AudioCapture, EGenericComponentType_VideoEncoder, EGenericComponentType_AudioEncoder))) {
                    LOGM_ERROR("not valid push_source_id %08x\n", push_source_id);
                    return EECode_BadParam;
                }
            } else {
                LOGM_ERROR("no push_source_id\n");
                return EECode_BadParam;
            }

            if (video_sink_id) {
                if (DUnlikely(0 == isComponentIDValid(video_sink_id, EGenericComponentType_CloudConnecterServerAgent))) {
                    LOGM_ERROR("not valid video_sink_id %08x\n", video_sink_id);
                    return EECode_BadParam;
                }
                video_path_enable = 1;
            }

            if (audio_sink_id) {
                if (DUnlikely(0 == isComponentIDValid(audio_sink_id, EGenericComponentType_CloudConnecterServerAgent))) {
                    LOGM_ERROR("not valid audio_sink_id %08x\n", audio_sink_id);
                    return EECode_BadParam;
                }
                audio_path_enable = 1;
            }

            LOGM_NOTICE("[API flow]: audio_path_enable %d, video_path_enable %d, running_at_startup %d\n", audio_path_enable, video_path_enable, running_at_startup);

            p_pipeline->pipeline_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_NativePush, mnNativePushPipelinesCurrentNumber);
            p_pipeline->index = mnNativePushPipelinesCurrentNumber;
            p_pipeline->type = EGenericPipelineType_NativePush;
            p_pipeline->pipeline_state = EGenericPipelineState_not_inited;
            p_pipeline->config_running_at_startup = running_at_startup;
            p_pipeline->push_source_id = push_source_id;

            if (video_path_enable) {
                p_pipeline->video_sink_id = video_sink_id;
                p_pipeline->video_sink_number = 1;
                p_pipeline->video_enabled = 1;
            }

            if (audio_path_enable) {
                p_pipeline->audio_sink_id = audio_sink_id;
                p_pipeline->audio_sink_number = 1;
                p_pipeline->audio_enabled = 1;
            }

            mnNativePushPipelinesCurrentNumber ++;
            push_pipeline_id = p_pipeline->pipeline_id;

        } else {
            LOGM_ERROR("too many native push pipeline %d, max value %d\n", mnNativePushPipelinesCurrentNumber, mnNativePushPipelinesMaxNumber);
            return EECode_BadParam;
        }
    } else {
        LOGM_FATAL("must not comes here, mpNativePushPipelines %p\n", mpNativePushPipelines);
        return EECode_InternalLogicalBug;
    }

    LOGM_PRINTF("[Gragh]: SetupNativePushPipeline() done, id 0x%08x\n", push_pipeline_id);

    return EECode_OK;
}

EECode CActiveGenericEngineV3::SetupCloudUploadPipeline(TGenericID &upload_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID uploader_id, TU8 running_at_startup)
{
    if (DLikely(mpCloudUploadPipelines)) {
        if (DLikely((mnCloudUploadPipelinesCurrentNumber + 1) < mnCloudUploadPipelinesMaxNumber)) {
            SCloudUploadPipeline *p_pipeline = mpCloudUploadPipelines + mnCloudUploadPipelinesCurrentNumber;

            if (DLikely(video_source_id)) {
                if (DUnlikely(0 == isComponentIDValid(video_source_id, EGenericComponentType_VideoEncoder))) {
                    LOGM_ERROR("not valid video_source_id %08x\n", video_source_id);
                    return EECode_BadParam;
                }
                p_pipeline->video_enabled = 1;
            } else {
                LOGM_NOTICE("no video_source_id\n");
            }

            if (DLikely(audio_source_id)) {
                if (DUnlikely(0 == isComponentIDValid(audio_source_id, EGenericComponentType_AudioEncoder))) {
                    LOGM_ERROR("not valid audio_source_id %08x\n", audio_source_id);
                    return EECode_BadParam;
                }
                p_pipeline->audio_enabled = 1;
            } else {
                LOGM_NOTICE("no audio_source_id\n");
            }

            if (DUnlikely(!video_source_id && !audio_source_id)) {
                LOGM_ERROR("no video or audio source\n");
                return EECode_BadParam;
            }

            if (DLikely(uploader_id)) {
                if (DUnlikely(0 == isComponentIDValid(uploader_id, EGenericComponentType_CloudConnecterClientAgent))) {
                    LOGM_ERROR("not valid uploader_id %08x\n", uploader_id);
                    return EECode_BadParam;
                }
            } else {
                LOGM_ERROR("no uploader\n");
                return EECode_BadParam;
            }

            p_pipeline->pipeline_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_CloudUpload, mnNativePushPipelinesCurrentNumber);
            p_pipeline->index = mnCloudUploadPipelinesCurrentNumber;
            p_pipeline->type = EGenericPipelineType_CloudUpload;
            p_pipeline->pipeline_state = EGenericPipelineState_not_inited;
            p_pipeline->config_running_at_startup = running_at_startup;
            p_pipeline->video_source_id = video_source_id;
            p_pipeline->audio_source_id = video_source_id;

            p_pipeline->uploader_id = uploader_id;
            upload_pipeline_id = p_pipeline->pipeline_id;

        } else {
            LOGM_ERROR("too many upload cloud pipeline %d, max value %d\n", mnCloudUploadPipelinesCurrentNumber, mnCloudUploadPipelinesMaxNumber);
            return EECode_BadParam;
        }
    }

    LOGM_PRINTF("[Gragh]: SetupCloudUploadPipeline() done, id 0x%08x\n", upload_pipeline_id);

    return EECode_OK;
}

EECode CActiveGenericEngineV3::SetupVODPipeline(TGenericID &vod_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID flow_controller_id, TGenericID streaming_transmiter_id, TGenericID streaming_server_id, TU8 running_at_startup)
{
    if (DLikely(mpVODPipelines)) {
        if (DLikely((mnVODPipelinesCurrentNumber + 1) < mnVODPipelinesMaxNumber)) {
            SVODPipeline *p_pipeline = mpVODPipelines + mnVODPipelinesCurrentNumber;
            TU8 enable_video = 0;
            TU8 enable_audio = 0;

            if (DUnlikely(0 == isComponentIDValid(streaming_transmiter_id, EGenericComponentType_StreamingTransmiter))) {
                LOGM_ERROR("not valid streaming_transmiter_id %08x\n", streaming_transmiter_id);
                return EECode_BadParam;
            }

            if (DUnlikely(0 == isComponentIDValid(streaming_server_id, EGenericComponentType_StreamingServer))) {
                LOGM_ERROR("not valid streaming_server_id %08x\n", streaming_server_id);
                return EECode_BadParam;
            }

            if (DLikely(video_source_id)) {
                if (DUnlikely(0 == isComponentIDValid(video_source_id, EGenericComponentType_Demuxer))) {
                    LOGM_ERROR("not valid demuxer id : 0x%x\n", video_source_id);
                    return EECode_BadParam;
                }
                enable_video = 1;
            }

            if (DLikely(audio_source_id)) {
                if (DUnlikely(0 == isComponentIDValid(audio_source_id, EGenericComponentType_Demuxer))) {
                    LOGM_ERROR("not valid demuxer id : 0x%x\n", audio_source_id);
                    return EECode_BadParam;
                }
                enable_audio = 1;
            }

            if (!enable_video && !enable_audio) {
                LOGM_ERROR("not valid source id\n");
                return EECode_BadParam;
            }

            if (DLikely(flow_controller_id)) {
                if (DUnlikely(0 == isComponentIDValid(flow_controller_id, EGenericComponentType_FlowController))) {
                    LOGM_ERROR("not valid flow controller id : 0x%x\n", flow_controller_id);
                    return EECode_BadParam;
                }
            }

            if (DLikely(enable_video || enable_audio)) {
                p_pipeline->pipeline_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_VOD, mnVODPipelinesCurrentNumber);
                p_pipeline->index = mnVODPipelinesCurrentNumber;
                p_pipeline->type = EGenericPipelineType_VOD;
                p_pipeline->pipeline_state = EGenericPipelineState_not_inited;
                p_pipeline->config_running_at_startup = running_at_startup;

                if (enable_video) {
                    p_pipeline->video_source_id[0] = video_source_id;
                    p_pipeline->video_enabled = enable_video;
                }

                if (enable_audio) {
                    p_pipeline->audio_source_id[0] = audio_source_id;
                    p_pipeline->audio_enabled = enable_audio;
                }
                p_pipeline->flow_controller_id = flow_controller_id;
                p_pipeline->data_transmiter_id = streaming_transmiter_id;
                p_pipeline->server_id = streaming_server_id;

                mnVODPipelinesCurrentNumber++;
                vod_pipeline_id = p_pipeline->pipeline_id;
            } else {
                LOGM_ERROR("video and audio path are both disabled?\n");
                return EECode_BadParam;
            }
        } else {
            LOGM_ERROR("too many vod pipeline %d, max value %d\n", mnVODPipelinesCurrentNumber, mnVODPipelinesMaxNumber);
            return EECode_BadParam;
        }
    }
    LOGM_PRINTF("[Gragh]: SetupVODPipeline() done, id 0x%08x\n", vod_pipeline_id);

    return EECode_OK;
}

EECode CActiveGenericEngineV3::FinalizeConfigProcessPipeline()
{
    EECode err = EECode_OK;

    if (DLikely(0 == mbGraghBuilt)) {

        DASSERT(EEngineState_not_alive == msState);

        err = createSchedulers();
        if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("createSchedulers() fail, err %d\n", err);
            return err;
        }

        err = createGraph();
        if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("createGraph() fail, err %d\n", err);
            return err;
        }

        msState = EEngineState_idle;
    } else {
        LOGM_FATAL("mbGraphBuilt?\n");
        return EECode_BadState;
    }

    msState = EEngineState_idle;
    return EECode_OK;
}

EECode CActiveGenericEngineV3::SetSourceUrl(TGenericID source_component_id, TChar *url)
{
    SComponent *p_component = findComponentFromID(source_component_id);
    if (DUnlikely(NULL == p_component)) {
        LOGM_ERROR("cannot find component, id %08x\n", source_component_id);
        return EECode_BadParam;
    }

    if (DLikely(NULL == p_component->p_filter)) {
        LOGM_ERROR("NULL filter? need check code\n");
        return EECode_InternalLogicalBug;
    }

    TMemSize url_length = 0;
    if (url) {
        url_length = strlen(url);

        if (DUnlikely(url_length >= 1024)) {
            LOGM_ERROR("url too long(size %ld) in SetSourceUrl, not support\n", url_length);
            return EECode_BadParam;
        }

        if (p_component->url && (p_component->url_buffer_size > url_length)) {
            memset(p_component->url, 0x0, p_component->url_buffer_size);
            strcpy(p_component->url, url);
        } else {
            if (p_component->url) {
                DDBG_FREE(p_component->url, "E3UR");
            }
            p_component->url = (TChar *)DDBG_MALLOC(url_length + 4, "E3UR");
            if (DLikely(p_component->url)) {
                p_component->url_buffer_size = url_length + 4;
            } else {
                LOGM_FATAL("no memory, request size %ld\n", url_length + 4);
                return EECode_NoMemory;
            }

            memset(p_component->url, 0x0, p_component->url_buffer_size);
            strcpy(p_component->url, url);
        }
    } else {
        if (p_component->url) {
            memset(p_component->url, 0x0, p_component->url_buffer_size);
        }
    }

    EECode err = EECode_OK;
    if (mbEnginePipelineRunning) {
        if (url) {
            LOGM_NOTICE("[API flow]: (component %s, %d, id 0x%02x) runtime set source url: %s\n", gfGetComponentStringFromGenericComponentType(DCOMPONENT_TYPE_FROM_GENERIC_ID(source_component_id)), DCOMPONENT_INDEX_FROM_GENERIC_ID(source_component_id), source_component_id, url);
        } else {
            LOGM_NOTICE("[API flow]: (component %s, %d, id 0x%02x) runtime set source NULL url\n", gfGetComponentStringFromGenericComponentType(DCOMPONENT_TYPE_FROM_GENERIC_ID(source_component_id)), DCOMPONENT_INDEX_FROM_GENERIC_ID(source_component_id), source_component_id);
        }
        err = p_component->p_filter->FilterProperty(EFilterPropertyType_update_source_url, 1, (void *) url);
        DASSERT_OK(err);
    } else {
        if (url) {
            LOGM_NOTICE("[API flow]: (component %s, %d, id 0x%02x) initial stage set source url: %s\n", gfGetComponentStringFromGenericComponentType(DCOMPONENT_TYPE_FROM_GENERIC_ID(source_component_id)), DCOMPONENT_INDEX_FROM_GENERIC_ID(source_component_id), source_component_id, url);
        } else {
            LOGM_NOTICE("[API flow]: (component %s, %d, id 0x%02x) initial stage set source NULL url\n", gfGetComponentStringFromGenericComponentType(DCOMPONENT_TYPE_FROM_GENERIC_ID(source_component_id)), DCOMPONENT_INDEX_FROM_GENERIC_ID(source_component_id), source_component_id);
        }
    }

    return err;
}

EECode CActiveGenericEngineV3::SetSinkUrl(TGenericID sink_component_id, TChar *url)
{
    SComponent *p_component = findComponentFromID(sink_component_id);
    if (DUnlikely(NULL == p_component)) {
        LOGM_ERROR("cannot find component, id %08x\n", sink_component_id);
        return EECode_BadParam;
    }

    if (DLikely(NULL == p_component->p_filter)) {
        LOGM_NOTICE("[API flow]: pre-set sink url\n");
    }

    TMemSize url_length = 0;
    if (url) {
        url_length = strlen(url);

        if (DUnlikely(url_length >= 1024)) {
            LOGM_ERROR("url too long(size %ld) in SetSinkUrl, not support\n", url_length);
            return EECode_BadParam;
        }

        if (p_component->url && (p_component->url_buffer_size > url_length)) {
            memset(p_component->url, 0x0, p_component->url_buffer_size);
            strcpy(p_component->url, url);
        } else {
            if (p_component->url) {
                DDBG_FREE(p_component->url, "E3UR");
            }
            p_component->url = (TChar *)DDBG_MALLOC(url_length + 4, "E3UR");
            if (DLikely(p_component->url)) {
                p_component->url_buffer_size = url_length + 4;
            } else {
                LOGM_FATAL("no memory, request size %ld\n", url_length + 4);
                return EECode_NoMemory;
            }

            memset(p_component->url, 0x0, p_component->url_buffer_size);
            strcpy(p_component->url, url);
        }
    } else {
        if (p_component->url) {
            memset(p_component->url, 0x0, p_component->url_buffer_size);
        }
    }

    EECode err = EECode_OK;
    if (mbEnginePipelineRunning) {
        if (url) {
            LOGM_NOTICE("[API flow]: (component %s, %d, id 0x%02x) runtime set sink url: %s\n", gfGetComponentStringFromGenericComponentType(DCOMPONENT_TYPE_FROM_GENERIC_ID(sink_component_id)), DCOMPONENT_INDEX_FROM_GENERIC_ID(sink_component_id), sink_component_id, url);
        } else {
            LOGM_NOTICE("[API flow]: (component %s, %d, id 0x%02x) runtime set sink NULL url\n", gfGetComponentStringFromGenericComponentType(DCOMPONENT_TYPE_FROM_GENERIC_ID(sink_component_id)), DCOMPONENT_INDEX_FROM_GENERIC_ID(sink_component_id), sink_component_id);
        }
        err = p_component->p_filter->FilterProperty(EFilterPropertyType_update_sink_url, 1, (void *) url);
        DASSERT_OK(err);
    } else {
        if (url) {
            LOGM_NOTICE("[API flow]: (component %s, %d, id 0x%02x) initial stage set sink url: %s\n", gfGetComponentStringFromGenericComponentType(DCOMPONENT_TYPE_FROM_GENERIC_ID(sink_component_id)), DCOMPONENT_INDEX_FROM_GENERIC_ID(sink_component_id), sink_component_id, url);
        } else {
            LOGM_NOTICE("[API flow]: (component %s, %d, id 0x%02x) initial stage set sink NULL url\n", gfGetComponentStringFromGenericComponentType(DCOMPONENT_TYPE_FROM_GENERIC_ID(sink_component_id)), DCOMPONENT_INDEX_FROM_GENERIC_ID(sink_component_id), sink_component_id);
        }
    }

    return err;
}

EECode CActiveGenericEngineV3::SetStreamingUrl(TGenericID pipeline_id, TChar *url)
{
    if (0 == isComponentIDValid(pipeline_id, EGenericPipelineType_Streaming)) {
        LOGM_ERROR("BAD pipeline id %08x\n", pipeline_id);
        return EECode_BadParam;
    }

    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(pipeline_id);
    if (DUnlikely(index >= mnStreamingPipelinesCurrentNumber)) {
        LOGM_ERROR("BAD pipeline id %08x, index %d exceed max value %d\n", pipeline_id, index, mnStreamingPipelinesCurrentNumber);
        return EECode_BadParam;
    }

    if (DUnlikely(NULL == mpStreamingPipelines)) {
        LOGM_ERROR("mpStreamingPipelines is NULL?\n");
        return EECode_BadState;
    }

    SStreamingPipeline *p_pipeline = mpStreamingPipelines + index;

    TMemSize url_length = 0;
    if (url) {
        url_length = strlen(url);

        if (DUnlikely(url_length >= 64)) {
            LOGM_ERROR("url too long(size %ld) in SetStreamingUrl, not support\n", url_length);
            return EECode_BadParam;
        }

        if (p_pipeline->url && (p_pipeline->url_buffer_size > url_length)) {
            memset(p_pipeline->url, 0x0, p_pipeline->url_buffer_size);
            snprintf(p_pipeline->url, p_pipeline->url_buffer_size, "%s", url);
        } else {
            if (p_pipeline->url) {
                DDBG_FREE(p_pipeline->url, "E3UR");
            }
            p_pipeline->url = (TChar *)DDBG_MALLOC(url_length + 4, "E3UR");
            if (DLikely(p_pipeline->url)) {
                p_pipeline->url_buffer_size = url_length + 4;
            } else {
                LOGM_FATAL("no memory, request size %ld\n", url_length + 4);
                return EECode_NoMemory;
            }

            memset(p_pipeline->url, 0x0, p_pipeline->url_buffer_size);
            snprintf(p_pipeline->url, p_pipeline->url_buffer_size, "%s", url);
        }
    } else {
        if (p_pipeline->url) {
            memset(p_pipeline->url, 0x0, p_pipeline->url_buffer_size);
        }
    }

    if (mbGraghBuilt) {
        if (DLikely(mpStreamingContent)) {
            SStreamingSessionContent *p_content = mpStreamingContent + index;
            memset(p_content->session_name, 0x0, sizeof(p_content->session_name));
            snprintf(p_content->session_name, sizeof(p_content->session_name) - 1, "%s", p_pipeline->url);
        } else {
            LOGM_FATAL("NULL mpStreamingContent here?\n");
            return EECode_BadState;
        }
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV3::SetVodUrl(TGenericID source_component_id, TGenericID pipeline_id, TChar *url)
{
    SComponent *p_component = findComponentFromID(source_component_id);
    if (DUnlikely(NULL == p_component)) {
        LOGM_ERROR("cannot find component, id %08x\n", source_component_id);
        return EECode_BadParam;
    }

    if (DLikely(NULL == p_component->p_filter)) {
        LOGM_ERROR("NULL filter? need check code\n");
        return EECode_InternalLogicalBug;
    }

    if (0 == isComponentIDValid(pipeline_id, EGenericPipelineType_VOD)) {
        LOGM_ERROR("BAD pipeline id %08x\n", pipeline_id);
        return EECode_BadParam;
    }

    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(pipeline_id);
    if (DUnlikely(index >= mnVODPipelinesCurrentNumber)) {
        LOGM_ERROR("BAD pipeline id %08x, index %d exceed max value %d\n", pipeline_id, index, mnVODPipelinesCurrentNumber);
        return EECode_BadParam;
    }

    if (DUnlikely(NULL == mpVODPipelines)) {
        LOGM_ERROR("mpStreamingPipelines is NULL?\n");
        return EECode_BadState;
    }

    SVODPipeline *p_pipeline = mpVODPipelines + index;
    TMemSize url_length = 0;
    if (url) {
        url_length = strlen(url);

        if (DUnlikely(url_length >= 64)) {
            LOGM_ERROR("url too long(size %ld) in SetStreamingUrl, not support\n", url_length);
            return EECode_BadParam;
        }

        if (p_pipeline->url && (p_pipeline->url_buffer_size > url_length)) {
            memset(p_pipeline->url, 0x0, p_pipeline->url_buffer_size);
            snprintf(p_pipeline->url, p_pipeline->url_buffer_size, "%s", url);
        } else {
            if (p_pipeline->url) {
                DDBG_FREE(p_pipeline->url, "E3UR");
            }
            p_pipeline->url = (TChar *)DDBG_MALLOC(url_length + 4, "E3UR");
            if (DLikely(p_pipeline->url)) {
                p_pipeline->url_buffer_size = url_length + 4;
            } else {
                LOGM_FATAL("no memory, request size %ld\n", url_length + 4);
                return EECode_NoMemory;
            }

            memset(p_pipeline->url, 0x0, p_pipeline->url_buffer_size);
            snprintf(p_pipeline->url, p_pipeline->url_buffer_size, "%s", url);
        }

        if (p_component->url && (p_component->url_buffer_size > url_length)) {
            memset(p_component->url, 0x0, p_component->url_buffer_size);
            snprintf(p_component->url, p_component->url_buffer_size, "%s", url);
        } else {
            if (p_component->url) {
                DDBG_FREE(p_component->url, "E3UR");
            }
            p_component->url = (TChar *)DDBG_MALLOC(url_length + 4, "E3UR");
            if (DLikely(p_component->url)) {
                p_component->url_buffer_size = url_length + 4;
            } else {
                LOGM_FATAL("no memory, request size %ld\n", url_length + 4);
                return EECode_NoMemory;
            }

            memset(p_component->url, 0x0, p_component->url_buffer_size);
            snprintf(p_component->url, p_component->url_buffer_size, "%s", url);
        }
    } else {
        if (p_pipeline->url) {
            memset(p_pipeline->url, 0x0, p_pipeline->url_buffer_size);
        }
    }

    if (mbGraghBuilt) {
        if (DLikely(mpVodContent)) {
            SStreamingSessionContent *p_content = mpVodContent + index;
            memset(p_content->session_name, 0x0, sizeof(p_content->session_name));
            snprintf(p_content->session_name, sizeof(p_content->session_name) - 1, "%s", p_pipeline->url);
        } else {
            LOGM_FATAL("NULL mpStreamingContent here?\n");
            return EECode_BadState;
        }
    }

    if (mbEnginePipelineRunning) {
        if (url) {
            EECode err = EECode_OK;
            err = p_component->p_filter->FilterProperty(EFilterPropertyType_demuxer_update_source_url, 1, (void *) url);
            DASSERT_OK(err);
        }
    }
    return EECode_OK;
}

EECode CActiveGenericEngineV3::SetCloudAgentTag(TGenericID agent_id, TChar *agent_tag, TGenericID server_id)
{
    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(agent_id);
    TComponentType type = DCOMPONENT_TYPE_FROM_GENERIC_ID(agent_id);

    if (1 == isComponentIDValid_2(agent_id, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_CloudConnecterCmdAgent)) {
        if (EGenericComponentType_CloudConnecterServerAgent == type) {
            if (DUnlikely(index >= mnComponentFiltersNumbers[EGenericComponentType_CloudConnecterServerAgent])) {
                LOGM_ERROR("BAD agent_id id %08x, index %d exceed max value %d\n", agent_id, index, mnComponentFiltersNumbers[EGenericComponentType_CloudConnecterServerAgent]);
                return EECode_BadParam;
            }
        } else {
            DASSERT(EGenericComponentType_CloudConnecterCmdAgent == type);
            if (DUnlikely(index >= mnComponentFiltersNumbers[EGenericComponentType_CloudConnecterCmdAgent])) {
                LOGM_ERROR("BAD agent_id id %08x, index %d exceed max value %d\n", agent_id, index, mnComponentFiltersNumbers[EGenericComponentType_CloudConnecterCmdAgent]);
                return EECode_BadParam;
            }
        }
    } else {
        LOGM_ERROR("BAD agent_id id %08x\n", agent_id);
        return EECode_BadParam;
    }

    SComponent *p_component = findComponentFromID(agent_id);
    if (NULL == p_component) {
        LOGM_ERROR("BAD agent_id id %08x, do not find related component\n", agent_id);
        return EECode_BadParam;
    }

    TMemSize url_length = 0;
    if (agent_tag) {
        url_length = strlen(agent_tag);

        if (DUnlikely(url_length >= 128)) {
            LOGM_ERROR("url too long(size %ld) in SetCloudAgentTag, not support\n", url_length);
            return EECode_BadParam;
        }

        if (p_component->url && (p_component->url_buffer_size > url_length)) {
            memset(p_component->url, 0x0, p_component->url_buffer_size);
            strcpy(p_component->url, agent_tag);
        } else {
            if (p_component->url) {
                DDBG_FREE(p_component->url, "E3UR");
            }
            p_component->url = (TChar *)DDBG_MALLOC(url_length + 4, "E3UR");
            if (DLikely(p_component->url)) {
                p_component->url_buffer_size = url_length + 4;
            } else {
                LOGM_FATAL("no memory, request size %ld\n", url_length + 4);
                return EECode_NoMemory;
            }

            memset(p_component->url, 0x0, p_component->url_buffer_size);
            strcpy(p_component->url, agent_tag);
        }
    } else {
        if (p_component->url) {
            memset(p_component->url, 0x0, p_component->url_buffer_size);
        }
    }

    SCloudContent *p_content = NULL;
    if (EGenericComponentType_CloudConnecterServerAgent == type) {
        p_content = mpCloudContent + index;
    } else {
        DASSERT(EGenericComponentType_CloudConnecterCmdAgent == type);
        p_content = mpCloudCmdContent + index;
    }

    if (DLikely(mpCloudContent)) {
        memset(p_content->content_name, 0x0, sizeof(p_content->content_name));
        snprintf(p_content->content_name, sizeof(p_content->content_name) - 1, "%s", p_component->url);
        LOGM_DEBUG("server_id 0x%08x\n", server_id);
        if (server_id) {
            if (DLikely(NULL == p_content->p_server)) {
                p_content->p_server = findCloudServer(server_id);
                LOGM_DEBUG("!!!!p_content %p, p_content->p_server %p, server_id 0x%08x\n", p_content, p_content->p_server, server_id);
            } else {
                DASSERT(p_content->p_server);
            }
        } else {
            LOGM_ERROR("server_id 0x%08x is not valid\n", server_id);
            DASSERT(p_content->p_server);
        }
    } else {
        LOGM_FATAL("NULL mpCloudContent here?\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV3::SetCloudClientTag(TGenericID agent_id, TChar *agent_tag, TChar *p_server_addr, TU16 server_port)
{
    if (!agent_tag || !p_server_addr) {
        LOGM_ERROR("NULL agent_tag %p, or NULL p_server_addr %p\n", agent_tag, p_server_addr);
        return EECode_BadParam;
    }

    if (0 == isComponentIDValid(agent_id, EGenericComponentType_CloudConnecterClientAgent)) {
        LOGM_ERROR("BAD agent_id id %08x\n", agent_id);
        return EECode_BadParam;
    }

    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(agent_id);
    if (DUnlikely(index >= mnComponentFiltersNumbers[EGenericComponentType_CloudConnecterClientAgent])) {
        LOGM_ERROR("BAD agent_id id %08x, index %d exceed max value %d\n", agent_id, index, mnComponentFiltersNumbers[EGenericComponentType_CloudConnecterClientAgent]);
        return EECode_BadParam;
    }

    SComponent *p_component = findComponentFromID(agent_id);
    if (NULL == p_component) {
        LOGM_ERROR("BAD agent_id id %08x, do not find related component\n", agent_id);
        return EECode_BadParam;
    }

    TMemSize url_length = 0;

    url_length = strlen(agent_tag);

    if (DUnlikely(url_length >= 128)) {
        LOGM_ERROR("url too long(size %ld) in SetCloudClientTag, not support\n", url_length);
        return EECode_BadParam;
    }

    if (p_component->url && (p_component->url_buffer_size > url_length)) {
        memset(p_component->url, 0x0, p_component->url_buffer_size);
        strcpy(p_component->url, agent_tag);
    } else {
        if (p_component->url) {
            DDBG_FREE(p_component->url, "E3UR");
        }
        p_component->url = (TChar *)DDBG_MALLOC(url_length + 4, "E3UR");
        if (DLikely(p_component->url)) {
            p_component->url_buffer_size = url_length + 4;
        } else {
            LOGM_FATAL("no memory, request size %ld\n", url_length + 4);
            return EECode_NoMemory;
        }

        memset(p_component->url, 0x0, p_component->url_buffer_size);
        strcpy(p_component->url, agent_tag);
    }

    url_length = strlen(p_server_addr);

    if (DUnlikely(url_length >= 128)) {
        LOGM_ERROR("url too long(size %ld) in SetCloudClientTag, not support\n", url_length);
        return EECode_BadParam;
    }

    if (p_component->remote_url && (p_component->remote_url_buffer_size > url_length)) {
        memset(p_component->remote_url, 0x0, p_component->remote_url_buffer_size);
        strcpy(p_component->remote_url, p_server_addr);
    } else {
        if (p_component->remote_url) {
            DDBG_FREE(p_component->remote_url, "E3UR");
        }
        p_component->remote_url = (TChar *)DDBG_MALLOC(url_length + 4, "E3UR");
        if (DLikely(p_component->remote_url)) {
            p_component->remote_url_buffer_size = url_length + 4;
        } else {
            LOGM_FATAL("no memory, request size %ld\n", url_length + 4);
            return EECode_NoMemory;
        }

        memset(p_component->remote_url, 0x0, p_component->remote_url_buffer_size);
        strcpy(p_component->remote_url, p_server_addr);
    }

    p_component->remote_port = server_port;

    return EECode_OK;
}

EECode CActiveGenericEngineV3::SetupVodContent(TUint channel, TChar *url, TU8 localfile, TU8 enable_video, TU8 enable_audio)
{
    LOGM_INFO("SetupVodContent IN\n");
    EECode err = EECode_OK;
    if (DUnlikely(channel > mnVODPipelinesMaxNumber)) {
        return EECode_BadParam;
    }
    DASSERT(url);
    DASSERT(mpExtraDataParser);

    SStreamingSessionContent *p_content = &mpVodContent[channel];
    TChar *pFilename = NULL;

    if (DLikely(url)) {
        memset(p_content->session_name, 0x0, DMaxStreamContentUrlLength);

        p_content->has_video = enable_video;
        p_content->has_audio = enable_audio;

        if (p_content->has_video && (NULL == p_content->sub_session_content[ESubSession_video])) {
            p_content->sub_session_content[ESubSession_video] = (SStreamingSubSessionContent *)DDBG_MALLOC(sizeof(SStreamingSubSessionContent), "ESSC");
            p_content->sub_session_content[ESubSession_video]->transmiter_input_pin_index = 0;
            p_content->sub_session_content[ESubSession_video]->type = StreamType_Video;
            p_content->sub_session_content[ESubSession_video]->format = StreamFormat_Invalid;
            p_content->sub_session_content[ESubSession_video]->parameters_setup = 0;
            p_content->sub_session_content[ESubSession_video]->enabled = 1;
        }

        if (p_content->has_audio && (NULL == p_content->sub_session_content[ESubSession_audio])) {
            p_content->sub_session_content[ESubSession_audio] = (SStreamingSubSessionContent *)DDBG_MALLOC(sizeof(SStreamingSubSessionContent), "ESSC");
            p_content->sub_session_content[ESubSession_audio]->transmiter_input_pin_index = 0;
            p_content->sub_session_content[ESubSession_audio]->type = StreamType_Audio;
            p_content->sub_session_content[ESubSession_audio]->format = StreamFormat_Invalid;
            p_content->sub_session_content[ESubSession_audio]->parameters_setup = 0;
            p_content->sub_session_content[ESubSession_audio]->enabled = 1;
        }

        if (DLikely(mpStreamingServerManager)) {
            //todo
            SVODPipeline *p_pipeline = mpVODPipelines;
            IStreamingServer *p_server = findStreamingServer(p_pipeline->server_id);
            mpStreamingServerManager->AddStreamingContent(p_server, p_content);
        }

        if (localfile) {
            TChar *pTmp = strrchr(url, '/');
            TChar *pVodUrl = (pTmp) ? (pTmp + 1) : url;
            snprintf(p_content->session_name, DMaxStreamContentUrlLength, "%s", pVodUrl);
            pFilename = url;
            p_content->content_type = 2; //todo
        } else {
            //request the first file of channel
            SDateTime /*requestTime, */ fileStartTime;
            TU16 fileDuration = 0;
            snprintf(p_content->session_name, DMaxStreamContentUrlLength, "%s", url);
            p_content->content_type = 1;
            //todo
            if (mpStorageManager->RequestExistUintSequentially(url, NULL, 0, pFilename, &fileStartTime, fileDuration) != EECode_OK) {
                LOGM_WARN("SetupVodContent, mpStorageManager->RequestExistUintSequentially fail! no file? channel name %s\n", url);
                return EECode_OK;
            }
            LOGM_WARN("SetupVodContent, mpStorageManager->RequestExistUintSequentially , file name: %s, channel name %s\n", pFilename, url);
        }

        //parse extra data
        if (pFilename) {
            err = parseExtraData(pFilename, p_content);
            if (err != EECode_OK) {
                LOGM_FATAL("parseExtraData fail!\n");
                return err;
            }
        }

        if (!localfile) {
            mpStorageManager->ReleaseExistUint(p_content->session_name, pFilename);
        }

    } else {
        LOGM_ERROR("SetupVodContent, url is NULL\n");
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV3::StartProcess()
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

EECode CActiveGenericEngineV3::StopProcess()
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

EECode CActiveGenericEngineV3::SetAppMsgCallback(void (*MsgProc)(void *, SMSG &), void *context)
{
    return inherited::SetAppMsgCallback(MsgProc, context);
}

EECode CActiveGenericEngineV3::SuspendPipeline(TGenericID pipeline_id, TUint release_content)
{
    TComponentType type = DCOMPONENT_TYPE_FROM_GENERIC_ID(pipeline_id);
    TComponentType index = DCOMPONENT_INDEX_FROM_GENERIC_ID(pipeline_id);

    LOGM_NOTICE("[Debug flow]: SuspendPipeline 0x%08x, %s begin\n", pipeline_id, gfGetComponentStringFromGenericComponentType(type));

    if (EGenericPipelineType_Playback == type) {
        if (DLikely(mpPlaybackPipelines && (mnPlaybackPipelinesCurrentNumber > index))) {
            SPlaybackPipeline *p_pipeline = mpPlaybackPipelines + index;

            if (EGenericPipelineState_running == p_pipeline->pipeline_state) {
                p_pipeline->pipeline_state = EGenericPipelineState_suspended;

                if (p_pipeline->p_video_source) {
                    deactivateComponent(p_pipeline->p_video_source);
                }

                if (p_pipeline->p_audio_source) {
                    deactivateComponent(p_pipeline->p_audio_source);
                }

                if (p_pipeline->p_video_decoder) {
                    deactivateComponent(p_pipeline->p_video_decoder);
                }

                if (p_pipeline->p_audio_decoder) {
                    deactivateComponent(p_pipeline->p_audio_decoder);
                }

                //if (p_pipeline->p_video_renderer) {
                //    deactivateComponent(p_pipeline->p_video_renderer);
                //}

                //if (p_pipeline->p_audio_renderer) {
                //    deactivateComponent(p_pipeline->p_audio_renderer);
                //}

                if (p_pipeline->p_audio_pinmuxer) {
                    deactivateComponent(p_pipeline->p_audio_pinmuxer);
                }

            } else {
                LOGM_WARN("pipeline 0x%08x, not in running state, %d\n", p_pipeline->pipeline_id, p_pipeline->pipeline_state);
                return EECode_BadState;
            }
        } else {
            LOG_ERROR("mpPlaybackPipelines %p, index %d, mnPlaybackPipelinesCurrentNumber %d\n", mpPlaybackPipelines, index, mnPlaybackPipelinesCurrentNumber);
            return EECode_BadState;
        }
    } else if (EGenericPipelineType_Recording == type) {
        if (DLikely(mpRecordingPipelines && (mnRecordingPipelinesCurrentNumber > index))) {
            SRecordingPipeline *p_pipeline = mpRecordingPipelines + index;

            if (EGenericPipelineState_running == p_pipeline->pipeline_state) {
                p_pipeline->pipeline_state = EGenericPipelineState_suspended;

                if (p_pipeline->p_sink) {
                    deactivateComponent(p_pipeline->p_sink);
                }

                if (p_pipeline->p_video_source[0]) {
                    deactivateComponent(p_pipeline->p_video_source[0]);
                }

                if (p_pipeline->p_audio_source[0]) {
                    deactivateComponent(p_pipeline->p_audio_source[0]);
                }

            } else {
                LOGM_WARN("pipeline 0x%08x, not in running state, %d\n", p_pipeline->pipeline_id, p_pipeline->pipeline_state);
                return EECode_BadState;
            }
        } else {
            LOG_ERROR("mpRecordingPipelines %p, index %d, mnRecordingPipelinesCurrentNumber %d\n", mpRecordingPipelines, index, mnRecordingPipelinesCurrentNumber);
            return EECode_BadState;
        }
    } else if (EGenericPipelineType_Streaming == type) {
        if (DLikely(mpStreamingPipelines && (mnStreamingPipelinesCurrentNumber > index))) {
            SStreamingPipeline *p_pipeline = mpStreamingPipelines + index;

            if (EGenericPipelineState_running == p_pipeline->pipeline_state) {
                p_pipeline->pipeline_state = EGenericPipelineState_suspended;

                if (p_pipeline->p_data_transmiter) {
                    deactivateComponent(p_pipeline->p_data_transmiter);
                }

                if (p_pipeline->p_video_source[0]) {
                    deactivateComponent(p_pipeline->p_video_source[0]);
                }

                if (p_pipeline->p_audio_source[0]) {
                    deactivateComponent(p_pipeline->p_audio_source[0]);
                }

                if (p_pipeline->p_audio_pinmuxer) {
                    deactivateComponent(p_pipeline->p_audio_pinmuxer);
                }

            } else {
                LOGM_WARN("pipeline 0x%08x, not in running state, %d\n", p_pipeline->pipeline_id, p_pipeline->pipeline_state);
                return EECode_BadState;
            }
        } else {
            LOG_ERROR("mpStreamingPipelines %p, index %d, mnStreamingPipelinesCurrentNumber %d\n", mpStreamingPipelines, index, mnStreamingPipelinesCurrentNumber);
            return EECode_BadState;
        }
    } else if (EGenericPipelineType_Cloud == type) {
        if (DLikely(mpCloudPipelines && (mnCloudPipelinesCurrentNumber > index))) {
            SCloudPipeline *p_pipeline = mpCloudPipelines + index;

            if (EGenericPipelineState_running == p_pipeline->pipeline_state) {
                p_pipeline->pipeline_state = EGenericPipelineState_suspended;

                if (p_pipeline->p_agent) {
                    deactivateComponent(p_pipeline->p_agent);
                }

                if (p_pipeline->p_video_sink[0]) {
                    deactivateComponent(p_pipeline->p_video_sink[0]);
                }

                if (p_pipeline->p_audio_sink[0]) {
                    deactivateComponent(p_pipeline->p_audio_sink[0]);
                }

            } else {
                LOGM_WARN("pipeline 0x%08x, not in running state, %d\n", p_pipeline->pipeline_id, p_pipeline->pipeline_state);
                return EECode_BadState;
            }
        } else {
            LOG_ERROR("mpCloudPipelines %p, index %d, mnCloudPipelinesCurrentNumber %d\n", mpCloudPipelines, index, mnCloudPipelinesCurrentNumber);
            return EECode_BadState;
        }
    } else if (EGenericPipelineType_NativeCapture == type) {
        if (DLikely(mpNativeCapturePipelines && (mnNativeCapturePipelinesCurrentNumber > index))) {
            SNativeCapturePipeline *p_pipeline = mpNativeCapturePipelines + index;

            if (EGenericPipelineState_running == p_pipeline->pipeline_state) {
                p_pipeline->pipeline_state = EGenericPipelineState_suspended;

                if (p_pipeline->p_video_capture) {
                    deactivateComponent(p_pipeline->p_video_capture);
                }

                if (p_pipeline->p_audio_capture) {
                    deactivateComponent(p_pipeline->p_audio_capture);
                }

                if (p_pipeline->p_video_encoder) {
                    deactivateComponent(p_pipeline->p_video_encoder);
                }

                if (p_pipeline->p_audio_encoder) {
                    deactivateComponent(p_pipeline->p_audio_encoder);
                }

            } else {
                LOGM_WARN("pipeline 0x%08x, not in running state, %d\n", p_pipeline->pipeline_id, p_pipeline->pipeline_state);
                return EECode_BadState;
            }
        } else {
            LOG_ERROR("mpNativeCapturePipelines %p, index %d, mnNativeCapturePipelinesCurrentNumber %d\n", mpNativeCapturePipelines, index, mnNativeCapturePipelinesCurrentNumber);
            return EECode_BadState;
        }
    } else if (EGenericPipelineType_NativePush == type) {
        if (DLikely(mpNativePushPipelines && (mnNativePushPipelinesCurrentNumber > index))) {
            SNativePushPipeline *p_pipeline = mpNativePushPipelines + index;

            if (EGenericPipelineState_running == p_pipeline->pipeline_state) {
                p_pipeline->pipeline_state = EGenericPipelineState_suspended;

                if (p_pipeline->p_push_source) {
                    deactivateComponent(p_pipeline->p_push_source);
                }

                if (p_pipeline->p_video_sink) {
                    deactivateComponent(p_pipeline->p_video_sink);
                }

                if (p_pipeline->p_audio_sink) {
                    deactivateComponent(p_pipeline->p_audio_sink);
                }

            } else {
                LOGM_WARN("pipeline 0x%08x, not in running state, %d\n", p_pipeline->pipeline_id, p_pipeline->pipeline_state);
                return EECode_BadState;
            }
        } else {
            LOG_ERROR("mpNativePushPipelines %p, index %d, mnNativePushPipelinesCurrentNumber %d\n", mpNativePushPipelines, index, mnNativePushPipelinesCurrentNumber);
            return EECode_BadState;
        }
    } else {
        LOGM_FATAL("BAD pipeline type %d, pipeline_id 0x%08x\n", type, pipeline_id);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV3::ResumePipeline(TGenericID pipeline_id, TU8 force_audio_path, TU8 force_video_path)
{
    TComponentType type = DCOMPONENT_TYPE_FROM_GENERIC_ID(pipeline_id);
    TComponentType index = DCOMPONENT_INDEX_FROM_GENERIC_ID(pipeline_id);

    LOGM_NOTICE("[Debug flow]: ResumePipeline 0x%08x, %s begin\n", pipeline_id, gfGetComponentStringFromGenericComponentType(type));

    if (EGenericPipelineType_Playback == type) {
        if (DLikely(mpPlaybackPipelines && (mnPlaybackPipelinesCurrentNumber > index))) {
            SPlaybackPipeline *p_pipeline = mpPlaybackPipelines + index;

            if (EGenericPipelineState_suspended == p_pipeline->pipeline_state) {
                p_pipeline->pipeline_state = EGenericPipelineState_running;

                if (p_pipeline->p_video_source) {
                    activateComponent(p_pipeline->p_video_source);
                }

                if (p_pipeline->p_audio_source) {
                    activateComponent(p_pipeline->p_audio_source);
                }

                if (p_pipeline->p_video_decoder) {
                    //if (force_video_path) {
                    DASSERT(p_pipeline->p_vd_input_connection);
                    activateComponent(p_pipeline->p_video_decoder, 1, p_pipeline->p_vd_input_connection->down_pin_index);
                    //}
                }

                if (p_pipeline->p_audio_decoder) {
                    if (force_audio_path && (!p_pipeline->p_audio_pinmuxer) && (!p_pipeline->audio_pinmuxer_id)) {
                        activateComponent(p_pipeline->p_audio_decoder, 1, p_pipeline->p_ad_input_connection->down_pin_index);
                    } else {
                        activateComponent(p_pipeline->p_audio_decoder);
                    }
                }

                //if (p_pipeline->p_video_renderer) {
                //    activateComponent(p_pipeline->p_video_renderer);
                //}

                //if (p_pipeline->p_audio_renderer) {
                //    activateComponent(p_pipeline->p_audio_renderer);
                //}

                if (p_pipeline->p_audio_pinmuxer) {
                    if (force_audio_path) {
                        activateComponent(p_pipeline->p_audio_pinmuxer, 1, p_pipeline->p_as_output_connection->down_pin_index);
                    } else {
                        activateComponent(p_pipeline->p_audio_pinmuxer);
                    }
                }

            } else {
                LOGM_WARN("pipeline 0x%08x, not in suspend state, %d\n", p_pipeline->pipeline_id, p_pipeline->pipeline_state);
                return EECode_BadState;
            }
        } else {
            LOG_ERROR("mpPlaybackPipelines %p, index %d, mnPlaybackPipelinesCurrentNumber %d\n", mpPlaybackPipelines, index, mnPlaybackPipelinesCurrentNumber);
            return EECode_BadState;
        }
    } else if (EGenericPipelineType_Recording == type) {
        if (DLikely(mpRecordingPipelines && (mnRecordingPipelinesCurrentNumber > index))) {
            SRecordingPipeline *p_pipeline = mpRecordingPipelines + index;

            if (EGenericPipelineState_suspended == p_pipeline->pipeline_state) {
                p_pipeline->pipeline_state = EGenericPipelineState_running;

                if (p_pipeline->p_sink) {
                    activateComponent(p_pipeline->p_sink);
                }

                if (p_pipeline->p_video_source[0]) {
                    activateComponent(p_pipeline->p_video_source[0]);
                }

                if (p_pipeline->p_audio_source[0]) {
                    activateComponent(p_pipeline->p_audio_source[0]);
                }

            } else {
                LOGM_WARN("pipeline 0x%08x, not in suspend state, %d\n", p_pipeline->pipeline_id, p_pipeline->pipeline_state);
                return EECode_BadState;
            }
        } else {
            LOG_ERROR("mpRecordingPipelines %p, index %d, mnRecordingPipelinesCurrentNumber %d\n", mpRecordingPipelines, index, mnRecordingPipelinesCurrentNumber);
            return EECode_BadState;
        }
    } else if (EGenericPipelineType_Streaming == type) {
        if (DLikely(mpStreamingPipelines && (mnStreamingPipelinesCurrentNumber > index))) {
            SStreamingPipeline *p_pipeline = mpStreamingPipelines + index;

            if (EGenericPipelineState_suspended == p_pipeline->pipeline_state) {
                p_pipeline->pipeline_state = EGenericPipelineState_running;

                if (p_pipeline->p_data_transmiter) {
                    activateComponent(p_pipeline->p_data_transmiter);
                }

                if (p_pipeline->p_video_source[0]) {
                    activateComponent(p_pipeline->p_video_source[0]);
                }

                if (p_pipeline->p_audio_source[0]) {
                    activateComponent(p_pipeline->p_audio_source[0]);
                }

                if (p_pipeline->p_audio_pinmuxer) {
                    if (force_audio_path) {
                        activateComponent(p_pipeline->p_audio_pinmuxer, 1, p_pipeline->p_audio_pinmuxer_connection[0]->down_pin_index);
                    } else {
                        activateComponent(p_pipeline->p_audio_pinmuxer);
                    }
                }

            } else {
                LOGM_WARN("pipeline 0x%08x, not in suspend state, %d\n", p_pipeline->pipeline_id, p_pipeline->pipeline_state);
                return EECode_BadState;
            }
        } else {
            LOG_ERROR("mpStreamingPipelines %p, index %d, mnStreamingPipelinesCurrentNumber %d\n", mpStreamingPipelines, index, mnStreamingPipelinesCurrentNumber);
            return EECode_BadState;
        }
    } else if (EGenericPipelineType_Cloud == type) {
        if (DLikely(mpCloudPipelines && (mnCloudPipelinesCurrentNumber > index))) {
            SCloudPipeline *p_pipeline = mpCloudPipelines + index;

            if (EGenericPipelineState_suspended == p_pipeline->pipeline_state) {
                p_pipeline->pipeline_state = EGenericPipelineState_running;

                if (p_pipeline->p_agent) {
                    activateComponent(p_pipeline->p_agent);
                }

                if (p_pipeline->p_video_sink[0]) {
                    if (force_video_path) {
                        activateComponent(p_pipeline->p_video_sink[0], 1, p_pipeline->p_video_sink_connection[0]->down_pin_index);
                    } else {
                        activateComponent(p_pipeline->p_video_sink[0]);
                    }
                }

                if (p_pipeline->p_audio_sink[0]) {
                    if (force_audio_path) {
                        activateComponent(p_pipeline->p_audio_sink[0], 1, p_pipeline->p_audio_sink_connection[0]->down_pin_index);
                    } else {
                        activateComponent(p_pipeline->p_audio_sink[0]);
                    }
                }

            } else {
                LOGM_WARN("pipeline 0x%08x, not in suspend state, %d\n", p_pipeline->pipeline_id, p_pipeline->pipeline_state);
                return EECode_BadState;
            }
        } else {
            LOG_ERROR("mpCloudPipelines %p, index %d, mnCloudPipelinesCurrentNumber %d\n", mpCloudPipelines, index, mnCloudPipelinesCurrentNumber);
            return EECode_BadState;
        }
    } else if (EGenericPipelineType_NativeCapture == type) {
        if (DLikely(mpNativeCapturePipelines && (mnNativeCapturePipelinesCurrentNumber > index))) {
            SNativeCapturePipeline *p_pipeline = mpNativeCapturePipelines + index;

            if (EGenericPipelineState_suspended == p_pipeline->pipeline_state) {
                p_pipeline->pipeline_state = EGenericPipelineState_running;

                if (p_pipeline->p_video_capture) {
                    activateComponent(p_pipeline->p_video_capture);
                }

                if (p_pipeline->p_audio_capture) {
                    activateComponent(p_pipeline->p_audio_capture);
                }

                if (p_pipeline->p_video_encoder) {
                    activateComponent(p_pipeline->p_video_encoder);
                }

                if (p_pipeline->p_audio_encoder) {
                    activateComponent(p_pipeline->p_audio_encoder);
                }
            } else {
                LOGM_WARN("pipeline 0x%08x, not in suspend state, %d\n", p_pipeline->pipeline_id, p_pipeline->pipeline_state);
                return EECode_BadState;
            }
        } else {
            LOG_ERROR("mpNativeCapturePipelines %p, index %d, mnNativeCapturePipelinesCurrentNumber %d\n", mpNativeCapturePipelines, index, mnNativeCapturePipelinesCurrentNumber);
            return EECode_BadState;
        }
    } else if (EGenericPipelineType_NativePush == type) {
        if (DLikely(mpNativePushPipelines && (mnNativePushPipelinesCurrentNumber > index))) {
            SNativePushPipeline *p_pipeline = mpNativePushPipelines + index;

            if (EGenericPipelineState_suspended == p_pipeline->pipeline_state) {
                p_pipeline->pipeline_state = EGenericPipelineState_running;

                if (p_pipeline->p_push_source) {
                    activateComponent(p_pipeline->p_push_source);
                }

                if (p_pipeline->p_video_sink) {
                    if (force_video_path) {
                        activateComponent(p_pipeline->p_video_sink, 1, p_pipeline->p_video_sink_connection->down_pin_index);
                    } else {
                        activateComponent(p_pipeline->p_video_sink);
                    }
                }

                if (p_pipeline->p_audio_sink) {
                    if (force_audio_path) {
                        activateComponent(p_pipeline->p_audio_sink, 1, p_pipeline->p_audio_sink_connection->down_pin_index);
                    } else {
                        activateComponent(p_pipeline->p_audio_sink);
                    }
                }

            } else {
                LOGM_WARN("pipeline 0x%08x, not in suspend state, %d\n", p_pipeline->pipeline_id, p_pipeline->pipeline_state);
                return EECode_BadState;
            }
        } else {
            LOG_ERROR("mpNativePushPipelines %p, index %d, mnNativePushPipelinesCurrentNumber %d\n", mpNativePushPipelines, index, mnNativePushPipelinesCurrentNumber);
            return EECode_BadState;
        }
    } else {
        LOGM_FATAL("BAD pipeline type %d, pipeline_id 0x%08x\n", type, pipeline_id);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV3::GenericPreConfigure(EGenericEnginePreConfigure config_type, void *param, TUint is_get)
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
                    LOGM_CONFIGURATION("EGenericEnginePreConfigure_NetworkReceiverSchedulerNumber %d\n", mPersistMediaConfig.number_scheduler_network_reciever);
                } else {
                    pre_config->number = mPersistMediaConfig.number_scheduler_network_reciever;
                }
            }
            break;

        case EGenericEnginePreConfigure_NetworkTCPReceiverSchedulerNumber: {
                SGenericPreConfigure *pre_config = (SGenericPreConfigure *) param;
                DASSERT(EGenericEnginePreConfigure_NetworkTCPReceiverSchedulerNumber == pre_config->check_field);

                if (!is_get) {
                    mPersistMediaConfig.number_scheduler_network_tcp_reciever = pre_config->number;
                    LOGM_CONFIGURATION("EGenericEnginePreConfigure_NetworkTCPReceiverSchedulerNumber %d\n", mPersistMediaConfig.number_scheduler_network_tcp_reciever);
                } else {
                    pre_config->number = mPersistMediaConfig.number_scheduler_network_tcp_reciever;
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
                            LOGM_ERROR("EGenericEnginePreConfigure_PlaybackSetDSPPrebuffer, pre_config->number (%d) should not excced max buffer number(%d) - 5\n", pre_config->number, mPersistMediaConfig.dsp_config.max_frm_num);
                            return EECode_BadParam;
                        }
                        mPersistMediaConfig.dsp_config.modeConfigMudec.enable_buffering_ctrl = 1;
                    } else {
                        mPersistMediaConfig.dsp_config.modeConfigMudec.enable_buffering_ctrl = 0;
                        mPersistMediaConfig.dsp_config.modeConfigMudec.pre_buffer_len = 0;
                        LOGM_CONFIGURATION("EGenericEnginePreConfigure_PlaybackSetDSPPrebuffer, disable dsp prebuffer\n");
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

        case EGenericEnginePreConfigure_RTSPStreamingServerSetAuthentication: {
                SGenericPreConfigure *pre_config = (SGenericPreConfigure *) param;
                DASSERT(EGenericEnginePreConfigure_RTSPStreamingServerSetAuthentication == pre_config->check_field);

                if (!is_get) {
                    mPersistMediaConfig.enable_rtsp_authentication = pre_config->number & 0xff;
                    LOGM_CONFIGURATION("EGenericEnginePreConfigure_RTSPStreamingServerSetAuthentication %d\n", pre_config->number);
                } else {
                    pre_config->number = mPersistMediaConfig.enable_rtsp_authentication;
                }
            }
            break;

        case EGenericEnginePreConfigure_RTSPStreamingServerAuthenticationMode: {
                SGenericPreConfigure *pre_config = (SGenericPreConfigure *) param;
                DASSERT(EGenericEnginePreConfigure_RTSPStreamingServerAuthenticationMode == pre_config->check_field);

                if (!is_get) {
                    mPersistMediaConfig.rtsp_authentication_mode = pre_config->number & 0xff;
                    LOGM_CONFIGURATION("EGenericEnginePreConfigure_RTSPStreamingServerAuthenticationMode %d\n", pre_config->number);
                } else {
                    pre_config->number = mPersistMediaConfig.rtsp_authentication_mode;
                }
            }
            break;

        case EGenericEnginePreConfigure_RTSPClientParseMultipleAccessUnit: {
                SGenericPreConfigure *pre_config = (SGenericPreConfigure *) param;
                DASSERT(EGenericEnginePreConfigure_RTSPClientParseMultipleAccessUnit == pre_config->check_field);

                if (!is_get) {
                    mPersistMediaConfig.rtsp_client_config.parse_multiple_access_unit = pre_config->number;
                    LOGM_CONFIGURATION("EGenericEnginePreConfigure_RTSPClientParseMultipleAccessUnit %d\n", pre_config->number);
                } else {
                    pre_config->number = mPersistMediaConfig.rtsp_client_config.parse_multiple_access_unit;
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
            break;

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

        case EGenericEnginePreConfigure_PreferModule_AudioDecoder_Customized_ADPCM: {
                mPersistMediaConfig.prefer_module_setting.mPreferedAudioDecoderModule = EAudioDecoderModuleID_CustomizedADPCM;
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

        case EGenericEnginePreConfigure_PreferModule_AudioEncoder_Customized_ADPCM: {
                mPersistMediaConfig.prefer_module_setting.mPreferedAudioEncoderModule = EAudioEncoderModuleID_CustomizedADPCM;
            }
            break;

        case EGenericEnginePreConfigure_PreferModule_AudioRenderer_ALSA: {
                mPersistMediaConfig.prefer_module_setting.mPreferedAudioRendererModule = EAudioRendererModuleID_ALSA;
            }
            break;

        case EGenericEnginePreConfigure_PreferModule_AudioRenderer_PulseAudio: {
                mPersistMediaConfig.prefer_module_setting.mPreferedAudioRendererModule = EAudioRendererModuleID_PulseAudio;
            }
            break;

        case EGenericEnginePreConfigure_AudioCapturer_Params: {
                SGenericAudioParam *pre_config = (SGenericAudioParam *) param;
                DASSERT(EGenericEnginePreConfigure_AudioCapturer_Params == pre_config->check_field);

                if (!is_get) {
                    mPersistMediaConfig.audio_prefer_setting.sample_rate = pre_config->audio_params.sample_rate;
                    mPersistMediaConfig.audio_prefer_setting.sample_format = pre_config->audio_params.sample_format;
                    mPersistMediaConfig.audio_prefer_setting.channel_number = pre_config->audio_params.channel_number;
                } else {
                    pre_config->audio_params.sample_rate = mPersistMediaConfig.audio_prefer_setting.sample_rate;
                    pre_config->audio_params.sample_format = (AudioSampleFMT) mPersistMediaConfig.audio_prefer_setting.sample_format;
                    pre_config->audio_params.channel_number = mPersistMediaConfig.audio_prefer_setting.channel_number;
                }
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

EECode CActiveGenericEngineV3::GenericControl(EGenericEngineConfigure config_type, void *param)
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
                if (DUnlikely(!playback_zoom)) {
                    LOGM_ERROR("NULL playback_zoom\n");
                    return EECode_BadParam;
                }

                DASSERT(EGenericEngineConfigure_VideoPlayback_Zoom == playback_zoom->check_field);

                SDSPControlParams params;
                params.u8_param[0] = playback_zoom->render_id;
                params.u8_param[1] = 0;
                params.u16_param[0] = (TU16)playback_zoom->input_center_x;
                params.u16_param[1] = (TU16)playback_zoom->input_center_y;
                params.u16_param[2] = (TU16)playback_zoom->input_width;
                params.u16_param[3] = (TU16)playback_zoom->input_height;

                if (DLikely(mpDSPAPI)) {
                    mpDSPAPI->DSPControl(EDSPControlType_UDEC_zoom, (void *)&params);
                } else {
                    LOGM_ERROR("NULL mpDSPAPI\n");
                    return EECode_BadState;
                }
            }
            break;

        case EGenericEngineConfigure_VideoPlayback_Zoom_OnDisplay: {
                SComponent *p_video_decoder;
                SPlaybackZoomOnDisplay *videoZoomParas = (SPlaybackZoomOnDisplay *)param;
                DASSERT(videoZoomParas);
                DASSERT(EGenericEngineConfigure_VideoPlayback_Zoom_OnDisplay == videoZoomParas->check_field);

                p_video_decoder = queryFilterComponent(EGenericComponentType_VideoDecoder, videoZoomParas->decoder_id);
                DASSERT(p_video_decoder);
                DASSERT(p_video_decoder->p_filter);
                p_video_decoder->p_filter->FilterProperty(EFilterPropertyType_vdecoder_zoom, 1, (void *)videoZoomParas);
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

        case EGenericEngineConfigure_VideoPlayback_CompensateDelayFromJitter: {
                mPersistMediaConfig.compensate_from_jitter = 1;
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

        case EGenericEngineConfigure_ReConnectServer: {
                SGenericReconnectStreamingServer *reconnect = (SGenericReconnectStreamingServer *)param;
                DASSERT(EGenericEngineConfigure_ReConnectServer == reconnect->check_field);
                DASSERT(reconnect);

                LOGM_NOTICE("before EGenericEngineConfigure_ReConnectServer\n");

                if (DLikely(!reconnect->all_demuxer)) {
                    SComponent *pfilter = queryFilterComponent(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_Demuxer, reconnect->index));
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
                SComponent *p_video_encoder;
                SGenericVideoEncoderParam *videoEncParas = (SGenericVideoEncoderParam *)param;
                DASSERT(videoEncParas);
                DASSERT(EGenericEngineConfigure_VideoEncoder_Params == videoEncParas->check_field);

                p_video_encoder = queryFilterComponent(EGenericComponentType_VideoEncoder, 0);
                DASSERT(p_video_encoder);
                DASSERT((EComponentState_started == p_video_encoder->cur_state) || (EComponentState_suspended == p_video_encoder->cur_state));
                if ((EComponentState_started != p_video_encoder->cur_state) && (EComponentState_suspended != p_video_encoder->cur_state)) {
                    LOGM_ERROR("component BAD state %d\n", p_video_encoder->cur_state);
                    return EECode_BadState;
                }
                DASSERT(p_video_encoder->p_filter);
                p_video_encoder->p_filter->FilterProperty(EFilterPropertyType_video_parameters, videoEncParas->set_or_get, (void *)&videoEncParas->video_params);
            }
            break;

        case EGenericEngineConfigure_VideoEncoder_Bitrate: {
                SGenericVideoEncoderParam *videoEncParas = (SGenericVideoEncoderParam *)param;
                if (DLikely(videoEncParas)) {
                    DASSERT(EGenericEngineConfigure_VideoEncoder_Bitrate == videoEncParas->check_field);
                    if (DLikely(mpDSPAPI)) {
                        err = mpDSPAPI->DSPControl(EDSPControlType_UDEC_TRANSCODER_update_bitrate, &videoEncParas->video_params);
                        DASSERT_OK(err);
                    } else {
                        LOGM_FATAL("NULL mpDSPAPI\n");
                        return EECode_BadState;
                    }
                }
            }
            break;

        case EGenericEngineConfigure_VideoEncoder_Framerate: {
                SGenericVideoEncoderParam *videoEncParas = (SGenericVideoEncoderParam *)param;
                if (DLikely(videoEncParas)) {
                    DASSERT(EGenericEngineConfigure_VideoEncoder_Framerate == videoEncParas->check_field);
                    if (DLikely(mpDSPAPI)) {
                        err = mpDSPAPI->DSPControl(EDSPControlType_UDEC_TRANSCODER_update_framerate, &videoEncParas->video_params);
                        DASSERT_OK(err);
                    } else {
                        LOGM_FATAL("NULL mpDSPAPI\n");
                        return EECode_BadState;
                    }
                }
            }
            break;

        case EGenericEngineConfigure_VideoEncoder_DemandIDR: {
                SGenericVideoEncoderParam *videoEncParas = (SGenericVideoEncoderParam *)param;
                if (DLikely(videoEncParas)) {
                    DASSERT(EGenericEngineConfigure_VideoEncoder_DemandIDR == videoEncParas->check_field);
                    if (DLikely(mpDSPAPI)) {
                        err = mpDSPAPI->DSPControl(EDSPControlType_UDEC_TRANSCODER_demand_IDR, &videoEncParas->video_params);
                        DASSERT_OK(err);
                    } else {
                        LOGM_FATAL("NULL mpDSPAPI\n");
                        return EECode_BadState;
                    }
                }
            }
            break;

        case EGenericEngineConfigure_VideoEncoder_GOP: {
                SGenericVideoEncoderParam *videoEncParas = (SGenericVideoEncoderParam *)param;
                if (DLikely(videoEncParas)) {
                    DASSERT(EGenericEngineConfigure_VideoEncoder_GOP == videoEncParas->check_field);
                    if (DLikely(mpDSPAPI)) {
                        err = mpDSPAPI->DSPControl(EDSPControlType_UDEC_TRANSCODER_update_gop, &videoEncParas->video_params);
                        DASSERT_OK(err);
                    } else {
                        LOGM_FATAL("NULL mpDSPAPI\n");
                        return EECode_BadState;
                    }
                }
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

        case EGenericEngineConfigure_VideoDisplay_UpdateOneWindow: {
                SGenericVideoPostPConfiguration *ppconfig = (SGenericVideoPostPConfiguration *)param;
                DASSERT(ppconfig);
                DASSERT(EGenericEngineConfigure_VideoDisplay_UpdateOneWindow == ppconfig->check_field);
                DASSERT(ppconfig->p_video_postp_info);
                if (!ppconfig->p_video_postp_info) {
                    LOGM_ERROR("EGenericEngineConfigure_VideoDisplay_UpdateOneWindow invalid para, p_video_postp_info=%p.\n", ppconfig->p_video_postp_info);
                    return EECode_BadParam;
                }

                if (DLikely(mpDSPAPI)) {
                    EECode err = mpDSPAPI->DSPControl(EDSPControlType_UDEC_one_window_config, (void *)ppconfig->p_video_postp_info);
                    DASSERT_OK(err);
                    if (DUnlikely(EECode_OK != err)) {
                        LOGM_FATAL("EDSPControlType_UDEC_window_config return %d, %s\n", err, gfGetErrorCodeString(err));
                        return err;
                    }
                } else {
                    LOGM_FATAL("NULL mpDSPAPI\n");
                    return EECode_BadState;
                }
            }
            break;

        case EGenericEngineConfigure_Record_Suspend: {
                SUserParamRecordSuspend *suspend = (SUserParamRecordSuspend *)param;
                DASSERT(suspend);
                DASSERT(EGenericEngineConfigure_Record_Suspend == suspend->check_field);

                SComponent *pfilter = queryFilterComponent(suspend->component_id);
                if (pfilter && pfilter->p_filter) {
                    err = pfilter->p_filter->FilterProperty(EFilterPropertyType_muxer_suspend, 1, param);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_muxer_suspend return %d\n", err);
                    }
                } else {
                    LOGM_FATAL("No muxer component for id %08x\n", suspend->component_id);
                    err = EECode_BadParam;
                }
            }
            break;

        case EGenericEngineConfigure_Record_ResumeSuspend: {
                SUserParamRecordSuspend *suspend = (SUserParamRecordSuspend *)param;
                DASSERT(suspend);
                DASSERT(EGenericEngineConfigure_Record_ResumeSuspend == suspend->check_field);

                SComponent *pfilter = queryFilterComponent(suspend->component_id);
                if (pfilter && pfilter->p_filter) {
                    err = pfilter->p_filter->FilterProperty(EFilterPropertyType_muxer_resume_suspend, 1, param);
                    if (EECode_OK != err) {
                        LOGM_FATAL("EFilterPropertyType_muxer_resume_suspend return %d\n", err);
                    }
                } else {
                    LOGM_FATAL("No muxer component for id %08x\n", suspend->component_id);
                    err = EECode_BadParam;
                }
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

EECode CActiveGenericEngineV3::GetPipeline(TU8 type, void *&p_pipeline)
{
    TInt index = 0;
    if (type == 0) {
        //streaming pipeline
        for (index = 0; index < mnStreamingPipelinesCurrentNumber; index++) {
            SStreamingPipeline *p = mpStreamingPipelines + index;
            //todo
            if (0) {
                p_pipeline = (void *)p;
                break;
            }
        }
    } else if (type == 1) {
        //vod pipeline
        for (index = 0; index < mnVODPipelinesCurrentNumber; index++) {
            SVODPipeline *p = mpVODPipelines + index;
            //todo
            if (p->is_used == 0) {
                p->is_used = 1;
                p_pipeline = (void *)p;
                break;
            }
        }
    } else {
        //other pipeline
    }

    return (p_pipeline ? EECode_OK : EECode_Error);
}

EECode CActiveGenericEngineV3::FreePipeline(TU8 type, void *p_pipeline)
{
    DASSERT(p_pipeline);
    if (type == 0) {
        //SStreamingPipeline* p = (SStreamingPipeline*)p_pipeline;

    } else if (type == 1) {
        SVODPipeline *p = (SVODPipeline *)p_pipeline;
        p->is_used = 0;
    } else {
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV3::ParseContentExtraData(void *p_content)
{
    DASSERT(p_content);
    SStreamingSessionContent *pContent = (SStreamingSessionContent *)p_content;
    SDateTime /*requestTime, */fileStartTime;
    TU16 fileDuration = 0;
    TChar *pFileName = NULL;
    if (mpStorageManager->RequestExistUintSequentially(pContent->session_name, NULL, 1, pFileName, &fileStartTime, fileDuration) != EECode_OK) {
        LOGM_FATAL("SetupVodContent, mpStorageManager get the first file fail! channel name %s\n", pContent->session_name);
        return EECode_InternalLogicalBug;
    }

    if (EECode_OK != parseExtraData(pFileName, pContent)) {
        LOGM_FATAL("parseExtraData fail!\n");
        return EECode_InternalLogicalBug;
    }

    mpStorageManager->ReleaseExistUint(pContent->session_name, pFileName);

    return EECode_OK;
}

EECode CActiveGenericEngineV3::UpdateDeviceDynamicCode(TGenericID agent_id, TU32 dynamic_code)
{

    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(agent_id);

    if (DUnlikely(NULL == mpCloudPipelines)) {
        LOGM_ERROR("mpCloudPipelines is NULL?\n");
        return EECode_BadState;
    }

    DASSERT(mbGraghBuilt);
    if (DLikely(mpCloudContent)) {
        mpCloudContent[index].dynamic_code = dynamic_code;
    } else {
        LOGM_FATAL("NULL mpCloudContent here?\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV3::UpdateUserDynamicCode(const TChar *p_user_name, const TChar *p_device_name, TU32 dynamic_code)
{
    EECode err = EECode_OK;

    DASSERT(mpSimpleHashTable);
    if (DUnlikely(!p_user_name)) {
        LOGM_FATAL("UpdateUserDynamicCode: user name is NULL\n");
        return EECode_BadParam;
    }

    if (dynamic_code == 0) {
        err = mpSimpleHashTable->Remove(p_user_name, p_device_name);
    } else {
        err = mpSimpleHashTable->Add(p_user_name, p_device_name, dynamic_code, 1);
    }

    if (DUnlikely(err != EECode_OK)) {
        LOGM_FATAL("UpdateUserDynamicCode, user name: %s, device name: %s, mpSimpleHashTable->Add fail\n", p_user_name, p_device_name);
        return err;
    }

    return EECode_OK;

}

EECode CActiveGenericEngineV3::LookupPassword(const TChar *p_user_name, const TChar *p_device_name, TU32 &dynamic_code)
{
    EECode err = EECode_OK;

    if (DUnlikely(!p_user_name || !p_device_name)) {
        LOGM_FATAL("LookupPassword, bad parameter\n");
        return EECode_BadParam;
    }

    DASSERT(mpSimpleHashTable);
    err = mpSimpleHashTable->Query(p_user_name, p_device_name, dynamic_code);
    if (DUnlikely(err != EECode_OK)) {
        LOGM_FATAL("LookupPassword, query hash table fail\n");
        return err;
    }

    return EECode_OK;
}

void CActiveGenericEngineV3::Destroy()
{
    Delete();
}

void CActiveGenericEngineV3::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
    if (DLikely(param2)) {
        if (DLikely(EEventType_Timer == type)) {
            if (((TULong)&mDailyCheckContext) == ((TULong)param2)) {
                LOGM_NOTICE("EventNotify %d\n", type);
                if (mbSelfLogFile) {
                    nextLogFile();
                }
                mpDailyCheckTimer = NULL;
                SMSG msg;
                memset(&msg, 0x0, sizeof(msg));
                msg.pExtra = NULL;
                msg.code = ECMDType_MiscCheckLicence;
                PostEngineMsg(msg);
                mDailyCheckTime += (TTime)24 * 60 * 60 * 1000000;
                mpDailyCheckTimer = mpSystemClockReference->AddTimer(this, EClockTimerType_once, mDailyCheckTime, (TTime)24 * 60 * 60 * 1000000, 1);
                mpDailyCheckTimer->user_context = (TULong)&mDailyCheckContext;
            } else {
                LOGM_FATAL("why comes here\n");
            }
        } else {
            LOGM_FATAL("unknown type %d\n", type);
        }
    } else {
        LOGM_FATAL("unknown event param2 %ld, type%d\n", param2, type);
    }
}

void CActiveGenericEngineV3::nextLogFile()
{
    SDateTime date;
    gfGetCurrentDateTime(&date);

    TChar logfile[256] = {0};
    if ((date.year == mLastCheckDate.year) && (date.month == mLastCheckDate.month) && (date.day == mLastCheckDate.day)) {
        mCurrentLogFileIndex ++;
        snprintf(logfile, 256, "./log/dataserver_v3_%04d_%02d_%02d_%04d.log", date.year, date.month, date.day, mCurrentLogFileIndex);
    } else {
        mCurrentLogFileIndex = 0;
        snprintf(logfile, 256, "./log/dataserver_v3_%04d_%02d_%02d.log", date.year, date.month, date.day);
        mLastCheckDate = date;
    }
    gfOpenLogFile((TChar *)logfile);
}

void CActiveGenericEngineV3::openNextSnapshotFile()
{
    SDateTime date;
    gfGetCurrentDateTime(&date);

    TChar snapshotfile[256] = {0};
    if ((date.year == mLastSnapshotDate.year) && (date.month == mLastSnapshotDate.month) && (date.day == mLastSnapshotDate.day)) {
    } else {
        mCurrentSnapshotFileIndex = 0;
        mLastSnapshotDate = date;
    }
    snprintf(snapshotfile, 256, "./log/snapshot_%04d_%02d_%02d_%04d.log", date.year, date.month, date.day, mCurrentSnapshotFileIndex);

    mCurrentSnapshotFileIndex ++;

    gfOpenSnapshotFile((TChar *)snapshotfile);
}

void CActiveGenericEngineV3::OnRun()
{
    SCMD cmd, nextCmd;
    SMSG msg;
    CIQueue::QType type;
    CIQueue::WaitResult result;
    DASSERT(mpFilterMsgQ);
    DASSERT(mpCmdQueue);
    mpWorkQ->CmdAck(EECode_OK);

    while (mbRun) {
        LOGM_FLOW("CActiveGenericEngineV3: msg cnt from filters %d.\n", mpFilterMsgQ->GetDataCnt());

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

    LOGM_STATE("CActiveGenericEngineV3: start Clear gragh for safe.\n");
    NewSession();
    mpWorkQ->CmdAck(EECode_OK);
}

SComponent *CActiveGenericEngineV3::newComponent(TComponentType type, TComponentIndex index, const TChar *prefer_string)
{
    SComponent *p = (SComponent *) DDBG_MALLOC(sizeof(SComponent), "E3CC");

    if (p) {
        memset(p, 0x0, sizeof(SComponent));
        p->id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(type, index);
        p->type = type;
        p->index = index;
        p->cur_state = EComponentState_not_alive;

        if (prefer_string) {
            strncpy(p->prefer_string, prefer_string, DMaxPreferStringLen - 1);
        }
    }

    return p;
}

void CActiveGenericEngineV3::deleteComponent(SComponent *p)
{
    if (p) {
        if (p->url) {
            DDBG_FREE(p->url, "E3UR");
            p->url = NULL;
        }
        if (p->remote_url) {
            DDBG_FREE(p->remote_url, "E3UR");
            p->remote_url = NULL;
        }
        DDBG_FREE(p, "E3CC");
    }
}

SConnection *CActiveGenericEngineV3::newConnetion(TGenericID up_id, TGenericID down_id, SComponent *up_component, SComponent *down_component)
{
    SConnection *p = (SConnection *) DDBG_MALLOC(sizeof(SConnection), "E3Cc");
    if (p) {
        memset(p, 0x0, sizeof(SConnection));
        p->connection_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_ConnectionPin, mnComponentProxysNumbers[EGenericComponentType_ConnectionPin - EGenericComponentType_TAG_proxy_start]++);
        p->up_id = up_id;
        p->down_id = down_id;

        p->up = up_component;
        p->down = down_component;

        p->state = EConnectionState_not_alive;
    }

    return p;
}

void CActiveGenericEngineV3::deleteConnection(SConnection *p)
{
    if (p) {
        DDBG_FREE(p, "E3Cc");
    }
}

void CActiveGenericEngineV3::deactivateComponent(SComponent *p_component, TUint release_flag)
{
    if (DLikely(p_component)) {
        //LOGM_PRINTF("[Debug flow]: deactivete component id 0x%08x, %s, current count %d\n", p_component->id, gfGetComponentStringFromGenericComponentType(p_component->type), p_component->active_count);
        if (1 == p_component->active_count) {
            p_component->active_count = 0;
            if (DLikely(p_component->p_filter)) {
                EECode err = p_component->p_filter->Suspend(release_flag);
                //DASSERT_OK(err);
                LOGM_PRINTF("[Debug flow]: deactivete component, Suspend, id 0x%08x, %s, return %s\n", p_component->id, gfGetComponentStringFromGenericComponentType(p_component->type), gfGetErrorCodeString(err));
            } else {
                LOGM_FATAL("NULL p_component->p_filter, id 0x%08x, %s\n", p_component->id, gfGetComponentStringFromGenericComponentType(p_component->type));
            }
        } else {
            LOGM_PRINTF("[Debug flow]: deactivate p_component(0x%08x, %s)->active_count %d\n", p_component->id, gfGetComponentStringFromGenericComponentType(p_component->type), p_component->active_count);
            p_component->active_count --;
        }
    } else {
        LOGM_FATAL("NULL p_component\n");
    }
}

void CActiveGenericEngineV3::activateComponent(SComponent *p_component, TU8 update_focus_index, TComponentIndex focus_index)
{
    if (DLikely(p_component)) {
        //LOGM_PRINTF("[Debug flow]: activete component id 0x%08x, %s, current count %d\n", p_component->id, gfGetComponentStringFromGenericComponentType(p_component->type), p_component->active_count);
        if (0 == p_component->active_count) {
            if (DUnlikely(EComponentState_activated == p_component->cur_state)) {
                if (DLikely(p_component->p_filter)) {
                    p_component->p_filter->Start();
                    p_component->cur_state = EComponentState_started;
                } else {
                    LOGM_FATAL("NULL p_component->p_filter\n");
                }
            }
            p_component->active_count = 1;
            if (DLikely(p_component->p_filter)) {
                EECode err = p_component->p_filter->ResumeSuspend(focus_index);
                //DASSERT_OK(err);
                LOGM_PRINTF("[Debug flow]: activete component: ResumeSuspend(%d), id 0x%08x, %s, update_focus_index %d, ret %s\n", focus_index, p_component->id, gfGetComponentStringFromGenericComponentType(p_component->type), update_focus_index, gfGetErrorCodeString(err));
            } else {
                LOGM_FATAL("NULL p_component->p_filter, id 0x%08x, %s\n", p_component->id, gfGetComponentStringFromGenericComponentType(p_component->type));
            }
        } else {
            if (DLikely((p_component->p_filter) && (update_focus_index))) {
                EECode err = p_component->p_filter->SwitchInput(focus_index);
                DASSERT_OK(err);
                LOGM_PRINTF("[Debug flow]: activete component: SwitchInput(%d), id 0x%08x, %s, update_focus_index %d\n", focus_index, p_component->id, gfGetComponentStringFromGenericComponentType(p_component->type), update_focus_index);
            } else {
                LOGM_PRINTF("[Debug flow]: activate p_component(0x%08x, %s)->active_count %d\n", p_component->id, gfGetComponentStringFromGenericComponentType(p_component->type), p_component->active_count);
            }
            p_component->active_count ++;
        }
    } else {
        LOGM_FATAL("NULL p_component\n");
    }
}

SComponent *CActiveGenericEngineV3::findComponentFromID(TGenericID id)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;

    while (pnode) {
        p_component = (SComponent *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            if (p_component->id == id) {
                return p_component;
            }
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mpListFilters->NextNode(pnode);
    }

    pnode = mpListProxys->FirstNode();
    while (pnode) {
        p_component = (SComponent *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            if (p_component->id == id) {
                return p_component;
            }
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mpListProxys->NextNode(pnode);
    }

    LOGM_ERROR("cannot find component, id 0x%08x\n", id);
    return NULL;
}

SComponent *CActiveGenericEngineV3::findComponentFromTypeIndex(TU8 type, TU8 index)
{
    TGenericID id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(type, index);

    return findComponentFromID(id);
}

SComponent *CActiveGenericEngineV3::findComponentFromFilter(IFilter *p_filter)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;

    while (pnode) {
        p_component = (SComponent *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            if (p_component->p_filter == p_filter) {
                return p_component;
            }
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mpListFilters->NextNode(pnode);
    }

    LOGM_WARN("cannot find component, by filter %p\n", p_filter);
    return NULL;
}

SComponent *CActiveGenericEngineV3::queryFilterComponent(TGenericID filter_id)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;

    while (pnode) {
        p_component = (SComponent *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component->id == filter_id) {
            return p_component;
        }
        pnode = mpListFilters->NextNode(pnode);
    }

    return NULL;
}

SComponent *CActiveGenericEngineV3::queryFilterComponent(TComponentType type, TComponentIndex index)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;

    while (pnode) {
        p_component = (SComponent *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component->type == type
                && p_component->index == index) {
            return p_component;
        }
        pnode = mpListFilters->NextNode(pnode);
    }

    return NULL;
}

SConnection *CActiveGenericEngineV3::queryConnection(TGenericID connection_id)
{
    CIDoubleLinkedList::SNode *pnode = mpListConnections->FirstNode();
    SConnection *p_connection = NULL;

    while (pnode) {
        p_connection = (SConnection *)(pnode->p_context);
        DASSERT(p_connection);
        if (p_connection->connection_id == connection_id) {
            return p_connection;
        }
        pnode = mpListConnections->NextNode(pnode);
    }

    return NULL;
}

SConnection *CActiveGenericEngineV3::queryConnection(TGenericID up_id, TGenericID down_id, StreamType type)
{
    CIDoubleLinkedList::SNode *pnode = NULL;
    SConnection *p_connection = NULL;

    pnode = mpListConnections->FirstNode();

    while (pnode) {
        p_connection = (SConnection *)(pnode->p_context);
        DASSERT(p_connection);
        if ((p_connection->up_id == up_id) && (p_connection->down_id == down_id)) {
            if (p_connection->pin_type == type) {
                return p_connection;
            }
        }
        pnode = mpListConnections->NextNode(pnode);
    }

    return NULL;
}

EECode CActiveGenericEngineV3::setupPlaybackPipeline(TComponentIndex index)
{
    if (DLikely(mpPlaybackPipelines && (mnPlaybackPipelinesCurrentNumber > index))) {
        SPlaybackPipeline *p_pipeline = mpPlaybackPipelines + index;
        TGenericID video_source_id = p_pipeline->video_source_id;
        TGenericID video_decoder_id = p_pipeline->video_decoder_id;
        TGenericID video_renderer_id = p_pipeline->video_renderer_id;
        TGenericID audio_source_id = p_pipeline->audio_source_id;
        TGenericID audio_decoder_id = p_pipeline->audio_decoder_id;
        TGenericID audio_renderer_id = p_pipeline->audio_renderer_id;
        TGenericID audio_pinmuxer_id = p_pipeline->audio_pinmuxer_id;

        DASSERT(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Playback, index) == p_pipeline->pipeline_id);
        DASSERT(p_pipeline->index == index);
        DASSERT(EGenericPipelineType_Playback == p_pipeline->type);
        DASSERT(EGenericPipelineState_not_inited == p_pipeline->pipeline_state);
        DASSERT(p_pipeline->audio_enabled || p_pipeline->video_enabled);

        if (video_source_id) {

            if (DUnlikely(0 == isComponentIDValid_3(video_source_id, EGenericComponentType_Demuxer, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_CloudConnecterClientAgent))) {
                LOGM_ERROR("not valid video source id %08x\n", video_source_id);
                return EECode_BadParam;
            }

            if (DUnlikely(0 == isComponentIDValid(video_decoder_id, EGenericComponentType_VideoDecoder))) {
                LOGM_ERROR("not valid video decoder id %08x\n", video_decoder_id);
                return EECode_BadParam;
            }

            if (DUnlikely(0 == isComponentIDValid_2(video_renderer_id, EGenericComponentType_VideoRenderer, EGenericComponentType_VideoOutput))) {
                LOGM_ERROR("not valid video renderer/output id %08x\n", video_renderer_id);
                return EECode_BadParam;
            }

            p_pipeline->video_enabled = 1;
        } else {
            DASSERT(!video_decoder_id);
            DASSERT(!video_renderer_id);
            p_pipeline->video_enabled = 0;
        }

        if (audio_source_id) {
            if (DUnlikely(0 == isComponentIDValid_4(audio_source_id, EGenericComponentType_Demuxer, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_CloudConnecterClientAgent, EGenericComponentType_AudioCapture))) {
                LOGM_ERROR("not valid audio source id %08x\n", audio_source_id);
                return EECode_BadParam;
            }

            if (audio_decoder_id) {
                if (DUnlikely(0 == isComponentIDValid(audio_decoder_id, EGenericComponentType_AudioDecoder))) {
                    LOGM_ERROR("not valid audio decoder id %08x\n", audio_decoder_id);
                    return EECode_BadParam;
                }
            }

            if (DUnlikely(0 == isComponentIDValid_2(audio_renderer_id, EGenericComponentType_AudioRenderer, EGenericComponentType_AudioOutput))) {
                LOGM_ERROR("not valid audio renderer/output id %08x\n", audio_renderer_id);
                return EECode_BadParam;
            }

            if (audio_pinmuxer_id) {
                if (DUnlikely(0 == isComponentIDValid(audio_pinmuxer_id, EGenericComponentType_ConnecterPinMuxer))) {
                    LOGM_ERROR("not valid audio pin muxer id %08x\n", audio_pinmuxer_id);
                    return EECode_BadParam;
                }
            }

            p_pipeline->audio_enabled = 1;
        } else {
            DASSERT(!audio_decoder_id);
            DASSERT(!audio_renderer_id);
            DASSERT(!audio_pinmuxer_id);
            p_pipeline->audio_enabled = 0;
        }

        if (DLikely(p_pipeline->video_enabled || p_pipeline->audio_enabled)) {
            p_pipeline->pipeline_state = EGenericPipelineState_build_gragh;
            SComponent *p_component = NULL;
            SConnection *p_connection = NULL;

            if (p_pipeline->video_enabled) {
                p_component = findComponentFromID(p_pipeline->video_source_id);
                if (DLikely(p_component)) {
                    p_component->playback_pipeline_number ++;
                    p_pipeline->p_video_source = p_component;
                    if (p_pipeline->config_running_at_startup) {
                        p_component->active_count ++;
                    }
                } else {
                    LOGM_ERROR("BAD p_pipeline->video_source_id %08x\n", p_pipeline->video_source_id);
                    return EECode_InternalLogicalBug;
                }

                p_component = findComponentFromID(p_pipeline->video_decoder_id);
                if (DLikely(p_component)) {
                    p_component->playback_pipeline_number ++;
                    p_pipeline->p_video_decoder = p_component;
                    if (p_pipeline->config_running_at_startup) {
                        p_component->active_count ++;
                    }
                } else {
                    LOGM_ERROR("BAD p_pipeline->video_decoder_id %08x\n", p_pipeline->video_decoder_id);
                    return EECode_InternalLogicalBug;
                }

                p_component = findComponentFromID(p_pipeline->video_renderer_id);
                if (DLikely(p_component)) {
                    p_component->playback_pipeline_number ++;
                    p_pipeline->p_video_renderer = p_component;
                    if (p_pipeline->config_running_at_startup) {
                        p_component->active_count ++;
                    }
                } else {
                    LOGM_ERROR("BAD p_pipeline->video_renderer_id %08x\n", p_pipeline->video_renderer_id);
                    return EECode_InternalLogicalBug;
                }

                p_connection = queryConnection(p_pipeline->video_source_id, p_pipeline->video_decoder_id, StreamType_Video);
                if (DLikely(p_connection)) {
                    p_pipeline->vd_input_connection_id = p_connection->connection_id;
                    p_pipeline->p_vd_input_connection = p_connection;
                } else {
                    LOGM_ERROR("no connection between video_source_id %08x and video_decoder_id %08x\n", p_pipeline->video_source_id, p_pipeline->video_decoder_id);
                    return EECode_InternalLogicalBug;
                }

                p_connection = queryConnection(p_pipeline->video_decoder_id, p_pipeline->video_renderer_id, StreamType_Video);
                if (DLikely(p_connection)) {
                    p_pipeline->vd_output_connection_id = p_connection->connection_id;
                    p_pipeline->p_vd_output_connection = p_connection;
                } else {
                    LOGM_ERROR("no connection between video_decoder_id %08x and video_renderer_id %08x\n", p_pipeline->video_decoder_id, p_pipeline->video_renderer_id);
                    return EECode_InternalLogicalBug;
                }

            }

            if (p_pipeline->audio_enabled) {
                p_component = findComponentFromID(p_pipeline->audio_source_id);
                if (DLikely(p_component)) {
                    p_component->playback_pipeline_number ++;
                    p_pipeline->p_audio_source = p_component;
                    if (p_pipeline->config_running_at_startup) {
                        p_component->active_count ++;
                    }
                } else {
                    LOGM_ERROR("BAD p_pipeline->audio_source_id %08x\n", p_pipeline->audio_source_id);
                    return EECode_InternalLogicalBug;
                }

                if (p_pipeline->audio_decoder_id) {
                    p_component = findComponentFromID(p_pipeline->audio_decoder_id);
                    if (DLikely(p_component)) {
                        p_component->playback_pipeline_number ++;
                        p_pipeline->p_audio_decoder = p_component;
                        if (p_pipeline->config_running_at_startup) {
                            p_component->active_count ++;
                        }
                    } else {
                        LOGM_ERROR("BAD p_pipeline->audio_decoder_id %08x\n", p_pipeline->audio_decoder_id);
                        return EECode_InternalLogicalBug;
                    }
                }

                p_component = findComponentFromID(p_pipeline->audio_renderer_id);
                if (DLikely(p_component)) {
                    p_component->playback_pipeline_number ++;
                    p_pipeline->p_audio_renderer = p_component;
                    if (p_pipeline->config_running_at_startup) {
                        p_component->active_count ++;
                    }
                } else {
                    LOGM_ERROR("BAD p_pipeline->audio_renderer_id %08x\n", p_pipeline->audio_renderer_id);
                    return EECode_InternalLogicalBug;
                }

                if (p_pipeline->audio_decoder_id) {
                    //have audio decoder's case
                    if (p_pipeline->audio_pinmuxer_id) {
                        p_component = findComponentFromID(p_pipeline->audio_pinmuxer_id);
                        if (DLikely(p_component)) {
                            p_component->playback_pipeline_number ++;
                            p_pipeline->p_audio_pinmuxer = p_component;
                            if (p_pipeline->config_running_at_startup) {
                                p_component->active_count ++;
                            }
                        } else {
                            LOGM_ERROR("BAD p_pipeline->audio_pinmuxer_id %08x\n", p_pipeline->audio_pinmuxer_id);
                            return EECode_InternalLogicalBug;
                        }

                        p_connection = queryConnection(p_pipeline->audio_source_id, p_pipeline->audio_pinmuxer_id, StreamType_Audio);
                        if (DLikely(p_connection)) {
                            p_pipeline->as_output_connection_id = p_connection->connection_id;
                            p_pipeline->p_as_output_connection = p_connection;
                        } else {
                            LOGM_ERROR("no connection between audio_source_id %08x and audio_pinmuxer_id %08x\n", p_pipeline->audio_source_id, p_pipeline->audio_pinmuxer_id);
                            return EECode_InternalLogicalBug;
                        }

                        p_connection = queryConnection(p_pipeline->audio_pinmuxer_id, p_pipeline->audio_decoder_id, StreamType_Audio);
                        if (DLikely(p_connection)) {
                            p_pipeline->ad_input_connection_id = p_connection->connection_id;
                            p_pipeline->p_ad_input_connection = p_connection;
                        } else {
                            LOGM_ERROR("no connection between audio_pinmuxer_id %08x and audio_decoder_id %08x\n", p_pipeline->audio_pinmuxer_id, p_pipeline->audio_decoder_id);
                            return EECode_InternalLogicalBug;
                        }

                    } else {
                        p_connection = queryConnection(p_pipeline->audio_source_id, p_pipeline->audio_decoder_id, StreamType_Audio);
                        if (DLikely(p_connection)) {
                            p_pipeline->ad_input_connection_id = p_connection->connection_id;
                            p_pipeline->p_ad_input_connection = p_connection;
                        } else {
                            LOGM_ERROR("no connection between audio_source_id %08x and audio_decoder_id %08x\n", p_pipeline->audio_source_id, p_pipeline->audio_decoder_id);
                            return EECode_InternalLogicalBug;
                        }
                    }

                    p_connection = queryConnection(p_pipeline->audio_decoder_id, p_pipeline->audio_renderer_id, StreamType_Audio);
                    if (DLikely(p_connection)) {
                        p_pipeline->ad_output_connection_id = p_connection->connection_id;
                        p_pipeline->p_ad_output_connection = p_connection;
                    } else {
                        LOGM_ERROR("no connection between audio_decoder_id %08x and audio_renderer_id %08x\n", p_pipeline->audio_decoder_id, p_pipeline->audio_renderer_id);
                        return EECode_InternalLogicalBug;
                    }
                } else {
                    //without audio decoder's case
                    if (p_pipeline->audio_pinmuxer_id) {
                        p_component = findComponentFromID(p_pipeline->audio_pinmuxer_id);
                        if (DLikely(p_component)) {
                            p_component->playback_pipeline_number ++;
                            p_pipeline->p_audio_pinmuxer = p_component;
                            if (p_pipeline->config_running_at_startup) {
                                p_component->active_count ++;
                            }
                        } else {
                            LOGM_ERROR("BAD p_pipeline->audio_pinmuxer_id %08x\n", p_pipeline->audio_pinmuxer_id);
                            return EECode_InternalLogicalBug;
                        }

                        p_connection = queryConnection(p_pipeline->audio_source_id, p_pipeline->audio_pinmuxer_id, StreamType_Audio);
                        if (DLikely(p_connection)) {
                            p_pipeline->as_output_connection_id = p_connection->connection_id;
                            p_pipeline->p_as_output_connection = p_connection;
                        } else {
                            LOGM_ERROR("no connection between audio_source_id %08x and audio_pinmuxer_id %08x\n", p_pipeline->audio_source_id, p_pipeline->audio_pinmuxer_id);
                            return EECode_InternalLogicalBug;
                        }

                        p_connection = queryConnection(p_pipeline->audio_pinmuxer_id, p_pipeline->audio_renderer_id, StreamType_Audio);
                        if (DLikely(p_connection)) {
                            p_pipeline->apinmuxer_output_connection_id = p_connection->connection_id;
                            p_pipeline->p_apinmuxer_output_connection = p_connection;
                        } else {
                            LOGM_ERROR("no connection between audio_pinmuxer_id %08x and audio_renderer_id %08x\n", p_pipeline->audio_pinmuxer_id, p_pipeline->audio_renderer_id);
                            return EECode_InternalLogicalBug;
                        }

                    } else {
                        p_connection = queryConnection(p_pipeline->audio_source_id, p_pipeline->audio_renderer_id, StreamType_Audio);
                        if (DLikely(p_connection)) {
                            p_pipeline->as_output_connection_id = p_connection->connection_id;
                            p_pipeline->p_as_output_connection = p_connection;
                        } else {
                            LOGM_ERROR("no connection between audio_source_id %08x and audio_renderer_id %08x\n", p_pipeline->audio_source_id, p_pipeline->audio_renderer_id);
                            return EECode_InternalLogicalBug;
                        }
                    }

                }

            }

            if (p_pipeline->config_running_at_startup) {
                p_pipeline->pipeline_state = EGenericPipelineState_running;
                LOGM_PRINTF("playback pipeline, id 0x%08x, start state is EGenericPipelineState_running\n", p_pipeline->pipeline_id);
            } else {
                p_pipeline->pipeline_state = EGenericPipelineState_suspended;
                LOGM_PRINTF("playback pipeline, id 0x%08x, start state is EGenericPipelineState_suspended\n", p_pipeline->pipeline_id);
            }

        } else {
            LOGM_ERROR("video and audio path are both disabled?\n");
            return EECode_BadParam;
        }


    } else {
        LOGM_ERROR("BAD state, mpPlaybackPipelines %p\n", mpPlaybackPipelines);
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV3::setupRecordingPipeline(TComponentIndex index)
{
    if (DLikely(mpRecordingPipelines && (mnRecordingPipelinesCurrentNumber > index))) {
        SRecordingPipeline *p_pipeline = mpRecordingPipelines + index;
        TGenericID video_source_id = p_pipeline->video_source_id[0];
        TGenericID audio_source_id = p_pipeline->audio_source_id[0];
        TGenericID sink_id = p_pipeline->sink_id;

        DASSERT(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Recording, index) == p_pipeline->pipeline_id);
        DASSERT(p_pipeline->index == index);
        DASSERT(EGenericPipelineType_Recording == p_pipeline->type);
        DASSERT(EGenericPipelineState_not_inited == p_pipeline->pipeline_state);
        DASSERT(p_pipeline->audio_enabled || p_pipeline->video_enabled);

        if (DUnlikely(0 == isComponentIDValid(sink_id, EGenericComponentType_Muxer))) {
            LOGM_ERROR("not valid sink_id %08x\n", sink_id);
            return EECode_BadParam;
        }

        if (video_source_id) {
            if (DUnlikely(0 == isComponentIDValid_3(video_source_id, EGenericComponentType_Demuxer, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_CloudConnecterClientAgent))) {
                LOGM_ERROR("not valid video source id %08x\n", video_source_id);
                return EECode_BadParam;
            }
            p_pipeline->video_enabled = 1;
        } else {
            p_pipeline->video_enabled = 0;
        }

        if (audio_source_id) {
            if (DUnlikely(0 == isComponentIDValid_3(audio_source_id, EGenericComponentType_Demuxer, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_CloudConnecterClientAgent))) {
                LOGM_ERROR("not valid audio source id %08x\n", audio_source_id);
                return EECode_BadParam;
            }
            p_pipeline->audio_enabled = 1;
        } else {
            p_pipeline->audio_enabled = 0;
        }

        if (DLikely(p_pipeline->video_enabled || p_pipeline->audio_enabled)) {
            p_pipeline->pipeline_state = EGenericPipelineState_build_gragh;
            SComponent *p_component = NULL;
            SConnection *p_connection = NULL;

            p_component = findComponentFromID(sink_id);
            if (DLikely(p_component)) {
                p_component->recording_pipeline_number ++;
                p_pipeline->p_sink = p_component;
                if (p_pipeline->config_running_at_startup) {
                    p_component->active_count ++;
                }
                //for muxer
                p_component->channel_name = p_pipeline->channel_name;
                if (p_component->channel_name) {
                    p_component->p_filter->FilterProperty(EFilterPropertyType_muxer_set_channel_name, 1, static_cast<void *>(p_component->channel_name));
                }
            } else {
                LOGM_ERROR("BAD sink_id %08x\n", sink_id);
                return EECode_InternalLogicalBug;
            }

            if (p_pipeline->video_enabled) {
                p_component = findComponentFromID(video_source_id);
                if (DLikely(p_component)) {
                    p_component->recording_pipeline_number ++;
                    p_pipeline->p_video_source[0] = p_component;
                    if (p_pipeline->config_running_at_startup) {
                        p_component->active_count ++;
                    }
                } else {
                    LOGM_ERROR("BAD video_source_id %08x\n", video_source_id);
                    return EECode_InternalLogicalBug;
                }

                p_connection = queryConnection(video_source_id, sink_id, StreamType_Video);
                if (DLikely(p_connection)) {
                    p_pipeline->video_source_connection_id[0] = p_connection->connection_id;
                    p_pipeline->p_video_source_connection[0] = p_connection;
                } else {
                    LOGM_ERROR("no connection between video_source_id %08x and sink_id %08x\n", video_source_id, sink_id);
                    return EECode_InternalLogicalBug;
                }
            }

            if (p_pipeline->audio_enabled) {
                p_component = findComponentFromID(audio_source_id);
                if (DLikely(p_component)) {
                    p_component->recording_pipeline_number ++;
                    p_pipeline->p_audio_source[0] = p_component;
                    if (p_pipeline->config_running_at_startup) {
                        p_component->active_count ++;
                    }
                } else {
                    LOGM_ERROR("BAD audio_source_id %08x\n", audio_source_id);
                    return EECode_InternalLogicalBug;
                }

                p_connection = queryConnection(audio_source_id, sink_id, StreamType_Audio);
                if (DLikely(p_connection)) {
                    p_pipeline->audio_source_connection_id[0] = p_connection->connection_id;
                    p_pipeline->p_audio_source_connection[0] = p_connection;
                    if (p_pipeline->config_running_at_startup) {
                        p_component->active_count ++;
                    }
                } else {
                    LOGM_ERROR("no connection between audio_source_id %08x and sink_id %08x\n", audio_source_id, sink_id);
                    return EECode_InternalLogicalBug;
                }

            }

            if (p_pipeline->config_running_at_startup) {
                p_pipeline->pipeline_state = EGenericPipelineState_running;
                LOGM_PRINTF("recording pipeline, id 0x%08x, start state is EGenericPipelineState_running\n", p_pipeline->pipeline_id);
            } else {
                p_pipeline->pipeline_state = EGenericPipelineState_suspended;
                LOGM_PRINTF("recording pipeline, id 0x%08x, start state is EGenericPipelineState_suspended\n", p_pipeline->pipeline_id);
            }

        } else {
            LOGM_ERROR("video and audio path are both disabled?\n");
            return EECode_BadParam;
        }

    } else {
        LOGM_ERROR("BAD state, mpRecordingPipelines %p\n", mpRecordingPipelines);
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV3::setupStreamingPipeline(TComponentIndex index)
{
    if (DUnlikely(index >= mnStreamingPipelinesCurrentNumber)) {
        LOGM_FATAL("BAD index %d, exceed max value %d\n", index, mnStreamingPipelinesCurrentNumber);
        return EECode_InternalLogicalBug;
    }

    if (DUnlikely((NULL == mpStreamingPipelines) || (NULL == mpStreamingContent))) {
        LOGM_FATAL("NULL mpStreamingPipelines %p or mpStreamingContent %p\n", mpStreamingPipelines, mpStreamingContent);
        return EECode_InternalLogicalBug;
    }

    SStreamingPipeline *p_pipeline = mpStreamingPipelines + index;
    SStreamingSessionContent *p_content = mpStreamingContent + index;
    TComponentIndex track_video_input_index = 0, track_audio_input_index = 0;

    DASSERT(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Streaming, index) == p_pipeline->pipeline_id);
    DASSERT(p_pipeline->index == index);
    DASSERT(EGenericPipelineType_Streaming == p_pipeline->type);
    DASSERT(EGenericPipelineState_not_inited == p_pipeline->pipeline_state);
    DASSERT(p_pipeline->audio_enabled || p_pipeline->video_enabled);

    //LOGM_DEBUG("streaming pipeline, %d\n", index);

    if (DUnlikely(0 == isComponentIDValid(p_pipeline->server_id, EGenericComponentType_StreamingServer))) {
        LOGM_ERROR("not valid p_pipeline->server_id %08x\n", p_pipeline->server_id);
        return EECode_BadParam;
    }

    if (DUnlikely(0 == isComponentIDValid(p_pipeline->data_transmiter_id, EGenericComponentType_StreamingTransmiter))) {
        LOGM_ERROR("not valid p_pipeline->data_transmiter_id %08x\n", p_pipeline->data_transmiter_id);
        return EECode_BadParam;
    }

    if (p_pipeline->video_source_id[0]) {
        if (DUnlikely(0 == isComponentIDValid_4(p_pipeline->video_source_id[0], EGenericComponentType_Demuxer, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_CloudConnecterClientAgent, EGenericComponentType_VideoEncoder))) {
            LOGM_ERROR("not valid p_pipeline->video_source_id[0] %08x\n", p_pipeline->video_source_id[0]);
            return EECode_BadParam;
        }
        p_pipeline->video_enabled = 1;
        //LOGM_DEBUG("streaming pipeline, have video source\n");
    } else {
        p_pipeline->video_enabled = 0;
    }

    if (p_pipeline->audio_source_id[0]) {
        if (DUnlikely(0 == isComponentIDValid_3(p_pipeline->audio_source_id[0], EGenericComponentType_Demuxer, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_CloudConnecterClientAgent))) {
            LOGM_ERROR("not valid p_pipeline->audio_source_id[0] %08x\n", p_pipeline->audio_source_id[0]);
            return EECode_BadParam;
        }
        if (p_pipeline->audio_pinmuxer_id) {
            if (DUnlikely(0 == isComponentIDValid(p_pipeline->audio_pinmuxer_id, EGenericComponentType_ConnecterPinMuxer))) {
                LOGM_ERROR("not valid p_pipeline->audio_pinmuxer_id %08x\n", p_pipeline->audio_pinmuxer_id);
                return EECode_BadParam;
            }
        }
        //LOGM_DEBUG("streaming pipeline, have audio source, pin muxer 0x%08x\n", p_pipeline->audio_pinmuxer_id);
        p_pipeline->audio_enabled = 1;
    } else if (p_pipeline->audio_pinmuxer_id) {
        if (DUnlikely(0 == isComponentIDValid(p_pipeline->audio_pinmuxer_id, EGenericComponentType_ConnecterPinMuxer))) {
            LOGM_ERROR("not valid p_pipeline->audio_pinmuxer_id %08x\n", p_pipeline->audio_pinmuxer_id);
            return EECode_BadParam;
        }
        p_pipeline->audio_enabled = 1;
        //LOGM_DEBUG("streaming pipeline, no audio source, but audio pin muxer\n");
    } else {
        p_pipeline->audio_enabled = 0;
        //LOGM_DEBUG("streaming pipeline, no audio source, and no audio pin muxer\n");
    }

    IStreamingServer *p_server = NULL;
    SComponent *p_component = NULL;
    SConnection *p_connection = NULL;

    p_server = findStreamingServer(p_pipeline->server_id);
    if (DUnlikely(!p_server)) {
        LOGM_FATAL("Bad p_pipeline->server_id 0x%08x.\n", p_pipeline->server_id);
        return EECode_BadParam;
    }
    p_component = findComponentFromID(p_pipeline->server_id);
    if (DUnlikely(!p_component)) {
        LOGM_FATAL("NULL server component, or BAD p_pipeline->server_id 0x%08x?\n", p_pipeline->server_id);
        return EECode_BadParam;
    }
    p_pipeline->p_server = p_component;
    p_component->streaming_pipeline_number ++;

    p_component = findComponentFromID(p_pipeline->data_transmiter_id);
    if (DUnlikely(!p_component)) {
        LOGM_FATAL("NULL transmiter component, or BAD p_pipeline->data_transmiter_id 0x%08x?\n", p_pipeline->data_transmiter_id);
        return EECode_BadParam;
    }
    p_pipeline->p_data_transmiter = p_component;
    p_component->streaming_pipeline_number ++;
    if (p_pipeline->config_running_at_startup) {
        p_component->active_count ++;
    }

    if (p_pipeline->video_enabled) {
        p_component = findComponentFromID(p_pipeline->video_source_id[0]);
        if (DUnlikely(!p_component)) {
            LOGM_FATAL("NULL video source component, or BAD p_pipeline->video_source_id[0] 0x%08x?\n", p_pipeline->video_source_id[0]);
            return EECode_BadParam;
        }
        p_pipeline->p_video_source[0] = p_component;
        p_component->streaming_pipeline_number ++;
        if (p_pipeline->config_running_at_startup) {
            p_component->active_count ++;
        }

        p_connection = queryConnection(p_pipeline->video_source_id[0], p_pipeline->data_transmiter_id, StreamType_Video);
        if (DLikely(p_connection)) {
            p_pipeline->video_source_connection_id[0] = p_connection->connection_id;
            p_pipeline->p_video_source_connection[0] = p_connection;
        } else {
            LOGM_ERROR("no connection between p_pipeline->video_source_id[0] %08x and p_pipeline->data_transmiter_id %08x\n", p_pipeline->video_source_id[0], p_pipeline->data_transmiter_id);
            return EECode_InternalLogicalBug;
        }

        track_video_input_index = p_connection->down_pin_index;
    }

    if (p_pipeline->audio_enabled) {

        if (p_pipeline->audio_source_id[0]) {
            p_component = findComponentFromID(p_pipeline->audio_source_id[0]);
            if (DUnlikely(!p_component)) {
                LOGM_FATAL("NULL audio source component, or BAD p_pipeline->audio_source_id[0] 0x%08x?\n", p_pipeline->audio_source_id[0]);
                return EECode_BadParam;
            }
            p_pipeline->p_audio_source[0] = p_component;
            p_component->streaming_pipeline_number ++;
            if (p_pipeline->config_running_at_startup) {
                p_component->active_count ++;
            }
        }

        if (p_pipeline->audio_pinmuxer_id) {
            p_component = findComponentFromID(p_pipeline->audio_pinmuxer_id);
            if (DUnlikely(!p_component)) {
                LOGM_FATAL("NULL audio_pinmuxer_id component, or BAD p_pipeline->audio_pinmuxer_id 0x%08x?\n", p_pipeline->audio_pinmuxer_id);
                return EECode_BadParam;
            }
            p_pipeline->p_audio_pinmuxer = p_component;
            p_component->streaming_pipeline_number ++;
            if (p_pipeline->config_running_at_startup) {
                p_component->active_count ++;
            }

            if (p_pipeline->p_audio_source[0]) {
                p_connection = queryConnection(p_pipeline->audio_source_id[0], p_pipeline->audio_pinmuxer_id, StreamType_Audio);
                if (DLikely(p_connection)) {
                    p_pipeline->audio_pinmuxer_connection_id[0] = p_connection->connection_id;
                    p_pipeline->p_audio_pinmuxer_connection[0] = p_connection;
                } else {
                    LOGM_ERROR("no connection between audio_source_id %08x and pinmuxer %08x\n", p_pipeline->audio_source_id[0], p_pipeline->audio_pinmuxer_id);
                    return EECode_InternalLogicalBug;
                }
            }

            p_connection = queryConnection(p_pipeline->audio_pinmuxer_id, p_pipeline->data_transmiter_id, StreamType_Audio);
            if (DLikely(p_connection)) {
                p_pipeline->audio_source_connection_id[0] = p_connection->connection_id;
                p_pipeline->p_audio_source_connection[0] = p_connection;
            } else {
                LOGM_ERROR("no connection between audio pinmuxer %08x and transmiter %08x\n", p_pipeline->audio_pinmuxer_id, p_pipeline->data_transmiter_id);
                return EECode_InternalLogicalBug;
            }
            track_audio_input_index = p_connection->down_pin_index;
        } else {

            if (DLikely(p_pipeline->p_audio_source[0])) {
                p_connection = queryConnection(p_pipeline->audio_source_id[0], p_pipeline->data_transmiter_id, StreamType_Audio);
                if (DLikely(p_connection)) {
                    p_pipeline->audio_source_connection_id[0] = p_connection->connection_id;
                    p_pipeline->p_audio_source_connection[0] = p_connection;
                } else {
                    LOGM_ERROR("no connection between audio_source_id[0] %08x and transmiter %08x\n", p_pipeline->audio_source_id[0], p_pipeline->data_transmiter_id);
                    return EECode_InternalLogicalBug;
                }
                track_audio_input_index = p_connection->down_pin_index;
            } else {
                LOGM_FATAL("should not comes here\n");
            }
        }

    }

    if (p_pipeline->config_running_at_startup) {
        p_pipeline->pipeline_state = EGenericPipelineState_running;
        LOGM_PRINTF("[Gragh]: streaming pipeline, id 0x%08x, start state is EGenericPipelineState_running\n", p_pipeline->pipeline_id);
    } else {
        p_pipeline->pipeline_state = EGenericPipelineState_suspended;
        LOGM_PRINTF("[Gragh]: streaming pipeline, id 0x%08x, start state is EGenericPipelineState_suspended\n", p_pipeline->pipeline_id);
    }

    p_content->id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_StreamingContent, index);
    p_content->content_index = index;

    p_content->p_server = p_server;

    snprintf(&p_content->session_name[0], DMaxStreamContentUrlLength - 1, "%s", p_pipeline->url);
    p_content->session_name[DMaxStreamContentUrlLength - 1] = 0x0;

    p_content->enabled = 1;
    p_content->sub_session_count = 0;
    p_content->content_type = 0;

    if (DLikely(p_pipeline->p_data_transmiter->p_filter)) {
        EECode reterr = EECode_OK;
        SQueryInterface query;

        if (p_pipeline->video_enabled) {
            query.index = track_video_input_index;
            query.p_pointer = 0;
            query.type = StreamType_Video;
            query.format = StreamFormat_Invalid;
            query.p_agent_pointer = 0;

            reterr = p_pipeline->p_data_transmiter->p_filter->FilterProperty(EFilterPropertyType_streaming_query_subsession_sink, 0, (void *) &query);
            DASSERT_OK(reterr);
            DASSERT(query.p_pointer);
            DASSERT(query.p_agent_pointer);

            p_content->sub_session_content[ESubSession_video] = (SStreamingSubSessionContent *)query.p_agent_pointer;
            DASSERT(query.p_pointer == ((TPointer)p_content->sub_session_content[ESubSession_video]->p_transmiter));
            p_content->sub_session_count ++;
            LOGM_INFO("streaming pipeline(%08x), add video sub session\n", p_pipeline->pipeline_id);
        }

        if (p_pipeline->audio_enabled) {
            query.index = track_audio_input_index;
            query.p_pointer = 0;
            query.type = StreamType_Audio;
            query.format = StreamFormat_Invalid;
            query.p_agent_pointer = 0;

            reterr = p_pipeline->p_data_transmiter->p_filter->FilterProperty(EFilterPropertyType_streaming_query_subsession_sink, 0, (void *) &query);
            DASSERT_OK(reterr);
            DASSERT(query.p_pointer);
            DASSERT(query.p_agent_pointer);

            p_content->sub_session_content[ESubSession_audio] = (SStreamingSubSessionContent *)query.p_agent_pointer;
            DASSERT(query.p_pointer == ((TPointer)p_content->sub_session_content[ESubSession_audio]->p_transmiter));
            p_content->sub_session_count ++;
            LOGM_INFO("streaming pipeline(%08x), add audio sub session\n", p_pipeline->pipeline_id);
        }
    } else {
        LOGM_FATAL("NULL p_pipeline(%08x)->p_data_transmiter->p_filter\n", p_pipeline->pipeline_id);
        return EECode_InternalLogicalBug;
    }

    p_content->has_video = p_pipeline->video_enabled;
    if (p_pipeline->video_enabled) {
        p_content->sub_session_content[ESubSession_video]->enabled = 1;
    }

    p_content->has_audio = p_pipeline->audio_enabled;
    if (p_pipeline->audio_enabled) {
        p_content->sub_session_content[ESubSession_audio]->enabled = 1;
    }

    if (DLikely(mpStreamingServerManager)) {
        LOGM_PRINTF("[Gragh]: pipeline %08x, add streaming content %s, has video %d, has audio %d\n", p_pipeline->pipeline_id, p_content->session_name, p_content->has_video, p_content->has_audio);
        p_content->p_streaming_transmiter_filter = p_pipeline->p_data_transmiter->p_filter;
        if (p_pipeline->p_video_source[0]) {
            p_content->p_video_source_filter = p_pipeline->p_video_source[0]->p_filter;
        }
        if (p_pipeline->p_audio_source[0]) {
            p_content->p_audio_source_filter = p_pipeline->p_audio_source[0]->p_filter;
        }
        mpStreamingServerManager->AddStreamingContent(p_server, p_content);
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV3::setupCloudPipeline(TComponentIndex index)
{
    if (DUnlikely(index >= mnCloudPipelinesCurrentNumber)) {
        LOGM_FATAL("BAD index %d, exceed max value %d\n", index, mnCloudPipelinesCurrentNumber);
        return EECode_InternalLogicalBug;
    }

    if (DUnlikely((NULL == mpCloudPipelines) || (NULL == mpCloudContent))) {
        LOGM_FATAL("NULL mpCloudPipelines %p or mpCloudContent %p\n", mpCloudPipelines, mpCloudContent);
        return EECode_InternalLogicalBug;
    }

    SCloudPipeline *p_pipeline = mpCloudPipelines + index;

    DASSERT(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Cloud, index) == p_pipeline->pipeline_id);
    DASSERT(p_pipeline->index == index);
    DASSERT(EGenericPipelineType_Cloud == p_pipeline->type);
    DASSERT(EGenericPipelineState_not_inited == p_pipeline->pipeline_state);
    DASSERT(p_pipeline->audio_enabled || p_pipeline->video_enabled);

    if (DUnlikely(0 == isComponentIDValid(p_pipeline->server_id, EGenericComponentType_CloudServer))) {
        LOGM_ERROR("not valid p_pipeline->server_id %08x\n", p_pipeline->server_id);
        return EECode_BadParam;
    }

    if (DUnlikely(0 == isComponentIDValid_3(p_pipeline->agent_id, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_CloudConnecterClientAgent, EGenericComponentType_CloudConnecterCmdAgent))) {
        LOGM_ERROR("not valid p_pipeline->agent_id %08x\n", p_pipeline->agent_id);
        return EECode_BadParam;
    }

    if (p_pipeline->video_sink_id[0]) {
        if (DUnlikely(0 == isComponentIDValid_3(p_pipeline->video_sink_id[0], EGenericComponentType_Muxer, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_StreamingTransmiter))) {
            LOGM_ERROR("not valid p_pipeline->video_sink_id[0] %08x\n", p_pipeline->video_sink_id[0]);
            return EECode_BadParam;
        }
        p_pipeline->video_enabled = 1;
    } else {
        p_pipeline->video_enabled = 0;
    }

    if (p_pipeline->audio_sink_id[0]) {
        if (DUnlikely(0 == isComponentIDValid_3(p_pipeline->audio_sink_id[0], EGenericComponentType_Muxer, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_StreamingTransmiter))) {
            LOGM_ERROR("not valid p_pipeline->audio_sink_id[0] %08x\n", p_pipeline->audio_sink_id[0]);
            return EECode_BadParam;
        }
        p_pipeline->audio_enabled = 1;
    } else {
        p_pipeline->audio_enabled = 0;
    }

    ICloudServer *p_server;
    SComponent *p_component;
    SConnection *p_connection;

    p_server = findCloudServer(p_pipeline->server_id);
    if (DUnlikely(!p_server)) {
        LOGM_FATAL("Bad p_pipeline->server_id 0x%08x.\n", p_pipeline->server_id);
        return EECode_BadParam;
    }

    p_component = findComponentFromID(p_pipeline->agent_id);
    if (DUnlikely(!p_component)) {
        LOGM_FATAL("NULL agent component, or BAD agent_id id 0x%08x?\n", p_pipeline->agent_id);
        return EECode_BadParam;
    } else if (DUnlikely(!p_component->p_filter)) {
        LOGM_FATAL("NULL agent component->p_filter, or BAD agent_id id 0x%08x?\n", p_pipeline->agent_id);
        return EECode_BadParam;
    }
    p_pipeline->p_agent = p_component;
    p_component->cloud_pipeline_number ++;
    if (p_pipeline->config_running_at_startup) {
        p_component->active_count ++;
    }

    if (p_pipeline->video_enabled) {
        p_component = findComponentFromID(p_pipeline->video_sink_id[0]);
        if (DUnlikely(!p_component)) {
            LOGM_FATAL("NULL video sink component, or BAD p_pipeline->video_sink_id[0] 0x%08x?\n", p_pipeline->video_sink_id[0]);
            return EECode_BadParam;
        }
        p_pipeline->p_video_sink[0] = p_component;
        p_component->cloud_pipeline_number ++;
        if (p_pipeline->config_running_at_startup) {
            p_component->active_count ++;
        }

        p_connection = queryConnection(p_pipeline->agent_id, p_pipeline->video_sink_id[0], StreamType_Video);
        if (DLikely(p_connection)) {
            p_pipeline->video_sink_connection_id[0] = p_connection->connection_id;
            p_pipeline->p_video_sink_connection[0] = p_connection;
        } else {
            LOGM_ERROR("no connection between p_pipeline->agent_id %08x and p_pipeline->video_sink_id[0] %08x\n", p_pipeline->agent_id, p_pipeline->video_sink_id[0]);
            return EECode_InternalLogicalBug;
        }
    }

    if (p_pipeline->audio_enabled) {
        p_component = findComponentFromID(p_pipeline->audio_sink_id[0]);
        if (DUnlikely(!p_component)) {
            LOGM_FATAL("NULL audio source component, or BAD p_pipeline->audio_sink_id[0] 0x%08x?\n", p_pipeline->audio_sink_id[0]);
            return EECode_BadParam;
        }
        p_pipeline->p_audio_sink[0] = p_component;
        p_component->cloud_pipeline_number ++;
        if (p_pipeline->config_running_at_startup) {
            p_component->active_count ++;
        }

        p_connection = queryConnection(p_pipeline->agent_id, p_pipeline->audio_sink_id[0], StreamType_Audio);
        if (DLikely(p_connection)) {
            p_pipeline->audio_sink_connection_id[0] = p_connection->connection_id;
            p_pipeline->p_audio_sink_connection[0] = p_connection;
        } else {
            LOGM_ERROR("no connection between agent_id %08x audio_sink_id[0] %08x\n", p_pipeline->agent_id, p_pipeline->audio_sink_id[0]);
            return EECode_InternalLogicalBug;
        }
    }

    if (p_pipeline->config_running_at_startup) {
        p_pipeline->pipeline_state = EGenericPipelineState_running;
        LOGM_PRINTF("[Gragh]: cloud pipeline, id 0x%08x, start state is EGenericPipelineState_running\n", p_pipeline->pipeline_id);
    } else {
        p_pipeline->pipeline_state = EGenericPipelineState_suspended;
        LOGM_PRINTF("[Gragh]: cloud pipeline, id 0x%08x, start state is EGenericPipelineState_suspended\n", p_pipeline->pipeline_id);
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV3::setupNativeCapturePipeline(TComponentIndex index)
{
    if (DUnlikely(index >= mnNativeCapturePipelinesCurrentNumber)) {
        LOGM_FATAL("BAD index %d, exceed max value %d\n", index, mnNativeCapturePipelinesCurrentNumber);
        return EECode_InternalLogicalBug;
    }

    if (DUnlikely(NULL == mpNativeCapturePipelines)) {
        LOGM_FATAL("NULL mpNativeCapturePipelines %p\n", mpNativeCapturePipelines);
        return EECode_InternalLogicalBug;
    }

    SNativeCapturePipeline *p_pipeline = mpNativeCapturePipelines + index;

    DASSERT(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_NativeCapture, index) == p_pipeline->pipeline_id);
    DASSERT(p_pipeline->index == index);
    DASSERT(EGenericPipelineType_NativeCapture == p_pipeline->type);
    DASSERT(EGenericPipelineState_not_inited == p_pipeline->pipeline_state);
    DASSERT(p_pipeline->audio_enabled || p_pipeline->video_enabled);

    if (p_pipeline->video_capture_id) {
        if (DUnlikely(0 == isComponentIDValid(p_pipeline->video_capture_id, EGenericComponentType_VideoCapture))) {
            LOGM_ERROR("not valid p_pipeline->video_capture_id %08x\n", p_pipeline->video_capture_id);
            return EECode_BadParam;
        }
        p_pipeline->video_enabled = 1;
    } else {
        p_pipeline->video_enabled = 0;
    }

    if (p_pipeline->audio_capture_id) {
        if (DUnlikely(0 == isComponentIDValid(p_pipeline->audio_capture_id, EGenericComponentType_AudioCapture))) {
            LOGM_ERROR("not valid p_pipeline->audio_capture_id %08x\n", p_pipeline->audio_capture_id);
            return EECode_BadParam;
        }
        p_pipeline->audio_enabled = 1;
    } else {
        p_pipeline->audio_enabled = 0;
    }

    SComponent *p_component = NULL;

    if (p_pipeline->video_enabled) {
        if (p_pipeline->video_capture_id) {
            p_component = findComponentFromID(p_pipeline->video_capture_id);
            if (DUnlikely(!p_component)) {
                LOGM_FATAL("NULL video capture component, or BAD p_pipeline->video_capture_id 0x%08x?\n", p_pipeline->video_capture_id);
                return EECode_BadParam;
            }
            p_pipeline->p_video_capture = p_component;
            p_component->native_capture_pipeline_number ++;
            if (p_pipeline->config_running_at_startup) {
                p_component->active_count ++;
            }
        }

        if (p_pipeline->video_encoder_id) {
            p_component = findComponentFromID(p_pipeline->video_encoder_id);
            if (DUnlikely(!p_component)) {
                LOGM_FATAL("NULL video encoder component, or BAD p_pipeline->video_encoder_id 0x%08x?\n", p_pipeline->video_encoder_id);
                return EECode_BadParam;
            }
            p_pipeline->p_video_encoder = p_component;
            p_component->native_capture_pipeline_number ++;
            if (p_pipeline->config_running_at_startup) {
                p_component->active_count ++;
            }
        }
    }

    if (p_pipeline->audio_enabled) {
        if (p_pipeline->audio_capture_id) {
            p_component = findComponentFromID(p_pipeline->audio_capture_id);
            if (DUnlikely(!p_component)) {
                LOGM_FATAL("NULL audio capture component, or BAD p_pipeline->audio_capture_id 0x%08x?\n", p_pipeline->audio_capture_id);
                return EECode_BadParam;
            }
            p_pipeline->p_audio_capture = p_component;
            p_component->native_capture_pipeline_number ++;
            if (p_pipeline->config_running_at_startup) {
                p_component->active_count ++;
            }
        } else {
            LOGM_WARN("no audio capture\n");
        }

        if (p_pipeline->audio_encoder_id) {
            p_component = findComponentFromID(p_pipeline->audio_encoder_id);
            if (DUnlikely(!p_component)) {
                LOGM_FATAL("NULL audio encoder component, or BAD p_pipeline->audio_encoder_id 0x%08x?\n", p_pipeline->audio_encoder_id);
                return EECode_BadParam;
            }
            p_pipeline->p_audio_encoder = p_component;
            p_component->native_capture_pipeline_number ++;
            if (p_pipeline->config_running_at_startup) {
                p_component->active_count ++;
            }
        }
    }

    if (p_pipeline->config_running_at_startup) {
        p_pipeline->pipeline_state = EGenericPipelineState_running;
        LOGM_PRINTF("[Gragh]: native capture pipeline, id 0x%08x, start state is EGenericPipelineState_running\n", p_pipeline->pipeline_id);
    } else {
        p_pipeline->pipeline_state = EGenericPipelineState_suspended;
        LOGM_PRINTF("[Gragh]: native capture pipeline, id 0x%08x, start state is EGenericPipelineState_suspended\n", p_pipeline->pipeline_id);
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV3::setupNativePushPipeline(TComponentIndex index)
{
    if (DUnlikely(index >= mnNativePushPipelinesCurrentNumber)) {
        LOGM_FATAL("BAD index %d, exceed max value %d\n", index, mnNativePushPipelinesCurrentNumber);
        return EECode_InternalLogicalBug;
    }

    if (DUnlikely(NULL == mpNativePushPipelines)) {
        LOGM_FATAL("NULL mpNativePushPipelines %p\n", mpNativePushPipelines);
        return EECode_InternalLogicalBug;
    }

    SNativePushPipeline *p_pipeline = mpNativePushPipelines + index;

    DASSERT(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_NativePush, index) == p_pipeline->pipeline_id);
    DASSERT(p_pipeline->index == index);
    DASSERT(EGenericPipelineType_NativePush == p_pipeline->type);
    DASSERT(EGenericPipelineState_not_inited == p_pipeline->pipeline_state);
    DASSERT(p_pipeline->audio_enabled || p_pipeline->video_enabled);

    if (p_pipeline->push_source_id) {
        if (DUnlikely(0 == isComponentIDValid_4(p_pipeline->push_source_id, EGenericComponentType_VideoCapture, EGenericComponentType_AudioCapture, EGenericComponentType_VideoEncoder, EGenericComponentType_AudioEncoder))) {
            LOGM_ERROR("not valid p_pipeline->push_source_id %08x\n", p_pipeline->push_source_id);
            return EECode_BadParam;
        }
    } else {
        LOGM_FATAL("no push_source_id, index %d\n", index);
        return EECode_InternalLogicalBug;
    }

    if (p_pipeline->video_sink_id) {
        if (DUnlikely(0 == isComponentIDValid(p_pipeline->video_sink_id, EGenericComponentType_CloudConnecterServerAgent))) {
            LOGM_ERROR("not valid p_pipeline->video_sink_id %08x\n", p_pipeline->video_sink_id);
            return EECode_BadParam;
        }
        p_pipeline->video_enabled = 1;
    } else {
        p_pipeline->video_enabled = 0;
    }

    if (p_pipeline->audio_sink_id) {
        if (DUnlikely(0 == isComponentIDValid(p_pipeline->audio_sink_id, EGenericComponentType_CloudConnecterServerAgent))) {
            LOGM_ERROR("not valid p_pipeline->audio_sink_id %08x\n", p_pipeline->audio_sink_id);
            return EECode_BadParam;
        }
        p_pipeline->audio_enabled = 1;
    } else {
        p_pipeline->audio_enabled = 0;
    }

    SComponent *p_component = NULL;
    SConnection *p_connection = NULL;

    if (p_pipeline->video_enabled) {
        if (p_pipeline->video_sink_id) {
            p_component = findComponentFromID(p_pipeline->video_sink_id);
            if (DUnlikely(!p_component)) {
                LOGM_FATAL("NULL video sink component, or BAD p_pipeline->video_sink_id 0x%08x?\n", p_pipeline->video_sink_id);
                return EECode_BadParam;
            }
            p_pipeline->p_video_sink = p_component;
            p_component->native_push_pipeline_number ++;
            if (p_pipeline->config_running_at_startup) {
                p_component->active_count ++;
            }
        }

        p_connection = queryConnection(p_pipeline->push_source_id, p_pipeline->video_sink_id, StreamType_Video);
        if (DLikely(p_connection)) {
            p_pipeline->video_sink_connection_id = p_connection->connection_id;
            p_pipeline->p_video_sink_connection = p_connection;
        } else {
            LOGM_ERROR("no connection between p_pipeline->push_source_id %08x and p_pipeline->video_sink_id %08x\n", p_pipeline->push_source_id, p_pipeline->video_sink_id);
            return EECode_InternalLogicalBug;
        }

    }

    if (p_pipeline->audio_enabled) {
        if (p_pipeline->audio_sink_id) {
            p_component = findComponentFromID(p_pipeline->audio_sink_id);
            if (DUnlikely(!p_component)) {
                LOGM_FATAL("NULL audio sink component, or BAD p_pipeline->audio_sink_id 0x%08x?\n", p_pipeline->audio_sink_id);
                return EECode_BadParam;
            }
            p_pipeline->p_audio_sink = p_component;
            p_component->native_push_pipeline_number ++;
            if (p_pipeline->config_running_at_startup) {
                p_component->active_count ++;
            }
        } else {
            LOGM_WARN("no audio sink\n");
        }

        p_connection = queryConnection(p_pipeline->push_source_id, p_pipeline->audio_sink_id, StreamType_Audio);
        if (DLikely(p_connection)) {
            p_pipeline->audio_sink_connection_id = p_connection->connection_id;
            p_pipeline->p_audio_sink_connection = p_connection;
        } else {
            LOGM_ERROR("no connection between p_pipeline->push_source_id %08x and p_pipeline->audio_sink_id %08x\n", p_pipeline->push_source_id, p_pipeline->audio_sink_id);
            return EECode_InternalLogicalBug;
        }
    }

    if (p_pipeline->config_running_at_startup) {
        p_pipeline->pipeline_state = EGenericPipelineState_running;
        LOGM_PRINTF("[Gragh]: native capture pipeline, id 0x%08x, start state is EGenericPipelineState_running\n", p_pipeline->pipeline_id);
    } else {
        p_pipeline->pipeline_state = EGenericPipelineState_suspended;
        LOGM_PRINTF("[Gragh]: native capture pipeline, id 0x%08x, start state is EGenericPipelineState_suspended\n", p_pipeline->pipeline_id);
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV3::setupCloudUploadPipeline(TComponentIndex index)
{
    if (DUnlikely(index >= mnCloudUploadPipelinesCurrentNumber)) {
        LOGM_FATAL("BAD index %d, exceed max value %d\n", index, mnCloudUploadPipelinesCurrentNumber);
        return EECode_InternalLogicalBug;
    }

    if (DUnlikely(NULL == mpCloudUploadPipelines)) {
        LOGM_FATAL("NULL mpCloudUploadPipelines %p\n", mpCloudUploadPipelines);
        return EECode_InternalLogicalBug;
    }

    SCloudUploadPipeline *p_pipeline = mpCloudUploadPipelines + index;
    SComponent *p_component = NULL;
    SConnection *p_connection = NULL;

    if (DUnlikely((!p_pipeline->audio_enabled) && (!p_pipeline->video_enabled))) {
        LOGM_FATAL("no audio_source_id, index %d\n", index);
        return EECode_InternalLogicalBug;
    }

    DASSERT(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_CloudUpload, index) == p_pipeline->pipeline_id);
    DASSERT(p_pipeline->index);
    DASSERT(EGenericPipelineType_CloudUpload == p_pipeline->type);
    DASSERT(EGenericPipelineState_not_inited == p_pipeline->pipeline_state);

    if (DLikely(p_pipeline->uploader_id)) {
        if (DUnlikely(0 == isComponentIDValid(p_pipeline->uploader_id, EGenericComponentType_CloudConnecterClientAgent))) {
            LOGM_ERROR("not valid p_pipeline->uploader_id %08x\n", p_pipeline->uploader_id);
            return EECode_BadParam;
        }

        p_component = findComponentFromID(p_pipeline->uploader_id);
        if (DUnlikely(!p_component)) {
            LOGM_FATAL("NULL uploader component, or BAD p_pipeline->uploader_id 0x%08x?\n", p_pipeline->uploader_id);
            return EECode_BadParam;
        }
        p_pipeline->p_uploader = p_component;
    } else {
        LOGM_FATAL("no uploader_id, index %d\n", index);
        return EECode_InternalLogicalBug;
    }

    if (p_pipeline->video_enabled) {
        if (DLikely(p_pipeline->video_source_id)) {
            if (DUnlikely(0 == isComponentIDValid(p_pipeline->video_source_id, EGenericComponentType_VideoEncoder))) {
                LOGM_ERROR("not valid p_pipeline->video_source_id %08x\n", p_pipeline->video_source_id);
                return EECode_BadParam;
            }

            p_component = findComponentFromID(p_pipeline->video_source_id);
            if (DUnlikely(!p_component)) {
                LOGM_FATAL("NULL video source component, or BAD p_pipeline->video_source_id 0x%08x?\n", p_pipeline->video_source_id);
                return EECode_BadParam;
            }
            p_pipeline->p_video_source = p_component;
            if (p_pipeline->config_running_at_startup) {
                p_component->active_count ++;
            }

            p_connection = queryConnection(p_pipeline->video_source_id, p_pipeline->uploader_id, StreamType_Video);
            if (DLikely(p_connection)) {
                p_pipeline->video_source_connection_id = p_connection->connection_id;
                p_pipeline->p_video_source_connection = p_connection;
            } else {
                LOGM_ERROR("no connection between p_pipeline->video_source_id %08x and p_pipeline->uploader_id %08x\n", p_pipeline->video_source_id, p_pipeline->uploader_id);
                return EECode_InternalLogicalBug;
            }

        } else {
            LOGM_FATAL("no video_source_id, index %d\n", index);
            return EECode_InternalLogicalBug;
        }
    }

    if (p_pipeline->audio_enabled) {
        if (DLikely(p_pipeline->audio_source_id)) {
            if (DUnlikely(0 == isComponentIDValid(p_pipeline->audio_source_id, EGenericComponentType_AudioEncoder))) {
                LOGM_ERROR("not valid p_pipeline->audio_source_id %08x\n", p_pipeline->audio_source_id);
                return EECode_BadParam;
            }

            p_component = findComponentFromID(p_pipeline->audio_source_id);
            if (DUnlikely(!p_component)) {
                LOGM_FATAL("NULL audio source component, or BAD p_pipeline->audio_source_id 0x%08x?\n", p_pipeline->audio_source_id);
                return EECode_BadParam;
            }
            p_pipeline->p_audio_source = p_component;
            if (p_pipeline->config_running_at_startup) {
                p_component->active_count ++;
            }

            p_connection = queryConnection(p_pipeline->audio_source_id, p_pipeline->uploader_id, StreamType_Audio);
            if (DLikely(p_connection)) {
                p_pipeline->audio_source_connection_id = p_connection->connection_id;
                p_pipeline->p_audio_source_connection = p_connection;
            } else {
                LOGM_ERROR("no connection between p_pipeline->audio_source_id %08x and p_pipeline->uploader_id %08x\n", p_pipeline->audio_source_id, p_pipeline->uploader_id);
                return EECode_InternalLogicalBug;
            }
        } else {
            LOGM_FATAL("no audio_source_id, index %d\n", index);
            return EECode_InternalLogicalBug;
        }
    }

    if (p_pipeline->config_running_at_startup) {
        p_pipeline->pipeline_state = EGenericPipelineState_running;
        LOGM_PRINTF("[Gragh]: uploader pipeline, id 0x%08x, start state is EGenericPipelineState_running\n", p_pipeline->pipeline_id);
    } else {
        p_pipeline->pipeline_state = EGenericPipelineState_suspended;
        LOGM_PRINTF("[Gragh]: uploader pipeline, id 0x%08x, start state is EGenericPipelineState_suspended\n", p_pipeline->pipeline_id);
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV3::setupVODPipeline(TComponentIndex index)
{
    if (DUnlikely(index >= mnVODPipelinesCurrentNumber)) {
        LOGM_FATAL("BAD index %d, exceed max value %d\n", index, mnVODPipelinesCurrentNumber);
        return EECode_InternalLogicalBug;
    }

    if (DUnlikely((NULL == mpVODPipelines) || (NULL == mpStreamingContent))) {
        LOGM_FATAL("NULL mpVODPipelines %p or mpStreamingContent %p\n", mpVODPipelines, mpStreamingContent);
        return EECode_InternalLogicalBug;
    }

    SVODPipeline *p_pipeline = mpVODPipelines + index;
    //TComponentIndex contentIndex = mnStreamingPipelinesMaxNumber +index;
    //SStreamingSessionContent* p_content = mpVodContent + index;
    TComponentIndex track_video_input_index = 0, track_audio_input_index = 0;

    DASSERT(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_VOD, index) == p_pipeline->pipeline_id);
    DASSERT(p_pipeline->index == index);
    DASSERT(EGenericPipelineType_VOD == p_pipeline->type);
    DASSERT(EGenericPipelineState_not_inited == p_pipeline->pipeline_state);
    DASSERT(p_pipeline->audio_enabled || p_pipeline->video_enabled);

    if (DUnlikely(0 == isComponentIDValid(p_pipeline->server_id, EGenericComponentType_StreamingServer))) {
        LOGM_ERROR("not valid p_pipeline->server_id %08x\n", p_pipeline->server_id);
        return EECode_BadParam;
    }

    if (DUnlikely(0 == isComponentIDValid(p_pipeline->data_transmiter_id, EGenericComponentType_StreamingTransmiter))) {
        LOGM_ERROR("not valid p_pipeline->data_transmiter_id %08x\n", p_pipeline->data_transmiter_id);
        return EECode_BadParam;
    }

    IStreamingServer *p_server = NULL;
    SComponent *p_component = NULL;
    SConnection *p_connection = NULL;

    p_server = findStreamingServer(p_pipeline->server_id);
    if (DUnlikely(!p_server)) {
        LOGM_FATAL("Bad p_pipeline->server_id 0x%08x.\n", p_pipeline->server_id);
        return EECode_BadParam;
    }
    p_component = findComponentFromID(p_pipeline->server_id);
    if (DUnlikely(!p_component)) {
        LOGM_FATAL("NULL server component, or BAD p_pipeline->server_id 0x%08x?\n", p_pipeline->server_id);
        return EECode_BadParam;
    }
    p_pipeline->p_server = p_component;
    p_component->streaming_pipeline_number ++;//attention please N

    p_component = findComponentFromID(p_pipeline->data_transmiter_id);
    if (DUnlikely(!p_component)) {
        LOGM_FATAL("NULL transmiter component, or BAD p_pipeline->data_transmiter_id 0x%08x?\n", p_pipeline->data_transmiter_id);
        return EECode_BadParam;
    }
    p_pipeline->p_data_transmiter = p_component;
    p_component->streaming_pipeline_number ++;//attention please N/2
    if (p_pipeline->config_running_at_startup) {
        p_component->active_count++;
    }

    if (p_pipeline->video_enabled) {
        p_component = findComponentFromID(p_pipeline->flow_controller_id);
        if (DUnlikely(!p_component)) {
            LOGM_FATAL("NULL flow controller component\n");
            return EECode_BadParam;
        }
        p_pipeline->p_flow_controller = p_component;
        p_component->streaming_pipeline_number++;//attention please N/2
        p_connection = queryConnection(p_pipeline->flow_controller_id, p_pipeline->data_transmiter_id, StreamType_Video);
        if (DLikely(p_connection)) {
            p_pipeline->video_flow_controller_connection_id[0] = p_connection->connection_id;
            p_pipeline->p_video_flow_controller_connection[0] = p_connection;
        } else {
            LOGM_ERROR("no connection between p_pipeline->p_flow_controller[0] and p_pipeline->p_data_transmiter\n");
            return EECode_InternalLogicalBug;
        }
        if (p_pipeline->config_running_at_startup) {
            p_component->active_count++;
        }

        p_component = findComponentFromID(p_pipeline->video_source_id[0]);
        if (DUnlikely(!p_component)) {
            LOGM_FATAL("NULL video source component, or BAD p_pipeline->video_source_id 0x%08x?\n", p_pipeline->video_source_id[0]);
            return EECode_BadParam;
        }
        p_component->url_buffer_size = 10;
        p_component->url = (TChar *)DDBG_MALLOC(10, "E3VU");
        snprintf(p_component->url, 10, "%s", "ts");
        p_pipeline->p_video_source[0] = p_component;
        p_component->streaming_pipeline_number ++;//attention please N/2
        if (p_pipeline->config_running_at_startup) {
            p_component->active_count++;
        }

        p_connection = queryConnection(p_pipeline->video_source_id[0], p_pipeline->flow_controller_id, StreamType_Video);
        if (DLikely(p_connection)) {
            p_pipeline->video_source_connection_id[0] = p_connection->connection_id;
            p_pipeline->p_video_source_connection[0] = p_connection;
        } else {
            LOGM_ERROR("no connection between p_pipeline->video_source_id[0] %08x and p_pipeline->flow_controller_id[0] %08x\n", p_pipeline->video_source_id[0], p_pipeline->flow_controller_id);
            return EECode_InternalLogicalBug;
        }

        track_video_input_index = p_connection->down_pin_index;
    }

    if (p_pipeline->audio_enabled) {
        p_component = findComponentFromID(p_pipeline->flow_controller_id);
        if (DUnlikely(!p_component)) {
            LOGM_FATAL("NULL flow controller component\n");
            return EECode_BadParam;
        }
        p_pipeline->p_flow_controller = p_component;
        p_component->streaming_pipeline_number++;//attention please N/2
        p_connection = queryConnection(p_pipeline->flow_controller_id, p_pipeline->data_transmiter_id, StreamType_Audio);
        if (DLikely(p_connection)) {
            p_pipeline->audio_flow_controller_connection_id[0] = p_connection->connection_id;
            p_pipeline->p_audio_flow_controller_connection[0] = p_connection;
        } else {
            LOGM_ERROR("no connection between p_pipeline->p_flow_controller[0] and p_pipeline->p_data_transmiter\n");
            return EECode_InternalLogicalBug;
        }
        if (p_pipeline->config_running_at_startup) {
            p_component->active_count++;
        }

        p_component = findComponentFromID(p_pipeline->audio_source_id[0]);
        if (DUnlikely(!p_component)) {
            LOGM_FATAL("NULL audio source component, or BAD p_pipeline->audio_source_id[0] 0x%08x?\n", p_pipeline->audio_source_id[0]);
            return EECode_BadParam;
        }
        p_pipeline->p_audio_source[0] = p_component;
        p_component->streaming_pipeline_number ++;//attention please N/2
        if (p_pipeline->config_running_at_startup) {
            p_component->active_count++;
        }
        p_connection = queryConnection(p_pipeline->audio_source_id[0], p_pipeline->flow_controller_id, StreamType_Audio);
        if (DLikely(p_connection)) {
            p_pipeline->audio_source_connection_id[0] = p_connection->connection_id;
            p_pipeline->p_audio_source_connection[0] = p_connection;
        } else {
            return EECode_InternalLogicalBug;
        }

        track_audio_input_index = p_connection->down_pin_index;
    }

    if (p_pipeline->config_running_at_startup) {
        p_pipeline->pipeline_state = EGenericPipelineState_running;
        LOGM_PRINTF("[Gragh]: vod pipeline, id 0x%08x, start state is EGenericPipelineState_running, v_index %d, a_index %d\n", p_pipeline->pipeline_id, track_video_input_index, track_audio_input_index);
    } else {
        p_pipeline->pipeline_state = EGenericPipelineState_suspended;
        LOGM_PRINTF("[Gragh]: vod pipeline, id 0x%08x, start state is EGenericPipelineState_suspended\n", p_pipeline->pipeline_id);
    }
#if 0
    p_content->id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_StreamingContent, contentIndex);
    p_content->content_index = contentIndex;
    p_content->p_server = p_server;
    snprintf(&p_content->session_name[0], DMaxStreamContentUrlLength - 1, "%s", p_pipeline->url);
    p_content->session_name[DMaxStreamContentUrlLength - 1] = '\0';
    p_content->enabled = 1;
    p_content->sub_session_count = 0;

    if (DLikely(p_pipeline->p_data_transmiter->p_filter)) {
        EECode ret = EECode_OK;
        SQueryInterface query;

        if (p_pipeline->video_enabled) {
            query.index = track_video_input_index;
            query.p_pointer = 0;
            query.type = StreamType_Video;
            query.format = StreamFormat_Invalid;
            query.p_agent_pointer = 0;

            ret = p_pipeline->p_data_transmiter->p_filter->FilterProperty(EFilterPropertyType_streaming_query_subsession_sink, 0, (void *) &query);
            DASSERT_OK(ret);
            DASSERT(query.p_pointer);
            DASSERT(query.p_agent_pointer);

            p_content->sub_session_content[ESubSession_video] = (SStreamingSubSessionContent *)query.p_agent_pointer;
            DASSERT(query.p_pointer == ((TPointer)p_content->sub_session_content[ESubSession_video]->p_transmiter));
            p_content->sub_session_count ++;
            LOGM_INFO("vod pipeline(%08x), add video sub session\n", p_pipeline->pipeline_id);
        }

        if (p_pipeline->audio_enabled) {
            query.index = track_audio_input_index;
            query.p_pointer = 0;
            query.type = StreamType_Audio;
            query.format = StreamFormat_Invalid;
            query.p_agent_pointer = 0;

            ret = p_pipeline->p_data_transmiter->p_filter->FilterProperty(EFilterPropertyType_streaming_query_subsession_sink, 0, (void *) &query);
            DASSERT_OK(ret);
            DASSERT(query.p_pointer);
            DASSERT(query.p_agent_pointer);

            p_content->sub_session_content[ESubSession_audio] = (SStreamingSubSessionContent *)query.p_agent_pointer;
            DASSERT(query.p_pointer == ((TPointer)p_content->sub_session_content[ESubSession_audio]->p_transmiter));
            p_content->sub_session_count ++;
            LOGM_INFO("vod pipeline(%08x), add audio sub session\n", p_pipeline->pipeline_id);
        }
    } else {
        LOGM_FATAL("NULL p_pipeline(%08x)->p_data_transmiter->p_filter\n", p_pipeline->pipeline_id);
        return EECode_InternalLogicalBug;
    }

    p_content->has_video = p_pipeline->video_enabled;
    if (p_pipeline->video_enabled) {
        p_content->sub_session_content[ESubSession_video]->enabled = 1;
    }

    p_content->has_audio = p_pipeline->audio_enabled;
    if (p_pipeline->audio_enabled) {
        p_content->sub_session_content[ESubSession_audio]->enabled = 1;
    }

    if (DLikely(mpStreamingServerManager)) {
        //LOGM_PRINTF("[Gragh]: pipeline %08x, add streaming content %s, has video %d, has audio %d\n", p_pipeline->pipeline_id, p_content->session_name, p_content->has_video, p_content->has_audio);
        //p_content->p_streaming_transmiter_filter = p_pipeline->p_data_transmiter->p_filter;
        //p_content->p_flow_controller_filter = p_pipeline->p_flow_controller[0]->p_filter;
        //p_content->p_demuxer_filter = (p_pipeline->p_video_source[0])? p_pipeline->p_video_source[0]->p_filter : NULL;
        mpStreamingServerManager->AddStreamingContent(p_server, p_content);
    }
#endif
    return EECode_OK;
}

EECode CActiveGenericEngineV3::setupCloudServerAgent(TGenericID agent_id)
{
    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(agent_id);

    if (DUnlikely(!isComponentIDValid(agent_id, EGenericComponentType_CloudConnecterServerAgent))) {
        LOGM_FATAL("BAD server agent id 0x%08x\n", agent_id);
        return EECode_InternalLogicalBug;
    }

    if (DUnlikely(index >= mnComponentFiltersNumbers[EGenericComponentType_CloudConnecterServerAgent])) {
        LOGM_FATAL("BAD index %d, exceed max value %d\n", index, mnComponentFiltersNumbers[EGenericComponentType_CloudConnecterServerAgent]);
        return EECode_InternalLogicalBug;
    }

    if (NULL == mpCloudContent) {
        LOGM_FATAL("NULL mpCloudContent %p\n", mpCloudContent);
        return EECode_InternalLogicalBug;
    }

    SCloudContent *p_content = mpCloudContent + index;
    SComponent *p_component = NULL;

    ICloudServer *p_server = p_content->p_server;
    if (DUnlikely(!p_server)) {
        LOGM_FATAL("NULL p_server, index %d, p_content %p, p_server %p.\n", index, p_content, p_server);
        return EECode_BadParam;
    }

    p_component = findComponentFromID(agent_id);
    if (DUnlikely(!p_component)) {
        LOGM_FATAL("NULL agent component, or BAD agent_id id 0x%08x?\n", agent_id);
        return EECode_BadParam;
    } else if (DUnlikely(!p_component->p_filter)) {
        LOGM_FATAL("NULL agent component->p_filter, or BAD agent_id id 0x%08x?\n", agent_id);
        return EECode_BadParam;
    }

    p_content->id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_CloudReceiverContent, index);
    p_content->content_index = index;

    p_content->p_server = p_server;
    p_content->p_cloud_agent_filter = p_component->p_filter;

    //DASSERT(p_component->url);
    if (p_component->url) {
        snprintf(&p_content->content_name[0], DMaxCloudAgentUrlLength - 1, "%s", p_component->url);
        p_content->content_name[DMaxCloudAgentUrlLength - 1] = 0x0;
    } else {
        LOGM_NOTICE("NULL cloud agent url, agent id %08x\n", agent_id);
    }

    p_content->enabled = 1;

    if (DLikely(mpCloudServerManager)) {
        LOGM_INFO("add cloud receiver content %s\n", p_content->content_name);
        mpCloudServerManager->AddCloudContent(p_server, p_content);
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV3::setupCloudCmdAgent(TGenericID agent_id)
{
    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(agent_id);

    if (DUnlikely(!isComponentIDValid(agent_id, EGenericComponentType_CloudConnecterCmdAgent))) {
        LOGM_FATAL("BAD cmd agent id 0x%08x\n", agent_id);
        return EECode_InternalLogicalBug;
    }

    if (DUnlikely(index >= mnComponentFiltersNumbers[EGenericComponentType_CloudConnecterCmdAgent])) {
        LOGM_FATAL("BAD index %d, exceed max value %d\n", index, mnComponentFiltersNumbers[EGenericComponentType_CloudConnecterCmdAgent]);
        return EECode_InternalLogicalBug;
    }

    if (NULL == mpCloudCmdContent) {
        LOGM_FATAL("NULL mpCloudCmdContent %p\n", mpCloudCmdContent);
        return EECode_InternalLogicalBug;
    }

    SCloudContent *p_content = mpCloudCmdContent + index;
    SComponent *p_component = NULL;

    ICloudServer *p_server = p_content->p_server;
    if (DUnlikely(!p_server)) {
        LOGM_FATAL("NULL p_server, index %d, p_content %p, p_server %p.\n", index, p_content, p_server);
        return EECode_BadParam;
    }

    p_component = findComponentFromID(agent_id);
    if (DUnlikely(!p_component)) {
        LOGM_FATAL("NULL agent component, or BAD agent_id id 0x%08x?\n", agent_id);
        return EECode_BadParam;
    } else if (DUnlikely(!p_component->p_filter)) {
        LOGM_FATAL("NULL agent component->p_filter, or BAD agent_id id 0x%08x?\n", agent_id);
        return EECode_BadParam;
    }

    p_content->id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_CloudReceiverContent, index);
    p_content->content_index = index;

    p_content->p_server = p_server;
    p_content->p_cloud_agent_filter = p_component->p_filter;

    //DASSERT(p_component->url);
    if (p_component->url) {
        snprintf(&p_content->content_name[0], DMaxCloudAgentUrlLength - 1, "%s", p_component->url);
        p_content->content_name[DMaxCloudAgentUrlLength - 1] = 0x0;
    } else {
        LOGM_NOTICE("NULL cloud agent url, agent id %08x\n", agent_id);
    }

    p_content->enabled = 1;

    if (DLikely(mpCloudServerManager)) {
        LOGM_INFO("add cloud receiver content %s\n", p_content->content_name);
        mpCloudServerManager->AddCloudContent(p_server, p_content);
    }

    return EECode_OK;
}

void CActiveGenericEngineV3::setupStreamingServerManger()
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

EECode CActiveGenericEngineV3::addStreamingServer(TGenericID &server_id, StreamingServerType type, StreamingServerMode mode)
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
    LOGM_CONFIGURATION("streaming server port %d\n", mPersistMediaConfig.rtsp_server_config.rtsp_listen_port);
    p_server = mpStreamingServerManager->AddServer(type, mode, mPersistMediaConfig.rtsp_server_config.rtsp_listen_port, mPersistMediaConfig.rtsp_streaming_video_enable, mPersistMediaConfig.rtsp_streaming_audio_enable);

    if (DLikely(p_server)) {
        server_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_StreamingServer, mnComponentProxysNumbers[EGenericComponentType_StreamingServer - EGenericComponentType_TAG_proxy_start]);
        p_server->SetServerID(server_id, mnComponentProxysNumbers[EGenericComponentType_StreamingServer - EGenericComponentType_TAG_proxy_start], EGenericComponentType_StreamingServer);
        mnComponentProxysNumbers[EGenericComponentType_StreamingServer - EGenericComponentType_TAG_proxy_start] ++;
        mpStreamingServerList->InsertContent(NULL, (void *)p_server, 0);

        DASSERT(!mpDebugLastStreamingServer);
        mpDebugLastStreamingServer = p_server;
        return EECode_OK;
    } else {
        LOGM_ERROR("AddServer(%d, %d) fail\n", type, mode);
    }

    return EECode_Error;
}

EECode CActiveGenericEngineV3::removeStreamingServer(TGenericID server_id)
{
    if (DUnlikely(!mpStreamingServerManager)) {
        LOGM_ERROR("NULL mpStreamingServerManager, but someone invoke CActiveGenericEngineV3::removeStreamingServer?\n");
        return EECode_Error;
    }

    IStreamingServer *p_server = findStreamingServer(server_id);
    if (DLikely(p_server)) {
        mpStreamingServerList->RemoveContent((void *)p_server);
        mpStreamingServerManager->RemoveServer(p_server);
    } else {
        LOGM_ERROR("NO Server's id is 0x%08x.\n", server_id);
        return EECode_BadParam;
    }

    return EECode_OK;
}

IStreamingServer *CActiveGenericEngineV3::findStreamingServer(TGenericID id)
{
    //find if the server is exist
    CIDoubleLinkedList::SNode *pnode;
    IStreamingServer *p_server;
    TGenericID id1;
    TComponentIndex index;
    TComponentType type;

    pnode = mpStreamingServerList->FirstNode();
    while (pnode) {
        p_server = (IStreamingServer *) pnode->p_context;
        p_server->GetServerID(id1, index, type);
        if (id1 == id) {
            LOGM_INFO("find streaming server in list(id 0x%08x).\n", id);
            return p_server;
        }
        pnode = mpStreamingServerList->NextNode(pnode);
    }

    LOGM_WARN("cannot find streaming server in list(id 0x%08x).\n", id);
    return NULL;
}

void CActiveGenericEngineV3::setupCloudServerManger()
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

EECode CActiveGenericEngineV3::addCloudServer(TGenericID &server_id, CloudServerType type, TU16 port)
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
        mpCloudServerList->InsertContent(NULL, (void *)p_server, 0);

        DASSERT(!mpDebugLastCloudServer);
        mpDebugLastCloudServer = p_server;
        return EECode_OK;
    } else {
        LOGM_ERROR("AddServer(%d, %d) fail\n", type, port);
    }

    return EECode_Error;
}

EECode CActiveGenericEngineV3::removeCloudServer(TGenericID server_id)
{
    if (DUnlikely(!mpCloudServerManager)) {
        LOGM_ERROR("NULL mpCloudServerManager, but someone invoke CActiveGenericEngineV3::removeCloudServer?\n");
        return EECode_Error;
    }

    ICloudServer *p_server = findCloudServer(server_id);
    if (DLikely(p_server)) {
        mpCloudServerList->RemoveContent((void *)p_server);
        mpCloudServerManager->RemoveServer(p_server);
    } else {
        LOGM_ERROR("NO Server's id is 0x%08x.\n", server_id);
        return EECode_BadParam;
    }

    return EECode_OK;
}

ICloudServer *CActiveGenericEngineV3::findCloudServer(TGenericID id)
{
    //find if the server is exist
    CIDoubleLinkedList::SNode *pnode;
    ICloudServer *p_server;
    TGenericID id1;
    TComponentIndex index;
    TComponentType type;

    pnode = mpCloudServerList->FirstNode();
    while (pnode) {
        p_server = (ICloudServer *) pnode->p_context;
        p_server->GetServerID(id1, index, type);
        if (id1 == id) {
            LOGM_DEBUG("find cloud server in list(id 0x%08x).\n", id);
            return p_server;
        }
        pnode = mpCloudServerList->NextNode(pnode);
    }

    LOGM_WARN("cannot find cloud server in list(id 0x%08x).\n", id);
    return NULL;
}

void CActiveGenericEngineV3::startServers()
{
    if (mpStreamingServerManager) {
        mpStreamingServerManager->Start();
    }

    if (mpCloudServerManager) {
        mpCloudServerManager->Start();
    }
}

void CActiveGenericEngineV3::stopServers()
{
    if (mpStreamingServerManager) {
        mpStreamingServerManager->Stop();
        mpStreamingServerManager->ExitServerManager();
        mpStreamingServerManager->Destroy();
        mpStreamingServerManager = NULL;
    }

    if (mpCloudServerManager) {
        mpCloudServerManager->Stop();
        mpCloudServerManager->ExitServerManager();
        mpCloudServerManager->Destroy();
        mpCloudServerManager = NULL;
    }
}

EECode CActiveGenericEngineV3::createGraph(void)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;
    SConnection *p_connection = NULL;
    IFilter *pfilter = NULL;
    EECode err;
    TUint uppin_index, uppin_sub_index, downpin_index;

    LOGM_INFO("CActiveGenericEngineV3::createGraph start.\n");

    //create all filters
    while (pnode) {
        p_component = (SComponent *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            LOGM_INFO("create component, type %d, %s, index %d\n", p_component->type, gfGetComponentStringFromGenericComponentType(p_component->type), p_component->index);
            pfilter = gfFilterFactory(__filterTypeFromGenericComponentType(p_component->type, mPersistMediaConfig.number_scheduler_video_decoder, mPersistMediaConfig.number_scheduler_io_writer), (const SPersistMediaConfig *)&mPersistMediaConfig, (IMsgSink *)this, (TU32)p_component->index, mPersistMediaConfig.engine_config.filter_version);
            if (DLikely(pfilter)) {
                p_component->p_filter = pfilter;
            } else {
                LOGM_FATAL("CActiveGenericEngineV3::createGraph gfFilterFactory(type %d) fail.\n", p_component->type);
                return EECode_InternalLogicalBug;
            }
            setupComponentSpecific(p_component->type, pfilter, p_component);
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mpListFilters->NextNode(pnode);
    }

    LOGM_INFO("CActiveGenericEngineV3::createGraph filters construct done, start connection.\n");

    pnode = mpListConnections->FirstNode();
    //implement connetions
    while (pnode) {
        p_connection = (SConnection *)(pnode->p_context);
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
                LOGM_ERROR("Connect(up(%s) 0x%08x, down(%s) 0x%08x) failed, ret %d, %s\n", gfGetComponentStringFromGenericComponentType(p_connection->up->type), p_connection->up->id, gfGetComponentStringFromGenericComponentType(p_connection->down->type), p_connection->down->id, err, gfGetErrorCodeString(err));
                return err;
            }

            p_connection->up_pin_index = (TU8)uppin_index;
            p_connection->down_pin_index = (TU8)downpin_index;
            p_connection->up_pin_sub_index = (TU8)uppin_sub_index;
            LOGM_INFO("Connect success, up index %d, sub index %d, down index %d\n", uppin_index, uppin_sub_index, downpin_index);
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mpListConnections->NextNode(pnode);
    }

    err = setupPipelines();
    if (DUnlikely(EECode_OK != err)) {
        LOGM_FATAL("setupPipelines return %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    LOGM_INFO("CActiveGenericEngineV3::createGraph done.\n");

    mbGraghBuilt = 1;

    return EECode_OK;
}

EECode CActiveGenericEngineV3::setupPipelines(void)
{
    TComponentIndex index = 0;
    EECode err = EECode_OK;

    LOGM_NOTICE("setupPipelines: mnPlaybackPipelinesCurrentNumber %d, mnRecordingPipelinesCurrentNumber %d, mnStreamingPipelinesCurrentNumber %d, mnCloudPipelinesCurrentNumber %d, mnVODPipelinesCurrentNumber %d\n",
                mnPlaybackPipelinesCurrentNumber, mnRecordingPipelinesCurrentNumber, mnStreamingPipelinesCurrentNumber, mnCloudPipelinesCurrentNumber, mnVODPipelinesCurrentNumber);

    DASSERT(mpPlaybackPipelines);
    DASSERT(mpRecordingPipelines);
    DASSERT(mpStreamingPipelines);
    DASSERT(mpCloudPipelines);

    if (mpPlaybackPipelines && mnPlaybackPipelinesCurrentNumber) {
        for (index = 0; index < mnPlaybackPipelinesCurrentNumber; index ++) {
            err = setupPlaybackPipeline(index);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("[Graph, playback pipeline]: return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    if (mpRecordingPipelines && mnRecordingPipelinesCurrentNumber) {
        for (index = 0; index < mnRecordingPipelinesCurrentNumber; index ++) {
            err = setupRecordingPipeline(index);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("[Graph, recording pipeline]: return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    if (mpStreamingPipelines && mnStreamingPipelinesCurrentNumber) {
        for (index = 0; index < mnStreamingPipelinesCurrentNumber; index ++) {
            err = setupStreamingPipeline(index);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("[Graph, streaming pipeline]: return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    if (mpCloudPipelines && mnCloudPipelinesCurrentNumber) {
        for (index = 0; index < mnCloudPipelinesCurrentNumber; index ++) {
            err = setupCloudPipeline(index);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("[Graph, cloud pipeline]: return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    if (mpNativeCapturePipelines && mnNativeCapturePipelinesCurrentNumber) {
        for (index = 0; index < mnNativeCapturePipelinesCurrentNumber; index ++) {
            err = setupNativeCapturePipeline(index);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("[Graph, native capture pipeline]: return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    if (mpNativePushPipelines && mnNativePushPipelinesCurrentNumber) {
        for (index = 0; index < mnNativePushPipelinesCurrentNumber; index ++) {
            err = setupNativePushPipeline(index);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("[Graph, native push pipeline]: return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    if (mpCloudUploadPipelines && mnCloudUploadPipelinesCurrentNumber) {
        for (index = 0; index < mnCloudUploadPipelinesCurrentNumber; index ++) {
            err = setupCloudUploadPipeline(index);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("[Graph, native push pipeline]: return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    if (mpVODPipelines && mnVODPipelinesCurrentNumber) {
        for (index = 0; index < mnVODPipelinesCurrentNumber; index++) {
            err = setupVODPipeline(index);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("[Graph, vod pipeline]: return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV3::clearGraph(void)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;
    SConnection *p_connection = NULL;

    LOGM_INFO("clearGraph start.\n");

    //delete all filters
    while (pnode) {
        p_component = (SComponent *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            LOGM_INFO("destroy component, type %d, index %d\n", p_component->type, p_component->index);
            deleteComponent(p_component);
            pnode->p_context = NULL;
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mpListFilters->NextNode(pnode);
    }

    //delete all proxys
    pnode = mpListProxys->FirstNode();
    while (pnode) {
        p_component = (SComponent *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            LOGM_INFO("destroy component, type %d, index %d\n", p_component->type, p_component->index);
            deleteComponent(p_component);
            pnode->p_context = NULL;
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mpListProxys->NextNode(pnode);
    }

    //delete all connections
    pnode = mpListConnections->FirstNode();
    while (pnode) {
        p_connection = (SConnection *)(pnode->p_context);
        DASSERT(p_connection);
        if (p_connection) {
            LOGM_INFO("destroy connection, pin type %d, id %08x\n", p_connection->pin_type, p_connection->connection_id);
            deleteConnection(p_connection);
            pnode->p_context = NULL;
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mpListConnections->NextNode(pnode);
    }

    LOGM_INFO("clearGraph end.\n");

    return EECode_OK;
}

bool CActiveGenericEngineV3::processGenericMsg(SMSG &msg)
{
    //    EECode err;
    SMSG msg0;
    SComponent *p_component = NULL;
    CIRemoteLogServer *p_log_server = mPersistMediaConfig.p_remote_log_server;

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
        p_component = queryFilterComponent(msg.owner_id);
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

        case EMSGType_StorageError:
            //post msg to app
            LOGM_ERROR("[MSG flow]: EMSGType_StorageError msg comes, IFilter %p, type %d, index %d.\n", (IFilter *)msg.p_owner, p_component->type, p_component->index);
            memset(&msg0, 0x0, sizeof(msg0));
            msg0.code = EMSGType_StorageError;
            msg0.sessionID = mSessionID;
            msg0.owner_id = p_component->id;
            PostAppMsg(msg0);
            //abort recording, the muxed file maybe corrupted
            LOGM_ERROR("EMSGType_StorageError: TODO\n");
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
            PostAppMsg(msg);
            break;

        case EMSGType_NotifyUDECUpdateResolution:
            PostAppMsg(msg);
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

            //external cmd:
        case ECMDType_CloudSourceClient_UnknownCmd:
        case ECMDType_CloudSourceClient_UpdateAudioParams:
        case ECMDType_CloudSinkClient_UpdateBitrate:
        case ECMDType_CloudSinkClient_UpdateFrameRate:
        case ECMDType_CloudSinkClient_UpdateDisplayLayout:
        case ECMDType_CloudSinkClient_SelectAudioSource:
        case ECMDType_CloudSinkClient_SelectAudioTarget:
        case ECMDType_CloudSinkClient_Zoom:
        case ECMDType_CloudSinkClient_UpdateDisplayWindow:
        case ECMDType_CloudSinkClient_SelectVideoSource:
        case ECMDType_CloudSinkClient_ShowOtherWindows:
        case ECMDType_CloudSinkClient_DemandIDR:
        case ECMDType_CloudSinkClient_SwapWindowContent:
        case ECMDType_CloudSinkClient_CircularShiftWindowContent:
        case ECMDType_CloudSinkClient_SwitchGroup:
        case ECMDType_CloudSinkClient_ZoomV2:
        case ECMDType_CloudClient_QueryVersion:
        case ECMDType_CloudClient_QueryStatus:
        case ECMDType_CloudClient_SyncStatus:
        case ECMDType_CloudClient_CustomizedCommand:
            PostAppMsg(msg);
            break;

        case ECMDType_DebugClient_PrintCloudPipeline:
            mPersistMediaConfig.runtime_print_mask = (msg.p0 >> 24) & 0xff;
            msg.p0 = msg.p0 & 0xffffff;
            LOGM_NOTICE("%08lx, %08x\n", msg.p0, mPersistMediaConfig.runtime_print_mask);

            if (mPersistMediaConfig.runtime_print_mask & DLOG_MASK_TO_SNAPSHOT) {
                openNextSnapshotFile();
            }
            doPrintComponentStatus(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Cloud, (TU32)msg.p0));
            if (mpDebugLastCloudServer) {
                mpDebugLastCloudServer->GetObject0()->PrintStatus();
            }

            if (mpDebugLastStreamingServer) {
                mpDebugLastStreamingServer->GetObject0()->PrintStatus();
            }
            if (mpClockSource) {
                mpClockSource->GetObject0()->PrintStatus();
            }

            if (mpClockManager) {
                mpClockManager->PrintStatus();
            }

            if (mpSystemClockReference) {
                mpSystemClockReference->PrintStatus();
            }

            doPrintSchedulersStatus();

            if (mPersistMediaConfig.runtime_print_mask & DLOG_MASK_TO_SNAPSHOT) {
                gfCloseSnapshotFile();
            }
            mPersistMediaConfig.runtime_print_mask = 0;
            break;

        case ECMDType_DebugClient_PrintStreamingPipeline:
            mPersistMediaConfig.runtime_print_mask = (msg.p0 >> 24) & 0xff;
            msg.p0 = msg.p0 & 0xffffff;
            LOGM_NOTICE("%08lx, %08x\n", msg.p0, mPersistMediaConfig.runtime_print_mask);

            if (mPersistMediaConfig.runtime_print_mask & DLOG_MASK_TO_SNAPSHOT) {
                openNextSnapshotFile();
            }

            doPrintComponentStatus(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Streaming, (TU32)msg.p0));
            if (mpDebugLastCloudServer) {
                mpDebugLastCloudServer->GetObject0()->PrintStatus();
            }

            if (mpDebugLastStreamingServer) {
                mpDebugLastStreamingServer->GetObject0()->PrintStatus();
            }
            if (mpClockSource) {
                mpClockSource->GetObject0()->PrintStatus();
            }

            if (mpClockManager) {
                mpClockManager->PrintStatus();
            }

            if (mpSystemClockReference) {
                mpSystemClockReference->PrintStatus();
            }

            doPrintSchedulersStatus();
            if (mPersistMediaConfig.runtime_print_mask & DLOG_MASK_TO_SNAPSHOT) {
                gfCloseSnapshotFile();
            }
            mPersistMediaConfig.runtime_print_mask = 0;
            break;

        case ECMDType_DebugClient_PrintRecordingPipeline:
            mPersistMediaConfig.runtime_print_mask = (msg.p0 >> 24) & 0xff;
            msg.p0 = msg.p0 & 0xffffff;
            LOGM_NOTICE("%08lx, %08x\n", msg.p0, mPersistMediaConfig.runtime_print_mask);

            if (mPersistMediaConfig.runtime_print_mask & DLOG_MASK_TO_SNAPSHOT) {
                openNextSnapshotFile();
            }

            doPrintComponentStatus(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Recording, (TU32)msg.p0));
            if (mpDebugLastCloudServer) {
                mpDebugLastCloudServer->GetObject0()->PrintStatus();
            }

            if (mpDebugLastStreamingServer) {
                mpDebugLastStreamingServer->GetObject0()->PrintStatus();
            }
            if (mpClockSource) {
                mpClockSource->GetObject0()->PrintStatus();
            }

            if (mpClockManager) {
                mpClockManager->PrintStatus();
            }

            if (mpSystemClockReference) {
                mpSystemClockReference->PrintStatus();
            }

            doPrintSchedulersStatus();
            if (mPersistMediaConfig.runtime_print_mask & DLOG_MASK_TO_SNAPSHOT) {
                gfCloseSnapshotFile();
            }
            mPersistMediaConfig.runtime_print_mask = 0;
            break;

        case ECMDType_DebugClient_PrintSingleChannel:
            mPersistMediaConfig.runtime_print_mask = (msg.p0 >> 24) & 0xff;
            msg.p0 = msg.p0 & 0xffffff;
            LOGM_NOTICE("%08lx, %08x\n", msg.p0, mPersistMediaConfig.runtime_print_mask);

            if (mPersistMediaConfig.runtime_print_mask & DLOG_MASK_TO_SNAPSHOT) {
                openNextSnapshotFile();
            }

            doPrintComponentStatus(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Cloud, (TU32)msg.p0));
            doPrintComponentStatus(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Streaming, (TU32)msg.p0));
            doPrintComponentStatus(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_Recording, (TU32)msg.p0));
            if (mpDebugLastCloudServer) {
                mpDebugLastCloudServer->GetObject0()->PrintStatus();
            }

            if (mpDebugLastStreamingServer) {
                mpDebugLastStreamingServer->GetObject0()->PrintStatus();
            }
            if (mpClockSource) {
                mpClockSource->GetObject0()->PrintStatus();
            }

            if (mpClockManager) {
                mpClockManager->PrintStatus();
            }

            if (mpSystemClockReference) {
                mpSystemClockReference->PrintStatus();
            }

            doPrintSchedulersStatus();
            if (mPersistMediaConfig.runtime_print_mask & DLOG_MASK_TO_SNAPSHOT) {
                gfCloseSnapshotFile();
            }

            if (DLikely(p_log_server)) {
                p_log_server->SyncLog();
            }
            mPersistMediaConfig.runtime_print_mask = 0;
            break;

        case ECMDType_DebugClient_PrintAllChannels:
            mPersistMediaConfig.runtime_print_mask = (msg.p0 >> 24) & 0xff;
            LOGM_NOTICE("%08lx, %08x\n", msg.p0, mPersistMediaConfig.runtime_print_mask);

            if (mPersistMediaConfig.runtime_print_mask & DLOG_MASK_TO_SNAPSHOT) {
                openNextSnapshotFile();
            }
            doPrintCurrentStatus(0, DPrintFlagBit_dataPipeline);
            if (mPersistMediaConfig.runtime_print_mask & DLOG_MASK_TO_SNAPSHOT) {
                gfCloseSnapshotFile();
            }
            if (DLikely(p_log_server)) {
                p_log_server->SyncLog();
            }
            mPersistMediaConfig.runtime_print_mask = 0;
            break;

        case ECMDType_CloudClient_PeerClose:
            LOGM_NOTICE("ECMDType_CloudClient_PeerClose, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            p_component = findComponentFromID(DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(msg.owner_type, msg.owner_index));
            if (DLikely(p_component && mpCloudServerManager)) {
                CIDoubleLinkedList::SNode *p_node = mpCloudServerList->FirstNode();
                if (DLikely(p_node && p_node->p_context)) {
                    DASSERT(msg.p_owner == (TULong)p_component->p_filter);
                    mpCloudServerManager->RemoveCloudClient((ICloudServer *)p_node->p_context, (void *)msg.p_owner /*p_component->p_filter*/);
                } else {
                    LOGM_ERROR("NULL point\n");
                }
            } else {
                LOGM_ERROR("NULL point\n");
            }
            PostAppMsg(msg);
            break;

        case ECMDType_MiscCheckLicence:
            LOGM_NOTICE("ECMDType_MiscCheckLicence\n");
#ifdef BUILD_CONFIG_WITH_LICENCE
            if (mpLicenceClient) {
                EECode err = mpLicenceClient->DailyCheck();
                if (EECode_OK != err) {
                    LOGM_ERROR("DailyCheck fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                    mbRun = false;
                }
            } else {
                LOGM_FATAL("NULL mpLicenceClient\n");
            }
#endif
            break;

        case EMSGType_OpenSourceFail:
        case EMSGType_OpenSourceDone:
            break;

        default:
            LOGM_ERROR("unknown msg code %x\n", msg.code);
            break;
    }

    return true;
}

bool CActiveGenericEngineV3::processGenericCmd(SCMD &cmd)
{
    EECode err = EECode_OK;
    //LOGM_FLOW("[cmd flow]: cmd.code %d, state %d.\n", cmd.code, msState);

    switch (cmd.code) {

        case ECMDType_Start:
            //LOGM_FLOW("[cmd flow]: ECMDType_Start cmd, start.\n");
            err = doStartProcess();
            mpWorkQ->CmdAck(err);
            //LOGM_FLOW("[cmd flow]: ECMDType_Start cmd, end.\n");
            break;

        case ECMDType_Stop:
            //LOGM_FLOW("[cmd flow]: ECMDType_Stop cmd, start.\n");
            err = doStopProcess();
            mpWorkQ->CmdAck(err);
            //LOGM_FLOW("[cmd flow]: ECMDType_Stop cmd, end.\n");
            break;

        case ECMDType_PrintCurrentStatus:
            //LOGM_FLOW("[cmd flow]: ECMDType_PrintCurrentStatus cmd, start.\n");
            openNextSnapshotFile();
            doPrintCurrentStatus((TGenericID)cmd.res32_1, (TULong)cmd.res64_1);
            gfCloseSnapshotFile();
            //LOGM_FLOW("[cmd flow]: ECMDType_PrintCurrentStatus cmd, end.\n");
            break;

        case ECMDType_SetRuntimeLogConfig:
            //LOGM_FLOW("[cmd flow]: ECMDType_SetRuntimeLogConfig cmd, start.\n");
            doSetRuntimeLogSetting((TGenericID)cmd.res32_1, (TU32)cmd.res64_1, (TU32)(cmd.res64_1 >> 32), (TU32)cmd.res64_2, (TU32)(cmd.res64_2 >> 32));
            //LOGM_FLOW("[cmd flow]: ECMDType_SetRuntimeLogConfig cmd, end.\n");
            break;

        case ECMDType_ExitRunning:
            //LOGM_FLOW("[cmd flow]: ECMDType_ExitRunning cmd, start.\n");
            mbRun = false;
            //LOGM_FLOW("[cmd flow]: ECMDType_ExitRunning cmd, end.\n");
            break;

        default:
            LOGM_ERROR("unknown cmd %x.\n", cmd.code);
            return false;
    }

    return true;
}

void CActiveGenericEngineV3::processCMD(SCMD &oricmd)
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

EECode CActiveGenericEngineV3::initializeFilter(SComponent *component, TU8 type)
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
            if (component->prefer_string[0]) {
                err = p_filter->Initialize((TChar *)component->prefer_string);
            } else {
                err = p_filter->Initialize((TChar *)"AMBA");
            }
            DASSERT_OK(err);
            break;

        case EGenericComponentType_VideoEncoder:
            if (component->prefer_string[0]) {
                err = p_filter->Initialize((TChar *)component->prefer_string);
            } else {
                err = p_filter->Initialize((TChar *)"AMBA");
            }
            DASSERT_OK(err);
            break;

        case EGenericComponentType_AudioEncoder:
            if (component->prefer_string[0]) {
                err = p_filter->Initialize((TChar *)component->prefer_string);
            } else {
                if (EAudioEncoderModuleID_FFMpeg == mPersistMediaConfig.prefer_module_setting.mPreferedAudioEncoderModule) {
                    err = p_filter->Initialize((TChar *)"FFMpeg");
                } else if (EAudioEncoderModuleID_PrivateLibAAC == mPersistMediaConfig.prefer_module_setting.mPreferedAudioEncoderModule) {
                    err = p_filter->Initialize((TChar *)"AAC");
                } else if (EAudioEncoderModuleID_CustomizedADPCM == mPersistMediaConfig.prefer_module_setting.mPreferedAudioEncoderModule) {
                    err = p_filter->Initialize((TChar *)"CustomizedADPCM");
                } else {
                    err = p_filter->Initialize((TChar *)"AAC");
                    //err = p_filter->Initialize((TChar*)"FFMpeg");
                }
            }
            DASSERT_OK(err);
            break;

        case EGenericComponentType_AudioDecoder:
            if (component->prefer_string[0]) {
                err = p_filter->Initialize((TChar *)component->prefer_string);
            } else {
                if (EAudioDecoderModuleID_FFMpeg == mPersistMediaConfig.prefer_module_setting.mPreferedAudioDecoderModule) {
                    err = p_filter->Initialize((TChar *)"FFMpeg");
                } else if (EAudioDecoderModuleID_AAC == mPersistMediaConfig.prefer_module_setting.mPreferedAudioDecoderModule) {
                    err = p_filter->Initialize((TChar *)"AAC");
                } else if (EAudioDecoderModuleID_CustomizedADPCM == mPersistMediaConfig.prefer_module_setting.mPreferedAudioDecoderModule) {
                    err = p_filter->Initialize((TChar *)"CustomizedADPCM");
                } else {
                    err = p_filter->Initialize((TChar *)"FFMpeg");
                }
            }
            DASSERT_OK(err);
            break;

        case EGenericComponentType_VideoPostProcessor:
            if (component->prefer_string[0]) {
                err = p_filter->Initialize((TChar *)component->prefer_string);
            } else {
                err = p_filter->Initialize((TChar *)"AMBA");
            }
            DASSERT_OK(err);
            break;

        case EGenericComponentType_AudioPostProcessor:
            if (component->prefer_string[0]) {
                err = p_filter->Initialize((TChar *)component->prefer_string);
            } else {
                err = p_filter->Initialize((TChar *)"AMBA");
            }
            DASSERT_OK(err);
            break;

        case EGenericComponentType_Muxer:
            if (component->prefer_string[0]) {
                err = p_filter->Initialize((TChar *)component->prefer_string);
            } else {
                if (EMuxerModuleID_FFMpeg == mPersistMediaConfig.prefer_module_setting.mPreferedMuxerModule) {
                    err = p_filter->Initialize((TChar *)"FFMpeg");
                } else if (EMuxerModuleID_PrivateTS == mPersistMediaConfig.prefer_module_setting.mPreferedMuxerModule) {
                    err = p_filter->Initialize((TChar *)"TS");
                } else {
                    err = p_filter->Initialize((TChar *)"TS");
                }
            }
            DASSERT_OK(err);

            err = p_filter->FilterProperty(EFilterPropertyType_update_sink_url, 1, component->url);
            DASSERT_OK(err);
            break;

        case EGenericComponentType_VideoRenderer:
            if (component->prefer_string[0]) {
                err = p_filter->Initialize((TChar *)component->prefer_string);
            } else {
                err = p_filter->Initialize((TChar *)"AMBA");
            }
            DASSERT_OK(err);
            break;

        case EGenericComponentType_AudioRenderer:
            if (component->prefer_string[0]) {
                err = p_filter->Initialize((TChar *)component->prefer_string);
            } else {
                err = p_filter->Initialize((TChar *)"ALSA");
            }
            DASSERT_OK(err);
            break;

        case EGenericComponentType_StreamingTransmiter:
            if (component->prefer_string[0]) {
                err = p_filter->Initialize((TChar *)component->prefer_string);
            } else {
                err = p_filter->Initialize((TChar *)"RTSP");
            }
            DASSERT_OK(err);
            break;

        case EGenericComponentType_ConnecterPinMuxer:
            err = p_filter->Initialize(NULL);
            DASSERT_OK(err);
            break;

        case EGenericComponentType_CloudConnecterServerAgent:
            err = p_filter->Initialize(NULL);
            DASSERT_OK(err);
            break;

        case EGenericComponentType_CloudConnecterClientAgent:
            err = p_filter->Initialize(NULL);
            DASSERT_OK(err);
            break;

        case EGenericComponentType_CloudConnecterCmdAgent:
            err = p_filter->Initialize(NULL);
            DASSERT_OK(err);
            break;

        case EGenericComponentType_AudioCapture:
            if (component->prefer_string[0]) {
                err = p_filter->Initialize((TChar *)component->prefer_string);
            } else {
                err = p_filter->Initialize((TChar *)"ALSA");
            }
            DASSERT_OK(err);
            break;

        case EGenericComponentType_FlowController:
            err = p_filter->Initialize();
            break;

        default:
            LOGM_FATAL("unknown type %d\n", type);
            break;
    }

    return err;
}

EECode CActiveGenericEngineV3::initializeAllFilters(TGenericID pipeline_id)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;
    IFilter *p_filter = NULL;
    EECode err = EECode_OK;

    if (DLikely(!IS_VALID_COMPONENT_ID(pipeline_id))) {
        DASSERT(0 == pipeline_id);
        LOGM_INFO(" initialize all filters begin, filters number %d\n", mpListFilters->NumberOfNodes());
        while (pnode) {
            p_component = (SComponent *)(pnode->p_context);
            DASSERT(p_component);
            DASSERT(p_component->p_filter);
            DASSERT(EComponentState_not_alive == p_component->cur_state);

            p_filter = (IFilter *)p_component->p_filter;
            if (p_filter) {
                err = initializeFilter(p_component, p_component->type);
                DASSERT_OK(err);
                if (EECode_OK == err) {
                    p_component->cur_state = EComponentState_initialized;
                } else {
                    LOGM_ERROR("initializeFilter(%s) fail, index %d, pipeline_id 0x%08x\n", gfGetComponentStringFromGenericComponentType(p_component->type), p_component->index, pipeline_id);
                    return err;
                }
            }
            pnode = mpListFilters->NextNode(pnode);
        }
        LOGM_INFO(" initialize all filters end\n");
    } else {
        LOGM_ERROR("initializeAllFilters: TODO!\n");
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV3::finalizeAllFilters(TGenericID pipeline_id)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;
    IFilter *p_filter = NULL;
    EECode err = EECode_OK;

    if (DLikely(!IS_VALID_COMPONENT_ID(pipeline_id))) {
        DASSERT(0 == pipeline_id);
        LOGM_INFO(" finalizeAllFilters begin, filters number %d\n", mpListFilters->NumberOfNodes());
        while (pnode) {
            p_component = (SComponent *)(pnode->p_context);
            DASSERT(p_component);
            DASSERT(p_component->p_filter);
            DASSERT(EComponentState_initialized == p_component->cur_state);
            p_filter = (IFilter *)p_component->p_filter;
            if (p_filter) {
                err = p_filter->Finalize();
                DASSERT_OK(err);
            }
            pnode = mpListFilters->NextNode(pnode);
        }
        LOGM_INFO(" finalizeAllFilters end\n");
    } else {
        LOGM_ERROR("finalizeAllFilters: TODO\n");
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV3::exitAllFilters(TGenericID pipeline_id)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;
    IFilter *p_filter = NULL;

    if (DLikely(!IS_VALID_COMPONENT_ID(pipeline_id))) {
        DASSERT(0 == pipeline_id);
        LOGM_INFO(" exit all fiters begin, filters number %d\n", mpListFilters->NumberOfNodes());
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
            pnode = mpListFilters->NextNode(pnode);
        }
        LOGM_INFO(" exit all fiters end\n");
    } else {
        LOGM_ERROR("exitAllFilters: TODO\n");
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV3::startAllFilters(TGenericID pipeline_id)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;
    IFilter *p_filter = NULL;

    if (DLikely(!IS_VALID_COMPONENT_ID(pipeline_id))) {
        DASSERT(0 == pipeline_id);
        LOGM_INFO(" start all fiters begin, filters number %d\n", mpListFilters->NumberOfNodes());
        while (pnode) {
            p_component = (SComponent *)(pnode->p_context);
            DASSERT(p_component);
            DASSERT(p_component->p_filter);
            DASSERT(EComponentState_activated == p_component->cur_state);
            if (p_component->active_count) {
                p_filter = (IFilter *)p_component->p_filter;
                if (p_filter) {
                    p_filter->Start();
                    LOGM_PRINTF("[Flow]: component start (0x%08x), active count %d\n", p_component->id, p_component->active_count);
                    p_component->cur_state = EComponentState_started;
                }
            } else {
                LOGM_PRINTF("[Flow]: component not start (0x%08x), active count %d\n", p_component->id, p_component->active_count);
            }
            pnode = mpListFilters->NextNode(pnode);
        }
        LOGM_INFO(" start all fiters end\n");
    } else {
        LOGM_ERROR("startAllFilters: TODO\n");
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV3::runAllFilters(TGenericID pipeline_id)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;
    IFilter *p_filter = NULL;

    if (DLikely(!IS_VALID_COMPONENT_ID(pipeline_id))) {
        DASSERT(0 == pipeline_id);
        LOGM_INFO(" run all fiters begin, filters number %d\n", mpListFilters->NumberOfNodes());
        while (pnode) {
            p_component = (SComponent *)(pnode->p_context);
            DASSERT(p_component);
            DASSERT(p_component->p_filter);
            DASSERT(EComponentState_initialized == p_component->cur_state);
            p_filter = (IFilter *)p_component->p_filter;
            if (p_filter) {
                p_filter->Run();
                p_component->cur_state = EComponentState_activated;
            }
            pnode = mpListFilters->NextNode(pnode);
        }
        LOGM_INFO(" run all fiters end\n");
    } else {
        LOGM_ERROR("runAllFilters: TODO\n");
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV3::stopAllFilters(TGenericID pipeline_id)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;
    IFilter *p_filter = NULL;

    if (DLikely(!IS_VALID_COMPONENT_ID(pipeline_id))) {
        DASSERT(0 == pipeline_id);
        LOGM_INFO(" stop all fiters begin, filters number %d\n", mpListFilters->NumberOfNodes());
        while (pnode) {
            p_component = (SComponent *)(pnode->p_context);
            DASSERT(p_component);
            DASSERT(p_component->p_filter);
            //DASSERT(p_component->p_scur_pipeline);
            //DASSERT(EComponentState_started == p_component->cur_state);
            p_filter = (IFilter *)p_component->p_filter;
            if (p_filter) {
                p_filter->Stop();
                p_component->cur_state = EComponentState_initialized;
            }
            pnode = mpListFilters->NextNode(pnode);
        }
        LOGM_INFO(" stop all fiters end\n");
    } else {
        LOGM_ERROR("stopAllFilters: TODO\n");
    }

    return EECode_OK;
}

bool CActiveGenericEngineV3::allFiltersFlag(TUint flag, TGenericID id)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;

    if (DLikely(!IS_VALID_COMPONENT_ID(id))) {
        DASSERT(0 == id);
        while (pnode) {
            p_component = (SComponent *)(pnode->p_context);
            DASSERT(p_component);
            if (p_component) {
                if (!(flag & p_component->status_flag)) {
                    return false;
                }
            } else {
                DASSERT("NULL pointer here, something would be wrong.\n");
            }
            pnode = mpListFilters->NextNode(pnode);
        }
        return true;
    } else {
        LOGM_ERROR("allFiltersFlag: TODO\n");
        return true;
    }
}

void CActiveGenericEngineV3::clearAllFiltersFlag(TUint flag, TGenericID id)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;

    if (DLikely(!IS_VALID_COMPONENT_ID(id))) {
        DASSERT(0 == id);
        while (pnode) {
            p_component = (SComponent *)(pnode->p_context);
            DASSERT(p_component);
            if (p_component) {
                p_component->status_flag &= (~ flag);
            } else {
                DASSERT("NULL pointer here, something would be wrong.\n");
            }
            pnode = mpListFilters->NextNode(pnode);
        }
    } else {
        LOGM_ERROR("clearAllFiltersFlag: TODO\n");
    }
}

void CActiveGenericEngineV3::setFiltersEOS(IFilter *pfilter)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;

    while (pnode) {
        p_component = (SComponent *)(pnode->p_context);
        DASSERT(p_component);
        if (p_component) {
            if (pfilter == p_component->p_filter) {
                p_component->status_flag |= DCAL_BITMASK(DFLAG_STATUS_EOS);
                return;
            }
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mpListFilters->NextNode(pnode);
    }

    LOGM_WARN("do not find filter %p\n", pfilter);
}

EECode CActiveGenericEngineV3::createSchedulers(void)
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

            LOGM_CONFIGURATION("udp receiver number %d\n", number);

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

        if (mPersistMediaConfig.number_scheduler_network_tcp_reciever) {
            number = mPersistMediaConfig.number_scheduler_network_tcp_reciever;
            DASSERT(number <= DMaxSchedulerGroupNumber);
            if (number > DMaxSchedulerGroupNumber) {
                number = DMaxSchedulerGroupNumber;
            }

            LOGM_CONFIGURATION("tcp receiver number %d\n", number);

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

            LOGM_CONFIGURATION("network sender number %d\n", number);

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

            LOGM_CONFIGURATION("io reader number %d\n", number);

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

            LOGM_CONFIGURATION("io writer number %d\n", number);

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

            LOGM_CONFIGURATION("video decoder number %d\n", number);

            for (i = 0; i < number; i ++) {
                DASSERT(!mPersistMediaConfig.p_scheduler_video_decoder[i]);
                LOGM_INFO("p_scheduler_video_decoder, gfSchedulerFactory(ESchedulerType_RunRobin, %d)\n", i);
                mPersistMediaConfig.p_scheduler_video_decoder[i] = gfSchedulerFactory(ESchedulerType_RunRobin, i);
                if (mPersistMediaConfig.p_scheduler_video_decoder[i]) {
                    mPersistMediaConfig.p_scheduler_video_decoder[i]->StartScheduling();
                }
            }
        }

        if (mPersistMediaConfig.number_tcp_pulse_sender) {
            number = mPersistMediaConfig.number_tcp_pulse_sender;
            DASSERT(number <= DMaxTCPPulseSenderNumber);
            if (number > DMaxTCPPulseSenderNumber) {
                number = DMaxTCPPulseSenderNumber;
            }

            LOGM_CONFIGURATION("tcp pulse sender number %d\n", number);

            for (i = 0; i < number; i ++) {
                DASSERT(!mPersistMediaConfig.p_tcp_pulse_sender[i]);
                LOGM_INFO("p_tcp_pulse_sender, %d)\n", i);
                mPersistMediaConfig.p_tcp_pulse_sender[i] = gfCreateTCPPulseSender();
                if (mPersistMediaConfig.p_tcp_pulse_sender[i]) {
                    mPersistMediaConfig.p_tcp_pulse_sender[i]->Start();
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

EECode CActiveGenericEngineV3::destroySchedulers(void)
{
    TUint i = 0;

    DASSERT(mbSchedulersCreated);

    for (i = 0; i < DMaxSchedulerGroupNumber; i ++) {
        if (mPersistMediaConfig.p_scheduler_network_reciever[i]) {
            mPersistMediaConfig.p_scheduler_network_reciever[i]->StopScheduling();
            mPersistMediaConfig.p_scheduler_network_reciever[i]->GetObject0()->Delete();
            mPersistMediaConfig.p_scheduler_network_reciever[i] = NULL;
        }
    }

    for (i = 0; i < DMaxSchedulerGroupNumber; i ++) {
        if (mPersistMediaConfig.p_scheduler_network_tcp_reciever[i]) {
            mPersistMediaConfig.p_scheduler_network_tcp_reciever[i]->StopScheduling();
            mPersistMediaConfig.p_scheduler_network_tcp_reciever[i]->GetObject0()->Delete();
            mPersistMediaConfig.p_scheduler_network_tcp_reciever[i] = NULL;
        }
    }

    for (i = 0; i < DMaxSchedulerGroupNumber; i ++) {
        if (mPersistMediaConfig.p_scheduler_network_sender[i]) {
            mPersistMediaConfig.p_scheduler_network_sender[i]->StopScheduling();
            mPersistMediaConfig.p_scheduler_network_sender[i]->GetObject0()->Delete();
            mPersistMediaConfig.p_scheduler_network_sender[i] = NULL;
        }
    }

    for (i = 0; i < DMaxSchedulerGroupNumber; i ++) {
        if (mPersistMediaConfig.p_scheduler_io_reader[i]) {
            mPersistMediaConfig.p_scheduler_io_reader[i]->StopScheduling();
            mPersistMediaConfig.p_scheduler_io_reader[i]->GetObject0()->Delete();
            mPersistMediaConfig.p_scheduler_io_reader[i] = NULL;
        }
    }

    for (i = 0; i < DMaxSchedulerGroupNumber; i ++) {
        if (mPersistMediaConfig.p_scheduler_io_writer[i]) {
            mPersistMediaConfig.p_scheduler_io_writer[i]->StopScheduling();
            mPersistMediaConfig.p_scheduler_io_writer[i]->GetObject0()->Delete();
            mPersistMediaConfig.p_scheduler_io_writer[i] = NULL;
        }
    }

    for (i = 0; i < DMaxTCPPulseSenderNumber; i ++) {
        if (mPersistMediaConfig.p_tcp_pulse_sender[i]) {
            mPersistMediaConfig.p_tcp_pulse_sender[i]->Stop();
            mPersistMediaConfig.p_tcp_pulse_sender[i]->Destroy();
            mPersistMediaConfig.p_tcp_pulse_sender[i] = NULL;
        }
    }

    mbSchedulersCreated = 0;

    return EECode_OK;
}

EECode CActiveGenericEngineV3::doStartProcess()
{
    EECode err;

    DASSERT(1 == mbGraghBuilt);
    if (0 == mbGraghBuilt) {
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

        startServers();

        msState = EEngineState_running;
    } else if (EEngineState_running == msState) {
        //already running
        LOGM_ERROR("engine is already running\n");
    } else {
        //bad state
        LOGM_ERROR("BAD engine state %d\n", msState);
        return EECode_BadState;
    }

    LOGM_INFO("doStartProcess(), done\n");

    return EECode_OK;
}

EECode CActiveGenericEngineV3::doStopProcess()
{
    if (0 == mbGraghBuilt) {
        LOGM_ERROR("gragh is not built yet\n");
        return EECode_OK;
    }

    if (EEngineState_running == msState) {
        mPersistMediaConfig.app_start_exit = 1;
        mpClockManager->UnRegisterClockReference(mpSystemClockReference);
        mpSystemClockReference->ClearAllTimers();
        stopServers();
        stopAllFilters(0);
        if (!mbExternalClock) {
            if (mpClockManager) {
                mpClockManager->Stop();
            }
        }
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

void CActiveGenericEngineV3::reconnectAllStreamingServer()
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;

    while (pnode) {
        p_component = (SComponent *)(pnode->p_context);
        DASSERT(p_component);
        if (EGenericComponentType_Demuxer == p_component->type) {
            if (p_component->p_filter) {
                p_component->p_filter->FilterProperty(EFilterPropertyType_demuxer_reconnect_server, 1, NULL);
            }
        }
        pnode = mpListFilters->NextNode(pnode);
    }
}

void CActiveGenericEngineV3::doPrintSchedulersStatus()
{
    TU32 i = 0, number = 0;

    if (mPersistMediaConfig.number_scheduler_network_reciever) {
        number = mPersistMediaConfig.number_scheduler_network_reciever;
        LOGM_CONFIGURATION("udp receiver number %d\n", number);
        for (i = 0; i < number; i ++) {
            if (mPersistMediaConfig.p_scheduler_network_reciever[i]) {
                mPersistMediaConfig.p_scheduler_network_reciever[i]->GetObject0()->PrintStatus();
            }
        }
    }

    if (mPersistMediaConfig.number_scheduler_network_tcp_reciever) {
        number = mPersistMediaConfig.number_scheduler_network_tcp_reciever;
        LOGM_CONFIGURATION("tcp receiver number %d\n", number);
        for (i = 0; i < number; i ++) {
            if (mPersistMediaConfig.p_scheduler_network_tcp_reciever[i]) {
                mPersistMediaConfig.p_scheduler_network_tcp_reciever[i]->GetObject0()->PrintStatus();
            }
        }
    }

    if (mPersistMediaConfig.number_scheduler_network_sender) {
        number = mPersistMediaConfig.number_scheduler_network_sender;
        LOGM_CONFIGURATION("network sender number %d\n", number);

        for (i = 0; i < number; i ++) {
            if (mPersistMediaConfig.p_scheduler_network_sender[i]) {
                mPersistMediaConfig.p_scheduler_network_sender[i]->GetObject0()->PrintStatus();
            }
        }
    }

    if (mPersistMediaConfig.number_scheduler_io_reader) {
        number = mPersistMediaConfig.number_scheduler_io_reader;
        LOGM_CONFIGURATION("io reader number %d\n", number);

        for (i = 0; i < number; i ++) {
            if (mPersistMediaConfig.p_scheduler_io_reader[i]) {
                mPersistMediaConfig.p_scheduler_io_reader[i]->GetObject0()->PrintStatus();
            }
        }
    }

    if (mPersistMediaConfig.number_scheduler_io_writer) {
        number = mPersistMediaConfig.number_scheduler_io_writer;
        LOGM_CONFIGURATION("io writer number %d\n", number);

        for (i = 0; i < number; i ++) {
            if (mPersistMediaConfig.p_scheduler_io_writer[i]) {
                mPersistMediaConfig.p_scheduler_io_writer[i]->GetObject0()->PrintStatus();
            }
        }
    }

    if (mPersistMediaConfig.number_scheduler_video_decoder) {
        number = mPersistMediaConfig.number_scheduler_video_decoder;
        LOGM_CONFIGURATION("video decoder number %d\n", number);

        for (i = 0; i < number; i ++) {
            if (mPersistMediaConfig.p_scheduler_video_decoder[i]) {
                mPersistMediaConfig.p_scheduler_video_decoder[i]->GetObject0()->PrintStatus();
            }
        }
    }

}

void CActiveGenericEngineV3::doPrintCurrentStatus(TGenericID id, TULong print_flag)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;
    TUint i = 0, max = 0;

    if (print_flag & DPrintFlagBit_logConfigure) {
        gfPrintLogConfig(&gmModuleLogConfig[0], ELogModuleListEnd);
    }

    if (print_flag & DPrintFlagBit_dataPipeline) {
        if (DLikely(!IS_VALID_COMPONENT_ID(id))) {
            DASSERT(0 == id);
            while (pnode) {
                p_component = (SComponent *)(pnode->p_context);
                DASSERT(p_component);
                if (p_component) {
                    p_component->p_filter->GetObject0()->PrintStatus();
                } else {
                    DASSERT("NULL pointer here, something would be wrong.\n");
                }
                pnode = mpListFilters->NextNode(pnode);
            }

            if (mpCloudServerManager) {
                mpCloudServerManager->PrintStatus0();
            }

            if (mpDebugLastCloudServer) {
                mpDebugLastCloudServer->GetObject0()->PrintStatus();
            }

            if (mpStreamingServerManager) {
                mpStreamingServerManager->PrintStatus0();
            }

            if (mpDebugLastStreamingServer) {
                mpDebugLastStreamingServer->GetObject0()->PrintStatus();
            }

            if (!(mPersistMediaConfig.runtime_print_mask & DLOG_MASK_TO_REMOTE)) {
                if (mpClockSource) {
                    mpClockSource->GetObject0()->PrintStatus();
                }

                if (mpClockManager) {
                    mpClockManager->PrintStatus();
                }

                if (mpSystemClockReference) {
                    mpSystemClockReference->PrintStatus();
                }

                doPrintSchedulersStatus();

                SStreamingSessionContent *p_content = NULL;
                max = mnStreamingPipelinesCurrentNumber;
                for (i = 0; i < max; i++) {
                    p_content = mpStreamingContent + i;
                    LOGM_PRINTF("[%d] streaming url %s\n", i, p_content->session_name);
                }

                SCloudContent *p_cloud_content = NULL;
                max = mnStreamingPipelinesCurrentNumber;
                for (i = 0; i < max; i++) {
                    p_cloud_content = mpCloudContent + i;
                    LOGM_PRINTF("[%d] cloud url %s\n", i, p_cloud_content->content_name);
                }
            } else {
                CIRemoteLogServer *p_log_server = mPersistMediaConfig.p_remote_log_server;
                if (p_log_server) {
                    //SStreamingSessionContent* p_content = NULL;
                    SCloudContent *p_cloud_content = NULL;
                    max = mnStreamingPipelinesCurrentNumber;
                    p_log_server->WriteLog("\033[40;36m\033[1m\n\turl settings:\n");
                    for (i = 0; i < max; i++) {
                        //p_content = mpStreamingContent + i;
                        p_cloud_content = mpCloudContent + i;
                        if (DLikely((i + 1) % 10)) {
                            //p_log_server->WriteLog("%d:%s,%s\t", i, p_cloud_content->content_name, p_content->session_name);
                            p_log_server->WriteLog("%d:%s\t", i, p_cloud_content->content_name);
                        } else {
                            //p_log_server->WriteLog("%d:%s,%s\n", i, p_cloud_content->content_name, p_content->session_name);
                            p_log_server->WriteLog("%d:%s\n", i, p_cloud_content->content_name);
                        }
                    }

                    if (DUnlikely(max < 10)) {
                        p_log_server->WriteLog("\n");
                    }
                }
            }

        } else {
            doPrintComponentStatus(id);

            if (mpCloudServerManager) {
                mpCloudServerManager->PrintStatus0();
            }

            if (mpDebugLastCloudServer) {
                mpDebugLastCloudServer->GetObject0()->PrintStatus();
            }

            if (mpStreamingServerManager) {
                mpStreamingServerManager->PrintStatus0();
            }

            if (mpDebugLastStreamingServer) {
                mpDebugLastStreamingServer->GetObject0()->PrintStatus();
            }

            if (!(mPersistMediaConfig.runtime_print_mask & DLOG_MASK_TO_REMOTE)) {
                if (mpClockSource) {
                    mpClockSource->GetObject0()->PrintStatus();
                }

                if (mpClockManager) {
                    mpClockManager->PrintStatus();
                }

                if (mpSystemClockReference) {
                    mpSystemClockReference->PrintStatus();
                }

                doPrintSchedulersStatus();
            }
        }

    }

}

void CActiveGenericEngineV3::doPrintComponentStatus(TGenericID id)
{
    TComponentType type = DCOMPONENT_TYPE_FROM_GENERIC_ID(id);
    TComponentType index = DCOMPONENT_INDEX_FROM_GENERIC_ID(id);

    switch (type) {

        case EGenericPipelineType_Streaming: {
                if (DUnlikely(index >= mnStreamingPipelinesCurrentNumber)) {
                    LOGM_FATAL("BAD index %d, exceed max value %d\n", index, mnStreamingPipelinesCurrentNumber);
                    return;
                }

                if (DUnlikely((NULL == mpStreamingPipelines) || (NULL == mpStreamingContent))) {
                    LOGM_FATAL("NULL mpStreamingPipelines %p or mpStreamingContent %p\n", mpStreamingPipelines, mpStreamingContent);
                    return;
                }

                SStreamingPipeline *p_pipeline = mpStreamingPipelines + index;
                SStreamingSessionContent *p_content = mpStreamingContent + index;

                LOGM_PRINTF("@@streaming url %s\n", p_content->session_name);

                if (p_pipeline->p_video_source[0]) {
                    if (p_pipeline->p_video_source[0]->url) {
                        LOGM_PRINTF("@@video source url %s\n", p_pipeline->p_video_source[0]->url);
                    }
                    if (DLikely(p_pipeline->p_video_source[0]->p_filter)) {
                        p_pipeline->p_video_source[0]->p_filter->GetObject0()->PrintStatus();
                    }
                }

                if (p_pipeline->p_audio_source[0]) {
                    if (p_pipeline->p_audio_source[0]->url) {
                        LOGM_PRINTF("@@audio source url %s\n", p_pipeline->p_audio_source[0]->url);
                    }
                    if (DLikely(p_pipeline->p_audio_source[0]->p_filter)) {
                        p_pipeline->p_audio_source[0]->p_filter->GetObject0()->PrintStatus();
                    }
                }

                if (p_pipeline->p_data_transmiter) {
                    if (DLikely(p_pipeline->p_data_transmiter->p_filter)) {
                        p_pipeline->p_data_transmiter->p_filter->GetObject0()->PrintStatus();
                    }
                }

            }
            break;

        case EGenericPipelineType_Cloud: {
                if (DUnlikely(index >= mnCloudPipelinesCurrentNumber)) {
                    LOGM_FATAL("BAD index %d, exceed max value %d\n", index, mnCloudPipelinesCurrentNumber);
                    return;
                }

                if (DUnlikely((NULL == mpCloudPipelines) || (NULL == mpCloudContent))) {
                    LOGM_FATAL("NULL mpCloudPipelines %p or mpCloudContent %p\n", mpCloudPipelines, mpCloudContent);
                    return;
                }

                SCloudPipeline *p_pipeline = mpCloudPipelines + index;
                SCloudContent *p_content = mpCloudContent + index;

                if (p_pipeline->p_agent) {
                    if (DLikely(p_pipeline->p_agent->p_filter)) {
                        p_pipeline->p_agent->p_filter->GetObject0()->PrintStatus();
                    }
                }

                if (p_pipeline->p_video_sink[0]) {
                    if (DLikely(p_pipeline->p_video_sink[0]->p_filter)) {
                        p_pipeline->p_video_sink[0]->p_filter->GetObject0()->PrintStatus();
                    }
                }

                if (p_pipeline->p_audio_sink[0]) {
                    if (DLikely(p_pipeline->p_audio_sink[0]->p_filter)) {
                        p_pipeline->p_audio_sink[0]->p_filter->GetObject0()->PrintStatus();
                    }
                }

                LOGM_PRINTF("cloud url %s\n", p_content->content_name);

            }
            break;

        case EGenericPipelineType_Recording: {
                if (DUnlikely(index >= mnRecordingPipelinesCurrentNumber)) {
                    LOGM_FATAL("BAD index %d, exceed max value %d\n", index, mnRecordingPipelinesCurrentNumber);
                    return;
                }

                if (DUnlikely(NULL == mpRecordingPipelines)) {
                    LOGM_FATAL("NULL mpRecordingPipelines %p\n", mpRecordingPipelines);
                    return;
                }

                SRecordingPipeline *p_pipeline = mpRecordingPipelines + index;

                if (p_pipeline->p_video_source[0]) {
                    if (DLikely(p_pipeline->p_video_source[0]->p_filter)) {
                        p_pipeline->p_video_source[0]->p_filter->GetObject0()->PrintStatus();
                    }
                }

                if (p_pipeline->p_audio_source[0]) {
                    if (DLikely(p_pipeline->p_audio_source[0]->p_filter)) {
                        p_pipeline->p_audio_source[0]->p_filter->GetObject0()->PrintStatus();
                    }
                }

                if (p_pipeline->p_sink) {
                    if (DLikely(p_pipeline->p_sink->p_filter)) {
                        p_pipeline->p_sink->p_filter->GetObject0()->PrintStatus();
                    }
                }

            }
            break;

        case EGenericPipelineType_Playback: {
                if (DUnlikely(index >= mnPlaybackPipelinesCurrentNumber)) {
                    LOGM_FATAL("BAD index %d, exceed max value %d\n", index, mnPlaybackPipelinesCurrentNumber);
                    return;
                }

                if (DUnlikely(NULL == mpPlaybackPipelines)) {
                    LOGM_FATAL("NULL mpPlaybackPipelines %p\n", mpPlaybackPipelines);
                    return;
                }

                SPlaybackPipeline *p_pipeline = mpPlaybackPipelines + index;

                if (p_pipeline->p_video_source) {
                    if (DLikely(p_pipeline->p_video_source->p_filter)) {
                        p_pipeline->p_video_source->p_filter->GetObject0()->PrintStatus();
                    }
                }

                if (p_pipeline->p_audio_source) {
                    if (DLikely(p_pipeline->p_audio_source->p_filter)) {
                        p_pipeline->p_audio_source->p_filter->GetObject0()->PrintStatus();
                    }
                }

                if (p_pipeline->p_video_decoder) {
                    if (DLikely(p_pipeline->p_video_decoder->p_filter)) {
                        p_pipeline->p_video_decoder->p_filter->GetObject0()->PrintStatus();
                    }
                }

                if (p_pipeline->p_audio_decoder) {
                    if (DLikely(p_pipeline->p_audio_decoder->p_filter)) {
                        p_pipeline->p_audio_decoder->p_filter->GetObject0()->PrintStatus();
                    }
                }

                if (p_pipeline->p_video_renderer) {
                    if (DLikely(p_pipeline->p_video_renderer->p_filter)) {
                        p_pipeline->p_video_renderer->p_filter->GetObject0()->PrintStatus();
                    }
                }

                if (p_pipeline->p_audio_renderer) {
                    if (DLikely(p_pipeline->p_audio_renderer->p_filter)) {
                        p_pipeline->p_audio_renderer->p_filter->GetObject0()->PrintStatus();
                    }
                }

            }
            break;

        default:
            LOG_ERROR("TO DO 0x%08x, %s\n", id, gfGetComponentStringFromGenericComponentType(type));
            break;
    }
}

void CActiveGenericEngineV3::doSetRuntimeLogSetting(TGenericID id, TU32 log_level, TU32 log_option, TU32 log_output, TU32 is_add)
{
    TComponentType type = DCOMPONENT_TYPE_FROM_GENERIC_ID(id);
    TComponentType index = DCOMPONENT_INDEX_FROM_GENERIC_ID(id);

    switch (type) {

        case EGenericPipelineType_Streaming: {
                if (DUnlikely(index >= mnStreamingPipelinesCurrentNumber)) {
                    LOGM_FATAL("BAD index %d, exceed max value %d\n", index, mnStreamingPipelinesCurrentNumber);
                    return;
                }

                if (DUnlikely((NULL == mpStreamingPipelines) || (NULL == mpStreamingContent))) {
                    LOGM_FATAL("NULL mpStreamingPipelines %p or mpStreamingContent %p\n", mpStreamingPipelines, mpStreamingContent);
                    return;
                }

                SStreamingPipeline *p_pipeline = mpStreamingPipelines + index;
                LOGM_CONFIGURATION("set log setting, streaming pipeline(%08x), %08x %08x %08x\n", id, log_level, log_option, log_output);
                if (p_pipeline->p_video_source[0]) {
                    if (DLikely(p_pipeline->p_video_source[0]->p_filter)) {
                        p_pipeline->p_video_source[0]->p_filter->GetObject0()->SetLogLevel(log_level);
                        p_pipeline->p_video_source[0]->p_filter->GetObject0()->SetLogOption(log_option);
                        //p_pipeline->p_video_source[0]->p_filter->GetObject0()->SetLogOutput(log_output);
                    }
                }

                if (p_pipeline->p_audio_source[0]) {
                    if (DLikely(p_pipeline->p_audio_source[0]->p_filter)) {
                        p_pipeline->p_audio_source[0]->p_filter->GetObject0()->SetLogLevel(log_level);
                        p_pipeline->p_audio_source[0]->p_filter->GetObject0()->SetLogOption(log_option);
                        //p_pipeline->p_audio_source[0]->p_filter->GetObject0()->SetLogOutput(log_output);
                    }
                }

                if (p_pipeline->p_data_transmiter) {
                    if (DLikely(p_pipeline->p_data_transmiter->p_filter)) {
                        p_pipeline->p_data_transmiter->p_filter->GetObject0()->SetLogLevel(log_level);
                        p_pipeline->p_data_transmiter->p_filter->GetObject0()->SetLogOption(log_option);
                        //p_pipeline->p_data_transmiter->p_filter->GetObject0()->SetLogOutput(log_output);
                    }
                }
            }
            break;

        case EGenericPipelineType_Cloud: {
                if (DUnlikely(index >= mnCloudPipelinesCurrentNumber)) {
                    LOGM_FATAL("BAD index %d, exceed max value %d\n", index, mnCloudPipelinesCurrentNumber);
                    return;
                }

                if (DUnlikely((NULL == mpCloudPipelines) || (NULL == mpCloudContent))) {
                    LOGM_FATAL("NULL mpCloudPipelines %p or mpCloudContent %p\n", mpCloudPipelines, mpCloudContent);
                    return;
                }

                SCloudPipeline *p_pipeline = mpCloudPipelines + index;
                LOGM_CONFIGURATION("set log setting, cloud pipeline(%08x), %08x %08x %08x\n", id, log_level, log_option, log_output);

                if (p_pipeline->p_agent) {
                    if (DLikely(p_pipeline->p_agent->p_filter)) {
                        p_pipeline->p_agent->p_filter->GetObject0()->SetLogLevel(log_level);
                        p_pipeline->p_agent->p_filter->GetObject0()->SetLogOption(log_option);
                        //p_pipeline->p_agent->p_filter->SetLogOutput(log_output);
                    }
                }

                if (p_pipeline->p_video_sink[0]) {
                    if (DLikely(p_pipeline->p_video_sink[0]->p_filter)) {
                        p_pipeline->p_video_sink[0]->p_filter->GetObject0()->SetLogLevel(log_level);
                        p_pipeline->p_video_sink[0]->p_filter->GetObject0()->SetLogOption(log_option);
                        //p_pipeline->p_video_sink[0]->p_filter->SetLogOutput(log_output);
                    }
                }

                if (p_pipeline->p_audio_sink[0]) {
                    if (DLikely(p_pipeline->p_audio_sink[0]->p_filter)) {
                        p_pipeline->p_audio_sink[0]->p_filter->GetObject0()->SetLogLevel(log_level);
                        p_pipeline->p_audio_sink[0]->p_filter->GetObject0()->SetLogOption(log_option);
                        //p_pipeline->p_audio_sink[0]->p_filter->SetLogOutput(log_output);
                    }
                }

            }
            break;

        case EGenericPipelineType_Recording: {
                if (DUnlikely(index >= mnRecordingPipelinesCurrentNumber)) {
                    LOGM_FATAL("BAD index %d, exceed max value %d\n", index, mnRecordingPipelinesCurrentNumber);
                    return;
                }

                if (DUnlikely(NULL == mpRecordingPipelines)) {
                    LOGM_FATAL("NULL mpRecordingPipelines %p\n", mpRecordingPipelines);
                    return;
                }

                SRecordingPipeline *p_pipeline = mpRecordingPipelines + index;
                LOGM_CONFIGURATION("set log setting, recording pipeline(%08x), %08x %08x %08x\n", id, log_level, log_option, log_output);

                if (p_pipeline->p_video_source[0]) {
                    if (DLikely(p_pipeline->p_video_source[0]->p_filter)) {
                        p_pipeline->p_video_source[0]->p_filter->GetObject0()->SetLogLevel(log_level);
                        p_pipeline->p_video_source[0]->p_filter->GetObject0()->SetLogOption(log_option);
                        //p_pipeline->p_video_source[0]->p_filter->GetObject0()->SetLogOutput(log_output);
                    }
                }

                if (p_pipeline->p_audio_source[0]) {
                    if (DLikely(p_pipeline->p_audio_source[0]->p_filter)) {
                        p_pipeline->p_audio_source[0]->p_filter->GetObject0()->SetLogLevel(log_level);
                        p_pipeline->p_audio_source[0]->p_filter->GetObject0()->SetLogOption(log_option);
                        //p_pipeline->p_audio_source[0]->p_filter->GetObject0()->SetLogOutput(log_output);
                    }
                }

                if (p_pipeline->p_sink) {
                    if (DLikely(p_pipeline->p_sink->p_filter)) {
                        p_pipeline->p_sink->p_filter->GetObject0()->SetLogLevel(log_level);
                        p_pipeline->p_sink->p_filter->GetObject0()->SetLogOption(log_option);
                        //p_pipeline->p_sink->p_filter->GetObject0()->SetLogOutput(log_output);
                    }
                }

            }
            break;

        case EGenericPipelineType_Playback: {
                if (DUnlikely(index >= mnPlaybackPipelinesCurrentNumber)) {
                    LOGM_FATAL("BAD index %d, exceed max value %d\n", index, mnPlaybackPipelinesCurrentNumber);
                    return;
                }

                if (DUnlikely(NULL == mpPlaybackPipelines)) {
                    LOGM_FATAL("NULL mpPlaybackPipelines %p\n", mpPlaybackPipelines);
                    return;
                }

                SPlaybackPipeline *p_pipeline = mpPlaybackPipelines + index;
                LOGM_CONFIGURATION("set log setting, playback pipeline(%08x), %08x %08x %08x\n", id, log_level, log_option, log_output);

                if (p_pipeline->p_video_source) {
                    if (DLikely(p_pipeline->p_video_source->p_filter)) {
                        p_pipeline->p_video_source->p_filter->GetObject0()->SetLogLevel(log_level);
                        p_pipeline->p_video_source->p_filter->GetObject0()->SetLogOption(log_option);
                        //p_pipeline->p_video_source->p_filter->SetLogOutput(log_output);
                    }
                }

                if (p_pipeline->p_audio_source) {
                    if (DLikely(p_pipeline->p_audio_source->p_filter)) {
                        p_pipeline->p_audio_source->p_filter->GetObject0()->SetLogLevel(log_level);
                        p_pipeline->p_audio_source->p_filter->GetObject0()->SetLogOption(log_option);
                        //p_pipeline->p_audio_source->p_filter->GetObject0()->SetLogOutput(log_output);
                    }
                }

                if (p_pipeline->p_video_decoder) {
                    if (DLikely(p_pipeline->p_video_decoder->p_filter)) {
                        p_pipeline->p_video_decoder->p_filter->GetObject0()->SetLogLevel(log_level);
                        p_pipeline->p_video_decoder->p_filter->GetObject0()->SetLogOption(log_option);
                        //p_pipeline->p_video_decoder->p_filter->GetObject0()->SetLogOutput(log_output);
                    }
                }

                if (p_pipeline->p_audio_decoder) {
                    if (DLikely(p_pipeline->p_audio_decoder->p_filter)) {
                        p_pipeline->p_audio_decoder->p_filter->GetObject0()->SetLogLevel(log_level);
                        p_pipeline->p_audio_decoder->p_filter->GetObject0()->SetLogOption(log_option);
                        //p_pipeline->p_audio_decoder->p_filter->GetObject0()->SetLogOutput(log_output);
                    }
                }

                if (p_pipeline->p_video_renderer) {
                    if (DLikely(p_pipeline->p_video_renderer->p_filter)) {
                        p_pipeline->p_video_renderer->p_filter->GetObject0()->SetLogLevel(log_level);
                        p_pipeline->p_video_renderer->p_filter->GetObject0()->SetLogOption(log_option);
                        //p_pipeline->p_video_renderer->p_filter->GetObject0()->SetLogOutput(log_output);
                    }
                }

                if (p_pipeline->p_audio_renderer) {
                    if (DLikely(p_pipeline->p_audio_renderer->p_filter)) {
                        p_pipeline->p_audio_renderer->p_filter->GetObject0()->SetLogLevel(log_level);
                        p_pipeline->p_audio_renderer->p_filter->GetObject0()->SetLogOption(log_option);
                        //p_pipeline->p_audio_renderer->p_filter->GetObject0()->SetLogOutput(log_output);
                    }
                }

            }
            break;

        default:
            LOG_ERROR("TO DO 0x%08x, %s\n", id, gfGetComponentStringFromGenericComponentType(type));
            break;
    }
}

TUint CActiveGenericEngineV3::isUniqueComponentCreated(TU8 type)
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

TUint CActiveGenericEngineV3::setupComponentSpecific(TU8 type, IFilter *pFilter, SComponent *p_component)
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

        case EGenericComponentType_AudioCapture:
            if (mpUniqueAudioCapture) {
                LOGM_FATAL("mpUniqueAudioCapture(should be unique) already created!\n");
                return 1;
            }
            mpUniqueAudioCapture = pFilter;
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

        case EGenericComponentType_ConnecterPinMuxer:
            if (mpUniqueAudioPinMuxer) {
                LOGM_FATAL("mpUniqueAudioPinMuxer(should be unique) already created!\n");
                return 1;
            }
            mpUniqueAudioPinMuxer = pFilter;
            mpUniqueAudioPinMuxerComponent = p_component;
            break;

        case EGenericComponentType_CloudConnecterServerAgent:
            setupCloudServerAgent(p_component->id);
            break;

        case EGenericComponentType_CloudConnecterCmdAgent:
            setupCloudCmdAgent(p_component->id);
            break;

        case EGenericComponentType_CloudConnecterClientAgent:
            if (DLikely(p_component->url && p_component->remote_url)) {
                pFilter->FilterProperty(EFilterPropertyType_update_cloud_tag, 1, p_component->url);
                pFilter->FilterProperty(EFilterPropertyType_update_cloud_remote_url, 1, p_component->remote_url);
                pFilter->FilterProperty(EFilterPropertyType_update_cloud_remote_port, 1, &(p_component->remote_port));
            } else {
                LOGM_ERROR("EGenericComponentType_CloudConnecterClientAgent can not without tag and server_addr\n");
                return 0;
            }
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

EECode CActiveGenericEngineV3::parseExtraData(TChar *pFilename, SStreamingSessionContent *pContent)
{
    DASSERT(pContent);
    DASSERT(pFilename);

    EECode err = EECode_OK;
    err = mpExtraDataParser->Initialize(pFilename);
    if (DUnlikely(EECode_OK != err)) {
        return err;
    }

    do {
        err = mpExtraDataParser->FilterProperty(EFilterPropertyType_demuxer_getextradata, 0, (void *)pContent);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_FATAL("get extradata fail!\n");
            break;
        }
    } while (0);
    DASSERT(pContent->has_audio || pContent->has_video);
    mpExtraDataParser->Stop();
    mpExtraDataParser->Finalize();
    return err;
}
DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

//#endif

