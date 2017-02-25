/*******************************************************************************
 * mtest.cpp
 *
 * History:
 *    2013/03/20 - [Zhi He] create file
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

#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <signal.h>
//#include <sys/stat.h>
//#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>

#include <pthread.h>

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "media_mw_if.h"

DCONFIG_COMPILE_OPTION_CPPFILE_BEGIN
DCODE_DELIMITER;

//unit test self log
#if 1
//log related
enum {
    e_log_level_always = 0,

    e_log_level_error = 1,
    e_log_level_warning,
    e_log_level_notice,
    e_log_level_debug,

    e_log_level_all,

    e_log_level_total_count,
};
static TInt g_log_level = e_log_level_notice;
static TChar g_log_tags[e_log_level_total_count][32] = {
    {"      [Info]: "},
    {"  [Error]: "},
    {"    [Warnning]: "},
    {"      [Notice]: "},
    {"        [Debug]: "},
    {"          [Verbose]: "},
};

#define test_log_level_trace(level, format, args...)  do { \
        if (level <= g_log_level) { \
            fprintf(stdout, "%s", g_log_tags[level]); \
            fprintf(stdout, format,##args); \
            fprintf(stdout,"[log location]: file %s, function %s, line %d.\n", __FILE__, __FUNCTION__, __LINE__); \
        } \
    } while (0)

#define test_log_level(level, format, args...)  do { \
        if (level <= g_log_level) { \
            fprintf(stdout, "%s", g_log_tags[level]); \
            fprintf(stdout, format,##args); \
        } \
    } while (0)

#define test_logv(format, args...) test_log_level(e_log_level_all, format, ##args)
#define test_logd(format, args...) test_log_level(e_log_level_debug, format, ##args)
#define test_logn(format, args...) test_log_level_trace(e_log_level_notice, format, ##args)
#define test_logw(format, args...) test_log_level_trace(e_log_level_warning, format, ##args)
#define test_loge(format, args...) test_log_level_trace(e_log_level_error, format, ##args)

#define test_log(format, args...) test_log_level(e_log_level_always, format, ##args)
#else

#define test_logv LOG_VERBOSE
#define test_logd LOG_DEBUG
#define test_logn LOG_NOTICE
#define test_logw LOG_WARN
#define test_loge LOG_ERROR

#define test_log LOG_PRINTF
#endif

static TUint g_mtest_voutmask = DCAL_BITMASK(EDisplayDevice_HDMI);
static EMediaWorkingMode g_mtest_dspmode = EMediaDeviceWorkingMode_SingleInstancePlayback;
static EPlaybackMode g_mtest_playbackmode = EPlaybackMode_Invalid;

static TU8 g_mtest_config_rtsp_live_streaming = 0;
static TU8 g_mtest_config_rtsp_file_streaming = 0;
static TU8 g_mtest_config_save_file = 0;
static TU8 g_mtest_config_http_hls_streaming = 0;

static TU8 g_mtest_config_enable_audio = 1;
static TU8 g_mtest_config_enable_video = 1;
static TU8 g_b_mtest_force_log_level = 0;
static TU8 g_mtest_config_log_level = ELogLevel_Notice;

static TU8 g_mtest_config_multiple_audio_decoder = 0;

static TU8 g_mtest_dsp_dec_instances_number_1 = 4;
static TU8 g_mtest_dsp_dec_instances_number_2 = 1;
static TU8 g_mtest_dsp_dec_layer = 2;

static TU8 g_mtest_net_receiver_scheduler_number = 1;
static TU8 g_mtest_net_sender_scheduler_number = 0;
static TU8 g_mtest_fileio_reader_scheduler_number = 0;
static TU8 g_mtest_fileio_writer_scheduler_number = 0;

static TU8 g_mtest_video_decoder_scheduler_number = 0;
static TU8 g_mtest_save_all_channel = 0;
static TU8 g_mtest_share_demuxer = 0;
static TU8 g_mtest_decoder_prefetch_count = 6;

static TU8 g_mtest_dsp_total_buffer_count = 0; //default value 7
static TU8 g_mtest_dsp_prebuffer_count = 2;
static TU8 g_mtest_streaming_timeout_threashold = 0;//default value 10
static TU8 g_mtest_streaming_retry_interval = 0;//default value 10

static TU8 gb_mtest_request_decoder_prefetch_count = 0;
static TU8 gb_mtest_request_dsp_prebuffer_count = 0;
static TU8 gb_mtest_compensate_delay_from_jitter = 0;
static TU8 gb_mtest_is_single_view = 0;

static TU8 gb_mtest_udec_no_usequpes = 0;
static TU8 gb_mtest_udec_not_feed_pts = 0;
static TU8 gb_mtest_udec_specify_fps = 0;
static TU8 gb_mtest_rtsp_client_try_tcp_mode_first = 0;

static TUint g_mtest_rtsp_server_port = 0;

static TU8 gb_mtest_prefer_ffmpeg_muxer = 0;
static TU8 gb_mtest_prefer_private_ts_muxer = 0;
static TU8 gb_mtest_prefer_ffmpeg_audio_decoder = 0;
static TU8 gb_mtest_prefer_private_aac_decoder = 0;

#define d_max_source_url_group_number 4
#define d_max_sink_url_group_number 4
#define d_max_streaming_url_group_number 4

#define d_max_source_url_number 20
#define d_max_sink_url_number 20
#define d_max_streaming_url_number 20

enum {
    e_url_type_source = 0,
    e_url_type_sink,
    e_url_type_streaming,

    e_url_type_total_count,
};

typedef struct s_url_list_node {
    TChar *p_url;
    TULong url_buffer_length;
} SUrlListNode;

typedef struct  {
    //url
    SUrlListNode source_url_group[d_max_source_url_group_number][d_max_source_url_number];
    SUrlListNode sink_url_group[d_max_sink_url_group_number][d_max_sink_url_number];
    SUrlListNode streaming_url_group[d_max_streaming_url_group_number][d_max_streaming_url_number];
    TULong source_url_count_in_group[d_max_source_url_group_number];
    TULong source_sd_url_count_in_group[d_max_source_url_group_number];
    TULong source_hd_url_count_in_group[d_max_source_url_group_number];
    TULong sink_url_count_in_group[d_max_sink_url_group_number];
    TULong streaming_url_count_in_group[d_max_streaming_url_group_number];

    //configurations
    TU8 enable_rtsp_streaming;
    TU8 enable_mux_file;
    TU8 enable_playback;
    TU8 enable_transcoding;

    TU8 auto_naming_rtsp_tag;
    TU8 enable_audio;
    TU8 enable_video;
    TU8 only_hd_has_audio;

    TU8 enable_bidirect_audio;
    TU8 enable_audio_pin_mux;
    TU8 enable_cloud_server_nvr;
    TU8 enable_sacp_server;

    TU8 debug_share_demuxer;
    TU8 decoder_prefetch;
    TU8 dsp_total_buffer_number;
    TU8 dsp_prebuffer_count;

    TU8 streaming_timeout_threashold;
    TU8 streaming_autoretry_connect_server_interval;
    TU8 b_set_decoder_prefetch;
    TU8 b_set_dsp_prebuffer;

    //rtsp source
    TU8 rtsp_streaming_from_playback_source;
    TU8 rtsp_streaming_from_local_file;
    TU8 rtsp_streaming_from_dsp_transcoder;
    TU8 rtsp_streaming_from_dsp_encoder;

    //engine
    IGenericEngineControl *p_engine;
    EMediaWorkingMode work_mode;

    TUint displayDeviceMask;
    TUint inputDeviceMask;

    //streaming server
    TGenericID rtspStreamingServerID;
    TGenericID rtspStreamingTransmiterID;
    TUint mnRTSPStreamingFromPlaybackSource;
    TUint mnRTSPStreamingFromLocalFileSource;
    TUint mnRTSPStreamingFromDSPTranscoder;
    TUint mnRTSPStreamingFromDSPCameraEncoder;
    TGenericID rtspStreamingContentPlaybackID[EComponentMaxNumber_RTSPStreamingContent];
    TGenericID rtspStreamingContentLocalFileID[EComponentMaxNumber_RTSPStreamingContent];
    TGenericID rtspStreamingContentDSPTranscoder[EComponentMaxNumber_RTSPStreamingContent];
    TGenericID rtspStreamingContentDSPCameraEncoder[EComponentMaxNumber_RTSPStreamingContent];

    TGenericID rtspStreamingContentFromRemoteAPP[EComponentMaxNumber_RTSPStreamingContent];

    //cloud server
    TGenericID cloudNVRServerID;
    TUint mnCloudReceiverCamera;
    TGenericID cloudClientReceiverCameraID[EComponentMaxNumber_CloudClientAgentReceiverFromCamera];
    TUint mnCloudReceiverAPP;
    TGenericID cloudClientReceiverAPPID[EComponentMaxNumber_CloudClientAgentReceiverFromAPP];

    TGenericID cloudClientReceiverCameraContentID[EComponentMaxNumber_CloudClientAgentReceiverFromCamera];
    TGenericID cloudClientReceiverAPPContentID[EComponentMaxNumber_CloudClientAgentReceiverFromAPP];

    //engine's process pipelines
    TUint mnPlaybackPipelineNumber;
    TUint mnRecordingPipelineNumber;
    TUint mnStreamingPipelineNumber;
    TGenericID playback_pipelines[EPipelineMaxNumber_Playback * 2];
    TGenericID recording_pipelines[EPipelineMaxNumber_Recording * 2];
    TGenericID streaming_pipelines[EPipelineMaxNumber_Streaming];

    //detailed configurations for engine, blow mainly for debug only
    //engine's components
    TUint mnDemuxerNumber;
    TUint mnVideoDecoderNumber;
    TUint mnAudioDecoderNumber;
    TUint mnVideoPostpNumber;
    TUint mnAudioPostpNumber;
    TUint mnVideoRendererNumber;
    TUint mnAudioRendererNumber;
    TUint mnMuxerNumber;

    TUint mnStreamingServerNumber;
    TUint mnStreamingDataTransmiterNumber;
    TUint mnStreamingContentNumber;

    TUint mnVideoCaptureNumber;
    TUint mnAudioCaptureNumber;
    TUint mnVideoEncoderNumber;
    TUint mnAudioEncoderNumber;

    TGenericID demuxerID[EComponentMaxNumber_Demuxer];
    TGenericID videoDecoderID[EComponentMaxNumber_VideoDecoder];
    TGenericID audioDecoderID[EComponentMaxNumber_AudioDecoder];
    //TGenericID videoPostProcessorID[EComponentMaxNumber_VideoPostProcessor];
    //TGenericID audioPostProcessorID[EComponentMaxNumber_AudioPostProcessor];
    TGenericID videoRendererID[EComponentMaxNumber_VideoRenderer];
    TGenericID audioRendererID[EComponentMaxNumber_AudioRenderer];
    TGenericID muxerID[EComponentMaxNumber_Muxer];
    //TGenericID streamingServer[EComponentMaxNumber_StreamingServer];
    //TGenericID streamingRTSPTransmiter[EComponentMaxNumber_RTSPStreamingTransmiter];
    //TGenericID streamingRTSPContent[EComponentMaxNumber_RTSPStreamingContent];

    TGenericID videoCaptureID[EComponentMaxNumber_VideoCapture];
    TGenericID audioCaptureID[EComponentMaxNumber_AudioCapture];
    TGenericID videoEncoderID[EComponentMaxNumber_VideoEncoder];
    TGenericID audioEncoderID[EComponentMaxNumber_AudioEncoder];

    //pin muxer
    TGenericID audioPinMuxerID[EComponentMaxNumber_AudioPinMuxer];

    //engine connections between components
    TGenericID connections_demuxer_2_video_decoder[EPipelineMaxNumber_Playback * 2];
    TGenericID connections_demuxer_2_audio_decoder[EPipelineMaxNumber_Playback];
    TGenericID connections_demuxer_2_muxer[EPipelineMaxNumber_Recording * 2];
    TGenericID connections_demuxer_2_rtsp_transmiter[EPipelineMaxNumber_Streaming * 2];

    TGenericID connections_demuxer_2_audio_pinmuxer[EPipelineMaxNumber_Playback];
    TGenericID connections_audio_pinmuxer_2_audio_decoder[EPipelineMaxNumber_Playback];
    TGenericID connections_audio_pinmuxer_2_streamer[EPipelineMaxNumber_Playback];

    TGenericID connections_video_decoder_2_video_renderer[EPipelineMaxNumber_Playback];
    TGenericID connections_audio_decoder_2_audio_renderer[EPipelineMaxNumber_Playback];

    TGenericID connections_video_capture_2_video_encoder[EPipelineMaxNumber_Recording];
    TGenericID connections_audio_capture_2_audio_encoder[EPipelineMaxNumber_Recording];

    TGenericID connections_video_encoder_2_muxer[EPipelineMaxNumber_Recording];
    TGenericID connections_audio_encoder_2_muxer[EPipelineMaxNumber_Recording];

    TGenericID connections_video_encoder_2_rtspstreamer[EPipelineMaxNumber_Streaming];
    TGenericID connections_audio_encoder_2_rtspstreamer[EPipelineMaxNumber_Streaming];

    //runtime status
    TU8 dsp_dec_trickplay[EPipelineMaxNumber_Playback];//0 resumed, 1 paused, 2 step mode
} s_media_processor_context;

static s_media_processor_context g_media_processor_context;

TULong g_cur_playback_url_group = 0, g_cur_recording_url_group = 0, g_cur_streaming_url_group = 0;
//static TU32 g_current_play_hd = 0;
/*
#define MAX_CHANNELS    20
#define DBasicScore (1<<8)
static TU64 performance_base = 1920*1080;
static TU32 tot_performance_score = 0;
static TU32 system_max_performance_score = DBasicScore * 2;//i1 1080p30 x 2
static TU32 cur_pb_speed[MAX_CHANNELS] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
static TInt get_stream_size(IGenericEngineControl* p_engine, TU16 window_index, TInt* p_width, TInt* p_height)
{
    SVideoPostPDisplayMWMapping mapping;
    EECode err = EECode_OK;
    SVideoCodecInfo dec_info;
    if (window_index >= (g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2)) {
        test_loge("get_stream_size, window id(%hu) exceed max value\n", window_index);
        return -1;
    }
    mapping.window_id = window_index;
    mapping.check_field = EGenericEngineQuery_VideoDisplayWindow;
    err = p_engine->GenericControl(EGenericEngineQuery_VideoDisplayWindow, (void *)&mapping);
    if (EECode_OK != err) {
        test_loge("get_stream_size, EGenericEngineQuery_VideoDisplayWindow fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return -1;
    }
    if (DINVALID_VALUE_TAG == mapping.decoder_id) {
        test_loge("get_stream_size, EGenericEngineQuery_VideoDisplayWindow fail, bad window id %d, ret %d, %s\n", mapping.window_id, err, gfGetErrorCodeString(err));
        return -1;
    }
    test_log("get_stream_size, EGenericEngineQuery_VideoDisplayWindow done, window_id=%d, decoder_id=%u, render_id=%u\n",
        mapping.window_id, mapping.decoder_id, mapping.render_id);

    dec_info.dec_id = mapping.decoder_id;
    dec_info.check_field = EGenericEngineQuery_VideoCodecInfo;
    err = p_engine->GenericControl(EGenericEngineQuery_VideoCodecInfo, (void *)&dec_info);
    DASSERT_OK(err);
    if (EECode_OK != err) {
        test_loge("get_stream_size, EGenericEngineQuery_VideoCodecInfo dec_id %d fail\n", dec_info.dec_id);
        return -1;
    }

    test_log("get_stream_size, EGenericEngineQuery_VideoCodecInfo done. dec_id=%d, pic_width=%u, pic_height=%u\n", dec_info.dec_id,dec_info.pic_width,dec_info.pic_height);
    *p_width = dec_info.pic_width;
    *p_height = dec_info.pic_height;
    return 0;
}

static TU16 max_possiable_speed(IGenericEngineControl* p_engine, TU16 window_index)
{
    TU16 i;
    TInt width, height;
    TU16 ret = 1;

    DASSERT(p_engine);
    if (!p_engine) {
        return 1;
    }

    if (g_current_play_hd) {
        // 1x's case
        if (0 > get_stream_size(p_engine, window_index, &width, &height)) {
            test_loge("get_stream_size() failed for window_index=%hu, max_possiable_speed use 1 as default.\n", window_index);
            return 1;
        }
        i = (TU16)((TU64)width * (TU64)height * DBasicScore / performance_base);
        if (i) {
            ret = system_max_performance_score/i;
            return ret;
        } else {
            test_loge("invalid width %d, or height %d\n", width, height);
            return 1;
        }
    } else {
        if (window_index >= (g_mtest_dsp_dec_instances_number_1)) {
            test_loge("BAD channel index %hu, tot sd number %d\n", window_index, g_mtest_dsp_dec_instances_number_1);
            return 1;
        }
        tot_performance_score = 0;
        for (i = 0; i < g_mtest_dsp_dec_instances_number_1; i ++) {
            if (i == window_index) {
                continue;
            }
            if (0 > get_stream_size(p_engine, i, &width, &height)) {
                test_loge("get_stream_size() failed for window_index=%hu, max_possiable_speed use 1 as default.\n", i);
                return 1;
            }
            tot_performance_score += (TUint)((TU64)width * (TU64)height * DBasicScore * cur_pb_speed[i] / performance_base);
            test_log("window_index=%hu, cur_pb_speed=%u, w=%d h=%d, tot_performance_score=%u.\n", i, cur_pb_speed[i], width, height,  tot_performance_score);
        }

        if (0 > get_stream_size(p_engine, window_index, &width, &height)) {
            test_loge("get_stream_size() failed for window_index=%hu, max_possiable_speed use 1 as default.\n", window_index);
            return 1;
        }

        if (tot_performance_score < system_max_performance_score) {
            ret = (TU16)((TU64)width * (TU64)height * DBasicScore / performance_base);
            ret = (system_max_performance_score - tot_performance_score)/ret;
            return ret;
        } else {
            test_loge("error, tot_performance_score %d >= system_max_performance_score %d\n", tot_performance_score, system_max_performance_score);
        }
    }

    return 1;
}
*/
void init_context(s_media_processor_context *p_content)
{
    DASSERT(p_content);
    if (p_content) {
        memset(p_content, 0x0, sizeof(s_media_processor_context));
    }
}

void release_content(s_media_processor_context *p_content)
{
    TULong i = 0, j = 0;

    DASSERT(p_content);
    if (p_content) {

        for (i = 0; i < d_max_source_url_group_number; i ++) {
            for (j = 0; j < d_max_source_url_number; j ++) {
                if (p_content->source_url_group[i][j].p_url) {
                    free(p_content->source_url_group[i][j].p_url);
                    p_content->source_url_group[i][j].p_url = NULL;
                    p_content->source_url_group[i][j].url_buffer_length = 0;
                }
            }
        }

        for (i = 0; i < d_max_sink_url_group_number; i ++) {
            for (j = 0; j < d_max_sink_url_number; j ++) {
                if (p_content->sink_url_group[i][j].p_url) {
                    free(p_content->sink_url_group[i][j].p_url);
                    p_content->sink_url_group[i][j].p_url = NULL;
                    p_content->sink_url_group[i][j].url_buffer_length = 0;
                }
            }
        }

        for (i = 0; i < d_max_streaming_url_group_number; i ++) {
            for (j = 0; j < d_max_streaming_url_number; j ++) {
                if (p_content->streaming_url_group[i][j].p_url) {
                    free(p_content->streaming_url_group[i][j].p_url);
                    p_content->streaming_url_group[i][j].p_url = NULL;
                    p_content->streaming_url_group[i][j].url_buffer_length = 0;
                }
            }
        }

    }
}

TInt add_url_into_context(TChar *url, TULong type, TULong group, TULong is_hd)
{
    TULong current_count = 0;
    TChar *ptmp = NULL;

    DASSERT(url);
    DASSERT(e_url_type_total_count > type);

    if ((!url) || (e_url_type_total_count <= type)) {
        test_loge("BAD input params in add_url_into_context: url %p, type %ld\n", url, type);
        return (-3);
    }

    if (e_url_type_source == type) {

        DASSERT(d_max_source_url_group_number > group);
        if (d_max_source_url_group_number <= group) {
            test_loge("BAD input params in add_url_into_context: group %ld\n", group);
            return (-3);
        }

        current_count = g_media_processor_context.source_url_count_in_group[group];
        if (current_count < d_max_source_url_number) {
            DASSERT(!g_media_processor_context.source_url_group[group][current_count].p_url);
            DASSERT(!g_media_processor_context.source_url_group[group][current_count].url_buffer_length);
            ptmp = (TChar *)malloc(strlen(url) + 4);
            DASSERT(ptmp);
            if (ptmp) {
                g_media_processor_context.source_url_group[group][current_count].p_url = ptmp;
                g_media_processor_context.source_url_group[group][current_count].url_buffer_length = strlen(url);
                strcpy(ptmp, url);
                g_media_processor_context.source_url_count_in_group[group] ++;
            } else {
                test_loge("NO memory for store url %s\n", url);
                return (-4);
            }
        } else {
            test_loge("Excced: current count %lu, max count %d\n", current_count, d_max_source_url_number);
            return (-5);
        }

        if (!is_hd) {
            g_media_processor_context.source_sd_url_count_in_group[group] ++;
        } else {
            g_media_processor_context.source_hd_url_count_in_group[group] ++;
        }

    } else if (e_url_type_sink == type) {

        DASSERT(d_max_sink_url_group_number > group);
        if (d_max_sink_url_group_number <= group) {
            test_loge("BAD input params in add_url_into_context: group %ld\n", group);
            return (-3);
        }

        current_count = g_media_processor_context.sink_url_count_in_group[group];

        if (current_count < d_max_sink_url_number) {
            DASSERT(!g_media_processor_context.sink_url_group[group][current_count].p_url);
            DASSERT(!g_media_processor_context.sink_url_group[group][current_count].url_buffer_length);
            ptmp = (TChar *)malloc(strlen(url) + 4);
            DASSERT(ptmp);
            if (ptmp) {
                g_media_processor_context.sink_url_group[group][current_count].p_url = ptmp;
                g_media_processor_context.sink_url_group[group][current_count].url_buffer_length = strlen(url);
                strcpy(ptmp, url);
                g_media_processor_context.sink_url_count_in_group[group] ++;
            } else {
                test_loge("NO memory for store url %s\n", url);
                return (-4);
            }
        } else {
            test_loge("Excced: current count %lu, max count %d\n", current_count, d_max_sink_url_number);
            return (-5);
        }
    } else if (e_url_type_streaming == type) {

        DASSERT(d_max_streaming_url_group_number > group);
        if (d_max_streaming_url_group_number <= group) {
            test_loge("BAD input params in add_url_into_context: group %ld\n", group);
            return (-3);
        }

        current_count = g_media_processor_context.streaming_url_count_in_group[group];

        if (current_count < d_max_streaming_url_number) {
            DASSERT(!g_media_processor_context.streaming_url_group[group][current_count].p_url);
            DASSERT(!g_media_processor_context.streaming_url_group[group][current_count].url_buffer_length);
            ptmp = (TChar *)malloc(strlen(url) + 4);
            DASSERT(ptmp);
            if (ptmp) {
                g_media_processor_context.streaming_url_group[group][current_count].p_url = ptmp;
                g_media_processor_context.streaming_url_group[group][current_count].url_buffer_length = strlen(url);
                strcpy(ptmp, url);
                g_media_processor_context.streaming_url_count_in_group[group] ++;
            } else {
                test_loge("NO memory for store url %s\n", url);
                return (-4);
            }
        } else {
            test_loge("Excced: current count %lu, max count %d\n", current_count, d_max_streaming_url_number);
            return (-5);
        }
    } else {
        test_loge("Must not comes here, type %ld\n", type);
    }

    return 0;
}

void _mtest_print_version()
{
    TU32 major, minor, patch;
    TU32 year;
    TU32 month, day;

    printf("\ncurrent version:\n");
    gfGetCommonVersion(major, minor, patch, year, month, day);
    printf("\tlibmwcg_common version: %d.%d.%d, date: %04d-%02d-%02d\n", major, minor, patch, year, month, day);
    gfGetMediaMWVersion(major, minor, patch, year, month, day);
    printf("\tlibmwcg_media_mw version: %d.%d.%d, date: %04d-%02d-%02d\n", major, minor, patch, year, month, day);
}

void _mtest_print_usage()
{
    printf("\nmtest options:\n");
    printf("\t'-f [filename]': '-f' specify input file name/url, for demuxer\n");
    printf("\t'-r [filename]': '-r' specify output file name/url, for muxer\n");
    printf("\t'-s [filename]': '-s' specify streaming tag name, for streaming\n");

    printf("\t'--udec': will choose udec mode\n");
    printf("\t'--mudec': will choose multiple window udec mode\n");
    printf("\t'--genericnvr': will choose generic nvr, recording + streaming, not local playback\n");
    printf("\t'--voutmask [mask]': '--voutmask' specify output device's mask, LCD's bit is 0x01, HDMI's bit is 0x02\n");
    printf("\t'--saveall': save all files (.ts format)\n");
    printf("\t'--enable-compensatejitter': try to monitor delay and compensate it from dsp side\n");
    printf("\t'--disable-compensatejitter': disable the monitor delay and compensate it from dsp side\n");
    printf("\t'--help': print help\n");
    printf("\t'--showversion': show current version\n");

    printf("\n\tblow will show playback mode in multiple window UDEC\n");
    printf("\t\t'--1x1080p': [mudec mode], 1x1080p\n");
    printf("\t\t'--4+1': [mudec mode], 1x1080p + 4xD1\n");
    printf("\t\t'--6+1': [mudec mode], 1x1080p + 6xD1\n");
    printf("\t\t'--8+1': [mudec mode], 1x1080p + 8xD1\n");
    printf("\t\t'--9+1': [mudec mode], 1x1080p + 9xD1\n");
    printf("\t\t'--4x720p': [mudec mode], 4x720p\n");
    printf("\t\t'--4xD1': [mudec mode], 4xD1\n");
    printf("\t\t'--6xD1': [mudec mode], 6xD1\n");
    printf("\t\t'--8xD1': [mudec mode], 8xD1\n");
    printf("\t\t'--9xD1': [mudec mode], 9xD1\n");
    printf("\t\t'--1x3M': [mudec mode], 1x3M\n");
    printf("\t\t'--4+1x3M': [mudec mode], 1x3M + 4xD1\n");
    printf("\t\t'--6+1x3M': [mudec mode], 1x3M + 6xD1\n");
    printf("\t\t'--8+1x3M': [mudec mode], 1x3M + 8xD1\n");
    printf("\t\t'--9+1x3M': [mudec mode], 1x3M + 9xD1\n");

    printf("\n\tdebug only options:\n");
    printf("\t'--loglevel [level]': '--loglevel' specify log level, regardless the log.config\n");
    printf("\t'--enable-audio': will enable audio path\n");
    printf("\t'--enable-video': will enable video path\n");
    printf("\t'--disable-audio': will disable audio path\n");
    printf("\t'--disable-video': will disable video path\n");
    printf("\t'--multipleaudiodecoder': will assign audio decoder for each playback pipeline\n");
    printf("\t'--uniqueaudiodecoder': will share one audio decoder for all playback pipeline\n");
    printf("\t'--sharedemuxer': debug only, will share demuxer\n");
    printf("\t'--maxbuffernumber [%%d]': will set buffer number of each udec instance (dsp), value should be in 7~20\n");
    printf("\t'--prebuffer [%%d]': will set prebuffer at dsp side, value should be in 1~{maxbuffernumber - 5}\n");
    printf("\t'--prefetch [%%d]': arm will prefetch some data(%%d frames) before start playback\n");
    printf("\t'--timeout [%%d]': set timeout threashold for streaming, %%d seconds, if timeout occur, will reconnect server\n");
    printf("\t'--reconnectinterval [%%d]': auto reconnect server interval, seconds\n");
    printf("\t'--nousequpes': not feed udec wrapper\n");
    printf("\t'--usequpes': feed udec wrapper\n");
    printf("\t'--nopts': not feed pts\n");
    printf("\t'--fps': specify fps\n");

    printf("\t'--prefer-ffmpeg-muxer': prefer ffmpeg muxer\n");
    printf("\t'--prefer-ts-muxer': prefer private ts muxer\n");
    printf("\t'--prefer-ffmpeg-audio-decoder': prefer ffmpeg audio decoder\n");
    printf("\t'--prefer-aac-audio-decoder': prefer aac audio decoder\n");
}

void _mtest_print_runtime_usage()
{
    printf("\nmtest runtime cmds: press cmd + Enter\n");
    printf("\t'q': quit unit test\n");
    printf("\t'pall': will print all components' status\n");
    printf("\t'plog': will print log configure\n");
    printf("\t'pdsp:[%%d]': will print udec(%%d)'s status\n");
    printf("\t'csavelogconfig': will save current log config settings to log.config\n");

    printf("\ttrickplay category\n");
    printf("\t\t' %%d', pause/resume, %%d is udec_id\n");
    printf("\t\t' w%%d', pause/resume, %%d is window_id\n");
    printf("\t\t's%%d', step play, %%d is udec_id\n");
    printf("\t\t'sw%%d', step play, %%d is window_id\n");

    printf("\t'cc': playback capture category:\n");
    printf("\t\t'ccjpeg:%%x %%x', udec_id %%x(if udec_id == 0xff, means screen shot), capture jpeg file index %%x.\n");

    printf("\t'cz': playback zoom category:\n");
    printf("\t\t'cz2:%%d,%%hd,%%hd,%%hd,%%hd', (zoom mode 2, input window) render_id %%d, input width %%d, height %%d, center x %%d, y %%d.\n");
    printf("\t\t'cz2w:%%d,%%hd,%%hd,%%hd,%%hd', (zoom mode 2, input window) window_id %%d, input width %%d, height %%d, center x %%d, y %%d.\n");

    printf("\taudio related:\n");
    printf("\t\t'cmute': mute audio.\n");
    printf("\t\t'cumute': umute audio.\n");
    printf("\t\t'cselectaudio:%%d': select audiochannel %%d.\n");

    printf("\tfast FW/BW related:\n");
    printf("\t\t'cpf:%%d %%x.%%x %%d': fast forward, %%d means udec instance, %%x.%%x means speed, %%d means feeding rule(1: all frame feeding, 2: ref only, 3 I Only)\n");
    printf("\t\t'cpw:%%d %%x.%%x %%d': fast backward, %%d means udec instance, %%x.%%x means speed, %%d means feeding rule(1: all frame feeding, 2: ref only, 3 I Only)\n");
    printf("\t\t'cpc:%%d': reset to 1x forward playback\n");

    printf("\transcode related:\n");
    printf("\t\t'--vencparas %%u %%u %%u %%u %%u %%u %%u %%u', set video transcode parameters, %%u encoder generic_id, %%u bitrate, %%u framerate, %%u demandIDR, %%u gop_M, %%u gop_N, %%u gop_idr_interval, %%u gop_structure\n");
    printf("\t\t'--vencbitrate %%u %%u', set video transcode bitrate, %%u encoder generic_id, %%u bitrate\n");
    printf("\t\t'--vencframerate %%u %%u', set video transcode framerate, %%u encoder generic_id, %%u framerate\n");
    printf("\t\t'--vencidr %%u %%u', video transcode demand IDR, %%u encoder generic_id, %%u demandIDR\n");
    printf("\t\t'--vencgop %%u %%u %%u %%u %%u', set video transcode gop, %%u encoder generic_id, %%u gop_M, %%u gop_N, %%u gop_idr_interval, %%u gop_structure\n");

    printf("\trecord related:\n");
    printf("\t\t'--recsplitduration %%u %%u', set record split duration, %%u record file index, %%u duration(seconds)\n");
    printf("\t\t'--recsplitframe %%u %%u', set record split frame count, %%u record file index, %%u frame number\n");

    printf("\t'--' some other cmds:\n");
    printf("\t\t'--sd', switch to multiple window view:\n");
    printf("\t\t'--hd %%d', switch to single window view: %%d means hd's index\n");
    printf("\t\t'--layout %%d %%d', switch to display layout, %%d: 1 means 'MxN table view', 2 means tele-presence view', '3 means 'highlighten view', %%d:0-window 0 show SD source, 1-window 0 show HD source, only valid for tele-presence view\n");
    printf("\t\t'--loopmode:%%u %%hu', update loop mode, %%u source index, %%hu loop mode: 0-no loop, 1-single source loop(default), 2-all sources loop\n");
    printf("\t\t'--wtohd %%u %%u', switch certain sd window to HD, %%u sd window index, %%u tobeHD: 0-to SD, 1-to HD\n");
    printf("\t\t'--switchsourcegroup %%u, switch source group, %%u target_source_group_index, [0, x]\n");

    printf("\tnetwork related:\n");
    printf("\t\t'creconnectserver %%d', reconnect rtsp server manually, %%d means source index, if %%d==255, means reconnect all servers\n");
    printf("\t\t'creconnectall', reconnect all rtsp servers manually\n");
}

TInt mtest_init_params(TInt argc, TChar **argv)
{
    TInt i = 0;
    TInt ret = 0;
    //    TInt started_scan_filename = 0;
    //    TUint param0, param1, param2, param3, param4, param5;
    //    TULong long performance_base = 1920*1080;

    TULong cur_playback_url_group = 0, cur_recording_url_group = 0, cur_streaming_url_group = 0;
    TULong cur_source_is_hd = 0;

    if (argc < 2) {
        //_mtest_print_version();
        _mtest_print_usage();
        _mtest_print_runtime_usage();
        return 1;
    }

    //g_current_play_hd = 0;

    //parse options
    for (i = 1; i < argc; i++) {
        if (!strcmp("-f", argv[i])) {
            if ((i + 1) < argc) {
                ret = add_url_into_context(argv[i + 1], e_url_type_source, cur_playback_url_group, cur_source_is_hd);
                test_log("add_url_into_context, argv[i + 1]: %s, is_hd %ld\n", argv[i + 1], cur_source_is_hd);
                if (ret < 0) {
                    test_loge("[input argument error]: add_url_into_context ret(%d).\n", ret);
                    return (-3);
                }
            } else {
                test_loge("[input argument error]: '-f', should follow with source url(%%s), argc %d, i %d.\n", argc, i);
                return (-4);
            }
            i ++;
        } else if (!strcmp("--hdsource", argv[i])) {
            test_log("'--hdsource': add_url_into_context, hd source start...\n");
            cur_source_is_hd = 1;
        } else if (!strcmp("--sdsource", argv[i])) {
            test_log("'--sdsource': add_url_into_context, sd source start...\n");
            cur_source_is_hd = 0;
        } else if (!strcmp("--nextgroupsource", argv[i])) {
            test_log("'--nextgroupsource': next group source url...\n");
            cur_playback_url_group ++;
            cur_source_is_hd = 0;
        } else if (!strcmp("--nextgroupsink", argv[i])) {
            test_log("'--nextgroupsink': next group sink url...\n");
            cur_recording_url_group ++;
            cur_source_is_hd = 0;
        } else if (!strcmp("--nextgroupstreaming", argv[i])) {
            test_log("'--nextgroupstreaming': next group streaming url...\n");
            cur_streaming_url_group ++;
            cur_source_is_hd = 0;
        } else if (!strcmp("--saveall", argv[i])) {
            test_log("'--saveall': save all streams\n");
            g_mtest_save_all_channel = 1;
        } else if (!strcmp("--showversion", argv[i])) {
            _mtest_print_version();
            return (18);
        } else if (!strcmp("--sharedemuxer", argv[i])) {
            test_log("'--sharedemuxer': share demuxer\n");
            g_mtest_share_demuxer = 1;
        } else if (!strcmp("--prefer-ffmpeg-muxer", argv[i])) {
            test_log("'--prefer-ffmpeg-muxer': prefer ffmpeg muxer\n");
            gb_mtest_prefer_ffmpeg_muxer = 1;
        } else if (!strcmp("--prefer-ts-muxer", argv[i])) {
            test_log("'--prefer-ts-muxer': prefer private ts muxer\n");
            gb_mtest_prefer_private_ts_muxer = 1;
        } else if (!strcmp("--prefer-ffmpeg-audio-decoder", argv[i])) {
            test_log("'--prefer-ffmpeg-audio-decoder': prefer ffmpeg audio decoder\n");
            gb_mtest_prefer_ffmpeg_audio_decoder = 1;
        } else if (!strcmp("--prefer-aac-audio-decoder", argv[i])) {
            test_log("'--prefer-aac-audio-decoder': prefer aac audio decoder\n");
            gb_mtest_prefer_private_aac_decoder = 1;
        } else if (!strcmp("--tcpmode", argv[i])) {
            test_log("'--tcpmode': let rtsp client try tcp mode first\n");
            gb_mtest_rtsp_client_try_tcp_mode_first = 1;
        } else if (!strcmp("--rtspserverport", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                g_mtest_rtsp_server_port = ret;
                test_log("[input argument]: '--rtspserverport': (%d).\n", g_mtest_rtsp_server_port);
            } else {
                test_loge("[input argument error]: '--rtspserverport', should follow with integer(port), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("-r", argv[i])) {
            if ((i + 1) < argc) {
                ret = add_url_into_context(argv[i + 1], e_url_type_sink, cur_recording_url_group, cur_source_is_hd);
                if (ret < 0) {
                    test_loge("[input argument error]: add_url_into_context ret(%d).\n", ret);
                    return (-5);
                }
            } else {
                test_loge("[input argument error]: '-r', should follow with sink url(%%s), argc %d, i %d.\n", argc, i);
                return (-6);
            }
            i ++;
        } else if (!strcmp("-s", argv[i])) {
            if ((i + 1) < argc) {
                ret = add_url_into_context(argv[i + 1], e_url_type_streaming, cur_streaming_url_group, cur_source_is_hd);
                if (ret < 0) {
                    test_loge("[input argument error]: add_url_into_context ret(%d).\n", ret);
                    return (-7);
                }
            } else {
                test_loge("[input argument error]: '-F', should follow with sink url(%%s), argc %d, i %d.\n", argc, i);
                return (-8);
            }
            i ++;
        } else if (!strcmp("--voutmask", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > 3) || (ret < 0)) {
                    test_loge("[input argument error]: '--voutmask', ret(%d) should between 0 to 3.\n", ret);
                    return (-9);
                } else {
                    g_mtest_voutmask = ret;
                    test_log("[input argument]: '--voutmask': (0x%x).\n", g_mtest_voutmask);
                    if (g_mtest_voutmask == 0) {
                        test_log("[input argument]: '--voutmask' disable vout!\n");
                    }
                }
            } else {
                test_loge("[input argument error]: '--voutmask', should follow with integer(voutmask), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--loglevel", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > ELogLevel_Verbose) || (ret < ELogLevel_Fatal)) {
                    test_loge("[input argument error]: '--loglevel', ret(%d) should between %d to %d.\n", ret, ELogLevel_Fatal, ELogLevel_Verbose);
                    return (-9);
                } else {
                    g_mtest_config_log_level = ret;
                    g_b_mtest_force_log_level = 1;
                    const TChar *p_log_level_string = NULL;
                    gfGetLogLevelString((ELogLevel)g_mtest_config_log_level, p_log_level_string);
                    test_log("[input argument]: '--loglevel': (%d) (%s).\n", g_mtest_config_log_level, p_log_level_string);
                }
            } else {
                test_loge("[input argument error]: '--loglevel', should follow with integer(loglevel), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--maxbuffernumber", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > 20) || (ret < 7)) {
                    test_loge("[input argument error]: '--maxbuffernumber', (%d) should between %d to %d.\n", ret, 7, 20);
                    return (-25);
                } else {
                    g_mtest_dsp_total_buffer_count = ret;
                    test_log("[input argument]: '--maxbuffernumber': (%d).\n", g_mtest_dsp_total_buffer_count);
                }
            } else {
                test_loge("[input argument error]: '--maxbuffernumber', should follow with integer(maxbuffernumber), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--prebuffer", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > 15) || (ret < 0)) {
                    test_loge("[input argument error]: '--prebuffer', (%d) should between %d to %d.\n", ret, 0, 15);
                    return (-9);
                } else {
                    g_mtest_dsp_prebuffer_count = ret;
                    gb_mtest_request_dsp_prebuffer_count = 1;
                    test_log("[input argument]: '--prebuffer': (%d).\n", g_mtest_dsp_prebuffer_count);
                }
            } else {
                test_loge("[input argument error]: '--prebuffer', should follow with integer(prebuffer count), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--prefetch", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > 30) || (ret < 1)) {
                    test_loge("[input argument error]: '--prefetch', (%d) should between %d to %d.\n", ret, 1, 30);
                    return (-9);
                } else {
                    g_mtest_decoder_prefetch_count = ret;
                    gb_mtest_request_decoder_prefetch_count = 1;
                    test_log("[input argument]: '--prefetch': (%d).\n", g_mtest_decoder_prefetch_count);
                }
            } else {
                test_loge("[input argument error]: '--prefetch', should follow with integer(prefetch count), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--timeout", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > 30) || (ret < 5)) {
                    test_loge("[input argument error]: '--timeout', (%d) should between %d to %d.\n", ret, 5, 30);
                    return (-9);
                } else {
                    g_mtest_streaming_timeout_threashold = ret;
                    test_log("[input argument]: '--timeout': (%d).\n", g_mtest_streaming_timeout_threashold);
                }
            } else {
                test_loge("[input argument error]: '--timeout', should follow with integer(timeout), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--reconnectinterval", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > 30) || (ret < 2)) {
                    test_loge("[input argument error]: '--reconnectinterval', (%d) should between %d to %d.\n", ret, 2, 30);
                    return (-9);
                } else {
                    g_mtest_streaming_retry_interval = ret;
                    test_log("[input argument]: '--reconnectinterval': (%d).\n", g_mtest_streaming_retry_interval);
                }
            } else {
                test_loge("[input argument error]: '--reconnectinterval', should follow with integer(reconnectinterval), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--disable-audio", argv[i])) {
            test_log("[input argument]: '--disable-audio', disable audio.\n");
            g_mtest_config_enable_audio = 0;
        } else if (!strcmp("--enable-audio", argv[i])) {
            test_log("[input argument]: '--enable-audio', enable audio.\n");
            g_mtest_config_enable_audio = 1;
        } else if (!strcmp("--disable-video", argv[i])) {
            test_log("[input argument]: '--disable-audio', disable video.\n");
            g_mtest_config_enable_video = 0;
        } else if (!strcmp("--enable-video", argv[i])) {
            test_log("[input argument]: '--enable-audio', enable video.\n");
            g_mtest_config_enable_video = 1;
        } else if (!strcmp("--nousequpes", argv[i])) {
            test_log("[input argument]: '--nousequpes', not add udec warpper.\n");
            gb_mtest_udec_no_usequpes = 0;
        } else if (!strcmp("--usequpes", argv[i])) {
            test_log("[input argument]: '--usequpes', add udec wrapper.\n");
            gb_mtest_udec_no_usequpes = 1;
        } else if (!strcmp("--nopts", argv[i])) {
            test_log("[input argument]: '--nopts', not feed udec pts.\n");
            gb_mtest_udec_not_feed_pts = 1;
        } else if (!strcmp("--fps", argv[i])) {
            TInt fps = 0;
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &fps))) {
                gb_mtest_udec_specify_fps = fps;
            } else {
                test_loge("[input argument error]: '--fps', should follow with integer, argc %d, i %d.\n", argc, i);
                gb_mtest_udec_specify_fps = 0;
            }
            i ++;
            test_log("[input argument]: '--fps', specify fps %d.\n", gb_mtest_udec_specify_fps);
        } else if (!strcmp("--enable-compensatejitter", argv[i])) {
            test_log("[input argument]: '--enable-compensatejitter', try to compensate delay from jitter.\n");
            gb_mtest_compensate_delay_from_jitter = 1;
        } else if (!strcmp("--disable-compensatejitter", argv[i])) {
            test_log("[input argument]: '--disable-compensatejitter', disable compensate delay from jitter.\n");
            gb_mtest_compensate_delay_from_jitter = 0;
        } else if (!strcmp("--dspmode", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > 3) || (ret < 1)) {
                    test_loge("[input argument error]: '--dspmode', ret(%d) should between 1 to 3.\n", ret);
                    return (-11);
                } else {
                    g_mtest_dspmode = (EMediaWorkingMode)ret;
                    test_log("[input argument]: '--dspmode': (%d).\n", g_mtest_dspmode);
                }
            } else {
                test_loge("[input argument error]: '--dspmode', should follow with integer(voutmask), argc %d, i %d.\n", argc, i);
            }
            i ++;
        } else if (!strcmp("--netreceiver-scheduler", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > DMaxSchedulerGroupNumber) || (ret < 0)) {
                    test_loge("[input argument error]: '--netreceiver-scheduler', number(%d) should between 0 to %d.\n", ret, DMaxSchedulerGroupNumber);
                    return (-11);
                } else {
                    g_mtest_net_receiver_scheduler_number = ret;

                    test_log("[input argument]: '--netreceiver-scheduler': (%d).\n", g_mtest_net_receiver_scheduler_number);
                }
            } else {
                test_loge("[input argument error]: '--netreceiver-scheduler', should follow with integer(number), argc %d, i %d.\n", argc, i);
            }
            i ++;
        } else if (!strcmp("--netsender-scheduler", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > DMaxSchedulerGroupNumber) || (ret < 0)) {
                    test_loge("[input argument error]: '--netsender-scheduler', number(%d) should between 0 to %d.\n", ret, DMaxSchedulerGroupNumber);
                    return (-11);
                } else {
                    g_mtest_net_sender_scheduler_number = ret;

                    test_log("[input argument]: '--netsender-scheduler': (%d).\n", g_mtest_net_sender_scheduler_number);
                }
            } else {
                test_loge("[input argument error]: '--netsender-scheduler', should follow with integer(number), argc %d, i %d.\n", argc, i);
            }
            i ++;
        } else if (!strcmp("--fileioreader-scheduler", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > DMaxSchedulerGroupNumber) || (ret < 0)) {
                    test_loge("[input argument error]: '--fileioreader-scheduler', number(%d) should between 0 to %d.\n", ret, DMaxSchedulerGroupNumber);
                    return (-11);
                } else {
                    g_mtest_fileio_reader_scheduler_number = ret;

                    test_log("[input argument]: '--fileioreader-scheduler': (%d).\n", g_mtest_fileio_reader_scheduler_number);
                }
            } else {
                test_loge("[input argument error]: '--fileioreader-scheduler', should follow with integer(number), argc %d, i %d.\n", argc, i);
            }
            i ++;
        } else if (!strcmp("--fileiowriter-scheduler", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > DMaxSchedulerGroupNumber) || (ret < 0)) {
                    test_loge("[input argument error]: '--fileiowriter-scheduler', number(%d) should between 0 to %d.\n", ret, DMaxSchedulerGroupNumber);
                    return (-11);
                } else {
                    g_mtest_fileio_writer_scheduler_number = ret;

                    test_log("[input argument]: '--fileiowriter-scheduler': (%d).\n", g_mtest_fileio_writer_scheduler_number);
                }
            } else {
                test_loge("[input argument error]: '--fileiowriter-scheduler', should follow with integer(number), argc %d, i %d.\n", argc, i);
            }
            i ++;
        } else if (!strcmp("--videodecoder-scheduler", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > DMaxSchedulerGroupNumber) || (ret < 0)) {
                    test_loge("[input argument error]: '--videodecoder-scheduler', number(%d) should between 0 to %d.\n", ret, DMaxSchedulerGroupNumber);
                    return (-11);
                } else {
                    g_mtest_video_decoder_scheduler_number = ret;

                    test_log("[input argument]: '--videodecoder-scheduler': (%d).\n", g_mtest_video_decoder_scheduler_number);
                }
            } else {
                test_loge("[input argument error]: '--videodecoder-scheduler', should follow with integer(number), argc %d, i %d.\n", argc, i);
            }
            i ++;
        } else if (!strcmp("--udec", argv[i])) {
            g_mtest_dspmode = EMediaDeviceWorkingMode_SingleInstancePlayback;
            test_log("[input argument]: '--udec'\n");
            g_mtest_dsp_dec_instances_number_1 = 1;
            g_mtest_dsp_dec_instances_number_2 = 0;
            g_mtest_dsp_dec_layer = 1;
        } else if (!strcmp("--mudec", argv[i])) {
            g_mtest_dspmode = EMediaDeviceWorkingMode_MultipleInstancePlayback;
            test_log("[input argument]: '--mudec'\n");
        } else if (!strcmp("--mudec-transcode", argv[i])) {
            g_mtest_dspmode = EMediaDeviceWorkingMode_MultipleInstancePlayback_Transcoding;
            test_log("[input argument]: '--mudec-transcode'\n");
        } else if (!strcmp("--genericnvr", argv[i])) {
            g_mtest_dspmode = EMediaDeviceWorkingMode_NVR_RTSP_Streaming;
            test_log("[input argument]: '--genericnvr'\n");
        } else if (!strcmp("--streaming-rtsp", argv[i])) {
            g_mtest_config_rtsp_live_streaming = 1;
            g_mtest_config_rtsp_file_streaming = 1;
        } else if (!strcmp("--savefile", argv[i])) {
            g_mtest_config_save_file = 1;
        } else if (!strcmp("--streaming-hls", argv[i])) {
            g_mtest_config_http_hls_streaming = 1;
        } else if (!strcmp("--multipleaudiodecoder", argv[i])) {
            g_mtest_config_multiple_audio_decoder = 1;
            test_log("[input argument]: '--multipleaudiodecoder'\n");
        } else if (!strcmp("--uniqueaudiodecoder", argv[i])) {
            g_mtest_config_multiple_audio_decoder = 0;
            test_log("[input argument]: '--uniqueaudiodecoder'\n");
        } else if (!strcmp("--help", argv[i])) {
            _mtest_print_version();
            _mtest_print_usage();
            _mtest_print_runtime_usage();
            return (3);
        } else if (!strcmp("--1x1080p", argv[i])) {
            g_mtest_playbackmode = EPlaybackMode_1x1080p;
            test_log("[input argument]: '--1x1080p'\n");
            g_mtest_dsp_dec_instances_number_1 = 1;
            g_mtest_dsp_dec_instances_number_2 = 0;
            g_mtest_dsp_dec_layer = 1;
            //g_current_play_hd = 1;
        } else if (!strcmp("--4+1", argv[i])) {
            g_mtest_playbackmode = EPlaybackMode_1x1080p_4xD1;
            test_log("[input argument]: '--4+1'\n");
            g_mtest_dsp_dec_instances_number_1 = 4;
            g_mtest_dsp_dec_instances_number_2 = 1;
            g_mtest_dsp_dec_layer = 2;
        } else if (!strcmp("--6+1", argv[i])) {
            g_mtest_playbackmode = EPlaybackMode_1x1080p_6xD1;
            test_log("[input argument]: '--6+1'\n");
            g_mtest_dsp_dec_instances_number_1 = 6;
            g_mtest_dsp_dec_instances_number_2 = 1;
            g_mtest_dsp_dec_layer = 2;
        } else if (!strcmp("--8+1", argv[i])) {
            g_mtest_playbackmode = EPlaybackMode_1x1080p_8xD1;
            test_log("[input argument]: '--8+1'\n");
            g_mtest_dsp_dec_instances_number_1 = 8;
            g_mtest_dsp_dec_instances_number_2 = 1;
            g_mtest_dsp_dec_layer = 2;
        } else if (!strcmp("--9+1", argv[i])) {
            g_mtest_playbackmode = EPlaybackMode_1x1080p_9xD1;
            test_log("[input argument]: '--9+1'\n");
            g_mtest_dsp_dec_instances_number_1 = 9;
            g_mtest_dsp_dec_instances_number_2 = 1;
            g_mtest_dsp_dec_layer = 2;
        } else if (!strcmp("--1x3M", argv[i])) {
            g_mtest_playbackmode = EPlaybackMode_1x3M;
            test_log("[input argument]: '--1x3M'\n");
            g_mtest_dsp_dec_instances_number_1 = 1;
            g_mtest_dsp_dec_instances_number_2 = 0;
            g_mtest_dsp_dec_layer = 1;
            //g_current_play_hd = 1;
        } else if (!strcmp("--4+1x3M", argv[i])) {
            g_mtest_playbackmode = EPlaybackMode_1x3M_4xD1;
            test_log("[input argument]: '--4+1x3M'\n");
            g_mtest_dsp_dec_instances_number_1 = 4;
            g_mtest_dsp_dec_instances_number_2 = 1;
            g_mtest_dsp_dec_layer = 2;
        } else if (!strcmp("--6+1x3M", argv[i])) {
            g_mtest_playbackmode = EPlaybackMode_1x3M_6xD1;
            test_log("[input argument]: '--6+1x3M'\n");
            g_mtest_dsp_dec_instances_number_1 = 6;
            g_mtest_dsp_dec_instances_number_2 = 1;
            g_mtest_dsp_dec_layer = 2;
        } else if (!strcmp("--8+1x3M", argv[i])) {
            g_mtest_playbackmode = EPlaybackMode_1x3M_8xD1;
            test_log("[input argument]: '--8+1x3M'\n");
            g_mtest_dsp_dec_instances_number_1 = 8;
            g_mtest_dsp_dec_instances_number_2 = 1;
            g_mtest_dsp_dec_layer = 2;
        } else if (!strcmp("--9+1x3M", argv[i])) {
            g_mtest_playbackmode = EPlaybackMode_1x3M_9xD1;
            test_log("[input argument]: '--9+1x3M'\n");
            g_mtest_dsp_dec_instances_number_1 = 9;
            g_mtest_dsp_dec_instances_number_2 = 1;
            g_mtest_dsp_dec_layer = 2;
        } else if (!strcmp("--4x720p", argv[i])) {
            g_mtest_playbackmode = EPlaybackMode_4x720p;
            test_log("[input argument]: '--4x720p'\n");
            g_mtest_dsp_dec_instances_number_1 = 4;
            g_mtest_dsp_dec_instances_number_2 = 0;
            g_mtest_dsp_dec_layer = 1;
        } else if (!strcmp("--4xD1", argv[i])) {
            g_mtest_playbackmode = EPlaybackMode_4xD1;
            test_log("[input argument]: '--4xD1'\n");
            g_mtest_dsp_dec_instances_number_1 = 4;
            g_mtest_dsp_dec_instances_number_2 = 0;
            g_mtest_dsp_dec_layer = 1;
        } else if (!strcmp("--6xD1", argv[i])) {
            g_mtest_playbackmode = EPlaybackMode_6xD1;
            test_log("[input argument]: '--6xD1'\n");
            g_mtest_dsp_dec_instances_number_1 = 6;
            g_mtest_dsp_dec_instances_number_2 = 0;
            g_mtest_dsp_dec_layer = 1;
        } else if (!strcmp("--8xD1", argv[i])) {
            g_mtest_playbackmode = EPlaybackMode_8xD1;
            test_log("[input argument]: '--8xD1'\n");
            g_mtest_dsp_dec_instances_number_1 = 8;
            g_mtest_dsp_dec_instances_number_2 = 0;
            g_mtest_dsp_dec_layer = 1;
        } else if (!strcmp("--9xD1", argv[i])) {
            g_mtest_playbackmode = EPlaybackMode_9xD1;
            test_log("[input argument]: '--9xD1'\n");
            g_mtest_dsp_dec_instances_number_1 = 9;
            g_mtest_dsp_dec_instances_number_2 = 0;
            g_mtest_dsp_dec_layer = 1;
        } else {
            test_loge("unknown option[%d]: %s\n", i, argv[i]);
            return (-255);
        }
    }

    return 0;
}

TInt mudec_build_process_pipelines()
{
    SGenericPreConfigure pre_config;
    TULong index = 0, group_index = 0, demuxer_start_index = 0;
    TULong total_number = 0, total_number1 = 0;
    TUint shared_sd_channel_number = 0, shared_hd_channel_number = 0;

    TChar rtsp_streaming_tag[64] = {0};

    TGenericID id;

    EECode err = EECode_NotInitilized;
    IGenericEngineControl *p_engine = gfMediaEngineFactory(EFactory_MediaEngineType_Generic);

    test_log("[ut flow]: mudec_build_process_pipelines, start, g_mtest_dsp_dec_instances_number_1 %d, g_mtest_dsp_dec_instances_number_2 %d\n", g_mtest_dsp_dec_instances_number_1, g_mtest_dsp_dec_instances_number_2);

    DASSERT(p_engine);

    if ((!p_engine)) {
        test_loge("gfMediaEngineFactory(%d), return %p\n", EFactory_MediaEngineType_Generic, p_engine);
        return (-3);
    }
#if 0
    if (g_mtest_dsp_dec_instances_number_2) {
        g_media_processor_context.only_hd_has_audio = 1;
    } else {
        g_media_processor_context.only_hd_has_audio = 0;
    }
#endif

    DASSERT(!g_media_processor_context.p_engine);
    g_media_processor_context.p_engine = p_engine;

    DASSERT(g_media_processor_context.enable_rtsp_streaming || g_media_processor_context.enable_playback || g_media_processor_context.enable_mux_file);

    DASSERT(EMediaDeviceWorkingMode_MultipleInstancePlayback == g_mtest_dspmode ||
            EMediaDeviceWorkingMode_MultipleInstancePlayback_Transcoding == g_mtest_dspmode);

    if ((!g_media_processor_context.debug_share_demuxer) && (g_media_processor_context.source_url_count_in_group[0] == 1)) {
        test_logw("[ut config]: only one source in multiple window udec's case, use 1x1080p playback mode\n");
        g_mtest_playbackmode = EPlaybackMode_1x1080p;
    }

    test_log("[ut flow]: udec_build_process_pipelines, before p_engine->SetWorkingMode(%d, %d)\n", g_mtest_dspmode, g_mtest_playbackmode);
    err = p_engine->SetWorkingMode(g_mtest_dspmode, g_mtest_playbackmode);
    DASSERT_OK(err);

    DASSERT(DCAL_BITMASK(EDisplayDevice_HDMI) == g_mtest_voutmask);
    test_log("[ut flow]: udec_build_process_pipelines, before p_engine->SetDisplayDevice(0x%08x)\n", g_mtest_voutmask);
    err = p_engine->SetDisplayDevice(g_mtest_voutmask);
    DASSERT_OK(err);

    if (g_b_mtest_force_log_level) {
        test_log("[ut flow]: udec_build_process_pipelines, before p_engine->SetEngineLogLevel(%d)\n", g_b_mtest_force_log_level);
        err = p_engine->SetEngineLogLevel((ELogLevel)g_mtest_config_log_level);
        DASSERT_OK(err);
    }

    if (g_mtest_net_receiver_scheduler_number) {
        test_log("[ut flow]: udec_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_NetworkReceiverSchedulerNumber, %d)\n", g_mtest_net_receiver_scheduler_number);
        pre_config.check_field = EGenericEnginePreConfigure_NetworkReceiverSchedulerNumber;
        pre_config.number = g_mtest_net_receiver_scheduler_number;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_NetworkReceiverSchedulerNumber, &pre_config);
        DASSERT_OK(err);
    }

    if (g_mtest_net_sender_scheduler_number) {
        test_log("[ut flow]: udec_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_NetworkTransmiterSchedulerNumber, %d)\n", g_mtest_net_sender_scheduler_number);
        pre_config.check_field = EGenericEnginePreConfigure_NetworkTransmiterSchedulerNumber;
        pre_config.number = g_mtest_net_sender_scheduler_number;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_NetworkTransmiterSchedulerNumber, &pre_config);
        DASSERT_OK(err);
    }

    if (g_mtest_fileio_reader_scheduler_number) {
        test_log("[ut flow]: udec_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_IOReaderSchedulerNumber, %d)\n", g_mtest_fileio_reader_scheduler_number);
        pre_config.check_field = EGenericEnginePreConfigure_IOReaderSchedulerNumber;
        pre_config.number = g_mtest_fileio_reader_scheduler_number;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_IOReaderSchedulerNumber, &pre_config);
        DASSERT_OK(err);
    }

    if (g_mtest_fileio_writer_scheduler_number) {
        test_log("[ut flow]: udec_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_IOWriterSchedulerNumber, %d)\n", g_mtest_fileio_writer_scheduler_number);
        pre_config.check_field = EGenericEnginePreConfigure_IOWriterSchedulerNumber;
        pre_config.number = g_mtest_fileio_writer_scheduler_number;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_IOWriterSchedulerNumber, &pre_config);
        DASSERT_OK(err);
    }

    if (g_mtest_video_decoder_scheduler_number) {
        test_log("[ut flow]: udec_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_VideoDecoderSchedulerNumber, %d)\n", g_mtest_video_decoder_scheduler_number);
        pre_config.check_field = EGenericEnginePreConfigure_VideoDecoderSchedulerNumber;
        pre_config.number = g_mtest_video_decoder_scheduler_number;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_VideoDecoderSchedulerNumber, &pre_config);
        DASSERT_OK(err);
    }

    if (gb_mtest_rtsp_client_try_tcp_mode_first) {
        test_log("[ut flow]: generic_nvr_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_RTSPClientTryTCPModeFirst, %d)\n", gb_mtest_rtsp_client_try_tcp_mode_first);
        pre_config.check_field = EGenericEnginePreConfigure_RTSPClientTryTCPModeFirst;
        pre_config.number = gb_mtest_rtsp_client_try_tcp_mode_first;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_RTSPClientTryTCPModeFirst, &pre_config);
        DASSERT_OK(err);
    }

    if (g_media_processor_context.b_set_decoder_prefetch) {
        test_log("[ut flow]: udec_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_PlaybackSetPreFetch, %d)\n", g_media_processor_context.decoder_prefetch);
        pre_config.check_field = EGenericEnginePreConfigure_PlaybackSetPreFetch;
        pre_config.number = g_media_processor_context.decoder_prefetch;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_PlaybackSetPreFetch, &pre_config);
        DASSERT_OK(err);
    }

    if (g_media_processor_context.dsp_total_buffer_number) {
        test_log("[ut flow]: udec_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_PlaybackSetDSPDecoderBufferNumber, %d)\n", g_media_processor_context.dsp_total_buffer_number);
        pre_config.check_field = EGenericEnginePreConfigure_PlaybackSetDSPDecoderBufferNumber;
        pre_config.number = g_media_processor_context.dsp_total_buffer_number;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_PlaybackSetDSPDecoderBufferNumber, &pre_config);
        DASSERT_OK(err);
    }

    if (g_media_processor_context.b_set_dsp_prebuffer) {
        test_log("[ut flow]: udec_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_PlaybackSetDSPPrebuffer, %d)\n", g_media_processor_context.dsp_prebuffer_count);
        pre_config.check_field = EGenericEnginePreConfigure_PlaybackSetDSPPrebuffer;
        pre_config.number = g_media_processor_context.dsp_prebuffer_count;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_PlaybackSetDSPPrebuffer, &pre_config);
        DASSERT_OK(err);
    }

    if (gb_mtest_udec_no_usequpes) {
        test_log("[ut flow]: udec_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_DSPAddUDECWrapper, gb_mtest_udec_no_usequpes %d)\n", gb_mtest_udec_no_usequpes);
        pre_config.check_field = EGenericEnginePreConfigure_DSPAddUDECWrapper;
        pre_config.number = gb_mtest_udec_no_usequpes;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_DSPAddUDECWrapper, &pre_config);
        DASSERT_OK(err);
    }

    if (gb_mtest_udec_not_feed_pts) {
        test_log("[ut flow]: udec_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_DSPFeedPTS, not feed pts %d)\n", gb_mtest_udec_not_feed_pts);
        pre_config.check_field = EGenericEnginePreConfigure_DSPFeedPTS;
        pre_config.number = gb_mtest_udec_not_feed_pts;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_DSPFeedPTS, &pre_config);
        DASSERT_OK(err);
    }

    if (gb_mtest_udec_specify_fps) {
        test_log("[ut flow]: udec_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_DSPSpecifyFPS, pecify fps %d)\n", gb_mtest_udec_specify_fps);
        pre_config.check_field = EGenericEnginePreConfigure_DSPSpecifyFPS;
        pre_config.number = gb_mtest_udec_specify_fps;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_DSPSpecifyFPS, &pre_config);
        DASSERT_OK(err);
    }

    if (g_mtest_rtsp_server_port) {
        test_log("[ut flow]: udec_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_RTSPStreamingServerPort, %d)\n", g_mtest_rtsp_server_port);
        pre_config.check_field = EGenericEnginePreConfigure_RTSPStreamingServerPort;
        pre_config.number = g_mtest_rtsp_server_port;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_RTSPStreamingServerPort, &pre_config);
        DASSERT_OK(err);
    }

    if (g_media_processor_context.streaming_timeout_threashold) {
        test_log("[ut flow]: udec_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_SetStreamingTimeoutThreashold, %d)\n", g_media_processor_context.streaming_timeout_threashold);
        pre_config.check_field = EGenericEnginePreConfigure_SetStreamingTimeoutThreashold;
        pre_config.number = g_media_processor_context.streaming_timeout_threashold;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_SetStreamingTimeoutThreashold, &pre_config);
        DASSERT_OK(err);
    }

    if (g_media_processor_context.streaming_autoretry_connect_server_interval) {
        test_log("[ut flow]: udec_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_StreamingRetryConnectServerInterval, %d)\n", g_media_processor_context.streaming_autoretry_connect_server_interval);
        pre_config.check_field = EGenericEnginePreConfigure_StreamingRetryConnectServerInterval;
        pre_config.number = g_media_processor_context.streaming_autoretry_connect_server_interval;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_StreamingRetryConnectServerInterval, &pre_config);
        DASSERT_OK(err);
    }

    if (gb_mtest_compensate_delay_from_jitter) {
        test_log("[ut flow]: udec_build_process_pipelines, before p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_CompensateDelayFromJitter)\n");
        err = p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_CompensateDelayFromJitter, NULL);
        DASSERT_OK(err);
    }

    if (gb_mtest_prefer_ffmpeg_muxer) {
        test_log("[ut flow]: udec_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_Muxer_FFMpeg)\n");
        pre_config.check_field = EGenericEnginePreConfigure_PreferModule_Muxer_FFMpeg;
        pre_config.number = 0;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_Muxer_FFMpeg, &pre_config);
        DASSERT_OK(err);
    }

    if (gb_mtest_prefer_private_ts_muxer) {
        test_log("[ut flow]: udec_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_Muxer_TS)\n");
        pre_config.check_field = EGenericEnginePreConfigure_PreferModule_Muxer_TS;
        pre_config.number = 0;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_Muxer_TS, &pre_config);
        DASSERT_OK(err);
    }

    if (gb_mtest_prefer_ffmpeg_audio_decoder) {
        test_log("[ut flow]: udec_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioDecoder_FFMpeg)\n");
        pre_config.check_field = EGenericEnginePreConfigure_PreferModule_AudioDecoder_FFMpeg;
        pre_config.number = 0;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioDecoder_FFMpeg, &pre_config);
        DASSERT_OK(err);
    }

    if (gb_mtest_prefer_private_aac_decoder) {
        test_log("[ut flow]: udec_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioDecoder_AAC)\n");
        pre_config.check_field = EGenericEnginePreConfigure_PreferModule_AudioDecoder_AAC;
        pre_config.number = 0;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioDecoder_AAC, &pre_config);
        DASSERT_OK(err);
    }

    test_log("[ut flow]: mudec_build_process_pipelines, before p_engine->BeginConfigProcessPipeline(1)\n");
    err = p_engine->BeginConfigProcessPipeline(1);
    DASSERT_OK(err);

    if (g_media_processor_context.debug_share_demuxer) {
        shared_sd_channel_number = g_mtest_dsp_dec_instances_number_1;
        if (g_mtest_dsp_dec_instances_number_2) {
            shared_hd_channel_number = g_mtest_dsp_dec_instances_number_1;
        }
        g_media_processor_context.mnCloudReceiverCamera = 1;
    } else {
        g_media_processor_context.mnCloudReceiverCamera = g_mtest_dsp_dec_instances_number_1;
    }
    g_media_processor_context.mnCloudReceiverAPP = 1;

    //build customized process pipeline

    //streaming server
    if (g_media_processor_context.enable_rtsp_streaming) {

        err = p_engine->NewComponent(EGenericComponentType_StreamingServer, id);
        DASSERT_OK(err);
        if (EECode_OK == err) {
            test_log("[ut flow]: mudec_build_process_pipelines, new rtsp streaming server success, id %08x\n", id);
            g_media_processor_context.rtspStreamingServerID = id;
        } else {
            test_loge("NewComponent fail, index %lu, ret %d.\n", index, err);
            return (-4);
        }

        err = p_engine->NewComponent(EGenericComponentType_StreamingTransmiter, id);
        DASSERT_OK(err);
        if (EECode_OK == err) {
            test_log("[ut flow]: mudec_build_process_pipelines, new rtsp streaming transmiter success, id %08x\n", id);
            g_media_processor_context.rtspStreamingTransmiterID = id;
        } else {
            test_loge("NewComponent fail, index %lu, ret %d.\n", index, err);
            return (-4);
        }

    }

    //cloud server
    if (g_media_processor_context.enable_cloud_server_nvr) {
        err = p_engine->NewComponent(EGenericComponentType_CloudServerNVR, id);
        DASSERT_OK(err);
        if (EECode_OK == err) {
            test_log("[ut flow]: mudec_build_process_pipelines, new cloud server nvr success, id %08x\n", id);
            g_media_processor_context.cloudNVRServerID = id;
        } else {
            test_loge("NewComponent fail, index %lu, ret %d.\n", index, err);
            return (-4);
        }
    }

    //demuxer
    if (DLikely(!g_media_processor_context.debug_share_demuxer)) {
        for (group_index = 0; group_index < d_max_source_url_group_number; group_index++) {
            if (g_media_processor_context.source_url_count_in_group[group_index]) {
                total_number = g_media_processor_context.source_url_count_in_group[group_index];
                test_log("[ut flow]: mudec_build_process_pipelines, before create source component, group %lu, request number %lu\n", group_index, total_number);
                for (index = 0; index < total_number; index ++) {
                    err = p_engine->NewComponent(EGenericComponentType_Demuxer, id);
                    DASSERT_OK(err);
                    if (EECode_OK == err) {
                        test_log("[ut flow]: mudec_build_process_pipelines, new demuxer success, id %08x\n", id);
                        g_media_processor_context.demuxerID[g_media_processor_context.mnDemuxerNumber + index] = id;
                    } else {
                        test_loge("NewComponent fail, index %lu, ret %d.\n", g_media_processor_context.mnDemuxerNumber + index, err);
                        return (-4);
                    }
                }
                g_media_processor_context.mnDemuxerNumber += total_number;
            }
        }
        if (g_media_processor_context.enable_audio_pin_mux) {
            test_log("[ut flow]: mudec_build_process_pipelines, before create pin muxer for bi-directional audio\n");
            err = p_engine->NewComponent(EGenericComponentType_ConnecterPinMuxer, id);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                test_log("[ut flow]: mudec_build_process_pipelines, new pin muxer success, id %08x\n", id);
                g_media_processor_context.audioPinMuxerID[0] = id;
            } else {
                test_loge("NewComponent fail, index 0, ret %d.\n", err);
                return (-4);
            }
        }
    } else {
        total_number = 0;
        if (shared_sd_channel_number) {
            err = p_engine->NewComponent(EGenericComponentType_Demuxer, id);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                test_log("[ut flow]: mudec_build_process_pipelines, new demuxer success(shared 0), id %08x\n", id);
                g_media_processor_context.demuxerID[0] = id;
            } else {
                test_loge("NewComponent fail, shared demuxer 0, ret %d.\n", err);
                return (-4);
            }
            total_number ++;
        }

        if (shared_hd_channel_number) {
            err = p_engine->NewComponent(EGenericComponentType_Demuxer, id);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                test_log("[ut flow]: mudec_build_process_pipelines, new demuxer success(shared 1), id %08x\n", id);
                g_media_processor_context.demuxerID[1] = id;
            } else {
                test_loge("NewComponent fail, shared demuxer, ret %d.\n", err);
                return (-4);
            }
            total_number ++;
        }

        if (!total_number) {
            test_loge("0 total_number, shared_sd_channel_number %d, shared_hd_channel_number %d\n", shared_sd_channel_number, shared_hd_channel_number);
            return (-50);
        }

        if (g_media_processor_context.enable_audio_pin_mux) {
            test_log("[ut flow]: mudec_build_process_pipelines, before create pin muxer for bi-directional audio\n");
            err = p_engine->NewComponent(EGenericComponentType_ConnecterPinMuxer, id);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                test_log("[ut flow]: mudec_build_process_pipelines, new pin muxer success, id %08x\n", id);
                g_media_processor_context.audioPinMuxerID[0] = id;
            } else {
                test_loge("NewComponent fail, index 0, ret %d.\n", err);
                return (-4);
            }
        }
    }

    if (g_media_processor_context.enable_cloud_server_nvr) {

        //receiver from APP
        if (g_media_processor_context.mnCloudReceiverAPP) {
            total_number = g_media_processor_context.mnCloudReceiverAPP;
            for (index = 0; index < total_number; index ++) {
                err = p_engine->NewComponent(EGenericComponentType_Demuxer, id);
                DASSERT_OK(err);
                if (EECode_OK == err) {
                    test_log("[ut flow]: mudec_build_process_pipelines, new cloud connecter(receiver from APP) success, id %08x\n", id);
                    g_media_processor_context.cloudClientReceiverAPPID[index] = id;
                } else {
                    test_loge("NewComponent fail, index %lu, ret %d.\n", index, err);
                    return (-4);
                }

                err = p_engine->NewComponent(EGenericComponentType_CloudReceiverContent, id);
                DASSERT_OK(err);
                if (EECode_OK == err) {
                    test_log("[ut flow]: mudec_build_process_pipelines, new content(receiver from APP) success, id %08x\n", id);
                    g_media_processor_context.cloudClientReceiverAPPContentID[index] = id;
                } else {
                    test_loge("NewComponent fail, index %lu, ret %d.\n", index, err);
                    return (-4);
                }
            }
        }

        //receiver from Camera
        if (g_media_processor_context.mnCloudReceiverCamera) {
            total_number = g_media_processor_context.mnCloudReceiverCamera;
            for (index = 0; index < total_number; index ++) {
                err = p_engine->NewComponent(EGenericComponentType_Demuxer, id);
                DASSERT_OK(err);
                if (EECode_OK == err) {
                    test_log("[ut flow]: mudec_build_process_pipelines, new cloud connecter(receiver from Camera) success, id %08x\n", id);
                    g_media_processor_context.cloudClientReceiverCameraID[index] = id;
                } else {
                    test_loge("NewComponent fail, index %lu, ret %d.\n", index, err);
                    return (-4);
                }

                err = p_engine->NewComponent(EGenericComponentType_CloudReceiverContent, id);
                DASSERT_OK(err);
                if (EECode_OK == err) {
                    test_log("[ut flow]: mudec_build_process_pipelines, new content(receiver from Camera) success, id %08x\n", id);
                    g_media_processor_context.cloudClientReceiverCameraContentID[index] = id;
                } else {
                    test_loge("NewComponent fail, index %lu, ret %d.\n", index, err);
                    return (-4);
                }
            }
        }
    }

    //muxer
    if (DLikely(!g_media_processor_context.debug_share_demuxer)) {
        if (g_media_processor_context.enable_mux_file) {
            //muxer
            if (g_mtest_save_all_channel) {
                total_number = g_media_processor_context.mnDemuxerNumber;
            } else {
                total_number = g_media_processor_context.sink_url_count_in_group[g_cur_recording_url_group];
            }
            test_log("[ut flow]: mudec_build_process_pipelines, before create sink component, group 0, request number %lu\n", total_number);
            for (index = 0; index < total_number; index ++) {
                err = p_engine->NewComponent(EGenericComponentType_Muxer, id);
                DASSERT_OK(err);
                if (EECode_OK == err) {
                    test_log("[ut flow]: mudec_build_process_pipelines, new muxer success, id %08x\n", id);
                    g_media_processor_context.muxerID[index] = id;
                } else {
                    test_loge("NewComponent fail, index %lu, ret %d.\n", index, err);
                    return (-4);
                }
            }
            g_media_processor_context.mnMuxerNumber = total_number;
        }
    } else {
        if (g_media_processor_context.enable_mux_file) {
            //muxer
            if (g_mtest_save_all_channel) {
                total_number = shared_sd_channel_number + shared_hd_channel_number;
            } else {
                total_number = g_media_processor_context.sink_url_count_in_group[g_cur_recording_url_group];
            }
            test_log("[ut flow]: mudec_build_process_pipelines, before create sink component, group 0, request number %lu\n", total_number);
            for (index = 0; index < total_number; index ++) {
                err = p_engine->NewComponent(EGenericComponentType_Muxer, id);
                DASSERT_OK(err);
                if (EECode_OK == err) {
                    test_log("[ut flow]: mudec_build_process_pipelines, new muxer success, id %08x\n", id);
                    g_media_processor_context.muxerID[index] = id;
                } else {
                    test_loge("NewComponent fail, index %lu, ret %d.\n", index, err);
                    return (-4);
                }
            }
            g_media_processor_context.mnMuxerNumber = total_number;
        }
    }

    if (g_media_processor_context.enable_transcoding) {
        test_log("[ut flow]: mudec_build_process_pipelines, before create video encoder component\n");
        err = p_engine->NewComponent(EGenericComponentType_VideoEncoder, id);
        DASSERT_OK(err);
        if (EECode_OK == err) {
            test_log("[ut flow]: mudec_build_process_pipelines, new video encoder success, id %08x\n", id);
            g_media_processor_context.videoEncoderID[0] = id;
        } else {
            test_loge("NewComponent video encoder fail, index %d, ret %d.\n", 0, err);
            return (-10);
        }
    }

    if (g_media_processor_context.enable_playback) {
        if (g_media_processor_context.enable_video) {
            //video decoder
            //total_number = g_media_processor_context.source_sd_url_count_in_group[g_cur_playback_url_group];
            total_number = g_mtest_dsp_dec_instances_number_1;
            test_log("[ut flow]: mudec_build_process_pipelines, before create video decoder component, group 0, request number %ld (sd), source number %ld\n", total_number, g_media_processor_context.source_sd_url_count_in_group[g_cur_playback_url_group]);
            for (index = 0; index < total_number; index ++) {
                err = p_engine->NewComponent(EGenericComponentType_VideoDecoder, id);
                DASSERT_OK(err);
                if (EECode_OK == err) {
                    test_log("[ut flow]: mudec_build_process_pipelines, new video decoder success, sd, id %08x, index %ld\n", id, index);
                    g_media_processor_context.videoDecoderID[index] = id;
                } else {
                    test_loge("NewComponent fail, index %lu, ret %d.\n", index, err);
                    return (-5);
                }
            }
            g_media_processor_context.mnVideoDecoderNumber = total_number;

            total_number = g_media_processor_context.source_hd_url_count_in_group[g_cur_playback_url_group];
            test_log("[ut flow]: mudec_build_process_pipelines, before create video decoder component, group 0, request number %d (hd), source number %ld\n", g_mtest_dsp_dec_instances_number_2, total_number);

            for (index = g_mtest_dsp_dec_instances_number_1; index < (g_mtest_dsp_dec_instances_number_2 + g_mtest_dsp_dec_instances_number_1); index ++) {
                err = p_engine->NewComponent(EGenericComponentType_VideoDecoder, id);
                DASSERT_OK(err);
                if (EECode_OK == err) {
                    test_log("[ut flow]: mudec_build_process_pipelines, new video decoder success, hd, id %08x, index %ld\n", id, index);
                    g_media_processor_context.videoDecoderID[index] = id;
                } else {
                    test_loge("NewComponent fail, index %lu, ret %d.\n", index, err);
                    return (-5);
                }
            }

            g_media_processor_context.mnVideoDecoderNumber += g_mtest_dsp_dec_instances_number_2;
        }

        if (g_media_processor_context.enable_audio) {
            //audio decoder
            if (g_mtest_config_multiple_audio_decoder) {
                test_log("[ut flow]: mudec_build_process_pipelines, before create audio decoder component, group 0, request number %lu\n", total_number);
                for (index = 0; index < total_number; index ++) {
                    err = p_engine->NewComponent(EGenericComponentType_AudioDecoder, id);
                    DASSERT_OK(err);
                    if (EECode_OK == err) {
                        test_log("[ut flow]: mudec_build_process_pipelines, new audio decoder success, id %08x\n", id);
                        g_media_processor_context.audioDecoderID[index] = id;
                    } else {
                        test_loge("NewComponent fail, index %lu, ret %d.\n", index, err);
                        return (-6);
                    }
                }
                g_media_processor_context.mnAudioDecoderNumber = total_number;
            } else {
                test_log("[ut flow]: mudec_build_process_pipelines, before create unique audio decoder component, group 0\n");
                err = p_engine->NewComponent(EGenericComponentType_AudioDecoder, id);
                DASSERT_OK(err);
                if (EECode_OK == err) {
                    test_log("[ut flow]: mudec_build_process_pipelines, new audio decoder success, id %08x\n", id);
                    g_media_processor_context.audioDecoderID[0] = id;
                } else {
                    test_loge("NewComponent fail, index 0, ret %d.\n", err);
                    return (-6);
                }
                g_media_processor_context.mnAudioDecoderNumber = 1;
            }
        }

        if (g_media_processor_context.enable_video) {
            //video renderer
            test_log("[ut flow]: mudec_build_process_pipelines, before create video renderer component, group 0, request number 1\n");
            err = p_engine->NewComponent(EGenericComponentType_VideoRenderer, id);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                test_log("[ut flow]: mudec_build_process_pipelines, new video renderer success, id %08x\n", id);
                g_media_processor_context.videoRendererID[0] = id;
            } else {
                test_loge("NewComponent fail, index %d, ret %d.\n", 0, err);
                return (-9);
            }
            g_media_processor_context.mnVideoRendererNumber = 1;
        }

        if (g_media_processor_context.enable_audio) {
            //audio renderer
            test_log("[ut flow]: mudec_build_process_pipelines, before create audio renderer component, group 0, request number 1\n");
            err = p_engine->NewComponent(EGenericComponentType_AudioRenderer, id);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                test_log("[ut flow]: mudec_build_process_pipelines, new demuxer success, id %08x\n", id);
                g_media_processor_context.audioRendererID[0] = id;
            } else {
                test_loge("NewComponent fail, index %d, ret %d.\n", 0, err);
                return (-10);
            }
            g_media_processor_context.mnAudioRendererNumber = 1;
        }
    }

    //streaming content
    if (g_media_processor_context.rtsp_streaming_from_playback_source) {
        total_number = g_media_processor_context.source_url_count_in_group[0];
        g_media_processor_context.mnRTSPStreamingFromPlaybackSource = total_number;
    }

    //connect components

    //demuxer to video decoder
    if (g_media_processor_context.enable_playback) {

        if (g_media_processor_context.enable_video) {
            //DASSERT(g_media_processor_context.mnDemuxerNumber == g_media_processor_context.mnVideoDecoderNumber);
            //total_number = g_media_processor_context.mnDemuxerNumber;
            TULong video_dec_index = 0;
            total_number = g_mtest_dsp_dec_instances_number_1;

            if (DLikely(!g_media_processor_context.debug_share_demuxer)) {
                for (index = 0; index < total_number; index ++) {
                    err = p_engine->ConnectComponent(id, \
                                                     g_media_processor_context.demuxerID[index], \
                                                     g_media_processor_context.videoDecoderID[index], \
                                                     StreamType_Video);
                    DASSERT_OK(err);
                    if (EECode_OK == err) {
                        test_log("[ut flow]: mudec_build_process_pipelines, connect demuxer(%lu) to video decoder(%lu) success, id %08x\n", index, index, id);
                        g_media_processor_context.connections_demuxer_2_video_decoder[index] = id;
                    } else {
                        test_loge("ConnectComponent(), demuxer(%lu) to video decoder(%lu) fail, ret %d.\n", index, index, err);
                        return (-11);
                    }
                }
            } else {
                DASSERT(shared_sd_channel_number == g_mtest_dsp_dec_instances_number_1);
                for (index = 0; index < shared_sd_channel_number; index ++) {
                    err = p_engine->ConnectComponent(id, \
                                                     g_media_processor_context.demuxerID[0], \
                                                     g_media_processor_context.videoDecoderID[index], \
                                                     StreamType_Video);
                    DASSERT_OK(err);
                    if (EECode_OK == err) {
                        test_log("[ut flow]: mudec_build_process_pipelines, connect shared demuxer(0) to video decoder(%lu) success, id %08x\n",  index, id);
                        g_media_processor_context.connections_demuxer_2_video_decoder[index] = id;
                    } else {
                        test_loge("ConnectComponent(), shared demuxer(0) to video decoder(%lu) fail, ret %d.\n", index, err);
                        return (-11);
                    }
                }
            }

            if (DLikely(!g_media_processor_context.debug_share_demuxer)) {
                //total_number = g_media_processor_context.mnDemuxerNumber;
                TULong group_source_number = g_media_processor_context.source_url_count_in_group[g_cur_playback_url_group];
                video_dec_index = g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2 - 1;
                DASSERT(video_dec_index < 100);

                for (; index < group_source_number; index ++) {
                    test_log("[ut flow]: mudec_build_process_pipelines, before connect demuxer(%lu, id 0x%08x) to video decoder(%lu, id 0x%08x)\n", index, g_media_processor_context.demuxerID[index], video_dec_index, g_media_processor_context.videoDecoderID[video_dec_index]);
                    err = p_engine->ConnectComponent(id, \
                                                     g_media_processor_context.demuxerID[index], \
                                                     g_media_processor_context.videoDecoderID[video_dec_index], \
                                                     StreamType_Video);
                    DASSERT_OK(err);
                    if (EECode_OK == err) {
                        test_log("[ut flow]: mudec_build_process_pipelines, connect demuxer(%lu) to video decoder(%lu) success, id %08x\n", index, video_dec_index, id);
                        g_media_processor_context.connections_demuxer_2_video_decoder[video_dec_index] = id;
                    } else {
                        test_loge("ConnectComponent(), demuxer(%lu) to video decoder(%lu) fail, ret %d.\n", index, video_dec_index, err);
                        return (-11);
                    }
                }
            } else {
                if (shared_hd_channel_number) {
                    total_number = shared_hd_channel_number;
                    video_dec_index = g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2 - 1;
                    DASSERT(video_dec_index < 100);

                    for (index = 0; index < total_number; index ++) {
                        test_log("[ut flow]: mudec_build_process_pipelines, before connect shared demuxer(1, id 0x%08x) to video decoder(%lu, id 0x%08x)\n", g_media_processor_context.demuxerID[1], video_dec_index, g_media_processor_context.videoDecoderID[video_dec_index]);
                        err = p_engine->ConnectComponent(id, \
                                                         g_media_processor_context.demuxerID[1], \
                                                         g_media_processor_context.videoDecoderID[video_dec_index], \
                                                         StreamType_Video);
                        DASSERT_OK(err);
                        if (EECode_OK == err) {
                            test_log("[ut flow]: mudec_build_process_pipelines, connect shared demuxer(1) to video decoder(%lu) success, id %08x\n", video_dec_index, id);
                            g_media_processor_context.connections_demuxer_2_video_decoder[video_dec_index] = id;
                        } else {
                            test_loge("ConnectComponent(), shared demuxer(1) to video decoder(%lu) fail, ret %d.\n", video_dec_index, err);
                            return (-11);
                        }
                    }
                }
            }
            TULong pre_groups_source_number = 0;
            for (group_index = 1; group_index < d_max_source_url_group_number; group_index++) {
                if (g_media_processor_context.source_url_count_in_group[group_index]) {
                    pre_groups_source_number += g_media_processor_context.source_url_count_in_group[group_index - 1];
                    total_number = g_mtest_dsp_dec_instances_number_1;
                    video_dec_index = g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2 - 1;
                    for (index = 0; index < total_number; index ++) {
                        err = p_engine->ConnectComponent(id, \
                                                         g_media_processor_context.demuxerID[pre_groups_source_number + index], \
                                                         g_media_processor_context.videoDecoderID[index], \
                                                         StreamType_Video);
                        DASSERT_OK(err);
                        if (EECode_OK == err) {
                            test_log("[ut flow]: mudec_build_process_pipelines, vod test, connect demuxer(%lu) to video decoder(%lu) success, id %08x\n", pre_groups_source_number + index, index, id);
                            g_media_processor_context.connections_demuxer_2_video_decoder[g_mtest_dsp_dec_instances_number_1 + 1 + index] = id;
                        } else {
                            test_loge("vod test, ConnectComponent(), demuxer(%lu) to video decoder(%lu) fail, ret %d.\n", pre_groups_source_number + index, index, err);
                            return (-11);
                        }
                    }
                    total_number = pre_groups_source_number + g_media_processor_context.source_url_count_in_group[group_index];
                    for (index = pre_groups_source_number + g_mtest_dsp_dec_instances_number_1; index < total_number; index ++) {
                        err = p_engine->ConnectComponent(id, \
                                                         g_media_processor_context.demuxerID[index], \
                                                         g_media_processor_context.videoDecoderID[video_dec_index], \
                                                         StreamType_Video);
                        DASSERT_OK(err);
                        if (EECode_OK == err) {
                            test_log("[ut flow]: mudec_build_process_pipelines, vod test connect demuxer(%lu) to video decoder(%lu) success, id %08x\n", index, video_dec_index, id);
                            g_media_processor_context.connections_demuxer_2_video_decoder[g_mtest_dsp_dec_instances_number_1 * 2 + 1] = id;
                        } else {
                            test_loge("vod test, ConnectComponent(), demuxer(%lu) to video decoder(%lu) fail, ret %d.\n", index, video_dec_index, err);
                            return (-11);
                        }
                    }
                }
            }

        }

        if (g_media_processor_context.enable_audio) {
            //demuxer to audio decoder
            if (DLikely(!g_media_processor_context.debug_share_demuxer)) {
                total_number = g_media_processor_context.mnDemuxerNumber;

                if (g_media_processor_context.enable_audio_pin_mux) {

                    if (g_mtest_config_multiple_audio_decoder) {
                        g_mtest_config_multiple_audio_decoder = 0;
                        test_loge("g_mtest_config_multiple_audio_decoder is set when audio pin muxer is also enabled?\n");
                    }

                    if (g_media_processor_context.only_hd_has_audio && g_mtest_dsp_dec_instances_number_2) {
                        index = g_mtest_dsp_dec_instances_number_1;
                    } else {
                        index = 0;
                    }

                    for (; index < total_number; index ++) {
                        err = p_engine->ConnectComponent(id, \
                                                         g_media_processor_context.demuxerID[index], \
                                                         g_media_processor_context.audioPinMuxerID[0], \
                                                         StreamType_Audio);
                        DASSERT_OK(err);
                        if (EECode_OK == err) {
                            test_log("[ut flow]: mudec_build_process_pipelines, connect demuxer(%lu) to audio pin muxer(0) success, id %08x\n", index, id);
                            g_media_processor_context.connections_demuxer_2_audio_pinmuxer[index] = id;
                        } else {
                            test_loge("ConnectComponent(), demuxer(%lu) to audio pin muxer(0) fail, ret %d.\n", index, err);
                            return (-12);
                        }
                    }

                    err = p_engine->ConnectComponent(id, \
                                                     g_media_processor_context.audioPinMuxerID[0], \
                                                     g_media_processor_context.audioDecoderID[0], \
                                                     StreamType_Audio);
                    DASSERT_OK(err);
                    if (EECode_OK == err) {
                        test_log("[ut flow]: mudec_build_process_pipelines, connect audio pin muxer(0) to audio decoder success, id %08x\n", id);
                        g_media_processor_context.connections_audio_pinmuxer_2_audio_decoder[0] = id;
                    } else {
                        test_loge("ConnectComponent(), audio pin muxer(0) to audio decoder fail, ret %d.\n", err);
                        return (-12);
                    }
                } else {

                    if (g_mtest_config_multiple_audio_decoder) {
                        test_log("[ut flow]: mudec_build_process_pipelines, multiple audio decoder!\n");
                        for (index = 0; index < total_number; index ++) {
                            err = p_engine->ConnectComponent(id, \
                                                             g_media_processor_context.demuxerID[index], \
                                                             g_media_processor_context.audioDecoderID[index], \
                                                             StreamType_Audio);
                            DASSERT_OK(err);
                            if (EECode_OK == err) {
                                test_log("[ut flow]: mudec_build_process_pipelines, connect demuxer(%lu) to audio decoder(%lu) success, id %08x\n", index, index, id);
                                g_media_processor_context.connections_demuxer_2_audio_decoder[index] = id;
                            } else {
                                test_loge("ConnectComponent(), demuxer(%lu) to audio decoder(%lu) fail, ret %d.\n", index, index, err);
                                return (-12);
                            }
                        }
                    } else {
                        if (g_media_processor_context.only_hd_has_audio) {
                            test_log("[ut flow]: mudec_build_process_pipelines, only hd has audio!\n");
                        } else {
                            test_log("[ut flow]: mudec_build_process_pipelines, all sd/hd have audio!\n");
                            for (index = 0; index < total_number; index ++) {
                                err = p_engine->ConnectComponent(id, \
                                                                 g_media_processor_context.demuxerID[index], \
                                                                 g_media_processor_context.audioDecoderID[0], \
                                                                 StreamType_Audio);
                                DASSERT_OK(err);
                                if (EECode_OK == err) {
                                    test_log("[ut flow]: mudec_build_process_pipelines, connect demuxer(%lu) to audio decoder(%lu) success, id %08x\n", index, index, id);
                                    g_media_processor_context.connections_demuxer_2_audio_decoder[index] = id;
                                } else {
                                    test_loge("ConnectComponent(), demuxer(%lu) to audio decoder(%lu) fail, ret %d.\n", index, index, err);
                                    return (-12);
                                }
                            }
                        }
                    }
                }
            } else {
                TUint audio_demuxer_index = 0;
                if (shared_hd_channel_number) {
                    total_number = shared_hd_channel_number;
                    audio_demuxer_index = shared_sd_channel_number;
                } else {
                    total_number = shared_sd_channel_number + shared_hd_channel_number;
                    audio_demuxer_index = 0;
                }
                DASSERT(total_number);

                if (g_media_processor_context.enable_audio_pin_mux) {

                    if (g_mtest_config_multiple_audio_decoder) {
                        g_mtest_config_multiple_audio_decoder = 0;
                        test_loge("g_mtest_config_multiple_audio_decoder is set when audio pin muxer is also enabled?\n");
                    }

                    total_number = 1;//hard code here
                    for (index = 0; index < total_number; index ++) {

                        if ((!g_media_processor_context.only_hd_has_audio) || (g_mtest_dsp_dec_instances_number_1 <= index)) {
                            err = p_engine->ConnectComponent(id, \
                                                             g_media_processor_context.demuxerID[0], \
                                                             g_media_processor_context.audioPinMuxerID[0], \
                                                             StreamType_Audio);
                            DASSERT_OK(err);
                            if (EECode_OK == err) {
                                test_log("[ut flow]: mudec_build_process_pipelines, connect shared demuxer(%d) to audio pin mux success, id %08x\n", 0, id);
                                g_media_processor_context.connections_demuxer_2_audio_pinmuxer[index] = id;
                            } else {
                                test_loge("ConnectComponent(), shared demuxer(%d) to audio pin muxer fail, pin muxer %08x, ret %d.\n", 0, g_media_processor_context.audioPinMuxerID[0], err);
                                return (-12);
                            }
                        } else {
                            test_loge("g_mtest_dsp_dec_instances_number_1 %d _2 %d.\n", g_mtest_dsp_dec_instances_number_1, g_mtest_dsp_dec_instances_number_2);
                        }

                    }

                    err = p_engine->ConnectComponent(id, \
                                                     g_media_processor_context.audioPinMuxerID[0], \
                                                     g_media_processor_context.audioDecoderID[0], \
                                                     StreamType_Audio);
                    DASSERT_OK(err);
                    if (EECode_OK == err) {
                        test_log("[ut flow]: mudec_build_process_pipelines, connect audio pin muxer(0) to audio decoder success, id %08x\n", id);
                        g_media_processor_context.connections_audio_pinmuxer_2_audio_decoder[0] = id;
                    } else {
                        test_loge("ConnectComponent(), audio pin muxer(0) to audio decoder fail, ret %d.\n", err);
                        return (-12);
                    }
                } else {

                    for (index = 0; index < total_number; index ++) {
                        if (g_mtest_config_multiple_audio_decoder) {
                            err = p_engine->ConnectComponent(id, \
                                                             g_media_processor_context.demuxerID[audio_demuxer_index], \
                                                             g_media_processor_context.audioDecoderID[index], \
                                                             StreamType_Audio);
                            DASSERT_OK(err);
                            if (EECode_OK == err) {
                                test_log("[ut flow]: mudec_build_process_pipelines, connect shared demuxer(%d) to audio decoder(%lu) success, id %08x\n", audio_demuxer_index, index, id);
                                g_media_processor_context.connections_demuxer_2_audio_decoder[index] = id;
                            } else {
                                test_loge("ConnectComponent(), shared demuxer(%d) to audio decoder(%lu) fail, ret %d.\n", audio_demuxer_index, index, err);
                                return (-12);
                            }
                        } else {
                            if ((!g_media_processor_context.only_hd_has_audio) || (g_mtest_dsp_dec_instances_number_1 <= index)) {
                                err = p_engine->ConnectComponent(id, \
                                                                 g_media_processor_context.demuxerID[audio_demuxer_index], \
                                                                 g_media_processor_context.audioDecoderID[0], \
                                                                 StreamType_Audio);
                                DASSERT_OK(err);
                                if (EECode_OK == err) {
                                    test_log("[ut flow]: mudec_build_process_pipelines, connect shared demuxer(%d) to unique audio decoder success, id %08x\n", audio_demuxer_index, id);
                                    g_media_processor_context.connections_demuxer_2_audio_decoder[index] = id;
                                } else {
                                    test_loge("ConnectComponent(), shared demuxer(%d) to unique audio decoder fail, ret %d.\n", audio_demuxer_index, err);
                                    return (-12);
                                }
                            } else {
                                test_loge("g_mtest_dsp_dec_instances_number_1 %d _2 %d.\n", g_mtest_dsp_dec_instances_number_1, g_mtest_dsp_dec_instances_number_2);
                            }
                        }
                    }
                }

            }
        }

    }

    if (g_media_processor_context.enable_mux_file) {
        //demuxer to muxer
        if (g_media_processor_context.mnMuxerNumber) {
            DASSERT(g_media_processor_context.mnDemuxerNumber == g_media_processor_context.mnMuxerNumber);

            if (DLikely(!g_media_processor_context.debug_share_demuxer)) {

                total_number = g_media_processor_context.mnMuxerNumber;

                for (index = 0; index < total_number; index ++) {
                    if (g_media_processor_context.enable_video) {
                        err = p_engine->ConnectComponent(id, \
                                                         g_media_processor_context.demuxerID[index], \
                                                         g_media_processor_context.muxerID[index], \
                                                         StreamType_Video);
                        DASSERT_OK(err);
                        if (EECode_OK == err) {
                            test_log("[ut flow]: mudec_build_process_pipelines, connect demuxer(%lu) to muxer(%lu) success, id %08x, video path\n", index, index, id);
                            g_media_processor_context.connections_demuxer_2_muxer[index * 2] = id;
                        } else {
                            test_loge("ConnectComponent(), demuxer(%lu) to muxer(%lu) fail, video path, ret %d.\n", index, index, err);
                            return (-13);
                        }
                    }

                    if (g_media_processor_context.enable_audio) {
                        if ((!g_media_processor_context.only_hd_has_audio) || (g_mtest_dsp_dec_instances_number_1 <= index)) {
                            err = p_engine->ConnectComponent(id, \
                                                             g_media_processor_context.demuxerID[index], \
                                                             g_media_processor_context.muxerID[index], \
                                                             StreamType_Audio);
                            DASSERT_OK(err);
                            if (EECode_OK == err) {
                                test_log("[ut flow]: mudec_build_process_pipelines, connect demuxer(%lu) to muxer(%lu) success, id %08x, audio path\n", index, index, id);
                                g_media_processor_context.connections_demuxer_2_muxer[index * 2 + 1] = id;
                            } else {
                                test_loge("ConnectComponent(), demuxer(%lu) to muxer(%lu) fail, audio path, ret %d.\n", index, index, err);
                                return (-13);
                            }
                        }
                    }
                }
            } else {

                total_number = shared_sd_channel_number;

                for (index = 0; index < total_number; index ++) {
                    if (g_media_processor_context.enable_video) {
                        err = p_engine->ConnectComponent(id, \
                                                         g_media_processor_context.demuxerID[0], \
                                                         g_media_processor_context.muxerID[index], \
                                                         StreamType_Video);
                        DASSERT_OK(err);
                        if (EECode_OK == err) {
                            test_log("[ut flow]: mudec_build_process_pipelines, connect shared demuxer(0) to muxer(%lu) success, id %08x, video path\n", index, id);
                            g_media_processor_context.connections_demuxer_2_muxer[index * 2] = id;
                        } else {
                            test_loge("ConnectComponent(), shared demuxer(0) to muxer(%lu) fail, video path, ret %d.\n", index, err);
                            return (-13);
                        }
                    }

                    if (g_media_processor_context.enable_audio && (!g_media_processor_context.only_hd_has_audio)) {
                        err = p_engine->ConnectComponent(id, \
                                                         g_media_processor_context.demuxerID[0], \
                                                         g_media_processor_context.muxerID[index], \
                                                         StreamType_Audio);
                        DASSERT_OK(err);
                        if (EECode_OK == err) {
                            test_log("[ut flow]: mudec_build_process_pipelines, connect shared demuxer(0) to muxer(%lu) success, id %08x, audio path\n", index, id);
                            g_media_processor_context.connections_demuxer_2_muxer[index * 2 + 1] = id;
                        } else {
                            test_loge("ConnectComponent(), shared demuxer(0) to muxer(%lu) fail, audio path, ret %d.\n", index, err);
                            return (-13);
                        }
                    }
                }

                total_number = shared_sd_channel_number + shared_hd_channel_number;

                for (; index < total_number; index ++) {
                    if (g_media_processor_context.enable_video) {
                        err = p_engine->ConnectComponent(id, \
                                                         g_media_processor_context.demuxerID[1], \
                                                         g_media_processor_context.muxerID[index], \
                                                         StreamType_Video);
                        DASSERT_OK(err);
                        if (EECode_OK == err) {
                            test_log("[ut flow]: mudec_build_process_pipelines, connect shared demuxer(1) to muxer(%lu) success, id %08x, video path\n", index, id);
                            g_media_processor_context.connections_demuxer_2_muxer[index * 2] = id;
                        } else {
                            test_loge("ConnectComponent(), shared demuxer(1) to muxer(%lu) fail, video path, ret %d.\n", index, err);
                            return (-13);
                        }
                    }

                    if (g_media_processor_context.enable_audio) {
                        err = p_engine->ConnectComponent(id, \
                                                         g_media_processor_context.demuxerID[1], \
                                                         g_media_processor_context.muxerID[index], \
                                                         StreamType_Audio);
                        DASSERT_OK(err);
                        if (EECode_OK == err) {
                            test_log("[ut flow]: mudec_build_process_pipelines, connect shared demuxer(1) to muxer(%lu) success, id %08x, audio path\n", index, id);
                            g_media_processor_context.connections_demuxer_2_muxer[index * 2 + 1] = id;
                        } else {
                            test_loge("ConnectComponent(), shared demuxer(1) to muxer(%lu) fail, audio path, ret %d.\n", index, err);
                            return (-13);
                        }
                    }
                }

            }
        }
    }

    if (g_media_processor_context.enable_rtsp_streaming) {
        if (g_media_processor_context.rtsp_streaming_from_playback_source) {

            //            DASSERT((g_media_processor_context.mnRTSPStreamingFromPlaybackSource + g_media_processor_context.mnRTSPStreamingFromLocalFileSource) <= EComponentMaxNumber_RTSPStreamingContent*2);
            total_number = g_media_processor_context.mnRTSPStreamingFromPlaybackSource;
            DASSERT(total_number);

            for (index = 0; index < total_number; index ++) {

                if (g_media_processor_context.enable_video) {
                    err = p_engine->ConnectComponent(id, \
                                                     g_media_processor_context.demuxerID[index], \
                                                     g_media_processor_context.rtspStreamingTransmiterID, \
                                                     StreamType_Video);
                    DASSERT_OK(err);
                    if (EECode_OK == err) {
                        test_log("[ut flow]: mudec_build_process_pipelines, connect demuxer(%lu) to rtsp streaming transmiter success, id %08x, video path\n", index, id);
                        g_media_processor_context.connections_demuxer_2_rtsp_transmiter[index * 2] = id;
                    } else {
                        test_loge("ConnectComponent(), demuxer(%lu) to rtsp streaming transmiter fail, video path, ret %d.\n", index, err);
                        return (-14);
                    }
                }

                if (g_media_processor_context.enable_audio) {
                    err = p_engine->ConnectComponent(id, \
                                                     g_media_processor_context.demuxerID[index], \
                                                     g_media_processor_context.rtspStreamingTransmiterID, \
                                                     StreamType_Audio);
                    DASSERT_OK(err);
                    if (EECode_OK == err) {
                        test_log("[ut flow]: mudec_build_process_pipelines, connect demuxer(%lu) to rtsp streaming transmiter success, id %08x, audio path\n", index, id);
                        g_media_processor_context.connections_demuxer_2_rtsp_transmiter[index * 2 + 1] = id;
                    } else {
                        test_loge("ConnectComponent(), demuxer(%lu) to rtsp streaming transmiter fail, audio path, ret %d.\n", index, err);
                        return (-15);
                    }
                }

                test_log("[ut flow]: mudec_build_process_pipelines, before create EGenericComponentType_StreamingContent manaully\n");
                err = p_engine->NewComponent(EGenericComponentType_StreamingContent, id);
                DASSERT_OK(err);
                if (EECode_OK == err) {
                    test_log("[ut flow]: mudec_build_process_pipelines, after EGenericComponentType_StreamingContent, id %08x\n", id);
                    g_media_processor_context.rtspStreamingContentPlaybackID[index] = id;
                } else {
                    test_loge("NewComponent fail, index %d, ret %d.\n", 0, err);
                    return (-7);
                }

                snprintf(rtsp_streaming_tag, 60, "live%lu", index);

                if (g_media_processor_context.enable_video && g_media_processor_context.enable_audio) {
                    err = p_engine->SetupStreamingContent(g_media_processor_context.rtspStreamingContentPlaybackID[index], g_media_processor_context.rtspStreamingServerID, g_media_processor_context.rtspStreamingTransmiterID, g_media_processor_context.demuxerID[index], g_media_processor_context.demuxerID[index], rtsp_streaming_tag);
                } else if (g_media_processor_context.enable_video && (!g_media_processor_context.enable_audio)) {
                    err = p_engine->SetupStreamingContent(g_media_processor_context.rtspStreamingContentPlaybackID[index], g_media_processor_context.rtspStreamingServerID, g_media_processor_context.rtspStreamingTransmiterID, g_media_processor_context.demuxerID[index], 0, rtsp_streaming_tag);
                } else if ((!g_media_processor_context.enable_video) && (g_media_processor_context.enable_audio)) {
                    err = p_engine->SetupStreamingContent(g_media_processor_context.rtspStreamingContentPlaybackID[index], g_media_processor_context.rtspStreamingServerID, g_media_processor_context.rtspStreamingTransmiterID, 0, g_media_processor_context.demuxerID[index], rtsp_streaming_tag);
                } else {
                    test_loge("video audio are all disabled?\n");
                    return (-8);
                }
                DASSERT_OK(err);
                if (EECode_OK == err) {
                    test_log("[ut flow]: mudec_build_process_pipelines, SetupStreamingContent, success\n");
                } else {
                    test_loge("[ut flow]: mudec_build_process_pipelines, SetupStreamingContent, fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                    return (-7);
                }

            }
        }

        if (g_media_processor_context.rtsp_streaming_from_dsp_transcoder) {

            err = p_engine->ConnectComponent(id, \
                                             g_media_processor_context.videoEncoderID[0], \
                                             g_media_processor_context.rtspStreamingTransmiterID, \
                                             StreamType_Video);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                test_log("[ut flow]: mudec_build_process_pipelines, connect video encoder(0) to rtsp streaming transmiter success, id %08x, video path\n", id);
                g_media_processor_context.connections_video_encoder_2_rtspstreamer[0] = id;
            } else {
                test_loge("ConnectComponent(), video encoder(0) to rtsp streaming transmiter fail, video path, ret %d.\n", err);
                return (-15);
            }

            if (!g_media_processor_context.debug_share_demuxer) {
                if ((g_media_processor_context.enable_audio) && (g_media_processor_context.enable_audio_pin_mux)) {
                    err = p_engine->ConnectComponent(id, \
                                                     g_media_processor_context.audioPinMuxerID[0], \
                                                     g_media_processor_context.rtspStreamingTransmiterID, \
                                                     StreamType_Audio);
                    DASSERT_OK(err);
                    if (EECode_OK == err) {
                        test_log("[ut flow]: mudec_build_process_pipelines, connect audio pin muxer(0) to rtsp streaming transmiter success, id %08x, audio path\n", id);
                        g_media_processor_context.connections_audio_pinmuxer_2_streamer[0] = id;
                    } else {
                        test_loge("ConnectComponent(), audio pin muxer(0) to rtsp streaming transmiter fail, audio path, ret %d.\n", err);
                        return (-15);
                    }
                }
            } else {
                if ((g_media_processor_context.enable_audio) && (g_media_processor_context.enable_audio_pin_mux)) {
                    err = p_engine->ConnectComponent(id, \
                                                     g_media_processor_context.audioPinMuxerID[0], \
                                                     g_media_processor_context.rtspStreamingTransmiterID, \
                                                     StreamType_Audio);
                    DASSERT_OK(err);
                    if (EECode_OK == err) {
                        test_log("[ut flow]: mudec_build_process_pipelines, connect audio pin muxer(0) to rtsp streaming transmiter success, id %08x, audio path\n", id);
                        g_media_processor_context.connections_audio_pinmuxer_2_streamer[0] = id;
                    } else {
                        test_loge("ConnectComponent(), audio pin muxer(0) to rtsp streaming transmiter fail, audio path, ret %d.\n", err);
                        return (-15);
                    }
                }
            }

            test_log("[ut flow]: mudec_build_process_pipelines, before create EGenericComponentType_StreamingContent manaully\n");
            err = p_engine->NewComponent(EGenericComponentType_StreamingContent, id);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                test_log("[ut flow]: mudec_build_process_pipelines, after EGenericComponentType_StreamingContent, id %08x\n", id);
                g_media_processor_context.rtspStreamingContentDSPTranscoder[0] = id;
            } else {
                test_loge("NewComponent fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                return (-7);
            }

            snprintf(rtsp_streaming_tag, 60, "transcode%lu", 0l);

            err = p_engine->SetupStreamingContent(g_media_processor_context.rtspStreamingContentDSPTranscoder[0], g_media_processor_context.rtspStreamingServerID, g_media_processor_context.rtspStreamingTransmiterID, g_media_processor_context.videoEncoderID[0], g_media_processor_context.audioPinMuxerID[0], rtsp_streaming_tag);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                test_log("[ut flow]: mudec_build_process_pipelines, after SetupStreamingContent, success\n");
            } else {
                test_loge("SetupStreamingContent fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                return (-7);
            }

        }

        if (g_media_processor_context.enable_cloud_server_nvr && g_media_processor_context.mnCloudReceiverAPP) {

            err = p_engine->ConnectComponent(id, \
                                             g_media_processor_context.cloudClientReceiverAPPID[0], \
                                             g_media_processor_context.rtspStreamingTransmiterID, \
                                             StreamType_Audio);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                test_log("[ut flow]: mudec_build_process_pipelines, connect app receiver(0) to rtsp streaming transmiter success, id %08x, audio path\n", id);
            } else {
                test_loge("ConnectComponent(), app receiver(0) to rtsp streaming transmiter fail, audio path, ret %d.\n", err);
                return (-15);
            }

            test_log("[ut flow]: mudec_build_process_pipelines, before create EGenericComponentType_StreamingContent manaully\n");
            err = p_engine->NewComponent(EGenericComponentType_StreamingContent, id);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                test_log("[ut flow]: mudec_build_process_pipelines, after EGenericComponentType_StreamingContent, id %08x\n", id);
                g_media_processor_context.rtspStreamingContentFromRemoteAPP[0] = id;
            } else {
                test_loge("NewComponent fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                return (-7);
            }

            snprintf(rtsp_streaming_tag, 60, "app%lu", 0l);

            err = p_engine->SetupStreamingContent(g_media_processor_context.rtspStreamingContentFromRemoteAPP[0], g_media_processor_context.rtspStreamingServerID, g_media_processor_context.rtspStreamingTransmiterID, 0, g_media_processor_context.cloudClientReceiverAPPID[0], rtsp_streaming_tag);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                test_log("[ut flow]: mudec_build_process_pipelines, after SetupStreamingContent, success\n");
            } else {
                test_loge("SetupStreamingContent fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                return (-7);
            }
        }

    }

    if (g_media_processor_context.enable_cloud_server_nvr) {

        //receiver from APP
        if (g_media_processor_context.mnCloudReceiverAPP) {
            total_number = g_media_processor_context.mnCloudReceiverAPP;
            for (index = 0; index < total_number; index ++) {
                TChar tag[128] = {0x0};
                snprintf(tag, 127, "uploading://app%ld", index);
                err = p_engine->SetupUploadingReceiverContent(g_media_processor_context.cloudClientReceiverAPPContentID[index], g_media_processor_context.cloudNVRServerID, g_media_processor_context.cloudClientReceiverAPPID[index], tag);
                DASSERT_OK(err);
                if (EECode_OK == err) {
                    test_log("[ut flow]: mudec_build_process_pipelines, SetupUploadingReceiverContent(%s) success, id %08x\n", tag, id);
                } else {
                    test_loge("SetupUploadingReceiverContent fail, index %lu, ret %d.\n", index, err);
                    return (-19);
                }
            }
        }

        //receiver from Camera
        if (g_media_processor_context.mnCloudReceiverCamera) {
            total_number = g_media_processor_context.mnCloudReceiverCamera;
            for (index = 0; index < total_number; index ++) {
                TChar tag[128] = {0x0};
                snprintf(tag, 127, "uploading://camera%ld", index);
                err = p_engine->SetupUploadingReceiverContent(g_media_processor_context.cloudClientReceiverCameraContentID[index], g_media_processor_context.cloudNVRServerID, g_media_processor_context.cloudClientReceiverCameraID[index], tag);
                DASSERT_OK(err);
                if (EECode_OK == err) {
                    test_log("[ut flow]: mudec_build_process_pipelines, SetupUploadingReceiverContent(%s) success, id %08x\n", tag, id);
                } else {
                    test_loge("SetupUploadingReceiverContent fail, index %lu, ret %d.\n", index, err);
                    return (-19);
                }
            }
        }
    }

    if (g_media_processor_context.enable_playback) {

        if (g_media_processor_context.enable_video) {
            //video decoder to video renderer
            total_number = g_media_processor_context.mnVideoDecoderNumber;

            for (index = 0; index < total_number; index ++) {
                err = p_engine->ConnectComponent(id, \
                                                 g_media_processor_context.videoDecoderID[index], \
                                                 g_media_processor_context.videoRendererID[0], \
                                                 StreamType_Video);
                DASSERT_OK(err);
                if (EECode_OK == err) {
                    test_log("[ut flow]: mudec_build_process_pipelines, connect video decoder(%lu) to video renderer success, id %08x\n", index, id);
                    g_media_processor_context.connections_video_decoder_2_video_renderer[index] = id;
                } else {
                    test_loge("ConnectComponent(), video decoder(%lu) to video renderer fail, ret %d.\n", index, err);
                    return (-14);
                }
            }
        }

        //audio decoder to audio renderer
        total_number = g_media_processor_context.mnAudioDecoderNumber;

        for (index = 0; index < total_number; index ++) {
            err = p_engine->ConnectComponent(id, \
                                             g_media_processor_context.audioDecoderID[index], \
                                             g_media_processor_context.audioRendererID[0], \
                                             StreamType_Audio);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                test_log("[ut flow]: mudec_build_process_pipelines, connect audio decoder(%lu) to audio renderer success, id %08x\n", index, id);
                g_media_processor_context.connections_audio_decoder_2_audio_renderer[index] = id;
            } else {
                test_loge("ConnectComponent(), audio decoder(%lu) to audio renderer fail, ret %d.\n", index, err);
                return (-15);
            }
        }

    }

    test_log("[ut flow]: mudec_build_process_pipelines, before p_engine->FinalizeConfigProcessPipeline(1)\n");
    err = p_engine->FinalizeConfigProcessPipeline();
    DASSERT_OK(err);

    //fill source url, sink url, and streaming tag
    test_log("[ut flow]: mudec_build_process_pipelines, specify source url, sink url and streaming tag\n");

    for (group_index = 0; group_index < d_max_source_url_group_number; group_index++) {
        if (g_media_processor_context.source_url_count_in_group[group_index]) {
            total_number = g_media_processor_context.source_url_count_in_group[group_index];
            test_log("[ut flow]: mudec_build_process_pipelines, SetDataSource, group %lu, request number %lu\n", group_index, total_number);
            for (index = 0; index < total_number; index ++) {
                err = p_engine->SetDataSource(g_media_processor_context.demuxerID[demuxer_start_index + index], g_media_processor_context.source_url_group[group_index][index].p_url);
                DASSERT_OK(err);
                if (EECode_OK == err) {
                    test_log("[ut flow]: mudec_build_process_pipelines, SetDataSource success, id %08x\n", g_media_processor_context.demuxerID[demuxer_start_index + index]);
                } else {
                    test_loge("SetDataSource fail, index %lu, ret %d.\n", demuxer_start_index + index, err);
                    return (-18);
                }
            }
            demuxer_start_index += total_number;
        }
    }
    //total_number = g_media_processor_context.sink_url_count_in_group[0];
    total_number = g_media_processor_context.mnMuxerNumber;
    test_log("[ut flow]: mudec_build_process_pipelines, SetSinkDst, group 0, request number %lu\n", total_number);
    for (index = 0; index < total_number; index ++) {
        if (g_media_processor_context.sink_url_group[0][index].p_url) {
            err = p_engine->SetSinkDst(g_media_processor_context.muxerID[index], g_media_processor_context.sink_url_group[0][index].p_url);
        } else {
            TChar recording_auto_name[128] = {0};
            snprintf(recording_auto_name, 127, "autoname_%ld_.ts", index);
            err = p_engine->SetSinkDst(g_media_processor_context.muxerID[index], recording_auto_name);
        }
        DASSERT_OK(err);
        if (EECode_OK == err) {
            test_log("[ut flow]: mudec_build_process_pipelines, SetSinkDst success, id %08x\n", id);
        } else {
            test_loge("SetSinkDst fail, index %lu, ret %d.\n", index, err);
            return (-19);
        }
    }

#if 0
    if (g_media_processor_context.enable_cloud_server_nvr) {

        //receiver from APP
        if (g_media_processor_context.mnCloudReceiverAPP) {
            total_number = g_media_processor_context.mnCloudReceiverAPP;
            for (index = 0; index < total_number; index ++) {
                TChar tag[128] = {0x0};
                snprintf(tag, 127, "uploading://app%d", index);
                err = p_engine->SetCloudClientRecieverTag(g_media_processor_context.cloudClientReceiverAPPID[index], tag);
                DASSERT_OK(err);
                if (EECode_OK == err) {
                    test_log("[ut flow]: mudec_build_process_pipelines, SetCloudClientRecieverTag(%s) success, id %08x\n", tag, id);
                } else {
                    test_loge("SetCloudClientRecieverTag fail, index %lu, ret %d.\n", index, err);
                    return (-19);
                }
            }
        }

        //receiver from Camera
        if (g_media_processor_context.mnCloudReceiverCamera) {
            total_number = g_media_processor_context.mnCloudReceiverCamera;
            for (index = 0; index < total_number; index ++) {
                TChar tag[128] = {0x0};
                snprintf(tag, 127, "uploading://camera%d", index);
                err = p_engine->SetCloudClientRecieverTag(g_media_processor_context.cloudClientReceiverCameraID[index], tag);
                DASSERT_OK(err);
                if (EECode_OK == err) {
                    test_log("[ut flow]: mudec_build_process_pipelines, SetCloudClientRecieverTag(%s) success, id %08x\n", tag, id);
                } else {
                    test_loge("SetCloudClientRecieverTag fail, index %lu, ret %d.\n", index, err);
                    return (-19);
                }
            }
        }
    }
#endif

    if (g_media_processor_context.enable_playback) {
        //query playback pipelines
        demuxer_start_index = 0;
        for (group_index = 0; group_index < d_max_source_url_group_number; group_index++) {
            if (g_media_processor_context.source_url_count_in_group[group_index]) {
                total_number = g_media_processor_context.source_url_count_in_group[group_index];
                test_log("[ut flow]: mudec_build_process_pipelines, before query playback pipeline from source component, group %lu, request number %lu\n", group_index, total_number);
                for (index = 0; index < total_number; index ++) {
                    err = p_engine->QueryPipelineID(EGenericPipelineType_Playback, g_media_processor_context.demuxerID[demuxer_start_index + index], id);
                    DASSERT_OK(err);
                    if (EECode_OK == err) {
                        test_log("[ut flow]: mudec_build_process_pipelines, QueryPipelineID(playback) success, id %08x\n", id);
                        g_media_processor_context.playback_pipelines[demuxer_start_index + index] = id;
                    } else {
                        test_loge("QueryPipelineID(playback) fail, index %lu, ret %d.\n", demuxer_start_index + index, err);
                        return (-4);
                    }
                }
                demuxer_start_index += total_number;
            }
        }
    }

    if (g_media_processor_context.enable_mux_file) {
        //query recording pipelines
        if (g_mtest_save_all_channel) {
            total_number = g_media_processor_context.mnDemuxerNumber;
        } else {
            total_number = g_media_processor_context.sink_url_count_in_group[g_cur_recording_url_group];
        }
        test_log("[ut flow]: mudec_build_process_pipelines, before query recording pipeline from sink component, group 0, request number %lu\n", total_number);
        for (index = 0; index < total_number; index ++) {
            err = p_engine->QueryPipelineID(EGenericPipelineType_Recording, g_media_processor_context.muxerID[index], id);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                test_log("[ut flow]: mudec_build_process_pipelines, QueryPipelineID(recording) success, id %08x\n", id);
                g_media_processor_context.recording_pipelines[index] = id;
            } else {
                test_loge("QueryPipelineID(recording) fail, index %lu, ret %d.\n", index, err);
                return (-4);
            }
        }
    }

    if (g_media_processor_context.enable_rtsp_streaming) {
        if (g_media_processor_context.rtsp_streaming_from_playback_source) {
            //query streaming pipelines, from playback sources
            total_number = g_media_processor_context.source_url_count_in_group[g_cur_playback_url_group];
            test_log("[ut flow]: mudec_build_process_pipelines, before query streaming pipeline from playback component, group 0, request number %lu\n", total_number);
            for (index = 0; index < total_number; index ++) {
                err = p_engine->QueryPipelineID(EGenericPipelineType_Streaming, g_media_processor_context.rtspStreamingContentPlaybackID[index], id);
                DASSERT_OK(err);
                if (EECode_OK == err) {
                    test_log("[ut flow]: mudec_build_process_pipelines, QueryPipelineID(streaming) success, id %08x\n", id);
                    g_media_processor_context.streaming_pipelines[index] = id;
                } else {
                    test_loge("QueryPipelineID(streaming) fail, index %lu, ret %d.\n", index, err);
                    return (-4);
                }
            }
        }

        if (g_media_processor_context.rtsp_streaming_from_dsp_transcoder) {
            //query streaming pipelines, from dsp transcoder
            err = p_engine->QueryPipelineID(EGenericPipelineType_Streaming, g_media_processor_context.rtspStreamingContentDSPTranscoder[0], id);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                test_log("[ut flow]: mudec_build_process_pipelines, QueryPipelineID(streaming) success, id %08x\n", id);
                g_media_processor_context.streaming_pipelines[0] = id;
            } else {
                test_loge("QueryPipelineID(streaming) fail, index 0, ret %d.\n", err);
                return (-4);
            }
        }
    }

    //p_engine->StartProcess();

    test_log("[ut flow]: mudec_build_process_pipelines, end\n");
    return 0;
}

TInt generic_nvr_build_process_pipelines()
{
    SGenericPreConfigure pre_config;
    TULong index = 0;
    TULong total_number = 0;
    TUint shared_sd_channel_number = 0, shared_hd_channel_number = 0;

    TChar rtsp_streaming_tag[64] = {0};

    TGenericID id;

    EECode err = EECode_NotInitilized;
    IGenericEngineControl *p_engine = gfMediaEngineFactory(EFactory_MediaEngineType_Generic);

    test_log("[ut flow]: generic_nvr_build_process_pipelines, start, g_mtest_dsp_dec_instances_number_1 %d, g_mtest_dsp_dec_instances_number_2 %d\n", g_mtest_dsp_dec_instances_number_1, g_mtest_dsp_dec_instances_number_2);

    DASSERT(p_engine);

    if ((!p_engine)) {
        test_loge("gfMediaEngineFactory(%d), return %p\n", EFactory_MediaEngineType_Generic, p_engine);
        return (-3);
    }

    DASSERT(!g_media_processor_context.p_engine);
    g_media_processor_context.p_engine = p_engine;

    DASSERT(g_media_processor_context.enable_rtsp_streaming || g_media_processor_context.enable_playback || g_media_processor_context.enable_mux_file);

    DASSERT(EMediaDeviceWorkingMode_NVR_RTSP_Streaming == g_mtest_dspmode);

    test_log("[ut flow]: generic_nvr_build_process_pipelines, before p_engine->SetWorkingMode(%d, %d)\n", g_mtest_dspmode, g_mtest_playbackmode);
    err = p_engine->SetWorkingMode(g_mtest_dspmode, g_mtest_playbackmode);
    DASSERT_OK(err);

    if (g_b_mtest_force_log_level) {
        test_log("[ut flow]: generic_nvr_build_process_pipelines, before p_engine->SetEngineLogLevel(%d)\n", g_b_mtest_force_log_level);
        err = p_engine->SetEngineLogLevel((ELogLevel)g_mtest_config_log_level);
        DASSERT_OK(err);
    }

    if (g_mtest_net_receiver_scheduler_number) {
        test_log("[ut flow]: generic_nvr_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_NetworkReceiverSchedulerNumber, %d)\n", g_mtest_net_receiver_scheduler_number);
        pre_config.check_field = EGenericEnginePreConfigure_NetworkReceiverSchedulerNumber;
        pre_config.number = g_mtest_net_receiver_scheduler_number;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_NetworkReceiverSchedulerNumber, &pre_config);
        DASSERT_OK(err);
    }

    if (g_mtest_net_sender_scheduler_number) {
        test_log("[ut flow]: generic_nvr_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_NetworkTransmiterSchedulerNumber, %d)\n", g_mtest_net_sender_scheduler_number);
        pre_config.check_field = EGenericEnginePreConfigure_NetworkTransmiterSchedulerNumber;
        pre_config.number = g_mtest_net_sender_scheduler_number;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_NetworkTransmiterSchedulerNumber, &pre_config);
        DASSERT_OK(err);
    }

    if (g_mtest_fileio_reader_scheduler_number) {
        test_log("[ut flow]: generic_nvr_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_IOReaderSchedulerNumber, %d)\n", g_mtest_fileio_reader_scheduler_number);
        pre_config.check_field = EGenericEnginePreConfigure_IOReaderSchedulerNumber;
        pre_config.number = g_mtest_fileio_reader_scheduler_number;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_IOReaderSchedulerNumber, &pre_config);
        DASSERT_OK(err);
    }

    if (g_mtest_fileio_writer_scheduler_number) {
        test_log("[ut flow]: generic_nvr_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_IOWriterSchedulerNumber, %d)\n", g_mtest_fileio_writer_scheduler_number);
        pre_config.check_field = EGenericEnginePreConfigure_IOWriterSchedulerNumber;
        pre_config.number = g_mtest_fileio_writer_scheduler_number;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_IOWriterSchedulerNumber, &pre_config);
        DASSERT_OK(err);
    }

    if (gb_mtest_rtsp_client_try_tcp_mode_first) {
        test_log("[ut flow]: generic_nvr_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_RTSPClientTryTCPModeFirst, %d)\n", gb_mtest_rtsp_client_try_tcp_mode_first);
        pre_config.check_field = EGenericEnginePreConfigure_RTSPClientTryTCPModeFirst;
        pre_config.number = gb_mtest_rtsp_client_try_tcp_mode_first;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_RTSPClientTryTCPModeFirst, &pre_config);
        DASSERT_OK(err);
    }

    if (g_mtest_rtsp_server_port) {
        test_log("[ut flow]: generic_nvr_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_RTSPStreamingServerPort, %d)\n", g_mtest_rtsp_server_port);
        pre_config.check_field = EGenericEnginePreConfigure_RTSPStreamingServerPort;
        pre_config.number = g_mtest_rtsp_server_port;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_RTSPStreamingServerPort, &pre_config);
        DASSERT_OK(err);
    }

    if (g_media_processor_context.streaming_timeout_threashold) {
        test_log("[ut flow]: generic_nvr_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_SetStreamingTimeoutThreashold, %d)\n", g_media_processor_context.streaming_timeout_threashold);
        pre_config.check_field = EGenericEnginePreConfigure_SetStreamingTimeoutThreashold;
        pre_config.number = g_media_processor_context.streaming_timeout_threashold;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_SetStreamingTimeoutThreashold, &pre_config);
        DASSERT_OK(err);
    }

    if (g_media_processor_context.streaming_autoretry_connect_server_interval) {
        test_log("[ut flow]: generic_nvr_build_process_pipelines, before p_engine->GenericPreConfigure(EGenericEnginePreConfigure_StreamingRetryConnectServerInterval, %d)\n", g_media_processor_context.streaming_autoretry_connect_server_interval);
        pre_config.check_field = EGenericEnginePreConfigure_StreamingRetryConnectServerInterval;
        pre_config.number = g_media_processor_context.streaming_autoretry_connect_server_interval;
        err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_StreamingRetryConnectServerInterval, &pre_config);
        DASSERT_OK(err);
    }

    test_log("[ut flow]: generic_nvr_build_process_pipelines, before p_engine->BeginConfigProcessPipeline(1)\n");
    err = p_engine->BeginConfigProcessPipeline(1);
    DASSERT_OK(err);

    //build customized process pipeline

    //streaming server
    if (g_media_processor_context.enable_rtsp_streaming) {

        err = p_engine->NewComponent(EGenericComponentType_StreamingServer, id);
        DASSERT_OK(err);
        if (EECode_OK == err) {
            test_log("[ut flow]: generic_nvr_build_process_pipelines, new rtsp streaming server success, id %08x\n", id);
            g_media_processor_context.rtspStreamingServerID = id;
        } else {
            test_loge("NewComponent fail, index %lu, ret %d.\n", index, err);
            return (-4);
        }

        err = p_engine->NewComponent(EGenericComponentType_StreamingTransmiter, id);
        DASSERT_OK(err);
        if (EECode_OK == err) {
            test_log("[ut flow]: generic_nvr_build_process_pipelines, new rtsp streaming transmiter success, id %08x\n", id);
            g_media_processor_context.rtspStreamingTransmiterID = id;
        } else {
            test_loge("NewComponent fail, index %lu, ret %d.\n", index, err);
            return (-4);
        }

    }

    //demuxer
    total_number = g_media_processor_context.source_url_count_in_group[g_cur_playback_url_group];
    test_log("[ut flow]: generic_nvr_build_process_pipelines, before create source component, group 0, request number %lu\n", total_number);
    for (index = 0; index < total_number; index ++) {
        err = p_engine->NewComponent(EGenericComponentType_Demuxer, id);
        DASSERT_OK(err);
        if (EECode_OK == err) {
            test_log("[ut flow]: generic_nvr_build_process_pipelines, new demuxer success, id %08x\n", id);
            g_media_processor_context.demuxerID[index] = id;
        } else {
            test_loge("NewComponent fail, index %lu, ret %d.\n", index, err);
            return (-4);
        }
    }
    g_media_processor_context.mnDemuxerNumber = total_number;

    if (g_media_processor_context.enable_mux_file) {
        //muxer
        if (g_mtest_save_all_channel) {
            total_number = g_media_processor_context.mnDemuxerNumber;
        } else {
            total_number = g_media_processor_context.sink_url_count_in_group[g_cur_recording_url_group];
        }
        test_log("[ut flow]: generic_nvr_build_process_pipelines, before create sink component, group 0, request number %lu\n", total_number);
        for (index = 0; index < total_number; index ++) {
            err = p_engine->NewComponent(EGenericComponentType_Muxer, id);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                test_log("[ut flow]: generic_nvr_build_process_pipelines, new muxer success, id %08x\n", id);
                g_media_processor_context.muxerID[index] = id;
            } else {
                test_loge("NewComponent fail, index %lu, ret %d.\n", index, err);
                return (-4);
            }
        }
        g_media_processor_context.mnMuxerNumber = total_number;
    }

    //streaming content
    if (g_media_processor_context.rtsp_streaming_from_playback_source) {
        total_number = g_media_processor_context.source_url_count_in_group[0];
#if 0
        test_log("[ut flow]: generic_nvr_build_process_pipelines, before create streaming(playback source) content component, request number %lu\n", total_number);
        for (index = 0; index < total_number; index ++) {
            err = p_engine->NewComponent(EGenericComponentType_StreamingContent, id);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                test_log("[ut flow]: generic_nvr_build_process_pipelines, new streaming content success, id %08x\n", id);
                g_media_processor_context.rtspStreamingContentPlaybackID[index] = id;
            } else {
                test_loge("NewComponent fail, index %lu, ret %d.\n", index, err);
                return (-4);
            }
        }
#endif
        g_media_processor_context.mnRTSPStreamingFromPlaybackSource = total_number;
    }

    //connect components

    if (g_media_processor_context.enable_mux_file) {
        //demuxer to muxer
        if (g_media_processor_context.mnMuxerNumber) {
            DASSERT(g_media_processor_context.mnDemuxerNumber == g_media_processor_context.mnMuxerNumber);

            total_number = g_media_processor_context.mnMuxerNumber;

            for (index = 0; index < total_number; index ++) {
                if (g_media_processor_context.enable_video) {
                    err = p_engine->ConnectComponent(id, \
                                                     g_media_processor_context.demuxerID[index], \
                                                     g_media_processor_context.muxerID[index], \
                                                     StreamType_Video);
                    DASSERT_OK(err);
                    if (EECode_OK == err) {
                        test_log("[ut flow]: generic_nvr_build_process_pipelines, connect demuxer(%lu) to muxer(%lu) success, id %08x, video path\n", index, index, id);
                        g_media_processor_context.connections_demuxer_2_muxer[index * 2] = id;
                    } else {
                        test_loge("ConnectComponent(), demuxer(%lu) to muxer(%lu) fail, video path, ret %d.\n", index, index, err);
                        return (-13);
                    }
                }

                if (g_media_processor_context.enable_audio) {
                    if ((!g_media_processor_context.only_hd_has_audio) || (g_mtest_dsp_dec_instances_number_1 <= index)) {
                        err = p_engine->ConnectComponent(id, \
                                                         g_media_processor_context.demuxerID[index], \
                                                         g_media_processor_context.muxerID[index], \
                                                         StreamType_Audio);
                        DASSERT_OK(err);
                        if (EECode_OK == err) {
                            test_log("[ut flow]: generic_nvr_build_process_pipelines, connect demuxer(%lu) to muxer(%lu) success, id %08x, audio path\n", index, index, id);
                            g_media_processor_context.connections_demuxer_2_muxer[index * 2 + 1] = id;
                        } else {
                            test_loge("ConnectComponent(), demuxer(%lu) to muxer(%lu) fail, audio path, ret %d.\n", index, index, err);
                            return (-13);
                        }
                    }
                }
            }
        }
    }

    if (g_media_processor_context.enable_rtsp_streaming) {
        //test_log("g_media_processor_context.rtsp_streaming_from_playback_source %d, g_media_processor_context.mnRTSPStreamingFromPlaybackSource %d\n", g_media_processor_context.rtsp_streaming_from_playback_source, g_media_processor_context.mnRTSPStreamingFromPlaybackSource);
        if (g_media_processor_context.rtsp_streaming_from_playback_source) {

            //            DASSERT((g_media_processor_context.mnRTSPStreamingFromPlaybackSource + g_media_processor_context.mnRTSPStreamingFromLocalFileSource) <= EComponentMaxNumber_RTSPStreamingContent*2);
            total_number = g_media_processor_context.mnRTSPStreamingFromPlaybackSource;

            for (index = 0; index < total_number; index ++) {
                err = p_engine->ConnectComponent(id, \
                                                 g_media_processor_context.demuxerID[index], \
                                                 g_media_processor_context.rtspStreamingTransmiterID, \
                                                 StreamType_Video);
                DASSERT_OK(err);
                if (EECode_OK == err) {
                    test_log("[ut flow]: generic_nvr_build_process_pipelines, connect demuxer(%lu) to rtsp streaming transmiter success, id %08x, video path\n", index, id);
                    g_media_processor_context.connections_demuxer_2_rtsp_transmiter[index * 2] = id;
                } else {
                    test_loge("ConnectComponent(), demuxer(%lu) to rtsp streaming transmiter fail, video path, ret %d.\n", index, err);
                    return (-14);
                }

                if (g_media_processor_context.enable_audio) {
                    err = p_engine->ConnectComponent(id, \
                                                     g_media_processor_context.demuxerID[index], \
                                                     g_media_processor_context.rtspStreamingTransmiterID, \
                                                     StreamType_Audio);
                    DASSERT_OK(err);
                    if (EECode_OK == err) {
                        test_log("[ut flow]: generic_nvr_build_process_pipelines, connect demuxer(%lu) to rtsp streaming transmiter success, id %08x, audio path\n", index, id);
                        g_media_processor_context.connections_demuxer_2_rtsp_transmiter[index * 2 + 1] = id;
                    } else {
                        test_loge("ConnectComponent(), demuxer(%lu) to rtsp streaming transmiter fail, audio path, ret %d.\n", index, err);
                        return (-15);
                    }
                }

                test_log("[ut flow]: generic_nvr_build_process_pipelines, before create EGenericComponentType_StreamingContent manaully\n");
                err = p_engine->NewComponent(EGenericComponentType_StreamingContent, id);
                DASSERT_OK(err);
                if (EECode_OK == err) {
                    test_log("[ut flow]: generic_nvr_build_process_pipelines, after EGenericComponentType_StreamingContent, id %08x\n", id);
                    g_media_processor_context.rtspStreamingContentPlaybackID[index] = id;
                } else {
                    test_loge("NewComponent fail, index %d, ret %d.\n", 0, err);
                    return (-7);
                }

                snprintf(rtsp_streaming_tag, 60, "live%lu", index);
                test_log("[ut flow]: generic_nvr_build_process_pipelines, SetupStreamingContent(content 0x%08x, server 0x%08x, transmiter 0x%08x, video 0x%08x, audio 0x%08x, %s)\n", g_media_processor_context.rtspStreamingContentPlaybackID[index], g_media_processor_context.rtspStreamingServerID, g_media_processor_context.rtspStreamingTransmiterID, g_media_processor_context.demuxerID[index], g_media_processor_context.demuxerID[index], rtsp_streaming_tag);
                err = p_engine->SetupStreamingContent(g_media_processor_context.rtspStreamingContentPlaybackID[index], g_media_processor_context.rtspStreamingServerID, g_media_processor_context.rtspStreamingTransmiterID, g_media_processor_context.demuxerID[index], g_media_processor_context.demuxerID[index], rtsp_streaming_tag);

                DASSERT_OK(err);
                if (EECode_OK == err) {
                    test_log("[ut flow]: setup EGenericComponentType_RTSPStreamingContent, id %08x\n", id);
                } else {
                    test_log("[ut flow]: setup EGenericComponentType_RTSPStreamingContent, id %08x fail, ret %d, %s\n", id, err, gfGetErrorCodeString(err));
                    return (-8);
                }

            }
        }
    }

    test_log("[ut flow]: generic_nvr_build_process_pipelines, before p_engine->FinalizeConfigProcessPipeline(1)\n");
    err = p_engine->FinalizeConfigProcessPipeline();
    DASSERT_OK(err);

    //fill source url, sink url, and streaming tag
    test_log("[ut flow]: generic_nvr_build_process_pipelines, specify source url, sink url and streaming tag\n");

    total_number = g_media_processor_context.source_url_count_in_group[0];
    test_log("[ut flow]: generic_nvr_build_process_pipelines, SetDataSource, group 0, request number %lu\n", total_number);
    for (index = 0; index < total_number; index ++) {
        err = p_engine->SetDataSource(g_media_processor_context.demuxerID[index], g_media_processor_context.source_url_group[0][index].p_url);
        DASSERT_OK(err);
        if (EECode_OK == err) {
            test_log("[ut flow]: generic_nvr_build_process_pipelines, SetDataSource success, id %08x\n", id);
        } else {
            test_loge("SetDataSource fail, index %lu, ret %d.\n", index, err);
            return (-18);
        }
    }

    //total_number = g_media_processor_context.sink_url_count_in_group[0];
    total_number = g_media_processor_context.mnMuxerNumber;
    test_log("[ut flow]: generic_nvr_build_process_pipelines, SetSinkDst, group 0, request number %lu\n", total_number);
    for (index = 0; index < total_number; index ++) {
        if (g_media_processor_context.sink_url_group[0][index].p_url) {
            err = p_engine->SetSinkDst(g_media_processor_context.muxerID[index], g_media_processor_context.sink_url_group[0][index].p_url);
        } else {
            TChar recording_auto_name[128] = {0};
            snprintf(recording_auto_name, 127, "autoname_%ld_.ts", index);
            err = p_engine->SetSinkDst(g_media_processor_context.muxerID[index], recording_auto_name);
        }
        DASSERT_OK(err);
        if (EECode_OK == err) {
            test_log("[ut flow]: generic_nvr_build_process_pipelines, SetSinkDst success, id %08x\n", id);
        } else {
            test_loge("SetSinkDst fail, index %lu, ret %d.\n", index, err);
            return (-19);
        }
    }

    if (g_media_processor_context.enable_mux_file) {
        //query recording pipelines
        if (g_mtest_save_all_channel) {
            total_number = g_media_processor_context.mnDemuxerNumber;
        } else {
            total_number = g_media_processor_context.sink_url_count_in_group[g_cur_recording_url_group];
        }
        test_log("[ut flow]: generic_nvr_build_process_pipelines, before query recording pipeline frm sink component, group 0, request number %lu\n", total_number);
        for (index = 0; index < total_number; index ++) {
            err = p_engine->QueryPipelineID(EGenericPipelineType_Recording, g_media_processor_context.muxerID[index], id);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                test_log("[ut flow]: generic_nvr_build_process_pipelines, QueryPipelineID(recording) success, id %08x\n", id);
                g_media_processor_context.recording_pipelines[index] = id;
            } else {
                test_loge("QueryPipelineID(recording) fail, index %lu, ret %d.\n", index, err);
                return (-4);
            }
        }
    }

    if (g_media_processor_context.enable_rtsp_streaming) {
        //query streaming pipelines, from playback sources
        total_number = g_media_processor_context.source_url_count_in_group[g_cur_playback_url_group];
        test_log("[ut flow]: generic_nvr_build_process_pipelines, before query streaming pipeline frm playback component, group 0, request number %lu\n", total_number);
        for (index = 0; index < total_number; index ++) {
            err = p_engine->QueryPipelineID(EGenericPipelineType_Streaming, g_media_processor_context.rtspStreamingContentPlaybackID[index], id);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                test_log("[ut flow]: generic_nvr_build_process_pipelines, QueryPipelineID(streaming) success, id %08x\n", id);
                g_media_processor_context.streaming_pipelines[index] = id;
            } else {
                test_loge("QueryPipelineID(streaming) fail, index %lu, ret %d.\n", index, err);
                return (-4);
            }
        }
    }

    test_log("[ut flow]: generic_nvr_build_process_pipelines, end\n");
    return 0;
}

static void _mtest_process_msg(void *context, SMSG &msg)
{
    test_log("engine's msg: %d, context=%p\n", msg.code, context);

    if (msg.code == EMSGType_PlaybackEOS) {
        test_log("==== Playback end ====\n");
        IGenericEngineControl *p_engine = (IGenericEngineControl *)context;
        if (p_engine) {
            test_log("==== Playback loop ====\n");
        }
    }
}

TInt udec_build_process_pipelines()
{
    TULong index = 0;
    TULong total_number = 0;

    TGenericID id = 0;

    EECode err = EECode_NotInitilized;
    IGenericEngineControl *p_engine = gfMediaEngineFactory(EFactory_MediaEngineType_Generic);

    test_log("[ut flow]: udec_build_process_pipelines, start\n");

    DASSERT(p_engine);

    if ((!p_engine)) {
        test_loge("gfMediaEngineFactory(%d), return %p\n", EFactory_MediaEngineType_Generic, p_engine);
        return (-3);
    }

    p_engine->SetAppMsgCallback(_mtest_process_msg, (void *)p_engine);

    DASSERT(!g_media_processor_context.p_engine);
    g_media_processor_context.p_engine = p_engine;

    DASSERT(g_media_processor_context.enable_playback);
    if (!g_media_processor_context.enable_playback) {
        test_loge("why g_media_processor_context.enable_playback is zero?\n");
        return (-4);
    }

    DASSERT(EMediaDeviceWorkingMode_SingleInstancePlayback == g_mtest_dspmode);
    DASSERT(EPlaybackMode_1x1080p == g_mtest_playbackmode);

    test_log("[ut flow]: udec_build_process_pipelines, before p_engine->SetWorkingMode(%d, %d)\n", g_mtest_dspmode, g_mtest_playbackmode);
    err = p_engine->SetWorkingMode(g_mtest_dspmode, g_mtest_playbackmode);
    DASSERT_OK(err);

    DASSERT(DCAL_BITMASK(EDisplayDevice_HDMI) == g_mtest_voutmask);
    test_log("[ut flow]: udec_build_process_pipelines, before p_engine->SetDisplayDevice(0x%08x)\n", g_mtest_voutmask);
    err = p_engine->SetDisplayDevice(g_mtest_voutmask);
    DASSERT_OK(err);

    if (g_b_mtest_force_log_level) {
        test_log("[ut flow]: udec_build_process_pipelines, before p_engine->SetEngineLogLevel(%d)\n", g_mtest_config_log_level);
        err = p_engine->SetEngineLogLevel((ELogLevel)g_mtest_config_log_level);
        DASSERT_OK(err);
    }

    test_log("[ut flow]: udec_build_process_pipelines, before p_engine->BeginConfigProcessPipeline(1)\n");
    err = p_engine->BeginConfigProcessPipeline(1);
    DASSERT_OK(err);

    //build customized process pipeline
    //demuxer
    total_number = g_media_processor_context.source_url_count_in_group[g_cur_playback_url_group];
    test_log("[ut flow]: udec_build_process_pipelines, before create source component, group 0, total number %lu\n", total_number);

    index = 0; //only one demuxer
    err = p_engine->NewComponent(EGenericComponentType_Demuxer, id);
    DASSERT_OK(err);
    if (EECode_OK == err) {
        test_log("[ut flow]: udec_build_process_pipelines, new demuxer success, id %08x\n", id);
        g_media_processor_context.demuxerID[index] = id;
    } else {
        test_loge("NewComponent fail, index %lu, ret %d.\n", index, err);
        return (-5);
    }
    g_media_processor_context.mnDemuxerNumber = 1;

    if (g_media_processor_context.enable_video) {
        index = 0; //only one video decoder
        err = p_engine->NewComponent(EGenericComponentType_VideoDecoder, id);
        DASSERT_OK(err);
        if (EECode_OK == err) {
            test_log("[ut flow]: udec_build_process_pipelines, new video decoder success, id %08x\n", id);
            g_media_processor_context.videoDecoderID[index] = id;
        } else {
            test_loge("NewComponent fail, index %lu, ret %d.\n", index, err);
            return (-6);
        }
        g_media_processor_context.mnVideoDecoderNumber = 1;
    }

    if (g_media_processor_context.enable_audio) {
        //audio decoder
        index = 0; //only one audio decoder
        err = p_engine->NewComponent(EGenericComponentType_AudioDecoder, id);
        DASSERT_OK(err);
        if (EECode_OK == err) {
            test_log("[ut flow]: udec_build_process_pipelines, new audio decoder success, id %08x\n", id);
            g_media_processor_context.audioDecoderID[index] = id;
        } else {
            test_loge("NewComponent fail, index %lu, ret %d.\n", index, err);
            return (-7);
        }
        g_media_processor_context.mnAudioDecoderNumber = 1;
    }

    if (g_media_processor_context.enable_video) {
        //video renderer
        err = p_engine->NewComponent(EGenericComponentType_VideoRenderer, id);
        DASSERT_OK(err);
        if (EECode_OK == err) {
            test_log("[ut flow]: udec_build_process_pipelines, new video renderer success, id %08x\n", id);
            g_media_processor_context.videoRendererID[0] = id;
        } else {
            test_loge("NewComponent fail, index %d, ret %d.\n", 0, err);
            return (-9);
        }
        g_media_processor_context.mnVideoRendererNumber = 1;
    }

    if (g_media_processor_context.enable_audio) {
        //audio renderer
        err = p_engine->NewComponent(EGenericComponentType_AudioRenderer, id);
        DASSERT_OK(err);
        if (EECode_OK == err) {
            test_log("[ut flow]: udec_build_process_pipelines, new demuxer success, id %08x\n", id);
            g_media_processor_context.audioRendererID[0] = id;
        } else {
            test_loge("NewComponent fail, index %d, ret %d.\n", 0, err);
            return (-10);
        }
        g_media_processor_context.mnAudioRendererNumber = 1;
    }
    //connect components

    if (g_media_processor_context.enable_video) {
        //demuxer to video decoder
        DASSERT(g_media_processor_context.mnDemuxerNumber == g_media_processor_context.mnVideoDecoderNumber);
        total_number = g_media_processor_context.mnDemuxerNumber;

        for (index = 0; index < total_number; index ++) {
            err = p_engine->ConnectComponent(id, \
                                             g_media_processor_context.demuxerID[index], \
                                             g_media_processor_context.videoDecoderID[index], \
                                             StreamType_Video);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                test_log("[ut flow]: udec_build_process_pipelines, connect demuxer(%lu) to video decoder(%lu) success, id %08x\n", index, index, id);
                g_media_processor_context.connections_demuxer_2_video_decoder[index] = id;
            } else {
                test_loge("ConnectComponent(), demuxer(%lu) to video decoder(%lu) fail, ret %d.\n", index, index, err);
                return (-11);
            }
        }
    }

    if (g_media_processor_context.enable_audio) {
        //demuxer to audio decoder
        DASSERT(g_media_processor_context.mnDemuxerNumber == g_media_processor_context.mnAudioDecoderNumber);
        total_number = g_media_processor_context.mnDemuxerNumber;

        for (index = 0; index < total_number; index ++) {
            err = p_engine->ConnectComponent(id, \
                                             g_media_processor_context.demuxerID[index], \
                                             g_media_processor_context.audioDecoderID[index], \
                                             StreamType_Audio);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                test_log("[ut flow]: udec_build_process_pipelines, connect demuxer(%lu) to audio decoder(%lu) success, id %08x\n", index, index, id);
                g_media_processor_context.connections_demuxer_2_audio_decoder[index] = id;
            } else {
                test_loge("ConnectComponent(), demuxer(%lu) to audio decoder(%lu) fail, ret %d.\n", index, index, err);
                return (-12);
            }
        }
    }

    if (g_media_processor_context.enable_video) {
        //video decoder to video renderer
        total_number = g_media_processor_context.mnVideoDecoderNumber;

        for (index = 0; index < total_number; index ++) {
            err = p_engine->ConnectComponent(id, \
                                             g_media_processor_context.videoDecoderID[index], \
                                             g_media_processor_context.videoRendererID[0], \
                                             StreamType_Video);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                test_log("[ut flow]: udec_build_process_pipelines, connect video decoder(%lu) to video renderer success, id %08x\n", index, id);
            } else {
                test_loge("ConnectComponent(), video decoder(%lu) to video renderer fail, ret %d.\n", index, err);
                return (-14);
            }
        }
    }

    if (g_media_processor_context.enable_audio) {
        //audio decoder to audio renderer
        total_number = g_media_processor_context.mnAudioDecoderNumber;

        for (index = 0; index < total_number; index ++) {
            err = p_engine->ConnectComponent(id, \
                                             g_media_processor_context.audioDecoderID[index], \
                                             g_media_processor_context.audioRendererID[0], \
                                             StreamType_Audio);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                test_log("[ut flow]: udec_build_process_pipelines, connect audio decoder(%lu) to audio renderer success, id %08x\n", index, id);
            } else {
                test_loge("ConnectComponent(), audio decoder(%lu) to audio renderer fail, ret %d.\n", index, err);
                return (-15);
            }
        }
    }


    //fill source url, sink url, and streaming tag
    test_log("[ut flow]: udec_build_process_pipelines, specify source url, sink url and streaming tag\n");

    total_number = 1; //only one demuxer
    test_log("[ut flow]: udec_build_process_pipelines, SetDataSource, group 0, request number %lu\n", total_number);
    for (index = 0; index < total_number; index ++) {
        err = p_engine->SetDataSource(g_media_processor_context.demuxerID[index], g_media_processor_context.source_url_group[0][index].p_url);
        DASSERT_OK(err);
        if (EECode_OK == err) {
            test_log("[ut flow]: udec_build_process_pipelines, SetDataSource success, id %08x\n", id);
        } else {
            test_loge("SetDataSource fail, index %lu, ret %d.\n", index, err);
            return (-18);
        }
    }

    test_log("[ut flow]: udec_build_process_pipelines, before p_engine->FinalizeConfigProcessPipeline(1)\n");
    err = p_engine->FinalizeConfigProcessPipeline();
    DASSERT_OK(err);

    //query playback pipelines
    total_number = 1; //only one playback pipeline
    test_log("[ut flow]: udec_build_process_pipelines, before query playback pipeline frm source component, group 0, request number %lu\n", total_number);
    for (index = 0; index < total_number; index ++) {
        err = p_engine->QueryPipelineID(EGenericPipelineType_Playback, g_media_processor_context.demuxerID[index], id);
        DASSERT_OK(err);
        if (EECode_OK == err) {
            test_log("[ut flow]: udec_build_process_pipelines, QueryPipelineID(playback) success, id %08x\n", id);
            g_media_processor_context.playback_pipelines[index] = id;
        } else {
            test_loge("QueryPipelineID(playback) fail, index %lu, ret %d.\n", index, err);
            return (-4);
        }
    }

    //p_engine->StartProcess();

    test_log("[ut flow]: udec_build_process_pipelines, end\n");
    return 0;
}

TInt __start_process()
{
    EECode err;
    IGenericEngineControl *p_engine = g_media_processor_context.p_engine;
    DASSERT(p_engine);

    test_log("[ut flow]: __start_process, start, p_engine %p\n", p_engine);

    if (p_engine) {
        err = p_engine->StartProcess();
        DASSERT_OK(err);

        if (EECode_OK == err) {
            test_log("[flow]: p_engine->StartProcess() success\n");
        } else {
            test_loge("p_engine->StartProcess() fail, return %d\n", err);
            return (-1);
        }
    } else {
        test_loge("NULL p_engine in mudec_start_process.\n");
        return (-2);
    }

    test_log("[ut flow]: __start_process, end\n");

    return 0;
}

TInt __stop_process()
{
    EECode err;
    IGenericEngineControl *p_engine = g_media_processor_context.p_engine;
    DASSERT(p_engine);

    if (p_engine) {
        err = p_engine->StopProcess();
        DASSERT_OK(err);

        if (EECode_OK == err) {
            test_log("[flow]: p_engine->StopProcess() success\n");
        } else {
            test_loge("p_engine->StopProcess() fail, return %d\n", err);
            return (-1);
        }
    } else {
        test_loge("NULL p_engine in mudec_stop_process.\n");
        return (-2);
    }

    return 0;
}

void __destroy_media_processor()
{
    IGenericEngineControl *p_engine = g_media_processor_context.p_engine;
    DASSERT(p_engine);

    if (p_engine) {
        test_log("before p_engine->Destroy()\n");
        p_engine->Destroy();
        test_log("after p_engine->Destroy()\n");
    }

    g_media_processor_context.p_engine = NULL;
}

static TInt __replace_enter(TChar *p_buffer, TUint size)
{
    while (size) {
        if (('\n') == (*p_buffer)) {
            *p_buffer = 0x0;
            return 0;
        }
        p_buffer ++;
        size --;
    }

    test_loge("no enter found, should be error\n");

    return 1;
}

TInt mudec_test()
{
    TInt ret = 0;
    EECode err = EECode_OK;

    IGenericEngineControl *p_engine = NULL;
    TChar buffer_old[128] = {0};
    TChar buffer[128] = {0};
    TChar *p_buffer = buffer;
    TInt udec_running = 1;

    ret = mudec_build_process_pipelines();

    if (!ret) {
        ret = __start_process();
        if (!ret) {
            //process cmd line

            while (udec_running) {
                //add sleep to avoid affecting the performance
                usleep(100000);
                //memset(buffer, 0x0, sizeof(buffer));
                if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0) {
                    TInt errcode = errno;
                    if (errcode == EAGAIN) {
                        continue;
                    }
                    test_loge("read STDIN_FILENO fail, errcode %d\n", errcode);
                    continue;
                }

                test_log("[user cmd debug]: read input '%s'\n", buffer);

                if (buffer[0] == '\n') {
                    p_buffer = buffer_old;
                    test_log("repeat last cmd:\n\t\t%s\n", buffer_old);
                } else if (buffer[0] == 'l') {
                    test_log("show last cmd:\n\t\t%s\n", buffer_old);
                    continue;
                } else {

                    ret = __replace_enter(buffer, (128 - 1));
                    DASSERT(0 == ret);
                    if (ret) {
                        test_loge("no enter found\n");
                        continue;
                    }
                    p_buffer = buffer;

                    //record last cmd
                    strncpy(buffer_old, buffer, sizeof(buffer_old) - 1);
                    buffer_old[sizeof(buffer_old) - 1] = 0x0;
                }

                test_log("[user cmd debug]: '%s'\n", p_buffer);

                p_engine = g_media_processor_context.p_engine;
                switch (p_buffer[0]) {

                    case 'q':   // exit
                        test_log("[user cmd]: 'q', Quit\n");
                        udec_running = 0;
                        break;

                    case 'h':   // help
                        _mtest_print_version();
                        _mtest_print_usage();
                        _mtest_print_runtime_usage();
                        break;

                    case 'c': {
                            TUint id = 0;
                            if (!strncmp("ccjpeg:", p_buffer, strlen("ccjpeg:"))) {
                                TUint cap_index = 0;
                                SPlaybackCapture capture;
                                SVideoCodecInfo dec_info;
                                SVideoDisplayDeviceInfo vout_info;

                                if (2 == sscanf(p_buffer, "ccjpeg:%x %x", &id, &cap_index)) {
                                    if ((0xff != id) && (id >= (g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2))) {
                                        test_loge("dec id(%d) exceed max value\n", id);
                                        break;
                                    }

                                    if (0xff != id) {
                                        dec_info.dec_id = id;
                                        dec_info.check_field = EGenericEngineQuery_VideoCodecInfo;
                                        err = p_engine->GenericControl(EGenericEngineQuery_VideoCodecInfo, (void *)&dec_info);
                                        DASSERT_OK(err);
                                        if (EECode_OK != err) {
                                            test_loge("EGenericEngineQuery_VideoCodecInfo dec_id %d fail\n", id);
                                            break;
                                        }
                                    }

                                    vout_info.check_field = EGenericEngineQuery_VideoDisplayDeviceInfo;
                                    err = p_engine->GenericControl(EGenericEngineQuery_VideoDisplayDeviceInfo, (void *)&vout_info);
                                    DASSERT_OK(err);
                                    if (EECode_OK != err) {
                                        test_loge("EGenericEngineQuery_VideoDisplayDeviceInfo fail\n");
                                        break;
                                    }

                                    memset(&capture, 0x0, sizeof(SPlaybackCapture));
                                    capture.check_field = EGenericEngineConfigure_VideoPlayback_Capture;
                                    capture.capture_id = id;
                                    if (0xff != id) {
                                        capture.capture_coded = 1;
                                    } else {
                                        capture.capture_coded = 0;
                                    }
                                    capture.capture_screennail = 1;
                                    capture.capture_thumbnail = 1;

                                    capture.jpeg_quality = 50;
                                    capture.capture_file_index = cap_index;
                                    capture.customized_file_name = 0;

                                    if (0xff != id) {
                                        capture.coded_width = dec_info.pic_width;
                                        capture.coded_height = dec_info.pic_height;
                                    } else {
                                        capture.coded_width = vout_info.primary_vout_width / 2;
                                        capture.coded_height = vout_info.primary_vout_height / 2;
                                    }

                                    capture.screennail_width = vout_info.primary_vout_width;
                                    capture.screennail_height = vout_info.primary_vout_height;

                                    capture.thumbnail_width = 320;
                                    capture.thumbnail_height = 240;

                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_Capture, (void *)&capture);
                                    DASSERT_OK(err);
                                }
                            } else if (!strncmp("cmute", p_buffer, strlen("cmute"))) {
                                test_log("mute audio: EGenericEngineConfigure_AudioPlayback_Mute start\n");
                                err = p_engine->GenericControl(EGenericEngineConfigure_AudioPlayback_Mute, NULL);
                                if (DUnlikely(EECode_OK != err)) {
                                    test_loge("EGenericEngineConfigure_AudioPlayback_Mute fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                                    break;
                                }
                                test_log("mute audio: EGenericEngineConfigure_AudioPlayback_Mute end\n");
                            } else if (!strncmp("cumute", p_buffer, strlen("cumute"))) {
                                test_log("umute audio: EGenericEngineConfigure_AudioPlayback_UnMute start\n");
                                err = p_engine->GenericControl(EGenericEngineConfigure_AudioPlayback_UnMute, NULL);
                                if (DUnlikely(EECode_OK != err)) {
                                    test_loge("EGenericEngineConfigure_AudioPlayback_UnMute fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                                    break;
                                }
                                test_log("umute audio: EGenericEngineConfigure_AudioPlayback_UnMute end\n");
                            } else if (!strncmp("cselectaudio:", p_buffer, strlen("cselectaudio:"))) {
                                SGenericSelectAudioChannel audio_channel_index;
                                if (1 == sscanf(p_buffer, "cselectaudio:%d", &audio_channel_index.channel)) {
                                    test_log("'cselectaudio:%d' start\n", audio_channel_index.channel);
                                    audio_channel_index.check_field = EGenericEngineConfigure_AudioPlayback_SelectAudioSource;
                                    err = p_engine->GenericControl(EGenericEngineConfigure_AudioPlayback_SelectAudioSource, &audio_channel_index);
                                    if (DUnlikely(EECode_OK != err)) {
                                        test_loge("EGenericEngineConfigure_AudioPlayback_SelectAudioSource fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                                        break;
                                    }
                                    test_log("'cselectaudio:%d' end\n", audio_channel_index.channel);
                                } else {
                                    test_loge("'cselectaudio:%%d' should specify audio channel's index\n");
                                }
                            } else if (!strncmp("cz", p_buffer, strlen("cz"))) {//cz1,cz2,czi,czo
                                switch (p_buffer[2]) {
                                    case '1':
                                        test_log("[user cmd]: '%s' no supported in mtest now.\n", p_buffer);
                                        break;
                                    case '2': {
                                            TU16 input_w = 0, input_h = 0, input_center_x = 0, input_center_y = 0;
                                            SPlaybackZoom zoom;
                                            SVideoCodecInfo dec_info;
                                            if (':' == p_buffer[3]) {
                                                if (5 == sscanf(p_buffer, "cz2:%d,%hd,%hd,%hd,%hd", &id, &input_w, &input_h, &input_center_x, &input_center_y)) {

                                                    SVideoPostPDisplayMWMapping mapping;
                                                    if (id >= (g_mtest_dsp_dec_instances_number_1 > g_mtest_dsp_dec_instances_number_2 ? g_mtest_dsp_dec_instances_number_1 : g_mtest_dsp_dec_instances_number_2)) {
                                                        test_loge("render id(%d) exceed max value\n", id);
                                                        break;
                                                    }
                                                    mapping.render_id = id;
                                                    mapping.check_field = EGenericEngineQuery_VideoDisplayRender;
                                                    err = p_engine->GenericControl(EGenericEngineQuery_VideoDisplayRender, (void *)&mapping);
                                                    if (EECode_OK != err) {
                                                        test_loge("EGenericEngineQuery_VideoDisplayRender fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                                                        break;
                                                    }
                                                    if (DINVALID_VALUE_TAG == mapping.decoder_id) {
                                                        test_loge("EGenericEngineQuery_VideoDisplayRender fail, bad render id %d, ret %d, %s\n", mapping.render_id, err, gfGetErrorCodeString(err));
                                                        break;
                                                    }
                                                    test_log("EGenericEngineQuery_VideoDisplayRender done, render_id=%u, decoder_id=%u\n",
                                                             mapping.render_id, mapping.decoder_id);

                                                    dec_info.dec_id = mapping.decoder_id;
                                                    dec_info.check_field = EGenericEngineQuery_VideoCodecInfo;
                                                    err = p_engine->GenericControl(EGenericEngineQuery_VideoCodecInfo, (void *)&dec_info);
                                                    DASSERT_OK(err);
                                                    if (EECode_OK != err) {
                                                        test_loge("EGenericEngineQuery_VideoCodecInfo dec_id %d fail\n", dec_info.dec_id);
                                                        break;
                                                    }

                                                    test_log("EGenericEngineQuery_VideoCodecInfo done. dec_id=%d, pic_width=%u, pic_height=%u\n", dec_info.dec_id, dec_info.pic_width, dec_info.pic_height);
                                                    if (input_w > dec_info.pic_width
                                                            || input_h > dec_info.pic_height
                                                            || (input_center_x + (input_w / 2)) > dec_info.pic_width
                                                            || (input_center_x - (input_w / 2)) < 0
                                                            || (input_center_y + (input_h / 2)) > dec_info.pic_height
                                                            || (input_center_y - (input_h / 2)) < 0) {
                                                        test_loge("zoom parameters excced expect range.\n");
                                                        break;
                                                    }

                                                    zoom.check_field = EGenericEngineConfigure_VideoPlayback_Zoom;
                                                    zoom.zoom_mode = 0;
                                                    zoom.render_id = mapping.render_id;
                                                    zoom.input_center_x = input_center_x;
                                                    zoom.input_center_y = input_center_y;
                                                    zoom.input_width = input_w;
                                                    zoom.input_height = input_h;

                                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_Zoom, (void *)&zoom);
                                                    DASSERT_OK(err);
                                                } else {
                                                    test_loge("command format invalid, pls input 'cz2:render_id,input_with,input_height,input_center_x,input_center_y', all paras are decimal format.\n");
                                                }
                                            } else if ('w' == p_buffer[3]) {
                                                if (5 == sscanf(p_buffer, "cz2w:%d,%hd,%hd,%hd,%hd", &id, &input_w, &input_h, &input_center_x, &input_center_y)) {

                                                    SVideoPostPDisplayMWMapping mapping;
                                                    if (id >= (g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2)) {
                                                        test_loge("window id(%d) exceed max value\n", id);
                                                        break;
                                                    }
                                                    mapping.window_id = id;
                                                    mapping.check_field = EGenericEngineQuery_VideoDisplayWindow;
                                                    err = p_engine->GenericControl(EGenericEngineQuery_VideoDisplayWindow, (void *)&mapping);
                                                    if (EECode_OK != err) {
                                                        test_loge("EGenericEngineQuery_VideoDisplayWindow fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                                                        break;
                                                    }
                                                    if (DINVALID_VALUE_TAG == mapping.decoder_id) {
                                                        test_loge("EGenericEngineQuery_VideoDisplayWindow fail, bad window id %d, ret %d, %s\n", mapping.window_id, err, gfGetErrorCodeString(err));
                                                        break;
                                                    }
                                                    test_log("EGenericEngineQuery_VideoDisplayWindow done, window_id=%d, decoder_id=%u, render_id=%u\n",
                                                             mapping.window_id, mapping.decoder_id, mapping.render_id);

                                                    dec_info.dec_id = mapping.decoder_id;
                                                    dec_info.check_field = EGenericEngineQuery_VideoCodecInfo;
                                                    err = p_engine->GenericControl(EGenericEngineQuery_VideoCodecInfo, (void *)&dec_info);
                                                    DASSERT_OK(err);
                                                    if (EECode_OK != err) {
                                                        test_loge("EGenericEngineQuery_VideoCodecInfo dec_id %d fail\n", dec_info.dec_id);
                                                        break;
                                                    }

                                                    test_log("EGenericEngineQuery_VideoCodecInfo done. dec_id=%d, pic_width=%u, pic_height=%u\n", dec_info.dec_id, dec_info.pic_width, dec_info.pic_height);
                                                    if (input_w > dec_info.pic_width
                                                            || input_h > dec_info.pic_height
                                                            || (input_center_x + (input_w / 2)) > dec_info.pic_width
                                                            || (input_center_x - (input_w / 2)) < 0
                                                            || (input_center_y + (input_h / 2)) > dec_info.pic_height
                                                            || (input_center_y - (input_h / 2)) < 0) {
                                                        test_loge("zoom parameters excced expect range.\n");
                                                        break;
                                                    }

                                                    zoom.check_field = EGenericEngineConfigure_VideoPlayback_Zoom;
                                                    zoom.zoom_mode = 0;
                                                    zoom.render_id = mapping.render_id;
                                                    zoom.input_center_x = input_center_x;
                                                    zoom.input_center_y = input_center_y;
                                                    zoom.input_width = input_w;
                                                    zoom.input_height = input_h;

                                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_Zoom, (void *)&zoom);
                                                    DASSERT_OK(err);
                                                } else {
                                                    test_loge("command format invalid, pls input 'cz2w:window_id,input_with,input_height,input_center_x,input_center_y', all paras are decimal format.\n");
                                                }
                                            } else {
                                                test_loge("command format invalid, pls input 'cz2r......' or 'cz2w......'.\n");
                                            }
                                        }
                                        break;
                                    case 'i':
                                        test_log("[user cmd]: '%s' no supported in mtest now.\n", p_buffer);
                                        break;
                                    case 'o':
                                        test_log("[user cmd]: '%s' no supported in mtest now.\n", p_buffer);
                                        break;
                                    default:
                                        test_log("[user cmd]: '%s' invalid for zoom operation.\n", p_buffer);
                                        break;
                                }
                            } else if (!strncmp("cs", p_buffer, strlen("cs"))) {
                                TU32 id, offset_x, offset_y, width, height;
                                if (5 == sscanf(buffer, "cs:%u %u %u %u %u", &id, &offset_x, &offset_y, &width, &height)) {
                                    SGenericVideoPostPConfiguration ppinfo;
                                    SVideoPostPConfiguration info;
                                    /*if(EDisplayLayout_TelePresence != mLayoutType){
                                        test_loge("current layout type(%d) is not EDisplayLayout_TelePresence, changing window position is not suported!", mLayoutType);
                                        break;
                                    }

                                    if(0 == id){
                                        test_loge("window 0 is the full screen window in EDisplayLayout_TelePresence, changing window position is not supported!");
                                        break;
                                    }*/

                                    //get first
                                    ppinfo.check_field = EGenericEngineConfigure_VideoDisplay_PostPConfiguration;
                                    ppinfo.set_or_get = 0;
                                    ppinfo.p_video_postp_info = &info;
                                    memset(&info, 0, sizeof(SVideoPostPConfiguration));

                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoDisplay_PostPConfiguration, &ppinfo);
                                    DASSERT_OK(err);

                                    for (TInt i = 0; i < info.cur_window_number; i++) {
                                        if ((TU8)id == info.window[i].win_config_id) { //mdf certain win config
                                            info.window[i].target_win_offset_x = offset_x;
                                            info.window[i].target_win_offset_y = offset_y;
                                            info.window[i].target_win_width = width;
                                            info.window[i].target_win_height = height;

                                            test_log("target window %u will be configed to size %ux%u, offset %ux%u\n", id, info.window[i].target_win_width, info.window[i].target_win_height, info.window[i].target_win_offset_x, info.window[i].target_win_offset_y);

                                            //set pp
                                            ppinfo.set_or_get = 1;
                                            info.set_config_direct_to_dsp = 1;
                                            err = p_engine->GenericControl(EGenericEngineConfigure_VideoDisplay_PostPConfiguration, &ppinfo);
                                            DASSERT_OK(err);
                                        }
                                    }
                                } else {
                                    test_loge("command format invalid, pls input 'cs:window_index offset_x offset_y width height.\n");
                                }
                            } else if (!strncmp("cp", p_buffer, strlen("cp"))) {//fast-forward, fast-backward
                                TU16 source_index = 0, speed = 0, speed_frac = 0, feeding_rule; //, possible_speed=0
                                switch (p_buffer[2]) {
                                    case 'f':
                                        if (4 == sscanf(buffer, "cpf:%hu %hu.%hu %hu", &source_index, &speed, &speed_frac, &feeding_rule)) {
                                            if (source_index >= g_media_processor_context.source_url_count_in_group[g_cur_playback_url_group]
                                                    || (feeding_rule != DecoderFeedingRule_AllFrames && feeding_rule != DecoderFeedingRule_IOnly)) {
                                                test_logw("BAD params, source_index %hu, feeding_rule=%hu\n", source_index, feeding_rule);
                                                break;
                                            }
                                            /*                                            possible_speed = max_possiable_speed(p_engine, source_index);
                                                                                        test_log("specify playback speed %hu.%hu, possible_speed %hu\n", speed, speed_frac, possible_speed);
                                                                                        if (speed > possible_speed) {
                                                                                            test_log("speed(%hu.%hu) > max_speed %hu, use I only mode\n", speed, speed_frac, possible_speed);
                                                                                            feeding_rule = DecoderFeedingRule_IOnly;
                                                                                        } else {
                                                                                            test_log("speed(%hu.%hu) <= max_speed %hu, use all frames mode\n", speed, speed_frac, possible_speed);
                                                                                            feeding_rule = DecoderFeedingRule_AllFrames;
                                                                                        }*/
                                            err = p_engine->PBSpeed(g_media_processor_context.playback_pipelines[source_index], speed, speed_frac, 0, (DecoderFeedingRule)feeding_rule);
                                            DASSERT_OK(err);
                                        } else {
                                            test_loge("command format invalid, pls input 'cpf:source_index speed.speed_frac feeding_rule, all paras are decimal format, feeding_rule only support 1-all frames and 3-IOnly now.\n");
                                        }
                                        break;
                                    case 'b':
                                        if (4 == sscanf(buffer, "cpb:%hu %hu.%hu %hu", &source_index, &speed, &speed_frac, &feeding_rule)) {
                                            if (source_index >= g_media_processor_context.source_url_count_in_group[g_cur_playback_url_group]
                                                    || (feeding_rule != DecoderFeedingRule_IOnly)) {
                                                test_logw("BAD params, source_index %hu, feeding_rule=%hu\n", source_index, feeding_rule);
                                                break;
                                            }
                                            err = p_engine->PBSpeed(g_media_processor_context.playback_pipelines[source_index], speed, speed_frac, 1, (DecoderFeedingRule)feeding_rule);
                                            DASSERT_OK(err);
                                        } else {
                                            test_loge("command format invalid, pls input 'cpb:source_index speed.speed_frac feeding_rule, all paras are decimal format, feeding_rule only support 3-IOnly now.\n");
                                        }
                                        break;
                                    case 'c':
                                        if (1 == sscanf(buffer, "cpc:%hu", &source_index)) {
                                            if (source_index >= g_media_processor_context.source_url_count_in_group[g_cur_playback_url_group]) {
                                                test_logw("BAD params, source_index %hu\n", source_index);
                                                break;
                                            }
                                            err = p_engine->PBSpeed(g_media_processor_context.playback_pipelines[source_index], 1, 0, 0, DecoderFeedingRule_AllFrames);
                                            DASSERT_OK(err);
                                        } else {
                                            test_loge("command format invalid, pls input 'cpc:source_index, all paras are decimal format.\n");
                                        }
                                        break;
                                    default:
                                        test_log("[user cmd]: '%s' invalid for playback speed updating.\n", p_buffer);
                                        break;
                                }
                            } else if (!strncmp("csuspenddsp", p_buffer, strlen("csuspenddsp"))) {
                                test_log("[user cmd]: csuspenddsp.\n");
                                err = p_engine->GenericControl(EGenericEngineConfigure_DSP_Suspend, (void *)1);
                                DASSERT_OK(err);
                            } else if (!strncmp("cresumedsp", p_buffer, strlen("cresumedsp"))) {
                                test_log("[user cmd]: cresumedsp.\n");
                                err = p_engine->GenericControl(EGenericEngineConfigure_DSP_Resume, (void *)1);
                                DASSERT_OK(err);
                            } else if (!strncmp("creconnectserver", p_buffer, strlen("creconnectserver"))) {
                                TUint reconnect_index = 0;
                                if (1 == sscanf(p_buffer, "creconnectserver %d", &reconnect_index)) {
                                    SGenericReconnectStreamingServer reconnect;
                                    memset(&reconnect, 0x0, sizeof(SGenericReconnectStreamingServer));
                                    reconnect.check_field = EGenericEngineConfigure_ReConnectServer;
                                    reconnect.index = reconnect_index;
                                    if (0xff == reconnect_index) {
                                        reconnect.all_demuxer = 1;
                                    }
                                    test_log("[user cmd]: creconnectserver %d, reconnect.all_demuxer %d.\n", reconnect_index, reconnect.all_demuxer);
                                    err = p_engine->GenericControl(EGenericEngineConfigure_ReConnectServer, (void *)&reconnect);
                                    DASSERT_OK(err);
                                }
                            } else if (!strncmp("creconnectall", p_buffer, strlen("creconnectall"))) {
                                SGenericReconnectStreamingServer reconnect;
                                memset(&reconnect, 0x0, sizeof(SGenericReconnectStreamingServer));
                                reconnect.check_field = EGenericEngineConfigure_ReConnectServer;
                                reconnect.all_demuxer = 1;
                                test_log("[user cmd]: creconnectall\n");
                                err = p_engine->GenericControl(EGenericEngineConfigure_ReConnectServer, (void *)&reconnect);
                                DASSERT_OK(err);
                            }

                        }
                        break;

                    case 'p': {
                            test_log("[user cmd]: '%s'\n", p_buffer);
                            if (!strncmp("pall", p_buffer, strlen("pall"))) {
                                test_log("print all\n");
                                p_engine->PrintCurrentStatus(0, DPrintFlagBit_dataPipeline);
                            } else if (!strncmp("plog", p_buffer, strlen("plog"))) {
                                test_log("print log configure\n");
                                p_engine->PrintCurrentStatus(0, DPrintFlagBit_logConfigure);
                            } else if (!strncmp("pdsp", p_buffer, strlen("pdsp"))) {
                                TU32 id = 0;
                                if (1 == sscanf(p_buffer, "pdsp:%u", &id)) {

                                    if (id >= (g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2)) {
                                        test_loge("window id(%d) exceed max value\n", id);
                                        break;
                                    }

                                    err = p_engine->GenericControl(EGenericEngineQuery_VideoDSPStatus, (void *)&id);
                                    DASSERT_OK(err);
                                    if (EECode_OK != err) {
                                        test_loge("EGenericEngineQuery_VideoDSPStatus id %d fail\n", id);
                                        break;
                                    }
                                    test_log("EGenericEngineQuery_VideoDSPStatus done. udec(%d) info:\n", id);
                                } else {
                                    test_loge("command format invalid, pls input 'pdsp:udec_id', all paras are decimal format.\n");
                                }
                            }
                        }
                        break;

                    case ' ': {
                            TUint id = 0;
                            SVideoPostPDisplayTrickPlay trickplay;

                            test_log("[user cmd]: '%s'\n", p_buffer);
                            if (1 == sscanf(p_buffer, " %d", &id)) {
                                if (id > (g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2)) {
                                    test_loge("dec id(%d) exceed max value\n", id);
                                    break;
                                }
                                if (0 == g_media_processor_context.dsp_dec_trickplay[id]) {
                                    test_log("pause dec %d\n", id);
                                    trickplay.check_field = EGenericEngineConfigure_VideoPlayback_UDECPause;
                                    trickplay.id = id;
                                    p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_UDECPause, (void *)&trickplay);
                                    g_media_processor_context.dsp_dec_trickplay[id] = 1;
                                } else if (1 == g_media_processor_context.dsp_dec_trickplay[id]) {
                                    test_log("resume dec %d\n", id);
                                    trickplay.check_field = EGenericEngineConfigure_VideoPlayback_UDECResume;
                                    trickplay.id = id;
                                    p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_UDECResume, (void *)&trickplay);
                                    g_media_processor_context.dsp_dec_trickplay[id] = 0;
                                } else if (2 == g_media_processor_context.dsp_dec_trickplay[id]) {
                                    test_log("resume dec %d from step mode\n", id);
                                    trickplay.check_field = EGenericEngineConfigure_VideoPlayback_UDECResume;
                                    trickplay.id = id;
                                    p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_UDECResume, (void *)&trickplay);
                                    g_media_processor_context.dsp_dec_trickplay[id] = 0;
                                } else {
                                    test_loge("BAD trickplay status %d for dec %d\n", g_media_processor_context.dsp_dec_trickplay[id], id);
                                }
                            } else if (1 == sscanf(p_buffer, " w%d", &id)) {
                                SVideoPostPDisplayMWMapping mapping;
                                if (id > (g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2)) {
                                    test_loge("window id(%d) exceed max value\n", id);
                                    break;
                                }
                                mapping.window_id = id;
                                mapping.check_field = EGenericEngineQuery_VideoDisplayWindow;
                                err = p_engine->GenericControl(EGenericEngineQuery_VideoDisplayWindow, (void *)&mapping);
                                if (EECode_OK != err) {
                                    test_loge("EGenericEngineQuery_VideoDisplayWindow fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                                    break;
                                }
                                if (DINVALID_VALUE_TAG == mapping.decoder_id) {
                                    test_loge("EGenericEngineQuery_VideoDisplayWindow fail, bad window id %d, ret %d, %s\n", id, err, gfGetErrorCodeString(err));
                                    break;
                                }

                                test_log("[trick cmd for window(%d)]: dec id %d\n", id, mapping.decoder_id);
                                id = mapping.decoder_id;

                                if (0 == g_media_processor_context.dsp_dec_trickplay[id]) {
                                    test_log("pause dec %d\n", id);
                                    trickplay.check_field = EGenericEngineConfigure_VideoPlayback_UDECPause;
                                    trickplay.id = id;
                                    p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_UDECPause, (void *)&trickplay);
                                    g_media_processor_context.dsp_dec_trickplay[id] = 1;
                                } else if (1 == g_media_processor_context.dsp_dec_trickplay[id]) {
                                    test_log("resume dec %d\n", id);
                                    trickplay.check_field = EGenericEngineConfigure_VideoPlayback_UDECResume;
                                    trickplay.id = id;
                                    p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_UDECResume, (void *)&trickplay);
                                    g_media_processor_context.dsp_dec_trickplay[id] = 0;
                                } else if (2 == g_media_processor_context.dsp_dec_trickplay[id]) {
                                    test_log("resume dec %d from step mode\n", id);
                                    trickplay.check_field = EGenericEngineConfigure_VideoPlayback_UDECResume;
                                    trickplay.id = id;
                                    p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_UDECResume, (void *)&trickplay);
                                    g_media_processor_context.dsp_dec_trickplay[id] = 0;
                                } else {
                                    test_loge("BAD trickplay status %d for dec %d\n", g_media_processor_context.dsp_dec_trickplay[id], id);
                                }
                            }
                        }
                        break;

                    case 's': {
                            TUint id = 0;
                            SVideoPostPDisplayTrickPlay trickplay;

                            test_log("[user cmd]: '%s'\n", p_buffer);
                            if (1 == sscanf(p_buffer, "s%d", &id)) {
                                if (id > (g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2)) {
                                    test_loge("dec id(%d) exceed max value\n", id);
                                    break;
                                }

                                test_log("step play dec %d\n", id);
                                trickplay.check_field = EGenericEngineConfigure_VideoPlayback_UDECStepPlay;
                                trickplay.id = id;
                                p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_UDECStepPlay, (void *)&trickplay);
                                g_media_processor_context.dsp_dec_trickplay[id] = 2;
                            } else if (1 == sscanf(p_buffer, "sw%d", &id)) {
                                SVideoPostPDisplayMWMapping mapping;
                                if (id > (g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2)) {
                                    test_loge("window id(%d) exceed max value\n", id);
                                    break;
                                }
                                mapping.window_id = id;
                                mapping.check_field = EGenericEngineQuery_VideoDisplayWindow;
                                err = p_engine->GenericControl(EGenericEngineQuery_VideoDisplayWindow, (void *)&mapping);
                                if (EECode_OK != err) {
                                    test_loge("EGenericEngineQuery_VideoDisplayWindow fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                                    break;
                                }
                                if (DINVALID_VALUE_TAG == mapping.decoder_id) {
                                    test_loge("EGenericEngineQuery_VideoDisplayWindow fail, bad window id %d, ret %d, %s\n", id, err, gfGetErrorCodeString(err));
                                    break;
                                }

                                test_log("[trick cmd for window(%d)]: dec id %d\n", id, mapping.decoder_id);
                                id = mapping.decoder_id;

                                test_log("step play dec %d\n", id);
                                trickplay.check_field = EGenericEngineConfigure_VideoPlayback_UDECStepPlay;
                                trickplay.id = id;
                                p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_UDECStepPlay, (void *)&trickplay);
                                g_media_processor_context.dsp_dec_trickplay[id] = 2;
                            }
                        }
                        break;

                    case '-':
                        if ('-' == p_buffer[1]) {
                            test_log("[user cmd]: '%s'\n", p_buffer);
                            if (!strncmp("--hd", p_buffer, strlen("--hd"))) {
                                TUint id = 0;
                                if (1 == sscanf(p_buffer, "--hd %u", &id)) {
                                    if (DUnlikely(id >= g_mtest_dsp_dec_instances_number_1)) {
                                        test_loge("[user cmd param error]: hd index(%d) excced max value, should in 0 ~ %d\n", id, g_mtest_dsp_dec_instances_number_1 - 1);
                                    } else {
                                        SVideoPostPDisplayLayer layer;
                                        layer.check_field = EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer;
                                        layer.request_display_layer = 1;
                                        layer.sd_window_index_for_hd = id;
                                        layer.flush_udec = 0;
                                        test_log("[user cmd parse]: '--hd %%d' ---> request layer 1, id = %d\n", id);
                                        err = p_engine->GenericControl(EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer, &layer);
                                        if (EECode_OK == err) {
                                            test_log("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer layer 1, success!\n");
                                            gb_mtest_is_single_view = 1;
                                        } else {
                                            test_loge("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer layer 1, fail, err %s!\n", gfGetErrorCodeString(err));
                                        }
                                    }
                                } else if (1 == sscanf(p_buffer, "--hdf %u", &id)) {
                                    if (DUnlikely(id >= g_mtest_dsp_dec_instances_number_1)) {
                                        test_loge("[user cmd param error]: hd index(%d) excced max value, should in 0 ~ %d\n", id, g_mtest_dsp_dec_instances_number_1 - 1);
                                    } else {
                                        SVideoPostPDisplayLayer layer;
                                        layer.check_field = EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer;
                                        layer.request_display_layer = 1;
                                        layer.sd_window_index_for_hd = id;
                                        layer.flush_udec = 1;
                                        test_log("[user cmd parse]: '--hdf %%d' ---> request layer 1, id = %d\n", id);
                                        err = p_engine->GenericControl(EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer, &layer);
                                        if (EECode_OK == err) {
                                            test_log("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer layer 1, success!\n");
                                            gb_mtest_is_single_view = 1;
                                        } else {
                                            test_loge("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer layer 1, fail, err %s!\n", gfGetErrorCodeString(err));
                                        }
                                    }
                                } else {
                                    test_loge("[user cmd example]: --hd need followed by according SD window index %%u\n");
                                }
                            } else if (!strncmp("--sd", p_buffer, strlen("--sd"))) {
                                SVideoPostPDisplayLayer layer;
                                if ('f' == p_buffer[4]) {
                                    test_log("[user cmd parse]: '--sdf' ---> request layer 0\n");
                                    layer.flush_udec = 1;
                                } else {
                                    test_log("[user cmd parse]: '--sd' ---> request layer 0\n");
                                    layer.flush_udec = 0;
                                }
                                layer.check_field = EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer;
                                layer.request_display_layer = 0;

                                err = p_engine->GenericControl(EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer, &layer);
                                if (EECode_OK == err) {
                                    test_log("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer layer 0, success!\n");
                                    gb_mtest_is_single_view = 0;
                                } else {
                                    test_loge("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer layer 0, fail, err %s!\n", gfGetErrorCodeString(err));
                                }
                            } else if (!strncmp("--layout", p_buffer, strlen("--layout"))) {
                                SVideoPostPDisplayLayout layout;
                                TUint layout_type = 0, isWindow0HD = 0;
                                TUint dsp_instance_number = g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2;

                                if (2 == sscanf(p_buffer, "--layout %d %d", &layout_type, &isWindow0HD)) {
                                    test_log("[user cmd parse]: '--layout' ---> request layout type %d, window 0 show %s\n", layout_type, isWindow0HD ? "HD" : "SD");
                                    if (EDisplayLayout_TelePresence != layout_type) {
                                        test_loge("specify source of window 0 only allowed in tele-presence view now, please switch to tele-presence view first.\n");
                                        break;
                                    }
                                } else if (1 == sscanf(p_buffer, "--layout %d", &layout_type)) {
                                    test_log("[user cmd parse]: '--layout' ---> request layout type %d\n", layout_type);
                                } else {
                                    test_loge("[user cmd example]: --layout need followed by layout type and isWindow0HD %%d %%d,or layout type %%d)\n");
                                    break;
                                }
                                if (1 == gb_mtest_is_single_view) {
                                    test_loge("change layout at single view is not allowed, please switch to multiple view first, (press '--sdf' or '--sd')\n");
                                    break;
                                }
                                layout.check_field = EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayout;
                                layout.is_window0_HD = (TU8)isWindow0HD;
                                layout.layer_number = g_mtest_dsp_dec_layer;
                                layout.layer_layout_type[0] = layout_type;
                                layout.layer_layout_type[1] = EDisplayLayout_Rectangle;
                                layout.layer_window_number[0] = g_mtest_dsp_dec_instances_number_1;
                                layout.layer_window_number[1] = g_mtest_dsp_dec_instances_number_2;

                                layout.total_decoder_number = dsp_instance_number;
                                layout.total_render_number = dsp_instance_number;
                                layout.total_window_number = dsp_instance_number;

                                layout.cur_decoder_number = g_mtest_dsp_dec_instances_number_1;
                                layout.cur_render_number = g_mtest_dsp_dec_instances_number_1;
                                layout.cur_window_number = dsp_instance_number;

                                layout.cur_decoder_start_index = 0;
                                layout.cur_render_start_index = 0;
                                layout.cur_window_start_index = 0;

                                err = p_engine->GenericControl(EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayout, &layout);
                                if (EECode_OK == err) {
                                    test_log("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayout %d, window 0 show %s, success!\n", layout_type, isWindow0HD ? "HD" : "SD");
                                } else {
                                    test_loge("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayout %d, window 0 show %s, fail, err %s!\n", layout_type, isWindow0HD ? "HD" : "SD", gfGetErrorCodeString(err));
                                }
                            } else if (!strncmp("--fulllayout", p_buffer, strlen("--fulllayout"))) {
                                SVideoPostPDisplayLayout layout;
                                TUint layout_type = 0;
                                TUint dsp_instance_number = g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2;

                                if (1 == sscanf(p_buffer, "--fulllayout %d", &layout_type)) {
                                    test_log("[user cmd parse]: '--fulllayout' ---> request layout type %d\n", layout_type);
                                    layout.check_field = EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayout;
                                    layout.layer_number = 1;
                                    layout.layer_layout_type[0] = layout_type;
                                    layout.layer_layout_type[1] = EDisplayLayout_Invalid;
                                    layout.layer_window_number[0] = g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2;
                                    layout.layer_window_number[1] = g_mtest_dsp_dec_instances_number_2;

                                    layout.total_decoder_number = dsp_instance_number;
                                    layout.total_render_number = dsp_instance_number;
                                    layout.total_window_number = dsp_instance_number;

                                    layout.cur_decoder_number = dsp_instance_number;
                                    layout.cur_render_number = dsp_instance_number;
                                    layout.cur_window_number = dsp_instance_number;

                                    layout.cur_decoder_start_index = 0;
                                    layout.cur_render_start_index = 0;
                                    layout.cur_window_start_index = 0;

                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayout, &layout);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayout %d, success!\n", layout_type);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayout %d, fail, err %s!\n", layout_type, gfGetErrorCodeString(err));
                                    }
                                }
                            } else if (!strncmp("--highlight", p_buffer, strlen("--highlight"))) {
                                TU16 highlighten_window_width = 0, highlighten_window_height = 0;
                                if (2 == sscanf(p_buffer, "--highlight %hu %hu", &highlighten_window_width, &highlighten_window_height)) {
                                    test_log("[user cmd parse]: '--highlight' ---> request highlighten window size %hux%hu\n", highlighten_window_width, highlighten_window_height);
                                    SVideoPostPDisplayHighLightenWindowSize size;
                                    size.check_field = EGenericEngineConfigure_VideoDisplay_UpdateHighLightenWindowSize;
                                    size.window_width = highlighten_window_width;
                                    size.window_height = highlighten_window_height;

                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoDisplay_UpdateHighLightenWindowSize, &size);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateHighLightenWindowSize to %hux%hu, success!\n", highlighten_window_width, highlighten_window_height);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateHighLightenWindowSize to %hux%hu, fail, err %s!\n", highlighten_window_width, highlighten_window_height, gfGetErrorCodeString(err));
                                    }
                                } else {
                                    test_loge("[user cmd example]: --highlight need followed by highlighten window width height type %%hu %%hu\n");
                                }
                            } else if (!strncmp("--voutmask", p_buffer, strlen("--voutmask"))) {
                                SVideoPostPDisplayMask mask;
                                TUint new_vout_mask = DCAL_BITMASK(EDisplayDevice_HDMI);

                                if (1 == sscanf(p_buffer, "--voutmask %x", &new_vout_mask)) {
                                    test_log("[user cmd parse]: '--voutmask %x'\n", new_vout_mask);
                                    mask.check_field = EGenericEngineConfigure_VideoDisplay_UpdateDisplayDeviceMask;
                                    mask.new_display_vout_mask = new_vout_mask;

                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoDisplay_UpdateDisplayDeviceMask, &mask);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayDeviceMask %d, success!\n", new_vout_mask);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayDeviceMask %d, fail, err %s!\n", new_vout_mask, gfGetErrorCodeString(err));
                                    }
                                }
                            } else if (!strncmp("--layer", p_buffer, strlen("--layer"))) {
                                SVideoPostPDisplayLayer layer;
                                TUint new_display_layer = 0;

                                if (1 == sscanf(p_buffer, "--layer %d", &new_display_layer)) {
                                    test_log("[user cmd parse]: '--layer %d'\n", new_display_layer);
                                    layer.check_field = EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer;
                                    layer.request_display_layer = new_display_layer;

                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer, &layer);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer %d, success!\n", new_display_layer);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer %d, fail, err %s!\n", new_display_layer, gfGetErrorCodeString(err));
                                    }
                                }
                            } else if (!strncmp("--loopmode", p_buffer, strlen("--loopmode"))) {
                                SPipeLineLoopMode loop_mode;
                                TUint new_loop_mode = 0;
                                TU16 source_index = 0;

                                if (2 == sscanf(p_buffer, "--loopmode %hu %u", &source_index, &new_loop_mode)) {
                                    test_log("[user cmd parse]: '--loopmode %hu %u\n", source_index, new_loop_mode);

                                    if (source_index >= g_media_processor_context.source_url_count_in_group[g_cur_playback_url_group]) {
                                        test_logw("BAD params, source_index %hu\n", source_index);
                                        break;
                                    }

                                    loop_mode.check_field = EGenericEngineConfigure_VideoPlayback_LoopMode;
                                    loop_mode.pb_pipeline = g_media_processor_context.playback_pipelines[source_index];
                                    loop_mode.loop_mode = new_loop_mode;

                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_LoopMode, &loop_mode);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoPlayback_LoopMode source %hu mode %u, success!\n", source_index, new_loop_mode);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoPlayback_LoopMode source %hu mode %u, fail, err %s!\n", source_index, new_loop_mode, gfGetErrorCodeString(err));
                                    }
                                } else {
                                    test_loge("[user cmd example]: --loopmode need followed by source index and loop mode as %%hu %%u, 0-no loop, 1-single source loop(default), 2-all sources loop\n");
                                }
                            } else if (!strncmp("--vencparas", p_buffer, strlen("--vencparas"))) {
                                SGenericVideoEncoderParam videoEncParas;
                                TGenericID id;
                                TUint bitrate, framerate, demandIDR, gop_M, gop_N, gop_idr_interval, gop_structure;

                                if (4 == sscanf(p_buffer, "--vencparas %u %u %u %u %u %u %u %u", &id, &bitrate, &framerate, &demandIDR, &gop_M, &gop_N, &gop_idr_interval, &gop_structure)) {
                                    test_log("[user cmd parse]: '--vencparas %u %u %u %u %u %u %u %u\n", id, bitrate, framerate, demandIDR, gop_M, gop_N, gop_idr_interval, gop_structure);

                                    //get current paras first
                                    videoEncParas.check_field = EGenericEngineConfigure_VideoEncoder_Params;
                                    videoEncParas.set_or_get = 0;
                                    memset(&videoEncParas.video_params, 0, sizeof(SVideoEncoderParam));

                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoEncoder_Params, &videoEncParas);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoEncoder_Params get, success! bitrate=%u, framerate=%f, demandIDR=%u, gop_M=%u, gop_N=%u, gop_idr_interval=%u, gop_structure=%u\n",
                                                 videoEncParas.video_params.bitrate,
                                                 videoEncParas.video_params.framerate,
                                                 videoEncParas.video_params.demand_idr_strategy,
                                                 videoEncParas.video_params.gop_M,
                                                 videoEncParas.video_params.gop_N,
                                                 videoEncParas.video_params.gop_idr_interval,
                                                 videoEncParas.video_params.gop_structure);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoEncoder_Params get, fail, err %s!\n", gfGetErrorCodeString(err));
                                        break;
                                    }
                                    //set modified paras
                                    videoEncParas.set_or_get = 1;
                                    videoEncParas.video_params.bitrate = bitrate;
                                    videoEncParas.video_params.framerate = framerate;
                                    videoEncParas.video_params.demand_idr_strategy = demandIDR;
                                    videoEncParas.video_params.gop_M = gop_M;
                                    videoEncParas.video_params.gop_N = gop_N;
                                    videoEncParas.video_params.gop_idr_interval = gop_idr_interval;
                                    videoEncParas.video_params.gop_structure = gop_structure;
                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoEncoder_Params, &videoEncParas);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoEncoder_Params set, success! bitrate=%u, framerate=%f, demandIDR=%u, gop_M=%u, gop_N=%u, gop_idr_interval=%u, gop_structure=%u\n",
                                                 videoEncParas.video_params.bitrate,
                                                 videoEncParas.video_params.framerate,
                                                 videoEncParas.video_params.demand_idr_strategy,
                                                 videoEncParas.video_params.gop_M,
                                                 videoEncParas.video_params.gop_N,
                                                 videoEncParas.video_params.gop_idr_interval,
                                                 videoEncParas.video_params.gop_structure);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoEncoder_Params set, fail, err %s!\n", gfGetErrorCodeString(err));
                                    }
                                } else {
                                    test_loge("[user cmd example]: --vencparas need followed by encoder generic_id, bitrate, framerate, demandIDR, gop_M, gop_N, gop_idr_interval and gop_structure as %%u %%u %%u %%u %%u %%u %%u %%u\n");
                                }
                            } else if (!strncmp("--vencbitrate", p_buffer, strlen("--vencbitrate"))) {
                                SGenericVideoEncoderParam videoEncParas;
                                TGenericID id;
                                TUint bitrate;

                                if (2 == sscanf(p_buffer, "--vencbitrate %u %u", &id, &bitrate)) {
                                    test_log("[user cmd parse]: '--vencbitrate %u %u\n", id, bitrate);

                                    //get current paras first
                                    videoEncParas.check_field = EGenericEngineConfigure_VideoEncoder_Bitrate;
                                    videoEncParas.set_or_get = 0;
                                    memset(&videoEncParas.video_params, 0, sizeof(SVideoEncoderParam));

                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoEncoder_Bitrate, &videoEncParas);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoEncoder_Bitrate get, success! bitrate=%u\n", videoEncParas.video_params.bitrate);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoEncoder_Bitrate get, fail, err %s!\n", gfGetErrorCodeString(err));
                                        break;
                                    }
                                    //set modified paras
                                    videoEncParas.set_or_get = 1;
                                    videoEncParas.video_params.bitrate = bitrate;
                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoEncoder_Bitrate, &videoEncParas);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoEncoder_Bitrate set, success! bitrate=%u\n", videoEncParas.video_params.bitrate);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoEncoder_Bitrate set, fail, err %s!\n", gfGetErrorCodeString(err));
                                    }
                                } else {
                                    test_loge("[user cmd example]: --vencbitrate need followed by encoder generic_id, bitrate as %%u %%u\n");
                                }
                            } else if (!strncmp("--vencframerate", p_buffer, strlen("--vencframerate"))) {
                                SGenericVideoEncoderParam videoEncParas;
                                TGenericID id;
                                TUint framerate;

                                if (2 == sscanf(p_buffer, "--vencframerate %u %u", &id, &framerate)) {
                                    test_log("[user cmd parse]: '--vencframerate %u %u\n", id, framerate);

                                    //get current paras first
                                    videoEncParas.check_field = EGenericEngineConfigure_VideoEncoder_Framerate;
                                    videoEncParas.set_or_get = 0;
                                    memset(&videoEncParas.video_params, 0, sizeof(SVideoEncoderParam));

                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoEncoder_Framerate, &videoEncParas);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoEncoder_Framerate get, success! framerate=%f\n", videoEncParas.video_params.framerate);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoEncoder_Framerate get, fail, err %s!\n", gfGetErrorCodeString(err));
                                        break;
                                    }
                                    //set modified paras
                                    videoEncParas.set_or_get = 1;
                                    videoEncParas.video_params.framerate = framerate;
                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoEncoder_Framerate, &videoEncParas);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoEncoder_Framerate set, success! framerate=%f\n", videoEncParas.video_params.framerate);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoEncoder_Framerate set, fail, err %s!\n", gfGetErrorCodeString(err));
                                    }
                                } else {
                                    test_loge("[user cmd example]: --vencframerate need followed by encoder generic_id, framerate as %%u %%u\n");
                                }
                            } else if (!strncmp("--vencidr", p_buffer, strlen("--vencidr"))) {
                                SGenericVideoEncoderParam videoEncParas;
                                TGenericID id;
                                TUint demandIDR;

                                if (2 == sscanf(p_buffer, "--vencidr %u %u", &id, &demandIDR)) {
                                    test_log("[user cmd parse]: '--vencidr %u %u\n", id, demandIDR);

                                    //get current paras first
                                    videoEncParas.check_field = EGenericEngineConfigure_VideoEncoder_DemandIDR;
                                    videoEncParas.set_or_get = 0;
                                    memset(&videoEncParas.video_params, 0, sizeof(SVideoEncoderParam));

                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoEncoder_DemandIDR, &videoEncParas);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoEncoder_DemandIDR get, success! demandIDR=%u\n", videoEncParas.video_params.demand_idr_strategy);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoEncoder_DemandIDR get, fail, err %s!\n", gfGetErrorCodeString(err));
                                        break;
                                    }
                                    //set modified paras
                                    videoEncParas.set_or_get = 1;
                                    videoEncParas.video_params.demand_idr_strategy = (TU8)demandIDR;
                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoEncoder_DemandIDR, &videoEncParas);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoEncoder_DemandIDR set, success! demandIDR=%u\n", videoEncParas.video_params.demand_idr_strategy);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoEncoder_DemandIDR set, fail, err %s!\n", gfGetErrorCodeString(err));
                                    }
                                } else {
                                    test_loge("[user cmd example]: --vencidr need followed by encoder generic_id, demandIDR as %%u %%u\n");
                                }
                            } else if (!strncmp("--vencgop", p_buffer, strlen("--vencgop"))) {
                                SGenericVideoEncoderParam videoEncParas;
                                TGenericID id;
                                TUint gop_M, gop_N, gop_idr_interval, gop_structure;

                                if (5 == sscanf(p_buffer, "--vencgop %u %u %u %u %u", &id, &gop_M, &gop_N, &gop_idr_interval, &gop_structure)) {
                                    test_log("[user cmd parse]: '--vencgop %u %u %u %u %u\n", id, gop_M, gop_N, gop_idr_interval, gop_structure);

                                    //get current paras first
                                    videoEncParas.check_field = EGenericEngineConfigure_VideoEncoder_GOP;
                                    videoEncParas.set_or_get = 0;
                                    memset(&videoEncParas.video_params, 0, sizeof(SVideoEncoderParam));

                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoEncoder_GOP, &videoEncParas);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoEncoder_GOP get, success! gop_M=%u, gop_N=%u, gop_idr_interval=%u, gop_structure=%u\n",
                                                 videoEncParas.video_params.gop_M,
                                                 videoEncParas.video_params.gop_N,
                                                 videoEncParas.video_params.gop_idr_interval,
                                                 videoEncParas.video_params.gop_structure);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoEncoder_GOP get, fail, err %s!\n", gfGetErrorCodeString(err));
                                        break;
                                    }
                                    //set modified paras
                                    videoEncParas.set_or_get = 1;
                                    videoEncParas.video_params.gop_M = gop_M;
                                    videoEncParas.video_params.gop_N = gop_N;
                                    videoEncParas.video_params.gop_idr_interval = gop_idr_interval;
                                    videoEncParas.video_params.gop_structure = gop_structure;
                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoEncoder_GOP, &videoEncParas);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoEncoder_GOP set, success! gop_M=%u, gop_N=%u, gop_idr_interval=%u, gop_structure=%u\n",
                                                 videoEncParas.video_params.gop_M,
                                                 videoEncParas.video_params.gop_N,
                                                 videoEncParas.video_params.gop_idr_interval,
                                                 videoEncParas.video_params.gop_structure);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoEncoder_GOP set, fail, err %s!\n", gfGetErrorCodeString(err));
                                    }
                                } else {
                                    test_loge("[user cmd example]: --vencgop need followed by encoder generic_id, gop_M, gop_N, gop_idr_interval and gop_structure as %%u %%u %%u %%u %%u\n");
                                }
                            } else if (!strncmp("--recsplitduration", p_buffer, strlen("--recsplitduration"))) {
                                SGenericRecordSavingStrategy rec;
                                TULong index;
                                TUint duration;
                                if (2 == sscanf(p_buffer, "--recsplitduration %lu %u", &index, &duration)) {
                                    //get current paras first
                                    /*rec.check_field = EGenericEngineConfigure_Record_Saving_Strategy;
                                    rec.set_or_get= 0;
                                    rec.rec_pipeline_id = g_media_processor_context.recording_pipelines[index];
                                    memset(&rec.rec_saving_strategy, 0, sizeof(SRecSavingStrategy));

                                    err = p_engine->GenericControl(EGenericEngineConfigure_Record_Saving_Strategy, &rec);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_Record_Saving_Strategy get, success! index=%u, rec_pipeline_id=%u, strategy=%u, condition=%u, naming=%u, param=%u\n",
                                            index, rec.rec_pipeline_id,
                                            rec.rec_saving_strategy.strategy,
                                            rec.rec_saving_strategy.condition,
                                            rec.rec_saving_strategy.naming,
                                            rec.rec_saving_strategy.param);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_Record_Saving_Strategy get, fail, index=%u, rec_pipeline_id=%u, err %s!\n", index, rec.rec_pipeline_id, gfGetErrorCodeString(err));
                                        break;
                                    }*/
                                    //set modified paras
                                    rec.check_field = EGenericEngineConfigure_Record_Saving_Strategy;
                                    rec.set_or_get = 1;
                                    rec.rec_pipeline_id = g_media_processor_context.recording_pipelines[index];
                                    rec.rec_saving_strategy.strategy = MuxerSavingFileStrategy_AutoSeparateFile;
                                    rec.rec_saving_strategy.condition = MuxerSavingCondition_InputPTS;
                                    rec.rec_saving_strategy.naming = MuxerAutoFileNaming_ByNumber;
                                    rec.rec_saving_strategy.param = duration * TimeUnitDen_90khz;
                                    err = p_engine->GenericControl(EGenericEngineConfigure_Record_Saving_Strategy, &rec);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_Record_Saving_Strategy set, success! index=%lu, rec_pipeline_id=%u, strategy=%u, condition=%u, naming=%u, param=%u\n",
                                                 index, rec.rec_pipeline_id,
                                                 rec.rec_saving_strategy.strategy,
                                                 rec.rec_saving_strategy.condition,
                                                 rec.rec_saving_strategy.naming,
                                                 rec.rec_saving_strategy.param);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_Record_Saving_Strategy set, fail, index=%lu, rec_pipeline_id=%u, err %s!\n", index, rec.rec_pipeline_id, gfGetErrorCodeString(err));
                                    }
                                } else {
                                    test_loge("[user cmd example]: --recsplitduration need followed by record file index and duration(seconds) as %%u %%u\n");
                                }
                            } else if (!strncmp("--recsplitframe", p_buffer, strlen("--recsplitframe"))) {
                                SGenericRecordSavingStrategy rec;
                                TULong index;
                                TUint frame;
                                if (2 == sscanf(p_buffer, "--recsplitframe %lu %u", &index, &frame)) {
                                    //get current paras first
                                    /*rec.check_field = EGenericEngineConfigure_Record_Saving_Strategy;
                                    rec.set_or_get= 0;
                                    rec.rec_pipeline_id = g_media_processor_context.recording_pipelines[index];
                                    memset(&rec.rec_saving_strategy, 0, sizeof(SRecSavingStrategy));

                                    err = p_engine->GenericControl(EGenericEngineConfigure_Record_Saving_Strategy, &rec);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_Record_Saving_Strategy get, success! index=%u, rec_pipeline_id=%u, strategy=%u, condition=%u, naming=%u, param=%u\n",
                                            index, rec.rec_pipeline_id,
                                            rec.rec_saving_strategy.strategy,
                                            rec.rec_saving_strategy.condition,
                                            rec.rec_saving_strategy.naming,
                                            rec.rec_saving_strategy.param);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_Record_Saving_Strategy get, fail, index=%u, rec_pipeline_id=%u, err %s!\n", index, rec.rec_pipeline_id, gfGetErrorCodeString(err));
                                        break;
                                    }*/
                                    //set modified paras
                                    rec.check_field = EGenericEngineConfigure_Record_Saving_Strategy;
                                    rec.set_or_get = 1;
                                    rec.rec_pipeline_id = g_media_processor_context.recording_pipelines[index];
                                    rec.rec_saving_strategy.strategy = MuxerSavingFileStrategy_AutoSeparateFile;
                                    rec.rec_saving_strategy.condition = MuxerSavingCondition_FrameCount;
                                    rec.rec_saving_strategy.naming = MuxerAutoFileNaming_ByNumber;
                                    rec.rec_saving_strategy.param = frame;
                                    err = p_engine->GenericControl(EGenericEngineConfigure_Record_Saving_Strategy, &rec);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_Record_Saving_Strategy set, success! index=%lu, rec_pipeline_id=%u, strategy=%u, condition=%u, naming=%u, param=%u\n",
                                                 index, rec.rec_pipeline_id,
                                                 rec.rec_saving_strategy.strategy,
                                                 rec.rec_saving_strategy.condition,
                                                 rec.rec_saving_strategy.naming,
                                                 rec.rec_saving_strategy.param);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_Record_Saving_Strategy set, fail, index=%lu, rec_pipeline_id=%u, err %s!\n", index, rec.rec_pipeline_id, gfGetErrorCodeString(err));
                                    }
                                } else {
                                    test_loge("[user cmd example]: --recsplitframe need followed by record file index and frame number as %%u %%u\n");
                                }
                            } else if (!strncmp("--wtohd", p_buffer, strlen("--wtohd"))) {
                                SVideoPostPDisplayHD hd;
                                TGenericID id;
                                TU32 tobeHD;

                                if (2 == sscanf(p_buffer, "--wtohd %u %u", &id, &tobeHD)) {
                                    test_log("[user cmd parse]: '--wtohd %u %u\n", id, tobeHD);

                                    SQueryVideoDisplayLayout req;
                                    req.check_field = EGenericEngineQuery_VideoDisplayLayout;
                                    req.layer_index = 0;
                                    err = p_engine->GenericControl(EGenericEngineQuery_VideoDisplayLayout, &req);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineQuery_VideoDisplayLayout, success! layer=%u layout_type=%u\n", req.layer_index, req.layout_type);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineQuery_VideoDisplayLayout, fail, err %s!\n", gfGetErrorCodeString(err));
                                    }

                                    hd.check_field = EGenericEngineConfigure_VideoDisplay_SwitchWindowToHD;
                                    hd.tobeHD = (TU8)tobeHD;
                                    hd.sd_window_index_for_hd = id;
                                    hd.flush_udec = 0;
                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoDisplay_SwitchWindowToHD, &hd);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoDisplay_SwitchWindowToHD, success! tobeHD=%u\n", tobeHD);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoDisplay_SwitchWindowToHD, fail, err %s!\n", gfGetErrorCodeString(err));
                                    }
                                } else if (2 == sscanf(p_buffer, "--wtohdf %u %u", &id, &tobeHD)) {
                                    test_log("[user cmd parse]: '--wtohdf %u %u\n", id, tobeHD);

                                    hd.check_field = EGenericEngineConfigure_VideoDisplay_SwitchWindowToHD;
                                    hd.sd_window_index_for_hd = id;
                                    hd.tobeHD = (TU8)tobeHD;
                                    hd.flush_udec = 1;
                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoDisplay_SwitchWindowToHD, &hd);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoDisplay_SwitchWindowToHD, success! tobeHD=%u\n", tobeHD);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoDisplay_SwitchWindowToHD, fail, err %s!\n", gfGetErrorCodeString(err));
                                    }
                                } else {
                                    test_loge("[user cmd example]: --wtohd need followed by sd window index and tobeHD as %%u %%u\n");
                                }
                            } else if (!strncmp("--switchsourcegroup", p_buffer, strlen("--switchsourcegroup"))) {
                                TU32 target_source_group_index;
                                if (1 == sscanf(p_buffer, "--switchsourcegroup %u", &target_source_group_index)) {
                                    SPipeLineSwitchSourceGroup switch_source_group;
                                    switch_source_group.check_field = EGenericEngineConfigure_VideoPlayback_SwitchSourceGroup;
                                    switch_source_group.target_source_group_index = target_source_group_index;
                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_SwitchSourceGroup, &switch_source_group);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoPlayback_SwitchSourceGroup, success!\n");
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoPlayback_SwitchSourceGroup fail, err %s!\n", gfGetErrorCodeString(err));
                                    }
                                } else {
                                    test_loge("[user cmd example]: --switchsourcegroup need followed by target_source_group_index as %%u\n");
                                }
                            } else {
                                test_logw("un-processed cmdline %s\n", p_buffer);
                            }
                        }
                        break;

                    default:
                        break;
                }
            }

        } else {
            test_loge("mudec_start_process fail(), ret %d\n", ret);
            return ret;
        }
    } else {
        test_loge("mudec_build_process_pipelines fail(), ret %d\n", ret);
        return ret;
    }

    ret = __stop_process();
    if (ret) {
        test_loge("mudec_stop_process fail(), ret %d\n", ret);
    }

    __destroy_media_processor();

    return 0;
}

TInt udec_test()
{
    TInt ret = 0;

    TChar buffer_old[128] = {0};
    TChar buffer[128];
    TChar *p_buffer = buffer;
    TInt udec_running = 1;

    ret = udec_build_process_pipelines();

    if (!ret) {
        ret = __start_process();
        if (!ret) {
            //process cmd line

            while (udec_running) {
                //add sleep to avoid affecting the performance
                usleep(100000);
                if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0)
                { continue; }

                if (buffer[0] == '\n') {
                    p_buffer = buffer_old;
                    test_log("repeat last cmd:\n\t\t%s\n", buffer_old);
                } else if (buffer[0] == 'l') {
                    test_log("show last cmd:\n\t\t%s\n", buffer_old);
                    continue;
                } else {
                    p_buffer = buffer;

                    //record last cmd
                    strncpy(buffer_old, buffer, sizeof(buffer_old) - 1);
                    buffer_old[sizeof(buffer_old) - 1] = 0x0;
                }

                switch (p_buffer[0]) {

                    case 'q':   // exit
                        test_log("[user cmd]: 'q', Quit\n");
                        udec_running = 0;
                        break;

                    case 'h':   // help
                        test_log("[user cmd]: 'h', print help\n");
                        _mtest_print_runtime_usage();
                        break;

                    case 'p':
                        if (!strncmp(p_buffer, "pall", strlen("pall"))) {
                            test_log("[user cmd]: 'pall', print all\n");
                            if (g_media_processor_context.p_engine) {
                                g_media_processor_context.p_engine->PrintCurrentStatus();
                            }
                        }
                        break;

                    case 'c':
                        if (!strncmp(p_buffer, "csavelogconfig", strlen("csavelogconfig"))) {
                            test_log("[user cmd]: 'csavelogconfig', save current to log.config\n");
                            if (g_media_processor_context.p_engine) {
                                g_media_processor_context.p_engine->SaveCurrentLogSetting();
                            }
                        }
                        break;

                    default:
                        break;
                }
            }

        } else {
            test_loge("mudec_start_process fail(), ret %d\n", ret);
            return ret;
        }
    } else {
        test_loge("udec_build_process_pipelines fail(), ret %d\n", ret);
        return ret;
    }

    ret = __stop_process();
    if (ret) {
        test_loge("mudec_stop_process fail(), ret %d\n", ret);
    }

    __destroy_media_processor();

    return 0;
}

TInt duplex_test()
{
    return 0;
}

TInt rectest_test()
{
    return 0;
}

TInt generic_nvr_test()
{
    TInt ret = 0;
    EECode err = EECode_OK;

    IGenericEngineControl *p_engine = NULL;
    TChar buffer_old[128] = {0};
    TChar buffer[128] = {0};
    TChar *p_buffer = buffer;
    TInt udec_running = 1;

    ret = generic_nvr_build_process_pipelines();

    if (!ret) {
        ret = __start_process();
        if (!ret) {
            //process cmd line

            while (udec_running) {
                //add sleep to avoid affecting the performance
                usleep(100000);
                //memset(buffer, 0x0, sizeof(buffer));
                if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0) {
                    test_loge("read STDIN_FILENO fail\n");
                    continue;
                }

                test_log("[user cmd debug]: read input '%s'\n", buffer);

                if (buffer[0] == '\n') {
                    p_buffer = buffer_old;
                    test_log("repeat last cmd:\n\t\t%s\n", buffer_old);
                } else if (buffer[0] == 'l') {
                    test_log("show last cmd:\n\t\t%s\n", buffer_old);
                    continue;
                } else {

                    ret = __replace_enter(buffer, (128 - 1));
                    DASSERT(0 == ret);
                    if (ret) {
                        test_loge("no enter found\n");
                        continue;
                    }
                    p_buffer = buffer;

                    //record last cmd
                    strncpy(buffer_old, buffer, sizeof(buffer_old) - 1);
                    buffer_old[sizeof(buffer_old) - 1] = 0x0;
                }

                test_log("[user cmd debug]: '%s'\n", p_buffer);

                p_engine = g_media_processor_context.p_engine;
                switch (p_buffer[0]) {

                    case 'q':   // exit
                        test_log("[user cmd]: 'q', Quit\n");
                        udec_running = 0;
                        break;

                    case 'h':   // help
                        _mtest_print_version();
                        _mtest_print_usage();
                        _mtest_print_runtime_usage();
                        break;

                    case 'c': {
                            TUint id = 0;
                            if (!strncmp("ccjpeg:", p_buffer, strlen("ccjpeg:"))) {
                                TUint cap_index = 0;
                                SPlaybackCapture capture;
                                SVideoCodecInfo dec_info;
                                SVideoDisplayDeviceInfo vout_info;

                                if (2 == sscanf(p_buffer, "ccjpeg:%x %x", &id, &cap_index)) {
                                    if ((0xff != id) && (id >= (g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2))) {
                                        test_loge("dec id(%d) exceed max value\n", id);
                                        break;
                                    }

                                    if (0xff != id) {
                                        dec_info.dec_id = id;
                                        dec_info.check_field = EGenericEngineQuery_VideoCodecInfo;
                                        err = p_engine->GenericControl(EGenericEngineQuery_VideoCodecInfo, (void *)&dec_info);
                                        DASSERT_OK(err);
                                        if (EECode_OK != err) {
                                            test_loge("EGenericEngineQuery_VideoCodecInfo dec_id %d fail\n", id);
                                            break;
                                        }
                                    }

                                    vout_info.check_field = EGenericEngineQuery_VideoDisplayDeviceInfo;
                                    err = p_engine->GenericControl(EGenericEngineQuery_VideoDisplayDeviceInfo, (void *)&vout_info);
                                    DASSERT_OK(err);
                                    if (EECode_OK != err) {
                                        test_loge("EGenericEngineQuery_VideoDisplayDeviceInfo fail\n");
                                        break;
                                    }

                                    memset(&capture, 0x0, sizeof(SPlaybackCapture));
                                    capture.check_field = EGenericEngineConfigure_VideoPlayback_Capture;
                                    capture.capture_id = id;
                                    if (0xff != id) {
                                        capture.capture_coded = 1;
                                    } else {
                                        capture.capture_coded = 0;
                                    }
                                    capture.capture_screennail = 1;
                                    capture.capture_thumbnail = 1;

                                    capture.jpeg_quality = 50;
                                    capture.capture_file_index = cap_index;
                                    capture.customized_file_name = 0;

                                    if (0xff != id) {
                                        capture.coded_width = dec_info.pic_width;
                                        capture.coded_height = dec_info.pic_height;
                                    } else {
                                        capture.coded_width = 0;
                                        capture.coded_height = 0;
                                    }

                                    capture.screennail_width = vout_info.primary_vout_width;
                                    capture.screennail_height = vout_info.primary_vout_height;

                                    capture.thumbnail_width = 320;
                                    capture.thumbnail_height = 240;

                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_Capture, (void *)&capture);
                                    DASSERT_OK(err);
                                }
                            } else if (!strncmp("cmute", p_buffer, strlen("cmute"))) {
                                test_log("mute audio: EGenericEngineConfigure_AudioPlayback_Mute start\n");
                                err = p_engine->GenericControl(EGenericEngineConfigure_AudioPlayback_Mute, NULL);
                                if (DUnlikely(EECode_OK != err)) {
                                    test_loge("EGenericEngineConfigure_AudioPlayback_Mute fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                                    break;
                                }
                                test_log("mute audio: EGenericEngineConfigure_AudioPlayback_Mute end\n");
                            } else if (!strncmp("cumute", p_buffer, strlen("cumute"))) {
                                test_log("umute audio: EGenericEngineConfigure_AudioPlayback_UnMute start\n");
                                err = p_engine->GenericControl(EGenericEngineConfigure_AudioPlayback_UnMute, NULL);
                                if (DUnlikely(EECode_OK != err)) {
                                    test_loge("EGenericEngineConfigure_AudioPlayback_UnMute fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                                    break;
                                }
                                test_log("umute audio: EGenericEngineConfigure_AudioPlayback_UnMute end\n");
                            } else if (!strncmp("cselectaudio:", p_buffer, strlen("cselectaudio:"))) {
                                SGenericSelectAudioChannel audio_channel_index;
                                if (1 == sscanf(p_buffer, "cselectaudio:%d", &audio_channel_index.channel)) {
                                    test_log("'cselectaudio:%d' start\n", audio_channel_index.channel);
                                    audio_channel_index.check_field = EGenericEngineConfigure_AudioPlayback_SelectAudioSource;
                                    err = p_engine->GenericControl(EGenericEngineConfigure_AudioPlayback_SelectAudioSource, &audio_channel_index);
                                    if (DUnlikely(EECode_OK != err)) {
                                        test_loge("EGenericEngineConfigure_AudioPlayback_SelectAudioSource fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                                        break;
                                    }
                                    test_log("'cselectaudio:%d' end\n", audio_channel_index.channel);
                                } else {
                                    test_loge("'cselectaudio:%%d' should specify audio channel's index\n");
                                }
                            } else if (!strncmp("cz", p_buffer, strlen("cz"))) {//cz1,cz2,czi,czo
                                switch (p_buffer[2]) {
                                    case '1':
                                        test_log("[user cmd]: '%s' no supported in mtest now.\n", p_buffer);
                                        break;
                                    case '2': {
                                            TU16 input_w = 0, input_h = 0, input_center_x = 0, input_center_y = 0;
                                            SPlaybackZoom zoom;
                                            SVideoCodecInfo dec_info;
                                            if (':' == p_buffer[3]) {
                                                if (5 == sscanf(p_buffer, "cz2:%d,%hd,%hd,%hd,%hd", &id, &input_w, &input_h, &input_center_x, &input_center_y)) {

                                                    SVideoPostPDisplayMWMapping mapping;
                                                    if (id >= (g_mtest_dsp_dec_instances_number_1 > g_mtest_dsp_dec_instances_number_2 ? g_mtest_dsp_dec_instances_number_1 : g_mtest_dsp_dec_instances_number_2)) {
                                                        test_loge("render id(%d) exceed max value\n", id);
                                                        break;
                                                    }
                                                    mapping.render_id = id;
                                                    mapping.check_field = EGenericEngineQuery_VideoDisplayRender;
                                                    err = p_engine->GenericControl(EGenericEngineQuery_VideoDisplayRender, (void *)&mapping);
                                                    if (EECode_OK != err) {
                                                        test_loge("EGenericEngineQuery_VideoDisplayRender fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                                                        break;
                                                    }
                                                    if (DINVALID_VALUE_TAG == mapping.decoder_id) {
                                                        test_loge("EGenericEngineQuery_VideoDisplayRender fail, bad render id %d, ret %d, %s\n", mapping.render_id, err, gfGetErrorCodeString(err));
                                                        break;
                                                    }
                                                    test_log("EGenericEngineQuery_VideoDisplayRender done, render_id=%u, decoder_id=%u\n",
                                                             mapping.render_id, mapping.decoder_id);

                                                    dec_info.dec_id = mapping.decoder_id;
                                                    dec_info.check_field = EGenericEngineQuery_VideoCodecInfo;
                                                    err = p_engine->GenericControl(EGenericEngineQuery_VideoCodecInfo, (void *)&dec_info);
                                                    DASSERT_OK(err);
                                                    if (EECode_OK != err) {
                                                        test_loge("EGenericEngineQuery_VideoCodecInfo dec_id %d fail\n", dec_info.dec_id);
                                                        break;
                                                    }

                                                    test_log("EGenericEngineQuery_VideoCodecInfo done. dec_id=%d, pic_width=%u, pic_height=%u\n", dec_info.dec_id, dec_info.pic_width, dec_info.pic_height);
                                                    if (input_w > dec_info.pic_width
                                                            || input_h > dec_info.pic_height
                                                            || (input_center_x + (input_w / 2)) > dec_info.pic_width
                                                            || (input_center_x - (input_w / 2)) < 0
                                                            || (input_center_y + (input_h / 2)) > dec_info.pic_height
                                                            || (input_center_y - (input_h / 2)) < 0) {
                                                        test_loge("zoom parameters excced expect range.\n");
                                                        break;
                                                    }

                                                    zoom.check_field = EGenericEngineConfigure_VideoPlayback_Zoom;
                                                    zoom.zoom_mode = 0;
                                                    zoom.render_id = mapping.render_id;
                                                    zoom.input_center_x = input_center_x;
                                                    zoom.input_center_y = input_center_y;
                                                    zoom.input_width = input_w;
                                                    zoom.input_height = input_h;

                                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_Zoom, (void *)&zoom);
                                                    DASSERT_OK(err);
                                                } else {
                                                    test_loge("command format invalid, pls input 'cz2:render_id,input_with,input_height,input_center_x,input_center_y', all paras are decimal format.\n");
                                                }
                                            } else if ('w' == p_buffer[3]) {
                                                if (5 == sscanf(p_buffer, "cz2w:%d,%hd,%hd,%hd,%hd", &id, &input_w, &input_h, &input_center_x, &input_center_y)) {

                                                    SVideoPostPDisplayMWMapping mapping;
                                                    if (id >= (g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2)) {
                                                        test_loge("window id(%d) exceed max value\n", id);
                                                        break;
                                                    }
                                                    mapping.window_id = id;
                                                    mapping.check_field = EGenericEngineQuery_VideoDisplayWindow;
                                                    err = p_engine->GenericControl(EGenericEngineQuery_VideoDisplayWindow, (void *)&mapping);
                                                    if (EECode_OK != err) {
                                                        test_loge("EGenericEngineQuery_VideoDisplayWindow fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                                                        break;
                                                    }
                                                    if (DINVALID_VALUE_TAG == mapping.decoder_id) {
                                                        test_loge("EGenericEngineQuery_VideoDisplayWindow fail, bad window id %d, ret %d, %s\n", mapping.window_id, err, gfGetErrorCodeString(err));
                                                        break;
                                                    }
                                                    test_log("EGenericEngineQuery_VideoDisplayWindow done, window_id=%d, decoder_id=%u, render_id=%u\n",
                                                             mapping.window_id, mapping.decoder_id, mapping.render_id);

                                                    dec_info.dec_id = mapping.decoder_id;
                                                    dec_info.check_field = EGenericEngineQuery_VideoCodecInfo;
                                                    err = p_engine->GenericControl(EGenericEngineQuery_VideoCodecInfo, (void *)&dec_info);
                                                    DASSERT_OK(err);
                                                    if (EECode_OK != err) {
                                                        test_loge("EGenericEngineQuery_VideoCodecInfo dec_id %d fail\n", dec_info.dec_id);
                                                        break;
                                                    }

                                                    test_log("EGenericEngineQuery_VideoCodecInfo done. dec_id=%d, pic_width=%u, pic_height=%u\n", dec_info.dec_id, dec_info.pic_width, dec_info.pic_height);
                                                    if (input_w > dec_info.pic_width
                                                            || input_h > dec_info.pic_height
                                                            || (input_center_x + (input_w / 2)) > dec_info.pic_width
                                                            || (input_center_x - (input_w / 2)) < 0
                                                            || (input_center_y + (input_h / 2)) > dec_info.pic_height
                                                            || (input_center_y - (input_h / 2)) < 0) {
                                                        test_loge("zoom parameters excced expect range.\n");
                                                        break;
                                                    }

                                                    zoom.check_field = EGenericEngineConfigure_VideoPlayback_Zoom;
                                                    zoom.zoom_mode = 0;
                                                    zoom.render_id = mapping.render_id;
                                                    zoom.input_center_x = input_center_x;
                                                    zoom.input_center_y = input_center_y;
                                                    zoom.input_width = input_w;
                                                    zoom.input_height = input_h;

                                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_Zoom, (void *)&zoom);
                                                    DASSERT_OK(err);
                                                } else {
                                                    test_loge("command format invalid, pls input 'cz2w:window_id,input_with,input_height,input_center_x,input_center_y', all paras are decimal format.\n");
                                                }
                                            } else {
                                                test_loge("command format invalid, pls input 'cz2r......' or 'cz2w......'.\n");
                                            }
                                        }
                                        break;
                                    case 'i':
                                        test_log("[user cmd]: '%s' no supported in mtest now.\n", p_buffer);
                                        break;
                                    case 'o':
                                        test_log("[user cmd]: '%s' no supported in mtest now.\n", p_buffer);
                                        break;
                                    default:
                                        test_log("[user cmd]: '%s' invalid for zoom operation.\n", p_buffer);
                                        break;
                                }
                            } else if (!strncmp("cp", p_buffer, strlen("cp"))) {//fast-forward, fast-backward
                                TU16 source_index = 0, speed = 0, speed_frac = 0, feeding_rule; //, possible_speed=0
                                switch (p_buffer[2]) {
                                    case 'f':
                                        if (4 == sscanf(buffer, "cpf:%hu %hu.%hu %hu", &source_index, &speed, &speed_frac, &feeding_rule)) {
                                            if (source_index >= g_media_processor_context.source_url_count_in_group[g_cur_playback_url_group]
                                                    || (feeding_rule != DecoderFeedingRule_AllFrames && feeding_rule != DecoderFeedingRule_IOnly)) {
                                                test_logw("BAD params, source_index %hu, feeding_rule=%hu\n", source_index, feeding_rule);
                                                break;
                                            }
                                            /*                                            possible_speed = max_possiable_speed(p_engine, source_index);
                                                                                        test_log("specify playback speed %hu.%hu, possible_speed %hu\n", speed, speed_frac, possible_speed);
                                                                                        if (speed > possible_speed) {
                                                                                            test_log("speed(%hu.%hu) > max_speed %hu, use I only mode\n", speed, speed_frac, possible_speed);
                                                                                            feeding_rule = DecoderFeedingRule_IOnly;
                                                                                        } else {
                                                                                            test_log("speed(%hu.%hu) <= max_speed %hu, use all frames mode\n", speed, speed_frac, possible_speed);
                                                                                            feeding_rule = DecoderFeedingRule_AllFrames;
                                                                                        }*/
                                            err = p_engine->PBSpeed(g_media_processor_context.playback_pipelines[source_index], speed, speed_frac, 0, (DecoderFeedingRule)feeding_rule);
                                            DASSERT_OK(err);
                                        } else {
                                            test_loge("command format invalid, pls input 'cpf:source_index speed.speed_frac feeding_rule, all paras are decimal format, feeding_rule only support 1-all frames and 3-IOnly now.\n");
                                        }
                                        break;
                                    case 'b':
                                        if (4 == sscanf(buffer, "cpb:%hu %hu.%hu %hu", &source_index, &speed, &speed_frac, &feeding_rule)) {
                                            if (source_index >= g_media_processor_context.source_url_count_in_group[g_cur_playback_url_group]
                                                    || (feeding_rule != DecoderFeedingRule_IOnly)) {
                                                test_logw("BAD params, source_index %hu, feeding_rule=%hu\n", source_index, feeding_rule);
                                                break;
                                            }
                                            err = p_engine->PBSpeed(g_media_processor_context.playback_pipelines[source_index], speed, speed_frac, 1, (DecoderFeedingRule)feeding_rule);
                                            DASSERT_OK(err);
                                        } else {
                                            test_loge("command format invalid, pls input 'cpb:source_index speed.speed_frac feeding_rule, all paras are decimal format, feeding_rule only support 3-IOnly now.\n");
                                        }
                                        break;
                                    case 'c':
                                        if (1 == sscanf(buffer, "cpc:%hu", &source_index)) {
                                            if (source_index >= g_media_processor_context.source_url_count_in_group[g_cur_playback_url_group]) {
                                                test_logw("BAD params, source_index %hu\n", source_index);
                                                break;
                                            }
                                            err = p_engine->PBSpeed(g_media_processor_context.playback_pipelines[source_index], 1, 0, 0, DecoderFeedingRule_AllFrames);
                                            DASSERT_OK(err);
                                        } else {
                                            test_loge("command format invalid, pls input 'cpc:source_index, all paras are decimal format.\n");
                                        }
                                        break;
                                    default:
                                        test_log("[user cmd]: '%s' invalid for playback speed updating.\n", p_buffer);
                                        break;
                                }
                            } else if (!strncmp("csuspenddsp", p_buffer, strlen("csuspenddsp"))) {
                                test_log("[user cmd]: csuspenddsp.\n");
                                err = p_engine->GenericControl(EGenericEngineConfigure_DSP_Suspend, (void *)1);
                                DASSERT_OK(err);
                            } else if (!strncmp("cresumedsp", p_buffer, strlen("cresumedsp"))) {
                                test_log("[user cmd]: cresumedsp.\n");
                                err = p_engine->GenericControl(EGenericEngineConfigure_DSP_Resume, (void *)1);
                                DASSERT_OK(err);
                            } else if (!strncmp("creconnectserver", p_buffer, strlen("creconnectserver"))) {
                                TUint reconnect_index = 0;
                                if (1 == sscanf(p_buffer, "creconnectserver %d", &reconnect_index)) {
                                    SGenericReconnectStreamingServer reconnect;
                                    memset(&reconnect, 0x0, sizeof(SGenericReconnectStreamingServer));
                                    reconnect.check_field = EGenericEngineConfigure_ReConnectServer;
                                    reconnect.index = reconnect_index;
                                    if (0xff == reconnect_index) {
                                        reconnect.all_demuxer = 1;
                                    }
                                    test_log("[user cmd]: creconnectserver %d, reconnect.all_demuxer %d.\n", reconnect_index, reconnect.all_demuxer);
                                    err = p_engine->GenericControl(EGenericEngineConfigure_ReConnectServer, (void *)&reconnect);
                                    DASSERT_OK(err);
                                }
                            } else if (!strncmp("creconnectall", p_buffer, strlen("creconnectall"))) {
                                SGenericReconnectStreamingServer reconnect;
                                memset(&reconnect, 0x0, sizeof(SGenericReconnectStreamingServer));
                                reconnect.check_field = EGenericEngineConfigure_ReConnectServer;
                                reconnect.all_demuxer = 1;
                                test_log("[user cmd]: creconnectall\n");
                                err = p_engine->GenericControl(EGenericEngineConfigure_ReConnectServer, (void *)&reconnect);
                                DASSERT_OK(err);
                            }

                        }
                        break;

                    case 'p': {
                            test_log("[user cmd]: '%s'\n", p_buffer);
                            if (!strncmp("pall", p_buffer, strlen("pall"))) {
                                test_log("print all\n");
                                p_engine->PrintCurrentStatus(0, DPrintFlagBit_dataPipeline);
                            } else if (!strncmp("plog", p_buffer, strlen("plog"))) {
                                test_log("print log configure\n");
                                p_engine->PrintCurrentStatus(0, DPrintFlagBit_logConfigure);
                            } else if (!strncmp("pdsp", p_buffer, strlen("pdsp"))) {
                                TU32 id = 0;
                                if (1 == sscanf(p_buffer, "pdsp:%u", &id)) {

                                    if (id >= (g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2)) {
                                        test_loge("window id(%d) exceed max value\n", id);
                                        break;
                                    }

                                    err = p_engine->GenericControl(EGenericEngineQuery_VideoDSPStatus, (void *)&id);
                                    DASSERT_OK(err);
                                    if (EECode_OK != err) {
                                        test_loge("EGenericEngineQuery_VideoDSPStatus id %d fail\n", id);
                                        break;
                                    }
                                    test_log("EGenericEngineQuery_VideoDSPStatus done. udec(%d) info:\n", id);
                                } else {
                                    test_loge("command format invalid, pls input 'pdsp:udec_id', all paras are decimal format.\n");
                                }
                            }
                        }
                        break;

                    case ' ': {
                            TUint id = 0;
                            SVideoPostPDisplayTrickPlay trickplay;

                            test_log("[user cmd]: '%s'\n", p_buffer);
                            if (1 == sscanf(p_buffer, " %d", &id)) {
                                if (id > (g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2)) {
                                    test_loge("dec id(%d) exceed max value\n", id);
                                    break;
                                }
                                if (0 == g_media_processor_context.dsp_dec_trickplay[id]) {
                                    test_log("pause dec %d\n", id);
                                    trickplay.check_field = EGenericEngineConfigure_VideoPlayback_UDECPause;
                                    trickplay.id = id;
                                    p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_UDECPause, (void *)&trickplay);
                                    g_media_processor_context.dsp_dec_trickplay[id] = 1;
                                } else if (1 == g_media_processor_context.dsp_dec_trickplay[id]) {
                                    test_log("resume dec %d\n", id);
                                    trickplay.check_field = EGenericEngineConfigure_VideoPlayback_UDECResume;
                                    trickplay.id = id;
                                    p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_UDECResume, (void *)&trickplay);
                                    g_media_processor_context.dsp_dec_trickplay[id] = 0;
                                } else if (2 == g_media_processor_context.dsp_dec_trickplay[id]) {
                                    test_log("resume dec %d from step mode\n", id);
                                    trickplay.check_field = EGenericEngineConfigure_VideoPlayback_UDECResume;
                                    trickplay.id = id;
                                    p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_UDECResume, (void *)&trickplay);
                                    g_media_processor_context.dsp_dec_trickplay[id] = 0;
                                } else {
                                    test_loge("BAD trickplay status %d for dec %d\n", g_media_processor_context.dsp_dec_trickplay[id], id);
                                }
                            } else if (1 == sscanf(p_buffer, " w%d", &id)) {
                                SVideoPostPDisplayMWMapping mapping;
                                if (id > (g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2)) {
                                    test_loge("window id(%d) exceed max value\n", id);
                                    break;
                                }
                                mapping.window_id = id;
                                mapping.check_field = EGenericEngineQuery_VideoDisplayWindow;
                                err = p_engine->GenericControl(EGenericEngineQuery_VideoDisplayWindow, (void *)&mapping);
                                if (EECode_OK != err) {
                                    test_loge("EGenericEngineQuery_VideoDisplayWindow fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                                    break;
                                }
                                if (DINVALID_VALUE_TAG == mapping.decoder_id) {
                                    test_loge("EGenericEngineQuery_VideoDisplayWindow fail, bad window id %d, ret %d, %s\n", id, err, gfGetErrorCodeString(err));
                                    break;
                                }

                                test_log("[trick cmd for window(%d)]: dec id %d\n", id, mapping.decoder_id);
                                id = mapping.decoder_id;

                                if (0 == g_media_processor_context.dsp_dec_trickplay[id]) {
                                    test_log("pause dec %d\n", id);
                                    trickplay.check_field = EGenericEngineConfigure_VideoPlayback_UDECPause;
                                    trickplay.id = id;
                                    p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_UDECPause, (void *)&trickplay);
                                    g_media_processor_context.dsp_dec_trickplay[id] = 1;
                                } else if (1 == g_media_processor_context.dsp_dec_trickplay[id]) {
                                    test_log("resume dec %d\n", id);
                                    trickplay.check_field = EGenericEngineConfigure_VideoPlayback_UDECResume;
                                    trickplay.id = id;
                                    p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_UDECResume, (void *)&trickplay);
                                    g_media_processor_context.dsp_dec_trickplay[id] = 0;
                                } else if (2 == g_media_processor_context.dsp_dec_trickplay[id]) {
                                    test_log("resume dec %d from step mode\n", id);
                                    trickplay.check_field = EGenericEngineConfigure_VideoPlayback_UDECResume;
                                    trickplay.id = id;
                                    p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_UDECResume, (void *)&trickplay);
                                    g_media_processor_context.dsp_dec_trickplay[id] = 0;
                                } else {
                                    test_loge("BAD trickplay status %d for dec %d\n", g_media_processor_context.dsp_dec_trickplay[id], id);
                                }
                            }
                        }
                        break;

                    case 's': {
                            TUint id = 0;
                            SVideoPostPDisplayTrickPlay trickplay;

                            test_log("[user cmd]: '%s'\n", p_buffer);
                            if (1 == sscanf(p_buffer, "s%d", &id)) {
                                if (id > (g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2)) {
                                    test_loge("dec id(%d) exceed max value\n", id);
                                    break;
                                }

                                test_log("step play dec %d\n", id);
                                trickplay.check_field = EGenericEngineConfigure_VideoPlayback_UDECStepPlay;
                                trickplay.id = id;
                                p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_UDECStepPlay, (void *)&trickplay);
                                g_media_processor_context.dsp_dec_trickplay[id] = 2;
                            } else if (1 == sscanf(p_buffer, "sw%d", &id)) {
                                SVideoPostPDisplayMWMapping mapping;
                                if (id > (g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2)) {
                                    test_loge("window id(%d) exceed max value\n", id);
                                    break;
                                }
                                mapping.window_id = id;
                                mapping.check_field = EGenericEngineQuery_VideoDisplayWindow;
                                err = p_engine->GenericControl(EGenericEngineQuery_VideoDisplayWindow, (void *)&mapping);
                                if (EECode_OK != err) {
                                    test_loge("EGenericEngineQuery_VideoDisplayWindow fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                                    break;
                                }
                                if (DINVALID_VALUE_TAG == mapping.decoder_id) {
                                    test_loge("EGenericEngineQuery_VideoDisplayWindow fail, bad window id %d, ret %d, %s\n", id, err, gfGetErrorCodeString(err));
                                    break;
                                }

                                test_log("[trick cmd for window(%d)]: dec id %d\n", id, mapping.decoder_id);
                                id = mapping.decoder_id;

                                test_log("step play dec %d\n", id);
                                trickplay.check_field = EGenericEngineConfigure_VideoPlayback_UDECStepPlay;
                                trickplay.id = id;
                                p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_UDECStepPlay, (void *)&trickplay);
                                g_media_processor_context.dsp_dec_trickplay[id] = 2;
                            }
                        }
                        break;

                    case '-':
                        if ('-' == p_buffer[1]) {
                            test_log("[user cmd]: '%s'\n", p_buffer);
                            if (!strncmp("--hd", p_buffer, strlen("--hd"))) {
                                TUint id = 0;
                                if (1 == sscanf(p_buffer, "--hd %u", &id)) {
                                    SVideoPostPDisplayLayer layer;
                                    layer.check_field = EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer;
                                    layer.request_display_layer = 1;
                                    layer.sd_window_index_for_hd = id;
                                    layer.flush_udec = 0;
                                    test_log("[user cmd parse]: '--hd %%d' ---> request layer 1, id = %d\n", id);
                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer, &layer);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer layer 1, success!\n");
                                        //g_current_play_hd = 1;
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer layer 1, fail, err %s!\n", gfGetErrorCodeString(err));
                                    }
                                } else if (1 == sscanf(p_buffer, "--hdf %u", &id)) {
                                    SVideoPostPDisplayLayer layer;
                                    layer.check_field = EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer;
                                    layer.request_display_layer = 1;
                                    layer.sd_window_index_for_hd = id;
                                    layer.flush_udec = 1;
                                    test_log("[user cmd parse]: '--hdf %%d' ---> request layer 1, id = %d\n", id);
                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer, &layer);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer layer 1, success!\n");
                                        //g_current_play_hd = 1;
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer layer 1, fail, err %s!\n", gfGetErrorCodeString(err));
                                    }
                                } else {
                                    test_loge("[user cmd example]: --hd need followed by according SD window index %%u\n");
                                }
                            } else if (!strncmp("--sd", p_buffer, strlen("--sd"))) {
                                SVideoPostPDisplayLayer layer;
                                if ('f' == p_buffer[4]) {
                                    test_log("[user cmd parse]: '--sdf' ---> request layer 0\n");
                                    layer.flush_udec = 1;
                                } else {
                                    test_log("[user cmd parse]: '--sd' ---> request layer 0\n");
                                    layer.flush_udec = 0;
                                }
                                layer.check_field = EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer;
                                layer.request_display_layer = 0;

                                err = p_engine->GenericControl(EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer, &layer);
                                if (EECode_OK == err) {
                                    test_log("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer layer 0, success!\n");
                                    //g_current_play_hd = 0;
                                } else {
                                    test_loge("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer layer 0, fail, err %s!\n", gfGetErrorCodeString(err));
                                }
                            } else if (!strncmp("--layout", p_buffer, strlen("--layout"))) {
                                SVideoPostPDisplayLayout layout;
                                TUint layout_type = 0;
                                TUint dsp_instance_number = g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2;

                                if (1 == sscanf(p_buffer, "--layout %d", &layout_type)) {
                                    test_log("[user cmd parse]: '--layout' ---> request layout type %d\n", layout_type);
                                    layout.check_field = EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayout;
                                    layout.layer_number = g_mtest_dsp_dec_layer;
                                    layout.layer_layout_type[0] = layout_type;
                                    layout.layer_layout_type[1] = EDisplayLayout_Rectangle;
                                    layout.layer_window_number[0] = g_mtest_dsp_dec_instances_number_1;
                                    layout.layer_window_number[1] = g_mtest_dsp_dec_instances_number_2;

                                    layout.total_decoder_number = dsp_instance_number;
                                    layout.total_render_number = dsp_instance_number;
                                    layout.total_window_number = dsp_instance_number;

                                    layout.cur_decoder_number = g_mtest_dsp_dec_instances_number_1;
                                    layout.cur_render_number = g_mtest_dsp_dec_instances_number_1;
                                    layout.cur_window_number = dsp_instance_number;

                                    layout.cur_decoder_start_index = 0;
                                    layout.cur_render_start_index = 0;
                                    layout.cur_window_start_index = 0;

                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayout, &layout);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayout %d, success!\n", layout_type);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayout %d, fail, err %s!\n", layout_type, gfGetErrorCodeString(err));
                                    }
                                } else {
                                    test_loge("[user cmd example]: --layout need followed by layout type %%d\n");
                                }
                            } else if (!strncmp("--fulllayout", p_buffer, strlen("--fulllayout"))) {
                                SVideoPostPDisplayLayout layout;
                                TUint layout_type = 0;
                                TUint dsp_instance_number = g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2;

                                if (1 == sscanf(p_buffer, "--fulllayout %d", &layout_type)) {
                                    test_log("[user cmd parse]: '--fulllayout' ---> request layout type %d\n", layout_type);
                                    layout.check_field = EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayout;
                                    layout.layer_number = 1;
                                    layout.layer_layout_type[0] = layout_type;
                                    layout.layer_layout_type[1] = EDisplayLayout_Invalid;
                                    layout.layer_window_number[0] = g_mtest_dsp_dec_instances_number_1 + g_mtest_dsp_dec_instances_number_2;
                                    layout.layer_window_number[1] = g_mtest_dsp_dec_instances_number_2;

                                    layout.total_decoder_number = dsp_instance_number;
                                    layout.total_render_number = dsp_instance_number;
                                    layout.total_window_number = dsp_instance_number;

                                    layout.cur_decoder_number = dsp_instance_number;
                                    layout.cur_render_number = dsp_instance_number;
                                    layout.cur_window_number = dsp_instance_number;

                                    layout.cur_decoder_start_index = 0;
                                    layout.cur_render_start_index = 0;
                                    layout.cur_window_start_index = 0;

                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayout, &layout);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayout %d, success!\n", layout_type);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayout %d, fail, err %s!\n", layout_type, gfGetErrorCodeString(err));
                                    }
                                }
                            } else if (!strncmp("--highlight", p_buffer, strlen("--highlight"))) {
                                TU16 highlighten_window_width = 0, highlighten_window_height = 0;
                                if (2 == sscanf(p_buffer, "--highlight %hu %hu", &highlighten_window_width, &highlighten_window_height)) {
                                    test_log("[user cmd parse]: '--highlight' ---> request highlighten window size %hux%hu\n", highlighten_window_width, highlighten_window_height);
                                    SVideoPostPDisplayHighLightenWindowSize size;
                                    size.check_field = EGenericEngineConfigure_VideoDisplay_UpdateHighLightenWindowSize;
                                    size.window_width = highlighten_window_width;
                                    size.window_height = highlighten_window_height;

                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoDisplay_UpdateHighLightenWindowSize, &size);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateHighLightenWindowSize to %hux%hu, success!\n", highlighten_window_width, highlighten_window_height);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateHighLightenWindowSize to %hux%hu, fail, err %s!\n", highlighten_window_width, highlighten_window_height, gfGetErrorCodeString(err));
                                    }
                                } else {
                                    test_loge("[user cmd example]: --highlight need followed by highlighten window width height type %%hu %%hu\n");
                                }
                            } else if (!strncmp("--voutmask", p_buffer, strlen("--voutmask"))) {
                                SVideoPostPDisplayMask mask;
                                TUint new_vout_mask = DCAL_BITMASK(EDisplayDevice_HDMI);

                                if (1 == sscanf(p_buffer, "--voutmask %x", &new_vout_mask)) {
                                    test_log("[user cmd parse]: '--voutmask %x'\n", new_vout_mask);
                                    mask.check_field = EGenericEngineConfigure_VideoDisplay_UpdateDisplayDeviceMask;
                                    mask.new_display_vout_mask = new_vout_mask;

                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoDisplay_UpdateDisplayDeviceMask, &mask);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayDeviceMask %d, success!\n", new_vout_mask);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayDeviceMask %d, fail, err %s!\n", new_vout_mask, gfGetErrorCodeString(err));
                                    }
                                }
                            } else if (!strncmp("--layer", p_buffer, strlen("--layer"))) {
                                SVideoPostPDisplayLayer layer;
                                TUint new_display_layer = 0;

                                if (1 == sscanf(p_buffer, "--layer %d", &new_display_layer)) {
                                    test_log("[user cmd parse]: '--layer %d'\n", new_display_layer);
                                    layer.check_field = EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer;
                                    layer.request_display_layer = new_display_layer;

                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer, &layer);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer %d, success!\n", new_display_layer);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer %d, fail, err %s!\n", new_display_layer, gfGetErrorCodeString(err));
                                    }
                                }
                            } else if (!strncmp("--loopmode", p_buffer, strlen("--loopmode"))) {
                                SPipeLineLoopMode loop_mode;
                                TUint new_loop_mode = 0;
                                TU16 source_index = 0;

                                if (2 == sscanf(p_buffer, "--loopmode %hu %u", &source_index, &new_loop_mode)) {
                                    test_log("[user cmd parse]: '--loopmode %hu %u\n", source_index, new_loop_mode);

                                    if (source_index >= g_media_processor_context.source_url_count_in_group[g_cur_playback_url_group]) {
                                        test_logw("BAD params, source_index %hu\n", source_index);
                                        break;
                                    }

                                    loop_mode.check_field = EGenericEngineConfigure_VideoPlayback_LoopMode;
                                    loop_mode.pb_pipeline = g_media_processor_context.playback_pipelines[source_index];
                                    loop_mode.loop_mode = new_loop_mode;

                                    err = p_engine->GenericControl(EGenericEngineConfigure_VideoPlayback_LoopMode, &loop_mode);
                                    if (EECode_OK == err) {
                                        test_log("[user cmd result]: EGenericEngineConfigure_VideoPlayback_LoopMode source %hu mode %u, success!\n", source_index, new_loop_mode);
                                    } else {
                                        test_loge("[user cmd result]: EGenericEngineConfigure_VideoPlayback_LoopMode source %hu mode %u, fail, err %s!\n", source_index, new_loop_mode, gfGetErrorCodeString(err));
                                    }
                                } else {
                                    test_loge("[user cmd example]: --loopmode need followed by source index and loop mode as %%hu %%u, 0-no loop, 1-single source loop(default), 2-all sources loop\n");
                                }
                            } else {
                                test_logw("un-processed cmdline %s\n", p_buffer);
                            }
                        }
                        break;

                    default:
                        break;
                }
            }

        } else {
            test_loge("mudec_start_process fail(), ret %d\n", ret);
            return ret;
        }
    } else {
        test_loge("mudec_build_process_pipelines fail(), ret %d\n", ret);
        return ret;
    }

    ret = __stop_process();
    if (ret) {
        test_loge("mudec_stop_process fail(), ret %d\n", ret);
    }

    __destroy_media_processor();

    return 0;
}

static void __guess_mudec_playback_mode()
{
    TULong num = g_media_processor_context.source_sd_url_count_in_group[0];
    TULong num_hd = g_media_processor_context.source_hd_url_count_in_group[0];


    test_log("in __guess_mudec_playback_mode, num %ld, hd_num %ld\n", num, num_hd);
    if (1 == num) {
        if (!num_hd) {
            g_mtest_playbackmode = EPlaybackMode_1x1080p;
            g_mtest_dsp_dec_instances_number_1 = 1;
            g_mtest_dsp_dec_instances_number_2 = 0;
        } else if (num_hd < 5) {
            g_mtest_playbackmode = EPlaybackMode_1x1080p_4xD1;
            g_mtest_dsp_dec_instances_number_1 = 4;
            g_mtest_dsp_dec_instances_number_2 = 1;
        } else if (num_hd < 7) {
            g_mtest_playbackmode = EPlaybackMode_1x1080p_6xD1;
            g_mtest_dsp_dec_instances_number_1 = 6;
            g_mtest_dsp_dec_instances_number_2 = 1;
        } else if (num_hd < 9) {
            g_mtest_playbackmode = EPlaybackMode_1x1080p_8xD1;
            g_mtest_dsp_dec_instances_number_1 = 8;
            g_mtest_dsp_dec_instances_number_2 = 1;
        } else if (num_hd < 10) {
            g_mtest_playbackmode = EPlaybackMode_1x1080p_9xD1;
            g_mtest_dsp_dec_instances_number_1 = 9;
            g_mtest_dsp_dec_instances_number_2 = 1;
        } else {
            test_log("too many hd sources %ld\n", num_hd);
            g_mtest_playbackmode = EPlaybackMode_1x1080p_4xD1;
            g_mtest_dsp_dec_instances_number_1 = 4;
            g_mtest_dsp_dec_instances_number_2 = 1;
        }
    } else if (num < 5) {
        g_mtest_playbackmode = EPlaybackMode_1x1080p_4xD1;
        g_mtest_dsp_dec_instances_number_1 = 4;
        g_mtest_dsp_dec_instances_number_2 = 1;
    } else if (num < 7) {
        g_mtest_playbackmode = EPlaybackMode_1x1080p_6xD1;
        g_mtest_dsp_dec_instances_number_1 = 6;
        g_mtest_dsp_dec_instances_number_2 = 1;
    } else if (num < 9) {
        g_mtest_playbackmode = EPlaybackMode_1x1080p_8xD1;
        g_mtest_dsp_dec_instances_number_1 = 8;
        g_mtest_dsp_dec_instances_number_2 = 1;
    } else if (num < 10) {
        g_mtest_playbackmode = EPlaybackMode_1x1080p_9xD1;
        g_mtest_dsp_dec_instances_number_1 = 9;
        g_mtest_dsp_dec_instances_number_2 = 1;
    } else {
        test_log("too many sources %ld\n", num);
        g_mtest_playbackmode = EPlaybackMode_1x1080p_4xD1;
        g_mtest_dsp_dec_instances_number_1 = 4;
        g_mtest_dsp_dec_instances_number_2 = 1;
    }

    test_log("in __guess_mudec_playback_mode, num %ld, hd_num %ld, g_mtest_playbackmode %d, dsp_1 %d, dsp_2 %d\n", num, num_hd, g_mtest_playbackmode, g_mtest_dsp_dec_instances_number_1, g_mtest_dsp_dec_instances_number_2);
}

static void __sig_pipe(int a)
{
    test_logw("sig pipe\n");
}

TInt main(TInt argc, TChar **argv)
{
    TInt ret;

    signal(SIGPIPE,  __sig_pipe);

    //_mtest_print_version();

    init_context(&g_media_processor_context);

    if ((ret = mtest_init_params(argc, argv)) < 0) {
        test_loge("mtest: mtest_init_params fail, return %d.\n", ret);
        goto mtest_main_exit;
    } else if (ret > 0) {
        goto mtest_main_exit;
    }

    if ((EPlaybackMode_Invalid == g_mtest_playbackmode) || (EPlaybackMode_AutoDetect == g_mtest_playbackmode)) {
        __guess_mudec_playback_mode();
    }

    g_media_processor_context.enable_audio = g_mtest_config_enable_audio;
    g_media_processor_context.enable_video = g_mtest_config_enable_video;
    g_media_processor_context.debug_share_demuxer = g_mtest_share_demuxer;

    g_media_processor_context.decoder_prefetch = g_mtest_decoder_prefetch_count;
    g_media_processor_context.dsp_total_buffer_number = g_mtest_dsp_total_buffer_count;
    g_media_processor_context.dsp_prebuffer_count = g_mtest_dsp_prebuffer_count;
    g_media_processor_context.streaming_timeout_threashold = g_mtest_streaming_timeout_threashold;
    g_media_processor_context.streaming_autoretry_connect_server_interval = g_mtest_streaming_retry_interval;

    g_media_processor_context.b_set_decoder_prefetch = gb_mtest_request_decoder_prefetch_count;
    g_media_processor_context.b_set_dsp_prebuffer = gb_mtest_request_dsp_prebuffer_count;

    test_log("[ut flow]: test start ... enable audio %d, enable video %d, request(%d) prefetch %d, total buffer number %d, request(%d) dsp prebuffer %d, timeout %d, retry interval %d\n", g_media_processor_context.enable_audio, g_media_processor_context.enable_video, \
             g_media_processor_context.b_set_decoder_prefetch, \
             g_media_processor_context.decoder_prefetch, \
             g_media_processor_context.dsp_total_buffer_number, \
             g_media_processor_context.b_set_dsp_prebuffer, \
             g_media_processor_context.dsp_prebuffer_count, \
             g_media_processor_context.streaming_timeout_threashold, \
             g_media_processor_context.streaming_autoretry_connect_server_interval);

    switch (g_mtest_dspmode) {

        case EMediaDeviceWorkingMode_SingleInstancePlayback:
            g_media_processor_context.enable_playback = 1;
            ret = udec_test();
            test_log("[ut flow]: udec_test end, ret %d\n", ret);
            break;

        case EMediaDeviceWorkingMode_MultipleInstancePlayback:
            g_media_processor_context.enable_playback = 1;
            g_media_processor_context.enable_mux_file = 1;
            g_media_processor_context.enable_rtsp_streaming = 1;
            g_media_processor_context.rtsp_streaming_from_playback_source = 1;
            ret = mudec_test();
            test_log("[ut flow]: mudec_test end, ret %d\n", ret);
            break;

        case EMediaDeviceWorkingMode_MultipleInstancePlayback_Transcoding:
            g_media_processor_context.enable_playback = 1;
            g_media_processor_context.enable_mux_file = 0;
            g_media_processor_context.enable_transcoding = 1;
            g_media_processor_context.enable_rtsp_streaming = 1;
            g_media_processor_context.rtsp_streaming_from_playback_source = 1;
            g_media_processor_context.rtsp_streaming_from_dsp_transcoder = 1;
            g_media_processor_context.enable_audio_pin_mux = 1;
            if (!g_mtest_config_enable_audio) {
                g_media_processor_context.enable_audio_pin_mux = 0;
            }
            g_media_processor_context.enable_cloud_server_nvr = 1;
            test_log("[ut flow]: before mudec_test(), [enable trancoding]\n", ret);
            ret = mudec_test();
            test_log("[ut flow]: mudec_test() end, ret %d, [enable trancoding]\n", ret);
            break;

        case EMediaDeviceWorkingMode_NVR_RTSP_Streaming:
            g_media_processor_context.enable_mux_file = 0;
            g_media_processor_context.enable_rtsp_streaming = 1;
            g_media_processor_context.rtsp_streaming_from_playback_source = 1;
            generic_nvr_test();
            break;

        default:
            test_loge("BAD workmode %d\n", g_mtest_dspmode);
            break;
    }

    test_log("[ut flow]: test end\n");

mtest_main_exit:

    release_content(&g_media_processor_context);

    return 0;
}

