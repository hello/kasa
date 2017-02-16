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

#include "amba_video_decoder.h"

IVideoDecoder *gfCreateAmbaVideoDecoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
    return CAmbaVideoDecoder::Create(pName, pPersistMediaConfig, pEngineMsgSink);
}

//-----------------------------------------------------------------------
//
// CAmbaVideoDecoder
//
//-----------------------------------------------------------------------
CAmbaVideoDecoder::CAmbaVideoDecoder(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited(pname)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpMsgSink(pMsgSink)
    , mCodecFormat(StreamFormat_Invalid)
    , mIavFd(-1)
    , mCapMaxCodedWidth(0)
    , mCapMaxCodedHeight(0)
    , mbAddAmbaGopHeader(1)
    , mDecId(0)
    , mbBWplayback(0)
    , mVoutNumber(1)
    , mbEnableVoutDigital(0)
    , mbEnableVoutHDMI(0)
    , mbEnableVoutCVBS(0)
    , mFeedingRule(DecoderFeedingRule_AllFrames)
    , mSpecifiedTimeScale(DDefaultVideoFramerateNum)
    , mSpecifiedFrameTick(DDefaultVideoFramerateDen)
    , mpBitSreamBufferStart(NULL)
    , mpBitSreamBufferEnd(NULL)
    , mpBitStreamBufferCurPtr(NULL)
    , mpVideoExtraData(NULL)
    , mVideoExtraDataSize(0)
{
    memset(&mMapBSB, 0x0, sizeof(mMapBSB));
}

IVideoDecoder *CAmbaVideoDecoder::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    CAmbaVideoDecoder *result = new CAmbaVideoDecoder(pname, pPersistMediaConfig, pMsgSink);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CObject *CAmbaVideoDecoder::GetObject0() const
{
    return (CObject *) this;
}

EECode CAmbaVideoDecoder::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleAmbaDecoder);

    DASSERT(mpPersistMediaConfig);

    mConfigLogLevel = ELogLevel_Info;

    //hard code here
    mFrameRateNum = DDefaultVideoFramerateNum;
    mFrameRateDen = DDefaultVideoFramerateDen;
    mFrameRate = 30;
    mbEnableVoutDigital = mpPersistMediaConfig->dsp_config.use_digital_vout;
    mbEnableVoutHDMI = mpPersistMediaConfig->dsp_config.use_hdmi_vout;
    mbEnableVoutCVBS = mpPersistMediaConfig->dsp_config.use_cvbs_vout;
    gfSetupDSPAlContext(&mfDSPAL);

    return EECode_OK;
}

CAmbaVideoDecoder::~CAmbaVideoDecoder()
{
    if (mpVideoExtraData) {
        free(mpVideoExtraData);
        mpVideoExtraData = NULL;
    }
}

EECode CAmbaVideoDecoder::SetupContext(SVideoDecoderInputParam *param)
{
    EECode err = EECode_OK;
    DASSERT(mpPersistMediaConfig);

    if (param) {
        mDecId = param->index;
        mCodecFormat = param->format;
        if (param->cap_max_width && param->cap_max_height) {
            mCapMaxCodedWidth = param->cap_max_width;
            mCapMaxCodedHeight = param->cap_max_height;
        } else {
            mCapMaxCodedWidth = 720;
            mCapMaxCodedHeight = 480;
            LOGM_WARN("max coded size not specified, use default %u x %u\n", mCapMaxCodedWidth, mCapMaxCodedHeight);
        }

        gfFillAmbaGopHeader(mpAmbaGopHeader, 3003, 90000, 0, 30, 1);

        mIavFd = mpPersistMediaConfig->dsp_config.device_fd;

        err = enterDecodeMode();
        if (EECode_OK != err) {
            LOG_FATAL("enter decode mode fail\n");
            return err;
        }

        err = createDecoder((TU8)mDecId, EAMDSP_VIDEO_CODEC_TYPE_H264, mCapMaxCodedWidth, mCapMaxCodedHeight);
        if (EECode_OK != err) {
            LOG_FATAL("create decoder fail\n");
            return err;
        }

        DASSERT(mfDSPAL.f_speed);
        DASSERT(mfDSPAL.f_start);
        mfDSPAL.f_start(mIavFd, (TU8)mDecId);
        mfDSPAL.f_speed(mIavFd, (TU8)mDecId, 0x100, EAMDSP_PB_SCAN_MODE_ALL_FRAMES, EAMDSP_PB_DIRECTION_FW);

        mbBWplayback = 0;
    } else {
        LOGM_FATAL("NULL input in CAmbaVideoDecoder::SetupContext\n");
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CAmbaVideoDecoder::DestroyContext()
{
    EECode err;

    mfDSPAL.f_stop(mIavFd, (TU8)mDecId, 1);

    err = destroyDecoder();
    if (EECode_OK != err) {
        LOG_FATAL("destroy decoder fail\n");
    }

    err = leaveDecodeMode();
    if (EECode_OK != err) {
        LOG_FATAL("leave decode mode fail\n");
    }

    return err;
}

EECode CAmbaVideoDecoder::SetBufferPool(IBufferPool *buffer_pool)
{
    return EECode_OK;
}

EECode CAmbaVideoDecoder::Start()
{
    return EECode_OK;
}

EECode CAmbaVideoDecoder::Stop()
{
    return EECode_OK;
}

EECode CAmbaVideoDecoder::Decode(CIBuffer *in_buffer, CIBuffer *&out_buffer)
{
    return EECode_NotSupported;
}

EECode CAmbaVideoDecoder::Flush()
{
    if (mfDSPAL.f_stop && mfDSPAL.f_start && mfDSPAL.f_speed && (0 < mIavFd)) {
        TInt ret = mfDSPAL.f_stop(mIavFd, mDecId, 1);
        DASSERT(!ret);
        LOGM_NOTICE("stop decoder done\n");
        mpBitStreamBufferCurPtr = mpBitSreamBufferStart;

        ret = mfDSPAL.f_start(mIavFd, mDecId);
        DASSERT(!ret);
        //ret = mfDSPAL.f_speed(mIavFd, (TU8)mDecId, 0x100, EAMDSP_PB_SCAN_MODE_ALL_FRAMES, EAMDSP_PB_DIRECTION_FW);
        //DASSERT(!ret);
        LOGM_NOTICE("start decoder done\n");
    }

    return EECode_OK;
}

EECode CAmbaVideoDecoder::Suspend()
{
    return EECode_OK;
}

void CAmbaVideoDecoder::Delete()
{
    LOGM_INFO("CAmbaVideoDecoder::Delete().\n");

    inherited::Delete();
}

EECode CAmbaVideoDecoder::SetExtraData(TU8 *p, TMemSize size)
{
    if (DUnlikely((!p) || (!size))) {
        LOGM_ERROR("NULL extradata %p, or zero size %ld\n", p, size);
        return EECode_BadParam;
    }

    if (mpVideoExtraData && (mVideoExtraDataSize == size)) {
        if (!memcmp(mpVideoExtraData, p, size)) {
            return EECode_OK;
        }
    }

    if (mpVideoExtraData) {
        free(mpVideoExtraData);
        mpVideoExtraData = NULL;
        mVideoExtraDataSize = 0;
    }

    DASSERT(!mpVideoExtraData);
    DASSERT(!mVideoExtraDataSize);

    mVideoExtraDataSize = size;
    LOGM_INFO("video extra data size %ld, first 8 bytes: 0x%02x 0x%02x 0x%02x 0x%02x, 0x%02x 0x%02x 0x%02x 0x%02x\n", mVideoExtraDataSize, \
              p[0], p[1], p[2], p[3], \
              p[4], p[5], p[6], p[7]);
    mpVideoExtraData = (TU8 *)DDBG_MALLOC(mVideoExtraDataSize, "VDAE");
    if (mpVideoExtraData) {
        memcpy(mpVideoExtraData, p, size);
    } else {
        LOGM_FATAL("NO memory\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

EECode CAmbaVideoDecoder::SetPbRule(TU8 direction, TU8 feeding_rule, TU16 speed)
{
    if (mfDSPAL.f_speed && (0 < mIavFd)) {
        TInt ret = 0;
#if 0
        if (mbBWplayback != direction) {
            LOGM_NOTICE("change direction\n");
            //destroyDecoder();

            //EECode err = createDecoder((TU8)mDecId, EAMDSP_VIDEO_CODEC_TYPE_H264, mCapMaxCodedWidth, mCapMaxCodedHeight);
            //if (EECode_OK != err) {
            //    LOG_FATAL("create decoder fail\n");
            //   return err;
            //}

            ret = mfDSPAL.f_stop(mIavFd, mDecId, 1);
            DASSERT(!ret);

            ret = mfDSPAL.f_start(mIavFd, mDecId);
            DASSERT(!ret);

            mpBitStreamBufferCurPtr = mpBitSreamBufferStart;
        } else {
            ret = mfDSPAL.f_stop(mIavFd, mDecId, 1);
            DASSERT(!ret);
            LOGM_NOTICE("stop decoder done\n");

            ret = mfDSPAL.f_start(mIavFd, mDecId);
            DASSERT(!ret);
            mpBitStreamBufferCurPtr = mpBitSreamBufferStart;
            LOGM_NOTICE("start decoder done\n");
        }
#endif

        if (DecoderFeedingRule_AllFrames == feeding_rule) {
            feeding_rule = EAMDSP_PB_SCAN_MODE_ALL_FRAMES;
        } else if (DecoderFeedingRule_IDROnly == feeding_rule) {
            feeding_rule = EAMDSP_PB_SCAN_MODE_I_ONLY;
        } else {
            LOGM_FATAL("bad feeding_rule %d\n", feeding_rule);
            return EECode_BadParam;
        }

        mbBWplayback = direction;
        mFeedingRule = feeding_rule;

        LOGM_PRINTF("direction %d, speed %x, scan mode %d\n", direction, speed, feeding_rule);

        ret = mfDSPAL.f_speed(mIavFd, (TU8)mDecId, speed, feeding_rule, direction);
        DASSERT(!ret);
        LOGM_NOTICE("set decoder speed\n");
    }

    return EECode_OK;
}

EECode CAmbaVideoDecoder::SetFrameRate(TUint framerate_num, TUint frameate_den)
{
    return EECode_OK;
}

EDecoderMode CAmbaVideoDecoder::GetDecoderMode() const
{
    return EDecoderMode_Direct;
}

EECode CAmbaVideoDecoder::DecodeDirect(CIBuffer *in_buffer)
{
    EECode err;

    if ((EBufferType_VideoExtraData != in_buffer->GetBufferType()) && (EBufferType_VideoES != in_buffer->GetBufferType())) {
        LOG_FATAL("bad buffer type %d\n", in_buffer->GetBufferType());
        return EECode_InternalLogicalBug;
    }

    switch (mCodecFormat) {

        case StreamFormat_H264:
            err = decodeH264(in_buffer);
            break;

        default:
            LOGM_FATAL("need add codec support %d\n", mCodecFormat);
            err = EECode_NotSupported;
            break;
    }

    return err;
}

EECode CAmbaVideoDecoder::SetBufferPoolDirect(IOutputPin *p_output_pin, IBufferPool *p_bufferpool)
{
    return EECode_OK;
}

void CAmbaVideoDecoder::PrintStatus()
{
    LOGM_PRINTF("heart beat %d\n", mDebugHeartBeat);
    mDebugHeartBeat = 0;
    DASSERT(mfDSPAL.f_query_print_decode_status);
    mfDSPAL.f_query_print_decode_status(mIavFd, (TU8)mDecId);
}

TU8 *CAmbaVideoDecoder::copyDataToBSB(TU8 *ptr, TU8 *buffer, TUint size)
{
    if (ptr + size <= mpBitSreamBufferEnd) {
        memcpy((void *)ptr, (const void *)buffer, size);
        return ptr + size;
    } else {
        TInt room = mpBitSreamBufferEnd - ptr;
        TU8 *ptr2;
        memcpy((void *)ptr, (const void *)buffer, room);
        ptr2 = buffer + room;
        size -= room;
        memcpy((void *)mpBitSreamBufferStart, (const void *)ptr2, size);
        return mpBitSreamBufferStart + size;
    }
}

EECode CAmbaVideoDecoder::decodeH264(CIBuffer *in_buffer)
{
    TU8 *p_data;
    TUint size;
    TUint b_append_extradata = 0;
    TInt ret = 0;
    TU8 h264_eos[] = {0x00, 0x00, 0x00, 0x01, 0x0A};

    SAmbaDSPDecode dec;

    DASSERT(in_buffer);
    DASSERT(mfDSPAL.f_request_bsb);
    DASSERT(mfDSPAL.f_decode);

    size = in_buffer->GetDataSize();
    p_data = in_buffer->GetDataPtr();

    if (in_buffer->GetBufferFlags() & CIBuffer::KEY_FRAME) {
        if (ENalType_SPS != (p_data[4] & 0x1f)) {
            b_append_extradata = 1;
        }
    }

    if (mpBitStreamBufferCurPtr == mpBitSreamBufferEnd) {
        mpBitStreamBufferCurPtr = mpBitSreamBufferStart;
    }

    dec.decoder_id = (TU8) mDecId;
    dec.num_frames = 1;
    dec.start_ptr_offset = (TU32)(mpBitStreamBufferCurPtr - mpBitSreamBufferStart);
    dec.first_frame_display = 0;

    ret = mfDSPAL.f_request_bsb(mIavFd, (TU8)mDecId, size + 1024, mpBitStreamBufferCurPtr);
    if (DUnlikely(0 > ret)) {
        LOGM_ERROR("request bsb fail, return %d\n", ret);
        return EECode_Error;
    }

    if (mbAddAmbaGopHeader && in_buffer->GetBufferFlags() & CIBuffer::KEY_FRAME) {
        gfUpdateAmbaGopHeader(mpAmbaGopHeader, (TU32)in_buffer->GetBufferNativePTS());
        mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, mpAmbaGopHeader, DAMBA_GOP_HEADER_LENGTH);
    }

    if (b_append_extradata) {
        mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, mpVideoExtraData, mVideoExtraDataSize);
    }
    mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, p_data, size);

    if (mbBWplayback) {
        mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, h264_eos, sizeof(h264_eos));
    }

    dec.end_ptr_offset = (TU32)(mpBitStreamBufferCurPtr - mpBitSreamBufferStart);
    dec.first_frame_display = in_buffer->GetBufferNativePTS();

    ret = mfDSPAL.f_decode(mIavFd, &dec);

    return EECode_OK;
}

EECode CAmbaVideoDecoder::enterDecodeMode()
{
    TInt ret = 0;
    TInt vout_num = 0;
    TInt has_digital = 0, has_hdmi = 0, has_cvbs = 0;
    DASSERT(mfDSPAL.f_map_bsb);
    DASSERT(mfDSPAL.f_get_vout_info);
    DASSERT(mfDSPAL.f_enter_mode);
    DASSERT(0 < mIavFd);

    mMapBSB.b_two_times = 0;
    mMapBSB.b_enable_write = 1;
    mMapBSB.b_enable_read = 0;
    ret = mfDSPAL.f_map_bsb(mIavFd, &mMapBSB);
    if (0 > ret) {
        LOGM_FATAL("map bsb vout fail\n");
        return EECode_Error;
    }

    ret = mfDSPAL.f_get_vout_info(mIavFd, 0, EAMDSP_VOUT_TYPE_DIGITAL, &mVoutInfos[0]);
    if ((0 > ret) || (!mVoutInfos[0].width) || (!mVoutInfos[0].height)) {
        LOGM_NOTICE("digital vout not enabled\n");
        mbEnableVoutDigital = 0;
    } else {
        has_digital = 1;
        vout_num ++;
    }

    ret = mfDSPAL.f_get_vout_info(mIavFd, 1, EAMDSP_VOUT_TYPE_HDMI, &mVoutInfos[1]);
    if ((0 > ret) || (!mVoutInfos[1].width) || (!mVoutInfos[1].height)) {
        LOGM_NOTICE("hdmi vout not enabled\n");
        mbEnableVoutHDMI = 0;
    } else {
        has_hdmi = 1;
        vout_num ++;
    }
    ret = mfDSPAL.f_get_vout_info(mIavFd, 1, EAMDSP_VOUT_TYPE_CVBS, &mVoutInfos[2]);
    if ((0 > ret) || (!mVoutInfos[2].width) || (!mVoutInfos[2].height)) {
        LOGM_NOTICE("cvbs vout not enabled\n");
        mbEnableVoutCVBS = 0;
    } else {
        has_cvbs = 1;
        vout_num ++;
    }
    if (!vout_num) {
        LOG_FATAL("no vout found\n");
        return EECode_Error;
    }
    if ((!mbEnableVoutDigital) && (!mbEnableVoutHDMI) && (!mbEnableVoutCVBS)) {
        if (has_digital) {
            mbEnableVoutDigital = 1;
        } else if (has_hdmi) {
            mbEnableVoutHDMI = 1;
        } else if (has_cvbs) {
            mbEnableVoutCVBS = 1;
        }
        LOGM_NOTICE("guess default vout: cvbs %d, digital %d, hdmi %d\n", mbEnableVoutCVBS, mbEnableVoutDigital, mbEnableVoutHDMI);
    }
    mVoutNumber = 1;
    mModeConfig.num_decoder = 1;

    mModeConfig.decoder_configs[0].max_frm_num = 6;
    mModeConfig.decoder_configs[0].max_frm_width = mCapMaxCodedWidth;
    mModeConfig.decoder_configs[0].max_frm_height = mCapMaxCodedHeight;

    LOGM_NOTICE("enter decode mode...\n");
    ret = mfDSPAL.f_enter_mode(mIavFd, &mModeConfig);
    if (0 > ret) {
        LOGM_ERROR("enter decode mode fail, ret %d\n", ret);
        return EECode_Error;
    }
    LOGM_NOTICE("enter decode mode done\n");

    return EECode_OK;
}

EECode CAmbaVideoDecoder::leaveDecodeMode()
{
    TInt ret = 0;

    DASSERT(mfDSPAL.f_leave_mode);
    DASSERT(0 < mIavFd);

    LOGM_NOTICE("leave decode mode...\n");
    ret = mfDSPAL.f_leave_mode(mIavFd);
    if (0 > ret) {
        LOGM_ERROR("leave decode mode fail, ret %d\n", ret);
        return EECode_Error;
    }
    LOGM_NOTICE("leave decode mode done\n");

    return EECode_OK;
}

EECode CAmbaVideoDecoder::createDecoder(TU8 decoder_id, TU8 decoder_type, TU32 width, TU32 height)
{
    TInt ret = 0;

    DASSERT(mfDSPAL.f_create_decoder);
    DASSERT(0 < mIavFd);

    mDecoderInfo.decoder_id = decoder_id;
    mDecoderInfo.decoder_type = decoder_type;
    mDecoderInfo.width = width;
    mDecoderInfo.height = height;

    if (mbEnableVoutDigital) {
        mDecoderInfo.vout_configs[0].enable = 1;
        mDecoderInfo.vout_configs[0].vout_id = 0;
        mDecoderInfo.vout_configs[0].flip = mVoutInfos[0].flip;
        mDecoderInfo.vout_configs[0].rotate = mVoutInfos[0].rotate;
        mDecoderInfo.vout_configs[0].target_win_offset_x = mVoutInfos[0].offset_x;
        mDecoderInfo.vout_configs[0].target_win_offset_y = mVoutInfos[0].offset_y;
        mDecoderInfo.vout_configs[0].target_win_width = mVoutInfos[0].width;
        mDecoderInfo.vout_configs[0].target_win_height = mVoutInfos[0].height;
        mDecoderInfo.vout_configs[0].zoom_factor_x = (mVoutInfos[0].width * 0x10000) / width;
        mDecoderInfo.vout_configs[0].zoom_factor_y = (mVoutInfos[0].height * 0x10000) / height;
        mDecoderInfo.vout_configs[0].vout_mode = mVoutInfos[0].mode;
    } else if (mbEnableVoutHDMI) {
        mDecoderInfo.vout_configs[0].enable = 1;
        mDecoderInfo.vout_configs[0].vout_id = 1;
        mDecoderInfo.vout_configs[0].flip = mVoutInfos[1].flip;
        mDecoderInfo.vout_configs[0].rotate = mVoutInfos[1].rotate;
        mDecoderInfo.vout_configs[0].target_win_offset_x = mVoutInfos[1].offset_x;
        mDecoderInfo.vout_configs[0].target_win_offset_y = mVoutInfos[1].offset_y;
        mDecoderInfo.vout_configs[0].target_win_width = mVoutInfos[1].width;
        mDecoderInfo.vout_configs[0].target_win_height = mVoutInfos[1].height;
        mDecoderInfo.vout_configs[0].zoom_factor_x = (mVoutInfos[1].width * 0x10000) / width;
        mDecoderInfo.vout_configs[0].zoom_factor_y = (mVoutInfos[1].height * 0x10000) / height;
        mDecoderInfo.vout_configs[0].vout_mode = mVoutInfos[1].mode;
    } else if (mbEnableVoutCVBS) {
        mDecoderInfo.vout_configs[0].enable = 1;
        mDecoderInfo.vout_configs[0].vout_id = 1;
        mDecoderInfo.vout_configs[0].flip = mVoutInfos[2].flip;
        mDecoderInfo.vout_configs[0].rotate = mVoutInfos[2].rotate;
        mDecoderInfo.vout_configs[0].target_win_offset_x = mVoutInfos[2].offset_x;
        mDecoderInfo.vout_configs[0].target_win_offset_y = mVoutInfos[2].offset_y;
        mDecoderInfo.vout_configs[0].target_win_width = mVoutInfos[2].width;
        mDecoderInfo.vout_configs[0].target_win_height = mVoutInfos[2].height;
        mDecoderInfo.vout_configs[0].zoom_factor_x = (mVoutInfos[2].width * 0x10000) / width;
        mDecoderInfo.vout_configs[0].zoom_factor_y = (mVoutInfos[2].height * 0x10000) / height;
        mDecoderInfo.vout_configs[0].vout_mode = mVoutInfos[2].mode;
    }  else {
        LOGM_FATAL("no vout\n");
        return EECode_Error;
    }
    mDecoderInfo.num_vout = mVoutNumber;

    ret = mfDSPAL.f_create_decoder(mIavFd, &mDecoderInfo);
    if (0 > ret) {
        LOGM_ERROR("create decoder fail, ret %d\n", ret);
        return EECode_Error;
    }

    mpBitStreamBufferCurPtr = mpBitSreamBufferStart = (TU8 *)mMapBSB.base + mDecoderInfo.bsb_start_offset;
    mpBitSreamBufferEnd = mpBitSreamBufferStart + mMapBSB.size;

    return EECode_OK;
}

EECode CAmbaVideoDecoder::destroyDecoder()
{
    TInt ret = 0;

    DASSERT(mfDSPAL.f_destroy_decoder);
    DASSERT(0 < mIavFd);

    LOGM_NOTICE("destroy decoder...\n");
    ret = mfDSPAL.f_destroy_decoder(mIavFd, mDecId);
    if (0 > ret) {
        LOGM_ERROR("destroy decoder fail, ret %d\n", ret);
        return EECode_Error;
    }
    LOGM_NOTICE("destroy decoder done\n");

    return EECode_OK;
}

#endif

