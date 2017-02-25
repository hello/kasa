/*******************************************************************************
 * framework_interface.cpp
 *
 * History:
 *  2012/12/07 - [Zhi He] create file
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
#include "media_mw_utils.h"

#include "framework_interface.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN

CIBuffer::CIBuffer()
    : mRefCount(0)
    , mpNext(NULL)
    , mBufferType(EBufferType_Invalid)
    , mVideoFrameType(0)
    , mbAudioPCMChannelInterlave(0)
    , mAudioChannelNumber(0)
    , mAudioSampleFormat(0)
    , mContentFormat(0)
    , mFlags(0)
    , mCustomizedContentFormat(0)
    , mCustomizedContentType(EBufferCustomizedContentType_Invalid)
    , mpCustomizedContent(NULL)
    , mSubPacketNumber(0)
    , mpPool(NULL)
    , mAudioBitrate(0)
    , mVideoBitrate(0)
    , mPTS(0)
    , mVideoWidth(0)
    , mVideoHeight(0)
    , mVideoOffsetX(0)
    , mVideoOffsetY(0)
    , mAudioSampleRate(0)
    , mAudioFrameSize(0)
    , mBufferSeqNumber0(0)
    , mBufferSeqNumber1(0)
    , mExpireTime(0)
    , mBufferId(0)
    , mEncId(0)
{
    TUint i = 0;
    for (i = 0; i < MEMORY_BLOCK_NUMBER; i++) {
        mpData[i] = NULL;
        mpMemoryBase[i] = NULL;
        mMemorySize[i] = 0;
        mDataSize[i] = 0;
    }

    for (i = 0; i < MEMORY_BLOCK_NUMBER; i++) {
        mVideoBufferLinesize[i] = 0;
    }

    for (i = 0; i < DMAX_MEDIA_SUB_PACKET_NUMBER; i++) {
        mOffsetHint[i] = 0;
    }
}

CIBuffer::~CIBuffer()
{
}

void CIBuffer::AddRef()
{
    if (mpPool) {
        mpPool->AddRef(this);
    }
}

void CIBuffer::Release()
{
    if (mpPool) {
        mpPool->Release(this);
    }
}

void CIBuffer::ReleaseAllocatedMemory()
{
    if (mbAllocated) {
        mbAllocated = 0;
        TU32 i = 0;
        for (i = 0; i < MEMORY_BLOCK_NUMBER; i ++) {
            if (mpMemoryBase[i]) {
                DDBG_FREE(mpMemoryBase[i], "BPBM");
                mpMemoryBase[i] = NULL;
                mMemorySize[i] = 0;
            }
        }
    }
}

EBufferType CIBuffer::GetBufferType() const
{
    return mBufferType;
}

void CIBuffer::SetBufferType(EBufferType type)
{
    mBufferType = type;
}

TUint CIBuffer::GetBufferFlags() const
{
    return mFlags;
}

void CIBuffer::SetBufferFlags(TUint flags)
{
    mFlags = flags;
}

void CIBuffer::SetBufferFlagBits(TUint flags)
{
    mFlags |= flags;
}

void CIBuffer::ClearBufferFlagBits(TUint flags)
{
    mFlags &= ~flags;
}

TU8 *CIBuffer::GetDataPtr(TUint index) const
{
    if (MEMORY_BLOCK_NUMBER <= index) {
        LOG_ERROR("BAD input index %d\n", index);
        return NULL;
    }

    return mpData[index];
}

void CIBuffer::SetDataPtr(TU8 *pdata, TUint index)
{
    if (MEMORY_BLOCK_NUMBER <= index) {
        LOG_ERROR("BAD input index %d\n", index);
        return;
    }

    mpData[index] = pdata;
}

TMemSize CIBuffer::GetDataSize(TUint index) const
{
    if (MEMORY_BLOCK_NUMBER <= index) {
        LOG_ERROR("BAD input index %d\n", index);
        return 0;
    }

    return mDataSize[index];
}

void CIBuffer::SetDataSize(TMemSize size, TUint index)
{
    if (MEMORY_BLOCK_NUMBER <= index) {
        LOG_ERROR("BAD input index %d\n", index);
        return;
    }

    mDataSize[index] = size;
}

TU32 CIBuffer::GetDataLineSize(TUint index) const
{
    if (MEMORY_BLOCK_NUMBER <= index) {
        LOG_ERROR("BAD input index %d\n", index);
        return 0;
    }

    return mDataLineSize[index];
}

void CIBuffer::SetDataLineSize(TU32 size, TUint index)
{
    if (MEMORY_BLOCK_NUMBER <= index) {
        LOG_ERROR("BAD input index %d\n", index);
        return;
    }

    mDataLineSize[index] = size;
}

TU8 *CIBuffer::GetDataPtrBase(TUint index) const
{
    if (MEMORY_BLOCK_NUMBER <= index) {
        LOG_ERROR("BAD input index %d\n", index);
        return NULL;
    }

    return mpMemoryBase[index];
}

void CIBuffer::SetDataPtrBase(TU8 *pdata, TUint index)
{
    if (MEMORY_BLOCK_NUMBER <= index) {
        LOG_ERROR("BAD input index %d\n", index);
        return;
    }

    mpMemoryBase[index] = pdata;
}

TMemSize CIBuffer::GetDataMemorySize(TUint index) const
{
    if (MEMORY_BLOCK_NUMBER <= index) {
        LOG_ERROR("BAD input index %d\n", index);
        return 0;
    }

    return mMemorySize[index];
}

void CIBuffer::SetDataMemorySize(TMemSize size, TUint index)
{
    if (MEMORY_BLOCK_NUMBER <= index) {
        LOG_ERROR("BAD input index %d\n", index);
        return;
    }

    mMemorySize[index] = size;
}

TTime CIBuffer::GetBufferPTS() const
{
    return mPTS;
}

void CIBuffer::SetBufferPTS(TTime pts)
{
    mPTS = pts;
}

TTime CIBuffer::GetBufferDTS() const
{
    return mDTS;
}

void CIBuffer::SetBufferDTS(TTime dts)
{
    mDTS = dts;
}

TTime CIBuffer::GetBufferNativePTS() const
{
    return mNativePTS;
}

void CIBuffer::SetBufferNativePTS(TTime pts)
{
    mNativePTS = pts;
}

TTime CIBuffer::GetBufferNativeDTS() const
{
    return mNativeDTS;
}

void CIBuffer::SetBufferNativeDTS(TTime dts)
{
    mNativeDTS = dts;
}

TTime CIBuffer::GetBufferLinearPTS() const
{
    return mLinearPTS;
}

void CIBuffer::SetBufferLinearPTS(TTime pts)
{
    mLinearPTS = pts;
}

TTime CIBuffer::GetBufferLinearDTS() const
{
    return mLinearDTS;
}

void CIBuffer::SetBufferLinearDTS(TTime dts)
{
    mLinearDTS = dts;
}

//-----------------------------------------------------------------------
//
//  CBufferPool
//
//-----------------------------------------------------------------------
CBufferPool *CBufferPool::Create(const TChar *name, TUint max_count)
{
    CBufferPool *result = new CBufferPool(name);
    if (result && result->Construct(max_count) != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

void CBufferPool::PrintStatus()
{
    TUint v = 0;

    DASSERT(mpBufferQ);
    if (mpBufferQ) {
        v = mpBufferQ->GetDataCnt();
    }
    LOGM_PRINTF("free buffer count %d.\n", v);
}

void CBufferPool::Delete()
{
    LOGM_RESOURCE("buffer pool '%s' destroyed\n", mpName);
    if (mpBufferQ) {
        mpBufferQ->Delete();
        mpBufferQ = NULL;
    }

    destroyBuffers();

    if (mpBufferMemory) {
        DDBG_FREE(mpBufferMemory, "MFBP");
        mpBufferMemory = NULL;
    }

    inherited::Delete();
}

CBufferPool::CBufferPool(const char *pName)
    : inherited(pName)
    , mpBufferQ(NULL)
    , mRefCount(1)
    , mpName(pName)
    , mpEventListener(NULL)
    , mpReleaseBufferCallBack(NULL)
    , mpBufferMemory(NULL)
{
}

const TChar *CBufferPool::GetName() const
{
    return mpName;
}

TUint CBufferPool::GetFreeBufferCnt() const
{
    DASSERT(mpBufferQ);
    if (mpBufferQ) {
        return mpBufferQ->GetDataCnt();
    }

    return 0;
}

void CBufferPool::AddBufferNotifyListener(IEventListener *p_listener)
{
    //DASSERT(!mpEventListener);
    mpEventListener = p_listener;
}

void CBufferPool::OnReleaseBuffer(CIBuffer *pBuffer)
{
    if (mpReleaseBufferCallBack) {
        mpReleaseBufferCallBack(pBuffer);
    }
}

EECode CBufferPool::Construct(TUint nMaxBuffers)
{
    EECode err = EECode_OK;

    if ((mpBufferQ = CIQueue::Create(NULL, this, sizeof(CIBuffer *), nMaxBuffers)) == NULL) {
        LOGM_FATAL("CBufferPool::Construct, this %p, nMaxBuffers %d, no memory!\n", this, nMaxBuffers);
        return EECode_NoMemory;
    }

    if (nMaxBuffers) {
        mpBufferMemory = (TU8 *) DDBG_MALLOC(sizeof(CIBuffer) * nMaxBuffers, "MFBP");
        if (mpBufferMemory == NULL) {
            return EECode_NoMemory;
        }

        mnTotalBufferNumber = nMaxBuffers;
        memset(mpBufferMemory, 0x0, sizeof(CIBuffer) * nMaxBuffers);

        CIBuffer *pBuffer = (CIBuffer *)mpBufferMemory;
        for (TUint i = 0; i < mnTotalBufferNumber; i++, pBuffer++) {
            pBuffer->mpPool = this;
            err = mpBufferQ->PostMsg(&pBuffer, sizeof(pBuffer));
            DASSERT_OK(err);
        }

        LOGM_INFO("count %d, mpBufferQ %p, CBufferPool %p\n", mpBufferQ->GetDataCnt(), mpBufferQ, this);
    }

    return EECode_OK;
}

CBufferPool::~CBufferPool()
{
}

void CBufferPool::Enable(bool bEnabled)
{
    mpBufferQ->Enable(bEnabled);
}

bool CBufferPool::AllocBuffer(CIBuffer *&pBuffer, TUint size)
{
    if (mpBufferQ->GetMsgEx(&pBuffer, sizeof(pBuffer))) {
        pBuffer->mRefCount = 1;
        pBuffer->mpPool = this;
        pBuffer->mpNext = NULL;
        pBuffer->mFlags = 0;
        return true;
    }
    return false;
}

void CBufferPool::AddRef(CIBuffer *pBuffer)
{
    __atomic_inc(&pBuffer->mRefCount);
}

void CBufferPool::Release(CIBuffer *pBuffer)
{
    //LOGM_VERBOSE("--mRefCount:%d\n",pBuffer->mRefCount);
    if (__atomic_dec(&pBuffer->mRefCount) == 1) {
        OnReleaseBuffer(pBuffer);
        EECode err = mpBufferQ->PostMsg((void *)&pBuffer, sizeof(CIBuffer *));
        if (mpEventListener) {
            mpEventListener->EventNotify(EEventType_BufferReleaseNotify, 0, (TPointer)this);
        }
        DASSERT_OK(err);
    }
    //LOGM_VERBOSE("--after Release\n");
}

void CBufferPool::SetReleaseBufferCallBack(TBufferPoolReleaseBufferCallBack callback)
{
    mpReleaseBufferCallBack = callback;
}

void CBufferPool::destroyBuffers()
{
    TU32 i = 0, j = 0;
    CIBuffer *pBuffer = (CIBuffer *)mpBufferMemory;
    for (i = 0; i < mnTotalBufferNumber; i++, pBuffer++) {
        if (pBuffer->mbAllocated) {
            pBuffer->mbAllocated = 0;
            for (j = 0; j < CIBuffer::MEMORY_BLOCK_NUMBER; j++) {
                if (pBuffer->mpMemoryBase[j]) {
                    DDBG_FREE(pBuffer->mpMemoryBase[j], "BPBM");
                    pBuffer->mpMemoryBase[j] = NULL;
                    pBuffer->mMemorySize[j] = 0;
                }
            }
        }
    }
}

CActiveObject::CActiveObject(const TChar *pName, TUint index)
    : inherited(pName, index)
    , mpWorkQ(NULL)
    , mbRun(1)
    , mbPaused(0)
    , mbTobeStopped(0)
    , mbTobeSuspended(0)
{
}

EECode CActiveObject::Construct()
{
    if (NULL == (mpWorkQ = CIWorkQueue::Create((IActiveObject *)this))) {
        LOGM_FATAL("CActiveObject::Construct: CIWorkQueue::Create fail\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CActiveObject::~CActiveObject()
{
    LOGM_RESOURCE("~CActiveObject\n");
    if (mpWorkQ) {
        mpWorkQ->Delete();
        mpWorkQ = NULL;
    }
    LOGM_RESOURCE("~CActiveObject Done\n");
}

void CActiveObject::Delete()
{
    inherited::Delete();
}

CObject *CActiveObject::GetObject() const
{
    return (CObject *) this;
}

EECode CActiveObject::SendCmd(TUint cmd)
{
    return mpWorkQ->SendCmd(cmd);
}

EECode CActiveObject::PostCmd(TUint cmd)
{
    return mpWorkQ->PostMsg(cmd);
}

void CActiveObject::OnRun()
{
}

CIQueue *CActiveObject::MsgQ()
{
    return mpWorkQ->MsgQ();
}

void CActiveObject::GetCmd(SCMD &cmd)
{
    mpWorkQ->GetCmd(cmd);
}

bool CActiveObject::PeekCmd(SCMD &cmd)
{
    return mpWorkQ->PeekCmd(cmd);
}

void CActiveObject::CmdAck(EECode err)
{
    mpWorkQ->CmdAck(err);
}

CFilter::CFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
    : inherited(pName)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpEngineMsgSink(pEngineMsgSink)
{
}

CFilter::~CFilter()
{
}

void CFilter::Delete()
{
    inherited::Delete();
}

void CFilter::Pause()
{
    LOGM_ERROR("please implement this api.\n");
}

void CFilter::Resume()
{
    LOGM_ERROR("please implement this api.\n");
}

void CFilter::Flush()
{
    LOGM_ERROR("please implement this api.\n");
}

void CFilter::PrintState()
{
    LOGM_ERROR("please implement this api.\n");
}

EECode CFilter::Start()
{
    return EECode_OK;
}

EECode CFilter::SendCmd(TUint cmd)
{
    LOGM_ERROR("please implement this api.\n");
    return EECode_NoImplement;
}

void CFilter::PostMsg(TUint cmd)
{
    LOGM_ERROR("please implement this api.\n");
}

EECode CFilter::FlowControl(EBufferType type)
{
    LOGM_ERROR("please implement this api, type %d.\n", type);
    return EECode_NoImplement;
}

IInputPin *CFilter::GetInputPin(TUint index)
{
    LOGM_ERROR("please implement this api.\n");
    return NULL;
}

IOutputPin *CFilter::GetOutputPin(TUint index, TUint sub_index)
{
    LOGM_ERROR("!!!must not comes here, not implement.\n");
    return NULL;
}

EECode CFilter::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    LOGM_ERROR("!!!must not comes here, not implement.\n");
    return EECode_NoImplement;
}

EECode CFilter::AddInputPin(TUint &index, TUint type)
{
    LOGM_ERROR("!!!must not comes here, not implement.\n");
    return EECode_NoImplement;
}

void CFilter::EventNotify(EEventType type, TUint param1, TPointer param2)
{
    LOGM_ERROR("no event listener.\n");
    return;
}

EECode CFilter::PostEngineMsg(SMSG &msg)
{
    if (mpEngineMsgSink) {
        return mpEngineMsgSink->MsgProc(msg);
    } else {
        LOGM_ERROR("NULL mpEngineMsgSink, in CFilter::PostEngineMsg\n");
    }
    return EECode_BadState;
}

EECode CFilter::PostEngineMsg(TUint code)
{
    SMSG msg;
    msg.code = code;

    msg.p1  = (TULong)static_cast<IFilter *>(this);
    return PostEngineMsg(msg);
}

//-----------------------------------------------------------------------
//
//  CInputPin
//
//-----------------------------------------------------------------------

CInputPin::CInputPin(const TChar *name, IFilter *pFilter, StreamType type)
    : inherited(name)
    , mpFilter(pFilter)
    , mpPeer(NULL)
    , mpBufferPool(NULL)
    , mPeerSubIndex(0)
    , mbEnable(1)
    , mbSyncedBufferComes(0)
    , mpMutex(NULL)
    , mpExtraData(NULL)
    , mExtraDataSize(0)
    , mExtraDataBufferSize(0)
    , mPinType(type)
    , mCodecFormat(StreamFormat_Invalid)
    , mVideoWidth(0)
    , mVideoHeight(0)
    , mVideoFrameRateNum(30)
    , mVideoFrameRateDen(1)
    , mVideoOffsetX(0)
    , mVideoOffsetY(0)
    , mVideoSampleAspectRatioNum(1)
    , mVideoSampleAspectRatioDen(1)
    , mAudioSampleRate(0)
    , mAudioFrameSize(0)
    , mAudioChannelNumber(2)
    , mAudioLayout(0)
    , mSessionNumber(0)
{
    mpMutex = gfCreateMutex();
    if (!mpMutex) {
        LOG_FATAL("CInputPin: gfCreateMutex() fail\n");
    }
}

void CInputPin::Delete()
{
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    if (mpExtraData) {
        DDBG_FREE(mpExtraData, "MIEX");
        mpExtraData = NULL;
    }

    mExtraDataSize = 0;
    mExtraDataBufferSize = 0;

    inherited::Delete();
}

CInputPin::~CInputPin()
{

}

void CInputPin::PrintStatus()
{
    LOGM_PRINTF("[CInputPin::PrintStatus()], mPeerSubIndex %d, mbEnable %d\n", mPeerSubIndex, mbEnable);
}

void CInputPin::Enable(TU8 bEnable)
{
    AUTO_LOCK(mpMutex);
    mbEnable = bEnable;
}

IOutputPin *CInputPin::GetPeer() const
{
    return mpPeer;
}

IFilter *CInputPin::GetFilter() const
{
    return mpFilter;
}

EECode CInputPin::Connect(IOutputPin *pPeer, TComponentIndex index)
{
    DASSERT(!mpPeer);

    if (!mpPeer) {
        mpPeer = pPeer;
        mPeerSubIndex = index;

        DASSERT(!mpBufferPool);

        mpBufferPool = mpPeer->GetBufferPool();
        if (!mpBufferPool) {
            //LOGM_FATAL("NULL mpBufferPool %p\n", mpBufferPool);
            //return EECode_InternalLogicalBug;
        }

        return EECode_OK;
    }

    LOGM_FATAL("already connected!\n");
    return EECode_InternalLogicalBug;
}

EECode CInputPin::Disconnect(TComponentIndex index)
{
    DASSERT(mpPeer);
    DASSERT(index == mPeerSubIndex);

    if (mpPeer) {
        mpPeer = NULL;

        DASSERT(mpBufferPool);
        mpBufferPool = NULL;
        return EECode_OK;
    }

    LOGM_FATAL("not connected!\n");
    return EECode_InternalLogicalBug;
}

StreamType CInputPin::GetPinType() const
{
    return mPinType;
}

EECode CInputPin::GetVideoParams(StreamFormat &format, TU32 &video_width, TU32 &video_height, TU8 *&p_extra_data, TMemSize &extra_data_size, TU32 &session_number) const
{
    AUTO_LOCK(mpMutex);

    if (DLikely(StreamType_Video == mPinType)) {
        if (DLikely(mbSyncedBufferComes)) {
            format = mCodecFormat;
            video_width = mVideoWidth;
            video_height = mVideoHeight;
            p_extra_data = mpExtraData;
            extra_data_size = mExtraDataSize;
            session_number = mSessionNumber;
        } else {
            LOGM_FATAL("sync buffer not comes yet?\n");
            return EECode_BadState;
        }
    } else {
        LOGM_FATAL("it's not a video pin\n");
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CInputPin::GetAudioParams(StreamFormat &format, TU32 &channel_number, TU32 &sample_frequency, AudioSampleFMT &fmt, TU8 *&p_extra_data, TMemSize &extra_data_size, TU32 &session_number) const
{
    AUTO_LOCK(mpMutex);

    if (DLikely(StreamType_Audio == mPinType)) {
        if (DLikely(mbSyncedBufferComes)) {
            format = mCodecFormat;
            channel_number = mAudioChannelNumber;
            sample_frequency = mAudioSampleRate;
            fmt = mAudioSampleFmt;
            p_extra_data = mpExtraData;
            extra_data_size = mExtraDataSize;
            session_number = mSessionNumber;
        } else {
            LOGM_WARN("sync buffer not comes yet?\n");
            return EECode_BadState;
        }
    } else {
        LOGM_FATAL("it's not a audio pin\n");
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CInputPin::GetExtraData(TU8 *&p_extra_data, TMemSize &extra_data_size, TU32 &session_number) const
{
    AUTO_LOCK(mpMutex);
    p_extra_data = mpExtraData;
    extra_data_size = mExtraDataSize;
    session_number = mSessionNumber;
    return EECode_OK;
}

void CInputPin::updateSyncBuffer(CIBuffer *p_sync_Buffer)
{
    if (StreamType_Video == mPinType) {
        mVideoWidth = p_sync_Buffer->mVideoWidth;
        mVideoHeight = p_sync_Buffer->mVideoHeight;
        mVideoFrameRateNum = p_sync_Buffer->mVideoFrameRateNum;
        mVideoFrameRateDen = p_sync_Buffer->mVideoFrameRateDen;
        mVideoOffsetX = p_sync_Buffer->mVideoOffsetX;
        mVideoOffsetY = p_sync_Buffer->mVideoOffsetY;
        mVideoSampleAspectRatioNum = p_sync_Buffer->mVideoSampleAspectRatioNum;
        mVideoSampleAspectRatioDen = p_sync_Buffer->mVideoSampleAspectRatioDen;
    } else if (StreamType_Audio == mPinType) {
        mAudioSampleRate = p_sync_Buffer->mAudioSampleRate;
        mAudioFrameSize = p_sync_Buffer->mAudioFrameSize;
        mAudioChannelNumber = p_sync_Buffer->mAudioChannelNumber;
    }

    mSessionNumber = p_sync_Buffer->mSessionNumber;

}

void CInputPin::updateExtraData(CIBuffer *p_extradata_Buffer)
{
    mSessionNumber = p_extradata_Buffer->mSessionNumber;

    TU8 *p = p_extradata_Buffer->GetDataPtr();
    TUint size = p_extradata_Buffer->GetDataSize();
    if (DLikely(p && size)) {
        if (mpExtraData && (size > mExtraDataBufferSize)) {
            DDBG_FREE(mpExtraData, "FIEX");
            mpExtraData = NULL;
            mExtraDataBufferSize = 0;
        }

        if (!mpExtraData) {

            mpExtraData = (TU8 *) DDBG_MALLOC(size, "FIEX");
            if (DLikely(mpExtraData)) {
                mExtraDataBufferSize = size;
            }
        }

        if (mpExtraData) {
            memcpy(mpExtraData, p, size);
            mExtraDataSize = size;
            LOG_INFO("inputPin %p received %s ExtraData size %d, first 8 bytes: 0x%02x 0x%02x 0x%02x 0x%02x, 0x%02x 0x%02x 0x%02x 0x%02x\n", this, (EBufferType_VideoExtraData == p_extradata_Buffer->GetBufferType()) ? "video" : "audio", size, \
                     p[0], p[1], p[2], p[3], \
                     p[4], p[5], p[6], p[7]);
        } else {
            LOG_FATAL("inputPin %p received %s ExtraData %p size %u, NO memory to save it.\n", this, (EBufferType_VideoExtraData == p_extradata_Buffer->GetBufferType()) ? "video" : "audio", p, size);
        }
    } else {
        LOG_FATAL("inputPin %p received %s ExtraData %p size %u, can't save.\n", this, (EBufferType_VideoExtraData == p_extradata_Buffer->GetBufferType()) ? "video" : "audio", p, size);
    }

}

//-----------------------------------------------------------------------
//
//  COutputPin
//
//-----------------------------------------------------------------------

COutputPin::COutputPin(const TChar *pname, IFilter *pFilter)
    : inherited(pname)
    , mpFilter(pFilter)
    , mpBufferPool(NULL)
    , mbSetBP(0)
    , mbEnabledAllSubpin(1)
    , mnCurrentSubPinNumber(0)
{
    TUint i = 0;
    for (i = 0; i < DMAX_CONNECTIONS_OUTPUTPIN; i ++) {
        mpPeers[i] = NULL;
        //mbEnabled[i] = 0;
    }
}

COutputPin::~COutputPin()
{
}

void COutputPin::PrintStatus()
{
    //TUint i = 0;

    LOGM_PRINTF("subpin number %d\n", mnCurrentSubPinNumber);
    //for (i = 0; i < mnCurrentSubPinNumber; i ++) {
    //LOGM_PRINTF("sub[%d], peer %p\n", i, mpPeers[i]);
    //}
}


COutputPin *COutputPin::Create(const TChar *pname, IFilter *pfilter)
{
    COutputPin *result = new COutputPin(pname, pfilter);

#if 0
    if (result && result->Construct() != EECode_OK) {
        LOGM_FATAL("COutputPin::Construct() fail.\n");
        delete result;
        result = NULL;
    }
#endif

    return result;
}

bool COutputPin::AllocBuffer(CIBuffer *&pBuffer, TUint size)
{
    DASSERT(mpBufferPool);
    if (mpBufferPool) {
        return mpBufferPool->AllocBuffer(pBuffer, size);
    } else {
        LOGM_FATAL("NULL pointer in COutputPin::AllocBuffer\n");
        return false;
    }
}

void COutputPin::SendBuffer(CIBuffer *pBuffer)
{
    if (mbEnabledAllSubpin) {
        TU16 index = 0;

        for (index = 0; index < mnCurrentSubPinNumber; index ++) {
            //if (mbEnabled[index] && mpPeers[index]) {
            pBuffer->AddRef();
            mpPeers[index]->Receive(pBuffer);
            //}
        }
    }

    pBuffer->Release();
}

/*
void COutputPin::Enable(TUint index, TU8 bEnable)
{
    if (index < mnCurrentSubPinNumber) {
        mbEnabled[index] = bEnable;
    } else {
        LOGM_FATAL("BAD index %u, exceed max %hu\n", index, mnCurrentSubPinNumber);
    }
}
*/

void COutputPin::EnableAllSubPin(TU8 bEnable)
{
    mbEnabledAllSubpin = bEnable;
}

IInputPin *COutputPin::GetPeer(TUint index)
{
    if (index < mnCurrentSubPinNumber) {
        return mpPeers[index];
    } else {
        LOGM_FATAL("BAD index %u, exceed max %hu\n", index, mnCurrentSubPinNumber);
    }

    return NULL;
}

IFilter *COutputPin::GetFilter()
{
    return mpFilter;
}

EECode COutputPin::Connect(IInputPin *pPeer, TUint index)
{
    DASSERT(mnCurrentSubPinNumber >= index);
    DASSERT(DMAX_CONNECTIONS_OUTPUTPIN >= mnCurrentSubPinNumber);

    if ((mnCurrentSubPinNumber >= index) && (DMAX_CONNECTIONS_OUTPUTPIN >= mnCurrentSubPinNumber)) {
        DASSERT(!mpPeers[index]);
        if (!mpPeers[index]) {
            mpPeers[index] = pPeer;
            //mbEnabled[index] = 1;
            return EECode_OK;
        } else {
            LOGM_FATAL("already connected? COutputPin this %p, mnCurrentSubPinNumber %d, request index %d, mpBufferPool %p, mpPeers[index] %p\n", this, mnCurrentSubPinNumber, index, mpBufferPool, mpPeers[index]);
            return EECode_InternalLogicalBug;
        }
    }

    LOGM_FATAL("BAD input parameters index %d, mnCurrentSubPinNumber %d, DMAX_CONNECTIONS_OUTPUTPIN %d\n", index, mnCurrentSubPinNumber, DMAX_CONNECTIONS_OUTPUTPIN);
    return EECode_BadParam;
}

EECode COutputPin::Disconnect(TUint index)
{
    DASSERT(mnCurrentSubPinNumber > index);
    DASSERT(DMAX_CONNECTIONS_OUTPUTPIN >= mnCurrentSubPinNumber);

    if ((mnCurrentSubPinNumber > index) && (DMAX_CONNECTIONS_OUTPUTPIN >= mnCurrentSubPinNumber)) {
        DASSERT(mpPeers[index]);
        if (mpPeers[index]) {
            mpPeers[index] = NULL;
            //mbEnabled[index] = 0;
            return EECode_OK;
        } else {
            LOGM_FATAL("not connected? COutputPin this %p, mnCurrentSubPinNumber %d, request index %d, mpBufferPool %p, mpPeers[index] %p\n", this, mnCurrentSubPinNumber, index, mpBufferPool, mpPeers[index]);
            return EECode_InternalLogicalBug;
        }
    }

    LOGM_FATAL("BAD input parameters index %d, mnCurrentSubPinNumber %d, DMAX_CONNECTIONS_OUTPUTPIN %d\n", index, mnCurrentSubPinNumber, DMAX_CONNECTIONS_OUTPUTPIN);
    return EECode_BadParam;
}

TUint COutputPin::NumberOfPins()
{
    return mnCurrentSubPinNumber;
}

EECode COutputPin::AddSubPin(TUint &sub_index)
{
    if (DMAX_CONNECTIONS_OUTPUTPIN > mnCurrentSubPinNumber) {
        sub_index = mnCurrentSubPinNumber;
        mnCurrentSubPinNumber ++;
    } else {
        LOGM_FATAL("COutputPin::AddSubPin(), too many sub pins(%d) now\n", mnCurrentSubPinNumber);
        return EECode_TooMany;
    }
    return EECode_OK;
}

IBufferPool *COutputPin::GetBufferPool() const
{
    return mpBufferPool;
}

void COutputPin::SetBufferPool(IBufferPool *pBufferPool)
{
    DASSERT(!mpBufferPool);
    if (!mpBufferPool) {
        mpBufferPool = pBufferPool;
    } else {
        LOG_FATAL("already set buffer pool!\n");
    }
}

//-----------------------------------------------------------------------
//
//  CQueueInputPin
//
//-----------------------------------------------------------------------

CQueueInputPin *CQueueInputPin::Create(const TChar *pname, IFilter *pfilter, StreamType type, CIQueue *pMsgQ)
{
    CQueueInputPin *result = new CQueueInputPin(pname, pfilter, type);
    if (result && result->Construct(pMsgQ) != EECode_OK) {
        LOG_FATAL("CQueueInputPin::Construct() fail.\n");
        delete result;
        result = NULL;
    }
    return result;
}

CQueueInputPin::CQueueInputPin(const TChar *name, IFilter *filter, StreamType type)
    : inherited(name, filter, type)
    , mpBufferQ(NULL)
{
}

EECode CQueueInputPin::Construct(CIQueue *pMsgQ)
{
    if ((mpBufferQ = CIQueue::Create(pMsgQ, this, sizeof(CIBuffer *), 16)) == NULL) {
        LOGM_FATAL("CQueueInputPin::Construct: CIQueue::Create fail.\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

void CQueueInputPin::PrintStatus()
{
    LOGM_PRINTF("\t\t[PrintStatus]: CQueueInputPin: mPeerSubIndex %d, mbEnable %d, mpBufferQ's data count %d\n", mPeerSubIndex, mbEnable, mpBufferQ->GetDataCnt());
}

CQueueInputPin::~CQueueInputPin()
{
    if (mpBufferQ) {
        mpBufferQ->Delete();
        mpBufferQ = NULL;
    }
}

void CQueueInputPin::Delete()
{
    if (mpBufferQ) {
        mpBufferQ->Delete();
        mpBufferQ = NULL;
    }

    inherited::Delete();
}

void CQueueInputPin::Receive(CIBuffer *pBuffer)
{
    AUTO_LOCK(mpMutex);

    if (DUnlikely(CIBuffer::SYNC_POINT & pBuffer->GetBufferFlags())) {
        updateSyncBuffer(pBuffer);
    }

    if (DUnlikely((EBufferType_VideoExtraData == pBuffer->GetBufferType()) || (EBufferType_AudioExtraData == pBuffer->GetBufferType()))) {
        updateExtraData(pBuffer);
    }

    if (mbEnable) {
        EECode err = EECode_OK;
        err = mpBufferQ->PutData(&pBuffer, sizeof(pBuffer));
        DASSERT_OK(err);
    } else {
        pBuffer->Release();
    }

    return;
}

void CQueueInputPin::Purge(TU8 disable_pin)
{
    AUTO_LOCK(mpMutex);

    if (mpBufferQ) {
        CIBuffer *pBuffer;
        while (mpBufferQ->PeekData((void *)&pBuffer, sizeof(pBuffer))) {
            pBuffer->Release();
        }
    }

    if (disable_pin) {
        mbEnable = 0;
    }
}

bool CQueueInputPin::PeekBuffer(CIBuffer *&pBuffer)
{
    AUTO_LOCK(mpMutex);
    if (!mbEnable) {
        return false;
    }

    return mpBufferQ->PeekData((void *)&pBuffer, sizeof(pBuffer));
}

CIQueue *CQueueInputPin::GetBufferQ() const
{
    return mpBufferQ;
}

//-----------------------------------------------------------------------
//
// CBypassInputPin
//
//-----------------------------------------------------------------------

CBypassInputPin *CBypassInputPin::Create(const TChar *pname, IFilter *pfilter, StreamType type)
{
    CBypassInputPin *result = new CBypassInputPin(pname, pfilter, type);
    if (result && result->Construct() != EECode_OK) {
        LOG_FATAL("CBypassInputPin::Construct() fail.\n");
        delete result;
        result = NULL;
    }
    return result;
}

CBypassInputPin::CBypassInputPin(const TChar *name, IFilter *filter, StreamType type)
    : inherited(name, filter, type)
{
}

EECode CBypassInputPin::Construct()
{
    return EECode_OK;
}

CBypassInputPin::~CBypassInputPin()
{

}

void CBypassInputPin::Delete()
{
    inherited::Delete();
}

void CBypassInputPin::Receive(CIBuffer *pBuffer)
{
    AUTO_LOCK(mpMutex);

    DASSERT(pBuffer);

    if (mbEnable) {
        if (DLikely(mpFilter)) {
            SProcessBuffer processbuffer;
            processbuffer.pin_owner = (void *)this;
            processbuffer.p_buffer = pBuffer;
            mpFilter->FilterProperty(EFilterPropertyType_process_buffer, 1, (void *)&processbuffer);
        } else {
            LOGM_FATAL("NULL filter\n");
            pBuffer->Release();
        }
    } else {
        pBuffer->Release();
    }
}

void CBypassInputPin::Purge(TU8 disable_pin)
{
    AUTO_LOCK(mpMutex);
    if (disable_pin) {
        mbEnable = 0;
    }
}

//-----------------------------------------------------------------------
//
//  CActiveFilter
//
//-----------------------------------------------------------------------
CActiveFilter::CActiveFilter(const TChar *pName, TUint index, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
    : inherited(pName, index)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpEngineMsgSink(pEngineMsgSink)
    , mpWorkQ(NULL)
    , mbRun(0)
    , mbPaused(0)
    , mbStarted(0)
    , mbSuspended(0)
    , msState(EModuleState_Invalid)
{
    if (mpPersistMediaConfig) {
        mpClockReference = mpPersistMediaConfig->p_system_clock_reference;
    } else {
        mpClockReference = NULL;
    }
}

EECode CActiveFilter::Construct()
{
    if (NULL == (mpWorkQ = CIWorkQueue::Create((IActiveObject *)this))) {
        LOGM_FATAL("CActiveFilter::Construct: CIWorkQueue::Create fail\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CActiveFilter::~CActiveFilter()
{

}

void CActiveFilter::Delete()
{
    if (mpWorkQ) {
        mpWorkQ->Delete();
        mpWorkQ = NULL;
    }

    inherited::Delete();
}

CObject *CActiveFilter::GetObject0() const
{
    return (CObject *) this;
}

CObject *CActiveFilter::GetObject() const
{
    return (CObject *) this;
}

EECode CActiveFilter::Run()
{
    return mpWorkQ->Run();
}

EECode CActiveFilter::Exit()
{
    return mpWorkQ->Exit();
}

EECode CActiveFilter::Start()
{
    return mpWorkQ->Start();
}

EECode CActiveFilter::Stop()
{
    return mpWorkQ->Stop();
}

void CActiveFilter::Pause()
{
    mpWorkQ->PostMsg(ECMDType_Pause);
}

void CActiveFilter::Resume()
{
    mpWorkQ->PostMsg(ECMDType_Resume);
}

void CActiveFilter::Flush()
{
    mpWorkQ->SendCmd(ECMDType_Flush);
}

void CActiveFilter::ResumeFlush()
{
    mpWorkQ->SendCmd(ECMDType_ResumeFlush);
}

EECode CActiveFilter::Suspend(TUint release_flag)
{
    SCMD cmd;
    cmd.code = ECMDType_Suspend;
    cmd.flag = release_flag;
    return mpWorkQ->SendCmd(cmd);
}

EECode CActiveFilter::ResumeSuspend(TComponentIndex input_index)
{
    SCMD cmd;
    cmd.code = ECMDType_ResumeSuspend;
    cmd.flag = input_index;
    return mpWorkQ->SendCmd(cmd);
}

EECode CActiveFilter::SendCmd(TUint cmd)
{
    return mpWorkQ->SendCmd(cmd);
}

void CActiveFilter::PostMsg(TUint cmd)
{
    mpWorkQ->PostMsg(cmd);
}

void CActiveFilter::GetInfo(INFO &info)
{
    info.nInput = 0;
    info.nOutput = 0;
    info.pName = mpModuleName;
}

CIQueue *CActiveFilter::MsgQ()
{
    return mpWorkQ->MsgQ();
}

EECode CActiveFilter::OnCmd(SCMD &cmd)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

const TChar *CActiveFilter::GetName() const
{
    return GetModuleName();
}

void CActiveFilter::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
    LOGM_FATAL("TO DO\n");
}

//-----------------------------------------------------------------------
//
//  CActiveEngine
//
//-----------------------------------------------------------------------
CActiveEngine::CActiveEngine(const TChar *pname, TUint index)
    : inherited(pname, index)
    , mSessionID(0)
    , mpMutex(NULL)
    , mpWorkQ(NULL)
    , mpCmdQueue(NULL)
    , mpFilterMsgQ(NULL)
    , mpAppMsgQueue(NULL)
    , mpAppMsgSink(NULL)
    , mpAppMsgProc(NULL)
    , mpAppMsgContext(NULL)
{
}

CActiveEngine::~CActiveEngine()
{
}

void CActiveEngine::Delete()
{
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    if (mpFilterMsgQ) {
        mpFilterMsgQ->Delete();
        mpFilterMsgQ = NULL;
    }

    if (mpWorkQ) {
        mpWorkQ->Delete();
        mpWorkQ = NULL;
    }

    inherited::Delete();
}

CObject *CActiveEngine::GetObject() const
{
    return (CObject *) this;
}

EECode CActiveEngine::MsgProc(SMSG &msg)
{
    EECode ret = EECode_OK;
    DASSERT(mpWorkQ);

    if (mpFilterMsgQ) {
        AUTO_LOCK(mpMutex);
        msg.sessionID = mSessionID;
        ret = mpFilterMsgQ->PutData(&msg, sizeof(msg));
        DASSERT_OK(ret);
    } else {
        LOGM_ERROR("CActiveEngine::MsgProc, NULL mpFilterMsgQ\n");
        return EECode_Error;
    }
    return ret;

}

void CActiveEngine::OnRun()
{
    LOGM_FATAL("CActiveEngine::OnRun(), shoud not comes here.\n");
}

void CActiveEngine::NewSession()
{
    AUTO_LOCK(mpMutex);
    mSessionID++;
}

bool CActiveEngine::IsSessionMsg(SMSG &msg)
{
    return msg.sessionID == mSessionID;
}

EECode CActiveEngine::Construct()
{
    if (NULL == (mpMutex = gfCreateMutex())) {
        LOGM_FATAL("CActiveEngine::Construct(), gfCreateMutex() fail\n");
        return EECode_NoMemory;
    }

    if (NULL == (mpWorkQ = CIWorkQueue::Create((IActiveObject *)this))) {
        LOGM_FATAL("CActiveEngine::Construct(), CIWorkQueue::Create fail\n");
        if (mpMutex) {
            mpMutex->Delete();
            mpMutex = NULL;
        }
        return EECode_NoMemory;
    }

    mpCmdQueue = mpWorkQ->MsgQ();
    if (NULL == (mpFilterMsgQ = CIQueue::Create(mpCmdQueue, this, sizeof(SMSG), 1))) {
        LOGM_FATAL("CActiveEngine::Construct(), CIQueue::Create fail\n");
        if (mpMutex) {
            mpMutex->Delete();
            mpMutex = NULL;
        }
        if (mpWorkQ) {
            mpWorkQ->Delete();
            mpWorkQ = NULL;
        }
        return EECode_NoMemory;
    }

    return EECode_OK;
}

EECode CActiveEngine::PostEngineMsg(SMSG &msg)
{
    DASSERT(mpFilterMsgQ);
    DASSERT(mpWorkQ);
    {
        AUTO_LOCK(mpMutex);
        msg.sessionID = mSessionID;
    }
    return mpFilterMsgQ->PutData(&msg, sizeof(SMSG));
}

EECode CActiveEngine::SetAppMsgQueue(CIQueue *p_msg_queue)
{
    mpAppMsgQueue = p_msg_queue;
    return EECode_OK;
}

EECode CActiveEngine::SetAppMsgSink(IMsgSink *pAppMsgSink)
{
    AUTO_LOCK(mpMutex);
    mpAppMsgSink = pAppMsgSink;
    return EECode_OK;
}

EECode CActiveEngine::SetAppMsgCallback(void (*MsgProc)(void *, SMSG &), void *context)
{
    AUTO_LOCK(mpMutex);
    mpAppMsgProc = MsgProc;
    mpAppMsgContext = context;
    return EECode_OK;
}

EECode CActiveEngine::PostAppMsg(SMSG &msg)
{
    //AUTO_LOCK(mpMutex);
    if (msg.sessionID == mSessionID) {
        if (mpAppMsgQueue) {
            mpAppMsgQueue->PostMsg(&msg, sizeof(SMSG));
        } else if (mpAppMsgSink) {
            mpAppMsgSink->MsgProc(msg);
        } else if (mpAppMsgProc) {
            mpAppMsgProc(mpAppMsgContext, msg);
        } else {
            LOGM_WARN("no app msg sink or proc in CActiveEngine::PostAppMsg\n");
        }
    } else {
        LOGM_WARN("should not comes here, not correct seesion id %d, engine's session id %d.\n", msg.sessionID, mSessionID);
    }
    return EECode_OK;
}

EECode CActiveEngine::Connect(IFilter *up, TUint uppin_index, IFilter *down, TUint downpin_index, TUint uppin_sub_index)
{
    EECode err;

    IOutputPin *pOutput;
    IInputPin *pInput;

    AUTO_LOCK(mpMutex);
    DASSERT(up && down);

    if (up && down) {
        pOutput = up->GetOutputPin(uppin_index, uppin_sub_index);
        if (NULL == pOutput) {
            LOGM_FATAL("No output pin: %p, index %d, sub index %d\n", up, uppin_index, uppin_sub_index);
            return EECode_InternalLogicalBug;
        }

        pInput = down->GetInputPin(downpin_index);
        if (NULL == pInput) {
            LOGM_FATAL("No input pin: %p, index %d\n", down, downpin_index);
            return EECode_InternalLogicalBug;
        }

        LOGM_INFO("{uppin_index %d, uppin_sub_index %d, downpin_index %d} connect start.\n", uppin_index, uppin_sub_index, downpin_index);
        err = pInput->Connect(pOutput, uppin_sub_index);
        if (EECode_OK == err) {
            err = pOutput->Connect(pInput, uppin_sub_index);
            if (EECode_OK == err) {
                LOGM_INFO("{uppin_index %d, uppin_sub_index %d, downpin_index %d} connect done.\n", uppin_index, uppin_sub_index, downpin_index);
                return EECode_OK;
            }

            LOGM_FATAL("pOutput->Connect() fail, return %d\n", err);

            err = pInput->Disconnect(uppin_sub_index);
            DASSERT_OK(err);

            return EECode_InternalLogicalBug;
        }

        LOGM_FATAL("pInput->Connect() fail, return %d\n", err);
        return EECode_InternalLogicalBug;
    }

    LOGM_FATAL("NULL pointer detected! up %p, down %p\n", up, down);
    return EECode_BadParam;
}

EECode CActiveEngine::OnCmd(SCMD &cmd)
{
    LOGM_FATAL("TO DO!\n");
    return EECode_NoImplement;
}

const TChar *CActiveEngine::GetName() const
{
    return GetModuleName();
}

//-----------------------------------------------------------------------
//
//  CScheduledFilter
//
//-----------------------------------------------------------------------
CScheduledFilter::CScheduledFilter(const TChar *pName, TUint index, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
    : inherited(pName, index)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpEngineMsgSink(pEngineMsgSink)
    , mpCmdQueue(NULL)
    , mbRun(0)
    , mbPaused(0)
    , mbStarted(0)
    , mbSuspended(0)
    , msState(EModuleState_Invalid)
{
}

EECode CScheduledFilter::Construct()
{
    if ((mpCmdQueue = CIQueue::Create(NULL, this, sizeof(SCMD), 16)) == NULL) {
        LOG_FATAL("No memory\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CScheduledFilter::~CScheduledFilter()
{
}

void CScheduledFilter::Delete()
{
    if (mpCmdQueue) {
        mpCmdQueue->Delete();
        mpCmdQueue = NULL;
    }

    inherited::Delete();
}

CObject *CScheduledFilter::GetObject0() const
{
    return (CObject *) this;
}

EECode CScheduledFilter::Run()
{
    DASSERT(!mbRun);
    mbRun = 1;
    return EECode_OK;
}

EECode CScheduledFilter::Exit()
{
    return EECode_OK;
}

EECode CScheduledFilter::EventHandling(EECode err)
{
    return EECode_OK;
}

void CScheduledFilter::Pause()
{
    //DASSERT(mpCmdQueue);
    //DASSERT(mbRun);
    //DASSERT(mbStarted);

    if (mpCmdQueue && mbRun && mbStarted) {
        SCMD cmd;
        cmd.code = ECMDType_Pause;
        cmd.needFreePExtra = 0;
        mpCmdQueue->PostMsg((const void *)&cmd, sizeof(cmd));
    }
}

void CScheduledFilter::Resume()
{
    //DASSERT(mpCmdQueue);
    //DASSERT(mbRun);
    //DASSERT(mbStarted);

    if (mpCmdQueue && mbRun && mbStarted) {
        SCMD cmd;
        cmd.code = ECMDType_Resume;
        cmd.needFreePExtra = 0;
        mpCmdQueue->PostMsg((const void *)&cmd, sizeof(cmd));
    }
}

void CScheduledFilter::Flush()
{
    //DASSERT(mpCmdQueue);
    //DASSERT(mbRun);
    //DASSERT(mbStarted);

    if (mpCmdQueue && mbRun && mbStarted) {
        SCMD cmd;
        cmd.code = ECMDType_Flush;
        cmd.needFreePExtra = 0;
        mpCmdQueue->SendMsg((const void *)&cmd, sizeof(cmd));
    }
}

void CScheduledFilter::ResumeFlush()
{
    //DASSERT(mpCmdQueue);
    //DASSERT(mbRun);
    //DASSERT(mbStarted);

    if (mpCmdQueue && mbRun && mbStarted) {
        SCMD cmd;
        cmd.code = ECMDType_ResumeFlush;
        cmd.needFreePExtra = 0;
        mpCmdQueue->SendMsg((const void *)&cmd, sizeof(cmd));
    }
}

EECode CScheduledFilter::Suspend(TUint release_context)
{
    //DASSERT(mpCmdQueue);
    //DASSERT(mbRun);
    //DASSERT(mbStarted);

    if (mpCmdQueue && mbRun && mbStarted) {
        SCMD cmd;
        cmd.code = ECMDType_Suspend;
        cmd.needFreePExtra = 0;
        return mpCmdQueue->SendMsg((const void *)&cmd, sizeof(cmd));
    } else {
        return EECode_BadState;
    }
}

EECode CScheduledFilter::ResumeSuspend(TComponentIndex input_index)
{
    //DASSERT(mpCmdQueue);
    //DASSERT(mbRun);
    //DASSERT(mbStarted);

    if (mpCmdQueue && mbRun && mbStarted) {
        SCMD cmd;
        cmd.code = ECMDType_ResumeSuspend;
        cmd.needFreePExtra = 0;
        return mpCmdQueue->SendMsg((const void *)&cmd, sizeof(cmd));
    } else {
        return EECode_BadState;
    }
}

EECode CScheduledFilter::SendCmd(TUint cmd_code)
{
    //DASSERT(mpCmdQueue);
    //DASSERT(mbRun);
    //DASSERT(mbStarted);

    if (DLikely(mpCmdQueue && mbRun && mbStarted)) {
        SCMD cmd;
        cmd.code = cmd_code;
        cmd.needFreePExtra = 0;
        return mpCmdQueue->SendMsg((const void *)&cmd, sizeof(cmd));
    } else {
        LOGM_FATAL("why comes here\n");
    }
    return EECode_BadState;
}

void CScheduledFilter::PostMsg(TUint cmd_code)
{
    //DASSERT(mpCmdQueue);
    //DASSERT(mbRun);
    //DASSERT(mbStarted);

    if (DLikely(mpCmdQueue && mbRun && mbStarted)) {
        SCMD cmd;
        cmd.code = cmd_code;
        cmd.needFreePExtra = 0;
        mpCmdQueue->PostMsg((const void *)&cmd, sizeof(cmd));
    } else {
        LOGM_FATAL("why comes here\n");
    }
}

CIQueue *CScheduledFilter::MsgQ()
{
    return mpCmdQueue;
}

void CScheduledFilter::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
    LOGM_FATAL("TO DO\n");
}

void gfClearBufferMemory(IBufferPool *p_buffer_pool)
{
    if (p_buffer_pool) {
        CIBuffer *p_buffer = NULL;
        while (p_buffer_pool->GetFreeBufferCnt()) {
            p_buffer_pool->AllocBuffer(p_buffer, sizeof(CIBuffer));
            if (p_buffer->mbAllocated) {
                p_buffer->ReleaseAllocatedMemory();
            }
        }
    }
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

