/*******************************************************************************
 * framework_interfaces.h
 *
 * History:
 *  2012/12/07 - [Zhi He] create file
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

#ifndef __FRAMEWORK_INTERFACES_H__
#define __FRAMEWORK_INTERFACES_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

#define DMAX_CONNECTIONS_OUTPUTPIN 32

typedef struct {
    TPointer p_pointer;
    TComponentIndex index;
    StreamType type;
    StreamFormat format;

    TPointer p_agent_pointer;
} SQueryInterface;

typedef enum {
    EBufferType_Invalid = 0,
    EBufferType_VideoES,
    EBufferType_VideoExtraData,
    EBufferType_AudioES,
    EBufferType_AudioExtraData,
    EBufferType_VideoFrameBuffer,
    EBufferType_AudioPCMBuffer,
    EBufferType_AudioFrameBuffer,
    EBufferType_PrivData,

    EBufferType_FlowControl_EOS,
    EBufferType_FlowControl_Pause,
    EBufferType_FlowControl_Resume,
    EBufferType_FlowControl_FinalizeFile,
    EBufferType_FlowControl_PassParameters,

} EBufferType;

typedef enum {
    EBufferCustomizedContentType_Invalid = 0,
    EBufferCustomizedContentType_RingBuffer,
    EBufferCustomizedContentType_FFMpegAVPacket,
    EBufferCustomizedContentType_AllocateByFilter,
} EBufferCustomizedContentType;

typedef enum {
    //common
    EFilterPropertyType_index = 0x0000,
    EFilterPropertyType_current_state,
    EFilterPropertyType_use_preset_mode,
    EFilterPropertyType_update_source_url,
    EFilterPropertyType_update_sink_url,
    EFilterPropertyType_update_streaming_url,
    EFilterPropertyType_update_cloud_tag,
    EFilterPropertyType_update_cloud_remote_url,
    EFilterPropertyType_update_cloud_remote_port,
    EFilterPropertyType_playback_loop_mode,
    EFilterPropertyType_process_buffer,

    //filter control
    EFilterPropertyType_suspend = 0x0010,
    EFilterPropertyType_enable_input_pin,
    EFilterPropertyType_enable_output_pin,
    EFilterPropertyType_purge_input_pin,

    EFilterPropertyType_purge_channel,
    EFilterPropertyType_resume_channel,

    //parameters query/set
    EFilterPropertyType_video_parameters = 0x0020,
    EFilterPropertyType_video_size,
    EFilterPropertyType_video_framerate,
    EFilterPropertyType_video_format,
    EFilterPropertyType_video_bitrate,
    EFilterPropertyType_video_demandIDR,
    EFilterPropertyType_video_gop,
    EFilterPropertyType_audio_parameters,
    EFilterPropertyType_audio_channelnumber,
    EFilterPropertyType_audio_samplerate,
    EFilterPropertyType_audio_format,
    EFilterPropertyType_audio_bitrate,

    //for external module
    EFilterPropertyType_assign_external_module = 0x0030,
    EFilterPropertyType_get_external_handler,

    EFilterPropertyType_set_pre_processing_callback = 0x0040,
    EFilterPropertyType_set_post_processing_callback = 0x0041,

    //demuxer
    EFilterPropertyType_demuxer_enable_video = 0x0100,
    EFilterPropertyType_demuxer_enable_audio,
    EFilterPropertyType_demuxer_enable_subtitle,
    EFilterPropertyType_demuxer_disable_video,
    EFilterPropertyType_demuxer_disable_audio,
    EFilterPropertyType_demuxer_disable_subtitle,
    EFilterPropertyType_demuxer_pb_speed_feedingrule,
    EFilterPropertyType_demuxer_seek,
    EFilterPropertyType_demuxer_priority,
    EFilterPropertyType_demuxer_receive_buffer_size,
    EFilterPropertyType_demuxer_send_buffer_size,
    EFilterPropertyType_demuxer_reconnect_server,
    EFilterPropertyType_demuxer_update_source_url,
    EFilterPropertyType_demuxer_getextradata,
    EFilterPropertyType_demuxer_seek_to_end,
    //vod related
    EFilterPropertyType_demuxer_navigation_seek,

    //cloud connecter
    EFilterPropertyType_assign_cloud_agent = 0x0110,
    EFilterPropertyType_remove_cloud_agent,
    EFilterPropertyType_relay_command,
    EFilterPropertyType_enable_relay_command,
    EFilterPropertyType_set_relay_target,

    //audio capture
    EFilterPropertyType_audio_capture_mute = 0x0120,
    EFilterPropertyType_audio_capture_unmute = 0x0121,

    //video decoder
    EFilterPropertyType_vdecoder_pb_speed_feedingrule = 0x0200,
    EFilterPropertyType_vdecoder_zoom,

    //audio decoder
    EFilterPropertyType_adecoder_todo = 0x0300,

    //video post processor
    EFilterPropertyType_vpostp_configure = 0x400,
    EFilterPropertyType_vpostp_global_setting,
    EFilterPropertyType_vpostp_initial_configure,
    EFilterPropertyType_vpostp_display_rect,
    EFilterPropertyType_vpostp_set_window,
    EFilterPropertyType_vpostp_set_render,
    EFilterPropertyType_vpostp_set_dec_source,
    EFilterPropertyType_vpostp_stream_switch,
    EFilterPropertyType_vpostp_update_display_mask,
    EFilterPropertyType_vpostp_update_display_layout,
    EFilterPropertyType_vpostp_update_display_layer,
    EFilterPropertyType_vpostp_update_display_focus,
    EFilterPropertyType_vpostp_swap_window_content,
    EFilterPropertyType_vpostp_window_circular_shift,
    EFilterPropertyType_vpostp_update_display_highlightenwindowsize,
    EFilterPropertyType_vpostp_switch_window_to_HD,

    EFilterPropertyType_vpostp_playback_capture,

    EFilterPropertyType_vpostp_playback_zoom,

    EFilterPropertyType_vpostp_query_window,
    EFilterPropertyType_vpostp_query_render,
    EFilterPropertyType_vpostp_query_dsp_decoder,
    EFilterPropertyType_vpostp_query_display,

    //audio post processor
    EFilterPropertyType_apostp_target_audio_channel_number = 0x0500,
    EFilterPropertyType_apostp_target_audio_samplerate,
    EFilterPropertyType_apostp_target_audio_sampleformat,
    EFilterPropertyType_apostp_target_audio_bitrate,

    //video renderer
    EFilterPropertyType_vrenderer_sync_to_audio = 0x600,
    EFilterPropertyType_vrenderer_align_all_video_streams,
    EFilterPropertyType_vrenderer_wake_vout,
    EFilterPropertyType_vrenderer_trickplay_pause,
    EFilterPropertyType_vrenderer_trickplay_resume,
    EFilterPropertyType_vrenderer_trickplay_step,
    EFilterPropertyType_vrenderer_get_last_shown_timestamp,

    //audio renderer
    EFilterPropertyType_arenderer_sync_to_video = 0x0700,
    EFilterPropertyType_arenderer_mute,
    EFilterPropertyType_arenderer_umute,
    EFilterPropertyType_arenderer_amplify,
    EFilterPropertyType_arenderer_pause,
    EFilterPropertyType_arenderer_resume,
    EFilterPropertyType_arenderer_get_last_shown_timestamp,

    //muxer
    EFilterPropertyType_muxer_stop = 0x0800,
    EFilterPropertyType_muxer_start,
    EFilterPropertyType_muxer_finalize_file,
    EFilterPropertyType_muxer_saving_strategy,
    EFilterPropertyType_muxer_set_channel_name,
    EFilterPropertyType_muxer_set_specified_info,
    EFilterPropertyType_muxer_get_specified_info,
    EFilterPropertyType_muxer_suspend,
    EFilterPropertyType_muxer_resume_suspend,

    //streaming
    EFilterPropertyType_streaming_add_subsession = 0x0900,
    EFilterPropertyType_streaming_remove_subsession = 0x0901,
    EFilterPropertyType_streaming_query_subsession_sink = 0x0902,
    EFilterPropertyType_streaming_set_content = 0x0903,
    EFilterPropertyType_streaming_set_content_video = 0x0904,
    EFilterPropertyType_streaming_set_content_audio = 0x0905,

    //connecter
    EFilterPropertyType_pinmuxer_set_type = 0x0a00,
    EFilterPropertyType_flow_controller_start = 0x0a01,
    EFilterPropertyType_flow_controller_stop = 0x0a02,
    EFilterPropertyType_flow_controller_update_speed = 0x0a03,

    //video encoder
    EFilterPropertyType_refresh_sequence = 0x0b00,

} EFilterPropertyType;

class IBufferPool;

//-----------------------------------------------------------------------
//
// CIBuffer
//
//-----------------------------------------------------------------------

class CIBuffer
{
public:
    enum {
        SYNC_POINT = (1 << 0),
        KEY_FRAME = (1 << 1),
        BROKEN_FRAME = (1 << 2),
        WITH_EXTRA_DATA = (1 << 3),
        NEW_SEQUENCE = (1 << 4),
        DUP_FRAME = (1 << 5),
        MEMORY_BLOCK_NUMBER = 8,
        RTP_AAC = 0x1000,
    };

public:
    TAtomic mRefCount;
    CIBuffer *mpNext;
    EBufferType     mBufferType;
    TU8 mVideoFrameType, mbAudioPCMChannelInterlave, mAudioChannelNumber, mAudioSampleFormat;
    TU8 mbAllocated, mbExtEdge, mContentFormat, mReserved0;
    TU32   mFlags;
    TU32   mCustomizedContentFormat;

    //for customized field
    EBufferCustomizedContentType mCustomizedContentType;
    void *mpCustomizedContent;

    //memory
    TU8    *mpData[MEMORY_BLOCK_NUMBER];
    TU8    *mpMemoryBase[MEMORY_BLOCK_NUMBER];
    TMemSize    mMemorySize[MEMORY_BLOCK_NUMBER];
    TMemSize    mDataSize[MEMORY_BLOCK_NUMBER];
    TU32    mDataLineSize[MEMORY_BLOCK_NUMBER];

    //hint
    TU32 mSubPacketNumber;
    TIOSize mOffsetHint[DMAX_MEDIA_SUB_PACKET_NUMBER];

    IBufferPool *mpPool;

    TU32 mAudioBitrate;
    TU32 mVideoBitrate;

    //us
    volatile TTime mPTS;
    volatile TTime mDTS;
    // 1/DDefaultTimeScale
    volatile TTime mNativePTS;
    volatile TTime mNativeDTS;
    volatile TTime mLinearPTS;
    volatile TTime mLinearDTS;

#ifdef DCONFIG_TEST_END2END_DELAY
    //for debug
    TTime  mDebugTime;
    TU64  mDebugCount;
#endif

    //video related
    TU32 mVideoWidth, mVideoHeight;
    TU32 mExtVideoWidth, mExtVideoHeight;
    TU32 mVideoFrameRateNum, mVideoFrameRateDen;
    TU32 mVideoBufferLinesize[MEMORY_BLOCK_NUMBER];
    TU32 mVideoOffsetX, mVideoOffsetY;
    TU32 mVideoSampleAspectRatioNum;
    TU32 mVideoSampleAspectRatioDen;
    TU32 mVideoProfileIndicator;
    TU32 mVideoLevelIndicator;

    //audio related
    TU32 mAudioSampleRate;
    TU32 mAudioFrameSize;

    TU16 mSessionNumber;
    TU16 mReserved5;

    float mVideoRoughFrameRate;

public:
    TU32 mBufferSeqNumber0;
    TU32 mBufferSeqNumber1;

    TTime  mExpireTime;//life time, for recording
    TU16  mBufferId;  //
    TU16  mEncId; //

public:
    CIBuffer();
    virtual ~CIBuffer();

public:
    void AddRef();
    void Release();

    void ReleaseAllocatedMemory();

public:
    EBufferType GetBufferType() const;
    void SetBufferType(EBufferType type);

    TUint GetBufferFlags() const;
    void SetBufferFlags(TUint flags);
    void SetBufferFlagBits(TUint flags);
    void ClearBufferFlagBits(TUint flags);

    TU8 *GetDataPtr(TUint index = 0) const;
    void SetDataPtr(TU8 *pdata, TUint index = 0);

    TMemSize GetDataSize(TUint index = 0) const;
    void SetDataSize(TMemSize size, TUint index = 0);

    TU32 GetDataLineSize(TUint index = 0) const;
    void SetDataLineSize(TU32 size, TUint index = 0);

    TU8 *GetDataPtrBase(TUint index = 0) const;
    void SetDataPtrBase(TU8 *pdata, TUint index = 0);

    TMemSize GetDataMemorySize(TUint index = 0) const;
    void SetDataMemorySize(TMemSize size, TUint index = 0);

    TTime GetBufferPTS() const;
    void SetBufferPTS(TTime pts);

    TTime GetBufferDTS() const;
    void SetBufferDTS(TTime dts);

    //with native time unit, eg 1/DDefaultTimeScale
    TTime GetBufferNativePTS() const;
    void SetBufferNativePTS(TTime pts);

    TTime GetBufferNativeDTS() const;
    void SetBufferNativeDTS(TTime dts);

    //with media stream's time unit, eg, 1/samplerate for audio
    TTime GetBufferLinearPTS() const;
    void SetBufferLinearPTS(TTime pts);

    TTime GetBufferLinearDTS() const;
    void SetBufferLinearDTS(TTime dts);

};

typedef struct {
    void *pin_owner;
    CIBuffer *p_buffer;
} SProcessBuffer;

typedef struct {
    TU32 param0;
} SFilterGenericPropertyParams;

//-----------------------------------------------------------------------
//
// IBufferPool
//
//-----------------------------------------------------------------------
typedef EECode(*TBufferPoolReleaseBufferCallBack)(CIBuffer *p_buffer);

class IBufferPool
{
public:
    virtual const char *GetName() const = 0;

    virtual void Enable(bool bEnabled = true) = 0;
    virtual bool AllocBuffer(CIBuffer *&pBuffer, TUint size) = 0;
    virtual TUint GetFreeBufferCnt() const = 0;
    virtual void AddBufferNotifyListener(IEventListener *p_listener) = 0;

    virtual void AddRef(CIBuffer *pBuffer) = 0;
    virtual void Release(CIBuffer *pBuffer) = 0;

    virtual void SetReleaseBufferCallBack(TBufferPoolReleaseBufferCallBack callback) = 0;
};

//-----------------------------------------------------------------------
//
// IInputPin
//
//-----------------------------------------------------------------------

class IOutputPin;
class IFilter;

class IInputPin
{
public:
    virtual EECode Connect(IOutputPin *pPeer, TComponentIndex index) = 0;
    virtual EECode Disconnect(TComponentIndex index) = 0;

    virtual void Receive(CIBuffer *pBuffer) = 0;
    virtual void Purge(TU8 disable_pin = 0) = 0;

    virtual IFilter *GetFilter() const = 0;
    virtual IOutputPin *GetPeer() const = 0;

    virtual StreamType GetPinType() const = 0;
    virtual EECode GetVideoParams(StreamFormat &format, TU32 &video_width, TU32 &video_height, TU8 *&p_extra_data, TMemSize &extra_data_size, TU32 &session_number) const = 0;
    virtual EECode GetAudioParams(StreamFormat &format, TU32 &channel_number, TU32 &sample_frequency, AudioSampleFMT &fmt, TU8 *&p_extra_data, TMemSize &extra_data_size, TU32 &session_number) const = 0;
};

//-----------------------------------------------------------------------
//
// IOutputPin
//
//-----------------------------------------------------------------------
class IOutputPin
{
public:
    virtual EECode Connect(IInputPin *pPeer, TUint index) = 0;
    virtual EECode Disconnect(TUint index) = 0;

    //virtual void Enable(TUint index, TU8 bEnable) = 0;
    virtual void SendBuffer(CIBuffer *pBuffer) = 0;

    virtual IInputPin *GetPeer(TUint index) = 0;
    virtual IFilter *GetFilter() = 0;

    virtual TUint NumberOfPins() = 0;
    virtual EECode AddSubPin(TUint &sub_index) = 0;

    virtual IBufferPool *GetBufferPool() const = 0;
    virtual void SetBufferPool(IBufferPool *pBufferPool) = 0;
};

//-----------------------------------------------------------------------
//
// IFilter
//
//-----------------------------------------------------------------------

class IFilter
{
public:
    struct INFO {
        TUint   nInput;
        TUint   nOutput;
        TUint   mPriority;
        TUint   mFlags;
        const TChar *pName;
    };

    enum {
        EReleaseFlag_None = 0,
        EReleaseFlag_Stop,
        EReleaseFlag_Destroy,
    };

public:
    virtual EECode Initialize(TChar *p_param = NULL) = 0;
    virtual EECode Finalize() = 0;

    virtual EECode Run() = 0;
    virtual EECode Exit() = 0;

    virtual EECode Start() = 0;
    virtual EECode Stop() = 0;

    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual void Flush() = 0;
    virtual void ResumeFlush() = 0;

    virtual EECode Suspend(TUint release_flag = EReleaseFlag_None) = 0;
    virtual EECode ResumeSuspend(TComponentIndex input_index = 0) = 0;

    virtual EECode SwitchInput(TComponentIndex focus_input_index = 0) = 0;

    virtual EECode FlowControl(EBufferType flowcontrol_type) = 0;

    virtual EECode SendCmd(TUint cmd) = 0;
    virtual void PostMsg(TUint cmd) = 0;

    virtual EECode AddOutputPin(TUint &index, TUint &sub_index, TUint type) = 0;
    virtual EECode AddInputPin(TUint &index, TUint type) = 0;

    virtual IOutputPin *GetOutputPin(TUint index, TUint sub_index = 0) = 0;
    virtual IInputPin *GetInputPin(TUint index) = 0;

    virtual EECode FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param) = 0;

    virtual void GetInfo(INFO &info) = 0;

public:
    virtual CObject *GetObject0() const = 0;
};

//-----------------------------------------------------------------------
//
//  CBufferPool
//
//-----------------------------------------------------------------------
class CBufferPool: public CObject, public IBufferPool
{
    typedef CObject inherited;

public:
    static CBufferPool *Create(const TChar *name, TUint max_count);
    virtual void Delete();

public:
    virtual void PrintStatus();

protected:
    CBufferPool(const char *pName);
    EECode Construct(TUint nMaxBuffers);
    virtual ~CBufferPool();

public:
    // IBufferPool
    virtual const char *GetName() const;

    virtual void Enable(bool bEnabled = true);
    virtual bool AllocBuffer(CIBuffer *&pBuffer, TUint size);
    virtual TUint GetFreeBufferCnt() const;
    virtual void AddBufferNotifyListener(IEventListener *p_listener);

    virtual void AddRef(CIBuffer *pBuffer);
    virtual void Release(CIBuffer *pBuffer);

    virtual void SetReleaseBufferCallBack(TBufferPoolReleaseBufferCallBack callback);

protected:
    virtual void OnReleaseBuffer(CIBuffer *pBuffer);

private:
    void destroyBuffers();

protected:
    CIQueue *mpBufferQ;
    TAtomic mRefCount;
    const TChar *mpName;
    IEventListener *mpEventListener;
    TBufferPoolReleaseBufferCallBack mpReleaseBufferCallBack;

protected:
    TU8 *mpBufferMemory;
    TU32 mnTotalBufferNumber;
};

#if 0
//-----------------------------------------------------------------------
//
//  CSimpleBufferPool
//
//-----------------------------------------------------------------------
class CSimpleBufferPool: public CBufferPool
{
    typedef CBufferPool inherited;

public:
    static CSimpleBufferPool *Create(const char *pName, TUint count, TUint objectSize = sizeof(CIBuffer));

protected:
    CSimpleBufferPool(const char *pName);
    EECode Construct(TUint count, TUint objectSize = sizeof(CIBuffer));
    virtual ~CSimpleBufferPool();

private:
    TU8 *mpBufferMemory;
};

//-----------------------------------------------------------------------
//
// CFixedBufferPool
//
//-----------------------------------------------------------------------
class CFixedBufferPool: public CBufferPool
{
    typedef CBufferPool inherited;

public:
    static CFixedBufferPool *Create(TUint size, TUint count);
    virtual void Delete();

protected:
    CFixedBufferPool(): inherited("CFixedBufferPool"), _pBuffers(NULL), _pMemory(NULL) {}
    EECode Construct(TUint size, TUint count);
    virtual ~CFixedBufferPool();

private:
    CIBuffer *_pBuffers;
    TU8 *_pMemory;
};
#endif

//-----------------------------------------------------------------------
//
//  CFilter
//
//-----------------------------------------------------------------------
class CFilter: virtual public CObject, virtual public IFilter, virtual public IEventListener
{
    typedef CObject inherited;

protected:
    CFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
    EECode Construct();
    virtual ~CFilter();

public:
    virtual void Delete();

    virtual void Pause();
    virtual void Resume();
    virtual void Flush();

    virtual void PrintState();

    virtual EECode Start();
    virtual EECode SendCmd(TUint cmd);
    virtual void PostMsg(TUint cmd);

    virtual EECode FlowControl(EBufferType type);

    virtual IInputPin *GetInputPin(TUint index);
    virtual IOutputPin *GetOutputPin(TUint index, TUint sub_index);
    virtual EECode AddOutputPin(TUint &index, TUint &sub_index, TUint type);
    virtual EECode AddInputPin(TUint &index, TUint type);

    virtual void EventNotify(EEventType type, TUint param1, TPointer param2);

public:
    EECode PostEngineMsg(SMSG &msg);
    EECode PostEngineMsg(TUint code);

protected:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpEngineMsgSink;

};

//-----------------------------------------------------------------------
//
//  CInputPin
//
//-----------------------------------------------------------------------
class CInputPin: public CObject, public IInputPin
{
    typedef CObject inherited;

protected:
    CInputPin(const TChar *name, IFilter *pFilter, StreamType type);
    virtual ~CInputPin();

public:
    virtual void PrintStatus();

public:
    virtual void Delete();
    //virtual void Purge(TU8 disable_pin = 0);
    virtual void Enable(TU8 bEnable);

    virtual IOutputPin *GetPeer() const;
    virtual IFilter *GetFilter() const;

public:
    virtual EECode Connect(IOutputPin *pPeer, TComponentIndex index);
    virtual EECode Disconnect(TComponentIndex index);

public:
    virtual StreamType GetPinType() const;
    virtual EECode GetVideoParams(StreamFormat &format, TU32 &video_width, TU32 &video_height, TU8 *&p_extra_data, TMemSize &extra_data_size, TU32 &session_number) const;
    virtual EECode GetAudioParams(StreamFormat &format, TU32 &channel_number, TU32 &sample_frequency, AudioSampleFMT &fmt, TU8 *&p_extra_data, TMemSize &extra_data_size, TU32 &session_number) const;

    virtual EECode GetExtraData(TU8 *&p_extra_data, TMemSize &extra_data_size, TU32 &session_number) const;

protected:
    void updateSyncBuffer(CIBuffer *p_sync_Buffer);
    void updateExtraData(CIBuffer *p_extradata_Buffer);

protected:
    IFilter *mpFilter;
    IOutputPin *mpPeer;
    IBufferPool *mpBufferPool;

    TComponentIndex mPeerSubIndex;
    TU8 mbEnable;
    TU8 mbSyncedBufferComes;

protected:
    IMutex *mpMutex;
    TU8 *mpExtraData;
    TMemSize mExtraDataSize;
    TMemSize mExtraDataBufferSize;

protected:
    StreamType mPinType;
    StreamFormat mCodecFormat;
    AudioSampleFMT mAudioSampleFmt;

    TU32 mVideoWidth, mVideoHeight;
    TU32 mVideoFrameRateNum, mVideoFrameRateDen;
    TU32 mVideoOffsetX, mVideoOffsetY;
    TU32 mVideoSampleAspectRatioNum;
    TU32 mVideoSampleAspectRatioDen;

    TU32 mAudioSampleRate;
    TU32 mAudioFrameSize;

    TU8 mAudioChannelNumber;
    TU8 mAudioLayout;

    TU16 mSessionNumber;
};

//-----------------------------------------------------------------------
//
//  COutputPin
//
//-----------------------------------------------------------------------
class COutputPin: public CObject, public IOutputPin
{
    typedef CObject inherited;

public:
    static COutputPin *Create(const TChar *name, IFilter *pFilter);

public:
    virtual void PrintStatus();

protected:
    COutputPin(const TChar *name, IFilter *pFilter);
    virtual ~COutputPin();

public:
    virtual EECode Connect(IInputPin *pPeer, TUint index);
    virtual EECode Disconnect(TUint index);

public:
    bool AllocBuffer(CIBuffer *&pBuffer, TUint size = 0);
    void SendBuffer(CIBuffer *pBuffer);

public:
    void EnableAllSubPin(TU8 bEnable);
    //virtual void Enable(TUint index, TU8 bEnable);
    virtual IInputPin *GetPeer(TUint index);
    virtual IFilter *GetFilter();
    virtual TUint NumberOfPins();
    virtual EECode AddSubPin(TUint &sub_index);
    virtual IBufferPool *GetBufferPool() const;
    void SetBufferPool(IBufferPool *pBufferPool);

protected:
    IFilter *mpFilter;
    IBufferPool *mpBufferPool;
    TU8 mbSetBP; // the buffer pool is specified by SetBufferPool()
    TU8 mbEnabledAllSubpin;

    TU16 mnCurrentSubPinNumber;

    //TU8 mbEnabled[DMAX_CONNECTIONS_OUTPUTPIN];
    IInputPin *mpPeers[DMAX_CONNECTIONS_OUTPUTPIN];
};

//-----------------------------------------------------------------------
//
//  CQueueInputPin
//
//-----------------------------------------------------------------------
class CQueueInputPin: public CInputPin
{
    typedef CInputPin inherited;

public:
    static CQueueInputPin *Create(const TChar *pname, IFilter *pfilter, StreamType type, CIQueue *pMsgQ);

public:
    virtual void PrintStatus();

protected:
    CQueueInputPin(const TChar *name, IFilter *filter, StreamType type);
    EECode Construct(CIQueue *pMsgQ);
    virtual ~CQueueInputPin();

public:
    virtual void Delete();

public:
    virtual void Receive(CIBuffer *pBuffer);
    virtual void Purge(TU8 disable_pin = 0);
    bool PeekBuffer(CIBuffer *&pBuffer);
    CIQueue *GetBufferQ() const;

protected:
    CIQueue *mpBufferQ;
};

//-----------------------------------------------------------------------
//
//  CBypassInputPin
//
//-----------------------------------------------------------------------
class CBypassInputPin: public CInputPin
{
    typedef CInputPin inherited;

public:
    static CBypassInputPin *Create(const TChar *pname, IFilter *pfilter, StreamType type);

protected:
    CBypassInputPin(const TChar *name, IFilter *filter, StreamType type);
    EECode Construct();
    virtual ~CBypassInputPin();

public:
    virtual void Delete();

public:
    virtual void Receive(CIBuffer *pBuffer);
    virtual void Purge(TU8 disable_pin = 0);

};

//-----------------------------------------------------------------------
//
//  CActiveObject
//
//-----------------------------------------------------------------------
class CActiveObject: public CObject, public IActiveObject
{
    typedef CObject inherited;

protected:
    typedef IActiveObject AO;
    CActiveObject(const char *pName, TUint index = 0);
    EECode Construct();
    virtual ~CActiveObject();

public:
    virtual void Delete();
    virtual CObject *GetObject() const;

public:
    virtual EECode SendCmd(TUint cmd);
    virtual EECode PostCmd(TUint cmd);
    //PostCmd
    virtual void OnRun();

protected:
    CIQueue *MsgQ();
    void GetCmd(SCMD &cmd);
    bool PeekCmd(SCMD &cmd);
    void CmdAck(EECode err);

protected:
    CIWorkQueue *mpWorkQ;
    TU8 mbRun;
    TU8 mbPaused;
    TU8 mbTobeStopped;
    TU8 mbTobeSuspended;

    EModuleState msState;
};

//-----------------------------------------------------------------------
//
//  CActiveFilter
//
//-----------------------------------------------------------------------
class CActiveFilter: public CObject, public IFilter, public IActiveObject, public IEventListener
{
    typedef CObject inherited;

protected:
    typedef IActiveObject AO;

protected:
    CActiveFilter(const TChar *pName, TUint index, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
    EECode Construct();
    virtual ~CActiveFilter();

public:
    virtual void Delete();
    virtual CObject *GetObject0() const;
    virtual CObject *GetObject() const;

    // IFilter
    virtual EECode Run();
    virtual EECode Exit();
    virtual EECode Start();
    virtual EECode Stop();
    virtual void Pause();
    virtual void Resume();
    virtual void Flush();

    virtual void ResumeFlush();

    virtual EECode Suspend(TUint release_flag = EReleaseFlag_None);
    virtual EECode ResumeSuspend(TComponentIndex input_index = 0);

    virtual EECode SendCmd(TUint cmd);
    virtual void PostMsg(TUint cmd);

    virtual void GetInfo(INFO &info);
    CIQueue *MsgQ();

public:
    void EventNotify(EEventType type, TU64 param1, TPointer param2);
    virtual EECode OnCmd(SCMD &cmd);
    virtual const TChar *GetName() const;

protected:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpEngineMsgSink;
    CIClockReference *mpClockReference;

    CIWorkQueue *mpWorkQ;
    TU8 mbRun, mbPaused, mbStarted, mbSuspended;
    EModuleState msState;
};

//-----------------------------------------------------------------------
//
//  CActiveEngine
//
//-----------------------------------------------------------------------
class CActiveEngine: public CObject, public IActiveObject, public IMsgSink
{
    typedef CObject inherited;

protected:
    enum {
        CMD_TYPE_SINGLETON = 0,//need excute one by one
        CMD_TYPE_REPEAT_LAST,//next will replace previous
        CMD_TYPE_REPEAT_CNT,//cmd count
        CMD_TYPE_REPEAT_AVATOR,//need translate cmd
    };

protected:
    CActiveEngine(const TChar *pname, TUint index);
    EECode Construct();
    virtual ~CActiveEngine();

public:
    virtual void Delete();
    virtual CObject *GetObject() const;
    virtual EECode MsgProc(SMSG &msg);

    virtual void OnRun();
    virtual EECode SetAppMsgQueue(CIQueue *p_msg_queue);
    virtual EECode SetAppMsgSink(IMsgSink *pAppMsgSink);
    virtual EECode SetAppMsgCallback(void (*MsgProc)(void *, SMSG &), void *context);
    virtual EECode Connect(IFilter *up, TUint uppin_index, IFilter *down, TUint downpin_index, TUint uppin_sub_index);

    EECode PostEngineMsg(SMSG &msg);
    EECode PostAppMsg(SMSG &msg);

    virtual EECode OnCmd(SCMD &cmd);

    virtual const TChar *GetName() const;

protected:
    void NewSession();
    bool IsSessionMsg(SMSG &msg);

protected:
    volatile TU16 mSessionID;
    TU8 mReserved0, mReserved1;
    IMutex *mpMutex;

    CIWorkQueue *mpWorkQ;//mainq, cmd from user/app
    CIQueue *mpCmdQueue;//msg from user/app
    CIQueue *mpFilterMsgQ;//msg from filters

    CIQueue *mpAppMsgQueue;

protected:
    IMsgSink *mpAppMsgSink;
    void (*mpAppMsgProc)(void *, SMSG &);
    void *mpAppMsgContext;
};

//-----------------------------------------------------------------------
//
//  CScheduledFilter
//
//-----------------------------------------------------------------------
class CScheduledFilter: public CObject, public IFilter, public IScheduledClient, public IEventListener
{
    typedef CObject inherited;

protected:
    CScheduledFilter(const TChar *pName, TUint index, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
    EECode Construct();
    virtual ~CScheduledFilter();

public:
    virtual void Delete();
    virtual CObject *GetObject0() const;

    virtual EECode EventHandling(EECode err);

    // IFilter
    virtual EECode Run();
    virtual EECode Exit();

    virtual void Pause();
    virtual void Resume();
    virtual void Flush();

    virtual void ResumeFlush();

    virtual EECode Suspend(TUint release_context = 0);
    virtual EECode ResumeSuspend(TComponentIndex input_index = 0);

    virtual EECode SendCmd(TUint cmd);
    virtual void PostMsg(TUint cmd);

    //virtual void GetInfo(INFO& info);
    CIQueue *MsgQ();

    void EventNotify(EEventType type, TU64 param1, TPointer param2);

protected:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpEngineMsgSink;
    CIClockReference *mpClockReference;

    //CIWorkQueue *mpWorkQ;
    CIQueue *mpCmdQueue;
    TU8 mbRun, mbPaused, mbStarted, mbSuspended;
    EModuleState msState;
};

typedef struct {
    TU16 max_seq;           ///< highest sequence number seen
    TU16 reserved0;           ///< highest sequence number seen
    TU32 cycles;            ///< shifted count of sequence number cycles
    TU32 base_seq;          ///< base sequence number
    TU32 bad_seq;           ///< last bad sequence number + 1
    TInt probation;              ///< sequence packets till source is valid
    TInt received;               ///< packets received
    TInt expected_prior;         ///< packets expected in last interval
    TInt received_prior;         ///< packets received in last interval
    TU32 transit;           ///< relative transit time for previous packet
    TU32 jitter;            ///< estimated jitter.
} SRTPStatistics;

typedef struct {
    TU8 enabled;
    TU8 track_id;
    TU8 initialized;
    TU8 index;

    TU8 priority;
    TU8 reserved0;
    TU8 rtp_tcp_channel_id;
    TU8 rtcp_tcp_channel_id;

    TU16 rtp_port;
    TU16 rtcp_port;

    TU16 server_rtp_port;
    TU16 server_rtcp_port;

    TSocketHandler rtsp_socket;
    TSocketHandler rtp_socket;
    TSocketHandler rtcp_socket;
    TChar *server_addr;

    COutputPin *outpin;
    CBufferPool *bufferpool;
    IMemPool *mempool;

    StreamType type;
    StreamFormat format;
} SRTPContext;

class IScheduledRTPReceiver
{
protected:
    virtual ~IScheduledRTPReceiver() {}

public:
    virtual CObject *GetObject0() const = 0;

public:
    virtual EECode Initialize(SRTPContext *context, SStreamCodecInfo *p_stream_info, TUint number_of_content = 1) = 0;
    virtual EECode ReInitialize(SRTPContext *context, SStreamCodecInfo *p_stream_info, TUint number_of_content = 1) = 0;
    virtual EECode Flush() = 0;

    virtual EECode Purge() = 0;

    virtual EECode SetExtraData(TU8 *p_extradata, TU32 extradata_size, TU32 index) = 0;

public:
    virtual void SetVideoDataPostProcessingCallback(void *callback_context, void *callback) = 0;
    virtual void SetAudioDataPostProcessingCallback(void *callback_context, void *callback) = 0;
};

typedef struct {
    TU8 priority;
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;
} SPriority;

typedef struct {
    TU32 size;
} SBufferSetting;

typedef struct {
    void *server_agent;

    TU8 *p_user_token;
    TMemSize user_token_length;

    TChar *uploading_url;
} SCloudAgentSetting;

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
// ICloudServer
//
//-----------------------------------------------------------------------
#define DMaxCloudAgentUrlLength 128

enum {
    ESubSession_video = 0,
    ESubSession_audio,

    ESubSession_tot_count,
};

typedef struct {
    TComponentIndex uploader_output_pin_index;
    TU8 type;//StreamType;
    TU8 format;//StreamFormat format;

    TU8 enabled;
    TU8 data_comes;
    TU8 parameters_setup;
    TU8 reserved1;

    //video info
    TU32 video_width, video_height;
    TU32 video_bit_rate, video_framerate;

    //audio info
    TU32 audio_channel_number, audio_sample_rate;
    TU32 audio_bit_rate, audio_sample_format;
} SCloudSubContent;

class ICloudServer;

#ifdef BUILD_OS_WINDOWS

#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

typedef struct {
    TU8 b_connected;
    TU8 reserved0[3];

    TSocketHandler socket;
    void *p_agent;

    struct sockaddr_in client_addr;
} SCloudClient;

typedef struct {
    TGenericID id;

    TComponentIndex content_index;
    TU8 b_content_setup;
    TU8 sub_session_count;

    TU8 enabled;
    TU8 has_video;
    TU8 has_audio;
    TU8 reserved0;

    SCloudSubContent sub_session[ESubSession_tot_count];

    TChar content_name[DMaxCloudAgentUrlLength];

    ICloudServer *p_server;

    IFilter *p_cloud_agent_filter;

    SCloudClient client;

    TU32 dynamic_code;
} SCloudContent;

class ICloudServer
{
public:
    virtual CObject *GetObject0() const = 0;

public:
    virtual EECode HandleServerRequest(TSocketHandler &max, void *all_set) = 0;
    //virtual void ScanClientList(TInt& nready, void*read_set, void* all_set) = 0;

    virtual EECode AddCloudContent(SCloudContent *content) = 0;
    virtual EECode RemoveCloudContent(SCloudContent *content) = 0;
    virtual EECode RemoveCloudClient(void *filter_owner) = 0;

    virtual EECode GetHandler(TSocketHandler &handle) const = 0;

    virtual EECode GetServerState(EServerState &state) const = 0;
    virtual EECode SetServerState(EServerState state) = 0;

    virtual EECode GetServerID(TGenericID &id, TComponentIndex &index, TComponentType &type) const = 0;
    virtual EECode SetServerID(TGenericID id, TComponentIndex index, TComponentType type) = 0;
};

//-----------------------------------------------------------------------
//
// ICloudServerManager
//
//-----------------------------------------------------------------------

class ICloudServerManager
{
public:
    virtual void Destroy() = 0;
    virtual void PrintStatus0() = 0;

public:
    virtual EECode RunServerManager() = 0;
    virtual EECode ExitServerManager() = 0;
    virtual EECode Start() = 0;
    virtual EECode Stop() = 0;

    virtual ICloudServer *AddServer(CloudServerType type, TSocketPort server_port) = 0;
    virtual EECode RemoveServer(ICloudServer *server) = 0;

    virtual EECode AddCloudContent(ICloudServer *server_info, SCloudContent *content) = 0;
    virtual EECode RemoveCloudClient(ICloudServer *server_info, void *filter_owner) = 0;
};

extern ICloudServerManager *gfCreateCloudServerManager(const volatile SPersistMediaConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);

void gfClearBufferMemory(IBufferPool *p_buffer_pool);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif


