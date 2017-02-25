/*******************************************************************************
 * am_video_input_linuxuvc.h
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

#ifndef __AM_VIDEO_INPUT_LINUXUVC_H__
#define __AM_VIDEO_INPUT_LINUXUVC_H__

#define DMAX_UVC_SUPPORT_FORMAT_NUMBER 16
#define DMAX_UVC_BUFFER_NUMBER 8

enum {
  EV2l2BufferStatus_init = 0x0,
  EV2l2BufferStatus_inside_driver = 0x01,
  EV2l2BufferStatus_used_by_app = 0x02,
};

typedef struct {
  TU8 *p_base;
  TU32 length;

  TU32 buffer_status;

  TU32 plane_offset[4];
} SV4L2Buffer;

class CVideoInputLinuxUVC: public CObject, public IVideoInput
{
  typedef CObject inherited;

protected:
  CVideoInputLinuxUVC(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
  EECode Construct();
  virtual ~CVideoInputLinuxUVC();

public:
  static IVideoInput *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
  virtual void Destroy();

public:
  virtual EECode SetupContext(SVideoCaptureParams *param);
  virtual void DestroyContext();
  virtual void SetBufferPool(IBufferPool *buffer_pool);
  virtual void ReleaseBuffer(TU32 index);
  virtual EECode Capture(CIBuffer *buffer);

private:
  EECode openDevice(TInt uvc_id);
  void closeDevice();
  EECode queryCapability();
  void chooseFormat();
  EECode prepareBuffers();
  EECode queueBuffers();
  void releaseBuffers();
  EECode beginCapture();
  EECode endCapture();
  EECode setParameters();
  EECode captureJPEG(CIBuffer *buffer);
  EECode captureYUV(CIBuffer *buffer);

private:
  const volatile SPersistMediaConfig *mpPersistMediaConfig;
  IMsgSink *mpMsgSink;
  IMutex *mpMutex;

private:
  TInt mDeviceFd;
  TInt mDeviceID;

private:
  struct v4l2_fmtdesc mSupportFormats[DMAX_UVC_SUPPORT_FORMAT_NUMBER];
  struct v4l2_capability mCapability;
  StreamFormat mSupportVideoFormats[DMAX_UVC_SUPPORT_FORMAT_NUMBER];

private:
  StreamFormat mPreferedFormat;
  TU32 mPreferedCaptureWidth;
  TU32 mPreferedCaptureHeight;
  TU32 mPreferedCaptureFPS;

private:
  StreamFormat mCurFormat;
  TU32 mCurCaptureWidth;
  TU32 mCurCaptureHeight;
  TU32 mCurCaptureFPS;
  TU32 mCurFramerateNum;
  TU32 mCurFramerateDen;

private:
  TU8 mCurFormatIndex;
  TU8 mSupportFormatNumber;
  TU8 mBufferNumber;
  TU8 mbCompressedFormat;

  TU8 mbSetup;
  TU8 mbStartCapture;
  TU8 mbSendSyncBuffer;
  TU8 mbUseDirectBufferForJpeg;

  TU8 mPlaneNumber;
  TU8 mReserved0;
  TU8 mReserved1;
  TU8 mReserved2;

private:
  SV4L2Buffer mBuffers[DMAX_UVC_BUFFER_NUMBER];
  struct v4l2_buffer mV4l2Buffers[DMAX_UVC_BUFFER_NUMBER];
  IBufferPool *mpBufferPool;
};

#endif

