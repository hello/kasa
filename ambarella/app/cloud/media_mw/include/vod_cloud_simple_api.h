/*******************************************************************************
 * vod_cloud_simple_api.h
 *
 * History:
 *    2014/06/11 - [Zhi He] create file
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


#ifndef __VOD_CLOUD_SIMPLE_API_H__
#define __VOD_CLOUD_SIMPLE_API_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

#define DMinChannelNumberPerStreamingTransmiterInVodMode 1

#define DMaxStreamingTransmiterNumberInVodMode (DSYSTEM_MAX_CHANNEL_NUM/DMinChannelNumberPerStreamingTransmiterInVodMode)

typedef struct {
    TU8 enable_playback;
    TU8 enable_recording;
    TU8 enable_streaming;
    TU8 enable_cloud_agent;

    TU8 enable_vod;
    TU8 reserved_1;
    TU8 reserved_2;
    TU8 reserved_3;

    TU32 remote_control_tag_number;
    TU32 channel_number;

    TChar channel_name[DSYSTEM_MAX_CHANNEL_NUM][DMaxUrlLen];
} SGroupUrlInVodMode;

typedef struct {
    //component
    TGenericID cloud_agent_id[DSYSTEM_MAX_CHANNEL_NUM];
    TGenericID cloud_agent_cmd_id[DSYSTEM_MAX_CHANNEL_NUM];
    TGenericID muxer_id[DSYSTEM_MAX_CHANNEL_NUM];

    TGenericID streaming_transmiter_id[DMaxStreamingTransmiterNumberInVodMode];

    TGenericID remote_control_agent_id[DMaxStreamingTransmiterNumberInVodMode];

    //vod
    TGenericID demuxer_id[DSYSTEM_MAX_CHANNEL_NUM];
    TGenericID flow_controller_id[DSYSTEM_MAX_CHANNEL_NUM];
    TGenericID vod_transmiter_id[DMaxStreamingTransmiterNumberInVodMode];

    //pipeline
    TGenericID recording_pipeline_id[DSYSTEM_MAX_CHANNEL_NUM];
    TGenericID streaming_pipeline_id[DSYSTEM_MAX_CHANNEL_NUM];
    TGenericID cloud_pipeline_id[DSYSTEM_MAX_CHANNEL_NUM];
    TGenericID remote_control_pipeline_id[DSYSTEM_MAX_CHANNEL_NUM];
    TGenericID vod_pipeline_id[DSYSTEM_MAX_CHANNEL_NUM];
} SGroupGraghInVodMode;

typedef struct {
    TU32 max_push_channel_number;
    TU32 max_streaming_transmiter_number;
    TU32 channel_number_per_transmiter;
    TU32 max_remote_control_channel_number;

    TU32 filter_version;

    TU32 voutmask;
    EMediaWorkingMode work_mode;
    EPlaybackMode playback_mode;

    TU8 enable_playback;
    TU8 enable_recording;
    TU8 enable_streaming_server;
    TU8 enable_cloud_server;

    TU8 enable_vod;
    TU8 enable_admin_server;
    TU16 admin_server_port;

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

    //file storage root dir
    TChar root_dir[DMaxUrlLen];
    TU32 max_storage_time;
    TU32 redudant_storage_time;
    TU32 single_file_duration_minutes;

    TU8 enable_rtsp_authentication;
    TU8 rtsp_authentication_mode;
    TU8 reserved0;
    TU8 reserved1;
} SSimpleAPISettingInVodMode;

typedef struct {
    //input setting
    SSimpleAPISettingInVodMode setting;

    SGroupUrlInVodMode group_url;

    //data processing gragh:
    TGenericID streaming_server_id;
    TGenericID cloud_server_id;

    TGenericID remote_control_agent_id;
    TGenericID remote_control_cloud_pipeline_id;

    SGroupGraghInVodMode gragh_component;

    TU8 daemon;
    TU8 reserved0;
    TSocketPort gd_port;

    TSocketHandler gd;
} SSimpleAPIContxtInVodMode;

class CSimpleAPIInVodMode
{
protected:
    CSimpleAPIInVodMode(TUint direct_access);
    EECode Construct();
    virtual ~CSimpleAPIInVodMode();

public:
    static CSimpleAPIInVodMode *Create(TUint direct_access);
    void Destroy();

public:
    IGenericEngineControlV3 *QueryEngineControl() const;
    EECode AssignContext(SSimpleAPIContxtInVodMode *p_context);

    EECode StartRunning();
    EECode ExitRunning();

    EECode Control(EUserParamType type, void *param);

public:
    static void MsgCallback(void *context, SMSG &msg);
    EECode ProcessMsg(SMSG &msg);

public:
    EECode AddPortSource(TChar *source, TChar *password, void *p_source, TChar *p_identification_string, void *p_context);
    EECode RemovePortSource(TU32 index);

    EECode ProcessReadBuffer(TU8 *p_data, TU32 datasize, TU32 subtype);

public:
    static EECode CommunicationClientReadCallBack(void *owner, TChar *source, void *p_source, void *p_port, TU8 *p_data, TU32 datasize, TU32 type, TU32 cat, TU32 subtype);

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
    EECode connectVodComponents();
    EECode setupRecordingPipelines();
    EECode setupStreamingPipelines();
    EECode setupCloudPipelines();
    EECode setupVodPipelines();
    EECode setupVodContent();
    EECode buildDataProcessingGragh();
    EECode preSetUrls();

private:
    EECode setupCommunicationServerPort();
    EECode updateUrls(TU32 index, TChar *p_url);
    EECode updateIdleChannelIndex();

    bool checkIsExist(TChar *target_url, TU32 &index);
    EECode removeChannel(TChar *p_url);
    EECode updateDeviceDynamicCode(TU32 index, TU32 dynamic_code);
    EECode updateUserDynamicCode(const TChar *p_user_name, const TChar *p_device_name, TU32 dynamic_code);

private:
    IGenericEngineControlV3 *mpEngineControl;
    IStorageManagement *mpStorageManager;
    TU32 mChannelTotalCount;
    SSimpleAPIContxtInVodMode *mpContext;

private:
    volatile SPersistMediaConfig *mpMediaConfig;

private:
    TU8 mbStartRunning;
    TU8 mbBuildGraghDone;
    TU8 mbAssignContext;
    TU8 mbDirectAccess;

private:
    CICommunicationServerPort *mpAdminServerPort;
    SCommunicationPortSource *mpPortSource[DMaxCommunicationPortSourceNumber];
    TU32 mnCurPortSourceNum;
    TU32 mnMaxPortSourceNum;

    TU32 mChannelIndex;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif
