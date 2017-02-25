/*******************************************************************************
 * x265_video_encoder.cpp
 *
 * History:
 *    2014/12/12 - [Zhi He] create file
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

#ifdef BUILD_MODULE_X265

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "media_mw_if.h"
#include "media_mw_utils.h"

#include "framework_interface.h"
#include "mw_internal.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

extern "C" {
#include "x265_config.h"
#include "x265.h"
}

#include "x265_video_encoder.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

#define DX265_MAX_FRAME_SIZE (1024 * 1024)
#define DX265_MAX_EXTRADATA_SIZE (1024)
#define DX265_TIME_CHECK_DURATION 7

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

IVideoEncoder *gfCreateX265VideoEncoder(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CX265VideoEncoder::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

CX265VideoEncoder *CX265VideoEncoder::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    CX265VideoEncoder *result = new CX265VideoEncoder(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

void CX265VideoEncoder::Destroy()
{
    Delete();
}

CX265VideoEncoder::CX265VideoEncoder(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
    : inherited(pName, index)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpEngineMsgSink(pEngineMsgSink)
    , mpBufferPool(NULL)
    , mpMemPool(NULL)
    , mbEncoderSetup(0)
    , mVideoWidth(1920)
    , mVideoHeight(1080)
    , mFramerateNum(DDefaultVideoFramerateNum)
    , mFramerateDen(DDefaultVideoFramerateDen)
    , mBitrate(2 * 1024 * 1024)
    , mParamsMaxIDRInterval(256)
    , mParamsMinIInterval(32)
    , mParamsBframes(0)
    , mFramerate((float)DDefaultVideoFramerateNum / (float)DDefaultVideoFramerateDen)
    , mStartTime(0)
    , mTimeBasedDTS(0)
    , mFrameCountBasedDTS(0)
    , mStartDTS(0)
    , mOriDTSGap(0)
    , mCurDTSGap(0)
    , mFrameCount(0)
    , mpX265(NULL)
    , mpNal(NULL)
    , mNalIndex(0)
    , mNalCount(0)
    , mFrameTick(0)
    , mTotalInputFrameCount(0)
    , mTotalOutputFrameCount(0)
{
}

CX265VideoEncoder::~CX265VideoEncoder()
{
}

EECode CX265VideoEncoder::Construct()
{
    mpMemPool = CRingMemPool::Create(8 * 1024 * 1024);
    if (DUnlikely(!mpMemPool)) {
        LOGM_FATAL("No Memory\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

void CX265VideoEncoder::Delete()
{
    if (mpMemPool) {
        mpMemPool->GetObject0()->Delete();
        mpMemPool = NULL;
    }

    inherited::Delete();
}

EECode CX265VideoEncoder::SetupContext(SVideoEncoderInputParam *param)
{
    if (DLikely(param)) {
        if (param->cap_max_width && param->cap_max_height) {
            mVideoWidth = param->cap_max_width;
            mVideoHeight = param->cap_max_height;
        }

        if (param->framerate_num && param->framerate_den) {
            mFramerateNum = param->framerate_num;
            mFramerateDen = param->framerate_den;
            mFramerate = (float)param->framerate_num / (float)param->framerate_den;
        } else {
            if (DLikely(param->framerate > 0.1)) {
                mFramerateNum = DDefaultVideoFramerateNum;
                mFramerateDen = (TU32)((float)DDefaultVideoFramerateNum / param->framerate);
                mFramerate = param->framerate;
            } else {
                mFramerateNum = DDefaultVideoFramerateNum;
                mFramerateDen = DDefaultVideoFramerateDen;
                mFramerate = (float)mFramerateNum / (float)mFramerateDen;
                LOGM_WARN("not specify framerate, use %f as default\n", mFramerate);
            }
        }

        if (param->bitrate) {
            mBitrate = param->bitrate;
        }

        mParamsMaxIDRInterval = 32;
        mParamsMinIInterval = 4;

        mParamsBframes = 0;
    } else {
        LOGM_FATAL("NULL param in CX265VideoEncoder::SetupContext()\n");
        return EECode_InternalLogicalBug;
    }

    EECode err = setupX265Encoder();
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("setupX264Encoder() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }


    return EECode_OK;
}

void CX265VideoEncoder::DestroyContext()
{
    closeX265Encoder();
}

EECode CX265VideoEncoder::SetBufferPool(IBufferPool *p_buffer_pool)
{
    mpBufferPool = p_buffer_pool;
    if (DUnlikely(!mpBufferPool)) {
        LOGM_ERROR("NULL p_buffer_pool in CX265VideoEncoder::SetBufferPool()\n");
        return EECode_BadParam;
    }

    mpBufferPool->SetReleaseBufferCallBack(__release_ring_buffer);

    return EECode_OK;
}

EECode CX265VideoEncoder::Start()
{
    if (DUnlikely(!mpBufferPool || !mpMemPool)) {
        LOGM_ERROR("NULL p_buffer_pool or mpMemPool in CX265VideoEncoder::Start()\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CX265VideoEncoder::Stop()
{
    return EECode_OK;
}

EECode CX265VideoEncoder::Encode(CIBuffer *in_buffer, CIBuffer *out_buffer, TU32 &current_remaining, TU32 &all_cached_frames)
{
    DASSERT(mpMemPool);
    current_remaining = 0;
    all_cached_frames = 0;
    if (DUnlikely(!out_buffer)) {
        LOGM_ERROR("CX265VideoEncoder::Encode() NULL out_buffer\n");
        return EECode_InternalLogicalBug;
    }

    if (mNalCount && ((mNalIndex + 1) < mNalCount)) {
        if (DUnlikely(in_buffer)) {
            LOGM_ERROR("CX265VideoEncoder::Encode(): still have nal remaining, must read them out first!\n");
            return EECode_InternalLogicalBug;
        }

        return encodeReturnRemaining(out_buffer, current_remaining);
    }

    TInt ret = 0;

    if (DLikely(in_buffer)) {
        mInputPicture.colorSpace = X265_CSP_I420;
        mInputPicture.planes[0] = in_buffer->GetDataPtr(0);
        mInputPicture.planes[1] = in_buffer->GetDataPtr(1);
        mInputPicture.planes[2] = in_buffer->GetDataPtr(2);
        mInputPicture.stride[0] = in_buffer->GetDataLineSize(0);
        mInputPicture.stride[1] = in_buffer->GetDataLineSize(1);
        mInputPicture.stride[2] = in_buffer->GetDataLineSize(2);
        mInputPicture.pts = in_buffer->GetBufferNativePTS();
        mInputPicture.dts = in_buffer->GetBufferNativeDTS();
        //LOGM_PRINTF("encoder input pts %lld, dts %lld\n", mInputPicture.i_pts, mInputPicture.i_dts);
        //mFrameTick += mFramerateDen;
        ret = x265_encoder_encode(mpX265, &mpNal, &mNalCount, &mInputPicture, &mOutputPicture);
        mTotalInputFrameCount ++;
    } else {
        if (mTotalInputFrameCount > mTotalOutputFrameCount) {
            ret = x265_encoder_encode(mpX265, &mpNal, &mNalCount, NULL, &mOutputPicture);
        } else {
            all_cached_frames = 0;
            return EECode_NotExist;
        }
    }

    if (DLikely(0 <= ret)) {

        if (DUnlikely(!mNalCount)) {
            LOGM_ERROR("no nal return, skip\n");
            return EECode_OK_NoOutputYet;
        }

        mCurrentFramePTS = mOutputPicture.pts;
        mCurrentFrameDTS = mOutputPicture.dts;

        //LOGM_PRINTF("encoder output pts %lld, dts %lld\n", mOutputPicture.i_pts, mOutputPicture.i_dts);

        mTotalOutputFrameCount ++;
        all_cached_frames = (TU32)(mTotalInputFrameCount - mTotalOutputFrameCount);

        TU8 *pout = NULL;
        TU8 *p = NULL;
        TU32 copy_size = 0;

        if (DUnlikely(NAL_UNIT_SPS == mpNal[0].type)) {
            pout = mpMemPool->RequestMemBlock(DX265_MAX_EXTRADATA_SIZE);
            p = pout;

            if (DLikely(2 <= mNalCount)) {
                out_buffer->mBufferType = EBufferType_VideoExtraData;
                out_buffer->SetBufferFlags(CIBuffer::SYNC_POINT | CIBuffer::WITH_EXTRA_DATA);
                DASSERT(NAL_UNIT_PPS == mpNal[1].type);
                memcpy(p, mpNal[0].payload, mpNal[0].sizeBytes);
                p += mpNal[0].sizeBytes;
                memcpy(p, mpNal[1].payload, mpNal[1].sizeBytes);
                copy_size = mpNal[0].sizeBytes + mpNal[1].sizeBytes;
                LOGM_PRINTF("build sps + pps\n");

                if (DLikely(copy_size < DX265_MAX_EXTRADATA_SIZE)) {
                    mpMemPool->ReturnBackMemBlock((TULong)(DX265_MAX_EXTRADATA_SIZE - copy_size), pout + copy_size);
                } else {
                    LOGM_FATAL("DX265_MAX_EXTRADATA_SIZE(%d) is not enough, copy_size %d\n", DX265_MAX_EXTRADATA_SIZE, copy_size);
                    return EECode_OutOfCapability;
                }

                out_buffer->mVideoWidth = mVideoWidth;
                out_buffer->mVideoHeight = mVideoHeight;
                out_buffer->mVideoFrameRateNum = mFramerateNum;
                out_buffer->mVideoFrameRateDen = mFramerateDen;
                if (mFramerateDen && mFramerateNum) {
                    out_buffer->mVideoRoughFrameRate = (float) mFramerateNum / (float) mFramerateDen;
                } else {
                    LOGM_WARN("not set frame rate?\n");
                    out_buffer->mVideoRoughFrameRate = 29.97;
                }
                out_buffer->SetDataPtrBase(pout);
                out_buffer->SetDataMemorySize((TMemSize)copy_size);
                out_buffer->SetDataPtr(pout);
                out_buffer->SetDataSize((TMemSize)copy_size);
                out_buffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
                out_buffer->mpCustomizedContent = (void *) mpMemPool;
                updateTimestamp(out_buffer);

                mNalIndex = 0;
                current_remaining = 1;

                return EECode_OK;
            } else {
                LOGM_ERROR("data corruption?, after sps nal, mNalCount %d\n", mNalCount);
                return EECode_OK_NoOutputYet;
            }
        } else {
            pout = mpMemPool->RequestMemBlock(DX265_MAX_FRAME_SIZE);
            p = pout;
        }

        out_buffer->SetBufferFlags(0);

        if (DUnlikely(NAL_UNIT_CODED_SLICE_IDR_N_LP <= mpNal[0].type)) {
            out_buffer->SetBufferFlagBits(CIBuffer::KEY_FRAME);
            out_buffer->mVideoFrameType = EPredefinedPictureType_IDR;
        } else if (NAL_UNIT_CODED_SLICE_IDR_N_LP > mpNal[0].type) {
            out_buffer->mVideoFrameType = EPredefinedPictureType_P;
        }
        out_buffer->mBufferType = EBufferType_VideoES;

        for (TS32 i = 0; i < mNalCount; i ++) {
            memcpy(p, mpNal[i].payload, mpNal[i].sizeBytes);
            p += mpNal[i].sizeBytes;
            copy_size += mpNal[i].sizeBytes;
        }

        mNalCount = 0;
        mNalIndex = 0;

        if (DLikely(copy_size < DX265_MAX_FRAME_SIZE)) {
            mpMemPool->ReturnBackMemBlock((TULong)(DX265_MAX_FRAME_SIZE - copy_size), pout + copy_size);
        } else {
            LOGM_FATAL("DX265_MAX_FRAME_SIZE(%d) is not enough, copy_size %d\n", DX265_MAX_FRAME_SIZE, copy_size);
            return EECode_OutOfCapability;
        }

        out_buffer->SetDataPtrBase(pout);
        out_buffer->SetDataMemorySize((TMemSize)copy_size);
        out_buffer->SetDataPtr(pout);
        out_buffer->SetDataSize((TMemSize)copy_size);
        out_buffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
        out_buffer->mpCustomizedContent = (void *) mpMemPool;
        updateTimestamp(out_buffer);

    } else {
        LOGM_ERROR("x265_encoder_encode() fail, ret %d\n", ret);
        mNalCount = 0;
        mNalIndex = 0;
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CX265VideoEncoder::VideoEncoderProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
    LOGM_ERROR("TODO : {CX265VideoEncoder::VideoEncoderProperty}\n");
    return EECode_NotSupported;
}

EECode CX265VideoEncoder::setupX265Encoder()
{
    if (DUnlikely(mbEncoderSetup)) {
        LOGM_ERROR("x264 encoder already setup\n");
        return EECode_BadState;
    }

    x265_param_default(&mX265Params);
    x265_param_default_preset(&mX265Params, "faster", "stillimage");

    mX265Params.sourceWidth = mVideoWidth;
    mX265Params.sourceHeight = mVideoHeight;
    mX265Params.fpsNum = mFramerateNum;
    mX265Params.fpsDenom = mFramerateDen;

    mX265Params.maxNumReferences = mParamsBframes + 1;
    mX265Params.keyframeMax = mParamsMaxIDRInterval;
    mX265Params.keyframeMin = mParamsMinIInterval;

    mX265Params.rc.bitrate = mBitrate >> 10;

    mX265Params.bframes = mParamsBframes;
    mX265Params.bRepeatHeaders = 1;

    mX265Params.bFrameAdaptive = 0;
    mX265Params.bframes = 0;
    mX265Params.lookaheadDepth = 0;
    mX265Params.scenecutThreshold = 0;
    mX265Params.rc.cuTree = 0;

    mpX265 = x265_encoder_open(&mX265Params);
    if (DUnlikely(NULL == mpX265)) {
        LOGM_ERROR("x265_encoder_open fail\n");
        return EECode_Error;
    }

    x265_picture_init(&mX265Params, &mInputPicture);

    mbEncoderSetup = 1;
    return EECode_OK;
}

void CX265VideoEncoder::closeX265Encoder()
{
    if (DUnlikely(!mbEncoderSetup)) {
        LOGM_ERROR("x264 encoder not setup\n");
        return;
    }

    mbEncoderSetup = 0;

    x265_encoder_close(mpX265);
    mpX265 = NULL;
}

EECode CX265VideoEncoder::encodeReturnRemaining(CIBuffer *out_buffer, TU32 &current_remaining)
{
    TU8 *pout = mpMemPool->RequestMemBlock(DX265_MAX_FRAME_SIZE);
    TU8 *p = pout;
    TU32 copy_size = 0;

    out_buffer->SetBufferFlags(0);
    out_buffer->mBufferType = EBufferType_VideoES;

    if (DUnlikely(NAL_UNIT_CODED_SLICE_IDR_N_LP <= mpNal[0].type)) {
        out_buffer->SetBufferFlags(CIBuffer::KEY_FRAME);
        out_buffer->mVideoFrameType = EPredefinedPictureType_IDR;
    } else if (NAL_UNIT_CODED_SLICE_IDR_N_LP > mpNal[0].type) {
        out_buffer->mVideoFrameType = EPredefinedPictureType_P;
    }

    for (TS32 i = mNalIndex; i < mNalCount; i ++) {
        memcpy(p, mpNal[i].payload, mpNal[i].sizeBytes);
        p += mpNal[i].sizeBytes;
        copy_size += mpNal[i].sizeBytes;
    }

    mNalIndex = 0;
    mNalCount = 0;

    if (DLikely(copy_size < DX265_MAX_FRAME_SIZE)) {
        mpMemPool->ReturnBackMemBlock((TULong)(DX265_MAX_FRAME_SIZE - copy_size), pout + copy_size);
    } else {
        LOGM_FATAL("DX265_MAX_FRAME_SIZE(%d) is not enough, copy_size %d\n", DX265_MAX_FRAME_SIZE, copy_size);
        return EECode_OutOfCapability;
    }

    out_buffer->SetDataPtrBase(pout);
    out_buffer->SetDataMemorySize((TMemSize)copy_size);
    out_buffer->SetDataPtr(pout);
    out_buffer->SetDataSize((TMemSize)copy_size);
    out_buffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
    out_buffer->mpCustomizedContent = (void *) mpMemPool;

    current_remaining = 0;
    updateTimestamp(out_buffer);

    return EECode_OK;
}

void CX265VideoEncoder::updateTimestamp(CIBuffer *out_buffer)
{
#if 0
    if (DLikely(mFrameCount)) {
        mFrameCountBasedDTS += mOriDTSGap;
    } else {
        mOriDTSGap = mFramerateDen;
        mFrameCountBasedDTS = 0;
        LOGM_NOTICE("CX265VideoEncoder DTS gap %lld\n", mOriDTSGap);
    }

    out_buffer->SetBufferNativeDTS(mFrameCountBasedDTS);
    out_buffer->SetBufferNativePTS(mFrameCountBasedDTS);

    out_buffer->SetBufferLinearDTS(mFrameCountBasedDTS);
    out_buffer->SetBufferLinearPTS(mFrameCountBasedDTS);


    LOGM_NOTICE("CX265VideoEncoder frame count %lld, PTS %lld\n", mFrameCount, mFrameCountBasedDTS);
#endif

    out_buffer->SetBufferNativeDTS(mCurrentFrameDTS);
    out_buffer->SetBufferNativePTS(mCurrentFramePTS);

    out_buffer->SetBufferLinearDTS(mCurrentFrameDTS);
    out_buffer->SetBufferLinearPTS(mCurrentFramePTS);
    mFrameCount ++;
    //LOGM_INFO("CX265VideoEncoder frame count %lld, PTS %lld, DTS %lld\n", mFrameCount, mCurrentFramePTS, mCurrentFrameDTS);
}


DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif

