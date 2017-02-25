/**
 * am_linuxfb_video_renderer.h
 *
 * History:
 *    2015/09/14 - [Zhi He] create file
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

#ifndef __AM_LINUXFB_VIDEO_RENDERER_H__
#define __AM_LINUXFB_VIDEO_RENDERER_H__

class CLinuxFBVideoRenderer
  : public CObject
  , public IVideoRenderer
{
  typedef CObject inherited;

protected:
  CLinuxFBVideoRenderer(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
  virtual ~CLinuxFBVideoRenderer();

public:
  static IVideoRenderer *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
  virtual void Destroy();

public:
  virtual EECode SetupContext(SVideoParams *param = NULL);
  virtual EECode DestroyContext();

  virtual EECode Start(TUint index = 0);
  virtual EECode Stop(TUint index = 0);
  virtual EECode Flush(TUint index = 0);

  virtual EECode Render(CIBuffer *p_buffer, TUint index = 0);

  virtual EECode Pause(TUint index = 0);
  virtual EECode Resume(TUint index = 0);
  virtual EECode StepPlay(TUint index = 0);

  virtual EECode SyncTo(TTime pts, TUint index = 0);
  virtual EECode QueryLastShownTimeStamp(TTime &pts, TUint index = 0);

  virtual TU32 IsMonitorMode(TUint index = 0) const;
  virtual EECode WaitReady(TUint index = 0);
  virtual EECode Wakeup(TUint index = 0);
  virtual EECode WaitEosMsg(TTime &last_pts, TUint index = 0);
  virtual EECode WaitLastShownTimeStamp(TTime &last_pts, TUint index = 0);

private:
  EECode Construct();

private:
  void yuv420p422p_to_vyu565(CIBuffer *p_buffer);
  void yuv420p422p_to_ayuv8888(CIBuffer *p_buffer);
  void yuyv_to_vyu565(CIBuffer *p_buffer);
  void yuyv_to_ayuv8888(CIBuffer *p_buffer);
  void grey8_to_vyu565(CIBuffer *p_buffer);
  void grey8_to_ayuv8888(CIBuffer *p_buffer);

private:
  const volatile SPersistMediaConfig *mpPersistMediaConfig;
  IMsgSink *mpMsgSink;

private:
  TInt mFd;
  TU32 mWidth;
  TU32 mHeight;
  TU32 mLinesize;
  TU32 mBufferSize;
  TU32 mBitsPerPixel;
  TU8 *mpBuffers;
  TU8 *mpBuffer[4];

  TU32 mSrcOffsetX;
  TU32 mSrcOffsetY;
  TU32 mSrcWidth;
  TU32 mSrcHeight;
  TTime mLastDisplayPTS;

  TU32 mTotalDisplayFrameCount;
  TTime mFirstFrameTime;
  TTime mCurFrameTime;

  TU32 *mpScaleIndexX;
  TU32 *mpScaleIndexY;

  TU8 mbScaled;
  TU8 mFrameCount;
  TU8 mCurFrameIndex;
  TU8 mbYUV16BitMode;

private:
  struct  fb_var_screeninfo    mVinfo;
  struct  fb_fix_screeninfo   mFinfo;
};

#endif

