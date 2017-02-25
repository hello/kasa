/*******************************************************************************
 * am_video_injecter.h
 *
 * History:
 *    2015/12/28 - [Zhi He] create file
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

#ifndef __AM_VIDEO_INJECTER_H__
#define __AM_VIDEO_INJECTER_H__

//-----------------------------------------------------------------------
//
// CVideoInjecter
//
//-----------------------------------------------------------------------
class CVideoInjecter: public CActiveFilter
{
  typedef CActiveFilter inherited;

public:
  static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

protected:
  CVideoInjecter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

  EECode Construct();
  virtual ~CVideoInjecter();

public:
  virtual void Delete();
  virtual void Pause();
  virtual void Resume();
  virtual void Flush();
  virtual void ResumeFlush();

  virtual EECode Suspend(TUint release_context = 0);
  virtual EECode ResumeSuspend(TComponentIndex input_index = 0);

  virtual EECode SwitchInput(TComponentIndex focus_input_index = 0);

  virtual void PrintState();

  virtual EECode Run();

  virtual EECode Start();
  virtual EECode Stop();

  virtual EECode FlowControl(EBufferType type);

  virtual IInputPin *GetInputPin(TUint index);
  virtual IOutputPin *GetOutputPin(TUint index, TUint sub_index);
  virtual EECode AddInputPin(TUint &index, TUint type);
  virtual EECode AddOutputPin(TUint &index, TUint &sub_index, TUint type);

  virtual void EventNotify(EEventType type, TU64 param1, TPointer param2);
  virtual EECode FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param);

protected:
  virtual void OnRun();

public:
  virtual EECode Initialize(TChar *p_param = NULL);
  virtual EECode Finalize();
  virtual void PrintStatus();
  void GetInfo(INFO &info);

private:
  EECode flushInputData(TU8 disable_pin);
  EECode processCmd(SCMD &cmd);
  EECode injectBuffer();
  EECode hackFeedLastFrame();

private:
  IVideoInjecter *mpInjecter;
  CQueueInputPin *mpInputPins[EConstVideoRendererMaxInputPinNumber];
  CQueueInputPin *mpCurrentInputPin;
  TU32 mnInputPinsNumber;

private:
  void *mpCallbackContext;
  TFInjecterCallback mfCallback;

private:
  CIBuffer *mpBuffer;

private:
  TU32 mVideoWidth;
  TU32 mVideoHeight;
  TU32 mVideoFramerateNum;
  TU32 mVideoFramerateDen;
  StreamFormat mFormat;

  EVideoInjecterModuleID mModuleID;

private:
  TU8 mbPaused;
  TU8 mbStartStastics;
  TU8 mbInputPinDisabled;
  TU8 mStreamIndex;

  TU8 mbSendLastFrameFlag;
  TU8 mbBeginFeed;
  TU8 mbBatchMode;
  TU8 mBatchModeSourceBufferID;

  TU64 mStasticsFrameCount;
  TTime mStasticsBeginTime;
  TTime mStasticsEndTime;

  TTime mStasticsBeginTimeInstantaneous;
};

#endif


