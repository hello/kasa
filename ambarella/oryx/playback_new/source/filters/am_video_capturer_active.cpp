/**
 * am_video_capturer_active.cpp
 *
 * History:
 *    2014/11/03 - [Zhi He] create file
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
 */

#include "am_native.h"
#include "am_native_log.h"

#include "am_internal.h"

#include "am_osal.h"
#include "am_framework.h"
#include "am_modules_interface.h"

#include "am_video_capturer_active.h"

IFilter *gfCreateVideoCapturerActiveFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
  return CVideoCapturerActive::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

static EVideoInputModuleID _guess_video_capturer_type_from_string(TChar *string)
{
  if (!string) {
    LOG_NOTICE("NULL input in _guess_video_capturer_type_from_string, choose default\n");
    return EVideoInputModuleID_LinuxUVC;
  }
  if (!strncmp(string, DNameTag_LinuxUVC, strlen(DNameTag_LinuxUVC))) {
    return EVideoInputModuleID_LinuxUVC;
  } else if (!strncmp(string, DNameTag_AMBA, strlen(DNameTag_AMBA))) {
    return EVideoInputModuleID_AMBADSP;
  }
  LOG_WARN("unknown string tag(%s) in _guess_video_capturer_type_from_string, choose default\n", string);
  return EVideoInputModuleID_LinuxUVC;
}

//-----------------------------------------------------------------------
//
// CVideoCapturerActive
//
//-----------------------------------------------------------------------

IFilter *CVideoCapturerActive::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
{
  CVideoCapturerActive *result = new CVideoCapturerActive(pName, pPersistMediaConfig, pEngineMsgSink, index);
  if ((result) && (EECode_OK != result->Construct())) {
    result->Delete();
    result = NULL;
  }
  return result;
}

CVideoCapturerActive::CVideoCapturerActive(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
  : inherited(pName, index, pPersistMediaConfig, pEngineMsgSink)
  , mpCapturer(NULL)
  , mbContentSetup(0)
  , mbCaptureFirstFrame(0)
  , mbRunning(0)
  , mbStarted(0)
  , mnCurrentOutputPinNumber(0)
  , mpOutputPin(NULL)
  , mpBufferPool(NULL)
  , mpBuffer(NULL)
  , mCapFramerateNum(DDefaultVideoFramerateNum)
  , mCapFramerateDen(DDefaultVideoFramerateDen)
  , mCaptureDuration(33333)
  , mSkipedFrameCount(0)
  , mCapturedFrameCount(0)
  , mFirstFrameTime(0)
  , mCurTime(0)
  , mEstimatedNextFrameTime(0)
  , mSkipThreashold(0)
  , mWaitThreashold(0)
  , mCurModuleID(EVideoInputModuleID_Auto)
{
  mbSnapshotCaptureMode = 0;
  mMaxSnapshotCaptureFramenumber = 0;
}

EECode CVideoCapturerActive::Construct()
{
  DSET_MODULE_LOG_CONFIG(ELogModuleVideoCapturerFilter);
  mPersistEOSBuffer.SetBufferType(EBufferType_FlowControl_EOS);
  memset(&mModuleParams, 0, sizeof(SVideoCaptureParams));
  if (mpPersistMediaConfig->video_capture_config.framerate_num && mpPersistMediaConfig->video_capture_config.framerate_den) {
    mCapFramerateNum = mpPersistMediaConfig->video_capture_config.framerate_num;
    mCapFramerateDen = mpPersistMediaConfig->video_capture_config.framerate_den;
  }
  return inherited::Construct();
}

CVideoCapturerActive::~CVideoCapturerActive()
{
}

void CVideoCapturerActive::Delete()
{
  if (mpCapturer) {
    mpCapturer->Destroy();
    mpCapturer = NULL;
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

EECode CVideoCapturerActive::Initialize(TChar *p_param)
{
  EVideoInputModuleID id;
  EECode err = EECode_OK;
  id = _guess_video_capturer_type_from_string(p_param);
  LOGM_INFO("Initialize() start, id %d\n", id);
  if (mbContentSetup) {
    LOGM_INFO("Initialize() start, there's a decoder already\n");
    DASSERT(mpCapturer);
    if (!mpCapturer) {
      LOGM_FATAL("[Internal bug], why the mpCapturer is NULL here?\n");
      return EECode_InternalLogicalBug;
    }
    LOGM_INFO("before mpCapturer->DestroyContext()\n");
    mpCapturer->DestroyContext();
    mbContentSetup = 0;
    if (id != mCurModuleID) {
      LOGM_INFO("before mpCapturer->Delete(), cur id %d request id %d\n", (TU32)mCurModuleID, (TU32)id);
      mpCapturer->Destroy();
      mpCapturer = NULL;
    }
  }
  if (!mpCapturer) {
    LOGM_INFO("Initialize() start, before gfModuleFactoryVideoInput(%d)\n", (TU32)id);
    mpCapturer = gfModuleFactoryVideoInput(id, mpPersistMediaConfig, mpEngineMsgSink);
    if (mpCapturer) {
      mCurModuleID = id;
    } else {
      mCurModuleID = EVideoInputModuleID_Auto;
      LOGM_FATAL("[Internal bug], request gfModuleFactoryVideoInput(%d) fail\n", (TU32)id);
      return EECode_InternalLogicalBug;
    }
  }
  LOGM_INFO("before mpCapturer->SetupContext()\n");
  err = mpCapturer->SetupContext(&mModuleParams);
  if (DUnlikely(EECode_OK != err)) {
    LOGM_ERROR("mpCapturer->SetupContext() failed.\n");
    return err;
  }
  mpCapturer->SetBufferPool(mpBufferPool);
  mbContentSetup = 1;
  LOGM_INFO("Initialize() done\n");
  return EECode_OK;
}

EECode CVideoCapturerActive::Finalize()
{
  if (mpCapturer) {
    mpCapturer->DestroyContext();
    mbContentSetup = 0;
    mpCapturer->Destroy();
    mpCapturer = NULL;
  }
  return EECode_OK;
}

EECode CVideoCapturerActive::SwitchInput(TComponentIndex focus_input_index)
{
  LOGM_FATAL("TO DO\n");
  return EECode_NoImplement;
}

EECode CVideoCapturerActive::FlowControl(EBufferType flowcontrol_type)
{
  LOGM_FATAL("TO DO\n");
  return EECode_NoImplement;
}

EECode CVideoCapturerActive::AddInputPin(TUint &index, TUint type)
{
  LOGM_FATAL("Not support yet\n");
  return EECode_InternalLogicalBug;
}

EECode CVideoCapturerActive::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
  DASSERT(StreamType_Video == type);
  if (StreamType_Video == type) {
    if (mpOutputPin == NULL) {
      mpOutputPin = COutputPin::Create("[Video output pin for CVideoCapturerActive filter]", (IFilter *) this);
      if (!mpOutputPin)  {
        LOGM_FATAL("COutputPin::Create() fail?\n");
        return EECode_NoMemory;
      }
      mpBufferPool = CBufferPool::Create("[Buffer pool for video output pin in CVideoCapturerActive filter]", 5);
      if (!mpBufferPool)  {
        LOGM_FATAL("CBufferPool::Create() fail?\n");
        return EECode_NoMemory;
      }
      mpOutputPin->SetBufferPool(mpBufferPool);
    }
    index = 0;
    if (mpOutputPin->AddSubPin(sub_index) != EECode_OK) {
      LOGM_FATAL("COutputPin::AddSubPin() fail?\n");
      return EECode_Error;
    }
  } else {
    LOGM_ERROR("BAD output pin type %d\n", type);
    return EECode_InternalLogicalBug;
  }
  return EECode_OK;
}

IOutputPin *CVideoCapturerActive::GetOutputPin(TUint index, TUint sub_index)
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

IInputPin *CVideoCapturerActive::GetInputPin(TUint index)
{
  return NULL;
}

EECode CVideoCapturerActive::FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
  EECode ret = EECode_OK;
  switch (property_type) {
    case EFilterPropertyType_video_capture_params: {
      SVideoCaptureParams *param = (SVideoCaptureParams *) p_param;
      if (param) {
        mModuleParams = *param;
        if (mModuleParams.framerate_num && mModuleParams.framerate_den) {
          mCapFramerateNum = mModuleParams.framerate_num;
          mCapFramerateDen = mModuleParams.framerate_den;
        }
        mbSnapshotCaptureMode = mModuleParams.b_snapshot_mode;
        LOGM_CONFIGURATION("num %d, den %d, snapshot mode %d\n", mCapFramerateNum, mCapFramerateDen, mbSnapshotCaptureMode);
      }
    }
    break;
    case EFilterPropertyType_video_capture_snapshot_params: {
      SVideoCaptureSnapshotParams *params = (SVideoCaptureSnapshotParams *) p_param;
      if (EModuleState_WaitSnapshotCapture != msState) {
        LOGM_WARN("previous not finished, state 0x%08x\n", msState);
        return EECode_Busy;
      }
      if (params) {
        SCMD cmd;
        cmd.code = ECMDType_SnapshotCapture;
        cmd.res32_1 = params->snapshot_framenumber;
        ret = mpWorkQ->SendCmd(cmd);
      } else {
        LOGM_FATAL("NULL params\n");
        return EECode_BadParam;
      }
    }
    break;
    case EFilterPropertyType_video_capture_periodic_params: {
      SVideoCapturePeriodicParams *params = (SVideoCapturePeriodicParams *) p_param;
      if ((EModuleState_WaitSnapshotCapture != msState) && (EModuleState_SnapshotCapture != msState)) {
        LOGM_ERROR("bad state, 0x%08x\n", msState);
        return EECode_BadState;
      }
      if (params) {
        SCMD cmd;
        cmd.code = ECMDType_PeriodicCapture;
        cmd.res32_1 = params->framerate_num;
        cmd.res64_1 = params->framerate_den;
        ret = mpWorkQ->SendCmd(cmd);
      } else {
        LOGM_FATAL("NULL params\n");
        return EECode_BadParam;
      }
    }
    break;
    default:
      LOGM_FATAL("BAD property 0x%08x\n", property_type);
      ret = EECode_InternalLogicalBug;
      break;
  }
  return ret;
}

void CVideoCapturerActive::GetInfo(INFO &info)
{
  info.mPriority = 0;
  info.mFlags = 0;
  info.nInput = 0;
  info.nOutput = 1;
  info.pName = mpModuleName;
}

void CVideoCapturerActive::PrintStatus()
{
  LOGM_PRINTF("msState %d, skiped %" DPrint64u ", captured %" DPrint64u ", heart beat %d\n", msState, mSkipedFrameCount, mCapturedFrameCount, mDebugHeartBeat);
  if (mpOutputPin) {
    LOGM_PRINTF("       mpOutputPin free buffer count %d\n", mpOutputPin->GetBufferPool()->GetFreeBufferCnt());
  }
  mDebugHeartBeat = 0;
}

void CVideoCapturerActive::OnRun()
{
  SCMD cmd;
  EECode err = EECode_OK;
  TU32 cap_fail = 0;
  TU8 cur_cap_frame = 0;
  //TTime begin_cap_time = 0, end_cap_time = 0, previous_cap_time = 0;
  mpWorkQ->CmdAck(EECode_OK);
  mbRun = 1;
  msState = EModuleState_WaitCmd;
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
      case EModuleState_WaitTiming:
        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
        processCmd(cmd);
        break;
      case EModuleState_Error:
      case EModuleState_Completed:
      case EModuleState_Pending:
        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
        processCmd(cmd);
        break;
      case EModuleState_Running:
        if (!mpBuffer) {
          mpOutputPin->AllocBuffer(mpBuffer, sizeof(CIBuffer));
        }
        while (mpWorkQ->PeekCmd(cmd)) {
          processCmd(cmd);
        }
        if (DUnlikely(EModuleState_Running != msState)) {
          break;
        }
        mCurTime = mpClockReference->GetCurrentTime();
        if ((mCurTime + mWaitThreashold) < (mEstimatedNextFrameTime)) {
          mpClockReference->AddTimer(this, EClockTimerType_once, mEstimatedNextFrameTime, mCaptureDuration, 1);
          msState = EModuleState_WaitTiming;
          break;
        }
        if (mCurTime > (mEstimatedNextFrameTime + mSkipThreashold)) {
          mSkipedFrameCount ++;
          mEstimatedNextFrameTime += mCaptureDuration;
        }
        //begin_cap_time = mpClockReference->GetCurrentTime();
        err = mpCapturer->Capture(mpBuffer);
        //end_cap_time = mpClockReference->GetCurrentTime();
        //LOGM_PRINTF("previous %" DPrint64d ", capture %" DPrint64d "\n", begin_cap_time - previous_cap_time, end_cap_time - begin_cap_time);
        //previous_cap_time = begin_cap_time;
        if (DUnlikely(EECode_OK != err)) {
          mpBuffer->SetBufferFlags(CIBuffer::DUP_FRAME);
          cap_fail = 1;
          LOGM_WARN("send duplicate frame\n");
        } else {
          cap_fail = 0;
        }
        if (DLikely(mCapturedFrameCount)) {
          TTime time = mCurTime - mFirstFrameTime;
          mpBuffer->SetBufferDTS(time);
          mpBuffer->SetBufferPTS(time);
          time = (time * 9) / 100;
          mpBuffer->SetBufferNativeDTS(time);
          mpBuffer->SetBufferNativePTS(time);
          mpBuffer->SetBufferLinearDTS(time);
          mpBuffer->SetBufferLinearPTS(time);
          //LOGM_PRINTF("capture, set pts %" DPrint64d "\n", time);
        } else {
          mFirstFrameTime = mCurTime;
          mpBuffer->SetBufferDTS(0);
          mpBuffer->SetBufferPTS(0);
          mpBuffer->SetBufferNativeDTS(0);
          mpBuffer->SetBufferNativePTS(0);
          mpBuffer->SetBufferLinearDTS(0);
          mpBuffer->SetBufferLinearPTS(0);
        }
        //LOGM_NOTICE("[debug flow]: capture done, send it\n");
        if (!cap_fail) {
          //LOGM_NOTICE("send frame\n");
          mpOutputPin->SendBuffer(mpBuffer);
          mpBuffer = NULL;
        }
        mCapturedFrameCount ++;
        mEstimatedNextFrameTime += mCaptureDuration;
        break;
      case EModuleState_SnapshotCapture:
        LOGM_NOTICE("[debug]: start snapshot capture (%d)\n", mMaxSnapshotCaptureFramenumber);
        if (5 < mMaxSnapshotCaptureFramenumber) {
          LOGM_WARN("max is 5 frames\n");
          mMaxSnapshotCaptureFramenumber = 5;
        }
        cur_cap_frame = 0;
        while (cur_cap_frame < mMaxSnapshotCaptureFramenumber) {
          cur_cap_frame ++;
          if (!mpBuffer) {
            mpOutputPin->AllocBuffer(mpBuffer, sizeof(CIBuffer));
          }
          err = mpCapturer->Capture(mpBuffer);
          if (DLikely(EECode_OK == err)) {
            if (cur_cap_frame == mMaxSnapshotCaptureFramenumber) {
              //mpBuffer->SetBufferFlagBits(CIBuffer::LAST_FRAME_INDICATOR);
            }
            mpOutputPin->SendBuffer(mpBuffer);
            mpBuffer = NULL;
          } else {
            LOGM_WARN("capture fail, ret 0x%08x, %s\n", err, gfGetErrorCodeString(err));
          }
        }
        LOGM_NOTICE("[debug]: snapshot capture end\n");
        msState = EModuleState_WaitSnapshotCapture;
        break;
      case EModuleState_WaitSnapshotCapture:
        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
        processCmd(cmd);
        break;
      default:
        LOGM_ERROR("state invalid: 0x%x\n", (TUint)msState);
        msState = EModuleState_Error;
        break;
    }
    mDebugHeartBeat ++;
  }
}

void CVideoCapturerActive::EventNotify(EEventType type, TU64 param1, TPointer param2)
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

void CVideoCapturerActive::processCmd(SCMD &cmd)
{
  //LOGM_FLOW("processCmd, cmd.code %d, %s, state %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code), msState, gfGetModuleStateString(msState));
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
      mpBufferPool->SetReleaseBufferCallBack(NULL);
      break;
    case ECMDType_Pause:
      mbPaused = 1;
      break;
    case ECMDType_Resume:
      mbPaused = 0;
      break;
    case ECMDType_Flush:
      flushData();
      msState = EModuleState_Pending;
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_ResumeFlush:
      msState = EModuleState_Idle;
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_NotifySynced:
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_Start:
      if (EModuleState_WaitCmd == msState) {
        if (!mbSnapshotCaptureMode) {
          msState = EModuleState_Running;
          beginCapture(mCapFramerateNum, mCapFramerateDen);
        } else {
          if (mMaxSnapshotCaptureFramenumber) {
            msState = EModuleState_SnapshotCapture;
          } else {
            msState = EModuleState_WaitSnapshotCapture;
          }
        }
      }
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case EMSGType_TimeNotify:
      if (EModuleState_WaitTiming == msState) {
        msState = EModuleState_Running;
      }
      break;
    case ECMDType_SnapshotCapture:
      if (mpBuffer) {
        mpBuffer->Release();
        mpBuffer = NULL;
      }
      if ((EModuleState_WaitSnapshotCapture != msState)) {
        LOGM_ERROR("not wait snapshot state 0x%08x\n", msState);
        mpWorkQ->CmdAck(EECode_BadState);
        break;
      }
      mMaxSnapshotCaptureFramenumber = cmd.res32_1;
      msState = EModuleState_SnapshotCapture;
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_PeriodicCapture:
      msState = EModuleState_Running;
      if (cmd.res32_1) {
        mCapFramerateNum = cmd.res32_1;
      }
      if (cmd.res64_1) {
        mCapFramerateDen = (TU32) cmd.res64_1;
      }
      beginCapture(mCapFramerateNum, mCapFramerateDen);
      mpWorkQ->CmdAck(EECode_OK);
      break;
    default:
      LOGM_ERROR("processCmd, wrong cmd 0x%08x, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code));
      break;
  }
}

void CVideoCapturerActive::flushData()
{
  if (mpBuffer) {
    mpBuffer->Release();
    mpBuffer = NULL;
  }
}

void CVideoCapturerActive::beginCapture(TU32 fr_num, TU32 fr_den)
{
  if (DLikely(fr_num && fr_den)) {
    mCaptureDuration = (TTime)(((TTime)fr_den * (TTime)1000000) / (TTime)fr_num);
    LOGM_NOTICE("video cap fr %d/%d, duration %" DPrint64d "\n", fr_num, fr_den, mCaptureDuration);
  } else {
    mCaptureDuration = 33333;
    LOGM_WARN("not valid params for cap fr, use default duration %" DPrint64d "\n", mCaptureDuration);
  }
  mSkipThreashold = mCaptureDuration;
  mWaitThreashold = mCaptureDuration / 2;
  mSkipedFrameCount = 0;
  mCapturedFrameCount = 0;
}


