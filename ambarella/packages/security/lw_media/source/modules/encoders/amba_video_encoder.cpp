/*
 * amba_video_decoder.cpp
 *
 * History:
 *    2012/08/03 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "lwmd_if.h"
#include "lwmd_log.h"

#include "internal.h"

#include "osal.h"
#include "framework.h"
#include "modules_interface.h"

#include "codec_interface.h"

#include "amba_dsp.h"

#ifdef BUILD_MODULE_AMBA_DSP

#include "amba_video_encoder.h"

IVideoEncoder *gfCreateAmbaVideoEncoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
    return CAmbaVideoEncoder::Create(pName, pPersistMediaConfig, pEngineMsgSink);
}

//-----------------------------------------------------------------------
//
// CAmbaVideoEncoder
//
//-----------------------------------------------------------------------
CAmbaVideoEncoder::CAmbaVideoEncoder(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited(pname)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpMsgSink(pMsgSink)
    , mIavFd(-1)
    , mCodecFormat(StreamFormat_Invalid)
    , mFrameRate(0)
    , mBitrate(1024 * 1024)
    , mFrameRateReduceFactor(1)
    , mpBitstreamBufferStart(NULL)
    , mBitstreamBufferSize(0)
    , mCapMaxCodedWidth(0)
    , mCapMaxCodedHeight(0)
    , mCurrentFramerateNum(0)
    , mCurrentFramerateDen(0)
    , mStreamID(0)
    , mRemainingDescNumber(0)
    , mCurrentDescIndex(0)
    , mbCopyOut(0)
    , mbSendSyncBuffer(0)
    , mbSendExtrabuffer(0)
    , mbWaitFirstKeyframe(1)
    , mbRemaingData(0)
    , mpVideoExtraData(NULL)
    , mVideoExtraDataSize(0)
    , mpRemainingData(NULL)
    , mnRemainingDataSize(0)
{
    mRemainBufferTimestamp = 0;
    mPreviousTimestamp = 0;
    memset(&mMapBSB, 0x0, sizeof(mMapBSB));
}

IVideoEncoder *CAmbaVideoEncoder::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    CAmbaVideoEncoder *result = new CAmbaVideoEncoder(pname, pPersistMediaConfig, pMsgSink);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

void CAmbaVideoEncoder::Destroy()
{
    Delete();
}

EECode CAmbaVideoEncoder::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleAmbaEncoder);

    gfSetupDSPAlContext(&mfDSPAL);

    return EECode_OK;
}

CAmbaVideoEncoder::~CAmbaVideoEncoder()
{
    if (mpVideoExtraData) {
        DDBG_FREE(mpVideoExtraData, "VEVE");
        mpVideoExtraData = NULL;
    }
}

EECode CAmbaVideoEncoder::SetupContext(SVideoEncoderInputParam *param)
{
    TInt ret = 0;
    DASSERT(mpPersistMediaConfig);
    DASSERT(mfDSPAL.f_map_bsb);
    mStreamID = mpPersistMediaConfig->rtsp_server_config.video_stream_id;

    mIavFd = mpPersistMediaConfig->dsp_config.device_fd;
    DASSERT(0 < mIavFd);

    mMapBSB.b_two_times = 1;
    mMapBSB.b_enable_write = 0;
    mMapBSB.b_enable_read = 1;
    ret = mfDSPAL.f_map_bsb(mIavFd, &mMapBSB);
    if (ret) {
        LOGM_ERROR("map fail\n");
        return EECode_OSError;
    }

    return EECode_OK;
}

void CAmbaVideoEncoder::DestroyContext()
{
}

EECode CAmbaVideoEncoder::SetBufferPool(IBufferPool *p_buffer_pool)
{
    return EECode_OK;
}

EECode CAmbaVideoEncoder::Start()
{
    return EECode_OK;
}

EECode CAmbaVideoEncoder::Stop()
{
    return EECode_OK;
}

EECode CAmbaVideoEncoder::Encode(CIBuffer *in_buffer, CIBuffer *out_buffer, TU32 &current_remaining, TU32 &all_cached_frames)
{
    if (mbRemaingData) {
        mbRemaingData = 0;

        out_buffer->SetBufferFlags(CIBuffer::KEY_FRAME);
        out_buffer->SetBufferType(EBufferType_VideoES);
        out_buffer->mpCustomizedContent = NULL;
        out_buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;

        if (DUnlikely(!mbSendSyncBuffer)) {
            mbSendSyncBuffer = 1;
            out_buffer->SetBufferFlagBits(CIBuffer::SYNC_POINT);
            out_buffer->mVideoWidth = mCapMaxCodedWidth;
            out_buffer->mVideoHeight = mCapMaxCodedHeight;
            out_buffer->mVideoBitrate = mBitrate;
            out_buffer->mVideoFrameRateNum = mCurrentFramerateNum;
            out_buffer->mVideoFrameRateDen = mCurrentFramerateDen;
            out_buffer->mVideoRoughFrameRate = mFrameRate;
        }

        out_buffer->SetDataPtr(mpRemainingData);
        out_buffer->SetDataSize(mnRemainingDataSize);

        //LOGM_PRINTF("key frame %d\n", mnRemainingDataSize);
        //gfPrintMemory(mpRemainingData, 8);

        out_buffer->SetBufferPTS(mRemainBufferTimestamp);
        out_buffer->SetBufferNativePTS(mRemainBufferTimestamp);

        current_remaining = 0;
        all_cached_frames = 0;
        return EECode_OK;
    }

    if (mfDSPAL.f_read_bitstream && (0 < mIavFd)) {
        SAmbaDSPReadBitstream param;
        TU8 *p = NULL;
        TU8 nal_type = 0;
        TU32 skip_size = 0;
        TU32 data_size = 0;
        TInt ret = 0;

        param.stream_id = mStreamID;
        ret = mfDSPAL.f_read_bitstream(mIavFd, &param);
        if (ret) {
            LOGM_ERROR("read bitstream error\n");
            return EECode_Error;
        }

        if ((!mCapMaxCodedWidth) || (!mCapMaxCodedHeight)) {
            mCapMaxCodedWidth = param.video_width;
            mCapMaxCodedHeight = param.video_height;
            LOGM_NOTICE("video width %d, height %d\n", mCapMaxCodedWidth, mCapMaxCodedHeight);
        }

        p = (TU8 *) mMapBSB.base + param.offset;
        data_size = param.size;
        skip_size = gfSkipDelimter(p);
        p += skip_size;
        data_size -= skip_size;

        if ((0x1 == p[2]) && (0x0 == p[0]) && (0x0 == p[1])) {
            nal_type = p[3] & 0x1f;
        } else if ((0x1 == p[3]) && (0x0 == p[0]) && (0x0 == p[1]) && (0x0 == p[2])) {
            nal_type = p[4] & 0x1f;
        } else {
            LOGM_ERROR("bad data, no start code? skip\n");
            gfPrintMemory(p, 8);
            return EECode_OK_NoOutputYet;
        }

        if ((ENalType_IDR == nal_type) || (ENalType_SPS == nal_type)) {
            mbWaitFirstKeyframe = 0;
            if (!mCurrentFramerateNum) {
                mCurrentFramerateNum = 90000;
                if (mPreviousTimestamp) {
                    mCurrentFramerateDen = param.pts - mPreviousTimestamp;
                    if ((3005 > mCurrentFramerateDen) && (3001 < mCurrentFramerateDen)) {
                        mCurrentFramerateDen = 3003;
                    } else if ((6009 > mCurrentFramerateDen) && (6002 < mCurrentFramerateDen)) {
                        mCurrentFramerateDen = 6006;
                    } else if ((1502 > mCurrentFramerateDen) && (1499 < mCurrentFramerateDen)) {
                        mCurrentFramerateDen = 1501;
                    }
                    mFrameRate = (float)mCurrentFramerateNum / (float)mCurrentFramerateDen;
                    LOGM_NOTICE("get estimzted den %d, fps %f\n", mCurrentFramerateDen, mFrameRate);
                } else {
                    mCurrentFramerateDen = 3003;
                    mFrameRate = (float)mCurrentFramerateNum / (float)mCurrentFramerateDen;
                    LOGM_NOTICE("use default %d, fps %f\n", mCurrentFramerateDen, mFrameRate);
                }
            }

            out_buffer->SetBufferFlags(CIBuffer::KEY_FRAME);
            if ((ENalType_SPS == nal_type)) {
                TU8 *p_idr = gfNALUFindPPSEnd(p, data_size);
                //out_buffer->SetBufferFlagBits(CIBuffer::WITH_EXTRA_DATA);
                if (!mbSendExtrabuffer) {
                    if (p_idr) {
                        mVideoExtraDataSize = (TU32)(p_idr - p);
                        LOGM_NOTICE("extra data size %d\n", mVideoExtraDataSize);
                        if (mpVideoExtraData) {
                            DDBG_FREE(mpVideoExtraData, "VEVE");
                        }
                        mpVideoExtraData = (TU8 *) DDBG_MALLOC(mVideoExtraDataSize, "VEVE");
                        if (!mpVideoExtraData) {
                            LOGM_FATAL("no memory\n");
                            return EECode_NoMemory;
                        }
                    } else {
                        LOGM_ERROR("no idr found?\n");
                        gfPrintMemory(p, 128);
                        return EECode_DataCorruption;
                    }

                    memcpy(mpVideoExtraData, p, mVideoExtraDataSize);

                    out_buffer->SetBufferType(EBufferType_VideoExtraData);
                    out_buffer->mpCustomizedContent = NULL;
                    out_buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;

                    if (DUnlikely(!mbSendSyncBuffer)) {
                        mbSendSyncBuffer = 1;
                        out_buffer->SetBufferFlagBits(CIBuffer::SYNC_POINT);
                        out_buffer->mVideoWidth = mCapMaxCodedWidth;
                        out_buffer->mVideoHeight = mCapMaxCodedHeight;
                        out_buffer->mVideoBitrate = mBitrate;
                        out_buffer->mVideoFrameRateNum = mCurrentFramerateNum;
                        out_buffer->mVideoFrameRateDen = mCurrentFramerateDen;
                        out_buffer->mVideoRoughFrameRate = mFrameRate;
                    }

                    out_buffer->SetDataPtr(mpVideoExtraData);
                    out_buffer->SetDataSize(mVideoExtraDataSize);

                    mRemainBufferTimestamp = param.pts;

                    out_buffer->SetBufferPTS(mRemainBufferTimestamp);
                    out_buffer->SetBufferNativePTS(mRemainBufferTimestamp);

                    current_remaining = 1;
                    all_cached_frames = 1;

#if 1
                    p += mVideoExtraDataSize;
                    data_size -= mVideoExtraDataSize;
                    skip_size = gfSkipSEI(p, 128);
                    p += skip_size;
                    data_size -= skip_size;
#endif

                    mpRemainingData = p;
                    mnRemainingDataSize = data_size;
                    mbRemaingData = 1;

                    mbSendExtrabuffer = 1;
                    return EECode_OK;
                } else {
                    if (p_idr) {
                        skip_size = (TU32)(p_idr - p);
                        p += skip_size;
                        data_size -= skip_size;
                    }
                }
            }
        } else if (mbWaitFirstKeyframe) {
            LOGM_NOTICE("skip till key frame\n");
            mPreviousTimestamp = param.pts;
            return EECode_OK_NoOutputYet;
        } else {
            out_buffer->SetBufferFlags(0);
        }

        out_buffer->SetBufferType(EBufferType_VideoES);
        out_buffer->mpCustomizedContent = NULL;
        out_buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;

        if (DUnlikely(!mbSendSyncBuffer)) {
            mbSendSyncBuffer = 1;
            out_buffer->SetBufferFlagBits(CIBuffer::SYNC_POINT);
            out_buffer->mVideoWidth = mCapMaxCodedWidth;
            out_buffer->mVideoHeight = mCapMaxCodedHeight;
            out_buffer->mVideoBitrate = mBitrate;
            out_buffer->mVideoFrameRateNum = mCurrentFramerateNum;
            out_buffer->mVideoFrameRateDen = mCurrentFramerateDen;
            out_buffer->mVideoRoughFrameRate = mFrameRate;
        }

        skip_size = gfSkipSEI(p, 128);
        p += skip_size;
        data_size -= skip_size;

        out_buffer->SetDataPtr(p);
        out_buffer->SetDataSize(data_size);

#if 0
        if (CIBuffer::KEY_FRAME & out_buffer->GetBufferFlags()) {
            LOGM_PRINTF("key frame 2 %d\n", data_size);
            gfPrintMemory(p, 8);
        } else {
            LOGM_PRINTF("p frame %d\n", data_size);
            gfPrintMemory(p, 8);
        }
#endif

        out_buffer->SetBufferPTS((TTime)param.pts);
        out_buffer->SetBufferNativePTS((TTime)param.pts);

        current_remaining = 0;
        all_cached_frames = 0;
    } else {
        LOGM_ERROR("error\n");
        return EECode_OSError;
    }

    return EECode_OK;
}

EECode CAmbaVideoEncoder::VideoEncoderProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
    return EECode_OK;
}

void CAmbaVideoEncoder::Delete()
{
    LOGM_INFO("CAmbaVideoEncoder::Delete().\n");

    DestroyContext();

    inherited::Delete();
}

void CAmbaVideoEncoder::PrintStatus()
{
    LOGM_PRINTF("heart beat %d\n", mDebugHeartBeat);
    mDebugHeartBeat = 0;
}

#endif

