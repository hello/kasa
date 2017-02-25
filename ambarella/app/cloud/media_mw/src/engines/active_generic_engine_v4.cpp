/*******************************************************************************
 * active_generic_engine_v4.cpp
 *
 * History:
 *    2014/10/28 - [Zhi He] create file
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
#include "active_generic_engine_v4.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

#ifdef BUILD_CONFIG_WITH_LICENCE
#include "licence.check"
#endif

IGenericEngineControlV4 *CreateGenericEngineV4(IStorageManagement *p_storage, SSharedComponent *p_shared_component = NULL)
{
    return (IGenericEngineControlV4 *)CActiveGenericEngineV4::Create("CActiveGenericEngineV4", 0, p_storage, p_shared_component);
}

//-----------------------------------------------------------------------
//
// CActiveGenericEngineV4
//
//-----------------------------------------------------------------------
CActiveGenericEngineV4 *CActiveGenericEngineV4::Create(const TChar *pname, TUint index, IStorageManagement *pstorage, SSharedComponent *p_shared_component)
{
    CActiveGenericEngineV4 *result = new CActiveGenericEngineV4(pname, index, pstorage);
    if (result && result->Construct(p_shared_component) != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CActiveGenericEngineV4::CActiveGenericEngineV4(const TChar *pname, TUint index, IStorageManagement *pstorage)
    : inherited(pname, index)
    , mnPlaybackPipelinesMaxNumber(EPipelineMaxNumber_Playback)
    , mnPlaybackPipelinesCurrentNumber(0)
    , mnRecordingPipelinesMaxNumber(EPipelineMaxNumber_Recording)
    , mnRecordingPipelinesCurrentNumber(0)
    , mnStreamingPipelinesMaxNumber(EPipelineMaxNumber_Streaming)
    , mnStreamingPipelinesCurrentNumber(0)
    , mnNativeCapturePipelinesMaxNumber(EPipelineMaxNumber_NativeCapture)
    , mnNativeCapturePipelinesCurrentNumber(0)
    , mnCloudUploadPipelinesMaxNumber(EPipelineMaxNumber_CloudUpload)
    , mnCloudUploadPipelinesCurrentNumber(0)
    , mnVODPipelinesMaxNumber(EPipelineMaxNumber_VOD)
    , mnVODPipelinesCurrentNumber(0)
    , mpPlaybackPipelines(NULL)
    , mpRecordingPipelines(NULL)
    , mpStreamingPipelines(NULL)
    , mpNativeCapturePipelines(NULL)
    , mpCloudUploadPipelines(NULL)
    , mpVODPipelines(NULL)
    , mbSetMediaConfig(0)
    , mbSchedulersCreated(0)
    , mRuningCount(0)
    , mbExternalClock(0)
    , mbReconnecting(0)
    , mbLoopPlay(0)
    , mbClockManagerStarted(0)
    , mpClockManager(NULL)
    , mpSystemClockReference(NULL)
    , mpClockSource(NULL)
    , mpStorageManager(pstorage)
    , mbRun(1)
    , mbGraghBuilt(0)
    , mbProcessingRunning(0)
    , mbProcessingStarted(0)
    , mpMutex(NULL)
    , msState(EEngineState_not_alive)
    , mpStreamingServerManager(NULL)
{
    TUint i = 0;
    EECode err = EECode_OK;

    mpStreamingContent = NULL;
    mpVodContent = NULL;
    mpCloudServerManager = NULL;
    mpCloudContent = NULL;
    mDailyCheckTime = 0;
    mDailyCheckContext = 0;
#ifdef BUILD_CONFIG_WITH_LICENCE
    mpLicenceClient = NULL;
#endif
    mpDailyCheckTimer = NULL;
    mCurrentLogFileIndex = 0;
    mCurrentSnapshotFileIndex = 0;

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
    mnComponentFiltersMaxNumbers[EGenericComponentType_ExtVideoESSource] = EComponentMaxNumber_VideoExtSource;
    mnComponentFiltersMaxNumbers[EGenericComponentType_ExtAudioESSource] = EComponentMaxNumber_AudioExtSource;

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

    mPersistMediaConfig.engine_config.engine_version = 4;
    mPersistMediaConfig.engine_config.filter_version = 2;
    mPersistMediaConfig.engine_config.streaming_server_version = 2;

#ifndef BUILD_OS_WINDOWS
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
#endif

    mpListFilters = NULL;
    mpListProxys = NULL;
    mpListConnections = NULL;
    mpStreamingServerList = NULL;
    mpCloudServerList = NULL;

    mbReconnecting = 0;
    mbLoopPlay = 1;

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

EECode CActiveGenericEngineV4::Construct(SSharedComponent *p_shared_component)
{
    DSET_MODULE_LOG_CONFIG(ELogModuleGenericEngine);

    EECode ret = inherited::Construct();
    if (DUnlikely(ret != EECode_OK)) {
        LOGM_FATAL("CActiveGenericEngineV4::Construct, inherited::Construct fail, ret %d\n", ret);
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
        TChar *p_licenc_check_string = (TChar *)DDBG_MALLOC(4096, "E4LC");
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
                DDBG_FREE(p_licenc_check_string, "E4LC");
                p_licenc_check_string = NULL;
                return EECode_Error;
            }
        }
        fclose(p_licence_check_file);
        p_licence_check_file = NULL;
        DDBG_FREE(p_licenc_check_string, "E4LC");
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

#ifndef BUILD_OS_WINDOWS
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
#endif

    TUint request_memsize = sizeof(SPlaybackPipeline) * mnPlaybackPipelinesMaxNumber;
    mpPlaybackPipelines = (SPlaybackPipeline *) DDBG_MALLOC(request_memsize, "E4PP");
    if (DLikely(mpPlaybackPipelines)) {
        memset(mpPlaybackPipelines, 0x0, request_memsize);
    } else {
        mnPlaybackPipelinesMaxNumber = 0;
        LOG_FATAL("no memory, request %d\n", request_memsize);
        return EECode_NoMemory;
    }

    request_memsize = sizeof(SRecordingPipeline) * mnRecordingPipelinesMaxNumber;
    mpRecordingPipelines = (SRecordingPipeline *) DDBG_MALLOC(request_memsize, "E4RP");
    if (DLikely(mpRecordingPipelines)) {
        memset(mpRecordingPipelines, 0x0, request_memsize);
    } else {
        mnRecordingPipelinesMaxNumber = 0;
        LOG_FATAL("no memory, request %d\n", request_memsize);
        return EECode_NoMemory;
    }

    request_memsize = sizeof(SStreamingPipeline) * mnStreamingPipelinesMaxNumber;
    mpStreamingPipelines = (SStreamingPipeline *) DDBG_MALLOC(request_memsize, "E4SP");
    if (DLikely(mpStreamingPipelines)) {
        memset(mpStreamingPipelines, 0x0, request_memsize);
    } else {
        mnStreamingPipelinesMaxNumber = 0;
        LOG_FATAL("no memory, request %d\n", request_memsize);
        return EECode_NoMemory;
    }

    request_memsize = sizeof(SNativeCapturePipeline) * mnNativeCapturePipelinesMaxNumber;
    mpNativeCapturePipelines = (SNativeCapturePipeline *) DDBG_MALLOC(request_memsize, "E4NC");
    if (DLikely(mpNativeCapturePipelines)) {
        memset(mpNativeCapturePipelines, 0x0, request_memsize);
    } else {
        mnNativeCapturePipelinesMaxNumber = 0;
        LOG_FATAL("no memory, request %d\n", request_memsize);
        return EECode_NoMemory;
    }

    request_memsize = sizeof(SCloudUploadPipeline) * mnCloudUploadPipelinesMaxNumber;
    mpCloudUploadPipelines = (SCloudUploadPipeline *)DDBG_MALLOC(request_memsize, "E4CU");
    if (DLikely(mpCloudUploadPipelines)) {
        memset(mpCloudUploadPipelines, 0x0, request_memsize);
    } else {
        mnCloudUploadPipelinesMaxNumber = 0;
        LOG_FATAL("no memory, request %d\n", request_memsize);
        return EECode_NoMemory;
    }

    request_memsize = sizeof(SVODPipeline) * mnVODPipelinesMaxNumber;
    mpVODPipelines = (SVODPipeline *)DDBG_MALLOC(request_memsize, "E4VP");
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
#ifdef DCONFIG_TEST_END2END_DELAY
        mpSystemClockReference = p_shared_component->p_clock_reference;
#endif
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
        mbClockManagerStarted = 1;
        DASSERT_OK(ret);
    }

    if (p_shared_component && p_shared_component->p_msg_queue) {
        mpAppMsgQueue = p_shared_component->p_msg_queue;
    }

    if (!mpSystemClockReference) {
        mpSystemClockReference = CIClockReference::Create();
        if (DUnlikely(!mpSystemClockReference)) {
            LOGM_FATAL("CIClockReference::Create() fail\n");
            return EECode_NoMemory;
        }
    }

    ret = mpClockManager->RegisterClockReference(mpSystemClockReference);
    DASSERT_OK(ret);

    mPersistMediaConfig.p_clock_source = mpClockSource;
    mPersistMediaConfig.p_system_clock_manager = mpClockManager;
    mPersistMediaConfig.p_system_clock_reference = mpSystemClockReference;

    DASSERT(mpWorkQ);
    mpWorkQ->Run();

    mpStreamingContent = (SStreamingSessionContent *)DDBG_MALLOC((mnStreamingPipelinesMaxNumber + mnVODPipelinesMaxNumber) * sizeof(SStreamingSessionContent), "E4SC");
    if (DLikely(mpStreamingContent)) {
        memset(mpStreamingContent, 0x0, (mnStreamingPipelinesMaxNumber + mnVODPipelinesMaxNumber) * sizeof(SStreamingSessionContent));
    } else {
        LOGM_FATAL("No Memory\n");
        return EECode_NoMemory;
    }
    mpVodContent = mpStreamingContent + mnStreamingPipelinesMaxNumber;

    mpCloudContent = (SCloudContent *)DDBG_MALLOC(mnComponentFiltersMaxNumbers[EGenericComponentType_CloudConnecterServerAgent] * sizeof(SCloudContent), "E4CC");
    if (DLikely(mpCloudContent)) {
        memset(mpCloudContent, 0x0, mnComponentFiltersMaxNumbers[EGenericComponentType_CloudConnecterServerAgent] * sizeof(SCloudContent));
    } else {
        LOGM_FATAL("No Memory\n");
        return EECode_NoMemory;
    }

    mDailyCheckTime = mpSystemClockReference->GetCurrentTime();
    mDailyCheckTime += (TTime)24 * 60 * 60 * 1000000;
    mpDailyCheckTimer = mpSystemClockReference->AddTimer(this, EClockTimerType_once, mDailyCheckTime, (TTime)24 * 60 * 60 * 1000000, 1);
    mpDailyCheckTimer->user_context = (TULong)&mDailyCheckContext;

    mPersistMediaConfig.p_storage_manager = mpStorageManager;

    mPersistMediaConfig.p_engine_v4 = (IGenericEngineControlV4 *) this;

    mPersistMediaConfig.p_global_mutex = gfCreateMutex();
    if (DUnlikely(!mPersistMediaConfig.p_global_mutex)) {
        LOGM_FATAL("mPersistMediaConfig.p_global_mutex = gfCreateMutex() fail\n");
    }

    return EECode_OK;
}

CActiveGenericEngineV4::~CActiveGenericEngineV4()
{
    if (mbSelfLogFile) {
        gfCloseLogFile();
    }

    LOGM_RESOURCE("CActiveGenericEngineV4::~CActiveGenericEngineV4 exit.\n");
}

void CActiveGenericEngineV4::Delete()
{
    LOGM_RESOURCE("CActiveGenericEngineV4::Delete, before mpWorkQ->Exit().\n");

    mpWorkQ->Exit();

    LOGM_RESOURCE("CActiveGenericEngineV4::Delete, before delete mpClockManager %p.\n", mpClockManager);

    if (DLikely(mPersistMediaConfig.p_remote_log_server)) {
        mPersistMediaConfig.p_remote_log_server->DestroyContext();
        mPersistMediaConfig.p_remote_log_server->Destroy();
        mPersistMediaConfig.p_remote_log_server = NULL;
    }

#ifndef DCONFIG_TEST_END2END_DELAY
    if (mpSystemClockReference) {
        mpSystemClockReference->Delete();
        mpSystemClockReference = NULL;
    }
#endif

    if (!mbExternalClock) {
        if (mpClockManager) {
            if (mbClockManagerStarted) {
                mpClockManager->Stop();
                mbClockManagerStarted = 0;
            }
            mpClockManager->Delete();
            mpClockManager = NULL;
        }

        if (mpClockSource) {
            mpClockSource->GetObject0()->Delete();
            mpClockSource = NULL;
        }
    }

    LOGM_RESOURCE("CActiveGenericEngineV4::Delete, before StopServerManager.\n");

    if (mpStreamingServerManager) {
        mpStreamingServerManager->Destroy();
        mpStreamingServerManager = NULL;
    }

    if (mpCloudServerManager) {
        mpCloudServerManager->Destroy();
        mpCloudServerManager = NULL;
    }

    LOGM_RESOURCE("CActiveGenericEngineV4::~CActiveGenericEngineV4 before clearGraph().\n");
    clearGraph();
    LOGM_RESOURCE("CActiveGenericEngineV4::~CActiveGenericEngineV4 before destroySchedulers().\n");
    destroySchedulers();

    LOGM_RESOURCE("CActiveGenericEngineV4::Delete, before inherited::Delete().\n");

    if (mpStreamingContent) {
        DDBG_FREE(mpStreamingContent, "E4SC");
        mpStreamingContent = NULL;
        mpVodContent = NULL;
    }

    if (mpCloudContent) {
        DDBG_FREE(mpCloudContent, "E4CC");
        mpCloudContent = NULL;
    }

    if (mpPlaybackPipelines) {
        DDBG_FREE(mpPlaybackPipelines, "E4PP");
        mpPlaybackPipelines = NULL;
    }

    if (mpRecordingPipelines) {
        DDBG_FREE(mpRecordingPipelines, "E4RP");
        mpRecordingPipelines = NULL;
    }

    if (mpStreamingPipelines) {
        DDBG_FREE(mpStreamingPipelines, "E4SP");
        mpStreamingPipelines = NULL;
    }

    if (mpNativeCapturePipelines) {
        DDBG_FREE(mpNativeCapturePipelines, "E4NC");
        mpNativeCapturePipelines = NULL;
    }

    if (mpCloudUploadPipelines) {
        DDBG_FREE(mpCloudUploadPipelines, "E4CU");
        mpCloudUploadPipelines = NULL;
    }

    if (mpVODPipelines) {
        DDBG_FREE(mpVODPipelines, "E4VP");
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
}

EECode CActiveGenericEngineV4::PostEngineMsg(SMSG &msg)
{
    return inherited::PostEngineMsg(msg);
}

EECode CActiveGenericEngineV4::GetMediaConfig(volatile SPersistMediaConfig *&pconfig) const
{
    pconfig = (volatile SPersistMediaConfig *)&mPersistMediaConfig;
    return EECode_OK;
}

EECode CActiveGenericEngineV4::SetMediaConfig()
{
    mbSetMediaConfig = 1;
    return EECode_OK;
}

EECode CActiveGenericEngineV4::SetEngineLogLevel(ELogLevel level)
{
    gfSetLogConfigLevel(level, &gmModuleLogConfig[0], ELogModuleListEnd);

    gfSetLogLevel(level);

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

EECode CActiveGenericEngineV4::SetRuntimeLogConfig(TGenericID id, TU32 level, TU32 option, TU32 output, TU32 is_add)
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

EECode CActiveGenericEngineV4::PrintCurrentStatus(TGenericID id, TULong print_flag)
{
    EECode err = EECode_OK;
    SCMD cmd;

    cmd.code = ECMDType_PrintCurrentStatus;
    cmd.res32_1 = id;
    cmd.res64_1 = print_flag;
    err = mpWorkQ->PostMsg(cmd);

    return err;
}

EECode CActiveGenericEngineV4::BeginConfigProcessPipeline(TUint customized_pipeline)
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

EECode CActiveGenericEngineV4::NewComponent(TUint component_type, TGenericID &component_id, const TChar *prefer_string, void *p_external_context)
{
    SComponent *component = NULL;
    EECode err = EECode_OK;

    if (DLikely((EGenericComponentType_TAG_filter_start <= component_type) && (EGenericComponentType_TAG_filter_end > component_type))) {

        DASSERT(mnComponentFiltersNumbers[component_type] < mnComponentFiltersMaxNumbers[component_type]);
        if (mnComponentFiltersNumbers[component_type] < mnComponentFiltersMaxNumbers[component_type]) {
            component_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(component_type, mnComponentFiltersNumbers[component_type]);

            component = newComponent((TComponentType)component_type, (TComponentIndex)mnComponentFiltersNumbers[component_type], prefer_string, p_external_context);
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
                        component = newComponent((TComponentType)component_type, DCOMPONENT_INDEX_FROM_GENERIC_ID(component_id), prefer_string, p_external_context);
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
                        component = newComponent((TComponentType)component_type,  DCOMPONENT_INDEX_FROM_GENERIC_ID(component_id), prefer_string, p_external_context);
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

EECode CActiveGenericEngineV4::ConnectComponent(TGenericID &connection_id, TGenericID upper_component_id, TGenericID down_component_id, StreamType pin_type)
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

    if (DUnlikely((up_type >= EGenericComponentType_TAG_filter_end) || (down_type >= EGenericComponentType_TAG_filter_end) || (up_index >= mnComponentFiltersNumbers[up_type]) || (down_index >= mnComponentFiltersNumbers[down_type]))) {
        LOG_ERROR("Bad parameters when connect: up id %08x, type [%s], down id %08x, type [%s], up number limit %d, down number limit %d\n",
                  upper_component_id,
                  gfGetComponentStringFromGenericComponentType((TComponentType)up_type),
                  down_component_id,
                  gfGetComponentStringFromGenericComponentType((TComponentType)up_type),
                  mnComponentFiltersNumbers[up_type],
                  mnComponentFiltersNumbers[down_type]);
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

EECode CActiveGenericEngineV4::SetupPlaybackPipeline(TGenericID &playback_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID video_decoder_id, TGenericID audio_decoder_id, TGenericID video_renderer_id, TGenericID audio_renderer_id, TGenericID audio_pinmuxer_id, TU8 running_at_startup)
{
    if (DLikely(mpPlaybackPipelines)) {
        if (DLikely((mnPlaybackPipelinesCurrentNumber + 1) < mnPlaybackPipelinesMaxNumber)) {
            SPlaybackPipeline *p_pipeline = mpPlaybackPipelines + mnPlaybackPipelinesCurrentNumber;
            TUint video_path_enable = 0;
            TUint audio_path_enable = 0;

            if (video_source_id) {

                if (DUnlikely(0 == isComponentIDValid_5(video_source_id, EGenericComponentType_Demuxer, EGenericComponentType_ExtVideoESSource, EGenericComponentType_ExtAudioESSource, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_CloudConnecterClientAgent))) {
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
                if (DUnlikely(0 == isComponentIDValid_6(audio_source_id, EGenericComponentType_Demuxer, EGenericComponentType_ExtVideoESSource, EGenericComponentType_ExtAudioESSource, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_CloudConnecterClientAgent, EGenericComponentType_AudioCapture))) {
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

EECode CActiveGenericEngineV4::SetupRecordingPipeline(TGenericID &recording_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID sink_id, TU8 running_at_startup, TChar *p_channelname)
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
                if (DUnlikely(0 == isComponentIDValid_5(video_source_id, EGenericComponentType_Demuxer, EGenericComponentType_ExtVideoESSource, EGenericComponentType_ExtAudioESSource, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_CloudConnecterClientAgent))) {
                    LOGM_ERROR("not valid video source id %08x\n", video_source_id);
                    return EECode_BadParam;
                }
                video_path_enable = 1;
            }

            if (audio_source_id) {
                if (DUnlikely(0 == isComponentIDValid_5(audio_source_id, EGenericComponentType_Demuxer, EGenericComponentType_ExtVideoESSource, EGenericComponentType_ExtAudioESSource, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_CloudConnecterClientAgent))) {
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

EECode CActiveGenericEngineV4::SetupStreamingPipeline(TGenericID &streaming_pipeline_id, TGenericID streaming_transmiter_id, TGenericID streaming_server_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID audio_pinmuxer_id, TU8 running_at_startup)
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
                if (DUnlikely(0 == isComponentIDValid_3(audio_source_id, EGenericComponentType_Demuxer, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_AudioEncoder))) {
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

EECode CActiveGenericEngineV4::SetupCapturePipeline(TGenericID &capture_pipeline_id, TGenericID video_capture_source_id, TGenericID audio_capture_source_id, TGenericID video_encoder_id, TGenericID audio_encoder_id, TU8 running_at_startup)
{
    if (DLikely(mpNativeCapturePipelines)) {
        if (DLikely((mnNativeCapturePipelinesCurrentNumber + 1) < mnNativeCapturePipelinesMaxNumber)) {
            SNativeCapturePipeline *p_pipeline = mpNativeCapturePipelines + mnNativeCapturePipelinesCurrentNumber;

            if (video_capture_source_id && video_encoder_id) {
                if (DUnlikely(0 == isComponentIDValid(video_capture_source_id, EGenericComponentType_VideoCapture))) {
                    LOGM_ERROR("not valid video_capture_source_id %08x\n", video_capture_source_id);
                    return EECode_BadParam;
                }
                if (DUnlikely(0 == isComponentIDValid(video_encoder_id, EGenericComponentType_VideoEncoder))) {
                    LOGM_ERROR("not valid video_encoder_id %08x\n", video_encoder_id);
                    return EECode_BadParam;
                }
                p_pipeline->video_enabled = 1;
                p_pipeline->video_capture_id = video_capture_source_id;
                p_pipeline->video_encoder_id = video_encoder_id;
            } else {
                p_pipeline->video_enabled = 0;
            }

            if (audio_capture_source_id && audio_encoder_id) {
                if (DUnlikely(0 == isComponentIDValid(audio_capture_source_id, EGenericComponentType_AudioCapture))) {
                    LOGM_ERROR("not valid audio_capture_id %08x\n", audio_capture_source_id);
                    return EECode_BadParam;
                }
                if (DUnlikely(0 == isComponentIDValid(audio_encoder_id, EGenericComponentType_AudioEncoder))) {
                    LOGM_ERROR("not valid audio_encoder_id %08x\n", audio_encoder_id);
                    return EECode_BadParam;
                }
                p_pipeline->audio_enabled = 1;
                p_pipeline->audio_capture_id = audio_capture_source_id;
                p_pipeline->audio_encoder_id = audio_encoder_id;
            } else {
                p_pipeline->audio_enabled = 0;
            }

            if (DUnlikely((!p_pipeline->video_enabled) && (!p_pipeline->audio_enabled))) {
                LOGM_ERROR("both audio and video path are disabled\n");
                return EECode_BadParam;
            }

            p_pipeline->pipeline_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_NativeCapture, mnNativeCapturePipelinesCurrentNumber);
            p_pipeline->index = mnNativeCapturePipelinesCurrentNumber;
            p_pipeline->type = EGenericPipelineType_NativeCapture;
            p_pipeline->pipeline_state = EGenericPipelineState_not_inited;
            p_pipeline->config_running_at_startup = running_at_startup;

            mnNativeCapturePipelinesCurrentNumber ++;
            capture_pipeline_id = p_pipeline->pipeline_id;

            LOGM_NOTICE("[API flow]: capture pipeline[%d]: audio_enable %d, video_enable %d, running_at_startup %d, pipeline id %08x\n", p_pipeline->index, p_pipeline->audio_enabled, p_pipeline->video_enabled, p_pipeline->config_running_at_startup, p_pipeline->pipeline_id);

        } else {
            LOGM_ERROR("too many native capture pipeline %d, max value %d\n", mnNativeCapturePipelinesCurrentNumber, mnNativeCapturePipelinesMaxNumber);
            return EECode_BadParam;
        }
    } else {
        LOGM_FATAL("NULL mpNativeCapturePipelines\n");
        return EECode_InternalLogicalBug;
    }

    LOGM_PRINTF("[Gragh]: SetupCapturePipeline() done, id 0x%08x\n", capture_pipeline_id);

    return EECode_OK;
}

EECode CActiveGenericEngineV4::SetupCloudUploadPipeline(TGenericID &upload_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID uploader_id, TU8 running_at_startup)
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

            p_pipeline->pipeline_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericPipelineType_CloudUpload, mnCloudUploadPipelinesCurrentNumber);
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

EECode CActiveGenericEngineV4::SetupVODPipeline(TGenericID &vod_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID flow_controller_id, TGenericID streaming_transmiter_id, TGenericID streaming_server_id, TU8 running_at_startup)
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

EECode CActiveGenericEngineV4::FinalizeConfigProcessPipeline()
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

EECode CActiveGenericEngineV4::SetSourceUrl(TGenericID source_component_id, TChar *url)
{
    SComponent *p_component = findComponentFromID(source_component_id);
    if (DUnlikely(NULL == p_component)) {
        LOGM_ERROR("SetSourceUrl: cannot find component, id %08x\n", source_component_id);
        return EECode_BadParam;
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
                DDBG_FREE(p_component->url, "E4UR");
            }
            p_component->url = (TChar *)DDBG_MALLOC(url_length + 4, "E4UR");
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
    if (mbProcessingRunning) {
        if (DLikely(NULL == p_component->p_filter)) {
            LOGM_ERROR("mbProcessingRunning, butp_component->p_filter is NULL, in SetSourceUrl?\n");
            return EECode_InternalLogicalBug;
        }
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

EECode CActiveGenericEngineV4::SetSinkUrl(TGenericID sink_component_id, TChar *url)
{
    SComponent *p_component = findComponentFromID(sink_component_id);
    if (DUnlikely(NULL == p_component)) {
        LOGM_ERROR("SetSinkUrl: cannot find component, id %08x\n", sink_component_id);
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
                DDBG_FREE(p_component->url, "E4UR");
            }
            p_component->url = (TChar *)DDBG_MALLOC(url_length + 4, "E4UR");
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
    if (mbProcessingRunning) {
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

EECode CActiveGenericEngineV4::SetStreamingUrl(TGenericID pipeline_id, TChar *url)
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
                DDBG_FREE(p_pipeline->url, "E4UR");
            }
            p_pipeline->url = (TChar *)DDBG_MALLOC(url_length + 4, "E4UR");
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

EECode CActiveGenericEngineV4::SetCloudAgentTag(TGenericID agent_id, TChar *agent_tag, TGenericID server_id)
{
    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(agent_id);
    TComponentType type = DCOMPONENT_TYPE_FROM_GENERIC_ID(agent_id);

    if (1 == isComponentIDValid(agent_id, EGenericComponentType_CloudConnecterServerAgent)) {
        if (DUnlikely(index >= mnComponentFiltersNumbers[EGenericComponentType_CloudConnecterServerAgent])) {
            LOGM_ERROR("BAD agent_id id %08x, index %d exceed max value %d\n", agent_id, index, mnComponentFiltersNumbers[EGenericComponentType_CloudConnecterServerAgent]);
            return EECode_BadParam;
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
                DDBG_FREE(p_component->url, "E4UR");
            }
            p_component->url = (TChar *)DDBG_MALLOC(url_length + 4, "E4UR");
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

EECode CActiveGenericEngineV4::SetCloudClientTag(TGenericID agent_id, TChar *agent_tag, TChar *p_server_addr, TU16 server_port)
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
            DDBG_FREE(p_component->url, "E4UR");
        }
        p_component->url = (TChar *)DDBG_MALLOC(url_length + 4, "E4UR");
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
            DDBG_FREE(p_component->remote_url, "E4UR");
        }
        p_component->remote_url = (TChar *)DDBG_MALLOC(url_length + 4, "E4UR");
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

EECode CActiveGenericEngineV4::SetupVodContent(TUint channel, TChar *url, TU8 localfile, TU8 enable_video, TU8 enable_audio)
{
    LOGM_INFO("SetupVodContent IN\n");

    if (DUnlikely(channel > mnVODPipelinesMaxNumber)) {
        return EECode_BadParam;
    }
    DASSERT(url);

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

        if (!localfile) {
            mpStorageManager->ReleaseExistUint(p_content->session_name, pFilename);
        }

    } else {
        LOGM_ERROR("SetupVodContent, url is NULL\n");
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV4::AssignExternalContext(TGenericID component_id, void *context)
{
    if (DUnlikely(!context)) {
        LOGM_FATAL("NULL p_component in AssignExternalContext()\n");
        return EECode_BadParam;
    }

    if (mbProcessingRunning) {
        LOGM_ERROR("engine is running now\n");
        return EECode_BadState;
    }

    SComponent *p_component = findComponentFromID(component_id);

    if (DUnlikely(!p_component)) {
        LOGM_FATAL("NULL p_component(id = 0x%08x) in AssignExternalContext()\n", component_id);
        return EECode_BadParam;
    }

    if (p_component->p_external_context) {
        LOGM_WARN("already have set context yet!\n");
    }

    if (DLikely(p_component->p_filter)) {
        p_component->p_filter->FilterProperty(EFilterPropertyType_assign_external_module, 1, context);
        p_component->p_external_context = context;
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV4::SetAppMsgCallback(void (*MsgProc)(void *, SMSG &), void *context)
{
    return inherited::SetAppMsgCallback(MsgProc, context);
}

EECode CActiveGenericEngineV4::RunProcessing()
{
    EECode err = EECode_OK;
    SCMD cmd;

    cmd.code = ECMDType_RunProcessing;
    err = mpWorkQ->SendCmd(cmd);

    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("CActiveGenericEngineV4::RunProcessing() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
    }

    return err;
}

EECode CActiveGenericEngineV4::ExitProcessing()
{
    if (DUnlikely(!mbProcessingRunning)) {
        LOGM_ERROR("CActiveGenericEngineV4::ExitProcessing: not in running state\n");
        return EECode_BadState;
    }

    EECode err = EECode_OK;
    SCMD cmd;

    cmd.code = ECMDType_ExitProcessing;
    err = mpWorkQ->SendCmd(cmd);

    if (DLikely(EECode_OK == err)) {
        mbProcessingRunning = 0;
    } else {
        LOGM_ERROR("CActiveGenericEngineV4::ExitProcessing() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
    }

    return err;
}

EECode CActiveGenericEngineV4::Start()
{
    if (DUnlikely(mbProcessingStarted)) {
        LOGM_ERROR("CActiveGenericEngineV4::Start(): already started\n");
        return EECode_BadState;
    }

    EECode err = EECode_OK;
    SCMD cmd;

    cmd.code = ECMDType_Start;
    err = mpWorkQ->SendCmd(cmd);

    if (DLikely(EECode_OK == err)) {
        mbProcessingStarted = 1;
    } else {
        LOGM_ERROR("CActiveGenericEngineV4::Start() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
    }

    return err;
}

EECode CActiveGenericEngineV4::Stop()
{
    if (DUnlikely(!mbProcessingStarted)) {
        LOGM_ERROR("CActiveGenericEngineV4::Stop(): not started\n");
        return EECode_BadState;
    }

    EECode err = EECode_OK;
    SCMD cmd;

    cmd.code = ECMDType_Stop;
    err = mpWorkQ->SendCmd(cmd);

    if (DLikely(EECode_OK == err)) {
        mbProcessingStarted = 0;
    } else {
        LOGM_ERROR("CActiveGenericEngineV4::Stop() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
    }

    return err;
}

EECode CActiveGenericEngineV4::SuspendPipeline(TGenericID pipeline_id, TUint release_content)
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
    } else {
        LOGM_FATAL("BAD pipeline type %d, pipeline_id 0x%08x\n", type, pipeline_id);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV4::ResumePipeline(TGenericID pipeline_id, TU8 force_audio_path, TU8 force_video_path)
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
    } else {
        LOGM_FATAL("BAD pipeline type %d, pipeline_id 0x%08x\n", type, pipeline_id);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV4::GenericPreConfigure(EGenericEnginePreConfigure config_type, void *param, TUint is_get)
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

EECode CActiveGenericEngineV4::GenericControl(EGenericEngineConfigure config_type, void *param)
{
    EECode err = EECode_OK;

    if (!param) {
        LOGM_NOTICE("NULL param for type 0x%08x\n", config_type);
    }

    switch (config_type) {

        case EGenericEngineConfigure_Record_SetSpecifiedInformation: {
                SRecordSpecifiedInfo *pinfo = (SRecordSpecifiedInfo *) param;
                if (DUnlikely(!param)) {
                    LOG_FATAL("NULL parameters in EGenericEngineConfigure_Record_SetSpecifiedInformation\n");
                    return EECode_BadParam;
                }
                DASSERT(EGenericEngineConfigure_Record_SetSpecifiedInformation == pinfo->check_field);
                SComponent *p_component = findComponentFromID(pinfo->id);
                if (DLikely(p_component && p_component->p_filter)) {
                    p_component->p_filter->FilterProperty(EFilterPropertyType_muxer_set_specified_info, 1, param);
                } else {
                    LOG_FATAL("bad id in EGenericEngineConfigure_Record_SetSpecifiedInformation\n");
                    return EECode_BadParam;
                }
            }
            break;

        case EGenericEngineConfigure_Record_GetSpecifiedInformation: {
                SRecordSpecifiedInfo *pinfo = (SRecordSpecifiedInfo *) param;
                if (DUnlikely(!param)) {
                    LOG_FATAL("NULL parameters in EGenericEngineConfigure_Record_GetSpecifiedInformation\n");
                    return EECode_BadParam;
                }
                DASSERT(EGenericEngineConfigure_Record_GetSpecifiedInformation == pinfo->check_field);
                SComponent *p_component = findComponentFromID(pinfo->id);
                if (DLikely(p_component && p_component->p_filter)) {
                    p_component->p_filter->FilterProperty(EFilterPropertyType_muxer_get_specified_info, 1, param);
                } else {
                    LOG_FATAL("bad id in EGenericEngineConfigure_Record_GetSpecifiedInformation\n");
                    return EECode_BadParam;
                }
            }
            break;

        case EGenericEngineConfigure_AudioCapture_Mute: {
                SUserParamType_CaptureAudioMuteControl *mute = (SUserParamType_CaptureAudioMuteControl *) param;
                if (DUnlikely(!param)) {
                    LOG_FATAL("NULL parameters in EGenericEngineConfigure_AudioCapture_Mute\n");
                    return EECode_BadParam;
                }
                SComponent *p_component = findComponentFromID(mute->id);
                if (DLikely(p_component && p_component->p_filter)) {
                    p_component->p_filter->FilterProperty(EFilterPropertyType_audio_capture_mute, 1, param);
                } else {
                    LOG_FATAL("bad id in EGenericEngineConfigure_AudioCapture_Mute\n");
                    return EECode_BadParam;
                }
            }
            break;

        case EGenericEngineConfigure_AudioCapture_UnMute: {
                SUserParamType_CaptureAudioMuteControl *mute = (SUserParamType_CaptureAudioMuteControl *) param;
                if (DUnlikely(!param)) {
                    LOG_FATAL("NULL parameters in EGenericEngineConfigure_AudioCapture_UnMute\n");
                    return EECode_BadParam;
                }
                SComponent *p_component = findComponentFromID(mute->id);
                if (DLikely(p_component && p_component->p_filter)) {
                    p_component->p_filter->FilterProperty(EFilterPropertyType_audio_capture_unmute, 1, param);
                } else {
                    LOG_FATAL("bad id in EGenericEngineConfigure_AudioCapture_UnMute\n");
                    return EECode_BadParam;
                }
            }
            break;

        case EGenericEngineConfigure_VideoDisplay_SwapWindowContent: {
                SVideoPostPSwap *swap = (SVideoPostPSwap *) param;
                DASSERT(EGenericEngineConfigure_VideoDisplay_SwapWindowContent == swap->check_field);

                LOGM_FATAL("TO DO!\n");
                err = EECode_NoImplement;
            }
            break;

        case EGenericEngineConfigure_VideoPlayback_Capture: {
                SPlaybackCapture *playback_capture = (SPlaybackCapture *) param;
                DASSERT(EGenericEngineConfigure_VideoPlayback_Capture == playback_capture->check_field);
            }
            break;

        case EGenericEngineConfigure_VideoPlayback_Zoom_OnDisplay: {
                SComponent *p_video_decoder;
                SPlaybackZoomOnDisplay *videoZoomParas = (SPlaybackZoomOnDisplay *) param;
                DASSERT(videoZoomParas);
                DASSERT(EGenericEngineConfigure_VideoPlayback_Zoom_OnDisplay == videoZoomParas->check_field);

                p_video_decoder = queryFilterComponent(EGenericComponentType_VideoDecoder, videoZoomParas->decoder_id);
                DASSERT(p_video_decoder);
                DASSERT(p_video_decoder->p_filter);
                p_video_decoder->p_filter->FilterProperty(EFilterPropertyType_vdecoder_zoom, 1, (void *)videoZoomParas);
            }
            break;

        case EGenericEngineConfigure_SetVideoExternalDataPostProcessingCallback: {
                SExternalDataProcessingCallback *set_callback = (SExternalDataProcessingCallback *) param;
                DASSERT(set_callback);
                DASSERT(EGenericEngineConfigure_SetVideoExternalDataPostProcessingCallback == set_callback->check_field);
                SComponent *p_component = findComponentFromID(set_callback->id);
                if (p_component && p_component->p_filter) {
                    p_component->p_filter->FilterProperty(EFilterPropertyType_set_post_processing_callback, 1, param);
                } else {
                    LOG_FATAL("bad id in EGenericEngineConfigure_SetExternalDataPostProcessingCallback\n");
                    return EECode_BadParam;
                }
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

        case EGenericEngineConfigure_VideoPlayback_CompensateDelayFromJitter: {
                mPersistMediaConfig.compensate_from_jitter = 1;
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
                }
            }
            break;

        case EGenericEngineConfigure_VideoEncoder_Framerate: {
                SGenericVideoEncoderParam *videoEncParas = (SGenericVideoEncoderParam *)param;
                if (DLikely(videoEncParas)) {
                    DASSERT(EGenericEngineConfigure_VideoEncoder_Framerate == videoEncParas->check_field);
                }
            }
            break;

        case EGenericEngineConfigure_VideoEncoder_DemandIDR: {
                SGenericVideoEncoderParam *videoEncParas = (SGenericVideoEncoderParam *)param;
                if (DLikely(videoEncParas)) {
                    DASSERT(EGenericEngineConfigure_VideoEncoder_DemandIDR == videoEncParas->check_field);
                }
            }
            break;

        case EGenericEngineConfigure_VideoEncoder_GOP: {
                SGenericVideoEncoderParam *videoEncParas = (SGenericVideoEncoderParam *)param;
                if (DLikely(videoEncParas)) {
                    DASSERT(EGenericEngineConfigure_VideoEncoder_GOP == videoEncParas->check_field);
                }
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
            }
            break;

        case EGenericEngineQuery_GetHandler: {
                SQueryGetHandler *ppconfig = (SQueryGetHandler *)param;
                DASSERT(ppconfig);
                DASSERT(EGenericEngineQuery_GetHandler == ppconfig->check_field);

                SComponent *p_component = findComponentFromID(ppconfig->id);
                if (!p_component || !p_component->p_filter) {
                    LOGM_ERROR("EGenericEngineQuery_GetHandler NULL compponent or bad id.\n");
                    return EECode_BadParam;
                }

                err = p_component->p_filter->FilterProperty(EFilterPropertyType_get_external_handler, 1, param);
            }
            break;

        case EGenericEngineConfigure_VideoPlayback_StepPlay: {
                SUserParamStepPlay *step_play = (SUserParamStepPlay *)param;
                DASSERT(step_play);
                DASSERT(EGenericEngineConfigure_VideoPlayback_StepPlay == step_play->check_field);

                SComponent *p_component = findComponentFromID(step_play->component_id);
                if (!p_component || !p_component->p_filter) {
                    LOGM_ERROR("EGenericEngineConfigure_VideoPlayback_StepPlay NULL compponent or bad id.\n");
                    return EECode_BadParam;
                }

                err = p_component->p_filter->FilterProperty(EFilterPropertyType_vrenderer_trickplay_step, 1, param);
            }
            break;

        case EGenericEngineConfigure_VideoPlayback_Pause: {
                SUserParamPause *pause = (SUserParamPause *)param;
                DASSERT(pause);
                DASSERT(EGenericEngineConfigure_VideoPlayback_Pause == pause->check_field);

                SComponent *p_component = findComponentFromID(pause->component_id);
                if (!p_component || !p_component->p_filter) {
                    LOGM_ERROR("EGenericEngineConfigure_VideoPlayback_Pause NULL compponent or bad id.\n");
                    return EECode_BadParam;
                }

                err = p_component->p_filter->FilterProperty(EFilterPropertyType_vrenderer_trickplay_pause, 1, param);
            }
            break;

        case EGenericEngineConfigure_VideoPlayback_Resume: {
                SUserParamResume *resume = (SUserParamResume *)param;
                DASSERT(resume);
                DASSERT(EGenericEngineConfigure_VideoPlayback_Resume == resume->check_field);

                SComponent *p_component = findComponentFromID(resume->component_id);
                if (!p_component || !p_component->p_filter) {
                    LOGM_ERROR("EGenericEngineConfigure_VideoPlayback_Resume NULL compponent or bad id.\n");
                    return EECode_BadParam;
                }

                err = p_component->p_filter->FilterProperty(EFilterPropertyType_vrenderer_trickplay_resume, 1, param);
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

        case EGenericEngineConfigure_RTSPServerNonblock:
            mPersistMediaConfig.rtsp_server_config.enable_nonblock_timeout = 1;
            break;

        case EGenericEngineConfigure_StepPlay: {
                SUserParamStepPlay *step_play = (SUserParamStepPlay *)param;
                DASSERT(step_play);
                DASSERT(EGenericEngineConfigure_StepPlay == step_play->check_field);

                SPlaybackPipeline *p_pipeline = queryPlaybackPipeline(step_play->component_id);
                if (!p_pipeline || !p_pipeline->p_video_renderer || !p_pipeline->p_video_renderer->p_filter) {
                    LOGM_ERROR("EGenericEngineConfigure_StepPlay, not valid pipeline id, %08x.\n", step_play->component_id);
                    return EECode_BadParam;
                }

                err = p_pipeline->p_video_renderer->p_filter->FilterProperty(EFilterPropertyType_vrenderer_trickplay_step, 1, param);
                p_pipeline->video_runtime_paused = 1;

                if (p_pipeline->p_audio_renderer && p_pipeline->p_audio_renderer->p_filter) {
                    if (!p_pipeline->audio_runtime_paused) {
                        LOGM_INFO("pause audio renderer\n");
                        err = p_pipeline->p_audio_renderer->p_filter->FilterProperty(EFilterPropertyType_arenderer_pause, 1, NULL);
                        p_pipeline->audio_runtime_paused = 1;
                    }
                }

                if (p_pipeline->p_audio_source && p_pipeline->p_audio_source->p_filter) {
                    if (!p_pipeline->audio_runtime_disabled) {
                        LOGM_INFO("disable audio\n");
                        p_pipeline->p_audio_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_disable_audio, 1, NULL);
                        p_pipeline->audio_runtime_disabled = 1;
                    }
                }
            }
            break;

        case EGenericEngineConfigure_Pause: {
                SUserParamPause *pause = (SUserParamPause *)param;
                DASSERT(pause);
                DASSERT(EGenericEngineConfigure_Pause == pause->check_field);

                SPlaybackPipeline *p_pipeline = queryPlaybackPipeline(pause->component_id);
                if (!p_pipeline) {
                    LOGM_ERROR("EGenericEngineConfigure_Pause, not valid pipeline id, %08x.\n", pause->component_id);
                    return EECode_BadParam;
                }

                if (p_pipeline->p_video_renderer && p_pipeline->p_video_renderer->p_filter) {
                    if (!p_pipeline->video_runtime_paused) {
                        LOGM_INFO("pause video renderer\n");
                        err = p_pipeline->p_video_renderer->p_filter->FilterProperty(EFilterPropertyType_vrenderer_trickplay_pause, 1, NULL);
                        p_pipeline->video_runtime_paused = 1;
                    }
                }

                if (p_pipeline->p_audio_renderer && p_pipeline->p_audio_renderer->p_filter) {
                    if (!p_pipeline->audio_runtime_paused) {
                        LOGM_INFO("pause audio renderer\n");
                        err = p_pipeline->p_audio_renderer->p_filter->FilterProperty(EFilterPropertyType_arenderer_pause, 1, NULL);
                        p_pipeline->audio_runtime_paused = 1;
                    }
                }
            }
            break;

        case EGenericEngineConfigure_Resume: {
                SUserParamResume *resume = (SUserParamResume *)param;
                DASSERT(resume);
                DASSERT(EGenericEngineConfigure_Resume == resume->check_field);

                SPlaybackPipeline *p_pipeline = queryPlaybackPipeline(resume->component_id);
                if (!p_pipeline) {
                    LOGM_ERROR("EGenericEngineConfigure_Resume, not valid pipeline id, %08x.\n", resume->component_id);
                    return EECode_BadParam;
                }

                if (p_pipeline->p_video_renderer && p_pipeline->p_video_renderer->p_filter) {
                    if (p_pipeline->video_runtime_paused) {
                        LOGM_INFO("resume video renderer\n");
                        err = p_pipeline->p_video_renderer->p_filter->FilterProperty(EFilterPropertyType_vrenderer_trickplay_resume, 1, NULL);
                        p_pipeline->video_runtime_paused = 0;
                    }
                }

                if (p_pipeline->p_audio_renderer && p_pipeline->p_audio_renderer->p_filter) {
                    if (p_pipeline->audio_runtime_paused) {
                        LOGM_INFO("resume audio renderer\n");
                        err = p_pipeline->p_audio_renderer->p_filter->FilterProperty(EFilterPropertyType_arenderer_resume, 1, NULL);
                        p_pipeline->audio_runtime_paused = 0;
                    }
                }

                if (p_pipeline->p_audio_source && p_pipeline->p_audio_source->p_filter) {
                    if (p_pipeline->audio_runtime_disabled) {
                        LOGM_INFO("enable audio\n");
                        p_pipeline->p_audio_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_enable_audio, 1, NULL);
                        p_pipeline->audio_runtime_disabled = 0;
                    }
                }
            }
            break;

        case EGenericEngineConfigure_Seek: {
                SPbSeek *seek = (SPbSeek *)param;
                DASSERT(seek);
                DASSERT(EGenericEngineConfigure_Seek == seek->check_field);

                SPlaybackPipeline *p_pipeline = queryPlaybackPipeline(seek->component_id);
                if (!p_pipeline) {
                    LOGM_ERROR("EGenericEngineConfigure_Seek, not valid pipeline id, %08x.\n", seek->component_id);
                    return EECode_BadParam;
                }

                LOGM_NOTICE("pause pipeline\n");
                pausePlaybackPipeline(p_pipeline);
                LOGM_NOTICE("purge pipeline\n");
                purgePlaybackPipeline(p_pipeline);

                if (p_pipeline->p_video_source && p_pipeline->p_video_source->p_filter) {
                    LOGM_NOTICE("seek video source\n");
                    p_pipeline->p_video_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_seek, 1, param);
                }

                if (p_pipeline->p_audio_source && p_pipeline->p_audio_source->p_filter && (p_pipeline->p_audio_source != p_pipeline->p_video_source)) {
                    LOGM_NOTICE("seek audio source\n");
                    p_pipeline->p_audio_source->p_filter->FilterProperty(EFilterPropertyType_demuxer_seek, 1, param);
                }
                LOGM_NOTICE("resume pipeline\n");
                resumePlaybackPipeline(p_pipeline);

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

EECode CActiveGenericEngineV4::GetPipeline(TU8 type, void *&p_pipeline)
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

EECode CActiveGenericEngineV4::FreePipeline(TU8 type, void *p_pipeline)
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

void CActiveGenericEngineV4::Destroy()
{
    Delete();
}

void CActiveGenericEngineV4::EventNotify(EEventType type, TU64 param1, TPointer param2)
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

void CActiveGenericEngineV4::nextLogFile()
{
    SDateTime date;
    gfGetCurrentDateTime(&date);

    TChar logfile[256] = {0};
    if ((date.year == mLastCheckDate.year) && (date.month == mLastCheckDate.month) && (date.day == mLastCheckDate.day)) {
        mCurrentLogFileIndex ++;
        snprintf(logfile, 256, "./log/dataserver_v4_%04d_%02d_%02d_%04d.log", date.year, date.month, date.day, mCurrentLogFileIndex);
    } else {
        mCurrentLogFileIndex = 0;
        snprintf(logfile, 256, "./log/dataserver_v4_%04d_%02d_%02d.log", date.year, date.month, date.day);
        mLastCheckDate = date;
    }
    gfOpenLogFile((TChar *)logfile);
}

void CActiveGenericEngineV4::openNextSnapshotFile()
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

void CActiveGenericEngineV4::OnRun()
{
    SCMD cmd, nextCmd;
    SMSG msg;
    CIQueue::QType type;
    CIQueue::WaitResult result;
    DASSERT(mpFilterMsgQ);
    DASSERT(mpCmdQueue);
    mpWorkQ->CmdAck(EECode_OK);

    while (mbRun) {
        LOGM_FLOW("CActiveGenericEngineV4: msg cnt from filters %d.\n", mpFilterMsgQ->GetDataCnt());

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

    LOGM_STATE("CActiveGenericEngineV4: start Clear gragh for safe.\n");
    NewSession();
    mpWorkQ->CmdAck(EECode_OK);
}

SComponent *CActiveGenericEngineV4::newComponent(TComponentType type, TComponentIndex index, const TChar *prefer_string, void *p_external_context)
{
    SComponent *p = (SComponent *) DDBG_MALLOC(sizeof(SComponent), "E4CC");

    if (p) {
        memset(p, 0x0, sizeof(SComponent));
        p->id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(type, index);
        p->type = type;
        p->index = index;
        p->cur_state = EComponentState_not_alive;

        if (prefer_string) {
            strncpy(p->prefer_string, prefer_string, DMaxPreferStringLen - 1);
        }
        p->p_external_context = p_external_context;
    }

    return p;
}

void CActiveGenericEngineV4::deleteComponent(SComponent *p)
{
    if (p) {
        if (p->p_filter) {
            p->p_filter->GetObject0()->Delete();
            p->p_filter = NULL;
        }
        if (p->url) {
            DDBG_FREE(p->url, "E4UR");
            p->url = NULL;
        }
        if (p->remote_url) {
            DDBG_FREE(p->remote_url, "E4UR");
            p->remote_url = NULL;
        }
        DDBG_FREE(p, "E4CC");
    }
}

SConnection *CActiveGenericEngineV4::newConnetion(TGenericID up_id, TGenericID down_id, SComponent *up_component, SComponent *down_component)
{
    SConnection *p = (SConnection *) DDBG_MALLOC(sizeof(SConnection), "E4Cc");
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

void CActiveGenericEngineV4::deleteConnection(SConnection *p)
{
    if (p) {
        DDBG_FREE(p, "E4Cc");
    }
}

void CActiveGenericEngineV4::deactivateComponent(SComponent *p_component, TUint release_flag)
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

void CActiveGenericEngineV4::activateComponent(SComponent *p_component, TU8 update_focus_index, TComponentIndex focus_index)
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

SComponent *CActiveGenericEngineV4::findComponentFromID(TGenericID id)
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

    LOGM_ERROR("findComponentFromID: cannot find component, id 0x%08x\n", id);
    return NULL;
}

SComponent *CActiveGenericEngineV4::findComponentFromTypeIndex(TU8 type, TU8 index)
{
    TGenericID id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(type, index);

    return findComponentFromID(id);
}

SComponent *CActiveGenericEngineV4::findComponentFromFilter(IFilter *p_filter)
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

    LOGM_WARN("findComponentFromFilter: cannot find component, by filter %p\n", p_filter);
    return NULL;
}

SComponent *CActiveGenericEngineV4::queryFilterComponent(TGenericID filter_id)
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

SComponent *CActiveGenericEngineV4::queryFilterComponent(TComponentType type, TComponentIndex index)
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

SConnection *CActiveGenericEngineV4::queryConnection(TGenericID connection_id)
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

SConnection *CActiveGenericEngineV4::queryConnection(TGenericID up_id, TGenericID down_id, StreamType type)
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

SPlaybackPipeline *CActiveGenericEngineV4::queryPlaybackPipeline(TGenericID id)
{
    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(id);

    if (DLikely(mpPlaybackPipelines && (mnPlaybackPipelinesCurrentNumber > index))) {
        SPlaybackPipeline *p_pipeline = mpPlaybackPipelines + index;
        return p_pipeline;
    } else {
        LOGM_ERROR("BAD id %08x, mpPlaybackPipelines %p\n", id, mpPlaybackPipelines);
    }

    return NULL;
}

SPlaybackPipeline *CActiveGenericEngineV4::queryPlaybackPipelineFromARID(TGenericID id)
{
    TU32 i = 0;
    SPlaybackPipeline *p_pipeline = NULL;

    for (i = 0; i < mnPlaybackPipelinesCurrentNumber; i ++) {
        p_pipeline = mpPlaybackPipelines + i;
        if (p_pipeline->audio_renderer_id == id) {
            return p_pipeline;
        }
    }

    return NULL;
}

EECode CActiveGenericEngineV4::setupPlaybackPipeline(TComponentIndex index)
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

            if (DUnlikely(0 == isComponentIDValid_5(video_source_id, EGenericComponentType_Demuxer, EGenericComponentType_ExtVideoESSource, EGenericComponentType_ExtAudioESSource, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_CloudConnecterClientAgent))) {
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
            if (DUnlikely(0 == isComponentIDValid_6(audio_source_id, EGenericComponentType_Demuxer, EGenericComponentType_ExtVideoESSource, EGenericComponentType_ExtAudioESSource, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_CloudConnecterClientAgent, EGenericComponentType_AudioCapture))) {
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

EECode CActiveGenericEngineV4::setupRecordingPipeline(TComponentIndex index)
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
            if (DUnlikely(0 == isComponentIDValid_5(video_source_id, EGenericComponentType_Demuxer, EGenericComponentType_ExtVideoESSource, EGenericComponentType_ExtAudioESSource, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_CloudConnecterClientAgent))) {
                LOGM_ERROR("not valid video source id %08x\n", video_source_id);
                return EECode_BadParam;
            }
            p_pipeline->video_enabled = 1;
        } else {
            p_pipeline->video_enabled = 0;
        }

        if (audio_source_id) {
            if (DUnlikely(0 == isComponentIDValid_5(audio_source_id, EGenericComponentType_Demuxer, EGenericComponentType_ExtVideoESSource, EGenericComponentType_ExtAudioESSource, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_CloudConnecterClientAgent))) {
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

EECode CActiveGenericEngineV4::setupStreamingPipeline(TComponentIndex index)
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
        if (DUnlikely(0 == isComponentIDValid_3(p_pipeline->audio_source_id[0], EGenericComponentType_Demuxer, EGenericComponentType_CloudConnecterServerAgent, EGenericComponentType_AudioEncoder))) {
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

EECode CActiveGenericEngineV4::setupCapturePipeline(TComponentIndex index)
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

EECode CActiveGenericEngineV4::setupCloudUploadPipeline(TComponentIndex index)
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

EECode CActiveGenericEngineV4::setupVODPipeline(TComponentIndex index)
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
        p_component->url = (TChar *)DDBG_MALLOC(10, "E4UR");
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
    return EECode_OK;
}

EECode CActiveGenericEngineV4::setupCloudServerAgent(TGenericID agent_id)
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

void CActiveGenericEngineV4::setupStreamingServerManger()
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

EECode CActiveGenericEngineV4::addStreamingServer(TGenericID &server_id, StreamingServerType type, StreamingServerMode mode)
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
        return EECode_OK;
    } else {
        LOGM_ERROR("AddServer(%d, %d) fail\n", type, mode);

        SMSG msg;
        memset(&msg, 0x0, sizeof(msg));
        msg.code = EMSGType_ServerError;
        msg.sessionID = mSessionID;
        PostAppMsg(msg);
    }

    return EECode_Error;
}

EECode CActiveGenericEngineV4::removeStreamingServer(TGenericID server_id)
{
    if (DUnlikely(!mpStreamingServerManager)) {
        LOGM_ERROR("NULL mpStreamingServerManager, but someone invoke CActiveGenericEngineV4::removeStreamingServer?\n");
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

IStreamingServer *CActiveGenericEngineV4::findStreamingServer(TGenericID id)
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

void CActiveGenericEngineV4::setupCloudServerManger()
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

EECode CActiveGenericEngineV4::addCloudServer(TGenericID &server_id, CloudServerType type, TU16 port)
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
        return EECode_OK;
    } else {
        LOGM_ERROR("AddServer(%d, %d) fail\n", type, port);
    }

    return EECode_Error;
}

EECode CActiveGenericEngineV4::removeCloudServer(TGenericID server_id)
{
    if (DUnlikely(!mpCloudServerManager)) {
        LOGM_ERROR("NULL mpCloudServerManager, but someone invoke CActiveGenericEngineV4::removeCloudServer?\n");
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

ICloudServer *CActiveGenericEngineV4::findCloudServer(TGenericID id)
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

void CActiveGenericEngineV4::exitServers()
{
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
}

void CActiveGenericEngineV4::startServers()
{
    if (mpStreamingServerManager) {
        mpStreamingServerManager->Start();
    }

    if (mpCloudServerManager) {
        mpCloudServerManager->Start();
    }
}

void CActiveGenericEngineV4::stopServers()
{
    if (mpStreamingServerManager) {
        mpStreamingServerManager->Stop();
    }

    if (mpCloudServerManager) {
        mpCloudServerManager->Stop();
    }
}

EECode CActiveGenericEngineV4::createGraph(void)
{
    CIDoubleLinkedList::SNode *pnode = mpListFilters->FirstNode();
    SComponent *p_component = NULL;
    SConnection *p_connection = NULL;
    IFilter *pfilter = NULL;
    EECode err;
    TUint uppin_index, uppin_sub_index, downpin_index;

    LOGM_INFO("CActiveGenericEngineV4::createGraph start.\n");

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
                LOGM_FATAL("CActiveGenericEngineV4::createGraph gfFilterFactory(type %d) fail.\n", p_component->type);
                return EECode_InternalLogicalBug;
            }
            setupComponentSpecific(p_component->type, pfilter, p_component);
        } else {
            DASSERT("NULL pointer here, something would be wrong.\n");
        }
        pnode = mpListFilters->NextNode(pnode);
    }

    LOGM_INFO("CActiveGenericEngineV4::createGraph filters construct done, start connection.\n");

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

    LOGM_INFO("CActiveGenericEngineV4::createGraph done.\n");

    mbGraghBuilt = 1;

    return EECode_OK;
}

EECode CActiveGenericEngineV4::setupPipelines(void)
{
    TComponentIndex index = 0;
    EECode err = EECode_OK;

    LOGM_NOTICE("setupPipelines: pb pipelines %d, rec pipelines %d, streaming pipelines %d, vod pipelines %d\n",
                mnPlaybackPipelinesCurrentNumber, mnRecordingPipelinesCurrentNumber, mnStreamingPipelinesCurrentNumber, mnVODPipelinesCurrentNumber);

    DASSERT(mpPlaybackPipelines);
    DASSERT(mpRecordingPipelines);
    DASSERT(mpStreamingPipelines);

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

    if (mpNativeCapturePipelines && mnNativeCapturePipelinesCurrentNumber) {
        for (index = 0; index < mnNativeCapturePipelinesCurrentNumber; index ++) {
            err = setupCapturePipeline(index);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("[Graph, native capture pipeline]: return %d, %s\n", err, gfGetErrorCodeString(err));
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

EECode CActiveGenericEngineV4::clearGraph(void)
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
            LOGM_INFO("destroy component, id %08x, type %d, %s, index %d\n", p_component->id, p_component->type, gfGetComponentStringFromGenericComponentType(p_component->type), p_component->index);
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
            LOGM_INFO("destroy component, id %08x, type %d, %s, index %d\n", p_component->id, p_component->type, gfGetComponentStringFromGenericComponentType(p_component->type), p_component->index);
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

bool CActiveGenericEngineV4::processGenericMsg(SMSG &msg)
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
            LOGM_ERROR("Internal ERROR, NULL pointer, please check code, msg.code 0x%08x\n", msg.code);
            return true;
        }
    } else if (msg.owner_id) {
        p_component = queryFilterComponent(msg.owner_id);
        DASSERT(p_component);
        if (!p_component) {
            LOGM_ERROR("Internal ERROR, NULL pointer, please check code, msg.code 0x%08x\n", msg.code);
            return true;
        }
    }

    switch (msg.code) {

        case EMSGType_Timeout:
            LOGM_INFO("[MSG flow]: Demuxer timeout msg comes, type %d, index %d, GenericID 0x%08x.\n", p_component->type, p_component->index, p_component->id);

            if (!mbReconnecting) {
                DASSERT(p_component->p_filter);
                if (p_component->p_filter) {
                    if (EGenericComponentType_Demuxer == p_component->type) {
                        memset(&msg0, 0x0, sizeof(msg0));
                        msg0.code = EMSGType_RePlay;
                        msg0.sessionID = mSessionID;
                        msg0.owner_id = p_component->id;
                        PostAppMsg(msg0);
                        mbReconnecting = 1;
                        //LOGM_INFO("[DEBUG]: EFilterPropertyType_demuxer_reconnect_server \n");
                        //p_component->p_filter->FilterProperty(EFilterPropertyType_demuxer_reconnect_server, 1, NULL);
                    } else {
                        LOGM_ERROR("need handle here\n");
                    }
                }
            }
            break;

        case EMSGType_PlaybackStatisticsFPS:
            memset(&msg0, 0x0, sizeof(msg0));
            msg0.code = EMSGType_PlaybackStatisticsFPS;
            msg0.sessionID = mSessionID;
            msg0.owner_id = p_component->id;
            msg0.f0 = msg.f0;
            PostAppMsg(msg0);
            break;

        case EMSGType_VideoSize:
            memset(&msg0, 0x0, sizeof(msg0));
            msg0.code = EMSGType_VideoSize;
            msg0.sessionID = mSessionID;
            msg0.p1 = msg.p1;
            msg0.p2 = msg.p2;
            PostAppMsg(msg0);
            break;

        case EMSGType_PlaybackEOS:
            //memset(&msg0, 0x0, sizeof(msg0));
            //msg0.code = EMSGType_PlaybackEOS;
            //msg0.sessionID = mSessionID;
            //msg0.owner_id = p_component->id;
            //PostAppMsg(msg0);
            break;

        case EMSGType_MissionComplete:
            memset(&msg0, 0x0, sizeof(msg0));
            msg0.code = EMSGType_MissionComplete;
            msg0.sessionID = mSessionID;
            msg0.owner_id = p_component->id;
            PostAppMsg(msg0);
            break;

        case EMSGType_ClientReconnectDone:
            memset(&msg0, 0x0, sizeof(msg0));
            msg0.code = EMSGType_ClientReconnectDone;
            msg0.sessionID = mSessionID;
            PostAppMsg(msg0);
            mbReconnecting = 0;
            break;

        case EMSGType_UnknownError:
        case EMSGType_NetworkError:
            if ((!mbReconnecting) && (!mbLoopPlay)) {
                memset(&msg0, 0x0, sizeof(msg0));
                msg0.code = EMSGType_NetworkError;
                msg0.sessionID = mSessionID;
                msg0.owner_id = p_component->id;
                PostAppMsg(msg0);
            } else if (!mbReconnecting) {
                memset(&msg0, 0x0, sizeof(msg0));
                msg0.code = EMSGType_RePlay;
                LOGM_NOTICE("network issue, reconnect now\n");
                msg0.sessionID = mSessionID;
                msg0.owner_id = p_component->id;
                PostAppMsg(msg0);
                mbReconnecting = 1;
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

        case EMSGType_OpenSourceFail:
            LOG_NOTICE("engine EMSGType_OpenSourceFail\n");
            PostAppMsg(msg);
            break;
        case EMSGType_OpenSourceDone:
            LOG_NOTICE("engine EMSGType_OpenSourceDone\n");
            PostAppMsg(msg);
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

            {
                CIDoubleLinkedList::SNode *p_n = mpCloudServerList->FirstNode();
                if (DLikely(p_n && p_n->p_context)) {
                    ICloudServer *p_cloud_server = (ICloudServer *) p_n->p_context;
                    p_cloud_server->GetObject0()->PrintStatus();
                }

                p_n = mpStreamingServerList->FirstNode();
                if (DLikely(p_n && p_n->p_context)) {
                    IStreamingServer *p_streaming_server = (IStreamingServer *) p_n->p_context;
                    p_streaming_server->GetObject0()->PrintStatus();
                }
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

            {
                CIDoubleLinkedList::SNode *p_n = mpCloudServerList->FirstNode();
                if (DLikely(p_n && p_n->p_context)) {
                    ICloudServer *p_cloud_server = (ICloudServer *) p_n->p_context;
                    p_cloud_server->GetObject0()->PrintStatus();
                }

                p_n = mpStreamingServerList->FirstNode();
                if (DLikely(p_n && p_n->p_context)) {
                    IStreamingServer *p_streaming_server = (IStreamingServer *) p_n->p_context;
                    p_streaming_server->GetObject0()->PrintStatus();
                }
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

            {
                CIDoubleLinkedList::SNode *p_n = mpCloudServerList->FirstNode();
                if (DLikely(p_n && p_n->p_context)) {
                    ICloudServer *p_cloud_server = (ICloudServer *) p_n->p_context;
                    p_cloud_server->GetObject0()->PrintStatus();
                }

                p_n = mpStreamingServerList->FirstNode();
                if (DLikely(p_n && p_n->p_context)) {
                    IStreamingServer *p_streaming_server = (IStreamingServer *) p_n->p_context;
                    p_streaming_server->GetObject0()->PrintStatus();
                }
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

            {
                CIDoubleLinkedList::SNode *p_n = mpCloudServerList->FirstNode();
                if (DLikely(p_n && p_n->p_context)) {
                    ICloudServer *p_cloud_server = (ICloudServer *) p_n->p_context;
                    p_cloud_server->GetObject0()->PrintStatus();
                }

                p_n = mpStreamingServerList->FirstNode();
                if (DLikely(p_n && p_n->p_context)) {
                    IStreamingServer *p_streaming_server = (IStreamingServer *) p_n->p_context;
                    p_streaming_server->GetObject0()->PrintStatus();
                }
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

        default:
            LOGM_ERROR("unknown msg code %x\n", msg.code);
            break;
    }

    return true;
}

bool CActiveGenericEngineV4::processGenericCmd(SCMD &cmd)
{
    EECode err = EECode_OK;
    //LOGM_FLOW("[cmd flow]: cmd.code %d, state %d.\n", cmd.code, msState);

    switch (cmd.code) {

        case ECMDType_RunProcessing:
            //LOGM_FLOW("[cmd flow]: ECMDType_RunProcessing cmd, start.\n");
            err = doRunProcessing();
            mpWorkQ->CmdAck(err);
            //LOGM_FLOW("[cmd flow]: ECMDType_RunProcessing cmd, end.\n");
            break;

        case ECMDType_ExitProcessing:
            //LOGM_FLOW("[cmd flow]: ECMDType_ExitProcessing cmd, start.\n");
            err = doExitProcessing();
            mpWorkQ->CmdAck(err);
            //LOGM_FLOW("[cmd flow]: ECMDType_ExitProcessing cmd, end.\n");
            break;

        case ECMDType_Start:
            //LOGM_FLOW("[cmd flow]: ECMDType_Start cmd, start.\n");
            err = doStart();
            mpWorkQ->CmdAck(err);
            //LOGM_FLOW("[cmd flow]: ECMDType_Start cmd, end.\n");
            break;

        case ECMDType_Stop:
            //LOGM_FLOW("[cmd flow]: ECMDType_Stop cmd, start.\n");
            err = doStop();
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

void CActiveGenericEngineV4::processCMD(SCMD &oricmd)
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

EECode CActiveGenericEngineV4::initializeFilter(SComponent *component, TU8 type)
{
    EECode err = EECode_OK;
    IFilter *p_filter = NULL;
    DASSERT(component);

    p_filter = component->p_filter;
    DASSERT(p_filter);

    if (component->p_external_context) {
        err = p_filter->FilterProperty(EFilterPropertyType_assign_external_module, 1, component->p_external_context);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_FATAL("filter->FilterProperty(EFilterPropertyType_assign_external_module) fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        }
    }

    switch (type) {

        case EGenericComponentType_Demuxer:
            if (component->url) {
                err = p_filter->FilterProperty(EFilterPropertyType_update_source_url, 1, (void *) component->url);
                DASSERT_OK(err);
            }
            err = p_filter->Initialize(component->prefer_string);
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

        case EGenericComponentType_VideoCapture:
            if (component->prefer_string[0]) {
                err = p_filter->Initialize((TChar *)component->prefer_string);
            } else {
                err = p_filter->Initialize((TChar *)"WDup");
            }
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

        case EGenericComponentType_VideoOutput:
            if (component->prefer_string[0]) {
                err = p_filter->Initialize((TChar *)component->prefer_string);
            } else {
                err = p_filter->Initialize((TChar *)"Generic");
            }
            DASSERT_OK(err);
            break;

        case EGenericComponentType_AudioOutput:
            if (component->prefer_string[0]) {
                err = p_filter->Initialize((TChar *)component->prefer_string);
            } else {
                err = p_filter->Initialize((TChar *)"Generic");
            }
            DASSERT_OK(err);
            break;

        case EGenericComponentType_FlowController:
            err = p_filter->Initialize();
            break;

        case EGenericComponentType_ExtVideoESSource:
            err = p_filter->Initialize();
            break;

        case EGenericComponentType_ExtAudioESSource:
            err = p_filter->Initialize();
            break;

        default:
            LOGM_FATAL("unknown filter type %d\n", type);
            break;
    }

    if (DLikely(EECode_OK != err)) {
        LOGM_FATAL("filter->Initialize() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV4::initializeAllFilters(TGenericID pipeline_id)
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

EECode CActiveGenericEngineV4::finalizeAllFilters(TGenericID pipeline_id)
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

EECode CActiveGenericEngineV4::runAllFilters(TGenericID pipeline_id)
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

EECode CActiveGenericEngineV4::exitAllFilters(TGenericID pipeline_id)
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
            DASSERT(EComponentState_activated == p_component->cur_state);
            p_filter = (IFilter *)p_component->p_filter;
            if (p_filter) {
                p_filter->Exit();
                p_component->cur_state = EComponentState_initialized;
            }
            pnode = mpListFilters->NextNode(pnode);
        }
        LOGM_INFO(" exit all fiters end\n");
    } else {
        LOGM_ERROR("exitAllFilters: TODO\n");
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV4::startAllFilters(TGenericID pipeline_id)
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

EECode CActiveGenericEngineV4::stopAllFilters(TGenericID pipeline_id)
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
            DASSERT(EComponentState_started == p_component->cur_state);
            p_filter = (IFilter *)p_component->p_filter;
            if (p_filter) {
                p_filter->Stop();
                p_component->cur_state = EComponentState_activated;
            }
            pnode = mpListFilters->NextNode(pnode);
        }
        LOGM_INFO(" stop all fiters end\n");
    } else {
        LOGM_ERROR("stopAllFilters: TODO\n");
    }

    return EECode_OK;
}

bool CActiveGenericEngineV4::allFiltersFlag(TUint flag, TGenericID id)
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

void CActiveGenericEngineV4::clearAllFiltersFlag(TUint flag, TGenericID id)
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

void CActiveGenericEngineV4::setFiltersEOS(IFilter *pfilter)
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

EECode CActiveGenericEngineV4::createSchedulers(void)
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

EECode CActiveGenericEngineV4::destroySchedulers(void)
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

EECode CActiveGenericEngineV4::doRunProcessing()
{
    EECode err;

    if (DUnlikely(0 == mbGraghBuilt)) {
        LOGM_ERROR("doRunProcessing: processing gragh not built yet\n");
        return EECode_BadState;
    }

    if (DUnlikely(mbProcessingRunning)) {
        LOGM_ERROR("doRunProcessing: already in running state\n");
        return EECode_BadState;
    }

    if (DUnlikely(mRuningCount)) {
        LOGM_ERROR("doRunProcessing: already invoke ExitProcessing.\n");
        return EECode_BadState;
    }

    LOGM_NOTICE("doRunProcessing(), start\n");

    if (DLikely(EEngineState_idle == msState)) {
        //start running
        LOGM_INFO("doRunProcessing(), before initializeAllFilters(0)\n");
        err = initializeAllFilters(0);
        if (DUnlikely(err != EECode_OK)) {
            LOGM_ERROR("initializeAllFilters() fail, err %d\n", err);
            return err;
        }
        LOGM_INFO("doRunProcessing(), after initializeAllFilters(0), before runAllFilters(0)\n");

        err = runAllFilters(0);
        if (err != EECode_OK) {
            LOGM_ERROR("runAllFilters() fail, err %d\n", err);
            return err;
        }
        LOGM_INFO("doRunProcessing(), after runAllFilters(0), before startAllFilters(0)\n");

        mbProcessingRunning = 1;
        mRuningCount ++;
        msState = EEngineState_running;
        LOGM_NOTICE("doRunProcessing(), done\n");
    } else if (EEngineState_running == msState) {
        //already running
        LOGM_ERROR("engine is already running state?\n");
        return EECode_BadState;
    } else {
        //bad state
        LOGM_ERROR("BAD engine state %d\n", msState);
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV4::doExitProcessing()
{
    if (DUnlikely(0 == mbGraghBuilt)) {
        LOGM_ERROR("doExitProcessing: processing gragh not built yet\n");
        return EECode_BadState;
    }

    if (DUnlikely(!mbProcessingRunning)) {
        LOGM_ERROR("doExitProcessing: not in running state\n");
        return EECode_BadState;
    }

    LOGM_NOTICE("doExitProcessing(), start\n");

    if (mbProcessingStarted) {
        LOGM_WARN("doExitProcessing: not stopped yet, stop here\n");
        doStop();
    }

    if (DLikely(EEngineState_running == msState)) {
        mPersistMediaConfig.app_start_exit = 1;
        mpClockManager->UnRegisterClockReference(mpSystemClockReference);
        mpSystemClockReference->ClearAllTimers();
        exitServers();
        exitAllFilters(0);
        finalizeAllFilters(0);
        msState = EEngineState_idle;
        LOGM_NOTICE("doExitProcessing(), done\n");
    } else {
        LOGM_ERROR("BAD state %d\n", msState);
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV4::doStart()
{
    EECode err;

    if (DUnlikely(0 == mbGraghBuilt)) {
        LOGM_ERROR("doStart: processing gragh not built yet\n");
        return EECode_BadState;
    }

    if (DUnlikely(!mbProcessingRunning)) {
        LOGM_ERROR("doStart: not in running state\n");
        return EECode_BadState;
    }

    if (DUnlikely(mbProcessingStarted)) {
        LOGM_ERROR("doStart: already started\n");
        return EECode_BadState;
    }

    LOGM_NOTICE("doStart(), start\n");

    if (DLikely(EEngineState_running == msState)) {

        LOGM_INFO("doStart(), before startAllFilters(0)\n");
        err = startAllFilters(0);
        if (err != EECode_OK) {
            LOGM_ERROR("startAllFilters() fail, err %d\n", err);
            stopAllFilters(0);
            return err;
        }
        LOGM_INFO("doStart(), after startAllFilters(0)\n");

        startServers();

        LOGM_NOTICE("doStart(): done\n");
    } else {
        //bad state
        LOGM_ERROR("doStart(): BAD engine state %d\n", msState);
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CActiveGenericEngineV4::doStop()
{
    if (DUnlikely(0 == mbGraghBuilt)) {
        LOGM_ERROR("doStop: processing gragh not built yet\n");
        return EECode_BadState;
    }

    if (DUnlikely(!mbProcessingRunning)) {
        LOGM_ERROR("doStop: not in running state\n");
        return EECode_BadState;
    }

    if (DUnlikely(!mbProcessingStarted)) {
        LOGM_ERROR("doStop: not started yet\n");
        return EECode_BadState;
    }

    LOGM_NOTICE("doStop(), start\n");

    if (DLikely(EEngineState_running == msState)) {
        stopServers();
        stopAllFilters(0);

        if (!mbExternalClock) {
            if (mpClockManager && mbClockManagerStarted) {
                mpClockManager->Stop();
                mbClockManagerStarted = 0;
            }
        }
        LOGM_NOTICE("doStop(), done\n");
    } else {
        LOGM_ERROR("doStop(): BAD state %d\n", msState);
        return EECode_BadState;
    }

    return EECode_OK;
}

void CActiveGenericEngineV4::reconnectAllStreamingServer()
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

void CActiveGenericEngineV4::doPrintSchedulersStatus()
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

void CActiveGenericEngineV4::doPrintCurrentStatus(TGenericID id, TULong print_flag)
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
                CIDoubleLinkedList::SNode *p_n = mpCloudServerList->FirstNode();
                if (DLikely(p_n && p_n->p_context)) {
                    ICloudServer *p_cloud_server = (ICloudServer *) p_n->p_context;
                    p_cloud_server->GetObject0()->PrintStatus();
                }
            }

            if (mpStreamingServerManager) {
                mpStreamingServerManager->PrintStatus0();
                CIDoubleLinkedList::SNode *p_n = mpStreamingServerList->FirstNode();
                if (DLikely(p_n && p_n->p_context)) {
                    IStreamingServer *p_streaming_server = (IStreamingServer *) p_n->p_context;
                    p_streaming_server->GetObject0()->PrintStatus();
                }
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
                CIDoubleLinkedList::SNode *p_n = mpCloudServerList->FirstNode();
                if (DLikely(p_n && p_n->p_context)) {
                    ICloudServer *p_cloud_server = (ICloudServer *) p_n->p_context;
                    p_cloud_server->GetObject0()->PrintStatus();
                }
            }

            if (mpStreamingServerManager) {
                mpStreamingServerManager->PrintStatus0();
                CIDoubleLinkedList::SNode *p_n = mpStreamingServerList->FirstNode();
                if (DLikely(p_n && p_n->p_context)) {
                    IStreamingServer *p_streaming_server = (IStreamingServer *) p_n->p_context;
                    p_streaming_server->GetObject0()->PrintStatus();
                }
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

void CActiveGenericEngineV4::doPrintComponentStatus(TGenericID id)
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

void CActiveGenericEngineV4::doSetRuntimeLogSetting(TGenericID id, TU32 log_level, TU32 log_option, TU32 log_output, TU32 is_add)
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
                        //p_pipeline->p_video_source->p_filter->GetObject0()->SetLogOutput(log_output);
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

TUint CActiveGenericEngineV4::setupComponentSpecific(TU8 type, IFilter *pFilter, SComponent *p_component)
{
    DASSERT(pFilter);
    DASSERT(p_component);

    switch (type) {

        case EGenericComponentType_CloudConnecterServerAgent:
            setupCloudServerAgent(p_component->id);
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

void CActiveGenericEngineV4::pausePlaybackPipeline(SPlaybackPipeline *p_pipeline)
{
    if (p_pipeline->p_video_source && p_pipeline->p_video_source->p_filter) {
        p_pipeline->p_video_source->p_filter->Pause();
    }

    if (p_pipeline->p_audio_source && p_pipeline->p_audio_source->p_filter && (p_pipeline->p_audio_source != p_pipeline->p_video_source)) {
        p_pipeline->p_audio_source->p_filter->Pause();
    }

    if (p_pipeline->p_video_renderer && p_pipeline->p_video_renderer->p_filter) {
        p_pipeline->p_video_renderer->p_filter->Pause();
    }

    if (p_pipeline->p_audio_renderer && p_pipeline->p_audio_renderer->p_filter) {
        p_pipeline->p_audio_renderer->p_filter->Pause();
    }

    if (p_pipeline->p_video_decoder && p_pipeline->p_video_decoder->p_filter) {
        p_pipeline->p_video_decoder->p_filter->Pause();
    }

    if (p_pipeline->p_audio_decoder && p_pipeline->p_audio_decoder->p_filter) {
        p_pipeline->p_audio_decoder->p_filter->Pause();
    }
}

void CActiveGenericEngineV4::resumePlaybackPipeline(SPlaybackPipeline *p_pipeline)
{
    if (p_pipeline->p_video_source && p_pipeline->p_video_source->p_filter) {
        p_pipeline->p_video_source->p_filter->Resume();
    }

    if (p_pipeline->p_audio_source && p_pipeline->p_audio_source->p_filter && (p_pipeline->p_audio_source != p_pipeline->p_video_source)) {
        p_pipeline->p_audio_source->p_filter->Resume();
    }

    if (p_pipeline->p_video_renderer && p_pipeline->p_video_renderer->p_filter) {
        p_pipeline->p_video_renderer->p_filter->Resume();
    }

    if (p_pipeline->p_audio_renderer && p_pipeline->p_audio_renderer->p_filter) {
        p_pipeline->p_audio_renderer->p_filter->Resume();
    }

    if (p_pipeline->p_video_decoder && p_pipeline->p_video_decoder->p_filter) {
        p_pipeline->p_video_decoder->p_filter->Resume();
    }

    if (p_pipeline->p_audio_decoder && p_pipeline->p_audio_decoder->p_filter) {
        p_pipeline->p_audio_decoder->p_filter->Resume();
    }
}

void CActiveGenericEngineV4::purgePlaybackPipeline(SPlaybackPipeline *p_pipeline)
{
    if (p_pipeline->p_video_renderer && p_pipeline->p_video_renderer->p_filter) {
        p_pipeline->p_video_renderer->p_filter->Flush();
    }

    if (p_pipeline->p_audio_renderer && p_pipeline->p_audio_renderer->p_filter) {
        p_pipeline->p_audio_renderer->p_filter->Flush();
    }

    if (p_pipeline->p_video_decoder && p_pipeline->p_video_decoder->p_filter) {
        p_pipeline->p_video_decoder->p_filter->Flush();
    }

    if (p_pipeline->p_audio_decoder && p_pipeline->p_audio_decoder->p_filter) {
        p_pipeline->p_audio_decoder->p_filter->Flush();
    }

    if (p_pipeline->p_video_renderer && p_pipeline->p_video_renderer->p_filter) {
        p_pipeline->p_video_renderer->p_filter->ResumeFlush();
    }

    if (p_pipeline->p_audio_renderer && p_pipeline->p_audio_renderer->p_filter) {
        p_pipeline->p_audio_renderer->p_filter->ResumeFlush();
    }

    if (p_pipeline->p_video_decoder && p_pipeline->p_video_decoder->p_filter) {
        p_pipeline->p_video_decoder->p_filter->ResumeFlush();
    }

    if (p_pipeline->p_audio_decoder && p_pipeline->p_audio_decoder->p_filter) {
        p_pipeline->p_audio_decoder->p_filter->ResumeFlush();
    }
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END



