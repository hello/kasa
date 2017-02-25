/*******************************************************************************
 * x264_video_encoder.cpp
 *
 * History:
 *    2014/10/15 - [Zhi He] create file
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

#ifdef BUILD_MODULE_X264

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
#include "x264_config.h"
#include "x264.h"
}

#include "x264_video_encoder.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

#define DX264_MAX_FRAME_SIZE (1024 * 1024)
#define DX264_MAX_EXTRADATA_SIZE (1024)
#define DX264_TIME_CHECK_DURATION 7

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

IVideoEncoder *gfCreateX264VideoEncoder(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CX264VideoEncoder::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

CX264VideoEncoder *CX264VideoEncoder::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    CX264VideoEncoder *result = new CX264VideoEncoder(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

void CX264VideoEncoder::Destroy()
{
    Delete();
}

CX264VideoEncoder::CX264VideoEncoder(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
    : inherited(pName, index)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpEngineMsgSink(pEngineMsgSink)
    , mpBufferPool(NULL)
    , mpMemPool(NULL)
    , mbX264EncoderSetup(0)
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
    , mpX264(NULL)
    , mpNal(NULL)
    , mNalIndex(0)
    , mNalCount(0)
    , mX264FrameTick(0)
    , mTotalInputFrameCount(0)
    , mTotalOutputFrameCount(0)
{
}

CX264VideoEncoder::~CX264VideoEncoder()
{
}

EECode CX264VideoEncoder::Construct()
{
    mpMemPool = CRingMemPool::Create(8 * 1024 * 1024);
    if (DUnlikely(!mpMemPool)) {
        LOGM_FATAL("No Memory\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

void CX264VideoEncoder::Delete()
{
    if (mpMemPool) {
        mpMemPool->GetObject0()->Delete();
        mpMemPool = NULL;
    }

    inherited::Delete();
}

EECode CX264VideoEncoder::SetupContext(SVideoEncoderInputParam *param)
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
        LOGM_FATAL("NULL param in CX264VideoEncoder::SetupContext()\n");
        return EECode_InternalLogicalBug;
    }

    EECode err = setupX264Encoder();
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("setupX264Encoder() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }


    return EECode_OK;
}

void CX264VideoEncoder::DestroyContext()
{
    closeX264Encoder();
}

EECode CX264VideoEncoder::SetBufferPool(IBufferPool *p_buffer_pool)
{
    mpBufferPool = p_buffer_pool;
    if (DUnlikely(!mpBufferPool)) {
        LOGM_ERROR("NULL p_buffer_pool in CX264VideoEncoder::SetBufferPool()\n");
        return EECode_BadParam;
    }

    mpBufferPool->SetReleaseBufferCallBack(__release_ring_buffer);

    return EECode_OK;
}

EECode CX264VideoEncoder::Start()
{
    if (DUnlikely(!mpBufferPool || !mpMemPool)) {
        LOGM_ERROR("NULL p_buffer_pool or mpMemPool in CX264VideoEncoder::Start()\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CX264VideoEncoder::Stop()
{
    return EECode_OK;
}

EECode CX264VideoEncoder::Encode(CIBuffer *in_buffer, CIBuffer *out_buffer, TU32 &current_remaining, TU32 &all_cached_frames)
{
    DASSERT(mpMemPool);
    current_remaining = 0;
    all_cached_frames = 0;
    if (DUnlikely(!out_buffer)) {
        LOGM_ERROR("CX264VideoEncoder::Encode() NULL out_buffer\n");
        return EECode_InternalLogicalBug;
    }

    if (mNalCount && ((mNalIndex + 1) < mNalCount)) {
        if (DUnlikely(in_buffer)) {
            LOGM_ERROR("CX264VideoEncoder::Encode(): still have nal remaining, must read them out first!\n");
            return EECode_InternalLogicalBug;
        }

        return encodeReturnRemaining(out_buffer, current_remaining);
    }

    TInt ret = 0;

    if (DLikely(in_buffer)) {
        mInputPicture.img.i_csp = X264_CSP_I420;
        mInputPicture.img.i_plane = 3;
        mInputPicture.img.plane[0] = in_buffer->GetDataPtr(0);
        mInputPicture.img.plane[1] = in_buffer->GetDataPtr(1);
        mInputPicture.img.plane[2] = in_buffer->GetDataPtr(2);
        mInputPicture.img.i_stride[0] = in_buffer->GetDataLineSize(0);
        mInputPicture.img.i_stride[1] = in_buffer->GetDataLineSize(1);
        mInputPicture.img.i_stride[2] = in_buffer->GetDataLineSize(2);
        mInputPicture.i_pts = in_buffer->GetBufferNativePTS();
        mInputPicture.i_dts = in_buffer->GetBufferNativeDTS();
        //LOGM_PRINTF("encoder input pts %lld, dts %lld\n", mInputPicture.i_pts, mInputPicture.i_dts);
        //mX264FrameTick += mFramerateDen;
        ret = x264_encoder_encode(mpX264, &mpNal, &mNalCount, &mInputPicture, &mOutputPicture);
        mTotalInputFrameCount ++;
    } else {
        if (mTotalInputFrameCount > mTotalOutputFrameCount) {
            ret = x264_encoder_encode(mpX264, &mpNal, &mNalCount, NULL, &mOutputPicture);
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

        mCurrentFramePTS = mOutputPicture.i_pts;
        mCurrentFrameDTS = mOutputPicture.i_dts;

        //LOGM_PRINTF("encoder output pts %lld, dts %lld\n", mOutputPicture.i_pts, mOutputPicture.i_dts);

        mTotalOutputFrameCount ++;
        all_cached_frames = (TU32)(mTotalInputFrameCount - mTotalOutputFrameCount);

        TU8 *pout = NULL;
        TU8 *p = NULL;
        TU32 copy_size = 0;

        if (DUnlikely(NAL_SPS == mpNal[0].i_type)) {
            pout = mpMemPool->RequestMemBlock(DX264_MAX_EXTRADATA_SIZE);
            p = pout;

            if (DLikely(2 <= mNalCount)) {
                out_buffer->mBufferType = EBufferType_VideoExtraData;
                out_buffer->SetBufferFlags(CIBuffer::SYNC_POINT | CIBuffer::WITH_EXTRA_DATA);
                DASSERT(NAL_PPS == mpNal[1].i_type);
                memcpy(p, mpNal[0].p_payload, mpNal[0].i_payload);
                p += mpNal[0].i_payload;
                memcpy(p, mpNal[1].p_payload, mpNal[1].i_payload);
                copy_size = mpNal[0].i_payload + mpNal[1].i_payload;
                //LOGM_PRINTF("build sps + pps\n");

                if (DLikely(copy_size < DX264_MAX_EXTRADATA_SIZE)) {
                    mpMemPool->ReturnBackMemBlock((TULong)(DX264_MAX_EXTRADATA_SIZE - copy_size), pout + copy_size);
                } else {
                    LOGM_FATAL("DX264_MAX_EXTRADATA_SIZE(%d) is not enough, copy_size %d\n", DX264_MAX_EXTRADATA_SIZE, copy_size);
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
            pout = mpMemPool->RequestMemBlock(DX264_MAX_FRAME_SIZE);
            p = pout;
        }

        out_buffer->SetBufferFlags(0);

        if (DUnlikely(NAL_SLICE_IDR <= mpNal[0].i_type)) {
            out_buffer->SetBufferFlagBits(CIBuffer::KEY_FRAME);
            //LOGM_PRINTF("22, set key frame flag\n");
            out_buffer->mVideoFrameType = EPredefinedPictureType_IDR;
        } else if (NAL_SLICE_IDR > mpNal[0].i_type) {
            out_buffer->mVideoFrameType = EPredefinedPictureType_P;
        }
        out_buffer->mBufferType = EBufferType_VideoES;

        //LOGM_PRINTF("22, Dump 264 nal, count %d\n", mNalCount);
        for (TS32 i = 0; i < mNalCount; i ++) {
            memcpy(p, mpNal[i].p_payload, mpNal[i].i_payload);
            p += mpNal[i].i_payload;
            copy_size += mpNal[i].i_payload;
            //LOGM_PRINTF("22, [%d] nal type %d, nal size %d\n", i, mpNal[i].i_type, mpNal[i].i_payload);
            //if (DLikely(p_nal[i].i_payload > 16)) {
            //    gfPrintMemory(p_nal[i].p_payload, 16);
            //} else {
            //    gfPrintMemory(p_nal[i].p_payload, p_nal[i].i_payload);
            //}
        }

        mNalCount = 0;
        mNalIndex = 0;

        if (DLikely(copy_size < DX264_MAX_FRAME_SIZE)) {
            mpMemPool->ReturnBackMemBlock((TULong)(DX264_MAX_FRAME_SIZE - copy_size), pout + copy_size);
        } else {
            LOGM_FATAL("DX264_MAX_FRAME_SIZE(%d) is not enough, copy_size %d\n", DX264_MAX_FRAME_SIZE, copy_size);
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
        LOGM_ERROR("x264_encoder_encode() fail, ret %d\n", ret);
        mNalCount = 0;
        mNalIndex = 0;
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CX264VideoEncoder::VideoEncoderProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
    LOGM_ERROR("TODO : {CX264VideoEncoder::VideoEncoderProperty}\n");
    return EECode_NotSupported;
}

EECode CX264VideoEncoder::setupX264Encoder()
{
    if (DUnlikely(mbX264EncoderSetup)) {
        LOGM_ERROR("x264 encoder already setup\n");
        return EECode_BadState;
    }

    x264_param_default(&mX264Params);
    x264_param_default_preset(&mX264Params, "faster", "stillimage");

    mX264Params.i_width = mVideoWidth;
    mX264Params.i_height = mVideoHeight;
    mX264Params.i_fps_num = mFramerateNum;
    mX264Params.i_fps_den = mFramerateDen;

    mX264Params.i_frame_reference = mParamsBframes + 1;
    mX264Params.i_keyint_max = mParamsMaxIDRInterval;
    mX264Params.i_keyint_min = mParamsMinIInterval;

    mX264Params.rc.i_bitrate = mBitrate >> 10;

    mX264Params.i_bframe = mParamsBframes;
    mX264Params.b_repeat_headers = 1;

    //mX264Params.pf_log = gfExternSubstitutionLogPrintf;

    mX264Params.rc.i_lookahead = 0;
    mX264Params.i_sync_lookahead = 0;
    mX264Params.i_bframe = 0;
    mX264Params.b_sliced_threads = 1;
    mX264Params.b_vfr_input = 0;
    mX264Params.rc.b_mb_tree = 0;

    mpX264 = x264_encoder_open(&mX264Params);
    if (DUnlikely(NULL == mpX264)) {
        LOGM_ERROR("x264_encoder_open fail\n");
        return EECode_Error;
    }

    x264_picture_init(&mInputPicture);

    mbX264EncoderSetup = 1;
    return EECode_OK;
}

void CX264VideoEncoder::closeX264Encoder()
{
    if (DUnlikely(!mbX264EncoderSetup)) {
        LOGM_ERROR("x264 encoder not setup\n");
        return;
    }

    mbX264EncoderSetup = 0;

    x264_encoder_close(mpX264);
    mpX264 = NULL;
}

EECode CX264VideoEncoder::encodeReturnRemaining(CIBuffer *out_buffer, TU32 &current_remaining)
{
    TU8 *pout = mpMemPool->RequestMemBlock(DX264_MAX_FRAME_SIZE);
    TU8 *p = pout;
    TU32 copy_size = 0;

    out_buffer->SetBufferFlags(0);
    out_buffer->mBufferType = EBufferType_VideoES;

    if (DUnlikely(NAL_SLICE_IDR <= mpNal[0].i_type)) {
        out_buffer->SetBufferFlags(CIBuffer::KEY_FRAME);
        //LOGM_PRINTF("11, set key frame flag\n");
        out_buffer->mVideoFrameType = EPredefinedPictureType_IDR;
    } else if (NAL_SLICE_IDR > mpNal[0].i_type) {
        out_buffer->mVideoFrameType = EPredefinedPictureType_P;
    }

    //LOGM_PRINTF("11 Dump 264 nal, count %d\n", mNalCount);
    for (TS32 i = mNalIndex; i < mNalCount; i ++) {
        memcpy(p, mpNal[i].p_payload, mpNal[i].i_payload);
        p += mpNal[i].i_payload;
        copy_size += mpNal[i].i_payload;
        //LOGM_PRINTF("11 [%d] nal type %d, nal size %d\n", i, mpNal[i].i_type, mpNal[i].i_payload);
        //if (DLikely(p_nal[i].i_payload > 16)) {
        //    gfPrintMemory(p_nal[i].p_payload, 16);
        //} else {
        //    gfPrintMemory(p_nal[i].p_payload, p_nal[i].i_payload);
        //}
    }

    mNalIndex = 0;
    mNalCount = 0;

    if (DLikely(copy_size < DX264_MAX_FRAME_SIZE)) {
        mpMemPool->ReturnBackMemBlock((TULong)(DX264_MAX_FRAME_SIZE - copy_size), pout + copy_size);
    } else {
        LOGM_FATAL("DX264_MAX_FRAME_SIZE(%d) is not enough, copy_size %d\n", DX264_MAX_FRAME_SIZE, copy_size);
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

void CX264VideoEncoder::updateTimestamp(CIBuffer *out_buffer)
{
#if 0
    if (DLikely(mFrameCount)) {
        mFrameCountBasedDTS += mOriDTSGap;
    } else {
        mOriDTSGap = mFramerateDen;
        mFrameCountBasedDTS = 0;
        LOGM_NOTICE("CX264VideoEncoder DTS gap %lld\n", mOriDTSGap);
    }

    out_buffer->SetBufferNativeDTS(mFrameCountBasedDTS);
    out_buffer->SetBufferNativePTS(mFrameCountBasedDTS);

    out_buffer->SetBufferLinearDTS(mFrameCountBasedDTS);
    out_buffer->SetBufferLinearPTS(mFrameCountBasedDTS);


    LOGM_NOTICE("CX264VideoEncoder frame count %lld, PTS %lld\n", mFrameCount, mFrameCountBasedDTS);
#endif

    out_buffer->SetBufferNativeDTS(mCurrentFrameDTS);
    out_buffer->SetBufferNativePTS(mCurrentFramePTS);

    out_buffer->SetBufferLinearDTS(mCurrentFrameDTS);
    out_buffer->SetBufferLinearPTS(mCurrentFramePTS);
    mFrameCount ++;
    //LOGM_INFO("CX264VideoEncoder frame count %lld, PTS %lld, DTS %lld\n", mFrameCount, mCurrentFramePTS, mCurrentFrameDTS);
}


DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif

