/*******************************************************************************
 * cloud_connecter_server_agent_v2.h
 *
 * History:
 *    2014/08/25 - [Zhi He] create file
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

#ifndef __CLOUD_CONNECTER_SERVER_AGENT_V2_H__
#define __CLOUD_CONNECTER_SERVER_AGENT_V2_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CCloudConnecterServerAgentV2
//
//-----------------------------------------------------------------------
class CCloudConnecterServerAgentV2
    : public CObject
    , public IFilter
{
    typedef CObject inherited;

public:
    static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index = 0);
    virtual CObject *GetObject0() const;

protected:
    CCloudConnecterServerAgentV2(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index = 0);

    EECode Construct();
    virtual ~CCloudConnecterServerAgentV2();

public:
    virtual void Delete();

public:
    virtual EECode Initialize(TChar *p_param = NULL);
    virtual EECode Finalize();

    virtual EECode Run();
    virtual EECode Exit();

    virtual EECode Start();
    virtual EECode Stop();

    virtual void Pause();
    virtual void Resume();
    virtual void Flush();
    virtual void ResumeFlush();

    virtual EECode Suspend(TUint release_context = 0);
    virtual EECode ResumeSuspend(TComponentIndex input_index = 0);

    virtual EECode SwitchInput(TComponentIndex focus_input_index = 0);

    virtual EECode FlowControl(EBufferType flowcontrol_type);

    virtual EECode SendCmd(TUint cmd);
    virtual void PostMsg(TUint cmd);

    virtual EECode AddOutputPin(TUint &index, TUint &sub_index, TUint type);
    virtual EECode AddInputPin(TUint &index, TUint type);

    virtual IOutputPin *GetOutputPin(TUint index, TUint sub_index = 0);
    virtual IInputPin *GetInputPin(TUint index);

    virtual EECode FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param);

    virtual void GetInfo(INFO &info);
    virtual void PrintStatus();

public:
    void ReleaseMemPiece(void *p);

public:
    static EECode CmdCallback(void *owner, TU32 cmd_type, TU8 *pcmd, TU32 cmdsize);
    static EECode DataCallback(void *owner, TU32 read_length, TU16 data_type, TU32 seq_num, TU8 extra_flag);

private:
    EECode ProcessCmdCallback(TU32 cmd_type, TU8 *pcmd, TU32 cmdsize);
    EECode ProcessDataCallback(TU32 read_length, TU16 data_type, TU32 seq_num, TU8 flag);

private:
    SDataPiece *allocSendBuffer(TMemSize max_buffer_size);
    void releaseSendBuffer(void *buffer);
    EECode discardData(TU32 length);
    void flushPrevious();

private:
    void processRecievePeerSyncVideo(TTime time, TU32 seq_number);
    void processRecievePeerSyncAudio(TTime time, TU32 seq_number);

    void resetVideoSyncContext();
    void resetAudioSyncContext();
    void resetConnectionContext();

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpEngineMsgSink;
    IMutex *mpMutex;
    IMutex *mpResourceMutex;

private:
    IScheduler *mpScheduler;
    ICloudServerAgent *mpAgent;

private:
    TU8 mbRunning;
    TU8 mbStarted;
    TU8 mbSuspended;
    TU8 mbPaused;

private:
    TU8 mbAddedToScheduler;
    TU8 mbUpdateVideoParams;
    TU8 mbUpdateAudioParams;
    TU8 mbVideoWaitFirstKeyFrame;

    TU8 mbEnableVideo;
    TU8 mbEnableAudio;
    TU8 mbEnableSubtitle;
    TU8 mbEnablePrivateData;

    TU32 mTimeCounter;

private:
    TComponentIndex mnTotalOutputPinNumber;
    TU8 mbVideoExtraDataComes;
    TU8 mbAudioExtraDataComes;

    COutputPin *mpOutputPins[ECloudConnecterAgentPinNumber];
    CBufferPool *mpBufferPool[ECloudConnecterAgentPinNumber];
    IMemPool *mpMemorypools[ECloudConnecterAgentPinNumber];

private:
    TU8 *mpDataPacketStart[ECloudConnecterAgentPinNumber];
    TMemSize mPacketMaxSize[ECloudConnecterAgentPinNumber];
    TMemSize mPacketRemainningSize[ECloudConnecterAgentPinNumber];
    TMemSize mPacketSize[ECloudConnecterAgentPinNumber];

private:
    TChar *mpSourceUrl;
    TU32 mSourceUrlLength;

private:
    TU32 mVideoWidth;
    TU32 mVideoHeight;
    TU32 mVideoFramerateNum;
    TU32 mVideoFramerateDen;
    float mVideoFramerate;
    TU32 mVideoBitrate;

    TU32 mVideoGOPM;
    TU32 mVideoGOPN;
    TU32 mVideoGOPIDRInterval;

private:
    TU8 mAudioFormat;
    TU8 mAudioChannnelNumber;
    TU8 mbAudioNeedSendSyncBuffer;
    TU8 mCurrentExtraFlag;
    TU32 mAudioSampleFrequency;
    TU32 mAudioBitrate;
    TU32 mAudioFrameSize;

private:
    TU32 mDebugHeartBeat_Received;
    TU32 mDebugHeartBeat_CommandReceived;
    TU32 mDebugHeartBeat_VideoReceived;
    TU32 mDebugHeartBeat_AudioReceived;

    TU32 mDebugHeartBeat_KeyframeReceived;
    TU32 mDebugHeartBeat_FrameSend;

    TTime mLastDebugTime;

private:
    TU8 *mpDiscardDataBuffer;
    TU32 mDiscardDataBufferSize;

private:
    TU32 mLastVideoSeqNumber32;
    TU32 mLastAudioSeqNumber32;

    TU64 mLastVideoSeqNumber;
    TU64 mLastAudioSeqNumber;
    TTime mLastVideoTime;
    TTime mLastAudioTime;

private:
    CIDoubleLinkedList *mpFreeExtraDataPieceList;

private:
    TU8 mbVideoFirstPacketComes;
    TU8 mbAudioFirstPacketComes;
    TU8 mbSetInitialVideoTime;
    TU8 mbSetInitialAudioTime;

    TU8 mbVideoFirstPeerSyncComes;
    TU8 mbAudioFirstPeerSyncComes;
    TU8 mbVideoFirstLocalSyncComes;
    TU8 mbAudioFirstLocalSyncComes;

    TTime mPeerLastVideoSyncTime;
    TTime mPeerLastAudioSyncTime;

    TTime mLocalLastVideoSyncTime;
    TTime mLocalLastAudioSyncTime;

    TU64 mLastSyncVideoSeqNumber;
    TU64 mLastSyncAudioSeqNumber;

    TTime mLastVideoSyncTime;
    TTime mLastAudioSyncTime;

    TTime mAccumulatedVideoDrift;
    TTime mAccumulatedAudioDrift;

    TTime mVideoDTSIncTicks;
    TTime mAudioDTSIncTicks;

    TTime mVideoDTSOriIncTicks;
    TTime mAudioDTSOriIncTicks;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif


