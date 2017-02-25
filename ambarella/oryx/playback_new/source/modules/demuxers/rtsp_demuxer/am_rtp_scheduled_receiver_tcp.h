/**
 * am_rtp_scheduled_receiver_tcp.h
 *
 * History:
 *    2014/12/13 - [Zhi He] create file
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

#ifndef __AM_RTP_SCHEDULED_RECEIVER_TCP_H__
#define __AM_RTP_SCHEDULED_RECEIVER_TCP_H__

//-----------------------------------------------------------------------
//
// CRTPScheduledReceiverTCP
//
//-----------------------------------------------------------------------
class CRTPScheduledReceiverTCP
  : public CObject
  , public IScheduledClient
  , public IScheduledRTPReceiver
  , public IEventListener
{
  typedef CObject inherited;

public:
  CRTPScheduledReceiverTCP(TUint index, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
  virtual ~CRTPScheduledReceiverTCP();

public:
  virtual EECode Scheduling(TUint times = 1, TU32 inout_mask = 0);
  virtual TInt IsPassiveMode() const;
  virtual TSchedulingHandle GetWaitHandle() const;
  virtual TSchedulingUnit HungryScore() const;
  virtual EECode EventHandling(EECode err);
  virtual TU8 GetPriority() const;

public:
  virtual CObject *GetObject0() const;

public:
  virtual EECode Initialize(SRTPContext *context, SStreamCodecInfo *p_stream_info, TUint number_of_content = 1);
  virtual EECode ReInitialize(SRTPContext *context, SStreamCodecInfo *p_stream_info, TUint number_of_content = 1);
  virtual EECode Flush();

  virtual EECode Purge();
  virtual EECode SetExtraData(TU8 *p_extradata, TU32 extradata_size, TU32 index);

  virtual void SetVideoDataPostProcessingCallback(void *callback_context, void *callback);
  virtual void SetAudioDataPostProcessingCallback(void *callback_context, void *callback);

public:
  virtual void PrintStatus();

public:
  virtual void EventNotify(EEventType type, TU64 param1, TPointer param2);

private:
  void postEngineMsg(EMSGType msg_type);
  void requestMemory();
  void releaseMemory();
  void sendVideoExtraDataBuffer();
  void sendVideoData(TU8 nal_type);
  void sendAudioExtraDataBuffer();
  void sendAudioData();
  EECode doSetVideoExtraData(TU8 *p_extradata, TMemSize extradata_size);
  EECode doSetAudioExtraData(TU8 *p_extradata, TMemSize extradata_size);
  void sendEOS();

  void writeRR();
  void updateRR();
  void sendRR();

private:
  const volatile SPersistMediaConfig *mpPersistMediaConfig;
  IMsgSink *mpMsgSink;
  CIClockReference *mpSystemClockReference;
  SClockListener *mpWatchDog;
  IMutex *mpMutex;

  StreamFormat mVideoFormat;
  StreamFormat mAudioFormat;

  COutputPin *mpOutputPin[EConstMaxDemuxerMuxerStreamNumber];
  CBufferPool *mpBufferPool[EConstMaxDemuxerMuxerStreamNumber];
  IMemPool *mpMemPool[EConstMaxDemuxerMuxerStreamNumber];
  TInt mRequestMemorySize[EConstMaxDemuxerMuxerStreamNumber];

  TTime mFakePts[EConstMaxDemuxerMuxerStreamNumber];
  TU8 mPayloadType[EConstMaxDemuxerMuxerStreamNumber];
  TU8 mbSendExtraData[EConstMaxDemuxerMuxerStreamNumber];

  TSocketHandler mRTSPSocket;
  TU32 mReconnectTag;

private:
  TU32 msState;

  TU8 mCurrentTrack;
  TU8 mbInitialized;
  TU8 mbEnableRCTP;
  TU8 mbSetTimeStamp;

  TU8 mSentErrorMsg;
  TU8 mCurPayloadType;
  TU8 mMarkFlag;
  TU8 mHaveFU;

  TU8 mFilledStartCode;
  TU8 mbReadRemaingVideoData;
  TU8 mbReadRemaingAudioData;
  TU8 mbWaitFirstSpsPps;

  TU8 mbGetSSRC;
  TU8 mbRunning;
  TU8 mReserved1;
  TU8 mReserved2;

  TInt mDataLength;

  TU32 mReconnectSessionID;

  TU32 mVideoWidth;
  TU32 mVideoHeight;
  TU32 mVideoFramerateNum;
  TU32 mVideoFramerateDen;
  float mVideoFramerate;

  TU8 mAudioChannelNumber;
  TU8 mAudioSampleFormat;
  TU8 mbAudioChannelInterlave;
  TU8 mbParseMultipleAccessUnit;

  TU32 mAudioSamplerate;
  TU32 mAudioBitrate;
  TU32 mAudioFrameSize;

private:
  TU8 *mpMemoryStart;
  //TU8* mpCurrentPointer;
  TInt mTotalWritenLength;
  TInt mMemorySize;

private:
  //from sdp
  TU8 *mpVideoExtraDataFromSDP;
  TU32 mVideoExtraDataSize;
  TU8 *mpH264spspps;
  TU32 mH264spsppsSize;

  TU8 *mpAudioExtraData;
  TU32 mAudioExtraDataSize;

private:
  volatile TTime mLastAliveTimeStamp;

private:
  TU8 mFrameHeader[4];
  TU8 mRTPHeader[DRTP_HEADER_FIXED_LENGTH + 4];
  TU8 mRTCPPacket[256];
  TU8 mAACHeader[4];
  TInt mCurrentFrameHeaderLength;
  TInt mCurrentRTPHeaderLength;
  TInt mCurrentRTPDataLength;
  TInt mCurrentRTCPPacketLength;
  TInt mCurrentAudioCodecSpecificLength;

  TChar mCName[128];
  TU8 mRTCPBuffer[256];
  TUint mRTCPDataLen;

  TU32 mServerSSRC;
  TU32 mSSRC;
  TU32 mPacketSSRC;

  TU32 mRTPTimeStamp;
  TU16 mRTPCurrentSeqNumber;
  TU16 mRTPLastSeqNumber;

  TS64 mLastNTPTime;
  TU32 mLastTimestamp;

private:
  CIBuffer mPersistExtradataBuffer;
  CIBuffer mPersistEOSBuffer;

private:
  TEmbededDataProcessingCallBack mVideoPostProcessingCallback;
  void *mpVideoPostProcessingCallbackContext;

private:
  TU8 mbDumpBitstream;
  TU8 mbDumpOnly;
  TU16 mDumpIndex;
  void *mpDumper;
};

#endif

