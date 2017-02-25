/*******************************************************************************
 * external_video_es_source.cpp
 *
 * History:
 *    2014/11/29 - [Zhi He] create file
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

#include "external_video_es_source.h"

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

IFilter *gfCreateExternalVideoEsSource(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CExtVideoEsSource::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

//-----------------------------------------------------------------------
//
// CExtVideoEsSource
//
//-----------------------------------------------------------------------

IFilter *CExtVideoEsSource::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
{
    CExtVideoEsSource *result = new CExtVideoEsSource(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

CObject *CExtVideoEsSource::GetObject0() const
{
    return (CObject *) this;
}

CExtVideoEsSource::CExtVideoEsSource(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
    : inherited(pName, index)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpEngineMsgSink(pEngineMsgSink)
    , mbWaitKeyFrame(1)
    , mbSendExtraData(0)
    , mpExtraData(NULL)
    , mExtraDataSize(0)
    , mExtraDataBufferSize(0)
    , mpOutputPin(NULL)
    , mpBufferPool(NULL)
    , mpMemPool(NULL)
    , mVideoWidth(0)
    , mVideoHeight(0)
    , mVideoFrameRateNum(DDefaultVideoFramerateNum)
    , mVideoFrameRateDen(DDefaultVideoFramerateDen)
    , mVideoProfileIndicator(0)
    , mVideoLevelIndicator(0)
    , mCurrentFormat(StreamFormat_H264)
{

}

EECode CExtVideoEsSource::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleExternalSourceVideoEs);
    mPersistEOSBuffer.SetBufferType(EBufferType_FlowControl_EOS);

    return EECode_OK;
}

CExtVideoEsSource::~CExtVideoEsSource()
{

}

void CExtVideoEsSource::Delete()
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
        DDBG_FREE(mpExtraData, "EVve");
        mpExtraData = NULL;
    }

    inherited::Delete();
}

EECode CExtVideoEsSource::Initialize(TChar *p_param)
{
    return EECode_OK;
}

EECode CExtVideoEsSource::Finalize()
{
    return EECode_OK;
}

EECode CExtVideoEsSource::Run()
{
    return EECode_OK;
}

EECode CExtVideoEsSource::Exit()
{
    return EECode_OK;
}

EECode CExtVideoEsSource::Start()
{
    return EECode_OK;
}

EECode CExtVideoEsSource::Stop()
{
    return EECode_OK;
}

void CExtVideoEsSource::Pause()
{
    return;
}

void CExtVideoEsSource::Resume()
{
    return;
}

void CExtVideoEsSource::Flush()
{
    return;
}

void CExtVideoEsSource::ResumeFlush()
{
    return;
}

EECode CExtVideoEsSource::Suspend(TUint release_context)
{
    return EECode_OK;
}

EECode CExtVideoEsSource::ResumeSuspend(TComponentIndex input_index)
{
    return EECode_OK;
}

EECode CExtVideoEsSource::SwitchInput(TComponentIndex focus_input_index)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

EECode CExtVideoEsSource::FlowControl(EBufferType flowcontrol_type)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

EECode CExtVideoEsSource::SendCmd(TUint cmd)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

void CExtVideoEsSource::PostMsg(TUint cmd)
{
    LOGM_FATAL("TO DO\n");
    return;
}

EECode CExtVideoEsSource::AddInputPin(TUint &index, TUint type)
{
    LOGM_FATAL("CExtVideoEsSource: no input pin\n");

    return EECode_InternalLogicalBug;
}

EECode CExtVideoEsSource::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    DASSERT(StreamType_Video == type);

    if (StreamType_Video == type) {
        if (mpOutputPin == NULL) {
            mpOutputPin = COutputPin::Create("[Video output pin for CExtVideoEsSource filter]", (IFilter *) this);

            if (!mpOutputPin)  {
                LOGM_FATAL("COutputPin::Create() fail?\n");
                return EECode_NoMemory;
            }

            mpBufferPool = CBufferPool::Create("[Buffer pool for video output pin in CExtVideoEsSource filter]", 64);
            if (!mpBufferPool)  {
                LOGM_FATAL("CBufferPool::Create() fail?\n");
                return EECode_NoMemory;
            }
            mpBufferPool->SetReleaseBufferCallBack(__release_ring_buffer);

            mpOutputPin->SetBufferPool(mpBufferPool);
            mpMemPool = CRingMemPool::Create(12 * 1024 * 1024);
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

IOutputPin *CExtVideoEsSource::GetOutputPin(TUint index, TUint sub_index)
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

IInputPin *CExtVideoEsSource::GetInputPin(TUint index)
{
    LOGM_FATAL("CExtVideoEsSource: no input pin\n");
    return NULL;
}

EECode CExtVideoEsSource::FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
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

void CExtVideoEsSource::GetInfo(INFO &info)
{
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = 0;
    info.nOutput = 1;

    info.pName = mpModuleName;
}

void CExtVideoEsSource::PrintStatus()
{
    LOGM_PRINTF("heart beat %d\n", mDebugHeartBeat);

    if (mpOutputPin) {
        LOGM_PRINTF("       mpOutputPin[%p]'s status:\n", mpOutputPin);
        mpOutputPin->PrintStatus();
    }

    mDebugHeartBeat = 0;
    return;
}

EECode CExtVideoEsSource::PushEs(TU8 *data, TU32 len, SEsInfo *info)
{
    if (DUnlikely((!mbSendExtraData) && (EPredefinedPictureType_IDR == info->frame_type))) {
        mbSendExtraData = 1;
        processExtraData(data, len);
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

    p_buffer->SetBufferType(EBufferType_VideoES);

    p_buffer->mVideoFrameType = info->frame_type;
    if (EPredefinedPictureType_IDR == info->frame_type) {
        p_buffer->SetBufferFlags(CIBuffer::KEY_FRAME | CIBuffer::SYNC_POINT);
        p_buffer->mVideoWidth = mVideoWidth;
        p_buffer->mVideoHeight = mVideoHeight;
        p_buffer->mVideoFrameRateNum = mVideoFrameRateNum;
        p_buffer->mVideoFrameRateDen = mVideoFrameRateDen;
        p_buffer->mVideoOffsetX = 0;
        p_buffer->mVideoOffsetY = 0;
        p_buffer->mVideoSampleAspectRatioNum = 1;
        p_buffer->mVideoSampleAspectRatioDen = 1;
        p_buffer->mVideoProfileIndicator = mVideoProfileIndicator;
        p_buffer->mVideoLevelIndicator = mVideoLevelIndicator;
    } else {
        p_buffer->SetBufferFlags(0);
    }

    p_buffer->mContentFormat = mCurrentFormat;
    p_buffer->mpCustomizedContent = mpMemPool;
    p_buffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;

    p_buffer->mSubPacketNumber = info->sub_packet_number;
    TU32 i = 0;
    for (i = 0; i < info->sub_packet_number; i ++) {
        p_buffer->mOffsetHint[i] = info->offset_hint[i];
    }

    mpOutputPin->SendBuffer(p_buffer);

    return EECode_OK;
}

EECode CExtVideoEsSource::AllocMemory(TU8 *&mem, TU32 len)
{
    mem = mpMemPool->RequestMemBlock((TMemSize) len, NULL);
    return EECode_OK;
}

EECode CExtVideoEsSource::ReturnMemory(TU8 *retm, TU32 len)
{
    mpMemPool->ReturnBackMemBlock((TMemSize) len, retm);
    return EECode_OK;
}

EECode CExtVideoEsSource::SetMediaInfo(SMediaInfo *info)
{
    if (info) {
        mVideoWidth = info->video_width;
        mVideoHeight = info->video_height;
        postVideoSizeMsg(mVideoWidth, mVideoHeight);
        if (info->video_framerate_num && info->video_framerate_den) {
            mVideoFrameRateNum = info->video_framerate_num;
            mVideoFrameRateDen = info->video_framerate_den;
        }
        mVideoProfileIndicator = info->profile_indicator;
        mVideoLevelIndicator = info->level_indicator;

        mCurrentFormat = (StreamFormat) info->format;
        if ((StreamFormat_H264 != mCurrentFormat) && (StreamFormat_H265 != mCurrentFormat)) {
            LOG_ERROR("CExtVideoEsSource::SetMediaInfo: not h264 or h265? %d\n", mCurrentFormat);
            return EECode_BadParam;
        }
    } else {
        return EECode_BadParam;
    }

    return EECode_OK;
}

void CExtVideoEsSource::SendEOS()
{
    CIBuffer *p_buffer = NULL;

    mpBufferPool->AllocBuffer(p_buffer, sizeof(CIBuffer));
    p_buffer->SetDataPtr(NULL, 0);
    p_buffer->SetDataPtrBase(NULL, 0);

    p_buffer->SetDataSize(0, 0);
    p_buffer->SetDataMemorySize(0, 0);

    p_buffer->SetBufferType(EBufferType_FlowControl_EOS);

    p_buffer->mContentFormat = mCurrentFormat;
    p_buffer->mpCustomizedContent = NULL;
    p_buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;

    mpOutputPin->SendBuffer(p_buffer);
}

EECode CExtVideoEsSource::processExtraData(TU8 *data, TU32 len)
{
    TU8 *pextra = NULL;
    TU32 extrasize = 0;

    EECode err = EECode_OK;

    if (StreamFormat_H264 == mCurrentFormat) {
        err = gfGetH264Extradata(data, len, pextra, extrasize);
    } else if (StreamFormat_H265 == mCurrentFormat) {
        err = gfGetH265Extradata(data, len, pextra, extrasize);
    } else {
        LOG_ERROR("CExtVideoEsSource::processExtraData: not h264 or h265? %d\n", mCurrentFormat);
        return EECode_BadParam;
    }

    if (DLikely(EECode_OK == err)) {
        if ((extrasize > mExtraDataBufferSize) || (!mpExtraData)) {
            if (mpExtraData) {
                DDBG_FREE(mpExtraData, "EVve");
                mpExtraData = NULL;
            }
            mpExtraData = (TU8 *) DDBG_MALLOC(extrasize, "EVve");
            if (DLikely(mpExtraData)) {
                mExtraDataBufferSize = extrasize;
            } else {
                LOG_ERROR("CExtVideoEsSource::processExtraData, no memory\n");
                return EECode_NoMemory;
            }
        }

        memcpy(mpExtraData, pextra, extrasize);
        mExtraDataSize = extrasize;

        CIBuffer *p_buffer = NULL;

        mpBufferPool->AllocBuffer(p_buffer, sizeof(CIBuffer));
        p_buffer->SetDataPtr(pextra, 0);
        p_buffer->SetDataPtrBase(NULL, 0);

        p_buffer->SetBufferType(EBufferType_VideoExtraData);

        p_buffer->SetDataSize((TMemSize) extrasize, 0);
        p_buffer->SetDataMemorySize(0, 0);

        p_buffer->SetBufferFlags(CIBuffer::SYNC_POINT | CIBuffer::WITH_EXTRA_DATA);

        p_buffer->mContentFormat = mCurrentFormat;

        p_buffer->mVideoWidth = mVideoWidth;
        p_buffer->mVideoHeight = mVideoHeight;
        p_buffer->mVideoFrameRateNum = mVideoFrameRateNum;
        p_buffer->mVideoFrameRateDen = mVideoFrameRateDen;
        p_buffer->mVideoOffsetX = 0;
        p_buffer->mVideoOffsetY = 0;
        p_buffer->mVideoSampleAspectRatioNum = 1;
        p_buffer->mVideoSampleAspectRatioDen = 1;
        p_buffer->mVideoProfileIndicator = mVideoProfileIndicator;
        p_buffer->mVideoLevelIndicator = mVideoLevelIndicator;

        p_buffer->mpCustomizedContent = NULL;
        p_buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;

        mpOutputPin->SendBuffer(p_buffer);
    } else {
        LOG_ERROR("CExtVideoEsSource::processExtraData, do not find extra data\n");
        gfPrintMemory(data, 256);
        return EECode_DataCorruption;
    }

    return EECode_OK;
}

void CExtVideoEsSource::postVideoSizeMsg(TU32 width, TU32 height)
{
    SMSG msg;
    memset(&msg, 0x0, sizeof(msg));
    msg.code = EMSGType_VideoSize;
    msg.p1 = width;
    msg.p2 = height;
    if (mpEngineMsgSink) {
        mpEngineMsgSink->MsgProc(msg);
    }
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

