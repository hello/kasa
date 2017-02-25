/*******************************************************************************
 * am_video_input_linuxuvc.cpp
 *
 * History:
 *    2015/12/24 - [Zhi He] create file
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

#ifdef BUILD_MODULE_LINUX_UVC

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <signal.h>
#include <linux/videodev2.h>

#include "am_video_input_linuxuvc.h"

//#define D_USE_DIRECT_V4L2_BUFFER

//#ifdef D_USE_DIRECT_V4L2_BUFFER
static EECode __release_linuxuvc_v4l2_buffer(CIBuffer *pBuffer)
{
  DASSERT(pBuffer);
  if (pBuffer) {
    if (EBufferCustomizedContentType_V4l2DirectBuffer == pBuffer->mCustomizedContentType) {
      CVideoInputLinuxUVC *context = (CVideoInputLinuxUVC *)pBuffer->mpCustomizedContent;
      DASSERT(context);
      if (context) {
        context->ReleaseBuffer((TU32) (TPointer) pBuffer->GetDataPtrBase());
      }
    }
  } else {
    LOG_FATAL("NULL pBuffer!\n");
    return EECode_InternalLogicalBug;
  }
  return EECode_OK;
}
//#else
static EECode __alloc_memory_for_v4l2_frame(CIBuffer *p_buffer, TU32 size)
{
  TU8 *p = (TU8 *) DDBG_MALLOC(size, "VIVP");
  if (DUnlikely(!p)) {
    LOG_FATAL("NULL p\n");
    return EECode_NoMemory;
  }
  p_buffer->SetDataPtrBase(p, 0);
  p_buffer->SetDataPtrBase(NULL, 1);
  p_buffer->SetDataPtrBase(NULL, 2);
  p_buffer->SetDataMemorySize((TMemSize) size, 0);
  return EECode_OK;
}

static TU32 __is_jpeg_header_valid(TU8 *p)
{
  if ((!p)) {
    LOG_FATAL("bad params, %p\n", p);
    return 0;
  }
  if ((EJPEG_MarkerPrefix == p[0]) && (EJPEG_SOI == p[1])) {
    return 1;
  }
  return 0;
}

//#endif

static TU32 __skip_tailing_zero_bytes_for_jpeg(TU8 *p, TU32 size, TU32 &missed_EOI)
{
  if ((!p) || (!size) || (32 > size)) {
    LOG_FATAL("bad params, %p, %d\n", p, size);
    return 0;
  }
  missed_EOI = 0;
  TU8 *p_end = p + size - 1 - 4;
  if ((!p_end[4]) && (!p_end[3]) && (!p_end[2])) {
    if ((EJPEG_MarkerPrefix == p_end[0]) && (EJPEG_EOI == p_end[1])) {
      return 3;
    } else {
      missed_EOI = 1;
      //LOG_ERROR("do not find EOI\n");
      //gfPrintMemory(p + size - 16, 16);
    }
  } else if ((!p_end[4]) && (!p_end[3])) {
    if ((EJPEG_MarkerPrefix == p_end[1]) && (EJPEG_EOI == p_end[2])) {
      return 2;
    } else {
      missed_EOI = 1;
      //LOG_ERROR("do not find EOI\n");
      //gfPrintMemory(p + size - 16, 16);
    }
  } else if (!p_end[4]) {
    if ((EJPEG_MarkerPrefix == p_end[2]) && (EJPEG_EOI == p_end[3])) {
      return 1;
    } else {
      missed_EOI = 1;
      //LOG_ERROR("do not find EOI\n");
      //gfPrintMemory(p + size - 16, 16);
    }
  } else {
    if ((EJPEG_MarkerPrefix == p_end[3]) && (EJPEG_EOI == p_end[4])) {
      return 0;
    } else {
      missed_EOI = 1;
      //LOG_ERROR("do not find EOI\n");
      //gfPrintMemory(p + size - 16, 16);
    }
  }
  return 0;
}

IVideoInput *gfCreateVideoInputLinuxUVC(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
  return CVideoInputLinuxUVC::Create(pName, pPersistMediaConfig, pEngineMsgSink);
}

//-----------------------------------------------------------------------
//
// CVideoInputLinuxUVC
//
//-----------------------------------------------------------------------
IVideoInput *CVideoInputLinuxUVC::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
  CVideoInputLinuxUVC *result = new CVideoInputLinuxUVC(pName, pPersistMediaConfig, pEngineMsgSink);
  if (result && result->Construct() != EECode_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

void CVideoInputLinuxUVC::Destroy()
{
  Delete();
}

CVideoInputLinuxUVC::CVideoInputLinuxUVC(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
  : inherited(pName, 0)
  , mpPersistMediaConfig(pPersistMediaConfig)
  , mpMsgSink(pEngineMsgSink)
  , mpMutex(NULL)
  , mDeviceFd(-1)
  , mDeviceID(0)
  , mPreferedFormat(StreamFormat_PixelFormat_YUYV)
  , mPreferedCaptureWidth(640)
  , mPreferedCaptureHeight(480)
  , mPreferedCaptureFPS(30)
  , mCurFormat(StreamFormat_Invalid)
  , mCurCaptureWidth(0)
  , mCurCaptureHeight(0)
  , mCurCaptureFPS(0)
  , mCurFramerateNum(DDefaultVideoFramerateNum)
  , mCurFramerateDen(DDefaultVideoFramerateDen)
  , mCurFormatIndex(0)
  , mSupportFormatNumber(0)
  , mBufferNumber(3)
  , mbCompressedFormat(0)
  , mbSetup(0)
  , mbStartCapture(0)
  , mbSendSyncBuffer(0)
  , mbUseDirectBufferForJpeg(0)
  , mpBufferPool(NULL)
{
  TU32 i = 0;
  memset(&mSupportFormats, 0x0, sizeof(mSupportFormats));
  memset(&mCapability, 0x0, sizeof(mCapability));
  memset(&mV4l2Buffers, 0x0, sizeof(mV4l2Buffers));
  for (i = 0; i < DMAX_UVC_BUFFER_NUMBER; i ++) {
    mBuffers[i].p_base = NULL;
    mBuffers[i].length = 0;
    mBuffers[i].buffer_status = EV2l2BufferStatus_init;
    mBuffers[i].plane_offset[0] = 0;
    mBuffers[i].plane_offset[1] = 0;
    mBuffers[i].plane_offset[2] = 0;
    mBuffers[i].plane_offset[3] = 0;
  }
  for (i = 0; i < DMAX_UVC_SUPPORT_FORMAT_NUMBER; i ++) {
    mSupportVideoFormats[i] = StreamFormat_Invalid;
  }
  mPlaneNumber = 1;
}

EECode CVideoInputLinuxUVC::Construct()
{
  DSET_MODULE_LOG_CONFIG(ELogModuleVideoInput);
  mpMutex = gfCreateMutex();
  if (!mpMutex) {
    LOGM_FATAL("no memory\n");
    return EECode_NoMemory;
  }
  return EECode_OK;
}

CVideoInputLinuxUVC::~CVideoInputLinuxUVC()
{
  if (mpMutex) {
    mpMutex->Delete();
    mpMutex = NULL;
  }
}

EECode CVideoInputLinuxUVC::SetupContext(SVideoCaptureParams *param)
{
  if (DUnlikely(mbSetup)) {
    LOGM_ERROR("already setup\n");
    return EECode_OK;
  }
  DASSERT(param);
  if (StreamFormat_Invalid != param->format) {
    mPreferedFormat = param->format;
  }
  if (param->cap_win_width && param->cap_win_height) {
    mPreferedCaptureWidth = param->cap_win_width;
    mPreferedCaptureHeight = param->cap_win_height;
  }
  if (param->framerate_num && param->framerate_den) {
    mCurFramerateNum = param->framerate_num;
    mCurFramerateDen = param->framerate_den;
  }
  LOGM_CONFIGURATION("prefered width %d, height %d, fr_num %d, fr_den %d, format 0x%08x\n", mPreferedCaptureWidth, mPreferedCaptureHeight, mCurFramerateNum, mCurFramerateDen, mPreferedFormat);
  if (mCurFramerateNum && mCurFramerateDen) {
    mPreferedCaptureFPS = (mCurFramerateNum + (mCurFramerateDen >> 1)) / mCurFramerateDen;
    LOGM_CONFIGURATION("fps %d, num %d, den %d\n", mPreferedCaptureFPS, mCurFramerateNum, mCurFramerateDen);
  } else {
    LOGM_ERROR("not specify framerate num/den\n");
    return EECode_BadParam;
  }
  EECode err = openDevice(mDeviceID);
  if (DUnlikely(EECode_OK != err)) {
    LOGM_FATAL("openDevice fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
    return err;
  }
  err = queryCapability();
  if (DUnlikely(EECode_OK != err)) {
    LOGM_FATAL("queryCapability fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
    return err;
  }
  chooseFormat();
  err = setParameters();
  if (DUnlikely(EECode_OK != err)) {
    LOGM_FATAL("setParameters fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
    return err;
  }
  err = queueBuffers();
  if (DUnlikely(EECode_OK != err)) {
    LOGM_FATAL("queueBuffers fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
    return err;
  }
  mbSetup = 1;
  return EECode_OK;
}

void CVideoInputLinuxUVC::DestroyContext()
{
  if (mbSetup) {
    mpBufferPool->SetReleaseBufferCallBack(NULL);
    releaseBuffers();
    if (mbStartCapture) {
      endCapture();
      mbStartCapture = 0;
    }
    closeDevice();
    mbSetup = 0;
  }
}

void CVideoInputLinuxUVC::SetBufferPool(IBufferPool *buffer_pool)
{
  if (buffer_pool) {
    mpBufferPool = buffer_pool;
    //#ifdef D_USE_DIRECT_V4L2_BUFFER
    if (mbUseDirectBufferForJpeg || (StreamFormat_JPEG != mCurFormat)) {
      mpBufferPool->SetReleaseBufferCallBack(__release_linuxuvc_v4l2_buffer);
    }
    //#endif
  } else {
    LOGM_FATAL("NULL buffer pool\n");
  }
}

void CVideoInputLinuxUVC::ReleaseBuffer(TU32 index)
{
  AUTO_LOCK(mpMutex);
  if (DLikely(index < mBufferNumber)) {
    if (DLikely(EV2l2BufferStatus_used_by_app == mBuffers[index].buffer_status)) {
      DASSERT(index == (TU32) mV4l2Buffers[index].index);
      TInt ret = ioctl(mDeviceFd, VIDIOC_QBUF, &mV4l2Buffers[index]);
      if (DUnlikely(0 > ret)) {
        perror("VIDIOC_QBUF");
        LOGM_ERROR("VIDIOC_QBUF fail, errno %d\n", errno);
      }
      mBuffers[index].buffer_status = EV2l2BufferStatus_inside_driver;
    } else {
      LOGM_NOTICE("not expected buffer_status %d, buffer index %d\n", mBuffers[index].buffer_status, index);
    }
  } else {
    LOGM_FATAL("index exceed max value %d\n", index);
  }
}

EECode CVideoInputLinuxUVC::Capture(CIBuffer *buffer)
{
  if (DUnlikely(!buffer)) {
    LOG_FATAL("NULL buffer in CVideoInputLinuxUVC::Capture()\n");
    return EECode_BadParam;
  }
  if (DLikely(mbSetup)) {
    EECode err = EECode_OK;
    if (DUnlikely(!mbStartCapture)) {
      err = beginCapture();
      if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("beginCapture fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
      }
      mbStartCapture = 1;
    }
    if (StreamFormat_JPEG == mCurFormat) {
      err = captureJPEG(buffer);
    } else if ((StreamFormat_PixelFormat_YUYV == mCurFormat)
      || (StreamFormat_PixelFormat_NV12 == mCurFormat)
      || (StreamFormat_PixelFormat_YUV420p== mCurFormat)
      || (StreamFormat_PixelFormat_YVU420p == mCurFormat)) {
      err = captureYUV(buffer);
    } else {
      LOGM_FATAL("not supported format 0x%08x\n", mCurFormat);
      return EECode_NotSupported;
    }
    if (DUnlikely(EECode_OK != err)) {
      LOGM_ERROR("capture fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
      return err;
    }
  } else {
    LOGM_ERROR("not setup yet\n");
    return EECode_BadState;
  }
  return EECode_OK;
}

EECode CVideoInputLinuxUVC::openDevice(TInt uvc_id)
{
  if (DUnlikely(0 <= mDeviceFd)) {
    LOGM_WARN("close previous handle %d\n", mDeviceFd);
    close(mDeviceFd);
    mDeviceFd = -1;
  }
  TChar fs[32];
  snprintf(fs, sizeof(fs), "/dev/video%d", uvc_id);
  mDeviceFd = open(fs, O_RDWR);
  if (0 > mDeviceFd) {
    perror("Failed to open uvc");
    LOGM_ERROR("open [%s] fail, errno %d.\n", fs, errno);
    return EECode_IOError;
  }
  LOGM_NOTICE("open [%s] success\n", fs);
  return EECode_OK;
}

void CVideoInputLinuxUVC::closeDevice()
{
  if (DLikely(0 <= mDeviceFd)) {
    close(mDeviceFd);
    mDeviceFd = -1;
  }
}

EECode CVideoInputLinuxUVC::queryCapability()
{
  TInt ret = 0;
  mSupportFormatNumber = 0;
  do {
    if (DMAX_UVC_SUPPORT_FORMAT_NUMBER <= mSupportFormatNumber) {
      LOGM_FATAL("too many (%d) formats\n", mSupportFormatNumber);
      return EECode_TooMany;
    }
    memset(&mSupportFormats[mSupportFormatNumber], 0, sizeof(struct v4l2_fmtdesc));
    mSupportFormats[mSupportFormatNumber].index = mSupportFormatNumber;
    mSupportFormats[mSupportFormatNumber].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(mDeviceFd, VIDIOC_ENUM_FMT, &mSupportFormats[mSupportFormatNumber]);
    if (!ret) {
      LOGM_CONFIGURATION("support pixelformat = %c%c%c%c, description = [%s]\n",
                         mSupportFormats[mSupportFormatNumber].pixelformat & 0xff, (mSupportFormats[mSupportFormatNumber].pixelformat >> 8) & 0xff,
                         (mSupportFormats[mSupportFormatNumber].pixelformat >> 16) & 0xff, (mSupportFormats[mSupportFormatNumber].pixelformat >> 24) & 0xff,
                         mSupportFormats[mSupportFormatNumber].description);
      if ((DFOURCC('M', 'J', 'P', 'G')) == mSupportFormats[mSupportFormatNumber].pixelformat) {
        mSupportVideoFormats[mSupportFormatNumber] = StreamFormat_JPEG;
      } else if ((DFOURCC('N', 'V', '1', '2')) == mSupportFormats[mSupportFormatNumber].pixelformat) {
        mSupportVideoFormats[mSupportFormatNumber] = StreamFormat_PixelFormat_NV12;
      } else if ((DFOURCC('Y', 'U', 'Y', 'V')) == mSupportFormats[mSupportFormatNumber].pixelformat) {
        mSupportVideoFormats[mSupportFormatNumber] = StreamFormat_PixelFormat_YUYV;
      } else if ((DFOURCC('Y', 'U', '1', '2')) == mSupportFormats[mSupportFormatNumber].pixelformat) {
        mSupportVideoFormats[mSupportFormatNumber] = StreamFormat_PixelFormat_YUV420p;
      } else if ((DFOURCC('Y', 'V', '1', '2')) == mSupportFormats[mSupportFormatNumber].pixelformat) {
        mSupportVideoFormats[mSupportFormatNumber] = StreamFormat_PixelFormat_YVU420p;
      } else {
        mSupportVideoFormats[mSupportFormatNumber] = StreamFormat_Invalid;
        LOGM_NOTICE("do not support this pixel format\n");
      }
      mSupportFormatNumber ++;
    } else {
      break;
    }
  } while (1);
  if (DUnlikely(!mSupportFormatNumber)) {
    LOGM_ERROR("zero number?\n");
    return EECode_NotExist;
  }
  memset(&mCapability, 0x0, sizeof(mCapability));
  ret = ioctl(mDeviceFd, VIDIOC_QUERYCAP, &mCapability);
  if (0 > ret) {
    perror("VIDIOC_QUERYCAP");
    LOGM_ERROR("VIDIOC_QUERYCAP fail, errno %d\n", errno);
    return EECode_OSError;
  }
  if (!(mCapability.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    LOGM_ERROR("Current device is not a video capture device\n");
    return EECode_NotExist;
  }
  if (!(mCapability.capabilities & V4L2_CAP_STREAMING)) {
    LOGM_ERROR("Current device does not support streaming i/o\n");
    return EECode_NotExist;
  }
  return EECode_OK;
}

void CVideoInputLinuxUVC::chooseFormat()
{
  mCurCaptureWidth = mPreferedCaptureWidth;
  mCurCaptureHeight = mPreferedCaptureHeight;
  mCurCaptureFPS = mPreferedCaptureFPS;
  for (mCurFormatIndex = 0; mCurFormatIndex < mSupportFormatNumber; mCurFormatIndex ++) {
    if (mPreferedFormat == mSupportVideoFormats[mCurFormatIndex]) {
      mCurFormat = mSupportVideoFormats[mCurFormatIndex];
      if (StreamFormat_JPEG == mCurFormat) {
        mbCompressedFormat = 1;
        mPlaneNumber = 1;
      } else {
        mbCompressedFormat = 0;
        if (StreamFormat_PixelFormat_NV12 == mCurFormat) {
          mPlaneNumber = 2;
        } else if (StreamFormat_PixelFormat_YUYV == mCurFormat) {
          mPlaneNumber = 1;
        } else if (StreamFormat_PixelFormat_YUV420p == mCurFormat) {
          mPlaneNumber = 3;
        } else if (StreamFormat_PixelFormat_YVU420p == mCurFormat) {
          mPlaneNumber = 3;
        } else {
          LOGM_FATAL("which format 0x%08x?\n", mCurFormat);
          return;
        }
      }
      LOGM_CONFIGURATION("get prefered format 0x%x, plane number %d, width %d, height %d, fps %d, buffer number %d\n", mCurFormat, mPlaneNumber, mCurCaptureWidth, mCurCaptureHeight, mCurCaptureFPS, mBufferNumber);
      return;
    }
  }
  for (mCurFormatIndex = 0; mCurFormatIndex < mSupportFormatNumber; mCurFormatIndex ++) {
    if (StreamFormat_JPEG == mSupportVideoFormats[mCurFormatIndex]) {
      mCurFormat = StreamFormat_JPEG;
      mbCompressedFormat = 1;
      mPlaneNumber = 1;
      break;
    } else if (StreamFormat_PixelFormat_NV12 == mSupportVideoFormats[mCurFormatIndex]) {
      mCurFormat = StreamFormat_PixelFormat_NV12;
      mbCompressedFormat = 0;
      mPlaneNumber = 2;
      break;
    } else if (StreamFormat_PixelFormat_YUYV == mSupportVideoFormats[mCurFormatIndex]) {
      mCurFormat = StreamFormat_PixelFormat_YUYV;
      mbCompressedFormat = 0;
      mPlaneNumber = 1;
      break;
    } else if (StreamFormat_PixelFormat_YUV420p == mSupportVideoFormats[mCurFormatIndex]) {
      mCurFormat = StreamFormat_PixelFormat_YUV420p;
      mbCompressedFormat = 0;
      mPlaneNumber = 3;
      break;
    } else if (StreamFormat_PixelFormat_YVU420p == mSupportVideoFormats[mCurFormatIndex]) {
      mCurFormat = StreamFormat_PixelFormat_YVU420p;
      mbCompressedFormat = 0;
      mPlaneNumber = 3;
      break;
    }
  }
  if (mCurFormatIndex == mSupportFormatNumber) {
    LOGM_WARN("do not find supported format, use first one\n");
    mCurFormatIndex = 0;
    mCurFormat = mSupportVideoFormats[0];
  } else {
    LOGM_CONFIGURATION("select support format 0x%x, plane number %d, width %d, height %d, fps %d, buffer number %d\n", mCurFormat, mPlaneNumber, mCurCaptureWidth, mCurCaptureHeight, mCurCaptureFPS, mBufferNumber);
    return;
  }
  LOGM_WARN("do not find matched format, choose first avaiable format 0x%x, width %d, height %d, fps %d, buffer number %d\n", mCurFormat, mCurCaptureWidth, mCurCaptureHeight, mCurCaptureFPS, mBufferNumber);
}

EECode CVideoInputLinuxUVC::prepareBuffers()
{
  TU32 i = 0, j = 0;
  TInt ret = 0;
  struct v4l2_requestbuffers reqbuf;
  DASSERT(mBufferNumber < DMAX_UVC_BUFFER_NUMBER);
  DASSERT(mBufferNumber > 1);
  memset(&reqbuf, 0, sizeof(reqbuf));
  reqbuf.count = mBufferNumber;
  reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  reqbuf.memory = V4L2_MEMORY_MMAP;
  ret = ioctl(mDeviceFd, VIDIOC_REQBUFS, &reqbuf);
  if (DUnlikely(0 > ret)) {
    perror("VIDIOC_REQBUFS");
    LOGM_ERROR("VIDIOC_REQBUFS fail, errno %d\n", errno);
    return EECode_OSError;
  }
  mBufferNumber = reqbuf.count;
  if (DMAX_UVC_BUFFER_NUMBER <= mBufferNumber) {
    LOGM_FATAL("buffer number excced max value %d\n", mBufferNumber);
    return EECode_BadFormat;
  }
  LOGM_CONFIGURATION("buffer number %d\n", mBufferNumber);
  for (i = 0; i < mBufferNumber; i++) {
    memset(&mV4l2Buffers[i], 0, sizeof(mV4l2Buffers[i]));
    mV4l2Buffers[i].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mV4l2Buffers[i].memory = V4L2_MEMORY_MMAP;
    mV4l2Buffers[i].index = i;
    ret = ioctl(mDeviceFd, VIDIOC_QUERYBUF, &mV4l2Buffers[i]);
    if (0 > ret) {
      perror("VIDIOC_QUERYBUF");
      LOGM_ERROR("VIDIOC_QUERYBUF fail, errno %d\n", errno);
      return EECode_OSError;
    }
    LOGM_CONFIGURATION("buffer %d, length %d, offset 0x%08x\n", i, mV4l2Buffers[i].length, mV4l2Buffers[i].m.offset);
    mBuffers[i].length = mV4l2Buffers[i].length;
    mBuffers[i].p_base = (TU8 *) mmap(NULL, mV4l2Buffers[i].length, PROT_READ /*| PROT_WRITE*/, MAP_SHARED, mDeviceFd, mV4l2Buffers[i].m.offset);
    LOGM_CONFIGURATION("mmap done, buffer %d, %p\n", i, mBuffers[i].p_base);
    if (MAP_FAILED == mBuffers[i].p_base) {
      perror("mmap\n");
      LOGM_ERROR("mmap fail, index %d, errno %d\n", i, errno);
      return EECode_OSError;
    }
    if ((1 < mPlaneNumber) && (4 > mPlaneNumber)) {
      //planes should be struct's pointer, but there's issue with some YU12 device, disable it
      if (0 && (mV4l2Buffers[i].m.planes)) {
        for (j = 0; j < mPlaneNumber; j ++) {
          mBuffers[i].plane_offset[j] = mV4l2Buffers[i].m.planes[j].m.mem_offset;
        }
        DASSERT(mBuffers[i].plane_offset[0] == 0);
      } else {
        //LOGM_WARN("YUV buffer have multiple plane, but v4l buffer's plane is NULL, guess each plane's offset, buffer %dx%d\n", mCurCaptureWidth, mCurCaptureHeight);
        if ((StreamFormat_PixelFormat_YUV420p == mCurFormat) || (StreamFormat_PixelFormat_YVU420p == mCurFormat)) {
          DASSERT(3 == mPlaneNumber);
          mBuffers[i].plane_offset[0] = 0;
          mBuffers[i].plane_offset[1] = mCurCaptureWidth * mCurCaptureHeight;
          mBuffers[i].plane_offset[2] = mBuffers[i].plane_offset[1] + ((mCurCaptureWidth * mCurCaptureHeight) / 4);
        } else if (StreamFormat_PixelFormat_NV12 == mCurFormat) {
          DASSERT(2 == mPlaneNumber);
          mBuffers[i].plane_offset[0] = 0;
          mBuffers[i].plane_offset[1] = mCurCaptureWidth * mCurCaptureHeight;
        } else {
          LOGM_FATAL("bad format 0x%08x\n", mCurFormat);
        }
      }
    } else if (1 != mPlaneNumber) {
      LOGM_FATAL("plane number (%d) not valid\n", mPlaneNumber);
    }
  }
  return EECode_OK;
}

EECode CVideoInputLinuxUVC::queueBuffers()
{
  TU32 i = 0;
  TInt ret = 0;
  for (i = 0; i < mBufferNumber; i++) {
    memset(&mV4l2Buffers[i], 0, sizeof(mV4l2Buffers[i]));
    mV4l2Buffers[i].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mV4l2Buffers[i].memory = V4L2_MEMORY_MMAP;
    mV4l2Buffers[i].index = i;
    //DASSERT(mV4l2Buffers[i].index == i);
    //LOGM_CONFIGURATION("before VIDIOC_QBUF %d\n", i);
    ret = ioctl(mDeviceFd, VIDIOC_QBUF, &mV4l2Buffers[i]);
    if (0 > ret) {
      perror("VIDIOC_QBUF");
      LOGM_ERROR("VIDIOC_QBUF fail, index %d, errno %d\n", i, errno);
      return EECode_OSError;
    }
    //LOGM_CONFIGURATION("after VIDIOC_QBUF %d\n", i);
    mBuffers[i].buffer_status = EV2l2BufferStatus_inside_driver;
  }
  return EECode_OK;
}

void CVideoInputLinuxUVC::releaseBuffers()
{
  TU32 i = 0;
  AUTO_LOCK(mpMutex);
  for (i = 0; i < mBufferNumber; i++) {
    if (EV2l2BufferStatus_used_by_app == mBuffers[i].buffer_status) {
      //LOGM_NOTICE("releaseBuffers, before VIDIOC_QBUF %d\n", i);
      TInt ret = ioctl(mDeviceFd, VIDIOC_QBUF, &mV4l2Buffers[i]);
      if (DUnlikely(0 > ret)) {
        perror("VIDIOC_QBUF");
        LOGM_ERROR("VIDIOC_QBUF fail, errno %d\n", errno);
      }
      //LOGM_NOTICE("releaseBuffers, after VIDIOC_QBUF %d\n", i);
      mBuffers[i].buffer_status = EV2l2BufferStatus_inside_driver;
    }
  }
  for (i = 0; i < mBufferNumber; i++) {
    if (mBuffers[i].p_base && mBuffers[i].length) {
      //LOGM_NOTICE("munmap %d, mBuffers[i].p_base %p\n", i, mBuffers[i].p_base);
      if (munmap(mBuffers[i].p_base, mBuffers[i].length) < 0) {
        perror("munmap");
        LOGM_ERROR("munmap fail, errno %d\n", errno);
      }
      mBuffers[i].p_base = NULL;
      mBuffers[i].length = 0;
      mBuffers[i].buffer_status = EV2l2BufferStatus_init;
    }
  }
  return;
}

EECode CVideoInputLinuxUVC::beginCapture()
{
  TInt ret = 0;
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  //LOGM_NOTICE("before VIDIOC_STREAMON\n");
  ret = ioctl(mDeviceFd, VIDIOC_STREAMON, &type);
  if (DUnlikely(0 > ret)) {
    perror("VIDIOC_STREAMON");
    LOGM_ERROR("VIDIOC_STREAMON fail, errno %d\n", errno);
    return EECode_OSError;
  }
  //LOGM_NOTICE("after VIDIOC_STREAMON\n");
  return EECode_OK;
}

EECode CVideoInputLinuxUVC::endCapture()
{
  TInt ret = 0;
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  //LOGM_NOTICE("before VIDIOC_STREAMOFF\n");
  ret = ioctl(mDeviceFd, VIDIOC_STREAMOFF, &type);
  if (0 > ret) {
    perror("VIDIOC_STREAMOFF");
    LOGM_ERROR("VIDIOC_STREAMOFF fail, errno %d\n", errno);
    return EECode_OSError;
  }
  //LOGM_NOTICE("after VIDIOC_STREAMOFF\n");
  return EECode_OK;
}

EECode CVideoInputLinuxUVC::setParameters()
{
  TInt ret = 0;
  struct v4l2_format stream_fmt;
  struct v4l2_streamparm stream;
  memset(&stream_fmt, 0 , sizeof(stream_fmt));
  stream_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  stream_fmt.fmt.pix.width = mCurCaptureWidth;
  stream_fmt.fmt.pix.height = mCurCaptureHeight;
  if (StreamFormat_JPEG == mCurFormat) {
    stream_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    mbCompressedFormat = 1;
  } else if (StreamFormat_PixelFormat_NV12 == mCurFormat) {
    stream_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
    mbCompressedFormat = 0;
  } else if (StreamFormat_PixelFormat_YUYV == mCurFormat) {
    stream_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    mbCompressedFormat = 0;
  } else if (StreamFormat_PixelFormat_YUV420p == mCurFormat) {
    stream_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
    mbCompressedFormat = 0;
  } else if (StreamFormat_PixelFormat_YVU420p == mCurFormat) {
    stream_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YVU420;
    mbCompressedFormat = 0;
  } else {
    LOGM_ERROR("not support %x\n", mCurFormat);
    return EECode_NotSupported;
  }
  stream_fmt.fmt.pix.field = V4L2_FIELD_NONE; //V4L2_FIELD_INTERLACED;
  ret = ioctl(mDeviceFd, VIDIOC_S_FMT, &stream_fmt);
  if (DUnlikely(0 > ret)) {
    perror("VIDIOC_S_FMT");
    LOGM_ERROR("VIDIOC_S_FMT fail, errno %d\n", errno);
    return EECode_OSError;
  }
  ret = ioctl(mDeviceFd, VIDIOC_G_FMT, &stream_fmt);
  if (DUnlikely(0 > ret)) {
    perror("VIDIOC_G_FMT");
    LOGM_ERROR("VIDIOC_G_FMT fail, errno %d\n", errno);
    return EECode_OSError;
  }
  LOGM_CONFIGURATION("read format:\nfmt.type:\t\t%d\n", stream_fmt.type);
  LOGM_CONFIGURATION("pix.pixelformat:\t%c%c%c%c\n",
                     stream_fmt.fmt.pix.pixelformat & 0xFF,
                     (stream_fmt.fmt.pix.pixelformat >> 8) & 0xFF,
                     (stream_fmt.fmt.pix.pixelformat >> 16) & 0xFF,
                     (stream_fmt.fmt.pix.pixelformat >> 24) & 0xFF);
  LOGM_CONFIGURATION("pix.height:\t\t%d\n", stream_fmt.fmt.pix.height);
  LOGM_CONFIGURATION("pix.width:\t\t%d\n", stream_fmt.fmt.pix.width);
  LOGM_CONFIGURATION("pix.field:\t\t%d\n", stream_fmt.fmt.pix.field);
  LOGM_CONFIGURATION("fps:\t\t\t%d\n", mCurCaptureFPS);
  if ((mCurCaptureWidth != stream_fmt.fmt.pix.width) || (mCurCaptureHeight != stream_fmt.fmt.pix.height)) {
    mCurCaptureWidth = stream_fmt.fmt.pix.width;
    mCurCaptureHeight = stream_fmt.fmt.pix.height;
    LOGM_WARN("Change size: width=%d, height=%d.\n", mCurCaptureWidth, mCurCaptureHeight);
  }
  EECode err = prepareBuffers();
  if (DUnlikely(EECode_OK != err)) {
    LOGM_FATAL("prepareBuffers fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
    return err;
  }
  memset(&stream, 0 , sizeof(stream));
  stream.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  stream.parm.capture.capturemode = 0;
  stream.parm.capture.timeperframe.numerator = 1;
  DASSERT(mCurCaptureFPS);
  stream.parm.capture.timeperframe.denominator = mCurCaptureFPS;
  //LOGM_NOTICE("before VIDIOC_S_PARM\n");
  ret = ioctl(mDeviceFd, VIDIOC_S_PARM, &stream);
  if (0 > ret) {
    perror("VIDIOC_S_PARM");
    LOGM_ERROR("VIDIOC_S_PARM fail, errno %d\n", errno);
    return EECode_OSError;
  }
  return EECode_OK;
}

EECode CVideoInputLinuxUVC::captureJPEG(CIBuffer *buffer)
{
  TInt ret = 0;
  struct v4l2_buffer vbuf;
  memset(&vbuf, 0, sizeof(vbuf));
  vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  vbuf.memory = V4L2_MEMORY_MMAP;
  ret = ioctl(mDeviceFd, VIDIOC_DQBUF, &vbuf);
  if (0 > ret) {
    perror("VIDIOC_DQBUF");
    LOGM_ERROR("VIDIOC_DQBUF fail, errno %d\n", errno);
    return EECode_OSError;
  }
  if (DLikely(vbuf.index < mBufferNumber)) {
    TU32 tail_size = 0;
    TU32 missed_EOI = 0;
    msync(mBuffers[vbuf.index].p_base, vbuf.bytesused, MS_SYNC);
    if (mbUseDirectBufferForJpeg) {
      if (StreamFormat_JPEG == mCurFormat) {
        tail_size = __skip_tailing_zero_bytes_for_jpeg(mBuffers[vbuf.index].p_base, (TU32) vbuf.bytesused, missed_EOI);
      }
      buffer->mCustomizedContentType = EBufferCustomizedContentType_V4l2DirectBuffer;
      buffer->mpCustomizedContent = (void *) this;
      buffer->SetDataPtrBase((TU8 *) (TPointer) (vbuf.index));
      buffer->SetDataPtr(mBuffers[vbuf.index].p_base);
      buffer->SetDataSize((TMemSize) vbuf.bytesused - tail_size);
    } else {
      TU32 size = 0;
      EECode err;
      if ((!buffer->mbAllocated) || (!buffer->GetDataPtrBase(0))) {
        size = vbuf.bytesused + 4096;
        if (size < (192 * 1024)) {
          size = 192 * 1024;
        }
        err = __alloc_memory_for_v4l2_frame(buffer, size);
        if (DUnlikely(EECode_OK != err)) {
          LOGM_FATAL("__alloc_memory_for_v4l2_frame fail, ret 0x%x, %s\n", err, gfGetErrorCodeString(err));
          return err;
        }
        buffer->mbAllocated = 1;
      } else if (((TU32) vbuf.bytesused + 32) > buffer->GetDataMemorySize(0)) {
        TU8 *pbuf = buffer->GetDataPtrBase(0);
        if (pbuf) {
          DDBG_FREE(pbuf, "VIVP");
          pbuf = NULL;
        }
        buffer->mbAllocated = 0;
        buffer->SetDataPtrBase(NULL, 0);
        size = vbuf.bytesused + 4096;
        err = __alloc_memory_for_v4l2_frame(buffer, size);
        if (DUnlikely(EECode_OK != err)) {
          LOGM_FATAL("__alloc_memory_for_v4l2_frame fail, ret 0x%x, %s\n", err, gfGetErrorCodeString(err));
          return err;
        }
        buffer->mbAllocated = 1;
      }
      size = vbuf.bytesused;
      memcpy(buffer->GetDataPtrBase(0), mBuffers[vbuf.index].p_base, size);
      buffer->SetDataPtr(buffer->GetDataPtrBase(0), 0);
      buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;
      buffer->mpCustomizedContent = NULL;
      if (!__is_jpeg_header_valid(buffer->GetDataPtr(0))) {
        LOGM_WARN("no soi, data corruption\n");
        return EECode_DataCorruption;
      }
      if (StreamFormat_JPEG == mCurFormat) {
        tail_size = __skip_tailing_zero_bytes_for_jpeg(buffer->GetDataPtr(0), size, missed_EOI);
      }
      if (1 || (!missed_EOI)) {
          buffer->SetDataSize((TMemSize) vbuf.bytesused - tail_size);
      } else {
          TU8 *pend = buffer->GetDataPtr(0) + vbuf.bytesused;
          pend[0] = EJPEG_MarkerPrefix;
          pend[1] = EJPEG_EOI;
          buffer->SetDataSize((TMemSize) vbuf.bytesused + 2);
      }
      mBuffers[vbuf.index].buffer_status = EV2l2BufferStatus_used_by_app;
      ReleaseBuffer(vbuf.index);
    }
    buffer->mbDataSegment = 0;
    buffer->mVideoWidth = mCurCaptureWidth;
    buffer->mVideoHeight = mCurCaptureHeight;
    buffer->mVideoFrameRateNum = mCurFramerateNum;
    buffer->mVideoFrameRateDen = mCurFramerateDen;
    buffer->mContentFormat = mCurFormat;
    buffer->SetBufferFlags(CIBuffer::KEY_FRAME);
    if (DUnlikely(!mbSendSyncBuffer)) {
      buffer->SetBufferFlagBits(CIBuffer::SYNC_POINT);
      mbSendSyncBuffer = 1;
    }
    buffer->mVideoFrameType = EPredefinedPictureType_IDR;
    buffer->SetBufferType(EBufferType_VideoES);
    if (mbUseDirectBufferForJpeg) {
      AUTO_LOCK(mpMutex);
      if (DUnlikely(EV2l2BufferStatus_inside_driver != mBuffers[vbuf.index].buffer_status)) {
        LOGM_ERROR("BAD buffer_status %d, buffer index %d\n", mBuffers[vbuf.index].buffer_status, vbuf.index);
      }
      mBuffers[vbuf.index].buffer_status = EV2l2BufferStatus_used_by_app;
    }
  } else {
    LOGM_FATAL("buffer index(%d) exceed max value\n", vbuf.index);
    return EECode_InternalLogicalBug;
  }
  return EECode_OK;
}

EECode CVideoInputLinuxUVC::captureYUV(CIBuffer *buffer)
{
  TInt ret = 0;
  struct v4l2_buffer vbuf;
  memset(&vbuf, 0, sizeof(vbuf));
  vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  vbuf.memory = V4L2_MEMORY_MMAP;
  ret = ioctl(mDeviceFd, VIDIOC_DQBUF, &vbuf);
  if (0 > ret) {
    perror("VIDIOC_DQBUF");
    LOGM_ERROR("VIDIOC_DQBUF fail, errno %d\n", errno);
    return EECode_OSError;
  }
  DASSERT(!mbCompressedFormat);
  if (DLikely(vbuf.index < mBufferNumber)) {
    msync(mBuffers[vbuf.index].p_base, vbuf.bytesused, MS_SYNC);
    buffer->mCustomizedContentType = EBufferCustomizedContentType_V4l2DirectBuffer;
    buffer->mpCustomizedContent = (void *) this;
    buffer->SetDataPtrBase((TU8 *) (TPointer) (vbuf.index));
    buffer->SetDataSize((TMemSize) vbuf.bytesused);
    if (StreamFormat_PixelFormat_YUYV == mCurFormat) {
      buffer->SetDataLineSize(mCurCaptureWidth * 2);
      buffer->SetDataPtr(mBuffers[vbuf.index].p_base, 0);
    } else if (StreamFormat_PixelFormat_NV12 == mCurFormat) {
      buffer->SetDataLineSize(mCurCaptureWidth, 0);
      buffer->SetDataLineSize(mCurCaptureWidth, 1);
      buffer->SetDataPtr(mBuffers[vbuf.index].p_base, 0);
      buffer->SetDataPtr(mBuffers[vbuf.index].p_base + mBuffers[vbuf.index].plane_offset[1], 1);
    }  else if (StreamFormat_PixelFormat_YUV420p == mCurFormat) {
      buffer->SetDataLineSize(mCurCaptureWidth, 0);
      buffer->SetDataLineSize(mCurCaptureWidth / 2, 1);
      buffer->SetDataLineSize(mCurCaptureWidth / 2, 2);
      buffer->SetDataPtr(mBuffers[vbuf.index].p_base, 0);
      buffer->SetDataPtr(mBuffers[vbuf.index].p_base + mBuffers[vbuf.index].plane_offset[1], 1);
      buffer->SetDataPtr(mBuffers[vbuf.index].p_base + mBuffers[vbuf.index].plane_offset[2], 2);
    } else if (StreamFormat_PixelFormat_YVU420p == mCurFormat) {
      buffer->SetDataLineSize(mCurCaptureWidth, 0);
      buffer->SetDataLineSize(mCurCaptureWidth / 2, 1);
      buffer->SetDataLineSize(mCurCaptureWidth / 2, 2);
      buffer->SetDataPtr(mBuffers[vbuf.index].p_base, 0);
      buffer->SetDataPtr(mBuffers[vbuf.index].p_base + mBuffers[vbuf.index].plane_offset[1], 1);
      buffer->SetDataPtr(mBuffers[vbuf.index].p_base + mBuffers[vbuf.index].plane_offset[2], 2);
    } else {
      LOGM_ERROR("not support %x\n", mCurFormat);
      return EECode_NotSupported;
    }
    buffer->mbDataSegment = 0;
    buffer->mVideoWidth = mCurCaptureWidth;
    buffer->mVideoHeight = mCurCaptureHeight;
    buffer->mVideoFrameRateNum = mCurFramerateNum;
    buffer->mVideoFrameRateDen = mCurFramerateDen;
    if (StreamFormat_PixelFormat_YVU420p != mCurFormat) {
      buffer->mContentFormat = mCurFormat;
    } else {
      TU8 * p_swap = buffer->GetDataPtr(1);
      TU32 linesize_swap = buffer->GetDataLineSize(1);
      buffer->mContentFormat = StreamFormat_PixelFormat_YUV420p;
      buffer->SetDataPtr(buffer->GetDataPtr(2), 1);
      buffer->SetDataLineSize(buffer->GetDataLineSize(2), 1);
      buffer->SetDataPtr(p_swap, 2);
      buffer->SetDataLineSize(linesize_swap, 2);
    }
    buffer->SetBufferFlags(CIBuffer::KEY_FRAME);
    if (DUnlikely(!mbSendSyncBuffer)) {
      buffer->SetBufferFlagBits(CIBuffer::SYNC_POINT);
      mbSendSyncBuffer = 1;
    }
    buffer->SetBufferType(EBufferType_VideoFrameBuffer);
    {
      AUTO_LOCK(mpMutex);
      if (DUnlikely(EV2l2BufferStatus_inside_driver != mBuffers[vbuf.index].buffer_status)) {
        LOGM_ERROR("BAD buffer_status %d, buffer index %d\n", mBuffers[vbuf.index].buffer_status, vbuf.index);
      }
      mBuffers[vbuf.index].buffer_status = EV2l2BufferStatus_used_by_app;
    }
  } else {
    LOGM_FATAL("buffer index(%d) exceed max value\n", vbuf.index);
    return EECode_InternalLogicalBug;
  }
  return EECode_OK;
}

#endif

