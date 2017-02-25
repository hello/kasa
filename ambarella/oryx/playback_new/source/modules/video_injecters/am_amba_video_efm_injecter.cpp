/*******************************************************************************
 * am_amba_video_efm_injecter.cpp
 *
 * History:
 *    2015/12/25 - [Zhi He] create file
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
#include "am_internal_asm.h"

#include "am_osal.h"
#include "am_framework.h"
#include "am_modules_interface.h"

#include "am_amba_dsp.h"

#ifdef BUILD_MODULE_AMBA_DSP

#include "am_amba_video_efm_injecter.h"

static void __yuv420p_to_nv12_interlave_uv(TU8 *p_uv_dst, TU8 *p_u_src, TU8 *p_v_src, TU32 width, TU32 height, TU32 dst_pitch, TU32 src_pitch_uv)
{
  TU32 i = 0, j = 0;
  DASSERT(dst_pitch >= width);
  dst_pitch -= width;
  width = width >> 1;
  height = height >> 1;
  DASSERT(src_pitch_uv >= width);
  src_pitch_uv -= width;
  for (j = 0; j < height; j ++) {
    for (i = 0; i < width; i ++) {
      p_uv_dst[0] = p_u_src[0];
      p_uv_dst[1] = p_v_src[0];
      p_uv_dst += 2;
      p_u_src ++;
      p_v_src ++;
    }
    p_uv_dst += dst_pitch;
    p_u_src += src_pitch_uv;
    p_v_src += src_pitch_uv;
  }
}

#ifndef BUILD_ASM_NEON
static void __yuyv_to_nv12(TU8 *p_y_dst, TU8 *p_uv_dst, TU8 *p_src, TU32 width, TU32 height, TU32 dst_y_pitch, TU32 dst_uv_pitch, TU32 src_pitch)
{
  TU32 i = 0, j = 0;
  TU8 *p_src_1 = NULL;
  TU8 *p_y_dst_1 = NULL;
  TU32 value = 0;
  DASSERT(dst_y_pitch >= width);
  p_y_dst_1 = p_y_dst + dst_y_pitch;
  dst_y_pitch = dst_y_pitch << 1;
  dst_y_pitch -= width;
  DASSERT(dst_uv_pitch >= width);
  dst_uv_pitch -= width;
  DASSERT(src_pitch >= (width << 1));
  p_src_1 = p_src + src_pitch;
  src_pitch = src_pitch << 1;
  src_pitch -= (width << 1);
  width = width >> 1;
  height = height >> 1;
  for (j = 0; j < height; j ++) {
    for (i = 0; i < width; i ++) {
      p_y_dst[0] = p_src[0];
      p_y_dst[1] = p_src[2];
      p_y_dst_1[0] = p_src_1[0];
      p_y_dst_1[1] = p_src_1[2];
      p_y_dst += 2;
      p_y_dst_1 += 2;
      value = (p_src[1] + p_src_1[1]) / 2;
      p_uv_dst[0] = value;
      value = (p_src[3] + p_src_1[3]) / 2;
      p_uv_dst[1] = value;
      p_uv_dst += 2;
      p_src += 4;
      p_src_1 += 4;
    }
    p_uv_dst += dst_uv_pitch;
    p_y_dst += dst_y_pitch;
    p_y_dst_1 += dst_y_pitch;
    p_src += src_pitch;
    p_src_1 += src_pitch;
  }
}
#endif

static void __copy_nv12(TU8 *p_y_dst, TU8 *p_uv_dst, TU8 *p_y_src, TU8 *p_uv_src, TU32 width, TU32 height, TU32 dst_y_pitch, TU32 dst_uv_pitch, TU32 src_y_pitch, TU32 src_uv_pitch)
{
  if (dst_y_pitch == src_y_pitch) {
    DASSERT(dst_y_pitch >= width);
    memcpy(p_y_dst, p_y_src, dst_y_pitch * height);
  } else {
    TU32 j = 0;
    for (j = 0; j < height; j ++) {
      memcpy(p_y_dst, p_y_src, width);
      p_y_dst += dst_y_pitch;
      p_y_src += src_y_pitch;
    }
  }
  if (dst_uv_pitch == src_uv_pitch) {
    DASSERT(dst_uv_pitch >= width);
    memcpy(p_uv_dst, p_uv_src, (dst_uv_pitch * height) / 2);
  } else {
    TU32 j = 0;
    height = height >> 1;
    for (j = 0; j < height; j ++) {
      memcpy(p_uv_dst, p_uv_src, width);
      p_uv_dst += dst_uv_pitch;
      p_uv_src += src_uv_pitch;
    }
  }
}

#if 0
static void __generate_me1(TU8 *p_me1, TU8 *p_y, TU32 width, TU32 height, TU32 me1_pitch)
{
  TU32 i = 0, j = 0;
  TU32 v = 0;
  TU8 *p_y1 = p_y + width;
  TU8 *p_y2 = p_y1 + width;
  TU8 *p_y3 = p_y2 + width;
  width = width >> 2;
  height = height >> 2;
  DASSERT(me1_pitch >= width);
  for (j = 0; j < height; j ++) {
    for (i = 0; i < width; i ++) {
      v = p_y[0] + p_y[1] + p_y[2] + p_y[3] + 8;
      v += p_y1[0] + p_y1[1] + p_y1[2] + p_y1[3];
      v += p_y2[0] + p_y2[1] + p_y2[2] + p_y2[3];
      v += p_y3[0] + p_y3[1] + p_y3[2] + p_y3[3];
      p_me1[i] = v >> 4;
      p_y += 4;
      p_y1 += 4;
      p_y2 += 4;
      p_y3 += 4;
    }
    p_y += width * 3;
    p_y1 += width * 3;
    p_y2 += width * 3;
    p_y3 += width * 3;
    p_me1 += me1_pitch;
  }
}
#endif

#ifndef BUILD_ASM_NEON
static void __generate_me1_pitch(TU8 *p_me1, TU8 *p_y, TU32 width, TU32 height, TU32 me1_pitch, TU32 y_pitch)
{
  TU32 i = 0, j = 0;
  TU32 v = 0;
  TU8 *p_y1 = p_y + width;
  TU8 *p_y2 = p_y1 + width;
  TU8 *p_y3 = p_y2 + width;
  DASSERT(y_pitch >= width);
  y_pitch = (y_pitch * 4) - width;
  width = width >> 2;
  height = height >> 2;
  DASSERT(me1_pitch >= width);
  for (j = 0; j < height; j ++) {
    for (i = 0; i < width; i ++) {
      v = p_y[0] + p_y[1] + p_y[2] + p_y[3] + 8;
      v += p_y1[0] + p_y1[1] + p_y1[2] + p_y1[3];
      v += p_y2[0] + p_y2[1] + p_y2[2] + p_y2[3];
      v += p_y3[0] + p_y3[1] + p_y3[2] + p_y3[3];
      p_me1[i] = v >> 4;
      p_y += 4;
      p_y1 += 4;
      p_y2 += 4;
      p_y3 += 4;
    }
    p_y += y_pitch;
    p_y1 += y_pitch;
    p_y2 += y_pitch;
    p_y3 += y_pitch;
    p_me1 += me1_pitch;
  }
}
#endif

IVideoInjecter *gfCreateVideoAmbaEFMInjecter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
  return CAmbaVideoEFMInjecter::Create(pName, pPersistMediaConfig, pEngineMsgSink);
}

//-----------------------------------------------------------------------
//
// CAmbaVideoEFMInjecter
//
//-----------------------------------------------------------------------
IVideoInjecter *CAmbaVideoEFMInjecter::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
  CAmbaVideoEFMInjecter *result = new CAmbaVideoEFMInjecter(pName, pPersistMediaConfig, pEngineMsgSink);
  if (result && result->Construct() != EECode_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

void CAmbaVideoEFMInjecter::Destroy()
{
  Delete();
}

CAmbaVideoEFMInjecter::CAmbaVideoEFMInjecter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
  : inherited(pName, 0)
  , mpPersistMediaConfig(pPersistMediaConfig)
  , mpMsgSink(pEngineMsgSink)
  , mIavFd(-1)
  , mbSetup(0)
  , mStreamIndex(0)
{
  memset(&mfDSPAL, 0x0, sizeof(mfDSPAL));
  memset(&mMapDSP, 0x0, sizeof(mMapDSP));
  memset(&mPoolInfo, 0x0, sizeof(mPoolInfo));
}

EECode CAmbaVideoEFMInjecter::Construct()
{
  DSET_MODULE_LOG_CONFIG(ELogModuleVideoInput);
  gfSetupDSPAlContext(&mfDSPAL);
  return EECode_OK;
}

CAmbaVideoEFMInjecter::~CAmbaVideoEFMInjecter()
{
}

EECode CAmbaVideoEFMInjecter::SetupContext(SVideoInjectParams *param)
{
  TInt ret = 0;
  DASSERT(mpPersistMediaConfig);
  DASSERT(mfDSPAL.f_map_dsp);
  DASSERT(mfDSPAL.f_query_encode_stream_info);
  DASSERT(mfDSPAL.f_query_encode_stream_fmt);
  DASSERT(mfDSPAL.f_encode_start);
  DASSERT(mfDSPAL.f_efm_get_buffer_pool_info);
  DASSERT(mfDSPAL.f_efm_request_frame);
  DASSERT(mfDSPAL.f_efm_finish_frame);
  {
    AUTO_LOCK(mpPersistMediaConfig->p_global_mutex);
    volatile SPersistMediaConfig *p_config = (volatile SPersistMediaConfig *) mpPersistMediaConfig;
    mIavFd = p_config->dsp_config.device_fd;
    if (DUnlikely(0 >= mIavFd)) {
      LOGM_FATAL("iav fd (%d) is not valid\n", mIavFd);
      return EECode_InternalLogicalBug;
    }
    if (!p_config->shared_count_dsp_mmap) {
      ret = mfDSPAL.f_map_dsp(mIavFd, &mMapDSP);
      if (ret) {
        LOGM_ERROR("map fail\n");
        return EECode_OSError;
      }
      LOGM_CONFIGURATION("map dsp mem\n");
      p_config->shared_dsp_mem_base = mMapDSP.base;
      p_config->shared_dsp_mem_size = mMapDSP.size;
    } else {
      mMapDSP.base = p_config->shared_dsp_mem_base;
      mMapDSP.size = p_config->shared_dsp_mem_size;
    }
    p_config->shared_count_dsp_mmap ++;
  }
  ret = mfDSPAL.f_efm_get_buffer_pool_info(mIavFd, &mPoolInfo);
  if (ret) {
    LOGM_ERROR("get buffer pool info fail\n");
    return EECode_OSError;
  }
  LOGM_CONFIGURATION("efm: yuv buffer number %d, me1 number %d\n", mPoolInfo.yuv_buf_num, mPoolInfo.me1_buf_num);
  LOGM_CONFIGURATION("efm: yuv buffer %dx%d, pitch %d\n", mPoolInfo.yuv_buf_width, mPoolInfo.yuv_buf_height, mPoolInfo.yuv_pitch);
  LOGM_CONFIGURATION("efm: me1 buffer %dx%d, pitch %d\n", mPoolInfo.me1_buf_width, mPoolInfo.me1_buf_height, mPoolInfo.me1_pitch);
  mbSetup= 1;
  return EECode_OK;
}

void CAmbaVideoEFMInjecter::DestroyContext()
{
  {
    AUTO_LOCK(mpPersistMediaConfig->p_global_mutex);
    volatile SPersistMediaConfig *p_config = (volatile SPersistMediaConfig *) mpPersistMediaConfig;
    if (p_config->shared_count_dsp_mmap) {
      p_config->shared_count_dsp_mmap --;
      if (!p_config->shared_count_dsp_mmap) {
        TInt ret = mfDSPAL.f_unmap_dsp(mIavFd, &mMapDSP);
        if (ret) {
          LOGM_ERROR("unmap fail\n");
        } else {
          LOGM_CONFIGURATION("unmap dsp mem\n");
          p_config->shared_dsp_mem_base = NULL;
          p_config->shared_dsp_mem_size = 0;
        }
      }
    }
  }
}

EECode CAmbaVideoEFMInjecter::Prepare(TU8 stream_index)
{
  TInt ret = 0;
  DASSERT(mbSetup);
  DASSERT(mfDSPAL.f_query_encode_stream_fmt);
  DASSERT(mfDSPAL.f_query_encode_stream_info);
  SAmbaDSPEncStreamInfo info;
  SAmbaDSPEncStreamFormat fmt;
  fmt.id = stream_index;
  mfDSPAL.f_query_encode_stream_fmt(mIavFd, &fmt);
  if (0 > ret) {
    LOGM_FATAL("query_encode_stream_fmt fail\n");
    return EECode_OSError;
  }
  if (EAMDSP_ENC_STREAM_SOURCE_BUFFER_EFM != fmt.source_buffer) {
    LOGM_FATAL("stream %d's source buffer type is not EFM\n", stream_index);
    return EECode_BadParam;
  }
  info.id = stream_index;
  mfDSPAL.f_query_encode_stream_info(mIavFd, &info);
  if (0 > ret) {
    LOGM_FATAL("f_query_encode_stream_info fail\n");
    return EECode_OSError;
  }
  if ((EAMDSP_ENC_STREAM_STATE_ENCODING == info.state) || (EAMDSP_ENC_STREAM_STATE_STARTING == info.state)) {
    LOGM_CONFIGURATION("stream %d (EFM) already started\n", stream_index);
  } else if (EAMDSP_ENC_STREAM_STATE_IDLE == info.state) {
    LOGM_CONFIGURATION("stream %d (EFM) not started, start now\n", stream_index);
    TU32 mask = 1 << stream_index;
    ret = mfDSPAL.f_encode_start(mIavFd, mask);
    if (0 > ret) {
      LOGM_ERROR("start encode fail\n");
      return EECode_OSError;
    }
  } else {
    LOGM_ERROR("not expected encode stream (%d) state %d\n", stream_index, info.state);
    return EECode_BadState;
  }
  mStreamIndex = stream_index;
  return EECode_OK;
}

EECode CAmbaVideoEFMInjecter::Inject(CIBuffer *buffer)
{
  if (DUnlikely(!buffer)) {
    LOG_FATAL("NULL buffer\n");
    return EECode_BadParam;
  }
  if (DLikely(mbSetup)) {
    DASSERT(mfDSPAL.f_efm_request_frame);
    DASSERT(mfDSPAL.f_efm_finish_frame);
    TInt ret = 0;
    SAmbaEFMFrame frame;
    SAmbaEFMFinishFrame finish_frame;
    memset(&frame, 0x0, sizeof(frame));
    LOGM_INFO("request frame ...\n");
    ret = mfDSPAL.f_efm_request_frame(mIavFd, &frame);
    if (ret) {
      LOGM_ERROR("f_efm_request_frame fail\n");
      return EECode_OSError;
    }
    if ((StreamFormat_PixelFormat_YUV420p == buffer->mContentFormat) || (StreamFormat_PixelFormat_YUV422p == buffer->mContentFormat)) {
      feedFrameYUV420pYUV422p(buffer, &frame);
    } else if (StreamFormat_PixelFormat_YUYV == buffer->mContentFormat) {
      feedFrameYUYV(buffer, &frame);
    } else if (StreamFormat_PixelFormat_NV12 == buffer->mContentFormat) {
      feedFrameNV12(buffer, &frame);
    } else {
      LOGM_ERROR("bad format 0x%08x\n", buffer->mContentFormat);
      return EECode_BadFormat;
    }
    memset(&finish_frame, 0x0, sizeof(finish_frame));
    finish_frame.stream_id = mStreamIndex;
    finish_frame.frame_idx = frame.frame_idx;
    finish_frame.frame_pts = buffer->GetBufferNativePTS();
    finish_frame.b_not_wait_next_interrupt = 0;
    finish_frame.use_hw_pts = 1;
    if (CIBuffer::LAST_FRAME_INDICATOR & buffer->GetBufferFlags()) {
      finish_frame.is_last_frame = 1;
      LOGM_CONFIGURATION("feed last frame\n");
    }
    finish_frame.buffer_width = mPoolInfo.yuv_buf_width;
    finish_frame.buffer_height = mPoolInfo.yuv_buf_height;
    LOGM_INFO("finish frame ...\n");
    ret = mfDSPAL.f_efm_finish_frame(mIavFd, &finish_frame);
    if (ret) {
      LOGM_ERROR("f_efm_finish_frame fail\n");
      return EECode_OSError;
    }
    LOGM_INFO("finish frame end\n");
  } else {
    LOGM_ERROR("not setup yet\n");
    return EECode_BadState;
  }
  return EECode_OK;
}

EECode CAmbaVideoEFMInjecter::InjectVoid(TU32 last_flag)
{
  if (DLikely(mbSetup)) {
    DASSERT(mfDSPAL.f_efm_request_frame);
    DASSERT(mfDSPAL.f_efm_finish_frame);
    TInt ret = 0;
    SAmbaEFMFrame frame;
    SAmbaEFMFinishFrame finish_frame;
    memset(&frame, 0x0, sizeof(frame));
    LOGM_INFO("request frame ...\n");
    ret = mfDSPAL.f_efm_request_frame(mIavFd, &frame);
    if (ret) {
      LOGM_ERROR("f_efm_request_frame fail\n");
      return EECode_OSError;
    }
    fillBlackFrameNV12(&frame);
    memset(&finish_frame, 0x0, sizeof(finish_frame));
    finish_frame.stream_id = mStreamIndex;
    finish_frame.frame_idx = frame.frame_idx;
    finish_frame.b_not_wait_next_interrupt = 0;
    finish_frame.use_hw_pts = 1;
    if (last_flag) {
      finish_frame.is_last_frame = 1;
      LOGM_CONFIGURATION("feed last frame\n");
    }
    finish_frame.buffer_width = mPoolInfo.yuv_buf_width;
    finish_frame.buffer_height = mPoolInfo.yuv_buf_height;
    LOGM_INFO("finish frame ...\n");
    ret = mfDSPAL.f_efm_finish_frame(mIavFd, &finish_frame);
    if (ret) {
      LOGM_ERROR("f_efm_finish_frame fail\n");
      return EECode_OSError;
    }
    LOGM_INFO("finish frame end\n");
  } else {
    LOGM_ERROR("not setup yet\n");
    return EECode_BadState;
  }
  return EECode_OK;
}

EECode CAmbaVideoEFMInjecter::BatchInject(TU8 src_buf, TU8 num, TU8 last_flag,
  void *callback_context, TFInjecterCallback callback)
{
  if (DLikely(mbSetup)) {
    DASSERT(mfDSPAL.f_query_yuv_buffer);
    DASSERT(mfDSPAL.f_efm_request_frame);
    DASSERT(mfDSPAL.f_efm_finish_frame);
    DASSERT(mfDSPAL.f_gdma_copy);
    SAmbaGDMACopy copy;
    SAmbaEFMFrame frame;
    SAmbaDSPQueryYUVBuffer yuv_buffer;
    SAmbaEFMFinishFrame finish_frame;
    TInt ret = 0;
    TU8 i = 0;
    if (3 < num) {
      LOGM_FATAL("num %d should not larger than 3\n", num);
      return EECode_BadParam;
    }
    for (i = 0; i < num; i ++) {
      memset(&frame, 0x0, sizeof(frame));
      ret = mfDSPAL.f_efm_request_frame(mIavFd, &frame);
      if (ret) {
        LOGM_ERROR("f_efm_request_frame fail\n");
        return EECode_OSError;
      }
      yuv_buffer.buf_id = src_buf;
      ret = mfDSPAL.f_query_yuv_buffer(mIavFd, &yuv_buffer);
      if (ret) {
        LOGM_ERROR("query yuv buffer fail, ret %d\n", ret);
        return EECode_OSError;
      }
      if ((yuv_buffer.width != mPoolInfo.yuv_buf_width) || (yuv_buffer.height!= mPoolInfo.yuv_buf_height)) {
        LOGM_ERROR("src buffer size not match efm stream size\n");
        return EECode_BadParam;
      }
      if (callback_context && callback) {
        callback(callback_context, yuv_buffer.mono_pts, yuv_buffer.dsp_pts);
      }
      copy.src_offset = yuv_buffer.y_addr_offset;
      copy.dst_offset = frame.yuv_luma_offset;
      copy.src_pitch = yuv_buffer.pitch;
      copy.dst_pitch = mPoolInfo.yuv_pitch;
      copy.width = yuv_buffer.width;
      copy.height = yuv_buffer.height;
      copy.src_mmap_type = EAmbaBufferType_DSP;
      copy.dst_mmap_type = EAmbaBufferType_DSP;
      ret = mfDSPAL.f_gdma_copy(mIavFd, &copy);
      if (ret) {
        LOGM_ERROR("gdma copy fail, ret %d\n", ret);
        return EECode_OSError;
      }
      copy.src_offset = yuv_buffer.uv_addr_offset;
      copy.dst_offset = frame.yuv_chroma_offset;
      copy.src_pitch = yuv_buffer.pitch;
      copy.dst_pitch = mPoolInfo.yuv_pitch;
      copy.width = yuv_buffer.width;
      copy.height = yuv_buffer.height / 2;
      copy.src_mmap_type = EAmbaBufferType_DSP;
      copy.dst_mmap_type = EAmbaBufferType_DSP;
      ret = mfDSPAL.f_gdma_copy(mIavFd, &copy);
      if (ret) {
        LOGM_ERROR("gdma copy fail, ret %d\n", ret);
        return EECode_OSError;
      }
#ifdef BUILD_ASM_NEON
      SASMArguGenerateMe1Pitch param;
      param.dst = ((unsigned char *) mMapDSP.base) + frame.me1_offset;
      param.src = ((unsigned char *) mMapDSP.base) + frame.yuv_luma_offset;
      param.width = mPoolInfo.yuv_buf_width;
      param.height = mPoolInfo.yuv_buf_height;
      param.dst_pitch = mPoolInfo.me1_pitch;
      param.src_pitch = mPoolInfo.yuv_pitch;
      asm_neon_generate_me1_pitch(&param);
#else
      __generate_me1_pitch(((TU8 *) mMapDSP.base) + frame.me1_offset,
                           ((TU8 *) mMapDSP.base) + frame.yuv_luma_offset,
                           mPoolInfo.yuv_buf_width, mPoolInfo.yuv_buf_height,
                           mPoolInfo.me1_pitch, mPoolInfo.yuv_pitch);
#endif
      memset(&finish_frame, 0x0, sizeof(finish_frame));
      finish_frame.stream_id = mStreamIndex;
      finish_frame.frame_idx = frame.frame_idx;
      finish_frame.frame_pts = yuv_buffer.mono_pts;
      finish_frame.b_not_wait_next_interrupt = 0;
      finish_frame.use_hw_pts = 1;
      finish_frame.buffer_width = mPoolInfo.yuv_buf_width;
      finish_frame.buffer_height = mPoolInfo.yuv_buf_height;
      ret = mfDSPAL.f_efm_finish_frame(mIavFd, &finish_frame);
      if (ret) {
        LOGM_ERROR("f_efm_finish_frame fail\n");
        return EECode_OSError;
      }
      LOGM_INFO("finish frame end\n");
    }
  } else {
    LOGM_ERROR("not setup yet\n");
    return EECode_BadState;
  }
  return EECode_OK;
}

void CAmbaVideoEFMInjecter::feedFrameYUV420pYUV422p(CIBuffer *buffer, SAmbaEFMFrame *frame)
{
  TU32 /*i = 0, */j = 0;
  TU32 width = buffer->mVideoWidth;
  TU32 height = buffer->mVideoHeight;
  TU8 *p_y_src = NULL;
  TU8 *p_u_src = NULL;
  TU8 *p_v_src = NULL;
  TU8 *p_y_dst = NULL;
  TU8 *p_uv_dst = NULL;
  TU32 src_pitch_y = 0;
  TU32 src_pitch_uv = 0;
  TU32 dst_pitch = 0;
  TU8 *p_me1 = NULL;
  TU32 me1_pitch = 0;
  TU32 uv_stride = 0;
  if (StreamFormat_PixelFormat_YUV420p == buffer->mContentFormat) {
    uv_stride = buffer->GetDataLineSize(1);
  } else if (StreamFormat_PixelFormat_YUV422p == buffer->mContentFormat) {
    //gfVerticalDownResampleUVU8By2(buffer->GetDataPtr(1), buffer->GetDataPtr(2), buffer->GetDataLineSize(1), width >> 1, height >> 1);
    uv_stride = buffer->GetDataLineSize(1) * 2;
  } else {
    LOGM_FATAL("bad format %d\n", buffer->mContentFormat);
    return;
  }
  if ((width == mPoolInfo.yuv_buf_width) && (height == mPoolInfo.yuv_buf_height)) {
    p_y_src = buffer->GetDataPtr(0);
    p_u_src = buffer->GetDataPtr(1);
    p_v_src = buffer->GetDataPtr(2);
    src_pitch_y = buffer->GetDataLineSize(0);
    src_pitch_uv = uv_stride;
    if (!mPoolInfo.b_use_addr) {
      p_y_dst = (TU8 *) mMapDSP.base + frame->yuv_luma_offset;
      p_uv_dst = (TU8 *) mMapDSP.base + frame->yuv_chroma_offset;
    } else {
      p_y_dst = (TU8 *) frame->yuv_luma_offset;
      p_uv_dst = (TU8 *) frame->yuv_chroma_offset;
    }
    dst_pitch = mPoolInfo.yuv_pitch;
    //LOGM_CONFIGURATION("src_pitch_y %d, src_pitch_uv %d, dst_pitch %d, width %d, height %d, mPoolInfo.me1_pitch %d\n", src_pitch_y, src_pitch_uv, dst_pitch, width, height, mPoolInfo.me1_pitch);
    if (dst_pitch == src_pitch_y) {
      memcpy(p_y_dst, p_y_src, height * dst_pitch);
    } else {
      for (j = 0; j < height; j ++) {
        memcpy(p_y_dst, p_y_src, width);
        p_y_src += src_pitch_y;
        p_y_dst += dst_pitch;
      }
    }
    __yuv420p_to_nv12_interlave_uv(p_uv_dst, p_u_src, p_v_src, width, height, dst_pitch, src_pitch_uv);
    if (!mPoolInfo.b_use_addr) {
      p_y_dst = (TU8 *) mMapDSP.base + frame->yuv_luma_offset;
      p_me1 = (TU8 *) mMapDSP.base + frame->me1_offset;
    } else {
      p_y_dst = (TU8 *) frame->yuv_luma_offset;
      p_me1 = (TU8 *) frame->me1_offset;
    }
    me1_pitch = mPoolInfo.me1_pitch;
    DASSERT(mPoolInfo.me1_buf_width == (width >> 2));
    DASSERT(mPoolInfo.me1_buf_height == (height >> 2));
#ifdef BUILD_ASM_NEON
    SASMArguGenerateMe1Pitch param;
    param.dst = p_me1;
    param.src = p_y_dst;
    param.width = width;
    param.height = height;
    param.dst_pitch = me1_pitch;
    param.src_pitch = mPoolInfo.yuv_pitch;
    asm_neon_generate_me1_pitch(&param);
#else
    __generate_me1_pitch(p_me1, p_y_dst, width, height, me1_pitch, mPoolInfo.yuv_pitch);
#endif
  } else if ((width <= mPoolInfo.yuv_buf_width) && (height <= mPoolInfo.yuv_buf_height)) {
    p_y_src = buffer->GetDataPtr(0);
    p_u_src = buffer->GetDataPtr(1);
    p_v_src = buffer->GetDataPtr(2);
    src_pitch_y = buffer->GetDataLineSize(0);
    src_pitch_uv = uv_stride;
    if (!mPoolInfo.b_use_addr) {
      p_y_dst = (TU8 *) mMapDSP.base + frame->yuv_luma_offset;
      p_uv_dst = (TU8 *) mMapDSP.base + frame->yuv_chroma_offset;
    } else {
      p_y_dst = (TU8 *) frame->yuv_luma_offset;
      p_uv_dst = (TU8 *) frame->yuv_chroma_offset;
    }
    dst_pitch = mPoolInfo.yuv_pitch;
    if (dst_pitch == src_pitch_y) {
      memcpy(p_y_dst, p_y_src, height * dst_pitch);
    } else {
      for (j = 0; j < height; j ++) {
        memcpy(p_y_dst, p_y_src, width);
        p_y_src += src_pitch_y;
        p_y_dst += dst_pitch;
      }
    }
    __yuv420p_to_nv12_interlave_uv(p_uv_dst, p_u_src, p_v_src, width, height, dst_pitch, src_pitch_uv);
    if (!mPoolInfo.b_use_addr) {
      p_y_dst = (TU8 *) mMapDSP.base + frame->yuv_luma_offset;
      p_me1 = (TU8 *) mMapDSP.base + frame->me1_offset;
    } else {
      p_y_dst = (TU8 *) frame->yuv_luma_offset;
      p_me1 = (TU8 *) frame->me1_offset;
    }
    me1_pitch = mPoolInfo.me1_pitch;
#ifdef BUILD_ASM_NEON
    SASMArguGenerateMe1Pitch param;
    param.dst = p_me1;
    param.src = p_y_dst;
    param.width = mPoolInfo.yuv_buf_width;
    param.height = mPoolInfo.yuv_buf_height;
    param.dst_pitch = me1_pitch;
    param.src_pitch = dst_pitch;
    asm_neon_generate_me1_pitch(&param);
#else
    __generate_me1_pitch(p_me1, p_y_dst, mPoolInfo.yuv_buf_width, mPoolInfo.yuv_buf_height, me1_pitch, dst_pitch);
#endif
  } else {
    LOGM_ERROR("do not support down scale now\n");
  }
}

void CAmbaVideoEFMInjecter::feedFrameYUYV(CIBuffer *buffer, SAmbaEFMFrame *frame)
{
  if (StreamFormat_PixelFormat_YUYV != buffer->mContentFormat) {
    LOGM_FATAL("bad format %d\n", buffer->mContentFormat);
    return;
  }
  if ((buffer->mVideoWidth <= mPoolInfo.yuv_buf_width) && (buffer->mVideoHeight <= mPoolInfo.yuv_buf_height)) {
    TU8 *lu = NULL, *ch = NULL, *me1 = NULL;
    if (!mPoolInfo.b_use_addr) {
      lu = (TU8 *) mMapDSP.base + frame->yuv_luma_offset;
      ch = (TU8 *) mMapDSP.base + frame->yuv_chroma_offset;
      me1 = (TU8 *) mMapDSP.base + frame->me1_offset;
    } else {
      lu = (TU8 *) frame->yuv_luma_offset;
      ch = (TU8 *) frame->yuv_chroma_offset;
      me1 = (TU8 *) frame->me1_offset;
    }
#ifdef BUILD_ASM_NEON
    SASMArguCopyYUYVtoNV12 cy_params;
    cy_params.y_dst = lu;
    cy_params.uv_dst = ch;
    cy_params.src = buffer->GetDataPtr(0);
    cy_params.width = buffer->mVideoWidth;
    cy_params.height = buffer->mVideoHeight;
    cy_params.dst_y_pitch = mPoolInfo.yuv_pitch;
    cy_params.dst_uv_pitch = mPoolInfo.yuv_pitch;
    cy_params.src_pitch = buffer->GetDataLineSize(0);
    asm_neon_copy_yuyv_to_nv12(&cy_params);
    SASMArguGenerateMe1Pitch param;
    param.dst = me1;
    param.src = lu;
    param.width = buffer->mVideoWidth;
    param.height = buffer->mVideoHeight;
    param.dst_pitch = mPoolInfo.me1_pitch;
    param.src_pitch = mPoolInfo.yuv_pitch;
    asm_neon_generate_me1_pitch(&param);
#else
    __yuyv_to_nv12(lu, ch,
                   buffer->GetDataPtr(0), buffer->mVideoWidth, buffer->mVideoHeight,
                   mPoolInfo.yuv_pitch, mPoolInfo.yuv_pitch, buffer->GetDataLineSize(0));
    __generate_me1_pitch(me1, lu,
                         buffer->mVideoWidth, buffer->mVideoHeight,
                         mPoolInfo.me1_pitch, mPoolInfo.yuv_pitch);
#endif
  } else {
    LOGM_ERROR("do not support down scale now\n");
  }
}

void CAmbaVideoEFMInjecter::feedFrameNV12(CIBuffer *buffer, SAmbaEFMFrame *frame)
{
  if (StreamFormat_PixelFormat_NV12 != buffer->mContentFormat) {
    LOGM_FATAL("bad format %d\n", buffer->mContentFormat);
    return;
  }
  if ((buffer->mVideoWidth <= mPoolInfo.yuv_buf_width) && (buffer->mVideoHeight <= mPoolInfo.yuv_buf_height)) {
    TU8 *lu = NULL, *ch = NULL, *me1 = NULL;
    if (!mPoolInfo.b_use_addr) {
      lu = (TU8 *) mMapDSP.base + frame->yuv_luma_offset;
      ch = (TU8 *) mMapDSP.base + frame->yuv_chroma_offset;
      me1 = (TU8 *) mMapDSP.base + frame->me1_offset;
    } else {
      lu = (TU8 *) frame->yuv_luma_offset;
      ch = (TU8 *) frame->yuv_chroma_offset;
      me1 = (TU8 *) frame->me1_offset;
    }
    __copy_nv12(lu, ch,
                buffer->GetDataPtr(0), buffer->GetDataPtr(1),
                buffer->mVideoWidth, buffer->mVideoHeight,
                mPoolInfo.yuv_pitch, mPoolInfo.yuv_pitch,
                buffer->GetDataLineSize(0), buffer->GetDataLineSize(1));
#ifdef BUILD_ASM_NEON
    SASMArguGenerateMe1Pitch param;
    param.dst = me1;
    param.src = lu;
    param.width = buffer->mVideoWidth;
    param.height = buffer->mVideoHeight;
    param.dst_pitch = mPoolInfo.me1_pitch;
    param.src_pitch = mPoolInfo.yuv_pitch;
    asm_neon_generate_me1_pitch(&param);
#else
    __generate_me1_pitch(me1, lu,
                         buffer->mVideoWidth, buffer->mVideoHeight,
                         mPoolInfo.me1_pitch, mPoolInfo.yuv_pitch);
#endif
  } else {
    LOGM_ERROR("do not support down scale now\n");
  }
}

void CAmbaVideoEFMInjecter::fillBlackFrameNV12(SAmbaEFMFrame *frame)
{
  TU8 *lu = NULL, *ch = NULL, *me1 = NULL;
  if (!mPoolInfo.b_use_addr) {
    lu = (TU8 *) mMapDSP.base + frame->yuv_luma_offset;
    ch = (TU8 *) mMapDSP.base + frame->yuv_chroma_offset;
    me1 = (TU8 *) mMapDSP.base + frame->me1_offset;
  } else {
    lu = (TU8 *) frame->yuv_luma_offset;
    ch = (TU8 *) frame->yuv_chroma_offset;
    me1 = (TU8 *) frame->me1_offset;
  }
  memset(lu, 0x0, mPoolInfo.yuv_buf_height * mPoolInfo.yuv_pitch);
  memset(ch, 0x80, (mPoolInfo.yuv_buf_height / 2) * mPoolInfo.yuv_pitch);
  memset(me1, 0x80, mPoolInfo.me1_buf_height * mPoolInfo.me1_pitch);
}

#endif

