/*******************************************************************************
 * media_mw_if.h
 *
 * History:
 *  2012/12/12 - [Zhi He] create file
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

#ifndef __MEDIA_MW_IF_H__
#define __MEDIA_MW_IF_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

#define DMaxChannelNumberPerGroup 16
#define DMaxUrlLen 512

#ifdef BUILD_MODULE_FFMPEG
#define DDefaultDemuxerModule DNameTag_FFMpeg
#else
#define DDefaultDemuxerModule DNameTag_PrivateTS
#endif

#define DDefaultMuxerModule DNameTag_PrivateMP4

#ifdef BUILD_MODULE_OPTCODEC_DEC
#define DDefaultVDModule DNameTag_OPTCDec
#define DDefaultADModule DNameTag_OPTCDec
#else
#define DDefaultVDModule DNameTag_FFMpeg
#define DDefaultADModule DNameTag_FFMpeg
#endif

#ifdef BUILD_MODULE_OPTCODEC_ENC_AVC
#define DDefaultVEModule DNameTag_OPTCAvc
#else
#define DDefaultVEModule DNameTag_X264
#endif

#ifdef BUILD_MODULE_OPTCODEC_ENC_AAC
#define DDefaultAEModule DNameTag_OPTCAac
#else
#define DDefaultAEModule DNameTag_FAAC
#endif

#define DDefaultVCModule DNameTag_WDup
#define DDefaultACModule DNameTag_MMD

#define DRemoteControlString "remote_control"

#define DRTP_HEADER_FIXED_LENGTH 12

typedef enum {
    EMediaDeviceWorkingMode_Invalid = 0,
    EMediaDeviceWorkingMode_SingleInstancePlayback = 0x01,
    EMediaDeviceWorkingMode_MultipleInstancePlayback = 0x02,
    EMediaDeviceWorkingMode_MultipleInstancePlayback_Transcoding = 0x03,
    EMediaDeviceWorkingMode_MultipleInstanceRecording = 0x04,
    EMediaDeviceWorkingMode_FullDuplex_SingleRecSinglePlayback = 0x05,
    EMediaDeviceWorkingMode_FullDuplex_SingleRecMultiplePlayback = 0x06,
    EMediaDeviceWorkingMode_NVR_RTSP_Streaming = 0x07,

    EMediaDeviceWorkingMode_Customized = 0x20,
} EMediaWorkingMode;

typedef enum {
    EPlaybackMode_Invalid = 0,
    EPlaybackMode_AutoDetect = 0x01,
    EPlaybackMode_1x1080p = 0x02,
    EPlaybackMode_1x1080p_4xD1 = 0x03,
    EPlaybackMode_1x1080p_6xD1 = 0x04,
    EPlaybackMode_1x1080p_8xD1 = 0x05,
    EPlaybackMode_1x1080p_9xD1 = 0x06,
    EPlaybackMode_4x720p = 0x07,
    EPlaybackMode_4xD1 = 0x08,
    EPlaybackMode_6xD1 = 0x09,
    EPlaybackMode_8xD1 = 0x0a,
    EPlaybackMode_9xD1 = 0x0b,
    EPlaybackMode_1x3M = 0x0c,
    EPlaybackMode_1x3M_4xD1 = 0x0d,
    EPlaybackMode_1x3M_6xD1 = 0x0e,
    EPlaybackMode_1x3M_8xD1 = 0x0f,
    EPlaybackMode_1x3M_9xD1 = 0x10,

    ENoPlaybackInstance = 0x50,
} EPlaybackMode;

typedef enum {
    EDisplayDevice_LCD = 0x00,
    EDisplayDevice_HDMI = 0x01,

    EDisplayDevice_TotCount,
} EDisplayDevice;

typedef enum {
    EInputDevice_FrontCamera = 0x00,
    EInputDevice_RearCamera = 0x01,

    EInputDevice_TotCount,
} EInputDevice;

//some constant
enum {
    InvalidAutoSavingParam = 0,
    DefaultAutoSavingParam = 180,
    //h264 encoder related
    DefaultH264M = 1,
    DefaultH264N = 15,
    DefaultH264IDRInterval = 4,
    //some auto save related
    DefaultMaxFileNumber = 1000,
    DefaultTotalFileNumber = 0, //0, no limit
    //private data related
    DefaultPridataDuration = DDefaultTimeScale,

    //main window's dimension
    DefaultMainWindowWidth = 1280,
    DefaultMainWindowHeight = 720,

    //video related
    DefaultMainVideoWidth = 1280,
    DefaultMainVideoHeight = 720,
    DefaultMainVideoOffset_x = 0,
    DefaultMainVideoOffset_y = 0,
    DefaultMainBitrate = 10000000,
    DefaultPreviewCWidth = 320,
    DefaultPreviewCHeight = 240,

    //rtsp server
    DefaultRTSPServerPort = 554,
    DefaultRTPServerPortBase = 20022,
    //rtsp client
    DefaultRTPClientPortBegin = 10022,
    DefaultRTPClientPortEnd = 19022,

    //duplex related
    DefaultDuplexH264M = 1,
    DefaultDuplexH264N = 15,
    DefaultDuplexH264IDRInterval = 2,
    DefaultDuplexMainBitrate = 8000000,
    DefaultDuplexPlaybackVoutIndex = EDisplayDevice_HDMI,
    DefaultDuplexPreviewVoutIndex = EDisplayDevice_HDMI,
    DefaultDuplexPreviewEnabled = 1,
    DefaultDuplexPlaybackDisplayEnabled = 1,
    DefaultDuplexPreviewAlpha = 240,
    DefaultDuplexPreviewInPIP = 1,

    DefaultDuplexPreviewLeft = 0,
    DefaultDuplexPreviewTop = 0,
    DefaultDuplexPreviewWidth = 320,
    DefaultDuplexPreviewHeight = 180,

    DefaultDuplexPbDisplayLeft = 0,
    DefaultDuplexPbDisplayTop = 0,
    DefaultDuplexPbDisplayWidth = 1280,
    DefaultDuplexPbDisplayheight = 720,

    //thumbnail related, wqvga?
    DefaultThumbnailWidth = 480,
    DefaultThumbnailHeight = 272,

    //dsp piv related
    DefaultJpegWidth = 320,
    DefaultJpegHeight = 240,

    //pb speed
    InvalidPBSpeedParam = 0xff,

    AllTargetTag = 0xff,
};

enum {
    EGenericPipelineState_not_inited = 0x0,
    EGenericPipelineState_build_gragh = 0x1,
    //EGenericPipelineState_idle = 0x2,
    EGenericPipelineState_running = 0x3,
    EGenericPipelineState_suspended = 0x4,
};

typedef enum {
    EUserParamType_Invalid = 0x0000,

    EUserParamType_QueryStatus = 0x0001,
    EUserParamType_VideoPlayback_Zoom_OnDisplay = 0x0002,

    EUserParamType_UpdateDisplayLayout = 0x0010,
    EUserParamType_DisplayZoom = 0x0011,
    EUserParamType_MoveWindow = 0x0012,
    EUserParamType_SwapWindowContent = 0x0013,
    EUserParamType_DisplayShowOtherWindows = 0x0014,

    EUserParamType_SwitchGroup = 0x0020,
    EUserParamType_QueryRecordedTimeline = 0x0021,
    EUserParamType_AddGroup = 0x0022,

    EUserParamType_AudioUpdateTargetMask = 0x0030,
    EUserParamType_AudioUpdateSourceMask = 0x0031,
    EUserParamType_AudioStartTalkTo = 0x0032,
    EUserParamType_AudioStopTalkTo = 0x0033,
    EUserParamType_AudioStartListenFrom = 0x0034,
    EUserParamType_AudioStopListenFrom = 0x0035,

    EUserParamType_VideoTranscoderUpdateBitrate = 0x0040,
    EUserParamType_VideoTranscoderUpdateFramerate = 0x0041,
    EUserParamType_VideoTranscoderDemandIDR = 0x0042,

    EUserParamType_PlaybackCapture = 0x0080,
    EUserParamType_PlaybackTrickplay = 0x0081,
    EUserParamType_PlaybackFastFwBw = 0x0082,
    EUserParamType_PlaybackSeek = 0x0083,

} EUserParamType;

typedef struct {
    //actual
    TU32 window_pos_x, window_pos_y;
    TU32 window_width, window_height;
    TU32 source_width, source_height;
    TU16 udec_index, reserved0;
    TU8 render_index, reserved1, reserved2, reserved3;
} SDisplayInfo;

typedef struct {
    TU32 check_field;//user cmd type

    TU16 total_window_number;
    TU16 current_display_window_number;
    TU32 enc_w;
    TU32 enc_h;
    TU16 vout_w;
    TU16 vout_h;
    SDisplayInfo window[DMaxChannelNumberPerGroup];
} SUserParamType_QueryStatus;

typedef struct {
    //check field
    TU32 check_field;

    TU8 render_id;
    TU8 decoder_id, reserved1, reserved2;

    //paras based on display windows
    TU16 input_center_x;
    TU16 input_center_y;
    TU16 input_width;
    TU16 input_height;
    TU16 window_width;
    TU16 window_height;
} SUserParamType_Zoom_OnDisplay;

typedef struct {
    TU32 check_field;//user cmd type

    TU32 layout;
    TU32 focus_stream_index;
} SUserParamType_UpdateDisplayLayout;

typedef struct {
    TU32 check_field;//user cmd type

    TU32 window_id;
    // accumulating one
    float zoom_factor;
    float gesture_offset_x;
    float gesture_offset_y;
} SUserParamType_DisplayZoom;

typedef struct {
    TU32 check_field;//user cmd type

    TU32 window_id;

    float offset_x;
    float offset_y;

    float size_x;
    float size_y;
} SUserParamType_MoveWindow;

typedef struct {
    TU32 check_field;//user cmd type

    TU32 window_id_0;
    TU32 window_id_1;
} SUserParamType_SwapWindowContent;

typedef struct {
    TU32 check_field;//user cmd type

    TU32 window_id;
    TU32 show_others;
} SUserParamType_DisplayShowOtherWindows;

typedef struct {
    TU32 check_field;//user cmd type

    TU32 group_id;
    TU32 group_type;// 0 is play live, 1 is play recorded
} SUserParamType_SwitchGroup;

typedef struct {
    TU32 check_field;//user cmd type
    TU32 group_id;
    TU32 source_channel_number;
    TU32 source_2rd_channel_number;
    TChar source_url[DMaxChannelNumberPerGroup][DMaxUrlLen];
    TChar source_url_2rd[DMaxChannelNumberPerGroup][DMaxUrlLen];
} SUserParamType_AddGroup;

typedef struct {
    TU32 check_field;//user cmd type

    TU32 group_id;
    SDateTime begin_datatime;

    TU32 timeline_unit;//how many seconds
    TU32 timeline_total_count;

    TU8 *p_timeline_mask;//need to be free
} SUserParamType_QueryRecordedTimeline;

typedef struct {
    TU32 check_field;//user cmd type

    TU32 audio_target_mask;
} SUserParamType_AudioUpdateTargetMask;

typedef struct {
    TU32 check_field;//user cmd type

    TU32 audio_target_mask;
} SUserParamType_AudioUpdateSourceMask;

typedef struct {
    TU32 check_field;//user cmd type

} SUserParamType_AudioStartTalkTo;

typedef struct {
    TU32 check_field;//user cmd type

} SUserParamType_AudioStopTalkTo;

typedef struct {
    TU32 check_field;//user cmd type

} SUserParamType_AudioStartListenFrom;

typedef struct {
    TU32 check_field;//user cmd type

} SUserParamType_AudioStopListenFrom;

typedef struct {
    TU32 check_field;//user cmd type

    TU32 bitrate;
} SUserParamType_VideoTranscoderUpdateBitrate;

typedef struct {
    TU32 check_field;//user cmd type

    TU32 framerate;
} SUserParamType_VideoTranscoderUpdateFramerate;

typedef struct {
    TU32 check_field;//user cmd type

    TU32 param0;
} SUserParamType_VideoTranscoderDemandIDR;

typedef struct {
    TU32 check_field;//user cmd type

    TU8 capture_screennail;
    TU8 capture_thumbnail;
    TU8 capture_coded;
    TU8 file_index;

    TU8 channel_id;
    TU8 source_id;
    TU8 reserved1;
    TU8 reserved2;

    TU32 screennail_width;
    TU32 screennail_height;

    TU32 thumbnail_width;
    TU32 thumbnail_height;

    TU32 coded_width;
    TU32 coded_height;
} SUserParamType_PlaybackCapture;

enum {
    SUserParamType_PlaybackTrickplay_Resume = 0x0,
    SUserParamType_PlaybackTrickplay_Pause = 0x1,
    SUserParamType_PlaybackTrickplay_Step = 0x2,
    SUserParamType_PlaybackTrickplay_PauseResume = 0x3,
} ;

typedef struct {
    TU32 check_field;//user cmd type

    TU8 use_channel_id;
    TU8 do_all;
    TU8 trickplay;
    TU8 reserved1;

    TU32 index;
} SUserParamType_PlaybackTrickplay;

typedef struct {
    TU32 check_field;//user cmd type

    TU8 use_channel_id;
    TU8 do_all;
    TU8 is_forward;
    TU8 feeding_mode;

    TU32 index;

    TU16 speed;
    TU16 speed_frac;
} SUserParamType_PlaybackFastFwBw;

typedef struct {
    TU32 check_field;//user cmd type

    TU8 at_current_file;
    TU8 do_all;
    TU8 reserved0;
    TU8 reserved1;

    TTime target_time;

    SDateTime target_date_time;
} SUserParamType_PlaybackSeek;

#define DInvalidGeneralIntParam 0xffffffff
//some preset save related

#define DCOMPONENT_TYPE_IN_GID 24
#define DCOMPONENT_VALID_MARKER_BITS 23
#define DCOMPONENT_INDEX_MASK_IN_GID 0xffff
typedef TU16 TComponentIndex;
typedef TU8 TComponentType;

#define DCOMPONENT_TYPE_FROM_GENERIC_ID(x) (x >> DCOMPONENT_TYPE_IN_GID)
#define DCOMPONENT_INDEX_FROM_GENERIC_ID(x) (x & DCOMPONENT_INDEX_MASK_IN_GID)
#define DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(t, i) ((t << DCOMPONENT_TYPE_IN_GID) | (i & DCOMPONENT_INDEX_MASK_IN_GID) | (1 << DCOMPONENT_VALID_MARKER_BITS))
#define IS_VALID_COMPONENT_ID(x) (x & DCAL_BITMASK(DCOMPONENT_VALID_MARKER_BITS))

//#define PARAM_MASK_MODE 0x80000000

#define DPrintFlagBit_dataPipeline (1<<0)
#define DPrintFlagBit_logConfigure (1<<1)

//-----------------------------------------------------------------------
//
// TGenericID
//
//-----------------------------------------------------------------------
typedef TU32 TGenericID;

enum {
    EGenericComponentType_TAG_filter_start = 0x00,
    EGenericComponentType_Demuxer = EGenericComponentType_TAG_filter_start,
    EGenericComponentType_VideoDecoder = 0x01,
    EGenericComponentType_AudioDecoder = 0x02,
    EGenericComponentType_VideoEncoder = 0x03,
    EGenericComponentType_AudioEncoder = 0x04,
    EGenericComponentType_VideoPostProcessor = 0x05,
    EGenericComponentType_AudioPostProcessor = 0x06,
    EGenericComponentType_VideoPreProcessor = 0x07,
    EGenericComponentType_AudioPreProcessor = 0x08,
    EGenericComponentType_VideoCapture = 0x09,
    EGenericComponentType_AudioCapture = 0x0a,
    EGenericComponentType_VideoRenderer = 0x0b,
    EGenericComponentType_AudioRenderer = 0x0c,
    EGenericComponentType_Muxer = 0x0d,
    EGenericComponentType_StreamingTransmiter = 0x0e,
    EGenericComponentType_ConnecterPinMuxer = 0x0f,
    EGenericComponentType_CloudConnecterServerAgent = 0x10,
    EGenericComponentType_CloudConnecterClientAgent = 0x11,
    EGenericComponentType_CloudConnecterCmdAgent = 0x12,
    EGenericComponentType_FlowController = 0x13,
    EGenericComponentType_VideoOutput = 0x14,
    EGenericComponentType_AudioOutput = 0x15,
    EGenericComponentType_ExtVideoESSource = 0x16,
    EGenericComponentType_ExtAudioESSource = 0x17,

    EGenericComponentType_TAG_filter_end,

    EGenericPipelineType_Playback = 0x70,
    EGenericPipelineType_Recording = 0x71,
    EGenericPipelineType_Streaming = 0x72,
    EGenericPipelineType_Cloud = 0x73,
    EGenericPipelineType_NativeCapture = 0x74,
    EGenericPipelineType_NativePush = 0x75,
    EGenericPipelineType_CloudUpload = 0x76,
    EGenericPipelineType_VOD = 0x77,

    EGenericComponentType_TAG_proxy_start = 0x80,
    EGenericComponentType_StreamingServer = EGenericComponentType_TAG_proxy_start,
    EGenericComponentType_StreamingContent = 0x81,
    EGenericComponentType_ConnectionPin = 0x82,
    EGenericComponentType_UserTimer = 0x83,
    EGenericComponentType_CloudServer = 0x84,
    EGenericComponentType_CloudReceiverContent = 0x85,

    EGenericComponentType_TAG_proxy_end,
};

//some const
enum {
    EComponentMaxNumber_Demuxer = DSYSTEM_MAX_CHANNEL_NUM,
    EComponentMaxNumber_VideoDecoder = 16,
    EComponentMaxNumber_AudioDecoder = 16 * 2,
    EComponentMaxNumber_VideoEncoder = 16,
    EComponentMaxNumber_AudioEncoder = 16,
    EComponentMaxNumber_VideoPostProcessor = 1,
    EComponentMaxNumber_AudioPostProcessor = 1,
    EComponentMaxNumber_VideoPreProcessor = 1,
    EComponentMaxNumber_AudioPreProcessor = 1,
    EComponentMaxNumber_VideoCapture = 1,
    EComponentMaxNumber_AudioCapture = 1,
    EComponentMaxNumber_VideoRenderer = 1,
    EComponentMaxNumber_AudioRenderer = 1,
    EComponentMaxNumber_Muxer = DSYSTEM_MAX_CHANNEL_NUM,
    EComponentMaxNumber_RTSPStreamingTransmiter = DSYSTEM_MAX_CHANNEL_NUM,
    EComponentMaxNumber_AudioPinMuxer = 2,
    EComponentMaxNumber_CloudConnecterServerAgent = DSYSTEM_MAX_CHANNEL_NUM,
    EComponentMaxNumber_CloudConnecterClientAgent = 16,
    EComponentMaxNumber_CloudConnecterCmdAgent = 256,
    EComponentMaxNumber_FlowController = DSYSTEM_MAX_CHANNEL_NUM,
    EComponentMaxNumber_VideoOutput = 32,
    EComponentMaxNumber_AudioOutput = 32,
    EComponentMaxNumber_VideoExtSource = 32,
    EComponentMaxNumber_AudioExtSource = 32,

    EComponentMaxNumber_RTSPStreamingServer = 1,
    EComponentMaxNumber_RTSPStreamingContent = DSYSTEM_MAX_CHANNEL_NUM * 2,
    EComponentMaxNumber_UserTimer = 128,
    EComponentMaxNumber_ConnectionPin = DSYSTEM_MAX_CHANNEL_NUM * 32,
    EComponentMaxNumber_CloudServer = 1,

    EComponentMaxNumber_CloudClientAgentReceiverFromCamera = 128,
    EComponentMaxNumber_CloudClientAgentReceiverFromAPP = 128,

    EComponentMaxNumber_CloudReceiverContent = (EComponentMaxNumber_CloudClientAgentReceiverFromCamera + EComponentMaxNumber_CloudClientAgentReceiverFromAPP),

    EPipelineMaxNumber_Playback = 32,
    EPipelineMaxNumber_Recording = DSYSTEM_MAX_CHANNEL_NUM,
    EPipelineMaxNumber_Streaming = DSYSTEM_MAX_CHANNEL_NUM,
    EPipelineMaxNumber_CloudPush = DSYSTEM_MAX_CHANNEL_NUM,

    EPipelineMaxNumber_NativeCapture = 2,
    EPipelineMaxNumber_NativePush = 32,

    EPipelineMaxNumber_CloudUpload = 2,

    EPipelineMaxNumber_VOD = DSYSTEM_MAX_CHANNEL_NUM,
};

enum {
    EMaxStreamingSessionNumber = DSYSTEM_MAX_CHANNEL_NUM * 8,
    EMaxSinkNumber_Demuxer = 32,
    EMaxSourceNumber_RecordingPipeline = 2,
    EMaxSourceNumber_StreamingPipeline = 2,
    EMaxSinkNumber_CloudPipeline = 8,
    //EMaxSinkNumber_NativePushPipeline = 8,

    EMaxSourceNumber_Muxer = 4,
    EMaxSourceNumber_Streamer = 32,
};

enum TimeUnit {
    TimeUnitDen_90khz = 90000,
    TimeUnitDen_27mhz = 27000000,
    TimeUnitDen_ms = 1000,
    TimeUnitDen_us = 1000000,
    TimeUnitDen_ns = 1000000000,
    TimeUnitNum_fps29dot97 = 3003,
};

enum VideoFrameRate {
    VideoFrameRate_MAX = 1024,
    VideoFrameRate_29dot97 = VideoFrameRate_MAX + 1,
    VideoFrameRate_59dot94,
    VideoFrameRate_23dot96,
    VideoFrameRate_240,
    VideoFrameRate_60,
    VideoFrameRate_30,
    VideoFrameRate_24,
    VideoFrameRate_15,
};

enum StreamTransportType {
    StreamTransportType_Invalid = 0,
    StreamTransportType_RTP,
};

enum ProtocolType {
    ProtocolType_Invalid = 0,
    ProtocolType_UDP,
    ProtocolType_TCP,
};

enum StreamingServerType {
    StreamingServerType_Invalid = 0,
    StreamingServerType_RTSP,
    StreamingServerType_RTMP,
    StreamingServerType_HTTP,
};

enum StreamingServerMode {
    StreamingServerMode_MulticastSetAddr = 0, //live streamming/boardcast
    StreamingServerMode_Unicast, //vod
    StreamingServerMode_MultiCastPickAddr, //join conference
};

enum StreamingContentType {
    StreamingContentType_Live = 0,
    StreamingContentType_LocalFile,
    StreamingContentType_Conference,
};

enum CloudServerType {
    CloudServerType_Invalid = 0,
    CloudServerType_SACP,
    CloudServerType_ONVIF,
};

enum ContainerType {
    ContainerType_Invalid = 0,
    ContainerType_AUTO = 1,
    ContainerType_MP4,
    ContainerType_3GP,
    ContainerType_TS,
    ContainerType_MOV,
    ContainerType_MKV,
    ContainerType_AVI,
    ContainerType_AMR,

    ContainerType_TotolNum,
};

enum StreamType {
    StreamType_Invalid = 0,
    StreamType_Video,
    StreamType_Audio,
    StreamType_Subtitle,
    StreamType_PrivateData,

    StreamType_Cmd,

    StreamType_TotalNum,
};

enum StreamFormat {
    StreamFormat_Invalid = 0,
    StreamFormat_H264 = 0x01,
    StreamFormat_VC1 = 0x02,
    StreamFormat_MPEG4 = 0x03,
    StreamFormat_WMV3 = 0x04,
    StreamFormat_MPEG12 = 0x05,
    StreamFormat_HybridMPEG4 = 0x06,
    StreamFormat_HybridRV40 = 0x07,
    StreamFormat_VideoSW = 0x08,
    StreamFormat_JPEG = 0x09,
    StreamFormat_AAC = 0x0a,
    StreamFormat_MPEG12Audio = 0x0b,
    StreamFormat_MP2 = 0x0c,
    StreamFormat_MP3 = 0x0d,
    StreamFormat_AC3 = 0x0e,
    StreamFormat_ADPCM = 0x0f,
    StreamFormat_AMR_NB = 0x10,
    StreamFormat_AMR_WB = 0x11,
    StreamFormat_PCMU = 0x12,
    StreamFormat_PCMA = 0x13,
    StreamFormat_H265 = 0x14,

    StreamFormat_PCM_S16 = 0x30,

    StreamFormat_CustomizedADPCM_1 = 0x40,
    StreamFormat_CustomizedADPCM_2 = 0x41,
    StreamFormat_PrivateData = 0x42,
    StreamFormat_AmbaTrickPlayData = 0x43,
    StreamFormat_GPSInfo = 0x44,

    StreamFormat_FFMpegCustomized = 0x60,

    StreamFormat_OCodecCustomized = 0x60,

    //tmp here
    StreamFormat_AudioFlowControl_Pause = 0xa0,
    StreamFormat_AudioFlowControl_EOS = 0xa1,
};

enum PixFormat {
    PixFormat_YUV420P = 0,
    PixFormat_NV12,
    PixFormat_RGB565,
};

enum DataCategory {
    DataCategory_PrivateData = 0,
    DataCategory_SubTitleString,
    DataCategory_SubTitleRaw,
    DataCategory_VideoES,
    DataCategory_AudioES,
    DataCategory_VideoYUV420p,
    DataCategory_VideoNV12,
    DataCategory_AudioPCM,
};

enum PrivateDataType {
    PrivateDataType_GPSInfo = 0,
    PrivateDataType_SensorInfo,
    //todo
};

enum EntropyType {
    EntropyType_NOTSet = 0,
    EntropyType_H264_CABAC,
    EntropyType_H264_CAVLC,
};

enum AudioSampleFMT {
    AudioSampleFMT_NONE = -1,
    AudioSampleFMT_U8,          ///< unsigned 8 bits
    AudioSampleFMT_S16,         ///< signed 16 bits
    AudioSampleFMT_S32,         ///< signed 32 bits
    AudioSampleFMT_FLT,         ///< float
    AudioSampleFMT_DBL,         ///< double
    AudioSampleFMT_NB           ///< Number of sample formats. DO NOT USE if linking dynamically
};

enum MuxerSavingFileStrategy {
    MuxerSavingFileStrategy_Invalid = 0x00,
    MuxerSavingFileStrategy_AutoSeparateFile = 0x01, //muxer will auto separate file itself, with more time accuracy
    MuxerSavingFileStrategy_ManuallySeparateFile = 0x02,//engine/app will management the separeting file
    MuxerSavingFileStrategy_ToTalFile = 0x03,//not recommanded, cannot store file like .mp4 if storage have not enough space
};

enum MuxerSavingCondition {
    MuxerSavingCondition_Invalid = 0x00,
    MuxerSavingCondition_InputPTS = 0x01,
    MuxerSavingCondition_CalculatedPTS = 0x02,
    MuxerSavingCondition_FrameCount = 0x03,
};

enum MuxerAutoFileNaming {
    MuxerAutoFileNaming_Invalid = 0x00,
    MuxerAutoFileNaming_ByDateTime = 0x01,
    MuxerAutoFileNaming_ByNumber = 0x02,
};

enum DecoderFeedingRule {
    DecoderFeedingRule_NotValid = 0,
    DecoderFeedingRule_AllFrames,
    DecoderFeedingRule_RefOnly,
    DecoderFeedingRule_IOnly,
    DecoderFeedingRule_IDROnly,
};

typedef enum {
    ENavigationPosition_Invalid = 0,
    ENavigationPosition_Begining,
    ENavigationPosition_End,
    ENavigationPosition_LastKeyFrame,
} ENavigationPosition;

enum OSDInputDataFormat {
    OSDInputDataFormat_RGBA = 0,
    OSDInputDataFormat_YUVA_CLUT,
};

typedef struct {
    TUint pic_width;
    TUint pic_height;
    TUint pic_offset_x;
    TUint pic_offset_y;
    TUint framerate_num;
    TUint framerate_den;
    float framerate;
    TUint M, N, IDRInterval;//control P, I, IDR percentage
    TUint sample_aspect_ratio_num;
    TUint sample_aspect_ratio_den;
    TUint bitrate;
    TUint lowdelay;
    EntropyType entropy_type;
} SVideoParams;

typedef struct {
    TGenericID id;
    TUint index;
    StreamFormat format;
    TUint pic_width;
    TUint pic_height;
    TUint bitrate;
    TU64 bitrate_pts;
    float framerate;

    TU8 gop_M;
    TU8 gop_N;
    TU8 gop_idr_interval;
    TU8 gop_structure;
    TU64 demand_idr_pts;
    TU8 demand_idr_strategy;
    TU8 framerate_reduce_factor;
    TU8 framerate_integer;
    TU8 reserved0;
} SVideoEncoderParam;

typedef struct {
    TDimension cap_win_offset_x, cap_win_offset_y;
    TDimension cap_win_width, cap_win_height;
    TDimension screen_width, screen_height;
    TDimension cap_buffer_linesize;

    float framerate;
    TU32 framerate_num;
    TU32 framerate_den;

    EPixelFMT pixel_format;
} SVideoCaptureParams;

typedef struct {
    TUint sample_rate;
    AudioSampleFMT sample_format;
    TUint channel_number;
    TUint channel_layout;
    TUint frame_size;
    TUint bitrate;
    TUint need_skip_adts_header;
    TUint pts_unit_num;//pts's unit
    TUint pts_unit_den;

    TU8 is_channel_interlave;
    TU8 is_big_endian;
    TU8 reserved0, reserved1;

    TU32 codec_format;
    TU32 customized_codec_type;
} SAudioParams;

typedef struct {
    //to do
    TUint duration;
} SSubtitleParams;

typedef struct {
    //to do
    TUint duration;
} SPriDataParams;

typedef union {
    SVideoParams video;
    SAudioParams audio;
    SSubtitleParams subtitle;
    SPriDataParams pridata;
} UFormatSpecific;

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE
//to be removed
typedef struct {
    TUint stream_number;
    ContainerType out_format;

    TU8 video_number;
    TU8 audio_number;
    TU8 subtitle_number;
    TU8 pridata_number;

    TU8 video_streamming;
    TU8 audio_streamming;
    TU16 mMaxTotalFileNumber;

    TU64 mMaxFileDurationUs;
    TU64 mMaxFileSizeBytes;

    //auto cut related parameters
    MuxerSavingFileStrategy mSavingStrategy;
    MuxerSavingCondition mSavingCondition;
    MuxerAutoFileNaming mAutoNaming;
    TUint mConditionParam;
} SMuxerConfig;
#endif

typedef struct {
    TU32 engine_version;
    TU32 filter_version;
    TU32 streaming_server_version;
    TU32 cloud_server_version;
} SDebugEngineConfig;

typedef struct {
    TU8 use_tcp_mode_first;
    TU8 parse_multiple_access_unit;

    TU16 rtp_rtcp_port_start;
} SRTSPClientConfig;

typedef struct {
    TU16 rtsp_listen_port;
    TU16 rtp_rtcp_port_start;

    TU8 enable_nonblock_timeout;
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;

    TS32 timeout_threashold;
} SRTSPServerConfig;

typedef struct {
    TU16 sacp_listen_port;
    TU16 reserved0;

    TS32 timeout_threashold;
} SSACPServerConfig;

typedef struct {
    TU8 mbUseTCPPulseSender;
    TU8 mReserved0;
    TU8 mReserved1;
    TU8 mReserved2;

    //TCP sender
    TU32 mPulseTransferMemSize;
    TU16 mPulseTransferMaxFramecount;
    TU16 mReserved3;
} SNetworkTuningConfig;

typedef struct {
    TUint container_format;
    TUint video_format, audio_format;
    TUint pic_width, pic_height;
    TUint channel_number, sample_format, sample_rate;
    TUint video_bitrate, audio_bitrate;
    TUint video_framerate_num, video_framerate_den;
    TUint M, N, IDRInterval;
    TUint video_lowdelay;
    EntropyType entropy_type;
} STargetRecorderConfig;

typedef struct {
    TU32 check_field;//user cmd type

    TGenericID id;
} SUserParamType_CaptureAudioMuteControl;

//udec/dsp related parameters
#define DMaxPostPWindowNumberPerRender 4
#define DMaxPostPWindowNumber 16
#define DMaxPostPRenderNumber 16
#define DMaxUDECInstanceNumber 16
#define DMaxDisplayLayerNumber 2
#define DMaxSchedulerGroupNumber 8
#define DMaxTCPPulseSenderNumber 8

typedef enum {
    EDSPOperationMode_Invalid = 0,

    EDSPOperationMode_IDLE,
    EDSPOperationMode_UDEC,
    EDSPOperationMode_MultipleWindowUDEC,
    EDSPOperationMode_MultipleWindowUDEC_Transcode,
    EDSPOperationMode_FullDuplex,
    EDSPOperationMode_CameraRecording,
    EDSPOperationMode_Transcode,
} EDSPOperationMode;

typedef struct {
    TU8   postp_mode;   // 0: no pp; 1: decpp; 2: voutpp
    TU8   enable_deint;   // 1: enable deinterlacer
    TU8   pp_chroma_fmt_max;  // 0: mon; 1: 420; 2: 422; 3: 444
    TU8   enable_transcode;

    //enable dewarp feature
    TU8   enable_horizontal_dewarp;
    TU8   enable_vertical_dewarp;
    TU8   primary_vout;
    TU8   cur_display_vout_mask;

    TU8   vout_mask;  // bit 0: use VOUT 0; bit 1: use VOUT 1
    TU8   num_udecs;
    TU8   reserved0;
    TU8   pp_max_frm_num;

    TU16   pp_max_frm_width;
    TU16   pp_max_frm_height;

    TU8   pp_background_Y;
    TU8   pp_background_Cb;
    TU8   pp_background_Cr;
    TU8   reserved2;
} SDSPUdecModeConfig;

typedef struct {
    TU8   win_config_id;
    TU8   reserved[3];

    TU16   input_offset_x;
    TU16   input_offset_y;
    TU16   input_width;
    TU16   input_height;

    TU16   target_win_offset_x;
    TU16   target_win_offset_y;
    TU16   target_win_width;
    TU16   target_win_height;
} SDSPMUdecWindow;

typedef struct {
    TU8   render_id;
    TU8   win_config_id;
    TU8   win_config_id_2nd;
    TU8   udec_id;

    TU32   first_pts_low;
    TU32   first_pts_high;

    TU8   input_source_type;
    TU8   reserved[3];
} SDSPMUdecRender;

typedef struct {
    TU8 max_num_windows;
    TU8 total_num_render_configs;
    TU8 reserved0[2];

    TU8 total_num_win_configs;
    TU8 audio_on_win_id;
    TU8 pre_buffer_len;
    TU8 enable_buffering_ctrl : 1;
    TU8 av_sync_enabled : 1;
    TU8 voutB_enabled : 1;
    TU8 voutA_enabled : 1;
    TU8 resereved : 4;

    TU16 video_win_width; //The overall width of output video window
    TU16 video_win_height;//The overall height of output video window

    TU32 frame_rate_in_tick;
    TU32 avg_bitrate;

    SDSPMUdecWindow windows_config[DMaxPostPWindowNumber];//plus 4 for safe
    SDSPMUdecRender render_config[DMaxPostPRenderNumber];
} SDSPMUdecModeConfig;

typedef struct {
    TInt enable;//video layer
    TInt osd_disable;//osd layer
    TInt sink_type;
    TInt sink_mode;
    TInt sink_id;
    TUint width, height;
    TInt pos_x, pos_y;
    TInt size_x, size_y;
    TU32 zoom_factor_x, zoom_factor_y;
    TInt input_center_x, input_center_y;
    TInt flip, rotate;
    TInt failed;//checking/opening failed
    TInt vout_mode;// 1: interlace_mode

    TInt vout_id;//debug use
} SDSPVoutConfig;

typedef struct {
    SDSPVoutConfig voutConfig[EDisplayDevice_TotCount];

    //vout common parameters
    TUint vout_mask;
    //TUint vout_start_index;
    //TUint num_vouts;
    //TInt input_center_x, input_center_y;

    //current source rect, related to zoom_factor
    TInt src_pos_x, src_pos_y;
    TInt src_size_x, src_size_y;

    //picture related
    TInt picture_offset_x;
    TInt picture_offset_y;
    TInt picture_width;
    TInt picture_height;
} SDSPVoutConfigs;

typedef struct {
    //de-interlace parameters
    TU8 init_tff; //top_field_first   1
    TU8 deint_lu_en; // 1
    TU8 deint_ch_en; // 1
    TU8 osd_en; // 0

    TU8 deint_mode; //for 1080i60 input, you need to set '1' here, for 480i/576i input, you can set it to '0' or '1'
    TU8 deint_reserved[3];

    TU8 deint_spatial_shift; //0
    TU8 deint_lowpass_shift; //7

    TU8 deint_lowpass_center_weight; // 112
    TU8 deint_lowpass_hor_weight; // 2
    TU8 deint_lowpass_ver_weight; // 4

    TU8 deint_gradient_bias; // 15
    TU8 deint_predict_bias; // 15
    TU8 deint_candidate_bias; // 10

    TS16 deint_spatial_score_bias; // 5
    TS16 deint_temporal_score_bias; // 5
} SDSPDeinterlaceConfig;

//hard code here, for libaacenc.lib
#define AUDIO_CHUNK_SIZE 1024
#define SINGLE_CHANNEL_MAX_AUDIO_BUFFER_SIZE (AUDIO_CHUNK_SIZE*sizeof(TS16))
#define MAX_AUDIO_BUFFER_SIZE (SINGLE_CHANNEL_MAX_AUDIO_BUFFER_SIZE*2)

// config SDSPUdecInstanceConfig::frm_chroma_fmt_max
#define UDEC_CFG_FRM_CHROMA_FMT_420    0
#define UDEC_CFG_FRM_CHROMA_FMT_422    1
#define TICK_PER_SECOND  DDefaultTimeScale

typedef struct {
    TU8   tiled_mode; // tiled mode, 0, 4, 5
    TU8   frm_chroma_fmt_max; // 0: 420; 1: 422
    TU8   dec_types;
    TU8   max_frm_num;
    TU16  max_frm_width;
    TU16  max_frm_height;
    TU32  max_fifo_size;

    TU8   udec_id;
    TU8   udec_type;
    TU8   enable_pp;
    TU8   enable_deint;

    // the vout used by this udec
    TU8   interlaced_out;
    TU8   packed_out; // 0: planar yuv; 1: packed yuyv
    TU8   enable_err_handle;
    TU8   reserved;
    TU32  bits_fifo_size;
    TU32  ref_cache_size;

    TU16  concealment_mode;   // 0 - use the last good picture as the concealment reference frame
    // 1 - use the last displayed as the concealment reference frame
    // 2 - use the frame given by ARM as the concealment reference frame
    TU32  concealment_ref_frm_buf_id; // used only when concealment_mode is 2

    //out
    TU8 *pbits_fifo_start;
} SDSPUdecInstanceConfig;

typedef struct {
    //error handling config
    TU16 enable_udec_error_handling;
    TU16 error_concealment_mode;
    TU16 error_concealment_frame_id;
    TU16 error_handling_app_behavior;//debug use, 0:default(stop(1)-->restart decoding), 1:always STOP(0), and exit, 2: halt, not touch env, for debug
} SDSPUdecErrorHandlingConfig;

typedef struct {
    TU32 pquant_mode;
    TU16 pquant_table[32];
} SDSPUdecDeblockingConfig;

enum {
    eAddVideoDataType_none = 0,
    eAddVideoDataType_iOneUDEC = 1,
    eAddVideoDataType_a5sH264GOPHeader = 2,
};

typedef struct {
    SDSPUdecModeConfig modeConfig;
    SDSPMUdecModeConfig modeConfigMudec;
    SDSPDeinterlaceConfig deinterlaceConfig;
    SDSPUdecInstanceConfig udecInstanceConfig[DMaxUDECInstanceNumber];
    SDSPUdecErrorHandlingConfig errorHandlingConfig[DMaxUDECInstanceNumber];
    SDSPVoutConfigs voutConfigs;
    SDSPUdecDeblockingConfig deblockingConfig;

    // input
    TInt device_fd;
    void *p_dsp_handler;
    TUint target_dsp_type;
    TU16 request_dsp_mode;
    TU16 dsp_type;

    TU8 not_add_udec_wrapper;
    TU8 not_feed_pts;
    TU8 specified_fps;
    TU8 prefer_udec_stop_flags;

    //TU32 deblockingFlag;
    TU8   tiled_mode; // tiled mode, 0, 4, 5
    TU8   frm_chroma_fmt_max; // 0: 420; 1: 422
    TU8   max_frm_num;
    TU8   enable_deint;
    TU16  max_frm_width;
    TU16  max_frm_height;
    TU32  max_fifo_size;

    TU8   enable_pp;
    TU8   interlaced_out;
    TU8   packed_out; // 0: planar yuv; 1: packed yuyv
    TU8   enable_err_handle;

    TU32  bits_fifo_size;
    TU32  ref_cache_size;

    TU16  concealment_mode;   // 0 - use the last good picture as the concealment reference frame
    // 1 - use the last displayed as the concealment reference frame
    // 2 - use the frame given by ARM as the concealment reference frame
    TU16  concealment_ref_frm_buf_id; // used only when concealment_mode is 2
} SDSPConfig;

typedef struct {
    TU16 main_win_width;
    TU16 main_win_height;
    TU16 dsp_mode;
    TU16 stream_number;

    TU16 enc_width, enc_height;
    TU16 enc_offset_x, enc_offset_y;
    TU16 second_enc_width, second_enc_height;

    TUint M, N, IDRInterval;
    TUint video_bitrate;
    EntropyType entropy_type;

    TU8 num_of_enc_chans;
    TU8 num_of_dec_chans;
    //vout related
    TU8 playback_vout_index;
    TU8 playback_in_pip;

    TU8 preview_enabled;
    TU8 preview_vout_index;
    TU8 preview_alpha;
    TU8 preview_in_pip;

    TU16 preview_left;
    TU16 preview_top;
    TU16 preview_width;
    TU16 preview_height;

    TU16 pb_display_left;
    TU16 pb_display_top;
    TU16 pb_display_width;
    TU16 pb_display_height;

    TU8 pb_display_enabled;

    //previewc related
    TU8 previewc_rawdata_enabled;
    TU8 thumbnail_enabled;
    TU8 dsp_piv_enabled;
    TU16 previewc_scaled_width;
    TU16 previewc_scaled_height;

    TU16 previewc_crop_offset_x;
    TU16 previewc_crop_offset_y;
    TU16 previewc_crop_width;
    TU16 previewc_crop_height;

    TU16 thumbnail_width;
    TU16 thumbnail_height;

    TU16 dsp_jpeg_width;
    TU16 dsp_jpeg_height;

    TU8 vcap_ppl_type;
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;
} SEncodingModeConfig;

typedef struct {
    TU8 mPreferedMuxerModule;
    TU8 mPreferedDemuxerModule;
    TU8 mPreferedAudioDecoderModule;
    TU8 mPreferedAudioEncoderModule;

    TU8 mPreferedCloudServerType;
    TU8 mPreferedStreamingServerType;
    TU8 mPreferedAudioRendererModule;
    TU8 mPreferedReserved0;
} SPreferModuleSetting;

typedef struct {
    TU32 bitrate;
} SAudioEncodingConfig;

typedef struct {
    TDimension offset_x, offset_y;
    TDimension width, height;
    TU32 framerate_num, framerate_den;
} SVideoCaptureConfig;

typedef struct {
    TDimension width, height;
    TU32 framerate_num, framerate_den;
    TU32 bitrate;
} SVideoEncodingConfig;

typedef struct {
    TU32 sample_rate;
    TU32 channel_number;
    //TU32 bitrate;
    TU32 framesize;
    TU32 sample_size;
    TU32 sample_format;
} SAudioPreferSetting;

typedef struct {
    TU8 precache_video_frames;
    TU8 max_video_frames;
    TU8 precache_audio_frames;
    TU8 max_audio_frames;

    TU8 reserved0;//audio_latency_compensation_count;
    TU8 b_constrain_latency;
    TU8 b_render_video_nodelay;
    TU8 reserved2;
} SPlaybackCacheSetting;

typedef struct {
    TU8 prealloc_buffer_number;
    TU8 prefer_thread_number;
    TU8 prefer_parallel_frame;
    TU8 prefer_parallel_slice;

    TU8 prefer_official_reference_model_decoder;
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;
} SPlaybackDecodeSetting;

typedef struct {
    TU8 prealloc_buffer_number;
    TU8 prefer_thread_number;
    TU8 prefer_lookahead_thread_number;
    TU8 reserved0;

    TU8 prefer_parallel_frame;
    TU8 prefer_parallel_slice;
    TU8 reserved1;
    TU8 reserved2;
} SEncodeSetting;

typedef struct {
    TU8 record_strategy;
    TU8 record_autocut_condition;
    TU8 record_autocut_naming;
    TU8 reserved0;
    TU32 record_autocut_framecount;
    TU32 record_autocut_duration;
} SRecordAutoCutSetting;

typedef struct {
    TU8 pre_send_audio_extradata;
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;
} SDebugMediaConfig;

typedef struct {
    TChar audio_device_name[32];
} SAudioDeviceConfig;

class ITCPPulseSender;
class IStorageManagement;
class IGenericEngineControlV3;
class IGenericEngineControlV4;

typedef struct {
    //general config
    SPersistCommonConfig common_config;

    SDebugEngineConfig engine_config;

    SDebugMediaConfig debug_config;

    TU8 streaming_enable, compensate_from_jitter, identifyer_count, app_start_exit;

    //rtsp streaming related
    TU8 rtsp_streaming_enable, rtsp_streaming_video_enable, rtsp_streaming_audio_enable, try_requested_video_framerate;
    SRTSPServerConfig rtsp_server_config;

    //rtsp client related
    SRTSPClientConfig rtsp_client_config;

    //cloud server related
    SSACPServerConfig sacp_cloud_server_config;

    //dsp related
    SDSPConfig dsp_config;

    //audio device related
    SAudioDeviceConfig audio_device;

    //network related option
    SNetworkTuningConfig network_config;

    SEncodingModeConfig encoding_mode_config;

    SVideoCaptureConfig video_capture_config;
    SVideoEncodingConfig video_encoding_config;

    SAudioEncodingConfig audio_encoding_config;

    SAudioPreferSetting audio_prefer_setting;

    SPlaybackCacheSetting pb_cache;

    SPlaybackDecodeSetting pb_decode;
    SEncodeSetting encode;
    TU32 requested_video_framerate_num;
    TU32 requested_video_framerate_den;

    //recorder config
    TU32 tot_muxer_number;
    STargetRecorderConfig target_recorder_config[EComponentMaxNumber_Muxer];
    TU8 cutfile_with_precise_pts;
    TU8 muxer_skip_video;
    TU8 muxer_skip_audio;
    TU8 muxer_skip_pridata;

    SRecordAutoCutSetting auto_cut;

    //internal use only:
    //scheduler related
    IScheduler *p_scheduler_network_reciever[DMaxSchedulerGroupNumber];
    IScheduler *p_scheduler_network_tcp_reciever[DMaxSchedulerGroupNumber];
    IScheduler *p_scheduler_network_sender[DMaxSchedulerGroupNumber];
    IScheduler *p_scheduler_io_writer[DMaxSchedulerGroupNumber];
    IScheduler *p_scheduler_io_reader[DMaxSchedulerGroupNumber];

    IScheduler *p_scheduler_video_decoder[DMaxSchedulerGroupNumber];

    ITCPPulseSender *p_tcp_pulse_sender[DMaxTCPPulseSenderNumber];

    IClockSource *p_clock_source;
    CIClockManager *p_system_clock_manager;
    CIClockReference *p_system_clock_reference;

    CIRemoteLogServer *p_remote_log_server;
    TU32 runtime_print_mask;

    TU8 number_scheduler_network_reciever;
    TU8 number_scheduler_network_tcp_reciever;
    TU8 number_scheduler_network_sender;
    TU8 number_scheduler_io_writer;

    TU8 number_scheduler_io_reader;
    TU8 number_scheduler_video_decoder;
    TU8 number_tcp_pulse_sender;

    TU8 tmp_number_of_d1;
    TU8 playback_prefetch_number;
    TU8 debug_print_color;// 0: normal. 1: waning

    TU8 enable_rtsp_authentication;
    TU8 rtsp_authentication_mode;// 0: basic, 1: digest

    TTime streaming_timeout_threashold;
    TTime streaming_autoconnect_interval;

    SPreferModuleSetting prefer_module_setting;

    IStorageManagement *p_storage_manager;

    IGenericEngineControlV4 *p_engine_v4;
    IGenericEngineControlV3 *p_engine_v3;

    //remote related
    TChar remote_bridge[DMaxConfStringLength];
    IMutex *p_global_mutex;
} SPersistMediaConfig;

typedef struct {
    TU8 use_preset_mode;
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;
} SFilterWorkingMode;

typedef struct {
    TU32 check_field;
    TGenericID component_id;

    TTime target;//ms
    ENavigationPosition position;
} SPbSeek;

typedef struct {
    TU32 check_field;
    TGenericID component_id;

    TU8 direction;
    TU8 feeding_rule;
    TU16 speed;
} SPbFeedingRule;

typedef struct {
    TU32 check_field;
    TGenericID component_id;
} SResume1xPlayback;

//for vod demuxer
typedef struct {
    TChar *pChannelName;

    TU8 rtp_over_rtsp;
    TU8 is_vod_mode;
    TU8 vod_has_end;
    TU8 speed_combo;

    SDateTime *starttime;
    SDateTime *targettime;
    SDateTime *endtime;

    TTime current_time;
} SContextInfo;

typedef struct {
    TU8 *pVideoExtraData;
    TU8 *pAudioExtraData;
    TU32 videoSize;
    TU32 audioSize;
    StreamFormat videoFormat;
    StreamFormat audioFormat;
} SExtraData;

typedef struct {
    MuxerSavingFileStrategy strategy;
    MuxerSavingCondition condition;
    MuxerAutoFileNaming naming;
    TUint param;
} SRecSavingStrategy;

typedef struct {
    TU8 up_pin_index;
    TU8 up_pin_sub_index;
    TU8 down_pin_index;
    TU8 reserved0;
} SEnableConnection;

typedef struct {
    TComponentIndex index;
    TU16 reserved;
    TPointer p_client_pointer;
    StreamType type;
} SSetContent;

//-----------------------------------------------------------------------
//
// Display layout related
//
//-----------------------------------------------------------------------
enum {
    EDisplayLayout_Invalid = 0x0,

    EDisplayLayout_Rectangle,
    EDisplayLayout_TelePresence,
    EDisplayLayout_BottomLeftHighLighten,
    EDisplayLayout_SingleWindowView,

    EDisplayLayout_Customized = 0x10,
};

enum {
    EDsplaySubWindowLayout_Horizontal_to_Right = 0,
    EDsplaySubWindowLayout_Vertical_to_Down,
    EDsplaySubWindowLayout_Horizontal_to_Right_Vertical_to_Up,
};

typedef struct {
    //check field
    TU32 check_field;

    TU8 capture_id;
    TU8 capture_coded;
    TU8 capture_screennail;
    TU8 capture_thumbnail;

    TU8 jpeg_quality;
    TU8 capture_file_index;
    TU8 customized_file_name;
    TU8 source_id;

    TU16 coded_width;
    TU16 coded_height;

    TU16 screennail_width;
    TU16 screennail_height;

    TU16 thumbnail_width;
    TU16 thumbnail_height;

    //TChar coded_file_name[128];
    //TChar sn_file_name[128];
    //TChar tn_file_name[128];
} SPlaybackCapture;

typedef struct {
    //check field
    TU32 check_field;

    TU8 zoom_mode;//0: mode2, input 5 paras as render_id, input width, height, center x, center y; 1: mode1, input 3 paras as render_id, zoom factor x, zoom factor y.
    TU8 render_id;

    //for mode2
    TU16 input_center_x;
    TU16 input_center_y;
    TU16 input_width;
    TU16 input_height;

    //for mode1
    TU32 zoom_factor_x;
    TU32 zoom_factor_y;
} SPlaybackZoom;

typedef struct {
    //check field
    TU32 check_field;

    TU8 render_id;
    TU8 decoder_id, reserved1, reserved2;

    //paras based on display windows
    TU16 input_center_x;
    TU16 input_center_y;
    TU16 input_width;
    TU16 input_height;
    TU16 window_width;
    TU16 window_height;
} SPlaybackZoomOnDisplay;

typedef struct {
    //global
    TU8 total_num_win_configs;
    TU8 audio_on_win_id;
    TU8 pre_buffer_len;
    TU8 enable_buffering_ctrl;
    TU8 av_sync_enabled;
    TU8 voutB_enabled;
    TU8 voutA_enabled;
    TU8 input_param_update_window_index;//tmp
    TU16 display_width;
    TU16 display_height;

    TU8 layer_window_number[DMaxDisplayLayerNumber];
    TU8 layer_number;
    TU8 current_layer;

    //render,window,udec
    TU8 total_window_number;
    TU8 total_render_number;
    TU8 total_decoder_number;
    TU8 cur_window_start_index;
    TU8 cur_render_start_index;
    TU8 cur_decoder_start_index;
    TU8 cur_window_number;
    TU8 cur_render_number;
    TU8 cur_decoder_number;

    TU8 set_config_direct_to_dsp;
    TU8 focus_window_id;
    TU8 single_view_mode;
    TU8 single_view_decoder_id;

    SDSPMUdecWindow window[DMaxPostPWindowNumber];
    SDSPMUdecRender render[DMaxPostPRenderNumber];
    SDSPUdecInstanceConfig decoder[DMaxUDECInstanceNumber];

    SDSPMUdecWindow single_view_window;
    SDSPMUdecRender single_view_render;
} SVideoPostPConfiguration;

typedef struct {
    //check field
    TU32 check_field;

    TU8 render_id;
    TU8 new_dec_id;

    TU8 seamless;
    TU8 wait_switch_done;
} SVideoPostPStreamSwitch;

typedef struct {
    //check field
    TU32 check_field;

    TU8 new_display_vout_mask;
    TU8 vout_mask;

    TU8 primary_vout_index;
    TU8 reserved0;
} SVideoPostPDisplayMask;

typedef struct {
    //check field
    TU32 check_field;

    TU8 layer_number;
    TU8 is_window0_HD;
    TU8 reserved1;

    TU8 total_window_number;
    TU8 total_render_number;
    TU8 total_decoder_number;

    TU8 cur_window_number;
    TU8 cur_render_number;
    TU8 cur_decoder_number;

    TU8 cur_window_start_index;
    TU8 cur_render_start_index;
    TU8 cur_decoder_start_index;

    TU8 layer_layout_type[DMaxDisplayLayerNumber];
    TU8 layer_window_number[DMaxDisplayLayerNumber];
} SVideoPostPDisplayLayout;

typedef struct {
    //check field
    TU32 check_field;

    TU16  window_width;
    TU16  window_height;
} SVideoPostPDisplayHighLightenWindowSize;

typedef struct {
    //check field
    TU32 check_field;

    TU8 display_mask;
    TU8 primary_display_index;
    TU8 cur_display_mask;
    TU8 reserved0;

    TU16 display_width;
    TU16 display_height;
} SVideoPostPGlobalSetting;

typedef struct {
    //check field
    TU32 check_field;

    TU8 request_display_layer;
    TU8 sd_window_index_for_hd, flush_udec, reserved2;
} SVideoPostPDisplayLayer;

typedef struct {
    //check field
    TU32 check_field;

    TGenericID pb_pipeline;
    TU32 loop_mode;//0-no loop, 1-single source loop, 2-all sources loop
} SPipeLineLoopMode;

typedef struct {
    //check field
    TU32 check_field;

    TU32 target_source_group_index;
} SPipeLineSwitchSourceGroup;
typedef struct {
    TU32 check_field;
    TU8 swap_win0_id;
    TU8 swap_win1_id;
    TU8 reserved0, reserved1;
} SVideoPostPSwap;

typedef struct {
    //check field
    TU32 check_field;

    TU8 shift_count;
    TU8 shift_direction;
    TU8 reserved0, reserved1;
} SVideoPostPShift;

typedef struct {
    //check field
    TU32 check_field;

    TU8 request_display_focus;
    TU8 reserved0, reserved1, reserved2;
} SVideoPostPDisplayFocus;

typedef struct {
    //check field
    TU32 check_field;

    TGenericID single_view_pipeline_id;
    TU8 single_view_window_id;
    TU8 single_view_from_window_id;
    TU8 enable_single_view_mode;
    TU8 reserved0;
} SVideoPostPDisplaySingleViewMode;

typedef struct {
    //check field
    TU32 check_field;

    TU8 id;
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;
} SVideoPostPDisplayTrickPlay;

//update hd udec instance's source
typedef struct {
    //check field
    TU32 check_field;

    TU8 halt_UDEC;

    TU8 win_id;

    TU8 dec_id;//debug check
    TU8 stop_flags;

    TGenericID pipeline_id;
} SUpdateUDECSource;

typedef struct {
    //check field
    TU32 check_field;

    TUint window_id;
    TUint window_id_2rd;
    TUint render_id;
    TUint decoder_id;
} SVideoPostPDisplayMWMapping;

typedef struct {
    //check field
    TU32 check_field;

    TU8 dec_id;
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;

    TUint pic_width;
    TUint pic_height;
} SVideoCodecInfo;

typedef struct {
    //check field
    TU32 check_field;

    TUint vout_mask;
    TUint primary_vout_index;

    TUint primary_vout_width;
    TUint primary_vout_height;
} SVideoDisplayDeviceInfo;

typedef struct {
    //check field
    TU32 check_field;

    TU8 all_demuxer;
    TU8 reserved0;

    TU16 index;
} SGenericReconnectStreamingServer;

typedef struct {
    //check field
    TU32 check_field;

    TUint number;
} SGenericPreConfigure;

typedef struct {
    //check field
    TU32 check_field;

    TUint channel;
} SGenericSelectAudioChannel;

typedef struct {
    //check field
    TU32 check_field;
    TU32 set_or_get;//0-get, 1-set
    SVideoEncoderParam video_params;
} SGenericVideoEncoderParam;

typedef struct {
    //check field
    TU32 check_field;
    TU32 set_or_get;//0-get, 1-set
    SAudioParams audio_params;
} SGenericAudioParam;

typedef struct {
    //check field
    TU32 check_field;
    TU32 set_or_get;//0-get, 1-set
    TGenericID rec_pipeline_id;
    SRecSavingStrategy rec_saving_strategy;
} SGenericRecordSavingStrategy;

typedef struct {
    //check field
    TU32 check_field;

    TU8 sd_window_index_for_hd, flush_udec, tobeHD, reserved1;
} SVideoPostPDisplayHD;

typedef struct {
    //check field
    TU32 check_field;
    TU32 set_or_get;//0-get, 1-set
    SVideoPostPConfiguration *p_video_postp_info;
} SGenericVideoPostPConfiguration;

typedef struct {
    //check field
    TU32 check_field;

    TU8 layer_index, layout_type, reserved2, reserved3;
} SQueryVideoDisplayLayout;

typedef struct {
    //check field
    TU32 check_field;

    TGenericID id;
    TPointer handler;
} SQueryGetHandler;

typedef struct {
    TUint codec_id;
    TUint payload_type;

    TU8 stream_index;
    TU8 stream_presence;
    TU8 stream_enabled;
    TU8 inited;

    StreamType stream_type;
    StreamFormat stream_format;
    UFormatSpecific spec;
} SStreamCodecInfo;

typedef struct {
    TU8 speed;
    TU8 speed_frac;

    TU8 direction;
    TU8 reserved0;

    TTime navigation_begin_time;
} SFlowControlSetting;

typedef struct {
    TU8 *p_data;
    TU32 data_size;
} SRelayData;

typedef struct {
    TU32 source_type;
    void *context;
} SUpdateVisualSourceContext;

typedef struct {
    TU32 source_type;
    void *context;
} SUpdateAudioSourceContext;

typedef struct {
    TU32 sink_type;
    void *context;
} SUpdateVisualSinkContext;

typedef struct {
    TU32 sink_type;
    void *context;
} SUpdateAudioSinkContext;

typedef struct {
    TU32 check_field;
    TGenericID component_id;
} SUserParamStepPlay;

typedef struct {
    TU32 check_field;
    TGenericID component_id;
} SUserParamPause;

typedef struct {
    TU32 check_field;
    TGenericID component_id;
} SUserParamResume;

typedef struct {
    TU32 check_field;
    TGenericID component_id;
} SUserParamRecordSuspend;

typedef struct {
    TU32 check_field;
    TGenericID id;
    void *callback;
    void *callback_context;
} SExternalDataProcessingCallback;

typedef struct {
    TU32 check_field;

    TU8 b_digital_vout;
    TU8 b_hdmi_vout;
    TU8 b_cvbs_vout;
    TU8 reserved0;
} SConfigVout;

typedef struct {
    TU32 check_field;
    TChar audio_device_name[32];
} SConfigAudioDevice;

typedef enum {

    //video post-processor, display
    EGenericEngineConfigure_VideoDisplay_UpdateDisplayDeviceMask = 0x0000,
    EGenericEngineConfigure_VideoDisplay_StreamSwitch,
    EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayout,
    EGenericEngineConfigure_VideoDisplay_UpdateDisplayLayer,
    EGenericEngineConfigure_VideoDisplay_SwapWindowContent,
    EGenericEngineConfigure_VideoDisplay_ShiftWindowContent,
    EGenericEngineConfigure_VideoDisplay_UpdateUDECSource,
    EGenericEngineConfigure_VideoDisplay_SingleViewMode,
    EGenericEngineConfigure_VideoDisplay_UpdateHighLightenWindowSize,
    EGenericEngineConfigure_VideoDisplay_SwitchWindowToHD,
    EGenericEngineConfigure_VideoDisplay_PostPConfiguration,
    EGenericEngineConfigure_VideoDisplay_UpdateOneWindow,

    //pre running, persist config
    EGenericEngineConfigure_RTSPServerPort = 0x0010,
    EGenericEngineConfigure_RTSPServerNonblock = 0x0011,
    EGenericEngineConfigure_VideoStreamID = 0x0012,
    EGenericEngineConfigure_ConfigVout = 0x0013,
    EGenericEngineConfigure_ConfigAudioDevice = 0x0014,

    EGenericEngineConfigure_FastForward = 0x0020,
    EGenericEngineConfigure_FastBackward = 0x0021,
    EGenericEngineConfigure_StepPlay = 0x0022,
    EGenericEngineConfigure_Pause = 0x0023,
    EGenericEngineConfigure_Resume = 0x0024,
    EGenericEngineConfigure_Seek = 0x0025,
    EGenericEngineConfigure_LoopMode = 0x0026,
    EGenericEngineConfigure_FastForwardFromBegin = 0x0027,
    EGenericEngineConfigure_FastBackwardFromEnd = 0x0028,
    EGenericEngineConfigure_Resume1xFromBegin = 0x0029,
    EGenericEngineConfigure_Resume1xFromCurrent = 0x002a,

    EGenericEngineConfigure_VideoPlayback_Capture = 0x0070,
    EGenericEngineConfigure_VideoPlayback_Zoom,
    EGenericEngineConfigure_VideoPlayback_Zoom_OnDisplay,

    //for each window, blow to be deprecated
    EGenericEngineConfigure_VideoPlayback_WindowPause = 0x0080,
    EGenericEngineConfigure_VideoPlayback_WindowResume,
    EGenericEngineConfigure_VideoPlayback_WindowStepPlay,

    //for each dsp instance
    EGenericEngineConfigure_VideoPlayback_UDECPause = 0x0090,
    EGenericEngineConfigure_VideoPlayback_UDECResume,
    EGenericEngineConfigure_VideoPlayback_UDECStepPlay,

    //external data processing callback
    EGenericEngineConfigure_SetVideoExternalDataPreProcessingCallback = 0x00c0,
    EGenericEngineConfigure_SetVideoExternalDataPostProcessingCallback = 0x00c1,

    //video playback, for pipeline
    EGenericEngineConfigure_VideoPlayback_FastForward = 0x0100,
    EGenericEngineConfigure_VideoPlayback_FastBackward,
    EGenericEngineConfigure_VideoPlayback_StepPlay,
    EGenericEngineConfigure_VideoPlayback_Pause,
    EGenericEngineConfigure_VideoPlayback_Resume,
    EGenericEngineConfigure_VideoPlayback_LoopMode,
    EGenericEngineConfigure_VideoPlayback_CompensateDelayFromJitter,
    EGenericEngineConfigure_VideoPlayback_SwitchSourceGroup,

    //audio playback
    EGenericEngineConfigure_AudioPlayback_SelectAudioSource = 0x0200,
    EGenericEngineConfigure_AudioPlayback_Mute,
    EGenericEngineConfigure_AudioPlayback_UnMute,
    EGenericEngineConfigure_AudioPlayback_SetVolume,

    //audio capture
    EGenericEngineConfigure_AudioCapture_Mute = 0x0210,
    EGenericEngineConfigure_AudioCapture_UnMute = 0x0211,

    //streaming related
    EGenericEngineConfigure_ReConnectServer = 0x0220,

    //dsp related
    EGenericEngineConfigure_DSP_Suspend = 0x0300, //to idle
    EGenericEngineConfigure_DSP_Resume,

    //video encoder
    EGenericEngineConfigure_VideoEncoder_Params = 0x0400,
    EGenericEngineConfigure_VideoEncoder_Bitrate,
    EGenericEngineConfigure_VideoEncoder_Framerate,
    EGenericEngineConfigure_VideoEncoder_DemandIDR,
    EGenericEngineConfigure_VideoEncoder_GOP,

    //record related, for pipeline
    EGenericEngineConfigure_Record_Saving_Strategy = 0x0450,
    EGenericEngineConfigure_Record_SetSpecifiedInformation = 0x0451,
    EGenericEngineConfigure_Record_GetSpecifiedInformation = 0x0452,
    EGenericEngineConfigure_Record_Suspend = 0x0453,
    EGenericEngineConfigure_Record_ResumeSuspend = 0x0454,

    //device /platform related
    EGenericEngineConfigure_DevicePlatform_UpdateVideoSourceContext = 0x0800,
    EGenericEngineConfigure_DevicePlatform_UpdateAudioSourceContext = 0x0801,
    EGenericEngineConfigure_DevicePlatform_UpdateVideoSinkContext = 0x0802,
    EGenericEngineConfigure_DevicePlatform_UpdateAudioSinkContext = 0x0803,

    //query api
    EGenericEngineQuery_VideoDisplayWindow = 0x8000,
    EGenericEngineQuery_VideoDisplayRender,
    EGenericEngineQuery_VideoDisplayLayout,
    EGenericEngineQuery_VideoDecoder,

    EGenericEngineQuery_VideoCodecInfo,
    EGenericEngineQuery_AudioCodecInfo,

    EGenericEngineQuery_VideoDisplayDeviceInfo,

    EGenericEngineQuery_VideoDSPStatus,

    EGenericEngineQuery_GetHandler = 0x8100,
} EGenericEngineConfigure;

typedef enum {
    //
    EGenericEnginePreConfigure_AutoDisableDecodingBackGround = 0x0000,

    //scheduler
    EGenericEnginePreConfigure_NetworkReceiverSchedulerNumber = 0x0010,
    EGenericEnginePreConfigure_NetworkTransmiterSchedulerNumber = 0x0011,
    EGenericEnginePreConfigure_IOWriterSchedulerNumber = 0x0012,
    EGenericEnginePreConfigure_IOReaderSchedulerNumber = 0x0013,
    EGenericEnginePreConfigure_VideoDecoderSchedulerNumber = 0x0014,
    EGenericEnginePreConfigure_NetworkTCPReceiverSchedulerNumber = 0x0015,

    //playback related
    EGenericEnginePreConfigure_PlaybackSetDSPDecoderBufferNumber = 0x0020,
    EGenericEnginePreConfigure_PlaybackSetDSPPrebuffer = 0x0021,
    EGenericEnginePreConfigure_PlaybackSetPreFetch = 0x0022,

    //network related
    EGenericEnginePreConfigure_SetStreamingTimeoutThreashold = 0x0030,
    EGenericEnginePreConfigure_StreamingRetryConnectServerInterval = 0x0031,

    //streaming server
    EGenericEnginePreConfigure_RTSPStreamingServerPort = 0x0040,
    EGenericEnginePreConfigure_RTSPStreamingServerSetAuthentication = 0x0041,
    EGenericEnginePreConfigure_RTSPStreamingServerAuthenticationMode = 0x0042,

    //streaming client
    EGenericEnginePreConfigure_RTSPClientTryTCPModeFirst = 0x0050,
    EGenericEnginePreConfigure_RTSPClientParseMultipleAccessUnit = 0x0051, //aac only now

    //cloud server
    EGenericEnginePreConfigure_SACPCloudServerPort = 0x0060,

    //dsp related setting
    EGenericEnginePreConfigure_DSPAddUDECWrapper = 0x0070,
    EGenericEnginePreConfigure_DSPFeedPTS = 0x0071,
    EGenericEnginePreConfigure_DSPSpecifyFPS = 0x0072,

    //specify prefer module
    EGenericEnginePreConfigure_PreferModule_Muxer_FFMpeg = 0x0080,
    EGenericEnginePreConfigure_PreferModule_Muxer_TS = 0x0081,
    EGenericEnginePreConfigure_PreferModule_AudioDecoder_FFMpeg = 0x0082,
    EGenericEnginePreConfigure_PreferModule_AudioDecoder_AAC = 0x0083,
    EGenericEnginePreConfigure_PreferModule_AudioDecoder_Customized_ADPCM = 0x0084,
    EGenericEnginePreConfigure_PreferModule_AudioEncoder_FFMpeg = 0x0085,
    EGenericEnginePreConfigure_PreferModule_AudioEncoder_AAC = 0x0086,
    EGenericEnginePreConfigure_PreferModule_AudioEncoder_Customized_ADPCM = 0x0087,
    EGenericEnginePreConfigure_PreferModule_AudioRenderer_ALSA = 0x0088,
    EGenericEnginePreConfigure_PreferModule_AudioRenderer_PulseAudio = 0x0089,

    EGenericEnginePreConfigure_PreferModule_VideoDecoder_FFMpeg = 0x008a,
    EGenericEnginePreConfigure_PreferModule_VideoEncoder_FFMpeg = 0x008b,
    EGenericEnginePreConfigure_PreferModule_VideoDecoder_X264 = 0x008c,
    EGenericEnginePreConfigure_PreferModule_VideoEncoder_X264 = 0x008d,

    //audio capturer params
    EGenericEnginePreConfigure_AudioCapturer_Params = 0x00a0,
} EGenericEnginePreConfigure;

#define DFLAG_STATUS_EOS 0

//-----------------------------------------------------------------------
//
// IGenericMediaControl
//
//-----------------------------------------------------------------------

typedef void (*TFTimerCallback)(void *thiz, void *param, TTime current_time);

class IGenericEngineControl
{
public:
    virtual EECode SetWorkingMode(EMediaWorkingMode preferedWorkingMode, EPlaybackMode playback_mode) = 0;
    virtual EECode SetDisplayDevice(TUint deviceMask) = 0;
    virtual EECode SetInputDevice(TUint deviceMask) = 0;

    //log/debug related api
public:
    virtual EECode SetEngineLogLevel(ELogLevel level) = 0;
    virtual EECode PrintCurrentStatus(TGenericID id = 0, TULong print_flag = 0) = 0;
    virtual EECode SaveCurrentLogSetting(const TChar *logfile = NULL) = 0;

public:
    //build graph related
    virtual EECode BeginConfigProcessPipeline(TUint customized_pipeline) = 0;
    virtual EECode NewComponent(TUint module_type, TGenericID &module_id) = 0;
    virtual EECode ConnectComponent(TGenericID &connection_id, TGenericID upper_component_id, TGenericID down_component_id, StreamType pin_type) = 0;
    virtual EECode SetupStreamingContent(TGenericID content_id, TGenericID server_id, TGenericID transmiter_id, TGenericID video_source_component_id, TGenericID audio_source_component_id, TChar *streaming_tag) = 0;
    virtual EECode SetupUploadingReceiverContent(TGenericID content_id, TGenericID server_id, TGenericID receiver_id, TChar *receiver_tag) = 0;
    virtual EECode FinalizeConfigProcessPipeline() = 0;

    //query API, p_pipeline for debug
    virtual EECode QueryPipelineID(TU8 pipeline_type, TGenericID source_sink_id, TGenericID &pipeline_id, TU8 index = 0) = 0;
    virtual EECode QueryPipelineID(TU8 pipeline_type, TU8 source_sink_index, TGenericID &pipeline_id, TU8 index = 0) = 0;

    //set source for source filters
    virtual EECode SetDataSource(TGenericID module_id, TChar *url) = 0;
    //set dst for sink filters
    virtual EECode SetSinkDst(TGenericID module_id, TChar *file_dst) = 0;
    //set url for streaming
    virtual EECode SetStreamingUrl(TGenericID content_id, TChar *streaming_tag) = 0;
    //set tag for cloud client receiver
    virtual EECode SetCloudClientRecieverTag(TGenericID receiver_id, TChar *receiver_tag) = 0;

    //flow related
    virtual EECode StartProcess() = 0;
    virtual EECode StopProcess() = 0;
    virtual EECode SetAppMsgCallback(void (*MsgProc)(void *, SMSG &), void *context) = 0;

    //pb related
    virtual EECode PBSpeed(TGenericID pb_pipeline, TU16 speed, TU16 speed_frac, TU8 backward = 0, DecoderFeedingRule feeding_rule = DecoderFeedingRule_AllFrames) = 0;

    virtual EECode GenericControl(EGenericEngineConfigure config_type, void *param) = 0;

    //rec related
    virtual EECode RECStop(TGenericID rec_pipeline) = 0;
    virtual EECode RECStart(TGenericID rec_pipeline) = 0;

    virtual EECode RECMuxerFinalizeFile(TGenericID rec_pipeline) = 0;
    virtual EECode RECSavingStrategy(TGenericID rec_pipeline, MuxerSavingFileStrategy strategy, MuxerSavingCondition condition = MuxerSavingCondition_InputPTS, MuxerAutoFileNaming naming = MuxerAutoFileNaming_ByNumber, TUint param = DefaultAutoSavingParam) = 0;

    //pipeline related
    virtual EECode SuspendPipeline(TGenericID pb_pipeline) = 0;
    virtual EECode ResumePipeline(TGenericID pb_pipeline) = 0;

    //timer related
    virtual TTime UserGetCurrentTime() const = 0;
    virtual TGenericID AddUserTimer(TFTimerCallback cb, void *thiz, void *param, EClockTimerType type, TTime time, TTime interval, TUint count) = 0;
    virtual EECode RemoveUserTimer(TGenericID user_timer_id) = 0;

    //configuration
public:
    virtual EECode GenericPreConfigure(EGenericEnginePreConfigure config_type, void *param, TUint is_get = 0) = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~IGenericEngineControl() {}
};

class IGenericEngineControlV2
{
public:
    virtual EECode InitializeDevice() = 0;
    virtual EECode ActivateDevice(TUint mode) = 0;
    virtual EECode DeActivateDevice() = 0;
    virtual EECode GetMediaConfig(volatile SPersistMediaConfig *&pconfig) const = 0;
    virtual EECode SetMediaConfig() = 0;

public:
    //log/debug related api
    virtual EECode SetEngineLogLevel(ELogLevel level) = 0;
    virtual EECode SetRuntimeLogConfig(TGenericID id, TU32 level, TU32 option, TU32 output, TU32 is_add = 0) = 0;
    virtual EECode PrintCurrentStatus(TGenericID id = 0, TULong print_flag = 0) = 0;
    virtual EECode SaveCurrentLogSetting(const TChar *logfile = NULL) = 0;

public:
    //build graph related
    virtual EECode BeginConfigProcessPipeline(TUint customized_pipeline = 1) = 0;

    virtual EECode NewComponent(TUint component_type, TGenericID &component_id) = 0;
    virtual EECode ConnectComponent(TGenericID &connection_id, TGenericID upper_component_id, TGenericID down_component_id, StreamType pin_type) = 0;

    virtual EECode SetupPlaybackPipeline(TGenericID &playback_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID video_decoder_id, TGenericID audio_decoder_id, TGenericID video_renderer_id, TGenericID audio_renderer_id, TGenericID audio_pinmuxer_id = 0, TU8 running_at_startup = 1) = 0;
    virtual EECode SetupRecordingPipeline(TGenericID &recording_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID sink_id, TU8 running_at_startup = 1) = 0;
    virtual EECode SetupStreamingPipeline(TGenericID &streaming_pipeline_id, TGenericID streaming_transmiter_id, TGenericID streaming_server_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID audio_pinmuxer_id = 0, TU8 running_at_startup = 1) = 0;
    virtual EECode SetupCloudPipeline(TGenericID &cloud_pipeline_id, TGenericID cloud_agent_id, TGenericID cloud_server_id, TGenericID video_sink_id, TGenericID audio_sink_id, TGenericID cmd_sink_id = 0, TU8 running_at_startup = 1) = 0;

    virtual EECode SetupNativeCapturePipeline(TGenericID &capture_pipeline_id, TGenericID video_capture_source_id, TGenericID audio_capture_source_id, TGenericID video_encoder_id, TGenericID audio_encoder_id, TU8 running_at_startup = 1) = 0;
    virtual EECode SetupNativePushPipeline(TGenericID &push_pipeline_id, TGenericID push_source_id, TGenericID video_sink_id, TGenericID audio_sink_id, TU8 running_at_startup = 1) = 0;

    virtual EECode SetupCloudUploadPipeline(TGenericID &upload_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID uploader_id, TU8 running_at_startup) = 0;
    virtual EECode FinalizeConfigProcessPipeline() = 0;

public:
    //set source for source filters
    virtual EECode SetSourceUrl(TGenericID source_component_id, TChar *url) = 0;
    //set dst for sink filters
    virtual EECode SetSinkUrl(TGenericID sink_component_id, TChar *url) = 0;
    //set url for streaming
    virtual EECode SetStreamingUrl(TGenericID pipeline_id, TChar *url) = 0;
    //set tag for cloud client's receiver at server side
    virtual EECode SetCloudAgentTag(TGenericID agent_id, TChar *agent_tag, TGenericID server_id = 0) = 0;
    //set tag for cloud client's tag and remote server addr and port
    virtual EECode SetCloudClientTag(TGenericID agent_id, TChar *agent_tag, TChar *p_server_addr, TU16 server_port) = 0;

    //couple related
    virtual EECode CoupleCmdData(TGenericID data_agent_id, TGenericID cmd_agent_id) = 0;

    //flow related
    virtual EECode StartProcess() = 0;
    virtual EECode StopProcess() = 0;
    virtual EECode SetAppMsgCallback(void (*MsgProc)(void *, SMSG &), void *context) = 0;

    //pipeline related
    virtual EECode SuspendPipeline(TGenericID pipeline_id, TUint release_content) = 0;
    virtual EECode ResumePipeline(TGenericID pipeline_id, TU8 force_audio_path = 0, TU8 force_video_path = 0) = 0;

public:
    //configuration
    virtual EECode GenericPreConfigure(EGenericEnginePreConfigure config_type, void *param, TUint is_get = 0) = 0;
    virtual EECode GenericControl(EGenericEngineConfigure config_type, void *param) = 0;

public:
    virtual const TChar *GetLastErrorCodeString() const = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~IGenericEngineControlV2() {}
};

class IGenericEngineControlV3
{
public:
    virtual EECode InitializeDevice() = 0;
    virtual EECode ActivateDevice(TU32 mode) = 0;
    virtual EECode DeActivateDevice() = 0;
    virtual EECode GetMediaConfig(volatile SPersistMediaConfig *&pconfig) const = 0;
    virtual EECode SetMediaConfig() = 0;

public:
    //log/debug related api
    virtual EECode SetEngineLogLevel(ELogLevel level) = 0;
    virtual EECode SetRuntimeLogConfig(TGenericID id, TU32 level, TU32 option, TU32 output, TU32 is_add = 0) = 0;
    virtual EECode PrintCurrentStatus(TGenericID id = 0, TULong print_flag = 0) = 0;
    virtual EECode SaveCurrentLogSetting(const TChar *logfile = NULL) = 0;

public:
    //build graph related
    virtual EECode BeginConfigProcessPipeline(TU32 customized_pipeline = 1) = 0;

    virtual EECode NewComponent(TU32 component_type, TGenericID &component_id, const TChar *prefer_string = NULL) = 0;
    virtual EECode ConnectComponent(TGenericID &connection_id, TGenericID upper_component_id, TGenericID down_component_id, StreamType pin_type) = 0;

    virtual EECode SetupPlaybackPipeline(TGenericID &playback_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID video_decoder_id, TGenericID audio_decoder_id, TGenericID video_renderer_id, TGenericID audio_renderer_id, TGenericID audio_pinmuxer_id = 0, TU8 running_at_startup = 1) = 0;
    virtual EECode SetupRecordingPipeline(TGenericID &recording_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID sink_id, TU8 running_at_startup = 1, TChar *p_channelname = NULL) = 0;
    virtual EECode SetupStreamingPipeline(TGenericID &streaming_pipeline_id, TGenericID streaming_transmiter_id, TGenericID streaming_server_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID audio_pinmuxer_id = 0, TU8 running_at_startup = 1) = 0;
    virtual EECode SetupCloudPipeline(TGenericID &cloud_pipeline_id, TGenericID cloud_agent_id, TGenericID cloud_server_id, TGenericID video_sink_id, TGenericID audio_sink_id, TGenericID cmd_sink_id = 0, TU8 running_at_startup = 1) = 0;

    virtual EECode SetupNativeCapturePipeline(TGenericID &capture_pipeline_id, TGenericID video_capture_source_id, TGenericID audio_capture_source_id, TGenericID video_encoder_id, TGenericID audio_encoder_id, TU8 running_at_startup = 1) = 0;
    virtual EECode SetupNativePushPipeline(TGenericID &push_pipeline_id, TGenericID push_source_id, TGenericID video_sink_id, TGenericID audio_sink_id, TU8 running_at_startup = 1) = 0;

    virtual EECode SetupCloudUploadPipeline(TGenericID &upload_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID uploader_id, TU8 running_at_startup) = 0;
    virtual EECode SetupVODPipeline(TGenericID &vod_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID flow_controller_id, TGenericID streaming_transmiter_id, TGenericID streaming_server_id, TU8 running_at_startup = 0) = 0;
    virtual EECode FinalizeConfigProcessPipeline() = 0;

public:
    //set source for source filters
    virtual EECode SetSourceUrl(TGenericID source_component_id, TChar *url) = 0;
    //set dst for sink filters
    virtual EECode SetSinkUrl(TGenericID sink_component_id, TChar *url) = 0;
    //set url for streaming
    virtual EECode SetStreamingUrl(TGenericID pipeline_id, TChar *url) = 0;
    //set url for vod
    virtual EECode SetVodUrl(TGenericID source_component_id, TGenericID pipeline_id, TChar *url) = 0;
    //set tag for cloud client's receiver at server side
    virtual EECode SetCloudAgentTag(TGenericID agent_id, TChar *agent_tag, TGenericID server_id = 0) = 0;
    //set tag for cloud client's tag and remote server addr and port
    virtual EECode SetCloudClientTag(TGenericID agent_id, TChar *agent_tag, TChar *p_server_addr, TU16 server_port) = 0;

    //vod content
    virtual EECode SetupVodContent(TUint channel, TChar *url, TU8 localfile, TU8 enable_video, TU8 enable_audio) = 0;

    //flow related
    virtual EECode StartProcess() = 0;
    virtual EECode StopProcess() = 0;
    virtual EECode SetAppMsgCallback(void (*MsgProc)(void *, SMSG &), void *context) = 0;

    //pipeline related
    virtual EECode SuspendPipeline(TGenericID pipeline_id, TUint release_content) = 0;
    virtual EECode ResumePipeline(TGenericID pipeline_id, TU8 force_audio_path = 0, TU8 force_video_path = 0) = 0;

public:
    //configuration
    virtual EECode GenericPreConfigure(EGenericEnginePreConfigure config_type, void *param, TUint is_get = 0) = 0;
    virtual EECode GenericControl(EGenericEngineConfigure config_type, void *param) = 0;

    virtual EECode GetPipeline(TU8 type, void *&p_pipeline) = 0;
    virtual EECode FreePipeline(TU8 type, void *p_pipeline) = 0;

    virtual EECode ParseContentExtraData(void *p_content) = 0;

public:
    virtual EECode UpdateDeviceDynamicCode(TGenericID pipeline_id, TU32 dynamic_code) = 0;
    virtual EECode UpdateUserDynamicCode(const TChar *p_user_name, const TChar *p_device_name, TU32 dynamic_code) = 0;
    virtual EECode LookupPassword(const TChar *p_user_name, const TChar *p_device_name, TU32 &dynamic_code) = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~IGenericEngineControlV3() {}
};

class IGenericEngineControlV4
{
public:
    virtual EECode GetMediaConfig(volatile SPersistMediaConfig *&pconfig) const = 0;
    virtual EECode SetMediaConfig() = 0;

public:
    //log/debug related api
    virtual EECode SetEngineLogLevel(ELogLevel level) = 0;
    virtual EECode SetRuntimeLogConfig(TGenericID id, TU32 level, TU32 option, TU32 output, TU32 is_add = 0) = 0;
    virtual EECode PrintCurrentStatus(TGenericID id = 0, TULong print_flag = 0) = 0;

public:
    //build graph related
    virtual EECode BeginConfigProcessPipeline(TU32 customized_pipeline = 1) = 0;

    virtual EECode NewComponent(TU32 component_type, TGenericID &component_id, const TChar *prefer_string = NULL, void *p_external_context = NULL) = 0;
    virtual EECode ConnectComponent(TGenericID &connection_id, TGenericID upper_component_id, TGenericID down_component_id, StreamType pin_type) = 0;

    virtual EECode SetupPlaybackPipeline(TGenericID &playback_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID video_decoder_id, TGenericID audio_decoder_id, TGenericID video_renderer_id, TGenericID audio_renderer_id, TGenericID audio_pinmuxer_id = 0, TU8 running_at_startup = 1) = 0;
    virtual EECode SetupRecordingPipeline(TGenericID &recording_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID sink_id, TU8 running_at_startup = 1, TChar *p_channelname = NULL) = 0;
    virtual EECode SetupStreamingPipeline(TGenericID &streaming_pipeline_id, TGenericID streaming_transmiter_id, TGenericID streaming_server_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID audio_pinmuxer_id = 0, TU8 running_at_startup = 1) = 0;

    virtual EECode SetupCapturePipeline(TGenericID &capture_pipeline_id, TGenericID video_capture_source_id, TGenericID audio_capture_source_id, TGenericID video_encoder_id, TGenericID audio_encoder_id, TU8 running_at_startup = 1) = 0;
    virtual EECode SetupCloudUploadPipeline(TGenericID &upload_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID uploader_id, TU8 running_at_startup = 1) = 0;
    virtual EECode SetupVODPipeline(TGenericID &vod_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID flow_controller_id, TGenericID streaming_transmiter_id, TGenericID streaming_server_id, TU8 running_at_startup = 0) = 0;
    virtual EECode FinalizeConfigProcessPipeline() = 0;

public:
    //set source for source filters
    virtual EECode SetSourceUrl(TGenericID source_component_id, TChar *url) = 0;
    //set dst for sink filters
    virtual EECode SetSinkUrl(TGenericID sink_component_id, TChar *url) = 0;
    //set url for streaming
    virtual EECode SetStreamingUrl(TGenericID pipeline_id, TChar *url) = 0;
    //set tag for cloud client's receiver at server side
    virtual EECode SetCloudAgentTag(TGenericID agent_id, TChar *agent_tag, TGenericID server_id = 0) = 0;
    //set tag for cloud client's tag and remote server addr and port
    virtual EECode SetCloudClientTag(TGenericID agent_id, TChar *agent_tag, TChar *p_server_addr, TU16 server_port) = 0;

    //vod content
    virtual EECode SetupVodContent(TU32 channel, TChar *url, TU8 localfile, TU8 enable_video, TU8 enable_audio) = 0;

    //for external
    virtual EECode AssignExternalContext(TGenericID component_id, void *context) = 0;

    //app msg
    virtual EECode SetAppMsgCallback(void (*MsgProc)(void *, SMSG &), void *context) = 0;

    //flow related
    virtual EECode RunProcessing() = 0;
    virtual EECode ExitProcessing() = 0;

    virtual EECode Start() = 0;
    virtual EECode Stop() = 0;

    //pipeline related
    virtual EECode SuspendPipeline(TGenericID pipeline_id, TUint release_content) = 0;
    virtual EECode ResumePipeline(TGenericID pipeline_id, TU8 force_audio_path = 0, TU8 force_video_path = 0) = 0;

public:
    //configuration
    virtual EECode GenericPreConfigure(EGenericEnginePreConfigure config_type, void *param, TUint is_get = 0) = 0;
    virtual EECode GenericControl(EGenericEngineConfigure config_type, void *param) = 0;

public:
    virtual EECode GetPipeline(TU8 type, void *&p_pipeline) = 0;
    virtual EECode FreePipeline(TU8 type, void *p_pipeline) = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~IGenericEngineControlV4() {}
};

enum {
    EFactory_MediaEngineType_Generic = 0,
};

TU8 *gfGenerateAudioExtraData(StreamFormat format, TInt samplerate, TInt channel_number, TUint &size);

extern IGenericEngineControl *gfMediaEngineFactory(TUint type);
extern IGenericEngineControlV2 *gfMediaEngineFactoryV2(TUint type);
extern IGenericEngineControlV3 *gfMediaEngineFactoryV3(TUint type, IStorageManagement *p_storage, SSharedComponent *p_shared_component = NULL);
extern IGenericEngineControlV4 *gfMediaEngineFactoryV4(TUint type, IStorageManagement *p_storage, SSharedComponent *p_shared_component = NULL);

extern void gfGetMediaMWVersion(TU32 &major, TU32 &minor, TU32 &patch, TU32 &year, TU32 &month, TU32 &day);

typedef struct {
    TU32 check_field;
    TGenericID id;

    TU8 enable_video;
    TU8 enable_audio;
    TU8 b_video_const_framerate;
    TU8 b_video_have_b_frame;

    TU8 video_format;
    TU8 audio_format;
    TU8 reserved0;
    TU8 reserved1;

    TU32 timescale;
    TU32 video_frametick;
    TU32 audio_frametick;

    SDateTime create_time;
    SDateTime modification_time;

    TChar video_compressorname[32];
    TChar audio_compressorname[32];

    TU64 duration;

    TU32 video_width;
    TU32 video_height;
    TU32 audio_channel_number;
    TU32 audio_samplerate;
    TU32 audio_framesize;

    TU32 video_framecount;
    TU32 audio_framecount;
} SRecordSpecifiedInfo;

typedef enum {
    EMediaFileParser_BasicAVC = 0x00,
    EMediaFileParser_BasicHEVC = 0x01,

    EMediaFileParser_BasicAAC = 0x02,
} EMediaFileParser;

#define DMAX_MEDIA_SUB_PACKET_NUMBER 16
//media file parser
typedef struct {
    TU8 *p_data;
    TIOSize data_len;

    TU8 media_type;
    TU8 paket_type;
    TU8 frame_type;
    TU8 have_pts;

    TTime pts;

    TU32 sub_packet_number;
    TIOSize offset_hint[DMAX_MEDIA_SUB_PACKET_NUMBER];
} SMediaPacket;

typedef struct {
    TU32 video_width;
    TU32 video_height;

    TU32 video_framerate_num;
    TU32 video_framerate_den;

    TU32 profile_indicator;
    TU32 level_indicator;

    TU8 audio_channel_number;
    TU8 audio_sample_size; // bits
    TU8 format;
    TU8 reserved1;

    TU32 audio_sample_rate;
    TU32 audio_frame_size;
} SMediaInfo;

class IMediaFileParser
{
public:
    virtual void Delete() = 0;

protected:
    virtual ~IMediaFileParser() {}

public:
    virtual EECode OpenFile(TChar *filename) = 0;
    virtual EECode CloseFile() = 0;
    virtual EECode ReadPacket(SMediaPacket *packet) = 0;
    virtual EECode SeekTo(TIOSize offset) = 0;

    virtual EECode GetMediaInfo(SMediaInfo *info) = 0;
};

IMediaFileParser *gfCreateMediaFileParser(EMediaFileParser type);

typedef struct {
    TU8 frame_type;
    TU8 have_pts;
    TU8 have_dts;
    TU8 new_seq;

    TU32 frame_seq_num;

    TTime pts;
    TTime dts;

    TU32 sub_packet_number;
    TIOSize offset_hint[DMAX_MEDIA_SUB_PACKET_NUMBER];
} SEsInfo;

//external module
class IExternalSourcePushModeES
{
public:
    virtual EECode PushEs(TU8 *data, TU32 len, SEsInfo *info) = 0;
    virtual EECode AllocMemory(TU8 *&mem, TU32 len) = 0;
    virtual EECode ReturnMemory(TU8 *retm, TU32 len) = 0;

    virtual EECode SetMediaInfo(SMediaInfo *info) = 0;
    virtual void SendEOS() = 0;

protected:
    virtual ~IExternalSourcePushModeES() {}
};

//some misc funtion
extern EECode gfPresetDisplayLayout(SVideoPostPGlobalSetting *p_global_setting, SVideoPostPDisplayLayout *p_input_param, SVideoPostPConfiguration *p_result);
extern EECode gfDefaultRenderConfig(SDSPMUdecRender *render, TUint number_1, TUint number_2);
extern const TChar *gfGetComponentStringFromGenericComponentType(TComponentType type);
extern EECode calculateDisplayLayoutWindow(SDSPMUdecWindow *p_window, TU16 display_width, TU16 display_height, TU8 layout, TU8 window_number, TU8 window_start_index);

//hack here
extern void *gfCreateOPTCAudioResamplerProxy(int out_rate, int in_rate, int filter_size, int phase_shift, int linear, double cutoff);
extern void gfDestroyOPTCAudioResamplerProxy(void *context);
extern int gfOPTCAudioResampleProxy(void *context, short *dst, short *src, int src_size, int dst_size, int update_ctx, int *remain_samples);
extern int gfOPTCAudioResampleGetRemainSamplesProxy(void *context, short *dst);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

