/**
 * am_rtp_h264_scheduled_receiver.h
 *
 * History:
 *    2013/07/09 - [Zhi He] create file
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

#ifndef __AM_RTP_H264_SCHEDULED_RECEIVER_H__
#define __AM_RTP_H264_SCHEDULED_RECEIVER_H__

//-----------------------------------------------------------------------
//
// CRTPH264ScheduledReceiver
//
//-----------------------------------------------------------------------
class CRTPH264ScheduledReceiver
  : public CObject
  , public IScheduledClient
  , public IScheduledRTPReceiver
  , public IEventListener
{
  typedef CObject inherited;

public:
  CRTPH264ScheduledReceiver(TUint index, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
  virtual ~CRTPH264ScheduledReceiver();

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
  void writeRR();
  void updateRR();
  void sendRR();
  void receiveSR();
  void checkAndSendRR(int count);
  int checkRtpPacket(unsigned char *packet, int len);
  EECode doSetExtraData(TU8 *p_extradata, TMemSize extradata_size);

private:
  IMutex *mpMutex;
  TU8 mTrackID;
  TU8 mbRun;
  TU8 mbInitialized;
  TU8 mbEnableRCTP;

  StreamType mType;
  StreamFormat mFormat;
  TU32 mVideoWidth;
  TU32 mVideoHeight;
  TU32 mVideoFramerateNum;
  TU32 mVideoFramerateDen;
  float mVideoFramerate;
  TInt mPayloadType;

  TU32 mReconnectTag;

  IMsgSink *mpMsgSink;
  const volatile SPersistMediaConfig *mpPersistMediaConfig;
  CIClockReference *mpSystemClockReference;
  SClockListener *mpWatchDog;

  COutputPin *mpOutputPin;
  CBufferPool *mpBufferPool;
  IMemPool *mpMemPool;

  TSocketHandler mRTSPSocket;
  TSocketHandler mRTPSocket;
  TSocketHandler mRTCPSocket;

private:
  struct sockaddr mSrcAddr;
  struct sockaddr mSrcRTCPAddr;
  socklen_t mFromLen;

private:
  TUint msState;

private:
  TInt mReservedDataLength;
  TInt mStartCodeLength;
  TInt mRTPHeaderLength;

  TU8 *mpMemoryStart;
  TU8 *mpCurrentPointer;
  TInt mTotalWritenLength;
  TInt mReadLen;

  TU32 mReconnectSessionID;

private:
  //from bit-stream
  TU8 *mpVideoExtraData;
  TInt mVideoExtraDataLen;
  TInt mVideoExtraDataBufLen;

  TU8 mH264DataFmt;
  TU8 mH264AVCCNaluLen;
  TU8 mReserved0;
  TU8 mReserved1;

  //from sdp
  TU8 *mpVideoExtraDataFromSDP;
  TUint mVideoExtraDataSize;
  TU8 *mpH264spspps;
  TUint mH264spsppsSize;

private:
  TU32 mRTPTimeStamp;
  TU16 mRTPCurrentSeqNumber;
  TU16 mRTPLastSeqNumber;

  CIBuffer *mpBuffer;

  TU8 mNalType;
  TU8 mNri;
  TU8 mbWaitFirstSpsPps;
  TU8 mbWaitRTPDelimiter;

  TInt mRequestMemorySize;
  TInt mCurrentMemorySize;

  TUint mCmd;

private:
  TU8 mDebugWaitMempoolFlag;
  TU8 mDebugWaitBufferpoolFlag;
  TU8 mDebugWaitReadSocketFlag;
  TU8 mbGetSSRC;

private:
  TU8 mbSendSyncPointBuffer;
  TU8 mPriority;
  TU16 mBufferSessionNumber;

private:
  TU8 mReservedData[32];

private:
  volatile TTime mLastAliveTimeStamp;

private:
  TTime mLastRtcpNtpTime;
  TTime mFirstRtcpNtpTime;
  TTime mLastRtcpTimeStamp;
  TTime mRtcpTimeStampOffset;

  TUint mPacketCount;
  TUint mOctetCount;
  TUint mLastOctetCount;

  TU32 mServerSSRC;
  TU32 mSSRC;
  TU32 mPacketSSRC;

  TU32 mRTCPCurrentTick;
  TU32 mRTCPCoolDown;

  TChar mCName[128];

  TU8 mRTCPBuffer[256];
  TUint mRTCPDataLen;

  TU8 mRTCPReadBuffer[256];
  TUint mRTCPReadDataLen;

  SRTPStatistics mRTPStatistics;

private:
  TU32 mbSetTimeStamp;
  TU32 mbBrokenFrame;
  TU32 mbSendSequenceData;
  TU32 mbWaitReconnect;

  TTime mFakePts;

private:
  CIBuffer mPersistExtradataBuffer;

  //the following added to calc pts(timestamp)
private:
  TU32 timestamp;
  TU32 base_timestamp;
  TU32 cur_timestamp;
  TS64  unwrapped_timestamp;
  TS64  range_start_offset;
  /* rtcp sender statistics receive */
  TS64 last_rtcp_ntp_time;
  TS64 first_rtcp_ntp_time;
  TU32 last_rtcp_timestamp;
  TS64 rtcp_ts_offset;
  //void do_calc_pts(TS64 &pts, TU32 curr_timestamp);
  TU32 octet_count;
  TU32 last_octet_count;

private:
  TU8 mbDumpBitstream;
  TU8 mbDumpOnly;
  TU16 mDumpIndex;
  void *mpDumper;
};

#endif

