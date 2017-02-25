/*******************************************************************************
 * am_video_es_file_demuxer.cpp
 *
 * History:
 *    2016/04/12 - [Zhi He] create file
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

#include "am_native.h"
#include "am_native_log.h"

#include "am_internal.h"

#include "am_osal.h"
#include "am_framework.h"
#include "am_modules_interface.h"
#include "am_codec_interface.h"

#include "am_video_es_file_demuxer.h"

static EECode __release_ring_buffer(CIBuffer *pBuffer)
{
  DASSERT(pBuffer);
  if (pBuffer) {
    if (EBufferCustomizedContentType_RingBuffer == pBuffer->mCustomizedContentType) {
      IMemPool *pool = (IMemPool *)pBuffer->mpCustomizedContent;
      DASSERT(pool);
      if (pool) {
        pool->ReleaseMemBlock((TULong)(pBuffer->GetDataMemorySize()), pBuffer->GetDataPtrBase());
      }
    }
  } else {
    LOG_FATAL("NULL pBuffer!\n");
    return EECode_InternalLogicalBug;
  }
  return EECode_OK;
}

IDemuxer *gfCreateVideoESFileDemuxerModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
  return CVideoESFileDemuxer::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

IDemuxer *CVideoESFileDemuxer::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU32 index)
{
  CVideoESFileDemuxer *result = new CVideoESFileDemuxer(pname, pPersistMediaConfig, pMsgSink, index);
  if (result && result->Construct() != EECode_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

CObject *CVideoESFileDemuxer::GetObject0() const
{
  return (CObject *) this;
}

CVideoESFileDemuxer::CVideoESFileDemuxer(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU32 index)
  : inherited(pname, index)
  , mpPersistMediaConfig(pPersistMediaConfig)
  , mpMsgSink(NULL)
  , mpCurOutputPin(NULL)
  , mpCurBufferPool(NULL)
  , mpCurMemPool(NULL)
  , mbRun(1)
  , mbPaused(0)
  , mbTobeStopped(0)
  , mbTobeSuspended(0)
  , msState(EModuleState_Invalid)
  , mVideoFormat(StreamFormat_Invalid)
{
  for (TUint i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
    mpOutputPins[i] = NULL;
    mpBufferPool[i] = NULL;
    mpMemPool[i] = NULL;
  }
  mpMediaParser = NULL;
  mVideoWidth = 0;
  mVideoHeight = 0;
  mpExtradata = NULL;
  mExtradataLen = 0;
  mVideoTimeScale = DDefaultTimeScale;
  mVideoFrameTick = DDefaultVideoFramerateDen;
  mCurrentTimestamp = 0;
}

EECode CVideoESFileDemuxer::Construct()
{
  //mConfigLogLevel = ELogLevel_Info;
  mPersistVideoEOSBuffer.SetBufferType(EBufferType_FlowControl_EOS);
  mPersistVideoEOSBuffer.mCustomizedContentType = EBufferCustomizedContentType_Invalid;
  mPersistVideoEOSBuffer.mpCustomizedContent = NULL;
  mPersistVideoEOSBuffer.mpPool = NULL;
  return inherited::Construct();
}

void CVideoESFileDemuxer::Delete()
{
  if (mpMediaParser) {
    mpMediaParser->Delete();
    mpMediaParser = NULL;
  }
  inherited::Delete();
}

CVideoESFileDemuxer::~CVideoESFileDemuxer()
{
}

EECode CVideoESFileDemuxer::SetupOutput(COutputPin *p_output_pins [ ], CBufferPool *p_bufferpools [ ],  IMemPool *p_mempools[], IMsgSink *p_msg_sink)
{
  DASSERT(!mpBufferPool[EDemuxerVideoOutputPinIndex]);
  DASSERT(!mpOutputPins[EDemuxerVideoOutputPinIndex]);
  DASSERT(!mpMsgSink);
  mpOutputPins[EDemuxerVideoOutputPinIndex] = (COutputPin *)p_output_pins[EDemuxerVideoOutputPinIndex];
  mpBufferPool[EDemuxerVideoOutputPinIndex] = (CBufferPool *)p_bufferpools[EDemuxerVideoOutputPinIndex];
  mpMsgSink = p_msg_sink;
  if (mpOutputPins[EDemuxerVideoOutputPinIndex] && mpBufferPool[EDemuxerVideoOutputPinIndex]) {
    mpBufferPool[EDemuxerVideoOutputPinIndex]->SetReleaseBufferCallBack(__release_ring_buffer);
    mpBufferPool[EDemuxerVideoOutputPinIndex]->AddBufferNotifyListener((IEventListener *)this);
    mpMemPool[EDemuxerVideoOutputPinIndex] = p_mempools[EDemuxerVideoOutputPinIndex];
  }
  return EECode_OK;
}

EECode CVideoESFileDemuxer::SetupContext(TChar *url, void *p_agent, TU8 priority, TU32 request_receive_buffer_size, TU32 request_send_buffer_size)
{
  EECode err;
  TChar *p_ext = NULL;
  if (DUnlikely(!url)) {
    LOGM_ERROR("NULL url\n");
    return EECode_BadParam;
  }
  p_ext = strrchr(url, '.');
  if (p_ext) {
    if ((!strcmp(p_ext, ".h264")) || (!strcmp(p_ext, ".H264")) || (!strcmp(p_ext, ".264"))
        || (!strcmp(p_ext, ".avc")) || (!strcmp(p_ext, ".AVC"))) {
      mVideoFormat = StreamFormat_H264;
    } else if ((!strcmp(p_ext, ".h265")) || (!strcmp(p_ext, ".H265")) || (!strcmp(p_ext, ".265"))
               || (!strcmp(p_ext, ".hevc")) || (!strcmp(p_ext, ".HEVC"))) {
      mVideoFormat = StreamFormat_H265;
    } else if ((!strncmp(p_ext, ".mjpeg", strlen(".mjpeg"))) || (!strncmp(p_ext, ".MJPEG", strlen(".MJPEG")))
               || (!strncmp(p_ext, ".mjpg", strlen(".mjpg"))) || (!strncmp(p_ext, ".MJPG", strlen(".MJPG")))
               || (!strncmp(p_ext, ".jpeg", strlen(".jpeg"))) || (!strncmp(p_ext, ".JPEG", strlen(".JPEG")))
               || (!strncmp(p_ext, ".jpg", strlen(".jpg"))) || (!strncmp(p_ext, ".JPG", strlen(".JPG")))) {
      mVideoFormat = StreamFormat_JPEG;
    } else {
      mVideoFormat = StreamFormat_H264;
      LOGM_WARN("guess file format: h264\n");
    }
  }
  if (StreamFormat_H264 == mVideoFormat) {
    mpMediaParser = gfCreateMediaFileParser(EMediaFileParser_BasicAVC);
  } else if (StreamFormat_H265 == mVideoFormat) {
    mpMediaParser = gfCreateMediaFileParser(EMediaFileParser_BasicHEVC);
  } else if (StreamFormat_JPEG == mVideoFormat) {
    mpMediaParser = gfCreateMediaFileParser(EMediaFileParser_BasicMJPEG);
  } else {
    LOGM_ERROR("not supported format %x\n", mVideoFormat);
    return EECode_BadParam;
  }
  err = mpMediaParser->OpenFile(url);
  if (DUnlikely(EECode_OK != err)) {
    LOGM_ERROR("OpenFile(%s) fail, ret %x, %s\n", url, err, gfGetErrorCodeString(err));
    return err;
  }
  if (mpWorkQ) {
    LOGM_INFO("before SendCmd(ECMDType_StartRunning)...\n");
    err = mpWorkQ->SendCmd(ECMDType_StartRunning);
    LOGM_INFO("after SendCmd(ECMDType_StartRunning)\n");
    DASSERT_OK(err);
  } else {
    LOGM_FATAL("NULL mpWorkQ?\n");
    return EECode_InternalLogicalBug;
  }
  return EECode_OK;
}

EECode CVideoESFileDemuxer::DestroyContext()
{
  if (mpWorkQ) {
    mpWorkQ->SendCmd(ECMDType_ExitRunning);
  }
  return EECode_OK;
}

EECode CVideoESFileDemuxer::ReconnectServer()
{
  return EECode_OK;
}

EECode CVideoESFileDemuxer::Seek(TTime &target_time, ENavigationPosition position)
{
  SInternalCMDSeek seek;
  SCMD cmd;
  EECode err;
  seek.target = target_time;
  seek.position = position;
  cmd.code = ECMDType_Seek;
  cmd.pExtra = (void *) &seek;
  err = mpWorkQ->SendCmd(cmd);
  target_time = seek.target;
  return err;
}

EECode CVideoESFileDemuxer::Start()
{
  EECode err = EECode_OK;
  DASSERT(mpWorkQ);
  LOGM_INFO("before SendCmd(ECMDType_Start)...\n");
  err = mpWorkQ->SendCmd(ECMDType_Start);
  LOGM_INFO("after SendCmd(ECMDType_Start)\n");
  DASSERT_OK(err);
  return err;
}

EECode CVideoESFileDemuxer::Stop()
{
  EECode err = EECode_OK;
  DASSERT(mpWorkQ);
  LOGM_INFO("before SendCmd(ECMDType_Stop)...\n");
  err = mpWorkQ->SendCmd(ECMDType_Stop);
  LOGM_INFO("after SendCmd(ECMDType_Stop)\n");
  DASSERT_OK(err);
  return err;
}

EECode CVideoESFileDemuxer::Suspend()
{
  return EECode_OK;
}

EECode CVideoESFileDemuxer::Pause()
{
  EECode err = EECode_OK;
  DASSERT(mpWorkQ);
  err = mpWorkQ->PostMsg(ECMDType_Pause);
  DASSERT_OK(err);
  return err;
}

EECode CVideoESFileDemuxer::Resume()
{
  EECode err = EECode_OK;
  DASSERT(mpWorkQ);
  err = mpWorkQ->PostMsg(ECMDType_Resume);
  DASSERT_OK(err);
  return err;
}

EECode CVideoESFileDemuxer::Flush()
{
  return mpWorkQ->SendCmd(ECMDType_Flush);
}

EECode CVideoESFileDemuxer::ResumeFlush()
{
  return mpWorkQ->SendCmd(ECMDType_ResumeFlush);
}

EECode CVideoESFileDemuxer::Purge()
{
  return EECode_OK;
}

EECode CVideoESFileDemuxer::SetPbRule(TU8 direction, TU8 feeding_rule, TU16 speed)
{
  EECode err = EECode_OK;
  SCMD cmd;
  if (DLikely(mpWorkQ)) {
    cmd.code = ECMDType_UpdatePlaybackSpeed;
    cmd.pExtra = NULL;
    cmd.res32_1 = direction;
    cmd.res64_1 = feeding_rule;
    err = mpWorkQ->PostMsg(cmd);
  } else {
    LOGM_FATAL("NULL mpWorkQ or NULL p_feedingrule\n");
    return EECode_BadState;
  }
  return err;
}

EECode CVideoESFileDemuxer::SetPbLoopMode(TU32 *p_loop_mode)
{
  EECode err = EECode_OK;
  SCMD cmd;
  if (DLikely(mpWorkQ && p_loop_mode)) {
    cmd.code = ECMDType_UpdatePlaybackLoopMode;
    cmd.pExtra = NULL;
    cmd.res32_1 = *p_loop_mode;
    err = mpWorkQ->PostMsg(cmd);
  } else {
    LOGM_FATAL("NULL mpWorkQ or NULL p_loop_mode\n");
    return EECode_BadState;
  }
  return err;
}

EECode CVideoESFileDemuxer::EnableVideo(TU32 enable)
{
  SCMD cmd;
  cmd.code = ECMDType_EnableVideo;
  cmd.res32_1 = enable;
  cmd.pExtra = NULL;
  mpWorkQ->PostMsg(cmd);
  return EECode_OK;
}

EECode CVideoESFileDemuxer::EnableAudio(TU32 enable)
{
  SCMD cmd;
  cmd.code = ECMDType_EnableAudio;
  cmd.res32_1 = enable;
  cmd.pExtra = NULL;
  mpWorkQ->PostMsg(cmd);
  return EECode_OK;
}

EECode CVideoESFileDemuxer::SetVideoPostProcessingCallback(void *callback_context, void *callback)
{
  return EECode_OK;
}

EECode CVideoESFileDemuxer::SetAudioPostProcessingCallback(void *callback_context, void *callback)
{
  return EECode_OK;
}

void CVideoESFileDemuxer::PrintStatus()
{
  LOGM_PRINTF("heart beat %d, msState %s\n", mDebugHeartBeat, gfGetModuleStateString(msState));
  if (mpMemPool[EDemuxerVideoOutputPinIndex]) {
    mpMemPool[EDemuxerVideoOutputPinIndex]->GetObject0()->PrintStatus();
  }
  mDebugHeartBeat = 0;
}

void CVideoESFileDemuxer::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
  EECode err = EECode_OK;
  DASSERT(mpWorkQ);
  LOGM_FLOW("before mpWorkQ->PostMsg(ECMDType_NotifyBufferRelease)...\n");
  err = mpWorkQ->PostMsg(ECMDType_NotifyBufferRelease, NULL);
  LOGM_FLOW("after mpWorkQ->PostMsg(ECMDType_NotifyBufferRelease), ret %d\n", err);
  DASSERT_OK(err);
  return;
}

EECode CVideoESFileDemuxer::QueryContentInfo(const SStreamCodecInfos *&pinfos) const
{
  return EECode_OK;
}

EECode CVideoESFileDemuxer::UpdateContext(SContextInfo *pContext)
{
  return EECode_OK;
}

EECode CVideoESFileDemuxer::GetExtraData(SStreamingSessionContent *pContent)
{
  return EECode_NoImplement;
}

EECode CVideoESFileDemuxer::NavigationSeek(SContextInfo *info)
{
  return EECode_NoImplement;
}

EECode CVideoESFileDemuxer::ResumeChannel()
{
  return EECode_OK;
}

void CVideoESFileDemuxer::OnRun()
{
  SCMD cmd;
  CIBuffer *pBuffer = NULL;
  EECode err = EECode_OK;
  msState = EModuleState_WaitCmd;
  TU8 *p;
  TInt need_send_first_sync_point = 0;
  SMediaPacket packet;
  mbRun = 1;
  CmdAck(EECode_OK);
  mpCurOutputPin = mpOutputPins[EDemuxerVideoOutputPinIndex];
  mpCurBufferPool = mpBufferPool[EDemuxerVideoOutputPinIndex];
  mpCurMemPool = mpMemPool[EDemuxerVideoOutputPinIndex];
  err = getMediaInfo();
  DASSERT_OK(err);
  if (mpExtradata && mExtradataLen) {
    err = sendExtraData();
    DASSERT_OK(err);
  } else {
    need_send_first_sync_point = 1;
  }
  while (mbRun) {
    LOGM_STATE("state(%s, %d)\n", gfGetModuleStateString(msState), msState);
    switch (msState) {
      case EModuleState_WaitCmd:
        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
        processCmd(cmd);
        break;
      case EModuleState_Idle:
        if (mpWorkQ->PeekCmd(cmd)) {
          processCmd(cmd);
        } else {
          err = mpMediaParser->ReadPacket(&packet);
          if (DUnlikely(EECode_OK_EOF == err)) {
            sendEOSBuffer();
            msState = EModuleState_Pending;
            LOGM_NOTICE("eof comes\n");
            break;
          } else if (DUnlikely(EECode_OK != err)) {
            msState = EModuleState_Error;
            LOGM_ERROR("parse fail\n");
            break;
          }
          msState = EModuleState_Running;
        }
        break;
      case EModuleState_HasInputData:
        DASSERT(mpCurBufferPool);
        if (mpCurBufferPool->GetFreeBufferCnt() > 0) {
          msState = EModuleState_Running;
        } else {
          mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
          processCmd(cmd);
        }
        break;
      case EModuleState_Running:
        DASSERT(!pBuffer);
        pBuffer = NULL;
        if (!mpCurOutputPin->AllocBuffer(pBuffer)) {
          LOGM_FATAL("AllocBuffer() fail? must have internal bugs.\n");
          msState = EModuleState_Error;
          break;
        }
        p = mpCurMemPool->RequestMemBlock((TULong) packet.data_len);
        pBuffer->SetBufferFlags(0);
        pBuffer->SetBufferType(EBufferType_VideoES);
        pBuffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
        pBuffer->mpCustomizedContent = (void *)mpCurMemPool;
        if (need_send_first_sync_point) {
          need_send_first_sync_point = 0;
          pBuffer->SetBufferFlagBits(CIBuffer::SYNC_POINT);
          pBuffer->mVideoOffsetX = 0;
          pBuffer->mVideoOffsetY = 0;
          pBuffer->mVideoWidth = mVideoWidth;
          pBuffer->mVideoHeight = mVideoHeight;
          pBuffer->mVideoBitrate = 1000000;//hard code here
          pBuffer->mContentFormat = mVideoFormat;
          pBuffer->mCustomizedContentFormat = 0;
          pBuffer->mVideoFrameRateNum = DDefaultVideoFramerateNum;
          pBuffer->mVideoFrameRateDen = DDefaultVideoFramerateDen;//hard code here
          if (pBuffer->mVideoFrameRateNum && pBuffer->mVideoFrameRateDen) {
            pBuffer->mVideoRoughFrameRate = (float)pBuffer->mVideoFrameRateNum / (float)pBuffer->mVideoFrameRateDen;
          }
          pBuffer->mVideoSampleAspectRatioNum = 1;
          pBuffer->mVideoSampleAspectRatioDen = 1;
        }
        if (DUnlikely(EPredefinedPictureType_IDR == packet.frame_type)) {
          pBuffer->SetBufferFlagBits(CIBuffer::KEY_FRAME);
          pBuffer->mVideoFrameType = EPredefinedPictureType_IDR;
        } else {
          pBuffer->mVideoFrameType = EPredefinedPictureType_P;
        }
        if (DUnlikely(packet.is_last_frame)) {
          pBuffer->SetBufferFlagBits(CIBuffer::LAST_FRAME_INDICATOR);
        }
        memcpy(p, packet.p_data, packet.data_len);
        pBuffer->SetDataPtr(p);
        pBuffer->SetDataSize(packet.data_len);
        pBuffer->SetDataPtrBase(p);
        pBuffer->SetDataMemorySize(packet.data_len);
        pBuffer->SetBufferLinearDTS(mCurrentTimestamp);
        pBuffer->SetBufferLinearPTS(mCurrentTimestamp);
        pBuffer->SetBufferNativeDTS(mCurrentTimestamp);
        pBuffer->SetBufferNativePTS(mCurrentTimestamp);
        mCurrentTimestamp += mVideoFrameTick;
        mpCurOutputPin->SendBuffer(pBuffer);
        pBuffer = NULL;
        msState = EModuleState_Idle;
        break;
      case EModuleState_Completed:
      case EModuleState_Pending:
      case EModuleState_Error:
        if (pBuffer) {
          LOGM_WARN("pBuffer\n");
          pBuffer->Release();
          pBuffer = NULL;
        }
        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
        processCmd(cmd);
        break;
      default:
        LOGM_ERROR("Check Me.\n");
        break;
    }
  }
  LOGM_INFO("CVideoESFileDemuxer OnRun Exit.\n");
  CmdAck(EECode_OK);
}

EECode CVideoESFileDemuxer::getMediaInfo()
{
  EECode err = EECode_OK;
  SMediaInfo info;
  err = mpMediaParser->GetMediaInfo(&info);
  if (EECode_OK != err) {
    LOGM_ERROR("get media info fail, ret 0x%08x, %s\n", err, gfGetErrorCodeString(err));
    return err;
  }
  mVideoWidth = info.video_width;
  mVideoHeight = info.video_height;
  err = mpMediaParser->GetExtradata(mpExtradata, mExtradataLen);
  if (EECode_OK != err) {
    LOGM_ERROR("get extradata fail, ret 0x%08x, %s\n", err, gfGetErrorCodeString(err));
    return err;
  }
  return EECode_OK;
}

EECode CVideoESFileDemuxer::sendExtraData()
{
  if (mpOutputPins[EDemuxerVideoOutputPinIndex]) {
    CIBuffer *buffer = NULL;
    DASSERT(mpExtradata);
    DASSERT(mExtradataLen);
    mpOutputPins[EDemuxerVideoOutputPinIndex]->AllocBuffer(buffer);
    buffer->SetDataPtr(mpExtradata);
    buffer->SetDataSize(mExtradataLen);
    buffer->SetBufferType(EBufferType_VideoExtraData);
    buffer->SetBufferFlags(CIBuffer::SYNC_POINT);
    buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;
    buffer->mpCustomizedContent = NULL;
    buffer->mVideoOffsetX = 0;
    buffer->mVideoOffsetY = 0;
    buffer->mVideoWidth = mVideoWidth;
    buffer->mVideoHeight = mVideoHeight;
    buffer->mVideoBitrate = 1000000;//hard code here
    buffer->mContentFormat = mVideoFormat;
    buffer->mCustomizedContentFormat = 0;
    buffer->mVideoFrameRateNum = DDefaultVideoFramerateNum;
    buffer->mVideoFrameRateDen = DDefaultVideoFramerateDen;//hard code here
    if (buffer->mVideoFrameRateNum && buffer->mVideoFrameRateDen) {
      buffer->mVideoRoughFrameRate = (float)buffer->mVideoFrameRateNum / (float)buffer->mVideoFrameRateDen;
    }
    buffer->mVideoSampleAspectRatioNum = 1;
    buffer->mVideoSampleAspectRatioDen = 1;
    LOGM_CONFIGURATION("send video extra data, %dx%d, rough framerate %f = %d/%d\n", buffer->mVideoWidth, buffer->mVideoHeight,
                       buffer->mVideoRoughFrameRate, buffer->mVideoFrameRateNum, buffer->mVideoFrameRateDen);
    mpOutputPins[EDemuxerVideoOutputPinIndex]->SendBuffer(buffer);
    buffer = NULL;
  } else {
    LOGM_FATAL("no video output pin\n");
  }
  return EECode_OK;
}

EECode CVideoESFileDemuxer::processCmd(SCMD &cmd)
{
  switch (cmd.code) {
    case ECMDType_ExitRunning:
      mbRun = 0;
      CmdAck(EECode_OK);
      break;
    case ECMDType_Start:
      DASSERT(EModuleState_WaitCmd == msState);
      msState = EModuleState_Idle;
      CmdAck(EECode_OK);
      break;
    case ECMDType_Stop:
      msState = EModuleState_Pending;
      CmdAck(EECode_OK);
      break;
    case ECMDType_Pause:
      mbPaused = 1;
      msState = EModuleState_Pending;
      break;
    case ECMDType_Resume:
      if (EModuleState_Pending == msState) {
        msState = EModuleState_Idle;
      } else if (EModuleState_Completed == msState) {
        msState = EModuleState_Idle;
      } else if (EModuleState_Stopped == msState) {
        msState = EModuleState_Idle;
      }
      mbPaused = 0;
      break;
    case ECMDType_Flush:
      msState = EModuleState_Pending;
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_ResumeFlush:
      msState = EModuleState_Idle;
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_Seek:
      CmdAck(EECode_OK);
      break;
    case ECMDType_NotifyBufferRelease:
      if (mpCurBufferPool && mpCurBufferPool->GetFreeBufferCnt() > 0 && msState == EModuleState_HasInputData) {
        msState = EModuleState_Running;
      }
      break;
    case ECMDType_UpdatePlaybackSpeed: {
      }
      break;
    case ECMDType_UpdatePlaybackLoopMode:
      break;
    case ECMDType_ResumeChannel:
      msState = EModuleState_Idle;
      CmdAck(EECode_OK);
      break;
    default:
      LOG_WARN("not processed cmd %x\n", cmd.code);
      break;
  }
  return EECode_OK;
}

void CVideoESFileDemuxer::sendEOSBuffer()
{
  if (mpOutputPins[EDemuxerVideoOutputPinIndex]) {
    mPersistVideoEOSBuffer.mNativeDTS = mPersistVideoEOSBuffer.mNativePTS = mCurrentTimestamp;
    mpOutputPins[EDemuxerVideoOutputPinIndex]->SendBuffer(&mPersistVideoEOSBuffer);
  }
}


