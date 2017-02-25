/**
 * am_audio_decoder.cpp
 *
 * History:
 *    2013/01/07 - [Zhi He] create file
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

#include "am_audio_decoder.h"

IFilter *gfCreateAudioDecoderFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
  return CAudioDecoder::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

static EAudioDecoderModuleID _guess_audio_decoder_type_from_string(TChar *string)
{
  if (!string) {
    LOG_NOTICE("NULL input in _guess_audio_decoder_type_from_string, choose default\n");
    return EAudioDecoderModuleID_LIBAAC;
  }
  if (!strncmp(string, DNameTag_LIBAAC, strlen(DNameTag_LIBAAC))) {
#ifdef BUILD_MODULE_LIBAAC
    return EAudioDecoderModuleID_LIBAAC;
#endif
  }
  LOG_WARN("unknown string tag(%s) in _guess_audio_decoder_type_from_string, choose default\n", string);
  return EAudioDecoderModuleID_LIBAAC;
}

static EAudioDecoderModuleID _guess_audio_decoder_type_from_codec_format(StreamFormat format)
{
  switch (format) {
    case StreamFormat_AAC:
#ifdef BUILD_MODULE_LIBAAC
      return EAudioDecoderModuleID_LIBAAC;
#endif
      break;
    default:
      LOG_WARN("not supported format %d\n", format);
      break;
  }
  return EAudioDecoderModuleID_Invalid;
}

//-----------------------------------------------------------------------
//
// CAudioDecoder
//
//-----------------------------------------------------------------------

IFilter *CAudioDecoder::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
  CAudioDecoder *result = new CAudioDecoder(pName, pPersistMediaConfig, pEngineMsgSink, index);
  if ((result) && (EECode_OK != result->Construct())) {
    result->Delete();
    result = NULL;
  }
  return result;
}

CAudioDecoder::CAudioDecoder(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
  : inherited(pName, index, pPersistMediaConfig, pEngineMsgSink)
  , mpDecoder(NULL)
  , mCurDecoderID(EAudioDecoderModuleID_Auto)
  , mbUsePresetMode(0)
  , mbNeedSendSyncPointBuffer(1)
  , mbMuted(0)
  , mbNewSequence(0)
  , mAudioCurrentCodecID(StreamFormat_AAC)
  , mAudioConfigSampleFormat(AudioSampleFMT_S16)
  , mAudioConfigChannelNumber(DDefaultAudioChannelNumber)
  , mAudioConfigInterlave(0)
  , mAudioConfigSampleRate(DDefaultAudioSampleRate)
  , mAudioConfigFramesize(DDefaultAudioFrameSize)
  , mAudioConfigBitRate(64000)
  , mbDecoderContentSetup(0)
  , mbWaitKeyFrame(1)
  , mbDecoderRunning(0)
  , mbDecoderStarted(0)
  , mbDecoderSuspended(0)
  , mbDecoderPaused(0)
  , mbBackward(0)
  , mScanMode(0)
  , mnCurrentInputPinNumber(0)
  , mpCurInputPin(NULL)
  , mnCurrentOutputPinNumber(0)
  , mpOutputPin(NULL)
  , mpBufferPool(NULL)
  , mpMemPool(NULL)
  , mpBuffer(NULL)
  , mpOutputBuffer(NULL)
  , mPCMBlockSize(192000)//can be smaller, if use ring buffer pool, TODO
{
  TUint i = 0;
  for (i = 0; i < EConstAudioDecoderMaxInputPinNumber; i ++) {
    mpInputPins[i] = NULL;
  }
}

EECode CAudioDecoder::Construct()
{
  DSET_MODULE_LOG_CONFIG(ELogModuleAudioDecoderFilter);
  //mConfigLogLevel = ELogLevel_Debug;
  //mConfigLogOption = DCAL_BITMASK(ELogOption_Flow) | DCAL_BITMASK(ELogOption_State);
  mPersistEOSBuffer.SetBufferType(EBufferType_FlowControl_EOS);
  return inherited::Construct();
}

CAudioDecoder::~CAudioDecoder()
{
}

void CAudioDecoder::Delete()
{
  TUint i = 0;
  LOGM_RESOURCE("CAudioDecoder::Delete(), before mpDecoder->Delete().\n");
  if (mpDecoder) {
    mpDecoder->Destroy();
    mpDecoder = NULL;
  }
  LOGM_RESOURCE("CAudioDecoder::Delete(), before delete inputpins.\n");
  for (i = 0; i < EConstAudioDecoderMaxInputPinNumber; i ++) {
    if (mpInputPins[i]) {
      mpInputPins[i]->Delete();
      mpInputPins[i] = NULL;
    }
    mnCurrentInputPinNumber = 0;
  }
  LOGM_RESOURCE("CAudioDecoder::Delete(), before delete output pin.\n");
  if (mpOutputPin) {
    mpOutputPin->Delete();
    mpOutputPin = NULL;
  }
  LOGM_RESOURCE("CAudioDecoder::Delete(), before delete buffer pool.\n");
  if (mpBufferPool) {
    mpBufferPool->Delete();
    mpBufferPool = NULL;
  }
  LOGM_RESOURCE("CAudioDecoder::Delete(), before delete memory pool.\n");
  if (mpMemPool) {
    mpMemPool->GetObject0()->Delete();
    mpMemPool = NULL;
  }
  LOGM_RESOURCE("CAudioDecoder::Delete(), before inherited::Delete().\n");
  inherited::Delete();
}

EECode CAudioDecoder::initialize(EAudioDecoderModuleID id)
{
  //LOGM_INFO("initialize() start, id %d ori id %d\n", id, mCurDecoderID);
  if (mbDecoderContentSetup) {
    //LOGM_INFO("initialize() start, there's a decoder already\n");
    if (!mpDecoder) {
      LOGM_FATAL("[Internal bug], why the mpDecoder is NULL here?\n");
      return EECode_InternalLogicalBug;
    }
    if (mbDecoderRunning) {
      mbDecoderRunning = 0;
      //LOGM_INFO("before mpDecoder->Stop()\n");
      mpDecoder->Stop();
    }
    //LOGM_INFO("before mpDecoder->DestroyContext()\n");
    mpDecoder->DestroyContext();
    mbDecoderContentSetup = 0;
    if (id != mCurDecoderID) {
      //LOGM_INFO("before mpDecoder->Delete(), cur id %d, request id %d\n", mCurDecoderID, id);
      mpDecoder->Destroy();
      mpDecoder = NULL;
    }
  } else {
    if (mpDecoder && id != mCurDecoderID) {
      //LOGM_INFO("before mpDecoder->Delete(), cur id %d, request id %d\n", mCurDecoderID, id);
      mpDecoder->Destroy();
      mpDecoder = NULL;
    }
  }
  if (!mpDecoder) {
    //LOGM_INFO("initialize() start, before gfModuleFactoryAudioDecoder(%d)\n", id);
    mpDecoder = gfModuleFactoryAudioDecoder(id, mpPersistMediaConfig, mpEngineMsgSink);
    if (mpDecoder) {
      mCurDecoderID = id;
    } else {
      mCurDecoderID = EAudioDecoderModuleID_Auto;
      LOGM_FATAL("[Internal bug], request gfModuleFactoryAudioDecoder(%d) fail\n", id);
      return EECode_InternalLogicalBug;
    }
  }
  //LOGM_INFO("initialize() done\n");
  return EECode_OK;
}

EECode CAudioDecoder::Initialize(TChar *p_param)
{
  EAudioDecoderModuleID id = _guess_audio_decoder_type_from_string(p_param);
  EECode err = initialize(id);

  if (EECode_OK != err) {
    LOGM_ERROR("initialize() fail\n");
    return err;
  }
  if (mbUsePresetMode) {
    err = setupDecoderContext();
    if (EECode_OK != err) {
      LOGM_ERROR("setupDecoderContext fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
      return err;
    }
  }
  //LOGM_INFO("[DecoderFilter flow]: Initialize() done\n");
  return EECode_OK;
}

EECode CAudioDecoder::Finalize()
{
  destroyDecoderContext();
  return EECode_OK;
}

EECode CAudioDecoder::Run()
{
  mbDecoderRunning = 1;
  inherited::Run();
  return EECode_OK;
}

EECode CAudioDecoder::Start()
{
  EECode err;
  if (mpDecoder) {
    mpDecoder->Start();
    mbDecoderStarted = 1;
  } else {
    LOGM_WARN("NULL mpDecoder.\n");
  }
  err = mpWorkQ->SendCmd(ECMDType_Start);
  return err;
}

EECode CAudioDecoder::Stop()
{
  mbDecoderStarted = 0;
  if (mpDecoder) {
    mpDecoder->Stop();
  } else {
    LOGM_FATAL("NULL mpDecoder.\n");
  }
  inherited::Stop();
  return EECode_OK;
}

void CAudioDecoder::Pause()
{
  if (mpDecoder) {
    DASSERT(!mbDecoderPaused);
    //mpDecoder->Pause();
    mbDecoderPaused = 1;
  } else {
    LOGM_FATAL("NULL mpDecoder.\n");
  }
  inherited::Pause();
  return;
}

void CAudioDecoder::Resume()
{
  if (mpDecoder) {
    DASSERT(mbDecoderPaused);
    //mpDecoder->Resume();
    mbDecoderPaused = 0;
  } else {
    LOGM_FATAL("NULL mpDecoder.\n");
  }
  inherited::Resume();
  return;
}

EECode CAudioDecoder::SwitchInput(TComponentIndex focus_input_index)
{
  SCMD cmd(ECMDType_SwitchInput);
  cmd.res32_1 = (TU32)focus_input_index;
  return mpWorkQ->SendCmd(cmd);
}

EECode CAudioDecoder::FlowControl(EBufferType flowcontrol_type)
{
  LOGM_FATAL("TO DO\n");
  return EECode_NoImplement;
}

EECode CAudioDecoder::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
  DASSERT(StreamType_Audio == type);
  if (StreamType_Audio == type) {
    if (mpOutputPin) {
      LOGM_ERROR("there's already a output pin in CAudioDecoder\n");
      return EECode_InternalLogicalBug;
    }
    mpOutputPin = COutputPin::Create("[Video output pin for CAudioDecoder filter]", (IFilter *) this);
    if (!mpOutputPin)  {
      LOGM_FATAL("COutputPin::Create() fail?\n");
      return EECode_NoMemory;
    }
    mpBufferPool = CBufferPool::Create("[Buffer pool for audio output pin in CAudioDecoder filter]", DDefaultAudioBufferNumber);
    if (!mpBufferPool)  {
      LOGM_FATAL("CBufferPool::Create() fail?\n");
      return EECode_NoMemory;
    }
    mpMemPool = CFixedMemPool::Create("[Memory pool for audio output in CAudioDecoder filter]", mPCMBlockSize, DDefaultAudioBufferNumber);
    if (!mpMemPool)  {
      LOGM_FATAL("mpMemPool::Create() fail?\n");
      return EECode_NoMemory;
    }
    mpOutputPin->SetBufferPool(mpBufferPool);
    mpBufferPool->AddBufferNotifyListener((IEventListener *) this);
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

EECode CAudioDecoder::AddInputPin(TUint &index, TUint type)
{
  if (StreamType_Audio == type) {
    if (mnCurrentInputPinNumber >= EConstAudioDecoderMaxInputPinNumber) {
      LOGM_ERROR("Max input pin number reached.\n");
      return EECode_InternalLogicalBug;
    }
    index = mnCurrentInputPinNumber;
    if (mpInputPins[mnCurrentInputPinNumber]) {
      LOGM_FATAL("not NULL mpInputPins[%d]\n", mnCurrentInputPinNumber);
    }
    mpInputPins[mnCurrentInputPinNumber] = CQueueInputPin::Create((const TChar *)"[Audio input pin for CAudioDecoder]", (IFilter *) this, StreamType_Audio, mpWorkQ->MsgQ());
    DASSERT(mpInputPins[mnCurrentInputPinNumber]);
    if (!mpCurInputPin) {
      mpCurInputPin = mpInputPins[mnCurrentInputPinNumber];
      //mpInputPins[mnCurrentInputPinNumber]->Enable(0);
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

IOutputPin *CAudioDecoder::GetOutputPin(TUint index, TUint sub_index)
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

IInputPin *CAudioDecoder::GetInputPin(TUint index)
{
  if (index < mnCurrentInputPinNumber) {
    return mpInputPins[index];
  }
  LOGM_FATAL("BAD index %d, exceed mnCurrentInputPinNumber %d\n", index, mnCurrentInputPinNumber);
  return NULL;
}

EECode CAudioDecoder::FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
  EECode ret = EECode_OK;
  switch (property_type) {
    case EFilterPropertyType_audio_parameters:
      DASSERT(p_param);
      DASSERT(!mbDecoderContentSetup);
      if (p_param) {
        SAudioParams *parameters = (SAudioParams *) p_param;
        if (set_or_get) {
          mAudioConfigChannelNumber = parameters->channel_number;
          mAudioConfigSampleFormat = parameters->sample_format;
          mAudioConfigSampleRate = parameters->sample_rate;
          mAudioConfigInterlave = parameters->is_channel_interlave;
          mAudioConfigBitRate = parameters->bitrate;
          mAudioCurrentCodecID = parameters->codec_format;
          mAudioConfigCustomizedCodecID = parameters->customized_codec_type;
        } else {
          parameters->channel_number = mAudioConfigChannelNumber;
          parameters->sample_format = (AudioSampleFMT)mAudioConfigSampleFormat;
          parameters->sample_rate = mAudioConfigSampleRate;
          parameters->is_channel_interlave = mAudioConfigInterlave;
          parameters->bitrate = mAudioConfigBitRate;
          parameters->codec_format = mAudioCurrentCodecID;
          parameters->customized_codec_type = mAudioConfigCustomizedCodecID;
        }
      } else {
        LOGM_FATAL("NULL p_param\n");
        ret = EECode_InternalLogicalBug;
      }
      break;
    case EFilterPropertyType_audio_channelnumber:
      DASSERT(p_param);
      DASSERT(!mbDecoderContentSetup);
      if (p_param) {
        SAudioParams *parameters = (SAudioParams *) p_param;
        if (set_or_get) {
          mAudioConfigChannelNumber = parameters->channel_number;
        } else {
          parameters->channel_number = mAudioConfigChannelNumber;
        }
      } else {
        LOGM_FATAL("NULL p_param\n");
        ret = EECode_InternalLogicalBug;
      }
      break;
    case EFilterPropertyType_audio_samplerate:
      DASSERT(p_param);
      DASSERT(!mbDecoderContentSetup);
      if (p_param) {
        SAudioParams *parameters = (SAudioParams *) p_param;
        if (set_or_get) {
          mAudioConfigSampleRate = parameters->sample_rate;
        } else {
          parameters->sample_rate = mAudioConfigSampleRate;
        }
      } else {
        LOGM_FATAL("NULL p_param\n");
        ret = EECode_InternalLogicalBug;
      }
      break;
    case EFilterPropertyType_audio_format:
      DASSERT(p_param);
      DASSERT(!mbDecoderContentSetup);
      if (p_param) {
        SAudioParams *parameters = (SAudioParams *) p_param;
        if (set_or_get) {
          mAudioConfigSampleFormat = parameters->sample_format;
        } else {
          parameters->sample_format = (AudioSampleFMT)mAudioConfigSampleFormat;
        }
      } else {
        LOGM_FATAL("NULL p_param\n");
        ret = EECode_InternalLogicalBug;
      }
      break;
    case EFilterPropertyType_audio_bitrate:
      DASSERT(p_param);
      DASSERT(!mbDecoderContentSetup);
      if (p_param) {
        SAudioParams *parameters = (SAudioParams *) p_param;
        if (set_or_get) {
          mAudioConfigBitRate = parameters->bitrate;
        } else {
          parameters->bitrate = mAudioConfigBitRate;
        }
      } else {
        LOGM_FATAL("NULL p_param\n");
        ret = EECode_InternalLogicalBug;
      }
      break;
    case EFilterPropertyType_arenderer_mute:
      SendCmd(ECMDType_MuteAudio);
      break;
    case EFilterPropertyType_arenderer_umute:
      SendCmd(ECMDType_UnMuteAudio);
      break;
    default:
      LOGM_FATAL("BAD property 0x%08x\n", property_type);
      ret = EECode_InternalLogicalBug;
      break;
  }
  return ret;
}

void CAudioDecoder::GetInfo(INFO &info)
{
  info.mPriority = 0;
  info.mFlags = 0;
  info.nInput = mnCurrentInputPinNumber;
  info.nOutput = 1;
  info.pName = mpModuleName;
  return;
}

void CAudioDecoder::PrintStatus()
{
  TUint i = 0;
  LOGM_PRINTF("\t[PrintStatus] CAudioDecoder[%d] msState=%d, %s, mnCurrentInputPinNumber %d, heart beat %d\n", mIndex, msState, gfGetModuleStateString(msState), mnCurrentInputPinNumber, mDebugHeartBeat);
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
  mDebugHeartBeat = 0;
  return;
}

void CAudioDecoder::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
  LOGM_FLOW("EventNotify, event type %d.\n", (TInt)type);
  if (mbDecoderStarted) {
    switch (type) {
      case EEventType_BufferReleaseNotify:
        mpWorkQ->PostMsg(ECMDType_NotifyBufferRelease);
        break;
      default:
        LOGM_ERROR("event type unsupported:  %d", (TInt)type);
    }
  }
}

void CAudioDecoder::processCmd(SCMD &cmd)
{
  LOGM_FLOW("processCmd, cmd.code %d, %s, state %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code), msState, gfGetModuleStateString(msState));
  switch (cmd.code) {
    case ECMDType_ExitRunning:
      mbRun = 0;
      flushInputData();
      msState = EModuleState_Halt;
      mpWorkQ->CmdAck(EECode_OK);
      LOGM_INFO("processCmd, ECMDType_Stop.\n");
      break;
    case ECMDType_Stop:
      msState = EModuleState_WaitCmd;
      mpWorkQ->CmdAck(EECode_OK);
      LOGM_INFO("processCmd, ECMDType_Stop.\n");
      break;
    case ECMDType_Pause:
      DASSERT(!mbPaused);
      mbPaused = 1;
      break;
    case ECMDType_Resume:
      if (EModuleState_Pending == msState) {
        msState = EModuleState_Idle;
        LOGM_INFO("from EModuleState_Pending.\n");
      } else if (msState == EModuleState_Completed) {
        msState = EModuleState_Idle;
        LOGM_INFO("from EModuleState_Completed.\n");
      }
      mbPaused = 0;
      break;
    case ECMDType_Flush:
    case ECMDType_Suspend:
      if (mpBuffer) {
        mpBuffer->Release();
        mpBuffer = NULL;
      }
      if (mpCurInputPin) {
        mpCurInputPin->Purge();
      }
      if (mpDecoder) {
        mpDecoder->Flush();
      }
      msState = EModuleState_Pending;
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_ResumeFlush:
    case ECMDType_ResumeSuspend:
      if (EModuleState_Pending == msState) {
        msState = EModuleState_Idle;
        LOGM_INFO("ResumeFlush from EModuleState_Pending.\n");
      } else if (msState == EModuleState_Completed) {
        msState = EModuleState_Idle;
        LOGM_INFO("ResumeFlush from EModuleState_Completed.\n");
      } else if (msState == EModuleState_Error) {
        msState = EModuleState_Idle;
        LOGM_INFO("ResumeFlush from EModuleState_Error.\n");
      } else {
        msState = EModuleState_Idle;
        LOGM_FATAL("invalid msState=%d in ECMDType_ResumeFlush\n", msState);
      }
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_NotifySynced:
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_Start:
      if (EModuleState_WaitCmd == msState) {
        msState = EModuleState_Idle;
      }
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_SwitchInput: {
        TUint focus = cmd.res32_1;
        LOGM_NOTICE("ECMDType_SwitchInput to %d, mbMuted %d, msState %d, %s\n", focus, mbMuted, msState, gfGetModuleStateString(msState));
        if (DLikely(focus < mnCurrentInputPinNumber)) {
          if (mpBuffer) {
            mpBuffer->Release();
            mpBuffer = NULL;
          }
          if (mpCurInputPin) {
            mpCurInputPin->Purge(1);
          }
          mpCurInputPin = mpInputPins[focus];
          if (DLikely(mpCurInputPin)) {
            mpCurInputPin->Enable(!mbMuted);
          } else {
            LOGM_FATAL("NULL mpCurInputPin\n");
            msState = EModuleState_Pending;
            mpWorkQ->CmdAck(EECode_InternalLogicalBug);
            break;
          }
        } else {
          LOGM_ERROR("BAD request channel index %d, mnCurrentInputPinNumber %d\n", focus, mnCurrentInputPinNumber);
          msState = EModuleState_Pending;
          mpWorkQ->CmdAck(EECode_BadParam);
          break;
        }
        msState = EModuleState_Idle;
        mpWorkQ->CmdAck(EECode_OK);
      }
      break;
    case ECMDType_NotifySourceFilterBlocked:
      break;
    case ECMDType_NotifyBufferRelease:
      if (mpOutputPin) {
        if (mpOutputPin->GetBufferPool()->GetFreeBufferCnt() > 0) {
          if (msState == EModuleState_Idle)
          { msState = EModuleState_HasOutputBuffer; }
          else if (msState == EModuleState_HasInputData)
          { msState = EModuleState_Running; }
        }
      }
      break;
    case ECMDType_MuteAudio:
      LOGM_NOTICE("ECMDType_MuteAudio, msState %d, %s\n", msState, gfGetModuleStateString(msState));
      if (DUnlikely(mbMuted)) {
        LOGM_WARN("Already muted\n");
      } else {
        mbMuted = 1;
        if (mpBuffer) {
          mpBuffer->Release();
          mpBuffer = NULL;
        }
        if (mpCurInputPin) {
          mpCurInputPin->Purge(1);
        }
        msState = EModuleState_Pending;
      }
      mpWorkQ->CmdAck(EECode_OK);
      break;
    case ECMDType_UnMuteAudio:
      LOGM_NOTICE("ECMDType_UnMuteAudio, msState %d, %s\n", msState, gfGetModuleStateString(msState));
      if (DUnlikely(!mbMuted)) {
        LOGM_WARN("Already unmuted\n");
      } else {
        mbMuted = 0;
        if (DLikely(EModuleState_Pending == msState)) {
          if (mpCurInputPin) {
            mpCurInputPin->Enable(1);
          }
          msState = EModuleState_Idle;
        } else {
          LOGM_WARN("how to handle in this state %d, %s\n", msState, gfGetModuleStateString(msState));
        }
      }
      mpWorkQ->CmdAck(EECode_OK);
      break;
    default:
      LOGM_ERROR("processCmd, wrong cmd %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code));
      break;
  }
}

EECode CAudioDecoder::setupDecoderContext()
{
  EECode err = EECode_OK;
  SAudioParams param;
  DASSERT(!mbDecoderContentSetup);
  LOGM_INFO("[DecoderFilter flow], before mpDecoder->SetupContext()\n");
  param.channel_number = mAudioConfigChannelNumber;
  param.sample_format = (AudioSampleFMT)mAudioConfigSampleFormat;
  param.sample_rate = mAudioConfigSampleRate;
  param.bitrate = mAudioConfigBitRate;
  param.is_channel_interlave = mAudioConfigInterlave;
  param.codec_format = mAudioCurrentCodecID;
  param.customized_codec_type = mAudioConfigCustomizedCodecID;
  err = mpDecoder->SetupContext(&param);
  DASSERT_OK(err);
  if (EECode_OK == err) {
    mbDecoderContentSetup = 1;
  } else {
    LOGM_FATAL("mpDecoder->SetupContext(&param) fail, ret %d\n", err);
  }
  return err;
}

void CAudioDecoder::destroyDecoderContext()
{
  if (mpDecoder) {
    if (mbDecoderRunning) {
      mbDecoderRunning = 0;
      LOGM_INFO("[DecoderFilter flow], before mpDecoder->Stop()\n");
      mpDecoder->Stop();
    }
    LOGM_INFO("[DecoderFilter flow], before mpDecoder->DestroyContext()\n");
    mpDecoder->DestroyContext();
    mbDecoderContentSetup = 0;
    LOGM_INFO("[DecoderFilter flow], before mpDecoder->Delete(), cur id %d\n", mCurDecoderID);
    mpDecoder->Destroy();
    mpDecoder = NULL;
  }
}

TU32 CAudioDecoder::isFormatChanges(CIBuffer *p_buffer)
{
  DASSERT(p_buffer);
  if (mAudioConfigChannelNumber != p_buffer->mAudioChannelNumber) {
    LOGM_ERROR("[DEBUG]: channel number changes %d --> %d\n", mAudioConfigChannelNumber, p_buffer->mAudioChannelNumber);
    return 1;
  }
  if (mAudioConfigSampleFormat != p_buffer->mAudioSampleFormat) {
    LOGM_ERROR("[DEBUG]: sample format changes %d --> %d\n", mAudioConfigSampleFormat, p_buffer->mAudioSampleFormat);
    //return 1;
  }
  if (mAudioConfigSampleRate != p_buffer->mAudioSampleRate) {
    LOGM_ERROR("[DEBUG]: sample rate changes %d --> %d\n", mAudioConfigSampleRate, p_buffer->mAudioSampleRate);
    return 1;
  }
  if (mAudioConfigInterlave != p_buffer->mbAudioPCMChannelInterlave) {
    LOGM_ERROR("[DEBUG]: channel interlave changes %d --> %d\n", mAudioConfigInterlave, p_buffer->mbAudioPCMChannelInterlave);
    return 1;
  }
  if (mAudioCurrentCodecID != p_buffer->mContentFormat) {
    LOGM_ERROR("[DEBUG]: codec format changes %d --> %d\n", mAudioCurrentCodecID, p_buffer->mContentFormat);
    return 1;
  }
  return 0;
}

void CAudioDecoder::OnRun(void)
{
  TInt consuming_bytes = 0;
  EECode err = EECode_OK;
  SCMD cmd;
  CIQueue::QType type;
  IBufferPool *pBufferPool = NULL;
  mbRun = 1;
  if (DLikely(mpCurInputPin)) {
    msState = EModuleState_WaitCmd;
  } else {
    msState = EModuleState_Error;
    LOGM_ERROR("audio decoder no input pin\n");
  }
  mpWorkQ->CmdAck(EECode_OK);
  LOGM_INFO("OnRun loop, start\n");
  DASSERT(mpOutputPin);
  pBufferPool = mpOutputPin->GetBufferPool();
  DASSERT(pBufferPool);
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
        if (pBufferPool->GetFreeBufferCnt() > 0) {
          msState = EModuleState_HasOutputBuffer;
        } else {
          DASSERT(mpCurInputPin);
          type = mpWorkQ->WaitDataMsgWithSpecifiedQueue(&cmd, sizeof(cmd), mpCurInputPin->GetBufferQ());
          if (type == CIQueue::Q_MSG) {
            processCmd(cmd);
          } else {
            DASSERT(!mpBuffer);
            if (mpCurInputPin->PeekBuffer(mpBuffer)) {
              if (DUnlikely(EBufferType_FlowControl_EOS == mpBuffer->GetBufferType())) {
                msState = EModuleState_Pending;
                mPersistEOSBuffer.SetBufferType(EBufferType_FlowControl_EOS);
                LOGM_INFO("bit-stream comes to end, sending EOS to down filter...\n");
                mpOutputPin->SendBuffer(&mPersistEOSBuffer);
                mpBuffer->Release();
                mpBuffer = NULL;
              } else if (DUnlikely(EBufferType_FlowControl_Pause == mpBuffer->GetBufferType())) {
                LOGM_INFO("receive pause buffer, sending it to down filter...\n");
                mPersistEOSBuffer.SetBufferType(EBufferType_FlowControl_Pause);
                mpOutputPin->SendBuffer(&mPersistEOSBuffer);
                mpBuffer->Release();
                mpBuffer = NULL;
              } else {
                DASSERT(EBufferType_AudioES == mpBuffer->GetBufferType());
                msState = EModuleState_HasInputData;
              }
            }
          }
        }
        break;
      case EModuleState_HasOutputBuffer:
        DASSERT(pBufferPool->GetFreeBufferCnt() > 0);
        DASSERT(mpCurInputPin);
        type = mpWorkQ->WaitDataMsgWithSpecifiedQueue(&cmd, sizeof(cmd), mpCurInputPin->GetBufferQ());
        if (type == CIQueue::Q_MSG) {
          processCmd(cmd);
        } else {
          DASSERT(!mpBuffer);
          if (mpCurInputPin->PeekBuffer(mpBuffer)) {
            if (DUnlikely(EBufferType_FlowControl_EOS == mpBuffer->GetBufferType())) {
              msState = EModuleState_Pending;
              mPersistEOSBuffer.SetBufferType(EBufferType_FlowControl_EOS);
              LOGM_INFO("bit-stream comes to end, sending EOS to down filter...\n");
              mpOutputPin->SendBuffer(&mPersistEOSBuffer);
              mpBuffer->Release();
              mpBuffer = NULL;
            } else if (DUnlikely(EBufferType_FlowControl_Pause == mpBuffer->GetBufferType())) {
              LOGM_INFO("receive pause buffer, sending it to down filter...\n");
              mPersistEOSBuffer.SetBufferType(EBufferType_FlowControl_Pause);
              mpOutputPin->SendBuffer(&mPersistEOSBuffer);
              mpBuffer->Release();
              mpBuffer = NULL;
            } else {
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
        if (pBufferPool->GetFreeBufferCnt() > 0) {
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
          EAudioDecoderModuleID id = _guess_audio_decoder_type_from_codec_format((StreamFormat) mpBuffer->mContentFormat);
          LOGM_INFO("SYNC_POINT buffer comes.\n");
          if (DUnlikely(id != mCurDecoderID)) {
            err = initialize(id);
            if (DUnlikely(EECode_OK != err)) {
              LOG_ERROR("CAudioDecoder: initialize() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
              msState = EModuleState_Error;
              break;
            }
          }
          if (DUnlikely(mpBuffer->GetBufferFlags() & CIBuffer::NEW_SEQUENCE)) {
            mbNewSequence = 1;
          } else {
            mbNewSequence = 0;
          }
          mbNeedSendSyncPointBuffer = 1;
          if (!mbDecoderContentSetup) {
            mAudioConfigChannelNumber = mpBuffer->mAudioChannelNumber;
            mAudioConfigSampleFormat = mpBuffer->mAudioSampleFormat;
            mAudioConfigSampleRate = mpBuffer->mAudioSampleRate;
            mAudioConfigFramesize = mpBuffer->mAudioFrameSize;
            mAudioConfigInterlave = mpBuffer->mbAudioPCMChannelInterlave;
            mAudioCurrentCodecID = mpBuffer->mContentFormat;
            mAudioConfigCustomizedCodecID = mpBuffer->mCustomizedContentFormat;
            if (EBufferType_AudioExtraData == mpBuffer->GetBufferType()) {
              LOGM_INFO("Get audio extra data!\n");
              DASSERT(mpDecoder);
              if (mpDecoder) {
                mpDecoder->SetExtraData(mpBuffer->GetDataPtr(), mpBuffer->GetDataSize());
              }
            }
            err = setupDecoderContext();
            if (EECode_OK != err) {
              LOGM_ERROR("audio decoder setup fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
              msState = EModuleState_Error;
              mpBuffer->Release();
              mpBuffer = NULL;
              break;
            }
            msState = EModuleState_Idle;
            mpBuffer->Release();
            mpBuffer = NULL;
            break;
          } else {
            if (isFormatChanges(mpBuffer)) {
              destroyDecoderContext();
            }
            if (!mbDecoderContentSetup) {
              mAudioConfigChannelNumber = mpBuffer->mAudioChannelNumber;
              mAudioConfigSampleFormat = mpBuffer->mAudioSampleFormat;
              mAudioConfigSampleRate = mpBuffer->mAudioSampleRate;
              mAudioConfigInterlave = mpBuffer->mbAudioPCMChannelInterlave;
              mAudioCurrentCodecID = mpBuffer->mContentFormat;
              if (EBufferType_AudioExtraData == mpBuffer->GetBufferType()) {
                LOGM_INFO("Get audio extra data!\n");
                DASSERT(mpDecoder);
                if (mpDecoder) {
                  mpDecoder->SetExtraData(mpBuffer->GetDataPtr(), mpBuffer->GetDataSize());
                  mpBuffer->Release();
                  mpBuffer = NULL;
                }
                msState = EModuleState_Idle;
                break;
              }
              err = setupDecoderContext();
              if (EECode_OK != err) {
                LOGM_ERROR("audio decoder setup fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                msState = EModuleState_Error;
                mpBuffer->Release();
                mpBuffer = NULL;
                break;
              }
            }
          }
        }
        if (DUnlikely(EBufferType_AudioExtraData == mpBuffer->GetBufferType())) {
          LOGM_INFO("Get audio extra data!\n");
          DASSERT(mpDecoder);
          if (mpDecoder) {
            mpDecoder->SetExtraData(mpBuffer->GetDataPtr(), mpBuffer->GetDataSize());
            mpBuffer->Release();
            mpBuffer = NULL;
          }
          msState = EModuleState_Idle;
          break;
        }
        if (!mbDecoderContentSetup) {
          err = setupDecoderContext();
          if (EECode_OK != err) {
            LOGM_ERROR("audio decoder setup fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
            msState = EModuleState_Error;
            mpBuffer->Release();
            mpBuffer = NULL;
            break;
          }
        }
        DASSERT(!mpOutputBuffer);
        if (mpOutputPin->AllocBuffer(mpOutputBuffer)) {
          if (mpOutputBuffer) {
            if (!mpOutputBuffer->GetDataPtrBase(0)) {
              TU8 *mem = mpMemPool->RequestMemBlock(mPCMBlockSize);
              LOGM_INFO("alloc memory for pcm buffer, ret %p\n", mem);
              if (mem) {
                mpOutputBuffer->SetDataPtrBase(mem);
                mpOutputBuffer->SetDataMemorySize((TUint)mPCMBlockSize);
              } else {
                LOGM_FATAL("no memory?\n");
                mpBuffer->Release();
                mpBuffer = NULL;
                msState = EModuleState_Error;
                break;
              }
            }
          }
        }
        if (EECode_OK != mpDecoder->Decode(mpBuffer, mpOutputBuffer, consuming_bytes)) {
          LOGM_ERROR("OnRun: Decode Fail.\n");
          if (mpBuffer) {
            mpBuffer->Release();
            mpBuffer = NULL;
          }
          if (mpOutputBuffer) {
            mpOutputBuffer->Release();
            mpOutputBuffer = NULL;
          }
          if (mpCurInputPin) {
            mpCurInputPin->Purge(1);
          }
          msState = EModuleState_Error;
          break;
        } else {
          msState = EModuleState_Idle;
        }
        mpOutputBuffer->SetBufferType(EBufferType_AudioPCMBuffer);
        if (DUnlikely(mbNeedSendSyncPointBuffer)) {
          mbNeedSendSyncPointBuffer = 0;
          //AAC, mpOutputBuffer->mAudioChannelNumber will be filled by decoder
          if (mAudioCurrentCodecID != StreamFormat_AAC) {
            mpOutputBuffer->mAudioChannelNumber = mAudioConfigChannelNumber;
          }
          mpOutputBuffer->mAudioSampleFormat = mAudioConfigSampleFormat;
          mpOutputBuffer->mAudioSampleRate = mAudioConfigSampleRate;
          mpOutputBuffer->mAudioFrameSize = mAudioConfigFramesize;
          mpOutputBuffer->mbAudioPCMChannelInterlave = mAudioConfigInterlave;
          if (mbNewSequence) {
            mpOutputBuffer->SetBufferFlags(CIBuffer::SYNC_POINT | CIBuffer::KEY_FRAME | CIBuffer::NEW_SEQUENCE);
            mbNewSequence = 0;
          } else {
            mpOutputBuffer->SetBufferFlags(CIBuffer::SYNC_POINT | CIBuffer::KEY_FRAME);
          }
        } else {
          mpOutputBuffer->SetBufferFlags(CIBuffer::KEY_FRAME);
        }
        mpOutputPin->SendBuffer(mpOutputBuffer);
        mpOutputBuffer = NULL;
        mpBuffer->Release();
        mpBuffer = NULL;
        break;
      default:
        LOGM_ERROR("CAudioDecoder: OnRun: state invalid: 0x%x\n", (TUint)msState);
        msState = EModuleState_Error;
        break;
    }
    mDebugHeartBeat ++;
  }
}

EECode CAudioDecoder::flushInputData()
{
  TUint index = 0;
  if (mpBuffer) {
    mpBuffer->Release();
    mpBuffer = NULL;
  }
  LOGM_INFO("before purge input pins\n");
  for (index = 0; index < EConstAudioDecoderMaxInputPinNumber; index ++) {
    if (mpInputPins[index]) {
      mpInputPins[index]->Purge(1);
    }
  }
  LOGM_INFO("after purge input pins\n");
  return EECode_OK;
}


