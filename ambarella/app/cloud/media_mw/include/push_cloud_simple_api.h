/*******************************************************************************
 * push_cloud_simple_api.h
 *
 * History:
 *    2014/03/13 - [Zhi He] create file
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

#ifndef __PUSH_CLOUD_SIMPLE_API_H__
#define __PUSH_CLOUD_SIMPLE_API_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

#define DMinChannelNumberPerStreamingTransmiterInPushMode 1

#define DMaxStreamingTransmiterNumberInPushMode (DSYSTEM_MAX_CHANNEL_NUM/DMinChannelNumberPerStreamingTransmiterInPushMode)

typedef struct {
    TU8 enable_playback;
    TU8 enable_recording;
    TU8 enable_streaming;
    TU8 enable_cloud_agent;

    TU32 cloud_tag_number;
    TU32 sink_url_number;
    TU32 streaming_url_number;
    TU32 remote_control_tag_number;

    //cloud url, source
    TChar cloud_agent_tag[DSYSTEM_MAX_CHANNEL_NUM][DMaxUrlLen];

    //sink url
    TChar sink_url[DSYSTEM_MAX_CHANNEL_NUM][DMaxUrlLen];

    //streaming url
    TChar streaming_url[DSYSTEM_MAX_CHANNEL_NUM][DMaxUrlLen];

    //cloud url, remote control
    //TChar remote_control_agent_tag[DSYSTEM_MAX_CHANNEL_NUM][DMaxUrlLen];
} SGroupUrlInPushMode;

typedef struct {
    //component
    TGenericID cloud_agent_id[DSYSTEM_MAX_CHANNEL_NUM];
    TGenericID cloud_agent_cmd_id[DSYSTEM_MAX_CHANNEL_NUM];
    TGenericID muxer_id[DSYSTEM_MAX_CHANNEL_NUM];

    TGenericID streaming_transmiter_id[DMaxStreamingTransmiterNumberInPushMode];

    TGenericID remote_control_agent_id[DMaxStreamingTransmiterNumberInPushMode];

    //pipeline
    TGenericID recording_pipeline_id[DSYSTEM_MAX_CHANNEL_NUM];
    TGenericID streaming_pipeline_id[DSYSTEM_MAX_CHANNEL_NUM];
    TGenericID cloud_pipeline_id[DSYSTEM_MAX_CHANNEL_NUM];
    TGenericID remote_control_pipeline_id[DSYSTEM_MAX_CHANNEL_NUM];
} SGroupGraghInPushMode;

typedef struct {
    TU32 max_push_channel_number;
    TU32 max_streaming_transmiter_number;
    TU32 channel_number_per_transmiter;
    TU32 max_remote_control_channel_number;

    TU32 voutmask;
    EMediaWorkingMode work_mode;
    EPlaybackMode playback_mode;

    TU8 enable_playback;
    TU8 enable_recording;
    TU8 enable_streaming_server;
    TU8 enable_cloud_server;

    TU16 rtsp_server_port;
    TU16 cloud_server_port;

    TU8 disable_video_path;
    TU8 disable_audio_path;
    TU8 enable_force_log_level;
    TU8 force_log_level;

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
    TU8 net_receiver_tcp_scheduler_number;

    TU8 compensate_delay_from_jitter;
    TU8 rtsp_client_try_tcp_mode_first;

    TU8 net_receiver_scheduler_number;
    TU8 net_sender_scheduler_number;
    TU8 fileio_reader_scheduler_number;
    TU8 fileio_writer_scheduler_number;

    TU8 streaming_timeout_threashold;//default value 10
    TU8 streaming_retry_interval;//default value 10

    //pulse sender related
    TU32 pulse_sender_memsize;
    TU16 pulse_sender_framecount;
    TU8 pulse_sender_number;
    TU8 enable_push_path;
} SSimpleAPISettingInPushMode;

typedef struct {
    //input setting
    SSimpleAPISettingInPushMode setting;

    SGroupUrlInPushMode group_url;

    //data processing gragh:
    TGenericID streaming_server_id;
    TGenericID cloud_server_id;

    TGenericID remote_control_agent_id;
    TGenericID remote_control_cloud_pipeline_id;

    SGroupGraghInPushMode gragh_component;

    TU8 daemon;
    TU8 reserved0;
    TSocketPort gd_port;

    TSocketHandler gd;
} SSimpleAPIContxtInPushMode;

class CSimpleAPIInPushMode
{
protected:
    CSimpleAPIInPushMode(TUint direct_access);
    EECode Construct();
    virtual ~CSimpleAPIInPushMode();

public:
    static CSimpleAPIInPushMode *Create(TUint direct_access);
    void Destroy();

public:
    IGenericEngineControlV2 *QueryEngineControl() const;
    EECode AssignContext(SSimpleAPIContxtInPushMode *p_context);

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
    EECode createGroupComponents();
    EECode connectRecordingComponents();
    EECode connectStreamingComponents();
    EECode connectCloudComponents();
    EECode setupRecordingPipelines();
    EECode setupStreamingPipelines();
    EECode setupCloudPipelines();
    EECode buildDataProcessingGragh();
    EECode preSetUrls();

private:
    IGenericEngineControlV2 *mpEngineControl;
    SSimpleAPIContxtInPushMode *mpContext;

private:
    volatile SPersistMediaConfig *mpMediaConfig;

private:
    TU8 mbStartRunning;
    TU8 mbBuildGraghDone;
    TU8 mbAssignContext;
    TU8 mbDirectAccess;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif


