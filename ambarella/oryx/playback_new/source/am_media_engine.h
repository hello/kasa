/**
 * am_media_engine.h
 *
 * History:
 *  2015/07/24 - [Zhi He] create file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
 */

#ifndef __AM_MEDIA_ENGINE_H__
#define __AM_MEDIA_ENGINE_H__

enum {
  EEngineState_not_alive = 0, //not build graph

  EEngineState_idle, //gragh build done, not running
  EEngineState_running, //running
  EEngineState_error,
};

enum {
  EComponentState_not_alive = 0,

  EComponentState_initialized = 0x01, // initialed, but not
  EComponentState_activated = 0x02, // thread is running
  EComponentState_started = 0x03, //stated

  EComponentState_eos_pending = 0x04,
  EComponentState_suspended = 0x05,
  EComponentState_error = 0x06,
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
  TU8 cur_state;

  //debug use
  TU8 playback_pipeline_number;
  TU8 recording_pipeline_number;
  TU8 streaming_pipeline_number;
  TU8 reserved1;

  TChar prefer_string[DMaxPreferStringLen];

  volatile TU32 status_flag;
  //volatile TU32 active_count;

  IFilter *p_filter;

  TChar *url;
  TMemSize url_buffer_size;
} SComponent;

typedef struct {
  TGenericID connection_id;

  TGenericID up_id;
  TGenericID down_id;

  TComponentIndex up_pin_index;
  TComponentIndex up_pin_sub_index;
  TComponentIndex down_pin_index;

  SComponent *up;
  SComponent *down;

  TU8 state;
  TU8 pin_type;//StreamType
  TU8 reserved0[2];
} SConnection;

typedef struct {
  TGenericID pipeline_id;//read only

  TComponentIndex index;//read only
  TComponentType type;//read only
  TU8 pipeline_state;

  TU8 video_enabled;
  TU8 audio_enabled;
  TU8 video_suspended;
  TU8 audio_suspended;

  TU8 video_runtime_disabled;
  TU8 audio_runtime_disabled;
  TU8 video_runtime_paused;
  TU8 audio_runtime_paused;

  //read only
  TGenericID video_source_id;
  TGenericID audio_source_id;
  TGenericID video_decoder_id;
  TGenericID audio_decoder_id;
  TGenericID vd_input_connection_id;
  TGenericID ad_input_connection_id;
  TGenericID vd_output_connection_id;
  TGenericID ad_output_connection_id;
  TGenericID as_output_connection_id;
  TGenericID video_renderer_id;
  TGenericID audio_renderer_id;

  //for quicker access
  SComponent *p_video_source;
  SComponent *p_audio_source;
  SComponent *p_video_decoder;
  SComponent *p_audio_decoder;
  SConnection *p_vd_input_connection;
  SConnection *p_ad_input_connection;
  SConnection *p_vd_output_connection;
  SConnection *p_ad_output_connection;
  SConnection *p_as_output_connection;
  SComponent *p_video_renderer;
  SComponent *p_audio_renderer;
} SPlaybackPipeline;

typedef struct {
  TGenericID pipeline_id;//read only

  TComponentIndex index;//read only
  TComponentType type;//read only
  TU8 pipeline_state;

  TU8 video_enabled;
  TU8 audio_enabled;
  TU8 video_source_number;
  TU8 audio_source_number;

  TGenericID sink_id;

  //for muxer
  //TChar* channel_name;

  TGenericID video_source_id[EMaxSourceNumber_RecordingPipeline];
  TGenericID video_source_connection_id[EMaxSourceNumber_RecordingPipeline];
  TGenericID audio_source_id[EMaxSourceNumber_RecordingPipeline];
  TGenericID audio_source_connection_id[EMaxSourceNumber_RecordingPipeline];

  //for quicker access
  SComponent *p_sink;
  SComponent *p_video_source[EMaxSourceNumber_RecordingPipeline];
  SConnection *p_video_source_connection[EMaxSourceNumber_RecordingPipeline];
  SComponent *p_audio_source[EMaxSourceNumber_RecordingPipeline];
  SConnection *p_audio_source_connection[EMaxSourceNumber_RecordingPipeline];
} SRecordingPipeline;

typedef struct {
  TGenericID pipeline_id;//read only

  TComponentIndex index;//read only
  TComponentType type;//read only
  TU8 pipeline_state;

  TU8 config_running_at_startup;
  TU8 reserved1;
  TU8 reserved2;
  TU8 reserved3;

  TU8 video_enabled;
  TU8 audio_enabled;
  TU8 video_source_number;
  TU8 audio_source_number;

  TGenericID server_id;//debug use
  TGenericID data_transmiter_id;//debug use

  TGenericID video_source_id[EMaxSourceNumber_StreamingPipeline];
  TGenericID video_source_connection_id[EMaxSourceNumber_StreamingPipeline];
  TGenericID audio_source_id[EMaxSourceNumber_StreamingPipeline];
  TGenericID audio_source_connection_id[EMaxSourceNumber_StreamingPipeline];
  TGenericID audio_pinmuxer_id;
  TGenericID audio_pinmuxer_connection_id[EMaxSourceNumber_StreamingPipeline];

  //for quicker access
  SComponent *p_server;
  SComponent *p_data_transmiter;
  SComponent *p_video_source[EMaxSourceNumber_StreamingPipeline];
  SConnection *p_video_source_connection[EMaxSourceNumber_StreamingPipeline];
  SComponent *p_audio_source[EMaxSourceNumber_StreamingPipeline];
  SConnection *p_audio_source_connection[EMaxSourceNumber_StreamingPipeline];
  SComponent *p_audio_pinmuxer;
  SConnection *p_audio_pinmuxer_connection[EMaxSourceNumber_StreamingPipeline];

  TChar *url;
  TMemSize url_buffer_size;
} SStreamingPipeline;

//-----------------------------------------------------------------------
//
// CGenericMediaEngine
//
//-----------------------------------------------------------------------

class CGenericMediaEngine
  : public CActiveEngine
  , public IGenericEngineControl
{
  typedef CActiveEngine inherited;

public:
  static CGenericMediaEngine *Create(const TChar *pname, TU32 index);

private:
  CGenericMediaEngine(const TChar *pname, TU32 index);
  EECode Construct();
  virtual ~CGenericMediaEngine();

public:
  virtual void Delete();
  virtual EECode PostEngineMsg(SMSG &msg);

public:
  virtual EECode GetMediaConfig(volatile SPersistMediaConfig *&pconfig) const;
  virtual EECode SetMediaConfig();

public:
  virtual EECode print_current_status(TGenericID id = 0, unsigned int print_flag = 0);

public:
  //build graph related
  virtual EECode begin_config_process_pipeline(TU32 customized_pipeline = 1);

  virtual EECode new_component(TU32 component_type, TGenericID &component_id, const TChar *prefer_string = NULL);
  virtual EECode connect_component(TGenericID &connection_id, TGenericID upper_component_id, TGenericID down_component_id, StreamType pin_type);

  virtual EECode setup_playback_pipeline(TGenericID &playback_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID video_decoder_id, TGenericID audio_decoder_id, TGenericID video_renderer_id, TGenericID audio_renderer_id);
  virtual EECode setup_recording_pipeline(TGenericID &recording_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID sink_id);
  virtual EECode setup_streaming_pipeline(TGenericID &streaming_pipeline_id, TGenericID streaming_transmiter_id, TGenericID streaming_server_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID video_connection_id = 0, TGenericID audio_connection_id = 0);

  virtual EECode finalize_config_process_pipeline();

public:
  virtual EECode generic_control(EGenericEngineConfigure config_type, void *param);

  //set urls
  virtual EECode set_source_url(TGenericID source_component_id, TChar *url);
  virtual EECode set_sink_url(TGenericID sink_component_id, TChar *url);
  virtual EECode set_streaming_url(TGenericID pipeline_id, TChar *url);

  //app msg
  virtual EECode set_app_msg_callback(void (*MsgProc)(void *, SMSG &), void *context);

  //flow related
  virtual EECode run_processing();
  virtual EECode exit_processing();

  virtual EECode start();
  virtual EECode stop();

public:
  virtual void destroy();

private:
  // IActiveObject
  void OnRun();

private:
  //component & connection
  SComponent *newComponent(TComponentType type, TComponentIndex index, const TChar *prefer_string);
  void deleteComponent(SComponent *p);

  SConnection *newConnetion(TGenericID up_id, TGenericID down_id, SComponent *up_component, SComponent *down_component);
  void deleteConnection(SConnection *p);

  void deactivateComponent(SComponent *p_component, TU32 release_flag = 1);
  void activateComponent(SComponent *p_component, TU8 update_focus_index = 0, TComponentIndex focus_index = 0);

  SComponent *findComponentFromID(TGenericID id);
  SComponent *findComponentFromTypeIndex(TU8 type, TU8 index);
  SComponent *findComponentFromFilter(IFilter *p_filter);

  SComponent *queryFilterComponent(TGenericID filter_id);
  SComponent *queryFilterComponent(TComponentType type, TComponentIndex index);
  SConnection *queryConnection(TGenericID connection_id);
  SConnection *queryConnection(TGenericID up_id, TGenericID down_id, StreamType type = StreamType_Invalid);

  SPlaybackPipeline *queryPlaybackPipeline(TGenericID id);
  SPlaybackPipeline *queryPlaybackPipelineFromARID(TGenericID id);
  SPlaybackPipeline *queryPlaybackPipelineFromVDID(TGenericID id);
  SPlaybackPipeline *queryPlaybackPipelineFromComponentID(TGenericID id);

private:
  //pipeline
  EECode setupPlaybackPipeline(TComponentIndex index);
  EECode setupRecordingPipeline(TComponentIndex index);
  EECode setupStreamingPipeline(TComponentIndex index);

private:
  //streaming service
  void setupStreamingServerManger();
  EECode addStreamingServer(TGenericID &server_id, StreamingServerType type, StreamingServerMode mode);
  EECode removeStreamingServer(TGenericID server_id);
  IStreamingServer *findStreamingServer(TGenericID id);

private:
  void exitServers();
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

  EECode runAllFilters(TGenericID pipeline_id);
  EECode exitAllFilters(TGenericID pipeline_id);

  EECode startAllFilters(TGenericID pipeline_id);
  EECode stopAllFilters(TGenericID pipeline_id);

  bool allFiltersFlag(TU32 flag, TGenericID id);
  void clearAllFiltersFlag(TU32 flag, TGenericID id);
  void setFiltersEOS(IFilter *pfilter);

private:
  //scheduler
  EECode createSchedulers(void);
  EECode destroySchedulers(void);

private:
  //flow
  EECode  doRunProcessing();
  EECode  doExitProcessing();
  EECode doStart();
  EECode doStop();

  void reconnectAllStreamingServer();
  void doPrintSchedulersStatus();
  void doPrintCurrentStatus(TGenericID id, TULong print_flag);
  void doPrintComponentStatus(TGenericID id);

private:
  void pausePlaybackPipeline(SPlaybackPipeline *p_pipeline);
  void resumePlaybackPipeline(SPlaybackPipeline *p_pipeline);
  void purgePlaybackPipeline(SPlaybackPipeline *p_pipeline);
  void triggerStopPlaybackPipeline(SPlaybackPipeline *p_pipeline);

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

  SPlaybackPipeline *mpPlaybackPipelines;
  SRecordingPipeline *mpRecordingPipelines;
  SStreamingPipeline *mpStreamingPipelines;

private:
  TU8 mbSetMediaConfig;
  TU8 mbSelfLogFile;
  TU8 mbSchedulersCreated;
  TU8 mRuningCount;

  TU8 mbExternalClock;
  TU8 mbReconnecting;
  TU8 mbLoopPlay;
  TU8 mbClockManagerStarted;

private:
  CIClockManager *mpClockManager;
  CIClockReference *mpSystemClockReference;
  IClockSource *mpClockSource;
  TU8 mbRun;
  TU8 mbGraghBuilt;
  TU8 mbProcessingRunning;
  TU8 mbProcessingStarted;
  IMutex *mpMutex;
  TU32 msState;

  TComponentIndex mnComponentFiltersNumbers[EGenericComponentType_TAG_filter_end - EGenericComponentType_TAG_filter_start];
  TComponentIndex mnComponentFiltersMaxNumbers[EGenericComponentType_TAG_filter_end - EGenericComponentType_TAG_filter_start];
  TComponentIndex mnComponentProxysNumbers[EGenericComponentType_TAG_proxy_end - EGenericComponentType_TAG_proxy_start];
  TComponentIndex mnComponentProxysMaxNumbers[EGenericComponentType_TAG_proxy_end - EGenericComponentType_TAG_proxy_start];

private:
  volatile SPersistMediaConfig mPersistMediaConfig;

private:
  //streaming related
  IStreamingServerManager *mpStreamingServerManager;
  SStreamingSessionContent *mpStreamingContent;
  CIDoubleLinkedList *mpStreamingServerList;

};

#endif

