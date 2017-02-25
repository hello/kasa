/**
 * am_video_renderer.cpp
 *
 * History:
 *    2013/04/05 - [Zhi He] create file
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

#include "am_video_renderer.h"

IFilter *gfCreateVideoRendererFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
  return CVideoRenderer::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

static EVideoRendererModuleID _guess_video_renderer_type_from_string(TChar *string)
{
  if (!string) {
    LOG_WARN("NULL input in _guess_video_renderer_type_from_string, choose default\n");
    return EVideoRendererModuleID_AMBADSP;
  }
  if (!strncmp(string, DNameTag_AMBA, strlen(DNameTag_AMBA))) {
    return EVideoRendererModuleID_AMBADSP;
  } else if (!strncmp(string, DNameTag_LinuxFB, strlen(DNameTag_LinuxFB))) {
    return EVideoRendererModuleID_LinuxFrameBuffer;
  }
  LOG_WARN("unknown string tag(%s) in _guess_video_renderer_type_from_string, choose default\n", string);
  return EVideoRendererModuleID_AMBADSP;
}

CVideoRendererInput *CVideoRendererInput::Create(const TChar *name, IFilter *pFilter, CIQueue *queue, TUint index)
{
  CVideoRendererInput *result = new CVideoRendererInput(name, pFilter, index);
  if (result && result->Construct(queue) != EECode_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

EECode CVideoRendererInput::Construct(CIQueue *queue)
{
  LOGM_DEBUG("CVideoRendererInput::Construct.\n");
  EECode err = inherited::Construct(queue);
  return err;
}

CVideoRendererInput::~CVideoRendererInput()
{
  LOGM_RESOURCE("CVideoRendererInput::~CVideoRendererInput.\n");
}

//-----------------------------------------------------------------------
//
// CVideoRenderer
//
//-----------------------------------------------------------------------
IFilter *CVideoRenderer::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
  CVideoRenderer *result = new CVideoRenderer(pName, pPersistMediaConfig, pEngineMsgSink, index);
  if (result && result->Construct() != EECode_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

CVideoRenderer::CVideoRenderer(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
  : inherited(pName, index, pPersistMediaConfig, pEngineMsgSink)
  , mpVideoRenderer(NULL)
  , mpCurrentInputPin(NULL)
  , mnInputPinsNumber(0)
  , mCurVideoRendererID(EVideoRendererModuleID_Invalid)
  , mpBuffer(NULL)
  , mbVideoRendererSetup(0)
  , mbBeginRendering(0)
  , mbPaused(0)
  , mbEOS(0)
{
  TUint i = 0;
  for (i = 0; i < EConstVideoRendererMaxInputPinNumber; i ++) {
    mpInputPins[i] = 0;
  }
  mCurVideoWidth = 0;
  mCurVideoHeight = 0;
  mVideoFramerateNum = 0;
  mVideoFramerateDen = 0;
  mVideoFramerate = 0;
  mLastTimestamp = 0;
  mSyncBeginTime = 0;
  mSyncBeginPTS = 0;
  mSyncCurrentTime = 0;
  mSyncCurrentPTS = 0;
  mWaitThreshold = 900;
  mpTimer = NULL;
}

EECode CVideoRenderer::Construct()
{
  DSET_MODULE_LOG_CONFIG(ELogModuleVideoRendererFilter);
  //mConfigLogLevel = ELogLevel_Debug;
  //mConfigLogOption = DCAL_BITMASK(ELogOption_State) | DCAL_BITMASK(ELogOption_Resource) | DCAL_BITMASK(ELogOption_Flow);
  return inherited::Construct();
}

CVideoRenderer::~CVideoRenderer()
{
}

void CVideoRenderer::Delete()
{
  TUint i = 0;
  LOGM_RESOURCE("CVideoRenderer::~CVideoRenderer(), before delete inputpins.\n");
  for (i = 0; i < EConstVideoRendererMaxInputPinNumber; i ++) {
    if (mpInputPins[i]) {
      mpInputPins[i]->Delete();
      mpInputPins[i] = NULL;
    }
  }
  LOGM_RESOURCE("CVideoRenderer::~CVideoRenderer(), before mpVideoRenderer->Delete().\n");
  if (mpVideoRenderer) {
    mpVideoRenderer->Destroy();
    mpVideoRenderer = NULL;
  }
  LOGM_RESOURCE("CVideoRenderer::~CVideoRenderer(), end.\n");
  inherited::Delete();
}

EECode CVideoRenderer::Initialize(TChar *p_param)
{
  EVideoRendererModuleID id;
  id = _guess_video_renderer_type_from_string(p_param);
  LOGM_INFO("EVideoRendererModuleID %d\n", id);
  if (mbVideoRendererSetup) {
    DASSERT(mpVideoRenderer);
    if (!mpVideoRenderer) {
      LOGM_FATAL("[Internal bug], why the mpVideoRenderer is NULL here?\n");
      return EECode_InternalLogicalBug;
    }
    LOGM_INFO("before mpVideoRenderer->DestroyContext()\n");
    mpVideoRenderer->DestroyContext();
    mbVideoRendererSetup = 0;
    if (id != mCurVideoRendererID) {
      LOGM_INFO("before mpVideoRenderer->Delete(), cur id %d, request id %d\n", mCurVideoRendererID, id);
      mpVideoRenderer->Destroy();
      mpVideoRenderer = NULL;
    }
  }
  if (!mpVideoRenderer) {
    LOGM_INFO("before gfModuleFactoryVideoRenderer(%d)\n", id);
    mpVideoRenderer = gfModuleFactoryVideoRenderer(id, mpPersistMediaConfig, mpEngineMsgSink);
    if (mpVideoRenderer) {
      mCurVideoRendererID = id;
      if (EVideoRendererModuleID_AMBADSP == mCurVideoRendererID) {
        EECode err = mpVideoRenderer->SetupContext(NULL);
        if (EECode_OK == err) {
          mbVideoRendererSetup = 1;
        } else {
          LOGM_ERROR("SetupContext fail, ret 0x%08x, %s\n", err, gfGetErrorCodeString(err));
          return err;
        }
      }
    } else {
      mCurVideoRendererID = EVideoRendererModuleID_Auto;
      LOGM_FATAL("[Internal bug], request gfModuleFactoryVideoRenderer(%d) fail\n", id);
      return EECode_InternalLogicalBug;
    }
  }
  LOGM_INFO("[VideoRenderer flow], CVideoRenderer::Initialize() done\n");
  return EECode_OK;
}

EECode CVideoRenderer::Finalize()
{
  if (mpVideoRenderer) {
    LOGM_INFO("before mpVideoRenderer->DestroyContext()\n");
    if (mbVideoRendererSetup) {
      mpVideoRenderer->DestroyContext();
      mbVideoRendererSetup = 0;
    }
    LOGM_INFO("before mpVideoRenderer->Delete(), cur id %d\n", mCurVideoRendererID);
    mpVideoRenderer->Destroy();
    mpVideoRenderer = NULL;
  }
  return EECode_OK;
}

void CVideoRenderer::PrintState()
{
  LOGM_FATAL("TO DO\n");
}

EECode CVideoRenderer::Run()
{
  //debug assert
  DASSERT(mpVideoRenderer);
  DASSERT(mpEngineMsgSink);
  DASSERT(mpPersistMediaConfig);
  DASSERT(mpInputPins[0]);
  //DASSERT(mbVideoRendererSetup);
  inherited::Run();
  return EECode_OK;
}

EECode CVideoRenderer::Start()
{
  //debug assert
  DASSERT(mpVideoRenderer);
  DASSERT(mpEngineMsgSink);
  DASSERT(mpPersistMediaConfig);
  DASSERT(mpInputPins[0]);
  //DASSERT(mbVideoRendererSetup);
  inherited::Start();
  return EECode_OK;
}

EECode CVideoRenderer::Stop()
{
  //debug assert
  DASSERT(mpVideoRenderer);
  DASSERT(mpEngineMsgSink);
  DASSERT(mpPersistMediaConfig);
  DASSERT(mpInputPins[0]);
  //DASSERT(mbVideoRendererSetup);
  inherited::Stop();
  return EECode_OK;
}

void CVideoRenderer::Pause()
{
  //debug assert
  DASSERT(mpVideoRenderer);
  DASSERT(mpEngineMsgSink);
  DASSERT(mpPersistMediaConfig);
  DASSERT(mpInputPins[0]);
  DASSERT(mbVideoRendererSetup);
  inherited::Pause();
  return;
}

void CVideoRenderer::Resume()
{
  //debug assert
  DASSERT(mpVideoRenderer);
  DASSERT(mpEngineMsgSink);
  DASSERT(mpPersistMediaConfig);
  DASSERT(mpInputPins[0]);
  DASSERT(mbVideoRendererSetup);
  inherited::Resume();
  return;
}

void CVideoRenderer::Flush()
{
  //debug assert
  DASSERT(mpVideoRenderer);
  DASSERT(mpEngineMsgSink);
  DASSERT(mpPersistMediaConfig);
  DASSERT(mpInputPins[0]);
  DASSERT(mbVideoRendererSetup);
  inherited::Flush();
  return;
}

void CVideoRenderer::ResumeFlush()
{
  inherited::ResumeFlush();
  return;
}

EECode CVideoRenderer::Suspend(TUint release_context)
{
  LOGM_FATAL("TO DO\n");
  return EECode_NoImplement;
}

EECode CVideoRenderer::ResumeSuspend(TComponentIndex input_index)
{
  LOGM_FATAL("TO DO\n");
  return EECode_NoImplement;
}

EECode CVideoRenderer::SwitchInput(TComponentIndex focus_input_index)
{
  LOGM_FATAL("TO DO\n");
  return EECode_NoImplement;
}

EECode CVideoRenderer::FlowControl(EBufferType flowcontrol_type)
{
  LOGM_FATAL("TO DO\n");
  return EECode_NoImplement;
}

EECode CVideoRenderer::AddInputPin(TUint &index, TUint type)
{
  if (StreamType_Video == type) {
    if (mnInputPinsNumber >= EConstVideoRendererMaxInputPinNumber) {
      LOGM_ERROR("Max input pin number reached.\n");
      return EECode_InternalLogicalBug;
    }
    index = mnInputPinsNumber;
    DASSERT(!mpInputPins[mnInputPinsNumber]);
    if (mpInputPins[mnInputPinsNumber]) {
      LOGM_FATAL("mpInputPins[mnInputPinsNumber] here, must have problems!!! please check it\n");
    }
    mpInputPins[mnInputPinsNumber] = CVideoRendererInput::Create("[Audio input pin for CVideoRenderer]", (IFilter *) this, mpWorkQ->MsgQ(), index);
    DASSERT(mpInputPins[mnInputPinsNumber]);
    mnInputPinsNumber ++;
    return EECode_OK;
  } else {
    LOGM_FATAL("BAD input pin type %d\n", type);
    return EECode_InternalLogicalBug;
  }
  return EECode_OK;
}

IInputPin *CVideoRenderer::GetInputPin(TUint index)
{
  if (EConstVideoRendererMaxInputPinNumber > index) {
    return mpInputPins[index];
  } else {
    LOGM_ERROR("BAD index %d, max value %d, in CVideoRenderer::GetInputPin()\n", index, EConstVideoRendererMaxInputPinNumber);
  }
  return NULL;
}

void CVideoRenderer::EventNotify(EEventType type, TU64 param1, TPointer param2)
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

EECode CVideoRenderer::FilterProperty(EFilterPropertyType property, TUint set_or_get, void *p_param)
{
  EECode err = EECode_OK;
  switch (property) {
    case EFilterPropertyType_vrenderer_trickplay_step:
      if (DLikely(mpVideoRenderer)) {
        mpVideoRenderer->StepPlay();
      } else {
        LOGM_ERROR("NULL mpVideoRenderer\n");
      }
      break;
    case EFilterPropertyType_vrenderer_trickplay_pause:
      if (DLikely(mpVideoRenderer)) {
        mpVideoRenderer->Pause();
      } else {
        LOGM_ERROR("NULL mpVideoRenderer\n");
      }
      break;
    case EFilterPropertyType_vrenderer_trickplay_resume:
      if (DLikely(mpVideoRenderer)) {
        mpVideoRenderer->Resume();
      } else {
        LOGM_ERROR("NULL mpVideoRenderer\n");
      }
      break;
    case EFilterPropertyType_vrenderer_sync_to_audio:
      LOGM_FATAL("TO DO\n");
      break;
    case EFilterPropertyType_vrenderer_align_all_video_streams:
      LOGM_FATAL("TO DO\n");
      break;
    case EFilterPropertyType_vrenderer_get_last_shown_timestamp: {
        SQueryLastShownTimeStamp *ptime = (SQueryLastShownTimeStamp *) p_param;
        if (ptime) {
          err = mpVideoRenderer->QueryLastShownTimeStamp(ptime->time);
        }
      }
      break;
    default:
      LOGM_FATAL("BAD property 0x%08x\n", property);
      err = EECode_InternalLogicalBug;
      break;
  }
  return err;
}

void CVideoRenderer::GetInfo(INFO &info)
{
  info.mPriority = 0;
  info.mFlags = 0;
  info.nInput = 1;
  info.nOutput = 0;
  info.pName = mpModuleName;
}

void CVideoRenderer::PrintStatus()
{
  TUint i = 0;
  LOGM_ALWAYS("msState=%d, %s, mnCurrentInputPinNumber\n", msState, gfGetModuleStateString(msState));
  for (i = 0; i < mnInputPinsNumber; i ++) {
    LOGM_ALWAYS("       inputpin[%p, %d]'s status:\n", mpInputPins[i], i);
    if (mpInputPins[i]) {
      mpInputPins[i]->PrintStatus();
    }
  }
  return;
}

void CVideoRenderer::postMsg(TUint msg_code)
{
  DASSERT(mpEngineMsgSink);
  if (mpEngineMsgSink) {
    SMSG msg;
    msg.code = msg_code;
    msg.owner_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_VideoRenderer, mIndex);
    msg.p_owner = (TULong) NULL;
    mpEngineMsgSink->MsgProc(msg);
  }
}

bool CVideoRenderer::readInputData(CVideoRendererInput *inputpin, EBufferType &type)
{
  DASSERT(!mpBuffer);
  DASSERT(inputpin);
  if (!inputpin->PeekBuffer(mpBuffer)) {
    LOGM_FATAL("No buffer?\n");
    return false;
  }
  type = mpBuffer->GetBufferType();
  if (EBufferType_FlowControl_EOS == type) {
    LOGM_FLOW("CVideoRenderer %p get EOS.\n", this);
    DASSERT(!mbEOS);
    mbEOS = 1;
    return true;
  } else {
    LOGM_ERROR("mpBuffer->GetBufferType() %d, mpBuffer->mFlags() %d.\n", mpBuffer->GetBufferType(), mpBuffer->mFlags);
    return true;
  }
  return true;
}

EECode CVideoRenderer::flushInputData(TU8 disable_pin)
{
  if (mpBuffer) {
    mpBuffer->Release();
    mpBuffer = NULL;
  }
  LOGM_FLOW("before purge input pins\n");
  if (mpCurrentInputPin) {
    mpCurrentInputPin->Purge(disable_pin);
  }
  LOGM_FLOW("after purge input pins\n");
  return EECode_OK;
}

EECode CVideoRenderer::processCmd(SCMD &cmd)
{
  EECode err = EECode_OK;
  switch (cmd.code) {
    case ECMDType_ExitRunning:
      mbRun = 0;
      flushInputData(1);
      msState = EModuleState_Halt;
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_Start:
      if (!mpVideoRenderer->IsMonitorMode()) {
        msState = EModuleState_Idle;
      } else {
        msState = EModuleState_Renderer_WaitDecoderReady;
      }
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_Stop:
      flushInputData(1);
      msState = EModuleState_WaitCmd;
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_Pause:
      mbPaused = 1;
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
      if (mpBuffer) {
        mpBuffer->Release();
        mpBuffer = NULL;
      }
      if (mpCurrentInputPin) {
        mpCurrentInputPin->Purge(0);
      }
      msState = EModuleState_Stopped;
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_ResumeFlush:
      msState = EModuleState_Idle;
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_BeginPlayback:
      if (EModuleState_Renderer_WaitPlaybackBegin == msState) {
        if (mpVideoRenderer) {
          err = mpVideoRenderer->Wakeup();
          DASSERT(EECode_OK == err);
        }
        msState = EModuleState_Renderer_WaitVoutEOSFlag;
      } else {
        LOGM_WARN("not expected state 0x%08x\n", msState);
      }
      break;
    case EMSGType_TimeNotify:
      if (mpBuffer) {
        mpVideoRenderer->Render(mpBuffer);
        mpBuffer->Release();
        mpBuffer = NULL;
        msState = EModuleState_Idle;
      }
      mpTimer = NULL;
      break;
    case EMSGType_NotifyDecoderReady:
      break;
    default:
      LOGM_ERROR("processCmd, wrong cmd 0x%08x, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code));
      break;
  }
  return err;
}

void CVideoRenderer::OnRun()
{
  SCMD cmd;
  CIQueue::QType type;
  CIQueue::WaitResult result;
  EECode err = EECode_OK;
  msState = EModuleState_WaitCmd;
  mbRun = 1;
  mpWorkQ->CmdAck(EECode_OK);
  LOGM_INFO("OnRun loop, start\n");
  mbRun = true;
  while (mbRun) {
    LOGM_STATE("OnRun: start switch, msState=%d, %s\n", msState, gfGetModuleStateString(msState));
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
          mpCurrentInputPin = (CVideoRendererInput *)result.pOwner;
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
        DASSERT(mpBuffer && mpVideoRenderer);
        if (EBufferType_FlowControl_EOS == mpBuffer->GetBufferType()) {
          DASSERT(mpEngineMsgSink);
          if (mpEngineMsgSink) {
            SMSG msg;
            msg.code = EMSGType_PlaybackEOS;
            msg.p_owner = (TULong)((IFilter *)this);
            msg.p3 = (mpBuffer->mNativeDTS >> 32) & 0xffffffff;
            msg.p4 = (mpBuffer->mNativeDTS) & 0xffffffff;
            msg.p_owner = 0;
            msg.owner_id = (TGenericID) DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_VideoRenderer, mIndex);
            mpEngineMsgSink->MsgProc(msg);
          }
          mpBuffer->Release();
          mpBuffer = NULL;
          msState = EModuleState_Completed;
          break;
        }
        if (DUnlikely(mpBuffer->GetBufferFlags() & CIBuffer::SYNC_POINT)) {
          mCurVideoWidth = mpBuffer->mVideoWidth;
          mCurVideoHeight = mpBuffer->mVideoHeight;
          mVideoFramerateNum = mpBuffer->mVideoFrameRateNum;
          mVideoFramerateDen = mpBuffer->mVideoFrameRateDen;
          mVideoFramerate = mpBuffer->mVideoRoughFrameRate;
          LOGM_CONFIGURATION("frame rate num %d den %d\n", mVideoFramerateNum, mVideoFramerateDen);
        }
        if (DUnlikely(!mbVideoRendererSetup)) {
          SVideoParams param;
          EECode err;
          memset(&param, 0x0, sizeof(param));
          param.pic_width = mCurVideoWidth;
          param.pic_height = mCurVideoHeight;
          param.pic_offset_x = 0;
          param.pic_offset_y = 0;
          param.framerate_num = mVideoFramerateNum;
          param.framerate_den = mVideoFramerateDen;
          LOGM_CONFIGURATION("video resolution %dx%d\n", mCurVideoWidth, mCurVideoHeight);
          err = mpVideoRenderer->SetupContext(&param);
          if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("SetupContext(), %d, %s fail\n", err, gfGetErrorCodeString(err));
            flushInputData(1);
            msState = EModuleState_Error;
            break;
          }
          mbVideoRendererSetup = 1;
        }
        if (!mbBeginRendering) {
          mbBeginRendering = 1;
          mSyncBeginTime = mpClockReference->GetCurrentTime();
          mSyncBeginPTS = mpBuffer->GetBufferPTS();
          mSyncCurrentTime = mSyncBeginTime;
          mSyncCurrentPTS = mSyncBeginPTS;
          LOGM_CONFIGURATION("begin time %" DPrint64d ", begin pts %" DPrint64d "\n", mSyncBeginTime, mSyncBeginPTS);
        } else {
          mSyncCurrentTime = mpClockReference->GetCurrentTime();
          mSyncCurrentPTS = mpBuffer->GetBufferPTS();
        }
        if ((mSyncCurrentTime + mWaitThreshold) > mSyncCurrentPTS) {
          mpVideoRenderer->Render(mpBuffer);
          mpBuffer->Release();
          mpBuffer = NULL;
          msState = EModuleState_Idle;
        } else {
          DASSERT(!mpTimer);
          mpTimer = mpClockReference->AddTimer(this, EClockTimerType_once, mSyncCurrentPTS - mSyncBeginPTS + mSyncBeginTime, 0, 1);
          msState = EModuleState_WaitTiming;
        }
        break;
      case EModuleState_Renderer_WaitDecoderReady:
        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
        if (EMSGType_NotifyDecoderReady == cmd.code) {
          msState = EModuleState_Renderer_WaitVoutDormant;
        } else {
          processCmd(cmd);
        }
        break;
      case EModuleState_Renderer_WaitVoutDormant:
        if (mpWorkQ->PeekCmd(cmd)) {
          processCmd(cmd);
        } else {
          err = mpVideoRenderer->WaitReady();
          if (EECode_OK == err) {
            postMsg(EMSGType_NotifyRendererReady);
            msState = EModuleState_Renderer_WaitPlaybackBegin;
          }
        }
        break;
      case EModuleState_Renderer_WaitPlaybackBegin:
        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
        processCmd(cmd);
        break;
      case EModuleState_Renderer_WaitVoutEOSFlag:
        if (mpWorkQ->PeekCmd(cmd)) {
          processCmd(cmd);
        } else {
          err = mpVideoRenderer->WaitEosMsg(mLastTimestamp);
          if (EECode_OK == err) {
            msState = EModuleState_Renderer_WaitVoutLastFrameDisplayed;
          } else if (EECode_OK_AlreadyStopped == err) {
            msState = EModuleState_Pending;
          } else if (EECode_TryAgain != err) {
            LOGM_ERROR("WaitEosMsg error\n");
            flushInputData(1);
            msState = EModuleState_Error;
          }
        }
        break;
      case EModuleState_Renderer_WaitVoutLastFrameDisplayed:
        if (mpWorkQ->PeekCmd(cmd)) {
          processCmd(cmd);
        } else {
          err = mpVideoRenderer->WaitLastShownTimeStamp(mLastTimestamp);
          if (EECode_OK == err) {
            postMsg(EMSGType_PlaybackEOS);
            msState = EModuleState_Completed;
          } else if (EECode_OK_AlreadyStopped == err) {
            msState = EModuleState_Pending;
          } else if (EECode_TryAgain != err) {
            LOGM_ERROR("WaitEosMsg error\n");
            flushInputData(1);
            msState = EModuleState_Error;
          }
        }
        break;
      case EModuleState_WaitTiming:
        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
        processCmd(cmd);
        break;
      default:
        LOGM_ERROR("CVideoRenderer: OnRun: state invalid: 0x%x\n", (TUint)msState);
        flushInputData(1);
        msState = EModuleState_Error;
        break;
    }
  }
}

