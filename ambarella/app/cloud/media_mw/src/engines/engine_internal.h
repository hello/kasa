/*******************************************************************************
 * engine_internal.h
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

#ifndef __ENGINE_INTERNAL_H__
#define __ENGINE_INTERNAL_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

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
    TGenericID apinmuxer_output_connection_id;
    TGenericID video_postp_id;
    TGenericID audio_postp_id;
    TGenericID video_renderer_id;
    TGenericID audio_renderer_id;

    TGenericID audio_pinmuxer_id;

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
    SConnection *p_apinmuxer_output_connection;
    SComponent *p_video_postp;
    SComponent *p_audio_postp;
    SComponent *p_video_renderer;
    SComponent *p_audio_renderer;
    SComponent *p_audio_pinmuxer;
} SPlaybackPipeline;

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

    TGenericID sink_id;

    //for muxer
    TChar *channel_name;

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
    TU8 video_sink_number;
    TU8 audio_sink_number;

    TGenericID server_id;//debug use
    TGenericID agent_id;//debug use

    TGenericID video_sink_id[EMaxSinkNumber_CloudPipeline];
    TGenericID video_sink_connection_id[EMaxSinkNumber_CloudPipeline];
    TGenericID audio_sink_id[EMaxSinkNumber_CloudPipeline];
    TGenericID audio_sink_connection_id[EMaxSinkNumber_CloudPipeline];

    //for quicker access
    SComponent *p_server;
    SComponent *p_agent;
    SComponent *p_video_sink[EMaxSinkNumber_CloudPipeline];
    SConnection *p_video_sink_connection[EMaxSinkNumber_CloudPipeline];
    SComponent *p_audio_sink[EMaxSinkNumber_CloudPipeline];
    SConnection *p_audio_sink_connection[EMaxSinkNumber_CloudPipeline];

    TChar *url;
    TMemSize url_buffer_size;

    TU32 dynamic_code;
} SCloudPipeline;

typedef struct {
    TGenericID pipeline_id;//read only

    TComponentIndex index;//read only
    TComponentType type;//read only
    TU8 pipeline_state;

    TU8 config_running_at_startup;
    TU8 video_enabled;
    TU8 audio_enabled;
    TU8 reserved0;

    TGenericID video_capture_id;
    TGenericID audio_capture_id;
    TGenericID video_encoder_id;
    TGenericID audio_encoder_id;

    //for quicker access
    SComponent *p_video_capture;
    SComponent *p_audio_capture;
    SComponent *p_video_encoder;
    SComponent *p_audio_encoder;
} SNativeCapturePipeline;

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
    TU8 video_sink_number;
    TU8 audio_sink_number;

    TGenericID push_source_id;

    TGenericID video_sink_id;
    TGenericID video_sink_connection_id;
    TGenericID audio_sink_id;
    TGenericID audio_sink_connection_id;

    //for quicker access
    SComponent *p_push_source;

    SComponent *p_video_sink;
    SConnection *p_video_sink_connection;
    SComponent *p_audio_sink;
    SConnection *p_audio_sink_connection;
} SNativePushPipeline;

typedef struct {
    TGenericID pipeline_id;//read only

    TComponentIndex index;//read only
    TComponentType type;//read only
    TU8 pipeline_state;

    TU8 config_running_at_startup;
    TU8 video_enabled;
    TU8 audio_enabled;
    TU8 reserved2;

    TGenericID video_source_id;
    TGenericID audio_source_id;

    TGenericID video_source_connection_id;
    TGenericID audio_source_connection_id;

    TGenericID uploader_id;

    //for quicker access
    SComponent *p_video_source;
    SComponent *p_audio_source;

    SConnection *p_video_source_connection;
    SConnection *p_audio_source_connection;

    SComponent *p_uploader;
} SCloudUploadPipeline;

TUint isComponentIDValid(TGenericID id, TComponentType check_type);
TUint isComponentIDValid_2(TGenericID id, TComponentType check_type, TComponentType check_type_2);
TUint isComponentIDValid_3(TGenericID id, TComponentType check_type, TComponentType check_type_2, TComponentType check_type_3);
TUint isComponentIDValid_4(TGenericID id, TComponentType check_type, TComponentType check_type_2, TComponentType check_type_3, TComponentType check_type_4);
TUint isComponentIDValid_5(TGenericID id, TComponentType check_type, TComponentType check_type_2, TComponentType check_type_3, TComponentType check_type_4, TComponentType check_type_5);
TUint isComponentIDValid_6(TGenericID id, TComponentType check_type, TComponentType check_type_2, TComponentType check_type_3, TComponentType check_type_4, TComponentType check_type_5, TComponentType check_type_6);
EFilterType __filterTypeFromGenericComponentType(TU8 generic_component_type, TU8 scheduled_video_decoder, TU8 scheduled_muxer);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif
