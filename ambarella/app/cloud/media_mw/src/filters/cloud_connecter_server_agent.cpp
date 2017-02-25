/*******************************************************************************
 * cloud_connecter_server_agent.cpp
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

#include "common_config.h"

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "cloud_lib_if.h"
#include "sacp_types.h"

#include "media_mw_if.h"
#include "media_mw_utils.h"
#include "mw_internal.h"
#include "codec_misc.h"

#include "framework_interface.h"
#include "filters_interface.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"
#include "mw_sacp_utils.h"

#include "cloud_connecter_server_agent.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

//#define DDEBUG_PRINT_DATA

static TUint isFlowControl(TU16 sub_type)
{
    if (DUnlikely((ESACPDataChannelSubType_AudioFlowControl_Pause == sub_type) || (ESACPDataChannelSubType_AudioFlowControl_EOS == sub_type))) {
        return 1;
    }

    return 0;
}

static TUint getIndexBufferType(TU16 type, EBufferType &buffer_type, TU8 &customized_type)
{
    switch (type) {

        case ESACPDataChannelSubType_H264_NALU:
            buffer_type = EBufferType_VideoES;
            customized_type = StreamFormat_H264;
            return ECloudConnecterAgentVideoPinIndex;
            break;

        case ESACPDataChannelSubType_AAC:
            customized_type = StreamFormat_AAC;
            buffer_type = EBufferType_AudioES;
            return ECloudConnecterAgentAudioPinIndex;
            break;

        case ESACPDataChannelSubType_MPEG12Audio:
            customized_type = StreamFormat_MPEG12Audio;
            buffer_type = EBufferType_AudioES;
            return ECloudConnecterAgentAudioPinIndex;
            break;

        case ESACPDataChannelSubType_G711_PCMU:
            customized_type = StreamFormat_PCMU,
            buffer_type = EBufferType_AudioES;
            return ECloudConnecterAgentAudioPinIndex;
            break;

        case ESACPDataChannelSubType_G711_PCMA:
            customized_type = StreamFormat_PCMA;
            buffer_type = EBufferType_AudioES;
            return ECloudConnecterAgentAudioPinIndex;
            break;

        case ESACPDataChannelSubType_Cusomized_ADPCM_S16:
            buffer_type = EBufferType_AudioES;
            customized_type = StreamFormat_CustomizedADPCM_1;
            return ECloudConnecterAgentAudioPinIndex;
            break;

        case ESACPDataChannelSubType_RAW_PCM_S16:
            buffer_type = EBufferType_AudioPCMBuffer;
            customized_type = StreamFormat_PCM_S16;
            return ECloudConnecterAgentAudioPinIndex;
            break;

        case ESACPDataChannelSubType_AudioFlowControl_Pause:
        case ESACPDataChannelSubType_AudioFlowControl_EOS:
            buffer_type = EBufferType_FlowControl_Pause;
            customized_type = StreamFormat_AudioFlowControl_Pause;
            return ECloudConnecterAgentAudioPinIndex;
            break;

        default:
            LOG_FATAL("BAD data type %d\n", type);
            break;
    }

    return 0;
}

static TU16 getSACPSubDataType(StreamFormat format)
{
    switch (format) {

        case StreamFormat_AAC:
            return ESACPDataChannelSubType_AAC;
            break;

        case StreamFormat_MPEG12Audio:
            return ESACPDataChannelSubType_MPEG12Audio;
            break;

        case StreamFormat_PCMU:
            return ESACPDataChannelSubType_G711_PCMU;
            break;

        case StreamFormat_PCMA:
            return ESACPDataChannelSubType_G711_PCMA;
            break;

        case StreamFormat_CustomizedADPCM_1:
            return ESACPDataChannelSubType_Cusomized_ADPCM_S16;
            break;

        case StreamFormat_PCM_S16:
            return ESACPDataChannelSubType_RAW_PCM_S16;
            break;

        case StreamFormat_AudioFlowControl_Pause:
            return ESACPDataChannelSubType_AudioFlowControl_Pause;
            break;

        case StreamFormat_AudioFlowControl_EOS:
            return ESACPDataChannelSubType_AudioFlowControl_EOS;
            break;

        default:
            LOG_ERROR("Bad format %d\n", format);
            break;
    }

    return ESACPDataChannelSubType_Invalid;
}

static EECode __release_ring_buffer(CIBuffer *pBuffer)
{
    DASSERT(pBuffer);
    if (pBuffer) {
        if (EBufferCustomizedContentType_RingBuffer == pBuffer->mCustomizedContentType) {
            IMemPool *pool = (IMemPool *)pBuffer->mpCustomizedContent;
            DASSERT(pool);
            if (pool) {
                pool->ReleaseMemBlock((TULong)(pBuffer->GetDataMemorySize()), pBuffer->GetDataPtrBase());
            }
        }
    } else {
        LOG_FATAL("NULL pBuffer!\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

//-----------------------------------------------------------------------
//
//  CCloudConnecterServerAgentInputPin
//
//-----------------------------------------------------------------------

CCloudConnecterServerAgentInputPin *CCloudConnecterServerAgentInputPin::Create(const TChar *pname, CCloudConnecterServerAgent *pfilter, StreamType type)
{
    CCloudConnecterServerAgentInputPin *result = new CCloudConnecterServerAgentInputPin(pname, pfilter, type);
    if (result && result->Construct() != EECode_OK) {
        LOG_FATAL("CCloudConnecterServerAgentInputPin::Construct() fail.\n");
        delete result;
        result = NULL;
    }
    return result;
}

CCloudConnecterServerAgentInputPin::CCloudConnecterServerAgentInputPin(const TChar *name, CCloudConnecterServerAgent *filter, StreamType type)
    : inherited(name, filter, type)
    , mpOwnerFilter(filter)
    , mDataSubType(ESACPDataChannelSubType_Invalid)
{
}

EECode CCloudConnecterServerAgentInputPin::Construct()
{
    return EECode_OK;
}

void CCloudConnecterServerAgentInputPin::PrintStatus()
{
    //LOGM_PRINTF("mPeerSubIndex %d, mbEnable %d\n", mPeerSubIndex, mbEnable);
}

CCloudConnecterServerAgentInputPin::~CCloudConnecterServerAgentInputPin()
{
}

void CCloudConnecterServerAgentInputPin::Delete()
{
    inherited::Delete();
}

void CCloudConnecterServerAgentInputPin::Receive(CIBuffer *pBuffer)
{
    AUTO_LOCK(mpMutex);

    if (!mbEnable) {
        //        LOG_NOTICE("pin disabled\n");
        pBuffer->Release();
        return;
    }

    if (DUnlikely(CIBuffer::SYNC_POINT & pBuffer->GetBufferFlags())) {
        updateSyncBuffer(pBuffer);
        mDataSubType = getSACPSubDataType((StreamFormat) pBuffer->mContentFormat);
    } else if (DUnlikely(ESACPDataChannelSubType_Invalid == mDataSubType)) {
        mDataSubType = getSACPSubDataType((StreamFormat) pBuffer->mContentFormat);
    }

    if (DUnlikely((EBufferType_VideoExtraData == pBuffer->GetBufferType()) || (EBufferType_AudioExtraData == pBuffer->GetBufferType()))) {
        updateExtraData(pBuffer);
    }

    if (mbEnable && mpOwnerFilter) {
        EECode err = mpOwnerFilter->SendBuffer(pBuffer, mDataSubType);
        DASSERT_OK(err);
    }

    pBuffer->Release();

    return;
}

void CCloudConnecterServerAgentInputPin::Purge(TU8 disable_pin)
{
    AUTO_LOCK(mpMutex);

    if (disable_pin) {
        mbEnable = 0;
    }
}


IFilter *gfCreateCloudConnecterServerAgentFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CCloudConnecterServerAgent::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

//-----------------------------------------------------------------------
//
// CCloudConnecterServerAgent
//
//-----------------------------------------------------------------------

IFilter *CCloudConnecterServerAgent::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    CCloudConnecterServerAgent *result = new CCloudConnecterServerAgent(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

CObject *CCloudConnecterServerAgent::GetObject0() const
{
    return (CObject *) this;
}

CCloudConnecterServerAgent::CCloudConnecterServerAgent(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
    : inherited(pName, index)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpEngineMsgSink(pEngineMsgSink)
    , mpScheduler(NULL)
    , mpAgent(NULL)
    , mProtocolType(EProtocolType_SACP)
    , mbRunning(0)
    , mbStarted(0)
    , mbSuspended(0)
    , mbPaused(0)
    , mbAddedToScheduler(0)
    , mbEnablePushData(0)
    , mbEnablePullData(0)
    , mbUpdateParams(1)
    , mbEnableVideo(0)
    , mbEnableAudio(0)
    , mbEnableSubtitle(0)
    , mbEnablePrivateData(0)
    , mCurrentDataType(ESACPDataChannelSubType_Invalid)
    , mbNeedSendSyncBuffer(1)
    , mCurrentCodecType(StreamFormat_CustomizedADPCM_1)
    , mBufferType(EBufferType_Invalid)
    , mnTotalOutputPinNumber(0)
    , mnTotalInputPinNumber(0)
    , mpSourceUrl(NULL)
    , mSourceUrlLength(0)
    , mVideoWidth(1280)
    , mVideoHeight(720)
    , mVideoFramerateNum(DDefaultVideoFramerateNum)
    , mVideoFramerateDen(DDefaultVideoFramerateDen)
    , mVideoFramerate(29.97)
    , mVideoBitrate(10240)
    , mAudioFormat(StreamFormat_CustomizedADPCM_1)
    , mAudioChannnelNumber(1)
    , mCurrentExtraFlag(0)
    , mAudioSampleFrequency(DDefaultAudioSampleRate)
    , mAudioBitrate(48000)
    , mAudioFrameSize(1024)
    , mpVideoExtraData(NULL)
    , mVideoExtraDataSize(0)
    , mVideoExtraBufferSize(0)
    , mpAudioExtraData(NULL)
    , mAudioExtraDataSize(0)
    , mAudioExtraBufferSize(0)
    , mSendSeqNumber(0)
    , mpRelayTarget(NULL)
    , mDebugHeartBeat_Received(0)
    , mDebugHeartBeat_CommandReceived(0)
    , mDebugHeartBeat_VideoReceived(0)
    , mDebugHeartBeat_AudioReceived(0)
    , mDebugHeartBeat_KeyframeReceived(0)
    , mDebugHeartBeat_FrameSend(0)
    , mLastDebugTime(0)
    , mFakePTS(0)
    , mAudioFakePTS(0)
    , mpDiscardDataBuffer(NULL)
    , mDiscardDataBufferSize(0)
{
    TUint j;
    for (j = 0; j < ECloudConnecterAgentPinNumber; j++) {
        mpOutputPins[j] = NULL;
        mpBufferPool[j] = NULL;
        mpMemorypools[j] = NULL;
        mpDataPacketStart[j] = NULL;
        mPacketRemainningSize[j] = 0;
        mPacketSize[j] = 0;

        mpInputPins[j] = NULL;
    }

    mPacketMaxSize[EDemuxerVideoOutputPinIndex] = 256 * 1024;
    mPacketMaxSize[EDemuxerAudioOutputPinIndex] = 16 * 1024;
    mPacketMaxSize[EDemuxerSubtitleOutputPinIndex] = 8 * 1024;
    mPacketMaxSize[EDemuxerPrivateDataOutputPinIndex] = 8 * 1024;
}

EECode CCloudConnecterServerAgent::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleCloudConnecterServerAgent);

    mpScheduler = gfGetNetworkReceiverTCPScheduler(mpPersistMediaConfig, mIndex);

    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOGM_FATAL("gfCreateMutex() fail\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CCloudConnecterServerAgent::~CCloudConnecterServerAgent()
{
    TUint i = 0;
    LOGM_RESOURCE("~CCloudConnecterServerAgent.\n");
    if (mpAgent) {
        mpAgent->Destroy();
        mpAgent = NULL;
    }

    for (i = 0; i < ECloudConnecterAgentPinNumber; i ++) {
        if (mpOutputPins[i]) {
            mpOutputPins[i]->Delete();
            mpOutputPins[i] = NULL;
        }

        if (mpBufferPool[i]) {
            mpBufferPool[i]->Delete();
            mpBufferPool[i] = NULL;
        }

        if (mpMemorypools[i]) {
            mpMemorypools[i]->Delete();
            mpMemorypools[i] = NULL;
        }

        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }
    }

    if (mpVideoExtraData) {
        DDBG_FREE(mpVideoExtraData, "CRVE");
        mpVideoExtraData = NULL;
    }
    if (mpAudioExtraData) {
        DDBG_FREE(mpAudioExtraData, "CRAE");
        mpAudioExtraData = NULL;
    }

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    if (mpDiscardDataBuffer) {
        DDBG_FREE(mpDiscardDataBuffer, "CRDB");
        mpDiscardDataBuffer = NULL;
    }

    LOGM_RESOURCE("~CCloudConnecterServerAgent done.\n");
}

void CCloudConnecterServerAgent::Delete()
{
    TUint i = 0;
    LOGM_RESOURCE("CCloudConnecterServerAgent::Delete(), before mpAgent->Delete().\n");
    if (mpAgent) {
        mpAgent->Destroy();
        mpAgent = NULL;
    }

    for (i = 0; i < ECloudConnecterAgentPinNumber; i ++) {
        if (mpOutputPins[i]) {
            mpOutputPins[i]->Delete();
            mpOutputPins[i] = NULL;
        }

        if (mpBufferPool[i]) {
            mpBufferPool[i]->Delete();
            mpBufferPool[i] = NULL;
        }

        if (mpMemorypools[i]) {
            mpMemorypools[i]->GetObject0()->Delete();
            mpMemorypools[i] = NULL;
        }

        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }
    }

    LOGM_RESOURCE("CCloudConnecterServerAgent::Delete(), before inherited::Delete().\n");
    inherited::Delete();
}

EECode CCloudConnecterServerAgent::Initialize(TChar *p_param)
{
    //AUTO_LOCK(mpMutex);

    if (!p_param) {
        LOGM_INFO("CCloudConnecterServerAgent::Initialize(NULL), not setup cloud connecter server agent at this time\n");
        return EECode_OK;
    }

    return EECode_OK;
}

EECode CCloudConnecterServerAgent::Finalize()
{
    //AUTO_LOCK(mpMutex);

    if (mpAgent && mpScheduler && mbAddedToScheduler) {
        LOGM_INFO("CCloudConnecterServerAgent::Stop(), before RemoveScheduledCilent\n");
        mpScheduler->RemoveScheduledCilent((IScheduledClient *) mpAgent);
        mbAddedToScheduler = 0;
        LOGM_INFO("CCloudConnecterServerAgent::Stop(), after RemoveScheduledCilent\n");
    }

    return EECode_OK;
}

EECode CCloudConnecterServerAgent::Run()
{
    DASSERT(!mbRunning);
    DASSERT(!mbStarted);

    mbRunning = 1;
    return EECode_OK;
}

EECode CCloudConnecterServerAgent::Exit()
{
    return EECode_OK;
}

EECode CCloudConnecterServerAgent::Start()
{
    //AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(!mbStarted);

    if (mpAgent && mpScheduler && (!mbAddedToScheduler)) {
        LOGM_INFO("CCloudConnecterServerAgent::Start(), before AddScheduledCilent\n");
        mpScheduler->AddScheduledCilent((IScheduledClient *) mpAgent);
        mbAddedToScheduler = 1;
        LOGM_INFO("CCloudConnecterServerAgent::Start(), after AddScheduledCilent\n");
    }
    mbStarted = 1;

    return EECode_OK;
}

EECode CCloudConnecterServerAgent::Stop()
{
    //AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    //DASSERT(mbStarted);

    if (mpAgent && mpScheduler && mbAddedToScheduler) {
        LOGM_INFO("CCloudConnecterServerAgent::Stop(), before RemoveScheduledCilent\n");
        mpScheduler->RemoveScheduledCilent((IScheduledClient *) mpAgent);
        mbAddedToScheduler = 0;
        LOGM_INFO("CCloudConnecterServerAgent::Stop(), after RemoveScheduledCilent\n");
    }
    mbStarted = 0;

    return EECode_OK;
}

void CCloudConnecterServerAgent::Pause()
{
    DASSERT(mbRunning);
    DASSERT(mbStarted);

    LOGM_WARN("TO DO\n");

    return;
}

void CCloudConnecterServerAgent::Resume()
{
    //AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(mbStarted);

    LOGM_WARN("TO DO\n");

    return;
}

void CCloudConnecterServerAgent::Flush()
{
    //AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(mbStarted);

    LOGM_WARN("TO DO\n");

    return;
}

void CCloudConnecterServerAgent::ResumeFlush()
{
    //AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(mbStarted);

    LOGM_WARN("TO DO\n");

    return;
}

EECode CCloudConnecterServerAgent::Suspend(TUint release_context)
{
    //AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(mbStarted);

    //send a flow control
    if (mpAgent && mbStarted && mpAgent) {
        TU32 type = DBuildSACPType(0, ESACPTypeCategory_DataChannel, ESACPDataChannelSubType_AudioFlowControl_Pause);
        LOGM_NOTICE("send flow control pause: type 0x%08x\n", type);
        fillHeader(type, 0);
        EECode err = mpAgent->WriteData((TU8 *)&mSACPHeader, sizeof(mSACPHeader));
        DASSERT_OK(err);
        return err;
    }

    return EECode_OK;
}

EECode CCloudConnecterServerAgent::ResumeSuspend(TComponentIndex input_index)
{
    //AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(mbStarted);

    LOGM_WARN("TO DO\n");

    return EECode_OK;
}

EECode CCloudConnecterServerAgent::SwitchInput(TComponentIndex focus_input_index)
{
    //AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(mbStarted);

    LOGM_NOTICE("switch input %d, mnTotalInputPinNumber %d\n", focus_input_index, mnTotalInputPinNumber);

    DASSERT(focus_input_index < mnTotalInputPinNumber);

    if (focus_input_index < mnTotalInputPinNumber) {
        TUint index = 0;
        for (index = 0; index < ECloudConnecterAgentPinNumber; index ++) {
            if (index == focus_input_index) {
                if (mpInputPins[index]) {
                    mpInputPins[index]->Enable(1);
                }
            } else {
                if (mpInputPins[index]) {
                    mpInputPins[index]->Enable(0);
                }
            }
        }
    } else {
        LOGM_ERROR("SwitchInput, input pin index %u invalid, mnCurrentInputPinNumber %u \n", focus_input_index, mnTotalInputPinNumber);
        return EECode_BadParam;
    }

    //send a flow control
    if (mpAgent && mbStarted && mpAgent) {
        TU32 type = DBuildSACPType(0, ESACPTypeCategory_DataChannel, ESACPDataChannelSubType_AudioFlowControl_Pause);
        LOGM_NOTICE("send flow control pause: type 0x%08x\n", type);
        fillHeader(type, 0);
        EECode err = mpAgent->WriteData((TU8 *)&mSACPHeader, sizeof(mSACPHeader));
        DASSERT_OK(err);
        return err;
    }

    LOGM_NOTICE("switch input %d, done\n", focus_input_index);

    return EECode_OK;
}

EECode CCloudConnecterServerAgent::FlowControl(EBufferType flowcontrol_type)
{
    //AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(mbStarted);

    LOGM_WARN("TO DO\n");

    return EECode_NoImplement;
}

EECode CCloudConnecterServerAgent::SendCmd(TUint cmd)
{
    //AUTO_LOCK(mpMutex);

    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

void CCloudConnecterServerAgent::PostMsg(TUint cmd)
{
    //AUTO_LOCK(mpMutex);

    LOGM_FATAL("TO DO\n");
    return;
}

EECode CCloudConnecterServerAgent::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    //AUTO_LOCK(mpMutex);

    //TUint pin_index = 0;
    EECode ret;

    if (StreamType_Video == type) {
        if (!mpOutputPins[ECloudConnecterAgentVideoPinIndex]) {
            mpOutputPins[ECloudConnecterAgentVideoPinIndex] = COutputPin::Create("[cloud connecter video output pin]", (IFilter *) this);
            if (!mpOutputPins[ECloudConnecterAgentVideoPinIndex]) {
                LOGM_FATAL("cloud connecter video output pin::Create() fail\n");
                return EECode_NoMemory;
            }
            DASSERT(!mpBufferPool[ECloudConnecterAgentVideoPinIndex]);
            mpBufferPool[ECloudConnecterAgentVideoPinIndex] = CBufferPool::Create("[cloud connecter buffer pool for video output pin]", 64);
            if (!mpBufferPool[ECloudConnecterAgentVideoPinIndex]) {
                LOGM_FATAL("CBufferPool::Create() fail\n");
                return EECode_NoMemory;
            }
            mpOutputPins[ECloudConnecterAgentVideoPinIndex]->SetBufferPool(mpBufferPool[ECloudConnecterAgentVideoPinIndex]);

            if (mpOutputPins[ECloudConnecterAgentVideoPinIndex] && mpBufferPool[EDemuxerVideoOutputPinIndex]) {
                mpBufferPool[ECloudConnecterAgentVideoPinIndex]->SetReleaseBufferCallBack(__release_ring_buffer);
                LOGM_INFO("before CRingMemPool::Create() for video\n");
                mpMemorypools[ECloudConnecterAgentVideoPinIndex] = CRingMemPool::Create(2 * 1024 * 1024);
            }
            mnTotalOutputPinNumber ++;
        }

        if (mpOutputPins[ECloudConnecterAgentVideoPinIndex]) {
            ret = mpOutputPins[ECloudConnecterAgentVideoPinIndex]->AddSubPin(sub_index);
            DASSERT_OK(ret);

            if (EECode_OK == ret) {
                index = ECloudConnecterAgentVideoPinIndex;
                mbEnableVideo = 1;
                return EECode_OK;
            }
        } else {
            LOGM_FATAL("Why the pin is NULL?\n");
        }
    } else if (StreamType_Audio == type) {
        if (!mpOutputPins[ECloudConnecterAgentAudioPinIndex]) {
            mpOutputPins[ECloudConnecterAgentAudioPinIndex] = COutputPin::Create("[cloud connecter audio output pin]", (IFilter *) this);
            if (!mpOutputPins[ECloudConnecterAgentAudioPinIndex]) {
                LOGM_FATAL("cloud connecter output pin::Create() fail\n");
                return EECode_NoMemory;
            }
            DASSERT(!mpBufferPool[ECloudConnecterAgentAudioPinIndex]);
            mpBufferPool[ECloudConnecterAgentAudioPinIndex] = CBufferPool::Create("[cloud connecter buffer pool for audio output pin]", 64);
            if (!mpBufferPool[ECloudConnecterAgentAudioPinIndex]) {
                LOGM_FATAL("CBufferPool::Create() fail\n");
                return EECode_NoMemory;
            }
            mpOutputPins[ECloudConnecterAgentAudioPinIndex]->SetBufferPool(mpBufferPool[ECloudConnecterAgentAudioPinIndex]);
            if (mpOutputPins[ECloudConnecterAgentAudioPinIndex] && mpBufferPool[ECloudConnecterAgentAudioPinIndex]) {
                mpBufferPool[ECloudConnecterAgentAudioPinIndex]->SetReleaseBufferCallBack(__release_ring_buffer);
                LOGM_INFO("before CRingMemPool::Create() for audio\n");
                mpMemorypools[ECloudConnecterAgentAudioPinIndex] = CRingMemPool::Create(256 * 1024);
            }
            mnTotalOutputPinNumber ++;
        }
        if (mpOutputPins[ECloudConnecterAgentAudioPinIndex]) {
            ret = mpOutputPins[ECloudConnecterAgentAudioPinIndex]->AddSubPin(sub_index);
            DASSERT_OK(ret);

            if (EECode_OK == ret) {
                index = ECloudConnecterAgentAudioPinIndex;
                mbEnableAudio = 1;
                return EECode_OK;
            }
        } else {
            LOGM_FATAL("Why the pin is NULL?\n");
        }
    } else if (StreamType_Subtitle == type) {
        if (!mpOutputPins[ECloudConnecterAgentSubtitlePinIndex]) {
            mpOutputPins[ECloudConnecterAgentSubtitlePinIndex] = COutputPin::Create("[cloud connecter subtitle output pin]", (IFilter *) this);
        }
        if (mpOutputPins[ECloudConnecterAgentSubtitlePinIndex]) {
            ret = mpOutputPins[ECloudConnecterAgentSubtitlePinIndex]->AddSubPin(sub_index);
            DASSERT_OK(ret);

            if (EECode_OK == ret) {
                index = ECloudConnecterAgentSubtitlePinIndex;
                mbEnableSubtitle = 1;
                return EECode_OK;
            }
        } else {
            LOGM_FATAL("Why the pin is NULL?\n");
        }
    } else if (StreamType_PrivateData == type) {
        if (!mpOutputPins[ECloudConnecterAgentPrivateDataPinIndex]) {
            mpOutputPins[ECloudConnecterAgentPrivateDataPinIndex] = COutputPin::Create("[cloud connecter private data output pin]", (IFilter *) this);
        }
        if (mpOutputPins[ECloudConnecterAgentPrivateDataPinIndex]) {
            ret = mpOutputPins[ECloudConnecterAgentPrivateDataPinIndex]->AddSubPin(sub_index);
            DASSERT_OK(ret);

            if (EECode_OK == ret) {
                index = ECloudConnecterAgentPrivateDataPinIndex;
                mbEnablePrivateData = 1;
                return EECode_OK;
            }
        } else {
            LOGM_FATAL("Why the pin is NULL?\n");
        }
    } else {
        LOGM_ERROR("BAD type %d\n", type);
    }

    return EECode_BadParam;
}

EECode CCloudConnecterServerAgent::AddInputPin(TUint &index, TUint type)
{
    //AUTO_LOCK(mpMutex);

    if (mnTotalInputPinNumber >= ECloudConnecterAgentPinNumber) {
        LOGM_ERROR("Max input pin number reached.\n");
        return EECode_InternalLogicalBug;
    }

    index = mnTotalInputPinNumber;
    DASSERT(!mpInputPins[mnTotalInputPinNumber]);
    if (mpInputPins[mnTotalInputPinNumber]) {
        LOGM_FATAL("mpInputPins[mnTotalInputPinNumber] here, must have problems!!! please check it\n");
    }

    mpInputPins[mnTotalInputPinNumber] = CCloudConnecterServerAgentInputPin::Create((const TChar *)"[input pin for CCloudConnecterServerAgent]", this, (StreamType) type);
    DASSERT(mpInputPins[mnTotalInputPinNumber]);
    if (mnTotalInputPinNumber) {
        mpInputPins[mnTotalInputPinNumber]->Enable(0);
    }

    mnTotalInputPinNumber ++;
    return EECode_OK;
}

IOutputPin *CCloudConnecterServerAgent::GetOutputPin(TUint index, TUint sub_index)
{
    //AUTO_LOCK(mpMutex);

    TUint max_sub_index = 0;

    if (ECloudConnecterAgentPinNumber > index) {
        if (mpOutputPins[index]) {
            max_sub_index = mpOutputPins[index]->NumberOfPins();
            if (max_sub_index > sub_index) {
                return mpOutputPins[index];
            } else {
                LOGM_ERROR("BAD sub_index %d, max value %d, in CCloudConnecterServerAgent::GetOutputPin(%u, %u)\n", sub_index, max_sub_index, index, sub_index);
            }
        } else {
            LOGM_FATAL("Why the pointer is NULL? in CCloudConnecterServerAgent::GetOutputPin(%u, %u)\n", index, sub_index);
        }
    } else {
        LOGM_ERROR("BAD index %d, max value %d, in CCloudConnecterServerAgent::GetOutputPin(%u, %u)\n", index, ECloudConnecterAgentPinNumber, index, sub_index);
    }

    return NULL;
}

IInputPin *CCloudConnecterServerAgent::GetInputPin(TUint index)
{
    //AUTO_LOCK(mpMutex);

    if (index >= mnTotalInputPinNumber) {
        LOGM_ERROR("BAD index %d.\n", index);
        return NULL;
    }

    return mpInputPins[index];
}

EECode CCloudConnecterServerAgent::FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
    //AUTO_LOCK(mpMutex);

    EECode ret = EECode_OK;

    switch (property_type) {

        case EFilterPropertyType_update_source_url:
            break;

        case EFilterPropertyType_set_relay_target:
            mpRelayTarget = (IFilter *) p_param;
            break;

        case EFilterPropertyType_assign_cloud_agent: {
                SCloudAgentSetting *uploading_setting = (SCloudAgentSetting *)p_param;
                if (DLikely(uploading_setting)) {
                    DASSERT(uploading_setting->server_agent);
                    if (DLikely(mpScheduler)) {
                        if (DUnlikely(mpAgent && mbAddedToScheduler)) {
                            mpScheduler->RemoveScheduledCilent((IScheduledClient *)mpAgent);
                            mbAddedToScheduler = 0;
                            mpAgent = NULL;
                        }
                        mpAgent = (ICloudServerAgent *)uploading_setting->server_agent;
                        mpAgent->SetProcessCMDCallBack((void *)this, CmdCallback);
                        mpAgent->SetProcessDataCallBack((void *)this, DataCallback);
                        mpScheduler->AddScheduledCilent((IScheduledClient *)mpAgent);
                        mbAddedToScheduler = 1;

                        TUint i = 0;
                        DASSERT(mnTotalInputPinNumber < ECloudConnecterAgentPinNumber);
                        for (i = 0; i < mnTotalInputPinNumber; i ++) {
                            if (DLikely(mpInputPins[i])) {
                                mpInputPins[i]->Enable(1);
                            }
                        }
                    } else {
                        LOGM_FATAL("NULL mpScheduler\n");
                    }
                } else {
                    LOGM_FATAL("NULL p_param\n");
                    ret = EECode_BadParam;
                }
            }
            break;

        case EFilterPropertyType_remove_cloud_agent: {
                if (mpAgent && mpScheduler && mbAddedToScheduler) {
                    mpScheduler->RemoveScheduledCilent((IScheduledClient *)mpAgent);
                    mbAddedToScheduler = 0;
                    mpAgent = NULL;
                }
            }
            break;

#if 0
        case EFilterPropertyType_relay_command: {
                if (mpAgent && p_param) {
                    SRelayData *data = (SRelayData *)p_param;
                    mpAgent->WriteData(data->p_data, data->data_size);
                } else {
                    LOGM_ERROR("NULL mpAgent %p or p_param %p\n", mpAgent, p_param);
                }
            }
            break;
#endif

        default:
            LOGM_FATAL("BAD property 0x%08x\n", property_type);
            ret = EECode_InternalLogicalBug;
            break;
    }

    return ret;
}

EECode CCloudConnecterServerAgent::SendBuffer(CIBuffer *buffer, TU16 data_type)
{
    AUTO_LOCK(mpMutex);

    if (mpAgent && mbStarted && mpAgent) {
        TU32 type = DBuildSACPType(0, ESACPTypeCategory_DataChannel, data_type);
        //LOGM_NOTICE("send: type 0x%08x, payload length %ld\n", type, buffer->GetDataSize());
        fillHeader(type, (TU32)buffer->GetDataSize());
        EECode err = mpAgent->WriteData((TU8 *)&mSACPHeader, sizeof(mSACPHeader));
        DASSERT_OK(err);
        TU8 *pdata = buffer->GetDataPtr();
        TMemSize datasize = buffer->GetDataSize();
        LOGM_INFO("[server agent, write data]: size %ld: %02x %02x %02x %02x %02x %02x %02x %02x\n", datasize, pdata[0], pdata[1], pdata[2], pdata[3], pdata[4], pdata[5], pdata[6], pdata[7]);
        if (DLikely(buffer->GetDataPtr() && buffer->GetDataSize())) {
            err = mpAgent->WriteData(buffer->GetDataPtr(), buffer->GetDataSize());

#if 0
            {
                FILE *p_debug_dump = NULL;
                TChar debug_dump_file_name[128] = {0};

                snprintf(debug_dump_file_name, 127, "agent_write_file_%d", mIndex);
                debug_dump_file_name[127] = 0;

                p_debug_dump = fopen(debug_dump_file_name, "ab");
                if (p_debug_dump) {
                    fwrite(pdata, 1, datasize, p_debug_dump);
                    fclose(p_debug_dump);
                    p_debug_dump = NULL;
                } else {
                    LOGM_ERROR("open %s fail\n", debug_dump_file_name);
                }
            }
#endif

            DASSERT_OK(err);
        } else {
            LOGM_NOTICE("flow control buffer comes, data_type %x\n", data_type);
        }
        return err;
    }

    LOGM_INFO("skip data\n");
    return EECode_OK;
}

void CCloudConnecterServerAgent::GetInfo(INFO &info)
{
    //AUTO_LOCK(mpMutex);

    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = mnTotalInputPinNumber;
    info.nOutput = mnTotalOutputPinNumber;

    info.pName = mpModuleName;
}

void CCloudConnecterServerAgent::PrintStatus()
{
    TUint i = 0;

    if (!(mpPersistMediaConfig->runtime_print_mask & DLOG_MASK_TO_REMOTE)) {
        LOGM_PRINTF("mbAddedToScheduler %d, mpAgent %p\n", mbAddedToScheduler, mpAgent);
        LOGM_PRINTF("Received %d, CommandReceived %d, VideoReceived %d, AudioReceived %d, key frame %d, send frame %d\n", mDebugHeartBeat_Received, mDebugHeartBeat_CommandReceived, mDebugHeartBeat_VideoReceived, mDebugHeartBeat_AudioReceived, mDebugHeartBeat_KeyframeReceived, mDebugHeartBeat_FrameSend);
    }

    if (mpPersistMediaConfig->p_system_clock_reference) {
        TTime curtime = mpPersistMediaConfig->p_system_clock_reference->GetCurrentTime();

        if (curtime > mLastDebugTime) {
            float time_gap = (curtime - mLastDebugTime) / 1000;
            float received_fps = (float)(mDebugHeartBeat_FrameSend * 1000) / time_gap;
            float received_packet_per_second = (float)(mDebugHeartBeat_Received * 1000) / time_gap;
            if (!(mpPersistMediaConfig->runtime_print_mask & DLOG_MASK_TO_REMOTE)) {
                LOGM_PRINTF("last time %lld, curtime %lld, time gap %f ms, fps %f, packet ps %f\n", mLastDebugTime, curtime, time_gap, received_fps, received_packet_per_second);
            }

            CIRemoteLogServer *p_log_server = mpPersistMediaConfig->p_remote_log_server;
            if (DLikely((p_log_server) && (mpPersistMediaConfig->runtime_print_mask & DLOG_MASK_TO_REMOTE))) {
                if (DUnlikely(!mIndex)) {
                    p_log_server->WriteLog("\033[40;35m\033[1m");
                    ((volatile SPersistMediaConfig *)mpPersistMediaConfig)->debug_print_color = 0;
                }
                if (received_fps < 1.0) {
                    if (mbAddedToScheduler) {
                        //p_log_server->WriteLog("\033[40;31m\033[1m [%d]: received %d, key frame %d, send frame %d, time gap %f ms, fps %f, few data comes!\n\033[0m", mIndex, mDebugHeartBeat_Received, mDebugHeartBeat_KeyframeReceived, mDebugHeartBeat_FrameSend, time_gap, received_fps);
                        if (1 == mpPersistMediaConfig->debug_print_color) {
                            p_log_server->WriteLog("[%d]:fr(%d,%d),fps(%2.1f),few!", mIndex, mDebugHeartBeat_FrameSend, mDebugHeartBeat_KeyframeReceived, received_fps);
                        } else {
                            ((volatile SPersistMediaConfig *)mpPersistMediaConfig)->debug_print_color = 1;
                            p_log_server->WriteLog("\033[40;31m\033[1m[%d]:fr(%d,%d),fps(%2.1f),few!", mIndex, mDebugHeartBeat_FrameSend, mDebugHeartBeat_KeyframeReceived, received_fps);
                        }
                    } else {
                        //p_log_server->WriteLog("\033[40;33m\033[1m [%d]: received %d, key frame %d, send frame %d, time gap %f ms, fps %f, not connected\n\033[0m", mIndex, mDebugHeartBeat_Received, mDebugHeartBeat_KeyframeReceived, mDebugHeartBeat_FrameSend, time_gap, received_fps);
                        if (1 == mpPersistMediaConfig->debug_print_color) {
                            p_log_server->WriteLog("[%d]:fr(%d,%d),fps(%2.1f),nocon", mIndex, mDebugHeartBeat_FrameSend, mDebugHeartBeat_KeyframeReceived, received_fps);
                        } else {
                            ((volatile SPersistMediaConfig *)mpPersistMediaConfig)->debug_print_color = 1;
                            p_log_server->WriteLog("\033[40;33m\033[1m[%d]:fr(%d,%d),fps(%2.1f),nocon", mIndex, mDebugHeartBeat_FrameSend, mDebugHeartBeat_KeyframeReceived, received_fps);
                        }
                    }
                } else {
                    if (mbAddedToScheduler) {
                        //p_log_server->WriteLog("\033[40;35m\033[1m [%d]: received %d, key frame %d, send frame %d, time gap %f ms, fps %f\n\033[0m", mIndex, mDebugHeartBeat_Received, mDebugHeartBeat_KeyframeReceived, mDebugHeartBeat_FrameSend, time_gap, received_fps);
                        if (1 == mpPersistMediaConfig->debug_print_color) {
                            ((volatile SPersistMediaConfig *)mpPersistMediaConfig)->debug_print_color = 0;
                            p_log_server->WriteLog("\033[40;35m\033[1m[%d]:fr(%d,%d),fps(%2.1f)\t", mIndex, mDebugHeartBeat_FrameSend, mDebugHeartBeat_KeyframeReceived, received_fps);
                        } else {
                            p_log_server->WriteLog("[%d]:fr(%d,%d),fps(%2.1f)\t", mIndex, mDebugHeartBeat_FrameSend, mDebugHeartBeat_KeyframeReceived, received_fps);
                        }
                    } else {
                        //p_log_server->WriteLog("\033[40;35m\033[1m [%d]: received %d, key frame %d, send frame %d, time gap %f ms, fps %f, not connected now\n\033[0m", mIndex, mDebugHeartBeat_Received, mDebugHeartBeat_KeyframeReceived, mDebugHeartBeat_FrameSend, time_gap, received_fps);
                        if (1 == mpPersistMediaConfig->debug_print_color) {
                            p_log_server->WriteLog("[%d]:fr(%d,%d),fps(%2.1f),nocon", mIndex, mDebugHeartBeat_FrameSend, mDebugHeartBeat_KeyframeReceived, received_fps);
                        } else {
                            ((volatile SPersistMediaConfig *)mpPersistMediaConfig)->debug_print_color = 1;
                            p_log_server->WriteLog("\033[40;35m\033[1m[%d]:fr(%d,%d),fps(%2.1f),nocon", mIndex, mDebugHeartBeat_FrameSend, mDebugHeartBeat_KeyframeReceived, received_fps);
                        }
                    }
                }

                if (!((mIndex + 1) % 5)) {
                    p_log_server->WriteLog("\n");
                } else {
                    p_log_server->WriteLog("\t");
                }
            }

        }
        mLastDebugTime = curtime;
    }

    mDebugHeartBeat_Received = 0;
    mDebugHeartBeat_CommandReceived = 0;
    mDebugHeartBeat_VideoReceived = 0;
    mDebugHeartBeat_AudioReceived = 0;
    mDebugHeartBeat_KeyframeReceived = 0;
    mDebugHeartBeat_FrameSend = 0;

    if (!(mpPersistMediaConfig->runtime_print_mask & DLOG_MASK_TO_REMOTE)) {
        for (i = 0; i < ECloudConnecterAgentPinNumber; i ++) {
            //DASSERT(mpOutputPins[i]);
            if (mpOutputPins[i]) {
                mpOutputPins[i]->PrintStatus();
            }
            if (mpBufferPool[i]) {
                mpBufferPool[i]->PrintStatus();
            }
        }

        for (i = 0; i < mnTotalInputPinNumber; i ++) {
            DASSERT(mpInputPins[i]);
            if (mpInputPins[i]) {
                mpInputPins[i]->PrintStatus();
            }
        }
    }
}

EECode CCloudConnecterServerAgent::CmdCallback(void *owner, TU32 cmd_type, TU32 params0, TU32 params1, TU32 params2, TU32 params3, TU32 params4, TU8 *pcmd, TU32 cmdsize)
{
    CCloudConnecterServerAgent *thiz = (CCloudConnecterServerAgent *)owner;

    if (DLikely(thiz)) {
        return thiz->ProcessCmdCallback(cmd_type, params0, params1, params2, params3, params4, pcmd, cmdsize);
    }

    LOG_FATAL("NULL onwer\n");
    return EECode_BadState;
}

EECode CCloudConnecterServerAgent::DataCallback(void *owner, TMemSize read_length, TU16 data_type, TU8 extra_flag)
{
    CCloudConnecterServerAgent *thiz = (CCloudConnecterServerAgent *)owner;

    if (DLikely(thiz)) {
        return thiz->ProcessDataCallback(read_length, data_type, extra_flag);
    }

    LOG_FATAL("NULL onwer\n");
    return EECode_BadState;
}

EECode CCloudConnecterServerAgent::ProcessCmdCallback(TU32 cmd_type, TU32 params0, TU32 params1, TU32 params2, TU32 params3, TU32 params4, TU8 *pcmd, TU32 cmdsize)
{
    AUTO_LOCK(mpMutex);

    if (DLikely(mpEngineMsgSink)) {

        ECMDType type = gfSACPClientSubCmdTypeToGenericCmdType((TU16)cmd_type);

        if (DUnlikely(ECMDType_CloudSinkClient_UpdateFrameRate == type)) {
            if (DLikely(params0 && (params0 < 256))) {
                mVideoFramerate = params0;
                mVideoFramerateDen = mVideoFramerateNum / params0;
                mbUpdateParams = 1;
                LOGM_NOTICE("[cloud uploader]: set framerate %d\n", params0);
            } else {
                LOGM_ERROR("BAD framerate %d\n", params0);
            }
        } else if (DUnlikely(ECMDType_CloudSinkClient_UpdateResolution == type)) {
            mVideoWidth = params0;
            mVideoHeight = params1;
            mbUpdateParams = 1;
            LOGM_NOTICE("[cloud uploader]: set resolution %dx%d\n", params0, params1);
        } else if (DUnlikely(ECMDType_CloudSourceClient_UpdateAudioParams == type)) {
            if (mbEnableAudio) {
                TU16 tlv16type = 0, tlv16len = 0;
                DBER16(tlv16type, pcmd);
                DBER16(tlv16len, (pcmd + 2));
                DASSERT(ETLV16Type_DeviceAudioParams_Format == tlv16type && 1 == tlv16len);
                mAudioFormat = gfGetStreamingFormatFromSACPSubType((ESACPDataChannelSubType)pcmd[4]);

                DBER16(tlv16type, (pcmd + 5));
                DBER16(tlv16len, (pcmd + 7));
                DASSERT(ETLV16Type_DeviceAudioParams_ChannelNum == tlv16type && 1 == tlv16len);
                mAudioChannnelNumber = (pcmd + 9)[0];

                DBER16(tlv16type, (pcmd + 10));
                DBER16(tlv16len, (pcmd + 12));
                DASSERT(ETLV16Type_DeviceAudioParams_SampleFrequency == tlv16type && 4 == tlv16len);
                DBER32(mAudioSampleFrequency, (pcmd + 14));

                DBER16(tlv16type, (pcmd + 18));
                DBER16(tlv16len, (pcmd + 20));
                DASSERT(ETLV16Type_DeviceAudioParams_Bitrate == tlv16type && 4 == tlv16len);
                DBER32(mAudioBitrate, (pcmd + 22));

                DBER16(tlv16type, (pcmd + 26));
                DBER16(tlv16len, (pcmd + 28));
                DASSERT(ETLV16Type_DeviceAudioParams_ExtraData == tlv16type && tlv16len > 0);
                if (mAudioExtraBufferSize >= (TMemSize)tlv16len) {
                    mAudioExtraDataSize = (TMemSize)tlv16len;
                } else {
                    if (mpAudioExtraData) {
                        DDBG_FREE(mpAudioExtraData, "CRAE");
                        mpAudioExtraData = NULL;
                    }
                    mAudioExtraDataSize = mAudioExtraBufferSize = (TMemSize)tlv16len;
                    mpAudioExtraData = (TU8 *) DDBG_MALLOC(mAudioExtraBufferSize, "CRAE");
                    DASSERT(mpAudioExtraData);
                }
                if (DLikely(mpAudioExtraData)) {
                    memcpy(mpAudioExtraData, pcmd + 30, mAudioExtraDataSize);
                    CIBuffer *p_buffer = NULL;
                    mpOutputPins[EDemuxerAudioOutputPinIndex]->AllocBuffer(p_buffer);
                    p_buffer->SetDataPtr(mpAudioExtraData);
                    p_buffer->SetDataSize(mAudioExtraDataSize);
                    p_buffer->SetDataPtrBase(NULL);
                    p_buffer->SetDataMemorySize(0);
                    p_buffer->SetBufferType(EBufferType_AudioExtraData);
                    p_buffer->mpCustomizedContent = NULL;
                    p_buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;
                    p_buffer->mAudioChannelNumber = mAudioChannnelNumber;
                    p_buffer->mAudioBitrate = mAudioBitrate;
                    p_buffer->mAudioSampleRate = mAudioSampleFrequency;
                    p_buffer->mAudioFrameSize = mAudioFrameSize;
                    //LOGM_ERROR("SendBuffer, %p, %x, %d\n", p_buffer, p_buffer->GetBufferFlags(), p_buffer->GetBufferType());
                    mpOutputPins[EDemuxerAudioOutputPinIndex]->SendBuffer(p_buffer);
                    LOGM_INFO("send audio extradata buffer done(format=0x%x, channum=%d, samplerate=%d, bitrate=%d), mAudioExtraDataSize=%lu, mpAudioExtraData first 2 bytes: 0x%x 0x%x\n",
                              mAudioFormat, mAudioChannnelNumber, mAudioSampleFrequency, mAudioBitrate, mAudioExtraDataSize, mpAudioExtraData[0], mpAudioExtraData[1]);
                    mDebugHeartBeat ++;
                    p_buffer = NULL;
                } else {
                    LOG_FATAL("ECMDType_CloudSourceClient_UpdateAudioParams, mpAudioExtraData NULL.\n");
                }
            } else {
                LOGM_WARN("audio path disabled, ignore ECMDType_CloudSourceClient_UpdateAudioParams.\n");
                //return EECode_InternalLogicalBug;
            }
        }

        SMSG msg;
        mDebugHeartBeat ++;

        memset(&msg, 0x0, sizeof(SMSG));
        msg.code = type;
        msg.owner_index = mIndex;
        msg.owner_type = EGenericComponentType_CloudConnecterServerAgent;

        msg.p_agent_pointer = (TULong)mpAgent;
        msg.p_owner = (TULong)((IFilter *) this);

        msg.p0 = params0;
        msg.p1 = params1;
        msg.p2 = params2;
        msg.p3 = params3;
        msg.p4 = params4;

        mDebugHeartBeat_Received ++;
        mDebugHeartBeat_CommandReceived ++;

        return mpEngineMsgSink->MsgProc(msg);
    }

    LOG_FATAL("NULL msg sink\n");
    return EECode_BadState;
}

EECode CCloudConnecterServerAgent::ProcessDataCallback(TMemSize read_length, TU16 data_type, TU8 flag)
{
    AUTO_LOCK(mpMutex);

    if (DUnlikely(isFlowControl(data_type))) {
        DASSERT(0 == read_length);
        CIBuffer *p_flowcontrol_buffer = NULL;

        LOGM_NOTICE("send flow control buffer to: mpOutputPins[%d] %p\n", ECloudConnecterAgentAudioPinIndex, mpOutputPins[ECloudConnecterAgentAudioPinIndex]);
        mpOutputPins[ECloudConnecterAgentAudioPinIndex]->AllocBuffer(p_flowcontrol_buffer);
        p_flowcontrol_buffer->SetDataPtr(NULL);
        p_flowcontrol_buffer->SetDataSize(0);

        p_flowcontrol_buffer->SetBufferType(EBufferType_FlowControl_Pause);
        p_flowcontrol_buffer->SetBufferFlags(0);
        p_flowcontrol_buffer->mContentFormat = StreamFormat_AudioFlowControl_Pause;

        p_flowcontrol_buffer->mpCustomizedContent = NULL;
        p_flowcontrol_buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;
        p_flowcontrol_buffer->SetDataPtrBase(NULL);
        p_flowcontrol_buffer->SetDataMemorySize(0);
        //LOGM_ERROR("SendBuffer, %p, %x, %d\n", p_flowcontrol_buffer, p_flowcontrol_buffer->GetBufferFlags(), p_flowcontrol_buffer->GetBufferType());
        mpOutputPins[ECloudConnecterAgentAudioPinIndex]->SendBuffer(p_flowcontrol_buffer);
        mDebugHeartBeat_CommandReceived ++;

        return EECode_OK;
    }

    TUint index = getIndexBufferType(data_type, mBufferType, mCurrentCodecType);
    mDebugHeartBeat ++;

    if (DUnlikely((index != ECloudConnecterAgentVideoPinIndex) && (index != ECloudConnecterAgentAudioPinIndex))) {
        LOGM_FATAL("client bugs, invalid data type %d\n", data_type);
        return EECode_BadParam;
    } else if (!mbEnableAudio && (ECloudConnecterAgentAudioPinIndex == index)) {
        LOGM_INFO("discard audio packet since audio path is disabled.\n");
        //discard audio path data
        if (DUnlikely(mDiscardDataBufferSize < read_length)) {
            mDiscardDataBufferSize = read_length + 32;
            if (mpDiscardDataBuffer) {
                DDBG_FREE(mpDiscardDataBuffer, "CRDB");
                mpDiscardDataBuffer = NULL;
            }
            mpDiscardDataBuffer = (TU8 *) DDBG_MALLOC(mDiscardDataBufferSize, "CRDB");
            if (DUnlikely(!mpDiscardDataBuffer)) {
                mDiscardDataBufferSize = 0;
                LOGM_FATAL("no memory, request size %d\n", mDiscardDataBufferSize);
                return EECode_NoMemory;
            }
        }
        mpAgent->ReadData(mpDiscardDataBuffer, read_length);
        return EECode_OK;
    }

    if (DUnlikely(read_length >= mPacketMaxSize[index])) {
        if (DUnlikely(read_length >= (mPacketMaxSize[index] * 2))) {
            if (DUnlikely(read_length >= (mPacketMaxSize[index] * 4))) {
                LOGM_FATAL("too big packet, please check code, %ld, %ld\n", read_length, mPacketMaxSize[index]);
                return EECode_BadParam;
            } else {
                mPacketMaxSize[index] = mPacketMaxSize[index] * 4;
                LOGM_WARN("too big packet, enlarge pre alloc memory block X4, %ld, %ld\n", read_length, mPacketMaxSize[index]);
            }
        } else {
            mPacketMaxSize[index] = mPacketMaxSize[index] * 2;
            LOGM_WARN("too big packet, enlarge pre alloc memory block X2, %ld, %ld\n", read_length, mPacketMaxSize[index]);
        }
    }

    DASSERT(mpBufferPool[index]);
    DASSERT(mpOutputPins[index]);
    DASSERT(mpMemorypools[index]);

    EECode err = EECode_OK;
    CIBuffer *p_buffer = NULL;

    //audio path
    if (DUnlikely(EDemuxerAudioOutputPinIndex == index)) {
        mpDataPacketStart[index] = mpMemorypools[index]->RequestMemBlock(read_length);
        err = mpAgent->ReadData(mpDataPacketStart[index], read_length);
        mDebugHeartBeat_Received ++;
        mDebugHeartBeat_AudioReceived ++;
        //TU8* pdata = mpDataPacketStart[index];
        //TMemSize datasize = read_length;
        //LOGM_PRINTF("[server agent, read data]: size %ld: %02x %02x %02x %02x %02x %02x %02x %02x\n", datasize, pdata[0], pdata[1], pdata[2], pdata[3], pdata[4], pdata[5], pdata[6], pdata[7]);
        if (DUnlikely(EECode_OK != err)) {
            //LOGM_ERROR("try read %ld fail, %s\n", read_length, gfGetErrorCodeString(err));
            mpMemorypools[index]->ReturnBackMemBlock(read_length, mpDataPacketStart[index]);
            mpDataPacketStart[index] = NULL;
            mPacketSize[index] = 0;
            mPacketRemainningSize[index] = 0;
            return err;
        }

        if (mpOutputPins[index]) {
            //LOGM_DEBUG("send data to: mpOutputPins[%d] %p\n", index, mpOutputPins[index]);
            mpOutputPins[index]->AllocBuffer(p_buffer);
            p_buffer->SetDataPtr(mpDataPacketStart[index]);
            p_buffer->SetDataSize(read_length);

            p_buffer->SetBufferType(mBufferType);
            if (DUnlikely(mbNeedSendSyncBuffer)) {
                mbNeedSendSyncBuffer = 0;
                p_buffer->mAudioBitrate = mAudioBitrate;
                p_buffer->mAudioSampleRate = mAudioSampleFrequency;
                p_buffer->mAudioChannelNumber = mAudioChannnelNumber;
                p_buffer->mAudioFrameSize = mAudioFrameSize;
                p_buffer->mContentFormat = mCurrentCodecType;
                p_buffer->SetBufferType(EBufferType_AudioES);
                p_buffer->SetBufferFlags(CIBuffer::KEY_FRAME | CIBuffer::SYNC_POINT);
            } else {
                p_buffer->SetBufferFlags(0);
                p_buffer->mContentFormat = mCurrentCodecType;
            }

            p_buffer->mpCustomizedContent = (void *)mpMemorypools[index];
            p_buffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
            p_buffer->SetDataPtrBase(mpDataPacketStart[index]);
            p_buffer->SetDataMemorySize(read_length);
            mAudioFakePTS += mAudioFrameSize;
            p_buffer->SetBufferLinearPTS(mAudioFakePTS);
            //LOGM_ERROR("SendBuffer, %p, %x, %d\n", p_buffer, p_buffer->GetBufferFlags(), p_buffer->GetBufferType());
            mpOutputPins[index]->SendBuffer(p_buffer);
            //LOGM_DEBUG("send data to: mpOutputPins[%d] %p done\n", index, mpOutputPins[index]);
        } else {
            LOGM_WARN("skip data due to NULL mpOutputPins[%d] %p\n", index, mpOutputPins[index]);
            mpMemorypools[index]->ReturnBackMemBlock(read_length, mpDataPacketStart[index]);
        }
        mPacketSize[index] = 0;
        mpDataPacketStart[index] = NULL;
        mPacketRemainningSize[index] = 0;

        mDebugHeartBeat_Received ++;
        mDebugHeartBeat_AudioReceived ++;

        return EECode_OK;
    }

#ifdef DDEBUG_PRINT_DATA
    LOG_PRINTF("[1] video packet, flag %x, mCurrentExtraFlag %x, length %ld\n", flag, mCurrentExtraFlag, read_length);
#endif

    if (DLikely(flag & DSACPHeaderFlagBit_PacketStartIndicator)) {
        //DASSERT(!mpDataPacketStart[index]);
        if (DUnlikely(mpDataPacketStart[index])) {
            if (DLikely(mPacketSize[index] + mPacketRemainningSize[index])) {
                LOGM_WARN("free exist memory in ring buffer[index %d], %p, size %ld, remaining size %ld\n", index, mpDataPacketStart[index], mPacketSize[index], mPacketRemainningSize[index]);
                mpMemorypools[index]->ReturnBackMemBlock(read_length, mpDataPacketStart[index]);
                mPacketSize[index] = 0;
                mPacketRemainningSize[index] = 0;
            } else {
                LOGM_ERROR("free exist memory in ring buffer[index %d], %p, but size is zero???? size %ld, remaining size %ld\n", index, mpDataPacketStart[index], mPacketSize[index], mPacketRemainningSize[index]);
            }
            mpDataPacketStart[index] = NULL;
        }

        mpDataPacketStart[index] = mpMemorypools[index]->RequestMemBlock(mPacketMaxSize[index]);
        mPacketRemainningSize[index] = mPacketMaxSize[index];
        mPacketSize[index] = 0;

        mCurrentExtraFlag = flag;
    } else {
        if (DUnlikely(NULL == mpDataPacketStart[index])) {
            LOGM_ERROR("NULL mpDataPacketStart[%d] %p, and not packet start, is_first flag wrong?\n", index, mpDataPacketStart[index]);
            return EECode_BadParam;
        }

        if (DUnlikely(mPacketRemainningSize[index] < read_length)) {
            LOGM_ERROR("pre allocated memory not enough? please check code, read_length %ld, mPacketRemainningSize[index] %ld\n", read_length, mPacketRemainningSize[index]);
            return EECode_BadParam;
        }
    }

    //LOGM_PRINTF("[debug flow]: read data, current size %ld, new size %ld, is_first %d, is_end %d\n", mPacketSize[index], read_length, is_first, is_last);
    err = mpAgent->ReadData(mpDataPacketStart[index] + mPacketSize[index], read_length);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("try read %ld fail, %s\n", read_length, gfGetErrorCodeString(err));
        if (DUnlikely(mpDataPacketStart[index])) {
            if (DLikely(mPacketSize[index] + mPacketRemainningSize[index])) {
                LOGM_WARN("free exist memory in ring buffer[index %d], %p, size %ld, remaining size %ld\n", index, mpDataPacketStart[index], mPacketSize[index], mPacketRemainningSize[index]);
                mpMemorypools[index]->ReturnBackMemBlock(read_length, mpDataPacketStart[index]);
                mPacketSize[index] = 0;
                mPacketRemainningSize[index] = 0;
            } else {
                LOGM_ERROR("free exist memory in ring buffer[index %d], %p, but size is zero???? size %ld, remaining size %ld\n", index, mpDataPacketStart[index], mPacketSize[index], mPacketRemainningSize[index]);
            }
            mpDataPacketStart[index] = NULL;
        }
        return err;
    }
    //LOGM_PRINTF("[debug flow]: read data done\n");
    mDebugHeartBeat_Received ++;
    mDebugHeartBeat_VideoReceived ++;

    mPacketSize[index] += read_length;
    mPacketRemainningSize[index] -= read_length;
    //LOGM_NOTICE("read size %ld, accumulated size %ld, flag %02x\n", read_length, mPacketSize[index], flag);

    if (DLikely(flag & DSACPHeaderFlagBit_PacketEndIndicator)) {
        p_buffer = NULL;

        mpMemorypools[index]->ReturnBackMemBlock(mPacketRemainningSize[index], (mpDataPacketStart[index] + mPacketSize[index]));
        mPacketRemainningSize[index] = 0;

        if (DUnlikely(mCurrentExtraFlag & DSACPHeaderFlagBit_PacketExtraDataIndicator)) {
            TU8 has_sps = 0, has_pps = 0, has_idr = 0;
            TU8 *p_sps = NULL, *p_pps = NULL, *p_idr = NULL, *p_pps_end = NULL;
            gfFindH264SpsPpsIdr(mpDataPacketStart[index], mPacketSize[index], has_sps, has_pps, has_idr, p_sps, p_pps, p_pps_end, p_idr);
            DASSERT(has_sps);
            DASSERT(has_pps);
            DASSERT(p_sps);
            DASSERT(p_pps_end);

#ifdef DDEBUG_PRINT_DATA
            LOG_PRINTF("[2] with extra data, size %ld, has_sps %d, has_pps %d, has_idr %d\n", mPacketSize[index], has_sps, has_pps, has_idr);
            gfPrintMemory(mpDataPacketStart[index], 64);
#endif

            if (p_sps && p_pps_end) {
                if (mVideoExtraBufferSize > (TMemSize)(p_pps_end - p_sps)) {
                    mVideoExtraDataSize = (TMemSize)(p_pps_end - p_sps);
                } else {
                    if (mpVideoExtraData) {
                        DDBG_FREE(mpVideoExtraData, "CRVE");
                        mpVideoExtraData = NULL;
                    }
                    mVideoExtraDataSize = mVideoExtraBufferSize = (TMemSize)(p_pps_end - p_sps);
                    mpVideoExtraData = (TU8 *)DDBG_MALLOC(mVideoExtraDataSize, "CRVE");
                    DASSERT(mpVideoExtraData);
                }
                memcpy(mpVideoExtraData, p_sps, mVideoExtraDataSize);

                mpOutputPins[index]->AllocBuffer(p_buffer);
                p_buffer->SetDataPtr(p_sps);
                p_buffer->SetDataSize(mVideoExtraDataSize);
                p_buffer->SetDataPtrBase(NULL);
                p_buffer->SetDataMemorySize(0);
                p_buffer->SetBufferType(EBufferType_VideoExtraData);
                p_buffer->mpCustomizedContent = NULL;
                p_buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;
                //LOGM_ERROR("SendBuffer, %p, %x, %d\n", p_buffer, p_buffer->GetBufferFlags(), p_buffer->GetBufferType());
                mpOutputPins[index]->SendBuffer(p_buffer);
                mDebugHeartBeat ++;
                p_buffer = NULL;

                mpOutputPins[index]->AllocBuffer(p_buffer);
                p_buffer->SetDataPtr(mpDataPacketStart[index]);
                p_buffer->SetDataSize(mPacketSize[index]);

                p_buffer->mpCustomizedContent = (void *)mpMemorypools[index];
                p_buffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
                p_buffer->SetDataPtrBase(mpDataPacketStart[index]);
                p_buffer->SetDataMemorySize(mPacketSize[index]);
                p_buffer->SetBufferType(EBufferType_VideoES);

                LOGM_NOTICE("send extra data\n");
                //LOGM_ERROR("SendBuffer, %p, %x, %d\n", p_buffer, p_buffer->GetBufferFlags(), p_buffer->GetBufferType());
                mpOutputPins[index]->SendBuffer(p_buffer);
                mDebugHeartBeat ++;
                p_buffer = NULL;
            } else {
                LOGM_ERROR("can not find sps pps\n");

                if (DLikely(mpDataPacketStart[index])) {
                    if (DLikely(mPacketSize[index] + mPacketRemainningSize[index])) {
                        LOGM_WARN("free exist memory in ring buffer[index %d], %p, size %ld, remaining size %ld\n", index, mpDataPacketStart[index], mPacketSize[index], mPacketRemainningSize[index]);
                        mpMemorypools[index]->ReturnBackMemBlock(read_length, mpDataPacketStart[index]);
                        mPacketSize[index] = 0;
                        mPacketRemainningSize[index] = 0;
                    } else {
                        LOGM_ERROR("free exist memory in ring buffer[index %d], %p, but size is zero???? size %ld, remaining size %ld\n", index, mpDataPacketStart[index], mPacketSize[index], mPacketRemainningSize[index]);
                    }
                    mpDataPacketStart[index] = NULL;
                }

                mCurrentExtraFlag = 0;
                mPacketSize[index] = 0;
                mpDataPacketStart[index] = NULL;
                mPacketRemainningSize[index] = 0;
                return EECode_DataCorruption;
            }
            mPacketSize[index] = 0;
            mpDataPacketStart[index] = NULL;
            mPacketRemainningSize[index] = 0;
            mCurrentExtraFlag = 0;
            return EECode_OK;
        }

        TU8 *p_check = mpDataPacketStart[index];
        TU8 nal_type = 0;

        if (DUnlikely(p_check[0] || p_check[1])) {
            LOGM_ERROR("no start code? %02x %02x %02x %02x %02x\n", mpDataPacketStart[index][0], mpDataPacketStart[index][1], mpDataPacketStart[index][2], mpDataPacketStart[index][3], mpDataPacketStart[index][4]);
            if (DLikely(mpDataPacketStart[index])) {
                if (DLikely(mPacketSize[index] + mPacketRemainningSize[index])) {
                    LOGM_WARN("free exist memory in ring buffer[index %d], %p, size %ld, remaining size %ld\n", index, mpDataPacketStart[index], mPacketSize[index], mPacketRemainningSize[index]);
                    mpMemorypools[index]->ReturnBackMemBlock(read_length, mpDataPacketStart[index]);
                    mPacketSize[index] = 0;
                    mPacketRemainningSize[index] = 0;
                } else {
                    LOGM_ERROR("free exist memory in ring buffer[index %d], %p, but size is zero???? size %ld, remaining size %ld\n", index, mpDataPacketStart[index], mPacketSize[index], mPacketRemainningSize[index]);
                }
                mpDataPacketStart[index] = NULL;
            }
            return EECode_OK;
        } else {
            if (DLikely(p_check[2] == 0x01)) {
                nal_type = p_check[3] & 0x1f;
            } else if (DLikely(p_check[3] == 0x01)) {
                nal_type = p_check[4] & 0x1f;
            } else {
                LOGM_ERROR("no start code? %02x %02x %02x %02x %02x\n", mpDataPacketStart[index][0], mpDataPacketStart[index][1], mpDataPacketStart[index][2], mpDataPacketStart[index][3], mpDataPacketStart[index][4]);
                if (DLikely(mpDataPacketStart[index])) {
                    if (DLikely(mPacketSize[index] + mPacketRemainningSize[index])) {
                        LOGM_WARN("free exist memory in ring buffer[index %d], %p, size %ld, remaining size %ld\n", index, mpDataPacketStart[index], mPacketSize[index], mPacketRemainningSize[index]);
                        mpMemorypools[index]->ReturnBackMemBlock(read_length, mpDataPacketStart[index]);
                        mPacketSize[index] = 0;
                        mPacketRemainningSize[index] = 0;
                    } else {
                        LOGM_ERROR("free exist memory in ring buffer[index %d], %p, but size is zero???? size %ld, remaining size %ld\n", index, mpDataPacketStart[index], mPacketSize[index], mPacketRemainningSize[index]);
                    }
                    mpDataPacketStart[index] = NULL;
                }
                return EECode_OK;
            }
        }

        if (DUnlikely(ENalType_SPS == nal_type)) {
            TU8 has_sps = 0, has_pps = 0, has_idr = 0;
            TU8 *p_sps = NULL, *p_pps = NULL, *p_idr = NULL, *p_pps_end = NULL;
            gfFindH264SpsPpsIdr(mpDataPacketStart[index], mPacketSize[index], has_sps, has_pps, has_idr, p_sps, p_pps, p_pps_end, p_idr);

#ifdef DDEBUG_PRINT_DATA
            LOG_PRINTF("[3] start with sps, size %ld, has_sps %d, has_pps %d, has_idr %d\n", mPacketSize[index], has_sps, has_pps, has_idr);
            gfPrintMemory(mpDataPacketStart[index], 64);
#endif

            //LOGM_NOTICE("find sps %d, pps %d, idr %d\n", has_sps, has_pps, has_idr);
            has_sps = 1;
            has_pps = 1;
            has_idr = 1;

            if (DUnlikely(has_sps && has_pps)) {
                if (has_idr) {
                    //extradata
                    TMemSize estimate_extra_data_size = (TMemSize)(p_idr - mpDataPacketStart[index]);
                    if (p_pps_end) {
                        estimate_extra_data_size = (TMemSize)(p_pps_end - mpDataPacketStart[index]);
                        mpOutputPins[index]->AllocBuffer(p_buffer);
                        p_buffer->SetDataPtr(mpDataPacketStart[index]);
                        p_buffer->SetDataSize(estimate_extra_data_size);
                        p_buffer->SetBufferType(EBufferType_VideoExtraData);
                        p_buffer->SetBufferFlags(CIBuffer::SYNC_POINT);
                        p_buffer->mVideoWidth = mVideoWidth;
                        p_buffer->mVideoHeight = mVideoHeight;
                        p_buffer->mVideoRoughFrameRate = mVideoFramerate;

                        if ((1 <= mVideoFramerate) && (240 >= mVideoFramerate)) {
                            p_buffer->mVideoFrameRateNum = DDefaultVideoFramerateNum;
                            p_buffer->mVideoFrameRateDen = p_buffer->mVideoFrameRateNum / mVideoFramerate;
                        } else {
                            p_buffer->mVideoFrameRateNum = mVideoFramerateNum;
                            p_buffer->mVideoFrameRateDen = mVideoFramerateDen;
                        }
                        p_buffer->mVideoBitrate = mVideoBitrate;

                        p_buffer->mpCustomizedContent = (void *)mpMemorypools[index];
                        p_buffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
                        p_buffer->SetDataPtrBase(mpDataPacketStart[index]);
                        p_buffer->SetDataMemorySize(estimate_extra_data_size);
                        //LOGM_DEBUG("send extra data\n");
                        //LOGM_ERROR("SendBuffer, %p, %x, %d\n", p_buffer, p_buffer->GetBufferFlags(), p_buffer->GetBufferType());
                        mpOutputPins[index]->SendBuffer(p_buffer);
                    } else {
                        p_pps_end = mpDataPacketStart[index];
                        estimate_extra_data_size = 0;
                    }

                    //debug assert
                    DASSERT(mPacketSize[index] > 256);
                    //idr
                    mpOutputPins[index]->AllocBuffer(p_buffer);
                    p_buffer->SetDataPtr(p_pps_end);
                    p_buffer->SetDataSize(mPacketSize[index] - estimate_extra_data_size);

                    p_buffer->SetBufferType(EBufferType_VideoES);
                    p_buffer->SetBufferFlags(CIBuffer::SYNC_POINT | CIBuffer::KEY_FRAME);
                    p_buffer->mVideoFrameType = EPredefinedPictureType_IDR;

                    p_buffer->mVideoWidth = mVideoWidth;
                    p_buffer->mVideoHeight = mVideoHeight;
                    p_buffer->mVideoRoughFrameRate = mVideoFramerate;
                    if ((1 <= mVideoFramerate) && (240 >= mVideoFramerate)) {
                        p_buffer->mVideoFrameRateNum = DDefaultTimeScale;
                        p_buffer->mVideoFrameRateDen = p_buffer->mVideoFrameRateNum / mVideoFramerate;
                    } else {
                        p_buffer->mVideoFrameRateNum = mVideoFramerateNum;
                        p_buffer->mVideoFrameRateDen = mVideoFramerateDen;
                    }
                    p_buffer->mVideoBitrate = mVideoBitrate;

                    p_buffer->mpCustomizedContent = (void *)mpMemorypools[index];
                    p_buffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
                    p_buffer->SetDataPtrBase(p_pps_end);
                    p_buffer->SetDataMemorySize(mPacketSize[index] - estimate_extra_data_size);
                    mDebugHeartBeat_KeyframeReceived ++;
                    mDebugHeartBeat_FrameSend ++;
                    //LOG_NOTICE("idr with sps, flag %08x\n", p_buffer->GetBufferFlags());

                    mFakePTS += mVideoFramerateDen;
                    p_buffer->SetBufferLinearPTS(mFakePTS);
                    //LOGM_ERROR("SendBuffer, %p, %x, %d\n", p_buffer, p_buffer->GetBufferFlags(), p_buffer->GetBufferType());
                    mpOutputPins[index]->SendBuffer(p_buffer);

                } else {
                    //debug assert
                    DASSERT(mPacketSize[index] <= 256);
                    mpOutputPins[index]->AllocBuffer(p_buffer);
                    p_buffer->SetDataPtr(mpDataPacketStart[index]);
                    p_buffer->SetDataSize(mPacketSize[index]);

                    p_buffer->SetBufferType(EBufferType_VideoExtraData);
                    p_buffer->SetBufferFlags(CIBuffer::SYNC_POINT);

                    p_buffer->mVideoWidth = mVideoWidth;
                    p_buffer->mVideoHeight = mVideoHeight;
                    p_buffer->mVideoRoughFrameRate = mVideoFramerate;
                    if ((1 <= mVideoFramerate) && (240 >= mVideoFramerate)) {
                        p_buffer->mVideoFrameRateNum = DDefaultTimeScale;
                        p_buffer->mVideoFrameRateDen = p_buffer->mVideoFrameRateNum / mVideoFramerate;
                    } else {
                        p_buffer->mVideoFrameRateNum = mVideoFramerateNum;
                        p_buffer->mVideoFrameRateDen = mVideoFramerateDen;
                    }
                    p_buffer->mVideoBitrate = mVideoBitrate;

                    p_buffer->mpCustomizedContent = (void *)mpMemorypools[index];
                    p_buffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
                    p_buffer->SetDataPtrBase(mpDataPacketStart[index]);
                    p_buffer->SetDataMemorySize(mPacketSize[index]);

                    //LOGM_INFO("sps with out sps, flag %08x\n", p_buffer->GetBufferFlags());
                    LOGM_NOTICE("send extra data\n");
                    //LOGM_ERROR("SendBuffer, %p, %x, %d\n", p_buffer, p_buffer->GetBufferFlags(), p_buffer->GetBufferType());
                    mpOutputPins[index]->SendBuffer(p_buffer);
                }
            } else {
                LOGM_ERROR("why not find pps?, size %ld\n", mPacketSize[index]);

                if (DLikely(mpDataPacketStart[index])) {
                    if (DLikely(mPacketSize[index] + mPacketRemainningSize[index])) {
                        LOGM_WARN("free exist memory in ring buffer[index %d], %p, size %ld, remaining size %ld\n", index, mpDataPacketStart[index], mPacketSize[index], mPacketRemainningSize[index]);
                        mpMemorypools[index]->ReturnBackMemBlock(read_length, mpDataPacketStart[index]);
                        mPacketSize[index] = 0;
                        mPacketRemainningSize[index] = 0;
                    } else {
                        LOGM_ERROR("free exist memory in ring buffer[index %d], %p, but size is zero???? size %ld, remaining size %ld\n", index, mpDataPacketStart[index], mPacketSize[index], mPacketRemainningSize[index]);
                    }
                    mpDataPacketStart[index] = NULL;
                }
                return EECode_OK;
            }
            mPacketSize[index] = 0;
            mpDataPacketStart[index] = NULL;
            mPacketRemainningSize[index] = 0;
            return EECode_OK;
        } else if ((ENalType_IDR == nal_type) || (mPacketSize[index] > 4096)) {
            nal_type = ENalType_IDR;
            mpOutputPins[index]->AllocBuffer(p_buffer);
            p_buffer->SetBufferType(EBufferType_VideoES);
            p_buffer->SetBufferFlags(CIBuffer::KEY_FRAME);
            p_buffer->mVideoFrameType = EPredefinedPictureType_IDR;
            mDebugHeartBeat_KeyframeReceived ++;
            mbUpdateParams = 1;
#ifdef DDEBUG_PRINT_DATA
            LOG_PRINTF("[4] start idr, size %ld\n", mPacketSize[index]);
            gfPrintMemory(mpDataPacketStart[index], 64);
#endif
            //LOGM_DEBUG("key frame, flag %08x\n", p_buffer->GetBufferFlags());
        } else {
            mpOutputPins[index]->AllocBuffer(p_buffer);
            p_buffer->SetBufferType(EBufferType_VideoES);
            p_buffer->SetBufferFlags(0);
            p_buffer->mVideoFrameType = EPredefinedPictureType_P;
            //LOGM_DEBUG("non key frame, flag %08x\n", p_buffer->GetBufferFlags());
#ifdef DDEBUG_PRINT_DATA
            LOG_PRINTF("[5] non-idr, size %ld\n", mPacketSize[index]);
            gfPrintMemory(mpDataPacketStart[index], 64);
#endif
        }

        p_buffer->SetDataPtr(mpDataPacketStart[index]);
        p_buffer->SetDataSize(mPacketSize[index]);

        if (DUnlikely(mbUpdateParams)) {
            mbUpdateParams = 0;
            p_buffer->SetBufferFlagBits(CIBuffer::SYNC_POINT);

            p_buffer->mVideoWidth = mVideoWidth;
            p_buffer->mVideoHeight = mVideoHeight;
            p_buffer->mVideoRoughFrameRate = mVideoFramerate;
            if ((1 <= mVideoFramerate) && (256 >= mVideoFramerate)) {
                p_buffer->mVideoFrameRateNum = DDefaultTimeScale;
                p_buffer->mVideoFrameRateDen = p_buffer->mVideoFrameRateNum / mVideoFramerate;
            } else {
                p_buffer->mVideoFrameRateNum = mVideoFramerateNum;
                p_buffer->mVideoFrameRateDen = mVideoFramerateDen;
            }
            p_buffer->mVideoBitrate = mVideoBitrate;

        }
        //LOGM_PRINTF("[debug flow]: uploader receiver, total size %ld, data %02x %02x %02x %02x, %02x %02x %02x %02x, end data %02x %02x %02x %02x\n", mPacketSize[index], mpDataPacketStart[index][0], mpDataPacketStart[index][1], mpDataPacketStart[index][2], mpDataPacketStart[index][3], mpDataPacketStart[index][4], mpDataPacketStart[index][5], mpDataPacketStart[index][6], mpDataPacketStart[index][7], mpDataPacketStart[index][mPacketSize[index] - 4], mpDataPacketStart[index][mPacketSize[index] - 3], mpDataPacketStart[index][mPacketSize[index] - 2], mpDataPacketStart[index][mPacketSize[index] - 1]);

        p_buffer->mpCustomizedContent = (void *)mpMemorypools[index];
        p_buffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
        p_buffer->SetDataPtrBase(mpDataPacketStart[index]);
        p_buffer->SetDataMemorySize(mPacketSize[index]);

        mFakePTS += mVideoFramerateDen;
        p_buffer->SetBufferLinearPTS(mFakePTS);
        //LOGM_ERROR("SendBuffer, %p, %x, %d\n", p_buffer, p_buffer->GetBufferFlags(), p_buffer->GetBufferType());
        mpOutputPins[index]->SendBuffer(p_buffer);

        mPacketSize[index] = 0;
        mpDataPacketStart[index] = NULL;
        mPacketRemainningSize[index] = 0;

        mDebugHeartBeat_FrameSend ++;
    }

    //LOGM_INFO("end: mpDataPacketStart[%d], %p\n", index, mpDataPacketStart[index]);

    return EECode_OK;
}

void CCloudConnecterServerAgent::fillHeader(TU32 type, TU32 size)
{
    mSACPHeader.type_1 = (type >> 24) & 0xff;
    mSACPHeader.type_2 = (type >> 16) & 0xff;
    mSACPHeader.type_3 = (type >> 8) & 0xff;
    mSACPHeader.type_4 = type & 0xff;

    mSACPHeader.size_1 = (size >> 8) & 0xff;
    mSACPHeader.size_2 = size & 0xff;
    mSACPHeader.size_0 = (size >> 16) & 0xff;

    mSACPHeader.seq_count_0 = (mSendSeqNumber >> 16) & 0xff;
    mSACPHeader.seq_count_1 = (mSendSeqNumber >> 8) & 0xff;
    mSACPHeader.seq_count_2 = mSendSeqNumber & 0xff;
    mSendSeqNumber ++;

    mSACPHeader.encryption_type_1 = 0;
    mSACPHeader.encryption_type_2 = 0;

    mSACPHeader.flags = 0;
    mSACPHeader.header_ext_type = EProtocolHeaderExtentionType_NoHeader;
    mSACPHeader.header_ext_size_1 = 0;
    mSACPHeader.header_ext_size_2 = 0;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif

