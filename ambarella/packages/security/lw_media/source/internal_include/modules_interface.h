/*
 * modules_interface.h
 *
 * History:
 *    2015/07/27 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __MODULES_INTERFACE_H__
#define __MODULES_INTERFACE_H__

typedef enum {
    EDemuxerModuleID_Invalid = 0,
    EDemuxerModuleID_Auto,
    EDemuxerModuleID_PrivateRTSPScheduled,
    EDemuxerModuleID_PrivateTS,
    EDemuxerModuleID_PrivateMP4,
} EDemuxerModuleID;

typedef enum {
    EMuxerModuleID_Invalid = 0,
    EMuxerModuleID_Auto,
    EMuxerModuleID_PrivateTS,
    EMuxerModuleID_PrivateMP4,
} EMuxerModuleID;

typedef enum {
    EVideoDecoderModuleID_Invalid = 0,
    EVideoDecoderModuleID_Auto,
    EVideoDecoderModuleID_AMBADSP,
    EVideoDecoderModuleID_FFMpeg,
} EVideoDecoderModuleID;

typedef enum {
    EAudioDecoderModuleID_Invalid = 0,
    EAudioDecoderModuleID_Auto,
    EAudioDecoderModuleID_AAC,
} EAudioDecoderModuleID;

typedef enum {
    EVideoRendererModuleID_Invalid = 0,
    EVideoRendererModuleID_Auto,
    EVideoRendererModuleID_AMBADSP,
    EVideoRendererModuleID_LinuxFrameBuffer,
    EVideoRendererModuleID_OpenGL,
    EVideoRendererModuleID_DirectDraw,
    EVideoRendererModuleID_AndroidSurface,
} EVideoRendererModuleID;

typedef enum {
    EAudioRendererModuleID_Invalid = 0,
    EAudioRendererModuleID_Auto,
    EAudioRendererModuleID_ALSA,
} EAudioRendererModuleID;

typedef enum {
    EVideoEncoderModuleID_Invalid = 0,
    EVideoEncoderModuleID_Auto,
    EVideoEncoderModuleID_AMBADSP,
} EVideoEncoderModuleID;

typedef enum {
    EAudioEncoderModuleID_Invalid = 0,
    EAudioEncoderModuleID_Auto,
    EAudioEncoderModuleID_PrivateLibAAC,
} EAudioEncoderModuleID;

typedef enum {
    EVideoInputModuleID_Invalid = 0,
    EVideoInputModuleID_Auto,
    EVideoInputModuleID_AMBADSP,
} EVideoInputModuleID;

typedef enum {
    EAudioInputModuleID_Invalid = 0,
    EAudioInputModuleID_Auto,
    EAudioInputModuleID_ALSA,
} EAudioInputModuleID;

typedef enum {
    EStreamingTransmiterModuleID_Invalid = 0,
    EStreamingTransmiterModuleID_Auto,

    EStreamingTransmiterModuleID_RTPRTCP_Video,
    EStreamingTransmiterModuleID_RTPRTCP_Audio,
} EStreamingTransmiterModuleID;

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

//-----------------------------------------------------------------------
//
// IDemuxer
//
//-----------------------------------------------------------------------

class IDemuxer
{
public:
    virtual CObject *GetObject0() const = 0;

public:
    virtual EECode SetupOutput(COutputPin *p_output_pins[], CBufferPool *p_bufferpools[], IMemPool *p_mempools[], IMsgSink *p_msg_sink) = 0;

    virtual EECode SetupContext(TChar *url, void *p_agent = NULL, TU8 priority = 0, TU32 request_receive_buffer_size = 0, TU32 request_send_buffer_size = 0) = 0;
    virtual EECode DestroyContext() = 0;
    virtual EECode ReconnectServer() = 0;

    virtual EECode Seek(TTime &target_time, ENavigationPosition position = ENavigationPosition_Invalid) = 0;
    virtual EECode Start() = 0;
    virtual EECode Stop() = 0;

    virtual EECode Suspend() = 0;//obsolete later
    virtual EECode Pause() = 0;
    virtual EECode Resume() = 0;
    virtual EECode Flush() = 0;
    virtual EECode ResumeFlush() = 0;
    virtual EECode Purge() = 0;

    virtual EECode SetPbRule(TU8 direction, TU8 feeding_rule, TU16 speed) = 0;
    virtual EECode SetPbLoopMode(TU32 *p_loop_mode) = 0;

    virtual EECode EnableVideo(TU32 enable) = 0;
    virtual EECode EnableAudio(TU32 enable) = 0;

public:
    virtual EECode SetVideoPostProcessingCallback(void *callback_context, void *callback) = 0;
    virtual EECode SetAudioPostProcessingCallback(void *callback_context, void *callback) = 0;

public:
    virtual EECode QueryContentInfo(const SStreamCodecInfos *&pinfos) const = 0;

public:
    virtual EECode UpdateContext(SContextInfo *pContext) = 0;
    virtual EECode GetExtraData(SStreamingSessionContent *pContent) = 0;

public:
    virtual EECode NavigationSeek(SContextInfo *info) = 0;
    virtual EECode ResumeChannel() = 0;
};

//-----------------------------------------------------------------------
//
// IVideoDecoder
//
//-----------------------------------------------------------------------
typedef struct {
    TUint index;

    TUint prefer_dsp_mode;
    TUint cap_max_width, cap_max_height;

    TU32 cuustomized_content_format;
    StreamFormat format;
    //EDSPPlatform platform;
} SVideoDecoderInputParam;

typedef enum {
    EDecoderMode_Invalid = 0x00,
    EDecoderMode_Normal = 0x01,
    EDecoderMode_Direct = 0x02,
} EDecoderMode;

class IVideoDecoder
{
public:
    virtual CObject *GetObject0() const = 0;

public:
    virtual EECode SetupContext(SVideoDecoderInputParam *param) = 0;
    virtual EECode DestroyContext() = 0;

    virtual EECode SetBufferPool(IBufferPool *buffer_pool) = 0;

    virtual EECode Start() = 0;
    virtual EECode Stop() = 0;

    virtual EECode Decode(CIBuffer *in_buffer, CIBuffer *&out_buffer) = 0;
    virtual EECode Flush() = 0;

    virtual EECode Suspend() = 0;

    virtual EECode SetExtraData(TU8 *p, TMemSize size) = 0;

    virtual EECode SetPbRule(TU8 direction, TU8 feeding_rule, TU16 speed) = 0;

    virtual EECode SetFrameRate(TUint framerate_num, TUint frameate_den) = 0;

    virtual EDecoderMode GetDecoderMode() const = 0;

    //direct mode
    virtual EECode DecodeDirect(CIBuffer *in_buffer) = 0;
    virtual EECode SetBufferPoolDirect(IOutputPin *p_output_pin, IBufferPool *p_bufferpool) = 0;

};

//-----------------------------------------------------------------------
//
// IAudioDecoder
//
//-----------------------------------------------------------------------
class IAudioDecoder
{
public:
    virtual void Destroy() = 0;

public:
    virtual EECode SetupContext(SAudioParams *param) = 0;
    virtual void DestroyContext() = 0;

    virtual EECode Start() = 0;
    virtual EECode Stop() = 0;

    virtual EECode Decode(CIBuffer *in_buffer, CIBuffer *out_buffer, TInt &consumed_bytes) = 0;
    virtual EECode Flush() = 0;

    virtual EECode Suspend() = 0;

    virtual EECode SetExtraData(TU8 *p, TUint size) = 0;
};

//-----------------------------------------------------------------------
//
// IVideoRenderer
//
//-----------------------------------------------------------------------
class IVideoRenderer
{
public:
    virtual void Destroy() = 0;

public:
    virtual EECode SetupContext(SVideoParams *param = NULL) = 0;
    virtual EECode DestroyContext() = 0;

    virtual EECode Start(TUint index = 0) = 0;
    virtual EECode Stop(TUint index = 0) = 0;
    virtual EECode Flush(TUint index = 0) = 0;

    virtual EECode Render(CIBuffer *p_buffer, TUint index = 0) = 0;

    virtual EECode Pause(TUint index = 0) = 0;
    virtual EECode Resume(TUint index = 0) = 0;
    virtual EECode StepPlay(TUint index = 0) = 0;

    virtual EECode SyncTo(TTime pts, TUint index = 0) = 0;
    virtual EECode QueryLastShownTimeStamp(TTime &pts, TUint index = 0) = 0;
};

//-----------------------------------------------------------------------
//
// IAudioRenderer
//
//-----------------------------------------------------------------------
class IAudioRenderer
{
public:
    virtual void Destroy() = 0;

public:
    virtual EECode SetupContext(SAudioParams *param) = 0;
    virtual EECode DestroyContext() = 0;

    virtual EECode Start(TUint index = 0) = 0;
    virtual EECode Stop(TUint index = 0) = 0;
    virtual EECode Flush(TUint index = 0) = 0;

    virtual EECode Render(CIBuffer *p_buffer, TUint index = 0) = 0;

    virtual EECode Pause(TUint index = 0) = 0;
    virtual EECode Resume(TUint index = 0) = 0;

    virtual EECode SyncTo(TTime pts, TUint index = 0) = 0;
};

//-----------------------------------------------------------------------
//
// IMuxer
//
//-----------------------------------------------------------------------
typedef struct {
    TTime pts, dts;
    TTime native_pts, native_dts;

    TTime duration;
    TTime av_normalized_duration;

    TU8 is_key_frame;
    TU8 stream_index;
    TU8 reserved0, reserved1;
} SMuxerDataTrunkInfo;

typedef struct {
    TTime file_duration;
    TU64 file_size;
    TU32 file_bitrate;
} SMuxerFileInfo;

class IMuxer
{
public:
    virtual void Destroy() = 0;

public:
    virtual EECode SetupContext() = 0;
    virtual EECode DestroyContext() = 0;

    virtual EECode SetSpecifiedInfo(SRecordSpecifiedInfo *info) = 0;
    virtual EECode GetSpecifiedInfo(SRecordSpecifiedInfo *info) = 0;

    virtual EECode SetExtraData(StreamType type, TUint stream_index, void *p_data, TUint data_size) = 0;
    virtual EECode SetPrivateDataDurationSeconds(void *p_data, TUint data_size) = 0;
    virtual EECode SetPrivateDataChannelName(void *p_data, TUint data_size) = 0;

    virtual EECode InitializeFile(const SStreamCodecInfos *infos, TChar *url, ContainerType type = ContainerType_AUTO, TChar *thumbnailname = NULL, TTime start_pts = 0, TTime start_dts = 0) = 0;
    virtual EECode FinalizeFile(SMuxerFileInfo *p_file_info) = 0;

    virtual EECode WriteVideoBuffer(CIBuffer *p_buffer, SMuxerDataTrunkInfo *info) = 0;
    virtual EECode WriteAudioBuffer(CIBuffer *p_buffer, SMuxerDataTrunkInfo *info) = 0;
    virtual EECode WritePridataBuffer(CIBuffer *p_buffer, SMuxerDataTrunkInfo *info) = 0;

    virtual EECode SetState(TU8 b_suspend) = 0;
};

//-----------------------------------------------------------------------
//
// IStreamingTransmiter
//
//-----------------------------------------------------------------------
struct SSubSessionInfo;
class IStreamingTransmiter
{
public:
    virtual void Destroy() = 0;

public:
    virtual EECode SetupContext(StreamTransportType type, ProtocolType protocol_type, const SStreamCodecInfos *infos, TTime start_pts = 0) = 0;
    virtual EECode DestroyContext() = 0;

    virtual EECode UpdateStreamFormat(StreamType type, StreamFormat format) = 0;

    virtual EECode Start() = 0;
    virtual EECode Stop() = 0;

    virtual EECode SetExtraData(TU8 *pdata, TMemSize size) = 0;
    virtual EECode SendData(CIBuffer *pBuffer) = 0;

    virtual EECode AddSubSession(SSubSessionInfo *p_sub_session) = 0;
    virtual EECode RemoveSubSession(SSubSessionInfo *p_sub_session) = 0;
    virtual EECode SetSrcPort(TSocketPort port, TSocketPort port_ext) = 0;
    virtual EECode GetSrcPort(TSocketPort &port, TSocketPort &port_ext) = 0;
};

//-----------------------------------------------------------------------
//
// IVideoEncoder
//
//-----------------------------------------------------------------------
typedef struct {
    TU32 index;

    TU32 prefer_dsp_mode;
    TU32 cap_max_width, cap_max_height;

    TU32 bitrate;
    TU32 framerate_num, framerate_den;

    float framerate;
    TU32 framerate_reduce_factor;

    StreamFormat format;
    //EDSPPlatform platform;
} SVideoEncoderInputParam;

class IVideoEncoder
{
public:
    virtual void Destroy() = 0;

public:
    virtual EECode SetupContext(SVideoEncoderInputParam *param) = 0;
    virtual void DestroyContext() = 0;

    virtual EECode SetBufferPool(IBufferPool *p_buffer_pool) = 0;

    virtual EECode Start() = 0;
    virtual EECode Stop() = 0;

    virtual EECode Encode(CIBuffer *in_buffer, CIBuffer *out_buffer, TU32 &current_remaining, TU32 &all_cached_frames) = 0;

    virtual EECode VideoEncoderProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param) = 0;
};

//-----------------------------------------------------------------------
//
// IAudioEncoder
//
//-----------------------------------------------------------------------
class IAudioEncoder
{
public:
    virtual void Destroy() = 0;

public:
    virtual EECode SetupContext(SAudioParams *format) = 0;
    virtual void DestroyContext() = 0;

    virtual EECode Encode(TU8 *p_in, TU8 *&p_out, TU32 &output_size) = 0;

    virtual EECode GetExtraData(TU8 *&p, TU32 &size) = 0;
};

//-----------------------------------------------------------------------
//
// IAudioInput
//
//-----------------------------------------------------------------------
class IAudioInput
{
public:
    virtual void Destroy() = 0;

public:
    virtual EECode SetupContext(SAudioParams *param) = 0;
    virtual EECode DestroyContext() = 0;
    virtual EECode SetupOutput(COutputPin *p_output_pin, CBufferPool *p_bufferpool, IMsgSink *p_msg_sink) = 0;

    virtual TUint GetChunkSize() = 0;
    virtual TUint GetBitsPerFrame() = 0;

    virtual EECode Start(TUint index = 0) = 0;
    virtual EECode Stop(TUint index = 0) = 0;
    virtual EECode Flush(TUint index = 0) = 0;

    virtual EECode Read(TU8 *pData, TUint dataSize, TUint *pNumFrames, TU64 *pTimeStamp, TUint index = 0) = 0;

    virtual EECode Pause(TUint index = 0) = 0;
    virtual EECode Resume(TUint index = 0) = 0;

    virtual EECode Mute() = 0;
    virtual EECode UnMute() = 0;
};

//-----------------------------------------------------------------------
//
// IStreamingServer
//
//-----------------------------------------------------------------------
struct SClientSessionInfo;
class IStreamingServer
{
public:
    virtual CObject *GetObject0() const = 0;

public:
    virtual EECode HandleServerRequest(TSocketHandler &max, void *all_set) = 0;
    virtual void ScanClientList(TInt &nready, void *read_set, void *all_set) = 0;

    virtual EECode AddStreamingContent(SStreamingSessionContent *content) = 0;
    virtual EECode RemoveStreamingContent(SStreamingSessionContent *content) = 0;

    virtual EECode RemoveStreamingClientSession(SClientSessionInfo *session) = 0;
    virtual EECode ClearStreamingClients(SStreamingSessionContent *related_content) = 0;

    virtual EECode GetHandler(TInt &handle) const = 0;

    virtual EECode GetServerState(EServerState &state) const = 0;
    virtual EECode SetServerState(EServerState state) = 0;

    virtual EECode GetServerID(TGenericID &id, TComponentIndex &index, TComponentType &type) const = 0;
    virtual EECode SetServerID(TGenericID id, TComponentIndex index, TComponentType type) = 0;

};

typedef struct {
    IStreamingTransmiter *p_transmiter;
    TU32 input_index;
    SClientSessionInfo *p_session;
    SSubSessionInfo *p_sub_session;
} SStreamingTransmiterCombo;

extern IDemuxer *gfModuleFactoryDemuxer(EDemuxerModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU32 index);
extern IVideoDecoder *gfModuleFactoryVideoDecoder(EVideoDecoderModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
extern IAudioDecoder *gfModuleFactoryAudioDecoder(EAudioDecoderModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
extern IVideoRenderer *gfModuleFactoryVideoRenderer(EVideoRendererModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
extern IAudioRenderer *gfModuleFactoryAudioRenderer(EAudioRendererModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
extern IVideoEncoder *gfModuleFactoryVideoEncoder(EVideoEncoderModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
extern IAudioEncoder *gfModuleFactoryAudioEncoder(EAudioEncoderModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
extern IAudioInput *gfModuleFactoryAudioInput(EAudioInputModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, void *pDev);
extern IMuxer *gfModuleFactoryMuxer(EMuxerModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
extern IStreamingTransmiter *gfModuleFactoryStreamingTransmiter(EStreamingTransmiterModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TComponentIndex index);
extern IStreamingServer *gfModuleFactoryStreamingServer(StreamingServerType type, StreamingServerMode mode, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU16 server_port, TU8 enable_video, TU8 enable_audio);

#endif

