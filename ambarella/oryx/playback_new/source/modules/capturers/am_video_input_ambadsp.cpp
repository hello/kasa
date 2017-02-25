/*******************************************************************************
 * am_video_input_ambadsp.cpp
 *
 * History:
 *    2016/08/08 - [Zhi He] create file
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

#include "am_video_input_ambadsp.h"

IVideoInput *gfCreateVideoInputAmbaDSP(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
  return CVideoInputAmbaDSP::Create(pName, pPersistMediaConfig, pEngineMsgSink);
}

//-----------------------------------------------------------------------
//
// CVideoInputAmbaDSP
//
//-----------------------------------------------------------------------
IVideoInput *CVideoInputAmbaDSP::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
  CVideoInputAmbaDSP *result = new CVideoInputAmbaDSP(pName, pPersistMediaConfig, pEngineMsgSink);
  if (result && result->Construct() != EECode_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

void CVideoInputAmbaDSP::Destroy()
{
  Delete();
}

CVideoInputAmbaDSP::CVideoInputAmbaDSP(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
  : inherited(pName, 0)
  , mpPersistMediaConfig(pPersistMediaConfig)
  , mpMsgSink(pEngineMsgSink)
  , mpMutex(NULL)
  , mCurFormat(StreamFormat_PixelFormat_NV12)
  , mCurCaptureWidth(0)
  , mCurCaptureHeight(0)
  , mbSetup(0)
  , mbSendSyncBuffer(0)
  , mBufferID(0)
  , mpBufferPool(NULL)
{
  memset(&mfDSPAL, 0x0, sizeof(mfDSPAL));
  memset(&mMapDSP, 0x0, sizeof(mMapDSP));
  memset(&mYUVBuffer, 0x0, sizeof(mYUVBuffer));
  mbPreallocateBuffer = 0;
}

EECode CVideoInputAmbaDSP::Construct()
{
  DSET_MODULE_LOG_CONFIG(ELogModuleVideoInput);
  mpMutex = gfCreateMutex();
  if (!mpMutex) {
    LOGM_FATAL("no memory\n");
    return EECode_NoMemory;
  }
  gfSetupDSPAlContext(&mfDSPAL);
  return EECode_OK;
}

CVideoInputAmbaDSP::~CVideoInputAmbaDSP()
{
  if (mpMutex) {
    mpMutex->Delete();
    mpMutex = NULL;
  }
}

EECode CVideoInputAmbaDSP::SetupContext(SVideoCaptureParams *param)
{
  TInt ret = 0;
  DASSERT(mpPersistMediaConfig);
  DASSERT(mfDSPAL.f_map_dsp);
  DASSERT(mfDSPAL.f_query_source_buffer_info);
  DASSERT(mfDSPAL.f_query_yuv_buffer);
  if (DUnlikely(mbSetup)) {
    LOGM_ERROR("already setup\n");
    return EECode_OK;
  }
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
  DASSERT(param);
  mBufferID = param->buffer_id;
  mSourceBufferInfo.buf_id = mBufferID;
  ret = mfDSPAL.f_query_source_buffer_info(mIavFd, &mSourceBufferInfo);
  if (0 > ret) {
    LOGM_ERROR("query buffer info fail, buf id %d\n", mSourceBufferInfo.buf_id);
    return EECode_OSError;
  }
  mCurFormat = StreamFormat_PixelFormat_NV12;
  mCurCaptureWidth = mSourceBufferInfo.size_width;
  mCurCaptureHeight = mSourceBufferInfo.size_height;
  LOGM_CONFIGURATION("buffer id %d, buffer size %dx%d\n", mBufferID, mCurCaptureWidth, mCurCaptureHeight);
  mbSetup = 1;
  if (param->b_snapshot_mode) {
    mbPreallocateBuffer = 1;
  }
  return EECode_OK;
}

void CVideoInputAmbaDSP::DestroyContext()
{
  if (mbSetup) {
    mbSetup = 0;
  }
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

void CVideoInputAmbaDSP::SetBufferPool(IBufferPool *buffer_pool)
{
  mpBufferPool = buffer_pool;
  if (mbPreallocateBuffer) {
    preAllocateFramebuffers();
  }
}

EECode CVideoInputAmbaDSP::Capture(CIBuffer *buffer)
{
  EECode err = EECode_OK;
  if (DUnlikely(!buffer)) {
    LOGM_FATAL("NULL buffer\n");
    return EECode_BadParam;
  }
  if (DLikely(mbSetup)) {
    TInt ret = 0;
    if (!buffer->mbAllocated) {
      err = gfAllocateNV12FrameBuffer(buffer, mCurCaptureWidth, mCurCaptureHeight, 32);
      if (EECode_OK != err) {
        LOGM_FATAL("new nv12 frame buffer fail, ret 0x%x, %s\n", err, gfGetErrorCodeString(err));
        return err;
      }
      LOGM_NOTICE("allocate nv12 buffer\n");
    }
    mYUVBuffer.buf_id = mBufferID;
    ret = mfDSPAL.f_query_yuv_buffer(mIavFd, &mYUVBuffer);
    if (ret) {
      LOGM_ERROR("query yuv buffer fail, ret %d\n", ret);
      return EECode_OSError;
    }
    DASSERT(mYUVBuffer.width == mCurCaptureWidth);
    DASSERT(mYUVBuffer.height == mCurCaptureHeight);
    if (mCurCaptureWidth == mYUVBuffer.pitch) {
      memcpy(buffer->GetDataPtr(0), ((TU8 *)mMapDSP.base) + mYUVBuffer.y_addr_offset, mCurCaptureWidth * mCurCaptureHeight);
      memcpy(buffer->GetDataPtr(1), ((TU8 *)mMapDSP.base) + mYUVBuffer.uv_addr_offset, (mCurCaptureWidth * mCurCaptureHeight) / 2);
    } else {
      TU32 i = 0;
      TU8 *p_src = ((TU8 *)mMapDSP.base) + mYUVBuffer.y_addr_offset;
      TU8 *p_des = buffer->GetDataPtr(0);
      for (i = 0; i < mCurCaptureHeight; i ++) {
        memcpy(p_des, p_src, mCurCaptureWidth);
        p_src += mYUVBuffer.pitch;
        p_des += mCurCaptureWidth;
      }
      p_src = ((TU8 *)mMapDSP.base) + mYUVBuffer.uv_addr_offset;
      p_des = buffer->GetDataPtr(1);
      for (i = 0; i < mCurCaptureHeight; i += 2) {
        memcpy(p_des, p_src, mCurCaptureWidth);
        p_src += mYUVBuffer.pitch;
        p_des += mCurCaptureWidth;
      }
    }
    //LOGM_PRINTF("dsp pts %d, mono pts %" DPrint64d "\n", mYUVBuffer.dsp_pts, mYUVBuffer.mono_pts);
    buffer->SetBufferLinearDTS((TTime)mYUVBuffer.mono_pts);
    buffer->SetBufferLinearPTS((TTime)mYUVBuffer.mono_pts);
    buffer->SetBufferNativeDTS((TTime)mYUVBuffer.dsp_pts);
    buffer->SetBufferNativePTS((TTime)mYUVBuffer.dsp_pts);
    buffer->mpCustomizedContent = NULL;
    buffer->mbDataSegment = 0;
    buffer->mVideoWidth = mCurCaptureWidth;
    buffer->mVideoHeight = mCurCaptureHeight;
    buffer->mContentFormat = mCurFormat;
    buffer->SetBufferFlags(CIBuffer::KEY_FRAME);
    if (DUnlikely(!mbSendSyncBuffer)) {
      buffer->SetBufferFlagBits(CIBuffer::SYNC_POINT);
      mbSendSyncBuffer = 1;
    }
    buffer->SetBufferType(EBufferType_VideoFrameBuffer);
  } else {
    LOGM_ERROR("not setup yet\n");
    return EECode_BadState;
  }
  return EECode_OK;
}

EECode CVideoInputAmbaDSP::preAllocateFramebuffers()
{
  CIBuffer *buffer = NULL;
  EECode err = EECode_OK;
  DASSERT(mpBufferPool);
  while (1) {
    mpBufferPool->AllocBuffer(buffer, sizeof(CIBuffer));
    if (!buffer->mbAllocated) {
      err = gfAllocateNV12FrameBuffer(buffer, mCurCaptureWidth, mCurCaptureHeight, 32);
      if (EECode_OK != err) {
        LOGM_FATAL("allocate nv12 frame buffer fail, ret 0x%x, %s\n", err, gfGetErrorCodeString(err));
        return err;
      }
      LOGM_NOTICE("allocate nv12 buffer\n");
      buffer->Release();
      buffer = NULL;
    } else {
      break;
    }
  }
  if (buffer) {
    buffer->Release();
    buffer = NULL;
  }
  return EECode_OK;
}

#endif

