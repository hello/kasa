/**
 * am_playback_new_if.h
 *
 * History:
 *  2015/07/24 - [Zhi He] create file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
 */

#ifndef __AM_PLAYBACK_NEW_IF_H__
#define __AM_PLAYBACK_NEW_IF_H__

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

#if __WORDSIZE == 64
typedef signed long int TS64;
typedef unsigned long int TU64;
#define DPrint64d "ld"
#define DPrint64u "lu"
#define DPrint64x "lx"
#else
typedef signed long long int TS64;
typedef unsigned long long int TU64;
#define DPrint64d "lld"
#define DPrint64u "llu"
#define DPrint64x "llx"
#endif

typedef long TLong;
typedef unsigned long TULong;

typedef TS64 TTime;
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

//-----------------------------------------------------------------------
//
//  Log related
//
//-----------------------------------------------------------------------

enum ELogLevel {
  ELogLevel_None = 0,
  ELogLevel_Fatal,
  ELogLevel_Error,
  ELogLevel_Warn,
  ELogLevel_Notice,
  ELogLevel_Info,
  ELogLevel_Debug,
  ELogLevel_Verbose,

  //last element
  ELogLevel_TotalCount,
};

enum ELogOption {
  ELogOption_State = 0,
  ELogOption_Congifuration,
  ELogOption_Flow,
  ELogOption_PTS,
  ELogOption_Resource,
  ELogOption_Timing,

  ELogOption_BinaryHeader,
  ELogOption_WholeBinary2WholeFile,
  ELogOption_WholeBinary2SeparateFile,

  //last element
  ELogOption_TotalCount,
};

enum ELogOutput {
  ELogOutput_Console = 0,
  ELogOutput_File,
  ELogOutput_ModuleFile,
  ELogOutput_AndroidLogcat,

  //binady dump
  ELogOutput_BinaryTotalFile,
  ELogOutput_BinarySeperateFile,

  //last element
  ELogOutput_TotalCount,
};

extern TU32 isLogFileOpened();
extern void gfOpenLogFile(TChar *name);
extern void gfCloseLogFile();
extern void gfSetLogLevel(ELogLevel level);
extern void gfPreSetAllLogLevel(ELogLevel level);

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
  EECode_DataMissing = 0x01a,

  EECode_InternalLogicalBug = 0x01c,
  EECode_ParseError = 0x021,

  EECode_UnknownError = 0x030,

  //not error, but a case, or need further API invoke
  EECode_OK_NoOutputYet = 0x43,
  EECode_OK_AlreadyStopped = 0x44,
  EECode_OK_EOF = 0x45,

  EECode_RTSP_ServerNoSession = 0x80,
  EECode_RTSP_ServerNotSupportTCP = 0x81,
  EECode_RTSP_ServerNotSupportUDP = 0x82,
  EECode_RTSP_ServerError = 0x83,
} EECode;

extern const char *gfGetErrorCodeString(EECode code);

typedef TU32 TGenericID;
typedef TS32 TDimension;

//defines
#define DMaxPreferStringLen 16
#define DCorpLOGO "Ambarella"

#define DNonePerferString "AUTO"

//platform related
#define DNameTag_AMBA "AMBA"
#define DNameTag_ALSA "ALSA"
#define DNameTag_LinuxFB "LinuxFB"
#define DNameTag_LinuxUVC "LinuxUVC"

//private demuxer muxer
#define DNameTag_PrivateMP4 "PRMP4"
#define DNameTag_PrivateTS "PRTS"
#define DNameTag_PrivateRTSP "PRRTSP"
#define DNameTag_PrivateVideoES "PRVES"

//external
#define DNameTag_FFMpeg "FFMpeg"
#define DNameTag_X264 "X264"
#define DNameTag_X265 "X265"
#define DNameTag_LIBAAC "LIBAAC"
#define DNameTag_HEVCHM "HEVCHM"

//injecters
#define DNameTag_AMBAEFM "AMBAEFM"

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
  ECMDType_ScreenCapture = 0x001d,

  ECMDType_SnapshotCapture = 0x001e,
  ECMDType_PeriodicCapture = 0x001f,

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
  ECMDType_SetServerUrlPort = 0x0037,
  ECMDType_BeginPlayback = 0x0038,

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
  EMSGType_VideoEditEOS = 0x0432,
  EMSGType_NotifyNewFileGenerated = 0x0433,
  EMSGType_RecordingReachPresetDuration = 0x0434,
  EMSGType_RecordingReachPresetFilesize = 0x0435,
  EMSGType_RecordingReachPresetTotalFileNumbers = 0x0436,
  EMSGType_PlaybackStatisticsFPS = 0x0437,
  EMSGType_StepPlayStatisticsFrameCount = 0x0438,

  EMSGType_NotifyThumbnailFileGenerated = 0x0440,
  EMSGType_NotifyUDECStateChanges = 0x0441,
  EMSGType_NotifyUDECUpdateResolution = 0x0442,
  EMSGType_NotifyDecoderReady = 0x0443,
  EMSGType_NotifyRendererReady = 0x0444,

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
  TU32 code;
  void    *pExtra;
  TU16 sessionID;
  TU8 flag;
  TU8 needFreePExtra;

  TU16 owner_index;
  TU8 owner_type;
  TU8 identifyer_count;

  TULong p_owner, owner_id;

  TULong p0, p1, p2, p3, p4;
  float f0;
} SMSG;

enum StreamType {
  StreamType_Invalid = 0,
  StreamType_Video,
  StreamType_Audio,
  StreamType_Subtitle,
  StreamType_PrivateData,

  StreamType_Cmd,

  StreamType_TotalNum,
};

typedef enum {
  ENavigationPosition_Invalid = 0,
  ENavigationPosition_Begining,
  ENavigationPosition_End,
  ENavigationPosition_LastKeyFrame,
} ENavigationPosition;

typedef enum {
  EPixelFMT_invalid = 0x00,
  EPixelFMT_rgba8888 = 0x01,
  EPixelFMT_yuv420p = 0x02,
  EPixelFMT_yuv422p = 0x03,
  EPixelFMT_yuv420_nv12 = 0x04,
  EPixelFMT_agbr8888 = 0x05,
  EPixelFMT_ayuv8888 = 0x06,
  EPixelFMT_rgb565 = 0x07,
  EPixelFMT_vyu565 = 0x08,
} EPixelFMT;

#define DAMBADSP_MAX_VOUT_NUMBER 4

enum {
    EAMDSP_VOUT_TYPE_INVALID = 0x00,
    EAMDSP_VOUT_TYPE_DIGITAL = 0x01,
    EAMDSP_VOUT_TYPE_HDMI = 0x02,
    EAMDSP_VOUT_TYPE_CVBS = 0x03,
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

  StreamFormat_PixelFormat_YUV420p = 0x40,
  StreamFormat_PixelFormat_NV12 = 0x41,
  StreamFormat_PixelFormat_YUYV = 0x42,
  StreamFormat_PixelFormat_YUV422p = 0x43,
  StreamFormat_PixelFormat_YVU420p = 0x44,
  StreamFormat_PixelFormat_GRAY8 = 0x45,

  StreamFormat_PCM_S16 = 0x58,

  StreamFormat_FFMpegCustomized = 0x68,
};

typedef struct {
    const TChar * mode_string;
    const TChar * sink_type_string;
    const TChar * device_string;
    TU8 vout_id;
    TU8 b_config_mixer;
    TU8 mixer_flag;
    TU8 b_direct_2_dsp;
} SVideoOutputConfigure;

typedef struct {
    SVideoOutputConfigure vout_config[DAMBADSP_MAX_VOUT_NUMBER];
    TU8 vout_number;
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;
} SVideoOutputConfigures;

enum DecoderFeedingRule {
  DecoderFeedingRule_NotValid = 0,
  DecoderFeedingRule_AllFrames,
  DecoderFeedingRule_RefOnly,
  DecoderFeedingRule_IOnly,
  DecoderFeedingRule_IDROnly,
};

typedef struct {
  TU32 check_field;

  TU8 b_digital_vout;
  TU8 b_hdmi_vout;
  TU8 b_cvbs_vout;
  TU8 reserved0;
} SConfigVout;

typedef struct {
  TU32 check_field;
  char audio_device_name[32];
} SConfigAudioDevice;

typedef struct {
  TU8 check_field;
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

typedef struct {
  TU32 check_field;
  TGenericID component_id;
  TTime time;
} SQueryLastShownTimeStamp;

typedef struct {
  TU32 check_field;
  TGenericID component_id;
  TU32 max_frame;
} SMaxFrameNumber;

typedef void (* TFInjecterCallback) (void *context, TTime mono_pts, TTime dsp_pts);

typedef struct {
  TU32 check_field;
  TGenericID component_id;
  TFInjecterCallback callback;
  void *callback_context;
} SVideoInjecterCallback;

typedef struct {
  TU32 check_field;
  TGenericID component_id;
  TU8 num_of_frame;
  TU8 reserved0;
  TU8 reserved1;
  TU8 reserved2;
} SVideoInjecterBatchInject;

typedef enum {

  //pre running, persist config
  EGenericEngineConfigure_RTSPServerPort = 0x0010,
  EGenericEngineConfigure_RTSPServerNonblock = 0x0011,
  EGenericEngineConfigure_VideoStreamID = 0x0012,
  EGenericEngineConfigure_ConfigVout = 0x0013,
  EGenericEngineConfigure_ConfigAudioDevice = 0x0014,
  EGenericEngineConfigure_EnableFastFWFastBWBackwardPlayback = 0x0015,
  EGenericEngineConfigure_DisableFastFWFastBWBackwardPlayback = 0x0016,
  EGenericEngineConfigure_EnableDumpBitstream = 0x0017,
  EGenericEngineConfigure_DumpBitstreamOnly = 0x0018,
  EGenericEngineConfigure_SetMaxFrameNumber = 0x0019,
  EGenericEngineConfigure_SourceBufferID = 0x001a,

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
  EGenericEngineConfigure_QueryLastDisplayTimestamp = 0x002b,

  //video capture
  EGenericEngineConfigure_VideoCapture_Params = 0x0100,
  EGenericEngineConfigure_VideoCapture_Snapshot = 0x0101,
  EGenericEngineConfigure_VideoCapture_periodic = 0x0102,

  //audio playback
  EGenericEngineConfigure_AudioPlayback_SelectAudioSource = 0x0200,
  EGenericEngineConfigure_AudioPlayback_Mute = 0x0201,
  EGenericEngineConfigure_AudioPlayback_UnMute = 0x0202,
  EGenericEngineConfigure_AudioPlayback_SetVolume = 0x0203,

  //audio capture
  EGenericEngineConfigure_AudioCapture_Mute = 0x0210,
  EGenericEngineConfigure_AudioCapture_UnMute = 0x0211,

  //streaming related
  EGenericEngineConfigure_ReConnectServer = 0x0220,
  EGenericEngineConfigure_RTSPClientTryTCPModeFirst = 0x0221,

  //video encoder
  EGenericEngineConfigure_VideoEncoder_Params = 0x0400,
  EGenericEngineConfigure_VideoEncoder_Bitrate = 0x0401,
  EGenericEngineConfigure_VideoEncoder_Framerate = 0x0402,
  EGenericEngineConfigure_VideoEncoder_DemandIDR = 0x0403,
  EGenericEngineConfigure_VideoEncoder_GOP = 0x0404,

  //record related, for pipeline
  EGenericEngineConfigure_Record_Saving_Strategy = 0x0450,
  EGenericEngineConfigure_Record_SetSpecifiedInformation = 0x0451,
  EGenericEngineConfigure_Record_GetSpecifiedInformation = 0x0452,
  EGenericEngineConfigure_Record_Suspend = 0x0453,
  EGenericEngineConfigure_Record_ResumeSuspend = 0x0454,

  //video injecter
  EGenericEngineConfigure_VideoInjecter_SetCallback = 0x0500,
  EGenericEngineConfigure_VideoInjecter_BatchInject = 0x0501,
} EGenericEngineConfigure;

typedef struct {
  TU32 check_field;
  TU32 port;
} SConfigRTSPServerPort;

typedef struct {
  TU32 check_field;
  TU32 stream_id;
  TGenericID component_id;
} SConfigVideoStreamID;

typedef struct {
  TU32 check_field;
  TU32 source_buffer_id;
  TGenericID component_id;
} SConfigSourceBufferID;

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
  unsigned int check_field;
  TGenericID module_id;

  TDimension cap_win_offset_x, cap_win_offset_y;
  TDimension cap_win_width, cap_win_height;
  TDimension screen_width, screen_height;
  TDimension cap_buffer_linesize;

  float framerate;
  unsigned int framerate_num;
  unsigned int framerate_den;

  EPixelFMT pixel_format;
  StreamFormat format;

  TU8 b_copy_data;
  TU8 buffer_id;
  TU8 b_snapshot_mode;
  TU8 snapshot_framenumber;
} SVideoCaptureParams;

typedef struct {
  TU32 check_field;
  TGenericID module_id;

  TU8 snapshot_framenumber;
  TU8 reserved0;
  TU8 reserved1;
  TU8 reserved2;
} SVideoCaptureSnapshotParams;

typedef struct {
  TU32 check_field;
  TGenericID module_id;

  float framerate;
  TU32 framerate_num;
  TU32 framerate_den;
} SVideoCapturePeriodicParams;

#define DMAX_PATHFILE_NANE_LENGTH 8192
#define DMAX_FILE_EXTERTION_NANE_LENGTH 32

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
  EGenericComponentType_VideoCapture = 0x0a,
  EGenericComponentType_AudioCapture = 0x0b,
  EGenericComponentType_VideoInjecter = 0x0c,
  EGenericComponentType_VideoRawSink = 0x0d,
  EGenericComponentType_VideoESSink = 0x0e,

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
  virtual EECode print_current_status(TGenericID id = 0, TU32 print_flag = 0) = 0;

public:
  virtual EECode begin_config_process_pipeline(TU32 customized_pipeline = 1) = 0;
  virtual EECode finalize_config_process_pipeline() = 0;

  virtual EECode new_component(TU32 component_type, TGenericID &component_id, const char *prefer_string = NULL) = 0;
  virtual EECode connect_component(TGenericID &connection_id, TGenericID upper_component_id, TGenericID down_component_id, StreamType pin_type) = 0;

  virtual EECode setup_playback_pipeline(TGenericID &playback_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID video_decoder_id, TGenericID audio_decoder_id, TGenericID video_renderer_id, TGenericID audio_renderer_id) = 0;
  virtual EECode setup_recording_pipeline(TGenericID &recording_pipeline_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID sink_id) = 0;
  virtual EECode setup_streaming_pipeline(TGenericID &streaming_pipeline_id, TGenericID streaming_transmiter_id, TGenericID streaming_server_id, TGenericID video_source_id, TGenericID audio_source_id, TGenericID video_connection_id, TGenericID audio_connection_id) = 0;

public:
  virtual EECode generic_control(EGenericEngineConfigure config_type, void *param) = 0;

  //set urls
  virtual EECode set_source_url(TGenericID source_component_id, char *url) = 0;
  virtual EECode set_sink_url(TGenericID sink_component_id, char *url) = 0;
  virtual EECode set_streaming_url(TGenericID pipeline_id, char *url) = 0;

  virtual EECode set_app_msg_callback(void (*MsgProc)(void *, SMSG &), void *context) = 0;

  virtual EECode run_processing() = 0;
  virtual EECode exit_processing() = 0;

  virtual EECode start() = 0;
  virtual EECode stop() = 0;

public:
  virtual void destroy() = 0;

protected:
  virtual ~IGenericEngineControl() {}
};

extern IGenericEngineControl *CreateGenericMediaEngine();

extern TU32 gfGetMaxEncodingStreamNumber();

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
  TU8 video_format;

  TU32 audio_sample_rate;
  TU32 audio_frame_size;
} SMediaInfo;

typedef struct {
  TU8 *p_buffer;
  TU32 buffer_size;
  TU32 data_size;
} SGenericDataBuffer;

typedef enum {
  EMediaProbeType_Invalid = 0x00,
  EMediaProbeType_MP4 = 0x01,
} EMediaProbeType;

typedef struct {
  SMediaInfo info;

  TU32 tot_video_frames;
  TU32 tot_audio_frames;

  TU8 *p_video_extradata;
  TU32 video_extradata_len;
} SMediaProbedInfo;

class IMediaProbe
{
public:
  virtual void Delete() = 0;

protected:
  virtual ~IMediaProbe() {}

public:
  virtual EECode Open(char *filename, SMediaProbedInfo *info) = 0;
  virtual EECode Close() = 0;

  virtual SGenericDataBuffer *GetKeyFrame(TU32 key_frame_index) = 0;
  virtual SGenericDataBuffer *GetFrame(TU32 frame_index) = 0;
  virtual void ReleaseFrame(SGenericDataBuffer *frame) = 0;
};

IMediaProbe *gfCreateMediaProbe(EMediaProbeType type);

typedef enum {
  EThumbPlayDisplayLayout_Invalid = 0x00,
  EThumbPlayDisplayLayout_Rectangle = 0x01,
} EThumbPlayDisplayLayout;

typedef enum {
  EDisplayDevice_Invalid = 0x00,
  EDisplayDevice_DIGITAL = 0x01,
  EDisplayDevice_HDMI = 0x02,
  EDisplayDevice_CVBS = 0x03,
} EDisplayDevice;

typedef struct {
  TDimension rows;
  TDimension columns;

  TDimension tot_width, tot_height;
  TDimension border_x, border_y;

  //derived parameters
  TDimension rect_width;
  TDimension rect_height;
} SThumbPlayDisplayRectangleLayoutParams;

typedef struct {
  TU8 *p[4];
  TU32 linesize[4];
  EPixelFMT fmt;
} SThumbPlayBufferDesc;

class IThumbPlayControl
{
public:
  virtual EECode SetMediaPath(char *url) = 0;
  virtual EECode SetDisplayDevice(EDisplayDevice device) = 0;
  virtual TU32 GetTotalFileNumber() = 0;
  virtual EECode SetDisplayLayout(EThumbPlayDisplayLayout layout, void *params) = 0;
  virtual EECode ExitDeviceMode() = 0;
  virtual EECode SetDisplayBuffer(SThumbPlayBufferDesc *buffer, TDimension buffer_width, TDimension buffer_height) = 0;
  virtual EECode DisplayThumb2Device(TU32 begin_index) = 0;
  virtual EECode DisplayThumb2Buffer(TU32 begin_index) = 0;

public:
  virtual void Destroy() = 0;

protected:
  virtual ~IThumbPlayControl() {}

};

IThumbPlayControl *CreateThumbPlay();

//dsp related
enum {
  EAMDSP_MODE_INVALID = 0x00,
  EAMDSP_MODE_INIT = 0x01,
  EAMDSP_MODE_IDLE = 0x02,
  EAMDSP_MODE_PREVIEW = 0x03,
  EAMDSP_MODE_ENCODE = 0x04,
  EAMDSP_MODE_DECODE = 0x05,
};

extern EECode gfAmbaDSPGetMode(TU32 *mode);
extern EECode gfAmbaDSPEnterIdle();

//misc functions
extern void gfPrintAvailableVideoOutputMode();
extern void gfPrintAvailableLCDModel();
extern const TChar *gfGetDSPPlatformName();

extern EECode gfAmbaDSPConfigureVout(SVideoOutputConfigure *config);

extern EECode gfAmbaDSPHaltCurrentVout();

#endif

