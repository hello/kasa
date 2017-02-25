/*******************************************************************************
 * active_generic_engine.h
 *
 * History:
 *    2012/12/12 - [Zhi He] create file
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

#ifndef __ACTIVE_GENERIC_ENGINE_H__
#define __ACTIVE_GENERIC_ENGINE_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//for debug
#define DCONFIG_DEBUG_CHECK_INTERNAL_POINTER

typedef struct _SPrivateDataConfig {
    struct _SPrivateDataConfig *p_next;
    TUint duration;
    TU16  data_type;
    TU16 reserved;
} SPrivateDataConfig;

#define DGENERIC_ENGINE_MAX_SOURCE_TO_SAME_MUXER 8

typedef struct {
    TComponentType pipeline_type;
    TU8 audio_stopped;
    TU8 video_stopped;
    TU8 pipeline_state;

    TComponentIndex pipeline_index;
    TComponentIndex source_sink_index;//checking field

    TGenericID streaming_content_id;

    TGenericID pipeline_id;

    //playback
    TGenericID pb_source_id;//demuxer
    TGenericID pb_video_decoder_id;
    TGenericID pb_audio_decoder_id;
    TGenericID pb_vd_inputpin_id;
    TGenericID pb_ad_inputpin_id;
    TGenericID pb_vd_outputpin_id;
    TGenericID pb_ad_outputpin_id;
    //unique module
    TGenericID pb_video_postp_id;
    TGenericID pb_audio_postp_id;
    TGenericID pb_video_renderer_id;
    TGenericID pb_audio_renderer_id;

    //recording
    TGenericID rec_sink_id;
    TComponentIndex rec_source_number;
    TU8 rec_audio_enabled;
    TU8 rec_video_enabled;
    TGenericID rec_source_id[EMaxSourceNumber_Muxer];
    TGenericID rec_source_pin_id[EMaxSourceNumber_Muxer];

    //rtsp recording
    TGenericID rtsp_content_id;
    TGenericID rtsp_server_id;//debug use
    TGenericID rtsp_data_transmiter_id;//debug use
    TComponentIndex rtsp_streaming_source_number;
    TU8 rtsp_streaming_audio_enabled;
    TU8 rtsp_streaming_video_enabled;
    TGenericID rtsp_source_id[EMaxSourceNumber_Streamer];
    TGenericID rtsp_source_pin_id[EMaxSourceNumber_Streamer];

    //multiple instances' shared resource management
    //video
    TComponentIndex mapped_dsp_dec_instance_id;
    TComponentIndex mapped_dsp_enc_instance_id;
    //audio
    TComponentIndex mapped_audio_dec_instance_id;
    TComponentIndex mapped_audio_enc_instance_id;

    TU8 occupied_dsp_dec_instance;
    TU8 occupied_dsp_enc_instance;
    TU8 occupied_audio_dec_instance;
    TU8 occupied_audio_enc_instance;

    //config
    SMuxerConfig *p_muxer_config;

    TU8 pb_video_suspended;
    TU8 pb_audio_suspended;
    TU8 pb_video_running;
    TU8 pb_audio_running;

    //debug use
    void *p_pb_demuxer;
    void *p_pb_video_decoder;
    void *p_pb_audio_decoder;
    void *p_pb_vd_inputpin;
    void *p_pb_ad_inputpin;
    void *p_pb_vd_outputpin;
    void *p_pb_ad_outputpin;
    //unique
    void *p_pb_video_postp;
    void *p_pb_audio_postp;
    void *p_pb_video_renderer;
    void *p_pb_audio_renderer;

    void *p_rec_muxer;
    void *p_rec_source[EMaxSourceNumber_Muxer];
    void *p_rec_source_pin[EMaxSourceNumber_Muxer];
} SGenericPipeline;

//for each source
typedef struct SPlaybackPipelines_s {
    TComponentIndex source_index;
    TComponentIndex pb_pipeline_number;

    SGenericPipeline p_pb_pipeline_head[EMaxSinkNumber_Demuxer];

    struct SPlaybackPipelines_s *p_next;
} SPlaybackPipelines;

//for total engine
typedef struct {
    TComponentIndex rec_pipeline_number;
    TU8 reserved0[2];

    SGenericPipeline p_rec_pipeline_head[EPipelineMaxNumber_Recording];
} SRecordingPipelines;

typedef struct {
    TComponentIndex streaming_pipeline_number;
    TU8 reserved0[2];

    SGenericPipeline p_streaming_pipeline_head[EPipelineMaxNumber_Streaming];
} SStreamingPipelines;

enum {
    EEngineState_not_alive = 0, //not build graph

    EEngineState_idle, //gragh build done, not running
    EEngineState_running, //running
    EEngineState_error,
};

enum {
    EComponentState_not_alive = 0,

    EComponentState_idle,
    EComponentState_running,

    EComponentState_eos_pending,
    EComponentState_suspended,
    EComponentState_error,
};

enum {
    EConnectionState_not_alive = 0,
    EConnectionState_connected,
    EConnectionState_disconnected,
};

typedef struct {
    TGenericID id;

    TComponentIndex index;
    TComponentType type;
    TU8 is_exclusive;//for shared resource management

    TU8 belong2playback_pipeline;
    TU8 belong2recording_pipeline;
    TU8 belong2streaming_pipeline;
    TU8 cur_state;

    TUint status_flag;

    IFilter *p_filter;

    TChar *url;
    TGenericID streaming_video_source;
    TGenericID streaming_audio_source;
    TGenericID streaming_server_id;
    TGenericID streaming_transmiter_id;

    TGenericID cloud_server_id;
    TGenericID cloud_receiver_id;

    TComponentIndex number_of_playback_pipeline;
    TComponentIndex number_of_recording_pipeline;
    TComponentIndex number_of_streaming_pipeline;
    TComponentIndex reserved1;

    //pipelines list
    CIDoubleLinkedList pipelines;
    SGenericPipeline *p_scur_pipeline;
} SComponent_Obsolete;

typedef struct {
    TGenericID connection_id;

    TGenericID up_id;
    TGenericID down_id;


    TComponentIndex up_pin_index;
    TComponentIndex up_pin_sub_index;
    TComponentIndex down_pin_index;

    SComponent_Obsolete *up;
    SComponent_Obsolete *down;

    TU8 state;
    TU8 pin_type;//StreamType
    TU8 reserved0[2];
} SConnection_Obsolete;

typedef struct {
    TGenericID id;

    TComponentIndex index;
    TU8 reserved0;
    TU8 reserved1;

    void *user_context;
    void *user_parameters;

    TFTimerCallback cb;

    SClockListener *mpClockListener;
} SUserTimerContext;

//-----------------------------------------------------------------------
//
// CActiveGenericEngine
//
//-----------------------------------------------------------------------

class CActiveGenericEngine
    : public CActiveEngine
    , public IGenericEngineControl
    , public IEventListener
{
    typedef CActiveEngine inherited;

public:
    static CActiveGenericEngine *Create(const TChar *pname, TUint index);

private:
    CActiveGenericEngine(const TChar *pname, TUint index);
    EECode Construct();
    virtual ~CActiveGenericEngine();

public:
    virtual void Delete();

    virtual EECode PostEngineMsg(SMSG &msg) {
        return inherited::PostEngineMsg(msg);
    }

    virtual EECode SetWorkingMode(EMediaWorkingMode preferedWorkingMode, EPlaybackMode playback_mode);
    virtual EECode SetDisplayDevice(TUint deviceMask);
    virtual EECode SetInputDevice(TUint deviceMask);

    virtual EECode SetEngineLogLevel(ELogLevel level);
    virtual EECode PrintCurrentStatus(TGenericID id = 0, TULong print_flag = 0);
    virtual EECode SaveCurrentLogSetting(const TChar *logfile = NULL);

    virtual EECode BeginConfigProcessPipeline(TUint customized_pipeline);
    virtual EECode NewComponent(TUint component_type, TGenericID &component_id);
    virtual EECode ConnectComponent(TGenericID &connection_id, TGenericID upper_component_id, TGenericID down_component_id, StreamType pin_type);
    virtual EECode SetupStreamingContent(TGenericID content_id, TGenericID server_id, TGenericID transmiter_id, TGenericID video_source_component_id, TGenericID audio_source_component_id, TChar *streaming_tag);
    virtual EECode SetupUploadingReceiverContent(TGenericID content_id, TGenericID server_id, TGenericID receiver_id, TChar *receiver_tag);
    virtual EECode FinalizeConfigProcessPipeline();

    virtual EECode QueryPipelineID(TU8 pipeline_type, TGenericID source_sink_id, TGenericID &pipeline_id, TU8 index = 0);
    virtual EECode QueryPipelineID(TU8 pipeline_type, TU8 source_sink_index, TGenericID &pipeline_id, TU8 index = 0);

    virtual EECode SetDataSource(TGenericID component_id, TChar *url);
    virtual EECode SetSinkDst(TGenericID component_id, TChar *file_dst);
    virtual EECode SetStreamingUrl(TGenericID content_id, TChar *streaming_tag);
    virtual EECode SetCloudClientRecieverTag(TGenericID receiver_id, TChar *receiver_tag);

    //canbe runtime config
    virtual EECode EnableConnection(TGenericID connection_id, TUint enable);

    //flow related
    virtual EECode StartProcess();
    virtual EECode StopProcess();
    virtual EECode SetAppMsgCallback(void (*MsgProc)(void *, SMSG &), void *context);

    //pb related
    virtual EECode PBSpeed(TGenericID pb_pipeline, TU16 speed, TU16 speed_frac, TU8 backward = 0, DecoderFeedingRule feeding_rule = DecoderFeedingRule_AllFrames);

    virtual EECode GenericControl(EGenericEngineConfigure config_type, void *param);
    virtual EECode GenericPreConfigure(EGenericEnginePreConfigure config_type, void *param, TUint is_get = 0);

    //virtual EECode PBRenderSwitch(TUint render_id, TUint new_dec_id, TUint seamless = 0, TUint wait_switch_done = 1);
    //virtual EECode PBUpdateWindowConfig(TUint win_id, TU16 offset_x, TU16 offset_y, TU16 size_x, TU16 size_y);
    //virtual EECode PBUpdateDisplayConfig(TUint new_vout_mask, TUint layout, TUint keep_cur_layout = 0);

    //rec related
    virtual EECode RECStop(TGenericID rec_pipeline);
    virtual EECode RECStart(TGenericID rec_pipeline);

    virtual EECode RECMuxerFinalizeFile(TGenericID rec_pipeline);
    virtual EECode RECSavingStrategy(TGenericID rec_pipeline, MuxerSavingFileStrategy strategy, MuxerSavingCondition condition = MuxerSavingCondition_InputPTS, MuxerAutoFileNaming naming = MuxerAutoFileNaming_ByNumber, TUint param = DefaultAutoSavingParam);
    virtual EECode RECSetupOutput(TUint out_index, const char *pFileName, ContainerType out_format) {return EECode_OK;}

    //pipeline related
    virtual EECode SuspendPipeline(TGenericID pb_pipeline);
    virtual EECode ResumePipeline(TGenericID pb_pipeline);

    virtual void PrintStatus();

    //timer related
    virtual TTime UserGetCurrentTime() const;
    virtual TGenericID AddUserTimer(TFTimerCallback cb, void *thiz, void *param, EClockTimerType type, TTime time, TTime interval, TUint count);
    virtual EECode RemoveUserTimer(TGenericID user_timer_id);

    virtual void EventNotify(EEventType type, TU64 param1, TPointer param2);

    virtual void Destroy();

private:
    void stopServers();

private:
    SUserTimerContext *allocUserTimerContext();
    void releaseUserTimerContext(SUserTimerContext *context);

private:
    EECode addStreamingServer(TGenericID &server_id, StreamingServerType type, StreamingServerMode mode);
    EECode removeStreamingServer(TGenericID server_id);
    EECode newStreamingContent(TGenericID &content_id);
    EECode setupStreamingContent(TGenericID content_id, TGenericID server_id, TGenericID transmiter_id, TChar *url, TU8 has_video, TU8 has_audio, TComponentIndex track_video_input_index, TComponentIndex track_audio_input_index);
    EECode updateStreamingContent(TGenericID content_id, TChar *name, TUint enable);
    void setupDefaultVideoSubsession(SStreamingSubSessionContent *p_content, StreamFormat format, TComponentIndex input_index);
    void setupDefaultAudioSubsession(SStreamingSubSessionContent *p_content, StreamFormat format, TComponentIndex input_index);
    //EECode findNextStreamIndex(TUint muxer_index, TUint& index, StreamType type) {return EECode_OK;}
    //SStreamingSessionContent* findStreamingContentByIndex(TUint output_index);
    //SStreamingSessionContent* generateStreamingContent(TUint output_index);
    IStreamingServer *findStreamingServer(TGenericID id);

private:
    void setupCloudServerManger();
    EECode addCloudServer(TGenericID &server_id, CloudServerType type, TU16 port);
    EECode removeCloudServer(TGenericID server_id);
    ICloudServer *findCloudServer(TGenericID id);

    EECode newCloudReceiverContent(TGenericID &content_id);
    EECode setupCloudReceiverContent(TGenericID content_id, TGenericID server_id, TGenericID receiver_id, TChar *url);

private:
    // IActiveObject
    void OnRun();
    bool allFiltersFlag(TUint flag, TGenericID id);
    void clearAllFiltersFlag(TUint flag, TGenericID id);
    void setFiltersEOS(IFilter *pfilter);
    EECode defaultVideoParameters(TUint out_index, TUint stream_index, TUint ucode_stream_index);
    EECode defaultAudioParameters(TUint out_index, TUint stream_index);
    EECode checkParameters(void);
    EECode setOutputFile(TUint index);
    EECode setThumbNailFile(TUint index);
    //EECode findFirstVideoStreamInMuxer(TUint muxer_index, TUint& video_index);

    EECode getParametersFromRecConfig();

    bool allvideoDisabled();
    bool allaudioDisabled();
    bool allpridataDisabled();
    bool allstreamingDisabled();
    SPrivateDataConfig *findConfigFromPrivateDataType(TU16 data_type);
    void setupStreamingServerManger();

private:
    EECode createGraph(void);
    EECode clearGraph(void);
    bool processGenericMsg(SMSG &msg);
    bool processGenericCmd(SCMD &cmd);
    void processCMD(SCMD &oricmd);

    EECode runAllFilters(TGenericID pipeline_id);
    EECode exitAllFilters(TGenericID pipeline_id);
    EECode startAllFilters(TGenericID pipeline_id);
    EECode stopAllFilters(TGenericID pipeline_id);

    EECode initializeFilter(SComponent_Obsolete *component, TU8 type);
    EECode initializeAllFilters(TGenericID pipeline_id);
    EECode finalizeAllFilters(TGenericID pipeline_id);
    void reconnectAllStreamingServer();

    void mappingPlaybackPipeline2Component();
    void mappingRecordingPipeline2Component();
    void mappingRTSPStreamingPipeline2Component();

    EECode doStartProcess();
    EECode doStopProcess();
    EECode initialSetting();

    void doPrintCurrentStatus(TGenericID id, TULong print_flag);

private:
    SComponent_Obsolete *newComponent(TComponentType type, TComponentIndex index);
    SConnection_Obsolete *newConnetion(TGenericID up_id, TGenericID down_id, SComponent_Obsolete *up_component, SComponent_Obsolete *down_component);

    void deleteComponent(SComponent_Obsolete *p);
    void deleteConnection(SConnection_Obsolete *p);

    SComponent_Obsolete *findComponentFromID(TGenericID id);
    SComponent_Obsolete *findComponentFromTypeIndex(TU8 type, TU8 index);
    SComponent_Obsolete *findComponentFromFilter(IFilter *p_filter);

    //SConnection_Obsolete* findConnectionFromUpDownID(TGenericID up_id, TGenericID down_id);
    //SConnection_Obsolete* findConnectionFromUpDownComponent(SComponent_Obsolete* up_component, SComponent_Obsolete* down_component);

private:
    SPlaybackPipelines *newPlaybackPipelines();
    void deletePlaybackPipelines(SPlaybackPipelines *pipelines);
    SGenericPipeline *queryRecordingPipeline(TGenericID sink_id);
    SGenericPipeline *queryPlaybackPipelineFromSourceID(TGenericID source_id, TU8 index);
    SGenericPipeline *queryPlaybackPipeline(TGenericID pipeline_id);
    SGenericPipeline *queryRTSPStreamingPipeline(TGenericID content_id);
    SGenericPipeline *queryPipeline(TGenericID pipeline_id);
    SComponent_Obsolete *queryFilter(TGenericID filter_id);
    SComponent_Obsolete *queryFilter(TComponentType type, TComponentIndex index);
    SConnection_Obsolete *queryConnection(TGenericID connection_id);
    SConnection_Obsolete *queryConnection(TGenericID up_id, TGenericID down_id, CIDoubleLinkedList::SNode *&start_node, StreamType type = StreamType_Invalid);

    SGenericPipeline *getRecordingPipeline(TComponentIndex sink_index);
    SPlaybackPipelines *getPlaybackPipelines(TComponentIndex source_index);
    SGenericPipeline *getStreamingPipeline(TGenericID id);

private:
    void setupInternalPlaybackPipelineInfos(SComponent_Obsolete *p_component);
    void setupInternalRecordingPipelineInfos(SComponent_Obsolete *p_component);
    void setupInternalRTSPStreamingPipelineInfos(SComponent_Obsolete *p_component);
    void setupInternalPipelineInfos();

private:
    EECode componentNumberStatistics();

private:
    TUint isUniqueComponentCreated(TU8 type);
    TUint setUniqueComponent(TU8 type, IFilter *pFilter, SComponent_Obsolete *p_component);

private:
    EECode selectDSPMode();
    EECode autoSelectDSPMode();
    EECode setupDSPConfig(TUint mode);
    EECode activateWorkingMode(TUint mode);
    void summarizeVoutParams();
    EECode setupMUdecDSPSettings();
    EECode setupUdecDSPSettings();

private:
    EECode presetPlaybackModeForDSP(TU16 num_of_instances_0, TU16 num_of_instances_1, TUint max_width_0, TUint max_height_0, TUint max_width_1, TUint max_height_1);

private:
    EECode createSchedulers(void);
    EECode destroySchedulers(void);

private:
    void suspendPlaybackPipelines(TUint start_index, TUint end_index, TUint release_context);
    EECode resumeSuspendPlaybackPipelines(TUint start_index, TUint end_index, TComponentIndex focus_input_index);
    void suspendPlaybackPipelines(TUint start_index_for_vid_decode, TUint end_index_for_vid_decode, TUint start_index_for_vid_demux, TUint end_index_for_vid_demux, TUint release_context);
    EECode resumeSuspendPlaybackPipelines(TUint start_index_for_vid_decode, TUint end_index_for_vid_decode, TUint start_index_for_vid_demux, TUint end_index_for_vid_demux, TComponentIndex focus_input_index);
    void autoStopBackgroudVideoPlayback();

private:
    void initializeSyncStatus();
    void initializeUrlGroup();
    EECode receiveSyncStatus(ICloudServerAgent *p_agent, TMemSize payload_size);
    EECode sendSyncStatus(ICloudServerAgent *p_agent);
    EECode updateSyncStatus();

    EECode processUpdateMudecDisplayLayout(TU32 layout, TU32 focus_index);
    EECode processZoom(TU32 render_id, TU32 width, TU32 height, TU32 input_center_x, TU32 input_center_y);
    EECode processZoom(TU32 window_id, TU32 zoom_factor, TU32 offset_center_x, TU32 offset_center_y);
    EECode processMoveWindow(TU32 window_id, TU32 pos_x, TU32 pos_y, TU32 width, TU32 height);
    EECode processShowOtherWindows(TU32 window_index, TU32 show);
    EECode sendCloudAgentVersion(ICloudServerAgent *p_agent, TU32 param0, TU32 param1);
    EECode processDemandIDR(TU32 param1);
    EECode processUpdateBitrate(TU32 param1);
    EECode processUpdateFramerate(TU32 param1);
    EECode processSwapWindowContent(TU32 window_id_1, TU32 window_id_2);
    EECode processCircularShiftWindowContent(TU32 shift, TU32 is_ccw);

    void resetZoomParameters();

private:
    CIDoubleLinkedList mListFilters;
    CIDoubleLinkedList mListProxys;
    CIDoubleLinkedList mListConnections;

private:
    //dsp/iav related
    IDSPAPI *mpDSPAPI;
    TInt mIavFd;

    TComponentIndex mnDemuxerNum;
    TComponentIndex mnMuxerNum;
    TComponentIndex mnVideoDecoderNum;
    TComponentIndex mnVideoEncoderNum;
    TComponentIndex mnAudioDecoderNum;
    TComponentIndex mnAudioEncoderNum;

    TComponentIndex mnStreamingTransmiterNum;

    TComponentIndex mnRTSPStreamingServerNum;
    TComponentIndex mnHLSStreamingServerNum;
    //TComponentIndex mnStreamingServerIndex;

    TU8 mbAutoNamingRTSPContextTag;
    TU8 mbEnginePipelineRunning;
    TU8 mbReserved4;
    TU8 mbReserved5;

private:
    CIClockManager *mpClockManager;
    CIClockReference *mpSystemClockReference;
    IClockSource *mpClockSource;
    TU8 mbRun;
    TU8 mbGraghBuilt;
    TU8 mbPipelineInfoGet;
    TU8 mbSchedulersCreated;
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
    IFilter *mpUniqueRTSPStreamingTransmiter;
    IFilter *mpUniqueVideoCapture;
    IFilter *mpUniqueAudioCapture;
    IFilter *mpUniqueAudioPinMuxer;

    SComponent_Obsolete *mpUniqueVideoPostProcessorComponent;
    SComponent_Obsolete *mpUniqueAudioPostProcessorComponent;
    SComponent_Obsolete *mpUniqueVideoRendererComponent;
    SComponent_Obsolete *mpUniqueAudioRendererComponent;
    SComponent_Obsolete *mpUniqueAudioDecoderComponent;
    SComponent_Obsolete *mpUniqueRTSPStreamingTransmiterComponent;
    SComponent_Obsolete *mpUniqueVideoCaptureComponent;
    SComponent_Obsolete *mpUniqueAudioCaptureComponent;
    SComponent_Obsolete *mpUniqueAudioPinMuxerComponent;

private:
    volatile SPersistMediaConfig mPersistMediaConfig;

private:
    EMediaWorkingMode mRequestWorkingMode;
    EDSPOperationMode mRequestDSPMode;
    EPlaybackMode mPresetPlaybackMode;
    TU16 mRequestVoutMask;
    TU16 mVoutStartIndex;
    TU16 mVoutNumber;
    TU8 mbDSPSuspended;//idle
    TU8 mbReserved0;

    TUint mMaxVoutWidth;
    TUint mMaxVoutHeight;

private:
    TU8 mbAllVideoDisabled;
    TU8 mbAllAudioDisabled;
    TU8 mbAllPridataDisabled;
    TU8 mbAllStreamingDisabled;

private:
    TU16 mTotalMuxerNumber;
    TU16 mTotalVideoDecoderNumber;
    TU16 mMultipleWindowDecoderNumber;
    TU16 mSingleWindowDecoderNumber;

private:
    //muxer related
    SMuxerConfig mMuxerConfig[EComponentMaxNumber_Muxer];

private:
    //decoder related
    SDSPUdecInstanceConfig mVideoDecoder[DMaxUDECInstanceNumber];

    //postp related input params
    TU8 mInitialVideoDisplayMask;
    TU8 mInitialVideoDisplayPrimaryIndex;
    TU8 mInitialVideoDisplayCurrentMask;
    TU8 mInitialVideoDisplayLayerNumber;

    TU8 mInitialVideoDisplayLayoutType[DMaxDisplayLayerNumber];
    TU8 mInitialVideoDisplayLayerWindowNumber[DMaxDisplayLayerNumber];

    SVideoPostPConfiguration mInitialVideoPostPConfig;

    SVideoPostPGlobalSetting mInitalVideoPostPGlobalSetting;
    SVideoPostPDisplayLayout mInitalVideoPostPDisplayLayout;
    TUint mbCertainSDWindowSwitchedtoHD;

private:
    //private data related
    SPrivateDataConfig *mpPrivateDataConfigList;

private:
    //streaming related
    IStreamingServerManager *mpStreamingServerManager;
    TComponentIndex mnCurrentStreamingContentNumber;
    TComponentIndex mnMaxStreamingContentNumber;
    SStreamingSessionContent *mpStreamingContent;
    CIDoubleLinkedList mStreamingServerList;

private:
    //cloud related
    ICloudServerManager *mpCloudServerManager;
    CIDoubleLinkedList mCloudServerList;
    TComponentIndex mnCurrentCloudReceiverContentNumber;
    TComponentIndex mnMaxCloudReceiverContentNumber;
    SCloudContent *mpCloudReceiverContent;

private:
    TUint mnPlaybackPipelinesNumber;
    SPlaybackPipelines *mpPlaybackPipelines;
    TU32 mCurSourceGroupIndex;
    TU8 mCurHDWindowIndex;
    TU8 reserved0, reserved1, reserved2;

    SRecordingPipelines mRecordingPipelines;
    SStreamingPipelines mRTSPStreamingPipelines;

private:
    TU8 mbSuspendAudioPlaybackPipelineInFwBw;
    TU8 mbSuspendBackgoundVideoPlaybackPipeline;

    TU8 mbChangePBSpeedWithFlush;
    TU8 mbChangePBSpeedWithSeek;

private:
    TU8 mbDisplayAllInstance;
    TU8 mbMultipleWindowView;
    TU8 mbSingleWindowView;
    TU8 mSingleViewDecIndex;

private:
    TComponentIndex mnUserTimerNumber;
    TComponentIndex mnUserTimerMaxNumber;//debug

    CIDoubleLinkedList mUserTimerList;
    CIDoubleLinkedList mUserTimerFreeList;

    //mudec related:
private:
    SVideoPostPConfiguration mCurrentVideoPostPConfig;
    SVideoPostPGlobalSetting mCurrentVideoPostPGlobalSetting;
    SVideoPostPDisplayLayout mCurrentDisplayLayout;

private:
    SSyncStatus mCurrentSyncStatus;
    SSyncStatus mInputSyncStatus;

    TU8 *mpCommunicationBuffer;
    TMemSize mCommunicationBufferSize;

private:
    TTime mDailyCheckTime;
    TULong mDailyCheckContext;
#ifdef BUILD_CONFIG_WITH_LICENCE
    ILicenceClient *mpLicenceClient;
#endif
    SClockListener *mpDailyCheckTimer;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

