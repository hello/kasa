/*******************************************************************************
 * media_simple_api.h
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

#ifndef __MEDIA_SIMPLE_API_H__
#define __MEDIA_SIMPLE_API_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

#define DMAX_PB_INSTANCE_NUMBER 64
#define DMAX_CAP_INSTANCE_NUMBER 4

#define DMAX_URL_LENGTH 512

typedef struct {
    TDimension offset_x;
    TDimension offset_y;
    TDimension width;
    TDimension height;
    TU32 framerate_num;
    TU32 framerate_den;
} SMediaSimpleAPISettingVideoCapture;

typedef struct {
    TDimension width;
    TDimension height;
    TU32 framerate_num;
    TU32 framerate_den;
    TU32 bitrate;
} SMediaSimpleAPISettingVideoEncoding;

typedef struct {
    TU32 bitrate;
} SMediaSimpleAPISettingAudioEncoding;

typedef struct {
    TU8 enable_video;
    TU8 enable_audio;
    TU8 enable_playback;
    TU8 enable_playback_local_storage;

    TU8 enable_capture;
    TU8 enable_capture_streaming_out;
    TU8 enable_capture_local_storage;
    TU8 enable_capture_cloud_uploading;

    TU8 enable_force_log_level;
    TU8 force_log_level;
    TU8 pb_use_external_source;
    TU8 b_constrain_latency;

    TU8 b_use_official_test_model_decoder;
    TU8 b_pb_video_source_use_external_data_processing;
    TU8 b_render_video_nodelay;
    TU8 reserved2;

    TU16 total_num_playback_instance;
    TU16 total_num_capture_instance;

    TSocketPort local_rtsp_port;

    TSocketPort im_server_port;
    TSocketPort data_server_port;
    TSocketPort data_server_rtsp_port;

    TChar cloud_server_url[DMaxUrlLen];

    //detailed setting
    TChar prefer_string_muxer[DMaxPreferStringLen];
    TChar prefer_string_demuxer[DMaxPreferStringLen];
    TChar prefer_string_ext_video_source[DMaxPreferStringLen];
    TChar prefer_string_ext_audio_source[DMaxPreferStringLen];
    TChar prefer_string_video_decoder[DMaxPreferStringLen];
    TChar prefer_string_audio_decoder[DMaxPreferStringLen];
    TChar prefer_string_video_encoder[DMaxPreferStringLen];
    TChar prefer_string_audio_encoder[DMaxPreferStringLen];
    TChar prefer_string_video_output[DMaxPreferStringLen];
    TChar prefer_string_audio_output[DMaxPreferStringLen];
    TChar prefer_string_video_capture[DMaxPreferStringLen];
    TChar prefer_string_audio_capture[DMaxPreferStringLen];

    TU8 parse_multiple_access_unit;
    TU8 compensate_delay_from_jitter;
    TU8 rtsp_client_try_tcp_mode_first;
    TU8 rtsp_enable_rtcp;

    TU8 decoder_prefetch_count;
    TU8 streaming_timeout_threashold;
    TU8 streaming_retry_interval;
    TU8 video_decoder_scheduler_number;

    TU8 net_receiver_scheduler_number;
    TU8 net_sender_scheduler_number;
    TU8 fileio_reader_scheduler_number;
    TU8 fileio_writer_scheduler_number;

    TU8 record_strategy;
    TU8 record_autocut_condition;
    TU8 record_autocut_naming;
    TU8 pb_ext_video_source_format;
    TU32 record_autocut_framecount;
    TU32 record_autocut_duration;

    TU8 pb_precached_video_frames;
    TU8 pb_max_video_frames;
    TU8 pb_try_requested_framerate;
    TU8 pb_ext_audio_source_format;

    TU32 requested_framerate_num;
    TU32 requested_framerate_den;

    //source url
    TChar pb_source_url[DMAX_PB_INSTANCE_NUMBER][DMAX_URL_LENGTH];
    //sink url
    TChar pb_sink_url[DMAX_PB_INSTANCE_NUMBER][DMAX_URL_LENGTH];

    //streaming url
    TChar cap_local_streaming_url[DMAX_CAP_INSTANCE_NUMBER][DMAX_URL_LENGTH];
    //sink url
    TChar cap_sink_url[DMAX_CAP_INSTANCE_NUMBER][DMAX_URL_LENGTH];
    //cloud url
    TChar cap_cloud_uploading_account[DMAX_CAP_INSTANCE_NUMBER][DMAX_URL_LENGTH];
    TChar cap_cloud_uploading_password[DMAX_CAP_INSTANCE_NUMBER][DMAX_URL_LENGTH];
} SMediaSimpleAPISetting;

typedef struct {
    SMediaSimpleAPISetting setting;
    SMediaSimpleAPISettingVideoCapture video_cap_setting;
    SMediaSimpleAPISettingVideoEncoding video_enc_setting;
    SMediaSimpleAPISettingAudioEncoding audio_enc_setting;
    SPlaybackDecodeSetting pb_decode_setting;

    SPersistCommonConfig common_config;

    //streaming related
    TGenericID streaming_server_id;
    TGenericID streaming_transmiter_id;

    //playback
    TGenericID pb_pipelines_id[DMAX_PB_INSTANCE_NUMBER];
    TGenericID pb_recording_pipelines_id[DMAX_PB_INSTANCE_NUMBER];

    TGenericID ext_video_source_id[DMAX_PB_INSTANCE_NUMBER];
    TGenericID ext_audio_source_id[DMAX_PB_INSTANCE_NUMBER];

    TGenericID demuxer_id[DMAX_PB_INSTANCE_NUMBER];
    TGenericID video_decoder_id[DMAX_PB_INSTANCE_NUMBER];
    TGenericID audio_decoder_id[DMAX_PB_INSTANCE_NUMBER];
    TGenericID pb_muxer_id[DMAX_PB_INSTANCE_NUMBER];
    TGenericID video_output_id[DMAX_PB_INSTANCE_NUMBER];
    TGenericID audio_output_id[DMAX_PB_INSTANCE_NUMBER];

    //capture
    TGenericID cap_pipelines_id[DMAX_CAP_INSTANCE_NUMBER];
    TGenericID cap_streaming_pipelines_id[DMAX_CAP_INSTANCE_NUMBER];
    TGenericID cap_recording_pipelines_id[DMAX_CAP_INSTANCE_NUMBER];
    TGenericID cap_uploading_pipelines_id[DMAX_CAP_INSTANCE_NUMBER];

    TGenericID video_capturer_id[DMAX_CAP_INSTANCE_NUMBER];
    TGenericID video_encoder_id[DMAX_CAP_INSTANCE_NUMBER];
    TGenericID audio_capturer_id[DMAX_CAP_INSTANCE_NUMBER];
    TGenericID audio_encoder_id[DMAX_CAP_INSTANCE_NUMBER];
    TGenericID cap_muxer_id[DMAX_CAP_INSTANCE_NUMBER];
    TGenericID cap_cloud_uploader_id[DMAX_CAP_INSTANCE_NUMBER];

    //external context
    void *p_pb_textures[DMAX_PB_INSTANCE_NUMBER];
    void *p_pb_sound_tracks[DMAX_PB_INSTANCE_NUMBER];

    //external devices
    void *p_sound_input_devices[DMAX_CAP_INSTANCE_NUMBER];

    //external source
    void *p_ext_video_source_handler[DMAX_PB_INSTANCE_NUMBER];
    void *p_ext_audio_source_handler[DMAX_PB_INSTANCE_NUMBER];

    //external data processing for playback, video source
    void *p_pb_videosource_ext_post_processing_callback[DMAX_PB_INSTANCE_NUMBER];
    void *p_pb_videosource_ext_post_processing_callback_context[DMAX_PB_INSTANCE_NUMBER];

} SMediaSimpleAPIContext;

class CMediaSimpleAPI
{
protected:
    CMediaSimpleAPI(TU32 direct_access);
    EECode Construct(SSharedComponent *p_shared_component);
    virtual ~CMediaSimpleAPI();

public:
    static CMediaSimpleAPI *Create(TU32 direct_access, SSharedComponent *p_shared_component);
    void Destroy();

public:
    IGenericEngineControlV4 *QueryEngineControl() const;
    EECode AssignContext(SMediaSimpleAPIContext *p_context);

    EECode Run();
    EECode Exit();

    EECode Start();
    EECode Stop();

public:
    void PrintRuntimeStatus();

public:
    static void MsgCallback(void *context, SMSG &msg);

private:
    EECode checkSetting();
    EECode initialSetting();

    // data processing pipelines
private:
    EECode createGlobalComponents();
    EECode createPlaybackComponents();
    EECode createPlaybackExtSourceComponents();

    EECode createPlaybackLocalStorageComponents();
    EECode createCaptureComponents();
    EECode createCaptureLocalStorageComponents();
    EECode createCaptureLocalStreamingComponents();

    EECode connectPlaybackComponents();
    EECode connectPlaybackExtSourceComponents();
    EECode connectPlaybackLocalStorageComponents();
    EECode connectPlaybackExtSourceLocalStorageComponents();
    EECode connectCaptureComponents();
    EECode connectCaptureLocalStorageComponents();
    EECode connectCaptureLocalStreamingComponents();

    EECode setupPlaybackPipelines();
    EECode setupPlaybackExtSourcePipelines();
    EECode setupPlaybackLocalStoragePipelines();
    EECode setupPlaybackExtSourceLocalStoragePipelines();
    EECode setupCapturePipelines();
    EECode setupCaptureLocalStoragePipelines();
    EECode setupCaptureLocalStreamingPipelines();

    EECode buildDataProcessingGragh();
    EECode preSetUrls();
    EECode setUrls();
    EECode getHandlerForExternalSource();
    void setPBVideoSourceExternalDataProcessingCallback();

private:
    IGenericEngineControlV4 *mpEngineControl;
    SMediaSimpleAPIContext *mpContext;

private:
    volatile SPersistMediaConfig *mpMediaConfig;

private:
    TU8 mbBuildGraghDone;
    TU8 mbAssignContext;
    TU8 mbDirectAccess;
    TU8 mbStarted;

    TU8 mbRunning;
    TU8 mReserved0;
    TU8 mReserved1;
    TU8 mReserved2;
};

EECode gfMediaSimpleAPILoadDefaultCapServerConfig(SMediaSimpleAPIContext *p_content);
EECode gfMediaSimpleAPILoadDefaultPlaybackConfig(SMediaSimpleAPIContext *p_content);
EECode gfMediaSimpleAPILoadDefaultDebugConfig(SMediaSimpleAPIContext *p_content);
EECode gfMediaSimpleAPILoadMediaConfig(SMediaSimpleAPIContext *p_content, TChar *filename);
EECode gfMediaSimpleAPIPrintMediaConfig(SMediaSimpleAPIContext *p_content);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif


