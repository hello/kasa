/*******************************************************************************
 * active_generic_engine_v2.h
 *
 * History:
 *    2013/12/28 - [Zhi He] create file
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

#ifndef __ACTIVE_GENERIC_ENGINE_V2_H__
#define __ACTIVE_GENERIC_ENGINE_V2_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

typedef struct {
    TComponentIndex combo_index;
    TComponentIndex data_index;

    TComponentIndex peer1_index;
    TComponentIndex peer2_index;

    SComponent *p_peer1_data;
    SComponent *p_peer1_cmd;
    SComponent *p_peer2_data;
    SComponent *p_peer2_cmd;

    SCloudContent *p_peer1_cloud_cmd_content;
    SCloudContent *p_peer1_cloud_data_content;
} SDualWayRelayCombo;

//-----------------------------------------------------------------------
//
// CActiveGenericEngineV2
//
//-----------------------------------------------------------------------

class CActiveGenericEngineV2
    : public CActiveEngine
    , public IGenericEngineControlV2
    , public IEventListener
{
    typedef CActiveEngine inherited;

public:
    static CActiveGenericEngineV2 *Create(const TChar *pname, TUint index);

private:
    CActiveGenericEngineV2(const TChar *pname, TUint index);
    EECode Construct();
    virtual ~CActiveGenericEngineV2();

public:
    virtual void Delete();
    virtual EECode PostEngineMsg(SMSG &msg);

public:
    virtual EECode InitializeDevice();
    virtual EECode ActivateDevice(TUint mode);
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
    virtual EECode BeginConfigProcessPipeline(TUint customized_pipeline = 1);

    virtual EECode NewComponent(TUint component_type, TGenericID &component_id);
    virtual EECode ConnectComponent(TGenericID &connection_id, TGenericID upper_component_id, TGenericID down_component_id, StreamType pin_type);

    virtual EECode SetupPlaybackPipeline(TGenericID &playback_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID video_decoder_id, TGenericID audio_decoder_id, TGenericID video_renderer_id, TGenericID audio_renderer_id, TGenericID audio_pinmuxer_id = 0, TU8 running_at_startup = 1);
    virtual EECode SetupRecordingPipeline(TGenericID &recording_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID sink_id, TU8 running_at_startup = 1);
    virtual EECode SetupStreamingPipeline(TGenericID &streaming_pipeline_id, TGenericID streaming_transmiter_id, TGenericID streaming_server_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID audio_pinmuxer_id = 0, TU8 running_at_startup = 1);
    virtual EECode SetupCloudPipeline(TGenericID &cloud_pipeline_id, TGenericID cloud_agent_id, TGenericID cloud_server_id, TGenericID video_sink_id, TGenericID audio_sink_id, TGenericID cmd_sink_id = 0, TU8 running_at_startup = 1);

    virtual EECode SetupNativeCapturePipeline(TGenericID &capture_pipeline_id, TGenericID video_capture_source_id, TGenericID audio_capture_source_id, TGenericID video_encoder_id, TGenericID audio_encoder_id, TU8 running_at_startup = 1);
    virtual EECode SetupNativePushPipeline(TGenericID &push_pipeline_id, TGenericID push_source_id, TGenericID video_sink_id, TGenericID audio_sink_id, TU8 running_at_startup = 1);

    virtual EECode SetupCloudUploadPipeline(TGenericID &upload_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID uploader_id, TU8 running_at_startup);

    virtual EECode FinalizeConfigProcessPipeline();

public:
    //set source for source filters
    virtual EECode SetSourceUrl(TGenericID source_component_id, TChar *url);
    //set dst for sink filters
    virtual EECode SetSinkUrl(TGenericID sink_component_id, TChar *url);
    //set url for streaming
    virtual EECode SetStreamingUrl(TGenericID pipeline_id, TChar *url);
    //set tag for cloud client's receiver at server side
    virtual EECode SetCloudAgentTag(TGenericID agent_id, TChar *agent_tag, TGenericID server_id = 0);
    //set tag for cloud client's tag and remote server addr and port
    virtual EECode SetCloudClientTag(TGenericID agent_id, TChar *agent_tag, TChar *p_server_addr, TU16 server_port);

    //couple related
    virtual EECode CoupleCmdData(TGenericID data_agent_id, TGenericID cmd_agent_id);

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

public:
    virtual const TChar *GetLastErrorCodeString() const;

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
    SComponent *newComponent(TComponentType type, TComponentIndex index);
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
    EECode postSourceList(ICloudServerAgent *p_agent);
    EECode aquireChannelControl(TComponentIndex index, TGenericID remote_id);
    EECode releaseChannelControl(TComponentIndex index);

private:
    CIDoubleLinkedList mListFilters;
    CIDoubleLinkedList mListProxys;
    CIDoubleLinkedList mListConnections;

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
    TU8 mbReadLicenceCheckFile;

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
    TUint msState;

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
    CIDoubleLinkedList mStreamingServerList;

    //vod content
    SStreamingSessionContent *mpVodContent;

private:
    //cloud related
    ICloudServerManager *mpCloudServerManager;
    CIDoubleLinkedList mCloudServerList;
    SCloudContent *mpCloudContent;
    SCloudContent *mpCloudCmdContent;

    ICloudServer *mpDebugLastCloudServer;
    IStreamingServer *mpDebugLastStreamingServer;

private:
    SDualWayRelayCombo *mpRelayCombos;
    TComponentIndex mnMaxRelayCombos;
    TComponentIndex mnCurrentRelayCombos;

private:
    EECode mLastErrorCode;
    TUint mErrorCodeStringLength;
    TChar *mpLastErrotCodeString;

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
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif


