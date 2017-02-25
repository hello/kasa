/*******************************************************************************
 * cloud_connecter_client_agent.h
 *
 * History:
 *    2013/12/28 - [Zhi He] create file
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

#ifndef __CLOUD_CONNECTER_CLIENT_AGENT_H__
#define __CLOUD_CONNECTER_CLIENT_AGENT_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
//  CCloudConnecterClientAgentInputPin
//
//-----------------------------------------------------------------------
class CCloudConnecterClientAgent;

class CCloudConnecterClientAgentInputPin: public CInputPin
{
    typedef CInputPin inherited;

public:
    static CCloudConnecterClientAgentInputPin *Create(const TChar *pname, CCloudConnecterClientAgent *pfilter, StreamType type);
    virtual CObject *GetObject0() const;

public:
    virtual void PrintStatus();

protected:
    CCloudConnecterClientAgentInputPin(const TChar *name, CCloudConnecterClientAgent *filter, StreamType type);
    EECode Construct();
    virtual ~CCloudConnecterClientAgentInputPin();

public:
    virtual void Delete();

public:
    virtual void Receive(CIBuffer *pBuffer);
    virtual void Purge(TU8 disable_pin = 0);

protected:
    CCloudConnecterClientAgent *mpOwnerFilter;
    TU16 mDataSubType;
    TU8 mbStartUploading;
    TU8 mReserved0;
};


//-----------------------------------------------------------------------
//
// CCloudConnecterClientAgent
//
//-----------------------------------------------------------------------
class CCloudConnecterClientAgent
    : public CObject
    , public IFilter
    , public IScheduledClient
{
    typedef CObject inherited;

public:
    static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index = 0);

protected:
    CCloudConnecterClientAgent(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index = 0);

    EECode Construct();
    virtual ~CCloudConnecterClientAgent();

public:
    virtual void Delete();
    virtual void Destroy();

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
    EECode SendVideoExtraData(TU8 *p_extradata, TMemSize size, TU16 data_type);

public:
    virtual EECode Scheduling(TUint times = 1, TU32 inout_mask = 0);

public:
    virtual TInt IsPassiveMode() const;
    virtual TSchedulingHandle GetWaitHandle() const;
    virtual TU8 GetPriority() const;
    virtual EECode EventHandling(EECode err);

public:
    virtual TSchedulingUnit HungryScore() const;

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpEngineMsgSink;
    IMutex *mpMutex;

private:
    IScheduler *mpScheduler;
    ICloudClientAgent *mpAgent;
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
    TU8 mbConnected2Server;

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
    TU16 mLocalPort;
    TU16 mServerPort;

private:
    TComponentIndex mnTotalOutputPinNumber;
    TComponentIndex mnTotalInputPinNumber;

    COutputPin *mpOutputPins[ECloudConnecterAgentPinNumber];
    CBufferPool *mpBufferPool[ECloudConnecterAgentPinNumber];
    IMemPool *mpMemorypools[ECloudConnecterAgentPinNumber];
    CCloudConnecterClientAgentInputPin *mpInputPins[ECloudConnecterAgentPinNumber];

private:
    TU8 *mpDataPacketStart[ECloudConnecterAgentPinNumber];
    TMemSize mPacketMaxSize[ECloudConnecterAgentPinNumber];
    TMemSize mPacketRemainningSize[ECloudConnecterAgentPinNumber];
    TMemSize mPacketSize[ECloudConnecterAgentPinNumber];

private:
    TChar *mpSourceUrl;
    TU32 mSourceUrlLength;
    TChar *mpRemoteUrl;
    TU32 mRemoteUrlLength;

private:
    TU32 mVideoWidth;
    TU32 mVideoHeight;
    TU32 mVideoFramerateNum;
    TU32 mVideoFramerateDen;
    float mVideoFramerate;
    TU32 mVideoBitrate;

private:
    TU8 *mpVideoExtraData;
    TMemSize mVideoExtraDataSize;
    TMemSize mVideoExtraBufferSize;

    TU8 *mpAudioExtraData;
    TMemSize mAudioExtraDataSize;
    TMemSize mAudioExtraBufferSize;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif


