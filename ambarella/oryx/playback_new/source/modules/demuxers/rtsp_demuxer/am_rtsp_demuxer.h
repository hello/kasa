/**
 * am_rtsp_demuxer.h
 *
 * History:
 *    2012/08/03 - [Zhi He] create file
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

#ifndef __AM_RTSP_DEMUXER_H__
#define __AM_RTSP_DEMUXER_H__

#define DMAX_RTSP_STRING_LEN 2048
#define DMAX_RTSP_CONTENT_BASE_STRING_LEN 256
#define DMAX_RTSP_SESSION_STRING_LEN 32

//-----------------------------------------------------------------------
//
// CScheduledRTSPDemuxer
//
//-----------------------------------------------------------------------
class CScheduledRTSPDemuxer
  : public CObject
  , public IDemuxer
{
  typedef CObject inherited;

protected:
  CScheduledRTSPDemuxer(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index);
  virtual ~CScheduledRTSPDemuxer();
  EECode Construct();

public:
  static IDemuxer *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index);
  virtual CObject *GetObject0() const;
  void Delete();

public:
  virtual EECode SetupOutput(COutputPin *p_output_pins[], CBufferPool *p_bufferpools[], IMemPool *p_mempools[], IMsgSink *p_msg_sink);

  virtual EECode SetupContext(TChar *url, void *p_agent = NULL, TU8 priority = 0, TU32 request_receive_buffer_size = 0, TU32 request_send_buffer_size = 0);
  virtual EECode DestroyContext();
  virtual EECode ReconnectServer();

  virtual EECode Seek(TTime &target_time, ENavigationPosition position = ENavigationPosition_Invalid);
  virtual EECode Start();
  virtual EECode Stop();

  virtual EECode Suspend();
  virtual EECode Pause();
  virtual EECode Resume();
  virtual EECode Flush();
  virtual EECode ResumeFlush();
  virtual EECode Purge();

  virtual EECode SetPbRule(TU8 direction, TU8 feeding_rule, TU16 speed);
  virtual EECode SetPbLoopMode(TU32 *p_loop_mode);

  virtual EECode EnableVideo(TU32 enable);
  virtual EECode EnableAudio(TU32 enable);

public:
  virtual EECode SetVideoPostProcessingCallback(void *callback_context, void *callback);
  virtual EECode SetAudioPostProcessingCallback(void *callback_context, void *callback);

public:
  void PrintStatus();

public:
  virtual EECode QueryContentInfo(const SStreamCodecInfos *&pinfos) const;
  EECode GetInfo(SStreamCodecInfos *&pinfos);

  virtual EECode UpdateContext(SContextInfo *pContext);
  virtual EECode GetExtraData(SStreamingSessionContent *pContent);
  virtual EECode NavigationSeek(SContextInfo *info);
  virtual EECode ResumeChannel();

private:
  EECode startRTPDataReceiver();
  EECode stopRTPDataReceiver();
  EECode startRTPDataReceiverTCP();
  EECode stopRTPDataReceiverTCP();
  EECode connectToRTSPServer(TChar *pFileName);
  EECode connectToRTSPServerTCP(TChar *pFileName);
  void sendTeardown();
  EECode createRTPDataReceivers();
  EECode createRTPDataReceiversTCP();
  void destroyRTPDataReceivers();
  void destroyRTPDataReceiversTCP();
  EECode clearContext();
  void reinitializeRTPDataReceivers();
  void reinitializeRTPDataReceiversTCP();
  void closeAllSockets();

  void postVideoSizeMsg(TU32 width, TU32 height);

private:
  const volatile SPersistMediaConfig *mpPersistMediaConfig;
  IMsgSink *mpMsgSink;

  SStreamCodecInfos mStreamCodecInfos;

private:
  SRTPContext mRTPContext[EConstMaxDemuxerMuxerStreamNumber];

private:
  IScheduledRTPReceiver *mpReceiver[EConstMaxDemuxerMuxerStreamNumber];
  IScheduledClient *mpScheduledClient[EConstMaxDemuxerMuxerStreamNumber];

  IScheduler *mpScheduler;
  IMutex *mpMutex;

  TU8 mbRTPReceiverCreated[EConstMaxDemuxerMuxerStreamNumber];
  TU8 mbRTPReceiverRegistered[EConstMaxDemuxerMuxerStreamNumber];

private:
  IScheduledRTPReceiver *mpReceiverTCP;
  IScheduledClient *mpScheduledClientTCP;

private:
  //flow control related
  TU8 mbRTSPServerConnected;
  TU8 mbOutputMsgSinkSet;
  TU8 mbContextCreated;
  TU8 mbContextRegistered;

  TU8 mbEnableAudio;
  TU8 mbEnableVideo;
  TU8 mReconnectCount;
  TU8 mMaxReconnectCount;

private:
  TU8 mPriority;
  TU8 mbUseContentBaseRTSPRequest;
  TU8 mbGetContentBase;
  TU8 mbUseTCPMode;

  TU32 mReceiveBufferSize;
  TU32 mSendBufferSize;

  //rtsp related
private:
  TChar *mpServerAddr;
  TChar *mpItemName;
  TMemSize mnUrlBufferSize;

  TSocketPort mServerRTSPPort;
  TU16 mRTSPSeq;

  TU32 mReconnectSessionID;

  TSocketPort mClientRTPPortRangeBegin;
  TSocketPort mClientRTPPortRangeEnd;
  TSocketPort mClientRTPPortRangeBeginBase;
  TU16 mbSendTeardown;

  TSocketHandler mRTSPSocket;

private:
  struct sockaddr_in mServerAddrIn;
  struct sockaddr_in mLocalHostAddr;

  TChar mRTSPBuffer[DMAX_RTSP_STRING_LEN];
  TChar mRTSPContentBaseBuffer[DMAX_RTSP_CONTENT_BASE_STRING_LEN];
  TChar mRTSPSessionString[DMAX_RTSP_SESSION_STRING_LEN];

private:
  TChar *mpSourceUrl;
  TU32 mSourceUrlLength;

  TU8 *mpVideoExtraDataFromSDP;
  TMemSize mVideoExtraDataFromSDPBufferSize;
  TMemSize mVideoExtraDataFromSDPSize;

private:
  TU8 *mpAudioExtraDataFromSDP;
  TMemSize mAudioExtraDataFromSDPBufferSize;
  TMemSize mAudioExtraDataFromSDPSize;

private:
  void *mpVideoPostProcessingCallback;
  void *mpVideoPostProcessingCallbackContext;
  void *mpAudioPostProcessingCallback;
  void *mpAudioPostProcessingCallbackContext;
};


#define RTP_SEQ_MOD (1<<16)

class RtpDecUtils
{
public:
  static void rtp_init_statistics(SRTPStatistics *s, TU16 base_sequence) {
    memset(s, 0, sizeof(SRTPStatistics));
    s->max_seq = base_sequence;
    s->probation = 1;
  }
  static void rtp_init_sequence(SRTPStatistics *s, TU16 seq) {
    s->max_seq = seq;
    s->cycles = 0;
    s->base_seq = seq - 1;
    s->bad_seq = RTP_SEQ_MOD + 1;
    s->received = 0;
    s->expected_prior = 0;
    s->received_prior = 0;
    s->jitter = 0;
    s->transit = 0;
  }
  /**
  * returns 1 if we should handle this packet.
  */
  static int rtp_valid_packet_in_sequence(SRTPStatistics *s, TU16 seq) {
    TU16 udelta = seq - s->max_seq;
    const int MAX_DROPOUT = 3000;
    const int MAX_MISORDER = 100;
    const int MIN_SEQUENTIAL = 2;
    /* source not valid until MIN_SEQUENTIAL packets with sequence seq. numbers have been received */
    if (s->probation) {
      if (seq == s->max_seq + 1) {
        s->probation--;
        s->max_seq = seq;
        if (s->probation == 0) {
          rtp_init_sequence(s, seq);
          s->received++;
          return 1;
        }
      } else {
        s->probation = MIN_SEQUENTIAL - 1;
        s->max_seq = seq;
      }
    } else if (udelta < MAX_DROPOUT) {
      // in order, with permissible gap
      if (seq < s->max_seq) {
        //sequence number wrapped; count antother 64k cycles
        s->cycles += RTP_SEQ_MOD;
      }
      s->max_seq = seq;
    } else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER) {
      // sequence made a large jump...
      if (seq == s->bad_seq) {
        // two sequential packets-- assume that the other side restarted without telling us; just resync.
        rtp_init_sequence(s, seq);
      } else {
        s->bad_seq = (seq + 1) & (RTP_SEQ_MOD - 1);
        return 0;
      }
    } else {
      // duplicate or reordered packet...
    }
    s->received++;
    return 1;
  }


};

#endif

