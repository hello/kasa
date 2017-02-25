/*******************************************************************************
 * simple_api.h
 *
 * History:
 *    2013/12/30 - [Zhi He] create file
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

#ifndef __SIMPLE_API_H__
#define __SIMPLE_API_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

#define DMaxGroupNumber 4
#define DMaxChannelNumberInGroup 16

typedef struct {
    TU8 enable_playback;
    TU8 enable_recording;
    TU8 enable_streaming;
    TU8 enable_cloud_agent;

    TU8 share_demuxer;
    TU8 enable_streaming_relay;
    TU8 reserved1;
    TU8 reserved2;

    //source url
    TUint source_channel_number;
    TUint source_2rd_channel_number;
    TChar source_url[DMaxChannelNumberInGroup][DMaxUrlLen];
    TChar source_url_2rd[DMaxChannelNumberInGroup][DMaxUrlLen];

    //sink url
    TUint sink_channel_number;
    TUint sink_2rd_channel_number;
    TChar sink_url[DMaxChannelNumberInGroup][DMaxUrlLen];
    TChar sink_url_2rd[DMaxChannelNumberInGroup][DMaxUrlLen];

    //streaming url
    TUint streaming_channel_number;
    TUint streaming_2rd_channel_number;
    TChar streaming_url[DMaxChannelNumberInGroup][DMaxUrlLen];
    TChar streaming_url_2rd[DMaxChannelNumberInGroup][DMaxUrlLen];

    //cloud
    TUint cloud_channel_number;
    TChar cloud_agent_tag[DMaxChannelNumberInGroup][DMaxUrlLen];
} SGroupUrl;

typedef struct {
    //component
    TGenericID demuxer_id[DMaxChannelNumberInGroup];
    TGenericID demuxer_2rd_id[DMaxChannelNumberInGroup];

    TGenericID muxer_id[DMaxChannelNumberInGroup];
    TGenericID muxer_2rd_id[DMaxChannelNumberInGroup];

    TGenericID cloud_agent_id[DMaxChannelNumberInGroup];

    //pipeline
    TGenericID playback_pipeline_id[DMaxChannelNumberInGroup];
    TGenericID playback_2rd_pipeline_id[DMaxChannelNumberInGroup];

    TGenericID recording_pipeline_id[DMaxChannelNumberInGroup];
    TGenericID recording_2rd_pipeline_id[DMaxChannelNumberInGroup];

    TGenericID streaming_pipeline_id[DMaxChannelNumberInGroup];
    TGenericID streaming_2rd_pipeline_id[DMaxChannelNumberInGroup];

    TGenericID cloud_pipeline_id[DMaxChannelNumberInGroup];

    TGenericID native_push_pipeline_id[DMaxChannelNumberInGroup];

    TU8 playback_pipeline_suspended[DMaxChannelNumberInGroup];
    TU8 playback_pipeline_2rd_suspended[DMaxChannelNumberInGroup];
    TU8 recording_pipeline_suspended[DMaxChannelNumberInGroup];
    TU8 recording_pipeline_2rd_suspended[DMaxChannelNumberInGroup];
    TU8 streaming_pipeline_suspended[DMaxChannelNumberInGroup];
    TU8 streaming_pipeline_2rd_suspended[DMaxChannelNumberInGroup];

    TU8 cloud_pipeline_suspended[DMaxChannelNumberInGroup];
    TU8 cloud_pipeline_connected[DMaxChannelNumberInGroup];

    TU8 native_push_pipeline_suspended[DMaxChannelNumberInGroup];
    TU8 native_push_pipeline_connected[DMaxChannelNumberInGroup];
} SGroupGragh;

typedef struct {
    TU32 voutmask;
    EMediaWorkingMode work_mode;
    EPlaybackMode playback_mode;

    TU8 enable_playback;
    TU8 enable_recording;
    TU8 enable_streaming_server;
    TU8 enable_cloud_server;

    TU8 enable_audio_capture;
    TU8 push_audio_type; //0: raw, 1 compressed
    TU8 enable_remote_audio_forwarding;
    TU8 reserved2;

    TU16 rtsp_server_port;
    TU16 cloud_server_port;

    TU8 disable_video_path;
    TU8 disable_audio_path;
    TU8 enable_force_log_level;
    TU8 force_log_level;

    TU8 enable_audio_pinmuxer;
    TU8 debug_not_build_full_gragh;
    TU8 debug_share_demuxer;
    TU8 debug_try_rtsp_tcp_mode_first;

    TU8 playback_group_number;
    TU8 recording_group_number;
    TU8 streaming_group_number;
    TU8 cloud_group_number;

    TU8 debug_prefer_ffmpeg_muxer;
    TU8 debug_prefer_private_ts_muxer;
    TU8 debug_prefer_ffmpeg_audio_decoder;
    TU8 debug_prefer_aac_audio_decoder;

    TU8 debug_prefer_aac_audio_encoder;
    TU8 debug_prefer_customized_adpcm_encoder;
    TU8 debug_prefer_customized_adpcm_decoder;
    TU8 debug_prefer_reserved1;

    TU8 enable_trasncoding;
    TU8 disable_dsp_related;
    TU8 parse_multiple_access_unit;
    TU8 reserved1;

    TU8 enable_cloud_upload;
    TU8 reserved3;
    TU16 cloud_upload_server_port;

    TU8 udec_instance_number;
    TU8 udec_2rd_instance_number;
    TU8 udec_not_feed_usequpes;
    TU8 udec_not_feed_pts;

    TU8 compensate_delay_from_jitter;
    TU8 rtsp_client_try_tcp_mode_first;
    TU8 decoder_prefetch_count;
    TU8 udec_specify_fps;

    TU8 net_receiver_scheduler_number;
    TU8 net_sender_scheduler_number;
    TU8 fileio_reader_scheduler_number;
    TU8 fileio_writer_scheduler_number;

    TU8 video_decoder_scheduler_number;
    TU8 dsp_decoder_total_buffer_count;
    TU8 streaming_timeout_threashold;//default value 10
    TU8 streaming_retry_interval;//default value 10

    TU8 udec_specify_transcode_bitrate;
    TU8 udec_specify_transcode_framerate;
    TU8 dsp_tilemode;
    TU8 b_smooth_stream_switch;

    TU8 bg_color_y;
    TU8 bg_color_cb;
    TU8 bg_color_cr;
    TU8 set_bg_color;

    TU32 transcode_bitrate;
    TU32 transcode_framerate;

    //debug option
    TU8 debug_prefer_udec_stop_flag;
    TU8 debug_reserved0;
    TU8 debug_reserved1;
    TU8 debug_reserved2;

    TChar cloud_upload_server_addr[DMaxUrlLen];
    TChar cloud_upload_tag[DMaxUrlLen];
} SSimpleAPISetting;

typedef struct {
    //input setting
    SSimpleAPISetting setting;

    //input url
    TU8 group_number;
    TU8 current_group_index;
    TU8 channel_number_per_group;
    TU8 have_dual_stream;

    TU8 is_vod_enabled;
    TU8 is_vod_ready;
    TU8 reserved0;
    TU8 reserved1;

    //single channel audio, relative index in group
    TComponentIndex single_channel_audio_camera2nvr;
    TComponentIndex single_channel_audio_camera2app;
    TComponentIndex single_channel_audio_app2camera;
    TComponentIndex single_channel_audio_app2nvr;

    SGroupUrl group[DMaxGroupNumber];

    //data processing gragh:
    //global
    TGenericID streaming_server_id;
    TGenericID cloud_server_id;

    TGenericID streaming_transmiter_id;

    TGenericID audio_pin_muxer_id;
    TGenericID audio_decoder_id;
    TGenericID audio_renderer_id;

    TGenericID audio_capture_id;
    TGenericID audio_encoder_id;

    TGenericID video_renderer_id;
    TGenericID video_decoder_id[DMaxChannelNumberInGroup];
    TGenericID video_decoder_2rd_id[DMaxChannelNumberInGroup];

    TGenericID video_encoder_id;

    TGenericID remote_control_agent_id;
    TGenericID remote_control_cloud_pipeline_id;

    TGenericID transcoding_streaming_pipeline_id;

    TGenericID transcoding_streaming_video_only_pipeline_id;
    TGenericID transcoding_streaming_audio_only_pipeline_id;

    TGenericID native_capture_pipeline;

    TGenericID cloud_client_agent_id;
    TGenericID cloud_upload_pipeline_id;

    //per group
    SGroupGragh gragh_component[DMaxGroupNumber];

    //status
    TU8 udec_instance_current_trickplay[DMaxChannelNumberInGroup * 2];

} SSimpleAPIContxt;

class CSimpleAPI
{
protected:
    CSimpleAPI(TUint direct_access);
    EECode Construct();
    virtual ~CSimpleAPI();

public:
    static CSimpleAPI *Create(TUint direct_access);
    void Destroy();

public:
    IGenericEngineControlV2 *QueryEngineControl() const;
    EECode AssignContext(SSimpleAPIContxt *p_context);

    EECode StartRunning();
    EECode ExitRunning();

    EECode Control(EUserParamType type, void *param);

public:
    static void MsgCallback(void *context, SMSG &msg);
    EECode ProcessMsg(SMSG &msg);

private:
    EECode checkSetting();
    EECode initialSetting();

    // data processing pipelines
private:
    EECode createGlobalComponents();
    EECode createGroupComponents(TUint index);
    EECode connectPlaybackComponents();
    EECode connectRecordingComponents();
    EECode connectStreamingComponents();
    EECode connectCloudComponents();
    EECode connectNativeAudioCaptureAndPushComponents();
    EECode connectCloudUploadComponents();
    EECode setupPlaybackPipelines();
    EECode setupRecordingPipelines();
    EECode setupStreamingPipelines();
    EECode setupCloudPipelines();
    EECode setupNativeCapturePushPipelines();
    EECode setupCloudUploadPipelines();
    EECode buildDataProcessingGragh();
    EECode preSetUrls();
    EECode setUrls();

    //pipelines
    void suspendPlaybackPipelines(TU8 start_index, TU8 end_index, TU8 group_index, TU8 is_second, TUint release_context);
    void resumePlaybackPipelines(TU8 start_index, TU8 end_index, TU8 group_index, TU8 is_second);

    // device related configuration
private:
    EECode initializeDevice();
    EECode setupInitialMediaConfig();
    EECode activateDevice();
    EECode deActivateDevice();
    EECode setupDSPConfig();
    EECode selectDSPMode(EMediaWorkingMode mode);
    void summarizeVoutParams();
    EECode presetPlaybackModeForDSP(TU16 num_of_instances_0, TU16 num_of_instances_1, TUint max_width_0, TUint max_height_0, TUint max_width_1, TUint max_height_1);
    EECode setupUdecDSPSettings();
    EECode setupMUdecDSPSettings();

    // native/remote APP's cmd processing
private:
    void initializeSyncStatus();
    EECode receiveSyncStatus(ICloudServerAgent *p_agent, TMemSize payload_size);
    EECode sendSyncStatus(ICloudServerAgent *p_agent);
    EECode updateSyncStatus();
    EECode sendCloudAgentVersion(ICloudServerAgent *p_agent, TU32 param0, TU32 param1);
    EECode processUdecStatusUpdateMsg(SMSG &msg);
    EECode processUdecResolutionUpdateMsg(SMSG &msg);

    EECode processUpdateMudecDisplayLayout(TU32 layout, TU32 focus_index);
    void resetZoomParameters();
    EECode processZoom(TU32 window_id, TU32 width, TU32 height, TU32 input_center_x, TU32 input_center_y);
    EECode processZoom(TU32 window_id, TU32 zoom_factor, TU32 offset_center_x, TU32 offset_center_y);
    EECode processMoveWindow(TU32 window_id, TU32 pos_x, TU32 pos_y, TU32 width, TU32 height);
    EECode processShowOtherWindows(TU32 window_index, TU32 show);
    EECode processDemandIDR(TU32 param1);
    EECode processUpdateBitrate(TU32 param1);
    EECode processUpdateFramerate(TU32 param1);
    EECode processSwapWindowContent(TU32 window_id_1, TU32 window_id_2);
    EECode processCircularShiftWindowContent(TU32 shift, TU32 is_ccw);
    EECode processSwitchGroup(TU32 group, TU32 param_0);

    EECode processUpdateMudecDisplayLayout_no_dual_stream(TU32 layout, TU32 focus_index);
    EECode processSwapWindowContent_no_dual_stream(TU32 window_id_1, TU32 window_id_2);
    EECode processSwitchGroup_no_dual_stream(TU32 group, TU32 param_0);

private:
    EECode processPlaybackCapture(SUserParamType_PlaybackCapture *capture);
    EECode processPlaybackTrickplay(SUserParamType_PlaybackTrickplay *input_trickplay);

private:
    IGenericEngineControlV2 *mpEngineControl;
    SSimpleAPIContxt *mpContext;

private:
    volatile SPersistMediaConfig *mpMediaConfig;

private:
    TU8 mbStartRunning;
    TU8 mbBuildGraghDone;
    TU8 mbAssignContext;
    TU8 mbDirectAccess;

private:
    EDSPOperationMode mRequestDSPMode;

    //mudec related user cmd
private:
    SVideoPostPConfiguration mCurrentVideoPostPConfig;
    SVideoPostPGlobalSetting mCurrentVideoPostPGlobalSetting;
    SVideoPostPDisplayLayout mCurrentDisplayLayout;

private:
    SSyncStatus mCurrentSyncStatus;
    SSyncStatus mInputSyncStatus;

    TU8 *mpCommunicationBuffer;
    TMemSize mCommunicationBufferSize;

    //inter-median state
private:
    TU8 mSwitchLayeroutIdentifyerCount;
    TU8 mbSmoothSwitchLayout;
    TU8 mbSdInHdWindow;
    TU8 mbHdInSdWindow;

    TU8 mFocusIndex;
    TU8 mReserved0;
    TU8 mReserved1;
    TU8 mReserved2;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif


