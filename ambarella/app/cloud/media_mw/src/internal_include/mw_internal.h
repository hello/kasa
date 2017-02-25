/*******************************************************************************
 * mw_internal.h
 *
 * History:
 *    2012/06/08 - [Zhi He] create file
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

#ifndef __MW_INTERNAL_H__
#define __MW_INTERNAL_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//filename handling
enum {
    eFileNameHandling_noAppendExtention,//have '.'+'known externtion', like "xxx.mp4", "xxx.3gp", "xxx.ts"
    eFileNameHandling_appendExtention,//have no '.' + 'known externtion'
};

enum {
    eFileNameHandling_noInsert,
    eFileNameHandling_insertFileNumber,//have '%', and first is '%d' or '%06d', like "xxx_%d.mp4", "xxx_%06d.mp4"
    eFileNameHandling_insertDateTime,//have '%t', and first is '%t', will insert datetime, like "xxx_%t.mp4" ---> "xxx_20111223_115503.mp4"
};

//hard code in this demuxer
enum {
    EDemuxerVideoOutputPinIndex = 0x0,
    EDemuxerAudioOutputPinIndex = 0x1,
    EDemuxerSubtitleOutputPinIndex = 0x2,
    EDemuxerPrivateDataOutputPinIndex = 0x3,

    EDemuxerOutputPinNumber = 0x4,
};

enum {
    ECloudConnecterAgentVideoPinIndex = 0x0,
    ECloudConnecterAgentAudioPinIndex = 0x1,
    ECloudConnecterAgentSubtitlePinIndex = 0x2,
    ECloudConnecterAgentPrivateDataPinIndex = 0x3,

    ECloudConnecterAgentPinNumber = 0x4,
};

//for simple communication
enum {
    CONTROL_CMD_QUIT = 0,
    CONTROL_CMD_BUFFER_NOTIFY,
    CONTROL_CMD_SPEEDUP,
};

enum {
    DATA_THREAD_STATE_READ_FIRST_RTP_PACKET = 0,
    DATA_THREAD_STATE_READ_REMANING_RTP_PACKET,
    DATA_THREAD_STATE_WAIT_OUTBUFFER,
    DATA_THREAD_STATE_SKIP_DATA,
    DATA_THREAD_STATE_READ_FRAME_HEADER,
    DATA_THREAD_STATE_READ_RTP_HEADER,
    DATA_THREAD_STATE_READ_RTP_VIDEO_HEADER,
    DATA_THREAD_STATE_READ_RTP_AUDIO_HEADER,
    DATA_THREAD_STATE_READ_RTP_VIDEO_DATA,
    DATA_THREAD_STATE_READ_RTP_AAC_HEADER,
    DATA_THREAD_STATE_READ_RTP_AUDIO_DATA,
    DATA_THREAD_STATE_READ_RTCP,
    DATA_THREAD_STATE_COMPLETE,
    DATA_THREAD_STATE_ERROR,
};

#define DMaxFileExterntionLength 32
#define DMaxFileIndexLength 32

enum {
    RTP_VERSION = 2,

    RTP_PT_G711_PCMU = 0,
    RTP_PT_G723 = 4,
    RTP_PT_G711_PCMA = 8,
    RTP_PT_G722 = 9,
    RTP_PT_G728 = 15,
    RTP_PT_G729 = 18,

    RTP_PT_JPG = 26,
    RTP_PT_H261 = 31,
    RTP_PT_MPV = 32,
    RTP_PT_MP2T = 33,
    RTP_PT_H263 = 34,

    RTP_PAYLOAD_TYPE_PRIVATE = 96,
    RTP_PT_H264 = RTP_PAYLOAD_TYPE_PRIVATE,
    RTP_PT_AAC,
};

enum {
    MaxDTSDrift = 4,

    //check preset flag
    EPresetCheckFlag_videoframecount = 0x1,
    EPresetCheckFlag_audioframecount = 0x2,
    EPresetCheckFlag_filesize = 0x4,
};

enum {
    RTCP_SR     = 200,
    RTCP_RR     = 201,
    RTCP_SDES   = 202,
    RTCP_BYE    = 203,
    RTCP_APP    = 204,
};

#define DRecommandMaxUDPPayloadLength 1440
#define DRecommandMaxRTPPayloadLength (DRecommandMaxUDPPayloadLength - 32)

#define DSRTING_RTSP_CLIENT_TAG     "User-Agent: "DCorpLOGO" RTSP Client v20141214\r\n"
#define DSRTING_RTSP_SERVER_TAG     "RTSP Server: "DCorpLOGO" Cloud (RTSP) Server v20141214"
#define DSRTING_RTSP_SERVER_SDP_TAG     "Session streamed by \""DCorpLOGO" Cloud (RTSP) Server\""
#define DSRTING_RTSP_REALM DCorpLOGO" Cloud (RTSP) Server"

#define DRTP_OVER_RTSP_MAGIC 0x24

#define DNTP_OFFSET 2208988800ULL
#define DNTP_OFFSET_US (DNTP_OFFSET * 1000000ULL)

//internal funtions
TU8 *__validStartPoint(TU8 *start, TUint &size);
ContainerType __guessMuxContainer(TChar *p);

class IFilter;

typedef struct {
    TGenericID id;

    TComponentIndex index;
    TComponentType type;
    TU8 cur_state;

    //debug use
    TU8 playback_pipeline_number;
    TU8 recording_pipeline_number;
    TU8 streaming_pipeline_number;
    TU8 cloud_pipeline_number;

    TU8 native_capture_pipeline_number;
    TU8 native_push_pipeline_number;
    TU8 cloud_upload_pipeline_number;
    TU8 reserved1;

    TChar prefer_string[DMaxPreferStringLen];

    volatile TU32 status_flag;
    volatile TU32 active_count;

    IFilter *p_filter;

    TChar *url;
    TMemSize url_buffer_size;

    void *p_external_context;

    TChar *remote_url;
    TMemSize remote_url_buffer_size;

    TU16 local_port;
    TU16 remote_port;

    //for muxer
    TChar *channel_name;
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
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;

    TU8 config_running_at_startup;
    TU8 video_enabled;
    TU8 audio_enabled;
    TU8 is_used;

    TGenericID video_source_id[EMaxSourceNumber_StreamingPipeline];
    TGenericID video_source_connection_id[EMaxSourceNumber_StreamingPipeline];
    TGenericID audio_source_id[EMaxSourceNumber_StreamingPipeline];
    TGenericID audio_source_connection_id[EMaxSourceNumber_StreamingPipeline];
    TGenericID flow_controller_id;
    TGenericID video_flow_controller_connection_id[EMaxSourceNumber_StreamingPipeline];
    TGenericID audio_flow_controller_connection_id[EMaxSourceNumber_StreamingPipeline];
    TGenericID data_transmiter_id;
    TGenericID server_id;

    SComponent *p_server;
    SComponent *p_data_transmiter;
    SComponent *p_flow_controller;
    SConnection *p_video_flow_controller_connection[EMaxSourceNumber_StreamingPipeline];
    SConnection *p_audio_flow_controller_connection[EMaxSourceNumber_StreamingPipeline];
    SComponent *p_video_source[EMaxSourceNumber_StreamingPipeline];
    SConnection *p_video_source_connection[EMaxSourceNumber_StreamingPipeline];
    SComponent *p_audio_source[EMaxSourceNumber_StreamingPipeline];
    SConnection *p_audio_source_connection[EMaxSourceNumber_StreamingPipeline];
    SConnection *p_audio_pinmuxer_connection[EMaxSourceNumber_StreamingPipeline];

    TChar *url;
    TMemSize url_buffer_size;
} SVODPipeline;

const TChar *gfGetRTPRecieverStateString(TU32 state);

typedef struct {
    ENavigationPosition position;
    TTime target;
} SInternalCMDSeek;

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif
