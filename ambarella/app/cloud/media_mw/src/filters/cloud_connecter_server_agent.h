/*******************************************************************************
 * cloud_connecter_server_agent.h
 *
 * History:
 *    2013/12/24 - [Zhi He] create file
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

#ifndef __CLOUD_CONNECTER_SERVER_AGENT_H__
#define __CLOUD_CONNECTER_SERVER_AGENT_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
//  CCloudConnecterServerAgentInputPin
//
//-----------------------------------------------------------------------
class CCloudConnecterServerAgent;

class CCloudConnecterServerAgentInputPin: public CInputPin
{
    typedef CInputPin inherited;

public:
    static CCloudConnecterServerAgentInputPin *Create(const TChar *pname, CCloudConnecterServerAgent *pfilter, StreamType type);
    virtual CObject *GetObject0() const;

public:
    virtual void PrintStatus();

protected:
    CCloudConnecterServerAgentInputPin(const TChar *name, CCloudConnecterServerAgent *filter, StreamType type);
    EECode Construct();
    virtual ~CCloudConnecterServerAgentInputPin();

public:
    virtual void Delete();

public:
    virtual void Receive(CIBuffer *pBuffer);
    virtual void Purge(TU8 disable_pin = 0);

protected:
    CCloudConnecterServerAgent *mpOwnerFilter;

    TU16 mDataSubType;
    TU16 mReserved0;
};


//-----------------------------------------------------------------------
//
// CCloudConnecterServerAgent
//
//-----------------------------------------------------------------------
class CCloudConnecterServerAgent
    : public CObject
    , public IFilter
{
    typedef CObject inherited;

public:
    static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index = 0);

protected:
    CCloudConnecterServerAgent(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index = 0);

    EECode Construct();
    virtual ~CCloudConnecterServerAgent();

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
    EECode SendBuffer(CIBuffer *buffer, TU16 data_type);

public:
    static EECode CmdCallback(void *owner, TU32 cmd_type, TU32 params0, TU32 params1, TU32 params2, TU32 params3, TU32 params4, TU8 *pcmd, TU32 cmdsize);
    static EECode DataCallback(void *owner, TMemSize read_length, TU16 data_type, TU8 extra_flag);

private:
    EECode ProcessCmdCallback(TU32 cmd_type, TU32 params0, TU32 params1, TU32 params2, TU32 params3, TU32 params4, TU8 *pcmd, TU32 cmdsize);
    EECode ProcessDataCallback(TMemSize read_length, TU16 data_type, TU8 extra_flag);

private:
    void fillHeader(TU32 type, TU32 size);

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpEngineMsgSink;
    IMutex *mpMutex;

private:
    IScheduler *mpScheduler;
    ICloudServerAgent *mpAgent;
    EProtocolType mProtocolType;

private:
    TU8 mbRunning;
    TU8 mbStarted;
    TU8 mbSuspended;
    TU8 mbPaused;

private:
    TU8 mbAddedToScheduler;
    TU8 mbEnablePushData;
    TU8 mbEnablePullData;
    TU8 mbUpdateParams;

    TU8 mbEnableVideo;
    TU8 mbEnableAudio;
    TU8 mbEnableSubtitle;
    TU8 mbEnablePrivateData;

private:
    TU16 mCurrentDataType;
    TU8 mbNeedSendSyncBuffer;
    TU8 mCurrentCodecType;

    EBufferType mBufferType;

private:
    TComponentIndex mnTotalOutputPinNumber;
    TComponentIndex mnTotalInputPinNumber;

    COutputPin *mpOutputPins[ECloudConnecterAgentPinNumber];
    CBufferPool *mpBufferPool[ECloudConnecterAgentPinNumber];
    IMemPool *mpMemorypools[ECloudConnecterAgentPinNumber];
    CCloudConnecterServerAgentInputPin *mpInputPins[ECloudConnecterAgentPinNumber];

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

private:
    TU8 mAudioFormat;
    TU8 mAudioChannnelNumber;
    TU8 mAudioParaReserved0;
    TU8 mCurrentExtraFlag;
    TU32 mAudioSampleFrequency;
    TU32 mAudioBitrate;
    TU32 mAudioFrameSize;

private:
    TU8 *mpVideoExtraData;
    TMemSize mVideoExtraDataSize;
    TMemSize mVideoExtraBufferSize;

    TU8 *mpAudioExtraData;
    TMemSize mAudioExtraDataSize;
    TMemSize mAudioExtraBufferSize;

private:
    SSACPHeader mSACPHeader;
    TU32 mSendSeqNumber;

    IFilter *mpRelayTarget;

private:
    TU32 mDebugHeartBeat_Received;
    TU32 mDebugHeartBeat_CommandReceived;
    TU32 mDebugHeartBeat_VideoReceived;
    TU32 mDebugHeartBeat_AudioReceived;

    TU32 mDebugHeartBeat_KeyframeReceived;
    TU32 mDebugHeartBeat_FrameSend;

    TTime mLastDebugTime;
    TTime mFakePTS;
    TTime mAudioFakePTS;

private:
    TU8 *mpDiscardDataBuffer;
    TU32 mDiscardDataBufferSize;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif


