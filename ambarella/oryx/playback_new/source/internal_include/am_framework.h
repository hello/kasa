/**
 * am_framework.h
 *
 * History:
 *  2015/07/27 - [Zhi He] create file
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

#ifndef __AM_FRAMEWORK_H__
#define __AM_FRAMEWORK_H__

#define DMAX_CONNECTIONS_OUTPUTPIN 32

typedef struct {
  TPointer p_pointer;
  TComponentIndex index;
  StreamType type;
  StreamFormat format;

  TPointer p_agent_pointer;
} SQueryInterface;

typedef struct {
  TU32 session_id;
} SAutoReconnectStreamingServerParams;

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
  EBufferCustomizedContentType_V4l2DirectBuffer,
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
  EFilterPropertyType_set_max_frame_number,
  EFilterPropertyType_set_stream_id,
  EFilterPropertyType_set_source_buffer_id,

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
  EFilterPropertyType_demuxer_reconnect_server, // from app
  EFilterPropertyType_demuxer_auto_reconnect_server, // from internal
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

  //video capture
  EFilterPropertyType_video_capture_params = 0x0130,
  EFilterPropertyType_video_capture_snapshot_params = 0x0131,
  EFilterPropertyType_video_capture_periodic_params = 0x0132,

  //video decoder
  EFilterPropertyType_vdecoder_pb_speed_feedingrule = 0x0200,
  EFilterPropertyType_vdecoder_zoom,
  EFilterPropertyType_vdecoder_wait_audio_precache,
  EFilterPropertyType_vdecoder_audio_precache_notify,

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
  EFilterPropertyType_vrenderer_screen_capture,

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

  //video injecter
  EFilterPropertyType_vinjecter_set_callback = 0x0b40,
  EFilterPropertyType_vinjecter_batch_inject = 0x0b41,

} EFilterPropertyType;

typedef struct {
  TUint code;
  void    *pExtra;
  TU16 sessionID;
  TU8 flag;
  TU8 needFreePExtra;

  TU8 owner_type, owner_index, reserved0, reserved1;
  TULong p_owner, owner_id;

  TULong p1, p2, p3, p4;
} SScheduledClient;

typedef enum {
  EModuleState_Invalid = 0,

  //commom used
  EModuleState_Idle = 0x0001,
  EModuleState_Preparing = 0x0002,
  EModuleState_Running = 0x0003,
  EModuleState_HasInputData = 0x0004,
  EModuleState_HasOutputBuffer = 0x0005,
  EModuleState_Completed = 0x0006,
  EModuleState_Bypass = 0x0007,
  EModuleState_Pending = 0x0008,
  EModuleState_Stopped = 0x0009,
  EModuleState_Error = 0x000a,

  EModuleState_WaitCmd = 0x000b,
  EModuleState_WaitTiming = 0x000c,
  EModuleState_Halt = 0x000d,

  EModuleState_Step = 0x000e,

  //specified module
  //video capture
  EModuleState_SnapshotCapture = 0x0030,
  EModuleState_WaitSnapshotCapture = 0x0031,

  //muxer
  EModuleState_Muxer_WaitExtraData = 0x0040,
  EModuleState_Muxer_SavingPartialFile = 0x0041,
  EModuleState_Muxer_SavingPartialFilePeekRemainingOnMasterPin = 0x0042,
  EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin = 0x0043,
  EModuleState_Muxer_SavingPartialFileWaitMasterPin = 0x0044,
  EModuleState_Muxer_SavingPartialFileWaitNonMasterPin = 0x0045,

  EModuleState_Muxer_SavingWholeFile = 0x0046,
  EModuleState_Muxer_FlushExpiredFrame = 0x0047,

  //renderer
  EModuleState_Renderer_PreBuffering = 0x0060,
  EModuleState_Renderer_PreBufferingDone = 0x0061,

  EModuleState_Renderer_WaitDecoderReady = 0x0062,
  EModuleState_Renderer_WaitVoutDormant = 0x0063,
  EModuleState_Renderer_WaitPlaybackBegin = 0x0064,
  EModuleState_Renderer_WaitVoutEOSFlag = 0x0065,
  EModuleState_Renderer_WaitVoutLastFrameDisplayed = 0x0066,

  //ts demuxer
  EModuleState_Demuxer_UpdatingContext = 0x0070,

  //audio capture
  EModuleState_AudioCapture_Muting = 0x0080,
  EModuleState_AudioCapture_Mute = 0x0081,

  //direct encoder
  EModuleState_DirectEncoding = 0x0090,
} EModuleState;

extern const TChar *gfGetModuleStateString(EModuleState state);

extern const TChar *gfGetCMDTypeString(ECMDType type);

//-----------------------------------------------------------------------
//
// IMemPool
//
//-----------------------------------------------------------------------
class IMemPool
{
public:
  virtual CObject *GetObject0() const = 0;

public:
  virtual TU8 *RequestMemBlock(TULong size, TU8 *start_pointer = NULL) = 0;
  virtual void ReturnBackMemBlock(TULong size, TU8 *start_pointer) = 0;
  virtual void ReleaseMemBlock(TULong size, TU8 *start_pointer) = 0;
  virtual TU32 IsEnoughRoom(TMemSize size) = 0;

protected:
  virtual ~IMemPool() {}
};

//-----------------------------------------------------------------------
//
//  IMsgSink
//
//-----------------------------------------------------------------------
class IMsgSink
{
public:
  virtual EECode MsgProc(SMSG &msg) = 0;

protected:
  virtual ~IMsgSink() {}
};

//-----------------------------------------------------------------------
//
// IStreamingServerManager
//
//-----------------------------------------------------------------------

class IStreamingServerManager
{
public:
  virtual void PrintStatus0() = 0;
  virtual void Destroy() = 0;

public:
  virtual EECode RunServerManager() = 0;
  virtual EECode ExitServerManager() = 0;

  virtual EECode Start() = 0;
  virtual EECode Stop() = 0;
  virtual IStreamingServer *AddServer(StreamingServerType type, StreamingServerMode mode, TU16 server_port, TU8 enable_video, TU8 enable_audio) = 0;
  virtual EECode RemoveServer(IStreamingServer *server) = 0;

  virtual EECode AddStreamingContent(IStreamingServer *server_info, SStreamingSessionContent *content) = 0;

  virtual EECode RemoveSession(IStreamingServer *server_info, void *session) = 0;
};

//-----------------------------------------------------------------------
//
//  IScheduledClient
//
//-----------------------------------------------------------------------
typedef TUint TSchedulingUnit;
typedef TSocketHandler TSchedulingHandle;
class IScheduledClient
{
public:
  virtual EECode Scheduling(TUint times = 1, TU32 inout_mask = 0) = 0;

public:
  virtual TInt IsPassiveMode() const = 0;
  virtual TSchedulingHandle GetWaitHandle() const = 0;
  virtual TU8 GetPriority() const = 0;
  virtual EECode EventHandling(EECode err) = 0;

public:
  virtual TSchedulingUnit HungryScore() const = 0;

protected:
  virtual ~IScheduledClient() {}
};

//-----------------------------------------------------------------------
//
//  IScheduler
//
//-----------------------------------------------------------------------
class IScheduler
{
public:
  virtual CObject *GetObject0() const = 0;

public:
  virtual EECode StartScheduling() = 0;
  virtual EECode StopScheduling() = 0;

public:
  virtual EECode AddScheduledCilent(IScheduledClient *client) = 0;
  virtual EECode RemoveScheduledCilent(IScheduledClient *client) = 0;

public:
  virtual TUint TotalNumberOfScheduledClient() const = 0;
  virtual TUint CurrentNumberOfClient() const = 0;
  virtual TSchedulingUnit CurrentHungryScore() const = 0;

public:
  virtual TInt IsPassiveMode() const = 0;

protected:
  virtual ~IScheduler() {}
};

//-----------------------------------------------------------------------
//
//  IEventListener
//
//-----------------------------------------------------------------------
typedef enum {
  EEventType_Invalid = 0,
  EEventType_BufferReleaseNotify,
  EEventType_Timer,
} EEventType;

class IEventListener
{
public:
  virtual void EventNotify(EEventType type, TU64 param1, TPointer param2) = 0;
protected:
  virtual ~IEventListener() {}
};

//-----------------------------------------------------------------------
//
//  IActiveObject
//
//-----------------------------------------------------------------------
class IActiveObject
{
public:
  virtual CObject *GetObject() const = 0;

public:
  virtual void OnRun() = 0;

protected:
  virtual ~IActiveObject() {}
};

//-----------------------------------------------------------------------
//
//  IStorageManagement
//
//-----------------------------------------------------------------------
typedef struct {
#ifdef BIG_ENDIAN
  TU8 duty_cycle_bit : 1;
  TU8 length : 7;
#else
  TU8 length : 7;
  TU8 duty_cycle_bit : 1;
#endif
} STimeLineDutyCycleUint;

typedef struct {
  TU32 total_unit_number;

  TU8 unit_length; // how many minutes
  TU8 reserved0;
  TU8 reserved1;
  TU8 reserved2;

  STimeLineDutyCycleUint *p_units;
} STimeLineDutyCycle;

typedef struct {
  TU32 total_unit_number;

  TU8 unit_length; // how many minutes
  TU8 reserved0;
  TU8 reserved1;
  TU8 reserved2;

  TU8 *p_units;
} STimeLineRawMask;

typedef enum {
  EQueryStorageType_dutyCycle,
  EQueryStorageType_rawMask,
} EQueryStorageType;

class IStorageManagement
{
public:
  virtual EECode LoadDataBase(TChar *root_dir, TU32 &channel_number) = 0;//if config files exist, load data base from file; if config files NOT exist, load data base from file system
  virtual EECode ClearDataBase(TChar *root_dir, TU32 channel_number) = 0;
  virtual EECode SaveDataBase(TU32 &channel_number) = 0;

public:
  virtual EECode AddChannel(TChar *channel_name, TU32 max_storage_minutes, TU32 redudant_storage_minutes, TU32 single_file_duration_minutes) = 0;
  virtual EECode RemoveChannel(TChar *channel_name) = 0;

public:
  virtual EECode ClearChannelStorage(TChar *channel_name) = 0;
  virtual EECode QueryChannelStorage(TChar *channel_name, void *&p_out, EQueryStorageType type = EQueryStorageType_dutyCycle) = 0;

public:
  virtual EECode RequestExistUint(TChar *channel_name, SDateTime *time, TChar *&file_name, SDateTime *file_start_time, TU16 &file_duration_minutes) = 0;
  virtual EECode RequestExistUintSequentially(TChar *channel_name, TChar *old_file_name, TU8 direction, TChar *&file_name, SDateTime *file_start_time, TU16 &file_duration_minutes) = 0;//direction: 0-newer file, 1-older file
  virtual EECode ReleaseExistUint(TChar *channel_name, TChar *file_name) = 0;

  virtual EECode AcquireNewUint(TChar *channel_name, TChar *&file_name) = 0;
  virtual EECode FinalizeNewUint(TChar *channel_name, TChar *file_name) = 0;

public:
  virtual void Delete() = 0;

protected:
  virtual ~IStorageManagement() {}
};

//-----------------------------------------------------------------------
//
//  ISimpleQueue
//
//-----------------------------------------------------------------------

class ISimpleQueue
{
public:
  virtual void Destroy() = 0;

protected:
  virtual ~ISimpleQueue() {}

public:
  virtual TU32 GetCnt() = 0;
  virtual void Lock() = 0;
  virtual void UnLock() = 0;

  virtual void Enqueue(TULong ctx) = 0;
  virtual TULong Dequeue() = 0;
  virtual TU32 TryDequeue(TULong &ret_ctx) = 0;
};

extern ISimpleQueue *gfCreateSimpleQueue(TU32 num);

//-----------------------------------------------------------------------
//
//  CIQueue
//
//-----------------------------------------------------------------------

class CIQueue
{
public:
  struct WaitResult {
    CIQueue *pDataQ;
    void *pOwner;
    TUint blockSize;
  };

  enum QType {
    Q_MSG,
    Q_DATA,
    Q_NONE,
  };

public:
  static CIQueue *Create(CIQueue *pMainQ, void *pOwner, TUint blockSize, TUint nReservedSlots);
  void Delete();

private:
  CIQueue(CIQueue *pMainQ, void *pOwner);
  EECode Construct(TUint blockSize, TUint nReservedSlots);
  ~CIQueue();

public:
  EECode PostMsg(const void *pMsg, TUint msgSize);
  EECode SendMsg(const void *pMsg, TUint msgSize);

  void GetMsg(void *pMsg, TUint msgSize);
  bool PeekMsg(void *pMsg, TUint msgSize);
  void Reply(EECode result);

  bool GetMsgEx(void *pMsg, TUint msgSize);
  void Enable(bool bEnabled = true);

  EECode PutData(const void *pBuffer, TUint size);

  QType WaitDataMsg(void *pMsg, TUint msgSize, WaitResult *pResult);
  QType WaitDataMsgCircularly(void *pMsg, TUint msgSize, WaitResult *pResult);
  QType WaitDataMsgWithSpecifiedQueue(void *pMsg, TUint msgSize, const CIQueue *pQueue);
  void WaitMsg(void *pMsg, TUint msgSize);
  bool PeekData(void *pBuffer, TUint size);
  TUint GetDataCnt() const {AUTO_LOCK(mpMutex); return mnData;}

private:
  EECode swicthToNextDataQueue(CIQueue *pCurrent);

public:
  bool IsMain() { return mpMainQ == NULL; }
  bool IsSub() { return mpMainQ != NULL; }

private:
  struct List {
    List *pNext;
    bool bAllocated;
    void Delete();
  };

private:
  void *mpOwner;
  bool mbDisabled;

  CIQueue *mpMainQ;
  CIQueue *mpPrevQ;
  CIQueue *mpNextQ;

  IMutex *mpMutex;
  ICondition *mpCondReply;
  ICondition *mpCondGet;
  ICondition *mpCondSendMsg;

  TUint mnGet;
  TUint mnSendMsg;

  TUint mBlockSize;
  TUint mnData;

  List *mpTail;
  List *mpFreeList;

  List mHead;

  List *mpSendBuffer;
  TU8 *mpReservedMemory;

  EECode *mpMsgResult;

private:
  CIQueue *mpCurrentCircularlyQueue;

private:
  static void Copy(void *to, const void *from, TUint bytes) {
    if (bytes == sizeof(void *)) {
      *(void **)to = *(void **)from;
    } else {
      memcpy(to, from, bytes);
    }
  }

  List *AllocNode();

  void WriteData(List *pNode, const void *pBuffer, TUint size);
  void ReadData(void *pBuffer, TUint size);

};

//-----------------------------------------------------------------------
//
//  CIDoubleLinkedList
//
//-----------------------------------------------------------------------
//common container, not thread safe
class CIDoubleLinkedList
{
public:
  struct SNode {
    void *p_context;
  };

private:
  //trick here, not very good, make pointer not expose
  struct SNodePrivate {
    void *p_context;
    SNodePrivate *p_next, *p_pre;
  };

public:
  CIDoubleLinkedList();
  virtual ~CIDoubleLinkedList();

public:
  //if target_node == NULL, means target_node == list head
  SNode *InsertContent(SNode *target_node, void *content, TUint after = 1);
  void RemoveContent(void *content);
  void FastRemoveContent(SNode *target_node, void *content);
  //AM_ERR RemoveNode(SNode* node);
  SNode *FirstNode() const;
  SNode *LastNode() const;
  SNode *PreNode(SNode *node) const;
  SNode *NextNode(SNode *node) const;
  TUint NumberOfNodes() const;
  void RemoveAllNodes();

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE
public:
  static void *operator new(size_t, void *p);
  static void *operator new(size_t);
  //static void operator delete(void* p, size_t);
#endif

  //debug function
public:
  bool IsContentInList(void *content) const;

private:
  bool IsNodeInList(SNode *node) const;
  void allocNode(SNodePrivate *&node);

private:
  SNodePrivate mHead;
  TUint mNumberOfNodes;
  SNodePrivate *mpFreeList;
};

//-----------------------------------------------------------------------
//
// CRingMemPool
//
//-----------------------------------------------------------------------
class CRingMemPool: public CObject, public IMemPool
{
  typedef CObject inherited;

public:
  static IMemPool *Create(TMemSize size);
  virtual CObject *GetObject0() const;
  virtual void PrintStatus();
  virtual void Delete();

  virtual TU8 *RequestMemBlock(TMemSize size, TU8 *start_pointer = NULL);
  virtual void ReturnBackMemBlock(TMemSize size, TU8 *start_pointer);
  virtual void ReleaseMemBlock(TMemSize size, TU8 *start_pointer);
  virtual TU32 IsEnoughRoom(TMemSize size);

protected:
  CRingMemPool();
  EECode Construct(TMemSize size);
  virtual ~CRingMemPool();

private:
  void printRingPoolStatus();

private:
  IMutex *mpMutex;
  ICondition *mpCond;

private:
  TU8 mRequestWrapCount;
  TU8 mReleaseWrapCount;
  TU8 mReserved1;
  TU8 mReserved2;

private:
  TU8 *mpMemoryBase;
  TU8 *mpMemoryEnd;
  TMemSize mMemoryTotalSize;

  TU8 *volatile mpFreeMemStart;
  TU8 *volatile mpUsedMemStart;

private:
  volatile TU32 mnWaiters;
};

//-----------------------------------------------------------------------
//
// CFixedMemPool
//
//-----------------------------------------------------------------------
class CFixedMemPool: public CObject, public IMemPool
{
  typedef CObject inherited;

public:
  static IMemPool *Create(const TChar *name, TMemSize size, TUint tot_count);
  virtual CObject *GetObject0() const;
  virtual void Delete();

  virtual TU8 *RequestMemBlock(TMemSize size, TU8 *start_pointer = NULL);
  virtual void ReturnBackMemBlock(TMemSize size, TU8 *start_pointer);
  virtual void ReleaseMemBlock(TMemSize size, TU8 *start_pointer);
  virtual TU32 IsEnoughRoom(TMemSize size);

protected:
  CFixedMemPool(const TChar *name);
  EECode Construct(TMemSize size, TUint tot_count);
  virtual ~CFixedMemPool();

private:
  IMutex *mpMutex;
  ICondition *mpCond;

private:
  TU8 *mpMemoryBase;
  TU8 *mpMemoryEnd;
  TMemSize mMemoryTotalSize;

  TU8 *volatile mpFreeMemStart;
  TU8 *volatile mpUsedMemStart;

  TMemSize mMemBlockSize;
  TU32   mnTotalBlockNumber;
  TU32 mnCurrentFreeBlockNumber;

private:
  volatile TU32 mnWaiters;
};

//-----------------------------------------------------------------------
//
//  CIWorkQueue
//
//-----------------------------------------------------------------------
class CIWorkQueue: public CObject
{
  typedef CObject inherited;
  typedef IActiveObject AO;

public:
  static CIWorkQueue *Create(AO *pAO);
  virtual void Delete();

private:
  CIWorkQueue(AO *pAO);
  EECode Construct();
  ~CIWorkQueue();

public:
  // return receive's reply
  EECode SendCmd(TUint cid, void *pExtra = NULL);
  EECode SendCmd(SCMD &cmd);
  void GetCmd(SCMD &cmd);
  bool PeekCmd(SCMD &cmd);

  EECode PostMsg(TUint cid, void *pExtra = NULL);
  EECode PostMsg(SCMD &cmd);
  EECode Run();
  EECode Exit();
  EECode Stop();
  EECode Start();

  void CmdAck(EECode result);
  CIQueue *MsgQ() const;

  CIQueue::QType WaitDataMsg(void *pMsg, TUint msgSize, CIQueue::WaitResult *pResult);
  CIQueue::QType WaitDataMsgCircularly(void *pMsg, TUint msgSize, CIQueue::WaitResult *pResult);
  CIQueue::QType WaitDataMsgWithSpecifiedQueue(void *pMsg, TUint msgSize, const CIQueue *pQueue);
  void WaitMsg(void *pMsg, TUint msgSize);

private:
  void MainLoop();
  static EECode ThreadEntry(void *p);
  void Terminate();

private:
  AO *mpAO;
  CIQueue *mpMsgQ;
  IThread *mpThread;

  const TChar *mpDebugModuleName;
};

//-----------------------------------------------------------------------
//
// CIClockReference
//
//-----------------------------------------------------------------------

typedef enum {
  EClockTimerType_once = 0,
  EClockTimerType_repeat_count,
  EClockTimerType_repeat_infinite,
} EClockTimerType;

typedef struct SClockListener_s {
  IEventListener *p_listener;
  EClockTimerType type;
  TTime event_time;
  TTime event_duration;

  TUint event_remaining_count;

  struct SClockListener_s *p_next;

  TULong user_context;
} SClockListener;


class CIClockReference: public CObject
{
  typedef CObject inherited;
protected:
  CIClockReference();
  ~CIClockReference();

  EECode Construct();

public:
  static CIClockReference *Create();
  void Delete();

public:
  void SetBeginTime(TTime start_time);
  void SetDirection(TU8 forward);
  void SetSpeed(TU8 speed, TU8 speed_frac);

  EECode Start();
  EECode Pause();
  EECode Resume();
  EECode Stop();

  EECode Reset(TTime source_time, TTime target_time, TU8 sync_source_time, TU8 sync_target_time, TU8 speed, TU8 speed_frac, TU8 forward);

  void Heartbeat(TTime current_source_time);

  void ClearAllTimers();
  SClockListener *AddTimer(IEventListener *p_listener, EClockTimerType type, TTime time, TTime interval, TUint count = 0);
  EECode RemoveTimer(SClockListener *listener);
  SClockListener *GuardedAddTimer(IEventListener *p_listener, EClockTimerType type, TTime time, TTime interval, TUint count);
  EECode GuardedRemoveTimer(SClockListener *listener);
  EECode RemoveAllTimers(IEventListener *p_listener);

  TTime GetCurrentTime() const;

  virtual void PrintStatus();

private:
  SClockListener *allocClockListener();
  void releaseClockListener(SClockListener *p);
  void clearAllClockListener();
  void updateTime(TTime target_time);

private:
  IMutex *mpMutex;

  volatile TTime mBeginSourceTime;
  volatile TTime mCurrentSourceTime;

  volatile TTime mBeginTime;
  volatile TTime mCurrentTime;

  TU8 mbPaused;
  TU8 mbStarted;
  TU8 mbSilent;
  TU8 mbForward;

  TU8 mSpeed;
  TU8 mSpeedFrac;

  TU16 mSpeedCombo;

  CIDoubleLinkedList *mpClockListenerList;
  SClockListener *mpFreeClockListenerList;
};

//-----------------------------------------------------------------------
//
// CIClockManager
//
//-----------------------------------------------------------------------
class CIClockManager: public CObject, public IActiveObject
{
  typedef CObject inherited;
public:
  static CIClockManager *Create();
  void Delete();
  virtual CObject *GetObject() const;

protected:
  EECode Construct();
  CIClockManager();
  ~CIClockManager();

public:
  IClockSource *GetClockSource() const;
  EECode SetClockSource(IClockSource *pClockSource);
  EECode RegisterClockReference(CIClockReference *p_clock_reference);
  EECode UnRegisterClockReference(CIClockReference *p_clock_reference);

  EECode Start();
  EECode Stop();

  virtual void PrintStatus();

protected:
  virtual void OnRun();

private:
  virtual EECode processCMD(SCMD &cmd);

private:
  void updateClockReferences();

private:
  CIWorkQueue *mpWorkQ;
  IClockSource *mpClockSource;

  CIDoubleLinkedList *mpClockReferenceList;
  IEvent *mpEvent;

  TU8 mbRunning;
  TU8 mbHaveClockSource;
  TU8 mbPaused, mbClockReferenceListDestroyed;
};

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
    DATA_SEGMENT_BEGIN_INDICATOR = (1 << 6),
    LAST_FRAME_INDICATOR = (1 << 7),
    END_OF_SEQUENCE_INDICATOR = (1 << 8),
    MEMORY_BLOCK_NUMBER = 8,
    RTP_AAC = 0x1000,
  };

public:
  TAtomic mRefCount;
  CIBuffer *mpNext;
  EBufferType     mBufferType;
  TU8 mVideoFrameType, mbAudioPCMChannelInterlave, mAudioChannelNumber, mAudioSampleFormat;
  TU8 mbAllocated, mbExtEdge, mContentFormat, mbDataSegment;
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

  TU16 mMaxVideoGopSize;
  TU16 mCurVideoGopSize;

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
  TTime mLastTimestamp;
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

  TU32 mReconnectSessionID;
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

void gfClearBufferMemory(IBufferPool *p_buffer_pool);

extern const TChar *gfGetComponentStringFromGenericComponentType(TComponentType type);

typedef enum {
  ESchedulerType_Invalid = 0,

  ESchedulerType_RunRobin,
  ESchedulerType_Preemptive,
  ESchedulerType_PriorityPreemptive,

} ESchedulerType;

extern IScheduler *gfSchedulerFactory(ESchedulerType type, TUint index);
extern IStreamingServerManager *gfCreateStreamingServerManager(const volatile SPersistMediaConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);

//misc
EECode gfAllocateNV12FrameBuffer(CIBuffer *buffer, TU32 width, TU32 height, TU32 alignment);

#endif

