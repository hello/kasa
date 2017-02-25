/**
 * am_video_decoder.cpp
 *
 * History:
 *    2014/10/23 - [Zhi He] create file
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

#include "am_osal.h"
#include "am_framework.h"
#include "am_modules_interface.h"

#include "am_video_decoder.h"

IFilter *gfCreateVideoDecoderFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
  return CVideoDecoder::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

static EVideoDecoderModuleID _guess_video_decoder_type_from_string(TChar *string)
{
  if (!string) {
    LOG_NOTICE("NULL input in _guess_video_decoder_type_from_string, choose default\n");
    return EVideoDecoderModuleID_AMBADSP;
  }
  if (!strncmp(string, DNameTag_AMBA, strlen(DNameTag_AMBA))) {
    return EVideoDecoderModuleID_AMBADSP;
  } else if (!strncmp(string, DNameTag_FFMpeg, strlen(DNameTag_FFMpeg))) {
    return EVideoDecoderModuleID_FFMpeg;
  }
  LOG_WARN("unknown string tag(%s) in _guess_video_decoder_type_from_string, choose default\n", string);
  return EVideoDecoderModuleID_AMBADSP;
}

//-----------------------------------------------------------------------
//
// CVideoDecoder
//
//-----------------------------------------------------------------------

IFilter *CVideoDecoder::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
{
  CVideoDecoder *result = new CVideoDecoder(pName, pPersistMediaConfig, pEngineMsgSink, index);
  if ((result) && (EECode_OK != result->Construct())) {
    result->Delete();
    result = NULL;
  }
  return result;
}

CVideoDecoder::CVideoDecoder(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
  : inherited(pName, index, pPersistMediaConfig, pEngineMsgSink)
  , mpDecoder(NULL)
  , mCurDecoderID(EVideoDecoderModuleID_Auto)
  , mCurDecoderMode(EDecoderMode_Invalid)
  , mCurrentFormat(StreamFormat_H264)
  , mCustomizedContentFormat(0)
  , mbDecoderContentSetup(0)
  , mbWaitKeyFrame(1)
  , mbDecoderRunning(0)
  , mbDecoderStarted(0)
  , mbDecoderSuspended(0)
  , mbDecoderPaused(0)
  , mbBackward(0)
  , mScanMode(0)
  , mbNeedSendSyncPoint(0)
  , mbNewSequence(0)
  , mbWaitARPrecache(0)
  , mbEnableBypassMode(1)
  , mbIsBypassMode(0)
  , mnCurrentInputPinNumber(0)
  , mpCurInputPin(NULL)
  , mpOutputPin(NULL)
  , mpBufferPool(NULL)
  , mpBuffer(NULL)
  , mCurVideoWidth(0)
  , mCurVideoHeight(0)
  , mVideoFramerateNum(DDefaultVideoFramerateNum)
  , mVideoFramerateDen(DDefaultVideoFramerateDen)
  , mVideoFramerate((float)DDefaultVideoFramerateNum / (float)DDefaultVideoFramerateDen)
{
  TUint i = 0;
  for (i = 0; i < EConstVideoDecoderMaxInputPinNumber; i ++) {
    mpInputPins[i] = NULL;
  }
  mMaxGopSize = 0;
}

EECode CVideoDecoder::Construct()
{
  DSET_MODULE_LOG_CONFIG(ELogModuleVideoDecoderFilter);
  mPersistEOSBuffer.SetBufferType(EBufferType_FlowControl_EOS);
  return inherited::Construct();
}

CVideoDecoder::~CVideoDecoder()
{
}

void CVideoDecoder::Delete()
{
  TUint i = 0;
  if (mpDecoder) {
    mpDecoder->GetObject0()->Delete();
    mpDecoder = NULL;
  }
  for (i = 0; i < EConstVideoDecoderMaxInputPinNumber; i ++) {
    if (mpInputPins[i]) {
      mpInputPins[i]->Delete();
      mpInputPins[i] = NULL;
    }
    mnCurrentInputPinNumber = 0;
  }
  if (mpOutputPin) {
    mpOutputPin->Delete();
    mpOutputPin = NULL;
  }
  if (mpBufferPool) {
    mpBufferPool->Delete();
    mpBufferPool = NULL;
  }
  inherited::Delete();
}

EECode CVideoDecoder::Initialize(TChar *p_param)
{
  EVideoDecoderModuleID id;
  id = _guess_video_decoder_type_from_string(p_param);
  LOGM_INFO("Initialize() start, id %d\n", id);
  if (mbDecoderContentSetup) {
    LOGM_INFO("Initialize() start, there's a decoder already\n");
    DASSERT(mpDecoder);
    if (!mpDecoder) {
      LOGM_FATAL("[Internal bug], why the mpDecoder is NULL here?\n");
      return EECode_InternalLogicalBug;
    }
    if (mbDecoderRunning) {
      mbDecoderRunning = 0;
      LOGM_INFO("before mpDecoder->Stop()\n");
      mpDecoder->Stop();
    }
    if (mbDecoderContentSetup) {
      LOGM_INFO("before mpDecoder->DestroyContext()\n");
      mpDecoder->DestroyContext();
      mbDecoderContentSetup = 0;
    }
    if (id != mCurDecoderID) {
      LOGM_INFO("before mpDecoder->Delete(), cur id %d, request id %d\n", mCurDecoderID, id);
      mpDecoder->GetObject0()->Delete();
      mpDecoder = NULL;
    }
  }
  if (!mpDecoder) {
    mpDecoder = gfModuleFactoryVideoDecoder(id, mpPersistMediaConfig, mpEngineMsgSink);
    if (mpDecoder) {
      mCurDecoderID = id;
      mCurDecoderMode = mpDecoder->GetDecoderMode();
    } else {
      if (!mbEnableBypassMode) {
        mCurDecoderID = EVideoDecoderModuleID_Auto;
        LOGM_FATAL("[Internal bug], request gfModuleFactoryVideoDecoder(%d) fail\n", id);
        return EECode_InternalLogicalBug;
      } else {
        mbIsBypassMode = 1;
        LOGM_CONFIGURATION("decoder is bypass mode\n");
      }
    }
  }
  if (mpDecoder) {
    if (EDecoderMode_Normal == mCurDecoderMode) {
      mpDecoder->SetBufferPool(mpBufferPool);
    } else {
      mpDecoder->SetBufferPoolDirect(mpOutputPin, mpBufferPool);
    }
  }
  LOGM_INFO("Initialize() done\n");
  return EECode_OK;
}

EECode CVideoDecoder::Finalize()
{
  if (mpDecoder) {
    if (mbDecoderRunning) {
      mbDecoderRunning = 0;
      mpDecoder->Stop();
    }
    if (mbDecoderContentSetup) {
      mpDecoder->DestroyContext();
      mbDecoderContentSetup = 0;
    }
    mpDecoder->GetObject0()->Delete();
    mpDecoder = NULL;
    if (mpBufferPool) {
      gfClearBufferMemory((IBufferPool *) mpBufferPool);
    }
  }
  return EECode_OK;
}

EECode CVideoDecoder::Run()
{
  mbDecoderRunning = 1;
  inherited::Run();
  return EECode_OK;
}

EECode CVideoDecoder::Start()
{
  EECode err;
  if (mpBufferPool) {
    mpBufferPool->AddBufferNotifyListener((IEventListener *) this);
  }
  if (mpDecoder) {
    mpDecoder->Start();
    mbDecoderStarted = 1;
  } else {
    if (!mbIsBypassMode) {
      LOGM_FATAL("NULL mpDecoder.\n");
    }
  }
  err = mpWorkQ->SendCmd(ECMDType_Start);
  return err;
}

EECode CVideoDecoder::Stop()
{
  if (mpBufferPool) {
    mpBufferPool->AddBufferNotifyListener(NULL);
  }
  if (mpDecoder && mbDecoderStarted) {
    mpDecoder->Stop();
  }
  mbDecoderStarted = 0;
  inherited::Stop();
  return EECode_OK;
}

EECode CVideoDecoder::SwitchInput(TComponentIndex focus_input_index)
{
  EECode err;
  SCMD cmd;
  cmd.code = ECMDType_SwitchInput;
  cmd.flag = focus_input_index;
  err = mpWorkQ->SendCmd(cmd);
  return err;
}

EECode CVideoDecoder::FlowControl(EBufferType flowcontrol_type)
{
  LOGM_FATAL("TO DO\n");
  return EECode_NoImplement;
}

EECode CVideoDecoder::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
  DASSERT(StreamType_Video == type);
  if (StreamType_Video == type) {
    if (!mpOutputPin) {
      mpOutputPin = COutputPin::Create("[Video output pin for CVideoDecoder filter]", (IFilter *) this);
      if (!mpOutputPin)  {
        LOGM_FATAL("COutputPin::Create() fail?\n");
        return EECode_NoMemory;
      }
      TU32 buffer_number = 6;
      LOGM_CONFIGURATION("video decoder buffer number %d\n", buffer_number);
      mpBufferPool = CBufferPool::Create("[Buffer pool for video output pin in CVideoDecoder filter]", buffer_number);
      if (!mpBufferPool)  {
        LOGM_FATAL("CBufferPool::Create() fail?\n");
        return EECode_NoMemory;
      }
      mpOutputPin->SetBufferPool(mpBufferPool);
    }
    EECode ret = mpOutputPin->AddSubPin(sub_index);
    if (DLikely(EECode_OK == ret)) {
      index = 0;
    } else {
      LOGM_FATAL("mpOutputPin->AddSubPin fail, ret %d, %s\n", ret, gfGetErrorCodeString(ret));
      return ret;
    }
  } else {
    LOGM_ERROR("BAD output pin type %d\n", type);
    return EECode_InternalLogicalBug;
  }
  return EECode_OK;
}

EECode CVideoDecoder::AddInputPin(TUint &index, TUint type)
{
  if (StreamType_Video == type) {
    if (mnCurrentInputPinNumber >= EConstVideoDecoderMaxInputPinNumber) {
      LOGM_ERROR("Max input pin number reached, mnCurrentInputPinNumber %d.\n", mnCurrentInputPinNumber);
      return EECode_InternalLogicalBug;
    }
    index = mnCurrentInputPinNumber;
    DASSERT(!mpInputPins[mnCurrentInputPinNumber]);
    if (mpInputPins[mnCurrentInputPinNumber]) {
      LOGM_FATAL("mpInputPins[mnCurrentInputPinNumber] here, must have problems!!! please check it\n");
    }
    mpInputPins[mnCurrentInputPinNumber] = CQueueInputPin::Create("[Video input pin for CVideoDecoder]", (IFilter *) this, (StreamType) type, mpWorkQ->MsgQ());
    DASSERT(mpInputPins[mnCurrentInputPinNumber]);
    if (!mpCurInputPin) {
      mpCurInputPin = mpInputPins[mnCurrentInputPinNumber];
    } else {
      mpInputPins[mnCurrentInputPinNumber]->Enable(0);
    }
    mnCurrentInputPinNumber ++;
    return EECode_OK;
  } else {
    LOGM_FATAL("BAD input pin type %d\n", type);
    return EECode_InternalLogicalBug;
  }
  return EECode_InternalLogicalBug;
}

IOutputPin *CVideoDecoder::GetOutputPin(TUint index, TUint sub_index)
{
  if (DLikely(!index)) {
    if (DLikely(mpOutputPin)) {
      if (DLikely(sub_index < mpOutputPin->NumberOfPins())) {
        return mpOutputPin;
      } else {
        LOGM_FATAL("BAD sub_index %d\n", sub_index);
      }
    } else {
      LOGM_FATAL("NULL mpOutputPin\n");
    }
  } else {
    LOGM_FATAL("BAD index %d\n", index);
  }
  return NULL;
}

IInputPin *CVideoDecoder::GetInputPin(TUint index)
{
  if (index < mnCurrentInputPinNumber) {
    return mpInputPins[index];
  }
  LOGM_FATAL("BAD index %d, exceed mnCurrentInputPinNumber %d\n", index, mnCurrentInputPinNumber);
  return NULL;
}

EECode CVideoDecoder::FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
  EECode ret = EECode_OK;
  switch (property_type) {
    case EFilterPropertyType_vdecoder_wait_audio_precache:
      mbWaitARPrecache = 1;
      break;
    case EFilterPropertyType_vdecoder_audio_precache_notify:
      mbWaitARPrecache = 0;
      mpWorkQ->PostMsg(ECMDType_NotifyPrecacheDone, NULL);
      break;
    case EFilterPropertyType_video_size:
      LOGM_ERROR("TO DO\n");
      break;
    case EFilterPropertyType_video_framerate:
      LOGM_ERROR("TO DO\n");
      break;
    case EFilterPropertyType_video_format:
      LOGM_ERROR("TO DO\n");
      break;
    case EFilterPropertyType_video_bitrate:
      LOGM_ERROR("TO DO\n");
      break;
    case EFilterPropertyType_vdecoder_pb_speed_feedingrule: {
        SCMD cmd;
        SPbFeedingRule *pb = (SPbFeedingRule *) p_param;
        DASSERT(mpWorkQ);
        DASSERT(p_param);
        if (!p_param) {
          LOGM_FATAL("INVALID p_param=%p\n", p_param);
          ret = EECode_InternalLogicalBug;
          break;
        }
        cmd.code = ECMDType_UpdatePlaybackSpeed;
        cmd.pExtra = NULL;
        cmd.res32_1 = pb->direction;
        cmd.res64_1 = pb->feeding_rule;
        cmd.res64_2 = pb->speed;
        ret = mpWorkQ->SendCmd(cmd);
        DASSERT_OK(ret);
      }
      break;
    default:
      LOGM_FATAL("BAD property 0x%08x\n", property_type);
      ret = EECode_InternalLogicalBug;
      break;
  }
  return ret;
}

void CVideoDecoder::GetInfo(INFO &info)
{
  info.mPriority = 0;
  info.mFlags = 0;
  info.nInput = mnCurrentInputPinNumber;
  info.nOutput = 1;
  info.pName = mpModuleName;
}

void CVideoDecoder::PrintStatus()
{
  TUint i = 0;
  LOGM_PRINTF("\t[PrintStatus] CVideoDecoder[%d]: msState=%d, %s, heart beat %d, mnCurrentInputPinNumber %d\n", mIndex, msState, gfGetModuleStateString(msState), mDebugHeartBeat, mnCurrentInputPinNumber);
  for (i = 0; i < mnCurrentInputPinNumber; i ++) {
    if (mpInputPins[i]) {
      if (mpCurInputPin == mpInputPins[i]) {
        LOGM_PRINTF("\t\t!!current input pin:\n");
      }
      mpInputPins[i]->PrintStatus();
    }
  }
  if (mpOutputPin) {
    mpOutputPin->PrintStatus();
  }
  if (mpDecoder) {
    mpDecoder->GetObject0()->PrintStatus();
  }
  mDebugHeartBeat = 0;
  return;
}

void CVideoDecoder::OnRun()
{
  SCMD cmd;
  CIQueue::QType type;
  IBufferPool *pBufferPool = NULL;
  EECode err = EECode_OK;
  CIBuffer *p_out = NULL;
  mpWorkQ->CmdAck(EECode_OK);
  LOGM_INFO("OnRun loop, start\n");
  mbRun = 1;
  msState = EModuleState_WaitCmd;
  if (mpOutputPin) {
    pBufferPool = mpOutputPin->GetBufferPool();
    DASSERT(pBufferPool);
  } else {
    LOGM_WARN("no output\n");
  }
  mpCurInputPin = mpInputPins[0];//hard code here
  if (!mpCurInputPin) {
    LOGM_ERROR("No input pin\n");
    msState = EModuleState_Error;
  }
  while (mbRun) {
    LOGM_STATE("CVideoDecoder: start switch, msState=%d, %s\n", msState, gfGetModuleStateString(msState));
    switch (msState) {
      case EModuleState_WaitCmd:
        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
        processCmd(cmd);
        break;
      case EModuleState_Idle:
        if (mbPaused) {
          msState = EModuleState_Pending;
          break;
        }
        if (pBufferPool && (pBufferPool->GetFreeBufferCnt() > 0)) {
          msState = EModuleState_HasOutputBuffer;
        } else {
          DASSERT(mpCurInputPin);
          type = mpWorkQ->WaitDataMsgWithSpecifiedQueue(&cmd, sizeof(cmd), mpCurInputPin->GetBufferQ());
          if (type == CIQueue::Q_MSG) {
            processCmd(cmd);
          } else {
            DASSERT(!mpBuffer);
            if (mpCurInputPin->PeekBuffer(mpBuffer)) {
              if (EBufferType_VideoFrameBuffer == mpBuffer->GetBufferType()) {
                if (mpOutputPin) {
                  mpOutputPin->SendBuffer(mpBuffer);
                } else {
                  mpBuffer->Release();
                }
                mpBuffer = NULL;
              } else {
                if (DUnlikely(mbIsBypassMode)) {
                  mpBuffer->Release();
                  mpBuffer = NULL;
                  LOGM_WARN("no decoder, discard packet\n");
                  break;
                }
                msState = EModuleState_HasInputData;
              }
            }
          }
        }
        break;
      case EModuleState_HasOutputBuffer:
        //DASSERT(pBufferPool->GetFreeBufferCnt() > 0);
        //DASSERT(mpCurInputPin);
        type = mpWorkQ->WaitDataMsgWithSpecifiedQueue(&cmd, sizeof(cmd), mpCurInputPin->GetBufferQ());
        if (type == CIQueue::Q_MSG) {
          processCmd(cmd);
        } else {
          DASSERT(!mpBuffer);
          if (mpCurInputPin->PeekBuffer(mpBuffer)) {
            if (EBufferType_VideoFrameBuffer == mpBuffer->GetBufferType()) {
              if (mpOutputPin) {
                mpOutputPin->SendBuffer(mpBuffer);
              } else {
                mpBuffer->Release();
              }
              mpBuffer = NULL;
            } else {
              if (DUnlikely(mbIsBypassMode)) {
                mpBuffer->Release();
                mpBuffer = NULL;
                LOGM_WARN("no decoder, discard packet\n");
                break;
              }
              msState = EModuleState_Running;
            }
          }
        }
        break;
      case EModuleState_Error:
      case EModuleState_Completed:
      case EModuleState_Pending:
        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
        processCmd(cmd);
        break;
      case EModuleState_HasInputData:
        DASSERT(mpBuffer);
        if (pBufferPool && (pBufferPool->GetFreeBufferCnt() > 0)) {
          msState = EModuleState_Running;
        } else {
          mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
          processCmd(cmd);
        }
        break;
      case EModuleState_Running:
        DASSERT(mpBuffer);
        DASSERT(mpDecoder);
        if (DUnlikely(mpBuffer->GetBufferFlags() & CIBuffer::SYNC_POINT)) {
          mMaxGopSize = mpBuffer->mMaxVideoGopSize;
          mCurVideoWidth = mpBuffer->mVideoWidth;
          mCurVideoHeight = mpBuffer->mVideoHeight;
          mVideoFramerateNum = mpBuffer->mVideoFrameRateNum;
          mVideoFramerateDen = mpBuffer->mVideoFrameRateDen;
          mVideoFramerate = mpBuffer->mVideoRoughFrameRate;
          mCurrentFormat = (StreamFormat) mpBuffer->mContentFormat;
          mCustomizedContentFormat = mpBuffer->mCustomizedContentFormat;
          mbNeedSendSyncPoint = 1;
          if (DUnlikely(mpBuffer->GetBufferFlags() & CIBuffer::NEW_SEQUENCE)) {
            mbNewSequence = 1;
          } else {
            mbNewSequence = 0;
          }
        }
        if (DUnlikely(EBufferType_FlowControl_EOS == mpBuffer->GetBufferType())) {
          msState = EModuleState_Completed;
          mPersistEOSBuffer.mNativeDTS = mpBuffer->mNativeDTS;
          mPersistEOSBuffer.mNativePTS = mpBuffer->mNativePTS;
          LOGM_NOTICE("feed bit-stream end, dts %" DPrint64d ", pts %" DPrint64d "\n", mPersistEOSBuffer.mNativeDTS, mPersistEOSBuffer.mNativePTS);
          if (mpOutputPin) {
            mpOutputPin->SendBuffer(&mPersistEOSBuffer);
          }
          msState = EModuleState_Idle;
          break;
        } else if (DUnlikely(EBufferType_VideoExtraData == mpBuffer->GetBufferType())) {
          DASSERT(mpDecoder);
          if (mpDecoder) {
            mpDecoder->SetExtraData(mpBuffer->GetDataPtr(), mpBuffer->GetDataSize());
          }
          mpBuffer->Release();
          mpBuffer = NULL;
          msState = EModuleState_Idle;
          break;
        } else if (DUnlikely(EBufferType_VideoES != mpBuffer->GetBufferType())) {
          LOGM_FATAL("not video es buffer? 0x%08x\n", mpBuffer->GetBufferType());
          flushInputData(1);
          msState = EModuleState_Error;
          break;
        } else if (DUnlikely((mpBuffer->GetBufferFlags() & CIBuffer::BROKEN_FRAME))) {
          if (mpBuffer->GetBufferFlags() & CIBuffer::KEY_FRAME) {
            LOGM_NOTICE("broken idr frame detected, flag 0x%x\n", mpBuffer->GetBufferFlags());
            if (mpDecoder) {
              //mpDecoder->Flush();
            }
            mbWaitKeyFrame = 1;
          }
          mpBuffer->Release();
          mpBuffer = NULL;
          msState = EModuleState_Idle;
          break;
        }
        if (DUnlikely(mbWaitKeyFrame)) {
          if (mpBuffer->GetBufferFlags() & CIBuffer::KEY_FRAME) {
            mbWaitKeyFrame = 0;
            LOGM_NOTICE("wait first key frame done\n");
          } else {
            LOGM_NOTICE("wait first key frame ... flag 0x%08x, data size %ld\n", mpBuffer->GetBufferFlags(), mpBuffer->GetDataSize());
            mpBuffer->Release();
            mpBuffer = NULL;
            msState = EModuleState_Idle;
            break;
          }
        }
        if (DUnlikely(!mbDecoderContentSetup)) {
          SVideoDecoderInputParam input_param;
          memset(&input_param, 0x0, sizeof(input_param));
          input_param.format = mCurrentFormat;
          input_param.customized_content_format = mCustomizedContentFormat;
          input_param.max_gop_size = mMaxGopSize;
          input_param.cap_max_width = mCurVideoWidth;
          input_param.cap_max_height = mCurVideoHeight;
          LOGM_NOTICE("decoder: video resolution %dx%d\n", mCurVideoWidth, mCurVideoHeight);
          err = mpDecoder->SetupContext(&input_param);
          if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("SetupContext(), %d, %s fail\n", err, gfGetErrorCodeString(err));
            msState = EModuleState_Error;
            flushInputData(1);
            break;
          }
          mbDecoderContentSetup = 1;
        }
        if (DLikely(EDecoderMode_Normal == mCurDecoderMode)) {
          err = mpDecoder->Decode(mpBuffer, p_out);
          if (EECode_OK_AlreadyStopped == err) {
            flushInputData(1);
            msState = EModuleState_Completed;
            break;
          } else if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("Decode Fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
            msState = EModuleState_Error;
            mpBuffer->Release();
            mpBuffer = NULL;
            mpCurInputPin->Purge(1);
            if (p_out) {
              p_out->Release();
              p_out = NULL;
            }
            break;
          } else {
            if (p_out) {
              p_out->SetBufferLinearDTS(mpBuffer->GetBufferLinearDTS());
              p_out->SetBufferLinearPTS(mpBuffer->GetBufferLinearPTS());
              p_out->SetBufferNativeDTS(mpBuffer->GetBufferNativeDTS());
              p_out->SetBufferNativePTS(mpBuffer->GetBufferNativePTS());
              p_out->SetBufferDTS(mpBuffer->GetBufferDTS());
              p_out->SetBufferPTS(mpBuffer->GetBufferPTS());
              if (DUnlikely(mbNeedSendSyncPoint)) {
                p_out->mVideoFrameRateNum = mVideoFramerateNum;
                p_out->mVideoFrameRateDen = mVideoFramerateDen;
                p_out->mVideoRoughFrameRate = mVideoFramerate;
                p_out->mVideoWidth = mCurVideoWidth;
                p_out->mVideoHeight = mCurVideoHeight;
                LOGM_NOTICE("decoder: sync buffer resolution %dx%d\n", p_out->mVideoWidth, p_out->mVideoHeight);
                if (mbNewSequence) {
                  p_out->SetBufferFlags(CIBuffer::SYNC_POINT | CIBuffer::NEW_SEQUENCE);
                  mbNewSequence = 0;
                } else {
                  p_out->SetBufferFlags(CIBuffer::SYNC_POINT);
                }
                mbNeedSendSyncPoint = 0;
              } else {
                p_out->SetBufferFlags(0);
              }
              if (mpBuffer->GetBufferFlags() & CIBuffer::LAST_FRAME_INDICATOR) {
                p_out->SetBufferFlagBits(CIBuffer::LAST_FRAME_INDICATOR);
              }
              if (mpOutputPin) {
                mpOutputPin->SendBuffer(p_out);
              } else {
                p_out->Release();
              }
            }
            msState = EModuleState_Idle;
          }
        } else {
          err = mpDecoder->DecodeDirect(mpBuffer);
          if (EECode_OK_AlreadyStopped == err) {
            flushInputData(1);
            msState = EModuleState_Completed;
            break;
          } else if (EECode_OK != err) {
            LOGM_ERROR("decode error\n");
            flushInputData(1);
            msState = EModuleState_Error;
            break;
          }
          msState = EModuleState_Idle;
        }
        mpBuffer->Release();
        mpBuffer = NULL;
        break;
      default:
        LOGM_ERROR("CVideoDecoder:: OnRun: state invalid: 0x%x\n", (TUint)msState);
        flushInputData(1);
        msState = EModuleState_Error;
        break;
    }
    mDebugHeartBeat ++;
  }
  if (mpBufferPool) {
    mpBufferPool->AddBufferNotifyListener(NULL);
  }
  //LOGM_INFO("debug: vd loop end\n");
}

void CVideoDecoder::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
  LOGM_FLOW("EventNotify, event type %d.\n", (TInt)type);
  if (mbDecoderStarted) {
    switch (type) {
      case EEventType_BufferReleaseNotify:
        mpWorkQ->PostMsg(ECMDType_NotifyBufferRelease);
        break;
      default:
        LOGM_ERROR("event type unsupported: %d\n", (TInt)type);
    }
  }
}

void CVideoDecoder::processCmd(SCMD &cmd)
{
  LOGM_FLOW("processCmd, cmd.code %d, %s, state %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code), msState, gfGetModuleStateString(msState));
  switch (cmd.code) {
    case ECMDType_ExitRunning:
      mbRun = 0;
      flushInputData(1);
      msState = EModuleState_Halt;
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_Stop:
      flushInputData(1);
      msState = EModuleState_WaitCmd;
      mpWorkQ->CmdAck(EECode_OK);
      LOGM_INFO("processCmd, ECMDType_Stop.\n");
      break;
    case ECMDType_Pause:
      DASSERT(!mbPaused);
      mbPaused = 1;
      break;
    case ECMDType_Resume:
      msState = EModuleState_Idle;
      mbPaused = 0;
      break;
    case ECMDType_Flush:
      if (mpBuffer) {
        mpBuffer->Release();
        mpBuffer = NULL;
      }
      if (mpCurInputPin) {
        mpCurInputPin->Purge(1);
      }
      if (mpDecoder) {
        mpDecoder->Flush();
      }
      msState = EModuleState_Pending;
      mbWaitKeyFrame = 1;
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_ResumeFlush:
      msState = EModuleState_Idle;
      mbWaitKeyFrame = 1;
      if (mpCurInputPin) {
        mpCurInputPin->Enable(1);
      }
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_Suspend: {
        msState = EModuleState_Pending;
        mbWaitKeyFrame = 1;
        if (mpBuffer) {
          mpBuffer->Release();
          mpBuffer = NULL;
        }
        LOGM_NOTICE("mpCurInputPin %p, mpInputPins[0] %p, mpInputPins[1] %p, cmd.flag %d\n", mpCurInputPin, mpInputPins[0], mpInputPins[1], cmd.flag);
        DASSERT(mpCurInputPin);
        if (DLikely(mpCurInputPin)) {
          mpCurInputPin->Purge(1);
          mpCurInputPin = NULL;
        }
        if (mpDecoder) {
          if ((EReleaseFlag_None != cmd.flag) && mbDecoderRunning) {
            mbDecoderRunning = 0;
            mpDecoder->Stop();
          }
          if ((EReleaseFlag_Destroy == cmd.flag) && mbDecoderContentSetup) {
            mpDecoder->DestroyContext();
            mbDecoderContentSetup = 0;
          }
        }
        mpWorkQ->CmdAck(EECode_OK);
      }
      break;
    case ECMDType_ResumeSuspend: {
        if (DUnlikely(cmd.flag >= mnCurrentInputPinNumber)) {
          LOGM_ERROR("cmd.flag(%d) exceed max value(%d)\n", cmd.flag, mnCurrentInputPinNumber);
          cmd.flag = 0;
        }
        msState = EModuleState_Idle;
        mbWaitKeyFrame = 1;
        CQueueInputPin *p_input = mpInputPins[cmd.flag];
        if (DUnlikely(mpCurInputPin)) {
          LOGM_WARN("mpCurInputPin not NULL, not suspend yet? mpCurInputPin %p, mpInputPins[0] %p, mpInputPins[1] %p, cmd.flag %d\n", mpCurInputPin, mpInputPins[0], mpInputPins[1], cmd.flag);
          if (mpCurInputPin != p_input) {
            mpCurInputPin->Purge(1);
            mpCurInputPin = NULL;
          }
        }
        if (DLikely(p_input)) {
          TU8 *p_extra_data = NULL;
          TMemSize extra_data_size = 0;
          TU32 session_number = 0;
          if (mpDecoder) {
            p_input->GetExtraData(p_extra_data, extra_data_size, session_number);
            mpDecoder->SetExtraData(p_extra_data, extra_data_size);
          }
          if (!mpCurInputPin) {
            mpCurInputPin = p_input;
            mpCurInputPin->Enable(1);
          }
        } else {
          LOGM_ERROR("NULL p_input\n");
          mpWorkQ->CmdAck(EECode_Error);
          break;
        }
        if (DLikely(mpDecoder)) {
          if (!mbDecoderContentSetup) {
            SVideoDecoderInputParam decoder_param;
            memset(&decoder_param, 0x0, sizeof(decoder_param));
            decoder_param.format = mCurrentFormat;
            decoder_param.customized_content_format = mCustomizedContentFormat;
            decoder_param.max_gop_size = mMaxGopSize;
            decoder_param.cap_max_width = mCurVideoWidth;
            decoder_param.cap_max_height = mCurVideoHeight;
            LOGM_NOTICE("decoder: video resolution %dx%d\n", mCurVideoWidth, mCurVideoHeight);
            mpDecoder->SetupContext(&decoder_param);
            mbDecoderContentSetup = 1;
          }
          if (!mbDecoderRunning) {
            mbDecoderRunning = 1;
            mpDecoder->Start();
          }
        } else {
          LOGM_WARN("NULL mpDecoder\n");
        }
        mpWorkQ->CmdAck(EECode_OK);
      }
      break;
    case ECMDType_SwitchInput:
      DASSERT(cmd.flag < mnCurrentInputPinNumber);
      mbWaitKeyFrame = 1;
      if (mpBuffer) {
        mpBuffer->Release();
        mpBuffer = NULL;
      }
      if (cmd.flag < mnCurrentInputPinNumber) {
        if (mpCurInputPin != mpInputPins[cmd.flag]) {
          if (mpCurInputPin) {
            TU8 *p_extra_data = NULL;
            TMemSize extra_data_size = 0;
            TU32 session_number = 0;
            mpCurInputPin->Purge(1);
            mpCurInputPin = mpInputPins[cmd.flag];
            mpCurInputPin->Enable(1);
            if (mpDecoder) {
              mpCurInputPin->GetExtraData(p_extra_data, extra_data_size, session_number);
              mpDecoder->SetExtraData(p_extra_data, extra_data_size);
            }
          }
        } else {
          LOGM_INFO("ECMDType_SwitchInput, focus_input_index %u not changed, do nothing.\n", cmd.flag);
        }
        mpWorkQ->CmdAck(EECode_OK);
      } else {
        LOGM_ERROR("ECMDType_SwitchInput, input pin index %u invalid, mnCurrentInputPinNumber %u \n", cmd.flag, mnCurrentInputPinNumber);
        mpWorkQ->CmdAck(EECode_BadParam);
      }
      break;
    case ECMDType_NotifySynced:
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_Start:
      if ((msState == EModuleState_WaitCmd) && (!mbWaitARPrecache)) {
        msState = EModuleState_Idle;
      }
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_NotifySourceFilterBlocked:
      LOGM_INFO("processCmd, got ECMDType_NotifySourceFilterBlocked\n");
      break;
    case ECMDType_NotifyBufferRelease:
      if (mpOutputPin) {
        if (mpOutputPin->GetBufferPool()->GetFreeBufferCnt() > 0) {
          if (msState == EModuleState_Idle) {
            msState = EModuleState_HasOutputBuffer;
          } else if (msState == EModuleState_HasInputData) {
            msState = EModuleState_Running;
          }
        }
      }
      break;
    case ECMDType_UpdatePlaybackSpeed: {
        if (mpDecoder) {
          mpDecoder->SetPbRule((TU8)cmd.res32_1, (TU8)cmd.res64_1, (TU16)cmd.res64_2);
        }
        mpWorkQ->CmdAck(EECode_OK);
      }
      break;
    case ECMDType_NotifyPrecacheDone:
      if (msState == EModuleState_WaitCmd) {
        msState = EModuleState_Idle;
        LOGM_NOTICE("AR ready, start decoding video\n");
      }
      break;
    default:
      LOGM_ERROR("processCmd, wrong cmd %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code));
      break;
  }
}

EECode CVideoDecoder::flushInputData(TU8 disable_pin)
{
  TUint index = 0;
  if (mpBuffer) {
    mpBuffer->Release();
    mpBuffer = NULL;
  }
  LOGM_INFO("before purge input pins\n");
  for (index = 0; index < EConstVideoDecoderMaxInputPinNumber; index ++) {
    if (mpInputPins[index]) {
      mpInputPins[index]->Purge(disable_pin);
    }
  }
  LOGM_INFO("after purge input pins\n");
  return EECode_OK;
}


