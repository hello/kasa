/**
 * am_amba_video_decoder.cpp
 *
 * History:
 *    2012/08/03 - [Zhi He] create file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
 */

#include "am_native.h"
#include "am_native_log.h"

#include "am_internal.h"

#include "am_osal.h"
#include "am_framework.h"
#include "am_modules_interface.h"
#include "am_codec_interface.h"

#include "am_amba_dsp.h"

#ifdef BUILD_MODULE_AMBA_DSP

#include "am_amba_video_decoder.h"

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
  , mbExitDecodeMode(1)
  , mSpecifiedTimeScale(DDefaultVideoFramerateNum)
  , mSpecifiedFrameTick(DDefaultVideoFramerateDen)
  , mpBitSreamBufferStart(NULL)
  , mpBitSreamBufferEnd(NULL)
  , mpBitStreamBufferCurPtr(NULL)
  , mpVideoExtraData(NULL)
  , mVideoExtraDataSize(0)
{
  mMaxGopSize = 0;
  mbStopCmdSent = 0;
  mbDiscardCurrentGOP = 0;
  mbGopBasedFeed = 0;
  mbAutoMapBSB = 0;
  mbSendDecodeReadyMsg = 0;
  memset(&mMapBSB, 0x0, sizeof(mMapBSB));
  memset(&mModeConfig, 0x0, sizeof(mModeConfig));
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
  //mConfigLogLevel = ELogLevel_Info;
  //hard code here
  mFrameRateNum = DDefaultVideoFramerateNum;
  mFrameRateDen = DDefaultVideoFramerateDen;
  mFrameRate = 30;
  mbEnableVoutDigital = mpPersistMediaConfig->dsp_config.use_digital_vout;
  mbEnableVoutHDMI = mpPersistMediaConfig->dsp_config.use_hdmi_vout;
  mbEnableVoutCVBS = mpPersistMediaConfig->dsp_config.use_cvbs_vout;
  gfSetupDSPAlContext(&mfDSPAL);
  mbDumpBitstream = mpPersistMediaConfig->dump_setting.dump_bitstream;
  mbDumpOnly = mpPersistMediaConfig->dump_setting.dump_only;
  mDumpIndex = 0;
  mpDumper = NULL;
  mbExitDecodeMode = mpPersistMediaConfig->b_exit_dsp_playback_mode;
  ((volatile SPersistMediaConfig *) mpPersistMediaConfig)->b_support_allframe_backward_playback = 1;
  mbSupportAllframeBackwardPlayback = mpPersistMediaConfig->b_support_allframe_backward_playback;
  LOGM_CONFIGURATION("b_exit_dsp_playback_mode %d, support ff/fw %d, support all frame backward playback %d\n",
                     mpPersistMediaConfig->b_exit_dsp_playback_mode,
                     mpPersistMediaConfig->dsp_config.b_support_ff_fb_bw,
                     mpPersistMediaConfig->b_support_allframe_backward_playback);
  return EECode_OK;
}

CAmbaVideoDecoder::~CAmbaVideoDecoder()
{
  if (mpVideoExtraData) {
    DDBG_FREE(mpVideoExtraData, "VDAE");
    mpVideoExtraData = NULL;
  }
  if (mpDumper) {
    gfDestroyFileDumper(mpDumper);
    mpDumper = NULL;
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
    mIavFd = mpPersistMediaConfig->dsp_config.device_fd;
    mMaxGopSize = param->max_gop_size;
    if ((mCapMaxCodedWidth * mCapMaxCodedHeight * mMaxGopSize) > (1920*1088*30)) {
      LOGM_WARN("disable all frame backward mode, with larger than 1920x1088x30 clips\n");
      mbSupportAllframeBackwardPlayback = 0;
    }
    DASSERT(mfDSPAL.f_query_decode_config);
    SAmbaDSPQueryDecodeConfig dec_config;
    mfDSPAL.f_query_decode_config(mIavFd, &dec_config);
    mbAutoMapBSB = dec_config.auto_map_bsb;
    err = enterDecodeMode();
    if (EECode_OK != err) {
      LOGM_FATAL("enter decode mode fail\n");
      return err;
    }
    if (StreamFormat_H264 == mCodecFormat) {
      gfFillAmbaH264GopHeader(mpAmbaGopHeader, 3003, 90000, 0, 0, 1);
      err = createDecoder((TU8)mDecId, EAMDSP_VIDEO_CODEC_TYPE_H264, mCapMaxCodedWidth, mCapMaxCodedHeight);
    } else if (StreamFormat_H265 == mCodecFormat) {
      gfFillAmbaH265GopHeader(mpAmbaGopHeader, 3003, 90000, 0, 0, 1);
      err = createDecoder((TU8)mDecId, EAMDSP_VIDEO_CODEC_TYPE_H265, mCapMaxCodedWidth, mCapMaxCodedHeight);
    } else {
      LOGM_FATAL("bad format 0x%08x\n", (TU32) mCodecFormat);
      return EECode_NotSupported;
    }
    if (EECode_OK != err) {
      LOGM_FATAL("create decoder fail\n");
      return err;
    }
    if (!mbDumpOnly) {
      DASSERT(mfDSPAL.f_speed);
      DASSERT(mfDSPAL.f_start);
      mfDSPAL.f_start(mIavFd, (TU8)mDecId);
      mfDSPAL.f_speed(mIavFd, (TU8)mDecId, 0x100, EAMDSP_PB_SCAN_MODE_ALL_FRAMES, EAMDSP_PB_DIRECTION_FW);
    }
    if (mbDumpBitstream) {
      if (mpDumper) {
        gfDestroyFileDumper(mpDumper);
        mpDumper = NULL;
      }
      TChar filename[128] = {0};
      snprintf(filename, 127, "amba_dec_%04d.h264", mDumpIndex);
      mDumpIndex ++;
      mpDumper = gfCreateFileDumper(filename);
    }
    mbBWplayback = 0;
  } else {
    LOGM_FATAL("NULL input in CAmbaVideoDecoder::SetupContext\n");
    return EECode_BadParam;
  }
  mbSendDecodeReadyMsg = 0;
  return EECode_OK;
}

EECode CAmbaVideoDecoder::DestroyContext()
{
  EECode err;
  if (mbDumpOnly) {
    return EECode_OK;
  }
  mfDSPAL.f_stop(mIavFd, (TU8)mDecId, 1);
  mbStopCmdSent = 1;
  err = destroyDecoder();
  if (EECode_OK != err) {
    LOGM_FATAL("destroy decoder fail\n");
  }
  if (mbExitDecodeMode) {
    err = leaveDecodeMode();
    if (EECode_OK != err) {
      LOGM_FATAL("leave decode mode fail\n");
    }
  }
  if (mMapBSB.base && mMapBSB.size) {
    TInt ret = mfDSPAL.f_unmap_bsb(mIavFd, &mMapBSB);
    if (0 > ret) {
      LOGM_FATAL("unmap bsb fail\n");
    }
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
  if (!mbStopCmdSent) {
    if (mfDSPAL.f_stop && (0 < mIavFd)) {
      mfDSPAL.f_stop(mIavFd, mDecId, 1);
      mbStopCmdSent = 1;
      mbSendDecodeReadyMsg = 0;
      LOGM_NOTICE("stop decoder done\n");
    }
  }
  return EECode_OK;
}

EECode CAmbaVideoDecoder::Decode(CIBuffer *in_buffer, CIBuffer *&out_buffer)
{
  return EECode_NotSupported;
}

EECode CAmbaVideoDecoder::Flush()
{
  if (mbDumpOnly) {
    return EECode_OK;
  }
  if (mfDSPAL.f_stop && mfDSPAL.f_start && mfDSPAL.f_speed && (0 < mIavFd)) {
    TInt ret = mfDSPAL.f_stop(mIavFd, mDecId, 1);
    DASSERT(!ret);
    LOGM_NOTICE("stop decoder done\n");
    mpBitStreamBufferCurPtr = mpBitSreamBufferStart;
    ret = mfDSPAL.f_start(mIavFd, mDecId);
    mbStopCmdSent = 0;
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
    DDBG_FREE(mpVideoExtraData, "VDAE");
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
    if (DecoderFeedingRule_AllFrames == feeding_rule) {
      if ((!direction) || (mbSupportAllframeBackwardPlayback)) {
        feeding_rule = EAMDSP_PB_SCAN_MODE_ALL_FRAMES;
      } else {
        LOGM_NOTICE("does not support all frame backward mode with this resolution and gop size (%dx%d, %d)\n", mCapMaxCodedWidth, mCapMaxCodedHeight, mMaxGopSize);
        feeding_rule = EAMDSP_PB_SCAN_MODE_I_ONLY;
      }
    } else if (DecoderFeedingRule_IDROnly == feeding_rule) {
      feeding_rule = EAMDSP_PB_SCAN_MODE_I_ONLY;
    } else {
      LOGM_FATAL("bad feeding_rule %d\n", feeding_rule);
      return EECode_BadParam;
    }
    mbBWplayback = direction;
    mFeedingRule = feeding_rule;
    LOGM_PRINTF("direction %d, speed %x, scan mode %d\n", direction, speed, feeding_rule);
    if (!mbDumpOnly) {
      ret = mfDSPAL.f_speed(mIavFd, (TU8)mDecId, speed, feeding_rule, direction);
      DASSERT(!ret);
      LOGM_NOTICE("set decoder speed\n");
    }
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
    case StreamFormat_H265:
      err = decodeH265(in_buffer);
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
  if (!mbDumpOnly) {
    DASSERT(mfDSPAL.f_query_print_decode_status);
    mfDSPAL.f_query_print_decode_status(mIavFd, (TU8)mDecId);
  }
}

TU8 *CAmbaVideoDecoder::copyDataToBSB(TU8 *ptr, TU8 *buffer, TUint size)
{
  if (mbDumpBitstream) {
    if (mpDumper) {
      gfFileDump(mpDumper, buffer, size);
    }
  }
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
  TU8 h264_end_of_sequence[] = {0x00, 0x00, 0x00, 0x01, ENalType_END_OF_SEQUENCE};
  TU8 h264_end_of_stream[] = {0x00, 0x00, 0x00, 0x01, ENalType_END_OF_STREAM};
  SAmbaDSPDecode dec;
  DASSERT(in_buffer);
  DASSERT(mfDSPAL.f_request_bsb);
  DASSERT(mfDSPAL.f_decode);
  size = in_buffer->GetDataSize();
  p_data = in_buffer->GetDataPtr();
  if (in_buffer->GetBufferFlags() & CIBuffer::KEY_FRAME) {
    TU8 *p_check = NULL;
    TU8 first_nal_type = 0;
    p_check = gfNALUFindFirstAVCSliceHeaderType(p_data, size, first_nal_type);
    if (!p_check) {
      mbDiscardCurrentGOP = 1;
      LOGM_WARN("not valid key frame: not find frame data, discard current GOP. data size %d, first bytes:\n", size);
      if (16 > size) {
        gfPrintMemory(p_data, size);
      } else {
        gfPrintMemory(p_data, 16);
      }
      return EECode_OK;
    } else if (ENalType_IDR != first_nal_type) {
      mbDiscardCurrentGOP = 1;
      LOGM_WARN("not valid key frame: not IDR nalu, discard current GOP, data size %d, first bytes:\n", size);
      if (16 > size) {
        gfPrintMemory(p_data, size);
      } else {
        gfPrintMemory(p_data, 16);
      }
      return EECode_OK;
    }
    mbDiscardCurrentGOP = 0;
    if (ENalType_SPS != (p_data[4] & 0x1f)) {
      b_append_extradata = 1;
    }
  }
  if (mpBitStreamBufferCurPtr == mpBitSreamBufferEnd) {
    mpBitStreamBufferCurPtr = mpBitSreamBufferStart;
  }
  if (!mbDumpOnly) {
    dec.decoder_id = (TU8) mDecId;
    dec.num_frames = 1;
    if (!mbAutoMapBSB) {
      dec.start_ptr_offset = (TU32) (TPointer) (mpBitStreamBufferCurPtr - mpBitSreamBufferStart);
    } else {
      dec.start_ptr_offset = (TU32) (TPointer) (mpBitStreamBufferCurPtr);
    }
    dec.first_frame_display = 0;
    ret = mfDSPAL.f_request_bsb(mIavFd, (TU8)mDecId, size + 1024, mpBitStreamBufferCurPtr);
    if (DDECODER_STOPPED == ret) {
      return EECode_OK_AlreadyStopped;
    } else if (DUnlikely(0 > ret)) {
      LOGM_ERROR("request bsb fail, return %d\n", ret);
      return EECode_Error;
    }
  }
  if (mbAddAmbaGopHeader && in_buffer->GetBufferFlags() & CIBuffer::KEY_FRAME) {
    gfUpdateAmbaH264GopHeader(mpAmbaGopHeader, (TU32) in_buffer->GetBufferNativePTS(), (TU8) in_buffer->mCurVideoGopSize);
    mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, mpAmbaGopHeader, DAMBA_H264_GOP_HEADER_LENGTH);
  }
  if (b_append_extradata) {
    mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, mpVideoExtraData, mVideoExtraDataSize);
  }
  mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, p_data, size);
  if (in_buffer->GetBufferFlags() & CIBuffer::LAST_FRAME_INDICATOR) {
    mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, h264_end_of_stream, sizeof(h264_end_of_stream));
    LOGM_NOTICE("feed end of stream pattern to dsp\n");
  } else if (mbBWplayback && (in_buffer->GetBufferFlags() & CIBuffer::END_OF_SEQUENCE_INDICATOR)) {
    mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, h264_end_of_sequence, sizeof(h264_end_of_sequence));
  }
  if (!mbDumpOnly) {
    if (!mbAutoMapBSB) {
      dec.end_ptr_offset = (TU32) (TPointer) (mpBitStreamBufferCurPtr - mpBitSreamBufferStart);
    } else {
      dec.end_ptr_offset = (TU32) (TPointer) (mpBitStreamBufferCurPtr);
    }
    dec.first_frame_display = in_buffer->GetBufferNativePTS();
    ret = mfDSPAL.f_decode(mIavFd, &dec);
    if (DDECODER_STOPPED == ret) {
      return EECode_OK_AlreadyStopped;
    } else if (DUnlikely(0 > ret)) {
      LOGM_ERROR("decode fail, return %d\n", ret);
      return EECode_Error;
    }
    if (!mbSendDecodeReadyMsg) {
      if (mpMsgSink) {
        SMSG msg;
        msg.code = EMSGType_NotifyDecoderReady;
        msg.owner_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_VideoDecoder, mIndex);
        msg.p_owner = (TULong) NULL;
        mpMsgSink->MsgProc(msg);
      }
      mbSendDecodeReadyMsg = 1;
    }
  }
  return EECode_OK;
}

EECode CAmbaVideoDecoder::decodeH265(CIBuffer *in_buffer)
{
  TU8 *p_data;
  TUint size;
  TUint b_append_extradata = 0;
  TInt ret = 0;
  TU8 h265_end_of_sequence[] = {0x00, 0x00, 0x00, 0x01, (EHEVCNalType_EOS << 1), 0x00};
  TU8 h265_end_of_stream[] = {0x00, 0x00, 0x00, 0x01, (EHEVCNalType_EOB << 1), 0x00};
  SAmbaDSPDecode dec;
  DASSERT(in_buffer);
  DASSERT(mfDSPAL.f_request_bsb);
  DASSERT(mfDSPAL.f_decode);
  size = in_buffer->GetDataSize();
  p_data = in_buffer->GetDataPtr();
  if (in_buffer->GetBufferFlags() & CIBuffer::KEY_FRAME) {
    TU8 first_nal_type = 0;
    TU8 is_first_slice = 0;
    TU8 *p_check = NULL;
    p_check = gfNALUFindFirstHEVCSliceHeaderType(p_data, size, first_nal_type, is_first_slice);
    if (!p_check) {
      mbDiscardCurrentGOP = 1;
      LOGM_WARN("not valid key frame: not find frame data, discard current GOP. data size %d, first bytes:\n", size);
      if (16 > size) {
        gfPrintMemory(p_data, size);
      } else {
        gfPrintMemory(p_data, 16);
      }
      return EECode_OK;
    } else if (((EHEVCNalType_IDR_W_RADL != first_nal_type) && (EHEVCNalType_IDR_N_LP != first_nal_type)) || !is_first_slice) {
      mbDiscardCurrentGOP = 1;
      LOGM_WARN("not valid key frame: not IDR nalu or not first slice, discard current GOP, data size %d, first bytes:\n", size);
      if (16 > size) {
        gfPrintMemory(p_data, size);
      } else {
        gfPrintMemory(p_data, 16);
      }
      return EECode_OK;
    } else if (mbDiscardCurrentGOP) {
      LOGM_WARN("discard frame before first valid key frame\n");
      return EECode_OK;
    }
    mbDiscardCurrentGOP = 0;
    p_check = gfNALUFindFirstHEVCVPSSPSPPSAndSliceNalType(p_data, size, first_nal_type);
    DASSERT(p_check);
    if (EHEVCNalType_VPS != first_nal_type) {
      b_append_extradata = 1;
    }
  }
  if (mpBitStreamBufferCurPtr == mpBitSreamBufferEnd) {
    mpBitStreamBufferCurPtr = mpBitSreamBufferStart;
  }
  if (!mbDumpOnly) {
    dec.decoder_id = (TU8) mDecId;
    dec.num_frames = 1;
    dec.start_ptr_offset = (TU32)(mpBitStreamBufferCurPtr - mpBitSreamBufferStart);
    dec.first_frame_display = 0;
    ret = mfDSPAL.f_request_bsb(mIavFd, (TU8)mDecId, size + 1024, mpBitStreamBufferCurPtr);
    if (DDECODER_STOPPED == ret) {
      return EECode_OK_AlreadyStopped;
    } else if (DUnlikely(0 > ret)) {
      LOGM_ERROR("request bsb fail, return %d\n", ret);
      return EECode_Error;
    }
  }
  if (mbAddAmbaGopHeader && in_buffer->GetBufferFlags() & CIBuffer::KEY_FRAME) {
    gfUpdateAmbaH265GopHeader(mpAmbaGopHeader, (TU32) in_buffer->GetBufferNativePTS(), (TU8) in_buffer->mCurVideoGopSize);
    mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, mpAmbaGopHeader, DAMBA_H265_GOP_HEADER_LENGTH);
  }
  if (b_append_extradata) {
    mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, mpVideoExtraData, mVideoExtraDataSize);
  }
  mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, p_data, size);
  if (in_buffer->GetBufferFlags() & CIBuffer::LAST_FRAME_INDICATOR) {
    mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, h265_end_of_stream, sizeof(h265_end_of_stream));
    LOGM_NOTICE("feed end of stream pattern to dsp\n");
  } else if (in_buffer->GetBufferFlags() & CIBuffer::END_OF_SEQUENCE_INDICATOR) {
    mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, h265_end_of_sequence, sizeof(h265_end_of_sequence));
  }
  if (!mbDumpOnly) {
    dec.end_ptr_offset = (TU32)(mpBitStreamBufferCurPtr - mpBitSreamBufferStart);
    dec.first_frame_display = in_buffer->GetBufferNativePTS();
    ret = mfDSPAL.f_decode(mIavFd, &dec);
    if (DDECODER_STOPPED == ret) {
      return EECode_OK_AlreadyStopped;
    } else if (DUnlikely(0 > ret)) {
      LOGM_ERROR("decode fail, return %d\n", ret);
      return EECode_Error;
    }
  }
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
  if (!mbAutoMapBSB) {
    mMapBSB.b_two_times = 0;
    mMapBSB.b_enable_write = 1;
    mMapBSB.b_enable_read = 0;
    ret = mfDSPAL.f_map_bsb(mIavFd, &mMapBSB);
    if (0 > ret) {
      LOGM_FATAL("map bsb fail\n");
      return EECode_Error;
    }
  }
  ret = mfDSPAL.f_get_vout_info(mIavFd, 0, EAMDSP_VOUT_TYPE_DIGITAL, &mVoutInfos[0]);
  if ((0 > ret) || (!mVoutInfos[0].width) || (!mVoutInfos[0].height)) {
    LOGM_CONFIGURATION("digital vout not enabled\n");
    mbEnableVoutDigital = 0;
  } else {
    has_digital = 1;
    vout_num ++;
  }
  ret = mfDSPAL.f_get_vout_info(mIavFd, 1, EAMDSP_VOUT_TYPE_HDMI, &mVoutInfos[1]);
  if ((0 > ret) || (!mVoutInfos[1].width) || (!mVoutInfos[1].height)) {
    LOGM_CONFIGURATION("hdmi vout not enabled\n");
    mbEnableVoutHDMI = 0;
  } else {
    has_hdmi = 1;
    vout_num ++;
  }
  ret = mfDSPAL.f_get_vout_info(mIavFd, 1, EAMDSP_VOUT_TYPE_CVBS, &mVoutInfos[2]);
  if ((0 > ret) || (!mVoutInfos[2].width) || (!mVoutInfos[2].height)) {
    LOGM_CONFIGURATION("cvbs vout not enabled\n");
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
    if (has_hdmi) {
      mbEnableVoutHDMI = 1;
    } else if (has_digital) {
      mbEnableVoutDigital = 1;
    } else if (has_cvbs) {
      mbEnableVoutCVBS = 1;
    }
    LOGM_WARN("usr do not specify vout, guess default: cvbs %d, digital %d, hdmi %d\n", mbEnableVoutCVBS, mbEnableVoutDigital, mbEnableVoutHDMI);
  } else {
    if (mbEnableVoutHDMI) {
      mbEnableVoutDigital = 0;
      mbEnableVoutCVBS = 0;
      mModeConfig.vout_mask = 0x02;
    } else if (mbEnableVoutDigital) {
      mbEnableVoutHDMI = 0;
      mbEnableVoutCVBS = 0;
      mModeConfig.vout_mask = 0x01;
    } else if (mbEnableVoutCVBS) {
      mbEnableVoutHDMI = 0;
      mbEnableVoutDigital = 0;
      mModeConfig.vout_mask = 0x02;
    }
  }
  mVoutNumber = 1;
  mModeConfig.num_decoder = 1;
  mModeConfig.max_frm_width = mCapMaxCodedWidth;
  mModeConfig.max_frm_height = mCapMaxCodedHeight;
  mModeConfig.b_support_ff_fb_bw = mpPersistMediaConfig->dsp_config.b_support_ff_fb_bw;
  if (mbSupportAllframeBackwardPlayback) {
    mModeConfig.max_gop_size = mMaxGopSize;
  } else {
    mModeConfig.max_gop_size = 0;
  }
  if (!mbDumpOnly) {
    LOGM_NOTICE("enter decode mode...\n");
    ret = mfDSPAL.f_enter_mode(mIavFd, &mModeConfig);
    if (0 > ret) {
      LOGM_ERROR("enter decode mode fail, ret %d\n", ret);
      return EECode_Error;
    }
    LOGM_NOTICE("enter decode mode done\n");
  }
  return EECode_OK;
}

EECode CAmbaVideoDecoder::leaveDecodeMode()
{
  TInt ret = 0;
  if (!mbDumpOnly) {
    DASSERT(mfDSPAL.f_leave_mode);
    DASSERT(0 < mIavFd);
    LOGM_NOTICE("leave decode mode...\n");
    ret = mfDSPAL.f_leave_mode(mIavFd);
    if (0 > ret) {
      LOGM_ERROR("leave decode mode fail, ret %d\n", ret);
      return EECode_Error;
    }
    LOGM_NOTICE("leave decode mode done\n");
  }
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
  if (!mbDumpOnly) {
    ret = mfDSPAL.f_create_decoder(mIavFd, &mDecoderInfo);
    if (0 > ret) {
      LOGM_ERROR("create decoder fail, ret %d\n", ret);
      return EECode_Error;
    }
  }
  if (!mbAutoMapBSB) {
    if (!mDecoderInfo.b_use_addr) {
      mpBitStreamBufferCurPtr = mpBitSreamBufferStart = (TU8 *)mMapBSB.base + mDecoderInfo.bsb_start_offset;
      mpBitSreamBufferEnd = mpBitSreamBufferStart + mMapBSB.size;
    } else {
      LOGM_FATAL("should not here\n");
    }
  } else {
    if (mDecoderInfo.b_use_addr) {
      mpBitStreamBufferCurPtr = mpBitSreamBufferStart = (TU8 *) (TULong) mDecoderInfo.bsb_start_offset;
      mpBitSreamBufferEnd = mpBitSreamBufferStart + mDecoderInfo.bsb_size;
    } else {
      LOGM_FATAL("should not here\n");
    }
  }
  return EECode_OK;
}

EECode CAmbaVideoDecoder::destroyDecoder()
{
  TInt ret = 0;
  if (!mbDumpOnly) {
    DASSERT(mfDSPAL.f_destroy_decoder);
    DASSERT(0 < mIavFd);
    LOGM_NOTICE("destroy decoder...\n");
    ret = mfDSPAL.f_destroy_decoder(mIavFd, mDecId);
    if (0 > ret) {
      LOGM_ERROR("destroy decoder fail, ret %d\n", ret);
      return EECode_Error;
    }
    LOGM_NOTICE("destroy decoder done\n");
  }
  return EECode_OK;
}

#endif

