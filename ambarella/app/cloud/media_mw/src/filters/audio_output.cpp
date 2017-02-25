/*******************************************************************************
 * audio_output.cpp
 *
 * History:
 *    2014/11/20 - [Zhi He] create file
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
#include "mw_internal.h"
#include "media_mw_utils.h"

#include "framework_interface.h"
#include "filters_interface.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "audio_output.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IFilter *gfCreateAudioOutputFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CAudioOutput::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

//-----------------------------------------------------------------------
//
// CAudioOutput
//
//-----------------------------------------------------------------------
IFilter *CAudioOutput::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    CAudioOutput *result = new CAudioOutput(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CAudioOutput::CAudioOutput(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
    : inherited(pName, index, pPersistMediaConfig, pEngineMsgSink)
    , mpRenderer(NULL)
    , mpCurrentInputPin(NULL)
    , mnInputPinsNumber(0)
    , mpBuffer(NULL)
    , mpRenderingBuffer(NULL)
    , mpTimer(NULL)
    , mbPaused(0)
    , mbEOS(0)
    , mSyncStrategy(0)
    , mbSyncStarted(0)
    , mSyncOverflowThreshold(0)
    , mSyncInitialCachedFrameCount(0)
    , mSyncPlaybackCurTime(0)
    , mSyncBaseTime(0)
    , mSyncDriftTime(0)
    , mSyncEstimatedNextFrameTime(0)
    , mSyncFrameDurationNum((TTime)DDefaultAudioFrameSize * (TTime)1000000)
    , mSyncFrameDurationDen(DDefaultAudioSampleRate)
    , mCurrentFramCount(0)
{
    TUint i = 0;

    for (i = 0; i < EConstAudioRendererMaxInputPinNumber; i ++) {
        mpInputPins[i] = 0;
    }
}

EECode CAudioOutput::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleAudioRendererFilter);

    //mConfigLogLevel = ELogLevel_Debug;
    //mConfigLogOption = DCAL_BITMASK(ELogOption_State) | DCAL_BITMASK(ELogOption_Resource) | DCAL_BITMASK(ELogOption_Flow);

    mSyncInitialCachedFrameCount = mpPersistMediaConfig->pb_cache.precache_audio_frames;
    if (mpPersistMediaConfig->pb_cache.b_constrain_latency) {
        mSyncOverflowThreshold = mpPersistMediaConfig->pb_cache.max_audio_frames + mpPersistMediaConfig->pb_cache.precache_audio_frames;
    } else {
        mSyncOverflowThreshold = 0;
    }

    return inherited::Construct();
}

CAudioOutput::~CAudioOutput()
{
    TUint i = 0;

    LOGM_RESOURCE("CAudioOutput::~CAudioOutput(), before delete inputpins.\n");
    for (i = 0; i < EConstAudioRendererMaxInputPinNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }
    }

    LOGM_RESOURCE("CAudioOutput::~CAudioOutput(), end.\n");
}

void CAudioOutput::Delete()
{
    TUint i = 0;

    LOGM_RESOURCE("CAudioOutput::Delete(), before delete inputpins.\n");
    for (i = 0; i < EConstAudioRendererMaxInputPinNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }
    }

    inherited::Delete();
}

EECode CAudioOutput::Initialize(TChar *p_param)
{
    return EECode_OK;
}

EECode CAudioOutput::Finalize()
{
    return EECode_OK;
}

void CAudioOutput::PrintState()
{
    LOGM_FATAL("TO DO\n");
}

EECode CAudioOutput::Run()
{
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    inherited::Run();

    return EECode_OK;
}

EECode CAudioOutput::Start()
{
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    return mpWorkQ->SendCmd(ECMDType_Start);
}

EECode CAudioOutput::Stop()
{
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    inherited::Stop();

    return EECode_OK;
}

void CAudioOutput::Pause()
{
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    inherited::Pause();

    return;
}

void CAudioOutput::Resume()
{
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    inherited::Resume();

    return;
}

void CAudioOutput::Flush()
{
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    inherited::Flush();

    return;
}

void CAudioOutput::ResumeFlush()
{
    LOGM_FATAL("TO DO\n");
    return;
}

EECode CAudioOutput::Suspend(TUint release_context)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CAudioOutput::ResumeSuspend(TComponentIndex input_index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CAudioOutput::SwitchInput(TComponentIndex focus_input_index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CAudioOutput::FlowControl(EBufferType flowcontrol_type)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

IInputPin *CAudioOutput::GetInputPin(TUint index)
{
    if (EConstAudioRendererMaxInputPinNumber > index) {
        return mpInputPins[index];
    } else {
        LOGM_ERROR("BAD index %d, max value %d, in CAudioOutput::GetInputPin()\n", index, EConstAudioRendererMaxInputPinNumber);
    }

    return NULL;
}

IOutputPin *CAudioOutput::GetOutputPin(TUint index, TUint sub_index)
{
    LOGM_FATAL("CAudioOutput do not have output pin\n");
    return NULL;
}

EECode CAudioOutput::AddInputPin(TUint &index, TUint type)
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

        mpInputPins[mnInputPinsNumber] = CQueueInputPin::Create("[audio input pin for CAudioOutput]", (IFilter *) this, StreamType_Audio, mpWorkQ->MsgQ());
        DASSERT(mpInputPins[mnInputPinsNumber]);

        mnInputPinsNumber ++;
        return EECode_OK;
    } else {
        LOGM_FATAL("BAD input pin type %d\n", type);
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CAudioOutput::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    LOGM_FATAL("CAudioOutput can not add a output pin\n");
    return EECode_OK;
}

void CAudioOutput::EventNotify(EEventType type, TU64 param1, TPointer param2)
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

EECode CAudioOutput::FilterProperty(EFilterPropertyType property, TUint set_or_get, void *p_param)
{
    EECode err = EECode_OK;

    switch (property) {

        case EFilterPropertyType_assign_external_module:
            LOGM_NOTICE("EFilterPropertyType_assign_external_module %p\n", p_param);
            mpRenderer = (ISoundDirectRendering *) p_param;
            break;

        default:
            LOGM_FATAL("BAD property 0x%08x\n", property);
            err = EECode_InternalLogicalBug;
            break;
    }

    return err;
}

void CAudioOutput::GetInfo(INFO &info)
{
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = 1;
    info.nOutput = 0;

    info.pName = mpModuleName;
}

void CAudioOutput::PrintStatus()
{
    TUint i = 0;

    LOGM_PRINTF("msState=%d, %s\n", msState, gfGetModuleStateString(msState));

    for (i = 0; i < mnInputPinsNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->PrintStatus();
        }
    }

    return;
}

bool CAudioOutput::readInputData(CQueueInputPin *inputpin, EBufferType &type)
{
    DASSERT(!mpBuffer);
    DASSERT(inputpin);

    if (!inputpin->PeekBuffer(mpBuffer)) {
        LOGM_FATAL("No buffer?\n");
        return false;
    }

    type = mpBuffer->GetBufferType();

    if (EBufferType_FlowControl_EOS == type) {
        LOGM_FLOW("CAudioOutput %p get EOS.\n", this);
        DASSERT(!mbEOS);
        mbEOS = 1;
        return true;
    } else {
        LOGM_ERROR("mpBuffer->GetBufferType() %d, mpBuffer->mFlags() %d.\n", mpBuffer->GetBufferType(), mpBuffer->mFlags);
        return true;
    }

    return true;
}

EECode CAudioOutput::flushInputData()
{
    if (mpRenderingBuffer) {
        mpRenderingBuffer->Release();
        mpRenderingBuffer = NULL;
    }

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

EECode CAudioOutput::processCmd(SCMD &cmd)
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
            msState = EModuleState_Stopped;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case EMSGType_TimeNotify:
            if (mpBuffer) {
                renderBuffer();
                mpBuffer->Release();
                mpBuffer = NULL;
                msState = EModuleState_Idle;
            }
            mpTimer = NULL;
            break;

        default:
            LOGM_ERROR("processCmd, wrong cmd %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code));
            break;
    }

    return err;
}

void CAudioOutput::renderBuffer()
{
    if (DLikely(mpRenderer)) {
        SSoundDirectRenderingContent rendering_context;
        rendering_context.data[0] = mpBuffer->GetDataPtr(0);
        rendering_context.size = mpBuffer->GetDataSize(0);

#ifdef DCONFIG_ENABLE_REALTIME_PRINT
        if (DUnlikely(mpPersistMediaConfig->common_config.debug_dump.debug_print_audio_realtime)) {
            static TTime last_time = 0;
            static TU64 packet_count = 0;
            static TTime long_term_begin_time = 0;
            TTime cur_time = mpClockReference->GetCurrentTime();
            if (0 == packet_count) {
                long_term_begin_time = cur_time;
            }
            LOGM_PRINTF("[AO]: audio output: real time stamp gap %lld, long term gap %lld, playback time %lld, cur time %lld\n", cur_time - last_time, (packet_count * 1024 * 1000000) / (48000) + long_term_begin_time - cur_time, (packet_count * 1024 * 1000000) / (48000), cur_time);
            last_time = cur_time;
            packet_count ++;
        }
#endif

        EECode err = mpRenderer->Render(&rendering_context, 1);
        if (DLikely(EECode_OK == err)) {
            if (mpRenderingBuffer) {
                mpRenderingBuffer->Release();
            }
            mpRenderingBuffer = mpBuffer;
            mpRenderingBuffer->AddRef();
        }
    }
}

void CAudioOutput::initSync()
{
    LOGM_NOTICE("[config]: audio, duration %lld\n", mSyncFrameDurationNum / mSyncFrameDurationDen);
    mSyncBaseTime = mpClockReference->GetCurrentTime();
    mSyncDriftTime = 0;
    mCurrentFramCount = 0;
}

TTime CAudioOutput::getEstimatedNextFrameTime()
{
    mSyncPlaybackCurTime = mpClockReference->GetCurrentTime();
    mSyncEstimatedNextFrameTime = mSyncBaseTime + mSyncDriftTime + mCurrentFramCount * mSyncFrameDurationNum / mSyncFrameDurationDen;
    //LOG_NOTICE("[audio sync debug]: mSyncPlaybackCurTime %lld, mSyncEstimatedNextFrameTime %lld\n", mSyncPlaybackCurTime, mSyncEstimatedNextFrameTime);
    return mSyncEstimatedNextFrameTime - mSyncPlaybackCurTime;
}

void CAudioOutput::OnRun()
{
    SCMD cmd;
    CIQueue::QType type;
    CIQueue::WaitResult result;
    CIQueue *data_queue = NULL;

    mpWorkQ->CmdAck(EECode_OK);
    LOGM_INFO("OnRun loop, start\n");

    mbRun = true;
    msState = EModuleState_WaitCmd;

    while (mbRun) {

        LOGM_STATE("CAudioOutput: start switch, msState=%d, %s\n", msState, gfGetModuleStateString(msState));

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
                        data_queue = mpCurrentInputPin->GetBufferQ();
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
                    if (DLikely(mpRenderer)) {
                        mpRenderer->ControlOutput(DCONTROL_NOTYFY_DATA_RESUME);
                    }
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
                DASSERT(mpBuffer && mpRenderer);

                if (EBufferType_FlowControl_EOS == mpBuffer->GetBufferType()) {
                    DASSERT(mpEngineMsgSink);
                    if (mpEngineMsgSink) {
                        SMSG msg;
                        msg.code = EMSGType_PlaybackEOS;
                        msg.p_owner = (TULong)((IFilter *)this);
                        mpEngineMsgSink->MsgProc(msg);
                    }
                    mpBuffer->Release();
                    mpBuffer = NULL;
                    if (DLikely(mpRenderer)) {
                        mpRenderer->ControlOutput(DCONTROL_NOTYFY_DATA_PAUSE);
                    }
                    msState = EModuleState_Completed;
                    break;
                } else {

                    if (DUnlikely(mpBuffer->GetBufferFlags() & CIBuffer::NEW_SEQUENCE)) {
                        TTime cur_frame_duration_num = (TTime)mpBuffer->mAudioFrameSize * (TTime)1000000;
                        TTime cur_frame_duration_den = mpBuffer->mAudioSampleRate;
                        LOGM_NOTICE("Force sync, new sequence comes\n");
                        mpRenderer->SetConfig(mpBuffer->mAudioSampleRate, mpBuffer->mAudioFrameSize, mpBuffer->mAudioChannelNumber, 0);
                        mSyncFrameDurationNum = cur_frame_duration_num;
                        mSyncFrameDurationDen = cur_frame_duration_den;
                        initSync();
                        mbSyncStarted = 1;
                    } else if (DUnlikely(mpBuffer->GetBufferFlags() & CIBuffer::SYNC_POINT)) {
                        TTime cur_frame_duration_num = (TTime)mpBuffer->mAudioFrameSize * (TTime)1000000;
                        TTime cur_frame_duration_den = mpBuffer->mAudioSampleRate;
                        if ((cur_frame_duration_num != mSyncFrameDurationNum) || (cur_frame_duration_den != mSyncFrameDurationDen)) {
                            LOGM_NOTICE("[audio update]: mpBuffer->mAudioFrameSize %d, mpBuffer->mAudioSampleRate %d\n", mpBuffer->mAudioFrameSize, mpBuffer->mAudioSampleRate);
                            mSyncFrameDurationNum = cur_frame_duration_num;
                            mSyncFrameDurationDen = cur_frame_duration_den;
                            initSync();
                            mbSyncStarted = 1;
                        }
                    }

                    if (DUnlikely(!mbSyncStarted)) {
                        initSync();
                        mbSyncStarted = 1;
                    }

                    if (DUnlikely(mSyncOverflowThreshold && (data_queue->GetDataCnt() > mSyncOverflowThreshold))) {
                        mSyncDriftTime -= 10000;
                        LOGM_NOTICE("[audio sync]: decrease drift, cur drift %lld\n", mSyncDriftTime);
                    }

                    TTime time_gap = getEstimatedNextFrameTime();
                    mCurrentFramCount ++;
                    //LOG_NOTICE("[audio sync debug]: time_gap %lld\n", time_gap);
                    if (time_gap < 20000) {
                        renderBuffer();
                    } else {
                        if (DUnlikely(mpTimer)) {
                            LOGM_WARN("mpTimer not NULL\n");
                            mpClockReference->RemoveTimer(mpTimer);
                            mpTimer = NULL;
                        }
                        mpTimer = mpClockReference->AddTimer(this, EClockTimerType_once, mSyncEstimatedNextFrameTime, 0, 1);
                        msState = EModuleState_WaitTiming;
                        break;
                    }
                }

                mpBuffer->Release();
                mpBuffer = NULL;
                msState = EModuleState_Idle;
                break;

            case EModuleState_WaitTiming:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            default:
                LOGM_ERROR("CAudioOutput: OnRun: state invalid: 0x%d\n", (TUint)msState);
                msState = EModuleState_Error;
                break;
        }
    }
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

