/**
 * lwmd_if.h
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

#ifndef __LWMD_IF_H__
#define __LWMD_IF_H__

//-----------------------------------------------------------------------
//
//  Basic types
//
//-----------------------------------------------------------------------

typedef int TInt;
typedef unsigned int TUint;

typedef unsigned char TU8;
typedef unsigned short TU16;
typedef unsigned int TU32;

typedef signed char TS8;
typedef signed short TS16;
typedef signed int TS32;

typedef signed long long TS64;
typedef unsigned long long TU64;

typedef long TLong;
typedef unsigned long TULong;

typedef TS64    TTime;
typedef char TChar;
typedef unsigned char TUChar;

typedef TS64 TFileSize;
typedef TS64 TIOSize;

typedef volatile int TAtomic;

typedef TULong TMemSize;
typedef TULong TPointer;
typedef TULong TFlag;

typedef TInt TSocketHandler;
typedef TU16 TSocketPort;
typedef TInt TSocketSize;

typedef TU16 TComponentIndex;
typedef TU8 TComponentType;
typedef TU32 TGenericID;

#define DInvalidSocketHandler (-1)
#define DIsSocketHandlerValid(x) (0 <= x)

//-----------------------------------------------------------------------
//
//  Error code
//
//-----------------------------------------------------------------------

typedef enum {
    EECode_OK = 0,

    EECode_NotInitilized = 0x001,
    EECode_NotRunning = 0x002,

    EECode_Error = 0x003,
    EECode_Closed = 0x004,
    EECode_Busy = 0x005,
    EECode_OSError = 0x006,
    EECode_IOError = 0x007,
    EECode_TimeOut = 0x008,
    EECode_TooMany = 0x009,
    EECode_OutOfCapability = 0x00a,

    EECode_NoMemory = 0x00c,
    EECode_NoPermission = 0x00d,
    EECode_NoImplement = 0x00e,
    EECode_NoInterface = 0x00f,

    EECode_NotExist = 0x010,
    EECode_NotSupported = 0x011,

    EECode_BadState = 0x012,
    EECode_BadParam = 0x013,
    EECode_BadCommand = 0x014,
    EECode_BadFormat = 0x015,
    EECode_BadMsg = 0x016,
    EECode_BadSessionNumber = 0x017,

    EECode_TryAgain = 0x018,
    EECode_DataCorruption = 0x019,

    EECode_InternalLogicalBug = 0x01c,
    EECode_ParseError = 0x021,

    EECode_UnknownError = 0x030,

    //not error, but a case, or need further API invoke
    EECode_OK_NoOutputYet = 0x43,
    EECode_OK_EOF = 0x45,
} EECode;

extern const TChar *gfGetErrorCodeString(EECode code);

//defines
#define DMaxPreferStringLen 16
#define DNonePerferString "AUTO"

#define DLikely(x)   __builtin_expect(!!(x),1)
#define DUnlikely(x)   __builtin_expect(!!(x),0)

extern void *gfDbgMalloc(TU32 size, const TChar *tag);
extern void gfDbgFree(void *p, const TChar *tag);
#if 0
#define DDBG_MALLOC(size, tag) gfDbgMalloc(size, tag)
#define DDBG_FREE(p, tag) gfDbgFree(p, tag)
#else
#define DDBG_MALLOC(size, tag) malloc(size)
#define DDBG_FREE(p, tag) free(p)
#endif

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
    StreamFormat_JPEG = 0x09,
    StreamFormat_AAC = 0x0a,
    StreamFormat_MPEG12Audio = 0x0b,
    StreamFormat_ADPCM = 0x0f,
    StreamFormat_PCMU = 0x12,
    StreamFormat_PCMA = 0x13,
    StreamFormat_H265 = 0x14,

    StreamFormat_PCM_S16 = 0x30,

    StreamFormat_FFMpegCustomized = 0x60,
};

enum PixFormat {
    PixFormat_YUV420P = 0,
    PixFormat_NV12,
    PixFormat_RGB565,
};

enum PrivateDataType {
    PrivateDataType_GPSInfo = 0,
    PrivateDataType_SensorInfo,
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

typedef union {
    SVideoParams video;
    SAudioParams audio;
} UFormatSpecific;

//-----------------------------------------------------------------------
//
//  Message and Command
//
//-----------------------------------------------------------------------

typedef enum {
    ECMDType_Invalid = 0x0000,

    ECMDType_FirstEnum = 0x0001,

    //common used
    ECMDType_Terminate = ECMDType_FirstEnum,
    ECMDType_StartRunning = 0x0002, //for AO, enter/exit OnRun
    ECMDType_ExitRunning = 0x0003,

    ECMDType_RunProcessing = 0x0004, //for processing, module specific work related, can not re-invoked start processing after exit processing
    ECMDType_ExitProcessing = 0x0005,

    ECMDType_Stop = 0x0010, //module start/stop, can be re-invoked
    ECMDType_Start = 0x0011,

    ECMDType_Pause = 0x0012,
    ECMDType_Resume = 0x0013,
    ECMDType_ResumeFlush = 0x0014,
    ECMDType_Flush = 0x0015,
    ECMDType_FlowControl = 0x0016,
    ECMDType_Suspend = 0x0017,
    ECMDType_ResumeSuspend = 0x0018,

    ECMDType_SwitchInput = 0x0019,

    ECMDType_SendData = 0x001a,
    ECMDType_Seek = 0x001b,

    ECMDType_Step = 0x0020,
    ECMDType_DebugDump = 0x0021,
    ECMDType_PrintCurrentStatus = 0x0022,
    ECMDType_SetRuntimeLogConfig = 0x0023,
    ECMDType_PlayUrl = 0x0024,
    ECMDType_PlayNewUrl = 0x0025,

    ECMDType_PurgeChannel = 0x0028,
    ECMDType_ResumeChannel = 0x0029,
    ECMDType_NavigationSeek = 0x002a,

    ECMDType_AddContent = 0x0030,
    ECMDType_RemoveContent = 0x0031,
    ECMDType_AddClient = 0x0032,
    ECMDType_RemoveClient = 0x0033,
    ECMDType_UpdateUrl = 0x0034,
    ECMDType_RemoveClientSession = 0x0035,
    ECMDType_UpdateRecSavingStrategy = 0x0036,

    //specific to each case
    ECMDType_ForceLowDelay = 0x0040,
    ECMDType_Speedup = 0x0041,
    ECMDType_DeleteFile = 0x0042,
    ECMDType_UpdatePlaybackSpeed = 0x0043,
    ECMDType_UpdatePlaybackLoopMode = 0x0044,
    ECMDType_DecoderZoom = 0x0045,

    ECMDType_EnableVideo = 0x0046,
    ECMDType_EnableAudio = 0x0047,

    ECMDType_NotifySynced = 0x0050,
    ECMDType_NotifySourceFilterBlocked = 0x0051,
    ECMDType_NotifyUDECInRuningState = 0x0052,
    ECMDType_NotifyBufferRelease = 0x0053,

    ECMDType_MuteAudio = 0x0060,
    ECMDType_UnMuteAudio = 0x0061,
    ECMDType_ConfigAudio = 0x0062,

    ECMDType_NotifyPrecacheDone = 0x0070,

    ECMDType_LastEnum = 0x0400,

} ECMDType;

typedef enum {
    EMSGType_Invalid = 0,

    EMSGType_FirstEnum = ECMDType_LastEnum,

    //common used
    EMSGType_ExternalMSG = EMSGType_FirstEnum,
    EMSGType_Timeout = 0x0401,
    EMSGType_InternalBug = 0x0402,
    EMSGType_RePlay = 0x0404,

    EMSGType_DebugDump_Flow = 0x0405,
    EMSGType_UnknownError = 0x406,
    EMSGType_MissionComplete = 0x407,
    EMSGType_TimeNotify = 0x408,
    EMSGType_DataNotify = 0x409,
    EMSGType_ControlNotify = 0x40a,
    EMSGType_ClientReconnectDone = 0x40b,

    EMSGType_StorageError = 0x0410,
    EMSGType_IOError = 0x0411,
    EMSGType_SystemError = 0x0412,
    EMSGType_DeviceError = 0x0413,

    EMSGType_StreamingError_TCPSocketConnectionClose = 0x0414,
    EMSGType_StreamingError_UDPSocketInvalidArgument = 0x0415,

    EMSGType_NetworkError = 0x0416,
    EMSGType_ServerError = 0x0417,

    EMSGType_DriverErrorBusy = 0x0420,
    EMSGType_DriverErrorNotSupport = 0x0421,
    EMSGType_DriverErrorOutOfCapability = 0x0422,
    EMSGType_DriverErrorNoPermmition = 0x0423,

    EMSGType_PlaybackEndNotify = 0x0428,
    EMSGType_AudioPrecacheReadyNotify = 0x0429,

    //for each user cases
    EMSGType_PlaybackEOS = 0x0430,
    EMSGType_RecordingEOS = 0x0431,
    EMSGType_NotifyNewFileGenerated = 0x0432,
    EMSGType_RecordingReachPresetDuration = 0x0433,
    EMSGType_RecordingReachPresetFilesize = 0x0434,
    EMSGType_RecordingReachPresetTotalFileNumbers = 0x0435,
    EMSGType_PlaybackStatisticsFPS = 0x0436,

    EMSGType_NotifyThumbnailFileGenerated = 0x0440,
    EMSGType_NotifyUDECStateChanges = 0x0441,
    EMSGType_NotifyUDECUpdateResolution = 0x0442,

    EMSGType_OpenSourceFail = 0x0450,
    EMSGType_OpenSourceDone = 0x0451,

    //application related
    EMSGType_WindowClose = 0x0460,
    EMSGType_WindowActive = 0x0461,
    EMSGType_WindowSize = 0x0462,
    EMSGType_WindowSizing = 0x0463,
    EMSGType_WindowMove = 0x0464,
    EMSGType_WindowMoving = 0x0465,

    EMSGType_WindowKeyPress = 0x0466,
    EMSGType_WindowKeyRelease = 0x0467,
    EMSGType_WindowLeftClick = 0x0468,
    EMSGType_WindowRightClick = 0x0469,
    EMSGType_WindowDoubleClick = 0x046a,
    EMSGType_WindowLeftRelease = 0x046b,
    EMSGType_WindowRightRelease = 0x046c,


    EMSGType_VideoSize = 0x0480,

    EMSGType_PostAgentMsg = 0x0490,

    EMSGType_ApplicationExit = 0x0500,

    //shared network related
    EMSGType_SayHelloDoneNotify = 0x0501,
    EMSGType_SayHelloFailNotify = 0x0502,
    EMSGType_SayHelloAlreadyInNotify = 0x503,

    EMSGType_JoinNetworkDoneNotify = 0x0504,
    EMSGType_JoinNetworkFailNotify = 0x0505,
    EMSGType_JoinNetworkAlreadyInNotify = 0x506,

    EMSGType_HandshakeDoneNotify = 0x0507,
    EMSGType_HandshakeFailNotify = 0x0508,

    EMSGType_SendFileAcceptNotify = 0x050a,
    EMSGType_SendFileDenyNotify = 0x050b,

    EMSGType_RequestDSSAcceptNotify = 0x050d,
    EMSGType_RequestDSSDenyNotify = 0x050e,

    EMSGType_QueryFriendListDoneNotify = 0x0512,
    EMSGType_QueryFriendListFailNotify = 0x0513,

    EMSGType_RequestDSS = 0x0540,
    EMSGType_RequestSendFile = 0x0541,
    EMSGType_RequestRecieveFile = 0x0542,

    EMSGType_HostDepartureNotify = 0x0550,
    EMSGType_FriendOnlineNotify = 0x0551,
    EMSGType_FriendOfflineNotify = 0x0552,
    EMSGType_FriendNotReachableNotify = 0x0553,
    EMSGType_FriendNotReachableNotifyFromHost = 0x0554,
    EMSGType_FriendInvitationNotify = 0x0555,

    EMSGType_HostIsNotReachableNotify = 0x0556,

    EMSGType_UserConfirmPermitDSS = 0x580,
    EMSGType_UserConfirmDenyDSS = 0x581,

    EMSGType_UserConfirmPermitRecieveFile = 0x0582,
    EMSGType_UserConfirmDenyRecieveFile = 0x0583,

    EMSGType_RecieveFileFinishNotify = 0x0584,
    EMSGType_SendFileFinishNotify = 0x0585,
    EMSGType_RecieveFileAbortNotify = 0x0586,

    EMSGType_DSViewEndNotify = 0x0587,
    EMSGType_DSShareAbortNotify = 0x0588,

} EMSGType;

typedef struct {
    TUint code;
    void    *pExtra;
    TU16 sessionID;
    TU8 flag;
    TU8 needFreePExtra;

    TU16 owner_index;
    TU8 owner_type;
    TU8 identifyer_count;

    TULong p_owner, owner_id;
    TULong p_agent_pointer;

    TULong p0, p1, p2, p3, p4;
    float f0;
} SMSG;

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

typedef enum {

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
} EGenericEngineConfigure;

typedef struct {
    TU32 check_field;
    TU32 port;
} SConfigRTSPServerPort;

typedef struct {
    TU32 check_field;
    TU32 stream_id;
} SConfigVideoStreamID;

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

//-----------------------------------------------------------------------
//
//  Media Engine
//
//-----------------------------------------------------------------------

#define DCAL_BITMASK(x) (1 << x)

#define DCOMPONENT_TYPE_IN_GID 24
#define DCOMPONENT_VALID_MARKER_BITS 23
#define DCOMPONENT_INDEX_MASK_IN_GID 0xffff

#define DCOMPONENT_TYPE_FROM_GENERIC_ID(x) (x >> DCOMPONENT_TYPE_IN_GID)
#define DCOMPONENT_INDEX_FROM_GENERIC_ID(x) (x & DCOMPONENT_INDEX_MASK_IN_GID)
#define DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(t, i) ((t << DCOMPONENT_TYPE_IN_GID) | (i & DCOMPONENT_INDEX_MASK_IN_GID) | (1 << DCOMPONENT_VALID_MARKER_BITS))
#define IS_VALID_COMPONENT_ID(x) (x & DCAL_BITMASK(DCOMPONENT_VALID_MARKER_BITS))

enum {
    EGenericPipelineState_not_inited = 0x0,
    EGenericPipelineState_build_gragh = 0x1,
    EGenericPipelineState_running = 0x3,
    EGenericPipelineState_suspended = 0x4,
};

enum {
    EGenericComponentType_TAG_filter_start = 0x00,
    EGenericComponentType_Demuxer = EGenericComponentType_TAG_filter_start,
    EGenericComponentType_VideoDecoder = 0x01,
    EGenericComponentType_AudioDecoder = 0x02,
    EGenericComponentType_VideoEncoder = 0x03,
    EGenericComponentType_AudioEncoder = 0x04,
    EGenericComponentType_VideoRenderer = 0x05,
    EGenericComponentType_AudioRenderer = 0x06,
    EGenericComponentType_Muxer = 0x07,
    EGenericComponentType_StreamingTransmiter = 0x08,
    EGenericComponentType_FlowController = 0x09,

    EGenericComponentType_TAG_filter_end,

    EGenericPipelineType_Playback = 0x70,
    EGenericPipelineType_Recording = 0x71,
    EGenericPipelineType_Streaming = 0x72,

    EGenericComponentType_TAG_proxy_start = 0x80,
    EGenericComponentType_StreamingServer = EGenericComponentType_TAG_proxy_start,
    EGenericComponentType_StreamingContent = 0x81,
    EGenericComponentType_ConnectionPin = 0x82,

    EGenericComponentType_TAG_proxy_end,
};

class IGenericEngineControl
{
public:
    virtual EECode PrintCurrentStatus(TGenericID id = 0, TULong print_flag = 0) = 0;

public:
    virtual EECode BeginConfigProcessPipeline(TU32 customized_pipeline = 1) = 0;
    virtual EECode FinalizeConfigProcessPipeline() = 0;

    virtual EECode NewComponent(TU32 component_type, TGenericID &component_id, const TChar *prefer_string = NULL) = 0;
    virtual EECode ConnectComponent(TGenericID &connection_id, TGenericID upper_component_id, TGenericID down_component_id, StreamType pin_type) = 0;

    virtual EECode SetupPlaybackPipeline(TGenericID &playback_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID video_decoder_id, TGenericID audio_decoder_id, TGenericID video_renderer_id, TGenericID audio_renderer_id) = 0;
    virtual EECode SetupRecordingPipeline(TGenericID &recording_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID sink_id) = 0;
    virtual EECode SetupStreamingPipeline(TGenericID &streaming_pipeline_id, TGenericID streaming_transmiter_id, TGenericID streaming_server_id, TGenericID video_source_id, TGenericID audio_source_id) = 0;

public:
    virtual EECode GenericControl(EGenericEngineConfigure config_type, void *param) = 0;

    //set urls
    virtual EECode SetSourceUrl(TGenericID source_component_id, TChar *url) = 0;
    virtual EECode SetSinkUrl(TGenericID sink_component_id, TChar *url) = 0;
    virtual EECode SetStreamingUrl(TGenericID pipeline_id, TChar *url) = 0;

    virtual EECode SetAppMsgCallback(void (*MsgProc)(void *, SMSG &), void *context) = 0;

    virtual EECode RunProcessing() = 0;
    virtual EECode ExitProcessing() = 0;

    virtual EECode Start() = 0;
    virtual EECode Stop() = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~IGenericEngineControl() {}
};

extern IGenericEngineControl *CreateLWMDEngine();

#endif

