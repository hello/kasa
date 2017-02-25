/*******************************************************************************
 * audio_renderer.cpp
 *
 * History:
 *    2013/01/24 - [Zhi He] create file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "media_mw_if.h"
#include "media_mw_utils.h"
#include "mw_internal.h"

#include "framework_interface.h"
#include "filters_interface.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "audio_renderer.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN

IFilter *gfCreateAudioRendererFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CAudioRenderer::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

static EAudioRendererModuleID _guess_audio_renderer_type_from_string(TChar *string)
{
    if (!string) {
        LOG_WARN("NULL input in _guess_audio_renderer_type_from_string, choose default\n");
        return EAudioRendererModuleID_ALSA;
    }

    if (!strncmp(string, DNameTag_ALSA, strlen(DNameTag_ALSA))) {
        return EAudioRendererModuleID_ALSA;
    } else if (!strncmp(string, DNameTag_DirectSound, strlen(DNameTag_DirectSound))) {
        return EAudioRendererModuleID_DirectSound;
    } else if (!strncmp(string, DNameTag_AndroidAudioOutput, strlen(DNameTag_AndroidAudioOutput))) {
        return EAudioRendererModuleID_AndroidAudioOutput;
    }

    LOG_WARN("unknown string tag(%s) in _guess_audio_renderer_type_from_string, choose default\n", string);
    return EAudioRendererModuleID_ALSA;
}

//-----------------------------------------------------------------------
//
// CAudioRenderer
//
//-----------------------------------------------------------------------
IFilter *CAudioRenderer::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    CAudioRenderer *result = new CAudioRenderer(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CAudioRenderer::CAudioRenderer(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
    : inherited(pName, index, pPersistMediaConfig, pEngineMsgSink)
    , mpRenderer(NULL)
    , mpCurrentInputPin(NULL)
    , mnInputPinsNumber(0)
    , mCurRendererID(EAudioRendererModuleID_Invalid)
    , mbUsePresetMode(0)
    , mpBuffer(NULL)
    , mRenderingPTS(0)
    , mPrebufferingLength(0)
    , mEstimatedPrebufferingLength(0)
    , mbRendererContentSetup(0)
    , mbRendererPaused(0)
    , mbRendererMuted(0)
    , mbRendererStarted(0)
    , mbBlendingMode(0)
    , mbRendererPrebufferReady(0)
    , mbRecievedEosSignal(0)
    , mbEos(0)
{
    TUint i = 0;
    for (i = 0; i < EConstAudioRendererMaxInputPinNumber; i ++) {
        mpInputPins[i] = NULL;
    }
}

EECode CAudioRenderer::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleAudioRendererFilter);
    //mConfigLogLevel = ELogLevel_Info;
    //mConfigLogOption = DCAL_BITMASK(ELogOption_Flow) | DCAL_BITMASK(ELogOption_State);

    mEstimatedPrebufferingLength = 8000;

    return inherited::Construct();
}

CAudioRenderer::~CAudioRenderer()
{
}

void CAudioRenderer::Delete()
{
    TUint i = 0;

    for (i = 0; i < EConstAudioRendererMaxInputPinNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }
    }

    if (mpRenderer) {
        mpRenderer->Destroy();
        mpRenderer = NULL;
    }

    inherited::Delete();
}

EECode CAudioRenderer::Initialize(TChar *p_param)
{
    EAudioRendererModuleID id;
    id = _guess_audio_renderer_type_from_string(p_param);

    LOGM_INFO("EVideoDecoderModuleID %d\n", id);

    if (mbRendererContentSetup) {

        DASSERT(mpRenderer);
        if (!mpRenderer) {
            LOGM_FATAL("[Internal bug], why the mpRenderer is NULL here?\n");
            return EECode_InternalLogicalBug;
        }

        LOGM_INFO("There's a renderer %p already, delete it first\n", mpRenderer);

        if (mbRendererStarted) {
            LOGM_INFO("before mpRenderer->Stop()\n");
            mpRenderer->Stop();
            mbRendererStarted = 0;
        }

        LOGM_INFO("before mpRenderer->DestroyContext()\n");
        if (mbRendererContentSetup) {
            mpRenderer->DestroyContext();
            mbRendererContentSetup = 0;
        }

        if (id != mCurRendererID) {
            LOGM_INFO("before mpRenderer->Delete(), cur id %d, request id %d\n", mCurRendererID, id);
            mpRenderer->Destroy();
            mpRenderer = NULL;
        }
    }

    if (!mpRenderer) {
        LOGM_INFO("before gfModuleFactoryAudioRenderer(%d)\n", id);
        mpRenderer = gfModuleFactoryAudioRenderer(id, mpPersistMediaConfig, mpEngineMsgSink);
        if (mpRenderer) {
            mCurRendererID = id;
        } else {
            mCurRendererID = EAudioRendererModuleID_Auto;
            LOGM_FATAL("[Internal bug], request gfModuleFactoryAudioRenderer(%d) fail\n", id);
            return EECode_InternalLogicalBug;
        }
    }

    if (mbUsePresetMode) {
        SAudioParams params;

        memset(&params, 0x0, sizeof(params));
        params.sample_rate = DDefaultAudioSampleRate;
        params.sample_format = AudioSampleFMT_S16;
        params.channel_number = DDefaultAudioChannelNumber;
        params.bitrate = 128000;
        params.frame_size = 1024;

        LOGM_INFO("[AudioRenderer flow], before mpRenderer->SetupContext()\n");

        mpRenderer->SetupContext(&params);
        mbRendererContentSetup = 1;
    }

    LOGM_INFO("[AudioRenderer flow], CAudioRenderer::Initialize() done\n");

    return EECode_OK;
}

EECode CAudioRenderer::Finalize()
{
    if (mpRenderer) {
        if (mbRendererStarted) {
            LOGM_INFO("before mpRenderer->Stop()\n");
            mpRenderer->Stop();
            mbRendererStarted = 0;
        }

        LOGM_INFO("before mpRenderer->DestroyContext()\n");
        DASSERT(mbRendererContentSetup);
        if (mbRendererContentSetup) {
            mpRenderer->DestroyContext();
            mbRendererContentSetup = 0;
        }

        LOGM_INFO("before mpRenderer->Delete(), cur id %d\n", mCurRendererID);
        mpRenderer->Destroy();
        mpRenderer = NULL;
    }

    return EECode_OK;
}

EECode CAudioRenderer::Run()
{
    //debug assert
    DASSERT(mpWorkQ);
    DASSERT(mpRenderer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    //    DASSERT(mbRendererContentSetup);

    mpWorkQ->SendCmd(ECMDType_StartRunning);

    return EECode_OK;
}

EECode CAudioRenderer::Start()
{
    //debug assert
    DASSERT(mpWorkQ);
    DASSERT(mpRenderer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    //    DASSERT(mbRendererContentSetup);

    mpWorkQ->SendCmd(ECMDType_Start);

    return EECode_OK;
}

EECode CAudioRenderer::Stop()
{
    //debug assert
    DASSERT(mpWorkQ);
    DASSERT(mpRenderer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    //DASSERT(mbRendererContentSetup);

    mpWorkQ->SendCmd(ECMDType_Stop);

    return EECode_OK;
}

void CAudioRenderer::Pause()
{
    //debug assert
    DASSERT(mpWorkQ);
    DASSERT(mpRenderer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    DASSERT(mbRendererContentSetup);

    mpWorkQ->PostMsg(ECMDType_Pause);

    return;
}

void CAudioRenderer::Resume()
{
    //debug assert
    DASSERT(mpWorkQ);
    DASSERT(mpRenderer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    DASSERT(mbRendererContentSetup);

    mpWorkQ->PostMsg(ECMDType_Resume);

    return;
}

void CAudioRenderer::Flush()
{
    //debug assert
    DASSERT(mpRenderer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    DASSERT(mbRendererContentSetup);

    mpWorkQ->SendCmd(ECMDType_Flush);

    return;
}

void CAudioRenderer::ResumeFlush()
{
    inherited::ResumeFlush();
    return;
}

EECode CAudioRenderer::Suspend(TUint release_context)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CAudioRenderer::ResumeSuspend(TComponentIndex input_index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CAudioRenderer::SwitchInput(TComponentIndex focus_input_index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CAudioRenderer::FlowControl(EBufferType flowcontrol_type)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

EECode CAudioRenderer::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    LOGM_FATAL("CAudioRenderer do not have output pin\n");
    return EECode_InternalLogicalBug;
}

EECode CAudioRenderer::AddInputPin(TUint &index, TUint type)
{
    if (StreamType_Audio == type) {
        if (mnInputPinsNumber >= EConstAudioRendererMaxInputPinNumber) {
            LOGM_ERROR("Max input pin number reached.\n");
            return EECode_InternalLogicalBug;
        }

        index = mnInputPinsNumber;
        DASSERT(!mpInputPins[mnInputPinsNumber]);
        if (mpInputPins[mnInputPinsNumber]) {
            LOGM_FATAL("mpInputPins[mnInputPinsNumber] here, must have problems!!! please check it\n");
        }

        mpInputPins[mnInputPinsNumber] = CQueueInputPin::Create("[Audio input pin for CAudioRenderer]", (IFilter *) this, StreamType_Audio, mpWorkQ->MsgQ());
        DASSERT(mpInputPins[mnInputPinsNumber]);

        if (!mpCurrentInputPin) {
            mpCurrentInputPin = mpInputPins[mnInputPinsNumber];
            DASSERT(0 == mnInputPinsNumber);
        } else {
            mpInputPins[mnInputPinsNumber]->Enable(0);
        }

        mnInputPinsNumber ++;
        return EECode_OK;
    } else {
        LOGM_FATAL("BAD input pin type %d\n", type);
        return EECode_InternalLogicalBug;
    }

    return EECode_InternalLogicalBug;
}

IOutputPin *CAudioRenderer::GetOutputPin(TUint index, TUint sub_index)
{
    LOGM_FATAL("CAudioRenderer do not have output pin\n");
    return NULL;
}

IInputPin *CAudioRenderer::GetInputPin(TUint index)
{
    if (EConstAudioRendererMaxInputPinNumber > index) {
        return mpInputPins[index];
    } else {
        LOGM_ERROR("BAD index %d, max value %d, in CAudioRenderer::GetInputPin()\n", index, EConstAudioRendererMaxInputPinNumber);
    }

    return NULL;
}

EECode CAudioRenderer::FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
    EECode ret = EECode_OK;

    switch (property_type) {

        case EFilterPropertyType_arenderer_sync_to_video:
            LOGM_ERROR("TO DO\n");
            ret = EECode_NoImplement;
            break;

        case EFilterPropertyType_arenderer_mute:
            SendCmd(ECMDType_MuteAudio);
            break;

        case EFilterPropertyType_arenderer_umute:
            break;

        case EFilterPropertyType_arenderer_amplify:
            LOGM_ERROR("TO DO\n");
            ret = EECode_NoImplement;
            break;

        case EFilterPropertyType_arenderer_pause:
            PostMsg(ECMDType_Pause);
            break;

        case EFilterPropertyType_arenderer_resume:
            PostMsg(ECMDType_Resume);
            break;

        default:
            LOGM_FATAL("BAD property 0x%08x\n", property_type);
            ret = EECode_InternalLogicalBug;
            break;
    }

    return ret;
}

void CAudioRenderer::GetInfo(INFO &info)
{
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = mnInputPinsNumber;
    info.nOutput = 1;

    info.pName = mpModuleName;
}

void CAudioRenderer::PrintStatus()
{
    TUint i = 0;

    LOGM_PRINTF("msState=%d, %s, heart beat %d, mPrebufferingLength %lld\n", msState, gfGetModuleStateString(msState), mDebugHeartBeat, mPrebufferingLength);

    for (i = 0; i < mnInputPinsNumber; i ++) {
        LOGM_PRINTF("       inputpin[%p, %d]'s status:\n", mpInputPins[i], i);
        if (mpInputPins[i]) {
            mpInputPins[i]->PrintStatus();
        }
    }

    mDebugHeartBeat = 0;
}

void CAudioRenderer::processCmd(SCMD cmd)
{
    LOGM_FLOW("processCmd, cmd.code %d, %s, state %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code), msState, gfGetModuleStateString(msState));

    switch (cmd.code) {
        case ECMDType_ExitRunning:
            mbRun = 0;
            if (mpBuffer) {
                mpBuffer->Release();
                mpBuffer = NULL;
            }
            msState = EModuleState_Halt;
            mpWorkQ->CmdAck(EECode_OK);
            LOGM_INFO("processCmd, ECMDType_ExitRunning.\n");
            break;

        case ECMDType_Stop:
            if (mpBuffer) {
                mpBuffer->Release();
                mpBuffer = NULL;
            }
            msState = EModuleState_WaitCmd;
            mpWorkQ->CmdAck(EECode_OK);
            LOGM_INFO("processCmd, ECMDType_Stop.\n");
            break;

        case ECMDType_Pause:
            DASSERT(!mbPaused);
            msState = EModuleState_Pending;
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
            if (mpBuffer) {
                mpBuffer->Release();
                mpBuffer = NULL;
            }
            if (mpCurrentInputPin) {
                mpCurrentInputPin->Purge(0);
            }
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
                msState = EModuleState_Renderer_PreBuffering;
            }
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_NotifySourceFilterBlocked:
            LOGM_INFO("processCmd, got ECMDType_NotifySourceFilterBlocked\n");
            break;

        case ECMDType_MuteAudio:
            flushInputData();
            mpWorkQ->CmdAck(EECode_OK);
            break;

        default:
            LOGM_ERROR("processCmd, wrong cmd %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code));
            break;
    }
}

void CAudioRenderer::OnRun()
{
    SCMD cmd;
    CIQueue::QType type;
    CIQueue::WaitResult result;
    CQueueInputPin *pPin;
    SMSG msg;

    DASSERT(mpRenderer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    //    DASSERT(mbRendererContentSetup);
    DASSERT(!mbRendererStarted);

    mbRun = 1;
    msState = EModuleState_WaitCmd;
    mpWorkQ->CmdAck(EECode_OK);
    LOGM_INFO("OnRun loop, start\n");

    DASSERT(mpCurrentInputPin);

    while (mbRun) {
        LOGM_STATE("OnRun: start switch, msState=%d, mpInputPins[0]->mpBufferQ->GetDataCnt() %d.\n", msState, mpInputPins[0]->GetBufferQ()->GetDataCnt());

        switch (msState) {

            case EModuleState_WaitCmd:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EModuleState_Halt:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EModuleState_Renderer_PreBuffering:
                DASSERT(!mbRendererPrebufferReady);

                if (!mbBlendingMode) {
                    DASSERT(mpCurrentInputPin);
                    type = mpWorkQ->WaitDataMsgWithSpecifiedQueue(&cmd, sizeof(cmd), mpCurrentInputPin->GetBufferQ());
                } else {
                    type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);
                }

                if (type == CIQueue::Q_MSG) {
                    processCmd(cmd);
                } else {
                    if (!mbBlendingMode) {
                        pPin = mpCurrentInputPin;
                    } else {
                        pPin = (CQueueInputPin *)result.pOwner;
                    }

                    if (!pPin->PeekBuffer(mpBuffer)) {
                        LOGM_FATAL("No buffer?\n");
                        msState = EModuleState_Error;
                        break;
                    }

                    if (EBufferType_AudioPCMBuffer == mpBuffer->GetBufferType()) {

                        if (DUnlikely(!mbRendererContentSetup)) {
                            DASSERT(mpBuffer->GetBufferFlags() & CIBuffer::SYNC_POINT);
                            if (mpBuffer->GetBufferFlags() & CIBuffer::SYNC_POINT) {
                                SAudioParams params;

                                memset(&params, 0x0, sizeof(params));
                                params.sample_rate = mpBuffer->mAudioSampleRate;
                                params.sample_format = AudioSampleFMT_S16;
                                params.channel_number = mpBuffer->mAudioChannelNumber;
                                params.bitrate = mpBuffer->mAudioBitrate;
                                params.frame_size = mpBuffer->mAudioFrameSize;

                                LOGM_NOTICE("[AudioRenderer flow], before mpRenderer->SetupContext(), params.sample_rate %d, params.channel_number %d, params.bitrate %d\n", params.sample_rate, params.channel_number, params.bitrate);

                                mpRenderer->SetupContext(&params);
                                mbRendererContentSetup = 1;
                            }
                        }

                        mpRenderer->Render(mpBuffer);
                        mPrebufferingLength += mpBuffer->GetDataSize() / 2 / 2;
                        mpBuffer->Release();
                        mpBuffer = NULL;
                        if (mPrebufferingLength >= mEstimatedPrebufferingLength) {
                            SMSG msg;
                            LOGM_NOTICE("Prebuffering done, mPrebufferingLength %lld, mEstimatedPrebufferingLength %lld.\n", mPrebufferingLength, mEstimatedPrebufferingLength);
                            msState = EModuleState_Renderer_PreBufferingDone;
                            msg.code = EMSGType_AudioPrecacheReadyNotify;
                            msg.p_owner = 0;
                            msg.owner_id = (TGenericID) DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_AudioRenderer, mIndex);
                            mpEngineMsgSink->MsgProc(msg);
                            mbRendererPrebufferReady = 1;
                            mpRenderer->Start();
                        }
                        break;
                    } else if (EBufferType_FlowControl_EOS == mpBuffer->GetBufferType()) {
                        mpRenderer->Stop();
                        mpBuffer->Release();
                        mpBuffer = NULL;
                        msg.code = EMSGType_PlaybackEOS;
                        msg.p1 = (TLong)this;
                        LOGM_WARN("Get EOS buffer in EModuleState_Renderer_PreBuffering.\n");
                        mpEngineMsgSink->MsgProc(msg);
                        msState = EModuleState_Completed;
                        break;
                    } else if (EBufferType_FlowControl_Pause == mpBuffer->GetBufferType()) {
                        LOGM_FATAL("TO DO: flow control(pause) comes.\n");
                        mpRenderer->Pause();
                        mpBuffer->Release();
                        mpBuffer = NULL;
                        break;
                    } else {
                        LOGM_FATAL("BAD buffer type %d\n", mpBuffer->GetBufferType());
                        mpBuffer->Release();
                        mpBuffer = NULL;
                        msg.code = EMSGType_InternalBug;
                        msg.p1 = (TLong)this;
                        mpEngineMsgSink->MsgProc(msg);
                        msState = EModuleState_Error;
                        break;
                    }
                }
                break;

            case EModuleState_Renderer_PreBufferingDone:
                DASSERT(mbRendererPrebufferReady);
                msState = EModuleState_Idle;
                //mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                //processCmd(cmd);
                break;

            case EModuleState_Idle:
                DASSERT(mbRendererPrebufferReady);

                if (!mbBlendingMode) {
                    DASSERT(mpCurrentInputPin);
                    type = mpWorkQ->WaitDataMsgWithSpecifiedQueue(&cmd, sizeof(cmd), mpCurrentInputPin->GetBufferQ());
                } else {
                    type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);
                }

                if (type == CIQueue::Q_MSG) {
                    processCmd(cmd);
                } else {

                    if (!mbBlendingMode) {
                        pPin = mpCurrentInputPin;
                    } else {
                        pPin = (CQueueInputPin *)result.pOwner;
                    }

                    if (!pPin->PeekBuffer(mpBuffer)) {
                        LOGM_FATAL("No buffer?\n");
                        return ;
                    }

                    if (EBufferType_AudioPCMBuffer == mpBuffer->GetBufferType()) {
                        msState = EModuleState_Running;
                        break;
                    } else if (EBufferType_FlowControl_EOS == mpBuffer->GetBufferType()) {
                        mpRenderer->Stop();
                        mpBuffer->Release();
                        mpBuffer = NULL;
                        msg.code = EMSGType_PlaybackEOS;
                        msg.p1 = (TLong)this;
                        LOGM_INFO("Get EOS buffer in EModuleState_Idle.\n");
                        mpEngineMsgSink->MsgProc(msg);
                        msState = EModuleState_Completed;
                        break;
                    } else if (EBufferType_FlowControl_Pause == mpBuffer->GetBufferType()) {
                        LOGM_FATAL("TO DO: flow control(pause) comes.\n");
                        mpRenderer->Pause();
                        mpBuffer->Release();
                        mpBuffer = NULL;
                        break;
                    } else {
                        LOGM_FATAL("BAD buffer type %d\n", mpBuffer->GetBufferType());
                        mpBuffer->Release();
                        mpBuffer = NULL;
                        msg.code = EMSGType_InternalBug;
                        msg.p1 = (TLong)this;
                        mpEngineMsgSink->MsgProc(msg);
                        msState = EModuleState_Error;
                        break;
                    }
                }
                break;

            case EModuleState_Running:
                DASSERT(mpBuffer);
                DASSERT(mpRenderer);
                if (mpBuffer) {
                    if (DUnlikely(EBufferType_FlowControl_Pause == mpBuffer->GetBufferType())) {
                        LOGM_FATAL("TO DO: flow control(pause) comes.\n");
                        mpRenderer->Pause();
                        mpBuffer->Release();
                        mpBuffer = NULL;
                        msState = EModuleState_Idle;
                        break;
                    }

                    DASSERT(EBufferType_AudioPCMBuffer == mpBuffer->GetBufferType());
                    if (mpRenderer) {
                        if (DUnlikely(!mbRendererContentSetup)) {
                            DASSERT(mpBuffer->GetBufferFlags() & CIBuffer::SYNC_POINT);
                            if (mpBuffer->GetBufferFlags() & CIBuffer::SYNC_POINT) {
                                SAudioParams params;

                                memset(&params, 0x0, sizeof(params));
                                params.sample_rate = mpBuffer->mAudioSampleRate;
                                params.sample_format = AudioSampleFMT_S16;
                                params.channel_number = mpBuffer->mAudioChannelNumber;
                                params.bitrate = mpBuffer->mAudioBitrate;

                                LOGM_NOTICE("[AudioRenderer flow], before mpRenderer->SetupContext(), params.sample_rate %d, params.channel_number %d, params.bitrate %d\n", params.sample_rate, params.channel_number, params.bitrate);

                                mpRenderer->SetupContext(&params);
                                mbRendererContentSetup = 1;
                            }
                        }
                        //LOGM_NOTICE("%ld\n", mpBuffer->GetDataSize());
                        mpRenderer->Render(mpBuffer);
                    }
                    mpBuffer->Release();
                    mpBuffer = NULL;
                }
                msState = EModuleState_Idle;
                break;

            case EModuleState_Completed:
            case EModuleState_Pending:
            case EModuleState_Stopped:
            case EModuleState_Error:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EModuleState_Bypass:

                if (!mbBlendingMode) {
                    DASSERT(mpCurrentInputPin);
                    type = mpWorkQ->WaitDataMsgWithSpecifiedQueue(&cmd, sizeof(cmd), mpCurrentInputPin->GetBufferQ());
                } else {
                    type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);
                }

                if (type == CIQueue::Q_MSG) {
                    processCmd(cmd);
                } else {

                    if (!mbBlendingMode) {
                        pPin = mpCurrentInputPin;
                    } else {
                        pPin = (CQueueInputPin *)result.pOwner;
                    }

                    if (!pPin->PeekBuffer(mpBuffer)) {
                        LOGM_FATAL("No buffer?\n");
                        msState = EModuleState_Error;
                        break;
                    }
                    mpBuffer->Release();
                    mpBuffer = NULL;
                }
                break;

            default:
                LOGM_FATAL("CAudioRenderer: BAD state=%d.\n", msState);
                msState = EModuleState_Error;
                break;
        }

        mDebugHeartBeat ++;
    }

    LOGM_INFO("OnRun loop end.\n");
}

EECode CAudioRenderer::flushInputData()
{
    TUint index = 0;

    if (mpBuffer) {
        mpBuffer->Release();
        mpBuffer = NULL;
    }

    LOGM_INFO("before purge input pins\n");

    for (index = 0; index < EConstAudioRendererMaxInputPinNumber; index ++) {
        if (mpInputPins[index]) {
            mpInputPins[index]->Purge();
        }
    }

    LOGM_INFO("after purge input pins\n");

    return EECode_OK;
}

EECode CAudioRenderer::stopAudioRenderer()
{
    DASSERT(mbRendererStarted);
    DASSERT(mpRenderer);

    if (mpRenderer && mbRendererStarted) {
        mpRenderer->Stop();
        mbRendererStarted = 0;
        return EECode_OK;
    } else if (!mpRenderer) {
        LOGM_FATAL("NULL mpRenderer.\n");
        return EECode_InternalLogicalBug;
    }

    LOGM_WARN("mbRendererStarted is 0.\n");
    return EECode_OK;
}

EECode CAudioRenderer::startAudioRenderer()
{
    DASSERT(!mbRendererStarted);
    DASSERT(mpRenderer);

    if (mpRenderer && (!mbRendererStarted)) {
        LOGM_INFO("before mpRenderer->Start()\n");
        mpRenderer->Start();
        LOGM_INFO("after mpRenderer->Start()\n");
        mbRendererStarted = 1;
        return EECode_OK;
    } else if (!mpRenderer) {
        LOGM_FATAL("NULL mpRenderer.\n");
        return EECode_InternalLogicalBug;
    }

    LOGM_WARN("mbRendererStarted is 1.\n");
    return EECode_OK;
}

EECode CAudioRenderer::flushAudioRenderer()
{
    DASSERT(mbRendererStarted);
    DASSERT(mpRenderer);

    if (mpRenderer && mbRendererStarted) {
        mpRenderer->Flush();
        return EECode_OK;
    } else if (!mpRenderer) {
        LOGM_FATAL("NULL mpRenderer.\n");
        return EECode_InternalLogicalBug;
    }

    LOGM_WARN("mbRendererStarted is 0.\n");
    return EECode_OK;
}

EECode CAudioRenderer::pauseAudioRenderer()
{
    DASSERT(mbRendererStarted);
    DASSERT(!mbRendererPaused);
    DASSERT(mpRenderer);

    if (mpRenderer && mbRendererStarted && (!mbRendererPaused)) {
        mpRenderer->Pause();
        mbRendererPaused = 1;
        return EECode_OK;
    } else if (!mpRenderer) {
        LOGM_FATAL("NULL mpRenderer.\n");
        return EECode_InternalLogicalBug;
    }

    LOGM_WARN("mbRendererStarted is %d, mbRendererPaused is %d.\n", mbRendererStarted, mbRendererPaused);
    return EECode_OK;
}

EECode CAudioRenderer::resumeAudioRenderer()
{
    DASSERT(mbRendererStarted);
    DASSERT(mbRendererPaused);
    DASSERT(mpRenderer);

    if (mpRenderer && mbRendererStarted && mbRendererPaused) {
        mpRenderer->Resume();
        mbRendererPaused = 0;
        return EECode_OK;
    } else if (!mpRenderer) {
        LOGM_FATAL("NULL mpRenderer.\n");
        return EECode_InternalLogicalBug;
    }

    LOGM_WARN("mbRendererStarted is %d, mbRendererPaused is %d.\n", mbRendererStarted, mbRendererPaused);
    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

