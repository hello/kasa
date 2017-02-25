/**
 * am_ffmpeg_video_decoder.cpp
 *
 * History:
 *    2014/09/26 - [Zhi He] create file
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

#ifdef BUILD_MODULE_FFMPEG

extern "C" {
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}

#include "am_ffmpeg_video_decoder.h"

#define DBUFFER_ALIGNMENT 32
#define DBUFFER_EXT_EDGE_SIZE 16

static EECode _new_video_frame_buffer(CIBuffer *buffer, EPixelFMT pix_format, TU32 width, TU32 height, TU32 ext_edge)
{
  TU32 size = 0;
  TU32 aligned_width = 0, aligned_height = 0;
  TU8 *p = NULL, *palign = NULL;
  if (DUnlikely(!buffer)) {
    LOG_FATAL("_new_video_frame_buffer, NULL buffer pointer\n");
    return EECode_BadParam;
  }
  if (DUnlikely(!width || !height)) {
    LOG_ERROR("!!!_new_video_frame_buffer, zero width %d or height %d\n", width, height);
    return EECode_BadParam;
  }
  if (DUnlikely(buffer->mbAllocated)) {
    LOG_ERROR("!!!_new_video_frame_buffer, already allocated\n");
    return EECode_BadParam;
  }
  if (DLikely(EPixelFMT_yuv420p == pix_format)) {
    buffer->mVideoWidth = width;
    buffer->mVideoHeight = height;
    aligned_width = (width + (DBUFFER_ALIGNMENT - 1)) & (~(DBUFFER_ALIGNMENT - 1));
    aligned_height = (height + (DBUFFER_ALIGNMENT - 1)) & (~(DBUFFER_ALIGNMENT - 1));
    if (ext_edge) {
      buffer->mExtVideoWidth = aligned_width + DBUFFER_EXT_EDGE_SIZE * 2;
      buffer->mExtVideoHeight = aligned_height + DBUFFER_EXT_EDGE_SIZE * 2;
      buffer->mbExtEdge = 1;
    } else {
      buffer->mExtVideoWidth = aligned_width;
      buffer->mExtVideoHeight = aligned_height;
      buffer->mbExtEdge = 0;
    }
    size = (buffer->mExtVideoWidth * buffer->mExtVideoHeight) * 3 / 2 + (DBUFFER_ALIGNMENT * 4);
    p = (TU8 *) DDBG_MALLOC(size, "VDFB");
    if (DUnlikely(!p)) {
      LOG_ERROR("_new_video_frame_buffer, not enough memory %u, width/height with ext %d/%d\n", size, buffer->mExtVideoWidth, buffer->mExtVideoHeight);
      return EECode_NoMemory;
    }
    memset(p, 0x0, size);
    buffer->SetDataPtrBase(p, 0);
    buffer->SetDataMemorySize(size, 0);
    size = buffer->mExtVideoWidth * buffer->mExtVideoHeight;
    if (ext_edge) {
      palign = (TU8 *)((((TULong)p + (buffer->mExtVideoWidth * DBUFFER_EXT_EDGE_SIZE) + DBUFFER_EXT_EDGE_SIZE + (DBUFFER_ALIGNMENT - 1)) & (~(DBUFFER_ALIGNMENT - 1))));
      p = (TU8 *)((TULong)palign - (buffer->mExtVideoWidth * DBUFFER_EXT_EDGE_SIZE) - DBUFFER_EXT_EDGE_SIZE);
      buffer->SetDataPtr(palign, 0);
      palign = (TU8 *)((((TULong)p + size + ((buffer->mExtVideoWidth >> 1) * (DBUFFER_EXT_EDGE_SIZE >> 1)) + (DBUFFER_EXT_EDGE_SIZE >> 1) + (DBUFFER_ALIGNMENT - 1)) & (~(DBUFFER_ALIGNMENT - 1))));
      buffer->SetDataPtr(palign, 1);
      palign = (TU8 *)((((TULong)p + size + (size >> 2) + ((buffer->mExtVideoWidth >> 1) * (DBUFFER_EXT_EDGE_SIZE >> 1)) + (DBUFFER_EXT_EDGE_SIZE >> 1) + (DBUFFER_ALIGNMENT - 1)) & (~(DBUFFER_ALIGNMENT - 1))));
      buffer->SetDataPtr(palign, 2);
    } else {
      palign = (TU8 *)(((TULong)p + (DBUFFER_ALIGNMENT - 1)) & (~(DBUFFER_ALIGNMENT - 1)));
      buffer->SetDataPtr(palign, 0);
      palign = (TU8 *)(((TULong)palign + size + (DBUFFER_ALIGNMENT - 1)) & (~(DBUFFER_ALIGNMENT - 1)));
      buffer->SetDataPtr(palign, 1);
      palign = (TU8 *)(((TULong)palign + (size >> 2) + (DBUFFER_ALIGNMENT - 1)) & (~(DBUFFER_ALIGNMENT - 1)));
      buffer->SetDataPtr(palign, 2);
    }
    buffer->SetDataLineSize(buffer->mExtVideoWidth, 0);
    buffer->SetDataLineSize(buffer->mExtVideoWidth / 2, 1);
    buffer->SetDataLineSize(buffer->mExtVideoWidth / 2, 2);
  } else if (DLikely(EPixelFMT_yuv422p == pix_format)) {
    buffer->mVideoWidth = width;
    buffer->mVideoHeight = height;
    aligned_width = (width + (DBUFFER_ALIGNMENT - 1)) & (~(DBUFFER_ALIGNMENT - 1));
    aligned_height = (height + (DBUFFER_ALIGNMENT - 1)) & (~(DBUFFER_ALIGNMENT - 1));
    if (ext_edge) {
      buffer->mExtVideoWidth = aligned_width + DBUFFER_EXT_EDGE_SIZE * 2;
      buffer->mExtVideoHeight = aligned_height + DBUFFER_EXT_EDGE_SIZE * 2;
      buffer->mbExtEdge = 1;
    } else {
      buffer->mExtVideoWidth = aligned_width;
      buffer->mExtVideoHeight = aligned_height;
      buffer->mbExtEdge = 0;
    }
    size = (buffer->mExtVideoWidth * buffer->mExtVideoHeight) * 2 + (DBUFFER_ALIGNMENT * 4);
    p = (TU8 *) DDBG_MALLOC(size, "VDFB");
    if (DUnlikely(!p)) {
      LOG_ERROR("_new_video_frame_buffer, not enough memory %u, width/height with ext %d/%d\n", size, buffer->mExtVideoWidth, buffer->mExtVideoHeight);
      return EECode_NoMemory;
    }
    buffer->SetDataPtrBase(p, 0);
    buffer->SetDataMemorySize(size, 0);
    size = buffer->mExtVideoWidth * buffer->mExtVideoHeight;
    if (ext_edge) {
      palign = (TU8 *)((((TULong)p + (buffer->mExtVideoWidth * DBUFFER_EXT_EDGE_SIZE) + DBUFFER_EXT_EDGE_SIZE + (DBUFFER_ALIGNMENT - 1)) & (~(DBUFFER_ALIGNMENT - 1))));
      p = (TU8 *)((TULong)palign - (buffer->mExtVideoWidth * DBUFFER_EXT_EDGE_SIZE) - DBUFFER_EXT_EDGE_SIZE);
      buffer->SetDataPtr(palign, 0);
      palign = (TU8 *)((((TULong)p + size + ((buffer->mExtVideoWidth >> 1) * (DBUFFER_EXT_EDGE_SIZE >> 1)) + (DBUFFER_EXT_EDGE_SIZE >> 1) + (DBUFFER_ALIGNMENT - 1)) & (~(DBUFFER_ALIGNMENT - 1))));
      buffer->SetDataPtr(palign, 1);
      palign = (TU8 *)((((TULong)p + size + (size >> 1) + ((buffer->mExtVideoWidth >> 1) * (DBUFFER_EXT_EDGE_SIZE >> 1)) + (DBUFFER_EXT_EDGE_SIZE >> 1) + (DBUFFER_ALIGNMENT - 1)) & (~(DBUFFER_ALIGNMENT - 1))));
      buffer->SetDataPtr(palign, 2);
    } else {
      palign = (TU8 *)(((TULong)p + (DBUFFER_ALIGNMENT - 1)) & (~(DBUFFER_ALIGNMENT - 1)));
      buffer->SetDataPtr(palign, 0);
      palign = (TU8 *)(((TULong)palign + size + (DBUFFER_ALIGNMENT - 1)) & (~(DBUFFER_ALIGNMENT - 1)));
      buffer->SetDataPtr(palign, 1);
      palign = (TU8 *)(((TULong)palign + (size >> 1) + (DBUFFER_ALIGNMENT - 1)) & (~(DBUFFER_ALIGNMENT - 1)));
      buffer->SetDataPtr(palign, 2);
    }
    buffer->SetDataLineSize(buffer->mExtVideoWidth, 0);
    buffer->SetDataLineSize(buffer->mExtVideoWidth / 2, 1);
    buffer->SetDataLineSize(buffer->mExtVideoWidth / 2, 2);
  } else {
    LOG_ERROR("!!!_new_video_frame_buffer, only support 420p and 422p now\n");
    return EECode_NotSupported;
  }
  buffer->mbAllocated = 1;
  return EECode_OK;
}

#if (LIBAVCODEC_VERSION_MAJOR < 57)

static int _callback_get_buffer_yuv422_for_jpeg(AVCodecContext *s, AVFrame *pic)
{
  CIBuffer *p_buffer = NULL;
  IBufferPool *p_buffer_pool = (IBufferPool *)s->opaque;
  if (DUnlikely(NULL == p_buffer_pool)) {
    LOG_FATAL("NULL p_buffer_pool\n");
    return (-3);
  }
  if (!p_buffer_pool->AllocBuffer(p_buffer, 0)) {
    LOG_FATAL("AllocBuffer fail\n");
    return (-4);
  }
  if (DUnlikely(!p_buffer->mbAllocated)) {
    EECode err = _new_video_frame_buffer(p_buffer, EPixelFMT_yuv422p, s->width, s->height, 1);
    if (DUnlikely(EECode_OK != err)) {
      LOG_FATAL("_new_video_frame_buffer fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
      return (-5);
    }
  }
  p_buffer->AddRef();
  pic->data[0] = p_buffer->GetDataPtr(0);
  pic->linesize[0] = p_buffer->GetDataLineSize(0);
  pic->data[1] = p_buffer->GetDataPtr(1);
  pic->linesize[1] = p_buffer->GetDataLineSize(1);
  pic->data[2] = p_buffer->GetDataPtr(2);
  pic->linesize[2] = p_buffer->GetDataLineSize(2);
#ifndef BUILD_OS_WINDOWS
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
  //pic->age = 256*256*256*64;
  pic->type = FF_BUFFER_TYPE_USER;
  pic->opaque = (void *)p_buffer;
#ifndef BUILD_OS_WINDOWS
#pragma GCC diagnostic pop
#endif
  return 0;
}

static int _callback_get_buffer(AVCodecContext *s, AVFrame *pic)
{
  CIBuffer *p_buffer = NULL;
  IBufferPool *p_buffer_pool = (IBufferPool *)s->opaque;
  if (DUnlikely(NULL == p_buffer_pool)) {
    LOG_FATAL("NULL p_buffer_pool\n");
    return (-3);
  }
  if (!p_buffer_pool->AllocBuffer(p_buffer, 0)) {
    LOG_FATAL("AllocBuffer fail\n");
    return (-4);
  }
  if (DUnlikely(!p_buffer->mbAllocated)) {
    EECode err = _new_video_frame_buffer(p_buffer, EPixelFMT_yuv420p, s->width, s->height, 1);
    if (DUnlikely(EECode_OK != err)) {
      LOG_FATAL("_new_video_frame_buffer fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
      return (-5);
    }
  }
  p_buffer->AddRef();
  pic->data[0] = p_buffer->GetDataPtr(0);
  pic->linesize[0] = p_buffer->GetDataLineSize(0);
  pic->data[1] = p_buffer->GetDataPtr(1);
  pic->linesize[1] = p_buffer->GetDataLineSize(1);
  pic->data[2] = p_buffer->GetDataPtr(2);
  pic->linesize[2] = p_buffer->GetDataLineSize(2);
#ifndef BUILD_OS_WINDOWS
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
  //pic->age = 256*256*256*64;
  pic->type = FF_BUFFER_TYPE_USER;
  pic->opaque = (void *)p_buffer;
#ifndef BUILD_OS_WINDOWS
#pragma GCC diagnostic pop
#endif
  return 0;
}

static void _callback_release_buffer(AVCodecContext *s, AVFrame *pic)
{
  CIBuffer *p_buffer = (CIBuffer *)pic->opaque;
  IBufferPool *p_buffer_pool = (IBufferPool *)s->opaque;
  if (DUnlikely((NULL == p_buffer_pool) || (NULL == p_buffer))) {
    LOG_FATAL("NULL p_buffer_pool or NULL p_buffer\n");
    return;
  }
  p_buffer->Release();
#ifndef BUILD_OS_WINDOWS
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
  pic->data[0] = NULL;
  pic->type = 0;
#ifndef BUILD_OS_WINDOWS
#pragma GCC diagnostic pop
#endif
}

#else

static void _callback_release_buffer_v3(void *ctx, TU8 *p)
{
  CIBuffer *p_buffer = (CIBuffer *) ctx;
  if (p_buffer) {
    p_buffer->Release();
  }
}

static int _callback_get_buffer_yuv422_for_jpeg_v3(AVCodecContext *s, AVFrame *pic, int flags)
{
  CIBuffer *p_buffer = NULL;
  IBufferPool *p_buffer_pool = (IBufferPool *)s->opaque;
  if (DUnlikely(NULL == p_buffer_pool)) {
    LOG_FATAL("NULL p_buffer_pool\n");
    return (-3);
  }
  if (!p_buffer_pool->AllocBuffer(p_buffer, 0)) {
    LOG_FATAL("AllocBuffer fail\n");
    return (-4);
  }
  if (DUnlikely(!p_buffer->mbAllocated)) {
    EECode err = _new_video_frame_buffer(p_buffer, EPixelFMT_yuv422p, s->width, s->height, 1);
    if (DUnlikely(EECode_OK != err)) {
      LOG_FATAL("_new_video_frame_buffer fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
      return (-5);
    }
  }
  p_buffer->AddRef();
  pic->data[0] = p_buffer->GetDataPtr(0);
  pic->linesize[0] = p_buffer->GetDataLineSize(0);
  pic->data[1] = p_buffer->GetDataPtr(1);
  pic->linesize[1] = p_buffer->GetDataLineSize(1);
  pic->data[2] = p_buffer->GetDataPtr(2);
  pic->linesize[2] = p_buffer->GetDataLineSize(2);
  pic->opaque = (void *)p_buffer;
  pic->buf[0] = av_buffer_create(NULL, 0, _callback_release_buffer_v3, p_buffer, 0);
  return 0;
}

static int _callback_get_buffer_v3(AVCodecContext *s, AVFrame *pic, int flags)
{
  CIBuffer *p_buffer = NULL;
  IBufferPool *p_buffer_pool = (IBufferPool *)s->opaque;
  if (DUnlikely(NULL == p_buffer_pool)) {
    LOG_FATAL("NULL p_buffer_pool\n");
    return (-3);
  }
  if (!p_buffer_pool->AllocBuffer(p_buffer, 0)) {
    LOG_FATAL("AllocBuffer fail\n");
    return (-4);
  }
  if (DUnlikely(!p_buffer->mbAllocated)) {
    EECode err = _new_video_frame_buffer(p_buffer, EPixelFMT_yuv420p, s->width, s->height, 1);
    if (DUnlikely(EECode_OK != err)) {
      LOG_FATAL("_new_video_frame_buffer fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
      return (-5);
    }
  }
  p_buffer->AddRef();
  pic->data[0] = p_buffer->GetDataPtr(0);
  pic->linesize[0] = p_buffer->GetDataLineSize(0);
  pic->data[1] = p_buffer->GetDataPtr(1);
  pic->linesize[1] = p_buffer->GetDataLineSize(1);
  pic->data[2] = p_buffer->GetDataPtr(2);
  pic->linesize[2] = p_buffer->GetDataLineSize(2);
  pic->opaque = (void *)p_buffer;
  pic->buf[0] = av_buffer_create(NULL, 0, _callback_release_buffer_v3, p_buffer, 0);
  return 0;
}

#endif

IVideoDecoder *gfCreateFFMpegVideoDecoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
{
  return CFFMpegVideoDecoder::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

//-----------------------------------------------------------------------
//
// CFFMpegVideoDecoder
//
//-----------------------------------------------------------------------
CFFMpegVideoDecoder::CFFMpegVideoDecoder(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index)
  : inherited(pname, index)
  , mpPersistMediaConfig(pPersistMediaConfig)
  , mpEngineMsgSink(pMsgSink)
  , mpCodec(NULL)
  , mpDecoder(NULL)
  , mCodecID(AV_CODEC_ID_NONE)
  , mpExtraData(NULL)
  , mExtraDataBufferSize(0)
  , mExtraDataSize(0)
  , mbDecoderSetup(0)
{
  mbSkipSPSPPS = 0;
  mbUseFFMpegDirectRendering = 1;
  mbDump = 0;
  mpDumper = NULL;
  mDumpIndex = 0;
}

CFFMpegVideoDecoder::~CFFMpegVideoDecoder()
{
}

EECode CFFMpegVideoDecoder::Construct()
{
  if (mpPersistMediaConfig->p_global_mutex) {
    mpPersistMediaConfig->p_global_mutex->Lock();
    if (!mpPersistMediaConfig->b_init_ffmpeg_library) {
      ((volatile SPersistMediaConfig *) mpPersistMediaConfig)->b_init_ffmpeg_library = 1;
      av_register_all();
      //avcodec_register_all();
    }
    mpPersistMediaConfig->p_global_mutex->Unlock();
  } else {
    if (!mpPersistMediaConfig->b_init_ffmpeg_library) {
      ((volatile SPersistMediaConfig *) mpPersistMediaConfig)->b_init_ffmpeg_library = 1;
      av_register_all();
      //avcodec_register_all();
    }
  }
  return EECode_OK;
}

IVideoDecoder *CFFMpegVideoDecoder::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index)
{
  CFFMpegVideoDecoder *result = new CFFMpegVideoDecoder(pname, pPersistMediaConfig, pMsgSink, index);
  if (DUnlikely(result && result->Construct() != EECode_OK)) {
    delete result;
    result = NULL;
  }
  return result;
}

CObject *CFFMpegVideoDecoder::GetObject0() const
{
  return (CObject *) this;
}

void CFFMpegVideoDecoder::PrintStatus()
{
}

EECode CFFMpegVideoDecoder::SetupContext(SVideoDecoderInputParam *param)
{
  if (!mpBufferPool) {
    LOGM_ERROR("not specified buffer pool\n");
    return EECode_BadState;
  }
  if (StreamFormat_H264 == param->format) {
    mCodecID = AV_CODEC_ID_H264;
    mbSkipSPSPPS = 1;
  } else if (StreamFormat_H265 == param->format) {
    mCodecID = AV_CODEC_ID_HEVC;
  } else if (StreamFormat_JPEG == param->format) {
    mCodecID = AV_CODEC_ID_MJPEG;
  } else if (StreamFormat_FFMpegCustomized == param->format) {
    mCodecID = (AVCodecID) param->customized_content_format;
  } else {
    LOGM_ERROR("BAD format %d\n", param->format);
    return EECode_BadParam;
  }
  EECode err = setupDecoder();
  if (DUnlikely(EECode_OK != err)) {
    LOGM_ERROR("setupDecoder fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
    return err;
  }
  return EECode_OK;
}

EECode CFFMpegVideoDecoder::DestroyContext()
{
  destroyDecoder();
  if (mpExtraData) {
    DDBG_FREE(mpExtraData, "VDEX");
    mpExtraData = NULL;
    mExtraDataBufferSize = 0;
  }
  if (mpDumper) {
    gfDestroyFileDumper(mpDumper);
    mpDumper = NULL;
  }
  return EECode_OK;
}

EECode CFFMpegVideoDecoder::SetBufferPool(IBufferPool *buffer_pool)
{
  mpBufferPool = buffer_pool;
  return EECode_OK;
}

EECode CFFMpegVideoDecoder::Start()
{
  return EECode_OK;
}

EECode CFFMpegVideoDecoder::Stop()
{
  return EECode_OK;
}

EECode CFFMpegVideoDecoder::Decode(CIBuffer *in_buffer, CIBuffer *&out_buffer)
{
  av_init_packet(&mPacket);
  if (DLikely(in_buffer)) {
    mPacket.data = in_buffer->GetDataPtr();
    mPacket.size = in_buffer->GetDataSize();
    if (mbDump) {
      TChar filename[128] = {0};
      snprintf(filename, 128, "./dump/%06d.jpg", mDumpIndex);
      mpDumper = gfCreateFileDumper(filename);
      if (mpDumper) {
        gfFileDump(mpDumper, mPacket.data, mPacket.size);
        gfDestroyFileDumper(mpDumper);
        mpDumper = NULL;
      } else {
        LOGM_WARN("open file fail, disable dump\n");
        mbDump = 0;
      }
      mDumpIndex ++;
    }
    mPacket.pts = in_buffer->GetBufferNativePTS();
    if (DUnlikely(in_buffer->GetBufferFlags() & CIBuffer::KEY_FRAME)) {
      mPacket.flags = AV_PKT_FLAG_KEY;
      if ((mbSkipSPSPPS && (AV_CODEC_ID_H264 == mCodecID))) {
        TU8 *p_idr = gfNALUFindIDR(mPacket.data, mPacket.size);
        if (p_idr) {
          TU32 new_size = mPacket.size - (TU32)(p_idr - mPacket.data);
          mPacket.data = p_idr;
          mPacket.size = new_size;
        } else {
          LOGM_FATAL("no found idr!\n");
          gfPrintMemory(mPacket.data, 256);
        }
      }
    }
  } else {
    LOGM_WARN("NULL in_buffer, get last frames? to do\n");
  }
  if (mpCodec) {
    mpCodec->extradata = mpExtraData;
    mpCodec->extradata_size = mExtraDataSize;
  }
  TInt frame_finished = 0;
  memset(&mFrame, 0x0, sizeof(AVFrame));
#if (LIBAVCODEC_VERSION_MAJOR > 56)
#ifndef BUILD_OS_WINDOWS
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#endif
  TInt ret = avcodec_decode_video2(mpCodec, &mFrame, &frame_finished, &mPacket);
#if (LIBAVCODEC_VERSION_MAJOR > 56)
#ifndef BUILD_OS_WINDOWS
#pragma GCC diagnostic pop
#endif
#endif
  if (DUnlikely(0 > ret)) {
    LOGM_ERROR("decode error\n");
    if (mFrame.opaque) {
      CIBuffer *p_buffer = (CIBuffer *) mFrame.opaque;
      p_buffer->Release();
    }
    return EECode_Error;
  }
  out_buffer = (CIBuffer *)mFrame.opaque;
  if (out_buffer) {
    if ((AV_PIX_FMT_YUV420P == mFrame.format) || (AV_PIX_FMT_YUVJ420P == mFrame.format)) {
      out_buffer->mContentFormat = StreamFormat_PixelFormat_YUV420p;
    } else if ((AV_PIX_FMT_YUV422P == mFrame.format) || (AV_PIX_FMT_YUVJ422P == mFrame.format)) {
      out_buffer->mContentFormat = StreamFormat_PixelFormat_YUV422p;
    } else if (AV_PIX_FMT_GRAY8 == mFrame.format) {
      out_buffer->mContentFormat = StreamFormat_PixelFormat_GRAY8;
    } else {
      LOGM_FATAL("yuv format not supported, %d\n", mFrame.format);
      return EECode_NotSupported;
    }
  }
  return EECode_OK;
}

EECode CFFMpegVideoDecoder::Flush()
{
  LOGM_ERROR("CFFMpegVideoDecoder::Flush TO DO\n");
  return EECode_NoImplement;
}

EECode CFFMpegVideoDecoder::Suspend()
{
  LOGM_ERROR("CFFMpegVideoDecoder::Suspend TO DO\n");
  return EECode_NoImplement;
}

EECode CFFMpegVideoDecoder::SetExtraData(TU8 *p, TMemSize size)
{
  if (mpExtraData) {
    if (size > mExtraDataBufferSize) {
      DDBG_FREE(mpExtraData, "VDEX");
      mpExtraData = NULL;
      mExtraDataBufferSize = size;
    }
  } else {
    mExtraDataBufferSize = size;
  }
  if (!mpExtraData) {
    mpExtraData = (TU8 *) DDBG_MALLOC(mExtraDataBufferSize, "VDEX");
    if (!mpExtraData) {
      LOG_FATAL("CFFMpegVideoDecoder::SetExtraData no memory\n");
      return EECode_NoMemory;
    }
  }
  mExtraDataSize = size;
  memcpy(mpExtraData, p, mExtraDataSize);
  LOGM_PRINTF("set extra data, %ld, mpCodec %p\n", size, mpCodec);
  gfPrintMemory(p, size);
  if (mpCodec) {
    mpCodec->extradata = mpExtraData;
    mpCodec->extradata_size = mExtraDataSize;
  }
  return EECode_OK;
}

EECode CFFMpegVideoDecoder::SetPbRule(TU8 direction, TU8 feeding_rule, TU16 speed)
{
  LOGM_ERROR("CFFMpegVideoDecoder::SetPbRule TO DO\n");
  return EECode_NoImplement;
}

EECode CFFMpegVideoDecoder::SetFrameRate(TUint framerate_num, TUint frameate_den)
{
  LOGM_ERROR("CFFMpegVideoDecoder::SetFrameRate TO DO\n");
  return EECode_NoImplement;
}

EDecoderMode CFFMpegVideoDecoder::GetDecoderMode() const
{
  return EDecoderMode_Normal;
}

EECode CFFMpegVideoDecoder::DecodeDirect(CIBuffer *in_buffer)
{
  LOGM_FATAL("not supported\n");
  return EECode_NotSupported;
}

EECode CFFMpegVideoDecoder::SetBufferPoolDirect(IOutputPin *p_output_pin, IBufferPool *p_bufferpool)
{
  LOGM_FATAL("not supported\n");
  return EECode_NotSupported;
}

EECode CFFMpegVideoDecoder::QueryFreeZoom(TUint &free_zoom, TU32 &fullness)
{
  LOGM_ERROR("CFFMpegVideoDecoder::QueryFreeZoom TO DO\n");
  return EECode_NoImplement;
}

EECode CFFMpegVideoDecoder::QueryLastDisplayedPTS(TTime &pts)
{
  LOGM_ERROR("CFFMpegVideoDecoder::QueryLastDisplayedPTS TO DO\n");
  return EECode_NoImplement;
}

EECode CFFMpegVideoDecoder::PushData(CIBuffer *in_buffer, TInt flush)
{
  LOGM_ERROR("CFFMpegVideoDecoder::PushData TO DO\n");
  return EECode_NoImplement;
}

EECode CFFMpegVideoDecoder::PullDecodedFrame(CIBuffer *out_buffer, TInt block_wait, TInt &remaining)
{
  LOGM_ERROR("CFFMpegVideoDecoder::PullDecodedFrame TO DO\n");
  return EECode_NoImplement;
}

EECode CFFMpegVideoDecoder::TuningPB(TU8 fw, TU16 frame_tick)
{
  LOGM_ERROR("CFFMpegVideoDecoder::TuningPB TO DO\n");
  return EECode_NoImplement;
}

EECode CFFMpegVideoDecoder::setupDecoder()
{
  if (DUnlikely(mbDecoderSetup)) {
    LOGM_WARN("decoder already setup?\n");
    return EECode_BadState;
  }
  if (DUnlikely(!mpBufferPool)) {
    LOGM_WARN("not set buffer pool\n");
    return EECode_BadState;
  }
  mpDecoder = avcodec_find_decoder(mCodecID);
  if (DUnlikely(NULL == mpDecoder)) {
    LOGM_ERROR("can not find codec_id %d\n", mCodecID);
    return EECode_BadParam;
  }
  mpCodec = avcodec_alloc_context3((const AVCodec *) mpDecoder);
  if (DUnlikely(NULL == mpCodec)) {
    LOGM_ERROR("avcodec_alloc_context3 fail\n");
    return EECode_NoMemory;
  }
  if (AV_CODEC_ID_MJPEG != mCodecID) {
    ((volatile SPersistMediaConfig *)mpPersistMediaConfig)->pb_decode.prefer_thread_number = 2;
    ((volatile SPersistMediaConfig *)mpPersistMediaConfig)->pb_decode.prefer_parallel_frame = 1;
    ((volatile SPersistMediaConfig *)mpPersistMediaConfig)->pb_decode.prefer_parallel_slice = 1;
    if (mpPersistMediaConfig->pb_decode.prefer_thread_number) {
      mpCodec->thread_count = mpPersistMediaConfig->pb_decode.prefer_thread_number;
      mpCodec->thread_safe_callbacks = 1;
      if (mpPersistMediaConfig->pb_decode.prefer_parallel_frame) {
        mpCodec->thread_type |= FF_THREAD_FRAME;
      } else {
        mpCodec->thread_type &= ~(FF_THREAD_FRAME);
      }
      if (mpPersistMediaConfig->pb_decode.prefer_parallel_slice) {
        mpCodec->thread_type |= FF_THREAD_SLICE;
      } else {
        mpCodec->thread_type &= ~(FF_THREAD_SLICE);
      }
      LOGM_CONFIGURATION("decoder config: thread_count %d, thread_safe_callbacks %d, thread_type %x\n", mpCodec->thread_count, mpCodec->thread_safe_callbacks, mpCodec->thread_type);
    }
  }
  LOGM_NOTICE("avcodec_find_decoder(%d), %s\n", mCodecID, mpDecoder->name);
  mpCodec->opaque = (void *) mpBufferPool;
#if (LIBAVCODEC_VERSION_MAJOR < 57)
#ifndef BUILD_OS_WINDOWS
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
  if (AV_CODEC_ID_MJPEG == mCodecID) {
    mpCodec->get_buffer = _callback_get_buffer_yuv422_for_jpeg;
  } else {
    mpCodec->get_buffer = _callback_get_buffer;
  }
  mpCodec->release_buffer = _callback_release_buffer;
#ifndef BUILD_OS_WINDOWS
#pragma GCC diagnostic pop
#endif
#else
  if (AV_CODEC_ID_MJPEG == mCodecID) {
    mpCodec->get_buffer2 = _callback_get_buffer_yuv422_for_jpeg_v3;
  } else {
    mpCodec->get_buffer2 = _callback_get_buffer_v3;
  }
  mpCodec->draw_horiz_band = NULL;
#endif
  mpCodec->extradata = mpExtraData;
  mpCodec->extradata_size = mExtraDataSize;
  LOGM_CONFIGURATION("mpCodec->extradata %p, mpCodec->extradata_size %d\n", mpCodec->extradata, mpCodec->extradata_size);
  TInt rval = avcodec_open2(mpCodec, mpDecoder, NULL);
  if (DUnlikely(0 > rval)) {
    LOGM_ERROR("avcodec_open failed ret %d\n", rval);
    return EECode_Error;
  }
  LOGM_NOTICE("setupDecoder(%d), %s success.\n", mCodecID, mpDecoder->name);
  mbDecoderSetup = 1;
  return EECode_OK;
}

void CFFMpegVideoDecoder::destroyDecoder()
{
  if (mbDecoderSetup) {
    mbDecoderSetup = 0;
    avcodec_close(mpCodec);
    av_free(mpCodec);
    mpCodec = NULL;
    mpDecoder = NULL;
  }
}

#endif

