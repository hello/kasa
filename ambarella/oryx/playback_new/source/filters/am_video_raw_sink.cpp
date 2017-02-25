/*******************************************************************************
 * am_video_raw_sink.cpp
 *
 * History:
 *    2016/04/21 - [Zhi He] create file
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

#include "am_video_raw_sink.h"

IFilter *gfCreateVideoRawSinkFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
  return CVideoRawSink::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

//-----------------------------------------------------------------------
//
// CVideoRawSink
//
//-----------------------------------------------------------------------
IFilter *CVideoRawSink::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
  CVideoRawSink *result = new CVideoRawSink(pName, pPersistMediaConfig, pEngineMsgSink, index);
  if (result && result->Construct() != EECode_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

CVideoRawSink::CVideoRawSink(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
  : inherited(pName, index, pPersistMediaConfig, pEngineMsgSink)
  , mpFileDumper(NULL)
  , mpCurrentInputPin(NULL)
  , mnInputPinsNumber(0)
  , mpBuffer(NULL)
  , mbRunning(0)
  , mbPaused(0)
  , mbEOS(0)
{
  TUint i = 0;
  for (i = 0; i < EConstVideoRendererMaxInputPinNumber; i ++) {
    mpInputPins[i] = 0;
  }
  mpScreenCaptureCacheBuffer = NULL;
  mScreenCaptureCacheBufferSize = 0;
  mScreenCapturePictureWidth = 0;
  mScreenCapturePictureHeight = 0;
  mScreenCaptureIndex = 0;
}

EECode CVideoRawSink::Construct()
{
  DSET_MODULE_LOG_CONFIG(ELogModuleVideoRendererFilter);
  return inherited::Construct();
}

CVideoRawSink::~CVideoRawSink()
{
}

void CVideoRawSink::Delete()
{
  TU32 i = 0;
  for (i = 0; i < EConstVideoRendererMaxInputPinNumber; i ++) {
    if (mpInputPins[i]) {
      mpInputPins[i]->Delete();
      mpInputPins[i] = NULL;
    }
  }
  if (mpScreenCaptureCacheBuffer) {
    DDBG_FREE(mpScreenCaptureCacheBuffer, "VOSC");
    mpScreenCaptureCacheBuffer = NULL;
  }
  inherited::Delete();
}

EECode CVideoRawSink::Initialize(TChar *p_param)
{
  return EECode_OK;
}

EECode CVideoRawSink::Finalize()
{
  return EECode_OK;
}

void CVideoRawSink::PrintState()
{
  LOGM_FATAL("TO DO\n");
}

EECode CVideoRawSink::Run()
{
  DASSERT(mpEngineMsgSink);
  DASSERT(mpPersistMediaConfig);
  DASSERT(mpInputPins[0]);
  inherited::Run();
  mbRunning = 1;
  return EECode_OK;
}

EECode CVideoRawSink::Start()
{
  DASSERT(mpEngineMsgSink);
  DASSERT(mpPersistMediaConfig);
  DASSERT(mpInputPins[0]);
  return inherited::Start();
}

EECode CVideoRawSink::Stop()
{
  DASSERT(mpEngineMsgSink);
  DASSERT(mpPersistMediaConfig);
  DASSERT(mpInputPins[0]);
  inherited::Stop();
  return EECode_OK;
}

void CVideoRawSink::Pause()
{
  DASSERT(mpEngineMsgSink);
  DASSERT(mpPersistMediaConfig);
  DASSERT(mpInputPins[0]);
  inherited::Pause();
  return;
}

void CVideoRawSink::Resume()
{
  DASSERT(mpEngineMsgSink);
  DASSERT(mpPersistMediaConfig);
  DASSERT(mpInputPins[0]);
  inherited::Resume();
  return;
}

void CVideoRawSink::Flush()
{
  DASSERT(mpEngineMsgSink);
  DASSERT(mpPersistMediaConfig);
  DASSERT(mpInputPins[0]);
  inherited::Flush();
  return;
}

void CVideoRawSink::ResumeFlush()
{
  LOGM_FATAL("TO DO\n");
  return;
}

EECode CVideoRawSink::Suspend(TUint release_context)
{
  LOGM_FATAL("TO DO\n");
  return EECode_NoImplement;
}

EECode CVideoRawSink::ResumeSuspend(TComponentIndex input_index)
{
  LOGM_FATAL("TO DO\n");
  return EECode_NoImplement;
}

EECode CVideoRawSink::SwitchInput(TComponentIndex focus_input_index)
{
  LOGM_FATAL("TO DO\n");
  return EECode_NoImplement;
}

EECode CVideoRawSink::FlowControl(EBufferType flowcontrol_type)
{
  LOGM_FATAL("TO DO\n");
  return EECode_NoImplement;
}

IInputPin *CVideoRawSink::GetInputPin(TUint index)
{
  if (EConstVideoRendererMaxInputPinNumber > index) {
    return mpInputPins[index];
  } else {
    LOGM_ERROR("BAD index %d, max value %d, in CVideoRawSink::GetInputPin()\n", index, EConstVideoRendererMaxInputPinNumber);
  }
  return NULL;
}

IOutputPin *CVideoRawSink::GetOutputPin(TUint index, TUint sub_index)
{
  LOGM_FATAL("CVideoRawSink do not have output pin\n");
  return NULL;
}

EECode CVideoRawSink::AddInputPin(TUint &index, TUint type)
{
  if (StreamType_Video == type) {
    if (mnInputPinsNumber >= EConstVideoRendererMaxInputPinNumber) {
      LOGM_ERROR("Max input pin number reached.\n");
      return EECode_InternalLogicalBug;
    }
    index = mnInputPinsNumber;
    if (mpInputPins[mnInputPinsNumber]) {
      LOGM_FATAL("mpInputPins[%d] not NULL\n", mnInputPinsNumber);
    }
    mpInputPins[mnInputPinsNumber] = CQueueInputPin::Create("[video input pin for CVideoRawSink]", (IFilter *) this, StreamType_Video, mpWorkQ->MsgQ());
    DASSERT(mpInputPins[mnInputPinsNumber]);
    if (!mnInputPinsNumber) {
      mpCurrentInputPin = mpInputPins[0];
    }
    mnInputPinsNumber ++;
    return EECode_OK;
  } else {
    LOGM_FATAL("BAD input pin type %d\n", type);
    return EECode_InternalLogicalBug;
  }
  return EECode_OK;
}

EECode CVideoRawSink::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
  LOGM_FATAL("CVideoRawSink can not add a output pin\n");
  return EECode_InternalLogicalBug;
}

void CVideoRawSink::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
  switch (type) {
    case EEventType_Timer:
      mpWorkQ->PostMsg(EMSGType_TimeNotify, NULL);
      break;
    default:
      LOG_FATAL("to do\n");
      break;
  }
}

EECode CVideoRawSink::FilterProperty(EFilterPropertyType property, TUint set_or_get, void *p_param)
{
  EECode err = EECode_OK;
  switch (property) {
    case EFilterPropertyType_vrenderer_trickplay_pause:
      LOG_NOTICE("EFilterPropertyType_vrenderer_trickplay_pause\n");
      mpWorkQ->PostMsg(ECMDType_Pause, NULL);
      break;
    case EFilterPropertyType_vrenderer_trickplay_resume:
      LOG_NOTICE("EFilterPropertyType_vrenderer_trickplay_resume\n");
      mpWorkQ->PostMsg(ECMDType_Resume, NULL);
      break;
    case EFilterPropertyType_update_sink_url: {
        if (mbRunning) {
          SCMD cmd;
          cmd.code = ECMDType_UpdateUrl;
          cmd.pExtra = (void *) p_param;
          cmd.needFreePExtra = 0;
          mpWorkQ->SendCmd(cmd);
        } else {
          if (mpFileDumper) {
            gfDestroyFileDumper(mpFileDumper);
            mpFileDumper = NULL;
          }
          if (p_param) {
            mpFileDumper = gfCreateFileDumper((TChar *)(p_param));
          }
        }
      }
      break;
    case EFilterPropertyType_vrenderer_screen_capture:
      mpWorkQ->PostMsg(ECMDType_ScreenCapture, NULL);
      break;
    default:
      LOGM_FATAL("BAD property 0x%08x\n", property);
      err = EECode_InternalLogicalBug;
      break;
  }
  return err;
}

void CVideoRawSink::GetInfo(INFO &info)
{
  info.mPriority = 0;
  info.mFlags = 0;
  info.nInput = 1;
  info.nOutput = 0;
  info.pName = mpModuleName;
}

void CVideoRawSink::PrintStatus()
{
  TUint i = 0;
  LOGM_PRINTF("\t[PrintStatus] CVideoRawSink[%d]: msState=%d, %s\n", mIndex, msState, gfGetModuleStateString(msState));
  for (i = 0; i < mnInputPinsNumber; i ++) {
    if (mpInputPins[i]) {
      mpInputPins[i]->PrintStatus();
    }
  }
  return;
}

EECode CVideoRawSink::flushInputData()
{
  if (mpBuffer) {
    mpBuffer->Release();
    mpBuffer = NULL;
  }
  LOGM_FLOW("before purge input pins\n");
  if (mpCurrentInputPin) {
    mpCurrentInputPin->Purge();
  }
  LOGM_FLOW("after purge input pins\n");
  return EECode_OK;
}

EECode CVideoRawSink::processCmd(SCMD &cmd)
{
  EECode err = EECode_OK;
  switch (cmd.code) {
    case ECMDType_ExitRunning:
      mbRun = 0;
      flushInputData();
      msState = EModuleState_Halt;
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_Start:
      if (EModuleState_WaitCmd == msState) {
        msState = EModuleState_Idle;
      }
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_Stop:
      flushInputData();
      msState = EModuleState_WaitCmd;
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_Flush:
      if (mpBuffer) {
        mpBuffer->Release();
        mpBuffer = NULL;
      }
      msState = EModuleState_Stopped;
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_Step:
      msState = EModuleState_Step;
      break;
    case ECMDType_Pause:
      if (mpBuffer) {
        mpBuffer->Release();
        mpBuffer = NULL;
      }
      mbPaused = 1;
      msState = EModuleState_Pending;
      break;
    case ECMDType_Resume:
      if (mpBuffer) {
        mpBuffer->Release();
        mpBuffer = NULL;
      }
      mbPaused = 0;
      msState = EModuleState_Idle;
      break;
    case ECMDType_UpdateUrl: {
        if (mpFileDumper) {
          gfDestroyFileDumper(mpFileDumper);
          mpFileDumper = NULL;
        }
        if (cmd.pExtra) {
          mpFileDumper = gfCreateFileDumper((TChar *)(cmd.pExtra));
        }
        mpWorkQ->CmdAck(EECode_OK);
      }
      break;
    case ECMDType_ScreenCapture:
      screenCapturePNG(mpBuffer);
      break;
    default:
      LOGM_ERROR("processCmd, wrong cmd 0x%x, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code));
      break;
  }
  return err;
}

void CVideoRawSink::dumpBuffer(CIBuffer *buffer)
{
  if (DLikely(mpFileDumper)) {
    TU32 i = 0;
    TU32 w = 0, h = 0;
    TU8 *p = NULL;
    TU32 linesize = 0;
    if (StreamFormat_PixelFormat_YUV420p == buffer->mContentFormat) {
      w = buffer->mVideoWidth;
      h = buffer->mVideoHeight;
      p = buffer->GetDataPtr(0);
      linesize = buffer->GetDataLineSize(0);
      for (i = 0; i < h; i ++) {
        gfFileDump(mpFileDumper, p, w);
        p += linesize;
      }
      w = buffer->mVideoWidth / 2;
      h = buffer->mVideoHeight / 2;
      p = buffer->GetDataPtr(1);
      linesize = buffer->GetDataLineSize(1);
      for (i = 0; i < h; i ++) {
        gfFileDump(mpFileDumper, p, w);
        p += linesize;
      }
      w = buffer->mVideoWidth / 2;
      h = buffer->mVideoHeight / 2;
      p = buffer->GetDataPtr(2);
      linesize = buffer->GetDataLineSize(2);
      for (i = 0; i < h; i ++) {
        gfFileDump(mpFileDumper, p, w);
        p += linesize;
      }
    } else if (StreamFormat_PixelFormat_YUV422p == buffer->mContentFormat) {
      w = buffer->mVideoWidth;
      h = buffer->mVideoHeight;
      p = buffer->GetDataPtr(0);
      linesize = buffer->GetDataLineSize(0);
      for (i = 0; i < h; i ++) {
        gfFileDump(mpFileDumper, p, w);
        p += linesize;
      }
      w = buffer->mVideoWidth / 2;
      h = buffer->mVideoHeight / 2;
      p = buffer->GetDataPtr(1);
      linesize = buffer->GetDataLineSize(1) * 2;
      for (i = 0; i < h; i ++) {
        gfFileDump(mpFileDumper, p, w);
        p += linesize;
      }
      w = buffer->mVideoWidth / 2;
      h = buffer->mVideoHeight / 2;
      p = buffer->GetDataPtr(2);
      linesize = buffer->GetDataLineSize(2) * 2;
      for (i = 0; i < h; i ++) {
        gfFileDump(mpFileDumper, p, w);
        p += linesize;
      }
    } else {
      LOGM_FATAL("bad format %x\n", buffer->mContentFormat);
    }
  } else {
    LOG_ERROR("NULL mpFileDumper\n");
  }
}

EECode CVideoRawSink::screenCapturePNG(CIBuffer *buffer)
{
  if (buffer) {
    if (DUnlikely((!mpScreenCaptureCacheBuffer) || (buffer->mVideoWidth != mScreenCapturePictureWidth) || (buffer->mVideoHeight != mScreenCapturePictureHeight))) {
      mScreenCapturePictureWidth = buffer->mVideoWidth;
      mScreenCapturePictureHeight = buffer->mVideoHeight;
      if (mpScreenCaptureCacheBuffer && (mScreenCaptureCacheBufferSize >= (mScreenCapturePictureWidth * mScreenCapturePictureHeight * 4))) {
      } else {
        mScreenCaptureCacheBufferSize = mScreenCapturePictureWidth * mScreenCapturePictureHeight * 4;
        if (mpScreenCaptureCacheBuffer) {
          DDBG_FREE(mpScreenCaptureCacheBuffer, "VOSC");
          mpScreenCaptureCacheBuffer = NULL;
        }
        mpScreenCaptureCacheBuffer = (TU8 *) DDBG_MALLOC(mScreenCaptureCacheBufferSize, "VOSC");
        if (DUnlikely(!mpScreenCaptureCacheBuffer)) {
          LOGM_FATAL("no memory, request size %d\n", mScreenCaptureCacheBufferSize);
          return EECode_NoMemory;
        }
      }
    }
    TChar filename[128] = {0};
    SDateTime datetime;
    gfGetCurrentDateTime(&datetime);
    snprintf(filename, sizeof(filename), "screen-capture-%04d_%02d_%02d-%02d_%02d_%02d-%08d.png",
             datetime.year, datetime.month, datetime.day, datetime.hour, datetime.minute, datetime.seconds, mScreenCaptureIndex);
    //gfSavePNGFileFromYUV(mpScreenCaptureCacheBuffer,
    //    mScreenCapturePictureWidth, mScreenCapturePictureHeight, (mScreenCapturePictureWidth << 2),
    //    buffer->GetDataPtr(0), buffer->GetDataPtr(1), buffer->GetDataPtr(2),
    //    buffer->GetDataLineSize(0), buffer->GetDataLineSize(1), buffer->GetDataLineSize(2),
    //    filename);
    mScreenCaptureIndex ++;
  } else {
    return EECode_BadState;
  }
  return EECode_OK;
}

void CVideoRawSink::OnRun()
{
  SCMD cmd;
  CIQueue::QType type;
  CIQueue::WaitResult result;
  //CIQueue *data_queue = NULL;
  //TUint frame_count = 0;
  mbRun = true;
  msState = EModuleState_WaitCmd;
  mpWorkQ->CmdAck(EECode_OK);
  //LOGM_INFO("OnRun loop, start\n");
  while (mbRun) {
    LOGM_STATE("start switch, msState=%d, %s\n", msState, gfGetModuleStateString(msState));
    switch (msState) {
      case EModuleState_WaitCmd:
        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
        processCmd(cmd);
        break;
      case EModuleState_Halt:
        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
        processCmd(cmd);
        break;
      case EModuleState_Idle:
        if (mbPaused) {
          msState = EModuleState_Pending;
          break;
        }
        type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);
        if (type == CIQueue::Q_MSG) {
          processCmd(cmd);
        } else {
          mpCurrentInputPin = (CQueueInputPin *)result.pOwner;
          DASSERT(!mpBuffer);
          if (mpCurrentInputPin->PeekBuffer(mpBuffer)) {
            msState = EModuleState_Running;
          } else {
            LOGM_FATAL("mpCurrentInputPin->PeekBuffer(mpBuffer) fail?\n");
            msState = EModuleState_Error;
          }
        }
        break;
      case EModuleState_Completed:
        type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);
        if (type == CIQueue::Q_MSG) {
          processCmd(cmd);
        } else {
          msState = EModuleState_Idle;
        }
        break;
      case EModuleState_Stopped:
      case EModuleState_Pending:
        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
        processCmd(cmd);
        break;
      case EModuleState_Error:
        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
        processCmd(cmd);
        break;
      case EModuleState_Running:
        if (EBufferType_FlowControl_EOS == mpBuffer->GetBufferType()) {
          mpBuffer->Release();
          mpBuffer = NULL;
          msState = EModuleState_Completed;
          break;
        } else {
          dumpBuffer(mpBuffer);
        }
        if (DLikely(mpBuffer)) {
          mpBuffer->Release();
          mpBuffer = NULL;
        }
        msState = EModuleState_Idle;
        break;
      default:
        LOGM_ERROR("CVideoRawSink: OnRun: state invalid: 0x%x\n", (TUint)msState);
        msState = EModuleState_Error;
        break;
    }
  }
}


