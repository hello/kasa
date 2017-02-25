/*******************************************************************************
 * am_video_encoder.cpp
 *
 * History:
 *    2014/10/29 - [Zhi He] create file
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

#include "am_video_encoder.h"

IFilter *gfCreateVideoEncoderFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
  return CVideoEncoderV2::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

static EVideoEncoderModuleID _guess_video_encoder_type_from_string(TChar *string)
{
  if (!string) {
    LOG_NOTICE("NULL input in _guess_video_encoder_type_from_string, choose default\n");
    return EVideoEncoderModuleID_AMBADSP;
  }
  if (!strncmp(string, DNameTag_AMBA, strlen(DNameTag_AMBA))) {
    return EVideoEncoderModuleID_AMBADSP;
  }
  LOG_WARN("unknown string tag(%s) in _guess_video_encoder_type_from_string, choose default\n", string);
  return EVideoEncoderModuleID_AMBADSP;
}

//-----------------------------------------------------------------------
//
// CVideoEncoderV2
//
//-----------------------------------------------------------------------

IFilter *CVideoEncoderV2::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
{
  CVideoEncoderV2 *result = new CVideoEncoderV2(pName, pPersistMediaConfig, pEngineMsgSink, index);
  if ((result) && (EECode_OK != result->Construct())) {
    result->Delete();
    result = NULL;
  }
  return result;
}

CVideoEncoderV2::CVideoEncoderV2(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
  : inherited(pName, index, pPersistMediaConfig, pEngineMsgSink)
  , mpEncoder(NULL)
  , mCurEncoderID(EVideoEncoderModuleID_Auto)
  , mbEncoderContentSetup(0)
  //, mbWaitKeyFrame(1)
  , mbEncoderStarted(0)
  , mnCurrentInputPinNumber(0)
  , mpCurInputPin(NULL)
  , mnCurrentOutputPinNumber(0)
  , mpBufferPool(NULL)
  , mpInputBuffer(NULL)
  , mpBuffer(NULL)
  , mpCachedInputBuffer(NULL)
{
  DASSERT(mpPersistMediaConfig);
  if (mpPersistMediaConfig) {
    mpSystemClockReference = mpPersistMediaConfig->p_system_clock_reference;
  }
  //DASSERT(mpSystemClockReference);
  for (TU32 i = 0; i < EConstVideoEncoderMaxInputPinNumber; i ++) {
    mpInputPins[i] = NULL;
  }
  for (TU32 i = 0; i < DMaxStreamNumber; i ++) {
    mpOutputPins[i] = NULL;
  }
  mStreamID = 0;
}

EECode CVideoEncoderV2::Construct()
{
  DSET_MODULE_LOG_CONFIG(ELogModuleVideoDecoderFilter);
  mPersistEOSBuffer.SetBufferType(EBufferType_FlowControl_EOS);
  return inherited::Construct();
}

CVideoEncoderV2::~CVideoEncoderV2()
{
}

void CVideoEncoderV2::Delete()
{
  if (mpEncoder) {
    mpEncoder->Destroy();
    mpEncoder = NULL;
  }
  TU32 i = 0;
  for (i = 0; i < EConstVideoEncoderMaxInputPinNumber; i ++) {
    if (mpInputPins[i]) {
      mpInputPins[i]->Delete();
      mpInputPins[i] = NULL;
    }
  }
  for (i = 0; i < DMaxStreamNumber; i ++) {
    if (mpOutputPins[i]) {
      mpOutputPins[i]->Delete();
      mpOutputPins[i] = NULL;
    }
  }
  if (mpBufferPool) {
    mpBufferPool->Delete();
    mpBufferPool = NULL;
  }
  inherited::Delete();
}

EECode CVideoEncoderV2::Initialize(TChar *p_param)
{
  EVideoEncoderModuleID id;
  EECode err = EECode_OK;
  id = _guess_video_encoder_type_from_string(p_param);
  LOGM_INFO("[Video Encoder flow], CVideoEncoderV2::Initialize() start, id %d\n", id);
  if (mbEncoderContentSetup) {
    LOGM_INFO("[Video Encoder flow], CVideoEncoderV2::Initialize() start, there's a decoder already\n");
    DASSERT(mpEncoder);
    if (!mpEncoder) {
      LOGM_FATAL("[Internal bug], why the mpEncoder is NULL here?\n");
      return EECode_InternalLogicalBug;
    }
    if (mbEncoderStarted) {
      mbEncoderStarted = 0;
      LOGM_INFO("[EncoderFilter flow], before mpEncoder->Stop()\n");
      mpEncoder->Stop();
    }
    LOGM_INFO("[EncoderFilter flow], before mpEncoder->DestroyContext()\n");
    mpEncoder->DestroyContext();
    mbEncoderContentSetup = 0;
    if (id != mCurEncoderID) {
      LOGM_INFO("[EncoderFilter flow], before mpEncoder->Delete(), cur id %d, request id %d\n", mCurEncoderID, id);
      mpEncoder->Destroy();
      mpEncoder = NULL;
    }
  }
  if (!mpEncoder) {
    LOGM_INFO("[Video Encoder flow], CVideoEncoderV2::Initialize() start, before gfModuleFactoryVideoEncoder(%d)\n", id);
    mpEncoder = gfModuleFactoryVideoEncoder(id, mpPersistMediaConfig, mpEngineMsgSink);
    if (mpEncoder) {
      mCurEncoderID = id;
    } else {
      mCurEncoderID = EVideoEncoderModuleID_Auto;
      LOGM_FATAL("[Internal bug], request gfModuleFactoryVideoEncoder(%d) fail\n", id);
      return EECode_InternalLogicalBug;
    }
  }
  LOGM_INFO("[EncoderFilter flow], before mpEncoder->SetupContext()\n");
  SVideoEncoderInputParam param = {0};
  param.index = 0;
  param.format = StreamFormat_H264;
  param.prefer_dsp_mode = 0;
  param.stream_id = mStreamID;
  err = mpEncoder->SetupContext(&param);
  if (DUnlikely(EECode_OK != err)) {
    LOGM_ERROR("mpEncoder->SetupContext() failed.\n");
    return err;
  }
  err = mpEncoder->SetBufferPool(mpBufferPool);
  if (DUnlikely(EECode_OK != err)) {
    LOGM_ERROR("mpEncoder->SetBufferPool() failed.\n");
    return err;
  }
  mbEncoderContentSetup = 1;
  LOGM_INFO("[EncoderFilter flow], CVideoEncoderV2::Initialize() done\n");
  return EECode_OK;
}

EECode CVideoEncoderV2::Finalize()
{
  if (mpEncoder) {
    if (mbEncoderStarted) {
      mbEncoderStarted = 0;
      LOGM_INFO("[EncoderFilter flow], before mpEncoder->Stop()\n");
      mpEncoder->Stop();
    }
    LOGM_INFO("[EncoderFilter flow], before mpEncoder->DestroyContext()\n");
    mpEncoder->DestroyContext();
    mbEncoderContentSetup = 0;
    LOGM_INFO("[EncoderFilter flow], before mpEncoder->Delete(), cur id %d\n", mCurEncoderID);
    mpEncoder->Destroy();
    mpEncoder = NULL;
  }
  return EECode_OK;
}

EECode CVideoEncoderV2::Start()
{
  EECode err = EECode_OK;
  //debug assert
  DASSERT(mpEncoder);
  DASSERT(mpEngineMsgSink);
  DASSERT(mpOutputPins[0]);
  DASSERT(mbEncoderContentSetup);
  DASSERT(!mbEncoderStarted);
  if (mpEncoder) {
    LOGM_INFO("[EncoderFilter flow], CVideoEncoderV2::Start(), before mpEncoder->Start()\n");
    mpEncoder->Start();
    LOGM_INFO("[EncoderFilter flow], CVideoEncoderV2::Start(), after mpEncoder->Start()\n");
    mbEncoderStarted = 1;
  } else {
    LOGM_FATAL("NULL mpEncoder in CVideoEncoderV2::Start().\n");
  }
  LOGM_FLOW("before inherited::Start()...\n");
  err = inherited::Start();
  LOGM_FLOW("after inherited::Start(), err %d\n", err);
  return err;
}

EECode CVideoEncoderV2::Stop()
{
  //debug assert
  DASSERT(mpEncoder);
  DASSERT(mpEngineMsgSink);
  DASSERT(mpOutputPins[0]);
  DASSERT(mbEncoderContentSetup);
  DASSERT(mbEncoderStarted);
  if (mpEncoder) {
    LOGM_INFO("[EncoderFilter flow], CVideoEncoderV2::Stop(), before mpEncoder->Stop()\n");
    mpEncoder->Stop();
    LOGM_INFO("[EncoderFilter flow], CVideoEncoderV2::Stop(), after mpEncoder->Stop()\n");
    mbEncoderStarted = 0;
  } else {
    LOGM_FATAL("NULL mpEncoder in CVideoEncoderV2::Stop().\n");
  }
  inherited::Stop();
  return EECode_OK;
}

EECode CVideoEncoderV2::SwitchInput(TComponentIndex focus_input_index)
{
  LOGM_FATAL("TO DO\n");
  return EECode_NoImplement;
}

EECode CVideoEncoderV2::FlowControl(EBufferType flowcontrol_type)
{
  LOGM_FATAL("TO DO\n");
  return EECode_NoImplement;
}

EECode CVideoEncoderV2::AddInputPin(TUint &index, TUint type)
{
  if (StreamType_Video == type) {
    if (mnCurrentInputPinNumber >= EConstVideoEncoderMaxInputPinNumber) {
      LOGM_ERROR("Max input pin number reached, mnCurrentInputPinNumber %d.\n", mnCurrentInputPinNumber);
      return EECode_InternalLogicalBug;
    }
    index = mnCurrentInputPinNumber;
    DASSERT(!mpInputPins[mnCurrentInputPinNumber]);
    if (mpInputPins[mnCurrentInputPinNumber]) {
      LOGM_FATAL("mpInputPins[mnCurrentInputPinNumber] here, must have problems!!! please check it\n");
    }
    mpInputPins[mnCurrentInputPinNumber] = CQueueInputPin::Create("[Video input pin for CVideoEncoderV2]", (IFilter *) this, (StreamType) type, mpWorkQ->MsgQ());
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
  }
  return EECode_InternalLogicalBug;
}

EECode CVideoEncoderV2::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
  DASSERT(StreamType_Video == type);
  if (StreamType_Video == type) {
    if (!mpOutputPins[mnCurrentOutputPinNumber]) {
      mpOutputPins[mnCurrentOutputPinNumber] = COutputPin::Create("[video encoder output pin]", (IFilter *) this);
      if (!mpOutputPins[mnCurrentOutputPinNumber]) {
        LOGM_FATAL("Video encoder video output pin::Create() fail\n");
        return EECode_NoMemory;
      }
      if (!mpBufferPool) {
        mpBufferPool = CBufferPool::Create("[Buffer pool for video output pin in CVideoEncoderV2 filter]", 24);
        if (!mpBufferPool)  {
          LOGM_FATAL("CBufferPool::Create() fail?\n");
          return EECode_NoMemory;
        }
        mpBufferPool->AddBufferNotifyListener((IEventListener *) this);
      }
      mpOutputPins[mnCurrentOutputPinNumber]->SetBufferPool(mpBufferPool);
    }
    if (mpOutputPins[mnCurrentOutputPinNumber]) {
      EECode ret = mpOutputPins[mnCurrentOutputPinNumber]->AddSubPin(sub_index);
      DASSERT_OK(ret);
      if (EECode_OK == ret) {
        index = mnCurrentOutputPinNumber;
        mnCurrentOutputPinNumber ++;
      } else {
      }
    } else {
      LOGM_FATAL("Why the pin is NULL?\n");
      return EECode_InternalLogicalBug;
    }
  }  else {
    LOGM_ERROR("BAD type %d\n", type);
    return EECode_BadParam;
  }
  return EECode_OK;
}

IOutputPin *CVideoEncoderV2::GetOutputPin(TUint index, TUint sub_index)
{
  TUint max_sub_index = 0;
  if (DMaxStreamNumber > index) {
    if (mpOutputPins[index]) {
      max_sub_index = mpOutputPins[index]->NumberOfPins();
      if (max_sub_index > sub_index) {
        return mpOutputPins[index];
      } else {
        LOGM_ERROR("BAD sub_index %d, max value %d, in GetOutputPin(%u, %u)\n", sub_index, max_sub_index, index, sub_index);
      }
    } else {
      LOGM_FATAL("Why the pointer is NULL? in GetOutputPin(%u, %u)\n", index, sub_index);
    }
  } else {
    LOGM_ERROR("BAD index %d, max value %d, in GetOutputPin(%u, %u)\n", index, DMaxStreamNumber, index, sub_index);
  }
  return NULL;
}

IInputPin *CVideoEncoderV2::GetInputPin(TUint index)
{
  if (index < mnCurrentInputPinNumber) {
    return mpInputPins[index];
  }
  LOGM_FATAL("BAD index %d, exceed mnCurrentInputPinNumber %d\n", index, mnCurrentInputPinNumber);
  return NULL;
}

EECode CVideoEncoderV2::FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
  EECode ret = EECode_OK;
  switch (property_type) {
    case EFilterPropertyType_set_stream_id: {
        SConfigVideoStreamID *p_vsid = (SConfigVideoStreamID *) p_param;
        DASSERT(p_vsid);
        if (p_vsid) {
          DASSERT(EGenericEngineConfigure_VideoStreamID == p_vsid->check_field);
          mStreamID = p_vsid->stream_id;
        }
      }
      break;
    case EFilterPropertyType_refresh_sequence:
      if (mpEncoder) {
        mpEncoder->VideoEncoderProperty(EFilterPropertyType_refresh_sequence, set_or_get, p_param);
      }
      break;
    default:
      LOGM_FATAL("BAD property 0x%08x\n", property_type);
      ret = EECode_InternalLogicalBug;
      break;
  }
  return ret;
}

void CVideoEncoderV2::GetInfo(INFO &info)
{
  info.mPriority = 0;
  info.mFlags = 0;
  info.nInput = 0;
  info.nOutput = 1;
  info.pName = mpModuleName;
}

void CVideoEncoderV2::PrintStatus()
{
  LOGM_PRINTF("msState=%d, %s, heart beat %d\n", msState, gfGetModuleStateString(msState), mDebugHeartBeat);
  TU32 i = 0;
  for (i = 0; i < DMaxStreamNumber; i ++) {
    if (mpOutputPins[i]) {
      LOGM_PRINTF("mpOutputPin[%d]'s status:\n", i);
      mpOutputPins[i]->PrintStatus();
    }
  }
  mDebugHeartBeat = 0;
  return;
}

void CVideoEncoderV2::OnRun()
{
  SCMD cmd;
  EECode err = EECode_OK;
  TU32 current_remaining = 0, cached_frames = 0;
  TU32 output_index = 0;
  CIQueue::QType type;
  mpWorkQ->CmdAck(EECode_OK);
  LOGM_INFO("OnRun loop, start\n");
  mbRun = 1;
  msState = EModuleState_WaitCmd;
  DASSERT(mnCurrentOutputPinNumber);
  DASSERT(mpBufferPool);
  while (mbRun) {
    LOGM_STATE("OnRun: start switch, msState=%d, %s\n", msState, gfGetModuleStateString(msState));
    switch (msState) {
      case EModuleState_WaitCmd:
        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
        processCmd(cmd);
        break;
      case EModuleState_Idle:
        DASSERT(!mpInputBuffer);
        DASSERT(!mpBuffer);
        while (mpWorkQ->PeekCmd(cmd)) {
          processCmd(cmd);
          if (DUnlikely(mbPaused)) {
            msState = EModuleState_Pending;
            break;
          }
        }
        if (DUnlikely(!mbRun)) {
          break;
        }
        if (mpBufferPool->GetFreeBufferCnt()) {
          msState = EModuleState_HasOutputBuffer;
          break;
        }
        DASSERT(mpCurInputPin);
        type = mpWorkQ->WaitDataMsgWithSpecifiedQueue(&cmd, sizeof(cmd), mpCurInputPin->GetBufferQ());
        if (type == CIQueue::Q_MSG) {
          processCmd(cmd);
        } else {
          if (DLikely(mpCurInputPin->PeekBuffer(mpInputBuffer))) {
            if (DUnlikely(EBufferType_FlowControl_EOS == mpInputBuffer->GetBufferType())) {
              msState = EModuleState_Pending;
              mPersistEOSBuffer.SetBufferType(EBufferType_FlowControl_EOS);
              LOGM_INFO("bit-stream comes to end, sending EOS to down filter...\n");
              sendFlowControlBuffer(&mPersistEOSBuffer);
              mpInputBuffer->Release();
              mpInputBuffer = NULL;
            } else if (EBufferType_VideoFrameBuffer == mpInputBuffer->GetBufferType()) {
              msState = EModuleState_HasInputData;
            } else {
              LOGM_FATAL("Buffer type not as expected, %d\n", mpInputBuffer->GetBufferType());
              mpInputBuffer->Release();
              mpInputBuffer = NULL;
            }
          } else {
            LOGM_FATAL("mpCurInputPin->PeekBuffer fail\n");
            msState = EModuleState_Error;
          }
        }
        break;
      case EModuleState_HasInputData:
        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
        processCmd(cmd);
        break;
      case EModuleState_HasOutputBuffer:
        type = mpWorkQ->WaitDataMsgWithSpecifiedQueue(&cmd, sizeof(cmd), mpCurInputPin->GetBufferQ());
        if (type == CIQueue::Q_MSG) {
          processCmd(cmd);
        } else {
          if (DLikely(mpCurInputPin->PeekBuffer(mpInputBuffer))) {
            if (DUnlikely(EBufferType_FlowControl_EOS == mpInputBuffer->GetBufferType())) {
              msState = EModuleState_Pending;
              mPersistEOSBuffer.SetBufferType(EBufferType_FlowControl_EOS);
              LOGM_INFO("bit-stream comes to end, sending EOS to down filter...\n");
              sendFlowControlBuffer(&mPersistEOSBuffer);
              mpInputBuffer->Release();
              mpInputBuffer = NULL;
            } else if (EBufferType_VideoFrameBuffer == mpInputBuffer->GetBufferType()) {
              msState = EModuleState_Running;
            } else {
              LOGM_FATAL("Buffer type not as expected, %d\n", mpInputBuffer->GetBufferType());
              mpInputBuffer->Release();
              mpInputBuffer = NULL;
            }
          } else {
            LOGM_FATAL("mpCurInputPin->PeekBuffer fail\n");
            msState = EModuleState_Error;
          }
        }
        break;
      case EModuleState_Running:
        DASSERT(mpInputBuffer);
        if (!mpBuffer) {
          DASSERT(mpBufferPool->GetFreeBufferCnt());
          if (DUnlikely(!mpBufferPool->AllocBuffer(mpBuffer, sizeof(CIBuffer)))) {
            LOGM_FATAL("AllocBuffer() fail? must have internal bugs.\n");
            msState = EModuleState_Error;
            mpInputBuffer->Release();
            mpInputBuffer = NULL;
            break;
          }
        }
        if (DUnlikely(CIBuffer::DUP_FRAME & mpInputBuffer->GetBufferFlags())) {
          if (DUnlikely(!mpCachedInputBuffer)) {
            //LOGM_DEBUG("no cached buffer?\n");
            mpInputBuffer->Release();
            mpInputBuffer = NULL;
            msState = EModuleState_HasOutputBuffer;
            break;
          }
          mpCachedInputBuffer->SetBufferDTS(mpInputBuffer->GetBufferDTS());
          mpCachedInputBuffer->SetBufferPTS(mpInputBuffer->GetBufferPTS());
          mpCachedInputBuffer->SetBufferNativeDTS(mpInputBuffer->GetBufferNativeDTS());
          mpCachedInputBuffer->SetBufferNativePTS(mpInputBuffer->GetBufferNativePTS());
          mpCachedInputBuffer->SetBufferLinearDTS(mpInputBuffer->GetBufferLinearDTS());
          mpCachedInputBuffer->SetBufferLinearPTS(mpInputBuffer->GetBufferLinearPTS());
          err = mpEncoder->Encode(mpCachedInputBuffer, mpBuffer, current_remaining, cached_frames, output_index);
        } else {
          if (mpCachedInputBuffer) {
            mpCachedInputBuffer->Release();
          }
          mpCachedInputBuffer = mpInputBuffer;
          mpCachedInputBuffer->AddRef();
          err = mpEncoder->Encode(mpInputBuffer, mpBuffer, current_remaining, cached_frames, output_index);
        }
        if (DUnlikely(EECode_OK_NoOutputYet == err)) {
          msState = EModuleState_HasOutputBuffer;
          mpInputBuffer->Release();
          mpInputBuffer = NULL;
          break;
        } else if (EECode_OK != err) {
          mpInputBuffer->Release();
          mpInputBuffer = NULL;
          msState = EModuleState_HasOutputBuffer;
          break;
        }
        DASSERT(0 == output_index);
        if (mpOutputPins[0]) {
          mpOutputPins[0]->SendBuffer(mpBuffer);
        } else {
          mpBuffer->Release();
        }
        mpBuffer = NULL;
        mpInputBuffer->Release();
        mpInputBuffer = NULL;
        while (current_remaining) {
          mpBufferPool->AllocBuffer(mpBuffer, sizeof(CIBuffer));
          err = mpEncoder->Encode(NULL, mpBuffer, current_remaining, cached_frames, output_index);
          DASSERT(0 == output_index);
          if (mpOutputPins[0]) {
            mpOutputPins[0]->SendBuffer(mpBuffer);
          } else {
            LOGM_FATAL("NULL outputpin[0]\n");
            mpBuffer->Release();
          }
          mpBuffer = NULL;
        }
        msState = EModuleState_Idle;
        break;
      case EModuleState_DirectEncoding:
        while (mpWorkQ->PeekCmd(cmd)) {
          processCmd(cmd);
          if (DUnlikely(mbPaused)) {
            msState = EModuleState_Pending;
            break;
          }
        }
        if (!mpBuffer) {
          if (DUnlikely(!mpBufferPool->AllocBuffer(mpBuffer, sizeof(CIBuffer)))) {
            LOGM_FATAL("AllocBuffer() fail? must have internal bugs.\n");
            msState = EModuleState_Error;
            break;
          }
        }
        err = mpEncoder->Encode(NULL, mpBuffer, current_remaining, cached_frames, output_index);
        if (EECode_OK == err) {
          if (output_index < mnCurrentOutputPinNumber) {
            if (mpOutputPins[output_index]) {
              mpOutputPins[output_index]->SendBuffer(mpBuffer);
            } else {
              mpBuffer->Release();
            }
          } else {
            LOGM_NOTICE("output index %d exceed %d, skip\n", output_index, mnCurrentOutputPinNumber);
            mpBuffer->Release();
          }
          mpBuffer = NULL;
        } else if (EECode_OK_NoOutputYet != err) {
          LOGM_FATAL("encode fail, output_index %d\n", output_index);
          msState = EModuleState_Error;
          break;
        }
        break;
      case EModuleState_Error:
      case EModuleState_Completed:
      case EModuleState_Pending:
        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
        processCmd(cmd);
        break;
      default:
        LOGM_ERROR("CVideoEncoderV2:: OnRun: state invalid: 0x%x\n", (TUint)msState);
        msState = EModuleState_Error;
        break;
    }
    mDebugHeartBeat ++;
  }
}

void CVideoEncoderV2::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
  LOGM_FLOW("EventNotify, event type %d.\n", (TInt)type);
  switch (type) {
    case EEventType_BufferReleaseNotify:
      mpWorkQ->PostMsg(ECMDType_NotifyBufferRelease);
      break;
    default:
      LOGM_ERROR("event type unsupported:  %d", (TInt)type);
  }
}

void CVideoEncoderV2::processCmd(SCMD &cmd)
{
  EECode err = EECode_OK;
  LOGM_FLOW("processCmd, cmd.code %d, %s, state %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code), msState, gfGetModuleStateString(msState));
  switch (cmd.code) {
    case ECMDType_ExitRunning:
      flushData();
      mbRun = 0;
      msState = EModuleState_Halt;
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_Stop:
      flushData();
      msState = EModuleState_WaitCmd;
      mpWorkQ->CmdAck(EECode_OK);
      LOGM_INFO("processCmd, ECMDType_Stop.\n");
      break;
    case ECMDType_Flush:
      flushData();
      msState = EModuleState_Pending;
      //mbWaitKeyFrame = 1;
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_ResumeFlush:
      if (EModuleState_Pending == msState) {
      } else if (msState == EModuleState_Error) {
      } else {
        LOGM_FATAL("invalid msState=%d in ECMDType_ResumeFlush\n", msState);
      }
      if (mnCurrentInputPinNumber) {
        msState = EModuleState_Idle;
      } else {
        msState = EModuleState_DirectEncoding;
      }
      //mbWaitKeyFrame = 1;
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_Suspend: {
        msState = EModuleState_Pending;
        flushData();
        if (mpEncoder) {
          if ((EReleaseFlag_None != cmd.flag) && mbEncoderStarted) {
            mbEncoderStarted = 0;
            mpEncoder->Stop();
          }
          if ((EReleaseFlag_Destroy == cmd.flag) && mbEncoderContentSetup) {
            mpEncoder->DestroyContext();
            mbEncoderContentSetup = 0;
          }
        }
        mpWorkQ->CmdAck(EECode_OK);
      }
      break;
    case ECMDType_ResumeSuspend: {
        if (mnCurrentInputPinNumber) {
          msState = EModuleState_Idle;
        } else {
          msState = EModuleState_DirectEncoding;
        }
        if (mpEncoder) {
          if (!mbEncoderContentSetup) {
            SVideoEncoderInputParam param = {0};
            param.index = 0;//hard code here
            param.format = StreamFormat_H264;
            param.prefer_dsp_mode = 0;
            param.stream_id = mStreamID;
            err = mpEncoder->SetupContext(&param);
            if (DUnlikely(EECode_OK != err)) {
              LOGM_ERROR("mpEncoder->SetupContext() failed.\n");
            }
            mbEncoderContentSetup = 1;
          }
          if (!mbEncoderStarted) {
            mbEncoderStarted = 1;
            mpEncoder->Start();
          }
        }
        mpWorkQ->CmdAck(EECode_OK);
      }
      break;
    case ECMDType_NotifySynced:
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_Start:
      if (msState == EModuleState_WaitCmd) {
        if (mnCurrentInputPinNumber) {
          msState = EModuleState_Idle;
        } else {
          msState = EModuleState_DirectEncoding;
        }
      }
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_NotifySourceFilterBlocked:
      LOGM_INFO("processCmd, got ECMDType_NotifySourceFilterBlocked\n");
      break;
    case ECMDType_NotifyBufferRelease:
      if (mpBufferPool->GetFreeBufferCnt() > 0) {
        if (msState == EModuleState_Idle)
        { msState = EModuleState_HasOutputBuffer; }
        else if (msState == EModuleState_HasInputData)
        { msState = EModuleState_Running; }
      }
      break;
    default:
      LOGM_ERROR("processCmd, wrong cmd %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code));
      break;
  }
}

void CVideoEncoderV2::flushData()
{
  if (mpBuffer) {
    mpBuffer->Release();
    mpBuffer = NULL;
  }
  if (mpCachedInputBuffer) {
    mpCachedInputBuffer->Release();
    mpCachedInputBuffer = NULL;
  }
  if (mpInputBuffer) {
    mpInputBuffer->Release();
    mpInputBuffer = NULL;
  }
}

void CVideoEncoderV2::sendFlowControlBuffer(CIBuffer *buffer)
{
  TU32 i = 0;
  for (i = 0; i < DMaxStreamNumber; i ++) {
    if (mpOutputPins[i]) {
      mpOutputPins[i]->SendBuffer(buffer);
    }
  }
}

