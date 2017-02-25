/**
 * am_audio_renderer.h
 *
 * History:
 *    2013/01/24 - [Zhi He] create file
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

#ifndef __AM_AUDIO_RENDERER_H__
#define __AM_AUDIO_RENDERER_H__

//-----------------------------------------------------------------------
//
// CAudioRenderer
//
//-----------------------------------------------------------------------
class CAudioRenderer: public CActiveFilter
{
  typedef CActiveFilter inherited;

public:
  static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

protected:
  CAudioRenderer(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

  EECode Construct();
  virtual ~CAudioRenderer();

public:
  virtual void Delete();

public:
  virtual EECode Initialize(TChar *p_param = NULL);
  virtual EECode Finalize();

  virtual void OnRun();

  virtual EECode Run();

  virtual EECode Start();
  virtual EECode Stop();

  virtual void Pause();
  virtual void Resume();
  virtual void Flush();
  virtual void ResumeFlush();

  virtual EECode Suspend(TUint release_context = 0);
  virtual EECode ResumeSuspend(TComponentIndex input_index = 0);

  virtual EECode SwitchInput(TComponentIndex focus_input_index = 0);

  virtual EECode FlowControl(EBufferType flowcontrol_type);

  virtual EECode AddOutputPin(TUint &index, TUint &sub_index, TUint type);
  virtual EECode AddInputPin(TUint &index, TUint type);

  virtual IOutputPin *GetOutputPin(TUint index, TUint sub_index = 0);
  virtual IInputPin *GetInputPin(TUint index);

  virtual EECode FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param);
  virtual void PrintStatus();

  void GetInfo(INFO &info);

  //protected:
  //virtual EECode OnCmd(SCMD& cmd);

private:
  EECode stopAudioRenderer();
  EECode startAudioRenderer();
  EECode flushAudioRenderer();
  EECode pauseAudioRenderer();
  EECode resumeAudioRenderer();
  EECode flushInputData();

private:
  void processCmd(SCMD cmd);

private:
  IAudioRenderer *mpRenderer;
  CQueueInputPin *mpInputPins[EConstAudioRendererMaxInputPinNumber];
  CQueueInputPin *mpCurrentInputPin;
  TUint mnInputPinsNumber;

private:
  EAudioRendererModuleID mCurRendererID;

private:
  TU8 mbUsePresetMode;
  TU8 mReserved0;
  TU8 mReserved1;
  TU8 mReserved2;

private:
  CIBuffer *mpBuffer;

private:
  TTime mRenderingPTS;
  TTime mPrebufferingLength;
  TTime mEstimatedPrebufferingLength;

private:
  TU8 mbRendererContentSetup;
  TU8 mbRendererPaused;
  TU8 mbRendererMuted;
  TU8 mbRendererStarted;

  TU8 mbBlendingMode;//blend audio data from multiple channel before renderering
  TU8 mbRendererPrebufferReady;
  TU8 mbRecievedEosSignal;
  TU8 mbEos;
};

#endif


