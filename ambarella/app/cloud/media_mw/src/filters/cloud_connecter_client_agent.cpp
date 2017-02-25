/*******************************************************************************
 * cloud_connecter_agent_agent.cpp
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

#include "common_config.h"

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "media_mw_if.h"
#include "media_mw_utils.h"
#include "mw_internal.h"
#include "codec_misc.h"

#include "framework_interface.h"
#include "filters_interface.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "cloud_lib_if.h"
#include "sacp_types.h"
#include "cloud_connecter_client_agent.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

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
//  CCloudConnecterClientAgentInputPin
//
//-----------------------------------------------------------------------

CCloudConnecterClientAgentInputPin *CCloudConnecterClientAgentInputPin::Create(const TChar *pname, CCloudConnecterClientAgent *pfilter, StreamType type)
{
    CCloudConnecterClientAgentInputPin *result = new CCloudConnecterClientAgentInputPin(pname, pfilter, type);
    if (result && result->Construct() != EECode_OK) {
        LOG_FATAL("CCloudConnecterClientAgentInputPin::Construct() fail.\n");
        delete result;
        result = NULL;
    }
    return result;
}

CCloudConnecterClientAgentInputPin::CCloudConnecterClientAgentInputPin(const TChar *name, CCloudConnecterClientAgent *filter, StreamType type)
    : inherited(name, filter, type)
    , mpOwnerFilter(filter)
    , mDataSubType(ESACPDataChannelSubType_Invalid)
    , mbStartUploading(0)
{
}

EECode CCloudConnecterClientAgentInputPin::Construct()
{
    return EECode_OK;
}

void CCloudConnecterClientAgentInputPin::PrintStatus()
{
    LOGM_PRINTF("mPeerSubIndex %d, mbEnable %d\n", mPeerSubIndex, mbEnable);
}

CCloudConnecterClientAgentInputPin::~CCloudConnecterClientAgentInputPin()
{
}

void CCloudConnecterClientAgentInputPin::Delete()
{
    inherited::Delete();
}

void CCloudConnecterClientAgentInputPin::Receive(CIBuffer *pBuffer)
{
    AUTO_LOCK(mpMutex);

    if (DUnlikely(CIBuffer::SYNC_POINT & pBuffer->GetBufferFlags())) {
        updateSyncBuffer(pBuffer);
        mDataSubType = getSACPSubDataType((StreamFormat) pBuffer->mContentFormat);
    }

    if (DUnlikely((EBufferType_VideoExtraData == pBuffer->GetBufferType()) || (EBufferType_AudioExtraData == pBuffer->GetBufferType()))) {
        updateExtraData(pBuffer);
    } else if (DUnlikely(CIBuffer::WITH_EXTRA_DATA & pBuffer->GetBufferFlags())) {
        TU8 has_sps = 0, has_pps = 0, has_idr = 0;
        TU8 *p_sps = NULL, *p_pps = NULL, *p_idr = NULL, *p_pps_end = NULL;
        gfFindH264SpsPpsIdr(pBuffer->GetDataPtr(), pBuffer->GetDataSize(), has_sps, has_pps, has_idr, p_sps, p_pps, p_pps_end, p_idr);

        if (DLikely(p_sps && p_pps_end)) {
            TMemSize size = (TMemSize)(p_pps_end - p_sps);
            if (mpExtraData && (size > mExtraDataBufferSize)) {
                DDBG_FREE(mpExtraData, "MIEX");
                mpExtraData = NULL;
                mExtraDataBufferSize = 0;
            }

            if (!mpExtraData) {
                mpExtraData = (TU8 *)DDBG_MALLOC(size, "MIEX");
                if (DLikely(mpExtraData)) {
                    mExtraDataBufferSize = size;
                }
            }

            if (mpExtraData) {
                memcpy(mpExtraData, p_sps, size);
                mExtraDataSize = size;
                LOG_INFO("inputPin %p received ExtraData size %ld, first 8 bytes: 0x%02x 0x%02x 0x%02x 0x%02x, 0x%02x 0x%02x 0x%02x 0x%02x\n", this, size, \
                         p_sps[0], p_sps[1], p_sps[2], p_sps[3], \
                         p_sps[4], p_sps[5], p_sps[6], p_sps[7]);
            }
        } else {
            LOGM_ERROR("error\n");
        }
    }

    if (mbEnable && mpOwnerFilter) {
        EECode err = EECode_OK;
        if (!mbStartUploading) {
            if (!(CIBuffer::KEY_FRAME & pBuffer->GetBufferFlags())) {
                return;
            }
            DASSERT(mpExtraData);
            DASSERT(mExtraDataSize);
            //LOG_FATAL("!!!!!! send video extra data\n");
            err = mpOwnerFilter->SendVideoExtraData(mpExtraData, mExtraDataSize, mDataSubType);
            mbStartUploading = 1;
        }
        err = mpOwnerFilter->SendBuffer(pBuffer, mDataSubType);
        DASSERT_OK(err);
    }

    pBuffer->Release();

    return;
}

void CCloudConnecterClientAgentInputPin::Purge(TU8 disable_pin)
{
    AUTO_LOCK(mpMutex);

    if (disable_pin) {
        mbEnable = 0;
    }
}

IFilter *gfCreateCloudConnecterClientAgentFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CCloudConnecterClientAgent::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

//-----------------------------------------------------------------------
//
// CCloudConnecterClientAgent
//
//-----------------------------------------------------------------------

IFilter *CCloudConnecterClientAgent::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    CCloudConnecterClientAgent *result = new CCloudConnecterClientAgent(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

CObject *CCloudConnecterClientAgent::GetObject0() const
{
    return (CObject *) this;
}

CCloudConnecterClientAgent::CCloudConnecterClientAgent(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
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
    , mbConnected2Server(0)
    , mbEnableVideo(0)
    , mbEnableAudio(0)
    , mbEnableSubtitle(0)
    , mbEnablePrivateData(0)
    , mCurrentDataType(ESACPDataChannelSubType_Invalid)
    , mbNeedSendSyncBuffer(1)
    , mCurrentCodecType()
    , mBufferType(EBufferType_Invalid)
    , mLocalPort(DDefaultClientPort)
    , mServerPort(DDefaultSACPServerPort)
    , mnTotalOutputPinNumber(0)
    , mnTotalInputPinNumber(0)
    , mpSourceUrl(NULL)
    , mSourceUrlLength(0)
    , mpRemoteUrl(NULL)
    , mRemoteUrlLength(0)
    , mVideoWidth(320)
    , mVideoHeight(240)
    , mVideoFramerateNum(DDefaultVideoFramerateNum)
    , mVideoFramerateDen(DDefaultVideoFramerateDen)
    , mVideoFramerate(10)
    , mVideoBitrate(10240)
    , mpVideoExtraData(NULL)
    , mVideoExtraDataSize(0)
    , mVideoExtraBufferSize(0)
    , mpAudioExtraData(NULL)
    , mAudioExtraDataSize(0)
    , mAudioExtraBufferSize(0)
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

EECode CCloudConnecterClientAgent::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleCloudConnecterClientAgent);

    mpScheduler = gfGetNetworkReceiverTCPScheduler(mpPersistMediaConfig, mIndex);

    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOGM_FATAL("gfCreateMutex() fail\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CCloudConnecterClientAgent::~CCloudConnecterClientAgent()
{

}

void CCloudConnecterClientAgent::Delete()
{
    TUint i = 0;
    LOGM_RESOURCE("CCloudConnecterClientAgent::Delete(), before mpAgent->Delete().\n");
    if (mpAgent) {
        mpAgent->Delete();
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

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    if (mpSourceUrl) {
        DDBG_FREE(mpSourceUrl, "CCUR");
        mpSourceUrl = NULL;
    }

    if (mpRemoteUrl) {
        DDBG_FREE(mpRemoteUrl, "CCUR");
        mpRemoteUrl = NULL;
    }

    LOGM_RESOURCE("CCloudConnecterClientAgent::Delete(), before inherited::Delete().\n");
    inherited::Delete();
}

void CCloudConnecterClientAgent::Destroy()
{
    Delete();
}

EECode CCloudConnecterClientAgent::Initialize(TChar *p_param)
{
    AUTO_LOCK(mpMutex);

    if (DUnlikely(!mpSourceUrl || !mpRemoteUrl)) {
        LOGM_NOTICE("CCloudConnecterClientAgent::Initialize(NULL), no tag %p or no remote url %p\n", mpSourceUrl, mpRemoteUrl);
        return EECode_BadState;
    }

    if (DUnlikely(mpAgent)) {
        mpAgent->Delete();
        mpAgent = NULL;
    }

    mpAgent = gfCreateCloudClientAgent(mProtocolType, mLocalPort, mServerPort);
    if (DLikely(mpAgent)) {
        TU64 hardware_verification_input = 0;
        LOGM_NOTICE("ConnectToServer(%s), port %d, %d, url %s, length %d\n", mpRemoteUrl, mLocalPort, mServerPort, mpSourceUrl, mSourceUrlLength);
        EECode err = mpAgent->ConnectToServer(mpRemoteUrl, hardware_verification_input, mLocalPort, mServerPort, (TU8 *)mpSourceUrl, (TU16)mSourceUrlLength);
        if (DLikely(EECode_OK == err)) {
            LOGM_NOTICE("connect to server %s, sucess\n", mpRemoteUrl);
            mbConnected2Server = 1;
        } else {
            LOGM_ERROR("connect to server '%s' fail, return %d, %s\n", mpRemoteUrl, err, gfGetErrorCodeString(err));
            return err;
        }
    } else {
        LOGM_FATAL("gfCreateCloudClientAgent() fail\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

EECode CCloudConnecterClientAgent::Finalize()
{
    AUTO_LOCK(mpMutex);

    if (mbConnected2Server && mpAgent) {
        mpAgent->DisconnectToServer();
        mbConnected2Server = 0;
    }

    if (mpAgent) {
        mpAgent->Delete();
        mpAgent = NULL;
    }

    return EECode_OK;
}

EECode CCloudConnecterClientAgent::Run()
{
    DASSERT(!mbRunning);
    DASSERT(!mbStarted);

    mbRunning = 1;
    return EECode_OK;
}

EECode CCloudConnecterClientAgent::Exit()
{
    return EECode_OK;
}

EECode CCloudConnecterClientAgent::Start()
{
    AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(!mbStarted);

    if (mpAgent && mpScheduler && (!mbAddedToScheduler)) {
        LOGM_NOTICE("CCloudConnecterClientAgent::Start(), before AddScheduledCilent\n");
        mpScheduler->AddScheduledCilent((IScheduledClient *) this);
        mbAddedToScheduler = 1;
        LOGM_NOTICE("CCloudConnecterClientAgent::Start(), after AddScheduledCilent\n");
    } else {
        LOGM_ERROR("NULL mpScheduler\n");
        return EECode_InternalLogicalBug;
    }

    mbStarted = 1;

    return EECode_OK;
}

EECode CCloudConnecterClientAgent::Stop()
{
    AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(mbStarted);

    if (mpAgent && mpScheduler && mbAddedToScheduler) {
        LOGM_INFO("CCloudConnecterClientAgent::Stop(), before RemoveScheduledCilent\n");
        mpScheduler->RemoveScheduledCilent((IScheduledClient *) this);
        mbAddedToScheduler = 0;
        LOGM_INFO("CCloudConnecterClientAgent::Stop(), after RemoveScheduledCilent\n");
    }

    mbStarted = 0;

    return EECode_OK;
}

void CCloudConnecterClientAgent::Pause()
{
    DASSERT(mbRunning);
    DASSERT(mbStarted);

    LOGM_WARN("TO DO\n");

    return;
}

void CCloudConnecterClientAgent::Resume()
{
    AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(mbStarted);

    LOGM_WARN("TO DO\n");

    return;
}

void CCloudConnecterClientAgent::Flush()
{
    AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(mbStarted);

    LOGM_WARN("TO DO\n");

    return;
}

void CCloudConnecterClientAgent::ResumeFlush()
{
    AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(mbStarted);

    LOGM_WARN("TO DO\n");

    return;
}

EECode CCloudConnecterClientAgent::Suspend(TUint release_context)
{
    AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(mbStarted);

    LOGM_WARN("TO DO\n");

    return EECode_OK;
}

EECode CCloudConnecterClientAgent::ResumeSuspend(TComponentIndex input_index)
{
    AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(mbStarted);

    LOGM_WARN("TO DO\n");

    return EECode_OK;
}

EECode CCloudConnecterClientAgent::SwitchInput(TComponentIndex focus_input_index)
{
    AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(mbStarted);

    LOGM_WARN("TO DO\n");

    return EECode_NoImplement;
}

EECode CCloudConnecterClientAgent::FlowControl(EBufferType flowcontrol_type)
{
    AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(mbStarted);

    LOGM_WARN("TO DO\n");

    return EECode_NoImplement;
}

EECode CCloudConnecterClientAgent::SendCmd(TUint cmd)
{
    AUTO_LOCK(mpMutex);

    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

void CCloudConnecterClientAgent::PostMsg(TUint cmd)
{
    AUTO_LOCK(mpMutex);

    LOGM_FATAL("TO DO\n");
    return;
}

EECode CCloudConnecterClientAgent::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    AUTO_LOCK(mpMutex);

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

EECode CCloudConnecterClientAgent::AddInputPin(TUint &index, TUint type)
{
    AUTO_LOCK(mpMutex);

    if (mnTotalInputPinNumber >= ECloudConnecterAgentPinNumber) {
        LOGM_ERROR("Max input pin number reached.\n");
        return EECode_InternalLogicalBug;
    }

    index = mnTotalInputPinNumber;
    DASSERT(!mpInputPins[mnTotalInputPinNumber]);
    if (mpInputPins[mnTotalInputPinNumber]) {
        LOGM_FATAL("mpInputPins[mnTotalInputPinNumber] here, must have problems!!! please check it\n");
    }

    mpInputPins[mnTotalInputPinNumber] = CCloudConnecterClientAgentInputPin::Create((const TChar *)"[input pin for CCloudConnecterClientAgent]", this, (StreamType) type);
    DASSERT(mpInputPins[mnTotalInputPinNumber]);

    mnTotalInputPinNumber ++;
    return EECode_OK;
}

IOutputPin *CCloudConnecterClientAgent::GetOutputPin(TUint index, TUint sub_index)
{
    AUTO_LOCK(mpMutex);

    TUint max_sub_index = 0;

    if (EDemuxerOutputPinNumber > index) {
        if (mpOutputPins[index]) {
            max_sub_index = mpOutputPins[index]->NumberOfPins();
            if (max_sub_index > sub_index) {
                return mpOutputPins[index];
            } else {
                LOGM_ERROR("BAD sub_index %d, max value %d, in CCloudConnecterClientAgent::GetOutputPin(%u, %u)\n", sub_index, max_sub_index, index, sub_index);
            }
        } else {
            LOGM_FATAL("Why the pointer is NULL? in CCloudConnecterClientAgent::GetOutputPin(%u, %u)\n", index, sub_index);
        }
    } else {
        LOGM_ERROR("BAD index %d, max value %d, in CCloudConnecterClientAgent::GetOutputPin(%u, %u)\n", index, EDemuxerOutputPinNumber, index, sub_index);
    }

    return NULL;
}

IInputPin *CCloudConnecterClientAgent::GetInputPin(TUint index)
{
    AUTO_LOCK(mpMutex);

    if (mnTotalInputPinNumber >= ECloudConnecterAgentPinNumber) {
        LOGM_ERROR("BAD index %d.\n", index);
        return NULL;
    }

    return mpInputPins[index];
}

EECode CCloudConnecterClientAgent::FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
    AUTO_LOCK(mpMutex);

    EECode ret = EECode_OK;

    switch (property_type) {

        case EFilterPropertyType_update_cloud_tag: {
                TChar *url = (TChar *) p_param;
                DASSERT(!mpSourceUrl);
                if (mpSourceUrl) {
                    DDBG_FREE(mpSourceUrl, "CCUR");
                    mpSourceUrl = NULL;
                }
                mSourceUrlLength = strlen(url) + 1;
                mpSourceUrl = (TChar *) DDBG_MALLOC(mSourceUrlLength, "CCUR");
                memset(mpSourceUrl, 0x0, mSourceUrlLength);
                memcpy(mpSourceUrl, url, mSourceUrlLength - 1);
            }
            break;

        case EFilterPropertyType_update_cloud_remote_url: {
                TChar *url = (TChar *) p_param;
                DASSERT(!mpRemoteUrl);
                if (mpRemoteUrl) {
                    DDBG_FREE(mpRemoteUrl, "CCUR");
                    mpRemoteUrl = NULL;
                }
                mRemoteUrlLength = strlen(url) + 1;
                mpRemoteUrl = (TChar *) DDBG_MALLOC(mRemoteUrlLength, "CCUR");
                memset(mpRemoteUrl, 0x0, mRemoteUrlLength);
                memcpy(mpRemoteUrl, url, mRemoteUrlLength - 1);
            }
            break;

        case EFilterPropertyType_update_cloud_remote_port: {
                mServerPort = (*(TU16 *)p_param);
            }
            break;

        default:
            LOGM_FATAL("BAD property 0x%08x\n", property_type);
            ret = EECode_InternalLogicalBug;
            break;
    }

    return ret;
}

EECode CCloudConnecterClientAgent::SendBuffer(CIBuffer *buffer, TU16 data_type)
{
    AUTO_LOCK(mpMutex);
    DASSERT(buffer);
    EECode err = EECode_OK;

    TU8 *pIn = buffer->GetDataPtr();
    TMemSize size = buffer->GetDataSize();
    if (DUnlikely(pIn == NULL)) {
        LOGM_ERROR("SendBuffer: buffer ptr is NULL!\n");
        return EECode_Error;
    }

    TU8 extra_flag = 0;

    if (buffer->GetBufferFlags() & CIBuffer::KEY_FRAME) {
        extra_flag |= DSACPHeaderFlagBit_PacketKeyFrameIndicator;
    }

    if (DUnlikely(EBufferType_VideoExtraData == buffer->GetBufferType())) {
        extra_flag |= DSACPHeaderFlagBit_PacketExtraDataIndicator;
    } else if (DUnlikely(buffer->GetBufferFlags() & CIBuffer::WITH_EXTRA_DATA)) {
        if ((0x01 == pIn[2]) && (EPredefinedPictureType_IDR == (0x1f & pIn[3]))) {
            extra_flag |= DSACPHeaderFlagBit_PacketExtraDataIndicator;
        }
    }

#if 0
    LOGM_VERBOSE("upload %ld, extra_flag %02x: 0x%02x 0x%02x 0x%02x 0x%02x, 0x%02x 0x%02x 0x%02x 0x%02x, 0x%02x 0x%02x 0x%02x 0x%02x, 0x%02x 0x%02x 0x%02x 0x%02x\n", size, extra_flag, \
                 pIn[0], pIn[1], pIn[2], pIn[3], \
                 pIn[4], pIn[5], pIn[6], pIn[7], \
                 pIn[8], pIn[9], pIn[10], pIn[11], \
                 pIn[12], pIn[13], pIn[14], pIn[15]);
#endif

    err = mpAgent->Uploading(pIn, size, (ESACPDataChannelSubType)data_type, extra_flag);
    if (DUnlikely(err != EECode_OK)) {
        LOGM_ERROR("Uploading  fail, err %d, %s\n", err, gfGetErrorCodeString(err));
    }

    return err;
}

EECode CCloudConnecterClientAgent::SendVideoExtraData(TU8 *p_extradata, TMemSize size, TU16 data_type)
{
    EECode err = EECode_OK;

    LOG_NOTICE("!!!!!! send video extra data\n");

    err = mpAgent->Uploading(p_extradata, size, (ESACPDataChannelSubType)data_type, DSACPHeaderFlagBit_PacketExtraDataIndicator);
    if (DUnlikely(err != EECode_OK)) {
        LOGM_ERROR("Uploading extra data fail, err %d, %s\n", err, gfGetErrorCodeString(err));
    }

    return err;
}

void CCloudConnecterClientAgent::GetInfo(INFO &info)
{
    AUTO_LOCK(mpMutex);

    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = mnTotalInputPinNumber;
    info.nOutput = mnTotalOutputPinNumber;

    info.pName = mpModuleName;
}

void CCloudConnecterClientAgent::PrintStatus()
{
    TUint i = 0;
    LOGM_PRINTF("mnTotalOutputPinNumber %d, mnTotalInputPinNumber %d\n", mnTotalOutputPinNumber, mnTotalInputPinNumber);

    for (i = 0; i < mnTotalOutputPinNumber; i ++) {
        DASSERT(mpOutputPins[i]);
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

EECode CCloudConnecterClientAgent::Scheduling(TUint times, TU32 inout_mask)
{
    AUTO_LOCK(mpMutex);

    if (DLikely(mpAgent)) {
        TU32 type = 0;
        TU32 payload_size = 0;
        TU16 header_ext_size = 0;

        EECode err = mpAgent->PeekCmd(type, payload_size, header_ext_size);
        if (DLikely(EECode_OK == err)) {
            if (DUnlikely(type & DSACPTypeBit_Request)) {
                LOGM_ERROR("CCloudConnecterClientAgent: TODO\n");
            } else {
                DASSERT(!header_ext_size);
                TU16 data_sub_type = type & DSACPTypeBit_SubTypeMask;
                TUint index = getIndexBufferType(data_sub_type, mBufferType, mCurrentCodecType);
                TMemSize read_length = payload_size;
                mDebugHeartBeat ++;

                if (DUnlikely(mCurrentDataType != data_sub_type)) {
                    LOGM_INFO("data type %d, ori %d\n", data_sub_type, mCurrentDataType);
                    mbNeedSendSyncBuffer = 1;
                    mCurrentDataType = data_sub_type;
                }

                //LOGM_NOTICE("received: type 0x%08x, payload length %d\n", type, payload_size);

                if (DUnlikely(isFlowControl(data_sub_type))) {
                    CIBuffer *p_flowcontrol_buffer = NULL;

                    LOGM_NOTICE("send flow control buffer to: mpOutputpins[%d] %p\n", ECloudConnecterAgentAudioPinIndex, mpOutputPins[ECloudConnecterAgentAudioPinIndex]);
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
                    mpOutputPins[ECloudConnecterAgentAudioPinIndex]->SendBuffer(p_flowcontrol_buffer);

                    return EECode_OK;
                }

                if (DLikely(ECloudConnecterAgentAudioPinIndex == index)) {

                    if (DUnlikely(read_length >= mPacketMaxSize[ECloudConnecterAgentAudioPinIndex])) {
                        if (DUnlikely(read_length >= (mPacketMaxSize[ECloudConnecterAgentAudioPinIndex] * 2))) {
                            if (DUnlikely(read_length >= (mPacketMaxSize[ECloudConnecterAgentAudioPinIndex] * 4))) {
                                LOGM_FATAL("too big packet, please check code, %ld, %ld\n", read_length, mPacketMaxSize[ECloudConnecterAgentAudioPinIndex]);
                                return EECode_BadParam;
                            } else {
                                mPacketMaxSize[ECloudConnecterAgentAudioPinIndex] = mPacketMaxSize[ECloudConnecterAgentAudioPinIndex] * 4;
                                LOGM_WARN("too big packet, enlarge pre alloc memory block X4, %ld, %ld\n", read_length, mPacketMaxSize[ECloudConnecterAgentAudioPinIndex]);
                            }
                        } else {
                            mPacketMaxSize[ECloudConnecterAgentAudioPinIndex] = mPacketMaxSize[ECloudConnecterAgentAudioPinIndex] * 2;
                            LOGM_WARN("too big packet, enlarge pre alloc memory block X2, %ld, %ld\n", read_length, mPacketMaxSize[ECloudConnecterAgentAudioPinIndex]);
                        }
                    }

                    DASSERT(mpBufferPool[ECloudConnecterAgentAudioPinIndex]);
                    DASSERT(mpOutputPins[ECloudConnecterAgentAudioPinIndex]);
                    DASSERT(mpMemorypools[ECloudConnecterAgentAudioPinIndex]);

                    mpDataPacketStart[ECloudConnecterAgentAudioPinIndex] = mpMemorypools[ECloudConnecterAgentAudioPinIndex]->RequestMemBlock(read_length);
                    if (DLikely(mpDataPacketStart[ECloudConnecterAgentAudioPinIndex])) {
                        err = mpAgent->ReadData(mpDataPacketStart[ECloudConnecterAgentAudioPinIndex], read_length);
                        if (DUnlikely(EECode_OK != err)) {
                            mpMemorypools[ECloudConnecterAgentAudioPinIndex]->ReturnBackMemBlock(read_length, mpDataPacketStart[ECloudConnecterAgentAudioPinIndex]);
                            LOG_ERROR("try read %ld fail, %s\n", read_length, gfGetErrorCodeString(err));
                            return err;
                        }
                    } else {
                        LOGM_FATAL("RequestMemBlock fail, size %ld\n", read_length);
                    }

                    CIBuffer *p_buffer = NULL;
                    mpOutputPins[ECloudConnecterAgentAudioPinIndex]->AllocBuffer(p_buffer);
                    p_buffer->SetDataPtr(mpDataPacketStart[ECloudConnecterAgentAudioPinIndex]);
                    p_buffer->SetDataSize(read_length);

                    p_buffer->SetBufferType(mBufferType);

                    if (DUnlikely(mbNeedSendSyncBuffer)) {
                        mbNeedSendSyncBuffer = 0;
                        p_buffer->mAudioBitrate = 48000;
                        p_buffer->mAudioSampleRate = DDefaultAudioSampleRate;
                        p_buffer->mAudioChannelNumber = DDefaultAudioChannelNumber;
                        p_buffer->mAudioFrameSize = 1024;
                        p_buffer->mContentFormat = mCurrentCodecType;
                        p_buffer->SetBufferFlags(CIBuffer::KEY_FRAME | CIBuffer::SYNC_POINT);
                    } else {
                        p_buffer->SetBufferFlags(0);
                        p_buffer->mContentFormat = mCurrentCodecType;
                    }

                    p_buffer->SetDataPtrBase(mpDataPacketStart[ECloudConnecterAgentAudioPinIndex]);
                    p_buffer->SetDataMemorySize(read_length);
                    mpDataPacketStart[ECloudConnecterAgentAudioPinIndex] = NULL;
                    p_buffer->mpCustomizedContent = mpMemorypools[ECloudConnecterAgentAudioPinIndex];
                    p_buffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;

                    mpOutputPins[ECloudConnecterAgentAudioPinIndex]->SendBuffer(p_buffer);

                } else {
                    LOGM_ERROR("TO DO!\n");
                }
            }
        } else {
            LOGM_ERROR("PeekCmd fail, return %d, %s\n", err, gfGetErrorCodeString(err));
            return EECode_Closed;
        }
    } else {
        LOGM_ERROR("NULL mpAgent\n");
        return EECode_NotRunning;
    }

    return EECode_OK;
}

TInt CCloudConnecterClientAgent::IsPassiveMode() const
{
    return 0;
}

TSchedulingHandle CCloudConnecterClientAgent::GetWaitHandle() const
{
    return (TSchedulingHandle)mpAgent->GetHandle();
}

TU8 CCloudConnecterClientAgent::GetPriority() const
{
    return 1;
}

EECode CCloudConnecterClientAgent::EventHandling(EECode err)
{
    LOGM_WARN("TO DO!\n");
    return EECode_OK;
}

TSchedulingUnit CCloudConnecterClientAgent::HungryScore() const
{
    LOGM_WARN("TO DO!\n");
    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif

