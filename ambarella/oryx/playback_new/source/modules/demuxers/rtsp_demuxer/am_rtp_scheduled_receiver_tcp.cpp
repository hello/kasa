/**
 * am_rtp_scheduled_receiver_tcp.cpp
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

#include "am_native.h"
#include "am_native_log.h"

#include "am_internal.h"

#include "am_network_al.h"

#include "am_osal.h"
#include "am_framework.h"
#include "am_modules_interface.h"

#ifdef BUILD_MODULE_DEMUXER_RTSP

#include "am_rtp_scheduled_receiver_tcp.h"
#include "am_rtsp_demuxer.h"

static void __parseRTPPacketHeader(TU8 *p_packet, TU32 &time_stamp, TU16 &seq_number, TU32 &ssrc)
{
  DASSERT(p_packet);
  if (p_packet) {
    time_stamp = (p_packet[7]) | (p_packet[6] << 8) | (p_packet[5] << 16) | (p_packet[4] << 24);
    ssrc = (p_packet[11]) | (p_packet[10] << 8) | (p_packet[9] << 16) | (p_packet[8] << 24);
    seq_number = (p_packet[2] << 8) | (p_packet[3]);
  }
}

CRTPScheduledReceiverTCP::CRTPScheduledReceiverTCP(TUint index, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
  : inherited("CRTPScheduledReceiverTCP", index)
  , mpPersistMediaConfig(pPersistMediaConfig)
  , mpMsgSink(pMsgSink)
  , mpSystemClockReference(NULL)
  , mpWatchDog(NULL)
  , mpMutex(NULL)
  , mVideoFormat(StreamFormat_H264)
  , mAudioFormat(StreamFormat_AAC)
  , mRTSPSocket(DInvalidSocketHandler)
  , mReconnectTag(0)
  , msState(DATA_THREAD_STATE_READ_FRAME_HEADER)
  , mCurrentTrack(0)
  , mbInitialized(0)
  , mbEnableRCTP(0)
  , mbSetTimeStamp(1)
  , mSentErrorMsg(0)
  , mDataLength(0)
  , mVideoWidth(720)
  , mVideoHeight(480)
  , mVideoFramerateNum(DDefaultVideoFramerateNum)
  , mVideoFramerateDen(DDefaultVideoFramerateDen)
  , mVideoFramerate((float)DDefaultVideoFramerateNum / (float)DDefaultVideoFramerateDen)
  , mAudioChannelNumber(DDefaultAudioChannelNumber)
  , mAudioSampleFormat(AudioSampleFMT_S16)
  , mbAudioChannelInterlave(0)
  , mbParseMultipleAccessUnit(0)
  , mAudioSamplerate(DDefaultAudioSampleRate)
  , mAudioBitrate(64000)
  , mAudioFrameSize(1024)
  , mpMemoryStart(NULL)
  , mTotalWritenLength(0)
  , mpVideoExtraDataFromSDP(NULL)
  , mVideoExtraDataSize(0)
  , mpH264spspps(NULL)
  , mH264spsppsSize(0)
  , mpAudioExtraData(NULL)
  , mAudioExtraDataSize(0)
  , mLastAliveTimeStamp(0)
  , mCurrentFrameHeaderLength(0)
  , mCurrentRTPHeaderLength(0)
  , mCurrentRTPDataLength(0)
  , mCurrentRTCPPacketLength(0)
{
  mpMutex = gfCreateMutex();
  DASSERT(mpMutex);
  DASSERT(mpPersistMediaConfig);
  if (mpPersistMediaConfig) {
    mpSystemClockReference = mpPersistMediaConfig->p_system_clock_reference;
  }
  DASSERT(mpSystemClockReference);
  for (TU32 i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
    mpOutputPin[i] = NULL;
    mpBufferPool[i] = NULL;
    mpMemPool[i] = NULL;
    mFakePts[i] = 0;
    mbSendExtraData[i] = 0;
  }
  mRequestMemorySize[EDemuxerVideoOutputPinIndex] = 512 * 1024;
  mRequestMemorySize[EDemuxerAudioOutputPinIndex] = 1024;
  mPayloadType[EDemuxerVideoOutputPinIndex] = RTP_PT_H264;
  mPayloadType[EDemuxerAudioOutputPinIndex] = RTP_PT_AAC;
  mReconnectSessionID = 0;
  mCurPayloadType = 0;
  mMarkFlag = 0;
  mHaveFU = 0;
  mFilledStartCode = 0;
  mbReadRemaingVideoData = 0;
  mbReadRemaingAudioData = 0;
  mbWaitFirstSpsPps = 1;
  memset(mRTCPPacket, 0x0, sizeof(mRTCPPacket));
  memset(mFrameHeader, 0x0, sizeof(mFrameHeader));
  memset(mRTCPPacket, 0x0, sizeof(mRTCPPacket));
  memset(mAACHeader, 0x0, sizeof(mAACHeader));
  mPersistEOSBuffer.SetBufferType(EBufferType_FlowControl_EOS);
  mpVideoPostProcessingCallbackContext = NULL;
  mVideoPostProcessingCallback = NULL;
  memset(mCName, 0x0, sizeof(mCName));
  memset(mRTCPBuffer, 0x0, sizeof(mRTCPBuffer));
  mRTCPDataLen = 0;
  mServerSSRC = 0;
  mSSRC = 0;
  mPacketSSRC = 0;
  mRTPTimeStamp = 0;
  mRTPCurrentSeqNumber = 0;
  mRTPLastSeqNumber = 0;
  mLastNTPTime = 0;
  mLastTimestamp = 0;
  mbGetSSRC = 0;
  mbDumpBitstream = mpPersistMediaConfig->dump_setting.dump_bitstream;
  mbDumpOnly = mpPersistMediaConfig->dump_setting.dump_only;
  mDumpIndex = 0;
  mpDumper = NULL;
}

CRTPScheduledReceiverTCP::~CRTPScheduledReceiverTCP()
{
  if (DLikely(mpSystemClockReference && mpWatchDog)) {
    mpSystemClockReference->RemoveTimer(mpWatchDog);
  }
  if (mpVideoExtraDataFromSDP) {
    DDBG_FREE(mpVideoExtraDataFromSDP, "DMVe");
    mpVideoExtraDataFromSDP = NULL;
  }
  if (mpH264spspps) {
    DDBG_FREE(mpH264spspps, "DMvE");
    mpH264spspps = NULL;
  }
  if (mpAudioExtraData) {
    DDBG_FREE(mpAudioExtraData, "RAEX");
    mpAudioExtraData = NULL;
  }
  if (mpMutex) {
    mpMutex->Delete();
    mpMutex = NULL;
  }
  if (mpDumper) {
    gfDestroyFileDumper(mpDumper);
    mpDumper = NULL;
  }
}

EECode CRTPScheduledReceiverTCP::Initialize(SRTPContext *context, SStreamCodecInfo *p_stream_info, TUint number_of_content)
{
  AUTO_LOCK(mpMutex);
  if (mbInitialized) {
    LOGM_FATAL("already initialized?\n");
    return EECode_BadState;
  }
  if (DUnlikely(!context || !p_stream_info || !number_of_content)) {
    LOGM_FATAL("NULL params in CRTPScheduledReceiverTCP::Initialize\n");
    return EECode_BadParam;
  }
  memset(mCName, 0x0, sizeof(mCName));
  gfOSGetHostName(mCName, sizeof(mCName));
  DASSERT(strlen(mCName) < (sizeof(mCName) + 16));
  snprintf(mCName, sizeof(mCName), "%s_%d_%d", mCName, mReconnectTag++, context->index);
  msState = DATA_THREAD_STATE_READ_FRAME_HEADER;
  mbWaitFirstSpsPps = 1;
  mTotalWritenLength = 0;
  mbSetTimeStamp = 0;
  mSentErrorMsg = 0;
  mDataLength = 0;
  mbGetSSRC = 0;
  mRTSPSocket = context->rtsp_socket;
  mCurrentFrameHeaderLength = 0;
  mCurrentRTPHeaderLength = 0;
  mCurrentRTPDataLength = 0;
  mCurrentRTCPPacketLength = 0;
  mCurPayloadType = 0;
  mMarkFlag = 0;
  mHaveFU = 0;
  mFilledStartCode = 0;
  mbReadRemaingVideoData = 0;
  mbReadRemaingAudioData = 0;
  mReconnectSessionID = context->mReconnectSessionID;
  mbRunning = 1;
  mbSendExtraData[EDemuxerVideoOutputPinIndex] = 0;
  mbSendExtraData[EDemuxerAudioOutputPinIndex] = 0;
  SRTPContext *p_c = NULL;
  SStreamCodecInfo *p_s_i = NULL;
  for (TU32 i = 0; i < number_of_content; i ++) {
    p_c = context + i;
    p_s_i = p_stream_info + i;
    mpOutputPin[i] = p_c->outpin;
    mpBufferPool[i] = p_c->bufferpool;
    mpMemPool[i] = p_c->mempool;
    if (StreamType_Video == p_s_i->stream_type) {
      if (DLikely(p_s_i->spec.video.pic_width && p_s_i->spec.video.pic_height)) {
        mVideoWidth = p_s_i->spec.video.pic_width;
        mVideoHeight = p_s_i->spec.video.pic_height;
      }
      if (DLikely(p_s_i->spec.video.framerate_num && p_s_i->spec.video.framerate_den)) {
        mVideoFramerateNum = p_s_i->spec.video.framerate_num;
        mVideoFramerateDen = p_s_i->spec.video.framerate_den;
      }
      if (DLikely(p_s_i->spec.video.framerate)) {
        mVideoFramerate = p_s_i->spec.video.framerate;
      }
    } else if (StreamType_Audio == p_s_i->stream_type) {
      mAudioChannelNumber = p_s_i->spec.audio.channel_number;
      mAudioSamplerate = p_s_i->spec.audio.sample_rate;
      if (p_s_i->spec.audio.bitrate) {
        mAudioBitrate = p_s_i->spec.audio.bitrate;
      }
      if (p_s_i->spec.audio.sample_format) {
        mAudioSampleFormat = p_s_i->spec.audio.sample_format;
      }
      mbAudioChannelInterlave = p_s_i->spec.audio.is_channel_interlave;
    }
    mPayloadType[i] = p_s_i->payload_type;
  }
  mbInitialized = 1;
  if (DLikely(mpSystemClockReference && mpPersistMediaConfig->rtsp_client_config.watchdog)) {
    DASSERT(!mpWatchDog);
    mpWatchDog = mpSystemClockReference->AddTimer(this, EClockTimerType_repeat_infinite, mpSystemClockReference->GetCurrentTime() + 20000000, 10000000, 0);
    DASSERT(mpWatchDog);
    mLastAliveTimeStamp = mpSystemClockReference->GetCurrentTime();
  }
  if (mbDumpBitstream) {
    if (mpDumper) {
      gfDestroyFileDumper(mpDumper);
      mpDumper = NULL;
    }
    TChar filename[128] = {0};
    //snprintf(filename, 127, "rtp_h264_tcp_%04d.h264", mDumpIndex);
    //mDumpIndex ++;
    SDateTime datetime;
    gfGetCurrentDateTime(&datetime);
    snprintf(filename, 127, "rtp_h264_tcp_%04d_%02d_%02d-%02d_%02d_%02d.h264", datetime.year, datetime.month, datetime.day, datetime.hour, datetime.minute, datetime.seconds);
    mpDumper = gfCreateFileDumper(filename);
  }
  return EECode_OK;
}

EECode CRTPScheduledReceiverTCP::ReInitialize(SRTPContext *context, SStreamCodecInfo *p_stream_info, TUint number_of_content)
{
  AUTO_LOCK(mpMutex);
  if (DUnlikely(!context || !p_stream_info || !number_of_content)) {
    LOGM_FATAL("NULL params in CRTPScheduledReceiverTCP::Initialize\n");
    return EECode_BadParam;
  }
  memset(mCName, 0x0, sizeof(mCName));
  gfOSGetHostName(mCName, sizeof(mCName));
  DASSERT(strlen(mCName) < (sizeof(mCName) + 16));
  snprintf(mCName, sizeof(mCName), "%s_%d_%d", mCName, mReconnectTag++, context->index);
  msState = DATA_THREAD_STATE_READ_FRAME_HEADER;
  mbWaitFirstSpsPps = 1;
  mTotalWritenLength = 0;
  mbSetTimeStamp = 0;
  mSentErrorMsg = 0;
  mDataLength = 0;
  mbGetSSRC = 0;
  mRTSPSocket = context->rtsp_socket;
  mCurrentFrameHeaderLength = 0;
  mCurrentRTPHeaderLength = 0;
  mCurrentRTPDataLength = 0;
  mCurrentRTCPPacketLength = 0;
  mCurPayloadType = 0;
  mMarkFlag = 0;
  mHaveFU = 0;
  mFilledStartCode = 0;
  mbReadRemaingVideoData = 0;
  mbReadRemaingAudioData = 0;
  mReconnectSessionID = context->mReconnectSessionID;
  mbRunning = 1;
  mbSendExtraData[EDemuxerVideoOutputPinIndex] = 0;
  mbSendExtraData[EDemuxerAudioOutputPinIndex] = 0;
  SRTPContext *p_c = NULL;
  SStreamCodecInfo *p_s_i = NULL;
  for (TU32 i = 0; i < number_of_content; i ++) {
    p_c = context + i;
    p_s_i = p_stream_info + i;
    mpOutputPin[i] = p_c->outpin;
    mpBufferPool[i] = p_c->bufferpool;
    mpMemPool[i] = p_c->mempool;
    if (StreamType_Video == p_s_i->stream_type) {
      if (DLikely(p_s_i->spec.video.pic_width && p_s_i->spec.video.pic_height)) {
        mVideoWidth = p_s_i->spec.video.pic_width;
        mVideoHeight = p_s_i->spec.video.pic_height;
      }
      if (DLikely(p_s_i->spec.video.framerate_num && p_s_i->spec.video.framerate_den)) {
        mVideoFramerateNum = p_s_i->spec.video.framerate_num;
        mVideoFramerateDen = p_s_i->spec.video.framerate_den;
      }
      if (DLikely(p_s_i->spec.video.framerate)) {
        mVideoFramerate = p_s_i->spec.video.framerate;
      }
    } else if (StreamType_Audio == p_s_i->stream_type) {
      mAudioChannelNumber = p_s_i->spec.audio.channel_number;
      mAudioSamplerate = p_s_i->spec.audio.sample_rate;
      if (p_s_i->spec.audio.bitrate) {
        mAudioBitrate = p_s_i->spec.audio.bitrate;
      }
      if (p_s_i->spec.audio.sample_format) {
        mAudioSampleFormat = p_s_i->spec.audio.sample_format;
      }
      mbAudioChannelInterlave = p_s_i->spec.audio.is_channel_interlave;
    }
    mPayloadType[i] = p_s_i->payload_type;
  }
  mbInitialized = 1;
  if (DLikely(mpSystemClockReference && mpPersistMediaConfig->rtsp_client_config.watchdog)) {
    if (DUnlikely(!mpWatchDog)) {
      mpSystemClockReference->RemoveTimer(mpWatchDog);
    }
    mpWatchDog = mpSystemClockReference->AddTimer(this, EClockTimerType_repeat_infinite, mpSystemClockReference->GetCurrentTime() + 20000000, 10000000, 0);
    mLastAliveTimeStamp = mpSystemClockReference->GetCurrentTime();
  }
  if (mbDumpBitstream) {
    if (mpDumper) {
      gfDestroyFileDumper(mpDumper);
      mpDumper = NULL;
    }
    TChar filename[128] = {0};
    //snprintf(filename, 127, "rtp_h264_tcp_%04d.h264", mDumpIndex);
    //mDumpIndex ++;
    SDateTime datetime;
    gfGetCurrentDateTime(&datetime);
    snprintf(filename, 127, "rtp_h264_tcp_%04d_%02d_%02d-%02d_%02d_%02d.h264", datetime.year, datetime.month, datetime.day, datetime.hour, datetime.minute, datetime.seconds);
    mpDumper = gfCreateFileDumper(filename);
  }
  return EECode_OK;
}

EECode CRTPScheduledReceiverTCP::Flush()
{
  //todo
  mbRunning = 0;
  return EECode_OK;
}

EECode CRTPScheduledReceiverTCP::Purge()
{
  LOGM_FATAL("need implement\n");
  return EECode_OK;
}

EECode CRTPScheduledReceiverTCP::SetExtraData(TU8 *p_extradata, TU32 extradata_size, TU32 index)
{
  AUTO_LOCK(mpMutex);
  if (EDemuxerVideoOutputPinIndex == index) {
    return doSetVideoExtraData(p_extradata, extradata_size);
  } else {
    return doSetAudioExtraData(p_extradata, extradata_size);
  }
}

void CRTPScheduledReceiverTCP::SetVideoDataPostProcessingCallback(void *callback_context, void *callback)
{
  mpVideoPostProcessingCallbackContext = callback_context;
  mVideoPostProcessingCallback = (TEmbededDataProcessingCallBack)callback;
}

void CRTPScheduledReceiverTCP::SetAudioDataPostProcessingCallback(void *callback_context, void *callback)
{
  LOGM_FATAL("not support!\n");
  return;
}

EECode CRTPScheduledReceiverTCP::Scheduling(TUint times, TU32 inout_mask)
{
  TInt try_read_len = 0;
  TInt read_len = 0;
  AUTO_LOCK(mpMutex);
  if (!mbRunning) {
    return EECode_NotRunning;
  }
  while (mbRunning) {
    if (DUnlikely(DATA_THREAD_STATE_ERROR == msState)) {
      if (DUnlikely(!mSentErrorMsg)) {
        postEngineMsg(EMSGType_UnknownError);
        mSentErrorMsg = 1;
      }
      return EECode_NotRunning;
    } else if (DATA_THREAD_STATE_COMPLETE == msState) {
      return EECode_NotRunning;
    }
    switch (msState) {
      case DATA_THREAD_STATE_READ_FRAME_HEADER: {
          try_read_len = 4 - mCurrentFrameHeaderLength;
          read_len = gfNet_Recv(mRTSPSocket, &mFrameHeader[0] + mCurrentFrameHeaderLength, try_read_len, DNETUTILS_RECEIVE_FLAG_READ_ALL);
          if (read_len == try_read_len) {
            mCurrentFrameHeaderLength = 0;
            if (DUnlikely(DRTP_OVER_RTSP_MAGIC != mFrameHeader[0])) {
              if (('R' == mFrameHeader[0]) && ('T' == mFrameHeader[1]) && ('S' == mFrameHeader[2]) && ('P' == mFrameHeader[3])) {
                LOGM_NOTICE("should be teardown's responce\n");
                postEngineMsg(EMSGType_MissionComplete);
              }
              LOGM_NOTICE("bad frame header: %02x %02x %02x %02x\n", mFrameHeader[0], mFrameHeader[1], mFrameHeader[2], mFrameHeader[3]);
              postEngineMsg(EMSGType_NetworkError);
              mSentErrorMsg = 1;
              msState = DATA_THREAD_STATE_ERROR;
              sendEOS();
              mbRunning = 0;
              break;
            }
            mDataLength = (((TU32) mFrameHeader[2]) << 8) | ((TU32) mFrameHeader[3]);
            if (DUnlikely(mFrameHeader[1])) {
              if (DLikely(mDataLength < 257)) {
                mCurrentRTCPPacketLength = 0;
                msState = DATA_THREAD_STATE_READ_RTCP;
              } else {
                LOGM_FATAL("too long rtcp packet?\n");
                postEngineMsg(EMSGType_NetworkError);
                msState = DATA_THREAD_STATE_ERROR;
                sendEOS();
                mSentErrorMsg = 1;
                mbRunning = 0;
                break;
              }
            } else {
              if (mbReadRemaingVideoData) {
                msState = DATA_THREAD_STATE_READ_RTP_VIDEO_HEADER;
              } else {
                msState = DATA_THREAD_STATE_READ_RTP_HEADER;
              }
              mCurrentRTPHeaderLength = 0;
            }
          } else if (0 < read_len) {
            mCurrentFrameHeaderLength += read_len;
          } else {
            LOGM_ERROR("(0) read error, ret %d\n", read_len);
            postEngineMsg(EMSGType_NetworkError);
            mSentErrorMsg = 1;
            msState = DATA_THREAD_STATE_ERROR;
            sendEOS();
            mbRunning = 0;
          }
        }
        break;
      case DATA_THREAD_STATE_READ_RTCP: {
          try_read_len = mDataLength - mCurrentRTCPPacketLength;
          read_len = gfNet_Recv(mRTSPSocket, mRTCPPacket + mCurrentRTCPPacketLength, try_read_len, DNETUTILS_RECEIVE_FLAG_READ_ALL);
          if (DUnlikely(try_read_len != read_len)) {
            if (0 < read_len) {
              mCurrentRTCPPacketLength += read_len;
            } else {
              LOGM_ERROR("CRTPScheduledReceiverTCP, (1) read error, ret %d\n", read_len);
              postEngineMsg(EMSGType_NetworkError);
              mSentErrorMsg = 1;
              msState = DATA_THREAD_STATE_ERROR;
              sendEOS();
            }
            mbRunning = 0;
            break;
          }
          msState = DATA_THREAD_STATE_READ_FRAME_HEADER;
          if (ERTCPType_BYE == mRTCPPacket[1]) {
            postEngineMsg(EMSGType_MissionComplete);
          } else if (ERTCPType_SR == mRTCPPacket[1]) {
            mLastNTPTime = DREAD_BE64(mRTCPPacket + 8);
            mLastTimestamp = DREAD_BE32(mRTCPPacket + 16);
            if (mbGetSSRC) {
              updateRR();
              sendRR();
            }
          }
          mCurrentFrameHeaderLength = 0;
        }
        break;
      case DATA_THREAD_STATE_READ_RTP_HEADER: {
          try_read_len = DRTP_HEADER_FIXED_LENGTH - mCurrentRTPHeaderLength;
          read_len = gfNet_Recv(mRTSPSocket, mRTPHeader + mCurrentRTPHeaderLength, try_read_len, DNETUTILS_RECEIVE_FLAG_READ_ALL);
          if (DUnlikely(try_read_len != read_len)) {
            if (0 < read_len) {
              mCurrentRTPHeaderLength += read_len;
            } else {
              LOGM_ERROR("CRTPScheduledReceiverTCP, (2) read error, ret %d\n", read_len);
              postEngineMsg(EMSGType_NetworkError);
              mSentErrorMsg = 1;
              msState = DATA_THREAD_STATE_ERROR;
              sendEOS();
            }
            mbRunning = 0;
            break;
          } else {
            if (DUnlikely(mpMemoryStart && mMemorySize)) {
              LOG_WARN("release non-released memory, mpMemoryStart %p, mMemorySize %d\n", mpMemoryStart, mMemorySize);
              releaseMemory();
            }
            mCurPayloadType = mRTPHeader[1] & 0x7f;
            mMarkFlag = mRTPHeader[1] & 0x80;
            if (!mMarkFlag) {
              mHaveFU = 1;
              mbReadRemaingVideoData = 1;
            } else {
              mHaveFU = 0;
            }
            if (mCurPayloadType == mPayloadType[EDemuxerVideoOutputPinIndex]) {
              mCurrentRTPDataLength = 0;
              mFilledStartCode = 0;
              mDataLength -= DRTP_HEADER_FIXED_LENGTH;
              mCurrentTrack = EDemuxerVideoOutputPinIndex;
              requestMemory();
              msState = DATA_THREAD_STATE_READ_RTP_VIDEO_DATA;
              if (mHaveFU) {
                mTotalWritenLength = 3;
              } else {
                mTotalWritenLength = 4;
              }
              __parseRTPPacketHeader(mRTPHeader, mRTPTimeStamp, mRTPCurrentSeqNumber, mPacketSSRC);
              if (DUnlikely(!mbGetSSRC)) {
                mServerSSRC = mPacketSSRC;
                mSSRC = mServerSSRC + 16;
                mbGetSSRC = 1;
                LOGM_INFO("get SSRC 0x%08x\n", mPacketSSRC);
                writeRR();
              }
            } else if (mCurPayloadType == mPayloadType[EDemuxerAudioOutputPinIndex]) {
              mCurrentRTPDataLength = 0;
              mFilledStartCode = 0;
              mDataLength -= DRTP_HEADER_FIXED_LENGTH;
              mCurrentTrack = EDemuxerAudioOutputPinIndex;
              requestMemory();
              msState = DATA_THREAD_STATE_READ_RTP_AAC_HEADER;
              mCurrentAudioCodecSpecificLength = 0;
            }
          }
        }
        break;
      case DATA_THREAD_STATE_READ_RTP_VIDEO_DATA: {
          if (mHaveFU) {
            try_read_len = mDataLength - mCurrentRTPDataLength;
            read_len = gfNet_Recv(mRTSPSocket, mpMemoryStart + mTotalWritenLength, try_read_len, DNETUTILS_RECEIVE_FLAG_READ_ALL);
            if (DUnlikely(try_read_len != read_len)) {
              if (0 < read_len) {
                mCurrentRTPDataLength += read_len;
                mTotalWritenLength += read_len;
              } else {
                LOGM_ERROR("CRTPScheduledReceiverTCP, (3) read error, ret %d\n", read_len);
                postEngineMsg(EMSGType_NetworkError);
                mSentErrorMsg = 1;
                msState = DATA_THREAD_STATE_ERROR;
                sendEOS();
              }
              mbRunning = 0;
              break;
            }
            mTotalWritenLength += read_len;
            if (!mFilledStartCode) {
              mpMemoryStart[4] = (mpMemoryStart[3] & 0x60) | (mpMemoryStart[4] & 0x1f);
              mpMemoryStart[0] = 0x00;
              mpMemoryStart[1] = 0x00;
              mpMemoryStart[2] = 0x00;
              mpMemoryStart[3] = 0x01;
              mFilledStartCode = 1;
            }
            if (mMarkFlag) {
              if (DLikely(!mbWaitFirstSpsPps)) {
                sendVideoData(mpMemoryStart[4] & 0x1f);
              } else if ((ENalType_SPS == (mpMemoryStart[4] & 0x1f)) || (mpH264spspps || mpVideoExtraDataFromSDP)) {
                mbWaitFirstSpsPps = 0;
                sendVideoData(mpMemoryStart[4] & 0x1f);
              } else {
                mpMemPool[EDemuxerVideoOutputPinIndex]->ReturnBackMemBlock((TULong)(mMemorySize), mpMemoryStart);
                mpMemoryStart = NULL;
                mTotalWritenLength = 0;
                mMemorySize = 0;
                LOGM_WARN("discard data due to no sps\n");
                gfPrintMemory(mpMemoryStart, 16);
              }
              mbReadRemaingVideoData = 0;
            }
            mDataLength = 0;
            msState = DATA_THREAD_STATE_READ_FRAME_HEADER;
            mCurrentFrameHeaderLength = 0;
          } else {
            if (!mFilledStartCode) {
              mpMemoryStart[0] = 0x00;
              mpMemoryStart[1] = 0x00;
              mpMemoryStart[2] = 0x00;
              mpMemoryStart[3] = 0x01;
              mFilledStartCode = 1;
            }
            try_read_len = mDataLength - mCurrentRTPDataLength;
            read_len = gfNet_Recv(mRTSPSocket, mpMemoryStart + mTotalWritenLength, try_read_len, DNETUTILS_RECEIVE_FLAG_READ_ALL);
            if (DUnlikely(try_read_len != read_len)) {
              if (0 < read_len) {
                mCurrentRTPDataLength += read_len;
                mTotalWritenLength += read_len;
              } else {
                LOGM_ERROR("CRTPScheduledReceiverTCP, (3) read error, ret %d\n", read_len);
                postEngineMsg(EMSGType_NetworkError);
                mSentErrorMsg = 1;
                msState = DATA_THREAD_STATE_ERROR;
                sendEOS();
              }
              mbRunning = 0;
              break;
            }
            mTotalWritenLength += read_len;
            if (DLikely(!mbWaitFirstSpsPps)) {
              sendVideoData(mpMemoryStart[4] & 0x1f);
            } else if ((ENalType_SPS == (mpMemoryStart[4] & 0x1f)) || mpH264spspps || mpVideoExtraDataFromSDP) {
              mbWaitFirstSpsPps = 0;
              sendVideoData(mpMemoryStart[4] & 0x1f);
            } else {
              mpMemPool[EDemuxerVideoOutputPinIndex]->ReturnBackMemBlock((TULong)(mMemorySize), mpMemoryStart);
              mpMemoryStart = NULL;
              mTotalWritenLength = 0;
              mMemorySize = 0;
            }
            mbReadRemaingVideoData = 0;
            mDataLength = 0;
            msState = DATA_THREAD_STATE_READ_FRAME_HEADER;
            mCurrentFrameHeaderLength = 0;
          }
        }
        break;
      case DATA_THREAD_STATE_READ_RTP_VIDEO_HEADER: {
          try_read_len = DRTP_HEADER_FIXED_LENGTH + 2 - mCurrentRTPHeaderLength;
          read_len = gfNet_Recv(mRTSPSocket, mRTPHeader + mCurrentRTPHeaderLength, try_read_len, DNETUTILS_RECEIVE_FLAG_READ_ALL);
          if (DUnlikely(try_read_len != read_len)) {
            if (0 < read_len) {
              mCurrentRTPHeaderLength += read_len;
            } else {
              LOGM_ERROR("CRTPScheduledReceiverTCP, (4) read error, ret %d\n", read_len);
              postEngineMsg(EMSGType_NetworkError);
              mSentErrorMsg = 1;
              msState = DATA_THREAD_STATE_ERROR;
              sendEOS();
            }
            mbRunning = 0;
            break;
          } else {
            mMarkFlag = mRTPHeader[1] & 0x80;
            mCurrentRTPDataLength = 0;
            mDataLength -= DRTP_HEADER_FIXED_LENGTH + 2;
            msState = DATA_THREAD_STATE_READ_RTP_VIDEO_DATA;
          }
        }
        break;
      case DATA_THREAD_STATE_READ_RTP_AAC_HEADER: {
          try_read_len = 4 - mCurrentAudioCodecSpecificLength;
          read_len = gfNet_Recv(mRTSPSocket, mAACHeader + mCurrentAudioCodecSpecificLength, try_read_len, DNETUTILS_RECEIVE_FLAG_READ_ALL);
          if (DUnlikely(try_read_len != read_len)) {
            if (0 < read_len) {
              mCurrentAudioCodecSpecificLength += read_len;
            } else {
              LOGM_ERROR("CRTPScheduledReceiverTCP, (4) read error, ret %d\n", read_len);
              postEngineMsg(EMSGType_NetworkError);
              mSentErrorMsg = 1;
              msState = DATA_THREAD_STATE_ERROR;
              sendEOS();
            }
            mbRunning = 0;
            break;
          }
          msState = DATA_THREAD_STATE_READ_RTP_AUDIO_DATA;
          mDataLength -= 4;
          mCurrentRTPDataLength = 0;
        }
        break;
      case DATA_THREAD_STATE_READ_RTP_AUDIO_DATA: {
          try_read_len = mDataLength - mCurrentRTPDataLength;
          read_len = gfNet_Recv(mRTSPSocket, mpMemoryStart + mTotalWritenLength, try_read_len, DNETUTILS_RECEIVE_FLAG_READ_ALL);
          if (DUnlikely(try_read_len != read_len)) {
            if (0 < read_len) {
              mCurrentRTPDataLength += read_len;
              mTotalWritenLength += read_len;
            } else {
              LOGM_ERROR("CRTPScheduledReceiverTCP, (4) read error, ret %d\n", read_len);
              postEngineMsg(EMSGType_NetworkError);
              mSentErrorMsg = 1;
              msState = DATA_THREAD_STATE_ERROR;
              sendEOS();
            }
            mbRunning = 0;
            break;
          }
          mTotalWritenLength += read_len;
          sendAudioData();
          mDataLength = 0;
          msState = DATA_THREAD_STATE_READ_FRAME_HEADER;
          mCurrentFrameHeaderLength = 0;
        }
        break;
      case DATA_THREAD_STATE_ERROR:
        // to do, error case
        LOGM_ERROR("error state, DATA_THREAD_STATE_ERROR\n");
        mbRunning = 0;
        break;
      default:
        LOGM_ERROR("unexpected msState %d\n", msState);
        break;
    }
  }
  return EECode_OK;
}

TInt CRTPScheduledReceiverTCP::IsPassiveMode() const
{
  return 0;
}

TSchedulingHandle CRTPScheduledReceiverTCP::GetWaitHandle() const
{
  return (TSchedulingHandle) mRTSPSocket;
}

TSchedulingUnit CRTPScheduledReceiverTCP::HungryScore() const
{
  return 1;
}

TU8 CRTPScheduledReceiverTCP::GetPriority() const
{
  return 1;
}

CObject *CRTPScheduledReceiverTCP::GetObject0() const
{
  return (CObject *) this;
}

EECode CRTPScheduledReceiverTCP::EventHandling(EECode err)
{
  SMSG msg;
  DASSERT(mpMsgSink);
  if (mpMsgSink) {
    if (EECode_TimeOut == err) {
      memset(&msg, 0x0, sizeof(msg));
      msg.code = EMSGType_Timeout;
      msg.owner_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_Demuxer, mIndex);
      msg.owner_type = EGenericComponentType_Demuxer;
      msg.owner_index = mIndex;
      msg.pExtra = NULL;
      msg.needFreePExtra = 0;
      //mpMsgSink->MsgProc(msg);
      return EECode_OK;
    }
  } else {
    return EECode_BadState;
  }
  return EECode_NoImplement;
}

void CRTPScheduledReceiverTCP::PrintStatus()
{
  LOGM_PRINTF("state %d, free video buffer count %d, free audio buffer count %d, reconnect count %d\n", msState, mpBufferPool[EDemuxerVideoOutputPinIndex]->GetFreeBufferCnt(), mpBufferPool[EDemuxerAudioOutputPinIndex]->GetFreeBufferCnt(), mReconnectTag - 1);
  mDebugHeartBeat = 0;
}

void CRTPScheduledReceiverTCP::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
  switch (type) {
    case EEventType_Timer:
      if (DLikely(mpSystemClockReference)) {
        if (DLikely(mpWatchDog)) {
          TTime cur_time = (TTime)param1;
          if (DUnlikely(cur_time > (mLastAliveTimeStamp + mpPersistMediaConfig->streaming_timeout_threashold))) {
            if (mpPersistMediaConfig->rtsp_client_config.watchdog) {
              SMSG msg;
              LOGM_NOTICE("time out detected, cur_time %" DPrint64d ", mLastAliveTimeStamp %" DPrint64d ", diff %" DPrint64d "\n", cur_time, mLastAliveTimeStamp, cur_time - mLastAliveTimeStamp);
              memset(&msg, 0x0, sizeof(msg));
              msg.code = EMSGType_Timeout;
              msg.owner_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_Demuxer, mIndex);
              msg.owner_type = EGenericComponentType_Demuxer;
              msg.owner_index = mIndex;
              msg.p4 = mReconnectSessionID;
              msg.pExtra = NULL;
              msg.needFreePExtra = 0;
              mpSystemClockReference->GuardedRemoveTimer(mpWatchDog);
              mpWatchDog = NULL;
              mpMsgSink->MsgProc(msg);
            }
          } else {
            LOGM_INFO("alive\n");
          }
        } else {
          LOGM_FATAL("NULL mpWatchDog\n");
        }
      }
      break;
    default:
      LOGM_FATAL("BAD type %d\n", type);
      break;
  }
}

void CRTPScheduledReceiverTCP::postEngineMsg(EMSGType msg_type)
{
  SMSG msg;
  if (mpMsgSink) {
    memset(&msg, 0x0, sizeof(msg));
    msg.code = msg_type;
    msg.owner_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_Demuxer, mIndex);
    msg.owner_type = EGenericComponentType_Demuxer;
    msg.owner_index = mIndex;
    msg.p4 = mReconnectSessionID;
    msg.pExtra = NULL;
    msg.needFreePExtra = 0;
    mpMsgSink->MsgProc(msg);
  }
}

void CRTPScheduledReceiverTCP::requestMemory()
{
  mMemorySize = mRequestMemorySize[mCurrentTrack];
  mpMemoryStart = mpMemPool[mCurrentTrack]->RequestMemBlock((TULong) mMemorySize);
  mTotalWritenLength = 0;
}

void CRTPScheduledReceiverTCP::releaseMemory()
{
  mpMemPool[mCurrentTrack]->ReleaseMemBlock((TULong)mMemorySize, mpMemoryStart);
  mMemorySize = 0;
  mpMemoryStart = NULL;
}

void CRTPScheduledReceiverTCP::sendVideoExtraDataBuffer()
{
  mLastAliveTimeStamp = mpSystemClockReference->GetCurrentTime();
  if (DUnlikely(!mpH264spspps && !mpVideoExtraDataFromSDP)) {
    LOG_FATAL("no extradata?\n");
    return;
  }
  CIBuffer *p_buffer = NULL;
  if (DUnlikely(!mpOutputPin[EDemuxerVideoOutputPinIndex]->AllocBuffer(p_buffer))) {
    LOGM_ERROR("CRTPScheduledReceiverTCP AllocBuffer Fail, must not comes here!\n");
  }
  if (DLikely(mpH264spspps)) {
    p_buffer->SetDataPtr(mpH264spspps);
    p_buffer->SetDataSize(mH264spsppsSize);
  } else if (DLikely(mpVideoExtraDataFromSDP)) {
    p_buffer->SetDataPtr(mpVideoExtraDataFromSDP);
    p_buffer->SetDataSize(mVideoExtraDataSize);
  }
  p_buffer->SetBufferFlagBits(CIBuffer::SYNC_POINT | CIBuffer::WITH_EXTRA_DATA | CIBuffer::NEW_SEQUENCE);
  p_buffer->mContentFormat = StreamFormat_H264;
  p_buffer->mVideoWidth = mVideoWidth;
  p_buffer->mVideoHeight = mVideoHeight;
  p_buffer->mVideoOffsetX = 0;
  p_buffer->mVideoOffsetY = 0;
  p_buffer->mVideoFrameRateNum = mVideoFramerateNum;
  p_buffer->mVideoFrameRateDen = mVideoFramerateDen;
  p_buffer->mVideoRoughFrameRate = mVideoFramerate;
  p_buffer->mVideoSampleAspectRatioNum = 1;
  p_buffer->mVideoSampleAspectRatioDen = 1;
  p_buffer->SetDataPtrBase(NULL);
  p_buffer->SetDataMemorySize(0);
  p_buffer->SetBufferType(EBufferType_VideoExtraData);
  p_buffer->mpCustomizedContent = NULL;
  p_buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;
  p_buffer->mContentFormat = mVideoFormat;
  if (mbDumpBitstream && mpDumper) {
    gfFileDump(mpDumper, p_buffer->GetDataPtr(), p_buffer->GetDataSize());
  }
  if (!mbDumpOnly) {
    mpOutputPin[EDemuxerVideoOutputPinIndex]->SendBuffer(p_buffer);
  } else {
    p_buffer->Release();
  }
}

void CRTPScheduledReceiverTCP::sendVideoData(TU8 nal_type)
{
  mLastAliveTimeStamp = mpSystemClockReference->GetCurrentTime();
  if (!mbSendExtraData[EDemuxerVideoOutputPinIndex]) {
    sendVideoExtraDataBuffer();
    mbSendExtraData[EDemuxerVideoOutputPinIndex] = 1;
  }
  if ((ENalType_IDR < nal_type) && (mTotalWritenLength < 128)) {
    LOGM_INFO("skip non-frame, nal_type %d, mTotalWritenLength %d\n", nal_type, mTotalWritenLength);
    mpMemPool[EDemuxerVideoOutputPinIndex]->ReturnBackMemBlock((TULong)mMemorySize, mpMemoryStart);
    mpMemoryStart = NULL;
    mTotalWritenLength = 0;
    mMemorySize = 0;
    return;
  }
  CIBuffer *p_buffer = NULL;
  if (DUnlikely(!mpOutputPin[EDemuxerVideoOutputPinIndex]->AllocBuffer(p_buffer))) {
    LOGM_ERROR("CRTPScheduledReceiverTCP AllocBuffer Fail, must not comes here!\n");
  }
  //LOG_PRINTF("video frame size %d, nal type %d\n", mTotalWritenLength, nal_type);
  if (DUnlikely(ENalType_IDR == nal_type)) {
    p_buffer->SetBufferFlagBits(CIBuffer::KEY_FRAME);
    p_buffer->mVideoFrameType = EPredefinedPictureType_IDR;
  } else if (ENalType_SPS == nal_type) {
    p_buffer->SetBufferFlagBits(CIBuffer::KEY_FRAME);
    p_buffer->mVideoFrameType = EPredefinedPictureType_IDR;
  } else {
    p_buffer->SetBufferFlags(0);
    p_buffer->mVideoFrameType = EPredefinedPictureType_P;
  }
  p_buffer->SetDataPtr(mpMemoryStart);
  p_buffer->SetDataSize((TMemSize) mTotalWritenLength);
  p_buffer->SetDataPtrBase(mpMemoryStart);
  p_buffer->SetDataMemorySize((TMemSize) mTotalWritenLength);
  p_buffer->SetBufferType(EBufferType_VideoES);
  p_buffer->mpCustomizedContent = (void *) mpMemPool[EDemuxerVideoOutputPinIndex];
  p_buffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
  mpMemPool[EDemuxerVideoOutputPinIndex]->ReturnBackMemBlock((TULong)(mMemorySize - mTotalWritenLength), mpMemoryStart + mTotalWritenLength);
  p_buffer->mContentFormat = mVideoFormat;
  if (mVideoPostProcessingCallback && mpVideoPostProcessingCallbackContext) {
    TU32 dataflag = 0;
    if (EPredefinedPictureType_IDR == p_buffer->mVideoFrameType) {
      dataflag = DEmbededDataProcessingReSyncFlag;
    }
    mVideoPostProcessingCallback(mpVideoPostProcessingCallbackContext, p_buffer->GetDataPtr(), p_buffer->GetDataSize(), dataflag);
  }
  if (mbDumpBitstream && mpDumper) {
    gfFileDump(mpDumper, p_buffer->GetDataPtr(), p_buffer->GetDataSize());
  }
  if (!mbDumpOnly) {
    mpOutputPin[EDemuxerVideoOutputPinIndex]->SendBuffer(p_buffer);
  } else {
    p_buffer->Release();
  }
  mpMemoryStart = NULL;
  mTotalWritenLength = 0;
  mMemorySize = 0;
}

void CRTPScheduledReceiverTCP::sendAudioExtraDataBuffer()
{
  mLastAliveTimeStamp = mpSystemClockReference->GetCurrentTime();
  CIBuffer *p_buffer = NULL;
  if (DUnlikely(!mpOutputPin[EDemuxerAudioOutputPinIndex]->AllocBuffer(p_buffer))) {
    LOGM_ERROR("CRTPScheduledReceiverTCP AllocBuffer Fail, must not comes here!\n");
  }
  p_buffer->SetDataPtr(mpAudioExtraData);
  p_buffer->SetDataSize((TMemSize) mAudioExtraDataSize);
  p_buffer->SetBufferFlagBits(CIBuffer::SYNC_POINT | CIBuffer::WITH_EXTRA_DATA | CIBuffer::NEW_SEQUENCE);
  p_buffer->mContentFormat = StreamFormat_AAC;
  p_buffer->mAudioFrameSize  = DDefaultAudioFrameSize;
  p_buffer->mAudioSampleRate = mAudioSamplerate;
  p_buffer->mAudioChannelNumber = mAudioChannelNumber;
  p_buffer->mAudioSampleFormat = mAudioSampleFormat;
  p_buffer->mbAudioPCMChannelInterlave = mbAudioChannelInterlave;
  p_buffer->SetDataPtrBase(NULL);
  p_buffer->SetDataMemorySize(0);
  p_buffer->mpCustomizedContent = NULL;
  p_buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;
  p_buffer->SetBufferType(EBufferType_AudioExtraData);
  p_buffer->mContentFormat = mAudioFormat;
  mpOutputPin[EDemuxerAudioOutputPinIndex]->SendBuffer(p_buffer);
}

void CRTPScheduledReceiverTCP::sendAudioData()
{
  mLastAliveTimeStamp = mpSystemClockReference->GetCurrentTime();
  if (!mbSendExtraData[EDemuxerAudioOutputPinIndex]) {
    sendAudioExtraDataBuffer();
    mbSendExtraData[EDemuxerAudioOutputPinIndex] = 1;
  }
  CIBuffer *p_buffer = NULL;
  if (DUnlikely(!mpOutputPin[EDemuxerAudioOutputPinIndex]->AllocBuffer(p_buffer))) {
    LOGM_ERROR("CRTPScheduledReceiverTCP AllocBuffer Fail, must not comes here!\n");
  }
  //LOG_PRINTF("audio frame size %d\n", mTotalWritenLength);
  p_buffer->SetBufferFlagBits(CIBuffer::KEY_FRAME);
  p_buffer->SetDataPtr(mpMemoryStart);
  p_buffer->SetDataSize((TMemSize) mTotalWritenLength);
  p_buffer->SetDataPtrBase(mpMemoryStart);
  p_buffer->SetDataMemorySize((TMemSize) mTotalWritenLength);
  p_buffer->SetBufferType(EBufferType_AudioES);
  p_buffer->mpCustomizedContent = (void *) mpMemPool[EDemuxerAudioOutputPinIndex];
  p_buffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
  mpMemPool[EDemuxerAudioOutputPinIndex]->ReturnBackMemBlock((TULong)(mMemorySize - mTotalWritenLength), mpMemoryStart + mTotalWritenLength);
  p_buffer->mContentFormat = mAudioFormat;
  mpOutputPin[EDemuxerAudioOutputPinIndex]->SendBuffer(p_buffer);
  mpMemoryStart = NULL;
  mTotalWritenLength = 0;
  mMemorySize = 0;
}

EECode CRTPScheduledReceiverTCP::doSetVideoExtraData(TU8 *p_extradata, TMemSize extradata_size)
{
  static const TU8 startcode[4] = {0, 0, 0, 0x01};
  TUint sps, pps;
  TU8 *pH264spspps_start = NULL;
  if (mpVideoExtraDataFromSDP && (mVideoExtraDataSize == extradata_size)) {
    if (!memcmp(mpVideoExtraDataFromSDP, p_extradata, extradata_size)) {
      //LOGM_WARN("same\n");
      return EECode_OK;
    }
  }
  if (mpVideoExtraDataFromSDP) {
    DDBG_FREE(mpVideoExtraDataFromSDP, "DMVe");
    mpVideoExtraDataFromSDP = NULL;
    mVideoExtraDataSize = 0;
  }
  if (mpH264spspps) {
    DDBG_FREE(mpH264spspps, "DMvE");
    mpH264spspps = NULL;
  }
  DASSERT(!mpVideoExtraDataFromSDP);
  DASSERT(!mVideoExtraDataSize);
  mVideoExtraDataSize = extradata_size;
  LOGM_INFO("video extra data size %d, first 8 bytes: 0x%02x 0x%02x 0x%02x 0x%02x, 0x%02x 0x%02x 0x%02x 0x%02x\n", mVideoExtraDataSize, \
            p_extradata[0], p_extradata[1], p_extradata[2], p_extradata[3], \
            p_extradata[4], p_extradata[5], p_extradata[6], p_extradata[7]);
  mpVideoExtraDataFromSDP = (TU8 *)DDBG_MALLOC(mVideoExtraDataSize, "DMVe");
  if (mpVideoExtraDataFromSDP) {
    memcpy(mpVideoExtraDataFromSDP, p_extradata, extradata_size);
  } else {
    LOGM_FATAL("NO memory\n");
    return EECode_NoMemory;
  }
  if (p_extradata[0] != 0x01) {
    LOGM_INFO("extradata is annex-b format.\n");
  } else {
    mH264spsppsSize = 0;
    DASSERT(!mpH264spspps);
    mpH264spspps = (TU8 *)DDBG_MALLOC(mVideoExtraDataSize + 16, "DMvE");
    if (mpH264spspps) {
      pH264spspps_start = mpH264spspps;
      sps = BE_16(mpVideoExtraDataFromSDP + 6);
      memcpy(mpH264spspps, startcode, sizeof(startcode));
      mpH264spspps += sizeof(startcode);
      mH264spsppsSize += sizeof(startcode);
      memcpy(mpH264spspps, mpVideoExtraDataFromSDP + 8, sps);
      mpH264spspps += sps;
      mH264spsppsSize += sps;
      pps = BE_16(mpVideoExtraDataFromSDP + 6 + 2 + sps + 1);
      memcpy(mpH264spspps, startcode, sizeof(startcode));
      mpH264spspps += sizeof(startcode);
      mH264spsppsSize += sizeof(startcode);
      memcpy(mpH264spspps, mpVideoExtraDataFromSDP + 6 + 2 + sps + 1 + 2, pps);
      mH264spsppsSize += pps;
      mpH264spspps = pH264spspps_start;
    } else {
      LOGM_FATAL("NO memory\n");
      return EECode_NoMemory;
    }
    LOGM_INFO("mH264spsppsSize %d\n", mH264spsppsSize);
  }
  return EECode_OK;
}

EECode CRTPScheduledReceiverTCP::doSetAudioExtraData(TU8 *p_extradata, TMemSize extradata_size)
{
  if (mpAudioExtraData) {
    free(mpAudioExtraData);
    mpAudioExtraData = NULL;
    mAudioExtraDataSize = 0;
  }
  mAudioExtraDataSize = extradata_size;
  mpAudioExtraData = (TU8 *) DDBG_MALLOC(mAudioExtraDataSize, "RAEX");
  memcpy(mpAudioExtraData, p_extradata, mAudioExtraDataSize);
  return EECode_OK;
}

void CRTPScheduledReceiverTCP::sendEOS()
{
  return;
  if (mpOutputPin[EDemuxerVideoOutputPinIndex]) {
    mpOutputPin[EDemuxerVideoOutputPinIndex]->SendBuffer(&mPersistEOSBuffer);
  }
  if (mpOutputPin[EDemuxerAudioOutputPinIndex]) {
    mpOutputPin[EDemuxerAudioOutputPinIndex]->SendBuffer(&mPersistEOSBuffer);
  }
}

void CRTPScheduledReceiverTCP::writeRR()
{
  TU32 len = 0;
  TU32 tmp = 0;
  mRTCPDataLen = 0;
  mRTCPBuffer[0] = DRTP_OVER_RTSP_MAGIC;
  mRTCPBuffer[1] = 1;
  mRTCPBuffer[4] = (RTP_VERSION << 6) + 1;
  mRTCPBuffer[5] = RTCP_RR;
  mRTCPBuffer[6] = 0;
  mRTCPBuffer[7] = 7;
  mRTCPBuffer[8] = (mSSRC >> 24) & 0xff;
  mRTCPBuffer[9] = (mSSRC >> 16) & 0xff;
  mRTCPBuffer[10] = (mSSRC >> 8) & 0xff;
  mRTCPBuffer[11] = (mSSRC) & 0xff;
  mRTCPBuffer[12] = (mServerSSRC >> 24) & 0xff;
  mRTCPBuffer[13] = (mServerSSRC >> 16) & 0xff;
  mRTCPBuffer[14] = (mServerSSRC >> 8) & 0xff;
  mRTCPBuffer[15] = (mServerSSRC) & 0xff;
  mRTCPBuffer[16] = 0;
  mRTCPBuffer[17] = 0;
  mRTCPBuffer[18] = 0;
  mRTCPBuffer[19] = 0;
  mRTCPBuffer[20] = 0;
  mRTCPBuffer[21] = 0;
  mRTCPBuffer[22] = 0;
  mRTCPBuffer[23] = 0;
  mRTCPBuffer[24] = 0;
  mRTCPBuffer[25] = 0;
  mRTCPBuffer[26] = 0;
  mRTCPBuffer[27] = 0;
  mRTCPBuffer[28] = (mLastNTPTime >> 24) & 0xff;
  mRTCPBuffer[29] = (mLastNTPTime >> 16) & 0xff;
  mRTCPBuffer[30] = (mLastNTPTime >> 8) & 0xff;
  mRTCPBuffer[31] = mLastNTPTime & 0xff;
  mRTCPBuffer[32] = 0;
  mRTCPBuffer[33] = 0;
  mRTCPBuffer[34] = 0;
  mRTCPBuffer[35] = 0;
  mRTCPBuffer[36] = (RTP_VERSION << 6) + 1;
  mRTCPBuffer[37] = RTCP_SDES;
  len = strlen(mCName);
  tmp = (6 + len + 3) / 4;
  mRTCPBuffer[38] = (tmp >> 8) & 0xff;
  mRTCPBuffer[39] = tmp & 0xff;
  mRTCPBuffer[40] = (mSSRC >> 24) & 0xff;
  mRTCPBuffer[41] = (mSSRC >> 16) & 0xff;
  mRTCPBuffer[42] = (mSSRC >> 8) & 0xff;
  mRTCPBuffer[43] = (mSSRC) & 0xff;
  mRTCPBuffer[44] = 0x01;
  mRTCPBuffer[45] = len & 0xff;
  DASSERT(len);
  DASSERT((len + 42) < (256 - 8));
  memcpy(mRTCPBuffer + 46, mCName, len);
  mRTCPDataLen = len + 42;
  // padding
  unsigned char *padding = &mRTCPBuffer[46 + len];
  for (len = (6 + len) % 4; len % 4; len++) {
    *padding++ = 0;
    ++mRTCPDataLen;
  }
  mRTCPBuffer[2] = (mRTCPDataLen >> 8) & 0xff;
  mRTCPBuffer[3] = mRTCPDataLen & 0xff;
}

void CRTPScheduledReceiverTCP::updateRR()
{
  mRTCPBuffer[28] = (mLastNTPTime >> 32) & 0xff;
  mRTCPBuffer[29] = (mLastNTPTime >> 24) & 0xff;
  mRTCPBuffer[30] = (mLastNTPTime >> 16) & 0xff;
  mRTCPBuffer[31] = (mLastNTPTime >> 8) & 0xff;
  mRTCPBuffer[32] = 0;
  mRTCPBuffer[33] = 0;
  mRTCPBuffer[34] = 0;
  mRTCPBuffer[35] = 0;
}

void CRTPScheduledReceiverTCP::sendRR()
{
  if (DLikely(DIsSocketHandlerValid(mRTSPSocket))) {
    gfNet_Send(mRTSPSocket, mRTCPBuffer, mRTCPDataLen + 4, 0);
  }
}

#endif

