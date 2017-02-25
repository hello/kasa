/*******************************************************************************
 * active_generic_engine_v3.h
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

#ifndef __ACTIVE_GENERIC_ENGINE_V3_H__
#define __ACTIVE_GENERIC_ENGINE_V3_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CActiveGenericEngineV3
//
//-----------------------------------------------------------------------

class CActiveGenericEngineV3
    : public CActiveEngine
    , public IGenericEngineControlV3
    , public IEventListener
{
    typedef CActiveEngine inherited;

    typedef struct {
        TChar name[DMAX_ACCOUNT_NAME_LENGTH_EX];
        TU64 dynamic_code;
    } SUserDynamicCode;

public:
    static CActiveGenericEngineV3 *Create(const TChar *pname, TUint index, IStorageManagement *pstorage, SSharedComponent *p_shared_component);

private:
    CActiveGenericEngineV3(const TChar *pname, TUint index, IStorageManagement *pstorage);
    EECode Construct(SSharedComponent *p_shared_component);
    virtual ~CActiveGenericEngineV3();

public:
    virtual void Delete();
    virtual EECode PostEngineMsg(SMSG &msg);

public:
    virtual EECode InitializeDevice();
    virtual EECode ActivateDevice(TU32 mode);
    virtual EECode DeActivateDevice();
    virtual EECode GetMediaConfig(volatile SPersistMediaConfig *&pconfig) const;
    virtual EECode SetMediaConfig();

public:
    //log/debug related api
    virtual EECode SetEngineLogLevel(ELogLevel level);
    virtual EECode SetRuntimeLogConfig(TGenericID id, TU32 level, TU32 option, TU32 output, TU32 is_add = 0);
    virtual EECode PrintCurrentStatus(TGenericID id = 0, TULong print_flag = 0);
    virtual EECode SaveCurrentLogSetting(const TChar *logfile = NULL);

public:
    //build graph related
    virtual EECode BeginConfigProcessPipeline(TU32 customized_pipeline = 1);

    virtual EECode NewComponent(TU32 component_type, TGenericID &component_id, const TChar *prefer_string = NULL);
    virtual EECode ConnectComponent(TGenericID &connection_id, TGenericID upper_component_id, TGenericID down_component_id, StreamType pin_type);

    virtual EECode SetupPlaybackPipeline(TGenericID &playback_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID video_decoder_id, TGenericID audio_decoder_id, TGenericID video_renderer_id, TGenericID audio_renderer_id, TGenericID audio_pinmuxer_id = 0, TU8 running_at_startup = 1);
    virtual EECode SetupRecordingPipeline(TGenericID &recording_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID sink_id, TU8 running_at_startup = 1, TChar *p_channelname = NULL);
    virtual EECode SetupStreamingPipeline(TGenericID &streaming_pipeline_id, TGenericID streaming_transmiter_id, TGenericID streaming_server_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID audio_pinmuxer_id = 0, TU8 running_at_startup = 1);
    virtual EECode SetupCloudPipeline(TGenericID &cloud_pipeline_id, TGenericID cloud_agent_id, TGenericID cloud_server_id, TGenericID video_sink_id, TGenericID audio_sink_id, TGenericID cmd_sink_id = 0, TU8 running_at_startup = 1);

    virtual EECode SetupNativeCapturePipeline(TGenericID &capture_pipeline_id, TGenericID video_capture_source_id, TGenericID audio_capture_source_id, TGenericID video_encoder_id, TGenericID audio_encoder_id, TU8 running_at_startup = 1);
    virtual EECode SetupNativePushPipeline(TGenericID &push_pipeline_id, TGenericID push_source_id, TGenericID video_sink_id, TGenericID audio_sink_id, TU8 running_at_startup = 1);

    virtual EECode SetupCloudUploadPipeline(TGenericID &upload_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID uploader_id, TU8 running_at_startup);

    virtual EECode SetupVODPipeline(TGenericID &vod_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID flow_controller_id, TGenericID streaming_transmiter_id, TGenericID streaming_server_id, TU8 running_at_startup = 0);

    virtual EECode FinalizeConfigProcessPipeline();

public:
    //set source for source filters
    virtual EECode SetSourceUrl(TGenericID source_component_id, TChar *url);
    //set dst for sink filters
    virtual EECode SetSinkUrl(TGenericID sink_component_id, TChar *url);
    //set url for streaming
    virtual EECode SetStreamingUrl(TGenericID pipeline_id, TChar *url);
    //set url for vod
    virtual EECode SetVodUrl(TGenericID source_component_id, TGenericID pipeline_id, TChar *url);
    //set tag for cloud client's receiver at server side
    virtual EECode SetCloudAgentTag(TGenericID agent_id, TChar *agent_tag, TGenericID server_id = 0);
    //set tag for cloud client's tag and remote server addr and port
    virtual EECode SetCloudClientTag(TGenericID agent_id, TChar *agent_tag, TChar *p_server_addr, TU16 server_port);

    //vod content
    virtual EECode SetupVodContent(TUint channel, TChar *url, TU8 localfile, TU8 enable_video, TU8 enable_audio);

    //flow related
    virtual EECode StartProcess();
    virtual EECode StopProcess();
    virtual EECode SetAppMsgCallback(void (*MsgProc)(void *, SMSG &), void *context);

    //pipeline related
    virtual EECode SuspendPipeline(TGenericID pipeline_id, TUint release_content);
    virtual EECode ResumePipeline(TGenericID pipeline_id, TU8 force_audio_path = 0, TU8 force_video_path = 0);

public:
    //configuration
    virtual EECode GenericPreConfigure(EGenericEnginePreConfigure config_type, void *param, TUint is_get = 0);
    virtual EECode GenericControl(EGenericEngineConfigure config_type, void *param);

    virtual EECode GetPipeline(TU8 type, void *&p_pipeline);
    virtual EECode FreePipeline(TU8 type, void *p_pipeline);

    virtual EECode ParseContentExtraData(void *p_content);

public:
    virtual EECode UpdateDeviceDynamicCode(TGenericID agent_id, TU32 dynamic_code);
    virtual EECode UpdateUserDynamicCode(const TChar *p_user_name, const TChar *p_device_name, TU32 dynamic_code);
    virtual EECode LookupPassword(const TChar *p_user_name, const TChar *p_device_name, TU32 &dynamic_code);

public:
    virtual void Destroy();

public:
    virtual void EventNotify(EEventType type, TU64 param1, TPointer param2);

private:
    void nextLogFile();
    void openNextSnapshotFile();

private:
    // IActiveObject
    void OnRun();

private:
    //component & connection
    SComponent *newComponent(TComponentType type, TComponentIndex index, const TChar *prefer_string);
    void deleteComponent(SComponent *p);

    SConnection *newConnetion(TGenericID up_id, TGenericID down_id, SComponent *up_component, SComponent *down_component);
    void deleteConnection(SConnection *p);

    void deactivateComponent(SComponent *p_component, TUint release_flag = 1);
    void activateComponent(SComponent *p_component, TU8 update_focus_index = 0, TComponentIndex focus_index = 0);

    SComponent *findComponentFromID(TGenericID id);
    SComponent *findComponentFromTypeIndex(TU8 type, TU8 index);
    SComponent *findComponentFromFilter(IFilter *p_filter);

    SComponent *queryFilterComponent(TGenericID filter_id);
    SComponent *queryFilterComponent(TComponentType type, TComponentIndex index);
    SConnection *queryConnection(TGenericID connection_id);
    SConnection *queryConnection(TGenericID up_id, TGenericID down_id, StreamType type = StreamType_Invalid);

private:
    //pipeline
    EECode setupPlaybackPipeline(TComponentIndex index);
    EECode setupRecordingPipeline(TComponentIndex index);
    EECode setupStreamingPipeline(TComponentIndex index);
    EECode setupCloudPipeline(TComponentIndex index);
    EECode setupNativeCapturePipeline(TComponentIndex index);
    EECode setupNativePushPipeline(TComponentIndex index);

    EECode setupCloudServerAgent(TGenericID agent_id);
    EECode setupCloudCmdAgent(TGenericID agent_id);

    EECode setupCloudUploadPipeline(TComponentIndex index);
    EECode setupVODPipeline(TComponentIndex index);

private:
    //streaming service
    void setupStreamingServerManger();
    EECode addStreamingServer(TGenericID &server_id, StreamingServerType type, StreamingServerMode mode);
    EECode removeStreamingServer(TGenericID server_id);
    IStreamingServer *findStreamingServer(TGenericID id);

private:
    //cloud service
    void setupCloudServerManger();
    EECode addCloudServer(TGenericID &server_id, CloudServerType type, TU16 port);
    EECode removeCloudServer(TGenericID server_id);
    ICloudServer *findCloudServer(TGenericID id);

private:
    void startServers();
    void stopServers();

private:
    //processing gragh
    EECode createGraph(void);
    EECode clearGraph(void);
    EECode setupPipelines(void);

private:
    //cmd & msg
    bool processGenericMsg(SMSG &msg);
    bool processGenericCmd(SCMD &cmd);
    void processCMD(SCMD &oricmd);

private:
    //filter related
    EECode initializeFilter(SComponent *component, TU8 type);
    EECode initializeAllFilters(TGenericID pipeline_id);
    EECode finalizeAllFilters(TGenericID pipeline_id);

    EECode exitAllFilters(TGenericID pipeline_id);
    EECode startAllFilters(TGenericID pipeline_id);
    EECode runAllFilters(TGenericID pipeline_id);
    EECode stopAllFilters(TGenericID pipeline_id);

    bool allFiltersFlag(TUint flag, TGenericID id);
    void clearAllFiltersFlag(TUint flag, TGenericID id);
    void setFiltersEOS(IFilter *pfilter);

private:
    //scheduler
    EECode createSchedulers(void);
    EECode destroySchedulers(void);

private:
    //flow
    EECode doStartProcess();
    EECode doStopProcess();

    void reconnectAllStreamingServer();
    void doPrintSchedulersStatus();
    void doPrintCurrentStatus(TGenericID id, TULong print_flag);
    void doPrintComponentStatus(TGenericID id);
    void doSetRuntimeLogSetting(TGenericID id, TU32 log_level, TU32 log_option, TU32 log_output, TU32 is_add);

private:
    //to be removed
    TUint isUniqueComponentCreated(TU8 type);
    TUint setupComponentSpecific(TU8 type, IFilter *pFilter, SComponent *p_component);

private:
    EECode parseExtraData(TChar *pFilename, SStreamingSessionContent *pContent);

private:
    CIDoubleLinkedList *mpListFilters;
    CIDoubleLinkedList *mpListProxys;
    CIDoubleLinkedList *mpListConnections;

private:
    TComponentIndex mnPlaybackPipelinesMaxNumber;
    TComponentIndex mnPlaybackPipelinesCurrentNumber;
    TComponentIndex mnRecordingPipelinesMaxNumber;
    TComponentIndex mnRecordingPipelinesCurrentNumber;
    TComponentIndex mnStreamingPipelinesMaxNumber;
    TComponentIndex mnStreamingPipelinesCurrentNumber;
    TComponentIndex mnCloudPipelinesMaxNumber;
    TComponentIndex mnCloudPipelinesCurrentNumber;
    TComponentIndex mnNativeCapturePipelinesMaxNumber;
    TComponentIndex mnNativeCapturePipelinesCurrentNumber;
    TComponentIndex mnNativePushPipelinesMaxNumber;
    TComponentIndex mnNativePushPipelinesCurrentNumber;

    TComponentIndex mnCloudUploadPipelinesMaxNumber;
    TComponentIndex mnCloudUploadPipelinesCurrentNumber;

    TComponentIndex mnVODPipelinesMaxNumber;
    TComponentIndex mnVODPipelinesCurrentNumber;

    SPlaybackPipeline *mpPlaybackPipelines;
    SRecordingPipeline *mpRecordingPipelines;
    SStreamingPipeline *mpStreamingPipelines;
    SCloudPipeline *mpCloudPipelines;

    SNativeCapturePipeline *mpNativeCapturePipelines;
    SNativePushPipeline *mpNativePushPipelines;

    SCloudUploadPipeline *mpCloudUploadPipelines;

    SVODPipeline *mpVODPipelines;

private:
    //dsp/iav related
    IDSPAPI *mpDSPAPI;
    TInt mIavFd;

private:
    TU8 mbSetMediaConfig;
    TU8 mbDeviceOpened;
    TU8 mbDeviceActivated;
    TU8 mbSelfLogFile;

    TU8 mbExternalClock;
    TU8 mbReserved0;
    TU8 mbReserved1;
    TU8 mbReserved2;

private:
    CIClockManager *mpClockManager;
    CIClockReference *mpSystemClockReference;
    IClockSource *mpClockSource;
    IStorageManagement *mpStorageManager;
    TU8 mbRun;
    TU8 mbGraghBuilt;
    TU8 mbSchedulersCreated;
    TU8 mbEnginePipelineRunning;
    IMutex *mpMutex;
    TU32 msState;

    TComponentIndex mnComponentFiltersNumbers[EGenericComponentType_TAG_filter_end - EGenericComponentType_TAG_filter_start];
    TComponentIndex mnComponentFiltersMaxNumbers[EGenericComponentType_TAG_filter_end - EGenericComponentType_TAG_filter_start];
    TComponentIndex mnComponentProxysNumbers[EGenericComponentType_TAG_proxy_end - EGenericComponentType_TAG_proxy_start];
    TComponentIndex mnComponentProxysMaxNumbers[EGenericComponentType_TAG_proxy_end - EGenericComponentType_TAG_proxy_start];

private:
    //some unique components
    IFilter *mpUniqueVideoPostProcessor;
    IFilter *mpUniqueAudioPostProcessor;
    IFilter *mpUniqueVideoRenderer;
    IFilter *mpUniqueAudioRenderer;
    IFilter *mpUniqueAudioDecoder;
    IFilter *mpUniqueVideoCapture;
    IFilter *mpUniqueAudioCapture;
    IFilter *mpUniqueAudioPinMuxer;

    SComponent *mpUniqueVideoPostProcessorComponent;
    SComponent *mpUniqueAudioPostProcessorComponent;
    SComponent *mpUniqueVideoRendererComponent;
    SComponent *mpUniqueAudioRendererComponent;
    SComponent *mpUniqueAudioDecoderComponent;
    SComponent *mpUniqueVideoCaptureComponent;
    SComponent *mpUniqueAudioCaptureComponent;
    SComponent *mpUniqueAudioPinMuxerComponent;

private:
    volatile SPersistMediaConfig mPersistMediaConfig;

private:
    //streaming related
    IStreamingServerManager *mpStreamingServerManager;
    SStreamingSessionContent *mpStreamingContent;
    CIDoubleLinkedList *mpStreamingServerList;

    //vod content
    SStreamingSessionContent *mpVodContent;

private:
    //cloud related
    ICloudServerManager *mpCloudServerManager;
    CIDoubleLinkedList *mpCloudServerList;
    SCloudContent *mpCloudContent;
    SCloudContent *mpCloudCmdContent;

    ICloudServer *mpDebugLastCloudServer;
    IStreamingServer *mpDebugLastStreamingServer;

private:
    TTime mDailyCheckTime;
    TULong mDailyCheckContext;
#ifdef BUILD_CONFIG_WITH_LICENCE
    ILicenceClient *mpLicenceClient;
#endif
    SClockListener *mpDailyCheckTimer;

    SDateTime mLastCheckDate;
    SDateTime mLastSnapshotDate;
    TU32 mCurrentLogFileIndex;
    TU32 mCurrentSnapshotFileIndex;

private:
    IFilter *mpExtraDataParser;//private demuxer
    CSimpleHashTable *mpSimpleHashTable;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif



