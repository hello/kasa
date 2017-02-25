/**
 * am_video_capturer_active.h
 *
 * History:
 *    2014/11/03 - [Zhi He] create file
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
 */

#ifndef __AM_VIDEO_CAPTURER_ACTIVE_H__
#define __AM_VIDEO_CAPTURER_ACTIVE_H__

//-----------------------------------------------------------------------
//
// CVideoCapturerActive
//
//-----------------------------------------------------------------------
class CVideoCapturerActive
  : public CActiveFilter
{
  typedef CActiveFilter inherited;

public:
  static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

protected:
  CVideoCapturerActive(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

  EECode Construct();
  virtual ~CVideoCapturerActive();

public:
  virtual void Delete();

public:
  virtual EECode Initialize(TChar *p_param = NULL);
  virtual EECode Finalize();

  virtual EECode SwitchInput(TComponentIndex focus_input_index = 0);
  virtual EECode FlowControl(EBufferType flowcontrol_type);

  virtual EECode AddOutputPin(TUint &index, TUint &sub_index, TUint type);
  virtual EECode AddInputPin(TUint &index, TUint type);

  virtual IOutputPin *GetOutputPin(TUint index, TUint sub_index = 0);
  virtual IInputPin *GetInputPin(TUint index);

  virtual EECode FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param);

  virtual void GetInfo(INFO &info);
  virtual void PrintStatus();

  virtual void EventNotify(EEventType type, TU64 param1, TPointer param2);

protected:
  virtual void OnRun();

private:
  void processCmd(SCMD &cmd);
  void flushData();
  void beginCapture(TU32 fr_num, TU32 fr_den);

private:
  IVideoInput *mpCapturer;

private:
  TU8 mbContentSetup;
  TU8 mbCaptureFirstFrame;
  TU8 mbRunning;
  TU8 mbStarted;

  TU8 mbSnapshotCaptureMode;
  TU8 mMaxSnapshotCaptureFramenumber;
  TU8 mReserved0;
  TU8 mReserved1;

private:
  TU32 mnCurrentOutputPinNumber;
  COutputPin *mpOutputPin;
  CBufferPool *mpBufferPool;
  CIBuffer *mpBuffer;

private:
  TU32 mCapFramerateNum;
  TU32 mCapFramerateDen;

private:
  TTime mCaptureDuration;
  TU64 mSkipedFrameCount;
  TU64 mCapturedFrameCount;

  TTime mFirstFrameTime;
  TTime mCurTime;
  TTime mEstimatedNextFrameTime;
  TTime mSkipThreashold;
  TTime mWaitThreashold;

private:
  CIBuffer mPersistEOSBuffer;

private:
  EVideoInputModuleID mCurModuleID;
  SVideoCaptureParams mModuleParams;
};

#endif

