/*******************************************************************************
 * am_muxer.h
 *
 * History:
 *    2014/08/25 - [Zhi He] create file
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

#ifndef __AM_MUXER_H__
#define __AM_MUXER_H__

//-----------------------------------------------------------------------
//
// CMuxerInput
//
//-----------------------------------------------------------------------
class CMuxerInput: public CQueueInputPin
{
  typedef CQueueInputPin inherited;
  friend class CScheduledMuxer;

  enum {
    DefaultPTSDTSGapDivideDuration = 4,
  };

public:
  static CMuxerInput *Create(const TChar *pName, IFilter *pFilter, CIQueue *pQueue, TUint type, TUint stream_index);

private:
  CMuxerInput(const TChar *pName, IFilter *pFilter, TUint type, TUint stream_index)
    : inherited(pName, pFilter, (StreamType) type)
    , mDiscardPacket(0)
    , mStreamIndex(stream_index)
    , mType((StreamType)type)
    , mEOSComes(0)
    , mpPrivateData(NULL)
    , mStartPTS(0)
    , mSessionInputPTSStartPoint(0)
    , mSessionPTSStartPoint(0)
    , mCurrentPTS(0)
    , mCurrentDTS(0)
    , mLastPTS(0)
    , mDuration(0)
    , mInputSatrtPTS(0)
    , mInputEndPTS(0)
    , mTotalFrameCount(0LL)
    , mCurrentFrameCount(0LL)
    , mSession(0)
    , mTimeUintNum(1)
    , mTimeUintDen(TimeUnitDen_90khz)
    , mNeedModifyPTS(0)
    , mbSkiped(0)
    , mbAutoBoundaryReached(0)
    , mbAutoBoundaryStarted(0)
    , mbExtraDataComes(0)
    , mPTSDTSGap(0)
    , mPTSDTSGapDivideDuration(DefaultPTSDTSGapDivideDuration)
    , mExpectedDuration(0)
    , mFrameCountFromLastBPicture(0)
    , mLastBPicturePTS(0)
    , mBitrate(0)
    , mpCachedBuffer(NULL) {
  }
  EECode Construct(CIQueue *pMsgQ);
  virtual ~CMuxerInput();

public:
  virtual void Delete();
  virtual TU32 GetDataCnt() const;
  TUint NeedDiscardPacket();
  void DiscardPacket(TUint discard);
  void PrintState();
  void PrintTimeInfo();

private:
  void timeDiscontinue();

protected:
  TUint mDiscardPacket;
  TUint mStreamIndex;

protected:
  StreamType mType;
  TUint mEOSComes;

protected:
  TU8 *mpPrivateData;

protected:
  TTime mStartPTS;
  TTime mSessionInputPTSStartPoint;//join two start points, make PTS continus when pause/resume
  TTime mSessionPTSStartPoint;
  TTime mCurrentPTS;
  TTime mCurrentDTS;
  TTime mLastPTS;//debug only
  TTime mDuration;
  TTime mAVNormalizedDuration;//use av's unit

  TTime mInputSatrtPTS;
  TTime mInputEndPTS;

  TU64 mTotalFrameCount;
  TU64 mCurrentFrameCount;
  TUint mSession;

protected:
  TUint mTimeUintNum;
  TUint mTimeUintDen;

protected:
  TUint mNeedModifyPTS;

protected:
  TU8 mbSkiped;
  TU8 mbAutoBoundaryReached;
  TU8 mbAutoBoundaryStarted;
  TU8 mbExtraDataComes;

  //some error concealment code, expect: "PTS not very correct" will not cause recording fail, only generate warning msg
  //muxer should be robust, code would be not beautiful
protected:
  //DTS generator logic: write pts = pts + ptsdts gap, dts = pts(sync B picture comes), dts = previous dts + duration
  TUint mPTSDTSGap;
  TUint mPTSDTSGapDivideDuration;
  //calculated from frame rate, debug only
  TUint mExpectedDuration;

  TUint mFrameCountFromLastBPicture;
  TU64 mLastBPicturePTS;

  TU32 mBitrate;

protected:
  CIBuffer *mpCachedBuffer;//saving for Next File
};

//-----------------------------------------------------------------------
//
// CScheduledMuxer
//
//-----------------------------------------------------------------------
class CScheduledMuxer: public CScheduledFilter
{
  typedef CScheduledFilter inherited;

public:
  static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

protected:
  CScheduledMuxer(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

  EECode Construct();
  virtual ~CScheduledMuxer();

public:
  virtual void Delete();

public:
  virtual EECode Initialize(TChar *p_param = NULL);
  virtual EECode Finalize();

  virtual EECode Start();
  virtual EECode Stop();

  virtual EECode SwitchInput(TComponentIndex focus_input_index = 0);

  virtual EECode FlowControl(EBufferType flowcontrol_type);

  virtual EECode AddOutputPin(TUint &index, TUint &sub_index, TUint type);
  virtual EECode AddInputPin(TUint &index, TUint type);

  virtual IOutputPin *GetOutputPin(TUint index, TUint sub_index = 0);
  virtual IInputPin *GetInputPin(TUint index);

  virtual EECode FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param);

  virtual void GetInfo(INFO &info);
  virtual void PrintStatus();

public:
  virtual EECode Scheduling(TUint times = 1, TU32 inout_mask = 0);
  virtual TInt IsPassiveMode() const;
  virtual TSchedulingHandle GetWaitHandle() const;
  virtual TSchedulingUnit HungryScore() const;
  virtual TU8 GetPriority() const;
  virtual void Destroy();

private:
  EECode initialize();
  void finalize();
  EECode setOutputFile(TChar *pFileName);

private:
  void postMsg(TUint msg_code);
  TU32 processCmd(SCMD &cmd);
  EECode writeData(CIBuffer *pBuffer, CMuxerInput *pPin);

  TU32 readInputData(CMuxerInput *inputpin, EBufferType &type);
  EECode flushInputData();

private:
  void deletePartialFile(TUint file_index);
  void analyseFileNameFormat(TChar *pstart);
  void updateFileName(TUint file_index);

  TU32 isCommingBufferAutoFileBoundary(CMuxerInput *pInputPin, CIBuffer *pBuffer);
  TU32 hasPinNeedReachBoundary(TUint &i);

  TU32 allInputEos();

private:
  EECode initializeFile(TChar *p_param, ContainerType container_type);
  EECode finalizeFile();

  void initializeTimeInfo();
  void getFileInformation();

private:
  EECode saveStreamInfo(StreamType type, TUint stream_index, CIBuffer *p_buffer);
  EECode saveExtraData(StreamType type, TUint stream_index, void *p_data, TUint data_size);
  EECode setExtraData();

private:
  IMuxer *mpMuxer;
  CMuxerInput *mpInputPins[EConstMuxerMaxInputPinNumber];
  CMuxerInput *mpCurrentInputPin;
  TUint mnInputPinsNumber;

  IScheduler *mpScheduler;

private:
  ContainerType mRequestContainerType;

private:
  EMuxerModuleID mCurMuxerID;

private:
  CIBuffer *mpBuffer;

private:
  TU8 mbMuxerContentSetup;
  TU8 mbMuxerPaused;
  TU8 mbMuxerStarted;
  TU8 mbMuxerVideoEnabled;

  TU8 mbMuxerAudioEnabled;
  TU8 mbMuxerPridataEnabled;
  TU8 mbRecievedEosSignal;
  TU8 mbEos;

  TU8 mbUseSourcePTS;
  TU8 mbSuspend;
  TU8 mbReserved1;
  TU8 mbReserved2;

private:
  TChar *mpOutputFileName;
  TUint mOutputFileNameLength;

private:
  TTime mStreamStartPTS;

private:
  TU8 mFileNameHanding1;
  TU8 mFileNameHanding2;
  TU8 mbMasterStarted;
  TU8 mbExtraDataRecieved;

  //for separate file
private:
  //TU64 mIncommingPTSThreshHold;
  CMuxerInput *mpMasterInput;//pin which will send finalize_control_buffer
  TU32 mCurrentFileIndex;//max
  TU32 mFirstFileIndexCanbeDeleted;//min
  TU32 mTotalFileNumber;
  TU32 mMaxTotalFileNumber;

  TU32 mTotalRecFileNumber;
  TChar *mpOutputFileNameBase;
  TU32 mOutputFileNameBaseLength;

private:
  MuxerSavingFileStrategy mSavingFileStrategy;
  MuxerSavingCondition mSavingCondition;
  MuxerAutoFileNaming mAutoFileNaming;
  TU64 mAutoSaveFrameCount;
  TTime mAutoSavingTimeDuration;
  TTime mNextFileTimeThreshold;

  TU8 mbNextFileTimeThresholdSet;
  TU8 mbPTSStartFromZero;
  TU8 mbAutoName;
  TU8 mbFileCreated;

  TU64 mCurrentTotalFilesize;

private:
  TU8        *mpExtraData[EConstMaxDemuxerMuxerStreamNumber];
  TU32       mExtraDataSize[EConstMaxDemuxerMuxerStreamNumber];
  SStreamCodecInfos mStreamCodecInfos;
  SMuxerFileInfo mMuxerFileInfo;

private:
  EBufferType mBufferType;
  CMuxerInput *mpPin;
  TU32 mPreferentialPinIndex;

  //saving file's variables
  TU32 mInputIndex;
  TTime mNewTimeThreshold;

private:
  //IStorageManagement* mpStorageManager;
  TChar *mpChannelName;
  TMemSize mChannelNameBufferSize;
};

#endif


