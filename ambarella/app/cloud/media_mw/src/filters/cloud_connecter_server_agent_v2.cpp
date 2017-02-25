/*******************************************************************************
 * cloud_connecter_server_agent_v2.cpp
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

#include "common_config.h"
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
#include "codec_interface.h"
#include "codec_misc.h"

#include "framework_interface.h"
#include "filters_interface.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "mw_sacp_utils.h"

#include "cloud_connecter_server_agent_v2.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

//#define DDEBUG_PRINT_DATA

TU8 *__h264_slice_header(TU8 *p, TU32 size)
{
    size -= 4;

    while (size) {
        if ((0x0 == p[0]) && (0x0 == p[1])) {
            if ((0x0 == p[2]) && (0x1 == p[3]) && ((0x1f & p[4]) < 6)) {
                return p + 4;
            } else if ((0x1 == p[2]) && ((0x1f & p[3]) < 6)) {
                return p + 3;
            }
        }
        p ++;
        size --;
    }

    return NULL;
}

static TUint isFlowControl(TU16 sub_type)
{
    if (DUnlikely((ESACPDataChannelSubType_AudioFlowControl_Pause == sub_type) || (ESACPDataChannelSubType_AudioFlowControl_EOS == sub_type))) {
        return 1;
    }

    return 0;
}

static TU32 __getIndexBufferType(TU16 type, EBufferType &buffer_type, TU8 &customized_type)
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
        } else if (EBufferCustomizedContentType_AllocateByFilter == pBuffer->mCustomizedContentType) {
            CCloudConnecterServerAgentV2 *filter = (CCloudConnecterServerAgentV2 *)pBuffer->mpCustomizedContent;
            if (DLikely(filter)) {
                filter->ReleaseMemPiece((void *) pBuffer->GetDataPtrBase());
            }
        }
    } else {
        LOG_FATAL("NULL pBuffer!\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

#define DMAX_VIDEO_ACCUMULATED_DRIFT 45000
#define DMAX_AUDIO_ACCUMULATED_DRIFT 27000

TU64 getSeqNumber64(TU32 seq_number, TU64 last_seq_number_64)
{
    if (DUnlikely(seq_number < (last_seq_number_64 & 0x00ffffff))) {
        LOG_NOTICE("wrap around %08x, %llx\n", seq_number, last_seq_number_64);
        return (last_seq_number_64 & 0xffffffffff000000LL) + (TU64)(0x1000000) + (TU64)seq_number;
    } else {
        return (last_seq_number_64 & 0xffffffffff000000LL) + (TU64)seq_number;
    }
}

IFilter *gfCreateCloudConnecterServerAgentFilterV2(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CCloudConnecterServerAgentV2::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

//-----------------------------------------------------------------------
//
// CCloudConnecterServerAgentV2
//
//-----------------------------------------------------------------------

IFilter *CCloudConnecterServerAgentV2::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    CCloudConnecterServerAgentV2 *result = new CCloudConnecterServerAgentV2(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

CObject *CCloudConnecterServerAgentV2::GetObject0() const
{
    return (CObject *) this;
}

CCloudConnecterServerAgentV2::CCloudConnecterServerAgentV2(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
    : inherited(pName, index)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpEngineMsgSink(pEngineMsgSink)
    , mpScheduler(NULL)
    , mpAgent(NULL)
    , mbRunning(0)
    , mbStarted(0)
    , mbSuspended(0)
    , mbPaused(0)
    , mbAddedToScheduler(0)
    , mbUpdateVideoParams(1)
    , mbUpdateAudioParams(1)
    , mbVideoWaitFirstKeyFrame(1)
    , mbEnableVideo(0)
    , mbEnableAudio(0)
    , mbEnableSubtitle(0)
    , mbEnablePrivateData(0)
    , mTimeCounter(0)
    , mnTotalOutputPinNumber(0)
    , mbVideoExtraDataComes(0)
    , mbAudioExtraDataComes(0)
    , mpSourceUrl(NULL)
    , mSourceUrlLength(0)
    , mVideoWidth(1280)
    , mVideoHeight(720)
    , mVideoFramerateNum(DDefaultVideoFramerateNum)
    , mVideoFramerateDen(DDefaultVideoFramerateDen)
    , mVideoFramerate(29.97)
    , mVideoBitrate(10240)
    , mVideoGOPM(1)
    , mVideoGOPN(120)
    , mVideoGOPIDRInterval(1)
    , mAudioFormat(StreamFormat_CustomizedADPCM_1)
    , mAudioChannnelNumber(1)
    , mbAudioNeedSendSyncBuffer(1)
    , mCurrentExtraFlag(0)
    , mAudioSampleFrequency(DDefaultAudioSampleRate)
    , mAudioBitrate(48000)
    , mAudioFrameSize(1024)
    , mDebugHeartBeat_Received(0)
    , mDebugHeartBeat_CommandReceived(0)
    , mDebugHeartBeat_VideoReceived(0)
    , mDebugHeartBeat_AudioReceived(0)
    , mDebugHeartBeat_KeyframeReceived(0)
    , mDebugHeartBeat_FrameSend(0)
    , mLastDebugTime(0)
    , mpDiscardDataBuffer(NULL)
    , mDiscardDataBufferSize(0)
    , mpFreeExtraDataPieceList(NULL)
{
    TUint j;
    for (j = 0; j < ECloudConnecterAgentPinNumber; j++) {
        mpOutputPins[j] = NULL;
        mpBufferPool[j] = NULL;
        mpMemorypools[j] = NULL;
        mpDataPacketStart[j] = NULL;
        mPacketRemainningSize[j] = 0;
        mPacketSize[j] = 0;
    }

    mPacketMaxSize[EDemuxerVideoOutputPinIndex] = 256 * 1024;
    mPacketMaxSize[EDemuxerAudioOutputPinIndex] = 16 * 1024;
    mPacketMaxSize[EDemuxerSubtitleOutputPinIndex] = 8 * 1024;
    mPacketMaxSize[EDemuxerPrivateDataOutputPinIndex] = 8 * 1024;

    mbVideoFirstPacketComes = 0;
    mbAudioFirstPacketComes = 0;
    mbSetInitialVideoTime = 0;
    mbSetInitialAudioTime = 0;

    mbVideoFirstPeerSyncComes = 0;
    mbAudioFirstPeerSyncComes = 0;
    mbVideoFirstLocalSyncComes = 0;
    mbAudioFirstLocalSyncComes = 0;

    mLastVideoSeqNumber = 0;
    mLastAudioSeqNumber = 0;

    mLastVideoSeqNumber32 = 0;
    mLastAudioSeqNumber32 = 0;

    mLastVideoTime = 0;
    mLastAudioTime = 0;

    mPeerLastVideoSyncTime = 0;
    mPeerLastAudioSyncTime = 0;
    mLocalLastVideoSyncTime = 0;
    mLocalLastAudioSyncTime = 0;
    mLastSyncVideoSeqNumber = 0;
    mLastSyncAudioSeqNumber = 0;
    mLastVideoSyncTime = 0;
    mLastAudioSyncTime = 0;

    mAccumulatedVideoDrift = 0;
    mAccumulatedAudioDrift = 0;

    mVideoDTSOriIncTicks = mVideoDTSIncTicks = mVideoFramerateDen;
    mAudioDTSOriIncTicks = mAudioDTSIncTicks = (TTime)((TU64)mAudioFrameSize * (TU64)DDefaultTimeScale) / (TTime)mAudioSampleFrequency;
    LOGM_NOTICE("video tick %lld, audio tick %lld\n", mVideoDTSIncTicks, mAudioDTSIncTicks);
}

EECode CCloudConnecterServerAgentV2::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleCloudConnecterServerAgent);
    //mConfigLogLevel = ELogLevel_Info;

    mpScheduler = gfGetNetworkReceiverTCPScheduler(mpPersistMediaConfig, mIndex);

    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOGM_FATAL("gfCreateMutex() fail\n");
        return EECode_NoMemory;
    }

    mpResourceMutex = gfCreateMutex();
    if (DUnlikely(!mpResourceMutex)) {
        LOGM_FATAL("gfCreateMutex() fail\n");
        return EECode_NoMemory;
    }

    mpFreeExtraDataPieceList = new CIDoubleLinkedList();
    if (DUnlikely(!mpFreeExtraDataPieceList)) {
        LOGM_FATAL("new CIDoubleLinkedList() fail\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CCloudConnecterServerAgentV2::~CCloudConnecterServerAgentV2()
{
    TUint i = 0;
    LOGM_RESOURCE("~CCloudConnecterServerAgentV2.\n");
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
    }

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    if (mpDiscardDataBuffer) {
        DDBG_FREE(mpDiscardDataBuffer, "CRDB");
        mpDiscardDataBuffer = NULL;
    }

    LOGM_RESOURCE("~CCloudConnecterServerAgentV2 done.\n");
}

void CCloudConnecterServerAgentV2::Delete()
{
    TUint i = 0;
    LOGM_RESOURCE("CCloudConnecterServerAgentV2::Delete(), before mpAgent->Delete().\n");
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
    }

    LOGM_RESOURCE("CCloudConnecterServerAgentV2::Delete(), before inherited::Delete().\n");
    inherited::Delete();
}

EECode CCloudConnecterServerAgentV2::Initialize(TChar *p_param)
{
    if (!p_param) {
        LOGM_INFO("CCloudConnecterServerAgentV2::Initialize(NULL), not setup cloud connecter server agent at this time\n");
        return EECode_OK;
    }

    return EECode_OK;
}

EECode CCloudConnecterServerAgentV2::Finalize()
{
    if (mpAgent && mpScheduler && mbAddedToScheduler) {
        LOGM_INFO("CCloudConnecterServerAgentV2::Stop(), before RemoveScheduledCilent\n");
        mpScheduler->RemoveScheduledCilent((IScheduledClient *) mpAgent);
        mbAddedToScheduler = 0;
        LOGM_INFO("CCloudConnecterServerAgentV2::Stop(), after RemoveScheduledCilent\n");
    }

    return EECode_OK;
}

EECode CCloudConnecterServerAgentV2::Run()
{
    DASSERT(!mbRunning);
    DASSERT(!mbStarted);

    mbRunning = 1;
    return EECode_OK;
}

EECode CCloudConnecterServerAgentV2::Exit()
{
    return EECode_OK;
}

EECode CCloudConnecterServerAgentV2::Start()
{
    //AUTO_LOCK(mpMutex);

    DASSERT(mbRunning);
    DASSERT(!mbStarted);

    if (mpAgent && mpScheduler && (!mbAddedToScheduler)) {
        LOGM_INFO("CCloudConnecterServerAgentV2::Start(), before AddScheduledCilent\n");
        mpScheduler->AddScheduledCilent((IScheduledClient *) mpAgent);
        mbAddedToScheduler = 1;
        LOGM_INFO("CCloudConnecterServerAgentV2::Start(), after AddScheduledCilent\n");
    }
    mbStarted = 1;

    return EECode_OK;
}

EECode CCloudConnecterServerAgentV2::Stop()
{
    DASSERT(mbRunning);

    if (mpAgent && mpScheduler && mbAddedToScheduler) {
        LOGM_INFO("CCloudConnecterServerAgentV2::Stop(), before RemoveScheduledCilent\n");
        mpScheduler->RemoveScheduledCilent((IScheduledClient *) mpAgent);
        mbAddedToScheduler = 0;
        LOGM_INFO("CCloudConnecterServerAgentV2::Stop(), after RemoveScheduledCilent\n");
    }
    mbStarted = 0;

    return EECode_OK;
}

void CCloudConnecterServerAgentV2::Pause()
{
    LOGM_WARN("TO DO\n");
    return;
}

void CCloudConnecterServerAgentV2::Resume()
{
    LOGM_WARN("TO DO\n");
    return;
}

void CCloudConnecterServerAgentV2::Flush()
{
    LOGM_WARN("TO DO\n");
    return;
}

void CCloudConnecterServerAgentV2::ResumeFlush()
{
    LOGM_WARN("TO DO\n");
    return;
}

EECode CCloudConnecterServerAgentV2::Suspend(TUint release_context)
{
    LOGM_WARN("TO DO\n");
    return EECode_OK;
}

EECode CCloudConnecterServerAgentV2::ResumeSuspend(TComponentIndex input_index)
{
    LOGM_WARN("TO DO\n");
    return EECode_OK;
}

EECode CCloudConnecterServerAgentV2::SwitchInput(TComponentIndex focus_input_index)
{
    LOG_FATAL("must have no input pin\n");
    return EECode_InternalLogicalBug;
}

EECode CCloudConnecterServerAgentV2::FlowControl(EBufferType flowcontrol_type)
{
    LOGM_WARN("TO DO\n");
    return EECode_NoImplement;
}

EECode CCloudConnecterServerAgentV2::SendCmd(TUint cmd)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

void CCloudConnecterServerAgentV2::PostMsg(TUint cmd)
{
    LOGM_FATAL("TO DO\n");
    return;
}

EECode CCloudConnecterServerAgentV2::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
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

EECode CCloudConnecterServerAgentV2::AddInputPin(TUint &index, TUint type)
{
    LOG_FATAL("must have no input pin\n");
    return EECode_InternalLogicalBug;
}

IOutputPin *CCloudConnecterServerAgentV2::GetOutputPin(TUint index, TUint sub_index)
{
    TUint max_sub_index = 0;

    if (ECloudConnecterAgentPinNumber > index) {
        if (mpOutputPins[index]) {
            max_sub_index = mpOutputPins[index]->NumberOfPins();
            if (max_sub_index > sub_index) {
                return mpOutputPins[index];
            } else {
                LOGM_ERROR("BAD sub_index %d, max value %d, in CCloudConnecterServerAgentV2::GetOutputPin(%u, %u)\n", sub_index, max_sub_index, index, sub_index);
            }
        } else {
            LOGM_FATAL("Why the pointer is NULL? in CCloudConnecterServerAgentV2::GetOutputPin(%u, %u)\n", index, sub_index);
        }
    } else {
        LOGM_ERROR("BAD index %d, max value %d, in CCloudConnecterServerAgentV2::GetOutputPin(%u, %u)\n", index, ECloudConnecterAgentPinNumber, index, sub_index);
    }

    return NULL;
}

IInputPin *CCloudConnecterServerAgentV2::GetInputPin(TUint index)
{
    LOG_FATAL("no input pin\n");
    return NULL;
}

EECode CCloudConnecterServerAgentV2::FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
    EECode ret = EECode_OK;

    switch (property_type) {

            //case EFilterPropertyType_update_source_url:
            //    break;

        case EFilterPropertyType_assign_cloud_agent: {
                SCloudAgentSetting *agent_setting = (SCloudAgentSetting *)p_param;
                if (DLikely(agent_setting)) {
                    DASSERT(agent_setting->server_agent);
                    if (DLikely(mpScheduler)) {
                        if (DUnlikely(mpAgent && mbAddedToScheduler)) {
                            mpScheduler->RemoveScheduledCilent((IScheduledClient *)mpAgent);
                            mbAddedToScheduler = 0;
                            mpAgent = NULL;
                        }
                        resetVideoSyncContext();
                        resetAudioSyncContext();

                        mpAgent = (ICloudServerAgent *)agent_setting->server_agent;
                        mpAgent->SetProcessCMDCallBackV2((void *)this, CmdCallback);
                        mpAgent->SetProcessDataCallBackV2((void *)this, DataCallback);
                        mpScheduler->AddScheduledCilent((IScheduledClient *)mpAgent);
                        mbAddedToScheduler = 1;
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

        default:
            LOGM_FATAL("BAD property 0x%08x\n", property_type);
            ret = EECode_InternalLogicalBug;
            break;
    }

    return ret;
}

void CCloudConnecterServerAgentV2::GetInfo(INFO &info)
{
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = 0; //mnTotalInputPinNumber;
    info.nOutput = mnTotalOutputPinNumber;

    info.pName = mpModuleName;
}

void CCloudConnecterServerAgentV2::PrintStatus()
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
    }
}

void CCloudConnecterServerAgentV2::ReleaseMemPiece(void *p)
{
    if (!p) {
        releaseSendBuffer(p);
    }
}


EECode CCloudConnecterServerAgentV2::CmdCallback(void *owner, TU32 cmd_type, TU8 *pcmd, TU32 cmdsize)
{
    CCloudConnecterServerAgentV2 *thiz = (CCloudConnecterServerAgentV2 *)owner;

    if (DLikely(thiz)) {
        return thiz->ProcessCmdCallback(cmd_type, pcmd, cmdsize);
    }

    LOG_FATAL("NULL onwer\n");
    return EECode_BadState;
}

EECode CCloudConnecterServerAgentV2::DataCallback(void *owner, TU32 read_length, TU16 data_type, TU32 seq_num, TU8 extra_flag)
{
    CCloudConnecterServerAgentV2 *thiz = (CCloudConnecterServerAgentV2 *)owner;

    if (DLikely(thiz)) {
        return thiz->ProcessDataCallback(read_length, data_type, seq_num, extra_flag);
    }

    LOG_FATAL("NULL onwer\n");
    return EECode_BadState;
}

EECode CCloudConnecterServerAgentV2::ProcessCmdCallback(TU32 cmd_type, TU8 *pcmd, TU32 cmdsize)
{
    AUTO_LOCK(mpMutex);

    ECMDType type = gfSACPClientSubCmdTypeToGenericCmdType((TU16)cmd_type);

    mDebugHeartBeat_Received ++;
    mDebugHeartBeat_CommandReceived ++;

    if (DUnlikely(ECMDType_CloudSinkClient_UpdateFrameRate == type)) {
        TU16 tlv16type = 0, tlv16len = 0;
        TU32 param = 0, framerate = 0, den = 0;
        DBER16(tlv16type, pcmd);
        DBER16(tlv16len, (pcmd + 2));
        DASSERT(ETLV16Type_DeviceVideoParams_FrameRate == tlv16type);
        DASSERT(4 == tlv16len);
        pcmd += 4;
        DBER32(param, pcmd);
        DParseVideoFrameRate(den, framerate, param);

        if (0 == den) {
            if (DLikely(framerate && (framerate < 256))) {
                mVideoFramerate = framerate;
                mVideoFramerateDen = DDefaultTimeScale / framerate;
                mVideoFramerateNum = DDefaultTimeScale;
                mbUpdateVideoParams = 1;
                LOGM_NOTICE("[cloud uploader]: set integer framerate %d\n", framerate);
            } else {
                LOGM_ERROR("BAD framerate %d\n", framerate);
            }
        } else {
            mVideoFramerateDen = den;
            mVideoFramerateNum = DDefaultTimeScale;
            mVideoFramerate = (float)mVideoFramerateNum / (float)mVideoFramerateDen;
            mbUpdateVideoParams = 1;
            LOGM_NOTICE("[cloud uploader]: set fra framerate %f, den %d\n", mVideoFramerate, den);
            mVideoDTSOriIncTicks = mVideoDTSIncTicks = mVideoFramerateDen;
        }

        return EECode_OK;
    } else if (DUnlikely(ECMDType_CloudSinkClient_UpdateResolution == type)) {
        TU16 tlv16type = 0, tlv16len = 0;
        DBER16(tlv16type, pcmd);
        DBER16(tlv16len, (pcmd + 2));
        DASSERT(ETLV16Type_DeviceVideoParams_Resolution == tlv16type);
        DASSERT(8 == tlv16len);
        pcmd += 4;
        DBER32(mVideoWidth, pcmd);
        pcmd += 4;
        DBER32(mVideoHeight, pcmd);
        mbUpdateVideoParams = 1;
        LOGM_NOTICE("[cloud uploader]: set resolution %dx%d\n", mVideoWidth, mVideoHeight);
        return EECode_OK;
    } else if (DUnlikely(ECMDType_CloudSourceClient_UpdateVideoParams == type)) {
        TU16 tlv16type = 0, tlv16len = 0;
        TU8 *ptmp = pcmd;
        TInt length = cmdsize;
        DASSERT(length > 4);

        DBER16(tlv16type, ptmp);
        ptmp += 2;
        DBER16(tlv16len, (ptmp));
        ptmp += 2;
        DASSERT(ETLV16Type_DeviceVideoParams_Format == tlv16type);
        DASSERT(1 == tlv16len);
        //mAudioFormat = gfGetStreamingFormatFromSACPSubType((ESACPDataChannelSubType)ptmp[0]);
        ptmp ++;
        length -= 5;

        if (length < 5) {
            LOG_ERROR("end here\n");
            return EECode_ServerReject_CorruptedProtocol;
        }

        DBER16(tlv16type, ptmp);
        ptmp += 2;
        DBER16(tlv16len, (ptmp));
        ptmp += 2;
        length -= 4;

        if (ETLV16Type_DeviceVideoParams_FrameRate == tlv16type) {
            TU32 framerate = 0, param = 0, den = 0;
            if (length < 5) {
                LOG_ERROR("end here\n");
                return EECode_ServerReject_CorruptedProtocol;
            }
            DBER32(param, ptmp);
            DParseVideoFrameRate(den, framerate, param);

            if (0 == den) {
                if (DLikely(framerate && (framerate < 256))) {
                    mVideoFramerate = (float)framerate;
                    mVideoFramerateDen = DDefaultTimeScale / framerate;
                    mVideoFramerateNum = DDefaultTimeScale;
                    mbUpdateVideoParams = 1;
                    LOGM_NOTICE("[cloud uploader]: set integer framerate %d\n", framerate);
                } else {
                    LOGM_ERROR("BAD framerate %d\n", framerate);
                }
            } else {
                mVideoFramerateDen = den;
                mVideoFramerateNum = DDefaultTimeScale;
                mVideoFramerate = (float)mVideoFramerateNum / (float)mVideoFramerateDen;
                mbUpdateVideoParams = 1;
                LOGM_NOTICE("[cloud uploader]: set fra framerate %f, den %d\n", mVideoFramerate, den);
            }

            length -= 4;
            if (length < 5) {
                LOG_ERROR("end here\n");
                return EECode_ServerReject_CorruptedProtocol;
            }
            ptmp += 4;

            DBER16(tlv16type, ptmp);
            ptmp += 2;
            DBER16(tlv16len, (ptmp));
            ptmp += 2;
            length -= 4;
        }

        if (ETLV16Type_DeviceVideoParams_Resolution == tlv16type) {
            if (length < 9) {
                LOG_ERROR("end here\n");
                return EECode_ServerReject_CorruptedProtocol;
            }
            DBER32(mVideoWidth, ptmp);
            ptmp += 4;
            DBER32(mVideoHeight, ptmp);
            ptmp += 4;
            mbUpdateVideoParams = 1;

            length -= 8;
            if (length < 5) {
                LOG_ERROR("end here\n");
                return EECode_ServerReject_CorruptedProtocol;
            }

            DBER16(tlv16type, ptmp);
            ptmp += 2;
            DBER16(tlv16len, (ptmp));
            ptmp += 2;
            length -= 4;
        }

        if (ETLV16Type_DeviceVideoParams_Bitrate == tlv16type) {
            if (length < 4) {
                LOG_ERROR("end here\n");
                return EECode_ServerReject_CorruptedProtocol;
            }
            DBER32(mVideoBitrate, ptmp);
            length -= 4;
            ptmp += 4;
            mbUpdateVideoParams = 1;

            if (length < 4) {
                LOG_ERROR("end here\n");
                return EECode_OK;
            }

            DBER16(tlv16type, ptmp);
            ptmp += 2;
            DBER16(tlv16len, (ptmp));
            ptmp += 2;
            length -= 4;
        }

        if (ETLV16Type_DeviceVideoParams_ExtraData == tlv16type) {
            if (DUnlikely(!mbEnableVideo)) {
                return EECode_OK;
            }

            if (length < tlv16len) {
                LOG_ERROR("end here\n");
                return EECode_ServerReject_CorruptedProtocol;
            }
            SDataPiece *piece = allocSendBuffer((TMemSize)tlv16len + 64);
            if (DUnlikely(!piece)) {
                LOG_FATAL("no memory\n");
                return EECode_NoMemory;
            }

            memcpy(piece->p_data, ptmp, tlv16len);
            CIBuffer *p_buffer = NULL;
            mpOutputPins[EDemuxerVideoOutputPinIndex]->AllocBuffer(p_buffer);

            p_buffer->SetDataPtr(piece->p_data);
            p_buffer->SetDataSize((TMemSize) tlv16len);
            p_buffer->SetDataPtrBase((TU8 *) piece);
            p_buffer->SetDataMemorySize(0);
            p_buffer->SetBufferType(EBufferType_VideoExtraData);
            p_buffer->mpCustomizedContent = (void *) this;
            p_buffer->mCustomizedContentType = EBufferCustomizedContentType_AllocateByFilter;
            p_buffer->mVideoBitrate = mVideoBitrate;
            p_buffer->mVideoFrameRateNum = mVideoFramerateNum;
            p_buffer->mVideoFrameRateDen = mVideoFramerateDen;
            p_buffer->mVideoWidth = mVideoWidth;
            p_buffer->mVideoHeight = mVideoHeight;
            p_buffer->mVideoRoughFrameRate = mVideoFramerate;
            p_buffer->SetBufferFlags(CIBuffer::SYNC_POINT | CIBuffer::WITH_EXTRA_DATA);

            mpOutputPins[EDemuxerVideoOutputPinIndex]->SendBuffer(p_buffer);
            LOGM_INFO("send video extradata buffer done(width=%d, height=%d, fps=%f)\n",
                      mVideoWidth, mVideoHeight, mVideoFramerate);
            mDebugHeartBeat ++;
            p_buffer = NULL;
            mbUpdateVideoParams = 0;
            mbVideoExtraDataComes = 1;

            length -= tlv16len;
            ptmp += tlv16len;

            if (8 > length) {
                LOGM_WARN("no gop, length left %d\n", length);
                return EECode_OK;
            }

            DBER16(tlv16type, ptmp);
            ptmp += 2;
            DBER16(tlv16len, (ptmp));
            ptmp += 2;
            length -= 4;
        }

        if (ETLV16Type_DeviceVideoGOPStructure == tlv16type) {
            if (length < 4) {
                LOG_ERROR("must not comes here, length %d\n", length);
                return EECode_ServerReject_CorruptedProtocol;
            } else {
                DASSERT(4 == length);
            }
            TU32 gop = 0;
            DBER32(gop, ptmp);
            if (gop) {
                DParseGOP(mVideoGOPM, mVideoGOPN, mVideoGOPIDRInterval, gop);
                LOGM_NOTICE("gop is set %d, %d, %d, %08x\n", mVideoGOPM, mVideoGOPN, mVideoGOPIDRInterval, gop);
            }
        }

        return EECode_OK;
    } else if (DUnlikely(ECMDType_CloudSourceClient_UpdateAudioParams == type)) {

        if (DUnlikely(!mbEnableAudio)) {
            return EECode_OK;
        }

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

        if (mAudioSampleFrequency) {
            mAudioDTSOriIncTicks = mAudioDTSIncTicks = (TTime)((TU64)mAudioFrameSize * (TU64)DDefaultTimeScale) / (TTime)mAudioSampleFrequency;
        }

        DBER16(tlv16type, (pcmd + 18));
        DBER16(tlv16len, (pcmd + 20));
        DASSERT(ETLV16Type_DeviceAudioParams_Bitrate == tlv16type && 4 == tlv16len);
        DBER32(mAudioBitrate, (pcmd + 22));

        DBER16(tlv16type, (pcmd + 26));
        DBER16(tlv16len, (pcmd + 28));
        DASSERT(ETLV16Type_DeviceAudioParams_ExtraData == tlv16type && tlv16len > 0);

        SDataPiece *piece = allocSendBuffer((TMemSize)tlv16len + 256);
        if (DUnlikely(!piece)) {
            LOG_FATAL("no memory\n");
            return EECode_NoMemory;
        }

        memcpy(piece->p_data, pcmd + 30, tlv16len);
        CIBuffer *p_buffer = NULL;
        mpOutputPins[EDemuxerAudioOutputPinIndex]->AllocBuffer(p_buffer);

        LOGM_NOTICE("audio params, sample rate %d, channel number %d, format %d, bitrate %d, extra data %02x %02x\n", mAudioSampleFrequency, mAudioChannnelNumber, mAudioFormat, mAudioBitrate, piece->p_data[0], piece->p_data[1]);

        p_buffer->SetDataPtr(piece->p_data);
        p_buffer->SetDataSize((TMemSize) tlv16len);
        p_buffer->SetDataPtrBase((TU8 *) piece);
        p_buffer->SetDataMemorySize(0);
        p_buffer->SetBufferType(EBufferType_AudioExtraData);
        p_buffer->mpCustomizedContent = (void *) this;
        p_buffer->mCustomizedContentType = EBufferCustomizedContentType_AllocateByFilter;
        p_buffer->mContentFormat = mAudioFormat;
        p_buffer->mAudioChannelNumber = mAudioChannnelNumber;
        p_buffer->mAudioBitrate = mAudioBitrate;
        p_buffer->mAudioSampleRate = mAudioSampleFrequency;
        p_buffer->mAudioFrameSize = mAudioFrameSize;

        p_buffer->SetBufferFlags(CIBuffer::SYNC_POINT | CIBuffer::WITH_EXTRA_DATA);

        mpOutputPins[EDemuxerAudioOutputPinIndex]->SendBuffer(p_buffer);
        LOGM_INFO("send audio extradata buffer done(format=0x%x, channum=%d, samplerate=%d, bitrate=%d)\n",
                  mAudioFormat, mAudioChannnelNumber, mAudioSampleFrequency, mAudioBitrate);
        mDebugHeartBeat ++;
        p_buffer = NULL;
        mbAudioExtraDataComes = 1;
        return EECode_OK;
    } else if (DUnlikely(ECMDType_CloudSourceClient_DiscIndicator == type)) {
        LOGM_NOTICE("ECMDType_CloudSourceClient_DiscIndicator\n");
        resetConnectionContext();
        return EECode_OK;
    } else if (DUnlikely(ECMDType_CloudSourceClient_VideoSync == type)) {
        TU16 tlv16type = 0, tlv16len = 0;
        TU32 seq_number = 0;
        TTime time = 0;

        DBER16(tlv16type, pcmd);
        pcmd += 2;
        DBER16(tlv16len, (pcmd));
        pcmd += 2;
        DASSERT(ETLV16Type_TimeUnit == tlv16type);
        DASSERT(1 == tlv16len);
        DASSERT(ETimeUnit_native == pcmd[0]);
        pcmd += tlv16len;

        DBER16(tlv16type, pcmd);
        pcmd += 2;
        DBER16(tlv16len, (pcmd));
        pcmd += 2;
        DASSERT(ETLV16Type_DeviceVideoSyncSeqNumber == tlv16type);
        DASSERT(4 == tlv16len);
        DBER32(seq_number, pcmd);
        pcmd += tlv16len;

        DBER16(tlv16type, pcmd);
        pcmd += 2;
        DBER16(tlv16len, (pcmd));
        pcmd += 2;
        DASSERT(ETLV16Type_DeviceVideoSyncTime == tlv16type);
        DASSERT(8 == tlv16len);
        DBER64(time, pcmd);
        pcmd += tlv16len;

        processRecievePeerSyncVideo(time, seq_number);
        return EECode_OK;
    } else if (DUnlikely(ECMDType_CloudSourceClient_AudioSync == type)) {
        TU16 tlv16type = 0, tlv16len = 0;
        TU32 seq_number = 0;
        TTime time = 0;

        DBER16(tlv16type, pcmd);
        pcmd += 2;
        DBER16(tlv16len, (pcmd));
        pcmd += 2;
        DASSERT(ETLV16Type_TimeUnit == tlv16type);
        DASSERT(1 == tlv16len);
        DASSERT(ETimeUnit_native == pcmd[0]);
        pcmd += tlv16len;

        DBER16(tlv16type, pcmd);
        pcmd += 2;
        DBER16(tlv16len, (pcmd));
        pcmd += 2;
        DASSERT(ETLV16Type_DeviceAudioSyncSeqNumber == tlv16type);
        DASSERT(4 == tlv16len);
        DBER32(seq_number, pcmd);
        pcmd += tlv16len;

        DBER16(tlv16type, pcmd);
        pcmd += 2;
        DBER16(tlv16len, (pcmd));
        pcmd += 2;
        DASSERT(ETLV16Type_DeviceAudioSyncTime == tlv16type);
        DASSERT(8 == tlv16len);
        DBER64(time, pcmd);
        pcmd += tlv16len;

        processRecievePeerSyncAudio(time, seq_number);
        return EECode_OK;
    }

    SMSG msg;
    mDebugHeartBeat ++;

    memset(&msg, 0x0, sizeof(SMSG));
    msg.code = type;
    msg.owner_index = mIndex;
    msg.owner_type = EGenericComponentType_CloudConnecterServerAgent;

    if (DLikely((ECMDType_DebugClient_PrintAllChannels == type) || \
                (ECMDType_DebugClient_PrintCloudPipeline == type) || \
                (ECMDType_DebugClient_PrintStreamingPipeline == type) || \
                (ECMDType_DebugClient_PrintRecordingPipeline == type) || \
                (ECMDType_DebugClient_PrintSingleChannel == type))) {
        DBER32(msg.p0, pcmd);
    } else {
        LOGM_WARN("not expected cmd %x\n", type);
    }

    msg.p_agent_pointer = (TULong)mpAgent;
    msg.p_owner = (TULong)((IFilter *) this);

    return mpEngineMsgSink->MsgProc(msg);
}

EECode CCloudConnecterServerAgentV2::ProcessDataCallback(TU32 read_length, TU16 data_type, TU32 seq_num, TU8 flag)
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

    EBufferType currentBufferType = EBufferType_Invalid;
    TU8 currentCodecType = 0;
    TU32 index = __getIndexBufferType(data_type, currentBufferType, currentCodecType);
    mDebugHeartBeat ++;

    if (DUnlikely((index != ECloudConnecterAgentVideoPinIndex) && (index != ECloudConnecterAgentAudioPinIndex))) {
        LOGM_FATAL("client bugs, invalid data type %d\n", data_type);
        return EECode_BadParam;
    } else if (DUnlikely(!mbEnableAudio && (ECloudConnecterAgentAudioPinIndex == index))) {
        LOGM_INFO("discard audio packet since audio path is disabled.\n");
        return discardData(read_length);
    } else if (DUnlikely(!mbEnableVideo && (ECloudConnecterAgentVideoPinIndex == index))) {
        LOGM_INFO("discard video packet since video path is disabled.\n");
        return discardData(read_length);
    }

    if (DUnlikely(read_length >= mPacketMaxSize[index])) {
        if (DUnlikely(read_length >= (mPacketMaxSize[index] * 2))) {
            if (DUnlikely(read_length >= (mPacketMaxSize[index] * 4))) {
                LOGM_FATAL("too big packet, please check code, %d, %ld\n", read_length, mPacketMaxSize[index]);
                return EECode_BadParam;
            } else {
                mPacketMaxSize[index] = mPacketMaxSize[index] * 4;
                LOGM_WARN("too big packet, enlarge pre alloc memory block X4, %d, %ld\n", read_length, mPacketMaxSize[index]);
            }
        } else {
            mPacketMaxSize[index] = mPacketMaxSize[index] * 2;
            LOGM_WARN("too big packet, enlarge pre alloc memory block X2, %d, %ld\n", read_length, mPacketMaxSize[index]);
        }
    }

    if (DUnlikely((!mpBufferPool[index]) || (!mpOutputPins[index]) || (!mpMemorypools[index]))) {
        LOGM_ERROR("index %d is not supported, mpBufferPool[index] %p, mpBufferPool[index] %p, mpMemorypools[index] %p\n", index, mpBufferPool[index], mpBufferPool[index], mpMemorypools[index]);
        return EECode_NotSupported;
    }

    EECode err = EECode_OK;
    CIBuffer *p_buffer = NULL;

    //audio path
    if (DUnlikely(EDemuxerAudioOutputPinIndex == index)) {
        mpDataPacketStart[index] = mpMemorypools[index]->RequestMemBlock(read_length);
        err = mpAgent->ReadData(mpDataPacketStart[index], read_length);
        mDebugHeartBeat_Received ++;
        mDebugHeartBeat_AudioReceived ++;
        if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("try read %d fail, %s\n", read_length, gfGetErrorCodeString(err));
            mpMemorypools[index]->ReturnBackMemBlock(read_length, mpDataPacketStart[index]);
            mpDataPacketStart[index] = NULL;
            mPacketSize[index] = 0;
            mPacketRemainningSize[index] = 0;
            return err;
        }

        if (mpOutputPins[index]) {
            //LOGM_INFO("send data to: mpOutputPins[%d] %p\n", index, mpOutputPins[index]);
            mpOutputPins[index]->AllocBuffer(p_buffer);
            p_buffer->SetDataPtr(mpDataPacketStart[index]);
            p_buffer->SetDataSize((TMemSize)read_length);

            p_buffer->SetBufferType(currentBufferType);

            if (DUnlikely(mbAudioNeedSendSyncBuffer)) {
                mbAudioNeedSendSyncBuffer = 0;
                p_buffer->mAudioBitrate = mAudioBitrate;
                p_buffer->mAudioSampleRate = mAudioSampleFrequency;
                p_buffer->mAudioChannelNumber = mAudioChannnelNumber;
                p_buffer->mAudioFrameSize = mAudioFrameSize;
                p_buffer->SetBufferFlags(CIBuffer::KEY_FRAME | CIBuffer::SYNC_POINT);
            } else {
                p_buffer->SetBufferFlags(0);
                p_buffer->SetBufferFlags(CIBuffer::KEY_FRAME);
            }
            p_buffer->SetBufferType(EBufferType_AudioES);
            p_buffer->mContentFormat = currentCodecType;

            p_buffer->mpCustomizedContent = (void *)mpMemorypools[index];
            p_buffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
            p_buffer->SetDataPtrBase(mpDataPacketStart[index]);
            p_buffer->SetDataMemorySize((TMemSize)read_length);

            TU64 seq_number_64 = getSeqNumber64(seq_num, mLastAudioSeqNumber);

            if (DLikely(mbAudioFirstPacketComes)) {

                if (DLikely(seq_number_64 == (1 + mLastAudioSeqNumber))) {
                    mLastAudioTime += mAudioDTSIncTicks;
                } else {
                    LOGM_NOTICE("[seq number not continous]: audio discard packet number %llx, cur %llx, last %llx\n", seq_number_64 - mLastAudioSeqNumber, seq_number_64, mLastAudioSeqNumber);
                    mLastAudioTime += mAudioDTSIncTicks * (seq_number_64 - mLastAudioSeqNumber);
                }
                if (mAccumulatedAudioDrift > 0) {
                    mLastAudioTime ++;
                    mAccumulatedAudioDrift --;
                } else {
                    mLastAudioTime --;
                    mAccumulatedAudioDrift ++;
                }
            } else {
                mbAudioFirstPacketComes = 1;
                if (!mbSetInitialAudioTime) {
                    mLastAudioTime = mAudioDTSIncTicks * seq_number_64;
                    LOGM_WARN("[audio begin]: no initial value, estimated audio time %lld, seq number %lld\n", mLastAudioTime, seq_number_64);
                } else {
                    LOGM_NOTICE("[audio begin]: audio time %lld, seq number %lld\n", mLastAudioTime, seq_number_64);
                }
                mAccumulatedAudioDrift = 0;
            }

            mLastAudioSeqNumber = seq_number_64;
            p_buffer->SetBufferNativeDTS(mLastAudioTime);
            p_buffer->SetBufferNativePTS(mLastAudioTime);
            TTime linear_time = (TTime)((TTime)mLastAudioTime * (TTime)mAudioSampleFrequency) / (TTime)DDefaultTimeScale;
            p_buffer->SetBufferLinearDTS(linear_time);
            p_buffer->SetBufferLinearPTS(linear_time);

            //LOG_PRINTF("audio native time %lld, linear %lld\n", mLastAudioTime, linear_time);

            mpOutputPins[index]->SendBuffer(p_buffer);
            //LOGM_INFO("send data to: mpOutputPins[%d] %p done\n", index, mpOutputPins[index]);
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
        if (DUnlikely(!mbVideoExtraDataComes)) {
            if (DUnlikely(!(flag & DSACPHeaderFlagBit_PacketExtraDataIndicator))) {
                LOGM_INFO("discard data(%d), flag %x\n", read_length, flag);
                return discardData(read_length);
            }
            if (DLikely(!(flag & DSACPHeaderFlagBit_PacketKeyFrameIndicator))) {
                DASSERT(read_length < 1024);
                DASSERT(flag & DSACPHeaderFlagBit_PacketEndIndicator);

                SDataPiece *piece = allocSendBuffer(read_length);
                if (DUnlikely(!piece)) {
                    LOG_FATAL("no memory\n");
                    return EECode_NoMemory;
                }

                err = mpAgent->ReadData(piece->p_data, read_length);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_FATAL("read extra data fail, %d, %s\n", err, gfGetErrorCodeString(err));
                    return err;
                }

                CIBuffer *p_buffer = NULL;
                mpOutputPins[EDemuxerVideoOutputPinIndex]->AllocBuffer(p_buffer);

                p_buffer->SetDataPtr(piece->p_data);
                p_buffer->SetDataSize((TMemSize) read_length);
                p_buffer->SetDataPtrBase((TU8 *) piece);
                p_buffer->SetDataMemorySize(0);
                p_buffer->SetBufferType(EBufferType_VideoExtraData);
                p_buffer->mpCustomizedContent = (void *) this;
                p_buffer->mCustomizedContentType = EBufferCustomizedContentType_AllocateByFilter;
                p_buffer->mVideoBitrate = mVideoBitrate;
                p_buffer->mVideoFrameRateNum = mVideoFramerateNum;
                p_buffer->mVideoFrameRateDen = mVideoFramerateDen;
                p_buffer->mVideoWidth = mVideoWidth;
                p_buffer->mVideoHeight = mVideoHeight;
                p_buffer->mVideoRoughFrameRate = mVideoFramerate;
                p_buffer->SetBufferFlags(CIBuffer::SYNC_POINT | CIBuffer::WITH_EXTRA_DATA);

                LOGM_INFO("extra data comes, length %d, flag %x\n", read_length, flag);
                gfPrintMemory(piece->p_data, read_length);

                mpOutputPins[EDemuxerVideoOutputPinIndex]->SendBuffer(p_buffer);
                mDebugHeartBeat ++;
                p_buffer = NULL;
                mbUpdateVideoParams = 0;
                mbVideoExtraDataComes = 1;
                return EECode_OK;
            }
        }

        DASSERT(!mpDataPacketStart[index]);
        if (DUnlikely(mpDataPacketStart[index])) {
            if (DLikely(mPacketSize[index] + mPacketRemainningSize[index])) {
                LOGM_WARN("DDBG_FREE exist memory in ring buffer[index %d], %p, size %ld, remaining size %ld\n", index, mpDataPacketStart[index], mPacketSize[index], mPacketRemainningSize[index]);
                mpMemorypools[index]->ReturnBackMemBlock(read_length, mpDataPacketStart[index]);
                mPacketSize[index] = 0;
                mPacketRemainningSize[index] = 0;
            } else {
                LOGM_ERROR("DDBG_FREE exist memory in ring buffer[index %d], %p, but size is zero???? size %ld, remaining size %ld\n", index, mpDataPacketStart[index], mPacketSize[index], mPacketRemainningSize[index]);
            }
            mpDataPacketStart[index] = NULL;
        }

        mpDataPacketStart[index] = mpMemorypools[index]->RequestMemBlock(mPacketMaxSize[index]);
        mPacketRemainningSize[index] = mPacketMaxSize[index];
        mPacketSize[index] = 0;

        mCurrentExtraFlag = flag;

        mLastVideoSeqNumber32 = seq_num;
    } else {
        if (DUnlikely(NULL == mpDataPacketStart[index])) {
            LOGM_ERROR("NULL mpDataPacketStart[%d] %p, and not packet start, is_first flag %x wrong? read_length %d\n", index, mpDataPacketStart[index], flag, read_length);
            return discardData(read_length);
        }

        if (DUnlikely(mLastVideoSeqNumber32 != seq_num)) {
            LOGM_ERROR("not the same frame %d, %d\n", mLastVideoSeqNumber32, seq_num);
            return discardData(read_length);
        }

        if (DUnlikely(mPacketRemainningSize[index] < read_length)) {
            LOGM_ERROR("pre allocated memory not enough? please check code, read_length %d, mPacketRemainningSize[index] %ld\n", read_length, mPacketRemainningSize[index]);
            return EECode_BadParam;
        }
    }

    //LOGM_PRINTF("[debug flow]: read data, current size %ld, new size %ld, is_first %d, is_end %d\n", mPacketSize[index], read_length, is_first, is_last);
    err = mpAgent->ReadData(mpDataPacketStart[index] + mPacketSize[index], read_length);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("try read %d fail, %s\n", read_length, gfGetErrorCodeString(err));
        if (DUnlikely(mpDataPacketStart[index])) {
            if (DLikely(mPacketSize[index] + mPacketRemainningSize[index])) {
                LOGM_WARN("DDBG_FREE exist memory in ring buffer[index %d], %p, size %ld, remaining size %ld\n", index, mpDataPacketStart[index], mPacketSize[index], mPacketRemainningSize[index]);
                mpMemorypools[index]->ReturnBackMemBlock(mPacketSize[index] + read_length, mpDataPacketStart[index]);
                mPacketSize[index] = 0;
                mPacketRemainningSize[index] = 0;
            } else {
                LOGM_ERROR("DDBG_FREE exist memory in ring buffer[index %d], %p, but size is zero???? size %ld, remaining size %ld\n", index, mpDataPacketStart[index], mPacketSize[index], mPacketRemainningSize[index]);
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
        if (DUnlikely(!mbVideoWaitFirstKeyFrame)) {
            if (DUnlikely(!(mCurrentExtraFlag & DSACPHeaderFlagBit_PacketKeyFrameIndicator) || !(mCurrentExtraFlag & DSACPHeaderFlagBit_PacketExtraDataIndicator))) {
                LOGM_WARN("discard non-key frame before first key frame, flag %x\n", mCurrentExtraFlag);
                mpMemorypools[index]->ReturnBackMemBlock(mPacketRemainningSize[index] + mPacketSize[index], mpDataPacketStart[index]);
                mPacketRemainningSize[index] = 0;
                mPacketSize[index] = 0;
                mpDataPacketStart[index] = NULL;
                return EECode_OK;
            }
        }

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

                SDataPiece *piece = allocSendBuffer((TMemSize)(p_pps_end - p_sps));
                if (DUnlikely(!piece)) {
                    LOG_FATAL("no memory\n");
                    return EECode_NoMemory;
                }

                memcpy(piece->p_data, p_sps, (TU32)(p_pps_end - p_sps));

                mpOutputPins[EDemuxerVideoOutputPinIndex]->AllocBuffer(p_buffer);

                p_buffer->SetDataPtr(piece->p_data);
                p_buffer->SetDataSize((TMemSize)(p_pps_end - p_sps));
                p_buffer->SetDataPtrBase((TU8 *) piece);
                p_buffer->SetDataMemorySize(0);
                p_buffer->SetBufferType(EBufferType_VideoExtraData);
                p_buffer->mpCustomizedContent = (void *) this;
                p_buffer->mCustomizedContentType = EBufferCustomizedContentType_AllocateByFilter;
                p_buffer->mVideoBitrate = mVideoBitrate;
                p_buffer->mVideoFrameRateNum = mVideoFramerateNum;
                p_buffer->mVideoFrameRateDen = mVideoFramerateDen;
                p_buffer->mVideoWidth = mVideoWidth;
                p_buffer->mVideoHeight = mVideoHeight;
                p_buffer->mVideoRoughFrameRate = mVideoFramerate;
                p_buffer->SetBufferFlags(CIBuffer::SYNC_POINT | CIBuffer::WITH_EXTRA_DATA);

                LOGM_INFO("extra data comes, length %d, flag %x\n", read_length, flag);
                gfPrintMemory(piece->p_data, read_length);

                mpOutputPins[EDemuxerVideoOutputPinIndex]->SendBuffer(p_buffer);
                mDebugHeartBeat ++;
                p_buffer = NULL;
                mbUpdateVideoParams = 0;
                mbVideoExtraDataComes = 1;

                if (has_idr) {
                    mpOutputPins[index]->AllocBuffer(p_buffer);
                    p_buffer->SetDataPtr(mpDataPacketStart[index]);
                    p_buffer->SetDataSize(mPacketSize[index]);

                    p_buffer->mpCustomizedContent = (void *)mpMemorypools[index];
                    p_buffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
                    p_buffer->SetDataPtrBase(mpDataPacketStart[index]);
                    p_buffer->SetDataMemorySize(mPacketSize[index]);
                    p_buffer->SetBufferType(EBufferType_VideoES);
                    p_buffer->SetBufferFlags(CIBuffer::KEY_FRAME);

                    TU64 seq_number_64 = getSeqNumber64(seq_num, mLastVideoSeqNumber);
                    if (DLikely(mbVideoFirstPacketComes)) {
                        if (DLikely(seq_number_64 == (1 + mLastVideoSeqNumber))) {
                            mLastVideoTime += mVideoDTSIncTicks;
                        } else {
                            LOGM_NOTICE("[seq number not continous]: video discard packet number %llx, cur %llx, last %llx\n", seq_number_64 - mLastVideoSeqNumber, seq_number_64, mLastVideoSeqNumber);
                            mLastVideoTime += mVideoDTSIncTicks * (seq_number_64 - mLastVideoSeqNumber);
                        }
                        if (mAccumulatedVideoDrift > 0) {
                            mLastVideoTime ++;
                            mAccumulatedVideoDrift --;
                        } else {
                            mLastVideoTime --;
                            mAccumulatedVideoDrift ++;
                        }
                    } else {
                        mbVideoFirstPacketComes = 1;
                        if (!mbSetInitialVideoTime) {
                            mLastVideoTime = mVideoDTSIncTicks * seq_number_64;
                            LOGM_WARN("[video begin]: no initial value, estimated video time %lld, seq number %lld\n", mLastVideoTime, seq_number_64);
                        } else {
                            LOGM_NOTICE("[video begin]: video time %lld, seq number %lld\n", mLastVideoTime, seq_number_64);
                        }
                        mAccumulatedVideoDrift = 0;
                    }
                    mLastVideoSeqNumber = seq_number_64;

                    p_buffer->SetBufferNativeDTS(mLastVideoTime);
                    p_buffer->SetBufferLinearDTS(mLastVideoTime);
                    p_buffer->SetBufferNativePTS(mLastVideoTime);
                    p_buffer->SetBufferLinearPTS(mLastVideoTime);

                    LOGM_INFO("receive video key frame with extradata, size %ld, %lld\n", p_buffer->GetDataSize(), p_buffer->GetBufferNativePTS());
                    mTimeCounter = 0;
                    mpOutputPins[index]->SendBuffer(p_buffer);
                    mDebugHeartBeat ++;
                    p_buffer = NULL;
                }
            } else {
                LOGM_ERROR("can not find sps pps\n");

                if (DLikely(mpDataPacketStart[index])) {
                    if (DLikely(mPacketSize[index] + mPacketRemainningSize[index])) {
                        LOGM_WARN("DDBG_FREE exist memory in ring buffer[index %d], %p, size %ld, remaining size %ld\n", index, mpDataPacketStart[index], mPacketSize[index], mPacketRemainningSize[index]);
                        mpMemorypools[index]->ReturnBackMemBlock(mPacketSize[index] + mPacketRemainningSize[index], mpDataPacketStart[index]);
                    } else {
                        LOGM_ERROR("DDBG_FREE exist memory in ring buffer[index %d], %p, but size is zero???? size %ld, remaining size %ld\n", index, mpDataPacketStart[index], mPacketSize[index], mPacketRemainningSize[index]);
                    }
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

        if (DUnlikely(p_check[0] || p_check[1])) {
            LOGM_ERROR("no start code? %02x %02x %02x %02x %02x\n", mpDataPacketStart[index][0], mpDataPacketStart[index][1], mpDataPacketStart[index][2], mpDataPacketStart[index][3], mpDataPacketStart[index][4]);
            if (DLikely(mpDataPacketStart[index])) {
                if (DLikely(mPacketSize[index] + mPacketRemainningSize[index])) {
                    LOGM_WARN("DDBG_FREE exist memory in ring buffer[index %d], %p, size %ld, remaining size %ld\n", index, mpDataPacketStart[index], mPacketSize[index], mPacketRemainningSize[index]);
                    mpMemorypools[index]->ReturnBackMemBlock(mPacketSize[index] + mPacketRemainningSize[index], mpDataPacketStart[index]);
                    mPacketSize[index] = 0;
                    mPacketRemainningSize[index] = 0;
                } else {
                    LOGM_ERROR("DDBG_FREE exist memory in ring buffer[index %d], %p, but size is zero???? size %ld, remaining size %ld\n", index, mpDataPacketStart[index], mPacketSize[index], mPacketRemainningSize[index]);
                }
                mpDataPacketStart[index] = NULL;
            }
            return EECode_OK;
        } else {
            if (DUnlikely((p_check[2] != 0x01) && (p_check[3] != 0x01))) {
                LOGM_ERROR("no start code? %02x %02x %02x %02x %02x\n", mpDataPacketStart[index][0], mpDataPacketStart[index][1], mpDataPacketStart[index][2], mpDataPacketStart[index][3], mpDataPacketStart[index][4]);
                if (DLikely(mpDataPacketStart[index])) {
                    if (DLikely(mPacketSize[index] + mPacketRemainningSize[index])) {
                        LOGM_WARN("DDBG_FREE exist memory in ring buffer[index %d], %p, size %ld, remaining size %ld\n", index, mpDataPacketStart[index], mPacketSize[index], mPacketRemainningSize[index]);
                        mpMemorypools[index]->ReturnBackMemBlock(mPacketSize[index] + mPacketRemainningSize[index], mpDataPacketStart[index]);
                        mPacketSize[index] = 0;
                        mPacketRemainningSize[index] = 0;
                    } else {
                        LOGM_ERROR("DDBG_FREE exist memory in ring buffer[index %d], %p, but size is zero???? size %ld, remaining size %ld\n", index, mpDataPacketStart[index], mPacketSize[index], mPacketRemainningSize[index]);
                    }
                    mpDataPacketStart[index] = NULL;
                }
                return EECode_OK;
            }
        }

        TU8 nal_type = 0, slice_type = 0;
        TU8 *p_slice_header = __h264_slice_header(p_check, (TU32)mPacketSize[index]);
        if (DUnlikely(!p_slice_header)) {
            LOGM_ERROR("not found slice header, data corruption!\n");
        } else {
            nal_type = p_slice_header[0] & 0x1f;
            slice_type = gfGetH264SilceType(p_slice_header + 1);
        }

        mpOutputPins[index]->AllocBuffer(p_buffer);

        TU64 seq_number_64 = getSeqNumber64(seq_num, mLastVideoSeqNumber);
        if (DLikely(mbVideoFirstPacketComes)) {
            if (DLikely(seq_number_64 == (1 + mLastVideoSeqNumber))) {
                mLastVideoTime += mVideoDTSIncTicks;
            } else {
                LOGM_NOTICE("[seq number not continous]: video discard packet number %llx, cur %llx, last %llx\n", seq_number_64 - mLastVideoSeqNumber, seq_number_64, mLastVideoSeqNumber);
                mLastVideoTime += mVideoDTSIncTicks * (seq_number_64 - mLastVideoSeqNumber);
            }
            if (mAccumulatedVideoDrift > 0) {
                mLastVideoTime ++;
                mAccumulatedVideoDrift --;
            } else {
                mLastVideoTime --;
                mAccumulatedVideoDrift ++;
            }
        } else {
            mbVideoFirstPacketComes = 1;
            if (!mbSetInitialVideoTime) {
                mLastVideoTime = mVideoDTSIncTicks * seq_number_64;
                LOGM_WARN("[video begin]: no initial value, estimated video time %lld, seq number %lld\n", mLastVideoTime, seq_number_64);
            } else {
                LOGM_NOTICE("[video begin]: video time %lld, seq number %lld\n", mLastVideoTime, seq_number_64);
            }
            mAccumulatedVideoDrift = 0;
        }
        mLastVideoSeqNumber = seq_number_64;

        p_buffer->SetBufferNativeDTS(mLastVideoTime);
        p_buffer->SetBufferLinearDTS(mLastVideoTime);

        if ((mCurrentExtraFlag & DSACPHeaderFlagBit_PacketKeyFrameIndicator) || (ENalType_IDR == nal_type)) {
            p_buffer->SetBufferType(EBufferType_VideoES);
            p_buffer->SetBufferFlags(CIBuffer::KEY_FRAME);
            p_buffer->mVideoFrameType = EPredefinedPictureType_IDR;
            mDebugHeartBeat_KeyframeReceived ++;
            mbUpdateVideoParams = 1;
            mTimeCounter = 0;

            p_buffer->SetBufferNativePTS(mLastVideoTime);
            p_buffer->SetBufferLinearPTS(mLastVideoTime);
#ifdef DDEBUG_PRINT_DATA
            LOG_PRINTF("[4] start idr, size %ld\n", mPacketSize[index]);
            gfPrintMemory(mpDataPacketStart[index], 64);
#endif
            //LOGM_PRINTF("[PTS]: key frame %lld, naltype %d, slice_type %d\n", p_buffer->GetBufferNativePTS(), nal_type, slice_type);
        } else {
            p_buffer->SetBufferType(EBufferType_VideoES);
            p_buffer->SetBufferFlags(0);
            if (1 < mVideoGOPM) {
                TTime pts = 0;
                if (DUnlikely(EH264SliceType_B == slice_type)) {
                    p_buffer->mVideoFrameType = EPredefinedPictureType_B;
                    if (DLikely(mTimeCounter)) {
                        pts = mLastVideoTime - mVideoDTSIncTicks;
                        p_buffer->SetBufferNativePTS(pts);
                        p_buffer->SetBufferLinearPTS(pts);
                    } else {
                        p_buffer->SetBufferNativePTS(mLastVideoTime);
                        p_buffer->SetBufferLinearPTS(mLastVideoTime);
                    }
                } else {
                    if (DLikely((EH264SliceType_P == slice_type) || (EH264SliceType_SP == slice_type))) {
                        p_buffer->mVideoFrameType = EPredefinedPictureType_P;
                    } else {
                        p_buffer->mVideoFrameType = EPredefinedPictureType_I;
                    }
                    pts = (mVideoGOPM - 1) * mVideoDTSIncTicks + mLastVideoTime;
                    p_buffer->SetBufferNativePTS(pts);
                    p_buffer->SetBufferLinearPTS(pts);
                    mTimeCounter ++;
                }
            } else {
                if (DUnlikely(EH264SliceType_B == slice_type)) {
                    p_buffer->mVideoFrameType = EPredefinedPictureType_B;
                } else {
                    if (DLikely((EH264SliceType_P == slice_type) || (EH264SliceType_SP == slice_type))) {
                        p_buffer->mVideoFrameType = EPredefinedPictureType_P;
                    } else {
                        p_buffer->mVideoFrameType = EPredefinedPictureType_I;
                    }
                }
                p_buffer->SetBufferNativePTS(mLastVideoTime);
                p_buffer->SetBufferLinearPTS(mLastVideoTime);
            }
            //LOGM_DEBUG("non key frame, flag %08x\n", p_buffer->GetBufferFlags());
#ifdef DDEBUG_PRINT_DATA
            LOG_PRINTF("[5] non-idr, size %ld\n", mPacketSize[index]);
            gfPrintMemory(mpDataPacketStart[index], 64);
#endif
            //LOGM_PRINTF("[PTS]: non-key frame %lld, naltype %d, slice_type %d\n", p_buffer->GetBufferNativePTS(), nal_type, slice_type);
        }

        p_buffer->SetDataPtr(mpDataPacketStart[index]);
        p_buffer->SetDataSize(mPacketSize[index]);

        if (DUnlikely(mbUpdateVideoParams)) {
            mbUpdateVideoParams = 0;
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

        p_buffer->mpCustomizedContent = (void *)mpMemorypools[index];
        p_buffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
        p_buffer->SetDataPtrBase(mpDataPacketStart[index]);
        p_buffer->SetDataMemorySize(mPacketSize[index]);

        LOGM_INFO("receive video frame, size %ld, pts %lld\n", p_buffer->GetDataSize(), p_buffer->GetBufferNativePTS());
        mpOutputPins[index]->SendBuffer(p_buffer);

        mPacketSize[index] = 0;
        mpDataPacketStart[index] = NULL;
        mPacketRemainningSize[index] = 0;

        mDebugHeartBeat_FrameSend ++;
    }

    //LOGM_INFO("end: mpDataPacketStart[%d], %p\n", index, mpDataPacketStart[index]);

    return EECode_OK;
}

SDataPiece *CCloudConnecterServerAgentV2::allocSendBuffer(TMemSize max_buffer_size)
{
    AUTO_LOCK(mpResourceMutex);
    SDataPiece *buffer = NULL;

    CIDoubleLinkedList::SNode *p_node = mpFreeExtraDataPieceList->FirstNode();
    if (p_node) {
        buffer = (SDataPiece *) p_node->p_context;
        if (DLikely(buffer)) {
            if (buffer->data_size < max_buffer_size) {
                if (buffer->p_data) {
                    DDBG_FREE(buffer->p_data, "CREX");
                    buffer->p_data = NULL;
                }
                buffer->data_size = 0;
            }
            if (!buffer->p_data) {
                buffer->p_data = (TU8 *) DDBG_MALLOC(max_buffer_size, "CREX");
                if (DUnlikely(!buffer->p_data)) {
                    LOG_FATAL("no memory, request size %ld\n", max_buffer_size);
                    return NULL;
                }
                buffer->data_size = max_buffer_size;
            }
            mpFreeExtraDataPieceList->RemoveContent((void *) buffer);
            return buffer;
        } else {
            LOG_FATAL("internal logic bug\n");
            return NULL;
        }
    } else {
        buffer = (SDataPiece *) DDBG_MALLOC(sizeof(SDataPiece), "CRDP");
        if (DUnlikely(!buffer)) {
            LOG_FATAL("no memory, request size %ld\n", (TULong)sizeof(SDataPiece));
            return NULL;
        }

        buffer->p_data = (TU8 *) DDBG_MALLOC(max_buffer_size, "CREX");
        if (DUnlikely(!buffer->p_data)) {
            LOG_FATAL("no memory, request size %ld\n", max_buffer_size);
            DDBG_FREE(buffer, "CREX");
            return NULL;
        }
        buffer->data_size = max_buffer_size;
    }

    return buffer;
}

void CCloudConnecterServerAgentV2::releaseSendBuffer(void *buffer)
{
    AUTO_LOCK(mpResourceMutex);
    mpFreeExtraDataPieceList->InsertContent(NULL, (void *) buffer, 0);
}

EECode CCloudConnecterServerAgentV2::discardData(TU32 length)
{
    if (DUnlikely(mDiscardDataBufferSize < length)) {
        mDiscardDataBufferSize = length + 32;
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

    return mpAgent->ReadData(mpDiscardDataBuffer, length);
}

void CCloudConnecterServerAgentV2::flushPrevious()
{
    mbVideoWaitFirstKeyFrame = 1;
    if (mpMemorypools[ECloudConnecterAgentVideoPinIndex] && mpDataPacketStart[ECloudConnecterAgentVideoPinIndex]) {
        mpMemorypools[ECloudConnecterAgentVideoPinIndex]->ReturnBackMemBlock(mPacketSize[ECloudConnecterAgentVideoPinIndex], mpDataPacketStart[ECloudConnecterAgentVideoPinIndex]);
        mpDataPacketStart[ECloudConnecterAgentVideoPinIndex] = NULL;
        mPacketSize[ECloudConnecterAgentVideoPinIndex] = 0;
        mPacketRemainningSize[ECloudConnecterAgentVideoPinIndex] = 0;
    }

    if (mpMemorypools[ECloudConnecterAgentAudioPinIndex] && mpDataPacketStart[ECloudConnecterAgentAudioPinIndex]) {
        mpMemorypools[ECloudConnecterAgentAudioPinIndex]->ReturnBackMemBlock(mPacketSize[ECloudConnecterAgentAudioPinIndex], mpDataPacketStart[ECloudConnecterAgentAudioPinIndex]);
        mpDataPacketStart[ECloudConnecterAgentAudioPinIndex] = NULL;
        mPacketSize[ECloudConnecterAgentAudioPinIndex] = 0;
        mPacketRemainningSize[ECloudConnecterAgentAudioPinIndex] = 0;
    }
}

void CCloudConnecterServerAgentV2::processRecievePeerSyncVideo(TTime time, TU32 seq_number)
{
    TU64 seq_number_64 = getSeqNumber64(seq_number, mLastSyncVideoSeqNumber);

    if (DLikely(mbVideoFirstPeerSyncComes)) {
        TTime time_gap = time - mPeerLastVideoSyncTime;
        TU64 seq_count_gap = 0;

        seq_count_gap = seq_number_64 - mLastSyncVideoSeqNumber;

        TTime target_ticks = (time_gap) / seq_count_gap;
        LOGM_INFO("[sync video]: ticks ori %lld, current %lld, target %lld, seq %d, seq_64 %lld\n", mVideoDTSOriIncTicks, mVideoDTSIncTicks, target_ticks, seq_number, seq_number_64);

        if (DLikely(mLastVideoSeqNumber == seq_number_64)) {
            mAccumulatedVideoDrift = time - mLastVideoTime;
            if (DLikely((mAccumulatedVideoDrift < DMAX_VIDEO_ACCUMULATED_DRIFT) && (mAccumulatedVideoDrift > (-DMAX_VIDEO_ACCUMULATED_DRIFT)))) {
                LOGM_INFO("[sync video]: drift %lld\n", mAccumulatedVideoDrift);
            } else {
                LOGM_WARN("video not sync, accumulated drift too large %lld, (time %lld, mLastVideoTime %lld), seq %d, seq_64 %lld\n", mAccumulatedVideoDrift, time, mLastVideoTime, seq_number, seq_number_64);
                if (mAccumulatedVideoDrift >= DMAX_VIDEO_ACCUMULATED_DRIFT) {
                    mAccumulatedVideoDrift = DMAX_VIDEO_ACCUMULATED_DRIFT;
                } else {
                    mAccumulatedVideoDrift = -DMAX_VIDEO_ACCUMULATED_DRIFT;
                }
            }
        } else {
            LOGM_ERROR("client should invoke Uploading() before VideoSync(), mLastVideoSeqNumber %llx, seq_number_64 %llx, seq_number %08x\n", mLastVideoSeqNumber, seq_number_64, seq_number);
            TTime estimated_pts = 0;

            estimated_pts = (seq_number_64 - mLastVideoSeqNumber) * mVideoDTSIncTicks + mLastVideoTime;
            mAccumulatedVideoDrift = time - estimated_pts;
            if (DLikely((mAccumulatedVideoDrift < DMAX_VIDEO_ACCUMULATED_DRIFT) && (mAccumulatedVideoDrift > (-DMAX_VIDEO_ACCUMULATED_DRIFT)))) {
                LOGM_INFO("[sync video]: estimated drift %lld\n", mAccumulatedVideoDrift);
            } else {
                LOGM_WARN("video not sync, estimated accumulated drift too large %lld, (time %lld, estimated_pts %lld), seq %d, seq_64 %lld\n", mAccumulatedVideoDrift, time, estimated_pts, seq_number, seq_number_64);
                if (mAccumulatedVideoDrift >= DMAX_VIDEO_ACCUMULATED_DRIFT) {
                    mAccumulatedVideoDrift = DMAX_VIDEO_ACCUMULATED_DRIFT;
                } else {
                    mAccumulatedVideoDrift = -DMAX_VIDEO_ACCUMULATED_DRIFT;
                }
            }
        }

        mVideoDTSIncTicks = target_ticks;
    } else {
        mbVideoFirstPeerSyncComes = 1;
        if (!mbVideoFirstPacketComes) {
            mLastVideoTime = time;
            mLastVideoSeqNumber = seq_number_64;
            mLastVideoSeqNumber32 = seq_number;
            mbSetInitialVideoTime = 1;
            LOGM_NOTICE("[sync video initial value] initial time %lld, seq %d, seq_64 %lld\n", time, seq_number, seq_number_64);
        } else {
            LOGM_WARN("[video]: should invoke first sync to set intial time\n");
        }
    }

    mLastSyncVideoSeqNumber = seq_number_64;
    mPeerLastVideoSyncTime = time;
}

void CCloudConnecterServerAgentV2::processRecievePeerSyncAudio(TTime time, TU32 seq_number)
{
    TU64 seq_number_64 = getSeqNumber64(seq_number, mLastSyncAudioSeqNumber);
    TTime estimated_pts = 0;

    if (DLikely(mbAudioFirstPeerSyncComes)) {

        TTime time_gap = time - mPeerLastAudioSyncTime;
        TU64 seq_count_gap = 0;

        seq_count_gap = seq_number_64 - mLastSyncAudioSeqNumber;

        TTime target_ticks = (time_gap) / seq_count_gap;
        LOGM_INFO("[sync audio]: ticks ori %lld, current %lld, target %lld, seq %d, seq_64 %lld\n", mAudioDTSOriIncTicks, mAudioDTSIncTicks, target_ticks, seq_number, seq_number_64);

        if (DLikely(mLastAudioSeqNumber == seq_number_64)) {
            mAccumulatedAudioDrift = time - mLastAudioTime;
            if (DLikely((mAccumulatedAudioDrift < DMAX_AUDIO_ACCUMULATED_DRIFT) && (mAccumulatedAudioDrift > (-DMAX_AUDIO_ACCUMULATED_DRIFT)))) {
                LOGM_INFO("[sync audio]: drift %lld\n", mAccumulatedAudioDrift);
            } else {
                LOGM_WARN("audio not sync, accumulated drift too large %lld, (time %lld, mLastAudioTime %lld), seq %d, seq_64 %lld\n", mAccumulatedAudioDrift, time, mLastAudioTime, seq_number, seq_number_64);
                if (mAccumulatedAudioDrift >= DMAX_AUDIO_ACCUMULATED_DRIFT) {
                    mAccumulatedAudioDrift = DMAX_AUDIO_ACCUMULATED_DRIFT;
                } else {
                    mAccumulatedAudioDrift = -DMAX_AUDIO_ACCUMULATED_DRIFT;
                }
            }
        } else {
            LOGM_ERROR("client should invoke Uploading() before AudioSync(), mLastAudioSeqNumber %llx, seq_number_64 %llx, seq_number %08x\n", mLastAudioSeqNumber, seq_number_64, seq_number);
            estimated_pts = (seq_number_64 - mLastAudioSeqNumber) * mAudioDTSIncTicks + mLastAudioTime;
            mAccumulatedAudioDrift = time - estimated_pts;
            if (DLikely((mAccumulatedAudioDrift < DMAX_AUDIO_ACCUMULATED_DRIFT) && (mAccumulatedAudioDrift > (-DMAX_AUDIO_ACCUMULATED_DRIFT)))) {
                LOGM_INFO("[sync audio]: estimated drift %lld\n", mAccumulatedAudioDrift);
            } else {
                LOGM_WARN("audio not sync, estimated accumulated drift too large %lld, (time %lld, estimated_pts %lld), seq %d, seq_64 %lld\n", mAccumulatedAudioDrift, time, estimated_pts, seq_number, seq_number_64);
                if (mAccumulatedAudioDrift >= DMAX_AUDIO_ACCUMULATED_DRIFT) {
                    mAccumulatedAudioDrift = DMAX_AUDIO_ACCUMULATED_DRIFT;
                } else {
                    mAccumulatedAudioDrift = -DMAX_AUDIO_ACCUMULATED_DRIFT;
                }
            }
        }
        mAudioDTSIncTicks = target_ticks;

    } else {
        mbAudioFirstPeerSyncComes = 1;
        if (!mbAudioFirstPacketComes) {
            mLastAudioTime = time;
            mLastAudioSeqNumber = seq_number_64;
            mLastAudioSeqNumber32 = seq_number;
            mbSetInitialAudioTime = 1;
            LOGM_NOTICE("[sync audio initial value] initial time %lld, seq %d, seq_64 %lld\n", time, seq_number, seq_number_64);
        } else {
            LOGM_WARN("[audio]: should invoke first sync to set intial time\n");
        }
    }

    mLastSyncAudioSeqNumber = seq_number_64;
    mPeerLastAudioSyncTime = time;
}

void CCloudConnecterServerAgentV2::resetVideoSyncContext()
{
    mbVideoFirstPeerSyncComes = 0;
    mbVideoFirstLocalSyncComes = 0;

    mPeerLastVideoSyncTime = 0;
    mLocalLastVideoSyncTime = 0;
    mLastSyncVideoSeqNumber = 0;
    mLastVideoSyncTime = 0;

    mAccumulatedVideoDrift = 0;
}

void CCloudConnecterServerAgentV2::resetAudioSyncContext()
{
    mbAudioFirstPeerSyncComes = 0;
    mbAudioFirstLocalSyncComes = 0;

    mPeerLastAudioSyncTime = 0;
    mLocalLastAudioSyncTime = 0;
    mLastSyncAudioSeqNumber = 0;
    mLastAudioSyncTime = 0;

    mAccumulatedAudioDrift = 0;
}

void CCloudConnecterServerAgentV2::resetConnectionContext()
{
    mbUpdateVideoParams = 1;
    mbUpdateAudioParams = 1;
    mbVideoWaitFirstKeyFrame = 1;
    mbVideoExtraDataComes = 0;
    mbAudioExtraDataComes = 0;
    mbAudioNeedSendSyncBuffer = 1;
    mCurrentExtraFlag = 0;

    mbVideoFirstPacketComes = 0;
    mbAudioFirstPacketComes = 0;

    mbSetInitialVideoTime = 0;
    mbSetInitialAudioTime = 0;

    mbVideoFirstPeerSyncComes = 0;
    mbAudioFirstPeerSyncComes = 0;
    mbVideoFirstLocalSyncComes = 0;
    mbAudioFirstLocalSyncComes = 0;

    mLastVideoSeqNumber = 0;
    mLastAudioSeqNumber = 0;

    mLastVideoSeqNumber32 = 0;
    mLastAudioSeqNumber32 = 0;

    mLastVideoTime = 0;
    mLastAudioTime = 0;

    mPeerLastVideoSyncTime = 0;
    mPeerLastAudioSyncTime = 0;
    mLocalLastVideoSyncTime = 0;
    mLocalLastAudioSyncTime = 0;
    mLastSyncVideoSeqNumber = 0;
    mLastSyncAudioSeqNumber = 0;
    mLastVideoSyncTime = 0;
    mLastAudioSyncTime = 0;

    mAccumulatedVideoDrift = 0;
    mAccumulatedAudioDrift = 0;

    mVideoDTSOriIncTicks = mVideoDTSIncTicks = mVideoFramerateDen;
    mAudioDTSOriIncTicks = mAudioDTSIncTicks = (TTime)((TTime)mAudioFrameSize * (TTime)DDefaultTimeScale) / (TTime)mAudioSampleFrequency;
    LOGM_NOTICE("audio tick %lld, ori %lld, video tick %lld, ori %lld, mAudioFrameSize %d, mAudioSampleFrequency %d\n", mAudioDTSIncTicks, mAudioDTSOriIncTicks, mVideoDTSIncTicks, mVideoDTSOriIncTicks, mAudioFrameSize, mAudioSampleFrequency);
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


