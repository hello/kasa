/*******************************************************************************
 * common_base.h
 *
 * History:
 *	2012/12/07 - [Zhi He] create file
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

#ifndef __COMMON_BASE_H__
#define __COMMON_BASE_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

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
    CObject(const TChar* name, TUint index = 0);
    virtual void SetLogLevel(TUint level);
    virtual void SetLogOption(TUint option);
    virtual void SetLogOutput(TUint output);

public:
    virtual const TChar* GetModuleName() const;
    virtual TUint GetModuleIndex() const;

protected:
    TUint mConfigLogLevel;
    TUint mConfigLogOption;
    TUint mConfigLogOutput;

protected:
    const TChar* mpModuleName;

protected:
    TUint mIndex;

protected:
    TUint mDebugHeartBeat;

protected:
    FILE* mpLogOutputFile;
};

#define DCONTROL_NOTYFY_DATA_PAUSE 0x01
#define DCONTROL_NOTYFY_DATA_RESUME 0x02

class SCMD
{
public:
    TUint code;
    void    *pExtra;
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

typedef struct
{
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

typedef struct
{
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
    EModuleState_Renderer_WaitVoutDormant = 0x0062,

    //ts demuxer
    EModuleState_Demuxer_UpdatingContext = 0x0070,

    //audio capture
    EModuleState_AudioCapture_Muting = 0x0080,
    EModuleState_AudioCapture_Mute = 0x0081,

    //direct encoder
    EModuleState_DirectEncoding = 0x0090,
} EModuleState;

extern const TChar* gfGetModuleStateString(EModuleState state);

extern const TChar* gfGetCMDTypeString(ECMDType type);

//-----------------------------------------------------------------------
//
//  IMsgSink
//
//-----------------------------------------------------------------------
class IMsgSink
{
public:
    virtual EECode MsgProc(SMSG& msg) = 0;

protected:
    virtual ~IMsgSink() {}
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
    virtual CObject* GetObject0() const = 0;

public:
    virtual EECode StartScheduling() = 0;
    virtual EECode StopScheduling() = 0;

public:
    virtual EECode AddScheduledCilent(IScheduledClient* client) = 0;
    virtual EECode RemoveScheduledCilent(IScheduledClient* client) = 0;

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
typedef enum
{
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
    virtual CObject* GetObject() const = 0;

public:
    virtual void OnRun() = 0;

protected:
    virtual ~IActiveObject() {}
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
    struct SNode
    {
        void* p_context;
    };

private:
    //trick here, not very good, make pointer not expose
    struct SNodePrivate
    {
        void* p_context;
        SNodePrivate* p_next, *p_pre;
    };

public:
    CIDoubleLinkedList();
    virtual ~CIDoubleLinkedList();

public:
    //if target_node == NULL, means target_node == list head
    SNode* InsertContent(SNode* target_node, void* content, TUint after = 1);
    void RemoveContent(void* content);
    void FastRemoveContent(SNode* target_node, void* content);
    //AM_ERR RemoveNode(SNode* node);
    SNode* FirstNode() const;
    SNode* LastNode() const;
    SNode* PreNode(SNode* node) const;
    SNode* NextNode(SNode* node) const;
    TUint NumberOfNodes() const;
    void RemoveAllNodes();

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE
public:
    static void* operator new (size_t, void* p);
    static void* operator new (size_t);
    //static void operator delete(void* p, size_t);
#endif

//debug function
public:
    bool IsContentInList(void* content) const;

private:
    bool IsNodeInList(SNode* node) const;
    void allocNode(SNodePrivate* & node);

private:
    SNodePrivate mHead;
    TUint mNumberOfNodes;
    SNodePrivate* mpFreeList;
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
    static CIWorkQueue* Create(AO *pAO);
    virtual void Delete();

private:
    CIWorkQueue(AO *pAO);
    EECode Construct();
    ~CIWorkQueue();

public:
    // return receive's reply
    EECode SendCmd(TUint cid, void *pExtra = NULL);
    EECode SendCmd(SCMD& cmd);
    void GetCmd(SCMD& cmd);
    bool PeekCmd(SCMD& cmd);

    EECode PostMsg(TUint cid, void *pExtra = NULL);
    EECode PostMsg(SCMD& cmd);
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

    const TChar* mpDebugModuleName;
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
    virtual void GetClockFrequency(TUint& num, TUint& den) const = 0;

    virtual void SetClockState(EClockSourceState state) = 0;
    virtual EClockSourceState GetClockState() const = 0;

    virtual void UpdateTime() = 0;

public:
    virtual CObject* GetObject0() const = 0;

protected:
    virtual ~IClockSource() {}
};

typedef enum {
    EClockSourceType_generic = 0,
} EClockSourceType;

extern IClockSource* gfCreateClockSource(EClockSourceType type = EClockSourceType_generic);

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

typedef struct SClockListener_s
{
    IEventListener* p_listener;
    EClockTimerType type;
    TTime event_time;
    TTime event_duration;

    TUint event_remaining_count;

    struct SClockListener_s* p_next;

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
    static CIClockReference* Create();
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
    SClockListener* AddTimer(IEventListener* p_listener, EClockTimerType type, TTime time, TTime interval, TUint count = 0);
    EECode RemoveTimer(SClockListener* listener);
    EECode GuardedRemoveTimer(SClockListener* listener);
    EECode RemoveAllTimers(IEventListener* p_listener);

    TTime GetCurrentTime() const;

    virtual void PrintStatus();

private:
    SClockListener* allocClockListener();
    void releaseClockListener(SClockListener* p);
    void clearAllClockListener();
    void updateTime(TTime target_time);

private:
    IMutex* mpMutex;

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

    CIDoubleLinkedList* mpClockListenerList;
    SClockListener* mpFreeClockListenerList;
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
    static CIClockManager* Create();
    void Delete();
    virtual CObject* GetObject() const;

protected:
    EECode Construct();
    CIClockManager();
    ~CIClockManager();

public:
    IClockSource* GetClockSource() const;
    EECode SetClockSource(IClockSource* pClockSource);
    EECode RegisterClockReference(CIClockReference* p_clock_reference);
    EECode UnRegisterClockReference(CIClockReference* p_clock_reference);

    EECode Start();
    EECode Stop();

    virtual void PrintStatus();

protected:
    virtual void OnRun();

private:
    virtual EECode processCMD(SCMD& cmd);

private:
    void updateClockReferences();

private:
    CIWorkQueue* mpWorkQ;
    IClockSource* mpClockSource;

    CIDoubleLinkedList* mpClockReferenceList;
    IEvent* mpEvent;

    TU8 mbRunning;
    TU8 mbHaveClockSource;
    TU8 mbPaused, mbClockReferenceListDestroyed;
};

//-----------------------------------------------------------------------
//
// IMemPool
//
//-----------------------------------------------------------------------
class IMemPool
{
public:
    virtual CObject* GetObject0() const = 0;

public:
    virtual TU8* RequestMemBlock(TULong size, TU8* start_pointer = NULL) = 0;
    virtual void ReturnBackMemBlock(TULong size, TU8* start_pointer) = 0;
    virtual void ReleaseMemBlock(TULong size, TU8* start_pointer) = 0;

protected:
    virtual ~IMemPool() {}
};

//-----------------------------------------------------------------------
//
//  ICustomizedCodec
//
//-----------------------------------------------------------------------
class ICustomizedCodec
{
public:
    virtual EECode ConfigCodec(TU32 param1, TU32 param2, TU32 param3, TU32 param4, TU32 param5) = 0;
    virtual EECode QueryInOutBufferSize(TMemSize& encoder_input_buffer_size, TMemSize& encoder_output_buffer_size) const = 0;

public:
    virtual EECode Encoding(void* in_buffer, void* out_buffer, TMemSize in_data_size, TMemSize& out_data_size) = 0;
    virtual EECode Decoding(void* in_buffer, void* out_buffer, TMemSize in_data_size, TMemSize& out_data_size) = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~ICustomizedCodec() {}
};

ICustomizedCodec* gfCustomizedCodecFactory(TChar* codec_name, TUint index);
const TChar* gfGetServerManagerStateString(EServerManagerState state);

typedef enum {
    ESchedulerType_Invalid = 0,

    ESchedulerType_RunRobin,
    ESchedulerType_Preemptive,
    ESchedulerType_PriorityPreemptive,

} ESchedulerType;

extern IScheduler* gfSchedulerFactory(ESchedulerType type, TUint index);

//-----------------------------------------------------------------------
//
//  IIPCAgent
//
//-----------------------------------------------------------------------
typedef EECode (*TIPCAgentReadCallBack) (void* owner, TU8*& p_data, TInt& datasize, TU32& remainning);

class IIPCAgent
{
public:
    virtual EECode CreateContext(TChar* tag, TUint is_server, void* p_context, TIPCAgentReadCallBack callback, TU16 native_port = 0, TU16 remote_port = 0) = 0;
    virtual void DestroyContext() = 0;

    virtual TInt GetHandle() const = 0;

    virtual EECode Start() = 0;
    virtual EECode Stop() = 0;

    virtual EECode Write(TU8* data, TInt size) = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~IIPCAgent() {}
};

extern IIPCAgent* gfIPCAgentFactory(EIPCAgentType type);

//-----------------------------------------------------------------------
//
//  CIRemoteLogServer
//
//-----------------------------------------------------------------------
class CIRemoteLogServer: public CObject
{
public:
    static CIRemoteLogServer* Create();

protected:
    CIRemoteLogServer();
    virtual ~CIRemoteLogServer();
    EECode Construct();

public:
    virtual EECode CreateContext(TChar* tag, TU16 native_port = 0, TU16 remote_port = 0);
    virtual void DestroyContext();

    virtual EECode WriteLog(const TChar* log, ...);
    virtual EECode SyncLog();

public:
    virtual void Delete();

public:
    virtual void Destroy();

private:
    IIPCAgent* mpAgent;

private:
    IMutex* mpMutex;

private:
    TU16 mPort;
    TU16 mRemotePort;

private:
    TChar* mpTextBuffer;
    TInt mTextBufferSize;

    TChar* mpCurrentWriteAddr;
    TInt mRemainingSize;

private:
    TU8 mbCreateContext;
    TU8 mbBufferFull;
    TU8 mReserved0;
    TU8 mReserved1;
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
    static IMemPool* Create(TMemSize size);
    virtual CObject* GetObject0() const;
    virtual void PrintStatus();
    virtual void Delete();

    virtual TU8* RequestMemBlock(TMemSize size, TU8* start_pointer = NULL);
    virtual void ReturnBackMemBlock(TMemSize size, TU8* start_pointer);
    virtual void ReleaseMemBlock(TMemSize size, TU8* start_pointer);

protected:
    CRingMemPool();
    EECode Construct(TMemSize size);
    virtual ~CRingMemPool();

private:
    IMutex* mpMutex;
    ICondition* mpCond;

private:
    TU8 mRequestWrapCount;
    TU8 mReleaseWrapCount;
    TU8 mReserved1;
    TU8 mReserved2;

private:
    TU8* mpMemoryBase;
    TU8* mpMemoryEnd;
    TMemSize mMemoryTotalSize;

    TU8* volatile mpFreeMemStart;
    TU8* volatile mpUsedMemStart;

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
    static IMemPool* Create(const TChar* name, TMemSize size, TUint tot_count);
    virtual CObject* GetObject0() const;
    virtual void Delete();

    virtual TU8* RequestMemBlock(TMemSize size, TU8* start_pointer = NULL);
    virtual void ReturnBackMemBlock(TMemSize size, TU8* start_pointer);
    virtual void ReleaseMemBlock(TMemSize size, TU8* start_pointer);

protected:
    CFixedMemPool(const TChar* name);
    EECode Construct(TMemSize size, TUint tot_count);
    virtual ~CFixedMemPool();

private:
    IMutex* mpMutex;
    ICondition* mpCond;

private:
    TU8* mpMemoryBase;
    TU8* mpMemoryEnd;
    TMemSize mMemoryTotalSize;

    TU8* volatile mpFreeMemStart;
    TU8* volatile mpUsedMemStart;

    TMemSize mMemBlockSize;
    TU32   mnTotalBlockNumber;
    TU32 mnCurrentFreeBlockNumber;

private:
    volatile TU32 mnWaiters;
};

//-----------------------------------------------------------------------
//
//  CISimplePool
//
//-----------------------------------------------------------------------
typedef struct
{
    TU8* p_data;
    TMemSize data_size;
} SDataPiece;

class CISimplePool;

typedef struct
{
    TInt fd;

    TU8 is_closed;
    TU8 has_socket_error;
    TU8 reserved1;
    TU8 reserved2;

    SDataPiece* p_cached_datapiece;

    CIQueue* data_queue;
    IMemPool* ring_pool;
    CISimplePool* data_piece_pool;
} STransferDataChannel;

class CISimplePool
{
public:
    static CISimplePool* Create(TUint max_count);
    void Delete();

public:
    void PrintStatus();

protected:
    CISimplePool();
    EECode Construct(TUint nMaxBuffers);
    virtual ~CISimplePool();

public:
    bool AllocDataPiece(SDataPiece*& pDataPiece, TUint size);
    void ReleaseDataPiece(SDataPiece* pDataPiece);
    TUint GetFreeDataPieceCnt() const;

protected:
    CIQueue *mpBufferQ;

protected:
    TU8 *mpDataPieceStructMemory;
};

class IThreadPool
{
public:
    virtual void Delete() = 0;

protected:
    virtual ~IThreadPool() {}

public:
    virtual EECode StartRunning(TU32 init_thread_number) = 0;
    virtual void StopRunning(TU32 abort = 0) = 0;

public:
    virtual EECode EnqueueCilent(IScheduledClient* client) = 0;

public:
    virtual EECode ResetThreadNumber(TU32 target_thread_number) = 0;
};

extern TU32 gfDateIsLater(SDateTime& current_date, SDateTime& start_date);
extern void gfDateCalculateGap(SDateTime& current_date, SDateTime& start_date, SDateTimeGap& gap);
extern void gfDateCalculateNextTime(SDateTime& next_date, SDateTime& start_date, TU64 seconds);

typedef struct {
    CIClockManager *p_clock_manager;
    CIClockReference* p_clock_reference;
    IClockSource *p_clock_source;
    CIQueue* p_msg_queue;
} SSharedComponent;

class IRunTimeConfigAPI
{
public:
    virtual void Destroy() = 0;

protected:
    virtual ~IRunTimeConfigAPI() {}

public:
    virtual ERunTimeConfigType QueryConfigType() const = 0;

    virtual EECode OpenConfigFile(const TChar* config_file_name, TU8 read_or_write = 0, TU8 read_once = 0, TU8 number = 1) = 0;
    virtual EECode GetContentValue(const TChar *content_path, TChar* content_value, TChar* content_value1 = NULL) = 0;
    virtual EECode SetContentValue(const TChar *content_path, TChar* content_value) = 0;
    virtual EECode SaveConfigFile(const TChar* config_file_name) = 0;
    virtual EECode CloseConfigFile() = 0;

    virtual EECode NewFile(const TChar* root_name, void*& root_node) = 0;
    virtual void* AddContent(void* p_parent, const TChar* content_name) = 0;
    virtual void* AddContentChild(void* p_parent, const TChar* child_name, const TChar* value) = 0;
    virtual EECode FinalizeFile(const TChar* file_name) = 0;
};

IRunTimeConfigAPI* gfRunTimeConfigAPIFactory(ERunTimeConfigType config_type, TU32 max = 512);

class IConfigAPI
{
public:
    virtual ~IConfigAPI() {}

public:
    virtual EECode OpenFile(const TChar* config_file_name, void*& root_node, const TChar*& root_name) = 0;
    virtual EECode NewFile(const TChar* config_file_name, void*& root_node, const TChar* root_name) = 0;
    virtual EECode CloseFile() = 0;

    virtual void* NewNode(const TChar* name) = 0;
    virtual void* NewChild(void* p_parent, const TChar* name, const TChar* value) = 0;
    virtual EECode AttachChild(void* p_parent, void* p_child) = 0;
    virtual EECode NewProperty(void* p_node, const TChar* name, const TChar* value) = 0;

    virtual void* GetRootNode(const TChar*& node_name, const TChar*& value) const = 0;
    virtual void* GetChildNode(void* p_node, const TChar*& node_name, const TChar*& value) const = 0;
    virtual void* GetNextNode(void* p_node, const TChar*& node_name, const TChar*& value) const = 0;

    virtual const TChar* GetProperty(void* p_node, const TChar* property_name) const = 0;
    virtual void FreeProperty(const TChar* property) = 0;

};

IConfigAPI* gfConfigAPIFactory(EConfigType config_type);

//-----------------------------------------------------------------------
//
// IVisualDirectRendering
//
//-----------------------------------------------------------------------
typedef struct
{
    TU8* data[4];
    TU32 linesize[4];

    TDimension off_x, off_y;
    TDimension size_x, size_y;

#ifdef DCONFIG_TEST_END2END_DELAY
    TTime debug_time;
    TU64 debug_count;
#endif

} SVisualDirectRenderingContent;

class IVisualDirectRendering
{
public:
    virtual EECode Render(SVisualDirectRenderingContent* p_context, TU32 reuse) = 0;
    //virtual EECode ControlOutput(TU32 control_flag) = 0;

protected:
    virtual ~IVisualDirectRendering() {}
};

//-----------------------------------------------------------------------
//
// ISoundDirectRendering
//
//-----------------------------------------------------------------------
typedef struct
{
    TU8* data[8];
    TU32 size;
} SSoundDirectRenderingContent;

class ISoundDirectRendering
{
public:
    virtual EECode SetConfig(TU32 samplerate, TU32 framesize, TU32 channel_number, TU32 sample_format) = 0;
    virtual EECode Render(SSoundDirectRenderingContent* p_context, TU32 reuse) = 0;
    virtual EECode ControlOutput(TU32 control_flag) = 0;

protected:
    virtual ~ISoundDirectRendering() {}
};

typedef TU8 TResourceType;
typedef struct {
    TResourceType res;
    TU8 state;

    TU8 reserved0;
    TU8 reserved1;
} SResourceSlot;

class CIResourceAllocator
{
public:
    CIResourceAllocator();
    virtual ~CIResourceAllocator();

public:
    EECode SetLimit(TResourceType max, TResourceType min, TResourceType refresh_threashold);
    TResourceType PreAllocate(TResourceType v);
    EECode Allocate(TResourceType v);
    EECode Release(TResourceType v);
    void Reset();

private:
    void clearPreAllocated();
    void reset();

private:
    TResourceType mMax;
    TResourceType mMin;
    TResourceType mRefreshThreshold;
    TResourceType mCurPreAllocateNumber;

private:
    IMutex* mpMutex;
    TU32 mCurrentBufferLength;
    SResourceSlot* mpSlots;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

