/*******************************************************************************
 * am_amba_video_encoder.cpp
 *
 * History:
 *    2012/08/03 - [Zhi He] create file
 *
 * Copyright (C) 2015 Ambarella, Inc.
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

#include "am_native.h"
#include "am_native_log.h"

#include "am_internal.h"

#include "am_osal.h"
#include "am_framework.h"
#include "am_modules_interface.h"
#include "am_codec_interface.h"

#include "am_amba_dsp.h"

#ifdef BUILD_MODULE_AMBA_DSP

#include "am_amba_video_encoder.h"

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
  , mpBitstreamBufferStart(NULL)
  , mBitstreamBufferSize(0)
  , mStreamID(0xffffffff)
  , mRemainingDescNumber(0)
  , mCurrentDescIndex(0)
  , mbCopyOut(0)
  , mbSkipDelimiter(1)
  , mbSkipSEI(1)
  , mbRemaingData(0)
  , mRemainingDataOutputIndex(0)
  , mpRemainingData(NULL)
  , mnRemainingDataSize(0)
{
  mRemainBufferTimestamp = 0;
  memset(&mMapBSB, 0x0, sizeof(mMapBSB));
  for (TU32 i = 0; i < DMaxStreamNumber; i ++) {
    mbWaitFirstKeyframe[i] = 1;
    mCodecFormat[i] = StreamFormat_Invalid;
    mBitrate[i] = 1024 * 1024;
    mVideoWidth[i] = 0;
    mVideoHeight[i] = 0;
    mFramerateNum[i] = 0;
    mFramerateDen[i] = 0;
    mFramerate[i] = 0;
    mpVideoExtraData[i] = 0;
    mVideoExtraDataSize[i] = 0;
    mbAlreadySendSyncBuffer[i] = 0;
  }
  mVinFramerateNum = 0;
  mVinFramerateDen = 0;
  mVinFramerate = 0;
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
  for (TU32 i = 0; i < DMaxStreamNumber; i ++) {
    if (mpVideoExtraData[i]) {
      DDBG_FREE(mpVideoExtraData[i], "VEVE");
      mpVideoExtraData[i] = NULL;
    }
  }
}

EECode CAmbaVideoEncoder::SetupContext(SVideoEncoderInputParam *param)
{
  SAmbaDSPVinInfo vininfo;
  TInt ret = 0;
  DASSERT(mpPersistMediaConfig);
  DASSERT(mfDSPAL.f_map_bsb);
  DASSERT(mfDSPAL.f_get_vin_info);
  DASSERT(mfDSPAL.f_get_stream_framefactor);
  //if (mpPersistMediaConfig->rtsp_server_config.video_stream_id) {
  //  mStreamID = mpPersistMediaConfig->rtsp_server_config.video_stream_id;
  //}
  if (param->stream_id) {
    mStreamID = param->stream_id;
  }
  LOGM_CONFIGURATION("mStreamID %08x\n", mStreamID);
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
  ret = mfDSPAL.f_get_vin_info(mIavFd, &vininfo);
  if (ret) {
    LOGM_ERROR("get vin info fail, ret %d\n", ret);
    return EECode_BadState;
  }
  DASSERT(vininfo.fr_den && vininfo.fr_num);
  mVinFramerateNum = vininfo.fr_num;
  mVinFramerateDen = vininfo.fr_den;
  mVinFramerate = (float) vininfo.fr_num / (float) vininfo.fr_den;
  LOGM_CONFIGURATION("vin framerate: %f, %d/%d\n", mVinFramerate, mVinFramerateNum, mVinFramerateDen);
  return EECode_OK;
}

void CAmbaVideoEncoder::DestroyContext()
{
  TInt ret = mfDSPAL.f_unmap_bsb(mIavFd, &mMapBSB);
  if (ret) {
    LOGM_ERROR("unmap fail\n");
  }
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

EECode CAmbaVideoEncoder::Encode(CIBuffer *in_buffer, CIBuffer *out_buffer, TU32 &current_remaining, TU32 &all_cached_frames, TU32 &output_index)
{
  if (mbRemaingData) {
    mbRemaingData = 0;
    output_index = mRemainingDataOutputIndex;
    DASSERT(mRemainingDataOutputIndex < DMaxStreamNumber);
    out_buffer->SetBufferFlags(CIBuffer::KEY_FRAME);
    out_buffer->SetBufferType(EBufferType_VideoES);
    out_buffer->mpCustomizedContent = NULL;
    out_buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;
    out_buffer->mVideoWidth = mVideoWidth[mRemainingDataOutputIndex];
    out_buffer->mVideoHeight = mVideoHeight[mRemainingDataOutputIndex];
    out_buffer->mVideoBitrate = mBitrate[mRemainingDataOutputIndex];
    out_buffer->mVideoFrameRateNum = mFramerateNum[mRemainingDataOutputIndex];
    out_buffer->mVideoFrameRateDen = mFramerateDen[mRemainingDataOutputIndex];
    out_buffer->mVideoRoughFrameRate = mFramerate[mRemainingDataOutputIndex];
    out_buffer->mContentFormat = mCodecFormat[mRemainingDataOutputIndex];
    out_buffer->SetDataPtr(mpRemainingData);
    out_buffer->SetDataSize(mnRemainingDataSize);
    if (StreamFormat_H265 == mCodecFormat[mRemainingDataOutputIndex]) {
      out_buffer->mbDataSegment = 1;
      out_buffer->SetBufferFlagBits(CIBuffer::DATA_SEGMENT_BEGIN_INDICATOR);
    }
    out_buffer->SetBufferPTS(mRemainBufferTimestamp);
    out_buffer->SetBufferNativePTS(mRemainBufferTimestamp);
    out_buffer->SetBufferLinearPTS(mRemainBufferTimestamp);
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
    TU8 *p_tmp = NULL;
    ret = mfDSPAL.f_is_ready_for_read_bitstream(mIavFd);
    if (!ret) {
      gfOSsleep(1);
      LOGM_INFO("wait encode start.\n");
      return EECode_OK_NoOutputYet;
    }
    param.stream_id = mStreamID;
    param.timeout_ms = 1000;
    ret = mfDSPAL.f_read_bitstream(mIavFd, &param);
    if (0 < ret) {
      if (1 != ret) {
        gfOSsleep(1);
        LOGM_INFO("wait encode start..\n");
      } else {
        LOGM_INFO("wait encode start...\n");
      }
      return EECode_OK_NoOutputYet;
    } else if (0 > ret) {
      LOGM_ERROR("read bitstream error\n");
      return EECode_Error;
    }
    if (DMaxStreamNumber <= param.stream_id) {
      LOGM_FATAL("stream_id (%d) exceed max value %d\n", param.stream_id, DMaxStreamNumber);
      return EECode_InternalLogicalBug;
    }
    if (!param.size) {
      LOGM_CONFIGURATION("stream(%d) end\n", param.stream_id);
      mFramerateNum[param.stream_id] = 0;
      mCodecFormat[param.stream_id] = StreamFormat_Invalid;
      mbWaitFirstKeyframe[param.stream_id] = 1;
      return EECode_OK_NoOutputYet;
    }
    if (0xffffffff == mStreamID) {
      output_index = param.stream_id;
    } else {
      if (mStreamID & (1 << param.stream_id)) {
        output_index = 0;
      } else {
        return EECode_OK_NoOutputYet;
      }
    }
    if (!mFramerateNum[output_index]) {
      if (mfDSPAL.f_get_stream_framefactor) {
        SAmbaDSPStreamFramefactor framefactor;
        ret = mfDSPAL.f_get_stream_framefactor(mIavFd, output_index, &framefactor);
        if (!ret) {
          mFramerateNum[output_index] = mVinFramerateNum;
          mFramerateDen[output_index] = (TU64)((TU64) mVinFramerateDen * (TU64) framefactor.framefactor_den) / (TU64)framefactor.framefactor_num;
          mFramerate[output_index] = (float) mFramerateNum[output_index] / (float)mFramerateDen[output_index];
          LOGM_CONFIGURATION("stream [%d], get framerate num %d, den %d, fps %f\n", output_index, mFramerateNum[output_index], mFramerateDen[output_index], mFramerate[output_index]);
        } else {
          mFramerateNum[output_index] = DDefaultVideoFramerateNum;
          mFramerateDen[output_index] = DDefaultVideoFramerateDen;
          mFramerate[output_index] = (float) mFramerateNum[output_index] / (float)mFramerateDen[output_index];
          LOGM_WARN("f_get_stream_framefactor fail, stream [%d], guess framerate num %d, den %d, fps %f\n", output_index, mFramerateNum[output_index], mFramerateDen[output_index], mFramerate[output_index]);
        }
      } else {
        mFramerateNum[output_index] = DDefaultVideoFramerateNum;
        mFramerateDen[output_index] = DDefaultVideoFramerateDen;
        mFramerate[output_index] = (float) mFramerateNum[output_index] / (float)mFramerateDen[output_index];
        LOGM_WARN("no f_get_stream_framefactor, stream [%d], guess framerate num %d, den %d, fps %f\n", output_index, mFramerateNum[output_index], mFramerateDen[output_index], mFramerate[output_index]);
      }
    }
    if (StreamFormat_Invalid == mCodecFormat[output_index]) {
      mCodecFormat[output_index] = (StreamFormat) param.stream_type;
      LOGM_CONFIGURATION("stream[%d] start, video format %d\n", output_index, mCodecFormat[output_index]);
    } else if (mCodecFormat[output_index] != (StreamFormat) param.stream_type) {
      LOGM_WARN("stream[%d], video format change? 0x%x --> 0x%x\n", output_index, mCodecFormat[output_index], param.stream_type);
      mCodecFormat[output_index] = (StreamFormat) param.stream_type;
    }
    out_buffer->mContentFormat = mCodecFormat[output_index];
    p = (TU8 *) mMapBSB.base + param.offset;
    data_size = param.size;
    //LOGM_PRINTF("stream_type %d, slice_id %d, slice_num %d, w %d, h %d, pts %" DPrint64d "\n", param.stream_type, param.slice_id, param.slice_num, param.video_width, param.video_height, param.pts);
    //LOGM_PRINTF("size %d\n", data_size);
    //gfPrintMemory(p, 16);
    if (mbSkipDelimiter) {
      skip_size = gfSkipDelimter(p);
      p += skip_size;
      data_size -= skip_size;
    }
    if (StreamFormat_H264 == mCodecFormat[output_index]) {
      if (256 < data_size) {
        p_tmp = gfNALUFindFirstAVCSliceHeaderType(p, 256, nal_type);
      } else {
        p_tmp = gfNALUFindFirstAVCSliceHeaderType(p, data_size, nal_type);
      }
      if (!p_tmp) {
        LOGM_INFO("stream[%d] bad data %02x %02x %02x %02x, %02x %02x %02x %02x, no start code? skip\n", output_index, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
        //gfPrintMemory(p, 8);
        return EECode_OK_NoOutputYet;
      }
      //LOGM_PRINTF("nal_type %d\n", nal_type);
      out_buffer->mbDataSegment = 0;
      if (ENalType_IDR == nal_type) {
        mbWaitFirstKeyframe[output_index] = 0;
        out_buffer->SetBufferFlags(CIBuffer::KEY_FRAME);
        TU8 has_sps = 0, has_pps = 0, has_idr = 0;
        TU8 *p_sps = NULL, *p_pps = NULL, *p_pps_end = NULL, *p_idr = NULL;
        gfFindH264SpsPpsIdr(p, data_size, has_sps, has_pps, has_idr, p_sps, p_pps, p_pps_end, p_idr);
        if (has_sps && has_pps && p_sps && p_pps_end) {
          TU32 new_extra_data_size = (TU32)(p_pps_end - p_sps);
          //LOGM_DEBUG("extra data size %d\n", new_extra_data_size);
          if (mpVideoExtraData[output_index] && (mVideoExtraDataSize[output_index] < new_extra_data_size)) {
            DDBG_FREE(mpVideoExtraData[output_index], "VEVE");
            mpVideoExtraData[output_index] = NULL;
          }
          if (!mpVideoExtraData[output_index]) {
            mpVideoExtraData[output_index] = (TU8 *) DDBG_MALLOC(new_extra_data_size, "VEVE");
            if (!mpVideoExtraData[output_index]) {
              LOGM_FATAL("no memory\n");
              return EECode_NoMemory;
            }
          }
          mVideoExtraDataSize[output_index] = new_extra_data_size;
          memcpy(mpVideoExtraData[output_index], p_sps, mVideoExtraDataSize[output_index]);
          out_buffer->SetBufferType(EBufferType_VideoExtraData);
          out_buffer->mpCustomizedContent = NULL;
          out_buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;
          out_buffer->SetBufferFlagBits(CIBuffer::SYNC_POINT);
          out_buffer->mVideoWidth = param.video_width;
          out_buffer->mVideoHeight = param.video_height;
          out_buffer->mVideoBitrate = mBitrate[output_index];
          out_buffer->mVideoFrameRateNum = mFramerateNum[output_index];
          out_buffer->mVideoFrameRateDen = mFramerateDen[output_index];
          out_buffer->mVideoRoughFrameRate = mFramerate[output_index];
          out_buffer->SetDataPtr(mpVideoExtraData[output_index]);
          out_buffer->SetDataSize(mVideoExtraDataSize[output_index]);
          mRemainBufferTimestamp = param.pts;
          out_buffer->SetBufferPTS(mRemainBufferTimestamp);
          out_buffer->SetBufferNativePTS(mRemainBufferTimestamp);
          out_buffer->SetBufferLinearPTS(mRemainBufferTimestamp);
          current_remaining = 1;
          all_cached_frames = 1;
          p += mVideoExtraDataSize[output_index];
          data_size -= mVideoExtraDataSize[output_index];
          if (mbSkipSEI) {
            skip_size = gfSkipSEI(p, 128);
            p += skip_size;
            data_size -= skip_size;
          }
          mpRemainingData = p;
          mnRemainingDataSize = data_size;
          mVideoWidth[output_index] = param.video_width;
          mVideoHeight[output_index] = param.video_height;
          mRemainingDataOutputIndex = output_index;
          mbRemaingData = 1;
          return EECode_OK;
        }
      } else if (mbWaitFirstKeyframe[output_index]) {
        LOGM_INFO("stream[%d] skip till key frame\n", output_index);
        return EECode_OK_NoOutputYet;
      } else {
        out_buffer->SetBufferFlags(0);
      }
      out_buffer->SetBufferType(EBufferType_VideoES);
      out_buffer->mpCustomizedContent = NULL;
      out_buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;
      out_buffer->mVideoWidth = mVideoWidth[output_index];
      out_buffer->mVideoHeight = mVideoHeight[output_index];
      out_buffer->mVideoBitrate = mBitrate[output_index];
      out_buffer->mVideoFrameRateNum = mFramerateNum[output_index];
      out_buffer->mVideoFrameRateDen = mFramerateDen[output_index];
      out_buffer->mVideoRoughFrameRate = mFramerate[output_index];
      if (mbSkipSEI) {
        skip_size = gfSkipSEI(p, 128);
        p += skip_size;
        data_size -= skip_size;
      }
      out_buffer->SetDataPtr(p);
      out_buffer->SetDataSize(data_size);
      out_buffer->SetBufferPTS((TTime)param.pts);
      out_buffer->SetBufferNativePTS((TTime)param.pts);
      out_buffer->SetBufferLinearPTS((TTime)param.pts);
      current_remaining = 0;
      all_cached_frames = 0;
    } else if (StreamFormat_H265 == mCodecFormat[output_index]) {
      TU8 is_first_slice = 0;
      if (384 < data_size) {
        p_tmp = gfNALUFindFirstHEVCSliceHeaderType(p, 384, nal_type, is_first_slice);
      } else {
        p_tmp = gfNALUFindFirstHEVCSliceHeaderType(p, data_size, nal_type, is_first_slice);
      }
      if (!p_tmp) {
        LOGM_INFO("stream[%d] bad data %02x %02x %02x %02x, %02x %02x %02x %02x, no start code? skip\n", output_index, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
        //gfPrintMemory(p, 8);
        return EECode_OK_NoOutputYet;
      }
      out_buffer->mbDataSegment = 1;
      //LOGM_NOTICE("nal_type %d, is first slice %d\n", nal_type, is_first_slice);
      //if (nal_type != 1) {
      //    LOGM_ERROR("not type 1, %d\n", nal_type);
      //    gfPrintMemory(p, 128);
      //}
      if ((EHEVCNalType_IDR_W_RADL == nal_type) || (EHEVCNalType_IDR_N_LP == nal_type)) {
        if (mbWaitFirstKeyframe[output_index] && (!is_first_slice)) {
          LOGM_INFO("stream[%d], skip till key frame (first slice)\n", output_index);
          return EECode_OK_NoOutputYet;
        }
        out_buffer->SetBufferFlags(CIBuffer::KEY_FRAME);
        if (is_first_slice) {
          TU8 has_vps = 0, has_sps = 0, has_pps = 0, has_idr = 0;
          TU8 *p_vps = NULL, *p_sps = NULL, *p_pps = NULL, *p_pps_end = NULL, *p_idr = NULL;
          gfFindH265VpsSpsPpsIdr(p, data_size, has_vps, has_sps, has_pps, has_idr, p_vps, p_sps, p_pps, p_pps_end, p_idr);
          if (has_vps && has_sps && has_pps && p_vps && p_sps && p_pps_end) {
            TU32 new_extra_data_size = (TU32)(p_pps_end - p_vps);
            //LOGM_DEBUG("extra data size %d\n", new_extra_data_size);
            if (mpVideoExtraData[output_index] && (mVideoExtraDataSize[output_index] < new_extra_data_size)) {
              DDBG_FREE(mpVideoExtraData[output_index], "VEVE");
              mpVideoExtraData[output_index] = NULL;
            }
            if (!mpVideoExtraData[output_index]) {
              mpVideoExtraData[output_index] = (TU8 *) DDBG_MALLOC(new_extra_data_size, "VEVE");
              if (!mpVideoExtraData[output_index]) {
                LOGM_FATAL("no memory\n");
                return EECode_NoMemory;
              }
            }
            mVideoExtraDataSize[output_index] = new_extra_data_size;
            memcpy(mpVideoExtraData[output_index], p_vps, mVideoExtraDataSize[output_index]);
            out_buffer->SetBufferType(EBufferType_VideoExtraData);
            out_buffer->mpCustomizedContent = NULL;
            out_buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;
            out_buffer->SetBufferFlagBits(CIBuffer::SYNC_POINT);
            //out_buffer->mVideoWidth = mVideoWidth;
            //out_buffer->mVideoHeight = mVideoHeight;
            out_buffer->mVideoWidth = param.video_width;
            out_buffer->mVideoHeight = param.video_height;
            out_buffer->mVideoBitrate = mBitrate[output_index];
            out_buffer->mVideoFrameRateNum = mFramerateNum[output_index];
            out_buffer->mVideoFrameRateDen = mFramerateDen[output_index];
            out_buffer->mVideoRoughFrameRate = mFramerate[output_index];
            out_buffer->SetDataPtr(mpVideoExtraData[output_index]);
            out_buffer->SetDataSize(mVideoExtraDataSize[output_index]);
            mRemainBufferTimestamp = param.pts;
            out_buffer->SetBufferPTS(mRemainBufferTimestamp);
            out_buffer->SetBufferNativePTS(mRemainBufferTimestamp);
            out_buffer->SetBufferLinearPTS(mRemainBufferTimestamp);
            current_remaining = 1;
            all_cached_frames = 1;
            data_size -= (TU32)(p_idr - p);
            p = p_idr;
            if (mbSkipSEI) {
              skip_size = gfSkipSEIHEVC(p, 128);
              p += skip_size;
              data_size -= skip_size;
            }
            mpRemainingData = p;
            mnRemainingDataSize = data_size;
            mVideoWidth[output_index] = param.video_width;
            mVideoHeight[output_index] = param.video_height;
            mbRemaingData = 1;
            mRemainingDataOutputIndex = output_index;
            mbWaitFirstKeyframe[output_index] = 0;
            return EECode_OK;
          } else {
            LOG_FATAL("do not find vps, sps, pps?\n");
          }
        }
      } else if (mbWaitFirstKeyframe[output_index]) {
        LOGM_INFO("stream[%d] skip till key frame\n", output_index);
        return EECode_OK_NoOutputYet;
      } else {
        out_buffer->SetBufferFlags(0);
      }
      out_buffer->SetBufferType(EBufferType_VideoES);
      out_buffer->mpCustomizedContent = NULL;
      out_buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;
      out_buffer->mVideoWidth = mVideoWidth[output_index];
      out_buffer->mVideoHeight = mVideoHeight[output_index];
      out_buffer->mVideoBitrate = mBitrate[output_index];
      out_buffer->mVideoFrameRateNum = mFramerateNum[output_index];
      out_buffer->mVideoFrameRateDen = mFramerateDen[output_index];
      out_buffer->mVideoRoughFrameRate = mFramerate[output_index];
      if (is_first_slice) {
        out_buffer->SetBufferFlagBits(CIBuffer::DATA_SEGMENT_BEGIN_INDICATOR);
      }
      if (mbSkipSEI) {
        skip_size = gfSkipSEIHEVC(p, 128);
        p += skip_size;
        data_size -= skip_size;
      }
      out_buffer->SetDataPtr(p);
      out_buffer->SetDataSize(data_size);
      out_buffer->SetBufferPTS((TTime) param.pts);
      out_buffer->SetBufferNativePTS((TTime) param.pts);
      out_buffer->SetBufferLinearPTS((TTime) param.pts);
      current_remaining = 0;
      all_cached_frames = 0;
    } else if (StreamFormat_JPEG == mCodecFormat[output_index]) {
      p = (TU8 *) mMapBSB.base + param.offset;
      data_size = param.size;
      if (DUnlikely((EJPEG_MarkerPrefix != p[0]) || (EJPEG_SOI != p[1]) || (EJPEG_MarkerPrefix != p[2]) || (128 > data_size))) {
        LOGM_ERROR("not find mjpeg header, or invalid data size %d\n", data_size);
        gfPrintMemory(p, 8);
        return EECode_OK_NoOutputYet;
      }
      out_buffer->mbDataSegment = 0;
      mbWaitFirstKeyframe[output_index] = 0;
      out_buffer->SetBufferFlags(CIBuffer::KEY_FRAME);
      if (!mbAlreadySendSyncBuffer[output_index]) {
        mbAlreadySendSyncBuffer[output_index] = 1;
        out_buffer->SetBufferFlagBits(CIBuffer::SYNC_POINT);
        mVideoWidth[output_index] = param.video_width;
        mVideoHeight[output_index] = param.video_height;
      }
      out_buffer->mVideoWidth = param.video_width;
      out_buffer->mVideoHeight = param.video_height;
      out_buffer->mVideoBitrate = mBitrate[output_index];
      out_buffer->mVideoFrameRateNum = mFramerateNum[output_index];
      out_buffer->mVideoFrameRateDen = mFramerateDen[output_index];
      out_buffer->mVideoRoughFrameRate = mFramerate[output_index];
      out_buffer->SetBufferPTS((TTime) param.pts);
      out_buffer->SetBufferNativePTS((TTime) param.pts);
      out_buffer->SetBufferLinearPTS((TTime) param.pts);
      out_buffer->SetBufferType(EBufferType_VideoES);
      out_buffer->mpCustomizedContent = NULL;
      out_buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;
      out_buffer->SetDataPtr(p);
      out_buffer->SetDataSize((TMemSize) data_size);
    } else {
      LOGM_ERROR("not h264 or h265 or mjpeg bitstream\n");
      gfPrintMemory(p, 8);
      return EECode_OK_NoOutputYet;
    }
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
  inherited::Delete();
}

void CAmbaVideoEncoder::PrintStatus()
{
  LOGM_PRINTF("heart beat %d\n", mDebugHeartBeat);
  mDebugHeartBeat = 0;
}

#endif

