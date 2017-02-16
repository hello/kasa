/**
 * internal.h
 *
 * History:
 *  2015/07/24 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __INTERNAL_H__
#define __INTERNAL_H__

#define DCorpLOGO "Ambarella"

//platform related
#define DNameTag_AMBA "AMBA"
#define DNameTag_ALSA "ALSA"
#define DNameTag_DirectSound "DirectSound"
#define DNameTag_DirectDraw "DirectDraw"
#define DNameTag_OpenGL "OpenGL"
#define DNameTag_WDup "WDup"
#define DNameTag_MMD "MMD"
#define DNameTag_AndroidAudioInput "AndroidAI"
#define DNameTag_AndroidAudioOutput "AndroidAO"
#define DNameTag_LinuxFB "LinuxFB"

//private demuxer muxer
#define DNameTag_PrivateMP4 "PRMP4"
#define DNameTag_PrivateTS "PRTS"
#define DNameTag_PrivateRTSP "PRRTSP"

//external
#define DNameTag_FFMpeg "FFMpeg"

#define DSYSTEM_MAX_CHANNEL_NUM 4
#define DMAX_FILE_NAME_LENGTH 512

#define DINVALID_VALUE_TAG_64 0xFEDCFEFEFEDCFEFELL

#define DFLAG_STATUS_EOS 0
#define DCAL_BITMASK(x) (1 << x)

//some const
enum {
    EComponentMaxNumber_Demuxer = DSYSTEM_MAX_CHANNEL_NUM,
    EComponentMaxNumber_VideoDecoder = DSYSTEM_MAX_CHANNEL_NUM,
    EComponentMaxNumber_AudioDecoder = DSYSTEM_MAX_CHANNEL_NUM,
    EComponentMaxNumber_VideoEncoder = DSYSTEM_MAX_CHANNEL_NUM,
    EComponentMaxNumber_AudioEncoder = DSYSTEM_MAX_CHANNEL_NUM,
    EComponentMaxNumber_VideoRenderer = 1,
    EComponentMaxNumber_AudioRenderer = 1,
    EComponentMaxNumber_Muxer = DSYSTEM_MAX_CHANNEL_NUM,
    EComponentMaxNumber_RTSPStreamingTransmiter = DSYSTEM_MAX_CHANNEL_NUM,
    EComponentMaxNumber_FlowController = DSYSTEM_MAX_CHANNEL_NUM,

    EComponentMaxNumber_RTSPStreamingServer = 1,
    EComponentMaxNumber_RTSPStreamingContent = DSYSTEM_MAX_CHANNEL_NUM * 2,
    EComponentMaxNumber_ConnectionPin = DSYSTEM_MAX_CHANNEL_NUM * 32,

    EPipelineMaxNumber_Playback = DSYSTEM_MAX_CHANNEL_NUM,
    EPipelineMaxNumber_Recording = DSYSTEM_MAX_CHANNEL_NUM,
    EPipelineMaxNumber_Streaming = DSYSTEM_MAX_CHANNEL_NUM,
};

enum {
    EMaxStreamingSessionNumber = DSYSTEM_MAX_CHANNEL_NUM * 8,
    EMaxSinkNumber_Demuxer = 32,
    EMaxSourceNumber_RecordingPipeline = 2,
    EMaxSourceNumber_StreamingPipeline = 2,

    EMaxSourceNumber_Muxer = 4,
    EMaxSourceNumber_Streamer = 32,
};

typedef enum {
    EPixelFMT_invalid = 0x00,
    EPixelFMT_rgba32 = 0x01,
    EPixelFMT_yuv420p = 0x02,
} EPixelFMT;

//-----------------------------------------------------------------------
//
//  Video related defines
//
//-----------------------------------------------------------------------

enum {
    //from dsp define
    EPredefinedPictureType_IDR = 1,
    EPredefinedPictureType_I = 2,
    EPredefinedPictureType_P = 3,
    EPredefinedPictureType_B = 4,
};

//refer to 264 spec table 7.1
enum {
    ENalType_unspecified = 0,
    ENalType_IDR = 5,
    ENalType_SEI = 6,
    ENalType_SPS = 7,
    ENalType_PPS = 8,
};

//refer to 265 spec table 7.1
enum {
    EHEVCNalType_TRAIL_N = 0,
    EHEVCNalType_TRAIL_R = 1,
    EHEVCNalType_TSA_N = 2,
    EHEVCNalType_TSA_R = 3,
    EHEVCNalType_STSA_N = 4,
    EHEVCNalType_STSA_R = 5,
    EHEVCNalType_RADL_N = 6,
    EHEVCNalType_RADL_R = 7,
    EHEVCNalType_RASL_N = 8,
    EHEVCNalType_RASL_R = 9,
    EHEVCNalType_RSV_VCL_N10 = 10,
    EHEVCNalType_RSV_VCL_R11 = 11,
    EHEVCNalType_RSV_VCL_N12 = 12,
    EHEVCNalType_RSV_VCL_R13 = 13,
    EHEVCNalType_RSV_VCL_N14 = 14,
    EHEVCNalType_RSV_VCL_R15 = 15,
    EHEVCNalType_BLA_W_LP = 16,
    EHEVCNalType_BLA_W_RADL = 17,
    EHEVCNalType_BLA_N_LP = 18,
    EHEVCNalType_IDR_W_RADL = 19,
    EHEVCNalType_IDR_N_LP = 20,
    EHEVCNalType_CRA_NUT = 21,
    EHEVCNalType_RSV_IRAP_VCL22 = 22,
    EHEVCNalType_RSV_IRAP_VCL23 = 23,
    EHEVCNalType_RSV_VCL24 = 24,
    EHEVCNalType_RSV_VCL25 = 25,
    EHEVCNalType_RSV_VCL26 = 26,
    EHEVCNalType_RSV_VCL27 = 27,
    EHEVCNalType_RSV_VCL28 = 28,
    EHEVCNalType_RSV_VCL29 = 29,
    EHEVCNalType_RSV_VCL30 = 30,
    EHEVCNalType_RSV_VCL31 = 31,
    EHEVCNalType_VPS = 32,
    EHEVCNalType_SPS = 33,
    EHEVCNalType_PPS = 34,
    EHEVCNalType_AUD = 35,
    EHEVCNalType_EOS = 36,
    EHEVCNalType_EOB = 37,
    EHEVCNalType_FD = 38,
    EHEVCNalType_PREFIX_SEI = 39,
    EHEVCNalType_SUFFIX_SEI = 40,
    EHEVCNalType_RSV_NVCL41 = 41,
    EHEVCNalType_RSV_NVCL42 = 42,
    EHEVCNalType_RSV_NVCL43 = 43,
    EHEVCNalType_RSV_NVCL44 = 44,
    EHEVCNalType_RSV_NVCL45 = 45,
    EHEVCNalType_RSV_NVCL46 = 46,
    EHEVCNalType_RSV_NVCL47 = 47,

    EHEVCNalType_VCL_END = 31,
};

enum {
    ERTCPType_SR = 200,
    ERTCPType_RR = 201,
    ERTCPType_SDC = 202,
    ERTCPType_BYE = 203,
};

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
    RTCP_SR     = 200,
    RTCP_RR     = 201,
    RTCP_SDES   = 202,
    RTCP_BYE    = 203,
    RTCP_APP    = 204,
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

class SCMD
{
public:
    TUint code;
    void *pExtra;
    TU8 repeatType;
    TU8 flag;
    TU8 needFreePExtra;
    TU8 reserved1;
    TU32 res32_1;
    TU64 res64_1;
    TU64 res64_2;

public:
    SCMD(TUint cid) { code = cid; pExtra = NULL; needFreePExtra = 0; repeatType = 0;}
    SCMD() {pExtra = NULL; needFreePExtra = 0; repeatType = 0;}
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

#define RTSP_MAX_BUFFER_SIZE 4096
#define RTSP_MAX_DATE_BUFFER_SIZE 512

#define DPresetSocketTimeOutUintSeconds 0
#define DPresetSocketTimeOutUintUSeconds 300000

#define DInvalidTimeStamp (-1LL)

#define DMaxChannelNameLength 128

#define DRTP_HEADER_FIXED_LENGTH 12

#define DREAD_BE16(x) (((*((TU8*)x))<<8) | (*((TU8*)x + 1)))
#define DREAD_BE32(x) (((*((TU8*)x))<<24) | ((*((TU8*)x + 1))<<16) | ((*((TU8*)x + 2))<<8) | (*((TU8*)x + 3)))
#define DREAD_BE64(x) (((TU64)(*((TU8*)x))<<56) | ((TU64)(*((TU8*)x + 1))<<48) | ((TU64)(*((TU8*)x + 2))<<40) | ((TU64)(*((TU8*)x + 3))<<32) | ((TU64)(*((TU8*)x + 4))<<24) | ((TU64)(*((TU8*)x + 5))<<16) | ((TU64)(*((TU8*)x + 6))<<8) | (TU64)(*((TU8*)x + 7)))

#define DMIN(a,b) ((a) > (b) ? (b) : (a))
#define DMAX(a,b) ((a) > (b) ? (a) : (b))

#define BE_16(x) (((TU8 *)(x))[0] <<  8 | ((TU8 *)(x))[1])

#define DBEW64(x, p) do { \
        p[0] = (x >> 56) & 0xff; \
        p[1] = (x >> 48) & 0xff; \
        p[2] = (x >> 40) & 0xff; \
        p[3] = (x >> 32) & 0xff; \
        p[4] = (x >> 24) & 0xff; \
        p[5] = (x >> 16) & 0xff; \
        p[6] = (x >> 8) & 0xff; \
        p[7] = x & 0xff; \
    } while(0)

#define DBEW48(x, p) do { \
        p[0] = (x >> 40) & 0xff; \
        p[1] = (x >> 32) & 0xff; \
        p[2] = (x >> 24) & 0xff; \
        p[3] = (x >> 16) & 0xff; \
        p[4] = (x >> 8) & 0xff; \
        p[5] = x & 0xff; \
    } while(0)

#define DBEW32(x, p) do { \
        p[0] = (x >> 24) & 0xff; \
        p[1] = (x >> 16) & 0xff; \
        p[2] = (x >> 8) & 0xff; \
        p[3] = x & 0xff; \
    } while(0)

#define DBEW24(x, p) do { \
        p[0] = (x >> 16) & 0xff; \
        p[1] = (x >> 8) & 0xff; \
        p[2] = (x) & 0xff; \
    } while(0)

#define DBEW16(x, p) do { \
        p[0] = (x >> 8) & 0xff; \
        p[1] = x & 0xff; \
    } while(0)

#define DBER64(x, p) do { \
        x = ((TU64)p[0] << 56) | ((TU64)p[1] << 48) | ((TU64)p[2] << 40) | ((TU64)p[3] << 32) | ((TU64)p[4] << 24) | ((TU64)p[5] << 16) | ((TU64)p[6] << 8) | (TU64)p[7]; \
    } while(0)

#define DBER48(x, p) do { \
        x = ((TU64)p[0] << 40) | ((TU64)p[1] << 32) | ((TU64)p[2] << 24) | ((TU64)p[3] << 16) | ((TU64)p[4] << 8) | (TU64)p[5] ; \
    } while(0)

#define DBER32(x, p) do { \
        x = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; \
    } while(0)

#define DBER24(x, p) do { \
        x = (p[0] << 16) | (p[1] << 8) | p[2]; \
    } while(0)

#define DBER16(x, p) do { \
        x = (p[0] << 8) | p[1]; \
    } while(0)

#define DLEW64(x, p) do { \
        p[0] = x & 0xff; \
        p[1] = (x >> 8) & 0xff; \
        p[2] = (x >> 16) & 0xff; \
        p[3] = (x >> 24) & 0xff; \
        p[4] = (x >> 32) & 0xff; \
        p[5] = (x >> 40) & 0xff; \
        p[6] = (x >> 48) & 0xff; \
        p[7] = (x >> 56) & 0xff; \
    } while(0)

#define DLEW32(x, p) do { \
        p[0] = x & 0xff; \
        p[1] = (x >> 8) & 0xff; \
        p[2] = (x >> 16) & 0xff; \
        p[3] = (x >> 24) & 0xff; \
    } while(0)

#define DLEW16(x, p) do { \
        p[0] = x & 0xff; \
        p[1] = (x >> 8) & 0xff; \
    } while(0)

#define DLER64(x, p) do { \
        x = ((TU64)p[7] << 56) | ((TU64)p[6] << 48) | ((TU64)p[5] << 40) | ((TU64)p[4] << 32) | ((TU64)p[3] << 24) | ((TU64)p[2] << 16) | ((TU64)p[1] << 8) | (TU64)p[0]; \
    } while(0)

#define DLER32(x, p) do { \
        x = (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0]; \
    } while(0)

#define DLER16(x, p) do { \
        x = (p[1] << 8) | p[0]; \
    } while(0)

#ifndef DROUND_UP
#define DROUND_UP(_size, _align)    (((_size) + ((_align) - 1)) & ~((_align) - 1))
#endif

#ifndef DROUND_DOWN
#define DROUND_DOWN(_size, _align)  ((_size) & ~((_align) - 1))
#endif

#ifndef DCAL_MIN
#define DCAL_MIN(_a, _b)    ((_a) < (_b) ? (_a) : (_b))
#endif

#ifndef DCAL_MAX
#define DCAL_MAX(_a, _b)    ((_a) > (_b) ? (_a) : (_b))
#endif

#ifndef DARRAY_SIZE
#define DARRAY_SIZE(_array) (sizeof(_array) / sizeof(_array[0]))
#endif

#ifndef DPTR_OFFSET
#define DPTR_OFFSET(_type, _member) ((int)&((_type*)0)->member)
#endif

#ifndef DPTR_ADD
#define DPTR_ADD(_ptr, _size)   (void*)((char*)(_ptr) + (_size))
#endif

#define DDefaultTimeScale 90000
#define DDefaultVideoFramerateNum 90000
#define DDefaultVideoFramerateDen 3003
#define DDefaultAudioSampleRate 48000
#define DDefaultAudioFrameSize 1024
#define DDefaultAudioChannelNumber 1
#define DDefaultAudioBufferNumber 24
#define DDefaultAudioOutputBufferFrameCount 48

#define AUDIO_CHUNK_SIZE 1024
#define SINGLE_CHANNEL_MAX_AUDIO_BUFFER_SIZE (AUDIO_CHUNK_SIZE*sizeof(TS16))
#define MAX_AUDIO_BUFFER_SIZE (SINGLE_CHANNEL_MAX_AUDIO_BUFFER_SIZE*2)

#define DMaxFileExterntionLength 32
#define DMaxFileIndexLength 32

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

//some constant
enum {
    EConstMaxDemuxerMuxerStreamNumber = 4,

    EConstDemxerMaxOutputPinNumber = 8,

    EConstVideoEncoderMaxInputPinNumber = 4,
    EConstVideoDecoderMaxInputPinNumber = 16,
    EConstAudioDecoderMaxInputPinNumber = 32,
    EConstAudioRendererMaxInputPinNumber = 8,
    EConstMuxerMaxInputPinNumber = 4,
    EConstVideoRendererMaxInputPinNumber = 16,

    EConstStreamingTransmiterMaxInputPinNumber = 128,
};

enum {
    EDemuxerVideoOutputPinIndex = 0x0,
    EDemuxerAudioOutputPinIndex = 0x1,
    EDemuxerSubtitleOutputPinIndex = 0x2,
    EDemuxerPrivateDataOutputPinIndex = 0x3,

    EDemuxerOutputPinNumber = 0x4,
};

typedef enum {
    EServerState_running = 0x0,
    EServerState_shuttingdown = 0x01,
    EServerState_closed = 0x02,
} EServerState;

typedef enum {
    EServerManagerState_idle = 0x0,
    EServerManagerState_noserver_alive = 0x01,
    EServerManagerState_running = 0x02,
    EServerManagerState_halt = 0x03,
    EServerManagerState_error = 0x04,
} EServerManagerState;

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

typedef struct {
    MuxerSavingFileStrategy strategy;
    MuxerSavingCondition condition;
    MuxerAutoFileNaming naming;
    TUint param;
} SRecSavingStrategy;

#define DMaxSchedulerGroupNumber 8

typedef struct {
    TU8 use_tcp_mode_first;
    TU8 parse_multiple_access_unit;

    TU16 rtp_rtcp_port_start;
} SRTSPClientConfig;

typedef struct {
    TU16 rtsp_listen_port;
    TU16 rtp_rtcp_port_start;

    TU8 enable_nonblock_timeout;
    TU8 video_stream_id;
    TU8 reserved1;
    TU8 reserved2;

    TS32 timeout_threashold;
} SRTSPServerConfig;

typedef struct {
    // input
    TInt device_fd;

    TU8 use_digital_vout;
    TU8 use_hdmi_vout;
    TU8 use_cvbs_vout;
    TU8 reserved0;
} SDSPConfig;

typedef struct {
    TU8 record_strategy;
    TU8 record_autocut_condition;
    TU8 record_autocut_naming;
    TU8 reserved0;
    TU32 record_autocut_framecount;
    TU32 record_autocut_duration;
} SRecordAutoCutSetting;

typedef struct {
    TChar audio_device_name[32];
} SAudioDeviceConfig;

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

class IMutex;
class IScheduler;
class IGenericEngineControl;
class IClockSource;
class CIClockManager;
class CIClockReference;
typedef struct {
    TU8 streaming_enable, compensate_from_jitter, cutfile_with_precise_pts, app_start_exit;

    //rtsp streaming related
    TU8 rtsp_streaming_enable, rtsp_streaming_video_enable, rtsp_streaming_audio_enable, try_requested_video_framerate;
    SRTSPServerConfig rtsp_server_config;

    //rtsp client related
    SRTSPClientConfig rtsp_client_config;

    //dsp related
    SDSPConfig dsp_config;

    SRecordAutoCutSetting auto_cut;

    //audio device related
    SAudioDeviceConfig audio_device;

    SPlaybackDecodeSetting pb_decode;

    //internal use only:
    //scheduler related
    IScheduler *p_scheduler_network_reciever[DMaxSchedulerGroupNumber];
    IScheduler *p_scheduler_network_tcp_reciever[DMaxSchedulerGroupNumber];
    IScheduler *p_scheduler_network_sender[DMaxSchedulerGroupNumber];
    IScheduler *p_scheduler_io_writer[DMaxSchedulerGroupNumber];
    IScheduler *p_scheduler_io_reader[DMaxSchedulerGroupNumber];

    IScheduler *p_scheduler_video_decoder[DMaxSchedulerGroupNumber];

    IClockSource *p_clock_source;
    CIClockManager *p_system_clock_manager;
    CIClockReference *p_system_clock_reference;

    TU8 number_scheduler_network_reciever;
    TU8 number_scheduler_network_tcp_reciever;
    TU8 number_scheduler_network_sender;
    TU8 number_scheduler_io_writer;

    TU8 number_scheduler_io_reader;
    TU8 playback_prefetch_number;
    TU8 enable_rtsp_authentication;
    TU8 rtsp_authentication_mode;// 0: basic, 1: digest

    TTime streaming_timeout_threashold;
    TTime streaming_autoconnect_interval;

    IGenericEngineControl *p_engine;

    IMutex *p_global_mutex;
} SPersistMediaConfig ;

typedef struct {
    TU16 year;
    TU8 month;
    TU8 day;

    TU8 hour;
    TU8 minute;
    TU8 seconds;

    TU8 weekday : 4;
    TU8 specify_weekday : 1;
    TU8 specify_hour : 1;
    TU8 specify_minute : 1;
    TU8 specify_seconds : 1;
} SDateTime;

typedef struct {
    TS32 days;

    TU64 total_hours;
    TU32 total_minutes;
    TU32 total_seconds;

    TU64 overall_seconds;
} SDateTimeGap;

void gfDateCalculateGap(SDateTime &current_date, SDateTime &start_date, SDateTimeGap &gap);
void gfDateNextDay(SDateTime &date, TU32 days);
void gfDatePreviousDay(SDateTime &date, TU32 days);
void gfDateCalculateNextTime(SDateTime &next_date, SDateTime &start_date, TU64 seconds);

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
    TU8 total_stream_number;
    TU8 reserved0, reserved1, reserved2;
    SStreamCodecInfo info[EConstMaxDemuxerMuxerStreamNumber];
} SStreamCodecInfos;

typedef enum {
    ESessionClientState_Init = 0,
    ESessionClientState_Ready,
    ESessionClientState_Playing,
    ESessionClientState_Recording,

    EClientSessionState_Not_Setup,
    EClientSessionState_Error,
    EClientSessionState_Timeout,
    EClientSessionState_Zombie,
} ESessionClientState;

typedef enum {
    ESessionServerState_Init = 0,
    ESessionServerState_Ready,
    ESessionServerState_Playing,
    ESessionServerState_Recording,

    ESessionServerState_authenticationDone,
}  ESessionServerState;

//-----------------------------------------------------------------------
//
// streaming
//
//-----------------------------------------------------------------------
typedef struct {
    TU16 first_seq_num;
    TU16 cur_seq_num;

    TU32 octet_count;
    TU32 packet_count;
    TU32 last_octet_count;
    TTime last_rtcp_ntp_time;
    TTime first_rtcp_ntp_time;
    TU32 base_timestamp;
    TU32 timestamp;
} SRTCPServerStatistics;

//-----------------------------------------------------------------------
//
// SStreamingContent
//
//-----------------------------------------------------------------------
#define DMaxStreamContentUrlLength 64

enum {
    ESubSession_video = 0,
    ESubSession_audio,

    ESubSession_tot_count,
};

class IStreamingTransmiter;
class IStreamingServer;
typedef struct {
    TComponentIndex transmiter_input_pin_index;
    TU8 type;//StreamType;
    TU8 format;//StreamFormat format;

    TU8 enabled;
    TU8 data_comes;
    TU8 parameters_setup;
    TU8 reserved1;

    //video info
    TU32 video_width, video_height;
    TU32 video_bit_rate;//, video_framerate;
    TU32 video_framerate_num, video_framerate_den;
    float video_framerate;

    //audio info
    TU32 audio_channel_number, audio_sample_rate;
    TU32 audio_bit_rate, audio_sample_format;
    TU32 audio_frame_size;

    //port_base
    TU16 server_rtp_port;
    TU16 server_rtcp_port;

    TChar profile_level_id_base16[8];
    TChar sps_base64[256];
    TChar pps_base64[32];

    TChar aac_config_base16[32];

    IStreamingTransmiter *p_transmiter;
} SStreamingSubSessionContent;

class CIDoubleLinkedList;
class IFilter;
typedef struct {
    TGenericID id;

    TComponentIndex content_index;
    TU8 b_content_setup;
    TU8 sub_session_count;

    TU8 enabled;
    TU8 has_video;
    TU8 has_audio;
    TU8 enable_remote_control;

    //SStreamingSubSessionContent sub_session_content[ESubSession_tot_count];
    SStreamingSubSessionContent *sub_session_content[ESubSession_tot_count];

    TChar session_name[DMaxStreamContentUrlLength];

    IStreamingServer *p_server;

    CIDoubleLinkedList *p_client_list;

    IFilter *p_streaming_transmiter_filter;
    IFilter *p_flow_controller_filter;
    IFilter *p_video_source_filter;
    IFilter *p_audio_source_filter;

    //vod
    TU8 content_type;// 0: streaming; 1: vod file from storage; 2: vod file seted by '-f filname'
    TU8 reserved_0;
    TU8 reserved_1;
    TU8 reserved_2;
} SStreamingSessionContent;

extern StreamFormat gfGetStreamFormatFromString(TChar *str);
extern EntropyType gfGetEntropyTypeFromString(char *str);
extern ContainerType gfGetContainerTypeFromString(char *str);
extern AudioSampleFMT gfGetSampleFormatFromString(char *str);
extern const TChar *gfGetContainerStringFromType(ContainerType type);
extern IScheduler *gfGetNetworkReceiverScheduler(const volatile SPersistMediaConfig *p, TUint index);
extern IScheduler *gfGetNetworkReceiverTCPScheduler(const volatile SPersistMediaConfig *p, TUint index);
extern IScheduler *gfGetFileIOWriterScheduler(const volatile SPersistMediaConfig *p, TU32 index);

extern void gfEncodingBase16(TChar *out, const TU8 *in, TInt in_size);
extern void gfDecodingBase16(TU8 *out, const TU8 *in, TInt in_size);
extern TInt gfDecodingBase64(TU8 *out, const TU8 *in_str, TInt out_size);
extern TChar *gfEncodingBase64(TChar *out, TInt out_size, const TU8 *in, TInt in_size);

extern TU32 gfRandom32(void);
extern TU8 *gfGenerateAudioExtraData(StreamFormat format, TInt samplerate, TInt channel_number, TUint &size);

extern EECode gfSDPProcessVideoExtraData(SStreamingSubSessionContent *subsession, TU8 *p_extra_data, TU32 size);
extern EECode gfSDPProcessAudioExtraData(SStreamingSubSessionContent *subsession, TU8 *p_extra_data, TU32 size);

extern const TChar *gfGetServerManagerStateString(EServerManagerState state);

//-----------------------------------------------------------------------
//
//  CObject
//
//-----------------------------------------------------------------------
class CObject
{
public:
    virtual void Delete();
    virtual ~CObject();

public:
    virtual void PrintStatus();

public:
    CObject(const TChar *name, TUint index = 0);
    virtual void SetLogLevel(TUint level);
    virtual void SetLogOption(TUint option);
    virtual void SetLogOutput(TUint output);

public:
    virtual const TChar *GetModuleName() const;
    virtual TUint GetModuleIndex() const;

protected:
    TUint mConfigLogLevel;
    TUint mConfigLogOption;
    TUint mConfigLogOutput;

protected:
    const TChar *mpModuleName;

protected:
    TUint mIndex;

protected:
    TUint mDebugHeartBeat;

protected:
    FILE *mpLogOutputFile;
};

//-----------------------------------------------------------------------
//
// IClockSource
//
//-----------------------------------------------------------------------
typedef enum {
    EClockSourceState_stopped = 0,
    EClockSourceState_running,
    EClockSourceState_paused,
} EClockSourceState;

class IClockSource
{
public:
    virtual TTime GetClockTime(TUint relative = 1) const = 0;
    virtual TTime GetClockBase() const = 0;

    virtual void SetClockFrequency(TUint num, TUint den) = 0;
    virtual void GetClockFrequency(TUint &num, TUint &den) const = 0;

    virtual void SetClockState(EClockSourceState state) = 0;
    virtual EClockSourceState GetClockState() const = 0;

    virtual void UpdateTime() = 0;

public:
    virtual CObject *GetObject0() const = 0;

protected:
    virtual ~IClockSource() {}
};

typedef enum {
    EClockSourceType_generic = 0,
} EClockSourceType;

extern IClockSource *gfCreateClockSource(EClockSourceType type = EClockSourceType_generic);

typedef enum {
    EIOPosition_NotSpecified = 0,
    EIOPosition_Begin,
    EIOPosition_End,
    EIOPosition_Current,
} EIOPosition;

typedef enum {
    EIOType_File = 0,
    EIOType_HTTP,
} EIOType;

enum {
    EIOFlagBit_Read = (1 << 0),
    EIOFlagBit_Write = (1 << 1),
    EIOFlagBit_Append = (1 << 2),
    EIOFlagBit_Text = (1 << 3),
    EIOFlagBit_Binary = (1 << 4),
    EIOFlagBit_Exclusive = (1 << 5),
};

class IIO
{
public:
    virtual void Delete() = 0;

protected:
    virtual ~IIO() {}

public:
    virtual EECode Open(TChar *name, TU32 flags) = 0;
    virtual EECode Close() = 0;

    virtual EECode SetProperty(TIOSize write_block_size, TIOSize read_block_size) = 0;
    virtual EECode GetProperty(TIOSize &write_block_size, TIOSize &read_block_size) const = 0;

    virtual EECode Write(TU8 *pdata, TU32 size, TIOSize count, TU8 sync = 0) = 0;
    virtual EECode Read(TU8 *pdata, TU32 size, TIOSize &count) = 0;

    virtual EECode Seek(TIOSize offset, EIOPosition posision) = 0;
    virtual void Sync() = 0;

    virtual EECode Query(TIOSize &total_size, TIOSize &current_posotion) const = 0;
};

extern IIO *gfCreateIO(EIOType type);

extern void gfPrintCodecInfoes(SStreamCodecInfos *pInfo);

extern TU8 *__validStartPoint(TU8 *start, TUint &size);

#define SPS_PPS_LEN (57+8+6)
extern bool gfGetSpsPps(TU8 *pBuffer, TUint size, TU8 *&p_spspps, TUint &spspps_size, TU8 *&p_IDR, TU8 *&p_ret_data);
TU32 gfSkipDelimter(TU8 *p);
TU32 gfSkipSEI(TU8 *p, TU32 len);

extern int gfOpenIAVHandle();
extern void gfCloseIAVHandle(int fd);

typedef struct {
    TU32 check_field;
    TGenericID id;
    void *callback;
    void *callback_context;
} SExternalDataProcessingCallback;

#define DEmbededDataProcessingReSyncFlag 0x01
typedef EECode(*TEmbededDataProcessingCallBack)(void *owner, TU8 *pdata, TU32 datasize, TU32 dataflag);

typedef struct {
    ENavigationPosition position;
    TTime target;
} SInternalCMDSeek;

typedef struct {
    TTime time;
} SQueryLastShownTimeStamp;

#endif

