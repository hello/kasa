/*******************************************************************************
 * external_audio_es_source.cpp
 *
 * History:
 *    2014/12/10 - [Zhi He] create file
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

#include "media_mw_if.h"
#include "mw_internal.h"
#include "media_mw_utils.h"

#include "framework_interface.h"
#include "filters_interface.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "external_audio_es_source.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

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

IFilter *gfCreateExternalAudioEsSource(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CExtAudioEsSource::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

//-----------------------------------------------------------------------
//
// CExtAudioEsSource
//
//-----------------------------------------------------------------------

IFilter *CExtAudioEsSource::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
{
    CExtAudioEsSource *result = new CExtAudioEsSource(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

CObject *CExtAudioEsSource::GetObject0() const
{
    return (CObject *) this;
}

CExtAudioEsSource::CExtAudioEsSource(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
    : inherited(pName, index)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpEngineMsgSink(pEngineMsgSink)
    , mbWaitKeyFrame(1)
    , mbSendExtraData(0)
    , mpExtraData(NULL)
    , mExtraDataSize(0)
    , mpOutputPin(NULL)
    , mpBufferPool(NULL)
    , mpMemPool(NULL)
    , mAudioChannelNumber(DDefaultAudioChannelNumber)
    , mAudioSampleSize(16)
    , mAudioProfile(0)
    , mAudioFormat(StreamFormat_AAC)
    , mAudioSampleRate(DDefaultAudioSampleRate)
    , mAudioFrameSize(DDefaultAudioFrameSize)
    , mAudioBitrate(64000)
{

}

EECode CExtAudioEsSource::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleExternalSourceVideoEs);
    mPersistEOSBuffer.SetBufferType(EBufferType_FlowControl_EOS);

    return EECode_OK;
}

CExtAudioEsSource::~CExtAudioEsSource()
{

}

void CExtAudioEsSource::Delete()
{
    if (mpOutputPin) {
        mpOutputPin->Delete();
        mpOutputPin = NULL;
    }

    if (mpBufferPool) {
        mpBufferPool->Delete();
        mpBufferPool = NULL;
    }

    if (mpMemPool) {
        mpMemPool->GetObject0()->Delete();
        mpMemPool = NULL;
    }

    if (mpExtraData) {
        DDBG_FREE(mpExtraData, "EAae");
        mpExtraData = NULL;
    }

    inherited::Delete();
}

EECode CExtAudioEsSource::Initialize(TChar *p_param)
{
    return EECode_OK;
}

EECode CExtAudioEsSource::Finalize()
{
    return EECode_OK;
}

EECode CExtAudioEsSource::Run()
{
    return EECode_OK;
}

EECode CExtAudioEsSource::Exit()
{
    return EECode_OK;
}

EECode CExtAudioEsSource::Start()
{
    return EECode_OK;
}

EECode CExtAudioEsSource::Stop()
{
    return EECode_OK;
}

void CExtAudioEsSource::Pause()
{
    return;
}

void CExtAudioEsSource::Resume()
{
    return;
}

void CExtAudioEsSource::Flush()
{
    return;
}

void CExtAudioEsSource::ResumeFlush()
{
    return;
}

EECode CExtAudioEsSource::Suspend(TUint release_context)
{
    return EECode_OK;
}

EECode CExtAudioEsSource::ResumeSuspend(TComponentIndex input_index)
{
    return EECode_OK;
}

EECode CExtAudioEsSource::SwitchInput(TComponentIndex focus_input_index)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

EECode CExtAudioEsSource::FlowControl(EBufferType flowcontrol_type)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

EECode CExtAudioEsSource::SendCmd(TUint cmd)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

void CExtAudioEsSource::PostMsg(TUint cmd)
{
    LOGM_FATAL("TO DO\n");
    return;
}

EECode CExtAudioEsSource::AddInputPin(TUint &index, TUint type)
{
    LOGM_FATAL("CExtAudioEsSource: no input pin\n");

    return EECode_InternalLogicalBug;
}

EECode CExtAudioEsSource::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    DASSERT(StreamType_Audio == type);

    if (StreamType_Audio == type) {
        if (mpOutputPin == NULL) {
            mpOutputPin = COutputPin::Create("[Audio output pin for CExtAudioEsSource filter]", (IFilter *) this);

            if (!mpOutputPin)  {
                LOGM_FATAL("COutputPin::Create() fail?\n");
                return EECode_NoMemory;
            }

            mpBufferPool = CBufferPool::Create("[Buffer pool for video output pin in CExtAudioEsSource filter]", 64);
            if (!mpBufferPool)  {
                LOGM_FATAL("CBufferPool::Create() fail?\n");
                return EECode_NoMemory;
            }
            mpBufferPool->SetReleaseBufferCallBack(__release_ring_buffer);

            mpOutputPin->SetBufferPool(mpBufferPool);
            mpMemPool = CRingMemPool::Create(1 * 1024 * 1024);
        }

        index = 0;
        if (mpOutputPin->AddSubPin(sub_index) != EECode_OK) {
            LOGM_FATAL("COutputPin::AddSubPin() fail?\n");
            return EECode_Error;
        }

    } else {
        LOGM_ERROR("BAD output pin type %d\n", type);
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

IOutputPin *CExtAudioEsSource::GetOutputPin(TUint index, TUint sub_index)
{
    if (DLikely(!index)) {
        if (DLikely(mpOutputPin)) {
            if (DLikely(sub_index < mpOutputPin->NumberOfPins())) {
                return mpOutputPin;
            } else {
                LOGM_FATAL("BAD sub_index %d\n", sub_index);
            }
        } else {
            LOGM_FATAL("NULL mpOutputPin\n");
        }
    } else {
        LOGM_FATAL("BAD index %d\n", index);
    }

    return NULL;
}

IInputPin *CExtAudioEsSource::GetInputPin(TUint index)
{
    LOGM_FATAL("CExtAudioEsSource: no input pin\n");
    return NULL;
}

EECode CExtAudioEsSource::FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
    EECode ret = EECode_OK;

    switch (property_type) {
        case EFilterPropertyType_get_external_handler: {
                SQueryGetHandler *ppconfig = (SQueryGetHandler *)p_param;
                ppconfig->handler = (TPointer)((IExternalSourcePushModeES *) this);
            }
            break;

        default:
            LOGM_FATAL("BAD property 0x%08x\n", property_type);
            ret = EECode_InternalLogicalBug;
            break;
    }

    return ret;
}

void CExtAudioEsSource::GetInfo(INFO &info)
{
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = 0;
    info.nOutput = 1;

    info.pName = mpModuleName;
}

void CExtAudioEsSource::PrintStatus()
{
    LOGM_PRINTF("heart beat %d\n", mDebugHeartBeat);

    if (mpOutputPin) {
        mpOutputPin->PrintStatus();
    }

    mDebugHeartBeat = 0;
    return;
}

EECode CExtAudioEsSource::PushEs(TU8 *data, TU32 len, SEsInfo *info)
{
    if (DUnlikely((!mbSendExtraData))) {
        mbSendExtraData = 1;
        processExtraData();
    }

    CIBuffer *p_buffer = NULL;

    mpBufferPool->AllocBuffer(p_buffer, sizeof(CIBuffer));
    p_buffer->SetDataPtr(data, 0);
    p_buffer->SetDataPtrBase(data, 0);

    p_buffer->SetDataSize((TMemSize) len, 0);
    p_buffer->SetDataMemorySize((TMemSize) len, 0);

    if (info->have_pts) {
        p_buffer->SetBufferPTS(info->pts);
    }

    if (info->have_dts) {
        p_buffer->SetBufferDTS(info->dts);
    }

    p_buffer->SetBufferType(EBufferType_AudioES);

    p_buffer->SetBufferFlags(0);

    p_buffer->mpCustomizedContent = mpMemPool;
    p_buffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;

    mpOutputPin->SendBuffer(p_buffer);

    return EECode_OK;
}

EECode CExtAudioEsSource::AllocMemory(TU8 *&mem, TU32 len)
{
    mem = mpMemPool->RequestMemBlock((TMemSize) len, NULL);
    return EECode_OK;
}

EECode CExtAudioEsSource::ReturnMemory(TU8 *retm, TU32 len)
{
    mpMemPool->ReturnBackMemBlock((TMemSize) len, retm);
    return EECode_OK;
}

EECode CExtAudioEsSource::SetMediaInfo(SMediaInfo *info)
{
    if (info) {
        mAudioChannelNumber = info->audio_channel_number;
        mAudioSampleSize = info->audio_sample_size;
        mAudioSampleRate = info->audio_sample_rate;
        mAudioFrameSize = info->audio_frame_size;
        mAudioProfile = info->profile_indicator;
    } else {
        return EECode_BadParam;
    }

    return EECode_OK;
}

void CExtAudioEsSource::SendEOS()
{
    CIBuffer *p_buffer = NULL;

    mpBufferPool->AllocBuffer(p_buffer, sizeof(CIBuffer));
    p_buffer->SetDataPtr(NULL, 0);
    p_buffer->SetDataPtrBase(NULL, 0);

    p_buffer->SetDataSize(0, 0);
    p_buffer->SetDataMemorySize(0, 0);

    p_buffer->SetBufferType(EBufferType_FlowControl_EOS);

    p_buffer->mpCustomizedContent = NULL;
    p_buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;

    mpOutputPin->SendBuffer(p_buffer);
}

EECode CExtAudioEsSource::processExtraData()
{
    if (!mpExtraData) {
        mpExtraData = gfGenerateAACExtraData(mAudioSampleRate, (TU32) mAudioChannelNumber, mExtraDataSize);
    }

    if (DLikely(mpExtraData)) {

        CIBuffer *p_buffer = NULL;

        mpBufferPool->AllocBuffer(p_buffer, sizeof(CIBuffer));
        p_buffer->SetDataPtr(mpExtraData, 0);
        p_buffer->SetDataPtrBase(NULL, 0);

        p_buffer->SetBufferType(EBufferType_AudioExtraData);

        p_buffer->SetDataSize((TMemSize) mExtraDataSize, 0);
        p_buffer->SetDataMemorySize(0, 0);

        p_buffer->SetBufferFlags(CIBuffer::SYNC_POINT | CIBuffer::WITH_EXTRA_DATA);

        p_buffer->mAudioChannelNumber = mAudioChannelNumber;
        p_buffer->mAudioFrameSize = mAudioFrameSize;
        p_buffer->mAudioSampleRate = mAudioSampleRate;
        p_buffer->mAudioSampleFormat = AudioSampleFMT_S16;
        p_buffer->mAudioBitrate = mAudioBitrate;
        p_buffer->mContentFormat = (StreamFormat) mAudioFormat;

        p_buffer->mpCustomizedContent = NULL;
        p_buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;

        mpOutputPin->SendBuffer(p_buffer);
    } else {
        LOG_ERROR("CExtAudioEsSource::processExtraData, can not generate extra data\n");
        return EECode_DataCorruption;
    }

    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

